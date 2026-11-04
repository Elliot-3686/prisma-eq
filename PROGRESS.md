# Journal — Prisma EQ

Budget accordé : **33 $** (phases 1–2 + interface minimale, voir PLAN.md).

## Phase 1 — Fondations
- [x] CMake + JUCE 8.0.15 (FetchContent), cibles AU / VST3 / Standalone, identité du produit en tête du CMakeLists
- [x] 24 bandes × 10 paramètres + 10 paramètres globaux, identifiants stables, état versionné
- [x] Section de sortie temps réel : gain, pan L/R ou M/S, inversion de phase, bypass doux (fondus de 20 ms)
- [x] Cœur DSP `prisma_dsp` sans JUCE + tests Catch2
- [x] CI macOS : build universel, tests, auval, pluginval 5, release `latest`
- [x] Vérifié sur la CI macOS 15.8 / Xcode 16.4 (commit cc02223) :
  - build universel arm64 + x86_64 et 4 tests passés ;
  - `auval -strict -v aufx PrEq Prsm` : **AU VALIDATION SUCCEEDED** ;
  - pluginval niveau 5 : **SUCCESS** sur l'AU et sur le VST3 ;
  - sous Linux aussi, pluginval niveau 5 passe sur le VST3.

## Phase 2 — Moteur Zero Latency
- [x] `AnalogPrototype` (10 formes, Butterworth, shelves d'ordre élevé, Oustaloup), `BiquadDesign` (matched Vicanek), `EqEngine` (24 bandes, TDF-II double, lissage + interpolation des coefficients, L/R/M/S, gain auto)
- [x] Branché dans `PrismaProcessor` (gain scale, gain auto, bypass doux sur le signal sec réel)
- [x] Tests de mesure (Linux, `./build/prisma_tests | grep MESURE`) :
  - magnitude contre l'analogique, 10 formes × 5 fréquences d'échantillonnage : ≤ 0,62 dB pour f0 ≤ fs/16 ; écarts près de Nyquist documentés (DECISIONS.md §6) ;
  - pentes 6 → 96 dB/oct par pas de 1,5 : erreur max 0,33 dB/oct ;
  - null test 24 bandes à 0 dB : −319 dB ; passe-tout : |H| = 1 à 1e−12 dB ;
  - zipper : −112,7 dB (44,1 kHz), −108,4 dB (96 kHz) ;
  - blocs 1 → 4096 et tailles aléatoires : rendu identique au bit près ;
  - 0 allocation dans le traitement (`operator new` remplacé) ;
  - placement L/R/M/S vérifié ; gain auto cohérent.
- [x] pluginval niveau 5 sur le VST3 sous Linux : SUCCESS

## Interface minimale
- [x] Afficheur (grille Hz/dB ±3/6/12/30, courbe jaune exacte, courbe de la bande sélectionnée), points déplaçables, molette Q/pente, double-clic = nouvelle cloche, clic droit = forme/pente/placement/active/supprimer
- [x] Bandeau de la bande (saisie directe) et barre du bas (mode, échelle, sortie, échelle de gain, gain auto, phase, bypass)
- [x] Rendu à la demande (écouteurs de paramètres + minuterie à 30 Hz qui ne redessine que si nécessaire)

## Vérification CI macOS 15.8.2 (commit 09199e3)
- build universel arm64 + x86_64 sans erreur ni avertissement ; 14 tests passés, mesures identiques à Linux ;
- `auval -strict -v aufx PrEq Prsm` : **AU VALIDATION SUCCEEDED** ;
- pluginval niveau 5 : **SUCCESS** sur l'AU (un seul avertissement bénin : « Current program is -1 ») et sur le VST3 ;
- release `latest` mise à jour (AU + VST3 + Standalone).

## Clôture (version 0.2.0)
- [x] README : fonctionnalités, installation dans Logic en 5 étapes (`xattr`, rescan), dépannage (`auval`, `AudioComponentRegistrar`, signature ad hoc), utilisation, compilation
- [x] `CHANGELOG.md` (sert de notes de release), écarts de distribution dans `DECISIONS.md` (§18 à 20)
- [x] CI : capture PNG de l'interface (`Tools/Snapshot.cpp`), rapport de tests généré (`rapport-tests.md`), archive `PrismaEQ-<version>-macOS.zip` avec `LISEZMOI.txt`
- [x] Release stable `v0.2.0` créée par la CI au premier build entièrement validé
- [x] Vérifié sur la CI macOS 15.8.2 (commit 9f219de) :
  - build, 14 tests, `auval -strict`, pluginval 5 (AU et VST3) et capture : tout réussi ;
  - release **v0.2.0** publiée (archive, rapport, capture) et `RAPPORT_TESTS.md` versé dans le dépôt.
- [x] Correction après la release : la sortie affichait « -0.00 dB » sur Mac (zéro négatif dû aux arrondis). Elle sera dans la prochaine version ; `latest` l'a déjà.

## Notes de passation (suite)
- **Limites connues** (voir DECISIONS.md) : bandes proches de Nyquist (§6), changement de forme ou de pente non lissé (§9), bells d'ordre élevé reportées (§2), saisie en notes reportée (§15).
- **Linux** : un conteneur neuf doit installer les paquets X11/ALSA avant la compilation de JUCE :
  `apt-get install libasound2-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev libxcomposite-dev libfreetype-dev libfontconfig1-dev libgl1-mesa-dev libcurl4-openssl-dev`.

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
