# Audit reprise Drum - 2026-05-12

## Constat

L'audition negative ne vient pas d'un seul bug. Le projet avait deja une refonte partielle des presets: snare Brush remontee, kick Trap raccourci, percussions limitees sous snare, Tom High eloigne de la snare. Les gates automatiques passaient, mais les criteres etaient trop permissifs pour juger le rendu musical.

Les points encore defendables apres lecture code + QA sont:

1. Cymbales trop pauvres en haut du spectre.
   - `metalBaseHz` etait calcule avec `startFrequency / 10`.
   - Un hat base 5500 Hz produisait donc une base metallique autour de 550 Hz, avec la plupart des partiels concentres trop bas pour un hat credible.
   - Resultat probable: hats/crash percussifs mais peu brillants, sensation de clic ou bruit pauvre.

2. Le modele Hat/Crash privilegiait trop le transient court.
   - Config Hat: corps faible, modulation faible, click relativement fort.
   - Config Crash: corps/noise conservateurs.
   - Resultat probable: crest factor eleve, peu de matiere continue, fatigue ou impression de sample mediocre.

3. Les rapports QA ne suffisent pas a valider l'audition.
   - `--validate-matrix` accepte des plages larges.
   - `identity/*.wav` est normalise pour l'ecoute, donc utile pour comparer le timbre, mais pas pour juger le niveau reel.
   - Les stems revelent mieux l'equilibre: avant correction, `hats_cymbals` et `fx` restaient tres faibles face au core kick/snare.

## Corrections appliquees

### DSP metallique

Fichiers:

- `synth_drum/synth_drum/Source/Engine/DrumConstants.h`
- `synth_drum/synth_drum/Source/Engine/DrumSynthVoice.cpp`

Changements:

- `kMetalBaseMinHz`: `400 -> 1200`
- `kMetalBaseFreqDivisor`: `10 -> 3`
- Hat: plus de corps metallique et de bruit utile, click reduit.
- Crash: plus de corps et de modulation continue, click reduit.

Objectif: rendre hats/crash plus brillants, plus presents et moins "tick only".

### Metadonnees nominales

Fichier:

- `synth_drum/synth_drum/Source/Engine/FactoryPresets.cpp`

Changements:

- `Classique Standard`: nominal peak `-3.9 -> -2.3 dB`
- `Classique Open`: nominal peak `-3.1 -> -1.6 dB`
- `Cinematique Epic`: nominal peak `-3.4 -> -1.4 dB`

Objectif: aligner les metadonnees avec le rendu apres enrichissement des cymbales, sans masquer de clipping.

## Mesures apres correction

Commandes executees:

```powershell
cmake --build synth_drum\build --config Release --target UWdeVST_drum_renderer UWdeVST_drum_tests
.\synth_drum\build\UWdeVST_drum_tests_artefacts\Release\UWdeVST_drum_tests.exe
.\synth_drum\build\UWdeVST_drum_renderer_artefacts\Release\UWdeVST_drum_renderer.exe --validate-presets --report synth_drum\qa\drum_preset_qa_report_after_metal_fix.csv
.\synth_drum\build\UWdeVST_drum_renderer_artefacts\Release\UWdeVST_drum_renderer.exe --validate-matrix
.\synth_drum\build\UWdeVST_drum_renderer_artefacts\Release\UWdeVST_drum_renderer.exe --render-release-suite --output-base synth_drum\qa\drum_release_suite_after_metal_fix --report synth_drum\qa\drum_release_suite_report_after_metal_fix.csv
```

Resultats:

- Tests production: `OK (18 tests)`
- Presets: `2070/2070 passed`
- Matrix: `90/90 rendered groups within tolerance`
- Release suite: `17/17 checks passed`

Comparaison stem cymbales:

| Element | Avant | Apres |
| --- | ---: | ---: |
| `stems/hats_cymbals.wav` peak | -14.43 dBFS | -12.24 dBFS |
| `stems/hats_cymbals.wav` RMS | -45.52 dBFS | -42.68 dBFS |
| `stems/hats_cymbals.wav` crest | 31.08 dB | 30.43 dB |

## Decision

La correction ameliore objectivement la presence hats/crash sans casser les gates. Elle ne suffit pas a declarer le synth drum "fini": le prochain gate doit etre une ecoute humaine A/B des fichiers:

- `synth_drum/qa/drum_release_suite_after_metal_fix/main.wav`
- `synth_drum/qa/drum_release_suite_after_metal_fix/stems/hats_cymbals.wav`
- `synth_drum/qa/drum_release_suite_after_metal_fix/identity/04_hat_closed.wav`
- `synth_drum/qa/drum_release_suite_after_metal_fix/identity/05_hat_open.wav`
- `synth_drum/qa/drum_release_suite_after_metal_fix/identity/10_crash.wav`

Si l'audition reste mediocre, le probleme restant n'est plus seulement le moteur metallique: il faudra recalibrer les patterns de demo, la balance hats/core, et probablement reduire la promesse "acoustique" du produit au profit d'un positionnement drum synth hybride.
