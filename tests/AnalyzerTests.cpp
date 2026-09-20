#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <vector>

#include "TestHelpers.h"

using namespace keyy;

TEST_CASE ("Alle 24 Tonarten werden an einer Kadenz erkannt, mit jedem Profil")
{
    const double sampleRate = 22050.0;

    AnalysisSettings settings;
    settings.detectTempo = false;

    for (int index = 0; index < 24; ++index)
    {
        const Key key = Key::fromIndex (index);
        const auto result = test::analyse (test::cadence (key, sampleRate), sampleRate, settings);
        REQUIRE (result.valid);

        for (const auto profile : allProfiles)
        {
            const auto estimate = estimateKey (result.chroma, profile);
            INFO (getProfile (profile).name << ": " << key.name() << " erkannt als " << estimate.key.name());
            CHECK (estimate.key == key);
        }
    }
}

TEST_CASE ("Verstimmung um fast einen Viertelton wird ausgeglichen")
{
    // A = 450 Hz liegt 39 Cent ueber dem Raster. Ohne Stimmton-Korrektur
    // fielen die Toene fast mittig zwischen zwei Tonklassen.
    const double sampleRate = 22050.0;
    const Key fMinor { 5, Mode::Minor };

    AnalysisSettings settings;
    settings.detectTempo = false;
    const auto result = test::analyse (test::cadence (fMinor, sampleRate, 1.0, 450.0), sampleRate, settings);

    REQUIRE (result.valid);
    CHECK (result.key == fMinor);
    CHECK (std::abs (result.tuningCents - 1200.0 * std::log2 (450.0 / 440.0)) < 3.0);
}

TEST_CASE ("Kadenz ueber einem Drum-Loop wird trotzdem erkannt")
{
    const double sampleRate = 44100.0;
    const Key key { 3, Mode::Minor };    // Eb-Moll

    auto audio = test::cadence (key, sampleRate, 60.0 / 140.0 * 4.0, 440.0, 2);    // ein Akkord je Takt
    const auto drums = test::drumLoop (140.0, 8, sampleRate);

    audio.resize (std::max (audio.size(), drums.size()), 0.0f);
    for (size_t i = 0; i < drums.size(); ++i)
        audio[i] += drums[i];

    const auto result = test::analyse (audio, sampleRate);

    REQUIRE (result.valid);
    CHECK (result.key == key);
}

TEST_CASE ("Ergebnis haengt nicht von der Blockgroesse ab")
{
    const double sampleRate = 44100.0;
    auto audio = test::cadence (Key { 7, Mode::Major }, sampleRate, 1.0);
    const auto drums = test::drumLoop (120.0, 4, sampleRate);
    for (size_t i = 0; i < std::min (audio.size(), drums.size()); ++i)
        audio[i] += drums[i];

    const auto small = test::analyse (audio, sampleRate, {}, 64);
    const auto large = test::analyse (audio, sampleRate, {}, 10000);

    REQUIRE (small.valid);
    REQUIRE (large.valid);
    CHECK (small.key == large.key);
    CHECK (small.tuningCents == large.tuningCents);
    CHECK (small.bpm == large.bpm);

    for (size_t i = 0; i < 12; ++i)
        CHECK (std::abs (small.chroma[i] - large.chroma[i]) < 1.0e-12);
}

TEST_CASE ("Taktgewichtung aus lässt das Ergebnis unverändert")
{
    // Der Schalter ist gemessen wirkungslos (siehe README), bleibt aber als
    // Messwerkzeug. Wichtigste Zusicherung: Vorgabe 0 rechnet wie zuvor.
    const double sampleRate = 22050.0;
    const Key aMinor { 9, Mode::Minor };
    const auto audio = test::cadence (aMinor, sampleRate, 1.0, 440.0, 4);

    AnalysisSettings settings;
    settings.detectTempo = true;

    const auto ohne = test::analyse (audio, sampleRate, settings);
    REQUIRE (ohne.valid);
    CHECK_FALSE (ohne.barWeighted);
    CHECK (ohne.barSeconds == 0.0);

    // Zweimal dieselbe Einstellung muss dasselbe liefern — sonst wäre die
    // Aufzeichnung der Rahmen nicht folgenlos.
    const auto nochmal = test::analyse (audio, sampleRate, settings);
    for (size_t i = 0; i < ohne.chroma.size(); ++i)
        CHECK (nochmal.chroma[i] == ohne.chroma[i]);
}

