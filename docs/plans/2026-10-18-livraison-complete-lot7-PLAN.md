# Livraison complète — lot 7 « Présentation et outils » — sous-lot 7a — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**Session et branche.** Ce lot est exécuté par une **seconde session orchestratrice**, chantier
`livraison-complete-b`, branche `feat/livraison-complete-b` (issue de `feat/livraison-complete` après la livraison du
lot 10), fichier d'état `docs/sessions/livraison-complete-b-etat.md`, en parallèle du lot 3 (sélection, gestes, gomme,
annulation) qui avance sur `feat/livraison-complete`. **Ce plan s'exécute sans rien connaître du lot 3** : aucune tâche
avant `L7-T33` (fusion, sous-lot 7b) ne lit, n'attend ni ne suppose un fichier, un nom ou un comportement du lot 3.
Les seules fonctions qui en ont besoin sont regroupées dans `L7-T34`, déclarée « après fusion du lot 3 ».

**Deux sous-lots, une PR, une version** (amendement 1, § 6 : plus de 20 tâches). Sous-lot 7a : ce fichier,
`L7-T0` à `L7-T19`. Sous-lot 7b : `docs/plans/2026-10-18-livraison-complete-lot7b-PLAN.md`, `L7-T20` à `L7-T34`,
exécuté après la fin de 7a. Une seule PR, depuis `feat/livraison-complete-b` vers `feat/livraison-complete`, à la fin
de 7b. Version : attribuée à la fusion (le lot 3 prend 0.4.0 ; le lot 7 prend la suivante dans l'ordre de fusion) ;
**aucune tâche n'écrit de numéro de version dans le code** ni dans `CHANGELOG.md`.

**But.** Les fonctions de présentation et les outils d'ActivInspire 3.5.18 (référence sous Windows, interface libre
sous Android, décision 0100) : rideau, spot, zoom de page et leurs réglages par page ; catalogue d'effets de
changement de page ; règle, équerre, rapporteur, compas, origine XY ; dés, calculatrice, horloge et compte à
rebours, date et heure, navigateur web, message défilant (7a) ; reconnaissance de forme (algorithme à nous),
reconnaissance d'écriture (Windows, moteur d'encre du système), équation (lue et écrite sans perte), caméra,
clavier à l'écran, annotation de bureau et outils de bureau (7b). Exigences : F-070, F-071, F-080, F-124, F-260,
F-265, F-270, F-275, F-280 à F-286, F-290, F-291, F-294, F-295, F-296, F-299, F-300, F-301 (équation), F-591,
F-592, F-593, F-640, F-750 (entrées du lot).

**Architecture.** Trois couches, qui ne touchent ni aux outils existants ni au cache :
1. **Logique pure dans `features/`** (QtCore + `core` + `input`, sans QtGui ni Qt Quick) : géométrie du rideau et du
   spot, zoom continu, outils de page, catalogue d'effets, outils de maths et **guides de tracé**, dés,
   calculatrice, compte à rebours, message défilant, reconnaissance de forme, sessions de reconnaissance, modèle
   d'édition d'équation, clavier. Tout y est testé sans écran.
2. **Contrôleurs QML dans `ui/src/`** (un `QObject` `QML_ELEMENT` par fonction, déclaré dans `ui/presentation.cmake`,
   fichier propre au lot 7) qui relient la logique aux commandes et au document.
3. **Surcouches et panneaux QML** (`ui/qml/*.qml` du lot 7), posés dans deux emplacements réservés par `L7-T2` :
   `pageOverlays` (dans la page, suit le zoom et le défilement) et `presentationLayer` (au-dessus de la fenêtre).
   Rideau, spot, zoom de page, outils de maths, message défilant et effets sont des **surcouches** : ils ne
   redessinent pas le cache de la page (prouvé par `QSignalSpy` sur `PageView::cacheRefreshRequested` et par
   `cache.refreshes` du banc). Les outils de maths sont des **aides, pas du contenu** : ils ont leurs propres
   poignées (déplacer, tourner, allonger), ne sont pas des objets de page, ne passent ni par la sélection ni par les
   gestes du lot 3 — c'est ainsi que la référence Python les réalise (`app/actcraft/extras/mathtools.py`, en-tête :
   éléments posés au-dessus de la page, ignorés à l'enregistrement, interactions par un filtre propre). Seule l'encre
   qu'ils guident entre au document, par le stylo existant, à travers un **filtre d'échantillons** de la vue
   (`PageView::setSampleFilter`, réservé par `L7-T2`) : le stylo n'est pas modifié.

