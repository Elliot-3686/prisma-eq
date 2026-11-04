#include "Params/ParameterLayout.h"

namespace prisma::params
{
    namespace
    {
        constexpr int kVersion = 1; // ParameterID version hint (AU/VST3)

        juce::NormalisableRange<float> logRange (float lo, float hi)
        {
            return { lo, hi,
                     [] (float start, float end, float t) { return start * std::pow (end / start, t); },
                     [] (float start, float end, float v) { return std::log (v / start) / std::log (end / start); },
                     [] (float start, float end, float v) { return juce::jlimit (start, end, v); } };
        }

        juce::String hzText (float hz, int)
        {
            return hz >= 1000.0f ? juce::String (hz / 1000.0f, hz >= 10000.0f ? 1 : 2) + " kHz"
                                 : juce::String (hz, hz >= 100.0f ? 1 : 2) + " Hz";
        }

        /// « 1,5 kHz », « 1.5k », « 250 » → Hz.
        float hzFromText (const juce::String& t)
        {
            auto s = t.trim().toLowerCase().replaceCharacter (',', '.');
            const bool kilo = s.containsChar ('k');
            const float v = s.retainCharacters ("0123456789.-").getFloatValue();
            return kilo ? v * 1000.0f : v;
        }

        /// Valeur arrondie au centième, sans « -0.00 » dû aux arrondis flottants.
        float tidy (float v) { return std::abs (v) < 0.005f ? 0.0f : v; }

        juce::String dbText (float db, int) { return juce::String (tidy (db), 2) + " dB"; }
    }

    const juce::StringArray& shapeNames()
    {
        static const juce::StringArray names { "Cloche", "Plateau grave", "Coupe-bas", "Plateau aigu", "Coupe-haut",
                                               "Encoche", "Passe-bande", juce::String::fromUTF8 ("Plateau incliné"), "Inclinaison", "Passe-tout" };
        return names;
    }

    const juce::StringArray& placementNames()
    {
        static const juce::StringArray names { juce::String::fromUTF8 ("Stéréo"), "Gauche", "Droite", "Mid", "Side" };
        return names;
    }

    juce::String bandId (int bandIndex, const char* name)
    {
        return "b" + juce::String (bandIndex + 1).paddedLeft ('0', 2) + "_" + name;
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace juce;
        AudioProcessorValueTreeState::ParameterLayout layout;

        for (int b = 0; b < kNumBands; ++b)
        {
            const auto prefix = "Bande " + String (b + 1) + " ";
            auto group = std::make_unique<AudioProcessorParameterGroup> ("band" + String (b + 1), "Bande " + String (b + 1), juce::String::fromUTF8 (" · "));

            group->addChild (std::make_unique<AudioParameterBool> (ParameterID { bandId (b, id::used), kVersion }, prefix + juce::String::fromUTF8 ("utilisée"), false));
            group->addChild (std::make_unique<AudioParameterBool> (ParameterID { bandId (b, id::enabled), kVersion }, prefix + "active", true));
            group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { bandId (b, id::shape), kVersion }, prefix + "forme", shapeNames(), 0));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, id::slope), kVersion }, prefix + "pente",
                                                                   NormalisableRange<float> (0.0f, kMaxSlope, 0.01f), 12.0f,
                                                                   AudioParameterFloatAttributes().withLabel ("dB/oct")));
            group->addChild (std::make_unique<AudioParameterBool> (ParameterID { bandId (b, id::brickwall), kVersion }, prefix + "brickwall", false));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, id::freq), kVersion }, prefix + juce::String::fromUTF8 ("fréquence"),
                                                                   logRange (kMinFreq, kMaxFreq), 1000.0f,
                                                                   AudioParameterFloatAttributes().withStringFromValueFunction (hzText)
                                                                                                  .withValueFromStringFunction (hzFromText)));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, id::gain), kVersion }, prefix + "gain",
                                                                   NormalisableRange<float> (-kMaxGain, kMaxGain, 0.01f), 0.0f,
                                                                   AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));
            group->addChild (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, id::q), kVersion }, prefix + "Q",
                                                                   logRange (kMinQ, kMaxQ), 1.0f,
                                                                   AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                                                                   { return String (v, v < 1.0f ? 3 : 2); })));
            group->addChild (std::make_unique<AudioParameterChoice> (ParameterID { bandId (b, id::placement), kVersion }, prefix + "placement", placementNames(), 0));
            group->addChild (std::make_unique<AudioParameterBool> (ParameterID { bandId (b, id::gainQ), kVersion }, prefix + "interaction gain-Q", false));
            layout.add (std::move (group));
        }

        auto out = std::make_unique<AudioProcessorParameterGroup> ("output", "Sortie", juce::String::fromUTF8 (" · "));
        out->addChild (std::make_unique<AudioParameterBool> (ParameterID { id::bypass, kVersion }, "Bypass", false));
        out->addChild (std::make_unique<AudioParameterChoice> (ParameterID { id::mode, kVersion }, "Mode de traitement",
                                                             StringArray { "Zero Latency", "Natural Phase", "Linear Phase" }, 0));
        out->addChild (std::make_unique<AudioParameterChoice> (ParameterID { id::resolution, kVersion }, juce::String::fromUTF8 ("Résolution (phase linéaire)"),
                                                             StringArray { "Low", "Medium", "High", "Very High", "Maximum" }, 1));
        out->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id::outGain, kVersion }, "Gain de sortie",
                                                            NormalisableRange<float> (kOutGainMin, kOutGainMax, 0.01f), 0.0f,
                                                            AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
                                                            { return v <= kOutGainMin ? String ("-inf dB") : String (tidy (v), 2) + " dB"; })));
        out->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id::outPan, kVersion }, "Panoramique de sortie",
                                                            NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f,
                                                            AudioParameterFloatAttributes().withLabel ("%")));
        out->addChild (std::make_unique<AudioParameterChoice> (ParameterID { id::panMode, kVersion }, "Mode de panoramique",
                                                             StringArray { "L/R", "M/S" }, 0));
        out->addChild (std::make_unique<AudioParameterBool> (ParameterID { id::phaseInvert, kVersion }, "Inversion de phase", false));
        out->addChild (std::make_unique<AudioParameterBool> (ParameterID { id::autoGain, kVersion }, "Gain automatique", false));
        out->addChild (std::make_unique<AudioParameterFloat> (ParameterID { id::gainScale, kVersion }, juce::String::fromUTF8 ("Échelle de gain"),
                                                            NormalisableRange<float> (0.0f, 200.0f, 0.1f), 100.0f,
                                                            AudioParameterFloatAttributes().withLabel ("%")));
        out->addChild (std::make_unique<AudioParameterChoice> (ParameterID { id::character, kVersion }, juce::String::fromUTF8 ("Caractère"),
                                                             StringArray { "Clean", "Subtle", "Warm" }, 0));
        layout.add (std::move (out));
        return layout;
    }
}
