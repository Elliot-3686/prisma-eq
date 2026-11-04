#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "DSP/DspBasics.h"

using namespace prisma::dsp;
using Catch::Matchers::WithinAbs;

TEST_CASE ("dB conversions round-trip", "[basics]")
{
    for (double db : { -60.0, -6.0206, 0.0, 6.0206, 30.0 })
        REQUIRE_THAT (gainToDb (dbToGain (db)), WithinAbs (db, 1e-9));
    REQUIRE_THAT (dbToGain (-6.0206), WithinAbs (0.5, 1e-4));
}

TEST_CASE ("Smoother converges without overshoot", "[basics]")
{
    OnePoleSmoother s;
    s.prepare (48000.0, 0.02);
    s.reset (0.0);
    s.setTarget (1.0);
    double prev = 0.0;
    for (int i = 0; i < 48000; ++i)
    {
        const double v = s.next();
        REQUIRE (v >= prev);
        REQUIRE (v <= 1.0);
        prev = v;
    }
    REQUIRE_THAT (prev, WithinAbs (1.0, 1e-9));
}

TEST_CASE ("Linear ramp reaches its target in exactly the requested time", "[basics]")
{
    LinearRamp r;
    r.prepare (48000.0, 0.02);
    r.reset (0.0);
    r.setTarget (1.0);
    int n = 0;
    while (r.isRamping()) { r.next(); ++n; }
    REQUIRE (n == 960);
    REQUIRE (r.getCurrent() == 1.0);
}

TEST_CASE ("Balance pan is unity at centre", "[basics]")
{
    double l = 0, r = 0;
    balanceGains (0.0, l, r);
    REQUIRE (l == 1.0);
    REQUIRE (r == 1.0);
    balanceGains (1.0, l, r);
    REQUIRE_THAT (l, WithinAbs (0.0, 1e-12));
    REQUIRE (r == 1.0);
}
