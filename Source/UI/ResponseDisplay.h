#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>

class PrismaProcessor;

namespace prisma::ui
{
    /// Afficheur de la réponse : grille Hz/dB, courbe globale jaune (réponse numérique exacte
    /// des biquads), points de bande déplaçables. Redessiné seulement quand un paramètre change.
    class ResponseDisplay final : public juce::Component,
                                  private juce::AudioProcessorValueTreeState::Listener,
                                  private juce::Timer
    {
    public:
        explicit ResponseDisplay (PrismaProcessor&);
        ~ResponseDisplay() override;

        void setRangeDb (float r) { rangeDb = r; repaint(); }
        int getSelectedBand() const noexcept { return selected; }
        void selectBand (int b);
        std::function<void (int)> onSelectionChanged;

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    private:
        void parameterChanged (const juce::String&, float) override { dirty = true; }
        void timerCallback() override;

        float xForFreq (double hz) const;
        double freqForX (float x) const;
        float yForDb (double db) const;
        double dbForY (float y) const;
        juce::Point<float> bandPoint (int band) const;
        int hitBand (juce::Point<float>) const;
        void showBandMenu (int band);

        juce::RangedAudioParameter* param (int band, const char* name) const;
        float value (int band, const char* name) const;
        void setValue (int band, const char* name, float v);

        PrismaProcessor& proc;
        juce::AudioProcessorValueTreeState& state;
        std::atomic<bool> dirty { true };
        float rangeDb = 12.0f;
        int selected = -1, dragging = -1;
    };
}
