#include "DSP/BiquadDesign.h"
#include "DSP/DspBasics.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace prisma::dsp
{
    namespace
    {
        constexpr double kPi = std::numbers::pi;

        double mag2 (const AnalogSection& a, double omega) noexcept { return std::norm (a.response (omega)); }

        /// 1er ordre : pôle par invariance impulsionnelle, gain DC exact ; le gain de Nyquist est
        /// choisi aux moindres carrés de l'erreur relative sur |H|² (±4 octaves autour de la section),
        /// ce qui évite l'écart à mi-bande d'un simple appariement DC/Nyquist.
        Biquad matchFirstOrder (const AnalogSection& a, double fs) noexcept
        {
            const double w0 = a.w / fs;
            const double p = std::exp (-w0);
            Biquad b;
            b.a1 = -p;
            const double A0 = (1.0 - p) * (1.0 - p), A1 = (1.0 + p) * (1.0 + p);
            const double B0 = A0 * mag2 (a, 0.0);
            double B1 = A1 * mag2 (a, kPi * fs);
            const double wc = std::min (w0, 0.9 * kPi);
            double num = 0.0, den = 0.0;
            for (int k = 0; k <= 24; ++k)
            {
                const double w = wc * std::exp2 ((k - 12) / 3.0);
                if (w >= 0.999 * kPi || w < 1.0e-6) continue;
                const double phi1 = std::sin (0.5 * w) * std::sin (0.5 * w), phi0 = 1.0 - phi1;
                const double t = mag2 (a, w * fs) * (A0 * phi0 + A1 * phi1);
                if (t <= 0.0) continue;
                const double c = (B0 * phi0 - t) / t, d = phi1 / t;
                num += c * d;
                den += d * d;
            }
            if (den > 0.0) B1 = std::max (0.0, -num / den);
            const double sum = std::sqrt (B0), diff = std::sqrt (B1); // b0+b1, b0−b1
            b.b0 = 0.5 * (sum + diff);
            b.b1 = 0.5 * (sum - diff);
            return b;
        }

        Biquad matchSecondOrder (const AnalogSection& a, double fs) noexcept
        {
            const double w0 = a.w / std::sqrt (a.d2) / fs;          // rad/échantillon
            const double zeta = 0.5 * a.d1 / std::sqrt (a.d2);
            const double wp = std::min (w0, 0.98 * kPi);             // au-delà de Nyquist : pôles bornés

            Biquad b;
            const double r = std::exp (-zeta * wp);
            b.a1 = zeta < 1.0 ? -2.0 * r * std::cos (wp * std::sqrt (1.0 - zeta * zeta))
                              : -2.0 * r * std::cosh (wp * std::sqrt (zeta * zeta - 1.0));
            b.a2 = std::exp (-2.0 * zeta * wp);

            if (a.allPass)
            {
                b.b0 = b.a2; b.b1 = b.a1; b.b2 = 1.0;
                return b;
            }

            const double A0 = (1.0 + b.a1 + b.a2) * (1.0 + b.a1 + b.a2);
            const double A1 = (1.0 - b.a1 + b.a2) * (1.0 - b.a1 + b.a2);
            const double A2 = -4.0 * b.a2;
            auto phis = [] (double w, double& p0, double& p1, double& p2)
            {
                const double s2 = std::sin (0.5 * w) * std::sin (0.5 * w);
                p0 = 1.0 - s2; p1 = s2; p2 = 4.0 * p0 * p1;
            };
            auto target = [&] (double w) // |Ha|² · |A|² à la pulsation numérique w
            {
                double p0, p1, p2;
                phis (w, p0, p1, p2);
                return mag2 (a, w * fs) * (A0 * p0 + A1 * p1 + A2 * p2);
            };

            const double hn2 = mag2 (a, kPi * fs);
            const double B1 = A1 * hn2;

            // Zéros réels à DC (coupe-bas) : double zéro imposé (Vicanek), gain de Nyquist exact.
            if (a.n0 <= 0.0 && a.n1 <= 0.0)
            {
                const double k = 0.25 * std::sqrt (B1);
                b.b0 = k; b.b1 = -2.0 * k; b.b2 = k;
                return b;
            }

            // Zéros sur l'axe imaginaire (encoche) : zéros sur le cercle unité à la même fréquence, gain DC exact.
            if (a.n1 <= 0.0 && a.n0 > 0.0 && a.n2 > 0.0)
            {
                const double wz = a.w * std::sqrt (a.n0 / a.n2) / fs;
                if (wz < 0.999 * kPi)
                {
                    const double c = -2.0 * std::cos (wz);
                    const double k = (1.0 + b.a1 + b.a2) * std::sqrt (mag2 (a, 0.0)) / (2.0 + c);
                    b.b0 = k; b.b1 = c * k; b.b2 = k;
                    return b;
                }
            }

            const double B0 = A0 * mag2 (a, 0.0);
            double B2;
            if (a.leastSquares)
            {
                // DC et Nyquist exacts ; B2 au sens des moindres carrés de l'erreur relative sur |H|²,
                // sur ±4 octaves autour de la fréquence de la section.
                const double wc = std::min (w0, 0.9 * kPi);
                double num = 0.0, den = 0.0;
                for (int k = 0; k <= 24; ++k)
                {
                    const double w = wc * std::exp2 ((k - 12) / 3.0);
                    if (w >= 0.999 * kPi || w < 1.0e-6) continue;
                    double p0, p1, p2;
                    phis (w, p0, p1, p2);
                    const double t = target (w);
                    if (t <= 0.0) continue;
                    const double c = (B0 * p0 + B1 * p1 - t) / t, d = p2 / t;
                    num += c * d;
                    den += d * d;
                }
                B2 = den > 0.0 ? -num / den : A2;
            }
            else
            {
                // Vicanek : égalité exacte à ω0 (bornée à 0,9·π).
                const double wm = std::min (w0, 0.9 * kPi);
                double p0, p1, p2;
                phis (wm, p0, p1, p2);
                B2 = (target (wm) - B0 * p0 - B1 * p1) / p2;
            }

            const double sB0 = std::sqrt (std::max (B0, 0.0)), sB1 = std::sqrt (std::max (B1, 0.0));
            const double W = 0.5 * (sB0 + sB1);
            b.b0 = 0.5 * (W + std::sqrt (std::max (W * W + B2, 0.0)));
            b.b1 = 0.5 * (sB0 - sB1);
            b.b2 = b.b0 > 0.0 ? -B2 / (4.0 * b.b0) : 0.0;
            return b;
        }
    }

    std::complex<double> Biquad::response (double omega) const noexcept
    {
        const std::complex<double> z1 = std::polar (1.0, -omega);
        return (b0 + z1 * (b1 + z1 * b2)) / (1.0 + z1 * (a1 + z1 * a2));
    }

    std::complex<double> DigitalDesign::response (double hz, double sampleRate) const noexcept
    {
        const double omega = 2.0 * kPi * hz / sampleRate;
        std::complex<double> h (1.0, 0.0);
        for (int i = 0; i < count; ++i)
            h *= s[(size_t) i].response (omega);
        return h;
    }

    double DigitalDesign::magnitudeDb (double hz, double sampleRate) const noexcept
    {
        return gainToDb (std::abs (response (hz, sampleRate)), -400.0);
    }

    Biquad matchSection (const AnalogSection& a, double fs) noexcept
    {
        return a.isSecondOrder() ? matchSecondOrder (a, fs) : matchFirstOrder (a, fs);
    }

    DigitalDesign digitize (const AnalogDesign& a, double fs) noexcept
    {
        DigitalDesign d;
        bool pendingFirst = false;
        Biquad first;
        for (int i = 0; i < a.count; ++i)
        {
            const auto& sec = a.s[(size_t) i];
            const Biquad b = matchSection (sec, fs);
            if (sec.isSecondOrder())
            {
                if (d.count < kMaxBiquads) d.s[(size_t) d.count++] = b;
            }
            else if (! pendingFirst)
            {
                first = b;
                pendingFirst = true;
            }
            else
            {
                // (b0 + b1 z⁻¹)(c0 + c1 z⁻¹) / (1 + a1 z⁻¹)(1 + c a1 z⁻¹)
                Biquad m;
                m.b0 = first.b0 * b.b0;
                m.b1 = first.b0 * b.b1 + first.b1 * b.b0;
                m.b2 = first.b1 * b.b1;
                m.a1 = first.a1 + b.a1;
                m.a2 = first.a1 * b.a1;
                if (d.count < kMaxBiquads) d.s[(size_t) d.count++] = m;
                pendingFirst = false;
            }
        }
        if (pendingFirst && d.count < kMaxBiquads)
            d.s[(size_t) d.count++] = first;

        if (! exactlyEqual (a.gain, 1.0))
        {
            if (d.count == 0) d.count = 1;
            auto& b = d.s[0];
            b.b0 *= a.gain; b.b1 *= a.gain; b.b2 *= a.gain;
        }
        return d;
    }

    DigitalDesign designBand (const BandParams& p, double sampleRate, double gainScale) noexcept
    {
        return digitize (designAnalog (effectiveParams (p, gainScale)), sampleRate);
    }
}
