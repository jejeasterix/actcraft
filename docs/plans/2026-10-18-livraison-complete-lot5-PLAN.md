# Livraison complète — lot 5 « Édition et fichiers », sous-lot 5a (édition des objets) — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**But.** Une seule PR et une seule version pour le lot (attribuée à la fusion, aucun numéro ici). Le sous-lot 5a livre
la tâche d'ouverture, les réservations, et toute l'édition des objets : couper, copier, coller, dupliquer ; grouper ;
organiser et couches ; transformer et aligner ; menu d'édition d'objet ; points de forme ; pot de peinture ;
connecteur ; modification de stylo ; verrouiller, masquer, glisser une copie ; insérer une image ou un média. Le
sous-lot 5b (`docs/plans/2026-10-18-livraison-complete-lot5b-PLAN.md`) livre les onglets de documents, le module
d'export (DA12 : page en image, PDF, import PDF), l'impression complète (amendement 1, § 2.2 et DA-A2), l'import
PowerPoint en images (amendement 1, § 5, recommandation appliquée à la porte de fin du lot 2), les bancs et la
clôture.

Exigences : F-010, F-011, F-012, F-129 (insertion ; lecture au lot 8), F-221, F-235, F-245, F-513, F-525, F-526,
F-527, F-535, F-536, F-537, F-540, F-585, F-586, F-587, F-660 (PDF et PowerPoint en images), F-661 (en images
seulement), F-664, F-670 (complète, amendement), F-671, F-672, F-710 (entrées du lot), F-740 (entrées du lot),
F-770 ; les 5 constats du corpus affectés au lot 5 (amendement 1, § 4 : 3 « Stylo, formes, couleurs », 1 « Sélection,
verrouillage, masquage », 1 « Pages, fond, grille ») ; les symétries autour de l'origine XY versées par le lot 7
(P20 de L7-T0). Scénarios : **S8** (cadrage), **S17** (amendement), **S8.1 à S8.6** ajoutés par ce plan (§ Scénarios).

**Architecture.** Tout passe par une commande nommée (`actions/`) ; un outil (`tools/`) ne voit que
`tools::ToolContext` ; toute modification passe par la pile de `core/`. Cinq contraintes :

1. **Cache par son interface figée (DA11).** Couper, coller, dupliquer, grouper, dégrouper, projeter :
   `core::ReplaceObjectsCommand` (socle L0-S3), seule commande de remplacement. Aligner, ajuster, inverser,
   transformer : `core::TransformObjectsCommand` (lot 3, L3-T4). Glisser une copie : **soulèvement** du lot 3
   (`DocumentController::liftObjects` / `dropLifted`), la copie est ajoutée au dépôt. Aucune tâche n'appelle
   `FixedPageItem` ni ne contourne `core::applyToCache`. Un changement d'ordre (organiser) est une insertion hors fin
   de page : il garde le redessin immédiat du lot 10 (limite déclarée, mesurée par L5-T19).
2. **L'export lit `core::Document`, jamais le cache.** Copie du document faite sur le fil principal, rendu par le
   peintre partagé (`render::paintPage`, DA5) sur un fil secondaire (5b).
3. **Une seule `PageView` pour tous les onglets (5b).** Le cache MSAA coûte 36 octets par pixel (coffre
   `04_Pieges/qt-msaa-couche-36-octets-par-pixel-et-profondeur-2d-trop-tard.md`) : 1920 × 1080 × 36 ≈ 74,6 Mo par
   vue ; quatre onglets à une vue chacun ≈ 300 Mo. Les onglets garent leurs documents hors de la vue ; L5-T20 le
   mesure (S8.5).
4. **`export` est un mot réservé du C++.** Le module s'appelle `export/` (`actcraft_export`, en-têtes
   `<actcraft/export/…>`), son espace de noms `actcraft::exporting`.
5. **Fichiers chauds réservés en tête.** `L5-T1` pose commandes, disposition, icônes, CMake (modules `export/` et
   `print/` compris), les 26 tests du lot (talons `QSKIP` nommant leur tâche) et les en-têtes figés des deux
   sous-lots ; `L5-T2` pose la coquille QML. Deux tâches d'une même vague ne partagent aucun fichier.

**Pile.** C++20, Qt 6.12 (Core, Gui, Quick, QuickControls2, Svg, Test ; PrintSupport au 5b ; Pdf au 5b, à installer
par L5-T22), CMake + Ninja, MSVC 2022, PowerShell 7.

