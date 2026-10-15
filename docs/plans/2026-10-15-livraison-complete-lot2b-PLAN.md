# Livraison complète — lot 2, sous-lot 2b « Rendu, document ouvert, fichiers, S4 et S5 » — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**But.** Version 0.2.0 (lot 2 du cadrage `docs/specs/2026-10-14-livraison-complete-cadrage.md`, décision 5) : ouvrir
un paperboard `.flipchart` et le voir comme dans ActivInspire (S4), enregistrer en `.actcraft`, fermer, rouvrir à
l'identique, y compris sous Android par `content://` (S5) ; Fichier ▸ Nouveau (cinq tailles), Ouvrir, Enregistrer,
Enregistrer sous, Fermer, Quitter avec demande d'enregistrement ; avertissements de lecture affichés sans bloquer.
Ce plan suit le sous-lot 2a (`docs/plans/2026-10-15-livraison-complete-lot2a-PLAN.md`), dont il utilise tous les
contrats (modèle, pile, format natif, lecteur, oracle) : **toute tâche de 2b suppose 2a terminé** (point d'étape 2a
passé) ; les dépendances de la table ne nomment une tâche de 2a que pour signaler un contrat précis. Il se termine par
la **clôture du lot 2** (une seule PR).

**Architecture.** `render/` : un peintre partagé (`paintPage`, DA5) pour les miniatures, la comparaison aux `.thm` et
le rendu logiciel des nouveaux types ; un moteur « peint » dans la scène (nœud QPainter en logiciel, texture
rastérisée en RHI, décision 37) ; images des ressources décodées sur fils secondaires ; fond et grille de page ;
**interface de cache figée** (`core::PageCache` : `insert`, `remove`, `invalidate`, DA11), implémentée par
`FixedPageItem`, qui **lit la page dans le document** au lieu d'en garder une copie. `ui/` : le `DocumentController`
du lot 1 est refait autour du document et de sa pile d'annulation (relais défait, revue L1-T9) ; la taille de page
suit le document ; ouverture et enregistrement hors du fil principal. QML : menu Fichier, dialogues, demande
d'enregistrement, bandeau d'avertissements. `app/` : le fichier passé au lancement s'ouvre. `bench/` : le joueur de
scénarios gagne fixtures, attentes, sonde de la boucle d'événements ; scénarios S4 et S5 ; scénario de banc
« changement de page ». Android : `content://` prouvé sur l'émulateur, ou commandes de fichier retirées de l'APK.

**Pile.** C++20, Qt 6.12 (Core, Gui, Quick, QuickControls2, QuickDialogs2, Test), CMake + Ninja, MSVC 2022 ; Qt 6.12
pour Android, émulateur `x86_64` ; PowerShell 7.

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L2-T11 | `render` : peintre partagé (`paintPage`, `paintObject`, fond, grille, miniature PNG) | L2-T1 | `render/include/actcraft/render/pagepainter.h`, `render/src/pagepainter.cpp`, `render/CMakeLists.txt`, `tests/render.cmake`, `tests/tst_render_painter.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 56 tests, `tst_render_painter` Passed ; banc hors écran (résultat) | S4 | #tache-l2-t11 |
| L2-T12 | `render` : images des ressources décodées sur fils secondaires, couleur transparente | L2-T1 | `render/include/actcraft/render/imagestore.h`, `render/src/imagestore.cpp`, `render/src/imagerenderer.cpp`, `tests/tst_render_images.cpp`, `THIRD_PARTY_NOTICES.md` | isolé | implementeur-tdd | TESTS OK, ≥ 55 tests, `tst_render_images` Passed (dont `resourceImagesAreDecodedOffThreadAndMasked`) ; banc hors écran (résultat) | S4, S5 | #tache-l2-t12 |
| L2-T13 | `render` : nouveaux types dans la scène (moteur peint), fond, grille, transformation et translucidité | L2-T11, L2-T12 | `render/include/actcraft/render/paintedrenderer.h`, `render/src/paintedrenderer.cpp`, `render/include/actcraft/render/objectrenderer.h`, `render/include/actcraft/render/fixedpageitem.h`, `render/src/fixedpageitem.cpp`, `render/CMakeLists.txt`, `tests/render.cmake`, `tests/tst_render_painted.cpp`, `tests/tst_render_fixedpage.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 57 tests, `tst_render_painted` (dont `sceneMatchesThePainter`) et `tst_render_fixedpage` Passed ; banc hors écran : `content_check.ok = True` | S4 | #tache-l2-t13 |
| L2-T14 | Interface de cache figée (DA11) et page fixe qui lit le document | L2-T2, L2-T13 | `core/include/actcraft/core/pagecache.h`, `core/src/pagecache.cpp`, `core/CMakeLists.txt`, `tests/core.cmake`, `tests/tst_core_pagecache.cpp`, `render/include/actcraft/render/fixedpageitem.h`, `render/src/fixedpageitem.cpp`, `tests/tst_render_fixedpage.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 58 tests, `tst_core_pagecache` et `tst_render_fixedpage` Passed ; banc hors écran : `content_check.ok = True` | S5 | #tache-l2-t14 |
| L2-T15 | `ui` : `DocumentController` sur le document et sa pile (relais du lot 1 défait) | L2-T14 | `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`, `ui/src/penadapters.h`, `ui/src/penadapters.cpp`, `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`, `ui/qml/Main.qml`, `ui/qml/PageArea.qml`, `tests/tst_ui_document.cpp`, `tests/tst_pageview.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 58 tests, `tst_ui_document` (dont `everyWriteGoesThroughTheUndoStack`), `tst_pageview`, `tst_ui_shell`, `tst_captures_premiers_pas` Passed ; banc hors écran (résultat) | S5 | #tache-l2-t15 |
| L2-T16 | `ui` : taille de page suivie, barres à jour, échelle de rastérisation | L2-T15 | `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`, `ui/qml/PageArea.qml`, `tests/tst_pageview.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 58 tests, `tst_pageview` Passed (dont `contentResizeRefitsAndEmitsScroll`) ; banc hors écran (résultat) | S4, S5 | #tache-l2-t16 |
| L2-T17 | `ui` : ouvrir, enregistrer, nouveau, fermer — hors du fil principal, miniatures | L2-T6, L2-T9, L2-T11, L2-T15 | `ui/include/actcraft/ui/documentio.h`, `ui/src/documentio.cpp`, `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`, `ui/CMakeLists.txt`, `tests/ui.cmake`, `tests/tst_ui_files.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 59 tests, `tst_ui_files` Passed (dont `saveThenOpenKeepsTheFingerprint`, `ioRunsOffTheMainThread`) ; banc hors écran (résultat) | S4, S5 | #tache-l2-t17 |
| L2-T18 | QML : menu Fichier, dialogues, « enregistrer les modifications ? », bandeau d'avertissements | L2-T3, L2-T16, L2-T17 | `ui/qml/Main.qml`, `ui/qml/FileActions.qml`, `ui/qml/SaveChangesDialog.qml`, `ui/qml/PageSizeDialog.qml`, `ui/qml/MessageBanner.qml`, `ui/CMakeLists.txt`, `actions/data/layout.json`, `tests/ui.cmake`, `tests/tst_ui_filemenu.cpp`, `tests/tst_ui_shell.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 60 tests, `tst_ui_filemenu` et `tst_ui_shell` Passed ; captures hors écran de la fenêtre (menu Fichier, bandeau) ; banc hors écran (résultat) | S4, S5 | #tache-l2-t18 |
| L2-T19 | Ouvrir le fichier passé au lancement (`actcraft.exe <fichier>`, `--ouvrir`) | L2-T18 | `app/startup.h`, `app/startup.cpp`, `app/main.cpp`, `tests/app.cmake`, `tests/tst_app_open.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 61 tests, `tst_app_open` Passed | S4 | #tache-l2-t19 |
| L2-T20 | Joueur de scénarios : fixtures, variables, attente, sonde de boucle ; scénario `aller-retour-natif` (S5) | L2-T18 | `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`, `bench/include/actcraft/bench/fixtures.h`, `bench/src/fixtures.cpp`, `bench/CMakeLists.txt`, `bench/scenarios/aller-retour-natif.json`, `tests/bench.cmake`, `tests/tst_bench_uiscenario.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 61 tests, `tst_bench_uiscenario` et `tst_captures_aller_retour_natif` Passed ; `captures.ps1 -Scenario aller-retour-natif -Rendu logiciel -HorsEcran` → `BOUCLE max=… ms OK`, `LU documentController.fingerprint=… OK`, `CAPTURES OK` | S5 | #tache-l2-t20 |
| L2-T21 | S4 : miniatures comparées aux `.thm`, scénarios `ouvrir-flipchart-synthetique` (CI) et `local/ouvrir-flipchart` | L2-T10, L2-T19, L2-T20 | `bench/flipchart/main.cpp`, `bench/CMakeLists.txt`, `bench/src/fixtures.cpp`, `bench/scenarios/ouvrir-flipchart-synthetique.json`, `bench/scenarios/local/ouvrir-flipchart.json`, `scripts/captures.ps1`, `tests/bench.cmake` | isolé | concepteur-critique | TESTS OK, ≥ 66 tests, `tst_flipchart_miniatures_synthetique` et `tst_captures_ouvrir_flipchart_synthetique` Passed ; localement `MINIATURES OK : 124 pages, médiane ≤ 2,0, ≤ 3 au-dessus de 7` et `captures.ps1 -Scenario local/ouvrir-flipchart -Ouvrir "<samples>\tri des formes.flipchart"` → `CAPTURES OK` | S4 | #tache-l2-t21 |
| L2-T22 | Android : Ouvrir et Enregistrer par `content://` sur l'émulateur, sinon commandes de fichier retirées de l'APK | L2-T18 | `scripts/essai-android-fichiers.ps1`, `actions/data/commands.json`, `docs/verifications/2026-10-15-android-fichiers.md` | isolé | concepteur-critique | `ANDROID FICHIERS OK` (empreinte enregistrée = empreinte relue, logcat) ; sinon repli : `APK OK` + capture du menu Fichier de l'APK sans « Ouvrir… » ni « Enregistrer » | S5 | #tache-l2-t22 |
| L2-T23 | Banc : scénario « changement de page » ; page fixe sans éclair blanc pendant le décodage | L2-T16, L2-T17 | `render/include/actcraft/render/fixedpageitem.h`, `render/src/fixedpageitem.cpp`, `tests/tst_render_fixedpage.cpp`, `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`, `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp`, `bench/src/runner.cpp`, `tests/tst_bench_scenarios.cpp`, `tests/tst_bench_result.cpp`, `tests/tst_bench_verdict.cpp`, `scripts/resume-banc.ps1`, `docs/banc.md` | isolé | concepteur-critique | TESTS OK, ≥ 59 tests, `tst_render_fixedpage` (dont `keepsTheOldContentUntilTheNewPageIsReady`), `tst_bench_scenarios`, `tst_bench_result` Passed ; banc hors écran : scénario `page` présent, `content_check.ok = True` | S4 | #tache-l2-t23 |

13 tâches. Les minima comptent les **55** tests de la fin de 2a, plus ceux des tâches dont la tâche dépend, plus les
siens ; « ≥ » est la règle. Tests ctest ajoutés : L2-T11 +1, L2-T13 +1, L2-T14 +1, L2-T17 +1, L2-T18 +1, L2-T19 +1,
L2-T20 +1, L2-T21 +4 (dont `tst_flipchart_miniatures_samples`, sauté — code 77 — sans `ACTCRAFT_SAMPLES_DIR`), soit
**66** à la fin du lot 2.

Fenêtres : aucune tâche ne joue de scénario à l'écran ni le banc en plein écran (clôture, jeton `ecran`). L2-T22
démarre un émulateur sans fenêtre (`-no-window`) et l'arrête par son PID.

**Conflit de fichier tenu par l'ordre** : L2-T23 touche `ui/src/documentcontroller.cpp` (une méthode de banc) et
`render/src/fixedpageitem.cpp` ; elle dépend de L2-T17 (vague 10) et ne partage sa vague qu'avec L2-T18, qui ne
touche aucun de ces fichiers.

## Écarts au cadrage (repris à la porte de fin du lot 2, avec ceux de 2a)

1. **Sonde de la boucle d'événements au lieu de la durée d'image** (S5, L2-T20). « Aucune image > 25 ms pendant
   l'enregistrement » se prouve par le plus long intervalle entre deux tours de la boucle du fil principal pendant
   l'enregistrement (minuterie de 5 ms) : une image ne peut pas durer moins que le blocage du fil qui la prépare, et la
   mesure tient hors écran (les images n'y sont produites qu'à la demande).
2. **Document de S5 construit par le joueur** (L2-T20). Le document de 3 pages (trait, forme, texte, image) est écrit
   par l'étape `fixture` d'`actcraft-captures` avant d'être ouvert par `file.open` : aucun binaire commité. Le trait
   est ajouté au stylet injecté, la forme, le texte et l'image viennent de la fixture (les outils sont aux lots 3 et 4).
3. **S4 en CI sur un paperboard synthétique** (L2-T21). Les `samples/` ne quittent pas le poste : la CI ouvre le
   paperboard de `fctest::sampleFlipchart`, dont les miniatures sont rendues par notre peintre (preuve du chemin, pas
   de la fidélité) ; la fidélité (médiane ≤ 2,0, au plus 3 pages au-dessus de 7) est prouvée localement. Mesure
   écrite par nous : écart moyen absolu par canal, images ramenées à 80 × 60 par lissage.
4. **Ligne de commande** (L2-T19) : `actcraft.exe <fichier>` (Ouvrir avec) et `--ouvrir <fichier>` (outils, captures).
5. **Pas d'onglets avant le lot 5** (L2-T18) : « Nouveau » et « Ouvrir » remplacent le document affiché, après la
   demande d'enregistrement s'il est modifié ; « Fermer » laisse un paperboard vierge « Sans titre » (F-656).
6. **`.flipchart` ouvert = document sans fichier** (L2-T17) : « Enregistrer » ouvre « Enregistrer sous » avec le nom
   proposé `<nom>.actcraft` (décision 18 : un `.flipchart` ne se réécrit pas).
7. **Miniatures du format natif** : PNG de 320 px de large (la hauteur suit la page), comme les `.thm`.
8. **Titre de fenêtre** : `[*]<titre> — ActCraft — rendu : <moteur>` (le moteur reste dans le titre, décision 19 du
   prototype).

## Couverture des scénarios

| Scénario | Tâches | Joué à la clôture |
|---|---|---|
| S4 — ouvrir un `.flipchart` | L2-T11 (peintre), L2-T12 (images), L2-T13 (scène), L2-T16 (taille de page), L2-T17 (ouverture), L2-T18 (avertissements), L2-T19 (ligne de commande), L2-T21 (miniatures, scénarios), L2-T23 (fluidité au changement de page) ; 2a : L2-T7 à L2-T10 | oui : oracle local (17 paperboards, 124 pages, 0 écart), `MINIATURES OK` local, `captures.ps1 -Scenario local/ouvrir-flipchart -Rendu d3d11` à l'écran, `actcraft.exe "<samples>\tri des formes.flipchart"` |
| S5 — aller-retour natif | L2-T14, L2-T15 (document, pile, cache), L2-T17 (fichiers), L2-T18 (commandes), L2-T20 (scénario), L2-T22 (Android) ; 2a : L2-T1 à L2-T6 | oui : `captures.ps1 -Scenario aller-retour-natif -Rendu d3d11` à l'écran ; Android : `ANDROID FICHIERS OK` (ou repli constaté) |

## Vagues proposées (parallélisme 2 au plus)

V6 : L2-T11 ‖ L2-T12 · V7 : L2-T13 · V8 : L2-T14 · V9 : L2-T15 · V10 : L2-T16 ‖ L2-T17 · V11 : L2-T18 ‖ L2-T23 ·
V12 : L2-T19 ‖ L2-T20 · V13 : L2-T21 ‖ L2-T22.

