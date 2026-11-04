#pragma once

// Moteur Zero Latency : 24 bandes, chacune en cascade de biquads TDF-II en
// double précision. Paramètres lissés (log-fréquence, dB, log-Q), coefficients
// recalculés tous les kUpdateInterval échantillons — à des instants
// indépendants de la taille des blocs. Aucune allocation après prepare().

#include "DSP/BiquadDesign.h"
#include "DSP/DspBasics.h"

#include <algorithm>
#include <array>

namespace prisma::dsp
{
    struct BandSettings
    {
        bool used = false;      // bande présente
        bool enabled = true;    // bande active (fondu de 20 ms)
        Placement placement = Placement::Stereo;
        BandParams params;

        bool operator== (const BandSettings&) const = default;
    };

    class EqEngine
    {
    public:
        static constexpr int kNumBands = 24;
        static constexpr int kUpdateInterval = 32;
        static constexpr int kAutoGainPoints = 40;

        void prepare (double sampleRate, int numChannels) noexcept;
        void reset() noexcept;

        void setBand (int index, const BandSettings&) noexcept;
        void setGainScale (double scale) noexcept;
        const BandSettings& getBand (int index) const noexcept { return bands[(size_t) index].target; }

        /// Compensation (dB) estimée à partir de la courbe cible : moyenne de puissance
        /// en pondération rose (uniforme en log-fréquence) de 20 Hz à 20 kHz.
        double getAutoGainDb() noexcept;

        /// Traite en place ; r peut être nul (mono).
        template <typename T>
        void process (T* l, T* r, int numSamples) noexcept
        {
            int pos = 0;
            while (pos < numSamples)
            {
                if (untilUpdate == 0) { updateCoefficients(); untilUpdate = kUpdateInterval; }
                const int len = std::min (numSamples - pos, untilUpdate);
                double x[2][kUpdateInterval];
                for (int i = 0; i < len; ++i) x[0][i] = (double) l[pos + i];
                if (r != nullptr) for (int i = 0; i < len; ++i) x[1][i] = (double) r[pos + i];
                processChunk (x[0], r != nullptr ? x[1] : nullptr, len);
                for (int i = 0; i < len; ++i) l[pos + i] = (T) x[0][i];
                if (r != nullptr) for (int i = 0; i < len; ++i) r[pos + i] = (T) x[1][i];
                untilUpdate -= len;
                pos += len;
            }
        }

    private:
        struct Band
        {
            BandSettings target;
            bool fresh = true;              // prochaine mise à jour : sauter le lissage
            double logF = 0, gainDb = 0, logQ = 0;
            BandParams designed;            // derniers paramètres conçus
            bool hasDesign = false;
            DigitalDesign design;           // coefficients visés à la fin du segment
            std::array<Biquad, kMaxBiquads> cur {}, delta {}; // coefficients courants, pas par échantillon
            int rampLeft = 0;               // échantillons d'interpolation restants
            double state[2][kMaxBiquads][2] {};
            int fade = 0;                   // 0 = sec, fadeLength = bande pleinement active (entier : exact)
            int fadeDir = 0;
            bool autoDirty = true;
            std::array<double, kAutoGainPoints> power {};   // |H|² analogique sur la grille du gain auto
        };

        void updateCoefficients() noexcept;
        void processChunk (double* l, double* r, int len) noexcept;
        void runBand (Band&, int channel, double* x, int len) noexcept;
        void advanceBand (Band&, int len) noexcept;

        std::array<Band, kNumBands> bands {};
        double fs = 48000.0;
        int channels = 2;
        int untilUpdate = 0;
        double smoothCoeff = 1.0;   // coefficient du lisseur, pour un pas de kUpdateInterval échantillons
        int fadeLength = 960;
        double gainScale = 1.0;
        bool gainScaleChanged = true;
        double autoGainDb = 0.0;
        bool autoGainDirty = true;
    };
}
