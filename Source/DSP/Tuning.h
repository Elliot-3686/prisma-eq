#pragma once

// Constantes réglables à l'oreille. Tout ce qui relève du goût plutôt que de
// la mathématique se trouve ici, et nulle part ailleurs.

namespace prisma::dsp::tuning
{
    // ─── Interaction gain-Q (Bell) ─────────────────────────────────────────
    // Q effectif = Q · ((|gain| + offset) / (ref + offset)) ^ exponent
    // → Q inchangé à ±ref dB, plus large à faible gain, plus serré au-delà.
    inline constexpr double kGainQRefDb    = 12.0;
    inline constexpr double kGainQOffsetDb = 3.0;
    inline constexpr double kGainQExponent = 0.5;

    // ─── Lissage anti-zipper ────────────────────────────────────────────────
    inline constexpr double kSmoothingSeconds  = 0.020; // fréquence (log), gain (dB), Q (log)
    inline constexpr double kBandFadeSeconds   = 0.020; // activation / désactivation d'une bande
    inline constexpr double kAutoGainSeconds   = 0.100; // gain automatique

    // ─── Pentes fractionnaires (approximation récursive d'Oustaloup) ───────
    inline constexpr int    kFractionalOctaves = 10;    // une paire pôle/zéro par octave au-delà de fc

    // ─── Flat Tilt ─────────────────────────────────────────────────────────
    inline constexpr double kFlatTiltLowHz     = 2.5;   // première paire pôle/zéro
    inline constexpr int    kFlatTiltPairs     = 15;    // une par octave : 2,5 Hz → 82 kHz
}
