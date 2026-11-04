#include "Plugin/PluginProcessor.h"
#include "Plugin/PluginEditor.h"
#include "DSP/Tuning.h"

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
    out.autoGain  = state.getRawParameterValue (params::id::autoGain);
    out.gainScale = state.getRawParameterValue (params::id::gainScale);

    for (int b = 0; b < params::kNumBands; ++b)
    {
        auto get = [&] (const char* name) { return state.getRawParameterValue (params::bandId (b, name)); };
        auto& p = bandPtrs[(size_t) b];
        p.used = get (params::id::used);       p.on = get (params::id::enabled);
        p.shape = get (params::id::shape);     p.slope = get (params::id::slope);
        p.brick = get (params::id::brickwall); p.freq = get (params::id::freq);
        p.gain = get (params::id::gain);       p.q = get (params::id::q);
        p.place = get (params::id::placement); p.gainQ = get (params::id::gainQ);
    }
}

dsp::BandSettings PrismaProcessor::readBand (int index) const noexcept
{
    const auto& p = bandPtrs[(size_t) index];
    dsp::BandSettings s;
    s.used = p.used->load() > 0.5f;
    s.enabled = p.on->load() > 0.5f;
    s.placement = (dsp::Placement) juce::jlimit (0, 4, (int) p.place->load());
    s.params.shape = (dsp::Shape) juce::jlimit (0, 9, (int) p.shape->load());
    s.params.slope = p.slope->load();
    s.params.brickwall = p.brick->load() > 0.5f;
    s.params.freq = p.freq->load();
    s.params.gainDb = p.gain->load();
    s.params.q = p.q->load();
    s.params.gainQ = p.gainQ->load() > 0.5f;
    return s;
}

void PrismaProcessor::pushParameters() noexcept
{
    for (int b = 0; b < params::kNumBands; ++b)
        engine.setBand (b, readBand (b));
    engine.setGainScale (getGainScale());
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
    pushParameters();
    engine.prepare (sampleRate, juce::jlimit (1, 2, getTotalNumOutputChannels()));
    autoGainSmoother.prepare (sampleRate, dsp::tuning::kAutoGainSeconds);
    autoGainSmoother.reset (out.autoGain->load() > 0.5f ? dsp::dbToGain (engine.getAutoGainDb()) : 1.0);
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
    processAll (buffer);
}

void PrismaProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    processAll (buffer);
}

// Moteur d'EQ puis section de sortie, par tranches de kChunk échantillons (copie sèche
// préallouée pour le bypass doux). Sans allocation ni verrou : atomics lus une fois par bloc.
template <typename Sample>
void PrismaProcessor::processAll (juce::AudioBuffer<Sample>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);
    if (numChannels == 0 || numSamples == 0) return;

    pushParameters();
    const float gainDb = out.gain->load();
    gainSmoother.setTarget (gainDb <= params::kOutGainMin ? 0.0 : dsp::dbToGain (gainDb));
    panSmoother.setTarget (numChannels == 2 ? out.pan->load() / 100.0 : 0.0);
    bypassRamp.setTarget (out.bypass->load() > 0.5f ? 1.0 : 0.0);
    invertRamp.setTarget (out.invert->load() > 0.5f ? -1.0 : 1.0);
    autoGainSmoother.setTarget (out.autoGain->load() > 0.5f ? dsp::dbToGain (engine.getAutoGainDb()) : 1.0);
    const bool midSidePan = out.panMode->load() > 0.5f;

    Sample* l = buffer.getWritePointer (0);
    Sample* r = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int start = 0; start < numSamples; start += kChunk)
    {
        const int len = juce::jmin (kChunk, numSamples - start);
        Sample* cl = l + start;
        Sample* cr = r != nullptr ? r + start : nullptr;
        for (int i = 0; i < len; ++i) dry[(size_t) i] = cl[i];
        if (cr != nullptr) for (int i = 0; i < len; ++i) dry[(size_t) (kChunk + i)] = cr[i];

        engine.process (cl, cr, len);

        for (int i = 0; i < len; ++i)
        {
            const double g = gainSmoother.next() * invertRamp.next() * autoGainSmoother.next();
            const double bypassMix = bypassRamp.next();
            double gl = 1.0, gr = 1.0;
            dsp::balanceGains (panSmoother.next(), gl, gr);

            const double dryL = dry[(size_t) i];
            double wetL = cl[i] * g;
            if (cr != nullptr)
            {
                const double dryR = dry[(size_t) (kChunk + i)];
                double wetR = cr[i] * g;
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
                cr[i] = static_cast<Sample> (wetR + (dryR - wetR) * bypassMix);
            }
            cl[i] = static_cast<Sample> (wetL + (dryL - wetL) * bypassMix);
        }
    }
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
