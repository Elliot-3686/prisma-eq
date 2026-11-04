#pragma once

#include <juce_core/juce_core.h>

// Chaînes de l'interface, regroupées pour une future traduction anglaise.

namespace prisma::ui::str
{
    inline const juce::String band        = juce::String::fromUTF8 ("Bande");
    inline const juce::String shape       = juce::String::fromUTF8 ("Forme");
    inline const juce::String slope       = juce::String::fromUTF8 ("Pente");
    inline const juce::String placement   = juce::String::fromUTF8 ("Placement");
    inline const juce::String active      = juce::String::fromUTF8 ("Active");
    inline const juce::String remove      = juce::String::fromUTF8 ("Supprimer la bande");
    inline const juce::String freq        = juce::String::fromUTF8 ("Fréquence");
    inline const juce::String gain        = juce::String::fromUTF8 ("Gain");
    inline const juce::String q           = juce::String::fromUTF8 ("Q");
    inline const juce::String gainQ       = juce::String::fromUTF8 ("Gain-Q");
    inline const juce::String noBand      = juce::String::fromUTF8 ("Double-cliquez sur la courbe pour créer une bande · glisser = fréquence/gain · molette = Q ou pente · clic droit = options");
    inline const juce::String mode        = juce::String::fromUTF8 ("Mode");
    inline const juce::String output      = juce::String::fromUTF8 ("Sortie");
    inline const juce::String bypass      = juce::String::fromUTF8 ("Bypass");
    inline const juce::String autoGain    = juce::String::fromUTF8 ("Gain auto");
    inline const juce::String gainScale   = juce::String::fromUTF8 ("Échelle");
    inline const juce::String range       = juce::String::fromUTF8 ("Échelle dB");
    inline const juce::String phase       = juce::String::fromUTF8 ("Phase Ø");
    inline const juce::String modeNotYet  = juce::String::fromUTF8 ("(seul Zero Latency est actif dans cette version)");
}
