#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "UI/ResponseDisplay.h"
#include "UI/Panels.h"

class PrismaProcessor;

/// Interface minimale : afficheur, bandeau de la bande sélectionnée, barre du bas.
class PrismaEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PrismaEditor (PrismaProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void selectBand (int band) { display.selectBand (band); }

private:
    prisma::ui::ResponseDisplay display;
    prisma::ui::BandStrip strip;
    prisma::ui::BottomBar bottom;
    juce::TooltipWindow tooltips { this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrismaEditor)
};
