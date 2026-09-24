#include "PluginEditor.h"

#include <cmath>

namespace keyy
{

namespace
{
    /** juce::String nimmt einen const char* als ASCII an und beanstandet
        alles darüber. Die Oberfläche ist englisch, benutzt aber Zeichen
        jenseits von ASCII (⇅, —, …) — also ausdrücklich als UTF-8 lesen.
    */
    juce::String ui (const char* text)
    {
        return juce::String::fromUTF8 (text);
    }

    juce::String formatBpm (double bpm)
    {
        // Ganzzahlige Tempi ohne Nachkommastelle: "140 BPM", aber "139.7 BPM".
        const bool whole = std::abs (bpm - std::round (bpm)) < 0.05;
        return (whole ? juce::String (static_cast<int> (std::round (bpm))) : juce::String (bpm, 1)) + " BPM";
    }
}

KeyyAudioProcessorEditor::KeyyAudioProcessorEditor (KeyyAudioProcessor& processorToUse)
    : AudioProcessorEditor (processorToUse), owner (processorToUse)
{
    loadButton.setButtonText (ui ("Load File…"));
    loadButton.onClick = [this] { openFileChooser(); };

    swapButton.setButtonText (ui ("Relative Key ⇅"));
    swapButton.onClick = [this]
    {
        owner.setShowRelative (! owner.getShowRelative());
        refresh();
    };

    halfButton.setButtonText ("/2");
    halfButton.onClick = [this]
    {
        owner.setTempoOctave (owner.getTempoOctave() - 1);
        refresh();
    };

    doubleButton.setButtonText ("x2");
    doubleButton.onClick = [this]
    {
        owner.setTempoOctave (owner.getTempoOctave() + 1);
        refresh();
    };

    keyLabel.setFont (juce::Font (juce::FontOptions (40.0f, juce::Font::bold)));
    keyLabel.setJustificationType (juce::Justification::centred);

    for (auto* label : { &relativeLabel, &tuningLabel, &tempoLabel })
    {
        label->setFont (juce::Font (juce::FontOptions (17.0f)));
        label->setJustificationType (juce::Justification::centred);
    }

    statusLabel.setJustificationType (juce::Justification::centred);
    hintLabel.setJustificationType (juce::Justification::centred);
    hintLabel.setText (ui ("Drop an audio file here — WAV, AIFF, FLAC, MP3, Ogg"), juce::dontSendNotification);
    hintLabel.setColour (juce::Label::textColourId, juce::Colours::grey);

    for (auto* component : std::initializer_list<juce::Component*> {
             &loadButton, &fileLabel, &statusLabel, &progressBar, &keyLabel, &relativeLabel,
             &swapButton, &tuningLabel, &tempoLabel, &halfButton, &doubleButton, &hintLabel })
        addAndMakeVisible (component);

    setSize (460, 320);
    refresh();
    startTimerHz (15);
}

KeyyAudioProcessorEditor::~KeyyAudioProcessorEditor()
{
    stopTimer();
}

void KeyyAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    if (dragHover)
    {
        g.setColour (juce::Colours::orange);
        g.drawRect (getLocalBounds().reduced (2), 3);
    }
}

void KeyyAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (14);

    auto top = area.removeFromTop (30);
    loadButton.setBounds (top.removeFromLeft (140));
    top.removeFromLeft (10);
    fileLabel.setBounds (top);

    area.removeFromTop (10);
    const auto statusRow = area.removeFromTop (22);
    statusLabel.setBounds (statusRow);
    progressBar.setBounds (statusRow);

    hintLabel.setBounds (area.removeFromBottom (22));

    area.removeFromTop (6);
    keyLabel.setBounds (area.removeFromTop (60));

    auto relativeRow = area.removeFromTop (30);
    swapButton.setBounds (relativeRow.removeFromRight (140).reduced (0, 2));
    relativeLabel.setBounds (relativeRow);

    area.removeFromTop (6);
    tuningLabel.setBounds (area.removeFromTop (28));

    area.removeFromTop (6);
    auto tempoRow = area.removeFromTop (30);
    doubleButton.setBounds (tempoRow.removeFromRight (44).reduced (0, 2));
    tempoRow.removeFromRight (6);
    halfButton.setBounds (tempoRow.removeFromRight (44).reduced (0, 2));
    tempoLabel.setBounds (tempoRow);
}

