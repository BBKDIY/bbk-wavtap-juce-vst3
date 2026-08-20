#include "PluginEditor.h"

namespace
{
void prepareLabel (juce::Label& label)
{
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colours::white);
}
}

BBKWavTapAudioProcessorEditor::BBKWavTapAudioProcessorEditor (BBKWavTapAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    prepareLabel (title);
    title.setText ("BBK WavTap", juce::dontSendNotification);
    title.setFont (juce::Font (24.0f, juce::Font::bold));
    addAndMakeVisible (title);

    prepareLabel (sampleRate);
    addAndMakeVisible (sampleRate);

    prepareLabel (status);
    addAndMakeVisible (status);

    prepareLabel (fileLabel);
    fileLabel.setFont (juce::Font (13.0f));
    addAndMakeVisible (fileLabel);

    prepareLabel (spec);
    spec.setFont (juce::Font (12.0f));
    spec.setText ("Bit-perfect passthrough tap - never alters the signal.\n"
                  "Writes 32-bit float WAV to Documents\\BBK WavTap Captures.",
                  juce::dontSendNotification);
    addAndMakeVisible (spec);

    recordButton.setClickingTogglesState (false);
    recordButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff8a1f1f));
    recordButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff1f8a2f));
    recordButton.onClick = [this] { toggleRecording(); };
    addAndMakeVisible (recordButton);

    setSize (560, 260);
    startTimerHz (4);
    timerCallback();
}

BBKWavTapAudioProcessorEditor::~BBKWavTapAudioProcessorEditor()
{
    // Intentionally does NOT stop recording on editor close - closing the
    // plugin window should not truncate a capture in progress.
}

void BBKWavTapAudioProcessorEditor::toggleRecording()
{
    if (processor.isRecording())
        processor.stopRecording();
    else
        processor.startRecording();

    timerCallback();
}

void BBKWavTapAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff171717));
    g.setColour (juce::Colour (0xff505050));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (8.0f), 8.0f, 1.0f);
}

void BBKWavTapAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (18);
    title.setBounds (area.removeFromTop (36));
    sampleRate.setBounds (area.removeFromTop (26));
    status.setBounds (area.removeFromTop (26));
    area.removeFromTop (8);
    recordButton.setBounds (area.removeFromTop (36).withSizeKeepingCentre (220, 34));
    area.removeFromTop (8);
    fileLabel.setBounds (area.removeFromTop (22));
    area.removeFromTop (8);
    spec.setBounds (area.removeFromTop (40));
}

void BBKWavTapAudioProcessorEditor::timerCallback()
{
    const double sr = processor.getCurrentSampleRateForUI();
    sampleRate.setText ("Host sample rate: " + juce::String (sr, 0) + " Hz",
                        juce::dontSendNotification);

    const bool recording = processor.isRecording();
    recordButton.setButtonText (recording ? "STOP RECORDING" : "START RECORDING");
    recordButton.setToggleState (recording, juce::dontSendNotification);

    if (recording)
    {
        status.setText ("RECORDING - " + juce::String (processor.getSamplesWrittenSeconds(), 1) + " s",
                        juce::dontSendNotification);
        fileLabel.setText (processor.getLastWrittenFile().getFileName(), juce::dontSendNotification);
    }
    else
    {
        status.setText ("Idle", juce::dontSendNotification);
        fileLabel.setText (processor.getLastWrittenFile() == juce::File()
                                ? juce::String()
                                : "Last: " + processor.getLastWrittenFile().getFileName(),
                          juce::dontSendNotification);
    }
}
