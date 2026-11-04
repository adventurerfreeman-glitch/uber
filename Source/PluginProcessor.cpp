#include "PluginProcessor.h"
#include "PluginEditor.h"

UberknallPreampAudioProcessor::UberknallPreampAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
UberknallPreampAudioProcessor::createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    p.push_back (std::make_unique<AudioParameterFloat> ("drive", "Drive",
        NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.0f));
    p.push_back (std::make_unique<AudioParameterFloat> ("output", "Output",
        NormalisableRange<float> (-24.0f, 12.0f, 0.01f), -3.0f));
    // false is the default: Standard mode.
    p.push_back (std::make_unique<AudioParameterBool> ("highGain", "High Gain", false));
    return { p.begin(), p.end() };
}

void UberknallPreampAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    for (auto& s : state) s = {};
}

void UberknallPreampAudioProcessor::releaseResources() {}

bool UberknallPreampAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

float UberknallPreampAudioProcessor::onePoleLowpass (float x, float& z, float cutoff, float sr)
{
    const float c = std::exp (-2.0f * juce::MathConstants<float>::pi * cutoff / sr);
    z = (1.0f - c) * x + c * z;
    return z;
}

// Smooth asymmetric transfer curve: a compact circuit-inspired stage model,
// not a transistor-level simulation or a claim of exact component matching.
float UberknallPreampAudioProcessor::softStage (float x, float asymmetry, float headroom)
{
    const float driven = x / juce::jmax (0.05f, headroom);
    const float positive = std::tanh (driven * (1.0f + asymmetry));
    const float negative = std::tanh (driven * (1.0f - asymmetry));
    return 0.5f * (positive + negative) * headroom;
}

float UberknallPreampAudioProcessor::processSample (float x, ChannelState& s,
                                                     float drive, bool highGain, float sr)
{
    // Input coupling / DC rejection.
    const float dc = x - s.dcInX + 0.995f * s.dcInY;
    s.dcInX = x; s.dcInY = dc;

    const float d = drive / 10.0f;
    // Drive changes preamp excitation. The input signal remains consequential:
    // a hotter external boost crosses the nonlinear knees more often.
    const float preGain = juce::jmap (d, 1.15f, highGain ? 24.0f : 13.5f);
    float v = dc * preGain;

    // Interstage bandwidth shaping approximates coupling/feedback poles.
    // Deliberately no amp tone stack or post-drive corrective EQ.
    v = onePoleLowpass (v, s.low1, 10500.0f, sr);
    v = softStage (v, 0.12f, highGain ? 0.78f : 0.92f);

    // Stage 2: stronger drive in High Gain mode is distributed here,
    // not applied as a final output multiplier.
    v = onePoleLowpass (v, s.low2, 7600.0f, sr);
    v = softStage (v * (highGain ? 1.85f : 1.20f), 0.20f, 0.72f);

    // Stage 3: further nonlinear gain; the High Gain switch changes this
    // stage's operating level as well as stage 2.
    v = onePoleLowpass (v, s.low3, 6200.0f, sr);
    v = softStage (v * (highGain ? 1.55f : 1.12f), 0.10f, 0.68f);

    // Final recovery buffer: no added clipping, no tone stack.
    const float dcFree = v - s.dcOutX + 0.9992f * s.dcOutY;
    s.dcOutX = v; s.dcOutY = dcFree;
    return dcFree;
}

void UberknallPreampAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin (2, buffer.getNumChannels());
    const float drive = parameters.getRawParameterValue ("drive")->load();
    const float outDb = parameters.getRawParameterValue ("output")->load();
    const bool highGain = parameters.getRawParameterValue ("highGain")->load() > 0.5f;
    const float output = juce::Decibels::decibelsToGain (outDb);
    const float sr = static_cast<float> (currentSampleRate);

    for (int ch = 0; ch < channels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        auto& st = state[static_cast<size_t> (ch)];
        for (int n = 0; n < buffer.getNumSamples(); ++n)
        {
            const float y = processSample (data[n], st, drive, highGain, sr);
            data[n] = juce::jlimit (-1.2f, 1.2f, y * output);
        }
    }
    // Clear any unexpected extra output channels.
    for (int ch = channels; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* UberknallPreampAudioProcessor::createEditor()
{
    return new UberknallPreampAudioProcessorEditor (*this);
}

void UberknallPreampAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto stateCopy = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (stateCopy.createXml());
    copyXmlToBinary (*xml, dest);
}

void UberknallPreampAudioProcessor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, size));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UberknallPreampAudioProcessor();
}
