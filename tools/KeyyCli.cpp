/** Offline-Werkzeug: bestimmt Tonart, Stimmton und Tempo einer Datei — oder
    misst die Trefferquote über einen ganzen Ordner.

    Der Batch-Modus ist der eigentliche Grund für dieses Werkzeug. Ob die
    Erkennung taugt, zeigt sich nicht an einer Datei, sondern an hunderten
    mit bekannter Tonart. Sample-Packs liefern sie gratis im Dateinamen
    ("Loop_Fm_140bpm.wav"), GiantSteps in Beschriftungsdateien (--labels).
*/

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_FLOAT_OUTPUT

#include <algorithm>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "AudioLoader.h"
#include "dsp/KeyAnalyzer.h"

namespace fs = std::filesystem;

namespace
{

struct Options
{
    std::string input;
    bool batch = false;
    std::string labelsDir;
    std::string csvPath;
    std::string reportPath;
    std::vector<keyy::Profile> profiles { keyy::defaultProfile };
    keyy::AnalysisSettings settings;
    int threads = 0;
};

void printUsage()
{
    std::cout <<
        "keyy-cli <datei> [Optionen]           Tonart, Stimmton und Tempo einer Datei\n"
        "keyy-cli --batch <ordner> [Optionen]  Trefferquote ueber alle WAV/MP3 im Ordner\n"
        "\n"
        "  --profile <name|all>  Tonart-Profil: krumhansl, temperley, shaath, edm, mix (Standard)\n"
        "                        oder all, um alle zu vergleichen\n"
        "  --labels <ordner>     Tonart aus <ordner>/<name>.key statt aus dem Dateinamen\n"
        "                        (Format des GiantSteps-Datensatzes)\n"
        "  --csv <datei>         Ergebnis je Datei als CSV (nur --batch)\n"
        "  --report <datei.csv>  Chromagramm und Tonart-Werte mitschreiben (nur Einzeldatei)\n"
        "  --exponent <x>        Gewichtung der Spektralspitzen, Betrag^x (Standard 0.5)\n"
        "  --threshold <x>       Spitze muss x-mal ueber ihrer Umgebung liegen (Standard 2)\n"
        "  --tonality <x>        Rahmen nach tonalem Anteil gewichten, hoch x (Standard 1)\n"
        "  --min-hz <Hz>         Untergrenze des Chromagramms (Standard 45)\n"
        "  --max-hz <Hz>         Obergrenze des Chromagramms (Standard 3500)\n"
        "  --threads <n>         Parallele Analysen im Batch (Standard: alle Kerne)\n";
}

bool parse (int argc, char** argv, Options& options)
{
    if (argc < 2)
        return false;

    int i = 1;
    if (std::string (argv[1]) == "--batch")
    {
        if (argc < 3)
            return false;

        options.batch = true;
        options.input = argv[2];
        i = 3;
    }
    else
    {
        options.input = argv[1];
        i = 2;
    }

    for (; i < argc; ++i)
    {
        const std::string flag = argv[i];

        if (i + 1 >= argc)
        {
            std::cerr << "Wert fehlt fuer " << flag << "\n";
            return false;
        }

        const std::string value = argv[++i];

        if (flag == "--profile")
        {
            if (value == "all")
                options.profiles.assign (keyy::allProfiles.begin(), keyy::allProfiles.end());
            else if (const auto profile = keyy::profileFromName (value))
                options.profiles = { *profile };
            else
            {
                std::cerr << "Unbekanntes Profil: " << value << "\n";
                return false;
            }
        }
        else if (flag == "--labels")   options.labelsDir = value;
        else if (flag == "--csv")      options.csvPath = value;
        else if (flag == "--report")   options.reportPath = value;
        else if (flag == "--exponent") options.settings.chroma.magnitudeExponent = std::stod (value);
        else if (flag == "--threshold") options.settings.chroma.peakThreshold = std::stod (value);
        else if (flag == "--tonality") options.settings.chroma.tonalityWeighting = std::stod (value);
        else if (flag == "--bar-weight") options.settings.barWeighting = std::stod (value);
        else if (flag == "--min-hz")   options.settings.chroma.minHz = std::stod (value);
        else if (flag == "--max-hz")   options.settings.chroma.maxHz = std::stod (value);
        else if (flag == "--threads")  options.threads = std::stoi (value);
        else
        {
            std::cerr << "Unbekannte Option: " << flag << "\n";
            return false;
        }
    }

    options.settings.profile = options.profiles.front();
    return true;
}

std::string fixed (double value, int decimals)
{
    std::ostringstream s;
    s << std::fixed << std::setprecision (decimals) << value;
    return s.str();
}

std::string signedFixed (double value, int decimals)
{
    return (value >= 0.0 ? "+" : "") + fixed (value, decimals);
}

std::string percent (int count, int total)
{
    return total > 0 ? fixed (100.0 * count / total, 1) + " %" : "-";
}

// Pfad aus einem Kommandozeilenargument. Nicht u8path: Windows uebergibt argv
// in der ANSI-Codepage (Umlaute sind dort Einzelbytes und kein gueltiges UTF-8),
// unter Linux sind es rohe Bytes. Beides ist genau die native schmale Kodierung.
fs::path argPath (const std::string& arg)
{
    return fs::path (arg);
}

std::string displayName (const fs::path& path)
{
    return path.filename().u8string();
}

std::optional<keyy::AnalysisResult> analyseFile (const fs::path& path, const keyy::AnalysisSettings& settings, std::string& error)
{
    const auto audio = keyy::audio::load (path, error);
    if (audio.getNumSamples() == 0)
    {
        if (error.empty())
            error = "Datei ist leer";

        return std::nullopt;
    }

    keyy::KeyAnalyzer analyzer;
    analyzer.prepare (audio.sampleRate, settings);

    const int total = audio.getNumSamples();
    const float scale = 1.0f / static_cast<float> (audio.getNumChannels());
    constexpr int blockSize = 4096;
    std::vector<float> block (blockSize);

    for (int pos = 0; pos < total; pos += blockSize)
    {
        const int n = std::min (blockSize, total - pos);

        for (int i = 0; i < n; ++i)
        {
            float sum = 0.0f;
            for (const auto& channel : audio.channels)
                sum += channel[static_cast<size_t> (pos + i)];

            block[static_cast<size_t> (i)] = sum * scale;
        }

        analyzer.process (block.data(), n);
    }

    return analyzer.finish();
}

std::string tempoText (const keyy::AnalysisResult& r)
{
    if (! r.tempoValid)
        return "nicht erkannt";

    if (r.bpmFromLoopLength)
        return fixed (r.bpm, 0) + " BPM (aus Looplaenge; Einsaetze: " + fixed (r.onsetBpm, 1) + ")";

    return fixed (r.bpm, 1) + " BPM";
}

int runSingle (const Options& options)
{
    std::string error;
    const auto result = analyseFile (argPath (options.input), options.settings, error);

    if (! result)
    {
        std::cerr << error << "\n";
        return 1;
    }

    const auto& r = *result;
    std::cout << "Datei:      " << displayName (argPath (options.input)) << " (" << fixed (r.durationSeconds, 1) << " s)\n";

    if (! r.valid)
    {
        std::cout << "Tonart:     keine tonalen Anteile gefunden\n";
    }
    else
    {
        for (const auto profile : options.profiles)
        {
            const auto estimate = keyy::estimateKey (r.chroma, profile);

            std::cout << "Tonart:     " << std::left << std::setw (10) << estimate.key.name()
                      << "  Paralleltonart " << std::setw (10) << estimate.key.relative().name()
                      << "  [" << keyy::getProfile (profile).name << " " << fixed (estimate.score, 3)
                      << ", danach " << estimate.runnerUp.name() << " " << fixed (estimate.runnerUpScore, 3) << "]\n"
                      << std::right;
        }

        std::cout << "Stimmton:   A = " << fixed (r.referenceHz, 1) << " Hz (" << signedFixed (r.tuningCents, 1) << " Cent)\n";
    }

    std::cout << "Tempo:      " << tempoText (r) << "\n";

    if (options.settings.barWeighting > 0.0)
        std::cout << "Takt:       " << (r.barWeighted
                        ? fixed (r.barSeconds, 3) + " s, erster Taktanfang bei " + fixed (r.barPhase, 3) + " s"
                        : std::string ("keine Gewichtung -- kein Tempo erkannt")) << "\n";

    if (! options.reportPath.empty())
    {
        std::ofstream report (argPath (options.reportPath));
        report << "tonklasse,anteil\n";
        for (int pc = 0; pc < 12; ++pc)
            report << keyy::noteName (pc) << ',' << r.chroma[static_cast<size_t> (pc)] << '\n';

        report << "\ntonart,korrelation\n";
        for (int index = 0; index < 24; ++index)
            report << keyy::Key::fromIndex (index).name() << ',' << r.keyScores[static_cast<size_t> (index)] << '\n';

        std::cout << "Protokoll:  " << options.reportPath << "\n";
    }

    return 0;
}

struct Entry
{
    fs::path path;
    std::optional<keyy::Key> label;
    std::optional<double> bpmLabel;
    bool analysed = false;
    std::string error;
    keyy::AnalysisResult result;
};

std::optional<keyy::Key> readLabelFile (const fs::path& path)
{
    std::ifstream file (path);
    if (! file)
        return std::nullopt;

    const std::string content ((std::istreambuf_iterator<char> (file)), std::istreambuf_iterator<char>());
    return keyy::parseKey (content);
}

std::string csvQuote (const std::string& text)
{
    std::string out = "\"";
    for (const char c : text)
        out += (c == '"') ? std::string ("\"\"") : std::string (1, c);

    return out + "\"";
}

int runBatch (const Options& options)
{
    const fs::path root = argPath (options.input);

    if (! fs::is_directory (root))
    {
        std::cerr << "Kein Ordner: " << options.input << "\n";
        return 1;
    }

    std::vector<Entry> entries;
    for (const auto& item : fs::recursive_directory_iterator (root, fs::directory_options::skip_permission_denied))
    {
        if (! item.is_regular_file() || ! keyy::audio::isSupported (item.path()))
            continue;

        Entry entry;
        entry.path = item.path();

        const auto stem = entry.path.stem().u8string();
        entry.label = options.labelsDir.empty() ? keyy::parseKey (stem)
                                                : readLabelFile (argPath (options.labelsDir) / (entry.path.stem().native() + fs::path (".key").native()));
        entry.bpmLabel = keyy::parseBpm (stem);
        entries.push_back (std::move (entry));
    }

    std::sort (entries.begin(), entries.end(), [] (const Entry& a, const Entry& b) { return a.path < b.path; });

    std::vector<size_t> todo;
    for (size_t i = 0; i < entries.size(); ++i)
        if (entries[i].label || entries[i].bpmLabel)
            todo.push_back (i);

    const int numThreads = options.threads > 0 ? options.threads
                                               : std::max (1, static_cast<int> (std::thread::hardware_concurrency()));

    std::atomic<size_t> next { 0 }, done { 0 };
    std::mutex printLock;

    const auto worker = [&]
    {
        for (size_t k = next++; k < todo.size(); k = next++)
        {
            auto& entry = entries[todo[k]];
            std::string error;

            if (const auto result = analyseFile (entry.path, options.settings, error))
            {
                entry.result = *result;
                entry.analysed = true;
            }
            else
            {
                entry.error = error;
            }

            const size_t finished = ++done;
            if (finished % 10 == 0 || finished == todo.size())
            {
                const std::lock_guard<std::mutex> guard (printLock);
                std::cerr << "\r" << finished << " / " << todo.size() << std::flush;
            }
        }
    };

    std::vector<std::thread> pool;
    for (int t = 0; t < numThreads; ++t)
        pool.emplace_back (worker);

    for (auto& thread : pool)
        thread.join();

    if (! todo.empty())
        std::cerr << "\n";

    int withKey = 0, withBpm = 0, unreadable = 0;
    for (const auto& e : entries)
    {
        if (e.label)    ++withKey;
        if (e.bpmLabel) ++withBpm;
        if (! e.error.empty()) ++unreadable;
    }

    std::cout << "\nDateien: " << entries.size() << " gefunden, " << withKey << " mit Tonart, "
              << withBpm << " mit Tempo, " << (entries.size() - todo.size()) << " ohne Angabe uebersprungen";
    if (unreadable > 0)
        std::cout << ", " << unreadable << " nicht lesbar";
    std::cout << "\n\n";

    // Tonart je Profil. Das Chromagramm ist fuer alle Profile dasselbe,
    // die Analyse lief deshalb nur einmal je Datei.
    // "Tonleiter" zaehlt Tonart und Paralleltonart zusammen: Beide bestehen
    // aus denselben sieben Toenen, und fuer die Tonhoehenkorrektur ist genau
    // das die Frage.
    std::cout << "Profil       Dateien   exakt    Quinte   Parallele Gleichn.  andere   MIREX  Tonleiter\n";

    for (const auto profile : options.profiles)
    {
        int counts[5] {};
        int n = 0;
        double mirex = 0.0;

        for (const auto& e : entries)
        {
            if (! e.label || ! e.analysed || ! e.result.valid)
                continue;

            const auto estimated = keyy::estimateKey (e.result.chroma, profile).key;
            ++counts[static_cast<int> (keyy::relationOf (estimated, *e.label))];
            mirex += keyy::mirexScore (estimated, *e.label);
            ++n;
        }

        std::cout << std::left << std::setw (13) << keyy::getProfile (profile).name << std::right
                  << std::setw (7) << n;

        for (const int count : counts)
            std::cout << std::setw (9) << percent (count, n);

        const int scale = counts[static_cast<int> (keyy::KeyRelation::Same)]
                        + counts[static_cast<int> (keyy::KeyRelation::Relative)];

        std::cout << std::setw (8) << (n > 0 ? fixed (100.0 * mirex / n, 1) : std::string ("-"))
                  << std::setw (10) << percent (scale, n) << "\n";
    }

    // Tempo: exakt, und mit /2 bzw. x2 korrigierbar.
    int tempoTotal = 0, tempoExact = 0, tempoOctave = 0, tempoLoop = 0, tempoMissing = 0;
    for (const auto& e : entries)
    {
        if (! e.bpmLabel || ! e.analysed)
            continue;

        ++tempoTotal;
        if (! e.result.tempoValid)
        {
            ++tempoMissing;
            continue;
        }

        const double label = *e.bpmLabel, bpm = e.result.bpm;
        const bool exact = std::abs (bpm - label) <= 0.5;
        const bool octave = exact || std::abs (bpm * 2.0 - label) <= 1.0 || std::abs (bpm / 2.0 - label) <= 0.5;

        tempoExact  += exact ? 1 : 0;
        tempoOctave += octave ? 1 : 0;
        tempoLoop   += e.result.bpmFromLoopLength ? 1 : 0;
    }

    if (tempoTotal > 0)
        std::cout << "\nTempo: " << tempoTotal << " Dateien, exakt (+-0,5) " << percent (tempoExact, tempoTotal)
                  << ", mit /2 oder x2 " << percent (tempoOctave, tempoTotal)
                  << ", " << tempoLoop << " aus Looplaenge, " << tempoMissing << " nicht erkannt\n";

    // Fehlgriffe des ersten Profils einzeln — die gehoeren angehoert, nicht
    // nur gezaehlt. Oft ist die Beschriftung selbst falsch.
    const auto primary = options.profiles.front();
    std::vector<const Entry*> misses;
    for (const auto& e : entries)
        if (e.label && e.analysed && e.result.valid
            && keyy::estimateKey (e.result.chroma, primary).key != *e.label)
            misses.push_back (&e);

    if (! misses.empty())
    {
        std::cout << "\nDaneben (" << keyy::getProfile (primary).name << "), erwartet -> erkannt:\n";
        for (size_t i = 0; i < misses.size() && i < 40; ++i)
        {
            const auto& e = *misses[i];
            const auto estimated = keyy::estimateKey (e.result.chroma, primary).key;
            std::cout << "  " << std::left << std::setw (5) << e.label->shortName() << " -> " << std::setw (5) << estimated.shortName()
                      << std::setw (13) << (std::string (" (") + keyy::relationName (keyy::relationOf (estimated, *e.label)) + ")")
                      << std::right << displayName (e.path) << "\n";
        }

        if (misses.size() > 40)
            std::cout << "  ... und " << (misses.size() - 40) << " weitere (alle in --csv)\n";
    }

    for (const auto& e : entries)
        if (! e.error.empty())
            std::cout << "Nicht lesbar: " << displayName (e.path) << " (" << e.error << ")\n";

    if (! options.csvPath.empty())
    {
        std::ofstream csv (argPath (options.csvPath));
        // Das Chromagramm je Datei gehoert mit hinein: Erst gemittelt ueber
        // viele Dateien, gedreht auf die richtige Tonart, zeigt es, wo die
        // Erkennung systematisch danebenliegt.
        csv << "datei,erwartet,erkannt,verhaeltnis,korrelation,stimmton_cent,bpm_erwartet,bpm_erkannt,bpm_aus_looplaenge";
        for (int pc = 0; pc < 12; ++pc)
            csv << ",chroma_" << keyy::noteName (pc);
        csv << '\n';

        for (const auto& e : entries)
        {
            if (! e.analysed)
                continue;

            const auto estimate = keyy::estimateKey (e.result.chroma, primary);
            csv << csvQuote (fs::relative (e.path, root).u8string()) << ','
                << (e.label ? e.label->shortName() : "") << ','
                << (e.result.valid ? estimate.key.shortName() : "") << ','
                << (e.label && e.result.valid ? keyy::relationName (keyy::relationOf (estimate.key, *e.label)) : "") << ','
                << fixed (estimate.score, 4) << ','
                << fixed (e.result.tuningCents, 1) << ','
                << (e.bpmLabel ? fixed (*e.bpmLabel, 1) : "") << ','
                << (e.result.tempoValid ? fixed (e.result.bpm, 2) : "") << ','
                << (e.result.bpmFromLoopLength ? 1 : 0);

            for (const auto value : e.result.chroma)
                csv << ',' << fixed (value, 5);

            csv << '\n';
        }

        std::cout << "\nCSV: " << options.csvPath << "\n";
    }

    return 0;
}

} // namespace

int main (int argc, char** argv)
{
    Options options;

    try
    {
        if (! parse (argc, argv, options))
        {
            printUsage();
            return 1;
        }

        return options.batch ? runBatch (options) : runSingle (options);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fehler: " << e.what() << "\n";
        return 1;
    }
}
