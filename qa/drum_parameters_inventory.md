# INVENTAIRE EXHAUSTIF DES PARAMETRES - SYNTH DRUM (MDR)

> Plugin: **UWdeVST Drum** | Namespace: `mdr::` | 12 pads | 5 familles | **296 parametres APVTS**

---

## 1. PADS

### Fonction de generation d'ID

```cpp
juce::String makeDrumParamId(int padIndex, const juce::String& suffix)
// "pad_" + padIndex + "_" + suffix
```

### Familles et pads

| Index | Nom | Short | Famille | Synthesis | Choke Group |
|-------|-----|-------|---------|-----------|-------------|
| 0 | Kick A | KICKA | Kick | Tonal | - |
| 1 | Kick B | KICKB | Kick | Tonal | - |
| 2 | Snare | SNARE | Snare | Tonal | - |
| 3 | Clap | CLAP | Clap | NoiseBurst | - |
| 4 | Hat Closed | HATCL | Hat | Metallic | 1 |
| 5 | Hat Open | HATOP | Hat | Metallic | 1 |
| 6 | Perc 1 | PERC1 | Perc | Modal | - |
| 7 | Perc 2 | PERC2 | Perc | Modal | - |
| 8 | Tom Low | TOMLO | Tom | Tonal | - |
| 9 | Tom High | TOMHI | Tom | Tonal | - |
| 10 | Crash | CRASH | Crash | Metallic | - |
| 11 | FX | FX | FX | FM | - |

---

## 2. PARAMETRES PAR PAD (12 parametres universels x 12 pads = 144)

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step | Notes |
|---|-------------|-------------|------|-----|-----|---------|------|-------|
| 1 | `pad_<n>_level` | Level | Float | 0.0 | 1.0 | 0.90 | 0.0001 | Niveau de sortie du pad |
| 2 | `pad_<n>_tune` | Tune | Float | -24.0 st | 24.0 st | 1.5 | 0.01 | Transposition en demi-tons |
| 3 | `pad_<n>_decay` | Decay | Float | 0.03 s | 2.5 s | 0.30 | 0.0001 | Temps de decay enveloppe (s) |
| 4 | `pad_<n>_attack` | Attack | Float | 0.0 s | 0.05 s | 0.0004 | 0.0001 | Temps d'attaque (s) |
| 5 | `pad_<n>_pitch_drop` | Pitch Drop | Float | 0.0 st | 48.0 st | 5.0 | 0.01 | Drop initial de pitch (st) |
| 6 | `pad_<n>_pitch_decay` | Pitch Decay | Float | 0.005 s | 1.2 s | 0.032 | 0.0001 | Vitesse de retour pitch (s) |
| 7 | `pad_<n>_noise` | Noise | Float | 0.0 | 1.0 | 0.003 | 0.0001 | Quantite de bruit |
| 8 | `pad_<n>_click` | Click | Float | 0.0 | 1.0 | 0.08 | 0.0001 | Intensite du transitoire d'attaque |
| 9 | `pad_<n>_drive` | Drive | Float | 1.0 | 12.0 | 1.02 | 0.01 | Saturation |
| 10 | `pad_<n>_cutoff` | Cutoff | Float | 120.0 Hz | 18000.0 Hz | 3000.0 | skew 0.28 | Frequence de coupure filtre |
| 11 | `pad_<n>_pan` | Pan | Float | -1.0 | 1.0 | 0.0 | 0.001 | Placement stereo |
| 12 | `pad_<n>_output` | Output | Choice | - | - | Out 1 | - | Routage: Master ou Out 1-12 |

> Defaults varient par pad. Valeurs ci-dessus = Kick A (pad 0).

---

## 3. PARAMETRES SPECIFIQUES PAR MODELE DE VOIX

### 3.1 Clap (Pad 3)

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step |
|---|-------------|-------------|------|-----|-----|---------|------|
| 1 | `pad_3_clap_spread` | Clap Spread | Float | 0.0 | 1.0 | 0.42 | 0.0001 |
| 2 | `pad_3_clap_density` | Clap Density | Float | 0.0 | 1.0 | computed | 0.0001 |

### 3.2 Metallic — Hat Closed (4), Hat Open (5), Crash (10)

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step |
|---|-------------|-------------|------|-----|-----|---------|------|
| 1 | `pad_<n>_metallic_density` | Metallic Density | Float | 0.0 | 1.0 | computed | 0.0001 |
| 2 | `pad_<n>_open_amount` | Open Amount | Float | 0.0 | 1.0 | computed | 0.0001 |

> `open_amount` disponible seulement sur pads 5 (Hat Open) et 10 (Crash).

### 3.3 Modal/Tonal — Perc 1 (6), Perc 2 (7), Tom Low (8), Tom High (9)

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step |
|---|-------------|-------------|------|-----|-----|---------|------|
| 1 | `pad_<n>_body_tone` | Body Tone | Float | 0.0 | 1.0 | 0.30 (PercMetal: 0.68) | 0.0001 |
| 2 | `pad_<n>_modal_ring` | Modal Ring | Float | 0.0 | 1.0 | computed | 0.0001 |

