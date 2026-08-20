#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <memory>

// BBK WavTap: a bit-perfect passthrough audio "tap". It never modifies the
// signal in any way - the output buffer is always identical to the input
// buffer. Its only job is to optionally mirror what passes through it to a
// 32-bit float WAV file on disk, so it can be inserted anywhere in a host's
// signal chain (e.g. Blue Cat PatchWork inside Audirvana) to capture an
// exact, bit-perfect copy of whatever that chain is playing, for later
// offline use in a different host (e.g. loading into REAPER for A/B
// listening with other plugins inserted).
//
// Deliberately as simple/stateless as possible in the audio path: no
// per-sample smoothing, no delay line, no persistent DSP state - the same
// minimal architecture pattern used by BBK ILD Matrix and BBK E280F Triode,
// both confirmed glitch-free when hosted inside PatchWork/Audirvana.
class BBKWavTapAudioProcessor final : public juce::AudioProcessor
{
public:
    BBKWavTapAudioProcessor();
    ~BBKWavTapAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override { return false; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BBK WavTap"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- WavTap control surface. These perform file I/O and must only ever
    // be called from the message thread (i.e. from the editor's UI), never
    // from processBlock. ---
    void startRecording();
    void stopRecording();
    bool isRecording() const noexcept { return activeWriter.load() != nullptr; }
    juce::File getLastWrittenFile() const noexcept { return lastFile; }
    double getSamplesWrittenSeconds() const noexcept;
    double getCurrentSampleRateForUI() const noexcept { return currentSampleRate.load(); }

private:
    template <typename SampleType>
    void process (juce::AudioBuffer<SampleType>& buffer) noexcept;

    juce::TimeSliceThread backgroundThread { "BBK WavTap Writer Thread" };
    juce::CriticalSection writerLock;
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;

    // Read from the audio thread without locking. Only ever written from the
    // message thread while holding writerLock.
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };
    std::atomic<int> writerNumChannels { 0 };
    std::atomic<juce::int64> samplesWritten { 0 };

    std::atomic<double> currentSampleRate { 0.0 };
    int preparedNumChannels = 2;
    juce::File lastFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BBKWavTapAudioProcessor)
};
