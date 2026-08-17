#include "PluginEditor.h"

namespace
{
void prepareLabel (juce::Label& label)
{
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, juce::Colours::white);
}
}

BBKBlack19AudioProcessorEditor::BBKBlack19AudioProcessorEditor (BBKBlack19AudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    prepareLabel (title);
    title.setText ("BBK Black-19", juce::dontSendNotification);
    title.setFont (juce::Font (24.0f, juce::Font::bold));
    addAndMakeVisible (title);

    prepareLabel (sampleRate);
    addAndMakeVisible (sampleRate);

    prepareLabel (status);
    addAndMakeVisible (status);

    prepareLabel (spec);
    spec.setText ("192 kHz only | 19 taps | pass 0-20 kHz | stop 76-96 kHz\n"
                  "linear phase | 9-sample matched A/B delay | 10 ms switch crossfade",
                  juce::dontSendNotification);
    addAndMakeVisible (spec);

    enable.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    addAndMakeVisible (enable);
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processor.getAPVTS(), "enabled", enable);

    setSize (560, 240);
    startTimerHz (4);
    timerCallback();
}

void BBKBlack19AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff171717));
    g.setColour (juce::Colour (0xff505050));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (8.0f), 8.0f, 1.0f);
}

void BBKBlack19AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (18);
    title.setBounds (area.removeFromTop (38));
    sampleRate.setBounds (area.removeFromTop (28));
    status.setBounds (area.removeFromTop (28));
    area.removeFromTop (8);
    enable.setBounds (area.removeFromTop (36).withSizeKeepingCentre (170, 32));
    area.removeFromTop (8);
    spec.setBounds (area.removeFromTop (56));
}

void BBKBlack19AudioProcessorEditor::timerCallback()
{
    const double sr = processor.getCurrentSampleRateForUI();
    sampleRate.setText ("Host sample rate: " + juce::String (sr, 0) + " Hz",
                        juce::dontSendNotification);

    if (processor.isRateValidForUI())
    {
        enable.setEnabled (true);
        status.setText (processor.isEnabledForUI()
                            ? "ACTIVE - 192 kHz"
                            : "BYPASS - 192 kHz, latency matched",
                        juce::dontSendNotification);
    }
    else
    {
        enable.setEnabled (false);
        status.setText ("SAFETY BYPASS - requires 192000 Hz from host",
                        juce::dontSendNotification);
    }
}
