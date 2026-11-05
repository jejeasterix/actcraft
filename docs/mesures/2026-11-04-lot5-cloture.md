# Banc de clôture du lot 5 (version 0.6.0) — Direct3D 11, plein écran

Joué le 2026-11-04 sur le Dell Latitude 5450 (écran principal 1920 × 1080 à 60 Hz, Windows 11), tête `c3ea5a8`,
`scripts/bench.ps1 -SansConstruire` sous le jeton `ecran`. Machine chargée : trois autres sessions actives,
environ 1,5 Go de mémoire libre. Référence : `docs/mesures/2026-10-21-lots-10-et-3.md`.

## Verdict

**Porte de non-régression tenue.** Tout critère tenu par la référence l'est encore. Le verdict brut reste
`BUDGET DEPASSE`, comme à chaque banc D3D11 depuis le lot 1 (décision 38 : un résultat, pas une porte).

## Passages

- P1 (`2026-11-04_224038-…-direct3d11.json`) : `MESURE INVALIDE`, `export-fond` en `content-check-failed`.
  Les autres scénarios ont des valeurs proches de P2.
- P2 (`2026-11-04_224412-…-direct3d11.json`) : passage valide, `BUDGET DEPASSE`.
- Rendu logiciel : informatif.

## Critères de la référence (P2)

| Critère | Seuil | Référence 0.4.0 | 0.6.0 P2 | Tenu |
|---|---|---|---|---|
| images/s `stroke`, `zoom`, `scroll` | ≥ 58 | 60,0 | 60,0 / 60,1 / 60,0 | oui |
| p95 `stroke`, `zoom`, `scroll` (ms) | ≤ 18,5 | 16,9 à 17,0 | 17,2 / 17,1 / 17,1 | oui |
| > 25 ms `stroke`, `zoom`, `scroll` | ≤ 1 % | 0,00 % | 0,00 % | oui |
| figements, reprises `stroke`, `zoom`, `scroll` | 0 | 0 | 0 | oui |
| latence p50 `stroke` (ms) | < 20 | 9,97 | 10,3 | oui |
| `pauses` > 25 ms | ≤ 1 % | 0,34 % / 1,02 % / 1,02 % | 1,02 % | comme la référence |

## Pertes déjà présentes dans la référence

- `page` : p95 49,4 ms, > 25 ms 43 % (référence : mêmes pertes en P2 à P6).
- `gommer` : p95 34,4 ms, > 25 ms 6,2 % (référence P6 D3D11 : 32,9 ms, 7,69 %).
- `all.memory_peak_bytes` : 1 036 Mio (lot 10 : 1 653 à 1 723 Mio).

## Scénarios hors référence (relevés)

- `onglets` : 52,5 images/s, p95 19,5 ms. Déjà hors budget au banc logiciel du Mac.
- `forme-page-annotee` : p95 41,4 ms, > 25 ms 5,7 %. Déjà hors budget au banc logiciel du Mac.
- `soulever` 1,64 %, `miniatures-pauses` 1,01 % (> 25 ms).

## H5-28 : export en fond

`export-fond` contre `export-fond-reference` (charge témoin `SpinningLoad`) : rapport 1,000 (P1) et 0,991 (P2),
environ 105 000 tours de charge. Le banc n'a pas de mode « référence sans charge » : la comparaison sans charge
n'est pas mesurée.

## Reliquat

`export-fond` invalide une fois sur deux (`content-check-failed` en P1). Cause non cherchée.
