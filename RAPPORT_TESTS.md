# Rapport de tests — Prisma EQ 0.2.0

Généré automatiquement par la CI macOS (`.github/workflows/build.yml`).

- Commit : `9f219de7ae6b372c6b9a84729e171405b50d1fbe`
- Date : 2026-11-04 22:32 UTC
- Machine : macOS 15.8.2, Xcode 16.4, arm64

## Verdicts

| Étape | Résultat |
|---|---|
| Build universel arm64 + x86_64 | ✅ réussi |
| Tests unitaires Catch2 | ✅ réussi (100% tests passed out of 14) |
| `auval -strict -v aufx PrEq Prsm` | ✅ réussi |
| pluginval niveau 5, AU et VST3 | ✅ réussi |
| Capture de l'interface | ✅ réussi |

## Mesures

Chaque ligne est affichée par un test Catch2 (préfixe `MESURE`). Seuils et écarts assumés : voir `DECISIONS.md`.

```
MESURE passe-tout fs= 44100  |H| max écart = 7.81e-14 dB
MESURE passe-tout fs= 48000  |H| max écart = 9.06e-14 dB
MESURE passe-tout fs= 88200  |H| max écart = 3.12e-13 dB
MESURE passe-tout fs= 96000  |H| max écart = 4.13e-13 dB
MESURE passe-tout fs=192000  |H| max écart = 1.40e-12 dB
MESURE null test (24 bandes à 0 dB, L/R/M/S) : écart max -319.1 dB
MESURE allocations pendant le traitement : 0
MESURE flat tilt fs= 44100 gain -30.0 : écart max à la droite 0.613 dB
MESURE flat tilt fs= 44100 gain  +9.0 : écart max à la droite 0.163 dB
MESURE flat tilt fs= 44100 gain +30.0 : écart max à la droite 0.545 dB
MESURE flat tilt fs= 48000 gain -30.0 : écart max à la droite 0.440 dB
MESURE flat tilt fs= 48000 gain  +9.0 : écart max à la droite 0.121 dB
MESURE flat tilt fs= 48000 gain +30.0 : écart max à la droite 0.391 dB
MESURE flat tilt fs= 88200 gain -30.0 : écart max à la droite 0.242 dB
MESURE flat tilt fs= 88200 gain  +9.0 : écart max à la droite 0.027 dB
MESURE flat tilt fs= 88200 gain +30.0 : écart max à la droite 0.039 dB
MESURE flat tilt fs= 96000 gain -30.0 : écart max à la droite 0.202 dB
MESURE flat tilt fs= 96000 gain  +9.0 : écart max à la droite 0.021 dB
MESURE flat tilt fs= 96000 gain +30.0 : écart max à la droite 0.044 dB
MESURE flat tilt fs=192000 gain -30.0 : écart max à la droite 0.291 dB
MESURE flat tilt fs=192000 gain  +9.0 : écart max à la droite 0.049 dB
MESURE flat tilt fs=192000 gain +30.0 : écart max à la droite 0.110 dB
MESURE gain auto : cloche +6 dB/Q1 → -1.41 dB, plateau grave +6 dB à 1 kHz → -4.17 dB, flat tilt ±5 dB → -0.95 dB
MESURE invariance aux blocs (1, 2, 31, 32, 33, 256, 1000, 4096, aléatoire) : écart max 0.0e+00
MESURE pente consigne   7.5 dB/oct : coupe-haut   7.39, coupe-bas   7.39
MESURE pente consigne  12.0 dB/oct : coupe-haut  12.03, coupe-bas  12.03
MESURE pente consigne  24.0 dB/oct : coupe-haut  24.08, coupe-bas  24.08
MESURE pente consigne  36.0 dB/oct : coupe-haut  36.13, coupe-bas  36.12
MESURE pente consigne  48.0 dB/oct : coupe-haut  48.17, coupe-bas  48.16
MESURE pente consigne  52.5 dB/oct : coupe-haut  52.63, coupe-bas  52.61
MESURE pente consigne  60.0 dB/oct : coupe-haut  60.21, coupe-bas  60.21
MESURE pente consigne  72.0 dB/oct : coupe-haut  72.25, coupe-bas  72.25
MESURE pente consigne  84.0 dB/oct : coupe-haut  84.29, coupe-bas  84.29
MESURE pente consigne  96.0 dB/oct : coupe-haut  96.33, coupe-bas  96.33
MESURE pente : erreur max 0.334 dB/oct (6 → 96 par pas de 1,5)
MESURE zipper fs= 44100 : bruit de modulation au-delà de 1 kHz = -112.7 dB
MESURE zipper fs= 96000 : bruit de modulation au-delà de 1 kHz = -108.4 dB
MESURE magnitude Bell      fs= 44100 | f0<=fs/16 : 0.305 dB (<fs/4), 0.325 dB (->0.95Ny) | f0>fs/16 : 4.236 / 4.635 dB
MESURE magnitude Bell      fs= 48000 | f0<=fs/16 : 0.278 dB (<fs/4), 0.292 dB (->0.95Ny) | f0>fs/16 : 3.867 / 3.803 dB
MESURE magnitude Bell      fs= 88200 | f0<=fs/16 : 0.443 dB (<fs/4), 0.420 dB (->0.95Ny) | f0>fs/16 : 1.503 / 1.577 dB
MESURE magnitude Bell      fs= 96000 | f0<=fs/16 : 0.384 dB (<fs/4), 0.383 dB (->0.95Ny) | f0>fs/16 : 1.478 / 1.544 dB
MESURE magnitude Bell      fs=192000 | f0<=fs/16 : 0.520 dB (<fs/4), 0.465 dB (->0.95Ny) | f0>fs/16 : 0.804 / 0.747 dB
MESURE magnitude LowShelf  fs= 44100 | f0<=fs/16 : 0.239 dB (<fs/4), 0.095 dB (->0.95Ny) | f0>fs/16 : 7.288 / 27.933 dB
MESURE magnitude LowShelf  fs= 48000 | f0<=fs/16 : 0.202 dB (<fs/4), 0.079 dB (->0.95Ny) | f0>fs/16 : 5.357 / 35.149 dB
MESURE magnitude LowShelf  fs= 88200 | f0<=fs/16 : 0.380 dB (<fs/4), 0.155 dB (->0.95Ny) | f0>fs/16 : 6.886 / 1.907 dB
MESURE magnitude LowShelf  fs= 96000 | f0<=fs/16 : 0.318 dB (<fs/4), 0.129 dB (->0.95Ny) | f0>fs/16 : 4.940 / 1.723 dB
MESURE magnitude LowShelf  fs=192000 | f0<=fs/16 : 0.466 dB (<fs/4), 0.193 dB (->0.95Ny) | f0>fs/16 : 0.858 / 0.387 dB
MESURE magnitude LowCut    fs= 44100 | f0<=fs/16 : 0.046 dB (<fs/4), 0.038 dB (->0.95Ny) | f0>fs/16 : 3.846 / 3.231 dB
MESURE magnitude LowCut    fs= 48000 | f0<=fs/16 : 0.039 dB (<fs/4), 0.032 dB (->0.95Ny) | f0>fs/16 : 3.090 / 2.596 dB
MESURE magnitude LowCut    fs= 88200 | f0<=fs/16 : 0.071 dB (<fs/4), 0.059 dB (->0.95Ny) | f0>fs/16 : 0.753 / 0.622 dB
MESURE magnitude LowCut    fs= 96000 | f0<=fs/16 : 0.060 dB (<fs/4), 0.050 dB (->0.95Ny) | f0>fs/16 : 0.628 / 0.520 dB
MESURE magnitude LowCut    fs=192000 | f0<=fs/16 : 0.087 dB (<fs/4), 0.071 dB (->0.95Ny) | f0>fs/16 : 0.153 / 0.125 dB
MESURE magnitude HighShelf fs= 44100 | f0<=fs/16 : 0.226 dB (<fs/4), 0.090 dB (->0.95Ny) | f0>fs/16 : 31.213 / 24.736 dB
MESURE magnitude HighShelf fs= 48000 | f0<=fs/16 : 0.190 dB (<fs/4), 0.076 dB (->0.95Ny) | f0>fs/16 : 32.890 / 28.421 dB
MESURE magnitude HighShelf fs= 88200 | f0<=fs/16 : 0.355 dB (<fs/4), 0.143 dB (->0.95Ny) | f0>fs/16 : 6.945 / 2.170 dB
MESURE magnitude HighShelf fs= 96000 | f0<=fs/16 : 0.299 dB (<fs/4), 0.120 dB (->0.95Ny) | f0>fs/16 : 4.827 / 1.705 dB
MESURE magnitude HighShelf fs=192000 | f0<=fs/16 : 0.438 dB (<fs/4), 0.175 dB (->0.95Ny) | f0>fs/16 : 0.812 / 0.323 dB
MESURE magnitude HighCut   fs= 44100 | f0<=fs/16 : 0.302 dB (<fs/4), 1.098 dB (->0.95Ny) | f0>fs/16 : 0.793 / 2.119 dB
MESURE magnitude HighCut   fs= 48000 | f0<=fs/16 : 0.213 dB (<fs/4), 1.098 dB (->0.95Ny) | f0>fs/16 : 0.568 / 1.916 dB
MESURE magnitude HighCut   fs= 88200 | f0<=fs/16 : 0.538 dB (<fs/4), 1.099 dB (->0.95Ny) | f0>fs/16 : 0.679 / 2.063 dB
MESURE magnitude HighCut   fs= 96000 | f0<=fs/16 : 0.371 dB (<fs/4), 1.098 dB (->0.95Ny) | f0>fs/16 : 0.588 / 1.880 dB
MESURE magnitude HighCut   fs=192000 | f0<=fs/16 : 0.568 dB (<fs/4), 1.100 dB (->0.95Ny) | f0>fs/16 : 0.595 / 1.364 dB
MESURE magnitude Notch     fs= 44100 | f0<=fs/16 : 0.016 dB (<fs/4), 0.079 dB (->0.95Ny) | f0>fs/16 : 1.078 / 5.760 dB
MESURE magnitude Notch     fs= 48000 | f0<=fs/16 : 0.013 dB (<fs/4), 0.067 dB (->0.95Ny) | f0>fs/16 : 0.881 / 4.670 dB
MESURE magnitude Notch     fs= 88200 | f0<=fs/16 : 0.025 dB (<fs/4), 0.123 dB (->0.95Ny) | f0>fs/16 : 0.253 / 1.272 dB
MESURE magnitude Notch     fs= 96000 | f0<=fs/16 : 0.020 dB (<fs/4), 0.104 dB (->0.95Ny) | f0>fs/16 : 0.210 / 1.072 dB
MESURE magnitude Notch     fs=192000 | f0<=fs/16 : 0.030 dB (<fs/4), 0.150 dB (->0.95Ny) | f0>fs/16 : 0.053 / 0.267 dB
MESURE magnitude BandPass  fs= 44100 | f0<=fs/16 : 0.619 dB (<fs/4), 0.970 dB (->0.95Ny) | f0>fs/16 : 2.571 / 1.411 dB
MESURE magnitude BandPass  fs= 48000 | f0<=fs/16 : 0.611 dB (<fs/4), 0.969 dB (->0.95Ny) | f0>fs/16 : 2.034 / 1.050 dB
MESURE magnitude BandPass  fs= 88200 | f0<=fs/16 : 0.612 dB (<fs/4), 0.972 dB (->0.95Ny) | f0>fs/16 : 0.561 / 1.019 dB
MESURE magnitude BandPass  fs= 96000 | f0<=fs/16 : 0.605 dB (<fs/4), 0.971 dB (->0.95Ny) | f0>fs/16 : 0.562 / 1.012 dB
MESURE magnitude BandPass  fs=192000 | f0<=fs/16 : 0.609 dB (<fs/4), 0.973 dB (->0.95Ny) | f0>fs/16 : 0.567 / 0.979 dB
MESURE magnitude TiltShelf fs= 44100 | f0<=fs/16 : 0.226 dB (<fs/4), 0.090 dB (->0.95Ny) | f0>fs/16 : 31.213 / 24.736 dB
MESURE magnitude TiltShelf fs= 48000 | f0<=fs/16 : 0.190 dB (<fs/4), 0.076 dB (->0.95Ny) | f0>fs/16 : 32.890 / 28.421 dB
MESURE magnitude TiltShelf fs= 88200 | f0<=fs/16 : 0.355 dB (<fs/4), 0.143 dB (->0.95Ny) | f0>fs/16 : 6.945 / 2.170 dB
MESURE magnitude TiltShelf fs= 96000 | f0<=fs/16 : 0.299 dB (<fs/4), 0.120 dB (->0.95Ny) | f0>fs/16 : 4.827 / 1.705 dB
MESURE magnitude TiltShelf fs=192000 | f0<=fs/16 : 0.438 dB (<fs/4), 0.175 dB (->0.95Ny) | f0>fs/16 : 0.812 / 0.323 dB
MESURE magnitude FlatTilt  fs= 44100 | f0<=fs/16 : 0.110 dB (<fs/4), 0.340 dB (->0.95Ny) | f0>fs/16 : 0.110 / 0.340 dB
MESURE magnitude FlatTilt  fs= 48000 | f0<=fs/16 : 0.098 dB (<fs/4), 0.349 dB (->0.95Ny) | f0>fs/16 : 0.098 / 0.349 dB
MESURE magnitude FlatTilt  fs= 88200 | f0<=fs/16 : 0.086 dB (<fs/4), 0.271 dB (->0.95Ny) | f0>fs/16 : 0.086 / 0.271 dB
MESURE magnitude FlatTilt  fs= 96000 | f0<=fs/16 : 0.071 dB (<fs/4), 0.271 dB (->0.95Ny) | f0>fs/16 : 0.071 / 0.271 dB
MESURE magnitude FlatTilt  fs=192000 | f0<=fs/16 : 0.031 dB (<fs/4), 0.148 dB (->0.95Ny) | f0>fs/16 : 0.031 / 0.148 dB
```
