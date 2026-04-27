# ÉTAPE 2 — Cartographie des 25 kits et des 12 pads

## 2.1. Cartographie des 25 kits

### A. Famille Classique (3 kits)

| ID | Nom | Rôle implicite | Usage probable | Place dans la gamme | Risque de redondance | Risque d'ambiguïté |
|----|-----|---------------|---------------|---------------------|---------------------|-------------------|
| C1 | Classique Standard | Foundation | Grooves彈basic, rock/pop classique | Kit de référence Classique | - | - |
| C2 | Classique Tight | Compact | Funk, R&B, nécessitant précision | Déclinaison Short decay | Faible vs Standard (decay différent) | "Tight" vs "Standard" non évident |
| C3 | Classique Open | Ouvert | Ballade, ambient, espace | Déclinaison Long decay/reverb | Faible vs Standard (reverb différent) | "Open" pourrait être confondu avec "Ambient" |

**Analyse redondance Classique** : Les 3 kits sont suffisamment différenciés par decay et reverb. Cependant, le passage de "Standard" à "Open" manque de nuance intermédiaire (par exemple, "Half-Open" ou "Room").

---

### B. Famille Acoustique (4 kits)

| ID | Nom | Rôle implicite | Usage probable | Place dans la gamme | Risque de redondance | Risque d'ambiguïté |
|----|-----|---------------|---------------|---------------------|---------------------|-------------------|
| A1 | Acoustique Room | Naturel/Organique | Pop rock, enregistrements home studio | Point d'entrée Acoustique | - | Ambiance "Room" vs "Studio" non distincte |
| A2 | Acoustique Studio | Mix-ready | Production professionnelle, clarté | Version optimisée Room | Moyen (Room vs Studio proches) | "Studio" implique high-end, livraison insuffisante |
| A3 | Acoustique Brush | Soft/Brush | Jazz, ballade, musique douce | Version douce de Room | Moyen vs Studio (soft vs precise) | "Brush" demande samples acoustic réels, synthétique moins crédible |
| A4 | Acoustique Jazz | Airy | Jazz, fusion, légèreté | Version light de Room | Faible (decay + reverb différents) | Cohérent avec promesse Jazz |

**Analyse redondance Acoustique** : 4 kits pour une même famille = risque de confusion. "Room" vs "Studio" vs "Brush" vs "Jazz" = 4 déclinaisons d'un même concept (acoustique) avec différences subtiles. La différenciation decay/reverb peut ne pas suffire à justifier 4 presets distincts sans variation de sample ou de traitement.

---

### C. Famille Ambient (3 kits)

| ID | Nom | Rôle implicite | Usage probable | Place dans la gamme | Risque de redondance | Risque d'ambiguïté |
|----|-----|---------------|---------------|---------------------|---------------------|-------------------|
| AM1 | Ambient Pad | Wash/Textured | Cinematic, trailer, ambient | Introduction Ambient | - | "Pad" implies long sustain, cohérent |
| AM2 | Ambient Dark | Dark/Deep | Thriller, horror, tension | Version sombre Pad | Faible vs Pad | Risque "dark" = simple pitch-shift, pas caractère distinct |
| AM3 | Ambient Sparse | Minimal | Minimalism, art rock, experimental | Version minimaliste Pad | Faible vs Pad (levels réduits) | Cohérent avec promesse Sparse |

**Analyse redondance Ambient** : 3 kits bien différenciés par niveau d'énergie (Pad > Dark > Sparse). Cohérence correcte.

---

### D. Famille Cinématique (4 kits)

