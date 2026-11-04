#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace prisma::ui
{
    /// Bandeau de la bande sélectionnée : valeurs exactes, saisie directe au clavier.
    class BandStrip final : public juce::Component
    {
    public:
        explicit BandStrip (juce::AudioProcessorValueTreeState&);
        void setBand (int band);
        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
        using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
        using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

        juce::AudioProcessorValueTreeState& state;
        int band = -1;
        juce::Label title;
        juce::ComboBox shape, placement;
        juce::Slider freq, gain, q, slope;
        juce::Label freqL, gainL, qL, slopeL;
        juce::ToggleButton active, gainQ;
        std::unique_ptr<SliderAtt> freqA, gainA, qA, slopeA;
        std::unique_ptr<ComboAtt> shapeA, placementA;
        std::unique_ptr<ButtonAtt> activeA, gainQA;
    };

    /// Barre du bas : mode de traitement, échelle de l'afficheur, sortie, bypass.
    class BottomBar final : public juce::Component
    {
    public:
        explicit BottomBar (juce::AudioProcessorValueTreeState&);
        std::function<void (float)> onRangeChanged;
        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        juce::ComboBox mode, range;
        juce::Slider outGain, gainScale;
        juce::Label outL, scaleL;
        juce::ToggleButton bypass, autoGain, invert;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeA;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outA, scaleA;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassA, autoA, invertA;
    };
}
