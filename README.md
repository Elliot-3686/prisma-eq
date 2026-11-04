# Prisma EQ

Égaliseur paramétrique 24 bandes pour **Logic Pro** (Audio Unit), aussi disponible en VST3 et en application autonome. Conçu avec JUCE 8, en C++20.

> **En construction** (phase 1 : fondations). Voir [PLAN.md](PLAN.md) et [PROGRESS.md](PROGRESS.md).

## Installer dans Logic Pro (macOS)
1. Téléchargez `PrismaEQ-macOS.zip` depuis la release [latest](https://github.com/Elliot-3686/prisma-eq/releases/tag/latest), puis décompressez-le.
2. Copiez `Prisma EQ.component` dans `~/Library/Audio/Plug-Ins/Components/`. Pour d'autres logiciels, copiez aussi `Prisma EQ.vst3` dans `~/Library/Audio/Plug-Ins/VST3/`.
3. Dans le Terminal, lancez :
   ```bash
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/"Prisma EQ.component"
   ```
   C'est nécessaire parce que l'app n'est pas notariée.
4. Dans Logic Pro, ouvrez **Réglages › Gestionnaire de modules**, sélectionnez **Prisma Audio › Prisma EQ**, puis cliquez sur **Réinitialiser et réanalyser la sélection**.
5. Insérez-le sur une piste : **Audio Units › Prisma Audio › Prisma EQ**.

## Compiler
Voir [CLAUDE.md](CLAUDE.md) (commandes Linux/macOS, tests, CI).

## Licence
Code sous **AGPLv3** (voir `LICENSE`). JUCE est utilisé sous sa licence AGPLv3. Pour une diffusion commerciale en code fermé, il faudrait passer à une licence JUCE commerciale.
Prisma EQ n'est pas affilié à FabFilter. Aucun code, visuel ou preset de FabFilter n'est utilisé.
