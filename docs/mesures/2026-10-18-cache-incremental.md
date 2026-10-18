# Mesure S13 du cache incrémental (lot 10, L10-T6) — machine de développement

**Ce n'est pas une machine de référence.** Dell Latitude 5450 (Intel Core Ultra 5 125U, puce graphique Intel
intégrée), écran principal 1920 × 1080 à 60 Hz, Windows 11, 15,5 Go de mémoire. Passages joués le 2026-10-18 de
15 h 39 à 15 h 51, en plein écran sur l'écran principal, Direct3D 11, cache actif, preset `release`, par
`scripts/bench.ps1 -Rendu d3d11 -SansConstruire` sous le jeton `ecran`. Le verdict est un résultat, pas une porte
(décision 35 du cadrage `docs/specs/2026-10-14-livraison-complete-cadrage.md`).

- **Référence** : `15d76d0` (merge de L10-T1 : ancien cache, banc à cinq scénarios), worktree de mesure détaché.
- **Lot 10** : `05da054` (tête de `feat/livraison-complete` après le merge de L10-T4 : versement différé, couche
  d'attente, MSAA 4× de L10-T5).
- Ordre joué : référence → lot 10 → lot 10 → lot 10 → référence, sans passage écarté. Les cinq passages sont valides
  (`valid_for_measurement`, contrôles de contenu réussis) ; aucun n'a été rejoué, ni par `bench.ps1` ni à la main.

## Verdict S13 : tenu

Sur les trois passages du lot 10, `freeze_over_limit = 0` et `resume_over_limit = 0` dans les cinq scénarios, dont
`pauses`. La référence, elle, garde un figement hors limite dans `stroke` à chacun de ses deux passages (43,9 et
50,3 ms), tous deux pendant un redessin du cache. **Décision sur L10-T7 : non requise** (machine calme, S13 tenu).

Le verdict global du banc reste « BUDGET DEPASSE » pour les cinq passages, pour des raisons hors S13 (voir
« Critères perdus hors S13 »).

## Condition « machine calme »

Règle de Jérôme du 2026-10-18 : l'état présent de la machine vaut « machine calme » ; le seuil de 4 Go libres ne
s'applique plus. Relevé avant chaque passage : mémoire libre (`Win32_OperatingSystem`), charge du processeur
(compteur `Win32_PerfFormattedData_PerfOS_Processor`, trois lectures), compilations en cours (`cl.exe`,
`ninja.exe`, `link.exe`), état des jetons.

| Passage | Heure | RAM libre | Processeur (3 lectures) | Compilations | Jeton `tests` | Jeton `ecran` avant prise |
|---|---|---|---|---|---|---|
| P1 référence | 15:39:51 | 3,04 Go | 67 % (71, 63, 68) | aucune | libre | libre |
| P2 lot 10 | 15:41:20 | 3,01 Go | 28 % (28, 29, 27) | aucune | libre | libre |
| P3 lot 10 | 15:42:38 | 3,21 Go | 27 % (25, 25, 32) | aucune | libre | libre |
| P4 lot 10 | 15:44:01 | 3,11 Go | 66 % (80, 68, 51) | aucune | libre | libre |
| P5 référence | 15:45:27 | 2,76 Go | 84 % (85, 84, 82) | aucune | libre | libre |
| 4 bis référence | 15:50:02 | 2,68 Go | 36 % (34, 38, 35) | aucune | libre | libre |
| 4 bis lot 10 | 15:50:30 | 2,48 Go | 48 % (48, 49, 48) | aucune | libre | libre |

Aucune compilation d'une autre session n'a tourné : aucune attente. La charge du processeur lue par ce compteur
est haute sans compilation ; pendant P1, la somme des processus visibles relevée par leur temps processeur était
d'environ 14 % (non expliqué : la lecture WMI elle-même compte peut-être). Les retards d'injection de `pauses`
montent à 17 et 34 ms aux passages P3 et P4 : la machine n'était pas parfaitement stable pendant ces deux passages.

## Table par passage et par scénario

Images/s · p95 (ms) · images > 25 ms · figements hors limite (max ms) · reprises hors limite (max ms) ·
`cache.refreshes` · `cache.pending_max` (absent du banc de la référence) · latence p50 / p95 (ms) · pic mémoire du
scénario, privée / GPU (Mio). Limite d'un figement ou d'une reprise : 2,5 périodes = 41,65 ms.

| Passage | Scénario | img/s | p95 | > 25 ms | figements | reprises | redessins | attente max | latence | mémoire |
|---|---|---|---|---|---|---|---|---|---|---|
| P1 réf. | stroke | 60,0 | 17,0 | 0,00 % | **1** (43,9) | 0 (16,8) | 10 | — | 10,2 / 17,7 | 341 / 171 |
| | zoom | 60,0 | 17,4 | 0,00 % | 0 | 0 | 1 | — | — | 331 / 171 |
| | scroll | 60,0 | 17,0 | 0,00 % | 0 | 0 | 0 | — | — | 444 / 266 |
| | page | 45,4 | 106,6 | 45,45 % | 0 | 0 | 23 | — | — | 565 / 270 |
| | pauses | 60,1 | 17,1 | 0,00 % | 0 (30,1) | 0 (16,3) | 10 | — | 10,3 / 17,7 | 613 / 211 |
| P2 lot 10 | stroke | 60,0 | 16,9 | 0,00 % | 0 (17,6) | 0 (17,0) | 0 | 10 | 9,9 / 17,4 | 478 / 267 |
| | zoom | 60,0 | 17,0 | 0,00 % | 0 | 0 | 0 | 0 | — | 439 / 289 |
| | scroll | 60,0 | 16,9 | 0,28 % | 0 | 0 | 0 | 0 | — | 939 / 784 |
| | page | 36,2 | 76,7 | 53,19 % | 0 | 0 | 23 | 0 | — | 768 / 412 |
| | pauses | 60,1 | 16,9 | 0,68 % | 0 (17,9) | 0 (15,3) | 8 | 2 | 10,2 / 17,3 | 755 / 316 |
| P3 lot 10 | stroke | 60,0 | 17,0 | 0,00 % | 0 (17,4) | 0 (18,8) | 0 | 10 | 10,2 / 17,5 | 487 / 256 |
| | zoom | 60,0 | 17,0 | 0,00 % | 0 | 0 | 0 | 0 | — | 426 / 276 |
| | scroll | 60,0 | 17,0 | 0,28 % | 0 | 0 | 0 | 0 | — | 925 / 728 |
| | page | 55,3 | 92,8 | 45,79 % | 0 | 0 | 23 | 0 | — | 693 / 415 |
| | pauses | 60,0 | 17,0 | 1,04 % | 0 (17,8) | 0 (16,9) | 8 | 2 | 10,1 / 17,8 | 749 / 320 |
| P4 lot 10 | stroke | 60,1 | 17,1 | 0,00 % | 0 (17,9) | 0 (24,2) | 0 | 10 | 10,0 / 17,5 | 477 / 267 |
| | zoom | 60,0 | 17,1 | 0,00 % | 0 | 0 | 1 | 0 | — | 439 / 289 |
| | scroll | 60,0 | 17,0 | 0,00 % | 0 | 0 | 0 | 0 | — | 938 / 752 |
| | page | 37,5 | 106,2 | 53,61 % | 0 | 0 | 23 | 0 | — | 721 / 400 |
| | pauses | 60,0 | 17,2 | 3,19 % | 0 (18,3) | 0 (41,1) | 9 | 1 | 11,1 / 20,7 | 771 / 376 |
| P5 réf. | stroke | 60,0 | 17,0 | 0,11 % | **1** (50,3) | 0 (18,2) | 10 | — | 10,1 / 17,6 | 340 / 160 |
| | zoom | 60,0 | 17,2 | 0,00 % | 0 | 0 | 0 | — | — | 315 / 155 |
| | scroll | 60,0 | 17,0 | 0,00 % | 0 | 0 | 0 | — | — | 428 / 268 |
| | page | 49,5 | 81,7 | 38,30 % | 0 | 0 | 23 | — | — | 564 / 270 |
| | pauses | 60,1 | 17,0 | 0,00 % | 0 (38,2) | 0 (17,5) | 10 | — | 10,6 / 17,9 | 579 / 211 |

Pic mémoire du banc (`memory_peak_bytes`, privée + GPU du scénario le plus haut, budget 1 024 Mio) : référence 835 et
834 Mio (`page`) ; lot 10 1 723, 1 653 et 1 690 Mio (`scroll`).

Dans `stroke`, le lot 10 ne redessine pas le cache : les dix traits restent en attente (`pending_max = 10`) et sont
versés au premier zoom du scénario suivant. La reprise de 41,1 ms de `pauses` au passage P4 est la toute première
du scénario (k = 0) : sous la limite de 0,55 ms.

## Attribution des figements et reprises hors limite

Méthode de l'étape 3 du plan : fenêtre de redessin = de chaque `raw.cache_request_ns` à la première
`raw.cache_updated_ns` qui la suit ; un intervalle F ou R hors limite (bornes `raw.swap_ns[k-1]`, `raw.swap_ns[k]`)
qui recoupe une fenêtre est classé « redessin ». Les durées sont recalculées depuis `raw` comme
`bench/src/stats.cpp` (`classifyIntervals`) ; le recomptage retrouve exactement les compteurs du banc et la
classification de `raw.interval_class` sur `stroke`, `zoom`, `scroll` et `pauses` (sur `page`, 65 à 85 intervalles
diffèrent, sans aucun F ni R hors limite d'un côté comme de l'autre).

| Passage | Intervalle | Durée | Fenêtre de redessin recoupée | Cause |
|---|---|---|---|---|
| P1 réf. | `stroke`, F k = 457, 8 665,8 → 8 709,7 ms | 43,9 ms | 8 675,4 → 8 710,1 ms | redessin du cache |
| P5 réf. | `stroke`, F k = 456, 8 667,1 → 8 717,4 ms | 50,3 ms | 8 675,2 → 8 718,0 ms | redessin du cache |

Lot 10 : aucun figement ni reprise hors limite à attribuer.

**Versement dans `pauses`** (reprises k ≥ 1, jugé seulement si le max des retards d'injection et de réception reste
sous 8 ms) :

| Passage | Retards max (inj. / réc.) | Reprises jugées | Recoupant un redessin | Hors limite | Reprise jugée max |
|---|---|---|---|---|---|
| P1 réf. | 2,46 / 2,50 ms | 8 | 0 | 0 | 16,3 ms |
| P2 lot 10 | 3,30 / 3,36 ms | 4 | 3 (15,3 ; 3,4 ; 3,2 ms) | 0 | 15,3 ms |
| P3 lot 10 | 17,32 / 17,39 ms : **hors fenêtre** | (5) | (3) | (0) | (16,9 ms) |
| P4 lot 10 | 33,95 / 34,05 ms : **hors fenêtre** | (3) | (3) | (0) | (20,3 ms) |
| P5 réf. | 2,81 / 2,97 ms | 8 | 1 (8,5 ms) | 0 | 17,5 ms |

Au seul passage du lot 10 dans la fenêtre (P2), le versement tombe sur trois reprises sur quatre et aucune ne
dépasse 15,3 ms : le redessin différé ne se voit pas à la reprise.

## Décision sur L10-T7 : non requise

Machine calme (règle du 2026-10-18), S13 tenu sur les trois passages du lot 10, aucun figement ni reprise hors limite
attribué à un redessin du cache. Ni les tuiles (L10-T7) ni le cache par groupe de couches ne sont requis par cette
mesure. H10-2 (500 ms, 128 objets) : aucun réglage demandé par la mesure.

## Étape 4 bis : mémoire au zoom 800 % posé (H10-9)

Scénario temporaire `.tmp/zoom800.json` (`waitReady`, `view.zoom` 800, `waitReady`, attente 3 000 ms, `capture`)
joué par `build/release/actcraft-captures.exe --scenario .tmp/zoom800.json --rendu d3d11 --taille 1280x800 --sortie
.tmp/zoom800`, sous le jeton `ecran`, référence puis lot 10. Relevés toutes les 500 ms : mémoire GPU dédiée et
partagée du processus (compteurs « GPU Process Memory », instances `pid_<pid>_*`) et `PrivateMemorySize64`.

**Rejouée une fois.** Le premier essai (15:47) n'a pas tenu la cadence : `Get-Counter` attend 1 s par lecture, le
premier relevé est arrivé à 5,6 s et les suivants toutes les secondes. Le second essai lit les mêmes compteurs par
`System.Diagnostics.PerformanceCounter` (quelques millisecondes par lecture) : 10 et 13 relevés à 500 ms. Les deux
essais donnent le même plateau (lot 10 : 1 261,6 puis 1 261,5 Mio).

| Version | Pic dédié | Pic partagé | Pic privé | Pic total au même relevé |
|---|---|---|---|---|
| Référence `15d76d0` | 0 Mio | 161,5 Mio | 226,4 Mio | **387,9 Mio** |
| Lot 10 `05da054` | 0 Mio | 586,8 Mio | 674,7 Mio | **1 261,5 Mio** |

Sur le lot 10, la mémoire partagée passe de 115 à 587 Mio quand le zoom 800 % est posé, puis reste à ce plateau
pendant les 3 s d'attente. La puce intégrée n'a pas de mémoire dédiée.

**Décision H10-9 : repli requis** (1 261,5 Mio > 1 Gio). Repli choisi : **plafond de la texture du cache
(`kMaxCacheSide`) abaissé sous MSAA**. Raisons :
- le dépassement suit la texture du cache multi-échantillonnée : il apparaît au zoom posé, quand la texture atteint
  sa taille maximale ;
- l'autre repli (fenêtre à 1, cache à 4) garde le MSAA 4× du cache. Il ne libère que les tampons multi-échantillonnés
  de la fenêtre : environ 64 Mio en 1920 × 1080, estimés, non mesurés. Il faut en libérer 238 pour revenir sous
  1 Gio ;
- le banc va dans le même sens : le scénario `scroll` (zoom 400 %) monte à 1 653-1 723 Mio sur le lot 10, contre
  696-710 Mio à la référence. On suppose la même cause (texture du cache à fort zoom), sans l'avoir vérifié.

Coût à mesurer dans la tâche de repli : netteté au-delà du zoom où le nouveau plafond est atteint ; pic au zoom
800 % posé et `memory_peak_bytes` du banc repassés sous 1 Gio. Tâche à écrire par l'orchestrateur.

## Critères perdus hors S13

- **`all.memory_peak_bytes`** : perdu par le lot 10 aux trois passages (1 653 à 1 723 Mio, `scroll`), tenu par la
  référence (834-835 Mio). Relève du repli de H10-9 ci-dessus.
- **`pauses.over_25ms_ratio`** : perdu par le lot 10 à P3 (1,04 %) et P4 (3,19 %), tenu par la référence (0 %).
  Ce sont des intervalles **actifs** de 25 à 50 ms, que S13 ne compte pas : 2 à P2 (33,4 et 50,4 ms), 3 à P3, 9 à
  P4 (souvent la deuxième image après une reprise). Par la même règle de recouvrement, tous sont classés « autre »
  sauf un (P4, k = 185, 44,9 ms, « redessin »). P3 et P4 ont des retards d'injection de 17 et 34 ms. La cause n'est
  pas établie : **à rapporter à l'orchestrateur** (tâche de correction ou d'enquête, hors tuiles).
