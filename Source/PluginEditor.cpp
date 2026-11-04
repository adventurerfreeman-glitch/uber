#include "PluginEditor.h"

UberknallPreampAudioProcessorEditor::UberknallPreampAudioProcessorEditor (UberknallPreampAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (520, 300);
    title.setText ("ÜBERKNALL PREAMP", juce::dontSendNotification);
    title.setJustificationType (juce::Justification::centred);
    title.setFont (juce::Font (26.0f, juce::Font::bold));
    addAndMakeVisible (title);

    auto setupSlider = [this] (juce::Slider& s, juce::Label& l, const juce::String& text)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 24);
        s.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffc64a35));
        s.setColour (juce::Slider::thumbColourId, juce::Colours::white);
        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (s); addAndMakeVisible (l);
    };
    setupSlider (drive, driveLabel, "DRIVE");
    setupSlider (output, outputLabel, "OUTPUT");
    driveAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "drive", drive);
    outputAttachment = std::make_unique<SliderAttachment> (audioProcessor.parameters, "output", output);

    highGainButton.setClickingTogglesState (true);
    highGainButton.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    highGainButton.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xffc64a35));
    addAndMakeVisible (highGainButton);
    highGainAttachment = std::make_unique<ButtonAttachment> (audioProcessor.parameters, "highGain", highGainButton);

    modeLabel.setText ("STANDARD is default  |  HIGH GAIN changes stage gain distribution",
                       juce::dontSendNotification);
    modeLabel.setJustificationType (juce::Justification::centred);
    modeLabel.setColour (juce::Label::textColourId, juce::Colour (0xffb9b9b9));
    modeLabel.setFont (12.0f);
    addAndMakeVisible (modeLabel);
}

void UberknallPreampAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff17191d));
    g.setColour (juce::Colour (0xffc64a35));
    g.fillRect (0, 0, getWidth(), 5);
    g.setColour (juce::Colour (0xff333840));
    g.drawRoundedRectangle (12.0f, 12.0f, (float) getWidth() - 24.0f,
                            (float) getHeight() - 24.0f, 12.0f, 1.5f);
}

void UberknallPreampAudioProcessorEditor::resized()
{
    title.setBounds (25, 25, getWidth() - 50, 42);
    driveLabel.setBounds (85, 88, 150, 24);
    drive.setBounds (85, 105, 150, 145);
    outputLabel.setBounds (285, 88, 150, 24);
    output.setBounds (285, 105, 150, 145);
    highGainButton.setBounds (175, 250, 170, 28);
    modeLabel.setBounds (20, 275, getWidth() - 40, 18);
}
