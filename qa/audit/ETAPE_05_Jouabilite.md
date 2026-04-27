# ÉTAPE 5 — Analyse approfondie de la jouabilité

## 5.1. Analyse de la jouabilité au niveau global

### Architecture de réponse à la vélocité

Le moteur DrumSynthVoice implémente la vélocité de manière différentiée selon les paramètres :

```cpp
// DrumSynthVoice.cpp - Velocity application
settings.cutoffHz = juce::jlimit(120.0f, 18000.0f,
    settings.cutoffHz * (k::kVelCutoffBase + k::kVelCutoffRange * velocity));
// kVelCutoffBase = 0.65, kVelCutoffRange = 0.35
// → cutoff × (0.65 + 0.35 × vel)
// → vel 0 = cutoff × 0.65, vel 1.0 = cutoff × 1.0
```

**Mécanisme** : La vélocité module le cutoff du filtre — vélocité haute = son plus brillant.

Pour le pitch drop :
```cpp
if (settings.pitchDropSemitones > k::kPitchDropVelThreshold)
// kPitchDropVelThreshold = 3.0 semitones
    settings.pitchDropSemitones = juce::jlimit(0.0f, 48.0f,
        settings.pitchDropSemitones * (k::kPitchDropVelBase + k::kPitchDropVelRange * velocity));
// kPitchDropVelBase = 0.78, kPitchDropVelRange = 0.22
// → pitch drop scales with velocity if > 3st
```

**Observation** : Le pitch drop ne responde à la vélocité que si > 3 semitones. Pour les kicks avec pitchDrop < 3st, la vélocité n'affecte pas le drop — seulement le cutoff.

---

### Modes de courbe de vélocité disponibles

Le code montre dans `drum_parameters_inventory.md` :
- Linear
- Soft
- Softer
- Hard
- Harder
- Fixed
- Touch

Ces courbes sont censées modifier la réponse entre input MIDI velocity et la vélocité effective du moteur.

**Constat** : Cette flexibility est positive pour adapter le comportement aux préférences de jeu.

---

## 5.2. Analyse de la jouabilité par famille de pads

### A. Jouabilité des Kicks (Pads 0-1)

| Aspect | Observation | Évaluation |
|--------|-------------|------------|
| Rapidité de déclenchement | Attack 0.0003-0.0006s (très rapide) | ✅ Excellent |
| Réponse à la vélocité | Cutoff modulé + pitchDrop au-delà de 3st | ✅ Bonne |
| Comportement en répétition | Decay 0.14-0.60s, pas de limitation visible | ✅ Correct |
| Punch cohérent | pitchDrop crée le punch | ✅ Adequat |
| Fatigue de jeu | Pas de limite de polyphonie sur kicks | ✅ Pas de problème |

**Problèmes identifiés** :
- Kick avec pitchDrop < 3st (ex: Acoustique Jazz avec 2st) n'a pas de dynamique de pitch — la vélocité n'affecte que le cutoff
- Kick Moderne Trap avec 12st de pitchDrop peut créer des sous-basses très longues (0.38s) qui "smear" en jeu rapide

---

### B. Jouabilité de la Snare (Pad 2)

| Aspect | Observation | Évaluation |
|--------|-------------|------------|
| Rapidité de déclenchement | Attack 0.0002-0.0003s | ✅ Excellent |
| Réponse à la vélocité | Cutoff modulé, clickAmount statique | ⚠️ ClickAmount non modulate par vélocité |
| Comportement en répétition | Decay 0.08-0.16s, correct | ✅ |
| Clarté en jeu rapide | Dépend du kit — Brush très doux | ⚠️ Variable |
| Cohérence Snare/Kick | Level Snare 0.68-0.76 vs Kick 0.86-0.90 | ✅ Correct |

