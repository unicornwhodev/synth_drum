# ÉTAPE 4 — Analyse approfondie de la cohérence des kits

## 4.1. Analyse globale de la cohérence des kits

### Méthodologie d'évaluation de cohérence de kit

Un kit coherent doit vérifier :
1. **Équilibre interne** : Les levels des pads sont-ils musicalement équilibrés (kick/snare hierarchy)?
2. **Cohérence stylistique** : Les pads fonctionnent-ils ensemble dans un contexte musical cohérent?
3. **Pas de redondance** : Deux pads ne font pas la même chose au même niveau d'énergie
4. **Pas de trous fonctionnels** : Tous les éléments nécessaires au style sont présents
5. **Cohérence FX/kit** : Les effets correspondent au style du kit

### Grille de cohérence par kit

---

## 4.2. Analyse par famille

### A. Famille Classique (3 kits)

#### Classique Standard

| Aspect | Observation | Score (1-5) |
|--------|-------------|-------------|
| Équilibre Kick/Snare | Kick A 0.88, Snare 0.76, Clap 0.68 | 4/5 |
| Hiérarchie Hats | Hat Closed 0.52, Hat Open 0.46 | 4/5 |
| Role Percs | Perc1 0.96 (très fort), Perc2 0.90 (très fort) | ⚠️ 2/5 - Perc trop fort vs snare |
| Role Toms | Tom Low 0.60, Tom High 0.56 | 4/5 |
| FX placement | FX 0.50 (bien plus bas que Percs) | 4/5 |
| Cohérence stylistique | "Dry, balanced, punchy" - cohérent | 4/5 |
| FX chain | Reverb 0.16, comp 0.35, limiter active | 4/5 |

**Problème identifié** : Perc 1 (0.96) et Perc 2 (0.90) sont PLUS FORTS que Snare (0.76) dans ce kit. Ce n'est pas cohérent avec un kit "Classique Standard" où la snare devrait être le centre rythmique.

**Conclusion** : Kit incohérent sur la hiérarchie — Percs dominent la snare.

---

#### Classique Tight

| Aspect | Observation | Score |
|--------|-------------|-------|
| Équilibre Kick/Snare | Decays réduits, snare decay 0.70x | 4/5 |
| Hiérarchie Hats | Decay Hat Closed 0.016s (very tight) | 4/5 |
| Role Percs | Perc1 0.96, Perc2 0.90 (inchangé de Standard) | ⚠️ 2/5 |
| Cohérence stylistique | "Tight, controlled, dry" | 4/5 |

**Problème identifié** : Même déséquilibre Perc/Snare que Standard.

**Conclusion** : Incohérent.

---

#### Classique Open

| Aspect | Observation | Score |
|--------|-------------|-------|
| Équilibre interne | Decay kicks allongés (0.48s), snare 0.145s | 3/5 |
| Hiérarchie | Perc1 0.96, Perc2 0.90 (toujours très forts) | ⚠️ 2/5 |
| Cohérence FX | Reverb 0.22 (up from 0.16), reverbSize 0.55 | 4/5 |

**Problème identifié** : Même problème Perc dominant.

**Conclusion** : Incohérent.

---

#### Bilan Classique : Problème systémique de hiérarchie

Le problème de Percs dominants (0.96/0.90 vs Snare 0.76) est **SYSTÉMIQUE** dans toute la famille Classique. Cela suggère que les levels par défaut de PercWood et PercMetal sont trop élevés par rapport aux autres pads.

---

### B. Famille Acoustique (4 kits)

#### Acoustique Room

| Aspect | Observation | Score |
|--------|-------------|-------|
| Équilibre | Perc1 0.92, Perc2 0.88, Snare 0.74 | ⚠️ 2/5 - Percs dominent snare |
| Hats balance | Hat Closed 0.52, Hat Open 0.48 | 4/5 |
| Cohérence "Room" | ReverbSize 0.52, reverbMix 0.20 | 4/5 |
| Perc levels | Encore 0.92/0.88 vs snare 0.74 | ⚠️ Incohérent |

#### Acoustique Studio

| Aspect | Observation | Score |
|--------|-------------|-------|
| Snare level | +5% par rapport à Room (level 0.74 → 0.78 approx) | 3/5 |
| Cohérence "Studio" | ReverbSize 0.36 (moins que Room), plus sec | 4/5 |
| Équilibre | Percs still 0.92/0.88 | ⚠️ 2/5 |

#### Acoustique Brush

