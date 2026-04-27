# PLAN D'ACTION — Corrections, optimisations & refonte UI
*UWdeVST_drum — basé sur l'audit 17 étapes + lecture du code réel*

> **Règle utilisateur** : paramètres APVTS et knobs **gelés**.
> → Toutes les corrections sonores s'opèrent **dans les presets factory** et **dans le code DSP interne** (constantes, courbes, mappings).
> → Aucun paramètre exposé n'est ajouté/renommé/déplacé.
> → La refonte UI = **layout/visuel/ergonomie uniquement** (mêmes contrôles, mêmes IDs).
> → Section "Dégels proposés" = liste à arbitrer, **non implémentée** par défaut.

---

## 0. Cartographie code → audit

| Audit pointe | Fichier réel concerné | Type d'intervention |
|---|---|---|
| Hiérarchie Perc > Snare | `FactoryPresets.cpp` → `applyTargetMatrix()` (lignes 884-955) | **Recalibrage matrice** |
| FX globaux non utilisés | `FactoryPresets.cpp` → `makeKit_*` (sat/comp/TS = 0) | **Per-kit FX values** |
| EQ flat | `FactoryPresets.cpp` → champs `k.fx` | **Per-kit EQ values** |
| Hat Closed decay 0.016s | `FactoryPresets.cpp` + `DrumConstants.h` | **Floor de decay** |
| Tom High 250 / Snare 248 | `FactoryPresets.cpp` baseFrequencyHz | **Ré-accordage Tom High** |
| Snare Brush inaudible | `makeKit_Acoustique_Brush()` | **Recalibrage local** |
| Kick Trap decay 0.38s | `makeKit_Moderne_Trap()` | **Recalibrage local** |
| Quasi-duplicates kits | Kits Standard/Open, Room/Studio | **Différenciation timbrale** |
| Layout déséquilibré / dense | `PluginEditor.cpp` `resized()` | **Refonte visuelle** |

---

## PHASE 1 — CORRECTIONS CRITIQUES PRESETS (priorité P0/P1)

### 1.1. Recalibrage de `kTargetMatrix` (FactoryPresets.cpp ~L83)

**Problème mesuré** : la matrice cible donne déjà Snare 0.54-0.56 vs Kick 0.78-0.87. Le vrai responsable du « Perc > Snare » est dans `applyTargetMatrix()` lignes 925-934 :

```cpp
const float percLevel = clamp(t.snare.level * 0.35f + t.hat.level * 0.45f + t.fx.level * 0.20f, 0.10f, 1.00f);
```

→ Avec snare 0.55 + hat 0.46 + fx 0.50 = **0.51 nudgé vers 0.92-1.00 du level pad initial** (nudgeLevel mix 70/30 conserve le pad initial trop élevé).

**Action 1.1.a — corriger le mix nudgeLevel pour les Percs** :
```cpp
// Avant (L890-892) — global:
auto nudgeLevel = [](float current, float target) {
    return std::clamp(current * 0.7f + target * 0.3f, 0.10f, 1.00f);
};

// Après — tirer plus fortement vers la cible (60/40 au lieu de 70/30)
auto nudgeLevel = [](float current, float target) {
    return std::clamp(current * 0.55f + target * 0.45f, 0.10f, 1.00f);
};
```

**Action 1.1.b — recalibrer la cible Perc explicite** :
Aujourd'hui Perc dérivé de hat/snare/fx. Forcer **Perc cible = snare cible × 0.85** (toujours sous la snare) :
```cpp
const float percLevel = std::clamp(t.snare.level * 0.85f, 0.10f, 0.85f);
```

**Impact** : Snare ~0.65-0.72, Perc ~0.46-0.48 → hiérarchie restaurée sans toucher à un paramètre exposé.

---

### 1.2. Activation des FX dans chaque kit (FactoryPresets.cpp)

**Constat** : tous les `makeKit_*` initialisent déjà `k.fx.compThreshold/Ratio/Attack/Release/Mix`, `k.fx.satDrive/Mix`, `k.fx.transient*` — donc l'audit "FX = 0%" était partiellement faux côté code, mais **les valeurs sont trop conservatrices** (compMix 0.28-0.35, satMix 0.03-0.06, transientMix 0.10-0.14).

**Action 1.2 — palette FX par famille** (dans chaque `makeKit_*`) :

