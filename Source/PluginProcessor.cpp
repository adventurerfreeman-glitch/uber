#include "PluginProcessor.h"
#include <cmath>

namespace
{
    float readParameter (const juce::AudioProcessorValueTreeState& state, const char* id) noexcept
    {
        if (auto* value = state.getRawParameterValue (id))
            return value->load (std::memory_order_relaxed);
        return 0.0f;
    }

    bool getBoolParameter (const juce::AudioProcessorValueTreeState& state, const char* id) noexcept
    {
        return readParameter (state, id) >= 0.5f;
    }

    double safeFrequency (double frequency, double sampleRate) noexcept
    {
        return juce::jlimit (10.0, sampleRate * 0.45, frequency);
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

    layout.add (std::make_unique<FloatParam> (juce::ParameterID { "gain", 1 }, "Gain", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.0f));
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
    currentSampleRate = juce::jmax (sampleRate, 8000.0);
    for (auto& filter : bassFilter) filter.reset();
    for (auto& filter : midFilter) filter.reset();
    for (auto& filter : trebleFilter) filter.reset();
    for (auto& filter : presenceFilter) filter.reset();

    constexpr double rampSeconds = 0.025;
    driveSmooth.reset (currentSampleRate, rampSeconds);
    driveBypassSmooth.reset (currentSampleRate, rampSeconds);
    toneBypassSmooth.reset (currentSampleRate, rampSeconds);
    bassSmooth.reset (currentSampleRate, rampSeconds);
    midSmooth.reset (currentSampleRate, rampSeconds);
    trebleSmooth.reset (currentSampleRate, rampSeconds);
    presenceSmooth.reset (currentSampleRate, rampSeconds);
    masterSmooth.reset (currentSampleRate, rampSeconds);

    driveSmooth.setCurrentAndTargetValue (readParameter (parameters, "gain"));
    driveBypassSmooth.setCurrentAndTargetValue (getBoolParameter (parameters, "driveBypass") ? 1.0f : 0.0f);
    toneBypassSmooth.setCurrentAndTargetValue (getBoolParameter (parameters, "toneBypass") ? 1.0f : 0.0f);
    bassSmooth.setCurrentAndTargetValue (readParameter (parameters, "bass"));
    midSmooth.setCurrentAndTargetValue (readParameter (parameters, "mid"));
    trebleSmooth.setCurrentAndTargetValue (readParameter (parameters, "treble"));
    presenceSmooth.setCurrentAndTargetValue (readParameter (parameters, "presence"));
    masterSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (readParameter (parameters, "master")));

    updateToneCoefficients (bassSmooth.getCurrentValue(), midSmooth.getCurrentValue(),
                            trebleSmooth.getCurrentValue(), presenceSmooth.getCurrentValue());
}

void UberknallInspiredAudioProcessor::releaseResources() {}

void UberknallInspiredAudioProcessor::updateToneCoefficients (float bassDb, float midDb, float trebleDb, float presenceDb)
{
    // Stable, conventional biquad approximations of an amp-style T/M/B + Presence network.
    // These are intentionally a DSP voicing model, not a claim of an exact passive circuit solution.
    for (int channel = 0; channel < 2; ++channel)
    {
        makeLowShelf (bassFilter[(size_t) channel], currentSampleRate, 115.0, bassDb);
        makePeak (midFilter[(size_t) channel], currentSampleRate, 430.0, midDb, 0.72);
        makeHighShelf (trebleFilter[(size_t) channel], currentSampleRate, 2400.0, trebleDb);
        makeHighShelf (presenceFilter[(size_t) channel], currentSampleRate, 4300.0, presenceDb);
    }
}

void UberknallInspiredAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int inputChannels = getTotalNumInputChannels();
    const int outputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (int channel = inputChannels; channel < outputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);

    driveSmooth.setTargetValue (readParameter (parameters, "gain"));
    driveBypassSmooth.setTargetValue (getBoolParameter (parameters, "driveBypass") ? 1.0f : 0.0f);
    toneBypassSmooth.setTargetValue (getBoolParameter (parameters, "toneBypass") ? 1.0f : 0.0f);
    bassSmooth.setTargetValue (readParameter (parameters, "bass"));
    midSmooth.setTargetValue (readParameter (parameters, "mid"));
    trebleSmooth.setTargetValue (readParameter (parameters, "treble"));
    presenceSmooth.setTargetValue (readParameter (parameters, "presence"));
    masterSmooth.setTargetValue (juce::Decibels::decibelsToGain (readParameter (parameters, "master")));

    // Update filter coefficients once per audio block from smoothed control values.
    // This avoids abrupt coefficient jumps/zipper artefacts when EQ knobs move.
    updateToneCoefficients (bassSmooth.getCurrentValue(), midSmooth.getCurrentValue(),
                            trebleSmooth.getCurrentValue(), presenceSmooth.getCurrentValue());

    // Parameter smoothers advance once per sample and are shared by both channels.
    // Each channel has its own independent filter state to prevent stereo crosstalk.
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float drive = driveSmooth.getNextValue();
        const float driveBypass = driveBypassSmooth.getNextValue();
        const float toneBypass = toneBypassSmooth.getNextValue();
        const float master = masterSmooth.getNextValue();
        // Gain is a real pre-drive gain control: 0 = approximately unity/clean,
        // 10 = +24 dB before the soft-clipping stage. No automatic normalization
        // is applied, so increasing Gain also increases signal level; use Master
        // to set the final output level, as on a conventional amp-style pedal.
        const float preGain = juce::Decibels::decibelsToGain (drive * 2.4f);

        for (int channel = 0; channel < inputChannels; ++channel)
        {
            const float input = buffer.getSample (channel, sample);
            const float driven = std::tanh (input * preGain);
            const float afterDrive = input + (driven - input) * (1.0f - driveBypass);

            const auto ch = (size_t) channel;
            float toneProcessed = bassFilter[ch].process (afterDrive);
            toneProcessed = midFilter[ch].process (toneProcessed);
            toneProcessed = trebleFilter[ch].process (toneProcessed);
            toneProcessed = presenceFilter[ch].process (toneProcessed);
            const float afterTone = toneProcessed + (afterDrive - toneProcessed) * toneBypass;

            const float output = afterTone * master;
            buffer.setSample (channel, sample, std::isfinite (output) ? output : 0.0f);
        }
    }

    // Advance EQ smoothers for the next coefficient update. Coefficients are updated
    // at block boundaries; the short ramp prevents zipper-like jumps during automation.
    bassSmooth.skip (numSamples);
    midSmooth.skip (numSamples);
    trebleSmooth.skip (numSamples);
    presenceSmooth.skip (numSamples);
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