**Point de départ.** Le lot 5 démarre sur `feat/livraison-complete` après la clôture du lot 4 et la fusion du lot 7
(session B). Ce plan est écrit sur 794cf84, avant le code du socle L0-S, des lots 3, 4 et 7 : **toute API citée dans
la table « API supposées » est une hypothèse « à revérifier »** ; L5-T0 la revérifie contre le code livré et tout
écart amende ce plan (relecture exigée), comme aux lots 4 et 10. Les numéros de ligne sont indicatifs, les noms
font foi.

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L5-T0 | Ouverture : vidéos, REA et ancien dépôt confrontés au lot ; décisions D1 à D20 ; API A1 à A21 revérifiées | — | `docs/verifications/2026-10-18-lot5-principes.md` (hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot5.md`) | isolé | concepteur-critique | rapport commité, `NOMS OK` après `git add`, contrôle des titres du corpus sans sortie | S8.6 | #tache-l5-t0 |
| L5-T1 | Réservation I : commandes, disposition, icônes, CMake (modules `export/` et `print/`), 26 tests enregistrés, en-têtes figés et talons des deux sous-lots | — | `actions/data/commands.json`, `actions/data/layout.json`, `ui/icons/{couper,copier,coller,dupliquer,grouper,degrouper,organiser,aligner,transformer,verrouiller,masquer,glisser-copie,points-forme,pot-peinture,connecteur,modification-stylo,menu-objet,image,media,onglets,exporter,imprimer,importer}.svg`, `ui/CMakeLists.txt`, `CMakeLists.txt`, `core/CMakeLists.txt`, `formats/CMakeLists.txt`, `tools/CMakeLists.txt`, `export/CMakeLists.txt`, `print/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/{core,formats,tools,ui}.cmake`, `tests/export.cmake`, `tests/print.cmake`, `core/include/actcraft/core/{clipboardedits,arrange,alignment,objectflags,fill,shapepoints,connectors,penmodifiers,printlayout}.h`, `core/src/{clipboardedits,arrange,alignment,objectflags,fill,shapepoints,connectors,penmodifiers,printlayout}.cpp`, `formats/include/actcraft/formats/clipboard.h`, `formats/src/clipboard.cpp`, `render/include/actcraft/render/imagefill.h`, `render/src/imagefill.cpp`, `tools/include/actcraft/tools/{filltool,penshapetool,connectortool}.h`, `tools/src/{filltool,penshapetool,connectortool}.cpp`, `export/include/actcraft/export/{pageexport,pdfexport,pdfimport,officeconvert}.h`, `export/src/{pageexport,pdfexport,pdfimport,officeconvert}.cpp`, `print/include/actcraft/print/printer.h`, `print/src/printer.cpp`, `ui/include/actcraft/ui/{objectediting,insertion,documenttabs,fileexchange,printcontroller}.h`, `ui/src/{objectediting,insertion,documenttabs,fileexchange,printcontroller}.cpp`, `ui/include/actcraft/ui/documentcontroller.h` (déclarations `parkSession` / `resumeSession` seulement), `ui/src/documentcontroller.cpp` (talons de ces deux membres seulement), 26 fichiers `tests/tst_*.cpp` de la carte, `tests/tst_actions_layout.cpp`, `core/include/actcraft/core/model.h` (champs `dragCopy`, `ConnectorCap`, `style`, `capA`, `capB`), `formats/src/nativejson.cpp` (ces champs), `tests/tst_formats_nativejson.cpp` | isolé | implementeur-tdd | build 0 avertissement ; `ctest -N` : N = N_départ + 26 ; `tst_actions_layout` Passed dont `lot5EntriesDeclared`, `objectMenuOrder` ; `tst_formats_nativejson` Passed dont `lot5FieldsRoundTrip` ; les 26 talons en SKIP nommant leur tâche ; `NOMS OK` | S8, S17 | #tache-l5-t1 |
| L5-T2 | Réservation II : coquille QML (menu d'édition d'objet, barre d'objet complète, onglets, emplacements export, impression, import ; gestionnaires des commandes du lot) | L5-T1 | `ui/qml/Main.qml`, `ui/qml/PageArea.qml`, `ui/qml/ObjectBar.qml`, `ui/qml/TopBar.qml`, `ui/qml/{ObjectEditMenu,PenModifierBar,ConnectorPanel,DocumentTabBar,FileExchangeActions,PrintDialog,SheetPreview}.qml` (neufs, talons), `ui/CMakeLists.txt` (bloc `QML_FILES` seulement), `tests/tst_ui_shell.cpp` | isolé | implementeur-tdd | `tst_ui_shell` Passed dont `everyLot5CommandHasAHandler`, `lot5SlotsExist`, `shellLoadsWithoutQmlWarning` ; `premiers-pas` hors écran `CAPTURES OK` | S8 | #tache-l5-t2 |
| L5-T3 | `formats` : presse-papiers d'objets (objets, ressources, groupes et connecteurs, sans perte) | L5-T0, L5-T1 | `formats/src/clipboard.cpp`, `tests/tst_formats_clipboard.cpp` | isolé | concepteur-critique | `tst_formats_clipboard` Passed (dont `roundTripKeepsEveryKind`, `resourcesTravelWithObjects`, `groupMembersTravelTogether`, `foreignPayloadRejected`) ; mutation rouge puis remise | S8, S8.1 | #tache-l5-t3 |
| L5-T5 | `core` : organiser (avancer, reculer, premier plan, arrière-plan, couches) et groupes (grouper, dégrouper, groupes de groupes) | L5-T0, L5-T1 | `core/src/arrange.cpp`, `tests/tst_core_arrange.cpp` | isolé | implementeur-tdd | `tst_core_arrange` Passed (dont `forwardSwapsOneStep`, `frontKeepsRelativeOrder`, `layerChangeGoesOnTop`, `groupingIsASingleStep`, `ungroupRestoresInnerGroups`, `reorderChangesSeenByCache`) | S8, S8.2 | #tache-l5-t5 |
| L5-T4 | `core` : copies prêtes à coller (identifiants neufs, groupes et connecteurs remappés, décalages, coller sur l'objet) | L5-T3 | `core/src/clipboardedits.cpp`, `tests/tst_core_clipboardedits.cpp` | isolé | concepteur-critique | `tst_core_clipboardedits` Passed (dont `pasteOffsetsBy20`, `duplicateOffsetsBy10AndSelectsCopy`, `copiesAreUnlockedAndUnnamed`, `groupReferencesRemapped`, `connectorEndsRemapped`, `pasteOnObjectKeepsItsLayer`, `cutIsOneStep`) ; mutation rouge puis remise | S8, S8.1 | #tache-l5-t4 |
| L5-T6 | `core` : aligner, ajuster, taille d'origine, projeter, inverser, symétries autour de l'origine XY | L5-T0, L5-T1 | `core/src/alignment.cpp`, `tests/tst_core_alignment.cpp` | isolé | implementeur-tdd | `tst_core_alignment` Passed (dont `alignLeftUsesTheLeftmostEdge`, `alignWidthUsesTheFirstCreated`, `lockedObjectsStay`, `fitOnlyShrinksOversized`, `flipXNegatesRotation`, `mirrorXPlacesCopyAbove`, `originSymmetry`) | S8, S8.2 | #tache-l5-t6 |
| L5-T7 | `core` : verrouiller, masquer, glisser une copie (bascules d'objet, une étape) | L5-T0, L5-T1 | `core/src/objectflags.cpp`, `tests/tst_core_objectflags.cpp` | isolé | implementeur-tdd | `tst_core_objectflags` Passed (dont `toggleLockIsOneStep`, `mixedSelectionLocksAll`, `hiddenObjectNotPainted`, `dragCopyFlagRoundTrips`) | S8.3 | #tache-l5-t7 |
| L5-T8 | `core` + `render` : pot de peinture (intérieur, contour, fond de page, zone d'une image avec tolérance) | L5-T0, L5-T1 | `core/src/fill.cpp`, `render/src/imagefill.cpp`, `tests/tst_core_fill.cpp` | isolé | implementeur-tdd | `tst_core_fill` Passed (dont `insideClosedShapeFills`, `onOutlineRecolorsPen`, `emptyAreaFillsBackground`, `lockedObjectsAreFilled`, `imageFloodFillTolerance5`, `fillIsOneStep`) | S8.3 | #tache-l5-t8 |
| L5-T9 | `core` : points de forme (contour en points, déplacer un point, une étape) | L5-T0, L5-T1 | `core/src/shapepoints.cpp`, `tests/tst_core_shapepoints.cpp` | isolé | implementeur-tdd | `tst_core_shapepoints` Passed (dont `squareHasEightPoints`, `movingAPointDeformsTheShape`, `smartShapeKeepsItsSmartPoints`, `pointMoveIsOneStep`) | S8.3 | #tache-l5-t9 |
| L5-T11 | `core` : modification de stylo (tracés géométriques produits comme annotations) | L5-T0, L5-T1 | `core/src/penmodifiers.cpp`, `tests/tst_core_penmodifiers.cpp` | isolé | implementeur-tdd | `tst_core_penmodifiers` Passed (dont `horizontalLineIgnoresVerticalMotion`, `squareHasEqualSides`, `circleClosed`, `arrowHasAHead`, `resultIsAStrokeOnTheTopLayer`) | S8.4 | #tache-l5-t11 |
| L5-T10 | `core` : connecteurs (points d'attache, accrochage à 20 px, suivi des objets dans toute commande qui les déplace) | L5-T0, L5-T1, L5-T5, L5-T6 | `core/src/connectors.cpp`, `tests/tst_core_connectors.cpp` | isolé | concepteur-critique | `tst_core_connectors` Passed (dont `attachPointsAreShapeVertices`, `endSnapsWithin20`, `connectorFollowsTransform`, `connectorFollowsAlign`, `undoRestoresConnectorAndObject`, `deletingAnEndDetaches`) ; mutation rouge puis remise | S8.4 | #tache-l5-t10 |
| L5-T14 | `ui` : contrôleur d'édition d'objet (presse-papiers du système, couper, copier, coller, dupliquer, supprimer, tout sélectionner, grouper, organiser, aligner, transformer, bascules) | L5-T2, L5-T4, L5-T5, L5-T6, L5-T7 | `ui/src/objectediting.cpp`, `tests/tst_ui_objectediting.cpp` | isolé | implementeur-tdd | `tst_ui_objectediting` Passed (dont `copyPasteThroughSystemClipboard`, `pasteTextFromAnotherApp`, `pasteImageFromAnotherApp`, `withoutSelectionActsOnPage`, `groupThenUndoClearsSelection`, `alignGoesThroughTransformCommand`, `cutGoesThroughReplaceCommand`) | S8, S8.1, S8.2 | #tache-l5-t14 |
| L5-T15 | `ui` : insérer une image ou un média (fichier, glisser-déposer, décodage hors du fil principal) | L5-T2, L5-T4 | `ui/src/insertion.cpp`, `tests/tst_ui_insertion.cpp` | isolé | implementeur-tdd | `tst_ui_insertion` Passed (dont `imageFitsIn80PercentCentered`, `imageNamedAfterFile`, `decodeOffMainThread`, `mediaInsertedAtCenter`, `droppedTextBecomesText`, `insertionIsOneStep`) | S8.1 | #tache-l5-t15 |
| L5-T12 | `tools` : outils pot de peinture et modification de stylo | L5-T8, L5-T11 | `tools/src/filltool.cpp`, `tools/src/penshapetool.cpp`, `tests/tst_tools_fill.cpp`, `tests/tst_tools_penshape.cpp` | isolé | implementeur-tdd | `tst_tools_fill` (dont `clickFillsTopmostClosedShape`, `clickOnEmptyFillsBackground`) et `tst_tools_penshape` (dont `dragDrawsAPreviewThenOneStroke`, `previewUsesTheLiftSurface`) Passed | S8.3, S8.4 | #tache-l5-t12 |
| L5-T13 | `tools` : outil connecteur (survol des points d'attache, tracé d'un objet à l'autre, une étape) | L5-T10 | `tools/src/connectortool.cpp`, `tests/tst_tools_connector.cpp` | isolé | implementeur-tdd | `tst_tools_connector` Passed (dont `hoverShowsAttachPoints`, `dragFromObjectToObjectConnects`, `releaseOnEmptyLeavesAFreeEnd`, `connectorGoesOnTopLayer`) | S8.4 | #tache-l5-t13 |
| L5-T16 | `tools` : sélection — glisser une copie et édition des points de forme | L5-T4, L5-T7, L5-T9 | `tools/src/selecttool.cpp`, `tools/src/selectiongeometry.cpp`, `tests/tst_tools_selectcopy.cpp` | isolé | concepteur-critique | `tst_tools_selectcopy` Passed (dont `dragCopyLeavesTheOriginal`, `dragCopyIsOneStep`, `lockedMovesOnlyByMoveHandle`, `shapePointsModeShowsSquares`, `shapePointDragIsOneStep`) ; `tst_tools_select` (lot 3) encore vert | S8.3 | #tache-l5-t16 |
| L5-T17 | `ui::ToolHost` : outils du lot branchés (pot de peinture, connecteur, modification de stylo, mode points de forme) et mémoire par outil | L5-T12, L5-T13, L5-T16 | `ui/src/toolhost.cpp`, `ui/src/tooladapters.cpp`, `ui/src/tooladapters.h`, `tests/tst_ui_lot5tools.cpp` | isolé | implementeur-tdd | `tst_ui_lot5tools` Passed (dont `fillToolActivatesFromCommand`, `penModifierOnlyWithInkTools`, `connectorRemembersStyle`, `escapeLeavesShapePoints`) ; banc hors écran joint | S8.3, S8.4 | #tache-l5-t17 |
| L5-T18 | QML : menu d'édition d'objet, sous-menus, barre d'objet complète, barre de modification de stylo, panneau des connecteurs ; scénario `edition` | L5-T14, L5-T15, L5-T17 | `ui/qml/ObjectEditMenu.qml`, `ui/qml/PenModifierBar.qml`, `ui/qml/ConnectorPanel.qml`, `ui/qml/ObjectBar.qml`, `ui/qml/Main.qml` (gestionnaire `connector.style`), `bench/scenarios/edition.json`, `tests/tst_ui_shell.cpp` | isolé | implementeur-tdd | `tst_ui_shell` Passed dont `objectMenuMatchesLayout`, `convertEntriesOnlyForAnnotations` ; `edition` hors écran `CAPTURES OK` avec `-Lire` (§ section) ; banc hors écran joint | S8, S8.2, S8.3 | #tache-l5-t18 |
| L5-T19 | Banc : scénario `edition` (coller, grouper, aligner, organiser sur page annotée), mesuré | L5-T14, L5-T17 | `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp`, `bench/src/runner.cpp`, `tests/tst_bench_scenarios.cpp` | isolé | implementeur-tdd | `tst_bench_scenarios` Passed dont `editionScenarioListed` ; banc hors écran `-Scenario court` JSON avec la section `edition` | S8.2 | #tache-l5-t19 |

Sous-lot 5b (`docs/plans/2026-10-18-livraison-complete-lot5b-PLAN.md`) : L5-T20 à L5-T33 (14 tâches). Lot entier :
**34 tâches** (estimation de l'amendement : 21 + PowerPoint en images 3 ; écart expliqué au § Écarts).

**N_départ** = nombre de tests ctest de `feat/livraison-complete` au départ du lot (après la fusion des lots 4 et 7),
relevé par L5-T1 (étape 1) et noté dans l'état par l'orchestrateur. L5-T1 enregistre **les 26 tests des deux
sous-lots** → **N = N_départ + 26** dès L5-T1 et jusqu'à la fin ; aucune autre tâche n'en enregistre.

## Vagues (largeur 3 au plus, 2 places de tests ; aucun fichier partagé dans une vague)

V1 : L5-T0 ‖ L5-T1 · V2 : L5-T2 ‖ L5-T3 ‖ L5-T5 · V3 : L5-T4 ‖ L5-T6 ‖ L5-T7 · V4 : L5-T8 ‖ L5-T9 ‖ L5-T11 ·
V5 : L5-T10 ‖ L5-T14 ‖ L5-T15 · V6 : L5-T12 ‖ L5-T13 ‖ L5-T16 · V7 : L5-T17 · V8 : L5-T18 ‖ L5-T19.
L5-T0 ne compile rien : il ne prend pas de place de tests. Les autres tâches d'une vague de 3 se partagent les deux
places (`jeton.ps1`) ; l'orchestrateur lance la suite complète une fois par vague, après les merges.

Recoupements vérifiés (colonne « fichiers touchés ») :
- V2 : T2 (QML de la coquille, `ui/CMakeLists.txt` bloc `QML_FILES`, `tst_ui_shell.cpp`) / T3 (`formats/src/clipboard.cpp`,
  son test) / T5 (`core/src/arrange.cpp`, son test) — disjoints.
- V3 : T4 / T6 / T7 — un `.cpp` de `core/` et son test chacun.
- V4 : T8 (`core/src/fill.cpp`, `render/src/imagefill.cpp`, test) / T9 / T11 — disjoints.
- V5 : T10 (`core/src/connectors.cpp`) / T14 (`ui/src/objectediting.cpp`) / T15 (`ui/src/insertion.cpp`) — disjoints.
- V6 : T12 (`filltool.cpp`, `penshapetool.cpp`) / T13 (`connectortool.cpp`) / T16 (`selecttool.cpp`,
  `selectiongeometry.cpp`) — disjoints.
- V8 : T18 (QML, `edition.json`, `tst_ui_shell.cpp`) / T19 (`bench/`, `tst_bench_scenarios.cpp`) — disjoints.
  `bench/scenarios/edition.json` (scénario d'interface, T18) et le scénario de banc `edition` (code de `bench/src`,
  T19) sont deux choses distinctes de même nom.

## API supposées (à revérifier par L5-T0)

| # | API | Tâche qui la livre | Tâches du lot qui s'en servent |
|---|---|---|---|
| A1 | `tools::Tool`, `tools::ToolContext` (`document`, `ink`, `strokes`, `lift`, `selection`, `view`, `commands`, `user`), `DocumentEditor::page/pageIndex/execute/lift/drop/isLifting`, `LiftSurface::show/setTransform/clear` | L0-S6 (en-têtes), L3-T6 à L3-T9 (corps) | T12, T13, T16 |
| A2 | `tools::SelectionView` (`corners`, `handles`, `bar`, `handlesVisible`, `rubberBand`, `smartPoints` du lot 4), `selectiongeometry.h` | L0-S6, L3-T9, L4-T1, L4-T10 | T16, T17 |
| A3 | `tools::UserState` (`settings`, `setSettings`, `currentTool`, `selection`), `tools::defaultSettings`, `ToolRegistry::add`, `registerBuiltinTools` (« select », « pen », « highlighter », « eraser », « shape », « text », « text-vertical ») | L3-T6, L4-T1 | T1, T12, T13, T17 |
| A4 | `ui::ToolHost` (type QML `Tools`, objet `appTools`), `activate`, `selectionFrame`, `user()`, `settingsFile` | L3-T14, L0-S6, L4-T11 | T14, T17 |
| A5 | `ui::DocumentController` : `execute`, `document()`, `liftObjects`, `dropLifted`, `isLifting`, `editAboutToApply`, `addChangeObserver` / `ChangeOrigin`, `deleteObjects`, `openUrl`, `saveToUrl`, `isModified`, `showPage` (L4-T1) ; ce lot y ajoute `parkSession` / `resumeSession` (L5-T1) | L0-S5, L3-T12, L4-T1 | T1, T14, T15, T20 |
| A6 | `core::ReplaceObjectsCommand(page, removed, added)`, `ReplaceObjectsCommand(page, std::vector<ObjectReplacement>)`, `addedIds()` ; placement au rang du plus haut retiré, sinon sommet de la couche | L0-S3 | T4, T5, T6, T14, T15, T16 |
| A7 | `core::TransformObjectsCommand(page, ids, transforms)`, `RemoveObjectsCommand`, `AddObjectCommand`, `SetTranslucencyCommand` | L3-T4 | T6, T10, T14 |
| A8 | `core::objectAt`, `core::objectsTouching`, `core::kHitToleranceScreen` | L3-T3 | T8, T12, T13 |
| A9 | `core::liftChanges`, `core::changesAfterLift`, `core::restoreObjects` (`lift.h`) | L3-T4 | T16 |
| A10 | `core::PageCache` (`insert`, `remove`, `invalidate`), `core::applyToCache`, `core::Changes` / `Change` (`ObjectInserted`, `ObjectRemoved`, `ObjectChanged`, `PageSettingsChanged`) ; règle du lot 10 : insertion hors fin de page → redessin immédiat | lot 2, lot 10 | T5, T8, T19 |
| A11 | `render::paintPage`, `render::renderPage`, `render::decodeImages`, `render::ImageLookup`, `render::PaintOptions` (**vérifié présent à 794cf84**, `render/include/actcraft/render/pagepainter.h:43-70`) | lot 2 | 5b (T23, T27) |
| A12 | Disposition : sous-menus `{"submenu": id, "label", "items"}`, `Layout::objectBar`, `contextMenu`, `MenuSpec::submenus`, filtrage par plateforme ; format `commands.json` : `id`, `label`, `icon`, `shortcut`, `checkable`, `platforms`, `argument` (`"url"`, `"number"`, texte), `optional` (**vérifié** à 794cf84 pour `argument` et `optional`) | L0-S4 | T1, T18 |
| A13 | Joueur de scénarios : `UiStepKind` `Pointer`, `DoubleClick`, `Touch`, `Pinch`, `expect` avec tolérance ; `typeText` | L0-S2, L4-T14 | T18, 5b |
| A14 | `PageArea.qml` (couches `pageContent`, `pendingLayer`, `liftLayer`, `ink`, `pageOverlays`, `selectionOverlay`, `toolCursor`), `Main.qml` (`appTools`, `appCommands`), `ObjectBar.qml`, `PageContextMenu.qml` (`popupAt`) | L0-S9, L3-T15, L3-T16, L4-T2 | T2, T18 |
| A15 | `core::pageedits` (insérer des copies de pages à identifiants neufs), `ui::PagesController` (sélection multiple, presse-papiers des pages, glisser), `PageNavigator.qml` | L4-T15, L4-T18, L4-T19 | 5b (T20, T21) |
| A16 | Catalogue des formes et points intelligents (`core/shapegeometry.h`, `SetShapeParametersCommand`) | L4-T3, L4-T10 | T9, T10 |
| A17 | Commandes `recognition.convert-to-shape`, `recognition.convert-to-text` (Windows), origine XY affichée et sa position (outils de maths du lot 7, `UserState::openMathTools`) | lot 7 (L7-T34, P7a:688-689) | T6, T18 |
| A18 | `ui::PageView` : `tools`, `sceneToPage`, `addInputCover`, `cacheRefreshCount` | L0-S7, L3-T13 | T15, T18, 5b |
| A19 | `ui::PageThumbnails` : copie de page sur le fil principal, rendu sur fil secondaire | L4-T17 | 5b (T23) |
| A20 | Copie d'un `core::Document` (valeur, ressources par clé SHA-256) ; `Page::notes` (`model.h:434`, **vérifié**) | lot 2 | T3, 5b |
| A21 | Commandes du lot 3 déjà là : `object.grow`, `object.shrink`, `edit.delete`, `edit.select-all` (identifiants supposés) | L0-S4, L3-T14 | T1, T14 |

## Décisions à rendre par L5-T0 (défaut appliqué tant qu'elles ne sont pas tranchées)

| # | Point | Défaut (source) |
|---|---|---|
| D1 | Coller : position, nom, verrou | décalage de +20 unités en x et y ; nom vide ; déverrouillé ; copie sélectionnée, outil Sélection (`app/actcraft/canvas/view.py:706-737`) ; « Coller » du menu d'objet : superposé à l'objet, même couche (F-540) |
| D2 | Dupliquer | décalage de +10 unités, le double est sélectionné (`view.py:748-760` ; plan comparatif ligne 22) |
| D3 | Couper, copier, coller, dupliquer, supprimer sans sélection | agissent sur la page (F-710) : copie de la page dans le presse-papiers des pages du lot 4 (A15) |
| D4 | Groupes | couche du groupe = la plus basse des membres ; ordre = le plus haut ; Ctrl+G bascule (grouper si ≥ 2, dégrouper si un groupe) ; Ctrl+G puis Ctrl+Z vide la sélection (`view.py:830-871`, plan comparatif ligne 25) |
| D5 | Organiser | avancer / reculer d'un cran parmi les objets de la même couche ; premier plan / arrière-plan ; changer de couche met au sommet de la couche d'arrivée (`view.py:780-818`, F-513) |
| D6 | Aligner | au moins 2 objets non verrouillés ; bords et centres sur l'union ; largeur, hauteur, angle, complet sur le **premier créé** (`view.py:873-937`) |
| D7 | Ajuster, taille d'origine | ajuster seulement les objets plus grands que la page (F-536) ; taille d'origine = transformation sans échelle |
| D8 | Projeter, inverser, symétries d'origine | inverser : retourne l'objet, angle opposé ; projeter X : copie miroir au-dessus, Y : à gauche (`view.py:939-960`) ; symétries autour de l'origine XY quand elle est affichée (P20 de L7-T0) |
| D9 | Verrouiller, masquer, glisser une copie | verrouillé : ni déplacé ni groupé par clic, seule la poignée « Déplacer » (F-585) ; masqué : non rendu (F-586 ; mode Création au lot 6) ; glisser une copie : chaque glissement laisse l'original et dépose une copie (F-587) |
| D10 | Pot de peinture | intérieur d'une forme fermée → remplissage ; contour → couleur du contour ; vide → fond de la page ; image → zone voisine, tolérance 5 sur 100 ; s'applique aux verrouillés (F-235, plan comparatif ligne 17) ; intersection de formes : non tranché → remplissage de la forme du dessus |
| D11 | Connecteur | 14 styles, ligne simple par défaut ; points d'attache aux sommets de la forme ; accrochage à moins de 20 px ; couche supérieure ; extrémités A et B : aucune, rayon, flèche (F-245, plan comparatif ligne 19) |
| D12 | Modification de stylo | ligne libre, horizontale, verticale, carré, rectangle, cercle, ellipse, arc, ligne brisée, flèche, zigzag (F-221, « liste exacte à confirmer ») |
| D13 | Points de forme | huit points pour un carré (sommets et milieux) ; un point déplacé fait de la forme un polygone ; une forme intelligente garde ses points dédiés (F-527, A16) |
| D14 | Onglets (5b) | onglets en haut ; nom en gras pour l'actif ; astérisque si modifié ; croix ; « Sans titre » ; pages déposées sur un onglet copiées avant sa première page (F-010 à F-012) |
| D15 | Export de page et PDF (5b) | BMP, JPEG, PNG, TIF à la taille de la page en pixels (1024 × 768 par défaut) ; PDF : une page par page, à la taille de la page (F-671, F-672, S8) |
| D16 | Import PDF (5b) | document neuf, une page par page du PDF, page à la taille du PDF à 96 ppp, image verrouillée sur la couche de fond |
| D17 | Impression (5b) | règle du nombre de feuilles (S17) ; notes ; page allongée entière ; gris ; aperçu ; réglages gardés (F-670, amendement § 2.2) |
| D18 | PowerPoint en images (5b) | une diapositive = une page, en image de fond, par la chaîne de D16 |
| D19 | Insérer une image ou un média | au plus 80 % de la page, centrée, couche intermédiaire, nommée d'après le fichier (`view.py:762-778`) ; média au centre (F-129) |
| D20 | Menu d'édition d'objet | entrées et ordre de F-770 pour les fonctions livrées (lots 3 à 5, 7) ; entrées des lots 6 et 8 absentes jusqu'à leur lot |

## Carte des fichiers

| Fichier | Rôle | Tâche |
|---|---|---|
| `formats/include/actcraft/formats/clipboard.h`, `formats/src/clipboard.cpp` | charge utile du presse-papiers : objets + ressources, JSON du format natif | T1 (figé), T3 |
| `core/include/actcraft/core/clipboardedits.h`, `core/src/clipboardedits.cpp` | copies prêtes à coller, dupliquer, coller sur l'objet | T1, T4 |
| `core/…/arrange.h`, `core/src/arrange.cpp` | ordre, couches, groupes | T1, T5 |
| `core/…/alignment.h`, `core/src/alignment.cpp` | aligner, ajuster, projeter, inverser | T1, T6 |
| `core/…/objectflags.h`, `core/src/objectflags.cpp` | verrouiller, masquer, glisser une copie | T1, T7 |
| `core/…/fill.h`, `core/src/fill.cpp`, `render/…/imagefill.h`, `render/src/imagefill.cpp` | pot de peinture | T1, T8 |
| `core/…/shapepoints.h`, `core/src/shapepoints.cpp` | points de forme | T1, T9 |
| `core/…/connectors.h`, `core/src/connectors.cpp` | connecteurs et leur suivi | T1, T10 |
| `core/…/penmodifiers.h`, `core/src/penmodifiers.cpp` | tracés de la modification de stylo | T1, T11 |
| `core/…/printlayout.h`, `core/src/printlayout.cpp` | feuilles d'impression | T1, 5b T26 |
| `tools/…/{filltool,penshapetool,connectortool}.h`, `tools/src/…` | outils du lot | T1, T12, T13 |
| `ui/…/objectediting.h`, `ui/src/objectediting.cpp` | commandes d'édition d'objet (type QML `ObjectEditing`, objet `appEditing`) | T1, T14 |
| `ui/…/insertion.h`, `ui/src/insertion.cpp` | insérer une image ou un média (type QML `Insertion`, objet `appInsertion`) | T1, T15 |
| `ui/…/documenttabs.h`, `ui/src/documenttabs.cpp` | onglets (type QML `DocumentTabs`, objet `appTabs`) | T1, 5b T20 |
| `ui/…/fileexchange.h`, `ui/src/fileexchange.cpp` | export, import PDF et PowerPoint (type QML `FileExchange`, objet `appExchange`) | T1, 5b T25, T30 |
| `ui/…/printcontroller.h`, `ui/src/printcontroller.cpp` | boîte d'impression (type QML `PrintController`, objet `appPrint`) | T1, 5b T28 |
| `export/…/{pageexport,pdfexport,pdfimport,officeconvert}.h`, `export/src/…` | module d'export (DA12) | T1, 5b T23, T24, T29 |
| `print/…/printer.h`, `print/src/printer.cpp` | impression par `QPrinter` (DA-A2) | T1, 5b T27 |

**Les 26 tests du lot** (enregistrés par L5-T1) : 5a — `tst_formats_clipboard` (T3), `tst_core_clipboardedits` (T4),
`tst_core_arrange` (T5), `tst_core_alignment` (T6), `tst_core_objectflags` (T7), `tst_core_fill` (T8),
`tst_core_shapepoints` (T9), `tst_core_connectors` (T10), `tst_core_penmodifiers` (T11), `tst_tools_fill` (T12),
`tst_tools_penshape` (T12), `tst_tools_connector` (T13), `tst_ui_objectediting` (T14), `tst_ui_insertion` (T15),
`tst_tools_selectcopy` (T16), `tst_ui_lot5tools` (T17) ; 5b — `tst_ui_tabs` (T20), `tst_export_image` (T23),
`tst_export_pdf` (T23), `tst_export_pdfimport` (T24), `tst_ui_fileexchange` (T25), `tst_core_printlayout` (T26),
`tst_print_printer` (T27), `tst_ui_printdialog` (T28), `tst_export_officeconvert` (T29), `tst_ui_import` (T30).

## Commandes communes des preuves

`<wt>` = dossier du worktree de la tâche. Jamais de `cd` : `git -C <wt>`, et un process enfant pour le dossier
courant. Toute construction et toute suite se jouent sous le jeton `tests` (une commande par prise).

**Commande ciblée** (preuve de tâche ; `<motif>` = les tests de la tâche, par exemple `^(tst_core_arrange)$`) :
```powershell
pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; . ./scripts/dev-env.ps1; pwsh -NoProfile -File ./scripts/build.ps1 > .tmp/build.txt 2>&1; ctest --test-dir build/debug -R '<motif>' --output-on-failure > .tmp/cible.txt 2>&1"
Select-String -Path '<wt>/.tmp/build.txt' -Pattern 'warning|error' | Select-Object -First 5      # aucune ligne
Select-String -Path '<wt>/.tmp/cible.txt' -Pattern 'tests passed|tests failed|Passed|Failed|Skipped' | Select-Object -Last 8
```
Motif de réussite : `100% tests passed, 0 tests failed out of <k>` avec k = nombre de tests du motif (k ≥ 1) et le
nom de chaque test attendu sur une ligne `Passed`. Le détail des fonctions : `build/debug/tests/<test>.exe -o
.tmp/<test>.txt,txt` (sortie standard muette dans cet environnement) puis `Select-String -Path .tmp/<test>.txt
-Pattern '^(PASS|FAIL|SKIP)'`.

**Format** : `pwsh -NoProfile -Command "Set-Location '<wt>'; pwsh -NoProfile -File ./scripts/format.ps1 -Check"` —
aucune sortie d'écart.

**Noms** (après `git add`, jamais avant : l'outil ne lit que les fichiers suivis) :
`python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" <wt> | Select-Object -Last 1`
→ `NOMS OK : <n> fichiers`.

**Titres du corpus** (toute tâche qui écrit un document) : pour chaque fiche du corpus, sa première ligne `# …`
cherchée dans le worktree ; **sortie vide, sans exception** :
```powershell
pwsh -NoProfile -Command "Get-ChildItem 'C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\*.md' -Exclude README.md | ForEach-Object { `$t = (Get-Content -LiteralPath `$_.FullName -TotalCount 5 -Encoding utf8 | Where-Object { `$_ -like '# *' } | Select-Object -First 1); if (`$t) { git -C '<wt>' grep -l -i -F -- `$t.Substring(2).Trim() } }"
```

**Banc sans écran** (toute tâche qui touche `render/`, `input/`, `tools/` ou `ui/`) :
```powershell
pwsh -NoProfile -Command "Set-Location '<wt>'; pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu logiciel -Scenario court -Taille 1280x800 -HorsEcran > .tmp/bench.txt 2>&1"
Select-String -Path '<wt>/.tmp/bench.txt' -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE' | Select-Object -Last 1
```
Le verdict est un résultat, pas une porte (profil) ; le JSON de `bench/results/` est joint au rapport.

**Mutation** : une mutation se remet **par `git -C <wt> checkout -- <fichier>`**, jamais par `Copy-Item` (une remise
par copie garde une date plus ancienne : ninja ne recompile pas et la preuve tourne sur un exécutable périmé).

**Commit** : chemins explicites (`git -C <wt> add <chemins>`, jamais `-A` ni `.`), message en français terminé par la
ligne `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.

## Scénarios

- **S8** (cadrage) — Copier un groupe vers un deuxième onglet, l'aligner à gauche, annuler ; `export.pdf <chemin>`
  sur 3 pages : le fichier compte 3 pages ; `export.page-image <chemin>` donne un PNG de 1024 × 768. Joué par
  L5-T32 (5b).
- **S17** (amendement) — Imprimer (Windows) : joué par L5-T28 (5b).
- **S8.1 — Presse-papiers.** Copier un trait, une image et un texte, coller : trois objets décalés de +20, déverrouillés,
  sélectionnés, une seule étape d'annulation ; un texte copié d'une autre application se colle en objet texte ; une
  image du presse-papiers du système se colle à 80 % de la page au plus.
- **S8.2 — Grouper, organiser, aligner.** Trois formes ; grouper (Ctrl+G), dupliquer le groupe (+10) ; aligner à gauche
  les deux groupes ; « Premier plan » sur le plus bas ; trois Annuler rendent l'état de départ ; le banc `edition` mesure
  ces gestes sur une page annotée.
- **S8.3 — Propriétés et outils d'objet.** Verrouiller une forme : un glissement ne la déplace pas ; masquer : ses
  pixels disparaissent ; glisser une copie : l'original reste ; pot de peinture dans une forme fermée, sur son contour,
  sur la page vide (contrôles de pixels) ; points de forme : un carré montre 8 points, en déplacer un déforme la forme.
- **S8.4 — Connecteur et modification de stylo.** Relier deux formes par le connecteur, déplacer l'une : la ligne suit
  (contrôle de pixels) ; Annuler remet les deux ; modification de stylo « horizontale » : un tracé oblique produit une
  annotation horizontale, effaçable à la gomme.
- **S8.5 — Onglets** (5b) — quatre documents ouverts : la mémoire du processus ne croît pas d'une vue par onglet
  (seuil de L5-T20) ; pages glissées d'un onglet à l'autre copiées avant la première page ; fermer un onglet modifié
  demande l'enregistrement.