Deux tâches d'une même vague ne partagent aucun fichier (vérifié sur la colonne `fichiers touchés`). Fichiers
partagés tenus par les dépendances : `render/src/fixedpageitem.cpp` et `tests/tst_render_fixedpage.cpp` (L2-T13 →
L2-T14 → L2-T23) ; `render/CMakeLists.txt` et `tests/render.cmake` (L2-T11 → L2-T13) ; `ui/src/pageview.cpp`,
`tests/tst_pageview.cpp` et `ui/qml/PageArea.qml` (L2-T15 → L2-T16) ; `ui/src/documentcontroller.cpp` (L2-T15 →
L2-T17 → L2-T23) ; `ui/CMakeLists.txt` et `tests/ui.cmake` (L2-T17 → L2-T18) ; `ui/qml/Main.qml` (L2-T15 → L2-T18) ;
`bench/CMakeLists.txt`, `bench/src/fixtures.cpp` et `tests/bench.cmake` (L2-T20 → L2-T21) ;
`actions/data/commands.json` (L2-T3 en 2a → L2-T22).

## Carte des fichiers

| Fichier | Responsabilité | Tâche |
|---|---|---|
| `render/include/actcraft/render/pagepainter.h` | `ImageLookup`, `DecodedImages`, `decodeImages`, `PaintOptions`, `paintObjectLocal`, `paintObject`, `paintBackground`, `paintGrid`, `paintPage`, `renderPage`, `thumbnailPng` | L2-T11 |
| `render/include/actcraft/render/imagestore.h` | images procédurales et ressources décodées, variantes à couleur transparente | L2-T12 |
| `render/include/actcraft/render/paintedrenderer.h` | `PaintedRenderer`, `usesPaintedRenderer`, `rasterizeLocal`, nœuds de fond et de grille | L2-T13 |
| `render/include/actcraft/render/fixedpageitem.h` | page affichée = page du document (`setSource`), `core::PageCache`, échelle de rastérisation, contenu gardé jusqu'au prêt | L2-T13, L2-T14, L2-T23 |
| `core/include/actcraft/core/pagecache.h` | `PageCache`, `applyToCache` (DA11) | L2-T14 |
| `ui/include/actcraft/ui/documentcontroller.h` | document, pile, page affichée, état modifié, titre, fichier, empreinte ; ouverture et enregistrement | L2-T15, L2-T17, L2-T23 |
| `ui/include/actcraft/ui/documentio.h` | `loadDocument`, `saveDocument` (fil secondaire), `titleFromUrl`, `defaultDocumentsFolder` | L2-T17 |
| `ui/qml/FileActions.qml`, `SaveChangesDialog.qml`, `PageSizeDialog.qml`, `MessageBanner.qml` | commandes de fichier, dialogues, bandeau | L2-T18 |
| `app/startup.*`, `app/main.cpp` | fichier ouvert au lancement | L2-T19 |
| `bench/include/actcraft/bench/fixtures.h` | documents de scénario écrits avant d'être ouverts | L2-T20, L2-T21 |
| `bench/scenarios/aller-retour-natif.json`, `ouvrir-flipchart-synthetique.json`, `local/ouvrir-flipchart.json` | scénarios S5 et S4 | L2-T20, L2-T21 |
| `scripts/essai-android-fichiers.ps1` | S5 sous Android (`content://`) | L2-T22 |
| `bench/src/scenarios.cpp`, `bench/src/runner.cpp` | scénario de banc `page` | L2-T23 |

## Contrats partagés entre tâches (noms figés)

Tous les noms de 2a, du lot 1 et du prototype restent figés ; on ajoute, on ne renomme pas.

- `actcraft::render` (L2-T11) : `kThumbnailWidth` (320), `ImageLookup`, `DecodedImages` (`insert`, `find`, `size`,
  `lookup`), `decodeImages`, `PaintOptions` (`background`, `grid`, `thumbnail`), `paintObjectLocal`, `paintObject`,
  `paintBackground`, `paintGrid`, `paintPage`, `renderPage`, `thumbnailPng`.
- `actcraft::render` (L2-T12) : `ImageStore::generate(const core::Page&, const core::Document* = nullptr)`,
  `ImageStore::resourceImage`, `ImageStore::objectImage`, `ImageStore::variantKey`.
- `actcraft::render` (L2-T13) : `PaintedRenderer` (`createNode`), `usesPaintedRenderer`, `rasterizeLocal`,
  `createBackgroundNode`, `createGridNode`, `RenderContext::rasterScale`, `FixedPageItem::setRasterScale`,
  `FixedPageItem::rasterScale`.
- `actcraft::core` (L2-T14) : `PageCache` (`insert`, `remove`, `invalidate`), `applyToCache`.
- `actcraft::render` (L2-T14) : `FixedPageItem` (dérive aussi de `core::PageCache`) : `setSource`, `sourceDocument`,
  `sourcePage`, `rebuildCount` (crochet de test), signal `contentChanged` ; `setPage`, `page`, `appendStroke`,
  `loadGeneratedPage`, `isReady`, `objectCount` gardés.
- `actcraft::ui` (L2-T15) : `DocumentController` — propriétés `pageSize`, `modified`, `title`, `fileUrl`,
  `fingerprint`, `canUndo`, `canRedo` ; méthodes `document() const`, `execute`, `undo`, `redo`, `commitStroke`,
  `replaceDocument`, `newDocument`, `loadGeneratedPages` (L2-T23) ; signaux `modifiedChanged`, `titleChanged`,
  `fileUrlChanged`, `documentChanged`, `undoChanged` (et ceux du lot 1). `PageView::documentController` ;
  `ControllerSink`. Titre d'un document neuf : « Sans titre ».
- `actcraft::ui` (L2-T17) : `LoadOutcome`, `loadDocument`, `SaveOutcome`, `saveDocument`, `titleFromUrl`,
  `defaultDocumentsFolder` ; `DocumentController` : `openUrl`, `saveToUrl`, `busy`, `warnings`, `lastError`,
  `suggestedFileName`, `urlFromArgument`, `defaultFolderUrl`, signaux `loadFinished(bool, QString)`,
  `saveFinished(bool, QString)`. Journal (catégorie `actcraft.document`) : `ACTCRAFT OUVERT <empreinte>`,
  `ACTCRAFT ENREGISTRE <empreinte>` (empreinte en hexadécimal, 16 chiffres).
- QML (L2-T18) : `objectName` `fileActions`, `openDialog`, `saveDialog`, `saveChangesDialog`, `pageSizeDialog`,
  `messageBanner`, `messageText`, `menu-file` (lot 1) ; ordre du menu Fichier dans `layout.json`.
