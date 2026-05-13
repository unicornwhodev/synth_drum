# UWdeVST Drum Knob Inventory

Reference rapide des controles exposes dans l'editeur Drum et de leurs IDs APVTS.

## Notes

- Les parametres de pad suivent le schema `pad_<padIndex>_<suffix>`.
- `padIndex` va de `0` a `11`.
- Les macros, LFO, humanize et FX sont globaux au kit.
- L'editeur expose maintenant les pages `ADV FX` pour `EQ`, `Chorus`, `Delay`, `Limiter` et `Reverb Predelay`.
- Pas de mod matrix dans le drum : la modulation est assuree par le LFO global uniquement.
- Chaque pad possede 15 parametres communs (niveau, tune, decay, attack, pitch drop, pitch decay, noise, click, drive, cutoff, pan, output, vel_to_click, rev_send, dly_send).

## Catalogue des pads

### Liste et roles

| Index | Nom | Role | Choke Group |
| --- | --- | --- | --- |
| 0 | Kick A | Kick principal | - |
| 1 | Kick B | Kick secondaire / layer | - |
| 2 | Snare | Caisse claire | - |
| 3 | Clap | Clap / hand clap | - |
| 4 | Hat Closed | Charleston ferme | 1 |
| 5 | Hat Open | Charleston ouvert | 1 |
| 6 | Perc 1 | Percussion auxiliaire 1 | - |
| 7 | Perc 2 | Percussion auxiliaire 2 | - |
| 8 | Tom Low | Tom grave | - |
| 9 | Tom High | Tom aigu | - |
| 10 | Crash | Cymbale crash | - |
| 11 | FX | Pad d'effet / son electronique | - |

### Parametres specifiques par type de pad (voice-specific)

En plus des 15 parametres communs, certains pads exposent des controles supplementaires lies a leur moteur de synthese:

| Pad | Parametre | Param ID | Role |
| --- | --- | --- | --- |
| 3 Clap | Clap Spread | `pad_3_clap_spread` | Ecart temporel entre les couches du clap. |
| 3 Clap | Clap Density | `pad_3_clap_density` | Nombre de couches dans le clap. |
| 4 Hat Closed | Metallic Density | `pad_4_metallic_density` | Densite des partiels metalliques. |
| 5 Hat Open | Metallic Density | `pad_5_metallic_density` | Densite des partiels metalliques. |
| 5 Hat Open | Open Amount | `pad_5_open_amount` | Degre d'ouverture du charleston. |
| 6 Perc 1 | Body Tone | `pad_6_body_tone` | Tonalite du corps de la percussion. |
| 6 Perc 1 | Modal Ring | `pad_6_modal_ring` | Quantite de resonance modale. |
| 7 Perc 2 | Body Tone | `pad_7_body_tone` | Tonalite du corps de la percussion. |
| 7 Perc 2 | Modal Ring | `pad_7_modal_ring` | Quantite de resonance modale. |
| 8 Tom Low | Body Tone | `pad_8_body_tone` | Tonalite du corps du tom. |
| 8 Tom Low | Modal Ring | `pad_8_modal_ring` | Quantite de resonance modale. |
| 9 Tom High | Body Tone | `pad_9_body_tone` | Tonalite du corps du tom. |
| 9 Tom High | Modal Ring | `pad_9_modal_ring` | Quantite de resonance modale. |
| 10 Crash | Metallic Density | `pad_10_metallic_density` | Densite des partiels metalliques. |
| 11 FX | FM Index | `pad_11_fm_index` | Profondeur de modulation FM. |
| 11 FX | FM Sweep | `pad_11_fm_sweep` | Balayage de la frequence FM dans le temps. |

## Etat de la chaine FX specifique a l'instrument

- Statut: `Non`.
- Portee: les FX documentes ci-dessous sont kit-globaux et ne changent pas de disponibilite selon le pad selectionne.
- Etat UI reel: le pad selectionne specialise bien les controles de synthese de pad, mais pas la chaine FX globale.

## Pads — parametres communs

| UI Label | Param ID | Role |
| --- | --- | --- |
| Level | `pad_<n>_level` | Niveau de sortie du pad. |
| Tune | `pad_<n>_tune` | Transposition en demi-tons. |
| Decay | `pad_<n>_decay` | Duree principale du son. |
| Attack | `pad_<n>_attack` | Temps d'attaque. |
| Pitch Drop | `pad_<n>_pitch_drop` | Chute de pitch initiale. |
| Pitch Decay | `pad_<n>_pitch_decay` | Vitesse du retour de pitch. |
| Noise | `pad_<n>_noise` | Quantite de bruit. |
| Click | `pad_<n>_click` | Quantite d'attaque/click. |
| Drive | `pad_<n>_drive` | Saturation interne du pad. |
| Cutoff | `pad_<n>_cutoff` | Filtrage principal. |
| Pan | `pad_<n>_pan` | Placement stereo. |
| Output | `pad_<n>_output` | Routage `Master` ou `Out 1..12`. |
| Vel to Click | `pad_<n>_vel_to_click` | Sensibilite de la velocite au click. |
| Rev Send | `pad_<n>_rev_send` | Envoi individuel vers la reverb. |
| Dly Send | `pad_<n>_dly_send` | Envoi individuel vers le delay. |

## Parametres globaux

