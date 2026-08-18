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

    wetMix.reset (sampleRate, 0.010); // 10 ms click-free A/B crossfade
    const bool requested = isEnabledForUI();
    wetMix.setCurrentAndTargetValue ((valid && requested) ? 1.0 : 0.0);

    // Always report zero latency to the host, even though the FIR path
    // still has a true 9-sample (46.9us) internal group delay (the dry
    // path is still delayed to match it for a correct wet/dry crossfade).
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
void BBKBlack19AudioProcessor::process (juce::AudioBuffer<SampleType>& buffer)
{
    if (! valid192k.load())
        return; // hard safety bypass outside 192 kHz

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Defensive: prepareToPlay sizes 'channels' from getTotalNumInputChannels()/
    // getTotalNumOutputChannels(), but some hosts can call processBlock with a
    // channel count that does not exactly match that (observed when hot-inserting
    // this plugin into an already-running chain). Guarantee the per-channel
    // delay-line state is always large enough before indexing into it, instead
    // of trusting prepareToPlay's channel count alone.
    if (static_cast<int> (channels.size()) < numChannels)
    {
        const auto oldSize = channels.size();
        channels.resize (static_cast<std::size_t> (numChannels));
        for (auto i = oldSize; i < channels.size(); ++i)
            channels[i].clear();
    }

    wetMix.setTargetValue (isEnabledForUI() ? 1.0 : 0.0);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const double mix = wetMix.getNextValue();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& state = channels[static_cast<std::size_t> (ch)];
            auto* data = buffer.getWritePointer (ch);
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

            // The taps have unity DC gain, but a negative-side-lobe FIR like
            // this one can still momentarily amplify transient content beyond
            // 0 dBFS (worst-case peak gain = the L1 norm of the impulse
            // response, which is > 1.0 here). Scale the wet output by the
            // precomputed safety factor so it can never overshoot +/-1.0 and
            // hard-clip at the DAC, regardless of program material. The dry
            // path never needs this: it is an unmodified delayed copy of the
            // source, which never introduces new headroom demands.
            wet *= bbk::black19::wetSafetyGain;

            // Delay dry path by the FIR's 9-sample group delay for fair A/B.
            int dryIndex = state.writeIndex - bbk::black19::groupDelaySamples;
            if (dryIndex < 0)
                dryIndex += bbk::black19::numTaps;

            const double dry = state.history[static_cast<std::size_t> (dryIndex)];
            const double y = dry + mix * (wet - dry);
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