void KeyyAudioProcessorEditor::timerCallback()
{
    refresh();
}

void KeyyAudioProcessorEditor::refresh()
{
    const auto status = owner.getStatus();
    const bool analysing = status == KeyyAudioProcessor::Status::Analysing;

    progressValue = owner.getProgress();
    progressBar.setVisible (analysing);
    statusLabel.setVisible (! analysing);
    // Die Meldung trägt beides: einen Fehler, oder den Hinweis, dass nur ein
    // Teil der Datei lesbar war.
    statusLabel.setText (analysing ? juce::String() : owner.getStatusMessage(), juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId, juce::Colours::orange);

    const auto r = owner.getResult();

    fileLabel.setText (r.valid ? r.fileName : ui ("No file loaded"), juce::dontSendNotification);

    swapButton.setEnabled (r.valid);
    halfButton.setEnabled (r.valid && r.tempoValid);
    doubleButton.setEnabled (r.valid && r.tempoValid);

    if (! r.valid)
    {
        keyLabel.setText (ui ("–"), juce::dontSendNotification);
        relativeLabel.setText ({}, juce::dontSendNotification);
        tuningLabel.setText ({}, juce::dontSendNotification);
        tempoLabel.setText ({}, juce::dontSendNotification);
        return;
    }

    const bool swapped = owner.getShowRelative();
    const Key shown = swapped ? r.key.relative() : r.key;
    const Key other = swapped ? r.key : r.key.relative();

    keyLabel.setText (shown.name(), juce::dontSendNotification);

    // Beide Tonarten bestehen aus denselben sieben Tönen. Für Auto-Tune ist
    // die Wahl deshalb gleichgültig — und genau dafür wird Keyy benutzt.
    relativeLabel.setText (ui ("= ") + other.name() + ui ("  (same scale)"), juce::dontSendNotification);

    tuningLabel.setText ("A = " + juce::String (r.referenceHz, 1) + " Hz   ("
                             + (r.tuningCents >= 0.0 ? "+" : "") + juce::String (r.tuningCents, 1) + " cents)",
                         juce::dontSendNotification);

    if (r.tempoValid)
    {
        const int octave = owner.getTempoOctave();
        const double bpm = r.bpm * std::pow (2.0, octave);

        juce::String text = formatBpm (bpm);
        if (octave == 0 && r.bpmFromLoopLength)
            text += ui ("   (from loop length)");
        else if (octave != 0)
            text += ui ("   (detected: ") + formatBpm (r.bpm) + ")";

        tempoLabel.setText (text, juce::dontSendNotification);
    }
    else
    {
        tempoLabel.setText (ui ("No tempo detected"), juce::dontSendNotification);
    }
}

void KeyyAudioProcessorEditor::openFileChooser()
{
    chooser = std::make_unique<juce::FileChooser> (ui ("Choose Audio File"), juce::File(), owner.getFormatWildcard());

    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();
                              if (file.existsAsFile())
                              {
                                  owner.analyseFile (file);
                                  refresh();
                              }
                          });
}

bool KeyyAudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    return files.size() == 1 && owner.canRead (juce::File (files[0]));
}

void KeyyAudioProcessorEditor::fileDragEnter (const juce::StringArray&, int, int)
{
    dragHover = true;
    repaint();
}

void KeyyAudioProcessorEditor::fileDragExit (const juce::StringArray&)
{
    dragHover = false;
    repaint();
}

void KeyyAudioProcessorEditor::filesDropped (const juce::StringArray& files, int, int)
{
    dragHover = false;
    repaint();

    if (files.size() == 1)
    {
        owner.analyseFile (juce::File (files[0]));
        refresh();
    }
}

} // namespace keyy
