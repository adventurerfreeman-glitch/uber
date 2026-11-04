#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class UberknallPreampAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit UberknallPreampAudioProcessorEditor (UberknallPreampAudioProcessor&);
    ~UberknallPreampAudioProcessorEditor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    UberknallPreampAudioProcessor& audioProcessor;
    juce::Slider drive, output;
    juce::Label title, driveLabel, outputLabel, modeLabel;
    juce::ToggleButton highGainButton { "HIGH GAIN" };
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> driveAttachment, outputAttachment;
    std::unique_ptr<ButtonAttachment> highGainAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UberknallPreampAudioProcessorEditor)
};