| ID | Nom | Rôle implicite | Usage probable | Place dans la gamme | Risque de redondance | Risque d'ambiguïté |
|----|-----|---------------|---------------|---------------------|---------------------|-------------------|
| CI1 | Cinematique Epic | Wide/Grand | Epic music, trailer, action | Flagship Cinématique | - | - |
| CI2 | Cinematique Tension | Aggressive | Thriller, tension, horror | Version agressive Epic | Moyen (diff drive/compression) | "Tension" vs "Epic" pourraient être inversés |
| CI3 | Cinematique Hybrid | Electronic/Acoustic blend | Cross-genre, modern scoring | Version Hybride Epic | Faible (delay enabled) | Cohérent avec proposition |
| CI4 | Cinematique Percussion | Perc-forward | Percussion ensemble, world percussive | Version Percussion Epic | Moyen (perc levels différents) | Peut résoudre un problème de hiérarchie |

**Analyse redondance Cinématique** : 4 kits solides, bien différenciés. "Percussion" est une variante pertinente et manquait potentiellement dans d'autres familles.

---

### E. Famille Moderne (4 kits)

| ID | Nom | Rôle implicite | Usage probable | Place dans la gamme | Risque de redondance | Risque d'ambiguïté |
|----|-----|---------------|---------------|---------------------|---------------------|-------------------|
| M1 | Moderne Club | Punchy/Club | House, techno, club music | Entry-point Moderne | - | - |
| M2 | Moderne Lo-Fi | Textured/Dirty | Lo-fi hip hop, chill, vaporwave | Version Lo-Fi Club | Moyen (EQ + saturation différents) | Lo-Fi pourrait être plus qu'un simple EQ - mais semble correct |
| M3 | Moderne Trap | Sub/Focused | Trap, hip-hop, 808 | Version 808-focused Club | Moyen (kick très différent) | Cohérent avec promesse 808 |
| M4 | Moderne Electro | Synthetic/Chorused | Electro, IDM, synthwave | Version synth Cluster | Faible (chorus + delay enabled) | Cohérent |

**Analyse redondance Moderne** : 4 kits bien différenciés. La présence de "Trap" et "Electro" dans la même famille que "Club" est cohérente — ce sont des sous-genres du spectre électronique.

---

### F. Vue d'ensemble de la cartographie des kits

| Famille | Nombre kits | Différenciation interne | Trous éventuels | Équilibre familles |
|---------|-------------|------------------------|-----------------|-------------------|
| Classique | 3 | Bonne (Standard/Tight/Open) | Pas de variant "Dry" extrême | Moyen |
| Acoustique | 4 | Moyenne (confusion Room/Studio/Brush) | Pas de variant "Live" ou "Arena" | Moyen |
| Ambient | 3 | Bonne (Pad/Dark/Sparse) | Pas de variant "Evolutive" | Bon |
| Cinématique | 4 | Bonne (Epic/Tension/Hybrid/Percussion) | Pas de variant "Minimal" | Bon |
| Moderne | 4 | Bonne (Club/LoFi/Trap/Electro) | Pas de variant "Industrial" ou "Rave" | Bon |

**Redondances identifiées** :
- Acoustique Room vs Studio : très proches, différenciation FX insuffisante
- Classique Standard vs Open : séparés principalement par reverb, decoys différents

---

## 2.2. Cartographie des 12 pads

### A. Structure pad par pad

| Pad | Nom | Famille | Modèle voix | Fréq. base (Hz) | Choke Group | Rôle dans le kit |
|-----|-----|---------|-------------|-----------------|-------------|------------------|
| 0 | Kick A | Kick | Tonal | 90 | 0 | Kick principal, sub/ground |
| 1 | Kick B | Kick | Tonal | 96 | 0 | Kick secondaire, variation |
| 2 | Snare | Snare | Tonal | 248 | 0 | Snare principal, groove anchor |
| 3 | Clap | Clap | NoiseBurst | 300 | 0 | Clap, layer/snare alternative |
| 4 | Hat Closed | Hat | Metallic | 5500 | 1 | Hi-hat court, timing |
| 5 | Hat Open | Hat | Metallic | 4800 | 1 | Hi-hat ouvert, sustain |
| 6 | Perc 1 | Perc | Modal | 480 | 0 | Perc wood, timbale/cjélé |
| 7 | Perc 2 | Perc | Modal | 650 | 0 | Perc metal, cowbell/metallic |
| 8 | Tom Low | Tom | Tonal | 175 | 0 | Tom grave, fills |
| 9 | Tom High | Tom | Tonal | 250 | 0 | Tom aigu, fills |
| 10 | Crash | Crash | Metallic | 6400 | 0 | Crash, accents |
| 11 | FX | FX | FM | 720 | 0 | FX/texture/atonal |