- **S8.6 — Ouverture.** Le rapport `docs/verifications/2026-10-18-lot5-principes.md` relie chaque constat du corpus
  affecté au lot à une exigence, une tâche et un scénario (ou une raison), et rend D1 à D20 et A1 à A21.

## Écarts au cadrage (repris à la porte de fin du lot 5)

1. **Volume.** 21 tâches estimées (amendement § 6) + 3 pour PowerPoint en images = 24 → 34 dans ce plan :
   réservations (2), modèle d'édition éclaté par fonction (8 tâches `core`), outils et ToolHost (4), onglets (2),
   installation de Qt PDF (1), bancs (2), contrôle de fin (1). Le constat « principes : + 1 tâche » est absorbé par les
   tâches des fonctions ; si L5-T0 relève un principe qui n'entre dans aucune, il ajoute une tâche corrective `L5-C<n>`
   par amendement.
2. **Imports retenus.** Packs de ressources (≈ 6 tâches) : **au lot 6**, parce qu'ils exigent le navigateur de
   ressources (amendement § 5) ; PowerPoint en images : au lot 5, parce qu'il réutilise l'import PDF (H5-1).
3. **Insérer un média** (F-129) : insertion et espace réservé au lot 5 ; lecture au lot 8 (DA-A3).
4. **Pot de peinture sur l'intersection de formes** : remplissage de la forme du dessus tant que L5-T0 ne l'a pas
   tranché (H5-4).
5. **Convertir en forme / en texte** : commandes du lot 7 (A17), entrées posées au menu d'édition d'objet par L5-T18.

---

### Tâche L5-T0 — Ouverture : vidéos, REA et ancien dépôt confrontés au lot ; API revérifiées

<a id="tache-l5-t0"></a>

**Objectif.** Décision 0100 et règle `Lx-T0` (amendement § 4), sur le modèle de L10-T0
(`docs/verifications/2026-10-18-lot10-principes.md`) et de L4-T0 : relever dans le corpus tous les principes des
fonctions du lot (presse-papiers, dupliquer, groupes, organiser, couches, aligner, transformer, menu d'édition d'objet,
points de forme, pot de peinture, connecteur, modification de stylo, verrouiller, masquer, glisser une copie, onglets,
insérer une image ou un média, export, import PDF, impression, PowerPoint en images), plus les **5 constats** affectés
au lot 5 par le tableau du § 4 de l'amendement ; trancher sur la 3.5.18 par REA ; écrire en nos mots les exigences,
les **décisions D1 à D20** (table ci-dessus), puis **revérifier les API A1 à A21** contre le code livré par L0-S et
les lots 3, 4 et 7.

**Sources.**
- Corpus : `C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\README.md` (« Principes absents », « Écarts
  avec la spec ») et ses fiches `*.md`.
- Spécification : `C:\Users\JeromeDenis\REA - Activinspire\docs\spec\fonctionnalites.md` (exigences du § « But »).
- Plan comparatif : `docs\tests\plan-comparatif.md`, lignes 17 (remplissage), 19 (connecteur), 22 (manipulation,
  dupliquer), 25 (groupes, aligner, organiser, transformer), 29 (fichiers, exporter, imprimer).
- Ancien dépôt : `app/actcraft/canvas/view.py` (685-960 : presse-papiers, dupliquer, organiser, groupes, aligner,
  inverser, projeter), `canvas/editmenus.py` (175-200 : menus Transformer, Aligner, Organiser), `canvas/items.py`
  (801 : tracés des connecteurs), `canvas/page.py` (90 : connecteurs qui suivent), `ui/doc_tabs.py`,
  `ui/main_window.py` (798-810 import PDF, 1275-1320 export et impression), `ui/print_dialog.py` ; graphe :
  `graphify query "…"` lancé **dans** `C:\Users\JeromeDenis\REA - Activinspire`.
- Format : `docs\re\format-flipchart.md` (§ 6, § 7) — notre documentation (groupes, connecteurs, verrou, masquage).
- Binaire : skill `reverse-engineer-anything` sur `C:\Program Files\Activ Software\Inspire\`, **en statique
  seulement** (ressources des boîtes, chaînes, valeurs par défaut, raccourcis). Aucune observation à l'écran sans
  prévenir Jérôme (mode autonome, point 3) ; si une décision l'exige : « non tranché, observation à l'écran
  demandée », et continuer.
- API : dans le worktree, `graphify query "ReplaceObjectsCommand TransformObjectsCommand ObjectReplacement"`,
  `graphify query "DocumentController liftObjects dropLifted ChangeOrigin"`, `graphify query "ToolContext
  LiftSurface SelectionView UserState"`, `graphify query "PagesController pageedits clipboard"`,
  `graphify query "recognition convert-to-shape origin"`, puis lecture des en-têtes trouvés.

**Fichiers.**
- Créer : `docs/verifications/2026-10-18-lot5-principes.md` (dans le dépôt, nos mots).
- Créer hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot5.md` (relevé brut).

**Test et commande bornée.** Pas de code. Preuves :
```powershell
Test-Path 'C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot5.md'   # True
git -C '<wt>' add docs/verifications/2026-10-18-lot5-principes.md
python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" '<wt>' | Select-Object -Last 1   # NOMS OK : <n> fichiers
```
puis la commande « Titres du corpus » (§ Commandes communes) : aucune sortie.

**Preuve visuelle.** Aucune.

**Interdits.** Aucun titre, aucune transcription, aucune citation de vidéo dans le dépôt (identifiants `Vnn`). Aucun
nom de l'original (classes, propriétés, libellés propres), aucun texte de l'aide, aucune capture, aucun nom de
paperboard réel. Aucune ligne de code. Ne modifier ni `samples/` ni le corpus. Pas d'observation à l'écran sans
annonce. Ne pas toucher au fichier d'état.

**Terminé quand.** Le rapport a : une ligne par constat (principe → exigence en nos mots → tâche du lot 5 → scénario,
ou raison et lot destinataire) ; une section « Décisions » D1 à D20 (valeur retenue et source, ou « non tranché » →
hypothèse de niveau 1) ; une section « API » A1 à A21 (« conforme » ou écart précis : nom réel, fichier:ligne) ; une
section « Amendements du plan » (tâche, fichier, changement) que l'orchestrateur applique avant V2 ; `NOMS OK` ;
titres du corpus sans sortie.

**Étapes.**
- [ ] **Étape 1 : constats.** Lire le README du corpus, sections « Principes absents » (thèmes « Stylo, formes,
  couleurs », « Sélection, verrouillage, masquage », « Pages, fond, grille ») et « Écarts avec la spec » ; ouvrir
  chaque fiche citée ; noter dans `lot5.md` (hors dépôt) le principe, la fiche, la fonction du lot.
- [ ] **Étape 2 : spécification et ancien dépôt.** Pour chaque exigence du lot, relever le comportement de l'ancien
  dépôt (fichiers des Sources) et l'accord avec la spécification ; noter les écarts.
- [ ] **Étape 3 : REA en statique.** Pour D1 à D20, chercher dans le binaire les chaînes et valeurs par défaut
  (raccourcis, ordre du menu d'objet, styles de connecteur, liste des tracés de modification de stylo, boîte
  d'impression : champs et valeurs par défaut, formats d'export de page). Relevé brut dans `lot5.md`.
- [ ] **Étape 4 : API.** Lancer les requêtes `graphify` des Sources dans le worktree ; ouvrir chaque en-tête ;
  remplir la section « API » (A1 à A21).
- [ ] **Étape 5 : rapport.** Écrire `docs/verifications/2026-10-18-lot5-principes.md` en nos mots : table des
  constats, décisions, API, amendements, hypothèses pour Jérôme.
- [ ] **Étape 6 : contrôles et commit.** `git add` du rapport, `NOMS OK`, titres du corpus sans sortie, puis :
  ```powershell
  git -C '<wt>' commit -m "Lot 5 : ouverture, principes du corpus et API revérifiées

  Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

### Tâche L5-T1 — Réservation I : commandes, disposition, icônes, CMake, tests, en-têtes figés

<a id="tache-l5-t1"></a>

**Objectif.** Poser d'un coup tous les fichiers chauds et les contrats des deux sous-lots, pour qu'aucune tâche
suivante ne touche un fichier de liste (CMake, données, enregistrement des tests) ni un en-tête public partagé :
commandes et disposition du lot, 23 icônes à nous, modules `export/` et `print/` vides, champs du modèle du lot
(lus et écrits sans perte par le format natif), 26 tests enregistrés (talons `QSKIP` nommant leur tâche), en-têtes
figés et corps talons. Aucune fonction visible : l'application se comporte comme avant, les nouvelles entrées de
menu sont inertes (aucun gestionnaire avant L5-T2).

**Fichiers.** Ceux de la ligne de la table, plus `core/include/actcraft/core/model.h` (champs du lot seulement),
`formats/src/nativejson.cpp` (ces champs seulement), `tests/tst_formats_nativejson.cpp` (une fonction).

**Test et commande bornée.**
- `tst_actions_layout` : `lot5EntriesDeclared` (chaque commande du lot existe, une seule fois ; raccourcis uniques),
  `objectMenuOrder` (ordre de `objectMenu` ci-dessous) ; `tst_formats_nativejson` : `lot5FieldsRoundTrip`.
- Commande ciblée, motif `^(tst_actions_layout|tst_formats_nativejson)$` ; puis
  `pwsh -NoProfile -Command "Set-Location '<wt>'; . ./scripts/dev-env.ps1; ctest --test-dir build/debug -N" | Select-Object -Last 1`
  → `Total Tests: <N_départ + 26>` ; et la commande ciblée sur les 26 tests du lot : 26 tests, chacun `Passed` (un
  `QSKIP` : Qt Test le compte réussi), chaque fichier contenant la ligne `QSKIP("réservé : L5-T<n>")`.

**Preuve visuelle.** Aucune (rien de visible).

**Interdits.** Aucune icône, aucun libellé, aucun nom de commande de Promethean (libellés écrits par nous).
Aucun corps réel : un corps talon rend une valeur neutre et porte `// Talon L5-T1 : rempli par L5-T<n>.`. Ne
modifier aucune signature publique des lots 3, 4, 7 et L0-S (seuls ajouts : `parkSession` / `resumeSession`).
Pas de `QSKIP` sans tâche nommée.

**Terminé quand.** Build 0 avertissement ; N = N_départ + 26 ; les deux tests ciblés `Passed` ; `format.ps1 -Check`
propre ; `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : départ.** Relever N_départ (`ctest -N`, dernière ligne) et le noter dans le rapport.
- [ ] **Étape 2 : modèle.** Dans `model.h`, ajouter à `ObjectCommon` (après `locked`) :
  ```cpp
      bool dragCopy = false; // F-587 : chaque glissement dépose une copie, l'original reste
  ```
  et, avant `struct Connector` :
  ```cpp
  // Extrémité d'un connecteur (F-245).
  enum class ConnectorCap : std::uint8_t
  {
      None = 0,
      Round = 1,
      Arrow = 2,
  };
  ```
  puis, à la fin de `struct Connector` :
  ```cpp
      std::uint8_t style = 0; // rang dans le panneau des connecteurs (0 : ligne simple), D11
      ConnectorCap capA = ConnectorCap::None;
      ConnectorCap capB = ConnectorCap::None;
  ```
  Dans `nativejson.cpp` : écrire `"dragCopy": true` seulement s'il est vrai ; `"style"`, `"capA"`, `"capB"`
  (`"none"`, `"round"`, `"arrow"`) seulement s'ils diffèrent du défaut ; les lire avec ces défauts. Test rouge puis
  vert dans `tst_formats_nativejson.cpp` (fabriques et aides : celles que ce test emploie déjà, à revérifier) :
  ```cpp
  void TestNativeJson::lot5FieldsRoundTrip()
  {
      core::Document document = blankDocument(1024, 768);
      core::PageObject shape = rectangleObject({100, 100, 200, 100});
      shape.common.dragCopy = true;
      core::Connector link;
      link.firstEnd = {0, 0};
      link.secondEnd = {50, 50};
      link.style = 3;
      link.capA = core::ConnectorCap::Arrow;
      link.capB = core::ConnectorCap::Round;
      document.pages[0].objects.push_back(shape);
      document.pages[0].objects.push_back(core::PageObject{link});
      const core::Document back = roundTrip(document);
      QVERIFY(back.pages[0].objects[0].common.dragCopy);
      const auto* c = std::get_if<core::Connector>(&back.pages[0].objects[1]);
      QVERIFY(c);
      QCOMPARE(int(c->style), 3);
      QCOMPARE(c->capA, core::ConnectorCap::Arrow);
      QCOMPARE(c->capB, core::ConnectorCap::Round);
      QCOMPARE(fingerprint(back), fingerprint(document));
  }
  ```
- [ ] **Étape 3 : commandes.** Ajouter à `actions/data/commands.json`, dans cet ordre (libellés à nous) :
  ```json
  { "id": "edit.cut", "label": "Couper", "icon": "couper", "shortcut": "Ctrl+X" },
  { "id": "edit.copy", "label": "Copier", "icon": "copier", "shortcut": "Ctrl+C" },
  { "id": "edit.paste", "label": "Coller", "icon": "coller", "shortcut": "Ctrl+V" },
  { "id": "edit.paste-onto", "label": "Coller sur l'objet" },
  { "id": "edit.duplicate", "label": "Dupliquer", "icon": "dupliquer", "shortcut": "Ctrl+D" },
  { "id": "object.menu", "label": "Menu de l'objet", "icon": "menu-objet" },
  { "id": "object.group", "label": "Grouper ou dégrouper", "icon": "grouper", "shortcut": "Ctrl+G", "checkable": true },
  { "id": "object.lock", "label": "Verrouiller", "icon": "verrouiller", "shortcut": "Ctrl+Shift+L", "checkable": true },
  { "id": "object.hide", "label": "Masquer", "icon": "masquer", "shortcut": "Ctrl+Shift+I", "checkable": true },
  { "id": "object.drag-copy", "label": "Glisser une copie", "icon": "glisser-copie", "shortcut": "Ctrl+Shift+J", "checkable": true },
  { "id": "object.shape-points", "label": "Modifier les points de la forme", "icon": "points-forme", "checkable": true },
  { "id": "arrange.front", "label": "Mettre au premier plan" },
  { "id": "arrange.back", "label": "Mettre à l'arrière-plan" },
  { "id": "arrange.forward", "label": "Avancer d'un cran" },
  { "id": "arrange.backward", "label": "Reculer d'un cran" },
  { "id": "arrange.layer-top", "label": "Vers la couche du dessus" },
  { "id": "arrange.layer-middle", "label": "Vers la couche du milieu" },
  { "id": "arrange.layer-bottom", "label": "Vers la couche du dessous" },
  { "id": "align.left", "label": "Bords gauches" },
  { "id": "align.center-x", "label": "Centres horizontaux" },
  { "id": "align.right", "label": "Bords droits" },
  { "id": "align.top", "label": "Bords hauts" },
  { "id": "align.center-y", "label": "Centres verticaux" },
  { "id": "align.bottom", "label": "Bords bas" },
  { "id": "align.width", "label": "Même largeur" },
  { "id": "align.height", "label": "Même hauteur" },
  { "id": "align.angle", "label": "Même angle" },
  { "id": "align.all", "label": "Tout aligner sur le premier" },
  { "id": "transform.original-size", "label": "Taille d'origine" },
  { "id": "transform.fit-width", "label": "Ajuster à la largeur de la page" },
  { "id": "transform.fit-height", "label": "Ajuster à la hauteur de la page" },
  { "id": "transform.fit-page", "label": "Ajuster à la page" },
  { "id": "transform.mirror-x", "label": "Copie miroir verticale" },
  { "id": "transform.mirror-y", "label": "Copie miroir horizontale" },
  { "id": "transform.flip-x", "label": "Retourner de haut en bas", "shortcut": "Ctrl+Shift+X" },
  { "id": "transform.flip-y", "label": "Retourner de gauche à droite", "shortcut": "Ctrl+Shift+Y" },
  { "id": "transform.origin-flip-x", "label": "Symétrie par l'axe horizontal de l'origine" },
  { "id": "transform.origin-flip-y", "label": "Symétrie par l'axe vertical de l'origine" },
  { "id": "tool.fill", "label": "Pot de peinture", "icon": "pot-peinture", "shortcut": "Ctrl+F" },
  { "id": "tool.connector", "label": "Connecteur", "icon": "connecteur", "shortcut": "Ctrl+Shift+C" },
  { "id": "connector.style", "label": "Style du connecteur", "argument": "number" },
  { "id": "view.pen-modifiers", "label": "Modification du stylo", "icon": "modification-stylo", "checkable": true },
  { "id": "pen.shape", "label": "Tracé du stylo", "argument": "text" },
  { "id": "insert.image", "label": "Image…", "icon": "image", "argument": "url", "optional": true },
  { "id": "insert.media", "label": "Média…", "icon": "media", "shortcut": "Ctrl+M", "argument": "url", "optional": true },
  { "id": "tab.next", "label": "Paperboard suivant", "shortcut": "Ctrl+Tab" },
  { "id": "tab.previous", "label": "Paperboard précédent", "shortcut": "Ctrl+Shift+Tab" },
  { "id": "view.tabs", "label": "Onglets des paperboards", "icon": "onglets", "argument": "text" },
  { "id": "export.page-image", "label": "Exporter la page en image…", "icon": "exporter", "shortcut": "Ctrl+Shift+E", "argument": "url", "optional": true },
  { "id": "export.pdf", "label": "Exporter en PDF…", "argument": "url", "optional": true },
  { "id": "file.import-pdf", "label": "PDF…", "icon": "importer", "argument": "url", "optional": true },
  { "id": "file.import-powerpoint-images", "label": "Présentation en images…", "argument": "url", "optional": true, "platforms": ["windows"] },
  { "id": "file.print", "label": "Imprimer…", "icon": "imprimer", "shortcut": "Ctrl+P" }
  ```
  `pen.shape` prend `free`, `horizontal`, `vertical`, `square`, `rectangle`, `circle`, `ellipse`, `arc`, `polyline`,
  `arrow`, `zigzag` (D12) ; `view.tabs` prend `top`, `bottom`, `hidden` (D14) ; `connector.style` un rang de 0 à 13.
  Un raccourci déjà pris par un lot antérieur (A21) est retiré ici et l'écart noté au rapport. Si `"argument":
  "text"` n'existe pas (A12), employer la forme que L0-S4 a posée pour `equation.edit`.
- [ ] **Étape 4 : disposition.** Dans `actions/data/layout.json` : menu Éditer, après les entrées des lots 3 et 4 :
  `edit.cut`, `edit.copy`, `edit.paste`, `edit.duplicate`, `"-"`, sous-menus `>transform` (« Transformer » :
  `object.grow`, `object.shrink`, `transform.original-size`, `transform.fit-width`, `transform.fit-height`,
  `transform.fit-page`, `"-"`, `transform.mirror-x`, `transform.mirror-y`, `transform.flip-x`, `transform.flip-y`,
  `transform.origin-flip-x`, `transform.origin-flip-y`, `"-"`, `>align`), `>align` (« Aligner » : les 10 `align.*`
  dans l'ordre de l'étape 3), `>arrange` (« Organiser » : `arrange.front`, `arrange.back`, `arrange.forward`,
  `arrange.backward`, `"-"`, les trois `arrange.layer-*`) ; menu Insérer : `insert.image`, `insert.media`,
  `tool.connector` ; menu Affichage : `view.pen-modifiers`, `view.tabs` ; menu Fichier : sous-menus `>import`
  (« Importer » : `file.import-pdf`, `file.import-powerpoint-images`) et `>export` (« Exporter » : `export.pdf`,
  `export.page-image`), puis `file.print` ; palette : `tool.fill`, `tool.connector` à la suite des outils du lot 4 ;
  `objectBar` (F-521), à la suite des boutons des lots 3 et 4, à leur rang : `object.menu`, `object.group`,
  `arrange.front`, `arrange.back`, `edit.duplicate`, `object.shape-points` ; nouvelle liste `objectMenu` (F-770,
  D20) :
  ```json
  "objectMenu": ["object.shape-points", "-", "edit.cut", "edit.copy", "edit.paste-onto", "edit.duplicate",
                 "edit.delete", "-", ">transform", ">arrange", "-", "object.lock", "object.group", "object.hide",
                 "object.drag-copy", "-", "recognition.convert-to-text", "recognition.convert-to-shape"]
  ```
  `objectMenu` se lit par la même règle que `contextMenu` (A12) ; s'il faut l'ajouter au type de disposition,
  `actions/include/actcraft/actions/layout.h` et `actions/src/layout.cpp` entrent dans les fichiers de la tâche
  (écart à noter au rapport et dans la table par l'orchestrateur). Tests rouges puis verts dans
  `tests/tst_actions_layout.cpp` (fabriques de registre et de disposition : celles du test existant) :
  ```cpp
  static const QStringList kLot5{
      "edit.cut", "edit.copy", "edit.paste", "edit.paste-onto", "edit.duplicate", "object.menu", "object.group",
      "object.lock", "object.hide", "object.drag-copy", "object.shape-points", "arrange.front", "arrange.back",
      "arrange.forward", "arrange.backward", "arrange.layer-top", "arrange.layer-middle", "arrange.layer-bottom",
      "align.left", "align.center-x", "align.right", "align.top", "align.center-y", "align.bottom", "align.width",
      "align.height", "align.angle", "align.all", "transform.original-size", "transform.fit-width",
      "transform.fit-height", "transform.fit-page", "transform.mirror-x", "transform.mirror-y", "transform.flip-x",
      "transform.flip-y", "transform.origin-flip-x", "transform.origin-flip-y", "tool.fill", "tool.connector",
      "connector.style", "view.pen-modifiers", "pen.shape", "insert.image", "insert.media", "tab.next",
      "tab.previous", "view.tabs", "export.page-image", "export.pdf", "file.import-pdf",
      "file.import-powerpoint-images", "file.print"};

  void TestLayout::lot5EntriesDeclared()
  {
      QCOMPARE(kLot5.size(), 53);
      const auto registry = loadRegistry();
      QSet<QString> shortcuts;
      for (const auto& command : registry.all()) {
          if (command.shortcut.isEmpty())
              continue;
          QVERIFY2(!shortcuts.contains(command.shortcut), qPrintable(command.shortcut));
          shortcuts.insert(command.shortcut);
      }
      for (const QString& id : kLot5)
          QVERIFY2(registry.find(id) != nullptr, qPrintable(id));
  }

  void TestLayout::objectMenuOrder()
  {
      const auto layout = loadLayout();
      const QStringList expected{"object.shape-points", "-", "edit.cut", "edit.copy", "edit.paste-onto",
                                 "edit.duplicate", "edit.delete", "-", ">transform", ">arrange", "-", "object.lock",
                                 "object.group", "object.hide", "object.drag-copy", "-",
                                 "recognition.convert-to-text", "recognition.convert-to-shape"};
      QCOMPARE(layout.objectMenu, expected);
  }
  ```
  `kLot5` est aussi la liste de L5-T2 (même ordre).
- [ ] **Étape 5 : icônes.** 23 SVG à nous sur la grille de 24 px, dans le style du jeu du lot 1 (traits de 2 px,
  `currentColor`), noms de la ligne de la table ; ajoutés au bloc `RESOURCES` de `ui/CMakeLists.txt`. Le test du
  thème du lot 1 (`iconsAreOurSvgOnA24Grid`) les couvre s'il tire ses données du dossier ; sinon ajouter les 23 lignes
  de données à `tests/tst_ui_theme.cpp` (fichier ajouté à la tâche, écart au rapport).
- [ ] **Étape 6 : CMake et modules.** Racine : `add_subdirectory(export)` puis `add_subdirectory(print)` après
  `render`, avant `tools` ; options :
  ```cmake
  option(ACTCRAFT_PRINT "Impression par QPrinter (Windows)" ${WIN32})
  option(ACTCRAFT_IMPORT_PDF "Import PDF par Qt PDF" OFF) # rendue automatique par L5-T22
  ```
  `export/CMakeLists.txt` et `print/CMakeLists.txt` suivent la forme exacte de `actcraft_add_module`
  (`cmake/ActCraftModule.cmake`, lue avant d'écrire) : `export` = sources `src/{pageexport,pdfexport,pdfimport,
  officeconvert}.cpp`, liens `actcraft_render actcraft_formats actcraft_core Qt6::Gui Qt6::Concurrent` (si
  `Qt6::Concurrent` n'est pas déjà trouvé par la racine, l'ajouter au `find_package` de la racine) ; `print` =
  `src/printer.cpp`, liens `actcraft_render actcraft_core Qt6::Gui` (Qt PrintSupport ajouté par L5-T27) ; `ui` lie
  `actcraft_export` et `actcraft_print`. Les sources neuves de `core/`, `formats/`, `render/`, `tools/`, `ui/` entrent
  dans leurs listes. `tests/export.cmake` et `tests/print.cmake` inclus par `tests/CMakeLists.txt` ; les 26 tests
  déclarés par `actcraft_add_test` dans le `.cmake` de leur module.
- [ ] **Étape 7 : en-têtes figés.** Écrire mot pour mot les en-têtes de l'annexe A (fin de ce plan), dans l'ordre :
  `clipboardedits.h`, `arrange.h`, `alignment.h`, `objectflags.h`, `fill.h`, `shapepoints.h`, `connectors.h`,
  `penmodifiers.h`, `printlayout.h`, `formats/clipboard.h`, `render/imagefill.h`, les trois outils,
  `export/{pageexport,pdfexport,pdfimport,officeconvert}.h`, `print/printer.h`, `ui/{objectediting,insertion,
  documenttabs,fileexchange,printcontroller}.h`, et l'ajout à `documentcontroller.h`.
- [ ] **Étape 8 : corps talons et tests réservés.** Chaque `.cpp` neuf définit ses fonctions avec une valeur neutre
  (`{}`, `nullptr`, `false`, objet par défaut) précédée de `// Talon L5-T1 : rempli par L5-T<n>.`. Chaque
  `tests/tst_*.cpp` du lot :
  ```cpp
  #include <QTest>

  class TestReserved : public QObject
  {
      Q_OBJECT
  private slots:
      void reserved() { QSKIP("réservé : L5-T<n>"); }
  };

  QTEST_GUILESS_MAIN(TestReserved) // QTEST_MAIN pour les tests ui, tools et print
  #include "tst_<nom>.moc"
  ```
