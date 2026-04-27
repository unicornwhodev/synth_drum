# ÉTAPE 6 — Analyse approfondie des fins de sons

## 6.1. Architecture des fins de sons (releases/decay) dans le moteur

### Mécanisme de décroissance

Le moteur utilise une décroissance exponentielle pour l'enveloppe d'amplitude :
```cpp
amplitudeDecayCoeff = std::exp(-1.0f / (std::max(0.001f, settings.decaySeconds) * static_cast<float>(sampleRate)));
```

Le decay est appliqué comme multiplicateur à chaque sample — plus le decay est court, plus le coefficient est proche de 0 (décroissance rapide).

### Voice life scale

Chaque modèle de voix a un `lifeScale` qui multiplie le decay pour le lifetime maximum de la voix :
```cpp
maxAgeSamples = static_cast<int>(sampleRate * std::max(k::kMinDecaySec, settings.decaySeconds * getLifeScale()));
```

| Voice Model | lifeScale | Impact sur decay |
|-------------|-----------|------------------|
| Kick | 2.2 | Decay 0.30s → vie max 0.66s |
| Snare | 2.0 | Decay 0.10s → vie max 0.20s |
| Clap | 1.8 | Decay 0.20s → vie max 0.36s |
| Hat | 1.7 | Decay 0.024s → vie max 0.041s |
| PercWood | 2.0 | Decay 0.085s → vie max 0.17s |
| PercMetal | 2.05 | Decay 0.095s → vie max 0.19s |
| Tom | 2.2 | Decay 0.125s → vie max 0.275s |
| Crash | 2.4 | Decay 0.25s → vie max 0.60s |
| FX | 2.0 | Decay 0.09s → vie max 0.18s |

### Threshold de mort

```cpp
constexpr float kAmpDeathThreshold = 0.00025f;  // amplitude below this = silent
constexpr float kNoiseDeathThreshold = 0.0006f; // noise envelope below this = silent
```

La voix s'éteint quand l'amplitude descend en dessous de ces seuils.

---

## 6.2. Analyse des fins de sons par groupe fonctionnel

### A. Fins de sons des Kicks (Pads 0-1)

| Kit | Decay Kick A | pitchDrop | Comportement | Évaluation |
|-----|-------------|-----------|--------------|------------|
| Classique Standard | 0.32s | 6st | Substantial tail with pitch drop | ✅ Musical |
| Classique Tight | 0.22s (0.32*0.68) | - | Shorter tail | ✅ Controlled |
| Classique Open | 0.48s | - | Long tail, reverb enhanced | ✅ Open feel |
| Acoustique Room | 0.38s | 4st | Natural decay | ✅ |
| Acoustique Jazz | 0.22s | 2st | Short, punchy | ✅ Jazz-appropriate |
| Moderne Club | 0.28s | 7st | Punch with sub tail | ✅ Club-ready |
| Moderne Trap | 0.38s | 12st | Very long sub tail | ⚠️ May mud pattern |
| Cinematique Epic | 0.40s | 8st | Epic sub tail | ✅ Cinematic |

**Problème identifié - Moderne Trap** : Decay 0.38s avec pitchDrop 12st crée un kick très long qui peut "muddy" les patterns en tempo rapide. En trap, les kicks sont souvent joués en syncope — le overlap entre le tail du kick et le kick suivant peut créer des problèmes de phase dans le bas-médium.

### B. Fins de sons des Snares (Pad 2)

| Kit | Decay | Noise | Comportement | Évaluation |
|-----|-------|-------|--------------|------------|
| Classique Standard | 0.10s | 0.65 | Snappy with noise tail | ✅ |
| Classique Tight | 0.07s | 0.65 | Very tight | ✅ |
| Acoustique Brush | 0.16s | 0.55 | Soft tail | ⚠️ May lack crack |
| Acoustique Jazz | 0.09s | - | Short, crisp | ✅ Jazz-appropriate |
| Moderne Club | 0.08s | 0.66 | Punchy | ✅ |
| Moderne Trap | 0.10s | 0.66 | Punchy but sustained | ✅ |

**Observation** : Les snares avec decay < 0.10s peuvent manquer de body (decay trop court pour entendre le body resonance). Les snares avec decay > 0.14s peuvent sembler "longues" pour des styles qui nécessitent du "snap".

### C. Fins de sons des Claps (Pad 3)

| Kit | Decay | Comportement | Évaluation |
|-----|-------|--------------|------------|
| Classique Standard | 0.20s | Multi-burst tail | ✅ |
| Acoustique Room | 0.22s | Slightly longer | ✅ |
| Ambient Sparse | 0.28s (level *0.82) | Minimal tail | ✅ Sparse-appropriate |

**Observation** : Le clap a un decay plus long que la snare, ce qui est cohérent avec la nature du son (le clap a plus de reverb inhérente). Cependant, un decay trop long (0.28s+) peut créer un son "washy" peu adapté aux grooves serrés.

### D. Fins de sons des Hats (Pads 4-5)

| Kit | Hat Closed Decay | Hat Open Decay | Comportement | Évaluation |
|-----|-----------------|----------------|--------------|------------|
| Classique Standard | 0.024s | 0.090s | Closed very short, open medium | ✅ |
| Classique Tight | 0.016s | 0.050s | Very tight both | ✅ Tight-appropriate |
| Acoustique Jazz | 0.028s (approx) | 0.14s | Jazz-appropriate | ✅ |
| Moderne Club | 0.020s | 0.080s | Short, metallic | ✅ Club-ready |