- `app` (L2-T19) : `StartupOptions::openUrl`, `openFileArgument`, option `--ouvrir <fichier>`.
- `actcraft::bench` (L2-T20) : `UiStepKind::Fixture`, `WaitFor`, `WatchEventLoop`, `MaxEventLoopGap`, `Remember`,
  `ArchiveContains` (ajoutés à la fin de l'énumération) ; lignes `FIXTURE`, `BOUCLE`, `MEMO`, `ARCHIVE` ; variables
  `${SORTIE}`, `${SAMPLES}`, `${<nom>}` ; `buildFixture` (`fixtures.h`) ; fixtures `trois-pages` (L2-T20),
  `flipchart-synthetique` (L2-T21).
- Outils (L2-T21) : `actcraft-flipchart-dump --miniatures <fichier>` → `MINIATURES OK : <n> pages, médiane <m>,
  <k> au-dessus de 7` / `MINIATURES EN ECHEC : <raison>` ; `--ecrire-synthetique <fichier>` → `SYNTHETIQUE OK :
  <fichier>` ; `--samples <dossier>` (tous les `.flipchart` du dossier, code 77 si le dossier est vide ou absent) ;
  `captures.ps1 -Ouvrir <fichier>`.
- Script (L2-T22) : `essai-android-fichiers.ps1 -Apk <apk>` → `ANDROID FICHIERS OK` / `ANDROID FICHIERS EN ECHEC :
  <raison>`.
- Banc (L2-T23) : scénario `page`, `InputKind::Command`, `PlannedInput::command`, `pageScenario`.

## Règles communes à toutes les tâches (rappelées dans chaque section)

Celles de 2a (`docs/plans/2026-10-15-livraison-complete-lot2a-PLAN.md`, « Règles communes ») s'appliquent toutes,
mot pour mot : processus enfant pour toute commande PowerShell (`pwsh -NoProfile -Command "Set-Location
'<worktree>'; …"`), `git -C`, commande ciblée et preuve bornée, banc sans écran avec `-Taille 1280x800`, mutation
remise par `git checkout --`, `format.ps1`, pièges de Clang, pas de `std::visit` direct sur `PageObject`, **aucune API
privée de Qt dans le code livré**, propriété intellectuelle et `samples/` en lecture seule, commits explicites, arrêt
par PID. S'y ajoutent, pour 2b :

- **Preuve visuelle** (tâches d'interface) : `scripts/captures.ps1` hors écran en rendu logiciel, lectures `-Lire`
  d'abord, puis description de la capture (forme bornée du profil, `Select-Object -Last 16`).
- **Polices hors écran** : tout test ou outil qui dessine du texte hors écran sous Windows a `QT_QPA_FONTDIR`
  (`actcraft_add_test` le pose ; un `add_test` direct prend `${_bench_env_offscreen}` de `tests/bench.cmake`). Les
  contrôles de pixels sur du texte restent tolérants (polices différentes selon le système).
- **Contrastes testés sur les couleurs rendues** (leçon L1-C5) : un élément d'interface neuf qui pose du texte sur
  un fond a un test qui lit les couleurs **rendues** (capture) et vérifie le rapport WCAG AA (4,5:1), pas seulement
  les jetons.
- **Touches injectées** : passer par l'étape `key` du joueur, qui livre les touches par `handleShortcutEvent` avant la
  livraison (macOS, piège L1-C4) ; ne jamais poster un `QKeyEvent` soi-même dans un scénario.
- **Démons et émulateurs** : un démon Gradle ou un émulateur lancé par une tâche est arrêté par son PID avant de
  rendre la main (`Stop-Process -Id`) ; le démon Gradle verrouille sinon le worktree (piège L1-T17).
- **CI** : toute action GitHub ajoutée ou changée est épinglée par son SHA (leçon L1-T19) ; aucun push hors des
  branches `essai-ci/…` (aucune tâche de 2b n'en a besoin).
- **QML qui appelle `Commands.trigger`** : si MSVC lève C4702 sur le code généré, ajouter le fichier à la liste
  `QT_QML_SKIP_CACHEGEN` de `ui/CMakeLists.txt` (comme `TopBar.qml`), jamais désactiver l'avertissement.

---

## Tâches

<a id="tache-l2-t11"></a>
### Tâche L2-T11 — `render` : peintre partagé (`paintPage`, `paintObject`, fond, grille, miniature PNG)

**Objectif.** DA5, décision 24 : **un seul** chemin `QPainter` pour dessiner une page, réutilisé par les miniatures
du format natif (L2-T17), la comparaison aux `.thm` (L2-T21), le rendu des nouveaux types dans la scène (L2-T13) et,
au lot 5, l'export d'image et de PDF. Règles de dessin reprises de la référence (`REA - Activinspire/tools/
render_flipchart.py` : `make_pen`, `make_brush`, `draw_object`, `render_page` ; `app/actcraft/canvas/grid.py` :
`Grid.paint`, `GridLevel.lines`, `GridLevel.points`, `_paint_level`, `_clip`), écrites en C++ par nous :

- objet invisible ou groupe : rien ; translucidité : opacité × (1 − t / 255) ; transformation objet → page ;
- trait du stylo d'ActCraft (`StrokeKind::Pen`, bouts carrés) : `paintStrokeSegments` (même géométrie que le moteur
  du prototype) ; autre trait (surligneur, encre révélatrice, bouts ronds ou plats) : un chemin avec une plume
  d'épaisseur `baseWidth`, jointures rondes ; un seul point : un point ;
- ligne et connecteur : plume du modèle ; forme `Path` : sous-chemins fermés sauf `open`, règle « winding » si
  demandée, remplissage puis contour ; formes du prototype : comme `ShapePainterNode` ;
- image : ressource (par `ImageLookup`) ou image procédurale (`synthesizeImage`), couleur transparente rendue
  transparente ; image absente : rectangle magenta pâle (repère visuel, comme la référence) ;
- texte : runs s'il y en a, police recalée sur la largeur mesurée par l'original quand l'écart dépasse 6 % (4
  caractères au moins, rapport entre 0,5 et 2) ; sinon HTML par `QTextDocument`, largeur = celle du rectangle ;
- média : image d'espace réservé, sinon cadre tireté et « média » ;
- fond : couleur, dégradé (vertical, horizontal, deux diagonales, radial), motif, image par-dessus ;
- grille : niveaux visibles, traits de 1 px écran (cosmétiques) non lissés à l'écran, lissés à 0,7 px en miniature,
  pointillés 1/4, tirets 3/8, points, croix et gros pointillés aux intersections, styles 45° et angle ;
- image de page : fond blanc, 96 ppp (1 pt = 4/3 px comme sous Windows), lissage ; miniature PNG de 320 px de large.

**Fichiers.**
- Créer : `render/include/actcraft/render/pagepainter.h`, `render/src/pagepainter.cpp`,
  `tests/tst_render_painter.cpp`
- Modifier : `render/CMakeLists.txt`, `tests/render.cmake`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_render_painter` → `error`. Vert : forme bornée,
N ≥ 56, `TESTS OK`, `tst_render_painter .*Passed`. Banc hors écran (module `render/`) : `BUDGET TENU` ou
`BUDGET DEPASSE`, `content_check.ok = True`.

**Interdits.** Le peintre ne garde aucun état et ne touche à aucun `QObject` : il doit pouvoir tourner sur un fil
secondaire. Ne pas modifier les moteurs du prototype (`shaperenderer`, `strokerenderer`, `imagerenderer`). Aucune
image de Promethean ; les contrôles se font sur des pixels calculés.

**Terminé quand.** Preuve verte, JSON du banc joint, commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests.** Créer `tests/tst_render_painter.cpp` :

```cpp
#include <actcraft/core/document.h>
#include <actcraft/render/pagepainter.h>

#include <QBuffer>
#include <QImageReader>
#include <QPainter>
#include <QTest>
#include <QThread>
#include <QtConcurrent/QtConcurrentRun>

using namespace actcraft;

namespace {

bool near(QColor actual, QColor expected, int tolerance)
{
    return qAbs(actual.red() - expected.red()) <= tolerance && qAbs(actual.green() - expected.green()) <= tolerance &&
           qAbs(actual.blue() - expected.blue()) <= tolerance;
}

core::PageObject square(double x, double y, double size, core::Argb fill)
{
    core::Shape s;
    s.kind = core::ShapeKind::Path;
    s.paths = {{{0, 0}, {size, 0}, {size, size}, {0, size}}};
    s.pen.style = core::PenStyle::None;
    s.brush = core::solidFill(fill);
    core::PageObject o = s;
    o.common.transform = core::translation(x, y);
    return o;
}

const render::ImageLookup kNoImages = [](const std::string&) { return QImage(); };

} // namespace

// Peintre partagé (DA5) : règles de dessin contrôlées au pixel sur des pages construites ici.
class TstRenderPainter : public QObject
{
    Q_OBJECT

private slots:
    void blankPageIsWhiteAtTheGivenScale()
    {
        const QImage image = render::renderPage(core::Page{}, kNoImages, 0.5);
        QCOMPARE(image.size(), QSize(512, 384));
        QVERIFY(near(image.pixelColor(10, 10), Qt::white, 0));
    }

    void pathShapeFollowsItsTransform()
    {
        core::Page page;
        page.objects.push_back(square(100, 50, 40, 0xFFFF0000u));
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        QVERIFY(near(image.pixelColor(120, 70), Qt::red, 2));
        QVERIFY(near(image.pixelColor(90, 70), Qt::white, 2));
        QVERIFY(near(image.pixelColor(150, 70), Qt::white, 2));
    }

    void rotationIsClockwise()
    {
        core::Page page;
        core::PageObject o = square(0, 0, 40, 0xFF0000FFu);
        o.common.transform = core::compose(core::rotation(90), core::translation(200, 200));
        page.objects.push_back(o);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        // (10, 10) → rotation de 90° → (−10, 10) → (190, 210)
        QVERIFY(near(image.pixelColor(190, 210), Qt::blue, 2));
        QVERIFY(near(image.pixelColor(210, 210), Qt::white, 2));
    }

    void translucencyAndVisibility()
    {
        core::Page page;
        core::PageObject half = square(0, 0, 40, 0xFF000000u);
        half.common.translucency = 128;
        page.objects.push_back(half);
        core::PageObject hidden = square(100, 0, 40, 0xFF000000u);
        hidden.common.visible = false;
        page.objects.push_back(hidden);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        const int grey = image.pixelColor(20, 20).red();
        QVERIFY2(grey > 110 && grey < 145, qPrintable(QString::number(grey)));
        QVERIFY(near(image.pixelColor(120, 20), Qt::white, 0));
    }

    void highlighterStrokeHasRoundCapsAndAlpha()
    {
        core::Stroke s;
        s.kind = core::StrokeKind::Highlighter;
        s.cap = core::PenCap::Round;
        s.baseWidth = 20;
        s.color = 0x80FFFF00u;
        s.points = {{100, 100, 0.5f}, {200, 100, 0.5f}};
        core::Page page;
        page.objects.push_back(s);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        QVERIFY(near(image.pixelColor(150, 100), QColor(255, 255, 127), 6)); // jaune à moitié transparent sur blanc
        QVERIFY(!near(image.pixelColor(93, 100), Qt::white, 10));            // bout rond au-delà du premier point
    }

    void penStrokeKeepsSquareSegments()
    {
        core::Stroke s;
        s.baseWidth = 10;
        s.points = {{100, 100, 0.5f}, {200, 100, 0.5f}};
        core::Page page;
        page.objects.push_back(s);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        QVERIFY(near(image.pixelColor(97, 102), Qt::black, 30)); // bout carré : coin rempli
    }

    void imageResourceWithTransparentColor()
    {
        QImage source(4, 4, QImage::Format_ARGB32);
        source.fill(Qt::white);
        source.setPixelColor(0, 0, Qt::green);
        render::DecodedImages images;
        images.insert("cle", source);
        core::ImageObject im;
        im.bounds = {0, 0, 4, 4};
        im.resource = "cle";
        im.hasTransparentColor = true;
        im.transparentColor = 0xFFFFFFFFu;
        core::PageObject o = im;
        o.common.transform = core::scaling(25, 25); // 100 × 100 px
        core::Page page;
        page.background.fill = core::solidFill(0xFF0000FFu);
        page.objects.push_back(o);
        const QImage image = render::renderPage(page, images.lookup(), 1.0);
        QVERIFY(near(image.pixelColor(10, 10), Qt::green, 30));
        QVERIFY(near(image.pixelColor(80, 80), Qt::blue, 30)); // le blanc de l'image laisse voir le fond
    }

    void missingImageIsMarked()
    {
        core::ImageObject im;
        im.bounds = {0, 0, 50, 50};
        im.resource = "absente";
        core::Page page;
        page.objects.push_back(im);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        QVERIFY(!near(image.pixelColor(25, 25), Qt::white, 5));
    }

    void htmlTextIsDrawnInItsBounds()
    {
        core::Text t;
        t.bounds = {0, 0, 300, 60};
        t.html = "<p style=\"color:#ff0000; font-size:40px\">XXXX</p>";
        core::PageObject o = t;
        o.common.transform = core::translation(100, 100);
        core::Page page;
        page.objects.push_back(o);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        int red = 0;
        for (int y = 100; y < 160; ++y) {
            for (int x = 100; x < 400; ++x) {
                const QColor c = image.pixelColor(x, y);
                red += c.red() > 200 && c.green() < 80 ? 1 : 0;
            }
        }
        QVERIFY2(red > 200, qPrintable(QString::number(red)));
        QVERIFY(near(image.pixelColor(50, 50), Qt::white, 0));
    }

    void runsAreDrawnAtTheirBaseline()
    {
        core::Text t;
        t.bounds = {0, 0, 200, 40};
        core::TextRun run;
        run.text = "XXXX";
        run.fontFamily = "Arial";
        run.pointSize = 24;
        run.color = 0xFF0000FFu;
        run.baseline = {0, 30};
        run.bounds = {0, 0, 0, 0}; // pas de recalage
        t.runs.push_back(run);
        core::PageObject o = t;
        o.common.transform = core::translation(100, 100);
        core::Page page;
        page.objects.push_back(o);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        int blue = 0;
        for (int y = 100; y < 131; ++y) {
            for (int x = 100; x < 220; ++x) {
                blue += image.pixelColor(x, y).blue() > 200 && image.pixelColor(x, y).red() < 80 ? 1 : 0;
            }
        }
        QVERIFY2(blue > 100, qPrintable(QString::number(blue)));
        QVERIFY(near(image.pixelColor(150, 140), Qt::white, 0)); // rien sous la ligne de base
    }

    void backgroundGradientAndPattern()
    {
        core::Page page;
        page.background.fill.kind = core::FillKind::Gradient;
        page.background.fill.color = 0xFF000000u;
        page.background.fill.color2 = 0xFFFFFFFFu;
        page.background.fill.gradient = core::GradientKind::Vertical;
        const QImage image = render::renderPage(page, kNoImages, 0.25);
        QVERIFY(image.pixelColor(100, 2).red() < 20);
        QVERIFY(image.pixelColor(100, 189).red() > 235);
        page.background.fill.gradient = core::GradientKind::Horizontal;
        const QImage horizontal = render::renderPage(page, kNoImages, 0.25);
        QVERIFY(horizontal.pixelColor(2, 100).red() < 20);
        QVERIFY(horizontal.pixelColor(253, 100).red() > 235);
    }

    void backgroundImageCoversThePage()
    {
        QImage source(2, 2, QImage::Format_ARGB32);
        source.fill(Qt::red);
        render::DecodedImages images;
        images.insert("fond", source);
        core::Page page;
        page.background.image = "fond";
        const QImage image = render::renderPage(page, images.lookup(), 0.25);
        QVERIFY(near(image.pixelColor(5, 5), Qt::red, 2));
        QVERIFY(near(image.pixelColor(250, 186), Qt::red, 2));
    }

    void gridLinesFollowTheStep()
    {
        core::Page page;
        page.grid.visible = true;
        core::GridLevel level;
        level.stepX = 100;
        level.stepY = 100;
        level.color = 0xFF000000u;
        page.grid.levels.push_back(level);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        // Trait de 1 px non lissé en x = floor(100 + 0,2) = 100.
        QVERIFY(near(image.pixelColor(100, 50), Qt::black, 10));
        QVERIFY(near(image.pixelColor(150, 50), Qt::white, 0));
        QVERIFY(near(image.pixelColor(50, 100), Qt::black, 10));
        page.grid.visible = false;
        QVERIFY(near(render::renderPage(page, kNoImages, 1.0).pixelColor(100, 50), Qt::white, 0));
    }

    void gridMarksAtIntersections()
    {
        core::Page page;
        page.grid.visible = true;
        core::GridLevel level;
        level.stepX = 100;
        level.stepY = 100;
        level.line = core::GridLine::Cross;
        page.grid.levels.push_back(level);
        const QImage image = render::renderPage(page, kNoImages, 1.0);
        QVERIFY(near(image.pixelColor(100, 100), Qt::black, 10));
        QVERIFY(near(image.pixelColor(102, 100), Qt::black, 10));
        QVERIFY(near(image.pixelColor(150, 100), Qt::white, 0));
    }

    void thumbnailIsA320PixelWidePng()
    {
        core::Page page;
        page.objects.push_back(square(0, 0, 512, 0xFFFF0000u));
        const QByteArray png = render::thumbnailPng(page, kNoImages);
        QVERIFY(png.startsWith("\x89PNG"));
        QImage image;
        QVERIFY(image.loadFromData(png, "PNG"));
        QCOMPARE(image.size(), QSize(320, 240));
        QVERIFY(near(image.pixelColor(40, 40), Qt::red, 4));
    }

    void decodeImagesReadsEveryResource()
    {
        core::Document document;
        QImage source(3, 3, QImage::Format_ARGB32);
        source.fill(Qt::blue);
        QByteArray png;
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        source.save(&buffer, "PNG");
        const std::string key = document.addResource(png, "ird"); // PNG renommé : la signature fait foi
        document.addResource(QByteArray("ID3 son"), "mp3");
        const render::DecodedImages images = render::decodeImages(document);
        QCOMPARE(images.size(), std::size_t{1});
        QCOMPARE(images.find(key).size(), QSize(3, 3));
        QVERIFY(images.find("absente").isNull());
    }

    void paintsOffTheMainThread()
    {
        core::Page page;
        page.objects.push_back(square(0, 0, 100, 0xFF00FF00u));
        QThread* worker = nullptr;
        const QImage image = QtConcurrent::run([&page, &worker] {
                                 worker = QThread::currentThread();
                                 return render::renderPage(page, kNoImages, 0.5);
                             }).result();
        QVERIFY(worker != QThread::currentThread());
        QVERIFY(near(image.pixelColor(10, 10), Qt::green, 2));
    }
};

QTEST_MAIN(TstRenderPainter)
#include "tst_render_painter.moc"
```

  `QtConcurrent` est un module de Qt (LGPL, présent dans l'installation) : il n'est lié **qu'au test**. Si
  `Qt6::Concurrent` n'est pas trouvé par `find_package`, remplacer par un `QThread::create(...)` + `wait()` et le
  noter au rapport. Ajouter à `tests/render.cmake` :

```cmake
find_package(Qt6 6.12 REQUIRED COMPONENTS Concurrent)
actcraft_add_test(tst_render_painter
    SOURCES tst_render_painter.cpp
    LIBS ActCraft::render Qt6::Gui Qt6::Concurrent)
```

- [ ] **Étape 2 : constater le rouge.** Commande ciblée, motif `tst_render_painter` → `error`.

- [ ] **Étape 3 : écrire l'en-tête.** Créer `render/include/actcraft/render/pagepainter.h` :

```cpp
#pragma once

#include <actcraft/core/document.h>

#include <QByteArray>
#include <QImage>

#include <functional>
#include <string>
#include <unordered_map>

class QPainter;

namespace actcraft::render {

// Peintre partagé (DA5, décision 24) : rendu logiciel des nouveaux types, miniatures, comparaison aux .thm, puis
// exports (lot 5). Sans état, sans QObject : utilisable sur un fil secondaire. Règles reprises de la référence
// (REA - Activinspire/tools/render_flipchart.py, app/actcraft/canvas/grid.py), écrites par nous.

inline constexpr int kThumbnailWidth = 320;

// Image décodée d'une ressource du document par sa clé ; image nulle si absente.
using ImageLookup = std::function<QImage(const std::string& resourceKey)>;

class DecodedImages
{
public:
    void insert(std::string key, QImage image);
    QImage find(const std::string& key) const;
    std::size_t size() const noexcept;
    // Recherche dans ces images ; `this` doit vivre aussi longtemps que la fonction rendue.
    ImageLookup lookup() const;

private:
    std::unordered_map<std::string, QImage> m_images;
};

// Décode toutes les ressources que QImage sait lire (la signature fait foi, pas l'extension) ; à appeler sur un fil
// secondaire pour un gros document.
DecodedImages decodeImages(const core::Document& document);

struct PaintOptions
{
    bool background = true;
    bool grid = true;
    bool thumbnail = false; // grille lissée à 0,7 px, échelle de miniature
};

// Objet en coordonnées de l'objet, sans sa transformation ni sa translucidité (le moteur peint de la scène les pose
// par ses nœuds) ; rien pour un groupe.
void paintObjectLocal(QPainter& painter, const core::PageObject& object, const ImageLookup& images);
// Objet en coordonnées de page : visibilité, translucidité, transformation, puis paintObjectLocal.
void paintObject(QPainter& painter, const core::PageObject& object, const ImageLookup& images);
// Fond de page (couleur, dégradé, motif, image) sur (0, 0, largeur, hauteur).
void paintBackground(QPainter& painter, const core::Page& page, const ImageLookup& images);
// Grille visible de la page (F-621) ; traits de 1 px écran.
void paintGrid(QPainter& painter, const core::Page& page, bool thumbnail);
// Page entière en coordonnées de page : fond, grille dessous (ou dessus si onTop), objets dans l'ordre du vecteur.
void paintPage(QPainter& painter, const core::Page& page, const ImageLookup& images, const PaintOptions& options = {});
// Image de la page à l'échelle `scale` : fond blanc, 96 ppp, lissage.
QImage renderPage(const core::Page& page, const ImageLookup& images, double scale, const PaintOptions& options = {});
// Miniature PNG de kThumbnailWidth pixels de large (hauteur selon la page), comme les .thm d'origine.
QByteArray thumbnailPng(const core::Page& page, const ImageLookup& images);

} // namespace actcraft::render
```

- [ ] **Étape 4 : écrire le peintre.** Créer `render/src/pagepainter.cpp` :

```cpp
#include <actcraft/render/imagesynth.h>
#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/render/painternode.h>
#include <actcraft/render/strokegeometry.h>

#include <QBuffer>
#include <QFontMetricsF>
#include <QLinearGradient>
#include <QLineF>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QRadialGradient>
#include <QTextDocument>

#include <algorithm>
#include <cmath>

namespace actcraft::render {

namespace {

constexpr int kMaxGridLines = 4000; // garde-fou : pas minuscule sur une grande page (grid.py)

QColor color(core::Argb argb)
{
    return QColor::fromRgba(argb);
}

QRectF rectOf(const core::RectF& r)
{
    return QRectF(r.x, r.y, r.width, r.height);
}

QPen penOf(const core::Outline& o)
{
    if (o.style == core::PenStyle::None || o.width <= 0) {
        return QPen(Qt::NoPen);
    }
    QPen pen(color(o.color));
    pen.setWidthF(o.width);
    pen.setStyle(static_cast<Qt::PenStyle>(static_cast<int>(o.style)));
    pen.setCapStyle(o.cap == core::PenCap::Flat     ? Qt::FlatCap
                    : o.cap == core::PenCap::Square ? Qt::SquareCap
                                                    : Qt::RoundCap);
    pen.setJoinStyle(o.join == core::PenJoin::Miter   ? Qt::MiterJoin
                     : o.join == core::PenJoin::Bevel ? Qt::BevelJoin
                                                      : Qt::RoundJoin);
    return pen;
}

QBrush brushOf(const core::Fill& f, const QRectF& rect)
{
    switch (f.kind) {
    case core::FillKind::None:
        return QBrush(Qt::NoBrush);
    case core::FillKind::Solid:
        return QBrush(color(f.color));
    case core::FillKind::Pattern:
        return QBrush(color(f.color), static_cast<Qt::BrushStyle>(std::clamp<int>(f.pattern, 2, 14)));
    case core::FillKind::Gradient:
        break;
    }
    QGradientStops stops{{0.0, color(f.color)}, {1.0, color(f.color2)}};
    switch (f.gradient) {
    case core::GradientKind::Radial: {
        QRadialGradient g(rect.center(), std::max(rect.width(), rect.height()) / 2);
        g.setStops(stops);
        return QBrush(g);
    }
    case core::GradientKind::Horizontal: {
        QLinearGradient g(rect.topLeft(), rect.topRight());
        g.setStops(stops);
        return QBrush(g);
    }
    case core::GradientKind::DiagonalDown: {
        QLinearGradient g(rect.topLeft(), rect.bottomRight());
        g.setStops(stops);
        return QBrush(g);
    }
    case core::GradientKind::DiagonalUp: {
        QLinearGradient g(rect.topRight(), rect.bottomLeft());
        g.setStops(stops);
        return QBrush(g);
    }
    case core::GradientKind::Vertical:
        break;
    }
    QLinearGradient g(rect.topLeft(), rect.bottomLeft());
    g.setStops(stops);
    return QBrush(g);
}

// Pixels de la couleur `transparent` (alpha ignoré) rendus transparents.
QImage withTransparentColor(const QImage& source, core::Argb transparent)
{
    QImage image = source.convertToFormat(QImage::Format_ARGB32);
    const QRgb key = transparent & 0x00FFFFFFu;
    for (int y = 0; y < image.height(); ++y) {
        auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            if ((line[x] & 0x00FFFFFFu) == key) {
                line[x] = 0;
            }
        }
    }
    return image;
}

QPainterPath polyline(const std::vector<core::PointF>& points)
{
    QPainterPath path;
    if (points.empty()) {
        return path;
    }
    path.moveTo(points.front().x, points.front().y);
    for (std::size_t i = 1; i < points.size(); ++i) {
        path.lineTo(points[i].x, points[i].y);
    }
    return path;
}

void drawPolyline(QPainter& p, const std::vector<core::PointF>& points)
{
    if (points.size() == 1 || (points.size() == 2 && points[0].x == points[1].x && points[0].y == points[1].y)) {
        p.drawPoint(QPointF(points[0].x, points[0].y));
    } else if (!points.empty()) {
        p.drawPath(polyline(points));
    }
}

void paintStroke(QPainter& p, const core::Stroke& s)
{
    if (s.kind == core::StrokeKind::Pen && s.cap == core::PenCap::Square) {
        paintStrokeSegments(p, s.points, s.baseWidth, color(s.color));
        return;
    }
    QPen pen(color(s.color));
    pen.setWidthF(s.baseWidth);
    pen.setCapStyle(s.cap == core::PenCap::Flat     ? Qt::FlatCap
                    : s.cap == core::PenCap::Square ? Qt::SquareCap
                                                    : Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    std::vector<core::PointF> points;
    points.reserve(s.points.size());
    for (const core::StrokePoint& q : s.points) {
        points.push_back(core::PointF{q.x, q.y});
    }
    drawPolyline(p, points);
}

void paintShape(QPainter& p, const core::Shape& s)
{
    if (s.kind == core::ShapeKind::Path) {
        QPainterPath path;
        path.setFillRule(s.windingFill ? Qt::WindingFill : Qt::OddEvenFill);
        for (const auto& poly : s.paths) {
            if (poly.empty()) {
                continue;
            }
            path.moveTo(poly.front().x, poly.front().y);
            for (std::size_t i = 1; i < poly.size(); ++i) {
                path.lineTo(poly[i].x, poly[i].y);
            }
            if (!s.open) {
                path.closeSubpath();
            }
        }
        if (!s.open) {
            p.fillPath(path, brushOf(s.brush, path.boundingRect()));
        }
        p.strokePath(path, penOf(s.pen));
        return;
    }
    // Formes du prototype : comme ShapePainterNode (shaperenderer.cpp).
    std::vector<core::PointF> outline;
    switch (s.kind) {
    case core::ShapeKind::Rectangle:
        outline = rectanglePolygon(s.bounds);
        break;
    case core::ShapeKind::Ellipse:
        outline = ellipsePolygon(s.bounds, 48);
        break;
    default:
        outline = s.polygon;
        break;
    }
    QPolygonF polygon;
    for (const core::PointF& q : outline) {
        polygon << QPointF(q.x, q.y);
    }
    p.setOpacity(p.opacity() * s.opacity);
    p.setBrush(color(s.fill));
    if (s.outlineWidth > 0) {
        QPen pen(color(s.outline));
        pen.setWidthF(s.outlineWidth);
        pen.setJoinStyle(Qt::MiterJoin);
        p.setPen(pen);
    } else {
        p.setPen(Qt::NoPen);
    }
    p.drawPolygon(polygon);
}

void paintImage(QPainter& p, const core::ImageObject& im, const ImageLookup& images)
{
    const QRectF rect = rectOf(im.bounds);
    QImage image = im.resource.empty() ? synthesizeImage(im) : (images ? images(im.resource) : QImage());
    if (image.isNull()) {
        p.fillRect(rect, QColor(255, 0, 255, 60)); // repère d'image illisible, comme la référence
        return;
    }
    if (im.hasTransparentColor) {
        image = withTransparentColor(image, im.transparentColor);
    }
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.drawImage(rect, image);
}

void paintText(QPainter& p, const core::Text& t)
{
    if (!t.runs.empty()) {
        for (const core::TextRun& run : t.runs) {
            const QString text = QString::fromStdString(run.text);
            QFont font(QString::fromStdString(run.fontFamily));
            font.setPointSizeF(std::max(1.0, run.pointSize));
            font.setWeight(static_cast<QFont::Weight>(std::clamp(run.weight, 100, 900)));
            font.setItalic(run.italic);
            font.setUnderline(run.underline);
            font.setStrikeOut(run.strikeOut);
            p.setFont(font);
            // Les métriques de certaines polices diffèrent entre l'original et Qt 6 : on recale la taille sur la
            // largeur mesurée par l'original (rectangle du run), comme la référence.
            const double target = run.bounds.width;
            const double advance = QFontMetricsF(font, p.device()).horizontalAdvance(text.trimmed());
            if (target > 0 && advance > 0 && text.trimmed().size() >= 4) {
                const double ratio = target / advance;
                if (std::abs(ratio - 1.0) > 0.06 && ratio > 0.5 && ratio < 2.0) {
                    font.setPointSizeF(std::max(1.0, run.pointSize * ratio));
                    p.setFont(font);
                }
            }
            p.setPen(color(run.color));
            p.drawText(QPointF(run.baseline.x, run.baseline.y), text);
        }
        return;
    }
    if (t.html.empty()) {
        return;
    }
    QTextDocument document;
    document.setDocumentMargin(0);
    document.setHtml(QString::fromStdString(t.html));
    if (t.bounds.width > 0) {
        document.setTextWidth(t.bounds.width);
    }
    p.translate(t.bounds.x, t.bounds.y);
    document.drawContents(&p, QRectF(0, 0, t.bounds.width > 0 ? t.bounds.width : 2000,
                                     t.bounds.height > 0 ? t.bounds.height : 2000));
}

void paintMedia(QPainter& p, const core::Media& m, const ImageLookup& images)
{
    const QRectF rect = rectOf(m.bounds);
    const QImage placeholder = m.placeholder.empty() || !images ? QImage() : images(m.placeholder);
    if (!placeholder.isNull()) {
        p.drawImage(rect, placeholder);
        return;
    }
    p.setPen(QPen(QColor(80, 80, 80), 2, Qt::DashLine));
    p.setBrush(QColor(0, 0, 0, 30));
    p.drawRect(rect);
    p.drawText(rect, Qt::AlignCenter, QStringLiteral("média"));
}

// --- Grille (grid.py) -----------------------------------------------------------------------------------------

struct Range
{
    long long first = 0;
    long long last = -1;
};

Range rangeOf(double start, double step, double lo, double hi)
{
    Range r;
    r.first = static_cast<long long>(std::ceil((lo - start) / step - 1e-9));
    r.last = std::min(static_cast<long long>(std::floor((hi - start) / step + 1e-9)), r.first + kMaxGridLines);
    return r;
}

double cotangent(const core::GridLevel& l)
{
    const double degrees = l.pattern == core::GridPattern::Diagonal45 ? 45.0 : std::clamp(l.angle, 1.0, 89.0);
    return 1.0 / std::tan(degrees * 3.14159265358979323846 / 180.0);
}

// Liang-Barsky : segment coupé au rectangle ; faux s'il en sort entièrement.
bool clip(QLineF& line, const QRectF& r)
{
    const double x0 = line.x1();
    const double y0 = line.y1();
    const double dx = line.dx();
    const double dy = line.dy();
    double t0 = 0;
    double t1 = 1;
    const double ps[4] = {-dx, dx, -dy, dy};
    const double qs[4] = {x0 - r.left(), r.right() - x0, y0 - r.top(), r.bottom() - y0};
    for (int i = 0; i < 4; ++i) {
        if (ps[i] == 0) {
            if (qs[i] < 0) {
                return false;
            }
            continue;
        }
        const double t = qs[i] / ps[i];
        if (ps[i] < 0) {
            t0 = std::max(t0, t);
        } else {
            t1 = std::min(t1, t);
        }
        if (t0 > t1) {
            return false;
        }
    }
    line = QLineF(x0 + t0 * dx, y0 + t0 * dy, x0 + t1 * dx, y0 + t1 * dy);
    return true;
}

std::vector<QLineF> gridLines(const core::GridLevel& l, double w, double h, double scale)
{
    const double sx = std::max(l.stepX * scale, 1.0);
    const double sy = std::max(l.stepY * scale, 1.0);
    std::vector<QLineF> out;
    if (l.pattern == core::GridPattern::Normal) {
        const double x0 = std::max(0.0, l.start.x);
        const double y0 = std::max(0.0, l.start.y);
        if (l.drawX) {
            const Range r = rangeOf(l.start.x, sx, x0, w);
            for (long long k = r.first; k <= r.last; ++k) {
                const double x = l.start.x + static_cast<double>(k) * sx;
                out.emplace_back(x, y0, x, h);
            }
        }
        if (l.drawY) {
            const Range r = rangeOf(l.start.y, sy, y0, h);
            for (long long k = r.first; k <= r.last; ++k) {
                const double y = l.start.y + static_cast<double>(k) * sy;
                out.emplace_back(x0, y, w, y);
            }
        }
        return out;
    }
    const double cot = cotangent(l);
    const QRectF page(0, 0, w, h);
    const struct
    {
        double sign;
        bool on;
        double step;
    } families[2] = {{1, l.drawX, sx}, {-1, l.drawY, sy}};
    for (const auto& f : families) {
        if (!f.on) {
            continue;
        }
        const double reach = h * cot + f.step;
        const Range r = rangeOf(l.start.x, f.step, -reach, w + reach);
        for (long long k = r.first; k <= r.last; ++k) {
            const double x0 = l.start.x + static_cast<double>(k) * f.step;
            QLineF seg(x0 + f.sign * (0 - l.start.y) * cot, 0, x0 + f.sign * (h - l.start.y) * cot, h);
            if (clip(seg, page) && seg.length() > 1e-6) {
                out.push_back(seg);
            }
        }
    }
    return out;
}

std::vector<QPointF> gridPoints(const core::GridLevel& l, double w, double h, double scale)
{
    const double sx = std::max(l.stepX * scale, 1.0);
    const double sy = std::max(l.stepY * scale, 1.0);
    std::vector<QPointF> out;
    if (l.pattern == core::GridPattern::Normal) {
        if (!l.drawX || !l.drawY) {
            return out;
        }
        const Range rx = rangeOf(l.start.x, sx, std::max(0.0, l.start.x), w);
        const Range ry = rangeOf(l.start.y, sy, std::max(0.0, l.start.y), h);
        for (long long i = rx.first; i <= rx.last; ++i) {
            for (long long j = ry.first; j <= ry.last; ++j) {
                out.emplace_back(l.start.x + static_cast<double>(i) * sx, l.start.y + static_cast<double>(j) * sy);
                if (out.size() > static_cast<std::size_t>(kMaxGridLines) * 8) {
                    return out;
                }
            }
        }
        return out;
    }
    const double cot = cotangent(l);
    const double reach = h * cot + std::max(sx, sy);
    const Range ri = rangeOf(l.start.x, sx, -reach, w + reach);
    for (long long i = ri.first; i <= ri.last; ++i) {
        const double a = l.start.x + static_cast<double>(i) * sx;
        const Range rj = rangeOf(l.start.x, sy, a + 2 * (0 - l.start.y) * cot, a + 2 * (h - l.start.y) * cot);
        for (long long j = rj.first; j <= rj.last; ++j) {
            const double b = l.start.x + static_cast<double>(j) * sy;
            const double t = (b - a) / 2;
            const QPointF q(a + t, l.start.y + t / cot);
            if (q.x() >= 0 && q.x() <= w && q.y() >= 0 && q.y() <= h) {
                out.push_back(q);
                if (out.size() > static_cast<std::size_t>(kMaxGridLines) * 8) {
                    return out;
                }
            }
        }
    }
    return out;
}

void paintGridLevel(QPainter& p, const core::GridLevel& l, double w, double h, double scale, double width)
{
    const QColor c = color(l.color | 0xFF000000u);
    if (l.line == core::GridLine::Solid || l.line == core::GridLine::Dotted || l.line == core::GridLine::Dashed) {
        QPen pen(c, width); // cosmétique : 1 px écran (0,7 px lissé en miniature)
        pen.setCosmetic(true);
        pen.setCapStyle(Qt::FlatCap);
        if (l.line == core::GridLine::Dotted) {
            pen.setDashPattern({1, 4});
        } else if (l.line == core::GridLine::Dashed) {
            pen.setDashPattern({3, 8});
        }
        p.setPen(pen);
        for (const QLineF& seg : gridLines(l, w, h, scale)) {
            p.drawLine(seg);
        }
        return;
    }
    // Marques aux intersections, en pixels écran.
    const QTransform t = p.worldTransform();
    std::vector<QPointF> marks;
    for (const QPointF& q : gridPoints(l, w, h, scale)) {
        marks.push_back(t.map(q));
    }
    p.save();
    p.resetTransform();
    for (const QPointF& q : marks) {
        const int x = static_cast<int>(std::floor(q.x()));
        const int y = static_cast<int>(std::lround(q.y()));
        if (l.line == core::GridLine::Points) {
            p.fillRect(static_cast<int>(std::ceil(q.x() - 0.2)), y, 1, 1, c);
        } else if (l.line == core::GridLine::Cross) {
            p.fillRect(x - 2, y, 5, 1, c);
            p.fillRect(x, y - 2, 1, 5, c);
        } else {
            p.fillRect(x - 1, y - 1, 3, 3, c);
        }
    }
    p.restore();
}

} // namespace

void DecodedImages::insert(std::string key, QImage image)
{
    m_images[std::move(key)] = std::move(image);
}

QImage DecodedImages::find(const std::string& key) const
{
    const auto it = m_images.find(key);
    return it == m_images.end() ? QImage() : it->second;
}

std::size_t DecodedImages::size() const noexcept
{
    return m_images.size();
}

ImageLookup DecodedImages::lookup() const
{
    return [this](const std::string& key) { return find(key); };
}

DecodedImages decodeImages(const core::Document& document)
{
    DecodedImages out;
    for (const auto& [key, resource] : document.resources()) {
        QImage image = QImage::fromData(resource.data);
        if (!image.isNull()) {
            out.insert(key, std::move(image));
        }
    }
    return out;
}

void paintObjectLocal(QPainter& p, const core::PageObject& object, const ImageLookup& images)
{
    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    if (const auto* s = std::get_if<core::Stroke>(&object)) {
        paintStroke(p, *s);
    } else if (const auto* l = std::get_if<core::Line>(&object)) {
        p.setPen(penOf(l->pen));
        p.setBrush(Qt::NoBrush);
        drawPolyline(p, l->points);
    } else if (const auto* sh = std::get_if<core::Shape>(&object)) {
        paintShape(p, *sh);
    } else if (const auto* im = std::get_if<core::ImageObject>(&object)) {
        paintImage(p, *im, images);
    } else if (const auto* t = std::get_if<core::Text>(&object)) {
        paintText(p, *t);
    } else if (const auto* m = std::get_if<core::Media>(&object)) {
        paintMedia(p, *m, images);
    } else if (const auto* c = std::get_if<core::Connector>(&object)) {
        p.setPen(penOf(c->pen));
        p.drawLine(QPointF(c->endA.x, c->endA.y), QPointF(c->endB.x, c->endB.y));
    }
    p.restore();
}

void paintObject(QPainter& p, const core::PageObject& object, const ImageLookup& images)
{
    if (!object.common.visible || std::get_if<core::Group>(&object)) {
        return;
    }
    p.save();
    const core::Transform& t = object.common.transform;
    p.setTransform(QTransform(t.m11, t.m12, t.m21, t.m22, t.dx, t.dy), true);
    if (object.common.translucency != 0) {
        p.setOpacity(p.opacity() * (1.0 - object.common.translucency / 255.0));
    }
    paintObjectLocal(p, object, images);
    p.restore();
}

void paintBackground(QPainter& p, const core::Page& page, const ImageLookup& images)
{
    const QRectF rect(0, 0, page.width, page.height);
    if (page.background.fill.kind != core::FillKind::None) {
        p.fillRect(rect, brushOf(page.background.fill, rect));
    }
    if (!page.background.image.empty() && images) {
        const QImage image = images(page.background.image);
        if (!image.isNull()) {
            p.save();
            p.setRenderHint(QPainter::SmoothPixmapTransform, true);
            p.drawImage(rect, image);
            p.restore();
        }
    }
}

void paintGrid(QPainter& p, const core::Page& page, bool thumbnail)
{
    if (!page.grid.visible) {
        return;
    }
    const double scale = thumbnail ? page.grid.thumbnailScale : page.grid.defaultScale;
    p.save();
    p.setRenderHint(QPainter::Antialiasing, thumbnail);
    p.setClipRect(QRectF(0, 0, page.width, page.height), Qt::IntersectClip);
    if (!thumbnail) {
        // Les traits nets de l'original tombent sur floor(x + 0,2) (relevé de grid.py).
        p.setWorldTransform(p.worldTransform() * QTransform::fromTranslate(0.2, 0));
    }
    for (const core::GridLevel& level : page.grid.levels) {
        if (level.visible) {
            paintGridLevel(p, level, page.width, page.height, scale, thumbnail ? 0.7 : 0);
        }
    }
    p.restore();
}

void paintPage(QPainter& p, const core::Page& page, const ImageLookup& images, const PaintOptions& options)
{
    if (options.background) {
        paintBackground(p, page, images);
    }
    if (options.grid && !page.grid.onTop) {
        paintGrid(p, page, options.thumbnail);
    }
    for (const core::PageObject& object : page.objects) {
        paintObject(p, object, images);
    }
    if (options.grid && page.grid.onTop) {
        paintGrid(p, page, options.thumbnail);
    }
}

QImage renderPage(const core::Page& page, const ImageLookup& images, double scale, const PaintOptions& options)
{
    const int w = std::max(1, static_cast<int>(std::lround(page.width * scale)));
    const int h = std::max(1, static_cast<int>(std::lround(page.height * scale)));
    QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
    image.setDotsPerMeterX(3780); // 96 ppp : 1 pt = 4/3 px, comme sous Windows
    image.setDotsPerMeterY(3780);
    image.fill(Qt::white);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.scale(scale, scale);
    paintPage(p, page, images, options);
    p.end();
    return image;
}

QByteArray thumbnailPng(const core::Page& page, const ImageLookup& images)
{
    PaintOptions options;
    options.thumbnail = true;
    const double scale = page.width > 0 ? kThumbnailWidth / page.width : 1.0;
    const QImage image = renderPage(page, images, scale, options);
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return png;
}

} // namespace actcraft::render
```

  Points d'attention : `rectanglePolygon`, `ellipsePolygon` et `paintStrokeSegments` existent déjà
  (`strokegeometry.h`, `painternode.h`) ; `QGradientStops` se construit par paires `{position, couleur}` ;
  `pen.setDashPattern({1, 4})` prend des `qreal` ; la boucle de `withTransparentColor` lit `scanLine` en `QRgb`
  (format ARGB32 garanti par la conversion). Dans le test `runsAreDrawnAtTheirBaseline`, la police « Arial » peut
  manquer sous macOS (CI) : Qt prend une police de repli, le comptage de pixels reste valable.

  Ajouter à `render/CMakeLists.txt`, dans `SOURCES` après `src/inkitem.cpp` :

```cmake
        include/actcraft/render/pagepainter.h
        src/pagepainter.cpp
```

- [ ] **Étape 5 : vert ciblé.** Commande ciblée, motif `tst_render_painter` : `100% tests passed`. Un contrôle de
  pixel faux à 1 px près sur la grille : relire `grid.py` (décalage de 0,2 px) avant de toucher au test.

- [ ] **Étape 6 : preuve et banc.** `format.ps1`, preuve de tâche (N ≥ 56 ; `tst_render_painter` Passed), banc
  sans écran (`content_check.ok = True` : le peintre n'est pas encore dans la scène, la page du banc ne change pas).

- [ ] **Étape 7 : commit.**

```powershell
git -C <worktree> add render/include/actcraft/render/pagepainter.h render/src/pagepainter.cpp render/CMakeLists.txt tests/render.cmake tests/tst_render_painter.cpp
git -C <worktree> commit -m "render : peintre partagé (page, objets, fond, grille, miniature PNG) pour la scène, les miniatures et les exports" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t12"></a>
### Tâche L2-T12 — `render` : images des ressources décodées sur fils secondaires, couleur transparente

**Objectif.** Décision 5 (« image décodée sur fil secondaire ») et règle de fluidité 1 : le magasin d'images de la
page fixe (`ImageStore`) décode, en plus des images procédurales du banc, les **ressources** du document qu'une page
utilise — images, images d'espace réservé des médias, image de fond — sur ses fils, une fois par clé. Une image à
couleur transparente a sa **variante** précalculée (clé = ressource + `#aarrggbb`), pour que la scène n'ait aucun
calcul de pixels à faire pendant la synchronisation. `ImageRenderer` dessine ces images. Les avis des composants
tiers de Qt Image Formats (TIFF, WebP… déployés avec l'installateur) entrent dans `THIRD_PARTY_NOTICES.md`.

**Fichiers.**
- Modifier : `render/include/actcraft/render/imagestore.h`, `render/src/imagestore.cpp`,
  `render/src/imagerenderer.cpp`, `tests/tst_render_images.cpp`, `THIRD_PARTY_NOTICES.md`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_render_images` → `error` (`resourceImage`
inconnu). Vert : forme bornée, N ≥ 55, `TESTS OK`, `tst_render_images .*Passed`. Banc hors écran (module `render/`) :
`content_check.ok = True` (page du banc inchangée).

**Interdits.** Ne pas changer la signature de `image(std::size_t)` ni de `imageCount()` (banc, tests du lot 1).
Aucun décodage sur le fil de l'objet. Pas de dépendance à `pagepainter.h` (tâche parallèle L2-T11) : le masquage de
couleur est écrit ici (six lignes), comme dans le peintre.

**Terminé quand.** Preuve verte, JSON du banc joint, commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests.** Dans `tests/tst_render_images.cpp`, ajouter les includes
  `<actcraft/core/document.h>` et `<QBuffer>`, une fonction utilitaire avant la classe :

```cpp
namespace {

QByteArray pngOf(const QImage& image)
{
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return png;
}

} // namespace
```

  puis ces fonctions de test à la classe :

```cpp
    void resourceImagesAreDecodedOffThreadAndMasked()
    {
        QImage source(4, 4, QImage::Format_ARGB32);
        source.fill(Qt::white);
        source.setPixelColor(1, 1, Qt::red);
        core::Document document;
        const std::string key = document.addResource(pngOf(source), "png");
        core::ImageObject plain;
        plain.bounds = {0, 0, 4, 4};
        plain.resource = key;
        core::ImageObject masked = plain;
        masked.hasTransparentColor = true;
        masked.transparentColor = 0xFFFFFFFFu;
        core::Page page;
        page.objects.emplace_back(plain);
        page.objects.emplace_back(masked);
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(page, &document);
        QVERIFY(ready.wait(5000));
        QVERIFY(store.generatedOffOwnerThread());
        QCOMPARE(store.resourceImage(key).size(), QSize(4, 4));
        QCOMPARE(store.objectImage(plain, 0).pixelColor(0, 0), QColor(Qt::white));
        const QImage cut = store.objectImage(masked, 1);
        QCOMPARE(cut.pixelColor(0, 0).alpha(), 0);
        QCOMPARE(cut.pixelColor(1, 1), QColor(Qt::red));
        QVERIFY(render::ImageStore::variantKey(masked) != render::ImageStore::variantKey(plain));
        QCOMPARE(store.imageCount(), std::size_t{0}); // aucune image procédurale
    }

    void placeholderAndBackgroundAreDecoded()
    {
        QImage source(2, 2, QImage::Format_ARGB32);
        source.fill(Qt::blue);
        core::Document document;
        const std::string key = document.addResource(pngOf(source), "png");
        core::Media media;
        media.bounds = {0, 0, 10, 10};
        media.placeholder = key;
        core::Page page;
        page.background.image = key;
        page.objects.emplace_back(media);
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(page, &document);
        QVERIFY(ready.wait(5000));
        QCOMPARE(store.resourceImage(key).pixelColor(0, 0), QColor(Qt::blue));
    }

    void unreadableOrMissingResourcesGiveNullImages()
    {
        core::Document document;
        const std::string bad = document.addResource(QByteArray("pas une image"), "png");
        core::ImageObject a;
        a.resource = bad;
        core::ImageObject b;
        b.resource = "absente";
        core::Page page;
        page.objects.emplace_back(a);
        page.objects.emplace_back(b);
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(page, &document);
        QVERIFY(ready.wait(5000));
        QVERIFY(store.isReady());
        QVERIFY(store.resourceImage(bad).isNull());
        QVERIFY(store.objectImage(b, 1).isNull());
    }

    void resourceImageRendersInSoftware()
    {
        QImage source(8, 8, QImage::Format_ARGB32);
        source.fill(Qt::red);
        core::Document document;
        core::ImageObject im;
        im.bounds = {20, 20, 64, 64};
        im.resource = document.addResource(pngOf(source), "png");
        core::Page page;
        page.objects.emplace_back(im);
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(page, &document);
        QVERIFY(ready.wait(5000));
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(200, 200);
        auto* host = new RenderHostItem;
        host->setSize(QSizeF(200, 200));
        host->objects = page.objects;
        host->images = &store;
        host->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(colorNear(window.grabWindow().pixelColor(52, 52), QColor(Qt::red), 3));
    }
```

- [ ] **Étape 2 : constater le rouge.** Commande ciblée, motif `tst_render_images` → `error`.

- [ ] **Étape 3 : étendre l'en-tête.** Dans `render/include/actcraft/render/imagestore.h` : ajouter
  `#include <actcraft/core/document.h>` et `#include <string>`, puis remplacer la déclaration de `generate` et ajouter
  les accès, dans la section publique :

```cpp
    // Images de la page sur des fils secondaires : procédurales par indice d'objet, ressources de `resources` par
    // clé (images, espaces réservés des médias, image de fond), une fois chacune, et variantes à couleur
    // transparente. ready() quand tout est prêt. Sans `resources`, les ressources restent absentes (images nulles).
    void generate(const core::Page& page, const core::Document* resources = nullptr);
    // Ressource décodée par sa clé, sans masque ; nulle si absente ou illisible. Sûre depuis le fil de rendu.
    QImage resourceImage(const std::string& key) const;
    // Image à dessiner pour cet objet : ressource (variante masquée si couleur transparente) ou image procédurale
    // d'indice `objectIndex`.
    QImage objectImage(const core::ImageObject& object, std::size_t objectIndex) const;
    // Clé de la ressource, suivie de « #aarrggbb » si l'objet a une couleur transparente.
    static std::string variantKey(const core::ImageObject& object);
```

  et dans la section privée, remplacer la signature de `onImageReady` par :

```cpp
    void onImageReady(std::uint64_t generation, std::size_t index, std::string key, const QImage& image,
                      bool offOwnerThread);
```

  et ajouter le membre `std::unordered_map<std::string, QImage> m_resources;` (sous `m_images`). `imageCount()`
  continue de compter les seules images procédurales.

- [ ] **Étape 4 : écrire l'implémentation.** Dans `render/src/imagestore.cpp`, ajouter les includes `<set>` et
  `<utility>`, une fonction locale et remplacer `generate` et `onImageReady` :

```cpp
namespace {

// Pixels de la couleur `transparent` (alpha ignoré) rendus transparents (même règle que pagepainter.cpp).
QImage withTransparentColor(const QImage& source, core::Argb transparent)
{
    QImage image = source.convertToFormat(QImage::Format_ARGB32);
    const QRgb key = transparent & 0x00FFFFFFu;
    for (int y = 0; y < image.height(); ++y) {
        auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            if ((line[x] & 0x00FFFFFFu) == key) {
                line[x] = 0;
            }
        }
    }
    return image;
}

} // namespace

std::string ImageStore::variantKey(const core::ImageObject& object)
{
    if (!object.hasTransparentColor) {
        return object.resource;
    }
    return object.resource + QStringLiteral("#%1").arg(object.transparentColor, 8, 16, QLatin1Char('0')).toStdString();
}

void ImageStore::generate(const core::Page& page, const core::Document* resources)
{
    const std::uint64_t generation = ++m_generation;
    {
        QMutexLocker lock(&m_mutex);
        m_images.clear();
        m_resources.clear();
    }
    m_ready = false;
    m_pending = 0;
    m_offOwnerThread = true;
    QThread* const owner = thread();
    const auto post = [this, generation, owner](std::size_t index, std::string key, QImage image) {
        const bool off = QThread::currentThread() != owner;
        QMetaObject::invokeMethod(
            this,
            [this, generation, index, key = std::move(key), image = std::move(image), off]() mutable {
                onImageReady(generation, index, std::move(key), image, off);
            },
            Qt::QueuedConnection);
    };
    // Ressources à décoder : clé → couleurs transparentes demandées (0 = aucune variante).
    std::map<std::string, std::set<std::optional<core::Argb>>> wanted;
    const auto want = [&wanted, resources](const std::string& key, std::optional<core::Argb> transparent) {
        if (!key.empty() && resources && resources->resource(key)) {
            wanted[key].insert(transparent);
        }
    };
    want(page.background.image, std::nullopt);
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        if (const auto* object = std::get_if<core::ImageObject>(&page.objects[i])) {
            if (object->resource.empty()) {
                ++m_pending;
                const core::ImageObject copy = *object;
                m_pool.start([post, i, copy] { post(i, std::string(), synthesizeImage(copy)); });
            } else {
                want(object->resource, object->hasTransparentColor ? std::optional<core::Argb>(object->transparentColor)
                                                                   : std::nullopt);
            }
        } else if (const auto* media = std::get_if<core::Media>(&page.objects[i])) {
            want(media->placeholder, std::nullopt);
        }
    }
    for (const auto& [key, variants] : wanted) {
        ++m_pending;
        const QByteArray bytes = resources->resource(key)->data; // partagée implicitement : copie sûre entre fils
        m_pool.start([this, generation, owner, key, variants, bytes] {
            const QImage decoded = QImage::fromData(bytes);
            const bool off = QThread::currentThread() != owner;
            std::vector<std::pair<std::string, QImage>> out{{key, decoded}};
            for (const std::optional<core::Argb>& color : variants) {
                if (color && !decoded.isNull()) {
                    core::ImageObject probe;
                    probe.resource = key;
                    probe.hasTransparentColor = true;
                    probe.transparentColor = *color;
                    out.emplace_back(variantKey(probe), withTransparentColor(decoded, *color));
                }
            }
            QMetaObject::invokeMethod(
                this,
                [this, generation, out = std::move(out), off] {
                    if (generation != m_generation) {
                        return;
                    }
                    {
                        QMutexLocker lock(&m_mutex);
                        for (const auto& [k, image] : out) {
                            m_resources[k] = image;
                        }
                    }
                    onImageReady(generation, 0, std::string(), QImage(), off);
                },
                Qt::QueuedConnection);
        });
    }
    if (m_pending == 0) {
        QMetaObject::invokeMethod(
            this,
            [this, generation] {
                if (generation == m_generation) {
                    m_ready = true;
                    emit ready();
                }
            },
            Qt::QueuedConnection);
    }
}

void ImageStore::onImageReady(std::uint64_t generation, std::size_t index, std::string key, const QImage& image,
                              bool offOwnerThread)
{
    if (generation != m_generation) {
        return;
    }
    m_offOwnerThread = m_offOwnerThread && offOwnerThread;
    if (key.empty() && !image.isNull()) {
        QMutexLocker lock(&m_mutex);
        m_images[index] = image;
    }
    if (--m_pending == 0) {
        m_ready = true;
        emit ready();
    }
}

QImage ImageStore::resourceImage(const std::string& key) const
{
    QMutexLocker lock(&m_mutex);
    const auto it = m_resources.find(key);
    return it == m_resources.end() ? QImage() : it->second;
}

QImage ImageStore::objectImage(const core::ImageObject& object, std::size_t objectIndex) const
{
    return object.resource.empty() ? image(objectIndex) : resourceImage(variantKey(object));
}
```

  Le lambda des images procédurales passe une clé vide et une image ; celui des ressources range ses images
  lui-même puis compte la tâche par `onImageReady(…, QImage(), …)`. `std::set<std::optional<core::Argb>>` ordonne
  `nullopt` avant toute valeur : correct. Ajouter `#include <map>` et `#include <optional>`. Garder le destructeur
  (attente des tâches), `isReady`, `imageCount`, `generatedOffOwnerThread`, `image` tels quels.

  Dans `render/src/imagerenderer.cpp`, remplacer `const QImage pixels = context.images->image(objectIndex);` par :

```cpp
    const QImage pixels = context.images->objectImage(*image, objectIndex);
```

- [ ] **Étape 5 : avis des composants tiers.** Le déploiement Windows (`windeployqt`) emporte les modules
  d'extension de Qt Image Formats (`plugins/imageformats` : `qtiff`, `qwebp`, `qicns`, `qtga`, `qwbmp`…). Lister ceux
  présents dans `C:\Qt\6.12.0\msvc2022_64\plugins\imageformats`, puis ajouter à la table « Composants tiers inclus
  dans Qt » de `THIRD_PARTY_NOTICES.md` les lignes du module « Qt Image Formats » relevées sur
  https://doc.qt.io/qt-6/licenses-used-in-qt.html (même source que les lignes du lot 1, L1-T11 ; une ligne par
  composant : module, composant, licence). Si la page n'est pas joignable, s'arrêter à cette étape et le rapporter (ne
  pas écrire de licence de mémoire). `tst_ui_about` doit rester vert.

- [ ] **Étape 6 : vert ciblé, preuve et banc.** Commande ciblée (`tst_render_images`) : `100% tests passed`.
  `format.ps1`, preuve de tâche (N ≥ 55 ; `tst_render_images` Passed), banc sans écran (`content_check.ok = True`).

- [ ] **Étape 7 : commit.**

```powershell
git -C <worktree> add render/include/actcraft/render/imagestore.h render/src/imagestore.cpp render/src/imagerenderer.cpp tests/tst_render_images.cpp THIRD_PARTY_NOTICES.md
git -C <worktree> commit -m "render : images des ressources décodées sur fils secondaires, variantes à couleur transparente ; avis de Qt Image Formats" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t13"></a>
### Tâche L2-T13 — `render` : nouveaux types dans la scène (moteur peint), fond, grille, transformation et translucidité

**Objectif.** Décision 5 (rendu des nouveaux types, fond couleur / dégradé / image, grille à niveaux, quatre couches,
ordre couche puis profondeur puis arbre) et décision 37 (texte dessiné par le peintre partagé dans une texture pour
le rendu RHI, redessinée après un zoom). La page fixe dessine :

- le **fond** de la page (rectangle uni si possible, sinon peint) puis la **grille** (sous les objets, ou au-dessus
  si `onTop`) ;
- chaque objet visible, dans l'ordre du vecteur (déjà rangé par couche puis profondeur), par le moteur du
  prototype s'il sait le faire (formes simples, trait du stylo à bouts carrés, image), sinon par le **moteur peint** :
  en rendu logiciel, un nœud QPainter qui appelle `paintObjectLocal` (vectoriel, net à tout zoom) ; en RHI, une
  texture rastérisée par le même peintre à `rasterScale` (pixels de page → pixels d'écran), refaite quand l'échelle
  change ;
- pour tout objet, sa **transformation** par un `QSGTransformNode` et sa **translucidité** par un `QSGOpacityNode`
  parents, quel que soit le moteur.

Patron à **ne pas** reprendre : que le moteur peint applique lui-même la transformation (double transformation sous un
nœud parent) ou lise les images du magasin au moment de peindre (un nœud gardé pendant un changement de page
afficherait les images de la page suivante). Le nœud peint **copie** à sa création les images dont il a besoin.

**Fichiers.**
- Créer : `render/include/actcraft/render/paintedrenderer.h`, `render/src/paintedrenderer.cpp`,
  `tests/tst_render_painted.cpp`
- Modifier : `render/include/actcraft/render/objectrenderer.h` (`RenderContext::rasterScale`),
  `render/include/actcraft/render/fixedpageitem.h`, `render/src/fixedpageitem.cpp`, `render/CMakeLists.txt`,
  `tests/render.cmake`, `tests/tst_render_fixedpage.cpp`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_render_(painted|fixedpage)` → `error`. Vert : forme
bornée, N ≥ 57, `TESTS OK`, `tst_render_painted .*Passed`, `tst_render_fixedpage .*Passed`. Banc hors écran :
`content_check.ok = True` (la page du banc n'a que des objets du prototype : aucun changement attendu ; un écart de
mesure notable est à rapporter).

**Interdits.** Ne pas modifier `ShapeRenderer`, `StrokeRenderer`, `ImageRenderer` (sauf leur choix par
`FixedPageItem`). Aucune texture rastérisée en rendu logiciel. `setRasterScale` ne reconstruit rien en rendu
logiciel ni sur une page sans objet peint, fond peint ou grille (la page du banc ne doit pas être reconstruite à
chaque fin de zoom).

**Terminé quand.** Preuve verte, JSON du banc joint, commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests.** Créer `tests/tst_render_painted.cpp` :

```cpp
#include "support/renderhost.h"

#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/render/paintedrenderer.h>

#include <QSignalSpy>
#include <QTest>

using namespace actcraft;

namespace {

core::PageObject pathSquare(double size, core::Argb fill)
{
    core::Shape s;
    s.kind = core::ShapeKind::Path;
    s.paths = {{{0, 0}, {size, 0}, {size, size}, {0, size}}};
    s.pen.style = core::PenStyle::None;
    s.brush = core::solidFill(fill);
    return core::PageObject(s);
}

core::PageObject redText()
{
    core::Text t;
    t.bounds = {0, 0, 300, 60};
    t.html = "<p style=\"color:#ff0000; font-size:40px\">XXXX</p>";
    core::PageObject o = t;
    o.common.transform = core::translation(100, 100);
    return o;
}

const render::ImageLookup kNoImages = [](const std::string&) { return QImage(); };

} // namespace

// Moteur peint, fond, grille, transformations dans la page fixe (rendu logiciel des tests).
class TstRenderPainted : public QObject
{
    Q_OBJECT

private:
    struct Fixture
    {
        QQuickWindow window;
        render::FixedPageItem* page = nullptr;
    };

    static bool open(Fixture& f, const core::Page& page)
    {
        f.window.setColor(QColor(0x3c, 0x3f, 0x45));
        f.window.resize(1024, 768);
        f.page = new render::FixedPageItem;
        f.page->setSize(QSizeF(1024, 768));
        f.page->setParentItem(f.window.contentItem());
        f.page->setPage(page);
        f.window.show();
        return QTest::qWaitForWindowExposed(&f.window) && QTest::qWaitFor([&f] { return f.page->isReady(); }, 5000);
    }

    static QImage grab(Fixture& f)
    {
        QSignalSpy swapped(&f.window, &QQuickWindow::frameSwapped);
        f.page->update();
        swapped.wait(2000);
        return f.window.grabWindow();
    }

private slots:
    void usesPaintedRendererChoosesTheEngine()
    {
        QVERIFY(render::usesPaintedRenderer(redText()));
        QVERIFY(render::usesPaintedRenderer(core::PageObject(core::Line{})));
        QVERIFY(render::usesPaintedRenderer(core::PageObject(core::Media{})));
        QVERIFY(render::usesPaintedRenderer(core::PageObject(core::Connector{})));
        QVERIFY(render::usesPaintedRenderer(pathSquare(10, 0xFF000000u)));
        core::Stroke highlighter;
        highlighter.kind = core::StrokeKind::Highlighter;
        QVERIFY(render::usesPaintedRenderer(core::PageObject(highlighter)));
        core::Stroke round;
        round.cap = core::PenCap::Round;
        QVERIFY(render::usesPaintedRenderer(core::PageObject(round)));
        QVERIFY(!render::usesPaintedRenderer(core::PageObject(core::Stroke{})));
        QVERIFY(!render::usesPaintedRenderer(core::PageObject(core::Shape{})));
        QVERIFY(!render::usesPaintedRenderer(core::PageObject(core::ImageObject{})));
        QVERIFY(!render::usesPaintedRenderer(core::PageObject(core::Group{})));
    }

    void rasterizeLocalCoversTheObjectAtScale()
    {
        QRectF rect;
        const QImage image = render::rasterizeLocal(redText(), kNoImages, 2.0, &rect);
        QCOMPARE(rect, QRectF(0, 0, 300, 60));
        QVERIFY(image.width() >= 600 && image.height() >= 120);
        bool red = false;
        for (int y = 0; y < image.height() && !red; ++y) {
            for (int x = 0; x < image.width() && !red; ++x) {
                red = image.pixelColor(x, y).red() > 200 && image.pixelColor(x, y).alpha() > 200 &&
                      image.pixelColor(x, y).green() < 80;
            }
        }
        QVERIFY(red);
    }

    void textIsDrawnInTheScene()
    {
        core::Page page;
        page.objects.push_back(redText());
        Fixture f;
        QVERIFY(open(f, page));
        const QImage image = grab(f);
        int red = 0;
        for (int y = 100; y < 160; ++y) {
            for (int x = 100; x < 400; ++x) {
                const QColor c = image.pixelColor(x, y);
                red += c.red() > 200 && c.green() < 80 ? 1 : 0;
            }
        }
        QVERIFY2(red > 200, qPrintable(QString::number(red)));
    }

    void transformedAndTranslucentObjects()
    {
        core::Page page;
        core::PageObject rotated = pathSquare(40, 0xFF0000FFu);
        rotated.common.transform = core::compose(core::rotation(90), core::translation(200, 200));
        page.objects.push_back(rotated);
        core::Shape legacy; // moteur du prototype, sous un nœud de transformation
        legacy.kind = core::ShapeKind::Rectangle;
        legacy.bounds = {0, 0, 40, 40};
        legacy.fill = 0xFF00FF00u;
        legacy.outlineWidth = 0;
        core::PageObject moved = legacy;
        moved.common.transform = core::translation(400, 100);
        page.objects.push_back(moved);
        core::PageObject half = pathSquare(40, 0xFF000000u);
        half.common.transform = core::translation(600, 100);
        half.common.translucency = 128;
        page.objects.push_back(half);
        core::PageObject hidden = pathSquare(40, 0xFF000000u);
        hidden.common.transform = core::translation(800, 100);
        hidden.common.visible = false;
        page.objects.push_back(hidden);
        Fixture f;
        QVERIFY(open(f, page));
        const QImage image = grab(f);
        QVERIFY(colorNear(image.pixelColor(190, 210), QColor(Qt::blue), 3));
        QVERIFY(colorNear(image.pixelColor(210, 210), QColor(Qt::white), 3));
        QVERIFY(colorNear(image.pixelColor(420, 120), QColor(Qt::green), 3));
        const int grey = image.pixelColor(620, 120).red();
        QVERIFY2(grey > 110 && grey < 145, qPrintable(QString::number(grey)));
        QVERIFY(colorNear(image.pixelColor(820, 120), QColor(Qt::white), 1));
    }

    void backgroundAndGrid()
    {
        core::Page page;
        page.background.fill.kind = core::FillKind::Gradient;
        page.background.fill.color = 0xFF000000u;
        page.background.fill.color2 = 0xFFFFFFFFu;
        page.background.fill.gradient = core::GradientKind::Vertical;
        Fixture f;
        QVERIFY(open(f, page));
        const QImage image = grab(f);
        QVERIFY(image.pixelColor(500, 3).red() < 30);
        QVERIFY(image.pixelColor(500, 764).red() > 225);
    }

    void gridOnTopStaysAboveObjects()
    {
        core::Page page;
        core::PageObject red = pathSquare(300, 0xFFFF0000u);
        page.objects.push_back(red);
        page.grid.visible = true;
        page.grid.onTop = true;
        core::GridLevel level;
        level.stepX = 100;
        level.stepY = 100;
        page.grid.levels.push_back(level);
        Fixture f;
        QVERIFY(open(f, page));
        const QImage image = grab(f);
        QVERIFY(colorNear(image.pixelColor(100, 50), QColor(Qt::black), 20)); // trait de grille sur le carré rouge
        QVERIFY(colorNear(image.pixelColor(150, 50), QColor(Qt::red), 3));
    }

    void sceneMatchesThePainter()
    {
        core::Page page;
        page.background.fill.kind = core::FillKind::Gradient;
        page.background.fill.color = 0xFFC8DCFFu;
        page.background.fill.color2 = 0xFFFFFFFFu;
        core::Shape rect;
        rect.kind = core::ShapeKind::Rectangle;
        rect.bounds = {50, 50, 200, 120};
        rect.fill = 0xFF336699u;
        rect.outlineWidth = 3;
        page.objects.push_back(rect);
        core::PageObject square = pathSquare(150, 0xFF00AA00u);
        square.common.transform = core::compose(core::rotation(30), core::translation(500, 120));
        page.objects.push_back(square);
        page.objects.push_back(redText());
        core::Line line;
        line.points = {{600, 500}, {900, 650}};
        line.pen.width = 6;
        line.pen.style = core::PenStyle::Dash;
        page.objects.push_back(line);
        core::Stroke pen;
        pen.baseWidth = 8;
        pen.points = {{100, 600, 0.5f}, {300, 650, 0.5f}, {400, 600, 0.5f}};
        page.objects.push_back(pen);
        core::Stroke marker;
        marker.kind = core::StrokeKind::Highlighter;
        marker.cap = core::PenCap::Round;
        marker.baseWidth = 30;
        marker.color = 0x80FFFF00u;
        marker.points = {{100, 400, 0.5f}, {500, 420, 0.5f}};
        core::PageObject top = marker;
        top.common.layer = core::Layer::Top;
        page.objects.push_back(top);
        Fixture f;
        QVERIFY(open(f, page));
        const QImage scene = grab(f).convertToFormat(QImage::Format_RGB32);
        const QImage painted = render::renderPage(page, kNoImages, 1.0).convertToFormat(QImage::Format_RGB32);
        QCOMPARE(scene.size(), painted.size());
        double total = 0;
        for (int y = 0; y < scene.height(); ++y) {
            for (int x = 0; x < scene.width(); ++x) {
                const QColor a = scene.pixelColor(x, y);
                const QColor b = painted.pixelColor(x, y);
                total += qAbs(a.red() - b.red()) + qAbs(a.green() - b.green()) + qAbs(a.blue() - b.blue());
            }
        }
        const double mean = total / (3.0 * scene.width() * scene.height());
        QVERIFY2(mean < 2.0, qPrintable(QString::number(mean)));
    }

    void rasterScaleRebuildsOnlyPaintedPagesInRhi()
    {
        core::Page page;
        page.objects.push_back(core::PageObject(core::Shape{}));
        Fixture f;
        QVERIFY(open(f, page));
        f.page->setRasterScale(2.0);
        QCOMPARE(f.page->rasterScale(), 2.0);
        // Rendu logiciel : rien à reconstruire (vérifié par l'absence de nouvelle construction au prochain dessin).
        const QImage before = grab(f);
        QVERIFY(!before.isNull());
    }
};

QTEST_MAIN(TstRenderPainted)
#include "tst_render_painted.moc"
```

  Ajouter à `tests/render.cmake` :

```cmake
actcraft_add_test(tst_render_painted
    SOURCES tst_render_painted.cpp
    LIBS ActCraft::render Qt6::Quick)
```

  La fenêtre fait 1024 × 768 et la page fixe est en (0, 0) à l'échelle 1 : la capture de la scène et l'image du
  peintre ont la même taille. Dans `tests/tst_render_fixedpage.cpp`, ajouter :

```cpp
    void solidBackgroundColorIsDrawn()
    {
        Fixture f;
        open(f);
        core::Page page;
        page.background.fill = core::solidFill(0xFF00FF00u);
        f.page->setPage(page);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(500, 400), QColor(Qt::green), 2));
    }
```

- [ ] **Étape 2 : constater le rouge.** Commande ciblée, motif `tst_render_(painted|fixedpage)` → `error`.

- [ ] **Étape 3 : écrire l'en-tête.** Dans `render/include/actcraft/render/objectrenderer.h`, ajouter à
  `RenderContext` : `double rasterScale = 1; // pixels d'écran par pixel de page (RHI : rastérisation du moteur peint)`.
  Créer `render/include/actcraft/render/paintedrenderer.h` :

```cpp
#pragma once

#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/pagepainter.h>

#include <QImage>
#include <QRectF>

class QSGNode;

namespace actcraft::render {

// Vrai si l'objet passe par le peintre partagé plutôt que par un moteur du prototype : texte, ligne, média,
// connecteur, forme à chemins, trait autre que le stylo à bouts carrés.
bool usesPaintedRenderer(const core::PageObject& object) noexcept;

// Objet en coordonnées de l'objet, rastérisé à `scale` dans une image ARGB prémultipliée ; `localRect` reçoit le
// rectangle couvert (coordonnées de l'objet, épaisseur comprise).
QImage rasterizeLocal(const core::PageObject& object, const ImageLookup& images, double scale, QRectF* localRect);

// Moteur peint (DA5, décision 37). Sa transformation et sa translucidité sont posées par la page fixe (nœuds
// parents) : il dessine en coordonnées de l'objet. Logiciel : nœud QPainter vectoriel ; RHI : texture rastérisée à
// context.rasterScale. Copie à sa création les images dont il a besoin.
class PaintedRenderer
{
public:
    QSGNode* createNode(const core::PageObject& object, const RenderContext& context) const;
};

// Fond de la page (rectangle uni si possible, sinon peint) ; grille (nullptr si invisible ou sans niveau).
QSGNode* createBackgroundNode(const core::Page& page, const RenderContext& context);
QSGNode* createGridNode(const core::Page& page, const RenderContext& context);

} // namespace actcraft::render
```

- [ ] **Étape 4 : écrire le moteur peint.** Créer `render/src/paintedrenderer.cpp` :

```cpp
#include <actcraft/core/edits.h>
#include <actcraft/render/imagestore.h>
#include <actcraft/render/paintedrenderer.h>
#include <actcraft/render/painternode.h>

#include <QPainter>
#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGRectangleNode>
#include <QSGTexture>

#include <cmath>
#include <utility>

namespace actcraft::render {

namespace {

// Images dont l'objet a besoin, copiées (partage implicite de QImage) : le nœud ne lit plus le magasin ensuite.
DecodedImages imagesOf(const core::PageObject& object, const ImageStore* store)
{
    DecodedImages out;
    if (!store) {
        return out;
    }
    if (const auto* m = std::get_if<core::Media>(&object); m && !m->placeholder.empty()) {
        out.insert(m->placeholder, store->resourceImage(m->placeholder));
    }
    if (const auto* im = std::get_if<core::ImageObject>(&object); im && !im->resource.empty()) {
        out.insert(im->resource, store->resourceImage(im->resource));
    }
    return out;
}

DecodedImages backgroundImageOf(const core::Page& page, const ImageStore* store)
{
    DecodedImages out;
    if (store && !page.background.image.empty()) {
        out.insert(page.background.image, store->resourceImage(page.background.image));
    }
    return out;
}

class PaintedObjectNode final : public PainterNode
{
public:
    PaintedObjectNode(QQuickWindow* window, core::PageObject object, DecodedImages images, const QRectF& rect)
        : PainterNode(window), m_object(std::move(object)), m_images(std::move(images))
    {
        setRect(rect);
    }

protected:
    void paint(QPainter& painter) override { paintObjectLocal(painter, m_object, m_images.lookup()); }

private:
    core::PageObject m_object;
    DecodedImages m_images;
};

class PaintedPageNode final : public PainterNode
{
public:
    enum class Part
    {
        Background,
        Grid,
    };
    PaintedPageNode(QQuickWindow* window, core::Page page, DecodedImages images, Part part)
        : PainterNode(window), m_page(std::move(page)), m_images(std::move(images)), m_part(part)
    {
        setRect(QRectF(0, 0, m_page.width, m_page.height));
    }

protected:
    void paint(QPainter& painter) override
    {
        if (m_part == Part::Background) {
            paintBackground(painter, m_page, m_images.lookup());
        } else {
            paintGrid(painter, m_page, false);
        }
    }

private:
    core::Page m_page; // sans ses objets (copie légère)
    DecodedImages m_images;
    Part m_part;
};

core::Page withoutObjects(const core::Page& page)
{
    core::Page copy;
    copy.width = page.width;
    copy.height = page.height;
    copy.background = page.background;
    copy.grid = page.grid;
    return copy;
}

QSGNode* textureNode(QQuickWindow* window, const QImage& image, const QRectF& rect)
{
    if (image.isNull() || !window) {
        return nullptr;
    }
    QSGImageNode* node = window->createImageNode();
    node->setTexture(window->createTextureFromImage(image));
    node->setOwnsTexture(true);
    node->setFiltering(QSGTexture::Linear);
    node->setRect(rect);
    return node;
}

QImage rasterizePage(const core::Page& page, const ImageLookup& images, double scale, bool grid)
{
    const int w = std::max(1, static_cast<int>(std::ceil(page.width * scale)));
    const int h = std::max(1, static_cast<int>(std::ceil(page.height * scale)));
    QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
    image.setDotsPerMeterX(3780);
    image.setDotsPerMeterY(3780);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.scale(scale, scale);
    if (grid) {
        paintGrid(p, page, false);
    } else {
        paintBackground(p, page, images);
    }
    return image;
}

} // namespace

bool usesPaintedRenderer(const core::PageObject& object) noexcept
{
    if (std::get_if<core::Text>(&object) || std::get_if<core::Line>(&object) || std::get_if<core::Media>(&object) ||
        std::get_if<core::Connector>(&object)) {
        return true;
    }
    if (const auto* s = std::get_if<core::Shape>(&object)) {
        return s->kind == core::ShapeKind::Path;
    }
    if (const auto* st = std::get_if<core::Stroke>(&object)) {
        return st->kind != core::StrokeKind::Pen || st->cap != core::PenCap::Square;
    }
    return false;
}

QImage rasterizeLocal(const core::PageObject& object, const ImageLookup& images, double scale, QRectF* localRect)
{
    core::PageObject local = object;
    local.common.transform = core::Transform{};
    const core::RectF b = core::boundsOnPage(local);
    const QRectF rect(b.x, b.y, std::max(b.width, 1.0), std::max(b.height, 1.0));
    if (localRect) {
        *localRect = rect;
    }
    const int w = std::max(1, static_cast<int>(std::ceil(rect.width() * scale)));
    const int h = std::max(1, static_cast<int>(std::ceil(rect.height() * scale)));
    QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
    image.setDotsPerMeterX(3780);
    image.setDotsPerMeterY(3780);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.scale(scale, scale);
    p.translate(-rect.x(), -rect.y());
    paintObjectLocal(p, local, images);
    p.end();
    return image;
}

QSGNode* PaintedRenderer::createNode(const core::PageObject& object, const RenderContext& context) const
{
    DecodedImages images = imagesOf(object, context.images);
    if (context.software) {
        core::PageObject local = object;
        local.common.transform = core::Transform{};
        const core::RectF b = core::boundsOnPage(local);
        // Marge d'un pixel : l'anticrénelage déborde du rectangle exact.
        const QRectF rect(b.x - 1, b.y - 1, b.width + 2, b.height + 2);
        return new PaintedObjectNode(context.window, std::move(local), std::move(images), rect);
    }
    QRectF rect;
    const QImage image = rasterizeLocal(object, images.lookup(), std::max(context.rasterScale, 0.01), &rect);
    return textureNode(context.window, image, rect);
}

QSGNode* createBackgroundNode(const core::Page& page, const RenderContext& context)
{
    const core::Fill& fill = page.background.fill;
    const QRectF rect(0, 0, page.width, page.height);
    if (page.background.image.empty() && (fill.kind == core::FillKind::Solid || fill.kind == core::FillKind::None)) {
        QSGRectangleNode* node = context.window->createRectangleNode();
        node->setRect(rect);
        node->setColor(fill.kind == core::FillKind::Solid ? QColor::fromRgba(fill.color) : QColor(Qt::white));
        return node;
    }
    DecodedImages images = backgroundImageOf(page, context.images);
    if (context.software) {
        return new PaintedPageNode(context.window, withoutObjects(page), std::move(images),
                                   PaintedPageNode::Part::Background);
    }
    return textureNode(context.window, rasterizePage(page, images.lookup(), std::max(context.rasterScale, 0.01), false),
                       rect);
}

QSGNode* createGridNode(const core::Page& page, const RenderContext& context)
{
    if (!page.grid.visible || page.grid.levels.empty()) {
        return nullptr;
    }
    if (context.software) {
        return new PaintedPageNode(context.window, withoutObjects(page), DecodedImages{}, PaintedPageNode::Part::Grid);
    }
    return textureNode(context.window, rasterizePage(page, ImageLookup{}, std::max(context.rasterScale, 0.01), true),
                       QRectF(0, 0, page.width, page.height));
}

} // namespace actcraft::render
```

  `render` dépend de `core` : `core/edits.h` (L2-T2) donne `boundsOnPage`. Le fond « aucun » reste blanc (la page est
  blanche par défaut, F-610).

- [ ] **Étape 5 : brancher la page fixe.** Dans `render/include/actcraft/render/fixedpageitem.h` : include
  `<actcraft/render/paintedrenderer.h>` ; méthodes publiques

```cpp
    // Pixels d'écran par pixel de page : échelle de rastérisation du moteur peint en RHI (décision 37). Ne
    // reconstruit qu'en RHI et seulement si la page a des éléments peints.
    void setRasterScale(double scale);
    double rasterScale() const noexcept;
```

  et membres privés `PaintedRenderer m_painted;`, `double m_rasterScale = 1;`, `QSGNode* m_gridOnTop = nullptr;`,
  `bool m_hasPainted = false;`.

  Dans `render/src/fixedpageitem.cpp`, ajouter les includes `<QSGOpacityNode>`, `<QMatrix4x4>` et `<algorithm>`,
  deux fonctions locales (dans l'espace anonyme existant) :

```cpp
// Transformation et translucidité de l'objet par des nœuds parents, quel que soit son moteur.
QSGNode* wrapped(QSGNode* node, const core::ObjectCommon& common)
{
    if (!common.transform.isIdentity()) {
        const core::Transform& t = common.transform;
        auto* transform = new QSGTransformNode;
        transform->setMatrix(QMatrix4x4(QTransform(t.m11, t.m12, t.m21, t.m22, t.dx, t.dy)));
        transform->appendChildNode(node);
        node = transform;
    }
    if (common.translucency != 0) {
        auto* opacity = new QSGOpacityNode;
        opacity->setOpacity(1.0 - common.translucency / 255.0);
        opacity->appendChildNode(node);
        node = opacity;
    }
    return node;
}

bool pagePaintsSomething(const core::Page& page)
{
    if (!page.background.image.empty() || page.background.fill.kind == core::FillKind::Gradient ||
        page.background.fill.kind == core::FillKind::Pattern || page.grid.visible) {
        return true;
    }
    return std::any_of(page.objects.begin(), page.objects.end(),
                       [](const core::PageObject& o) { return usesPaintedRenderer(o); });
}
```

  les méthodes :

```cpp
void FixedPageItem::setRasterScale(double scale)
{
    if (scale <= 0 || qFuzzyCompare(scale, m_rasterScale)) {
        return;
    }
    m_rasterScale = scale;
    const bool software =
        window() && window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
    if (!software && pagePaintsSomething(m_page)) {
        m_rebuild = true;
        update();
    }
}

double FixedPageItem::rasterScale() const noexcept
{
    return m_rasterScale;
}
```

  et remplacer `updatePaintNode` par :

```cpp
QSGNode* FixedPageItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
    const bool software = window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
    // Logiciel : racine de transformation (identité). Sous une racine QSGNode nue déjà rendue, le moteur
    // logiciel de Qt 6.12 peint les nœuds ajoutés sans la transformation de l'élément (voir InkItem).
    QSGNode* root = old ? old : (software ? new QSGTransformNode : new QSGNode);
    RenderContext context;
    context.window = window();
    context.software = software;
    context.images = &m_images;
    context.rasterScale = m_rasterScale;
    const core::Page& shown = m_page;
    if (m_rebuild || !old) {
        clearChildren(root);
        m_gridOnTop = nullptr;
        if (m_ready) {
            root->appendChildNode(createBackgroundNode(shown, context));
            if (QSGNode* grid = createGridNode(shown, context)) {
                if (shown.grid.onTop) {
                    m_gridOnTop = grid;
                }
                root->appendChildNode(grid);
            }
        } else {
            QSGRectangleNode* background = window()->createRectangleNode();
            background->setRect(boundingRect());
            background->setColor(Qt::white);
            root->appendChildNode(background); // tant que les images manquent, on ne dessine que le fond
        }
        m_builtCount = 0;
        m_rebuild = !m_ready;
    }
    if (m_ready) {
        for (std::size_t i = m_builtCount; i < shown.objects.size(); ++i) {
            const core::PageObject& object = shown.objects[i];
            if (!object.common.visible || std::get_if<core::Group>(&object)) {
                continue;
            }
            QSGNode* node = nullptr;
            if (usesPaintedRenderer(object)) {
                node = m_painted.createNode(object, context);
                m_hasPainted = true;
            } else if (const ObjectRenderer* renderer = m_registry.find(core::kindOf(object))) {
                node = renderer->createNode(object, i, context);
            }
            if (!node) {
                continue;
            }
            node = wrapped(node, object.common);
            if (m_gridOnTop) {
                root->insertChildNodeBefore(node, m_gridOnTop);
            } else {
                root->appendChildNode(node);
            }
        }
        m_builtCount = shown.objects.size();
    }
    return root;
}
```

  Le fond se dessine sur `(0, 0, largeur, hauteur)` de la page, qui est aussi la taille de l'élément (`PageArea.qml`
  la règle sur la page, L2-T16). Le test du lot 1 `emptyPageIsWhite` reste vrai (fond blanc par défaut). Ajouter
  `src/paintedrenderer.cpp` et `include/actcraft/render/paintedrenderer.h` aux `SOURCES` de `render/CMakeLists.txt`.

- [ ] **Étape 6 : vert ciblé.** Commande ciblée, motif `tst_render_` : `100% tests passed` (tous les tests de
  `render/`, ceux du lot 1 compris). Si `sceneMatchesThePainter` dépasse 2,0, comparer les deux images à l'endroit
  fautif (sauver les deux sous `.tmp/`) et corriger la cause (marge du nœud, ordre, anticrénelage), jamais le seuil.

- [ ] **Étape 7 : preuve et banc.** `format.ps1`, preuve de tâche (N ≥ 57 ; `tst_render_painted`,
  `tst_render_fixedpage` Passed), banc sans écran (`content_check.ok = True`).

- [ ] **Étape 8 : commit.**

```powershell
git -C <worktree> add render/include/actcraft/render/paintedrenderer.h render/src/paintedrenderer.cpp render/include/actcraft/render/objectrenderer.h render/include/actcraft/render/fixedpageitem.h render/src/fixedpageitem.cpp render/CMakeLists.txt tests/render.cmake tests/tst_render_painted.cpp tests/tst_render_fixedpage.cpp
git -C <worktree> commit -m "render : moteur peint des nouveaux types, fond et grille de page, transformation et translucidité dans la scène" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t14"></a>
### Tâche L2-T14 — Interface de cache figée (DA11) et page fixe qui lit le document

**Objectif.** DA11 et décision 5 : une **interface de cache figée**, `core::PageCache` (`insert(id)`, `remove(id)`,
`invalidate(rect)`), seule voie des outils et du document vers le cache ; son implémentation du lot 2 est le redessin
actuel (`FixedPageItem` + redessin du `ShaderEffectSource` par la vue), et le lot 10 ne changera **que**
l'implémentation. `applyToCache` traduit les changements de la pile (`core::Changes`, L2-T2) en appels de cache pour
la page affichée. `FixedPageItem` ne possède plus la page affichée : il **lit** `document.page(index)` (relais du lot 1
défait, revue L1-T9 : « page possédée par l'élément »). Il garde, pour le banc et les tests, une page interne
(`setPage`, `appendStroke`, `loadGeneratedPage`, noms figés du prototype).

Règles du redessin (implémentation lot 2) : `insert` d'un objet ajouté **en fin de page** (le cas du stylo) → ajout
d'un nœud sans reconstruction ; tout autre `insert`, `remove`, `invalidate` → reconstruction complète et, si la page
a des images, régénération du magasin d'images ; chaque appel émet `contentChanged` (la vue redessine son cache dans
le même tour de boucle, décision 10).

Patron à **ne pas** reprendre : que l'élément copie l'objet reçu (deux sources de vérité, revue L1-T9) ou que le
document appelle l'élément directement (le lot 10 devrait alors changer le document). Le document ignore le cache ;
le contrôleur (L2-T15) relaie les changements par `applyToCache`.

**Fichiers.**
- Créer : `core/include/actcraft/core/pagecache.h`, `core/src/pagecache.cpp`, `tests/tst_core_pagecache.cpp`
- Modifier : `core/CMakeLists.txt`, `tests/core.cmake`, `render/include/actcraft/render/fixedpageitem.h`,
  `render/src/fixedpageitem.cpp`, `tests/tst_render_fixedpage.cpp`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_core_pagecache|tst_render_fixedpage` → `error`.
Vert : forme bornée, N ≥ 58, `TESTS OK`, `tst_core_pagecache .*Passed`, `tst_render_fixedpage .*Passed`. Banc hors
écran : `content_check.ok = True` (le banc passe par la page interne et `loadGeneratedFirstPage`).

**Interdits.** Aucune copie de la page du document dans l'élément. Aucun appel de l'élément depuis `core/`. Ne
renommer ni `setPage`, ni `appendStroke`, ni `loadGeneratedPage`, ni `strokeAppended`, ni `readyChanged`.

**Terminé quand.** Preuve verte, JSON du banc joint, commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests du routage.** Créer `tests/tst_core_pagecache.cpp` :

```cpp
#include <actcraft/core/pagecache.h>

#include <QStringList>
#include <QTest>

using namespace actcraft::core;

namespace {

class RecordingCache final : public PageCache
{
public:
    QStringList calls;
    void insert(ObjectId id) override { calls << QStringLiteral("insert %1").arg(id); }
    void remove(ObjectId id) override { calls << QStringLiteral("remove %1").arg(id); }
    void invalidate(const RectF& r) override { calls << QStringLiteral("invalidate %1 %2").arg(r.width).arg(r.height); }
};

} // namespace

// DA11 : les changements de la pile n'atteignent le cache que par son interface, pour la seule page affichée.
class TstCorePageCache : public QObject
{
    Q_OBJECT

private slots:
    void onlyTheShownPageReachesTheCache()
    {
        const Changes changes{
            Change{Change::Kind::ObjectInserted, 1, 7, RectF{0, 0, 10, 10}},
            Change{Change::Kind::ObjectInserted, 0, 8, RectF{}},
            Change{Change::Kind::ObjectRemoved, 1, 9, RectF{}},
            Change{Change::Kind::ObjectChanged, 1, 10, RectF{5, 5, 20, 30}},
            Change{Change::Kind::PageChanged, 1, 0, RectF{}},
            Change{Change::Kind::PageInserted, 1, 0, RectF{}},
            Change{Change::Kind::PageRemoved, 1, 0, RectF{}},
        };
        RecordingCache cache;
        applyToCache(changes, 1, cache);
        QCOMPARE(cache.calls, (QStringList{QStringLiteral("insert 7"), QStringLiteral("remove 9"),
                                           QStringLiteral("invalidate 20 30"), QStringLiteral("invalidate 0 0")}));
    }
};

QTEST_GUILESS_MAIN(TstCorePageCache)
#include "tst_core_pagecache.moc"
```

  Ajouter à `tests/core.cmake` :

```cmake
actcraft_add_test(tst_core_pagecache
    SOURCES tst_core_pagecache.cpp
    LIBS ActCraft::core)
```

- [ ] **Étape 2 : écrire les tests de la page fixe.** Dans `tests/tst_render_fixedpage.cpp`, ajouter les includes
  `<actcraft/core/document.h>` et `<actcraft/core/edits.h>`, puis à la classe :

```cpp
    void showsThePageOfItsDocument()
    {
        Fixture f;
        open(f);
        core::Document d;
        d.appendPage();
        core::PageObject stroke = lineAt(300);
        stroke.common.id = d.allocateId();
        stroke.common.layer = core::Layer::Top;
        core::insertOnTopOfLayer(d.page(1), stroke);
        f.page->setSource(&d, 1);
        QCOMPARE(f.page->sourceDocument(), &d);
        QCOMPARE(f.page->sourcePage(), std::size_t{1});
        QCOMPARE(&f.page->page(), &d.page(1)); // lue dans le document, jamais copiée
        QCOMPARE(f.page->objectCount(), 1);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(60, 300), QColor(Qt::black), 40));
    }

    void insertAtTheEndAppendsWithoutRebuilding()
    {
        Fixture f;
        open(f);
        core::Document d;
        f.page->setSource(&d, 0);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        const int rebuilds = f.page->rebuildCount();
        QSignalSpy changed(f.page, &render::FixedPageItem::contentChanged);
        QSignalSpy appended(f.page, &render::FixedPageItem::strokeAppended);
        core::PageObject stroke = lineAt(200);
        stroke.common.id = d.allocateId();
        stroke.common.layer = core::Layer::Top;
        core::insertOnTopOfLayer(d.page(0), stroke);
        f.page->insert(stroke.common.id);
        waitFrame(f);
        QCOMPARE(f.page->rebuildCount(), rebuilds);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(appended.count(), 1);
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(60, 200), QColor(Qt::black), 40));
    }

    void insertBelowAnotherObjectRebuilds()
    {
        Fixture f;
        open(f);
        core::Document d;
        core::PageObject top = lineAt(200);
        top.common.id = d.allocateId();
        top.common.layer = core::Layer::Top;
        core::insertOnTopOfLayer(d.page(0), top);
        f.page->setSource(&d, 0);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        const int rebuilds = f.page->rebuildCount();
        core::PageObject middle = lineAt(400);
        middle.common.id = d.allocateId();
        core::insertOnTopOfLayer(d.page(0), middle); // couche intermédiaire : sous le trait du dessus
        f.page->insert(middle.common.id);
        waitFrame(f);
        QCOMPARE(f.page->rebuildCount(), rebuilds + 1);
        QCOMPARE(f.page->objectCount(), 2);
    }

    void removeAndInvalidateRebuild()
    {
        Fixture f;
        open(f);
        core::Document d;
        core::PageObject stroke = lineAt(200);
        stroke.common.id = d.allocateId();
        core::insertOnTopOfLayer(d.page(0), stroke);
        f.page->setSource(&d, 0);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        const int rebuilds = f.page->rebuildCount();
        d.page(0).objects.clear();
        f.page->remove(stroke.common.id);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        QCOMPARE(f.page->rebuildCount(), rebuilds + 1);
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(60, 200), QColor(Qt::white), 1));
        d.page(0).background = core::Background{};
        d.page(0).background.fill = core::solidFill(0xFF00FF00u);
        f.page->invalidate(core::RectF{});
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        QCOMPARE(f.page->rebuildCount(), rebuilds + 2);
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(500, 400), QColor(Qt::green), 2));
    }

    void appendStrokeIsRefusedWhileShowingADocument()
    {
        Fixture f;
        open(f);
        core::Document d;
        f.page->setSource(&d, 0);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("appendStroke")));
        f.page->appendStroke(lineAt(100));
        QVERIFY(d.page(0).objects.empty());
        QCOMPARE(f.page->objectCount(), 0);
    }

    void detachingFallsBackToAnEmptyInternalPage()
    {
        Fixture f;
        open(f);
        core::Document d;
        core::PageObject stroke = lineAt(200);
        stroke.common.id = d.allocateId();
        core::insertOnTopOfLayer(d.page(0), stroke);
        f.page->setSource(&d, 0);
        QCOMPARE(f.page->objectCount(), 1);
        f.page->setSource(nullptr, 0);
        QVERIFY(f.page->sourceDocument() == nullptr);
        QCOMPARE(f.page->objectCount(), 0);
    }
```

  `lineAt(y)` rend un `core::Stroke` : `core::PageObject stroke = lineAt(300);` passe par le constructeur hérité.
  Ajouter `#include <QRegularExpression>`.

- [ ] **Étape 3 : constater le rouge.** Commande ciblée, motif `tst_core_pagecache|tst_render_fixedpage` → `error`.

- [ ] **Étape 4 : écrire l'interface.** Créer `core/include/actcraft/core/pagecache.h` :

```cpp
#pragma once

#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <cstddef>

namespace actcraft::core {

// Cache du contenu fixe de la page affichée (DA11). Seule voie des outils et du document vers le cache : le lot 10
// ne change que son implémentation (lot 2 : redessin complet, FixedPageItem + ShaderEffectSource de la vue).
class PageCache
{
public:
    virtual ~PageCache() = default;
    // L'objet `id` est déjà dans la page affichée du document.
    virtual void insert(ObjectId id) = 0;
    // L'objet `id` n'est plus dans la page affichée.
    virtual void remove(ObjectId id) = 0;
    // Zone à redessiner en coordonnées de page ; rectangle vide = toute la page.
    virtual void invalidate(const RectF& pageRect) = 0;
};

// Envoie au cache les changements de la page `shownPage`, dans l'ordre. Les ajouts et retraits de pages ne le
// concernent pas : le contrôleur change alors de page affichée.
void applyToCache(const Changes& changes, std::size_t shownPage, PageCache& cache);

} // namespace actcraft::core
```

  Créer `core/src/pagecache.cpp` :

```cpp
#include <actcraft/core/pagecache.h>

namespace actcraft::core {

void applyToCache(const Changes& changes, std::size_t shownPage, PageCache& cache)
{
    for (const Change& c : changes) {
        if (c.page != shownPage) {
            continue;
        }
        switch (c.kind) {
        case Change::Kind::ObjectInserted:
            cache.insert(c.object);
            break;
        case Change::Kind::ObjectRemoved:
            cache.remove(c.object);
            break;
        case Change::Kind::ObjectChanged:
            cache.invalidate(c.area);
            break;
        case Change::Kind::PageChanged:
            cache.invalidate(RectF{});
            break;
        case Change::Kind::PageInserted:
        case Change::Kind::PageRemoved:
            break;
        }
    }
}

} // namespace actcraft::core
```

  Ajouter `src/pagecache.cpp` aux `SOURCES` de `core/CMakeLists.txt`.

- [ ] **Étape 5 : la page fixe lit le document.** Dans `render/include/actcraft/render/fixedpageitem.h` : includes
  `<actcraft/core/document.h>`, `<actcraft/core/pagecache.h>`, `<atomic>` ; la classe devient
  `class FixedPageItem : public QQuickItem, public core::PageCache` ; remplacer le commentaire de tête par :

```cpp
// Contenu fixe de la page affichée : modèle → nœuds par les moteurs (décision 10). La page est LUE dans le document
// (setSource), jamais copiée ; ses changements arrivent par l'interface de cache (DA11). Pour le banc et les tests,
// une page interne (setPage, appendStroke, loadGeneratedPage). Le cache (ShaderEffectSource, PageArea.qml) prend
// cet élément pour source.
```

  et ajouter à la section publique :

```cpp
    // Page `pageIndex` de `document` : le document doit survivre à l'élément, ou appeler setSource(nullptr, 0)
    // avant de mourir. nullptr : page interne vide.
    void setSource(const core::Document* document, std::size_t pageIndex);
    const core::Document* sourceDocument() const noexcept;
    std::size_t sourcePage() const noexcept;
    // core::PageCache (DA11), implémentation du lot 2 : ajout en fin de page sans reconstruction, sinon
    // reconstruction complète. Chaque appel émet contentChanged.
    void insert(core::ObjectId id) override;
    void remove(core::ObjectId id) override;
    void invalidate(const core::RectF& pageRect) override;
    // Crochet de test : nombre de reconstructions complètes de la scène.
    int rebuildCount() const noexcept;
```

  avec les commentaires de `setPage` (« Banc et tests : la page devient celle de la page interne ; la source est
  oubliée ») et d'`appendStroke` (« Page interne seulement : refusé, avec un avertissement, quand l'élément montre un
  document. Le trait va au sommet de la couche supérieure. »). Signal ajouté : `void contentChanged();`. Membres
  privés : supprimer `core::Page m_page;`, ajouter

