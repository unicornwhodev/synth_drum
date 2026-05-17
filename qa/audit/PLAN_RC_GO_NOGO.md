# Plan de correction et de clôture RC Go/No-Go — Synth Drum

## 1. Objectif du plan

Ce plan transforme les problèmes identifiés pendant l'audit drum en un parcours de correction contrôlé jusqu'à une décision **RC Go/No-Go**.

L'objectif n'est pas d'ajouter des fonctions secondaires, mais de rendre le produit suffisamment cohérent, jouable et vérifiable pour une release candidate.

Le plan couvre :
- la cohérence de la gamme de kits ;
- la cohérence des 12 pads ;
- les transients et attaques ;
- la hiérarchie rythmique ;
- les fins de sons ;
- le chevauchement ;
- la lisibilité en groove et mini-mix ;
- les effets ;
- la qualité sonore réelle ;
- les critères de décision Go/No-Go.

## 2. Position de départ

### 2.1. État produit audité

Le produit est un drum synth procédural à 12 pads. La promesse produit mentionne 25 kits couvrant un spectre classique à techno, mais l'état inspecté expose 18 kits factory répartis en 5 familles :

| Famille | Kits observés | Risque principal |
|---|---:|---|
| Classique | 3 | variantes proches, différenciation modérée |
| Acoustique | 4 | crédibilité acoustique limitée, snare douce à risque |
| Ambient | 3 | kits texturaux plus que rythmiques |
| Cinématique | 4 | impact flatteur, densité et queues longues |
| Moderne | 4 | meilleur potentiel synthétique, techno insuffisamment spécialisée |

### 2.2. Arbitrage de clôture

La RC ne doit pas chercher à transformer le produit en batterie acoustique réaliste ou en clone de Battery/Groove Agent. La RC doit valider un positionnement plus défendable :

> Drum synth procédural à 12 pads, orienté kits stylisés, électronique, hybride, cinématique léger et production rapide, avec familles acoustiques assumées comme stylisées et non comme reproduction réaliste.

## 3. Principes de correction

1. **Stabiliser avant d'élargir** : corriger la banque actuelle avant d'ajouter des kits.
2. **Prioriser le groove réel** : un kit moins spectaculaire mais lisible vaut mieux qu'un kit impressionnant en solo et brouillon en pattern.
3. **Séparer les familles** : les kits classiques, acoustiques, ambient, cinématiques et modernes ne doivent pas partager exactement la même logique d'effets et de queues.
4. **Hiérarchiser les pads** : Kick A, Snare et Hat Closed doivent rester les repères rythmiques principaux sauf exception explicitement assumée.
5. **Limiter les queues longues** : les longues fins de sons doivent être réservées à des rôles précis, pas devenir un comportement par défaut.
6. **Mesurer puis écouter** : les métriques QA ne suffisent pas ; chaque correction critique doit être confirmée par patterns et mini-mix.
7. **Fermer le scope RC** : toute amélioration non critique est reportée après RC.

## 4. Découpage en phases

| Phase | Nom | But | Décision attendue |
|---:|---|---|---|
| 0 | Gel du périmètre RC | empêcher l'élargissement incontrôlé | scope figé |
| 1 | Correction de gamme | résoudre 18 vs 25 kits et familles | gamme cohérente |
| 2 | Correction des presets | corriger hiérarchie, transients, niveaux | kits jouables |
| 3 | Correction des fins et chevauchements | rendre les patterns propres | groove lisible |
| 4 | Correction FX | adapter les effets aux familles | traitement cohérent |
| 5 | QA audio et mini-mix | valider usage réel | défauts bloquants levés |
| 6 | Benchmark court de contrôle | vérifier position marché | promesse réaliste |
| 7 | RC Go/No-Go | arbitrer release candidate | Go, Go conditionnel ou No-Go |

## 5. Phase 0 — Gel du périmètre RC

### Objectif

Empêcher que la stabilisation RC soit diluée par de nouvelles fonctions, de nouveaux effets ou une extension non contrôlée de la banque.

### Actions

| ID | Action | Sortie attendue | Priorité |
|---|---|---|---|
| P0-1 | Geler les fonctionnalités moteur hors bug critique | liste de fonctionnalités exclues RC | P0 |
| P0-2 | Geler le nombre cible de kits RC | décision 18 kits assumés ou 25 kits restaurés | P0 |
| P0-3 | Geler les familles RC | liste officielle des familles | P0 |
| P0-4 | Geler les critères QA | grille Go/No-Go validée | P0 |

### Critère de sortie

