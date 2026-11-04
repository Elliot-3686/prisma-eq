#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/DspBasics.h"
#include "DSP/EqEngine.h"
#include "Params/ParameterLayout.h"

class PrismaProcessor final : public juce::AudioProcessor
{
public:
    PrismaProcessor();
    ~PrismaProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override { return true; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
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

    juce::AudioProcessorValueTreeState& getState() noexcept { return state; }

    /// Saved-state format version (bump + migrate in setStateInformation).
    static constexpr int kStateVersion = 1;

    /// Réglages courants d'une bande, lus depuis l'APVTS (thread quelconque, sans allocation).
    prisma::dsp::BandSettings readBand (int index) const noexcept;
    double getGainScale() const noexcept { return out.gainScale->load() / 100.0; }

private:
    template <typename Sample>
    void processAll (juce::AudioBuffer<Sample>&);
    void pushParameters() noexcept;

    juce::AudioProcessorValueTreeState state;

    struct Output
    {
        std::atomic<float>* bypass = nullptr;
        std::atomic<float>* gain = nullptr;
        std::atomic<float>* pan = nullptr;
        std::atomic<float>* panMode = nullptr;
        std::atomic<float>* invert = nullptr;
        std::atomic<float>* autoGain = nullptr;
        std::atomic<float>* gainScale = nullptr;
    } out;

    struct BandPtrs
    {
        std::atomic<float>* used = nullptr;
        std::atomic<float>* on = nullptr;
        std::atomic<float>* shape = nullptr;
        std::atomic<float>* slope = nullptr;
        std::atomic<float>* brick = nullptr;
        std::atomic<float>* freq = nullptr;
        std::atomic<float>* gain = nullptr;
        std::atomic<float>* q = nullptr;
        std::atomic<float>* place = nullptr;
        std::atomic<float>* gainQ = nullptr;
    };
    std::array<BandPtrs, prisma::params::kNumBands> bandPtrs;

    static constexpr int kChunk = 512;
    std::array<double, 2 * kChunk> dry {};
    prisma::dsp::EqEngine engine;

    prisma::dsp::OnePoleSmoother gainSmoother, panSmoother, autoGainSmoother;
    prisma::dsp::LinearRamp bypassRamp, invertRamp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrismaProcessor)
};
