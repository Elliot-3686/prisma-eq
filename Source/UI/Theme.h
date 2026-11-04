#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace prisma::ui::theme
{
    inline const juce::Colour background  { 0xff0e1014 };
    inline const juce::Colour panel       { 0xff161920 };
    inline const juce::Colour grid        { 0xff262b35 };
    inline const juce::Colour gridStrong  { 0xff3a4150 };
    inline const juce::Colour text        { 0xffe9e4d8 };
    inline const juce::Colour textDim     { 0xff8b8f99 };
    inline const juce::Colour curve       { 0xfff2c94c };   // courbe globale (jaune)

    /// Couleur propre à chaque bande (teinte répartie sur le cercle chromatique).
    inline juce::Colour bandColour (int index)
    {
        return juce::Colour::fromHSV (std::fmod (0.58f + 0.137f * (float) index, 1.0f), 0.55f, 0.95f, 1.0f);
    }
}