**Problème potentiel** : Le decay court des Hat Closed (0.016-0.024s) peut créer une sensation de son "coupé" si le decay arrive avant que le son ne soit naturellement éteint. Pour un hi-hat frappé, le son du métal continue naturellement pendant plusieurs dizaines de ms.

### E. Fins de sons des Percs (Pads 6-7)

| Kit | Perc1 Decay | Perc2 Decay | Comportement | Évaluation |
|-----|------------|------------|--------------|------------|
| Classique Standard | 0.085s | 0.095s | Quick, definition | ✅ |
| Moderne Electro | 0.075s | 0.12s | Perc2 longer | ✅ Electronic-appropriate |

**Observation** : Les decays des Percs (0.075-0.12s) sont cohérents avec la nature percussive. Pas de problème majeur identifié.

### F. Fins de sons des Toms (Pads 8-9)

| Kit | Tom Low Decay | Tom High Decay | Comportement | Évaluation |
|-----|--------------|----------------|--------------|------------|
| Classique Standard | 0.125s | 0.105s | Balanced | ✅ |
| Cinematique Percussion | 0.18s | 0.15s | Longer for impact | ✅ Percussion-appropriate |

**Observation** : Les decays des toms sont corrects pour des éléments de fills et de groove.

### G. Fins de sons du Crash (Pad 10)

| Kit | Decay | Level | Comportement | Évaluation |
|-----|-------|-------|--------------|------------|
| Classique Standard | 0.25s | 0.46 | Substantial crash | ✅ |
| Ambient Dark | 0.60s | 0.40 | Long, ambient | ✅ Dark-appropriate |
| Moderne Electro | 0.22s | 0.44 | Shorter crash | ✅ Electronic |

**Observation** : Les crashes ont des decays variés (0.22-0.60s), ce qui est pertinent pour différencier les styles. Pas de problème majeur.

### H. Fins de sons du FX (Pad 11)

| Kit | Decay | FM params | Comportement | Évaluation |
|-----|-------|-----------|--------------|------------|
| Classique Standard | 0.09s | pitchDrop 9st, cutoff 4000 | Sweep down | ✅ Texture |
| Moderne Electro | 0.07s | Different | Fast texture | ✅ Electronic |

---

## 6.3. Problèmes de fins de sons identifiés

### Problème #F1 : Kick Trap trop long pour contexte trap (KIT-SPECIFIQUE)
- **Kit** : Moderne Trap
- **Niveau** : Kit
- **Problème** : Decay 0.38s + pitchDrop 12st = tail très long qui peut créer des interférences en patterns trap
- **Impact** : 6/10
- **Conclusion** : Decay devrait être réduit à 0.25s maximum pour maintain groove clarity

### Problème #F2 : Snare Acoustique Brush trop douce pour groove (KIT)
- **Kit** : Acoustique Brush
- **Niveau** : Kit
- **Problème** : Decay 0.16s avec noise réduit (0.55) et click très faible (0.02) crée une snare sans attack ni body
- **Impact** : 5/10
- **Conclusion** : Manque de "crack" rend la snare presque inutile en contexte groove

### Problème #F3 : Hat Closed decay potentiellement trop court (SYSTÉMIQUE)
- **Niveau** : Global/Presets
- **Problème** : Decay 0.016s pour Hat Closed crée une sensation de son "coupé" unnatural
- **Impact** : 4/10
- **Conclusion** : Le decay minimum devrait être revu — un hi-hat naturel ne "coupe" pas si abruptement

### Problème #F4 : Impact du lifeScale sur les tails (SYSTÉMIQUE)
- **Niveau** : Moteur
- **Problème** : Le lifeScale multiplie le decay — ainsi un kick avec decay 0.30s peut vivre jusqu'à 0.66s. Si le tail du kick en trap n'est pas desired, le lifeScale peut exacerber le problème F1.
- **Impact** : 5/10
- **Conclusion** : La vie de la voix est automatiquement plus longue que le decay — ceci est normal pour permettre au tail de mourir naturellement, mais peut créer des overlaps non désirés.

---

## 6.4. Analyse du naturel des fins de sons

### Ce qui fonctionne bien

1. **Kicks avec pitch drop** : Le pitch drop crée un "punch" naturel suivi d'un tail qui descend en fréquence — c'est physiquement accurate pour les kicks acoustiques.

2. **Clap multi-burst** : La structure multi-burst avec decays croissants crée un clap qui "résonne" naturellement.

3. **Percs modal resonator** : Le body resonator crée des attaques et fins naturelles avec résonance harmonnique.

4. **Decay variable** : Les presets utilisent des decays variés de manière pertinente (Tight vs Open vs Standard).

### Ce qui ne fonctionne pas bien

1. **Snare "Brush" sans crack** : L'absence de click perceptible dans Acoustique Brush élimine le transient attack, créant une snare thérapeutiquement peu usable.

2. **Hat Closed trop court** : Le decay de 0.016s est irréaliste pour un hi-hat — le son du métal continue naturellement bien au-delà.

3. **Kick Trap overlap** : Le kick avec decay 0.38s dans un pattern trap peut créer des interférences低频.

---

**Conclusion étape 6** : Les fins de sons sont généralement correctes au niveau du moteur. Les problèmes principaux sont : (1) Snare Brush trop douce, (2) Hat Closed decay trop court créant une coupure unnatural, et (3) Kick Trap trop long pour le contexte trap. Le problème le plus critique est la snare Brush qui est presque inutilisable en groove réel.