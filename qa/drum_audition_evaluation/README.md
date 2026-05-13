# Drum Audition Evaluation

But: fournir un protocole d'ecoute reproductible pour valider une RC `UWdeVST_Drum`.

Sources a utiliser:

- `qa/drum_release_suite/main.wav`
- `qa/drum_release_suite/stems/*.wav`
- `qa/drum_release_suite/identity/*.wav`
- `qa/drum_release_suite_report.csv`
- `qa/drum_release_listening_report.md`

Procedure:

1. Lire `LISTENING_FILES.md` pour l'ordre d'ecoute.
2. Noter chaque critere avec `SCORING_GRID.md`.
3. Reporter les notes et verdicts dans `qa/drum_release_listening_report.md`.
4. Bloquer la RC si une ligne contient `P0-regression` ou `P1-blocker`.
5. Autoriser `P2` seulement si le probleme est documente et non bloquant en contexte.
