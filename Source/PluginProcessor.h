#pragma once

#include <JuceHeader.h>

class UberknallInspiredAudioProcessor final : public juce::AudioProcessor
{
public:
    UberknallInspiredAudioProcessor();
    ~UberknallInspiredAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    struct Biquad
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;
        double z1 = 0.0, z2 = 0.0;

        void reset() noexcept { z1 = z2 = 0.0; }
        void set (double newB0, double newB1, double newB2, double newA0, double newA1, double newA2) noexcept;
        float process (float input) noexcept;
    };

    void updateToneCoefficients();
    static void makeLowShelf (Biquad&, double sampleRate, double frequency, double gainDb);
    static void makePeak (Biquad&, double sampleRate, double frequency, double gainDb, double q);
    static void makeHighShelf (Biquad&, double sampleRate, double frequency, double gainDb);

    double currentSampleRate = 44100.0;
    Biquad bassFilter, midFilter, trebleFilter, presenceFilter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UberknallInspiredAudioProcessor)
};
