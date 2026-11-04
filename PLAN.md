# Plan — Prisma EQ (égaliseur paramétrique pour Logic Pro)

## Contexte
Vous voulez un égaliseur fonctionnellement équivalent à Pro-Q 4 pour Logic Pro : AU, VST3 et Standalone, avec JUCE 8, CMake et C++20, et une interface en français. Le cahier des charges complet représente un très gros projet.

**Budget : 33 $.** Il couvre les **phases 1 et 2 du cahier, plus une interface minimale**. Le reste (phases 3 à 10) sera chiffré et planifié à la fin.

Code, interface, nom, presets et format de preset sont originaux : aucun binaire FabFilter, aucune rétro-ingénierie. Les bases sont uniquement la littérature publique (RBJ Cookbook, Orfanidis, Vicanek, Oustaloup).

**Environnement vérifié.**
- Ce conteneur dispose de gcc 13, clang 18, CMake 3.28, Ninja, Xvfb et des paquets Linux nécessaires à JUCE.
- JUCE 8.0.15, Catch2 3.16.1 et pluginval 1.0.4 sont téléchargeables.
- Je compile et teste donc le DSP, le VST3 et le Standalone **ici, en quelques secondes**.
- **AU, auval et binaire universel** passent sur GitHub Actions macOS, avec le même circuit que Spectre : résultats sur la branche `ci-results`, release `latest`.
- Je ne peux pas lancer Logic Pro. C'est vous qui ferez l'écoute finale.

## Ce que livrent les 33 $

### Phase 1 — Fondations (≈ 6 $)
- Dépôt `prisma-eq` public, licence AGPLv3 (compatible avec JUCE). Vous restez propriétaire de votre code.
- CMake avec JUCE 8.0.15 épinglé via FetchContent. Cibles **AU (aufx) + VST3 + Standalone**, en arm64 et x86_64. Le nom tient dans une seule constante CMake :
  - nom : `Prisma EQ` ;
  - fabricant : `Prisma Audio` ;
  - codes : `Prsm` / `PrEq`.
- Paramètres APVTS pour 24 bandes. Chaque bande a : actif, forme, pente, fréquence, gain, Q, placement, interaction gain-Q. Les identifiants sont stables (`b07_freq`…), donc l'automation reste valide sans renumérotation.
- Section de sortie : bypass, inversion de phase, gain auto, gain scale, gain, pan L/R ou M/S, mode de traitement.
- `CLAUDE.md` : architecture, commandes, règle « aucune allocation, aucun verrou ni I/O dans `processBlock` ».
- CI macOS dérivée de `spectre/.github/workflows/build.yml` : tests, build universel, `auval -v aufx PrEq Prsm`, pluginval niveau 5+ sur AU et VST3, capture de l'interface, release `latest`.

### Phase 2 — Moteur d'EQ Zero Latency (≈ 14 $)
- **10 formes** : Bell, Low/High Shelf, Low/High Cut, Notch, Band Pass, Tilt Shelf, Flat Tilt, All Pass.
- **Conception calée sur le prototype analogique.**
  - Biquads « matched » de Vicanek, gain de Nyquist prescrit d'Orfanidis : pas de cramping près de Nyquist.
  - Ordres supérieurs pour les bells et shelves : EQ paramétrique d'ordre élevé d'Orfanidis (type Butterworth).
