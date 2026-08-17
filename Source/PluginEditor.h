#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class BBKBlack19AudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit BBKBlack19AudioProcessorEditor (BBKBlack19AudioProcessor&);
    ~BBKBlack19AudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    BBKBlack19AudioProcessor& processor;

    juce::Label title;
    juce::Label sampleRate;
    juce::Label status;
    juce::Label spec;
    juce::ToggleButton enable { "BLACK-19 ON" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BBKBlack19AudioProcessorEditor)
};