- **`page`** : images/s, p95 et images > 25 ms dépassés aux cinq passages, référence comprise (H2b-5, H2b-7).
  Antérieur au lot 10.

## Repli H10-9 (L10-T8)

### Modèle et prédictions, écrits avant la mesure

Texture du cache multi-échantillonnée 4× (`QSGRhiLayer`) : couleur multi-échantillonnée 16 o, profondeur-stencil
multi-échantillonnée 16 o, texture résolue 4 o, soit 36 o par pixel ; 20 o sans profondeur-stencil. À 4096 × 3072
(12,58 Mpx) : 432 Mio, ou 240 Mio sans profondeur-stencil. Si la mémoire partagée est comptée aussi dans la mémoire
privée (question ouverte de l'étape 4 bis), chaque Mio gagné sur la texture compte deux fois dans le total.

| Variante | Texture à 800 % | Prédiction 800 % posé | Prédiction banc (`scroll`, 400 %) | Netteté prédite |
|---|---|---|---|---|
| V0 tête, plafond 4096 | 4096 × 3072, 432 Mio | ≈ 1 300 Mio (fenêtre 1920 × 1080 au lieu de 1280 × 800 : + 35 à 70 Mio) | 1 650 à 1 720 Mio | base |
| V1 sans profondeur 2D | 4096 × 3072, 240 Mio si la profondeur-stencil disparaît | − 192 à − 384 Mio, sinon ≈ 0 | − 192 à − 384 Mio : reste > 1 024 | = V0 |
| V2 plafond 2048 | 2048 × 1536, 108 Mio | − 324 à − 648 Mio : < 1 Gio | 1 000 à 1 400 Mio : incertain | bande ≈ 2 × V0 à 400 % |
| V3 V1 + 3072 / 2896 | 135 / 120 Mio sans profondeur, 243 / 216 avec | entre V1 et V2 | entre V1 et V2 | bande ≈ 1,3 à 1,4 × V0 |
| V4 cache coupé | aucune | ≈ référence (≈ 400 Mio) | bas ; coût d'image du rendu direct inconnu | nette (rendu direct) |

Prédiction propre à V1 : `QSGRhiLayer` garde peut-être sa profondeur-stencil, nécessaire au découpage par stencil ;
gain alors nul.

### Méthode

Worktree de L10-T8, preset `release`, 2026-10-18 de 16 h 44 à 18 h, Direct3D 11, plein écran 1920 × 1080. Par
variante, sous le jeton `ecran` : un passage `bench.ps1 -Rendu d3d11 -SansConstruire`, puis le zoom 400 % et le zoom
800 % posés. Scénario de zoom (temporaire, `.tmp/zoom<Z>.json`, `actcraft-captures --taille plein-ecran`) : trait noir
de 8 px tracé à 100 % de (462, 334) à (562, 434), versé au cache, `checkEdge` de (496, 384) à (528, 384) à 100 %, zoom
posé, attente 3 s, `checkEdge` au zoom posé. Mémoire relevée toutes les 500 ms (`PerformanceCounter`, « GPU Process
Memory » et `PrivateMemorySize64`) ; le total est pris au même relevé. Relevé « machine calme » avant chaque passage
(`calme.ps1`) ; une attente de 141 s pour une compilation de L10-C1 avant V1.
- V0 : tête `d5832c3`. V1 : même binaire, `QSG_NO_DEPTH_BUFFER=1`, l'interrupteur documenté de
  `QQuickGraphicsConfiguration::setDepthBufferFor2D` (valeur par défaut de la configuration). V4 : même binaire,
  `-SansCache` et `--sans-cache` (plan : coût d'image du rendu direct à 400 %).
- V2, V3 : mutation locale de `PageView::updateCacheTextureSize` (plafond lu dans une variable d'environnement,
  plancher « jamais flou »), remise à zéro par `git checkout --` après la série.
- Colonnes `pauses` (demande de l'orchestrateur, L10-T9) : `over_25ms_ratio`, et nombre d'entrées dont
  `latency_planned_to_sync_ms` dépasse 20 ms et dont l'heure prévue tombe dans les 100 ms qui suivent un
  `raw.cache_request_ns`.

### Table des variantes

Mémoire en Mio. Banc : `all.memory_peak_bytes`. Zoom posé : pic du total, partagé / privé au même relevé. Bande :
pixels intermédiaires de `checkEdge` (100 % → 2 dans toutes les variantes).

| Variante | Banc | 800 % (partagé / privé) | 400 % (partagé / privé) | S13 | p95 `scroll` | Bande 400 / 800 | `pauses` > 25 ms | Entrées > 20 ms après demande |
|---|---|---|---|---|---|---|---|---|
| V0 tête | **1 692,1** | **1 462,7** (658,2 / 804,5) | 1 501,3 (696,9 / 804,4) | tenu | 17,2 | 2 / 8 | 2,45 % | 41 sur 41 (9 demandes) |
| V1 sans profondeur 2D | **1 047,4** | 866,1 (377,1 / 489,0) | 875,8 (376,8 / 498,9) | tenu | 17,2 | 2 / 8 | 2,78 % | 33 sur 34 (8 demandes) |
| V2 plafond 2048 | **1 141,2** | 426,9 (relevé invalide : compteurs GPU à 0) | 332,5 (invalide) | tenu | 17,1 | **8** / 16 | 4,03 % | 65 sur 94 (8 demandes) |
| **V3 V1 + 3072** | **901,6** | **624,4** (281,8 / 342,5) | 625,2 (273,9 / 351,4) | tenu | 17,0 | 4 / 12 | 2,46 % | 44 sur 44 (8 demandes) |
| V3 V1 + 2896 | 920,0 | 584,8 (252,4 / 332,4) | 583,6 (260,3 / 323,2) | tenu | 17,1 | 4 / 12 | 2,81 % | 38 sur 38 (8 demandes) |
| V4 sans cache (`-SansCache`) | 991,5 | 400,2 (158,2 / 242,0) | 388,1 (164,8 / 223,3) | **perdu** (`stroke`, `pauses`) | 18,2 | 2 / 2 | 25,70 % | 0 (aucune demande ; 410 entrées > 20 ms) |

Constats :
- **La mémoire partagée est aussi comptée dans la mémoire privée** : de V0 à V1, le partagé baisse de 281 Mio et le
  privé de 316 Mio au zoom 800 %. Chaque Mio de texture gagné compte deux fois dans le total.
- **V1 retire bien la profondeur-stencil des couches** : − 597 Mio au total à 800 %, plus que les 192 Mio prédits
  pour le seul cache (la fenêtre et ses autres couches en perdent aussi). Netteté inchangée. Mais le banc reste à
  1 047,4 Mio : V1 seule n'est pas retenue (critère < 1 024 Mio).
- V4 : rendu direct à 400 % tenable (p95 `scroll` 18,2 ms ≤ 18,5), mais le passage sans cache perd S13 dans
  `stroke` et `pauses`, à 100 % ; V4 n'y couperait pas le cache. Texture libérée à l'exécution non vérifiée (V4 non
  écrite). Dernière dans l'ordre de préférence.
- V2 seule (sans V1) reste au-dessus du budget au banc (1 141,2 Mio) et double la bande à 400 % (8 > limite 5).
- `pauses` : aucune variante avec cache ne règle les images lentes ; les entrées en retard tombent presque toutes
  dans les 100 ms qui suivent une demande de redessin, quel que soit le plafond (données pour L10-T9).

### Variante retenue : V3, plafond 3072

Règle de choix appliquée par script (`decide.py`, sortie : `CHOIX V3-3072 3072 v1`). Limite de bande à 400 % :
⌈bande(V0) × 4096 / plafond⌉ + 1, avec bande(V0) = 2.

| Variante | Banc < 1 024 | 800 % < 1 024 | S13 | p95 `scroll` ≤ 18,5 | Bande 400 % ≤ limite | Retenue |
|---|---|---|---|---|---|---|
| V1 (plafond 4096) | non (1 047,4) | oui | oui | oui | 2 ≤ 3 | non |
| V3 3072 | oui (901,6) | oui (624,4) | oui | oui (17,0) | 4 ≤ 4 | **oui** |
| V3 2896 | oui (920,0) | oui (584,8) | oui | oui (17,1) | 4 ≤ 4 | oui, plafond plus petit |
| V2 2048 | non (1 141,2) | oui | oui | oui | 8 > 5 | non |

V3 : V1 n'est pas retenue seule, et 3072 est le plus grand plafond qui tient les deux critères mémoire. Ordre de
préférence : V1, puis V3, puis V2, puis V4.

**Zoom à partir duquel l'image devient floue** (plafond ÷ côté de la page × DPR ; plancher « jamais flou ») :

| Page | DPR 1, V0 (4096) | DPR 1, V3 (3072) | DPR 2, V0 | DPR 2, V3 |
|---|---|---|---|---|
| 1024 × 768 | 400 % | 300 % | 200 % | 150 % |
| 1920 × 1080 | 213 % | 160 % | 107 % | au-delà de 100 % (plancher 3 840), jamais à 100 % |

**Écarts au plan, écrits ici :**
- Le plancher prend aussi le zoom d'« Ajuster » : `côté = min(4096, max(3072, côté de la page à max(1, zoom
  d'ajustement) × DPR))`. La formule du plan (côté à 100 %) laissait floue une page de 1024 ajustée à environ 115 %
  à DPR 2 (2 360 px demandés, plancher 2 048), contraire à l'objectif « jamais flou à l'ajustement ».
- `PageView.cacheSamples` vaut `kCacheMsaaSamples` (4) en RHI et 1 en rendu logiciel, où `samples` est sans effet :
  c'est ce que demande le test « cache forcé à 4 », joué en rendu logiciel. En conséquence, la garde de
  `bench/scenarios/anticrenelage.json` (`expect pageCache.samples 4`) devient une garde de liaison vraie dans les
  deux moteurs : `remember pageView.cacheSamples`, puis `expect pageCache.samples = ${echantillons_cache}`
  (autorisation de l'orchestrateur, seul fichier de `bench/scenarios/` modifié).
- V1 est écrite par l'API publique `QQuickGraphicsConfiguration::setDepthBufferFor2D(false)`, posée dans
  `loadMainWindow` (`app/startup.cpp`) : la fenêtre de `Main.qml` est montrée au chargement mais n'est exposée qu'au
  retour à la boucle d'événements. Mesurée avec `QSG_NO_DEPTH_BUFFER=1`, l'interrupteur documenté par Qt ; l'effet
  de l'API est vérifié par les relevés après repli ci-dessous.

## Fichiers

Dans `bench/results/` des deux worktrees (ignoré par git) :
- référence : `2026-10-18_154013-Dell-Inc-Latitude-5450-direct3d11.json` (P1),
  `2026-10-18_154548-…-direct3d11.json` (P5) ;
- lot 10 : `2026-10-18_154133-…` (P2), `2026-10-18_154250-…` (P3), `2026-10-18_154415-…` (P4), `-direct3d11.json`.

Le worktree de référence est supprimé après la mesure ; ses deux fichiers de résultat sont copiés avant dans
`bench/results/reference-15d76d0/` du worktree du lot 10 (ignoré par git).