| Famille | satDrive | satMix | compRatio | compMix | TS attack | TS mix | Reverb size/mix |
|---|---|---|---|---|---|---|---|
| Classique | 1.10 | 0.08 | 2.2 | 0.40 | 0.10 | 0.18 | 0.40 / 0.18 |
| Acoustique | 1.05 | 0.05 | 1.8 | 0.30 | 0.06 | 0.12 | 0.50 / 0.20 |
| Ambient | 1.02 | 0.04 | 1.5 | 0.20 | 0.04 | 0.08 | 0.65 / 0.32 |
| Cinematique | 1.20 | 0.10 | 2.5 | 0.45 | 0.14 | 0.22 | 0.55 / 0.30 |
| Moderne | 1.35 | 0.15 | 3.0 | 0.50 | 0.18 | 0.28 | 0.30 / 0.18 |

→ Ces valeurs sont des **seeds par famille** appliquées dans chaque `makeKit_*` ; les variantes (Tight, Open, Trap…) modulent ensuite.

---

### 1.3. EQ par famille (champs `k.fx.eq*`)

**À vérifier** : les champs EQ sont dans `KitFxSettings` (cf. `FactoryPresets.h`). Si présents, les seeder par famille :

| Famille | Low shelf | Mid bell | High shelf |
|---|---|---|---|
| Classique | +1 dB @ 80 | flat | +1.5 dB @ 8k |
| Acoustique | flat | +0.5 dB @ 2.5k Q 0.7 | +1 dB @ 10k |
| Ambient | -1 dB @ 60 | -2 dB @ 1k Q 1.0 | +2 dB @ 12k |
| Cinematique | +2 dB @ 70 | flat | +2.5 dB @ 9k |
| Moderne | +3 dB @ 60 | -1 dB @ 350 Q 0.8 | +2 dB @ 8k |

→ Si les champs n'existent pas dans `KitFxSettings`, **action différée** (pas un dégel d'API utilisateur, juste extension struct interne — acceptable).

---

### 1.4. Recalibrages locaux (per-kit fixes)

**Acoustique Brush** (snare quasi-inaudible) :
```cpp
k.pads[2] = makePad(0.78f, 5.0f, 0.16f, ..., noise=0.58f, click=0.10f, ...);
// avant: level 0.40 / click 0.02
```

**Moderne Trap** (kick decay 0.38 + drop 12st) :
```cpp
k.pads[0].decaySeconds = 0.26f;        // était 0.38
k.pads[0].pitchDropSemitones = 9.0f;   // était 12 — gain de clarté
```

**Tom High vs Snare collision** :
```cpp
// Tous les kits — pad[9]:
k.pads[9].baseFrequencyHz = 195.0f;   // était 250 — ratio 0.79 vs snare 248
```
> Justification : 195 Hz crée un intervalle musical (≈ tierce mineure inférieure) au lieu d'unisson.

**Hat Closed decay floor** dans `DrumConstants.h` :
```cpp
constexpr float kMinDecaySec = 0.025f;  // était 0.001 — évite "coupé"
```
> Affecte uniquement le clamp interne ; ne change pas le mapping du knob `decay`.

---

## PHASE 2 — DIFFÉRENCIATION DES KITS (priorité P2)

### 2.1. Quasi-duplicates à différencier

| Couple | Action |
|---|---|
| Classique Standard ↔ Open | Open : reverb + drier comp (0.20→0.10), kick decay +50%, pas seulement reverb size |
| Acoustique Room ↔ Studio | Studio : EQ +2 dB high shelf, transient mix 0.18, comp ratio 2.5 (plus "produit") |
| Cinematique Epic ↔ Cinematic | Epic = transients durs (TS 0.20), Cinematic = reverb dominant (mix 0.42) |

### 2.2. Cohérence timbrale par famille

Vérifier que dans chaque `makeKit_*`, le décalage `tuneSemitones` du Kick B vs Kick A est cohérent (Classique +5, Acoustique +3, Moderne -2 typique pour 808). À harmoniser via la matrice cible.

---

## PHASE 3 — DSP / OPTIMISATIONS MOTEUR (priorité P2/P3)

### 3.1. Velocity-modulation du clickAmount (Snare/Hat)