| Aspect | Observation | Score |
|--------|-------------|-------|
| Snare character | Noise 0.55 (réduit), click 0.02, cutoff 4400Hz | 3/5 |
| Kick | Level 0.76 (reduced), drive 1.00 | 3/5 |
| Hats | Level 0.44/0.40 (réduit) | 4/5 |
| Perc levels | Perc1 0.92, Perc2 0.88 (toujours) | ⚠️ 2/5 |
| Cohérence "Brush" | Soft, low-energy — cohérent | 4/5 |

#### Acoustique Jazz

| Aspect | Observation | Score |
|--------|-------------|-------|
| Caractère Jazz | Decay kicks court (0.22s), snare 0.09s, hats light | 4/5 |
| Cohérence Jazz | ReverbSize 0.60, reverbMix 0.24, predelay 20ms | 4/5 |
| Perc levels | Toujours 0.92/0.88 | ⚠️ 2/5 |

**Bilan Acoustique** : Problème systémique PERC dominates sur Snare dans TOUS les kits Acoustique.

---

### C. Famille Ambient (3 kits)

#### Ambient Pad

| Aspect | Observation | Score |
|--------|-------------|-------|
| Équilibre global | Tous les levels réduits (Perc1 0.88, Perc2 0.82) | 3/5 |
| Snare | Level 0.68, moins de bruit que Classique | 3/5 |
| Cohérence "Pad/Wash" | ReverbMix 0.28, reverbSize 0.68 | 5/5 |
| FX balance | FX 0.48 — correct pour texture | 4/5 |

#### Ambient Dark

| Aspect | Observation | Score |
|--------|-------------|-------|
| Caractère Dark | Cuts HP, boost LP, decays longs | 4/5 |
| Perc levels | 0.88/0.82 (inchangé de Pad) | 3/5 |
| Crash | Decay 0.60s, level 0.40 — ambient crash | 4/5 |

#### Ambient Sparse

| Aspect | Observation | Score |
|--------|-------------|-------|
| Energy minimale | Tous levels * 0.82 | 4/5 |
| Cohérence "Sparse" | ReverbMix 0.28, reverbSize 0.65 | 5/5 |

**Bilan Ambient** : Les kits Ambient sont plus cohérents que Classique/Acoustique. Le problème Perc dominant existe mais est atténué par les levels généraux réduits.

---

### D. Famille Cinématique (4 kits)

#### Cinematique Epic

| Aspect | Observation | Score |
|--------|-------------|-------|
| Équilibre | Perc1 1.00 (MAX), Perc2 0.96 (MAX) | ⚠️ 2/5 |
| Kick A | Level 0.90 — correct | 4/5 |
| Snare | Level 0.76 — correct | 4/5 |
| FX | Level 0.56 — plus présent que Classique | 4/5 |
| Cohérence Epic | Reverb 0.92 width, comp 0.45 mix | 5/5 |

**Problème** : Perc1/Perc2 à 1.00 — Dominance perc très forte.

---

#### Cinematique Tension

| Aspect | Observation | Score |
|--------|-------------|-------|
| Caractère Tension | Drive elevated, compression agressive | 4/5 |
| Perc levels | Toujours 1.00/0.96 | ⚠️ 2/5 |
| Kick | Drive 1.18, pitchDrop 10st | 4/5 |

#### Cinematique Hybrid

| Aspect | Observation | Score |
|--------|-------------|-------|
| Electronic blend | Kick B modified (decay 0.14s, pitchDrop 8st) | 4/5 |
| Snare | Click Amount 0.12 (augmenté) | 4/5 |
| Delay enabled | DelayTime 250ms, feedback 0.22 | 4/5 |

#### Cinematique Percussion

| Aspect | Observation | Score |
|--------|-------------|-------|
| Perc-forward | Perc1 1.00, Perc2 1.00, Toms 0.70/0.66 | 4/5 |
| Kicks reduits | Kick A 0.80, Kick B 0.65 | 5/5 |
| Cohérence Percussion | HIÉRARARCHIE CORRIGÉE | 5/5 |

**Bilan Cinématique** : Le problème Perc dominant existe dans Epic, Tension, Hybrid mais EST CORRIGÉ dans Percussion. Le dernier kit de la famille corrige le problème des précédents — ce qui suggère une reconnaissance progressive du problème.

---

### E. Famille Moderne (4 kits)

#### Moderne Club

| Aspect | Observation | Score |
|--------|-------------|-------|
| Équilibre | Perc1 0.96, Perc2 0.92, Snare 0.74 | ⚠️ 3/5 - encore Perc dominant mais moins |
| Kick | Level 0.88, drive 1.06 — punchy | 4/5 |
| Hats | Noise 0.72, cutoff 8800Hz — très metallique | 4/5 |
| Cohérence Club | ReverbMix 0.13, comp 0.42 | 5/5 |

#### Moderne Lo-Fi