Les nouveaux objets de page (équation, image de caméra, texte de date, de dés ou d'écriture, forme reconnue) entrent
par la pile d'annulation (`core::AddObjectCommand`, `core::ReplaceObjectsCommand` de `L7-T1`) et donc par l'interface
de cache figée (`core::PageCache`, DA11) : aucun appel direct au rendu.

**Pile.** C++20, Qt 6.12 (Core, Gui, Quick, QuickControls2, Test), CMake + Ninja, MSVC 2022, PowerShell 7.

## Table des tâches (sous-lot 7a)

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L7-T0 | Ouverture : vidéos, REA, spécification et ancien dépôt confrontés au lot 7 ; écarts E5, E9, E11 tranchés | — | `docs/verifications/AAAA-MM-JJ-lot7-principes.md` (date d'exécution) ; hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot7.md` | isolé | concepteur-critique | rapport commité, relevé brut hors dépôt présent, `NOMS OK`, `git grep` des titres du corpus vide, N0 (tests de la base) relevé | S30 | #tache-l7-t0 |
| L7-T1 | Réservation I (données et modèle) : commandes du lot, argument texte, type « équation », réglages de présentation de la page, remplacement annulable, format natif et `.flipchart` | — | `actions/data/commands.json`, `actions/include/actcraft/actions/commandregistry.h`, `actions/src/commandregistry.cpp`, `tests/tst_actions_registry.cpp`, `core/include/actcraft/core/model.h`, `core/src/model.cpp`, `core/src/edits.cpp`, `core/include/actcraft/core/replace.h`, `core/src/replace.cpp`, `core/include/actcraft/core/equation.h`, `core/src/equation.cpp`, `core/CMakeLists.txt`, `tests/tst_core_model.cpp`, `formats/src/nativejson.cpp`, `tests/tst_formats_nativejson.cpp`, `formats/src/flipchart.cpp`, `formats/src/flipchartids.h`, `tests/support/flipchartwriter.h`, `tests/tst_formats_flipchart.cpp`, `render/src/pagepainter.cpp`, `render/src/paintedrenderer.cpp`, `render/src/equationlayout.h`, `render/src/equationlayout.cpp`, `render/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/presentation.cmake`, `tests/tst_core_presentation.cpp` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 1, `tst_core_presentation` Passed (dont `replaceIsOneUndoStep`, `fingerprintUnchangedWithoutPresentation`), `tst_formats_nativejson` Passed (dont `equationRoundTripKeepsMathml`, `presentationRoundTrip`), `tst_formats_flipchart` Passed (dont `mathmlObjectBecomesEquation`), `tst_actions_registry` Passed (dont `textArgument`) | S24, S10 | #tache-l7-t1 |
| L7-T2 | Réservation II (interface) : sous-menus de la disposition, menus et palette du lot, emplacements de surcouches, filtre d'échantillons, zoom autour d'un point, API d'insertion du contrôleur, étapes de scénario `pointer` et `doubleClick` | L7-T1 | `actions/data/layout.json`, `actions/include/actcraft/actions/layout.h`, `actions/src/layout.cpp`, `tests/tst_actions_layout.cpp`, `ui/src/commands.cpp`, `ui/qml/CommandMenu.qml`, `tests/tst_ui_commands.cpp`, `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`, `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`, `ui/qml/PageArea.qml`, `ui/qml/Main.qml`, `ui/qml/Presentation.qml`, `ui/qml/FloatingPanel.qml`, `ui/CMakeLists.txt`, `ui/presentation.cmake`, `features/CMakeLists.txt`, `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`, `tests/tst_bench_uiscenario.cpp`, `tests/presentation.cmake`, `tests/tst_ui_presentation.cpp` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 2, `tst_ui_presentation` Passed (dont `overlaySlotsExist`, `sampleFilterReachesThePen`, `insertTextIsOneUndoStep`, `zoomAroundKeepsThePointStill`), `tst_actions_layout` (dont `submenuParsed`), `tst_ui_commands` (dont `submenuFilteredByPlatform`), `tst_bench_uiscenario` (dont `pointerStepParsed`) Passed ; banc hors écran joint ; `captures.ps1 -Scenario premiers-pas` hors écran `CAPTURES OK` | S10 | #tache-l7-t2 |
| L7-T3 | Rideau (F-265) : store opaque, tirage depuis les quatre bords, couleur, bascule Ctrl+R, sans redessin du cache | L7-T0, L7-T2 | `features/include/actcraft/features/curtain.h`, `features/src/curtain.cpp`, `features/CMakeLists.txt`, `ui/src/curtaincontroller.h`, `ui/src/curtaincontroller.cpp`, `ui/qml/CurtainOverlay.qml`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `ui/icons/rideau.svg`, `tests/tst_features_curtain.cpp`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake`, `bench/scenarios/rideau.json` | isolé | implementeur-tdd | `TESTS OK`, N ≥ N0 + 4, `tst_features_curtain` Passed (dont `dragFromTopUncovers`, `dragFromLeftUncovers`), `tst_ui_presentation` (dont `curtainNeverRefreshesTheCache`), `tst_captures_rideau` Passed ; `captures.ps1 -Scenario rideau` hors écran : `LU curtain.coverage=…` puis `PIXEL` couvert / découvert OK ; banc hors écran joint | S10 | #tache-l7-t3 |
| L7-T4 | Spot (F-270) : quatre formes, déplacer et redimensionner, sans redessin du cache | L7-T3 | `features/include/actcraft/features/spotlight.h`, `features/src/spotlight.cpp`, `features/CMakeLists.txt`, `ui/src/spotlightcontroller.h`, `ui/src/spotlightcontroller.cpp`, `ui/qml/SpotlightOverlay.qml`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `ui/icons/spot.svg`, `tests/tst_features_spotlight.cpp`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake`, `bench/scenarios/spot.json` | isolé | implementeur-tdd | `TESTS OK`, N ≥ N0 + 6, `tst_features_spotlight` Passed (dont `dragInsideMaskMoves`, `dragOnRimResizesToOval`, `solidSpotHidesInside`), `tst_ui_presentation` (dont `spotlightNeverRefreshesTheCache`), `tst_captures_spot` Passed ; banc hors écran joint | S10 | #tache-l7-t4 |
| L7-T5 | Zoom de page (F-260) : appui long gauche = zoom avant continu, droit = zoom arrière, glisser = panoramique, double-clic = ajuster | L7-T2 | `features/include/actcraft/features/pagezoom.h`, `features/src/pagezoom.cpp`, `features/CMakeLists.txt`, `ui/src/pagezoomcontroller.h`, `ui/src/pagezoomcontroller.cpp`, `ui/qml/PageZoomOverlay.qml`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `ui/icons/zoom-page.svg`, `tests/tst_features_pagezoom.cpp`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake` | isolé | implementeur-tdd | `TESTS OK`, N ≥ N0 + 7, `tst_features_pagezoom` Passed (dont `holdZoomsInAroundThePoint`, `doubleClickFits`), `tst_ui_presentation` (dont `pageZoomRefreshesTheCacheOnceAtRelease`, `pageZoomLeavesTheDocumentUnchanged`) ; banc hors écran joint | S10 | #tache-l7-t5 |
| L7-T6 | Outils de page par page (F-591, F-592, F-593) : « comme avant », rideau, spot, désactivés ; mode et distance du rideau, mode et cadre du spot ; lecture des réglages de page des `.flipchart` | L7-T3, L7-T4 | `features/include/actcraft/features/pagetools.h`, `features/src/pagetools.cpp`, `features/CMakeLists.txt`, `ui/src/pagetoolscontroller.h`, `ui/src/pagetoolscontroller.cpp`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `formats/src/flipchart.cpp`, `formats/src/flipchartids.h`, `tests/support/flipchartwriter.h`, `tests/tst_formats_flipchart.cpp`, `tests/tst_features_pagetools.cpp`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 8, `tst_features_pagetools` Passed (dont `keepCarriesCurtainAcrossPages`, `offStopsTools`, `pageModeOverridesKeep`), `tst_formats_flipchart` (dont `pagePresentationRead`), `tst_ui_presentation` (dont `pageChangeAppliesPageTools`) | S10 | #tache-l7-t6 |
| L7-T7 | Effets de changement de page (F-640) I : catalogue, effet global et par page, durée, règles de désactivation, lecteur d'effets en surcouche | L7-T0, L7-T6 | `features/include/actcraft/features/transitions.h`, `features/src/transitions.cpp`, `features/CMakeLists.txt`, `ui/src/transitioncontroller.h`, `ui/src/transitioncontroller.cpp`, `ui/qml/TransitionLayer.qml`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `tests/tst_features_transitions.cpp`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 9, `tst_features_transitions` Passed (dont `pageEffectWinsOverGlobal`, `effectPlayedWhenLeavingThePage`, `disabledWhileCurtainActive`, `durationClampedTo5To30`), `tst_ui_presentation` (dont `transitionDoesNotRefreshTheCacheBeforeTheEnd`) ; banc hors écran joint | S22 | #tache-l7-t7 |
| L7-T8 | Effets (F-640) II : navigateur d'effets, banc `effets` (chaque effet mesuré), scénario de captures | L7-T7 | `ui/qml/TransitionBrowser.qml`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `ui/icons/effets.svg`, `bench/scenarios/effets.json`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake` | isolé | implementeur-tdd | `TESTS OK`, N ≥ N0 + 10, `tst_captures_effets` Passed, `tst_ui_presentation` (dont `browserListsEveryEffect`) ; banc hors écran : `-Scenario court` joint + `captures.ps1 -Scenario effets` `LU transition.played=…` | S22 | #tache-l7-t8 |
| L7-T9 | Règle (F-280) et guides de tracé : déplacer, allonger, tourner (angle affiché), stylo accroché au bord, unités | L7-T2 | `features/include/actcraft/features/strokeguide.h`, `features/src/strokeguide.cpp`, `features/include/actcraft/features/mathtools.h`, `features/src/mathtools.cpp`, `features/CMakeLists.txt`, `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/MathToolsLayer.qml`, `ui/qml/RulerTool.qml`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `ui/icons/regle.svg`, `tests/tst_features_mathtools.cpp`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake`, `bench/scenarios/regle.json` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 12, `tst_features_mathtools` Passed (dont `rulerSnapsNearEdge`, `rulerIgnoresFarSamples`, `strokeStaysOnTheEdgeOnceSnapped`), `tst_ui_presentation` (dont `rulerGuidesThePenWithoutChangingIt`, `mathToolIsNotSaved`), `tst_captures_regle` Passed ; banc hors écran joint | S10 | #tache-l7-t9 |
| L7-T10 | Équerre (F-283) : trois bords guides, angle, déplacer, tourner, redimensionner | L7-T9 | `features/include/actcraft/features/mathtools.h`, `features/src/mathtools.cpp`, `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/SetSquareTool.qml`, `ui/qml/MathToolsLayer.qml`, `ui/presentation.cmake`, `ui/icons/equerre.svg`, `tests/tst_features_mathtools.cpp`, `tests/tst_ui_presentation.cpp` | isolé | implementeur-tdd | `TESTS OK`, N ≥ N0 + 12, `tst_features_mathtools` (dont `setSquareHasThreeEdges`, `setSquareHypotenuseAt45`) ; banc hors écran joint | S10 | #tache-l7-t10 |
| L7-T11 | Rapporteur (F-281) : complet ou demi, arc, arc fermé, arc plein, angle affiché | L7-T10 | `features/include/actcraft/features/mathtools.h`, `features/src/mathtools.cpp`, `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/ProtractorTool.qml`, `ui/qml/MathToolsLayer.qml`, `ui/presentation.cmake`, `ui/icons/rapporteur.svg`, `tests/tst_features_mathtools.cpp`, `tests/tst_ui_presentation.cpp` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 12, `tst_features_mathtools` (dont `protractorArcFollowsTheRim`, `closedArcAddsTwoRadii`, `halfProtractorStopsAt180`) ; banc hors écran joint | S10 | #tache-l7-t11 |
| L7-T12 | Compas (F-282) : pointe, bras, rayon, arc ou cercle tracé avec le stylo, rayon et angle affichés | L7-T11 | `features/include/actcraft/features/mathtools.h`, `features/src/mathtools.cpp`, `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/CompassTool.qml`, `ui/qml/MathToolsLayer.qml`, `ui/presentation.cmake`, `ui/icons/compas.svg`, `tests/tst_features_mathtools.cpp`, `tests/tst_ui_presentation.cpp` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 12, `tst_features_mathtools` (dont `compassDrawsAnArcAtItsRadius`, `fullTurnClosesTheCircle`) ; banc hors écran joint | S10 | #tache-l7-t12 |
| L7-T13 | Origine XY (F-284) : point déplaçable, mémoire par page, bascule (le pivot de rotation attend le lot 3 : `L7-T34`) | L7-T12 | `features/include/actcraft/features/mathtools.h`, `features/src/mathtools.cpp`, `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/XyOriginTool.qml`, `ui/qml/MathToolsLayer.qml`, `ui/presentation.cmake`, `ui/icons/origine.svg`, `tests/tst_features_mathtools.cpp`, `tests/tst_ui_presentation.cpp` | isolé | implementeur-tdd | `TESTS OK`, N ≥ N0 + 12, `tst_features_mathtools` (dont `originRememberedPerPage`), `tst_ui_presentation` (dont `originSavedWithThePage`) | S10 | #tache-l7-t13 |
| L7-T14 | Dés (F-285, E5) : 1 à 5 dés, vitesse, lancer animé, total écrit sur la page | L7-T2, L7-T0 | `features/include/actcraft/features/dice.h`, `features/src/dice.cpp`, `features/CMakeLists.txt`, `ui/src/dicecontroller.h`, `ui/src/dicecontroller.cpp`, `ui/qml/DicePanel.qml`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `ui/icons/des.svg`, `tests/tst_features_dice.cpp`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake` | isolé | implementeur-tdd | `TESTS OK`, N ≥ N0 + 13, `tst_features_dice` Passed (dont `seededRollIsReproducible`, `countClampedTo1To5`), `tst_ui_presentation` (dont `diceTotalWrittenOnThePage`) | S10 | #tache-l7-t14 |
| L7-T15 | Calculatrice (F-286) : la nôtre par défaut, calculatrice du système ou programme choisi (Windows) | L7-T2 | `features/include/actcraft/features/calculator.h`, `features/src/calculator.cpp`, `features/CMakeLists.txt`, `ui/src/calculatorcontroller.h`, `ui/src/calculatorcontroller.cpp`, `ui/qml/CalculatorPanel.qml`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `ui/icons/calculatrice.svg`, `tests/tst_features_calculator.cpp`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake` | isolé | implementeur-tdd | `TESTS OK`, N ≥ N0 + 14, `tst_features_calculator` Passed (dont `operatorPrecedenceLeftToRight`, `divisionByZeroIsAnError`, `percentAndSign`) | S10 | #tache-l7-t15 |
| L7-T16 | Horloge et compte à rebours (F-290, E9) : analogique, numérique, les deux, pause, minuterie, compte à rebours, répéter, action de fin | L7-T2, L7-T0 | `features/include/actcraft/features/countdown.h`, `features/src/countdown.cpp`, `features/CMakeLists.txt`, `ui/src/clockcontroller.h`, `ui/src/clockcontroller.cpp`, `ui/qml/ClockPanel.qml`, `ui/qml/CountdownDialog.qml`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `ui/icons/horloge.svg`, `tests/tst_features_countdown.cpp`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake`, `bench/scenarios/compte-a-rebours.json` | isolé | implementeur-tdd | `TESTS OK`, N ≥ N0 + 16, `tst_features_countdown` Passed (dont `endTriggersActionOnce`, `repeatRestarts`, `pauseFreezes`), `tst_captures_compte_a_rebours` Passed (`LU pageIndicator.text=2 / 2` après 3 s) | S10 | #tache-l7-t16 |
| L7-T17 | Date et heure (F-300) et navigateur web (F-299) | L7-T2 | `ui/src/presentationcommands.h`, `ui/src/presentationcommands.cpp`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `tests/tst_ui_presentation.cpp` | isolé | implementeur-tdd | `TESTS OK`, N ≥ N0 + 16, `tst_ui_presentation` (dont `dateTimeInsertsOneText`, `webBrowserOpensTheHomeUrl`) | S10 | #tache-l7-t17 |
| L7-T18 | Message défilant (F-291) I : modèle, surcouche animée sur le fil de rendu, position, boucle, vitesse, pause ; banc pendant le défilement | L7-T2 | `features/include/actcraft/features/ticker.h`, `features/src/ticker.cpp`, `features/CMakeLists.txt`, `ui/src/tickercontroller.h`, `ui/src/tickercontroller.cpp`, `ui/qml/TickerOverlay.qml`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `tests/tst_features_ticker.cpp`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 17, `tst_features_ticker` Passed (dont `singlePassEndsHidden`, `loopRestarts`, `speedMapsToPixelsPerSecond`), `tst_ui_presentation` (dont `tickerNeverRefreshesTheCache`) ; banc hors écran avec message en boucle joint | S23 | #tache-l7-t18 |
| L7-T19 | Message défilant (F-291, E11) II : fenêtre d'édition (texte, police, couleur, ombre, fond, position, boucle, vitesse), « appliquer » = défaut gardé, enregistrer et charger des messages | L7-T18, L7-T0 | `ui/qml/TickerDialog.qml`, `ui/src/tickercontroller.h`, `ui/src/tickercontroller.cpp`, `features/include/actcraft/features/ticker.h`, `features/src/ticker.cpp`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `ui/icons/message.svg`, `tests/tst_features_ticker.cpp`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake`, `bench/scenarios/message.json` | isolé | implementeur-tdd | `TESTS OK`, N ≥ N0 + 18, `tst_features_ticker` (dont `messageFileRoundTrip`), `tst_captures_message` Passed, `tst_ui_presentation` (dont `applyKeepsTheDefault`) | S23 | #tache-l7-t19 |

20 tâches dans ce sous-lot (15 dans 7b, voir son plan). **N0** = nombre de tests ctest de la base
`feat/livraison-complete-b` au départ (relevé par L7-T0 ; 94 attendu après le lot 10). Tests ajoutés en 7a :
`tst_core_presentation`, `tst_ui_presentation`, `tst_features_curtain`, `tst_captures_rideau`,
`tst_features_spotlight`, `tst_captures_spot`, `tst_features_pagezoom`, `tst_features_pagetools`,
`tst_features_transitions`, `tst_captures_effets`, `tst_features_mathtools`, `tst_captures_regle`,
`tst_features_dice`, `tst_features_calculator`, `tst_features_countdown`, `tst_captures_compte_a_rebours`,
`tst_features_ticker`, `tst_captures_message` : **N0 + 18** à la fin de 7a. « ≥ » est la règle.

Worktrees : `.worktrees/livraison-complete-b--L7-T<n>` (profil), tous isolés : deux tâches d'une même vague ne
partagent aucun fichier (colonne « fichiers touchés ») ; les tâches qui partagent un fichier sont reliées par la
colonne « dépend de » ou placées dans des vagues successives.

**Écran.** Aucune tâche de 7a n'ouvre de fenêtre visible : tout se joue hors écran (rendu logiciel). Les passages à
l'écran (D3D11, jeton `ecran`) sont ceux de la clôture du lot (sous-lot 7b, « Clôture »).

## Vagues (session B : une seule place de tests)

Règle de la session B : **une seule tâche construit à la fois** ; une vague a au plus deux tâches, dont une sans
construction (relevé, documentation).

V1 : L7-T0 (sans construction) ‖ L7-T1 · V2 : L7-T2 · V3 : L7-T3 · V4 : L7-T4 · V5 : L7-T5 · V6 : L7-T6 ·
V7 : L7-T7 · V8 : L7-T8 · V9 : L7-T9 · V10 : L7-T10 · V11 : L7-T11 · V12 : L7-T12 · V13 : L7-T13 · V14 : L7-T14 ·
V15 : L7-T15 · V16 : L7-T16 · V17 : L7-T17 · V18 : L7-T18 · V19 : L7-T19.

Fichiers partagés, tenus par l'ordre des vagues : `ui/qml/Presentation.qml`, `ui/presentation.cmake`,
`tests/presentation.cmake`, `tests/tst_ui_presentation.cpp`, `features/CMakeLists.txt` (L7-T2 puis chaque tâche de
fonction, une à la fois) ; `features/…/mathtools.h`, `features/src/mathtools.cpp`, `ui/src/mathtoolscontroller.*`,
`ui/qml/MathToolsLayer.qml`, `tests/tst_features_mathtools.cpp` (L7-T9 → T10 → T11 → T12 → T13) ;
`formats/src/flipchart.cpp`, `formats/src/flipchartids.h`, `tests/support/flipchartwriter.h`,
`tests/tst_formats_flipchart.cpp` (L7-T1 → L7-T6) ; `features/…/ticker.h`, `features/src/ticker.cpp`,
`ui/src/tickercontroller.*`, `tests/tst_features_ticker.cpp` (L7-T18 → L7-T19).

**L7-T0 et les autres tâches.** L7-T1 ne l'attend pas : la réservation pose des noms et des données
indépendants de ses constats (l'équation est gardée en MathML brut, quel que soit le jeu de modèles retenu). Les
tâches de fonction l'attendent (L7-T3 en tête, donc toutes par la chaîne). Si L7-T0 rapporte un écart qui change une
tâche, l'orchestrateur amende ce plan **avant V3** (tâche ajoutée ou section corrigée, relecture du plan exigée) ;
l'amendement 1 prévoit jusqu'à trois tâches de principes pour le lot 7 (§ 4) : elles s'ajoutent alors à 7a ou 7b
selon leurs dépendances.

## Ce que l'ancien dépôt dit déjà (relevé pour ce plan, reformulé)

Lu dans `C:\Users\JeromeDenis\REA - Activinspire` ; L7-T0 le confirme ou le corrige par le corpus et le binaire.

1. **Outils de maths = aides, pas contenu.** La référence Python les pose au-dessus de toutes les couches, les ignore
   à l'enregistrement et dans les miniatures, et gère leurs interactions elle-même ; seule l'encre tracée le long de
   leurs bords entre au document, par la pile d'annulation. Distance d'accrochage du stylo : 16 pixels d'écran ;
   1 cm = 96 / 2,54 pixels de page. → L7-T9 à L7-T13 (aucune dépendance au lot 3).
2. **Rideau et spot = surcouches** avec leur propre modèle (couverture par bord, cadre du spot redimensionnable en
   ovale ou rectangle). → L7-T3, L7-T4.
3. **Reconnaissance de forme** : ligne, rectangle ou carré, ellipse ou cercle, triangle ; sinon le trait reste une
   annotation (`app/actcraft/canvas/recognition.py`). L'algorithme du lot 7 est **écrit par nous** (ré-échantillonnage,
   coins, écart radial), testé sur nos propres tracés synthétiques. → L7-T20.
4. **Reconnaissance d'écriture** : moteur d'encre de Windows par COM (reconnaisseur français préféré, meilleur texte
   puis alternatives) ; indisponible → commande grisée avec une explication. → L7-T22, L7-T23.
5. **Réglages de page lus dans les `.flipchart`** : mode et distance du rideau, mode et cadre du spot, origine XY,
   effet propre à la page (notre documentation du format, § de la page). → L7-T6 (rideau, spot, origine), L7-T7
   (effet) ; lève l'hypothèse N2-A12 pour l'effet.
6. **Clavier à l'écran** : la référence Python lance le clavier du système ; le cadrage (décision 34) veut un clavier
   **écrit par nous** (pas de Qt Virtual Keyboard, GPL). → L7-T29.

## Écarts au cadrage (repris à la porte de fin du lot 7)

1. **Volume.** 35 tâches (7a : 20, 7b : 15) contre 29 estimées (amendement 1, § 6) : deux tâches de réservation, une
   tâche de fusion et une tâche « après fusion du lot 3 » viennent de l'exécution en parallèle ; une tâche de
   contrôle du lot (L7-T32). Les tâches de principes (jusqu'à 3) s'ajouteront après L7-T0.
2. **Profils (amendement 1, N2-6 amendée).** La règle « chaque lot qui livre un outil épinglé complète ce profil »
   suppose les profils du lot 6, qui vient **après** le lot 7 dans l'ordre en vigueur. Le lot 7 ne peut pas les
   compléter : L7-T32 écrit la liste des commandes du lot 7 à épingler (identifiants stables) dans
   `docs/verifications/AAAA-MM-JJ-lot7-epinglages.md`, que le lot 6 reprend ; S32 (lot 11) reste le contrôle.
3. **Destinations de la caméra vers les ressources** (« Mes ressources », « Ressources partagées », F-275) : le
   navigateur de ressources est au lot 6. Le lot 7 livre page actuelle, nouvelle page, presse-papiers ; les deux
   destinations « ressources » sont déclarées au lot 6 (hypothèse H7-3).
4. **Son de fin du compte à rebours** (F-290) : la lecture de sons est au lot 8 (Qt Multimedia, DA-A3). Le lot 7
   livre « aucun son » et l'action de fin ; le choix d'un son est versé au lot 8 (H7-4).
5. **Ce qui attend le lot 3**, regroupé dans L7-T34 : pivot de rotation sur l'origine XY (F-284), « convertir en
   forme » et « convertir en texte » sur des annotations sélectionnées (F-295, F-296 a posteriori), double-clic sur
   une équation pour la rééditer (F-301), action de fin « rétablir la page » du compte à rebours (F-290), compas
   avec la gomme (F-282). Avant L7-T34, l'équation se réédite par la commande `equation.edit <id>` et les
   conversions par la commande sur les derniers traits reconnus.
6. **Menu d'édition d'objet** (lot 5) : les entrées « Convertir en forme / en texte » du menu d'objet y seront
   ajoutées ; le lot 7 livre les commandes.
7. **Zoom de page et cache.** Le zoom continu ne redessine pas le cache pendant l'appui ; il en demande **un** redessin
   au relâché (texture à la nouvelle échelle, politique de fin de zoom du lot 10) : prouvé par
   `pageZoomRefreshesTheCacheOnceAtRelease`.

## Couverture des scénarios

| Scénario | Tâches | Joué à la clôture |
|---|---|---|
| S10 — présentation | L7-T1, L7-T2 (socle), L7-T3 (rideau), L7-T4, L7-T5, L7-T6, L7-T9 (règle), L7-T10 à L7-T17, L7-T21 à L7-T23 (reconnaissance, 7b), L7-T28 à L7-T32 (7b) | oui : `presentation` (7b, L7-T32) hors écran et à l'écran D3D11 ; captures de l'APK (commandes de bureau et d'écriture absentes) |
| S22 — effets de page | L7-T7, L7-T8 | oui : `effets` à l'écran, banc D3D11 plein écran pendant les transitions |
| S23 — message défilant | L7-T18, L7-T19 | oui : `message` à l'écran, banc D3D11 pendant le défilement |
| S24 — équation | L7-T1 (socle), L7-T24 à L7-T27 (7b) | oui : `equation` (7b) |
| S30 — tâche d'ouverture | L7-T0 | contrôlé à la relecture : rapport, relevé brut, `NOMS OK` |

## Carte des fichiers (7a)

| Fichier | Responsabilité | Tâche |
|---|---|---|
| `docs/verifications/AAAA-MM-JJ-lot7-principes.md` | principes du lot en nos mots ; principe → exigence → tâche → scénario, ou raison | L7-T0 |
| `actions/data/commands.json` | toutes les commandes du lot 7, plateformes déclarées | L7-T1 |
| `actions/…/commandregistry.h`, `actions/src/commandregistry.cpp` | `ArgumentKind::Text` (texte non vide) | L7-T1 |
| `core/…/model.h`, `core/src/model.cpp`, `core/src/edits.cpp` | `core::Equation`, `ObjectKind::Equation`, `core::PagePresentation` dans `Page::presentation` ; empreinte | L7-T1 |
| `core/…/replace.h`, `core/src/replace.cpp` | `core::ReplaceObjectsCommand` : retirer des objets et en ajouter d'autres en une étape | L7-T1 |
| `formats/src/nativejson.cpp` | type `"equation"`, clé de page `"presentation"` | L7-T1 |
| `formats/src/flipchart.cpp`, `formats/src/flipchartids.h` | objet MathML → équation (L7-T1) ; réglages de présentation de la page (L7-T6) | L7-T1, L7-T6 |
| `render/src/pagepainter.cpp`, `render/src/paintedrenderer.cpp` | équation dessinée par un repère neutre (remplacé en 7b, L7-T25) | L7-T1 |
| `actions/data/layout.json`, `actions/…/layout.h`, `actions/src/layout.cpp`, `ui/src/commands.cpp`, `ui/qml/CommandMenu.qml` | sous-menus ; entrées du lot dans les menus et la palette | L7-T2 |
| `ui/…/pageview.h`, `ui/src/pageview.cpp` | `setSampleFilter`, `zoomAround`, `sceneToPage` | L7-T2 |
| `ui/…/documentcontroller.h`, `ui/src/documentcontroller.cpp` | bloc « lot 7 » : `insertObject`, `insertObjectOnNewPage`, `insertText`, `addResource`, `replaceObjects`, `setStrokeObserver`, `shownPage`, `setPagePresentation` | L7-T2 |
| `ui/qml/PageArea.qml`, `ui/qml/Main.qml` | emplacements `pageOverlays`, `presentationLayer` ; instance unique de `Presentation` | L7-T2 |
| `ui/qml/Presentation.qml`, `ui/qml/FloatingPanel.qml`, `ui/presentation.cmake` | racine des fonctions du lot (gestionnaires de commandes) ; panneau flottant commun ; sources QML et C++ du lot | L7-T2 puis chaque tâche |
| `bench/…/uiscenario.h`, `bench/src/uiscenario.cpp` | étapes `pointer` (souris, bouton, maintien) et `doubleClick` | L7-T2 |
| `features/…/<fonction>.h`, `features/src/<fonction>.cpp` | logique pure de chaque fonction | L7-T3 à L7-T19 |
| `ui/src/<fonction>controller.h/.cpp`, `ui/qml/<Fonction>*.qml` | contrôleur QML et surcouche ou panneau | L7-T3 à L7-T19 |
| `tests/presentation.cmake`, `tests/tst_*` du lot | tests du lot 7, déclarés hors des fichiers de tests partagés | L7-T1 puis chaque tâche |
| `bench/scenarios/<nom>.json` | scénarios de captures du lot | L7-T3, T4, T8, T9, T16, T19 |

## Contrats partagés entre tâches (noms figés)

Tous les noms des lots 1, 2 et 10 restent figés ; on ajoute, on ne renomme pas.

- `actions` (L7-T1) : `ArgumentKind::Text` ajouté **en dernier** (`None, Color, Number, Url, Text`) ; dans
  `commands.json`, `"argument": "text"` ; valide = `QVariant` convertible en `QString` non vide après `trimmed()`.
- `core` (L7-T1) :
  ```cpp
  struct Equation { RectF bounds; std::string mathml; double pointSize = 24; Argb color = 0xFF000000u; };
  // ObjectKind : … Connector = 7, Equation = 8, Unknown = 9 (toujours le dernier) ; kObjectKindCount = 10
  // ObjectVariant : <Shape, ImageObject, Stroke, Line, Text, Media, Group, Connector, Equation, UnknownObject>
  enum class PageToolsMode : std::uint8_t { Keep = 0, Curtain = 1, Spotlight = 2, Off = 3 };
  enum class CurtainMode : std::uint8_t { Off = 0, Top = 1, Bottom = 2, Left = 3, Right = 4, Full = 5 };
  enum class SpotlightShape : std::uint8_t { Off = 0, Circle = 1, Square = 2, SolidCircle = 3, SolidSquare = 4 };
  struct PagePresentation {
      PageToolsMode tools = PageToolsMode::Keep;
      CurtainMode curtain = CurtainMode::Off;
      double curtainDistance = 0;          // pixels de page depuis le bord
      SpotlightShape spotlight = SpotlightShape::Off;
      RectF spotlightBounds;               // vide : cadre par défaut
      bool hasOrigin = false; PointF origin; // origine XY mémorisée (F-284)
      std::string transition;              // identifiant d'effet propre à la page ; vide : effet global
  };
  bool isDefault(const PagePresentation&) noexcept;   // vrai : rien à écrire, rien dans l'empreinte
  // Page : membre `PagePresentation presentation;` ajouté après `notes`.
  ```
  `core::ReplaceObjectsCommand(std::size_t page, std::vector<ObjectId> removed, std::vector<PageObject> added)` :
  une étape d'annulation ; `addedIds()` après le premier `redo()`. `core::SetPagePresentationCommand(std::size_t page,
  PagePresentation value)` : une étape.
- `formats` (L7-T1) : objet natif `{"type": "equation", "bounds": [...], "mathml": "<math>…</math>", "pointSize": n,
  "color": "#AARRGGBB"}` ; page : clé `"presentation"` écrite seulement si `!isDefault(...)`, sous-clés `tools`,
  `curtain`, `curtainDistance`, `spotlight`, `spotlightBounds`, `origin`, `transition` (noms anglais à nous).
- `ui` (L7-T2) :
  ```cpp
  // PageView
  using SampleFilter = std::function<input::Sample(const input::Sample&)>;
  void setSampleFilter(SampleFilter filter);        // vide : aucun ; appelé avant le stylo, sur le fil principal
  Q_INVOKABLE void zoomAround(QPointF viewPoint, qreal zoom); // le point de page sous viewPoint ne bouge pas
  Q_INVOKABLE QPointF sceneToPage(QPointF scenePoint) const;
  // DocumentController — bloc « lot 7 »
  core::ObjectId insertObject(core::PageObject object);            // page affichée, une étape ; 0 si refusé
  core::ObjectId insertObjectOnNewPage(core::PageObject object);   // page ajoutée en fin, affichée ; une étape
  Q_INVOKABLE quint64 insertText(const QString& plainText, const QRectF& bounds); // couche intermédiaire
  std::string addResource(QByteArray data, std::string extension);
  std::vector<core::ObjectId> replaceObjects(std::vector<core::ObjectId> removed, std::vector<core::PageObject> added);
  using StrokeObserver = std::function<void(core::ObjectId)>;
  void setStrokeObserver(StrokeObserver observer); // après chaque trait versé par le stylo
  const core::Page* shownPage() const noexcept;
  std::size_t shownIndex() const noexcept;
  void setPagePresentation(core::PagePresentation value);          // page affichée, une étape
  ```
  QML : `PageArea.pageOverlays` (Item, objectName `pageOverlays`, coordonnées de page, au-dessus de l'encre et de la
  couche d'attente) ; `Main.qml` : `presentationLayer` (Item, objectName `presentationLayer`, plein cadre, `z: 50`)
  et l'instance `Presentation { objectName: "presentation" }`.
- `bench` (L7-T2) : `UiStepKind::Pointer` et `UiStepKind::DoubleClick` ajoutés **après** `CheckEdge` ;
  `{"pointer": [[x, y], …], "button": "left"|"right", "holdMs": n, "space": "page"|"window"}` (souris ; maintien au
  dernier point avant le relâché) ; `{"doubleClick": [x, y], "space": "page"|"window"}` ; lignes `POINTEUR <n> OK` et
  `DOUBLECLIC OK`.
- `features` (L7-T9) : `class StrokeGuide { virtual std::optional<input::Sample> capture(const input::Sample&, double
  tolerance) = 0; virtual input::Sample follow(const input::Sample&) = 0; }` ; `GuideSet::filter(const
  input::Sample&)` rend l'échantillon guidé (accroché à l'appui, suivi jusqu'au relâché), branché par
  `MathToolsController` sur `PageView::setSampleFilter`.
- Identifiants de commandes (L7-T1, liste complète dans la section de L7-T1) : préfixes `present.`, `view.`,
  `page.`, `math.`, `tool.`, `insert.`, `desktop.`, `recognition.`, `equation.`.

## Règles communes à toutes les tâches (rappelées dans chaque section)

Celles des plans 2a, 2b et 10 s'appliquent toutes, mot pour mot (`docs/plans/2026-10-15-livraison-complete-lot2a-PLAN.md`,
`…-lot2b-PLAN.md`, `docs/plans/2026-10-18-livraison-complete-lot10-PLAN.md`, « Règles communes ») : processus enfant
pour toute commande PowerShell, `git -C`, jamais de `cd`, mutation remise par `git checkout --`, `format.ps1` avant
la preuve, pièges de Clang (`-Wshadow`, conversions explicites, pas d'agrégat partiel par position), pas de
`std::visit` direct sur `PageObject` (passer par `body()`), **aucune API privée de Qt** dans le code livré,
propriété intellectuelle, `samples/` en lecture seule, commits explicites en français terminés par
`Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`, arrêt par PID, jeton `tests` pour toute construction,
suite, banc ou capture (une commande par prise, aucun démon laissé vivant, attendre la notification sans sonder en
boucle). S'y ajoutent, pour le lot 7 :

- `<wt>` = `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b--L7-T<n>` (branche
  `feat/livraison-complete-b--L7-T<n>`, partie de `feat/livraison-complete-b`) ; `<id>` = `L7-T<n>`.
- **Session B, une place de tests** : une seule construction à la fois dans toute la session ; la commande du jeton
  est la même (`-Nom tests -Proprietaire <id>`).
- **Ne rien toucher du lot 3** : aucun fichier de `tools/`, `input/src/`, ni des tests de ces modules ; aucune
  modification du comportement du stylo, de la vue ou du cache hors des points réservés par L7-T1 et L7-T2 (une tâche
  qui croit en avoir besoin s'arrête et le rapporte à l'orchestrateur).
- **Fichiers de tests du lot** : chaque test nouveau est déclaré dans `tests/presentation.cmake` (créé par L7-T1),
  jamais dans `tests/core.cmake`, `ui.cmake`, `bench.cmake` (partagés avec le lot 3). Chaque source QML ou C++ du lot
  dans `ui/` est déclarée dans `ui/presentation.cmake` (créé par L7-T2), jamais dans `ui/CMakeLists.txt`.
- **Textes** : libellés en français, écrits par nous (`qsTr`), sans terme propre à Promethean (N1-12) : ni nom de
  produit, de service, de télécommande ni libellé copié de l'original ; icônes SVG neuves, grille de 24 px,
  jetons de `Theme` (décision 23).
- **Plateformes (DA10)** : bureau seulement (`"platforms": ["windows", "macos", "linux"]`) pour caméra, clavier à
  l'écran, annotation et outils de bureau, calculatrice du système ; Windows seulement pour la reconnaissance
  d'écriture ; toutes les autres pour le reste. Code propre à Windows dans des fichiers `*_win.cpp` seulement.
- **Surcouche = aucun redessin du cache** : toute tâche de surcouche a un test `QSignalSpy` sur
  `PageView::cacheRefreshRequested` (0 émission pendant l'usage, sauf mention contraire écrite dans sa section).
- **Commande ciblée** (rouge, vert d'un seul test) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande '. ./scripts/dev-env.ps1; cmake --build --preset debug *> .tmp/build.txt; ctest --preset debug -R ''<motif>'' --output-on-failure *> .tmp/ctest.txt'; Select-String -Path .tmp/build.txt,.tmp/ctest.txt -Pattern 'error|tests passed|tests failed|No tests were found' | Select-Object -Last 4"
  ```
  Premier passage dans un worktree neuf : la même commande avec `-Commande 'pwsh -NoProfile -File ./scripts/build.ps1 *> .tmp/build.txt'`.
  Attendu au rouge : `tests failed` ou une erreur de compilation sur le nom testé ; au vert : `100% tests passed`.
- **Preuve de tâche** (forme bornée du profil, sous le jeton), puis le nom du test :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1'; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2; Select-String -Path .tmp/test.txt -Pattern '<nom_du_test> .*Passed' | Select-Object -Last 1"
  ```
  Attendu : `100% tests passed, 0 tests failed out of N` (N ≥ la valeur de la table), `TESTS OK`, puis la ligne
  `Passed` du test nommé. Un test a tourné : N ≥ 1 et le nom attendu est présent. Les fonctions de test citées par
  la table se lisent dans `ctest -R <test> --output-on-failure -V` (ligne `PASS   : <Classe>::<fonction>()`).
- **Banc sans écran** (toute tâche qui touche `render/`, `input/`, `tools/` ou `ui/` ; résultat, pas une porte) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu logiciel -Scenario court -Taille 1280x800 -HorsEcran > .tmp/bench.txt 2>&1'; Select-String -Path .tmp/bench.txt -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE' | Select-Object -Last 1"
  ```
  Attendu : `BUDGET TENU` ou `BUDGET DEPASSE` (jamais `MESURE INVALIDE`) ; nom du JSON de `bench/results/` joint.
- **Preuve visuelle hors écran** (forme bornée du profil, sous le jeton `tests`), lectures `-Lire` d'abord :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario <nom> -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire ''<objet>.<propriété>=<attendu>'' > .tmp/captures.txt 2>&1'; Select-String -Path .tmp/captures.txt -Pattern '^(COMMANDE|TRAIT|TOUCHE|ATTENTE|PRET|FENETRE|LU|PIXEL|BORD|POINTEUR|DOUBLECLIC|CAPTURE) |^SCENARIO |CAPTURES (OK|EN ECHEC)' | Select-Object -Last 16"
  ```
  Attendu : `LU … OK`, aucune ligne `ECART` ni `ECHEC`, `CAPTURES OK` ; décrire ensuite en une phrase chaque capture
  nommée par la section.
- **Contrôle des noms** (toute tâche) : `python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" <wt>` → `NOMS OK`.

---

## Tâches

<a id="tache-l7-t0"></a>
### Tâche L7-T0 — Ouverture : vidéos, REA, spécification et ancien dépôt confrontés au lot 7

**Objectif.** Décision 0100 et règle `Lx-T0` de l'amendement 1 (§ 4) : avant d'écrire une ligne du lot, relever dans
le corpus vidéo tous les principes des fonctions du lot 7 (thèmes « Outils » : 11 constats, « Interface et profils » :
4, « Texte et reconnaissance » : 4, « Sélection, verrouillage, masquage » : 2, « Pages, fond, grille » : 2 — soit
23 constats, amendement 1, § 4) ; trancher sur la 3.5.18, par REA en **statique**, les écarts **E5** (nombre de dés à
l'ouverture), **E9** (actions de l'horloge en fin de délai), **E11** (validation du message défilant) et les points
« à élucider » du lot (amendement 1, § 2 : liste et comportement des effets de changement de page ; jeu de modèles
de l'éditeur d'équations et représentation des équations des `.flipchart`) ; écrire le résultat en nos mots ; relever
**N0** (nombre de tests de la base).

**Sources.**
- Corpus : `C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\README.md` (« Principes absents », « Écarts
  avec la spec ») et les fiches `*.md` du même dossier qui parlent de rideau, spot, zoom, outils de maths, dés,
  horloge, calculatrice, message, reconnaissance, caméra, clavier, bureau, effets de page, équation.
- Spécification : `C:\Users\JeromeDenis\REA - Activinspire\docs\spec\fonctionnalites.md` (F-070, F-071, F-080, F-124,
  F-260, F-265, F-270, F-275, F-280 à F-286, F-290, F-291, F-294 à F-296, F-299 à F-301, F-591 à F-593, F-640, F-750).
- Ancien dépôt : `app/actcraft/extras/` (rideau, spot, outils de maths, dés, horloge, minuterie, calculatrice,
  caméra, bureau, clavier), `app/actcraft/canvas/recognition.py`, `app/actcraft/canvas/transitions.py`,
  `docs/re/format-flipchart.md` (réglages de la page, objet MathML), `docs/tests/plan-comparatif.md` ; son graphe :
  `graphify query "…"` lancé **dans** `C:\Users\JeromeDenis\REA - Activinspire`.
- Binaire : skill `reverse-engineer-anything` sur `C:\Program Files\Activ Software\Inspire\`, **en statique
  seulement** (ressources des boîtes, valeurs par défaut des réglages et des profils, chaînes). Aucune observation à
  l'écran : un point qui l'exigerait est noté « non tranché » et devient une hypothèse (la spécification s'applique).

**Fichiers.**
- Créer : `docs/verifications/AAAA-MM-JJ-lot7-principes.md` (date du jour d'exécution ; dans le dépôt, nos mots).
- Créer hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot7.md` (relevé brut : constats REA,
  noms de l'original, renvois aux fiches). Le dossier `docs\re\lots` existe (lot 10).

**Test et commande bornée.** Pas de code. Preuves :
```powershell
Test-Path 'C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot7.md'   # True
python -I "C:/Users/JeromeDenis/REA - Activinspire/tools/controle_noms_actcraft.py" "C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b--L7-T0" | Select-Object -Last 1   # NOMS OK : <n> fichiers
pwsh -NoProfile -Command "Get-ChildItem 'C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\*.md' -Exclude README.md | ForEach-Object { `$t = (Get-Content -LiteralPath `$_.FullName -TotalCount 5 | Where-Object { `$_ -like '# *' } | Select-Object -First 1); if (`$t) { `$t = `$t.Substring(2).Trim(); git -C 'C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b--L7-T0' grep -l -i -F -- `$t } }"   # aucune sortie
pwsh -NoProfile -Command "Set-Location 'C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b--L7-T0'; . ./scripts/dev-env.ps1; cmake --preset debug *> `$null; ctest --preset debug -N | Select-String 'Total Tests'"   # N0, sous le jeton tests
```

**Interdits.** Aucun titre, aucune transcription, aucune citation de vidéo dans le dépôt (identifiants `Vnn`
seulement). Aucun nom composé de l'original (classes, propriétés, libellés), aucun texte de l'aide, aucune capture.
Aucune ligne de code. Ne pas modifier `samples/` ni le corpus. Pas d'observation de l'original à l'écran.

**Terminé quand.** Le rapport a une ligne par constat (principe → exigence en nos mots → tâche du lot 7 →
scénario, ou raison et lot destinataire) ; E5, E9, E11 tranchés (valeur retenue, ou « non tranché : spécification
appliquée ») ; liste des effets de page retenue (identifiants anglais à nous, noms français à nous, N2-A6) ; jeu de
modèles d'équation retenu (au moins : fraction, puissance, indice, racine carrée, racine n-ième, parenthèses,
somme, intégrale, matrice 2 × 2, lettres grecques) et représentation des équations des `.flipchart` (MathML ou
autre) ; N0 écrit ; le relevé brut existe hors dépôt ; `NOMS OK` ; la commande des titres ne sort rien ; le rapport
de tâche liste les écarts à verser au plan (tâches à ajouter ou sections à corriger) et ceux qui vont à d'autres
lots.

**Étapes.**

- [ ] **Étape 1 : relever le corpus.**
  ```powershell
  Select-String -Path 'C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\*.md' -Pattern 'rideau|spot|projecteur|zoom|règle|regle|rapporteur|compas|équerre|equerre|origine|dés\b|dé\b|horloge|minuteur|compte à rebours|calculatrice|message|défil|reconnaiss|écriture|forme|caméra|camera|capture|clavier|bureau|tourne-page|transition|équation|date' -List | Select-Object -ExpandProperty Path
  ```
  Pour chaque fiche retenue : son numéro `Vnn` (colonne « # » du `README.md`) et le principe, reformulé en une phrase
  à nous.
- [ ] **Étape 2 : relever l'ancien dépôt.** Dans l'ancien dépôt : `graphify query "rideau spot surcouche"`,
  `graphify query "outils mathématiques accrochage du stylo"`, `graphify query "effets tourne-page"`,
  `graphify query "reconnaissance de forme et d'écriture"`, `graphify query "horloge compte à rebours action de fin"` ;
  lire les fichiers d'`app/actcraft/extras/` cités par le graphe. Confirmer ou corriger les six constats du § « Ce que
  l'ancien dépôt dit déjà ».
- [ ] **Étape 3 : REA statique.** Avec le skill `reverse-engineer-anything` : valeurs par défaut (nombre de dés,
  vitesse, durée du tourne-page, délai de reconnaissance, couleur du rideau, vitesse du message), liste des effets de
  page et leur comportement décrit par les ressources, boutons des boîtes du message défilant et du compteur
  d'horloge, palettes de l'éditeur d'équations, codage des réglages de page des `.flipchart` (rideau, spot, origine,
  effet) et de l'objet MathML. Constats bruts, avec les noms de l'original, dans le relevé hors dépôt seulement.
- [ ] **Étape 4 : écrire le rapport.**
  ```markdown
  # Lot 7 — principes confrontés (vidéos, spécification, ancien dépôt, binaire)

  Relevé brut (hors dépôt) : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot7.md`. Références `Vnn` :
  numéros des fiches du corpus (table hors dépôt). Constats écrits par nous. Base : N0 = <n> tests ctest.

  ## Écarts tranchés
  | # | Sujet | Valeur retenue pour ActCraft | Source (statique) | Tâche |
  |---|---|---|---|---|
  | E5 | Nombre de dés à l'ouverture | … | … | L7-T14 |
  | E9 | Actions de l'horloge en fin de délai | … | … | L7-T16 |
  | E11 | Validation du message défilant | … | … | L7-T19 |

  ## Effets de page retenus (N2-A6)
  | Identifiant | Nom (à nous) | Mouvement | Tâche |
  |---|---|---|---|

  ## Modèles d'équation retenus
  | Identifiant | Nom (à nous) | Forme MathML écrite | Tâche |
  |---|---|---|---|

  ## Principes
  | # | Principe (nos mots) | Source | Exigence pour ActCraft | Tâche | Scénario | Écart, raison ou lot destinataire |
  |---|---|---|---|---|---|---|
  ```
  Une ligne par constat des cinq thèmes ; ceux qui demandent une tâche à eux sont signalés « tâche à ajouter ».
- [ ] **Étape 5 : contrôles.** Lancer les quatre commandes de « Test et commande bornée » ; corriger jusqu'à
  `NOMS OK` et sortie vide.
- [ ] **Étape 6 : commit.**
  ```powershell
  git -C <wt> add docs/verifications/AAAA-MM-JJ-lot7-principes.md
  git -C <wt> commit -m "Lot 7 : principes de la présentation et des outils confrontés aux vidéos, à la spécification et au binaire (L7-T0)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

<a id="tache-l7-t1"></a>
### Tâche L7-T1 — Réservation I (données et modèle)

**Objectif.** Poser d'un coup, dans les fichiers partagés avec le lot 3, tout ce que le lot 7 y ajoute côté données :
les déclarations de toutes les commandes du lot, l'argument texte, le type d'objet « équation » (gardé et réécrit
sans perte, L2-C8), les réglages de présentation de la page, le remplacement annulable et la commande de réglage de
page, leur lecture et écriture au format natif, l'objet MathML des `.flipchart`. Après elle, la fusion avec le lot 3
ne porte, côté données, que sur ces points.

**Fichiers.** Voir la table (ligne L7-T1). Créer : `core/include/actcraft/core/replace.h`, `core/src/replace.cpp`,
`tests/presentation.cmake`, `tests/tst_core_presentation.cpp`, et les **fichiers réservés** de 7b, déclarés ici pour
que `core/CMakeLists.txt` et `render/CMakeLists.txt` (partagés avec le lot 3) ne soient plus touchés :
`core/include/actcraft/core/equation.h` et `core/src/equation.cpp` (espace de noms et commentaire seulement, remplis
par L7-T24), `render/src/equationlayout.h` et `render/src/equationlayout.cpp` (`void paintEquation(QPainter&, const
core::Equation&)` qui dessine le repère neutre, remplie par L7-T25).

**Test et commande bornée.** Commande ciblée avec `-R 'tst_core_presentation|tst_core_model|tst_formats_nativejson|tst_formats_flipchart|tst_actions_registry'`,
puis preuve de tâche, N ≥ N0 + 1, noms `tst_core_presentation`, `tst_formats_nativejson`, `tst_formats_flipchart`.

**Interdits.** Ne changer le rang d'aucun type existant (`Shape` … `Connector` gardent 0 à 7). Ne pas modifier
l'empreinte d'une page sans réglages de présentation (`fingerprintUnchangedWithoutPresentation` le prouve sur la
page du banc, empreinte relevée avant la tâche). Aucun nom de l'original dans les clés JSON ni dans le code (les
identifiants de propriétés des `.flipchart` sont des numéros, écrits à la main d'après notre documentation). Aucune
interprétation du MathML ici (texte gardé tel quel).

**Terminé quand.** Les cinq tests nommés passent ; un `.actcraft` avec une équation et des réglages de présentation
fait l'aller-retour à l'identique ; un `.flipchart` synthétique avec un objet MathML s'ouvre avec une équation ;
`TESTS OK` ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : relever l'empreinte de référence.** Avant tout changement, dans `<wt>` (commande ciblée sur
  `tst_core_model`), noter la valeur de `fingerprint(page générée du banc)` affichée par `tst_core_model` (ou, à
  défaut, ajouter temporairement un `qInfo()` et la relever, puis retirer la ligne). Elle devient la constante
  `kBenchPageFingerprint` du test de l'étape 2.
- [ ] **Étape 2 : test rouge du modèle.** Créer `tests/tst_core_presentation.cpp` :
  ```cpp
  #include <actcraft/core/document.h>
  #include <actcraft/core/edits.h>
  #include <actcraft/core/generator.h>
  #include <actcraft/core/model.h>
  #include <actcraft/core/replace.h>
  #include <actcraft/core/undo.h>

  #include <QtTest>

  using namespace actcraft::core;

  class TstCorePresentation : public QObject
  {
      Q_OBJECT
  private slots:
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
          Page page; // page vierge : l'empreinte d'avant L7-T1 est celle relevée à l'étape 1 pour le banc
          const auto before = fingerprint(page);
          page.presentation = PagePresentation{};
          QCOMPARE(fingerprint(page), before);
          page.presentation.transition = "fade";
          QVERIFY(fingerprint(page) != before);
      }
      void replaceIsOneUndoStep()
      {
          Document doc;
          UndoStack stack;
          Stroke s;
          s.points = {{0, 0, 0.5f}, {10, 10, 0.5f}};
          auto add = std::make_unique<AddObjectCommand>(0, PageObject{s});
          auto* addPtr = add.get();
          Changes ch;
          stack.push(doc, std::move(add), ch);
          const ObjectId strokeId = addPtr->id();
          Shape r;
          r.bounds = RectF{0, 0, 10, 10};
          auto replace = std::make_unique<ReplaceObjectsCommand>(0, std::vector<ObjectId>{strokeId},
                                                                 std::vector<PageObject>{PageObject{r}});
          auto* replacePtr = replace.get();
          stack.push(doc, std::move(replace), ch);
          QCOMPARE(doc.page(0).objects.size(), std::size_t{1});
          QCOMPARE(kindOf(doc.page(0).objects[0]), ObjectKind::Shape);
          QCOMPARE(replacePtr->addedIds().size(), std::size_t{1});
          stack.undo(doc, ch);
          QCOMPARE(doc.page(0).objects.size(), std::size_t{1});
          QCOMPARE(kindOf(doc.page(0).objects[0]), ObjectKind::Stroke);
          QCOMPARE(doc.page(0).objects[0].common.id, strokeId);
      }
      void setPagePresentationIsOneUndoStep()
      {
          Document doc;
          UndoStack stack;
          Changes ch;
          PagePresentation p;
          p.tools = PageToolsMode::Curtain;
          stack.push(doc, std::make_unique<SetPagePresentationCommand>(0, p), ch);
          QCOMPARE(doc.page(0).presentation.tools, PageToolsMode::Curtain);
          stack.undo(doc, ch);
          QVERIFY(isDefault(doc.page(0).presentation));
      }
  };

  QTEST_GUILESS_MAIN(TstCorePresentation)
  #include "tst_core_presentation.moc"
  ```
  Les noms `UndoStack::push(doc, cmd, changes)`, `undo(doc, changes)`, `Document::page(i)` sont ceux de
  `core/include/actcraft/core/undo.h` et `document.h` : les lire d'abord et aligner les appels (sans changer ces
  en-têtes). Créer `tests/presentation.cmake` :
  ```cmake
  # Tests du lot 7 (présentation et outils) : déclarés ici seulement (fichiers de tests partagés laissés au lot 3).
  actcraft_add_test(tst_core_presentation SOURCES tst_core_presentation.cpp LIBS ActCraft::core)
  ```
  et ajouter à `tests/CMakeLists.txt`, après la ligne `include(${CMAKE_CURRENT_SOURCE_DIR}/app.cmake)` :
  ```cmake
  include(${CMAKE_CURRENT_SOURCE_DIR}/presentation.cmake)
  ```
  Commande ciblée `-R tst_core_presentation` : rouge (compilation : `Equation`, `PagePresentation`, `replace.h`
  inconnus).
- [ ] **Étape 3 : modèle.** Dans `core/include/actcraft/core/model.h` : ajouter `struct Equation` (contrat) avant
  `UnknownObject` ; `ObjectKind::Equation = 8`, `Unknown = 9`, `kObjectKindCount = 10` ; `Equation` dans
  `ObjectVariant` entre `Connector` et `UnknownObject` ; les trois énumérations et `PagePresentation` avant `struct
  Page` ; `PagePresentation presentation;` dans `Page` après `notes` ; déclarer `bool isDefault(const
  PagePresentation&) noexcept;`. Dans `core/src/model.cpp` : `hashType(Fnv1a&, const Equation&)` (bounds, mathml,
  pointSize, color), `isDefault` (tous les champs à leur valeur par défaut), et dans `fingerprint(const Page&)`, **en
  fin de flux, avant les clés inconnues**, `if (!isDefault(page.presentation)) { h.byte(0xA7); … champs … }`. Dans
  `core/src/edits.cpp`, `boundsOnPage` : `Equation` → `bounds` transformé, comme `Text`.
- [ ] **Étape 4 : remplacement et réglage de page.** Créer `core/include/actcraft/core/replace.h` :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>
  #include <actcraft/core/undo.h>

  #include <cstddef>
  #include <vector>

  namespace actcraft::core {

  // Retire `removed` de la page `page` et ajoute `added` au sommet de leur couche, en une étape (reconnaissance de
  // forme et d'écriture, lot 7). Les objets retirés retrouvent leur indice exact à l'annulation. Un identifiant
  // absent de la page est ignoré ; les identifiants des ajouts sont alloués au premier redo() puis gardés.
  class ReplaceObjectsCommand final : public UndoCommand
  {
  public:
      ReplaceObjectsCommand(std::size_t page, std::vector<ObjectId> removed, std::vector<PageObject> added);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;
      const std::vector<ObjectId>& addedIds() const noexcept;

  private:
      std::size_t m_page;
      std::vector<ObjectId> m_removedIds;
      std::vector<std::pair<std::size_t, PageObject>> m_removed; // indice d'origine, objet (rempli au redo)
      std::vector<PageObject> m_added;
      std::vector<ObjectId> m_addedIds;
  };

  // Remplace les réglages de présentation de la page `page` (F-591 à F-593, F-640, F-284), en une étape.
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
  `core/src/replace.cpp` : `redo` relève chaque objet retiré avec son indice (`indexOf`), les retire du plus grand
  indice au plus petit, puis insère chaque ajout par `insertAtTopOfLayer` (identifiant par `document.allocateId()`
  au premier passage, comme `AddObjectCommand`) ; `undo` retire les ajouts par identifiant puis réinsère les retirés
  du plus petit indice au plus grand ; les deux signalent la page dans `changes` comme `AddObjectCommand` (lire
  `core/src/edits.cpp` et reprendre exactement sa façon de remplir `Changes`, y compris l'information destinée au
  cache). Ajouter `include/actcraft/core/replace.h`, `src/replace.cpp`, `include/actcraft/core/equation.h` et
  `src/equation.cpp` aux `SOURCES` de `core/CMakeLists.txt` ; les deux fichiers d'équation ne contiennent que :
  ```cpp
  #pragma once
  // Équation (F-301) : arbre et MathML — fichier réservé par L7-T1, rempli par L7-T24.
  namespace actcraft::core {
  } // namespace actcraft::core
  ```
  (et, pour `equation.cpp`, `#include <actcraft/core/equation.h>` suivi du même espace de noms vide).
  Commande ciblée : `tst_core_presentation` et `tst_core_model` verts (dans `tst_core_model.cpp`, les deux
  `QCOMPARE` sur `kObjectKindCount` et `ObjectKind::Unknown` suivent d'eux-mêmes ; ajouter un objet `Equation` au
  document témoin s'il en énumère un par type).
- [ ] **Étape 5 : argument texte.** Test rouge dans `tests/tst_actions_registry.cpp` :
  ```cpp
  void textArgument()
  {
      actions::CommandRegistry r;
      QString error;
      QVERIFY(r.loadDeclarations(R"({"format":"actcraft-commands/1","commands":[{"id":"a.b","label":"L","argument":"text"}]})", error));
      QCOMPARE(r.find(u"a.b"_s)->argument, actions::ArgumentKind::Text);
      QVERIFY(actions::argumentIsValid(actions::ArgumentKind::Text, QVariant(u"fade"_s)));
      QVERIFY(!actions::argumentIsValid(actions::ArgumentKind::Text, QVariant(u"  "_s)));
      QVERIFY(!actions::argumentIsValid(actions::ArgumentKind::Text, QVariant()));
  }
  ```
  (Pas d'apostrophe dans la chaîne brute : piège de moc, fichier d'état.) Puis `Text` en fin d'`ArgumentKind`,
  `"text"` dans le lecteur de `commandregistry.cpp` (ligne 29 et suivantes), `argumentIsValid` : `argument.canConvert<QString>()
  && !argument.toString().trimmed().isEmpty()`.
- [ ] **Étape 6 : commandes du lot.** Ajouter à `actions/data/commands.json`, après `help.about`, les entrées
  suivantes (libellés à nous ; aucune n'a de gestionnaire avant sa tâche) :
  ```json
  { "id": "view.page-zoom", "label": "Zoom de page", "icon": "zoom-page", "shortcut": "Ctrl+Shift+Z", "checkable": true },
  { "id": "view.page-effects", "label": "Effets de changement de page…", "icon": "effets" },
  { "id": "view.page-effect", "label": "Effet de changement de page pour tout le paperboard", "argument": "text" },
  { "id": "view.page-effect-duration", "label": "Durée de l'effet (dixièmes de seconde)", "argument": "number" },
  { "id": "page.effect", "label": "Effet propre à cette page", "argument": "text" },
  { "id": "page.tools-mode", "label": "Outils de présentation de la page", "argument": "text" },
  { "id": "page.curtain-mode", "label": "Rideau de la page", "argument": "text" },
  { "id": "page.curtain-distance", "label": "Distance du rideau", "argument": "number" },
  { "id": "page.spotlight-mode", "label": "Spot de la page", "argument": "text" },
  { "id": "present.curtain", "label": "Rideau", "icon": "rideau", "shortcut": "Ctrl+R", "checkable": true },
  { "id": "present.curtain-color", "label": "Couleur du rideau", "argument": "color" },
  { "id": "present.spotlight-circle", "label": "Spot rond", "icon": "spot", "shortcut": "Ctrl+Shift+O", "checkable": true },
  { "id": "present.spotlight-square", "label": "Spot carré", "checkable": true },
  { "id": "present.spotlight-solid-circle", "label": "Cache rond", "checkable": true },
  { "id": "present.spotlight-solid-square", "label": "Cache carré", "checkable": true },
  { "id": "math.ruler", "label": "Règle", "icon": "regle" },
  { "id": "math.set-square", "label": "Équerre", "icon": "equerre" },
  { "id": "math.protractor", "label": "Rapporteur", "icon": "rapporteur" },
  { "id": "math.compass", "label": "Compas", "icon": "compas" },
  { "id": "math.xy-origin", "label": "Origine XY", "icon": "origine", "checkable": true },
  { "id": "tool.dice", "label": "Dés", "icon": "des" },
  { "id": "tool.calculator", "label": "Calculatrice", "icon": "calculatrice" },
  { "id": "tool.clock", "label": "Horloge", "icon": "horloge", "shortcut": "Ctrl+Shift+U" },
  { "id": "tool.ticker", "label": "Message défilant…", "icon": "message", "shortcut": "Ctrl+Shift+T" },
  { "id": "tool.web-browser", "label": "Navigateur web" },
  { "id": "insert.date-time", "label": "Date et heure" },
  { "id": "insert.equation", "label": "Équation…" },
  { "id": "equation.edit", "label": "Modifier l'équation", "argument": "number" },
  { "id": "tool.shape-recognition", "label": "Reconnaissance de forme", "icon": "forme-reconnue", "checkable": true },
  { "id": "tool.handwriting", "label": "Reconnaissance d'écriture", "icon": "ecriture", "shortcut": "Ctrl+Shift+H", "checkable": true, "platforms": ["windows"] },
  { "id": "recognition.convert-to-shape", "label": "Convertir en forme" },
  { "id": "recognition.convert-to-text", "label": "Convertir en texte", "platforms": ["windows"] },
  { "id": "tool.camera-area", "label": "Capture d'une zone", "icon": "camera", "shortcut": "Ctrl+Shift+K", "platforms": ["windows", "macos", "linux"] },
  { "id": "tool.camera-polygon", "label": "Capture point par point", "platforms": ["windows", "macos", "linux"] },
  { "id": "tool.camera-freehand", "label": "Capture à main levée", "platforms": ["windows", "macos", "linux"] },
  { "id": "tool.camera-window", "label": "Capture d'une fenêtre", "platforms": ["windows", "macos", "linux"] },
  { "id": "tool.camera-screen", "label": "Capture de l'écran entier", "platforms": ["windows", "macos", "linux"] },
  { "id": "tool.keyboard", "label": "Clavier à l'écran", "icon": "clavier", "shortcut": "Ctrl+K", "platforms": ["windows", "macos", "linux"] },
  { "id": "desktop.annotate", "label": "Annoter le bureau", "icon": "bureau", "shortcut": "Ctrl+Shift+A", "checkable": true, "platforms": ["windows", "macos", "linux"] },
  { "id": "desktop.tools", "label": "Outils de bureau", "shortcut": "F6", "platforms": ["windows", "macos", "linux"] },
  { "id": "desktop.restore", "label": "Revenir à ActCraft", "platforms": ["windows", "macos", "linux"] },
  { "id": "page.background-desktop-capture", "label": "Fond : capture du bureau", "platforms": ["windows", "macos", "linux"] },
  { "id": "page.background-desktop-overlay", "label": "Fond : bureau en transparence", "checkable": true, "platforms": ["windows", "macos", "linux"] }
  ```
  Ajouter à `tests/tst_actions_registry.cpp` un test `lot7CommandsDeclared` : `defaultCommandsJson()` se charge sans
  erreur, les 43 identifiants ci-dessus sont déclarés, `tool.handwriting` n'est pas disponible sous
  `Platform::Android` ni `Platform::MacOS`, `tool.camera-area` ne l'est pas sous Android, `present.curtain` l'est
  partout ; aucun raccourci n'est en double dans tout le fichier.
- [ ] **Étape 7 : format natif.** Tests rouges dans `tests/tst_formats_nativejson.cpp` : `equationRoundTripKeepsMathml`
  (page avec une équation dont le MathML contient `<mfrac>` et une entité `&#x3B1;` ; écrire, relire : même empreinte,
  `mathml` identique octet pour octet, type `"equation"` dans le JSON écrit) ; `presentationRoundTrip` (tous les champs
  non par défaut ; relire : égaux ; une page par défaut n'écrit pas la clé `"presentation"`) ;
  `unknownPresentationKeyKept` (une sous-clé inconnue dans `"presentation"` est réécrite telle quelle, règle L2-C8).
  Puis, dans `formats/src/nativejson.cpp` : nom `"equation"` pour `ObjectKind::Equation` (table `kObjectKinds`,
  ligne 162 et suivantes, et l'assertion de la ligne 329 qui suit d'elle-même), `writeBody(QJsonObject&, const
  Equation&)` et `readBody(Reader&, const QJsonObject&, Equation&)` sur le modèle de `Text` (bounds, mathml,
  pointSize, color), lecture et écriture de `"presentation"` dans la page (valeurs d'énumération écrites en texte :
  `"keep"`, `"curtain"`, `"spotlight"`, `"off"` ; `"off"`, `"top"`, `"bottom"`, `"left"`, `"right"`, `"full"` ;
  `"off"`, `"circle"`, `"square"`, `"solid-circle"`, `"solid-square"`), sous-clés inconnues gardées par le mécanisme
  `unknownKeys` déjà en place pour la page.
- [ ] **Étape 8 : objet MathML des `.flipchart`.** Test rouge `mathmlObjectBecomesEquation` dans
  `tests/tst_formats_flipchart.cpp` : `tests/support/flipchartwriter.h` gagne l'écriture d'un objet de type 460
  (numéro de notre documentation `docs/re/format-flipchart.md`, liste des types) portant un texte MathML et un
  rectangle ; le lecteur rend un `Equation` dont `mathml` est ce texte et `bounds` ce rectangle. Ajouter la
  constante `kTypeMathMl = 460` à `formats/src/flipchartids.h` et sa lecture dans `formats/src/flipchart.cpp` (même
  façon que les autres types lus ; le numéro de la propriété qui porte le texte est relevé par L7-T0 dans notre
  documentation ou le relevé brut ; s'il reste inconnu, prendre la première chaîne de l'objet et le consigner en
  hypothèse H7-1).
- [ ] **Étape 9 : repère neutre au rendu.** Créer `render/src/equationlayout.h` (`void paintEquation(QPainter&
  painter, const core::Equation& equation);`) et `render/src/equationlayout.cpp` (corps : le même repère neutre que
  `paintUnknown` de `render/src/pagepainter.cpp:563`, sur `equation.bounds`) ; les ajouter aux sources de
  `render/CMakeLists.txt`. Dans `render/src/pagepainter.cpp` (près de la ligne 646) et `render/src/paintedrenderer.cpp`
  (près de la ligne 233) : une `Equation` est dessinée par `paintEquation`. L7-T25 ne changera que
  `equationlayout.cpp`.
- [ ] **Étape 10 : preuve et commit.** `format.ps1`, preuve de tâche (N ≥ N0 + 1), `NOMS OK`, puis :
  ```powershell
  git -C <wt> add actions/data/commands.json actions/include/actcraft/actions/commandregistry.h actions/src/commandregistry.cpp tests/tst_actions_registry.cpp core/include/actcraft/core/model.h core/src/model.cpp core/src/edits.cpp core/include/actcraft/core/replace.h core/src/replace.cpp core/include/actcraft/core/equation.h core/src/equation.cpp core/CMakeLists.txt tests/tst_core_model.cpp formats/src/nativejson.cpp tests/tst_formats_nativejson.cpp formats/src/flipchart.cpp formats/src/flipchartids.h tests/support/flipchartwriter.h tests/tst_formats_flipchart.cpp render/src/pagepainter.cpp render/src/paintedrenderer.cpp render/src/equationlayout.h render/src/equationlayout.cpp render/CMakeLists.txt tests/CMakeLists.txt tests/presentation.cmake tests/tst_core_presentation.cpp
  git -C <wt> commit -m "Lot 7 : réservation des données (commandes, équation, réglages de présentation de la page, remplacement annulable) (L7-T1)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

<a id="tache-l7-t2"></a>
### Tâche L7-T2 — Réservation II (interface)

**Objectif.** Poser d'un coup, dans les fichiers d'interface partagés avec le lot 3, les points d'accroche du lot 7 :
sous-menus dans la disposition (menus « Outils ▸ Spot », « Caméra », « Outils de maths », « Autres outils »),
entrées du lot dans les menus et la palette, emplacements de surcouches, filtre d'échantillons de la vue, zoom autour
d'un point, API d'insertion du contrôleur de document, racine QML `Presentation`, panneau flottant commun, fichier
CMake propre au lot, étapes de scénario `pointer` et `doubleClick`. Après elle, aucune tâche du lot ne touche plus
`PageArea.qml`, `Main.qml`, `pageview.*`, `documentcontroller.*`, `commands.cpp`, `CommandMenu.qml`, `layout.*`,
`ui/CMakeLists.txt`, `uiscenario.*`.

**Fichiers.** Voir la table (ligne L7-T2). Créer : `ui/qml/Presentation.qml`, `ui/qml/FloatingPanel.qml`,
`ui/presentation.cmake`, `tests/tst_ui_presentation.cpp`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_presentation|tst_actions_layout|tst_ui_commands|tst_bench_uiscenario|tst_pageview|tst_ui_drawing'`,
puis preuve de tâche (N ≥ N0 + 2), banc sans écran, preuve visuelle `premiers-pas` (la fenêtre n'a pas changé :
`-Lire "pageIndicator.text=1 / 2"`, comme le profil).

**Interdits.** Ne changer aucun comportement existant : le stylo, le défilement, le zoom à la molette, le cache et
la palette restent identiques (les suites `tst_pageview`, `tst_ui_drawing`, `tst_ui_shell` passent sans
modification). Ne renommer aucun `objectName` lu par le banc. Le format `actcraft-layout/1` reste lisible tel quel :
un sous-menu est un **ajout** (un élément objet dans `items`), un fichier sans sous-menu se lit comme avant.

**Terminé quand.** Les tests nommés passent ; `premiers-pas` `CAPTURES OK` ; banc hors écran sans régression de
`content_check.ok` ; les menus montrent les entrées du lot (inertes jusqu'à leur tâche) ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : sous-menus, test rouge.** Dans `tests/tst_actions_layout.cpp`, `submenuParsed` : une disposition
  dont un menu contient `{"label": "Spot", "items": ["a.b", "-", "c.d"]}` se lit ; `MenuSpec::items` garde l'ordre ;
  le sous-menu est rendu par `MenuSpec::submenus` (voir étape 2) ; `unknownCommands` voit `a.b` et `c.d`. Dans
  `tests/tst_ui_commands.cpp`, `submenuFilteredByPlatform` : un sous-menu dont toutes les commandes sont absentes
  d'Android disparaît du menu sous Android ; présent sous Windows, il apparaît dans `menus()` comme
  `{submenu: true, label, items: [...]}`.
- [ ] **Étape 2 : sous-menus, code.** `actions/include/actcraft/actions/layout.h` : ajouter
  ```cpp
  // Sous-menu d'un menu (L7-T2) : dans MenuSpec::items, sa place est marquée « >n » (n = indice dans submenus).
  struct SubmenuSpec
  {
      QString label;
      QStringList items; // identifiants ou "-"
  };
  ```
  et `std::vector<SubmenuSpec> submenus;` dans `MenuSpec`. `actions/src/layout.cpp` : un élément objet de `items`
  devient `">" + QString::number(index)` et son contenu va dans `submenus` ; un sous-menu dans un sous-menu est une
  erreur (« sous-menu imbriqué »). `unknownCommands` parcourt aussi les sous-menus. `ui/src/commands.cpp`,
  `menus()` : un `">n"` rend `{submenu: true, label, items}` filtré comme un menu (omis s'il est vide). 
  `ui/qml/CommandMenu.qml` : un élément `submenu` crée un `Menu` enfant par `Instantiator` (jamais de boucle
  `removeItem`, piège du fichier d'état), avec les mêmes délégués que le menu parent.
- [ ] **Étape 3 : disposition du lot.** `actions/data/layout.json` : menu `view` ← après `view.fit` : `"-",
  "view.page-zoom", "view.page-effects"` ; menu `insert` ← `["insert.date-time", "insert.equation"]` ; menu `tools` ←
  après `tool.pen`, dans l'ordre de F-750 : `"-", "desktop.annotate", "desktop.tools", "-", "tool.handwriting",
  "tool.shape-recognition", "-", "present.curtain", {"label": "Spot", "items": ["present.spotlight-circle",
  "present.spotlight-square", "present.spotlight-solid-circle", "present.spotlight-solid-square"]}, {"label":
  "Caméra", "items": ["tool.camera-area", "tool.camera-polygon", "tool.camera-freehand", "tool.camera-window",
  "tool.camera-screen"]}, {"label": "Outils de maths", "items": ["math.ruler", "math.protractor", "math.set-square",
  "math.compass", "math.xy-origin", "tool.dice", "tool.calculator"]}, {"label": "Autres outils", "items":
  ["tool.keyboard", "tool.clock", "tool.ticker", "tool.web-browser"]}` ; palette : groupe `{ "group": "presentation",
  "items": ["present.curtain", "present.spotlight-circle", "view.page-zoom", "math.ruler", "tool.clock"] }` après le
  groupe `outils`. Les icônes citées sont dessinées par leurs tâches ; d'ici là le bouton montre son libellé
  (comportement existant de `CommandButton` sans icône : à vérifier, sinon ne mettre le groupe `presentation` dans la
  palette qu'avec les icônes présentes et laisser L7-T3, T4, T5, T9, T16 ajouter chacune la sienne — la ligne de la
  palette est alors réservée vide `{ "group": "presentation", "items": [] }`).
- [ ] **Étape 4 : vue, test rouge.** `tests/tst_ui_presentation.cpp` (charge `Main.qml` hors écran comme
  `tests/tst_ui_shell.cpp` ; lire ce test et reprendre son montage) :
  ```cpp
  void overlaySlotsExist();          // pageOverlays (enfant de « page »), presentationLayer, presentation trouvés
  void sampleFilterReachesThePen();  // filtre qui décale x de +50 : un trait injecté de (100,100) à (200,100) donne
                                     // un trait versé dont le premier point est (150,100) ; filtre vide : (100,100)
  void zoomAroundKeepsThePointStill(); // zoomAround(QPointF(300,200), 2.0) : sceneToPage(300,200) inchangé (±0,5)
  void insertTextIsOneUndoStep();    // insertText("12", {100,100,80,40}) : 1 objet Text de plus, couche
                                     // intermédiaire ; undo : retiré ; l'id rendu est non nul
  void strokeObserverSeesEveryStroke(); // 2 traits injectés : l'observateur reçoit 2 identifiants, ceux des traits
  void replaceObjectsIsOneUndoStep(); // via le contrôleur : 1 trait → 1 forme ; undo rend le trait
  ```
  Déclarer dans `tests/presentation.cmake` :
  ```cmake
  actcraft_add_test(tst_ui_presentation SOURCES tst_ui_presentation.cpp LIBS ActCraft::ui ActCraft::features Qt6::Quick)
  ```
  (aligner `LIBS` sur la déclaration de `tst_ui_shell` dans `tests/ui.cmake`, plugin QML compris).
- [ ] **Étape 5 : vue et contrôleur, code.** `ui/include/actcraft/ui/pageview.h` et `ui/src/pageview.cpp` : membres
  du contrat ; le filtre est appliqué **à chaque échantillon juste avant** l'appel au stylo (`m_pen->onSample`) — lire
  `pageview.cpp` et poser l'appel au seul endroit où les échantillons atteignent le stylo ; `zoomAround` = zoom
  ancré sur le point (même règle de cache que la molette : la vue demande le redessin à sa fin de zoom, politique du
  lot 10 inchangée) ; `sceneToPage` = inverse de `pageToScene`. `ui/include/actcraft/ui/documentcontroller.h` et
  `ui/src/documentcontroller.cpp` : **un seul bloc** commenté `// Lot 7 — présentation et outils` à la fin de la
  partie publique et à la fin du fichier source, avec les membres du contrat ; `insertObject` et `insertText` passent
  par `execute(std::make_unique<core::AddObjectCommand>(index, …))` ; `insertObjectOnNewPage` par `AddPageCommand` +
  `AddObjectCommand` dans une même étape (commande composée locale au fichier) puis affiche la page ;
  `replaceObjects` par `core::ReplaceObjectsCommand` ; `setPagePresentation` par
  `core::SetPagePresentationCommand` ; `insertText` crée un `core::Text` dont le HTML est le texte échappé
  (`QString::toHtmlEscaped`), police par défaut N2-3 ; l'observateur est appelé dans `commitStroke` après
  l'insertion, avec l'identifiant du trait.
- [ ] **Étape 6 : emplacements QML.** `ui/qml/PageArea.qml` : dans `pageContent`, **après** l'encre (et après la
  couche d'attente du lot 10), ajouter :
  ```qml
  // Lot 7 : surcouches en coordonnées de page (outils de maths, origine XY, spot, rideau de la page) ; jamais dans
  // la scène que le cache photographie.
  Item {
      id: pageOverlays
      objectName: "pageOverlays"
      anchors.fill: parent
  }
  ```
  et `property alias pageOverlays: pageOverlays` en tête. `ui/qml/Main.qml`, à la fin du `ApplicationWindow` (après
  `messageBanner`) :
  ```qml
  // Lot 7 : surcouches plein cadre et panneaux flottants (rideau, spot, message défilant, dés, horloge…).
  Item {
      id: presentationLayer
      objectName: "presentationLayer"
      anchors.fill: parent
      z: 50
  }
  Presentation {
      objectName: "presentation"
      commands: appCommands
      documentController: appDocument
      pageArea: pageArea
      layer: presentationLayer
  }
  ```
  `ui/qml/Presentation.qml` :
  ```qml
  import QtQuick
  import ActCraft.Ui

  // Racine des fonctions du lot 7 : chaque fonction pose ici son contrôleur, sa surcouche et ses gestionnaires de
  // commandes (commands.setHandler). Aucune autre racine n'est modifiée par le lot.
  Item {
      id: presentation
      property Commands commands: null
      property DocumentController documentController: null
      property Item pageArea: null
      property Item layer: null
  }
  ```
  `ui/qml/FloatingPanel.qml` : panneau flottant commun (titre, bouton de fermeture, déplaçable par sa barre de titre
  avec un `DragHandler`, borné à la fenêtre, jetons de `Theme`), propriétés `title`, `default property alias content`,
  signal `closed()`.
- [ ] **Étape 7 : CMake du lot.** `ui/presentation.cmake` :
  ```cmake
  # Sources QML et C++ du lot 7 (présentation et outils) : ajoutées au module ActCraft.Ui hors de ui/CMakeLists.txt.
  qt_target_qml_sources(actcraft_ui
      QML_FILES
          qml/Presentation.qml
          qml/FloatingPanel.qml)
  target_link_libraries(actcraft_ui PUBLIC ActCraft::features)
  ```
  et à la fin de `ui/CMakeLists.txt` : `include(${CMAKE_CURRENT_SOURCE_DIR}/presentation.cmake)`. Si
  `qt_target_qml_sources` refuse d'ajouter à un module déjà finalisé (erreur de configuration), déplacer les deux
  fichiers dans `QML_FILES` de `ui/CMakeLists.txt` et faire de `presentation.cmake` une liste de variables
  (`set(ACTCRAFT_UI_LOT7_QML …)`, `set(ACTCRAFT_UI_LOT7_SOURCES …)`) lue par `qt_add_qml_module` — dans les deux cas,
  les tâches suivantes ne modifient que `ui/presentation.cmake`. `features/CMakeLists.txt` : `DEPENDS` gagne
  `ActCraft::input`.
- [ ] **Étape 8 : étapes de scénario.** Test rouge `pointerStepParsed` dans `tests/tst_bench_uiscenario.cpp` (les deux
  formes du contrat lues ; `button` inconnu → `SCENARIO INVALIDE` ; `holdMs` négatif refusé ; `space` par défaut
  `page`). Puis `UiStepKind::Pointer` et `UiStepKind::DoubleClick` **après** `CheckEdge` dans
  `bench/include/actcraft/bench/uiscenario.h`, lecture et exécution dans `bench/src/uiscenario.cpp` par l'injecteur
  existant (événements souris `QMouseEvent` postés à la fenêtre, coordonnées de page converties par
  `pageView.pageToScene`), lignes `POINTEUR <n> OK` (n = nombre de points) et `DOUBLECLIC OK`.
- [ ] **Étape 9 : preuves et commit.** `format.ps1`, preuve de tâche, banc sans écran, preuve visuelle `premiers-pas`,
  `NOMS OK`, puis :
  ```powershell
  git -C <wt> add actions/data/layout.json actions/include/actcraft/actions/layout.h actions/src/layout.cpp tests/tst_actions_layout.cpp ui/src/commands.cpp ui/qml/CommandMenu.qml tests/tst_ui_commands.cpp ui/include/actcraft/ui/pageview.h ui/src/pageview.cpp ui/include/actcraft/ui/documentcontroller.h ui/src/documentcontroller.cpp ui/qml/PageArea.qml ui/qml/Main.qml ui/qml/Presentation.qml ui/qml/FloatingPanel.qml ui/CMakeLists.txt ui/presentation.cmake features/CMakeLists.txt bench/include/actcraft/bench/uiscenario.h bench/src/uiscenario.cpp tests/tst_bench_uiscenario.cpp tests/presentation.cmake tests/tst_ui_presentation.cpp
  git -C <wt> commit -m "Lot 7 : réservation de l'interface (sous-menus, emplacements de surcouches, filtre d'échantillons, insertion d'objets, étapes de scénario) (L7-T2)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```


<a id="tache-l7-t3"></a>
### Tâche L7-T3 — Rideau (F-265)

**Objectif.** Ctrl+R (ou `present.curtain`) bascule un store opaque (noir par défaut, `present.curtain-color`) qui
couvre la zone de la page. Glisser depuis le haut, le bas, la gauche ou la droite le découvre progressivement ; il
reste actif en changeant de page (« comme avant », la règle par page vient en L7-T6). Le cache n'est jamais
redessiné.

**Fichiers.** Voir la table (ligne L7-T3).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_curtain|tst_ui_presentation|tst_captures_rideau'`,
preuve de tâche (N ≥ N0 + 4), banc sans écran, preuve visuelle :
`-Scenario rideau -Lire "curtain.active=true"` puis `-Lire "curtain.coverage=0.5"` (deux passages ou une lecture
double, selon `captures.ps1`).

**Preuve visuelle.** Scénario `bench/scenarios/rideau.json` : `present.curtain` ; `checkPixel` noir au centre de la
page ; `pointer` depuis le haut de la page (512, 2) jusqu'à (512, 384), bouton gauche ; `expect`
`curtain.coverage` = 0,5 ± 0,02 (valeur arrondie à 2 décimales par le contrôleur) ; `checkPixel` couleur de la page à
(512, 200) et noir à (512, 600) ; captures `rideau-ferme`, `rideau-a-moitie`. Décrire : la moitié haute visible, la
moitié basse noire, poignée de tirage au bord du store.

**Interdits.** Le rideau n'est pas un objet de page, n'entre pas dans le document, ne passe pas par
`pageOverlays` (il couvre la zone de la page à l'écran, dans `presentationLayer`). Aucun appel à
`requestCacheRefresh`.

**Terminé quand.** Tests verts ; `curtainNeverRefreshesTheCache` (0 émission de `cacheRefreshRequested` pendant
l'ouverture et un tirage complet) ; captures conformes ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge de la logique.** `tests/tst_features_curtain.cpp` :
  ```cpp
  #include <actcraft/features/curtain.h>

  #include <QtTest>

  using namespace actcraft::features;

  class TstFeaturesCurtain : public QObject
  {
      Q_OBJECT
  private slots:
      void closedCoversEverything()
      {
          Curtain c(QSizeF(1000, 800));
          QCOMPARE(c.coverage(), 1.0);
          QCOMPARE(c.covered(), QRectF(0, 0, 1000, 800));
      }
      void dragFromTopUncovers()
      {
          Curtain c(QSizeF(1000, 800));
          QVERIFY(c.press(QPointF(500, 3)));      // à moins de kGrabMargin du bord haut
          c.move(QPointF(500, 400));
          c.release();
          QCOMPARE(c.covered(), QRectF(0, 400, 1000, 400));
          QCOMPARE(c.coverage(), 0.5);
      }
      void dragFromLeftUncovers()
      {
          Curtain c(QSizeF(1000, 800));
          QVERIFY(c.press(QPointF(2, 300)));
          c.move(QPointF(250, 300));
          QCOMPARE(c.covered(), QRectF(250, 0, 750, 800));
      }
      void pressInsideDoesNotGrab()
      {
          Curtain c(QSizeF(1000, 800));
          QVERIFY(!c.press(QPointF(500, 400)));
      }
      void edgeOfAnOpenCurtainCanBeDraggedBack()
      {
          Curtain c(QSizeF(1000, 800));
          c.press(QPointF(500, 1));
          c.move(QPointF(500, 400));
          c.release();
          QVERIFY(c.press(QPointF(500, 402)));   // le bord du store est à 400
          c.move(QPointF(500, 100));
          QCOMPARE(c.covered().top(), 100.0);
      }
      void clampedToTheArea()
      {
          Curtain c(QSizeF(1000, 800));
          c.press(QPointF(500, 1));
          c.move(QPointF(500, 5000));
          QCOMPARE(c.coverage(), 0.0);
      }
      void resizeKeepsProportions()
      {
          Curtain c(QSizeF(1000, 800));
          c.press(QPointF(500, 1));
          c.move(QPointF(500, 400));
          c.release();
          c.setArea(QSizeF(500, 400));
          QCOMPARE(c.covered(), QRectF(0, 200, 500, 200));
      }
      void startModeAndDistance()
      {
          Curtain c(QSizeF(1000, 800));
          c.reset(Curtain::Start::Top, 100);     // F-592 : mode « supérieur », distance 100
          QCOMPARE(c.covered(), QRectF(0, 100, 1000, 700));
      }
  };

  QTEST_GUILESS_MAIN(TstFeaturesCurtain)
  #include "tst_features_curtain.moc"
  ```
  Déclarer dans `tests/presentation.cmake` :
  `actcraft_add_test(tst_features_curtain SOURCES tst_features_curtain.cpp LIBS ActCraft::features)`.
- [ ] **Étape 2 : logique.** `features/include/actcraft/features/curtain.h` :
  ```cpp
  #pragma once

  #include <QPointF>
  #include <QRectF>
  #include <QSizeF>

  namespace actcraft::features {

  // Store du rideau (F-265, F-592) dans un rectangle de taille `area` : quatre bords mobiles, le store couvre ce qui
  // reste entre eux. Un appui à moins de kGrabMargin d'un bord du store (ou du cadre, store fermé) saisit ce bord.
  class Curtain
  {
  public:
      enum class Start { Full, Top, Bottom, Left, Right };
      static constexpr double kGrabMargin = 24; // pixels de la zone (unités de `area`)

      explicit Curtain(QSizeF area);
      void setArea(QSizeF area);        // garde les proportions découvertes
      void reset(Start start, double distance); // store fermé, ouvert de `distance` depuis le bord `start`
      bool press(QPointF point);        // vrai si un bord est saisi
      void move(QPointF point);
      void release();
      bool dragging() const noexcept;
      QRectF covered() const noexcept;  // vide : tout découvert
      double coverage() const noexcept; // aire couverte / aire de la zone, 0 à 1

  private:
      enum class Edge { None, Top, Bottom, Left, Right };
      QSizeF m_area;
      double m_top = 0, m_bottom = 0, m_left = 0, m_right = 0; // fractions découvertes depuis chaque bord, 0 à 1
      Edge m_edge = Edge::None;
  };

  } // namespace actcraft::features
  ```
  `features/src/curtain.cpp` : `covered()` = `QRectF(m_left·w, m_top·h, w·(1−m_left−m_right), h·(1−m_top−m_bottom))`,
  vide si largeur ou hauteur ≤ 0 ; `press` cherche le bord du **store** le plus proche (haut du store = `m_top·h`, etc.)
  à moins de `kGrabMargin` ; `move` met la fraction du bord saisi à la position, bornée pour que les fractions
  opposées ne se croisent pas (somme ≤ 1) ; `reset(Full, _)` ferme tout ; `reset(Top, d)` met `m_top = d / h`, etc.
  Ajouter `include/actcraft/features/curtain.h` et `src/curtain.cpp` aux `SOURCES` de `features/CMakeLists.txt`.
  Commande ciblée `-R tst_features_curtain` : vert.
- [ ] **Étape 3 : contrôleur QML.** `ui/src/curtaincontroller.h` (`QML_ELEMENT`, `CurtainController`) : propriétés
  `active` (bool), `color` (QColor, noir), `area` (QSizeF, écrite par la surcouche), `covered` (QRectF, NOTIFY
  `changed`), `coverage` (réel arrondi à 0,01) ; `Q_INVOKABLE bool press(QPointF)`, `move(QPointF)`, `release()`,
  `reset(int start, qreal distance)`, `toggle()`. Une instance par fenêtre ; aucun lien au document.
- [ ] **Étape 4 : surcouche.** `ui/qml/CurtainOverlay.qml` : `Item` posé dans `presentation.layer`, aligné sur
  `pageArea` (`x`, `y`, `width`, `height` liés par `mapToItem`) ; un `Rectangle` couleur `controller.color` au
  rectangle `controller.covered` ; une poignée ronde (dessin à nous, jetons de `Theme`) au milieu du bord saisissable ;
  un `PointHandler` / `DragHandler` (tous périphériques) qui appelle `press/move/release` et **accepte** l'événement
  seulement quand `press` rend vrai — sinon l'événement traverse vers la page (le stylo écrit dans la partie
  découverte). `visible: controller.active`. Dans `ui/qml/Presentation.qml` :
  ```qml
  CurtainController { id: curtain; objectName: "curtain" }
  CurtainOverlay { parent: presentation.layer; controller: curtain; pageArea: presentation.pageArea }
  Component.onCompleted: {
      commands.setHandler("present.curtain", function () { curtain.toggle(); commands.setChecked("present.curtain", curtain.active) })
      commands.setHandler("present.curtain-color", function (c) { curtain.color = c })
  }
  ```
  Déclarer `qml/CurtainOverlay.qml`, `src/curtaincontroller.h`, `src/curtaincontroller.cpp`, `icons/rideau.svg` dans
  `ui/presentation.cmake` (`QML_FILES`, `SOURCES`, `RESOURCES`). Dessiner `ui/icons/rideau.svg` (à nous, 24 px).
- [ ] **Étape 5 : test d'interface.** Dans `tests/tst_ui_presentation.cpp`, `curtainNeverRefreshesTheCache` :
  `QSignalSpy spy(pageView, &PageView::cacheRefreshRequested)` après `waitReady` ; `present.curtain` ; tirage souris
  injecté du haut au milieu ; `QCOMPARE(spy.count(), 0)` ; `curtain.coverage` ≈ 0,5 ; un trait au stylo dans la
  moitié découverte est versé (le document a un objet de plus).
- [ ] **Étape 6 : scénario de captures.** `bench/scenarios/rideau.json` (format `actcraft-scenario/1`, étapes de la
  « Preuve visuelle ») et, dans `tests/presentation.cmake`, sur le modèle de `tst_captures_premiers_pas`
  (`tests/bench.cmake:107`) :
  ```cmake
  if(TARGET actcraft-captures)
      add_test(NAME tst_captures_rideau
          COMMAND $<TARGET_FILE:actcraft-captures>
              --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/rideau.json
              --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-rideau)
      set_tests_properties(tst_captures_rideau PROPERTIES
          TIMEOUT 300 ENVIRONMENT "${_bench_env_offscreen}" PASS_REGULAR_EXPRESSION "SCENARIO OK")
  endif()
  ```
  (`presentation.cmake` est inclus après `bench.cmake` : `_bench_env_offscreen` y est défini.)
- [ ] **Étape 7 : preuves et commit.** `format.ps1`, preuve de tâche, banc sans écran, preuve visuelle, `NOMS OK` ;
  `git -C <wt> add` de chaque fichier de la ligne L7-T3 de la table, puis commit
  « Lot 7 : rideau (F-265), sans redessin du cache (L7-T3) » terminé par la ligne `Co-Authored-By`.

<a id="tache-l7-t4"></a>
### Tâche L7-T4 — Spot (F-270)

**Objectif.** Quatre commandes exclusives (`present.spotlight-circle`, `-square`, `-solid-circle`, `-solid-square`) :
spot rond ou carré (tout est masqué sauf la zone), cache rond ou carré (tout est visible sauf la zone). Glisser
dans la zone masquée déplace le spot ; glisser son pourtour le redimensionne (ovale ou rectangle possibles) ;
recliquer la commande active ferme le spot.

**Fichiers.** Voir la table (ligne L7-T4).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_spotlight|tst_ui_presentation|tst_captures_spot'`,
preuve de tâche (N ≥ N0 + 6), banc sans écran, preuve visuelle `-Scenario spot -Lire "spotlight.shape=1"`.

**Preuve visuelle.** `bench/scenarios/spot.json` : `present.spotlight-circle` ; `checkPixel` couleur de la page au
centre du spot, noir à 20 px du bord de la page ; `pointer` qui déplace le spot de (512, 384) vers (300, 300) ;
`expect` `spotlight.center` ; `pointer` sur le pourtour qui l'élargit en ovale ; captures `spot-rond`,
`spot-deplace`, `spot-ovale`. Décrire : disque clair sur fond noir, déplacé, puis ovale.

**Interdits.** Comme L7-T3 : pas d'objet de page, aucun redessin du cache, l'événement non saisi traverse.

**Terminé quand.** Tests verts ; `spotlightNeverRefreshesTheCache` ; captures conformes ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.** `tests/tst_features_spotlight.cpp` (même montage que `tst_features_curtain`) :
  ```cpp
  void defaultFrameIsCentered()
  {
      Spotlight s(QSizeF(1000, 800));
      s.setShape(Spotlight::Shape::Circle);
      QCOMPARE(s.frame().center(), QPointF(500, 400));
      QCOMPARE(s.frame().width(), 300.0);              // kDefaultSize
  }
  void dragInsideMaskMoves()
  {
      Spotlight s(QSizeF(1000, 800));
      s.setShape(Spotlight::Shape::Circle);
      QVERIFY(s.press(QPointF(100, 100)));             // dans le masque
      s.move(QPointF(150, 120));
      s.release();
      QCOMPARE(s.frame().center(), QPointF(550, 420));
  }
  void dragOnRimResizesToOval()
  {
      Spotlight s(QSizeF(1000, 800));
      s.setShape(Spotlight::Shape::Circle);
      QVERIFY(s.press(QPointF(650, 400)));             // bord droit du cadre (500 + 150)
      s.move(QPointF(750, 400));
      s.release();
      QCOMPARE(s.frame().width(), 400.0);
      QCOMPARE(s.frame().height(), 300.0);
  }
  void minimumSize()
  {
      Spotlight s(QSizeF(1000, 800));
      s.setShape(Spotlight::Shape::Square);
      s.press(QPointF(650, 400));
      s.move(QPointF(0, 400));
      QVERIFY(s.frame().width() >= Spotlight::kMinimumSize);
  }
  void solidSpotHidesInside()
  {
      Spotlight s(QSizeF(1000, 800));
      s.setShape(Spotlight::Shape::SolidCircle);
      QVERIFY(s.hidesInside());
      QVERIFY(s.press(QPointF(500, 400)));             // cache plein : on le saisit par l'intérieur
  }
  void frameKeptInsideTheArea()
  {
      Spotlight s(QSizeF(1000, 800));
      s.setShape(Spotlight::Shape::Circle);
      s.press(QPointF(100, 100));
      s.move(QPointF(-5000, -5000));
      QVERIFY(QRectF(0, 0, 1000, 800).intersects(s.frame()));
  }
  ```
  Déclarer `tst_features_spotlight` dans `tests/presentation.cmake` (`LIBS ActCraft::features`).
- [ ] **Étape 2 : logique.** `features/include/actcraft/features/spotlight.h` : classe `Spotlight` (`enum class Shape
  { Off, Circle, Square, SolidCircle, SolidSquare }`, mêmes valeurs que `core::SpotlightShape` ;
  `kDefaultSize = 300`, `kMinimumSize = 40`, `kRimMargin = 16`) ; `setArea`, `setShape`, `shape`, `setFrame`,
  `frame`, `hidesInside()` (vrai pour les deux caches pleins), `press/move/release` : sur le pourtour (distance au
  bord du cadre ≤ `kRimMargin`) → redimensionner le côté saisi ; dans la partie masquée (hors du cadre pour un spot,
  dans le cadre pour un cache) → déplacer ; ailleurs → faux. Cadre borné pour garder une intersection avec la zone.
  `features/src/spotlight.cpp` ; ajout aux `SOURCES` de `features/CMakeLists.txt`.
- [ ] **Étape 3 : contrôleur et surcouche.** `ui/src/spotlightcontroller.h/.cpp` (`SpotlightController` : `shape`
  (int), `frame` (QRectF), `area`, `color` (noir), `press/move/release`, `toggle(int shape)`) ;
  `ui/qml/SpotlightOverlay.qml` : masque dessiné par un `Shape` Qt Quick (`ShapePath` à remplissage pair-impair :
  rectangle de la zone + ellipse ou rectangle du cadre) pour le spot, ou la seule ellipse / le seul rectangle pour le
  cache plein ; mêmes règles d'événements que le rideau. Dans `Presentation.qml`, quatre gestionnaires
  `present.spotlight-*` : la commande active ferme le spot, une autre change la forme ; `setChecked` tenu à jour
  pour les quatre ; ouvrir le spot ferme le rideau et inversement (F-591 : un seul outil de page à la fois).
  `ui/presentation.cmake`, `ui/icons/spot.svg`.
- [ ] **Étape 4 : test d'interface et scénario.** `spotlightNeverRefreshesTheCache` dans `tst_ui_presentation.cpp`
  (spy à 0 après ouverture, déplacement, redimensionnement ; `curtain.active` faux après ouverture du spot) ;
  `bench/scenarios/spot.json` ; `tst_captures_spot` dans `tests/presentation.cmake` (bloc de L7-T3, autre nom).
- [ ] **Étape 5 : preuves et commit.** Comme L7-T3 ; message « Lot 7 : spot (F-270), sans redessin du cache (L7-T4) ».

<a id="tache-l7-t5"></a>
### Tâche L7-T5 — Zoom de page (F-260)

**Objectif.** `view.page-zoom` (Ctrl+Shift+Z, cochable) : tant qu'il est actif, maintenir le bouton gauche (ou le
stylet) sur la page zoome en continu autour du point, jusqu'au relâché ; le bouton droit dézoome ; glisser fait un
panoramique ; un double-clic revient à « Ajuster » ; le document n'est jamais modifié. Le cache n'est pas redessiné
pendant l'appui, une seule fois au relâché.

**Fichiers.** Voir la table (ligne L7-T5).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_pagezoom|tst_ui_presentation'`, preuve de tâche
(N ≥ N0 + 7), banc sans écran.

**Interdits.** Ne pas toucher à `pageview.*` (L7-T2 a posé `zoomAround` et `sceneToPage`). Aucun trait ne part pendant
le mode (la surcouche saisit tout événement de pointeur sur la page tant que le mode est actif).

**Terminé quand.** Tests verts, dont `pageZoomRefreshesTheCacheOnceAtRelease` (spy = 0 pendant 600 ms de maintien,
puis ≤ 1 après le relâché) et `pageZoomLeavesTheDocumentUnchanged` (empreinte identique) ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.** `tests/tst_features_pagezoom.cpp` :
  ```cpp
  void holdZoomsInAroundThePoint()
  {
      PageZoom z(1.0);                                   // zoom de départ
      z.press(QPointF(300, 200), PageZoom::Direction::In, 0);
      QCOMPARE(z.zoomAt(500'000'000), 1.0 * std::pow(PageZoom::kRatePerSecond, 0.5)); // 500 ms
      QCOMPARE(z.anchor(), QPointF(300, 200));
  }
  void rightButtonZoomsOut()
  {
      PageZoom z(2.0);
      z.press(QPointF(0, 0), PageZoom::Direction::Out, 0);
      QVERIFY(z.zoomAt(1'000'000'000) < 2.0);
  }
  void clampedToViewLimits()
  {
      PageZoom z(1.0);
      z.press(QPointF(0, 0), PageZoom::Direction::In, 0);
      QCOMPARE(z.zoomAt(60'000'000'000), PageZoom::kMaxZoom);
  }
  void dragTurnsIntoPan()
  {
      PageZoom z(1.0);
      z.press(QPointF(100, 100), PageZoom::Direction::In, 0);
      QVERIFY(z.move(QPointF(100 + PageZoom::kPanThreshold + 1, 100), 50'000'000)); // vrai : panoramique
      QVERIFY(z.panning());
      QCOMPARE(z.zoomAt(500'000'000), 1.0);              // plus de zoom une fois en panoramique
  }
  void doubleClickFits()
  {
      PageZoom z(3.0);
      QVERIFY(z.isDoubleClick(QPointF(10, 10), 0) == false);
      QVERIFY(z.isDoubleClick(QPointF(12, 11), 200'000'000));
  }
  ```
  `PageZoom::kRatePerSecond = 2.0` (le zoom double en une seconde ; valeur à nous, confirmée ou corrigée par L7-T0),
  `kMaxZoom` et `kMinZoom` = bornes de la vue (lire `ui/include/actcraft/ui/viewtransform.h` et reprendre ses
  constantes ; les dupliquer en `features` sans dépendre d'`ui`), `kPanThreshold = 8` pixels, double-clic : 400 ms et
  6 pixels. Déclarer `tst_features_pagezoom`.
- [ ] **Étape 2 : logique.** `features/include/actcraft/features/pagezoom.h`, `features/src/pagezoom.cpp` : classe
  `PageZoom` (fonctions pures du temps, aucune minuterie), `press`, `move` (vrai si l'appui devient un panoramique ;
  rend le déplacement à appliquer par `panDelta()`), `release`, `zoomAt(nowNs)`, `anchor()`, `panning()`,
  `isDoubleClick(point, nowNs)`. `features/CMakeLists.txt`.
- [ ] **Étape 3 : contrôleur et surcouche.** `ui/src/pagezoomcontroller.h/.cpp` (`PageZoomController` : `active`,
  propriété `view` (`PageView*`) ; une `QTimer` à 16 ms **pendant l'appui seulement** qui applique
  `view->zoomAround(anchor, zoomAt(now))` ; au relâché, plus rien : la vue fait sa fin de zoom elle-même ; double-clic
  → `view->fitPage()`) ; `ui/qml/PageZoomOverlay.qml` : `MouseArea` (boutons gauche et droit,
  `preventStealing: true`) posée sur `pageArea` quand le mode est actif, curseur main pendant le panoramique.
  `Presentation.qml` : gestionnaire `view.page-zoom` (bascule, `setChecked`). `ui/presentation.cmake`,
  `ui/icons/zoom-page.svg`.
- [ ] **Étape 4 : tests d'interface.** `pageZoomRefreshesTheCacheOnceAtRelease`, `pageZoomLeavesTheDocumentUnchanged`,
  `pageZoomBlocksThePen` (un appui maintenu en mode zoom ne verse aucun trait) dans `tst_ui_presentation.cpp`, avec
  l'étape `pointer` par l'injecteur du banc ou des `QMouseEvent` envoyés à la fenêtre.
- [ ] **Étape 5 : preuves et commit.** Preuve de tâche, banc sans écran, `NOMS OK` ; commit « Lot 7 : zoom de page
  (F-260) (L7-T5) ».

<a id="tache-l7-t6"></a>
### Tâche L7-T6 — Outils de page par page (F-591, F-592, F-593)

**Objectif.** Chaque page porte ses outils de présentation (`core::PagePresentation`, L7-T1) : « comme avant »
(défaut : l'état du rideau ou du spot est gardé en changeant de page), « rideau » (la page s'ouvre rideau fermé, au
mode et à la distance de la page), « spot » (spot de la forme et du cadre de la page), « désactivés » (rideau et spot
fermés). Commandes `page.tools-mode`, `page.curtain-mode`, `page.curtain-distance`, `page.spotlight-mode` (une étape
d'annulation chacune). Les `.flipchart` qui portent ces réglages les donnent à la page.

**Fichiers.** Voir la table (ligne L7-T6).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_pagetools|tst_formats_flipchart|tst_ui_presentation'`,
preuve de tâche (N ≥ N0 + 8).

**Interdits.** Ne pas modifier `core/` ni `nativejson.cpp` (champs et format posés en L7-T1). Valeurs d'énumération
des `.flipchart` écrites à la main d'après notre documentation ou le relevé de L7-T0 ; inconnues → valeur par défaut
et avertissement de lecture, jamais d'échec.

**Terminé quand.** Tests verts, dont `keepCarriesCurtainAcrossPages`, `offStopsTools`, `pageModeOverridesKeep`,
`pagePresentationRead`, `pageChangeAppliesPageTools` ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge de la règle.** `tests/tst_features_pagetools.cpp` :
  ```cpp
  using actcraft::core::PagePresentation;
  using actcraft::core::PageToolsMode;
  using actcraft::core::CurtainMode;
  using actcraft::core::SpotlightShape;
  using namespace actcraft::features;

  void keepCarriesCurtainAcrossPages()
  {
      PresentationState now{true, SpotlightShape::Off};          // rideau ouvert sur la page quittée
      const auto next = resolvePageTools(now, PagePresentation{}); // page « comme avant »
      QVERIFY(next.curtain);
      QCOMPARE(next.resetCurtain, false);
  }
  void offStopsTools()
  {
      PagePresentation p;
      p.tools = PageToolsMode::Off;
      const auto next = resolvePageTools({true, SpotlightShape::Circle}, p);
      QVERIFY(!next.curtain);
      QCOMPARE(next.spotlight, SpotlightShape::Off);
  }
  void pageModeOverridesKeep()
  {
      PagePresentation p;
      p.tools = PageToolsMode::Curtain;
      p.curtain = CurtainMode::Left;
      p.curtainDistance = 120;
      const auto next = resolvePageTools({false, SpotlightShape::Off}, p);
      QVERIFY(next.curtain);
      QVERIFY(next.resetCurtain);
      QCOMPARE(next.curtainStart, Curtain::Start::Left);
      QCOMPARE(next.curtainDistance, 120.0);
  }
  void spotlightPageUsesItsFrame()
  {
      PagePresentation p;
      p.tools = PageToolsMode::Spotlight;
      p.spotlight = SpotlightShape::Square;
      p.spotlightBounds = {10, 20, 200, 100};
      const auto next = resolvePageTools({true, SpotlightShape::Off}, p);
      QVERIFY(!next.curtain);
      QCOMPARE(next.spotlight, SpotlightShape::Square);
      QCOMPARE(next.spotlightFrame, QRectF(10, 20, 200, 100));
  }
  ```
  (`core::RectF` → `QRectF` par une conversion locale au test.) Déclarer `tst_features_pagetools`.
- [ ] **Étape 2 : logique.** `features/include/actcraft/features/pagetools.h` :
  ```cpp
  struct PresentationState { bool curtain = false; core::SpotlightShape spotlight = core::SpotlightShape::Off; };
  struct PageToolsResult
  {
      bool curtain = false;
      bool resetCurtain = false;
      Curtain::Start curtainStart = Curtain::Start::Full;
      double curtainDistance = 0;
      core::SpotlightShape spotlight = core::SpotlightShape::Off;
      QRectF spotlightFrame; // vide : cadre par défaut
  };
  PageToolsResult resolvePageTools(PresentationState now, const core::PagePresentation& page);
  ```
  `features/src/pagetools.cpp` ; `CurtainMode::Off` sous `PageToolsMode::Curtain` vaut « complet » (F-592).
- [ ] **Étape 3 : application au changement de page.** `ui/src/pagetoolscontroller.h/.cpp` (`PageToolsController` :
  propriétés `documentController`, `curtain` (`CurtainController*`), `spotlight` (`SpotlightController*`) ; relié à
  `DocumentController::pagesChanged` : à chaque changement de page affichée, `resolvePageTools` et application aux deux
  contrôleurs). Gestionnaires `page.tools-mode` (`"keep"`, `"curtain"`, `"spotlight"`, `"off"`),
  `page.curtain-mode`, `page.curtain-distance`, `page.spotlight-mode` dans `Presentation.qml`, par
  `documentController.setPagePresentation(…)` (exposer, si besoin, un `Q_INVOKABLE` sur le contrôleur de la tâche, pas
  sur `DocumentController`). Le cadre du spot déplacé sur une page en mode « spot » est écrit dans la page au relâché
  (une étape). Test `pageChangeAppliesPageTools` dans `tst_ui_presentation.cpp`.
- [ ] **Étape 4 : lecture `.flipchart`.** Test rouge `pagePresentationRead` dans `tests/tst_formats_flipchart.cpp` :
  `tests/support/flipchartwriter.h` écrit, sur une page, le mode et la distance du rideau, le mode et le cadre du
  spot, l'origine XY (et, si L7-T0 en a établi le codage, l'effet propre à la page — sinon L7-T7) ; le lecteur rend
  ces valeurs dans `Page::presentation`. Numéros de propriétés de la page : ceux de notre documentation
  (`docs/re/format-flipchart.md`, liste des propriétés de la page) recopiés à la main dans `flipchartids.h` sous des
  noms à nous (`kPageCurtainMode`, `kPageCurtainDistance`, `kPageSpotlightMode`, `kPageSpotlightBounds`,
  `kPageOrigin`) ; codage des valeurs relevé par L7-T0.
- [ ] **Étape 5 : preuves et commit.** Preuve de tâche, banc sans écran, `NOMS OK` ; commit « Lot 7 : outils de
  présentation par page et leur lecture .flipchart (F-591 à F-593) (L7-T6) ».

<a id="tache-l7-t7"></a>
### Tâche L7-T7 — Effets de changement de page I (F-640)

**Objectif.** Catalogue d'effets (liste retenue par L7-T0, noms et dessin à nous, N2-A6), effet global
(`view.page-effect`, « aucun » par défaut), effet propre à une page (`page.effect`, joué **en quittant** la page, prime
sur le global), durée 5 à 30 dixièmes de seconde (défaut 15, `view.page-effect-duration`). Désactivés : rideau ou spot
actif, annotation de bureau, navigation autre que `page.next` / `page.previous` (les sauts du navigateur de pages,
lot 4, ne jouent rien). Le lecteur joue l'effet en surcouche : instantané de l'ancienne page, nouvelle page dessous,
animation sur le fil de rendu ; le cache n'est pas redessiné par l'effet.

**Fichiers.** Voir la table (ligne L7-T7).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_transitions|tst_ui_presentation'`, preuve de tâche
(N ≥ N0 + 9), banc sans écran.

**Interdits.** Aucun `grabToImage` (asynchrone, lent) : l'instantané est un `ShaderEffectSource` `live: false` de la
texture du cache déjà à jour (`pageArea.view` → objet `pageCache`) figé **avant** le changement de page par
`scheduleUpdate()` attendu une image. Pas de nuanceur exotique : opacité, translation, échelle, découpe (`clip`) et
au plus un nuanceur de fondu compilé par `qsb` (Qt Shader Tools, installé). Aucune boucle JavaScript par image : les
animations sont des `Animator` (fil de rendu).

**Terminé quand.** Tests verts, dont `pageEffectWinsOverGlobal`, `effectPlayedWhenLeavingThePage`,
`disabledWhileCurtainActive`, `durationClampedTo5To30`, `transitionDoesNotRefreshTheCacheBeforeTheEnd` (le seul
redessin du cache est celui que la vue fait déjà pour la nouvelle page) ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.** `tests/tst_features_transitions.cpp` :
  ```cpp
  void catalogueStartsWithNone()
  {
      const auto& list = transitionCatalogue();
      QVERIFY(list.size() >= 3);
      QCOMPARE(list.front().id, QStringLiteral("none"));
      for (const auto& e : list)
          QVERIFY(!e.label.isEmpty());
  }
  void pageEffectWinsOverGlobal()
  {
      TransitionSettings s;
      s.global = QStringLiteral("fade");
      QCOMPARE(effectForLeaving(s, std::string("slide-left"), NavigationKind::Next, {}), QStringLiteral("slide-left"));
      QCOMPARE(effectForLeaving(s, std::string(), NavigationKind::Next, {}), QStringLiteral("fade"));
  }
  void effectPlayedWhenLeavingThePage()
  {
      TransitionSettings s; // global « none »
      // l'effet de la page QUITTÉE compte, pas celui de la page d'arrivée
      QCOMPARE(effectForLeaving(s, std::string("fade"), NavigationKind::Previous, {}), QStringLiteral("fade"));
  }
  void disabledWhileCurtainActive()
  {
      TransitionSettings s;
      s.global = QStringLiteral("fade");
      QCOMPARE(effectForLeaving(s, {}, NavigationKind::Next, {true, false, false}), QStringLiteral("none"));
      QCOMPARE(effectForLeaving(s, {}, NavigationKind::Jump, {}), QStringLiteral("none"));
  }
  void unknownEffectFallsBackToNone()
  {
      TransitionSettings s;
      s.global = QStringLiteral("does-not-exist");
      QCOMPARE(effectForLeaving(s, {}, NavigationKind::Next, {}), QStringLiteral("none"));
  }
  void durationClampedTo5To30()
  {
      QCOMPARE(clampDurationTenths(2), 5);
      QCOMPARE(clampDurationTenths(15), 15);
      QCOMPARE(clampDurationTenths(99), 30);
  }
  ```
  Déclarer `tst_features_transitions`.
- [ ] **Étape 2 : logique.** `features/include/actcraft/features/transitions.h` :
  ```cpp
  struct TransitionEffect { QString id; QString label; }; // id anglais à nous ; label français à nous
  const std::vector<TransitionEffect>& transitionCatalogue(); // « none » d'abord, puis la liste de L7-T0
  struct TransitionSettings { QString global = QStringLiteral("none"); int durationTenths = 15; };
  enum class NavigationKind { Next, Previous, Jump };
  struct TransitionBlockers { bool curtainOrSpotlight = false; bool desktopAnnotation = false; bool designMode = false; };
  QString effectForLeaving(const TransitionSettings& settings, const std::string& leavingPageEffect,
                           NavigationKind navigation, TransitionBlockers blockers);
  int clampDurationTenths(int tenths) noexcept;
  ```
  Le catalogue suit la liste de L7-T0 ; à défaut (non tranché), il contient `none`, `fade`, `slide-left`,
  `slide-right`, `slide-up`, `slide-down`, `zoom`, `wipe-left`, `wipe-right`, `page-turn` (ce dernier dessiné par un
  pliage simple à nous).
- [ ] **Étape 3 : lecteur en surcouche.** `ui/src/transitioncontroller.h/.cpp` (`TransitionController` : `settings`
  (global, durée, gardés par `QSettings` sous `presentation/transition`), `playing` (bool), `played` (texte : dernier
  effet joué, pour les scénarios), `Q_INVOKABLE void navigate(int kind)`) ; `ui/qml/TransitionLayer.qml` : un
  `ShaderEffectSource` d'instantané posé au-dessus de `pageArea`, dans `presentation.layer`, et un `ParallelAnimation`
  d'`Animator` par effet ; déroulé : (1) figer l'instantané (une image), (2) changer de page par la commande
  d'origine, (3) jouer l'effet sur l'instantané pendant `durationTenths × 100` ms, (4) retirer l'instantané. Dans
  `Presentation.qml`, les gestionnaires `page.next` et `page.previous` **ne sont pas remplacés** : le contrôleur
  s'abonne à `Commands.triggered` et, si un effet doit jouer, fige l'instantané au signal
  `DocumentController::pageAboutToChange` (le changement a déjà lieu ; l'instantané est celui de la page quittée, prêt
  depuis la dernière image). Gestionnaires `view.page-effect`, `view.page-effect-duration`, `page.effect`
  (`setPagePresentation` avec le champ `transition`).
- [ ] **Étape 4 : tests d'interface.** `transitionDoesNotRefreshTheCacheBeforeTheEnd`, `noEffectWhenNone` (aucun
  instantané créé), `effectPlayedOnNext` (`played` = l'effet attendu) dans `tst_ui_presentation.cpp`.
- [ ] **Étape 5 : lecture `.flipchart` de l'effet par page.** Si L7-T0 a établi le codage de l'effet propre à la page
  et que L7-T6 ne l'a pas lu : l'ajouter ici (`formats/src/flipchart.cpp`, `formats/src/flipchartids.h`,
  `tests/support/flipchartwriter.h`, `tests/tst_formats_flipchart.cpp`, test `pageTransitionRead`) — ces quatre
  fichiers s'ajoutent alors à la ligne L7-T7 de la table (l'orchestrateur la corrige avant de lancer la tâche) ;
  sinon N2-A12 reste ouverte pour l'effet, à la porte de fin.
- [ ] **Étape 6 : preuves et commit.** Preuve de tâche, banc sans écran, `NOMS OK` ; commit « Lot 7 : effets de
  changement de page, catalogue et lecteur (F-640) (L7-T7) ».

<a id="tache-l7-t8"></a>
### Tâche L7-T8 — Effets II : navigateur d'effets, scénario et banc (F-640, S22)

**Objectif.** `view.page-effects` ouvre un navigateur d'effets à nous : une vignette animée par effet (aperçu joué
au survol, sur deux pages dessinées par nous), un clic choisit l'effet global, « Terminé » ferme ; une case
« pour cette page seulement » écrit `page.effect`. Scénario de captures `effets` ; chaque effet passe au banc
pendant une transition (mesure jouée à la clôture à l'écran, S22).

**Fichiers.** Voir la table (ligne L7-T8).

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_presentation|tst_captures_effets'`, preuve de tâche (N ≥ N0 +
10), banc sans écran, preuve visuelle `-Scenario effets -Lire "transition.played=fade"`.

**Preuve visuelle.** `bench/scenarios/effets.json` : `view.page-effect` `fade` ; `page.next` ; `wait` 600 ;
capture `fondu-milieu` pendant l'effet (attente de 750 ms à partir du déclenchement pour la durée par défaut, 1,5 s) ;
`waitReady` ; `expect` `pageIndicator.text` `2 / 2`, `transition.played` `fade` ; `page.effect` `slide-left` sur la
page 2, `page.previous` : `transition.played` `slide-left` ; capture `glissement`. Décrire : deux pages mêlées à
moitié, puis l'ancienne page qui sort par la gauche.

**Interdits.** Aucune image ni libellé de l'original dans les vignettes ; aperçus dessinés en QML.

**Terminé quand.** `browserListsEveryEffect` (un délégué par effet du catalogue), `tst_captures_effets` Passed ;
`NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.** `browserListsEveryEffect` et `browserChoiceSetsTheGlobalEffect` dans
  `tst_ui_presentation.cpp` (ouvrir par `view.page-effects`, compter les délégués d'objectName `effect-<id>`,
  cliquer le deuxième, `Terminé` : réglage global égal à son identifiant).
- [ ] **Étape 2 : navigateur.** `ui/qml/TransitionBrowser.qml` (`FloatingPanel` ; `GridView` sur
  `TransitionController.catalogue` (liste `{id, label}` exposée par le contrôleur) ; vignette = deux rectangles de
  couleurs à nous animés par le même composant que `TransitionLayer`) ; gestionnaire `view.page-effects` dans
  `Presentation.qml` ; `ui/presentation.cmake`, `ui/icons/effets.svg`.
- [ ] **Étape 3 : scénario.** `bench/scenarios/effets.json` et `tst_captures_effets` dans `tests/presentation.cmake`.
- [ ] **Étape 4 : mesure des effets (préparation de S22).** Rien à écrire ici : la procédure de mesure de S22 est
  dans la section « Clôture » du plan 7b (banc plein écran, un passage par effet avec `view.page-effect <id>` puis
  `page.next` répété) ; cette tâche fournit le scénario `effets`, qui sert aussi de témoin à l'écran.
- [ ] **Étape 5 : preuves et commit.** Preuve de tâche, banc sans écran, preuve visuelle, `NOMS OK` ; commit « Lot 7 :
  navigateur d'effets de changement de page et scénario (F-640, S22) (L7-T8) ».

<a id="tache-l7-t9"></a>
### Tâche L7-T9 — Règle (F-280) et guides de tracé

**Objectif.** `math.ruler` pose une règle sur la page (aide, pas contenu) : glisser le centre la déplace, glisser une
extrémité l'allonge ou la raccourcit, glisser le bord haut ou bas la tourne (angle affiché en degrés entiers) ; menu à
nous (unités des deux bords : cm, mm, pouces, aucune ; afficher l'angle ; fermer), réglages gardés (`QSettings`
`mathtools/ruler`). Avec le stylo, un appui à moins de 16 pixels d'écran d'un bord accroche le trait : tout le trait
suit ce bord en ligne droite jusqu'au relâché ; un petit cercle montre le point d'accroche. Le trait est versé par le
stylo, comme tout trait.

**Fichiers.** Voir la table (ligne L7-T9).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_mathtools|tst_ui_presentation|tst_captures_regle'`,
preuve de tâche (N ≥ N0 + 12), banc sans écran, preuve visuelle `-Scenario regle -Lire "mathTools.count=1"`.

**Preuve visuelle.** `bench/scenarios/regle.json` : `math.ruler` ; `expect` `mathTools.count` 1 ; `stroke` de
(300, 402) à (700, 395) (presque le long du bord haut posé à y = 400 par défaut, voir étape 2) ; `waitReady` ;
`checkPixel` encre à (500, 400) et page à (500, 395) ; capture `regle-trait`. Décrire : règle graduée, trait droit
collé à son bord.

**Interdits.** Ne pas modifier `tools/pentool.*`, ni la vue (le filtre est posé par L7-T2). La règle n'est jamais
écrite dans le document ni dans les miniatures (`mathToolIsNotSaved` : empreinte inchangée après ouverture,
déplacement et fermeture de la règle). Pas de dépendance à la sélection.

**Terminé quand.** Tests verts, dont `rulerSnapsNearEdge`, `rulerIgnoresFarSamples`, `strokeStaysOnTheEdgeOnceSnapped`,
`rulerGuidesThePenWithoutChangingIt` ; captures conformes ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge des guides.** `tests/tst_features_mathtools.cpp` :
  ```cpp
  #include <actcraft/features/mathtools.h>
  #include <actcraft/features/strokeguide.h>

  #include <QtTest>

  using namespace actcraft;
  using namespace actcraft::features;

  namespace {
  input::Sample at(double x, double y, input::Phase phase)
  {
      input::Sample s;
      s.x = x;
      s.y = y;
      s.phase = phase;
      s.device = input::DeviceKind::Stylus;
      return s;
  }
  } // namespace

  class TstFeaturesMathTools : public QObject
  {
      Q_OBJECT
  private slots:
      void rulerSnapsNearEdge()
      {
          Ruler r;                                     // bord haut : de (200, 400) à (800, 400), voir rulerEdges()
          r.setFrame(QPointF(500, 440), 600, 80, 0);   // centre, longueur, largeur, angle en degrés
          const auto snapped = r.capture(at(300, 405, input::Phase::Press), 16);
          QVERIFY(snapped.has_value());
          QCOMPARE(snapped->y, 400.0);
          QCOMPARE(snapped->x, 300.0);
      }
      void rulerIgnoresFarSamples()
      {
          Ruler r;
          r.setFrame(QPointF(500, 440), 600, 80, 0);
          QVERIFY(!r.capture(at(300, 300, input::Phase::Press), 16).has_value());
      }
      void strokeStaysOnTheEdgeOnceSnapped()
      {
          Ruler r;
          r.setFrame(QPointF(500, 440), 600, 80, 0);
          GuideSet guides;
          guides.add(&r);
          guides.setTolerance(16);
          QCOMPARE(guides.filter(at(300, 405, input::Phase::Press)).y, 400.0);
          QCOMPARE(guides.filter(at(500, 360, input::Phase::Move)).y, 400.0); // loin du bord : reste dessus
          QCOMPARE(guides.filter(at(900, 420, input::Phase::Move)).x, 800.0); // borné à la longueur
          QCOMPARE(guides.filter(at(600, 380, input::Phase::Release)).y, 400.0);
          QCOMPARE(guides.filter(at(300, 300, input::Phase::Press)).y, 300.0); // nouvel appui loin : libre
      }
      void rotatedRulerProjects()
      {
          Ruler r;
          r.setFrame(QPointF(500, 500), 400, 80, 90);  // verticale : bords à x = 460 et x = 540
          const auto s = r.capture(at(465, 520, input::Phase::Press), 16);
          QVERIFY(s.has_value());
          QCOMPARE(qRound(s->x), 460);
      }
      void angleDisplayedInWholeDegrees()
      {
          Ruler r;
          r.setFrame(QPointF(0, 0), 400, 80, 359.6);
          QCOMPARE(r.displayAngle(), 0);
      }
      void centimetreScale()
      {
          QCOMPARE(kPixelsPerCentimetre, 96.0 / 2.54);
      }
  };

  QTEST_GUILESS_MAIN(TstFeaturesMathTools)
  #include "tst_features_mathtools.moc"
  ```
  Déclarer `tst_features_mathtools` (`LIBS ActCraft::features`).
- [ ] **Étape 2 : guides et règle.** `features/include/actcraft/features/strokeguide.h` :
  ```cpp
  #pragma once

  #include <actcraft/input/sample.h>

  #include <optional>
  #include <vector>

  namespace actcraft::features {

  // Un bord ou un arc qui guide l'encre (outils de maths, F-280 à F-283). Coordonnées de page.
  class StrokeGuide
  {
  public:
      virtual ~StrokeGuide() = default;
      // Échantillon projeté sur le guide si `sample` en est à moins de `tolerance` (pixels de page) ; sinon rien.
      virtual std::optional<input::Sample> capture(const input::Sample& sample, double tolerance) = 0;
      // Échantillon projeté sur le guide saisi au dernier capture() réussi (borné à son étendue).
      virtual input::Sample follow(const input::Sample& sample) = 0;
  };

  // Ensemble des guides ouverts : à l'appui, le plus proche saisit le trait ; il le garde jusqu'au relâché.
  class GuideSet
  {
  public:
      void add(StrokeGuide* guide);
      void remove(StrokeGuide* guide);
      void setTolerance(double pagePixels) noexcept; // 16 pixels d'écran ÷ zoom, tenu à jour par le contrôleur
      input::Sample filter(const input::Sample& sample);
      bool guiding() const noexcept;
      std::optional<input::Sample> snapPreview(const input::Sample& hover); // cercle d'accroche au survol

  private:
      std::vector<StrokeGuide*> m_guides;
      StrokeGuide* m_active = nullptr;
      double m_tolerance = 16;
  };

  } // namespace actcraft::features
  ```
  `features/src/strokeguide.cpp` : à `Press`, essayer `capture` sur chaque guide et garder le plus proche (distance
  entre l'échantillon d'origine et sa projection) ; à `Move`, `follow` du guide actif ; à `Release`, `follow` puis
  plus de guide actif. `features/include/actcraft/features/mathtools.h` : `inline constexpr double
  kPixelsPerCentimetre = 96.0 / 2.54;`, `kPixelsPerInch = 96.0`, `class Ruler final : public StrokeGuide`
  (`setFrame(QPointF center, double length, double width, double angleDegrees)`, `edges()` : deux segments, haut et
  bas du rectangle tourné ; `capture` projette sur le segment le plus proche ; `follow` projette sur le segment saisi,
  borné ; `displayAngle()` = `qRound(angle) % 360` ramené dans 0..359 ; unités par bord : `enum class RulerUnit { Cm,
  Mm, Inch, None }`). `features/src/mathtools.cpp`. `features/CMakeLists.txt`. Défaut d'ouverture : centre de la
  page, longueur 600, largeur 80, angle 0.
- [ ] **Étape 3 : contrôleur.** `ui/src/mathtoolscontroller.h/.cpp` (`MathToolsController` : propriétés `view`
  (`PageView*`), `count`, liste `tools` (modèle QML : type, centre, longueur, largeur, angle, réglages) ;
  `Q_INVOKABLE int openRuler()`, `close(int)`, `setFrame(int, QPointF, qreal, qreal, qreal)` ; à la construction,
  `view->setSampleFilter([this](const input::Sample& s) { return m_guides.filter(s); })` ; tolérance mise à jour au
  changement de zoom ; filtre retiré quand plus aucun outil n'est ouvert). Les outils sont **par fenêtre**, pas par
  page : ils restent en changeant de page (comme la référence Python ; L7-T0 le confirme).
- [ ] **Étape 4 : surcouche.** `ui/qml/MathToolsLayer.qml` (dans `pageArea.pageOverlays`, un `Repeater` sur
  `controller.tools` qui choisit le délégué par type) et `ui/qml/RulerTool.qml` (corps translucide à nous, graduations
  en cm ou pouces dessinées par un `Shape` ou un `Canvas` **mis en cache** (`renderStrategy: Canvas.Cooperative`,
  redessiné seulement quand la longueur ou l'unité change), trois zones de saisie : centre (déplacer), extrémités
  (allonger), bords (tourner) ; étiquette d'angle ; bouton de menu ; cercle d'accroche lié à `snapPreview`).
  Les zones de saisie de la règle n'acceptent que les appuis sur le corps, jamais sur la bande de 16 pixels des bords
  côté extérieur (le stylo doit pouvoir s'y poser). `Presentation.qml` : `MathToolsController { id: mathTools;
  objectName: "mathTools"; view: presentation.pageArea.view }` et `MathToolsLayer { controller: mathTools; commands:
  presentation.commands }`. **`MathToolsLayer.qml` reçoit `property Commands commands` et pose lui-même les
  gestionnaires `math.*`** (ici `math.ruler`) : L7-T10 à L7-T13 n'ont pas à toucher `Presentation.qml`.
  `ui/presentation.cmake`, `ui/icons/regle.svg`.
- [ ] **Étape 5 : tests d'interface et scénario.** `rulerGuidesThePenWithoutChangingIt` (trait injecté presque le
  long du bord : tous les points du trait versé ont y = 400 ± 0,5 ; la règle fermée, le même trait garde ses y
  d'origine) et `mathToolIsNotSaved` dans `tst_ui_presentation.cpp` ; `bench/scenarios/regle.json`,
  `tst_captures_regle` dans `tests/presentation.cmake`.
- [ ] **Étape 6 : preuves et commit.** Preuve de tâche, banc sans écran, preuve visuelle, `NOMS OK` ; commit « Lot 7 :
  règle et guides de tracé (F-280) (L7-T9) ».

<a id="tache-l7-t10"></a>
### Tâche L7-T10 — Équerre (F-283)

**Objectif.** `math.set-square` : triangle rectangle isocèle (90°, 45°, 45°) déplaçable, tournable, redimensionnable
comme la règle ; ses trois bords guident le stylo ; angle affiché ; graduations sur les côtés de l'angle droit.

**Fichiers.** Voir la table (ligne L7-T10).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_mathtools|tst_ui_presentation'`, preuve de tâche (N ≥ N0
+ 12), banc sans écran.

**Interdits.** Comme L7-T9.

**Terminé quand.** `setSquareHasThreeEdges`, `setSquareHypotenuseAt45`, `setSquareGuidesThePen` verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.**
  ```cpp
  void setSquareHasThreeEdges()
  {
      SetSquare q;
      q.setFrame(QPointF(400, 400), 300, 0); // sommet de l'angle droit, longueur des côtés, angle
      QCOMPARE(q.edges().size(), std::size_t{3});
  }
  void setSquareHypotenuseAt45()
  {
      SetSquare q;
      q.setFrame(QPointF(400, 400), 300, 0);
      const auto s = q.capture(at(552, 552, input::Phase::Press), 16); // près de l'hypoténuse (700,400)-(400,700)
      QVERIFY(s.has_value());
      QCOMPARE(qRound(s->x + s->y), 1100);
  }
  ```
  (côtés de l'angle droit vers +x et +y depuis le sommet, à l'angle 0.)
- [ ] **Étape 2 : logique.** `class SetSquare final : public StrokeGuide` dans `mathtools.h/.cpp` (même projection
  par segment que `Ruler`, factorisée dans une fonction libre `projectOnSegments` de `mathtools.cpp`).
- [ ] **Étape 3 : contrôleur et délégué.** `openSetSquare()` dans `MathToolsController` ; `ui/qml/SetSquareTool.qml` ;
  délégué choisi dans `MathToolsLayer.qml`, qui pose aussi le gestionnaire `math.set-square` (règle de L7-T9 :
  `Presentation.qml` n'est pas touché par T10 à T13). `ui/presentation.cmake`, `ui/icons/equerre.svg`.
- [ ] **Étape 4 : test d'interface.** `setSquareGuidesThePen` dans `tst_ui_presentation.cpp`.
- [ ] **Étape 5 : preuves et commit.** Commit « Lot 7 : équerre (F-283) (L7-T10) ».

<a id="tache-l7-t11"></a>
### Tâche L7-T11 — Rapporteur (F-281)

**Objectif.** `math.protractor` : rapporteur complet (360°) ou demi (180°), positionnable, redimensionnable ; menu à
nous : complet, demi, arc, arc fermé, arc plein, afficher l'angle, fermer. Avec le stylo près du bord : le curseur
s'accroche et le trait devient un arc parfait de ce cercle, du style choisi (arc seul ; arc fermé = arc + deux rayons ;
arc plein = secteur rempli, versé comme une forme `ShapeKind::Path` remplie de la couleur du stylo) ; l'angle balayé
se lit sur l'échelle.

**Fichiers.** Voir la table (ligne L7-T11).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_mathtools|tst_ui_presentation'`, preuve de tâche (N ≥ N0
+ 12), banc sans écran.

**Interdits.** Comme L7-T9. L'arc plein et l'arc fermé passent par `DocumentController::insertObject` (une étape),
jamais par le stylo modifié.

**Terminé quand.** `protractorArcFollowsTheRim`, `closedArcAddsTwoRadii`, `halfProtractorStopsAt180`,
`filledArcBecomesOneShape` verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.**
  ```cpp
  void protractorArcFollowsTheRim()
  {
      Protractor p;
      p.setFrame(QPointF(500, 500), 200, 0, Protractor::Kind::Full); // centre, rayon, angle, forme
      const auto s = p.capture(at(705, 500, input::Phase::Press), 16);
      QVERIFY(s.has_value());
      QCOMPARE(qRound(std::hypot(s->x - 500, s->y - 500)), 200);
      const auto f = p.follow(at(500, 300, input::Phase::Move));
      QCOMPARE(qRound(std::hypot(f.x - 500, f.y - 500)), 200);
      QCOMPARE(qRound(p.sweptDegrees()), 90);
  }
  void halfProtractorStopsAt180()
  {
      Protractor p;
      p.setFrame(QPointF(500, 500), 200, 0, Protractor::Kind::Half);  // demi-disque au-dessus du diamètre
      QVERIFY(!p.capture(at(500, 705, input::Phase::Press), 16).has_value()); // sous le diamètre : rien
  }
  void closedArcAddsTwoRadii()
  {
      const auto path = arcPath(QPointF(0, 0), 100, 0, 90, ArcStyle::Closed);
      QCOMPARE(path.front(), QPointF(0, 0));
      QCOMPARE(path.back(), QPointF(0, 0));
  }
  ```
- [ ] **Étape 2 : logique.** `class Protractor final : public StrokeGuide` (`Kind { Full, Half }`, `sweptDegrees()`
  cumulé sans saut à ±180°), `enum class ArcStyle { Arc, Closed, Filled }`, `std::vector<QPointF> arcPath(center,
  radius, startDegrees, sweepDegrees, style)` (pas de 2°). `mathtools.h/.cpp`.
- [ ] **Étape 3 : contrôleur et délégué.** `openProtractor()`, réglage du style ; au relâché d'un trait guidé par le
  rapporteur en style fermé ou plein, le contrôleur ajoute l'objet par `insertObject` (fermé : `core::Line` des deux
  rayons ; plein : `core::Shape` `Path` rempli) ; `ui/qml/ProtractorTool.qml` ; `MathToolsLayer.qml` ;
  `ui/presentation.cmake` ; `ui/icons/rapporteur.svg`.
- [ ] **Étape 4 : test d'interface.** `filledArcBecomesOneShape` (style plein : un trait guidé + une forme, ou la forme
  seule si L7-T0 établit que l'original remplace le trait — trancher sur le relevé) dans `tst_ui_presentation.cpp`.
- [ ] **Étape 5 : preuves et commit.** Commit « Lot 7 : rapporteur (F-281) (L7-T11) ».

<a id="tache-l7-t12"></a>
### Tâche L7-T12 — Compas (F-282)

**Objectif.** `math.compass` : pointe (centre), bras gauche (déplacer), bras droit (tourner sans tracer), porte-mine
(changer le rayon), charnière (masquer). Stylo près de la mine : le trait devient un arc ou un cercle de centre la
pointe et de rayon le rayon courant, couleur et épaisseur du stylo. Menu à nous : réfléchi (miroir), afficher
l'angle, afficher le rayon (cm), marquer le centre (petite croix versée comme `core::Line`), augmenter, réduire,
réinitialiser la longueur, fermer. La gomme près de la mine attend le lot 3 (`L7-T34`).

**Fichiers.** Voir la table (ligne L7-T12).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_mathtools|tst_ui_presentation'`, preuve de tâche (N ≥ N0
+ 12), banc sans écran.

**Interdits.** Comme L7-T9.

**Terminé quand.** `compassDrawsAnArcAtItsRadius`, `fullTurnClosesTheCircle`, `radiusInCentimetres`,
`markCentreAddsACross` verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.**
  ```cpp
  void compassDrawsAnArcAtItsRadius()
  {
      Compass c;
      c.setPivot(QPointF(400, 400));
      c.setRadius(150);
      c.setAngle(0);                                  // mine à (550, 400)
      QVERIFY(c.capture(at(553, 402, input::Phase::Press), 16).has_value());
      const auto f = c.follow(at(400, 200, input::Phase::Move));
      QCOMPARE(qRound(std::hypot(f.x - 400, f.y - 400)), 150);
  }
  void fullTurnClosesTheCircle()
  {
      Compass c;
      c.setPivot(QPointF(0, 0));
      c.setRadius(100);
      c.capture(at(100, 0, input::Phase::Press), 16);
      for (int d = 10; d <= 400; d += 10)
          c.follow(at(100 * std::cos(qDegreesToRadians(double(d))), 100 * std::sin(qDegreesToRadians(double(d))), input::Phase::Move));
      QVERIFY(c.sweptDegrees() >= 360);
      QVERIFY(c.closedCircle());
  }
  void radiusInCentimetres()
  {
      Compass c;
      c.setRadius(kPixelsPerCentimetre * 3);
      QCOMPARE(c.radiusText(), QStringLiteral("3,0 cm"));
  }
  ```
- [ ] **Étape 2 : logique.** `class Compass final : public StrokeGuide` (`setPivot`, `setRadius` (borné 20 à 2000),
  `setAngle`, `mirrored`, `sweptDegrees`, `closedCircle()` (balayage ≥ 360° : le trait est fermé exactement sur son
  premier point), `radiusText()` (une décimale, virgule)). `mathtools.h/.cpp`.
- [ ] **Étape 3 : contrôleur et délégué.** `openCompass()` ; « marquer le centre » par `insertObject` (deux
  `core::Line` en croix de 12 pixels, ou une ligne à quatre points) ; `ui/qml/CompassTool.qml` (dessin à nous) ;
  `MathToolsLayer.qml` ; `ui/presentation.cmake` ; `ui/icons/compas.svg`.
- [ ] **Étape 4 : test d'interface.** `markCentreAddsACross`, `compassStrokeIsACircle` dans `tst_ui_presentation.cpp`.
- [ ] **Étape 5 : preuves et commit.** Commit « Lot 7 : compas (F-282) (L7-T12) ».

<a id="tache-l7-t13"></a>
### Tâche L7-T13 — Origine XY (F-284)

**Objectif.** `math.xy-origin` (cochable) affiche un point d'origine déplaçable sur la page ; sa dernière position est
mémorisée **par page** (`PagePresentation::hasOrigin`, `origin`, L7-T1, une étape d'annulation au relâché) et
enregistrée avec le document ; recliquer la commande le retire de l'écran (la position reste mémorisée). Le pivot de
rotation des objets sélectionnés autour de ce point attend le lot 3 (`L7-T34`).

**Fichiers.** Voir la table (ligne L7-T13).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_mathtools|tst_ui_presentation'`, preuve de tâche (N ≥ N0
+ 12).

**Interdits.** Comme L7-T9 ; pas de lien à la sélection ni à la poignée de rotation (lot 3).

**Terminé quand.** `originRememberedPerPage`, `originSavedWithThePage` verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.** `originRememberedPerPage` (`OriginMemory` : `set(page 0, (100, 100))`, `set(page 1,
  (300, 200))`, `at(0)` → (100, 100), `at(2)` → défaut = centre de la page) dans `tst_features_mathtools.cpp` ;
  `originSavedWithThePage` dans `tst_ui_presentation.cpp` (déplacer l'origine sur la page 1, enregistrer en
  `.actcraft` sous `.tmp`, rouvrir : `presentation.origin` égal ; annuler le déplacement : origine précédente).
- [ ] **Étape 2 : logique.** `struct OriginMemory` dans `mathtools.h/.cpp` (lecture depuis `core::PagePresentation`,
  défaut = centre).
- [ ] **Étape 3 : contrôleur et délégué.** `MathToolsController::originVisible`, `origin` (suivi de la page
  affichée), `moveOrigin(QPointF)` → `DocumentController::setPagePresentation` au relâché ; `ui/qml/XyOriginTool.qml`
  (croix et cercle à nous) ; gestionnaire `math.xy-origin` (bascule + `setChecked`) dans `MathToolsLayer.qml` ;
  `ui/presentation.cmake` ; `ui/icons/origine.svg`.
- [ ] **Étape 4 : preuves et commit.** Commit « Lot 7 : origine XY mémorisée par page (F-284) (L7-T13) ».

<a id="tache-l7-t14"></a>
### Tâche L7-T14 — Dés (F-285, E5)

**Objectif.** `tool.dice` ouvre un panneau flottant : nombre de dés (1 à 5 ; nombre à l'ouverture = valeur
tranchée par L7-T0 pour E5, à défaut 5 comme la spécification), vitesse (3 à 11 s, défaut 7), bouton « Lancer »
(animation des faces, puis valeurs finales), « Écrire sur la page » pose le total en texte sur la page affichée.

**Fichiers.** Voir la table (ligne L7-T14).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_dice|tst_ui_presentation'`, preuve de tâche (N ≥ N0 +
13).

**Interdits.** Dés dessinés par nous (aucune image ni couleur imposée par l'original ; couleur à nous) ; aucun
libellé copié de la boîte d'origine.

**Terminé quand.** `seededRollIsReproducible`, `countClampedTo1To5`, `totalIsTheSum`, `speedClampedTo3To11`,
`diceTotalWrittenOnThePage` verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.**
  ```cpp
  void seededRollIsReproducible()
  {
      Dice a(42), b(42);
      a.setCount(5);
      b.setCount(5);
      QCOMPARE(a.roll(), b.roll());
  }
  void countClampedTo1To5()
  {
      Dice d(1);
      d.setCount(0);
      QCOMPARE(d.count(), 1);
      d.setCount(9);
      QCOMPARE(d.count(), 5);
  }
  void totalIsTheSum()
  {
      Dice d(7);
      d.setCount(3);
      const auto faces = d.roll();
      QCOMPARE(faces.size(), std::size_t{3});
      int sum = 0;
      for (int f : faces) { QVERIFY(f >= 1 && f <= 6); sum += f; }
      QCOMPARE(d.total(), sum);
  }
  void speedClampedTo3To11()
  {
      QCOMPARE(Dice::animationMs(1), 3000);
      QCOMPARE(Dice::animationMs(7), 7000);
      QCOMPARE(Dice::animationMs(20), 11000);
  }
  ```
  Déclarer `tst_features_dice`.
- [ ] **Étape 2 : logique.** `features/include/actcraft/features/dice.h`, `features/src/dice.cpp` : `class Dice`
  (`explicit Dice(quint32 seed)` sur `QRandomGenerator`, `setCount`, `count`, `std::vector<int> roll()`, `total()`,
  `static int animationMs(int seconds)` borné 3 à 11 s ; `kDefaultCount` = valeur de E5). `features/CMakeLists.txt`.
- [ ] **Étape 3 : contrôleur et panneau.** `ui/src/dicecontroller.h/.cpp` (`DiceController` : `count`, `speed`,
  `faces` (liste), `rolling`, `roll()` (graine de `QRandomGenerator::global()` en usage, fixée par une propriété
  `seed` dans les tests), `writeTotal()` → `documentController.insertText(QString::number(total), rectangle près du
  centre de la vue)`), `ui/qml/DicePanel.qml` (`FloatingPanel` ; faces dessinées en QML ; animation par `Timer` à
  100 ms pendant la durée choisie — panneau hors de la page, sans effet sur le cache) ; gestionnaire `tool.dice` dans
  `Presentation.qml` ; `ui/presentation.cmake` ; `ui/icons/des.svg`.
- [ ] **Étape 4 : test d'interface.** `diceTotalWrittenOnThePage` (graine fixée, roll, écrire : un `Text` dont le HTML
  contient le total ; une étape d'annulation).
- [ ] **Étape 5 : preuves et commit.** Commit « Lot 7 : dés (F-285) (L7-T14) ».

<a id="tache-l7-t15"></a>
### Tâche L7-T15 — Calculatrice (F-286)

**Objectif.** `tool.calculator` ouvre **notre** calculatrice (panneau flottant : chiffres, quatre opérations, %,
±, virgule, effacer, mémoire M+ M− MR MC, égal) ; réglage « calculatrice » (`QSettings` `tools/calculator`) :
« ActCraft » (défaut), « calculatrice du système » ou « programme choisi » — ces deux derniers sous Windows seulement
(lancement par `QProcess::startDetached`, `calc.exe` pour celle du système), cachés ailleurs.

**Fichiers.** Voir la table (ligne L7-T15).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_calculator|tst_ui_presentation'`, preuve de tâche (N ≥ N0
+ 14).

**Interdits.** Aucun programme lancé par les tests (le lancement passe par une fonction injectable, remplacée par un
faux dans `tst_ui_presentation`).

**Terminé quand.** Tests nommés verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.**
  ```cpp
  void operatorPrecedenceLeftToRight() // calculatrice de poche : immédiat, de gauche à droite
  {
      Calculator c;
      for (const char* k : {"2", "+", "3", "*", "4", "="}) c.press(QString::fromLatin1(k));
      QCOMPARE(c.display(), QStringLiteral("20"));
  }
  void decimalComma()
  {
      Calculator c;
      for (const char* k : {"1", ",", "5", "+", "1", "="}) c.press(QString::fromLatin1(k));
      QCOMPARE(c.display(), QStringLiteral("2,5"));
  }
  void divisionByZeroIsAnError()
  {
      Calculator c;
      for (const char* k : {"5", "/", "0", "="}) c.press(QString::fromLatin1(k));
      QCOMPARE(c.display(), QStringLiteral("Erreur"));
      c.press(QStringLiteral("C"));
      QCOMPARE(c.display(), QStringLiteral("0"));
  }
  void percentAndSign()
  {
      Calculator c;
      for (const char* k : {"2", "0", "0", "*", "5", "%"}) c.press(QString::fromLatin1(k));
      QCOMPARE(c.display(), QStringLiteral("10"));
      c.press(QStringLiteral("±"));
      QCOMPARE(c.display(), QStringLiteral("-10"));
  }
  void memoryKeys()
  {
      Calculator c;
      for (const char* k : {"7", "M+", "C", "MR"}) c.press(QString::fromUtf8(k));
      QCOMPARE(c.display(), QStringLiteral("7"));
  }
  ```
  Déclarer `tst_features_calculator`.
- [ ] **Étape 2 : logique.** `features/include/actcraft/features/calculator.h`, `features/src/calculator.cpp` :
  `class Calculator` (machine à états de calculatrice de poche, `double`, affichage à 12 chiffres significatifs au
  plus, virgule française, « Erreur » sur division par zéro ou résultat non fini). `features/CMakeLists.txt`.
- [ ] **Étape 3 : contrôleur et panneau.** `ui/src/calculatorcontroller.h/.cpp` (`CalculatorController` : `display`,
  `press(QString)`, `mode` (0 nôtre, 1 système, 2 programme), `programPath`, `open()` qui montre le panneau ou lance
  le programme par le lanceur injectable) ; `ui/qml/CalculatorPanel.qml` (`FloatingPanel`, grille de touches ; le
  clavier physique tape aussi) ; gestionnaire `tool.calculator` ; `ui/presentation.cmake` ;
  `ui/icons/calculatrice.svg`.
- [ ] **Étape 4 : test d'interface.** `systemCalculatorLaunchedOnWindows` (faux lanceur : appelé avec `calc.exe` en
  mode 1 sous Windows ; sous une autre plateforme le mode 1 retombe sur le nôtre).
- [ ] **Étape 5 : preuves et commit.** Commit « Lot 7 : calculatrice (F-286) (L7-T15) ».

<a id="tache-l7-t16"></a>
### Tâche L7-T16 — Horloge et compte à rebours (F-290, E9)

**Objectif.** `tool.clock` (Ctrl+Shift+U) ouvre un panneau redimensionnable (les coins gardent les proportions) :
horloge analogique, numérique ou les deux, pause, minuterie (compte en avant), compte à rebours ; barre des boutons
affichable ou masquable. Le réglage du compte à rebours : durée `hh:mm:ss`, répéter, réinitialiser, action de fin
(liste tranchée par L7-T0 pour E9 ; à défaut celle de la spécification : ne rien faire, page suivante, page
précédente ; « rétablir la page » s'y ajoute en L7-T34, après la fusion du lot 3 qui livre cette commande). Répéter + page suivante
ajoute des pages vierges jusqu'à fermeture (le `page.next` existant crée une page en fin). Son de fin : « aucun »
seulement (lot 8, H7-4).

**Fichiers.** Voir la table (ligne L7-T16).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_countdown|tst_ui_presentation|tst_captures_compte_a_rebours'`,
preuve de tâche (N ≥ N0 + 16), preuve visuelle `-Scenario compte-a-rebours -Lire "pageIndicator.text=2 / 2"`.

**Preuve visuelle.** `bench/scenarios/compte-a-rebours.json` : `tool.clock` ; ouverture du réglage du compte à
rebours et saisie de `00:00:03` par touches injectées dans le champ d'objectName `countdownField` (aucune commande
ajoutée hors de L7-T1), action de fin « page suivante » choisie au clavier, validation ; `waitFor`
`pageIndicator.text` `2 / 2` avec `timeoutMs` 6000 ; capture `horloge`. Décrire : horloge analogique et numérique
côte à côte, compte à rebours à 00:00:00, page 2 affichée.

**Interdits.** Le panneau ne redessine pas le cache ; son aiguille tourne par un `RotationAnimator` ou une rotation
liée à une minuterie à 1 s (pas plus fréquente) ; aucune nouvelle commande.

**Terminé quand.** `endTriggersActionOnce`, `repeatRestarts`, `pauseFreezes`, `formatHms`, `tst_captures_compte_a_rebours`
verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.**
  ```cpp
  void endTriggersActionOnce()
  {
      Countdown c;
      c.start(3, 0);                       // 3 s, à t = 0 ns
      QCOMPARE(c.remainingSeconds(2'000'000'000), 1);
      QVERIFY(!c.poll(2'900'000'000));     // pas encore
      QVERIFY(c.poll(3'000'000'000));      // une fin
      QVERIFY(!c.poll(3'500'000'000));     // pas deux
  }
  void repeatRestarts()
  {
      Countdown c;
      c.setRepeat(true);
      c.start(2, 0);
      QVERIFY(c.poll(2'000'000'000));
      QCOMPARE(c.remainingSeconds(2'500'000'000), 2); // reparti à 2 s (arrondi au supérieur)
      QVERIFY(c.poll(4'000'000'000));
  }
  void pauseFreezes()
  {
      Countdown c;
      c.start(10, 0);
      c.pause(2'000'000'000);
      QCOMPARE(c.remainingSeconds(9'000'000'000), 8);
      c.resume(9'000'000'000);
      QCOMPARE(c.remainingSeconds(10'000'000'000), 7);
  }
  void formatHms()
  {
      QCOMPARE(formatHms(3725), QStringLiteral("01:02:05"));
      QCOMPARE(parseHms(QStringLiteral("00:00:03")), 3);
      QCOMPARE(parseHms(QStringLiteral("xx")), -1);
  }
  void stopwatchCountsUp()
  {
      Stopwatch w;
      w.start(0);
      QCOMPARE(w.elapsedSeconds(65'000'000'000), 65);
  }
  ```
  Déclarer `tst_features_countdown`.
- [ ] **Étape 2 : logique.** `features/include/actcraft/features/countdown.h`, `features/src/countdown.cpp` :
  `Countdown` (`start(seconds, nowNs)`, `pause`, `resume`, `reset`, `setRepeat`, `poll(nowNs)` vrai une fois par fin,
  `remainingSeconds(nowNs)` arrondi au supérieur), `Stopwatch`, `formatHms`, `parseHms`. Temps fourni par l'appelant
  (`core::monotonicNowNs()` en usage). `features/CMakeLists.txt`.
- [ ] **Étape 3 : contrôleur et panneau.** `ui/src/clockcontroller.h/.cpp` (`ClockController` : `mode` (analogique,
  numérique, les deux), `paused`, `countdownSeconds`, `repeat`, `endAction` (identifiant de commande ou vide),
  `endActions` (liste `{commande, libellé}` ; L7-T34 y ajoutera « rétablir la page »), `remainingText`, une `QTimer` à 250 ms qui appelle `poll`
  et, à la fin, `commands.trigger(endAction)`) ; `ui/qml/ClockPanel.qml` (cadran à nous, chiffres, barre de boutons,
  coins proportionnels), `ui/qml/CountdownDialog.qml` (champ `countdownField`, répéter, réinitialiser, action de
  fin) ; gestionnaire `tool.clock` ; `ui/presentation.cmake` ; `ui/icons/horloge.svg`.
- [ ] **Étape 4 : tests d'interface et scénario.** `countdownEndGoesToNextPage` (3 s, `page.next` : la page 2
  existe et est affichée) et `repeatAddsBlankPages` (répéter : après deux fins, 3 pages) dans
  `tst_ui_presentation.cpp` (temps injecté au contrôleur par une propriété de test `clockNowOffsetMs`) ;
  `bench/scenarios/compte-a-rebours.json`, `tst_captures_compte_a_rebours`.
- [ ] **Étape 5 : preuves et commit.** Commit « Lot 7 : horloge, minuterie et compte à rebours (F-290) (L7-T16) ».

<a id="tache-l7-t17"></a>
### Tâche L7-T17 — Date et heure (F-300) et navigateur web (F-299)

**Objectif.** `insert.date-time` pose sur la page affichée la date et l'heure courantes en texte (format long
français : « samedi 18 octobre 2026, 14:05 », `QLocale(QLocale::French)`), couche intermédiaire, une étape.
`tool.web-browser` ouvre l'adresse d'accueil dans le navigateur du système (`QDesktopServices::openUrl`) ; adresse
d'accueil = page du projet par défaut (N2-A11), réglable (`QSettings` `tools/webHome`).

**Fichiers.** Voir la table (ligne L7-T17).

**Test et commande bornée.** Commande ciblée `-R tst_ui_presentation`, preuve de tâche (N ≥ N0 + 16).

**Interdits.** Aucun navigateur lancé par les tests (ouvreur d'URL injectable). Aucune adresse d'un service tiers par
défaut.

**Terminé quand.** `dateTimeInsertsOneText`, `webBrowserOpensTheHomeUrl` verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.** `dateTimeInsertsOneText` (horloge injectée au 2026-10-18 14:05 : un `Text` dont le HTML
  contient « 18 octobre 2026 » et « 14:05 » ; annuler le retire) ; `webBrowserOpensTheHomeUrl` (faux ouvreur reçoit
  l'URL de la page du projet, celle de `ui/src/appinfo.cpp` ; réglage changé : l'URL réglée).
- [ ] **Étape 2 : code.** `ui/src/presentationcommands.h/.cpp` (`PresentationCommands`, `QML_ELEMENT` : propriétés
  `documentController`, `nowOverride` (test), `urlOpener` (test) ; `Q_INVOKABLE void insertDateTime()`, `openWebHome()`)
  ; gestionnaires `insert.date-time` et `tool.web-browser` dans `Presentation.qml` ; `ui/presentation.cmake`.
- [ ] **Étape 3 : preuves et commit.** Commit « Lot 7 : date et heure (F-300), navigateur web (F-299) (L7-T17) ».

<a id="tache-l7-t18"></a>
### Tâche L7-T18 — Message défilant I (F-291) : modèle, surcouche, banc

**Objectif.** `tool.ticker` (Ctrl+Shift+T) fait aussitôt défiler un message par défaut à nous (« Votre message
ici ») en haut de la fenêtre ; position haut, centre ou bas ; un seul passage puis disparition, ou en boucle (arrêt
par clic droit sur le message) ; vitesse réglable ; pause et reprise. Le défilement tourne **sur le fil de rendu**
(`XAnimator`) : l'écriture au stylet pendant le défilement reste dans le budget (banc, S23).

**Fichiers.** Voir la table (ligne L7-T18).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_ticker|tst_ui_presentation'`, preuve de tâche (N ≥ N0 +
17), banc sans écran **avec un message en boucle** (lancer `tool.ticker` par le scénario du banc n'existe pas encore :
lancer le banc normal et joindre son JSON ; la mesure S23 avec message se joue à la clôture, voir 7b).

**Interdits.** Aucun `Timer` QML qui déplace le texte ; pas de redessin du cache (`tickerNeverRefreshesTheCache`) ;
texte du message toujours dessiné par un `Text` en texte brut (`textFormat: Text.PlainText`) — le message est
saisi par l'utilisateur et chargé de fichiers : jamais de texte riche (règle de sécurité du fichier d'état).

