#include "PluginProcessor.h"
#include <cmath>

namespace
{
    constexpr float pi = juce::MathConstants<float>::pi;

    float getParameter (const juce::AudioProcessorValueTreeState& state, const char* id)
    {
        if (auto* value = state.getRawParameterValue (id))
            return value->load();
        return 0.0f;
    }

    bool getBoolParameter (const juce::AudioProcessorValueTreeState& state, const char* id)
    {
        return getParameter (state, id) >= 0.5f;
    }
}

UberknallInspiredAudioProcessor::UberknallInspiredAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout UberknallInspiredAudioProcessor::createParameterLayout()
{
    using FloatParam = juce::AudioParameterFloat;
    using BoolParam = juce::AudioParameterBool;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<FloatParam> (juce::ParameterID { "drive", 1 }, "Drive", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.0f));
    layout.add (std::make_unique<BoolParam> (juce::ParameterID { "driveBypass", 1 }, "Drive Bypass", false));
    layout.add (std::make_unique<FloatParam> (juce::ParameterID { "bass", 1 }, "Bass", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, " dB"));
    layout.add (std::make_unique<FloatParam> (juce::ParameterID { "mid", 1 }, "Mid", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, " dB"));
    layout.add (std::make_unique<FloatParam> (juce::ParameterID { "treble", 1 }, "Treble", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, " dB"));
    layout.add (std::make_unique<FloatParam> (juce::ParameterID { "presence", 1 }, "Presence", juce::NormalisableRange<float> (-6.0f, 9.0f, 0.1f), 0.0f, " dB"));
    layout.add (std::make_unique<BoolParam> (juce::ParameterID { "toneBypass", 1 }, "Tone Stack Bypass", false));
    layout.add (std::make_unique<FloatParam> (juce::ParameterID { "master", 1 }, "Master", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f, " dB"));
    return layout;
}

const juce::String UberknallInspiredAudioProcessor::getName() const { return JucePlugin_Name; }
bool UberknallInspiredAudioProcessor::acceptsMidi() const { return false; }
bool UberknallInspiredAudioProcessor::producesMidi() const { return false; }
bool UberknallInspiredAudioProcessor::isMidiEffect() const { return false; }
double UberknallInspiredAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int UberknallInspiredAudioProcessor::getNumPrograms() { return 1; }
int UberknallInspiredAudioProcessor::getCurrentProgram() { return 0; }
void UberknallInspiredAudioProcessor::setCurrentProgram (int) {}
const juce::String UberknallInspiredAudioProcessor::getProgramName (int) { return {}; }
void UberknallInspiredAudioProcessor::changeProgramName (int, const juce::String&) {}

bool UberknallInspiredAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& input = layouts.getMainInputChannelSet();
    const auto& output = layouts.getMainOutputChannelSet();
    return input == output && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

void UberknallInspiredAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    bassFilter.reset(); midFilter.reset(); trebleFilter.reset(); presenceFilter.reset();
    updateToneCoefficients();
}

void UberknallInspiredAudioProcessor::releaseResources() {}

void UberknallInspiredAudioProcessor::updateToneCoefficients()
{
    makeLowShelf (bassFilter, currentSampleRate, 120.0, getParameter (parameters, "bass"));
    makePeak (midFilter, currentSampleRate, 420.0, getParameter (parameters, "mid"), 0.75);
    makeHighShelf (trebleFilter, currentSampleRate, 2200.0, getParameter (parameters, "treble"));
    makeHighShelf (presenceFilter, currentSampleRate, 3900.0, getParameter (parameters, "presence"));
}

void UberknallInspiredAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = getTotalNumInputChannels();
    const int samples = buffer.getNumSamples();

    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
        buffer.clear (channel, 0, samples);

    const bool driveIsBypassed = getBoolParameter (parameters, "driveBypass");
    const bool toneIsBypassed = getBoolParameter (parameters, "toneBypass");
    const float drive = getParameter (parameters, "drive");
    const float masterGain = juce::Decibels::decibelsToGain (getParameter (parameters, "master"));

    if (! toneIsBypassed)
        updateToneCoefficients();

    const float driveAmount = 1.0f + drive * 1.65f;
    const float normaliser = std::tanh (driveAmount);

    for (int channel = 0; channel < channels; ++channel)
    {
        auto* data = buffer.getWritePointer (channel);
        for (int i = 0; i < samples; ++i)
        {
            float x = data[i];

            if (! driveIsBypassed)
                x = std::tanh (x * driveAmount) / normaliser;

            if (! toneIsBypassed)
            {
                x = bassFilter.process (x);
                x = midFilter.process (x);
                x = trebleFilter.process (x);
                x = presenceFilter.process (x);
            }

            data[i] = x * masterGain;
        }
    }
}

bool UberknallInspiredAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* UberknallInspiredAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void UberknallInspiredAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void UberknallInspiredAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

void UberknallInspiredAudioProcessor::Biquad::set (double newB0, double newB1, double newB2, double newA0, double newA1, double newA2) noexcept
{
    if (std::abs (newA0) < 1.0e-12) return;
    b0 = newB0 / newA0; b1 = newB1 / newA0; b2 = newB2 / newA0;
    a1 = newA1 / newA0; a2 = newA2 / newA0;
}

float UberknallInspiredAudioProcessor::Biquad::process (float input) noexcept
{
    const double output = b0 * input + z1;
    z1 = b1 * input - a1 * output + z2;
    z2 = b2 * input - a2 * output;
    return static_cast<float> (output);
}

void UberknallInspiredAudioProcessor::makeLowShelf (Biquad& f, double sr, double freq, double gainDb)
{
    const double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * juce::MathConstants<double>::pi * freq / sr;
    const double c = std::cos (w), s = std::sin (w), alpha = s / 2.0 * std::sqrt (2.0);
    const double beta = 2.0 * std::sqrt (A) * alpha;
    f.set (A*((A+1)-(A-1)*c+beta), 2*A*((A-1)-(A+1)*c), A*((A+1)-(A-1)*c-beta), (A+1)+(A-1)*c+beta, -2*((A-1)+(A+1)*c), (A+1)+(A-1)*c-beta);
}

void UberknallInspiredAudioProcessor::makePeak (Biquad& f, double sr, double freq, double gainDb, double q)
{
    const double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * juce::MathConstants<double>::pi * freq / sr;
    const double alpha = std::sin (w) / (2.0 * q), c = std::cos (w);
    f.set (1.0 + alpha*A, -2.0*c, 1.0-alpha*A, 1.0+alpha/A, -2.0*c, 1.0-alpha/A);
}

void UberknallInspiredAudioProcessor::makeHighShelf (Biquad& f, double sr, double freq, double gainDb)
{
    const double A = std::pow (10.0, gainDb / 40.0), w = 2.0 * juce::MathConstants<double>::pi * freq / sr;
    const double c = std::cos (w), s = std::sin (w), alpha = s / 2.0 * std::sqrt (2.0);
    const double beta = 2.0 * std::sqrt (A) * alpha;
    f.set (A*((A+1)+(A-1)*c+beta), -2*A*((A-1)+(A+1)*c), A*((A+1)+(A-1)*c-beta), (A+1)-(A-1)*c+beta, 2*((A-1)-(A+1)*c), (A+1)-(A-1)*c-beta);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UberknallInspiredAudioProcessor();
}
