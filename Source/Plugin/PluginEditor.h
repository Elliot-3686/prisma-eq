#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PrismaProcessor;

/// Phase 1 placeholder editor (dark theme, French). The interactive display
/// arrives with the "interface minimale" milestone.
class PrismaEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PrismaEditor (PrismaProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrismaEditor)
};