**Problème identifié** : Le clickAmount n'est pas modulé par vélocité (c'est confirmé dans le code `settings.clickAmount = juce::jlimit(...)` après le calcul de vélocité — le click n'est pas velocity-scaled). Cela signifie que le transient attack est identique que la velocity soit 0.1 ou 1.0. Pour une snare, cela peut créer un son "tap-like" peu naturel quand velocity faible.

---

### C. Jouabilité du Clap (Pad 3)

| Aspect | Observation | Évaluation |
|--------|-------------|------------|
| Rapidité de déclenchement | Attack 0.0002s | ✅ Excellent |
| Multi-burst behavior | Le NoiseBurst génère plusieurs reverberated hits | ✅ Intéressant |
| Réponse en répétition | Decay 0.18-0.28s | ✅ Correct |
| Cohérence avec Snare | Clap souvent utilisé comme layer ou alternative | ✅ Correct |

**Observation** : Le clap n'a pas de body resonance (bodyResonator feedback = 0.0 selon DrumDefs.h), ce qui peut le rendre "thin" en contexte. Le clap est souvent utilisé comme "layer" avec snare pour ajouter de la颗粒感 — mais sans body, il peut manquer de chaleur.

---

### D. Jouabilité des Hats (Pads 4-5)

| Aspect | Observation | Évaluation |
|--------|-------------|------------|
| Rapidité de déclenchement | Attack 0.0s (instant) | ✅ Excellent |
| Choke group | Hat Closed/Open shared choke group 1 | ✅ Réaliste |
| Comportement en répétition | Decay très court (0.016-0.028s closed) | ✅ Correct |
| Hi-hat rolls | Decay courts permettent rolls速 | ✅ Good |
| MetallicDensity | Computed depuis cutoff — controle la raideur | ✅ |

**Problèmes identifiés** :
- clickAmount présent (0.08-0.12) mais non-naturaliste pour hats acoustiques
- Open hat avec decay long (0.055-0.16s) peut créer des overlaps en patterns denses

---

### E. Jouabilité des Percs (Pads 6-7)

| Aspect | Observation | Évaluation |
|--------|-------------|------------|
| Modal resonance | Body resonance crée attack naturel | ✅ Excellent |
| BodyTone level | PercWood 0.30, PercMetal 0.68 — bois plus léger | ✅ Correct |
| Rapidité de déclenchement | Attack 0.0001s | ✅ Excellent |
| Répétition | Decay 0.075-0.13s | ✅ Correct |

**Observation** : Les Percs ont le meilleur comportement d'attaque (attack 0.0001s) après les hats (0.0s). Le modal resonator crée une attaque très naturelle.

---

### F. Jouabilité des Toms (Pads 8-9)

| Aspect | Observation | Évaluation |
|--------|-------------|------------|
| Tom Low (175Hz) | Attack 0.0004s, decay 0.11-0.18s | ✅ Correct |
| Tom High (250Hz) | Très proche de Snare (248Hz) | ⚠️ Risque de masque |
| Comportement en fills | Decay longs supports fills | ✅ |
| Rapidité | Correct pour toms | ✅ |

**Problème identifié** : Tom High (250Hz) est si proche de Snare (248Hz) qu'en contexte de groove, jouer Tom High après Snare peut créer un "double snare" non intentionnel. C'est un problème de design structurel.

---

### G. Jouabilité du Crash (Pad 10)

| Aspect | Observation | Évaluation |
|--------|-------------|------------|
| Attack | Pas de click — metallic partials attack | ✅ Cohérent |
| Decay | 0.22-0.60s selon kit | ✅ |
| Choke behavior | Pas de choke group — sustain complet | ✅ |

---

### H. Jouabilité du FX (Pad 11)

| Aspect | Observation | Évaluation |
|--------|-------------|------------|
| FM synthesis | Attack par FM modulation | ✅ Intéressant |
| Textures | FM index/sweep génèrent sweeps | ✅ |
| Cohérence avec usage | FX pour impacts, risers, textures | ✅ |

---

## 5.3. Analyse de la jouabilité par famille de kits

