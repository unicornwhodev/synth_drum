# Listening Files

Base audio source: `qa/drum_release_suite`.

Dans le bundle RC, utiliser la copie locale:

```text
audition_evaluation/audio/
```

## Global

| Ordre | Fichier | Objectif | Criteres prioritaires |
|---:|---|---|---|
| 1 | `main.wav` | Valider le groove RC complet 8 mesures a 90 BPM. | headroom, lisibilite mix, transitions, fatigue |

## Stems

| Ordre | Fichier | Groupe | Objectif | Criteres prioritaires |
|---:|---|---|---|---|
| 2 | `stems/core_kick_snare.wav` | Core | Kick A/B, snare, clap. | punch, backbeat, role Kick B, niveau relatif |
| 3 | `stems/hats_cymbals.wav` | Hats/Cymbals | Hats repetes, choke, crash. | repetition, agressivite, decay, choke |
| 4 | `stems/percussion_toms.wav` | Perc/Toms | Perc 1/2 et tom fills. | attaque, pitch modal, lisibilite fill |
| 5 | `stems/fx.wav` | FX | Impacts et transitions synthetiques. | utilite musicale, decay, non-masquage |

## Identity 00-11

| Ordre | Fichier | Pad | Objectif | Criteres prioritaires |
|---:|---|---|---|---|
| 6 | `identity/00_kick_a.wav` | Kick A | Fondation grave. | pitch/weight, attaque, decay |
| 7 | `identity/01_kick_b.wav` | Kick B | Accent ou layer alternatif. | distinction Kick A, role clair |
| 8 | `identity/02_snare.wav` | Snare | Backbeat principal. | snap, body, tail |
| 9 | `identity/03_clap.wav` | Clap | Couche noise/multi-burst. | attaque, largeur, non-redondance snare |
| 10 | `identity/04_hat_closed.wav` | Hat Closed | Repetitions rapides. | transient, agressivite, longueur |
| 11 | `identity/05_hat_open.wav` | Hat Open | Ouverture et choke. | queue, interaction closed hat |
| 12 | `identity/06_perc_1.wav` | Perc 1 | Percussion modale. | pitch, attaque, niveau relatif |
| 13 | `identity/07_perc_2.wav` | Perc 2 | Percussion metal/alt. | differenciation Perc 1 |
| 14 | `identity/08_tom_low.wav` | Tom Low | Fill grave. | pitch, decay, non-masquage kick |
| 15 | `identity/09_tom_high.wav` | Tom High | Fill medium. | attaque, decay, transition tom low |
| 16 | `identity/10_crash.wav` | Crash | Accent cymbale. | decay, largeur, fatigue |
| 17 | `identity/11_fx.wav` | FX | Transition/impact synthetique. | utilite, tail, niveau |

## Ordre de decision

1. Si `main.wav` echoue: isoler par stem.
2. Si un stem echoue: isoler par identity.
3. Si une identity echoue: noter le pad en `P1-blocker` ou `P0-regression` selon la gravite.
4. Si tout passe avec reserve mineure: noter `P2`.
