#include "DSP/EqEngine.h"
#include "DSP/Tuning.h"

#include <cmath>

namespace prisma::dsp
{
    void EqEngine::prepare (double sampleRate, int numChannels) noexcept
    {
        fs = sampleRate;
        channels = std::clamp (numChannels, 1, 2);
        // Lisseur exponentiel par échantillon, évalué exactement tous les kUpdateInterval échantillons.
        const double perSample = 1.0 - std::exp (-1.0 / (tuning::kSmoothingSeconds * fs));
        smoothCoeff = 1.0 - std::pow (1.0 - perSample, (double) kUpdateInterval);
        fadeLength = std::max (1, (int) std::lround (tuning::kBandFadeSeconds * fs));
        reset();
    }

    void EqEngine::reset() noexcept
    {
        for (auto& b : bands)
        {
            b.fresh = true;
            b.hasDesign = false;
            b.fade = (b.target.used && b.target.enabled) ? fadeLength : 0;
            b.fadeDir = 0;
            b.rampLeft = 0;
            b.autoDirty = true;
            for (auto& ch : b.state) for (auto& s : ch) s[0] = s[1] = 0.0;
        }
        untilUpdate = 0;
        autoGainDirty = true;
        gainScaleChanged = true;
    }

    void EqEngine::setBand (int index, const BandSettings& s) noexcept
    {
        auto& b = bands[(size_t) index];
        if (s == b.target) return;
        b.target = s;
        b.autoDirty = true;
        autoGainDirty = true;
    }

    void EqEngine::setGainScale (double scale) noexcept
    {
        if (exactlyEqual (scale, gainScale)) return;
        gainScale = scale;
        gainScaleChanged = true;
        autoGainDirty = true;
        for (auto& b : bands) b.autoDirty = true;
    }

    void EqEngine::updateCoefficients() noexcept
    {
        for (auto& b : bands)
        {
            const bool on = b.target.used && b.target.enabled;
            b.fadeDir = on ? (b.fade < fadeLength ? 1 : 0) : (b.fade > 0 ? -1 : 0);
            if (b.fade == 0 && ! on)
            {
                b.fresh = true;
                b.hasDesign = false;
                continue;
            }

            const auto& t = b.target.params;
            const double tf = std::log (std::max (t.freq, 1.0)), tg = t.gainDb, tq = std::log (std::max (t.q, 1.0e-3));
            if (b.fresh)
            {
                b.logF = tf; b.gainDb = tg; b.logQ = tq;
                b.fresh = false;
                for (auto& ch : b.state) for (auto& s : ch) s[0] = s[1] = 0.0;
            }
            else
            {
                auto step = [this] (double& v, double target, double eps)
                {
                    v += (target - v) * smoothCoeff;
                    if (std::abs (target - v) < eps) v = target;
                };
                step (b.logF, tf, 1.0e-7);
                step (b.gainDb, tg, 1.0e-6);
                step (b.logQ, tq, 1.0e-7);
            }

            BandParams cur = t;
            cur.freq = std::exp (b.logF);
            cur.gainDb = b.gainDb;
            cur.q = std::exp (b.logQ);

            if (! b.hasDesign || ! (cur == b.designed) || gainScaleChanged)
            {
                const int oldCount = b.hasDesign ? b.design.count : 0;
                const bool topologyChanged = b.hasDesign && (cur.shape != b.designed.shape
                                                             || ! exactlyEqual (cur.slope, b.designed.slope)
                                                             || cur.brickwall != b.designed.brickwall);
                b.design = designBand (cur, fs, gainScale);
                if (topologyChanged)
                    for (auto& ch : b.state) for (auto& s : ch) s[0] = s[1] = 0.0;
                else
                    for (int k = oldCount; k < b.design.count; ++k)
                        for (auto& ch : b.state) ch[k][0] = ch[k][1] = 0.0;
                // Interpolation linéaire des coefficients sur le segment (même topologie uniquement).
                if (b.hasDesign && ! topologyChanged && oldCount == b.design.count)
                {
                    for (int k = 0; k < b.design.count; ++k)
                    {
                        const auto& to = b.design.s[(size_t) k];
                        const auto& c = b.cur[(size_t) k];
                        constexpr double inv = 1.0 / kUpdateInterval;
                        b.delta[(size_t) k] = { (to.b0 - c.b0) * inv, (to.b1 - c.b1) * inv, (to.b2 - c.b2) * inv,
                                                (to.a1 - c.a1) * inv, (to.a2 - c.a2) * inv };
                    }
                    b.rampLeft = kUpdateInterval;
                }
                else
                {
                    b.cur = b.design.s;
                    b.rampLeft = 0;
                }
                b.designed = cur;
                b.hasDesign = true;
            }
        }
        gainScaleChanged = false;
    }

