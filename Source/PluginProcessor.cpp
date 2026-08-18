#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

BBKBlack19AudioProcessor::BBKBlack19AudioProcessor()
: AudioProcessor (BusesProperties()
    .withInput ("Input", juce::AudioChannelSet::stereo(), true)
    .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
  parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout BBKBlack19AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "enabled", 1 }, "Black-19 enabled", false));
    return layout;
}

void BBKBlack19AudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate.store (sampleRate);

    const bool valid = std::abs (sampleRate - static_cast<double> (bbk::black19::sampleRateHz)) < 0.5;
    valid192k.store (valid);

    const int requiredChannels = juce::jmax (getTotalNumInputChannels(), getTotalNumOutputChannels());
    channels.resize (static_cast<std::size_t> (requiredChannels));
    for (auto& channel : channels)
        channel.clear();

    wetMix.reset (sampleRate, 0.010);
    wetMix.setCurrentAndTargetValue (0.0);

    // IDENTITY TEST BUILD: no DSP, no delay line, zero latency always.
    // This build exists purely to test whether inserting ANY JUCE/VST3
    // plugin into this 192 kHz signal path causes slow-motion corruption,
    // independent of Black-19's own FIR/delay-line/crossfade code.
    setLatencySamples (0);
}

bool BBKBlack19AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    if (input != output)
        return false;

    return output == juce::AudioChannelSet::mono()
        || output == juce::AudioChannelSet::stereo();
}

bool BBKBlack19AudioProcessor::isEnabledForUI() const noexcept
{
    if (const auto* value = parameters.getRawParameterValue ("enabled"))
        return value->load() >= 0.5f;

    return false;
}

template <typename SampleType>
void BBKBlack19AudioProcessor::process (juce::AudioBuffer<SampleType>&)
{
    // IDENTITY TEST BUILD: intentionally does nothing. The AudioBuffer JUCE
    // hands to processBlock already contains the input audio; leaving it
    // untouched is a perfect, zero-latency, zero-DSP passthrough.
}

void BBKBlack19AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    process (buffer);
}

void BBKBlack19AudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    process (buffer);
}

juce::AudioProcessorEditor* BBKBlack19AudioProcessor::createEditor()
{
    return new BBKBlack19AudioProcessorEditor (*this);
}

void BBKBlack19AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void BBKBlack19AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BBKBlack19AudioProcessor();
}
