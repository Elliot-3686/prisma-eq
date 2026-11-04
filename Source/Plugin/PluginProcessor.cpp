#include "Plugin/PluginProcessor.h"
#include "Plugin/PluginEditor.h"

using namespace prisma;

PrismaProcessor::PrismaProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Entrée", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Sortie", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "PrismaEQ", params::createLayout())
{
    out.bypass  = state.getRawParameterValue (params::id::bypass);
    out.gain    = state.getRawParameterValue (params::id::outGain);
    out.pan     = state.getRawParameterValue (params::id::outPan);
    out.panMode = state.getRawParameterValue (params::id::panMode);
    out.invert  = state.getRawParameterValue (params::id::phaseInvert);
}

bool PrismaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in = layouts.getMainInputChannelSet();
    const auto& o  = layouts.getMainOutputChannelSet();
    if (in != o) return false;
    return o == juce::AudioChannelSet::mono() || o == juce::AudioChannelSet::stereo();
}

void PrismaProcessor::prepareToPlay (double sampleRate, int)
{
    gainSmoother.prepare (sampleRate, 0.02);
    panSmoother.prepare (sampleRate, 0.02);
    bypassRamp.prepare (sampleRate, 0.02);
    invertRamp.prepare (sampleRate, 0.02);

    gainSmoother.reset (out.gain->load() <= params::kOutGainMin ? 0.0 : dsp::dbToGain (out.gain->load()));
    panSmoother.reset (out.pan->load() / 100.0);
    bypassRamp.reset (out.bypass->load() > 0.5f ? 1.0 : 0.0);
    invertRamp.reset (out.invert->load() > 0.5f ? -1.0 : 1.0);
}

void PrismaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    processOutput (buffer);
}

void PrismaProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    processOutput (buffer);
}

// Phase 1: the EQ engine is not wired yet; only the output section runs.
// Everything is allocation-free and lock-free (atomics read once per block).
template <typename Sample>
void PrismaProcessor::processOutput (juce::AudioBuffer<Sample>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    if (numChannels == 0 || numSamples == 0) return;

    const float gainDb = out.gain->load();
    gainSmoother.setTarget (gainDb <= params::kOutGainMin ? 0.0 : dsp::dbToGain (gainDb));
    panSmoother.setTarget (numChannels == 2 ? out.pan->load() / 100.0 : 0.0);
    bypassRamp.setTarget (out.bypass->load() > 0.5f ? 1.0 : 0.0);
    invertRamp.setTarget (out.invert->load() > 0.5f ? -1.0 : 1.0);
    const bool midSidePan = out.panMode->load() > 0.5f;

    Sample* l = buffer.getWritePointer (0);
    Sample* r = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const double g = gainSmoother.next() * invertRamp.next();
        const double bypassMix = bypassRamp.next();
        double gl = 1.0, gr = 1.0;
        dsp::balanceGains (panSmoother.next(), gl, gr);

        const double dryL = l[i];
        double wetL = dryL * g;

        if (r != nullptr)
        {
            const double dryR = r[i];
            double wetR = dryR * g;
            if (midSidePan)
            {
                const double m = 0.5 * (wetL + wetR) * gl, s = 0.5 * (wetL - wetR) * gr;
                wetL = m + s;
                wetR = m - s;
            }
            else
            {
                wetL *= gl;
                wetR *= gr;
            }
            r[i] = static_cast<Sample> (wetR + (dryR - wetR) * bypassMix);
        }
        l[i] = static_cast<Sample> (wetL + (dryL - wetL) * bypassMix);
    }

    for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);
}

juce::AudioProcessorEditor* PrismaProcessor::createEditor()
{
    return new PrismaEditor (*this);
}

void PrismaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto copy = state.copyState();
    copy.setProperty ("stateVersion", kStateVersion, nullptr);
    if (auto xml = copy.createXml())
        copyXmlToBinary (*xml, destData);
}

void PrismaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        if (tree.hasType (state.state.getType()))
        {
            // Future migrations: switch on (int) tree.getProperty ("stateVersion", 1)
            state.replaceState (tree);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PrismaProcessor();
}
