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
| L5-T2 | Réservation II : coquille QML (menu d'édition d'objet, barre d'objet complète, onglets, emplacements export, impression, import ; gestionnaires des commandes du lot) | L5-T1 | `ui/qml/Main.qml`, `ui/qml/PageArea.qml`, `ui/qml/ObjectBar.qml`, `ui/qml/TopBar.qml`, `ui/qml/{ObjectEditMenu,PenModifierBar,ConnectorPanel,DocumentTabBar,FileExchangeActions,PrintDialog,PrintPreview}.qml` (neufs, talons), `ui/CMakeLists.txt` (bloc `QML_FILES` seulement), `tests/tst_ui_shell.cpp` | isolé | implementeur-tdd | `tst_ui_shell` Passed dont `everyLot5CommandHasAHandler`, `lot5SlotsExist`, `shellLoadsWithoutQmlWarning` ; `premiers-pas` hors écran `CAPTURES OK` | S8 | #tache-l5-t2 |
| L5-T3 | `formats` : presse-papiers d'objets (objets, ressources, groupes et connecteurs, sans perte) | L5-T0, L5-T1 | `formats/src/clipboard.cpp`, `tests/tst_formats_clipboard.cpp` | isolé | concepteur-critique | `tst_formats_clipboard` Passed (dont `roundTripKeepsEveryKind`, `resourcesTravelWithObjects`, `groupMembersTravelTogether`, `foreignPayloadRejected`) ; mutation rouge puis remise | S8, S8.1 | #tache-l5-t3 |
| L5-T5 | `core` : organiser (avancer, reculer, premier plan, arrière-plan, couches) et groupes (grouper, dégrouper, groupes de groupes) | L5-T0, L5-T1 | `core/src/arrange.cpp`, `tests/tst_core_arrange.cpp` | isolé | implementeur-tdd | `tst_core_arrange` Passed (dont `forwardSwapsOneStep`, `frontKeepsRelativeOrder`, `layerChangeGoesOnTop`, `groupIsOneUndoStep`, `ungroupRestoresInnerGroups`, `reorderChangesSeenByCache`) | S8, S8.2 | #tache-l5-t5 |
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
| L5-T18 | QML : menu d'édition d'objet, sous-menus, barre d'objet complète, barre de modification de stylo, panneau des connecteurs ; scénario `edition` | L5-T14, L5-T15, L5-T17 | `ui/qml/ObjectEditMenu.qml`, `ui/qml/PenModifierBar.qml`, `ui/qml/ConnectorPanel.qml`, `ui/qml/ObjectBar.qml`, `bench/scenarios/edition.json`, `tests/tst_ui_shell.cpp` | isolé | implementeur-tdd | `tst_ui_shell` Passed dont `objectMenuMatchesLayout`, `convertEntriesOnlyForAnnotations` ; `edition` hors écran `CAPTURES OK` avec `-Lire` (§ section) ; banc hors écran joint | S8, S8.2, S8.3 | #tache-l5-t18 |
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
(dialogues de fichiers d'export et d'import), `PrintDialog` et `PrintPreview` ; chaque fichier neuf est un talon
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
                             "printDialog", "printPreview"})
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
