# Scoring Grid

Notation par critere: `0` a `5`.

| Note | Sens |
|---:|---|
| 5 | Release-ready, aucun probleme audible en contexte. |
| 4 | Bon, reserve mineure acceptable en RC. |
| 3 | Utilisable mais defaut audible a documenter en `P2`. |
| 2 | Faiblesse musicale nette, `P1-blocker` si le pad est central. |
| 1 | Probleme grave: instable, masquant, agressif ou incoherent. |
| 0 | P0-regression: silence, clip, NaN/Inf audible, mauvais pad, crash, export manquant. |

## Criteres

| Critere | Question a trancher |
|---|---|
| Attaque | Le transient est-il lisible sans click artificiel excessif ? |
| Pitch / centre | Le centre tonal est-il stable quand le pad est tonal ou modal ? |
| Decay / release | La fin de son est-elle naturelle pour le role du pad ? |
| Chevauchement | Les queues restent-elles lisibles en repetitions et en groove ? |
| Lisibilite mix | Le pad reste-t-il identifiable dans `main.wav` sans masquer les autres ? |
| Role musical | Le pad a-t-il un role clair dans la banque 12 pads ? |
| FX | Les traitements soutiennent-ils le son sans compenser un defaut de moteur ? |

## Verdicts

| Verdict | Condition |
|---|---|
| `OK` | Tous les criteres prioritaires sont notes `4` ou `5`. |
| `P2` | Un ou plusieurs criteres a `3`, sans risque release. |
| `P1-blocker` | Un critere prioritaire a `2` ou moins sur un pad central ou un stem. |
| `P0-regression` | Export manquant, silence, clip, NaN/Inf, crash, mauvais mapping, ou test automatique rouge. |
