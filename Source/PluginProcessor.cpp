
#include "PluginProcessor.h"
#include "PluginEditor.h"

// ============================================================
// UBERKNALL PREAMP
// Prototype 0.2 - Saturation Test
//
// STANDARD = Default
// HIGH GAIN = Increased nonlinear saturation
//
// No tone stack.
// No post-drive EQ.
// Output level independent of saturation.
// ============================================================

UberknallPreampAudioProcessor::UberknallPreampAudioProcessor()
    : AudioProcessor(
        BusesProperties()
            .withInput(
                "Input",
                juce::AudioChannelSet::stereo(),
                true)
            .withOutput(
                "Output",
                juce::AudioChannelSet::stereo(),
                true)),
      parameters(
          *this,
          nullptr,
          "PARAMETERS",
          createParameterLayout())
{
}

// ============================================================
// PARAMETERS
// ============================================================

juce::AudioProcessorValueTreeState::ParameterLayout
UberknallPreampAudioProcessor::createParameterLayout()
{
    using namespace juce;

    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back(
        std::make_unique<AudioParameterFloat>(
            "drive",
            "Drive",
            NormalisableRange<float>(0.0f, 10.0f, 0.01f),
            5.0f));

    p.push_back(
        std::make_unique<AudioParameterFloat>(
            "output",
            "Output",
            NormalisableRange<float>(-24.0f, 12.0f, 0.01f),
            -3.0f));

    // OFF = STANDARD DEFAULT
    p.push_back(
        std::make_unique<AudioParameterBool>(
            "highGain",
            "High Gain",
            false));

    return { p.begin(), p.end() };
}

// ============================================================
// PREPARATION
// ============================================================

void UberknallPreampAudioProcessor::prepareToPlay(
    double sampleRate,
    int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);

    currentSampleRate = sampleRate;

    for (auto& s : state)
        s = {};
}

void UberknallPreampAudioProcessor::releaseResources()
{
}

// ============================================================
// BUS LAYOUT
// ============================================================

bool UberknallPreampAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    return input == output &&
        (input == juce::AudioChannelSet::mono() ||
         input == juce::AudioChannelSet::stereo());
}

// ============================================================
// NONLINEAR PREAMP STAGE
// ============================================================

float UberknallPreampAudioProcessor::processSample(
    float x,
    ChannelState& s,
    float drive,
    bool highGain,
    float sr)
{
    juce::ignoreUnused(sr);

    // Input DC rejection
    const float input =
        x - s.dcInX + 0.995f * s.dcInY;

    s.dcInX = x;
    s.dcInY = input;

    // Drive range: 0 to 10
    const float d =
        juce::jlimit(0.0f, 10.0f, drive) / 10.0f;

    // Fully clean at zero
    if (d < 0.001f)
        return input;

    // High Gain changes saturation intensity,
    // not output volume.
    const float maximumDrive =
        highGain ? 79.0f : 39.0f;

    // Progressive nonlinear response
    const float k =
        1.0f + maximumDrive * std::pow(d, 2.2f);

    // Normalized soft saturation.
    // Small-signal gain remains approximately constant.
    const float saturated =
        std::tanh(k * input) / k;

    return saturated;
}

// ============================================================
// AUDIO PROCESSING
// ============================================================

void UberknallPreampAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;

    const int channels =
        juce::jmin(2, buffer.getNumChannels());

    const float drive =
        parameters.getRawParameterValue("drive")->load();

    const float outputDb =
        parameters.getRawParameterValue("output")->load();

    const bool highGain =
        parameters.getRawParameterValue("highGain")->load()
        > 0.5f;

    const float outputGain =
        juce::Decibels::decibelsToGain(outputDb);

    const float sr =
        static_cast<float>(currentSampleRate);

    for (int ch = 0; ch < channels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);

        auto& channelState =
            state[static_cast<size_t>(ch)];

        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            const float processed =
                processSample(
                    data[n],
                    channelState,
                    drive,
                    highGain,
                    sr);

            data[n] = processed * outputGain;
        }
    }

    // Clear unexpected additional channels
    for (int ch = channels;
         ch < buffer.getNumChannels();
         ++ch)
    {
        buffer.clear(ch, 0, buffer.getNumSamples());
    }
}

// ============================================================
// EDITOR
// ============================================================

juce::AudioProcessorEditor*
UberknallPreampAudioProcessor::createEditor()
{
    return new UberknallPreampAudioProcessorEditor(*this);
}

// ============================================================
// SAVE / RESTORE SETTINGS
// ============================================================

void UberknallPreampAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    auto stateCopy = parameters.copyState();

    std::unique_ptr<juce::XmlElement> xml(
        stateCopy.createXml());

    copyXmlToBinary(*xml, destData);
}

void UberknallPreampAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(
        getXmlFromBinary(data, sizeInBytes));

    if (xml != nullptr &&
        xml->hasTagName(parameters.state.getType()))
    {
        parameters.replaceState(
            juce::ValueTree::fromXml(*xml));
    }
}

// ============================================================
// PLUGIN ENTRY POINT
// ============================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UberknallPreampAudioProcessor();
}