**Sans dégel** : modulation **interne** dans `DrumSynthVoice.cpp` lors du `startNote()` :
```cpp
// Pour Snare/Hat, scaler le click stocké par la vélocité
const float clickVel = settings.clickAmount * (0.4f + 0.6f * velocity);
```
→ Le knob `click` reste contrôle de **maximum** (cohérent avec ce que l'utilisateur voit).

### 3.2. Choke groups étendus

Aujourd'hui choke = Hat Closed/Open uniquement. Ajouter (en interne, non exposé) :
- Crash : choke par autre Crash (déjà cas ?)
- Toms : pas de choke (volontaire)

### 3.3. Anti-overlap kick

Pour les kicks 808 (modèle Kick avec pitchDrop > 8st), si une nouvelle note arrive alors que la voix précédente a > 0.3 amplitude, **dimming** automatique de la voix précédente (-3 dB instantané) pour éviter accumulation low-end.

→ Implémentation dans `DrumSynthVoice.cpp` voice steal logic.

---

## PHASE 4 — REFONTE UI (layout/visuel uniquement)

### 4.1. Diagnostic du layout actuel (`PluginEditor.cpp` resized())

```
[ HEADER 64px : preset(280) | filter(180) | prev/next | … | masterDial(80) ]
[ LEFT 320 : pads 4×3 (300px) + macros 4 knobs ] [ CENTER 340 : 2×5 voice knobs ] [ RIGHT : FX selector + 4 knobs + 2 meters ]
[ BOTTOM 48 : single | utility | … | next | prev | preset(200) ]
```

**Problèmes identifiés** :
1. Header **et** bottom contiennent le preset → redondance visuelle
2. Macros 4 knobs **sous** la grille de pads = écrasés visuellement, peu de place
3. Voice design = grille 2×5 → ratio écran défavorable (knobs très étroits)
4. Right column : 4 knobs FX + 2 meters → encombré
5. Le ratio 1340×760 (≈1.76) impose un format "wide" qui sous-exploite la verticalité

### 4.2. Nouveau layout proposé — **3 zones horizontales équilibrées**

```
┌─────────────────────────────────────────────────────────────────────┐
│ TOPBAR 56px                                                         │
│ [Logo] [◀ Preset ▶] [Family ▼] [A/B] [Init] ─────── [Master] [VU] │
├─────────────────────────────────────────────────────────────────────┤
│ ZONE PADS (gauche, 38%)        │ ZONE VOICE (centre, 38%)           │
│ ┌──┬──┬──┬──┐                   │ ┌─ MIX ─┐ ┌─ ENV ─┐               │
│ │1 │2 │3 │4 │  pads 4×3         │ │LVL TUN│ │ATK DEC│               │
│ ├──┼──┼──┼──┤  carrés larges    │ │       │ │PDR PDC│               │
│ │5 │6 │7 │8 │  + nom/cat        │ └───────┘ └───────┘               │
│ ├──┼──┼──┼──┤                   │ ┌─ TIMBRE ─────────┐              │
│ │9 │10│11│12│                   │ │NOI CLK DRV CUT   │              │
│ └──┴──┴──┴──┘                   │ └──────────────────┘              │
│                                 │                                    │
│ ENVELOPE DISPLAY (compact)      │ MACROS                             │
│ ┌─────────────────────┐         │ ┌───┬───┬───┬───┐                 │
│ │ ⌒\___                │         │ │PCH│WGT│AIR│DRT│                 │
│ └─────────────────────┘         │ └───┴───┴───┴───┘                 │
├─────────────────────────────────┴────────────────────────────────────┤
│ ZONE FX (droite ou bas, 24%)                                        │
│ [Selector: Sat | TS | Comp | EQ | Cho | Dly | Rev | Lim]            │
│ ┌─ P1 ─┬─ P2 ─┬─ P3 ─┬─ P4 ─┐                                       │
│ └──────┴──────┴──────┴──────┘                                       │
└─────────────────────────────────────────────────────────────────────┘
```

### 4.3. Règles de design

| Règle | Valeur |
|---|---|
| Marge externe | 20 px (était 16) |
| Gouttière inter-zones | 18 px (était 12) |
| Coins arrondis panneaux | 14 px |
| Knob taille min | 64 px (touch-friendly) |
| Pad taille min | 78 px carré, 12 px gap |
| Hiérarchie typo | Section 13 px bold caps / Knob label 10 px / Value 11 px |
| Palette | Conserver `UIThemeV5` (déjà cohérente) |

### 4.4. Trois améliorations ergonomiques majeures (zéro dégel)

1. **Pad mini-VU intégré** : chaque pad affiche un mini-meter horizontal en bas (2 px) pour visualiser l'activité par pad.
2. **Section voice contextuelle** : afficher un badge "KICK / SNARE / HAT…" à côté du nom du pad sélectionné, avec sub-grouping visuel (MIX / ENV / TIMBRE encadrés).
3. **Envelope display** : composant déjà présent (`EnvelopeDisplayComponentV5.h`) **non utilisé** actuellement ; l'intégrer sous la grille pads pour donner un feedback visuel immédiat des knobs ENV.

### 4.5. Suppression de redondances

- Retirer le `presetBox` dupliqué du bottom bar (garder uniquement en topbar)
- Fusionner `singleNoteBtn` + `utilToggleBtn` dans un menu utilities en topbar
- Retirer `auxMeter` (ou le déplacer comme aux send d'une éventuelle FX bus future)

### 4.6. Resizing & breakpoints

```cpp
static constexpr int kMinW = 980;   // était 900 — assure lisibilité knobs
static constexpr int kMinH = 620;   // était 580
static constexpr float kAspectRatio = 16.0f / 10.0f;  // au lieu de 1340/760 ≈ 1.76
// Tailles cibles : 1280×800 (small), 1440×900 (default), 1600×1000 (large)
```

### 4.7. Fichiers UI à modifier

| Fichier | Modification |
|---|---|
| `PluginEditor.cpp` `resized()` | Réécriture complète selon §4.2 |
| `PluginEditor.h` | Ajouter `EnvelopeDisplayComponentV5 envDisplay;`, supprimer `auxMeter`, `singleNoteBtn` (déplacé dans menu) |
| `PadComponentV5` | Ajouter `setActivityLevel(float)` + paint d'une mini barre en bas |
| `UIThemeV5.h` | (option) ajouter constantes `kSectionGap`, `kSectionRadius` |

---

## PHASE 5 — DÉGELS PROPOSÉS (à arbitrer, non implémentés par défaut)

> Ces propositions **modifieraient le jeu de paramètres exposé**.
> À ne valider que si le bénéfice musical est jugé majeur. Sinon, restent gelés.

| ID | Dégel | Bénéfice | Impact rupture |
|---|---|---|---|
| D1 | Ajouter `velocity_to_click` (0-100%) per-pad | Snare réactive à la vélocité côté click | Ajout 12 params (1/pad) |
| D2 | Ajouter `delay_sync` (free/¼/⅛/⅛T/16) | Delay musicalement synchronisé tempo | 1 param + dépendance host BPM |
| D3 | Per-pad FX send (rev_send 0-100%) | Permet kit avec snare wet et kick dry | 12 params, gros impact mix |
| D4 | Dégel UI : 16 pads au lieu de 12 | Aligne sur Battery/AD2 | Modification kits, remap MIDI |

**Recommandation** : valider D2 (gain élevé, coût faible — 1 paramètre) ; reporter D1/D3/D4 à V2.

---

## PHASE 6 — PLANNING D'EXÉCUTION

| Sprint | Contenu | Effort | Gain audit |
|---|---|---|---|
| **S1 — Critique** | Phase 1.1 + 1.4 (matrice + recalibrages locaux) | 1 j | +2.0 pts |
| **S2 — FX activation** | Phase 1.2 + 1.3 (FX et EQ par famille) | 2 j | +1.5 pts |
| **S3 — Cohérence** | Phase 2 (différenciation kits) | 1 j | +0.5 pt |
| **S4 — DSP** | Phase 3.1 + 3.3 (vel-click, anti-overlap) | 2 j | +0.5 pt |
| **S5 — UI** | Phase 4 (refonte layout) | 3 j | qualitatif |
| **S6 — QA** | Régénération report QA, A/B vs avant | 1 j | validation |

**Total : ~10 jours** pour passer de **5.2/10 → 7.0/10** (objectif audit).

---

## PHASE 7 — VALIDATION & NON-RÉGRESSION

1. **Re-générer** `qa/drum_preset_qa_report.csv` après chaque sprint et differ.
2. **Test A/B** : conserver une branche `legacy-presets` pour comparaison sonore.
3. **Vérifier** que les peaks restent ≤ -0.3 dB (limiter) sur tous les kits.
4. **Test parametric** : pour chaque kit, jouer un pattern 4-on-the-floor à 90/120/140 BPM et mesurer le rapport snare/perc en RMS bandé 200-2k Hz (cible : snare ≥ perc + 1 dB).
5. **Smoke test UI** : resize 980×620 → 1600×1000, vérifier zéro overlap de composants.

---

## RÈGLES STRICTES MAINTENUES

✅ Aucun `addParameter()` ajouté/supprimé/renommé dans `PluginParameters.cpp`
✅ Aucun changement de range, default, label de knob
✅ Aucun changement d'ID APVTS
✅ Toutes les corrections sonores = valeurs internes presets ou DSP
✅ Refonte UI = `resized()` + composants existants réagencés

> Tout dégel doit être validé explicitement avant implémentation.