**Terminé quand.** `singlePassEndsHidden`, `loopRestarts`, `speedMapsToPixelsPerSecond`, `positionsAreThree`,
`tickerNeverRefreshesTheCache` verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.**
  ```cpp
  void speedMapsToPixelsPerSecond()
  {
      QCOMPARE(TickerSettings::pixelsPerSecond(1), 40.0);
      QCOMPARE(TickerSettings::pixelsPerSecond(10), 400.0);
      QCOMPARE(TickerSettings::pixelsPerSecond(99), 400.0);
  }
  void singlePassEndsHidden()
  {
      TickerRun run(1000, 200, 100.0, false);       // largeur d'écran, largeur du texte, px/s, boucle
      QCOMPARE(run.durationMs(), 12000);            // (1000 + 200) / 100 s
      QCOMPARE(run.xAt(0), 1000.0);
      QVERIFY(run.finishedAt(12000));
  }
  void loopRestarts()
  {
      TickerRun run(1000, 200, 100.0, true);
      QVERIFY(!run.finishedAt(50000));
      QCOMPARE(run.xAt(12000), 1000.0);
  }
  void positionsAreThree()
  {
      QCOMPARE(TickerSettings::positionFromText(QStringLiteral("bottom")), TickerSettings::Position::Bottom);
      QCOMPARE(TickerSettings::positionFromText(QStringLiteral("?")), TickerSettings::Position::Top);
  }
  ```
  Déclarer `tst_features_ticker`.
