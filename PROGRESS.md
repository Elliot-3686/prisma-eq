# Journal — Prisma EQ

Budget accordé : **33 $** (phases 1–2 + interface minimale, voir PLAN.md).

## Phase 1 — Fondations
- [x] CMake + JUCE 8.0.15 (FetchContent), cibles AU / VST3 / Standalone, identité du produit en tête du CMakeLists
- [x] 24 bandes × 10 paramètres + 10 paramètres globaux, identifiants stables, état versionné
- [x] Section de sortie temps réel : gain, pan L/R ou M/S, inversion de phase, bypass doux (fondus de 20 ms)
- [x] Cœur DSP `prisma_dsp` sans JUCE + tests Catch2
- [x] CI macOS : build universel, tests, auval, pluginval 5, release `latest`
- [ ] Vérification sur la CI macOS (dès que le dépôt existe)

## Phase 2 — Moteur Zero Latency
- [ ] Formes, pentes fractionnaires, placement L/R/M/S, lissage anti-zipper, gain auto — avec tests de mesure

## Interface minimale
- [ ] Afficheur, points déplaçables, bandeau de bande, barre du bas

## Notes de passation pour la phase 2 (moteur Zero Latency)
Ordre conseillé (chaque étape avec son test Catch2 qui affiche `MESURE …`) :
1. `AnalogPrototype` : H(s) exact de chaque forme (RBJ Cookbook, forme analogique) + évaluation complexe de la réponse en fréquence (sert aux tests ET à la courbe de l'interface).
2. `BiquadDesign` « matched » (M. Vicanek, *Matched Second Order Digital Filters*, 2016) :
   - pôles par invariance impulsionnelle : a1 = −2·e^(−ζω₀)·cos(ω₀√(1−ζ²)) (ou cosh si ζ > 1), a2 = e^(−2ζω₀), avec ζ = 1/(2Q) et ω₀ = 2πf/fs ;
   - zéros obtenus en égalant la magnitude analogique à DC, à ω₀ et à Nyquist (formules du papier pour LP, HP, BP et peaking) ;
   - pour les bells, vérifier le gain à Nyquist prescrit (S. Orfanidis, *Digital Parametric Equalizer Design with Prescribed Nyquist-Frequency Gain*, JAES 1997).
3. `HighOrderDesign` : bells et shelves d'ordre > 2 (S. Orfanidis, *High-Order Digital Parametric Equalizer Design*, JAES 2005, type Butterworth). Coupes : cascades de Butterworth, avec le Q utilisateur sur la section résonante.
4. `FractionalSlope` : la partie fractionnaire de la pente (et Flat Tilt) est une cascade de shelves du 1er ordre (approximation récursive d'Oustaloup sur 10 Hz – 0,45·fs). Test : pente mesurée entre 4·fc et 16·fc à ±0,5 dB/oct.
5. `BandProcessor` / `EqEngine` :
   - jusqu'à 8 sections + la section fractionnaire par bande, en TDF-II double précision ;
   - paramètres lissés par échantillon (log-fréquence, dB, log-Q), coefficients recalculés tous les 32 échantillons ;
   - placement L/R/M/S (encodage M/S seulement si une bande en a besoin) ;
   - aucune allocation : tout est dimensionné dans `prepare`.
6. Brancher le moteur dans `PrismaProcessor` : gain scale, gain auto (moyenne pondérée rose de la courbe, 20 Hz – 20 kHz), interaction gain-Q (constantes dans `Tuning.h`).

Tests obligatoires :
- erreur de magnitude max contre l'analogique (10 formes × 44,1/48/88,2/96/192 kHz, en précisant la zone proche de Nyquist) ;
- pentes 0 → 96 dB/oct ;
- null test ;
- balayage rapide fréquence/gain/Q avec bruit de modulation < −90 dB ;
- invariance aux tailles de bloc (1 → 4096) ;
- aucune allocation (remplacer `operator new` dans le binaire de test).