### B. Analyse de la logique des 12 pads

**Structure positive** :
- ✅ 2 kicks pour variation (sub vs punchy)
- ✅ Snare + Clap séparés (historiquement approprié)
- ✅ 2 hats avec choke group partagé (réaliste)
- ✅ 2 percs avec timbres différents (wood vs metal)
- ✅ 2 toms pour fills complets
- ✅ 1 crash générique
- ✅ 1 FX pour textures/atonal

**Problèmes identifiés** :

| Pad | Problème | Impact |
|-----|----------|--------|
| Kick A vs B | Différence uniquement par niveau (0.88 vs 0.70 dans Classique Standard) et tune (+0 vs +5st). Pas de différence de caractère synthétiquement créée. | Si l'utilisateur switche entre les 2 kicks, la différence est subtile et peut ne pas justifier 2 pads |
| Perc 1 vs 2 | Fréquences 480 Hz vs 650 Hz = pas une octave, donc cohérence harmonique non évidente. Ratio = 1.35 (tierce majeure), ce qui peut créer un cluster avec snare (248 Hz) et clap (300 Hz) |
| FX | Fréquence 720 Hz = très proche de Perc 2 (650 Hz). Ratio 1.11 = quasi-unisson,.clusterage spectral potentiel |
| Tom Low | Fréquence 175 Hz = légèrement en dessous de Snare (248 Hz), cohérence correcte |
| Tom High | Fréquence 250 Hz = très proche de Snare (248 Hz). Ratio 1.008 = quasi-unisson. Peut créer confusion Snare/Tom High |

### C. Vérification de la différenciation des 25 kits pad à pad

L'analyse des FactoryPresets révèle que la différenciation entre kits est réalisé via :
- Level (variable)
- Decay (variable)
- Noise Amount (variable)
- Drive (variable)
- Cutoff (variable)
- FX chain parameters

MAIS les pad parameters fondamentaux (baseFrequencyHz, voiceModel, synthesisMode) sont FIXES par pad. Cela signifie qu'un "Kick A" dans Classique Standard et dans Moderne Trap partagent la même architecture de synthèse — seuls les paramètres diffèrent.

**Conséquence** : Les kits ne peuvent pas créer un Kick totalement différent (par exemple, un 808 sub vs un Kick acoustic classique) — ils ne peuvent qu'ajuster les paramètres d'un même modèle.

---

## 2.3. Identification des trous dans la gamme

### Trous stylistiques identifiés

| Style/Génie manquant | Impact | Severity |
|---------------------|--------|----------|
| Hard Rock / Metal | Kits manquants de "aggressive", "distorted", "overdriven" | Haute |
| Latin / World | Aucune percusión latina (congas, bongos, timbales) au-delà des 2 percs standard | Moyenne |
| Hip-Hop Classic | Pas de variant "Boom bap" avec snare dead, kick punchy | Moyenne |
| Jungle / DnB | Pas de variant "Rolls" ou "Ragga" | Basse |
| Industrial / EBM | Pas de variant "Metallic harsh", "Mechanical" | Moyenne |
| Glitch / IDM | Pas de variant "Fractured", "Glitchy" | Basse |

### Trous fonctionnels identifiés