- [ ] **Étape 2 : logique.** `features/include/actcraft/features/ticker.h`, `features/src/ticker.cpp` :
  `struct TickerSettings` (texte, famille et taille de police, couleur, décalage et couleur de l'ombre (0 = sans
  ombre), fond : écran (transparent) ou couleur, position, boucle, vitesse 1 à 10 ; `pixelsPerSecond(speed)` = 40 ×
  vitesse borné à 1..10 ; `positionFromText`) et `TickerRun` (fonctions pures du temps).
- [ ] **Étape 3 : contrôleur et surcouche.** `ui/src/tickercontroller.h/.cpp` (`TickerController` : `settings`
  exposés en propriétés, `running`, `paused`, `start()`, `stop()`, `togglePause()`, `durationMs`) ;
  `ui/qml/TickerOverlay.qml` (bande pleine largeur dans `presentation.layer`, `Text` en texte brut + ombre par un
  second `Text` décalé, `XAnimator` de `from: width` à `to: -textWidth`, `loops: Animation.Infinite` en boucle ;
  `MouseArea` clic droit = arrêt ; la bande ne prend aucun autre événement : `acceptedButtons: Qt.RightButton`) ;
  gestionnaire `tool.ticker` (démarre le défilement ; l'ouverture de la fenêtre d'édition vient en L7-T19) ;
  `ui/presentation.cmake`.
