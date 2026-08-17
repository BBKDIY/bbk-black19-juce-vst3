#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include <vector>
#include "Black19Kernel.h"

class BBKBlack19AudioProcessor final : public juce::AudioProcessor
{
public:
    BBKBlack19AudioProcessor();
    ~BBKBlack19AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override { return true; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "BBK Black-19"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return parameters; }
    double getCurrentSampleRateForUI() const noexcept { return currentSampleRate.load(); }
    bool isRateValidForUI() const noexcept { return valid192k.load(); }
    bool isEnabledForUI() const noexcept;

private:
    struct ChannelState
    {
        std::array<double, bbk::black19::numTaps> history {};
        int writeIndex = 0;

        void clear() noexcept
        {
            history.fill (0.0);
            writeIndex = 0;
        }
    };

    template <typename SampleType>
    void process (juce::AudioBuffer<SampleType>& buffer);

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState parameters;
    std::vector<ChannelState> channels;
    juce::SmoothedValue<double, juce::ValueSmoothingTypes::Linear> wetMix;

    std::atomic<double> currentSampleRate { 0.0 };
    std::atomic<bool> valid192k { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BBKBlack19AudioProcessor)
};
