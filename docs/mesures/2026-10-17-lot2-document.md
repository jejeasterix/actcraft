# Mesures de fin de lot 2 (document) — machine de développement

**Ce n'est pas une machine de référence.** Dell Latitude 5450 (Intel Core Ultra 5 125U, puce graphique Intel
intégrée), écran principal 1920 × 1080 à 60 Hz, Windows 11. Passages joués le 2026-10-17 en plein écran, preset
`release`, par `scripts/bench.ps1`. Le verdict est un résultat, pas une porte (décision 35 du cadrage
`docs/specs/2026-10-14-livraison-complete-cadrage.md`).

**Machine chargée.** Pendant ces passages, trois autres sessions de travail tournaient sur la machine, avec 1,1 à
2,4 Go de mémoire libre. Ces conditions ne sont pas celles de la mesure du lot 1
(`docs/mesures/2026-10-15-lot1-socle.md`, machine calme).

## Contrôles de contenu : tenus

Passage complet `bench.ps1 -Rendu tous -Captures .tmp/banc-lot2`, commit `5c0a38b`, Direct3D 11, cache, passage
valide (`valid_for_measurement`) :

| Contrôle | Attendu | Lu |
|---|---|---|
| `page_rasterizations_during_zoom` | 80 (deux échelles) | 80 |
| `page_rebuilds_during_zoom` | 0 | 0 |
| `page_flash` (sondes `page_flash_probes`) | 0 (≥ 1 sonde) | 0 (3 sondes) |

## Porte de non-régression : non jugeable sur cette machine aujourd'hui

Pour séparer l'effet de la charge de celui du lot 2, `main` (0.1.0, commit `3de027a`) et la branche du lot 2
(commit `75c7e3c`) ont été mesurés en alternance, dans les mêmes conditions, Direct3D 11, scénario complet, cache
(`bench.ps1 -Rendu d3d11 -SansConstruire`).

| Passage | Version | RAM libre | stroke p95 (ms) | zoom p95 (ms) | scroll p95 (ms) | stroke > 25 ms | latence p50 / p95 (ms) | page p95 (ms) |
|---|---|---|---|---|---|---|---|---|
| 1 | 0.1.0 | 2,36 Go | 36,4 | 71,5 | 34,6 | 28,8 % | 17,9 / 50,1 | — |
| 2 | lot 2 | 1,51 Go | 34,3 | 36,2 | 48,0 | 18,6 % | 15,7 / 49,3 | 362,2 |
| 3 | 0.1.0 | 1,13 Go | 70,5 | 260,0 | 125,8 | 15,7 % | 20,7 / 360,1 | — |
| 4 | lot 2 | 1,58 Go | 49,9 | 130,6 | 42,2 | 19,3 % | 17,5 / 118,2 | 163,9 |

Le passage 3 a d'abord été déclaré invalide (`invalid-refresh-period`), puis rejoué automatiquement ; la ligne donne
le second essai. Images/s : 57,9 à 59,8 pour stroke, zoom et scroll dans les quatre passages, sauf le zoom du
passage 3 (36,1).

Lecture : la 0.1.0, inchangée depuis le 2026-10-15 où elle tenait 17 ms de p95 partout, dépasse ici le budget
autant que le lot 2. **L'écart avec la mesure du lot 1 vient de la charge de la machine, pas du lot 2** ; dans les
mêmes conditions, le lot 2 fait jeu égal ou mieux que la 0.1.0. Aucun critère ne peut être déclaré tenu ou perdu
sur ces passages.

À refaire sur machine calme (aucune autre session active, mémoire libre ≥ 4 Go), même commande :
`pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu tous -Captures .tmp/banc-lot2`, puis comparer au tableau du lot 1.

## Scénario `page` (nouveau au lot 2)

Pas de référence antérieure. Sous charge : 47,0 et 58,3 images/s, p95 de 362 et 164 ms, aucun éclair blanc. Le
p95 mesure les images qui suivent un changement de page (décodage des images de la page). À juger sur machine
calme avant d'en tirer une conclusion.

## Fichiers

Dans `bench/results/` (ignoré par git) :

- passage complet : `2026-10-17_185409-Dell-Inc-Latitude-5450-direct3d11.json`,
  `2026-10-17_185511-Dell-Inc-Latitude-5450-software.json` ;
- 0.1.0 (worktree de mesure) : `2026-10-17_191043-…-direct3d11.json`, `2026-10-17_191512-…-direct3d11.json` ;
- lot 2 : `2026-10-17_191213-…-direct3d11.json`, `2026-10-17_192044-…-direct3d11.json`.

## Rejeu du 2026-10-18 (machine plus calme)

Même commande (`bench.ps1 -Rendu d3d11 -SansConstruire`), Direct3D 11, plein écran, cache, en alternance : 0.1.0
(`3de027a`) et **0.2.0 exacte** (`70c0493`, construite en release pour la mesure). Quatre passages valides, contrôles de
contenu réussis. Charge relevée au début de chaque passage : processeur 0 % ; mémoire libre en baisse (2,85 → 1,19 Go,
une autre session a démarré pendant la mesure).

| Passage | Version | RAM libre | p95 stroke / zoom / scroll (ms) | > 25 ms stroke / zoom / scroll | latence p50 / p95 (ms) | figements hors limite (stroke) | pic mémoire max (Mo) |
|---|---|---|---|---|---|---|---|
| 1 | 0.1.0 | 2,85 Go | 17,1 / 17,2 / 17,1 | 0,00 / 0,00 / 0,28 % | 10,7 / 18,0 | 6 | 696 |
| 2 | 0.2.0 | 2,20 Go | 22,6 / 19,8 / 17,9 | 4,88 / 1,40 / 2,26 % | 11,8 / 32,0 | 10 | 842 (`page`) |
| 3 | 0.1.0 | 1,62 Go | 17,1 / 17,4 / 17,1 | 0,00 / 0,00 / 0,00 % | 10,7 / 18,2 | 5 | 693 |
| 4 | 0.2.0 | 1,19 Go | 17,0 / 17,2 / 17,1 | 0,00 / 0,00 / 0,00 % | 10,8 / 18,2 | 7 | 834 (`page`) |

Images/s : 59,9 à 60,1 partout.

**Lecture.** Le passage 4 de la 0.2.0 tient tous les critères que la 0.1.0 tient (images/s ≥ 58, p95 ≤ 18,5 ms,
images > 25 ms ≤ 1 %, reprises 0, latence p50 < 20 ms, pic mémoire < 1 024 Mo) ; le passage 2, juste après la
construction de la 0.2.0, les dépasse sur `stroke` et `zoom`. Les figements hors limite de `stroke` (5 à 10 pour les
deux versions, 1 au lot 1 sur machine calme) ne font pas partie de la porte avant le lot 10. **Verdict : pas de
régression attribuable au lot 2 sur les scénarios `stroke`, `zoom`, `scroll` (1 passage sur 2 de la 0.2.0 tenu, l'autre
dégradé sous charge) ; à confirmer à la clôture du lot 10** (0.1.0, 0.2.0 et 0.3.0 en alternance, plan du lot 10). Le
scénario `page` (nouveau) monte le pic mémoire à 834-842 Mo, sous le budget de 1 024 Mo.

Fichiers : 0.1.0 `2026-10-18_081852-…`, `…_082352-…` ; 0.2.0 `…_082237-…`, `…_082452-…` (`-direct3d11.json`, dans
`bench/results/` des worktrees de mesure).
