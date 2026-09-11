#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

namespace keyy
{

/** Minimale Oberfläche: Datei laden (Knopf oder Ziehen), Ergebnis anzeigen.

    Bewusst ohne Gestaltung — erst die Funktion, das Aussehen später. Anders
    als in Voxx reicht der generische Editor hier nicht: Er kann nur
    Parameter darstellen, keine Datei entgegennehmen.

    Die Oberfläche hält keinen eigenen Zustand. Ein Timer liest ihn aus dem
    Prozessor — der Analyse-Thread greift dadurch nie auf die Oberfläche
    zu, und ein wieder geöffnetes Fenster zeigt sofort den aktuellen Stand.
*/
class KeyyAudioProcessorEditor : public juce::AudioProcessorEditor,
                                 public juce::FileDragAndDropTarget,
                                 private juce::Timer
{
public:
    explicit KeyyAudioProcessorEditor (KeyyAudioProcessor&);
    ~KeyyAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override;
    void fileDragExit (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    void timerCallback() override;
    void openFileChooser();
    void refresh();

    KeyyAudioProcessor& owner;

    juce::TextButton loadButton;
    juce::Label fileLabel, statusLabel, keyLabel, relativeLabel, tuningLabel, tempoLabel, hintLabel;
    juce::TextButton swapButton, halfButton, doubleButton;

    double progressValue = 0.0;
    juce::ProgressBar progressBar { progressValue };

    std::unique_ptr<juce::FileChooser> chooser;
    bool dragHover = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeyyAudioProcessorEditor)
};

} // namespace keyy