| Fonction manquante | Impact |
|-------------------|--------|
| Ride cymbal | 12 pads incluent crash mais pas ride. Pour les styles jazz/funk, le ride est essentiel. |
| Splash cymbal | Pas de splashes pour patterns "tréis" |
| China cymbal | Pas de china pourrock/métal |
| Rimshot/Clave | Pas de stroke alternative pour Latin |
| Shaker/Maracas | Pas de элемент pour styles acoustiques |

---

## 2.4. Identification des doublons ou quasi-doublons

| Paire | Similarité | Différenciation | Conclusion |
|------|------------|-----------------|------------|
| Classique Standard vs Classique Open | Décay kick 0.32 vs 0.48, reverbMix 0.16 vs 0.22 | Différences mineures | Quasi-doublon potentiel — le nom "Open" vs "Standard" ne révèle pas assez la différence |
| Acoustique Room vs Acoustique Studio | FX reverbSize 0.52 vs 0.36, reverbMix 0.20 vs 0.13 | Différences mineures | Quasi-doublon — la distinction "Room" vs "Studio" nest pas assez forte pourjustifier 2 presets |
| Moderne Club vs Moderne Trap | Différences principales sur kick (decay 0.28 vs 0.38, pitchDrop 7st vs 12st) | Bon | Différenciation correcte |
| Ambient Pad vs Ambient Dark | Levels réduites pour Dark, pas de différence architecturale | Suffisant | Différenciation correcte |

---

## 2.5. Identification des déséquilibres entre styles

| Famille | Déséquilibre identifié |
|---------|------------------------|
| Classique | 3 kits pour la famille la plus "standard" — cohérent |
| Acoustique | 4 kits = sur-représentation vs Classique (3) si l'objectif est production généraliste |
| Ambient | 3 kits = correct pour niche |
| Cinématique | 4 kits = justifié par polyvalence scoring |
| Moderne | 4 kits = cohérent avec couverture électronique |

**Déséquilibre perçu** : Acoustique (4) vs Classique (3) pourrait indiquer une orientation "production moderne" vs "classic acoustic", mais le nom de famille "Classique" est ambigu (pourrait inclure acoustic).

---

## 2.6. Cohérence de la structure 12 pads / 25 kits

| Critère | Évaluation |
|---------|------------|
| Logique des pads stable de kit en kit | ✅ Les pad IDs sont fixes, les paramètres varient. Cohérence structurelle maintenue. |
| Hiérarchie rythmique cohérente | ⚠️ Les levels varient considérablement (Kick A 0.88 → FX 0.50), créant des déséquilibres non résolus |
| Différenciation kits justifiée | ⚠️ Quasi-doublons identifiés (Classique Standard/Open, Acoustique Room/Studio) |
| Trous dans la gamme | ⚠️ Styles manquants (metal, latin, boom-bap) ≠ couverture "Classique→Techno" complète |
| Stabilité des choke groups | ✅ Choke group 1 pour Hat Closed/Open est réaliste |

---

## 2.7. Solidité de la gamme

| Aspect | Score | Commentaire |
|--------|-------|-------------|
| Différenciation kits | 6/10 | Quasi-doublons affaiblissent la proposition |
| Couverture stylistique | 7/10 | Large mais trous identifiés (metal, latin) |
| Cohérence structure pads | 8/10 | Structure cohérente, quelques problèmes de freq proximité |
| Logique FX globale | 4/10 | Incohérence musicale potentielle (même reverb pour jazz et techno) |
| Score global | 6.25/10 | Gamme correcte mais perfectible, principalement à cause des FX et des doublons |

---

**Conclusion étape 2** : La cartographie révèle 25 kits sur 12 pads avec des forces (structure pads cohérente, familles bien représentées) et des faiblesses (quasi-doublons, FX global incohérent, trous stylistiques). La différenciation kit-par-kit repose sur des ajustements de paramètres plutôt que sur des architectures radicalement différentes, ce qui limite le spectre réel de variation.