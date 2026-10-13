# Mesures du prototype — machine de développement

**Ce n'est pas une machine de référence.** Dell Latitude 5450 (Intel Core Ultra 5 125U, puce graphique Intel
intégrée), écran principal 1920 × 1080 à 60 Hz, Windows 11. Passages joués le 2026-10-14 en plein écran, preset
`release` (RelWithDebInfo, `/Ob2`), commit `283df8a`, scénario complet, par `scripts/bench.ps1` puis
`scripts/bench.ps1 -SansCache -SansConstruire`. Les quatre passages sont valides (`valid_for_measurement`) et
leurs quatre contrôles de contenu réussis. Pendant ce chantier, le verdict est un résultat, pas une porte
(décision 38).

**Verdicts.** Direct3D 11 avec cache : `BUDGET DEPASSE` pour un seul critère ; tous les autres tiennent.
Direct3D 11 sans cache : `BUDGET DEPASSE` sur les trois scénarios. Rendu logiciel : informatif (N1-3).

**Rapport chiffré du dépassement (Direct3D 11 avec cache).**

| Critère | Scénario | Valeur | Seuil |
|---|---|---|---|
| figements hors limite (`freeze_over_limit`) | stroke | 1 sur 10 (le plus long : 64,4 ms) | 0 (< 2,5 périodes, soit < 41,7 ms) |

Tout le reste tient : 60 images/s sur les trois scénarios, p95 de l'intervalle 17,0 à 17,1 ms (seuil 18,5),
aucune image au-delà de 25 ms, aucune reprise hors limite, latence interne p50 10,0 ms et p95 17,3 ms (seuil
p50 < 20), pic mémoire 474 à 679 Mo processus + puce graphique (seuil 1 Go). Le cache de la page fait la
différence : sans lui, 20 à 43 % des images dépassent 25 ms.

**Hypothèse proposée à Jérôme (N1).** Un figement est, par définition, l'intervalle où un trait terminé passe de l'encre au cache de la
page (décision 10) ; 1 fois sur 10 il coûte plus de 2,5 images. La part du redessin de la couche (décision 8)
dans ce coût n'est pas mesurée isolément (non vérifié).
Deux options : (a) accepter un figement ponctuel par trait terminé jusqu'à 4 images (≈ 67 ms) et le compter
à part dans le verdict ; (b) garder le seuil et traiter le redessin du cache (redessin partiel de la zone du
trait, ou ajout du trait au cache sans redessin complet) dans un chantier dédié. Avant de trancher : rejouer
sur le Dell 3510, où la marge sera plus faible (`docs/banc.md`).

## Résumé du banc de fluidité

| Rendu | Cache | Scénario | Images/s | p95 intervalle (ms) | > 25 ms | Figements / reprises > limite | Latence p50 / p95 (ms) | Pic mémoire (Mo) | Verdict | Valide |
|---|---|---|---|---|---|---|---|---|---|---|
| direct3d11 | oui | stroke | 60.0 | 17.1 | 0.00 % | 1 / 0 | 10.0 / 17.3 | 474 | dépassé | oui |
| direct3d11 | oui | zoom | 60.0 | 17.0 | 0.00 % | 0 / 0 | — | 473 | dépassé | oui |
| direct3d11 | oui | scroll | 60.0 | 17.0 | 0.00 % | 0 / 0 | — | 679 | dépassé | oui |
| software | oui | stroke | 160.2 | 9.6 | 0.00 % | 10 / 0 | 7.2 / 9.6 | 89 | dépassé (informatif) | oui |
| software | oui | zoom | 103.3 | 15.3 | 0.00 % | 0 / 0 | — | 89 | dépassé (informatif) | oui |
| software | oui | scroll | 112.1 | 14.3 | 0.16 % | 0 / 0 | — | 179 | dépassé (informatif) | oui |
| direct3d11 | non | stroke | 59.0 | 33.9 | 20.63 % | 1 / 4 | 18.9 / 40.3 | 470 | dépassé | oui |
| direct3d11 | non | zoom | 56.4 | 34.3 | 40.78 % | 0 / 0 | — | 467 | dépassé | oui |
| direct3d11 | non | scroll | 56.5 | 35.1 | 43.03 % | 0 / 0 | — | 467 | dépassé | oui |
| software | non | stroke | 66.6 | 24.0 | 3.46 % | 0 / 0 | 21.0 / 38.5 | 82 | dépassé (informatif) | oui |
| software | non | zoom | 11.8 | 232.8 | 100.00 % | 0 / 0 | — | 83 | dépassé (informatif) | oui |
| software | non | scroll | 13.2 | 84.6 | 100.00 % | 0 / 0 | — | 83 | dépassé (informatif) | oui |

Fichiers :

- `2026-10-14_011903-Dell-Inc-Latitude-5450-direct3d11.json` : direct3d11, cache oui, Dell Inc. Latitude 5450, Intel(R) Core(TM) Ultra 5 125U, puce Intel(R) Graphics, écran 1920 × 1080 à 60 Hz, fenêtre fullscreen, boucle threaded, construction RelWithDebInfo, commit 283df8a7bac1, critères dépassés : stroke freeze_over_limit = 1.00 (seuil 0.00)
- `2026-10-14_011940-Dell-Inc-Latitude-5450-software.json` : software, cache oui, Dell Inc. Latitude 5450, Intel(R) Core(TM) Ultra 5 125U, puce software, écran 1920 × 1080 à 60 Hz, fenêtre fullscreen, boucle basic, construction RelWithDebInfo, commit 283df8a7bac1, critères dépassés : stroke freeze_over_limit = 10.00 (seuil 0.00)
- `2026-10-14_012111-Dell-Inc-Latitude-5450-direct3d11.json` : direct3d11, cache non, Dell Inc. Latitude 5450, Intel(R) Core(TM) Ultra 5 125U, puce Intel(R) Graphics, écran 1920 × 1080 à 60 Hz, fenêtre fullscreen, boucle threaded, construction RelWithDebInfo, commit 283df8a7bac1, critères dépassés : stroke p95_ms = 33.92 (seuil 18.50) ; stroke over_25ms_ratio = 0.21 (seuil 0.01) ; stroke freeze_over_limit = 1.00 (seuil 0.00) ; stroke resume_over_limit = 4.00 (seuil 0.00) ; zoom fps = 56.38 (seuil 58.00) ; zoom p95_ms = 34.30 (seuil 18.50) ; zoom over_25ms_ratio = 0.41 (seuil 0.01) ; scroll fps = 56.48 (seuil 58.00) ; scroll p95_ms = 35.10 (seuil 18.50) ; scroll over_25ms_ratio = 0.43 (seuil 0.01)
- `2026-10-14_012146-Dell-Inc-Latitude-5450-software.json` : software, cache non, Dell Inc. Latitude 5450, Intel(R) Core(TM) Ultra 5 125U, puce software, écran 1920 × 1080 à 60 Hz, fenêtre fullscreen, boucle basic, construction RelWithDebInfo, commit 283df8a7bac1, critères dépassés : stroke p95_ms = 23.99 (seuil 18.50) ; stroke over_25ms_ratio = 0.03 (seuil 0.01) ; stroke latency_p50_ms = 20.99 (seuil 20.00) ; zoom fps = 11.77 (seuil 58.00) ; zoom p95_ms = 232.85 (seuil 18.50) ; zoom over_25ms_ratio = 1.00 (seuil 0.01) ; scroll fps = 13.16 (seuil 58.00) ; scroll p95_ms = 84.56 (seuil 18.50) ; scroll over_25ms_ratio = 1.00 (seuil 0.01)
