# Livraison complète — lot 3 « Outils I » — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**Démarrage : après L0-S10.** Toutes les tâches de ce plan démarrent sur `feat/livraison-complete` **après la tâche
L0-S10** (contrôle du socle), dernière tâche du socle commun des lots 3 et 7 (plan
`docs/plans/2026-10-18-livraison-complete-socle-3-7-PLAN.md`, 9b13275). Aucune tâche du lot 3 ne part avant que L0-S10
soit mergée.

**But.** Version **0.4.0** (amendement 1, § 6 : lot 3 → 0.4.0, le lot 10 livre 0.3.0). Écrire, gommer, sélectionner,
annuler, pincer pour zoomer : sélection (clic, rectangle qui touche, déplacer, huit poignées, redimensionner,
rotation, barre d'objet), stylo et marqueur (mémoire par outil, translucidité du marqueur), gomme qui coupe et scinde
les seules annotations, Annuler / Rétablir, Supprimer, corbeille, Effacer ▸, Rétablir la page, gestes à deux doigts
(décision 6 du cadrage ; F-050, F-130, F-131, F-132, F-136, F-137, F-201, F-202 (partie), F-210, F-225, F-230,
F-520 à F-524, F-528, F-541, F-951 à F-953 pour les fonctions du lot). Scénario d'acceptation du cadrage : **S6** ;
tâche d'ouverture : **S30** ; scénarios ajoutés par ce plan : S34 à S37.

**Ce que le socle a déjà posé (ne plus le faire).** Commandes et disposition des deux lots (sous-menus, barre
d'objet, menu contextuel, raccourcis uniques, L0-S4) ; icônes (L0-S1) ; joueur de scénarios (`touch`, `pinch`,
`pointer`, `doubleClick`, tolérance d'`expect`, L0-S2) ; modèle partagé (`ObjectKind::Equation`, `PagePresentation`,
**`core::ReplaceObjectsCommand`** unique, C4062 en erreur, L0-S3) ; `DocumentController` : API des deux lots en corps
vides, observateur unique `addChangeObserver` (origines `Lift`, `Drop`…), `liftObjects` **sans** `editAboutToApply`
(L0-S5) ; en-têtes figés et talons du lot 3, `LiftLayer` exposé au QML, `UserState::openMathTools` et
`ToolHost::user()` réels (L0-S6) ; chemin d'entrée figé de la vue : événement → `Sample` → `PointerRouter`
(couvertures, `Route::Cover`) → filtre d'échantillons → `Pipeline` → outil actif, garde du versement sur `isBusy`,
`finishStroke` par `cancel`, `contextMenuRequested`, `zoomAround`, `tools`, `cacheRefreshCount`, `router()` (L0-S7) ;
coquille QML : couches et emplacements avec leurs `required property`, gestionnaires des commandes du lot 3 (L0-S9) ;
enregistrement CMake de tous les fichiers et des **19 tests** (L0-S0). Les anciennes tâches **L3-T1, L3-T2 et L3-T17
sont retirées** (faites par le socle).

**Architecture.** Tout passe par une commande nommée (`actions/`) ; un outil (`tools/`) ne voit que des interfaces
(`tools::ToolContext`) ; toute modification du document passe par la pile de `core/`. Deux contraintes structurent le
lot :

1. **Routage unique du doigt (DA-A1 de l'amendement).** Le routeur du socle décide à l'appui (outil, geste,
   couverture ou rien) et tient la décision pour tout le contact ; le lot 3 écrit la politique `FullTouchPolicy`
   (geste à deux doigts, paume, stylet qui prend la main) et le geste ; l'utilisateur (`tools::UserState`) porte l'outil
   courant, les réglages par outil et la sélection.
2. **Cache par son interface figée (DA11), sans redessin pendant un geste.** Le patron évident — pousser une
   commande par mouvement, fusionnée dans la pile — redessinerait tout le cache à chaque mouvement. Le lot 3
   **soulève** : au début d'un glissement (sélection) ou au premier contact de la gomme avec un trait, les objets visés
   sont retirés de la page affichée hors pile (`DocumentController::liftObjects`, le cache reçoit `remove`) ; la couche
   soulevée (`render::LiftLayerItem`) les dessine pendant le geste, déplacés par une transformation seule ou recoupés
   par la gomme ; au relâché, `dropLifted` remet les originaux, exécute **une** commande, et le cache reçoit `insert`
   pour chaque objet soulevé encore présent (`core::changesAfterLift`). Les demandes de redessin d'un même tour de
   boucle sont regroupées (L3-T13) : un redessin au soulèvement, un au dépôt. Aucun outil n'appelle la page fixe, la
   couche d'attente ni la couche soulevée (`tst_ui_lift`, `tst_ui_tools`, `tst_modules`).

**Pile.** C++20, Qt 6.12 (Core, Gui, Quick, QuickControls2, Test), CMake + Ninja, MSVC 2022, PowerShell 7.

**Historique du plan.** 60a4290 (20 tâches) → 61c03fb (correctifs de la relecture, en fin de plan) → ce texte
(amendements du socle, 17 points, relevé en fin de plan « Amendements du socle »). Les numéros de ligne du code cités
sont indicatifs, les noms font foi.

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L3-T0 | Ouverture : vidéos, REA et ancien dépôt confrontés au lot (sélection, stylo, marqueur, gomme, annulation, effacer, corbeille, gestes, schéma d'entrée) | L0-S10 | `docs/verifications/2026-10-18-lot3-principes.md` (hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot3.md`) | isolé | concepteur-critique | rapport commité, relevé brut hors dépôt présent, `NOMS OK`, `git grep` des titres du corpus vide, décisions D1 à D12 écrites | S30 | #tache-l3-t0 |
| L3-T3 | `core` : désignation (objet touché, rectangle qui touche) et géométrie de la gomme | L0-S10 | `core/src/hittest.cpp`, `core/src/erasegeometry.cpp`, `tests/tst_core_hittest.cpp`, `tests/tst_core_erase.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_départ + 19, `tst_core_hittest` (dont `topmostObjectWins`, `strokeHitUsesWidthAndTolerance`, `rectangleTouchingSelects`, `equationHitsLikeText`) et `tst_core_erase` (dont `middleCutSplitsInTwo`, `sweepDoesNotSkip`) Passed, aucun `reserved` | S6 | #tache-l3-t3 |
| L3-T10 | `render` : couche soulevée `LiftLayerItem` (dessin identique à la page fixe, transformation sans reconstruction) | L0-S10 | `render/src/liftlayeritem.cpp`, `tests/tst_render_liftlayer.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_départ + 19, `tst_render_liftlayer` Passed (dont `drawsLikeTheFixedPage`, `transformDoesNotRebuild`, `clearEmpties`) ; banc hors écran `content_check.ok = True` | S6 | #tache-l3-t10 |
| L3-T4 | `core` : commandes d'édition du lot et changements vus par le cache après un soulèvement | L3-T0, L3-T3 | `core/src/objectedits.cpp`, `core/src/lift.cpp`, `tests/tst_core_objectedits.cpp`, `tests/tst_core_lift.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_départ + 19, `tst_core_objectedits` (dont `clearAnnotationsKeepsShapes`, `revertPageIsOneStep`) et `tst_core_lift` (dont `liftThenDropMoveYieldsInsertOnly`, `liftThenCancelReinserts`, `erasePiecesInsertedNoRemoveOfLifted`) Passed ; mutation de `changesAfterLift` rouge puis remise | S6, S35 | #tache-l3-t4 |
| L3-T5 | `input` : politique `FullTouchPolicy` (geste à deux doigts, paume, stylet qui prend la main) et geste à deux doigts | L3-T0 | `input/src/routing.cpp` (corps de `FullTouchPolicy::decide` seulement), `input/src/gesture.cpp`, `tests/tst_input_routing.cpp`, `tests/tst_input_gesture.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_départ + 19, `tst_input_routing` (dont `secondFingerCancelsToolAndStartsGesture`, `palmIgnoredWhileStylusDown`, `stylusTakesOverFromFinger`, et les fonctions du socle `coverRouteHeldForWholeContact`, `coverLeavesTheStateAlone`, `pressRedecides` toujours vertes) et `tst_input_gesture` (dont `pinchScalesAroundCenter`) Passed | S6 | #tache-l3-t5 |
| L3-T6 | `tools` : registre, réglages par outil de l'utilisateur, stylo et marqueur | L3-T0 | `tools/src/userstate.cpp`, `tools/src/toolregistry.cpp`, `tools/include/actcraft/tools/pentool.h`, `tools/src/pentool.cpp`, `tests/tst_pentool.cpp`, `tests/tst_tools_registry.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_départ + 19, `tst_tools_registry` (dont `builtinsRegistered`, `userKeepsSettingsPerTool`, `defaultsPerTool`, et `openMathToolsKept` du socle) et `tst_pentool` (dont `highlighterStrokeIsTranslucentKind`, `newStrokesHaveRoundCaps`) Passed | S6, S37 | #tache-l3-t6 |
| L3-T11 | `render` : forme du trait (bouts et jonctions ronds, courbe lissée) et marqueur translucide d'un seul tenant | L3-T0 | `render/include/actcraft/render/strokegeometry.h`, `render/src/strokegeometry.cpp`, `render/include/actcraft/render/inkitem.h`, `render/src/inkitem.cpp`, `render/src/strokerenderer.cpp`, `render/src/pagepainter.cpp`, `ui/qml/PageArea.qml` (une propriété d'`InkLayer` au plus, aucune couche déplacée), `tests/tst_render_strokeshape.cpp`, `tests/tst_render_painter.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_départ + 19, `tst_render_strokeshape` (dont `roundCapsCoverTheDisk`, `selfCrossingHighlighterDoesNotDarken`, `twoHighlighterPassesDarken`) et `tst_render_painter` Passed ; banc hors écran `content_check.ok = True` ; à l'écran D3D11 (jeton `ecran`) `anticrenelage` `CAPTURES OK` | S37 | #tache-l3-t11 |
| L3-T12 | `DocumentController` : soulever et déposer, Annuler / Rétablir après le trait en cours, Supprimer, Effacer, Rétablir la page | L3-T4 | `ui/src/documentcontroller.cpp`, `tests/tst_ui_document.cpp`, `tests/tst_ui_lift.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_départ + 19, `tst_ui_lift` (dont `liftRemovesThroughTheCacheInterface`, `dropInsertsLiftedObjects`, `undoWhileLiftingCancelsFirst`, `pageChangeWhileLiftingDropsFirst`, `saveWhileLiftingDropsFirst`) et `tst_ui_document` (dont `revertPageReturnsToSaved`, `revertNeverSavedEmptiesPage`, `undoAnnouncesTheEditFirst`, et `liftDoesNotAnnounceAnEdit` du socle) Passed | S6, S35 | #tache-l3-t12 |
| L3-T7 | `tools` : gomme (soulève chaque trait touché une fois, une `ReplaceObjectsCommand` au relâché) | L3-T3, L3-T4, L3-T6 | `tools/src/erasertool.cpp`, `tests/tst_tools_eraser.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_départ + 19, `tst_tools_eraser` Passed (dont `liftsEachTouchedStrokeOnce`, `nothingExecutedDuringTheGesture`, `oneCommandAtRelease`, `shapesAreNeverTouched`, `cancelRestoresEverything`) | S6 | #tache-l3-t7 |
| L3-T8 | `tools` : sélection — désigner, rectangle qui touche, déplacer par soulèvement, déposer sur la corbeille | L3-T3, L3-T4, L3-T6 | `tools/src/selecttool.cpp`, `tools/src/selectiongeometry.cpp`, `tests/tst_tools_select.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_départ + 19, `tst_tools_select` Passed (dont `clickSelectsTopmost`, `rubberBandSelectsTouched`, `dragLiftsOnceAndDropsOnce`, `lockedObjectDoesNotMove`, `dropOverTrashDeletes`, `escapeCancelsDrag`) | S6, S34 | #tache-l3-t8 |
| L3-T9 | `tools` : sélection — huit poignées, redimensionner (proportionnel au coin), rotation, barre d'objet | L3-T8 | `tools/src/selecttool.cpp`, `tools/src/selectiongeometry.cpp`, `tests/tst_tools_select.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_départ + 19, `tst_tools_select` Passed (dont `eightHandlesAroundTheFrame`, `cornerHandleResizesProportionally`, `rotationAroundCenter`, `handlesHiddenWhileDragging`, `barButtonTriggersCommand`, `restrictionsHonoured`) | S34 | #tache-l3-t9 |
| L3-T14 | `ui::ToolHost` : outils branchés (`activeTool`, commandes, mémoire par outil gardée au redémarrage, adaptateurs d'interfaces, Échap) | L3-T7, L3-T8, L3-T10, L3-T12 | `ui/src/toolhost.cpp`, `ui/src/tooladapters.h`, `ui/src/tooladapters.cpp`, `ui/src/penadapters.h`, `ui/src/penadapters.cpp`, `ui/qml/Main.qml`, `tests/tst_ui_tools.cpp`, `tests/tst_ui_drawing.cpp`, `tests/tst_modules.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_départ + 19, `tst_ui_tools` (dont `colorGoesToActiveTool`, `settingsSurviveRestart`, `eraseGoesThroughTheCacheInterface`, `dragGoesThroughTheCacheInterface`, `undoAfterEraseRestores`, `liftDoesNotCancelTheLiftingTool`, `pageChangeWhileDraggingClearsTheLiftLayer`, `escapeCancelsTheGesture`, `finishStrokeCancelsTheActiveTool`, et `oneUserForNow`, `liftLayerTypeIsRegistered` du socle), `tst_modules` (dont `toolsSeeNoRenderItem`), `tst_ui_drawing` Passed ; `premiers-pas` hors écran `CAPTURES OK` ; banc hors écran `content_check.ok = True` | S6, S37 | #tache-l3-t14 |
| L3-T13 | `PageView` : gestes à deux doigts (pincer, défiler), demandes de redessin regroupées par tour, outil actif de `ToolHost` occupé | L3-T5, L3-T6, L3-T14 | `ui/src/pageview.cpp`, `tests/tst_pageview.cpp`, `tests/tst_ui_drawing.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_départ + 19, `tst_pageview` (dont `pinchZoomsAroundItsCenter`, `twoFingerPanScrolls`, `secondFingerEndsFingerStroke`, `palmIgnoredDuringStylus`, `requestsOfOneTurnMakeOneRedraw`, `foldWaitsForTheActiveTool`, `latencyProbeSeesToolSamples`, `pageChangeEndsToolStroke`) et `tst_ui_drawing` (dont `dragOfTwoObjectsCostsTwoRedraws`) Passed ; `tst_ui_inputpath` toujours vert ; banc hors écran `content_check.ok = True` | S6 | #tache-l3-t13 |
| L3-T15 | QML : cadre de sélection, poignées, barre d'objet, réglette de translucidité ; fixtures `rectangle-et-traits` et `objet-verrouille` ; scénario `selection` | L3-T9, L3-T13, L3-T14 | `ui/qml/SelectionOverlay.qml`, `ui/qml/ObjectBar.qml`, `ui/qml/TranslucencyPopup.qml`, `bench/src/fixtures.cpp`, `bench/scenarios/selection.json` | isolé | implementeur-tdd | `TESTS OK`, N = N_départ + 19, `tst_captures_selection` Passed ; `captures.ps1 -Scenario selection` hors écran : `LU appTools.selectionCount=2 OK`, `LU appTools.handlesVisible=true OK`, trait au stylet hors du cadre versé (`LU appDocument.shownObjectCount=…`), `CAPTURES OK`, au moins une ligne `TRAIT` ; banc hors écran `content_check.ok = True` | S34 | #tache-l3-t15 |
| L3-T19 | Banc : scénarios `gommer` et `soulever` (H10-8 e), outil choisi par commande avant la mesure | L3-T13, L3-T14 | `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp`, `bench/src/runner.cpp`, `tests/tst_bench_scenarios.cpp`, `tests/tst_bench_result.cpp`, `docs/banc.md` | isolé | implementeur-tdd | `TESTS OK`, N = N_départ + 19, `tst_bench_scenarios` (dont `eraseScenarioCrossesStrokes`, `liftScenarioDragsAnObject`) et `tst_bench_result` (dont `sevenScenariosWithFrames`) Passed ; banc hors écran : scénarios `gommer` et `soulever` présents, `cache.refreshes` dans l'attendu chiffré de S36 (`soulever` : 2 ; `gommer` : ≤ k + 1, plus le redessin du dernier trait encore en attente gommé en deux morceaux) | S36 | #tache-l3-t19 |
| L3-T16 | QML : corbeille, menu contextuel de la page, curseurs par outil ; scénarios `corbeille` et `marqueur` | L3-T14, L3-T15 | `ui/qml/TrashCan.qml`, `ui/qml/PageContextMenu.qml`, `ui/qml/ToolCursor.qml`, `bench/scenarios/corbeille.json`, `bench/scenarios/marqueur.json` | isolé | implementeur-tdd | `TESTS OK`, N = N_départ + 19, `tst_captures_corbeille` et `tst_captures_marqueur` Passed ; `captures.ps1 -Scenario corbeille` puis `marqueur` hors écran `CAPTURES OK` (lectures `LU appDocument.shownObjectCount=…` d'abord, au moins une ligne `TRAIT` chacun) | S35, S37 | #tache-l3-t16 |
| L3-T18 | Scénario S6 `ecrire-gommer-annuler` (pincement compris), rejoué à l'écran en D3D11 (H10-1) | L3-T13, L3-T14, L3-T15 | `bench/scenarios/ecrire-gommer-annuler.json` | isolé | implementeur-tdd | `TESTS OK`, N = N_départ + 19, `tst_captures_ecrire_gommer_annuler` Passed ; hors écran `CAPTURES OK` (`LU appDocument.shownObjectCount=3 OK` après la gomme, `LU pageView.zoom≈1.5 OK` après le pincement, lignes `TRAIT` et `PINCEMENT`) ; à l'écran D3D11 (jeton `ecran`) `CAPTURES OK`, `LU fixedPage.pendingCount=0 OK` et `LU pageView.cacheRefreshCount=<n> OK` après l'annulation du trait en attente | S6 | #tache-l3-t18 |

17 tâches, un seul plan. **N = N_départ + 19** : N_départ est le nombre de tests ctest relevé par L0-S0 au départ du
socle (noté dans l'état du chantier) ; le socle a enregistré les 19 tests (dont les 17 du lot 3, talons sautés
`QSKIP("Talon L0-S : rempli par L3-T<n>.")`) ; **aucune tâche du lot 3 n'enregistre de test**, ni n'en crée : elle
remplit ceux du socle (suppression de `reserved()`, fonctions du socle gardées). Un `SKIP` ne prouve rien : chaque
preuve nomme les fonctions attendues en `PASS`.

Worktrees : `.worktrees/livraison-complete--L3-T<n>`, branche `feat/livraison-complete--L3-T<n>`, issue de
`feat/livraison-complete` à jour (L0-S10 comprise). Tous isolés : deux tâches d'une même vague ne partagent aucun
fichier (vérifié sur la colonne « fichiers touchés »).

**Écran.** L3-T11 (preuve D3D11 du lissage conservé), L3-T18 (S6 à l'écran, H10-1) et la clôture ouvrent des
fenêtres à l'écran (D3D11) sous le jeton `ecran` : l'orchestrateur l'annonce à Jérôme d'une ligne **avant** de
lancer chacune. Aucune autre tâche n'ouvre de fenêtre visible.

## Vagues (largeur 3 au plus ; 2 places de tests ; aucun fichier partagé dans une vague)

V1 : L3-T0 ‖ L3-T3 ‖ L3-T10 · V2 : L3-T4 ‖ L3-T5 ‖ L3-T6 · V3 : L3-T11 ‖ L3-T12 · V4 : L3-T7 ‖ L3-T8 ·
V5 : L3-T9 ‖ L3-T14 · V6 : L3-T13 · V7 : L3-T15 ‖ L3-T19 · V8 : L3-T16 ‖ L3-T18.

**Places.** Deux constructions de front pour ce lot (la session B garde la troisième place du jeton `tests`). V1 en
a deux (L3-T0 ne construit rien) ; V2 en a trois : la troisième attend la notification du jeton (prévu ; l'agent
attend, il ne sonde pas). Ordre de lancement en V2 : L3-T4 (chemin critique),
L3-T6, puis L3-T5.

Vérification des recoupements :
- V1 : L3-T0 (un rapport) / L3-T3 (`core/src/{hittest,erasegeometry}.cpp`, deux tests) / L3-T10 (`liftlayeritem.cpp`,
  un test) — disjoints.
- V2 : L3-T4 (`core/src/{objectedits,lift}.cpp`, deux tests) / L3-T5 (`input/src/{routing,gesture}.cpp`, deux tests)
  / L3-T6 (`tools/src/{userstate,toolregistry,pentool}.cpp`, `pentool.h`, deux tests) — disjoints.
- V3 : L3-T11 (`render/` trait et encre, `PageArea.qml`, deux tests) / L3-T12 (`documentcontroller.cpp`, deux tests) —
  disjoints.
- V4 : L3-T7 (`erasertool.cpp`, un test) / L3-T8 (`selecttool.cpp`, `selectiongeometry.cpp`, un test) — disjoints.
- V5 : L3-T9 (fichiers de L3-T8, après lui) / L3-T14 (`ui/src/{toolhost,tooladapters,penadapters}`, `Main.qml`, trois
  tests) — disjoints.
- V6 : L3-T13 seule (`pageview.cpp`, `tst_pageview.cpp`, `tst_ui_drawing.cpp` repris de L3-T14).
- V7 : L3-T15 (trois QML, `fixtures.cpp`, `selection.json`) / L3-T19 (`bench/` scénarios et joueur, deux tests,
  `docs/banc.md`) — disjoints.
- V8 : L3-T16 (trois QML, deux scénarios) / L3-T18 (un scénario) — disjoints.
- Fichiers repris d'une vague à l'autre, tenus par « dépend de » : `tools/src/selecttool.cpp`,
  `tools/src/selectiongeometry.cpp`, `tests/tst_tools_select.cpp` (L3-T8 → L3-T9) ; `tests/tst_ui_drawing.cpp`
  (L3-T14 → L3-T13) ; `bench/src/fixtures.cpp` (L3-T15 → L3-T16) ; les talons du socle → leur tâche.

**Pourquoi L3-T13 après L3-T14.** La branche « outil de `ToolHost` » de D-S9 du socle (minuterie de versement et
`finishStroke` sur l'outil actif) n'est prouvable qu'une fois `ToolHost::activeTool()` rempli (L3-T14) :
`foldWaitsForTheActiveTool` est donc en L3-T13, après L3-T14 ; `finishStrokeCancelsTheActiveTool` est en L3-T14. Le
regroupement des redessins (L3-T13) précède les mesures qui en dépendent (L3-T19 `soulever`, L3-T15, L3-T18).

**L3-T0 et les autres tâches.** Les tâches de mécanique (L3-T3, L3-T10) ne l'attendent pas. Les tâches de
comportement (L3-T4, L3-T5, L3-T6, L3-T11, donc tout ce qui suit) l'attendent. Ses décisions D1 à D12 **amendent ce
plan** avant la vague V2 (relecture du plan exigée). Une décision qui change une donnée de commande (raccourci,
libellé, ordre de la barre d'objet ou du menu contextuel) est appliquée par un correctif **`L3-C0`**
(`actions/data/commands.json`, `actions/data/layout.json` seulement), joué seul avant V2. Si D11 donne Ctrl+Maj+Z à
Rétablir, le conflit avec `view.page-zoom` (lot 7) est refusé par `noShortcutTwice` (L0-S4) : L3-C0 tranche d'après
F-750 et F-950 à F-953, garde Ctrl+Y si rien ne départage, et le rapporte.

**Règle des `switch` (C4062 en erreur, `/we4062`, L0-S3).** Tout `switch` écrit par une tâche du lot 3 sur
`core::ObjectKind` traite `Equation` (comme `Text` pour l'emprise et la désignation) ; tout `switch` sur
`core::Change::Kind` traite `PageSettingsChanged` (sans effet de cache, sans redessin).

## Dépendances aux autres lots

**Socle L0-S (avant ce lot).** Voir l'en-tête : le lot 3 ne crée plus aucun fichier ni test, il remplit les talons du
socle et respecte ses contrats figés (« Contrats figés par le socle » de son plan). Un en-tête figé ne change que par
amendement du plan ; seuls des membres privés s'ajoutent dans l'en-tête de son module.

**Lot 10 (clos).** Interface de cache inchangée (`core::PageCache`, `core::applyToCache`) ; `DocumentController` en est
le seul appelant. Comportements dont le lot dépend : un objet inséré **hors fin de page** (dépôt d'un objet soulevé,
morceaux de gomme) garde le redessin immédiat (limite de DA11) ; annuler un trait encore en attente ne redessine rien
(L10-T3) ; `cacheReady` couvre cache + couche d'attente. La couche soulevée réutilise `render::createObjectNode`
(L10-T2), au-dessus de `pendingLayer` (ordre figé par L0-S9). Reports du lot 10 tenus ici : H10-1 (L3-T18) ; H10-4
(S36) ; H10-6 (L3-T0, L3-T11) ; H10-8 (e) (L3-T19) ; P10 et P17 : réglages, lot 6 (H3-8). Si L10-T7 (tuiles) a été
jouée, rien ne change ici.

**Lot 7 (session B, `feat/livraison-complete-b`).** Plus aucun fichier chaud commun : le socle a posé les données,
la coquille et le chemin d'entrée. Les surcouches du lot 7 (rideau, spot, zoom de page, panneaux, outils de maths)
sont des **couvertures** (`PageView::addInputCover`, `Route::Cover` du routeur), pas des `tools::Tool` ; les guides
de tracé passent par le filtre d'échantillons (`PageView::setSampleFilter`) ; les outils de maths ouverts vont dans
`tools::UserState::openMathTools`, par `ToolHost::user()` ; « Convertir en texte » (F-202) est une entrée de données du
menu contextuel (`layout.json`, `contextMenu`) ; la reconnaissance remplace des traits par `core::ReplaceObjectsCommand`.
Le lot 3 ne touche à aucun fichier du lot 7 (`features/`, `ui/presentation.cmake`, `tests/presentation.cmake`,
`Presentation.qml`).

**Lots 5 et 6.** Les boutons de la barre d'objet hors lot 3 (menu d'édition, regroupement, avancer, reculer,
dupliquer, points de forme : F-521) arrivent au lot 5 à leur place dans `objectBar` ; l'attribut « verrouillable »
(F-298) est posé par le lot 6 sur les commandes des lots 1 à 5 (amendement § 1) ; les profils (N2-6 amendée) sont
livrés au lot 6 avec les outils déjà là : aucune action au lot 3.

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

1. **Volume.** 13 tâches estimées (amendement § 6) → 17 dans ce plan, plus le socle commun L0-S (9 tâches, partagé
   avec le lot 7) : soulèvement (couche, commandes, contrôleur), routeur unique (DA-A1), forme du trait (H10-6),
   banc des gestes ; les réservations et le joueur de scénarios sont passés au socle.
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
  ≤ k + 1 (plus un si le dernier trait encore en attente est gommé en deux morceaux : son premier morceau n'est pas en
  fin de page, redessin complet). Au-delà de l'attendu : défaut du lot 3 (tâche corrective). Dans l'attendu mais figements jugés gênants en
  D3D11 à la clôture : H10-8 (e) décide (tuiles, cache par groupe de couches), présenté à la porte de fin.
- **S37 — Marqueur et mémoire par outil (lot 3).** `captures.ps1 -Scenario marqueur` : marqueur jaune de 20 par
  défaut ; un trait de marqueur qui se recoupe a une teinte uniforme à son croisement ; deux traits superposés
  foncent ; stylo rouge 8, puis marqueur, puis stylo : rouge 8 rendu ; un second lancement avec le même fichier de
  réglages garde le rouge 8 (`tst_ui_tools`, `settingsSurviveRestart`).

## Couverture des scénarios

| Scénario | Tâches | Joué à la clôture |
|---|---|---|
| S6 — écrire, gommer, annuler, pincer | (L0-S2) L3-T3, T4, T5, T6, T7, T8, T10, T12, T13, T14, T18 | oui : hors écran et à l'écran en D3D11 |
| S30 — tâche d'ouverture | L3-T0 | contrôlé à la relecture : rapport, relevé brut, `NOMS OK` |
| S34 — sélection | (L0-S1, L0-S4) L3-T8, T9, T15 | oui |
| S35 — effacer, supprimer, corbeille | (L0-S4) L3-T4, T12, T16 | oui |
| S36 — banc des gestes | L3-T19 | oui : banc complet D3D11 plein écran |
| S37 — marqueur et mémoire par outil | L3-T6, T11, T14, T16 | oui |
| S13 (lot 10) — non-régression du cache | toutes les tâches de `render/ ui/` | oui : scénarios `pauses`, `cache-differe` rejoués |

## Carte des fichiers

Tous les fichiers ci-dessous **existent** après L0-S10 (talons du socle, marqués `Talon L0-S : rempli par L3-T<n>.`,
ou fichiers des lots précédents). Le lot 3 n'en crée aucun.

| Fichier | Responsabilité | Posé par | Rempli par |
|---|---|---|---|
| `docs/verifications/2026-10-18-lot3-principes.md` | constats du corpus, de l'ancien dépôt et du binaire ; décisions D1 à D12 | — (créé) | L3-T0 |
| `core/src/hittest.cpp`, `core/src/erasegeometry.cpp` | `isAnnotation`, `objectAt`, `objectsTouching`, rectangles ; `cutByDisk`, `cutBySweep`, `strokePointsOnPage` | L0-S6 (en-têtes figés, talons) | L3-T3 |
| `core/src/objectedits.cpp`, `core/src/lift.cpp` | `TransformObjectsCommand`, `RemoveObjectsCommand`, `ClearPageCommand`, `ReplacePageCommand`, `SetTranslucencyCommand`, pivots ; `takeObjects`, `restoreObjects`, `liftChanges`, `changesAfterLift` | L0-S6 | L3-T4 |
| `core/include/actcraft/core/replace.h`, `core/src/replace.cpp` | `ReplaceObjectsCommand` (gomme, reconnaissance) | L0-S3 (rempli) | — (employée par L3-T7) |
| `input/src/routing.cpp` (corps de `FullTouchPolicy::decide`), `input/src/gesture.cpp` | politique du toucher, geste à deux doigts | L0-S6, L0-S7 (mécanisme du routeur rempli) | L3-T5 |
| `tools/src/userstate.cpp`, `tools/src/toolregistry.cpp`, `tools/include/actcraft/tools/pentool.h`, `tools/src/pentool.cpp` | réglages par outil, registre, stylo et marqueur | L0-S6, L0-S7 (`PenTool` est déjà un `Tool`) | L3-T6 |
| `tools/src/erasertool.cpp` | gomme | L0-S6 | L3-T7 |
| `tools/src/selecttool.cpp`, `tools/src/selectiongeometry.cpp` | sélection, cadre, poignées, barre | L0-S6 | L3-T8, L3-T9 |
| `render/src/liftlayeritem.cpp` | couche soulevée (type QML `LiftLayer`) | L0-S6 | L3-T10 |
| `render/src/{strokegeometry,inkitem,strokerenderer,pagepainter}.cpp` | bouts ronds, courbe lissée, marqueur d'un seul tenant | lots 1 à 10 | L3-T11 |
| `ui/src/documentcontroller.cpp` | soulever, déposer, `editAboutToApply`, effacer, rétablir la page | L0-S5 (API, corps vides, observateur unique) | L3-T12 |
| `ui/src/pageview.cpp` | geste à deux doigts, demandes de redessin regroupées par tour | L0-S7 (chemin d'entrée rempli) | L3-T13 |
| `ui/src/toolhost.cpp`, `ui/src/tooladapters.{h,cpp}`, `ui/src/penadapters.{h,cpp}`, `ui/qml/Main.qml` | `ToolHost::activeTool`, adaptateurs, commandes, mémoire par outil, Échap | L0-S6, L0-S9 | L3-T14 |
| `ui/qml/SelectionOverlay.qml`, `ObjectBar.qml`, `TranslucencyPopup.qml`, `bench/src/fixtures.cpp` | cadre, poignées, barre, réglette ; fixtures | L0-S6 (QML à `required property`, branchés par L0-S9) | L3-T15 |
| `ui/qml/TrashCan.qml`, `PageContextMenu.qml`, `ToolCursor.qml` | corbeille, menu contextuel (données `commands.contextMenu`), curseurs | L0-S6, L0-S9 | L3-T16 |
| `bench/src/scenarios.cpp`, `bench/src/runner.cpp` | scénarios de banc `gommer`, `soulever` | lots 1 et 10 | L3-T19 |
| `bench/scenarios/{selection,corbeille,marqueur,ecrire-gommer-annuler}.json` | S34, S35, S37, S6 | L0-S0 (talons) | L3-T15, L3-T16, L3-T18 |
| `tests/tst_{core_hittest,core_erase,core_objectedits,core_lift,input_routing,input_gesture,tools_registry,tools_eraser,tools_select,render_liftlayer,render_strokeshape,ui_lift,ui_tools}.cpp` | un par module | L0-S0 (enregistrés), talons `reserved()` | la tâche du module |

## Contrats partagés entre tâches (noms figés)

Les contrats sont ceux du socle (« Contrats figés par le socle » de `docs/plans/2026-10-18-livraison-complete-socle-3-7-PLAN.md`),
lus dans le worktree de la tâche ; les en-têtes publics du lot 3 (`hittest.h`, `erasegeometry.h`, `objectedits.h`,
`lift.h`, `routing.h`, `gesture.h`, `tool.h`, `toolcontext.h`, `userstate.h`, `toolregistry.h`, `erasertool.h`,
`selecttool.h`, `selectiongeometry.h`, `liftlayeritem.h`, `toolhost.h`, ajouts de `documentcontroller.h` et
`pageview.h`) sont **figés** par L0-S5, L0-S6 et L0-S7 : seuls des membres privés s'ajoutent. Rappel des noms dont les
tâches dépendent :

- `core::ReplaceObjectsCommand(page, std::vector<ObjectReplacement>)` et `(page, removed, added)`, `addedIds()` ;
  `core::ObjectReplacement { removed; added; }` ; `objectedits.h` **sans** `ReplaceStrokesCommand` ni
  `StrokeReplacement`.
- `ui::DocumentController::ChangeOrigin` (`Command`, `Undo`, `Redo`, `Stroke`, `Lift`, `Drop`), `ChangeObserver`,
  `addChangeObserver(observer) → jeton`, `removeChangeObserver(jeton)` (remplacent `setCacheObserver`) ;
  `liftObjects` n'émet **pas** `editAboutToApply`.
- `input::Route` = `Tool, Gesture, Ignore, Cover` ; `PointerRouter::setCoverTest`, `release` ; `Sample::pointer`,
  `Sample::user`.
- `ui::PageView` : `tools`, `cacheRefreshCount`, `gestureActive`, `currentTool()`, `router()`, `zoomAround(viewPoint,
  zoom)`, `sceneToPage`, `addInputCover`, `setSampleFilter`, signal `contextMenuRequested(viewPos)`.
- `ui::ToolHost` (type QML `Tools`) : API de L0-S6, `user()` réel, `activeTool()` talon jusqu'à L3-T14.
- Commandes (19 du lot 3, données de L0-S4) : `tool.select` (Échap), `tool.highlighter` (Ctrl+H), `tool.eraser`
  (Ctrl+E), `edit.undo` (Ctrl+Z), `edit.redo` (Ctrl+Y, D11), `edit.delete` (Suppr), `edit.select-all` (Ctrl+A),
  `clear.annotations`, `clear.objects`, `clear.grid`, `clear.background`, `clear.page`, `page.revert`, `view.trash`,
  `object.move-free`, `object.rotate`, `object.translucency`, `object.enlarge` (Ctrl++), `object.reduce` (Ctrl+-) ;
  `Commands::objectBar`, `Commands::contextMenu`.
- Identifiants d'outil : `select`, `pen`, `highlighter`, `eraser`. `core::ClearKind` : `Annotations = 0`,
  `Objects = 1`, `Grid = 2`, `Background = 3`, `Page = 4`.
- Objets QML lus par les scénarios (L0-S9) : `appTools`, `appDocument`, `pageView`, `fixedPage`, `liftLayer`,
  `pageOverlays`, `selectionOverlay`, `toolCursor`, `pageContextMenu`, `trashCan`.
- `ToolHost::selectionFrame` : `{visible, corners ×4, handles ×8, bar: [{command, icon, x, y}], rubberBand | null,
  handlesVisible}`, **en coordonnées de page**, placé par `pageView.pageToScene`.
- Constantes : `core::kHitToleranceScreen = 6.0` ; `tools::kHandleRadiusScreen = 6.0`, `kBarButtonScreen = 24.0`,
  `kBarGapScreen = 8.0`, `kEnlargeStep = 1.1` ; `input::kPinchDeadZone = 0.03` ; `tools::defaultSettings` (D5).

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
  Attendu : `100% tests passed, 0 tests failed out of N` (N = N_départ + 19, valeur de N_départ dans l'état du chantier), `TESTS OK`, puis la ligne `Passed`
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
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario <nom> -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire ''<objet.propriété=attendu>'' > .tmp/captures.txt 2>&1'; Select-String -Path .tmp/captures.txt -Pattern '^(COMMANDE|TRAIT|TOUCHE|DOIGT|PINCEMENT|POINTEUR|DOUBLECLIC|ATTENTE|PRET|FENETRE|LU|PIXEL|BORD|CAPTURE) |^SCENARIO |CAPTURES (OK|EN ECHEC)' | Select-Object -Last 16"
  ```
  Motifs d'échec : `CAPTURES EN ECHEC`, `SCENARIO EN ECHEC`, `SCENARIO INVALIDE`, toute ligne `ECART` ou `ECHEC`,
  `FENETRE …`. Les lignes `DOIGT`, `PINCEMENT`, `POINTEUR` et `DOUBLECLIC` existent depuis L0-S2.
- **Dépôt public** : `python -I "C:/Users/JeromeDenis/REA - Activinspire/tools/controle_noms_actcraft.py" <wt>` →
  `NOMS OK` avant chaque commit qui ajoute du texte (libellés, commentaires, docs). Libellés à nous (N1-12), aucun
  nom de l'original, aucune icône d'origine.
- **moc** ne reconnaît pas les chaînes brutes `R"(...)"` qui contiennent une apostrophe (piège L2-T10) : dans un
  test, pas d'apostrophe dans une chaîne brute (écrire `’` ou une chaîne ordinaire).
- **Talons** (règle du socle, section « Talons » de son plan) : remplir un talon, c'est remplacer son corps,
  retirer son marqueur `Talon L0-S : rempli par L3-T<n>.` et supprimer `reserved()` ; les fonctions du socle
  présentes dans le même test restent et passent. Un en-tête figé ne change que par amendement du plan (membres
  privés seulement). Aucune tâche n'enregistre ni ne crée de test : N = N_départ + 19 partout.
- **`switch` sous `/we4062`** (C4062 en erreur, L0-S3) : tout `switch` sur `core::ObjectKind` traite `Equation`
  (comme `Text` pour l'emprise et la désignation) ; tout `switch` sur `core::Change::Kind` traite
  `PageSettingsChanged` (sans effet de cache).
- **Contrôle des noms** : `git -C <wt> add <fichiers>` **avant** `NOMS OK` (le script ne lit que les fichiers
  suivis), puis `NOMS OK` relancé **après** le commit ; les deux sorties au rapport.

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
### Tâche L3-T1 — retirée (faite par le socle)

Faite par L0-S0, L0-S1, L0-S4, L0-S5, L0-S6, L0-S7 (amendement 1 du socle). Ne pas exécuter.

---

<a id="tache-l3-t2"></a>
### Tâche L3-T2 — retirée (faite par le socle)

Faite par L0-S9 (coquille QML). Ne pas exécuter.

---

<a id="tache-l3-t3"></a>
### Tâche L3-T3 — `core` : désignation et géométrie de la gomme

**Objectif.** Remplir `hittest.*` et `erasegeometry.*` (en-têtes figés par le socle, L0-S6) : quel objet est sous le pointeur,
quels objets touche un rectangle, comment un disque coupe la ligne centrale d'un trait (algorithme de l'ancien
dépôt, coupe d'une ligne brisée par un disque, réécrit par nous).

**Fichiers.** Modifier (talons) : `core/src/hittest.cpp`, `core/src/erasegeometry.cpp`, `tests/tst_core_hittest.cpp`,
`tests/tst_core_erase.cpp` ; en-têtes `core/include/actcraft/core/hittest.h`, `erasegeometry.h` : membres privés ou
fonctions internes seulement.

**Test et commande bornée.** Commande ciblée `tst_core_hittest|tst_core_erase` ; preuve de tâche N = N_départ + 19 ; fonctions
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
      void equationHitsLikeText()
      {
          // C4062 : l'équation (L0-S3) se désigne par son emprise, comme un texte.
          Page page;
          Equation equation;
          equation.bounds = {100, 100, 80, 40};
          PageObject o{equation};
          o.common.id = 9;
          insertAtTopOfLayer(page, o);
          QCOMPARE(objectAt(page, {120, 120}, 0).value_or(0), ObjectId{9});
          QCOMPARE(objectsTouching(page, normalizedRect({170, 130}, {200, 160})), (std::vector<ObjectId>{9}));
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

**Test et commande bornée.** Commande ciblée `tst_render_liftlayer` ; preuve de tâche N = N_départ + 19, fonctions en `PASS` ;
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

**Socle.** `ReplaceStrokesCommand` n'existe pas : la gomme dépose `core::ReplaceObjectsCommand` du socle (L0-S3,
`core/include/actcraft/core/replace.h`, un `ObjectReplacement` par trait touché, `addedIds()`), déjà prouvée par
`tst_core_replace`. `objectedits.h` (figé par L0-S6) n'a ni `ReplaceStrokesCommand` ni `StrokeReplacement`.

**Objectif.** Remplir `objectedits.*` et `lift.*`. Invariant à tenir (le patron évident est le mauvais) : **le cache
ne reçoit jamais `remove` ni `invalidate` d'un objet qu'il n'a plus**, et reçoit `insert` pour chaque objet soulevé
qui revient. Patron à ne pas reprendre : rejouer au dépôt les `Changes` de la commande tels quels (un
`ObjectChanged` d'un objet soulevé arriverait au cache qui ne l'a plus ; un `ObjectRemoved` de trait gommé y
arriverait deux fois). Les décisions D8, D9 et D10 de L3-T0 s'appliquent (amendement éventuel de cette section).

**Fichiers.** Modifier (talons) : `core/src/objectedits.cpp`, `core/src/lift.cpp`, `tests/tst_core_objectedits.cpp`,
`tests/tst_core_lift.cpp` ; en-têtes `objectedits.h`, `lift.h` : membres privés seulement.

**Test et commande bornée.** Commande ciblée `tst_core_objectedits|tst_core_lift` ; preuve de tâche N = N_départ + 19 ;
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
  #include <actcraft/core/replace.h>

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
          Stroke leftStroke;
          leftStroke.points = {{100, 100, 0.5f}, {180, 100, 0.5f}};
          Stroke rightStroke;
          rightStroke.points = {{220, 100, 0.5f}, {300, 100, 0.5f}};
          PageObject left{leftStroke};
          left.common.layer = Layer::Top;
          PageObject right{rightStroke};
          right.common.layer = Layer::Top;
          ReplaceObjectsCommand erase(0, std::vector<ObjectId>{a}, std::vector<PageObject>{left, right});
          Changes command;
          erase.redo(doc, command);
          const Changes cache = changesAfterLift(0, lifted, command, doc.page(0));
          QCOMPARE(count(cache, Change::Kind::ObjectRemoved, a), 0);
          QCOMPARE(count(cache, Change::Kind::ObjectInserted, erase.addedIds()[0]), 1);
          QCOMPARE(count(cache, Change::Kind::ObjectInserted, erase.addedIds()[1]), 1);
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
  (la gomme n'a pas de commande ici : `ReplaceObjectsCommand` du socle) ;
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
### Tâche L3-T5 — `input` : politique du toucher et geste à deux doigts

**Objectif.** DA-A1 : une seule politique décide, à l'appui, où va chaque contact (outil, geste, rien) et à quel
utilisateur il appartient ; la décision tient jusqu'au relâché. Patron évident à **ne pas** reprendre : des
`if (pointCount() == 2)` répartis dans la vue (ce que fait `PageView::touchEvent` aujourd'hui), qui obligeraient le
lot 11 à réécrire la vue pour chaque mode du toucher. Décision D12 de L3-T0 (seuils) appliquée.

**Socle.** `sample.h` a déjà `pointer` et `user` (L0-S7) ; le mécanisme de `PointerRouter` (décisions retenues,
`Route::Cover` sans effet sur `RouterState`, `setCoverTest`, `release`, `endToolContacts`) est posé et rempli par
L0-S7. L3-T5 remplit **seulement** le corps de `FullTouchPolicy::decide` (geste à deux doigts, paume, stylet qui prend
la main) et `gesture.cpp` ; les fonctions du socle de `tst_input_routing` (`coverRouteHeldForWholeContact`,
`coverLeavesTheStateAlone`, `pressRedecides`) restent vertes, sans modification.

**Fichiers.** Modifier : `input/src/routing.cpp` (corps de `FullTouchPolicy::decide` seulement), `input/src/gesture.cpp`,
`tests/tst_input_routing.cpp` (`reserved()` supprimé, fonctions du socle gardées), `tests/tst_input_gesture.cpp` ;
en-têtes `routing.h`, `gesture.h` (figés) : membres privés seulement.

**Test et commande bornée.** Commande ciblée `tst_input_routing|tst_input_gesture|tst_input` ; preuve de tâche
N = N_départ + 19 ; fonctions en `PASS` ; `tst_input` Passed.

**Interdits.** `input/` ne dépend que de `core/` (règle de modularité) ; aucune dépendance à Qt Quick ; rien dans
`ui/` (L3-T13).

**Terminé quand.** Les tests ci-dessous passent, et les fonctions du socle de `tst_input_routing` avec eux.

**Étapes.**
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
- [ ] **Étape 2 bis : les deux règles de M4.** (b) Un stylet ou une souris qui appuie pendant qu'un doigt va à
  l'outil → `Tool` avec `endToolContacts` ; le doigt n'écrit plus. Le mécanisme de L0-S7 fait passer au geste les
  doigts touchés par `endToolContacts` : si, sur un appui de stylet, il ne les fait pas passer à `Ignore`,
  `stylusTakesOverFromFinger` le montre ; **s'arrêter et le rapporter** (amendement du socle, pas de changement
  d'un en-tête figé ici). (a) Deux doigts posés dans le **même** événement : les champs figés de `Sample`
  (`pointer`, `user`) ne disent pas l'événement d'origine ; `simultaneousPinchDrawsNothing` (et son
  `routeEvent`) **n'est pas écrit** dans ce lot (H3-16) : un premier doigt suivi d'un second dans le même
  événement peut verser un point ; S6 ne compte pas les objets après le pincement.
- [ ] **Étape 3 : implémentation.** `FullTouchPolicy::decide` : `Stylus`, `Mouse`, `Other` → `Tool` ; `Touch` :
  `stylusDown` → `Ignore` ; `toolTouches + gestureTouches == 0` → `Tool` ; `toolTouches == 1 && gestureTouches == 0`
  → `Gesture` + `endToolContacts` ; `gestureTouches == 1` (un doigt resté du geste) → `Gesture` ; sinon `Ignore`.
  `PointerRouter::route` : appui → décision de la politique, retenue (clé `pointer`, 0 pour stylet et souris), compteurs
  de `m_state` mis à jour ; `endToolContacts` → décisions retenues des doigts `Tool` passées à `Gesture`, compteurs
  recalculés ; relâché → décision rendue puis retirée ; contact inconnu → `Ignore`. `TwoFingerGesture` : milieu,
  distance ; `scale = d / d0`, ramené à 1 si `|scale − 1| < kPinchDeadZone`.
- [ ] **Étape 4 : vert, preuves, commit.**
  ```powershell
  git -C <wt> add input/src/routing.cpp input/src/gesture.cpp tests/tst_input_routing.cpp tests/tst_input_gesture.cpp
  git -C <wt> commit -m "input : routeur unique des contacts, politique remplaçable, geste à deux doigts (L3-T5)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t6"></a>
### Tâche L3-T6 — `tools` : interface `Tool`, contexte, registre, état d'utilisateur, stylo et marqueur

**Objectif.** Remplir `userstate.*`, `toolregistry.*` (en-têtes figés) ; faire du stylo un `tools::Tool` qui sert
aussi de marqueur (`core::StrokeKind::Highlighter`, translucidité, F-225) ; nouveaux traits à bouts ronds
(`core::PenCap::Round`, D2) ; réglages par défaut (D5). Patron à ne pas reprendre : une classe `HighlighterTool`
copiée du stylo (deux chemins d'encre à tenir en parité).

**Socle.** `tool.h` existe (figé) ; `PenTool` est déjà un `tools::Tool` (`id`, `isBusy`, `cancel`, L0-S7) ;
`UserState::openMathTools` et `UserState::id()` sont réels (L0-S6) : L3-T6 remplit le reste de `userstate.cpp`, sans
toucher `openMathToolsKept` (fonction du socle dans `tst_tools_registry`).

**Fichiers.** Modifier : `tools/include/actcraft/tools/pentool.h`, `tools/src/pentool.cpp`, `tests/tst_pentool.cpp` ;
talons `tools/src/userstate.cpp`, `tools/src/toolregistry.cpp`, `tests/tst_tools_registry.cpp` ; en-têtes figés
`tool.h`, `toolcontext.h`, `userstate.h`, `toolregistry.h` : membres privés seulement.

**Test et commande bornée.** Commande ciblée `tst_pentool|tst_tools_registry` ; preuve de tâche N = N_départ + 19 ;
fonctions en `PASS` ; les cas existants de `tst_pentool` passent sans modification de leurs assertions.

**Interdits.** `registerBuiltinTools` crée la gomme et la sélection par leurs constructeurs (talons du socle, L0-S6) sans
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

  class PenTool final : public Tool // déjà un Tool (L0-S7) : on ajoute le genre et la translucidité
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
  git -C <wt> add tools/include/actcraft/tools/pentool.h tools/src/pentool.cpp tools/src/userstate.cpp tools/src/toolregistry.cpp tests/tst_pentool.cpp tests/tst_tools_registry.cpp
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
tâche N = N_départ + 19 ; fonctions en `PASS` ; banc sans écran `content_check.ok = True` ; à l'écran D3D11 (jeton `ecran`,
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

**Objectif.** Remplir l'API posée par le socle (L0-S5 : corps vides, observateur unique `addChangeObserver`). Le contrôleur reste la
**seule** voie vers le cache : au soulèvement,
`core::takeObjects` sur la page affichée puis `core::liftChanges` → `applyToCache` ; au dépôt,
`core::restoreObjects`, exécution de la commande (pile), puis `core::changesAfterLift` → `applyToCache` (et non les
changements bruts de la commande). Avant toute édition qui n'est pas un trait (annuler, rétablir, effacer,
rétablir la page, supprimer, enregistrer, ouvrir) — **jamais au soulèvement** (B1) —, `editAboutToApply` est émis : la vue y termine le trait en cours (L2-T15 : sinon le
trait irait à la page bornée et effacerait la branche à rétablir). Page de référence de Rétablir la page : la page
telle qu'ouverte ou enregistrée en dernier ; jamais enregistré : page vierge de même taille (F-131, D9).

**Fichiers.** Modifier : `ui/src/documentcontroller.cpp`, `tests/tst_ui_document.cpp` (fonctions du socle gardées,
dont `liftDoesNotAnnounceAnEdit`) ; talon `tests/tst_ui_lift.cpp` ; en-tête figé `documentcontroller.h` : membres
privés seulement.

**Test et commande bornée.** Commande ciblée `tst_ui_lift|tst_ui_document` ; preuve de tâche N = N_départ + 19 ; fonctions
en `PASS` ; mutation : au dépôt, appliquer les changements bruts de la commande → `dropInsertsLiftedObjects` rouge,
puis remise.

**Interdits.** Aucune signature publique figée (L0-S5) changée. Aucun appel direct à `FixedPageItem` hors de
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
  // Observateur unique du socle (L0-S5) : reçoit exactement les changements passés au cache, et leur origine.
  struct CacheSpy
  {
      std::vector<std::pair<char, core::ObjectId>> calls; // 'i' inséré, 'r' retiré, 'v' changé
      std::vector<ui::DocumentController::ChangeOrigin> origins;
      void attach(ui::DocumentController& controller)
      {
          controller.addChangeObserver([this](const core::Changes& changes, ui::DocumentController::ChangeOrigin origin) {
              origins.push_back(origin);
              for (const core::Change& c : changes) {
                  switch (c.kind) {
                  case core::Change::Kind::ObjectInserted: calls.emplace_back('i', c.object); break;
                  case core::Change::Kind::ObjectRemoved: calls.emplace_back('r', c.object); break;
                  case core::Change::Kind::ObjectChanged:
                  case core::Change::Kind::PageChanged: calls.emplace_back('v', c.object); break;
                  case core::Change::Kind::PageInserted:
                  case core::Change::Kind::PageRemoved:
                  case core::Change::Kind::PageSettingsChanged: break; // sans effet de cache (C4062)
                  }
              }
          });
      }
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
          spy.attach(controller);
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
          QCOMPARE(spy.origins.back(), ui::DocumentController::ChangeOrigin::Lift);
      }
      void dropInsertsLiftedObjects()
      {
          ui::DocumentController controller;
          CacheSpy spy;
          spy.attach(controller);
          controller.commitStroke(line(100));
          const core::ObjectId id = controller.document().page(0).objects.back().common.id;
          controller.liftObjects({id});
          spy.calls.clear();
          controller.dropLifted(std::make_unique<core::TransformObjectsCommand>(0, std::vector<core::ObjectId>{id},
                                                                                 core::translation(40, 0)));
          QCOMPARE(spy.calls, (std::vector<std::pair<char, core::ObjectId>>{{'i', id}}));
          QCOMPARE(spy.origins.back(), ui::DocumentController::ChangeOrigin::Drop);
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
  contenu affiché ; vide pour un document neuf). Les appels de cache passent par le `settle(changes, origin)` du socle
  (page fixe puis observateurs), avec l'origine `Lift` au soulèvement et `Drop` au dépôt. `liftObjects` :
  **aucune annonce** (B1 : un seul utilisateur, aucun trait d'un autre outil n'est en cours ; annoncer ferait annuler
  l'outil qui soulève par L3-T14), `takeObjects`, `liftChanges`, `documentChanged`, `liftingChanged`. **Dépôt forcé
  (M2)** : `nextPage`, `pageBack`, `replaceDocument` émettent d'abord `pageAboutToChange` (déjà émis aujourd'hui),
  `saveToUrl` et `openUrl` émettent d'abord `editAboutToApply` ; l'outil y abandonne son geste (`ToolHost`, L3-T14 :
  `drop(nullptr)` et couche soulevée vidée) ; ensuite seulement, s'il reste des objets soulevés, le contrôleur les
  dépose sans étape. `dropLifted` : `restoreObjects`, `run(command)` si non nul, `changesAfterLift`, signaux.
  `undo`/`redo` : `editAboutToApply`, dépôt sans étape si besoin, puis la pile. `revertPage` : `ReplacePageCommand`
  avec la page de référence (ou `core::Page` vierge de même taille). `canRevertPage` : la page affichée diffère de
  sa référence (empreinte).
- [ ] **Étape 3 : vert, mutation, preuves, commit.**
  ```powershell
  git -C <wt> add ui/src/documentcontroller.cpp tests/tst_ui_document.cpp tests/tst_ui_lift.cpp
  git -C <wt> commit -m "ui : soulever et déposer par l'interface de cache, édition annoncée avant le trait, effacer, rétablir la page (L3-T12)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t17"></a>
### Tâche L3-T17 — retirée (faite par le socle)

Faite par L0-S2 (étapes `touch`, `pinch`, `pointer`, `doubleClick`, tolérance d'`expect`). Ne pas exécuter.

---

<a id="tache-l3-t7"></a>
### Tâche L3-T7 — `tools` : gomme

**Socle.** La commande déposée au relâché est `core::ReplaceObjectsCommand` (L0-S3) : un `ObjectReplacement` par
trait touché, `removed = {original}`, `added` = ses morceaux (objets traits de la couche du dessus, commun de
l'original, transformation identité).

**Objectif.** F-230 : au premier contact du disque avec une annotation, la gomme la **soulève** (une fois par trait
et par geste) et la montre recoupée dans la couche soulevée ; elle ne touche jamais une forme, une image, un texte, ni
une annotation déplacée hors de la couche du dessus ; au relâché, **une** `ReplaceObjectsCommand` déposée (un pas
d'annulation, P11) ; rien n'est exécuté pendant le geste. Rayon = largeur de la gomme / 2 (D4).

**Fichiers.** Modifier (talons) : `tools/src/erasertool.cpp`, `tests/tst_tools_eraser.cpp` ; en-tête
`tools/include/actcraft/tools/erasertool.h` : membres privés seulement.

**Test et commande bornée.** Commande ciblée `tst_tools_eraser` ; preuve de tâche N = N_départ + 19 ; fonctions en `PASS`.

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
  - `oneCommandAtRelease` : au relâché, un seul `drop` avec une `ReplaceObjectsCommand` ; page : 4 morceaux.
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
  `ReplaceObjectsCommand` (un `ObjectReplacement` par trait soulevé et coupé ; un trait soulevé mais non coupé
  n'y figure pas : il est remis tel quel par le dépôt) ; rien de soulevé : rien. `cancel` : `drop(nullptr)`, `LiftSurface::clear`.
- [ ] **Étape 3 : vert, preuves, commit.**
  ```powershell
  git -C <wt> add tools/src/erasertool.cpp tests/tst_tools_eraser.cpp
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

**Test et commande bornée.** Commande ciblée `tst_tools_select` ; preuve de tâche N = N_départ + 19 ; fonctions en `PASS`.

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
  git -C <wt> add tools/src/selecttool.cpp tools/src/selectiongeometry.cpp tests/tst_tools_select.cpp
  git -C <wt> commit -m "tools : sélection — désigner, rectangle qui touche, déplacer par soulèvement, corbeille (L3-T8)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t13"></a>
### Tâche L3-T13 — `PageView` : gestes à deux doigts, demandes de redessin regroupées, outil actif de `ToolHost` occupé

**Objectif.** Le chemin d'entrée est posé et rempli par le socle (L0-S7) : événement → `Sample` → `PointerRouter`
(couvertures) → filtre d'échantillons → `Pipeline` (numérotation `seq`, doublons, sonde) → outil actif
(`currentTool()` : `ToolHost::activeTool(user)`, sinon le stylo interne) ; garde du versement différé sur
`currentTool()->isBusy()`, `finishStroke()` par `cancel()`, `contextMenuRequested`, `zoomAround`,
`cacheRefreshCount`, `router()`. Cette tâche ajoute, dans `pageview.cpp` seulement :

1. **Le geste à deux doigts.** `Route::Gesture` (décidé par `FullTouchPolicy`, L3-T5) → `input::TwoFingerGesture` :
   `begin` quand deux contacts de geste sont posés, `update` à chaque mouvement de l'un d'eux, zoom = zoom à l'appui
   × `scale` appliqué par **`zoomAround`** du socle (ancre : le point de vue `center`, le point de page qui était sous
   `anchor` y est ramené ; zoom borné comme la molette, quitte « Ajuster ») ; fin quand un des deux se relève.
   `endToolContacts` → le trait du doigt en cours est terminé (versé) avant le geste. `gestureActive` suit le geste.
   La vue ne garde aucune règle propre au nombre de doigts : tout vient de la décision du routeur.
2. **Les demandes de redessin d'un même tour regroupées (M3, règle retenue).** Toutes les demandes arrivées dans un
   même tour de boucle (les `contentChanged` répétés d'un soulèvement de plusieurs objets ou d'un dépôt) font **un
   seul** redessin : `requestCacheRefresh` pose un drapeau et poste une seule demande en file
   (`QMetaObject::invokeMethod`, `Qt::QueuedConnection`) qui appelle `scheduleUpdate` une fois ; le relancement de fin
   de redessin (`m_refreshAgain`) n'est posé que par une demande arrivée **après** le départ du redessin en cours. La
   décision 10 (« un trait figé n'attend pas ») tient : la demande part au tour suivant, avant l'image suivante.
3. **La preuve de la branche « outil de `ToolHost` » de D-S9** (socle) : `ToolHost::activeTool()` est rempli par
   L3-T14 ; c'est ici seulement que la minuterie de versement, la sonde de latence et le changement de page sont
   prouvés avec l'outil de `ToolHost` (au socle, seul le stylo interne l'était).

**Fichiers.** Modifier : `ui/src/pageview.cpp`, `tests/tst_pageview.cpp`, `tests/tst_ui_drawing.cpp` ; en-tête
`ui/include/actcraft/ui/pageview.h` (figé) : membres privés seulement, ce qui n'en fait pas un fichier de la tâche
au sens des vagues (aucune autre tâche ne le touche).

**Test et commande bornée.** Commande ciblée `tst_pageview|tst_ui_drawing|tst_ui_inputpath` ; preuve de tâche
N = N_départ + 19 ; fonctions nommées en `PASS` (dont tous les cas existants de `tst_pageview`, inchangés) ;
`tst_ui_inputpath` toujours vert (dont `cacheRefreshCountCounts`, `foldWaitsForTheBusyTool` du socle) ; banc sans
écran `content_check.ok = True` ; `premiers-pas` et `cache-differe` hors écran `CAPTURES OK`.

**Interdits.** Ne pas refaire ni contourner le chemin d'entrée du socle ; aucun `if` sur le nombre de points tactiles
hors du routeur ; pas d'activation conjointe de `AA_CompressHighFrequencyEvents` et `AA_CompressTabletEvents` ; ne pas
toucher `toolhost.*`, `input/` ; tout `switch` sur `Change::Kind` traite `PageSettingsChanged` (C4062 en erreur).

**Terminé quand.** Tests ci-dessous en `PASS` ; S2, `cache-differe` et le banc inchangés.

**Étapes.**
- [ ] **Étape 1 : tests rouges** (`tests/tst_pageview.cpp`, fenêtre hors écran, `QTest::touchEvent` avec un
  `QPointingDevice` tactile de `QTest::createTouchDevice()`, une image laissée entre deux envois — piège L1-T8) :
  - `pinchZoomsAroundItsCenter` : zoom 1 ; deux doigts à (400, 300) et (600, 300) puis (300, 300) et (700, 300) :
    `zoom() == 2` (± 0,01) ; le point de page qui était sous (500, 300) y est encore (`pageToScene`, ± 1 px).
  - `twoFingerPanScrolls` : deux doigts déplacés ensemble de (0, +50) : `pan` change de 50 px de vue, zoom inchangé.
  - `secondFingerEndsFingerStroke` : un doigt trace (3 mouvements), un deuxième se pose : un trait versé
    (`fixedPage->objectCount()` + 1), puis aucun point de plus à ce trait quand les deux doigts bougent ;
    `gestureActive` vrai puis faux au relâché des deux.
  - `palmIgnoredDuringStylus` : stylet appuyé, un doigt se pose et bouge : aucun trait de doigt, zoom inchangé.
  - `requestsOfOneTurnMakeOneRedraw` : trois `requestCacheRefresh` dans le même tour, puis attente de `cacheReady`
    → `cacheRefreshCount()` + 1 ; une demande arrivée pendant un redessin commencé → + 1 de plus (relancement gardé).
  - `foldWaitsForTheActiveTool` : `PageView::setTools` sur un `ToolHost` branché (contrôleur, vue, couche, comme
    `tst_ui_tools`), outil stylo de `ToolHost` : appui du stylet puis **immobile 800 ms** (au-delà de
    `kCacheFoldIdleMs`) : aucun versement (`fixedPage.pendingCount` et `cacheRefreshCount` inchangés) tant que
    l'outil est occupé ; relâché, puis 500 ms : versement.
  - `latencyProbeSeesToolSamples` : sonde de latence posée (`setInkProbe`), outil de `ToolHost`, dix échantillons de
    stylet : la sonde en a vu dix, numéros `seq` croissants.
  - `pageChangeEndsToolStroke` : un trait de l'outil de `ToolHost` en cours, `DocumentController::nextPage()` (signal
    `pageAboutToChange`, branché à `finishStroke`) → le trait est versé à la page qu'il quitte, l'outil n'est plus
    occupé.
  À `tests/tst_ui_drawing.cpp` (fenêtre qui charge `Main.qml`) : `dragOfTwoObjectsCostsTwoRedraws` (M3) — deux traits
  versés, outil Sélectionner, rectangle de sélection sur les deux, glisser de 80 px : `pageView.cacheRefreshCount`
  + 2 exactement sur tout le geste (un au soulèvement des deux, un au dépôt).
  Commande ciblée → rouge.
- [ ] **Étape 2 : implémentation** d'après l'objectif (regroupement d'abord, puis geste).
- [ ] **Étape 3 : vert, preuves, commit.**
  ```powershell
  git -C <wt> add ui/src/pageview.cpp ui/include/actcraft/ui/pageview.h tests/tst_pageview.cpp tests/tst_ui_drawing.cpp
  git -C <wt> commit -m "ui : pincer et défiler à deux doigts, demandes de redessin regroupées par tour, outil actif prouvé (L3-T13)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```
  (`pageview.h` seulement s'il a changé.)

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

**Fichiers.** Modifier : `tools/src/selecttool.cpp`, `tools/src/selectiongeometry.cpp`, `tests/tst_tools_select.cpp` ;
en-têtes figés `selecttool.h`, `selectiongeometry.h` : membres privés seulement.

**Test et commande bornée.** Commande ciblée `tst_tools_select` ; preuve de tâche N = N_départ + 19 ; fonctions en `PASS`
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
  git -C <wt> add tools/src/selecttool.cpp tools/src/selectiongeometry.cpp tests/tst_tools_select.cpp
  git -C <wt> commit -m "tools : sélection — poignées, redimensionner, rotation, barre d'objet (L3-T9)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t14"></a>
### Tâche L3-T14 — `ui::ToolHost` : outils branchés

**Objectif.** Remplir `ToolHost` (API figée par L0-S6 ; `user()` déjà réel) et `ToolAdapters` : un
`tools::ToolRegistry` (`registerBuiltinTools`), l'utilisateur de `user()`, un `ToolContext` dont chaque interface est
un adaptateur — `DocumentEditor` sur `DocumentController` (`liftObjects`, `dropLifted`, `execute`, page affichée),
`LiftSurface` sur `render::LiftLayerItem`, `SelectionSurface` vers `selectionFrame` / `handlesVisible` (signal
`selectionChanged`), `ViewInfo` sur `PageView` (zoom, `trashArea` en coordonnées de scène comparée au point de page
passé par `pageToScene`, `Commands::objectBar`), `CommandSink` sur `Commands::trigger`, `InkSurface` et
`DocumentSink` sur l'encre et le contrôleur (`ControllerSink::commitAnnotation` →
`DocumentController::commitAnnotation`). **`activeTool(user)`** rend l'outil courant de l'utilisateur (une instance
par outil, créée à la première demande). `activate(id)` : `deactivate` de l'outil courant, création ou reprise de
l'outil, commande `tool.<id>` cochée et les autres décochées, couleur et épaisseur de l'outil rendues à `PageView`
(`inkColor`, `inkWidth`, alpha du marqueur). Mémoire par outil (F-136) : `setColor` / `setWidth` écrivent dans
`UserState` **et** dans `QSettings` (`outils/<id>/couleur`, `outils/<id>/epaisseur` ; fichier `settingsFile` s'il
est posé), relus à la création. `DocumentController::editAboutToApply` et `pageAboutToChange` → `cancel()` de l'outil
courant (la sélection et la gomme déposent alors sans étape et vident la couche soulevée, M2) ; le soulèvement
lui-même n'annonce rien (B1, `liftDoesNotAnnounceAnEdit` du socle) ; `pagesChanged` → `SelectTool::refresh()`.
**Échap (M9)** : raccourci d'application de `tool.select`, jamais livré à `Tool::onKey` ; le gestionnaire de
`Main.qml` devient `appTools.cancelGesture(); appTools.activate("select")` (`cancelGesture` abandonne le geste de
l'outil occupé, sans effet sinon ; activer l'outil déjà courant ne fait rien) : Échap pendant un glissement =
abandon ; au repos, outil Sélectionner. `Main.qml` : aussi `tool.pen` → `appTools.activate("pen")`, `pen.color` →
`appTools.setColor(color)`, `pen.width` → `appTools.setWidth(width)` (les autres gestionnaires du lot 3 sont posés par
L0-S9) ; `PageView` n'a plus de couleur propre quand `tools` est posé (S2 lit toujours `pageView.inkColor`, que
`ToolHost` tient à jour).

**Fichiers.** Modifier : `ui/src/toolhost.cpp`, `ui/src/tooladapters.h`, `ui/src/tooladapters.cpp`,
`ui/src/penadapters.h`, `ui/src/penadapters.cpp`, `ui/qml/Main.qml`, `tests/tst_ui_tools.cpp` (talon : `reserved()`
supprimé, `oneUserForNow` et `liftLayerTypeIsRegistered` du socle gardés), `tests/tst_ui_drawing.cpp`,
`tests/tst_modules.cpp` ; en-tête `ui/include/actcraft/ui/toolhost.h` (figé) : membres privés seulement.

**Test et commande bornée.** Commande ciblée `tst_ui_tools|tst_ui_drawing|tst_modules|tst_ui_shell` ; preuve de
tâche N = N_départ + 19 ; fonctions nommées en `PASS` ; `premiers-pas` hors écran `CAPTURES OK` ; banc sans écran
`content_check.ok = True`.

**Interdits.** Aucun outil ne reçoit un pointeur vers `FixedPageItem`, `PendingLayerItem`, `LiftLayerItem` ou
`PageView` (seulement les adaptateurs) ; aucune signature publique figée changée ; aucun réglage écrit dans le profil
réel pendant les tests (`settingsFile` sous le dossier temporaire du test) ; tout `switch` sur `ObjectKind` traite
`Equation`, sur `Change::Kind` traite `PageSettingsChanged`.

**Terminé quand.** Tests ci-dessous en `PASS` ; S2 inchangé ; banc joint.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** `tests/tst_ui_tools.cpp` (fenêtre hors écran qui charge `Main.qml` comme
  `tst_ui_drawing`, fichier de réglages dans un `QTemporaryDir`, observateur posé par
  `appDocument.addChangeObserver` qui note `i`/`r`/`v` par changement et l'origine) :
  - `activatingAToolChecksItsCommand` : `trigger("tool.eraser")` → `currentTool == "eraser"`, `tool.eraser` coché,
    `tool.pen` décoché ; `activeTool()->id() == "eraser"`.
  - `colorGoesToActiveTool` : stylo, `trigger("pen.color", "#E53935")` ; marqueur : couleur jaune par défaut,
    `pageView.inkColor` jaune ; retour au stylo : rouge.
  - `settingsSurviveRestart` : stylo rouge 8, fenêtre détruite ; nouvelle fenêtre avec le même `settingsFile` :
    stylo rouge 8 (F-136).
  - `eraseGoesThroughTheCacheInterface` : deux traits au stylet injecté, gomme, trait de gomme qui coupe le premier :
    l'observateur voit `r(id1)` (origine `Lift`) pendant le geste, puis au relâché `i(morceau1)`, `i(morceau2)`
    (origine `Drop`) ; aucun `v` ; `shownObjectCount == 3` ; `liftLayer.objectCount == 0` après le relâché.
  - `dragGoesThroughTheCacheInterface` : sélection, glisser un trait de 80 px en 8 mouvements : `r(id)` (`Lift`) puis
    `i(id)` (`Drop`), rien entre les deux.
  - `undoAfterEraseRestores` : après la gomme, `trigger("edit.undo")` → 2 objets, empreinte d'avant la gomme.
  - `liftDoesNotCancelTheLiftingTool` (B1) : sélection, glisser un trait : pendant le geste `liftLayer.objectCount ==
    1` et l'outil toujours occupé ; au relâché l'objet est déplacé (et non perdu) ; même essai avec la gomme : les
    morceaux sont dans la page au relâché.
  - `pageChangeWhileDraggingClearsTheLiftLayer` (M2) : pendant un glissement, `trigger("page.next")` →
    `liftLayer.objectCount == 0` aussitôt, l'objet est à sa place d'origine sur la page 1, aucune étape ajoutée ; le
    relâché qui suit ne dépose rien.
  - `escapeCancelsTheGesture` (M9) : pendant un glissement, touche Échap injectée → objet à sa place, couche vide,
    outil courant toujours `select` ; Échap au repos avec le stylo → outil `select`.
  - `finishStrokeCancelsTheActiveTool` (D-S9 du socle) : outil Sélectionner en plein glissement (outil occupé, que
    `finishStroke` ne termine pas comme un trait) ; `pageView.finishStroke()` → l'outil reçoit `cancel()` : plus
    occupé, couche soulevée vide, objet à sa place.
  `tests/tst_modules.cpp` : `toolsSeeNoRenderItem` — aucun fichier de `tools/include` ni `tools/src` ne contient
  `#include <actcraft/render/` ni `#include <actcraft/ui/`. À `tests/tst_ui_drawing.cpp` : les cas existants passent
  avec `tools` posé (`Main.qml`) ; ajouter `fingerStrokeStillDraws` (un doigt injecté trace avec l'outil stylo, S3
  tenu). Commande ciblée → rouge.
- [ ] **Étape 2 : implémentation** d'après l'objectif.
- [ ] **Étape 3 : vert, preuves, commit.**
  ```powershell
  git -C <wt> add ui/src/toolhost.cpp ui/include/actcraft/ui/toolhost.h ui/src/tooladapters.h ui/src/tooladapters.cpp ui/src/penadapters.h ui/src/penadapters.cpp ui/qml/Main.qml tests/tst_ui_tools.cpp tests/tst_ui_drawing.cpp tests/tst_modules.cpp
  git -C <wt> commit -m "ui : outils branchés par leurs interfaces, mémoire par outil gardée au redémarrage, Échap (L3-T14)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```
  (`toolhost.h` seulement s'il a changé.)

---

<a id="tache-l3-t15"></a>
### Tâche L3-T15 — QML : cadre de sélection, poignées, barre d'objet, réglette ; scénario `selection`

**Objectif.** Dessiner `appTools.selectionFrame` (S34) : cadre fin aux quatre coins (polygone, il pivote avec
l'objet), huit poignées rondes grises, la poignée bas droite barrée d'un trait (F-520), barre de boutons carrés aux
icônes des commandes (F-521), rectangle élastique gris ; tout placé par `pageView.pageToScene`, réévalué sur
`zoomChanged` et `panChanged` ; masqué quand `handlesVisible` est faux (poignées et barre) ; la surcouche **ne prend
aucune entrée** (les clics vont à la vue, donc au routeur et à l'outil). `TranslucencyPopup.qml` : réglette 0 à 255
ouverte par `object.translucency` sans argument (le gestionnaire posé par L0-S9 appelle
`selectionOverlay.openTranslucency()`, que cette tâche remplit ; s'il ne l'appelle pas, l'écrire dans le rapport,
ne pas toucher `Main.qml`), chaque mouvement déclenchant
`object.translucency <valeur>` (une étape par fusion, L3-T4). Aspect à nous (thème `Theme`), lisible à 3:1 sur la
page et sur le fond.

**Fichiers.** Modifier (talons) : `ui/qml/SelectionOverlay.qml`, `ui/qml/ObjectBar.qml`,
`ui/qml/TranslucencyPopup.qml`, `bench/scenarios/selection.json` ; modifier `bench/src/fixtures.cpp` (M8) : deux
fixtures nommées, écrites par nous comme les existantes — `rectangle-et-traits` (page 1024 × 768 : un rectangle plein gris
#808080 (100, 100, 100, 100) sur la couche intermédiaire, deux traits sur la couche du dessus, y 300 et 400, x 300 à 500) et
`objet-verrouille` (un rectangle plein #808080 libre (100, 100, 100, 100) et un rectangle plein #404040 verrouillé
(400, 100, 100, 100), `common.locked = true`). L3-T16 utilise la première.

**Test et commande bornée.** Preuve de tâche N = N_départ + 19, `tst_captures_selection` Passed ; preuve visuelle hors écran :
`-Scenario selection -Lire "appTools.selectionCount=2"` → `LU appTools.selectionCount=2 OK`, puis les captures
décrites (cadre, poignées, barre, déplacement) ; banc sans écran.

**Piège (amendement 12 du socle).** `SelectionOverlay` est une sœur de la vue posée au-dessus d'elle en
`anchors.fill` : un gestionnaire d'appui (`TapHandler`, `DragHandler`, `PointHandler`, `MouseArea`) sur sa
**racine** ferait que `coveredAt` (sœurs de la vue) la voie partout, et le stylet ne tracerait plus nulle part. Les
poignées et la barre sont désignées par l'outil (`hitSelection`, L3-T9) : aucun gestionnaire n'est nécessaire ; s'il
en faut un (info-bulle d'un bouton de barre), il va sur ce bouton seulement, à sa taille, jamais sur la racine.

**Interdits.** Aucun gestionnaire d'appui sur la racine d'une surcouche ; éléments décoratifs `enabled: false` ;
aucune icône d'origine (icônes de L0-S1) ; ne pas toucher `Main.qml`, `PageArea.qml` (les `required property` sont
branchées par L0-S9).

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
      {"stroke": [[700, 650], [800, 650]], "pressure": 0.5},
      {"expect": "appTools.selectionCount", "value": 0},
      {"command": "tool.pen"},
      {"stroke": [[700, 680], [800, 680]], "pressure": 0.5},
      {"waitReady": true},
      {"expect": "appDocument.shownObjectCount", "value": 3},
      {"command": "edit.undo"},
      {"command": "tool.select"},
      {"stroke": [[400, 300], [400, 300]], "pressure": 0.5},
      {"expect": "appTools.selectionCount", "value": 1},
      {"command": "tool.pen"},
      {"stroke": [[700, 680], [800, 680]], "pressure": 0.5},
      {"waitReady": true},
      {"expect": "appDocument.shownObjectCount", "value": 3},
      {"command": "edit.undo"},
      {"command": "tool.select"},
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
  Piège (amendement 12) : juste après `un-trait-selectionne`, avec le cadre affiché, un trait au stylet **hors du
  cadre** (outil Sélectionner : rectangle élastique, la sélection tombe à 0 ; outil Stylo, sélection gardée : le trait
  est versé, 3 objets) prouve que la surcouche ne couvre pas la page ; chaque essai est annulé, et la sélection
  reprise par un clic, pour que la suite parte du même état. Compléments (M8) : `agrandi-deborde` — après `object.enlarge` (× 1,1 autour du centre des deux traits déplacés,
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
  (polygone de `corners`) ; **aucun gestionnaire d'appui sur la racine** (voir « Piège ») ; `ObjectBar.qml` : `Repeater` sur `bar` (icône `appCommands.iconUrl(command)`) ;
  `TranslucencyPopup.qml` : `Popup` avec `Slider` (le `PageView.inputBlocked` suit déjà les fenêtres surgissantes).
- [ ] **Étape 3 : preuves, commit.**
  ```powershell
  git -C <wt> add ui/qml/SelectionOverlay.qml ui/qml/ObjectBar.qml ui/qml/TranslucencyPopup.qml bench/scenarios/selection.json
  git -C <wt> commit -m "ui : cadre de sélection, poignées, barre d'objet, réglette de translucidité ; scénario S34 (L3-T15)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l3-t16"></a>
### Tâche L3-T16 — QML : corbeille, menu contextuel, curseurs ; scénarios `corbeille` et `marqueur`

**Socle.** Les trois QML sont des talons à `required property`, déjà branchés par la coquille (L0-S9) ; `TrashCan`
est déjà instancié dans `ToolPalette.qml` ; `PageContextMenu.qml` est ouvert par `PageView::contextMenuRequested` et
lit les données `commands.contextMenu` (L0-S4 : `tool.select`, `tool.pen`, séparateur, `recognition.convert-to-text`,
filtrées par plateforme ; « Convertir en texte » y est déjà) ; les icônes sont dessinées (L0-S1).

**Objectif.** `TrashCan.qml` (F-050) : icône de corbeille sous la palette, flèche rouge quand `appTools.trashHover`,
clic → `edit.delete` ; sa zone publiée dans `appTools.trashArea` (coordonnées de scène, recalculée à chaque
changement de géométrie) ; visible selon `view.trash`. `PageContextMenu.qml` (F-202, partie) : montre les entrées de
`commands.contextMenu`, sauf celle de l'outil courant (avec Sélectionner, « Stylo » en premier ; avec le stylo ou le
marqueur, « Sélectionner ») ; libellés lus des commandes. `ToolCursor.qml` (F-201) : `cursorShape` selon
l'outil (flèche pour la sélection, croix pour le stylo ; pour le marqueur et la gomme, `Qt.BlankCursor` et un
**cercle** de la largeur de l'outil, au zoom, sous le pointeur — F-201 : « marqueur avec cercle, gomme avec
cercle ») ; l'indicateur de largeur existant (F-138) reste pour le stylo
et le marqueur. Scénarios S35 et S37.

**Fichiers.** Modifier (talons) : `ui/qml/TrashCan.qml`, `ui/qml/PageContextMenu.qml`, `ui/qml/ToolCursor.qml`,
`bench/scenarios/corbeille.json`, `bench/scenarios/marqueur.json`.

**Test et commande bornée.** Preuve de tâche N = N_départ + 19, `tst_captures_corbeille` et `tst_captures_marqueur` Passed ;
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

**Test et commande bornée.** Commande ciblée `tst_bench_scenarios|tst_bench_result` ; preuve de tâche N = N_départ + 19 ;
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
  pour l'attendu `cache.refreshes ≤ k + 1` de S36. **Amendement 14** : gommer en deux morceaux le dernier trait
  encore **en attente** (couche d'attente du lot 10) coûte un redessin complet de plus (le premier morceau n'est pas en
  fin de page) : le scénario écrit aussi `pending_cut` (0 ou 1) et l'attendu devient `cache.refreshes ≤ k + 1 +
  pending_cut`. `tst_bench_result` :
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

**Test et commande bornée.** Preuve de tâche N = N_départ + 19, `tst_captures_ecrire_gommer_annuler` Passed ; preuve visuelle
hors écran `-Scenario ecrire-gommer-annuler -Lire "appDocument.shownObjectCount=1"` (état final) ; à l'écran
(jeton `ecran`, annoncé par l'orchestrateur) : même commande avec `-Rendu d3d11` sans `-HorsEcran` → `CAPTURES OK`.

**Interdits.** Aucun fichier de code : les étapes `pinch` et `touch` viennent du joueur du socle (L0-S2) ; si une
étape manque au joueur ou une lecture à l'interface, l'écrire dans le rapport et s'arrêter (tâche corrective).

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
- **H3-16** Deux doigts posés dans le même événement : le premier peut verser un point avant que le second ne
  déclenche le geste (les champs figés de `Sample` ne disent pas l'événement d'origine) ; à revoir avec les modes du
  toucher du lot 11 (amendement du socle : décision commune des appuis d'un même événement).
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
4. **Suites** (jeton `tests`) : `scripts/test.ps1 -Preset release` puis `scripts/test.ps1`, formes bornées, N = N_départ + 19
   chacune, `TESTS OK` ; aucun test sauté hors les trois tests locaux du lot 2 (rejoués avec leurs variables :
   `ORACLE OK`, `ORACLE SYNTHETIQUE OK`, `MINIATURES OK`, H3-15) ; **aucun talon `QSKIP` restant** :
   **règle 4 des talons du socle** : `git -C <wt> grep -n "Talon L0-S : rempli par L3-" -- . ':(exclude)docs/'` →
   sortie vide ; pour chaque test talon rempli, `build/debug/tests/<test>.exe -functions` rend au moins une fonction
   et aucune `reserved` ; chaque scénario talon rempli (`selection`, `corbeille`, `marqueur`,
   `ecrire-gommer-annuler`) produit au moins une ligne `TRAIT`, `DOIGT` ou `POINTEUR` ; sur `ctest --preset debug
   -V`, aucune ligne `SKIP   :` qui contienne `Talon L0-S`.
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
| M5 | Talons QML sans `required property`, réglette jamais ouverte | renvoyé au socle (emplacements QML avec leurs `required property`, `openTranslucency()`) | fait par L0-S6, L0-S9 |
| M6 | Entrées du lot 7 hors du routeur ; clic droit en conflit | renvoyé au socle (chemin d'entrée, routes des surcouches) | L0-S |
| M7 | Travail en double avec le lot 7 (sous-menus, commande de remplacement, zoom ancré) | renvoyé au socle | fait par L0-S3, L0-S4, L0-S7 ; L3-T4, L3-T7 amendés |
| M8 | S35 sans fixture, S34 incomplet | corrigé : `bench/src/fixtures.cpp` dans L3-T15 (`rectangle-et-traits`, `objet-verrouille`), S34 complété (poignée proportionnelle, agrandir vérifié, objet verrouillé) ; L3-T16 dépend de L3-T15 | L3-T15, L3-T16 |
| M9 | Échap jamais livré à `Tool::onKey` | corrigé : `activate()` annule l'outil occupé ; Échap pendant un geste = abandon ; test `escapeCancelsTheGesture` | L3-T14 |
| m1 | Dépendances : T11 et T2 sur `PageArea.qml` ; T14 sur T9 et T11 sans raison | corrigé : T11 dépend de T2 ; T14 ne dépend plus de T9 ni de T11 ; T15 dépend de T9 ; vagues V6 T9 ‖ T14, 8 vagues après le socle | table, vagues |
| m2 | Marqueur des talons | corrigé : contrôle de clôture = règle 4 des talons du socle (`Talon L0-S : rempli par L3-`) | clôture, étape 4 |
| m3 | S6 : `waitReady` avant le trait en attente | corrigé | L3-T18 |
| m4 | Ctrl+Maj+Z pris par le lot 7 | corrigé : D11 garde Ctrl+Y, conflit rendu au socle si la 3.5.18 dit autre chose | L3-T0 |
| m5 | Curseur du marqueur : cercle (F-201) | corrigé : `Qt.BlankCursor` + cercle (attendu 10) | L3-T16 |
| m6 | `saveWhileLiftingIsRefused` testait un dépôt | corrigé : renommé et dédoublé (`pageChangeWhileLiftingDropsFirst`, `saveWhileLiftingDropsFirst`) | L3-T12 |
| m7 | Trois demandes d'un tour → deux fins | corrigé : `cacheRefreshCountCounts` attend chaque fin ; `requestsOfOneTurnMakeOneRedraw` ajouté | L3-T13 |

## Amendements du socle (plan du socle 9b13275, « Amendements pour le lot 3 », 17 points)

| # | Amendement | Sort | Où |
|---|---|---|---|
| 1 | L3-T1 disparaît (L0-S0, S1, S4, S5, S6, S7) ; dépendances « L3-T1 » tombées | appliqué : section réduite à « retirée » ; L3-T0, L3-T3, L3-T10 en tête, après L0-S10 | table, sections |
| 2 | L3-T2 disparaît (L0-S9) | appliqué | table, sections |
| 3 | L3-T17 disparaît (L0-S2) ; L3-T18 n'en dépend plus | appliqué | table, sections, L3-T18 |
| 4 | L3-T4 : `ReplaceObjectsCommand`, `addedIds()`, `replaceStrokesKeepsRankAndIds` retiré, `objectedits.h` sans `StrokeReplacement` | appliqué | L3-T4 |
| 5 | L3-T5 : `sample.h` sort ; mécanisme du routeur posé ; L3-T5 remplit `FullTouchPolicy::decide` et `gesture.cpp` ; fonctions du socle vertes | appliqué ; M4 (a) « appuis d'un même événement » **écarté** (champs figés de `Sample` sans numéro d'événement) → H3-16 ; M4 (b) gardé avec arrêt et rapport si le mécanisme du socle ne le permet pas | L3-T5, H3-16 |
| 6 | L3-T6 : `tool.h` existe, `PenTool` déjà un `Tool`, `openMathTools` et `id()` réels | appliqué | L3-T6 |
| 7 | L3-T7 : `ReplaceObjectsCommand` | appliqué | L3-T7 |
| 8 | L3-T11 : `PageArea.qml` seulement pour une propriété d'`InkLayer`, aucune couche déplacée | appliqué (fichier gardé sous cette condition) | L3-T11 |
| 9 | L3-T12 : `addChangeObserver` (origines `Lift`, `Drop`), pas d'`editAboutToApply` au soulèvement, corps seulement | appliqué (observateur d'essai réécrit, origines vérifiées) | L3-T12 |
| 10 | L3-T13 : chemin d'entrée posé ; garder `foldWaitsForTheActiveTool`, `latencyProbeSeesToolSamples`, `pageChangeEndsToolStroke` ; retirer `cacheRefreshCountCounts` | appliqué ; L3-T13 placée **après** L3-T14 (seul moyen de prouver la branche « outil de `ToolHost` ») ; `dragOfTwoObjectsCostsTwoRedraws` passé de L3-T14 à L3-T13 (`tst_ui_drawing`) | L3-T13, vagues |
| 11 | L3-T14 : `user()` réel, `activeTool()` à remplir, test `finishStrokeCancelsTheActiveTool`, `Main.qml` gardé, Échap par `cancelGesture()` | appliqué | L3-T14 |
| 12 | L3-T15 : piège du gestionnaire sur la racine de `SelectionOverlay` ; test d'un trait au stylet hors du cadre | appliqué (section « Piège », étapes ajoutées au scénario `selection`) | L3-T15 |
| 13 | L3-T15, L3-T16 : `required property` déjà branchées, `commands.contextMenu`, `TrashCan` instancié, icônes posées | appliqué | L3-T15, L3-T16 |
| 14 | L3-T19 : dernier trait en attente gommé en deux morceaux = un redessin de plus | appliqué (`pending_cut` dans l'attendu) | L3-T19, S36 |
| 15 | L3-C0 : Ctrl+Maj+Z refusé par `noShortcutTwice`, tranché par F-750 et F-950 à F-953 | appliqué | vagues (« L3-T0 et les autres tâches ») |
| 16 | « Dépendances aux autres lots » (lot 7) réécrites | appliqué | Dépendances aux autres lots |
| 17 | 20 → 17 tâches ; N = N_départ + 19 ; vagues recalculées | appliqué : 17 tâches, 8 vagues de largeur 3 au plus | table, vagues |
| règles | `switch` sous `/we4062` (`Equation`, `PageSettingsChanged`) ; contrôle de clôture par la règle 4 des talons | appliqué (règles communes, L3-T3 `equationHitsLikeText`, L3-T12, L3-T13, L3-T14, clôture) | — |
