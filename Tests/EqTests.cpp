#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "DSP/EqEngine.h"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <random>
#include <vector>

using namespace prisma::dsp;

// ─── Compteur d'allocations (remplace operator new dans le binaire de test) ───
namespace { std::atomic<long> gAllocs { 0 }; }
void* operator new (std::size_t n)
{
    gAllocs.fetch_add (1, std::memory_order_relaxed);
    if (void* p = std::malloc (n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }

namespace
{
    constexpr double kRates[] = { 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };
    const char* shapeName (Shape s)
    {
        static const char* n[] = { "Bell", "LowShelf", "LowCut", "HighShelf", "HighCut", "Notch", "BandPass", "TiltShelf", "FlatTilt", "AllPass" };
        return n[(int) s];
    }

    BandParams make (Shape s, double f, double g, double q, double slope = 12.0)
    {
        BandParams p; p.shape = s; p.freq = f; p.gainDb = g; p.q = q; p.slope = slope; return p;
    }

    double rms (const std::vector<double>& x, size_t from)
    {
        double e = 0; for (size_t i = from; i < x.size(); ++i) e += x[i] * x[i];
        return std::sqrt (e / double (x.size() - from));
    }

    BandSettings band (const BandParams& p, Placement pl = Placement::Stereo)
    {
        BandSettings s; s.used = true; s.enabled = true; s.placement = pl; s.params = p; return s;
    }
}

TEST_CASE ("Magnitude numérique contre prototype analogique", "[eq][mesure]")
{
    // Erreur maximale (dB) en deux zones de mesure : f < fs/4, et fs/4 → 0,95·fs/2 (proche de Nyquist).
    // Bandes « basses » (f0 ≤ fs/16) et « hautes » (f0 jusqu'à 16 kHz) séparées : près de Nyquist, un
    // biquad ne peut pas reproduire tout le prototype analogique (zéros/pôles repliés), l'écart est documenté.
    // Mesuré là où |H| > −60 dB. Gains −12/+6/+18 dB, Q 0,5/0,71/3, pentes 12/30 dB/oct.
    for (int si = 0; si < 10; ++si)
    {
        const auto shape = (Shape) si;
        if (shape == Shape::AllPass) continue; // magnitude unité : testée à part
        for (double fs : kRates)
        {
            double err[2][2] = {};   // [bande basse/haute][zone basse/haute]
            for (double f0 : { 40.0, 300.0, 1000.0, 2000.0, 5000.0, 8000.0, 12000.0, 16000.0 })
                for (double g : { -12.0, 6.0, 18.0 })
                    for (double q : { 0.5, 0.71, 3.0 })
                        for (double slope : { 12.0, 30.0 })
                        {
                            const auto p = effectiveParams (make (shape, f0, g, q, slope));
                            const auto ana = designAnalog (p);
                            const auto dig = digitize (ana, fs);
                            for (int k = 0; k <= 400; ++k)
                            {
                                const double f = 20.0 * std::pow (0.95 * fs / 2 / 20.0, k / 400.0);
                                const double ha = gainToDb (std::abs (ana.response (f)), -400);
                                if (ha < -60.0) continue;
                                auto& e = err[f0 > fs / 16 ? 1 : 0][f < fs / 4 ? 0 : 1];
                                e = std::max (e, std::abs (dig.magnitudeDb (f, fs) - ha));
                            }
                        }
            std::printf ("MESURE magnitude %-9s fs=%6.0f | f0<=fs/16 : %.3f dB (<fs/4), %.3f dB (->0.95Ny) | f0>fs/16 : %.3f / %.3f dB\n",
                         shapeName (shape), fs, err[0][0], err[0][1], err[1][0], err[1][1]);
            CHECK (err[0][0] < 0.65);
        }
    }
}

TEST_CASE ("Passe-tout : magnitude unité et phase analogique à basse fréquence", "[eq][mesure]")
{
    for (double fs : kRates)
    {
        const auto dig = designBand (make (Shape::AllPass, 1000.0, 0.0, 0.7), fs);
        double worst = 0;
        for (int k = 0; k <= 200; ++k)
            worst = std::max (worst, std::abs (dig.magnitudeDb (20.0 * std::pow (1000.0, k / 200.0), fs)));
        std::printf ("MESURE passe-tout fs=%6.0f  |H| max écart = %.2e dB\n", fs, worst);
        CHECK (worst < 1e-9);
    }
}

TEST_CASE ("Pentes des coupes de 6 à 96 dB/oct (pas fractionnaires)", "[eq][mesure]")
{
    const double fs = 48000.0;
    double worst = 0;
    for (double s = 6.0; s <= 96.0 + 1e-9; s += 1.5)
    {
        // Coupe-haut à 200 Hz : pente entre 4·fc et 16·fc. Coupe-bas à 5 kHz : entre fc/16 et fc/4.
        const auto hc = designBand (make (Shape::HighCut, 200.0, 0, 0.7071, s), fs);
        const auto lc = designBand (make (Shape::LowCut, 5000.0, 0, 0.7071, s), fs);
        const double sh = -(hc.magnitudeDb (3200.0, fs) - hc.magnitudeDb (800.0, fs)) / 2.0;
        const double sl = (lc.magnitudeDb (1250.0, fs) - lc.magnitudeDb (312.5, fs)) / 2.0;
        worst = std::max ({ worst, std::abs (sh - s), std::abs (sl - s) });
        if (std::fmod (s, 12.0) < 1e-9 || s == 7.5 || s == 52.5)
            std::printf ("MESURE pente consigne %5.1f dB/oct : coupe-haut %6.2f, coupe-bas %6.2f\n", s, sh, sl);
    }
    std::printf ("MESURE pente : erreur max %.3f dB/oct (6 → 96 par pas de 1,5)\n", worst);
    CHECK (worst <= 0.5);
}

TEST_CASE ("Flat Tilt : pente constante de 20 Hz à 20 kHz", "[eq][mesure]")
{
    for (double fs : kRates)
        for (double g : { -30.0, 9.0, 30.0 })
        {
            const auto d = designBand (make (Shape::FlatTilt, 1000.0, g, 1.0), fs);
            const double perOct = g / std::log2 (1000.0);
            double worst = 0;
            for (int k = 0; k <= 200; ++k)
            {
                const double f = 20.0 * std::pow (1000.0, k / 200.0);
                if (f > 0.45 * fs) break;
                worst = std::max (worst, std::abs (d.magnitudeDb (f, fs) - perOct * std::log2 (f / 1000.0)));
            }
            std::printf ("MESURE flat tilt fs=%6.0f gain %+5.1f : écart max à la droite %.3f dB\n", fs, g, worst);
            CHECK (worst < 0.65);
        }
}

TEST_CASE ("Null test : bandes à 0 dB = entrée", "[eq][mesure]")
{
    EqEngine eq;
    eq.prepare (48000.0, 2);
    const Shape shapes[] = { Shape::Bell, Shape::LowShelf, Shape::HighShelf, Shape::TiltShelf, Shape::FlatTilt };
    for (int i = 0; i < 24; ++i)
        eq.setBand (i, band (make (shapes[i % 5], 30.0 * std::pow (1.3, i), 0.0, 0.3 + 0.2 * i, 6.0 * (1 + i % 8)),
                             (Placement) (i % 5)));
    std::mt19937 rng (1);
    std::normal_distribution<double> n (0.0, 0.3);
    std::vector<double> l (48000), r (48000), l0, r0;
    for (size_t i = 0; i < l.size(); ++i) { l[i] = n (rng); r[i] = n (rng); }
    l0 = l; r0 = r;
    eq.process (l.data(), r.data(), (int) l.size());
    double err = 0;
    for (size_t i = 0; i < l.size(); ++i) err = std::max ({ err, std::abs (l[i] - l0[i]), std::abs (r[i] - r0[i]) });
    std::printf ("MESURE null test (24 bandes à 0 dB, L/R/M/S) : écart max %.1f dB\n", gainToDb (err, -400));
    CHECK (gainToDb (err, -400) < -200.0);
}

TEST_CASE ("Balayage rapide fréquence/gain/Q : bruit de modulation", "[eq][mesure]")
{
    // Sinus de 100 Hz ; une cloche balaie 300 Hz → 5 kHz, −12 → +12 dB, Q 0,5 → 4 en 0,5 s.
    // Le bruit de zipper tombe aux multiples de fs/32 : on mesure l'énergie au-delà de 1 kHz
    // (coupe-bas 96 dB/oct) relativement au signal.
    for (double fs : { 44100.0, 96000.0 })
    {
        EqEngine eq;
        eq.prepare (fs, 1);
        const int n = (int) fs;
        std::vector<double> y ((size_t) n);
        auto p = make (Shape::Bell, 300.0, -12.0, 0.5);
        eq.setBand (0, band (p));
        const int block = 64;
        for (int start = 0; start < n; start += block)
        {
            const double t = std::min (1.0, start / (0.5 * fs));
            const double u = std::abs (std::fmod (t * 2.0, 2.0) - 1.0); // aller-retour
            p.freq = 300.0 * std::pow (5000.0 / 300.0, u);
            p.gainDb = -12.0 + 24.0 * u;
            p.q = 0.5 * std::pow (8.0, u);
            eq.setBand (0, band (p));
            const int len = std::min (block, n - start);
            for (int i = 0; i < len; ++i) y[(size_t) (start + i)] = std::sin (2 * M_PI * 100.0 * (start + i) / fs);
            eq.process (y.data() + start, (double*) nullptr, len);
        }
        const double sig = rms (y, (size_t) (fs * 0.1));
        EqEngine filt;
        filt.prepare (fs, 1);
        filt.setBand (0, band (make (Shape::LowCut, 1000.0, 0, 0.7071, 96.0)));
        filt.process (y.data(), (double*) nullptr, n);
        const double noise = rms (y, (size_t) (fs * 0.1));
        const double ratio = gainToDb (noise / sig, -400);
        std::printf ("MESURE zipper fs=%6.0f : bruit de modulation au-delà de 1 kHz = %.1f dB\n", fs, ratio);
        CHECK (ratio < -90.0);
    }
}

TEST_CASE ("Invariance à la taille des blocs (1 → 4096)", "[eq][mesure]")
{
    double worst = 0;
    // Sans changement dépendant du découpage : comparer des tailles différentes.
    auto renderStatic = [] (int bs)
    {
        EqEngine eq;
        eq.prepare (44100.0, 2);
        eq.setBand (0, band (make (Shape::HighCut, 12000.0, 0, 0.7, 52.5)));
        eq.setBand (1, band (make (Shape::Bell, 900.0, 6.0, 2.0), Placement::Mid));
        eq.setBand (2, band (make (Shape::TiltShelf, 1500.0, 4.0, 0.7, 6.0), Placement::Right));
        std::vector<double> l (30000), r (30000);
        for (size_t i = 0; i < l.size(); ++i) { l[i] = std::sin (0.01 * i) ; r[i] = std::cos (0.023 * i); }
        int pos = 0;
        std::mt19937 sizes (11);
        while (pos < (int) l.size())
        {
            const int len = std::min (bs > 0 ? bs : 1 + (int) (sizes() % 4096), (int) l.size() - pos);
            if (pos <= 15000 && pos + len > 15000)
            {
                const int a = 15000 - pos;
                eq.process (l.data() + pos, r.data() + pos, a);
                eq.setBand (1, band (make (Shape::Bell, 3000.0, -6.0, 1.0), Placement::Mid));
                eq.process (l.data() + pos + a, r.data() + pos + a, len - a);
            }
            else
            {
                if (pos == 15000) eq.setBand (1, band (make (Shape::Bell, 3000.0, -6.0, 1.0), Placement::Mid));
                eq.process (l.data() + pos, r.data() + pos, len);
            }
            pos += len;
        }
        return l;
    };
    const auto s0 = renderStatic (4096);
    for (int bs : { 1, 2, 31, 32, 33, 256, 1000, 4096, 0 })
    {
        const auto s = renderStatic (bs);
        double e = 0;
        for (size_t i = 0; i < s.size(); ++i) e = std::max (e, std::abs (s[i] - s0[i]));
        worst = std::max (worst, e);
    }
    std::printf ("MESURE invariance aux blocs (1, 2, 31, 32, 33, 256, 1000, 4096, aléatoire) : écart max %.1e\n", worst);
    CHECK (worst == 0.0);
}

TEST_CASE ("Placement L/R/M/S", "[eq]")
{
    auto run = [] (Placement pl, double inL, double inR, double& outL, double& outR)
    {
        EqEngine eq;
        eq.prepare (48000.0, 2);
        eq.setBand (0, band (make (Shape::Bell, 1000.0, 12.0, 1.0), pl));
        std::vector<double> l (48000), r (48000);
        for (size_t i = 0; i < l.size(); ++i) { const double s = std::sin (2 * M_PI * 1000.0 * i / 48000.0); l[i] = inL * s; r[i] = inR * s; }
        eq.process (l.data(), r.data(), (int) l.size());
        outL = rms (l, 24000) * std::sqrt (2.0);
        outR = rms (r, 24000) * std::sqrt (2.0);
    };
    double l, r;
    const double g = dbToGain (12.0);
    run (Placement::Left, 1, 1, l, r);   CHECK (std::abs (l - g) < 1e-3); CHECK (std::abs (r - 1) < 1e-9);
    run (Placement::Right, 1, 1, l, r);  CHECK (std::abs (l - 1) < 1e-9); CHECK (std::abs (r - g) < 1e-3);
    run (Placement::Mid, 1, 1, l, r);    CHECK (std::abs (l - g) < 1e-3); CHECK (std::abs (r - g) < 1e-3);
    run (Placement::Side, 1, 1, l, r);   CHECK (std::abs (l - 1) < 1e-9); CHECK (std::abs (r - 1) < 1e-9);
    run (Placement::Side, 1, -1, l, r);  CHECK (std::abs (l - g) < 1e-3); CHECK (std::abs (r - g) < 1e-3);
    run (Placement::Stereo, 1, 0.5, l, r); CHECK (std::abs (l - g) < 1e-3); CHECK (std::abs (r - 0.5 * g) < 1e-3);
}

TEST_CASE ("Aucune allocation pendant le traitement", "[eq]")
{
    EqEngine eq;
    eq.prepare (48000.0, 2);
    std::vector<double> l (4096, 0.1), r (4096, -0.1);
    std::vector<float> lf (4096, 0.1f), rf (4096, -0.1f);
    const long before = gAllocs.load();
    for (int i = 0; i < 24; ++i)
        eq.setBand (i, band (make ((Shape) (i % 10), 50.0 * (i + 1), 3.0, 1.0, 6.0 + 3.5 * i), (Placement) (i % 5)));
    for (int bs : { 1, 64, 4096 })
    {
        eq.process (l.data(), r.data(), bs);
        eq.process (lf.data(), rf.data(), bs);
        (void) eq.getAutoGainDb();
        eq.setBand (5, band (make (Shape::Bell, 100.0 * bs, -3.0, 2.0)));
        eq.setGainScale (0.5);
    }
    const long count = gAllocs.load() - before;
    std::printf ("MESURE allocations pendant le traitement : %ld\n", count);
    CHECK (count == 0);
}

TEST_CASE ("Gain automatique : compensation de la courbe", "[eq][mesure]")
{
    EqEngine eq;
    eq.prepare (48000.0, 2);
    CHECK (std::abs (eq.getAutoGainDb()) < 1e-12);
    eq.setBand (0, band (make (Shape::Bell, 1000.0, 6.0, 1.0)));
    const double bell = eq.getAutoGainDb();
    eq.setBand (0, band (make (Shape::LowShelf, 1000.0, 6.0, 0.7071, 12.0)));
    const double shelf = eq.getAutoGainDb();
    eq.setBand (0, band (make (Shape::FlatTilt, 632.5, 10.0, 1.0)));
    const double tilt = eq.getAutoGainDb();
    std::printf ("MESURE gain auto : cloche +6 dB/Q1 → %.2f dB, plateau grave +6 dB à 1 kHz → %.2f dB, flat tilt ±5 dB → %.2f dB\n",
                 bell, shelf, tilt);
    CHECK (bell < -0.3);
    CHECK (bell > -6.0);
    CHECK (shelf < -2.0);
    CHECK (shelf > -6.0);
}
