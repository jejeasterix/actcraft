# Livraison complète — lot 4 « Outils II », sous-lot 4a (formes, grille, réservations) — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**But.** Version **0.5.0** (amendement 1, § 6), livrée à la fin du sous-lot 4b
(`docs/plans/2026-10-18-livraison-complete-lot4b-PLAN.md`) : une seule PR, une seule version pour le lot. Le
sous-lot 4a livre la tâche d'ouverture, les réservations, les formes (palette à nous, tracé depuis le coin ou le
centre, carré, lignes, points intelligents), l'accrochage à la grille, la grille du dessus en surcouche, le créateur
de grille et les scénarios de banc demandés par le lot 10. Le sous-lot 4b livre le panneau des navigateurs, le
navigateur de pages, le menu de page, le fond de page, le texte et sa barre, le texte de haut en bas, la page
allongée et les zones de page. Exigences : F-040, F-041, F-042, F-100, F-240, F-241, F-242, F-250 à F-253, F-301
(a, b), F-600, F-601, F-603, F-610 (boîte, écart E2), F-620 à F-623, F-771, F-800 à F-803 (cadrage, décision 7 ;
amendement 1, § 2.1 et § 2.3). Scénarios : **S7** (cadrage), **S15**, **S16** (amendement), **S30** (ouverture),
S7.1 à S7.5 ajoutés par ce plan.

**Architecture.** Tout passe par une commande nommée (`actions/`) ; un outil (`tools/`) ne voit que
`tools::ToolContext` (lot 3) ; toute modification passe par la pile de `core/`. Quatre contraintes :

1. **Cache par son interface figée (DA11), limites du lot 10 respectées.** Une forme en cours de tracé n'entre pas
   dans le document : l'outil la montre par la **couche soulevée** du lot 3 (`tools::LiftSurface::show`), sans
   aucun redessin du cache ; au relâché, **une** commande `core::AddObjectCommand`. Une forme et un texte vont sur
   la couche intermédiaire, donc **sous** les annotations : sur une page annotée, l'insertion est « hors fin de
   page » et garde le redessin immédiat (limite déclarée, écart 5 du lot 10). Le lot 10 exige un scénario de banc
   « forme sur page annotée » : L4-T14 le livre et le **mesure**. Un texte existant est **soulevé** pendant son
   édition (un redessin au début, un à la fin, aucun pendant la frappe).
2. **Grille du dessus en surcouche, grille du dessous dans le fond.** La grille « toujours visible » (devant les
   objets) sort de la scène que le cache photographie : un élément `render::GridOverlayItem` la dessine au-dessus
   de la couche soulevée. Masquer, montrer ou régler une grille du dessus ne redessine plus le cache, et un trait
   posé sur une page à grille du dessus redevient différé (limite de DA11 levée pour ce cas, L4-T7). La grille du
   dessous reste dans la page fixe, avec le fond : la régler redessine le cache **une fois par réglage**, jamais
   pendant un déplacement. Accrocher un objet déplacé se calcule au dépôt : un redessin au soulèvement, un au dépôt,
   aucun entre les deux (preuve : `snapDragRefreshesTheCacheTwice`, scénario de banc `grille-dessus`).
3. **Miniatures sans figer l'écriture (4b).** Rendues par le peintre partagé (DA5) sur un fil secondaire, à partir
   d'une copie de la page faite sur le fil principal, regroupées (400 ms après la dernière modification de la page)
   et jamais pendant un contact. Mesurées par le scénario de banc `miniatures` (L4-T29).
4. **Fichiers chauds réservés en tête.** `L4-T1` pose d'un coup commandes, disposition, icônes, champs du modèle,
   enregistrements CMake, tests (talons `QSKIP`) et en-têtes figés des deux sous-lots ; `L4-T2` pose la coquille QML.
   Deux tâches d'une même vague ne partagent aucun fichier.

**Pile.** C++20, Qt 6.12 (Core, Gui, Quick, QuickControls2, Test), CMake + Ninja, MSVC 2022, PowerShell 7.