- [ ] **Étape 4 : test d'interface.** `tickerNeverRefreshesTheCache` (spy à 0 pendant 1 s de défilement ; un trait au
  stylet pendant le défilement est versé normalement) ; `rightClickStopsTheLoop`.
- [ ] **Étape 5 : preuves et commit.** Preuve de tâche, banc sans écran, `NOMS OK` ; commit « Lot 7 : message
  défilant sur le fil de rendu (F-291) (L7-T18) ».

<a id="tache-l7-t19"></a>
### Tâche L7-T19 — Message défilant II (F-291, E11) : fenêtre d'édition, défaut, fichiers de messages

**Objectif.** Fenêtre d'édition du message : texte, police, couleur, ombre (décalage, couleur), fond (écran ou
couleur), position, boucle, vitesse, « Essayer » (joue sans garder), « Appliquer » (garde comme défaut, `QSettings`
`tools/ticker`), validation selon E11 (L7-T0 ; à défaut : « Appliquer » garde et ferme), pause et reprise ; menu
« enregistrer le message » et « ouvrir un message » dans un fichier JSON à nous (`actcraft-ticker/1`).

**Fichiers.** Voir la table (ligne L7-T19).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_ticker|tst_ui_presentation|tst_captures_message'`, preuve
de tâche (N ≥ N0 + 18), preuve visuelle `-Scenario message -Lire "ticker.running=true"`.

