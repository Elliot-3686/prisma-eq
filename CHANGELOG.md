# Historique des versions

## Non publié
- Affichage : plus de « -0.00 dB » pour un gain nul (zéro négatif dû aux arrondis flottants).
- (Renommer cette section en « ## X.Y.Z — date » en même temps que `PRISMA_VERSION` : la CI en tire les notes de release.)

## 0.2.0 — 2026-11-04
- **Moteur Zero Latency.**
  - 10 formes.
  - Pentes de 6 à 96 dB/oct, fractionnaires pour les coupes.
  - Biquads « matched » en double précision.
  - Placement stéréo, gauche, droite, mid ou side par bande.
  - Paramètres lissés et coefficients interpolés (pas de zipper).
  - Gain auto, échelle de gain, interaction gain-Q.
- **Interface minimale.**
  - Afficheur avec la courbe exacte et une échelle réglable.
  - Points déplaçables, molette, menu contextuel.
  - Bandeau de la bande avec saisie directe ; barre du bas.
- **Validation** : 14 tests de mesure, `auval -strict`, pluginval niveau 5 sur l'AU et le VST3. Le rapport de tests est joint à la release.
- **Limites connues** (détail dans `DECISIONS.md`) :
  - précision réduite pour les bandes proches de Nyquist ;
  - changement de forme ou de pente non lissé ;
  - modes Natural Phase et Linear Phase pas encore actifs.

## 0.1.0
- Fondations : AU, VST3 et Standalone ; 24 bandes de paramètres ; section de sortie ; CI macOS.
