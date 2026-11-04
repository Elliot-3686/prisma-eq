#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/DspBasics.h"
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

private:
    template <typename Sample>
    void processOutput (juce::AudioBuffer<Sample>&);

    juce::AudioProcessorValueTreeState state;

    struct Output
    {
        std::atomic<float>* bypass = nullptr;
        std::atomic<float>* gain = nullptr;
        std::atomic<float>* pan = nullptr;
        std::atomic<float>* panMode = nullptr;
        std::atomic<float>* invert = nullptr;
    } out;

    prisma::dsp::OnePoleSmoother gainSmoother, panSmoother;
    prisma::dsp::LinearRamp bypassRamp, invertRamp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrismaProcessor)
};