    void EqEngine::runBand (Band& b, int channel, double* x, int len) noexcept
    {
        auto& st = b.state[channel];
        auto coeffs = b.cur;             // copie locale : les deux canaux partent du même point
        int ramp = b.rampLeft;
        const int n = b.design.count;
        const bool fading = b.fadeDir != 0 || b.fade < fadeLength;
        const double invFade = 1.0 / fadeLength;
        for (int i = 0; i < len; ++i)
        {
            if (ramp > 0)
            {
                for (int k = 0; k < n; ++k)
                {
                    auto& c = coeffs[(size_t) k];
                    const auto& d = b.delta[(size_t) k];
                    c.b0 += d.b0; c.b1 += d.b1; c.b2 += d.b2; c.a1 += d.a1; c.a2 += d.a2;
                }
                if (--ramp == 0)
                    for (int k = 0; k < n; ++k) coeffs[(size_t) k] = b.design.s[(size_t) k];
            }
            const double in = x[i];
            double y = in;
            for (int k = 0; k < n; ++k)
            {
                const auto& c = coeffs[(size_t) k];
                const double out = c.b0 * y + st[k][0];
                st[k][0] = c.b1 * y - c.a1 * out + st[k][1];
                st[k][1] = c.b2 * y - c.a2 * out;
                y = out;
            }
            if (fading)
            {
                const int f = std::clamp (b.fade + b.fadeDir * (i + 1), 0, fadeLength);
                y = in + (f * invFade) * (y - in);
            }
            x[i] = y;
        }
    }

    void EqEngine::advanceBand (Band& b, int len) noexcept
    {
        const int n = b.design.count;
        const int steps = std::min (len, b.rampLeft);
        if (steps > 0)
        {
            b.rampLeft -= steps;
            if (b.rampLeft == 0)
                b.cur = b.design.s;
            else
                for (int k = 0; k < n; ++k)
                {
                    auto& c = b.cur[(size_t) k];
                    const auto& d = b.delta[(size_t) k];
                    for (int i = 0; i < steps; ++i) { c.b0 += d.b0; c.b1 += d.b1; c.b2 += d.b2; c.a1 += d.a1; c.a2 += d.a2; }
                }
        }
        b.fade = std::clamp (b.fade + b.fadeDir * len, 0, fadeLength);
    }

    void EqEngine::processChunk (double* l, double* r, int len) noexcept
    {
        bool midSide = false;
        auto toMidSide = [&] (bool want)
        {
            if (want == midSide) return;
            for (int i = 0; i < len; ++i)
            {
                const double a = l[i], c = r[i];
                if (want) { l[i] = 0.5 * (a + c); r[i] = 0.5 * (a - c); }
                else      { l[i] = a + c;         r[i] = a - c; }
            }
            midSide = want;
        };

        for (auto& b : bands)
        {
            if (! b.hasDesign) continue;
            const auto place = b.target.placement;

            if (r == nullptr)
            {
                if (place != Placement::Side)
                    runBand (b, 0, l, len);
            }
            else switch (place)
            {
                case Placement::Stereo: runBand (b, 0, l, len); runBand (b, 1, r, len); break;
                case Placement::Left:   toMidSide (false); runBand (b, 0, l, len); break;
                case Placement::Right:  toMidSide (false); runBand (b, 1, r, len); break;
                case Placement::Mid:    toMidSide (true);  runBand (b, 0, l, len); break;
                case Placement::Side:   toMidSide (true);  runBand (b, 1, r, len); break;
            }
            advanceBand (b, len);
        }
        if (r != nullptr) toMidSide (false);
    }

    double EqEngine::getAutoGainDb() noexcept
    {
        if (! autoGainDirty) return autoGainDb;

        std::array<double, kAutoGainPoints> pa, pb;
        pa.fill (1.0);
        pb.fill (1.0);
        for (auto& b : bands)
        {
            const auto& t = b.target;
            if (! (t.used && t.enabled)) continue;
            if (b.autoDirty)
            {
                const auto design = designAnalog (effectiveParams (t.params, gainScale));
                for (int k = 0; k < kAutoGainPoints; ++k)
                    b.power[(size_t) k] = std::norm (design.response (20.0 * std::pow (1000.0, k / (kAutoGainPoints - 1.0))));
                b.autoDirty = false;
            }
            const bool a = t.placement != Placement::Right && t.placement != Placement::Side;
            const bool c = t.placement == Placement::Stereo || t.placement == Placement::Right || t.placement == Placement::Side;
            if (channels == 1 && t.placement == Placement::Side) continue;
            for (int k = 0; k < kAutoGainPoints; ++k)
            {
                if (a || channels == 1) pa[(size_t) k] *= b.power[(size_t) k];
                if (c && channels == 2) pb[(size_t) k] *= b.power[(size_t) k];
            }
        }
        double mean = 0.0;
        for (int k = 0; k < kAutoGainPoints; ++k)
            mean += channels == 2 ? 0.5 * (pa[(size_t) k] + pb[(size_t) k]) : pa[(size_t) k];
        mean /= kAutoGainPoints;
        autoGainDb = -10.0 * std::log10 (std::max (mean, 1.0e-30));
        autoGainDirty = false;
        return autoGainDb;
    }
}
