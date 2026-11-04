#pragma once

// Prototypes analogiques exacts de chaque forme (RBJ Cookbook, Butterworth,
// shelves Butterworth d'ordre élevé, approximation récursive d'Oustaloup).
// La réponse analogique sert de référence aux tests et à la conception
// « matched » des biquads numériques (BiquadDesign.h).

#include <array>
#include <complex>

namespace prisma::dsp
{
    /// Même ordre que params::Shape (ordre figé : ajout en fin de liste uniquement).
    enum class Shape { Bell, LowShelf, LowCut, HighShelf, HighCut, Notch, BandPass, TiltShelf, FlatTilt, AllPass };
    enum class Placement { Stereo, Left, Right, Mid, Side };

    struct BandParams
    {
        Shape shape = Shape::Bell;
        double freq = 1000.0;     // Hz
        double gainDb = 0.0;
        double q = 0.70710678118654752;
        double slope = 12.0;      // dB/oct
        bool brickwall = false;
        bool gainQ = false;       // interaction gain-Q (Bell)

        bool operator== (const BandParams&) const = default;
    };

    /// Section analogique normalisée : H(p) = (n0 + n1 p + n2 p²) / (1 + d1 p + d2 p²), p = s / w.
    struct AnalogSection
    {
        double w = 1.0;                    // rad/s
        double n0 = 1.0, n1 = 0.0, n2 = 0.0;
        double d1 = 0.0, d2 = 0.0;
        bool allPass = false;
        bool leastSquares = false;         // numérisation : B2 aux moindres carrés plutôt qu'égalité à ω0

        bool isSecondOrder() const noexcept { return d2 > 0.0; }
        std::complex<double> response (double omega) const noexcept; // omega en rad/s
    };

    constexpr int kMaxAnalogSections = 32;

    struct AnalogDesign
    {
        std::array<AnalogSection, kMaxAnalogSections> s {};
        int count = 0;
        double gain = 1.0;

        void add (const AnalogSection& x) noexcept { if (count < kMaxAnalogSections) s[(size_t) count++] = x; }
        std::complex<double> response (double hz) const noexcept;
    };

    /// Pente réellement utilisée (minimums par forme, brickwall → 96 dB/oct).
    double effectiveSlope (Shape, double slope, bool brickwall) noexcept;

    /// Paramètres effectifs : échelle de gain, interaction gain-Q, bornes.
    BandParams effectiveParams (const BandParams&, double gainScale = 1.0) noexcept;

    /// Conception analogique (à partir de paramètres effectifs).
    AnalogDesign designAnalog (const BandParams&) noexcept;

    /// Vrai si la forme utilise le paramètre de gain.
    bool shapeHasGain (Shape) noexcept;
    /// Vrai si la forme utilise le paramètre de pente.
    bool shapeHasSlope (Shape) noexcept;
}
