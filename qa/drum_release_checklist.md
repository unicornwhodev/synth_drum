# Drum Release Checklist

Produit vise : `UWdeVST_Drum` comme drum synth hybride leger / beat sketch / production synthetique.

Hors promesse release : remplacement premium de Battery, Superior Drummer ou batterie acoustique multi-sample realiste.

## Gates automatiques

| Gate | Commande | Cible release |
|---|---|---|
| Tests production | `.\build\UWdeVST_drum_tests_artefacts\Release\UWdeVST_drum_tests.exe` | `OK` |
| Presets factory | `.\build\UWdeVST_drum_renderer_artefacts\Release\UWdeVST_drum_renderer.exe --validate-presets --report .\qa\drum_preset_qa_report.csv` | `2070/2070 passed`, aucun `status=FAIL` |
| Matrix sonore | `.\build\UWdeVST_drum_renderer_artefacts\Release\UWdeVST_drum_renderer.exe --validate-matrix` | `90/90` |
| CPU | `.\build\UWdeVST_drum_renderer_artefacts\Release\UWdeVST_drum_renderer.exe --benchmark --report .\qa\drum_cpu_benchmark.csv` | peak CPU `< 5%` |
| Groove RC | `.\build\UWdeVST_drum_renderer_artefacts\Release\UWdeVST_drum_renderer.exe --render-release-suite --output-base .\qa\drum_release_suite --report .\qa\drum_release_suite_report.csv` | `17/17 checks passed`, Main + 4 stems + 12 identity |

## Gates de niveau

| Critere | Seuil |
|---|---|
| Peak kit master-ready | `<= -0.3 dBFS` |
| Coherence `nominalPeakDb` | ecart mesure `<= 1.5 dB` |
| Limiter factory | active sur chaque kit factory |
| Seuil limiter factory | plafonne a `-1.0 dBFS` ou plus bas |
| Metadata preset | `familyLabel`, `mixRole`, `description`, `outputProfile`, `nominalPeakDb`, `tags` complets |

## Ecoute P1 avant RC

| Scenario | Kits/pads a couvrir | Verdict attendu |
|---|---|---|
| Groove lent | Kick A, Snare, Hat Closed, Hat Open | backbeat stable, choke audible, pas de queue envahissante |
| Groove rapide | hats repetes, Kick B en accent/layer | repetitions lisibles, pas de flou metallique excessif |
| Fill | Tom Low, Tom High, Perc 1, Perc 2 | attaque claire, niveau coherent avec snare/kick |
| Cinematic dense | `Cinematique Epic`, `Cinematique Tension`, `Cinematique Percussion` | impact sans clip, tails controlees |
| Mini-mix bass/drum | kit moderne + basse simple | kick lisible, snare presente, pas de masquage excessif |

Verdicts autorises : `OK`, `P2`, `P1-blocker`, `P0-regression`.

Passage RC interdit si une ligne contient `P0-regression` ou `P1-blocker`.

Le detail des fichiers et de la notation est dans:

```text
qa/drum_audition_evaluation/
qa/drum_release_listening_report.md
```

## Packaging RC

- Version RC courante: `1.0.1`.
- Installer Windows non signe accepte pour validation interne:

```powershell
.\_package_inno.ps1 -Configuration Release -RunTests -AppVersion 1.0.1
```

- Artefact attendu:

```text
installer\output\UWdeVST_Drum_1.0.1_Windows_x64_Setup.exe
```

- Bundle local non versionne attendu:

```text
release_candidate\UWdeVST_Drum_1.0.1_RC\
```

- La RC reste bloquee tant que `qa\drum_release_listening_report.md` n'est pas rempli sans `P0-regression` ni `P1-blocker`.