```cpp
    const core::Document& document() const noexcept; // source, ou page interne
    void restart();                                  // reconstruction complète et images régénérées

    const core::Document* m_source = nullptr;
    std::size_t m_sourcePage = 0;
    core::Document m_own; // page interne (une page)
    std::atomic<int> m_rebuildCount{0};
```

  Dans `render/src/fixedpageitem.cpp`, include `<actcraft/core/edits.h>` et `<QDebug>`, puis remplacer
  `loadGeneratedPage`, `setPage`, `page`, `appendStroke`, `objectCount` et ajouter les nouvelles méthodes :

```cpp
void FixedPageItem::loadGeneratedPage()
{
    setPage(core::generatePage(core::kDefaultSeed));
}

const core::Document& FixedPageItem::document() const noexcept
{
    return m_source ? *m_source : m_own;
}

const core::Page& FixedPageItem::page() const noexcept
{
    return m_source ? m_source->page(m_sourcePage) : m_own.page(0);
}

void FixedPageItem::restart()
{
    m_rebuild = true;
    if (m_ready) {
        m_ready = false;
        emit readyChanged();
    }
    m_images.generate(page(), &document());
    emit objectCountChanged();
    emit contentChanged();
    update();
}

void FixedPageItem::setPage(core::Page page)
{
    m_source = nullptr;
    m_sourcePage = 0;
    m_own = core::Document(std::vector<core::Page>{std::move(page)});
    restart();
}

void FixedPageItem::setSource(const core::Document* document, std::size_t pageIndex)
{
    m_source = document;
    m_sourcePage = document ? pageIndex : 0;
    if (!document) {
        m_own = core::Document();
    }
    restart();
}

const core::Document* FixedPageItem::sourceDocument() const noexcept
{
    return m_source;
}

std::size_t FixedPageItem::sourcePage() const noexcept
{
    return m_sourcePage;
}

void FixedPageItem::appendStroke(core::Stroke stroke)
{
    if (m_source) {
        qWarning("FixedPageItem::appendStroke : la page affichée appartient à un document ; passer par sa pile");
        return;
    }
    core::PageObject object = std::move(stroke);
    object.common.id = m_own.allocateId();
    object.common.layer = core::Layer::Top;
    const core::ObjectId id = object.common.id;
    core::insertOnTopOfLayer(m_own.page(0), std::move(object));
    insert(id);
}

void FixedPageItem::insert(core::ObjectId id)
{
    const core::Page& shown = page();
    const std::size_t index = core::indexOf(shown, id);
    if (index == shown.objects.size()) {
        return;
    }
    const core::PageObject& object = shown.objects[index];
    const bool needsImages = std::get_if<core::ImageObject>(&object) || std::get_if<core::Media>(&object);
    if (index + 1 == shown.objects.size() && index >= m_builtCount && !needsImages) {
        // Fin de page (le stylo) : le prochain updatePaintNode n'ajoute que ce nœud.
        emit objectCountChanged();
        if (std::get_if<core::Stroke>(&object)) {
            emit strokeAppended();
        }
        emit contentChanged();
        update();
        return;
    }
    restart();
}

void FixedPageItem::remove(core::ObjectId)
{
    restart();
}

void FixedPageItem::invalidate(const core::RectF&)
{
    restart();
}

int FixedPageItem::rebuildCount() const noexcept
{
    return m_rebuildCount.load();
}

int FixedPageItem::objectCount() const noexcept
{
    return static_cast<int>(page().objects.size());
}
```

  Dans `updatePaintNode` (L2-T13) : `const core::Page& shown = m_page;` devient `const core::Page& shown = page();`
  et la branche `if (m_rebuild || !old)` commence par `++m_rebuildCount;`. Dans `setRasterScale` (L2-T13),
  `pagePaintsSomething(m_page)` devient `pagePaintsSomething(page())`. Dans `onImagesReady` et `geometryChange`, rien ne
  change. `restart()` régénère les images à chaque appel (le magasin ne fait rien de coûteux sur une page sans image) :
  c'est le « redessin actuel » que le lot 10 remplacera.

  Le test du lot 1 `strokeAppendedAfterFirstFrameKeepsTransform` et `appendedStrokeIsDrawnOnTop` passent par
  `appendStroke` : ils restent verts (ajout en fin de page sans reconstruction).

