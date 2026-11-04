# Écarts avec la spécification — Prisma EQ

Chaque écart est assumé, justifié et, quand il concerne la qualité, mesuré par un test (`MESURE …`).

## Moteur Zero Latency (phase 2)
1. **Brickwall** : en Zero Latency, c'est la pente maximale (96 dB/oct). Le vrai brickwall viendra avec le mode Linear Phase.
2. **Bell, Notch, Band Pass, All Pass : 2e ordre uniquement.** La pente ne s'applique qu'aux coupes et aux shelves (y compris Tilt Shelf). Les bells d'ordre élevé (Orfanidis 2005) sont reportées : le gain en complexité n'est pas prioritaire.
3. **Shelves d'ordre élevé** : shelf Butterworth d'ordre N = arrondi(pente / 6), de 6 à 96 dB/oct, gain √G exactement à la fréquence. Pas de pente fractionnaire pour les shelves (elle n'a pas de sens physique clair) ; le Q agit sur la section la moins amortie.
4. **Pentes fractionnaires des coupes** : Butterworth d'ordre ⌊S/6⌋ + cascade de paires pôle/zéro du 1er ordre (une par octave sur 10 octaves au-delà de fc, Oustaloup récursif). Le Q utilisateur s'applique à la section résonante, et il est inactif à 6 dB/oct.
   - Mesure : erreur ≤ 0,35 dB/oct de 6 à 96 dB/oct par pas de 1,5 (pente entre 4·fc et 16·fc).
5. **Numérisation « matched »** (Vicanek 2016) :
   - pôles par invariance impulsionnelle ;
   - |H|² exact à DC et à Nyquist ;
   - 3e contrainte : égalité à ω0 pour les sections génériques (bell, coupe-haut, passe-bande), moindres carrés de l'erreur relative pour les shelves et les sections du 1er ordre ;
   - double zéro imposé à DC pour les coupe-bas ; zéros sur le cercle unité pour l'encoche ;
   - fréquences au-delà de Nyquist : pôles bornés à 0,98·π.
6. **Limite mesurée près de Nyquist.** Pour les bandes sous fs/16, l'écart au prototype analogique est ≤ 0,62 dB sur toutes les formes et toutes les fréquences d'échantillonnage (≤ 0,31 dB pour les cloches à 44,1 kHz). Les bandes placées près de Nyquist ne peuvent pas reproduire tout le prototype avec un seul biquad :
   - jusqu'à 4,6 dB pour une cloche large à 16 kHz en 44,1 kHz ;
   - bien davantage pour un shelf résonant (Q = 3) d'ordre 5 à 16 kHz.

   Ces chiffres sont affichés par le test. Les modes Natural Phase et Linear Phase (phase 4) corrigeront ce point.
7. **Flat Tilt** : le gain est l'écart total entre 20 Hz et 20 kHz (pente = gain / log2(1000) dB/oct), avec 0 dB à la fréquence pivot. Il est réalisé par 15 paires pôle/zéro du 1er ordre, de 2,5 Hz à 82 kHz.
   - Mesure : écart à la droite ≤ 0,16 dB pour ±9 dB, et ≤ 0,61 dB pour l'extrême ±30 dB à 44,1 kHz.
8. **Tilt Shelf** : high shelf de gain G multiplié par G^(−1/2), soit −G/2 en bas et +G/2 en haut.
9. **Anti-zipper** : lissage exponentiel de 20 ms (log-fréquence, dB, log-Q), conception tous les 32 échantillons, **interpolation linéaire des coefficients** entre deux conceptions.
   - Mesure : bruit de modulation −112,7 dB (44,1 kHz) et −108,4 dB (96 kHz) pour un balayage complet en 0,5 s.
   - Un changement de forme ou de pente remet à zéro l'état de la bande (clic possible) : ce n'est pas lissé dans cette version.
10. **Activation/désactivation d'une bande** : fondu sec/traité de 20 ms par bande, compté en échantillons entiers. Le rendu est ainsi identique au bit près quelle que soit la taille des blocs.
11. **Placement** : M/S n'est encodé que pour les bandes M/S, et décodé avant une bande L/R. En mono, toutes les bandes s'appliquent sauf Side.
12. **Gain auto** : moyenne de puissance de la courbe analogique cible sur 40 points log-espacés de 20 Hz à 20 kHz (pondération rose = uniforme par octave). Elle est calculée sur deux « canaux » virtuels (L/M et R/S), puis lissée sur 100 ms.
   - Recalculé seulement quand une bande change.
   - Ce n'est pas une mesure du signal : la compensation est une estimation à partir de la courbe.
13. **Interaction gain-Q** : Q effectif = Q·((|g| + 3)/(12 + 3))^0,5, avec les constantes dans `Tuning.h`. Elle n'est pas encore « mémorisée pour les nouvelles instances » : les valeurs sont des constantes de compilation.

## Interface minimale
14. Les modes Natural Phase et Linear Phase sont sélectionnables, mais le moteur reste Zero Latency (infobulle explicite).
15. La saisie directe des fréquences accepte « 1,5 kHz », « 1.5k » ou « 250 ». La saisie en notes (« A4 », `NoteParsing`) est reportée.
16. La courbe globale additionne toutes les bandes actives, quel que soit leur placement (L/R/M/S). Des courbes séparées par canal viendront avec l'analyseur.
17. Les points des formes sans gain (coupes, encoche, passe-bande, passe-tout) sont placés sur la ligne 0 dB. La molette règle la pente pour les coupes, et le Q pour les autres formes.

## Distribution (clôture)
18. **Signature** : le plugin n'est ni signé par un Developer ID ni notarié, car cela demande un compte Apple Developer payant.
    - Les binaires portent seulement la signature ad hoc posée par l'éditeur de liens (option JUCE `-adhoc_codesign`).
    - L'utilisateur lance donc `xattr -dr com.apple.quarantine` après le téléchargement (README).
19. **Releases** :
    - `latest` est une préversion recompilée à chaque push.
    - `vX.Y.Z` est créée automatiquement, une seule fois par version, au premier build où les tests, auval et pluginval passent tous.
    - Ses notes sont la section correspondante de `CHANGELOG.md`.
20. **Capture de l'interface** : un petit outil (`Tools/Snapshot.cpp`, cible `PrismaSnapshot`, architecture native seulement) instancie l'éditeur hors écran avec des bandes de démonstration. La CI publie l'image `interface.png`.