### A. Jouabilité en contexte Classique

| Kit | Jouabilité globale | Points faibles |
|-----|-------------------|---------------|
| Classique Standard | Correcte | Perc > Snare peut perturber hiérarchie en jeu |
| Classique Tight | Bonne — decays courts = jeu précis | - |
| Classique Open | Correcte | Reverb importante peut afectar clarté |

### B. Jouabilité en contexte Acoustique

| Kit | Jouabilité globale | Points faibles |
|-----|-------------------|---------------|
| Acoustique Room | Correcte | Snare peut être trop douce pour groove energy |
| Acoustique Studio | Bonne — snare plus présente | - |
| Acoustique Brush | Délicate — snare très légère | Risque de manque de réponse |
| Acoustique Jazz | Bonne — decays courts, hats légers | - |

### C. Jouabilité en contexte Moderne

| Kit | Jouabilité globale | Points forts |
|-----|-------------------|--------------|
| Moderne Club | Excellente — hats metalliques, kicks punchy | Punchy, clair |
| Moderne Trap | Excellente — kick 808 ideal pour trap | - |
| Moderne Electro | Excellente — chorus/delay ajoutent texture | - |
| Moderne Lo-Fi | Bonne — karakter | - |

---

## 5.4. Problèmes de jouabilité transversaux

### Problème #J1 : Tom High masqué par Snare (STRUCTUREL)
- **Niveau** : Structure 12 pads
- **Zone** : Jouabilité / Cohérence freq
- **Problème** : Tom High (250Hz) trop proche de Snare (248Hz) crée confusion en jeu
- **Impact musical** : 6/10
- **Conclusion** : Probleme structurel — corrigible uniquement par modification de la fréquence de base

### Problème #J2 : ClickAmount non modulé par vélocité (SYSTÉMIQUE)
- **Niveau** : Moteur
- **Zone** : Jouabilité / Réponse dynamics
- **Problème** : Le click d'attaque ne responde pas à la vélocité, ce qui peut créer des attaques "tap-like" à faible velocity
- **Impact musical** : 5/10
- **Conclusion** : Amélioration possible dans le moteur

### Problème #J3 : Perc dominant sur Snare en jeu réel (SYSTÉMIQUE)
- **Niveau** : Preset/kit
- **Zone** : Jouabilité / Hiérarchie
- **Problème** : Perc level 0.92-1.00 vs Snare 0.68-0.76 — Percs sonnent plus fort que Snare en jeu
- **Impact musical** : 8/10
- **Conclusion** : Critique — affecte la hiérarchie rythmique fondamentale

### Problème #J4 : Kick Trap trop long pour jeu rapide (KIT)
- **Niveau** : Kit spécifique (Moderne Trap)
- **Zone** : Jouabilité / Overlap
- **Problème** : Decay 0.38s pour kick crée overlap en patterns rapides
- **Impact musical** : 5/10
- **Conclusion** : Utilization typique trap (tempo bas, patterns denses) rend ce problème worst

---

## 5.5. Conclusion de jouabilité

| Aspect | Score global | Commentaire |
|--------|-------------|-------------|
| Rapidité de déclenchement | 9/10 | Attaques très rapides sur tous les pads |
| Réponse à la vélocité | 7/10 | Bonne mais clickAmount non modulate |
| Cohérence pad à pad | 6/10 | Tom High/Snare conflict, Perc dominates Snare |
| Jouabilité en patterns | 7/10 | Correct mais problèmes de hiérarchie |
| Finger drumming | 7/10 | Decays courts便利ent rolls, mais choke groups limitent |
| Score global | 7.2/10 | Jouabilité correcte mais problèmes de hiérarchie affectent l'expérience |

**Recommandation critique** : Le problème Perc > Snare doit être corrigé pour améliorer la jouabilité. Un utilisateur qui joue un groove sur un kit "Classique" entendra les Percs dominer la Snare — ce qui est musicalement incorrect pour la plupart des styles.