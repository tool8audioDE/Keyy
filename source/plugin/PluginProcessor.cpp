#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace keyy
{

class KeyyAudioProcessor::AnalysisThread : public juce::Thread
{
public:
    AnalysisThread (KeyyAudioProcessor& ownerToUse, juce::File fileToAnalyse)
        : juce::Thread ("Keyy Analysis"), owner (ownerToUse), file (std::move (fileToAnalyse))
    {
    }

    void run() override { owner.runAnalysis (file, *this); }

private:
    KeyyAudioProcessor& owner;
    juce::File file;
};

KeyyAudioProcessor::KeyyAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // WAV, AIFF, FLAC, Ogg und — per JUCE_USE_MP3AUDIOFORMAT — MP3.
    formatManager.registerBasicFormats();
}

KeyyAudioProcessor::~KeyyAudioProcessor()
{
    stopAnalysis();
}

bool KeyyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();

    return in == out && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
}

void KeyyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // Das Signal läuft unverändert durch. Nur Kanäle, die der Host
    // bereitstellt, aber nicht gefüllt hat, werden gelöscht.
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* KeyyAudioProcessor::createEditor()
{
    return new KeyyAudioProcessorEditor (*this);
}

// --- Datei-Analyse --------------------------------------------------------

bool KeyyAudioProcessor::canRead (const juce::File& file) const
{
    return file.existsAsFile()
        && const_cast<juce::AudioFormatManager&> (formatManager).findFormatForFileExtension (file.getFileExtension()) != nullptr;
}

juce::String KeyyAudioProcessor::getFormatWildcard() const
{
    return formatManager.getWildcardForAllFormats();
}

void KeyyAudioProcessor::analyseFile (const juce::File& file)
{
    stopAnalysis();

    {
        const juce::ScopedLock sl (lock);
        statusMessage = juce::String::fromUTF8 ("Analysing ") + file.getFileName() + juce::String::fromUTF8 ("…");
    }

    progress = 0.0f;
    status = Status::Analysing;

    analysisThread = std::make_unique<AnalysisThread> (*this, file);
    analysisThread->startThread (juce::Thread::Priority::low);
}

void KeyyAudioProcessor::stopAnalysis()
{
    if (analysisThread != nullptr)
    {
        analysisThread->stopThread (5000);
        analysisThread.reset();
    }
}

void KeyyAudioProcessor::fail (const juce::String& reason)
{
    {
        const juce::ScopedLock sl (lock);
        statusMessage = reason;
    }

    status = Status::Failed;
}

void KeyyAudioProcessor::runAnalysis (const juce::File& file, juce::Thread& thread)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));

    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->sampleRate <= 0.0)
    {
        fail (juce::String::fromUTF8 ("Cannot read file: ") + file.getFileName());
        return;
    }

    KeyAnalyzer analyzer;
    analyzer.prepare (reader->sampleRate);

    constexpr int readBlockSize = 1 << 15;
    const int numChannels = juce::jmax (1, static_cast<int> (reader->numChannels));
    const auto length = reader->lengthInSamples;

    juce::AudioBuffer<float> buffer (numChannels, readBlockSize);
    std::vector<float> mono (static_cast<size_t> (readBlockSize));
    juce::int64 analysedSamples = 0;

    for (juce::int64 pos = 0; pos < length; pos += readBlockSize)
    {
        if (thread.threadShouldExit())
            return;

        const int n = static_cast<int> (juce::jmin<juce::int64> (readBlockSize, length - pos));

        // Ein fehlgeschlagener Block beendet das Lesen, verwirft aber nicht
        // die Analyse: JUCEs MP3-Dekoder steigt bei manchen Dateien vorzeitig
        // aus, wo andere Dekoder durchlaufen. Lieber die Tonart aus den
        // ersten zwei Minuten als gar keine.
        if (! reader->read (buffer.getArrayOfWritePointers(), numChannels, pos, n))
            break;

        std::fill (mono.begin(), mono.begin() + n, 0.0f);
        for (int ch = 0; ch < numChannels; ++ch)
            juce::FloatVectorOperations::add (mono.data(), buffer.getReadPointer (ch), n);

        juce::FloatVectorOperations::multiply (mono.data(), 1.0f / static_cast<float> (numChannels), n);

        analyzer.process (mono.data(), n);
        analysedSamples += n;
        progress = static_cast<float> (static_cast<double> (pos + n) / static_cast<double> (length));
    }

    if (analysedSamples == 0)
    {
        fail (juce::String::fromUTF8 ("Cannot decode file: ") + file.getFileName());
        return;
    }

    const auto analysis = analyzer.finish();

    if (! analysis.valid)
    {
        fail (juce::String::fromUTF8 ("No tonal content found in ") + file.getFileName());
        return;
    }

    DisplayResult display;
    display.valid             = true;
    display.fileName          = file.getFileName();
    display.key               = analysis.key;
    display.referenceHz       = analysis.referenceHz;
    display.tuningCents       = analysis.tuningCents;
    display.tempoValid        = analysis.tempoValid;
    display.bpm               = analysis.bpm;
    display.bpmFromLoopLength = analysis.bpmFromLoopLength;

    {
        const juce::ScopedLock sl (lock);
        result = display;

        // Offenlegen, wenn der Dekoder vorzeitig ausgestiegen ist: Eine
        // Tonart aus den ersten Sekunden einer langen Datei ist etwas
        // anderes als eine aus der ganzen.
        //
        // Erst ab einem nennenswerten Rest: JUCEs MP3-Dekoder lässt bei
        // LAME-Dateien aus FL Studio regelmäßig den letzten Sekunden-
        // bruchteil aus. Das ändert an der Tonart nichts, und eine Warnung
        // bei jeder Datei wäre nur Rauschen.
        const juce::int64 missing = length - analysedSamples;
        statusMessage = missing > juce::jmax<juce::int64> (static_cast<juce::int64> (reader->sampleRate), length / 50)
            ? juce::String::fromUTF8 ("Only ") + juce::String (analysedSamples / reader->sampleRate, 1)
                  + juce::String::fromUTF8 (" s of ") + juce::String (length / reader->sampleRate, 1)
                  + juce::String::fromUTF8 (" s readable — result from that part")
            : juce::String();
    }

    // Eine neue Datei beginnt mit der erkannten Tonart und dem erkannten Tempo.
    showRelative = false;
    tempoOctave = 0;
    status = Status::Done;
}