- [ ] **Étape 9 : preuves.** Commande ciblée (`^(tst_actions_layout|tst_formats_nativejson)$`), `ctest -N`
  (N = N_départ + 26), les 26 talons `Passed`, `format.ps1 -Check`.
- [ ] **Étape 10 : commit.** `git add` des chemins de la table (liste exacte), `NOMS OK`, puis :
  ```powershell
  git -C '<wt>' commit -m "Lot 5 : réservation des commandes, de la disposition, des modules export et print, des 26 tests et des en-têtes figés

  Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

### Tâche L5-T2 — Réservation II : coquille QML

<a id="tache-l5-t2"></a>

**Objectif.** Poser une fois les fichiers QML chauds : les objets `appEditing` (`ObjectEditing`), `appInsertion`
(`Insertion`), `appTabs` (`DocumentTabs`), `appExchange` (`FileExchange`), `appPrint` (`PrintController`) instanciés
dans `Main.qml` et reliés à `appDocument` et `appTools` ; un gestionnaire par commande du lot qui appelle la méthode
C++ correspondante (corps talons jusqu'à leur tâche) ; les emplacements `ObjectEditMenu` (ouvert par `object.menu`
et par le clic droit sur un objet), `PenModifierBar` (à côté de la palette, visible avec `view.pen-modifiers`),
`ConnectorPanel` (visible avec l'outil connecteur), `DocumentTabBar` (sous la ligne supérieure), `FileExchangeActions`
(dialogues de fichiers d'export et d'import), `PrintDialog` et `SheetPreview` ; chaque fichier neuf est un talon
minimal (`Item { id: root; visible: false }` et ses `required property`).

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Dans `tests/tst_ui_shell.cpp` (aides `handledCommandIds`, `loadMainWindow`,
`findByObjectName` : celles de L0-S9, à revérifier) :
```cpp
void TestShell::everyLot5CommandHasAHandler()
{
    const QStringList lot5{ /* la liste kLot5 de L5-T1, étape 4, recopiée mot pour mot */ };
    QCOMPARE(lot5.size(), 53);
    const QStringList handled = handledCommandIds();
    for (const QString& id : lot5)
        QVERIFY2(handled.contains(id), qPrintable(id));
}

void TestShell::lot5SlotsExist()
{
    auto window = loadMainWindow();
    for (const char* name : {"appEditing", "appInsertion", "appTabs", "appExchange", "appPrint", "objectEditMenu",
                             "penModifierBar", "connectorPanel", "documentTabBar", "fileExchangeActions",
                             "printDialog", "sheetPreview"})
        QVERIFY2(findByObjectName(window.get(), name) != nullptr, name);
}
```
`shellLoadsWithoutQmlWarning` (existant) reste vert. Commande ciblée, motif `^(tst_ui_shell)$`.

**Preuve visuelle.** `premiers-pas` hors écran (forme bornée du profil) : `CAPTURES OK`, aucune ligne en `ECART` ; la
capture montre la même interface qu'avant (`DocumentTabBar` invisible tant qu'un seul document est ouvert).

**Interdits.** Aucun comportement réel (les gestionnaires appellent des corps talons). Ordre des couches de
`PageArea.qml` (A14) inchangé. Aucun autre QML réécrit hors de l'ajout d'instances et de gestionnaires.

**Terminé quand.** `tst_ui_shell` `Passed` (fonctions neuves comprises) ; `premiers-pas` `CAPTURES OK` ; `NOMS OK` ;
commit.

**Étapes.**
- [ ] **Étape 1 : tests rouges** ci-dessus ; commande ciblée → rouge.
- [ ] **Étape 2 : instances.** Dans `Main.qml`, après `appTools` :
  ```qml
  ObjectEditing { id: appEditing; objectName: "appEditing"; document: appDocument; tools: appTools }
  Insertion { id: appInsertion; objectName: "appInsertion"; document: appDocument }
  DocumentTabs { id: appTabs; objectName: "appTabs"; document: appDocument }
  FileExchange { id: appExchange; objectName: "appExchange"; document: appDocument; tabs: appTabs }
  PrintController { id: appPrint; objectName: "appPrint"; document: appDocument }
  ```
- [ ] **Étape 3 : gestionnaires.** À la suite des gestionnaires existants (forme de L0-S9, A14) : `edit.cut` →
  `appEditing.cut()` ; `edit.copy` → `copy()` ; `edit.paste` → `paste()` ; `edit.paste-onto` → `pasteOnto()` ;
  `edit.duplicate` → `duplicate()` ; `object.group` → `toggleGroup()` ; `object.lock` / `object.hide` /
  `object.drag-copy` → `toggleFlag("lock" | "hide" | "drag-copy")` ; `arrange.<x>` → `arrange("<x>")` ;
  `align.<x>` → `align("<x>")` ; `transform.<x>` → `transform("<x>")` ; `object.menu` →
  `objectEditMenu.popupAtSelection()` ; `object.shape-points`, `tool.fill`, `tool.connector`, `pen.shape` →
  `appTools.activate("select-points" | "fill" | "connector" | "pen-shape")` (argument passé à l'outil par L5-T17) ;
  `connector.style` → gestionnaire vide commenté `// L5-T17` ; `view.pen-modifiers` → bascule de
  `penModifierBar.visible` ; `insert.image` / `insert.media` → `appInsertion.insertImage(url)` /
  `insertMedia(url)` si l'argument est donné, sinon `fileExchangeActions.chooseImage()` / `chooseMedia()` ;
  `tab.next` / `tab.previous` → `appTabs.next()` / `previous()` ; `view.tabs` → `appTabs.position = argument` ;
  `export.*`, `file.import-*` → `appExchange.<méthode>(url)` ou le dialogue de `fileExchangeActions` ; `file.print`
  → `printDialog.open()` si `Qt.platform.os === "windows"`, sinon `appExchange.printThroughPdf()`.
