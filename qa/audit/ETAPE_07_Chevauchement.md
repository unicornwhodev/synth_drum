# ÉTAPE 7 — Analyse approfondie du chevauchement des sons joués

## 7.1. Architecture de superposition dans le moteur

### Mécanisme de polyphonie

Le moteur gère jusqu'à 72 voix actives (8 voix par modèle × 9 modèles). Chaque voix est indépendante et peut se chevaucher avec d'autres.

**Problème potentiel** : Avec 72 voix max et 12 pads, si tous les pads sont joués rapidement et avec des decays longs, les voix peuvent s'accumuler et créer de la confusion spectrale.

### Choke groups

Les hats partagent un choke group (groupe 1) :
```cpp
// Hat Closed et Hat Open — quand l'un est joué, l'autre est coupé
```

**Comportement** : Si Hat Open est en train de sonner et Hat Closed est joué, Hat Open s'éteint immédiatement. C'est un comportement réaliste pour les hi-hats.

**Absence de choke pour les autres pads** : Kick, Snare, Clap, Percs, Toms, Crash, FX n'ont pas de choke groups — ils peuvent sonner simultanément sans s'êteindre mutuellement.

---

## 7.2. Analyse du chevauchement en patterns lents

### Scenario : Tempo 90 BPM, pattern simple (kick-snare-hat)

À 90 BPM, chaque beat = 667ms.

| Element | Decay | Chevauchement |
|---------|-------|---------------|
| Kick | 0.32s | Fin du kick à ~320ms — avant le next beat (667ms) — pas de chevauchement |
| Snare | 0.10s | Fin de la snare à ~100ms — très avant le prochain beat — pas de chevauchement |
| Hat Closed | 0.024s | Fin quasi-immédiate — pas de chevauchement |
| Hat Open | 0.09s | Fin à ~90ms — pas de chevauchement |

**Conclusion pattern lent** : En tempo lent, le chevauchement nest pas un problème — les decays courts permettent aux éléments de finir avant le prochain coup.

---

## 7.3. Analyse du chevauchement en patterns rapides

### Scenario : Tempo 140 BPM, pattern complexe (groove 8th notes)

À 140 BPM, chaque 8th note = 214ms.

| Element | Decay | Comportement à 140 BPM |
|---------|-------|------------------------|
| Kick (Moderne Club) | 0.28s | Chevauche avec le siguiente kick si rythme 8th — probleme potentiel |
| Snare (Moderne Club) | 0.08s | Fin à 80ms — OK |
| Hat Closed | 0.020s | Fin à 20ms — OK |
| Hat Open | 0.080s | Fin à 80ms — OK |

**Problème identifié** : En pattern 8th notes sur kick (tous les 214ms), un kick avec decay 0.28s va chevaucher le suivant. Le kick suivant attack à ~214ms, mais le kick précédent a encore une amplitude significative (non éteint).

### Calcul du chevauchement

Pour un kick avec decay 0.28s à 140 BPM (214ms entre hits) :
- Après 214ms, l'amplitude du kick initial est : `exp(-214ms / 280ms)` = exp(-0.76) = 0.47
- L新的 kick attaque à 1.0
- Le premier kick a encore ~47% d'amplitude — mélangeadditive

**Conséquence auditive** : Le kick va sembler "squashed" ou "smears" — la densité low-end augmente mais la clarté diminue.

---

## 7.4. Analyse du chevauchement par style

### A. Patterns trap (Tempo 60-70, 808 kick)

| Element | Decay | Comportement |
|---------|-------|--------------|
| Kick (Trap) | 0.38s | Chevaauche heavily avec snare ou suivant kick |
| Snare | 0.10s | OK — assez courte |
| Hats | 0.016-0.055s | OK |

**Problème** : Le kick 808 avec decay 0.38s crée un low-end "wash" en contexte trap. Le kick suivant (souvent à 1 beat dinterval) va se mezcl avec le tail du précédent.

### B. Patterns house/techno (Tempo 124-130)

