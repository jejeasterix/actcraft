# Livraison complète — socle commun des lots 3 et 7 (L0-S) — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**But.** Poser **une seule fois**, sur `feat/livraison-complete`, ce que les lots 3 (« Outils I ») et 7 (« Présentation
et outils ») toucheraient tous les deux en parallèle : données de commandes et de disposition, joueur de scénarios,
modèle (équation, réglages de présentation de la page, commande de remplacement unique), chemin d'entrée de la vue
(routeur, couvertures, filtre d'échantillons, garde du versement différé), contrôleur de document (API des deux lots,
observateur unique), coquille QML (couches et emplacements), enregistrement CMake de tous les fichiers et tests du
lot 3 et du socle, crochets CMake du lot 7. Après L0-S, la fusion des lots 3 et 7 ne porte plus que sur des fichiers
propres à chacun. Aucune fonction visible nouvelle : l'application se comporte comme à la fin du lot 10, sauf les
sous-menus de commandes inertes (aucun gestionnaire) et le menu contextuel par signal.

**Place dans le chantier.** Jouée sur `feat/livraison-complete` après le lot 10, avant toute tâche du lot 3. La
session B (lot 7) **tourne déjà** sur `feat/livraison-complete-b` : sa phase 1 travaille dans ses propres fichiers
(`features/`, contrôleurs, QML du lot, `ui/presentation.cmake`, `tests/presentation.cmake`) ; elle reprend le socle par
fusion (section « Ce que la session B doit savoir »). Le squelette du lot 7 (ancienne L0-S8) **n'est pas** dans le
socle : il est fait par la session B. Aucune version n'est écrite (le lot 3 prend 0.4.0). Écrit sur 333c811 (L10-T4
compris) : les numéros de ligne du code sont indicatifs, les noms font foi.

**Sources figées (lecture seule, objets git immuables, lisibles de tout worktree du dépôt).**
- Plan du lot 3 : `git -C <wt> show 61c03fb:docs/plans/2026-10-18-livraison-complete-lot3-PLAN.md` (cité « P3:<ligne> »).
- Plan du lot 7a : `git -C <wt> show bf8c930:docs/plans/2026-10-18-livraison-complete-lot7-PLAN.md` (« P7a:<ligne> »).
- Plan du lot 7b : `git -C <wt> show bf8c930:docs/plans/2026-10-18-livraison-complete-lot7b-PLAN.md` (« P7b:<ligne> »).
Une section qui dit « copier P3:624-1468 » demande d'extraire ces lignes par `git show … | Select-Object -Skip 623
-First 845` et d'en reprendre les blocs de code **mot pour mot**, puis d'appliquer les écarts listés dans la section.

**Pile.** C++20, Qt 6.12 (Core, Gui, Quick, QuickControls2, Svg, Test), CMake + Ninja, MSVC 2022, PowerShell 7.

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L0-S0 | Réservation : tous les fichiers neufs du socle et du lot 3 créés vides et marqués, enregistrés dans CMake ; 19 tests réservés ; crochets CMake du lot 7 | — | `core/CMakeLists.txt`, `input/CMakeLists.txt`, `tools/CMakeLists.txt`, `render/CMakeLists.txt`, `ui/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/{core,input,tools,render,ui,bench}.cmake`, fichiers neufs vides : `core/include/actcraft/core/{replace,equation,hittest,erasegeometry,objectedits,lift}.h`, `core/src/{replace,equation,hittest,erasegeometry,objectedits,lift}.cpp`, `render/src/equationlayout.{h,cpp}`, `render/include/actcraft/render/liftlayeritem.h`, `render/src/liftlayeritem.cpp`, `input/include/actcraft/input/{routing,gesture}.h`, `input/src/{routing,gesture}.cpp`, `tools/include/actcraft/tools/{tool,toolcontext,userstate,toolregistry,erasertool,selecttool,selectiongeometry}.h`, `tools/src/{userstate,toolregistry,erasertool,selecttool,selectiongeometry}.cpp`, `ui/include/actcraft/ui/toolhost.h`, `ui/src/{toolhost,tooladapters}.cpp`, `ui/src/tooladapters.h`, `ui/qml/{SelectionOverlay,ObjectBar,TranslucencyPopup,TrashCan,PageContextMenu,ToolCursor,Presentation}.qml`, `tests/tst_{core_replace,core_hittest,core_erase,core_objectedits,core_lift,input_routing,input_gesture,tools_registry,tools_eraser,tools_select,render_liftlayer,render_strokeshape,ui_lift,ui_tools,ui_inputpath}.cpp`, `bench/scenarios/{selection,corbeille,marqueur,ecrire-gommer-annuler}.json` | isolé | implementeur-tdd | `TESTS OK`, N = N_départ + 19 (N_départ relevé avant), les 19 tests neufs Passed (`SKIP` nommant leur tâche) ; `premiers-pas` hors écran `CAPTURES OK` ; `NOMS OK` (après `git add`, puis après le commit) | S38 | #tache-l0-s0 |
| L0-S1 | Icônes des deux lots (31 SVG à nous) enregistrées dans le module QML | L0-S0 | `ui/icons/{selection,marqueur,gomme,annuler,retablir,supprimer,corbeille,effacer,deplacer,rotation,translucidite,agrandir,reduire,rideau,spot,zoom-page,effets,regle,equerre,rapporteur,compas,origine,des,calculatrice,horloge,message,forme-reconnue,ecriture,camera,clavier,bureau}.svg`, `ui/CMakeLists.txt` (bloc `RESOURCES` seulement), `tests/tst_ui_theme.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_départ + 19, `tst_ui_theme` Passed dont `iconsAreOurSvgOnA24Grid` avec 41 lignes de données (10 + 31) ; `NOMS OK` | S10, S34 | #tache-l0-s1 |
| L0-S2 | Joueur de scénarios : étapes `touch`, `pinch`, `pointer`, `doubleClick`, `expect` avec tolérance | L0-S0 | `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`, `bench/include/actcraft/bench/injector.h`, `bench/src/injector.cpp`, `tests/tst_bench_uiscenario.cpp`, `tests/tst_bench_injector.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_départ + 19, `tst_bench_uiscenario` (dont `touchStepParsed`, `pinchStepParsed`, `pointerStepParsed`, `doubleClickStepParsed`, `expectWithTolerance`, `stepKindRanksFrozen`) et `tst_bench_injector` (dont `pinchDeliversTwoTouchPoints`, `pointerHoldsBeforeRelease`) Passed ; `premiers-pas` hors écran `CAPTURES OK` | S6, S10 | #tache-l0-s2 |
| L0-S3 | Modèle partagé : `ObjectKind::Equation`, `PagePresentation`, `ReplaceObjectsCommand` unique, `SetPagePresentationCommand` sans redessin, format natif sans perte, repère neutre de l'équation, C4062 en erreur | L0-S0 | `core/include/actcraft/core/model.h`, `core/src/model.cpp`, `core/src/edits.cpp`, `core/include/actcraft/core/undo.h`, `core/src/pagecache.cpp`, `core/include/actcraft/core/replace.h`, `core/src/replace.cpp`, `core/include/actcraft/core/equation.h`, `core/src/equation.cpp`, `formats/src/nativejson.cpp`, `render/src/pagepainter.cpp`, `render/src/paintedrenderer.cpp`, `render/src/equationlayout.h`, `render/src/equationlayout.cpp`, `cmake/ActCraftModule.cmake`, `tests/tst_core_replace.cpp`, `tests/tst_core_model.cpp`, `tests/tst_formats_nativejson.cpp`, `tests/tst_render_painter.cpp` (et tout fichier où C4062 / `-Wswitch` signale un `switch` sur `ObjectKind` ou `Change::Kind` : liste rendue au rapport) | isolé | concepteur-critique | `TESTS OK`, N = N_départ + 19, `tst_core_replace` Passed (dont `piecesTakeTheOriginalRank`, `groupOfTwoTakesTheHigherRank`, `cacheSeesRemoveThenInsert`, `undoIsTheExactInverse`, `addedIdsStableAcrossRedo`, `presentationTouchesNoCache`), `tst_core_model` (dont `equationKindIsBeforeUnknown`, `fingerprintUnchangedWithoutPresentation`), `tst_formats_nativejson` (dont `equationRoundTripKeepsMathml`, `presentationRoundTrip`, `unknownPresentationKeyKept`, `unknownOnlyPresentationKept`), `tst_render_painter` (dont `equationDrawsTheNeutralFrame`) Passed ; mutation de l'ordre des `Change` rouge puis remise ; banc hors écran `content_check.ok = True` | S24, S31, S13 | #tache-l0-s3 |
| L0-S4 | Données et disposition des deux lots : 62 commandes, argument texte, sous-menus (forme du lot 3) filtrés par plateforme, menu Outils dans l'ordre de F-750, barre d'objet, menu contextuel, raccourcis uniques | L0-S1 | `actions/data/commands.json`, `actions/data/layout.json`, `actions/include/actcraft/actions/layout.h`, `actions/src/layout.cpp`, `actions/include/actcraft/actions/commandregistry.h`, `actions/src/commandregistry.cpp`, `tests/tst_actions_layout.cpp`, `tests/tst_actions_registry.cpp`, `ui/include/actcraft/ui/commands.h`, `ui/src/commands.cpp`, `ui/qml/CommandMenu.qml`, `tests/tst_ui_commands.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_départ + 19, `tst_actions_layout` (dont `submenuItems`, `objectBarRead`, `contextMenuRead`, `lot3EntriesDeclared`, `lot7EntriesPlaced`, `toolsMenuFollowsF750`), `tst_actions_registry` (dont `textArgument`, `lot7CommandsDeclared`, `noShortcutTwice`), `tst_ui_commands` (dont `submenuInMenus`, `submenuFilteredByPlatform`, `objectBarAndContextMenu`) Passed ; `premiers-pas` hors écran `CAPTURES OK` ; `NOMS OK` | S10, S34, S35 | #tache-l0-s4 |
| L0-S5 | `DocumentController` : API des deux lots (corps vides, sans `editAboutToApply` au soulèvement), observateur unique des modifications versées | L0-S3 | `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`, `tests/tst_ui_document.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_départ + 19, `tst_ui_document` Passed (dont `observerGetsTheCacheChanges`, `observerSeesTheOrigin`, `removedObserverIsSilent`, `replaceInTheMiddleIsNotDeferred`, `presentationChangeRedrawsNothing`, `shownObjectCountCounts`, `liftDoesNotAnnounceAnEdit`) ; mutation de l'origine `Stroke` rouge puis remise ; banc hors écran `content_check.ok = True` | S38, S13 | #tache-l0-s5 |
| L0-S6 | Squelette du lot 3 : en-têtes figés et talons (`core`, `input`, `tools`, `render`, `ui`), `LiftLayer` exposé au QML, QML à `required property`, `openMathTools` et `ToolHost::user()` réels | L0-S0 | `core/include/actcraft/core/{hittest,erasegeometry,objectedits,lift}.h`, `core/src/{hittest,erasegeometry,objectedits,lift}.cpp`, `input/include/actcraft/input/{routing,gesture}.h`, `input/src/{routing,gesture}.cpp`, `tools/include/actcraft/tools/{tool,toolcontext,userstate,toolregistry,erasertool,selecttool,selectiongeometry}.h`, `tools/src/{userstate,toolregistry,erasertool,selecttool,selectiongeometry}.cpp`, `render/include/actcraft/render/liftlayeritem.h`, `render/src/liftlayeritem.cpp`, `ui/include/actcraft/ui/toolhost.h`, `ui/src/{toolhost,tooladapters}.cpp`, `ui/src/tooladapters.h`, `ui/src/uiforeigntypes.h`, `ui/qml/{SelectionOverlay,ObjectBar,TranslucencyPopup,TrashCan,PageContextMenu,ToolCursor}.qml`, `tests/tst_tools_registry.cpp`, `tests/tst_ui_tools.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_départ + 19, talons en `SKIP` nommant leur tâche, `tst_tools_registry` (dont `openMathToolsKept`) et `tst_ui_tools` (dont `oneUserForNow`, `liftLayerTypeIsRegistered`) en `PASS` ; `premiers-pas` hors écran `CAPTURES OK` ; banc hors écran `content_check.ok = True` ; `NOMS OK` | S38 | #tache-l0-s6 |
| L0-S7 | Chemin d'entrée figé : événement → `Sample` → `PointerRouter` (couvertures) → filtre d'échantillons → `Pipeline` → outil actif ; garde du différé sur `isBusy`, menu contextuel par signal, `zoomAround` unique | L0-S6 | `input/include/actcraft/input/sample.h`, `input/src/qtevents.cpp`, `input/src/routing.cpp`, `tools/include/actcraft/tools/pentool.h`, `tools/src/pentool.cpp`, `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`, `ui/src/toolhost.cpp`, `tests/tst_ui_inputpath.cpp`, `tests/tst_input_routing.cpp`, `tests/tst_pentool.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_départ + 19, `tst_ui_inputpath` Passed (dont `coverStopsTablet`, `coverStopsFinger`, `coverStopsMouse`, `coverHandlerGetsThePress`, `uncoveredStillDraws`, `coveredStylusTapLeavesFingersFree`, `filterReachesThePen`, `syntheticReleaseIsFiltered`, `foldWaitsForTheBusyTool`, `filteredStrokeStaysPendingUntilIdle`, `rightClickRequestsTheContextMenu`, `coverTakesTheRightClick`, `zoomAroundKeepsThePointStill`, `cacheRefreshCountCounts`), `tst_input_routing` (dont `coverRouteHeldForWholeContact`, `coverLeavesTheStateAlone`, `pressRedecides`), `tst_pageview`, `tst_ui_drawing`, `tst_pentool` Passed ; mutation de la consultation des couvertures rouge puis remise ; banc hors écran `content_check.ok = True` ; `cache-differe` hors écran `CAPTURES OK` | S38, S13 | #tache-l0-s7 |
| L0-S9 | Coquille QML : ordre des couches figé, emplacements `liftLayer`, `pageOverlays`, `selectionOverlay`, `toolCursor`, `presentationSlot`, `presentationLayer`, racine `Presentation` (talon minimal), `appTools`, gestionnaires des commandes du lot 3 | L0-S4, L0-S5, L0-S7 | `ui/qml/Main.qml`, `ui/qml/PageArea.qml`, `ui/qml/ToolPalette.qml`, `ui/qml/Presentation.qml`, `tests/tst_ui_shell.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_départ + 19, `tst_ui_shell` Passed (dont `socleLayerOrder`, `socleSlotsExist`, `everyLot3CommandHasAHandler`, `contextMenuOpensFromTheView`, `pageAreaWithoutToolsDraws`, `shellLoadsWithoutQmlWarning`) ; `premiers-pas` hors écran `CAPTURES OK` (`-Lire "pageIndicator.text=1 / 2"`) ; `cache-differe` et `pauses` hors écran `CAPTURES OK` ; banc hors écran `content_check.ok = True` | S38, S13 | #tache-l0-s9 |
| L0-S10 | Contrôle du socle : suites debug et release, talons comptés, graphe | L0-S0 à L0-S7, L0-S9 | `graphify-out/` | partagé | implementeur-tdd | debug et release `TESTS OK`, N = N_départ + 19 ; comptes des talons (`git grep … -- . ':(exclude)docs/'`) relevés et notés dans l'état (référence des clôtures) ; aucun `QSKIP` sans tâche nommée ; captures hors écran `-Scenario tous` `CAPTURES OK` ; banc hors écran joint ; `NOMS OK` ; `graphify update .` commité | S38, S13 | #tache-l0-s10 |

