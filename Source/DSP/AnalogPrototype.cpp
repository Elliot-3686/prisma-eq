#include "DSP/AnalogPrototype.h"
#include "DSP/DspBasics.h"
#include "DSP/Tuning.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace prisma::dsp
{
    namespace
    {
        constexpr double kPi = std::numbers::pi;
        constexpr double kButterworthQ = 0.70710678118654752;

        int shelfOrder (double slope) noexcept { return std::clamp ((int) std::lround (slope / 6.0), 1, 16); }

        /// Shelf Butterworth d'ordre N : pôles sur un cercle de rayon b·wc, zéros de rayon a·wc,
        /// a = G^(1/2N), b = 1/a → gain √G exactement à wc. Le Q agit sur la section la moins amortie.
        void addShelf (AnalogDesign& d, double wc, double gain, int order, double q, bool high) noexcept
        {
            const double a = std::pow (gain, 0.5 / order);
            const double r = a * a; // G^(1/N), rapport zéro/pôle
            for (int k = 1; k <= order / 2; ++k)
            {
                double c = std::sin ((2 * k - 1) * kPi / (2.0 * order));
                if (k == 1) c *= kButterworthQ / q;
                AnalogSection s;
                s.d1 = 2.0 * c;
                s.d2 = 1.0;
                s.leastSquares = true;
                if (high) { s.w = a * wc; s.n0 = 1.0;   s.n1 = 2.0 * c * r; s.n2 = r * r; }
                else      { s.w = wc / a; s.n0 = r * r; s.n1 = 2.0 * c * r; s.n2 = 1.0; }
                d.add (s);
            }
            if (order % 2 == 1)
            {
                AnalogSection s;
                s.d1 = 1.0;
                if (high) { s.w = a * wc; s.n0 = 1.0; s.n1 = r; }
                else      { s.w = wc / a; s.n0 = r;   s.n1 = 1.0; }
                d.add (s);
            }
        }

        /// Butterworth d'ordre n ; le Q utilisateur s'applique à la section résonante (la moins amortie).
        void addButterworth (AnalogDesign& d, double wc, int order, double q, bool highPass) noexcept
        {
            for (int k = 1; k <= order / 2; ++k)
            {
                double c = std::sin ((2 * k - 1) * kPi / (2.0 * order));
                if (k == 1) c *= kButterworthQ / q;
                AnalogSection s;
                s.w = wc;
                s.d1 = 2.0 * c;
                s.d2 = 1.0;
                if (highPass) { s.n0 = 0.0; s.n2 = 1.0; }
                else          { s.n0 = 1.0; }
                d.add (s);
            }
            if (order % 2 == 1)
            {
                AnalogSection s;
                s.w = wc;
                s.d1 = 1.0;
                if (highPass) { s.n0 = 0.0; s.n1 = 1.0; }
                else          { s.n0 = 1.0; }
                d.add (s);
            }
        }

        /// Partie fractionnaire r ∈ ]0, 1[ de la pente : une paire pôle/zéro du 1er ordre par octave
        /// au-delà de fc (Oustaloup récursif), chacune apportant 6·r dB.
        void addFractional (AnalogDesign& d, double wc, double r, bool highPass) noexcept
        {
            if (r < 1.0e-6) return;
            for (int i = 0; i < tuning::kFractionalOctaves; ++i)
            {
                AnalogSection s;
                s.d1 = 1.0;
                if (highPass)
                {
                    const double p = wc * std::exp2 (-i), z = p * std::exp2 (-r);
                    s.w = p; s.n0 = z / p; s.n1 = 1.0;   // (s + z) / (s + p)
                }
                else
                {
                    const double p = wc * std::exp2 (i), z = p * std::exp2 (r);
                    s.w = p; s.n0 = 1.0; s.n1 = p / z;   // (1 + s/z) / (1 + s/p)
                }
                d.add (s);
            }
        }
    }

    std::complex<double> AnalogSection::response (double omega) const noexcept
    {
        const std::complex<double> p (0.0, omega / w);
        return (n0 + p * (n1 + p * n2)) / (1.0 + p * (d1 + p * d2));
    }

    std::complex<double> AnalogDesign::response (double hz) const noexcept
    {
        const double omega = 2.0 * kPi * hz;
        std::complex<double> h (gain, 0.0);
        for (int i = 0; i < count; ++i)
            h *= s[(size_t) i].response (omega);
        return h;
    }

    bool shapeHasGain (Shape s) noexcept
    {
        return s == Shape::Bell || s == Shape::LowShelf || s == Shape::HighShelf || s == Shape::TiltShelf || s == Shape::FlatTilt;
    }

    bool shapeHasSlope (Shape s) noexcept
    {
        return s == Shape::LowShelf || s == Shape::HighShelf || s == Shape::TiltShelf || s == Shape::LowCut || s == Shape::HighCut;
    }

    double effectiveSlope (Shape shape, double slope, bool brickwall) noexcept
    {
        switch (shape)
        {
            case Shape::LowCut:
            case Shape::HighCut:   return brickwall ? 96.0 : std::clamp (slope, 6.0, 96.0);
            case Shape::LowShelf:
            case Shape::HighShelf:
            case Shape::TiltShelf: return 6.0 * shelfOrder (slope);
            default:               return 12.0;
        }
    }

    BandParams effectiveParams (const BandParams& in, double gainScale) noexcept
    {
        BandParams p = in;
        p.freq = std::clamp (p.freq, 1.0, 1.0e6);
        p.q = std::clamp (p.q, 0.01, 100.0);
        p.gainDb = shapeHasGain (p.shape) ? p.gainDb * gainScale : 0.0;
        p.slope = effectiveSlope (p.shape, p.slope, p.brickwall);
        p.brickwall = false;
        if (p.shape == Shape::Bell && p.gainQ)
            p.q *= std::pow ((std::abs (p.gainDb) + tuning::kGainQOffsetDb) / (tuning::kGainQRefDb + tuning::kGainQOffsetDb),
                             tuning::kGainQExponent);
        p.gainQ = false;
        return p;
    }

    AnalogDesign designAnalog (const BandParams& p) noexcept
    {
        AnalogDesign d;
        const double wc = 2.0 * kPi * p.freq;
        const double g = dbToGain (p.gainDb);
        const double q = p.q;

        switch (p.shape)
        {
            case Shape::Bell:
            {
                if (exactlyEqual (p.gainDb, 0.0)) break; // identité exacte
                const double A = std::sqrt (g);
                AnalogSection s;
                s.w = wc; s.n0 = 1.0; s.n1 = A / q; s.n2 = 1.0; s.d1 = 1.0 / (A * q); s.d2 = 1.0;
                d.add (s);
                break;
            }
            case Shape::LowShelf:
            case Shape::HighShelf:
                if (! exactlyEqual (p.gainDb, 0.0))
                    addShelf (d, wc, g, shelfOrder (p.slope), q, p.shape == Shape::HighShelf);
                break;
            case Shape::TiltShelf:
                if (! exactlyEqual (p.gainDb, 0.0))
                {
                    addShelf (d, wc, g, shelfOrder (p.slope), q, true);
                    d.gain = 1.0 / std::sqrt (g); // −g/2 dB en bas, +g/2 dB en haut
                }
                break;
            case Shape::LowCut:
            case Shape::HighCut:
            {
                const bool hp = p.shape == Shape::LowCut;
                const double units = p.slope / 6.0;
                int order = (int) std::floor (units + 1.0e-9);
                double frac = units - order;
                if (order >= 16) { order = 16; frac = 0.0; }
                addButterworth (d, wc, order, q, hp);
                addFractional (d, wc, frac, hp);
                break;
            }
            case Shape::Notch:
            {
                AnalogSection s;
                s.w = wc; s.n0 = 1.0; s.n2 = 1.0; s.d1 = 1.0 / q; s.d2 = 1.0;
                d.add (s);
                break;
            }
            case Shape::BandPass:
            {
                AnalogSection s;
                s.w = wc; s.n0 = 0.0; s.n1 = 1.0 / q; s.d1 = 1.0 / q; s.d2 = 1.0;
                d.add (s);
                break;
            }
            case Shape::AllPass:
            {
                AnalogSection s;
                s.w = wc; s.n0 = 1.0; s.n1 = -1.0 / q; s.n2 = 1.0; s.d1 = 1.0 / q; s.d2 = 1.0; s.allPass = true;
                d.add (s);
                break;
            }
            case Shape::FlatTilt:
            {
                if (exactlyEqual (p.gainDb, 0.0)) break;
                // Pente constante : le gain est l'écart total entre 20 Hz et 20 kHz (log2(1000) octaves),
                // 0 dB à la fréquence pivot. Une paire zéro/pôle par octave (voir Tuning.h).
                const double perOctaveDb = p.gainDb / std::log2 (1000.0);
                const double ratio = dbToGain (perOctaveDb);
                for (int i = 0; i < tuning::kFlatTiltPairs; ++i)
                {
                    const double z = 2.0 * kPi * tuning::kFlatTiltLowHz * std::exp2 (i);
                    const double pole = z * ratio;
                    AnalogSection s;
                    s.w = pole; s.n0 = 1.0; s.n1 = pole / z; s.d1 = 1.0;   // (1 + s/z) / (1 + s/p)
                    d.add (s);
                }
                d.gain = 1.0 / std::abs (d.response (p.freq));
                break;
            }
        }
        return d;
    }
}