- [ ] **Étape 6 : vert ciblé.** Commande ciblée, motif `tst_core_pagecache|tst_render_` : `100% tests passed`.
  `tst_pageview`, `tst_ui_document`, `tst_ui_drawing` et `tst_ui_shell` doivent rester verts sans changement
  (le contrôleur du lot 1 passe encore par `setPage` et le stylo par `appendStroke` sur la page interne… **sauf** que
  `DocumentController::setFixedPage` du lot 1 appelle `setPage` : la page interne reste le relais jusqu'à L2-T15).
  Lancer la suite ciblée `tst_ui_` pour le vérifier.

- [ ] **Étape 7 : preuve et banc.** `format.ps1`, preuve de tâche (N ≥ 58 ; `tst_core_pagecache` et
  `tst_render_fixedpage` Passed), banc sans écran (`content_check.ok = True`).

- [ ] **Étape 8 : commit.**

```powershell
git -C <worktree> add core/include/actcraft/core/pagecache.h core/src/pagecache.cpp core/CMakeLists.txt tests/core.cmake tests/tst_core_pagecache.cpp render/include/actcraft/render/fixedpageitem.h render/src/fixedpageitem.cpp tests/tst_render_fixedpage.cpp
git -C <worktree> commit -m "Interface de cache figée (DA11) ; la page fixe lit la page du document au lieu de la posséder" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

