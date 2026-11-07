# Passe Windows du lot 7 — suites, banc S22 et S23, captures D3D11

Jouée la nuit du 2026-11-07 sur le Dell Latitude 5450 (écran principal 1920 × 1080 à 60 Hz, Windows 11), branche
`feat/livraison-complete-b--pc-lot7-preuves`, tête 33f2ce14. Écran allumé, session ouverte ; banc et captures joués
copie OneDrive suspendue, processeur à 0 %. Liste de la passe : « Fin du lot 7 côté Mac » de
`docs/sessions/livraison-complete-b-etat.md`.

Préalable des ressources : la branche n'a pas `ResourceLibrary`, le correctif L8-C14 ne s'y applique pas. Le témoin
`Documents\ActCraft\Ressources\temoin-o33.txt` est présent après les suites, le banc et les captures.

## Suites

- `test.ps1 -Preset release` (P = 4) : 222/223, 21,3 min. `test.ps1 -Preset debug` : 221/223, 41,1 min.
- **`tst_ui_camera_ratio2`** (`QT_SCALE_FACTOR=2`) : construit et vert en release et en debug.
- `tst_captures_imprimer`, rouge en release et en debug, et rejoué seul : `LU appPrint.gray=false ECART (attendu true)`.
  Le clic `{"pointer": [[566, 154]], "space": "window"}` de `bench/scenarios/imprimer.json` tombe sur le fond de la
  boîte (pixel #f4f4f4) : sous Windows, le bouton « En gris » est à x = 486 et son libellé s'arrête vers 535. Les
  coordonnées viennent du Mac. Le même scénario passe à l'écran en plein écran (`captures.ps1`). À corriger par B :
  viser le bouton, pas la fin du libellé. Vu aussi sur la capture : libellés de la boîte presque invisibles
  (#eaecee sur #f4f4f4), à confirmer à l'écran.
- `tst_bench_result` : rouge en debug seulement (2 fois sur 2), vert en release. `pageEffectChoicesAreNotJudged` :
  `intervalle 31 (choix 1) classé A` (`tests/tst_bench_result.cpp:353`). En debug, sur ce PC lent, un intervalle
  qui suit un choix d'effet est classé actif ; la mesure debug n'est pas valide pour le banc, mais le test l'exige.

## Banc D3D11 plein écran (05 h 09)

`bench.ps1 -Rendu d3d11 -SansConstruire`, passage valide :
`bench/results/2026-11-07_050911-Dell-Inc-Latitude-5450-direct3d11.json`. Verdict brut `BUDGET DEPASSE`, comme à chaque
banc D3D11 (décision 38). Critères de la référence tenus : `stroke`, `zoom`, `scroll` à 60 i/s, p95 16,9 à 17,0 ms,
0 % > 25 ms, latence p50 10,0 ms, aucun figement ni reprise.

- **S22 `effets-page` : non tenu.** 60 i/s, p95 17,1 ms, mais **1,65 % > 25 ms**, p99 36,2 ms, **max 83,5 ms** ;
  16 reconstructions du cache (une par changement de page). Mac logiciel : 2,4 à 2,5 %. « Aucune image au-delà de la
  limite » n'est pas vrai sur le PC.
- **S23 `message-defilant` : images tenues, latence non tenue.** 60 i/s, p95 17,1 ms, 0 % > 25 ms, max 17,7 ms ; mais
  **latence du stylet p50 35,3 ms, p95 50,5 ms** (seuil 20 ms ; 10 ms sans message). Le délai de réception des
  entrées monte aussi (p95 16,7 ms contre 1,9 ms) : le défilement retarde le traitement des points du stylet.
- `reconnaissance` : 60 i/s, p95 17,0 ms, 0 %.
- Hors critères, comme au lot 6 : `page` 40,7 i/s, p95 42,5 ms ; `gommer` p95 26,2 ms ; `forme-page-annotee` 36,1 ms ;
  `onglets` 51,1 i/s ; `miniatures-pauses` 1,69 % ; pic mémoire 1 182 Mio.

## Captures D3D11 à l'écran

`captures.ps1 -Scenario tous -Rendu d3d11` : **29/33**.
- `importer` : `ATTENTE appTabs.count=2 ECHEC` alors que `captures.ps1` pose le convertisseur factice ; vert en ctest.
  Même échec qu'au Mac hors ctest ; cause non cherchée.
- `exporter`, `edition-export` : `FICHIER …/page.png ECHEC (image illisible)`, aucun fichier écrit, en D3D11, en
  logiciel et hors écran. Cause : `captures.ps1` passe `-Sortie` avec des « \ ». `actcraft-captures` lancé avec le
  même dossier écrit en « / » passe à l'écran en D3D11 (`page.png 1024x768`, `sortie.pdf 2 pages`). Le lot 6, avec le
  `bench/captures/main.cpp` de A, passe. À vérifier après la fusion.
- `page-allongee-zones` : `LU pageView.cacheRefreshCount=8 ECART (attendu 7)` dans la série, vert rejoué seul
  (instabilité connue, L7-T40).

## Contrôles à l'écran (plateforme Windows, hors offscreen)

`tst_ui_lot7 desktop` et `tst_ui_lot7 camera` en release, sans `QT_QPA_PLATFORM` :
- **Garde M4** (`kHiddenWindowsSettleMs = 250`, `desktopcontroller.cpp:163`) : `captureHidesOurWindows`,
  `desktopCaptureBecomesTheBackground`, `captureDoesNotBlockTheMainThread` verts sur vraie machine.
- Caméra : `desktopCaptureKeepsTheSelection` et `captureAtPixelRatioTwoKeepsTheLogicalSize` verts (pas de plantage).
- Rouges à l'écran seulement (tests écrits pour l'offscreen) : `restoreFromTheWheelGivesBackFullScreen(sans
  annotation)` (`Windowed` au lieu de `FullScreen`), `windowCaptureFramesTheWindowUnderThePointer` et
  `windowCaptureKeepsTheClickedWindow` (sélection `0,0 1920x1080` au lieu de la fenêtre de test `50,60 300x200`).

## Non fait

Preuves de `keyinjector_win.cpp` (champ d'une autre application, frappes au stylet : à jouer avec Jérôme) ; seuil de
125 ms de la garde M4 (aucune constante de 125 ms trouvée : 250 ms au bureau, 200 ms à la caméra) ; mutant
`desktopCaptureKeepsTheSelection` (la mutation n'est pas décrite dans l'état) ; `NOMS OK` ; `graphify update .`.