9 tâches (L0-S8 sortie vers la session B : identifiant laissé libre pour ne renuméroter aucun renvoi), un seul plan.
**N_départ** = nombre de tests ctest de `feat/livraison-complete` au départ du socle, relevé par L0-S0 (étape 1) et noté
dans l'état du chantier par l'orchestrateur (94 attendu si le lot 10 n'en a pas ajouté depuis L10-T4). L0-S0 enregistre
**tous** les tests du socle : `tst_core_replace`, les 17 du lot 3 et `tst_ui_inputpath` → **N = N_départ + 19** dès
L0-S0 et jusqu'à la fin ; aucune autre tâche du socle ni du lot 3 n'en enregistre. Les tests du lot 7 s'ajoutent à la
fusion de B (L7-T33) : N = N_départ + 19 + n_B.

## Vagues (largeur 3 au plus ; aucun fichier partagé dans une vague)

V0 : L0-S0 · V1 : L0-S3 ‖ L0-S6 ‖ L0-S1 · V2 : L0-S7 ‖ L0-S5 ‖ L0-S4 · V3 : L0-S9 ‖ L0-S2 · V4 : L0-S10.

Vérification des recoupements (colonne « fichiers touchés ») :
- V1 : L0-S3 (`core/` modèle et commandes, `formats/`, `render/` peintre, `cmake/ActCraftModule.cmake`, quatre tests) /
  L0-S6 (en-têtes et talons du lot 3, `uiforeigntypes.h`, six QML, deux tests) / L0-S1 (`ui/icons/`, bloc `RESOURCES`,
  `tst_ui_theme.cpp`) — disjoints. Aucun ne touche un fichier CMake de liste : L0-S0 les a tous posés.
- V2 : L0-S7 (`input/`, `pentool.*`, `pageview.*`, `toolhost.cpp`, trois tests) / L0-S5 (`documentcontroller.*`,
  `tst_ui_document.cpp`) / L0-S4 (`actions/`, `ui/…/commands.*`, `CommandMenu.qml`, trois tests) — disjoints. Les tests
  de L0-S7 comptent les objets par `fixedPage->objectCount()` (et non `shownObjectCount`, posé par L0-S5).
- V3 : L0-S9 (`Main.qml`, `PageArea.qml`, `ToolPalette.qml`, `Presentation.qml`, `tst_ui_shell.cpp`) / L0-S2
  (`bench/`, deux tests) — disjoints.
- Fichiers repris d'une vague à l'autre, tenus par « dépend de » : fichiers réservés par L0-S0 → leur tâche ;
  `ui/CMakeLists.txt` (L0-S0 → L0-S1) ; `input/src/routing.cpp`, `ui/src/toolhost.cpp` (talons L0-S6 → L0-S7) ;
  `tests/tst_ui_inputpath.cpp`, `tests/tst_input_routing.cpp` (L0-S0 → L0-S7) ; `ui/qml/Presentation.qml` (L0-S0 →
  L0-S9).

**Places.** Deux constructions de front pour le socle ; la session B prend la troisième place du jeton `tests`. Une
vague de trois tâches qui construisent a donc **toujours une tâche qui attend** la notification du jeton : c'est
normal et prévu ; l'agent attend, il ne sonde pas. Ordre de lancement : V1 L0-S3 puis L0-S6 (chemin critique), L0-S1
ensuite ; V2 L0-S7 puis L0-S5, L0-S4 ensuite.

**Écran.** Aucune tâche n'ouvre de fenêtre visible : tout se joue hors écran (rendu logiciel). Le passage à l'écran
du socle est celui de la porte de fin du lot 3 (S6 en D3D11) : le socle ne change rien de visible.

Worktrees : `.worktrees/livraison-complete--L0-S<n>`, branche `feat/livraison-complete--L0-S<n>`, issue de
`feat/livraison-complete` à jour des tâches déjà mergées.

## Décisions appliquées (tranchées par l'orchestrateur, amendement 1 faisant autorité)

- **D-S1 (B1, B2, M6 ; précisée en accord avec DA-A1).** Toute entrée passe par `input::PointerRouter`. Une surcouche
  du lot 7 (rideau, spot, zoom de page, panneaux, outils de maths) reste une surcouche QML autonome, pas un
  `tools::Tool` ; elle **se déclare couverture** par `PageView::addInputCover(QQuickItem*)`. Une couverture est une
  **route de la politique** : à l'appui, le routeur appelle son test de couverture (branché par la vue sur
  `coveredAt`) et, s'il répond vrai, rend `Route::Cover` ; la vue ne prend alors rien, **rend aussitôt le contact au
  routeur** (`release`) et l'événement continue vers la surcouche, qui le reçoit par ses gestionnaires QML. Une
  décision `Cover` **ne modifie jamais `RouterState`** (un appui couvert du stylet ne bloque pas les doigts). Le
  stylet, capté par le filtre de fenêtre, passe par la même décision. Ordre figé : événement → `Sample` →
  `PointerRouter` (couvertures) → filtre d'échantillons (voie `Tool` seulement) → `Pipeline` (numérotation, sonde,
  doublons) → outil actif. Clic droit : signal unique `PageView::contextMenuRequested`, après un test de couverture
  direct (`sampleFromEvent` ne rend rien pour le bouton droit : le routeur n'est pas consulté). Guides de tracé :
  `PageView::setSampleFilter`, appliqué dans `handlePointer` ; les relâchés synthétisés (`finishStroke`,
  `finishTouchStroke`) passent par le même filtre à partir du dernier point brut, donc tombent sur le dernier point
  filtré. `coveredAt` regarde les sœurs de la vue (comportement du lot 1) **et** les couvertures déclarées : visible,
  active, et `cover->contains(cover->mapFromScene(point))` (le `containmentMask` de la surcouche est respecté).
- **D-S2.** Sous-menus : forme du lot 3 (`{"submenu": id, "label", "items"}`, jeton `>id`, `MenuSpec::submenus`), plus
  le filtrage par plateforme du lot 7 (sous-menu sans commande disponible : omis).
- **D-S3.** Toutes les commandes des deux lots dans `commands.json`, en blocs contigus (lot 3 puis lot 7), puis
  `layout.json` (menus, sous-menus, groupes de palette, barre d'objet, menu contextuel), `ArgumentKind::Text`, test
  d'absence de raccourci en double sur tout le fichier. Menu Outils dans l'ordre de F-750 (H-S3 tranchée).
- **D-S4.** Joueur de scénarios : `Touch`, `Pinch`, `Pointer`, `DoubleClick` (dans cet ordre, après `CheckEdge`),
  `expect` avec tolérance. Absorbe L3-T17 et l'étape 8 de L7-T2.
- **D-S5.** `ObjectKind::Equation`, `PagePresentation` (avec ses sous-clés inconnues gardées) ; **une seule** commande
  de remplacement, `core::ReplaceObjectsCommand` (remplace `ReplaceStrokesCommand` du lot 3) avec son contrat de
  `Change` asserté (B4) ; `SetPagePresentationCommand` dont le `Change` (`PageSettingsChanged`) n'atteint pas le cache
  (I3) ; repère neutre de l'équation ; équation et réglages de page relus et réécrits sans perte au format natif
  (L2-C8). C4062 en erreur sous MSVC (`/we4062`) : un `switch` incomplet échoue en local comme en CI.
- **D-S6.** Ordre des couches figé ; talons honnêtes (section « Talons »).
- **D-S7.** Un seul observateur des modifications versées (`DocumentController::addChangeObserver`), qui remplace
  `setCacheObserver` (lot 3) et `setStrokeObserver` (lot 7).
- **D-S8.** Insertions du lot 7 hors de `documentcontroller.*` (`execute()` et `document()` sont publics) : fichier du
  lot 7, écrit par la session B.
- **D-S9 (M1 du lot 3).** La minuterie de versement et `finishStroke` reposent sur `PageView::currentTool()->isBusy()`
  et `cancel()` (outil de `ToolHost`, sinon stylo interne), plus sur `m_pen`. Le `Pipeline` reste devant l'outil.
  Au socle, `ToolHost::activeTool()` est un talon (`nullptr`) : la branche « outil de `ToolHost` » est prouvée par le
  lot 3 (`foldWaitsForTheActiveTool` en L3-T13, `finishStrokeCancelsTheActiveTool` en L3-T14).
- **D-S10.** `tools::UserState::openMathTools` (réel) et `ui::ToolHost::user()` (réel, un utilisateur) posés par le
  socle ; le lot 7 y range ses outils de maths ouverts.
- **Sortie de L0-S8.** Le squelette du lot 7 est fait par la session B. Le socle pose seulement les crochets : `ui/`
  lit `ui/presentation.cmake` (listes `ACTCRAFT_UI_LOT7_QML`, `ACTCRAFT_UI_LOT7_SOURCES`), `tests/` lit
  `tests/presentation.cmake`, tous deux en `OPTIONAL` ; `ui/qml/Presentation.qml` est créé et enregistré par le socle.
- **Relectures intégrées** : du lot 3 (B1, M1, M5, M6, M7, mineur 2) et du socle (bloquant 1, importants 2 à 12,
  mineurs).

## Talons : règle d'honnêteté (vérifiable)

1. **Marqueur unique** `Talon L0-S : rempli par <tâche>.` — en commentaire C++ (`// …`) dans chaque `.h`/`.cpp`
   talon, en commentaire QML dans chaque `.qml` talon, dans le champ `description` de chaque scénario JSON talon, et
   comme texte du `QSKIP` de chaque test talon : `QSKIP("Talon L0-S : rempli par L3-T3.");`. `<tâche>` est **une**
   tâche d'un plan (`L0-S<n>`, `L3-T<n>` ou `L7-T<n>`), jamais « plus tard ».
2. **Un test talon n'a qu'une fonction sautée**, `reserved()`. Il peut porter en plus des fonctions réelles du socle
   (elles passent) : la tâche propriétaire **supprime `reserved()`** et ajoute les siennes, sans toucher à celles du
   socle.
3. **Compteur.** Le socle fixe N = N_départ + 19 ; aucune tâche du lot 3 n'enregistre de test ; la session B ajoute
   les siens dans `tests/presentation.cmake` (fusion L7-T33). Chaque preuve donne N exact **et** les fonctions
   attendues en `PASS` (un `SKIP` ne prouve rien).
4. **Contrôle de clôture** (à recopier dans la clôture de chaque lot ; `docs/` exclu, sinon le grep trouve les plans et
   l'état) :
   - `git -C <wt> grep -n "Talon L0-S : rempli par L3-" -- . ':(exclude)docs/'` vide à la clôture du lot 3 ;
     `"… par L7-"` vide à la clôture du lot 7 ; `"… par L0-"` vide à la fin du socle ;
   - pour chaque test talon rempli : `build/debug/<test>.exe -functions` rend au moins une fonction et aucune
     `reserved` ;
   - pour chaque scénario talon rempli : sa capture produit au moins une ligne `TRAIT`, `DOIGT` ou `POINTEUR` (un
     `waitReady` seul ne prouve rien) ;
   - sur la sortie de `ctest --preset debug -V`, aucune ligne `SKIP   :` qui contienne `Talon L0-S`.
5. Un en-tête **figé** (texte public donné par le plan d'un lot) n'est pas un talon : seul son `.cpp` l'est. Ses
   signatures publiques ne changent que par amendement du plan.

## Scénarios

- **S38 — Socle commun (ajouté par ce plan).** Sur l'application du socle : (a) une surcouche déclarée couverture,
  posée sur la page, ne laisse tracer ni le stylet (`QTabletEvent` injecté), ni un doigt, ni la souris, et son
  gestionnaire reçoit l'appui ; ôtée, les trois tracent ; un appui couvert du stylet laisse les doigts libres ; (b) un
  filtre d'échantillons qui décale x de +50 décale le trait versé, relâché synthétisé compris ; (c) un trait tenu
  immobile plus de 500 ms n'est pas versé pendant l'écriture ; (d) les 62 commandes se chargent, aucun raccourci en
  double, sous-menus visibles sous Windows, absents sous Android quand vides, menu Outils dans l'ordre de F-750 ;
  (e) l'ordre des couches de la page est `fixedPage`, `pageCache`, `pendingLayer`, `liftLayer`, `ink`, `pageOverlays` ;
  (f) N = N_départ + 19, chaque talon nomme sa tâche.
- Scénarios servis du cadrage et des lots : S6 (joueur : pincement), S10 (commandes et icônes du lot 7), S13
  (non-régression du cache différé : `cache-differe`, `pauses`), S24 et S31 (équation et objet relus et réécrits sans
  perte), S34, S35 (commandes et disposition du lot 3).

| Scénario | Tâches |
|---|---|
| S38 | L0-S0, L0-S5, L0-S6, L0-S7, L0-S9, L0-S10 |
| S13 | L0-S3, L0-S5, L0-S7, L0-S9, L0-S10 |
| S6 | L0-S2 |
| S10 | L0-S1, L0-S2, L0-S4 |
| S24, S31 | L0-S3 |
| S34 | L0-S1, L0-S4 |
| S35 | L0-S4 |

## Contrats figés par le socle (noms)

Tous les noms des lots 1, 2 et 10 restent ; on ajoute, on ne renomme pas.

- **`input`** (L0-S6, L0-S7) : `Sample` gagne, **à la fin**, `std::uint64_t pointer = 0;` (stylet et souris : 0 ;
  doigt : identifiant du point + 1) et `std::uint8_t user = 0;`. `Route` = `Tool, Gesture, Ignore, Cover` (`Cover`
  ajouté en dernier). `PointerRouter::setCoverTest(std::function<bool(const Sample&)>)`, `release(std::uint64_t
  pointer)` (contact terminé ou non pris par la vue). À l'appui : décision retenue remplacée ; test de couverture
  d'abord ; `Cover` sans effet sur `RouterState`.
- **`tools`** (L0-S6, L0-S7) : `tool.h`, `toolcontext.h`, `userstate.h`, `toolregistry.h`, `erasertool.h`,
  `selecttool.h`, `selectiongeometry.h` de P3:958-1295 ; `PenTool final : public Tool` (`id()` = « pen », `isBusy()` =
  trait en cours, `cancel()` = verse le trait en cours).
- **`ui::PageView`** (L0-S7) : `tools`, `cacheRefreshCount`, `gestureActive` ; `addInputCover`, `removeInputCover`,
  `setSampleFilter`, `zoomAround(QPointF viewPoint, qreal zoom)`, `sceneToPage`, `currentTool()`, `router()` ;
  signal `contextMenuRequested(QPointF viewPos)`.
- **`ui::ToolHost`** (L0-S6) : en-tête de P3:1345-1456 plus `tools::UserState& user(input::UserId id =
  input::kFirstUser);` ; type QML `LiftLayer` exposé au module `ActCraft.Ui` (`ui/src/uiforeigntypes.h`).
- **`ui::DocumentController`** (L0-S5) : API de P3:1472-1498 sans `setCacheObserver`, plus `ChangeOrigin`,
  `ChangeObserver`, `addChangeObserver`, `removeChangeObserver`.
- **`core`** (L0-S3) : `Equation`, `ObjectKind::Equation = 8`, `Unknown = 9`, `kObjectKindCount = 10`,
  `PagePresentation` et ses énumérations (P7a:209-220, champ `spotlightFrame`, plus `std::string unknownKeys`),
  `Change::Kind::PageSettingsChanged` (dernier), `ReplaceObjectsCommand`, `ObjectReplacement`,
  `SetPagePresentationCommand` (`core/include/actcraft/core/replace.h`).
- **`bench`** (L0-S2) : `UiStepKind` … `CheckEdge, Touch, Pinch, Pointer, DoubleClick`.
- **CMake** (L0-S0) : `ACTCRAFT_UI_LOT7_QML`, `ACTCRAFT_UI_LOT7_SOURCES` (listes lues dans `ui/presentation.cmake`) ;
  `include(${CMAKE_CURRENT_SOURCE_DIR}/presentation.cmake OPTIONAL)` en dernière ligne de `tests/CMakeLists.txt`.
- **QML** (L0-S9) : `appTools`, `liftLayer`, `pageOverlays`, `selectionOverlay`, `toolCursor`, `presentationSlot`,
  `pageContextMenu`, `trashCan`, `presentationLayer`, `presentation` ; `Presentation.qml` : propriétés `commands`,
  `documentController`, `tools`, `pageArea`, `pageOverlays`, `layer`.

## Règles communes à toutes les tâches (jointes à chaque section par l'orchestrateur)

Celles des plans 2a, 2b et 10 s'appliquent toutes, mot pour mot (`docs/plans/2026-10-15-livraison-complete-lot2a-PLAN.md`,
`…-lot2b-PLAN.md`, `docs/plans/2026-10-18-livraison-complete-lot10-PLAN.md`, « Règles communes ») : processus enfant
pour toute commande PowerShell, `git -C` et chemins absolus, **jamais de `cd`**, `format.ps1` avant la preuve, pièges de
Clang (`-Wshadow` : aucune capture ni variable locale qui masque un membre ou une liaison structurée ; conversions
explicites ; pas d'agrégat partiel par position), pas de `std::visit` direct sur `PageObject` (passer par `body()`),
**aucune API privée de Qt** dans le code livré, `core/` sans interface graphique, tout passe par une commande nommée,
propriété intellectuelle (aucune ressource de Promethean : icônes, textes et noms à nous), `samples/` en lecture
seule, commits explicites en français terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`, arrêt
par PID, fils de Qt Quick (tout état lu par `updatePaintNode` écrit sur le fil principal seulement). S'y ajoutent :

- `<wt>` = `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L0-S<n>` ; `<id>` = `L0-S<n>`.
- **Graphe d'abord** : `graphify query "…"` lancé dans `<wt>` avant de lire des fichiers pour localiser du code.
- **Jeton** : toute construction, suite, banc ou capture passe par
  `C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id>` ; une commande par prise ; attendre
  sa notification, ne pas le sonder en boucle.
- **Commande ciblée** (rouge, vert) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande '. ./scripts/dev-env.ps1; cmake --build --preset debug *> .tmp/build.txt; ctest --preset debug -R ''<motif>'' --output-on-failure *> .tmp/ctest.txt'; Select-String -Path .tmp/build.txt,.tmp/ctest.txt -Pattern 'error|tests passed|tests failed|No tests were found' | Select-Object -Last 4"
  ```
  Premier passage dans un worktree neuf : `-Commande 'pwsh -NoProfile -File ./scripts/build.ps1 *> .tmp/build.txt'`.
  Rouge attendu : `tests failed` ou erreur de compilation sur le nom testé ; vert : `100% tests passed`.
- **Fonctions d'un test** :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; . ./scripts/dev-env.ps1; & build/debug/<test>.exe -o .tmp/<test>.txt,txt; Select-String -Path .tmp/<test>.txt -Pattern '^PASS   :|^FAIL!  :|^SKIP   :|^Totals' | Select-Object -Last 40"
  ```
  (chemin : celui que `ctest --preset debug -N -V -R <test>` imprime s'il diffère.)
- **Preuve de tâche** (forme bornée du profil) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1'; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2; Select-String -Path .tmp/test.txt -Pattern '<nom_du_test> .*Passed' | Select-Object -Last 1"
  ```
  Attendu : `100% tests passed, 0 tests failed out of N` (N de la table), `TESTS OK`, la ligne `Passed` du test nommé,
  puis les fonctions nommées en `PASS` par la commande « Fonctions d'un test ».
- **Banc sans écran** (tâche qui touche `render/`, `input/`, `tools/` ou `ui/` ; résultat, pas une porte) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu logiciel -Scenario court -Taille 1280x800 -HorsEcran > .tmp/bench.txt 2>&1'; Select-String -Path .tmp/bench.txt -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE' | Select-Object -Last 1"
  pwsh -NoProfile -Command "Set-Location '<wt>'; `$j = Get-ChildItem bench/results/*.json | Sort-Object LastWriteTime | Select-Object -Last 1; `$o = Get-Content -Raw `$j | ConvertFrom-Json; Write-Output (`$j.Name + ' content_check.ok = ' + `$o.content_check.ok)"
  ```
  Attendu : `BUDGET TENU` ou `BUDGET DEPASSE` (jamais `MESURE INVALIDE`), `content_check.ok = True`, nom du JSON joint.
- **Preuve visuelle hors écran** (lectures d'abord, puis une phrase par capture) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario <nom> -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire ''<objet.propriété=attendu>'' > .tmp/captures.txt 2>&1'; Select-String -Path .tmp/captures.txt -Pattern '^(COMMANDE|TRAIT|TOUCHE|DOIGT|PINCEMENT|POINTEUR|DOUBLECLIC|ATTENTE|PRET|FENETRE|LU|PIXEL|BORD|CAPTURE) |^SCENARIO |CAPTURES (OK|EN ECHEC)' | Select-Object -Last 16"
  ```
  Échec : `CAPTURES EN ECHEC`, `SCENARIO EN ECHEC`, `SCENARIO INVALIDE`, toute ligne `ECART` ou `ECHEC`, `FENETRE …`.
- **Contrôle des noms** : `python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" <wt>`
  → `NOMS OK`, avant chaque commit qui ajoute du texte. **Piège** : le script ne lit que les fichiers suivis par git
  (`git ls-files`) ; un fichier neuf non ajouté passe toujours. Donc : `git -C <wt> add <fichiers neufs>` **avant** le
  contrôle, puis contrôle relancé **après** le commit ; les deux sorties `NOMS OK` vont au rapport. Un nom copié des plans sources (P3, P7a, P7b) que le
  contrôle refuse est renommé (nom à nous), et le renommage est rapporté : il devient un amendement du plan du lot
  concerné (voir l’amendement 13 du lot 7).
- **Mutation** (tâches qui en demandent une) : on casse une ligne, la commande ciblée doit passer au rouge, puis on
  **remet le fichier par `git -C <wt> checkout -- <fichier>`**. Une remise par `Copy-Item` garde l'ancienne date et
  ninja ne recompile pas : si on remet autrement, poser `(Get-Item <fichier>).LastWriteTime = Get-Date` avant de
  reconstruire. La commande ciblée doit repasser au vert après la remise.
- **moc** : pas d'apostrophe dans une chaîne brute `R"(...)"` d'un test (piège L2-T10).
- **Talons** : règle de la section « Talons » ; texte figé copié des sources, jamais réécrit de mémoire.

---

## Tâches

<a id="tache-l0-s0"></a>
### Tâche L0-S0 — Réservation : fichiers neufs et tests du socle, crochets du lot 7

**Objectif.** Créer **vides et marqués** tous les fichiers neufs du socle et du lot 3, et les enregistrer d'un coup dans
tous les fichiers CMake de listes, avec les 19 tests réservés et les crochets du lot 7. Après elle, aucune tâche du
socle ni du lot 3 ne touche plus un fichier CMake de liste (sauf `ui/CMakeLists.txt`, bloc `RESOURCES`, par L0-S1) : la
dépendance L0-S3 → L0-S6 tombe.

**Fichiers.** Voir la table.

**Test et commande bornée.** Relevé de N_départ (preuve de tâche sur le worktree neuf, avant tout changement) ; puis
construction, preuve de tâche : N = N_départ + 19 ; « Fonctions d'un test » sur `tst_core_hittest` → une ligne
`SKIP   : … Talon L0-S : rempli par L3-T3.` ; `premiers-pas` hors écran.

**Interdits.** Aucun contenu au-delà du marqueur (aucune déclaration, aucune classe, aucun `QML_ELEMENT`) ; aucun
fichier existant modifié hors des fichiers CMake de la table ; ne pas créer `ui/presentation.cmake` ni
`tests/presentation.cmake` (fichiers de la session B). Fichiers neufs : `git add` avant `controle_noms_actcraft.py`,
contrôle relancé après le commit (le script ne lit que `git ls-files`).

**Terminé quand.** Tous les fichiers de la table existent, compilent, sont enregistrés ; N = N_départ + 19 ; chaque
fichier porte le marqueur de sa tâche ; `TESTS OK` ; `NOMS OK` deux fois.

**Étapes.**
- [ ] **Étape 1 : relever N_départ.** Preuve de tâche sur le worktree neuf ; noter `out of N` au rapport.
- [ ] **Étape 2 : fichiers vides marqués.** Chaque en-tête : `#pragma once` puis `// Talon L0-S : rempli par <tâche>.`
  Chaque `.cpp` : `#include <actcraft/<module>/<nom>.h>` (ou `"<nom>.h"` pour `render/src/equationlayout.h`,
  `ui/src/tooladapters.h`) puis le marqueur. Tâches : `replace`, `equation` (core), `equationlayout` (render) →
  `L0-S3` ; `hittest`, `erasegeometry`, `objectedits`, `lift`, `routing`, `gesture`, `tool`, `toolcontext`,
  `userstate`, `toolregistry`, `erasertool`, `selecttool`, `selectiongeometry`, `liftlayeritem`, `toolhost`,
  `tooladapters` → `L0-S6`. QML : `import QtQuick`, marqueur en commentaire, `Item { }` — six QML du lot 3 → `L0-S6`,
  `Presentation.qml` → `L0-S9`.
- [ ] **Étape 3 : tests réservés.** Les 19 fichiers de la table, sur le modèle de P3:1537-1549 (classe et `.moc`
  adaptés ; `QTEST_MAIN` pour `render` et `ui`, `QTEST_GUILESS_MAIN` pour `core`, `input`, `tools`), texte du `QSKIP` :
  `tst_core_replace` → `L0-S3` ; `tst_ui_inputpath` → `L0-S7` ; les 13 du lot 3 → leur tâche du lot 3
  (`tst_core_hittest`, `tst_core_erase` L3-T3 ; `tst_core_objectedits`, `tst_core_lift` L3-T4 ; `tst_input_routing`,
  `tst_input_gesture` L3-T5 ; `tst_tools_registry` L3-T6 ; `tst_tools_eraser` L3-T7 ; `tst_tools_select` L3-T8 ;
  `tst_render_liftlayer` L3-T10 ; `tst_render_strokeshape` L3-T11 ; `tst_ui_lift` L3-T12 ; `tst_ui_tools` L3-T14).
  Scénarios `bench/scenarios/{selection,corbeille,marqueur,ecrire-gommer-annuler}.json` sur le modèle de P3:1552-1558,
  `"description": "Talon L0-S : rempli par L3-T15 (S34)."` (`corbeille` : L3-T16 (S35) ; `marqueur` : L3-T16 (S37) ;
  `ecrire-gommer-annuler` : L3-T18 (S6)).
- [ ] **Étape 4 : CMake.** Sources : `core/CMakeLists.txt` (`replace`, `equation`, `hittest`, `erasegeometry`,
  `objectedits`, `lift`, en-têtes compris si le fichier les liste), `input/CMakeLists.txt` (`routing`, `gesture`),
  `tools/CMakeLists.txt` (`userstate`, `toolregistry`, `erasertool`, `selecttool`, `selectiongeometry`),
  `render/CMakeLists.txt` (`include/actcraft/render/liftlayeritem.h`, `src/liftlayeritem.cpp`, `src/equationlayout.h`, `src/equationlayout.cpp` ; l'en-tête public est listé comme ceux des autres éléments, sinon AUTOMOC et `qt_extract_metatypes` ne verront pas la classe remplie par L0-S6 — relecture tour 2),
  `ui/CMakeLists.txt` (`include/actcraft/ui/toolhost.h`, `src/toolhost.cpp`, `src/tooladapters.h`,
  `src/tooladapters.cpp` ; les sept QML ; `SelectionOverlay.qml`, `ObjectBar.qml`, `TrashCan.qml`,
  `PageContextMenu.qml`, `TranslucencyPopup.qml`, `Presentation.qml` dans la liste `QT_QML_SKIP_CACHEGEN` comme
  `TopBar.qml`). Crochet du lot 7 dans `ui/CMakeLists.txt`, **avant** `qt_add_qml_module` :
  ```cmake
  # Lot 7 (présentation et outils) : ses sources QML et C++ sont listées dans ui/presentation.cmake (fichier de la
  # session B) ; ce fichier-ci n'est plus modifié pour elles. Presentation.qml est déclaré ici (socle) : ne pas le
  # relister dans presentation.cmake.
  set(ACTCRAFT_UI_LOT7_QML "")
  set(ACTCRAFT_UI_LOT7_SOURCES "")
  include(${CMAKE_CURRENT_SOURCE_DIR}/presentation.cmake OPTIONAL)
  ```
  puis `${ACTCRAFT_UI_LOT7_QML}` à la fin de `QML_FILES`, `${ACTCRAFT_UI_LOT7_SOURCES}` à la fin de `SOURCES`, et
  `ActCraft::features` ajouté à `target_link_libraries(actcraft_ui PUBLIC …)` (`features` dépend de `core`,
  `actions`, `render` : aucun cycle ; s'il en apparaît un, s'arrêter et le rapporter). Tests : `tests/core.cmake`
  (`tst_core_replace`, `tst_core_hittest`, `tst_core_erase`, `tst_core_objectedits`, `tst_core_lift`,
  `LIBS ActCraft::core`), `tests/input.cmake` (`tst_input_routing`, `tst_input_gesture`, `LIBS ActCraft::input`),
  `tests/tools.cmake` (`tst_tools_registry`, `tst_tools_eraser`, `tst_tools_select`, `LIBS ActCraft::tools`),
  `tests/render.cmake` (`tst_render_liftlayer`, `tst_render_strokeshape`, comme `tst_render_pendinglayer`),
  `tests/ui.cmake` (`tst_ui_lift`, `tst_ui_tools`, comme `tst_ui_document` ; `tst_ui_inputpath`, comme `tst_pageview`
  plus `actcraft_uiplugin`), `tests/bench.cmake` (`tst_captures_selection`, `tst_captures_corbeille`,
  `tst_captures_marqueur`, `tst_captures_ecrire_gommer_annuler`, comme `tst_captures_anticrenelage`). En **dernière
  ligne** de `tests/CMakeLists.txt`, après `include(${CMAKE_CURRENT_SOURCE_DIR}/app.cmake)` :
  ```cmake
  # Lot 7 : tests déclarés par la session B dans tests/presentation.cmake (absent tant que B n'est pas fusionnée).
  include(${CMAKE_CURRENT_SOURCE_DIR}/presentation.cmake OPTIONAL)
  ```
  (même ligne que celle de la session B : à la fusion, une seule reste.)
- [ ] **Étape 5 : preuves et commit.** `format.ps1`, construction, preuve de tâche (N = N_départ + 19), `premiers-pas`
  hors écran ; `git -C <wt> add` de chaque fichier de la table, nommé ; `NOMS OK` ; puis :
  ```powershell
  git -C <wt> diff --cached --stat
  git -C <wt> commit -m "Socle 3-7 : réservation des fichiers et tests du socle et du lot 3, crochets CMake du lot 7 (L0-S0)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```
  et `NOMS OK` relancé après le commit.

<a id="tache-l0-s1"></a>
### Tâche L0-S1 — Icônes des deux lots

**Objectif.** Dessiner, une fois, les 31 icônes des lots 3 et 7 (SVG à nous, grille de 24 px) et les déclarer dans le
module QML, pour que les commandes du socle (L0-S4) et les boutons de palette les trouvent, et qu'aucune tâche des
lots 3 et 7 ne touche plus `ui/CMakeLists.txt` pour une icône.

**Fichiers.** Créer `ui/icons/<nom>.svg` pour : `selection`, `marqueur`, `gomme`, `annuler`, `retablir`,
`supprimer`, `corbeille`, `effacer`, `deplacer`, `rotation`, `translucidite`, `agrandir`, `reduire` (lot 3) ;
`rideau`, `spot`, `zoom-page`, `effets`, `regle`, `equerre`, `rapporteur`, `compas`, `origine`, `des`,
`calculatrice`, `horloge`, `message`, `forme-reconnue`, `ecriture`, `camera`, `clavier`, `bureau` (lot 7). Modifier
`ui/CMakeLists.txt` (bloc `RESOURCES` de `qt_add_qml_module` **seulement**) et `tests/tst_ui_theme.cpp` (`kIcons`).

**Test et commande bornée.** Commande ciblée `tst_ui_theme` ; preuve de tâche, N = N_départ + 19 ; `iconsAreOurSvgOnA24Grid`
en `PASS` pour 41 lignes (`Totals` le montre).

**Interdits.** Aucune icône copiée ou retracée d'ActivInspire ni d'une bibliothèque tierce ; mêmes attributs que
`ui/icons/stylo.svg` ; aucun autre bloc de `ui/CMakeLists.txt`. Fichiers neufs : `git add` avant `controle_noms_actcraft.py`, contrôle relancé après le commit (le script ne lit que `git ls-files`).

**Terminé quand.** 31 fichiers, `viewBox="0 0 24 24"`, rendus valides, déclarés ; `TESTS OK` ; `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : base.** N_départ est relevé par L0-S0 (état du chantier) ; la base de cette tâche compte
  N_départ + 19.
- [ ] **Étape 2 : test rouge.** Dans `tests/tst_ui_theme.cpp`, ajouter les 31 noms à la fin de `kIcons`, dans l'ordre
  ci-dessus. Commande ciblée `tst_ui_theme` → rouge (`QFile::exists` faux).
- [ ] **Étape 3 : icônes.** Lire `ui/icons/stylo.svg` et reprendre ses attributs (`viewBox`, `fill`, `stroke`,
  `stroke-width`, `stroke-linecap`, `stroke-linejoin`). Formes simples à nous : flèche (sélection), surligneur biseauté,
  gomme, flèches courbes gauche et droite, croix, poubelle, balai, quatre flèches, flèche circulaire, damier à moitié,
  loupes + et − ; store à demi baissé (rideau), disque dans un carré (spot), loupe sur une page (zoom-page), deux pages
  décalées (effets), règle graduée, équerre, demi-disque gradué (rapporteur), compas, axes croisés (origine), dé à cinq
  points, calculatrice, cadran, bandeau à flèche (message), cercle et carré tracés (forme-reconnue), « a » manuscrit
  (écriture), appareil photo, clavier, écran à stylo (bureau). Fichiers en LF.
- [ ] **Étape 4 : déclaration.** Ajouter les 31 chemins `icons/<nom>.svg` à la fin du bloc `RESOURCES`, dans l'ordre
  ci-dessus. Commande ciblée → vert.
- [ ] **Étape 5 : preuves et commit.** `format.ps1`, preuve de tâche, `NOMS OK`, puis :
  ```powershell
  git -C <wt> add ui/icons ui/CMakeLists.txt tests/tst_ui_theme.cpp
  git -C <wt> diff --cached --stat
  git -C <wt> commit -m "Socle 3-7 : icônes des lots 3 et 7, dessinées par nous (L0-S1)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

<a id="tache-l0-s2"></a>
### Tâche L0-S2 — Joueur de scénarios : doigt, pincement, pointeur, double-clic, tolérance

**Objectif.** Le joueur ne sait injecter que stylet, souris par trait, touches et commandes. Ajouter, **après
`CheckEdge`** et dans cet ordre, `Touch` et `Pinch` (lot 3, S6) puis `Pointer` et `DoubleClick` (lot 7, S10), et une
tolérance numérique à `expect`. Absorbe L3-T17 et l'étape 8 de L7-T2.

**Fichiers.** Modifier `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`,
`bench/include/actcraft/bench/injector.h`, `bench/src/injector.cpp`, `tests/tst_bench_uiscenario.cpp`,
`tests/tst_bench_injector.cpp`.

**Contrat (formes JSON et lignes rapportées).**
- `{"touch": [[x, y], …]}` : un doigt, points de page convertis en fenêtre comme l'étape `stroke`, même intervalle ;
  ligne `DOIGT <n> points OK`.
- `{"pinch": [cx, cy], "from": 100, "to": 200, "steps": 10}` : deux doigts placés horizontalement autour du centre de
  page (converti en fenêtre), écart en pixels de vue de `from` à `to` en `steps` pas d'une image ; `steps` absent : 12 ;
  `from` ≤ 0 ou `to` ≤ 0 : `SCENARIO INVALIDE` ; ligne `PINCEMENT ×<to/from> OK`.
- `{"pointer": [[x, y], …], "button": "left"|"right", "holdMs": n, "space": "page"|"window"}` : souris, appui au premier
  point, mouvements, maintien `holdMs` au dernier point, relâché ; `button` par défaut `left`, inconnu →
  `SCENARIO INVALIDE` ; `holdMs` négatif refusé ; `space` par défaut `page` ; ligne `POINTEUR <n> OK`.
- `{"doubleClick": [x, y], "space": "page"|"window"}` ; ligne `DOUBLECLIC OK`.
- `{"expect": "objet.propriété", "value": 1.5, "tolerance": 0.01}` : `tolerance` facultative, numérique ≥ 0 ; sans
  elle, comparaison exacte comme aujourd'hui.

**Test et commande bornée.** Commande ciblée `tst_bench_uiscenario|tst_bench_injector` ; preuve de tâche, N = N_départ + 19 ;
preuve visuelle `premiers-pas` (aucune régression du joueur).

**Interdits.** Rangs existants de `UiStepKind` inchangés ; injection par l'API publique de Qt seulement (comme
l'injecteur existant ; un `QPointingDevice` tactile enregistré une seule fois) ; aucune souris réelle.

**Terminé quand.** Quatre étapes lues, jouées, rapportées ; tolérance lue ; tests en `PASS` ; `premiers-pas`
`CAPTURES OK`.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** `tests/tst_bench_uiscenario.cpp` :
  `touchStepParsed` (`{"touch": [[100, 100], [300, 100]]}` → `UiStepKind::Touch`, 2 points de page) ;
  `pinchStepParsed` (forme du contrat → `UiStepKind::Pinch`, centre, `from`, `to`, `steps` 10 ; sans `steps` : 12 ;
  `"from": 0` → erreur) ; `pointerStepParsed` (les deux `space`, `button` inconnu → erreur, `holdMs` -1 → erreur,
  `space` absent → page) ; `doubleClickStepParsed` ; `expectWithTolerance` (`value` 1.5, `tolerance` 0.01 : lu 1.505 →
  `LU … OK`, lu 1.6 → `ECART`) ; `stepKindRanksFrozen` :
  ```cpp
  void stepKindRanksFrozen()
  {
      using K = actcraft::bench::UiStepKind;
      QCOMPARE(static_cast<int>(K::CheckEdge) + 1, static_cast<int>(K::Touch));
      QCOMPARE(static_cast<int>(K::Touch) + 1, static_cast<int>(K::Pinch));
      QCOMPARE(static_cast<int>(K::Pinch) + 1, static_cast<int>(K::Pointer));
      QCOMPARE(static_cast<int>(K::Pointer) + 1, static_cast<int>(K::DoubleClick));
  }
  ```
  `tests/tst_bench_injector.cpp` : `pinchDeliversTwoTouchPoints` (élément d'essai qui compte les points des
  `QTouchEvent` reçus : appui à deux points, `steps` mouvements, relâché ; écart final = `to` ± 1 px, milieu fixe à
  ± 1 px) ; `pointerHoldsBeforeRelease` (élément d'essai qui horodate appui et relâché : écart ≥ `holdMs`). Commande
  ciblée → rouge (noms inconnus).
- [ ] **Étape 2 : implémentation.** Dans `uiscenario.h`, après `CheckEdge` :
  ```cpp
      // Ajoutées par le socle des lots 3 et 7 (L0-S2), à la fin, dans cet ordre.
      Touch,       // {"touch": [[x, y], …]} — un doigt, coordonnées de page
      Pinch,       // {"pinch": [cx, cy], "from": 100, "to": 200, "steps": 12} — écarts en pixels de vue
      Pointer,     // {"pointer": [[x, y], …], "button": "left", "holdMs": 0, "space": "page"}
      DoubleClick, // {"doubleClick": [x, y], "space": "page"}
  ```
  Champs de `UiStep` ajoutés à la fin de la structure (points, `from`, `to`, `steps`, bouton, maintien, espace,
  tolérance optionnelle `std::optional<double>`). Lecture dans `uiscenario.cpp` à côté des étapes existantes, même
  style d'erreur ; exécution par l'injecteur : `injector.h` gagne `touchPath(points)`, `pinch(center, from, to,
  steps)`, `pointerPath(points, Qt::MouseButton, holdMs)`, `doubleClick(point)` (noms libres s'ils suivent le style du
  fichier ; publics, documentés en une ligne). Conversion page → fenêtre par la même voie que `stroke`
  (`pageView.pageToScene`). Commande ciblée → vert.
- [ ] **Étape 3 : preuves et commit.** `format.ps1`, preuve de tâche, `premiers-pas` hors écran, puis :
  ```powershell
  git -C <wt> add bench/include/actcraft/bench/uiscenario.h bench/src/uiscenario.cpp bench/include/actcraft/bench/injector.h bench/src/injector.cpp tests/tst_bench_uiscenario.cpp tests/tst_bench_injector.cpp
  git -C <wt> commit -m "Socle 3-7 : étapes doigt, pincement, pointeur et double-clic du joueur, tolérance des lectures (L0-S2)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

<a id="tache-l0-s3"></a>
### Tâche L0-S3 — Modèle partagé : équation, réglages de présentation, remplacement unique

**Objectif.** Poser dans `core/`, `formats/` et `render/` ce que les deux lots supposent du modèle, pour que le code
du lot 3 (désignation, gomme, transformations, soulèvement) traite l'équation dès sa naissance et qu'une seule
commande de remplacement serve la gomme (lot 3) et la reconnaissance (lot 7), avec un contrat de `Change` asserté.

**Fichiers.** Voir la table. Remplir les fichiers réservés par L0-S0 : `core/include/actcraft/core/replace.h`, `core/src/replace.cpp`,
`core/include/actcraft/core/equation.h`, `core/src/equation.cpp`, `render/src/equationlayout.h`,
`render/src/equationlayout.cpp`, `tests/tst_core_replace.cpp` (supprimer son `reserved()`). Aucun fichier CMake de
liste (déjà posés) ; `cmake/ActCraftModule.cmake` pour `/we4062` seulement.

**Test et commande bornée.** Commande ciblée
`tst_core_replace|tst_core_model|tst_formats_nativejson|tst_render_painter`, puis preuve de tâche (N = N_départ + 19),
banc sans écran.

**Interdits.** Aucun rang existant changé (`Shape` … `Connector` gardent 0 à 7 ; `Change::Kind` existants gardent les
leurs). Empreinte d'une page **sans** réglages de présentation inchangée. Aucune interprétation du MathML (texte gardé
tel quel). Aucun `switch` avec `default:` ajouté pour faire taire `-Wswitch` : chaque `switch` reçoit son `case`.
`ReplaceStrokesCommand` n'est pas écrite. Fichiers neufs : `git add` avant `controle_noms_actcraft.py`, contrôle relancé après le commit (le script ne lit que `git ls-files`).

**Terminé quand.** Tests nommés verts ; mutation rouge puis remise ; un `.actcraft` avec une équation et des réglages
de présentation fait l'aller-retour à l'identique ; l'équation se dessine (repère neutre) ; `TESTS OK` ; `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : empreinte de référence (I1).** Avant tout changement, relever par la commande « Fonctions d'un
  test » (ou un `qInfo()` temporaire retiré ensuite) la valeur de `core::fingerprint` de la page générée du banc
  (`core::generatePage` avec `core::kDefaultSeed`) ; elle devient `kBenchPageFingerprint` dans `tst_core_model.cpp`.
- [ ] **Étape 2 : tests rouges du modèle.** Dans `tests/tst_core_model.cpp` :
  ```cpp
  void equationKindIsBeforeUnknown()
  {
      QCOMPARE(static_cast<int>(ObjectKind::Equation), 8);
      QCOMPARE(static_cast<int>(ObjectKind::Unknown), 9);
      QCOMPARE(kObjectKindCount, std::size_t{10});
      PageObject o{Equation{RectF{0, 0, 100, 40}, "<math><mn>1</mn></math>", 24, 0xFF000000u}};
      QCOMPARE(kindOf(o), ObjectKind::Equation);
  }
  void defaultPresentationIsDefault()
  {
      QVERIFY(isDefault(PagePresentation{}));
      PagePresentation p;
      p.curtain = CurtainMode::Top;
      QVERIFY(!isDefault(p));
  }
  void fingerprintUnchangedWithoutPresentation()
  {
      Page page = generatePage(kDefaultSeed); // nom et signature de core/generator.h, à aligner
      QCOMPARE(fingerprint(page), kBenchPageFingerprint); // relevée à l'étape 1, avant la tâche
      page.presentation.transition = "fade";
      QVERIFY(fingerprint(page) != kBenchPageFingerprint);
  }
  ```
  (Les deux `QCOMPARE` existants sur `kObjectKindCount` et `ObjectKind::Unknown` suivent ; un document témoin qui
  énumère un objet par type reçoit une `Equation`.)
- [ ] **Étape 3 : tests rouges du remplacement.** Remplir `tests/tst_core_replace.cpp` (réservé et déclaré par L0-S0 ;
  supprimer `reserved()`). Un cache espion :
  ```cpp
  struct SpyCache final : actcraft::core::PageCache
  {
      QStringList calls;
      void insert(ObjectId id) override { calls << QStringLiteral("insert %1").arg(id); }
      void remove(ObjectId id, const RectF&) override { calls << QStringLiteral("remove %1").arg(id); }
      void invalidate(ObjectId id, const CacheArea&) override { calls << QStringLiteral("invalidate %1").arg(id); }
  };
  ```
  (aligner sur les signatures exactes de `core/include/actcraft/core/pagecache.h`, sans les changer). Fonctions, sur
  une page de trois traits `a`, `b`, `c` (couche supérieure, ajoutés par `AddObjectCommand` dans cet ordre) :
  - `oneGroupMakesOneStep` : `ReplaceObjectsCommand(0, {b}, {forme sur la couche de b})` poussée ; 3 objets, la forme
    à l'indice 1 ; `undo` : `b` revient à l'indice 1 avec son identifiant.
  - `piecesTakeTheOriginalRank` : groupe `{b} → {b1, b2}` (morceaux de même couche) : ordre `a, b1, b2, c`.
  - `groupOfTwoTakesTheHigherRank` : page `a, s1, b, s2, c` ; groupe `{s1, s2} → {forme de même couche}` : ordre
    `a, b, forme, c` ; `undo` rend `a, s1, b, s2, c`.
  - `otherLayerGoesToTopOfLayer` : groupe `{b} → {objet d'une autre couche}` : l'ajout va au sommet de sa couche
    (`insertAtTopOfLayer`).
  - `absentIdIgnored` : identifiant inconnu dans `removed` ignoré ; groupe sans retrait présent et sans ajout : `Changes`
    vide (la commande n'entre pas dans la pile).
  - `addedIdsStableAcrossRedo` : `addedIds()` vide avant le premier `redo`, puis rempli ; `undo` puis `redo` : mêmes
    identifiants, mêmes objets.
  - `cacheSeesRemoveThenInsert` : deux groupes `{a} → {a1}` et `{c} → {c1, c2}` ; `core::applyToCache(changes, 0,
    spy)` au redo donne exactement `remove a`, `insert a1`, `remove c`, `insert c1`, `insert c2`.
  - `undoIsTheExactInverse` : à l'annulation, exactement `remove c2`, `remove c1`, `insert c`, `remove a1`,
    `insert a` ; chaque `ObjectRemoved` porte l'emprise (`boundsOnPage`) de l'objet retiré.
  - `presentationTouchesNoCache` (I3) : `SetPagePresentationCommand(0, p)` : `Changes` non vide (un seul
    `Change::Kind::PageSettingsChanged`), `applyToCache` → `spy.calls` vide ; `undo` idem.
  - `setPagePresentationMakesOneStep` : valeur posée, `undo` rend `isDefault`.
  Commande ciblée → rouge (compilation).
- [ ] **Étape 4 : modèle.** `core/include/actcraft/core/model.h` : `struct Equation` (P7a:206) avant `UnknownObject` ;
  `ObjectKind::Equation = 8`, `Unknown = 9`, `kObjectKindCount = 10` ; `Equation` dans `ObjectVariant` entre
  `Connector` et `UnknownObject` ; `PageToolsMode`, `CurtainMode`, `SpotlightShape`, `PagePresentation` (P7a:209-220) — un seul écart au texte copié : le champ des bornes du spot s'appelle `spotlightFrame` (nom de bf8c930), et `std::string unknownKeys` est ajouté en dernier (étape 7) (nom refusé par le contrôle des noms)
  avant `struct Page` ; `PagePresentation presentation;` dans `Page` après `notes` ; `bool isDefault(const
  PagePresentation&) noexcept;`. `core/src/model.cpp` : hachage d'`Equation` (bounds, mathml, pointSize, color),
  `isDefault`, et dans `fingerprint(const Page&)`, **en fin de flux, avant les clés inconnues**,
  `if (!isDefault(page.presentation)) { h.byte(0xA7); … champs … }`. `core/src/edits.cpp`, `boundsOnPage` :
  `Equation` → `bounds` transformé, comme `Text`. `core/include/actcraft/core/undo.h` : `PageSettingsChanged` ajouté
  **en dernier** à `Change::Kind`, commenté « réglages de la page (présentation) : aucun pixel du cache ne change » ;
  `core/src/pagecache.cpp` : `case Change::Kind::PageSettingsChanged: break;`.
- [ ] **Étape 5 : `replace.h`.** Créer tel quel :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>
  #include <actcraft/core/undo.h>

  #include <cstddef>
  #include <utility>
  #include <vector>

  namespace actcraft::core {

  // Un groupe : objets retirés et objets qui les remplacent (gomme : un trait → ses morceaux ; reconnaissance :
  // des traits → une forme ou un texte).
  struct ObjectReplacement
  {
      std::vector<ObjectId> removed;
      std::vector<PageObject> added;
  };

  // Remplacement annulable en une étape (socle des lots 3 et 7, seule commande de remplacement).
  // Placement, groupe après groupe sur la page courante : soit r le rang (dans page.objects, avant le retrait du
  // groupe) du plus haut de ses objets retirés présents, et k le nombre de ses retirés présents situés sous lui ; les
  // ajouts du groupe sont insérés, dans l'ordre, à partir du rang r - k (exemple : a, s1, b, s2, c avec
  // {s1, s2} → {f} donne a, b, f, c) quand ils sont de la même couche que ce plus haut retiré ; sinon (autre couche,
  // ou aucun retrait présent) ils vont au sommet de leur couche (insertAtTopOfLayer). Identifiants des ajouts tirés au premier redo() (allocateId) puis gardés ;
  // l'identifiant porté par un ajout est ignoré. Identifiant retiré absent de la page : ignoré.
  // Changes au redo, groupe par groupe dans l'ordre : un ObjectRemoved par objet retiré présent (zone : son
  // emprise, boundsOnPage), puis un ObjectInserted par ajout (zone : son emprise). À l'annulation, l'inverse exact :
  // groupes en ordre inverse ; pour chacun, un ObjectRemoved par ajout (ordre inverse), puis un ObjectInserted par
  // objet retiré, remis à son rang d'origine (rangs croissants). Le cache (DA11) traite une insertion hors fin de
  // page par un redessin immédiat (règle du lot 10) : rien de plus à faire ici.
  class ReplaceObjectsCommand final : public UndoCommand
  {
  public:
      ReplaceObjectsCommand(std::size_t page, std::vector<ObjectReplacement> replacements);
      ReplaceObjectsCommand(std::size_t page, std::vector<ObjectId> removed, std::vector<PageObject> added); // un groupe
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;
      const std::vector<ObjectId>& addedIds() const noexcept; // groupes puis ajouts, dans l'ordre ; vide avant redo

  private:
      struct Applied
      {
          std::vector<std::pair<std::size_t, PageObject>> removed; // rang d'origine, objet ; rangs croissants
          std::vector<ObjectId> added;
      };
      std::size_t m_page;
      std::vector<ObjectReplacement> m_groups;
      std::vector<Applied> m_applied;
      std::vector<ObjectId> m_addedIds;
  };

  // Réglages de présentation de la page (F-591 à F-593, F-640, F-284), en une étape. Changes : un seul
  // PageSettingsChanged (page `page`), que applyToCache ignore (aucun redessin, I3).
  class SetPagePresentationCommand final : public UndoCommand
  {
  public:
      SetPagePresentationCommand(std::size_t page, PagePresentation value);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_page;
      PagePresentation m_value;
      PagePresentation m_previous;
  };

  } // namespace actcraft::core
  ```
  `core/src/replace.cpp` : lire d'abord `core/src/edits.cpp` (`AddObjectCommand`, `indexOf`, `insertAtTopOfLayer`,
  `allocateId`) et en reprendre les outils ; `SetPagePresentationCommand::redo` sans effet si la valeur est déjà
  celle de la page (`Changes` vide). `core/include/actcraft/core/equation.h` et `core/src/equation.cpp` : fichiers
  réservés au lot 7, contenu :
  ```cpp
  #pragma once
  // Talon L0-S : rempli par L7-T24. Équation (F-301) : arbre et MathML.
  namespace actcraft::core {
  } // namespace actcraft::core
  ```
  (`equation.cpp` : `#include <actcraft/core/equation.h>`, même commentaire, même espace de noms vide). Fichiers déjà
  enregistrés par L0-S0. Commande ciblée `tst_core_replace|tst_core_model` → vert.
- [ ] **Étape 6 : mutation.** Dans `replace.cpp`, échanger l'ordre « retraits puis ajouts » du redo : la commande
  ciblée `tst_core_replace` passe au rouge (`cacheSeesRemoveThenInsert`) ; remettre par
  `git -C <wt> checkout -- core/src/replace.cpp` (rappel : une remise par `Copy-Item` garde l'ancienne date, ninja ne
  recompile pas ; sinon `LastWriteTime = Get-Date`) ; vert.
- [ ] **Étape 7 : format natif, tests rouges.** `tests/tst_formats_nativejson.cpp` : `equationRoundTripKeepsMathml`
  (MathML avec `<mfrac>` et l'entité `&#x3B1;` : écrire, relire, même empreinte, `mathml` identique octet pour octet,
  `"type": "equation"` dans le JSON) ; `presentationRoundTrip` (tous les champs non par défaut, relus égaux) ;
  `defaultPresentationNotWritten` (page par défaut : pas de clé `"presentation"`) ; `unknownPresentationKeyKept`
  (une sous-clé connue et une inconnue : l'inconnue réécrite telle quelle, règle L2-C8) ; `unknownOnlyPresentationKept`
  (seule une sous-clé inconnue : la clé `"presentation"` est encore écrite, avec elle). Puis `formats/src/nativejson.cpp` : nom `"equation"` dans la
  table des types (l'assertion de distinction suit) ; `writeBody`/`readBody` d'`Equation` sur le modèle de `Text`
  (`bounds`, `mathml`, `pointSize`, `color` en `#AARRGGBB`) ; clé de page `"presentation"` écrite si `!isDefault`,
  sous-clés `tools`, `curtain`, `curtainDistance`, `spotlight`, `spotlightFrame`, `origin` (absente si
  `!hasOrigin`), `transition` ; valeurs en texte : `"keep" "curtain" "spotlight" "off"` ; `"off" "top" "bottom" "left"
  "right" "full"` ; `"off" "circle" "square" "solid-circle" "solid-square"` ; sous-clés inconnues gardées par le
  mécanisme déjà en place pour les objets et la page (`withKept` / clés gardées) : `PagePresentation` gagne, en
  dernier champ, `std::string unknownKeys; // sous-clés inconnues de "presentation", JSON compact, réécrites telles
  quelles (L2-C8)` ; `isDefault` est faux si `unknownKeys` n'est pas vide ; l'empreinte l'inclut (dans le bloc
  `0xA7`). Vert.
- [ ] **Étape 8 : repère neutre.** `render/src/equationlayout.h` :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>

  class QPainter;

  namespace actcraft::render {
  // Dessin d'une équation (F-301). Socle : le repère neutre de paintUnknown sur equation.bounds. Rempli par L7-T25,
  // qui ne change que equationlayout.cpp.
  void paintEquation(QPainter& painter, const core::Equation& equation);
  } // namespace actcraft::render
  ```
  `render/src/equationlayout.cpp` : `// Talon L0-S : rempli par L7-T25.` puis le corps de `paintUnknown`
  (`render/src/pagepainter.cpp`, vers la ligne 563) appliqué à `equation.bounds` (fichiers déjà enregistrés par
  L0-S0). `pagepainter.cpp` (vers la ligne 646) : une `Equation` est dessinée par
  `paintEquation` ; `paintedrenderer.cpp`, `usesPaintedRenderer` (vers la ligne 231) : `Equation` → `true`. Test rouge
  puis vert dans `tests/tst_render_painter.cpp`, `equationDrawsTheNeutralFrame` : une page avec une équation de
  `bounds` (100, 100, 200, 80), peinte dans une `QImage` comme les autres fonctions du fichier : un pixel du bord du
  repère n'est pas la couleur du fond, un pixel hors de l'emprise l'est.
- [ ] **Étape 9 : `switch` signalés.** C4062 est désactivé par défaut sous MSVC (`-Wswitch`, par `-Wall`, joue sous
  tout compilateur non MSVC, en CI macOS et Linux) : dans `cmake/ActCraftModule.cmake`, ajouter `/we4062` aux options MSVC de
  `actcraft_set_warnings`. Construire tout (`build.ps1`) : chaque erreur C4062 / `-Wswitch` sur
  `ObjectKind` ou `Change::Kind` reçoit son `case` (équation traitée comme `Text` pour l'emprise, comme un objet peint
  pour le rendu ; `PageSettingsChanged` sans effet de cache). Lister les fichiers touchés au rapport.
- [ ] **Étape 10 : preuves et commit.** `format.ps1`, preuve de tâche (N = N_départ + 19), banc sans écran, `NOMS OK` :
  ```powershell
  git -C <wt> add core/include/actcraft/core/model.h core/src/model.cpp core/src/edits.cpp core/include/actcraft/core/undo.h core/src/pagecache.cpp core/include/actcraft/core/replace.h core/src/replace.cpp core/include/actcraft/core/equation.h core/src/equation.cpp formats/src/nativejson.cpp render/src/pagepainter.cpp render/src/paintedrenderer.cpp render/src/equationlayout.h render/src/equationlayout.cpp cmake/ActCraftModule.cmake tests/tst_core_replace.cpp tests/tst_core_model.cpp tests/tst_formats_nativejson.cpp tests/tst_render_painter.cpp
  git -C <wt> diff --cached --stat
  git -C <wt> commit -m "Socle 3-7 : équation, réglages de présentation de la page, remplacement unique au contrat de changements asserté (L0-S3)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```
  (ajouter à `git add` chaque fichier de l'étape 9, nommé.)

<a id="tache-l0-s4"></a>
### Tâche L0-S4 — Données et disposition des deux lots

**Objectif.** Une seule forme de sous-menu (celle du lot 3) avec le filtrage par plateforme du lot 7 ; toutes les
commandes des deux lots déclarées en blocs contigus ; leur place dans les menus, la palette, la barre d'objet et le menu
contextuel ; l'argument texte ; aucun raccourci en double. Après elle, ni le lot 3 ni le lot 7 ne touchent plus
`commands.json`, `layout.json`, `layout.*`, `commandregistry.*`, `commands.*`, `CommandMenu.qml`.

**Fichiers.** Voir la table.

**Test et commande bornée.** Commande ciblée `tst_actions_layout|tst_actions_registry|tst_ui_commands|tst_ui_shell` ;
preuve de tâche (N = N_départ + 19) ; `premiers-pas` hors écran `-Lire "pageIndicator.text=1 / 2"`.

**Interdits.** Aucune commande existante renommée ni déplacée hors des menus cités ; le format `actcraft-layout/1` reste
lisible tel quel (un fichier sans sous-menu se lit comme avant) ; aucun gestionnaire de commande ajouté ici ; libellés
à nous.

**Terminé quand.** Tests nommés verts ; `Commands::menus()` montre « Éditer ▸ Effacer », « Outils ▸ Spot, Caméra,
Outils de maths, Autres outils » sous Windows ; sous Android, « Caméra » disparaît ; `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : tests rouges de la disposition.** Copier dans `tests/tst_actions_layout.cpp` les fonctions
  `submenuItems`, `objectBarRead` et `lot3EntriesDeclared` de P3:524-568 **mot pour mot** ; ajouter :
  ```cpp
  void contextMenuRead()
  {
      const QByteArray json = R"({"format": "actcraft-layout/1", "menus": [], "palette": [], "colors": [],
          "penWidths": [], "contextMenu": ["tool.select", "-", "tool.pen"]})";
      actions::Layout layout;
      QString error;
      QVERIFY2(actions::parseLayout(json, layout, error), qPrintable(error));
      QCOMPARE(layout.contextMenu, (QStringList{QStringLiteral("tool.select"), QStringLiteral("-"),
                                                QStringLiteral("tool.pen")}));
  }
  void nestedSubmenuRefused()
  {
      const QByteArray json = R"({"format": "actcraft-layout/1", "menus": [{"id": "m", "label": "M", "items": [
          {"submenu": "a", "label": "A", "items": [{"submenu": "b", "label": "B", "items": []}]}]}],
          "palette": [], "colors": [], "penWidths": []})";
      actions::Layout layout;
      QString error;
      QVERIFY(!actions::parseLayout(json, layout, error));
      QVERIFY(error.contains(QStringLiteral("sous-menu")));
  }
  void lot7EntriesPlaced(); // disposition par défaut : sous-menus spot, camera, math, other-tools dans tools ;
                            // view.page-zoom et view.page-effects dans view ; insert.date-time et insert.equation
                            // dans insert ; groupe de palette presentation ; unknownCommands vide
  ```
  (corps de `lot7EntriesPlaced` : lire la disposition par défaut comme `lot3EntriesDeclared` et asserter chaque
  point.) Commande ciblée → rouge.
- [ ] **Étape 2 : sous-menus, barre d'objet, menu contextuel.** `layout.h` : `MenuSpec` de P3:574-580 (champ
  `std::vector<MenuSpec> submenus`) ; dans `Layout` : `QStringList objectBar;` et `QStringList contextMenu; // menu
  contextuel de la page (F-202), dans l'ordre ; "-" séparateur`. `layout.cpp` : règles de P3:582-587 (objet
  `{"submenu": id, "label", "items"}` → jeton `">" + id` et un `MenuSpec` ; id ou libellé vide, élément d'un autre
  type, sous-menu imbriqué : erreur « menu <n> : sous-menu invalide ») ; clés facultatives `objectBar` et `contextMenu`
  (tableaux de chaînes ; sinon « <clé> : tableau de chaînes attendu ») ; `unknownCommands` parcourt sous-menus,
  `objectBar` et `contextMenu` (le séparateur `-` n'est pas une commande). Vert.
- [ ] **Étape 3 : argument texte.** Test rouge `textArgument` dans `tests/tst_actions_registry.cpp` (P7a:641-650,
  mot pour mot) ; puis `Text` **en dernier** dans `ArgumentKind`, `"text"` dans le lecteur de
  `commandregistry.cpp`, `argumentIsValid` : `argument.canConvert<QString>() &&
  !argument.toString().trimmed().isEmpty()`. Vert.
- [ ] **Étape 4 : commandes.** Dans `actions/data/commands.json`, **en fin** de `commands`, deux blocs contigus :
  d'abord les 19 commandes du lot 3, P3:590-608 mot pour mot ; puis les 43 commandes du lot 7, P7a:658-700 mot pour mot.
  Tests rouges puis verts dans `tests/tst_actions_registry.cpp` : `lot7CommandsDeclared` (énoncé de P7a:702-705 : les
  43 déclarées, `tool.handwriting` absente d'Android et de macOS, `tool.camera-area` absente d'Android,
  `present.curtain` partout) et :
  ```cpp
  void noShortcutTwice()
  {
      actions::CommandRegistry registry;
      QString error;
      QVERIFY2(registry.loadDeclarations(actions::defaultCommandsJson(), error), qPrintable(error));
      QHash<QString, QString> seen; // raccourci normalisé → commande
      for (const QString& id : registry.ids()) { // nom de l'accesseur du registre, à aligner
          const QString shortcut = QKeySequence(registry.find(id)->shortcut).toString(QKeySequence::PortableText);
          if (shortcut.isEmpty()) {
              continue;
          }
          QVERIFY2(!seen.contains(shortcut), qPrintable(shortcut + QStringLiteral(" : ") + seen.value(shortcut) +
                                                         QStringLiteral(" et ") + id));
          seen.insert(shortcut, id);
      }
  }
  ```
  (`tests/tst_actions_registry.cpp` doit pouvoir lier `QKeySequence` : si le test n'a que QtCore, comparer les textes
  après `trimmed()` et mise en minuscules, et le dire au rapport.) Raccourcis écrits en QML (H-S5) :
  `git -C <wt> grep -n "Shortcut {" -- ui/qml` ; une séquence littérale trouvée hors de l'instanciateur des commandes
  est ajoutée au test comme liste fixe, et rapportée.
- [ ] **Étape 5 : disposition par défaut.** `actions/data/layout.json` :
  - `edit` : `["edit.undo", "edit.redo", "-", "edit.delete", "edit.select-all", "-", {"submenu": "clear", "label":
    "Effacer", "items": ["clear.annotations", "clear.objects", "clear.grid", "clear.background", "clear.page"]},
    "page.revert"]` ;
  - `view` : existant, puis `"-", "view.page-zoom", "view.page-effects", "-", "view.trash"` ;
  - `insert` : `["insert.date-time", "insert.equation"]` ;
  - `tools`, dans l'ordre de F-750 (`C:\Users\JeromeDenis\REA - Activinspire\docs\spec\fonctionnalites.md`,
    lecture seule ; H-S3 tranchée) : `["desktop.annotate", "desktop.tools", "-", "tool.pen", "tool.highlighter",
    "tool.eraser", "-", "tool.handwriting", "tool.shape-recognition", "-", "present.curtain", {"submenu": "spot",
    "label": "Spot", "items": ["present.spotlight-circle", "present.spotlight-square",
    "present.spotlight-solid-circle", "present.spotlight-solid-square"]}, {"submenu": "camera", "label": "Caméra",
    "items": ["tool.camera-area", "tool.camera-polygon", "tool.camera-freehand", "tool.camera-window",
    "tool.camera-screen"]}, {"submenu": "math", "label": "Outils de maths", "items": ["math.ruler", "math.protractor",
    "math.set-square", "math.compass", "math.xy-origin", "tool.dice", "tool.calculator"]}, {"submenu": "other-tools",
    "label": "Autres outils", "items": ["tool.keyboard", "tool.clock", "tool.ticker", "tool.web-browser"]}]` —
    `tool.select` n'est pas dans ce menu (F-750) : palette et menu contextuel. Les commandes d'autres lots citées par
    F-750 s'insèreront à leur rang ; test `toolsMenuFollowsF750` : la liste ci-dessus, dans cet ordre ;
  - palette : groupe `outils` → `["tool.select", "tool.pen", "tool.highlighter", "tool.eraser"]`, puis
    `{"group": "edition", "items": ["edit.undo", "edit.redo", "edit.delete"]}`, puis `{"group": "presentation",
    "items": ["present.curtain", "present.spotlight-circle", "view.page-zoom", "math.ruler", "tool.clock"]}` ;
  - `"objectBar": ["object.move-free", "object.rotate", "object.translucency", "object.enlarge", "object.reduce"]` ;
  - `"contextMenu": ["tool.select", "tool.pen", "-", "recognition.convert-to-text"]` (F-202 ; « Convertir en texte »
    sous Windows seulement, par sa plateforme).
  Vert : `lot3EntriesDeclared`, `lot7EntriesPlaced`.
- [ ] **Étape 6 : interface.** Tests rouges dans `tests/tst_ui_commands.cpp` : `submenuInMenus` (P3:1562-1584 mot pour
  mot) ; `submenuFilteredByPlatform` (sous Android, le sous-menu `camera` est omis du menu `tools` ; sous Windows,
  présent comme `{submenu: true, id: "camera", label: "Caméra", items: [5 éléments]}` ; utiliser le moyen du fichier
  pour choisir la plateforme) ; `objectBarAndContextMenu` (`objectBar()` : 5 éléments `{id, label, icon}` ;
  `contextMenu()` sous Windows : 4 éléments dont un séparateur `{separator: true}` ; sous Android : 2 éléments, sans
  séparateur final). Puis `ui/include/actcraft/ui/commands.h` :
  `Q_PROPERTY(QVariantList objectBar READ objectBar NOTIFY layoutChanged)` et
  `Q_PROPERTY(QVariantList contextMenu READ contextMenu NOTIFY layoutChanged)` ; `ui/src/commands.cpp` : `menus()` rend
  pour un jeton `>id` un élément `{submenu: true, id, label, items}` filtré comme un menu (vide : omis ; séparateurs
  nettoyés comme au premier niveau) ; `objectBar()` et `contextMenu()` filtrés par plateforme (séparateurs en tête, en
  queue ou doublés retirés). `ui/qml/CommandMenu.qml` : un élément `submenu` crée un `Menu` enfant par `Instantiator`
  (jamais de boucle `removeItem`, piège L1-T12), mêmes délégués que le premier niveau. Vert ; `tst_ui_shell` vert sans
  modification.
- [ ] **Étape 7 : preuves et commit.** `format.ps1`, preuve de tâche, `premiers-pas` hors écran, `NOMS OK` :
  ```powershell
  git -C <wt> add actions/data/commands.json actions/data/layout.json actions/include/actcraft/actions/layout.h actions/src/layout.cpp actions/include/actcraft/actions/commandregistry.h actions/src/commandregistry.cpp tests/tst_actions_layout.cpp tests/tst_actions_registry.cpp ui/include/actcraft/ui/commands.h ui/src/commands.cpp ui/qml/CommandMenu.qml tests/tst_ui_commands.cpp
  git -C <wt> commit -m "Socle 3-7 : commandes et disposition des lots 3 et 7, sous-menus filtrés par plateforme, barre d'objet, menu contextuel (L0-S4)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

<a id="tache-l0-s5"></a>
### Tâche L0-S5 — `DocumentController` : API des deux lots, observateur unique

**Objectif.** Figer dans `documentcontroller.*` l'API que le lot 3 remplira (soulever, déposer, effacer, rétablir la
page, supprimer, annotation translucide), **sans** que le soulèvement émette `editAboutToApply` (B1 de la relecture du
lot 3), et remplacer les deux crochets prévus (`setCacheObserver` du lot 3, `setStrokeObserver` du lot 7) par **un**
observateur des modifications versées, réel dès le socle.

**Fichiers.** `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`,
`tests/tst_ui_document.cpp`.

**Test et commande bornée.** Commande ciblée `tst_ui_document|tst_ui_drawing|tst_ui_shell` ; preuve de tâche (N =
N_départ + 19, base L0-S3 mergée) ; banc sans écran.

**Interdits.** Aucun comportement existant changé (Annuler, Rétablir, pages, ouverture, enregistrement) ; les corps
réservés au lot 3 ne font rien ; l'observateur reçoit **exactement** les `Changes` passés à `core::applyToCache` ;
aucun appel au cache hors de `settle`.

**Terminé quand.** Tests nommés verts ; mutation rouge puis remise ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** Dans `tests/tst_ui_document.cpp` (reprendre son montage du contrôleur et de la page
  fixe) :
  - `observerGetsTheCacheChanges` : un observateur applique ce qu'il reçoit à un cache espion par
    `core::applyToCache(changes, controller.currentIndex(), spy)` ; un trait versé par `commitStroke` → `insert <id>` ;
    `undo` → `remove <id>`.
  - `observerSeesTheOrigin` : `commitStroke` → `ChangeOrigin::Stroke` ; `execute(AddObjectCommand)` → `Command` ;
    `undo` → `Undo` ; `redo` → `Redo`.
  - `removedObserverIsSilent` : après `removeChangeObserver(jeton)`, plus aucun appel ; jeton > 0.
  - `replaceInTheMiddleIsNotDeferred` : page fixe en différé (`setDeferredFolding(true)` comme les tests du lot 10),
    trois traits versés puis versement (`foldPending`) ; `execute(ReplaceObjectsCommand(0, {milieu}, {forme de même
    couche}))` → `fixedPage->pendingCount() == 0` et au moins un `contentChanged` (deux attendus : retrait hors attente
    puis insertion hors fin de page, chacun un `restart()`) ; `undo` → `pendingCount() == 0` (règle du
    lot 10 : une insertion hors fin de page redessine tout de suite).
  - `presentationChangeRedrawsNothing` : `execute(SetPagePresentationCommand)` : espion de `contentChanged` de la page
    fixe à 0, cache espion de l'observateur vide, `documentChanged` émis, `canUndo` vrai.
  - `shownObjectCountCounts` : 0 sur un document neuf, 2 après deux traits.
  - `liftDoesNotAnnounceAnEdit` : un espion de `editAboutToApply` ; `liftObjects({})` → 0 émission (contrat B1 ; le
    corps est un talon, l'assertion reste vraie quand L3-T12 le remplit).
  Commande ciblée → rouge.
- [ ] **Étape 2 : en-tête.** Copier dans `documentcontroller.h` le bloc P3:1472-1498 (propriétés, membres publics,
  signaux) **sauf** la ligne `setCacheObserver` et son commentaire ; remplacer le commentaire de `liftObjects` par :
  ```cpp
  // Soulever (DA11, lot 3) : retire les objets de la page affichée hors pile et rend leurs copies ; le cache reçoit
  // core::liftChanges. N'émet PAS editAboutToApply (B1 : l'outil qui soulève ne doit pas s'annuler lui-même).
  // Pendant un soulèvement, Enregistrer, Ouvrir et les changements de page sont refusés ou attendent le dépôt.
  ```
  Ajouter, en public :
  ```cpp
  // Origine d'une modification versée (observateur unique, socle des lots 3 et 7).
  enum class ChangeOrigin : std::uint8_t
  {
      Command, // execute()
      Undo,
      Redo,
      Stroke, // commitStroke / commitAnnotation : trait versé par un outil d'encre
      Lift,   // liftObjects (lot 3)
      Drop,   // dropLifted (lot 3)
  };
  // Reçoit, après chaque modification de la page affichée, exactement les changements passés au cache
  // (core::applyToCache) et leur origine. Appelé sur le fil principal, après la page fixe.
  using ChangeObserver = std::function<void(const core::Changes& applied, ChangeOrigin origin)>;
  int addChangeObserver(ChangeObserver observer); // jeton > 0
  void removeChangeObserver(int token);
  ```
  (`#include <functional>`, `#include <vector>`, `#include <actcraft/core/model.h>` si absents.)
- [ ] **Étape 3 : corps.** `settle(const core::Changes&, ChangeOrigin origin)` (privé) appelle, juste après
  `core::applyToCache`, chaque observateur dans l'ordre d'ajout ; `execute` → `Command`, `undo` → `Undo`, `redo` →
  `Redo` ; `commitStroke` → `Stroke` (par un membre privé d'origine posé le temps de l'appel, ou une surcharge privée
  d'`execute`). `shownObjectCount` réel (objets de la page affichée). Corps réservés, chacun précédé de
  `// Talon L0-S : rempli par L3-T12.` : `liftObjects` (rend `{}`), `dropLifted` (ne fait rien de la commande),
  `isLifting` (faux), `commitAnnotation` (ne fait rien), `deleteObjects`, `clearPage`, `revertPage` (rien),
  `canRevertPage` (faux). Vert.
- [ ] **Étape 4 : mutation.** Dans `commitStroke`, passer `ChangeOrigin::Command` au lieu de `Stroke` : rouge
  (`observerSeesTheOrigin`) ; remise par `git -C <wt> checkout -- ui/src/documentcontroller.cpp` (une remise par
  `Copy-Item` garde l'ancienne date, ninja ne recompile pas) ; vert.
- [ ] **Étape 5 : preuves et commit.** `format.ps1`, preuve de tâche, banc sans écran :
  ```powershell
  git -C <wt> add ui/include/actcraft/ui/documentcontroller.h ui/src/documentcontroller.cpp tests/tst_ui_document.cpp
  git -C <wt> commit -m "Socle 3-7 : API du contrôleur de document des lots 3 et 7, observateur unique des modifications versées (L0-S5)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

<a id="tache-l0-s6"></a>
### Tâche L0-S6 — Squelette du lot 3

**Objectif.** Remplir les fichiers réservés par L0-S0 pour le lot 3 : en-têtes publics figés (texte du plan du lot 3),
talons `.cpp`, QML à `required property` (M5) ; exposer `LiftLayer` au module QML de `ui` ; rendre réels
`UserState::openMathTools` et `ToolHost::user()` (D-S10). Aucun fichier CMake.

**Fichiers.** Voir la table.

**Test et commande bornée.** Commande ciblée `tst_core_|tst_input_|tst_tools_|tst_render_|tst_ui_` ; preuve de tâche
(N = N_départ + 19) ; `premiers-pas` hors écran ; banc sans écran.

**Interdits.** Aucun comportement d'outil (talons) ; en-têtes copiés mot pour mot, aux seuls écarts listés ; ne pas
toucher `pageview.*`, `documentcontroller.*`, `Main.qml`, `PageArea.qml`, `ToolPalette.qml`, ni un fichier CMake ; ne
renommer aucun objet QML existant. Fichiers neufs : `git add` avant `controle_noms_actcraft.py`, contrôle relancé après
le commit (le script ne lit que `git ls-files`).

**Terminé quand.** Tous les en-têtes du lot 3 sont ceux de P3, aux écarts près ; chaque talon porte le marqueur de sa
tâche du lot 3 ; `LiftLayer` est un type QML de `ActCraft.Ui` ; N = N_départ + 19 ; `TESTS OK` ; `NOMS OK` deux fois.

**Étapes.**
- [ ] **Étape 1 : en-têtes figés.** Remplacer le contenu des en-têtes réservés par les blocs de P3:624-1468
  (`hittest.h`, `erasegeometry.h`, `objectedits.h`, `lift.h`, `routing.h`, `gesture.h`, `tool.h`, `toolcontext.h`,
  `userstate.h`, `toolregistry.h`, `erasertool.h`, `selecttool.h`, `selectiongeometry.h`, `liftlayeritem.h`,
  `toolhost.h`, `tooladapters.h`), mot pour mot, avec ces écarts **seulement** :
  1. `objectedits.h` : retirer `StrokeReplacement` et `ReplaceStrokesCommand` (P3:727-749) ; à leur place :
     `// Gomme (F-230) : core::ReplaceObjectsCommand (replace.h), un groupe par trait touché, morceaux au rang de
     l'original.`
  2. `erasertool.h` : « dépose une ReplaceStrokesCommand » → « dépose une core::ReplaceObjectsCommand ».
  3. `routing.h` : `#include <functional>` ; `Route` gagne, en dernier,
     `Cover, // une surcouche déclarée couvre le point d'appui : la vue ne prend rien (socle, D-S1)` ; dans
     `PointerRouter`, en public :
     ```cpp
     // Consulté à l'appui, avant la politique : vrai → Route::Cover rendue, sans effet sur RouterState.
     void setCoverTest(std::function<bool(const Sample&)> test);
     // Contact terminé ou non pris par la vue (deuxième doigt, annulation, appui couvert) : sa décision est oubliée.
     void release(std::uint64_t pointer);
     ```
     et en privé `std::function<bool(const Sample&)> m_coverTest;`. Phrase ajoutée au commentaire de `route` :
     « À l'appui, une décision retenue pour le même pointeur est remplacée. »
  4. `toolhost.h` : `#include <actcraft/tools/userstate.h>` ; en public, après `activeTool` :
     `tools::UserState& user(input::UserId id = input::kFirstUser); // un utilisateur jusqu'au lot 11 (D-S10)` ; en
     privé `std::unique_ptr<tools::UserState> m_user;`.
- [ ] **Étape 2 : talons `.cpp`.** Chaque `.cpp` réservé (`core/src/{hittest,erasegeometry,objectedits,lift}.cpp`,
  `input/src/{routing,gesture}.cpp`, `tools/src/{userstate,toolregistry,erasertool,selecttool,selectiongeometry}.cpp`,
  `render/src/liftlayeritem.cpp`, `ui/src/{toolhost,tooladapters}.cpp`) inclut son en-tête et définit chaque fonction
  déclarée par le corps le plus simple qui compile (valeur par défaut, `{}`, `nullptr`, conteneur vide) ; son marqueur
  `L0-S6` devient `// Talon L0-S : rempli par L3-T<n>.` (tâche de la carte P3:234-265). Exceptions **réelles** :
  `UserState::UserState`, `id()`, `openMathTools()`, `setOpenMathTools()` ; `ToolHost::user()` (crée l'unique
  `UserState` au premier appel) ; `ToolHost::activeTool()` rend `nullptr` (talon, L3-T14) ; `PointerRouter` rend
  `Decision{}` pour tout (marqueur `rempli par L0-S7`).
- [ ] **Étape 3 : `LiftLayer` au QML (relecture du socle, important 11).** Dans `ui/src/uiforeigntypes.h`, sur le modèle
  des types déjà exposés (`FixedPage`, `PendingLayer`) :
  ```cpp
  struct LiftLayerForeign
  {
      Q_GADGET
      QML_FOREIGN(actcraft::render::LiftLayerItem)
      QML_NAMED_ELEMENT(LiftLayer)
  };
  ```
  (reprendre exactement la forme du fichier : `#include <actcraft/render/liftlayeritem.h>`, même espace de noms).
- [ ] **Étape 4 : QML talons (M5).** Chaque fichier : `import QtQuick` puis `import ActCraft.Ui`, marqueur
  `// Talon L0-S : rempli par L3-T15.` (ou `L3-T16`), puis :
  ```qml
  // SelectionOverlay.qml (L3-T15)
  Item {
      required property Tools tools
      required property PageView view
      function openTranslucency() {}
  }
  // ObjectBar.qml (L3-T15)
  Item {
      required property Tools tools
      required property Commands commands
  }
  // TranslucencyPopup.qml (L3-T15)
  Item {
      required property Tools tools
  }
  // TrashCan.qml (L3-T16)
  Item {
      required property Tools tools
      required property Commands commands
  }
  // PageContextMenu.qml (L3-T16) : lit commands.contextMenu (données, L0-S4)
  Item {
      required property Commands commands
      function popupAt(x, y) {}
  }
  // ToolCursor.qml (L3-T16)
  Item {
      required property Tools tools
      property int cursorShape: Qt.ArrowCursor
  }
  ```
  Vérification H-S2 (5 min) : dans `tst_ui_tools.cpp`, une fonction `requiredPropertyAcceptsNull` charge par
  `QQmlComponent::setData` `import QtQuick; Item { required property Item t }` avec la propriété initiale `t` à
  `null` (`createWithInitialProperties`) : objet créé, aucune erreur. Si Qt refuse, rendre ces propriétés ordinaires
  (`property Tools tools: null`) dans les talons, sans changer leur nom, et le rapporter.
- [ ] **Étape 5 : fonctions réelles des tests talons** (à côté de `reserved()`) :
  ```cpp
  // tests/tst_tools_registry.cpp
  void openMathToolsKept()
  {
      actcraft::tools::UserState user;
      QCOMPARE(user.id(), actcraft::input::kFirstUser);
      QVERIFY(user.openMathTools().empty());
      user.setOpenMathTools({QStringLiteral("ruler"), QStringLiteral("compass")});
      QCOMPARE(user.openMathTools().size(), std::size_t{2});
      QCOMPARE(user.openMathTools()[1], QStringLiteral("compass"));
  }
  // tests/tst_ui_tools.cpp
  void oneUserForNow()
  {
      actcraft::ui::ToolHost host;
      actcraft::tools::UserState& a = host.user();
      actcraft::tools::UserState& b = host.user(actcraft::input::kFirstUser);
      QCOMPARE(&a, &b);
      QVERIFY(host.activeTool() == nullptr); // talon jusqu'à L3-T14
  }
  void liftLayerTypeIsRegistered()
  {
      QQmlEngine engine;
      QQmlComponent component(&engine);
      component.setData("import QtQuick\nimport ActCraft.Ui\nLiftLayer {}\n", QUrl());
      QScopedPointer<QObject> object(component.create());
      QVERIFY2(object, qPrintable(component.errorString()));
  }
  ```
- [ ] **Étape 6 : preuves.** Construire ; commande ciblée ; preuve de tâche : N = N_départ + 19 ; « Fonctions d'un
  test » sur `tst_core_hittest` → `SKIP … Talon L0-S : rempli par L3-T3.` ; sur `tst_tools_registry` →
  `PASS   : …openMathToolsKept()` ; `premiers-pas` hors écran ; banc sans écran ; `git add` de chaque fichier de la
  table, nommé ; `NOMS OK`.
- [ ] **Étape 7 : commit.**
  ```powershell
  git -C <wt> diff --cached --stat
  git -C <wt> commit -m "Socle 3-7 : squelette du lot 3 (en-têtes figés, talons, LiftLayer exposé, utilisateur unique) (L0-S6)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```
  puis `NOMS OK` relancé après le commit.

<a id="tache-l0-s7"></a>
### Tâche L0-S7 — Chemin d'entrée figé de la vue

**Objectif.** Faire passer toute entrée de `PageView` par le chemin figé de D-S1 — événement → `Sample` →
`PointerRouter` (couvertures) → filtre d'échantillons (voie `Tool`) → `Pipeline` → outil actif — sans changer ce que
voit l'utilisateur ; corriger B2 (le stylet traverse les surcouches) pour les couvertures déclarées ; poser la garde
du versement différé sur l'outil actif (D-S9) ; menu contextuel par signal ; `zoomAround` unique (M7).

**Fichiers.** Voir la table.

**Contrat.**
```cpp
// pageview.h — en tête
#include <actcraft/input/routing.h>
#include <functional>
namespace actcraft::tools { class Tool; }
namespace actcraft::ui { class ToolHost; class ToolDispatch; }
// dans la classe (toolhost.h inclut pageview.h : inclusion circulaire évitée)
Q_MOC_INCLUDE(<actcraft/ui/toolhost.h>)
Q_PROPERTY(actcraft::ui::ToolHost* tools READ tools WRITE setTools NOTIFY toolsChanged)
Q_PROPERTY(int cacheRefreshCount READ cacheRefreshCount NOTIFY cacheUpdated)
Q_PROPERTY(bool gestureActive READ gestureActive NOTIFY gestureActiveChanged)
// public
using SampleFilter = std::function<input::Sample(const input::Sample&)>;
ToolHost* tools() const noexcept;
// Posé : les échantillons routés vers l'outil vont à ToolHost::activeTool(utilisateur du contact) ; sinon, ou s'il
// rend nullptr, au stylo interne (lots 1 et 2, banc).
void setTools(ToolHost* tools);
tools::Tool* currentTool() const;       // outil qui reçoit la voie Tool maintenant
int cacheRefreshCount() const noexcept; // redessins du cache terminés depuis la création (crochet S6, H10-1)
bool gestureActive() const noexcept;    // un geste à deux doigts est en cours (L3-T13 ; faux au socle)
input::PointerRouter& router() noexcept; // tests
// Surcouche (lot 7) qui couvre la page là où elle est visible : à l'appui sous elle, ni stylet, ni doigt, ni souris
// ne vont à l'outil (Route::Cover) ; l'événement continue vers ses gestionnaires. Oubliée à sa destruction.
Q_INVOKABLE void addInputCover(QQuickItem* cover);
Q_INVOKABLE void removeInputCover(QQuickItem* cover);
// Guides de tracé (lot 7) : appliqué à chaque échantillon de la voie Tool, avant le Pipeline, relâchés synthétisés
// compris (à partir du dernier point brut). Vide : aucun. Fil principal.
void setSampleFilter(SampleFilter filter);
// Zoom ancré : le point de page sous viewPoint ne bouge pas ; quitte « Ajuster » ; cache comme la molette
// (redessin kCacheRefreshDelayMs après le dernier appel). Pincement (L3-T13) et zoom de page (L7-T5).
Q_INVOKABLE void zoomAround(QPointF viewPoint, qreal zoom);
Q_INVOKABLE QPointF sceneToPage(QPointF scenePoint) const; // inverse de pageToScene
// signaux
void toolsChanged();
void gestureActiveChanged();
void contextMenuRequested(QPointF viewPos); // clic droit non couvert sur la vue (F-202)
```

**Test et commande bornée.** Commande ciblée `tst_ui_inputpath|tst_input_routing|tst_pageview|tst_ui_drawing|tst_pentool|tst_input` ;
preuve de tâche (N = N_départ + 19) ; banc sans écran ; preuve visuelle `cache-differe` (scénario du lot 10) hors écran
`CAPTURES OK` avec `-Lire "fixedPage.pendingCount=0"` en fin.

**Interdits.** Aucun changement visible : stylo, doigt, souris, molette, barres de défilement, différé et cache se
comportent comme au lot 10 (`tst_pageview`, `tst_ui_drawing`, `tst_ui_shell`, `tst_pentool` passent **sans
modification**) ; pas de `FullTouchPolicy` à gestes (L3-T5) ; pas de `QTabletEvent` construit par API privée
(`QCoreApplication::sendEvent` sur la fenêtre avec le constructeur public de `QTabletEvent`) ; `AA_CompressHighFrequencyEvents`
et `AA_CompressTabletEvents` jamais ensemble.

**Terminé quand.** Tests nommés verts ; mutation rouge puis remise ; banc et `cache-differe` sans régression ;
`TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : routeur, tests rouges.** `tests/tst_input_routing.cpp` (garder `reserved()` du talon, pour L3-T5) :
  - `coverRouteHeldForWholeContact` : `setCoverTest` vrai pour x < 100 ; appui stylet en (50, 50) → `Route::Cover` ;
    mouvement en (500, 50) → `Cover` ; relâché → `Cover`, puis oublié (mouvement suivant : `Ignore`).
  - `coverLeavesTheStateAlone` (bloquant 1 de la relecture) : appui stylet couvert, **jamais relâché** auprès du
    routeur ; puis appui d'un doigt non couvert → `Tool` (un `Cover` ne pose pas `stylusDown`).
  - `pressRedecides` : appui couvert, puis (sans relâché vu) nouvel appui du même pointeur en (500, 50) → `Tool`.
  - `uncoveredGoesToTheTool` : stylet, souris, doigt non couverts → `Tool` (politique minimale du socle).
  - `releaseForgetsAContact` : doigt (pointeur 3) appuyé, `release(3)`, mouvement du pointeur 3 → `Ignore`.
- [ ] **Étape 2 : routeur, code.** `input/include/actcraft/input/sample.h` : à la fin de `Sample`,
  `std::uint64_t pointer = 0; // stylet et souris : 0 ; doigt : identifiant du point + 1` et
  `std::uint8_t user = 0; // utilisateur du contact (DA-A1), rempli par le routeur`. `input/src/qtevents.cpp` : doigt →
  `pointer = static_cast<std::uint64_t>(point.id()) + 1`. `input/src/routing.cpp` (remplace le talon ;
  `// Talon L0-S : rempli par L3-T5.` reste devant `FullTouchPolicy::decide`) : `PointerRouter` retient la décision par
  `sample.pointer` (appui : remplace ; mouvement d'un inconnu : `Ignore` ; relâché : rend puis oublie) ; à l'appui,
  `m_coverTest` d'abord — vrai : `Decision{Route::Cover}` rendue, **sans toucher `RouterState`** ;
  `RouterState::stylusDown` tenu pour les seules décisions `Tool` ; `FullTouchPolicy::decide` minimal : stylet et souris →
  `Tool` ; doigt → `Ignore` si `state.stylusDown`, sinon `Tool` ; `user = kFirstUser`. Vert.
- [ ] **Étape 3 : outil, tests rouges puis code.** `tests/tst_pentool.cpp` gagne `penIsATool` : `PenTool` vu comme
  `tools::Tool&` : `id() == "pen"`, `isBusy()` vrai entre appui et relâché, `cancel()` pendant un trait verse le trait
  (le puits reçoit un trait) et rend `isBusy()` faux. `pentool.h` : `#include <actcraft/tools/tool.h>`,
  `class PenTool final : public Tool`, `QString id() const override;`, `bool isBusy() const noexcept override;`,
  `void cancel() override;` ; `pentool.cpp` : `cancel()` = `if (m_drawing) finish();`. Vert.
- [ ] **Étape 4 : vue, tests rouges.** Remplacer le talon `tests/tst_ui_inputpath.cpp` (supprimer `reserved()`). Montage
  : celui de `tests/tst_pageview.cpp` (fenêtre hors écran, `PageView` avec contenu, page fixe, encre, couche d'attente,
  contrôleur). Les traits se comptent par `fixedPage->objectCount()` (L0-S5 est dans la même vague). Surcouche
  d'essai : un élément QML créé par `QQmlComponent::setData`, enfant du contenu, (0, 0, 200, 200) en coordonnées de
  page, portant un `PointHandler` (`acceptedDevices: PointerDevice.AllDevices`) et une propriété `int presses`
  incrémentée à chaque activation ; `addInputCover(cover)`. Fonctions :
  - `coverStopsTablet` : `QTabletEvent` appui / mouvements / relâché envoyés à la fenêtre par
    `QCoreApplication::sendEvent` en (100, 100) → (150, 150) de page : aucun trait versé (`objectCount()`
    inchangé), l'appui n'est pas accepté par le filtre.
  - `coverStopsFinger` : `QTest::touchEvent` (périphérique de `QTest::createTouchDevice()`) même trajet : aucun trait.
  - `coverStopsMouse` : `QTest::mousePress/mouseMove/mouseRelease` même trajet : aucun trait.
  - `coverHandlerGetsThePress` : après chacun des trois trajets, `presses` a augmenté de 1 (le gestionnaire de la
    surcouche reçoit l'appui : un filtre qui avalerait l'événement serait rouge).
  - `coveredStylusTapLeavesFingersFree` : appui et relâché du stylet sous la couverture, puis un doigt hors d'elle :
    un trait versé.
  - `uncoveredStillDraws` : `removeInputCover(cover)` puis les trois trajets : trois traits versés ; une couverture
    détruite (`delete cover`) ne couvre plus.
  - `filterReachesThePen` : filtre `x + 50` ; trait de (300, 300) à (400, 300) : premier point versé (350, 300) ;
    filtre vide : (300, 300).
  - `syntheticReleaseIsFiltered` : filtre `x + 50` ; stylet appuyé en (300, 300), mouvement en (400, 300), puis
    `finishStroke()` : le trait versé finit en (450, 300) ; même chose pour un doigt fini par un deuxième doigt
    (`finishTouchStroke`).
  - `foldWaitsForTheBusyTool` : différé actif, un trait versé (en attente) ; stylet appuyé pour un second trait et tenu
    immobile `kCacheFoldIdleMs + 200` ms (`QTest::qWait`) : aucun `cacheRefreshRequested`, `pendingCount() == 1` ;
    relâché, puis attente `kCacheFoldIdleMs + 200` ms : un `cacheRefreshRequested`, `pendingCount() == 0`.
  - `filteredStrokeStaysPendingUntilIdle` : filtre posé, un trait versé : `pendingCount() == 1` tout de suite ;
    versé seulement après l'inactivité.
  - `rightClickRequestsTheContextMenu` : clic droit souris en (300, 300) : un `contextMenuRequested` à la position de
    vue ; aucun trait.
  - `coverTakesTheRightClick` : clic droit sous la couverture : aucun `contextMenuRequested`.
  - `zoomAroundKeepsThePointStill` : `zoomAround(QPointF(300, 200), 2.0)` : `sceneToPage(mapToScene(300, 200))`
    inchangé à ± 0,5 ; `zoom() == 2.0` ; un `cacheRefreshRequested` après `kCacheRefreshDelayMs` + marge, aucun avant.
  - `cacheRefreshCountCounts` : deux redessins terminés (`notifyCacheUpdated`) → 2.
  Commande ciblée → rouge.
- [ ] **Étape 5 : vue, code.** `pageview.h` : contrat ci-dessus, membres privés `input::PointerRouter m_router;`,
  `QList<QPointer<QQuickItem>> m_covers;`, `SampleFilter m_filter;`, `QPointer<ToolHost> m_tools;`,
  `std::unique_ptr<ToolDispatch> m_dispatch;`, `int m_cacheRefreshCount = 0;`, `QPointF m_lastRawPagePoint;`,
  `QPointF m_lastTouchPagePoint;`. `pageview.cpp` :
  1. `ToolDispatch final : public input::SampleSink` (dans le `.cpp`) transmet à `view.currentTool()` ; `rebuildPen`
     construit le `Pipeline` sur `*m_dispatch` (et non plus sur `*m_pen`), sonde inchangée.
  2. `currentTool()` : `m_tools && m_tools->activeTool() ? m_tools->activeTool() : m_pen.get()`.
  3. Constructeur : `setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton)` ;
     `m_router.setCoverTest([this](const input::Sample& s) { return coveredAt(m_transform.pageToView(QPointF(s.x,
     s.y))); });`.
  4. `coveredAt` : boucle des sœurs inchangée, puis, pour chaque couverture vivante : `isVisible() && isEnabled() &&
     cover->contains(cover->mapFromScene(scenePos))` → vrai.
  5. `handlePointer` : échantillon → `m_router.route(sample)` ; `Route::Tool` seulement : `m_lastRawPagePoint =
     pagePos`, `pushToTool(sample)` (filtre s'il est posé, puis `m_pipeline->push`) et rend vrai ; sinon rend faux.
     `eventFilter` du stylet : la condition `!coveredAt(viewPos)` disparaît (le routeur la porte) ; le reste est
     inchangé.
  6. `mousePressEvent` : bouton droit → **test de couverture direct** `coveredAt(event->position())`, sans routeur
     (`sampleFromEvent` ne rend rien pour le bouton droit ; `qtevents.cpp` n'est pas touché pour lui) ; couvert :
     `ignore()` ; sinon `emit contextMenuRequested(event->position())`, accepté, rien au `Pipeline`, et la suite du
     contact droit est ignorée.
  6 bis. Contact non pris (`Cover`, `Ignore`) : la vue appelle aussitôt `m_router.release(sample.pointer)` (aucune
     entrée ne s'accumule par contact couvert). Dans `eventFilter`, à l'appui du stylet, `finishTouchStroke()` n'est
     appelé que si le point n'est pas couvert (`!coveredAt(viewPos)`) : un appui couvert ne termine pas le trait d'un
     doigt.
  7. Minuterie de versement : `if (auto* tool = currentTool(); tool && tool->isBusy()) { m_foldTimer.start(); return; }`.
  8. `finishStroke` : `tool = currentTool()` ; rien si `!m_pipeline || !tool || !tool->isBusy()` ; relâché synthétisé
     au dernier point **brut** (`m_lastRawPagePoint`) par `pushToTool` ; puis, si `tool->isBusy()`, `tool->cancel()`.
     `finishTouchStroke` : idem au dernier point brut du doigt, puis `m_router.release(pointeur du doigt)`. Au socle,
     `activeTool()` rend `nullptr` : ces deux gardes ne voient que le stylo interne ; la branche « outil de
     `ToolHost` » est prouvée par le lot 3 (`foldWaitsForTheActiveTool`, L3-T13 ; `finishStrokeCancelsTheActiveTool`,
     L3-T14).
  9. `notifyCacheUpdated` : `++m_cacheRefreshCount` quand le redessin est fini (pas à la relance).
  10. `zoomAround`, `sceneToPage`, `setTools` (`toolsChanged`), `setSampleFilter`, `addInputCover` (connexion à
      `destroyed` qui la retire), `removeInputCover`, `router()`.
  `ui/src/toolhost.cpp` : `setView` ne fait rien de plus (talon L3-T14) — la vue lit `activeTool()` à chaque
  échantillon. Vert ; `tst_pageview`, `tst_ui_drawing`, `tst_pentool`, `tst_input` verts sans modification.
- [ ] **Étape 6 : mutation.** Dans le constructeur, ne pas poser `setCoverTest` : rouge (`coverStopsTablet`,
  `coverStopsMouse`) ; remise par `git -C <wt> checkout -- ui/src/pageview.cpp` (une remise par `Copy-Item` garde
  l'ancienne date, ninja ne recompile pas ; sinon `LastWriteTime = Get-Date`) ; vert.
- [ ] **Étape 7 : preuves et commit.** `format.ps1`, preuve de tâche, banc sans écran, `cache-differe` hors écran
  (`-Lire "fixedPage.pendingCount=0"`), `NOMS OK` :
  ```powershell
  git -C <wt> add input/include/actcraft/input/sample.h input/src/qtevents.cpp input/src/routing.cpp tools/include/actcraft/tools/pentool.h tools/src/pentool.cpp ui/include/actcraft/ui/pageview.h ui/src/pageview.cpp ui/src/toolhost.cpp tests/tst_ui_inputpath.cpp tests/tst_input_routing.cpp tests/tst_pentool.cpp
  git -C <wt> commit -m "Socle 3-7 : chemin d'entrée figé (routeur, couvertures, filtre, Pipeline, outil actif), garde du différé sur l'outil, menu contextuel par signal, zoom ancré (L0-S7)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

<a id="tache-l0-s9"></a>
### Tâche L0-S9 — Coquille QML

**Objectif.** Figer une fois l'ordre des couches et les emplacements des deux lots dans `PageArea.qml` et `Main.qml`,
brancher `appTools`, la racine `Presentation`, le menu contextuel par le signal de la vue, et les gestionnaires des
commandes du lot 3 (absorbe L3-T2). Après elle, le lot 7 ne touche plus `Main.qml`, `PageArea.qml`, `ToolPalette.qml`.

**Fichiers.** `ui/qml/Main.qml`, `ui/qml/PageArea.qml`, `ui/qml/ToolPalette.qml`, `ui/qml/Presentation.qml` (réservé
vide par L0-S0, déjà enregistré dans `ui/CMakeLists.txt`), `tests/tst_ui_shell.cpp`.

**Test et commande bornée.** Commande ciblée `tst_ui_shell|tst_ui_drawing|tst_captures_premiers_pas` ; preuve de
tâche (N = N_départ + 19) ; preuves visuelles hors écran `premiers-pas` (`-Lire "pageIndicator.text=1 / 2"`),
`cache-differe` et `pauses` (`-Lire "fixedPage.pendingCount=0"`) ; banc sans écran.

**Interdits.** Aucun `objectName` existant renommé ; aucun comportement dans les talons ; gestionnaires existants
(`tool.pen`, `pen.color`, `pen.width`) inchangés ; `PageArea` doit fonctionner avec `tools: null` (annotation de bureau
du lot 7, B3) ; aucun `TapHandler` de clic droit (le signal de la vue le remplace).

**Terminé quand.** Ordre et emplacements vérifiés par test ; gestionnaires du lot 3 posés ; S2 et S13 sans
régression ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** Dans `tests/tst_ui_shell.cpp` (aide de chargement du fichier) :
  ```cpp
  void socleLayerOrder()
  {
      QQuickWindow* window = loadMainWindow(); // nom de l'aide du fichier
      QVERIFY(window);
      auto* page = window->findChild<QQuickItem*>(QStringLiteral("page"));
      QVERIFY(page);
      QStringList names;
      for (QQuickItem* child : page->childItems()) {
          names << child->objectName();
      }
      QCOMPARE(names, (QStringList{"fixedPage", "pageCache", "pendingLayer", "liftLayer", "ink", "pageOverlays"}));
      for (QQuickItem* child : page->childItems()) {
          QCOMPARE(child->z(), 0.0); // l'ordre des enfants fait l'empilement
      }
  }
  void socleSlotsExist()
  {
      QQuickWindow* window = loadMainWindow();
      for (const char* name : {"appTools", "selectionOverlay", "toolCursor", "presentationSlot", "pageContextMenu",
                               "trashCan", "presentationLayer", "presentation"}) {
          QVERIFY2(window->findChild<QObject*>(QString::fromLatin1(name)) != nullptr, name);
      }
      auto* view = window->findChild<QQuickItem*>(QStringLiteral("pageView"));
      auto* selection = window->findChild<QQuickItem*>(QStringLiteral("selectionOverlay"));
      auto* slot = window->findChild<QQuickItem*>(QStringLiteral("presentationSlot"));
      QVERIFY(view && selection && slot);
      const QList<QQuickItem*> siblings = view->parentItem()->childItems();
      QVERIFY(siblings.indexOf(view) < siblings.indexOf(selection));
      QVERIFY(siblings.indexOf(selection) < siblings.indexOf(slot));
  }
  ```
  `everyLot3CommandHasAHandler` (P3:1641-1653 mot pour mot) ; `pageAreaWithoutToolsDraws` (un `PageArea` chargé seul
  par `QQmlComponent`, `tools` à `null` : un trait injecté au stylet est versé) ; `shellLoadsWithoutQmlWarning`
  (`QTest::failOnWarning(QRegularExpression(QStringLiteral("qrc:.*\\.qml")))` avant le chargement de `Main.qml` :
  aucun avertissement QML) ; `contextMenuOpensFromTheView` : émettre
  `contextMenuRequested(QPointF(10, 10))` sur la vue → `pageContextMenu.popupAt` appelée (espion : propriété
  `property int popupCount` ajoutée **au test**, par un QML d'essai, ou lecture d'un drapeau du talon si le test ne peut
  pas l'espionner autrement — dire au rapport le moyen pris). Commande ciblée → rouge.
- [ ] **Étape 2 : `PageArea.qml`.** Propriétés `property Tools tools: null`, `property alias liftLayer: liftLayerItem`,
  `property alias pageOverlays: pageOverlaysItem`, `property alias presentationSlot: presentationSlotItem`,
  `property alias selectionOverlay: selectionOverlayItem`, `property Commands commands: null`. `view.tools: area.tools`.
  Dans `pageContent`, entre `pendingLayerItem` et `inkLayer` :
  ```qml
  // Lot 3 : objets soulevés pendant un geste, au-dessus de la couche d'attente, sous l'encre.
  LiftLayer {
      id: liftLayerItem
      objectName: "liftLayer"
      anchors.fill: parent
      fixedPage: fixedPageItem
  }
  ```
  et après `inkLayer` :
  ```qml
  // Lot 7 : surcouches en coordonnées de page (outils de maths, origine XY, spot, rideau de la page), au-dessus de
  // l'encre et de la couche d'attente ; jamais dans la scène que le cache photographie. Une surcouche qui prend
  // l'entrée se déclare couverture (view.addInputCover).
  Item {
      id: pageOverlaysItem
      objectName: "pageOverlays"
      anchors.fill: parent
  }
  ```
  Après `view`, avant `HoverHandler` :
  ```qml
  SelectionOverlay {
      id: selectionOverlayItem
      objectName: "selectionOverlay"
      anchors.fill: parent
      tools: area.tools
      view: view
      visible: area.tools !== null
  }
  ToolCursor {
      id: toolCursor
      objectName: "toolCursor"
      anchors.fill: parent
      tools: area.tools
  }
  // Lot 7 : surcouches en coordonnées de la zone (au-dessus de la sélection, sous les barres de défilement).
  Item {
      id: presentationSlotItem
      objectName: "presentationSlot"
      anchors.fill: parent
  }
  ```
  `HoverHandler` : `cursorShape: toolCursor.cursorShape`. Après les barres de défilement :
  `PageContextMenu { id: pageContextMenu; objectName: "pageContextMenu"; commands: area.commands }` et, dans `view`,
  `onContextMenuRequested: (viewPos) => pageContextMenu.popupAt(viewPos.x, viewPos.y)`. Si `required property` refuse
  une valeur `null` (`tools` absent), donner aux talons concernés la valeur par l'instance comme ci-dessus — la
  propriété requise est posée, même à `null`.
- [ ] **Étape 2 bis : `Presentation.qml`** (talon minimal, remplacer le contenu réservé ; marqueur retiré : fichier du
  lot 7 que la session B complète) :
  ```qml
  import QtQuick
  import ActCraft.Ui

  // Racine des fonctions du lot 7 : chaque fonction pose ici son contrôleur, sa surcouche et ses gestionnaires de
  // commandes (commands.setHandler). Une surcouche qui prend l'entrée se déclare couverture :
  // Component.onCompleted: pageArea.view.addInputCover(surcouche)
  Item {
      id: presentation
      property Commands commands: null
      property DocumentController documentController: null
      property Tools tools: null
      property Item pageArea: null
      property Item pageOverlays: null
      property Item layer: null
  }
  ```
- [ ] **Étape 3 : `Main.qml`.** Après `appDocument` : bloc `Tools { id: appTools … }`, `Connections` et
  `syncEditCommands()` de P3:1659-1677 mot pour mot ; `PageArea` reçoit `tools: appTools` et `commands: appCommands` ;
  gestionnaires de P3:1681-1699 à la suite des existants, sauf `object.translucency` :
  `appCommands.setHandler("object.translucency", function (value) { if (value === undefined)
  pageArea.selectionOverlay.openTranslucency(); else appTools.setSelectionTranslucency(value) })`. À la fin de
  l'`ApplicationWindow`, après `messageBanner`, le bloc de P7a:845-858 avec en plus `tools: appTools` et
  `pageOverlays: pageArea.pageOverlays` dans `Presentation`.
- [ ] **Étape 4 : `ToolPalette.qml`.** `required property Tools tools` (passée par `Main.qml`) ; sous le `Flickable`,
  ancrée en bas : `TrashCan { id: trash; objectName: "trashCan"; tools: paletteRoot.tools; commands:
  paletteRoot.commands; visible: paletteRoot.commands.revision >= 0 && paletteRoot.commands.isChecked("view.trash") }`
  (nom de la racine : celui du fichier), hauteur réservée en bas du `Flickable`.
- [ ] **Étape 5 : vert et preuves.** Commande ciblée → vert ; `format.ps1` ; preuve de tâche ; `premiers-pas`,
  `cache-differe`, `pauses` hors écran ; banc sans écran ; `NOMS OK`.
- [ ] **Étape 6 : commit.**
  ```powershell
  git -C <wt> add ui/qml/Main.qml ui/qml/PageArea.qml ui/qml/ToolPalette.qml ui/qml/Presentation.qml tests/tst_ui_shell.cpp
  git -C <wt> commit -m "Socle 3-7 : coquille QML (ordre des couches figé, emplacements des lots 3 et 7, menu contextuel par la vue, gestionnaires du lot 3) (L0-S9)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

<a id="tache-l0-s10"></a>
### Tâche L0-S10 — Contrôle du socle

**Objectif.** Prouver le socle entier sur `feat/livraison-complete` (toutes les tâches mergées) : suites debug et
release, talons comptés et nommés, scénarios, banc, graphe à jour.

**Fichiers.** `graphify-out/` seulement.

**Test et commande bornée.**
```powershell
pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire L0-S10 -Commande 'pwsh -NoProfile -File ./scripts/test.ps1 -Preset release > .tmp/test-release.txt 2>&1'; Select-String -Path .tmp/test-release.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2"
git -C <wt> grep -n "Talon L0-S : rempli par L0-" -- . ':(exclude)docs/'
git -C <wt> grep -c "Talon L0-S : rempli par L3-" -- . ':(exclude)docs/'
git -C <wt> grep -c "Talon L0-S : rempli par L7-" -- . ':(exclude)docs/'
git -C <wt> grep -n "QSKIP(" -- tests | Select-String -NotMatch "Talon L0-S : rempli par L[37]-T[0-9]+\."
```
puis la preuve de tâche (debug), les captures hors écran `-Scenario tous`, le banc sans écran.

**Interdits.** Aucun code modifié : un écart se rapporte à l'orchestrateur (tâche corrective du socle), il ne se
corrige pas ici.

**Terminé quand.** Debug et release `TESTS OK`, N = N_départ + 19 ; la première commande `grep` ne sort **rien**
(plus aucun talon du socle) ; les comptes des deux suivantes relevés (repère : lot 3 de l'ordre de 35 lignes ; lot 7
de l'ordre de 3 — `equation.h`, `equation.cpp`, `equationlayout.cpp`) : le compte exact devient la référence des
clôtures ; la dernière ne sort que les `QSKIP` antérieurs au socle (listés au rapport) ; `CAPTURES OK` sur tous les
scénarios ; banc joint ; `NOMS OK` ; `graphify update .` commité.

**Étapes.**
- [ ] **Étape 1 : suites.** Release puis debug (preuve de tâche) ; noter N.
- [ ] **Étape 2 : talons.** Les commandes `grep` ; noter les comptes dans le rapport (l'orchestrateur les recopie dans
  l'état : références des clôtures des lots 3 et 7).
- [ ] **Étape 3 : scénarios et banc.** Captures hors écran `-Scenario tous` ; banc sans écran.
- [ ] **Étape 4 : graphe et commit.**
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; graphify update ."
  git -C <wt> add graphify-out
  git -C <wt> commit -m "Socle 3-7 : graphe du code à jour (L0-S10)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

## Amendements à porter une fois le socle mergé (entrée des planificateurs des lots 3 et 7)

Règles communes aux deux : base = `feat/livraison-complete` après L0-S10 ; noms du socle repris tels quels (section
« Contrats figés ») ; chaque tâche qui remplit un talon retire son marqueur et `reserved()` ; clôture de chaque lot :
contrôle de la règle 4 des talons pour son préfixe (`-- . ':(exclude)docs/'`, `-functions`, ligne `TRAIT`/`DOIGT`/
`POINTEUR` par scénario rempli) ; tout `switch` sur `ObjectKind` traite `Equation` (comme `Text` pour l'emprise et la
désignation) et tout `switch` sur `Change::Kind` traite `PageSettingsChanged` (sans effet de cache) — C4062 est une
erreur (`/we4062`, L0-S3).

### Plan du lot 3 (61c03fb)

**Disparaissent.**
1. **L3-T1** (réservation I, P3:470-) : fait par L0-S0, L0-S1, L0-S4, L0-S5, L0-S6, L0-S7. Les dépendances « L3-T1 »
   tombent ; L3-T0 seule reste en tête.
2. **L3-T2** (coquille QML, P3:1603-) : fait par L0-S9.
3. **L3-T17** (joueur : doigt, pincement, tolérance, P3:2893-) : fait par L0-S2. L3-T18 ne dépend plus de L3-T17.

**Rétrécissent ou changent de fichiers.**
4. **L3-T4** (P3:1972-) : `ReplaceStrokesCommand` n'existe pas ; la gomme dépose `core::ReplaceObjectsCommand` (un
   `ObjectReplacement` par trait touché, `addedIds()` au lieu de `pieceIds()`) ; `replaceStrokesKeepsRankAndIds` sort
   de sa preuve (couvert par `tst_core_replace`). Fichiers inchangés ; `objectedits.h` sans `StrokeReplacement`.
5. **L3-T5** (P3:2264-) : `sample.h` sort de ses fichiers (champs `pointer`, `user` posés) ; le mécanisme de
   `PointerRouter` (décisions retenues, `Route::Cover` sans effet sur `RouterState`, `setCoverTest`, `release`) est
   posé : L3-T5 remplit `FullTouchPolicy::decide` (geste à deux doigts, paume, M4) et `gesture.cpp` ; la règle
   « Si le socle L0-S crée lui-même `PointerRouter` » (P3:2469) s'applique ; les fonctions du socle de
   `tst_input_routing` restent vertes.
6. **L3-T6** (P3:2488-) : `tool.h` existe ; `PenTool` est déjà un `tools::Tool` (`id`, `isBusy`, `cancel`) ;
   `UserState::openMathTools` et `id()` réels — L3-T6 remplit le reste, sans toucher `openMathToolsKept`.
7. **L3-T7** (P3:2934-) : contrat `ReplaceObjectsCommand` (déjà prévu par sa note « Socle »).
8. **L3-T11** (P3:2643-) : `ui/qml/PageArea.qml` sort de ses fichiers si son seul besoin était l'ordre des couches
   (figé) ; sinon il garde le fichier pour une propriété d'`InkLayer`, aucune couche déplacée.
9. **L3-T12** (P3:2707-) : `addChangeObserver` (origines `Lift`, `Drop`) au lieu de `setCacheObserver` ;
   `liftObjects` n'émet pas `editAboutToApply` (B1 ; `liftDoesNotAnnounceAnEdit` du socle) ; en-tête figé : corps
   seulement.
10. **L3-T13** (P3:79, P3:3040-) : chemin d'entrée déjà posé (routeur, couvertures, filtre, `Pipeline` devant l'outil,
    garde du différé sur `isBusy`, `finishStroke` par `cancel`, `contextMenuRequested`, `tools`, `cacheRefreshCount`,
    `router()`). L3-T13 garde **`foldWaitsForTheActiveTool`**, `latencyProbeSeesToolSamples`,
    `pageChangeEndsToolStroke` (la branche « outil de `ToolHost` » de D-S9 n'est prouvée qu'ici) ; retirer
    `cacheRefreshCountCounts` de sa preuve (déjà dans `tst_ui_inputpath`).
11. **L3-T14** (P3:3168-) : `ToolHost::user()` réel ; `activeTool()` à remplir ; ajouter le test
    **`finishStrokeCancelsTheActiveTool`** (un outil actif occupé reçoit `cancel()` quand `finishStroke` ne l'a pas
    terminé) ; `Main.qml` reste dans ses fichiers (le lot 7 n'y touche plus) ; Échap (M9) par `cancelGesture()`.
12. **L3-T15** (P3:3249-) : **piège** — `SelectionOverlay` est une sœur posée au-dessus de la vue, `anchors.fill`. Un
    gestionnaire d'appui (`TapHandler`, `DragHandler`, `PointHandler`, `MouseArea`) sur sa **racine** ferait que
    `coveredAt` (sœurs de la vue) la voie partout : le stylet ne tracerait plus nulle part. Les gestionnaires vont sur
    les poignées et la barre seulement (éléments à la taille du cadre) ; test : avec une sélection affichée, un trait
    au stylet hors du cadre est versé.
13. **L3-T15, L3-T16** (P3:3249-, 3352-) : talons QML à `required property` déjà branchés par la coquille ;
    `PageContextMenu.qml` lit `commands.contextMenu` (« Convertir en texte » y est déjà, B3) ; `TrashCan` déjà
    instancié dans `ToolPalette.qml` ; icônes déjà dessinées (L0-S1) : `ui/icons/*` sort des fichiers.
14. **L3-T19** : gommer le dernier trait encore en attente en deux morceaux donne un redessin complet (le premier
    morceau n'est pas en fin de page) : à écrire dans les attendus de `gommer`.
15. **L3-C0** : si D11 donne `Ctrl+Shift+Z` à Rétablir, conflit avec `view.page-zoom` : `noShortcutTwice` le refuse ;
    L3-C0 tranche par F-750 et F-950 à F-953 et le rapporte.
16. **Lignes 137-147 (« Dépendances aux autres lots », lot 7)** à réécrire : surcouches du lot 7 = couvertures, routes
    de la politique (`Route::Cover`), pas des `tools::Tool` ; guides par le filtre d'échantillons ; outils de maths
    ouverts dans `tools::UserState::openMathTools` via `ToolHost::user()` ; « Convertir en texte » en données du menu
    contextuel ; reconnaissance par `ReplaceObjectsCommand` ; plus aucun fichier chaud commun.
17. **Table, vagues, compte** : 20 → 17 tâches ; « N ≥ 111 » → **N = N_départ + 19** ; vagues recalculées.

### Plans du lot 7 (bf8c930) — phase 2 de la session B

**Ce que la session B doit savoir (à la fusion L7-T33 et pour la suite).**
- Noms figés : listes `ACTCRAFT_UI_LOT7_QML` et `ACTCRAFT_UI_LOT7_SOURCES`, lues par `ui/CMakeLists.txt` dans
  `ui/presentation.cmake` (`include(… OPTIONAL)`, avant `qt_add_qml_module`). `ui/presentation.cmake` ne fait que
  remplir ces deux listes (et ses `set_source_files_properties`) : ni `qt_target_qml_sources`, ni `qt_add_qml_module`.
- `ui/qml/Presentation.qml` est **créé et enregistré par le socle** (L0-S0, rempli par L0-S9, propriétés `commands`,
  `documentController`, `tools`, `pageArea`, `pageOverlays`, `layer`) : **ne pas le relister** dans
  `ACTCRAFT_UI_LOT7_QML` (doublon de module QML) ; B le complète.
- `tests/CMakeLists.txt` se termine par `include(${CMAKE_CURRENT_SOURCE_DIR}/presentation.cmake OPTIONAL)` : la ligne
  de B, identique, se fusionne en une seule.
- Interdits à B (posés par le socle) : `core/`, `formats/`, `render/`, `actions/data/*`, `ui/icons/*` (dessinées par
  L0-S1), `ui/CMakeLists.txt`, `tests/{core,input,tools,render,ui,bench}.cmake`, `Main.qml`, `PageArea.qml`,
  `ToolPalette.qml`, `pageview.*`, `documentcontroller.*`. B ne recrée pas `PagePresentation`, `CurtainMode`,
  `SpotlightShape`, `PageToolsMode`, `Equation`, `ReplaceObjectsCommand`, `SetPagePresentationCommand`.
- Attentes : `tst_core_equation` et `tst_render_equation` (talons `equation.*`, `equationlayout.cpp`) attendent L0-S3 ;
  les scénarios à `pointer` ou `doubleClick` attendent L0-S2 ; `DocumentInsertions` attend `addChangeObserver`
  (L0-S5) ; tout ce qui appelle `addInputCover`, `setSampleFilter`, `zoomAround`, `sceneToPage` attend L0-S7 ;
  `pageOverlays`, `presentationLayer` et l'instance `presentation` attendent L0-S9.
- Compte : N = N_départ + 19 (socle) + n_B (tests de `tests/presentation.cmake`).

**Disparaissent.**
1. **L7-T2, étape 8** (`pointer`, `doubleClick`) : fait par L0-S2.
2. **L7-T1, sauf l'objet MathML des `.flipchart`** : commandes, `ArgumentKind::Text`, modèle, `replace.*`,
   `SetPagePresentationCommand`, format natif, repère neutre, fichiers réservés d'équation : faits par L0-S3, L0-S4.
   L7-T1 se réduit à `formats/src/flipchart.cpp`, `formats/src/flipchartids.h`, `tests/support/flipchartwriter.h`,
   `tests/tst_formats_flipchart.cpp` (`mathmlObjectBecomesEquation`) ; `tst_core_presentation` disparaît.
3. **L7-T2, étapes 1 à 3 et 5 à 7** : sous-menus, disposition, `pageview.*`, `documentcontroller.*`, `PageArea.qml`,
   `Main.qml`, `ui/CMakeLists.txt` : faits par le socle. L7-T2 garde : `FloatingPanel.qml`, l'API d'insertion dans un
   fichier du lot 7 (D-S8), `ui/presentation.cmake` (les deux listes), et dans `tests/tst_ui_presentation.cpp` :
   `overlaySlotsExist`, insertion de texte et remplacement en une étape, traits suivis par `addChangeObserver`
   (origine `Stroke`). Filtre et zoom ancré sont prouvés par `tst_ui_inputpath` (socle).

**Changent.**
4. Sortent de toutes les tâches : `ui/CMakeLists.txt`, `ui/icons/<nom>.svg` (une tâche peut redessiner **son** icône,
   sans la déclarer ailleurs), `ui/qml/Presentation.qml` comme fichier à créer (il existe).
5. Toute surcouche ou tout panneau qui prend l'entrée : `pageArea.view.addInputCover(<item>)` à sa création (déjà au
   plan, P7a:38), et un test par surcouche et panneau qui injecte un `QTabletEvent`, un doigt et la souris sous elle :
   aucun trait versé, **et son gestionnaire reçoit l'appui**.
6. **L7-T5** : `zoomAround` et `sceneToPage` existent ; l'appui long droit est pris par la couverture du zoom de page
   (test de couverture direct au clic droit) ; `pageZoomRefreshesTheCacheOnceAtRelease` : exactement 1.
7. **L7-T6, L7-T13** : `SetPagePresentationCommand` existe, son `Change` est `PageSettingsChanged` ; les sous-clés
   inconnues de `"presentation"` sont gardées (`PagePresentation::unknownKeys`, ajouté au contrat P7a:209-220).
8. **L7-T9 à L7-T13** : guides par `PageView::setSampleFilter` (relâchés synthétisés filtrés depuis le dernier point
   brut) ; outils de maths ouverts rangés par `ToolHost::user().setOpenMathTools` (le contrôleur reçoit `ToolHost*`
   de `Presentation.tools`).
9. **L7-T30** : `DesktopAnnotation.qml` réutilise `PageArea` avec `tools: null` : supporté (prouvé par
   `pageAreaWithoutToolsDraws`, L0-S9).
10. **L7-T33** : conflits attendus réduits à `tests/CMakeLists.txt` (une ligne identique) ; tout autre conflit hors
    `graphify-out/` est un écart à rapporter.
11. **L7-T34** : « Convertir en texte » du menu contextuel est déjà en données ; les points d'accroche du lot 3 sont
    connus par leurs noms (contrats figés du socle).
12. **Décisions 1 et 2 de 7b (P7b:1206-1208)** : surcouches autonomes **déclarées couvertures** ; encre guidée par le
    filtre d'échantillons du socle.
13. **Noms** : le socle reprend ceux de bf8c930 (`spotlightFrame`, `shapefinder`, `OriginMarker.qml`) ; aucun
    renommage de plus.

## Durée d'exécution estimée

| Tâche | Durée (agent, construction comprise) |
|---|---|
| L0-S0 | 45 min |
| L0-S1 | 1 h |
| L0-S2 | 1 h 30 |
| L0-S3 | 3 h |
| L0-S4 | 2 h |
| L0-S5 | 1 h 30 |
| L0-S6 | 2 h 30 |
| L0-S7 | 3 h |
| L0-S9 | 1 h 30 |
| L0-S10 | 1 h |

Chemin critique L0-S0 → L0-S6 → L0-S7 → L0-S9 → L0-S10 : environ **9 h 15**. Avec deux constructions de front
(V1 et V2 ont chacune une tâche qui attend le jeton) et les relectures et merges (≈ 30 min par vague) : **10 h 30 à
11 h 30** de bout en bout.

## Hypothèses à revoir

- H-S1 (levée) : un `QTabletEvent` envoyé par `QCoreApplication::sendEvent` à la fenêtre atteint le filtre de la vue
  hors écran — déjà prouvé par `tests/tst_pageview.cpp` (lignes 63, 126-128).
- H-S2 : une valeur `null` passée à une `required property` d'un talon QML est acceptée (Qt 6.12) ; vérifiée par
  L0-S6 (`requiredPropertyAcceptsNull`), repli écrit dans sa section.
- H-S3 (tranchée) : menu Outils dans l'ordre de F-750 (bureau d'abord, puis stylo, marqueur, gomme, reconnaissances,
  rideau, sous-menus) ; « Sélectionner » n'y figure pas (palette et menu contextuel).
- H-S4 : `ActCraft::features` lié à `actcraft_ui` ne crée pas de cycle ; sinon L0-S0 rapporte avant de continuer.
- H-S5 : `noShortcutTwice` ne voit que les raccourcis déclarés dans `commands.json` ; L0-S4 relève les `Shortcut {}`
  écrits en QML (`git grep -n "Shortcut {" -- ui/qml`) et les ajoute au test s'il y en a hors de l'instanciateur des
  commandes.
