# Livraison complète — lot 3 « Outils I » — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**But.** Version **0.4.0** (amendement 1, § 6 : lot 3 → 0.4.0, le lot 10 livre 0.3.0). Écrire, gommer, sélectionner,
annuler, pincer pour zoomer : sélection (clic, rectangle qui touche, déplacer, huit poignées, redimensionner,
rotation, barre d'objet), stylo et marqueur (mémoire par outil, translucidité du marqueur), gomme qui coupe et scinde
les seules annotations, Annuler / Rétablir, Supprimer, corbeille, Effacer ▸, Rétablir la page, gestes à deux doigts
(décision 6 du cadrage ; F-050, F-130, F-131, F-132, F-136, F-137, F-201, F-202 (partie), F-210, F-225, F-230,
F-520 à F-524, F-528, F-541, F-951 à F-953 pour les fonctions du lot). Scénario d'acceptation du cadrage : **S6** ;
tâche d'ouverture : **S30** ; scénarios ajoutés par ce plan : S34 à S37.

**Architecture.** Tout passe par une commande nommée (`actions/`) ; un outil (`tools/`) ne voit que des interfaces
(`tools::ToolContext`) ; toute modification du document passe par la pile de `core/`. Trois contraintes
structurent le lot :

1. **Routage unique du doigt (DA-A1 de l'amendement).** Chaque contact (stylet, souris, doigt) passe par un seul
   routeur `input::PointerRouter`, dont la politique `input::RoutingPolicy` est remplaçable : la décision (outil,
   geste ou rien) est prise à l'appui et tenue pour tout le contact ; chaque contact est rattaché à un utilisateur
   (un seul au lot 3) ; l'utilisateur (`tools::UserState`) porte l'outil courant, les réglages par outil et la
   sélection. La vue (`ui::PageView`) ne décide plus rien elle-même ; le lot 11 ajoutera un utilisateur et des
   politiques sans réécrire les outils.
2. **Cache par son interface figée (DA11), sans redessin pendant un geste.** Le patron évident — pousser une
   commande par mouvement de souris, fusionnée dans la pile (`mergeId`) — redessinerait tout le cache à chaque
   mouvement (chaque `ObjectChanged` d'un objet versé redessine tout, limite de DA11, H10-8). Le lot 3 **soulève** :
   au début d'un glissement (sélection) ou au premier contact de la gomme avec un trait, les objets visés sont
   retirés de la page affichée hors pile (`DocumentController::liftObjects`), ce que le cache reçoit par
   `PageCache::remove(id, emprise)` ; une **couche soulevée** (`render::LiftLayerItem`, au-dessus de la couche
   d'attente du lot 10) les dessine pendant le geste, déplacés par une simple transformation (aucun nœud
   reconstruit) ou recoupés par la gomme ; au relâché, `DocumentController::dropLifted` remet les originaux puis
   exécute **une** commande par la pile, et le cache reçoit `PageCache::insert(id)` pour chaque objet soulevé encore
   présent (`core::changesAfterLift`). Coût : un redessin au soulèvement, un au dépôt, aucun pendant le geste ;
   mesuré par les scénarios de banc `soulever` et `gommer` (S36, H10-8 e). Aucun outil n'appelle la page fixe, la
   couche d'attente ni la couche soulevée : prouvé par test (`tst_ui_lift`, `tst_ui_tools`, `tst_modules`).
3. **Fichiers chauds réservés en tête de lot.** `L3-T1` pose d'un coup les commandes, menus (sous-menu « Effacer »),
   emplacements de palette, barre d'objet, icônes, enregistrements CMake, fichiers et tests du lot (talons
   compilables, tests sautés `QSKIP`), et les API figées de `ui::ToolHost`, `DocumentController` et `PageView`
   (corps vides). `L3-T2` pose la coquille QML (palette, corbeille, surcouches, gestionnaires des commandes). Les
   tâches suivantes ne remplissent que leurs propres fichiers ; deux tâches d'une même vague ne partagent aucun
   fichier.

**Pile.** C++20, Qt 6.12 (Core, Gui, Quick, QuickControls2, Test), CMake + Ninja, MSVC 2022, PowerShell 7.

**Amendement 1 du plan (relecture du 2026-10-18, « exécutable après correctifs »).** Ce qui est commun aux lots 3
et 7 passe dans la **tâche de socle `L0-S`** (plan `docs/plans/2026-10-18-livraison-complete-socle-3-7-PLAN.md`,
écrit à part), jouée **avant** ce lot : disposition et sous-menus, commandes des deux lots, étapes du joueur de
scénarios (tout L3-T17), chemin d'entrée de la vue (événement → `Sample` → `PointerRouter`, couvertures des
surcouches → filtre d'échantillons → `Pipeline` → outil actif), `isBusy` de l'outil actif pour la minuterie de
versement, API vide de `DocumentController` (**sans** `editAboutToApply` au soulèvement), une seule
`ReplaceObjectsCommand`, emplacements QML avec leurs `required property`, `UserState::openMathTools`. **L3-T1, L3-T2
et L3-T17 seront amendés** (réduits à ce que le socle ne pose pas, ou retirés) quand le plan du socle sera écrit ;
d'ici là, leurs sections restent celles de 60a4290 et ne doivent **pas** être exécutées. Les correctifs propres au
lot 3 (B1, M1 à M4, M8, M9, mineurs 1 à 7) sont appliqués dans les sections concernées ci-dessous ; leur relevé est
en fin de plan (« Correctifs de la relecture »).

**Point de départ.** Le lot 3 démarre sur `feat/livraison-complete` **après la clôture du lot 10** (merge de
L10-T4, L10-T6 et de L10-T7 si elle est jouée) : `FixedPageItem` en attente (`pendingCount`, `foldPending`),
`PendingLayerItem`, `PageView::pendingLayer`, versement différé dans `requestCacheRefresh`, fabrique
`render::createObjectNode` (L10-T2). Ce plan est écrit sur e016107 (L10-T4 non mergée) : les numéros de ligne cités
sont indicatifs, les noms font foi. Tests au départ : **94** (fin du lot 10).

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L3-T0 | Ouverture : vidéos, REA et ancien dépôt confrontés au lot (sélection, stylo, marqueur, gomme, annulation, effacer, corbeille, gestes, schéma d'entrée) | — | `docs/verifications/2026-10-18-lot3-principes.md` (hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot3.md`) | isolé | concepteur-critique | rapport commité, relevé brut hors dépôt présent, `NOMS OK`, `git grep` des titres du corpus vide, décisions D1 à D12 écrites | S30 | #tache-l3-t0 |
| L3-T1 | Réservation I : commandes, disposition (sous-menus, barre d'objet), icônes, fichiers et tests du lot enregistrés, API figées | — | `actions/data/commands.json`, `actions/data/layout.json`, `actions/include/actcraft/actions/layout.h`, `actions/src/layout.cpp`, `tests/tst_actions_layout.cpp`, `ui/include/actcraft/ui/commands.h`, `ui/src/commands.cpp`, `tests/tst_ui_commands.cpp`, `ui/qml/CommandMenu.qml`, `ui/icons/{selection,marqueur,gomme,annuler,retablir,supprimer,corbeille,effacer,deplacer,rotation,translucidite,agrandir,reduire}.svg`, `core/CMakeLists.txt`, `input/CMakeLists.txt`, `tools/CMakeLists.txt`, `render/CMakeLists.txt`, `ui/CMakeLists.txt`, `tests/core.cmake`, `tests/input.cmake`, `tests/tools.cmake`, `tests/render.cmake`, `tests/ui.cmake`, `tests/bench.cmake`, talons neufs `core/include/actcraft/core/{hittest,erasegeometry,objectedits,lift}.h`, `core/src/{hittest,erasegeometry,objectedits,lift}.cpp`, `input/include/actcraft/input/{routing,gesture}.h`, `input/src/{routing,gesture}.cpp`, `tools/include/actcraft/tools/{tool,toolcontext,userstate,toolregistry,erasertool,selecttool,selectiongeometry}.h`, `tools/src/{userstate,toolregistry,erasertool,selecttool,selectiongeometry}.cpp`, `render/include/actcraft/render/liftlayeritem.h`, `render/src/liftlayeritem.cpp`, `ui/include/actcraft/ui/toolhost.h`, `ui/src/{toolhost,tooladapters}.cpp`, `ui/src/tooladapters.h`, `ui/qml/{SelectionOverlay,ObjectBar,TranslucencyPopup,TrashCan,PageContextMenu,ToolCursor}.qml`, `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`, `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`, 13 tests talons `tests/tst_{core_hittest,core_erase,core_objectedits,core_lift,input_routing,input_gesture,tools_registry,tools_eraser,tools_select,render_liftlayer,render_strokeshape,ui_lift,ui_tools}.cpp`, 4 scénarios talons `bench/scenarios/{selection,corbeille,marqueur,ecrire-gommer-annuler}.json` | isolé | implementeur-tdd | `100% tests passed … out of N`, N ≥ 111, `TESTS OK`, `tst_actions_layout` (dont `submenuItems`, `objectBarRead`, `lot3EntriesDeclared`), `tst_ui_commands` (dont `submenuInMenus`), `tst_ui_shell` Passed ; `captures.ps1 -Scenario premiers-pas` hors écran `CAPTURES OK` | S6, S34, S35 | #tache-l3-t1 |
| L3-T2 | Réservation II : coquille QML (palette, corbeille, surcouches, gestionnaires des commandes du lot) | L3-T1 | `ui/qml/Main.qml`, `ui/qml/PageArea.qml`, `ui/qml/ToolPalette.qml`, `tests/tst_ui_shell.cpp` | isolé | implementeur-tdd | `TESTS OK`, N ≥ 111, `tst_ui_shell` Passed (dont `lot3ShellPlaceholders`, `everyLot3CommandHasAHandler`) ; `captures.ps1 -Scenario premiers-pas` hors écran `CAPTURES OK` ; banc hors écran `content_check.ok = True` | S34, S35 | #tache-l3-t2 |
| L3-T3 | `core` : désignation (objet touché, rectangle qui touche) et géométrie de la gomme | L3-T1 | `core/include/actcraft/core/hittest.h`, `core/src/hittest.cpp`, `core/include/actcraft/core/erasegeometry.h`, `core/src/erasegeometry.cpp`, `tests/tst_core_hittest.cpp`, `tests/tst_core_erase.cpp` | isolé | implementeur-tdd | `TESTS OK`, N ≥ 111, `tst_core_hittest` (dont `topmostObjectWins`, `strokeHitUsesWidthAndTolerance`, `rectangleTouchingSelects`) et `tst_core_erase` (dont `middleCutSplitsInTwo`, `sweepDoesNotSkip`) Passed | S6 | #tache-l3-t3 |
| L3-T10 | `render` : couche soulevée `LiftLayerItem` (dessin identique à la page fixe, transformation sans reconstruction) | L3-T1 | `render/include/actcraft/render/liftlayeritem.h`, `render/src/liftlayeritem.cpp`, `tests/tst_render_liftlayer.cpp` | isolé | concepteur-critique | `TESTS OK`, N ≥ 111, `tst_render_liftlayer` Passed (dont `drawsLikeTheFixedPage`, `transformDoesNotRebuild`, `clearEmpties`) ; banc hors écran `content_check.ok = True` | S6 | #tache-l3-t10 |
| L3-T4 | `core` : commandes d'édition du lot et changements vus par le cache après un soulèvement | L3-T0, L3-T1, L3-T3 | `core/include/actcraft/core/objectedits.h`, `core/src/objectedits.cpp`, `core/include/actcraft/core/lift.h`, `core/src/lift.cpp`, `tests/tst_core_objectedits.cpp`, `tests/tst_core_lift.cpp` | isolé | concepteur-critique | `TESTS OK`, N ≥ 111, `tst_core_objectedits` (dont `replaceStrokesKeepsRankAndIds`, `clearAnnotationsKeepsShapes`, `revertPageIsOneStep`) et `tst_core_lift` (dont `liftThenDropMoveYieldsInsertOnly`, `liftThenCancelReinserts`, `erasePiecesInsertedNoRemoveOfLifted`) Passed ; mutation de `changesAfterLift` rouge puis remise | S6, S35 | #tache-l3-t4 |
| L3-T5 | `input` : routeur unique des contacts (politique remplaçable, utilisateur par contact) et geste à deux doigts | L3-T0, L3-T1 | `input/include/actcraft/input/sample.h`, `input/include/actcraft/input/routing.h`, `input/src/routing.cpp`, `input/include/actcraft/input/gesture.h`, `input/src/gesture.cpp`, `tests/tst_input_routing.cpp`, `tests/tst_input_gesture.cpp` | isolé | concepteur-critique | `TESTS OK`, N ≥ 111, `tst_input_routing` (dont `secondFingerCancelsToolAndStartsGesture`, `palmIgnoredWhileStylusDown`, `decisionHeldForWholeContact`, `policyIsReplaceable`, `simultaneousPinchDrawsNothing`, `stylusTakesOverFromFinger`) et `tst_input_gesture` (dont `pinchScalesAroundCenter`) Passed ; `tst_input` Passed | S6 | #tache-l3-t5 |
| L3-T6 | `tools` : interface `Tool`, contexte d'outil, registre, état d'utilisateur, stylo et marqueur | L3-T0, L3-T1 | `tools/include/actcraft/tools/tool.h`, `tools/include/actcraft/tools/toolcontext.h`, `tools/include/actcraft/tools/userstate.h`, `tools/src/userstate.cpp`, `tools/include/actcraft/tools/toolregistry.h`, `tools/src/toolregistry.cpp`, `tools/include/actcraft/tools/pentool.h`, `tools/src/pentool.cpp`, `tests/tst_pentool.cpp`, `tests/tst_tools_registry.cpp` | isolé | concepteur-critique | `TESTS OK`, N ≥ 111, `tst_tools_registry` (dont `builtinsRegistered`, `userKeepsSettingsPerTool`, `defaultsPerTool`) et `tst_pentool` (dont `highlighterStrokeIsTranslucentKind`, `newStrokesHaveRoundCaps`) Passed | S6, S37 | #tache-l3-t6 |
| L3-T11 | `render` : forme du trait (bouts et jonctions ronds, courbe lissée) et marqueur translucide d'un seul tenant | L3-T0, L3-T1, L3-T2 | `render/include/actcraft/render/strokegeometry.h`, `render/src/strokegeometry.cpp`, `render/include/actcraft/render/inkitem.h`, `render/src/inkitem.cpp`, `render/src/strokerenderer.cpp`, `render/src/pagepainter.cpp`, `ui/qml/PageArea.qml`, `tests/tst_render_strokeshape.cpp`, `tests/tst_render_painter.cpp` | isolé | concepteur-critique | `TESTS OK`, N ≥ 111, `tst_render_strokeshape` (dont `roundCapsCoverTheDisk`, `selfCrossingHighlighterDoesNotDarken`, `twoHighlighterPassesDarken`) et `tst_render_painter` Passed ; banc hors écran `content_check.ok = True` ; à l'écran D3D11 (jeton `ecran`) `anticrenelage` `CAPTURES OK` | S37 | #tache-l3-t11 |
| L3-T12 | `DocumentController` : soulever et déposer, Annuler / Rétablir après le trait en cours, Supprimer, Effacer, Rétablir la page | L3-T4 | `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`, `tests/tst_ui_document.cpp`, `tests/tst_ui_lift.cpp` | isolé | concepteur-critique | `TESTS OK`, N ≥ 111, `tst_ui_lift` (dont `liftRemovesThroughTheCacheInterface`, `dropInsertsLiftedObjects`, `undoWhileLiftingCancelsFirst`, `pageChangeWhileLiftingDropsFirst`, `saveWhileLiftingDropsFirst`) et `tst_ui_document` (dont `revertPageReturnsToSaved`, `revertNeverSavedEmptiesPage`, `undoAnnouncesTheEditFirst`) Passed | S6, S35 | #tache-l3-t12 |
| L3-T17 | Joueur de scénarios : étapes `pinch` et `touch` (doigts injectés), `expect` avec tolérance | L3-T1 | `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`, `bench/include/actcraft/bench/injector.h`, `bench/src/injector.cpp`, `tests/tst_bench_uiscenario.cpp`, `tests/tst_bench_injector.cpp` | isolé | implementeur-tdd | `TESTS OK`, N ≥ 111, `tst_bench_uiscenario` (dont `pinchStepParsed`, `touchStepParsed`, `expectWithTolerance`) et `tst_bench_injector` (dont `pinchDeliversTwoTouchPoints`) Passed | S6 | #tache-l3-t17 |
| L3-T7 | `tools` : gomme (soulève chaque trait touché une fois, une commande au relâché) | L3-T3, L3-T4, L3-T6 | `tools/include/actcraft/tools/erasertool.h`, `tools/src/erasertool.cpp`, `tests/tst_tools_eraser.cpp` | isolé | implementeur-tdd | `TESTS OK`, N ≥ 111, `tst_tools_eraser` Passed (dont `liftsEachTouchedStrokeOnce`, `nothingExecutedDuringTheGesture`, `oneCommandAtRelease`, `shapesAreNeverTouched`, `cancelRestoresEverything`) | S6 | #tache-l3-t7 |
| L3-T8 | `tools` : sélection — désigner, rectangle qui touche, déplacer par soulèvement, déposer sur la corbeille | L3-T3, L3-T4, L3-T6 | `tools/include/actcraft/tools/selecttool.h`, `tools/src/selecttool.cpp`, `tools/include/actcraft/tools/selectiongeometry.h`, `tools/src/selectiongeometry.cpp`, `tests/tst_tools_select.cpp` | isolé | implementeur-tdd | `TESTS OK`, N ≥ 111, `tst_tools_select` Passed (dont `clickSelectsTopmost`, `rubberBandSelectsTouched`, `dragLiftsOnceAndDropsOnce`, `lockedObjectDoesNotMove`, `dropOverTrashDeletes`, `escapeCancelsDrag`) | S6, S34 | #tache-l3-t8 |
| L3-T13 | `PageView` : gestes à deux doigts (pincer, défiler) sur le chemin d'entrée du socle, demandes de redessin regroupées par tour, outil actif occupé | L0-S, L3-T5, L3-T6 | `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`, `tests/tst_pageview.cpp`, `tests/tst_ui_drawing.cpp` | isolé | concepteur-critique | `TESTS OK`, N ≥ 111, `tst_pageview` (dont `pinchZoomsAroundItsCenter`, `twoFingerPanScrolls`, `secondFingerEndsFingerStroke`, `palmIgnoredDuringStylus`, `cacheRefreshCountCounts`, `requestsOfOneTurnMakeOneRedraw`, `foldWaitsForTheActiveTool`, `latencyProbeSeesToolSamples`, `pageChangeEndsToolStroke`) et `tst_ui_drawing` Passed ; banc hors écran `content_check.ok = True` | S6 | #tache-l3-t13 |
| L3-T9 | `tools` : sélection — huit poignées, redimensionner (proportionnel au coin), rotation, barre d'objet | L3-T8 | `tools/include/actcraft/tools/selecttool.h`, `tools/src/selecttool.cpp`, `tools/include/actcraft/tools/selectiongeometry.h`, `tools/src/selectiongeometry.cpp`, `tests/tst_tools_select.cpp` | isolé | implementeur-tdd | `TESTS OK`, N ≥ 111, `tst_tools_select` Passed (dont `eightHandlesAroundTheFrame`, `cornerHandleResizesProportionally`, `rotationAroundCenter`, `handlesHiddenWhileDragging`, `barButtonTriggersCommand`, `restrictionsHonoured`) | S34 | #tache-l3-t9 |
| L3-T14 | `ui::ToolHost` : outils branchés (commandes, mémoire par outil gardée au redémarrage, adaptateurs d'interfaces) | L3-T2, L3-T7, L3-T8, L3-T10, L3-T12, L3-T13 | `ui/include/actcraft/ui/toolhost.h`, `ui/src/toolhost.cpp`, `ui/src/tooladapters.h`, `ui/src/tooladapters.cpp`, `ui/src/penadapters.h`, `ui/src/penadapters.cpp`, `ui/qml/Main.qml`, `tests/tst_ui_tools.cpp`, `tests/tst_ui_drawing.cpp`, `tests/tst_modules.cpp` | isolé | concepteur-critique | `TESTS OK`, N ≥ 111, `tst_ui_tools` (dont `colorGoesToActiveTool`, `settingsSurviveRestart`, `eraseGoesThroughTheCacheInterface`, `dragGoesThroughTheCacheInterface`, `dragOfTwoObjectsCostsTwoRedraws`, `undoAfterEraseRestores`, `liftDoesNotCancelTheLiftingTool`, `pageChangeWhileDraggingClearsTheLiftLayer`, `escapeCancelsTheGesture`), `tst_modules` (dont `toolsSeeNoRenderItem`), `tst_ui_drawing` Passed ; `premiers-pas` hors écran `CAPTURES OK` ; banc hors écran `content_check.ok = True` | S6, S37 | #tache-l3-t14 |
| L3-T15 | QML : cadre de sélection, poignées, barre d'objet, réglette de translucidité ; fixtures `rectangle-et-traits` et `objet-verrouille` ; scénario `selection` | L3-T9, L3-T14 | `ui/qml/SelectionOverlay.qml`, `ui/qml/ObjectBar.qml`, `ui/qml/TranslucencyPopup.qml`, `bench/src/fixtures.cpp`, `bench/scenarios/selection.json` | isolé | implementeur-tdd | `TESTS OK`, N ≥ 111, `tst_captures_selection` Passed ; `captures.ps1 -Scenario selection` hors écran : `LU appTools.selectionCount=2 OK`, `LU appTools.handlesVisible=true OK`, `CAPTURES OK` ; banc hors écran `content_check.ok = True` | S34 | #tache-l3-t15 |
| L3-T16 | QML : corbeille, menu contextuel de la page, curseurs par outil ; scénarios `corbeille` et `marqueur` | L3-T14, L3-T15 | `ui/qml/TrashCan.qml`, `ui/qml/PageContextMenu.qml`, `ui/qml/ToolCursor.qml`, `bench/scenarios/corbeille.json`, `bench/scenarios/marqueur.json` | isolé | implementeur-tdd | `TESTS OK`, N ≥ 111, `tst_captures_corbeille` et `tst_captures_marqueur` Passed ; `captures.ps1 -Scenario corbeille` puis `marqueur` hors écran `CAPTURES OK` (lectures `LU appDocument.shownObjectCount=…` d'abord) | S35, S37 | #tache-l3-t16 |
| L3-T19 | Banc : scénarios `gommer` et `soulever` (H10-8 e), outil choisi par commande avant la mesure | L3-T14 | `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp`, `bench/src/runner.cpp`, `tests/tst_bench_scenarios.cpp`, `tests/tst_bench_result.cpp`, `docs/banc.md` | isolé | implementeur-tdd | `TESTS OK`, N ≥ 111, `tst_bench_scenarios` (dont `eraseScenarioCrossesStrokes`, `liftScenarioDragsAnObject`) et `tst_bench_result` (dont `sevenScenariosWithFrames`) Passed ; banc hors écran : scénarios `gommer` et `soulever` présents, `cache.refreshes` relevé et dans l'attendu chiffré de S36 (`soulever` : 2 ; `gommer` : ≤ traits touchés + 1) | S36 | #tache-l3-t19 |
| L3-T18 | Scénario S6 `ecrire-gommer-annuler` (pincement compris), rejoué à l'écran en D3D11 (H10-1) | L3-T14, L3-T15, L3-T17 | `bench/scenarios/ecrire-gommer-annuler.json` | isolé | implementeur-tdd | `TESTS OK`, N ≥ 111, `tst_captures_ecrire_gommer_annuler` Passed ; hors écran `CAPTURES OK` (`LU appDocument.shownObjectCount=3 OK` après la gomme, `LU pageView.zoom≈1.5 OK` après le pincement) ; à l'écran D3D11 (jeton `ecran`) `CAPTURES OK`, `LU fixedPage.pendingCount=0 OK` et `LU pageView.cacheRefreshCount=<n> OK` après l'annulation du trait en attente | S6 | #tache-l3-t18 |

20 tâches, un seul plan (seuil de 20 de l'amendement, § 6). Tests ctest : L3-T1 enregistre d'un coup les 17 tests du
lot (13 unitaires, 4 de captures), talons sautés par `QSKIP` jusqu'à ce que leur tâche les remplisse : **111** dès
L3-T1 (94 + 17), et aucune autre tâche n'en enregistre. « ≥ » est la règle. Un talon sauté ne prouve rien : chaque
preuve nomme les fonctions de test attendues `PASS`.

Worktrees : `.worktrees/livraison-complete--L3-T<n>` (profil), tous isolés : deux tâches d'une même vague ne
partagent aucun fichier (vérifié sur la colonne « fichiers touchés »).

**Écran.** L3-T11 (preuve D3D11 du lissage conservé), L3-T18 (S6 à l'écran, H10-1) et la clôture ouvrent des
fenêtres à l'écran (D3D11) sous le jeton `ecran` : l'orchestrateur l'annonce à Jérôme d'une ligne **avant** de
lancer chacune. Aucune autre tâche n'ouvre de fenêtre visible.

## Vagues (largeur 3 au plus ; 2 places de tests pour ce lot)

V0 : L0-S (socle, plan à part) · V1 : L3-T0 ‖ L3-T1 · V2 : L3-T2 ‖ L3-T3 ‖ L3-T10 · V3 : L3-T4 ‖ L3-T5 ‖ L3-T6 ·
V4 : L3-T11 ‖ L3-T12 ‖ L3-T17 · V5 : L3-T7 ‖ L3-T8 ‖ L3-T13 · V6 : L3-T9 ‖ L3-T14 · V7 : L3-T15 ‖ L3-T19 ·
V8 : L3-T16 ‖ L3-T18. (8 vagues après le socle ; L3-T1, L3-T2, L3-T17 à amender d'après le socle, voir l'en-tête.)

**Places.** Le jeton `tests` a 3 places sur la machine, dont **2 pour ce lot** (la seconde session, lot 7, garde la
troisième). Une vague de 3 tâches qui construisent en a donc toujours une qui attend la notification du jeton : c'est
prévu, l'agent attend (règle commune), il ne sonde pas. L3-T0 ne construit rien.

**L3-T0 et les autres tâches.** Les tâches de données et de mécanique (L3-T1, L3-T2, L3-T3, L3-T10, L3-T17) ne
l'attendent pas. Les tâches de comportement (L3-T4, L3-T5, L3-T6, L3-T11, donc tout ce qui suit) l'attendent. Ses
décisions D1 à D12 (section de L3-T0) **amendent ce plan** avant la vague V3 (relecture du plan exigée), comme au
lot 10 ; une décision qui change une donnée de commande (raccourci, libellé, ordre de la barre d'objet) est
appliquée par un correctif `L3-C0` (`actions/data/commands.json`, `actions/data/layout.json` seulement), joué seul
avant V3.

Fichiers partagés tenus par les dépendances : `ui/include/actcraft/ui/documentcontroller.h`,
`ui/src/documentcontroller.cpp` (L3-T1 → L3-T12) ; `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`
(L3-T1 → L3-T13) ; `ui/qml/Main.qml` (L3-T2 → L3-T14) ; `ui/qml/PageArea.qml` (L3-T2 → L3-T11, dépendance déclarée) ;
`bench/src/fixtures.cpp` (L3-T15 → L3-T16) ;
`tests/tst_ui_drawing.cpp` (L3-T13 → L3-T14) ; `tools/src/selecttool.cpp`, `selectiongeometry.*`,
`tests/tst_tools_select.cpp` (L3-T8 → L3-T9) ; `ui/src/toolhost.cpp`, `ui/src/tooladapters.*` (talons L3-T1 →
L3-T14) ; les talons de L3-T1 → leur tâche ; `tests/tst_ui_shell.cpp` (L3-T1 si besoin → L3-T2).

## Dépendances aux autres lots

**Lot 10 (en cours).** Interface de cache inchangée (`core::PageCache`, `core::applyToCache`) ; le lot 3 en est un
appelant de plus, par `DocumentController` seulement. Comportements dont il dépend : un objet inséré **hors fin de
page** (dépôt d'un objet soulevé, morceaux de gomme) garde le redessin immédiat (limite de DA11) ; annuler un trait
encore en attente ne redessine rien (`undoingAPendingStrokeRebuildsNothing`, L10-T3) ; `cacheReady` couvre cache +
couche d'attente. La couche soulevée réutilise la fabrique `render::createObjectNode` (L10-T2) et se place
au-dessus de `pendingLayer` (L10-T4) dans `PageArea.qml`. Reports du lot 10 tenus ici : H10-1 (S6 à l'écran en
D3D11, annulation d'un trait en attente) en L3-T18 ; H10-4 (annuler un objet versé) mesuré par S36 ; H10-6 (forme
du trait) tranché par L3-T0, fait par L3-T11 ; H10-8 (e) (gomme) : scénario de banc `gommer` en L3-T19 ; P10
(regroupement automatique des annotations) et P17 (réglage du curseur du stylo) : réglages, lot 6 (H3-8). Si L10-T7
(tuiles) est jouée, rien ne change ici : seule l'implémentation du cache change.

**Lot 7 (seconde session, en parallèle sur `feat/livraison-complete-b`).** Fichiers chauds communs :
`actions/data/commands.json`, `actions/data/layout.json`, `ui/qml/Main.qml`, `ui/qml/PageArea.qml`,
`ui/qml/ToolPalette.qml`, `ui/CMakeLists.txt`, `tests/*.cmake`, `DocumentController`, `PageView`. Règle : le lot 3
pose ses entrées en **blocs contigus** (commandes du lot 3 groupées en fin de liste, groupes de palette nommés,
sous-menu « Effacer » dans Éditer) ; le lot qui merge le second se remet à jour sur l'autre et résout les conflits
par juxtaposition, sans réordonner. Contrats que le lot 7 doit suivre une fois le lot 3 mergé : tout contact de doigt
passe par `input::PointerRouter` (rideau, spot, outils de maths compris : ils deviennent des `tools::Tool` ou des
routes de la politique) ; les outils de maths ouverts appartiennent à `tools::UserState` (champ réservé,
DA-A1) ; « Convertir en texte » (F-202, reconnaissance) s'ajoute au menu contextuel `PageContextMenu.qml` ; la
reconnaissance (P14) remplace des traits par une commande de la pile. Si le lot 7 merge le premier et touche
`PageView` (entrées), L3-T13 l'absorbe (relecture du plan exigée).

**Lots 5 et 6.** Les boutons de la barre d'objet hors lot 3 (menu d'édition, regroupement, avancer, reculer,
dupliquer, points de forme : F-521) arrivent au lot 5 à leur place dans `objectBar` ; l'attribut « verrouillable »
(F-298) est posé par le lot 6 sur les commandes des lots 1 à 5 (amendement § 1) : aucune action au lot 3 ; les
profils (N2-6 amendée) sont livrés au lot 6 avec les outils déjà là : aucun profil à compléter au lot 3.

## Ce que l'ancien dépôt dit déjà (relevé pour ce plan, reformulé)

Lu dans `C:\Users\JeromeDenis\REA - Activinspire` ; L3-T0 le confirme sur la 3.5.18 et le corpus.

1. **Gomme** (`app/actcraft/canvas/tools.py`, coupe d'une ligne brisée par un disque) : la partie du **trait central**
   intérieure au disque est retirée ; les morceaux restants deviennent des objets distincts ; les bouts arrondis
   débordent dans le trou (relevé au pixel, plan comparatif ligne 9) ; entre deux positions éloignées, le passage est
   interpolé pour ne pas sauter un trait ; seules les annotations sont touchées, formes intactes.
2. **Sélection** (plan comparatif lignes 14-15) : huit poignées aux coins et milieux (gauche, haut, droite, bas) ;
   restrictions du fichier respectées (déplacer, redimensionner, pivoter) ; la barre d'objet pivote avec l'objet ;
   déplacer, supprimer et corbeille relevés identiques.
3. **Marqueur** (ligne 8) : trait à 50 % d'opacité, dessiné d'un seul tenant ; Rétablir grisé après une
   modification.
4. **Annuler** (ligne 16) : Annuler, Rétablir, Effacer les annotations puis Annuler, Rétablir la page : mêmes
   contenus et mêmes états grisés des boutons à chaque étape.
5. **Spécification** : F-210 (clic, rectangle qui touche, glisser déplace sauf objet verrouillé), F-225 (marqueur
   jaune, 20 px, translucidité 128 sur 255), F-230, F-520 (huit poignées, coin bas droit proportionnel), F-521
   (ordre de la barre), F-523 (pas de 10 %), F-528 (poignées masquées pendant un glissement), F-541 (Suppr sans
   sélection ne supprime pas la page), F-050 (corbeille : glisser dessus, flèche au survol, clic avec sélection,
   annulable), F-130, F-131, F-136 (mémoire par outil, y compris au redémarrage).

## Écarts au cadrage (repris à la porte de fin du lot 3)

1. **Volume.** 13 tâches estimées (amendement § 6) → 20 : deux tâches de réservation (consigne de parallélisme du
   2026-10-18), soulèvement (couche, commandes, contrôleur), routeur unique (DA-A1), forme du trait (H10-6), deux
   tâches de banc et de joueur de scénarios.
2. **S6, nombre d'Annuler.** S6 dit « trois Annuler rendent les 2 traits d'origine ». Avec une étape par action
   (P11 du lot 10 : un effacement s'annule en un pas ; la sélection n'est pas une étape), après deux traits, un
   passage de gomme et un glissement, **deux** Annuler rendent les deux traits ; un troisième retire le second trait.
   Le scénario joue deux Annuler, contrôle les deux traits, puis un troisième et contrôle qu'il reste un trait
   (H3-5). L3-T0 relève le décompte de la 3.5.18 ; s'il diffère (glissement en deux étapes, sélection annulable), il
   amende L3-T4 et L3-T18.
3. **Barre d'objet partielle.** Seuls les boutons des fonctions du lot (déplacer librement, rotation, translucidité,
   agrandir, réduire) ; les autres au lot 5, à leur rang (H3-10).
4. **F-202 en partie.** Menu contextuel « Stylo » / « Sélectionner » ; « Convertir en texte » au lot 7
   (reconnaissance).
5. **Mémoire par outil gardée par `QSettings`** dès le lot 3 (F-136 le demande au redémarrage) ; le lot 6
   (réglages, profils) pourra la reprendre dans son système (H3-7).
6. **Corbeille masquable** : la commande `view.trash` (Affichage) la montre ou la masque ; la persistance du réglage
   vient au lot 6.

## Scénarios ajoutés

- **S34 — Sélection (lot 3).** `captures.ps1 -Scenario selection` : un rectangle et deux traits posés par commandes
  et stylet ; outil Sélectionner ; un clic sur le rectangle le sélectionne (cadre, huit poignées, barre d'objet
  visibles : lectures `appTools.selectionCount`, `appTools.handlesVisible`, contrôle de pixels sur une poignée) ; un
  clic dans le vide désélectionne ; un rectangle de sélection qui touche les deux traits les sélectionne (2) ; glisser
  les déplace (contrôle de pixels avant / après) ; la poignée du coin bas droit agrandit en gardant les proportions ;
  `object.enlarge` agrandit de 10 % ; un objet verrouillé (fixture) ne bouge pas au glisser.
- **S35 — Effacer, supprimer, corbeille (lot 3).** Effacer ▸ Annotations retire les traits et garde la forme ;
  Annuler les rend ; Suppr sans sélection ne change rien (nombre d'objets et de pages relus) ; sélection puis clic
  sur la corbeille supprime ; Annuler rend l'objet ; Rétablir la page d'un paperboard jamais enregistré vide la page ;
  après `file.save-as`, une modification puis Rétablir la page rend l'état enregistré (empreinte relue par
  `Remember`).
- **S36 — Banc des gestes (lot 3).** `bench.ps1` : scénarios `gommer` (la gomme traverse des traits versés de la page
  générée) et `soulever` (un objet glissé sur la page générée) mesurés comme les autres, `cache.refreshes` et
  figements relevés ; résultat, pas porte (DA7). **Verdict (M3)** : `gommer` et `soulever` sont **exclus** du
  critère « 0 figement, 0 reprise hors limite » de S13, qui reste jugé sur les cinq scénarios du lot 10 ; ils ont un
  **attendu chiffré** à la place : `soulever` → `cache.refreshes = 2` exactement (soulèvement, dépôt ; regroupement
  par tour de L3-T13), figements hors limite ≤ 2, aucun pendant le mouvement ; `gommer` → `cache.refreshes ≤ k + 1`,
  où k = nombre de traits distincts touchés (relevé par le scénario, écrit dans le JSON), figements hors limite
  ≤ k + 1. Au-delà de l'attendu : défaut du lot 3 (tâche corrective). Dans l'attendu mais figements jugés gênants en
  D3D11 à la clôture : H10-8 (e) décide (tuiles, cache par groupe de couches), présenté à la porte de fin.
- **S37 — Marqueur et mémoire par outil (lot 3).** `captures.ps1 -Scenario marqueur` : marqueur jaune de 20 par
  défaut ; un trait de marqueur qui se recoupe a une teinte uniforme à son croisement ; deux traits superposés
  foncent ; stylo rouge 8, puis marqueur, puis stylo : rouge 8 rendu ; un second lancement avec le même fichier de
  réglages garde le rouge 8 (`tst_ui_tools`, `settingsSurviveRestart`).

## Couverture des scénarios

| Scénario | Tâches | Joué à la clôture |
|---|---|---|
| S6 — écrire, gommer, annuler, pincer | L3-T1, T3, T4, T5, T6, T7, T8, T10, T12, T13, T14, T17, T18 | oui : hors écran et à l'écran en D3D11 |
| S30 — tâche d'ouverture | L3-T0 | contrôlé à la relecture : rapport, relevé brut, `NOMS OK` |
| S34 — sélection | L3-T1, T2, T8, T9, T15 | oui |
| S35 — effacer, supprimer, corbeille | L3-T1, T2, T4, T12, T16 | oui |
| S36 — banc des gestes | L3-T19 | oui : banc complet D3D11 plein écran |
| S37 — marqueur et mémoire par outil | L3-T6, T11, T14, T16 | oui |
| S13 (lot 10) — non-régression du cache | toutes les tâches de `render/ ui/` | oui : scénarios `pauses`, `cache-differe` rejoués |

## Carte des fichiers

| Fichier | Responsabilité | Talon / API (L3-T1) | Rempli par |
|---|---|---|---|
| `docs/verifications/2026-10-18-lot3-principes.md` | constats du corpus, de l'ancien dépôt et du binaire, en nos mots ; décisions D1 à D12 | — | L3-T0 |
| `actions/data/commands.json`, `actions/data/layout.json` | 19 commandes du lot ; menus Éditer (sous-menu « Effacer »), Affichage, Outils ; palette (`outils`, `edition`) ; `objectBar` | L3-T1 | L3-T1 |
| `actions/include/actcraft/actions/layout.h`, `actions/src/layout.cpp` | sous-menus (`MenuSpec::submenus`, jeton `>id`), `Layout::objectBar` | L3-T1 | L3-T1 |
| `ui/include/actcraft/ui/commands.h`, `ui/src/commands.cpp`, `ui/qml/CommandMenu.qml` | menus QML avec sous-menus ; propriété `objectBar` | L3-T1 | L3-T1 |
| `ui/icons/*.svg` (13) | icônes du lot, dessinées par nous (grille de 24 px) | L3-T1 | L3-T1 |
| `core/include/actcraft/core/hittest.h`, `core/src/hittest.cpp` | `isAnnotation`, `objectAt`, `objectsTouching`, rectangles | talon | L3-T3 |
| `core/include/actcraft/core/erasegeometry.h`, `core/src/erasegeometry.cpp` | `cutByDisk`, `cutBySweep`, `strokePointsOnPage` | talon | L3-T3 |
| `core/include/actcraft/core/objectedits.h`, `core/src/objectedits.cpp` | `TransformObjectsCommand`, `RemoveObjectsCommand`, `ReplaceStrokesCommand`, `ClearPageCommand`, `ReplacePageCommand`, `SetTranslucencyCommand`, pivots | talon | L3-T4 |
| `core/include/actcraft/core/lift.h`, `core/src/lift.cpp` | `takeObjects`, `restoreObjects`, `liftChanges`, `changesAfterLift` | talon | L3-T4 |
| `input/include/actcraft/input/routing.h`, `input/src/routing.cpp` | `RoutingPolicy`, `FullTouchPolicy`, `PointerRouter`, `Decision` | talon | L3-T5 |
| `input/include/actcraft/input/gesture.h`, `input/src/gesture.cpp` | `TwoFingerGesture`, `GestureStep` | talon | L3-T5 |
| `input/include/actcraft/input/sample.h` | champs `pointer`, `user` ajoutés à la fin | — | L3-T5 |
| `tools/include/actcraft/tools/{tool,toolcontext,userstate,toolregistry}.h`, `tools/src/{userstate,toolregistry}.cpp` | interface d'outil, contexte, utilisateur, registre | talon | L3-T6 |
| `tools/include/actcraft/tools/pentool.h`, `tools/src/pentool.cpp` | stylo et marqueur (`Tool`, `StrokeKind`, translucidité, bouts ronds) | — | L3-T6 |
| `tools/include/actcraft/tools/erasertool.h`, `tools/src/erasertool.cpp` | gomme | talon | L3-T7 |
| `tools/include/actcraft/tools/{selecttool,selectiongeometry}.h`, `tools/src/{selecttool,selectiongeometry}.cpp` | sélection, cadre, poignées, barre | talon | L3-T8, L3-T9 |
| `render/include/actcraft/render/liftlayeritem.h`, `render/src/liftlayeritem.cpp` | couche soulevée (type QML `LiftLayer`) | talon | L3-T10 |
| `render/src/{strokegeometry,inkitem,strokerenderer,pagepainter}.cpp` | bouts ronds, courbe lissée, marqueur d'un seul tenant | — | L3-T11 |
| `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp` | soulever, déposer, `editAboutToApply`, effacer, rétablir la page, observateur de cache | API (corps vides) | L3-T12 |
| `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp` | routeur, gestes, `tools`, `cacheRefreshCount` | API (corps vides) | L3-T13 |
| `ui/include/actcraft/ui/toolhost.h`, `ui/src/toolhost.cpp`, `ui/src/tooladapters.h`, `ui/src/tooladapters.cpp` | `ui::ToolHost` (type QML `Tools`) et adaptateurs des interfaces d'outil | API (corps vides) | L3-T14 |
| `ui/qml/Main.qml`, `ui/qml/PageArea.qml`, `ui/qml/ToolPalette.qml` | coquille : `appTools`, couches, corbeille, gestionnaires | — | L3-T2 (puis L3-T11, L3-T14) |
| `ui/qml/SelectionOverlay.qml`, `ObjectBar.qml`, `TranslucencyPopup.qml` | cadre, poignées, barre, réglette | talon `Item {}` | L3-T15 |
| `ui/qml/TrashCan.qml`, `PageContextMenu.qml`, `ToolCursor.qml` | corbeille, menu contextuel, curseurs | talon `Item {}` | L3-T16 |
| `bench/src/uiscenario.cpp`, `bench/src/injector.cpp` | étapes `pinch`, `touch`, `expect` + `tolerance` | — | L3-T17 |
| `bench/src/scenarios.cpp`, `bench/src/runner.cpp` | scénarios de banc `gommer`, `soulever` | — | L3-T19 |
| `bench/scenarios/{selection,corbeille,marqueur,ecrire-gommer-annuler}.json` | S34, S35, S37, S6 | talon (`waitReady` seul) | L3-T15, L3-T16, L3-T18 |
| `tests/tst_*.cpp` (13 nouveaux) | un par module neuf | talon `QSKIP` | la tâche du module |

## Contrats partagés entre tâches (noms figés)

Tous les noms des lots 1, 2 et 10 restent figés ; on ajoute, on ne renomme pas. Les en-têtes publics du lot sont
**créés tels quels par L3-T1** (sa section, étape 4, donne leur texte complet) ; une tâche suivante les lit dans son
worktree, ne change pas une signature publique sans amendement du plan, et ajoute librement des membres **privés**
dans l'en-tête de son propre module.

- `core::PageCache`, `core::applyToCache`, `render::FixedPageItem`, `render::PendingLayerItem`, `ui::FixedPageSink`
  : inchangés. Seul `DocumentController` appelle le cache ; aucun fichier de `tools/` n'inclut un en-tête de
  `render/` (contrôle `toolsSeeNoRenderItem`, L3-T14).
- Commandes (19) : `tool.select` (Échap), `tool.highlighter` (Ctrl+H), `tool.eraser` (Ctrl+E), `edit.undo` (Ctrl+Z),
  `edit.redo` (Ctrl+Y, D11), `edit.delete` (Suppr), `edit.select-all` (Ctrl+A), `clear.annotations`,
  `clear.objects`, `clear.grid`, `clear.background`, `clear.page`, `page.revert`, `view.trash` (cochable),
  `object.move-free`, `object.rotate`, `object.translucency` (nombre, facultatif), `object.enlarge` (Ctrl++),
  `object.reduce` (Ctrl+-). `object.move-free` et `object.rotate` se saisissent (glisser), ils n'ont pas de
  gestionnaire.
- Identifiants d'outil : `select`, `pen`, `highlighter`, `eraser` ; la commande `tool.<id>` cochée = outil courant.
- `core::ClearKind` : `Annotations = 0`, `Objects = 1`, `Grid = 2`, `Background = 3`, `Page = 4` (argument de
  `DocumentController::clearPage(int)`).
- Objets QML lus par les scénarios : `appTools` (`selectionCount`, `handlesVisible`, `currentTool`, `color`,
  `width`), `appDocument` (`shownObjectCount`, `canUndo`, `canRedo`, `fingerprint`), `pageView` (`zoom`,
  `cacheRefreshCount`, `gestureActive`), `fixedPage` (`pendingCount`), `liftLayer` (`objectCount`), `trashCan`,
  `selectionOverlay`, `pageContextMenu`, `toolCursor`.
- `ToolHost::selectionFrame` : `{visible: bool, corners: [{x, y}] ×4, handles: [{x, y}] ×8, bar: [{command, icon,
  x, y}], rubberBand: {x, y, width, height} | null, handlesVisible: bool}`, **en coordonnées de page** ; le QML les
  place par `pageView.pageToScene`.
- Constantes : `core::kHitToleranceScreen = 6.0` ; `tools::kHandleRadiusScreen = 6.0`, `kBarButtonScreen = 24.0`,
  `kBarGapScreen = 8.0`, `kEnlargeStep = 1.1` ; `input::kPinchDeadZone = 0.03` ; réglages par défaut :
  `tools::defaultSettings` (D5).

## Règles communes à toutes les tâches (jointes à chaque section par l'orchestrateur)

Celles des plans 2a, 2b et 10 s'appliquent toutes, mot pour mot (`docs/plans/2026-10-15-livraison-complete-lot2a-PLAN.md`,
`…-lot2b-PLAN.md`, `…-lot10-PLAN.md`, « Règles communes ») : processus enfant pour toute commande PowerShell,
`git -C`, mutation remise par `git checkout --`, `format.ps1` avant la preuve, pièges de Clang (`-Wshadow`,
conversions explicites, pas d'agrégat partiel par position), pas de `std::visit` direct sur `PageObject`, **aucune
API privée de Qt** dans le code livré, propriété intellectuelle, `samples/` en lecture seule, commits explicites en
français terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`, arrêt par PID, fils de Qt Quick
(tout état lu par `updatePaintNode` est écrit sur le fil principal seulement). S'y ajoutent, pour le lot 3 :

- **Graphe d'abord** : pour localiser du code, `graphify query "…"` dans le worktree avant de lire les fichiers ;
  pour l'application de référence, `graphify query "…"` lancé **dans** `C:\Users\JeromeDenis\REA - Activinspire`.
- **Machine partagée : toute construction, toute suite, tout banc et toute capture passent par le jeton**
  `C:\Users\JeromeDenis\.claude\scripts\jeton.ps1` : `-Nom tests` pour construire, tester, jouer le banc ou les
  captures hors écran ; `-Nom ecran` pour toute fenêtre visible. Une commande par prise ; aucun démon laissé
  vivant ; si le jeton est long à venir (2 places pour ce lot, la troisième à la session du lot 7), attendre sa
  notification, ne pas le sonder en boucle.
- `<wt>` = `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L3-T<n>` ; `<id>` = `L3-T<n>`.
- **Commande ciblée** (rouge, vert d'un seul test) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande '. ./scripts/dev-env.ps1; cmake --build --preset debug *> .tmp/build.txt; ctest --preset debug -R ''<motif>'' --output-on-failure *> .tmp/ctest.txt'; Select-String -Path .tmp/build.txt,.tmp/ctest.txt -Pattern 'error|tests passed|tests failed|No tests were found' | Select-Object -Last 4"
  ```
  Premier passage dans un worktree neuf : la même commande avec `-Commande 'pwsh -NoProfile -File ./scripts/build.ps1 *> .tmp/build.txt'`.
- **Fonctions d'un test** (les `tst_*.exe` n'écrivent rien sur la sortie standard ici ; toujours après `dev-env.ps1`) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; . ./scripts/dev-env.ps1; & build/debug/tests/<test>.exe -o .tmp/<test>.txt,txt; Select-String -Path .tmp/<test>.txt -Pattern '^PASS   :|^FAIL!  :|^SKIP   :|^Totals' | Select-Object -Last 40"
  ```
  (chemin de l'exécutable : celui que `ctest --preset debug -N -V -R <test>` imprime, s'il diffère.) Attendu pour
  un test dont le talon était sauté : chaque fonction nommée par la table en `PASS`, aucun `SKIP`,
  `Totals: <k> passed, 0 failed, 0 skipped`, k ≥ 1.
- **Preuve de tâche** (forme bornée du profil, sous le jeton), puis le nom du test :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1'; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2; Select-String -Path .tmp/test.txt -Pattern '<nom_du_test> .*Passed' | Select-Object -Last 1"
  ```
  Attendu : `100% tests passed, 0 tests failed out of N` (N ≥ 111 dès L3-T1), `TESTS OK`, puis la ligne `Passed`
  du test nommé.
- **Banc sans écran** (toute tâche qui touche `render/`, `input/`, `tools/` ou `ui/` ; résultat, pas une porte) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu logiciel -Scenario court -Taille 1280x800 -HorsEcran > .tmp/bench.txt 2>&1'; Select-String -Path .tmp/bench.txt -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE' | Select-Object -Last 1"
  pwsh -NoProfile -Command "Set-Location '<wt>'; `$j = Get-ChildItem bench/results/*.json | Sort-Object LastWriteTime | Select-Object -Last 1; `$o = Get-Content -Raw `$j | ConvertFrom-Json; Write-Output (`$j.Name + ' content_check.ok = ' + `$o.content_check.ok); `$o.scenarios | ForEach-Object { Write-Output (`$_.name + ' freeze>lim=' + `$_.frames.freeze_over_limit + ' resume>lim=' + `$_.frames.resume_over_limit + ' cache=' + (`$_.cache | ConvertTo-Json -Compress)) }"
  ```
  Attendu : `BUDGET TENU` ou `BUDGET DEPASSE` (jamais `MESURE INVALIDE`), `content_check.ok = True` ; nom du JSON
  joint au rapport. Le rendu logiciel hors écran ne juge pas la fluidité.
- **Preuve visuelle hors écran** (jeton `tests`) : lectures `-Lire "<objet>.<propriété>=<attendu>"` d'abord (une
  valeur lue prouve plus qu'une image), puis description des captures :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario <nom> -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire ''<objet.propriété=attendu>'' > .tmp/captures.txt 2>&1'; Select-String -Path .tmp/captures.txt -Pattern '^(COMMANDE|TRAIT|TOUCHE|DOIGT|PINCEMENT|ATTENTE|PRET|FENETRE|LU|PIXEL|BORD|CAPTURE) |^SCENARIO |CAPTURES (OK|EN ECHEC)' | Select-Object -Last 16"
  ```
  Motifs d'échec : `CAPTURES EN ECHEC`, `SCENARIO EN ECHEC`, `SCENARIO INVALIDE`, toute ligne `ECART` ou `ECHEC`,
  `FENETRE …`. Les lignes `DOIGT` et `PINCEMENT` existent après L3-T17.
- **Dépôt public** : `python -I "C:/Users/JeromeDenis/REA - Activinspire/tools/controle_noms_actcraft.py" <wt>` →
  `NOMS OK` avant chaque commit qui ajoute du texte (libellés, commentaires, docs). Libellés à nous (N1-12), aucun
  nom de l'original, aucune icône d'origine.
- **moc** ne reconnaît pas les chaînes brutes `R"(...)"` qui contiennent une apostrophe (piège L2-T10) : dans un
  test, pas d'apostrophe dans une chaîne brute (écrire `’` ou une chaîne ordinaire).
- **Talons** : remplir un talon, c'est **remplacer** son corps (déclarations publiques de L3-T1 gardées) ; la
  fonction `reserved()` du test talon est supprimée.

---

## Tâches

<a id="tache-l3-t0"></a>
### Tâche L3-T0 — Ouverture : vidéos, REA et ancien dépôt confrontés au lot

**Objectif.** Décision 0100 et règle `Lx-T0` (amendement § 4) : relever dans le corpus tous les principes des
fonctions du lot (sélection, poignées, barre d'objet, stylo, marqueur, gomme, annulation, effacer, rétablir la page,
corbeille, curseurs, menu contextuel, raccourcis, gestes, mémoire par outil), plus le constat « Stylo, formes,
couleurs » du tableau du § 4 affecté au lot 3 et les constats P4, P5, P9, P10, P11, P12, P17 du lot 10
(`docs/verifications/2026-10-18-lot10-principes.md`) ; trancher sur la 3.5.18 par REA les points ambigus ; écrire
en nos mots les exigences et les **décisions D1 à D12**, qui amendent ce plan.

**Décisions à rendre** (chacune : valeur retenue et source, ou « non tranché » → hypothèse de niveau 1 du lot) :
- D1 — **Schéma d'entrée du second utilisateur** (DA-A1) : quels périphériques forment le second utilisateur dans
  l'original. Non tranché → N1 du lot 3 ; le lot 3 n'a de toute façon qu'un utilisateur.
- D2 — Forme du trait : bouts et jonctions ronds, courbe lissée par les milieux (H10-6, P4) ; vaut-elle pour les
  traits lus d'un `.flipchart` ?
- D3 — Marqueur : translucidité par défaut ; un trait ne fonce pas sur lui-même, deux passages foncent (P5).
- D4 — Gomme : épaisseur par défaut ; coupe sur la ligne centrale ; annotations déplacées hors de la couche du
  dessus intactes (F-230).
- D5 — Réglages par défaut par outil (stylo noir 4, marqueur jaune 20 à 128 sur 255, gomme 20 supposée) et mémoire
  au redémarrage (F-136).
- D6 — Sélection : rectangle qui « touche » = emprise ou encre (H3-4) ; tolérance de désignation ; ajout à la
  sélection (Ctrl+clic ou Maj+clic).
- D7 — Barre d'objet : ordre (F-521), position (au-dessus, replacée), boutons d'une sélection multiple ; rotation
  par la barre seulement, ou aussi par une poignée du cadre.
- D8 — Effacer ▸ Objets : quelles couches et quels types (F-130) ; « Fond » : fond seul ou aussi les objets de la
  couche de fond.
- D9 — Rétablir la page : une étape d'annulation ou non ; état de référence d'un `.flipchart` ouvert.
- D10 — Décompte des étapes d'annulation : un glissement, un passage de gomme, une translucidité réglée (écart 2).
- D11 — Raccourci de Rétablir et raccourcis du lot (F-951 à F-953). Ctrl+Maj+Z est **pris** par le lot 7
  (`view.page-zoom`) : si la 3.5.18 utilise Ctrl+Maj+Z pour Rétablir, le conflit est rendu à l'orchestrateur (le
  socle tranche), Ctrl+Y reste le défaut.
- D12 — Gestes : seuil du pincement, défilement à deux doigts, trait de doigt en cours à l'arrivée du deuxième
  doigt ; et le constat « Stylo, formes, couleurs » du § 4 de l'amendement (à nommer).

**Sources.**
- Corpus : `C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\README.md` (« Principes absents »,
  « Écarts avec la spec ») et les fiches `*.md`.
- Ancien dépôt : `app/actcraft/canvas/tools.py` (sélection, stylo, marqueur, gomme), `canvas/selection.py`
  (poignées), `canvas/items.py` (encre), `docs/tests/plan-comparatif.md` (lignes 7 à 9 et 14 à 16),
  `docs/re/interface.md` ; son graphe (`graphify query "…"` lancé dans `C:\Users\JeromeDenis\REA - Activinspire`).
- Spécification : `C:\Users\JeromeDenis\REA - Activinspire\docs\spec\fonctionnalites.md` (F-050, F-130 à F-137,
  F-201, F-202, F-210 à F-230, F-520 à F-528, F-541, F-930, F-951 à F-953).
- Binaire : skill `reverse-engineer-anything` sur `C:\Program Files\Activ Software\Inspire\`, **en statique
  seulement** (ressources, chaînes, réglages par défaut, raccourcis). Aucune observation à l'écran.

**Fichiers.**
- Créer : `docs/verifications/2026-10-18-lot3-principes.md` (dans le dépôt, nos mots).
- Créer hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot3.md` (relevé brut).

**Test et commande bornée.** Pas de code. Preuves :
```powershell
Test-Path 'C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot3.md'   # True
python -I "C:/Users/JeromeDenis/REA - Activinspire/tools/controle_noms_actcraft.py" "C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L3-T0" | Select-Object -Last 1   # NOMS OK : <n> fichiers
pwsh -NoProfile -Command "`$exclus = @('Boutons définis par l''utilisateur', 'Nouveau paperboard'); Get-ChildItem 'C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\*.md' -Exclude README.md | ForEach-Object { `$t = (Get-Content -LiteralPath `$_.FullName -TotalCount 5 -Encoding utf8 | Where-Object { `$_ -like '# *' } | Select-Object -First 1); if (`$t) { `$t = `$t.Substring(2).Trim(); if (`$exclus -notcontains `$t) { git -C 'C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L3-T0' grep -l -i -F -- `$t } } }"   # aucune sortie
```

**Interdits.** Aucun titre, aucune transcription, aucune citation de vidéo dans le dépôt (identifiants `Vnn`
seulement). Aucun nom de l'original, aucun texte de l'aide, aucune capture. Aucune ligne de code. Ne modifier ni
`samples/` ni le corpus. Pas d'observation de l'original à l'écran.

**Terminé quand.** Le rapport a une ligne par constat (principe → exigence en nos mots → tâche du lot 3 → scénario,
ou raison et lot destinataire) et une section « Décisions » D1 à D12 ; le relevé brut existe et est cité ;
`NOMS OK` ; la commande des titres ne sort rien ; le rapport de tâche liste pour l'orchestrateur les décisions qui
amendent le plan (tâche visée) et celles qui changent une donnée de commande (`L3-C0`).

**Étapes.**
- [ ] **Étape 1 : relever le corpus.**
  ```powershell
  Select-String -Path 'C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\*.md' -Pattern 'sélection|sélectionn|poignée|redimension|rotation|pivot|translucid|marqueur|surligneur|stylo|gomme|effac|annul|rétabl|corbeille|supprim|curseur|clic droit|menu contextuel|doigt|tactile|pincer|zoom|verrouill' -List | Select-Object -ExpandProperty Path
  ```
  Pour chaque fiche retenue : numéro `Vnn` (colonne « # » du `README.md`) et principe observé, en une phrase à nous.
- [ ] **Étape 2 : relever l'ancien dépôt.** Dans `C:\Users\JeromeDenis\REA - Activinspire` :
  `graphify query "outil de sélection poignées rotation"`, `graphify query "gomme coupe des traits"`,
  `graphify query "marqueur translucidité"`, `graphify query "annuler rétablir effacer la page"` ; lire les
  fonctions trouvées ; confirmer ou corriger le § « Ce que l'ancien dépôt dit déjà » de ce plan.
- [ ] **Étape 3 : REA statique.** Réglages par défaut (stylo, marqueur, gomme, translucidité du marqueur, rectangle
  de sélection, tactile), raccourcis, ordre par défaut de la barre d'objet, entrées du menu Effacer, mode à deux
  utilisateurs (périphériques). Constats bruts, noms de l'original compris, dans le relevé hors dépôt seulement.
- [ ] **Étape 4 : écrire le rapport.**
  ```markdown
  # Lot 3 — principes confrontés (vidéos, ancien dépôt, binaire)

  Relevé brut (hors dépôt) : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot3.md`. Références `Vnn` :
  numéros des fiches du corpus (table hors dépôt). Constats écrits par nous. Tâche L3-T0, scénario S30.

  ## Constats

  | # | Principe (nos mots) | Source | Exigence pour ActCraft | Tâche | Scénario | Écart, raison ou lot destinataire |
  |---|---|---|---|---|---|---|
  | P1 | La gomme coupe la ligne centrale des annotations et scinde le trait en objets distincts | ancien dépôt (gomme), Vnn… | morceaux sélectionnables séparément, formes intactes | L3-T3, L3-T7 | S6 | — |
  | … | | | | | | |

  ## Décisions

  | # | Question | Décision | Source | Tâche amendée |
  |---|---|---|---|---|
  | D1 | Schéma d'entrée du second utilisateur | … | … | — (lot 11) |
  | … | | | | |
  ```
  Une ligne par principe du corpus lié aux fonctions du lot ; D1 à D12 toutes remplies.
- [ ] **Étape 5 : contrôles.** Les trois commandes de « Test et commande bornée » ; corriger jusqu'à `NOMS OK` et
  sortie vide.
- [ ] **Étape 6 : commit.**
  ```powershell
  git -C <wt> add docs/verifications/2026-10-18-lot3-principes.md
  git -C <wt> commit -m "Lot 3 : principes des outils confrontés aux vidéos, à l'ancien dépôt et au binaire (L3-T0)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t1"></a>
### Tâche L3-T1 — Réservation I : commandes, disposition, icônes, fichiers et tests du lot, API figées

**Objectif.** Poser d'un coup tout ce que les tâches du lot écriraient sinon en même temps dans les fichiers chauds :
les 19 commandes et leur place dans les menus, la palette et la barre d'objet ; le sous-menu « Effacer » (le format
de disposition n'a pas de sous-menu) ; les 13 icônes ; l'enregistrement CMake de tous les fichiers neufs du lot ;
des talons compilables pour chacun ; les 17 tests du lot, sautés ; les API publiques figées (en-têtes complets,
corps vides). Aucun comportement nouveau, hors sous-menus et `objectBar`.

**Fichiers.**
- Modifier : `actions/data/commands.json`, `actions/data/layout.json`, `actions/include/actcraft/actions/layout.h`,
  `actions/src/layout.cpp`, `tests/tst_actions_layout.cpp`, `ui/include/actcraft/ui/commands.h`,
  `ui/src/commands.cpp`, `tests/tst_ui_commands.cpp`, `ui/qml/CommandMenu.qml`, `core/CMakeLists.txt`,
  `input/CMakeLists.txt`, `tools/CMakeLists.txt`, `render/CMakeLists.txt`, `ui/CMakeLists.txt`, `tests/core.cmake`,
  `tests/input.cmake`, `tests/tools.cmake`, `tests/render.cmake`, `tests/ui.cmake`, `tests/bench.cmake`,
  `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`, `ui/include/actcraft/ui/pageview.h`,
  `ui/src/pageview.cpp`.
- Créer (icônes) : `ui/icons/selection.svg`, `marqueur.svg`, `gomme.svg`, `annuler.svg`, `retablir.svg`,
  `supprimer.svg`, `corbeille.svg`, `effacer.svg`, `deplacer.svg`, `rotation.svg`, `translucidite.svg`,
  `agrandir.svg`, `reduire.svg`.
- Créer (talons) : `core/include/actcraft/core/hittest.h`, `core/src/hittest.cpp`,
  `core/include/actcraft/core/erasegeometry.h`, `core/src/erasegeometry.cpp`, `core/include/actcraft/core/objectedits.h`,
  `core/src/objectedits.cpp`, `core/include/actcraft/core/lift.h`, `core/src/lift.cpp`,
  `input/include/actcraft/input/routing.h`, `input/src/routing.cpp`, `input/include/actcraft/input/gesture.h`,
  `input/src/gesture.cpp`, `tools/include/actcraft/tools/tool.h`, `tools/include/actcraft/tools/toolcontext.h`,
  `tools/include/actcraft/tools/userstate.h`, `tools/src/userstate.cpp`, `tools/include/actcraft/tools/toolregistry.h`,
  `tools/src/toolregistry.cpp`, `tools/include/actcraft/tools/erasertool.h`, `tools/src/erasertool.cpp`,
  `tools/include/actcraft/tools/selecttool.h`, `tools/src/selecttool.cpp`, `tools/include/actcraft/tools/selectiongeometry.h`,
  `tools/src/selectiongeometry.cpp`, `render/include/actcraft/render/liftlayeritem.h`, `render/src/liftlayeritem.cpp`,
  `ui/include/actcraft/ui/toolhost.h`, `ui/src/toolhost.cpp`, `ui/src/tooladapters.h`, `ui/src/tooladapters.cpp`,
  `ui/qml/SelectionOverlay.qml`, `ui/qml/ObjectBar.qml`, `ui/qml/TranslucencyPopup.qml`, `ui/qml/TrashCan.qml`,
  `ui/qml/PageContextMenu.qml`, `ui/qml/ToolCursor.qml`.
- Créer (tests talons) : `tests/tst_core_hittest.cpp`, `tst_core_erase.cpp`, `tst_core_objectedits.cpp`,
  `tst_core_lift.cpp`, `tst_input_routing.cpp`, `tst_input_gesture.cpp`, `tst_tools_registry.cpp`,
  `tst_tools_eraser.cpp`, `tst_tools_select.cpp`, `tst_render_liftlayer.cpp`, `tst_render_strokeshape.cpp`,
  `tst_ui_lift.cpp`, `tst_ui_tools.cpp` ; scénarios `bench/scenarios/selection.json`, `corbeille.json`,
  `marqueur.json`, `ecrire-gommer-annuler.json`.

**Test et commande bornée.** Commande ciblée `tst_actions_layout|tst_ui_commands|tst_ui_shell`, puis preuve de
tâche : `100% tests passed … out of N`, **N ≥ 111**, `TESTS OK` ; fonctions `submenuItems`, `objectBarRead`,
`lot3EntriesDeclared` (`tst_actions_layout`) et `submenuInMenus` (`tst_ui_commands`) en `PASS` ; les 17 tests neufs
`Passed` (talons sautés). Preuve visuelle : `premiers-pas` hors écran `CAPTURES OK` (aucune régression de la
coquille). Banc sans écran (touche `ui/`) : `content_check.ok = True`.

**Interdits.** Aucun comportement d'outil (les talons ne font rien) ; ne renommer aucune commande ni aucun objet QML
existant ; aucune icône copiée (dessin à nous, mêmes attributs que `ui/icons/stylo.svg`) ; ne pas toucher
`ui/qml/Main.qml`, `PageArea.qml`, `ToolPalette.qml` (L3-T2).

**Terminé quand.** Les 19 commandes sont déclarées et placées ; le sous-menu « Effacer » s'affiche dans Éditer
(`submenuInMenus`) ; tous les fichiers du lot existent, compilent et sont enregistrés ; les en-têtes publics sont
ceux de l'étape 4, mot pour mot ; N ≥ 111, `TESTS OK` ; `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge de la disposition.** Ajouter à `tests/tst_actions_layout.cpp` :
  ```cpp
  void submenuItems()
  {
      const QByteArray json = R"({"format": "actcraft-layout/1", "menus": [{"id": "edit", "label": "Éditer",
          "items": ["edit.undo", {"submenu": "clear", "label": "Effacer", "items": ["clear.page"]}]}],
          "palette": [], "colors": [], "penWidths": []})";
      actions::Layout layout;
      QString error;
      QVERIFY2(actions::parseLayout(json, layout, error), qPrintable(error));
      QCOMPARE(layout.menus.size(), std::size_t{1});
      QCOMPARE(layout.menus[0].items, (QStringList{QStringLiteral("edit.undo"), QStringLiteral(">clear")}));
      QCOMPARE(layout.menus[0].submenus.size(), std::size_t{1});
      QCOMPARE(layout.menus[0].submenus[0].id, QStringLiteral("clear"));
      QCOMPARE(layout.menus[0].submenus[0].label, QStringLiteral("Effacer"));
      QCOMPARE(layout.menus[0].submenus[0].items, QStringList{QStringLiteral("clear.page")});
  }
  void objectBarRead()
  {
      const QByteArray json = R"({"format": "actcraft-layout/1", "menus": [], "palette": [], "colors": [],
          "penWidths": [], "objectBar": ["object.enlarge", "object.reduce"]})";
      actions::Layout layout;
      QString error;
      QVERIFY2(actions::parseLayout(json, layout, error), qPrintable(error));
      QCOMPARE(layout.objectBar, (QStringList{QStringLiteral("object.enlarge"), QStringLiteral("object.reduce")}));
  }
  void lot3EntriesDeclared()
  {
      actions::CommandRegistry registry;
      QString error;
      QVERIFY2(registry.loadDeclarations(actions::defaultCommandsJson(), error), qPrintable(error));
      const QStringList lot3 = {"tool.select", "tool.highlighter", "tool.eraser", "edit.undo", "edit.redo",
                                "edit.delete", "edit.select-all", "clear.annotations", "clear.objects",
                                "clear.grid", "clear.background", "clear.page", "page.revert", "view.trash",
                                "object.move-free", "object.rotate", "object.translucency", "object.enlarge",
                                "object.reduce"};
      for (const QString& id : lot3) {
          QVERIFY2(registry.find(id) != nullptr, qPrintable(id));
      }
      actions::Layout layout;
      const QByteArray json = actions::readLayeredFile(QStringLiteral("layout.json"), QString(),
                                                       actions::dataResourceDir());
      QVERIFY2(actions::parseLayout(json, layout, error), qPrintable(error));
      QVERIFY(actions::unknownCommands(layout, registry).isEmpty());
      QCOMPARE(layout.objectBar, (QStringList{"object.move-free", "object.rotate", "object.translucency",
                                              "object.enlarge", "object.reduce"}));
  }
  ```
  (Si `tst_actions_layout.cpp` lit déjà la disposition par défaut autrement, reprendre son moyen ; les assertions
  restent.) Commande ciblée `tst_actions_layout` → rouge (`submenus`, `objectBar` inconnus : erreur de compilation).
- [ ] **Étape 2 : sous-menus et barre d'objet dans `actions/`.** Dans `layout.h` :
  ```cpp
  struct MenuSpec
  {
      QString id;
      QString label;
      QStringList items;              // identifiants, « - » (séparateur) ou « >id » (sous-menu `id` de `submenus`)
      std::vector<MenuSpec> submenus; // lot 3 : un niveau (« Effacer ▸ »)
  };
  ```
  et dans `Layout` : `QStringList objectBar; // barre d'objet de la sélection (F-521), dans l'ordre`. Dans
  `layout.cpp` : un élément de `items` qui est un objet `{"submenu": id, "label": …, "items": [...]}` ajoute
  `">" + id` à `items` et un `MenuSpec` à `submenus` (id vide, libellé vide, élément d'un autre type ou sous-menu
  imbriqué : erreur « menu <n> : sous-menu invalide ») ; clé `objectBar` facultative (tableau de chaînes, sinon
  erreur « objectBar : tableau de chaînes attendu ») ; `unknownCommands` parcourt aussi les sous-menus et
  `objectBar`. Commande ciblée → vert.
- [ ] **Étape 3 : données du lot.** Ajouter **en fin de** `commands` de `actions/data/commands.json`, en un bloc :
  ```json
      { "id": "tool.select", "label": "Sélectionner", "icon": "selection", "shortcut": "Esc", "checkable": true },
      { "id": "tool.highlighter", "label": "Marqueur", "icon": "marqueur", "shortcut": "Ctrl+H", "checkable": true },
      { "id": "tool.eraser", "label": "Gomme", "icon": "gomme", "shortcut": "Ctrl+E", "checkable": true },
      { "id": "edit.undo", "label": "Annuler", "icon": "annuler", "shortcut": "Ctrl+Z" },
      { "id": "edit.redo", "label": "Rétablir", "icon": "retablir", "shortcut": "Ctrl+Y" },
      { "id": "edit.delete", "label": "Supprimer", "icon": "supprimer", "shortcut": "Del" },
      { "id": "edit.select-all", "label": "Tout sélectionner", "shortcut": "Ctrl+A" },
      { "id": "clear.annotations", "label": "Annotations", "icon": "effacer" },
      { "id": "clear.objects", "label": "Objets" },
      { "id": "clear.grid", "label": "Grille" },
      { "id": "clear.background", "label": "Fond" },
      { "id": "clear.page", "label": "Page" },
      { "id": "page.revert", "label": "Rétablir la page" },
      { "id": "view.trash", "label": "Corbeille du paperboard", "icon": "corbeille", "checkable": true },
      { "id": "object.move-free", "label": "Déplacer librement", "icon": "deplacer" },
      { "id": "object.rotate", "label": "Faire pivoter", "icon": "rotation" },
      { "id": "object.translucency", "label": "Translucidité", "icon": "translucidite", "argument": "number", "optional": true },
      { "id": "object.enlarge", "label": "Agrandir l'objet", "icon": "agrandir", "shortcut": "Ctrl++" },
      { "id": "object.reduce", "label": "Réduire l'objet", "icon": "reduire", "shortcut": "Ctrl+-" }
  ```
  Dans `actions/data/layout.json` : menu `edit` → `["edit.undo", "edit.redo", "-", "edit.delete", "edit.select-all",
  "-", {"submenu": "clear", "label": "Effacer", "items": ["clear.annotations", "clear.objects", "clear.grid",
  "clear.background", "clear.page"]}, "page.revert"]` ; menu `view` → ajouter `"-", "view.trash"` en fin ; menu
  `tools` → `["tool.select", "tool.pen", "tool.highlighter", "tool.eraser"]` ; palette : groupe `outils` →
  `["tool.select", "tool.pen", "tool.highlighter", "tool.eraser"]`, puis un groupe
  `{"group": "edition", "items": ["edit.undo", "edit.redo", "edit.delete"]}` juste après ; clé
  `"objectBar": ["object.move-free", "object.rotate", "object.translucency", "object.enlarge", "object.reduce"]`.
  Dessiner les 13 icônes (24 × 24, mêmes attributs de trait que `ui/icons/stylo.svg`, formes simples à nous :
  flèche, surligneur biseauté, gomme, flèches courbes, croix, poubelle, balai, quatre flèches, flèche circulaire,
  damier à moitié, loupes + et −).
- [ ] **Étape 4 : en-têtes figés et talons.** Créer les en-têtes ci-dessous **tels quels** ; chaque `.cpp` talon
  inclut son en-tête et définit chaque fonction déclarée par le corps le plus simple qui compile (valeur par défaut,
  `{}`, `nullptr`, conteneur vide), précédé de `// Talon L3-T1 : rempli par L3-T<n>.`.

  `core/include/actcraft/core/hittest.h` (rempli par L3-T3) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>

  #include <optional>
  #include <vector>

  namespace actcraft::core {

  // Désignation (F-210), coordonnées de page. L'appelant passe kHitToleranceScreen / zoom.
  inline constexpr double kHitToleranceScreen = 6.0;

  // Annotation (F-230) : trait de stylo, de marqueur ou d'encre révélatrice posé sur la couche supérieure.
  bool isAnnotation(const PageObject& object) noexcept;
  // Sélectionnable : visible (un objet verrouillé se sélectionne mais ne se déplace pas, F-210).
  bool isSelectable(const PageObject& object) noexcept;
  // Objet sélectionnable le plus haut (le dernier de page.objects, ordonnée par couche) touché par `point` : un trait à
  // moins de sa demi-épaisseur + `tolerance` de sa ligne centrale (transformation appliquée) ; un autre objet, point
  // dans son emprise (boundsOnPage) agrandie de `tolerance`.
  std::optional<ObjectId> objectAt(const Page& page, PointF point, double tolerance);
  // Objets sélectionnables dont l'emprise touche `rect` (bords compris), dans l'ordre de la page (D6, H3-4).
  std::vector<ObjectId> objectsTouching(const Page& page, const RectF& rect);
  RectF normalizedRect(PointF a, PointF b) noexcept;
  RectF unitedRect(const RectF& a, const RectF& b) noexcept; // un rectangle 0 × 0 est neutre
  bool rectsTouch(const RectF& a, const RectF& b) noexcept;

  } // namespace actcraft::core
  ```
  `core/include/actcraft/core/erasegeometry.h` (L3-T3) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>

  #include <vector>

  namespace actcraft::core {

  struct StrokeCut
  {
      std::vector<std::vector<StrokePoint>> pieces; // morceaux restants (≥ 2 points chacun), dans l'ordre du trait
      bool touched = false;                         // le disque a retiré une partie du trait
  };

  // Gomme (F-230) : retire de la ligne centrale la partie intérieure au disque (`center`, `radius`) ; pression
  // interpolée aux points de coupe ; un reste d'un seul point est abandonné.
  StrokeCut cutByDisk(const std::vector<StrokePoint>& points, PointF center, double radius);
  // Disque glissé de `from` à `to` : coupes successives à des pas d'au plus radius / 2 (aucun trait sauté).
  StrokeCut cutBySweep(const std::vector<StrokePoint>& points, PointF from, PointF to, double radius);
  // Points d'un objet trait en coordonnées de page (sa transformation appliquée) ; vide si ce n'est pas un trait.
  std::vector<StrokePoint> strokePointsOnPage(const PageObject& object);

  } // namespace actcraft::core
  ```
  `core/include/actcraft/core/objectedits.h` (L3-T4) :
  ```cpp
  #pragma once

  #include <actcraft/core/document.h>
  #include <actcraft/core/undo.h>

  #include <cstdint>
  #include <utility>
  #include <vector>

  namespace actcraft::core {

  inline constexpr int kMergeTranslucency = 3; // famille de fusion de SetTranslucencyCommand

  // Déplacer, redimensionner, faire pivoter, agrandir, réduire (F-210, F-522, F-523) : la transformation de chaque
  // objet est composée avec `delta` (coordonnées de page, appliquée après la sienne). Changes : un ObjectChanged par
  // objet, zone = emprise avant ∪ emprise après. Identifiants absents ignorés.
  class TransformObjectsCommand final : public UndoCommand
  {
  public:
      TransformObjectsCommand(std::size_t page, std::vector<ObjectId> ids, Transform delta);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_page;
      std::vector<ObjectId> m_ids;
      Transform m_delta;
      std::vector<std::pair<ObjectId, Transform>> m_before;
  };

  // Supprimer, corbeille (F-050, F-541). Undo : objets remis à leurs rangs d'origine (même profondeur).
  // Changes : ObjectRemoved (zone = emprise) ; à l'annulation, ObjectInserted.
  class RemoveObjectsCommand final : public UndoCommand
  {
  public:
      RemoveObjectsCommand(std::size_t page, std::vector<ObjectId> ids);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_page;
      std::vector<ObjectId> m_ids;
      std::vector<std::pair<std::size_t, PageObject>> m_removed; // rang d'origine, objet ; rangs croissants
  };

  // Gomme (F-230) : chaque trait `original` est remplacé par ses morceaux, au rang de l'original (même couche, même
  // profondeur, même commun hormis l'identifiant et la transformation, remise à l'identité : points en coordonnées de
  // page). Identifiants des morceaux tirés au premier redo et gardés ensuite. Aucun morceau : trait retiré.
  // Changes : ObjectRemoved(original), puis ObjectInserted par morceau ; à l'annulation, l'inverse.
  struct StrokeReplacement
  {
      ObjectId original = 0;
      std::vector<Stroke> pieces;
  };
  class ReplaceStrokesCommand final : public UndoCommand
  {
  public:
      ReplaceStrokesCommand(std::size_t page, std::vector<StrokeReplacement> replacements);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;
      const std::vector<ObjectId>& pieceIds() const noexcept; // vide avant le premier redo

  private:
      std::size_t m_page;
      std::vector<StrokeReplacement> m_replacements;
      std::vector<PageObject> m_originals;
      std::vector<ObjectId> m_pieceIds;
  };

  enum class ClearKind : std::uint8_t
  {
      Annotations = 0, // isAnnotation
      Objects = 1,     // tout objet qui n'est pas une annotation (D8)
      Grid = 2,        // grille remise à Grid{}
      Background = 3,  // fond remis à Background{}
      Page = 4,        // tout
  };
  // Effacer ▸ (F-130). Changes : un PageChanged (page entière) si quelque chose a changé ; rien sinon.
  class ClearPageCommand final : public UndoCommand
  {
  public:
      ClearPageCommand(std::size_t page, ClearKind kind);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_page;
      ClearKind m_kind;
      Page m_before;
  };

  // Rétablir la page (F-131) : la page devient `saved` (taille, fond, grille, objets), en une étape (D9).
  // Changes : un PageChanged (page entière).
  class ReplacePageCommand final : public UndoCommand
  {
  public:
      ReplacePageCommand(std::size_t page, Page saved);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_page;
      Page m_other;
  };

  // Translucidité (F-524) : 0 opaque … 255 invisible. Fusionne avec la précédente de même famille portant sur les
  // mêmes objets (réglette glissée : une étape). Changes : ObjectChanged par objet.
  class SetTranslucencyCommand final : public UndoCommand
  {
  public:
      SetTranslucencyCommand(std::size_t page, std::vector<ObjectId> ids, std::uint8_t value);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;
      int mergeId() const noexcept override;
      bool mergeWith(const UndoCommand& other) override;

  private:
      std::size_t m_page;
      std::vector<ObjectId> m_ids;
      std::uint8_t m_value;
      std::vector<std::pair<ObjectId, std::uint8_t>> m_before;
  };

  // Centre de l'emprise réunie des objets (pivot de la rotation et des pas de 10 %) ; page sans ces objets : (0, 0).
  PointF selectionCenter(const Page& page, const std::vector<ObjectId>& ids);
  Transform scalingAbout(PointF center, double sx, double sy) noexcept;
  Transform rotationAbout(PointF center, double degrees) noexcept;

  } // namespace actcraft::core
  ```
  `core/include/actcraft/core/lift.h` (L3-T4) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>
  #include <actcraft/core/undo.h>

  #include <vector>

  namespace actcraft::core {

  // Soulever (DA11, lot 3) : objets retirés de la page affichée le temps d'un geste, hors pile.
  struct LiftedObject
  {
      std::size_t index = 0; // rang d'origine dans page.objects
      PageObject object;
  };

  // Retire les objets `ids` présents (inconnus ignorés) ; rend leur liste dans l'ordre de la page (rangs croissants).
  std::vector<LiftedObject> takeObjects(Page& page, const std::vector<ObjectId>& ids);
  // Remet les objets à leurs rangs d'origine : la page redevient celle d'avant takeObjects.
  void restoreObjects(Page& page, const std::vector<LiftedObject>& lifted);
  // Pour le cache, au soulèvement : un ObjectRemoved par objet, zone = son emprise (boundsOnPage).
  Changes liftChanges(std::size_t page, const std::vector<LiftedObject>& lifted);
  // Pour le cache, au dépôt, `command` étant les changements de la commande exécutée sur la page restaurée (vide :
  // geste annulé) ; le cache n'a plus aucun objet soulevé :
  // - chaque objet soulevé encore présent dans `pageAfter` : un ObjectInserted (zone = emprise dans `pageAfter`) ;
  // - aucun ObjectRemoved ni ObjectChanged d'un objet soulevé ;
  // - tout autre changement de `command` passe tel quel, dans son ordre (morceaux de gomme : ObjectInserted).
  Changes changesAfterLift(std::size_t page, const std::vector<LiftedObject>& lifted, const Changes& command,
                           const Page& pageAfter);

  } // namespace actcraft::core
  ```
  `input/include/actcraft/input/routing.h` (L3-T5) :
  ```cpp
  #pragma once

  #include <actcraft/input/sample.h>

  #include <cstdint>
  #include <memory>
  #include <unordered_map>

  namespace actcraft::input {

  using UserId = std::uint8_t;
  inline constexpr UserId kFirstUser = 0;

  enum class Route : std::uint8_t
  {
      Tool,    // l'outil courant de l'utilisateur reçoit les échantillons du contact
      Gesture, // le geste à deux doigts de la vue
      Ignore,  // rien (paume, troisième doigt)
  };

  struct RouterState
  {
      bool stylusDown = false; // un contact de stylet ou de souris est en cours
      int toolTouches = 0;     // doigts en cours routés vers l'outil
      int gestureTouches = 0;  // doigts en cours routés vers le geste
  };

  struct Decision
  {
      Route route = Route::Ignore;
      UserId user = kFirstUser;
      bool endToolContacts = false; // les doigts en cours vers l'outil passent au geste : la vue termine leur trait
  };

  // Politique de routage (DA-A1), remplaçable : décide à l'appui ; la décision vaut jusqu'au relâché.
  class RoutingPolicy
  {
  public:
      virtual ~RoutingPolicy() = default;
      virtual Decision decide(const Sample& press, const RouterState& state) const = 0;
  };

  // Gestion complète du toucher (décision 25, N2-A5) : stylet et souris → outil ; doigt pendant un contact de stylet ou
  // de souris → rien (paume) ; premier doigt → outil ; deuxième doigt → geste, et le premier y passe
  // (endToolContacts) ; au-delà de deux doigts → rien. Un seul utilisateur (kFirstUser) jusqu'au lot 11.
  class FullTouchPolicy final : public RoutingPolicy
  {
  public:
      Decision decide(const Sample& press, const RouterState& state) const override;
  };

  class PointerRouter
  {
  public:
      PointerRouter(); // FullTouchPolicy
      void setPolicy(std::unique_ptr<RoutingPolicy> policy); // les contacts en cours gardent leur décision
      // À l'appui, la politique décide et la décision est retenue pour `sample.pointer` (stylet et souris : 0) ;
      // ensuite, la décision retenue ; au relâché, elle est rendue puis oubliée. Mouvement d'un contact inconnu : Ignore.
      // endToolContacts : les décisions retenues des doigts routés vers l'outil passent à Gesture.
      Decision route(const Sample& sample);
      int gestureTouches() const noexcept;
      void reset(); // tout contact oublié (perte de saisie)

  private:
      std::unique_ptr<RoutingPolicy> m_policy;
      std::unordered_map<std::uint64_t, Decision> m_contacts;
      RouterState m_state;
  };

  } // namespace actcraft::input
  ```
  `input/include/actcraft/input/gesture.h` (L3-T5) :
  ```cpp
  #pragma once

  namespace actcraft::input {

  struct Point2
  {
      double x = 0;
      double y = 0;
  };

  // Écart relatif des deux doigts sous lequel le zoom ne bouge pas (défilement seul).
  inline constexpr double kPinchDeadZone = 0.03;

  struct GestureStep
  {
      Point2 anchor; // milieu des doigts à l'appui (vue) : le point de page qui s'y trouvait suit `center`
      Point2 center; // milieu des doigts maintenant (vue)
      double scale = 1; // écart des doigts / écart à l'appui ; 1 dans la zone morte
  };

  // Pincer et défiler à deux doigts (décision 25), pixels de vue.
  class TwoFingerGesture
  {
  public:
      void begin(Point2 a, Point2 b);
      GestureStep update(Point2 a, Point2 b) const; // sans begin : scale 1, anchor = center = milieu
      bool active() const noexcept;
      void end() noexcept;

  private:
      Point2 m_anchor;
      double m_distance = 0;
      bool m_active = false;
  };

  } // namespace actcraft::input
  ```
  `tools/include/actcraft/tools/tool.h` (L3-T6) :
  ```cpp
  #pragma once

  #include <actcraft/input/sample.h>

  #include <QString>

  namespace actcraft::tools {

  // Un outil (DA6) : reçoit les échantillons de son utilisateur, ne voit que son ToolContext.
  class Tool : public input::SampleSink
  {
  public:
      ~Tool() override = default;
      virtual QString id() const = 0;           // « select », « pen », « highlighter », « eraser »
      virtual void activate() {}                // devient l'outil courant
      virtual void deactivate() { cancel(); }   // n'est plus l'outil courant
      virtual bool isBusy() const noexcept = 0; // un geste est en cours
      // Termine le geste en cours proprement (changement de page, Annuler, Échap, changement d'outil) : le stylo verse
      // son trait ; la sélection et la gomme remettent ce qu'elles ont soulevé, sans étape d'annulation.
      virtual void cancel() = 0;
      // Touche (Qt::Key, Qt::KeyboardModifiers) ; vrai si l'outil l'a prise.
      virtual bool onKey(int key, int modifiers)
      {
          static_cast<void>(key);
          static_cast<void>(modifiers);
          return false;
      }
  };

  } // namespace actcraft::tools
  ```
  `tools/include/actcraft/tools/toolcontext.h` (L3-T6) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>
  #include <actcraft/core/undo.h>
  #include <actcraft/tools/pentool.h>

  #include <QString>
  #include <QStringList>
  #include <QVariant>

  #include <memory>
  #include <optional>
  #include <vector>

  namespace actcraft::tools {

  class UserState;

  // Le document vu par un outil : lecture de la page affichée, écriture par la pile seulement.
  class DocumentEditor
  {
  public:
      virtual ~DocumentEditor() = default;
      virtual const core::Page& page() const = 0; // page affichée, sans les objets soulevés
      virtual std::size_t pageIndex() const = 0;
      virtual void execute(std::unique_ptr<core::UndoCommand> command) = 0;
      // Soulever (DA11) : retire les objets de la page affichée hors pile (le cache en reçoit remove) et rend leurs
      // copies, dans l'ordre de la page. Un second appel pendant le même geste ajoute aux objets soulevés.
      virtual std::vector<core::PageObject> lift(const std::vector<core::ObjectId>& ids) = 0;
      // Remet les objets soulevés puis exécute `command` par la pile (une étape) ; nullptr : remise sans étape.
      virtual void drop(std::unique_ptr<core::UndoCommand> command) = 0;
      virtual bool isLifting() const = 0;
  };

  // Couche soulevée : dessine les copies au-dessus de la page pendant le geste.
  class LiftSurface
  {
  public:
      virtual ~LiftSurface() = default;
      virtual void show(std::vector<core::PageObject> objects) = 0; // remplace ; transformation remise à l'identité
      virtual void setTransform(const core::Transform& extra) = 0;  // appliquée après celle de chaque objet
      virtual void clear() = 0;
  };

  enum class HandleKind : std::uint8_t
  {
      TopLeft,
      Top,
      TopRight,
      Right,
      BottomRight, // proportionnelle (F-520)
      Bottom,
      BottomLeft,
      Left,
  };

  struct BarSpot
  {
      QString command; // « object.enlarge »…
      core::PointF center;
  };

  // Ce que la sélection montre ; l'interface le dessine (coordonnées de page).
  struct SelectionView
  {
      bool visible = false;
      std::vector<core::PointF> corners; // 4 coins : haut gauche, haut droit, bas droit, bas gauche
      std::vector<core::PointF> handles; // 8 poignées, dans l'ordre de HandleKind
      std::vector<BarSpot> bar;          // barre d'objet (F-521)
      bool handlesVisible = true;        // F-528 : faux pendant un glissement
      std::optional<core::RectF> rubberBand;
  };

  class SelectionSurface
  {
  public:
      virtual ~SelectionSurface() = default;
      virtual void show(const SelectionView& view) = 0;
  };

  class ViewInfo
  {
  public:
      virtual ~ViewInfo() = default;
      virtual double zoom() const = 0;                          // pixels d'écran par pixel de page
      virtual bool overTrash(core::PointF pagePoint) const = 0; // F-050
      virtual void setTrashHover(bool hover) = 0;               // flèche de la corbeille
      virtual QStringList objectBarCommands() const = 0;        // disposition (`objectBar`)
  };

  class CommandSink
  {
  public:
      virtual ~CommandSink() = default;
      virtual bool trigger(const QString& id, const QVariant& argument) = 0;
  };

  struct ToolContext
  {
      DocumentEditor& document;
      InkSurface& ink;
      DocumentSink& strokes;
      LiftSurface& lift;
      SelectionSurface& selection;
      ViewInfo& view;
      CommandSink& commands;
      UserState& user;
  };

  } // namespace actcraft::tools
  ```
  `tools/include/actcraft/tools/userstate.h` (L3-T6) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>
  #include <actcraft/input/routing.h>

  #include <QString>

  #include <cstdint>
  #include <map>
  #include <vector>

  namespace actcraft::tools {

  struct InkSettings
  {
      core::Argb color = 0xFF000000u;
      double width = 4;
      std::uint8_t translucency = 0; // 0 opaque … 255 invisible
  };

  // Réglages par défaut (D5) : « pen » noir, 4, opaque ; « highlighter » #FFFF00, 20, 128 ; « eraser » 20 ;
  // autre outil : ceux du stylo.
  InkSettings defaultSettings(const QString& toolId);

  // Un utilisateur (DA-A1) : outil courant, réglages par outil (F-136), sélection, outils de maths ouverts (lot 7).
  class UserState
  {
  public:
      explicit UserState(input::UserId id = input::kFirstUser);
      input::UserId id() const noexcept;
      const QString& currentTool() const noexcept; // « pen » au départ
      void setCurrentTool(const QString& toolId);
      InkSettings settings(const QString& toolId) const; // jamais réglé : defaultSettings
      void setSettings(const QString& toolId, const InkSettings& settings);
      const std::vector<core::ObjectId>& selection() const noexcept;
      void setSelection(std::vector<core::ObjectId> ids);
      const std::vector<QString>& openMathTools() const noexcept; // réservé au lot 7
      void setOpenMathTools(std::vector<QString> ids);

  private:
      input::UserId m_id;
      QString m_tool = QStringLiteral("pen");
      std::map<QString, InkSettings> m_settings;
      std::vector<core::ObjectId> m_selection;
      std::vector<QString> m_mathTools;
  };

  } // namespace actcraft::tools
  ```
  `tools/include/actcraft/tools/toolregistry.h` (L3-T6) :
  ```cpp
  #pragma once

  #include <actcraft/tools/tool.h>
  #include <actcraft/tools/toolcontext.h>

  #include <QStringList>

  #include <functional>
  #include <memory>
  #include <utility>
  #include <vector>

  namespace actcraft::tools {

  class ToolRegistry
  {
  public:
      using Factory = std::function<std::unique_ptr<Tool>(ToolContext& context)>;
      bool add(const QString& id, Factory factory); // faux : identifiant vide ou déjà pris
      bool contains(const QString& id) const;
      QStringList ids() const; // ordre d'ajout
      std::unique_ptr<Tool> create(const QString& id, ToolContext& context) const; // nullptr : inconnu

  private:
      std::vector<std::pair<QString, Factory>> m_factories;
  };

  // « select », « pen », « highlighter », « eraser », dans cet ordre.
  void registerBuiltinTools(ToolRegistry& registry);

  } // namespace actcraft::tools
  ```
  `tools/include/actcraft/tools/erasertool.h` (L3-T7) et `selecttool.h` (L3-T8, L3-T9) :
  ```cpp
  // erasertool.h
  #pragma once

  #include <actcraft/tools/tool.h>
  #include <actcraft/tools/toolcontext.h>

  namespace actcraft::tools {

  // Gomme (F-230) : soulève chaque annotation touchée (une fois), la recoupe dans la couche soulevée pendant le geste,
  // dépose une ReplaceStrokesCommand au relâché.
  class EraserTool final : public Tool
  {
  public:
      explicit EraserTool(ToolContext& context);
      QString id() const override; // « eraser »
      void onSample(const input::Sample& sample) override;
      bool isBusy() const noexcept override;
      void cancel() override;
      double radius() const; // largeur réglée de la gomme / 2, pixels de page

  private:
      ToolContext& m_context;
  };

  } // namespace actcraft::tools
  ```
  ```cpp
  // selecttool.h
  #pragma once

  #include <actcraft/tools/tool.h>
  #include <actcraft/tools/toolcontext.h>

  #include <cstdint>

  namespace actcraft::tools {

  // Sélection (F-210, F-520 à F-528, F-541, F-050).
  class SelectTool final : public Tool
  {
  public:
      explicit SelectTool(ToolContext& context);
      QString id() const override; // « select »
      void activate() override;
      void deactivate() override; // garde la sélection, masque le cadre
      void onSample(const input::Sample& sample) override;
      bool isBusy() const noexcept override;
      void cancel() override;
      bool onKey(int key, int modifiers) override; // Suppr : supprimer ; Échap : abandon du geste
      void selectAll();
      void clearSelection();
      void deleteSelection(); // sans sélection : rien (F-541)
      void enlarge();         // × kEnlargeStep autour du centre (F-523)
      void reduce();          // ÷ kEnlargeStep
      void setTranslucency(std::uint8_t value);
      void refresh(); // la page a changé (annuler, page suivante) : sélection filtrée, cadre recalculé

  private:
      ToolContext& m_context;
  };

  } // namespace actcraft::tools
  ```
  `tools/include/actcraft/tools/selectiongeometry.h` (L3-T8, L3-T9) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>
  #include <actcraft/tools/toolcontext.h>

  #include <QString>
  #include <QStringList>

  #include <vector>

  namespace actcraft::tools {

  inline constexpr double kHandleRadiusScreen = 6.0; // poignée ronde (F-520), pixels d'écran
  inline constexpr double kBarButtonScreen = 24.0;   // bouton carré de la barre (F-521)
  inline constexpr double kBarGapScreen = 8.0;       // écart cadre → barre
  inline constexpr double kEnlargeStep = 1.1;        // F-523

  // Cadre (emprise réunie ; un seul objet : son cadre transformé), 8 poignées (ordre de HandleKind), barre au-dessus
  // du cadre, replacée sous lui si elle sortirait du haut de la page (F-521). Coordonnées de page.
  SelectionView layoutSelection(const core::Page& page, const std::vector<core::ObjectId>& ids, double zoom,
                                const QStringList& barCommands);

  enum class SelectionHitKind : std::uint8_t
  {
      None,
      Inside,
      Handle,
      BarButton,
  };
  struct SelectionHit
  {
      SelectionHitKind kind = SelectionHitKind::None;
      HandleKind handle = HandleKind::TopLeft;
      QString command;
  };
  // Ordre d'essai : bouton de barre, poignée, intérieur du cadre.
  SelectionHit hitSelection(const SelectionView& view, core::PointF point, double zoom);

  } // namespace actcraft::tools
  ```
  `render/include/actcraft/render/liftlayeritem.h` (L3-T10) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>

  #include <actcraft/render/fixedpageitem.h> // type complet exigé par moc pour la propriété fixedPage

  #include <QPointer>
  #include <QQuickItem>
  #include <QtQml/qqmlregistration.h>

  #include <vector>

  namespace actcraft::render {

  // Couche soulevée (lot 3, DA11) : dessine les copies des objets soulevés pendant un geste, avec les moteurs de la
  // page fixe (createObjectNode), sous une transformation supplémentaire qui ne reconstruit aucun nœud.
  class LiftLayerItem : public QQuickItem
  {
      Q_OBJECT
      QML_NAMED_ELEMENT(LiftLayer)
      Q_PROPERTY(actcraft::render::FixedPageItem* fixedPage READ fixedPage WRITE setFixedPage NOTIFY fixedPageChanged)
      Q_PROPERTY(int objectCount READ objectCount NOTIFY objectsChanged)

  public:
      explicit LiftLayerItem(QQuickItem* parent = nullptr);
      FixedPageItem* fixedPage() const noexcept;
      void setFixedPage(FixedPageItem* page);
      void setObjects(std::vector<core::PageObject> objects); // remplace ; transformation remise à l'identité
      void setExtraTransform(const core::Transform& extra);
      void clear();
      int objectCount() const noexcept;
      int builtCount() const noexcept; // crochet de test : nœuds d'objet construits depuis la création

  signals:
      void fixedPageChanged();
      void objectsChanged();

  protected:
      QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData* data) override;

  private:
      QPointer<FixedPageItem> m_fixedPage;
      std::vector<core::PageObject> m_objects;
  };

  } // namespace actcraft::render
  ```
  `ui/include/actcraft/ui/toolhost.h` (L3-T14) :
  ```cpp
  #pragma once

  #include <actcraft/input/routing.h>
  #include <actcraft/render/liftlayeritem.h> // types complets exigés par moc pour les propriétés
  #include <actcraft/ui/commands.h>
  #include <actcraft/ui/documentcontroller.h>
  #include <actcraft/ui/pageview.h>

  #include <QColor>
  #include <QObject>
  #include <QPointer>
  #include <QRectF>
  #include <QString>
  #include <QVariantMap>
  #include <QtQml/qqmlregistration.h>

  #include <memory>

  namespace actcraft::tools {
  class Tool;
  class UserState;
  } // namespace actcraft::tools

  namespace actcraft::ui {

  struct ToolAdapters; // tooladapters.h

  // Outils de l'utilisateur (DA6, DA-A1) : registre, état d'utilisateur, adaptateurs des interfaces d'outil,
  // commandes du lot 3, mémoire par outil (F-136, QSettings).
  class ToolHost : public QObject
  {
      Q_OBJECT
      QML_NAMED_ELEMENT(Tools)
      Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY wiringChanged)
      Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                     setDocumentController NOTIFY wiringChanged)
      Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY wiringChanged)
      Q_PROPERTY(actcraft::render::LiftLayerItem* liftLayer READ liftLayer WRITE setLiftLayer NOTIFY wiringChanged)
      Q_PROPERTY(QString currentTool READ currentTool NOTIFY currentToolChanged)
      Q_PROPERTY(QColor color READ color NOTIFY settingsChanged)
      Q_PROPERTY(qreal width READ width NOTIFY settingsChanged)
      Q_PROPERTY(int selectionCount READ selectionCount NOTIFY selectionChanged)
      Q_PROPERTY(bool handlesVisible READ handlesVisible NOTIFY selectionChanged)
      Q_PROPERTY(QVariantMap selectionFrame READ selectionFrame NOTIFY selectionChanged)
      Q_PROPERTY(QRectF trashArea READ trashArea WRITE setTrashArea NOTIFY trashAreaChanged) // scène
      Q_PROPERTY(bool trashHover READ trashHover NOTIFY trashHoverChanged)
      // Vide : réglages de l'application ; tests : chemin d'un fichier .ini.
      Q_PROPERTY(QString settingsFile READ settingsFile WRITE setSettingsFile NOTIFY settingsFileChanged)

  public:
      explicit ToolHost(QObject* parent = nullptr);
      ~ToolHost() override;

      Commands* commands() const noexcept;
      void setCommands(Commands* commands);
      DocumentController* documentController() const noexcept;
      void setDocumentController(DocumentController* controller);
      PageView* view() const noexcept;
      void setView(PageView* view);
      render::LiftLayerItem* liftLayer() const noexcept;
      void setLiftLayer(render::LiftLayerItem* layer);
      QString currentTool() const;
      QColor color() const;
      qreal width() const;
      int selectionCount() const;
      bool handlesVisible() const;
      QVariantMap selectionFrame() const;
      QRectF trashArea() const noexcept;
      void setTrashArea(const QRectF& area);
      bool trashHover() const noexcept;
      QString settingsFile() const;
      void setSettingsFile(const QString& path);

      // « select », « pen », « highlighter », « eraser » : outil courant de l'utilisateur 1, commande tool.<id> cochée,
      // réglages de l'outil rendus à la vue (couleur et épaisseur de l'encre).
      Q_INVOKABLE void activate(const QString& toolId);
      Q_INVOKABLE void setColor(const QColor& color); // outil courant (F-134)
      Q_INVOKABLE void setWidth(qreal width);
      Q_INVOKABLE void selectAll();
      Q_INVOKABLE void clearSelection();
      Q_INVOKABLE void deleteSelection();
      Q_INVOKABLE void enlargeSelection();
      Q_INVOKABLE void reduceSelection();
      Q_INVOKABLE void setSelectionTranslucency(int value);
      Q_INVOKABLE void cancelGesture();

      // Outil courant de l'utilisateur ; nullptr tant que le contrôleur, la vue et la couche ne sont pas branchés.
      tools::Tool* activeTool(input::UserId user = input::kFirstUser) const;

  signals:
      void wiringChanged();
      void currentToolChanged();
      void settingsChanged();
      void selectionChanged();
      void trashAreaChanged();
      void trashHoverChanged();
      void settingsFileChanged();

  private:
      QPointer<Commands> m_commands;
      QPointer<DocumentController> m_controller;
      QPointer<PageView> m_view;
      QPointer<render::LiftLayerItem> m_liftLayer;
      QRectF m_trashArea;
      bool m_trashHover = false;
      QString m_settingsFile;
  };

  } // namespace actcraft::ui
  ```
  `ui/src/tooladapters.h` (L3-T14 ; talon : déclaration seule) :
  ```cpp
  #pragma once

  namespace actcraft::ui {

  // Adaptateurs des interfaces d'outil (tools::DocumentEditor, LiftSurface, SelectionSurface, ViewInfo, CommandSink)
  // sur DocumentController, LiftLayerItem, ToolHost, PageView et Commands. Rempli par L3-T14.
  struct ToolAdapters;

  } // namespace actcraft::ui
  ```
  **Ajouts d'API aux fichiers existants** (déclarations ci-dessous, corps vides ou valeur par défaut dans le `.cpp`,
  `// Talon L3-T1 : rempli par L3-T12.` / `L3-T13`) :
  ```cpp
  // documentcontroller.h — propriétés, à la suite des existantes
  Q_PROPERTY(int shownObjectCount READ shownObjectCount NOTIFY documentChanged)
  Q_PROPERTY(bool lifting READ isLifting NOTIFY liftingChanged)
  Q_PROPERTY(bool canRevertPage READ canRevertPage NOTIFY documentChanged)
  // documentcontroller.h — public, à la suite des existants
  int shownObjectCount() const noexcept;
  // Soulever (DA11, lot 3) : retire les objets de la page affichée hors pile et rend leurs copies ; le cache reçoit
  // core::liftChanges. Annonce editAboutToApply avant le premier soulèvement d'un geste. Pendant un soulèvement,
  // Enregistrer, Ouvrir et les changements de page sont refusés ou attendent le dépôt (L3-T12).
  std::vector<core::PageObject> liftObjects(const std::vector<core::ObjectId>& ids);
  // Remet les objets soulevés, exécute `command` par la pile (nullptr : aucune étape), et le cache reçoit
  // core::changesAfterLift.
  void dropLifted(std::unique_ptr<core::UndoCommand> command);
  bool isLifting() const noexcept;
  // Trait d'un outil d'encre avec sa translucidité (marqueur, F-225) : couche supérieure, par la pile.
  void commitAnnotation(core::Stroke stroke, std::uint8_t translucency);
  void deleteObjects(const std::vector<core::ObjectId>& ids);
  Q_INVOKABLE void clearPage(int kind); // core::ClearKind (F-130)
  Q_INVOKABLE void revertPage();        // F-131
  bool canRevertPage() const;
  // Tests : reçoit, après la page fixe, les mêmes appels de cache (nullptr : aucun).
  void setCacheObserver(core::PageCache* observer);
  // documentcontroller.h — signaux
  // Avant Annuler, Rétablir, un soulèvement, Effacer, Rétablir la page, Supprimer : la vue termine le trait en cours
  // et les outils leur geste (L2-T15).
  void editAboutToApply();
  void liftingChanged();
  ```
  ```cpp
  // pageview.h — en tête : #include <actcraft/input/routing.h> ; namespace actcraft::ui { class ToolHost; } ; dans la
  // classe, Q_MOC_INCLUDE(<actcraft/ui/toolhost.h>) (toolhost.h inclut pageview.h : inclusion circulaire évitée)
  Q_PROPERTY(actcraft::ui::ToolHost* tools READ tools WRITE setTools NOTIFY toolsChanged)
  Q_PROPERTY(int cacheRefreshCount READ cacheRefreshCount NOTIFY cacheUpdated)
  Q_PROPERTY(bool gestureActive READ gestureActive NOTIFY gestureActiveChanged)
  // public
  ToolHost* tools() const noexcept;
  // Posé : les échantillons routés vers l'outil vont à ToolHost::activeTool(utilisateur du contact) ; sinon, ou s'il
  // rend nullptr, au stylo interne (lots 1 et 2, banc).
  void setTools(ToolHost* tools);
  int cacheRefreshCount() const noexcept; // redessins du cache terminés depuis la création (crochet S6, H10-1)
  bool gestureActive() const noexcept;    // un geste à deux doigts est en cours
  input::PointerRouter& router() noexcept; // tests
  // signaux
  void toolsChanged();
  void gestureActiveChanged();
  ```
  Talons QML (`SelectionOverlay.qml`, `ObjectBar.qml`, `TranslucencyPopup.qml`, `TrashCan.qml`,
  `PageContextMenu.qml`, `ToolCursor.qml`) : `import QtQuick` puis `Item { }` avec, pour `ToolCursor.qml`,
  `property int cursorShape: Qt.ArrowCursor`, et pour `PageContextMenu.qml`, `function popupAt(x, y) {}`.
- [ ] **Étape 5 : CMake.** Ajouter les sources aux modules : `core/CMakeLists.txt` (`src/hittest.cpp`,
  `src/erasegeometry.cpp`, `src/objectedits.cpp`, `src/lift.cpp`), `input/CMakeLists.txt` (`src/routing.cpp`,
  `src/gesture.cpp`), `tools/CMakeLists.txt` (`src/userstate.cpp`, `src/toolregistry.cpp`, `src/erasertool.cpp`,
  `src/selecttool.cpp`, `src/selectiongeometry.cpp`), `render/CMakeLists.txt` (`src/liftlayeritem.cpp`),
  `ui/CMakeLists.txt` (`src/toolhost.cpp`, `src/tooladapters.cpp` ; les six QML ; les 13 icônes dans `RESOURCES` ;
  `SelectionOverlay.qml`, `ObjectBar.qml`, `TrashCan.qml`, `PageContextMenu.qml`, `TranslucencyPopup.qml` dans la
  liste `QT_QML_SKIP_CACHEGEN` des composants qui appellent `Commands.trigger`, comme `TopBar.qml`). Tests :
  `actcraft_add_test(tst_core_hittest SOURCES tst_core_hittest.cpp LIBS ActCraft::core)` et de même
  `tst_core_erase`, `tst_core_objectedits`, `tst_core_lift` (`tests/core.cmake`) ; `tst_input_routing`,
  `tst_input_gesture` (`LIBS ActCraft::input`, `tests/input.cmake`) ; `tst_tools_registry`, `tst_tools_eraser`,
  `tst_tools_select` (`LIBS ActCraft::tools`, `tests/tools.cmake`) ; `tst_render_liftlayer`, `tst_render_strokeshape`
  (comme `tst_render_pendinglayer`, `tests/render.cmake`) ; `tst_ui_lift`, `tst_ui_tools` (comme `tst_ui_document`,
  `tests/ui.cmake`) ; `tst_captures_selection`, `tst_captures_corbeille`, `tst_captures_marqueur`,
  `tst_captures_ecrire_gommer_annuler` (comme `tst_captures_anticrenelage`, `tests/bench.cmake`).
- [ ] **Étape 6 : tests talons.** Chacun des 13 fichiers, sur ce modèle (nom de classe et de `.moc` adaptés ;
  `QTEST_MAIN` pour `render` et `ui`, `QTEST_GUILESS_MAIN` pour `core`, `input`, `tools`) :
  ```cpp
  #include <QTest>

  class TstCoreHittest : public QObject
  {
      Q_OBJECT

  private slots:
      void reserved() { QSKIP("Talon L3-T1 : rempli par L3-T3."); }
  };

  QTEST_GUILESS_MAIN(TstCoreHittest)
  #include "tst_core_hittest.moc"
  ```
  Scénarios talons, sur ce modèle (nom et tâche adaptés) :
  ```json
  {
    "format": "actcraft-scenario/1",
    "name": "selection",
    "description": "Talon (L3-T1) : rempli par L3-T15 (S34).",
    "steps": [ {"waitReady": true} ]
  }
  ```
- [ ] **Étape 7 : sous-menus à l'écran.** Test rouge dans `tests/tst_ui_commands.cpp` :
  ```cpp
  void submenuInMenus()
  {
      ui::Commands commands;
      const QVariantList menus = commands.menus();
      QVariantMap edit;
      for (const QVariant& m : menus) {
          if (m.toMap().value(QStringLiteral("id")).toString() == QLatin1String("edit")) {
              edit = m.toMap();
          }
      }
      QVERIFY(!edit.isEmpty());
      bool found = false;
      for (const QVariant& it : edit.value(QStringLiteral("items")).toList()) {
          const QVariantMap item = it.toMap();
          if (item.value(QStringLiteral("submenu")).toBool()) {
              found = true;
              QCOMPARE(item.value(QStringLiteral("label")).toString(), QStringLiteral("Effacer"));
              QCOMPARE(item.value(QStringLiteral("items")).toList().size(), 5);
          }
      }
      QVERIFY(found);
      QCOMPARE(commands.objectBar().size(), 5);
  }
  ```
  Puis `Commands::menus()` rend un élément `{submenu: true, id, label, items: [...]}` pour un jeton `>id` (sous-menu
  sans commande disponible : omis, séparateurs nettoyés comme au premier niveau) ; propriété
  `Q_PROPERTY(QVariantList objectBar READ objectBar NOTIFY layoutChanged)` → `[{id, label, icon}]` des commandes
  disponibles de `objectBar` ; `CommandMenu.qml` dessine un `Menu` imbriqué pour un élément `submenu` (par un
  `Instantiator` comme le premier niveau ; jamais de boucle `removeItem`, piège L1-T12). Vert.
- [ ] **Étape 8 : preuves.** `format.ps1`, preuve de tâche (N ≥ 111), fonctions des deux tests nommés en `PASS`,
  `premiers-pas` hors écran `CAPTURES OK`, banc sans écran, `NOMS OK`.
- [ ] **Étape 9 : commit.**
  ```powershell
  git -C <wt> add actions/data/commands.json actions/data/layout.json actions/include/actcraft/actions/layout.h actions/src/layout.cpp tests/tst_actions_layout.cpp ui/include/actcraft/ui/commands.h ui/src/commands.cpp tests/tst_ui_commands.cpp ui/qml/CommandMenu.qml ui/icons core/CMakeLists.txt input/CMakeLists.txt tools/CMakeLists.txt render/CMakeLists.txt ui/CMakeLists.txt tests/core.cmake tests/input.cmake tests/tools.cmake tests/render.cmake tests/ui.cmake tests/bench.cmake core/include/actcraft/core/hittest.h core/src/hittest.cpp core/include/actcraft/core/erasegeometry.h core/src/erasegeometry.cpp core/include/actcraft/core/objectedits.h core/src/objectedits.cpp core/include/actcraft/core/lift.h core/src/lift.cpp input/include/actcraft/input/routing.h input/src/routing.cpp input/include/actcraft/input/gesture.h input/src/gesture.cpp tools/include/actcraft/tools/tool.h tools/include/actcraft/tools/toolcontext.h tools/include/actcraft/tools/userstate.h tools/src/userstate.cpp tools/include/actcraft/tools/toolregistry.h tools/src/toolregistry.cpp tools/include/actcraft/tools/erasertool.h tools/src/erasertool.cpp tools/include/actcraft/tools/selecttool.h tools/src/selecttool.cpp tools/include/actcraft/tools/selectiongeometry.h tools/src/selectiongeometry.cpp render/include/actcraft/render/liftlayeritem.h render/src/liftlayeritem.cpp ui/include/actcraft/ui/toolhost.h ui/src/toolhost.cpp ui/src/tooladapters.h ui/src/tooladapters.cpp ui/qml/SelectionOverlay.qml ui/qml/ObjectBar.qml ui/qml/TranslucencyPopup.qml ui/qml/TrashCan.qml ui/qml/PageContextMenu.qml ui/qml/ToolCursor.qml ui/include/actcraft/ui/documentcontroller.h ui/src/documentcontroller.cpp ui/include/actcraft/ui/pageview.h ui/src/pageview.cpp tests/tst_core_hittest.cpp tests/tst_core_erase.cpp tests/tst_core_objectedits.cpp tests/tst_core_lift.cpp tests/tst_input_routing.cpp tests/tst_input_gesture.cpp tests/tst_tools_registry.cpp tests/tst_tools_eraser.cpp tests/tst_tools_select.cpp tests/tst_render_liftlayer.cpp tests/tst_render_strokeshape.cpp tests/tst_ui_lift.cpp tests/tst_ui_tools.cpp bench/scenarios/selection.json bench/scenarios/corbeille.json bench/scenarios/marqueur.json bench/scenarios/ecrire-gommer-annuler.json
  git -C <wt> diff --cached --stat
  git -C <wt> commit -m "Lot 3 : réservation des commandes, menus, icônes, fichiers et tests du lot ; sous-menus de disposition (L3-T1)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t2"></a>
### Tâche L3-T2 — Réservation II : coquille QML

**Objectif.** Placer dans la coquille, une fois pour toutes, ce que les tâches d'interface rempliront : l'objet
`appTools` (type `Tools`), la couche soulevée au-dessus de la couche d'attente, la surcouche de sélection, le menu
contextuel, le curseur d'outil, la corbeille sous la palette ; les gestionnaires des **nouvelles** commandes du lot
et l'état actif d'Annuler, Rétablir et Rétablir la page. Les gestionnaires existants (`tool.pen`, `pen.color`,
`pen.width`) ne changent pas ici (L3-T14).

**Fichiers.** Modifier : `ui/qml/Main.qml`, `ui/qml/PageArea.qml`, `ui/qml/ToolPalette.qml`, `tests/tst_ui_shell.cpp`.

**Test et commande bornée.** Commande ciblée `tst_ui_shell` ; preuve de tâche N ≥ 111, `TESTS OK`, fonctions
`lot3ShellPlaceholders` et `everyLot3CommandHasAHandler` en `PASS` ; `premiers-pas` hors écran `CAPTURES OK` ;
banc sans écran `content_check.ok = True`.

**Interdits.** Ne renommer aucun `objectName` existant (lus par le banc et les scénarios) ; aucun comportement dans
les talons QML ; ne pas toucher aux gestionnaires existants ; la couche soulevée au-dessus de `pendingLayer`
(L10-T4) et sous `ink`.

**Terminé quand.** Les objets `appTools`, `liftLayer`, `selectionOverlay`, `pageContextMenu`, `toolCursor`,
`trashCan` existent dans la fenêtre ; chaque commande du lot sauf `object.move-free` et `object.rotate` a un
gestionnaire ; `edit.undo` et `edit.redo` suivent `canUndo` / `canRedo`, `page.revert` suit `canRevertPage` ; S2
inchangé.

**Étapes.**
- [ ] **Étape 1 : test rouge.** Dans `tests/tst_ui_shell.cpp`, avec le moyen qu'il a déjà de charger `Main.qml` :
  ```cpp
  void lot3ShellPlaceholders()
  {
      QQuickWindow* window = loadMainWindow(); // aide existante du test (sinon, celle qui charge Main.qml)
      QVERIFY(window);
      for (const char* name : {"appTools", "liftLayer", "selectionOverlay", "pageContextMenu", "toolCursor", "trashCan"}) {
          QVERIFY2(window->findChild<QObject*>(QString::fromLatin1(name)) != nullptr, name);
      }
      auto* lift = window->findChild<QQuickItem*>(QStringLiteral("liftLayer"));
      auto* ink = window->findChild<QQuickItem*>(QStringLiteral("ink"));
      QVERIFY(lift && ink && lift->parentItem() == ink->parentItem());
      QVERIFY(lift->z() <= ink->z());
  }
  void everyLot3CommandHasAHandler()
  {
      QQuickWindow* window = loadMainWindow();
      auto* commands = window->findChild<ui::Commands*>();
      QVERIFY(commands);
      const QStringList lot3 = {"tool.select", "tool.highlighter", "tool.eraser", "edit.undo", "edit.redo",
                                "edit.delete", "edit.select-all", "clear.annotations", "clear.objects",
                                "clear.grid", "clear.background", "clear.page", "page.revert", "view.trash",
                                "object.translucency", "object.enlarge", "object.reduce"};
      for (const QString& id : lot3) {
          QVERIFY2(commands->registry().hasHandler(id), qPrintable(id));
      }
  }
  ```
  (Nom de l'aide de chargement : celui du fichier ; l'ordre d'empilement se vérifie aussi par l'ordre des enfants si
  `z` vaut 0 partout.) Commande ciblée → rouge.
- [ ] **Étape 2 : `Main.qml`.** Après `appDocument` :
  ```qml
  Tools {
      id: appTools
      objectName: "appTools"
      commands: appCommands
      documentController: appDocument
      view: pageArea.view
      liftLayer: pageArea.liftLayer
  }
  Connections {
      target: appDocument
      function onEditAboutToApply() { pageArea.view.finishStroke() }
      function onUndoChanged() { root.syncEditCommands() }
      function onDocumentChanged() { root.syncEditCommands() }
  }
  function syncEditCommands() {
      appCommands.setEnabled("edit.undo", appDocument.canUndo)
      appCommands.setEnabled("edit.redo", appDocument.canRedo)
      appCommands.setEnabled("page.revert", appDocument.canRevertPage)
  }
  ```
  `pageArea` reçoit `tools: appTools` (propriété ajoutée à `PageArea.qml`). Gestionnaires, à la suite des existants :
  ```qml
  appCommands.setHandler("tool.select", function () { appTools.activate("select") })
  appCommands.setHandler("tool.highlighter", function () { appTools.activate("highlighter") })
  appCommands.setHandler("tool.eraser", function () { appTools.activate("eraser") })
  appCommands.setHandler("edit.undo", function () { appDocument.undo() })
  appCommands.setHandler("edit.redo", function () { appDocument.redo() })
  appCommands.setHandler("edit.delete", function () { appTools.deleteSelection() })
  appCommands.setHandler("edit.select-all", function () { appTools.selectAll() })
  appCommands.setHandler("clear.annotations", function () { appDocument.clearPage(0) })
  appCommands.setHandler("clear.objects", function () { appDocument.clearPage(1) })
  appCommands.setHandler("clear.grid", function () { appDocument.clearPage(2) })
  appCommands.setHandler("clear.background", function () { appDocument.clearPage(3) })
  appCommands.setHandler("clear.page", function () { appDocument.clearPage(4) })
  appCommands.setHandler("page.revert", function () { appDocument.revertPage() })
  appCommands.setHandler("view.trash", function () { appCommands.setChecked("view.trash", !appCommands.isChecked("view.trash")) })
  appCommands.setHandler("object.translucency", function (value) { if (value !== undefined) appTools.setSelectionTranslucency(value) })
  appCommands.setHandler("object.enlarge", function () { appTools.enlargeSelection() })
  appCommands.setHandler("object.reduce", function () { appTools.reduceSelection() })
  appCommands.setChecked("view.trash", true)
  root.syncEditCommands()
  ```
  (`object.translucency` sans argument ouvrira la réglette : L3-T15 le branche dans `ObjectBar.qml`.)
- [ ] **Étape 3 : `PageArea.qml`.** Propriétés `property Tools tools: null`, `property alias liftLayer: liftLayerItem` ;
  dans `pageContent`, entre `pendingLayer` (L10-T4) et `inkLayer` :
  ```qml
  LiftLayer {
      id: liftLayerItem
      objectName: "liftLayer"
      anchors.fill: parent
      fixedPage: fixedPageItem
  }
  ```
  `view.tools: area.tools` ; hors de `view`, au-dessus d'elle et sous les barres de défilement :
  `SelectionOverlay { objectName: "selectionOverlay"; anchors.fill: parent }`,
  `ToolCursor { id: toolCursor; objectName: "toolCursor"; anchors.fill: parent }`,
  `PageContextMenu { id: pageContextMenu; objectName: "pageContextMenu" }` ; le `HoverHandler` existant reçoit
  `cursorShape: toolCursor.cursorShape` ; un `TapHandler { acceptedButtons: Qt.RightButton; onTapped: (point) =>
  pageContextMenu.popupAt(point.position.x, point.position.y) }`.
- [ ] **Étape 4 : `ToolPalette.qml`.** Propriété `required property Tools tools` (passée par `Main.qml`) ; sous le
  `Flickable`, ancrée en bas de la palette : `TrashCan { id: trash; objectName: "trashCan"; visible:
  paletteRoot.commands.revision >= 0 && paletteRoot.commands.isChecked("view.trash") }` (la première clause relit
  l'état à chaque `revision`), hauteur réservée en bas du `Flickable`.
- [ ] **Étape 5 : vert et preuves.** Commande ciblée → vert ; `format.ps1` ; preuve de tâche ; `premiers-pas` hors
  écran ; banc sans écran ; `NOMS OK`.
- [ ] **Étape 6 : commit.**
  ```powershell
  git -C <wt> add ui/qml/Main.qml ui/qml/PageArea.qml ui/qml/ToolPalette.qml tests/tst_ui_shell.cpp
  git -C <wt> commit -m "Lot 3 : coquille QML réservée (outils, couche soulevée, surcouches, corbeille, gestionnaires) (L3-T2)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t3"></a>
### Tâche L3-T3 — `core` : désignation et géométrie de la gomme

**Objectif.** Remplir `hittest.*` et `erasegeometry.*` (en-têtes figés par L3-T1) : quel objet est sous le pointeur,
quels objets touche un rectangle, comment un disque coupe la ligne centrale d'un trait (algorithme de l'ancien
dépôt, coupe d'une ligne brisée par un disque, réécrit par nous).

**Fichiers.** Modifier (talons) : `core/src/hittest.cpp`, `core/src/erasegeometry.cpp`, `tests/tst_core_hittest.cpp`,
`tests/tst_core_erase.cpp` ; en-têtes `core/include/actcraft/core/hittest.h`, `erasegeometry.h` : membres privés ou
fonctions internes seulement.

**Test et commande bornée.** Commande ciblée `tst_core_hittest|tst_core_erase` ; preuve de tâche N ≥ 111 ; fonctions
des deux tests en `PASS`, aucun `SKIP`.

**Interdits.** Aucune dépendance à QtGui (`core/`). Pas de `std::visit` direct sur `PageObject` (passer par
`body()` et `std::get_if`). Ne pas lisser la courbe ici (rendu : L3-T11).

**Terminé quand.** Les huit fonctions de test ci-dessous passent ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** `tests/tst_core_hittest.cpp` :
  ```cpp
  #include <actcraft/core/edits.h>
  #include <actcraft/core/hittest.h>

  #include <QTest>

  using namespace actcraft::core;

  namespace {
  PageObject strokeObject(ObjectId id, std::vector<StrokePoint> points, double width)
  {
      Stroke s;
      s.points = std::move(points);
      s.baseWidth = width;
      PageObject o{s};
      o.common.id = id;
      o.common.layer = Layer::Top;
      return o;
  }
  PageObject rectObject(ObjectId id, RectF bounds, Layer layer = Layer::Middle)
  {
      Shape shape;
      shape.bounds = bounds;
      PageObject o{shape};
      o.common.id = id;
      o.common.layer = layer;
      return o;
  }
  } // namespace

  class TstCoreHittest : public QObject
  {
      Q_OBJECT

  private slots:
      void topmostObjectWins()
      {
          Page page;
          insertAtTopOfLayer(page, rectObject(1, {100, 100, 200, 200}));
          insertAtTopOfLayer(page, rectObject(2, {150, 150, 200, 200}));
          QCOMPARE(objectAt(page, {180, 180}, 0).value_or(0), ObjectId{2});
          QCOMPARE(objectAt(page, {120, 120}, 0).value_or(0), ObjectId{1});
          QVERIFY(!objectAt(page, {10, 10}, 0).has_value());
      }
      void strokeHitUsesWidthAndTolerance()
      {
          Page page;
          insertAtTopOfLayer(page, strokeObject(7, {{0, 100, 0.5f}, {400, 100, 0.5f}}, 10));
          QCOMPARE(objectAt(page, {200, 104}, 0).value_or(0), ObjectId{7}); // dans la demi-épaisseur (5)
          QVERIFY(!objectAt(page, {200, 112}, 0).has_value());
          QCOMPARE(objectAt(page, {200, 112}, 8).value_or(0), ObjectId{7}); // grâce à la tolérance
          QVERIFY(!objectAt(page, {200, 150}, 8).has_value());              // dans l'emprise, loin de l'encre
      }
      void rectangleTouchingSelects()
      {
          Page page;
          insertAtTopOfLayer(page, rectObject(1, {100, 100, 50, 50}));
          insertAtTopOfLayer(page, rectObject(2, {400, 400, 50, 50}));
          insertAtTopOfLayer(page, strokeObject(3, {{120, 300, 0.5f}, {600, 300, 0.5f}}, 4));
          const auto ids = objectsTouching(page, normalizedRect({140, 140}, {130, 310}));
          QCOMPARE(ids, (std::vector<ObjectId>{1, 3})); // touche le coin du 1 et le trait, pas le 2
      }
      void annotationsAreTopLayerStrokes()
      {
          QVERIFY(isAnnotation(strokeObject(1, {{0, 0, 0.5f}, {1, 1, 0.5f}}, 2)));
          PageObject moved = strokeObject(2, {{0, 0, 0.5f}, {1, 1, 0.5f}}, 2);
          moved.common.layer = Layer::Middle;
          QVERIFY(!isAnnotation(moved));
          QVERIFY(!isAnnotation(rectObject(3, {0, 0, 1, 1}, Layer::Top)));
      }
  };

  QTEST_GUILESS_MAIN(TstCoreHittest)
  #include "tst_core_hittest.moc"
  ```
  `tests/tst_core_erase.cpp` :
  ```cpp
  #include <actcraft/core/erasegeometry.h>

  #include <QTest>

  using namespace actcraft::core;

  namespace {
  std::vector<StrokePoint> line(double y = 100)
  {
      return {{0, y, 0.2f}, {100, y, 0.4f}, {200, y, 0.6f}, {300, y, 0.8f}};
  }
  } // namespace

  class TstCoreErase : public QObject
  {
      Q_OBJECT

  private slots:
      void middleCutSplitsInTwo()
      {
          const StrokeCut cut = cutByDisk(line(), {150, 100}, 20);
          QVERIFY(cut.touched);
          QCOMPARE(cut.pieces.size(), std::size_t{2});
          QCOMPARE(cut.pieces[0].back().x, 130.0);
          QCOMPARE(cut.pieces[1].front().x, 170.0);
          QVERIFY(qAbs(cut.pieces[0].back().pressure - 0.52f) < 1e-4f); // interpolée entre 0.4 et 0.6
      }
      void endCutShortens()
      {
          const StrokeCut cut = cutByDisk(line(), {300, 100}, 50);
          QVERIFY(cut.touched);
          QCOMPARE(cut.pieces.size(), std::size_t{1});
          QCOMPARE(cut.pieces[0].back().x, 250.0);
      }
      void missLeavesIntact()
      {
          const StrokeCut cut = cutByDisk(line(), {150, 200}, 20);
          QVERIFY(!cut.touched);
          QCOMPARE(cut.pieces.size(), std::size_t{1});
          QCOMPARE(cut.pieces[0].size(), std::size_t{4});
      }
      void wholeStrokeInsideIsRemoved()
      {
          const StrokeCut cut = cutByDisk({{10, 10, 0.5f}, {12, 10, 0.5f}}, {11, 10}, 20);
          QVERIFY(cut.touched);
          QVERIFY(cut.pieces.empty());
      }
      void sweepDoesNotSkip()
      {
          // Deux positions de gomme éloignées de part et d'autre d'un trait vertical : le balayage le coupe.
          const std::vector<StrokePoint> vertical = {{150, 0, 0.5f}, {150, 300, 0.5f}};
          QVERIFY(!cutByDisk(vertical, {100, 150}, 10).touched);
          QVERIFY(!cutByDisk(vertical, {200, 150}, 10).touched);
          const StrokeCut cut = cutBySweep(vertical, {100, 150}, {200, 150}, 10);
          QVERIFY(cut.touched);
          QCOMPARE(cut.pieces.size(), std::size_t{2});
      }
      void pointsOnPageApplyTheTransform()
      {
          Stroke s;
          s.points = {{0, 0, 0.5f}, {10, 0, 0.5f}};
          PageObject o{s};
          o.common.transform = translation(100, 50);
          const auto pts = strokePointsOnPage(o);
          QCOMPARE(pts.size(), std::size_t{2});
          QCOMPARE(pts[1].x, 110.0);
          QCOMPARE(pts[1].y, 50.0);
      }
  };

  QTEST_GUILESS_MAIN(TstCoreErase)
  #include "tst_core_erase.moc"
  ```
  Commande ciblée → rouge (talons).
- [ ] **Étape 2 : `hittest.cpp`.** `isAnnotation` : `std::get_if<Stroke>(&body(o))` et `o.common.layer ==
  Layer::Top` ; `isSelectable` : `o.common.visible` ; `objectAt` : parcours de `page.objects` à rebours ; pour un
  trait, points mappés par `common.transform.map`, distance point–segment ≤ `strokeWidthAt(baseWidth, pression
  max des deux bouts) / 2 + tolerance` ; sinon `boundsOnPage(o)` agrandie de `tolerance` contient le point ;
  `objectsTouching` : `rectsTouch(boundsOnPage(o), rect)` dans l'ordre ; `rectsTouch` bords compris.
- [ ] **Étape 3 : `erasegeometry.cpp`.** `cutByDisk` : pour chaque segment [p0, p1], intersection paramétrique avec le
  cercle (t1, t2 solutions de |p0 + t(p1 − p0) − c|² = r², bornées à [0, 1]) ; la partie [ta, tb] intérieure est
  retirée, le morceau courant est fermé au point ta (pression interpolée) et un nouveau commence au point tb ;
  segment de longueur nulle : retiré si son point est intérieur ; morceaux d'un point abandonnés. `cutBySweep` :
  `n = max(1, ceil(distance(from, to) / (radius / 2)))` centres régulièrement espacés de `from` à `to` compris,
  appliqués successivement à chaque morceau. `strokePointsOnPage` : points mappés, pression gardée.
- [ ] **Étape 4 : vert, preuves, commit.** Commande ciblée → vert ; `format.ps1` ; preuve de tâche ; fonctions en
  `PASS`.
  ```powershell
  git -C <wt> add core/src/hittest.cpp core/src/erasegeometry.cpp core/include/actcraft/core/hittest.h core/include/actcraft/core/erasegeometry.h tests/tst_core_hittest.cpp tests/tst_core_erase.cpp
  git -C <wt> commit -m "core : désignation des objets et coupe des traits par la gomme (L3-T3)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t10"></a>
### Tâche L3-T10 — `render` : couche soulevée `LiftLayerItem`

**Objectif.** Dessiner, pendant un geste, les copies des objets soulevés **exactement comme la page fixe** les
dessinerait (mêmes moteurs, même ordre, même translucidité), sous une transformation supplémentaire qui se change
sans reconstruire de nœud (glisser fluide). Patron évident à **ne pas** reprendre : redessiner les objets par un
`QQuickPaintedItem` (texture repeinte à chaque mouvement) ou reconstruire les nœuds à chaque `setExtraTransform`.

**Fichiers.** Modifier (talons) : `render/src/liftlayeritem.cpp`, `tests/tst_render_liftlayer.cpp` ; en-tête
`render/include/actcraft/render/liftlayeritem.h` : membres privés seulement.

**Test et commande bornée.** Commande ciblée `tst_render_liftlayer` ; preuve de tâche N ≥ 111, fonctions en `PASS` ;
banc sans écran `content_check.ok = True`.

**Interdits.** Ne pas modifier `fixedpageitem.*`, `pendinglayeritem.*`, `objectnodes.*` (lecture seule ; si la
fabrique manque d'un point d'accès, l'écrire dans le rapport et s'arrêter : relecture du plan). État lu par
`updatePaintNode` écrit sur le fil principal seulement. Aucune API privée de Qt.

**Terminé quand.** `drawsLikeTheFixedPage`, `transformDoesNotRebuild`, `clearEmpties`, `translucencyKept` passent ;
banc sans écran joint.

**Étapes.**
- [ ] **Étape 1 : lire le patron.** `graphify query "PendingLayerItem updatePaintNode createObjectNode"` ; lire
  `render/src/pendinglayeritem.cpp` (accès aux moteurs et au contexte de la page fixe, `builtCount`) : la couche
  soulevée suit le même accès, avec un `QSGTransformNode` racine dont seule la matrice change.
- [ ] **Étape 2 : tests rouges.** `tests/tst_render_liftlayer.cpp`, sur le modèle de `tests/tst_render_pendinglayer.cpp`
  (fenêtre hors écran, rendu logiciel, `grabWindow`) :
  - `drawsLikeTheFixedPage` : une page fixe vide et une couche soulevée qui montre un trait rouge et un rectangle
    bleu ; capture A ; puis la même page fixe qui contient ces deux objets, couche vide ; capture B ; A et B
    identiques au pixel (tolérance 0) sur la zone de la page.
  - `transformDoesNotRebuild` : `setObjects` (2 objets), une image, `builtCount() == 2` ; dix
    `setExtraTransform(translation(10 * k, 0))` avec une image chacune : `builtCount()` toujours 2, et le pixel du
    trait suit le décalage (contrôle au pixel à x + 100).
  - `clearEmpties` : `clear()` → `objectCount() == 0`, pixel du trait blanc à l'image suivante.
  - `translucencyKept` : objet de translucidité 128 : même pixel que dans la page fixe.
- [ ] **Étape 3 : implémentation.** `setObjects` : copie gardée, drapeau « reconstruire », `update()` ;
  `setExtraTransform` : matrice gardée, drapeau « matrice seule », `update()` ; `updatePaintNode` : racine
  `QSGTransformNode`, enfants construits par `createObjectNode` (contexte et moteurs de `fixedPage`) seulement au
  drapeau « reconstruire » (`builtCount` += nombre construit), matrice posée à chaque passage.
- [ ] **Étape 4 : vert, preuves, commit.**
  ```powershell
  git -C <wt> add render/src/liftlayeritem.cpp render/include/actcraft/render/liftlayeritem.h tests/tst_render_liftlayer.cpp
  git -C <wt> commit -m "render : couche soulevée, dessin identique à la page fixe, transformation sans reconstruction (L3-T10)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t4"></a>
### Tâche L3-T4 — `core` : commandes d'édition du lot et changements vus par le cache après un soulèvement

**Socle.** Le socle L0-S pose **une seule** `ReplaceObjectsCommand` (rang de l'original, identifiants stables),
commune à la gomme et à la reconnaissance du lot 7. Quand son plan sera écrit, cette section est amendée :
`ReplaceStrokesCommand` n'est pas écrite, ses tests (`replaceStrokesKeepsRankAndIds`,
`erasePiecesInsertedNoRemoveOfLifted`) portent sur la commande du socle, et L3-T7 l'emploie. D'ici là, ne pas
exécuter la partie `ReplaceStrokesCommand` de cette tâche.

**Objectif.** Remplir `objectedits.*` et `lift.*`. Invariant à tenir (le patron évident est le mauvais) : **le cache
ne reçoit jamais `remove` ni `invalidate` d'un objet qu'il n'a plus**, et reçoit `insert` pour chaque objet soulevé
qui revient. Patron à ne pas reprendre : rejouer au dépôt les `Changes` de la commande tels quels (un
`ObjectChanged` d'un objet soulevé arriverait au cache qui ne l'a plus ; un `ObjectRemoved` de trait gommé y
arriverait deux fois). Les décisions D8, D9 et D10 de L3-T0 s'appliquent (amendement éventuel de cette section).

**Fichiers.** Modifier (talons) : `core/src/objectedits.cpp`, `core/src/lift.cpp`, `tests/tst_core_objectedits.cpp`,
`tests/tst_core_lift.cpp` ; en-têtes `objectedits.h`, `lift.h` : membres privés seulement.

**Test et commande bornée.** Commande ciblée `tst_core_objectedits|tst_core_lift` ; preuve de tâche N ≥ 111 ;
fonctions en `PASS` ; **mutation** : dans `changesAfterLift`, laisser passer les `ObjectChanged` des objets soulevés
→ `liftThenDropMoveYieldsInsertOnly` rouge, puis `git -C <wt> checkout -- core/src/lift.cpp`.

**Interdits.** Aucun changement de `core::PageCache`, `applyToCache`, `UndoStack`, `edits.*`. Une commande sans
effet ne produit aucun changement (le contrôleur ne l'empile pas). Identifiants des morceaux stables entre redo.

**Terminé quand.** Les tests ci-dessous passent ; mutation rouge puis remise.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** `tests/tst_core_lift.cpp` :
  ```cpp
  #include <actcraft/core/document.h>
  #include <actcraft/core/edits.h>
  #include <actcraft/core/lift.h>
  #include <actcraft/core/objectedits.h>

  #include <QTest>

  using namespace actcraft::core;

  namespace {
  Document twoStrokes(ObjectId& a, ObjectId& b)
  {
      Document doc;
      Stroke s;
      s.points = {{100, 100, 0.5f}, {300, 100, 0.5f}};
      Changes ignored;
      auto annotation = [](Stroke st) {
          PageObject o{std::move(st)};
          o.common.layer = Layer::Top; // annotation (F-230)
          return o;
      };
      AddObjectCommand first(0, annotation(s));
      first.redo(doc, ignored);
      a = first.id();
      s.points = {{100, 200, 0.5f}, {300, 200, 0.5f}};
      AddObjectCommand second(0, annotation(s));
      second.redo(doc, ignored);
      b = second.id();
      return doc;
  }
  int count(const Changes& changes, Change::Kind kind, ObjectId id)
  {
      int n = 0;
      for (const Change& c : changes) {
          n += (c.kind == kind && c.object == id) ? 1 : 0;
      }
      return n;
  }
  } // namespace

  class TstCoreLift : public QObject
  {
      Q_OBJECT

  private slots:
      void takeAndRestoreKeepRanks()
      {
          ObjectId a = 0, b = 0;
          Document doc = twoStrokes(a, b);
          const std::uint64_t before = fingerprint(doc.page(0));
          auto lifted = takeObjects(doc.page(0), {a});
          QCOMPARE(lifted.size(), std::size_t{1});
          QCOMPARE(lifted[0].index, std::size_t{0});
          QCOMPARE(doc.page(0).objects.size(), std::size_t{1});
          const Changes up = liftChanges(0, lifted);
          QCOMPARE(count(up, Change::Kind::ObjectRemoved, a), 1);
          restoreObjects(doc.page(0), lifted);
          QCOMPARE(fingerprint(doc.page(0)), before);
      }
      void liftThenDropMoveYieldsInsertOnly()
      {
          ObjectId a = 0, b = 0;
          Document doc = twoStrokes(a, b);
          auto lifted = takeObjects(doc.page(0), {a});
          restoreObjects(doc.page(0), lifted); // dépôt : originaux remis, puis la commande
          TransformObjectsCommand move(0, {a}, translation(50, 0));
          Changes command;
          move.redo(doc, command);
          QCOMPARE(count(command, Change::Kind::ObjectChanged, a), 1);
          const Changes cache = changesAfterLift(0, lifted, command, doc.page(0));
          QCOMPARE(count(cache, Change::Kind::ObjectInserted, a), 1);
          QCOMPARE(count(cache, Change::Kind::ObjectChanged, a), 0);
          QCOMPARE(count(cache, Change::Kind::ObjectRemoved, a), 0);
          QCOMPARE(cache.size(), std::size_t{1});
      }
      void liftThenCancelReinserts()
      {
          ObjectId a = 0, b = 0;
          Document doc = twoStrokes(a, b);
          auto lifted = takeObjects(doc.page(0), {a, b});
          restoreObjects(doc.page(0), lifted);
          const Changes cache = changesAfterLift(0, lifted, {}, doc.page(0));
          QCOMPARE(count(cache, Change::Kind::ObjectInserted, a), 1);
          QCOMPARE(count(cache, Change::Kind::ObjectInserted, b), 1);
          QCOMPARE(cache.size(), std::size_t{2});
      }
      void erasePiecesInsertedNoRemoveOfLifted()
      {
          ObjectId a = 0, b = 0;
          Document doc = twoStrokes(a, b);
          auto lifted = takeObjects(doc.page(0), {a});
          restoreObjects(doc.page(0), lifted);
          Stroke left;
          left.points = {{100, 100, 0.5f}, {180, 100, 0.5f}};
          Stroke right;
          right.points = {{220, 100, 0.5f}, {300, 100, 0.5f}};
          ReplaceStrokesCommand erase(0, {StrokeReplacement{a, {left, right}}});
          Changes command;
          erase.redo(doc, command);
          const Changes cache = changesAfterLift(0, lifted, command, doc.page(0));
          QCOMPARE(count(cache, Change::Kind::ObjectRemoved, a), 0);
          QCOMPARE(count(cache, Change::Kind::ObjectInserted, erase.pieceIds()[0]), 1);
          QCOMPARE(count(cache, Change::Kind::ObjectInserted, erase.pieceIds()[1]), 1);
          QCOMPARE(cache.size(), std::size_t{2});
      }
      void otherObjectsChangesPassThrough()
      {
          ObjectId a = 0, b = 0;
          Document doc = twoStrokes(a, b);
          auto lifted = takeObjects(doc.page(0), {a});
          restoreObjects(doc.page(0), lifted);
          RemoveObjectsCommand both(0, {a, b});
          Changes command;
          both.redo(doc, command);
          const Changes cache = changesAfterLift(0, lifted, command, doc.page(0));
          QCOMPARE(count(cache, Change::Kind::ObjectRemoved, a), 0); // le cache ne l'a plus
          QCOMPARE(count(cache, Change::Kind::ObjectRemoved, b), 1); // b n'était pas soulevé
          QCOMPARE(count(cache, Change::Kind::ObjectInserted, a), 0); // a n'est plus dans la page
      }
  };

  QTEST_GUILESS_MAIN(TstCoreLift)
  #include "tst_core_lift.moc"
  ```
  `tests/tst_core_objectedits.cpp` (mêmes aides `twoStrokes` et `count`, recopiées dans le fichier) :
  ```cpp
  void transformUndoRestores()
  {
      ObjectId a = 0, b = 0;
      Document doc = twoStrokes(a, b);
      const std::uint64_t before = fingerprint(doc.page(0));
      TransformObjectsCommand move(0, {a, b}, translation(10, 20));
      Changes c;
      move.redo(doc, c);
      QCOMPARE(c.size(), std::size_t{2});
      QCOMPARE(doc.page(0).objects[0].common.transform.dx, 10.0);
      c.clear();
      move.undo(doc, c);
      QCOMPARE(fingerprint(doc.page(0)), before);
  }
  void removeRestoresAtSameDepth()
  {
      ObjectId a = 0, b = 0;
      Document doc = twoStrokes(a, b);
      RemoveObjectsCommand remove(0, {a});
      Changes c;
      remove.redo(doc, c);
      QCOMPARE(doc.page(0).objects.size(), std::size_t{1});
      c.clear();
      remove.undo(doc, c);
      QCOMPARE(doc.page(0).objects[0].common.id, a);
      QCOMPARE(count(c, Change::Kind::ObjectInserted, a), 1);
  }
  void replaceStrokesKeepsRankAndIds()
  {
      ObjectId a = 0, b = 0;
      Document doc = twoStrokes(a, b);
      Stroke left;
      left.points = {{100, 100, 0.5f}, {180, 100, 0.5f}};
      Stroke right;
      right.points = {{220, 100, 0.5f}, {300, 100, 0.5f}};
      ReplaceStrokesCommand erase(0, {StrokeReplacement{a, {left, right}}});
      Changes c;
      erase.redo(doc, c);
      const auto ids = erase.pieceIds();
      QCOMPARE(ids.size(), std::size_t{2});
      QCOMPARE(doc.page(0).objects.size(), std::size_t{3});
      QCOMPARE(doc.page(0).objects[0].common.id, ids[0]); // au rang de l'original
      QCOMPARE(doc.page(0).objects[1].common.id, ids[1]);
      QCOMPARE(doc.page(0).objects[2].common.id, b);
      QCOMPARE(doc.page(0).objects[0].common.layer, Layer::Top);
      c.clear();
      erase.undo(doc, c);
      QCOMPARE(doc.page(0).objects[0].common.id, a);
      c.clear();
      erase.redo(doc, c);
      QCOMPARE(erase.pieceIds(), ids); // mêmes identifiants au rétablissement
  }
  void clearAnnotationsKeepsShapes()
  {
      ObjectId a = 0, b = 0;
      Document doc = twoStrokes(a, b);
      Shape shape;
      shape.bounds = {10, 10, 50, 50};
      Changes c;
      AddObjectCommand add(0, PageObject{shape});
      add.redo(doc, c);
      c.clear();
      ClearPageCommand clear(0, ClearKind::Annotations);
      clear.redo(doc, c);
      QCOMPARE(doc.page(0).objects.size(), std::size_t{1});
      QVERIFY(std::holds_alternative<Shape>(body(doc.page(0).objects[0])));
      QCOMPARE(c.size(), std::size_t{1});
      QCOMPARE(c[0].kind, Change::Kind::PageChanged);
      c.clear();
      clear.undo(doc, c);
      QCOMPARE(doc.page(0).objects.size(), std::size_t{3});
  }
  void clearNothingChangesNothing()
  {
      Document doc;
      ClearPageCommand clear(0, ClearKind::Annotations);
      Changes c;
      clear.redo(doc, c);
      QVERIFY(c.empty());
  }
  void revertPageIsOneStep()
  {
      ObjectId a = 0, b = 0;
      Document doc = twoStrokes(a, b);
      Page saved; // page vierge 1024 × 768 : un paperboard jamais enregistré (F-131)
      const std::uint64_t before = fingerprint(doc.page(0));
      UndoStack stack;
      stack.push(doc, std::make_unique<ReplacePageCommand>(0, saved));
      QVERIFY(doc.page(0).objects.empty());
      QCOMPARE(stack.count(), std::size_t{1});
      stack.undo(doc);
      QCOMPARE(fingerprint(doc.page(0)), before);
  }
  void translucencyDragMerges()
  {
      ObjectId a = 0, b = 0;
      Document doc = twoStrokes(a, b);
      UndoStack stack;
      stack.push(doc, std::make_unique<SetTranslucencyCommand>(0, std::vector<ObjectId>{a}, std::uint8_t{50}));
      stack.push(doc, std::make_unique<SetTranslucencyCommand>(0, std::vector<ObjectId>{a}, std::uint8_t{120}));
      QCOMPARE(stack.count(), std::size_t{1});
      QCOMPARE(doc.page(0).objects[0].common.translucency, std::uint8_t{120});
      stack.undo(doc);
      QCOMPARE(doc.page(0).objects[0].common.translucency, std::uint8_t{0});
  }
  void pivotsAreTheSelectionCenter()
  {
      ObjectId a = 0, b = 0;
      Document doc = twoStrokes(a, b);
      const PointF c = selectionCenter(doc.page(0), {a, b});
      QCOMPARE(c.x, 200.0);
      QVERIFY(qAbs(c.y - 150.0) < 3.0); // emprises élargies de la demi-épaisseur
      const Transform t = scalingAbout({100, 100}, 2, 2);
      QCOMPARE(t.map({100, 100}).x, 100.0);
      QCOMPARE(t.map({110, 100}).x, 120.0);
  }
  ```
  Commande ciblée → rouge.
- [ ] **Étape 2 : `objectedits.cpp`.** Chaque commande relève son état d'avant au premier `redo` ; `ObjectChanged`,
  `ObjectRemoved`, `ObjectInserted` avec `area` = `boundsOnPage` (réunion avant ∪ après pour un changement) ;
  `ReplaceStrokesCommand::redo` tire les identifiants des morceaux par `Document::allocateId()` au premier passage
  seulement, insère les morceaux au rang de l'original (commun recopié, identifiant et transformation remis) ;
  `ClearPageCommand` garde la page entière d'avant, ne produit rien si rien ne change ; `SetTranslucencyCommand::mergeWith`
  : même famille, même page, mêmes identifiants → prend la valeur de l'autre, garde son état d'avant.
- [ ] **Étape 3 : `lift.cpp`.** `takeObjects` : rangs relevés puis retraits du plus haut au plus bas ;
  `restoreObjects` : insertions par rangs croissants ; `changesAfterLift` : ensemble des identifiants soulevés ;
  parcourir `command` : changement d'objet soulevé → ignoré ; autre → recopié ; puis, pour chaque objet soulevé
  encore dans `pageAfter`, un `ObjectInserted` (zone `boundsOnPage`), dans l'ordre de la page.
- [ ] **Étape 4 : vert, mutation, preuves, commit.**
  ```powershell
  git -C <wt> add core/src/objectedits.cpp core/src/lift.cpp core/include/actcraft/core/objectedits.h core/include/actcraft/core/lift.h tests/tst_core_objectedits.cpp tests/tst_core_lift.cpp
  git -C <wt> commit -m "core : commandes d'édition du lot 3 et changements vus par le cache après un soulèvement (L3-T4)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t5"></a>
### Tâche L3-T5 — `input` : routeur unique des contacts et geste à deux doigts

**Objectif.** DA-A1 : une seule politique décide, à l'appui, où va chaque contact (outil, geste, rien) et à quel
utilisateur il appartient ; la décision tient jusqu'au relâché. Patron évident à **ne pas** reprendre : des
`if (pointCount() == 2)` répartis dans la vue (ce que fait `PageView::touchEvent` aujourd'hui), qui obligeraient le
lot 11 à réécrire la vue pour chaque mode du toucher. Décision D12 de L3-T0 (seuils) appliquée.

**Fichiers.** Modifier : `input/include/actcraft/input/sample.h` (deux champs à la fin) ; talons
`input/src/routing.cpp`, `input/src/gesture.cpp`, `tests/tst_input_routing.cpp`, `tests/tst_input_gesture.cpp` ;
en-têtes `routing.h`, `gesture.h` : membres privés seulement.

**Test et commande bornée.** Commande ciblée `tst_input_routing|tst_input_gesture|tst_input` ; preuve de tâche
N ≥ 111 ; fonctions en `PASS` ; `tst_input` Passed (champs ajoutés sans changer les valeurs existantes).

**Interdits.** `input/` ne dépend que de `core/` (règle de modularité) ; aucune dépendance à Qt Quick ; rien dans
`ui/` (L3-T13).

**Terminé quand.** Les tests ci-dessous passent ; `sample.h` a `pointer` et `user` en fin de structure.

**Étapes.**
- [ ] **Étape 1 : champs de l'échantillon.** À la fin de `struct Sample` :
  ```cpp
      // Ajoutés au lot 3 (DA-A1), à la fin : contact (doigt : identifiant du point Qt + 1 ; souris et stylet : 0) et
      // utilisateur auquel le routeur l'a rattaché.
      std::uint64_t pointer = 0;
      std::uint8_t user = 0;
  ```
- [ ] **Étape 2 : tests rouges.** `tests/tst_input_routing.cpp` :
  ```cpp
  #include <actcraft/input/routing.h>

  #include <QTest>

  using namespace actcraft::input;

  namespace {
  Sample s(DeviceKind device, Phase phase, std::uint64_t pointer = 0)
  {
      Sample out;
      out.device = device;
      out.phase = phase;
      out.pointer = pointer;
      return out;
  }
  class AlwaysIgnore final : public RoutingPolicy
  {
  public:
      Decision decide(const Sample&, const RouterState&) const override { return {}; }
  };
  } // namespace

  class TstInputRouting : public QObject
  {
      Q_OBJECT

  private slots:
      void stylusAndMouseGoToTool()
      {
          PointerRouter router;
          QCOMPARE(router.route(s(DeviceKind::Stylus, Phase::Press)).route, Route::Tool);
          QCOMPARE(router.route(s(DeviceKind::Stylus, Phase::Release)).route, Route::Tool);
          QCOMPARE(router.route(s(DeviceKind::Mouse, Phase::Press)).route, Route::Tool);
      }
      void oneFingerGoesToTool()
      {
          PointerRouter router;
          QCOMPARE(router.route(s(DeviceKind::Touch, Phase::Press, 1)).route, Route::Tool);
          QCOMPARE(router.route(s(DeviceKind::Touch, Phase::Move, 1)).route, Route::Tool);
      }
      void secondFingerCancelsToolAndStartsGesture()
      {
          PointerRouter router;
          router.route(s(DeviceKind::Touch, Phase::Press, 1));
          const Decision second = router.route(s(DeviceKind::Touch, Phase::Press, 2));
          QCOMPARE(second.route, Route::Gesture);
          QVERIFY(second.endToolContacts);
          QCOMPARE(router.route(s(DeviceKind::Touch, Phase::Move, 1)).route, Route::Gesture); // le premier y passe
          QCOMPARE(router.gestureTouches(), 2);
          QCOMPARE(router.route(s(DeviceKind::Touch, Phase::Press, 3)).route, Route::Ignore); // troisième doigt
      }
      void palmIgnoredWhileStylusDown()
      {
          PointerRouter router;
          router.route(s(DeviceKind::Stylus, Phase::Press));
          QCOMPARE(router.route(s(DeviceKind::Touch, Phase::Press, 5)).route, Route::Ignore);
          QCOMPARE(router.route(s(DeviceKind::Touch, Phase::Move, 5)).route, Route::Ignore);
          router.route(s(DeviceKind::Stylus, Phase::Release));
          QCOMPARE(router.route(s(DeviceKind::Touch, Phase::Press, 6)).route, Route::Tool);
      }
      void decisionHeldForWholeContact()
      {
          PointerRouter router;
          router.route(s(DeviceKind::Stylus, Phase::Press));
          router.route(s(DeviceKind::Touch, Phase::Press, 5)); // ignoré : paume
          router.route(s(DeviceKind::Stylus, Phase::Release));
          QCOMPARE(router.route(s(DeviceKind::Touch, Phase::Move, 5)).route, Route::Ignore); // toujours ignoré
          QCOMPARE(router.route(s(DeviceKind::Touch, Phase::Release, 5)).route, Route::Ignore);
          QCOMPARE(router.route(s(DeviceKind::Touch, Phase::Move, 5)).route, Route::Ignore); // oublié : inconnu
      }
      void everyContactHasAUser()
      {
          PointerRouter router;
          QCOMPARE(router.route(s(DeviceKind::Stylus, Phase::Press)).user, kFirstUser);
          QCOMPARE(router.route(s(DeviceKind::Touch, Phase::Press, 1)).user, kFirstUser);
      }
      void policyIsReplaceable()
      {
          PointerRouter router;
          router.route(s(DeviceKind::Touch, Phase::Press, 1)); // décidé par la politique d'origine
          router.setPolicy(std::make_unique<AlwaysIgnore>());
          QCOMPARE(router.route(s(DeviceKind::Touch, Phase::Move, 1)).route, Route::Tool); // garde sa décision
          QCOMPARE(router.route(s(DeviceKind::Stylus, Phase::Press)).route, Route::Ignore);
      }
      void simultaneousPinchDrawsNothing()
      {
          // Deux doigts posés dans le même événement : aucun ne va à l'outil (M4 de la relecture).
          PointerRouter router;
          Sample a = s(DeviceKind::Touch, Phase::Press, 1);
          Sample b = s(DeviceKind::Touch, Phase::Press, 2);
          a.event = b.event = 7;
          const std::vector<Decision> both = router.routeEvent({a, b});
          QCOMPARE(both.size(), std::size_t{2});
          QCOMPARE(both[0].route, Route::Gesture);
          QCOMPARE(both[1].route, Route::Gesture);
          QVERIFY(!both[0].endToolContacts && !both[1].endToolContacts);
      }
      void stylusTakesOverFromFinger()
      {
          PointerRouter router;
          router.route(s(DeviceKind::Touch, Phase::Press, 1));
          const Decision stylus = router.route(s(DeviceKind::Stylus, Phase::Press));
          QCOMPARE(stylus.route, Route::Tool);
          QVERIFY(stylus.endToolContacts); // la vue termine le trait du doigt
          QCOMPARE(router.route(s(DeviceKind::Touch, Phase::Move, 1)).route, Route::Ignore);
      }
      void resetForgetsContacts()
      {
          PointerRouter router;
          router.route(s(DeviceKind::Touch, Phase::Press, 1));
          router.reset();
          QCOMPARE(router.route(s(DeviceKind::Touch, Phase::Move, 1)).route, Route::Ignore);
      }
  };

  QTEST_GUILESS_MAIN(TstInputRouting)
  #include "tst_input_routing.moc"
  ```
  `tests/tst_input_gesture.cpp` :
  ```cpp
  #include <actcraft/input/gesture.h>

  #include <QTest>

  using namespace actcraft::input;

  class TstInputGesture : public QObject
  {
      Q_OBJECT

  private slots:
      void pinchScalesAroundCenter()
      {
          TwoFingerGesture g;
          g.begin({400, 300}, {600, 300});
          const GestureStep step = g.update({300, 300}, {700, 300});
          QCOMPARE(step.scale, 2.0);
          QCOMPARE(step.anchor.x, 500.0);
          QCOMPARE(step.center.x, 500.0);
      }
      void panFollowsCenter()
      {
          TwoFingerGesture g;
          g.begin({400, 300}, {600, 300});
          const GestureStep step = g.update({450, 350}, {650, 350});
          QCOMPARE(step.scale, 1.0);
          QCOMPARE(step.center.x, 550.0);
          QCOMPARE(step.center.y, 350.0);
      }
      void deadZoneKeepsZoom()
      {
          TwoFingerGesture g;
          g.begin({400, 300}, {600, 300});
          QCOMPARE(g.update({398, 300}, {603, 300}).scale, 1.0); // 2,5 % < kPinchDeadZone
      }
      void endDeactivates()
      {
          TwoFingerGesture g;
          g.begin({0, 0}, {10, 0});
          QVERIFY(g.active());
          g.end();
          QVERIFY(!g.active());
      }
  };

  QTEST_GUILESS_MAIN(TstInputGesture)
  #include "tst_input_gesture.moc"
  ```
  Commande ciblée → rouge.
- [ ] **Étape 2 bis : deux règles de la relecture (M4).** (a) `Sample` gagne un troisième champ en fin de
  structure, `std::uint64_t event = 0; // numéro de l'événement d'entrée qui a porté l'échantillon (0 : inconnu)`, et
  `PointerRouter` gagne `std::vector<Decision> routeEvent(const std::vector<Sample>& samples);` : les appuis d'un même
  événement sont décidés **ensemble** (`RouterState` compte d'abord tous les appuis tactiles de l'événement) ; deux
  doigts ou plus posés dans le même événement sans doigt déjà routé vers l'outil → tous au geste, aucun trait ;
  (b) un stylet ou une souris qui appuie pendant qu'un doigt va à l'outil → `Tool` avec `endToolContacts` ; la
  décision retenue du doigt passe à `Ignore` (son trait est terminé par la vue, la suite du contact n'écrit plus).
  **Si le socle L0-S crée lui-même `PointerRouter` et `routing.h`**, ces deux règles, leurs deux tests et le champ
  `event` lui reviennent : l'orchestrateur amende alors cette section (L3-T5 garde la politique `FullTouchPolicy` et
  le geste).
- [ ] **Étape 3 : implémentation.** `FullTouchPolicy::decide` : `Stylus`, `Mouse`, `Other` → `Tool` ; `Touch` :
  `stylusDown` → `Ignore` ; `toolTouches + gestureTouches == 0` → `Tool` ; `toolTouches == 1 && gestureTouches == 0`
  → `Gesture` + `endToolContacts` ; `gestureTouches == 1` (un doigt resté du geste) → `Gesture` ; sinon `Ignore`.
  `PointerRouter::route` : appui → décision de la politique, retenue (clé `pointer`, 0 pour stylet et souris), compteurs
  de `m_state` mis à jour ; `endToolContacts` → décisions retenues des doigts `Tool` passées à `Gesture`, compteurs
  recalculés ; relâché → décision rendue puis retirée ; contact inconnu → `Ignore`. `TwoFingerGesture` : milieu,
  distance ; `scale = d / d0`, ramené à 1 si `|scale − 1| < kPinchDeadZone`.
- [ ] **Étape 4 : vert, preuves, commit.**
  ```powershell
  git -C <wt> add input/include/actcraft/input/sample.h input/src/routing.cpp input/src/gesture.cpp input/include/actcraft/input/routing.h input/include/actcraft/input/gesture.h tests/tst_input_routing.cpp tests/tst_input_gesture.cpp
  git -C <wt> commit -m "input : routeur unique des contacts, politique remplaçable, geste à deux doigts (L3-T5)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t6"></a>
### Tâche L3-T6 — `tools` : interface `Tool`, contexte, registre, état d'utilisateur, stylo et marqueur

**Objectif.** Remplir `userstate.*`, `toolregistry.*` (en-têtes figés) ; faire du stylo un `tools::Tool` qui sert
aussi de marqueur (`core::StrokeKind::Highlighter`, translucidité, F-225) ; nouveaux traits à bouts ronds
(`core::PenCap::Round`, D2) ; réglages par défaut (D5). Patron à ne pas reprendre : une classe `HighlighterTool`
copiée du stylo (deux chemins d'encre à tenir en parité).

**Fichiers.** Modifier : `tools/include/actcraft/tools/pentool.h`, `tools/src/pentool.cpp`, `tests/tst_pentool.cpp` ;
talons `tools/src/userstate.cpp`, `tools/src/toolregistry.cpp`, `tests/tst_tools_registry.cpp` ; en-têtes
`tool.h`, `toolcontext.h`, `userstate.h`, `toolregistry.h` : membres privés seulement.

**Test et commande bornée.** Commande ciblée `tst_pentool|tst_tools_registry` ; preuve de tâche N ≥ 111 ;
fonctions en `PASS` ; les cas existants de `tst_pentool` passent sans modification de leurs assertions.

**Interdits.** `registerBuiltinTools` crée la gomme et la sélection par leurs constructeurs (talons de L3-T1) sans
toucher `erasertool.*` ni `selecttool.*` (L3-T7, L3-T8). Ne pas changer la signature de `DocumentSink::commitStroke`.

**Terminé quand.** Tests ci-dessous en `PASS` ; `tst_pentool` existant vert.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** `tests/tst_tools_registry.cpp` :
  ```cpp
  #include <actcraft/tools/toolregistry.h>
  #include <actcraft/tools/userstate.h>

  #include <QTest>

  using namespace actcraft;
  using namespace actcraft::tools;

  class TstToolsRegistry : public QObject
  {
      Q_OBJECT

  private slots:
      void builtinsRegistered()
      {
          ToolRegistry registry;
          registerBuiltinTools(registry);
          QCOMPARE(registry.ids(), (QStringList{"select", "pen", "highlighter", "eraser"}));
          QVERIFY(!registry.add(QStringLiteral("pen"), {}));
          QVERIFY(!registry.add(QString(), {}));
      }
      void defaultsPerTool()
      {
          QCOMPARE(defaultSettings(QStringLiteral("pen")).color, core::Argb{0xFF000000u});
          QCOMPARE(defaultSettings(QStringLiteral("pen")).width, 4.0);
          QCOMPARE(defaultSettings(QStringLiteral("highlighter")).color, core::Argb{0xFFFFFF00u});
          QCOMPARE(defaultSettings(QStringLiteral("highlighter")).width, 20.0);
          QCOMPARE(defaultSettings(QStringLiteral("highlighter")).translucency, std::uint8_t{128});
          QCOMPARE(defaultSettings(QStringLiteral("eraser")).width, 20.0);
      }
      void userKeepsSettingsPerTool()
      {
          UserState user;
          QCOMPARE(user.currentTool(), QStringLiteral("pen"));
          user.setSettings(QStringLiteral("pen"), {0xFFE53935u, 8, 0});
          user.setCurrentTool(QStringLiteral("highlighter"));
          QCOMPARE(user.settings(QStringLiteral("highlighter")).color, core::Argb{0xFFFFFF00u});
          user.setCurrentTool(QStringLiteral("pen"));
          QCOMPARE(user.settings(QStringLiteral("pen")).color, core::Argb{0xFFE53935u});
          QCOMPARE(user.settings(QStringLiteral("pen")).width, 8.0);
      }
      void selectionBelongsToTheUser()
      {
          UserState user(1);
          QCOMPARE(user.id(), input::UserId{1});
          user.setSelection({4, 5});
          QCOMPARE(user.selection().size(), std::size_t{2});
      }
  };

  QTEST_GUILESS_MAIN(TstToolsRegistry)
  #include "tst_tools_registry.moc"
  ```
  À `tests/tst_pentool.cpp`, avec ses doublures existantes d'`InkSurface` et de `DocumentSink` (la doublure de
  `DocumentSink` gagne `commitAnnotation`, qui garde la translucidité reçue) :
  ```cpp
  void highlighterStrokeIsTranslucentKind()
  {
      FakeInk ink;
      FakeSink sink;
      tools::PenTool marker(ink, sink, core::StrokeKind::Highlighter);
      marker.setTranslucency(128);
      QCOMPARE(marker.id(), QStringLiteral("highlighter"));
      drawLine(marker); // aide existante : appui, deux mouvements, relâché
      QCOMPARE(sink.strokes.size(), std::size_t{1});
      QCOMPARE(sink.strokes[0].kind, core::StrokeKind::Highlighter);
      QCOMPARE(sink.lastTranslucency, std::uint8_t{128});
  }
  void newStrokesHaveRoundCaps()
  {
      FakeInk ink;
      FakeSink sink;
      tools::PenTool pen(ink, sink);
      QCOMPARE(pen.id(), QStringLiteral("pen"));
      drawLine(pen);
      QCOMPARE(sink.strokes[0].cap, core::PenCap::Round);
  }
  void cancelCommitsTheStrokeInProgress()
  {
      FakeInk ink;
      FakeSink sink;
      tools::PenTool pen(ink, sink);
      pressAndMove(pen); // aide : appui et un mouvement, sans relâché
      QVERIFY(pen.isBusy());
      pen.cancel();
      QVERIFY(!pen.isBusy());
      QCOMPARE(sink.strokes.size(), std::size_t{1});
  }
  ```
  (Noms des doublures et aides : ceux du fichier ; en ajouter s'il en manque.) Commande ciblée → rouge.
- [ ] **Étape 2 : implémentation.** `pentool.h` :
  ```cpp
  class DocumentSink
  {
  public:
      virtual ~DocumentSink() = default;
      virtual void commitStroke(core::Stroke stroke) = 0;
      // Lot 3 : trait d'un outil d'encre avec sa translucidité (marqueur). Par défaut : commitStroke.
      virtual void commitAnnotation(core::Stroke stroke, std::uint8_t translucency)
      {
          static_cast<void>(translucency);
          commitStroke(std::move(stroke));
      }
  };

  class PenTool final : public Tool
  {
  public:
      PenTool(InkSurface& ink, DocumentSink& document, core::StrokeKind kind = core::StrokeKind::Pen);
      QString id() const override; // « pen » ou « highlighter »
      bool isBusy() const noexcept override;
      void cancel() override; // verse le trait en cours à son dernier point
      void setTranslucency(std::uint8_t translucency) noexcept;
      std::uint8_t translucency() const noexcept;
      core::StrokeKind kind() const noexcept;
      // … membres existants inchangés (setColor, setBaseWidth, color, baseWidth, isDrawing, onSample)
  };
  ```
  `finish()` : `m_current.kind = m_kind; m_current.cap = core::PenCap::Round;` puis `commitAnnotation(…,
  m_translucency)` ; `InkSurface::beginStroke` reçoit la couleur avec l'alpha déduit de la translucidité pour l'encre
  vive (`alpha = 255 − translucency`), le trait versé garde la couleur opaque et `common.translucency` (posé par le
  contrôleur, L3-T12). `userstate.cpp`, `toolregistry.cpp` : d'après les en-têtes ; `registerBuiltinTools` ajoute
  `select` (`std::make_unique<SelectTool>(context)`), `pen` (`PenTool(context.ink, context.strokes)`),
  `highlighter` (`PenTool(…, StrokeKind::Highlighter)`), `eraser` (`EraserTool(context)`).
- [ ] **Étape 3 : vert, preuves, commit.**
  ```powershell
  git -C <wt> add tools/include/actcraft/tools/pentool.h tools/src/pentool.cpp tools/src/userstate.cpp tools/src/toolregistry.cpp tools/include/actcraft/tools/tool.h tools/include/actcraft/tools/toolcontext.h tools/include/actcraft/tools/userstate.h tools/include/actcraft/tools/toolregistry.h tests/tst_pentool.cpp tests/tst_tools_registry.cpp
  git -C <wt> commit -m "tools : interface d'outil, registre, état d'utilisateur ; stylo et marqueur, bouts ronds (L3-T6)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t11"></a>
### Tâche L3-T11 — `render` : forme du trait et marqueur translucide d'un seul tenant

**Objectif.** H10-6 et P4, tranchés par D2 : bouts et jonctions ronds (selon `Stroke::cap`), courbe lissée par les
milieux des points successifs, dans les trois chemins (géométrie RHI de la page fixe et de la couche d'attente, encre
vive `InkItem`, peintre logiciel `pagepainter.cpp`). P5 et D3 : un trait de marqueur ne fonce pas là où il se
recoupe ; deux traits superposés foncent. Patron évident à **ne pas** reprendre : une couleur à alpha par sommet sur
une bande de triangles qui se recouvrent (le trait fonce à chaque jonction et à chaque croisement, en D3D11) ; ni
une opacité de nœud (`QSGOpacityNode`), qui a le même défaut.

**Fichiers.** Modifier : `render/include/actcraft/render/strokegeometry.h`, `render/src/strokegeometry.cpp`,
`render/include/actcraft/render/inkitem.h`, `render/src/inkitem.cpp`, `render/src/strokerenderer.cpp`,
`render/src/pagepainter.cpp`, `ui/qml/PageArea.qml` (seulement si l'encre vive translucide a besoin d'une couche
`layer.enabled` sur `inkLayer`), `tests/tst_render_painter.cpp` (cas existants recalés **seulement** pour les bouts
ronds, chaque recalage expliqué dans le rapport), talon `tests/tst_render_strokeshape.cpp`.

**Test et commande bornée.** Commande ciblée `tst_render_strokeshape|tst_render_painter|tst_render_` ; preuve de
tâche N ≥ 111 ; fonctions en `PASS` ; banc sans écran `content_check.ok = True` ; à l'écran D3D11 (jeton `ecran`,
annoncé) : `captures.ps1 -Scenario anticrenelage -Rendu d3d11` → `CAPTURES OK`, `BORD … OK` (le lissage du lot 10
tient).

**Interdits.** Aucune API privée de Qt (pas de `qTriangulate`, pas de `QPainterPath` triangulé par une API privée).
Ne pas changer `core::Stroke` ni la lecture des `.flipchart`. Pas de redessin du cache pendant l'écriture (le
marqueur vif reste dans l'encre).

**Terminé quand.** Les tests ci-dessous passent dans le rendu logiciel **et** dans le chemin RHI hors écran
(`tst_render_engines` donne le moyen de rendre les deux) ; le banc ne régresse pas sur `stroke` (comparé au JSON de
référence joint par l'orchestrateur, deux passages) ; le choix technique (piste retenue, piste écartée, mesure) est
écrit dans le rapport.

**Étapes.**
- [ ] **Étape 1 : tests rouges** (`tests/tst_render_strokeshape.cpp`, fenêtre hors écran comme
  `tst_render_engines`, rendu des deux moteurs quand le test le permet) :
  - `roundCapsCoverTheDisk` : trait horizontal d'épaisseur 20, `cap = Round` : le pixel à 8 px au-delà du dernier
    point, sur l'axe, est encré ; avec `cap = Square` du prototype, le pixel à 8 px au-delà, à 9 px de l'axe, est
    encré et pas avec `Round`.
  - `midpointSmoothing` : trait en V (3 points) : le pixel du sommet anguleux exact (sur la bissectrice, à
    l'extérieur, à la demi-épaisseur + 2) est blanc (la courbe passe par les milieux) ; les deux bouts restent aux
    points d'origine.
  - `selfCrossingHighlighterDoesNotDarken` : marqueur jaune translucide 128 en boucle qui se recoupe : pixel au
    croisement = pixel au milieu d'une branche (écart ≤ 2 par canal).
  - `twoHighlighterPassesDarken` : deux traits de marqueur superposés : pixel commun plus sombre qu'un pixel d'un seul
    trait (écart ≥ 20 sur le canal bleu, sur fond blanc).
  - `liveInkMatchesTheFoldedStroke` : le même marqueur dessiné par `InkItem` (encre vive) puis versé dans la page
    fixe : mêmes pixels au croisement (écart ≤ 4).
- [ ] **Étape 2 : géométrie.** `strokegeometry` : lissage par les milieux (courbe quadratique de milieu en milieu,
  points de contrôle aux points d'origine, échantillonnée à pas fixe en pixels de page), bouts ronds (demi-disque
  triangulé) et jonctions rondes selon `Stroke::cap`. Peintre logiciel : `QPainterPath` lissé de même,
  `Qt::RoundCap`, `Qt::RoundJoin` ; un trait translucide est peint d'un seul `drawPath` (ne fonce pas sur
  lui-même).
- [ ] **Étape 3 : marqueur d'un seul tenant en RHI.** Pistes, à mesurer au banc avant de choisir : (a) le trait
  translucide passe par le moteur peint de la page fixe (`PaintedRenderer`, texture d'un seul `drawPath`) et
  l'encre vive translucide se dessine opaque dans `InkItem` avec `layer.enabled` et `opacity` sur la couche d'encre
  le temps du trait ; (b) géométrie sans recouvrement (contour du trait calculé par nous, union des segments sans
  API privée). Retenir la moins coûteuse qui passe les tests ; écrire la mesure.
- [ ] **Étape 4 : vert, preuves, commit.**
  ```powershell
  git -C <wt> add render/include/actcraft/render/strokegeometry.h render/src/strokegeometry.cpp render/include/actcraft/render/inkitem.h render/src/inkitem.cpp render/src/strokerenderer.cpp render/src/pagepainter.cpp tests/tst_render_strokeshape.cpp tests/tst_render_painter.cpp
  git -C <wt> commit -m "render : bouts et jonctions ronds, courbe lissée, marqueur translucide d'un seul tenant (L3-T11)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```
  (`ui/qml/PageArea.qml` ajouté à la commande seulement s'il a changé.)

---

<a id="tache-l3-t12"></a>
### Tâche L3-T12 — `DocumentController` : soulever et déposer, Annuler / Rétablir, Supprimer, Effacer, Rétablir la page

**Objectif.** Remplir l'API posée par le socle L0-S (ou L3-T1 avant son amendement). Le contrôleur reste la
**seule** voie vers le cache : au soulèvement,
`core::takeObjects` sur la page affichée puis `core::liftChanges` → `applyToCache` ; au dépôt,
`core::restoreObjects`, exécution de la commande (pile), puis `core::changesAfterLift` → `applyToCache` (et non les
changements bruts de la commande). Avant toute édition qui n'est pas un trait (annuler, rétablir, effacer,
rétablir la page, supprimer, enregistrer, ouvrir) — **jamais au soulèvement** (B1) —, `editAboutToApply` est émis : la vue y termine le trait en cours (L2-T15 : sinon le
trait irait à la page bornée et effacerait la branche à rétablir). Page de référence de Rétablir la page : la page
telle qu'ouverte ou enregistrée en dernier ; jamais enregistré : page vierge de même taille (F-131, D9).

**Fichiers.** Modifier : `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`,
`tests/tst_ui_document.cpp` ; talon `tests/tst_ui_lift.cpp`.

**Test et commande bornée.** Commande ciblée `tst_ui_lift|tst_ui_document` ; preuve de tâche N ≥ 111 ; fonctions
en `PASS` ; mutation : au dépôt, appliquer les changements bruts de la commande → `dropInsertsLiftedObjects` rouge,
puis remise.

**Interdits.** Aucune signature publique de L3-T1 changée. Aucun appel direct à `FixedPageItem` hors de
`attach()`/`applyToCache` existants. Pendant un soulèvement : `saveToUrl`, `openUrl`, `nextPage`, `pageBack`,
`replaceDocument` déposent d'abord sans étape (`dropLifted(nullptr)`) — Enregistrer n'écrit jamais un document dont
des objets sont soulevés.

**Terminé quand.** Tests ci-dessous en `PASS` ; mutation rouge puis remise ; `tst_ui_document` existant vert.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** `tests/tst_ui_lift.cpp` (contrôleur sans fenêtre quand c'est possible ; sinon avec
  la page fixe hors écran comme `tst_ui_document`) :
  ```cpp
  #include <actcraft/core/edits.h>
  #include <actcraft/core/objectedits.h>
  #include <actcraft/core/pagecache.h>
  #include <actcraft/ui/documentcontroller.h>

  #include <QSignalSpy>
  #include <QTest>

  using namespace actcraft;

  namespace {
  struct CacheSpy final : core::PageCache
  {
      std::vector<std::pair<char, core::ObjectId>> calls; // 'i' insert, 'r' remove, 'v' invalidate
      void insert(core::ObjectId id) override { calls.emplace_back('i', id); }
      void remove(core::ObjectId id, const core::RectF&) override { calls.emplace_back('r', id); }
      void invalidate(core::ObjectId id, const core::CacheArea&) override { calls.emplace_back('v', id); }
  };
  core::Stroke line(double y)
  {
      core::Stroke s;
      s.points = {{100, y, 0.5f}, {300, y, 0.5f}};
      return s;
  }
  } // namespace

  class TstUiLift : public QObject
  {
      Q_OBJECT

  private slots:
      void liftRemovesThroughTheCacheInterface()
      {
          ui::DocumentController controller;
          CacheSpy spy;
          controller.setCacheObserver(&spy);
          controller.commitStroke(line(100));
          const core::ObjectId id = controller.document().page(0).objects.back().common.id;
          spy.calls.clear();
          QSignalSpy about(&controller, &ui::DocumentController::editAboutToApply);
          const auto copies = controller.liftObjects({id});
          QCOMPARE(copies.size(), std::size_t{1});
          QCOMPARE(about.count(), 0); // B1 : soulever n'annonce rien, sinon l'outil qui soulève s'annulerait
          QVERIFY(controller.isLifting());
          QCOMPARE(controller.shownObjectCount(), 0);
          QCOMPARE(spy.calls, (std::vector<std::pair<char, core::ObjectId>>{{'r', id}}));
      }
      void dropInsertsLiftedObjects()
      {
          ui::DocumentController controller;
          CacheSpy spy;
          controller.setCacheObserver(&spy);
          controller.commitStroke(line(100));
          const core::ObjectId id = controller.document().page(0).objects.back().common.id;
          controller.liftObjects({id});
          spy.calls.clear();
          controller.dropLifted(std::make_unique<core::TransformObjectsCommand>(0, std::vector<core::ObjectId>{id},
                                                                                 core::translation(40, 0)));
          QCOMPARE(spy.calls, (std::vector<std::pair<char, core::ObjectId>>{{'i', id}}));
          QVERIFY(!controller.isLifting());
          QCOMPARE(controller.document().page(0).objects.back().common.transform.dx, 40.0);
          controller.undo(); // une étape
          QCOMPARE(controller.document().page(0).objects.back().common.transform.dx, 0.0);
      }
      void dropWithoutCommandAddsNoStep()
      {
          ui::DocumentController controller;
          controller.commitStroke(line(100));
          const core::ObjectId id = controller.document().page(0).objects.back().common.id;
          const QString before = controller.fingerprintText();
          controller.liftObjects({id});
          controller.dropLifted(nullptr);
          QCOMPARE(controller.fingerprintText(), before);
          controller.undo(); // annule le trait, pas un geste
          QCOMPARE(controller.shownObjectCount(), 0);
      }
      void undoWhileLiftingCancelsFirst()
      {
          ui::DocumentController controller;
          controller.commitStroke(line(100));
          controller.commitStroke(line(200));
          const core::ObjectId first = controller.document().page(0).objects.front().common.id;
          controller.liftObjects({first});
          controller.undo(); // dépose sans étape, puis annule le second trait
          QVERIFY(!controller.isLifting());
          QCOMPARE(controller.shownObjectCount(), 1);
          QCOMPARE(controller.document().page(0).objects.front().common.id, first);
      }
      void pageChangeWhileLiftingDropsFirst()
      {
          ui::DocumentController controller;
          controller.commitStroke(line(100));
          const core::ObjectId id = controller.document().page(0).objects.back().common.id;
          controller.liftObjects({id});
          QSignalSpy about(&controller, &ui::DocumentController::pageAboutToChange);
          controller.nextPage(); // annonce, puis dépose sans étape ce qui reste soulevé
          QCOMPARE(about.count(), 1);
          QVERIFY(!controller.isLifting());
          QCOMPARE(controller.document().page(0).objects.size(), std::size_t{1});
      }
      void saveWhileLiftingDropsFirst()
      {
          QTemporaryDir dir;
          ui::DocumentController controller;
          controller.commitStroke(line(100));
          const core::ObjectId id = controller.document().page(0).objects.back().common.id;
          controller.liftObjects({id});
          QSignalSpy about(&controller, &ui::DocumentController::editAboutToApply);
          QSignalSpy saved(&controller, &ui::DocumentController::saveFinished);
          controller.saveToUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("a.actcraft"))));
          QCOMPARE(about.count(), 1); // l'outil abandonne son geste avant l'écriture
          QVERIFY(!controller.isLifting());
          QVERIFY(saved.wait(10000));
          QCOMPARE(saved.first().first().toBool(), true);
          // rouvert : l'objet est dans le fichier
          QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
          controller.openUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("a.actcraft"))));
          QVERIFY(loaded.wait(10000));
          QCOMPARE(controller.shownObjectCount(), 1);
      }
  };

  QTEST_MAIN(TstUiLift)
  #include "tst_ui_lift.moc"
  ```
  (En tête du fichier : `#include <QTemporaryDir>` et `#include <QUrl>`.) À `tests/tst_ui_document.cpp` :
  `revertPageReturnsToSaved` (enregistrer dans un dossier temporaire, ajouter un trait, `revertPage()` : empreinte
  égale à celle de l'enregistrement, `canUndo` vrai), `revertNeverSavedEmptiesPage` (document neuf, deux traits,
  `revertPage()` : 0 objet, taille gardée), `undoAnnouncesTheEditFirst` (`QSignalSpy` sur `editAboutToApply` :
  émis une fois par `undo()`, `redo()`, `clearPage`, `revertPage`, `deleteObjects`, et **avant** `undoChanged` ;
  zéro fois par `liftObjects` et `dropLifted`),
  `clearPageAnnotations` (`clearPage(0)` retire les traits, garde une forme ajoutée par `execute`),
  `highlighterKeepsItsTranslucency` (`commitAnnotation(trait, 128)` → `common.translucency == 128`, couche
  supérieure). Commande ciblée → rouge.
- [ ] **Étape 2 : implémentation.** Membres privés : `std::vector<core::LiftedObject> m_lifted`,
  `std::vector<core::Page> m_savedPages` (posée par `replaceDocument` et à la fin d'un enregistrement réussi du
  contenu affiché ; vide pour un document neuf), `core::PageCache* m_observer`. Les appels de cache passent par une
  fonction privée unique qui applique `applyToCache` à la page fixe puis à l'observateur. `liftObjects` :
  **aucune annonce** (B1 : un seul utilisateur, aucun trait d'un autre outil n'est en cours ; annoncer ferait annuler
  l'outil qui soulève par L3-T14), `takeObjects`, `liftChanges`, `documentChanged`, `liftingChanged`. **Dépôt forcé
  (M2)** : `nextPage`, `pageBack`, `replaceDocument` émettent d'abord `pageAboutToChange` (déjà émis aujourd'hui),
  `saveToUrl` et `openUrl` émettent d'abord `editAboutToApply` ; l'outil y abandonne son geste (`ToolHost`, L3-T14 :
  `drop(nullptr)` et couche soulevée vidée) ; ensuite seulement, s'il reste des objets soulevés, le contrôleur les
  dépose sans étape. Le commentaire de `liftObjects` posé par L3-T1 (« Annonce editAboutToApply… ») est corrigé dans
  l'en-tête en conséquence. `dropLifted` : `restoreObjects`, `run(command)` si non nul, `changesAfterLift`, signaux.
  `undo`/`redo` : `editAboutToApply`, dépôt sans étape si besoin, puis la pile. `revertPage` : `ReplacePageCommand`
  avec la page de référence (ou `core::Page` vierge de même taille). `canRevertPage` : la page affichée diffère de
  sa référence (empreinte).
- [ ] **Étape 3 : vert, mutation, preuves, commit.**
  ```powershell
  git -C <wt> add ui/include/actcraft/ui/documentcontroller.h ui/src/documentcontroller.cpp tests/tst_ui_document.cpp tests/tst_ui_lift.cpp
  git -C <wt> commit -m "ui : soulever et déposer par l'interface de cache, édition annoncée avant le trait, effacer, rétablir la page (L3-T12)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t17"></a>
### Tâche L3-T17 — Joueur de scénarios : étapes `pinch` et `touch`, `expect` avec tolérance

**Objectif.** S6 demande « un pincement à deux doigts injecté » : le joueur ne sait injecter que stylet, souris,
touches et commandes. Ajouter deux étapes (à la fin de `UiStepKind`) et une tolérance numérique à `expect`.

**Fichiers.** Modifier : `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`,
`bench/include/actcraft/bench/injector.h`, `bench/src/injector.cpp`, `tests/tst_bench_uiscenario.cpp`,
`tests/tst_bench_injector.cpp`.

**Test et commande bornée.** Commande ciblée `tst_bench_uiscenario|tst_bench_injector` ; preuve de tâche N ≥ 111 ;
fonctions en `PASS`.

**Interdits.** Rangs existants de `UiStepKind` inchangés (ajouts à la fin) ; injection par l'API publique de Qt
(`QWindowSystemInterface` publique, comme l'injecteur existant ; un `QPointingDevice` tactile enregistré une fois) ;
aucune souris réelle.

**Terminé quand.** Étapes lues, jouées et rapportées (`DOIGT …`, `PINCEMENT …`) ; tests en `PASS`.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** `tst_bench_uiscenario` : `touchStepParsed`
  (`{"touch": [[100, 100], [300, 100]]}` → `UiStepKind::Touch`, 2 points de page), `pinchStepParsed`
  (`{"pinch": [500, 400], "from": 100, "to": 200, "steps": 10}` → `UiStepKind::Pinch`, centre de page, écarts en
  pixels de vue, `steps` 10 ; `steps` absent : 12 ; `from` ≤ 0 : `SCENARIO INVALIDE`), `expectWithTolerance`
  (`{"expect": "pageView.zoom", "value": 1.5, "tolerance": 0.01}` : 1.505 → `LU … OK`, 1.6 → `ECART`).
  `tst_bench_injector` : `pinchDeliversTwoTouchPoints` (élément d'essai qui compte les points d'un `QTouchEvent` :
  appui à deux points, `steps` mouvements, relâché ; écart final entre les deux points = `to` ± 1 px, milieu fixe).
- [ ] **Étape 2 : implémentation.** `enum class UiStepKind` : ajouter `Touch` et `Pinch` à la fin (commentaire
  « Ajoutées au lot 3 (L3-T17, S6), à la fin »). `touch` : un doigt, points de page convertis en points de fenêtre
  comme l'étape `stroke`, intervalle de l'étape `stroke` ; ligne `DOIGT <n> points OK`. `pinch` : deux doigts
  placés horizontalement autour du centre (converti en fenêtre), écart `from` → `to` en `steps` pas d'une image ;
  ligne `PINCEMENT ×<to/from> OK`. `expect` : `tolerance` numérique facultative (comparaison exacte sinon, comme
  aujourd'hui).
- [ ] **Étape 3 : vert, preuves, commit.**
  ```powershell
  git -C <wt> add bench/include/actcraft/bench/uiscenario.h bench/src/uiscenario.cpp bench/include/actcraft/bench/injector.h bench/src/injector.cpp tests/tst_bench_uiscenario.cpp tests/tst_bench_injector.cpp
  git -C <wt> commit -m "bench : étapes doigt et pincement du joueur de scénarios, tolérance des lectures (L3-T17)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t7"></a>
### Tâche L3-T7 — `tools` : gomme

**Socle.** La commande déposée au relâché est la `ReplaceObjectsCommand` du socle L0-S si elle existe (voir
L3-T4) ; le nom `ReplaceStrokesCommand` de cette section se lit alors comme elle.

**Objectif.** F-230 : au premier contact du disque avec une annotation, la gomme la **soulève** (une fois par trait
et par geste) et la montre recoupée dans la couche soulevée ; elle ne touche jamais une forme, une image, un texte, ni
une annotation déplacée hors de la couche du dessus ; au relâché, **une** `ReplaceStrokesCommand` déposée (un pas
d'annulation, P11) ; rien n'est exécuté pendant le geste. Rayon = largeur de la gomme / 2 (D4).

**Fichiers.** Modifier (talons) : `tools/src/erasertool.cpp`, `tests/tst_tools_eraser.cpp` ; en-tête
`tools/include/actcraft/tools/erasertool.h` : membres privés seulement.

**Test et commande bornée.** Commande ciblée `tst_tools_eraser` ; preuve de tâche N ≥ 111 ; fonctions en `PASS`.

**Interdits.** Aucun en-tête de `render/` ni de `ui/` ; aucune écriture du document hors `DocumentEditor` ; aucune
commande exécutée pendant le geste.

**Terminé quand.** Tests ci-dessous en `PASS`.

**Étapes.**
- [ ] **Étape 1 : doublures et tests rouges.** `tests/tst_tools_eraser.cpp` définit des doublures de toutes les
  interfaces de `ToolContext` : `FakeEditor` (une `core::Document` réelle ; `lift` retire par `core::takeObjects` et
  journalise les identifiants ; `drop` restaure puis exécute la commande sur le document et la journalise ;
  `execute` journalise), `FakeLift` (garde les objets montrés et compte les `show`), `FakeSelection`, `FakeView`
  (zoom 1, pas de corbeille, barre vide), `FakeCommands`, `FakeInk`, `FakeStrokes` ; et une aide
  `drag(tool, {points…})` qui envoie appui, mouvements, relâché (`DeviceKind::Mouse`). Cas :
  - `liftsEachTouchedStrokeOnce` : deux traits horizontaux (y 100 et 200), un trait de gomme vertical de (200, 50) à
    (200, 250) en 20 mouvements : `lift` appelé 2 fois en tout, une par trait, chaque identifiant une fois.
  - `nothingExecutedDuringTheGesture` : pendant les mouvements, `executed` et `dropped` vides.
  - `oneCommandAtRelease` : au relâché, un seul `drop` avec une `ReplaceStrokesCommand` ; page : 4 morceaux.
  - `shapesAreNeverTouched` : un rectangle sous le passage de la gomme : jamais soulevé, toujours dans la page.
  - `movedAnnotationIsNotErased` : un trait de la couche intermédiaire sous le passage : intact.
  - `cancelRestoresEverything` : appui, mouvements, `cancel()` : `drop(nullptr)`, empreinte d'origine.
  - `pieceCountAfterMiddleCut` : un trait, une touche au milieu (appui et relâché au même point) : 2 morceaux.
  - `liftLayerShowsTheCutPieces` : après un mouvement qui coupe, `FakeLift` montre les morceaux (2 objets).
- [ ] **Étape 2 : implémentation.** À l'appui : point de départ. Pour chaque segment de gomme (dernier point → point
  courant) : pour chaque annotation de la page (`core::isAnnotation`) dont l'emprise touche le balayage élargi du
  rayon, pas encore soulevée → `lift` (copie gardée sous forme de points de page, `core::strokePointsOnPage`) ;
  pour chaque trait soulevé, `core::cutBySweep` sur ses morceaux courants ; `LiftSurface::show` avec les morceaux
  courants (objets traits, commun de l'original, transformation identité). Au relâché : `drop` d'une
  `ReplaceStrokesCommand` (un `StrokeReplacement` par trait soulevé et coupé ; un trait soulevé mais non coupé
  garde un seul morceau égal à l'original) ; rien de soulevé : rien. `cancel` : `drop(nullptr)`, `LiftSurface::clear`.
- [ ] **Étape 3 : vert, preuves, commit.**
  ```powershell
  git -C <wt> add tools/src/erasertool.cpp tools/include/actcraft/tools/erasertool.h tests/tst_tools_eraser.cpp
  git -C <wt> commit -m "tools : gomme qui soulève, recoupe et dépose les annotations en une étape (L3-T7)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t8"></a>
### Tâche L3-T8 — `tools` : sélection — désigner, rectangle qui touche, déplacer, corbeille

**Objectif.** F-210 et F-541, F-050 : clic sur un objet → sélectionné (le plus haut) ; clic dans le vide →
désélection ; glisser dans le vide → rectangle élastique, tout objet **touché** est sélectionné (D6) ; glisser dans
la sélection → **soulèvement** au premier mouvement au-delà de 3 px d'écran (pas avant : un clic ne soulève rien),
couche soulevée déplacée par `setTransform` seul, cadre suivi, poignées masquées (F-528), dépôt d'une
`TransformObjectsCommand` au relâché ; relâché au-dessus de la corbeille → `RemoveObjectsCommand` (F-050), flèche
de la corbeille au survol (`ViewInfo::setTrashHover`) ; objet verrouillé ou non déplaçable
(`common.locked`, `restrictions.movable == false`) : sélectionné, jamais déplacé ; Suppr → supprimer, sans sélection
rien (F-541) ; Échap pendant un glissement → `drop(nullptr)`. `selectAll`, `clearSelection`, `deleteSelection`,
`enlarge`, `reduce` (`kEnlargeStep` autour de `selectionCenter`), `setTranslucency`, `refresh`. Le cadre et la barre
de base viennent de `layoutSelection` (sa forme complète, poignées et barre, est L3-T9 ; ici : coins et `visible`).

**Fichiers.** Modifier (talons) : `tools/src/selecttool.cpp`, `tools/src/selectiongeometry.cpp`,
`tests/tst_tools_select.cpp` ; en-têtes `selecttool.h`, `selectiongeometry.h` : membres privés seulement.

**Test et commande bornée.** Commande ciblée `tst_tools_select` ; preuve de tâche N ≥ 111 ; fonctions en `PASS`.

**Interdits.** Aucun en-tête de `render/` ni de `ui/` ; pas de commande par mouvement ; sélection rangée dans
`UserState` (DA-A1), nulle part ailleurs.

**Terminé quand.** Tests ci-dessous en `PASS`.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** `tests/tst_tools_select.cpp`, mêmes doublures que `tst_tools_eraser.cpp`
  (recopiées dans ce fichier ; `FakeView::overTrash` vrai pour x > 1000) :
  - `clickSelectsTopmost` : deux rectangles qui se recouvrent, clic au recouvrement → `user.selection() == {2}` ;
    `FakeSelection` a reçu `visible == true`, 4 coins.
  - `clickOnEmptyClears` : puis clic dans le vide → sélection vide, `visible == false`.
  - `rubberBandSelectsTouched` : glisser de (90, 90) à (130, 130) au-dessus de deux objets dont un seul est touché
    → sélection de celui-là ; pendant le glisser, `rubberBand` posé ; au relâché, effacé.
  - `dragLiftsOnceAndDropsOnce` : objet sélectionné, glisser de 100 px en 10 mouvements : `lift` une fois, `execute`
    jamais, `FakeLift` a reçu 10 `setTransform` et 1 `show`, `drop` une fois avec une `TransformObjectsCommand` ;
    l'objet est déplacé de 100 px dans le document.
  - `clickDoesNotLift` : appui et relâché sans mouvement (ou de 2 px) sur un objet → aucun `lift`.
  - `lockedObjectDoesNotMove` : objet `locked` : sélectionné au clic ; glisser → aucun `lift`, aucune commande.
  - `dropOverTrashDeletes` : glisser jusqu'à x = 1100 → `setTrashHover(true)` pendant le survol, puis `drop` d'une
    `RemoveObjectsCommand` ; objet absent ; sélection vide.
  - `escapeCancelsDrag` : pendant un glisser, `onKey(Qt::Key_Escape, 0)` → `drop(nullptr)`, objet à sa place.
  - `deleteWithoutSelectionDoesNothing` : `deleteSelection()` sans sélection → aucune commande (F-541).
  - `enlargeScalesAboutTheCenter` : `enlarge()` → `execute` d'une `TransformObjectsCommand` ; emprise × 1,1, même
    centre (± 0,01).
- [ ] **Étape 2 : implémentation.** Machine d'états (`Idle`, `Pressed`, `RubberBand`, `Dragging`) ; tolérance de
  désignation `core::kHitToleranceScreen / view.zoom()` ; seuil de glissement 3 px d'écran ; pendant `Dragging`,
  transformation `translation(dx, dy)` ; cadre recalculé par `layoutSelection` translaté ; `handlesVisible = false`.
- [ ] **Étape 3 : vert, preuves, commit.**
  ```powershell
  git -C <wt> add tools/src/selecttool.cpp tools/src/selectiongeometry.cpp tools/include/actcraft/tools/selecttool.h tools/include/actcraft/tools/selectiongeometry.h tests/tst_tools_select.cpp
  git -C <wt> commit -m "tools : sélection — désigner, rectangle qui touche, déplacer par soulèvement, corbeille (L3-T8)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t13"></a>
### Tâche L3-T13 — `PageView` : gestes à deux doigts, demandes de redessin regroupées, outil actif occupé

**Objectif.** Le chemin d'entrée est posé par le socle L0-S : événement → `input::Sample` → `PointerRouter`
(couvertures des surcouches) → filtre d'échantillons → **`input::Pipeline`** (numérotation `seq`, filtre de
doublons, sonde de latence du banc) → outil actif (`tools()->activeTool(user)`, ou le stylo interne si `tools` est
nul ou rend nullptr). Cette tâche **vérifie** que les garde-fous du lot 10, liés jusqu'ici au stylo interne, suivent
l'outil actif (M1) : la minuterie de versement teste `activeTool()->isBusy()` (et non `m_pen->isDrawing()`),
`finishStroke()` appelle `activeTool()->cancel()` (le stylo y verse son trait), la sonde de latence voit les
échantillons de l'outil routé ; si le socle ne l'a pas fait, elle le fait. Elle ajoute ensuite le geste : `Route::Gesture` → `input::TwoFingerGesture` : zoom = zoom à l'appui × `scale`, le point de page sous
`anchor` suit `center` ; `endToolContacts` → le trait du doigt en cours est terminé (`finishTouchStroke`, versé)
avant le geste ; `Route::Ignore` → rien (paume). La vue ne garde aucune règle propre au nombre de doigts.
`cacheRefreshCount` compte les `notifyCacheUpdated` ; `gestureActive` suit le geste. **Demandes de redessin
regroupées par tour (M3, règle retenue)** : toutes les demandes de redessin du cache arrivées dans un même tour de
boucle (contrôleur : `contentChanged` répétés d'un soulèvement de plusieurs objets ou d'un dépôt) font **un seul**
redessin : `requestCacheRefresh` pose un drapeau et poste une seule demande en file (`QMetaObject::invokeMethod`,
`Qt::QueuedConnection`) qui appelle `scheduleUpdate` une fois ; le relancement à la fin d'un redessin
(`m_refreshAgain`) n'est posé que par une demande arrivée **après** le départ de ce redessin. La décision 10 (« un
trait figé n'attend pas ») tient : la demande part au tour suivant, avant l'image suivante. Les règles déjà tenues
restent :
`canStartStroke` (zone et surcouches, `coveredAt`), `inputBlocked`, `finishStroke` avant zoom et défilement,
périphériques exclusifs du banc, `Qt Quick retarde les TouchUpdate` (piège L1-T8).

**Fichiers.** Modifier : `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`, `tests/tst_pageview.cpp`,
`tests/tst_ui_drawing.cpp`.

**Test et commande bornée.** Commande ciblée `tst_pageview|tst_ui_drawing` ; preuve de tâche N ≥ 111 ; fonctions en
`PASS` (dont tous les cas existants de `tst_pageview`, inchangés) ; banc sans écran `content_check.ok = True` ;
`premiers-pas` hors écran `CAPTURES OK`.

**Interdits.** Ne pas refaire le chemin d'entrée du socle (le compléter seulement s'il manque un garde-fou, et
l'écrire dans le rapport). Aucun `if` sur le nombre de points tactiles hors du routeur ; pas d'activation conjointe de
`AA_CompressHighFrequencyEvents` et `AA_CompressTabletEvents` ; ne pas toucher `toolhost.*` (L3-T14).

**Terminé quand.** Tests ci-dessous en `PASS` ; S2 et le banc inchangés.

**Étapes.**
- [ ] **Étape 1 : tests rouges** (`tests/tst_pageview.cpp`, fenêtre hors écran et `QTest::touchEvent` avec un
  `QPointingDevice` tactile créé par `QTest::createTouchDevice()`, une image laissée entre deux envois) :
  - `pinchZoomsAroundItsCenter` : zoom 1 ; deux doigts à (400, 300) et (600, 300) puis (300, 300) et (700, 300) :
    `zoom() == 2` (± 0,01) ; le point de page qui était sous (500, 300) y est encore (`pageToScene`, ± 1 px).
  - `twoFingerPanScrolls` : deux doigts déplacés ensemble de (0, +50) : `pan` change de 50 px de vue, zoom inchangé.
  - `secondFingerEndsFingerStroke` : un doigt trace (3 mouvements), un deuxième se pose : un trait versé (nombre
    d'objets + 1), puis aucun point de plus à ce trait quand les deux doigts bougent ; `gestureActive` vrai puis faux
    au relâché des deux.
  - `palmIgnoredDuringStylus` : stylet appuyé (filtre de la fenêtre), un doigt se pose et bouge : aucun trait de doigt,
    zoom inchangé.
  - `samplesGoToActiveTool` : `tools` nul → le stylo interne trace (comportement des lots 1 et 2).
  - `cacheRefreshCountCounts` : trois fois, `requestCacheRefresh` puis **attendre sa fin** (`QTRY_COMPARE` sur
    `cacheReady`) avant la demande suivante → `cacheRefreshCount()` + 3 (mineur 7 : trois demandes d'un même tour ne
    font plus qu'un redessin).
  - `requestsOfOneTurnMakeOneRedraw` : trois `requestCacheRefresh` dans le même tour, puis attente de `cacheReady` →
    `cacheRefreshCount()` + 1 ; une demande arrivée pendant un redessin commencé → + 1 de plus (relancement gardé).
  - `foldWaitsForTheActiveTool` : `tools` posé, outil stylo de `ToolHost` (ou doublure de `ToolHost` qui rend un
    `tools::Tool` occupé) : appui du stylet puis **immobile 800 ms** (au-delà de `kCacheFoldIdleMs`) : aucun versement
    (`fixedPage.pendingCount` inchangé, `cacheRefreshCount` inchangé) tant que l'outil est occupé ; relâché, puis
    500 ms : versement.
  - `latencyProbeSeesToolSamples` : sonde de latence posée (`setInkProbe`), dix échantillons de stylet routés vers
    l'outil actif : la sonde en a vu dix, numéros `seq` croissants.
  - `pageChangeEndsToolStroke` : un trait de l'outil actif en cours, `DocumentController::nextPage()` (signal
    `pageAboutToChange`, branché à `finishStroke` dans `Main.qml`) → le trait est versé à la page qu'il quitte,
    l'outil n'est plus occupé.
- [ ] **Étape 2 : implémentation.** Garde-fous du lot 10 sur l'outil actif (si le socle ne les a pas posés) ;
  regroupement des demandes de redessin ; puis le geste, sur les décisions du routeur du socle ;
  table « contact → dernier point » pour le geste ; à `Route::Gesture`, `TwoFingerGesture::begin` quand deux contacts
  de geste sont posés, `update` à chaque mouvement de l'un d'eux, `ViewTransform` recalculé (zoom borné comme la
  molette, ancrage par `anchor`, quitte « Ajuster ») ; fin du geste quand un des deux se relève ;
  `touchUngrabEvent` et `TouchCancel` : `m_router.reset()` et trait terminé.
- [ ] **Étape 3 : vert, preuves, commit.**
  ```powershell
  git -C <wt> add ui/include/actcraft/ui/pageview.h ui/src/pageview.cpp tests/tst_pageview.cpp tests/tst_ui_drawing.cpp
  git -C <wt> commit -m "ui : contacts par le routeur unique, pincer et défiler à deux doigts (L3-T13)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t9"></a>
### Tâche L3-T9 — `tools` : sélection — poignées, redimensionner, rotation, barre d'objet

**Objectif.** F-520 à F-523, F-528 : huit poignées rondes (rayon `kHandleRadiusScreen` d'écran), aux coins et aux
milieux des côtés du cadre ; glisser une poignée de côté étire (un seul axe), de coin redimensionne librement, sauf
le coin **bas droit**, qui garde les proportions ; le côté opposé reste fixe ; barre d'objet (`objectBarCommands()`,
D7) au-dessus du cadre, boutons de `kBarButtonScreen` espacés de 4 px, replacée sous le cadre si elle sortirait de
la page ; `object.rotate` se saisit : glisser fait tourner autour du centre (angle du pointeur autour du centre) ;
`object.move-free` se saisit : déplace même un objet verrouillé ou limité (F-521, 1) ; un autre bouton se clique :
`CommandSink::trigger(command, {})` (translucidité sans argument : la réglette s'ouvre, L3-T15). Tout redimensionnement
et toute rotation passent par le même soulèvement que le déplacement (couche soulevée, `setTransform`, une
`TransformObjectsCommand` au relâché). Restrictions : `resizable == false` → poignées absentes ; `rotatable ==
false` → bouton de rotation absent.

**Fichiers.** Modifier : `tools/include/actcraft/tools/selecttool.h`, `tools/src/selecttool.cpp`,
`tools/include/actcraft/tools/selectiongeometry.h`, `tools/src/selectiongeometry.cpp`, `tests/tst_tools_select.cpp`.

**Test et commande bornée.** Commande ciblée `tst_tools_select` ; preuve de tâche N ≥ 111 ; fonctions en `PASS`
(celles de L3-T8 comprises).

**Interdits.** Comme L3-T8. Aucune signature publique changée.

**Terminé quand.** Tests ci-dessous en `PASS`.

**Étapes.**
- [ ] **Étape 1 : tests rouges** (à la suite de L3-T8, mêmes doublures ; `FakeView::objectBarCommands` rend les cinq
  commandes de la barre) :
  - `eightHandlesAroundTheFrame` : rectangle (100, 100, 200, 100) sélectionné : `handles` = (100, 100), (200, 100),
    (300, 100), (300, 150), (300, 200), (200, 200), (100, 200), (100, 150) ; `bar` à 5 boutons, au-dessus de y = 100.
  - `barMovesBelowWhenItWouldLeaveThePage` : rectangle à y = 5 : `bar` sous le cadre.
  - `cornerHandleResizesProportionally` : glisser la poignée bas droite de (300, 200) à (400, 220) : emprise finale
    300 × 150 depuis (100, 100) (rapport 2 : 1 gardé, plus grand des deux facteurs).
  - `sideHandleStretches` : poignée droite de (300, 150) à (350, 190) : largeur 250, hauteur 100 inchangée.
  - `rotationAroundCenter` : saisir le bouton `object.rotate`, glisser d'un quart de tour autour du centre (200, 150)
    : transformation finale ≈ `rotationAbout({200, 150}, 90)` (± 0,5°) ; une seule commande.
  - `handlesHiddenWhileDragging` : pendant le glisser d'une poignée, `handlesVisible == false` ; après, vrai.
  - `barButtonTriggersCommand` : clic sur le bouton `object.enlarge` → `FakeCommands` a reçu `object.enlarge` ;
    aucun soulèvement.
  - `restrictionsHonoured` : `resizable = false` → aucune poignée ; `rotatable = false` → pas de bouton de rotation ;
    `locked` + glisser `object.move-free` → déplacé.
- [ ] **Étape 2 : implémentation.** `layoutSelection` complet (coins du cadre transformé pour un objet seul,
  emprise réunie sinon) ; `hitSelection` (ordre : barre, poignée, intérieur) ; états `Resizing(handle)` et `Rotating`
  dans la machine de L3-T8 ; transformation de redimensionnement = `scalingAbout(coin opposé, sx, sy)`, de rotation
  = `rotationAbout(centre, angle)`.
- [ ] **Étape 3 : vert, preuves, commit.**
  ```powershell
  git -C <wt> add tools/include/actcraft/tools/selecttool.h tools/src/selecttool.cpp tools/include/actcraft/tools/selectiongeometry.h tools/src/selectiongeometry.cpp tests/tst_tools_select.cpp
  git -C <wt> commit -m "tools : sélection — poignées, redimensionner, rotation, barre d'objet (L3-T9)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t14"></a>
### Tâche L3-T14 — `ui::ToolHost` : outils branchés

**Objectif.** Remplir `ToolHost` et `ToolAdapters` : un `tools::ToolRegistry` (`registerBuiltinTools`), un
`tools::UserState` (utilisateur 1), un `ToolContext` dont chaque interface est un adaptateur — `DocumentEditor` sur
`DocumentController` (`liftObjects`, `dropLifted`, `execute`, page affichée), `LiftSurface` sur
`render::LiftLayerItem`, `SelectionSurface` vers `selectionFrame` / `handlesVisible` (signal `selectionChanged`),
`ViewInfo` sur `PageView` (zoom, `trashArea` en coordonnées de scène comparée au point de page passé par
`pageToScene`, `Commands::objectBar`), `CommandSink` sur `Commands::trigger`, `InkSurface` et `DocumentSink` sur
l'encre et le contrôleur (`ControllerSink::commitAnnotation` → `DocumentController::commitAnnotation`).
`activate(id)` : `deactivate` de l'outil courant, création ou reprise de l'outil, commande `tool.<id>` cochée et
les autres décochées, couleur et épaisseur de l'outil rendues à `PageView` (`inkColor`, `inkWidth`, alpha du
marqueur). Mémoire par outil (F-136) : `setColor` / `setWidth` écrivent dans `UserState` **et** dans `QSettings`
(`outils/<id>/couleur`, `outils/<id>/epaisseur` ; fichier `settingsFile` s'il est posé), relus à la création.
`DocumentController::editAboutToApply` et `pageAboutToChange` → `cancel()` de l'outil courant (la sélection et la
gomme déposent alors sans étape et vident la couche soulevée, M2) ; le soulèvement lui-même n'annonce rien (B1) ;
`pagesChanged` → `SelectTool::refresh()`. **Échap (M9)** : la touche est le raccourci d'application de
`tool.select` (`Main.qml`), jamais livré à `Tool::onKey` ; `activate(id)` appelle donc `cancel()` sur l'outil
courant s'il est occupé (`isBusy`), et, si `id` est déjà l'outil courant, s'en tient là (Échap pendant un glissement =
abandon du geste ; Échap au repos = outil Sélectionner).
`Main.qml` : les gestionnaires `tool.pen` → `appTools.activate("pen")`, `tool.select` (Échap) →
`appTools.activate("select")` (règle de M9 ci-dessus), `pen.color` → `appTools.setColor(color)`,
`pen.width` → `appTools.setWidth(width)` ; `PageView` n'a plus de couleur propre quand `tools` est posé (S2 lit
toujours `pageView.inkColor`, que `ToolHost` tient à jour).

**Fichiers.** Modifier : `ui/include/actcraft/ui/toolhost.h`, `ui/src/toolhost.cpp`, `ui/src/tooladapters.h`,
`ui/src/tooladapters.cpp`, `ui/src/penadapters.h`, `ui/src/penadapters.cpp`, `ui/qml/Main.qml`,
`tests/tst_ui_tools.cpp` (talon), `tests/tst_ui_drawing.cpp`, `tests/tst_modules.cpp`.

**Test et commande bornée.** Commande ciblée `tst_ui_tools|tst_ui_drawing|tst_modules|tst_ui_shell` ; preuve de
tâche N ≥ 111 ; fonctions en `PASS` ; `premiers-pas` hors écran `CAPTURES OK` ; banc sans écran
`content_check.ok = True`.

**Interdits.** Aucun outil ne reçoit un pointeur vers `FixedPageItem`, `PendingLayerItem`, `LiftLayerItem` ou
`PageView` (seulement les adaptateurs) ; aucune signature publique de L3-T1 changée ; aucun réglage écrit dans le
profil réel pendant les tests (`settingsFile` sous le dossier temporaire du test).

**Terminé quand.** Tests ci-dessous en `PASS` ; S2 inchangé ; banc joint.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** `tests/tst_ui_tools.cpp` (fenêtre hors écran qui charge `Main.qml` comme
  `tst_ui_drawing`, fichier de réglages dans un `QTemporaryDir`, `CacheSpy` de `tst_ui_lift` posé par
  `appDocument.setCacheObserver`) :
  - `activatingAToolChecksItsCommand` : `trigger("tool.eraser")` → `currentTool == "eraser"`, `tool.eraser` coché,
    `tool.pen` décoché.
  - `colorGoesToActiveTool` : stylo, `trigger("pen.color", "#E53935")` ; marqueur : couleur jaune par défaut,
    `pageView.inkColor` jaune ; retour au stylo : rouge.
  - `settingsSurviveRestart` : stylo rouge 8, fenêtre détruite ; nouvelle fenêtre avec le même `settingsFile` :
    stylo rouge 8 (F-136).
  - `eraseGoesThroughTheCacheInterface` : deux traits au stylet injecté (méthode de `tst_ui_drawing`), gomme, trait
    de gomme qui coupe le premier : `CacheSpy` = `r(id1)` pendant le geste, puis au relâché `i(morceau1)`,
    `i(morceau2)` ; aucun `v` ; `shownObjectCount == 3` ; `liftLayer.objectCount == 0` après le relâché.
  - `dragGoesThroughTheCacheInterface` : sélection, glisser un trait de 80 px en 8 mouvements : `CacheSpy` = `r(id)`
    puis `i(id)`, rien entre les deux ; `cacheRefreshCount` de la vue augmenté de 2 au plus pendant le geste.
  - `dragOfTwoObjectsCostsTwoRedraws` (M3) : deux traits versés, rectangle de sélection sur les deux, glisser de 80 px
    : `cacheRefreshCount` + 2 exactement sur tout le geste (un au soulèvement des deux, un au dépôt), grâce au
    regroupement par tour de L3-T13.
  - `undoAfterEraseRestores` : après la gomme, `trigger("edit.undo")` → 2 objets, empreinte d'avant la gomme.
  - `liftDoesNotCancelTheLiftingTool` (B1) : sélection, glisser un trait : pendant le geste `liftLayer.objectCount
    == 1` et `appTools` toujours occupé ; au relâché l'objet est déplacé (et non perdu) ; même essai avec la gomme :
    les morceaux sont dans la page au relâché.
  - `pageChangeWhileDraggingClearsTheLiftLayer` (M2) : pendant un glissement, `trigger("page.next")` →
    `liftLayer.objectCount == 0` aussitôt, l'objet est à sa place d'origine sur la page 1, aucune étape ajoutée ; le
    relâché qui suit ne dépose rien.
  - `escapeCancelsTheGesture` (M9) : pendant un glissement, touche Échap injectée → objet à sa place, couche vide,
    outil courant toujours `select` ; Échap au repos avec le stylo → outil `select`.
  `tests/tst_modules.cpp` : `toolsSeeNoRenderItem` — aucun fichier de `tools/include` ni `tools/src` (lus depuis
  `ACTCRAFT_SOURCE_DIR` ou le chemin que le test utilise déjà) ne contient `#include <actcraft/render/` ni
  `#include <actcraft/ui/`. À `tests/tst_ui_drawing.cpp` : les cas existants passent avec `tools` posé
  (`Main.qml`) ; ajouter `fingerStrokeStillDraws` (un doigt injecté trace avec l'outil stylo, S3 tenu).
  Commande ciblée → rouge.
- [ ] **Étape 2 : implémentation** d'après l'objectif ; l'outil courant est créé à la première demande, gardé par
  identifiant (une instance par outil et par utilisateur).
- [ ] **Étape 3 : vert, preuves, commit.**
  ```powershell
  git -C <wt> add ui/include/actcraft/ui/toolhost.h ui/src/toolhost.cpp ui/src/tooladapters.h ui/src/tooladapters.cpp ui/src/penadapters.h ui/src/penadapters.cpp ui/qml/Main.qml tests/tst_ui_tools.cpp tests/tst_ui_drawing.cpp tests/tst_modules.cpp
  git -C <wt> commit -m "ui : outils branchés par leurs interfaces, mémoire par outil gardée au redémarrage (L3-T14)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t15"></a>
### Tâche L3-T15 — QML : cadre de sélection, poignées, barre d'objet, réglette ; scénario `selection`

**Objectif.** Dessiner `appTools.selectionFrame` (S34) : cadre fin aux quatre coins (polygone, il pivote avec
l'objet), huit poignées rondes grises, la poignée bas droite barrée d'un trait (F-520), barre de boutons carrés aux
icônes des commandes (F-521), rectangle élastique gris ; tout placé par `pageView.pageToScene`, réévalué sur
`zoomChanged` et `panChanged` ; masqué quand `handlesVisible` est faux (poignées et barre) ; la surcouche **ne prend
aucune entrée** (les clics vont à la vue, donc au routeur et à l'outil). `TranslucencyPopup.qml` : réglette 0 à 255
ouverte par `object.translucency` sans argument (le gestionnaire de `Main.qml` l'ouvre via
`selectionOverlay.openTranslucency()`, à ajouter ici et appelé par un gestionnaire déjà posé : si le gestionnaire
de L3-T2 ne l'appelle pas, l'écrire dans le rapport, ne pas toucher `Main.qml`), chaque mouvement déclenchant
`object.translucency <valeur>` (une étape par fusion, L3-T4). Aspect à nous (thème `Theme`), lisible à 3:1 sur la
page et sur le fond.

**Fichiers.** Modifier (talons) : `ui/qml/SelectionOverlay.qml`, `ui/qml/ObjectBar.qml`,
`ui/qml/TranslucencyPopup.qml`, `bench/scenarios/selection.json` ; modifier `bench/src/fixtures.cpp` (M8) : deux
fixtures nommées, écrites par nous comme les existantes — `rectangle-et-traits` (page 1024 × 768 : un rectangle plein gris
#808080 (100, 100, 100, 100) sur la couche intermédiaire, deux traits sur la couche du dessus, y 300 et 400, x 300 à 500) et
`objet-verrouille` (un rectangle plein #808080 libre (100, 100, 100, 100) et un rectangle plein #404040 verrouillé
(400, 100, 100, 100), `common.locked = true`). L3-T16 utilise la première.

**Test et commande bornée.** Preuve de tâche N ≥ 111, `tst_captures_selection` Passed ; preuve visuelle hors écran :
`-Scenario selection -Lire "appTools.selectionCount=2"` → `LU appTools.selectionCount=2 OK`, puis les captures
décrites (cadre, poignées, barre, déplacement) ; banc sans écran.

**Interdits.** Aucune entrée prise par la surcouche (`enabled: false` sur les éléments décoratifs, aucun
`MouseArea` ni `TapHandler` sur la page) ; aucune icône d'origine ; ne pas toucher `Main.qml`, `PageArea.qml`.

**Terminé quand.** Le scénario `selection` passe (`SCENARIO OK`, `CAPTURES OK`) ; captures décrites dans le rapport.

**Étapes.**
- [ ] **Étape 1 : scénario rouge.** `bench/scenarios/selection.json` :
  ```json
  {
    "format": "actcraft-scenario/1",
    "name": "selection",
    "description": "S34 : cliquer, rectangle qui touche, glisser, poignée proportionnelle, agrandir.",
    "steps": [
      {"waitReady": true},
      {"stroke": [[300, 300], [500, 300]], "pressure": 0.5},
      {"stroke": [[300, 400], [500, 400]], "pressure": 0.5},
      {"waitReady": true},
      {"command": "tool.select"},
      {"expect": "appTools.currentTool", "value": "select"},
      {"stroke": [[400, 300], [400, 300]], "pressure": 0.5},
      {"expect": "appTools.selectionCount", "value": 1},
      {"expect": "appTools.handlesVisible", "value": true},
      {"capture": "un-trait-selectionne"},
      {"stroke": [[700, 600], [700, 600]], "pressure": 0.5},
      {"expect": "appTools.selectionCount", "value": 0},
      {"stroke": [[250, 250], [320, 450]], "pressure": 0.5},
      {"expect": "appTools.selectionCount", "value": 2},
      {"capture": "deux-traits"},
      {"stroke": [[400, 300], [450, 320], [500, 340], [550, 350]], "pressure": 0.5},
      {"waitReady": true},
      {"checkPixel": "trait-deplace", "at": [550, 350], "color": "#000000", "tolerance": 80},
      {"checkPixel": "place-quittee", "at": [300, 300], "color": "#FFFFFF", "tolerance": 8},
      {"command": "object.enlarge"},
      {"waitReady": true},
      {"checkPixel": "agrandi-deborde", "at": [655, 345], "color": "#000000", "tolerance": 80},
      {"capture": "agrandi"},
      {"fixture": "objet-verrouille", "path": "${SORTIE}/s34-verrou.actcraft"},
      {"command": "file.open", "argument": "${SORTIE}/s34-verrou.actcraft"},
      {"waitFor": "appDocument.busy", "value": false, "timeoutMs": 10000},
      {"waitReady": true},
      {"command": "tool.select"},
      {"stroke": [[150, 150], [150, 150]], "pressure": 0.5},
      {"expect": "appTools.selectionCount", "value": 1},
      {"stroke": [[200, 200], [250, 225], [300, 250]], "pressure": 0.5},
      {"waitReady": true},
      {"checkPixel": "poignee-proportionnelle", "at": [280, 280], "color": "#808080", "tolerance": 24},
      {"capture": "poignee-glissee"},
      {"stroke": [[450, 150], [500, 200], [550, 250]], "pressure": 0.5},
      {"waitReady": true},
      {"expect": "appTools.selectionCount", "value": 1},
      {"checkPixel": "verrou-en-place", "at": [450, 150], "color": "#404040", "tolerance": 24},
      {"checkPixel": "verrou-pas-deplace", "at": [550, 250], "color": "#FFFFFF", "tolerance": 8},
      {"capture": "verrouille"}
    ]
  }
  ```
  Compléments (M8) : `agrandi-deborde` — après `object.enlarge` (× 1,1 autour du centre des deux traits déplacés,
  x 450 à 650, y 350 et 450, centre (550, 400)), le premier trait va de x 440 à 660 en y 345 : le point (655, 345)
  n'était pas encré avant ; `poignee-proportionnelle` — la poignée bas droite (200, 200) du rectangle libre glissée
  à (300, 250) : facteurs 2 et 1,5, le plus grand gardé, rectangle 200 × 200, donc (280, 280) gris ; sans la règle
  proportionnelle (200 × 150), ce point serait blanc ; `verrou-*` — le rectangle verrouillé ne bouge pas au
  glisser. Recaler les points d'au plus 3 px sur la première capture si l'épaisseur diffère, et l'écrire.
  (Les coordonnées de `stroke` sont des coordonnées de page ; un trait d'un seul point répété est un clic. Si
  l'injecteur exige deux points distincts pour un clic, décaler le second d'un dixième de pixel.) Rouge tant que
  la surcouche n'existe pas ? Les lectures passent déjà (L3-T14) ; le rouge porte sur `checkPixel` d'une poignée :
  ajouter après `un-trait-selectionne` `{"checkPixel": "poignee", "at": [300, 300], "color": "<gris des poignées du
  thème>", "tolerance": 40}` (coin haut gauche du cadre, au zoom de la page).
- [ ] **Étape 2 : QML.** `SelectionOverlay.qml` : `Repeater` sur `handles`, `Shape`/`Canvas` léger pour le cadre
  (polygone de `corners`) ; `ObjectBar.qml` : `Repeater` sur `bar` (icône `appCommands.iconUrl(command)`) ;
  `TranslucencyPopup.qml` : `Popup` avec `Slider` (le `PageView.inputBlocked` suit déjà les fenêtres surgissantes).
- [ ] **Étape 3 : preuves, commit.**
  ```powershell
  git -C <wt> add ui/qml/SelectionOverlay.qml ui/qml/ObjectBar.qml ui/qml/TranslucencyPopup.qml bench/scenarios/selection.json
  git -C <wt> commit -m "ui : cadre de sélection, poignées, barre d'objet, réglette de translucidité ; scénario S34 (L3-T15)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t16"></a>
### Tâche L3-T16 — QML : corbeille, menu contextuel, curseurs ; scénarios `corbeille` et `marqueur`

**Objectif.** `TrashCan.qml` (F-050) : icône de corbeille sous la palette, flèche rouge quand `appTools.trashHover`,
clic → `edit.delete` ; sa zone publiée dans `appTools.trashArea` (coordonnées de scène, recalculée à chaque
changement de géométrie) ; visible selon `view.trash`. `PageContextMenu.qml` (F-202, partie) : `popupAt(x, y)` ouvre
un menu selon `appTools.currentTool` : outil Sélectionner → « Stylo » (`tool.pen`) ; stylo ou marqueur →
« Sélectionner » (`tool.select`) ; libellés lus des commandes. `ToolCursor.qml` (F-201) : `cursorShape` selon
l'outil (flèche pour la sélection, croix pour le stylo ; pour le marqueur et la gomme, `Qt.BlankCursor` et un
**cercle** de la largeur de l'outil, au zoom, sous le pointeur — F-201 : « marqueur avec cercle, gomme avec
cercle ») ; l'indicateur de largeur existant (F-138) reste pour le stylo
et le marqueur. Scénarios S35 et S37.

**Fichiers.** Modifier (talons) : `ui/qml/TrashCan.qml`, `ui/qml/PageContextMenu.qml`, `ui/qml/ToolCursor.qml`,
`bench/scenarios/corbeille.json`, `bench/scenarios/marqueur.json`.

**Test et commande bornée.** Preuve de tâche N ≥ 111, `tst_captures_corbeille` et `tst_captures_marqueur` Passed ;
preuves visuelles hors écran : `-Scenario corbeille -Lire "appDocument.shownObjectCount=1"`, `-Scenario marqueur
-Lire "appTools.currentTool=pen"`, lectures d'abord, captures décrites ensuite ; banc sans écran.

**Interdits.** Comme L3-T15 ; aucune icône d'origine ; le menu contextuel n'offre que des commandes existantes.

**Terminé quand.** Les deux scénarios passent ; captures décrites.

**Étapes.**
- [ ] **Étape 1 : scénarios rouges.** `bench/scenarios/corbeille.json` :
  ```json
  {
    "format": "actcraft-scenario/1",
    "name": "corbeille",
    "description": "S35 : effacer les annotations et annuler, Suppr sans sélection, corbeille, rétablir la page.",
    "steps": [
      {"waitReady": true},
      {"fixture": "rectangle-et-traits", "path": "${SORTIE}/s35.actcraft"},
      {"command": "file.open", "argument": "${SORTIE}/s35.actcraft"},
      {"waitFor": "appDocument.busy", "value": false, "timeoutMs": 10000},
      {"waitReady": true},
      {"expect": "appDocument.shownObjectCount", "value": 3},
      {"command": "clear.annotations"},
      {"waitReady": true},
      {"expect": "appDocument.shownObjectCount", "value": 1},
      {"command": "edit.undo"},
      {"expect": "appDocument.shownObjectCount", "value": 3},
      {"remember": "appDocument.fingerprint", "as": "enregistre"},
      {"command": "tool.select"},
      {"key": "Del"},
      {"expect": "appDocument.shownObjectCount", "value": 3},
      {"expect": "appDocument.pageCount", "value": 1},
      {"stroke": [[150, 150], [150, 150]], "pressure": 0.5},
      {"expect": "appTools.selectionCount", "value": 1},
      {"command": "edit.delete"},
      {"expect": "appDocument.shownObjectCount", "value": 2},
      {"command": "edit.undo"},
      {"expect": "appDocument.shownObjectCount", "value": 3},
      {"command": "tool.pen"},
      {"stroke": [[600, 600], [700, 650]], "pressure": 0.5},
      {"waitReady": true},
      {"expect": "appDocument.shownObjectCount", "value": 4},
      {"command": "page.revert"},
      {"waitReady": true},
      {"expect": "appDocument.fingerprint", "value": "${enregistre}"},
      {"capture": "page-retablie"}
    ]
  }
  ```
  La fixture `rectangle-et-traits` est écrite par L3-T15 (`bench/src/fixtures.cpp`, M8) : rectangle en
  (100, 100, 100, 100), d'où le clic en (150, 150). Le clic sur la corbeille se joue par `edit.delete` (la
  corbeille est hors de la page ; son clic est prouvé par la capture et `trashCan.visible`).
  `bench/scenarios/marqueur.json` :
  ```json
  {
    "format": "actcraft-scenario/1",
    "name": "marqueur",
    "description": "S37 : marqueur jaune 20 par défaut, croisement uniforme, double passage plus foncé, mémoire par outil.",
    "steps": [
      {"waitReady": true},
      {"command": "pen.color", "argument": "#E53935"},
      {"command": "pen.width", "argument": 8},
      {"command": "tool.highlighter"},
      {"expect": "appTools.width", "value": 20},
      {"stroke": [[200, 300], [500, 300], [500, 500], [350, 200]], "pressure": 0.5},
      {"stroke": [[200, 600], [500, 600]], "pressure": 0.5},
      {"stroke": [[200, 600], [500, 600]], "pressure": 0.5},
      {"waitReady": true},
      {"checkPixel": "croisement", "at": [400, 300], "color": "#FFFF80", "tolerance": 30},
      {"checkPixel": "branche", "at": [300, 300], "color": "#FFFF80", "tolerance": 30},
      {"checkPixel": "double-passage", "at": [350, 600], "color": "#FFFF40", "tolerance": 30},
      {"capture": "marqueur"},
      {"command": "tool.pen"},
      {"expect": "appTools.width", "value": 8},
      {"expect": "pageView.inkColor", "value": "#e53935"}
    ]
  }
  ```
  (Couleurs attendues : jaune à 50 % sur blanc ≈ `#FFFF80`, deux passages ≈ `#FFFF40` ; recaler la tolérance sur la
  première capture et l'écrire.) Rouges tant que les QML sont des talons ? Les lectures passent (L3-T14) ; le rouge
  porte sur `{"expect": "trashCan.visible", "value": true}` et `{"expect": "toolCursor.cursorShape", "value": 10}`
  (`Qt.BlankCursor` et cercle, mineur 5) à ajouter en tête de `marqueur.json` après `tool.highlighter`.
- [ ] **Étape 2 : QML** d'après l'objectif.
- [ ] **Étape 3 : preuves, commit.**
  ```powershell
  git -C <wt> add ui/qml/TrashCan.qml ui/qml/PageContextMenu.qml ui/qml/ToolCursor.qml bench/scenarios/corbeille.json bench/scenarios/marqueur.json
  git -C <wt> commit -m "ui : corbeille, menu contextuel de la page, curseurs par outil ; scénarios S35 et S37 (L3-T16)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t19"></a>
### Tâche L3-T19 — Banc : scénarios `gommer` et `soulever`

**Objectif.** H10-8 (e) et S36 : mesurer, sur la page générée de 300 objets, un passage de gomme qui traverse des
traits versés et le glissement d'un objet versé, comme les autres scénarios (figements, reprises, `cache.refreshes`).
`ScenarioPlan` gagne un champ `toolCommand` (commande déclenchée par le joueur avant la mesure, vide pour les
scénarios existants) ; le joueur (`runner.cpp`) la déclenche par `Commands::trigger` de la fenêtre chargée, puis
`tool.pen` après la mesure.

**Fichiers.** Modifier : `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp`, `bench/src/runner.cpp`,
`tests/tst_bench_scenarios.cpp`, `tests/tst_bench_result.cpp`, `docs/banc.md`.

**Test et commande bornée.** Commande ciblée `tst_bench_scenarios|tst_bench_result` ; preuve de tâche N ≥ 111 ;
fonctions en `PASS` ; banc sans écran : `gommer` et `soulever` présents dans le JSON, `cache.refreshes` relevé,
`content_check.ok = True`.

**Interdits.** Scénarios existants inchangés (mêmes entrées, mêmes noms) ; `allScenarios` les rend dans l'ordre
`stroke, zoom, scroll, page, pauses, gommer, soulever` ; aucun verdict de budget changé (résultat, DA7).

**Terminé quand.** Tests en `PASS` ; `docs/banc.md` décrit les deux scénarios ; JSON du banc joint.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** `tst_bench_scenarios` : `eraseScenarioCrossesStrokes` (plan `gommer` :
  `toolCommand == "tool.eraser"`, entrées stylet en zigzag sur la page entière, à `kStylusPeriodNs`, qui croisent au
  moins 20 traits de la page générée par `core::generatePage` — compte par `core::objectsTouching` sur l'emprise de
  chaque segment), `liftScenarioDragsAnObject` (plan `soulever` : `toolCommand == "tool.select"`, rectangle de sélection
  qui prend **deux** objets versés de la page générée (M3 : le cas à plusieurs objets est celui qui coûtait un
  redessin par objet), puis appui sur l'un d'eux choisi par `core::objectAt` et glissement de 300 px en 2 s ; le JSON
  du scénario porte `cache.refreshes`, attendu 2). Le plan `gommer` écrit aussi dans son JSON `touched_strokes` (k),
  pour l'attendu `cache.refreshes ≤ k + 1` de S36. `tst_bench_result` :
  `sevenScenariosWithFrames` (remplace le compte de scénarios du lot 10 : le cas existant qui compte cinq scénarios
  est **mis à jour**, la raison écrite dans le rapport).
- [ ] **Étape 2 : implémentation**, puis `docs/banc.md` (deux paragraphes, nos mots).
- [ ] **Étape 3 : vert, banc, commit.**
  ```powershell
  git -C <wt> add bench/include/actcraft/bench/scenarios.h bench/src/scenarios.cpp bench/src/runner.cpp tests/tst_bench_scenarios.cpp tests/tst_bench_result.cpp docs/banc.md
  git -C <wt> commit -m "bench : scénarios gommer et soulever, outil choisi par commande avant la mesure (L3-T19)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t18"></a>
### Tâche L3-T18 — Scénario S6 `ecrire-gommer-annuler`

**Objectif.** S6 du cadrage, avec l'écart 2 (décompte des Annuler) et H10-1 (annulation d'un trait encore en
attente, à l'écran en D3D11). Joué hors écran par `tst_captures_ecrire_gommer_annuler`, puis à l'écran.

**Fichiers.** Modifier (talon) : `bench/scenarios/ecrire-gommer-annuler.json`.

**Test et commande bornée.** Preuve de tâche N ≥ 111, `tst_captures_ecrire_gommer_annuler` Passed ; preuve visuelle
hors écran `-Scenario ecrire-gommer-annuler -Lire "appDocument.shownObjectCount=1"` (état final) ; à l'écran
(jeton `ecran`, annoncé par l'orchestrateur) : même commande avec `-Rendu d3d11` sans `-HorsEcran` → `CAPTURES OK`.

**Interdits.** Aucun fichier de code : si une étape manque au joueur ou une lecture à l'interface, l'écrire dans le
rapport et s'arrêter (tâche corrective).

**Terminé quand.** Hors écran et à l'écran : `SCENARIO OK`, `CAPTURES OK` ; captures décrites (deux traits, trois
morceaux, déplacés, rendus, zoom).

**Étapes.**
- [ ] **Étape 1 : scénario.**
  ```json
  {
    "format": "actcraft-scenario/1",
    "name": "ecrire-gommer-annuler",
    "description": "S6 : deux traits ; la gomme coupe le premier (3 objets) ; rectangle qui prend les deux morceaux ; glisser ; deux Annuler rendent les 2 traits ; Rétablir ; trait en attente annulé ; pincement autour de son centre.",
    "steps": [
      {"waitReady": true},
      {"command": "view.zoom", "argument": 100},
      {"stroke": [[200, 300], [600, 300]], "pressure": 0.5},
      {"stroke": [[200, 500], [600, 500]], "pressure": 0.5},
      {"waitReady": true},
      {"remember": "appDocument.fingerprint", "as": "deux-traits"},
      {"command": "tool.eraser"},
      {"stroke": [[400, 260], [400, 340]], "pressure": 0.5},
      {"waitReady": true},
      {"expect": "appDocument.shownObjectCount", "value": 3},
      {"checkPixel": "trou", "at": [400, 300], "color": "#FFFFFF", "tolerance": 8},
      {"capture": "gomme"},
      {"command": "tool.select"},
      {"stroke": [[180, 280], [620, 320]], "pressure": 0.5},
      {"expect": "appTools.selectionCount", "value": 2},
      {"stroke": [[250, 300], [260, 340], [270, 380]], "pressure": 0.5},
      {"waitReady": true},
      {"checkPixel": "morceau-deplace", "at": [270, 380], "color": "#000000", "tolerance": 80},
      {"capture": "deplaces"},
      {"command": "edit.undo"},
      {"command": "edit.undo"},
      {"waitReady": true},
      {"expect": "appDocument.fingerprint", "value": "${deux-traits}"},
      {"checkPixel": "trait-rendu", "at": [400, 300], "color": "#000000", "tolerance": 80},
      {"command": "edit.redo"},
      {"waitReady": true},
      {"expect": "appDocument.shownObjectCount", "value": 3},
      {"command": "edit.undo"},
      {"command": "tool.pen"},
      {"waitReady": true},
      {"stroke": [[200, 650], [600, 650]], "pressure": 0.5},
      {"expect": "fixedPage.pendingCount", "value": 1},
      {"remember": "pageView.cacheRefreshCount", "as": "redessins"},
      {"command": "edit.undo"},
      {"expect": "fixedPage.pendingCount", "value": 0},
      {"expect": "pageView.cacheRefreshCount", "value": "${redessins}"},
      {"checkPixel": "attente-annulee", "at": [400, 650], "color": "#FFFFFF", "tolerance": 8},
      {"command": "edit.undo"},
      {"waitReady": true},
      {"expect": "appDocument.shownObjectCount", "value": 1},
      {"pinch": [400, 300], "from": 100, "to": 150, "steps": 12},
      {"waitReady": true},
      {"expect": "pageView.zoom", "value": 1.5, "tolerance": 0.02},
      {"checkPixel": "centre-du-pincement", "at": [400, 300], "color": "#000000", "tolerance": 80},
      {"capture": "pincement"}
    ]
  }
  ```
  Notes : `expect` de `pendingCount` juste après le trait suppose le versement différé du lot 10 (500 ms
  d'inactivité) : le trait est encore en attente si l'étape suit sans `wait` ; si L10-T6 a changé le délai, garder
  l'enchaînement sans attente. `${redessins}` comparé tel quel (aucun redessin entre les deux lectures, H10-1).
  Après l'annulation du trait en attente, la pile rend la page à « deux traits » ; le dernier `edit.undo` retire le
  second trait (écart 2 : il reste 1 objet, le premier trait, sous le centre du pincement). Si `checkPixel` lit des
  coordonnées de page, il ne prouve pas l'ancrage du pincement : `tst_pageview` (`pinchZoomsAroundItsCenter`) le
  prouve ; le scénario prouve le zoom et l'encre sous le centre.
- [ ] **Étape 2 : hors écran**, puis rapport : lectures `LU …` d'abord, captures décrites.
- [ ] **Étape 3 : à l'écran en D3D11** (jeton `ecran`, après l'annonce de l'orchestrateur) ; rapport identique.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add bench/scenarios/ecrire-gommer-annuler.json
  git -C <wt> commit -m "bench : scénario S6 écrire, gommer, sélectionner, annuler, pincer (L3-T18)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

## Hypothèses à revoir (porte de fin du lot 3)

### Niveau 1

- **H3-1 — Schéma d'entrée du second utilisateur** (DA-A1) : tranché par L3-T0 (D1) ou, à défaut, laissé ouvert pour
  le lot 11 ; le lot 3 ne connaît qu'un utilisateur, tous les contacts lui sont rattachés.
- **H3-2 — Soulever** : pendant un glissement, les objets soulevés sont dessinés **au-dessus** de tout (couche
  soulevée, sous l'encre) ; ils retrouvent leur profondeur au relâché. Un redessin complet du cache au soulèvement et
  un au dépôt, quel que soit le nombre d'objets (demandes d'un même tour regroupées, L3-T13) ; deux figements
  possibles par geste ; mesuré par `soulever` (S36, attendu : 2 redessins). Hors du critère « 0 figement » de S13.
- **H3-3 — Gomme** : un redessin du cache par tour où la gomme touche un trait pour la première fois, plus un au
  relâché (≤ k + 1 pour k traits touchés) ; sur une page chargée, plusieurs figements possibles **pendant**
  l'écriture ; mesuré par `gommer` (S36), hors du critère « 0 figement » de S13. Repli si Jérôme les juge gênants :
  tuiles (L10-T7), ou gomme qui soulève d'un coup, à l'appui, toutes les annotations proches du point d'appui.

### Niveau 2

- **H3-4** Rectangle de sélection : un objet est « touché » quand son **emprise** touche le rectangle (pas son
  encre) ; D6 peut l'amender.
- **H3-5** S6 : deux Annuler (pas trois) rendent les deux traits (écart 2 ; D10).
- **H3-6** Marqueur vif : technique d'un seul tenant retenue par L3-T11 et son coût mesuré, rapportés.
- **H3-7** Mémoire par outil dans `QSettings` (`outils/<id>/…`) dès le lot 3 ; le lot 6 pourra la reprendre.
- **H3-8** Regroupement automatique des annotations (P10) et réglage du curseur du stylo (P17) : lot 6 (réglages).
- **H3-9** Gomme de 20 px par défaut (D4, D5).
- **H3-10** Barre d'objet réduite aux fonctions du lot ; les autres boutons au lot 5, à leur rang.
- **H3-11** Rétablir la page s'annule en une étape (D9).
- **H3-12** Rétablir : Ctrl+Y (D11).
- **H3-13** Effacer ▸ Objets : tout ce qui n'est pas une annotation, couche de fond comprise (D8).
- **H3-14** Tolérance de désignation 6 px d'écran ; seuil de glissement 3 px ; zone morte du pincement 3 %.
- **H3-15** Les traits lus d'un `.flipchart` sont aussi lissés (D2) : le test local des miniatures (`MINIATURES OK`,
  écart médian ≤ 2,0) est rejoué à la clôture ; un dépassement devient une tâche corrective.

## Décisions tranchées par ce plan

1. **Version 0.4.0** (amendement 1, § 6), pas une hypothèse.
2. **Soulever plutôt que fusionner des commandes par mouvement** : aucun redessin pendant un geste ; un objet
   soulevé quitte le document le temps du geste (Enregistrer, Ouvrir et les changements de page déposent d'abord).
3. **Un stylo pour deux outils** (stylo, marqueur) : un seul chemin d'encre.
4. **Réservation en deux tâches** (données et CMake, puis coquille QML) : les vagues suivantes n'écrivent plus dans
   les fichiers chauds en même temps.
5. **Le routeur décide à l'appui, une fois par contact** : le lot 11 change la politique, pas la vue.

## Clôture du lot 3 (orchestrateur, hors tâches)

Jouée après la dernière vague, dans le worktree du chantier, sur `feat/livraison-complete` à jour, avant la PR.

1. **Double relecture** (conformité, qualité) du diff du lot, chaque tâche `concepteur-critique` relue après rejeu ;
   correctifs `L3-C<n>` (deux tours au plus).
2. **Simplification** : tâche `L3-S1` (diff de `tests/` vide).
3. **Version 0.4.0** : `project(VERSION 0.4.0)` dans `CMakeLists.txt` ; `CHANGELOG.md` section `## 0.4.0 — <date>`
   (sélection, stylo et marqueur, gomme, annuler, effacer, corbeille, gestes à deux doigts) ;
   `scripts/notes-version.ps1` → `NOTES OK`.
4. **Suites** (jeton `tests`) : `scripts/test.ps1 -Preset release` puis `scripts/test.ps1`, formes bornées, N ≥ 111
   chacune, `TESTS OK` ; aucun test sauté hors les trois tests locaux du lot 2 (rejoués avec leurs variables :
   `ORACLE OK`, `ORACLE SYNTHETIQUE OK`, `MINIATURES OK`, H3-15) ; **aucun talon `QSKIP` restant** :
   `git -C <wt> grep -n -E "Talon \(?L3-T1|Talon L0-S" -- tests core input tools render ui bench` → sortie vide
   (un même marqueur dans les trois formes de talon : commentaire C++ et QML `// Talon L3-T1 …`, description JSON
   `Talon (L3-T1) …`, `QSKIP("Talon L3-T1 …")` ; le socle pose le sien, `Talon L0-S`). Les `tst_captures_*` talons
   passent avec `waitReady` seul : leur `Passed` ne prouve rien tant que ce contrôle n'est pas vide.
5. **Captures à l'écran en D3D11** (jeton `ecran`, annoncé) : `captures.ps1 -Scenario tous -Rendu d3d11 -Sortie
   .tmp/captures-ecran` → `SCENARIO OK` chacun (dont `selection`, `corbeille`, `marqueur`,
   `ecrire-gommer-annuler`, `anticrenelage`, `cache-differe`), `CAPTURES OK` ; captures regardées et décrites.
6. **Banc** (jeton `ecran`, machine relevée) : non-régression 0.3.0 → 0.4.0 → 0.3.0, `bench.ps1 -Rendu d3d11
   -SansConstruire` dans des worktrees détachés construits en release ; comparée critère par critère à la mesure de
   fin de lot 10 (`docs/mesures/…-cache-incremental.md`) sur `stroke`, `zoom`, `scroll`, `pauses` (figements et
   reprises compris, DA7) ; puis `bench.ps1 -Rendu tous` sur la 0.4.0 avec `gommer` et `soulever` (S36) ; résumé
   commité dans `docs/mesures/<date>-lot3-outils.md`. Hors limite : H3-2, H3-3, décision tuiles ou tâche
   corrective.
7. **Paquets** : `scripts/paquet.ps1` + `scripts/essai-installateur.ps1` (`PAQUET OK`, `INSTALLATEUR OK`,
   `--version : ActCraft 0.4.0`), `scripts/paquet-android.ps1` (`APK OK`, démon Gradle arrêté par PID) ;
   `essai-android.ps1` sur l'émulateur si disponible : un doigt trace (S3), deux doigts zooment.
8. **Dépôt public** : `NOMS OK` ; `git grep -n "Private\|_p\.h" -- core formats actions render ui app tools input`
   vide ; commande des titres du corpus de la section de L3-T0 lancée sur le worktree du chantier → sortie vide.
9. **Graphe** : `graphify update .`, commit de `graphify-out/`.
10. **PR du lot** (étage 4) et **porte de fin du lot 3** : release 0.4.0 proposée (publiée seulement sur go écrit) ;
    S6, S34 à S37 ; écarts 1 à 6 (dont le volume, 13 → 20 tâches) ; hypothèses H3-1 à H3-15 ; décisions D1 à D12 de
    L3-T0 ; reports au lot 7 (routeur, menu contextuel) et aux lots 5, 6 et 11.

## Correctifs de la relecture (2026-10-18, rapport `revue-plan-lot3.md`)

| # | Constat | Sort | Où |
|---|---|---|---|
| B1 | L'outil qui soulève était annulé par son propre soulèvement (`editAboutToApply` au soulèvement → `cancel()`) | corrigé : le soulèvement n'annonce rien ; `about.count() == 0` ; test `liftDoesNotCancelTheLiftingTool` | L3-T12, L3-T14 ; API du socle (sans `editAboutToApply` au soulèvement) |
| M1 | Garde-fous du lot 10 liés au stylo interne (minuterie de versement, `finishStroke`, sonde, `Pipeline`) | corrigé : chemin d'entrée du socle avec `Pipeline` devant l'outil routé ; minuterie et `finishStroke` sur `isBusy` / `cancel` de l'outil actif ; tests `foldWaitsForTheActiveTool`, `latencyProbeSeesToolSamples`, `pageChangeEndsToolStroke` | L3-T13 (dépend de L0-S), L3-T14 |
| M2 | Dépôt forcé par le contrôleur : couche soulevée laissée pleine | corrigé : `pageAboutToChange` / `editAboutToApply` avant le dépôt forcé, l'outil abandonne son geste ; test `pageChangeWhileDraggingClearsTheLiftLayer` | L3-T12, L3-T14 |
| M3 | Coût du soulèvement sous-estimé (un redessin par objet, `m_refreshAgain`) | corrigé : **règle retenue** — demandes de redessin d'un même tour regroupées dans `PageView` (L3-T13) ; cas à deux objets (`dragOfTwoObjectsCostsTwoRedraws`, scénario `soulever` à deux objets) ; `gommer` et `soulever` **exclus** du « 0 figement » de S13, attendus chiffrés (`soulever` : 2 redessins ; `gommer` : ≤ k + 1) | L3-T13, L3-T14, L3-T19, S36, H3-2, H3-3 |
| M4 | Pincement : le premier doigt traçait ; stylet et doigt mêlés | corrigé : appuis d'un même événement décidés ensemble ; stylet qui prend la main ; tests `simultaneousPinchDrawsNothing`, `stylusTakesOverFromFinger` (au socle si le socle crée le routeur) | L3-T5 |
| M5 | Talons QML sans `required property`, réglette jamais ouverte | renvoyé au socle (emplacements QML avec leurs `required property`, `openTranslucency()`) | L0-S ; L3-T1, L3-T2 à amender |
| M6 | Entrées du lot 7 hors du routeur ; clic droit en conflit | renvoyé au socle (chemin d'entrée, routes des surcouches) | L0-S |
| M7 | Travail en double avec le lot 7 (sous-menus, commande de remplacement, zoom ancré) | renvoyé au socle | L0-S ; L3-T1, L3-T4, L3-T7 à amender |
| M8 | S35 sans fixture, S34 incomplet | corrigé : `bench/src/fixtures.cpp` dans L3-T15 (`rectangle-et-traits`, `objet-verrouille`), S34 complété (poignée proportionnelle, agrandir vérifié, objet verrouillé) ; L3-T16 dépend de L3-T15 | L3-T15, L3-T16 |
| M9 | Échap jamais livré à `Tool::onKey` | corrigé : `activate()` annule l'outil occupé ; Échap pendant un geste = abandon ; test `escapeCancelsTheGesture` | L3-T14 |
| m1 | Dépendances : T11 et T2 sur `PageArea.qml` ; T14 sur T9 et T11 sans raison | corrigé : T11 dépend de T2 ; T14 ne dépend plus de T9 ni de T11 ; T15 dépend de T9 ; vagues V6 T9 ‖ T14, 8 vagues après le socle | table, vagues |
| m2 | Marqueur des talons | corrigé : contrôle de clôture sur les trois formes (`Talon \(?L3-T1`, `Talon L0-S`) | clôture, étape 4 |
| m3 | S6 : `waitReady` avant le trait en attente | corrigé | L3-T18 |
| m4 | Ctrl+Maj+Z pris par le lot 7 | corrigé : D11 garde Ctrl+Y, conflit rendu au socle si la 3.5.18 dit autre chose | L3-T0 |
| m5 | Curseur du marqueur : cercle (F-201) | corrigé : `Qt.BlankCursor` + cercle (attendu 10) | L3-T16 |
| m6 | `saveWhileLiftingIsRefused` testait un dépôt | corrigé : renommé et dédoublé (`pageChangeWhileLiftingDropsFirst`, `saveWhileLiftingDropsFirst`) | L3-T12 |
| m7 | Trois demandes d'un tour → deux fins | corrigé : `cacheRefreshCountCounts` attend chaque fin ; `requestsOfOneTurnMakeOneRedraw` ajouté | L3-T13 |