| UI Label | Param ID | Role |
| --- | --- | --- |
| Quality Mode | `quality_mode` | Choix `Live` / `Studio`. |
| Velocity Curve | `velocity_curve` | Courbe de velocite (7 options). |
| LFO Rate | `lfo_rate` | Frequence du LFO global [0.1, 20] Hz. |
| LFO Depth | `lfo_depth` | Profondeur du LFO global [0, 1]. |
| LFO Wave | `lfo_wave` | Forme d'onde du LFO (4 options). |
| Humanize Timing | `humanize_timing` | Variation aleatoire de timing [0, 50] ms. |
| Humanize Level | `humanize_level` | Variation aleatoire de niveau [0, 0.2]. |
| Aux Post FX | `aux_post_fx` | Routage aux apres les FX. |
| Reverb Enable | `fx_reverb_en` | Active la reverb globale. |
| Transient Enable | `fx_transient_en` | Active le transient shaper global. |
| Saturator Enable | `fx_saturator_en` | Active la saturateur globale. |
| Compressor Enable | `fx_comp_en` | Active le compresseur global. |

## Presets et transport

| UI Control | Param ID | Role |
| --- | --- | --- |
| Preset selector | n/a | Selection du kit factory ou utilisateur. |
| Pad Preset selector | n/a | Application d'un preset factory au pad actif. |
| Save / Override / Save As | n/a | Mise a jour utilisateur, override factory, ou creation d'un preset utilisateur. |
| Single Note | `single_note_mode` | Toute note MIDI joue le pad selectionne. |
| Selected Pad | `selected_pad` | Selection du pad edite. |
| Output Gain | `output_gain` | Gain de sortie global. |

## Macros

| UI Label | Param ID | Role |
| --- | --- | --- |
| Punch | `macro_punch` | Plus d'attaque et d'impact. |
| Weight | `macro_weight` | Plus de masse et de bas. |
| Air | `macro_air` | Plus d'ouverture dans le haut. |
| Dirt | `macro_dirt` | Plus de grain et de saturation. |

## FX principaux

### Compressor

| UI Label | Param ID | Role |
| --- | --- | --- |
| Threshold | `comp_threshold` | Seuil du compresseur. |
| Ratio | `comp_ratio` | Ratio de compression. |
| Attack | `comp_attack` | Temps d'attaque du compresseur. |
| Release | `comp_release` | Temps de release du compresseur. |
| Makeup | `comp_makeup` | Gain de compensation. |
| Mix | `comp_mix` | Balance dry/wet. |

### Saturation

| UI Label | Param ID | Role |
| --- | --- | --- |
| Drive | `sat_drive` | Quantite de saturation globale. |
| Mix | `sat_mix` | Balance dry/wet. |

### Transient

| UI Label | Param ID | Role |
| --- | --- | --- |
| Attack | `transient_attack` | Accentue ou adoucit l'attaque. |
| Sustain | `transient_sustain` | Renforce ou raccourcit la tenue. |
| Mix | `transient_mix` | Balance dry/wet. |

### Reverb

| UI Label | Param ID | Role |
| --- | --- | --- |
| Size | `reverb_size` | Taille de la reverb. |
| Damping | `reverb_damping` | Absorption des aigus. |
| Width | `reverb_width` | Largeur stereo. |
| Mix | `reverb_mix` | Balance dry/wet. |
| Predelay | `reverb_predelay` | Delai avant la queue de reverb. |

## Advanced FX Pages

### EQ A

| UI Label | Param ID | Role |
| --- | --- | --- |
| Enable | `fx_eq_en` | Active l'EQ globale. |
| Low Freq | `eq_low_freq` | Frequence de la bande grave. |
| Low Gain | `eq_low_gain` | Gain de la bande grave. |
| Mid Freq | `eq_mid_freq` | Frequence de la bande medium. |
| Mid Gain | `eq_mid_gain` | Gain de la bande medium. |

### EQ B

| UI Label | Param ID | Role |
| --- | --- | --- |
| Enable | `fx_eq_en` | Active l'EQ globale. |
| Mid Q | `eq_mid_q` | Q de la bande medium. |
| High Freq | `eq_high_freq` | Frequence de la bande aigue. |
| High Gain | `eq_high_gain` | Gain de la bande aigue. |
| Predelay | `reverb_predelay` | Predelay de la reverb. |

### Chorus

| UI Label | Param ID | Role |
| --- | --- | --- |
| Enable | `fx_chorus_en` | Active le chorus global. |
| Rate | `chorus_rate` | Vitesse de modulation. |
| Depth | `chorus_depth` | Profondeur de modulation. |
| Mix | `chorus_mix` | Balance dry/wet. |

### Delay

| UI Label | Param ID | Role |
| --- | --- | --- |
| Enable | `fx_delay_en` | Active le delay global. |
| Sync | `delay_sync` | Synchronisation au tempo hote. |
| Time | `delay_time` | Temps de delay libre. |
| Feedback | `delay_feedback` | Quantite de repetition. |
| Mix | `delay_mix` | Balance dry/wet. |
| Note Div | `delay_note_div` | Division rythmique si `Sync` est actif. |

### Limiter

| UI Label | Param ID | Role |
| --- | --- | --- |
| Enable | `fx_limiter_en` | Active le limiteur de sortie. |
| Threshold | `limiter_threshold` | Seuil du limiteur. |
| Release | `limiter_release` | Temps de release du limiteur. |