La phase est close quand le projet sait explicitement s'il livre **18 kits assumés** ou **25 kits complets**. Sans cette décision, la RC est automatiquement **No-Go**.

## 6. Phase 1 — Correction de gamme et positionnement

### Objectif

Rendre la promesse produit cohérente avec ce qui est réellement livré.

### Actions critiques

| ID | Problème visé | Correction | Niveau | Urgence | Critère d'acceptation |
|---|---|---|---|---|---|
| P1-1 | 25 kits annoncés / 18 observés | choisir entre ajouter 7 kits ou corriger la promesse | global | P0 | aucune documentation ni metadata ne contredit la banque réelle |
| P1-2 | promesse acoustique trop réaliste | renommer ou décrire les kits acoustiques comme stylisés | perception | P1 | pas de promesse de réalisme acoustique non tenue |
| P1-3 | techno insuffisamment spécialisée | créer un kit techno dédié ou retirer le terme techno dominant | gamme | P1 | l'utilisateur comprend le territoire moderne/electro réel |
| P1-4 | familles inégales | équilibrer le nombre et le rôle des familles | gamme | P2 | aucune famille ne ressemble à un remplissage |

### Arbitrage recommandé

Pour une RC rapide et sûre, livrer **18 kits assumés** est préférable à ajouter 7 kits tardifs. Ajouter 7 kits en fin de cycle augmente le risque de régression sonore et de kits non validés.

### Critère de sortie

La gamme doit être lisible en moins de 30 secondes par un utilisateur : nom de famille, rôle du kit, usage probable.

## 7. Phase 2 — Correction des presets et de la hiérarchie des 12 pads

### Objectif

Transformer chaque kit en instrument jouable, pas seulement en collection de sons isolés.

### Règle de hiérarchie pad par défaut

| Priorité | Pad | Rôle attendu |
|---:|---|---|
| 1 | Kick A | fondation grave principale |
| 2 | Snare | repère backbeat principal |
| 3 | Hat Closed | subdivision lisible |
| 4 | Kick B | variation ou accent clairement distinct |
| 5 | Clap | accent ou alternative snare |
| 6 | Hat Open | respiration contrôlée |
| 7 | Perc 1 / Perc 2 | contre-rythme secondaire |
| 8 | Tom Low / Tom High | fills, pas fondation permanente |
| 9 | Crash | ponctuation |
| 10 | FX | transition ou texture, jamais base du groove |

### Actions critiques

| ID | Problème visé | Correction | Niveau | Urgence | Critère d'acceptation |
|---|---|---|---|---|---|
| P2-1 | Kick A/B trop proches | donner un rôle distinct à chaque kick | pad | P1 | Kick B est identifiable sans concurrencer Kick A |
| P2-2 | Snare parfois trop faible | rehausser présence, attaque ou bruit selon famille | pad/kit | P1 | la snare reste lisible dans un groove simple |
| P2-3 | percussions trop dominantes | réduire niveaux ou attaques hors kit percussion-forward | pad/kit | P1 | Perc 1/2 ne masquent pas snare/hats |
| P2-4 | hats trop clicky ou trop coupés | ajuster attack/decay et niveau | pad | P1 | répétitions rapides restent lisibles et non fatigantes |
| P2-5 | toms proches snare | séparer pitch, decay ou niveau | pad | P2 | les fills ne brouillent pas le backbeat |
| P2-6 | FX trop présents | réduire niveau, durée ou spectre selon kit | pad | P1 | le pad FX ne détruit pas le groove |

### Critère de sortie

Chaque kit doit produire un pattern 4/4 simple kick-snare-hat lisible sans édition utilisateur.

## 8. Phase 3 — Transients, fins de sons et chevauchements

### Objectif

Assurer que les attaques restent lisibles quand les sons se répètent ou se superposent.

### Tests musicaux obligatoires

| Test | Pattern | Ce qu'il vérifie |
|---|---|---|
| T3-1 | kick sur chaque temps + snare 2/4 + hats 1/8 | base groove |
| T3-2 | hats 1/16 + ghost snare léger | répétitions et fatigue |
| T3-3 | kick rapide syncopé + open hat | chevauchement grave/aigu |
| T3-4 | crash + FX + groove continu | queues longues |
| T3-5 | tom fill sur 1 mesure | masque snare/toms |

### Actions critiques