- **Pentes de 0 à 96 dB/oct, fractionnaires.**
  - Cascade de sections du 2e ordre.
  - La fraction est réalisée par une cascade de shelves du 1er ordre (approximation récursive d'Oustaloup).
  - Flat Tilt utilise la même technique pour obtenir une pente constante de 20 Hz à 20 kHz.
  - Minimums par forme conformes au cahier ; Q inactif à 6 dB/oct.
- **Brickwall** : en Zero Latency, ce sera la pente maximale (96 dB/oct), avec cet écart documenté. Le vrai brickwall arrivera avec le mode Linear Phase.
- **Interaction Gain-Q** (Bell) : 3 constantes réglables dans `Tuning.h`, mémorisées pour les nouvelles instances.
- **Placement par bande** : Stereo, L, R, M, S. Le passage en M/S ne se fait que si une bande en a besoin. Les modes mono et stéréo s'adaptent automatiquement.
- **Absence de zipper** : paramètres lissés par échantillon (en log-fréquence et en dB), coefficients recalculés tous les 16 à 32 échantillons, TDF-II en double précision.
- **Bypass doux** par fondu de 20 ms ; denormals coupés.
- **Gain auto** estimé à partir de la courbe : moyenne pondérée rose de 20 Hz à 20 kHz. Méthode documentée.

### Interface minimale (≈ 8 $)
- Afficheur avec la courbe jaune globale, calculée avec la réponse numérique exacte, et une grille Hz/dB réglable (±3/6/12/30 dB).
- Points de bande déplaçables :
  - glisser = fréquence et gain ;
  - molette = Q ou pente ;
  - double-clic sur le fond = nouvelle Bell ;
  - clic droit = forme, pente, placement, supprimer.
- Bandeau de la bande sélectionnée (valeurs exactes, saisie directe) et barre du bas (mode de traitement, sortie, bypass).
- Rendu à la demande, sans animation tant qu'il n'y a pas d'analyseur.

### Clôture (≈ 5 $)
Release AU/VST3/Standalone, README (installation dans Logic, `xattr`, rescan), rapport de tests, `PROGRESS.md`, et liste des écarts avec leur justification.

> Je n'ai pas de compteur de dépenses. Je m'arrête quand mon estimation approche 33 $, avec un état des lieux honnête. Si le budget manque, je sacrifie l'interface avant les tests.

## Arborescence
```
CMakeLists.txt  CLAUDE.md  README.md  PROGRESS.md  DECISIONS.md
Source/Plugin/   PluginProcessor, PluginEditor
Source/DSP/      AnalogPrototype, BiquadDesign (matched), HighOrderDesign,
                 FractionalSlope, BandProcessor, EqEngine, MidSide, Smoothing, Tuning.h
Source/Params/   ParameterLayout, NoteParsing (« A4 », « C#3+13 »)
Source/UI/       ResponseDisplay, BandStrip, BottomBar, Theme, Strings_fr
Tests/           Catch2 : réponse, pentes, null, zipper, allocations, fréq. d'éch./blocs
.github/workflows/build.yml
```
- `NoteParsing` reprend la logique note/cents de `spectre/Packages/SpectreCore/Sources/SpectreCore/Music.swift` (`MusicalNote`).
- L'analyseur des phases suivantes pourra reprendre la FFT multi-résolution de Spectre (`SpectrumAnalyzer.swift`).

## Points propres à l'AU et à Logic (règle 5 du cahier)
- **MIDI Learn** : dans Logic, un effet AU ne reçoit le MIDI que s'il est de type `aumf` (menu « AU MIDI-controlled Effects »). Je propose deux composants issus du même code :
  - `Prisma EQ` (aufx) ;
  - `Prisma EQ MIDI` (aumf, validé par `auval -v aumf`), avec le même format d'état.

  Ce sera en phase 7, et le README décrira la procédure.
- **Liste d'instances** : l'AU ne transmet ni l'ordre ni la couleur des pistes. Logic peut fournir le nom de piste (`kAudioUnitProperty_ContextName`, que JUCE remonte via `updateTrackProperties`), à vérifier chez vous.
  - Logic peut charger des AU hors processus (AUHostingService), d'où le besoin d'un registre en mémoire partagée.
- **Surround** : à ma connaissance, Logic va jusqu'à 7.1.4 (Atmos) ; je ne m'attends pas à du 9.1.6. Le plugin acceptera les dispositions annoncées par l'hôte.
- **Side chain AU** : un 2e bus d'entrée, géré par le menu « Side Chain » de Logic.
- **Latence variable** (Linear Phase) : Logic peut demander un arrêt et une relance de la lecture pour réaligner.
- **Plein écran** : une fenêtre de plugin dans Logic ne passe pas en plein écran macOS. Alternative : agrandir à la taille de l'écran.

## Risques techniques
1. **Natural Phase** (phase 4). Proposition : IIR « matched », plus un court FIR de correction de phase limité aux octaves aiguës, avec une latence d'environ 128 à 256 échantillons.
   - Le FIR est recalculé hors du thread audio, avec fondu enchaîné.
   - Un test mesurera l'écart de phase au prototype analogique.
   - Risque moyen.
2. **Linear Phase et dynamique** (phases 4 et 6) :
   - bandes statiques : convolution FFT par partitions, fondu entre l'ancien et le nouveau FIR pour éviter le zipper ;
   - bandes dynamiques et spectrales : moteur STFT avec des gains à phase nulle lissés, limité à la résolution High comme dans le cahier ;
   - latence rapportée et vérifiée par test d'impulsion.
3. **Spectral Dynamics** (phase 6) : détection par bin, lissages temporel et fréquentiel (Density), tilt de 3 dB/oct, seuil automatique.
   - Risques : bruit musical et CPU.
   - La bande spectrale s'applique en étage STFT après la chaîne IIR.
4. **Liste d'instances en AU** (phase 8) : mémoire partagée POSIX sans verrou ; contraintes de bac à sable possibles hors processus. Risque élevé.
5. **Character** (phase 9) : non-linéarités suréchantillonnées avec pré/post-accentuation, réglage à l'oreille via `Tuning.h`. Tests de THD et d'aliasing.
6. **Objectif CPU** (24 bandes × 50 instances) : mesuré sur le Mac virtuel de la CI (indicatif seulement) puis chez vous.

## Vérification
- **Ici, à chaque commit** : `cmake --build` puis `ctest` (Catch2).
  - Magnitude de chaque forme contre le prototype analogique (erreurs maximales documentées, y compris près de Nyquist) à 44,1 / 48 / 88,2 / 96 / 192 kHz.
  - Pentes de 0 à 96 dB/oct par pas fractionnaires : erreur ≤ 0,5 dB/oct.
  - Null test (bandes à 0 dB = entrée).
  - Balayage rapide fréquence/gain/Q avec mesure du bruit de modulation.
  - Blocs de 1 à 4096.
  - Aucune allocation dans `processBlock` (hook `operator new`).
  - pluginval VST3 sous Linux.
- **CI macOS** : build universel, `auval -v aufx PrEq Prsm`, pluginval 5+ sur AU et VST3, capture PNG de l'interface que je contrôle visuellement.
- **Chez vous** : installation, rescan dans Logic, écoute. Je vous enverrai une procédure de 5 lignes.

## Après validation de ce plan
1. Créez le dépôt **public** `prisma-eq` (avec README) :
   - dans GitHub : Settings › Applications › Claude › Configure › Repository access, puis ajoutez `prisma-eq` ;
   - dans le dépôt : Settings › Actions › General › « Read and write permissions ».
2. Donnez-moi son nom. Je pousse `CLAUDE.md`, ce plan et le squelette.
3. **Recommandé pour le budget** : continuez dans une **nouvelle session** ouverte sur `prisma-eq`. Cette conversation-ci porte tout l'historique de Spectre, donc chaque étape y coûte plus cher. La nouvelle session repartira de `CLAUDE.md` et `PLAN.md`.

## Suite : chiffrage des phases 3 à 10 (novembre 2026)
> **Reconstitué sans le cahier des charges**, qui n'est pas dans le dépôt. Les phases 4, 6, 7, 8 et 9 sont nommées plus haut dans ce plan. Le contenu des phases 3, 5 et 10 est déduit des fonctionnalités prévues. À corriger avec le cahier.

**Base du chiffrage.** La phase 2, l'interface minimale et la clôture étaient estimées à 27 $ ; elles ont coûté environ 22 $ (estimation, faute de compteur). Chaque phase compte une session neuve ouverte sur le dépôt, tests de mesure et vérification sur la CI macOS compris.

| Phase | Contenu | Estimation | Risque |
|---|---|---|---|
| 3 — Analyseur et interface complète | Analyseur FFT temps réel (pré/post, side-chain, gel), solo de bande, annuler/rétablir, A/B, sélection multiple, agrandissement. Rattrapages de la phase 2 : fondu au changement de forme ou de pente, saisie en notes, interaction gain-Q mémorisée | 12–16 $ | faible |
| 4 — Natural Phase et Linear Phase | FIR calculé hors du thread audio, convolution FFT par partitions, fondu entre filtres, latence rapportée à l'hôte, vrai brickwall ; tests de phase et de latence | 18–25 $ | élevé |
| 5 — Presets et EQ Match | Format de preset original, presets d'usine, navigateur ; EQ Match (capture de spectre, ajustement automatique des bandes) | 8–12 $ | moyen |
| 6 — Dynamique | Bandes dynamiques (seuil, plage, attaque, relâchement, seuil auto), side-chain AU ; Spectral Dynamics en STFT | 18–24 $ | élevé |
| 7 — MIDI Learn | Apprentissage MIDI ; composant `aumf` « Prisma EQ MIDI » validé par `auval -v aumf` | 5–7 $ | faible |
| 8 — Liste d'instances | Registre en mémoire partagée sans verrou, liste dans l'interface, cas des AU hors processus | 8–12 $ | élevé |
| 9 — Character | Saturations suréchantillonnées (Subtle, Warm), réglage dans `Tuning.h` ; tests de THD et d'aliasing | 6–9 $ | moyen |
| 10 — Surround et version 1.0 | Dispositions jusqu'au 7.1.4, optimisation CPU (SIMD, objectif 24 bandes × 50 instances), release 1.0 | 10–14 $ | moyen |
| **Total** | | **85–119 $** | |

**Recommandations.**
- Une phase par session neuve. Prévoir 20 % de marge pour les phases 4, 6 et 8, les plus risquées.
- Pour une 1.0 « essentielle » utile dans Logic, enchaîner les phases 3, 4, 5 et 6 (≈ 56–77 $) et reporter les phases 7 à 10.
- Le reliquat de l'enveloppe actuelle (quelques dollars) ne suffit pas pour ouvrir une phase.
- Message de départ conseillé pour chaque session : « Lis CLAUDE.md, PLAN.md et PROGRESS.md. Réalise la phase N avec ses tests de mesure, vérifie sur la CI macOS. Budget : X $. »
