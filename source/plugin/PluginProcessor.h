#pragma once

#include <atomic>
#include <memory>

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/KeyAnalyzer.h"

namespace keyy
{

/** Was das Plugin anzeigt und im FL-Projekt speichert. */
struct DisplayResult
{
    bool valid = false;
    juce::String fileName;
    Key key;
    double referenceHz = 440.0;
    double tuningCents = 0.0;
    bool tempoValid = false;
    double bpm = 0.0;
    bool bpmFromLoopLength = false;
};

/** Keyy: bestimmt Tonart, Stimmton und Tempo einer Audiodatei.

    Das Audiosignal läuft unverändert durch. Keyy sitzt als Effekt auf einer
    Mixer-Spur, damit ein späterer Listen-Modus das laufende Signal
    mithören kann, ohne dass sich am Plugin-Typ etwas ändert.

    ANALYSE IM HINTERGRUND
    Eine Datei wird in einem eigenen Thread gelesen und analysiert, nie im
    Audio-Thread und nie im Message-Thread — ein Song dauert Sekunden-
    bruchteile, eine lange Datei auf einer Netzwerkfreigabe aber nicht.
    Die Oberfläche fragt Fortschritt und Ergebnis per Timer ab.

    GESPEICHERT
    Ergebnis und Dateiname, nicht der Pfad zum Neu-Analysieren: Beim Öffnen
    des Projekts steht das Ergebnis sofort da, auch wenn die Datei
    inzwischen verschoben oder gelöscht wurde.
*/
class KeyyAudioProcessor : public juce::AudioProcessor
{
public:
    enum class Status { Idle, Analysing, Done, Failed };

    KeyyAudioProcessor();
    ~KeyyAudioProcessor() override;

    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // --- Datei-Analyse ----------------------------------------------------

    /** Startet die Analyse; eine laufende wird abgebrochen. */
    void analyseFile (const juce::File& file);

    bool canRead (const juce::File& file) const;
    juce::String getFormatWildcard() const;

    Status getStatus() const noexcept { return status.load(); }
    float getProgress() const noexcept { return progress.load(); }
    juce::String getStatusMessage() const;
    DisplayResult getResult() const;

    // --- Anzeige ------------------------------------------------------------

    /** Paralleltonart statt erkannter Tonart zeigen (Tauschen-Knopf). */
    bool getShowRelative() const noexcept { return showRelative.load(); }
    void setShowRelative (bool shouldShow) noexcept { showRelative = shouldShow; }

    /** Tempo-Korrektur in Oktaven: -1 = /2, +1 = x2. */
    int getTempoOctave() const noexcept { return tempoOctave.load(); }
    void setTempoOctave (int octave) noexcept { tempoOctave = juce::jlimit (-2, 2, octave); }

private:
    class AnalysisThread;
    friend class AnalysisThread;

    void runAnalysis (const juce::File& file, juce::Thread& thread);
    void fail (const juce::String& reason);
    void stopAnalysis();

    juce::AudioFormatManager formatManager;
    std::unique_ptr<AnalysisThread> analysisThread;

    mutable juce::CriticalSection lock;
    DisplayResult result;           // unter lock
    juce::String statusMessage;     // unter lock

    std::atomic<Status> status { Status::Idle };
    std::atomic<float> progress { 0.0f };
    std::atomic<bool> showRelative { false };
    std::atomic<int> tempoOctave { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyyAudioProcessor)
};

} // namespace keyy
