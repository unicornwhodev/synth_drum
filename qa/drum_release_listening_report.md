# Drum Release Listening Report

Contexte: QA humaine obligatoire apres generation de `qa/drum_release_suite`.

Commande de reference:

```powershell
.\build\UWdeVST_drum_renderer_artefacts\Release\UWdeVST_drum_renderer.exe --render-release-suite --output-base qa\drum_release_suite --report qa\drum_release_suite_report.csv
```

Verdicts autorises: `OK`, `P2`, `P1-blocker`, `P0-regression`.

Systeme de notation detaille:

```text
qa/drum_audition_evaluation/
```

Dans le bundle RC, copier le meme contenu dans:

```text
audition_evaluation/
```

## Table d'ecoute par pad

| Pad | Famille | Fichiers a ecouter | Attaque | Pitch / centre | Decay / release | Chevauchement | Lisibilite mix | Role musical | Verdict | Gravite |
|---:|---|---|---|---|---|---|---|---|---|---|
| 00 Kick A | Core | `main.wav`, `stems/core_kick_snare.wav`, `identity/00_kick_a.wav` |  |  |  |  |  |  |  |  |
| 01 Kick B | Core | `main.wav`, `stems/core_kick_snare.wav`, `identity/01_kick_b.wav` |  |  |  |  |  |  |  |  |
| 02 Snare | Core | `main.wav`, `stems/core_kick_snare.wav`, `identity/02_snare.wav` |  |  |  |  |  |  |  |  |
| 03 Clap | Core | `main.wav`, `stems/core_kick_snare.wav`, `identity/03_clap.wav` |  |  |  |  |  |  |  |  |
| 04 Hat Closed | Hats | `main.wav`, `stems/hats_cymbals.wav`, `identity/04_hat_closed.wav` |  |  |  |  |  |  |  |  |
| 05 Hat Open | Hats | `main.wav`, `stems/hats_cymbals.wav`, `identity/05_hat_open.wav` |  |  |  |  |  |  |  |  |
| 06 Perc 1 | Perc/Toms | `main.wav`, `stems/percussion_toms.wav`, `identity/06_perc_1.wav` |  |  |  |  |  |  |  |  |
| 07 Perc 2 | Perc/Toms | `main.wav`, `stems/percussion_toms.wav`, `identity/07_perc_2.wav` |  |  |  |  |  |  |  |  |
| 08 Tom Low | Perc/Toms | `main.wav`, `stems/percussion_toms.wav`, `identity/08_tom_low.wav` |  |  |  |  |  |  |  |  |
| 09 Tom High | Perc/Toms | `main.wav`, `stems/percussion_toms.wav`, `identity/09_tom_high.wav` |  |  |  |  |  |  |  |  |
| 10 Crash | Hats | `main.wav`, `stems/hats_cymbals.wav`, `identity/10_crash.wav` |  |  |  |  |  |  |  |  |
| 11 FX | FX | `main.wav`, `stems/fx.wav`, `identity/11_fx.wav` |  |  |  |  |  |  |  |  |

## Verdict global

- Main:
- Stems:
- Identity 00-11:
- Headroom percu:
- Lisibilite mini-mix:
- Decision RC: `GO` seulement si aucune ligne n'est `P0-regression` ou `P1-blocker`.