| ID | Problème visé | Correction | Niveau | Urgence | Critère d'acceptation |
|---|---|---|---|---|---|
| P3-1 | attaque molle sur acoustique doux | renforcer transients utiles sans rendre artificiel | famille/kit | P1 | backbeat lisible en mini-mix |
| P3-2 | cinématique trop dense | raccourcir ou baisser queues secondaires | famille | P1 | nouvelle attaque audible après queue précédente |
| P3-3 | ambient destructeur de groove | séparer kits texture et kits rythmiques | famille | P1 | le rôle texture est assumé ou le groove est lisible |
| P3-4 | trap/sub envahissant | réduire decay ou énergie grave répétée | kit | P1 | kick rapide sans wash grave |
| P3-5 | open/closed hats | formaliser une logique de choke ou pseudo-choke | pad/moteur | P1 | hat fermé reprend la priorité sur open hat |

### Critère de sortie

Aucun kit orienté groove ne doit perdre sa pulsation principale dans un pattern dense de 2 mesures.

## 9. Phase 4 — Correction FX par famille

### Objectif

Faire des effets un outil de cohérence musicale, non un habillage global indifférencié.

### Profils FX attendus

| Famille | Saturation | Compression | Transient | Reverb | Delay | Widener/modulation |
|---|---|---|---|---|---|---|
| Classique | très faible | légère | discret | courte | non | très faible |
| Acoustique | faible | légère à moyenne | naturel | room courte | non | faible |
| Ambient | faible à moyenne | douce | réduit | longue mais contrôlée | ponctuel | possible |
| Cinématique | moyenne | moyenne | fort mais contrôlé | large contrôlée | ponctuel | possible |
| Moderne | moyenne à forte | punchy | net | courte ou spéciale | tempo-sync seulement | prudent |

### Actions critiques

| ID | Problème visé | Correction | Niveau | Urgence | Critère d'acceptation |
|---|---|---|---|---|---|
| P4-1 | FX globaux incohérents | créer profils par famille | global/famille | P1 | chaque famille a une logique FX explicite |
| P4-2 | reverb trop uniforme | adapter taille/mix/decay | famille | P1 | kits dry restent dry, kits larges restent contrôlés |
| P4-3 | saturation non contextualisée | doser selon style | famille/kit | P2 | moderne plus mordant sans salir classique |
| P4-4 | compression potentiellement écrasante | préserver transients | kit | P1 | kick/snare gardent leur attaque |
| P4-5 | delay musicalement risqué | le désactiver si non tempo-sync | global | P1 | pas de répétition non calée en kit rythmique |

### Critère de sortie

Un changement de kit ne doit pas donner l'impression que le même preset FX est collé sur des styles incompatibles.

## 10. Phase 5 — QA audio, patterns et mini-mix

### Objectif

Valider la qualité réelle en situation musicale, pas seulement sur sons isolés.

### Matrice QA minimale

Chaque kit RC doit être évalué sur 5 rendus :

| Rendu | Durée | Contenu | Verdict attendu |
|---|---:|---|---|
| Pad sweep | 12 pads isolés | un coup par pad | identité et niveau contrôlés |
| Pattern simple | 2 mesures | kick/snare/hat | groove lisible |
| Pattern dense | 2 mesures | hats rapides, percs, variations | pas de brouillard |
| Fill + crash/FX | 2 mesures | toms, crash, FX | queues contrôlées |
| Mini-mix | 4 mesures | basse + accord/pad léger | kit exploitable |

### Notation RC

| Score | Signification | Décision locale |
|---:|---|---|
| 5 | prêt RC | validé |
| 4 | défaut mineur | validé sous surveillance |
| 3 | acceptable mais faible | correction recommandée |
| 2 | problème sérieux | correction obligatoire |
| 1 | inutilisable | bloque RC |

### Critères bloquants

Un kit bloque la RC si au moins une condition est vraie :
- snare non lisible dans pattern simple ;
- kick dominant au point de masquer le groove ;
- hats fatigants ou coupés de manière artificielle ;
- queue FX/crash détruisant la mesure suivante ;
- niveau perçu très différent des autres kits ;
- nom du kit promet un usage qu'il ne tient pas.

## 11. Phase 6 — Benchmark court de contrôle

### Objectif

Ne pas refaire un benchmark complet, mais vérifier que la RC n'est pas positionnée de manière irréaliste face au marché.

### Produits de contrôle

| Produit | Rôle du contrôle | Attente RC réaliste |
|---|---|---|
| Native Instruments Battery 4 | référence reconnue pads/kits/FX | ne pas prétendre égaler son workflow |
| Steinberg Groove Agent SE | outil régulier acoustique + beat | accepter que l'acoustique soit moins réaliste |
| AIR Boom | drum machine simple régulière | dépasser en richesse et modernité |
| SampleScience 606 Koncept | freeware ciblé | dépasser en gamme et contrôle |

### Critère de sortie

