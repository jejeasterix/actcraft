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
| L2-T21 | S4 : miniatures comparées aux `.thm`, scénarios `ouvrir-flipchart-synthetique` (CI) et `local/ouvrir-flipchart` | L2-T10, L2-T19, L2-T20 | `bench/flipchart/main.cpp`, `bench/CMakeLists.txt`, `bench/src/fixtures.cpp`, `bench/src/uiscenario.cpp`, `bench/scenarios/ouvrir-flipchart-synthetique.json`, `bench/scenarios/local/ouvrir-flipchart.json`, `scripts/captures.ps1`, `tests/bench.cmake` | isolé | concepteur-critique | TESTS OK, ≥ 66 tests, `tst_flipchart_miniatures_synthetique` et `tst_captures_ouvrir_flipchart_synthetique` Passed ; localement `MINIATURES OK : 124 pages, médiane ≤ 2,0, ≤ 3 au-dessus de 7` et `captures.ps1 -Scenario local/ouvrir-flipchart -Ouvrir "<samples>\<paperboard réel n° 16>"` → `CAPTURES OK` | S4 | #tache-l2-t21 |
| L2-T22 | Android : Ouvrir et Enregistrer par `content://` sur l'émulateur, sinon commandes de fichier retirées de l'APK | L2-T18 | `scripts/android-commun.ps1`, `scripts/essai-android.ps1`, `scripts/essai-android-fichiers.ps1`, `actions/data/commands.json`, `docs/verifications/2026-10-15-android-fichiers.md` | isolé | concepteur-critique | `ANDROID FICHIERS OK` (empreinte enregistrée = empreinte relue, logcat) ; sinon repli : `APK OK` + capture du menu Fichier de l'APK sans « Ouvrir… » ni « Enregistrer » | S5 | #tache-l2-t22 |
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
9. **Dialogues non natifs pas encore forcés** (décision 22, « moyen exact relevé au lot 2 ») : relevé —
   `QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs)` dans `actcraft-captures` — mais pas appliqué, aucun
   scénario du lot n'ouvrant de dialogue (H2b-6).
10. **S4 : pages au-dessus de 7 nommées** (L2-T21) : la liste des trois pages connues est passée à l'outil en local
    (`--connues`), pas écrite dans le code (dépôt public).

## Couverture des scénarios

| Scénario | Tâches | Joué à la clôture |
|---|---|---|
| S4 — ouvrir un `.flipchart` | L2-T11 (peintre), L2-T12 (images), L2-T13 (scène), L2-T16 (taille de page), L2-T17 (ouverture), L2-T18 (avertissements), L2-T19 (ligne de commande), L2-T21 (miniatures, scénarios), L2-T23 (fluidité au changement de page) ; 2a : L2-T7 à L2-T10 | oui : oracle local (17 paperboards, 124 pages, 0 écart), `MINIATURES OK` local, `captures.ps1 -Scenario local/ouvrir-flipchart -Rendu d3d11` à l'écran, `actcraft.exe "<samples>\<paperboard réel n° 16>"` |
| S5 — aller-retour natif | L2-T14, L2-T15 (document, pile, cache), L2-T17 (fichiers), L2-T18 (commandes), L2-T20 (scénario), L2-T22 (Android) ; 2a : L2-T1 à L2-T6 | oui : `captures.ps1 -Scenario aller-retour-natif -Rendu d3d11` à l'écran ; Android : `ANDROID FICHIERS OK` (ou repli constaté) |

## Vagues proposées (parallélisme 2 au plus)

V6 : L2-T11 ‖ L2-T12 · V7 : L2-T13 · V8 : L2-T14 · V9 : L2-T15 · V10 : L2-T16 ‖ L2-T17 · V11 : L2-T18 ‖ L2-T23 ·
V12 : L2-T19 ‖ L2-T20 · V13 : L2-T21 ‖ L2-T22.

Deux tâches d'une même vague ne partagent aucun fichier (vérifié sur la colonne `fichiers touchés`). Fichiers
partagés tenus par les dépendances : `render/src/fixedpageitem.cpp` et `tests/tst_render_fixedpage.cpp` (L2-T13 →
L2-T14 → L2-T23) ; `render/CMakeLists.txt` et `tests/render.cmake` (L2-T11 → L2-T13) ; `ui/src/pageview.cpp`,
`tests/tst_pageview.cpp` et `ui/qml/PageArea.qml` (L2-T15 → L2-T16) ; `ui/src/documentcontroller.cpp` (L2-T15 →
L2-T17 → L2-T23) ; `ui/CMakeLists.txt` et `tests/ui.cmake` (L2-T17 → L2-T18) ; `ui/qml/Main.qml` (L2-T15 → L2-T18) ;
`bench/CMakeLists.txt`, `bench/src/fixtures.cpp`, `bench/src/uiscenario.cpp` et `tests/bench.cmake` (L2-T20 → L2-T21) ;
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
  `ImageStore::resourceImage`, `ImageStore::objectImage`, `ImageStore::variantKey`, `ImageStore::covers`.
- `actcraft::render` (L2-T13) : `PaintedRenderer` (`createNode`), `usesPaintedRenderer`, `rasterizeLocal`,
  `createBackgroundNode`, `createGridNode`, `RenderContext::rasterScale`, `FixedPageItem::setRasterScale`,
  `FixedPageItem::rasterScale`, `FixedPageItem::rebuildCount` (crochet de test).
- `actcraft::core` (L2-T14) : `PageCache` (`insert`, `remove`, `invalidate`), `applyToCache`.
- `actcraft::render` (L2-T14) : `FixedPageItem` (dérive aussi de `core::PageCache`) : `setSource`, `sourceDocument`,
  `sourcePage`, signal `contentChanged` ; `setPage`, `page`, `appendStroke`,
  `loadGeneratedPage`, `isReady`, `objectCount` gardés.
- `actcraft::ui` (L2-T15) : `DocumentController` — propriétés `pageSize`, `modified`, `title`, `fileUrl`,
  `fingerprint`, `canUndo`, `canRedo` ; méthodes `document() const`, `execute`, `undo`, `redo`, `commitStroke`,
  `replaceDocument`, `newDocument`, `loadGeneratedPages` (L2-T23) ; signaux `modifiedChanged`, `titleChanged`,
  `fileUrlChanged`, `documentChanged`, `undoChanged` (et ceux du lot 1). `PageView::documentController` ;
  `ControllerSink`. Titre d'un document neuf : « Sans titre ».
- `actcraft::ui` (L2-T17) : `LoadOutcome`, `loadDocument`, `SaveOutcome`, `saveDocument`, `titleFromUrl`,
  `defaultDocumentsFolder` ; `DocumentController` : `openUrl`, `saveToUrl`, `busy`, `warnings`, `lastError`,
  `suggestedFileName`, `lastIoOffMainThread` (signal `lastIoChanged`), `urlFromArgument`, `defaultFolderUrl`, `LoadOutcome::offMainThread`,
  `SaveOutcome::offMainThread`, signaux `loadFinished(bool, QString)`,
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
- Banc (L2-T23) : scénario `page`, `InputKind::Command`, `PlannedInput::command`, `PlannedInput::argument`,
  `pageScenario`, `DocumentController::loadGeneratedPages(int, bool painted)`, `content_check.page_flash`.
- `PageView::onStrokeFrozen` (L2-T15) n'émet plus que `strokeFrozen` : le redessin du cache vient de
  `FixedPageItem::contentChanged`, une fois par trait.

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

    void imageResourceWithKeyColor()
    {
        QImage source(4, 4, QImage::Format_ARGB32);
        source.fill(Qt::white);
        source.setPixelColor(0, 0, Qt::green);
        render::DecodedImages images;
        images.insert("cle", source);
        core::ImageObject im;
        im.bounds = {0, 0, 4, 4};
        im.resource = "cle";
        im.hasKeyColor = true;
        im.keyColor = 0xFFFFFFFFu;
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
        core::TextSpan run;
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

    void pictureBackgroundCoversThePage()
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
        level.spacingX = 100;
        level.spacingY = 100;
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
        level.spacingX = 100;
        level.spacingY = 100;
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
        // Comme les miniatures (L2-T17) : formes ET texte (polices, QTextDocument) peints sur un fil secondaire.
        core::Page page;
        page.objects.push_back(square(0, 0, 100, 0xFF00FF00u));
        core::Text t;
        t.bounds = {0, 0, 300, 60};
        t.html = "<p style=\"color:#ff0000; font-size:40px\">XXXX</p>";
        core::PageObject text = t;
        text.common.transform = core::translation(200, 200);
        page.objects.push_back(text);
        QThread* worker = nullptr;
        const QImage image = QtConcurrent::run([&page, &worker] {
                                 worker = QThread::currentThread();
                                 return render::renderPage(page, kNoImages, 1.0);
                             }).result();
        QVERIFY(worker != QThread::currentThread());
        QVERIFY(near(image.pixelColor(20, 20), Qt::green, 2));
        int red = 0;
        for (int y = 200; y < 260; ++y) {
            for (int x = 200; x < 500; ++x) {
                red += image.pixelColor(x, y).red() > 200 && image.pixelColor(x, y).green() < 80 ? 1 : 0;
            }
        }
        QVERIFY2(red > 200, qPrintable(QString::number(red)));
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
// Page entière en coordonnées de page : fond, grille dessous (ou dessus si drawnAbove), objets dans l'ordre du vecteur.
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
QImage withKeyColor(const QImage& source, core::Argb transparent)
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
    if (im.hasKeyColor) {
        image = withKeyColor(image, im.keyColor);
    }
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.drawImage(rect, image);
}

