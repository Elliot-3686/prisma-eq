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