La RC doit être vendable comme synthé drum stylisé. Elle ne doit pas être présentée comme une banque réaliste premium ni comme un remplaçant Battery/Groove Agent.

## 12. Phase 7 — Décision RC Go/No-Go

### Barème de décision

| Domaine | Pondération | Go | Go conditionnel | No-Go |
|---|---:|---|---|---|
| Cohérence gamme | 15% | promesse alignée | 1 ambiguïté mineure | 18/25 non résolu |
| Transients | 15% | attaques lisibles | quelques kits faibles | snare/kick flous récurrents |
| Cohérence kits | 20% | hiérarchie stable | 1-2 kits à surveiller | kits majeurs incohérents |
| Fins/chevauchements | 15% | queues contrôlées | familles ambient/ciné seulement | groove détruit par queues |
| Jouabilité | 10% | patterns simples/denses OK | fatigue ponctuelle | réponse imprévisible |
| FX | 10% | profils cohérents | quelques dosages à affiner | FX globaux incohérents |
| Qualité réelle | 10% | mini-mix exploitable | usage limité mais clair | son brouillon ou trompeur |
| Documentation/positionnement | 5% | promesse honnête | termes à clarifier | promesse fausse |

### Conditions Go

La RC peut passer **Go** seulement si :
- la décision 18 vs 25 kits est résolue ;
- aucun kit principal n'a de score inférieur à 4 en pattern simple ;
- aucun kit orienté groove n'a de score inférieur à 3 en pattern dense ;
- les kits acoustiques sont décrits comme stylisés si leur réalisme reste limité ;
- les niveaux inter-kits ne créent pas de saut utilisateur dangereux ;
- les FX sont différenciés par famille ou suffisamment neutres pour ne pas créer d'incohérence.

### Conditions Go conditionnel

La RC peut passer **Go conditionnel** si :
- les défauts restants sont documentés ;
- aucun défaut restant ne détruit le groove de base ;
- les problèmes sont limités à des kits secondaires ou texturaux ;
- le plan post-RC liste précisément les corrections restantes.

### Conditions No-Go

La RC est **No-Go** si :
- la contradiction 25 kits / 18 kits reste visible ;
- plus de 2 kits orientés groove échouent en pattern simple ;
- les fins de sons masquent régulièrement les attaques suivantes ;
- les FX donnent une signature incohérente aux familles ;
- la promesse marketing reste plus ambitieuse que la qualité réelle.

## 13. Checklist opérationnelle de clôture

| # | Check | Responsable suggéré | Statut attendu |
|---:|---|---|---|
| 1 | nombre de kits officiel validé | produit/dev | obligatoire |
| 2 | noms et familles validés | produit/sound design | obligatoire |
| 3 | hiérarchie pad par kit validée | sound design | obligatoire |
| 4 | transients kick/snare/hats validés | sound design/QA | obligatoire |
| 5 | hats open/closed validés | dev/sound design | obligatoire |
| 6 | queues crash/FX validées | sound design/QA | obligatoire |
| 7 | profils FX par famille validés | sound design/dev | obligatoire |
| 8 | patterns simples exportés | QA | obligatoire |
| 9 | patterns denses exportés | QA | obligatoire |
| 10 | mini-mix de contrôle exportés | QA | obligatoire |
| 11 | rapport QA mis à jour | QA | obligatoire |
| 12 | documentation positionnement alignée | produit | obligatoire |
| 13 | benchmark court relu | produit | recommandé |
| 14 | décision Go/No-Go signée | lead | obligatoire |

## 14. Backlog post-RC explicitement hors scope

Les éléments suivants ne doivent pas bloquer la RC si les critères précédents sont atteints :
- refonte UI ;
- passage de 12 à 16 pads ;
- per-pad FX complet ;
- moteur de sampling ;
- humanisation avancée ;
- extension massive de kits ;
- émulation acoustique réaliste ;
- nouveau système de routing multi-output avancé ;
- bibliothèque techno spécialisée large.

## 15. Conclusion de plan

Le chemin le plus sûr vers une RC est de réduire l'ambition affichée, stabiliser la banque actuelle, rendre chaque kit jouable en pattern réel, différencier les effets par famille et documenter honnêtement le positionnement.

Le **Go RC** doit être refusé tant que la contradiction **25 kits annoncés / 18 kits observés** n'est pas résolue. Une fois ce point fermé, les principaux risques deviennent sonores : hiérarchie pad, transients, queues, chevauchements et FX.

Le produit peut atteindre une RC crédible s'il est assumé comme drum synth procédural stylisé plutôt que comme solution universelle de batterie réaliste.