- [ ] **Étape 4 : talons QML** des sept fichiers neufs, enregistrés au bloc `QML_FILES` ; `ObjectBar.qml` lit la
  liste `objectBar` complète (boutons du lot inertes jusqu'à L5-T18) ; `TopBar.qml` réserve la place de
  `DocumentTabBar`.
- [ ] **Étape 5 : preuves** (commande ciblée, captures `premiers-pas`), `format.ps1 -Check`.
- [ ] **Étape 6 : commit** des chemins de la table, `NOMS OK`, message « Lot 5 : coquille QML des fonctions d'édition
  et de fichiers » terminé par la ligne `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.

### Tâche L5-T3 — `formats` : presse-papiers d'objets

<a id="tache-l5-t3"></a>

**Objectif.** Remplir `formats/src/clipboard.cpp` (contrat figé : annexe A, `clipboard.h`). La charge utile du
presse-papiers transporte des objets **sans perte** (tous les types, types inconnus compris, L2-C8) et les ressources
qu'ils citent (images, médias), pour coller dans un autre onglet ou une autre instance d'ActCraft. Le patron évident
(sérialiser chaque objet à la main) est le mauvais : **réutiliser le code d'objet du format natif** (`nativejson.cpp`,
fonctions d'écriture et de lecture d'un objet ; si elles sont internes au fichier, les exposer dans un en-tête interne
`formats/src/nativeobjects.h` — fichier ajouté à la tâche, écart au rapport) pour que le presse-papiers et
l'enregistrement ne divergent jamais.

**Fichiers.** `formats/src/clipboard.cpp`, `tests/tst_formats_clipboard.cpp` (et, au besoin, `formats/src/nativeobjects.h`).

**Test et commande bornée.** Commande ciblée, motif `^(tst_formats_clipboard)$`. Fonctions :
```cpp
void TestClipboard::roundTripKeepsEveryKind()
{
    // Un objet de chaque type d'ObjectKind (Shape, Image, Stroke, Line, Text, Media, Group, Connector, Unknown).
    formats::ClipboardPayload payload = sampleOfEveryKind(); // aide du test : fabrique à nous
    const auto back = formats::decodeClipboard(formats::encodeClipboard(payload));
    QVERIFY(back);
    QCOMPARE(back->objects.size(), payload.objects.size());
    for (std::size_t i = 0; i < payload.objects.size(); ++i)
        QCOMPARE(objectFingerprint(back->objects[i]), objectFingerprint(payload.objects[i]));
}
void TestClipboard::resourcesTravelWithObjects()
{
    core::Document document = documentWithImage(); // une ImageObject et sa ressource PNG
    const auto resources = formats::resourcesFor(document, document.pages[0].objects);
    QCOMPARE(resources.size(), std::size_t(1));
    formats::ClipboardPayload payload{document.pages[0].objects, resources};
    const auto back = formats::decodeClipboard(formats::encodeClipboard(payload));
    QVERIFY(back);
    QCOMPARE(back->resources, resources);
}
void TestClipboard::groupMembersTravelTogether()
{
    // Le format ne remappe rien : identifiants et références du groupe identiques après l'aller-retour.
    formats::ClipboardPayload payload = groupOfTwo();
    const auto back = formats::decodeClipboard(formats::encodeClipboard(payload));
    QVERIFY(back);
    const auto* group = std::get_if<core::Group>(&back->objects.back());
    QVERIFY(group);
    QCOMPARE(group->members, (std::vector<core::ObjectId>{payload.objects[0].common.id, payload.objects[1].common.id}));
}
void TestClipboard::foreignPayloadRejected()
{
    QVERIFY(!formats::decodeClipboard(QByteArray("{\"format\":\"autre/1\",\"objects\":[]}")));
    QVERIFY(!formats::decodeClipboard(QByteArray("pas du json")));
    // Ressource citée absente, ou octets qui ne correspondent pas à leur clé SHA-256 : refusée.
    QVERIFY(!formats::decodeClipboard(payloadWithWrongResourceKey()));
}
```
Mutation : dans `encodeClipboard`, ne plus écrire les ressources → `resourcesTravelWithObjects` rouge ; remise par
`git -C <wt> checkout -- formats/src/clipboard.cpp` ; vert.

**Preuve visuelle.** Aucune.

**Interdits.** Aucun champ d'objet écrit autrement que par le code du format natif. Ressources en base64 dans le JSON
(pas de chemin de fichier). Pas de QtGui dans `formats/` (CMake le refuse).

**Terminé quand.** 4 fonctions en `PASS` ; mutation rouge puis remise ; `tst_formats_nativejson` encore vert ;
`NOMS OK` ; commit « Lot 5 : presse-papiers d'objets sans perte (formats) ».

**Étapes.**
- [ ] **Étape 1** : tests rouges ci-dessus (aides de fabrique écrites dans le test) ; commande ciblée → rouge.
- [ ] **Étape 2** : `encodeClipboard` : objet JSON `{"format": "actcraft-objects/1", "objects": [ … objets écrits par
  le code natif … ], "resources": { clé: base64 }}`, `QJsonDocument::Compact`.
- [ ] **Étape 3** : `decodeClipboard` : contrôle du format, lecture des objets par le code natif, contrôle de chaque
  ressource (SHA-256 des octets = clé, `QCryptographicHash::Sha256`), refus sinon ; `resourcesFor` parcourt les clés
  citées (`ImageObject`, `Media.placeholder`, `Media.media`).
- [ ] **Étape 4** : commande ciblée → vert ; mutation ; `format.ps1 -Check`.
- [ ] **Étape 5** : commit (chemins explicites, `NOMS OK` après `git add`).

### Tâche L5-T4 — `core` : copies prêtes à coller

<a id="tache-l5-t4"></a>

**Objectif.** Remplir `core/src/clipboardedits.cpp` (annexe A, `clipboardedits.h`). Point délicat, d'où le
`concepteur-critique` : `ReplaceObjectsCommand` (A6) **ignore** l'identifiant porté par un ajout et tire les siens au
premier `redo()` ; un groupe collé cite ses membres et un connecteur ses objets **par identifiant**. Le patron évident
(poser des identifiants neufs avant d'ajouter) casse donc les références. Contrat : `preparePaste` remplace les
identifiants par des rangs provisoires 1..n et remappe les références vers ces rangs ; `PasteObjectsCommand` exécute un
`ReplaceObjectsCommand({}, prepared)` puis réécrit, dans la page, les références de rang en identifiant réel
(`addedIds()[rang - 1]`) ; au redo suivant, mêmes identifiants (ils sont gardés par la commande intérieure), même
réécriture. Les références ne changent aucun pixel : aucun `Change` en plus.

**Fichiers.** `core/src/clipboardedits.cpp`, `tests/tst_core_clipboardedits.cpp`.

**Test et commande bornée.** Commande ciblée, motif `^(tst_core_clipboardedits)$`. Fonctions (document de test : une
page 1024 × 768, aides `addRect`, `addStroke`, `addGroup`, `addConnector` écrites dans le test) :
```cpp
void TestClipboardEdits::pasteOffsetsBy20()
{
    core::Document d = blank();
    const auto a = addRect(d, {100, 100, 50, 50});
    auto prepared = core::preparePaste(core::closureForCopy(d.pages[0], {a}), core::kPasteOffset, core::kPasteOffset);
    core::UndoStack stack;
    stack.push(std::make_unique<core::PasteObjectsCommand>(0, std::move(prepared)), d);
    QCOMPARE(d.pages[0].objects.size(), std::size_t(2));
    QCOMPARE(boundsOf(d.pages[0].objects[1]).topLeft(), core::PointF(120, 120));
    stack.undo(d);
    QCOMPARE(d.pages[0].objects.size(), std::size_t(1));
}
void TestClipboardEdits::duplicateOffsetsBy10AndSelectsCopy()
{
    core::Document d = blank();
    const auto a = addRect(d, {100, 100, 50, 50});
    auto command = core::makeDuplicateCommand(d.pages[0], 0, {a});
    core::PasteObjectsCommand* raw = command.get();
    core::UndoStack stack;
    stack.push(std::move(command), d);
    QCOMPARE(raw->addedIds().size(), std::size_t(1));
    QCOMPARE(boundsOf(objectById(d, raw->addedIds()[0])).topLeft(), core::PointF(110, 110));
}
void TestClipboardEdits::copiesAreUnlockedAndUnnamed()
{
    core::Document d = blank();
    const auto a = addRect(d, {0, 0, 10, 10});
    objectById(d, a).common.locked = true;
    objectById(d, a).common.name = "étiquette";
    const auto copies = core::preparePaste(core::closureForCopy(d.pages[0], {a}), 0, 0);
    QVERIFY(!copies[0].common.locked);
    QVERIFY(copies[0].common.name.empty());
}
void TestClipboardEdits::groupReferencesRemapped()
{
    core::Document d = blank();
    const auto a = addRect(d, {0, 0, 10, 10});
    const auto b = addRect(d, {20, 0, 10, 10});
    const auto g = addGroup(d, {a, b});
    core::UndoStack stack;
    stack.push(std::make_unique<core::PasteObjectsCommand>(0, core::preparePaste(core::closureForCopy(d.pages[0], {g}), 20, 20)), d);
    const auto& group = std::get<core::Group>(d.pages[0].objects.back());
    QCOMPARE(group.members.size(), std::size_t(2));
    for (core::ObjectId member : group.members) {
        QVERIFY(member != a && member != b);
        QVERIFY(hasObject(d, member));
    }
    stack.undo(d);
    stack.redo(d); // mêmes identifiants, références toujours justes
    const auto& again = std::get<core::Group>(d.pages[0].objects.back());
    QVERIFY(hasObject(d, again.members[0]) && hasObject(d, again.members[1]));
}
void TestClipboardEdits::connectorEndsRemapped()
{
    // Connecteur entre a et b, copié avec a seulement : objectA remappé vers la copie de a, objectB remis à 0.
    core::Document d = blank();
    const auto a = addRect(d, {0, 0, 10, 10});
    const auto b = addRect(d, {100, 0, 10, 10});
    const auto c = addConnector(d, a, b);
    core::UndoStack stack;
    stack.push(std::make_unique<core::PasteObjectsCommand>(0, core::preparePaste(core::closureForCopy(d.pages[0], {a, c}), 20, 20)), d);
    const auto& link = std::get<core::Connector>(d.pages[0].objects.back());
    QVERIFY(link.objectA != a && hasObject(d, link.objectA));
    QCOMPARE(link.objectB, core::ObjectId(0));
}
void TestClipboardEdits::pasteOnObjectKeepsItsLayer()
{
    core::Document d = blank();
    const auto a = addRect(d, {0, 0, 10, 10}, core::Layer::Top);
    const auto target = addRect(d, {200, 200, 100, 100}, core::Layer::Bottom);
    const auto copies = core::preparePasteOnto(core::closureForCopy(d.pages[0], {a}), objectById(d, target));
    QCOMPARE(copies[0].common.layer, core::Layer::Bottom);
    QCOMPARE(boundsOf(copies[0]).center(), core::PointF(250, 250));
}
void TestClipboardEdits::cutIsOneStep()
{
    core::Document d = blank();
    const auto a = addRect(d, {0, 0, 10, 10});
    const auto b = addStroke(d);
    core::UndoStack stack;
    stack.push(core::makeCutCommand(d.pages[0], 0, {a, b}), d);
    QVERIFY(d.pages[0].objects.empty());
    stack.undo(d);
    QCOMPARE(d.pages[0].objects.size(), std::size_t(2));
}
```
Noms des membres de `UndoStack` (`push`, `undo`, `redo`) et des aides de rect : ceux de `core/undo.h` et du modèle (A6,
A20, à revérifier). Mutation : dans `PasteObjectsCommand::redo`, sauter la réécriture des références →
`groupReferencesRemapped` rouge ; remise par `git -C <wt> checkout -- core/src/clipboardedits.cpp` ; vert.

**Interdits.** Aucune autre commande d'ajout que `ReplaceObjectsCommand` (via `PasteObjectsCommand`) ; aucun
`allocateId` appelé ici ; `makeCutCommand` rend un `ReplaceObjectsCommand(page, ids élargis par expandGroups, {})` ;
aucune modification de `replace.cpp`.

**Terminé quand.** 7 fonctions en `PASS` ; mutation rouge puis remise ; `tst_core_replace` encore vert ; commit
« Lot 5 : copies prêtes à coller, références remappées (core) ».

**Étapes.**
- [ ] **Étape 1** : tests rouges ; commande ciblée → rouge.
- [ ] **Étape 2** : `closureForCopy` : `expandGroups` (livré par L5-T5, vague V2, avant cette tâche) ; ordre de la
  page ; connecteurs : lien gardé si l'objet est emporté, sinon 0.
- [ ] **Étape 3** : `preparePaste` : ordre membres, groupes (du plus intérieur au plus extérieur), connecteurs ;
  identifiants → rangs ; références remappées ; `translate` de chaque transformation de (dx, dy) ; nom vidé, verrou ôté.
  `preparePasteOnto` : centre de l'union sur le centre de la cible, couche de la cible.
- [ ] **Étape 4** : `PasteObjectsCommand` : intérieur `ReplaceObjectsCommand(page, {}, prepared)` ; après
  `m_inner->redo`, `m_ids = addedIds()` puis réécriture des références dans la page ; `undo` = `m_inner->undo`.
  `makeDuplicateCommand` = `preparePaste(closure, kDuplicateOffset, kDuplicateOffset)`.
- [ ] **Étape 5** : vert, mutation, format, commit.

### Tâche L5-T5 — `core` : organiser, couches et groupes

<a id="tache-l5-t5"></a>

**Objectif.** Remplir `core/src/arrange.cpp` (annexe A, `arrange.h`) selon D4 et D5 (`view.py:780-871`).

**Fichiers.** `core/src/arrange.cpp`, `tests/tst_core_arrange.cpp`.

**Test et commande bornée.** Commande ciblée, motif `^(tst_core_arrange)$`. Fonctions (page : a, b, c, d sur la couche
du milieu dans cet ordre, e sur celle du dessus) :
```cpp
void TestArrange::forwardSwapsOneStep()
{
    auto d = pageABCDE();
    QCOMPARE(core::arrangedOrder(d.pages[0], {id(d, 'b')}, core::ArrangeMove::Forward), ids(d, "acbde"));
}
void TestArrange::frontKeepsRelativeOrder()
{
    auto d = pageABCDE();
    QCOMPARE(core::arrangedOrder(d.pages[0], {id(d, 'a'), id(d, 'b')}, core::ArrangeMove::Front), ids(d, "cdabe"));
    QCOMPARE(core::arrangedOrder(d.pages[0], {id(d, 'd')}, core::ArrangeMove::Back), ids(d, "dabce"));
}
void TestArrange::layerChangeGoesOnTop()
{
    auto d = pageABCDE();
    core::UndoStack stack;
    stack.push(std::make_unique<core::SetLayerCommand>(0, std::vector{id(d, 'a')}, core::Layer::Top), d);
    QCOMPARE(objectById(d, id(d, 'a')).common.layer, core::Layer::Top);
    QCOMPARE(d.pages[0].objects.back().common.id, id(d, 'a')); // au sommet de la couche du dessus
    stack.undo(d);
    QCOMPARE(ordered(d), ids(d, "abcde"));
}
void TestArrange::groupingIsASingleStep()
{
    auto d = pageABCDE();
    core::UndoStack stack;
    stack.push(core::makeGroupCommand(d.pages[0], 0, {id(d, 'a'), id(d, 'e')}), d);
    const auto& g = std::get<core::Group>(d.pages[0].objects.back());
    QCOMPARE(g.members.size(), std::size_t(2));
    QCOMPARE(d.pages[0].objects.back().common.layer, core::Layer::Middle); // la plus basse des membres (D4)
    QCOMPARE(core::topLevelGroupOf(d.pages[0], id(d, 'a')), d.pages[0].objects.back().common.id);
    stack.undo(d);
    QCOMPARE(d.pages[0].objects.size(), std::size_t(5));
    QVERIFY(core::makeGroupCommand(d.pages[0], 0, {id(d, 'a')}) == nullptr);
}
void TestArrange::ungroupRestoresInnerGroups()
{
    auto d = pageABCDE();
    core::UndoStack stack;
    stack.push(core::makeGroupCommand(d.pages[0], 0, {id(d, 'a'), id(d, 'b')}), d);
    const auto inner = d.pages[0].objects.back().common.id;
    stack.push(core::makeGroupCommand(d.pages[0], 0, {inner, id(d, 'c')}), d);
    const auto outer = d.pages[0].objects.back().common.id;
    stack.push(core::makeUngroupCommand(d.pages[0], 0, {outer}), d);
    QVERIFY(hasObject(d, inner));
    QVERIFY(!hasObject(d, outer));
    QCOMPARE(core::expandGroups(d.pages[0], {inner}), (std::vector{id(d, 'a'), id(d, 'b'), inner}));
}
void TestArrange::reorderChangesSeenByCache()
{
    auto d = pageABCDE();
    core::Changes changes;
    core::ReorderObjectsCommand command(0, ids(d, "bacde"));
    command.redo(d, changes);
    // a et b changent de rang : un retrait puis une insertion chacun, rien pour c, d, e.
    QCOMPARE(countKind(changes, core::ChangeKind::ObjectRemoved), 2);
    QCOMPARE(countKind(changes, core::ChangeKind::ObjectInserted), 2);
}
```
Noms de `Changes` et de ses genres : ceux de `core/pagecache.h` (A10, à revérifier).

**Interdits.** Grouper et dégrouper passent par `ReplaceObjectsCommand` (A6), jamais par une commande à eux ; aucune
modification de `replace.cpp`, `model.h`.

**Terminé quand.** 6 fonctions en `PASS` ; commit « Lot 5 : organiser, couches et groupes (core) ».

**Étapes.**
- [ ] **Étape 1** : tests rouges (aides `pageABCDE`, `id`, `ids`, `ordered`, `countKind` dans le test).
- [ ] **Étape 2** : `arrangedOrder` : pour chaque couche, membres d'un groupe choisi ajoutés à la sélection
  (`expandGroups`), puis l'algorithme de `view.py:788-806` (avant / arrière par échange avec le voisin non choisi ;
  premier plan = reste + choisis ; arrière-plan = choisis + reste) ; les autres couches gardent leur ordre.
- [ ] **Étape 3** : `ReorderObjectsCommand` (ancien ordre gardé au premier redo) ; `SetLayerCommand` ; groupes
  (`makeGroupCommand` = `ReplaceObjectsCommand(page, {}, {groupe})`, groupe sur la couche la plus basse des membres ;
  `makeUngroupCommand` = `ReplaceObjectsCommand(page, groupes, {})`) ; `topLevelGroupOf`, `expandGroups` (parcours des
  `Group::members`, protégé contre un cycle : un identifiant déjà vu est ignoré — fichiers lus d'une autre source).
- [ ] **Étape 4** : vert, format, commit.

### Tâche L5-T6 — `core` : aligner, ajuster, projeter, inverser, symétries d'origine

<a id="tache-l5-t6"></a>

**Objectif.** Remplir `core/src/alignment.cpp` (annexe A, `alignment.h`) selon D6, D7, D8 (`view.py:873-960`). Le
résultat est une liste de transformations passée par l'appelant (L5-T14) à `TransformObjectsCommand` (A7) ; les copies
miroir passent par `preparePaste` et `PasteObjectsCommand` (L5-T4).

**Fichiers.** `core/src/alignment.cpp`, `tests/tst_core_alignment.cpp`.

**Test et commande bornée.** Commande ciblée, motif `^(tst_core_alignment)$`. Fonctions :
```cpp
void TestAlignment::alignLeftUsesTheLeftmostEdge()
{
    auto d = blank();
    const auto a = addRect(d, {100, 10, 50, 50});
    const auto b = addRect(d, {300, 80, 20, 20});
    const auto t = core::alignTransforms(d.pages[0], {a, b}, core::AlignMode::Left);
    QCOMPARE(t.size(), std::size_t(1)); // a ne bouge pas
    QCOMPARE(boundsWith(d, b, t[0].second).left(), 100.0);
}
void TestAlignment::alignWidthUsesTheFirstCreated()
{
    auto d = blank();
    const auto a = addRect(d, {0, 0, 80, 40});  // créé d'abord
    const auto b = addRect(d, {200, 0, 20, 20});
    const auto t = core::alignTransforms(d.pages[0], {b, a}, core::AlignMode::Width);
    QCOMPARE(boundsWith(d, b, t[0].second).width(), 80.0);
    QCOMPARE(boundsWith(d, b, t[0].second).left(), 200.0); // bord gauche gardé
}
void TestAlignment::lockedObjectsStay()
{
    auto d = blank();
    const auto a = addRect(d, {100, 0, 10, 10});
    const auto b = addRect(d, {300, 0, 10, 10});
    objectById(d, b).common.locked = true;
    QVERIFY(core::alignTransforms(d.pages[0], {a, b}, core::AlignMode::Left).empty()); // moins de 2 non verrouillés
}
void TestAlignment::fitOnlyShrinksOversized()
{
    auto d = blank(); // 1024 × 768
    const auto big = addRect(d, {0, 0, 2048, 300});
    const auto small = addRect(d, {0, 400, 100, 100});
    const auto t = core::fitTransforms(d.pages[0], {big, small}, core::FitMode::Width);
    QCOMPARE(t.size(), std::size_t(1));
    QCOMPARE(boundsWith(d, big, t[0].second).width(), 1024.0);
    QCOMPARE(boundsWith(d, big, t[0].second).height(), 150.0); // proportions gardées
}
void TestAlignment::flipXNegatesRotation()
{
    auto d = blank();
    const auto a = addRotatedRect(d, {100, 100, 60, 20}, 30.0);
    const auto t = core::flipTransforms(d.pages[0], {a}, core::Axis::X);
    QCOMPARE(rotationOf(t[0].second), -30.0);
    QCOMPARE(boundsWith(d, a, t[0].second).center(), boundsOf(objectById(d, a)).center());
}
void TestAlignment::mirrorXPlacesCopyAbove()
{
    auto d = blank();
    const auto a = addRect(d, {100, 300, 60, 40});
    const auto copies = core::mirrorCopies(d.pages[0], {a}, core::Axis::X);
    QCOMPARE(copies.size(), std::size_t(1));
    QCOMPARE(boundsOf(copies[0]).bottom(), 300.0); // bord contre bord, au-dessus
    QCOMPARE(boundsOf(copies[0]).left(), 100.0);
}
void TestAlignment::originSymmetry()
{
    auto d = blank();
    const auto a = addRect(d, {300, 100, 20, 20});
    const auto t = core::originFlipTransforms(d.pages[0], {a}, core::Axis::Y, {200, 0});
    QCOMPARE(boundsWith(d, a, t[0].second).left(), 80.0); // symétrique de [300, 320] par x = 200
}
```
Les modes restants (`CenterX`, `Right`, `Top`, `CenterY`, `Bottom`, `Height`, `Angle`, `All`, `Height` et `Page` de
l'ajustement, `OriginalSize`) ont chacun une fonction de test `align<Mode>` / `fit<Mode>` sur le même gabarit (valeurs
attendues tirées de `view.py:873-937`) — 11 fonctions de plus, écrites à l'étape 1.

**Interdits.** Aucune commande dans ce fichier (fonctions pures) ; aucune dépendance à l'interface.

**Terminé quand.** 18 fonctions en `PASS` ; commit « Lot 5 : aligner, ajuster, projeter, inverser (core) ».

**Étapes.**
- [ ] **Étape 1** : tests rouges (7 ci-dessus + 11 modes) ; commande ciblée → rouge.
- [ ] **Étape 2** : emprise en page de chaque objet (fonction d'emprise du lot 3 ou du modèle, A8) ; union ; règles
  de `view.py` traduites en transformations (translation, échelle autour du centre, rotation).
- [ ] **Étape 3** : inverser = échelle −1 sur l'axe autour du centre, angle opposé ; symétrie d'origine = réflexion
  par l'axe passant par l'origine ; copies miroir = copie de l'objet, inversée, translatée bord contre bord.
- [ ] **Étape 4** : vert, format, commit.

### Tâche L5-T7 — `core` : verrouiller, masquer, glisser une copie

<a id="tache-l5-t7"></a>

**Objectif.** Remplir `core/src/objectflags.cpp` (annexe A, `objectflags.h`) selon D9.

**Fichiers.** `core/src/objectflags.cpp`, `tests/tst_core_objectflags.cpp`.

**Test et commande bornée.** Commande ciblée, motif `^(tst_core_objectflags)$`. Fonctions :
```cpp
void TestFlags::toggleLockIsOneStep()
{
    auto d = blank();
    const auto a = addRect(d, {0, 0, 10, 10});
    const auto b = addRect(d, {20, 0, 10, 10});
    core::UndoStack stack;
    stack.push(std::make_unique<core::SetObjectFlagCommand>(0, std::vector{a, b}, core::ObjectFlag::Locked, true), d);
    QVERIFY(core::flagOfAll(d.pages[0], {a, b}, core::ObjectFlag::Locked));
    stack.undo(d);
    QVERIFY(!objectById(d, a).common.locked && !objectById(d, b).common.locked);
}
void TestFlags::mixedSelectionLocksAll()
{
    auto d = blank();
    const auto a = addRect(d, {0, 0, 10, 10});
    const auto b = addRect(d, {20, 0, 10, 10});
    objectById(d, a).common.locked = true;
    QVERIFY(!core::flagOfAll(d.pages[0], {a, b}, core::ObjectFlag::Locked)); // la bascule posera « vrai »
}
void TestFlags::hiddenObjectNotPainted()
{
    auto d = blank();
    const auto a = addRect(d, {0, 0, 10, 10});
    core::Changes changes;
    core::SetObjectFlagCommand command(0, {a}, core::ObjectFlag::Hidden, true);
    command.redo(d, changes);
    QVERIFY(!objectById(d, a).common.visible);
    QCOMPARE(countKind(changes, core::ChangeKind::ObjectChanged), 1);
    core::Changes none;
    core::SetObjectFlagCommand lock(0, {a}, core::ObjectFlag::Locked, true);
    lock.redo(d, none);
    QVERIFY(none.empty()); // verrouiller ne change aucun pixel : aucun redessin
}
void TestFlags::dragCopyFlagRoundTrips()
{
    auto d = blank();
    const auto a = addRect(d, {0, 0, 10, 10});
    core::UndoStack stack;
    stack.push(std::make_unique<core::SetObjectFlagCommand>(0, std::vector{a}, core::ObjectFlag::DragCopy, true), d);
    QVERIFY(objectById(d, a).common.dragCopy);
    stack.undo(d);
    QVERIFY(!objectById(d, a).common.dragCopy);
}
```

**Interdits.** Aucun `Change` pour `Locked` et `DragCopy` ; aucune modification de `model.h` (champ posé par L5-T1).

**Terminé quand.** 4 fonctions en `PASS` ; commit « Lot 5 : verrouiller, masquer, glisser une copie (core) ».

**Étapes.**
- [ ] **Étape 1** : tests rouges ; **Étape 2** : commande (valeurs d'avant gardées au premier redo) et `flagOfAll` ;
  **Étape 3** : vert, format, commit.

### Tâche L5-T8 — `core` + `render` : pot de peinture

<a id="tache-l5-t8"></a>

**Objectif.** Remplir `core/src/fill.cpp` et `render/src/imagefill.cpp` (annexe A) selon D10 (F-235, plan comparatif
ligne 17 : intérieur, contour, fond ; objets verrouillés remplis comme l'original).

**Fichiers.** `core/src/fill.cpp`, `render/src/imagefill.cpp`, `tests/tst_core_fill.cpp` (le test lie `actcraft_render`
pour `floodFill` : déclaration de `tests/core.cmake` posée ainsi par L5-T1).

**Test et commande bornée.** Commande ciblée, motif `^(tst_core_fill)$`. Fonctions :
```cpp
void TestFill::insideClosedShapeFills()
{
    auto d = blank();
    const auto a = addRect(d, {100, 100, 200, 100});
    const auto hit = core::fillHitAt(d.pages[0], {200, 150}, 3.0);
    QCOMPARE(hit.target, core::FillTarget::ShapeInside);
    QCOMPARE(hit.object, a);
}
void TestFill::onOutlineRecolorsPen()
{
    auto d = blank();
    const auto a = addRect(d, {100, 100, 200, 100});
    QCOMPARE(core::fillHitAt(d.pages[0], {100, 150}, 3.0).target, core::FillTarget::Outline);
    const auto s = addStroke(d, {{400, 400}, {500, 400}});
    QCOMPARE(core::fillHitAt(d.pages[0], {450, 401}, 3.0).object, s);
}
void TestFill::emptyAreaFillsBackground()
{
    auto d = blank();
    core::UndoStack stack;
    const auto hit = core::fillHitAt(d.pages[0], {10, 10}, 3.0);
    QCOMPARE(hit.target, core::FillTarget::PageBackground);
    stack.push(core::makeFillCommand(d.pages[0], 0, hit, 0xFF336699u), d);
    QCOMPARE(backgroundColor(d.pages[0]), core::Argb(0xFF336699u));
}
void TestFill::lockedObjectsAreFilled()
{
    auto d = blank();
    const auto a = addRect(d, {100, 100, 200, 100});
    objectById(d, a).common.locked = true;
    QCOMPARE(core::fillHitAt(d.pages[0], {200, 150}, 3.0).object, a);
}
void TestFill::imageFloodFillTolerance5()
{
    QImage image(10, 10, QImage::Format_ARGB32);
    image.fill(QColor(200, 200, 200));
    image.setPixelColor(9, 9, QColor(200, 200, 230)); // écart de 30 sur 255 ≈ 12 % : hors tolérance 5
    const QImage out = render::floodFill(image, {0, 0}, 0xFFFF0000u, core::kDefaultImageFillTolerance);
    QCOMPARE(out.pixelColor(5, 5), QColor(255, 0, 0));
    QCOMPARE(out.pixelColor(9, 9), QColor(200, 200, 230));
}
void TestFill::fillIsOneStep()
{
    auto d = blank();
    const auto a = addRect(d, {100, 100, 200, 100});
    core::UndoStack stack;
    stack.push(core::makeFillCommand(d.pages[0], 0, core::fillHitAt(d.pages[0], {200, 150}, 3.0), 0xFF00FF00u), d);
    QCOMPARE(fillColorOf(objectById(d, a)), core::Argb(0xFF00FF00u));
    stack.undo(d);
    QVERIFY(fillColorOf(objectById(d, a)) != core::Argb(0xFF00FF00u));
}
```

**Interdits.** `core/src/fill.cpp` sans QtGui (le remplissage d'image est dans `render/`) ; intersection de formes :
défaut H5-4 (forme du dessus) tant que L5-T0 ne l'a pas tranché ; le remplissage de fond passe par la commande de fond
du lot 4 (`pagesettings.h`, L4-T4, à revérifier) et ne redessine que le fond.

**Terminé quand.** 6 fonctions en `PASS` ; commit « Lot 5 : pot de peinture (core, render) ».

**Étapes.**
- [ ] **Étape 1** : tests rouges ; **Étape 2** : `fillHitAt` (objets du dessus vers le dessous ; forme fermée :
  point dans un chemin fermé ; contour : distance au chemin ≤ tolérance + demi-épaisseur ; sinon fond) ;
  **Étape 3** : `makeFillCommand` (remplissage plein : `Fill` du modèle ; contour : `Outline.color`), une étape ;
  `makeImageFillCommand` (ressource neuve, clé SHA-256) ; **Étape 4** : `floodFill` (file de pixels, 4-connexe,
  écart par canal ≤ tolérance × 255 / 100) ; **Étape 5** : vert, format, commit.

### Tâche L5-T9 — `core` : points de forme

<a id="tache-l5-t9"></a>

**Objectif.** Remplir `core/src/shapepoints.cpp` (annexe A) selon D13 (F-527). Une forme intelligente du lot 4 (A16)
garde ses points dédiés : ses points de forme sont ceux de son contour calculé, et déplacer l'un d'eux la fait
**polygone** (elle perd ses paramètres, comme l'original quand on déforme une forme libre ; à confirmer par L5-T0).

**Fichiers.** `core/src/shapepoints.cpp`, `tests/tst_core_shapepoints.cpp`.

**Test et commande bornée.** Commande ciblée, motif `^(tst_core_shapepoints)$`. Fonctions :
```cpp
void TestShapePoints::squareHasEightPoints()
{
    auto d = blank();
    const auto a = addRect(d, {100, 100, 100, 100});
    const auto points = core::shapePoints(objectById(d, a));
    QCOMPARE(points.size(), std::size_t(8));
    QVERIFY(contains(points, {100, 100}) && contains(points, {150, 100}) && contains(points, {200, 200}));
}
void TestShapePoints::movingAPointDeformsTheShape()
{
    auto d = blank();
    const auto a = addRect(d, {100, 100, 100, 100});
    core::UndoStack stack;
    stack.push(std::make_unique<core::MoveShapePointCommand>(0, a, 0, core::PointF(80, 60)), d);
    QVERIFY(contains(core::shapePoints(objectById(d, a)), {80, 60}));
    QCOMPARE(boundsOf(objectById(d, a)).top(), 60.0);
}
void TestShapePoints::smartShapeKeepsItsSmartPoints()
{
    auto d = blank();
    const auto star = addSmartShape(d, "star"); // préréglage du lot 4 (A16)
    QVERIFY(!core::shapePoints(objectById(d, star)).empty());
    QVERIFY(!smartPointsOf(objectById(d, star)).empty()); // tant qu'aucun point de forme n'a bougé
}
void TestShapePoints::pointMoveIsOneStep()
{
    auto d = blank();
    const auto a = addRect(d, {100, 100, 100, 100});
    const auto before = objectFingerprint(objectById(d, a));
    core::UndoStack stack;
    stack.push(std::make_unique<core::MoveShapePointCommand>(0, a, 3, core::PointF(250, 250)), d);
    stack.undo(d);
    QCOMPARE(objectFingerprint(objectById(d, a)), before);
}
```

**Interdits.** Aucune modification de `shapegeometry.*` (lot 4) ; la commande garde contour, remplissage, couche, nom,
verrou ; un objet verrouillé refuse la commande (redo sans effet, aucun `Change`).

**Terminé quand.** 4 fonctions en `PASS` ; commit « Lot 5 : points de forme (core) ».

**Étapes.**
- [ ] **Étape 1** : tests rouges ; **Étape 2** : `shapePoints` (chemins de `Shape::paths` passés en page par la
  transformation ; sommets puis milieux) ; **Étape 3** : `MoveShapePointCommand` (point remis en coordonnées de
  l'objet par l'inverse de la transformation ; milieu déplacé = nouveau sommet inséré ; forme passée au genre
  polygone) ; **Étape 4** : vert, format, commit.

### Tâche L5-T11 — `core` : modification de stylo

<a id="tache-l5-t11"></a>

**Objectif.** Remplir `core/src/penmodifiers.cpp` (annexe A) selon D12 (F-221). Le tracé produit est une annotation
(couche du dessus), donc effaçable en partie par la gomme du lot 3.

**Fichiers.** `core/src/penmodifiers.cpp`, `tests/tst_core_penmodifiers.cpp`.

**Test et commande bornée.** Commande ciblée, motif `^(tst_core_penmodifiers)$`. Fonctions :
```cpp
void TestPenModifiers::horizontalLineIgnoresVerticalMotion()
{
    const auto p = core::penShapePoints(core::PenShape::Horizontal, {10, 10}, {200, 90});
    QCOMPARE(p.front().y, 10.0);
    QCOMPARE(p.back(), core::PointF(200, 10));
}
void TestPenModifiers::squareHasEqualSides()
{
    const auto p = core::penShapePoints(core::PenShape::Square, {0, 0}, {100, 40});
    QCOMPARE(p.size(), std::size_t(5)); // fermé
    QCOMPARE(p[2], core::PointF(100, 100)); // côté = la plus grande des deux dimensions
}
void TestPenModifiers::circleClosed()
{
    const auto p = core::penShapePoints(core::PenShape::Circle, {0, 0}, {100, 100});
    QVERIFY(p.size() >= 32);
    QCOMPARE(p.front(), p.back());
}
void TestPenModifiers::arrowHasAHead()
{
    const auto p = core::penShapePoints(core::PenShape::Arrow, {0, 0}, {100, 0});
    QVERIFY(p.size() >= 5); // fût, puis deux branches de la pointe en un seul tracé
}
void TestPenModifiers::resultIsAStrokeOnTheTopLayer()
{
    core::Outline pen;
    pen.color = 0xFF0000FFu;
    pen.width = 4;
    const auto object = core::penShapeStroke({{0, 0}, {10, 0}}, pen, false);
    QVERIFY(std::get_if<core::Stroke>(&object));
    QCOMPARE(object.common.layer, core::Layer::Top);
    QCOMPARE(core::penShapeFromId("zigzag"), std::optional(core::PenShape::Zigzag));
    QVERIFY(!core::penShapeFromId("autre"));
}
```
Plus une fonction par tracé restant (`Vertical`, `Rectangle`, `Ellipse`, `Arc`, `Polyline`, `Zigzag`, `Free`) sur le
même gabarit : 7 de plus.

**Interdits.** Fonctions pures, aucune commande ; le marqueur (`highlighter = true`) garde la translucidité du lot 3.

**Terminé quand.** 12 fonctions en `PASS` ; commit « Lot 5 : tracés de la modification de stylo (core) ».

**Étapes.**
- [ ] **Étape 1** : tests rouges ; **Étape 2** : un générateur par tracé (cercle et ellipse : 64 segments ; arc :
  demi-ellipse inscrite ; zigzag : pas de 16 unités ; flèche : pointe de 20 unités à ±30°) ; **Étape 3** :
  `penShapeStroke` avec le genre d'annotation stylo ou marqueur du modèle ; **Étape 4** : vert, format, commit.

### Tâche L5-T10 — `core` : connecteurs et leur suivi

<a id="tache-l5-t10"></a>

**Objectif.** Remplir `core/src/connectors.cpp` (annexe A) selon D11. Point délicat, d'où le `concepteur-critique` :
**parité de tous les chemins qui déplacent un objet** (glisser par soulèvement, aligner, ajuster, inverser, points de
forme, poignées du lot 3, supprimer, couper, annuler, rétablir). Le patron évident — recalculer les connecteurs au
dessin, ou dans chaque outil — est le mauvais : il diverge d'un chemin à l'autre et ne s'annule pas. Contrat :
`withFollowingConnectors` enveloppe la commande ; **L5-T14 et L5-T16 enveloppent chacune de leurs commandes**, et
L5-T17 enveloppe le dépôt du lot 3 (`dropLifted`) par l'adaptateur de `ToolHost` ; un test de L5-T14 vérifie que
chaque commande d'édition de `ObjectEditing` passe par l'enveloppe.

**Fichiers.** `core/src/connectors.cpp`, `tests/tst_core_connectors.cpp`.

**Test et commande bornée.** Commande ciblée, motif `^(tst_core_connectors)$`. Fonctions :
```cpp
void TestConnectors::attachPointsAreShapeVertices()
{
    auto d = blank();
    const auto a = addRect(d, {100, 100, 100, 50});
    const auto points = core::attachPoints(objectById(d, a));
    QCOMPARE(points.size(), std::size_t(4));
    QVERIFY(contains(points, {200, 150}));
}
void TestConnectors::endSnapsWithin20()
{
    auto d = blank();
    const auto a = addRect(d, {100, 100, 100, 50});
    QCOMPARE(core::snapConnectorEnd(d.pages[0], {215, 160})->object, a); // à 18 du coin (200, 150)
    QVERIFY(!core::snapConnectorEnd(d.pages[0], {240, 160}));
}
void TestConnectors::connectorFollowsTransform()
{
    auto d = blank();
    const auto a = addRect(d, {100, 100, 100, 50});
    const auto b = addRect(d, {400, 100, 100, 50});
    const auto c = addConnectorAt(d, {200, 150}, a, {400, 100}, b);
    core::UndoStack stack;
    stack.push(core::withFollowingConnectors(d.pages[0], 0, translateCommand(a, 50, 0)), d);
    QCOMPARE(std::get<core::Connector>(objectById(d, c)).firstEnd, core::PointF(250, 150));
}
void TestConnectors::connectorFollowsAlign()
{
    auto d = blank();
    const auto a = addRect(d, {100, 100, 100, 50});
    const auto b = addRect(d, {400, 300, 100, 50});
    const auto c = addConnectorAt(d, {200, 150}, a, {400, 300}, b);
    const auto t = core::alignTransforms(d.pages[0], {a, b}, core::AlignMode::Top);
    core::UndoStack stack;
    stack.push(core::withFollowingConnectors(d.pages[0], 0, std::make_unique<core::TransformObjectsCommand>(0, t)), d);
    QCOMPARE(std::get<core::Connector>(objectById(d, c)).secondEnd.y, 100.0);
}
void TestConnectors::undoRestoresConnectorAndObject()
{
    auto d = blank();
    const auto a = addRect(d, {100, 100, 100, 50});
    const auto c = addConnectorAt(d, {200, 150}, a, {600, 600}, 0);
    const auto before = fingerprint(d);
    core::UndoStack stack;
    stack.push(core::withFollowingConnectors(d.pages[0], 0, translateCommand(a, 30, 30)), d);
    stack.undo(d);
    QCOMPARE(fingerprint(d), before);
    stack.redo(d);
    QCOMPARE(std::get<core::Connector>(objectById(d, c)).firstEnd, core::PointF(230, 180));
}
void TestConnectors::deletingAnEndDetaches()
{
    auto d = blank();
    const auto a = addRect(d, {100, 100, 100, 50});
    const auto c = addConnectorAt(d, {200, 150}, a, {600, 600}, 0);
    core::UndoStack stack;
    stack.push(core::withFollowingConnectors(d.pages[0], 0, core::makeCutCommand(d.pages[0], 0, {a})), d);
    QCOMPARE(std::get<core::Connector>(objectById(d, c)).objectA, core::ObjectId(0));
    QCOMPARE(std::get<core::Connector>(objectById(d, c)).firstEnd, core::PointF(200, 150)); // reste en place
}
```
`TransformObjectsCommand` : signature du lot 3 (A7, à revérifier) ; `translateCommand` : aide du test qui la construit.
Mutation : dans `FollowingConnectorsCommand::undo`, annuler la commande intérieure **avant** de remettre les
connecteurs → `undoRestoresConnectorAndObject` rouge (ou vert : alors l'ordre n'importe pas et le contrat est
simplifié dans le rapport) ; remise par `git -C <wt> checkout -- core/src/connectors.cpp`.

**Interdits.** Aucun calcul de connecteur au dessin ; aucune modification de `objectedits.cpp` (lot 3) ni de
`replace.cpp` ; position relative d'attache gardée par l'indice du point d'attache (pas par une distance).

**Terminé quand.** 6 fonctions en `PASS` ; mutation jouée ; commit « Lot 5 : connecteurs qui suivent leurs objets
(core) ».

**Étapes.**
- [ ] **Étape 1** : tests rouges ; **Étape 2** : `attachPoints`, `snapConnectorEnd`, `makeConnector` ;
  **Étape 3** : `FollowingConnectorsCommand` : au premier redo, garder les connecteurs liés aux objets de la page
  avant la commande ; redo intérieur ; pour chaque connecteur lié, extrémité recalculée (indice du point d'attache le
  plus proche avant, appliqué à l'objet après) ou détachée si l'objet n'est plus là ; un `ObjectChanged` par
  connecteur modifié ; undo : connecteurs remis, puis undo intérieur ; **Étape 4** : `withFollowingConnectors` ;
  **Étape 5** : vert, mutation, format, commit.

### Tâche L5-T14 — `ui` : contrôleur d'édition d'objet

<a id="tache-l5-t14"></a>

**Objectif.** Remplir `ui/src/objectediting.cpp` (annexe A, `objectediting.h`) : chaque méthode lit la sélection de
l'utilisateur courant (`appTools.user().selection`, A3, A4), construit la commande `core` du lot et la passe à
`DocumentController::execute` (A5) **enveloppée par `core::withFollowingConnectors`** (L5-T10) quand elle déplace,
transforme ou retire des objets. Presse-papiers du système (`QGuiApplication::clipboard()`) : `QMimeData` avec
`formats::kObjectsMimeType` (L5-T3) ; une seule image copiée → aussi `setImageData` ; collage d'une autre application :
image → `Insertion` (L5-T15, même règle de taille), texte → objet texte au centre (`view.py:706-728`). Sans sélection,
couper / copier / coller / dupliquer / supprimer agissent sur la page (D3) par `ui::PagesController` (A15). Après
coller et dupliquer, les copies sont sélectionnées et l'outil Sélection activé (D1, D2) ; après grouper puis Annuler,
la sélection est vide (D4).

**Fichiers.** `ui/src/objectediting.cpp`, `tests/tst_ui_objectediting.cpp`.

**Test et commande bornée.** Commande ciblée, motif `^(tst_ui_objectediting)$`. Contrôleur et `ToolHost` sans fenêtre
quand c'est possible, sinon page fixe hors écran comme `tst_ui_document` ; observateur espion `addChangeObserver` (A5) :
```cpp
void TestObjectEditing::copyPasteThroughSystemClipboard()
{
    Fixture f; // DocumentController + ToolHost + ObjectEditing, une page, un rectangle sélectionné
    f.editing.copy();
    QVERIFY(QGuiApplication::clipboard()->mimeData()->hasFormat(formats::kObjectsMimeType));
    f.editing.paste();
    QCOMPARE(f.objectCount(), 2);
    QCOMPARE(f.selection().size(), std::size_t(1));
    QVERIFY(f.selection()[0] != f.firstId());
    f.document.undo();
    QCOMPARE(f.objectCount(), 1);
}
void TestObjectEditing::pasteTextFromAnotherApp()
{
    Fixture f;
    QGuiApplication::clipboard()->setText(QStringLiteral("bonjour"));
    f.editing.paste();
    QVERIFY(f.lastObjectIs<core::Text>());
}
void TestObjectEditing::pasteImageFromAnotherApp()
{
    Fixture f;
    QImage big(4000, 100, QImage::Format_ARGB32);
    big.fill(Qt::red);
    QGuiApplication::clipboard()->setImage(big);
    f.editing.paste();
    QTRY_VERIFY(f.lastObjectIs<core::ImageObject>()); // décodage hors du fil principal (L5-T15)
    QVERIFY(f.lastBounds().width() <= 1024 * 0.8 + 0.5);
}
void TestObjectEditing::withoutSelectionActsOnPage()
{
    Fixture f;
    f.clearSelection();
    f.editing.duplicate(); // D3 : duplique la page
    QCOMPARE(f.document.pageCount(), 2);
}
void TestObjectEditing::groupThenUndoClearsSelection()
{
    Fixture f(2); // deux rectangles sélectionnés
    f.editing.toggleGroup();
    QVERIFY(f.editing.selectionIsGroup());
    f.document.undo();
    QVERIFY(f.selection().empty());
}
void TestObjectEditing::alignGoesThroughTransformCommand()
{
    Fixture f(2);
    f.spy.clear();
    f.editing.align(QStringLiteral("left"));
    QCOMPARE(f.spy.lastOrigin(), ui::DocumentController::ChangeOrigin::Command);
    QVERIFY(f.spy.has('v')); // ObjectChanged : la transformation passe par la pile, pas par la vue
}
void TestObjectEditing::cutGoesThroughReplaceCommand()
{
    Fixture f;
    f.spy.clear();
    f.editing.cut();
    QVERIFY(f.spy.has('r'));
    QCOMPARE(f.objectCount(), 0);
    QVERIFY(QGuiApplication::clipboard()->mimeData()->hasFormat(formats::kObjectsMimeType));
}
```
Plus `everyEditGoesThroughConnectorFollower` : un connecteur lié au rectangle ; pour chaque méthode qui déplace
(`align`, `transform`, `cut`), l'extrémité suit ou se détache (contrat de L5-T10).

**Preuve visuelle.** Aucune ici (L5-T18). Banc sans écran joint (la tâche touche `ui/`).

**Interdits.** Aucun accès à `FixedPageItem` ; aucune commande qui ne passe pas par `execute` ; presse-papiers
interne au processus interdit (copier entre onglets et entre instances passe par le presse-papiers du système).

**Terminé quand.** 8 fonctions en `PASS` ; `tst_ui_document` et `tst_ui_lift` (lot 3) encore verts ; banc joint ;
commit « Lot 5 : contrôleur d'édition d'objet ».

**Étapes.**
- [ ] **Étape 1** : tests rouges ; **Étape 2** : sélection et états (`hasSelection`, bascules, `selectionIsGroup`,
  `selectionIsAnnotation`) relus à chaque `selectionChanged` de `ToolHost` et `documentChanged` ; **Étape 3** :
  presse-papiers (copier, couper, coller, coller sur l'objet, coller à un point) ; **Étape 4** : dupliquer, grouper,
  organiser (`arrangedOrder` → `ReorderObjectsCommand`, ou `SetLayerCommand`), aligner, ajuster, inverser, projeter
  (`mirrorCopies` → `preparePaste(…, 0, 0)` → `PasteObjectsCommand`), symétries d'origine (position de l'origine
  lue par A17 ; commande inerte si l'origine n'est pas affichée), bascules (`SetObjectFlagCommand`) ; **Étape 5** :
  vert, banc sans écran, format, commit.

### Tâche L5-T15 — `ui` : insérer une image ou un média

<a id="tache-l5-t15"></a>

**Objectif.** Remplir `ui/src/insertion.cpp` (annexe A, `insertion.h`) selon D19 : lecture et décodage du fichier sur
un fil secondaire (`QtConcurrent::run`), ressource à clé SHA-256, image ramenée à 80 % de la page au plus, centrée,
couche du milieu, nommée d'après le fichier ; média : objet `Media` au centre avec l'image d'espace réservé (lecture au
lot 8) ; dépôt de fichiers et de texte (F-664) ; une étape par insertion (`ReplaceObjectsCommand({}, {objet})`, A6) ;
l'objet inséré est sélectionné.

**Fichiers.** `ui/src/insertion.cpp`, `tests/tst_ui_insertion.cpp`.

**Test et commande bornée.** Commande ciblée, motif `^(tst_ui_insertion)$`. Fonctions : `imageFitsIn80PercentCentered`
(PNG 2000 × 1000 écrit sous `QTemporaryDir` → emprise 819,2 × 409,6 centrée sur (512, 384)), `imageNamedAfterFile`
(nom = nom de fichier sans extension), `decodeOffMainThread` (le décodage tourne hors du fil principal :
`QThread::currentThread()` relevé par un crochet de test, ou propriété `lastDecodeOffMainThread` sur le modèle de
`lastIoOffMainThread` du lot 2), `mediaInsertedAtCenter`, `droppedTextBecomesText` (`acceptDrop({}, "abc", {100,
100})` → objet texte en (100, 100)), `insertionIsOneStep` (un Annuler retire l'image et la ressource n'est plus citée).

**Interdits.** Aucun décodage sur le fil principal ; aucune lecture réseau (`QUrl` locale ou `content://`) ; ressource
écrite telle quelle (octets d'origine), jamais réencodée.

**Terminé quand.** 6 fonctions en `PASS` ; banc sans écran joint ; commit « Lot 5 : insérer une image ou un média ».

**Étapes.** Tests rouges → lecture (`DocumentController::urlFromArgument` pour l'URL, A5) et décodage en tâche de fond
→ retour sur le fil principal (`QFutureWatcher`) → commande → vert → banc → format → commit.

### Tâche L5-T12 — `tools` : pot de peinture et modification de stylo

<a id="tache-l5-t12"></a>

**Objectif.** Remplir `tools/src/filltool.cpp` (clic → `core::fillHitAt` avec la tolérance de désignation du lot 3
convertie en page, puis `makeFillCommand` ou, sur une image, `render::floodFill` puis `makeImageFillCommand`, par
`DocumentEditor::execute`, A1) et `tools/src/penshapetool.cpp` (glisser : aperçu par la couche soulevée
`LiftSurface::show`, aucun redessin du cache pendant le geste ; relâché : **un** `AddObjectCommand` de
`core::penShapeStroke` ; ligne brisée : chaque clic pose un sommet, double-clic termine).

**Fichiers.** `tools/src/filltool.cpp`, `tools/src/penshapetool.cpp`, `tests/tst_tools_fill.cpp`,
`tests/tst_tools_penshape.cpp`.

**Test et commande bornée.** Commande ciblée, motif `^(tst_tools_fill|tst_tools_penshape)$`. Faux `ToolContext` du lot
3 (aides de `tst_tools_select`, à revérifier) : `clickFillsTopmostClosedShape`, `clickOnEmptyFillsBackground`,
`clickOnImageFloodFills`, `dragDrawsAPreviewThenOneStroke` (un seul `execute` au relâché), `previewUsesTheLiftSurface`
(`show` appelé pendant le geste, `clear` au relâché, aucun `execute` avant), `polylineEndsOnDoubleClick`.

**Interdits.** Aucun accès au document hors de `ToolContext` ; aucune commande pendant le geste.

**Terminé quand.** Les deux tests `Passed` (6 fonctions en `PASS`) ; banc sans écran joint ; commit « Lot 5 : outils
pot de peinture et modification de stylo ».

**Étapes.** Tests rouges → `FillTool` → `PenShapeTool` → vert → banc → format → commit.

### Tâche L5-T13 — `tools` : outil connecteur

<a id="tache-l5-t13"></a>

**Objectif.** Remplir `tools/src/connectortool.cpp` : au survol, les points d'attache de l'objet sous le pointeur
(`core::attachPoints`) sont montrés par la surcouche d'outil (carré de 13 px, plan comparatif ligne 19) ; presser sur
un objet puis glisser : aperçu de la ligne par `LiftSurface::show` ; relâcher : `snapConnectorEnd` à chaque bout, puis
**un** `AddObjectCommand(makeConnector(…, style))` ; un bout sans objet reste libre.

**Fichiers.** `tools/src/connectortool.cpp`, `tests/tst_tools_connector.cpp`.

**Test et commande bornée.** Commande ciblée, motif `^(tst_tools_connector)$` : `hoverShowsAttachPoints`,
`dragFromObjectToObjectConnects` (`objectA` et `objectB` posés), `releaseOnEmptyLeavesAFreeEnd`,
`connectorGoesOnTopLayer`, `styleIsUsed` (`setStyle(5)` → `Connector::style == 5`).

**Interdits.** Aucune commande pendant le geste ; aucun dessin de connecteur hors du peintre partagé (le rendu des
connecteurs existe depuis le lot 2 ; extrémités `capA`/`capB` dessinées par `render/src/pagepainter.cpp` : si le
peintre ne les dessine pas encore, **tâche corrective `L5-C1`** ajoutée par l'orchestrateur, fichier
`render/src/pagepainter.cpp`, après V6).

**Terminé quand.** 5 fonctions en `PASS` ; banc sans écran joint ; commit « Lot 5 : outil connecteur ».

**Étapes.** Tests rouges → survol et surcouche → glisser et aperçu → relâché → vert → banc → format → commit.

### Tâche L5-T16 — `tools` : sélection, glisser une copie et points de forme

<a id="tache-l5-t16"></a>

**Objectif.** Dans `tools/src/selecttool.cpp` (lot 3) : (1) **glisser une copie** (D9) : un objet à `dragCopy` vrai
est soulevé comme au lot 3, mais au dépôt l'original reste et une copie est ajoutée à la position de dépôt —
`dropLifted` reçoit une `PasteObjectsCommand` (L5-T4) au lieu d'une `TransformObjectsCommand`, l'original étant
redéposé sans étape ; (2) **verrouillé** : seul le glissement par la poignée « Déplacer » du lot 3 le déplace ;
(3) **mode points de forme** (activé par `object.shape-points`) : `SelectionView` montre les points
(`core::shapePoints`) comme carrés (nouveau champ `shapePoints` à la fin de `SelectionView`, ajouté par cette tâche dans
`selectiongeometry.cpp` seulement si l'en-tête du lot 3 le permet ; sinon dans un en-tête d'outil propre — écart au
rapport), glisser un point → **une** `MoveShapePointCommand` au relâché, aperçu par la couche soulevée.

Point délicat (`concepteur-critique`) : parité du chemin de dépôt — un dépôt normal et un dépôt de copie doivent
laisser le cache dans le même état qu'un dépôt du lot 3 (même observateur, `ChangeOrigin::Drop`) ; la copie ne doit
pas laisser l'original soulevé.

**Fichiers.** `tools/src/selecttool.cpp`, `tools/src/selectiongeometry.cpp`, `tests/tst_tools_selectcopy.cpp`.

**Test et commande bornée.** Commande ciblée, motif `^(tst_tools_selectcopy|tst_tools_select)$` :
`dragCopyLeavesTheOriginal`, `dragCopyIsOneStep` (un Annuler retire la copie, l'original n'a pas bougé),
`lockedMovesOnlyByMoveHandle`, `shapePointsModeShowsSquares` (8 pour un carré), `shapePointDragIsOneStep` ; et tout
`tst_tools_select` (lot 3) vert. Mutation : déposer l'original par une transformation (comme un glissement normal)
quand `dragCopy` est vrai → `dragCopyLeavesTheOriginal` rouge ; remise par
`git -C <wt> checkout -- tools/src/selecttool.cpp`.

**Interdits.** Aucune signature publique du lot 3 changée ; aucun redessin du cache pendant un glissement.

**Terminé quand.** Les deux tests `Passed` ; mutation rouge puis remise ; banc sans écran joint ; commit « Lot 5 :
sélection — glisser une copie et points de forme ».

**Étapes.** Tests rouges → dépôt de copie → verrou → mode points de forme → vert → mutation → banc → format → commit.

### Tâche L5-T17 — `ui::ToolHost` : outils du lot branchés

<a id="tache-l5-t17"></a>

**Objectif.** Dans `ui/src/toolhost.cpp` et `ui/src/tooladapters.{h,cpp}` : inscrire « fill », « pen-shape »,
« connector » au registre (A3) ; `activate("select-points")` = outil Sélection en mode points de forme ;
`activate("pen-shape", argument)` seulement si l'outil courant est stylo, marqueur (ou encre révélatrice au lot 8) —
sinon sans effet (F-221) ; `setConnectorStyle(int)` (méthode invocable ajoutée ici, gestionnaire de L5-T2 complété :
`ui/qml/Main.qml` **n'est pas** touché, le gestionnaire `connector.style` appelle `appTools.setConnectorStyle` par
le nom déjà écrit en commentaire — à décommenter par L5-T18) ; mémoire par outil (couleur du pot, style du connecteur,
tracé choisi) gardée comme celle du lot 3 (`settingsFile`) ; l'adaptateur de dépôt de `ToolHost` enveloppe la commande
de dépôt par `core::withFollowingConnectors` (L5-T10) ; Échap quitte le mode points de forme.

**Fichiers.** `ui/src/toolhost.cpp`, `ui/src/tooladapters.cpp`, `ui/src/tooladapters.h`, `tests/tst_ui_lot5tools.cpp`.

**Test et commande bornée.** Commande ciblée, motif `^(tst_ui_lot5tools)$` : `fillToolActivatesFromCommand`,
`penModifierOnlyWithInkTools`, `connectorRemembersStyle` (après relance du `ToolHost` sur le même `settingsFile`),
`escapeLeavesShapePoints`, `dropMovesLinkedConnectors` (glisser un objet relié : l'extrémité suit, une étape).

**Interdits.** Aucun outil qui contourne `ToolContext` ; aucune signature publique de `ToolHost` changée hors de
l'ajout de `setConnectorStyle`.

**Terminé quand.** 5 fonctions en `PASS` ; `tst_ui_tools` (lot 3) encore vert ; banc sans écran joint ; commit
« Lot 5 : outils du lot branchés à ToolHost ».

**Étapes.** Tests rouges → inscription → activation et mémoire → enveloppe de dépôt → vert → banc → format → commit.

### Tâche L5-T18 — QML : menu d'édition d'objet, barre d'objet, barres du lot ; scénario `edition`

<a id="tache-l5-t18"></a>

**Objectif.** Remplir `ObjectEditMenu.qml` (liste `objectMenu` de la disposition, A12 ; sous-menus Transformer et
Organiser ; entrées « Convertir » visibles seulement si `appEditing.selectionIsAnnotation`, et « Convertir en texte »
seulement sous Windows ; bascules cochées selon `selectionLocked`, `selectionIsGroup`, `selectionHidden`,
`selectionDragCopy` ; ouvert par `object.menu`, par le bouton de la barre d'objet et par le clic droit sur un objet),
les boutons du lot dans `ObjectBar.qml` (actifs), `PenModifierBar.qml` (barre verticale à côté de la palette, un bouton
par tracé de D12, recliquer revient au trait libre), `ConnectorPanel.qml` (panneau des 14 styles, ligne simple par
défaut, ancré à droite), et le scénario d'interface `bench/scenarios/edition.json` (S8.2, S8.3) ; dans `Main.qml`, le
gestionnaire `connector.style` décommenté — `Main.qml` est déjà réservé par L5-T2 et n'est touché par aucune autre
tâche de V8 : **fichier ajouté à cette tâche**.

**Fichiers.** Ceux de la ligne de la table, plus `ui/qml/Main.qml` (une ligne).

**Test et commande bornée.** `tst_ui_shell` : `objectMenuMatchesLayout` (entrées du menu = `objectMenu` filtrée par
plateforme), `convertEntriesOnlyForAnnotations`. Commande ciblée, motif `^(tst_ui_shell)$`.

**Preuve visuelle.** Scénario `edition` (pas : poser trois formes par `tool.shape`, sélectionner, `object.group`,
`edit.duplicate`, `align.left`, `arrange.front`, `object.lock`, glissement refusé, `tool.fill` dans une forme,
`object.shape-points`, glisser un point, `tool.connector` entre deux formes, déplacer l'une). Lectures d'abord :
```powershell
New-Item -ItemType Directory -Force '<wt>/.tmp' | Out-Null
pwsh -NoProfile -Command "Set-Location '<wt>'; pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario edition -Rendu logiciel -HorsEcran -Sortie .tmp/captures-edition -Lire 'appEditing.selectionIsGroup=true' -Lire 'appDocument.canUndo=true' -Lire 'objectEditMenu.count=<n attendu>' > .tmp/captures.txt 2>&1"
Select-String -Path '<wt>/.tmp/captures.txt' -Pattern '^(COMMANDE|TRAIT|TOUCHE|ATTENTE|PRET|FENETRE|LU|PIXEL|CAPTURE) |^SCENARIO |CAPTURES (OK|EN ECHEC)' | Select-Object -Last 16
```
Attendu : `CAPTURES OK`, chaque `LU` conforme, contrôles de pixels : couleur de remplissage dans la forme, ligne du
connecteur entre les deux formes après le déplacement. Capture à décrire : les deux groupes alignés à gauche, le menu
d'objet ouvert, la barre d'objet complète.

**Interdits.** Aucun texte de l'aide d'origine ; icônes du lot 1 et de L5-T1 seulement ; aucun JavaScript qui modifie
le document (tout passe par les commandes).

**Terminé quand.** `tst_ui_shell` `Passed` ; `edition` `CAPTURES OK` ; banc sans écran joint ; `NOMS OK` ; commit
« Lot 5 : menu d'édition d'objet, barre d'objet, barres du lot ».

**Étapes.** Tests rouges → menu → barre d'objet → barres → scénario → lectures et captures → banc → format → commit.

### Tâche L5-T19 — Banc : scénario `edition`

<a id="tache-l5-t19"></a>

**Objectif.** Ajouter au banc un scénario `edition` : sur une page de 200 annotations, coller 20 objets, grouper,
aligner, organiser (premier plan), pendant qu'un trait est écrit toutes les 2 s ; relever figements et reprises
(métriques existantes) et le nombre de redessins du cache par geste (`cacheRefreshCount`, A18). Il mesure la limite
déclarée (contrainte 1 : organiser = insertion hors fin de page → redessin immédiat).

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée, motif `^(tst_bench_scenarios)$` : `editionScenarioListed` ; puis banc
sans écran (`-Scenario court` inclut `edition`) : la ligne de verdict et le JSON avec la section `edition`.

**Interdits.** Aucune mesure en `debug` présentée comme preuve de fluidité ; le verdict est un résultat, pas une porte.

**Terminé quand.** Test `Passed` ; JSON joint (section `edition` présente, `content_check.ok = True`) ; commit « Lot 5 :
scénario de banc edition ».

**Étapes.** Test rouge → scénario dans `scenarios.cpp` et branchement dans `runner.cpp` → vert → banc → commit.

## Annexe A — en-têtes figés (écrits par L5-T1, mot pour mot)

Noms du modèle vérifiés à 794cf84 (`core/include/actcraft/core/model.h`) : `ObjectCommon` (`id`, `name`, `layer`,
`visible`, `locked`, `translucency`, `transform`), `Layer` (`Background`, `Bottom`, `Middle`, `Top`), `Transform`,
`Outline`, `Fill`, `Shape::paths`, `Group::members`, `Connector` (`firstEnd`, `secondEnd`, `objectA`, `objectB`,
`pen`), `Page::notes`. `UndoCommand::redo/undo(Document&, Changes&)` (`core/undo.h:37-46`). Types `PointF`, `RectF`,
`SizeF`, `Argb` : ceux du modèle (à revérifier pour `SizeF`).

### `core/include/actcraft/core/clipboardedits.h`
```cpp
#pragma once
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <memory>
#include <vector>

namespace actcraft::core {

inline constexpr double kPasteOffset = 20.0;     // D1, unités de page
inline constexpr double kDuplicateOffset = 10.0; // D2

// Objets à copier, fermés : un groupe emporte ses membres (récursivement), dans l'ordre de la page ; un connecteur
// garde le lien vers un objet emporté, perd l'autre (objectA ou objectB remis à 0).
std::vector<PageObject> closureForCopy(const Page& page, const std::vector<ObjectId>& ids);
// Copies prêtes à coller : nom vidé, déverrouillées, translatées de (dx, dy), ordonnées membres avant groupes,
// connecteurs en dernier ; identifiants remplacés par des rangs provisoires 1..n (références remappées).
std::vector<PageObject> preparePaste(std::vector<PageObject> objects, double dx, double dy);
// Coller sur l'objet (F-540) : centre de l'ensemble sur le centre de `target`, couche de `target`.
std::vector<PageObject> preparePasteOnto(std::vector<PageObject> objects, const PageObject& target);

// Ajout de copies préparées en une étape : un ReplaceObjectsCommand (A6) puis, au redo, références de groupe et de
// connecteur passées des rangs provisoires aux identifiants tirés (addedIds(), même ordre). Aucun Change de plus.
class PasteObjectsCommand final : public UndoCommand
{
public:
    PasteObjectsCommand(std::size_t page, std::vector<PageObject> prepared);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;
    const std::vector<ObjectId>& addedIds() const noexcept;

private:
    std::unique_ptr<UndoCommand> m_inner; // ReplaceObjectsCommand
    std::size_t m_page;
    std::vector<ObjectId> m_ids;
};

std::unique_ptr<UndoCommand> makeCutCommand(const Page& page, std::size_t pageIndex, const std::vector<ObjectId>& ids);
std::unique_ptr<PasteObjectsCommand> makeDuplicateCommand(const Page& page, std::size_t pageIndex,
                                                          const std::vector<ObjectId>& ids);

} // namespace actcraft::core
```

### `core/include/actcraft/core/arrange.h`
```cpp
#pragma once
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <memory>
#include <utility>
#include <vector>

namespace actcraft::core {

enum class ArrangeMove : std::uint8_t
{
    Forward,
    Backward,
    Front,
    Back,
};

// Ordre de la page après le déplacement des objets choisis, couche par couche (D5) ; les membres d'un groupe choisi
// bougent avec lui ; ordre inchangé si rien ne bouge.
std::vector<ObjectId> arrangedOrder(const Page& page, const std::vector<ObjectId>& selected, ArrangeMove move);

// Réordonne la page en une étape. Changes : pour chaque objet dont le rang change, un ObjectRemoved puis un
// ObjectInserted (zone : son emprise) ; l'inverse à l'annulation ; rien si l'ordre est identique.
class ReorderObjectsCommand final : public UndoCommand
{
public:
    ReorderObjectsCommand(std::size_t page, std::vector<ObjectId> newOrder);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    std::vector<ObjectId> m_newOrder;
    std::vector<ObjectId> m_oldOrder;
};

// Change la couche des objets (F-513) : au sommet de la couche d'arrivée, ordre relatif gardé ; une étape.
class SetLayerCommand final : public UndoCommand
{
public:
    SetLayerCommand(std::size_t page, std::vector<ObjectId> ids, Layer layer);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    std::vector<ObjectId> m_ids;
    Layer m_layer;
    std::vector<std::pair<ObjectId, Layer>> m_before;
    std::vector<ObjectId> m_orderBefore;
};

// Groupes (F-525, D4) par ReplaceObjectsCommand (A6) ; nullptr si moins de 2 objets / aucun groupe.
std::unique_ptr<UndoCommand> makeGroupCommand(const Page& page, std::size_t pageIndex, const std::vector<ObjectId>& ids);
std::unique_ptr<UndoCommand> makeUngroupCommand(const Page& page, std::size_t pageIndex,
                                                const std::vector<ObjectId>& groups);
ObjectId topLevelGroupOf(const Page& page, ObjectId id);
std::vector<ObjectId> expandGroups(const Page& page, const std::vector<ObjectId>& ids);

} // namespace actcraft::core
```

### `core/include/actcraft/core/alignment.h`
```cpp
#pragma once
#include <actcraft/core/model.h>

#include <utility>
#include <vector>

namespace actcraft::core {

enum class AlignMode : std::uint8_t
{
    Left, CenterX, Right, Top, CenterY, Bottom, Width, Height, Angle, All,
};
enum class FitMode : std::uint8_t
{
    Width, Height, Page, OriginalSize,
};
enum class Axis : std::uint8_t
{
    X, // haut / bas
    Y, // gauche / droite
};
using TransformList = std::vector<std::pair<ObjectId, Transform>>;

// Transformations à passer à TransformObjectsCommand (A7) ; objets verrouillés et inchangés absents ; moins de 2 objets
// non verrouillés : vide (D6).
TransformList alignTransforms(const Page& page, const std::vector<ObjectId>& ids, AlignMode mode);
TransformList fitTransforms(const Page& page, const std::vector<ObjectId>& ids, FitMode mode); // F-536
TransformList flipTransforms(const Page& page, const std::vector<ObjectId>& ids, Axis axis);   // F-537 inverser
TransformList originFlipTransforms(const Page& page, const std::vector<ObjectId>& ids, Axis axis, PointF origin);
// F-537 projeter : copies miroir au-dessus (X) ou à gauche (Y), bord contre bord ; à passer à preparePaste(…, 0, 0).
std::vector<PageObject> mirrorCopies(const Page& page, const std::vector<ObjectId>& ids, Axis axis);

} // namespace actcraft::core
```
(L5-T6 adapte `TransformList` au type d'argument réel de `TransformObjectsCommand` relevé par L5-T0, A7.)

### `core/include/actcraft/core/objectflags.h`
```cpp
#pragma once
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <vector>

namespace actcraft::core {

enum class ObjectFlag : std::uint8_t
{
    Locked,   // F-585
    Hidden,   // F-586 (visible = false)
    DragCopy, // F-587
};
bool flagOfAll(const Page& page, const std::vector<ObjectId>& ids, ObjectFlag flag);

// Une étape. Changes : ObjectChanged (zone : emprise) pour Hidden ; aucun pour Locked et DragCopy.
class SetObjectFlagCommand final : public UndoCommand
{
public:
    SetObjectFlagCommand(std::size_t page, std::vector<ObjectId> ids, ObjectFlag flag, bool value);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    std::vector<ObjectId> m_ids;
    ObjectFlag m_flag;
    bool m_value;
    std::vector<bool> m_before;
};

} // namespace actcraft::core
```

### `core/include/actcraft/core/fill.h`
```cpp
#pragma once
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <memory>
#include <string>

namespace actcraft::core {

enum class FillTarget : std::uint8_t
{
    Nothing, ShapeInside, Outline, Image, PageBackground,
};
struct FillHit
{
    FillTarget target = FillTarget::Nothing;
    ObjectId object = 0;
};
inline constexpr int kDefaultImageFillTolerance = 5; // F-235, sur 100

// Cible d'un clic en `point` (page), objet du dessus d'abord, verrouillés compris (D10) ; `tolerance` en unités de page.
FillHit fillHitAt(const Page& page, PointF point, double tolerance);
// Une étape ; nullptr pour Nothing et Image.
std::unique_ptr<UndoCommand> makeFillCommand(const Page& page, std::size_t pageIndex, const FillHit& hit, Argb color);
// Remplace la ressource de l'image par `pngBytes` (clé SHA-256 calculée ici) ; une étape.
std::unique_ptr<UndoCommand> makeImageFillCommand(std::size_t pageIndex, ObjectId image, std::string pngBytes);

} // namespace actcraft::core
```

### `core/include/actcraft/core/shapepoints.h`
```cpp
#pragma once
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <optional>
#include <vector>

namespace actcraft::core {

// Points de forme (F-527, D13) en page : sommets puis milieux des côtés, chemin par chemin ; vide hors Shape.
std::vector<PointF> shapePoints(const PageObject& object);

// Déplace le point `index` en `to` (page), une étape ; la forme devient un polygone ; objet verrouillé : sans effet.
class MoveShapePointCommand final : public UndoCommand
{
public:
    MoveShapePointCommand(std::size_t page, ObjectId shape, std::size_t index, PointF to);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    ObjectId m_shape;
    std::size_t m_index;
    PointF m_to;
    std::optional<PageObject> m_before;
};

} // namespace actcraft::core
```

### `core/include/actcraft/core/connectors.h`
```cpp
#pragma once
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <memory>
#include <optional>
#include <vector>

namespace actcraft::core {

inline constexpr double kConnectorSnap = 20.0; // D11, unités de page

std::vector<PointF> attachPoints(const PageObject& object); // sommets d'une forme, coins de l'emprise sinon
struct Attachment
{
    ObjectId object = 0;
    PointF point;
};
std::optional<Attachment> snapConnectorEnd(const Page& page, PointF point); // connecteurs exclus
PageObject makeConnector(PointF a, ObjectId objectA, PointF b, ObjectId objectB, std::uint8_t style); // couche du dessus

// Enveloppe une commande qui déplace, transforme ou retire : après son redo, chaque connecteur lié suit (même indice
// de point d'attache) ou se détache ; une étape ; undo : connecteurs remis, puis commande annulée. Changes : ceux de
// la commande plus un ObjectChanged par connecteur modifié.
class FollowingConnectorsCommand final : public UndoCommand
{
public:
    FollowingConnectorsCommand(std::size_t page, std::unique_ptr<UndoCommand> inner);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    std::unique_ptr<UndoCommand> m_inner;
    std::vector<PageObject> m_connectorsBefore;
};
std::unique_ptr<UndoCommand> withFollowingConnectors(const Page& page, std::size_t pageIndex,
                                                     std::unique_ptr<UndoCommand> inner);

} // namespace actcraft::core
```

### `core/include/actcraft/core/penmodifiers.h`
```cpp
#pragma once
#include <actcraft/core/model.h>

#include <optional>
#include <string_view>
#include <vector>

namespace actcraft::core {

enum class PenShape : std::uint8_t
{
    Free, Horizontal, Vertical, Square, Rectangle, Circle, Ellipse, Arc, Polyline, Arrow, Zigzag,
};
std::optional<PenShape> penShapeFromId(std::string_view id); // argument de pen.shape
std::vector<PointF> penShapePoints(PenShape shape, PointF from, PointF to, const std::vector<PointF>& clicks = {});
PageObject penShapeStroke(const std::vector<PointF>& points, const Outline& pen, bool highlighter); // couche du dessus

} // namespace actcraft::core
```

### `core/include/actcraft/core/printlayout.h`
```cpp
#pragma once
#include <actcraft/core/model.h>

#include <vector>

namespace actcraft::core {

enum class PrintRange : std::uint8_t
{
    All, Current, FromTo,
};
enum class PrintNotes : std::uint8_t
{
    None, WithPages, NotesOnly,
};
struct PrintSettings
{
    int rows = 1;
    int columns = 1;
    bool frames = false;
    bool pageNumbers = false;
    PrintRange range = PrintRange::All;
    int from = 1; // 1-based, inclus
    int to = 1;
    int copies = 1;
    double marginMm = 5.0; // F-670
    bool gray = false;
    PrintNotes notes = PrintNotes::None;
    bool noteBreak = true;
    bool noteSeparator = false;
};
struct SheetCell
{
    std::size_t page = 0;
    RectF target; // millimètres sur la feuille, rapport de la page gardé (page allongée entière)
};
struct Sheet
{
    bool notes = false;
    std::vector<SheetCell> cells;
    std::vector<std::size_t> noteOf;
};
inline constexpr int kNoteLinesPerSheet = 60;

std::vector<Sheet> layoutSheets(const Document& document, const PrintSettings& settings, std::size_t current,
                                SizeF sheetMm);
int sheetCount(const Document& document, const PrintSettings& settings, std::size_t current, SizeF sheetMm);

} // namespace actcraft::core
```

### `formats/include/actcraft/formats/clipboard.h`
```cpp
#pragma once
#include <actcraft/core/model.h>

#include <QByteArray>
#include <QString>

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace actcraft::formats {

inline const QString kObjectsMimeType = QStringLiteral("application/x-actcraft-objects");

struct ClipboardPayload
{
    std::vector<core::PageObject> objects;        // fermés (closureForCopy), ordre de la page
    std::map<std::string, std::string> resources; // clé SHA-256 → octets
};
// {"format": "actcraft-objects/1", "objects": […], "resources": {clé: base64}} ; objets écrits par le code natif.
QByteArray encodeClipboard(const ClipboardPayload& payload);
std::optional<ClipboardPayload> decodeClipboard(const QByteArray& bytes);
std::map<std::string, std::string> resourcesFor(const core::Document& document,
                                                const std::vector<core::PageObject>& objects);

} // namespace actcraft::formats
```

### `render/include/actcraft/render/imagefill.h`
```cpp
#pragma once
#include <actcraft/core/model.h>

#include <QImage>
#include <QPoint>

namespace actcraft::render {

// Zone 4-connexe de `seed` à moins de `tolerance` (0 à 100 % de l'écart maximal par canal) ; alpha gardé.
QImage floodFill(const QImage& image, QPoint seed, core::Argb color, int tolerance);

} // namespace actcraft::render
```

### Outils (`tools/include/actcraft/tools/{filltool,penshapetool,connectortool}.h`)
Une classe `final : public Tool` chacun — `FillTool` (id « fill »), `PenShapeTool` (« pen-shape »), `ConnectorTool`
(« connector ») — avec les membres de l'interface `Tool` du lot 3 **recopiés de `selecttool.h`** (A1), plus :
```cpp
void setColor(core::Argb color);                          // FillTool
void setShape(core::PenShape shape);                      // PenShapeTool
void setPen(const core::Outline& pen, bool highlighter);  // PenShapeTool
void setStyle(std::uint8_t style);                        // ConnectorTool
```

### `export/include/actcraft/export/pageexport.h`
```cpp
#pragma once
#include <actcraft/core/model.h>

#include <QFuture>
#include <QString>
#include <QUrl>

namespace actcraft::exporting { // « export » est un mot réservé du C++

enum class ImageFormat : std::uint8_t
{
    Png, Jpeg, Bmp, Tiff,
};
ImageFormat imageFormatFor(const QUrl& url); // d'après l'extension ; PNG par défaut
struct ExportResult
{
    bool ok = false;
    QString error; // texte à nous, pour la bannière
};
// Copie du document prise par valeur sur le fil appelant ; rendu par render::renderPage sur un fil secondaire, à la
// taille de la page en pixels.
QFuture<ExportResult> exportPageImage(core::Document document, std::size_t page, QUrl url);

} // namespace actcraft::exporting
```

### `export/include/actcraft/export/pdfexport.h`
```cpp
#pragma once
#include <actcraft/export/pageexport.h>

#include <QByteArray>
#include <vector>

namespace actcraft::exporting {

// Une page PDF par page (toutes si `pages` est vide), taille de la page (points = pixels × 72 / 96), marges nulles,
// render::paintPage sur un fil secondaire.
QFuture<ExportResult> exportPdf(core::Document document, QUrl url, std::vector<std::size_t> pages = {});
int countPdfPages(const QByteArray& pdf); // dictionnaires « /Type /Page », sans Qt PDF

} // namespace actcraft::exporting
```

### `export/include/actcraft/export/pdfimport.h`
```cpp
#pragma once
#include <actcraft/core/model.h>

#include <QFuture>
#include <QString>
#include <QUrl>

namespace actcraft::exporting {

bool pdfImportAvailable(); // construit avec ACTCRAFT_IMPORT_PDF
struct ImportResult
{
    bool ok = false;
    QString error;
    core::Document document; // une page par page du PDF (D16)
};
QFuture<ImportResult> importPdf(QUrl url, int dpi = 150);

} // namespace actcraft::exporting
```

### `export/include/actcraft/export/officeconvert.h`
```cpp
#pragma once
#include <QFuture>
#include <QString>
#include <QStringList>

#include <optional>

namespace actcraft::exporting {

struct Converter
{
    QString program;
    QStringList arguments; // {in} et {out} remplacés
    QString name;
};
// ACTCRAFT_OFFICE_CONVERTER (programme puis arguments séparés par « | »), puis LibreOffice, puis PowerPoint.
std::optional<Converter> findConverter();
struct ConvertResult
{
    bool ok = false;
    QString error;
    QString pdfPath;
};
QFuture<ConvertResult> convertToPdf(const Converter& converter, const QString& presentationPath); // 120 s au plus

} // namespace actcraft::exporting
```

### `print/include/actcraft/print/printer.h`
```cpp
#pragma once
#include <actcraft/core/printlayout.h>

#include <QImage>
#include <QString>
#include <QStringList>

namespace actcraft::print {

bool printingAvailable(); // construit avec ACTCRAFT_PRINT
QStringList printerNames();
QString defaultPrinterName();
QImage renderSheet(const core::Document& document, const core::Sheet& sheet, const core::PrintSettings& settings,
                   core::SizeF sheetMm, int dpi); // gris si settings.gray
struct PrintJob
{
    QString printer;
    QString outputFile; // non vide : fichier écrit par l'imprimante
    core::PrintSettings settings;
};
struct PrintResult
{
    bool ok = false;
    QString error;
    int sheets = 0;
};
PrintResult printDocument(const core::Document& document, std::size_t current, const PrintJob& job);

} // namespace actcraft::print
```

### `ui/include/actcraft/ui/objectediting.h`
```cpp
#pragma once
#include <QObject>
#include <QPointF>
#include <QQmlEngine>

namespace actcraft::ui {
class DocumentController;
class ToolHost;

class ObjectEditing : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(ToolHost* tools READ tools WRITE setTools NOTIFY toolsChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionStateChanged)
    Q_PROPERTY(bool canPaste READ canPaste NOTIFY clipboardChanged)
    Q_PROPERTY(bool selectionLocked READ selectionLocked NOTIFY selectionStateChanged)
    Q_PROPERTY(bool selectionHidden READ selectionHidden NOTIFY selectionStateChanged)
    Q_PROPERTY(bool selectionDragCopy READ selectionDragCopy NOTIFY selectionStateChanged)
    Q_PROPERTY(bool selectionIsGroup READ selectionIsGroup NOTIFY selectionStateChanged)
    Q_PROPERTY(bool selectionIsAnnotation READ selectionIsAnnotation NOTIFY selectionStateChanged)
public:
    explicit ObjectEditing(QObject* parent = nullptr);
    DocumentController* document() const;
    void setDocument(DocumentController* document);
    ToolHost* tools() const;
    void setTools(ToolHost* tools);
    bool hasSelection() const;
    bool canPaste() const;
    bool selectionLocked() const;
    bool selectionHidden() const;
    bool selectionDragCopy() const;
    bool selectionIsGroup() const;
    bool selectionIsAnnotation() const;

    Q_INVOKABLE void cut();
    Q_INVOKABLE void copy();
    Q_INVOKABLE void paste();
    Q_INVOKABLE void pasteAt(QPointF pagePoint);
    Q_INVOKABLE void pasteOnto();
    Q_INVOKABLE void duplicate();
    Q_INVOKABLE void toggleGroup();
    Q_INVOKABLE void arrange(const QString& how);   // front, back, forward, backward, layer-top, layer-middle, layer-bottom
    Q_INVOKABLE void align(const QString& how);     // left, center-x, right, top, center-y, bottom, width, height, angle, all
    Q_INVOKABLE void transform(const QString& how); // original-size, fit-*, mirror-x|y, flip-x|y, origin-flip-x|y
    Q_INVOKABLE void toggleFlag(const QString& flag); // lock, hide, drag-copy

signals:
    void documentChanged();
    void toolsChanged();
    void selectionStateChanged();
    void clipboardChanged();

private:
    DocumentController* m_document = nullptr;
    ToolHost* m_tools = nullptr;
};

} // namespace actcraft::ui
```

### Autres en-têtes de `ui/` (même forme : `QObject` ou `QAbstractListModel`, `QML_ELEMENT`)
- `insertion.h` — `Insertion` : `Q_PROPERTY(DocumentController* document …)`, `Q_PROPERTY(bool busy READ isBusy NOTIFY
  busyChanged)`, `Q_PROPERTY(bool lastDecodeOffMainThread READ lastDecodeOffMainThread NOTIFY inserted)`,
  `Q_INVOKABLE void insertImage(const QUrl& url)`, `Q_INVOKABLE void insertMedia(const QUrl& url)`, `Q_INVOKABLE void
  insertImageData(const QImage& image)` (collage, L5-T14), `Q_INVOKABLE bool acceptDrop(const QVariantList& urls, const
  QString& text, QPointF pagePoint)` ; signaux `busyChanged()`, `inserted(int count)`, `failed(QString message)`.
- `documenttabs.h` — `DocumentTabs : QAbstractListModel` : propriétés `document` (`DocumentController*`),
  `currentIndex` (lecture), `position` (« top », « bottom », « hidden », lu et écrit) ; rôles `TitleRole`,
  `ModifiedRole`, `CurrentRole`, `UrlRole` ; `Q_INVOKABLE void newTab()`, `openInNewTab(const QUrl&)`,
  `activate(int)`, `bool requestClose(int)` (faux : `saveRequested(int)` émis), `closeWithoutSaving(int)`,
  `copyPagesToTab(const QVariantList& pageIndexes, int tabIndex)` (F-011 : avant la première page), `next()`,
  `previous()` ; signaux `documentChanged`, `currentIndexChanged`, `positionChanged`, `saveRequested(int)`.
- `fileexchange.h` — `FileExchange` : propriétés `document`, `tabs` (`DocumentTabs*`), `busy`, `pdfImportAvailable`
  (constante), `powerpointAvailable` (`NOTIFY converterChanged`) ; `Q_INVOKABLE void exportPdf(const QUrl&)`,
  `exportPageImage(const QUrl&)`, `importPdf(const QUrl&)`, `importPowerpointImages(const QUrl&)`,
  `printThroughPdf()` (Android, N1-15) ; signaux `busyChanged`, `converterChanged`, `finished(QString)`,
  `failed(QString)`.
- `printcontroller.h` — `PrintController` : propriétés lues et écrites `rows`, `columns`, `frames`, `pageNumbers`,
  `range` (« all », « current », « from-to »), `from`, `to`, `copies`, `marginMm`, `gray`, `notes` (« none »,
  « with-pages », « only »), `noteBreak`, `noteSeparator`, `printer` ; `printers` (constante), `sheetCount` (lecture,
  `NOTIFY settingsChanged`), `document` ; `Q_INVOKABLE QImage previewSheet(int index, int dpi)`,
  `Q_INVOKABLE bool print(const QString& outputFile = {})`, `Q_INVOKABLE void saveSettings()`,
  `Q_INVOKABLE void loadSettings()` ; signal `settingsChanged`.

### Ajout à `ui/include/actcraft/ui/documentcontroller.h` (A5), en public
```cpp
// Document garé par un onglet (L5-T20) : document, pile d'annulation, URL, état modifié, avertissements, page affichée.
struct ParkedSession;
// Rend la session affichée et en installe une vide ; dépose d'abord un soulèvement en cours (dropLifted(nullptr)).
std::unique_ptr<ParkedSession> parkSession();
// Installe une session garée : la page affichée est reconstruite une fois (cache redessiné une fois).
void resumeSession(std::unique_ptr<ParkedSession> session);
```
Corps talons dans `documentcontroller.cpp` : `struct ParkedSession {};`, `parkSession` rend `nullptr`,
`resumeSession` ne fait rien, chacun précédé de `// Talon L5-T1 : rempli par L5-T20.`

## Hypothèses pour Jérôme (niveau 1 d'abord)

- **H5-1** (N1) — Imports retenus à la porte du lot 2 : PowerPoint en images au lot 5 (réutilise l'import PDF), packs
  de ressources au lot 6 (exigent le navigateur de ressources).
- **H5-2** (N1) — Qt PDF installé sur le poste et en CI (`aqt … -m qtpdf`), licence LGPLv3, liste
  `packaging/qt-autorises.txt` complétée (L5-T22) ; sans lui, l'import PDF et PowerPoint sont déclarés absents.
- **H5-3** (N1) — PowerPoint en images passe par un convertisseur installé (LibreOffice, sinon PowerPoint) ; non
  livré, Windows seulement ; Android : commande absente.
- **H5-4** (N2) — Pot de peinture sur l'intersection de formes : forme du dessus remplie tant que L5-T0 ne l'a pas
  tranché.
- **H5-5** (N2) — Une seule vue pour tous les onglets (mémoire du cache MSAA) : changer d'onglet redessine le cache une
  fois.
- **H5-6** (N2) — Organiser (avancer, reculer) garde le redessin immédiat du cache (insertion hors fin de page, lot 10) :
  mesuré par le banc `edition`.