| Element | Decay | Comportement |
|---------|-------|--------------|
| Kick (Club) | 0.28s | Déc高端 à 124 BPM (8th = 242ms) = 47% remaining |
| Snare | 0.08s | OK |
| Hat Open | 0.08s | OK |

**Problème modéré** : Le kick de club (0.28s) à 124 BPM laisse ~47% d'amplitude au siguiente kick — cela crée un low-end plus dense mais peut être désirable pour "four-on-the-floor".

### C. Patterns jazz (Tempo 120-140)

| Element | Decay | Comportement |
|---------|-------|--------------|
| Kick (Jazz) | 0.22s | OK — relativement court |
| Snare | 0.09s | OK |
| Hat Open | 0.14s | OK — plus long open hat |

**Observation** : Les presets Jazz ont des decays courts, ce qui minimise le chevauchement. Cohérent avec le style.

---

## 7.5. Accumulation spectrale et résiduels

### Problème #C1 : Accumulation low-end en patterns rapides (SYSTÉMIQUE)

En patterns rapides avec kicks à decay long (Trap, Deep House), les tails de kicks s'accumulent, créant un "wash" low-end qui peut :
- Masquer la clarté du kick suivant
- Créer des interférences de phase
- Causer de la fatigue auditive

**Gravité** : 7/10
**Impact utilisateur** : 8/10
**Impact musical** : 7/10

### Problème #C2 : Résidus de hats en rolls (SYSTÉMIQUE)

Les Hat Open avec decay long (0.14s) peuvent créer des résidus qui se mélangent avec les Hat Closed suivants, créant une confusion dans les rolls.

**Gravité** : 4/10
**Impact utilisateur** : 5/10
**Impact musical** : 4/10

### Problème #C3 : Percs mask snare en jeu simultané (SYSTÉMIQUE)

Perc1 (0.92-1.00) et Perc2 (0.90-0.96) avec snare (0.68-0.76) en jeu simultané — les Percs dominent car level supérieur.

**Gravité** : 8/10
**Impact utilisateur** : 9/10
**Impact musical** : 8/10

Ce problème combine chevauchement et cohérence de kit — c'est le même problème identifié dans l'étape 4.

---

## 7.6. Gestion du sustain/release par le moteur

### Absences de Release controls

Le moteur utilise uniquement decay (qui controle le temps de décroissance) — il n'y a pas de contrôle de "release" séparé comme dans un synthé ADSR classique.

```cpp
// amplitudeDecayCoeff = exp(-1 / (decaySeconds * sampleRate))
// Pas de release phase distincte
```

**Conséquence** : Tous les sons décroissent exponentiellement à partir du decay. Il n'y a pas de sustain phase (maintained amplitude) ou de release distinct (rampe de descente après la fin du son).

**Cela peut créer** :
- Des sons qui semblent "coupés" si le decay est trop court
- Pas de natural sustain pour les sons qui devraient avoir une phase sustain (ex: cymbales)

### Crash et sustain

Les crashes ont des decays longs (0.22-0.60s) ce qui simule le sustain naturel d'une cymbale. Cependant, sans release control, la décroissance est toujours exponentielle — pas de plateau sustain.

---

## 7.7. Conclusion sur le chevauchement

| Pattern | Problème principal | Gravité |
|---------|-------------------|---------|
| Trap (tempo bas) | Kick 808 trop long, accumulation low-end | 7/10 |
| House/Techno | Kick club decay 0.28s, accumulation modérée | 5/10 |
| Jazz (tempo haut) | Pas de problème — decays courts | 2/10 |
| Dense/fast patterns | Problèmes generic avec overlaps | 7/10 |

**Recommandations** :
1. Réduire le decay du Kick dans les kits Trap (de 0.38s à 0.25s max)
2. Considérer un contrôle de release distinct pour améliorer les tails
3. Réduire les levels Perc pour éviter masquage snare

---

**Conclusion étape 7** : Le chevauchement est bien géré pour les patterns lents mais pose des problèmes pour les patterns rapides avec kicks à decay long. Le problème le plus critique est l'accumulation low-end en Trap et le masquage Perc/Snare. Le moteur n'a pas de contrôle de release distinct, ce qui peut créer des tails peu naturels pour certains sons.