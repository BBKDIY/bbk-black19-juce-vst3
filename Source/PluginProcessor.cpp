#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

BBKBlack19AudioProcessor::BBKBlack19AudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
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

    wetMix.reset (sampleRate, 0.010); // 10 ms click-free A/B crossfade
    const bool requested = isEnabledForUI();
    wetMix.setCurrentAndTargetValue ((valid && requested) ? 1.0 : 0.0);

    // At 192 kHz both wet and internal bypass paths are delayed by 9 samples,
    // making ON/OFF time-aligned. At unsupported rates the plug-in is a true
    // safety bypass and reports zero latency.
    setLatencySamples (valid ? bbk::black19::groupDelaySamples : 0);
}

bool BBKBlack19AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input  = layouts.getMainInputChannelSet();
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
void BBKBlack19AudioProcessor::process (juce::AudioBuffer<SampleType>& buffer)
{
    if (! valid192k.load())
        return; // hard safety bypass outside 192 kHz

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    wetMix.setTargetValue (isEnabledForUI() ? 1.0 : 0.0);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const double mix = wetMix.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& state = channels[static_cast<std::size_t> (ch)];
            auto* data  = buffer.getWritePointer (ch);
            const double x = static_cast<double> (data[sample]);

            // Store current input sample.
            state.history[static_cast<std::size_t> (state.writeIndex)] = x;

            // FIR output y[n] = sum_k h[k] x[n-k].
            double wet = 0.0;
            for (int k = 0; k < bbk::black19::numTaps; ++k)
            {
                int index = state.writeIndex - k;
                if (index < 0)
                    index += bbk::black19::numTaps;

                wet += bbk::black19::taps[static_cast<std::size_t> (k)]
                     * state.history[static_cast<std::size_t> (index)];
            }

            // Delay dry path by the FIR's 9-sample group delay for fair A/B.
            int dryIndex = state.writeIndex - bbk::black19::groupDelaySamples;
            if (dryIndex < 0)
                dryIndex += bbk::black19::numTaps;

            const double dry = state.history[static_cast<std::size_t> (dryIndex)];
            const double y   = dry + mix * (wet - dry);
            data[sample] = static_cast<SampleType> (y);

            if (++state.writeIndex == bbk::black19::numTaps)
                state.writeIndex = 0;
        }
    }
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