juce::String KeyyAudioProcessor::getStatusMessage() const
{
    const juce::ScopedLock sl (lock);
    return statusMessage;
}

DisplayResult KeyyAudioProcessor::getResult() const
{
    const juce::ScopedLock sl (lock);
    return result;
}

// --- Zustand ---------------------------------------------------------------

void KeyyAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const auto r = getResult();

    juce::ValueTree state ("KEYY");
    state.setProperty ("version", 1, nullptr);
    state.setProperty ("valid", r.valid, nullptr);

    if (r.valid)
    {
        state.setProperty ("file",        r.fileName, nullptr);
        state.setProperty ("tonic",       r.key.tonic, nullptr);
        state.setProperty ("minor",       r.key.mode == Mode::Minor, nullptr);
        state.setProperty ("referenceHz", r.referenceHz, nullptr);
        state.setProperty ("tuningCents", r.tuningCents, nullptr);
        state.setProperty ("tempoValid",  r.tempoValid, nullptr);
        state.setProperty ("bpm",         r.bpm, nullptr);
        state.setProperty ("bpmFromLoop", r.bpmFromLoopLength, nullptr);
    }

    state.setProperty ("showRelative", showRelative.load(), nullptr);
    state.setProperty ("tempoOctave",  tempoOctave.load(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void KeyyAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName ("KEYY"))
        return;

    stopAnalysis();

    const auto state = juce::ValueTree::fromXml (*xml);

    DisplayResult r;
    r.valid = state.getProperty ("valid", false);

    if (r.valid)
    {
        r.fileName          = state.getProperty ("file").toString();
        r.key.tonic         = juce::jlimit (0, 11, static_cast<int> (state.getProperty ("tonic", 0)));
        r.key.mode          = static_cast<bool> (state.getProperty ("minor", false)) ? Mode::Minor : Mode::Major;
        r.referenceHz       = state.getProperty ("referenceHz", 440.0);
        r.tuningCents       = state.getProperty ("tuningCents", 0.0);
        r.tempoValid        = state.getProperty ("tempoValid", false);
        r.bpm               = state.getProperty ("bpm", 0.0);
        r.bpmFromLoopLength = state.getProperty ("bpmFromLoop", false);
    }

    {
        const juce::ScopedLock sl (lock);
        result = r;
        statusMessage = {};
    }

    showRelative = static_cast<bool> (state.getProperty ("showRelative", false));
    setTempoOctave (state.getProperty ("tempoOctave", 0));
    status = r.valid ? Status::Done : Status::Idle;
}

} // namespace keyy

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new keyy::KeyyAudioProcessor();
}
