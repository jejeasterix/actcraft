# Passe Windows du lot 7 — suites, banc S22 et S23, captures D3D11

Jouée la nuit du 2026-11-07 sur le Dell Latitude 5450 (écran principal 1920 × 1080 à 60 Hz, Windows 11), branche
`feat/livraison-complete-b--pc-lot7-preuves`, tête 33f2ce14. Écran allumé, session ouverte ; banc et captures joués
copie OneDrive suspendue, processeur à 0 %. Liste de la passe : « Fin du lot 7 côté Mac » de
`docs/sessions/livraison-complete-b-etat.md`.

Préalable des ressources : la branche n'a pas `ResourceLibrary`, le correctif L8-C14 ne s'y applique pas. Le témoin
`Documents\ActCraft\Ressources\temoin-o33.txt` est présent après les suites, le banc et les captures.

## Suites

- `test.ps1 -Preset release` (`CTEST_PARALLEL_LEVEL=4`) : 219 verts, 3 sautés (code 77, attendus : `tst_formats_oracle`,
  `tst_formats_oracle_synth`, `tst_flipchart_miniatures_samples`), 1 rouge sur 223. `test.ps1 -Preset debug` : 2 rouges.
  Journaux : `.tmp/o33-test-release.log`, `.tmp/o33-test-debug.log`.