void paintText(QPainter& p, const core::Text& t)
{
    if (!t.runs.empty()) {
        for (const core::TextSpan& run : t.runs) {
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
    const double sx = std::max(l.spacingX * scale, 1.0);
    const double sy = std::max(l.spacingY * scale, 1.0);
    std::vector<QLineF> out;
    if (l.pattern == core::GridPattern::Normal) {
        const double x0 = std::max(0.0, l.start.x);
        const double y0 = std::max(0.0, l.start.y);
        if (l.linesAlongX) {
            const Range r = rangeOf(l.start.x, sx, x0, w);
            for (long long k = r.first; k <= r.last; ++k) {
                const double x = l.start.x + static_cast<double>(k) * sx;
                out.emplace_back(x, y0, x, h);
            }
        }
        if (l.linesAlongY) {
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
    } families[2] = {{1, l.linesAlongX, sx}, {-1, l.linesAlongY, sy}};
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
    const double sx = std::max(l.spacingX * scale, 1.0);
    const double sy = std::max(l.spacingY * scale, 1.0);
    std::vector<QPointF> out;
    if (l.pattern == core::GridPattern::Normal) {
        if (!l.linesAlongX || !l.linesAlongY) {
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
        p.drawLine(QPointF(c->firstEnd.x, c->firstEnd.y), QPointF(c->secondEnd.x, c->secondEnd.y));
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
    const double scale = thumbnail ? page.grid.previewZoom : page.grid.baseZoom;
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
    if (options.grid && !page.grid.drawnAbove) {
        paintGrid(p, page, options.thumbnail);
    }
    for (const core::PageObject& object : page.objects) {
        paintObject(p, object, images);
    }
    if (options.grid && page.grid.drawnAbove) {
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
  `pen.setDashPattern({1, 4})` prend des `qreal` ; la boucle de `withKeyColor` lit `scanLine` en `QRgb`
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
        masked.hasKeyColor = true;
        masked.keyColor = 0xFFFFFFFFu;
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
    // Vrai si les images de la dernière génération suffisent à `page` (mêmes images procédurales aux mêmes indices,
    // toutes les ressources voulues décodées) : la page fixe se reconstruit alors sans redécoder (L2-T14).
    bool covers(const core::Page& page, const core::Document* resources) const;
```

  et dans la section privée, remplacer la signature de `onImageReady` par :

```cpp
    void onImageReady(std::uint64_t generation, std::size_t index, std::string key, const QImage& image,
                      bool offOwnerThread);
```

  et ajouter les membres `std::unordered_map<std::string, QImage> m_resources;` et
  `std::unordered_map<std::size_t, core::ImageObject> m_procedural;` (sources des images procédurales de la
  dernière génération, par indice ; remplis dans `generate`). `imageCount()` continue de compter les seules images
  procédurales.

- [ ] **Étape 4 : écrire l'implémentation.** Dans `render/src/imagestore.cpp`, ajouter les includes `<set>` et
  `<utility>`, une fonction locale et remplacer `generate` et `onImageReady` :

```cpp
namespace {

// Pixels de la couleur `transparent` (alpha ignoré) rendus transparents (même règle que pagepainter.cpp).
QImage withKeyColor(const QImage& source, core::Argb transparent)
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
    if (!object.hasKeyColor) {
        return object.resource;
    }
    return object.resource + QStringLiteral("#%1").arg(object.keyColor, 8, 16, QLatin1Char('0')).toStdString();
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
                want(object->resource, object->hasKeyColor ? std::optional<core::Argb>(object->keyColor)
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
                    probe.hasKeyColor = true;
                    probe.keyColor = *color;
                    out.emplace_back(variantKey(probe), withKeyColor(decoded, *color));
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

bool ImageStore::covers(const core::Page& page, const core::Document* resources) const
{
    if (!m_ready) {
        return false;
    }
    QMutexLocker lock(&m_mutex);
    // Ressource absente du document : rien à décoder (image nulle attendue). Présente : décodée ou non.
    const auto has = [this, resources](const std::string& key) {
        return key.empty() || !resources || !resources->resource(key) || m_resources.count(key) > 0;
    };
    // Variante à couleur transparente : sa clé n'est pas une ressource du document ; on regarde la ressource de base.
    const auto hasVariant = [this, resources](const core::ImageObject& im) {
        return !resources || !resources->resource(im.resource) || m_resources.count(variantKey(im)) > 0;
    };
    std::size_t procedural = 0;
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        if (const auto* im = std::get_if<core::ImageObject>(&page.objects[i])) {
            if (im->resource.empty()) {
                ++procedural;
                const auto it = m_procedural.find(i);
                if (it == m_procedural.end() || it->second.pattern != im->pattern || it->second.seed != im->seed ||
                    it->second.colorA != im->colorA || it->second.colorB != im->colorB ||
                    it->second.sourceWidth != im->sourceWidth || it->second.sourceHeight != im->sourceHeight) {
                    return false;
                }
            } else if (!has(im->resource) || !hasVariant(*im)) {
                return false;
            }
        } else if (const auto* m = std::get_if<core::Media>(&page.objects[i]); m && !has(m->placeholder)) {
            return false;
        }
    }
    return procedural == m_procedural.size() && has(page.background.image);
}
```

  Dans `generate`, `m_procedural.clear()` avec `m_images.clear()`, et `m_procedural[i] = *object;` pour chaque image
  procédurale. Ajouter le test :

```cpp
    void coversTellsWhetherAPageNeedsDecoding()
    {
        core::Page page = core::generatePage();
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(page);
        QVERIFY(ready.wait(20000));
        QVERIFY(store.covers(page, nullptr));
        core::Stroke stroke;
        stroke.points = {{1, 1, 0.5f}};
        page.objects.emplace_back(stroke); // trait ajouté en fin : les images gardent leurs indices
        QVERIFY(store.covers(page, nullptr));
        page.objects.insert(page.objects.begin(), core::PageObject(stroke)); // indices décalés
        QVERIFY(!store.covers(page, nullptr));
    }

    void coversSeesAMissingTransparentVariant()
    {
        QImage source(4, 4, QImage::Format_ARGB32);
        source.fill(Qt::white);
        core::Document document;
        core::ImageObject plain;
        plain.resource = document.addResource(pngOf(source), "png");
        core::ImageObject masked = plain;
        masked.hasKeyColor = true;
        masked.keyColor = 0xFFFFFFFFu;
        core::Page withPlain;
        withPlain.objects.emplace_back(plain);
        core::Page withMasked;
        withMasked.objects.emplace_back(masked);
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(withPlain, &document);
        QVERIFY(ready.wait(5000));
        QVERIFY(store.covers(withPlain, &document));
        QVERIFY(!store.covers(withMasked, &document)); // même ressource, variante jamais calculée
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
  si `drawnAbove`) ;
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

    void gridAtTopStaysAboveObjects()
    {
        core::Page page;
        core::PageObject red = pathSquare(300, 0xFFFF0000u);
        page.objects.push_back(red);
        page.grid.visible = true;
        page.grid.drawnAbove = true;
        core::GridLevel level;
        level.spacingX = 100;
        level.spacingY = 100;
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

    void rasterScaleNeverRebuildsInSoftware()
    {
        core::Page page;
        page.objects.push_back(redText()); // objet peint : en RHI, une nouvelle échelle le rastériserait
        Fixture f;
        QVERIFY(open(f, page));
        grab(f);
        const int before = f.page->rebuildCount();
        f.page->setRasterScale(2.0);
        QCOMPARE(f.page->rasterScale(), 2.0);
        grab(f);
        QCOMPARE(f.page->rebuildCount(), before); // logiciel : nœuds vectoriels, rien à rastériser
        f.page->setRasterScale(2.0);              // même échelle : rien
        grab(f);
        QCOMPARE(f.page->rebuildCount(), before);
        // Rouge vu par mutation : retirer le test « !software » de setRasterScale → rebuildCount + 1.
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

DecodedImages pictureBackgroundOf(const core::Page& page, const ImageStore* store)
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
    DecodedImages images = pictureBackgroundOf(page, context.images);
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

  et membres privés `PaintedRenderer m_painted;`, `double m_rasterScale = 1;`, `QSGNode* m_gridDrawnAbove = nullptr;`,
  `bool m_hasPainted = false;`, `std::atomic<int> m_rebuildCount{0};` (include `<atomic>`), avec la méthode publique
  `int rebuildCount() const noexcept; // crochet de test : reconstructions complètes de la scène` (rend
  `m_rebuildCount.load()`). Le chemin RHI (rastérisation dans `updatePaintNode`, reconstruction à chaque zoom posé
  d'une page qui peint) ne se teste pas en ctest (rendu logiciel) : il est **mesuré** par le scénario de banc `page`
  (page à objets peints, zooms posés, L2-T23) en D3D11 à la clôture.

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
        ++m_rebuildCount;
        clearChildren(root);
        m_gridDrawnAbove = nullptr;
        if (m_ready) {
            root->appendChildNode(createBackgroundNode(shown, context));
            if (QSGNode* grid = createGridNode(shown, context)) {
                if (shown.grid.drawnAbove) {
                    m_gridDrawnAbove = grid;
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
            if (m_gridDrawnAbove) {
                root->insertChildNodeBefore(node, m_gridDrawnAbove);
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
        core::insertAtTopOfLayer(d.page(1), stroke);
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
        core::insertAtTopOfLayer(d.page(0), stroke);
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
        core::insertAtTopOfLayer(d.page(0), top);
        f.page->setSource(&d, 0);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        const int rebuilds = f.page->rebuildCount();
        core::PageObject middle = lineAt(400);
        middle.common.id = d.allocateId();
        core::insertAtTopOfLayer(d.page(0), middle); // couche intermédiaire : sous le trait du dessus
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
        core::insertAtTopOfLayer(d.page(0), stroke);
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
        core::insertAtTopOfLayer(d.page(0), stroke);
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
    // Redécoder seulement si le magasin ne couvre plus la page (annuler un trait ne redécode rien).
    if (!m_images.covers(page(), &document())) {
        if (m_ready) {
            m_ready = false;
            emit readyChanged();
        }
        m_images.generate(page(), &document());
    }
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
    core::insertAtTopOfLayer(m_own.page(0), std::move(object));
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

int FixedPageItem::objectCount() const noexcept
{
    return static_cast<int>(page().objects.size());
}
```

  Dans `updatePaintNode` (L2-T13) : `const core::Page& shown = m_page;` devient `const core::Page& shown = page();`
  (le compteur `rebuildCount` est posé par L2-T13). Dans `setRasterScale` (L2-T13),
  `pagePaintsSomething(m_page)` devient `pagePaintsSomething(page())`. Dans `onImagesReady` et `geometryChange`, rien ne
  change. `restart()` ne régénère les images que si le magasin ne couvre plus la page (`ImageStore::covers`,
  L2-T12) : annuler ou rétablir un trait ne redécode rien. La reconstruction complète des nœuds reste le « redessin
  actuel » que le lot cache (qui suit le lot 2) remplacera. Ajouter le test `undoingAStrokeDoesNotRedecode` : page
  générée (30 images) montrée par un document, trait ajouté puis retiré (`remove`) → `isReady()` reste vrai pendant
  tout l'aller-retour (aucun `readyChanged` à faux, `QSignalSpy`).

  Le test du lot 1 `strokeAppendedAfterFirstFrameKeepsTransform` et `appendedStrokeIsDrawnAtTop` passent par
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


<a id="tache-l2-t15"></a>
### Tâche L2-T15 — `ui` : `DocumentController` sur le document et sa pile (relais du lot 1 défait)

**Objectif.** Revue L1-T9 (« Pour le plan du lot 2 ») : défaire le relais. Le contrôleur **possède** le
`core::Document` et sa `core::UndoStack` ; la page fixe **lit** la page affichée (`setSource`, L2-T14) ; plus de
`syncFromView`, plus de page possédée par l'élément, `document()` devient constant ; le stylo verse son trait au
contrôleur (`ControllerSink`), qui l'ajoute par une commande annulable (`AddObjectCommand`, couche supérieure, F-510) ;
« page suivante » sur la dernière page ajoute la page par une commande annulable (`AddPageCommand`) ; chaque commande
rend ses changements, envoyés au cache par `applyToCache` (DA11). Le contrôleur expose aussi l'état que l'interface
et S5 lisent : taille de la page affichée, modifié (pile non propre), titre (« Sans titre », F-656), URL du fichier,
empreinte (hexadécimal), annuler / rétablir possibles. Le test `documentFollowsEveryWriterOfTheShownPage` est
réécrit : il n'y a plus qu'un écrivain, la pile (`everyWriteGoesThroughTheUndoStack`).

Après une annulation ou un rétablissement qui ajoute ou retire des pages, la page affichée reste valide : ajout
avant elle → indice + 1 ; retrait avant elle → indice − 1 ; retrait de la page affichée → même indice borné au
nombre de pages. Le trait en cours se termine avant tout changement de page (`pageAboutToChange`, lot 1, inchangé).

**Fichiers.**
- Modifier : `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`,
  `ui/src/penadapters.h`, `ui/src/penadapters.cpp`, `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`,
  `ui/qml/Main.qml`, `ui/qml/PageArea.qml`, `tests/tst_ui_document.cpp`, `tests/tst_pageview.cpp`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_ui_document` → `error` (`commitStroke`
inconnu). Vert : forme bornée, N ≥ 58, `TESTS OK`, `tst_ui_document`, `tst_pageview`, `tst_ui_shell`,
`tst_ui_drawing`, `tst_captures_premiers_pas` Passed. Banc hors écran : `content_check.ok = True`. Preuve visuelle :
`captures.ps1 -Scenario premiers-pas -Rendu logiciel -HorsEcran -Lire "pageIndicator.text=1 / 2"` →
`LU pageIndicator.text=1 / 2 OK`, `CAPTURES OK` (le trait rouge survit à l'aller-retour de page par le document).

**Interdits.** Ne renommer aucun nom figé du lot 1 (`fixedPage`, `pageCount`, `currentIndex`, `pageLabel`,
`nextPage`, `pageBack`, `loadGeneratedFirstPage`, `document`, `pageAboutToChange`, `pagesChanged`). Aucune
écriture du document hors de la pile, sauf `replaceDocument` (ouverture, nouveau document, page générée du banc), qui
vide la pile. Pas d'Annuler / Rétablir dans l'interface (lot 3). Tests existants : seuls ceux qui supposaient le relais
(écrire dans l'élément et le lire dans le document) changent, pour cette raison écrite.

**Terminé quand.** Preuve verte, capture décrite, JSON du banc joint, commit fait.

**Étapes.**

- [ ] **Étape 1 : réécrire les tests du contrôleur.** Remplacer les fonctions de `tests/tst_ui_document.cpp` qui
  supposent le relais (`strokeSurvivesPageRoundTrip`, `documentReadsTheShownPage`,
  `documentFollowsEveryWriterOfTheShownPage`, `newPageTakesTheShownPageSize`, `swappingTheItemHandsBackItsPage`,
  `generatedFirstPageWhileShowingPageTwo`) par les suivantes ; garder `startsWithOneBlankPageShown`,
  `nextOnLastPageAddsABlankPage`, `previousOnFirstPageDoesNothing`, `pagesChangedOncePerMoveWithTheNewPageSettled`,
  `noPagesChangedWithoutAMove`, `pageAboutToChangeComesBeforeTheCopy` (renommée
  `pageAboutToChangeComesBeforeTheMove`, même contenu, le mot « copie » n'a plus de sens), `generatedFirstPageForTheBench`
  et `registeredInQml` en remplaçant tout `fixed.appendStroke(redStroke())` par `controller.commitStroke(redStroke())` :

```cpp
    void strokeLandsOnTheTopLayerOfTheShownPage()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        const core::Page& page = controller.document().page(0);
        QCOMPARE(page.objects.size(), std::size_t{1});
        QCOMPARE(page.objects[0].common.layer, core::Layer::Top);
        QVERIFY(page.objects[0].common.id != 0);
        QCOMPARE(fixed.objectCount(), 1);
        QCOMPARE(&fixed.page(), &page); // l'élément lit le document
        QVERIFY(controller.isModified());
    }

    void everyWriteGoesThroughTheUndoStack()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QSignalSpy modified(&controller, &ui::DocumentController::modifiedChanged);
        controller.commitStroke(redStroke());
        QVERIFY(controller.canUndo());
        controller.undo();
        QVERIFY(controller.document().page(0).objects.empty());
        QCOMPARE(fixed.objectCount(), 0);
        QVERIFY(!controller.isModified());
        controller.redo();
        QCOMPARE(fixed.objectCount(), 1);
        QVERIFY(controller.isModified());
        QVERIFY(modified.count() >= 3);
    }

    void strokeSurvivesPageRoundTrip()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        controller.nextPage();
        QCOMPARE(fixed.objectCount(), 0);
        controller.pageBack();
        QCOMPARE(fixed.objectCount(), 1);
        QVERIFY(std::get<core::Stroke>(controller.document().page(0).objects[0]).color == 0xFFE53935u);
        QVERIFY(controller.document().page(1).objects.empty());
    }

    void addedPageIsUndoneAndTheShownPageStaysValid()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.nextPage(); // page 2 ajoutée par la pile, affichée
        QCOMPARE(controller.pageLabel(), QStringLiteral("2 / 2"));
        controller.undo();
        QCOMPARE(controller.pageCount(), 1);
        QCOMPARE(controller.currentIndex(), 0);
        QCOMPARE(fixed.sourcePage(), std::size_t{0});
        controller.redo();
        QCOMPARE(controller.pageCount(), 2);
        QCOMPARE(controller.pageLabel(), QStringLiteral("1 / 2"));
    }

    void newPageTakesTheLastPageSize()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.newDocument(800, 600);
        controller.nextPage();
        QCOMPARE(controller.document().page(1).width, 800.0);
        QCOMPARE(controller.pageSize(), QSizeF(800, 600));
    }

    void newAndReplacedDocumentsAreCleanAndShownFromPageOne()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QCOMPARE(controller.title(), QStringLiteral("Sans titre"));
        controller.commitStroke(redStroke());
        core::Document other(std::vector<core::Page>(3));
        other.setTitle("Autre");
        QSignalSpy pages(&controller, &ui::DocumentController::pagesChanged);
        controller.replaceDocument(std::move(other), QUrl::fromLocalFile(QStringLiteral("C:/x/autre.actcraft")),
                                   QStringLiteral("autre"));
        QCOMPARE(controller.pageLabel(), QStringLiteral("1 / 3"));
        QCOMPARE(controller.title(), QStringLiteral("autre"));
        QVERIFY(!controller.isModified());
        QVERIFY(!controller.canUndo());
        QCOMPARE(controller.fileUrl().fileName(), QStringLiteral("autre.actcraft"));
        QCOMPARE(pages.count(), 1);
        controller.newDocument(1280, 1024);
        QCOMPARE(controller.pageSize(), QSizeF(1280, 1024));
        QCOMPARE(controller.title(), QStringLiteral("Sans titre"));
        QVERIFY(controller.fileUrl().isEmpty());
    }

    void fingerprintPropertyFollowsTheDocument()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        const QString empty = controller.property("fingerprint").toString();
        QCOMPARE(empty.size(), 16);
        QSignalSpy changed(&controller, &ui::DocumentController::documentChanged);
        controller.commitStroke(redStroke());
        QCOMPARE(changed.count(), 1);
        QVERIFY(controller.property("fingerprint").toString() != empty);
        QCOMPARE(controller.property("fingerprint").toString(),
                 QStringLiteral("%1").arg(core::fingerprint(controller.document()), 16, 16, QLatin1Char('0')));
    }

    void detachingTheItemLeavesItEmpty()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        controller.setFixedPage(nullptr);
        QVERIFY(fixed.sourceDocument() == nullptr);
        QCOMPARE(fixed.objectCount(), 0);
        QCOMPARE(controller.document().page(0).objects.size(), std::size_t{1}); // rien n'est perdu
    }
```

  Dans `generatedFirstPageForTheBench`, ajouter `QVERIFY(!controller.isModified());` (la page du banc n'est pas une
  modification). Inclure `<QUrl>` et `<QSizeF>`. Dans `tests/tst_pageview.cpp`, ajouter (avec l'include
  `<actcraft/ui/documentcontroller.h>`) un test qui reprend l'installation de la fixture existante (fenêtre, vue, page
  fixe, encre) et vérifie qu'avec `view->setDocumentController(&controller)` (contrôleur relié à la même page fixe), un
  trait injecté au stylet par l'aide existante du fichier (celle qu'emploient les tests de trait du lot 1) arrive dans
  `controller.document().page(0)` (1 objet, couche `Top`) et que `strokeFrozen` est émis une fois ; sans contrôleur,
  le trait va comme avant dans la page interne (test existant inchangé).

- [ ] **Étape 2 : constater le rouge.** Commande ciblée, motif `tst_ui_document|tst_pageview` → `error`.

- [ ] **Étape 3 : écrire l'en-tête du contrôleur.** Remplacer `ui/include/actcraft/ui/documentcontroller.h` par :

```cpp
#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/undo.h>
#include <actcraft/render/fixedpageitem.h>

#include <QObject>
#include <QPointer>
#include <QSizeF>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

#include <cstddef>
#include <memory>

namespace actcraft::ui {

// Document ouvert (DA2, décision 16) : le contrôleur possède le document et sa pile d'annulation ; la page fixe
// LIT la page affichée (setSource) ; toute modification passe par execute() (pile), ses changements vont au cache
// de la page affichée par core::applyToCache (DA11). Relais du lot 1 défait (revue L1-T9).
class DocumentController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        actcraft::render::FixedPageItem* fixedPage READ fixedPage WRITE setFixedPage NOTIFY fixedPageChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pagesChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY pagesChanged)
    Q_PROPERTY(QString pageLabel READ pageLabel NOTIFY pagesChanged)
    Q_PROPERTY(QSizeF pageSize READ pageSize NOTIFY pagesChanged)
    Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QUrl fileUrl READ fileUrl NOTIFY fileUrlChanged)
    Q_PROPERTY(QString fingerprint READ fingerprintText NOTIFY documentChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY undoChanged)

public:
    explicit DocumentController(QObject* parent = nullptr);
    ~DocumentController() override; // détache la page fixe

    render::FixedPageItem* fixedPage() const noexcept;
    // L'élément montre aussitôt la page affichée ; l'ancien est détaché (page interne vide).
    void setFixedPage(render::FixedPageItem* page);
    int pageCount() const noexcept;
    int currentIndex() const noexcept;
    QString pageLabel() const; // « n / N » (F-002)
    QSizeF pageSize() const;
    bool isModified() const noexcept;
    QString title() const;
    QUrl fileUrl() const;
    QString fingerprintText() const; // core::fingerprint en hexadécimal (16 chiffres)
    bool canUndo() const noexcept;
    bool canRedo() const noexcept;

    const core::Document& document() const noexcept;
    // Seule voie d'écriture du document (décision 16).
    void execute(std::unique_ptr<core::UndoCommand> command);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    // Le stylo verse son trait (DocumentSink du stylo, ControllerSink) : couche supérieure de la page affichée.
    void commitStroke(core::Stroke stroke);
    // Remplace le document (ouverture, nouveau) : pile vidée, état propre, page 1 affichée.
    void replaceDocument(core::Document document, const QUrl& fileUrl, const QString& title);
    // Paperboard vierge « Sans titre » d'une page `width` × `height` (F-656, F-700).
    Q_INVOKABLE void newDocument(double width, double height);

    // F-125 : sur la dernière page, ajoute une page (commande annulable) et l'affiche.
    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void pageBack();
    // Banc et tests : la page 1 devient la page générée de 300 objets ; document propre, pile vide.
    Q_INVOKABLE void loadGeneratedFirstPage();

signals:
    void fixedPageChanged();
    // Juste avant un changement de page affichée (ajout compris) : la vue y termine le trait en cours.
    void pageAboutToChange();
    // Une fois par changement de page affichée ou du nombre de pages, l'élément montrant déjà la nouvelle page.
    void pagesChanged();
    void modifiedChanged();
    void titleChanged();
    void fileUrlChanged();
    void documentChanged();
    void undoChanged();

private:
    // Changements d'une commande : page affichée recalée, cache, signaux.
    void apply(const core::Changes& changes);
    void show(std::size_t index);
    void afterReplace();

    core::Document m_document;
    core::UndoStack m_undo;
    std::size_t m_current = 0;
    QPointer<render::FixedPageItem> m_fixed;
    QUrl m_fileUrl;
    QString m_title;
    bool m_wasModified = false;
};

} // namespace actcraft::ui
```

- [ ] **Étape 4 : écrire le contrôleur.** Remplacer `ui/src/documentcontroller.cpp` par :

```cpp
#include <actcraft/core/edits.h>
#include <actcraft/core/generator.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/ui/documentcontroller.h>

#include <algorithm>
#include <utility>

namespace actcraft::ui {

namespace {

const QString kUntitled = QStringLiteral("Sans titre");

} // namespace

DocumentController::DocumentController(QObject* parent) : QObject(parent), m_title(kUntitled) {}

DocumentController::~DocumentController()
{
    if (m_fixed) {
        m_fixed->setSource(nullptr, 0);
    }
}

render::FixedPageItem* DocumentController::fixedPage() const noexcept
{
    return m_fixed.data();
}

void DocumentController::setFixedPage(render::FixedPageItem* page)
{
    if (page == m_fixed) {
        return;
    }
    if (m_fixed) {
        m_fixed->setSource(nullptr, 0);
    }
    m_fixed = page;
    if (m_fixed) {
        m_fixed->setSource(&m_document, m_current);
    }
    emit fixedPageChanged();
}

int DocumentController::pageCount() const noexcept
{
    return static_cast<int>(m_document.pageCount());
}

int DocumentController::currentIndex() const noexcept
{
    return static_cast<int>(m_current);
}

QString DocumentController::pageLabel() const
{
    return QStringLiteral("%1 / %2").arg(m_current + 1).arg(m_document.pageCount());
}

QSizeF DocumentController::pageSize() const
{
    const core::Page& p = m_document.page(m_current);
    return QSizeF(p.width, p.height);
}

bool DocumentController::isModified() const noexcept
{
    return !m_undo.isClean();
}

QString DocumentController::title() const
{
    return m_title;
}

QUrl DocumentController::fileUrl() const
{
    return m_fileUrl;
}

QString DocumentController::fingerprintText() const
{
    return QStringLiteral("%1").arg(core::fingerprint(m_document), 16, 16, QLatin1Char('0'));
}

bool DocumentController::canUndo() const noexcept
{
    return m_undo.canUndo();
}

bool DocumentController::canRedo() const noexcept
{
    return m_undo.canRedo();
}

const core::Document& DocumentController::document() const noexcept
{
    return m_document;
}

void DocumentController::execute(std::unique_ptr<core::UndoCommand> command)
{
    apply(m_undo.push(m_document, std::move(command)));
}

void DocumentController::undo()
{
    apply(m_undo.undo(m_document));
}

void DocumentController::redo()
{
    apply(m_undo.redo(m_document));
}

void DocumentController::commitStroke(core::Stroke stroke)
{
    core::PageObject object = std::move(stroke);
    object.common.layer = core::Layer::Top; // F-510 : les annotations vont sur la couche supérieure
    execute(std::make_unique<core::AddObjectCommand>(m_current, std::move(object)));
}

void DocumentController::apply(const core::Changes& changes)
{
    if (changes.empty()) {
        return;
    }
    // Page affichée recalée : ajout avant ou sur elle → + 1 ; retrait avant elle → − 1 ; puis bornée.
    std::size_t target = m_current;
    bool pagesMoved = false;
    for (const core::Change& c : changes) {
        if (c.kind == core::Change::Kind::PageInserted) {
            pagesMoved = true;
            if (c.page <= target) {
                ++target;
            }
        } else if (c.kind == core::Change::Kind::PageRemoved) {
            pagesMoved = true;
            if (c.page < target) {
                --target;
            }
        }
    }
    if (pagesMoved) {
        target = std::min(target, m_document.pageCount() - 1);
        if (target != m_current) {
            emit pageAboutToChange();
        }
        show(target); // la page fixe relit la page affichée ; pagesChanged une fois
    } else if (m_fixed) {
        core::applyToCache(changes, m_current, *m_fixed);
    }
    notifyState();
}

void DocumentController::notifyState()
{
    emit documentChanged();
    emit undoChanged();
    if (isModified() != m_wasModified) {
        m_wasModified = isModified();
        emit modifiedChanged();
    }
}

void DocumentController::show(std::size_t index)
{
    m_current = std::min(index, m_document.pageCount() - 1);
    if (m_fixed) {
        m_fixed->setSource(&m_document, m_current);
    }
    emit pagesChanged();
}

void DocumentController::replaceDocument(core::Document document, const QUrl& fileUrl, const QString& title)
{
    emit pageAboutToChange();
    m_document = std::move(document);
    m_undo.clear();
    m_fileUrl = fileUrl;
    m_title = title.isEmpty() ? kUntitled : title;
    afterReplace();
}

void DocumentController::newDocument(double width, double height)
{
    replaceDocument(core::Document(width, height), QUrl(), kUntitled);
}

void DocumentController::afterReplace()
{
    show(0);
    emit fileUrlChanged();
    emit titleChanged();
    emit documentChanged();
    emit undoChanged();
    m_wasModified = false;
    emit modifiedChanged();
}

void DocumentController::nextPage()
{
    emit pageAboutToChange(); // le trait en cours rejoint la page affichée avant le déplacement
    if (m_current + 1 < m_document.pageCount()) {
        show(m_current + 1);
        return;
    }
    // Taille relevée avant l'ajout (la référence à la dernière page ne survit pas à l'insertion).
    const double width = m_document.page(m_document.pageCount() - 1).width;
    const double height = m_document.page(m_document.pageCount() - 1).height;
    const std::size_t added = m_document.pageCount();
    // Page ajoutée après la page affichée : rien pour son cache ; une seule émission de pagesChanged.
    m_undo.push(m_document, std::make_unique<core::AddPageCommand>(added, width, height));
    show(added);
    notifyState();
}

void DocumentController::pageBack()
{
    const std::size_t target = core::formerPageIndex(m_current);
    if (target == m_current) {
        return;
    }
    emit pageAboutToChange();
    show(target);
}

void DocumentController::loadGeneratedFirstPage()
{
    core::Page generated = core::generatePage(core::kDefaultSeed);
    std::vector<core::Page> pages;
    pages.reserve(m_document.pageCount());
    pages.push_back(std::move(generated));
    for (std::size_t i = 1; i < m_document.pageCount(); ++i) {
        pages.push_back(m_document.page(i));
    }
    m_document = core::Document(std::move(pages));
    m_undo.clear();
    if (m_fixed) {
        m_fixed->setSource(&m_document, m_current);
    }
    emit documentChanged();
}

} // namespace actcraft::ui
```

  Règle de recalage vérifiée par `addedPageIsUndoneAndTheShownPageStaysValid` : annuler l'ajout de la page affichée
  (retrait sur elle) borne l'indice et montre la page 1 ; rétablir (ajout après elle) ne la change pas (« 1 / 2 »).
  `nextPage` n'émet `pagesChanged` qu'une fois (`pagesChangedOncePerMoveWithTheNewPageSettled`, lot 1). Ajouter
  `void notifyState();` aux méthodes privées de l'en-tête (signaux d'état : document, pile, modifié).

- [ ] **Étape 5 : le stylo verse au contrôleur.** Dans `ui/src/penadapters.h`, déclarer :

```cpp
class DocumentController;

// Le trait figé va au document par la pile (L2-T15) ; la vue est prévenue dans le même tour de boucle (le cache
// est redessiné avec le vidage de l'encre, décision 10).
class ControllerSink final : public tools::DocumentSink
{
public:
    ControllerSink(DocumentController& controller, PageView& view);
    void commitStroke(core::Stroke stroke) override;

private:
    DocumentController& m_controller;
    PageView& m_view;
};
```

  et dans `penadapters.cpp` (include `<actcraft/ui/documentcontroller.h>`) :

```cpp
ControllerSink::ControllerSink(DocumentController& controller, PageView& view)
    : m_controller(controller), m_view(view)
{
}

void ControllerSink::commitStroke(core::Stroke stroke)
{
    m_controller.commitStroke(std::move(stroke));
    m_view.onStrokeFrozen();
}
```

  Dans `ui/include/actcraft/ui/pageview.h` : déclaration avancée `class DocumentController;`, propriété
  `Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
  setDocumentController NOTIFY documentControllerChanged)`, accesseurs, signal `documentControllerChanged()`, membres
  `QPointer<DocumentController> m_controller;` et `std::unique_ptr<ControllerSink> m_controllerSink;` (déclaration
  avancée de `ControllerSink`). Dans `pageview.cpp`, `setDocumentController` mémorise le contrôleur et appelle
  `rebuildPen()` ; dans `rebuildPen()`, le puits du stylo est `ControllerSink(*m_controller, *this)` quand un
  contrôleur est posé, sinon `FixedPageSink` comme avant :

```cpp
    tools::DocumentSink* sink = nullptr;
    if (m_controller) {
        m_controllerSink = std::make_unique<ControllerSink>(*m_controller, *this);
        sink = m_controllerSink.get();
    } else {
        m_documentSink = std::make_unique<FixedPageSink>(*m_fixedPage, *this);
        sink = m_documentSink.get();
    }
    m_pen = std::make_unique<tools::PenTool>(*m_inkSurface, *sink);
```

  (`m_controllerSink.reset()` avec les autres remises à zéro en tête de `rebuildPen`.) La vue relie aussi
  `FixedPageItem::contentChanged` à `requestCacheRefresh()` dans `setFixedPage` (connexion directe, gardée dans une
  `QMetaObject::Connection` comme `m_readyConnection`) : une annulation redessine le cache. Le trait figé arrive
  alors **deux fois** à la vue (`contentChanged` par l'insertion, puis `onStrokeFrozen` par le puits) et
  `requestCacheRefresh` ne fusionne pas : une demande pendant un redessin en relance un second (`m_refreshAgain`,
  `pageview.cpp`). **Un seul redessin par trait** : `onStrokeFrozen()` n'émet plus que `strokeFrozen` (banc, joueur de
  scénarios) ; la demande de redessin vient de `contentChanged`, émis dans le même tour de boucle que le vidage de
  l'encre (décision 10 tenue). Test d'abord, dans `tests/tst_pageview.cpp` :

```cpp
// Vue AVEC cache, page fixe et encre, sans QML : le test termine lui-même chaque redessin demandé (personne
// n'appelle notifyCacheUpdated hors de PageArea.qml) et compte les demandes.
struct CachedFixture
{
    QQuickWindow window;
    PageView* view = nullptr;
    actcraft::render::FixedPageItem* fixed = nullptr;
    QPointingDevice* stylus = nullptr;
};

bool openCached(CachedFixture& f, QPointingDevice* stylus)
{
    f.stylus = stylus;
    f.window.resize(1000, 600);
    f.view = new PageView;
    f.view->setParentItem(f.window.contentItem());
    f.view->setSize(QSizeF(1000, 600));
    auto* content = new QQuickItem;
    content->setSize(QSizeF(1024, 768));
    content->setParentItem(f.view);
    f.fixed = new actcraft::render::FixedPageItem;
    f.fixed->setSize(content->size());
    f.fixed->setParentItem(content);
    auto* ink = new actcraft::render::InkItem;
    ink->setSize(content->size());
    ink->setParentItem(content);
    f.view->setContent(content); // cache activé (défaut)
    f.view->setFixedPage(f.fixed);
    f.view->setInk(ink);
    f.fixed->setPage(actcraft::core::Page{});
    f.window.show();
    if (!QTest::qWaitForWindowExposed(&f.window) || !QTest::qWaitFor([&] { return f.fixed->isReady(); }, 5000)) {
        return false;
    }
    for (int i = 0; i < 5 && !f.view->cacheReady(); ++i) {
        f.view->notifyCacheUpdated();
    }
    return f.view->cacheReady();
}

void cachedStroke(CachedFixture& f, QPointF from, QPointF to)
{
    const auto send = [&f](QEvent::Type type, QPointF scene, Qt::MouseButtons buttons) {
        QTabletEvent event(type, f.stylus, scene, f.window.mapToGlobal(scene), buttons ? 0.5 : 0.0, 0, 0, 0, 0, 0,
                           Qt::NoModifier, Qt::LeftButton, buttons);
        QCoreApplication::sendEvent(&f.window, &event);
    };
    send(QEvent::TabletPress, from, Qt::LeftButton);
    for (int i = 1; i <= 4; ++i) {
        send(QEvent::TabletMove, from + (to - from) * i / 4.0, Qt::LeftButton);
    }
    send(QEvent::TabletRelease, to, Qt::NoButton);
}
```

  (ces trois définitions dans l'espace anonyme du fichier, après `DrawFixture`), puis, dans la classe :

```cpp
    void oneCacheRenderPerStroke()
    {
        CachedFixture f;
        QVERIFY(openCached(f, m_stylus.get()));
        ui::DocumentController controller;
        controller.setFixedPage(f.fixed);
        f.view->setDocumentController(&controller);
        for (int i = 0; i < 5 && !f.view->cacheReady(); ++i) {
            f.view->notifyCacheUpdated(); // setSource du contrôleur a demandé un redessin
        }
        QVERIFY(f.view->cacheReady());
        QSignalSpy requested(f.view, &PageView::cacheRefreshRequested);
        cachedStroke(f, QPointF(300, 200), QPointF(500, 260));
        QCOMPARE(controller.document().page(0).objects.size(), std::size_t{1});
        for (int i = 0; i < 5 && !f.view->cacheReady(); ++i) {
            f.view->notifyCacheUpdated(); // fin de redessin, comme ShaderEffectSource.scheduledUpdateCompleted
        }
        QVERIFY(f.view->cacheReady());
        QCOMPARE(requested.count(), 1);
    }
```

  Rouge attendu avant le changement de `onStrokeFrozen` (vérifié sur `pageview.cpp:207-235`) : `contentChanged` →
  demande 1 (redessin en cours) ; `onStrokeFrozen` → demande 2 et `m_refreshAgain` ; le premier `notifyCacheUpdated`
  relance → demande 3 ; `requested.count() == 3`. Vert : 1. Le relever tel quel au rapport (rouge 3, vert 1) ; si le
  code donne un autre nombre, l'écrire et l'expliquer, sans ajuster l'assertion du vert (1).

- [ ] **Étape 6 : brancher le QML.** Dans `ui/qml/PageArea.qml`, ajouter `property DocumentController
  documentController: null` à l'élément racine et `documentController: area.documentController` à la `PageView`.
  Dans `ui/qml/Main.qml`, passer `documentController: appDocument` au `PageArea`. Le reste de `Main.qml` ne change pas.

- [ ] **Étape 7 : vert ciblé.** Commande ciblée, motif `tst_ui_|tst_pageview|tst_captures` : `100% tests passed`.
  `tst_ui_shell`, `tst_ui_drawing` et `tst_captures_premiers_pas` passent sans changement (même comportement vu de
  l'interface). Rouge par mutation : remplacer dans `commitStroke` `core::Layer::Top` par `core::Layer::Middle`,
  constater l'échec de `strokeLandsOnTheTopLayerOfTheShownPage`, remettre le fichier par
  `git -C <worktree> checkout -- ui/src/documentcontroller.cpp` (si le fichier n'est pas encore commité, faire la
  mutation après le commit de l'étape 9 puis remettre).

- [ ] **Étape 8 : preuve, capture et banc.** `format.ps1`, preuve de tâche (N ≥ 58), commande de preuve visuelle
  (lecture `pageIndicator.text=1 / 2` puis `CAPTURES OK` ; décrire la capture `retour-page-1` : trait rouge présent),
  banc sans écran (`content_check.ok = True`).

- [ ] **Étape 9 : commit.**

```powershell
git -C <worktree> add ui/include/actcraft/ui/documentcontroller.h ui/src/documentcontroller.cpp ui/src/penadapters.h ui/src/penadapters.cpp ui/include/actcraft/ui/pageview.h ui/src/pageview.cpp ui/qml/Main.qml ui/qml/PageArea.qml tests/tst_ui_document.cpp tests/tst_pageview.cpp
git -C <worktree> commit -m "ui : le contrôleur possède le document et sa pile ; le stylo verse par une commande annulable ; relais du lot 1 défait" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t16"></a>
### Tâche L2-T16 — `ui` : taille de page suivie, barres à jour, échelle de rastérisation

**Objectif.** F-602 et constat L1-T8 (« `scrollChanged` non émis quand la taille du contenu change seule ») : la page
affichée n'a plus toujours 1024 × 768 (paperboard 800 × 600, Nouveau 1280 × 1024). L'élément `page` de `PageArea.qml`
prend la taille de la page affichée (`documentController.pageSize`) ; la vue, quand la taille de son contenu change
**sans** changer d'élément, se réajuste (mode « Ajuster ») ou borne son défilement, met à jour la taille de texture du
cache, redemande le cache et émet `scrollChanged`. Quand le zoom se pose (fin de zoom, ajustement), la vue donne à la
page fixe son échelle de rastérisation (`zoom × devicePixelRatio`, L2-T13), avant de redemander le cache.

**Fichiers.**
- Modifier : `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`, `ui/qml/PageArea.qml`,
  `tests/tst_pageview.cpp`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_pageview` → échec de
`contentResizeRefitsAndEmitsScroll`. Vert : forme bornée, N ≥ 58, `TESTS OK`, `tst_pageview .*Passed`. Banc hors
écran : `content_check.ok = True` (page du banc à 1024 × 768 : aucun changement).

**Interdits.** Ne pas changer `ViewTransform`, `fitPage`, `clampPan` (lot 1). Pas de reconstruction de la page fixe
à chaque image d'un zoom continu : l'échelle de rastérisation n'est posée qu'à la fin du zoom (minuterie existante
`m_cacheTimer`) et par `setZoom` / `fitPage`.

**Terminé quand.** Preuve verte, JSON du banc joint, commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests.** Dans `tests/tst_pageview.cpp`, ajouter (avec la fixture existante qui pose une
  vue sur un élément `content` de 1024 × 768) :

```cpp
    void contentResizeRefitsAndEmitsScroll()
    {
        Fixture f;
        open(f);
        f.view->fitPage();
        const qreal before = f.view->zoom();
        QSignalSpy scrolled(f.view, &PageView::scrollChanged);
        f.content->setSize(QSizeF(2048, 1536)); // même élément, page deux fois plus grande
        QVERIFY(scrolled.count() >= 1);
        QVERIFY(qAbs(f.view->zoom() - before / 2) < 1e-6); // « Ajuster » suit la nouvelle taille
    }

    void contentResizeRequestsTheCacheOnce()
    {
        Fixture f;
        open(f);
        f.view->fitPage();
        QTRY_VERIFY(f.view->cacheReady());
        QSignalSpy requested(f.view, &PageView::cacheRefreshRequested);
        f.content->setSize(QSizeF(800, 600));
        QCOMPARE(requested.count(), 1);
    }

    void contentResizeOutsideFitClampsThePan()
    {
        Fixture f;
        open(f);
        f.view->setZoom(2.0);
        f.view->scrollTo(1.0, 1.0); // en bas à droite
        QSignalSpy scrolled(f.view, &PageView::scrollChanged);
        f.content->setSize(QSizeF(512, 384)); // la page rapetisse : le défilement doit rester dans les bornes
        QVERIFY(scrolled.count() >= 1);
        QCOMPARE(f.view->zoom(), 2.0);
        QVERIFY(f.view->scrollPositionX() <= 1.0);
    }

    void settledZoomSetsTheRasterScale()
    {
        Fixture f;
        open(f);
        f.view->setZoom(2.0);
        const qreal dpr = f.window.effectiveDevicePixelRatio();
        QCOMPARE(f.fixed->rasterScale(), 2.0 * dpr);
        f.view->fitPage();
        QCOMPARE(f.fixed->rasterScale(), f.view->zoom() * dpr);
    }
```

  Les noms `Fixture`, `open`, `f.content`, `f.fixed`, `f.window` sont ceux de la fixture existante du fichier : les
  relire et s'y conformer (si l'élément de contenu porte un autre nom, l'utiliser).

- [ ] **Étape 2 : constater le rouge.** Commande ciblée, motif `tst_pageview` → `contentResizeRefitsAndEmitsScroll`
  et `settledZoomSetsTheRasterScale` en échec (`FAIL!`).

- [ ] **Étape 3 : suivre la taille du contenu.** Dans `pageview.h`, membre `QMetaObject::Connection
  m_contentWidthConnection, m_contentHeightConnection;` (déclarés séparément) et méthode privée
  `void onContentSizeChanged(QSizeF oldPage);`. Dans `pageview.cpp`, `setContent` déconnecte les deux connexions de
  l'ancien contenu et relie `widthChanged` / `heightChanged` du nouveau :

```cpp
    disconnect(m_contentWidthConnection);
    disconnect(m_contentHeightConnection);
    // … (code existant qui pose m_content) …
    if (m_content) {
        m_lastPageSize = pageSize();
        m_contentWidthConnection = connect(m_content, &QQuickItem::widthChanged, this, [this] {
            onContentSizeChanged(m_lastPageSize);
        });
        m_contentHeightConnection = connect(m_content, &QQuickItem::heightChanged, this, [this] {
            onContentSizeChanged(m_lastPageSize);
        });
    }
```

  avec le membre `QSizeF m_lastPageSize;` et :

```cpp
void PageView::onContentSizeChanged(QSizeF oldPage)
{
    const QSizeF newPage = pageSize();
    m_lastPageSize = newPage;
    if (newPage == oldPage) {
        return;
    }
    finishStroke(); // un trait en cours reste sur sa page (comme pour un zoom)
    const ViewTransform old = m_transform;
    // Pas fitPage() ici : il demanderait lui-même un redessin du cache, et la fin de cette fonction un second.
    m_cacheTimer.stop();
    apply(m_fitMode ? ui::fitPage(size(), newPage) : clampPan(m_transform, size(), newPage));
    // apply n'émet rien si ni le zoom ni le défilement n'ont bougé : la taille de la page change pourtant les barres.
    if (qFuzzyCompare(old.zoom, m_transform.zoom) && old.pan == m_transform.pan) {
        emit scrollChanged();
    }
    updateCacheTextureSize();
    requestCacheRefresh();
}
```

- [ ] **Étape 4 : échelle de rastérisation.** À la fin de `updateCacheTextureSize()` (appelée quand le zoom est posé :
  minuterie, `setZoom`, `fitPage`, contenu redimensionné), ajouter :

```cpp
    if (m_fixedPage) {
        const qreal dpr = window() ? window()->effectiveDevicePixelRatio() : 1.0;
        m_fixedPage->setRasterScale(m_transform.zoom * dpr);
    }
```

  `setRasterScale` ne reconstruit qu'en RHI et pour une page qui peint quelque chose (L2-T13).

- [ ] **Étape 5 : la page prend la taille du document.** Dans `ui/qml/PageArea.qml`, l'élément `pageContent` :

```qml
            width: area.documentController ? area.documentController.pageSize.width : 1024
            height: area.documentController ? area.documentController.pageSize.height : 768
```

  (au lieu de `width: 1024` / `height: 768`) ; le commentaire « Page au format d'un paperboard (1024 × 768) » devient
  « Page à la taille de la page affichée (F-602) ».

- [ ] **Étape 6 : vert ciblé, preuve et banc.** Commande ciblée (`tst_pageview|tst_ui_`) : `100% tests passed`.
  `format.ps1`, preuve de tâche (N ≥ 58 ; `tst_pageview` Passed), banc sans écran (`content_check.ok = True`).

- [ ] **Étape 7 : commit.**

```powershell
git -C <worktree> add ui/include/actcraft/ui/pageview.h ui/src/pageview.cpp ui/qml/PageArea.qml tests/tst_pageview.cpp
git -C <worktree> commit -m "ui : la page suit la taille de la page affichée ; barres et cache à jour ; échelle de rastérisation au zoom posé" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t17"></a>
### Tâche L2-T17 — `ui` : ouvrir, enregistrer, nouveau, fermer — hors du fil principal, miniatures

**Objectif.** Décisions 17, 18, 22 et règle de fluidité 1 : lecture et écriture des documents **sur un fil
secondaire**, résultat rendu au fil principal. Ouvrir une URL : la signature choisit le lecteur (`PK\3\4` → archive
`.actcraft`, `" Bamboo "` → `.flipchart`), sinon erreur. Un `.flipchart` ouvert devient un document **sans fichier**
(« Enregistrer » proposera `<nom>.actcraft`, décision 18). Enregistrer : instantané du document (copie, ressources
partagées implicitement), miniatures PNG de chaque page par le peintre partagé (L2-T11, images décodées dans le même
fil), archive (L2-T6), écriture atomique (L2-T6) ; à la fin, état propre si le document n'a pas changé pendant
l'enregistrement (empreinte comparée). Pendant une opération, `busy` est vrai et une seconde opération est refusée.
Avertissements de lecture et dernière erreur exposés (le bandeau est en L2-T18). Journal : `ACTCRAFT OUVERT
<empreinte>` et `ACTCRAFT ENREGISTRE <empreinte>` (catégorie `actcraft.document`, info) — L2-T22 les lit dans
logcat. Dossier par défaut (F-657, N2-4) : `Documents/ActCraft` sous Windows (créé), dossier de l'application ailleurs.

**Fichiers.**
- Créer : `ui/include/actcraft/ui/documentio.h`, `ui/src/documentio.cpp`, `tests/tst_ui_files.cpp`
- Modifier : `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`, `ui/CMakeLists.txt`,
  `tests/ui.cmake`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_ui_files` → `error`. Vert : forme bornée, N ≥ 59,
`TESTS OK`, `tst_ui_files .*Passed`. Banc hors écran (module `ui/`) : `content_check.ok = True`.

**Interdits.** Aucune lecture, aucun décodage, aucune compression, aucune écriture sur le fil principal (sauf le
calcul de l'empreinte, rapide). Le document affiché n'est jamais modifié par le fil secondaire (il travaille sur une
copie). Un `.flipchart` n'est jamais réécrit. Pas de dialogue ici (L2-T18).

**Terminé quand.** Preuve verte, JSON du banc joint, commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests.** Créer `tests/tst_ui_files.cpp` :

```cpp
#include "support/flipchartwriter.h"

#include <actcraft/formats/nativefile.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documentio.h>

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace actcraft;

namespace {

core::Stroke redStroke()
{
    core::Stroke s;
    s.points = {core::StrokePoint{100, 100, 0.5f}, core::StrokePoint{300, 100, 0.5f}};
    s.baseWidth = 8;
    s.color = 0xFFE53935u;
    return s;
}

bool writeFile(const QString& path, const QByteArray& bytes)
{
    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size();
}

} // namespace

// Ouverture et enregistrement hors du fil principal (décision 17, règle de fluidité 1).
class TstUiFiles : public QObject
{
    Q_OBJECT

private slots:
    void saveThenOpenKeepsTheFingerprint()
    {
        QTemporaryDir dir;
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        controller.nextPage();
        const QString saved = controller.property("fingerprint").toString();
        const QUrl url = QUrl::fromLocalFile(dir.filePath(QStringLiteral("essai.actcraft")));
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        controller.saveToUrl(url);
        QVERIFY(controller.property("busy").toBool());
        QVERIFY(savedSpy.wait(10000));
        QCOMPARE(savedSpy.first().at(0).toBool(), true);
        QVERIFY(!controller.isModified());
        QCOMPARE(controller.fileUrl(), url);
        QCOMPARE(controller.title(), QStringLiteral("essai"));
        controller.newDocument(1024, 768);
        QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
        controller.openUrl(url);
        QVERIFY(loaded.wait(10000));
        QCOMPARE(loaded.first().at(0).toBool(), true);
        QCOMPARE(controller.property("fingerprint").toString(), saved);
        QCOMPARE(controller.pageLabel(), QStringLiteral("1 / 2"));
        QVERIFY(!controller.isModified());
    }

    // Le fil d'exécution est relevé PAR l'écriture et la lecture elles-mêmes (point d'injection : offMainThread des
    // résultats) : une écriture synchrone émise ensuite en file serait vue (relecture du plan, tour 1).
    void ioRunsOffTheMainThread()
    {
        QTemporaryDir dir;
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        const QUrl url = QUrl::fromLocalFile(dir.filePath(QStringLiteral("fil.actcraft")));
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        controller.saveToUrl(url);
        QVERIFY(savedSpy.wait(10000));
        QVERIFY(controller.property("lastIoOffMainThread").toBool());
        QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
        controller.openUrl(url);
        QVERIFY(loaded.wait(10000));
        QVERIFY(controller.property("lastIoOffMainThread").toBool());
        // Appel direct sur le fil principal : le résultat le dit (le relevé n'est pas une constante).
        QVERIFY(!ui::saveDocument(controller.document(), url).offMainThread);
    }

    void thumbnailsAreWrittenForEveryPage()
    {
        QTemporaryDir dir;
        core::Document document(std::vector<core::Page>(3));
        const QUrl url = QUrl::fromLocalFile(dir.filePath(QStringLiteral("m.actcraft")));
        QVERIFY(ui::saveDocument(document, url).ok);
        QFile file(url.toLocalFile());
        QVERIFY(file.open(QIODevice::ReadOnly));
        const formats::DecodeResult r = formats::decodeActcraft(file.readAll());
        QVERIFY(r.ok);
        QCOMPARE(r.thumbnails.size(), std::size_t{3});
        QImage thumb;
        QVERIFY(thumb.loadFromData(r.thumbnails[2], "PNG"));
        QCOMPARE(thumb.size(), QSize(320, 240));
    }

    void openingAFlipchartGivesADocumentWithoutFile()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("cours.flipchart"));
        QVERIFY(writeFile(path, fctest::sampleFlipchart()));
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
        controller.openUrl(QUrl::fromLocalFile(path));
        QVERIFY(loaded.wait(10000));
        QCOMPARE(loaded.first().at(0).toBool(), true);
        QCOMPARE(controller.pageCount(), 2);
        QVERIFY(controller.fileUrl().isEmpty());
        QCOMPARE(controller.title(), QStringLiteral("cours"));
        QCOMPARE(controller.property("suggestedFileName").toString(), QStringLiteral("cours.actcraft"));
        QCOMPARE(controller.property("warnings").toStringList().size(), 1); // la formule non lue
    }

    void unreadableFileKeepsTheDocumentAndReportsWhy()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("faux.actcraft"));
        QVERIFY(writeFile(path, "ni zip ni paperboard"));
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        const QString before = controller.property("fingerprint").toString();
        QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
        controller.openUrl(QUrl::fromLocalFile(path));
        QVERIFY(loaded.wait(10000));
        QCOMPARE(loaded.first().at(0).toBool(), false);
        QVERIFY(!controller.property("lastError").toString().isEmpty());
        QCOMPARE(controller.property("fingerprint").toString(), before);
        QVERIFY(controller.isModified());
    }

    void secondOperationIsRefusedWhileBusy()
    {
        QTemporaryDir dir;
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        controller.saveToUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("a.actcraft"))));
        controller.saveToUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("b.actcraft"))));
        QCOMPARE(savedSpy.count(), 0); // le refus aussi arrive en file, jamais pendant l'appel
        QTRY_COMPARE_WITH_TIMEOUT(savedSpy.count(), 2, 10000);
        // Le refus, mis en file aussitôt, arrive AVANT la fin de l'écriture de « a ».
        QCOMPARE(savedSpy.at(0).at(0).toBool(), false);
        QCOMPARE(savedSpy.at(1).at(0).toBool(), true);
        QVERIFY(QFile::exists(dir.filePath(QStringLiteral("a.actcraft"))));
        QVERIFY(!QFile::exists(dir.filePath(QStringLiteral("b.actcraft"))));
    }

    void argumentsBecomeUrlsAndTheDefaultFolderExists()
    {
        ui::DocumentController controller;
        QVERIFY(controller.urlFromArgument(QStringLiteral("content://fournisseur/doc")).scheme() ==
                QStringLiteral("content"));
        QVERIFY(controller.urlFromArgument(QVariant()).isEmpty());
        const QUrl folder = controller.defaultFolderUrl();
        QVERIFY(folder.isLocalFile());
        QVERIFY(QFileInfo(folder.toLocalFile()).isDir());
    }
};

QTEST_MAIN(TstUiFiles)
#include "tst_ui_files.moc"
```

  `secondOperationIsRefusedWhileBusy` : le refus est rendu par `saveFinished(false, …)` émis **en file** dès le
  second appel, donc avant le succès du premier. Ajouter `#include <QFileInfo>`,
  `<QImage>`, `<QThread>`. Ajouter à `tests/ui.cmake` :

```cmake
actcraft_add_test(tst_ui_files
    SOURCES tst_ui_files.cpp
    LIBS Qt6::Quick ActCraft::ui ActCraft::formats)
```

- [ ] **Étape 2 : constater le rouge.** Commande ciblée, motif `tst_ui_files` → `error`.

- [ ] **Étape 3 : écrire les E/S de document.** Créer `ui/include/actcraft/ui/documentio.h` :

```cpp
#pragma once

#include <actcraft/core/document.h>

#include <QString>
#include <QStringList>
#include <QUrl>

#include <cstdint>

namespace actcraft::ui {

// Lecture et écriture d'un document par son URL (décision 17), à appeler sur un fil secondaire.
struct LoadOutcome
{
    bool ok = false;
    QString error;
    QStringList warnings;
    core::Document document;
    bool native = false; // .actcraft (sinon .flipchart : document sans fichier, décision 18)
    QString title;       // nom du fichier sans extension
    bool offMainThread = false; // relevé par la lecture elle-même (preuve de S5, point d'injection de test)
};

LoadOutcome loadDocument(const QUrl& url);

struct SaveOutcome
{
    bool ok = false;
    QString error;
    std::uint64_t fingerprint = 0; // empreinte du document enregistré
    bool offMainThread = false;    // relevé par l'écriture elle-même
};

// Miniatures (peintre partagé, 320 px), archive .actcraft, écriture atomique (fichier local) ou en une passe
// (content://).
SaveOutcome saveDocument(const core::Document& snapshot, const QUrl& url);
// Nom du fichier sans extension ; pour content://, dernier segment décodé.
QString titleFromUrl(const QUrl& url);
// F-657, N2-4 : Documents/ActCraft (créé s'il manque) ; sous Android, dossier de l'application.
QString defaultDocumentsFolder();

} // namespace actcraft::ui
```

  Créer `ui/src/documentio.cpp` :

```cpp
#include <actcraft/formats/flipchart.h>
#include <actcraft/formats/nativefile.h>
#include <actcraft/formats/urlio.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/ui/documentio.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QThread>
#include <QSysInfo>

namespace actcraft::ui {

namespace {

bool offMainThread()
{
    return QCoreApplication::instance() && QThread::currentThread() != QCoreApplication::instance()->thread();
}

} // namespace

LoadOutcome loadDocument(const QUrl& url)
{
    LoadOutcome out;
    out.offMainThread = offMainThread();
    const formats::UrlReadResult read = formats::readUrl(url);
    if (!read.ok) {
        out.error = read.error;
        return out;
    }
    out.title = titleFromUrl(url);
    if (formats::looksLikeActcraft(read.data)) {
        formats::DecodeResult r = formats::decodeActcraft(read.data);
        if (!r.ok) {
            out.error = r.error;
            return out;
        }
        out.ok = true;
        out.native = true;
        out.warnings = r.warnings;
        out.document = std::move(r.document);
        return out;
    }
    if (formats::flipchart::looksLikeFlipchart(read.data)) {
        formats::flipchart::FlipchartReadResult r = formats::flipchart::readFlipchart(read.data);
        if (!r.ok) {
            out.error = r.error;
            out.warnings = r.warnings;
            return out;
        }
        out.ok = true;
        out.warnings = r.warnings;
        out.document = std::move(r.document);
        return out;
    }
    out.error = QStringLiteral("format non reconnu : ni paperboard ActCraft, ni .flipchart");
    return out;
}

SaveOutcome saveDocument(const core::Document& snapshot, const QUrl& url)
{
    SaveOutcome out;
    out.offMainThread = offMainThread();
    const render::DecodedImages images = render::decodeImages(snapshot);
    std::vector<QByteArray> thumbnails;
    thumbnails.reserve(snapshot.pageCount());
    for (std::size_t i = 0; i < snapshot.pageCount(); ++i) {
        thumbnails.push_back(render::thumbnailPng(snapshot.page(i), images.lookup()));
    }
    QString error;
    const QByteArray archive = formats::encodeActcraft(snapshot, thumbnails, &error);
    if (archive.isEmpty()) {
        out.error = error;
        return out;
    }
    if (!formats::writeUrl(url, archive, &error)) {
        out.error = error;
        return out;
    }
    out.ok = true;
    out.fingerprint = core::fingerprint(snapshot);
    return out;
}

QString titleFromUrl(const QUrl& url)
{
    const QString name = url.isLocalFile() ? QFileInfo(url.toLocalFile()).fileName()
                                           : QUrl::fromPercentEncoding(url.path().section(u'/', -1).toUtf8())
                                                 .section(u':', -1)
                                                 .section(u'/', -1);
    const qsizetype dot = name.lastIndexOf(u'.');
    return dot > 0 ? name.left(dot) : name;
}

QString defaultDocumentsFolder()
{
    // Choix à l'exécution (pas de #ifdef hors de app/ et des fichiers *_android, DA10).
    const QString base = QSysInfo::productType() == QLatin1String("android")
                             ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                             : QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) +
                                   QStringLiteral("/ActCraft");
    QDir().mkpath(base);
    return base;
}

} // namespace actcraft::ui
```

- [ ] **Étape 4 : le contrôleur ouvre et enregistre.** Dans `documentcontroller.h`, ajouter les propriétés et
  méthodes :

```cpp
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(QStringList warnings READ warnings NOTIFY warningsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString suggestedFileName READ suggestedFileName NOTIFY titleChanged)
    // Vrai si la dernière lecture ou écriture a tourné hors du fil principal (S5 : la lit ; mutation : écriture
    // synchrone → faux).
    Q_PROPERTY(bool lastIoOffMainThread READ lastIoOffMainThread NOTIFY lastIoChanged)
public:
    bool isBusy() const noexcept;
    QStringList warnings() const;
    QString lastError() const;
    QString suggestedFileName() const; // « <titre>.actcraft »
    // Ouverture et enregistrement sur un fil secondaire ; loadFinished / saveFinished à la fin (toujours émis, en
    // file, même pour un refus). Refusés pendant une autre opération.
    Q_INVOKABLE void openUrl(const QUrl& url);
    Q_INVOKABLE void saveToUrl(const QUrl& url);
    // Argument d'une commande de fichier → URL (formats::urlFromArgument) ; vide si absent.
    Q_INVOKABLE QUrl urlFromArgument(const QVariant& argument) const;
    Q_INVOKABLE QUrl defaultFolderUrl() const;
signals:
    void busyChanged();
    void warningsChanged();
    void lastErrorChanged();
    void loadFinished(bool ok, const QString& message);
    void saveFinished(bool ok, const QString& message);
private:
    void setBusy(bool busy);
    bool m_busy = false;
    QStringList m_warnings;
    QString m_lastError;
    std::uint64_t m_operation = 0; // identifiant de l'opération en cours (résultats périmés ignorés)
```

  Dans `documentcontroller.cpp` (includes `<actcraft/formats/urlio.h>`, `<actcraft/ui/documentio.h>`,
  `<QLoggingCategory>`, `<QThreadPool>`, `<memory>`) :

```cpp
Q_LOGGING_CATEGORY(lcDocument, "actcraft.document", QtInfoMsg)

void DocumentController::setBusy(bool busy)
{
    if (busy != m_busy) {
        m_busy = busy;
        emit busyChanged();
    }
}

void DocumentController::openUrl(const QUrl& url)
{
    if (m_busy) {
        QMetaObject::invokeMethod(this, [this] { emit loadFinished(false, tr("Une opération est déjà en cours.")); },
                                  Qt::QueuedConnection);
        return;
    }
    setBusy(true);
    const std::uint64_t operation = ++m_operation;
    QPointer<DocumentController> self(this);
    QThreadPool::globalInstance()->start([self, operation, url] {
        auto outcome = std::make_shared<LoadOutcome>(loadDocument(url));
        QMetaObject::invokeMethod(
            self.data(),
            [self, operation, url, outcome] {
                if (!self || operation != self->m_operation) {
                    return;
                }
                self->setBusy(false);
                if (!outcome->ok) {
                    self->m_warnings = outcome->warnings;
                    emit self->warningsChanged();
                    self->m_lastError = outcome->error;
                    emit self->lastErrorChanged();
                    emit self->loadFinished(false, outcome->error);
                    return;
                }
                self->m_lastIoOffMainThread = outcome->offMainThread;
                self->replaceDocument(std::move(outcome->document), outcome->native ? url : QUrl(), outcome->title);
                self->m_warnings = outcome->warnings; // après replaceDocument, qui les efface
                emit self->warningsChanged();
                qCInfo(lcDocument, "ACTCRAFT OUVERT %s", qPrintable(self->fingerprintText()));
                emit self->loadFinished(true, QString());
            },
            Qt::QueuedConnection);
    });
}

void DocumentController::saveToUrl(const QUrl& url)
{
    if (m_busy) {
        QMetaObject::invokeMethod(this, [this] { emit saveFinished(false, tr("Une opération est déjà en cours.")); },
                                  Qt::QueuedConnection);
        return;
    }
    setBusy(true);
    const std::uint64_t operation = ++m_operation;
    auto snapshot = std::make_shared<const core::Document>(m_document); // copie : le fil n'écrit jamais l'affiché
    QPointer<DocumentController> self(this);
    QThreadPool::globalInstance()->start([self, operation, url, snapshot] {
        const SaveOutcome outcome = saveDocument(*snapshot, url);
        QMetaObject::invokeMethod(
            self.data(),
            [self, operation, url, outcome] {
                if (!self || operation != self->m_operation) {
                    return;
                }
                self->setBusy(false);
                self->m_lastIoOffMainThread = outcome.offMainThread;
                if (!outcome.ok) {
                    self->m_lastError = outcome.error;
                    emit self->lastErrorChanged();
                    emit self->saveFinished(false, outcome.error);
                    return;
                }
                if (core::fingerprint(self->m_document) == outcome.fingerprint) {
                    self->m_undo.setClean(); // rien n'a changé pendant l'écriture
                }
                self->m_fileUrl = url;
                self->m_title = titleFromUrl(url);
                emit self->fileUrlChanged();
                emit self->titleChanged();
                self->notifyState();
                qCInfo(lcDocument, "ACTCRAFT ENREGISTRE %s", qPrintable(QStringLiteral("%1").arg(
                                                                 outcome.fingerprint, 16, 16, QLatin1Char('0'))));
                emit self->saveFinished(true, QString());
            },
            Qt::QueuedConnection);
    });
}
```

  plus le membre `bool m_lastIoOffMainThread = false;`, le signal `lastIoChanged()` — émis à **chaque** fin de
  lecture ou d'écriture, succès ou échec, juste après l'affectation de `m_lastIoOffMainThread` (y compris dans la
  branche d'échec d'`openUrl` : y affecter `outcome->offMainThread` aussi) —, l'accesseur `lastIoOffMainThread()` et
  les accesseurs
  (`isBusy`, `warnings`, `lastError`, `suggestedFileName` = `m_title + ".actcraft"`,
  `urlFromArgument` = `formats::urlFromArgument(argument)`, `defaultFolderUrl` =
  `QUrl::fromLocalFile(defaultDocumentsFolder())`). `replaceDocument` vide `m_warnings` et `m_lastError` (et émet
  leurs signaux) ; `openUrl` pose les avertissements de la lecture **après** lui.

  Dans `ui/CMakeLists.txt` : ajouter `include/actcraft/ui/documentio.h` et `src/documentio.cpp` aux `SOURCES`, et
  `ActCraft::formats` à `target_link_libraries(actcraft_ui PUBLIC …)`.

- [ ] **Étape 5 : vert ciblé, rouge par mutation, preuve et banc.** Commande ciblée (`tst_ui_`) : `100% tests
  passed`. Après le commit de l'étape 6 : remplacer, dans `saveToUrl`, le `QThreadPool::globalInstance()->start(…)` par
  un appel direct du même lambda (écriture synchrone, résultat toujours émis en file) → `ioRunsOffTheMainThread` rouge
  (`lastIoOffMainThread` faux) ; remettre par `git -C <worktree> checkout -- ui/src/documentcontroller.cpp`. Puis
  `format.ps1`, preuve de tâche (N ≥ 59 ; `tst_ui_files` Passed), banc sans écran (`content_check.ok = True`).

- [ ] **Étape 6 : commit.**

```powershell
git -C <worktree> add ui/include/actcraft/ui/documentio.h ui/src/documentio.cpp ui/include/actcraft/ui/documentcontroller.h ui/src/documentcontroller.cpp ui/CMakeLists.txt tests/ui.cmake tests/tst_ui_files.cpp
git -C <worktree> commit -m "ui : ouvrir et enregistrer hors du fil principal (.actcraft, .flipchart), miniatures, état propre, journal" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t18"></a>
### Tâche L2-T18 — QML : menu Fichier, dialogues, « enregistrer les modifications ? », bandeau d'avertissements

**Objectif.** F-700 (partie), décisions 20 et 22 : le menu Fichier propose Nouveau (et ses cinq tailles), Ouvrir,
Enregistrer, Enregistrer sous, Fermer, Quitter ; raccourcis Ctrl+N, Ctrl+O, Ctrl+S, Ctrl+Maj+S, Ctrl+W, Ctrl+Q.
Chaque entrée déclenche sa commande (L2-T3) ; les commandes `file.open` et `file.save-as` **avec** une URL agissent
sans dialogue (scénarios, ligne de commande), **sans** URL ouvrent le dialogue de fichier (`QtQuick.Dialogs`, natif ;
`content://` sous Android). Un document modifié demande « Enregistrer les modifications ? » (Enregistrer / Ne pas
enregistrer / Annuler) avant Nouveau, Ouvrir, Fermer, Quitter et la fermeture de la fenêtre. Un document sans
fichier (neuf ou `.flipchart`) s'enregistre par « Enregistrer sous », nom proposé `<titre>.actcraft`, dans le dossier
par défaut. Avertissements de lecture et erreurs : **bandeau** en haut de la page, sans bloquer, refermable. Titre de
fenêtre : `[*]<titre> — ActCraft — rendu : <moteur>`.

**Fichiers.**
- Créer : `ui/qml/FileActions.qml`, `ui/qml/SaveChangesDialog.qml`, `ui/qml/PageSizeDialog.qml`,
  `ui/qml/MessageBanner.qml`, `tests/tst_ui_filemenu.cpp`
- Modifier : `ui/qml/Main.qml`, `ui/CMakeLists.txt`, `actions/data/layout.json`, `tests/ui.cmake`,
  `tests/tst_ui_shell.cpp`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_ui_filemenu` → `error`. Vert : forme bornée,
N ≥ 60, `TESTS OK`, `tst_ui_filemenu .*Passed`, `tst_ui_shell .*Passed`. Preuve visuelle (hors écran) : après le
commit, capture de la fenêtre avec le bandeau ouvert, par le test `bannerShowsReadingWarnings` qui l'écrit sous
`.tmp/` (`QT_ACTCRAFT_CAPTURES` = dossier) ; décrire : bandeau lisible au-dessus de la page, texte « 1 avertissement
de lecture », bouton de fermeture. Banc hors écran : `content_check.ok = True`.

**Interdits.** Aucun texte ni icône de Promethean ; libellés écrits par nous (`qsTr`). Pas de dialogue de fichier
dans les scénarios ni les tests (commandes avec URL). Pas de nouveau jeton de couleur sans test de contraste sur les
couleurs rendues. Tests existants : `tst_ui_shell` ne change que pour les entrées du menu Fichier (comportement voulu).

**Terminé quand.** Preuve verte, capture décrite, JSON du banc joint, commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests.** Créer `tests/tst_ui_filemenu.cpp` (même harnais que `tst_ui_shell.cpp` :
  `Shell`, `start`, `findItem`, `run` — les recopier du fichier existant, ils ne sont pas partagés) :

```cpp
// (includes et harnais Shell/start/findItem/item/run recopiés de tests/tst_ui_shell.cpp, plus :)
#include <actcraft/formats/nativefile.h>
#include <QSignalSpy>
#include <QTemporaryDir>

// Menu Fichier et ses commandes (F-700, L2-T18), sans dialogue de fichier : commandes avec URL.
class TstUiFileMenu : public QObject
{
    Q_OBJECT

private:
    static ui::DocumentController* controller(Shell& s)
    {
        return s.window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
    }

private slots:
    void fileMenuListsTheLotTwoCommands()
    {
        Shell s;
        QVERIFY(start(s));
        auto* commands = s.window->findChild<ui::Commands*>(QStringLiteral("commands"));
        QVERIFY(commands);
        QStringList ids;
        for (const QVariant& menu : commands->menus()) {
            const QVariantMap m = menu.toMap();
            if (m.value(QStringLiteral("id")).toString() == QLatin1String("file")) {
                for (const QVariant& entry : m.value(QStringLiteral("items")).toList()) {
                    ids << entry.toMap().value(QStringLiteral("id")).toString();
                }
            }
        }
        for (const char* id : {"file.new", "file.new-screen", "file.new-1024x768", "file.new-1152x864",
                               "file.new-1280x1024", "file.new-custom", "file.open", "file.save", "file.save-as",
                               "file.close", "file.quit"}) {
            QVERIFY2(ids.contains(QString::fromLatin1(id)), id);
        }
    }

    void saveAsThenOpenByCommands()
    {
        Shell s;
        QVERIFY(start(s));
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("essai.actcraft"));
        auto* doc = controller(s);
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(doc->isModified());
        QSignalSpy saved(doc, &ui::DocumentController::saveFinished);
        QVERIFY(run(s, QStringLiteral("file.save-as"), path));
        QVERIFY(saved.wait(10000));
        QVERIFY(!doc->isModified());
        QVERIFY(s.window->title().startsWith(QStringLiteral("essai — ActCraft")));
        QVERIFY(run(s, QStringLiteral("file.close")));
        QCOMPARE(doc->pageCount(), 1);
        QSignalSpy loaded(doc, &ui::DocumentController::loadFinished);
        QVERIFY(run(s, QStringLiteral("file.open"), path));
        QVERIFY(loaded.wait(10000));
        QCOMPARE(doc->pageCount(), 2);
    }

    void modifiedDocumentAsksBeforeClosing()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(run(s, QStringLiteral("file.close")));
        // Un Dialog est un Popup, pas un QQuickItem : item() ne le voit pas (comme tst_ui_shell.cpp:551).
        QObject* dialog = s.window->findChild<QObject*>(QStringLiteral("saveChangesDialog"));
        QVERIFY(dialog);
        QTRY_VERIFY(dialog->property("visible").toBool());
        QCOMPARE(doc->pageCount(), 2); // rien n'est fermé avant la réponse
        QVERIFY(QMetaObject::invokeMethod(dialog, "discard"));
        QTRY_COMPARE(doc->pageCount(), 1);
        QVERIFY(!doc->isModified());
    }

    void cancelKeepsTheDocument()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(run(s, QStringLiteral("file.new")));
        QObject* dialog = s.window->findChild<QObject*>(QStringLiteral("saveChangesDialog"));
        QVERIFY(dialog);
        QTRY_VERIFY(dialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(dialog, "cancel"));
        QTRY_VERIFY(!dialog->property("visible").toBool());
        QCOMPARE(doc->pageCount(), 2);
        QVERIFY(doc->isModified());
    }

    void newSizesComeFromTheirCommands()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QVERIFY(run(s, QStringLiteral("file.new-1280x1024")));
        QCOMPARE(doc->pageSize(), QSizeF(1280, 1024));
        QVERIFY(run(s, QStringLiteral("file.new-1152x864")));
        QCOMPARE(doc->pageSize(), QSizeF(1152, 864));
        QVERIFY(run(s, QStringLiteral("file.new-screen")));
        QCOMPARE(doc->pageSize(), QSizeF(s.window->screen()->size()));
        QVERIFY(run(s, QStringLiteral("file.new-custom")));
        QObject* sizeDialog = s.window->findChild<QObject*>(QStringLiteral("pageSizeDialog"));
        QVERIFY(sizeDialog);
        QTRY_VERIFY(sizeDialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(sizeDialog, "acceptSize", Q_ARG(QVariant, 640), Q_ARG(QVariant, 480)));
        QTRY_COMPARE(doc->pageSize(), QSizeF(640, 480));
    }

    void bannerShowsReadingWarnings()
    {
        Shell s;
        QVERIFY(start(s));
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("cours.flipchart"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(fctest::sampleFlipchart()); // une formule non lue : un avertissement
        file.close();
        auto* doc = controller(s);
        QSignalSpy loaded(doc, &ui::DocumentController::loadFinished);
        QVERIFY(run(s, QStringLiteral("file.open"), path));
        QVERIFY(loaded.wait(10000));
        QQuickItem* banner = item(s, "messageBanner");
        QQuickItem* text = item(s, "messageText");
        QVERIFY(banner && text);
        QTRY_VERIFY(banner->isVisible());
        QVERIFY(text->property("text").toString().contains(QStringLiteral("1 avertissement")));
        // Contraste sur les couleurs RENDUES (leçon L1-C5) : fond lu dans la capture, texte lu sur l'élément.
        const QImage shot = s.window->grabWindow();
        // Fond lu à l'intérieur du bandeau, à mi-hauteur, entre le bord (1 px) et le texte (marge Theme.espace2).
        const QPointF inside = banner->mapToScene(QPointF(4, banner->height() / 2));
        const QColor background = shot.pixelColor(inside.toPoint());
        QVERIFY(qAbs(background.red() - banner->property("color").value<QColor>().red()) <= 2);
        const QColor ink = text->property("color").value<QColor>();
        const auto luminance = [](QColor c) {
            const auto channel = [](double v) {
                v /= 255.0;
                return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
            };
            return 0.2126 * channel(c.red()) + 0.7152 * channel(c.green()) + 0.0722 * channel(c.blue());
        };
        const double a = luminance(background);
        const double b = luminance(ink);
        const double ratio = (std::max(a, b) + 0.05) / (std::min(a, b) + 0.05);
        QVERIFY2(ratio >= 4.5, qPrintable(QString::number(ratio)));
        if (const QString out = qEnvironmentVariable("QT_ACTCRAFT_CAPTURES"); !out.isEmpty()) {
            shot.save(out + QStringLiteral("/bandeau.png"));
        }
        QVERIFY(QMetaObject::invokeMethod(banner, "dismiss"));
        QTRY_VERIFY(!banner->isVisible());
    }

    void cancelledSaveAsForgetsThePendingAction()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(run(s, QStringLiteral("file.close")));
        QObject* ask = s.window->findChild<QObject*>(QStringLiteral("saveChangesDialog"));
        QVERIFY(ask);
        QTRY_VERIFY(ask->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(ask, "save")); // document sans fichier → « Enregistrer sous »
        QObject* saveDialog = s.window->findChild<QObject*>(QStringLiteral("saveDialog"));
        QVERIFY(saveDialog);
        QVERIFY(QMetaObject::invokeMethod(saveDialog, "reject"));
        QObject* actions = s.window->findChild<QObject*>(QStringLiteral("fileActions"));
        QVERIFY(actions);
        QTRY_VERIFY(actions->property("afterSave").isNull() || !actions->property("afterSave").isValid());
        // Un enregistrement plus tard ne doit pas fermer le document (l'action abandonnée est oubliée).
        QTemporaryDir dir;
        QSignalSpy saved(doc, &ui::DocumentController::saveFinished);
        QVERIFY(run(s, QStringLiteral("file.save-as"), dir.filePath(QStringLiteral("plus-tard.actcraft"))));
        QVERIFY(saved.wait(10000));
        QCOMPARE(doc->pageCount(), 2);
    }

    void titleShowsTheModifiedMark()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(s.window->title().startsWith(QStringLiteral("Sans titre — ActCraft — rendu : ")));
        QVERIFY(run(s, QStringLiteral("page.next")));
        QTRY_VERIFY(s.window->title().startsWith(QStringLiteral("*Sans titre")));
    }
};

QTEST_MAIN(TstUiFileMenu)
#include "tst_ui_filemenu.moc"
```

  Includes à ajouter au harnais recopié : `"support/flipchartwriter.h"`, `<QFile>`, `<QScreen>`, `<cmath>`. Ajouter à
  `tests/ui.cmake` :

```cmake
actcraft_add_test(tst_ui_filemenu
    SOURCES tst_ui_filemenu.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick Qt6::Gui ActCraft::ui ActCraft::formats actcraft_uiplugin)
target_include_directories(tst_ui_filemenu PRIVATE ${PROJECT_SOURCE_DIR}/app)
```

  Dans `tests/tst_ui_shell.cpp`, la liste d'objets attendus qui contient `"menu-file"` ne change pas ; seul un test
  qui énumérerait les entrées du menu Fichier (aucun au lot 1, à vérifier par `grep file.quit`) serait à mettre à jour.

- [ ] **Étape 2 : constater le rouge.** Commande ciblée, motif `tst_ui_filemenu` → échecs (`saveChangesDialog`
  introuvable, entrées absentes).

- [ ] **Étape 3 : la disposition.** Dans `actions/data/layout.json`, le menu `file` devient :

```json
    { "id": "file", "label": "Fichier", "items": ["file.new", "file.new-screen", "file.new-1024x768", "file.new-1152x864", "file.new-1280x1024", "file.new-custom", "-", "file.open", "-", "file.save", "file.save-as", "-", "file.close", "-", "file.quit"] },
```

- [ ] **Étape 4 : les composants QML.** Créer `ui/qml/SaveChangesDialog.qml` :

```qml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// « Enregistrer les modifications ? » (F-700 : Fermer, Quitter) : Enregistrer, Ne pas enregistrer, Annuler.
Dialog {
    id: dialog
    objectName: "saveChangesDialog"
    property string documentTitle: ""
    signal saveRequested()
    signal discardRequested()
    title: qsTr("Enregistrer les modifications ?")
    modal: true
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.CloseOnEscape

    function discard() { dialog.close(); dialog.discardRequested() }
    function cancel() { dialog.reject() }
    function save() { dialog.close(); dialog.saveRequested() }

    Label {
        text: qsTr("Le paperboard « %1 » a été modifié. Enregistrer les modifications ?").arg(dialog.documentTitle)
        wrapMode: Text.WordWrap
        width: Math.min(420, implicitWidth)
    }
    footer: DialogButtonBox {
        Button { objectName: "saveChangesSave"; text: qsTr("Enregistrer"); onClicked: dialog.save() }
        Button { objectName: "saveChangesDiscard"; text: qsTr("Ne pas enregistrer"); onClicked: dialog.discard() }
        Button { objectName: "saveChangesCancel"; text: qsTr("Annuler"); onClicked: dialog.cancel() }
    }
}
```

  Créer `ui/qml/PageSizeDialog.qml` :

```qml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// « Taille de paperboard personnalisée » (F-700) : largeur × hauteur en pixels.
Dialog {
    id: dialog
    objectName: "pageSizeDialog"
    signal sizeAccepted(real width, real height)
    title: qsTr("Taille de paperboard personnalisée")
    modal: true
    anchors.centerIn: Overlay.overlay
    standardButtons: Dialog.Ok | Dialog.Cancel

    function acceptSize(w, h) { widthBox.value = w; heightBox.value = h; dialog.accept() }
    onAccepted: dialog.sizeAccepted(widthBox.value, heightBox.value)

    GridLayout {
        columns: 2
        Label { text: qsTr("Largeur (pixels)") }
        SpinBox { id: widthBox; objectName: "pageWidthBox"; from: 200; to: 8000; value: 1024; editable: true }
        Label { text: qsTr("Hauteur (pixels)") }
        SpinBox { id: heightBox; objectName: "pageHeightBox"; from: 200; to: 8000; value: 768; editable: true }
    }
}
```

  Créer `ui/qml/MessageBanner.qml` :

```qml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Avertissements de lecture et erreurs (décision 5) : affichés sans bloquer, refermables.
Rectangle {
    id: banner
    objectName: "messageBanner"
    property string message: ""
    visible: message.length > 0
    color: Theme.barre
    border.color: Theme.contour
    radius: Theme.rayon
    implicitHeight: row.implicitHeight + 2 * Theme.espace2

    function show(text) { banner.message = text }
    function dismiss() { banner.message = "" }

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: Theme.espace2
        Label {
            objectName: "messageText"
            Layout.fillWidth: true
            text: banner.message
            color: Theme.texte
            wrapMode: Text.WordWrap
            font.pixelSize: Theme.taillePolice
        }
        Button {
            objectName: "messageClose"
            text: qsTr("Fermer")
            onClicked: banner.dismiss()
        }
    }
}
```

  Créer `ui/qml/FileActions.qml` :

```qml
import QtQuick
import QtQuick.Dialogs
import ActCraft.Ui

// Commandes de fichier (F-700, décisions 20 et 22) : avec une URL, sans dialogue ; sans URL, le dialogue de fichier
// (natif ; content:// sous Android). Tout ce qui remplace un document modifié demande d'abord de l'enregistrer.
Item {
    id: actions
    objectName: "fileActions"
    required property DocumentController document
    signal popupOpened()
    signal popupClosed()
    property var pending: null    // action à reprendre après « enregistrer ? »
    property var afterSave: null  // action à reprendre après un enregistrement réussi
    property bool quitting: false

    function guard(action) {
        if (!actions.document.modified) { action(); return }
        actions.pending = action
        saveChanges.documentTitle = actions.document.title
        saveChanges.open()
    }
    function newDocument(w, h) { guard(function () { actions.document.newDocument(w, h) }) }
    function askSize() { sizeDialog.open() }
    function open(argument) {
        const url = actions.document.urlFromArgument(argument)
        if (url.toString() === "")
            guard(function () { openDialog.currentFolder = actions.document.defaultFolderUrl(); openDialog.open() })
        else
            guard(function () { actions.document.openUrl(url) })
    }
    function save() {
        if (actions.document.fileUrl.toString() === "") saveAs(undefined)
        else actions.document.saveToUrl(actions.document.fileUrl)
    }
    function saveAs(argument) {
        const url = actions.document.urlFromArgument(argument)
        if (url.toString() !== "") { actions.document.saveToUrl(url); return }
        saveDialog.currentFolder = actions.document.defaultFolderUrl()
        saveDialog.selectedFile = actions.document.defaultFolderUrl() + "/" + actions.document.suggestedFileName
        saveDialog.open()
    }
    function close() { guard(function () { actions.document.newDocument(1024, 768) }) }
    function quit() { guard(function () { actions.quitting = true; Qt.quit() }) }

    FileDialog {
        id: openDialog
        objectName: "openDialog"
        title: qsTr("Ouvrir un paperboard")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Paperboards (*.actcraft *.flipchart)"), qsTr("Tous les fichiers (*)")]
        onAccepted: actions.document.openUrl(selectedFile)
    }
    FileDialog {
        id: saveDialog
        objectName: "saveDialog"
        title: qsTr("Enregistrer le paperboard sous")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "actcraft"
        nameFilters: [qsTr("Paperboards ActCraft (*.actcraft)")]
        onAccepted: actions.document.saveToUrl(selectedFile)
        // « Enregistrer sous » abandonné après « Enregistrer les modifications ? » : l'action en attente est oubliée.
        onRejected: actions.afterSave = null
    }
    SaveChangesDialog {
        id: saveChanges
        onOpened: actions.popupOpened()
        onClosed: actions.popupClosed()
        onSaveRequested: { actions.afterSave = actions.pending; actions.pending = null; actions.save() }
        onDiscardRequested: { const a = actions.pending; actions.pending = null; if (a) a() }
        onRejected: actions.pending = null
    }
    PageSizeDialog {
        id: sizeDialog
        onOpened: actions.popupOpened()
        onClosed: actions.popupClosed()
        onSizeAccepted: (w, h) => actions.newDocument(w, h)
    }
    Connections {
        target: actions.document
        function onSaveFinished(ok, message) {
            const a = actions.afterSave
            actions.afterSave = null
            if (ok && a) a()
        }
    }
}
```

- [ ] **Étape 5 : brancher `Main.qml`.** Le titre devient :

```qml
    title: qsTr("%1%2 — ActCraft — rendu : %3").arg(appDocument.modified ? "*" : "").arg(appDocument.title)
               .arg(root.apiName(GraphicsInfo.api))
```

  Ajouter, après `AboutDialog` :

```qml
    FileActions {
        id: fileActions
        document: appDocument
        onPopupOpened: root.popupOpened()
        onPopupClosed: root.popupClosed()
    }

    // Fermeture de la fenêtre : même demande que « Quitter ».
    onClosing: close => {
        if (appDocument.modified && !fileActions.quitting) {
            close.accepted = false
            fileActions.quit()
        }
    }
```

  et, en tête de `PageArea` (à l'intérieur, en haut, au-dessus de la page), un bandeau :

```qml
        MessageBanner {
            id: messageBanner
            parent: pageArea
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Theme.espace2
            z: 10
        }
        Connections {
            target: appDocument
            function onLoadFinished(ok, message) {
                if (!ok) messageBanner.show(qsTr("Ouverture impossible : %1").arg(message))
                else if (appDocument.warnings.length === 1)
                    messageBanner.show(qsTr("1 avertissement de lecture : %1").arg(appDocument.warnings[0]))
                else if (appDocument.warnings.length > 1)
                    messageBanner.show(qsTr("%1 avertissements de lecture : %2").arg(appDocument.warnings.length)
                                       .arg(appDocument.warnings.slice(0, 3).join(" ; ")))
                else messageBanner.dismiss()
            }
            function onSaveFinished(ok, message) {
                if (!ok) messageBanner.show(qsTr("Enregistrement impossible : %1").arg(message))
            }
        }
```

  Dans `Component.onCompleted`, remplacer le gestionnaire de
  `file.quit` et ajouter :

```js
        appCommands.setHandler("file.quit", function () { fileActions.quit() })
        appCommands.setHandler("file.new", function () { fileActions.newDocument(1024, 768) })
        appCommands.setHandler("file.new-screen", function () { fileActions.newDocument(root.screen.width, root.screen.height) })
        appCommands.setHandler("file.new-1024x768", function () { fileActions.newDocument(1024, 768) })
        appCommands.setHandler("file.new-1152x864", function () { fileActions.newDocument(1152, 864) })
        appCommands.setHandler("file.new-1280x1024", function () { fileActions.newDocument(1280, 1024) })
        appCommands.setHandler("file.new-custom", function () { fileActions.askSize() })
        appCommands.setHandler("file.open", function (url) { fileActions.open(url) })
        appCommands.setHandler("file.save", function () { fileActions.save() })
        appCommands.setHandler("file.save-as", function (url) { fileActions.saveAs(url) })
        appCommands.setHandler("file.close", function () { fileActions.close() })
```

  Ajouter les quatre fichiers à `QML_FILES` de `ui/CMakeLists.txt`. Si MSVC lève C4702 sur l'un d'eux, l'ajouter à la
  liste `QT_QML_SKIP_CACHEGEN` (règle commune).

- [ ] **Étape 6 : vert ciblé.** Commande ciblée, motif `tst_ui_` : `100% tests passed`.

- [ ] **Étape 7 : preuve, capture et banc.** `format.ps1`, preuve de tâche (N ≥ 60), puis la capture :
  `pwsh -NoProfile -Command "Set-Location '<worktree>'; New-Item -ItemType Directory -Force .tmp/bandeau | Out-Null; . ./scripts/dev-env.ps1; `$env:QT_ACTCRAFT_CAPTURES = (Resolve-Path .tmp/bandeau).Path; ctest --preset debug -R '^tst_ui_filemenu$' *> .tmp/filemenu.txt; Select-String -Path .tmp/filemenu.txt -Pattern 'tests passed|tests failed' | Select-Object -Last 1"`
  puis lire `.tmp/bandeau/bandeau.png` et la décrire au rapport. Banc sans écran (`content_check.ok = True`).

- [ ] **Étape 8 : commit.**

```powershell
git -C <worktree> add ui/qml/Main.qml ui/qml/FileActions.qml ui/qml/SaveChangesDialog.qml ui/qml/PageSizeDialog.qml ui/qml/MessageBanner.qml ui/CMakeLists.txt actions/data/layout.json tests/ui.cmake tests/tst_ui_filemenu.cpp tests/tst_ui_shell.cpp
git -C <worktree> commit -m "Interface : menu Fichier, dialogues, demande d'enregistrement, bandeau d'avertissements" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

  (`tests/tst_ui_shell.cpp` n'est ajouté que s'il a changé.)


<a id="tache-l2-t19"></a>
### Tâche L2-T19 — Ouvrir le fichier passé au lancement (`actcraft.exe <fichier>`, `--ouvrir`)

**Objectif.** S4 (« `actcraft.exe "<samples>\<paperboard réel n° 16>"` ouvre le document ») et association de
fichiers de l'installateur (décision 26) : le premier argument qui n'est pas une option est un fichier à ouvrir ;
l'option `--ouvrir <fichier>` fait de même (outils : `actcraft-captures`, qui a ses propres options). Le fichier est
ouvert par la commande `file.open` avec son URL, une fois la fenêtre chargée (même chemin que le menu).

**Fichiers.**
- Créer : `tests/tst_app_open.cpp`
- Modifier : `app/startup.h`, `app/startup.cpp`, `app/main.cpp`, `tests/app.cmake`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_app_open` → `error`. Vert : forme bornée, N ≥ 61,
`TESTS OK`, `tst_app_open .*Passed`.

**Interdits.** Pas de `#ifdef` hors de `app/`. Les options connues (`--rendu`, `--quitter-apres`, `--sans-cache`,
`--version`, `--ouvrir`) ne sont jamais prises pour un fichier.

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests.** Créer `tests/tst_app_open.cpp` :

```cpp
#include "startup.h"

#include <actcraft/core/document.h>
#include <actcraft/formats/nativefile.h>
#include <actcraft/ui/documentcontroller.h>

#include <QFile>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

// Fichier ouvert au lancement (S4, décision 26).
class TstAppOpen : public QObject
{
    Q_OBJECT

private slots:
    void firstNonOptionArgumentIsTheFile()
    {
        QCOMPARE(openFileArgument({QStringLiteral("actcraft.exe"), QStringLiteral("C:/a/b.flipchart")}),
                 QStringLiteral("C:/a/b.flipchart"));
        QCOMPARE(openFileArgument({QStringLiteral("actcraft.exe"), QStringLiteral("--rendu"), QStringLiteral("d3d11"),
                                   QStringLiteral("x.actcraft")}),
                 QStringLiteral("x.actcraft"));
        QCOMPARE(openFileArgument({QStringLiteral("actcraft.exe"), QStringLiteral("--quitter-apres"),
                                   QStringLiteral("5000")}),
                 QString());
        QCOMPARE(openFileArgument({QStringLiteral("x"), QStringLiteral("--ouvrir"), QStringLiteral("y.actcraft")}),
                 QStringLiteral("y.actcraft"));
        QVERIFY(parseStartupOptions({QStringLiteral("x"), QStringLiteral("--ouvrir"), QStringLiteral("y.actcraft")})
                    .openUrl.isLocalFile());
    }

    void startupOpensTheFile()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("trois.actcraft"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(formats::encodeActcraft(core::Document(std::vector<core::Page>(3)), {}));
        file.close();
        StartupOptions options;
        options.openUrl = QUrl::fromLocalFile(path);
        QQmlApplicationEngine engine;
        QQuickWindow* window = loadMainWindow(engine, options);
        QVERIFY(window);
        auto* doc = window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        QVERIFY(doc);
        QTRY_COMPARE_WITH_TIMEOUT(doc->pageCount(), 3, 10000);
        QCOMPARE(doc->title(), QStringLiteral("trois"));
    }
};

QTEST_MAIN(TstAppOpen)
#include "tst_app_open.moc"
```

  Ajouter à `tests/app.cmake` :

```cmake
actcraft_add_test(tst_app_open
    SOURCES tst_app_open.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui ActCraft::formats actcraft_uiplugin)
target_include_directories(tst_app_open PRIVATE ${PROJECT_SOURCE_DIR}/app)
```

- [ ] **Étape 2 : constater le rouge.** Commande ciblée, motif `tst_app_open` → `error`.

- [ ] **Étape 3 : écrire le code.** Dans `app/startup.h` : include `<QUrl>` ; champ `QUrl openUrl;` dans
  `StartupOptions` (« fichier à ouvrir au lancement, vide sinon ») ; déclaration
  `QString openFileArgument(const QStringList& args);` (« `--ouvrir <f>`, sinon premier argument qui n'est ni une
  option ni la valeur d'une option ; vide sinon »). Dans `app/startup.cpp` :

```cpp
QString openFileArgument(const QStringList& args)
{
    const qsizetype explicitIndex = args.indexOf(QStringLiteral("--ouvrir"));
    if (explicitIndex >= 0 && explicitIndex + 1 < args.size()) {
        return args.at(explicitIndex + 1);
    }
    static const QStringList withValue{QStringLiteral("--rendu"), QStringLiteral("--quitter-apres"),
                                       QStringLiteral("--ouvrir")};
    for (qsizetype i = 1; i < args.size(); ++i) {
        const QString& a = args.at(i);
        if (withValue.contains(a)) {
            ++i; // sa valeur n'est pas un fichier
            continue;
        }
        if (!a.startsWith(QLatin1String("--"))) {
            return a;
        }
    }
    return QString();
}
```

  `parseStartupOptions` lit seulement `--ouvrir` (les outils ont leurs propres arguments positionnels) :

```cpp
    const qsizetype at = args.indexOf(QStringLiteral("--ouvrir"));
    if (at >= 0 && at + 1 < args.size()) {
        options.openUrl = actcraft::formats::urlFromArgument(args.at(at + 1));
    }
```

  (include `<actcraft/formats/urlio.h>`), et `loadMainWindow`, après le chargement :

```cpp
    if (window && options.openUrl.isValid() && !options.openUrl.isEmpty()) {
        QMetaObject::invokeMethod(window, "runCommand", Q_ARG(QVariant, QVariant(QStringLiteral("file.open"))),
                                  Q_ARG(QVariant, QVariant(options.openUrl)));
    }
```

  (`runCommand` rend une valeur : ajouter `Q_RETURN_ARG(QVariant, result)` avec un `QVariant result;` local, comme le
  joueur de scénarios.) Dans `app/main.cpp`, après `parseStartupOptions(args)`, si `openUrl` est vide,
  `openFileArgument(args)` non vide → `options.openUrl = actcraft::formats::urlFromArgument(<valeur>)` (l'application
  accepte le fichier nu, « Ouvrir avec »). `app/CMakeLists.txt` lie déjà `ui`, qui lie `formats` (L2-T17) : rien à
  ajouter ; sinon l'ajouter et le déclarer au rapport.

- [ ] **Étape 4 : vert ciblé, preuve.** Commande ciblée (`tst_app_`) : `100% tests passed`. `format.ps1`, preuve
  (N ≥ 61 ; `tst_app_open` Passed).

- [ ] **Étape 5 : commit.**

```powershell
git -C <worktree> add app/startup.h app/startup.cpp app/main.cpp tests/app.cmake tests/tst_app_open.cpp
git -C <worktree> commit -m "app : ouvrir le fichier passé au lancement (Ouvrir avec, --ouvrir)" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t20"></a>
### Tâche L2-T20 — Joueur de scénarios : fixtures, variables, attente, sonde de boucle ; scénario `aller-retour-natif` (S5)

**Objectif.** S5 joué sans souris (DA8) : le joueur d'`actcraft-captures` gagne six étapes, **ajoutées à la fin** de
`UiStepKind` (contrat du lot 1 : on ajoute, on ne renomme pas) :

| Étape JSON | Effet | Ligne de rapport |
|---|---|---|
| `{"fixture": "trois-pages", "path": "${SORTIE}/source.actcraft"}` | écrit un document de scénario (`buildFixture`) | `FIXTURE trois-pages <chemin> OK` |
| `{"waitFor": "objet.propriété", "value": …, "timeoutMs": 10000}` | attend que la propriété vaille la valeur | `ATTENTE objet.propriété=valeur OK` / `ECHEC` |
| `{"watchEventLoop": true}` | démarre la sonde : minuterie de 5 ms sur le fil principal, plus long intervalle mesuré | `BOUCLE sonde démarrée` |
| `{"maxEventLoopGapMs": 25}` | arrête la sonde, compare le plus long intervalle | `BOUCLE max=<x> ms OK` / `ECART` |
| `{"remember": "objet.propriété", "as": "nom"}` | mémorise une valeur pour `${nom}` | `MEMO nom=valeur` |
| `{"archive": "<chemin>", "contains": ["document.json", …]}` | ouvre l'archive (`formats::archiveEntryNames`), vérifie les entrées (préfixe accepté) | `ARCHIVE <chemin> OK` / `ECART (manque …)` |

Variables remplacées dans les arguments textuels, les chemins et les valeurs attendues : `${SORTIE}` (dossier de
sortie du scénario), `${SAMPLES}` (`ACTCRAFT_SAMPLES_DIR` ; étape en `ECHEC` si absente), `${<nom>}` (mémorisé).
Fixture `trois-pages` : page 1 vide (le trait y sera ajouté au stylet injecté), page 2 un rectangle bleu et un texte
« Bonjour », page 3 une image (PNG 64 × 64 rouge et blanc produit par Qt, ressource). Scénario
`bench/scenarios/aller-retour-natif.json` (S5) et test ctest `tst_captures_aller_retour_natif`.

**Fichiers.**
- Créer : `bench/include/actcraft/bench/fixtures.h`, `bench/src/fixtures.cpp`,
  `bench/scenarios/aller-retour-natif.json`
- Modifier : `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`, `bench/CMakeLists.txt`,
  `tests/bench.cmake`, `tests/tst_bench_uiscenario.cpp`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_bench_uiscenario` → échecs des nouveaux cas
d'analyse. Vert : forme bornée, N ≥ 61, `TESTS OK`, `tst_bench_uiscenario .*Passed`,
`tst_captures_aller_retour_natif .*Passed`. Preuve visuelle :

```powershell
pwsh -NoProfile -Command "Set-Location '<worktree>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario aller-retour-natif -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire 'pageIndicator.text=1 / 3' > .tmp/captures.txt 2>&1; Select-String -Path .tmp/captures.txt -Pattern '^(COMMANDE|TRAIT|ATTENTE|FIXTURE|BOUCLE|MEMO|ARCHIVE|LU|PIXEL|CAPTURE) |^SCENARIO |CAPTURES (OK|EN ECHEC)' | Select-Object -Last 16"
```

Attendu : `BOUCLE max=<x> ms OK` (x ≤ 25), `LU documentController.lastIoOffMainThread=true OK` (deux fois :
écriture et relecture faites hors du fil principal, relevé par elles-mêmes, L2-T17), `ARCHIVE … OK`,
`LU documentController.fingerprint=<mémorisée> OK`,
`LU pageIndicator.text=1 / 3 OK`, `SCENARIO OK`, `CAPTURES OK` ; décrire les captures `relu-page-1` (trait noir),
`relu-page-2` (rectangle bleu, « Bonjour ») et `relu-page-3` (image rouge et blanche).

**Interdits.** Aucun binaire commité (la fixture est écrite à chaque passage, sous la sortie du scénario). Aucune
attente à durée fixe hors de l'étape `wait`. Touches : seulement par l'étape `key` existante.

**Terminé quand.** Preuve verte, captures décrites, commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests d'analyse.** Dans `tests/tst_bench_uiscenario.cpp`, ajouter :

```cpp
    void newStepsAreParsed()
    {
        UiScenario s;
        QString error;
        QVERIFY2(parseUiScenario(R"({"format":"actcraft-scenario/1","name":"x","steps":[
            {"fixture":"trois-pages","path":"${SORTIE}/a.actcraft"},
            {"waitFor":"documentController.busy","value":false,"timeoutMs":5000},
            {"watchEventLoop":true},
            {"maxEventLoopGapMs":25},
            {"remember":"documentController.fingerprint","as":"avant"},
            {"archive":"${SORTIE}/a.actcraft","contains":["document.json","thumbnails/"]}]})",
                                 s, error),
                 qPrintable(error));
        QCOMPARE(s.steps.size(), std::size_t{6});
        QVERIFY(s.steps[0].kind == UiStepKind::Fixture);
        QCOMPARE(s.steps[0].name, QStringLiteral("trois-pages"));
        QVERIFY(s.steps[1].kind == UiStepKind::WaitFor);
        QCOMPARE(s.steps[1].ms, 5000);
        QVERIFY(s.steps[2].kind == UiStepKind::WatchEventLoop);
        QVERIFY(s.steps[3].kind == UiStepKind::MaxEventLoopGap);
        QCOMPARE(s.steps[3].ms, 25);
        QVERIFY(s.steps[4].kind == UiStepKind::Remember);
        QCOMPARE(s.steps[4].name, QStringLiteral("avant"));
        QVERIFY(s.steps[5].kind == UiStepKind::ArchiveContains);
        QCOMPARE(s.steps[5].values, (QStringList{QStringLiteral("document.json"), QStringLiteral("thumbnails/")}));
    }

    void badNewStepsAreRefused_data()
    {
        QTest::addColumn<QByteArray>("step");
        QTest::newRow("fixture sans chemin") << QByteArray(R"({"fixture":"trois-pages"})");
        QTest::newRow("fixture inconnue") << QByteArray(R"({"fixture":"inconnue","path":"x"})");
        QTest::newRow("waitFor sans valeur") << QByteArray(R"({"waitFor":"a.b"})");
        QTest::newRow("remember sans nom") << QByteArray(R"({"remember":"a.b"})");
        QTest::newRow("archive sans liste") << QByteArray(R"({"archive":"x"})");
    }

    void badNewStepsAreRefused()
    {
        QFETCH(QByteArray, step);
        UiScenario s;
        QString error;
        QVERIFY(!parseUiScenario(R"({"format":"actcraft-scenario/1","name":"x","steps":[)" + step + "]}", s, error));
        QVERIFY(error.startsWith(QStringLiteral("étape 1")));
    }

    void fixtureWritesAThreePageDocument()
    {
        QTemporaryDir dir;
        QString error;
        const QString path = dir.filePath(QStringLiteral("f.actcraft"));
        QVERIFY2(buildFixture(QStringLiteral("trois-pages"), path, error), qPrintable(error));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const formats::DecodeResult r = formats::decodeActcraft(file.readAll());
        QVERIFY(r.ok);
        QCOMPARE(r.document.pageCount(), std::size_t{3});
        QCOMPARE(r.document.resources().size(), std::size_t{1});
        QVERIFY(!buildFixture(QStringLiteral("inconnue"), path, error));
    }
```

  et un cas joué sur la fenêtre d'essai du fichier (`tests/data/uiscenario/fenetre.qml`) : `rememberThenExpect` —
  `{"remember":"window.width","as":"l"}` puis `{"expect":"window.width","value":"${l}"}` → `MEMO l=…` puis
  `LU window.width=… OK` ; et `eventLoopProbeMeasures` — `watchEventLoop`, `wait` 100, `maxEventLoopGapMs` 1000 →
  ligne `BOUCLE max=` suivie d'un nombre, `OK`. Includes : `<actcraft/bench/fixtures.h>`,
  `<actcraft/formats/nativefile.h>`, `<QTemporaryDir>`, `<QFile>`.

- [ ] **Étape 2 : constater le rouge.** Commande ciblée, motif `tst_bench_uiscenario` → `error`
  (`UiStepKind::Fixture` inconnu).

- [ ] **Étape 3 : les fixtures.** Créer `bench/include/actcraft/bench/fixtures.h` :

```cpp
#pragma once

#include <QString>

namespace actcraft::bench {

// Documents de scénario (DA8), écrits avant d'être ouverts : aucun binaire commité. « trois-pages » (S5) : page 1
// vide, page 2 rectangle bleu et texte « Bonjour », page 3 image rouge et blanche. Faux et `error` si le nom est
// inconnu ou l'écriture impossible.
bool buildFixture(const QString& name, const QString& path, QString& error);

} // namespace actcraft::bench
```

  Créer `bench/src/fixtures.cpp` :

```cpp
#include <actcraft/bench/fixtures.h>
#include <actcraft/core/document.h>
#include <actcraft/formats/nativefile.h>
#include <actcraft/formats/urlio.h>

#include <QBuffer>
#include <QImage>
#include <QPainter>
#include <QUrl>

namespace actcraft::bench {

namespace {

QByteArray redAndWhitePng()
{
    QImage image(64, 64, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter p(&image);
    p.fillRect(QRect(0, 0, 32, 64), Qt::red);
    p.end();
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return png;
}

core::Document threePages()
{
    std::vector<core::Page> pages(3);
    core::Shape rect;
    rect.kind = core::ShapeKind::Rectangle;
    rect.bounds = {200, 200, 300, 200};
    rect.fill = 0xFF1E88E5u;
    core::PageObject shape = rect;
    shape.common.id = 1;
    pages[1].objects.push_back(shape);
    core::Text text;
    text.bounds = {600, 200, 300, 80};
    text.html = "<p style=\"font-size:48px; color:#000000\">Bonjour</p>";
    core::PageObject textObject = text;
    textObject.common.id = 2;
    pages[1].objects.push_back(textObject);
    const QByteArray png = redAndWhitePng();
    core::ImageObject image;
    image.bounds = {0, 0, 64, 64};
    image.sourceWidth = 64;
    image.sourceHeight = 64;
    image.resource = core::sha256Hex(png);
    core::PageObject imageObject = image;
    imageObject.common.id = 3;
    imageObject.common.transform = core::compose(core::scaling(4, 4), core::translation(300, 200));
    pages[2].objects.push_back(imageObject);
    core::Document document(std::move(pages));
    document.addResource(png, "png");
    document.setTitle("Trois pages");
    return document;
}

} // namespace

bool buildFixture(const QString& name, const QString& path, QString& error)
{
    core::Document document;
    if (name == QLatin1String("trois-pages")) {
        document = threePages();
    } else {
        error = QStringLiteral("fixture inconnue « %1 »").arg(name);
        return false;
    }
    const QByteArray archive = formats::encodeActcraft(document, {}, &error);
    return !archive.isEmpty() && formats::writeUrl(QUrl::fromLocalFile(path), archive, &error);
}

} // namespace actcraft::bench
```

  Dans `bench/CMakeLists.txt`, ajouter `include/actcraft/bench/fixtures.h` et `src/fixtures.cpp` aux `SOURCES` de
  `actcraft_benchui`, et `ActCraft::formats` à ses `DEPENDS` (le module `benchui` n'est jamais livré).

- [ ] **Étape 4 : les étapes du joueur.** Dans `uiscenario.h` : ajouter à la fin de `UiStepKind`
  `Fixture, WaitFor, WatchEventLoop, MaxEventLoopGap, Remember, ArchiveContains` (commentaires JSON comme les autres) ;
  à `UiStep`, `QString path;` et `QStringList values;` ; à `UiScenarioPlayer`, membres privés
  `QHash<QString, QString> m_memory;`, `QElapsedTimer m_loopClock;`, `QTimer* m_loopTimer = nullptr;`,
  `qint64 m_loopLast = 0;`, `qint64 m_loopMax = 0;` et la méthode `QString expand(const QString& text) const;`.
  Mettre à jour le commentaire de classe (liste des lignes de rapport). Dans `uiscenario.cpp`, `parseStep` gagne :

```cpp
    if (o.contains(QStringLiteral("fixture"))) {
        step.kind = UiStepKind::Fixture;
        step.name = o.value(QStringLiteral("fixture")).toString();
        step.path = o.value(QStringLiteral("path")).toString();
        if (step.path.isEmpty() || step.name != QLatin1String("trois-pages")) {
            why = QStringLiteral("fixture invalide (nom connu et path)");
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("waitFor"))) {
        step.kind = UiStepKind::WaitFor;
        step.ms = o.value(QStringLiteral("timeoutMs")).toInt(10000);
        if (!splitTarget(o.value(QStringLiteral("waitFor")).toString(), step.target, step.property) ||
            !o.contains(QStringLiteral("value"))) {
            why = QStringLiteral("attente invalide (waitFor « objet.propriété », value)");
            return false;
        }
        step.value = jsonText(o.value(QStringLiteral("value")));
        return true;
    }
    if (o.contains(QStringLiteral("watchEventLoop"))) {
        step.kind = UiStepKind::WatchEventLoop;
        return true;
    }
    if (o.contains(QStringLiteral("maxEventLoopGapMs"))) {
        step.kind = UiStepKind::MaxEventLoopGap;
        step.ms = o.value(QStringLiteral("maxEventLoopGapMs")).toInt(-1);
        if (step.ms <= 0) {
            why = QStringLiteral("seuil de boucle invalide");
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("remember"))) {
        step.kind = UiStepKind::Remember;
        step.name = o.value(QStringLiteral("as")).toString();
        if (!splitTarget(o.value(QStringLiteral("remember")).toString(), step.target, step.property) ||
            step.name.isEmpty()) {
            why = QStringLiteral("mémoire invalide (remember « objet.propriété », as)");
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("archive"))) {
        step.kind = UiStepKind::ArchiveContains;
        step.path = o.value(QStringLiteral("archive")).toString();
        for (const QJsonValue& v : o.value(QStringLiteral("contains")).toArray()) {
            step.values << v.toString();
        }
        if (step.path.isEmpty() || step.values.isEmpty()) {
            why = QStringLiteral("archive invalide (archive, contains)");
            return false;
        }
        return true;
    }
```

  (avant la ligne `why = QStringLiteral("étape inconnue");`). `expand` remplace `${SORTIE}` par `m_outputDir`,
  `${SAMPLES}` par `qEnvironmentVariable("ACTCRAFT_SAMPLES_DIR")` (vide → l'étape échoue avec
  `ECHEC (ACTCRAFT_SAMPLES_DIR absent)`), puis chaque `${nom}` de `m_memory`. `runStep` l'applique à l'argument textuel
  d'une commande (`step.argument` de type chaîne), à `step.path` et à `step.value` (étapes `Expect` et `WaitFor`), et
  traite les nouvelles étapes :

```cpp
    case UiStepKind::Fixture: {
        QString error;
        const QString path = expand(step.path);
        const bool ok = buildFixture(step.name, path, error);
        report << (ok ? QStringLiteral("FIXTURE %1 %2 OK").arg(step.name, path)
                      : QStringLiteral("FIXTURE %1 ECHEC (%2)").arg(step.name, error));
        return ok;
    }
    case UiStepKind::WaitFor: {
        QObject* target = find(step.target);
        const QString expected = expand(step.value);
        const bool ok = target && waitUntil([&] {
            return propertyText(target->property(step.property.toUtf8().constData())) == expected;
        }, step.ms);
        report << QStringLiteral("ATTENTE %1.%2=%3 %4")
                      .arg(step.target, step.property, expected, ok ? QStringLiteral("OK") : QStringLiteral("ECHEC"));
        return ok;
    }
    case UiStepKind::WatchEventLoop: {
        delete m_loopTimer;
        m_loopTimer = new QTimer;
        m_loopTimer->setInterval(5);
        m_loopClock.start();
        m_loopLast = 0;
        m_loopMax = 0;
        QObject::connect(m_loopTimer, &QTimer::timeout, m_loopTimer, [this] {
            const qint64 now = m_loopClock.elapsed();
            m_loopMax = std::max(m_loopMax, now - m_loopLast);
            m_loopLast = now;
        });
        m_loopTimer->start();
        report << QStringLiteral("BOUCLE sonde démarrée");
        return true;
    }
    case UiStepKind::MaxEventLoopGap: {
        if (!m_loopTimer) {
            report << QStringLiteral("BOUCLE ECHEC (sonde non démarrée)");
            return false;
        }
        m_loopTimer->stop();
        const qint64 tail = m_loopClock.elapsed() - m_loopLast;
        m_loopMax = std::max(m_loopMax, tail);
        delete m_loopTimer;
        m_loopTimer = nullptr;
        const bool ok = m_loopMax <= step.ms;
        report << QStringLiteral("BOUCLE max=%1 ms %2")
                      .arg(m_loopMax)
                      .arg(ok ? QStringLiteral("OK") : QStringLiteral("ECART (seuil %1 ms)").arg(step.ms));
        return ok;
    }
    case UiStepKind::Remember: {
        QObject* target = find(step.target);
        if (!target) {
            report << QStringLiteral("MEMO %1 ECHEC (objet %2 absent)").arg(step.name, step.target);
            return false;
        }
        const QString value = propertyText(target->property(step.property.toUtf8().constData()));
        m_memory.insert(step.name, value);
        report << QStringLiteral("MEMO %1=%2").arg(step.name, value);
        return true;
    }
    case UiStepKind::ArchiveContains: {
        const QString path = expand(step.path);
        QFile file(path);
        const QStringList names = file.open(QIODevice::ReadOnly) ? formats::archiveEntryNames(file.readAll())
                                                                  : QStringList();
        QStringList missing;
        for (const QString& wanted : step.values) {
            if (std::none_of(names.cbegin(), names.cend(),
                             [&wanted](const QString& n) { return n.startsWith(wanted); })) {
                missing << wanted;
            }
        }
        report << (missing.isEmpty() ? QStringLiteral("ARCHIVE %1 OK").arg(path)
                                     : QStringLiteral("ARCHIVE %1 ECART (manque %2)").arg(path, missing.join(u", ")));
        return missing.isEmpty();
    }
```

  `UiScenarioPlayer::~UiScenarioPlayer` détruit `m_loopTimer`. Le minuteur mesure les intervalles **entre deux tours de
  la boucle du fil principal** : un enregistrement fait sur le fil principal les allongerait au-delà du seuil (écart 1
  de ce plan). Pour l'étape `Expect`, appliquer `expand` à `step.value` avant la comparaison.

- [ ] **Étape 5 : le scénario S5.** Créer `bench/scenarios/aller-retour-natif.json` :

```json
{
  "format": "actcraft-scenario/1",
  "name": "aller-retour-natif",
  "description": "S5 : un document de 3 pages (trait, forme, texte, image) enregistré en .actcraft hors du fil principal, fermé, rouvert : même empreinte ; l'archive contient document.json, resources/, thumbnails/.",
  "steps": [
    {"waitReady": true},
    {"fixture": "trois-pages", "path": "${SORTIE}/source.actcraft"},
    {"command": "file.open", "argument": "${SORTIE}/source.actcraft"},
    {"waitFor": "documentController.busy", "value": false},
    {"expect": "pageIndicator.text", "value": "1 / 3"},
    {"waitReady": true},
    {"stroke": [[200, 600], [600, 600]], "pressure": 0.5},
    {"waitReady": true},
    {"expect": "documentController.modified", "value": true},
    {"watchEventLoop": true},
    {"command": "file.save-as", "argument": "${SORTIE}/copie.actcraft"},
    {"waitFor": "documentController.busy", "value": false},
    {"maxEventLoopGapMs": 25},
    {"expect": "documentController.lastIoOffMainThread", "value": true},
    {"expect": "documentController.modified", "value": false},
    {"remember": "documentController.fingerprint", "as": "avant"},
    {"archive": "${SORTIE}/copie.actcraft", "contains": ["document.json", "resources/", "thumbnails/page-1.png", "thumbnails/page-3.png"]},
    {"command": "file.close"},
    {"waitReady": true},
    {"expect": "pageIndicator.text", "value": "1 / 1"},
    {"command": "file.open", "argument": "${SORTIE}/copie.actcraft"},
    {"waitFor": "documentController.busy", "value": false},
    {"expect": "documentController.fingerprint", "value": "${avant}"},
    {"expect": "documentController.lastIoOffMainThread", "value": true},
    {"expect": "pageIndicator.text", "value": "1 / 3"},
    {"waitReady": true},
    {"checkPixel": "trait-relu", "at": [400, 600], "color": "#000000", "tolerance": 60},
    {"capture": "relu-page-1"},
    {"command": "page.next"},
    {"waitReady": true},
    {"checkPixel": "rectangle-relu", "at": [350, 300], "color": "#1E88E5", "tolerance": 30},
    {"capture": "relu-page-2"},
    {"command": "page.next"},
    {"waitReady": true},
    {"checkPixel": "image-relue", "at": [340, 300], "color": "#FF0000", "tolerance": 40},
    {"capture": "relu-page-3"}
  ]
}
```

  (Le trait du stylo est noir par défaut ; l'image de 64 px agrandie 4 fois en (300, 200) est rouge sur sa moitié
  gauche : (340, 300) y tombe.) Ajouter à `tests/bench.cmake`, sur le modèle de `tst_captures_premiers_pas` :

```cmake
# Scénario d'acceptation S5 (aller-retour natif), hors écran en rendu logiciel.
add_test(NAME tst_captures_aller_retour_natif
    COMMAND $<TARGET_FILE:actcraft-captures>
        --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/aller-retour-natif.json
        --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-aller-retour-natif)
set_tests_properties(tst_captures_aller_retour_natif PROPERTIES
    TIMEOUT 180
    ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "SCENARIO OK"
    FAIL_REGULAR_EXPRESSION "ECART|SCENARIO EN ECHEC|SCENARIO INVALIDE")
```

- [ ] **Étape 6 : vert ciblé.** Commande ciblée, motif `tst_bench_uiscenario|tst_captures_` : `100% tests passed`. Si
  `BOUCLE` dépasse 25 ms en debug hors écran, rejouer en release (`-Preset release`, mesures de fluidité en release
  seulement, profil) ; un dépassement en release est un défaut à rapporter (enregistrement pas assez déporté), pas un
  seuil à relever.

- [ ] **Étape 7 : preuve et captures.** `format.ps1`, preuve de tâche (N ≥ 61), commande de preuve visuelle, capture
  décrite.

- [ ] **Étape 8 : commit.**

```powershell
git -C <worktree> add bench/include/actcraft/bench/uiscenario.h bench/src/uiscenario.cpp bench/include/actcraft/bench/fixtures.h bench/src/fixtures.cpp bench/CMakeLists.txt bench/scenarios/aller-retour-natif.json tests/bench.cmake tests/tst_bench_uiscenario.cpp
git -C <worktree> commit -m "Captures : fixtures, attente, sonde de boucle, mémoire, archive ; scénario S5 aller-retour natif" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t21"></a>
### Tâche L2-T21 — S4 : miniatures comparées aux `.thm`, scénarios `ouvrir-flipchart-synthetique` (CI) et `local/ouvrir-flipchart`

**Objectif.** S4 : le rendu de chaque page d'un `.flipchart`, réduit à 80 × 60, est comparé à sa miniature `.thm`
d'origine ; **écart médian ≤ 2,0 sur 255** et au plus 3 pages au-dessus de 7 (comme la référence,
`format-flipchart.md` § 9). Mesure écrite par nous (écart 3 du plan) : les deux images converties en RGB 8 bits,
ramenées à 80 × 60 par `QImage::scaled(…, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)`, écart = moyenne des
|a − b| sur les trois canaux et tous les pixels ; rendu par `render::renderPage` (peintre partagé, images décodées par
`decodeImages`). L'outil `actcraft-flipchart-dump` gagne :

- `--miniatures <fichier>` : compare les pages d'un paperboard ; lignes `PAGE <n> écart <e>` puis
  `MINIATURES OK : <n> pages, médiane <m>, <k> au-dessus de 7` (code 0 si médiane ≤ 2,0 et k ≤ 3) ou
  `MINIATURES EN ECHEC : …` (code 1) ; pages sans `.thm` ignorées et comptées à part ;
- `--connues "<fichier>:<page>;…"` (page `*` = une page quelconque du fichier) : le cadrage exige que les pages
  au-dessus de 7 soient **les** 3 pages déjà connues (`cadrage` l. 252-253, `format-flipchart.md` § 8-9 : deux à
  miniature présumée périmée, le paperboard réel n° 4 p2 et une page du n° 9, plus le n° 15 p5) ; toute page
  au-dessus de 7 hors de cette liste → `MINIATURES EN ECHEC : page <fichier>:<n> au-dessus de 7 et non connue`. La
  liste ne vit que dans la commande locale et dans la variable `ACTCRAFT_THM_CONNUES` lue par le test local
  (aucun nom de paperboard réel dans le code ni dans `tests/`). **Sans `--connues`** (ou liste vide) : aucune page au-
  dessus de 7 n'est tenue pour connue, le verdict n'exige que médiane ≤ 2,0 et k ≤ 3, et la ligne finale le dit
  (`… , liste des pages connues absente`) — c'est le cas en CI, où le test est de toute façon sauté (77). Poser
  `ACTCRAFT_THM_CONNUES` comme l'argument de la commande locale avant de configurer pour le test local ;
- `--samples <dossier>` : même chose sur tous les `.flipchart` du dossier (124 pages attendues), code **77** si le
  dossier est absent ou vide (test sauté) ;
- `--ecrire-synthetique <fichier>` : écrit `fctest::sampleFlipchart` avec des miniatures rendues par notre peintre
  (JPEG 320 × 240), `SYNTHETIQUE OK : <fichier>` — chemin prouvé en CI (fidélité seulement en local).

Le joueur gagne la fixture `flipchart-synthetique` (même paperboard, pour le scénario CI). Scénarios :
`bench/scenarios/ouvrir-flipchart-synthetique.json` (CI : ouvre la fixture, « 1 / 2 », bandeau d'un avertissement,
pixels du fond dégradé et de la forme verte, capture) et `bench/scenarios/local/ouvrir-flipchart.json` (local : la
fenêtre est lancée avec `--ouvrir "<samples>\<paperboard réel n° 16>"`, l'indicateur montre « 1 / N » avec N relevé
par L2-T10, capture de chaque page). `captures.ps1` gagne `-Ouvrir <fichier>` (passé en `--ouvrir`).

**Fichiers.**
- Créer : `bench/scenarios/ouvrir-flipchart-synthetique.json`, `bench/scenarios/local/ouvrir-flipchart.json`
- Modifier : `bench/flipchart/main.cpp`, `bench/CMakeLists.txt`, `bench/src/fixtures.cpp`,
  `bench/src/uiscenario.cpp` (nom de fixture accepté par `parseStep`), `scripts/captures.ps1`, `tests/bench.cmake`

**Test et commande bornée.** Vert : forme bornée, N ≥ 66, `TESTS OK`, `tst_flipchart_miniatures_synthetique .*Passed`,
`tst_captures_ouvrir_flipchart_synthetique .*Passed` (`tst_flipchart_miniatures_samples` : « Skipped » en CI, Passed
en local). Local, S4 complet :

```powershell
pwsh -NoProfile -Command "Set-Location '<worktree>'; . ./scripts/dev-env.ps1; ./build/debug/actcraft-flipchart-dump.exe --samples 'C:\Users\JeromeDenis\REA - Activinspire\samples' --connues \"<liste lue hors dépôt>\" > .tmp/miniatures.txt 2>&1; Select-String -Path .tmp/miniatures.txt -Pattern 'MINIATURES (OK|EN ECHEC)' | Select-Object -Last 1"
pwsh -NoProfile -Command "Set-Location '<worktree>'; pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario local/ouvrir-flipchart -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Ouvrir '<samples>\<paperboard réel n° 16>' > .tmp/captures.txt 2>&1; Select-String -Path .tmp/captures.txt -Pattern '^(LU|PIXEL|CAPTURE) |^SCENARIO |CAPTURES (OK|EN ECHEC)' | Select-Object -Last 16"
```

Attendu : `MINIATURES OK : 124 pages, médiane <m ≤ 2,0>, <k ≤ 3> au-dessus de 7` (toutes dans la liste `--connues`,
pages fautives nommées sinon) ; `LU pageIndicator.text=1 / N OK`,
`CAPTURES OK`. Décrire deux captures (page 1, une page à images).

**Interdits.** Aucun octet de `samples/` dans le dépôt, les tests CI ou les captures commitées (sorties sous `.tmp/`).
Ne pas relever les seuils : une médiane au-dessus de 2,0 se corrige dans le peintre (L2-T11) ou le lecteur, par un
correctif que l'orchestrateur ouvre — la tâche rapporte les pages fautives (numéro, écart, capture côte à côte sous
`.tmp/`).

**Terminé quand.** Preuve verte en CI et en local (ou écarts rapportés), commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests ctest (rouges).** Ajouter à `tests/bench.cmake` :

```cmake
# S4 en CI (paperboard synthétique, miniatures rendues par notre peintre) et en local (samples/, sauté sinon).
set(_flipchart_synthetic ${CMAKE_BINARY_DIR}/flipchart-synthetique.flipchart)
add_test(NAME tst_flipchart_ecrire_synthetique
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --ecrire-synthetique ${_flipchart_synthetic})
set_tests_properties(tst_flipchart_ecrire_synthetique PROPERTIES
    FIXTURES_SETUP flipchart_synthetic ENVIRONMENT "${_bench_env_offscreen}" PASS_REGULAR_EXPRESSION "SYNTHETIQUE OK")
add_test(NAME tst_flipchart_miniatures_synthetique
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --miniatures ${_flipchart_synthetic})
set_tests_properties(tst_flipchart_miniatures_synthetique PROPERTIES
    FIXTURES_REQUIRED flipchart_synthetic ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "MINIATURES OK : 2 pages")
add_test(NAME tst_flipchart_miniatures_samples
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --samples "$ENV{ACTCRAFT_SAMPLES_DIR}"
        --connues "$ENV{ACTCRAFT_THM_CONNUES}")
set_tests_properties(tst_flipchart_miniatures_samples PROPERTIES
    SKIP_RETURN_CODE 77 TIMEOUT 600 ENVIRONMENT "${_bench_env_offscreen}" PASS_REGULAR_EXPRESSION "MINIATURES OK")
add_test(NAME tst_captures_ouvrir_flipchart_synthetique
    COMMAND $<TARGET_FILE:actcraft-captures>
        --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/ouvrir-flipchart-synthetique.json
        --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-ouvrir-flipchart)
set_tests_properties(tst_captures_ouvrir_flipchart_synthetique PROPERTIES
    TIMEOUT 120 ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "SCENARIO OK" FAIL_REGULAR_EXPRESSION "ECART|SCENARIO EN ECHEC|SCENARIO INVALIDE")
```

  `$ENV{ACTCRAFT_SAMPLES_DIR}` est lu à la configuration : vide en CI → l'outil reçoit `""` → code 77 → sauté.
  Lancer la commande ciblée (`tst_flipchart_|tst_captures_ouvrir`) : échecs (options inconnues).

- [ ] **Étape 2 : l'outil.** Dans `bench/flipchart/main.cpp`, avant le traitement actuel, aiguiller sur les options
  (le chemin existant reste le défaut) ; l'outil lie désormais `ActCraft::render` et `Qt6::Gui`
  (`target_link_libraries(actcraft-flipchart-dump PRIVATE ActCraft::formats ActCraft::render Qt6::Gui)`,
  `target_include_directories(actcraft-flipchart-dump PRIVATE ${PROJECT_SOURCE_DIR}/tests/support)` pour l'écrivain
  synthétique), et crée un `QGuiApplication` (texte) au lieu du `QCoreApplication`. Code de la comparaison :

```cpp
// Écart moyen (0 à 255) entre deux images ramenées à 80 × 60 (mesure de S4, L2-T21).
double thumbnailGap(const QImage& rendered, const QImage& thumbnail)
{
    const QImage a = rendered.convertToFormat(QImage::Format_RGB888)
                         .scaled(80, 60, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    const QImage b = thumbnail.convertToFormat(QImage::Format_RGB888)
                         .scaled(80, 60, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    double total = 0;
    for (int y = 0; y < 60; ++y) {
        const uchar* la = a.constScanLine(y);
        const uchar* lb = b.constScanLine(y);
        for (int x = 0; x < 80 * 3; ++x) {
            total += std::abs(int(la[x]) - int(lb[x]));
        }
    }
    return total / (80.0 * 60.0 * 3.0);
}

// Écarts de toutes les pages à miniature d'un paperboard ; `withoutThumbnail` compte les autres.
std::vector<double> pageGaps(const QByteArray& data, int& withoutThumbnail, QString& error)
{
    const auto r = actcraft::formats::flipchart::readFlipchart(data);
    if (!r.ok) {
        error = r.error;
        return {};
    }
    const actcraft::render::DecodedImages images = actcraft::render::decodeImages(r.document);
    std::vector<double> gaps;
    for (std::size_t i = 0; i < r.document.pageCount(); ++i) {
        const QImage thumbnail = QImage::fromData(r.thumbnails[i]);
        if (thumbnail.isNull()) {
            ++withoutThumbnail;
            continue;
        }
        const actcraft::core::Page& page = r.document.page(i);
        const double scale = page.width > 0 ? thumbnail.width() / page.width : 1.0;
        actcraft::render::PaintOptions options;
        options.thumbnail = true;
        gaps.push_back(thumbnailGap(actcraft::render::renderPage(page, images.lookup(), scale, options), thumbnail));
    }
    return gaps;
}
```

  `--miniatures` et `--samples` impriment `PAGE <fichier>:<n> écart <e>` par page (deux décimales), puis la synthèse :
  médiane (moyenne des deux valeurs centrales si le nombre est pair), nombre au-dessus de 7, verdict (médiane ≤ 2,0 et
  au plus 3 au-dessus de 7) ; `--samples` trie les fichiers par nom et rend 77 si aucun `.flipchart`.
  `--ecrire-synthetique` : lire `fctest::sampleFlipchart()`, rendre ses deux pages par `renderPage` (320 px de large,
  `PaintOptions::thumbnail`), les encoder en JPEG (`QImage::save(&buffer, "JPG", 90)`), réécrire le paperboard par
  `fctest::sampleFlipchart({jpeg1, jpeg2})` et l'écrire ; `SYNTHETIQUE OK : <chemin>`.

- [ ] **Étape 3 : la fixture et les scénarios.** Dans `bench/src/fixtures.cpp`, ajouter la fixture
  `flipchart-synthetique` (octets de `fctest::sampleFlipchart()` écrits tels quels par `formats::writeUrl` ;
  `bench/CMakeLists.txt` : `target_include_directories(actcraft_benchui PRIVATE ${PROJECT_SOURCE_DIR}/tests/support)`),
  et élargir la vérification de nom de `parseStep` (`uiscenario.cpp`) aux deux fixtures. Créer
  `bench/scenarios/ouvrir-flipchart-synthetique.json` :

```json
{
  "format": "actcraft-scenario/1",
  "name": "ouvrir-flipchart-synthetique",
  "description": "S4 en CI : un paperboard .flipchart écrit par nous s'ouvre ; deux pages, avertissement affiché sans bloquer, fond dégradé et forme verte visibles ; document sans fichier.",
  "steps": [
    {"waitReady": true},
    {"fixture": "flipchart-synthetique", "path": "${SORTIE}/cours.flipchart"},
    {"command": "file.open", "argument": "${SORTIE}/cours.flipchart"},
    {"waitFor": "documentController.busy", "value": false},
    {"expect": "pageIndicator.text", "value": "1 / 2"},
    {"expect": "documentController.title", "value": "cours"},
    {"expect": "messageBanner.visible", "value": true},
    {"waitReady": true},
    {"checkPixel": "forme-verte", "at": [340, 230], "color": "#008000", "tolerance": 40},
    {"capture": "page-1"},
    {"command": "page.next"},
    {"waitReady": true},
    {"capture": "page-2"}
  ]
}
```

  (La forme du paperboard synthétique est translatée en (300, 200), 80 × 60, verte : (340, 230) est dedans. Si le
  dessin de la forme la recouvre d'une étiquette ou du trait, ajuster le point **dans** la forme, pas la tolérance.)
  Créer `bench/scenarios/local/ouvrir-flipchart.json` : `waitReady`, `waitFor documentController.busy=false`,
  `expect pageIndicator.text = "1 / N"` (N = nombre de pages du paperboard réel n° 16, relevé par L2-T10 étape 7),
  `capture` de la page 1, puis `page.next` + `waitReady` + `capture` pour chaque page (N − 1 fois).

  Dans `scripts/captures.ps1` : paramètre `[string]$Ouvrir = ''` ; s'il n'est pas vide, ajouter
  `'--ouvrir', $Ouvrir` aux arguments d'`actcraft-captures` ; `actcraft-captures` les passe déjà à
  `parseStartupOptions` (L2-T19) : vérifier qu'il ne prend pas `--ouvrir` pour une option inconnue. Documenter
  `-Ouvrir` dans l'en-tête du script.

- [ ] **Étape 4 : vert ciblé, puis S4 local.** Commande ciblée (`tst_flipchart_|tst_captures_`) : `100% tests
  passed` (`tst_flipchart_miniatures_samples` « Skipped » si la variable n'était pas posée à la configuration). Puis
  les deux commandes locales de la section « Test et commande bornée » ; noter médiane, nombre au-dessus de 7, pages
  sans miniature ; décrire les captures.

- [ ] **Étape 5 : preuve.** `format.ps1`, preuve de tâche (N ≥ 66).

- [ ] **Étape 6 : commit.**

```powershell
git -C <worktree> add bench/flipchart/main.cpp bench/CMakeLists.txt bench/src/fixtures.cpp bench/src/uiscenario.cpp bench/scenarios/ouvrir-flipchart-synthetique.json bench/scenarios/local/ouvrir-flipchart.json scripts/captures.ps1 tests/bench.cmake
git -C <worktree> commit -m "S4 : miniatures comparées aux .thm (outil), scénarios d'ouverture d'un .flipchart (CI synthétique, local)" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t22"></a>
### Tâche L2-T22 — Android : Ouvrir et Enregistrer par `content://` sur l'émulateur, sinon commandes de fichier retirées de l'APK

**Objectif.** Décision 17 et S5 (Android) : sur l'émulateur, enregistrer puis rouvrir le même document par une URL
`content://` donne la **même empreinte**. Chemin : APK `x86_64` construit (`paquet-android.ps1`), installé ; un trait
au doigt (`adb shell input swipe`) ; « Enregistrer sous » par le raccourci Ctrl+Maj+S
(`adb shell input keycombination 113 59 47`) → sélecteur du système (DocumentsUI, `ACTION_CREATE_DOCUMENT`), bouton
d'enregistrement trouvé par `adb shell uiautomator dump` et touché par `adb shell input tap` ; journal
`ACTCRAFT ENREGISTRE <empreinte>` lu par `adb logcat -d -s actcraft.document` (catégorie Qt) ; Ctrl+W, puis Ctrl+O
(`keycombination 113 51`, `113 43`) → sélecteur → le fichier touché → `ACTCRAFT OUVERT <empreinte>` ; les deux
empreintes égales → `ANDROID FICHIERS OK`. L'émulateur est démarré par le script **sans fenêtre** (`-no-window`,
`-gpu host`, piège SwiftShader du lot 1) et arrêté par son PID ; aucun démon Gradle laissé vivant.

**Repli (décision 17)** : si, après trois essais, le chemin ne peut être prouvé (sélecteur non pilotable, écriture
refusée, URL inutilisable), les commandes `file.open`, `file.save` et `file.save-as` sont déclarées absentes
d'Android (`"platforms": ["windows", "macos", "linux"]` dans `actions/data/commands.json`) jusqu'au lot 11 ; preuve :
`APK OK` puis capture (`adb exec-out screencap -p`) du menu Fichier de l'APK sans « Ouvrir… » ni « Enregistrer » ;
cause écrite dans `docs/verifications/2026-10-15-android-fichiers.md` et au rapport (porte de fin du lot 2).

**Fichiers.**
- Créer : `scripts/android-commun.ps1`, `scripts/essai-android-fichiers.ps1`,
  `docs/verifications/2026-10-15-android-fichiers.md`
- Modifier : `scripts/essai-android.ps1` (passe par le module commun) ; en repli seulement :
  `actions/data/commands.json`

**Test et commande bornée.**

```powershell
pwsh -NoProfile -Command "Set-Location '<worktree>'; pwsh -NoProfile -File ./scripts/paquet-android.ps1 > .tmp/apk.txt 2>&1; Select-String -Path .tmp/apk.txt -Pattern 'APK (OK|EN ECHEC)' | Select-Object -Last 1"
pwsh -NoProfile -Command "Set-Location '<worktree>'; pwsh -NoProfile -File ./scripts/essai-android-fichiers.ps1 -Apk build/paquet/ActCraft-0.1.0-x86_64.apk > .tmp/fichiers.txt 2>&1; Select-String -Path .tmp/fichiers.txt -Pattern 'ENREGISTRE|OUVERT|ANDROID FICHIERS (OK|EN ECHEC)' | Select-Object -Last 3"
```

Attendu : `APK OK`, deux lignes d'empreinte égales, `ANDROID FICHIERS OK` (ou le repli et sa preuve). Le numéro de
version de l'APK est celui du `CMakeLists.txt` au moment de la tâche (0.1.0 ; 0.2.0 après la clôture).

**Interdits.** L'émulateur et la chaîne Android sont ceux de L1-T2 (`scripts/dev-env-android.ps1`, `~/.tools`) :
aucune installation nouvelle sans le rapporter. Rien n'est poussé. Aucune donnée réelle sur l'émulateur. Ne jamais
arrêter un processus par son nom. Pas de nouvelle permission Android (décision 28 : le sélecteur suffit).

**Terminé quand.** `ANDROID FICHIERS OK` (ou repli prouvé), note de vérification écrite, commit fait, émulateur et
démons arrêtés (PID au rapport).

**Étapes.**

- [ ] **Étape 1 : écrire le script.** Créer `scripts/essai-android-fichiers.ps1`, sur le modèle de
  `scripts/essai-android.ps1` (relire ce script : démarrage de l'émulateur, attente du démarrage, `adb` borné,
  arrêt par PID, lignes `… EN ECHEC` sur toute erreur, y compris imprévue) :

```powershell
# S5 sous Android (décision 17) : enregistrer puis rouvrir par content:// sur l'émulateur, empreintes égales.
# Usage : pwsh -NoProfile -File ./scripts/essai-android-fichiers.ps1 -Apk <…-x86_64.apk> [-Avd <nom>]
# Dernière ligne : « ANDROID FICHIERS OK » (0) ou « ANDROID FICHIERS EN ECHEC : <raison> » (1).
param([Parameter(Mandatory)][string]$Apk, [string]$Avd = '')
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
. "$PSScriptRoot/dev-env-android.ps1"
$paquet = 'io.github.jejeasterix.actcraft'
$emulateur = $null
function Stop-Essai([string]$raison) {
    if ($emulateur) { Stop-Process -Id $emulateur.Id -Force -ErrorAction SilentlyContinue }
    Write-Output "ANDROID FICHIERS EN ECHEC : $raison"
    exit 1
}
# Adaptateur sur l'appel borné du module commun (Adb) : aucune copie ; ses exceptions arrivent au catch final.
function Invoke-Adb([string[]]$arguments, [int]$secondes = 30) {
    return "$(Adb $secondes @arguments)"
}
# Touche le nœud de l'interface Android dont le texte ou l'identifiant correspond (sélecteur du système).
function Invoke-Toucher([string]$motif) {
    Invoke-Adb @('shell', 'uiautomator', 'dump', '/sdcard/ui.xml') | Out-Null
    Invoke-Adb @('pull', '/sdcard/ui.xml', '.tmp/ui.xml') | Out-Null
    [xml]$ui = Get-Content -Raw .tmp/ui.xml
    $noeud = $ui.SelectNodes('//node') | Where-Object { $_.text -match $motif -or $_.'resource-id' -match $motif } | Select-Object -First 1
    if (-not $noeud) { return $false }
    $b = [regex]::Matches($noeud.bounds, '\d+') | ForEach-Object { [int]$_.Value }
    Invoke-Adb @('shell', 'input', 'tap', [string](($b[0] + $b[2]) / 2), [string](($b[1] + $b[3]) / 2)) | Out-Null
    return $true
}
function Wait-Journal([string]$motif, [int]$secondes = 30) {
    $fin = (Get-Date).AddSeconds($secondes)
    while ((Get-Date) -lt $fin) {
        $ligne = (Invoke-Adb @('logcat', '-d')) -split "`n" | Where-Object { $_ -match $motif } | Select-Object -Last 1
        if ($ligne) { return $ligne }
        Start-Sleep -Milliseconds 500
    }
    return $null
}
try {
    New-Item -ItemType Directory -Force .tmp | Out-Null
    Initialize-ActCraftAdb                                   # android-commun.ps1 : adb du PATH, exception sinon
    $emulateur = Start-ActCraftEmulateur -Avd $Avd          # défini dans CE script : -no-window -gpu host, PID gardé
    Wait-ActCraftDemarrage                                   # android-commun.ps1 : appareil, sys.boot_completed, borné
    Ecarter-Anr                                              # android-commun.ps1
    Install-ActCraftApk -Apk $Apk                            # android-commun.ps1 : install -r, logcat -c, lancement
    Invoke-Adb @('shell', 'input', 'swipe', '600', '700', '1000', '760', '400') | Out-Null
    Invoke-Adb @('shell', 'input', 'keycombination', '113', '59', '47') | Out-Null    # Ctrl+Maj+S
    Start-Sleep -Seconds 3
    if (-not (Invoke-Toucher '(?i)^(save|enregistrer)$|android:id/button1')) { Stop-Essai 'bouton d''enregistrement du sélecteur introuvable' }
    $enregistre = Wait-Journal 'ACTCRAFT ENREGISTRE ([0-9a-f]{16})'
    if (-not $enregistre) { Stop-Essai 'aucune ligne ACTCRAFT ENREGISTRE' }
    Write-Output $enregistre
    Invoke-Adb @('shell', 'input', 'keycombination', '113', '51') | Out-Null           # Ctrl+W
    Invoke-Adb @('shell', 'input', 'keycombination', '113', '43') | Out-Null           # Ctrl+O
    Start-Sleep -Seconds 3
    if (-not (Invoke-Toucher '\.actcraft$')) { Stop-Essai 'fichier enregistré absent du sélecteur' }
    $ouvert = Wait-Journal 'ACTCRAFT OUVERT ([0-9a-f]{16})'
    if (-not $ouvert) { Stop-Essai 'aucune ligne ACTCRAFT OUVERT' }
    Write-Output $ouvert
    $a = [regex]::Match($enregistre, '[0-9a-f]{16}').Value
    $b = [regex]::Match($ouvert, '[0-9a-f]{16}').Value
    if ($a -ne $b) { Stop-Essai "empreintes différentes : $a enregistrée, $b relue" }
    Stop-Process -Id $emulateur.Id -Force
    Write-Output 'ANDROID FICHIERS OK'
    exit 0
} catch {
    Stop-Essai $_.Exception.Message
}
```

  **Pas de copie** : `essai-android.ps1` **ne démarre pas** l'émulateur (contrat inchangé : un appareil est déjà là ;
  en CI, `android-emulator-runner` le démarre, `ci.yml:208`). Extraire de `essai-android.ps1` dans
  `scripts/android-commun.ps1` seulement ce qui est commun : la fonction `Adb` (appel borné), l'attente bornée de
  l'appareil et de `sys.boot_completed` (`Wait-ActCraftDemarrage`), l'installation et le lancement de l'APK
  (`Install-ActCraftApk`) et `Ecarter-Anr` — mêmes bornes, mêmes messages ; module chargé par point
  (`. "$PSScriptRoot/android-commun.ps1"`) dans les deux scripts. **Module autonome** : il ne lit aucune variable ni
  fonction de ses appelants. `Initialize-ActCraftAdb` cherche `adb` dans le PATH et le garde dans
  `$script:ActCraftAdb` (variable propre au module, nommée pour ne heurter personne) ; `Adb`, `Wait-ActCraftDemarrage`,
  `Install-ActCraftApk` et `Ecarter-Anr` **lèvent une exception** (`throw "<raison>"`) au lieu d'appeler `Echec` ;
  chaque script appelle `Initialize-ActCraftAdb` en tête de son `try` et traduit l'exception dans son `catch` par sa
  propre dernière ligne (`ANDROID EN ECHEC : <raison>` pour `essai-android.ps1`, inchangée ;
  `ANDROID FICHIERS EN ECHEC : <raison>` pour `essai-android-fichiers.ps1`). `essai-android-fichiers.ps1` n'a plus
  d'appel `adb` à lui : son `Invoke-Adb` n'est qu'un adaptateur sur `Adb`. `essai-android.ps1` garde son contrat et ses lignes
  de sortie, et redonne `ANDROID OK` sur un émulateur démarré (rejouer S3 une fois) ; le job `android` de la CI ne
  change pas de comportement (même commande, même dernière ligne). Le **démarrage local** de l'émulateur
  (`Start-ActCraftEmulateur`, `-no-window -gpu host`, PID gardé) et son arrêt par PID (`Stop-ActCraftEmulateur`) restent
  dans `essai-android-fichiers.ps1` ; `Stop-Essai` l'arrête. `Start-Sleep` est permis ici (script hors du joueur de scénarios). Les intitulés du sélecteur dépendent de la langue de l'image système : le motif
  accepte l'anglais et le français.

- [ ] **Étape 2 : jouer l'essai.** Les deux commandes de la section « Test et commande bornée ». En cas d'échec,
  diagnostiquer (capture `adb exec-out screencap -p > .tmp/ecran.png`, `.tmp/ui.xml`), corriger le script, rejouer ;
  au troisième échec, appliquer le repli.

- [ ] **Étape 3 : la note de vérification.** Écrire `docs/verifications/2026-10-15-android-fichiers.md` : image
  système et API de l'émulateur, commande jouée, les deux lignes d'empreinte, verdict ; ou, en repli, la cause, la
  modification de `commands.json`, la ligne `APK OK` et la description de la capture du menu.

- [ ] **Étape 4 : commit.**

```powershell
git -C <worktree> add scripts/android-commun.ps1 scripts/essai-android.ps1 scripts/essai-android-fichiers.ps1 docs/verifications/2026-10-15-android-fichiers.md
git -C <worktree> commit -m "Android : enregistrer et rouvrir par content:// sur l'émulateur (S5)" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

  En repli, ajouter `actions/data/commands.json` et écrire « commandes de fichier retirées de l'APK (repli de la
  décision 17) » dans le message.


<a id="tache-l2-t23"></a>
### Tâche L2-T23 — Banc : scénario « changement de page » ; page fixe sans éclair blanc pendant le décodage

**Objectif.** Revue L1-T9 (« ajouter un scénario de banc “changement de page” : éclair blanc tant que les images ne
sont pas prêtes ») et S4 (un paperboard réel a des images sur chaque page). Deux volets :

1. **Page fixe** : pendant qu'une nouvelle page décode ses images (`!isReady()`), l'élément **garde ses nœuds
   précédents** au lieu de ne dessiner qu'un fond blanc, et ne les remplace qu'une fois prêt ; la vue ne redessine
   son cache qu'à `readyChanged` (déjà le cas). Plus d'éclair blanc, avec ou sans cache.
2. **Banc** : scénario `page` (complet et court) — sur un document de deux pages générées (graines différentes, 30
   images chacune, `DocumentController::loadGeneratedPages(2, true)` : la page 2 reçoit en plus 20 textes et 20
   formes à chemins, objets **peints** — le chemin RHI rastérisé de L2-T13 entre ainsi dans la mesure), des
   changements de page à intervalle fixe, puis deux zooms posés sur la page peinte (`view.zoom` 150, `view.fit` :
   reconstruction rastérisée à la nouvelle échelle en RHI),
   (`InputKind::Command`, commande `page.next` puis `page.previous`, appelée par `runCommand` au moment prévu) ;
   mesures habituelles (temps par image, images > 25 ms) ; contrôle de contenu `page_flash` : juste après chaque
   changement, à l'image suivante, le pixel du témoin de forme (`core::witnesses().shapeRect`, mêmes positions pour
   toutes les graines) n'est pas blanc. Le verdict reste un **résultat** (décision 35) ; la clôture le compare.

**Fichiers.**
- Modifier : `render/include/actcraft/render/fixedpageitem.h`, `render/src/fixedpageitem.cpp`,
  `tests/tst_render_fixedpage.cpp`, `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`,
  `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp`, `bench/src/runner.cpp`,
  `tests/tst_bench_scenarios.cpp`, `tests/tst_bench_result.cpp`, `tests/tst_bench_verdict.cpp`,
  `scripts/resume-banc.ps1`, `docs/banc.md`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_render_fixedpage` → échec de
`keepsTheOldContentUntilTheNewPageIsReady` ; motif `tst_bench_scenarios` → échec de `pageScenarioFlipsPages`. Vert :
forme bornée, N ≥ 59, `TESTS OK`, `tst_render_fixedpage`, `tst_bench_scenarios`, `tst_bench_result` Passed. Banc
hors écran : le JSON contient le scénario `page`, `content_check.ok = True`, `page_flash = 0`.

**Interdits.** Ne pas changer les scénarios `stroke`, `zoom`, `scroll` (référence de non-régression). Ne pas poster
de touches pour changer de page (commande nommée appelée directement : pas de piège de raccourci sous macOS).
`tst_bench_verdict.cpp` et `tst_bench_result.cpp` ne changent que pour compter le quatrième scénario (raison écrite) ;
`tst_bench_verdict` reste instable sous charge (piège du lot 1) : le rejouer machine calme.

**Terminé quand.** Preuve verte, JSON du banc joint (avec le scénario `page`), commit fait.

**Étapes.**

- [ ] **Étape 1 : le test de la page fixe.** Dans `tests/tst_render_fixedpage.cpp` :

```cpp
    void keepsTheOldContentUntilTheNewPageIsReady()
    {
        Fixture f;
        open(f);
        core::Page first;
        first.background.fill = core::solidFill(0xFF0000FFu);
        f.page->setPage(first);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(500, 400), QColor(Qt::blue), 2));
        f.page->setPage(core::generatePage(core::kDefaultSeed)); // 30 images à synthétiser : pas prêt tout de suite
        QVERIFY(!f.page->isReady());
        waitFrame(f);
        // Tant que la page suivante n'est pas prête, l'ancienne reste affichée (jamais un fond blanc nu).
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(500, 400), QColor(Qt::blue), 2));
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        QVERIFY(!colorNear(f.window.grabWindow().pixelColor(500, 400), QColor(Qt::blue), 2));
    }
```

  Le test du lot 1 qui attendait un fond blanc avant le premier « prêt » d'une page **sans contenu précédent**
  (`emptyPageIsWhite`) reste vrai : sans nœuds précédents (premier dessin), le fond blanc est dessiné.

- [ ] **Étape 2 : garder l'ancien contenu.** Dans `FixedPageItem::updatePaintNode` (L2-T13/L2-T14), la branche de
  reconstruction ne vide les nœuds que si la page est prête **ou** s'il n'y a pas encore de contenu :

```cpp
    if (m_rebuild || !old) {
        if (!m_ready && old && root->childCount() > 0) {
            return root; // garder la page précédente jusqu'au prêt (pas d'éclair blanc, L2-T23)
        }
        // … reconstruction existante …
    }
```

  Après `onImagesReady`, `m_rebuild` vaut vrai : la reconstruction a lieu au dessin suivant. Le crochet
  `rebuildCount` n'est incrémenté que lors d'une vraie reconstruction (après ce retour anticipé).

- [ ] **Étape 3 : pages générées pour le banc.** Dans `documentcontroller.h`, ajouter
  `Q_INVOKABLE void loadGeneratedPages(int count, bool painted = false);` (« Banc : `count` pages générées, graines
  `kDefaultSeed + i` ; `painted` : chaque page paire (2, 4…) reçoit 20 textes et 20 formes à chemins ; document propre,
  page 1 affichée ») et dans le `.cpp` :

```cpp
void DocumentController::loadGeneratedPages(int count, bool painted)
{
    std::vector<core::Page> pages;
    for (int i = 0; i < std::max(1, count); ++i) {
        core::Page page = core::generatePage(core::kDefaultSeed + static_cast<std::uint32_t>(i));
        // Identifiants uniques dans le document (1 à 300 sur chaque page générée sinon).
        if (painted && i % 2 == 1) {
            for (int k = 0; k < 20; ++k) {
                core::Text text;
                text.bounds = {0, 0, 220, 50};
                text.html = "<p style=\"font-size:32px; color:#202020\">Texte du banc</p>";
                core::PageObject t = text;
                t.common.transform = core::translation(40 + 45.0 * k, 60 + 30.0 * k);
                page.objects.push_back(t);
                core::Shape shape;
                shape.kind = core::ShapeKind::Path;
                shape.paths = {{{0, 0}, {80, 10}, {60, 70}, {-10, 50}}};
                shape.pen.width = 4;
                shape.brush = core::solidFill(0xFF26A69Au);
                core::PageObject s = shape;
                s.common.transform = core::translation(500 + 20.0 * k, 120 + 25.0 * k);
                page.objects.push_back(s);
            }
        }
        for (std::size_t j = 0; j < page.objects.size(); ++j) {
            page.objects[j].common.id = static_cast<core::ObjectId>(i) * 1000 + j + 1;
        }
        pages.push_back(std::move(page));
    }
    replaceDocument(core::Document(std::move(pages)), QUrl(), QString());
}
```

- [ ] **Étape 4 : le scénario de banc.** Dans `scenarios.h`, ajouter à la fin de `InputKind` `Command` ; à
  `PlannedInput`, `QString command; // InputKind::Command : commande nommée appelée par runCommand` et
  `QVariant argument; // son argument éventuel` (include `<QVariant>`) ; déclarer
  `ScenarioPlan pageScenario(ScenarioSet set);` et mettre à jour le commentaire d'`allScenarios` (« stroke, zoom,
  scroll, page »). Dans `scenarios.cpp` :

```cpp
ScenarioPlan pageScenario(ScenarioSet set)
{
    ScenarioPlan plan;
    plan.name = QStringLiteral("page");
    const int flips = 20 / durationDivisor(set); // complet : 20 changements, court : 4
    for (int i = 0; i < flips; ++i) {
        PlannedInput in;
        in.offsetNs = static_cast<std::int64_t>(i) * 500'000'000; // toutes les 500 ms
        in.kind = InputKind::Command;
        in.command = i % 2 == 0 ? QStringLiteral("page.next") : QStringLiteral("page.previous");
        plan.inputs.push_back(in);
    }
    // Sur la page peinte (2), deux zooms posés : rastérisation à la nouvelle échelle en RHI (L2-T13).
    const std::int64_t end = static_cast<std::int64_t>(flips) * 500'000'000;
    const std::pair<const char*, QVariant> zooms[] = {{"page.next", QVariant()}, {"view.zoom", QVariant(150)},
                                                      {"view.fit", QVariant()}};
    for (std::size_t k = 0; k < 3; ++k) {
        PlannedInput in;
        in.offsetNs = end + static_cast<std::int64_t>(k) * 700'000'000;
        in.kind = InputKind::Command;
        in.command = QString::fromLatin1(zooms[k].first);
        in.argument = zooms[k].second;
        plan.inputs.push_back(in);
    }
    return plan;
}
```

  et `allScenarios` rend `{strokeScenario(set), zoomScenario(set), scrollScenario(set), pageScenario(set)}`. Dans
  `runner.cpp` : avant le scénario `page`, `loadGeneratedPages(2, true)` sur le contrôleur (trouvé par `objectName`
  `documentController`) puis attente du prêt ; une entrée `Command` est jouée par
  `QMetaObject::invokeMethod(window, "runCommand", …)` à son heure (dans la boucle qui poste les entrées, à côté de la
  molette) ; après la mesure, le contrôle `page_flash` : trois changements, chacun suivi d'un `grabWindow` à la
  première image rendue (`frameSwapped`), pixel du centre de `witnesses().shapeRect` (converti en coordonnées de
  fenêtre par la vue) comparé au blanc ; `content_check.page_flash` = nombre de captures blanches, et
  `content_check.ok` faux si > 0. Le JSON reçoit aussi (amendé à la revue de L2-T23) `content_check.page_rebuilds_during_zoom` (écart de
  `FixedPageItem::rebuildCount()`) et `content_check.page_rasterizations_during_zoom` (écart de
  `rasterizationCount()`) entre le `view.zoom` et la fin du scénario, relevés seulement si la page peinte est
  prête et dessinée au zoom (sinon -1 : sa reconstruction au prêt gonflerait le compte). Le zoom refait les
  nœuds rastérisés en place (L2-T13), sans reconstruction : `page_rebuilds_during_zoom` vaut 0 partout. En
  rendu logiciel (preuve de tâche, ctest) `page_rasterizations_during_zoom` vaut 0 (nœuds vectoriels) ;
  **en D3D11 la clôture exige `page_rasterizations_during_zoom ≥ 80`** (zoom posé puis ajustement : deux
  échelles × 40 éléments peints de la page 2). Aucun test ctest ne tourne en RHI (tous hors écran en rendu
  logiciel, `QT_QUICK_BACKEND=software`) : le chemin RHI n'est prouvé que par ce relevé du banc à l'écran. Le verdict du scénario `page` suit les règles de `scroll` (images par seconde,
  images > 25 ms) ; `verdict.cpp` traite déjà les entrées autres que la molette comme des entrées mesurées : exclure
  `InputKind::Command` de la latence d'encre (aucune encre).

  Tests : dans `tests/tst_bench_scenarios.cpp`, `pageScenarioFlipsPages` (court : **7** entrées `Command` — 4
  changements de page en alternance `page.next` / `page.previous` à 500 ms d'écart, puis `page.next`, `view.zoom`
  (argument 150) et `view.fit` à 700 ms d'écart ; complet : 23 entrées ; `allScenarios(Court).size() == 4`, dernier nommé `page`) ; dans
  `tests/tst_bench_result.cpp`, le nombre de scénarios lus passe de 3 à 4 (raison : scénario `page` ajouté) ; dans
  `tests/tst_bench_verdict.cpp`, seulement si un cas compte les scénarios. `scripts/resume-banc.ps1` ajoute une ligne
  `page` à son tableau ; `docs/banc.md` décrit le scénario et `page_flash`.

- [ ] **Étape 5 : vert ciblé, preuve, banc.** Commande ciblée (`tst_render_fixedpage|tst_bench_`) : `100% tests
  passed`. `format.ps1`, preuve de tâche (N ≥ 59), banc sans écran : le JSON a le scénario `page` et
  `content_check.page_flash = 0` (le lire avec `ConvertFrom-Json` et le recopier au rapport).

- [ ] **Étape 6 : commit.**

```powershell
git -C <worktree> add render/include/actcraft/render/fixedpageitem.h render/src/fixedpageitem.cpp tests/tst_render_fixedpage.cpp ui/include/actcraft/ui/documentcontroller.h ui/src/documentcontroller.cpp bench/include/actcraft/bench/scenarios.h bench/src/scenarios.cpp bench/src/runner.cpp tests/tst_bench_scenarios.cpp tests/tst_bench_result.cpp tests/tst_bench_verdict.cpp scripts/resume-banc.ps1 docs/banc.md
git -C <worktree> commit -m "Banc : scénario changement de page et contrôle d'éclair ; la page fixe garde l'ancienne page jusqu'au prêt" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

  (Ne nommer que les fichiers réellement modifiés.)

---

## Hypothèses nouvelles de 2b (à revoir, porte de fin du lot 2)

- **H2b-1 — Mesure des miniatures** (L2-T21) écrite par nous ; si la médiane locale dépasse 2,0 alors que la
  référence Python l'atteint, l'écart vient du peintre ou de la mesure : à trancher avec la page fautive sous les yeux.
- **H2b-2 — Sélecteur Android piloté par uiautomator** (L2-T22) : dépend de la langue et de la version de DocumentsUI
  de l'image système ; en repli, les commandes de fichier manquent à l'APK jusqu'au lot 11.
- **H2b-3 — Pas d'onglets avant le lot 5** : Nouveau et Ouvrir remplacent le document affiché (après la demande
  d'enregistrement).
- **H2b-4 — Translucidité et transformation par nœuds parents** (L2-T13) : coût par objet en RHI à surveiller au
  banc quand des pages réelles chargées seront mesurées (lot cache, qui suit le lot 2).
- **H2b-5 — Rastérisation dans la synchronisation** (L2-T13) : en RHI, le moteur peint rastérise ses objets pendant
  `updatePaintNode` (fil principal bloqué) et toute la page peinte est reconstruite à chaque zoom posé. Mesuré par le
  scénario `page` en D3D11 à la clôture ; si les images de cette phase dépassent le budget, le lot cache (qui suit)
  déplace la rastérisation sur un fil secondaire derrière la même interface.
- **H2b-7 — Deux redessins du cache vers une page à images** (L2-T14, L2-T23) : `setSource` émet `contentChanged`
  (redessin qui reprend l'ancienne page, gardée jusqu'au prêt) puis `readyChanged` (redessin de la nouvelle) ; le
  premier est inutile mais sans éclair. Consigné, pas corrigé au lot 2 : le lot cache (qui suit) peut ne demander le
  redessin qu'au prêt ; le scénario `page` le mesure.
- **H2b-6 — Dialogues non natifs** (décision 22) : aucun scénario de 2b n'ouvre de dialogue (commandes avec URL) ; le
  moyen relevé pour forcer les dialogues Qt Quick dans `actcraft-captures`, `QCoreApplication::setAttribute(
  Qt::AA_DontUseNativeDialogs)` avant la création de l'application, est laissé au premier lot dont un scénario doit
  voir un dialogue (hors écran, la plateforme `offscreen` n'a de toute façon pas de dialogue natif).

## Clôture du lot 2 (orchestrateur, hors tâches)

Jouée par l'orchestrateur après la dernière vague de 2b, dans le worktree du chantier, sur `feat/livraison-complete`
à jour, avant la PR du lot (étage 4). Le lot « cache incrémental » (H-mes-1), avancé par Jérôme, **suit** le lot 2 :
il part de l'interface `core::PageCache` figée ici (L2-T14) et ne change que son implémentation.

1. **Version 0.2.0** : `project(VERSION 0.2.0)` dans `CMakeLists.txt` racine ; `CHANGELOG.md` : section
   `## 0.2.0 — <date>` (ouvrir un `.flipchart`, format natif `.actcraft`, menu Fichier, avertissements, Android selon
   L2-T22) ; `scripts/notes-version.ps1` → `NOTES OK`. Commit.
2. **Suite complète** : `scripts/test.ps1 -Preset release` puis `scripts/test.ps1` (formes bornées du profil),
   N ≥ 66 chacune, `TESTS OK` ; aucun test retiré ; seuls sautés (code 77) : les **trois** tests locaux
   `tst_formats_oracle`, `tst_formats_oracle_synth` et `tst_flipchart_miniatures_samples`.
   **Amendement du 2026-10-17** (le texte d'origine, « deux tests sautés », omettait `tst_formats_oracle_synth`).
   Pour les rejouer, poser **avant la configuration** (`scripts/build.ps1` ou `scripts/test.ps1` les lisent à ce
   moment ; configuration refaite si elles changent) : `ACTCRAFT_SAMPLES_DIR` (dossier des paperboards réels) ;
   `ACTCRAFT_REFERENCE_DIR` (racine de l'ancien dépôt, pour l'oracle synthétique, qui exige Python 3 : `ACTCRAFT_PYTHON`
   sinon `python` / `python3`) ; `ACTCRAFT_THM_PAGES=124` ; `ACTCRAFT_THM_CONNUES` (liste des pages connues, lue d'un
   fichier **hors dépôt** et jamais écrite dans ce plan ni dans un rapport). Lignes attendues (`ctest -V`, sorties
   gardées dans `.tmp/`) : `tst_formats_oracle` → `ORACLE OK : 17 paperboards, 124 pages, 0 écart` ;
   `tst_formats_oracle_synth` → `ORACLE SYNTHETIQUE OK : <n> paperboards, <p> pages, 0 écart ; écarts attendus nommés : …` ;
   `tst_flipchart_miniatures_samples` → `MINIATURES OK : 124 pages, médiane ≤ 2,0, ≤ 3 au-dessus de 7`. Les trois
   Passed, aucun sauté.
3. **S4 à l'écran** (jeton `ecran`, annoncé d'une ligne à Jérôme) : oracle local `0 écart` (L2-T10) ;
   `MINIATURES OK : 124 pages, médiane ≤ 2,0, ≤ 3 au-dessus de 7` (L2-T21) ;
   `captures.ps1 -Scenario local/ouvrir-flipchart -Rendu d3d11 -Sortie .tmp/captures-ecran -Ouvrir "<samples>\<paperboard réel n° 16>"`
   → `CAPTURES OK`, captures regardées et décrites ; puis `build/release/actcraft.exe "<samples>\<paperboard réel n° 16>" --quitter-apres 8000` lancé, suivi par son PID : fenêtre ouverte sur le document, sortie à 0.
4. **S5 à l'écran** : `captures.ps1 -Scenario aller-retour-natif -Rendu d3d11 -Sortie .tmp/captures-ecran` →
   `BOUCLE max=… ms OK`, `LU documentController.fingerprint=… OK`, `CAPTURES OK` ; Android : `ANDROID FICHIERS OK`
   (L2-T22, rejoué sur l'APK 0.2.0) ou repli constaté (capture du menu).
5. **Banc de non-régression (DA7, décision 35)** sous le jeton `ecran` :
   `pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu tous -Captures .tmp/banc-lot2`, comparé critère par critère à
   `docs/mesures/2026-10-15-lot1-socle.md` : tout critère tenu par la référence doit l'être encore (scénarios
   `stroke`, `zoom`, `scroll`), sinon une tâche de correction précède la clôture ; le scénario `page` (nouveau) est
   rapporté — avec ses images > 25 ms sur la page peinte et pendant les deux zooms posés (chemin RHI rastérisé,
   H2b-5) —, `content_check.page_flash = 0` exigé (avec `page_flash_probes ≥ 1`), et
   `content_check.page_rasterizations_during_zoom ≥ 80` exigé pour le passage D3D11 (preuve que la
   rastérisation RHI a bien eu lieu à chaque échelle ; -1 : page peinte pas prête au zoom, passage à rejouer ;
   amendé à la revue de L2-T23, `page_rebuilds_during_zoom = 0` exigé, aussi bien en D3D11 qu'en logiciel). Résumé commité `docs/mesures/<date>-lot2-document.md` (par
   `scripts/resume-banc.ps1`) : il devient la référence du lot suivant (cache incrémental).
6. **Paquets remis à Jérôme** : `scripts/paquet.ps1` + `scripts/essai-installateur.ps1` (`PAQUET OK`,
   `INSTALLATEUR OK`, `--version : ActCraft 0.2.0`), `scripts/paquet-android.ps1` (`APK OK`) ; l'association
   `.actcraft` de l'installateur ouvre désormais le fichier (L2-T19) : l'essayer une fois (double-clic simulé par
   `Start-Process <fichier.actcraft>` après installation silencieuse, fenêtre suivie par PID puis fermée).
7. **Contrôles du dépôt public** : ceux du point d'étape 2a, plus `git ls-files 'bench/scenarios/local/*'` (aucune
   donnée de `samples/` : seulement des commandes et des chemins `${SAMPLES}` ou `-Ouvrir`).
8. **Graphe** : `graphify update .` puis commit de `graphify-out/`.
9. **Porte de fin du lot 2** : release 0.2.0 proposée (publiée seulement sur go écrit, profil) ; signaler les écarts
   au cadrage de 2a et de 2b, les hypothèses H2a-* et H2b-*, le verdict Android (preuve ou repli), les médianes S4,
   et les captures à l'écran.
