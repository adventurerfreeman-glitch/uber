#pragma once
#include <JuceHeader.h>

class UberknallPreampAudioProcessor : public juce::AudioProcessor
{
public:
    UberknallPreampAudioProcessor();
    ~UberknallPreampAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Uberknall Preamp"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;

private:
    struct ChannelState
    {
        float dcInX = 0.0f, dcInY = 0.0f;
        float dcOutX = 0.0f, dcOutY = 0.0f;
        float low1 = 0.0f, low2 = 0.0f, low3 = 0.0f;
    };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    float processSample (float x, ChannelState& s, float drive, bool highGain, float sr);
    static float onePoleLowpass (float x, float& z, float cutoff, float sr);
    static float softStage (float x, float asymmetry, float headroom);

    std::array<ChannelState, 2> state;
    double currentSampleRate = 44100.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UberknallPreampAudioProcessor)
};