### 3.4 FM — FX (Pad 11)

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step |
|---|-------------|-------------|------|-----|-----|---------|------|
| 1 | `pad_11_fm_index` | FM Index | Float | 0.0 | 1.0 | computed | 0.0001 |
| 2 | `pad_11_fm_sweep` | FM Sweep | Float | 0.0 | 1.0 | computed | 0.0001 |

---

## 4. PARAMETRES GLOBAUX HOTE

### 4.1 Master / Selection / Performance

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step |
|---|-------------|-------------|------|-----|-----|---------|------|
| 1 | `output_gain` | Output Gain | Float | -24.0 dB | 12.0 dB | -3.0 | 0.01 |
| 2 | `selected_pad` | Selected Pad | Choice | 0 | 11 | 0 (Kick A) | - |
| 3 | `single_note_mode` | Single Note Mode | Bool | false | true | false | - |
| 4 | `quality_mode` | Quality Mode | Choice | - | - | Live | - |
|   | | | | Choix: Live, Studio | | | |
| 5 | `velocity_curve` | Velocity Curve | Choice | - | - | Linear | - |
|   | | | | Choix: Linear, Soft, Softer, Hard, Harder, Fixed, Touch | | | |

### 4.2 Macros Globales

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step |
|---|-------------|-------------|------|-----|-----|---------|------|
| 6 | `macro_punch` | Macro Punch | Float | 0.0 | 1.0 | 0.5 | 0.0001 |
| 7 | `macro_weight` | Macro Weight | Float | 0.0 | 1.0 | 0.5 | 0.0001 |
| 8 | `macro_air` | Macro Air | Float | 0.0 | 1.0 | 0.5 | 0.0001 |
| 9 | `macro_dirt` | Macro Dirt | Float | 0.0 | 1.0 | 0.18 | 0.0001 |

### 4.3 LFO Global

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step |
|---|-------------|-------------|------|-----|-----|---------|------|
| 10 | `lfo_rate` | LFO Rate | Float | 0.1 Hz | 20.0 Hz | 2.0 | 0.01 | Skew: 0.4 |
| 11 | `lfo_depth` | LFO Depth | Float | 0.0 | 1.0 | 0.0 | 0.0001 |
| 12 | `lfo_wave` | LFO Wave | Choice | - | - | Sine | - |
|   | | | | Choix: Sine, Triangle, Saw, Square | | | |

### 4.4 Humanisation

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step |
|---|-------------|-------------|------|-----|-----|---------|------|
| 13 | `humanize_timing` | Humanize Timing | Float | 0.0 ms | 50.0 ms | 0.0 | 0.01 |
| 14 | `humanize_level` | Humanize Level | Float | 0.0 | 0.2 | 0.0 | 0.0001 |

### 4.5 Routage

| # | Parameter ID | Nom Affiche | Type | Default |
|---|-------------|-------------|------|---------|
| 15 | `aux_post_fx` | Aux Post-FX | Bool | false |

---

## 5. CHAINE FX GLOBALE

### 5.1 Toggles Enable FX

| # | Parameter ID | Nom Affiche | Type | Default | Slot FX |
|---|-------------|-------------|------|---------|---------|
| 1 | `fx_saturator_en` | Saturator Enable | Bool | true | SAT |
| 2 | `fx_transient_en` | Transient Enable | Bool | true | TRANS |
| 3 | `fx_comp_en` | Compressor Enable | Bool | true | COMP |
| 4 | `fx_eq_en` | EQ Enable | Bool | false | EQ |
| 5 | `fx_chorus_en` | Chorus Enable | Bool | false | CHORUS |
| 6 | `fx_delay_en` | Delay Enable | Bool | false | DELAY |
| 7 | `fx_reverb_en` | Reverb Enable | Bool | true | REVERB |
| 8 | `fx_limiter_en` | Limiter Enable | Bool | true | LIMITER |

### 5.2 Saturateur

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step |
|---|-------------|-------------|------|-----|-----|---------|------|
| 1 | `sat_drive` | Sat Drive | Float | 1.0 | 16.0 | 1.45 | 0.01 |
| 2 | `sat_mix` | Sat Mix | Float | 0.0 | 1.0 | 0.18 | 0.0001 |

### 5.3 Transient Shaper

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step |
|---|-------------|-------------|------|-----|-----|---------|------|
| 1 | `transient_attack` | Transient Attack | Float | -1.0 | 1.0 | 0.12 | 0.0001 |
| 2 | `transient_sustain` | Transient Sustain | Float | -1.0 | 1.0 | -0.05 | 0.0001 |
| 3 | `transient_mix` | Transient Mix | Float | 0.0 | 1.0 | 0.28 | 0.0001 |

