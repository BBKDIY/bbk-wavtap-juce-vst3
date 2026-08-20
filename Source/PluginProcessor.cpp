#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <type_traits>

BBKWavTapAudioProcessor::BBKWavTapAudioProcessor()
: AudioProcessor (BusesProperties()
    .withInput ("Input", juce::AudioChannelSet::stereo(), true)
    .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    backgroundThread.startThread (juce::Thread::Priority::high);
}

BBKWavTapAudioProcessor::~BBKWavTapAudioProcessor()
{
    stopRecording();
    backgroundThread.stopThread (2000);
}

void BBKWavTapAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate.store (sampleRate);
    preparedNumChannels = juce::jlimit (1, 2, juce::jmax (getTotalNumInputChannels(),
                                                            getTotalNumOutputChannels()));
}

void BBKWavTapAudioProcessor::releaseResources()
{
    stopRecording();
}

bool BBKWavTapAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    if (input != output)
        return false;

    return output == juce::AudioChannelSet::mono()
        || output == juce::AudioChannelSet::stereo();
}

template <typename SampleType>
void BBKWavTapAudioProcessor::process (juce::AudioBuffer<SampleType>& buffer) noexcept
{
    // Pure passthrough: the buffer is never read for modification and never
    // written to - input equals output, always. WavTap only observes.
    if constexpr (std::is_same_v<SampleType, float>)
    {
        if (auto* writer = activeWriter.load (std::memory_order_acquire))
        {
            const int expectedChannels = writerNumChannels.load (std::memory_order_relaxed);

            // Defensive: only write if the buffer actually has at least as
            // many channel pointers as the writer was constructed for. A
            // host bus renegotiation mid-recording could otherwise hand us
            // fewer channels than the open file expects.
            if (buffer.getNumChannels() >= expectedChannels && expectedChannels > 0)
            {
                writer->write (buffer.getArrayOfReadPointers(), buffer.getNumSamples());
                samplesWritten.fetch_add (buffer.getNumSamples(), std::memory_order_relaxed);
            }
        }
    }
    // Double-precision buffers never reach here - supportsDoublePrecisionProcessing()
    // is false, so hosts must use the float path.
}

void BBKWavTapAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    process (buffer);
}

void BBKWavTapAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    process (buffer);
}

void BBKWavTapAudioProcessor::startRecording()
{
    // Message-thread only: opens a file and constructs the writer.
    stopRecording();

    const int numChannels = preparedNumChannels;
    const double sr = currentSampleRate.load();
    if (sr <= 0.0 || numChannels <= 0)
        return;

    auto folder = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                      .getChildFile ("BBK WavTap Captures");
    folder.createDirectory();

    const auto stamp = juce::Time::getCurrentTime().formatted ("%Y-%m-%d_%H-%M-%S");
    lastFile = folder.getNonexistentChildFile ("WavTap_" + stamp, ".wav", false);

    if (auto stream = std::unique_ptr<juce::FileOutputStream> (lastFile.createOutputStream()))
    {
        juce::WavAudioFormat wavFormat;
        if (auto* writer = wavFormat.createWriterFor (stream.get(), sr,
                                                        static_cast<unsigned int> (numChannels),
                                                        32, {}, 0))
        {
            stream.release(); // writer now owns the stream

            const juce::ScopedLock sl (writerLock);
            samplesWritten.store (0);
            threadedWriter.reset (new juce::AudioFormatWriter::ThreadedWriter (writer, backgroundThread, 32768));
            writerNumChannels.store (numChannels, std::memory_order_relaxed);
            activeWriter.store (threadedWriter.get(), std::memory_order_release);
        }
    }
}

void BBKWavTapAudioProcessor::stopRecording()
{
    activeWriter.store (nullptr, std::memory_order_release);

    const juce::ScopedLock sl (writerLock);
    threadedWriter.reset(); // flushes remaining buffered audio and closes the file
    writerNumChannels.store (0, std::memory_order_relaxed);
}

double BBKWavTapAudioProcessor::getSamplesWrittenSeconds() const noexcept
{
    const double sr = currentSampleRate.load();
    if (sr <= 0.0)
        return 0.0;
    return static_cast<double> (samplesWritten.load()) / sr;
}

juce::AudioProcessorEditor* BBKWavTapAudioProcessor::createEditor()
{
    return new BBKWavTapAudioProcessorEditor (*this);
}

void BBKWavTapAudioProcessor::getStateInformation (juce::MemoryBlock&) {}
void BBKWavTapAudioProcessor::setStateInformation (const void*, int) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BBKWavTapAudioProcessor();
}
