#pragma once

// Plain C++20 DSP helpers shared by the engine and the plugin. No JUCE here:
// everything under Source/DSP must stay testable without the framework.

#include <algorithm>
#include <cmath>
#include <numbers>

namespace prisma::dsp
{
    const char* versionString() noexcept;

    /// Exact comparison without -Wfloat-equal noise (used where exactness is intended).
    inline bool exactlyEqual (double a, double b) noexcept { return ! (a < b) && ! (b < a); }

    inline double dbToGain (double db) noexcept         { return std::pow (10.0, db / 20.0); }
    inline double gainToDb (double g, double floorDb = -200.0) noexcept
    {
        return g > 0.0 ? std::max (20.0 * std::log10 (g), floorDb) : floorDb;
    }

    /// One-pole exponential smoother, evaluated per sample. Allocation-free.
    /// `setTime` may be called from the audio thread (prepare-time constants only).
    class OnePoleSmoother
    {
    public:
        void prepare (double sampleRate, double timeSeconds) noexcept
        {
            coeff = timeSeconds <= 0.0 ? 1.0 : 1.0 - std::exp (-1.0 / (timeSeconds * sampleRate));
        }

        void reset (double value) noexcept   { current = target = value; }
        void setTarget (double value) noexcept { target = value; }

        double next() noexcept
        {
            current += (target - current) * coeff;
            if (std::abs (target - current) < 1.0e-12) current = target;
            return current;
        }

        double getCurrent() const noexcept { return current; }
        double getTarget() const noexcept  { return target; }
        bool isSmoothing() const noexcept  { return ! exactlyEqual (current, target); }

    private:
        double coeff = 1.0, current = 0.0, target = 0.0;
    };

    /// Linear ramp over a fixed duration (soft bypass, clean crossfades).
    class LinearRamp
    {
    public:
        void prepare (double sampleRate, double timeSeconds) noexcept
        {
            length = std::max (1, static_cast<int> (std::lround (sampleRate * timeSeconds)));
        }

        void reset (double value) noexcept { current = target = value; remaining = 0; }

        void setTarget (double value) noexcept
        {
            if (exactlyEqual (value, target)) return;
            target = value;
            remaining = length;
            step = (target - current) / static_cast<double> (length);
        }

        double next() noexcept
        {
            if (remaining > 0)
            {
                current += step;
                if (--remaining == 0) current = target;
            }
            return current;
        }

        bool isRamping() const noexcept { return remaining > 0; }
        double getCurrent() const noexcept { return current; }

    private:
        int length = 1, remaining = 0;
        double current = 0.0, target = 0.0, step = 0.0;
    };

    /// Equal-power-ish balance used by the output pan (L/R or M/S).
    /// pan ∈ [−1, 1]; at 0 both gains are 1 (unity, no level change).
    inline void balanceGains (double pan, double& left, double& right) noexcept
    {
        pan = std::clamp (pan, -1.0, 1.0);
        left  = pan <= 0.0 ? 1.0 : std::cos (pan * std::numbers::pi * 0.5);
        right = pan >= 0.0 ? 1.0 : std::cos (-pan * std::numbers::pi * 0.5);
    }
}
