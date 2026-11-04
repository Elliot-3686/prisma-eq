#pragma once

// Numérisation « matched » des sections analogiques (M. Vicanek, « Matched
// Second Order Digital Filters », 2016) :
//  - pôles par invariance impulsionnelle (pas de cramping) ;
//  - zéros obtenus en égalant |H|² analogique à DC, à ω0 et à Nyquist
//    (gain de Nyquist prescrit, comme chez Orfanidis), solution à phase minimale.
// Les sections du 1er ordre sont appariées deux à deux en biquads.

#include "DSP/AnalogPrototype.h"

namespace prisma::dsp
{
    struct Biquad
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        std::complex<double> response (double omega) const noexcept; // omega en rad/échantillon
    };

    constexpr int kMaxBiquads = 16;

    struct DigitalDesign
    {
        std::array<Biquad, kMaxBiquads> s {};
        int count = 0;

        std::complex<double> response (double hz, double sampleRate) const noexcept;
        double magnitudeDb (double hz, double sampleRate) const noexcept;
    };

    Biquad matchSection (const AnalogSection&, double sampleRate) noexcept;
    DigitalDesign digitize (const AnalogDesign&, double sampleRate) noexcept;

    /// Raccourci : paramètres bruts → biquads (applique effectiveParams).
    DigitalDesign designBand (const BandParams&, double sampleRate, double gainScale = 1.0) noexcept;
}