**Point de départ.** Le lot 4 démarre sur `feat/livraison-complete` **après la clôture du lot 3** et de la tâche
de socle commun **L0-S** (plan `docs/plans/2026-10-18-livraison-complete-socle-3-7-PLAN.md`, en cours d'écriture à
la date de ce plan ; proposition lue : relecture du plan du lot 7, § « Socle commun proposé »). Le lot 7 tourne en
parallèle sur `feat/livraison-complete-b`. Ce plan est écrit sur 333c811, avant le code des lots 3 et L0-S : **toute
API de ces lots citée ici est une hypothèse « à revérifier à l'ouverture du lot »** ; L4-T0 la revérifie contre le
code livré et l'écart amende ce plan, comme au lot 10. Les numéros de ligne sont indicatifs, les noms font foi.

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L4-T0 | Ouverture : vidéos, REA et ancien dépôt confrontés au lot ; API des lots 3, 7 et L0-S revérifiées | — | `docs/verifications/2026-10-18-lot4-principes.md` (hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot4.md`) | isolé | concepteur-critique | rapport commité, relevé brut hors dépôt présent, décisions D1 à D16 et table A1 à A14 écrites, `NOMS OK` (plan ajouté à l'index avant le contrôle), `git grep` des titres du corpus vide, N3 relevé | S30 | #tache-l4-t0 |
| L4-T1 | Réservation I : commandes, disposition (`pageMenu`), icônes, champs du modèle, CMake, 23 tests enregistrés, en-têtes figés et talons des deux sous-lots | — | `actions/data/commands.json`, `actions/data/layout.json`, `actions/include/actcraft/actions/layout.h`, `actions/src/layout.cpp`, `tests/tst_actions_layout.cpp`, `ui/include/actcraft/ui/commands.h`, `ui/src/commands.cpp`, `ui/icons/{forme,texte,texte-vertical,grille,grille-masquer,aimant,createur-grille,navigateurs,page-avant,page-apres,page-couper,page-copier,page-coller,page-dupliquer,page-supprimer,fond,allonger,zones,gras,italique,souligne,exposant,indice,aligner-gauche,centrer,aligner-droite,interligne-plus,interligne-moins,retrait-plus,retrait-moins,puces,symboles,taille-plus,taille-moins}.svg`, `core/include/actcraft/core/model.h`, `core/src/model.cpp`, `tests/tst_core_model.cpp`, `core/CMakeLists.txt`, `tools/CMakeLists.txt`, `render/CMakeLists.txt`, `ui/CMakeLists.txt`, `tests/core.cmake`, `tests/formats.cmake`, `tests/render.cmake`, `tests/tools.cmake`, `tests/ui.cmake`, `tests/bench.cmake`, talons `core/include/actcraft/core/{shapegeometry,shapeedits,gridsnap,pagesettings,pageedits,textedits}.h`, `core/src/{shapegeometry,shapeedits,gridsnap,pagesettings,pageedits,textedits}.cpp`, `tools/include/actcraft/tools/{shapetool,texttool}.h`, `tools/src/{shapetool,texttool}.cpp`, `tools/include/actcraft/tools/userstate.h`, `tools/src/userstate.cpp`, `tools/include/actcraft/tools/toolcontext.h`, `tools/src/toolregistry.cpp`, `render/include/actcraft/render/gridoverlayitem.h`, `render/src/gridoverlayitem.cpp`, `ui/include/actcraft/ui/{pagethumbnails,pagescontroller,pageoptions,textediting}.h`, `ui/src/{pagethumbnails,pagescontroller,pageoptions,textediting}.cpp`, `ui/include/actcraft/ui/toolhost.h`, `ui/src/toolhost.cpp`, `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`, `tests/tst_ui_document.cpp`, 16 tests talons `tests/tst_{core_shapes,core_gridsnap,core_pagesettings,core_pageedits,core_textedits,core_zones,formats_lot4,render_gridoverlay,render_verticaltext,tools_shape,tools_text,ui_shapes,ui_pagesettings,ui_pagethumbnails,ui_pages,ui_text}.cpp`, 7 scénarios talons `bench/scenarios/{formes,grille,texte,navigateur-pages,page-allongee-zones,texte-vertical,formes-texte-pages-grille}.json` | isolé | implementeur-tdd | `100% tests passed … out of N`, N ≥ N3 + 23, `TESTS OK`, `tst_actions_layout` (dont `pageMenuRead`, `lot4EntriesDeclared`, `noShortcutTwice`), `tst_core_model` (dont `lot4FieldsKeepOldFingerprints`), `tst_ui_document` (dont `pageContentChangedNamesThePage`, `showPageShowsIt`) Passed ; `premiers-pas` hors écran `CAPTURES OK` | S7, S15, S16 | #tache-l4-t1 |
| L4-T2 | Réservation II : coquille QML (panneau des navigateurs, palette des formes, barre de texte, surcouches de page, gestionnaires des commandes du lot) | L4-T1 | `ui/qml/Main.qml`, `ui/qml/PageArea.qml`, `ui/qml/ToolPalette.qml`, `ui/qml/PageContextMenu.qml`, `ui/qml/{BrowserPanel,PageNavigator,ShapePalette,TextFormatBar,TextEditor,VerticalTextBar,GridDesigner,BackgroundDialog,PageExtensionHandle,PageZonesOverlay}.qml`, `ui/CMakeLists.txt`, `tests/tst_ui_shell.cpp` | isolé | implementeur-tdd | `TESTS OK`, N ≥ N3 + 23, `tst_ui_shell` (dont `lot4ShellPlaceholders`, `everyLot4CommandHasAHandler`, `pageLayersInOrder`) Passed ; `premiers-pas` hors écran `CAPTURES OK` ; banc hors écran `content_check.ok = True` | S7, S15, S16 | #tache-l4-t2 |
| L4-T3 | `core` : catalogue des formes, boîte tracée (coin, centre, carré), lignes, points intelligents, commande de paramètres | L4-T0, L4-T1 | `core/include/actcraft/core/shapegeometry.h`, `core/src/shapegeometry.cpp`, `core/include/actcraft/core/shapeedits.h`, `core/src/shapeedits.cpp`, `tests/tst_core_shapes.cpp` | isolé | implementeur-tdd | `TESTS OK`, `tst_core_shapes` Passed (dont `catalogOrderAndDefault`, `dragFromCornerAndCenter`, `squarePresetsStaySquare`, `linePresetsConstrained`, `arrowHeadSmartPointBounded`, `setParametersIsOneStepAndRebuilds`) | S7, S7.1 | #tache-l4-t3 |
| L4-T4 | `core` : réglages de page (grille, fond, extension) et contrat de `Change` ignoré par le cache | L4-T0, L4-T1 | `core/include/actcraft/core/pagesettings.h`, `core/src/pagesettings.cpp`, `core/include/actcraft/core/undo.h`, `core/src/pagecache.cpp`, `tests/tst_core_pagesettings.cpp`, `tests/tst_core_pagecache.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_core_pagesettings` (dont `gridAboveChangeIsIgnoredByTheCache`, `gridBelowChangeRedrawsOnce`, `extendStepsByTenPercentOfBase`, `threeUndosRestoreTheHeight`, `designerChangesMerge`) et `tst_core_pagecache` Passed ; mutation de `gridDrawsUnderObjects` rouge puis remise | S7.2, S15 | #tache-l4-t4 |
| L4-T5 | `core` : accrochage à la grille (intersection la plus proche, coin bas gauche, ligne de base d'un texte) | L4-T0, L4-T1 | `core/include/actcraft/core/gridsnap.h`, `core/src/gridsnap.cpp`, `tests/tst_core_gridsnap.cpp` | isolé | implementeur-tdd | `TESTS OK`, `tst_core_gridsnap` Passed (dont `nearestOnNormalGrid`, `onlySnappingLevelsCount`, `invisibleGridStillSnaps`, `snapDisabledReturnsNothing`, `bottomLeftCornerLandsOnCrossing`, `textUsesItsBaseline`, `diagonalGridCrossings`) | S7, S7.2 | #tache-l4-t5 |
| L4-T6 | `formats` : natif — préréglage et paramètres des formes, texte de haut en bas, hauteur d'origine de la page | L4-T1 | `formats/src/nativejson.cpp`, `tests/tst_formats_lot4.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_formats_lot4` Passed (dont `presetAndParametersRoundTrip`, `defaultsAreOmitted`, `verticalTextRoundTrip`, `baseHeightRoundTrip`, `olderFileReadsWithDefaults`), `tst_formats_nativejson` et `tst_formats_nativefile` Passed | S7, S15, S16 | #tache-l4-t6 |
| L4-T7 | `render` : grille du dessus en surcouche `GridOverlayItem`, retirée de la scène du cache ; trait différé sur page à grille du dessus | L4-T1, L4-T2 | `render/include/actcraft/render/gridoverlayitem.h`, `render/src/gridoverlayitem.cpp`, `render/include/actcraft/render/fixedpageitem.h`, `render/src/fixedpageitem.cpp`, `ui/qml/PageArea.qml`, `tests/tst_render_gridoverlay.cpp`, `tests/tst_render_pendinglayer.cpp`, `tests/tst_render_fixedpage.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_render_gridoverlay` (dont `drawsOnlyTheGridAbove`, `hidingTheGridAboveRebuildsNoCache`, `sharpOnePixelLines`), `tst_render_pendinglayer` (dont `strokeOnGridAbovePageIsDeferred`), `tst_render_fixedpage` Passed ; banc hors écran `content_check.ok = True` | S7.2, S7.5 | #tache-l4-t7 |
| L4-T8 | `tools` : outil des formes (aperçu par la couche soulevée, coin ou centre, carré, Maj, accrochage, clic simple sans forme) | L4-T3, L4-T5 | `tools/include/actcraft/tools/shapetool.h`, `tools/src/shapetool.cpp`, `tests/tst_tools_shape.cpp` | isolé | implementeur-tdd | `TESTS OK`, `tst_tools_shape` Passed (dont `dragPreviewsWithoutExecuting`, `releaseExecutesOneAdd`, `singleClickAddsNothing`, `fromCenterSetting`, `shiftKeepsSquare`, `snapsPressAndRelease`, `cancelClearsThePreview`, `unfilledChoiceHonoured`) | S7, S7.1 | #tache-l4-t8 |
| L4-T9 | `tools` : sélection — accrochage au dépôt (coin bas gauche, ligne de base) | L4-T5 | `tools/src/selecttool.cpp`, `tools/include/actcraft/tools/selectiongeometry.h`, `tools/src/selectiongeometry.cpp`, `tests/tst_tools_select.cpp` | isolé | implementeur-tdd | `TESTS OK`, `tst_tools_select` Passed (dont `dropSnapsBottomLeftCorner`, `noSnapWithoutGridSnap`, `snapDragRefreshesTheCacheTwice`, tests du lot 3 inchangés) | S7, S7.2 | #tache-l4-t9 |
| L4-T13 | Réglages de grille : contrôleur de page (masquer, accrocher, créateur à niveaux), boîte du créateur, scénario `grille` | L4-T2, L4-T4, L4-T7 | `ui/include/actcraft/ui/pageoptions.h`, `ui/src/pageoptions.cpp`, `tests/tst_ui_pagesettings.cpp`, `ui/qml/GridDesigner.qml`, `bench/scenarios/grille.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_pagesettings` (dont `hideGridAboveRefreshesNoCache`, `hideGridBelowRefreshesOnce`, `snapToggleFollowsThePage`, `addRemoveResetLevels`, `designerSessionMakesASingleStep`), `tst_captures_grille` Passed ; `captures.ps1 -Scenario grille` hors écran : `LU appPageSettings.gridVisible=false OK`, `LU pageView.cacheRefreshCount=${avant} OK`, `CAPTURES OK` | S7.2 | #tache-l4-t13 |
| L4-T10 | `tools` : sélection — points intelligents d'une forme (affichés, glissés, une étape) | L4-T3, L4-T9 | `tools/src/selecttool.cpp`, `tools/include/actcraft/tools/selectiongeometry.h`, `tools/src/selectiongeometry.cpp`, `tests/tst_tools_select.cpp`, `ui/qml/SelectionOverlay.qml` | isolé | implementeur-tdd | `TESTS OK`, `tst_tools_select` Passed (dont `smartPointsShownForArrow`, `smartPointDragLiftsOnceAndDropsOneStep`, `noSmartPointForReadShapes`) ; banc hors écran joint | S7.1 | #tache-l4-t10 |
| L4-T11 | `ui::ToolHost` : outil des formes branché (choix, remplissage, coin ou centre, mémoire), contour des préréglages, points intelligents dans le cadre | L4-T2, L4-T8 | `ui/src/toolhost.cpp`, `ui/src/tooladapters.h`, `ui/src/tooladapters.cpp`, `tests/tst_ui_shapes.cpp` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_shapes` Passed (dont `shapeToolActivatesWithPalette`, `presetChoiceSurvivesRestart`, `outlineOfEveryPreset`, `smartPointsInSelectionFrame`, `shapeGoesThroughTheCacheInterface`) ; banc hors écran joint | S7, S7.1 | #tache-l4-t11 |
| L4-T12 | QML : palette des formes (nuancier, formes, sans remplissage, coin ou centre), curseur à miniature ; scénario `formes` | L4-T10, L4-T11 | `ui/qml/ShapePalette.qml`, `ui/qml/ToolCursor.qml`, `bench/scenarios/formes.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_captures_formes` Passed ; `captures.ps1 -Scenario formes` hors écran : `LU appTools.currentTool=shape OK`, `LU appDocument.shownObjectCount=2 OK`, `PIXEL rectangle-contour OK`, `CAPTURES OK` ; banc hors écran joint | S7.1 | #tache-l4-t12 |
| L4-T14 | Banc et joueur : scénarios de banc `forme-page-annotee` et `grille-dessus` ; étape `typeText` du joueur | L4-T7, L4-T11 | `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp`, `bench/src/runner.cpp`, `tests/tst_bench_scenarios.cpp`, `tests/tst_bench_result.cpp`, `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`, `tests/tst_bench_uiscenario.cpp`, `docs/banc.md` | isolé | implementeur-tdd | `TESTS OK`, `tst_bench_scenarios` (dont `shapeOnAnnotatedPageScenario`, `gridAboveScenarioDrawsStrokes`), `tst_bench_result` (dont `lot4ScenariosWithFrames`), `tst_bench_uiscenario` (dont `typeTextStepParsed`) Passed ; banc hors écran : scénarios `forme-page-annotee` et `grille-dessus` présents, `cache.refreshes` relevé (`grille-dessus` : 0 pendant les traits) | S7.5 | #tache-l4-t14 |

15 tâches dans ce sous-lot (16 dans 4b : 31 pour le lot). Tests ctest : L4-T1 enregistre d'un coup les **23** tests des
deux sous-lots (16 unitaires, 7 de captures), talons `QSKIP` jusqu'à ce que leur tâche les remplisse : **N3 + 23**
dès L4-T1, où N3 est le nombre de tests au départ du lot (fin du lot 3 et de L0-S, plus le lot 7 s'il a fusionné ;
relevé par L4-T0, étape 6, et écrit dans l'amendement du plan). Aucune autre tâche n'en enregistre. « ≥ » est la
règle. Un talon sauté ne prouve rien : chaque preuve nomme les fonctions attendues `PASS`.

Worktrees : `.worktrees/livraison-complete--L4-T<n>` (profil), tous isolés : deux tâches d'une même vague ne
partagent aucun fichier (vérifié sur la colonne « fichiers touchés »).

**Écran.** Aucune tâche de 4a n'ouvre de fenêtre visible. La clôture du lot (fin de 4b) joue les captures et le banc
à l'écran sous le jeton `ecran`, annoncés à Jérôme d'une ligne avant.

## Vagues (largeur 3 au plus ; 2 places de tests pour ce lot)

V1 : L4-T0 ‖ L4-T1 · V2 : L4-T2 ‖ L4-T3 ‖ L4-T4 · V3 : L4-T5 ‖ L4-T6 ‖ L4-T7 · V4 : L4-T8 ‖ L4-T9 ‖ L4-T13 ·
V5 : L4-T10 ‖ L4-T11 · V6 : L4-T12 ‖ L4-T14 · puis le point d'étape de 4a, puis 4b (son plan).

**Places.** Le jeton `tests` a 3 places sur la machine, dont **2 pour ce lot** (la session B, lot 7, garde la
troisième). Une vague de 3 tâches qui construisent en a toujours une qui attend la notification du jeton : c'est
prévu, l'agent attend, il ne sonde pas. L4-T0 ne construit rien.

**L4-T0 et les autres tâches.** L4-T1 (données, mécanique) et L4-T6 (sérialisation à nos noms) ne l'attendent pas
pour leur contenu ; toutes les tâches de comportement l'attendent (V2 commence quand V1 est finie). Ses décisions
D1 à D16 et sa table A1 à A14 **amendent ce plan et celui de 4b** avant V2 (relecture du plan exigée). Une décision
qui change une donnée de commande (raccourci, libellé, ordre de la palette des formes) passe par un correctif
`L4-C0` (`actions/data/commands.json`, `actions/data/layout.json` seulement), joué seul avant V2. Une API supposée
qui diffère du code livré (A1 à A14) amende la section de la tâche qui l'emploie.

Fichiers partagés tenus par les dépendances : `ui/qml/Main.qml`, `ui/qml/ToolPalette.qml`, `ui/qml/PageContextMenu.qml`, `ui/CMakeLists.txt`
(L4-T1 → L4-T2 → 4b) ; `ui/qml/PageArea.qml` (L4-T2 → L4-T7) ; `tools/src/selecttool.cpp`,
`selectiongeometry.*`, `tests/tst_tools_select.cpp` (L4-T9 → L4-T10) ; `ui/src/toolhost.cpp` (L4-T1 → L4-T11) ;
`ui/src/pageoptions.cpp` (L4-T1 → L4-T13 → 4b) ; `ui/qml/GridDesigner.qml` (L4-T2 → L4-T13) ;
`ui/qml/ShapePalette.qml` (L4-T2 → L4-T12) ; `ui/qml/SelectionOverlay.qml` (lot 3 → L4-T10) ;
`bench/src/scenarios.cpp`, `bench/src/runner.cpp` (L4-T14 → L4-T29) ; `core/include/actcraft/core/model.h`
(L4-T1 → L4-T27) ; `formats/src/nativejson.cpp` (L4-T6 → L4-T27) ; les talons de L4-T1 → leur tâche.

## Dépendances aux autres lots — API supposées (à revérifier à l'ouverture du lot, L4-T0)

| # | API supposée | Source (plan, tâche) | Employée par |
|---|---|---|---|
| A1 | `tools::Tool`, `tools::ToolContext` (`document`, `ink`, `strokes`, `lift`, `selection`, `view`, `commands`, `user`), `DocumentEditor::page/pageIndex/execute/lift/drop/isLifting`, `LiftSurface::show/setTransform/clear` | lot 3, L3-T1 étape 4, rempli par L3-T6 | L4-T8, L4-T9, L4-T10, L4-T22 |
| A2 | `tools::SelectionView` (`corners`, `handles`, `bar`, `handlesVisible`, `rubberBand`) ; `L4-T1` y ajoute `smartPoints` à la fin | lot 3, L3-T1 | L4-T10, L4-T11 |
| A3 | `tools::UserState` (`settings`, `setSettings`, `currentTool`, `selection`) et `tools::defaultSettings` ; `L4-T1` y ajoute `shapeSettings`, `textSettings` | lot 3, L3-T6 | L4-T8, L4-T11, L4-T21 |
| A4 | `tools::ToolRegistry::add`, `registerBuiltinTools` (« select », « pen », « highlighter », « eraser ») ; `L4-T1` y ajoute « shape », « text », « text-vertical » | lot 3, L3-T6 | L4-T1, L4-T11 |
| A5 | `ui::ToolHost` (type QML `Tools`, objet `appTools`), `activate`, `selectionFrame`, `settingsFile` | lot 3, L3-T14 | L4-T11, L4-T12 |
| A6 | `ui::DocumentController::execute`, `document()`, `liftObjects`, `dropLifted`, `editAboutToApply`, `shownObjectCount`, `setCacheObserver` ; `L4-T1` y ajoute `showPage`, `pageContentChanged` | lots 2 et 3 (L3-T12), socle L0-S (`document()` public) | L4-T1, L4-T13, 4b |
| A7 | `ui::PageView::tools`, `cacheRefreshCount`, `pageToScene`, `sceneToPage` (L7-T2, posé par L0-S ?), `addInputCover(QQuickItem*)` (L0-S) | lot 3 (L3-T13), socle L0-S | L4-T2, L4-T13, 4b |
| A8 | `core::objectAt`, `core::objectsTouching`, `core::kHitToleranceScreen` | lot 3, L3-T3 | L4-T10, L4-T21 |
| A9 | `core::TransformObjectsCommand`, `RemoveObjectsCommand`, `ReplaceObjectsCommand` (socle L0-S : une seule commande de remplacement) | lot 3, L3-T4 ; socle L0-S | L4-T9, 4b |
| A10 | Sous-menus `{"submenu": id, "label", "items"}` (jeton `>id`, `MenuSpec::submenus`) et `Layout::objectBar` ; filtrage par plateforme | socle L0-S (forme du lot 3) | L4-T1, L4-T20 |
| A11 | Joueur de scénarios : `UiStepKind` `Pinch`, `Touch`, `Pointer`, `DoubleClick` (ordre figé par L0-S), `expect` avec tolérance ; lignes `POINTEUR`, `DOUBLECLIC` | socle L0-S | L4-T12, L4-T14, 4b |
| A12 | Ordre des couches de `PageArea.qml` (`pageContent` : page fixe, `pendingLayer`, `liftLayer`, `ink`, `pageOverlays`) ; `selectionOverlay`, `toolCursor` ; `Main.qml` : `appTools` | socle L0-S, lot 3 (L3-T2) | L4-T2 |
| A13 | Contrat de `Change` ignoré par le cache posé pour `SetPagePresentationCommand` (relecture du lot 7, I3) | socle L0-S | L4-T4 |
| A14 | `PageContextMenu.qml` du lot 3 (`popupAt(x, y)`, entrées « Stylo » / « Sélectionner ») ; « Convertir en texte » ajouté par le lot 7 | lot 3 (L3-T16), lot 7 (L7-T34) | L4-T2, L4-T20 |

**Lot 7 (session B, en parallèle).** Fichiers chauds communs : `actions/data/commands.json`, `layout.json`,
`Main.qml`, `PageArea.qml`, `ToolPalette.qml`, `PageContextMenu.qml`, `ui/CMakeLists.txt`, `tests/*.cmake`,
`documentcontroller.*`. Règle du lot 3, reprise : les entrées du lot 4 forment des **blocs contigus** (commandes du
lot 4 groupées en fin de liste, groupes de palette nommés `formes-texte` et `pages`, entrées de menu en fin de
menu) ; le lot qui fusionne le second se remet à jour sur l'autre et juxtapose, sans réordonner. Les textes posés
par le lot 7 (`DocumentController::insertText`, L7-T2) sont des `core::Text` ordinaires : l'outil Texte du lot 4
les rouvre et les modifie (`textOfLot7IsEditable`, L4-T21). L'effet de changement de page du lot 7 ne joue pas sur
les sauts du navigateur de pages (`PagesController::goTo`), comme le dit le plan du lot 7.

**Lot 10.** Interface de cache inchangée. Limite de DA11 tenue (contrainte 1) ; levée pour la grille du dessus
(L4-T7) ; scénarios de banc demandés (« forme sur page annotée », « grille au-dessus ») livrés par L4-T14.

**Lots 5, 6, 11.** Points de forme (F-527), couper / copier / coller d'objets, onglets de documents (glisser des
pages vers un autre onglet, F-011, F-803) : lot 5. Navigateurs d'objet, de notes, de propriétés, de ressources,
d'action, panneau flottant, persistance des réglages, profils : lot 6. Les icônes de ces navigateurs sont posées
**grisées** dans la rangée du panneau dès le lot 4 (F-040), sans navigateur derrière. Mise en page étroite : lot 11.

## Ce que l'ancien dépôt dit déjà (relevé pour ce plan, reformulé)

Lu dans `C:\Users\JeromeDenis\REA - Activinspire` ; L4-T0 le complète par le corpus et par le binaire.

1. **Formes.** L'application de référence offre 22 formes, dans cet ordre : ligne, ligne horizontale, ligne
   verticale, ligne brisée, zigzag, chevron, rectangle, rectangle arrondi, carré, ellipse, cercle, triangle,
   losange, pentagone, hexagone, étoile, flèche, bulle, octogone, étoile à quatre branches, explosion, explosion à
   douze pointes (`app/actcraft/canvas/items.py:421-428`). Défaut : la ligne. Carré et cercle restent carrés ;
   Maj garde un carré, ou contraint une ligne à des multiples de 45° ; ligne horizontale et verticale fixent un
   axe (`canvas/tools.py:822-868`). Un simple clic (moins de 3 px) ne pose aucune forme. Le point d'appui et le
   point courant sont accrochés à la grille pendant le tracé. La forme va sur la couche intermédiaire.
2. **Texte.** Cliquer avec l'outil Texte sur un texte l'édite ; ailleurs, un texte neuf au point cliqué, texte de
   départ tout sélectionné, police et taille courantes (`canvas/tools.py:942-960`). Barre de mise en forme :
   `canvas/textbar.py`. Le texte de départ de la référence n'est pas repris : le nôtre est « Votre texte » (N1-12).
3. **Grille.** Accrochage : intersection la plus proche parmi les niveaux qui accrochent ; sans accrochage autorisé,
   le point reste tel quel ; grilles en losanges : recherche parmi les croisements voisins
   (`canvas/grid.py:221-251`). Lignes nettes de 1 px (lot 10, P2).
4. **Pages.** Réordonner : une vignette déposée sur une autre prend sa place et devient courante ; insertion qui
   garde le défilement des vignettes (`docs/tests/plan-comparatif.md:24`).
5. **Formes palette.** Panneau ancré à droite qui réduit la zone des pages, nuancier de 2 × 4, formes au pas de
   38 px (`docs/tests/plan-comparatif.md:18`). Nos dimensions viennent de notre thème (décision 23).

## Écarts au cadrage (repris à la porte de fin du lot 4)

1. **Volume.** 20 tâches estimées (amendement § 6) → 31, en deux sous-lots : deux réservations, la vérification
   des API, la grille en surcouche (limite de DA11), deux scénarios de banc, le contrôleur de miniatures, la boîte
   du fond (écart E2). Une tâche « principes » de plus est probable (amendement § 4 : « lot 4 + 1 ») : L4-T0 la
   crée par l'amendement si un principe du corpus n'entre dans aucune tâche (fichiers fixés par l'amendement).
2. **Courbe de Bézier et forme libre** (F-240 les cite ; la référence ne les a pas) : L4-T0 relève si la 3.5.18
   les a dans sa palette ; si oui, l'amendement ajoute une tâche `tools` (tracé en plusieurs clics), sinon H4-2.
3. **Panneau flottant** (F-041) : ancrage à gauche ou à droite au lot 4 ; flottant au lot 6 avec la palette
   flottante (même mécanique) — H4-5.
4. **Presse-papiers des pages** interne au document au lot 4 ; entre documents et onglets au lot 5 (F-011).
5. **Entrées du menu de page hors lot** (capture de bureau : lot 7 ; exporter la page : lot 5 ; ajouter à la
   bibliothèque, plus de modèles : lot 6 ; question : exclue) : absentes du menu jusqu'à leur lot.
6. **Écart E10** (taille personnalisée en hauteurs de page) : tranché par L4-T0 ; s'il diffère de F-700, correctif
   `L4-C1` (boîte `ui/qml/PageSizeDialog.qml` et sa commande), ajouté par l'amendement.

## Scénarios ajoutés

- **S7.1 — Formes (lot 4).** `captures.ps1 -Scenario formes` : palette ouverte par `tool.shape` ; rectangle tracé
  du coin (contour noir de 4 relevé par `PIXEL`) ; cercle tracé du centre (`shape.draw-from-center`), resté rond ;
  un clic simple ne pose rien (nombre d'objets relu) ; « sans remplissage » : l'intérieur reste blanc ; flèche
  sélectionnée : son point intelligent est visible et le glisser élargit la tête (une étape, `appDocument.canUndo`).
- **S7.2 — Grille (lot 4).** `captures.ps1 -Scenario grille` : créateur de grille ouvert, pas de 50, accrochage ;
  un rectangle déplacé pose son coin bas gauche sur une intersection (`PIXEL`) ; « Masquer la grille » sur une
  grille du dessus : `pageView.cacheRefreshCount` inchangé ; sur une grille du dessous : + 1.
- **S7.3 — Texte (lot 4, 4b).** Texte posé par touches injectées, mis en gras par la barre, validé hors de la zone ;
  un texte vide n'est pas gardé ; un texte existant rouvert et modifié en une étape.
- **S7.4 — Navigateur de pages (lot 4, 4b).** Quatre pages ; Maj+clic sélectionne une plage, Ctrl+clic en retire
  une ; glisser deux pages en tête ; couper, coller avant la page active ; dupliquer ; menu de page ▸ Insérer après.
- **S7.5 — Banc du lot 4.** `bench.ps1` : `forme-page-annotee`, `grille-dessus`, `miniatures` (4b) mesurés comme
  les autres, `cache.refreshes` et figements relevés ; résultat, pas porte (DA7) ; hors limite en D3D11 à la
  clôture : H4-8 décide (tuiles L10-T7, cache par groupe de couches, ou tâche corrective).

## Couverture des scénarios (4a et 4b)

| Scénario | Tâches | Joué à la clôture |
|---|---|---|
| S7 — formes, texte, pages, grille | L4-T1, T2, T3, T5, T6, T8, T9, T11 ; 4b : T15 à T19, T21 à T23, T30 | oui : hors écran et à l'écran en D3D11 (`formes-texte-pages-grille`) |
| S7.1 — formes | L4-T3, T8, T10, T11, T12 | oui |
| S7.2 — grille | L4-T4, T5, T7, T9, T13 | oui |
| S7.3 — texte | 4b : L4-T16, T21, T22, T23 | oui |
| S7.4 — navigateur de pages | 4b : L4-T15, T17, T18, T19, T20 | oui |
| S7.5 — banc du lot 4 | L4-T7, T14 ; 4b : T29 | oui : banc complet D3D11 plein écran |
| S15 — page allongée et zones | L4-T1, T2, T4, T6 ; 4b : T26, T27, T28 | oui |
| S16 — texte de haut en bas | L4-T1, T2, T6 ; 4b : T24, T25 | oui |
| S30 — tâche d'ouverture | L4-T0 | contrôlé à la relecture : rapport, relevé brut, `NOMS OK` |
| S13 (lot 10) — non-régression du cache | toutes les tâches de `render/ ui/ tools/` | oui : `pauses`, `cache-differe`, S6 rejoués |

## Carte des fichiers

| Fichier | Responsabilité | Talon / API (L4-T1) | Rempli par |
|---|---|---|---|
| `docs/verifications/2026-10-18-lot4-principes.md` | constats, décisions D1 à D16, table A1 à A14, N3 | — | L4-T0 |
| `actions/data/commands.json`, `layout.json` | 47 commandes du lot ; menus Insérer (sous-menu « Page »), Outils, Affichage ; palette ; `pageMenu` | L4-T1 | L4-T1 |
| `actions/…/layout.h`, `actions/src/layout.cpp` | `Layout::pageMenu` | L4-T1 | L4-T1 |
| `core/…/model.h`, `core/src/model.cpp` | `Shape::preset`, `Shape::parameters`, `Text::vertical`, `Page::baseHeight` ; empreinte | L4-T1 | L4-T1 (zones : L4-T27) |
| `core/…/shapegeometry.h`, `shapeedits.h` | catalogue, boîte, lignes, points intelligents, `SetShapeParametersCommand` | talon | L4-T3 |
| `core/…/pagesettings.h` | `SetGridCommand`, `SetBackgroundCommand`, `ExtendPageCommand`, `gridDrawsUnderObjects` | talon | L4-T4 |
| `core/…/gridsnap.h` | `nearestGridPoint`, `snapAnchor`, `snapDelta` | talon | L4-T5 |
| `core/…/pageedits.h` | `MovePagesCommand`, `RemovePagesCommand`, `InsertPagesCommand`, `withFreshIds` | talon | L4-T15 |
| `core/…/textedits.h` | `SetTextCommand`, `textBaseline` | talon | L4-T16 |
| `formats/src/nativejson.cpp` | clés `preset`, `parameters`, `vertical`, `baseHeight` | — | L4-T6 (zones : L4-T27) |
| `formats/src/flipchart.cpp`, `flipchartids.h` | texte de haut en bas lu | — | L4-T24 |
| `render/…/gridoverlayitem.h` | surcouche de la grille du dessus (type QML `GridOverlay`) | talon | L4-T7 |
| `render/…/fixedpageitem.*` | grille du dessus retirée ; différé rétabli | — | L4-T7 |
| `render/src/pagepainter.cpp`, `paintedrenderer.cpp` | texte de haut en bas dessiné | — | L4-T24 |
| `tools/…/shapetool.h`, `texttool.h` | outils Forme, Texte, Texte de haut en bas | talon | L4-T8, L4-T22 |
| `tools/…/userstate.h`, `toolcontext.h`, `toolregistry.cpp` | `ShapeSettings`, `TextSettings`, `SelectionView::smartPoints`, outils inscrits | L4-T1 | L4-T1 |
| `tools/src/selecttool.cpp`, `selectiongeometry.*` | accrochage au dépôt, points intelligents | — | L4-T9, L4-T10 |
| `ui/…/toolhost.h`, `ui/src/toolhost.cpp`, `tooladapters.*` | réglages de forme, contour des préréglages, points intelligents | API | L4-T11 |
| `ui/…/pageoptions.h` | grille, fond, extension, zones (type QML `PageSettings`, objet `appPageSettings`) | API | L4-T13, L4-T20, L4-T26, L4-T28 |
| `ui/…/pagethumbnails.h` | miniatures (types QML `PageThumbnails`, `ThumbnailImage`) | API | L4-T17 |
| `ui/…/pagescontroller.h` | pages (type QML `Pages`, objet `appPages`) | API | L4-T18 |
| `ui/…/textediting.h` | édition du texte (type QML `TextEditing`, objet `appText`) | API | L4-T21, L4-T25 |
| `ui/…/documentcontroller.h` | `showPage`, `pageContentChanged` | L4-T1 | L4-T1 |
| `ui/qml/*.qml` du lot | coquille, puis contenu | talon `Item {}` | L4-T2, puis leur tâche |
| `bench/src/scenarios.cpp`, `runner.cpp`, `uiscenario.*` | banc du lot, étape `typeText` | — | L4-T14, L4-T29 |
| `bench/scenarios/*.json` (7) | scénarios d'interface du lot | talon (`waitReady` seul) | leur tâche |
| `tests/tst_*.cpp` (16 neufs) | un par fichier neuf | talon `QSKIP` | la tâche du fichier |

## Contrats partagés entre tâches (noms figés)

Tous les noms des lots 1, 2, 3, 10 et de L0-S restent figés ; on ajoute, on ne renomme pas. Les en-têtes publics
du lot sont **créés tels quels par L4-T1** (sa section, étape 5, donne leur texte complet) ; une tâche suivante les
lit dans son worktree, ne change pas une signature publique sans amendement du plan, et ajoute librement des
membres **privés**.

- Commandes (47, identifiants anglais, libellés à nous) : `tool.shape` (Ctrl+Maj+S, cochable), `shape.choose`
  (texte), `shape.filled` (cochable), `shape.fill-color` (texte), `shape.draw-from-center` (cochable), `tool.text`
  (Ctrl+T, cochable), `tool.text-vertical` (cochable), `text.font` (texte), `text.size` (nombre), `text.size-up`,
  `text.size-down`, `text.bold`, `text.italic`, `text.underline`, `text.superscript`, `text.subscript`,
  `text.color` (texte), `text.background` (texte), `text.align-left`, `text.align-center`, `text.align-right`,
  `text.spacing-up`, `text.spacing-down`, `text.indent`, `text.outdent`, `text.bullets`, `text.select-all`,
  `text.symbol` (texte), `text.edit-at` (sans libellé de menu ; argument `{x, y, vertical}`), `view.browsers`
  (Ctrl+B, cochable), `browsers.dock-left`, `browsers.dock-right`, `page.goto` (nombre), `page.insert-before`,
  `page.insert-after` (Ctrl+I), `page.cut`, `page.copy`, `page.paste`, `page.duplicate`, `page.delete`,
  `page.background`, `grid.designer` (Ctrl+Maj+G), `grid.hide` (cochable), `grid.snap` (cochable),
  `view.page-extension` (cochable), `page.extend`, `page.zones` (texte `"<horizontales>x<verticales>"`). Aucune
  commande de page n'a Ctrl+X/C/V/D ni Suppr : ces raccourcis restent à l'édition d'objets (lots 3 et 5) ; dans le
  navigateur de pages, le QML les traduit en commandes de page quand il a le focus.
- Identifiants d'outil : `shape`, `text`, `text-vertical` (en plus de ceux du lot 3).
- Préréglages de forme (ordre de la palette, D1) : `line`, `line-horizontal`, `line-vertical`, `polyline`,
  `zigzag`, `chevron`, `rectangle`, `rounded-rectangle`, `square`, `ellipse`, `circle`, `triangle`, `diamond`,
  `pentagon`, `hexagon`, `star`, `arrow`, `callout`, `octagon`, `star-four`, `burst`, `burst-twelve`.
- Objets QML lus par les scénarios : `appTools` (`currentTool`, `shapePreset`, `shapeFilled`,
  `shapeFromCenter`), `appDocument` (`shownObjectCount`, `pageCount`, `currentIndex`, `pageSize`, `canUndo`),
  `appPageSettings` (`gridVisible`, `gridSnap`, `gridAbove`, `pageHeight`, `zones`), `appPages`
  (`selection`, `canPaste`), `appThumbnails` (`renderCount`, `pendingCount`), `appText` (`active`, `vertical`,
  `bold`, `plainText`), `pageView` (`cacheRefreshCount`), `browserPanel` (`visible`, `side`), `shapePalette`,
  `textFormatBar`, `verticalTextBar`, `gridDesigner`, `backgroundDialog`, `pageExtensionHandle`, `pageZones`,
  `gridOverlay`.
- Ordre des couches dans `PageArea.qml` (A12, à revérifier) : page fixe et cache, `pendingLayer`, `liftLayer`,
  **`gridOverlay`**, `ink`, `pageOverlays` (où vont `pageZones` et `pageExtensionHandle`) ; `textEditor` à côté de
  `selectionOverlay`.
- Constantes : `core::kShapeMinDragScreen = 3.0` ; `core::kExtendRatio = 0.10` ; `core::kMergeGrid = 4` ;
  `ui::PageThumbnails::kSettleMs = 400` ; `ui::PageThumbnails::kDefaultWidth = 160`.

## Règles communes à toutes les tâches (jointes à chaque section par l'orchestrateur)

Celles des plans 2a, 2b, 10 et 3 s'appliquent toutes, mot pour mot
(`docs/plans/2026-10-15-livraison-complete-lot2a-PLAN.md`, `…-lot2b-PLAN.md`, `…-lot10-PLAN.md`,
`…-lot3-PLAN.md`, « Règles communes ») : processus enfant pour toute commande PowerShell, `git -C`, `format.ps1`
avant la preuve, pièges de Clang (`-Wshadow`, `-Wswitch` : tout `switch` sur une énumération étendue est complété,
conversions explicites, pas d'agrégat partiel par position), pas de `std::visit` direct sur `PageObject`, **aucune
API privée de Qt**, propriété intellectuelle, `samples/` en lecture seule, commits explicites en français terminés
par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`, arrêt par PID, fils de Qt Quick (tout état lu par
`updatePaintNode` est écrit sur le fil principal), moc et chaînes brutes à apostrophe, talons remplacés en gardant
les déclarations publiques de L4-T1 (la fonction `reserved()` du test talon est supprimée). S'y ajoutent :

- **Graphe d'abord** : `graphify query "…"` dans le worktree avant de lire les fichiers ; pour l'application de
  référence, `graphify query "…"` lancé **dans** `C:\Users\JeromeDenis\REA - Activinspire`.
- **Jeton** `C:\Users\JeromeDenis\.claude\scripts\jeton.ps1` : `-Nom tests` pour construire, tester, jouer le banc
  ou les captures hors écran ; `-Nom ecran` pour toute fenêtre visible ; une commande par prise ; attendre la
  notification, ne pas sonder.
- `<wt>` = `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L4-T<n>` ; `<id>` = `L4-T<n>`.
- **Commande ciblée** (rouge, vert d'un seul test) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande '. ./scripts/dev-env.ps1; cmake --build --preset debug *> .tmp/build.txt; ctest --preset debug -R ''<motif>'' --output-on-failure *> .tmp/ctest.txt'; Select-String -Path .tmp/build.txt,.tmp/ctest.txt -Pattern 'error|tests passed|tests failed|No tests were found' | Select-Object -Last 4"
  ```
  Premier passage dans un worktree neuf : la même commande avec `-Commande 'pwsh -NoProfile -File ./scripts/build.ps1 *> .tmp/build.txt'`.
- **Fonctions d'un test** :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; . ./scripts/dev-env.ps1; & build/debug/tests/<test>.exe -o .tmp/<test>.txt,txt; Select-String -Path .tmp/<test>.txt -Pattern '^PASS   :|^FAIL!  :|^SKIP   :|^Totals' | Select-Object -Last 40"
  ```
  Attendu pour un talon rempli : chaque fonction nommée par la table en `PASS`, aucun `SKIP`,
  `Totals: <k> passed, 0 failed, 0 skipped`, k ≥ 1.
- **Preuve de tâche** (forme bornée du profil, sous le jeton) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1'; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2; Select-String -Path .tmp/test.txt -Pattern '<nom_du_test> .*Passed' | Select-Object -Last 1"
  ```
  Attendu : `100% tests passed, 0 tests failed out of N` (N ≥ N3 + 23 dès L4-T1), `TESTS OK`, puis la ligne
  `Passed` du test nommé.
- **Banc sans écran** (toute tâche qui touche `render/`, `input/`, `tools/` ou `ui/` ; résultat, pas porte) : la
  commande et la lecture du JSON du plan du lot 3 (« Banc sans écran »), avec `-Proprietaire <id>`. Attendu :
  `BUDGET TENU` ou `BUDGET DEPASSE` (jamais `MESURE INVALIDE`), `content_check.ok = True` ; nom du JSON joint.
- **Preuve visuelle hors écran** (jeton `tests`) : lectures `-Lire "<objet>.<propriété>=<attendu>"` d'abord, puis
  description des captures :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario <nom> -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire ''<objet.propriété=attendu>'' > .tmp/captures.txt 2>&1'; Select-String -Path .tmp/captures.txt -Pattern '^(COMMANDE|TRAIT|TOUCHE|TEXTE|DOIGT|PINCEMENT|POINTEUR|DOUBLECLIC|ATTENTE|PRET|FENETRE|LU|PIXEL|BORD|CAPTURE) |^SCENARIO |CAPTURES (OK|EN ECHEC)' | Select-Object -Last 16"
  ```
  Motifs d'échec : `CAPTURES EN ECHEC`, `SCENARIO EN ECHEC`, `SCENARIO INVALIDE`, toute ligne `ECART` ou `ECHEC`,
  `FENETRE …`. La ligne `TEXTE` existe après L4-T14.
- **Mutation** (toute tâche qui en demande une) : `Copy-Item` garde l'ancienne date du fichier et ninja ne
  recompile pas. Après avoir posé ou retiré la mutation, remettre la date
  (`(Get-Item <fichier>).LastWriteTime = Get-Date`) ou remettre le fichier par `git -C <wt> checkout -- <fichier>`
  (qui la change). Constater le rouge, remettre, constater le vert.
- **Dépôt public** : le contrôle des noms ne lit que les fichiers suivis par git. Avant de le lancer, `git -C <wt>
  add` de chaque fichier neuf ou modifié ; puis
  `python -I "C:/Users/JeromeDenis/REA - Activinspire/tools/controle_noms_actcraft.py" <wt>` → `NOMS OK` ; le
  relancer après le commit. Libellés à nous (N1-12), aucun nom de l'original, aucune icône d'origine, aucun titre de
  vidéo, aucun nom de paperboard réel.

---

## Tâches

<a id="tache-l4-t0"></a>
### Tâche L4-T0 — Ouverture : vidéos, REA et ancien dépôt confrontés au lot ; API revérifiées

**Objectif.** Décision 0100 et règle `Lx-T0` (amendement § 4), sur le modèle de L10-T0
(`docs/verifications/2026-10-18-lot10-principes.md`) : relever dans le corpus tous les principes des fonctions du
lot (formes, palette, points intelligents, texte, barre de mise en forme, texte de haut en bas, panneau des
navigateurs, navigateur de pages, menu de page, fond de page, grille, accrochage, page allongée, zones), plus les
**8 constats** affectés au lot 4 par le tableau du § 4 de l'amendement (1 « Interface et profils », 3 « Stylo,
formes, couleurs », 1 « Texte et reconnaissance », 3 « Pages, fond, grille ») et les écarts **E1, E2, E10, E12** ;
trancher sur la 3.5.18 par REA ; écrire en nos mots les exigences, les **décisions D1 à D16**, puis **revérifier
les API A1 à A14** contre le code livré par les lots 3, L0-S (et 7 s'il a fusionné).

**Décisions à rendre** (chacune : valeur retenue et source, ou « non tranché » → hypothèse de niveau 1) :
- D1 — Liste et ordre des formes de la palette de la 3.5.18 ; défaut ; courbe de Bézier et forme libre présentes
  ou non (écart 2).
- D2 — Remplissage à la création (E1) : rempli par défaut, choix « sans remplissage », couleur de remplissage par
  défaut.
- D3 — Contour par défaut (couleur courante ou noir ; 4 px) ; nom par défaut d'une forme.
- D4 — Points intelligents : quelles formes en ont, combien, bornes (tête de flèche, rayon des coins, creux des
  étoiles, pointe de la bulle).
- D5 — Tracé depuis le centre : réglage gardé où (réglage de l'application ou de la palette) ; Maj.
- D6 — Accrochage (F-622) : bascule « Accrocher à la grille » = « Autoriser l'accrochage » de la grille de la page,
  ou état propre de l'application ; ancrage (coin bas gauche ; ligne de base d'un texte) ; forme en cours de tracé.
- D7 — Texte : police et taille par défaut (N2-3), couleur, fond ; cadre et poignées ; texte vide ; validation.
- D8 — Barre de mise en forme (F-100) : contenu et ordre, pas de la taille (2 points), interligne et retrait
  (pas), symboles offerts, place de la barre (haut ou bas).
- D9 — Texte de haut en bas (F-301 b) : lettres empilées ou ligne tournée ; barre propre ; lecture du type dans les
  `.flipchart` (identifiants à décrire dans notre documentation hors dépôt).
- D10 — Page allongée (F-301 a, E12) : place de la commande, pas de 10 % fixe (hauteur d'origine) ou cumulé, la
  poignée reste-t-elle d'une page à l'autre, une étape d'annulation par clic.
- D11 — Zones de page (F-603) : comportement entier (dessin des séparations, effet sur les objets, l'accrochage, la
  présentation), réglage, valeurs bornées. **Aucune ligne de code avant cette décision.**
- D12 — Panneau des navigateurs : icônes et ordre (sans le vote), ancrage, Ctrl+B, largeur minimale et maximale,
  menu propre à chaque navigateur.
- D13 — Navigateur de pages : taille des miniatures (réglette), sélection multiple, fantôme d'insertion, pages
  restées sélectionnées après le dépôt, coller avant la page active, menu sur plusieurs pages.
- D14 — Menu de page (F-771) : entrées et ordre de la 3.5.18 pour les fonctions du lot.
- D15 — Fond de page (F-610, E2) : un bouton « Terminé » ou appliquer puis valider ; options du lot 4 (couleur,
  dégradé, image) ; les autres (capture, superposition de bureau) au lot 7.
- D16 — E10 : taille personnalisée d'un nouveau paperboard (pixels ou hauteurs de page) ; correctif `L4-C1` si la
  3.5.18 diffère de F-700.

**Sources.**
- Corpus : `C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\README.md` (« Principes absents »,
  « Écarts avec la spec ») et ses fiches `*.md`.
- Spécification : `C:\Users\JeromeDenis\REA - Activinspire\docs\spec\fonctionnalites.md` (F-040 à F-042, F-100,
  F-240 à F-242, F-250 à F-253, F-301, F-600 à F-603, F-610, F-620 à F-623, F-700, F-771, F-800 à F-803).
- Plan comparatif : `docs\tests\plan-comparatif.md`, lignes 18, 20, 24 et 33 (formes, texte, pages, grille).
- Ancien dépôt : `app/actcraft/canvas/grid.py`, `canvas/items.py`, `canvas/tools.py`, `canvas/textbar.py`,
  `canvas/poly.py`, `ui/navigator.py`, `ui/grid_designer.py` ; son graphe.
- Format : `docs\re\format-flipchart.md` (§ 6, § 7.4, § 7.5) — notre documentation.
- Binaire : skill `reverse-engineer-anything` sur `C:\Program Files\Activ Software\Inspire\`, **en statique
  seulement** (ressources des boîtes, chaînes, valeurs par défaut, raccourcis, types du flux objet). Aucune
  observation à l'écran sans prévenir Jérôme (mode autonome, point 3) ; si une décision l'exige, la marquer
  « non tranché, observation à l'écran demandée » et continuer.
- API : `graphify query "ToolContext LiftSurface SelectionView"`, `graphify query "ToolHost selectionFrame"`,
  `graphify query "UiStepKind Pointer DoubleClick"`, `graphify query "PageArea couches pendingLayer liftLayer"`,
  `graphify query "SetPagePresentationCommand Change"` dans le worktree, puis lecture des en-têtes trouvés.

**Fichiers.**
- Créer : `docs/verifications/2026-10-18-lot4-principes.md` (dans le dépôt, nos mots).
- Créer hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot4.md` (relevé brut).

**Test et commande bornée.** Pas de code. Preuves :
```powershell
Test-Path 'C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot4.md'   # True
git -C "C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L4-T0" add docs/verifications/2026-10-18-lot4-principes.md
python -I "C:/Users/JeromeDenis/REA - Activinspire/tools/controle_noms_actcraft.py" "C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L4-T0" | Select-Object -Last 1   # NOMS OK : <n> fichiers
pwsh -NoProfile -Command "`$exclus = @('Boutons définis par l''utilisateur', 'Nouveau paperboard'); Get-ChildItem 'C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\*.md' -Exclude README.md | ForEach-Object { `$t = (Get-Content -LiteralPath `$_.FullName -TotalCount 5 -Encoding utf8 | Where-Object { `$_ -like '# *' } | Select-Object -First 1); if (`$t) { `$t = `$t.Substring(2).Trim(); if (`$exclus -notcontains `$t) { git -C 'C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L4-T0' grep -l -i -F -- `$t } } }"   # aucune sortie
```

**Interdits.** Aucun titre, aucune transcription, aucune citation de vidéo dans le dépôt (identifiants `Vnn`).
Aucun nom de l'original (classes, propriétés, libellés), aucun texte de l'aide, aucune capture. Aucune ligne de
code. Ne modifier ni `samples/` ni le corpus. Pas d'observation à l'écran sans annonce.

**Terminé quand.** Le rapport a une ligne par constat (principe → exigence en nos mots → tâche du lot 4 →
scénario, ou raison et lot destinataire), une section « Décisions » D1 à D16, une section « API » A1 à A14
(conforme, ou écart : signature réelle et tâche à amender), et la valeur **N3** (`ctest --preset debug -N` sur
`feat/livraison-complete` à jour : `Total Tests: <N3>`) ; le relevé brut existe et est cité ; `NOMS OK` ; la commande
des titres ne sort rien ; le rapport de tâche liste pour l'orchestrateur les amendements (tâche visée), les
correctifs `L4-C0` et `L4-C1`, et la tâche « principes » éventuelle (titre, fichiers, preuve).

**Étapes.**
- [ ] **Étape 1 : relever le corpus.**
  ```powershell
  Select-String -Path 'C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\*.md' -Pattern 'forme|rectangle|cercle|flèche|ligne|remplissage|contour|texte|police|gras|souligné|vertical|navigateur|miniature|vignette|page|réordonn|insérer|dupliquer|grille|accroch|aimant|fond|allong|extension|cadre|zone' -List | Select-Object -ExpandProperty Path
  ```
  Pour chaque fiche retenue : numéro `Vnn` (colonne « # » du `README.md`) et principe observé, en une phrase à nous.
- [ ] **Étape 2 : relever l'ancien dépôt.** Dans `C:\Users\JeromeDenis\REA - Activinspire` :
  `graphify query "outil des formes tracé carré ligne"`, `graphify query "outil texte barre de mise en forme"`,
  `graphify query "navigateur de pages vignettes réordonner"`, `graphify query "grille accrochage niveaux"` ;
  confirmer ou corriger le § « Ce que l'ancien dépôt dit déjà ».
- [ ] **Étape 3 : REA statique.** Palette des formes (liste, ordre, défaut, remplissage), points intelligents,
  valeurs par défaut du texte, contenu de la barre, texte de haut en bas (type du flux objet, propriétés), commande
  d'extension de page et son pas, zones de page (propriétés de page, dessin), boîte du fond, taille personnalisée.
  Constats bruts, noms de l'original compris, dans le relevé hors dépôt seulement.
- [ ] **Étape 4 : écrire le rapport.**
  ```markdown
  # Lot 4 — principes confrontés (vidéos, ancien dépôt, binaire) et API revérifiées

  Relevé brut (hors dépôt) : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot4.md`. Références `Vnn` :
  numéros des fiches du corpus. Constats écrits par nous. Tâche L4-T0, scénario S30.

  ## Constats

  | # | Principe (nos mots) | Source | Exigence pour ActCraft | Tâche | Scénario | Écart, raison ou lot destinataire |
  |---|---|---|---|---|---|---|
  | P1 | Un simple clic avec l'outil des formes ne pose aucune forme | ancien dépôt (outil des formes), Vnn… | aucune forme sous 3 px d'écran | L4-T8 | S7.1 | — |
  | … | | | | | | |

  ## Décisions

  | # | Question | Décision | Source | Tâche amendée |
  |---|---|---|---|---|
  | D1 | Liste et ordre des formes | … | … | L4-T3, L4-C0 |
  | … | | | | |

  ## API des lots 3, 7 et L0-S

  | # | API supposée par le plan | Code livré (fichier:ligne) | Conforme | Tâche à amender |
  |---|---|---|---|---|
  | A1 | `tools::ToolContext` … | `tools/include/actcraft/tools/toolcontext.h:…` | oui / non : … | … |
  | … | | | | |

  Tests au départ du lot : N3 = … (`ctest --preset debug -N`).
  ```
- [ ] **Étape 5 : API.** Pour A1 à A14 : `graphify query` (sources ci-dessus), puis lecture de l'en-tête ;
  recopier la signature réelle quand elle diffère.
- [ ] **Étape 6 : N3.**
  ```powershell
  pwsh -NoProfile -Command "Set-Location 'C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L4-T0'; . ./scripts/dev-env.ps1; cmake --preset debug *> .tmp/cfg.txt; ctest --preset debug -N | Select-String 'Total Tests'"
  ```
  (sous le jeton `tests`, comme la commande ciblée).
- [ ] **Étape 7 : contrôles.** Les commandes de « Test et commande bornée » ; corriger jusqu'à `NOMS OK` et sortie
  vide.
- [ ] **Étape 8 : commit**, puis relancer le contrôle des noms.
  ```powershell
  git -C <wt> add docs/verifications/2026-10-18-lot4-principes.md
  git -C <wt> commit -m "Lot 4 : principes des formes, du texte, des pages et de la grille confrontés aux vidéos, à l'ancien dépôt et au binaire ; API revérifiées (L4-T0)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t1"></a>
### Tâche L4-T1 — Réservation I : commandes, disposition, icônes, modèle, CMake, tests, en-têtes figés et talons

**Objectif.** Poser d'un coup tout ce que les tâches des deux sous-lots partagent, pour qu'aucune vague n'écrive
ensuite dans un fichier chaud en même temps qu'une autre : les 47 commandes et leur disposition (`pageMenu`
compris), 34 icônes à nous, les champs du modèle (avec leur empreinte), les enregistrements CMake, les 23 tests
(talons sautés), les en-têtes publics figés et leurs talons, deux ajouts réels à `DocumentController`.

**Fichiers.** Ceux de la ligne L4-T1 de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_actions_layout|tst_core_model|tst_ui_document'`, puis preuve
de tâche (N ≥ N3 + 23, `TESTS OK`), puis `premiers-pas` hors écran (`-Lire "pageIndicator.text=1 / 1"`) :
`CAPTURES OK`.

**Interdits.** Aucun comportement dans un talon (valeur par défaut seulement). Aucune modification d'une
signature publique des lots 1 à 3, 10 et L0-S (ajouts seulement, à la fin). Aucune icône copiée.

**Terminé quand.** Les trois tests nommés passent avec leurs fonctions neuves ; les 23 tests du lot sont listés par
`ctest -N` (16 talons `SKIP`, 7 scénarios réduits à `waitReady` qui passent) ; N ≥ N3 + 23 ; `TESTS OK` ;
`premiers-pas` passe ; `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge de la disposition.** Ajouter à `tests/tst_actions_layout.cpp` :
  ```cpp
  void pageMenuRead()
  {
      const auto layout = actcraft::actions::Layout::fromJson(QByteArrayLiteral(
          R"({"format": "actcraft-layout/1", "menus": [], "palette": [], "pageMenu": ["page.insert-after", "-", "grid.hide"]})"));
      QVERIFY(layout.has_value());
      QCOMPARE(layout->pageMenu, (QStringList{QStringLiteral("page.insert-after"), QStringLiteral("-"), QStringLiteral("grid.hide")}));
  }
  void lot4EntriesDeclared()
  {
      const auto registry = loadBuiltinRegistry(); // aide du fichier (lot 1)
      for (const char* id : {"tool.shape", "shape.choose", "tool.text", "tool.text-vertical", "text.bold", "view.browsers",
                             "page.insert-after", "page.paste", "grid.designer", "grid.snap", "page.extend", "page.zones"})
          QVERIFY2(registry.contains(QString::fromLatin1(id)), id);
      const auto layout = loadBuiltinLayout();
      QVERIFY(layout.pageMenu.contains(QStringLiteral("grid.hide")));
      QVERIFY(layout.unknownCommands(registry).isEmpty());
  }
  void noShortcutTwice()
  {
      const auto registry = loadBuiltinRegistry();
      QHash<QString, QString> seen;
      for (const auto& c : registry.commands()) {
          if (c.shortcut.isEmpty())
              continue;
          QVERIFY2(!seen.contains(c.shortcut), qPrintable(c.shortcut + QStringLiteral(" : ") + seen.value(c.shortcut) + QStringLiteral(", ") + c.id));
          seen.insert(c.shortcut, c.id);
      }
  }
  ```
  (Noms des aides et de la fabrique : ceux du fichier ; si L0-S a déjà posé `noShortcutTwice`, ne pas le doubler :
  il couvre alors les commandes du lot 4.) Commande ciblée `-R tst_actions_layout` → rouge (`pageMenu` inconnu).
- [ ] **Étape 2 : `pageMenu`.** Dans `layout.h`, à la fin de `Layout` :
  `QStringList pageMenu; // menu de page (F-771), dans l'ordre ; « - » : séparateur ; « >id » : sous-menu`. Dans
  `layout.cpp` : clé facultative lue comme `objectBar` (tableau de chaînes ou objets de sous-menu, erreur
  « pageMenu : tableau attendu ») ; `unknownCommands` la parcourt. Dans `ui/include/actcraft/ui/commands.h` et
  `ui/src/commands.cpp` : propriété `Q_PROPERTY(QVariantList pageMenu READ pageMenu NOTIFY layoutChanged)` au même
  format que `menus()` (éléments `{id}`, `{separator: true}`, `{submenu: true, id, label, items}`). Vert.
- [ ] **Étape 3 : données.** En fin de `commands` de `actions/data/commands.json`, un bloc :
  ```json
      { "id": "tool.shape", "label": "Formes", "icon": "forme", "shortcut": "Ctrl+Shift+S", "checkable": true },
      { "id": "shape.choose", "label": "Choisir une forme", "argument": "text" },
      { "id": "shape.filled", "label": "Forme remplie", "checkable": true },
      { "id": "shape.fill-color", "label": "Couleur de remplissage", "argument": "text" },
      { "id": "shape.draw-from-center", "label": "Tracer depuis le centre", "checkable": true },
      { "id": "tool.text", "label": "Texte", "icon": "texte", "shortcut": "Ctrl+T", "checkable": true },
      { "id": "tool.text-vertical", "label": "Texte de haut en bas", "icon": "texte-vertical", "checkable": true },
      { "id": "text.font", "label": "Police", "argument": "text" },
      { "id": "text.size", "label": "Taille", "argument": "number" },
      { "id": "text.size-up", "label": "Agrandir le texte", "icon": "taille-plus" },
      { "id": "text.size-down", "label": "Réduire le texte", "icon": "taille-moins" },
      { "id": "text.bold", "label": "Gras", "icon": "gras", "checkable": true },
      { "id": "text.italic", "label": "Italique", "icon": "italique", "checkable": true },
      { "id": "text.underline", "label": "Souligné", "icon": "souligne", "checkable": true },
      { "id": "text.superscript", "label": "Exposant", "icon": "exposant", "checkable": true },
      { "id": "text.subscript", "label": "Indice", "icon": "indice", "checkable": true },
      { "id": "text.color", "label": "Couleur du texte", "argument": "text" },
      { "id": "text.background", "label": "Fond du texte", "argument": "text" },
      { "id": "text.align-left", "label": "À gauche", "icon": "aligner-gauche", "checkable": true },
      { "id": "text.align-center", "label": "Au centre", "icon": "centrer", "checkable": true },
      { "id": "text.align-right", "label": "À droite", "icon": "aligner-droite", "checkable": true },
      { "id": "text.spacing-up", "label": "Plus d'interligne", "icon": "interligne-plus" },
      { "id": "text.spacing-down", "label": "Moins d'interligne", "icon": "interligne-moins" },
      { "id": "text.indent", "label": "Plus de retrait", "icon": "retrait-plus" },
      { "id": "text.outdent", "label": "Moins de retrait", "icon": "retrait-moins" },
      { "id": "text.bullets", "label": "Puces", "icon": "puces", "checkable": true },
      { "id": "text.select-all", "label": "Tout le texte" },
      { "id": "text.symbol", "label": "Symboles", "icon": "symboles", "argument": "text" },
      { "id": "text.edit-at", "label": "Écrire ici", "argument": "text" },
      { "id": "view.browsers", "label": "Navigateurs", "icon": "navigateurs", "shortcut": "Ctrl+B", "checkable": true },
      { "id": "browsers.dock-left", "label": "À gauche", "checkable": true },
      { "id": "browsers.dock-right", "label": "À droite", "checkable": true },
      { "id": "page.goto", "label": "Aller à la page", "argument": "number" },
      { "id": "page.insert-before", "label": "Page vierge avant", "icon": "page-avant" },
      { "id": "page.insert-after", "label": "Page vierge après", "icon": "page-apres", "shortcut": "Ctrl+I" },
      { "id": "page.cut", "label": "Couper les pages", "icon": "page-couper" },
      { "id": "page.copy", "label": "Copier les pages", "icon": "page-copier" },
      { "id": "page.paste", "label": "Coller les pages", "icon": "page-coller" },
      { "id": "page.duplicate", "label": "Dupliquer les pages", "icon": "page-dupliquer" },
      { "id": "page.delete", "label": "Supprimer les pages", "icon": "page-supprimer" },
      { "id": "page.background", "label": "Fond de la page…", "icon": "fond" },
      { "id": "grid.designer", "label": "Grille…", "icon": "createur-grille", "shortcut": "Ctrl+Shift+G" },
      { "id": "grid.hide", "label": "Cacher la grille", "icon": "grille-masquer", "checkable": true },
      { "id": "grid.snap", "label": "Aimanter à la grille", "icon": "aimant", "checkable": true },
      { "id": "view.page-extension", "label": "Poignée d'allongement", "icon": "allonger", "checkable": true },
      { "id": "page.extend", "label": "Allonger la page", "icon": "allonger" },
      { "id": "page.zones", "label": "Zones de la page", "icon": "zones", "argument": "text" }
  ```
  `text.edit-at` n'apparaît dans aucun menu (outil Texte seulement). Dans `actions/data/layout.json` : menu `insert`
  → `[{"submenu": "insert-page", "label": "Page", "items": ["page.insert-before", "page.insert-after"]}]` ; menu
  `tools` → ajouter `"-", "tool.shape", "tool.text", "tool.text-vertical"` en fin ; menu `view` → ajouter `"-",
  "view.browsers", "view.page-extension"` en fin ; palette : groupe `{"group": "formes-texte", "items":
  ["tool.shape", "tool.text"]}` après le groupe `outils` ; clé
  `"pageMenu": [{"submenu": "page-insert", "label": "Insérer une page", "items": ["page.insert-before", "page.insert-after"]}, "-", "page.background", "grid.designer", "grid.hide", "grid.snap", "-", "page.cut", "page.copy", "page.paste", "page.duplicate", "page.delete"]`.
- [ ] **Étape 4 : icônes.** Dessiner les 34 fichiers de la table (24 × 24, mêmes attributs de trait que
  `ui/icons/stylo.svg`), formes simples à nous : triangle et cercle superposés (`forme`), « T » (`texte`), « T » sur
  une flèche descendante (`texte-vertical`), damier de lignes (`grille`), damier barré (`grille-masquer`), aimant en
  U (`aimant`), damier et crayon (`createur-grille`), panneau à rangée d'icônes (`navigateurs`), page et flèche à
  gauche / à droite (`page-avant`, `page-apres`), page et ciseaux / deux pages / presse-papiers / page doublée /
  page barrée (`page-couper` … `page-supprimer`), pot renversé (`fond`), page et flèche vers le bas (`allonger`),
  page coupée de traits (`zones`), « G », « I », « S » soulignés (`gras`, `italique`, `souligne`), « x² », « x₂ »
  (`exposant`, `indice`), lignes alignées (`aligner-gauche`, `centrer`, `aligner-droite`), lignes écartées ou
  serrées avec flèche (`interligne-plus`, `interligne-moins`), lignes décalées et flèche (`retrait-plus`,
  `retrait-moins`), points et lignes (`puces`), « Ω » (`symboles`), « A+ », « A− » (`taille-plus`,
  `taille-moins`). Les ajouter à la liste des ressources de `ui/CMakeLists.txt` (bloc de `icons/stylo.svg`).
- [ ] **Étape 5 : modèle (réel, pas un talon).** Test rouge dans `tests/tst_core_model.cpp` :
  ```cpp
  void lot4FieldsKeepOldFingerprints()
  {
      using namespace actcraft::core;
      Page page;
      PageObject shape{Shape{}};
      shape.common.id = 1;
      PageObject text{Text{}};
      text.common.id = 2;
      page.objects = {shape, text};
      const auto before = fingerprint(page);
      std::get<Shape>(body(page.objects[0])).preset = "arrow";
      QVERIFY(fingerprint(page) != before);
      std::get<Shape>(body(page.objects[0])).preset.clear();
      QCOMPARE(fingerprint(page), before);
      std::get<Shape>(body(page.objects[0])).parameters = {0.3};
      QVERIFY(fingerprint(page) != before);
      std::get<Shape>(body(page.objects[0])).parameters.clear();
      std::get<Text>(body(page.objects[1])).vertical = true;
      QVERIFY(fingerprint(page) != before);
      std::get<Text>(body(page.objects[1])).vertical = false;
      page.baseHeight = 768;
      QVERIFY(fingerprint(page) != before);
      page.baseHeight = 0;
      QCOMPARE(fingerprint(page), before);
  }
  ```
  Puis, dans `model.h`, à la fin de `Shape` : `std::string preset; // préréglage de la palette (« rectangle »…) ;
  vide : forme lue ou générique` et `std::vector<double> parameters; // points intelligents du préréglage (F-242)` ;
  à la fin de `Text` : `bool vertical = false; // texte de haut en bas (F-301) : colonne d'un caractère` ; dans
  `Page`, après `height` : `double baseHeight = 0; // hauteur avant la première extension (F-301) ; 0 : jamais
  allongée`. Dans `model.cpp`, `fingerprint(const Page&)` : après le flux actuel (clés inconnues comprises), une
  **queue** écrite seulement s'il y a une donnée du lot 4 : pour chaque objet, dans l'ordre, qui a un `preset` non
  vide, des `parameters` ou `vertical` vrai, l'octet `0xA4`, son rang (`std::uint64_t`), `preset` (longueur puis
  octets), le nombre de paramètres puis leurs motifs IEEE 754, `vertical` (un octet) ; puis, si
  `baseHeight != 0`, l'octet `0xA5` et son motif. Une page sans donnée du lot 4 garde son empreinte. Vert.
- [ ] **Étape 6 : en-têtes figés et talons.** Créer les en-têtes ci-dessous **tels quels** ; chaque `.cpp` talon
  inclut son en-tête et définit chaque fonction par le corps le plus simple qui compile, précédé de
  `// Talon L4-T1 : rempli par L4-T<n>.` ; une commande talon ne change rien (`redo`/`undo` vides).

  `core/include/actcraft/core/shapegeometry.h` (L4-T3) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>

  #include <cstddef>
  #include <string>
  #include <string_view>
  #include <utility>
  #include <vector>

  namespace actcraft::core {

  inline constexpr double kShapeMinDragScreen = 3.0; // en deçà, un tracé est un clic : aucune forme (F-241)

  enum class ShapeFamily : std::uint8_t
  {
      Line,   // objet core::Line (ligne, ligne brisée, zigzag, chevron)
      Closed, // objet core::Shape (polygone ou chemin rempli)
  };

  struct ShapePreset
  {
      std::string id;                  // « rectangle »… (contrat du plan)
      ShapeFamily family = ShapeFamily::Closed;
      bool square = false;             // carré, cercle : boîte carrée
      int axis = 0;                    // 1 : horizontale seulement ; 2 : verticale seulement ; 0 : libre
      std::vector<double> defaults;    // paramètres par défaut des points intelligents
      std::vector<double> minimum;     // bornes, même taille que defaults
      std::vector<double> maximum;
  };

  struct ShapeStyle
  {
      Argb outline = 0xFF000000u;
      double width = 4;      // F-241
      bool filled = true;    // E1, D2
      Argb fill = 0xFFFFFFFFu;
  };

  // Ordre de la palette (D1) ; le premier est le défaut.
  const std::vector<ShapePreset>& shapeCatalog();
  const ShapePreset* findShapePreset(std::string_view id) noexcept;

  // Boîte tracée de `press` à `current` (coordonnées de page) : depuis le coin, ou depuis le centre (`press` au
  // milieu) ; `square` : côté = le plus grand des deux écarts, dans le sens du geste. Toujours normalisée.
  RectF dragBox(PointF press, PointF current, bool fromCenter, bool square) noexcept;

  // Extrémités d'une ligne tracée : axe fixé par le préréglage, `constrain45` (Maj) arrondit l'angle à 45°.
  std::pair<PointF, PointF> dragLine(PointF press, PointF current, const ShapePreset& preset, bool fromCenter,
                                     bool constrain45) noexcept;

  // Objet neuf (sans identifiant, couche intermédiaire, nom vide) : Shape (preset, parameters = defaults, géométrie
  // construite) ou Line, selon la famille. Préréglage inconnu : rectangle.
  PageObject makeShapeObject(std::string_view preset, const RectF& box, const ShapeStyle& style);
  PageObject makeLineObject(std::string_view preset, PointF from, PointF to, const ShapeStyle& style);

  // Reconstruit `polygon` ou `paths` depuis preset, parameters et bounds. Faux (forme inchangée) si le préréglage est
  // inconnu ou vide (forme lue).
  bool rebuildShape(Shape& shape);

  // Points intelligents (F-242) en coordonnées de l'objet ; vide pour une forme sans préréglage ou sans paramètre.
  std::vector<PointF> smartPoints(const Shape& shape);

  // Paramètres quand le point `index` est amené en `objectPoint` (coordonnées de l'objet), bornés par le préréglage.
  std::vector<double> dragSmartPoint(const Shape& shape, std::size_t index, PointF objectPoint);

  } // namespace actcraft::core
  ```
  `core/include/actcraft/core/shapeedits.h` (L4-T3) :
  ```cpp
  #pragma once

  #include <actcraft/core/document.h>
  #include <actcraft/core/undo.h>

  #include <cstddef>
  #include <vector>

  namespace actcraft::core {

  // Point intelligent déplacé (F-242) : paramètres remplacés et géométrie reconstruite. Changes : un ObjectChanged,
  // zone = emprise avant ∪ emprise après. Objet absent ou sans préréglage : rien.
  class SetShapeParametersCommand final : public UndoCommand
  {
  public:
      SetShapeParametersCommand(std::size_t page, ObjectId id, std::vector<double> parameters);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_page;
      ObjectId m_id;
      std::vector<double> m_after;
      std::vector<double> m_before;
  };

  } // namespace actcraft::core
  ```
  `core/include/actcraft/core/pagesettings.h` (L4-T4) :
  ```cpp
  #pragma once

  #include <actcraft/core/document.h>
  #include <actcraft/core/undo.h>

  #include <cstddef>
  #include <cstdint>

  namespace actcraft::core {

  inline constexpr double kExtendRatio = 0.10; // F-301 : 10 % par clic (D10)
  inline constexpr int kMergeGrid = 4;          // famille de fusion de SetGridCommand (une séance du créateur)

  // Vrai si la grille dessine quelque chose sous les objets : visible, non dessinée au-dessus, un niveau visible.
  bool gridDrawsUnderObjects(const Grid& grid) noexcept;
  // Vrai si la grille dessine quelque chose au-dessus des objets (surcouche, L4-T7).
  bool gridDrawsAboveObjects(const Grid& grid) noexcept;

  // Grille de la page remplacée (F-620 à F-623). Changes : PageChanged (toute la page) si la grille du dessous
  // change (avant ou après) ; sinon un changement de page ignoré par le cache (contrat A13, L4-T4). `merge` vrai :
  // fusion avec la précédente SetGridCommand de la même page (kMergeGrid).
  class SetGridCommand final : public UndoCommand
  {
  public:
      SetGridCommand(std::size_t page, Grid grid, bool merge = false);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_page;
      Grid m_after;
      Grid m_before;
      bool m_merge;
  };

  // Fond de la page remplacé (F-610). Changes : PageChanged (toute la page).
  class SetBackgroundCommand final : public UndoCommand
  {
  public:
      SetBackgroundCommand(std::size_t page, Background background);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_page;
      Background m_after;
      Background m_before;
  };

  // Hauteur après un clic d'extension : height + kExtendRatio × base, base = baseHeight, ou height si 0 (D10).
  double nextExtendedHeight(const Page& page) noexcept;

  // Un clic d'extension (F-301 a) : baseHeight posé au premier clic, height = nextExtendedHeight. Une étape par
  // clic. Changes : PageChanged (toute la page).
  class ExtendPageCommand final : public UndoCommand
  {
  public:
      explicit ExtendPageCommand(std::size_t page);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_page;
      double m_beforeHeight = 0;
      double m_beforeBase = 0;
  };

  } // namespace actcraft::core
  ```
  `core/include/actcraft/core/gridsnap.h` (L4-T5) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>

  #include <optional>
  #include <vector>

  namespace actcraft::core {

  // Intersection la plus proche de `point` parmi les niveaux qui accrochent ; std::nullopt si la grille n'autorise
  // pas l'accrochage (snapEnabled faux) ou si aucun niveau n'accroche. Grille invisible : accroche quand même.
  std::optional<PointF> nearestGridPoint(const Grid& grid, PointF point, double pageWidth,
                                         double pageHeight) noexcept;

  // Point d'ancrage d'un objet pour l'accrochage (F-622), en coordonnées de page : ligne de base d'un texte
  // (core::textBaseline), sinon coin inférieur gauche de son emprise.
  PointF snapAnchor(const PageObject& object);

  // Déplacement corrigé pour que l'ancrage du premier objet de `ids` (ordre de la page) tombe, après `delta`, sur
  // l'intersection la plus proche ; `delta` inchangé sans accrochage ou sans objet.
  PointF snapDelta(const Page& page, const std::vector<ObjectId>& ids, PointF delta) noexcept;

  } // namespace actcraft::core
  ```
  `core/include/actcraft/core/pageedits.h` (L4-T15) :
  ```cpp
  #pragma once

  #include <actcraft/core/document.h>
  #include <actcraft/core/undo.h>

  #include <cstddef>
  #include <vector>

  namespace actcraft::core {

  // Copie de `page` dont chaque objet reçoit un identifiant neuf du document ; membres de groupes, extrémités de
  // connecteurs et cibles d'actions qui visent un objet de la page suivent leur nouvel identifiant.
  Page withFreshIds(Document& document, Page page);

  // Indices des pages déplacées après MovePagesCommand(pages, before) (F-802 : elles restent sélectionnées).
  std::vector<std::size_t> positionsAfterMove(std::size_t pageCount, const std::vector<std::size_t>& pages,
                                              std::size_t before);

  // Réordonner (F-802) : `pages` (triés, uniques) retirées puis insérées dans leur ordre devant la page qui était à
  // l'indice `before` (pageCount : à la fin). Changes : PageRemoved puis PageInserted par page. Sans effet si rien
  // ne bouge.
  class MovePagesCommand final : public UndoCommand
  {
  public:
      MovePagesCommand(std::vector<std::size_t> pages, std::size_t before);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::vector<std::size_t> m_pages;
      std::size_t m_before;
  };

  // Supprimer, couper (F-601) : refusée (aucun effet) si elle retirerait toutes les pages. Changes : PageRemoved.
  class RemovePagesCommand final : public UndoCommand
  {
  public:
      explicit RemovePagesCommand(std::vector<std::size_t> pages);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::vector<std::size_t> m_pages;
      std::vector<Page> m_removed;
  };

  // Coller, dupliquer, insérer (F-600, F-601) : copies à identifiants neufs (withFreshIds au premier redo, gardés
  // ensuite) insérées à `index`. Changes : PageInserted.
  class InsertPagesCommand final : public UndoCommand
  {
  public:
      InsertPagesCommand(std::size_t index, std::vector<Page> pages);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_index;
      std::vector<Page> m_pages;
      bool m_fresh = false;
  };

  } // namespace actcraft::core
  ```
  `core/include/actcraft/core/textedits.h` (L4-T16) :
  ```cpp
  #pragma once

  #include <actcraft/core/document.h>
  #include <actcraft/core/undo.h>

  #include <cstddef>
  #include <optional>

  namespace actcraft::core {

  // Texte remplacé (F-251) : HTML, bornes, lignes lues et sens. Changes : un ObjectChanged, zone = emprise avant ∪
  // emprise après. Objet absent ou qui n'est pas un texte : rien.
  class SetTextCommand final : public UndoCommand
  {
  public:
      SetTextCommand(std::size_t page, ObjectId id, Text text);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_page;
      ObjectId m_id;
      Text m_after;
      Text m_before;
  };

  // Ligne de base de la première ligne, coordonnées de l'objet : celle de la première ligne lue (runs) ; sinon
  // std::nullopt (l'appelant prend le coin inférieur gauche de la première ligne qu'il a mesurée, ou de l'emprise).
  std::optional<PointF> textBaseline(const Text& text) noexcept;

  } // namespace actcraft::core
  ```
  `tools/include/actcraft/tools/shapetool.h` (L4-T8) et `texttool.h` (L4-T22) :
  ```cpp
  #pragma once

  #include <actcraft/tools/tool.h>
  #include <actcraft/tools/toolcontext.h>

  #include <optional>

  namespace actcraft::tools {

  // Outil des formes (F-240, F-241) : aperçu par la couche soulevée, une AddObjectCommand au relâché.
  class ShapeTool final : public Tool
  {
  public:
      explicit ShapeTool(ToolContext& context);
      QString id() const override;
      void onSample(const input::Sample& sample) override;
      bool isBusy() const noexcept override;
      void cancel() override;
      bool onKey(int key, int modifiers) override; // Maj suivie pendant le tracé ; Échap annule

  private:
      ToolContext& m_context;
      std::optional<core::PointF> m_press;
      core::PointF m_current;
      bool m_shift = false;
  };

  } // namespace actcraft::tools
  ```
  ```cpp
  #pragma once

  #include <actcraft/tools/tool.h>
  #include <actcraft/tools/toolcontext.h>

  namespace actcraft::tools {

  // Outils Texte et Texte de haut en bas (F-250, F-301 b) : un appui demande l'édition au point touché par la
  // commande « text.edit-at » ({x, y, vertical}) ; l'édition elle-même est dans ui::TextEditing.
  class TextTool final : public Tool
  {
  public:
      TextTool(ToolContext& context, bool vertical);
      QString id() const override; // « text » ou « text-vertical »
      void onSample(const input::Sample& sample) override;
      bool isBusy() const noexcept override;
      void cancel() override;
      void deactivate() override; // valide l'édition en cours (F-252) : commande « text.edit-at » sans argument

  private:
      ToolContext& m_context;
      bool m_vertical;
  };

  } // namespace actcraft::tools
  ```
  Ajouts à `tools/include/actcraft/tools/userstate.h` (corps réels dans `userstate.cpp`, simples accesseurs) :
  ```cpp
  struct ShapeSettings
  {
      QString preset = QStringLiteral("line"); // D1 : défaut de la palette
      bool filled = true;                       // E1
      core::Argb fill = 0xFFFFFFFFu;
      bool fromCenter = false;                  // F-241 : depuis le coin par défaut
  };

  struct TextSettings
  {
      QString fontFamily;   // vide : police par défaut de l'application (N2-3, D7)
      double pointSize = 14;
      core::Argb color = 0xFF000000u;
  };
  // dans UserState, public, à la suite :
  ShapeSettings shapeSettings() const;
  void setShapeSettings(const ShapeSettings& settings);
  TextSettings textSettings() const;
  void setTextSettings(const TextSettings& settings);
  // privé : ShapeSettings m_shape; TextSettings m_text;
  ```
  Ajout à `tools/include/actcraft/tools/toolcontext.h`, à la fin de `SelectionView` :
  `std::vector<core::PointF> smartPoints; // F-242 : points intelligents de la forme sélectionnée seule (page)`.
  Dans `tools/src/toolregistry.cpp`, `registerBuiltinTools` ajoute, après les quatre du lot 3, « shape »
  (`ShapeTool`), « text » (`TextTool(context, false)`), « text-vertical » (`TextTool(context, true)`).

  `render/include/actcraft/render/gridoverlayitem.h` (L4-T7) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>

  #include <QQuickItem>
  #include <QtQml/qqmlregistration.h>

  namespace actcraft::render {

  // Grille dessinée au-dessus des objets (« toujours visible », F-620), hors de la scène que le cache photographie.
  // Coordonnées de page ; lignes nettes de 1 px d'écran (lot 10, P2). Ne dessine rien si la grille de la page n'est
  // pas au-dessus.
  class GridOverlayItem : public QQuickItem
  {
      Q_OBJECT
      QML_NAMED_ELEMENT(GridOverlay)
      Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
      Q_PROPERTY(int rebuildCount READ rebuildCount NOTIFY rebuilt) // tests

  public:
      explicit GridOverlayItem(QQuickItem* parent = nullptr);
      void setPage(const core::Page* page); // fil principal ; nullptr : rien
      void pageChanged();                    // la grille a pu changer : relue au prochain rendu
      qreal zoom() const noexcept;
      void setZoom(qreal zoom);
      int rebuildCount() const noexcept;

  signals:
      void zoomChanged();
      void rebuilt();

  protected:
      QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData* data) override;

  private:
      const core::Page* m_page = nullptr;
      core::Grid m_grid;
      double m_width = 0;
      double m_height = 0;
      qreal m_zoom = 1;
      bool m_dirty = true;
      int m_rebuilds = 0;
  };

  } // namespace actcraft::render
  ```
  `ui/include/actcraft/ui/pageoptions.h` (L4-T13, L4-T20, L4-T26, L4-T28) :
  ```cpp
  #pragma once

  #include <actcraft/ui/documentcontroller.h>

  #include <QObject>
  #include <QPointer>
  #include <QString>
  #include <QVariantMap>
  #include <QtQml/qqmlregistration.h>

  namespace actcraft::ui {

  // Réglages de la page affichée : grille (F-620 à F-623), fond (F-610), extension (F-301 a), zones (F-603).
  class PageOptions : public QObject
  {
      Q_OBJECT
      QML_NAMED_ELEMENT(PageSettings)
      Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                     setDocumentController NOTIFY wiringChanged)
      Q_PROPERTY(bool gridVisible READ gridVisible NOTIFY changed)
      Q_PROPERTY(bool gridSnap READ gridSnap NOTIFY changed)
      Q_PROPERTY(bool gridAbove READ gridAbove NOTIFY changed)
      // {visible, snap, above, previewZoom, baseZoom, zoomIncrement, levels: [{visible, snap, startX, startY,
      //  spacingX, spacingY, color, pattern, angle, line, alongX, alongY}]} ; noms de nos clés
      Q_PROPERTY(QVariantMap grid READ grid NOTIFY changed)
      Q_PROPERTY(QVariantMap background READ background NOTIFY changed) // {kind, color, color2, gradient, image}
      Q_PROPERTY(qreal pageHeight READ pageHeight NOTIFY changed)
      Q_PROPERTY(QString zones READ zones NOTIFY changed) // « <horizontales>x<verticales> », « 0x0 » par défaut

  public:
      explicit PageOptions(QObject* parent = nullptr);
      DocumentController* documentController() const noexcept;
      void setDocumentController(DocumentController* controller);
      bool gridVisible() const;
      bool gridSnap() const;
      bool gridAbove() const;
      QVariantMap grid() const;
      QVariantMap background() const;
      qreal pageHeight() const;
      QString zones() const;

      Q_INVOKABLE void toggleGridHidden();                 // F-623
      Q_INVOKABLE void toggleSnap();                       // F-622
      Q_INVOKABLE void beginDesigner();                    // ouvre une séance : les réglages suivants fusionnent
      Q_INVOKABLE void setGrid(const QVariantMap& grid);   // aperçu immédiat (F-620)
      Q_INVOKABLE void addLevel();
      Q_INVOKABLE void removeLevel(int level);
      Q_INVOKABLE void resetGrid();                        // un niveau sans accrochage
      Q_INVOKABLE void stepSpacing(int sign);              // « + » et « - » : × (1 ± zoomIncrement)
      Q_INVOKABLE void endDesigner();
      Q_INVOKABLE void setBackground(const QVariantMap& background); // F-610 (L4-T20)
      Q_INVOKABLE void extendPage();                                  // F-301 a (L4-T26)
      Q_INVOKABLE void setZones(const QString& zones);                // F-603 (L4-T28)

  signals:
      void wiringChanged();
      void changed();

  private:
      QPointer<DocumentController> m_controller;
      bool m_designer = false;
  };

  } // namespace actcraft::ui
  ```
  `ui/include/actcraft/ui/pagethumbnails.h` (L4-T17) :
  ```cpp
  #pragma once

  #include <actcraft/ui/documentcontroller.h>
  #include <actcraft/ui/pageview.h>

  #include <QAbstractListModel>
  #include <QImage>
  #include <QPointer>
  #include <QQuickPaintedItem>
  #include <QtQml/qqmlregistration.h>

  #include <vector>

  namespace actcraft::ui {

  // Miniatures des pages (F-800), à jour après chaque modification sans figer l'écriture : copie de la page sur le
  // fil principal, rendu par le peintre partagé (DA5) sur un fil secondaire, kSettleMs après la dernière
  // modification de la page, jamais pendant un contact sur la vue.
  class PageThumbnails : public QAbstractListModel
  {
      Q_OBJECT
      QML_NAMED_ELEMENT(PageThumbnails)
      Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                     setDocumentController NOTIFY wiringChanged)
      Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY wiringChanged)
      Q_PROPERTY(int thumbnailWidth READ thumbnailWidth WRITE setThumbnailWidth NOTIFY thumbnailWidthChanged)
      Q_PROPERTY(int pendingCount READ pendingCount NOTIFY renderingChanged)
      Q_PROPERTY(int renderCount READ renderCount NOTIFY renderingChanged)
      Q_PROPERTY(double lastCopyMs READ lastCopyMs NOTIFY renderingChanged) // fil principal, dernier rendu

  public:
      static constexpr int kSettleMs = 400;
      static constexpr int kDefaultWidth = 160;
      enum Role
      {
          PageNumberRole = Qt::UserRole + 1, // 1 à N
          RevisionRole,                      // change à chaque nouvelle image
          AspectRole,                        // hauteur / largeur de la page
          CurrentRole,                       // page affichée
      };

      explicit PageThumbnails(QObject* parent = nullptr);
      ~PageThumbnails() override;
      int rowCount(const QModelIndex& parent = QModelIndex()) const override;
      QVariant data(const QModelIndex& index, int role) const override;
      QHash<int, QByteArray> roleNames() const override;

      DocumentController* documentController() const noexcept;
      void setDocumentController(DocumentController* controller);
      PageView* view() const noexcept;
      void setView(PageView* view);
      int thumbnailWidth() const noexcept;
      void setThumbnailWidth(int width); // 80 à 320
      int pendingCount() const noexcept;
      int renderCount() const noexcept;
      double lastCopyMs() const noexcept;
      QImage image(int page) const; // dernière image rendue ; nulle avant le premier rendu
      Q_INVOKABLE void refreshAll();

  signals:
      void wiringChanged();
      void thumbnailWidthChanged();
      void renderingChanged();

  private:
      QPointer<DocumentController> m_controller;
      QPointer<PageView> m_view;
      std::vector<QImage> m_images;
      std::vector<int> m_revisions;
      int m_width = kDefaultWidth;
      int m_pending = 0;
      int m_renders = 0;
      double m_lastCopyMs = 0;
  };

  // Image d'une miniature dans le QML (type ThumbnailImage) : peint PageThumbnails::image(page).
  class ThumbnailItem : public QQuickPaintedItem
  {
      Q_OBJECT
      QML_NAMED_ELEMENT(ThumbnailImage)
      Q_PROPERTY(actcraft::ui::PageThumbnails* source READ source WRITE setSource NOTIFY sourceChanged)
      Q_PROPERTY(int page READ page WRITE setPage NOTIFY pageChanged)          // indice 0 à N − 1
      Q_PROPERTY(int revision READ revision WRITE setRevision NOTIFY revisionChanged)

  public:
      explicit ThumbnailItem(QQuickItem* parent = nullptr);
      PageThumbnails* source() const noexcept;
      void setSource(PageThumbnails* source);
      int page() const noexcept;
      void setPage(int page);
      int revision() const noexcept;
      void setRevision(int revision);
      void paint(QPainter* painter) override;

  signals:
      void sourceChanged();
      void pageChanged();
      void revisionChanged();

  private:
      QPointer<PageThumbnails> m_source;
      int m_page = -1;
      int m_revision = 0;
  };

  } // namespace actcraft::ui
  ```
  `ui/include/actcraft/ui/pagescontroller.h` (L4-T18) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>
  #include <actcraft/ui/documentcontroller.h>

  #include <QList>
  #include <QObject>
  #include <QPointer>
  #include <QtQml/qqmlregistration.h>

  #include <vector>

  namespace actcraft::ui {

  // Pages du document vues par le navigateur de pages et le menu de page (F-600, F-601, F-771, F-801 à F-803).
  class PagesController : public QObject
  {
      Q_OBJECT
      QML_NAMED_ELEMENT(Pages)
      Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                     setDocumentController NOTIFY wiringChanged)
      Q_PROPERTY(QList<int> selection READ selection NOTIFY selectionChanged) // indices triés, jamais vide
      Q_PROPERTY(bool canPaste READ canPaste NOTIFY clipboardChanged)

  public:
      explicit PagesController(QObject* parent = nullptr);
      DocumentController* documentController() const noexcept;
      void setDocumentController(DocumentController* controller);
      QList<int> selection() const;
      bool canPaste() const noexcept;

      // F-801 : sans modificateur, sélection = {index} et la page s'affiche ; Maj : plage depuis l'ancre ; Ctrl :
      // bascule (la dernière page sélectionnée ne se retire pas). `modifiers` : Qt::KeyboardModifiers.
      Q_INVOKABLE void click(int index, int modifiers);
      Q_INVOKABLE void goTo(int index);                 // F-800 : sans effet de changement de page
      Q_INVOKABLE void moveSelection(int before);       // F-802 : pages gardées sélectionnées
      Q_INVOKABLE void insertBlank(bool after);         // F-600, par rapport à la page affichée
      Q_INVOKABLE void copySelection();                 // F-601 : presse-papiers interne
      Q_INVOKABLE void cutSelection();
      Q_INVOKABLE void paste();                         // F-601, F-803 : avant la page active
      Q_INVOKABLE void duplicateSelection();            // F-600 : après la dernière page sélectionnée
      Q_INVOKABLE void deleteSelection();

  signals:
      void wiringChanged();
      void selectionChanged();
      void clipboardChanged();

  private:
      QPointer<DocumentController> m_controller;
      QList<int> m_selection;
      int m_anchor = 0;
      std::vector<core::Page> m_clipboard;
  };

  } // namespace actcraft::ui
  ```
  `ui/include/actcraft/ui/textediting.h` (L4-T21, L4-T25) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>
  #include <actcraft/ui/documentcontroller.h>
  #include <actcraft/ui/pageview.h>

  #include <QObject>
  #include <QPointer>
  #include <QQuickTextDocument>
  #include <QRectF>
  #include <QString>
  #include <QVariant>
  #include <QtQml/qqmlregistration.h>

  namespace actcraft::ui {

  // Édition d'un texte (F-250 à F-253, F-100, F-301 b) dans le TextEdit de TextEditor.qml : un texte existant est
  // soulevé pendant l'édition (DA11) ; validation = une étape (AddObjectCommand ou SetTextCommand) ; texte vide :
  // rien n'est gardé.
  class TextEditing : public QObject
  {
      Q_OBJECT
      QML_NAMED_ELEMENT(TextEditing)
      Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                     setDocumentController NOTIFY wiringChanged)
      Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY wiringChanged)
      Q_PROPERTY(QQuickTextDocument* textDocument READ textDocument WRITE setTextDocument NOTIFY wiringChanged)
      Q_PROPERTY(int selectionStart READ selectionStart WRITE setSelectionStart NOTIFY formatChanged)
      Q_PROPERTY(int selectionEnd READ selectionEnd WRITE setSelectionEnd NOTIFY formatChanged)
      Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
      Q_PROPERTY(bool vertical READ isVertical NOTIFY activeChanged)
      Q_PROPERTY(QRectF frame READ frame NOTIFY frameChanged) // coordonnées de page
      Q_PROPERTY(QString plainText READ plainText NOTIFY formatChanged)
      Q_PROPERTY(QString fontFamily READ fontFamily NOTIFY formatChanged)
      Q_PROPERTY(qreal fontSize READ fontSize NOTIFY formatChanged)
      Q_PROPERTY(bool bold READ bold NOTIFY formatChanged)
      Q_PROPERTY(bool italic READ italic NOTIFY formatChanged)
      Q_PROPERTY(bool underline READ underline NOTIFY formatChanged)

  public:
      explicit TextEditing(QObject* parent = nullptr);
      DocumentController* documentController() const noexcept;
      void setDocumentController(DocumentController* controller);
      PageView* view() const noexcept;
      void setView(PageView* view);
      QQuickTextDocument* textDocument() const noexcept;
      void setTextDocument(QQuickTextDocument* document);
      int selectionStart() const noexcept;
      void setSelectionStart(int position);
      int selectionEnd() const noexcept;
      void setSelectionEnd(int position);
      bool isActive() const noexcept;
      bool isVertical() const noexcept;
      QRectF frame() const noexcept;
      QString plainText() const;
      QString fontFamily() const;
      qreal fontSize() const;
      bool bold() const;
      bool italic() const;
      bool underline() const;

      // Texte touché au point (page) : édition ; sinon texte neuf « Votre texte » tout sélectionné, coin du cadre au
      // point. Une édition en cours est d'abord validée. Vrai si une édition commence.
      Q_INVOKABLE bool beginAt(QPointF pagePoint, bool vertical);
      Q_INVOKABLE void commit(); // F-252
      Q_INVOKABLE void cancel();
      // Commandes « text.* » de la barre (F-100) sur la sélection ; argument selon la commande.
      Q_INVOKABLE void apply(const QString& command, const QVariant& argument);
      Q_INVOKABLE void setFrameWidth(qreal width); // poignée d'extension (page)
      Q_INVOKABLE void moveFrame(QPointF delta);   // poignée de déplacement (page)

  signals:
      void wiringChanged();
      void activeChanged();
      void frameChanged();
      void formatChanged();
      void focusRequested();

  private:
      QPointer<DocumentController> m_controller;
      QPointer<PageView> m_view;
      QPointer<QQuickTextDocument> m_document;
      core::ObjectId m_editing = 0; // 0 : texte neuf
      bool m_active = false;
      bool m_vertical = false;
      QRectF m_frame;
      int m_selectionStart = 0;
      int m_selectionEnd = 0;
  };

  } // namespace actcraft::ui
  ```
  Ajouts à `ui/include/actcraft/ui/toolhost.h` (corps talons dans `toolhost.cpp`, `// Talon L4-T1 : rempli par
  L4-T11.`) :
  ```cpp
  Q_PROPERTY(QString shapePreset READ shapePreset NOTIFY settingsChanged)
  Q_PROPERTY(bool shapeFilled READ shapeFilled NOTIFY settingsChanged)
  Q_PROPERTY(bool shapeFromCenter READ shapeFromCenter NOTIFY settingsChanged)
  Q_PROPERTY(QStringList shapePresets READ shapePresets CONSTANT) // ordre de la palette
  // public
  QString shapePreset() const;
  bool shapeFilled() const;
  bool shapeFromCenter() const;
  QStringList shapePresets() const;
  Q_INVOKABLE void chooseShape(const QString& preset); // active aussi l'outil « shape »
  Q_INVOKABLE void setShapeFilled(bool filled);
  Q_INVOKABLE void setShapeFillColor(const QColor& color);
  Q_INVOKABLE void setShapeFromCenter(bool fromCenter);
  // Contour normalisé du préréglage dans un carré 1 × 1, pour la palette et le curseur : [{x, y}, …] fermé.
  Q_INVOKABLE QVariantList shapeOutline(const QString& preset) const;
  ```
  Ajouts réels à `ui/include/actcraft/ui/documentcontroller.h` et `.cpp` (pas des talons) :
  ```cpp
  Q_INVOKABLE void showPage(int index);          // borne, puis show(index) : comme nextPage / pageBack
  // signaux
  void pageContentChanged(int page);             // une fois par page touchée par une commande, après settle()
  ```
  `showPage` termine d'abord le trait en cours et les gestes comme `nextPage` (même chemin). `pageContentChanged`
  est émis dans `settle` pour chaque indice de page distinct des `Change` (objet ou page ; page retirée : rien).
  Test rouge puis vert dans `tests/tst_ui_document.cpp` :
  ```cpp
  void pageContentChangedNamesThePage()
  {
      actcraft::ui::DocumentController controller;
      controller.newDocument(1024, 768);
      controller.nextPage(); // deux pages, la 2e affichée
      QSignalSpy spy(&controller, &actcraft::ui::DocumentController::pageContentChanged);
      actcraft::core::Stroke stroke;
      stroke.points = {{10, 10, 0.5f}, {50, 50, 0.5f}};
      controller.commitStroke(stroke);
      QCOMPARE(spy.count(), 1);
      QCOMPARE(spy.at(0).at(0).toInt(), 1);
  }
  void showPageShowsIt()
  {
      actcraft::ui::DocumentController controller;
      controller.newDocument(1024, 768);
      controller.nextPage();
      controller.nextPage();
      controller.showPage(0);
      QCOMPARE(controller.currentIndex(), 0);
      controller.showPage(9);
      QCOMPARE(controller.currentIndex(), 2);
  }
  ```
  Talons QML (créés par L4-T2) : rien ici.
- [ ] **Étape 7 : CMake.** Ajouter les sources : `core/CMakeLists.txt` (`src/shapegeometry.cpp`,
  `src/shapeedits.cpp`, `src/gridsnap.cpp`, `src/pagesettings.cpp`, `src/pageedits.cpp`, `src/textedits.cpp`),
  `tools/CMakeLists.txt` (`src/shapetool.cpp`, `src/texttool.cpp`), `render/CMakeLists.txt`
  (`src/gridoverlayitem.cpp`, en-tête dans la liste QML du module comme `pendinglayeritem.h`), `ui/CMakeLists.txt`
  (`src/pageoptions.cpp`, `src/pagethumbnails.cpp`, `src/pagescontroller.cpp`, `src/textediting.cpp` et
  leurs en-têtes dans la liste QML). Enregistrer par `actcraft_add_test` : `tests/core.cmake` (`tst_core_shapes`,
  `tst_core_gridsnap`, `tst_core_pagesettings`, `tst_core_pageedits`, `tst_core_textedits`, `tst_core_zones`),
  `tests/formats.cmake` (`tst_formats_lot4`), `tests/render.cmake` (`tst_render_gridoverlay`,
  `tst_render_verticaltext`), `tests/tools.cmake` (`tst_tools_shape`, `tst_tools_text`), `tests/ui.cmake`
  (`tst_ui_shapes`, `tst_ui_pagesettings`, `tst_ui_pagethumbnails`, `tst_ui_pages`, `tst_ui_text`),
  `tests/bench.cmake` (`tst_captures_formes`, `tst_captures_grille`, `tst_captures_texte`,
  `tst_captures_navigateur_pages`, `tst_captures_page_allongee_zones`, `tst_captures_texte_vertical`,
  `tst_captures_formes_texte_pages_grille`, sur le modèle de `tst_captures_cache_differe`).
- [ ] **Étape 8 : tests et scénarios talons.** Chacun des 16 fichiers de test :
  ```cpp
  #include <QtTest>

  class TstCoreShapes : public QObject
  {
      Q_OBJECT

  private slots:
      void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T3."); }
  };

  QTEST_GUILESS_MAIN(TstCoreShapes)
  #include "tst_core_shapes.moc"
  ```
  (`QTEST_MAIN` pour les tests de `render/` et `ui/` ; nom de classe, de `.moc` et tâche adaptés.) Chacun des 7
  scénarios : `{"format": "actcraft-scenario/1", "name": "<nom>", "description": "Talon L4-T1 : rempli par
  L4-T<n>.", "steps": [{"waitReady": true}]}`.
- [ ] **Étape 9 : preuves.** `format.ps1`, commande ciblée, preuve de tâche (N ≥ N3 + 23), fonctions des trois tests
  nommés en `PASS`, `premiers-pas` hors écran `CAPTURES OK`, `git add` des fichiers, `NOMS OK`.
- [ ] **Étape 10 : commit** (chemins explicites, la liste de la table), puis `NOMS OK` relancé.
  ```powershell
  git -C <wt> commit -m "Lot 4 : réservation des commandes, de la disposition, des icônes, du modèle, des tests et des API (L4-T1)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t2"></a>
### Tâche L4-T2 — Réservation II : coquille QML

**Objectif.** Poser les éléments QML du lot à leur place, en talons nommés, et un gestionnaire pour chaque commande
du lot, pour que les tâches suivantes ne touchent plus `Main.qml`, `PageArea.qml`, `ToolPalette.qml` ni
`PageContextMenu.qml`.

**Fichiers.** Ceux de la ligne L4-T2.

**Test et commande bornée.** Commande ciblée `-R tst_ui_shell` ; preuve de tâche ; `premiers-pas` hors écran ;
banc sans écran.

**Interdits.** Aucun comportement dans les talons. Ne pas réordonner les couches du lot 3 et de L0-S (A12).

**Terminé quand.** `tst_ui_shell` passe avec `lot4ShellPlaceholders`, `everyLot4CommandHasAHandler`,
`pageLayersInOrder` ; `premiers-pas` `CAPTURES OK` ; banc `content_check.ok = True`.

**Étapes.**
- [ ] **Étape 1 : test rouge.** Dans `tests/tst_ui_shell.cpp` (fabrique du fichier : moteur QML chargé sur
  `Main.qml`) :
  ```cpp
  void lot4ShellPlaceholders()
  {
      auto window = loadMainWindow(); // aide du fichier
      for (const char* name : {"browserPanel", "pageNavigator", "shapePalette", "textFormatBar", "textEditor",
                               "verticalTextBar", "gridDesigner", "backgroundDialog", "pageExtensionHandle",
                               "pageZones", "gridOverlay", "appPageSettings", "appThumbnails", "appPages", "appText"})
          QVERIFY2(findObject(window.get(), QString::fromLatin1(name)) != nullptr, name);
  }
  void everyLot4CommandHasAHandler()
  {
      auto window = loadMainWindow();
      const QStringList lot4 = lot4CommandIds(); // lit le bloc du lot 4 de commands.json (de tool.shape à page.zones)
      QCOMPARE(lot4.size(), 47);
      for (const auto& id : lot4)
          QVERIFY2(hasHandler(window.get(), id), qPrintable(id)); // gestionnaire déclaré dans Main.qml (aide du lot 3)
  }
  void pageLayersInOrder()
  {
      auto window = loadMainWindow();
      QQuickItem* content = findObject<QQuickItem*>(window.get(), QStringLiteral("page"));
      QVERIFY(content);
      QStringList names;
      for (QQuickItem* child : content->childItems())
          names << child->objectName();
      const auto lift = names.indexOf(QStringLiteral("liftLayer"));
      const auto grid = names.indexOf(QStringLiteral("gridOverlay"));
      const auto ink = names.indexOf(QStringLiteral("ink"));
      QVERIFY(lift >= 0 && grid == lift + 1 && ink > grid);
  }
  ```
  (Aides `loadMainWindow`, `findObject`, `hasHandler` : celles du fichier ; `lot4CommandIds` s'ajoute à côté.)
- [ ] **Étape 2 : talons QML.** Créer `BrowserPanel.qml`, `PageNavigator.qml`, `ShapePalette.qml`,
  `TextFormatBar.qml`, `TextEditor.qml`, `VerticalTextBar.qml`, `GridDesigner.qml`, `BackgroundDialog.qml`,
  `PageExtensionHandle.qml`, `PageZonesOverlay.qml` : `import QtQuick` puis `Item { visible: false }` ; ajouter les
  fichiers à la liste QML de `ui/CMakeLists.txt`.
- [ ] **Étape 3 : `Main.qml`.** Instancier `PageSettings { id: appPageSettings; objectName: "appPageSettings";
  documentController: appDocument }`, `PageThumbnails { id: appThumbnails; objectName: "appThumbnails";
  documentController: appDocument; view: pageArea.view }`, `Pages { id: appPages; objectName: "appPages";
  documentController: appDocument }`, `TextEditing { id: appText; objectName: "appText"; documentController:
  appDocument; view: pageArea.view }` ; `BrowserPanel { objectName: "browserPanel" }` à gauche de la zone de page
  (ancre à régler par L4-T19) ; `ShapePalette { objectName: "shapePalette" }` à droite, avant la palette d'outils ;
  `TextFormatBar { objectName: "textFormatBar" }` et `VerticalTextBar { objectName: "verticalTextBar" }` sous la
  ligne supérieure ; `GridDesigner { objectName: "gridDesigner" }` et `BackgroundDialog { objectName:
  "backgroundDialog" }` en fin de fenêtre. Gestionnaires des 47 commandes, sur le modèle du lot 3, chacun d'une
  ligne vers sa cible : `tool.shape` → `appTools.activate("shape")` ; `shape.choose` → `appTools.chooseShape(arg)` ;
  `shape.filled` → `appTools.setShapeFilled(!appTools.shapeFilled)` ; `shape.fill-color` →
  `appTools.setShapeFillColor(arg)` ; `shape.draw-from-center` → `appTools.setShapeFromCenter(!appTools.shapeFromCenter)` ;
  `tool.text` → `appTools.activate("text")` ; `tool.text-vertical` → `appTools.activate("text-vertical")` ;
  `text.edit-at` → `arg ? appText.beginAt(Qt.point(arg.x, arg.y), arg.vertical === true) : appText.commit()` ;
  chaque autre `text.*` → `appText.apply(id, arg)` ; `view.browsers` → bascule de `browserPanel.visible` ;
  `browsers.dock-left` / `-right` → `browserPanel.side = "left"` / `"right"` ; `page.goto` → `appPages.goTo(arg - 1)` ;
  `page.insert-before` / `-after` → `appPages.insertBlank(false / true)` ; `page.cut`, `page.copy`, `page.paste`,
  `page.duplicate`, `page.delete` → `appPages.cutSelection()`, `copySelection()`, `paste()`,
  `duplicateSelection()`, `deleteSelection()` ; `page.background` → `backgroundDialog.open()` ; `grid.designer` →
  `gridDesigner.open()` ; `grid.hide` → `appPageSettings.toggleGridHidden()` ; `grid.snap` →
  `appPageSettings.toggleSnap()` ; `view.page-extension` → bascule de `pageExtensionHandle.visible` ; `page.extend`
  → `appPageSettings.extendPage()` ; `page.zones` → `appPageSettings.setZones(arg)`. Les talons reçoivent une
  fonction vide `open()` (`GridDesigner`, `BackgroundDialog`) et une propriété `property string side: "left"`
  (`BrowserPanel`).
- [ ] **Étape 4 : `PageArea.qml`.** Dans `page`, juste après `liftLayer` : `GridOverlay { id: gridOverlay;
  objectName: "gridOverlay"; anchors.fill: parent; zoom: pageView.zoom }` ; dans `pageOverlays` :
  `PageZonesOverlay { objectName: "pageZones"; anchors.fill: parent }` et `PageExtensionHandle { objectName:
  "pageExtensionHandle" }` ; à côté de `selectionOverlay` : `TextEditor { objectName: "textEditor" }`. Le
  branchement de `gridOverlay` à la page (C++ `setPage`) se fait par `DocumentController` dans L4-T7 : ici,
  l'élément seul.
- [ ] **Étape 5 : `ToolPalette.qml`, `PageContextMenu.qml`.** Palette : le groupe `formes-texte` s'affiche comme les
  autres groupes (même délégué). Menu de page : en plus des entrées du lot 3 (A14), un `Instantiator` sur
  `appCommands.pageMenu` (même délégué que `CommandMenu.qml`, sous-menus compris).
- [ ] **Étape 6 : preuves.** `format.ps1`, commande ciblée verte, preuve de tâche, `premiers-pas` hors écran
  `CAPTURES OK`, banc sans écran, `git add`, `NOMS OK`.
- [ ] **Étape 7 : commit.**
  ```powershell
  git -C <wt> add ui/qml/Main.qml ui/qml/PageArea.qml ui/qml/ToolPalette.qml ui/qml/PageContextMenu.qml ui/qml/BrowserPanel.qml ui/qml/PageNavigator.qml ui/qml/ShapePalette.qml ui/qml/TextFormatBar.qml ui/qml/TextEditor.qml ui/qml/VerticalTextBar.qml ui/qml/GridDesigner.qml ui/qml/BackgroundDialog.qml ui/qml/PageExtensionHandle.qml ui/qml/PageZonesOverlay.qml ui/CMakeLists.txt tests/tst_ui_shell.cpp
  git -C <wt> commit -m "Lot 4 : coquille QML des formes, du texte, des navigateurs et de la grille (L4-T2)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t3"></a>
### Tâche L4-T3 — `core` : catalogue des formes, boîte tracée, lignes, points intelligents

**Objectif.** Remplir `shapegeometry` et `shapeedits` (en-têtes de L4-T1) : catalogue de 22 préréglages à nos
identifiants dans l'ordre D1, boîte depuis le coin ou le centre, carré, lignes contraintes, géométrie de chaque
préréglage (polygone de `Shape` en coordonnées de l'objet, origine au coin haut gauche de la boîte, transformation
= translation de la boîte), points intelligents de `arrow` (longueur et largeur de la tête), `rounded-rectangle`
(rayon), `star`, `star-four`, `burst`, `burst-twelve` (rayon intérieur), `callout` (pointe), commande
`SetShapeParametersCommand`. Paramètres et bornes : ceux de D4 ; à défaut, ceux du test ci-dessous.

**Fichiers.** `core/include/actcraft/core/shapegeometry.h`, `core/src/shapegeometry.cpp`,
`core/include/actcraft/core/shapeedits.h`, `core/src/shapeedits.cpp`, `tests/tst_core_shapes.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_core_shapes` ; fonctions du test : `catalogOrderAndDefault`,
`dragFromCornerAndCenter`, `squarePresetsStaySquare`, `linePresetsConstrained`, `arrowHeadSmartPointBounded`,
`setParametersIsOneStepAndRebuilds` en `PASS` ; preuve de tâche.

**Interdits.** Pas de QtGui dans `core/`. Pas de géométrie dupliquée dans `tools/` ou `ui/` (ils appellent ce
fichier). Ne pas changer l'en-tête public.

**Terminé quand.** Les six fonctions passent ; `TESTS OK` ; chaque préréglage du catalogue rend une géométrie non
vide dans une boîte de 100 × 60 (boucle du test `catalogOrderAndDefault`).

**Étapes.**
- [ ] **Étape 1 : test rouge.** Remplacer le talon de `tests/tst_core_shapes.cpp` :
  ```cpp
  #include <actcraft/core/document.h>
  #include <actcraft/core/edits.h>
  #include <actcraft/core/shapeedits.h>
  #include <actcraft/core/shapegeometry.h>
  #include <actcraft/core/undo.h>

  #include <QtTest>

  using namespace actcraft::core;

  class TstCoreShapes : public QObject
  {
      Q_OBJECT

  private slots:
      void catalogOrderAndDefault()
      {
          const auto& catalog = shapeCatalog();
          QCOMPARE(catalog.size(), std::size_t{22});
          QCOMPARE(QString::fromStdString(catalog.front().id), QStringLiteral("line"));
          QCOMPARE(QString::fromStdString(catalog[6].id), QStringLiteral("rectangle"));
          QVERIFY(findShapePreset("circle")->square);
          QVERIFY(findShapePreset("nothing") == nullptr);
          for (const auto& preset : catalog) {
              const PageObject o = preset.family == ShapeFamily::Line
                                       ? makeLineObject(preset.id, {0, 0}, {100, 60}, {})
                                       : makeShapeObject(preset.id, {0, 0, 100, 60}, {});
              QCOMPARE(o.common.layer, Layer::Middle);
              if (preset.family == ShapeFamily::Line)
                  QVERIFY2(std::get<Line>(body(o)).points.size() >= 2, preset.id.c_str());
              else
                  QVERIFY2(!std::get<Shape>(body(o)).polygon.empty() || !std::get<Shape>(body(o)).paths.empty(), preset.id.c_str());
          }
      }
      void dragFromCornerAndCenter()
      {
          const RectF corner = dragBox({10, 20}, {110, 80}, false, false);
          QCOMPARE(corner.x, 10.0);
          QCOMPARE(corner.width, 100.0);
          const RectF backwards = dragBox({110, 80}, {10, 20}, false, false);
          QCOMPARE(backwards.x, 10.0);
          QCOMPARE(backwards.height, 60.0);
          const RectF center = dragBox({100, 100}, {150, 130}, true, false);
          QCOMPARE(center.x, 50.0);
          QCOMPARE(center.y, 70.0);
          QCOMPARE(center.width, 100.0);
          QCOMPARE(center.height, 60.0);
      }
      void squarePresetsStaySquare()
      {
          const RectF box = dragBox({0, 0}, {100, -30}, false, true);
          QCOMPARE(box.width, 100.0);
          QCOMPARE(box.height, 100.0);
          QCOMPARE(box.y, -100.0);
      }
      void linePresetsConstrained()
      {
          const auto h = dragLine({0, 0}, {100, 40}, *findShapePreset("line-horizontal"), false, false);
          QCOMPARE(h.second.y, 0.0);
          const auto v = dragLine({0, 0}, {100, 40}, *findShapePreset("line-vertical"), false, false);
          QCOMPARE(v.second.x, 0.0);
          const auto d = dragLine({0, 0}, {100, 90}, *findShapePreset("line"), false, true);
          QVERIFY(std::abs(d.second.x - d.second.y) < 1e-9); // 45°
      }
      void arrowHeadSmartPointBounded()
      {
          PageObject o = makeShapeObject("arrow", {0, 0, 200, 100}, {});
          auto& shape = std::get<Shape>(body(o));
          const auto points = smartPoints(shape);
          QVERIFY(!points.empty());
          const auto preset = findShapePreset("arrow");
          const auto far = dragSmartPoint(shape, 0, {-1000, -1000});
          for (std::size_t i = 0; i < far.size(); ++i) {
              QVERIFY(far[i] >= preset->minimum[i]);
              QVERIFY(far[i] <= preset->maximum[i]);
          }
          PageObject read{Shape{}};
          QVERIFY(smartPoints(std::get<Shape>(body(read))).empty()); // forme lue : aucun
      }
      void setParametersIsOneStepAndRebuilds()
      {
          Document document;
          UndoStack stack;
          stack.push(document, std::make_unique<AddObjectCommand>(0, makeShapeObject("arrow", {0, 0, 200, 100}, {})));
          const ObjectId id = document.page(0).objects.back().common.id;
          const auto before = std::get<Shape>(body(document.page(0).objects.back())).polygon;
          auto params = findShapePreset("arrow")->defaults;
          params[0] = findShapePreset("arrow")->maximum[0];
          const Changes changes = stack.push(document, std::make_unique<SetShapeParametersCommand>(0, id, params));
          QCOMPARE(changes.size(), std::size_t{1});
          QCOMPARE(changes[0].kind, Change::Kind::ObjectChanged);
          QVERIFY(std::get<Shape>(body(document.page(0).objects.back())).polygon != before);
          stack.undo(document);
          QCOMPARE(std::get<Shape>(body(document.page(0).objects.back())).polygon, before);
      }
  };

  QTEST_GUILESS_MAIN(TstCoreShapes)
  #include "tst_core_shapes.moc"
  ```
  (Si `PointF` n'a pas d'`operator==`, comparer les sommets un à un par une aide locale.) Commande ciblée → rouge.
- [ ] **Étape 2 : catalogue.** Dans `shapegeometry.cpp`, un `static const std::vector<ShapePreset>` construit une
  fois : familles `Line` pour `line`, `line-horizontal` (axe 1), `line-vertical` (axe 2), `polyline`, `zigzag`,
  `chevron` ; `square` pour `square` et `circle` ; paramètres : `arrow` {longueur de la tête 0,35 (0,05 à 0,9),
  demi-largeur du corps 0,25 (0,05 à 0,5)}, `rounded-rectangle` {rayon 0,15 (0 à 0,5)}, `star`, `star-four`,
  `burst`, `burst-twelve` {rayon intérieur 0,45 / 0,35 / 0,6 / 0,7 (0,1 à 0,95)}, `callout` {pointe x 0,2, y 1,3
  (−1 à 2)} — fractions de la boîte. `findShapePreset` : recherche linéaire.
- [ ] **Étape 3 : boîtes et lignes.** `dragBox` : coin = boîte de `press` à `current` normalisée ; centre =
  boîte centrée sur `press` de demi-côtés `|current − press|` ; `square` : le plus grand des deux, signe du geste.
  `dragLine` : depuis le centre, `from = press − (current − press)` ; axe 1 : `to.y = from.y` ; axe 2 :
  `to.x = from.x` ; `constrain45` : angle arrondi au multiple de 45° le plus proche, longueur gardée.
- [ ] **Étape 4 : géométrie.** `rebuildShape` : `ShapeKind::Polygon` pour tous les préréglages fermés, sommets
  dans la boîte `{0, 0, w, h}` (ellipse et cercle : 64 sommets ; rectangle arrondi : arcs de 8 sommets par coin ;
  polygones réguliers inscrits ; étoiles et explosions alternées entre rayon extérieur et intérieur ; flèche : corps
  puis tête ; bulle : rectangle arrondi plus pointe). `bounds = {0, 0, w, h}` ; `makeShapeObject` pose
  `common.transform = translation(box.x, box.y)`, `outline`, `outlineWidth = style.width`, `fill =
  style.filled ? style.fill : 0` (alpha nul), `pen`, `brush`, `preset`, `parameters = defaults`.
  `makeLineObject` : `Line` à deux points (ligne, axes), trois (chevron), cinq (zigzag), deux (ligne brisée, dont les
  points s'ajoutent au lot 5 par les points de forme), plume = contour.
- [ ] **Étape 5 : points intelligents.** `smartPoints` et `dragSmartPoint` : un point par paramètre (deux pour la
  pointe de la bulle : un seul point qui porte x et y), positions dans la boîte, conversion inverse bornée par
  `minimum` / `maximum`.
- [ ] **Étape 6 : commande.** `SetShapeParametersCommand::redo` : objet trouvé par identifiant sur la page, `Shape`
  avec préréglage ; garde `m_before` au premier passage, pose `m_after`, `rebuildShape`, un `ObjectChanged` de zone
  emprise avant ∪ après (emprise calculée comme `AddObjectCommand` le fait, fonction d'emprise de `edits`) ; `undo`
  symétrique.
- [ ] **Étape 7 : vert**, `format.ps1`, preuve de tâche.
- [ ] **Étape 8 : commit.**
  ```powershell
  git -C <wt> add core/include/actcraft/core/shapegeometry.h core/src/shapegeometry.cpp core/include/actcraft/core/shapeedits.h core/src/shapeedits.cpp tests/tst_core_shapes.cpp
  git -C <wt> commit -m "core : catalogue des formes, tracé depuis le coin ou le centre, points intelligents (L4-T3)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t4"></a>
### Tâche L4-T4 — `core` : réglages de page et contrat de `Change` ignoré par le cache

**Objectif.** Remplir `pagesettings` : `SetGridCommand` (fusion d'une séance du créateur), `SetBackgroundCommand`,
`ExtendPageCommand`, `nextExtendedHeight`, `gridDrawsUnderObjects`, `gridDrawsAboveObjects`. **Le patron évident
est le mauvais** : émettre `PageChanged` pour toute grille ferait redessiner tout le cache pour une grille du dessus,
qui n'est plus dans le cache (L4-T7). Contrat : une grille du dessous qui change (avant ou après) → `PageChanged`
(zone vide = toute la page) ; sinon, un changement de page **ignoré par le cache** mais vu par l'interface
(miniature, surcouche).

**Contrat A13.** Si L0-S a posé un `Change` ignoré par `applyToCache` pour `SetPagePresentationCommand` (constat
de L4-T0), le réutiliser tel quel et ne rien ajouter à `undo.h` ni à `pagecache.cpp`. Sinon : ajouter à la fin de
`Change::Kind` la valeur `PageOverlayChanged` (« la page a changé hors du contenu que le cache photographie :
grille du dessus, réglages de présentation »), l'ignorer dans `applyToCache` (`core/src/pagecache.cpp`), compléter
tout `switch` sur `Change::Kind` (`git -C <wt> grep -n "Change::Kind::" -- core formats render ui tools bench`) et
le tester dans `tst_core_pagecache` (`overlayChangeIsIgnored`). Dans les deux cas, `DocumentController::settle`
émet `pageContentChanged` pour ce changement (contrat de L4-T1 : « objet ou page »).

**Fichiers.** `core/include/actcraft/core/pagesettings.h`, `core/src/pagesettings.cpp`,
`core/include/actcraft/core/undo.h`, `core/src/pagecache.cpp`, `tests/tst_core_pagesettings.cpp`,
`tests/tst_core_pagecache.cpp`.

**Test et commande bornée.** Commande ciblée `-R 'tst_core_pagesettings|tst_core_pagecache'` ; fonctions
`gridAboveChangeIsIgnoredByTheCache`, `gridBelowChangeRedrawsOnce`, `extendStepsByTenPercentOfBase`,
`threeUndosRestoreTheHeight`, `designerChangesMerge` en `PASS` ; mutation ; preuve de tâche.

**Interdits.** Ne pas changer le sens des `Change` existants. Ne pas toucher à `edits.cpp`.

**Terminé quand.** Fonctions vertes ; mutation de `gridDrawsUnderObjects` (rendre toujours `true`) rouge sur
`gridAboveChangeIsIgnoredByTheCache`, puis remise et verte (règle « Mutation ») ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge.** `tests/tst_core_pagesettings.cpp` :
  ```cpp
  #include <actcraft/core/document.h>
  #include <actcraft/core/pagecache.h>
  #include <actcraft/core/pagesettings.h>
  #include <actcraft/core/undo.h>

  #include <QtTest>

  using namespace actcraft::core;

  namespace {
  struct Spy final : PageCache
  {
      int inserts = 0, removes = 0, invalidations = 0;
      void insert(ObjectId) override { ++inserts; }
      void remove(ObjectId, const RectF&) override { ++removes; }
      void invalidate(ObjectId, const CacheArea&) override { ++invalidations; }
  };
  Grid aboveGrid(bool visible)
  {
      Grid g;
      g.visible = visible;
      g.drawnAbove = true;
      g.levels.push_back(GridLevel{});
      return g;
  }
  } // namespace

  class TstCorePageSettings : public QObject
  {
      Q_OBJECT

  private slots:
      void gridAboveChangeIsIgnoredByTheCache()
      {
          Document document;
          document.page(0).grid = aboveGrid(true);
          UndoStack stack;
          const Changes changes = stack.push(document, std::make_unique<SetGridCommand>(0, aboveGrid(false)));
          QCOMPARE(changes.size(), std::size_t{1}); // l'interface est prévenue
          Spy spy;
          applyToCache(changes, 0, spy);
          QCOMPARE(spy.invalidations, 0);
          QVERIFY(!document.page(0).grid.visible);
      }
      void gridBelowChangeRedrawsOnce()
      {
          Document document;
          Grid below = aboveGrid(true);
          below.drawnAbove = false;
          UndoStack stack;
          const Changes changes = stack.push(document, std::make_unique<SetGridCommand>(0, below));
          Spy spy;
          applyToCache(changes, 0, spy);
          QCOMPARE(spy.invalidations, 1);
      }
      void extendStepsByTenPercentOfBase()
      {
          Document document; // 1024 × 768
          UndoStack stack;
          stack.push(document, std::make_unique<ExtendPageCommand>(0));
          QCOMPARE(document.page(0).baseHeight, 768.0);
          QVERIFY(std::abs(document.page(0).height - 844.8) < 1e-9);
          stack.push(document, std::make_unique<ExtendPageCommand>(0));
          stack.push(document, std::make_unique<ExtendPageCommand>(0));
          QVERIFY(std::abs(document.page(0).height - 998.4) < 1e-9); // pas fixe (D10)
      }
      void threeUndosRestoreTheHeight()
      {
          Document document;
          UndoStack stack;
          for (int i = 0; i < 3; ++i)
              stack.push(document, std::make_unique<ExtendPageCommand>(0));
          for (int i = 0; i < 3; ++i)
              stack.undo(document);
          QCOMPARE(document.page(0).height, 768.0);
          QCOMPARE(document.page(0).baseHeight, 0.0);
      }
      void designerChangesMerge()
      {
          Document document;
          UndoStack stack;
          Grid g = aboveGrid(true);
          stack.push(document, std::make_unique<SetGridCommand>(0, g, false));
          g.levels[0].spacingX = 50;
          stack.push(document, std::make_unique<SetGridCommand>(0, g, true));
          g.levels[0].spacingX = 60;
          stack.push(document, std::make_unique<SetGridCommand>(0, g, true));
          QCOMPARE(stack.count(), std::size_t{1});
          stack.undo(document);
          QVERIFY(!document.page(0).grid.visible);
      }
  };

  QTEST_GUILESS_MAIN(TstCorePageSettings)
  #include "tst_core_pagesettings.moc"
  ```
  Si D10 retient un pas cumulé, `extendStepsByTenPercentOfBase` attend `768 × 1,1³` (amendement).
- [ ] **Étape 2 : contrat A13.** Selon le constat de L4-T0 (ci-dessus) : réutiliser, ou ajouter
  `PageOverlayChanged` avec son test `overlayChangeIsIgnored` dans `tests/tst_core_pagecache.cpp` (un `Change` de
  ce genre envoyé à un espion : aucun appel).
- [ ] **Étape 3 : commandes.** `gridDrawsUnderObjects` : `visible && !drawnAbove && un niveau visible` ;
  `gridDrawsAboveObjects` : `visible && drawnAbove && un niveau visible`. `SetGridCommand::redo` : garde la grille
  d'avant au premier passage, pose la nouvelle ; un seul `Change` de page : `PageChanged` si
  `gridDrawsUnderObjects(avant) || gridDrawsUnderObjects(après)`, sinon le changement ignoré par le cache.
  Fusion : la famille `kMergeGrid` par le mécanisme de fusion de la pile (`mergeId` et `mergeWith` de `UndoCommand`,
  lot 2), seulement si `merge` est vrai et la page la même ; la commande fusionnée garde le « avant » de la première.
  `SetBackgroundCommand` : `PageChanged`. `ExtendPageCommand::redo` : `m_beforeHeight`, `m_beforeBase`, puis
  `baseHeight = base` (premier clic), `height = nextExtendedHeight(page)` ; `PageChanged` ; `undo` remet les deux.
- [ ] **Étape 4 : vert**, puis mutation (règle « Mutation ») : `gridDrawsUnderObjects` rend `true` → rouge sur
  `gridAboveChangeIsIgnoredByTheCache` ; remettre → vert.
- [ ] **Étape 5 : preuve de tâche, commit.**
  ```powershell
  git -C <wt> add core/include/actcraft/core/pagesettings.h core/src/pagesettings.cpp tests/tst_core_pagesettings.cpp
  git -C <wt> add core/include/actcraft/core/undo.h core/src/pagecache.cpp tests/tst_core_pagecache.cpp   # seulement si modifiés (contrat A13)
  git -C <wt> commit -m "core : réglages de page (grille, fond, extension), grille du dessus ignorée par le cache (L4-T4)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t5"></a>
### Tâche L4-T5 — `core` : accrochage à la grille

**Objectif.** Remplir `gridsnap` (F-622) d'après la règle reformulée de la référence (`canvas/grid.py:221-251`) :
pour chaque niveau qui accroche, grille normale = arrondi de `(p − début) / pas` sur chaque axe dessiné ; grilles
« 45° » et « angle » = croisement le plus proche parmi les croisements voisins (rayon de deux pas) ; puis le plus
proche de tous les niveaux. Pas effectif = pas × `baseZoom` (« Échelle par défaut »).

**Fichiers.** `core/include/actcraft/core/gridsnap.h`, `core/src/gridsnap.cpp`, `tests/tst_core_gridsnap.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_core_gridsnap` ; fonctions `nearestOnNormalGrid`,
`onlySnappingLevelsCount`, `invisibleGridStillSnaps`, `snapDisabledReturnsNothing`,
`bottomLeftCornerLandsOnCrossing`, `textUsesItsBaseline`, `diagonalGridCrossings` en `PASS` ; preuve de tâche.

**Interdits.** Pas de QtGui. Ne pas dupliquer le calcul des croisements de `render/` : si le peintre en a un, le
laisser où il est ; `core/` a le sien (le peintre pourra l'appeler au lot 11, passe de nettoyage).

**Terminé quand.** Fonctions vertes, `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge.** `tests/tst_core_gridsnap.cpp` :
  ```cpp
  #include <actcraft/core/gridsnap.h>

  #include <QtTest>

  using namespace actcraft::core;

  namespace {
  Grid grid(double step, bool snapEnabled = true)
  {
      Grid g;
      g.visible = true;
      g.snapEnabled = snapEnabled;
      GridLevel level;
      level.spacingX = step;
      level.spacingY = step;
      g.levels.push_back(level);
      return g;
  }
  } // namespace

  class TstCoreGridSnap : public QObject
  {
      Q_OBJECT

  private slots:
      void nearestOnNormalGrid()
      {
          const auto p = nearestGridPoint(grid(50), {74, 26}, 1024, 768);
          QVERIFY(p.has_value());
          QCOMPARE(p->x, 50.0);
          QCOMPARE(p->y, 50.0);
      }
      void onlySnappingLevelsCount()
      {
          Grid g = grid(50);
          GridLevel fine;
          fine.spacingX = fine.spacingY = 10;
          fine.snap = false;
          g.levels.push_back(fine);
          QCOMPARE(nearestGridPoint(g, {74, 26}, 1024, 768)->x, 50.0);
      }
      void invisibleGridStillSnaps()
      {
          Grid g = grid(50);
          g.visible = false;
          QVERIFY(nearestGridPoint(g, {74, 26}, 1024, 768).has_value());
      }
      void snapDisabledReturnsNothing() { QVERIFY(!nearestGridPoint(grid(50, false), {74, 26}, 1024, 768).has_value()); }
      void bottomLeftCornerLandsOnCrossing()
      {
          Page page;
          page.grid = grid(50);
          PageObject o{Shape{}};
          o.common.id = 7;
          std::get<Shape>(body(o)).bounds = {0, 0, 40, 30};
          o.common.transform = translation(12, 12); // coin bas gauche en (12, 42)
          page.objects.push_back(o);
          const PointF d = snapDelta(page, {7}, {30, 0}); // ancrage visé (42, 42) → (50, 50)
          QCOMPARE(d.x, 38.0);
          QCOMPARE(d.y, 8.0);
          page.grid.snapEnabled = false;
          QCOMPARE(snapDelta(page, {7}, {30, 0}).x, 30.0);
      }
      void textUsesItsBaseline()
      {
          PageObject o{Text{}};
          auto& text = std::get<Text>(body(o));
          text.bounds = {0, 0, 100, 40};
          TextSpan span;
          span.baseline = {0, 30};
          text.runs.push_back(span);
          o.common.transform = translation(10, 10);
          const PointF a = snapAnchor(o);
          QCOMPARE(a.x, 10.0);
          QCOMPARE(a.y, 40.0);
      }
      void diagonalGridCrossings()
      {
          Grid g = grid(50);
          g.levels[0].pattern = GridPattern::Diagonal45;
          const auto p = nearestGridPoint(g, {26, 24}, 1024, 768);
          QVERIFY(p.has_value());
          QVERIFY(std::hypot(p->x - 26, p->y - 24) <= 50 * std::sqrt(2.0));
      }
  };

  QTEST_GUILESS_MAIN(TstCoreGridSnap)
  #include "tst_core_gridsnap.moc"
  ```
- [ ] **Étape 2 : implémenter.** `nearestGridPoint` ; `snapAnchor` : `textBaseline` (en-tête de L4-T16, talon qui
  rend `std::nullopt` tant que L4-T16 n'est pas faite : le test `textUsesItsBaseline` lit alors la ligne lue
  directement — implémenter ici la lecture de `runs[0].baseline` dans une fonction locale, et appeler
  `textBaseline` seulement si elle rend une valeur) ; sinon coin bas gauche de l'emprise transformée (fonction
  d'emprise de `edits`) ; `snapDelta` : ancrage du premier objet de `ids` dans l'ordre de la page, `+ delta`, puis
  écart à l'intersection la plus proche.
- [ ] **Étape 3 : vert**, `format.ps1`, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add core/include/actcraft/core/gridsnap.h core/src/gridsnap.cpp tests/tst_core_gridsnap.cpp
  git -C <wt> commit -m "core : accrochage à la grille, coin bas gauche et ligne de base (L4-T5)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t6"></a>
### Tâche L4-T6 — `formats` : natif — champs du lot 4

**Objectif.** Lire et écrire en `.actcraft` les champs posés par L4-T1 : `"preset"` et `"parameters"` d'une forme,
`"vertical"` d'un texte, `"baseHeight"` d'une page, **omis quand ils valent leur défaut** (un fichier du lot 3
réécrit sans modification garde ses octets de `document.json`). **Le patron évident est le mauvais** : ajouter la
clé toujours, ou la ranger parmi les clés inconnues gardées (L2-C8) — une clé connue lue doit sortir de
`unknownKeys`, sinon elle serait écrite deux fois. Un fichier plus ancien se lit avec les défauts ; une valeur
invalide (préréglage qui n'est pas une chaîne, paramètre non fini) donne un avertissement de lecture et le défaut.

**Fichiers.** `formats/src/nativejson.cpp`, `tests/tst_formats_lot4.cpp`.

**Test et commande bornée.** Commande ciblée `-R 'tst_formats_lot4|tst_formats_nativejson|tst_formats_nativefile'` ;
fonctions `presetAndParametersRoundTrip`, `defaultsAreOmitted`, `verticalTextRoundTrip`, `baseHeightRoundTrip`,
`olderFileReadsWithDefaults` en `PASS` ; preuve de tâche.

**Interdits.** Pas de QtGui dans `formats/`. Pas de version de format changée (`actcraft-document/1` : clés
facultatives ajoutées). Ne pas toucher au lecteur `.flipchart` (L4-T24).

**Terminé quand.** Fonctions vertes ; les tests existants du format natif inchangés et verts ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge.** `tests/tst_formats_lot4.cpp` (fonctions de lecture et d'écriture JSON du module :
  `formats::documentToJson` / `formats::documentFromJson` ou leurs noms réels dans `nativejson.h`, à lire d'abord) :
  ```cpp
  #include <actcraft/core/document.h>
  #include <actcraft/formats/nativejson.h>

  #include <QtTest>

  using namespace actcraft;

  namespace {
  core::Document sample()
  {
      core::Document document;
      core::PageObject shape{core::Shape{}};
      shape.common.id = 1;
      std::get<core::Shape>(core::body(shape)).preset = "arrow";
      std::get<core::Shape>(core::body(shape)).parameters = {0.4, 0.2};
      core::PageObject text{core::Text{}};
      text.common.id = 2;
      std::get<core::Text>(core::body(text)).html = "<p>ABC</p>";
      std::get<core::Text>(core::body(text)).vertical = true;
      document.page(0).objects = {shape, text};
      document.page(0).baseHeight = 768;
      document.page(0).height = 844.8;
      document.refreshNextId();
      return document;
  }
  core::Document roundTrip(const core::Document& d)
  {
      QStringList warnings;
      auto back = formats::documentFromJson(formats::documentToJson(d), &warnings);
      Q_ASSERT(back.has_value());
      return std::move(*back);
  }
  } // namespace

  class TstFormatsLot4 : public QObject
  {
      Q_OBJECT

  private slots:
      void presetAndParametersRoundTrip()
      {
          const auto d = roundTrip(sample());
          const auto& s = std::get<core::Shape>(core::body(d.page(0).objects[0]));
          QCOMPARE(QString::fromStdString(s.preset), QStringLiteral("arrow"));
          QCOMPARE(s.parameters.size(), std::size_t{2});
          QCOMPARE(core::fingerprint(d), core::fingerprint(sample()));
      }
      void defaultsAreOmitted()
      {
          core::Document plain;
          plain.page(0).objects.push_back(core::PageObject{core::Shape{}});
          plain.page(0).objects.back().common.id = 1;
          const QByteArray json = formats::documentToJson(plain);
          QVERIFY(!json.contains("\"preset\""));
          QVERIFY(!json.contains("\"parameters\""));
          QVERIFY(!json.contains("\"vertical\""));
          QVERIFY(!json.contains("\"baseHeight\""));
      }
      void verticalTextRoundTrip()
      {
          QVERIFY(std::get<core::Text>(core::body(roundTrip(sample()).page(0).objects[1])).vertical);
      }
      void baseHeightRoundTrip() { QCOMPARE(roundTrip(sample()).page(0).baseHeight, 768.0); }
      void olderFileReadsWithDefaults()
      {
          core::Document plain;
          plain.page(0).objects.push_back(core::PageObject{core::Text{}});
          plain.page(0).objects.back().common.id = 1;
          const auto back = roundTrip(plain);
          QVERIFY(!std::get<core::Text>(core::body(back.page(0).objects[0])).vertical);
          QVERIFY(back.page(0).objects[0].unknownKeys.empty());
          QCOMPARE(back.page(0).baseHeight, 0.0);
      }
  };

  QTEST_GUILESS_MAIN(TstFormatsLot4)
  #include "tst_formats_lot4.moc"
  ```
  (Adapter les deux appels aux signatures réelles de `nativejson.h`, sans changer ce que le test vérifie.)
- [ ] **Étape 2 : implémenter** dans `nativejson.cpp`, à côté des clés des mêmes objets : écriture conditionnelle,
  lecture avec contrôle de type, retrait des clés connues de l'ensemble « inconnues ».
- [ ] **Étape 3 : vert**, `format.ps1`, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add formats/src/nativejson.cpp tests/tst_formats_lot4.cpp
  git -C <wt> commit -m "formats : préréglage et points des formes, texte de haut en bas, hauteur d'origine en .actcraft (L4-T6)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t7"></a>
### Tâche L4-T7 — `render` : grille du dessus en surcouche, retirée du cache

**Objectif.** La grille « toujours visible » est dessinée par `GridOverlayItem` (au-dessus de `liftLayer`, posé par
L4-T2) et **plus** par `FixedPageItem` ; la condition `!shown.grid.drawnAbove` de la règle de différé
(`render/src/fixedpageitem.cpp:180`, lot 10) tombe, puisque rien n'est plus dessiné au-dessus des objets dans la
scène du cache. **Le patron évident est le mauvais** : garder la grille du dessus dans la page fixe et y insérer
les objets « avant » elle (lot 2) ; cela fige la limite de DA11 et redessine tout à chaque bascule.
Branchement : `render/` ne connaît pas `ui/`, et le QML ne passe pas de pointeur de page. La surcouche lit
donc la page par la page fixe : `GridOverlayItem` reçoit une propriété
`Q_PROPERTY(actcraft::render::FixedPageItem* fixedPage READ fixedPage WRITE setFixedPage NOTIFY fixedPageChanged)`
(**ajoutée** par cette tâche à son en-tête), se branche sur le signal de changement de source et sur le signal de
contenu de `FixedPageItem` (existants, ou ajoutés à la fin de son en-tête), et relit la grille de la page affichée.
`PageArea.qml` reçoit une ligne : `fixedPage: fixedPage` sur `gridOverlay`.

**Fichiers.** Ceux de la ligne L4-T7 de la table (`ui/qml/PageArea.qml` : une ligne).

**Test et commande bornée.** Commande ciblée
`-R 'tst_render_gridoverlay|tst_render_pendinglayer|tst_render_fixedpage'` ; fonctions `drawsOnlyTheGridAbove`,
`hidingTheGridAboveRebuildsNoCache`, `sharpOnePixelLines`, `strokeOnGridAbovePageIsDeferred` en `PASS` ; banc sans
écran ; preuve de tâche.

**Interdits.** Ne pas modifier un test du lot 10 sans comprendre : `tst_render_pendinglayer` a pu asserter qu'un
trait sur page à grille du dessus n'est pas différé ; cette assertion est **remplacée** par
`strokeOnGridAbovePageIsDeferred`, avec un commentaire qui renvoie à cette tâche (changement de règle voulu). Les
miniatures et le peintre partagé dessinent toujours la grille du dessus (inchangés).

**Terminé quand.** Fonctions vertes ; `tst_render_fixedpage` vert ; banc `content_check.ok = True` ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** `tests/tst_render_gridoverlay.cpp` (fenêtre hors écran en rendu logiciel, aides
  de `tst_render_fixedpage.cpp` recopiées : `QQuickWindow` + `QQuickRenderControl` ou `grabWindow`) :
  `drawsOnlyTheGridAbove` (page à grille du dessous : l'image de la surcouche est transparente ; grille du dessus :
  pixels noirs sur les lignes `x = 32 × k`) ; `hidingTheGridAboveRebuildsNoCache` (page fixe et surcouche sur la
  même page ; `SetGridCommand` qui cache la grille du dessus, `applyToCache` sur la page fixe : son compteur de
  reconstructions — celui des tests du lot 10 — n'a pas bougé ; `rebuildCount` de la surcouche + 1) ;
  `sharpOnePixelLines` (zoom 2 : une ligne de grille fait exactement 1 pixel d'écran, aucun pixel intermédiaire, comme
  `BORD grille-nette` du lot 10). Dans `tst_render_pendinglayer.cpp` : `strokeOnGridAbovePageIsDeferred` (page à
  grille du dessus, différé actif, trait inséré en fin de page : `pendingCount() == 1`, aucun redessin).
- [ ] **Étape 2 : surcouche.** `updatePaintNode` : nœuds de lignes de 1 px d'écran (épaisseur `1 / zoom` en
  coordonnées de page, ou géométrie en pixels d'écran sous un nœud de transformation), sans lissage, seulement si
  `gridDrawsAboveObjects` ; réutiliser `createGridNode` (`render/include/actcraft/render/paintedrenderer.h:49`) s'il
  dessine déjà au pixel, sinon une fonction locale ; `rebuildCount` + 1 à chaque reconstruction.
- [ ] **Étape 3 : page fixe.** Retirer le dessin de la grille du dessus et `m_gridDrawnAbove` ; la grille du dessous
  reste ; condition de différé sans `drawnAbove`.
- [ ] **Étape 4 : vert**, banc sans écran, preuve de tâche.
- [ ] **Étape 5 : commit.**
  ```powershell
  git -C <wt> add render/include/actcraft/render/gridoverlayitem.h render/src/gridoverlayitem.cpp render/include/actcraft/render/fixedpageitem.h render/src/fixedpageitem.cpp ui/qml/PageArea.qml tests/tst_render_gridoverlay.cpp tests/tst_render_pendinglayer.cpp tests/tst_render_fixedpage.cpp
  git -C <wt> commit -m "render : grille du dessus en surcouche hors du cache, trait différé sur page à grille du dessus (L4-T7)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t8"></a>
### Tâche L4-T8 — `tools` : outil des formes

**Objectif.** Remplir `ShapeTool` (en-tête de L4-T1) : à l'appui, point accroché (`core::nearestGridPoint` si la
grille de la page accroche) ; au mouvement, aperçu par `ToolContext::lift.show({objet})` — **aucune** commande
pendant le geste ; au relâché, si l'écart dépasse `kShapeMinDragScreen / view.zoom()`, `lift.clear()` puis
`document.execute(std::make_unique<core::AddObjectCommand>(document.pageIndex(), objet))`, sinon `lift.clear()`
seul (F-241, P1). Réglages : `user.shapeSettings()` (préréglage, remplissage, centre) et
`user.settings("shape")` (couleur et épaisseur du contour). Maj (`onKey(Qt::Key_Shift, …)`) : carré ou 45°.
Échap : annule.

**Fichiers.** `tools/include/actcraft/tools/shapetool.h`, `tools/src/shapetool.cpp`, `tests/tst_tools_shape.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_tools_shape` ; fonctions `dragPreviewsWithoutExecuting`,
`releaseExecutesOneAdd`, `singleClickAddsNothing`, `fromCenterSetting`, `shiftKeepsSquare`, `snapsPressAndRelease`,
`cancelClearsThePreview`, `unfilledChoiceHonoured` en `PASS` ; banc sans écran ; preuve de tâche.

**Interdits.** Aucun en-tête de `render/` ni de `ui/` dans `tools/`. Aucune écriture dans le document hors
`execute`. Pas de géométrie recopiée (appeler `shapegeometry`).

**Terminé quand.** Fonctions vertes ; `TESTS OK` ; banc joint.

**Étapes.**
- [ ] **Étape 1 : test rouge.** `tests/tst_tools_shape.cpp`, avec des faux de chaque interface du contexte :
  ```cpp
  #include <actcraft/core/document.h>
  #include <actcraft/core/edits.h>
  #include <actcraft/tools/shapetool.h>
  #include <actcraft/tools/userstate.h>

  #include <QtTest>

  using namespace actcraft;

  namespace {
  struct FakeDocument final : tools::DocumentEditor
  {
      core::Document document;
      int executed = 0;
      const core::Page& page() const override { return document.page(0); }
      std::size_t pageIndex() const override { return 0; }
      void execute(std::unique_ptr<core::UndoCommand> command) override
      {
          core::Changes changes;
          command->redo(document, changes);
          ++executed;
      }
      std::vector<core::PageObject> lift(const std::vector<core::ObjectId>&) override { return {}; }
      void drop(std::unique_ptr<core::UndoCommand>) override {}
      bool isLifting() const override { return false; }
  };
  struct FakeLift final : tools::LiftSurface
  {
      std::vector<core::PageObject> shown;
      void show(std::vector<core::PageObject> objects) override { shown = std::move(objects); }
      void setTransform(const core::Transform&) override {}
      void clear() override { shown.clear(); }
  };
  // FakeInk, FakeStrokes, FakeSelection, FakeView (zoom 1, pas de corbeille), FakeCommands : méthodes vides, sur le
  // modèle de tests/tst_tools_select.cpp (lot 3) ; un `Harness` les possède et construit le ToolContext.
  input::Sample at(double x, double y, input::Phase phase)
  {
      input::Sample s;
      s.x = x;
      s.y = y;
      s.phase = phase;
      s.device = input::DeviceKind::Mouse;
      return s;
  }
  } // namespace

  class TstToolsShape : public QObject
  {
      Q_OBJECT

  private slots:
      void dragPreviewsWithoutExecuting()
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("rectangle"), true, 0xFFFFFFFFu, false});
          tools::ShapeTool tool(h.context);
          tool.onSample(at(100, 100, input::Phase::Press));
          tool.onSample(at(200, 160, input::Phase::Move));
          QCOMPARE(h.lift.shown.size(), std::size_t{1});
          QCOMPARE(h.doc.executed, 0);
          QVERIFY(tool.isBusy());
      }
      void releaseExecutesOneAdd()
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("rectangle"), true, 0xFFFFFFFFu, false});
          tools::ShapeTool tool(h.context);
          tool.onSample(at(100, 100, input::Phase::Press));
          tool.onSample(at(200, 160, input::Phase::Move));
          tool.onSample(at(200, 160, input::Phase::Release));
          QCOMPARE(h.doc.executed, 1);
          QVERIFY(h.lift.shown.empty());
          const auto& o = h.doc.document.page(0).objects.back();
          QCOMPARE(o.common.layer, core::Layer::Middle);
          QCOMPARE(QString::fromStdString(std::get<core::Shape>(core::body(o)).preset), QStringLiteral("rectangle"));
      }
      void singleClickAddsNothing()
      {
          Harness h;
          tools::ShapeTool tool(h.context);
          tool.onSample(at(100, 100, input::Phase::Press));
          tool.onSample(at(101, 102, input::Phase::Release));
          QCOMPARE(h.doc.executed, 0);
      }
      void fromCenterSetting()
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("rectangle"), true, 0xFFFFFFFFu, true});
          tools::ShapeTool tool(h.context);
          tool.onSample(at(100, 100, input::Phase::Press));
          tool.onSample(at(150, 130, input::Phase::Release));
          const auto& o = h.doc.document.page(0).objects.back();
          QCOMPARE(o.common.transform.dx, 50.0);
          QCOMPARE(o.common.transform.dy, 70.0);
      }
      void shiftKeepsSquare()
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("rectangle"), true, 0xFFFFFFFFu, false});
          tools::ShapeTool tool(h.context);
          tool.onSample(at(0, 0, input::Phase::Press));
          QVERIFY(tool.onKey(Qt::Key_Shift, Qt::ShiftModifier));
          tool.onSample(at(100, 40, input::Phase::Release));
          const auto& s = std::get<core::Shape>(core::body(h.doc.document.page(0).objects.back()));
          QCOMPARE(s.bounds.width, s.bounds.height);
      }
      void snapsPressAndRelease()
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("rectangle"), true, 0xFFFFFFFFu, false});
          auto& grid = h.doc.document.page(0).grid;
          grid.snapEnabled = true;
          core::GridLevel level;
          level.spacingX = level.spacingY = 50;
          grid.levels.push_back(level);
          tools::ShapeTool tool(h.context);
          tool.onSample(at(48, 52, input::Phase::Press));
          tool.onSample(at(147, 103, input::Phase::Release));
          const auto& o = h.doc.document.page(0).objects.back();
          QCOMPARE(o.common.transform.dx, 50.0);
          QCOMPARE(std::get<core::Shape>(core::body(o)).bounds.width, 100.0);
      }
      void cancelClearsThePreview()
      {
          Harness h;
          tools::ShapeTool tool(h.context);
          tool.onSample(at(0, 0, input::Phase::Press));
          tool.onSample(at(100, 100, input::Phase::Move));
          tool.cancel();
          QVERIFY(h.lift.shown.empty());
          QVERIFY(!tool.isBusy());
          QCOMPARE(h.doc.executed, 0);
      }
      void unfilledChoiceHonoured()
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("rectangle"), false, 0xFFFFFFFFu, false});
          tools::ShapeTool tool(h.context);
          tool.onSample(at(0, 0, input::Phase::Press));
          tool.onSample(at(100, 100, input::Phase::Release));
          QCOMPARE(std::get<core::Shape>(core::body(h.doc.document.page(0).objects.back())).fill >> 24, 0u);
      }
  };

  QTEST_GUILESS_MAIN(TstToolsShape)
  #include "tst_tools_shape.moc"
  ```
  (Le `Harness` et les faux sont écrits en entier dans le fichier, d'après les signatures réelles de
  `toolcontext.h` ; `id()` rend « shape ».)
- [ ] **Étape 2 : implémenter** `shapetool.cpp` : `objetCourant()` construit par `makeShapeObject` /
  `makeLineObject` (famille du préréglage) depuis `dragBox` / `dragLine` ; accrochage de l'appui et du point courant
  par `nearestGridPoint(page.grid, p, page.width, page.height)`.
- [ ] **Étape 3 : vert**, banc sans écran, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add tools/include/actcraft/tools/shapetool.h tools/src/shapetool.cpp tests/tst_tools_shape.cpp
  git -C <wt> commit -m "tools : outil des formes, aperçu sans redessin du cache, une étape au relâché (L4-T8)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t9"></a>
### Tâche L4-T9 — `tools` : sélection — accrochage au dépôt

**Objectif.** Au dépôt d'un glissement de la sélection (lot 3, L3-T8), la translation finale passe par
`core::snapDelta(page, sélection, delta)` avant la commande `TransformObjectsCommand` (F-622) ; pendant le geste,
la couche soulevée suit le pointeur sans accrochage (comme la référence, qui aligne « au relâchement »). Coût cache
inchangé : un redessin au soulèvement, un au dépôt.

**Fichiers.** `tools/src/selecttool.cpp`, `tools/include/actcraft/tools/selectiongeometry.h`,
`tools/src/selectiongeometry.cpp`, `tests/tst_tools_select.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_tools_select` ; fonctions `dropSnapsBottomLeftCorner`,
`noSnapWithoutGridSnap`, `snapDragRefreshesTheCacheTwice` en `PASS` et toutes celles du lot 3 vertes ; banc sans
écran ; preuve de tâche.

**Interdits.** Ne pas changer une signature publique du lot 3. Ne pas modifier un test du lot 3.

**Terminé quand.** Fonctions vertes, lot 3 intact, `TESTS OK`, banc joint.

**Étapes.**
- [ ] **Étape 1 : tests rouges** dans `tests/tst_tools_select.cpp` (faux du fichier) : `dropSnapsBottomLeftCorner`
  (rectangle 40 × 30 en (12, 12), grille de 50 qui accroche, glissé de (30, 0) : la commande déposée a pour
  translation (38, 8)) ; `noSnapWithoutGridSnap` (même geste, `snapEnabled` faux : (30, 0)) ;
  `snapDragRefreshesTheCacheTwice` (faux `DocumentEditor` qui compte `lift` et `drop` : un `lift`, un `drop`,
  aucun `execute` pendant dix mouvements).
- [ ] **Étape 2 : implémenter.** Dans le dépôt du glissement : `delta = core::snapDelta(page, ids, delta)` (fonction
  d'aide dans `selectiongeometry` si la géométrie y est déjà).
- [ ] **Étape 3 : vert**, banc sans écran, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add tools/src/selecttool.cpp tools/include/actcraft/tools/selectiongeometry.h tools/src/selectiongeometry.cpp tests/tst_tools_select.cpp
  git -C <wt> commit -m "tools : la sélection déposée s'accroche à la grille (L4-T9)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t13"></a>
### Tâche L4-T13 — Réglages de grille : contrôleur, créateur, scénario `grille`

**Objectif.** Remplir `PageOptions` pour la grille (propriétés `gridVisible`, `gridSnap`, `gridAbove`,
`grid` ; `toggleGridHidden`, `toggleSnap`, `beginDesigner`, `setGrid`, `addLevel`, `removeLevel`, `resetGrid`,
`stepSpacing`, `endDesigner`), chaque réglage par `core::SetGridCommand` sur la page affichée (`merge` vrai entre
`beginDesigner` et `endDesigner`) ; `changed` émis sur `pageContentChanged` de la page affichée et sur
`pagesChanged`. Remplir `GridDesigner.qml` (fenêtre à nous : cadre Grille, cadre des niveaux, niveau courant,
aperçu immédiat, F-620) et `bench/scenarios/grille.json` (S7.2 sans la partie « rectangle déplacé », jouée par S7).

**Fichiers.** `ui/include/actcraft/ui/pageoptions.h`, `ui/src/pageoptions.cpp`,
`tests/tst_ui_pagesettings.cpp`, `ui/qml/GridDesigner.qml`, `bench/scenarios/grille.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_pagesettings|tst_captures_grille'` ; fonctions
`hideGridAboveRefreshesNoCache`, `hideGridBelowRefreshesOnce`, `snapToggleFollowsThePage`,
`addRemoveResetLevels`, `designerSessionMakesASingleStep` en `PASS` ; preuve visuelle hors écran (ci-dessous) ;
banc sans écran ; preuve de tâche.

**Preuve visuelle.** `captures.ps1 -Scenario grille -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire
"appPageSettings.gridVisible=false"` : lignes `LU appPageSettings.gridVisible=false OK`,
`LU pageView.cacheRefreshCount=${avant} OK` (après « Cacher la grille » sur la grille du dessus), `CAPTURES OK` ;
décrire `grille-du-dessus.png` (lignes nettes au-dessus d'un rectangle) et `grille-cachee.png`.

**Interdits.** Aucun libellé de l'original dans la boîte. Aucune écriture du document hors commandes.

**Terminé quand.** Fonctions vertes ; scénario `CAPTURES OK` avec les deux lectures ; `TESTS OK` ; banc joint.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_ui_pagesettings.cpp` (contrôleur réel, `DocumentController` réel,
  espion de cache par `setCacheObserver` du lot 3) : `hideGridAboveRefreshesNoCache` (page à grille du dessus
  visible, `toggleGridHidden()` : l'espion n'a reçu aucun `invalidate`, `gridVisible` faux) ;
  `hideGridBelowRefreshesOnce` (grille du dessous : un `invalidate` de toute la page) ; `snapToggleFollowsThePage`
  (`toggleSnap()` sur la page 1, `showPage(1)` : `gridSnap` relit la page 2) ; `addRemoveResetLevels` (deux niveaux
  après `addLevel`, un après `removeLevel(1)`, `resetGrid()` : un niveau, `snap` faux) ;
  `designerSessionMakesASingleStep` (`beginDesigner`, trois `setGrid`, `endDesigner`, un `undo()` : grille d'origine).
- [ ] **Étape 2 : implémenter** le contrôleur ; conversion `QVariantMap` ↔ `core::Grid` à nos clés (contrat de
  L4-T1).
- [ ] **Étape 3 : boîte.** `GridDesigner.qml` : `function open()` (montre la boîte, `appPageSettings.beginDesigner()`),
  fermeture → `endDesigner()` ; champs liés à `appPageSettings.grid` ; chaque changement appelle `setGrid` ; objets
  nommés `gridDesigner`, `gridSpacingX`, `gridAboveCheck`, `gridSnapCheck`.
- [ ] **Étape 4 : scénario** `bench/scenarios/grille.json` :
  ```json
  {
    "format": "actcraft-scenario/1",
    "name": "grille",
    "description": "S7.2 : créateur de grille, grille du dessus cachée sans redessin du cache, grille du dessous cachée avec un redessin.",
    "steps": [
      {"waitReady": true},
      {"command": "grid.designer"},
      {"expect": "gridDesigner.visible", "value": true},
      {"command": "grid.snap"},
      {"expect": "appPageSettings.gridSnap", "value": true},
      {"capture": "createur"},
      {"key": "Escape"},
      {"waitReady": true},
      {"remember": "pageView.cacheRefreshCount", "as": "avant"},
      {"command": "grid.hide"},
      {"waitReady": true},
      {"expect": "appPageSettings.gridVisible", "value": false},
      {"expect": "pageView.cacheRefreshCount", "value": "${avant}"},
      {"capture": "grille-cachee"}
    ]
  }
  ```
  (La fixture de départ — page à grille du dessus visible — se pose par une étape `fixture` existante du joueur si
  elle offre une page à grille, sinon par la commande `grid.designer` et la case « au-dessus » cochée par `pointer`
  (A11) ; le choix est écrit dans le rapport.)
- [ ] **Étape 5 : vert**, preuve visuelle, banc sans écran, preuve de tâche.
- [ ] **Étape 6 : commit.**
  ```powershell
  git -C <wt> add ui/include/actcraft/ui/pageoptions.h ui/src/pageoptions.cpp tests/tst_ui_pagesettings.cpp ui/qml/GridDesigner.qml bench/scenarios/grille.json
  git -C <wt> commit -m "ui : réglages de grille et créateur à niveaux, grille du dessus cachée sans redessin du cache (L4-T13)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t10"></a>
### Tâche L4-T10 — `tools` : sélection — points intelligents

**Objectif.** Une forme seule sélectionnée qui a des points intelligents (`core::smartPoints`) les montre
(`SelectionView::smartPoints`, coordonnées de page) ; un appui sur l'un d'eux (tolérance
`tools::kHandleRadiusScreen / zoom`) **soulève** la forme, l'aperçu suit (copie reconstruite montrée par
`lift.show`), et le relâché dépose avec **une** `core::SetShapeParametersCommand`. Les points se dessinent dans
`SelectionOverlay.qml` (losanges jaunes à contour sombre, à nous).

**Fichiers.** `tools/src/selecttool.cpp`, `tools/include/actcraft/tools/selectiongeometry.h`,
`tools/src/selectiongeometry.cpp`, `tests/tst_tools_select.cpp`, `ui/qml/SelectionOverlay.qml`.

**Test et commande bornée.** Commande ciblée `-R tst_tools_select` ; fonctions `smartPointsShownForArrow`,
`smartPointDragLiftsOnceAndDropsOneStep`, `noSmartPointForReadShapes` en `PASS`, lot 3 et L4-T9 verts ; banc sans
écran ; preuve de tâche.

**Interdits.** Comme L4-T9. Les poignées du lot 3 gardent la priorité si elles se recouvrent (ordre de test :
point intelligent seulement hors des huit poignées).

**Terminé quand.** Fonctions vertes, `TESTS OK`, banc joint.

**Étapes.**
- [ ] **Étape 1 : tests rouges** : `smartPointsShownForArrow` (flèche sélectionnée seule : `smartPoints` non vide
  dans la vue montrée ; deux objets sélectionnés : vide) ; `smartPointDragLiftsOnceAndDropsOneStep` (un `lift`, un
  `drop` portant une `SetShapeParametersCommand`, paramètres changés) ; `noSmartPointForReadShapes` (forme sans
  préréglage : vide).
- [ ] **Étape 2 : implémenter** (désignation du point, état de geste « point intelligent », aperçu, dépôt).
- [ ] **Étape 3 : QML.** `SelectionOverlay.qml` : `Repeater` sur `appTools.selectionFrame.smartPoints` (rempli par
  L4-T11 ; tableau vide d'ici là), placés par `pageView.pageToScene`.
- [ ] **Étape 4 : vert**, banc sans écran, preuve de tâche.
- [ ] **Étape 5 : commit.**
  ```powershell
  git -C <wt> add tools/src/selecttool.cpp tools/include/actcraft/tools/selectiongeometry.h tools/src/selectiongeometry.cpp tests/tst_tools_select.cpp ui/qml/SelectionOverlay.qml
  git -C <wt> commit -m "tools : points intelligents des formes sélectionnées, une étape au dépôt (L4-T10)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t11"></a>
### Tâche L4-T11 — `ui::ToolHost` : outil des formes branché

**Objectif.** Remplir les ajouts de L4-T1 à `ToolHost` : `chooseShape` (préréglage connu seulement, active
« shape », commande `tool.shape` cochée), `setShapeFilled`, `setShapeFillColor`, `setShapeFromCenter`, propriétés
`shapePreset`, `shapeFilled`, `shapeFromCenter`, `shapePresets` (ordre de `core::shapeCatalog`), `shapeOutline`
(contour normalisé par `makeShapeObject` / `makeLineObject` dans une boîte 1 × 1) ; mémoire des réglages de forme
dans le même `QSettings` que la mémoire par outil du lot 3 (clés `tools/shape/preset`, `tools/shape/filled`,
`tools/shape/fill`, `tools/shape/fromCenter`) ; `selectionFrame` ajoute la clé `smartPoints` (`[{x, y}]`, page).

**Fichiers.** `ui/src/toolhost.cpp`, `ui/src/tooladapters.h`, `ui/src/tooladapters.cpp`, `tests/tst_ui_shapes.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_shapes` ; fonctions `shapeToolActivatesWithPalette`,
`presetChoiceSurvivesRestart`, `outlineOfEveryPreset`, `smartPointsInSelectionFrame`,
`shapeGoesThroughTheCacheInterface` en `PASS` ; banc sans écran ; preuve de tâche.

**Interdits.** Aucun outil n'appelle la page fixe ni les couches : `shapeGoesThroughTheCacheInterface` le prouve
(le cache ne reçoit qu'un `insert` au relâché, aucun appel pendant le tracé).

**Terminé quand.** Fonctions vertes, `TESTS OK`, banc joint.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_ui_shapes.cpp` (montage de `tst_ui_tools.cpp` du lot 3 : `ToolHost` avec
  contrôleur, vue et couche soulevée réels, fichier de réglages temporaire) : `shapeToolActivatesWithPalette`
  (`chooseShape("circle")` : `currentTool == "shape"`, `shapePreset == "circle"`) ; `presetChoiceSurvivesRestart`
  (second `ToolHost` sur le même fichier : `circle`, `shapeFromCenter` gardé) ; `outlineOfEveryPreset` (chaque
  préréglage : au moins 2 points, tous dans [0, 1]) ; `smartPointsInSelectionFrame` (flèche sélectionnée :
  `selectionFrame["smartPoints"]` non vide) ; `shapeGoesThroughTheCacheInterface` (tracé injecté par
  `activeTool()->onSample` : espion `setCacheObserver` — zéro appel avant le relâché, un `insert` après).
- [ ] **Étape 2 : implémenter.**
- [ ] **Étape 3 : vert**, banc sans écran, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add ui/src/toolhost.cpp ui/src/tooladapters.h ui/src/tooladapters.cpp tests/tst_ui_shapes.cpp
  git -C <wt> commit -m "ui : outil des formes branché, réglages gardés, contours de la palette (L4-T11)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t12"></a>
### Tâche L4-T12 — QML : palette des formes, curseur ; scénario `formes`

**Objectif.** Remplir `ShapePalette.qml` (panneau vertical ancré à droite, visible quand l'outil courant est
« shape » : nuancier de contour 2 × 4 tiré des couleurs de la disposition, case « sans remplissage », bascule
« depuis le centre », grille des 22 formes dessinées par `Shape`/`PathPolyline` depuis `appTools.shapeOutline`,
forme courante encadrée ; objet `shapePalette`, boutons `shape-<preset>`) ; `ToolCursor.qml` : flèche suivie de la
miniature de la forme courante quand l'outil est « shape » ; scénario `formes` (S7.1).

**Fichiers.** `ui/qml/ShapePalette.qml`, `ui/qml/ToolCursor.qml`, `bench/scenarios/formes.json`.

**Test et commande bornée.** Commande ciblée `-R tst_captures_formes` ; preuve visuelle ; banc sans écran ; preuve
de tâche.

**Preuve visuelle.** `captures.ps1 -Scenario formes -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire
"appTools.currentTool=shape" -Lire "appDocument.shownObjectCount=2"` : `LU … OK` pour les deux, `PIXEL
rectangle-contour OK`, `PIXEL cercle-interieur-blanc OK`, `CAPTURES OK` ; décrire `palette.png`, `rectangle.png`,
`cercle-centre.png`, `fleche-point.png`.

**Interdits.** Aucune icône ni miniature de l'original ; formes dessinées depuis notre géométrie.

**Terminé quand.** `tst_captures_formes` vert ; preuve visuelle conforme ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : scénario rouge** `bench/scenarios/formes.json` :
  ```json
  {
    "format": "actcraft-scenario/1",
    "name": "formes",
    "description": "S7.1 : palette, rectangle depuis le coin, clic simple sans forme, cercle depuis le centre sans remplissage, point intelligent d'une flèche.",
    "steps": [
      {"waitReady": true},
      {"command": "tool.shape"},
      {"expect": "shapePalette.visible", "value": true},
      {"capture": "palette"},
      {"command": "shape.choose", "argument": "rectangle"},
      {"stroke": [[200, 200], [400, 320]], "pressure": 0.5},
      {"waitReady": true},
      {"checkPixel": "rectangle-contour", "at": [200, 260], "color": "#000000", "tolerance": 60},
      {"capture": "rectangle"},
      {"stroke": [[600, 600], [601, 601]], "pressure": 0.5},
      {"waitReady": true},
      {"expect": "appDocument.shownObjectCount", "value": 1},
      {"command": "shape.fill-color", "argument": "#43A047"},
      {"command": "shape.choose", "argument": "circle"},
      {"command": "shape.draw-from-center"},
      {"command": "shape.filled"},
      {"stroke": [[700, 300], [760, 360]], "pressure": 0.5},
      {"waitReady": true},
      {"expect": "appDocument.shownObjectCount", "value": 2},
      {"checkPixel": "cercle-interieur-blanc", "at": [700, 300], "color": "#FFFFFF", "tolerance": 8},
      {"capture": "cercle-centre"},
      {"expect": "appTools.currentTool", "value": "shape"}
    ]
  }
  ```
  La partie « flèche et point intelligent » s'ajoute à la fin : `shape.choose arrow`, tracé, `tool.select`, clic
  sur la flèche par `pointer` (A11), `expect` d'un point intelligent visible (`selectionOverlay` : objet
  `smartPoint0`), glisser le point, `expect appDocument.canUndo true`, `capture fleche-point`.
- [ ] **Étape 2 : QML** (palette, curseur).
- [ ] **Étape 3 : vert**, preuve visuelle, banc sans écran, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add ui/qml/ShapePalette.qml ui/qml/ToolCursor.qml bench/scenarios/formes.json
  git -C <wt> commit -m "ui : palette des formes et curseur à miniature ; scénario formes (L4-T12)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t14"></a>
### Tâche L4-T14 — Banc et joueur : `forme-page-annotee`, `grille-dessus`, étape `typeText`

**Objectif.** Deux scénarios de banc demandés par le lot 10 (écart 5) : `forme-page-annotee` (page générée avec
ses annotations ; rectangles posés un à un par `tool.shape` et tracés injectés — chaque insertion est hors fin de
page et redessine : on **mesure** le figement) ; `grille-dessus` (page générée à grille du dessus ; traits du
scénario `stroke` : différés depuis L4-T7, `cache.refreshes = 0` pendant les traits). Étape `typeText` du joueur
(`{"typeText": "Bonjour"}` : un `QKeyEvent` par caractère avec son texte, ligne `TEXTE <texte> OK`), ajoutée à la
**fin** de `UiStepKind` après les étapes de L0-S.

**Fichiers.** `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp`, `bench/src/runner.cpp`,
`tests/tst_bench_scenarios.cpp`, `tests/tst_bench_result.cpp`, `bench/include/actcraft/bench/uiscenario.h`,
`bench/src/uiscenario.cpp`, `tests/tst_bench_uiscenario.cpp`, `docs/banc.md`.

**Test et commande bornée.** Commande ciblée `-R 'tst_bench_scenarios|tst_bench_result|tst_bench_uiscenario'` ;
fonctions `shapeOnAnnotatedPageScenario`, `gridAboveScenarioDrawsStrokes`, `lot4ScenariosWithFrames`,
`typeTextStepParsed` en `PASS` ; banc sans écran : les deux scénarios présents dans le JSON, `cache.refreshes`
relevé ; preuve de tâche.

**Interdits.** Ne pas changer les scénarios existants ni leurs critères. Le verdict reste un résultat (DA7).

**Terminé quand.** Fonctions vertes ; JSON du banc joint avec les deux scénarios ; `docs/banc.md` décrit les deux
scénarios et l'étape `typeText` ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : tests rouges** : `shapeOnAnnotatedPageScenario` (le scénario existe, a au moins 10 formes, sur
  une page dont la couche du dessus a des annotations) ; `gridAboveScenarioDrawsStrokes` (page à grille du dessus,
  traits) ; `lot4ScenariosWithFrames` (sur le modèle de `sevenScenariosWithFrames` du lot 3 : compte des scénarios
  + 2, chacun avec ses images) ; `typeTextStepParsed` (`{"typeText": "Bonjour"}` → `UiStepKind::TypeText`, texte
  gardé ; chaîne vide : `SCENARIO INVALIDE`).
- [ ] **Étape 2 : implémenter** (scénarios, choix de l'outil par commande avant la mesure comme L3-T19, étape du
  joueur et sa ligne `TEXTE`).
- [ ] **Étape 3 : vert**, banc sans écran (lire `forme-page-annotee` et `grille-dessus` dans le JSON), preuve de
  tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add bench/include/actcraft/bench/scenarios.h bench/src/scenarios.cpp bench/src/runner.cpp tests/tst_bench_scenarios.cpp tests/tst_bench_result.cpp bench/include/actcraft/bench/uiscenario.h bench/src/uiscenario.cpp tests/tst_bench_uiscenario.cpp docs/banc.md
  git -C <wt> commit -m "banc : scénarios forme sur page annotée et grille au-dessus ; étape typeText du joueur (L4-T14)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

## Hypothèses à revoir (porte de fin du lot 4)

### Niveau 1
- **H4-1** — Catalogue des formes : celui de la référence (22) tant que D1 ne dit pas autre chose.
- **H4-2** — Courbe de Bézier et forme libre absentes si la 3.5.18 ne les a pas (écart 2).
- **H4-3** — Texte de haut en bas = lettres empilées (colonne d'un caractère, retour à la ligne partout) : D9.
- **H4-4** — Zones de page : comportement fixé par D11 ; tant qu'il ne l'est pas, L4-T27 et L4-T28 ne commencent pas.
- **H4-5** — Panneau des navigateurs ancré seulement au lot 4 ; flottant au lot 6.

### Niveau 2
- **H4-6** — Ancrage d'un texte créé par ActCraft (sans lignes lues) : coin bas gauche de la première ligne mesurée
  par `ui::TextEditing` ; à défaut, de l'emprise.
- **H4-7** — Miniatures regroupées à 400 ms et suspendues pendant un contact : à revoir si `miniatures` montre un
  coût de copie de page supérieur à 2 ms sur la page générée.
- **H4-8** — Formes et textes sous les annotations : redessin immédiat (limite de DA11). Si `forme-page-annotee`
  dépasse la limite en D3D11 : tuiles (L10-T7) ou cache par groupe de couches, décision de la porte.
- **H4-9** — « Accrocher à la grille » bascule l'autorisation d'accrochage de la grille de la page (D6).
- **H4-10** — Texte de départ « Votre texte » (à nous) ; police par défaut N2-3.

## Décisions tranchées par ce plan

1. **Version 0.5.0**, à la fin de 4b ; un seul plan de réservation pour les deux sous-lots.
2. **Aperçu des formes par la couche soulevée** : aucun redessin pendant le tracé, une étape au relâché.
3. **Grille du dessus en surcouche, hors du cache** : bascule sans redessin, traits différés rétablis.
4. **Accrochage au dépôt**, pas pendant le glissement (comme la référence).
5. **Texte soulevé pendant son édition** (4b) : aucun redessin pendant la frappe.
6. **Miniatures rendues hors du fil principal** (4b), regroupées, jamais pendant un contact.
7. **Presse-papiers des pages interne** au lot 4 ; entre documents au lot 5.
8. **Réalisation libre** (décision 0100) : miniatures peintes par un élément C++ plutôt qu'un fournisseur d'images
   enregistré dans le moteur (aucun fichier de `app/` touché) ; à signaler au rapport de lot.

## Point d'étape du sous-lot 4a (orchestrateur, hors tâches)

Joué après V6, dans le worktree du chantier, avant de lancer 4b.

1. **Double relecture** (conformité, qualité) du diff de 4a ; correctifs `L4-C<n>` (deux tours au plus).
2. **Suite** (jeton `tests`) : `scripts/test.ps1`, forme bornée, N ≥ N3 + 23, `TESTS OK` ; talons restants : ceux
   de 4b seulement (`git -C <wt> grep -n "Talon L4-T1" -- tests core tools render ui bench` ne cite que des tâches
   de 4b).
3. **Captures hors écran** : `formes`, `grille`, `premiers-pas`, `selection` (lot 3), `cache-differe` (lot 10) →
   `CAPTURES OK`.
4. **Banc hors écran** : `forme-page-annotee`, `grille-dessus` relevés ; résultat noté dans le fichier d'état.
5. `graphify update .`, commit de `graphify-out/`. Pas de version, pas de PR : la clôture du lot est à la fin de 4b.
