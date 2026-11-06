# Passe Windows du lot 6 — preuves et banc Direct3D 11 plein écran

Joué le 2026-11-06 sur le Dell Latitude 5450 (écran principal 1920 × 1080 à 60 Hz, Windows 11), branche
`feat/livraison-complete--pc-lot6-preuves`. Tête de `feat/livraison-complete` prise : e4aa3e94 (lot 6 mergé, lot 8 en
cours jusqu'à L8-T7). Banc sur 0e5efff1, `scripts/bench.ps1 -Rendu d3d11 -SansConstruire` sous le jeton `ecran`.
Machine chargée (autres sessions actives, environ 2 Go de mémoire libre). Référence : `docs/mesures/2026-10-21-lots-10-et-3.md` ;
dernier banc : `docs/mesures/2026-11-04-lot5-cloture.md`.

## Suites et contrôles

- `scripts/test.ps1 -Preset release` : **235/235, `TESTS OK`, 0 avertissement** (N Windows = 235, lot 8 en cours compris).
  Sautés (code 77, attendus) : `tst_formats_oracle`, `tst_formats_oracle_synth`, `tst_flipchart_miniatures_samples`
  (variables locales absentes), cinq `tst_captures_*` du lot 8 (talons).
- `scripts/test.ps1 -Preset debug` : 234/235, 0 avertissement ; seul échec `tst_captures_proprietes` (pixel), corrigé
  (0e5efff1) puis rejoué vert avec les tests touchés par les renommages. Suite debug complète non rejouée sur la tête finale.
- `namesAreOurs` (L6-T6) vert avec `ACTCRAFT_REFERENCE_NAMES` (liste hors dépôt, noms anglais relevés et noms français supposés).
- `NOMS OK : 910 fichiers` après 57d10f6c ; contrôle des titres du corpus vide.
- `captures.ps1 -Scenario tous -Rendu d3d11` : 41/42 ; `proprietes` en écart (`contour-annule`, moyenne 3 × 3 sur le
  bord du trait fin), corrigé (9823c651), puis `CAPTURES OK` en logiciel et en D3D11.

## Banc D3D11 plein écran — verdict

**Porte de non-régression tenue sur les critères de la référence** (`stroke`, `zoom`, `scroll`), sauf `pauses` au
deuxième passage (1,37 % contre 1,02 % au plus dans la référence). Verdict brut `BUDGET DEPASSE` aux deux passages,
comme à chaque banc D3D11 depuis le lot 1 (décision 38). Deux passages valides, `export-fond` valide aux deux.

| Critère | Seuil | Lot 5 (P2) | P1 | P2 | Tenu |
|---|---|---|---|---|---|
| images/s `stroke`, `zoom`, `scroll` | ≥ 58 | 60,0 / 60,1 / 60,0 | 60,0 / 60,1 / 60,1 | 60,0 / 60,0 / 60,0 | oui |
| p95 `stroke`, `zoom`, `scroll` (ms) | ≤ 18,5 | 17,2 / 17,1 / 17,1 | 17,3 / 17,5 / 17,2 | 17,3 / 17,3 / 17,2 | oui |
| > 25 ms `stroke`, `zoom`, `scroll` | ≤ 1 % | 0,00 % | 0,11 / 0,00 / 0,28 % | 0,00 / 0,00 / 0,28 % | oui |
| figements, reprises | 0 | 0 | 0 | 0 | oui |
| latence p50 `stroke` (ms) | < 20 | 10,3 | 10,7 | 10,6 | oui |
| `pauses` > 25 ms | ≤ 1 % | 1,02 % | 1,01 % | 1,37 % | P1 comme la référence, P2 au-dessus |

## Écarts relevés (hors critères de la référence)

- `onglets` : 52,3 images/s (P1), puis **34,2 images/s, 90 % > 25 ms (P2)** ; lot 5 : 52,5. Variation forte entre passages.
- `zones-zoom` : p95 19,5 ms (P1), puis **45,8 ms, 13,4 % > 25 ms (P2)**.
- `page` : p95 66,3 / 72,9 ms (lot 5 : 49,4) ; `gommer` : p95 29,4 / 40,4 ms (lot 5 : 34,4).
- `forme-page-annotee` p95 41,8 / 42,1 ms (lot 5 : 41,4) ; `soulever` 2,46 % > 25 ms (lot 5 : 1,64 %) ; `edition` 1,6 %.
- Pic mémoire : 1 041 à 1 055 Mio (lot 5 : 1 036 Mio), au-dessus du seuil de 1 Gio comme au lot 5.
- `page-flash-unverified` vu au Mac hors écran : **écarté** en D3D11 (0 éclair sur 3 captures sans cache, aux deux passages).

Les écarts de P2 (`onglets`, `zones-zoom`) ne se voient pas en P1 : machine chargée, cause non cherchée. À rejouer
sur machine calme avant de conclure à une régression ; le lot 8 en cours est dans la tête mesurée.

## Non fait (PC fermé)

`PAQUET OK`, `verifier-licences.ps1`, suite debug complète sur la tête finale, captures à décrire (S9, S19, S20, S21),
`APK OK`, PBKDF2 sur le Dell 3510 (machine de référence de Jérôme), L6-T29 à l'écran, PR relue.