### 5.4 Compresseur

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step |
|---|-------------|-------------|------|-----|-----|---------|------|
| 1 | `comp_threshold` | Comp Threshold | Float | -60.0 dB | 0.0 dB | -14.0 | 0.01 |
| 2 | `comp_ratio` | Comp Ratio | Float | 1.0 | 20.0 | 2.4 | 0.01 |
| 3 | `comp_attack` | Comp Attack | Float | 0.1 ms | 100.0 ms | 8.0 | 0.01 |
| 4 | `comp_release` | Comp Release | Float | 5.0 ms | 500.0 ms | 140.0 | 0.01 |
| 5 | `comp_makeup` | Comp Makeup | Float | 0.0 dB | 24.0 dB | 0.0 | 0.01 |
| 6 | `comp_mix` | Comp Mix | Float | 0.0 | 1.0 | 0.55 | 0.0001 |

### 5.5 EQ 3 Bandes Parametrique

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step | Skew |
|---|-------------|-------------|------|-----|-----|---------|------|------|
| 1 | `eq_low_freq` | EQ Low Freq | Float | 40.0 Hz | 500.0 Hz | 120.0 | 0.1 | 0.4 |
| 2 | `eq_low_gain` | EQ Low Gain | Float | -12.0 dB | 12.0 dB | 0.0 | 0.01 | - |
| 3 | `eq_mid_freq` | EQ Mid Freq | Float | 200.0 Hz | 8000.0 Hz | 1200.0 | 0.1 | 0.35 |
| 4 | `eq_mid_gain` | EQ Mid Gain | Float | -12.0 dB | 12.0 dB | 0.0 | 0.01 | - |
| 5 | `eq_mid_q` | EQ Mid Q | Float | 0.1 | 10.0 | 1.0 | 0.01 | 0.5 |
| 6 | `eq_high_freq` | EQ High Freq | Float | 2000.0 Hz | 16000.0 Hz | 6000.0 | 0.1 | 0.4 |
| 7 | `eq_high_gain` | EQ High Gain | Float | -12.0 dB | 12.0 dB | 0.0 | 0.01 | - |

### 5.6 Chorus

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step |
|---|-------------|-------------|------|-----|-----|---------|------|
| 1 | `chorus_rate` | Chorus Rate | Float | 0.1 Hz | 5.0 Hz | 1.0 | 0.01 |
| 2 | `chorus_depth` | Chorus Depth | Float | 0.0 | 1.0 | 0.5 | 0.0001 |
| 3 | `chorus_mix` | Chorus Mix | Float | 0.0 | 1.0 | 0.0 | 0.0001 |

### 5.7 Delay

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step | Skew |
|---|-------------|-------------|------|-----|-----|---------|------|------|
| 1 | `delay_time` | Delay Time | Float | 1.0 ms | 2000.0 ms | 300.0 | 0.1 | 0.35 |
| 2 | `delay_feedback` | Delay Feedback | Float | 0.0 | 0.95 | 0.30 | 0.001 | - |
| 3 | `delay_mix` | Delay Mix | Float | 0.0 | 1.0 | 0.0 | 0.0001 | - |
| 4 | `delay_sync` | Delay Sync | Bool | false | true | false | - | - |
| 5 | `delay_note_div` | Delay Note Div | Choice | - | - | 1/4 | - | - |
|   | | | | Choix: 1/4, 1/8, 1/16, dotted 1/8, triplet 1/8 | | | | |

### 5.8 Reverb

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step |
|---|-------------|-------------|------|-----|-----|---------|------|
| 1 | `reverb_size` | Reverb Size | Float | 0.0 | 1.0 | 0.35 | 0.0001 |
| 2 | `reverb_damping` | Reverb Damping | Float | 0.0 | 1.0 | 0.70 | 0.0001 |
| 3 | `reverb_width` | Reverb Width | Float | 0.0 | 1.0 | 0.80 | 0.0001 |
| 4 | `reverb_mix` | Reverb Mix | Float | 0.0 | 1.0 | 0.15 | 0.0001 |
| 5 | `reverb_predelay` | Reverb Predelay | Float | 0.0 ms | 100.0 ms | 12.0 | 0.1 |

### 5.9 Limiter

| # | Parameter ID | Nom Affiche | Type | Min | Max | Default | Step |
|---|-------------|-------------|------|-----|-----|---------|------|
| 1 | `limiter_threshold` | Limiter Threshold | Float | -12.0 dB | 0.0 dB | -0.3 | 0.01 |
| 2 | `limiter_release` | Limiter Release | Float | 1.0 ms | 200.0 ms | 50.0 | 0.1 |

---

## 6. ARCHITECTURE FX

| Slot | Processeur | Default Enable | Parametres |
|------|------------|----------------|------------|
| 0 | Saturateur | ON | 2 |
| 1 | Transient Shaper | ON | 3 |
| 2 | Compresseur | ON | 6 |
| 3 | EQ 3-bandes | OFF | 7 |
| 4 | Chorus | OFF | 3 |
| 5 | Delay | OFF | 5 |
| 6 | Reverb | ON | 5 |
| 7 | Limiter | ON | 2 |

---

## 7. RESUME

| Categorie | Nombre |
|-----------|--------|
| Parametres universels par pad (x12) | 144 |
| Parametres voix specifiques | 96 |
| Parametres globaux (master, macros, LFO, humanise, routing) | 15 |
| Parametres FX globaux (enables + knobs) | 41 |
| **TOTAL APVTS** | **296** |