**Preuve visuelle.** `bench/scenarios/message.json` : `tool.ticker` ; `expect` `ticker.running` true ; `wait` 500 ;
capture `message-haut` ; réglage de la position en bas par la fenêtre (touches injectées) ; capture `message-bas`.
Décrire : bande de texte en haut, puis en bas de la fenêtre.

**Interdits.** Fichier de message lu avec une taille maximale (64 Kio) et un texte gardé en texte brut ; clés
inconnues ignorées sans échec.

**Terminé quand.** `messageFileRoundTrip`, `applyKeepsTheDefault`, `tst_captures_message` verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.** `messageFileRoundTrip` (écrire puis relire `TickerSettings` en `actcraft-ticker/1` :
  égal ; un fichier de 70 Kio est refusé ; un `"format"` inconnu est refusé) dans `tst_features_ticker.cpp` ;
  `applyKeepsTheDefault` dans `tst_ui_presentation.cpp` (appliquer un texte, recréer le contrôleur : texte gardé ;
  `QSettings` dans un dossier temporaire par `QStandardPaths::setTestModeEnabled(true)`).
- [ ] **Étape 2 : code.** `toJson` / `fromJson` dans `ticker.h/.cpp` ; `TickerController::apply()`, `test()`,
  `saveToUrl(QUrl)`, `openUrl(QUrl)` ; `ui/qml/TickerDialog.qml` (`FloatingPanel`, contrôles Qt Quick Controls
  `Basic`, dialogue de fichiers `QtQuick.Dialogs` qui ne fait que donner une URL, décision 22) ; `tool.ticker`
  ouvre désormais aussi la fenêtre (gestionnaire mis à jour dans `Presentation.qml`) ; `ui/presentation.cmake` ;
  `ui/icons/message.svg`.
- [ ] **Étape 3 : scénario.** `bench/scenarios/message.json`, `tst_captures_message` dans `tests/presentation.cmake`.
- [ ] **Étape 4 : preuves et commit.** Commit « Lot 7 : fenêtre du message défilant et fichiers de messages (F-291)
  (L7-T19) ».

## Fin du sous-lot 7a (orchestrateur, hors tâches)

Rejouer, sur la tête de `feat/livraison-complete-b` : suite debug (N ≥ N0 + 18, `TESTS OK`), `NOMS OK`, banc sans
écran, captures hors écran `premiers-pas`, `rideau`, `spot`, `regle`, `effets`, `compte-a-rebours`, `message`
(`CAPTURES OK`). Puis relecture du plan 7b (amendée par les constats de L7-T0 s'il y a lieu) et passage à 7b. Aucune
PR à la fin de 7a.
