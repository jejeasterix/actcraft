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
| L5-T1 | Réservation I : commandes, disposition, icônes, CMake (modules `export/` et `print/`), 26 tests enregistrés, en-têtes figés et talons des deux sous-lots | — | `actions/data/commands.json`, `actions/data/layout.json`, `ui/icons/{couper,copier,coller,dupliquer,grouper,degrouper,organiser,aligner,transformer,verrouiller,masquer,glisser-copie,points-forme,pot-peinture,connecteur,modification-stylo,menu-objet,image,media,onglets,exporter,imprimer,importer}.svg`, `ui/CMakeLists.txt`, `CMakeLists.txt`, `core/CMakeLists.txt`, `formats/CMakeLists.txt`, `tools/CMakeLists.txt`, `export/CMakeLists.txt`, `print/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/{core,formats,tools,ui}.cmake`, `tests/export.cmake`, `tests/print.cmake`, `core/include/actcraft/core/{clipboardedits,arrange,alignment,objectflags,fill,shapepoints,connectors,penmodifiers,printlayout}.h`, `core/src/{clipboardedits,arrange,alignment,objectflags,fill,shapepoints,connectors,penmodifiers,printlayout}.cpp`, `formats/include/actcraft/formats/clipboard.h`, `formats/src/clipboard.cpp`, `render/include/actcraft/render/imagefill.h`, `render/src/imagefill.cpp`, `tools/include/actcraft/tools/{filltool,penshapetool,connectortool}.h`, `tools/src/{filltool,penshapetool,connectortool}.cpp`, `export/include/actcraft/export/{pageexport,pdfexport,pdfimport,officeconvert}.h`, `export/src/{pageexport,pdfexport,pdfimport,officeconvert}.cpp`, `print/include/actcraft/print/printer.h`, `print/src/printer.cpp`, `ui/include/actcraft/ui/{objectediting,insertion,documenttabs,fileexchange,printcontroller}.h`, `ui/src/{objectediting,insertion,documenttabs,fileexchange,printcontroller}.cpp`, `ui/include/actcraft/ui/documentcontroller.h` (déclarations `parkSession` / `resumeSession` seulement), `ui/src/documentcontroller.cpp` (talons de ces deux membres seulement), 26 fichiers `tests/tst_*.cpp` de la carte, `tests/tst_actions_layout.cpp` | isolé | implementeur-tdd | build 0 avertissement ; `ctest -N` : N = N_départ + 26 ; `tst_actions_layout` Passed dont `lot5EntriesDeclared`, `objectMenuOrder` ; les 26 talons en SKIP nommant leur tâche ; `NOMS OK` | S8, S17 | #tache-l5-t1 |
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
