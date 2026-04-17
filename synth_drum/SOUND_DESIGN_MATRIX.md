# Drum Sound Design Matrix

Cette matrice reste la source de design pour la construction des kits factory, mais la QA ne valide plus une "density" theorique decorrelee du moteur. La validation de reference est maintenant un rendu audio mesure dans le renderer.

## Groupes cibles

- `Kick`: pads `0-1`
- `Snare`: pads `2-3`
- `Hat`: pads `4-5`
- `Crash`: pad `10`
- `FX`: pad `11`

## Matrice de design

| Family | Kick Level | Kick Density | Snare Level | Snare Density | Hat Level | Hat Density | Crash Level | Crash Density | FX Level | FX Density |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Classique | 0.86 | 0.58 | 0.73 | 0.66 | 0.55 | 0.52 | 0.46 | 0.43 | 0.50 | 0.68 |
| Acoustique | 0.84 | 0.44 | 0.71 | 0.50 | 0.54 | 0.38 | 0.46 | 0.34 | 0.52 | 0.50 |
| Ambient | 0.78 | 0.40 | 0.66 | 0.48 | 0.52 | 0.37 | 0.42 | 0.34 | 0.48 | 0.52 |
| Cinematique | 0.87 | 0.64 | 0.72 | 0.63 | 0.54 | 0.46 | 0.47 | 0.50 | 0.54 | 0.72 |
| Moderne | 0.85 | 0.60 | 0.71 | 0.61 | 0.56 | 0.49 | 0.46 | 0.40 | 0.53 | 0.66 |

## Interpretation

- `Level` guide le balance de groupe au moment de la construction du kit.
- `Density` guide les nudges de `drive`, `noise`, `click` et `cutoff`.
- L'application se fait dans `Source/Engine/FactoryPresets.cpp` via `applyTargetMatrix`.

## QA audio rendue

Le renderer valide maintenant chaque groupe sur un rendu reel avec des tolerances larges mais musicales:

| Group | Peak dBFS | HF Ratio | Tail ms | Stereo |
| --- | --- | --- | --- | --- |
| Kick | `-18 .. -0.05` | `0.008 .. 0.18` | `40 .. 1050` | `0.00 .. 0.06` |
| Snare | `-22 .. -1.2` | `0.06 .. 0.92` | `18 .. 460` | `0.00 .. 0.24` |
| Hat | `-28 .. -6` | `0.14 .. 1.00` | `6 .. 300` | `0.02 .. 0.28` |
| Crash | `-30 .. -8` | `0.08 .. 0.95` | `60 .. 1300` | `0.02 .. 0.35` |
| FX | `-28 .. -4` | `0.04 .. 0.92` | `10 .. 720` | `0.02 .. 0.35` |

## Validation pratique

- `--validate-matrix` doit passer a `90/90`
- les kits restent libres de varier musicalement tant qu'ils restent dans ces plages
- la matrice de design sert au shaping des kits, pas a imposer une metrique impossible au moteur