void UberknallInspiredAudioProcessor::Biquad::set (double newB0, double newB1, double newB2,
                                                    double newA0, double newA1, double newA2) noexcept
{
    if (! std::isfinite (newA0) || std::abs (newA0) < 1.0e-12)
        return;

    const double invA0 = 1.0 / newA0;
    const double nb0 = newB0 * invA0;
    const double nb1 = newB1 * invA0;
    const double nb2 = newB2 * invA0;
    const double na1 = newA1 * invA0;
    const double na2 = newA2 * invA0;

    if (std::isfinite (nb0) && std::isfinite (nb1) && std::isfinite (nb2)
        && std::isfinite (na1) && std::isfinite (na2))
    {
        b0 = nb0; b1 = nb1; b2 = nb2; a1 = na1; a2 = na2;
    }
}

float UberknallInspiredAudioProcessor::Biquad::process (float input) noexcept
{
    const double output = b0 * static_cast<double> (input) + z1;
    z1 = b1 * static_cast<double> (input) - a1 * output + z2;
    z2 = b2 * static_cast<double> (input) - a2 * output;
    const auto result = static_cast<float> (output);
    return std::isfinite (result) ? result : 0.0f;
}

void UberknallInspiredAudioProcessor::makeLowShelf (Biquad& f, double sr, double frequency, double gainDb)
{
    const double A = std::pow (10.0, static_cast<double> (gainDb) / 40.0);
    const double w = 2.0 * juce::MathConstants<double>::pi * safeFrequency (frequency, sr) / sr;
    const double c = std::cos (w), s = std::sin (w);
    const double alpha = (s / 2.0) * std::sqrt (2.0);
    const double beta = 2.0 * std::sqrt (A) * alpha;
    f.set (A * ((A + 1.0) - (A - 1.0) * c + beta),
           2.0 * A * ((A - 1.0) - (A + 1.0) * c),
           A * ((A + 1.0) - (A - 1.0) * c - beta),
           (A + 1.0) + (A - 1.0) * c + beta,
           -2.0 * ((A - 1.0) + (A + 1.0) * c),
           (A + 1.0) + (A - 1.0) * c - beta);
}

void UberknallInspiredAudioProcessor::makePeak (Biquad& f, double sr, double frequency, double gainDb, double q)
{
    const double A = std::pow (10.0, static_cast<double> (gainDb) / 40.0);
    const double w = 2.0 * juce::MathConstants<double>::pi * safeFrequency (frequency, sr) / sr;
    const double alpha = std::sin (w) / (2.0 * juce::jmax (0.1, q));
    const double c = std::cos (w);
    f.set (1.0 + alpha * A, -2.0 * c, 1.0 - alpha * A,
           1.0 + alpha / A, -2.0 * c, 1.0 - alpha / A);
}

void UberknallInspiredAudioProcessor::makeHighShelf (Biquad& f, double sr, double frequency, double gainDb)
{
    const double A = std::pow (10.0, static_cast<double> (gainDb) / 40.0);
    const double w = 2.0 * juce::MathConstants<double>::pi * safeFrequency (frequency, sr) / sr;
    const double c = std::cos (w), s = std::sin (w);
    const double alpha = (s / 2.0) * std::sqrt (2.0);
    const double beta = 2.0 * std::sqrt (A) * alpha;
    f.set (A * ((A + 1.0) + (A - 1.0) * c + beta),
           -2.0 * A * ((A - 1.0) + (A + 1.0) * c),
           A * ((A + 1.0) + (A - 1.0) * c - beta),
           (A + 1.0) - (A - 1.0) * c + beta,
           2.0 * ((A - 1.0) - (A + 1.0) * c),
           (A + 1.0) - (A - 1.0) * c - beta);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new UberknallInspiredAudioProcessor();
}
