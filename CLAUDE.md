# CLAUDE.md — Prisma EQ

Égaliseur paramétrique 24 bandes pour Logic Pro (AU), avec VST3 et Standalone. JUCE 8.0.15, CMake, C++20.
Fonctionnalités inspirées de Pro-Q 4, mais **code, interface, nom, presets et format de preset originaux**. Aucune rétro-ingénierie : uniquement la littérature DSP publique (RBJ, Orfanidis, Vicanek, Oustaloup).
Le plan et le budget sont dans `PLAN.md`. L'avancement est dans `PROGRESS.md`, les écarts avec la spécification dans `DECISIONS.md`.

## Architecture
- `Source/DSP/` : **C++20 pur, sans JUCE** (cible `prisma_dsp`). Tout le traitement du signal est ici, testable sans le framework.
- `Source/Params/` : paramètres APVTS. Les identifiants sont **stables** (`b07_freq`…) : ne jamais renuméroter ni réordonner les listes de choix (ajout en fin de liste uniquement).
- `Source/Plugin/` : processeur et éditeur JUCE (colle entre les paramètres et `prisma_dsp`).
- `Source/UI/` : afficheur et contrôles. Interface en français, chaînes regroupées pour une future traduction anglaise.
- `Tests/` : Catch2, liés seulement à `prisma_dsp`.

## Règles temps réel (non négociables)
- Dans `processBlock` : **aucune allocation, aucun verrou, aucune I/O**. Les paramètres se lisent via les `std::atomic<float>*` de l'APVTS.
- Échanges UI ↔ audio : atomics ou FIFO sans verrou.
- `juce::ScopedNoDenormals` dans chaque `processBlock`.
- Tout calcul coûteux (FIR, tables) se fait hors du thread audio, puis est publié par échange de pointeurs atomique et fondu enchaîné.

## Qualité
- Chaque affirmation sur la qualité DSP doit être **mesurée par un test** (réponse en fréquence contre le prototype analogique, pentes, null test, zipper, tailles de blocs de 1 à 4096, fréquences d'échantillonnage de 44,1 à 192 kHz).
- Les mesures chiffrées s'affichent dans la sortie des tests, préfixées par `MESURE`.

## Commandes
```
# Linux (ce conteneur) : VST3 + Standalone + tests
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4 && ctest --test-dir build --output-on-failure
# Tests seuls, sans JUCE (rapide)
cmake -S . -B build-tests -G Ninja -DPRISMA_BUILD_PLUGIN=OFF && cmake --build build-tests && ./build-tests/prisma_tests
```
- **Conteneur Linux neuf** : installer d'abord `libasound2-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev libxcomposite-dev libfreetype-dev libfontconfig1-dev libgl1-mesa-dev libcurl4-openssl-dev`.
- **Capture de l'interface** : `xvfb-run -a build/PrismaSnapshot_artefacts/Release/PrismaSnapshot capture.png` (la CI publie `interface.png`).
- **Release stable** : incrémenter `PRISMA_VERSION` et ajouter sa section dans `CHANGELOG.md`. La CI crée `vX.Y.Z` au premier build entièrement validé (tests, auval, pluginval).
- **macOS** : tout `git push` déclenche `.github/workflows/build.yml` (build universel, tests, `auval -v aufx PrEq Prsm`, pluginval niveau 5 sur AU et VST3, release `latest`).
- **Résultats de la CI** : les journaux d'Actions ne sont pas lisibles depuis le conteneur. Le workflow publie donc tout sur la branche `ci-results`. Pour suivre un run :
  - état : `gh api repos/Elliot-3686/prisma-eq/commits/<sha>/check-runs` ;
  - résultats : `git fetch origin ci-results`.

## Conventions
- Un commit par jalon fonctionnel, messages en français.
- Identité du produit (nom, codes AU, bundle ID) : en tête de `CMakeLists.txt` uniquement.
- Les constantes à régler à l'oreille vont dans `Source/DSP/Tuning.h`.
