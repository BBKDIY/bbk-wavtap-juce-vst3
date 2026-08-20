#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class BBKWavTapAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer
{
public:
    explicit BBKWavTapAudioProcessorEditor (BBKWavTapAudioProcessor&);
    ~BBKWavTapAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void toggleRecording();

    BBKWavTapAudioProcessor& processor;

    juce::Label title;
    juce::Label sampleRate;
    juce::Label status;
    juce::Label fileLabel;
    juce::Label spec;
    juce::TextButton recordButton { "START RECORDING" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BBKWavTapAudioProcessorEditor)
};
