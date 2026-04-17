# Musique Drum Synth V2

Synth drum 12 pads pour Windows `Standalone` et `VST3`, calibre pour un usage studio polyvalent avec compatibilite stricte des IDs APVTS, rappel projet fiable et banque preset editorialisee.

## Produit

- 12 pads: `Kick A`, `Kick B`, `Snare`, `Clap`, `Hat Closed`, `Hat Open`, `Perc 1`, `Perc 2`, `Tom Low`, `Tom High`, `Crash`, `FX`
- 18 kits factory repartis sur 5 familles:
  - `Classique`: 3 kits
  - `Acoustique`: 4 kits
  - `Ambient`: 3 kits
  - `Cinematique`: 4 kits
  - `Moderne`: 4 kits
- Presets de kit en `preset_version = 5` avec metadonnees `familyLabel`, `mixRole`, `tags`, `description`, `outputProfile`, `nominalPeakDb`
- Presets utilisateur avec manifest JSON sidecar `.preset.json`, scan au chargement, backfill automatique des manifests manquants et compatibilite XML legacy
- Overrides factory explicites et rappel projet avec etat plugin + mappings `MIDI Learn`
- Double mode de jeu:
  - `Drum Pads`: mapping MIDI standard sur les 12 pads
  - `Single Note`: toute note joue le pad actuellement selectionne
- Choke group hi-hat entre `Hat Closed` et `Hat Open`

## Moteur

- Moteur par famille sans rupture d'interface publique:
  - kick/tom: corps tonal, sub, contour de pitch, click
  - snare: couche tonale + bruit/rattle plus separes
  - clap: multi-burst avec largeur stereo interne
  - hat/crash: voix metalliques optimisees avec pruning adaptatif des partiels
  - perc/fx: modal et FM retunes pour des roles plus distincts
- Hardening audio:
  - trim par voix
  - blocage DC
  - meilleure fermeture de voix
  - protection legere des sorties auxiliaires

## FX et routage

- Macros globales: `Punch`, `Weight`, `Air`, `Dirt`
- Chaine FX globale complete:
  - `Compressor`
  - `Saturator`
  - toggles dedies pour `Compressor`, `Saturator`, `Transient` et `Reverb`
  - `Quality Mode`: `Live` / `Studio` avec oversampling 4x du saturateur global en mode `Studio`
  - `Transient`
  - `Reverb` avec `Pre-delay`
  - `EQ`
  - `Chorus`
  - `Delay` avec `Sync` et `Note Division`
  - `Limiter`
- Modulation globale:
  - `Velocity Curve`: `Linear`, `Soft`, `Softer`, `Hard`, `Harder`, `Fixed`, `Touch`
  - `LFO` global (tremolo / auto-pan): `Rate`, `Depth`, `Wave`
  - `Humanize`: jitter temporel jusqu'a `50 ms` et variation de niveau jusqu'a `20%`
- Multi-out:
  - `Bus 0`: `Master`
  - `Bus 1..12`: sorties auxiliaires dediees
  - les sorties auxiliaires restent `pre-FX globaux` pour compatibilite
  - une securite de production applique trim + soft protection sur les aux

## UI

- Edition pad: `Level`, `Tune`, `Decay`, `Attack`, `Pitch Drop`, `Pitch Decay`, `Noise`, `Click`, `Drive`, `Cutoff`, `Pan`
- Selecteur de `Pad Preset` directement dans l'editeur
- Pages `ADV FX` pour exposer `EQ`, `Chorus`, `Delay`, `Limiter` et `Reverb Predelay`
- Header global enrichi:
  - `Quality Mode`
  - `Velocity Curve`
  - `LFO` (`Rate`, `Depth`, `Wave`)
  - `Humanize` (`Timing`, `Level`)
- Browser preset production:
  - filtres `source`, `family`, `mixRole`, `tags`
  - affichage du peak nominal, du profil de sortie et des metadonnees du kit courant
- Telemetrie runtime:
  - meters `Main` / `Aux`
  - clip latch
  - statut `delay_sync`, BPM detecte et mode `aux_post_fx`
- `MIDI Learn` explicite:
  - armement d'un parametre
  - capture CC
  - liste des mappings
  - suppression unitaire ou reset global
- Gestion preset clarifiee:
  - `SAVE` met a jour le preset utilisateur courant
  - `SAVE AS` cree un nouveau preset utilisateur
  - `OVERRIDE` ecrit un override factory explicite

## Validation

Le renderer console fournit la batterie QA de reference:

```bash
.\build\UWdeVST_drum_renderer_artefacts\Release\UWdeVST_drum_renderer.exe --validate-presets
.\build\UWdeVST_drum_renderer_artefacts\Release\UWdeVST_drum_renderer.exe --validate-matrix
.\build\UWdeVST_drum_renderer_artefacts\Release\UWdeVST_drum_renderer.exe --benchmark
.\build\UWdeVST_drum_renderer_artefacts\Release\UWdeVST_drum_renderer.exe --benchmark --baseline .\qa\drum_cpu_benchmark.csv
```

Couverture actuelle:

- `validate-presets`
  - multi-velocite
  - rendus courts et longs
  - NaN/Inf/silence/clipping
  - stereo utile sur les sources larges
  - choke hats
  - FX on/off
  - round-trip des nouveaux params globaux (`quality_mode`, `delay_sync`, `velocity_curve`, `humanize`)
  - securite aux
  - round-trip preset
  - rapport CSV par kit: peak, RMS, crest factor, tail, stereo, output profile et completude metadata
- `validate-matrix`
  - verification audio rendue par groupes `Kick`, `Snare`, `Hat`, `Crash`, `FX`
  - metriques: peak, densite haute frequence, queue, stereo
- `benchmark`
  - scenarios musicaux representatifs
  - `Single Hit`
  - `Dense Groove`
  - `Hat Choke`
  - `FX Chain`
  - export CSV avec baseline optionnelle et echec si regression CPU > `20%`

## Build

### Prerequis

- CMake 3.22+
- C++20
- JUCE 8.x

### Configuration

```bash
cmake -S . -B build
cmake --build build --config Release
```

Scope signe:

- Windows `VST3 + Standalone` uniquement
- `VST2` hors support, hors QA et hors release

## Structure

- `Source/PluginProcessor.*`
- `Source/PluginEditor.*`
- `Source/Engine/DrumSynthVoice.*`
- `Source/Engine/FactoryPresets.*`
- `Source/Render/DrumManifestRenderer.cpp`