- **`tst_ui_camera_ratio2`** (`QT_SCALE_FACTOR=2`) : construit et vert en release et en debug.
- `tst_captures_imprimer` : rouge en release et en debug, rejoué seul rouge : `LU appPrint.gray=false ECART (attendu
  true)`. Le clic `{"pointer": [[566, 154]], "space": "window"}` de `bench/scenarios/imprimer.json` dépend de la police.
  Hors écran (ctest), la police est plus étroite : le bouton « En gris » est à x = 486, le clic tombe sur le fond de la
  boîte (pixel #f4f4f4 de `imprimer-02-boite-impression.png`). À l'écran (`captures.ps1`, 1280 × 800), le bouton est
  vers x = 541 et le clic touche son libellé : le scénario passe. Aucune coordonnée fixe ne vise le bouton dans les
  deux rendus : à B de viser l'élément (ou de lire sa position). Vu aussi sur la capture hors écran : libellés de la
  boîte presque invisibles (#eaecee sur #f4f4f4), à confirmer à l'écran.
- `tst_bench_result` : rouge en debug (dans la suite et rejoué seul par ctest), vert en release. Sortie QtTest :
  `.tmp/o33-bench-result-debug.txt`, `pageEffectChoicesAreNotJudged` : `intervalle 31 (choix 1) classé A`
  (`tests/tst_bench_result.cpp:353`). En debug, sur ce PC lent, un intervalle qui suit un choix d'effet est classé
  actif ; la mesure debug n'est pas valide pour le banc, mais le test l'exige. Cause non prouvée.

## Banc D3D11 plein écran (05 h 09)

`bench.ps1 -Rendu d3d11 -SansConstruire`, passage valide :
`bench/results/2026-11-07_050911-Dell-Inc-Latitude-5450-direct3d11.json` (journal `.tmp/o33-banc-d3d11.log`). Verdict
brut `BUDGET DEPASSE`, comme à chaque banc D3D11 (décision 38). Critères de la référence tenus : `stroke`, `zoom`,
`scroll` à 60 i/s, p95 16,9 à 17,0 ms, 0 % > 25 ms, latence p50 10,0 ms, aucun figement ni reprise.

- **S22 `effets-page` : non tenu.** 60 i/s, p95 17,1 ms, mais **1,65 % > 25 ms**, p99 36,2 ms, **max 83,5 ms** ;
  16 reconstructions du cache (une par changement de page). Mac logiciel : 2,4 à 2,5 %. « Aucune image au-delà de la
  limite » n'est pas vrai sur le PC.
- **S23 `message-defilant` : images tenues, latence non tenue.** 60 i/s, p95 17,1 ms, 0 % > 25 ms, max 17,7 ms ; mais
  **latence du stylet p50 35,3 ms, p95 50,5 ms** (seuil 20 ms ; 10 ms sans message). Le délai de réception des
  entrées monte aussi (p95 16,7 ms contre 1,9 ms) : le défilement retarde le traitement des points du stylet.
- `reconnaissance` : 60 i/s, p95 17,0 ms, 0 %.
- Hors critères, comme au lot 6 : `page` 40,7 i/s, p95 42,5 ms ; `gommer` p95 26,2 ms ; `soulever` 2,48 % > 25 ms ;
  `forme-page-annotee` 36,1 ms ; `onglets` 51,1 i/s ; `miniatures-pauses` 1,69 % ; pic mémoire 1 127,5 Mio
  (1 182 277 632 octets, pendant `effets-page` ; seuil 1 024 Mio).

## Captures D3D11 à l'écran

`captures.ps1 -Scenario tous -Rendu d3d11` : **29/33** (`.tmp/o33-captures-d3d11.log`).
- `importer` : `ATTENTE appTabs.count=2 ECHEC`. Sur B, `ACTCRAFT_OFFICE_CONVERTER` n'est posé que par
  `tests/bench.cmake:543` (vert en ctest) ; `captures.ps1` le pose sur A depuis 34ab29c2. Disparaît à la fusion.
- `exporter`, `edition-export` : `FICHIER …/page.png ECHEC (image illisible)`, aucun fichier écrit, en D3D11, en
  logiciel et hors écran. Cause : `captures.ps1` passe `-Sortie` résolu avec des « \ » (`Resolve-Path`), et
  `ui/src/fileexchange.cpp:25` (`url.toString()`) lit `C:` comme un schéma d'une lettre et rend « %5C ». Le même
  scénario lancé directement avec un dossier en « / » passe à l'écran en D3D11 (`page.png` 1024 × 768,
  `sortie.pdf` 2 pages ; preuve copiée dans `.tmp/o33-export-slash/`). Corrigé sur A par 34ab29c2 (`localized()`).
- `page-allongee-zones` : `LU pageView.cacheRefreshCount=8 ECART (attendu 7)` dans la série, vert rejoué seul
  (instabilité connue, L7-T40).

## Contrôles à l'écran (plateforme Windows, hors offscreen)

`tst_ui_lot7 desktop` et `tst_ui_lot7 camera` en release, sans `QT_QPA_PLATFORM` (sorties `.tmp/o33-ecran-desktop.txt`,
`.tmp/o33-ecran-camera.txt`) :
- **Garde M4** (`kHiddenWindowsSettleMs = 250`, `desktopcontroller.cpp:163` ; caméra `kSettleMs = 200`,
  `cameracontroller.cpp:36`) : `captureHidesOurWindows`, `desktopCaptureBecomesTheBackground`,
  `captureDoesNotBlockTheMainThread` verts sur vraie machine.
- Caméra : `desktopCaptureKeepsTheSelection` et `captureAtPixelRatioTwoKeepsTheLogicalSize` verts.
- Rouges à l'écran seulement (tests écrits pour l'offscreen) : `restoreFromTheWheelGivesBackFullScreen(sans
  annotation)` (`Windowed` au lieu de `FullScreen`), `windowCaptureFramesTheWindowUnderThePointer` et
  `windowCaptureKeepsTheClickedWindow` (sélection `0,0 1920x1080` au lieu de la fenêtre de test `50,60 300x200`).

## Mutant `desktopCaptureKeepsTheSelection`

Mutation de `tests/tst_ui_camera.cpp:725-727` (« tout masquage arrête la sélection ») posée dans
`CameraController::eventFilter` (`QEvent::Close` ou `QEvent::Hide`), `tst_ui_lot7` release seul recompilé, hors
écran : **mutant tué, mais par `Caught unhandled exception`** (code 0xE06D7363), pas par un échec de comparaison
(`.tmp/o33-mutant-camera.txt`). Piste : le test garde un `QQuickWindow*` nu ; la sélection arrêtée le détruit, puis
`window->isVisible()` le lit. Un `QPointer` donnerait un échec propre. Code rétabli et recompilé, `tst_ui_camera*` verts.

## Non fait

Preuves de `keyinjector_win.cpp` (champ d'une autre application, trois frappes au stylet injecté : à jouer avec
Jérôme) ; seuil de 125 ms de la garde M4 (aucune constante de 125 ms trouvée : 250 ms au bureau, 200 ms à la
caméra) ; `NOMS OK` ; `graphify update .`. Durées et `P = 4` : relevés à l'exécution, non gardés dans un journal.