| Aspect | Observation | Score |
|--------|-------------|-------|
| Lo-Fi character | Cutoff * 0.68, drive * 1.08, satDrive 1.6 | 4/5 |
| Perc levels | Toujours 0.96/0.92 | ⚠️ 3/5 |
| EQ enabled | High cut 5000Hz, low cut 200Hz | 4/5 |

#### Moderne Trap

| Aspect | Observation | Score |
|--------|-------------|-------|
| Kick 808 | Decay 0.38s, pitchDrop 12st, cutoff 1400Hz | 4/5 |
| Snare | Decay 0.10s, noise 0.66, level 0.74 | 4/5 |
| Perc levels | 0.96/0.92 | ⚠️ 3/5 |
| Cohérence Trap | Comp 0.48, transientAttack 0.10 | 4/5 |

#### Moderne Electro

| Aspect | Observation | Score |
|--------|-------------|-------|
| Electronic character | Chorus enabled, delay enabled | 5/5 |
| Perc2 | Level 1.00, decay 0.12s, pitchDrop 14st | 4/5 |
| Cohérence Electro | ChorusRate 2.0, depth 0.6, delay sync | 5/5 |

**Bilan Moderne** : Le problème Perc dominant est atténué mais toujours présent (Perc1 0.96 vs Snare 0.74). La famille Moderne est plus cohérente que Classique/Acoustique.

---

## 4.3. Synthèse des problèmes de cohérence par kit

### Problèmes systémiques identifiés

| ID | Problème | Niveau | Gravité | Kits affectés |
|----|----------|--------|---------|---------------|
| K1 | Perc dominant > Snare | SYSTÉMIQUE | 8/10 | Presque tous (sauf Cinematique Percussion) |
| K2 | FX level trop bas pour rôle texture | SYSTÉMIQUE | 5/10 | Classique, Acoustique |
| K3 | Tom High trop proche Snare freq | STRUCTUREL | 6/10 | Tous |
| K4 | Perc2 (650Hz) trop proche FX (720Hz) | STRUCTUREL | 4/10 | Tous |
| K5 | Hats clickAmount non-naturaliste | SYSTÉMIQUE | 5/10 | Presque tous |

### Kits individuels problématiques

| Kit | Problème principal | Cohérence globale |
|-----|-------------------|-------------------|
| Classique Standard | Perc > Snare, hiérarchie inversée | 3/10 |
| Classique Tight | Même problème | 3/10 |
| Classique Open | Même problème | 3/10 |
| Acoustique Room | Perc > Snare, confusion Room/Studio | 3/10 |
| Acoustique Studio | Perc > Snare | 3/10 |
| Acoustique Brush | Perc > Snare, snare trop douce | 3/10 |
| Acoustique Jazz | Perc > Snare | 3/10 |
| Ambient Pad | Plus cohérent | 7/10 |
| Ambient Dark | Plus cohérent | 7/10 |
| Ambient Sparse | Plus cohérent | 7/10 |
| Cinematique Epic | Perc > Snare mais FX compensateur | 5/10 |
| Cinematique Tension | Perc > Snare | 5/10 |
| Cinematique Hybrid | Perc > Snare | 5/10 |
| Cinematique Percussion | HIÉRARCHIE CORRIGÉE | 8/10 |
| Moderne Club | Perc > Snare mais moins | 6/10 |
| Moderne Lo-Fi | Même | 6/10 |
| Moderne Trap | Plus équilibré | 7/10 |
| Moderne Electro | Perc2 fort mais cohérent avec style | 7/10 |

---

## 4.4. Classification des kits par cohérence

| Catégorie | Kits | Caractérisation |
|-----------|------|-----------------|
| **Kit cohérent** | Ambient Pad, Ambient Dark, Ambient Sparse, Cinematique Percussion, Moderne Trap, Moderne Electro | Hiérarchie respected, cohérent avec promesse stylistique |
| **Kit acceptable** | Moderne Club, Moderne Lo-Fi, Cinematique Epic, Cinematique Tension, Cinematique Hybrid | Quelques small déséquilibres maisusable |
| **Kit déséquilibré** | Tous les Classique, Tous les Acoustique | Perc dominates Snare, hiérarchie incorrecte |
| **Kit incohérent** | Aucun parmi les 18 analysés n'est complètement unusable, mais plusieurs sont significativement déséquilibrés | - |

---

**Conclusion étape 4** : Le problème le plus critique de cohérence est le **Perc dominant sur Snare** dans presque tous les kits, à l'exception notable de Cinematique Percussion qui corrige ce problème. Les familles Ambient et Moderne sont plus cohérentes que Classique et Acoustique. Le problème est majeur car il affecte la hiérarchie rythmique fondamentale — dans un groove, la snare doit être le point focal, pas les percussions.