TEST_CASE ("Taktgewichtung an: Raster nur mit Tempo, sonst unverändert")
{
    const double sampleRate = 22050.0;
    const Key aMinor { 9, Mode::Minor };
    const auto audio = test::cadence (aMinor, sampleRate, 1.0, 440.0, 4);

    AnalysisSettings aus;
    aus.detectTempo = true;
    AnalysisSettings an = aus;
    an.barWeighting = 2.0;

    const auto ohne = test::analyse (audio, sampleRate, aus);
    const auto mit  = test::analyse (audio, sampleRate, an);
    REQUIRE (ohne.valid);
    REQUIRE (mit.valid);

    if (mit.barWeighted)
    {
        CHECK (mit.barSeconds == Catch::Approx (240.0 / mit.bpm));
        CHECK (mit.barPhase >= 0.0);
        CHECK (mit.barPhase < mit.barSeconds);
    }
    else
    {
        // Ohne erkanntes Tempo bleibt das Chromagramm unangetastet.
        for (size_t i = 0; i < ohne.chroma.size(); ++i)
            CHECK (mit.chroma[i] == Catch::Approx (ohne.chroma[i]));
    }
}

TEST_CASE ("Gleiche Gewichte für alle Rahmen ergeben das ungewichtete Chromagramm")
{
    const double sampleRate = 11025.0;
    const Key cMajor { 0, Mode::Major };
    const auto audio = test::cadence (cMajor, sampleRate, 1.0, 440.0, 2);

    ChromaAccumulator::Settings settings;
    settings.recordFrames = true;

    ChromaAccumulator chroma;
    chroma.prepare (sampleRate, settings);
    chroma.process (audio.data(), static_cast<int> (audio.size()));
    chroma.flush();

    const int frames = chroma.getNumRecordedFrames();
    REQUIRE (frames > 0);

    const auto ungewichtet = chroma.getChroma();

    // Das Chromagramm ist auf Summe 1 normiert: Gleiche Gewichte — egal
    // welche — müssen deshalb dasselbe ergeben.
    for (const double gewicht : { 1.0, 2.0, 0.25 })
    {
        const auto gleich = chroma.getChroma (std::vector<double> (static_cast<size_t> (frames), gewicht));
        for (size_t i = 0; i < ungewichtet.size(); ++i)
            CHECK (gleich[i] == Catch::Approx (ungewichtet[i]).margin (1.0e-9));
    }

    // Eine unpassend lange Liste fällt auf das ungewichtete Ergebnis zurück.
    const auto unpassend = chroma.getChroma (std::vector<double> (3, 1.0));
    for (size_t i = 0; i < ungewichtet.size(); ++i)
        CHECK (unpassend[i] == Catch::Approx (ungewichtet[i]).margin (1.0e-9));

    // Nur die erste Hälfte zählen lassen muss etwas anderes ergeben —
    // sonst greift die Gewichtung gar nicht.
    std::vector<double> halb (static_cast<size_t> (frames), 0.0);
    for (int i = 0; i < frames / 2; ++i)
        halb[static_cast<size_t> (i)] = 1.0;

    const auto teil = chroma.getChroma (halb);
    double abstand = 0.0;
    for (size_t i = 0; i < ungewichtet.size(); ++i)
        abstand += std::abs (teil[i] - ungewichtet[i]);

    CHECK (abstand > 1.0e-6);
}
