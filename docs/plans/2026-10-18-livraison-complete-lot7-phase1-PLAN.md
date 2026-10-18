# Livraison complète — lot 7 « Présentation et outils » — amendement « phase 1 » (logique pure) — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**Ce que c'est.** Un amendement aux plans du lot 7 (`docs/plans/2026-10-18-livraison-complete-lot7-PLAN.md`, dit
« 7a », et `docs/plans/2026-10-18-livraison-complete-lot7b-PLAN.md`, dit « 7b »). Il extrait des tâches `L7-T*` la
**logique pure** de `features/`, testée par Qt Test sans écran, en tâches `L7-P<n>` exécutables **avant le socle
L0-S**. Il ne réécrit pas 7a ni 7b, à une exception près, décidée par l'orchestrateur : les identifiants d'effets du
scénario de L7-T8 (7a, « Test et commande bornée » et « Preuve visuelle ») suivent le catalogue de L7-P2. Pour le reste, la phase 2 (après la fusion du socle) les amendera en tenant compte de ce qui est
fait ici (section « Ce que la phase 2 reprend »).

**Autorités, dans l'ordre.** (1) `docs/sessions/livraison-complete-b-brief.md`, section « Phase 1 » ; (2) l'addendum 1
de l'orchestrateur A, `C:\Users\JEROME~1\AppData\Local\Temp\claude\C--Users-JeromeDenis-ActCraft\fd5ef06a-7b9b-46ba-a8c3-b06809fefd99\scratchpad\brief-b-addendum-1.md`,
qui complète le brief et fait autorité sur lui ; (3) ce plan ; (4) les sections `L7-T*` de 7a et 7b, pour tout ce que ce
plan ne redit pas. Le plan du socle qui fait autorité est la copie relue et mergée (9b13275) :
`C:\Users\JEROME~1\AppData\Local\Temp\claude\C--Users-JeromeDenis-pilotage\67662d88-b0d4-48e1-9ce6-e529d6829fab\scratchpad\socle-PLAN-1ffb07b.md`
(« socle 1ffb07b »), section « Ce que la session B doit savoir » ; le socle ne touche pas `features/`. Relecture adversariale de ce plan : imposée par le hook `relire-plan.js`, lancée par l'orchestrateur.

**Branche et worktrees.** Branche du chantier `feat/livraison-complete-b`
(`C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b`). Une tâche = un worktree isolé
`C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b--L7-P<n>`, branche `feat/livraison-complete-b--L7-P<n>`,
partie de `feat/livraison-complete-b` à jour des tâches déjà mergées. Merge `--no-ff` par l'orchestrateur, puis
`graphify update .` et commit de `graphify-out/`.

## Décisions de cet amendement

1. **Fichiers permis, et seulement eux** (brief, phase 1, point 2) : `features/include/actcraft/features/*.h`,
   `features/src/*.cpp`, `features/CMakeLists.txt`, `tests/tst_features_*.cpp`, un nouveau `tests/presentation.cmake`,
   et une ligne ajoutée **en dernière ligne** de `tests/CMakeLists.txt`, juste après
   `include(${CMAKE_CURRENT_SOURCE_DIR}/app.cmake)`, exactement :
   `include(${CMAKE_CURRENT_SOURCE_DIR}/presentation.cmake OPTIONAL)` (addendum 1, point 3 ; le socle écrit la même
   ligne au même endroit, **précédée d'un commentaire** : le conflit de `tests/CMakeLists.txt` est donc **attendu** à
   la fusion de la phase 2, et se résout en gardant le commentaire du socle et une seule ligne d'inclusion ; le brief
   impose ici la ligne seule). Ce plan lui-même est le seul autre fichier ajouté (par le planificateur).
2. **Fichiers interdits** : `core/`, `render/`, `formats/`, `ui/` (dont `ui/icons/*` et `ui/CMakeLists.txt`),
   `actions/`, `bench/`, `app/`, `input/`, `tools/`, tous les autres `CMakeLists.txt`, `tests/{core,input,tools,render,ui,bench,actions,formats,flipchart,app,verifications}.cmake`,
   tout test qui n'est pas `tests/tst_features_*.cpp`. Lire ces fichiers est permis.
3. **Types du socle jamais recréés** (addendum 1, point 1) : aucun identifiant `PagePresentation`, `CurtainMode`,
   `SpotlightShape`, `PageToolsMode`, `Equation`, `ReplaceObjectsCommand`, `SetPagePresentationCommand` n'est déclaré dans `features/`. La logique
   travaille sur des types neutres de Qt ou de la STL, ou sur des types propres à `features/` **aux noms distincts**
   (`Curtain::Start`, `Spotlight::Shape`, `PageToolChoice`, `PageToolsSettings`), reliés au modèle par la phase 2.
   Les types existants de `core/` (`core::ObjectId`) et d'`input/` (`input::Sample`) se lisent et s'utilisent.
4. **Dépendances CMake de `features/`, corrigées par L7-P0.** Aujourd'hui `features` dépend de `ActCraft::core`,
   `ActCraft::actions` et `ActCraft::render` ; `render` lie `Qt6::Quick` en `PUBLIC` : `features` dépend donc **déjà**
   de Qt Quick, par transitivité (constat ; 7a:28-29 l'ignorait, la relecture du plan l'a relevé en mineur). Aucune
   logique de `features/` n'utilise `render` (`git grep`). Il manque `ActCraft::input` (guides de tracé sur
   `input::Sample`, L7-T9). L7-P0 : `DEPENDS ActCraft::core ActCraft::actions ActCraft::input` (input lie `Qt6::Gui`,
   permis), `render` retiré, et un garde-fou à la configuration :
   `actcraft_forbid_links(actcraft_features Qt6::Quick Qt6::Qml Qt6::QuickControls2 Qt6::Widgets)`.
   Bibliothèques du système sous `WIN32` posées d'avance (`ole32`, `oleaut32` pour L7-P8 ; `dwmapi` pour la phase 2 de
   L7-T28) : la phase 2 ne touche plus `features/CMakeLists.txt`.
   **Pourquoi retirer `render` sans risque** : vérifié dans 7a et 7b, aucun code de `features/` de la phase 2 n'utilise
   `render` ; le seul usage de `render` du lot 7 est `paintEquation` (`render/src/equationlayout.*`, L7-T25), appelé
   par l'aperçu `ui/src/equationpreviewitem.*` (L7-T26, `ui/` lie déjà `render`) ; l'éditeur `features/…/equationeditor.*`
   travaille sur l'arbre de `core/equation.*`. Si une tâche de phase 2 en avait besoin, la règle se tiendrait autrement :
   lien `PRIVATE` vers `render` et garde-fou appliqué aux seuls en-têtes publics (aucun `#include` de Qt Quick dans
   `features/include`, contrôlé par `git grep`).
5. **Fichiers communs sans conflit.** `features/CMakeLists.txt` liste ses sources par
   `file(GLOB … CONFIGURE_DEPENDS)` sur `src/*.cpp` et `include/actcraft/features/*.h`, puis écarte `*_other.cpp`
   sous Windows et `*_win.cpp` ailleurs. **Justification en une ligne : dix tâches écrites en parallèle créent chacune
   leurs fichiers dans `features/` ; avec une liste écrite, chacune modifierait le même `CMakeLists.txt`.** Raison
   détaillée : chaque tâche crée ses fichiers sans toucher au CMake, et aucun
   fichier source talon n'est nécessaire (un `.cpp` talon vide ferait dire `LNK4221` à `lib.exe`). Écart assumé à la
   convention des autres modules (listes écrites) ; la clôture du lot 7 peut remplacer le motif par la liste finale.
   `tests/presentation.cmake` déclare d'avance, dans L7-P0, les 16 tests de la phase 1, chacun avec un talon
   `reserved()` sauté : **aucune tâche après L7-P0 ne touche un fichier CMake**, et le compte de tests est figé.
   Règle des paires : un fichier `<nom>_win.cpp` n'existe jamais sans son `<nom>_other.cpp` (sinon la CI macOS et
   Android ne lie plus).
6. **Regroupement.** Chaque worktree neuf demande une construction complète, et la session n'a qu'une place de
   tests : 10 tâches au lieu de 17 candidates, groupées par thème, chaque sous-fonction en étapes courtes.
7. **Pas de `Q_OBJECT` dans `features/`** : le fil de reconnaissance (L7-P8) dérive de `QThread` sans signal propre ;
   aucune dépendance au `moc` pour un en-tête placé sous `include/`.
8. **Noms imposés** (socle 1ffb07b, « Changent », point 13 : noms de bf8c930, aucun renommage de plus ; `spotlightFrame`,
   et non `spotFrame` de la version antérieure du socle, contrôlé `NOMS OK`) : `spotlightFrame` (champ des bornes du spot, ici dans `PageToolsSettings` et
   `PageToolsResult`), `shapefinder` (`features/…/shapefinder.h`, `features/src/shapefinder.cpp`,
   `tst_features_shapefinder`). Aucun test de cette phase ne porte sur l'annulation (pas de `…MakesOneStep` ici).
   Les noms de fonctions de test déjà nommés par 7a et 7b pour la logique pure sont repris tels quels.
9. **Constats de L7-T0 intégrés** (rapport `docs/verifications/2026-10-18-lot7-principes.md`, commit 0be9a77 du
   worktree `livraison-complete-b--L7-T0`, « Écarts pour l'orchestrateur » et principes P1 à P23) ; ils remplacent les
   valeurs « à défaut » de 7a et 7b :
   - **dés (E5)** : 1 dé à l'ouverture la première fois, puis le dernier nombre choisi (mémorisé : phase 2) ; bornes 1
     à 5, vitesse 3 à 11 s, défaut 7 → L7-P5 ;
   - **calculatrice (P1, P2, P3)** : affichage du calcul en cours au-dessus du résultat, deux effacements (dernière
     saisie, tout le calcul), texte du calcul et de son résultat à poser sur la page (la pose elle-même : phase 2) →
     L7-P5 ;
   - **spot** : largeur du contour par défaut 10 (réglage embarqué ; mesuré 9 ; la spécification disait 15) → L7-P1 ;
   - **outils de maths** : translucidité par défaut 100 sur 255 (réglage embarqué ; non 255) → L7-P3 ;
   - **`shapefinder` (P16)** : classe « polygone » ajoutée ; rectangle seulement si les angles sont droits, un
     quadrilatère approximatif devient un polygone → L7-P7 ;
   - **reconnaissance** : délai par défaut 1 s, bornes 0 à 5 s (réglage embarqué ; la spécification disait 5 s, 1 à
     5) → L7-P7 ;
   - **outils par page** : la propriété de page 27 (F-591) existe ; sa lecture `.flipchart` reste en phase 2
     (`formats/` interdit) ;
   - **effets (N2-A6)** : 16 mouvements, plus « aucun » et « au hasard », soit 18 entrées, identifiants et noms à nous
     (liste du rapport ; L7-T0 a relevé 17 mouvements, dont un à nom de marque, écarté : décision de l'orchestrateur) ;
     durée 5 à 30 dixièmes, défaut 15, effet de la page prioritaire (confirmés) → L7-P2 ;
   - **E9 et E11** : non tranchés, la spécification s'applique ; aucun effet sur la logique pure (listes d'actions et
     boutons : phase 2). Le zoom de page (`kRatePerSecond = 2.0`) n'est pas traité par le rapport : valeur à nous.

## Compte de tests

**N1** = nombre de tests ctest de `feat/livraison-complete-b` au lancement de L7-P0, relevé par l'orchestrateur
(référence de la session, inscrite dans le fichier d'état). L7-P0 ajoute 16 tests : **N = N1 + 16** pour L7-P0 et pour
**toutes** les tâches suivantes (« = », pas « ≥ »). Après la fusion du socle (phase 2), le compte devient celui de
`feat/livraison-complete` socle compris : N = N_départ + 19 (socle) + n_B, où n_B = les tests de `tests/presentation.cmake` (16 après cette phase ; socle 1ffb07b, « Compte »).

Tests déclarés par L7-P0 (talon `reserved()` → tâche qui le remplit) : `tst_features_curtain`,
`tst_features_spotlight`, `tst_features_pagetools` (L7-P1) ; `tst_features_pagezoom`, `tst_features_transitions`
(L7-P2) ; `tst_features_mathtools` (L7-P3, complété par L7-P4) ; `tst_features_dice`, `tst_features_calculator`,
`tst_features_countdown` (L7-P5) ; `tst_features_ticker` (L7-P6) ; `tst_features_shapefinder`,
`tst_features_recognitionsession` (L7-P7) ; `tst_features_handwriting` (L7-P8) ; `tst_features_camera`,
`tst_features_keyboard`, `tst_features_desktopwheel` (L7-P9).

## Table des tâches (phase 1)

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L7-P0 | Squelette de la phase 1 : dépendances de `features` (sans Qt Quick, avec `input`), sources par motif, 16 tests talons, ligne d'inclusion | — | `features/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/presentation.cmake`, `tests/tst_features_curtain.cpp`, `tests/tst_features_spotlight.cpp`, `tests/tst_features_pagetools.cpp`, `tests/tst_features_pagezoom.cpp`, `tests/tst_features_transitions.cpp`, `tests/tst_features_mathtools.cpp`, `tests/tst_features_dice.cpp`, `tests/tst_features_calculator.cpp`, `tests/tst_features_countdown.cpp`, `tests/tst_features_ticker.cpp`, `tests/tst_features_shapefinder.cpp`, `tests/tst_features_recognitionsession.cpp`, `tests/tst_features_handwriting.cpp`, `tests/tst_features_camera.cpp`, `tests/tst_features_keyboard.cpp`, `tests/tst_features_desktopwheel.cpp` | isolé | implementeur-tdd | rouge : configuration en échec `actcraft_features ne doit pas dépendre de Qt6::Quick` ; vert : `TESTS OK`, N = N1 + 16, les 16 `tst_features_*` Passed, 16 lignes `SKIP   : …reserved() Talon L7-P0`, `NOMS OK` | S10, S22, S23 (support) | #tache-l7-p0 |
| L7-P1 | Rideau, spot et règle des outils par page (T3, T4, T6) | L7-P0 | `features/include/actcraft/features/curtain.h`, `features/src/curtain.cpp`, `features/include/actcraft/features/spotlight.h`, `features/src/spotlight.cpp`, `features/include/actcraft/features/pagetools.h`, `features/src/pagetools.cpp`, `tests/tst_features_curtain.cpp`, `tests/tst_features_spotlight.cpp`, `tests/tst_features_pagetools.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N1 + 16 ; `Totals: 11 passed, 0 failed, 0 skipped` (curtain), `8 passed … 0 skipped` (spotlight), `7 passed … 0 skipped` (pagetools) ; PASS de `dragFromTopUncovers`, `dragFromLeftUncovers`, `dragInsideMaskMoves`, `dragOnRimResizesToOval`, `solidSpotHidesInside`, `keepCarriesCurtainAcrossPages`, `offStopsTools`, `pageModeOverridesKeep` ; `NOMS OK` | S10 | #tache-l7-p1 |
| L7-P2 | Calculs du zoom de page et catalogue et règles des effets (T5, T7) | L7-P0 | `features/include/actcraft/features/pagezoom.h`, `features/src/pagezoom.cpp`, `features/include/actcraft/features/transitions.h`, `features/src/transitions.cpp`, `tests/tst_features_pagezoom.cpp`, `tests/tst_features_transitions.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N1 + 16 ; `Totals: 8 passed, 0 failed, 0 skipped` (pagezoom), `10 passed … 0 skipped` (transitions) ; PASS de `holdZoomsInAroundThePoint`, `doubleClickFits`, `catalogueHasSixteenEffectsPlusTwo`, `randomPicksARealEffect`, `pageEffectWinsOverGlobal`, `effectPlayedWhenLeavingThePage`, `disabledWhileCurtainActive`, `durationClampedTo5To30` ; `NOMS OK` | S10, S22 | #tache-l7-p2 |
| L7-P3 | Guides de tracé, règle et équerre (T9, T10) | L7-P0 | `features/include/actcraft/features/strokeguide.h`, `features/src/strokeguide.cpp`, `features/include/actcraft/features/mathtools.h`, `features/src/mathtools.cpp`, `tests/tst_features_mathtools.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N1 + 16 ; `Totals: 13 passed, 0 failed, 0 skipped` (mathtools) ; PASS de `rulerSnapsNearEdge`, `rulerIgnoresFarSamples`, `strokeStaysOnTheEdgeOnceSnapped`, `setSquareHasThreeEdges`, `setSquareHypotenuseAt45` ; `NOMS OK` | S10 | #tache-l7-p3 |
| L7-P4 | Rapporteur, compas et mémoire de l'origine XY (T11, T12, T13) | L7-P3 | `features/include/actcraft/features/mathtools.h`, `features/src/mathtools.cpp`, `tests/tst_features_mathtools.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N1 + 16 ; `Totals: 22 passed, 0 failed, 0 skipped` (mathtools) ; PASS de `protractorArcFollowsTheRim`, `closedArcAddsTwoRadii`, `halfProtractorStopsAt180`, `compassDrawsAnArcAtItsRadius`, `fullTurnClosesTheCircle`, `radiusInCentimetres`, `originRememberedPerPage` ; `NOMS OK` | S10 | #tache-l7-p4 |
| L7-P5 | Dés, calculatrice et compte à rebours (T14, T15, T16) | L7-P0 | `features/include/actcraft/features/dice.h`, `features/src/dice.cpp`, `features/include/actcraft/features/calculator.h`, `features/src/calculator.cpp`, `features/include/actcraft/features/countdown.h`, `features/src/countdown.cpp`, `tests/tst_features_dice.cpp`, `tests/tst_features_calculator.cpp`, `tests/tst_features_countdown.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N1 + 16 ; `Totals: 7 passed … 0 skipped` (dice), `11 passed … 0 skipped` (calculator), `8 passed … 0 skipped` (countdown) ; PASS de `seededRollIsReproducible`, `countClampedTo1To5`, `defaultIsOneDie`, `operatorPrecedenceLeftToRight`, `expressionShownAboveTheResult`, `clearEntryKeepsTheCalculation`, `pageTextHasTheCalculationAndItsResult`, `divisionByZeroIsAnError`, `percentAndSign`, `endTriggersActionOnce`, `repeatRestarts`, `pauseFreezes` ; `NOMS OK` | S10 | #tache-l7-p5 |
| L7-P6 | Modèle du message défilant et fichier de messages (T18, T19) | L7-P0 | `features/include/actcraft/features/ticker.h`, `features/src/ticker.cpp`, `tests/tst_features_ticker.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N1 + 16 ; `Totals: 9 passed, 0 failed, 0 skipped` (ticker) ; PASS de `singlePassEndsHidden`, `loopRestarts`, `speedMapsToPixelsPerSecond`, `positionsAreThree`, `messageFileRoundTrip` ; `NOMS OK` | S23 | #tache-l7-p6 |
| L7-P7 | Reconnaissance de forme `shapefinder` et session de reconnaissance (T20, minuterie de T21) | L7-P0 | `features/include/actcraft/features/shapefinder.h`, `features/src/shapefinder.cpp`, `features/include/actcraft/features/recognitionsession.h`, `features/src/recognitionsession.cpp`, `tests/tst_features_shapefinder.cpp`, `tests/tst_features_recognitionsession.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N1 + 16 ; `Totals: 13 passed, 0 failed, 0 skipped` (shapefinder), `7 passed … 0 skipped` (recognitionsession) ; PASS de `wobblyCircleIsACircle`, `tiltedRectangleKeepsItsCorners`, `roughQuadrilateralIsAPolygon`, `pentagonIsAPolygon`, `defaultDelayIsOneSecond`, `openScribbleIsNothing`, `shortStrokeIsNothing`, `sessionFiresAfterTheDelay`, `newStrokeRestartsTheDelay` ; `NOMS OK` | S10 | #tache-l7-p7 |
| L7-P8 | Reconnaissance d'écriture : interface, moteur d'encre de Windows (`handwriting_win.cpp`) et fil dédié en STA (T22, fil de T23) | L7-P0 | `features/include/actcraft/features/handwriting.h`, `features/src/handwriting.cpp`, `features/src/handwriting_win.cpp`, `features/src/handwriting_other.cpp`, `tests/tst_features_handwriting.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N1 + 16 ; `Totals: 7 passed, 0 failed, 0 skipped` (handwriting, sous Windows), ou `6 passed … 1 skipped` si et seulement si le SKIP est `recognizesSyntheticInk` avec « reconnaisseur absent : … » (signalé au rapport) ; PASS de `recognitionRunsOnOneDedicatedThread`, `recognitionThreadIsSingleThreadedApartment`, `factoryNeverReturnsNull` ; `NOMS OK` | S10 | #tache-l7-p8 |
| L7-P9 | Géométrie de la caméra, disposition du clavier à l'écran, roue des outils de bureau (T28, T29, T31) | L7-P0 | `features/include/actcraft/features/camera.h`, `features/src/camera.cpp`, `features/include/actcraft/features/screenkeyboard.h`, `features/src/screenkeyboard.cpp`, `features/include/actcraft/features/desktopwheel.h`, `features/src/desktopwheel.cpp`, `tests/tst_features_camera.cpp`, `tests/tst_features_keyboard.cpp`, `tests/tst_features_desktopwheel.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N1 + 16 ; `Totals: 7 passed … 0 skipped` (camera), `7 passed … 0 skipped` (keyboard), `4 passed … 0 skipped` (desktopwheel) ; PASS de `clickNearFirstPointClosesThePolygon`, `freehandClosesOnRelease`, `cropOutsideIsTransparent`, `windowRectsInLogicalPixels`, `shiftTypesCapitals`, `compactLayoutHasFewerKeys`, `accentKeysTypeFrench`, `wheelIsClockwiseFromTop` ; `NOMS OK` | S10 | #tache-l7-p9 |

10 tâches, un seul lot. Aucune preuve visuelle : aucune tâche ne change l'interface (logique sans écran).

## Vagues (une seule construction à la fois ; le code s'écrit en parallèle)

Après L7-P0, les tâches ne partagent aucun fichier, sauf L7-P3 → L7-P4 (`mathtools.h`, `mathtools.cpp`,
`tst_features_mathtools.cpp`), tenu par « dépend de ». Chaque agent écrit dans son worktree ; **toute construction,
suite ou test passe par le jeton `tests`** (une à la fois dans la session ; un agent qui attend le jeton attend sa
notification sans sonder). **Deux tâches à la fois au plus** (profil `.claude/chantier.md`, « Parallélisme », qui fait
autorité), et une seule construction à la fois sous le jeton, toujours :

- **V1** : L7-P0, seule (tout le reste s'appuie sur ses fichiers CMake).
- **V2** : L7-P3 ‖ L7-P7 (les deux `concepteur-critique` les plus longues d'abord, pour que leur revue adversariale
  chevauche la vague suivante).
- **V3** : L7-P8 ‖ L7-P1.
- **V4** : L7-P4 (L7-P3 mergée) ‖ L7-P2.
- **V5** : L7-P5 ‖ L7-P6.
- **V6** : L7-P9.

Toute tâche de V2 à V6 peut changer de vague, sauf L7-P4 (après le merge de L7-P3) : aucun fichier commun ; une vague
ne compte jamais plus de deux tâches. **L7-T0** a rendu son
rapport (décision 9) : ses valeurs sont dans les sections. Si un rapport ultérieur (amendement) en change une, son
constante nommée se corrige dans la tâche concernée avant son lancement, ou par la phase 2.

## Couverture des scénarios du cadrage

| Scénario | Tâches de phase 1 | Reste à la phase 2 |
|---|---|---|
| S10 — présentation | L7-P0, L7-P1, L7-P2, L7-P3, L7-P4, L7-P5, L7-P7, L7-P8, L7-P9 | contrôleurs, surcouches, couvertures, scénario `presentation` |
| S22 — effets de page | L7-P2 (catalogue, règles, durée) | lecteur, navigateur `PageEffectsPicker.qml`, banc |
| S23 — message défilant | L7-P6 (modèle, temps, fichier) | surcouche `XAnimator`, fenêtre, banc |
| S24 — équation | aucune : `core/equation.*`, `render/src/equationlayout.*` réservés par L0-S3 (addendum 1, point 4) | tout (L7-T24 à L7-T27) |
| S30 — tâche d'ouverture | aucune : L7-T0, hors de cet amendement | L7-T0 |

## Candidats écartés ou réduits

- **Aucun candidat de la liste n'est écarté entièrement.** Réductions, chacune pour sa raison :
- **L7-T6** : la conversion depuis `core::PagePresentation` (et la règle « `CurtainMode::Off` sous le mode rideau =
  complet ») exige le type du socle (`core/`, L0-S3) : la phase 1 livre la règle sur `PageToolsSettings` ; la
  conversion revient à la phase 2.
- **L7-T13** : la lecture de l'origine dans `PagePresentation` exige `core/` : `OriginMemory` travaille sur un index de
  page et un `QPointF`.
- **L7-T21** : le remplacement du trait (`replaceObjects`, `DocumentInsertions`) exige `ui/` et L0-S5 ; seule la
  minuterie (`RecognitionSession`) entre ici.
- **L7-T23** : le contrôleur et les alternatives exigent `ui/` ; le fil dédié STA (la partie qui coûte cher si le
  patron est faux, relecture I7) entre ici, avec `recognitionRunsOnOneDedicatedThread`.
- **L7-T28** : `topLevelWindows()` (`camera_win.cpp`, `EnumWindows`, `DwmGetWindowAttribute`) n'est pas de la
  géométrie et ne se prouve qu'avec de vraies fenêtres : phase 2 (fichiers de `features/`, permis, mais hors du
  périmètre « géométrie » du brief). `toLogicalPixels`, la fonction pure qui en corrige les pixels physiques, entre
  ici.
- **L7-T29** : `keyinjector_win.cpp` (`SendInput`) n'est jamais appelé par un test (7b, interdits) : phase 2.
- **L7-T31** : `wheelHasNoVoteNorVendorEntry` vérifie les identifiants contre le registre des commandes, dont les
  commandes du lot 7 arrivent par L0-S4 (`actions/`) : phase 2. La phase 1 contrôle les préfixes
  (`defaultWheelUsesDesktopAndToolCommands`).
- **L7-T26** (éditeur d'équation, hors liste) : son modèle s'écrit sur l'arbre de `core/equation.*` (L0-S3) : phase 2.

## Règles communes à toutes les tâches (à joindre à chaque section)

Celles de 7a (« Règles communes ») s'appliquent pour tout ce qui suit et qu'elles ne contredisent pas. S'y ajoutent ou
s'y substituent :

- `<wt>` = `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b--L7-P<n>` ; `<id>` = `L7-P<n>`. Toute
  commande PowerShell commence par `Set-Location '<wt>';` ou passe par `git -C '<wt>'`. Toujours `pwsh -NoProfile`.
- **Fichiers** : ceux de la ligne de la table, et aucun autre. Un besoin hors de la liste (un type de `core/`, une
  ligne d'un CMake) : s'arrêter et le rapporter à l'orchestrateur.
- **Code** : C++20, QtCore et QtGui seulement, **jamais Qt Quick ni QML** (le garde-fou de L7-P0 le refuse à la
  configuration) ; API publique de Qt seulement ; aucun `#ifdef` de plateforme dans le code livré (fichiers
  `*_win.cpp` / `*_other.cpp`, choisis par CMake) ; aucun `Q_OBJECT` dans `features/`. Avertissements en erreur :
  MSVC `/W4 /WX /permissive-` en local ; Clang `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror` en CI (macOS,
  Android), invisible en local : conversions explicites (`static_cast`, `qsizetype` contre `int`, `std::size_t`
  contre `int`), paramètres de constructeur nommés autrement que les membres, `QCOMPARE` sur deux valeurs du **même**
  type (`qsizetype{3}`, `std::size_t{3}`), pas de `M_PI` (`std::numbers::pi`).
- **Propriété intellectuelle** : rien de Promethean, aucun titre ni citation des vidéos, seuils et données écrits par
  nous avec leur raison en commentaire ; libellés français à nous.
- **Types du socle** : ne jamais déclarer `PagePresentation`, `CurtainMode`, `SpotlightShape`, `PageToolsMode`,
  `Equation`, `ReplaceObjectsCommand`, `SetPagePresentationCommand` (addendum 1, socle 1ffb07b).
- **Talon remplacé** : la tâche réécrit entièrement chaque `tests/tst_features_<x>.cpp` de sa ligne ; la fonction
  `reserved()` et le marqueur `Talon L7-P0 : rempli par L7-P<n>.` disparaissent. Contrôle :
  `git -C '<wt>' grep -n "Talon L7-P0 : rempli par L7-P<n>\."` vide.
- **Premier passage dans un worktree neuf** (construction complète) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire livraison-complete-b/<id> -Commande 'pwsh -NoProfile -File ./scripts/build.ps1 *> .tmp/build.txt'; Select-String -Path .tmp/build.txt -Pattern 'error|FAILED' | Select-Object -Last 4"
  ```
- **Commande ciblée** (rouge puis vert, `<motif>` donné par la section) — sortie de Qt Test lue par `-V` :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire livraison-complete-b/<id> -Commande '. ./scripts/dev-env.ps1; cmake --build --preset debug *> .tmp/build.txt; ctest --preset debug -R ''<motif>'' -V *> .tmp/ctest.txt'; Select-String -Path .tmp/build.txt,.tmp/ctest.txt -Pattern 'error C|error:|FAIL!  :|SKIP   :|Totals:|tests passed|tests failed|No tests were found' | Select-Object -Last 12"
  ```
  Rouge attendu : `error C…` / `error:` sur le nom testé, ou `FAIL!  :`. Vert attendu (k = nombre de fonctions de la
  classe **+ 2** : Qt Test compte toujours `initTestCase` et `cleanupTestCase`) : une ligne `Totals: <k> passed,
  0 failed, 0 skipped` par test du motif (k de la table), `100% tests passed`. Le nom d'une fonction se lit par
  `Select-String -Path .tmp/ctest.txt -Pattern 'PASS   : .*<fonction>\(\)'`.
  Jamais de `|` sur la sortie du jeton (le jeton en meurt) : on lit les fichiers après.
- **Mise en forme** avant la preuve : `pwsh -NoProfile -Command "Set-Location '<wt>'; pwsh -NoProfile -File ./scripts/format.ps1"`
  (la suite contrôle la mise en forme : `TESTS EN ECHEC` sinon).
- **Preuve de tâche** (forme bornée du profil, sous le jeton) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire livraison-complete-b/<id> -Commande 'pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1'; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2; Select-String -Path .tmp/test.txt -Pattern 'tst_features_.*Passed' | Select-Object -Last 16"
  ```
  Attendu : `100% tests passed, 0 tests failed out of N` avec **N = N1 + 16** exactement, `TESTS OK`, et la ligne
  `Passed` de chaque test de la tâche. Un test a tourné : N ≥ 1, le nom attendu est présent, et la commande ciblée
  verte a montré `0 skipped` (un SKIP ne prouve rien).
- **Contrôle des noms**, après `git -C '<wt>' add <fichiers>` (l'outil ne lit que les fichiers suivis) :
  `python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" '<wt>'` → dernière ligne
  `NOMS OK`. Le préfixe `m_` est retiré avant la comparaison : un membre `m_<nom>` peut être refusé alors que
  `<nom>` seul, nom de Qt, passe (vu : le membre de la taille de page de `OriginMemory`). Un nom refusé se renomme ;
  il ne se contourne pas. **Seul cas permis, tranché** : un nom d'une API tierce imposée (SDK de Windows) homonyme d'un
  nom d'Inspire — vu : l'accesseur de la propriété `String` d'une alternative d'écriture (L7-P8). C'est un faux
  positif ; on ne l'écrit pas (ni code ni commentaire) et on passe par une voie de l'API qui ne l'écrit pas (ici
  `IDispatch`, identifiant obtenu par son nom à l'exécution). Le contrôle reste appliqué tel quel, sans exception
  dans l'outil : ce n'est pas un contournement, aucun identifiant refusé n'entre dans le dépôt.
- **Commit** : `git -C '<wt>' add` de chaque fichier de la ligne, nommé (jamais `-A`, `--all`, `.`), relire
  `git -C '<wt>' diff --cached --stat` (aucun fichier hors de la ligne), puis
  `git -C '<wt>' commit -m "<message de la section>" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"`.
- **Rapport** (avant la limite de tours) : sha du commit, les trois sorties bornées (commande ciblée verte, preuve,
  `NOMS OK`), écarts au plan.

---

## Tâches

<a id="tache-l7-p0"></a>
### Tâche L7-P0 — Squelette de la phase 1

**Objectif.** `features` cesse de dépendre de Qt Quick (via `render`) et gagne `input` ; ses sources se lisent par
motif, `*_win.cpp` / `*_other.cpp` selon la plateforme ; les 16 tests de la phase 1 sont déclarés dans le nouveau
`tests/presentation.cmake`, chacun avec un talon sauté ; `tests/CMakeLists.txt` l'inclut en dernière ligne. Aucun
comportement.

**Fichiers.** Ligne L7-P0 de la table.

**Test et commande bornée.** Rouge : la configuration échoue sur le garde-fou. Vert : commande ciblée
`-R 'tst_features_|tst_modules'`, puis preuve de tâche (N = N1 + 16).

**Preuve visuelle.** Aucune.

**Interdits.** Aucun fichier de `features/include` ni de `features/src` créé ou modifié (`features.h`, `features.cpp`
restent) ; aucun autre fichier CMake ; aucune autre ligne de `tests/CMakeLists.txt`.

**Terminé quand.** Garde-fou vu en rouge puis vert ; `TESTS OK`, N = N1 + 16 ; 16 lignes
`SKIP   : Tst….:reserved() Talon L7-P0 : rempli par L7-P<n>.` dans `.tmp/ctest.txt` ; `tst_modules` Passed ;
`NOMS OK` ; commit.

**Étapes.**

- [ ] **Étape 1 : relever N1.** Premier passage (construction complète), puis
  `pwsh -NoProfile -Command "Set-Location '<wt>'; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire livraison-complete-b/L7-P0 -Commande '. ./scripts/dev-env.ps1; ctest --preset debug -N *> .tmp/liste.txt'; Select-String -Path .tmp/liste.txt -Pattern 'Total Tests:'"`.
  Comparer à N1 donné par l'orchestrateur ; un écart se rapporte (la preuve utilise la valeur relevée ici).
- [ ] **Étape 2 : rouge du garde-fou.** Ajouter seulement, à la fin de `features/CMakeLists.txt` actuel :
  ```cmake
  actcraft_forbid_links(actcraft_features Qt6::Quick Qt6::Qml Qt6::QuickControls2 Qt6::Widgets)
  ```
  puis, sous le jeton : `-Commande '. ./scripts/dev-env.ps1; cmake --preset debug *> .tmp/configure.txt'` et
  `Select-String -Path .tmp/configure.txt -Pattern 'ne doit pas d'` (motif sans accent : la sortie redirigée peut ne
  pas être en UTF-8). Attendu :
  `actcraft_features ne doit pas dépendre de Qt6::Quick (règle de modularité, voir docs/BRIEF.md)`.
- [ ] **Étape 3 : `features/CMakeLists.txt` complet** (remplace tout le fichier) :
  ```cmake
  # Fonctions activables ou retirables : règle, rapporteur, dés, horloge, rideau, spot…
  # Logique pure (QtCore, QtGui), testée sans écran ; jamais Qt Quick ni QML (garde-fou ci-dessous).
  # Sources lues par motif (lot 7, phase 1) : chaque fonction ajoute ses fichiers sans toucher à ce fichier.
  # Code propre à une plateforme : <nom>_win.cpp (Windows) et son jumeau <nom>_other.cpp (ailleurs), toujours par paire.
  file(GLOB _features_sources CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/include/actcraft/features/*.h)
  if(WIN32)
      list(FILTER _features_sources EXCLUDE REGEX "_other\\.cpp$")
  else()
      list(FILTER _features_sources EXCLUDE REGEX "_win\\.cpp$")
  endif()

  actcraft_add_module(features
      SOURCES
          ${_features_sources}
      DEPENDS
          ActCraft::core ActCraft::actions ActCraft::input)

  if(WIN32)
      # Reconnaissance d'écriture par COM (handwriting_win.cpp) ; cadres des fenêtres (camera_win.cpp, phase 2).
      target_link_libraries(actcraft_features PRIVATE ole32 oleaut32 dwmapi)
  endif()

  # Règle du lot 7 : la logique de features/ ne dépend d'aucune interface Qt Quick, QML ou Widgets.
  actcraft_forbid_links(actcraft_features Qt6::Quick Qt6::Qml Qt6::QuickControls2 Qt6::Widgets)
  ```
  Sous le jeton, `cmake --preset debug *> .tmp/configure.txt` : plus de ligne `ne doit pas dépendre`.
- [ ] **Étape 4 : seize tests talons.** Un fichier par test, sur ce modèle exact (classe, macro et tâche selon la
  liste ci-dessous) :
  ```cpp
  // Talon L7-P0 : rempli par L7-P1.
  #include <QtTest>

  class TstFeaturesCurtain : public QObject
  {
      Q_OBJECT

  private slots:
      void reserved()
      {
          QSKIP("Talon L7-P0 : rempli par L7-P1.");
      }
  };

  QTEST_GUILESS_MAIN(TstFeaturesCurtain)
  #include "tst_features_curtain.moc"
  ```
  | fichier | classe | macro | tâche |
  |---|---|---|---|
  | `tst_features_curtain.cpp` | `TstFeaturesCurtain` | `QTEST_GUILESS_MAIN` | L7-P1 |
  | `tst_features_spotlight.cpp` | `TstFeaturesSpotlight` | `QTEST_GUILESS_MAIN` | L7-P1 |
  | `tst_features_pagetools.cpp` | `TstFeaturesPageTools` | `QTEST_GUILESS_MAIN` | L7-P1 |
  | `tst_features_pagezoom.cpp` | `TstFeaturesPageZoom` | `QTEST_GUILESS_MAIN` | L7-P2 |
  | `tst_features_transitions.cpp` | `TstFeaturesTransitions` | `QTEST_GUILESS_MAIN` | L7-P2 |
  | `tst_features_mathtools.cpp` | `TstFeaturesMathTools` | `QTEST_GUILESS_MAIN` | L7-P3 |
  | `tst_features_dice.cpp` | `TstFeaturesDice` | `QTEST_GUILESS_MAIN` | L7-P5 |
  | `tst_features_calculator.cpp` | `TstFeaturesCalculator` | `QTEST_GUILESS_MAIN` | L7-P5 |
  | `tst_features_countdown.cpp` | `TstFeaturesCountdown` | `QTEST_GUILESS_MAIN` | L7-P5 |
  | `tst_features_ticker.cpp` | `TstFeaturesTicker` | `QTEST_GUILESS_MAIN` | L7-P6 |
  | `tst_features_shapefinder.cpp` | `TstFeaturesShapeFinder` | `QTEST_GUILESS_MAIN` | L7-P7 |
  | `tst_features_recognitionsession.cpp` | `TstFeaturesRecognitionSession` | `QTEST_GUILESS_MAIN` | L7-P7 |
  | `tst_features_handwriting.cpp` | `TstFeaturesHandwriting` | `QTEST_GUILESS_MAIN` | L7-P8 |
  | `tst_features_camera.cpp` | `TstFeaturesCamera` | `QTEST_MAIN` | L7-P9 |
  | `tst_features_keyboard.cpp` | `TstFeaturesKeyboard` | `QTEST_GUILESS_MAIN` | L7-P9 |
  | `tst_features_desktopwheel.cpp` | `TstFeaturesDesktopWheel` | `QTEST_GUILESS_MAIN` | L7-P9 |
- [ ] **Étape 5 : `tests/presentation.cmake`** (nouveau) :
  ```cmake
  # Tests du lot 7 (présentation et outils) : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).
  # Inclus en dernier par tests/CMakeLists.txt (OPTIONAL). Phase 1 (L7-P0) : logique pure de features/, sans écran ;
  # la phase 2 ajoute ici les tests d'interface et de captures du lot.
  actcraft_add_test(tst_features_curtain SOURCES tst_features_curtain.cpp LIBS ActCraft::features)
  actcraft_add_test(tst_features_spotlight SOURCES tst_features_spotlight.cpp LIBS ActCraft::features)
  actcraft_add_test(tst_features_pagetools SOURCES tst_features_pagetools.cpp LIBS ActCraft::features)
  actcraft_add_test(tst_features_pagezoom SOURCES tst_features_pagezoom.cpp LIBS ActCraft::features)
  actcraft_add_test(tst_features_transitions SOURCES tst_features_transitions.cpp LIBS ActCraft::features)
  actcraft_add_test(tst_features_mathtools SOURCES tst_features_mathtools.cpp LIBS ActCraft::features)
  actcraft_add_test(tst_features_dice SOURCES tst_features_dice.cpp LIBS ActCraft::features)
  actcraft_add_test(tst_features_calculator SOURCES tst_features_calculator.cpp LIBS ActCraft::features)
  actcraft_add_test(tst_features_countdown SOURCES tst_features_countdown.cpp LIBS ActCraft::features)
  actcraft_add_test(tst_features_ticker SOURCES tst_features_ticker.cpp LIBS ActCraft::features)
  actcraft_add_test(tst_features_shapefinder SOURCES tst_features_shapefinder.cpp LIBS ActCraft::features)
  actcraft_add_test(tst_features_recognitionsession SOURCES tst_features_recognitionsession.cpp LIBS ActCraft::features)
  actcraft_add_test(tst_features_handwriting SOURCES tst_features_handwriting.cpp LIBS ActCraft::features)
  actcraft_add_test(tst_features_camera SOURCES tst_features_camera.cpp LIBS ActCraft::features)
  actcraft_add_test(tst_features_keyboard SOURCES tst_features_keyboard.cpp LIBS ActCraft::features)
  actcraft_add_test(tst_features_desktopwheel SOURCES tst_features_desktopwheel.cpp LIBS ActCraft::features)
  ```
- [ ] **Étape 6 : ligne d'inclusion.** Ajouter **après** la ligne `include(${CMAKE_CURRENT_SOURCE_DIR}/app.cmake)`, en
  dernière ligne de `tests/CMakeLists.txt`, exactement :
  `include(${CMAKE_CURRENT_SOURCE_DIR}/presentation.cmake OPTIONAL)`. `git -C '<wt>' diff tests/CMakeLists.txt` :
  une seule ligne ajoutée.
- [ ] **Étape 7 : vert.** `format.ps1` ; commande ciblée `-R 'tst_features_|tst_modules'` : 17 tests, 16 lignes `SKIP   :`
  nommant leur tâche, `tst_modules` en `PASS` ; preuve de tâche : N = N1 + 16, `TESTS OK`.
- [ ] **Étape 8 : noms et commit.** `git add` des 19 fichiers nommés, `NOMS OK`, commit
  « Lot 7, phase 1 : features sans Qt Quick, sources par motif, seize tests réservés (L7-P0) ».

<a id="tache-l7-p1"></a>
### Tâche L7-P1 — Rideau, spot et règle des outils par page (T3, T4, T6)

**Objectif.** Logique du store du rideau (F-265, F-592), du spot (F-270) et de la règle « outils de page » (F-591 à
F-593), sur des types propres à `features/`.

**Fichiers.** Ligne L7-P1 de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_features_(curtain|spotlight|pagetools)'`, preuve de tâche.

**Preuve visuelle.** Aucune.

**Interdits.** Règles communes ; pas de `core::PagePresentation` ni d'énumération du socle (`PageToolChoice` est à
nous, valeurs égales à celles de `core::PageToolsMode` pour la phase 2) ; aucune minuterie.

**Terminé quand.** Les trois tests verts sans saut, preuve, `NOMS OK`, commit.

**Étapes.**

- [ ] **Étape 1 : rouge du rideau.** `tests/tst_features_curtain.cpp` = le test de 7a, section L7-T3, étape 1, **mot pour
  mot** (`closedCoversEverything`, `dragFromTopUncovers`, `dragFromLeftUncovers`, `pressInsideDoesNotGrab`,
  `edgeOfAnOpenCurtainCanBeDraggedBack`, `clampedToTheArea`, `resizeKeepsProportions`, `startModeAndDistance`), plus :
  ```cpp
      void releaseEndsTheDrag()
      {
          Curtain c(QSizeF(1000, 800));
          QVERIFY(c.press(QPointF(500, 2)));
          QVERIFY(c.dragging());
          c.release();
          QVERIFY(!c.dragging());
          c.move(QPointF(500, 600)); // sans saisie : rien ne bouge
          QCOMPARE(c.coverage(), 1.0);
      }
  ```
  Commande ciblée `-R tst_features_curtain` : rouge (`curtain.h` absent).
- [ ] **Étape 2 : rideau.** `features/include/actcraft/features/curtain.h` = l'en-tête de 7a, L7-T3, étape 2, mot pour
  mot. `features/src/curtain.cpp` :
  ```cpp
  #include <actcraft/features/curtain.h>

  #include <algorithm>
  #include <cmath>

  namespace actcraft::features {

  Curtain::Curtain(QSizeF area)
      : m_area(area)
  {
  }

  void Curtain::setArea(QSizeF area)
  {
      m_area = area; // les fractions découvertes restent : les proportions sont gardées
  }

  void Curtain::reset(Start start, double distance)
  {
      m_top = m_bottom = m_left = m_right = 0;
      m_edge = Edge::None;
      const double w = m_area.width();
      const double h = m_area.height();
      const auto fraction = [](double d, double size) { return size > 0 ? std::clamp(d / size, 0.0, 1.0) : 0.0; };
      switch (start) {
      case Start::Full:
          break;
      case Start::Top:
          m_top = fraction(distance, h);
          break;
      case Start::Bottom:
          m_bottom = fraction(distance, h);
          break;
      case Start::Left:
          m_left = fraction(distance, w);
          break;
      case Start::Right:
          m_right = fraction(distance, w);
          break;
      }
  }

  bool Curtain::press(QPointF point)
  {
      const double w = m_area.width();
      const double h = m_area.height();
      if (w <= 0 || h <= 0)
          return false;
      const double top = m_top * h, bottom = (1 - m_bottom) * h, left = m_left * w, right = (1 - m_right) * w;
      const bool alongX = point.x() >= left - kGrabMargin && point.x() <= right + kGrabMargin;
      const bool alongY = point.y() >= top - kGrabMargin && point.y() <= bottom + kGrabMargin;
      struct Candidate { Edge edge; double distance; };
      const Candidate candidates[] = {
          {Edge::Top, alongX ? std::abs(point.y() - top) : kGrabMargin + 1},
          {Edge::Bottom, alongX ? std::abs(point.y() - bottom) : kGrabMargin + 1},
          {Edge::Left, alongY ? std::abs(point.x() - left) : kGrabMargin + 1},
          {Edge::Right, alongY ? std::abs(point.x() - right) : kGrabMargin + 1},
      };
      const auto best = std::min_element(std::begin(candidates), std::end(candidates),
                                         [](const Candidate& a, const Candidate& b) { return a.distance < b.distance; });
      m_edge = best->distance <= kGrabMargin ? best->edge : Edge::None;
      return m_edge != Edge::None;
  }

  void Curtain::move(QPointF point)
  {
      const double w = m_area.width();
      const double h = m_area.height();
      if (w <= 0 || h <= 0)
          return;
      switch (m_edge) {
      case Edge::None:
          break;
      case Edge::Top:
          m_top = std::clamp(point.y() / h, 0.0, 1 - m_bottom);
          break;
      case Edge::Bottom:
          m_bottom = std::clamp((h - point.y()) / h, 0.0, 1 - m_top);
          break;
      case Edge::Left:
          m_left = std::clamp(point.x() / w, 0.0, 1 - m_right);
          break;
      case Edge::Right:
          m_right = std::clamp((w - point.x()) / w, 0.0, 1 - m_left);
          break;
      }
  }

  void Curtain::release()
  {
      m_edge = Edge::None;
  }

  bool Curtain::dragging() const noexcept
  {
      return m_edge != Edge::None;
  }

  QRectF Curtain::covered() const noexcept
  {
      const double w = m_area.width();
      const double h = m_area.height();
      const QRectF r(m_left * w, m_top * h, w * (1 - m_left - m_right), h * (1 - m_top - m_bottom));
      return (r.width() <= 0 || r.height() <= 0) ? QRectF() : r;
  }

  double Curtain::coverage() const noexcept
  {
      const double area = m_area.width() * m_area.height();
      if (area <= 0)
          return 0;
      const QRectF r = covered();
      return r.width() * r.height() / area;
  }

  } // namespace actcraft::features
  ```
  Commande ciblée `-R tst_features_curtain` : `Totals: 11 passed, 0 failed, 0 skipped` (9 fonctions + 2).
- [ ] **Étape 3 : rouge du spot.** `tests/tst_features_spotlight.cpp` : montage du rideau (`#include
  <actcraft/features/spotlight.h>`, `#include <QtTest>`, `using namespace actcraft::features;`, classe
  `TstFeaturesSpotlight`, `QTEST_GUILESS_MAIN`, `#include "tst_features_spotlight.moc"`) et, comme fonctions, les six
  de 7a, L7-T4, étape 1, mot pour mot (`defaultFrameIsCentered`, `dragInsideMaskMoves`, `dragOnRimResizesToOval`,
  `minimumSize`, `solidSpotHidesInside`, `frameKeptInsideTheArea`). Rouge.
- [ ] **Étape 4 : spot.** `features/include/actcraft/features/spotlight.h` :
  ```cpp
  #pragma once

  #include <QPointF>
  #include <QRectF>
  #include <QSizeF>

  namespace actcraft::features {

  // Spot (F-270) dans un rectangle de taille `area` : un spot montre le cadre et masque le reste ; un cache plein
  // masque le cadre. Le pourtour du cadre (à kRimMargin près) le redimensionne côté par côté (ovale, rectangle) ; la
  // partie masquée le déplace ; ailleurs, l'appui n'est pas pris (il traverse vers la page).
  class Spotlight
  {
  public:
      // Mêmes valeurs que core::SpotlightShape (socle L0-S3) : la phase 2 les relie par une conversion vérifiée.
      enum class Shape { Off = 0, Circle = 1, Square = 2, SolidCircle = 3, SolidSquare = 4 };
      static constexpr double kDefaultSize = 300; // côté du cadre par défaut, unités de `area`
      static constexpr double kMinimumSize = 40;  // un cadre plus petit ne se saisit plus au doigt
      static constexpr double kRimMargin = 16;    // largeur de la bande de saisie du pourtour
      // Largeur du contour dessiné autour du cadre (lue par la surcouche, phase 2) : 10, réglage embarqué relevé par
      // L7-T0 (docs/verifications/2026-10-18-lot7-principes.md ; mesuré 9 ; la spécification disait 15).
      static constexpr double kDefaultOutline = 10;

      explicit Spotlight(QSizeF area);
      void setArea(QSizeF area);
      void setShape(Shape shape);
      Shape shape() const noexcept;
      void setFrame(QRectF frame); // vide : cadre par défaut, centré, kDefaultSize de côté
      QRectF frame() const noexcept;
      bool hidesInside() const noexcept; // vrai pour SolidCircle et SolidSquare
      bool press(QPointF point);         // vrai si le cadre est saisi (déplacer ou redimensionner)
      void move(QPointF point);
      void release();
      bool dragging() const noexcept;

  private:
      enum class Grab { None, Move, Left, Right, Top, Bottom };
      QRectF defaultFrame() const;
      void keepInside(); // le centre du cadre reste dans la zone : le cadre garde une intersection avec elle

      QSizeF m_area;
      Shape m_shape = Shape::Off;
      QRectF m_frame;
      Grab m_grab = Grab::None;
      QPointF m_last;
  };

  } // namespace actcraft::features
  ```
  `features/src/spotlight.cpp` : le constructeur pose `m_frame = defaultFrame()` ; `setFrame` (vide → défaut) ;
  `press` : rien si `Off` ou zone vide ; d'abord le **pourtour** (pour chaque côté du rectangle `m_frame`, distance
  au côté ≤ `kRimMargin` et position le long du côté dans l'étendue du côté ± `kRimMargin` ; le plus proche gagne →
  `Grab::Left/Right/Top/Bottom`) ; sinon la **partie masquée** (hors du cadre pour `Circle`/`Square`, dans le cadre
  pour les caches pleins) → `Grab::Move` ; sinon faux. `m_last = point`. `move` : `Move` translate de
  `point - m_last` puis `keepInside()` ; `Left` : `left = min(point.x, right − kMinimumSize)`, `Right` :
  `right = max(point.x, left + kMinimumSize)`, idem `Top`/`Bottom` ; `m_last = point`. `release` : `Grab::None`.
  Vérifier à la main sur les six tests : `dragOnRimResizesToOval` saisit le côté droit (650 = 500 + 150) avant le
  masque. Commande ciblée `-R tst_features_spotlight` : `8 passed … 0 skipped`.
- [ ] **Étape 5 : rouge de la règle par page.** `tests/tst_features_pagetools.cpp` :
  ```cpp
  #include <actcraft/features/pagetools.h>

  #include <QtTest>

  using namespace actcraft::features;

  class TstFeaturesPageTools : public QObject
  {
      Q_OBJECT

  private slots:
      void keepCarriesCurtainAcrossPages()
      {
          const PresentationState now{true, Spotlight::Shape::Off}; // rideau ouvert sur la page quittée
          const auto next = resolvePageTools(now, PageToolsSettings{}); // page « comme avant »
          QVERIFY(next.curtain);
          QCOMPARE(next.resetCurtain, false);
      }
      void offStopsTools()
      {
          PageToolsSettings p;
          p.tools = PageToolChoice::Off;
          const auto next = resolvePageTools({true, Spotlight::Shape::Circle}, p);
          QVERIFY(!next.curtain);
          QCOMPARE(next.spotlight, Spotlight::Shape::Off);
      }
      void pageModeOverridesKeep()
      {
          PageToolsSettings p;
          p.tools = PageToolChoice::Curtain;
          p.curtainStart = Curtain::Start::Left;
          p.curtainDistance = 120;
          const auto next = resolvePageTools({false, Spotlight::Shape::Off}, p);
          QVERIFY(next.curtain);
          QVERIFY(next.resetCurtain);
          QCOMPARE(next.curtainStart, Curtain::Start::Left);
          QCOMPARE(next.curtainDistance, 120.0);
          QCOMPARE(next.spotlight, Spotlight::Shape::Off); // un seul outil de page à la fois (F-591)
      }
      void spotlightPageUsesItsFrame()
      {
          PageToolsSettings p;
          p.tools = PageToolChoice::Spotlight;
          p.spotlight = Spotlight::Shape::Square;
          p.spotlightFrame = QRectF(10, 20, 200, 100);
          const auto next = resolvePageTools({true, Spotlight::Shape::Off}, p);
          QVERIFY(!next.curtain);
          QCOMPARE(next.spotlight, Spotlight::Shape::Square);
          QCOMPARE(next.spotlightFrame, QRectF(10, 20, 200, 100));
      }
      void spotlightPageWithoutShapeOpensACircle()
      {
          PageToolsSettings p;
          p.tools = PageToolChoice::Spotlight; // forme absente de la page
          const auto next = resolvePageTools({false, Spotlight::Shape::Off}, p);
          QCOMPARE(next.spotlight, Spotlight::Shape::Circle);
      }
  };

  QTEST_GUILESS_MAIN(TstFeaturesPageTools)
  #include "tst_features_pagetools.moc"
  ```
  Rouge.
- [ ] **Étape 6 : règle par page.** `features/include/actcraft/features/pagetools.h` :
  ```cpp
  #pragma once

  #include <actcraft/features/curtain.h>
  #include <actcraft/features/spotlight.h>

  #include <QRectF>

  #include <cstdint>

  namespace actcraft::features {

  // Outil de présentation d'une page (F-591). Type de features/, valeurs égales à core::PageToolsMode (socle) : la
  // phase 2 fournit la conversion depuis core::PagePresentation (addendum 1 : aucun type du socle recréé ici).
  enum class PageToolChoice : std::uint8_t { Keep = 0, Curtain = 1, Spotlight = 2, Off = 3 };

  // Réglages de présentation d'une page vus par les outils (F-591 à F-593).
  struct PageToolsSettings
  {
      PageToolChoice tools = PageToolChoice::Keep;
      Curtain::Start curtainStart = Curtain::Start::Full; // « complet » quand la page ne dit rien (F-592)
      double curtainDistance = 0;                         // pixels de page depuis le bord de départ
      Spotlight::Shape spotlight = Spotlight::Shape::Off; // Off sous le mode spot : rond (défaut à nous)
      QRectF spotlightFrame;                                   // vide : cadre par défaut
  };

  // État des outils au moment de quitter une page.
  struct PresentationState
  {
      bool curtain = false;
      Spotlight::Shape spotlight = Spotlight::Shape::Off;
  };

  // Ce que la page d'arrivée demande aux contrôleurs du rideau et du spot.
  struct PageToolsResult
  {
      bool curtain = false;
      bool resetCurtain = false; // vrai : refermer le store selon curtainStart et curtainDistance
      Curtain::Start curtainStart = Curtain::Start::Full;
      double curtainDistance = 0;
      Spotlight::Shape spotlight = Spotlight::Shape::Off;
      QRectF spotlightFrame; // vide : garder le cadre courant (comme avant) ou cadre par défaut
  };

  PageToolsResult resolvePageTools(PresentationState now, const PageToolsSettings& page);

  } // namespace actcraft::features
  ```
  `features/src/pagetools.cpp` :
  ```cpp
  #include <actcraft/features/pagetools.h>

  namespace actcraft::features {

  PageToolsResult resolvePageTools(PresentationState now, const PageToolsSettings& page)
  {
      PageToolsResult r;
      switch (page.tools) {
      case PageToolChoice::Keep: // F-591 « comme avant » : l'état de la page quittée continue
          r.curtain = now.curtain;
          r.spotlight = now.spotlight;
          break;
      case PageToolChoice::Curtain:
          r.curtain = true;
          r.resetCurtain = true;
          r.curtainStart = page.curtainStart;
          r.curtainDistance = page.curtainDistance;
          break;
      case PageToolChoice::Spotlight:
          r.spotlight = page.spotlight == Spotlight::Shape::Off ? Spotlight::Shape::Circle : page.spotlight;
          r.spotlightFrame = page.spotlightFrame;
          break;
      case PageToolChoice::Off:
          break;
      }
      return r;
  }

  } // namespace actcraft::features
  ```
  Commande ciblée `-R 'tst_features_(curtain|spotlight|pagetools)'` : trois `Totals` à `0 failed, 0 skipped`.
- [ ] **Étape 7 : preuves et commit.** `format.ps1`, preuve de tâche, `NOMS OK`, talons absents, commit
  « Lot 7, phase 1 : rideau, spot et règle des outils par page (T3, T4, T6) (L7-P1) ».

<a id="tache-l7-p2"></a>
### Tâche L7-P2 — Calculs du zoom de page, catalogue et règles des effets (T5, T7)

**Objectif.** Zoom de page continu en fonction du temps, panoramique, double-clic (F-260) ; catalogue des effets de
changement de page, effet global ou de la page quittée, bloqueurs, durée (F-640). Fonctions pures du temps, aucune
minuterie.

**Fichiers.** Ligne L7-P2 de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_features_(pagezoom|transitions)'`, preuve de tâche.

**Preuve visuelle.** Aucune.

**Interdits.** Règles communes ; ne pas inclure `ui/…/viewtransform.h` (bornes recopiées, la phase 2 vérifie leur
égalité par `static_assert` dans le contrôleur) ; libellés d'effets à nous.

**Terminé quand.** Deux tests verts sans saut, preuve, `NOMS OK`, commit.

**Étapes.**

- [ ] **Étape 1 : rouge du zoom.** `tests/tst_features_pagezoom.cpp` (montage du rideau, `#include
  <actcraft/features/pagezoom.h>`, `#include <cmath>`, classe `TstFeaturesPageZoom`) : les cinq fonctions de 7a,
  L7-T5, étape 1, mot pour mot (`holdZoomsInAroundThePoint`, `rightButtonZoomsOut`, `clampedToViewLimits`,
  `dragTurnsIntoPan`, `doubleClickFits`), plus :
  ```cpp
      void releaseFreezesTheZoom()
      {
          PageZoom z(1.0);
          z.press(QPointF(10, 10), PageZoom::Direction::In, 0);
          z.release(1'000'000'000);
          QCOMPARE(z.zoomAt(3'000'000'000), 2.0); // figé au relâché : 2 fois en une seconde
          QVERIFY(!z.pressed());
      }
  ```
  Rouge.
- [ ] **Étape 2 : zoom.** `features/include/actcraft/features/pagezoom.h` :
  ```cpp
  #pragma once

  #include <QPointF>

  #include <cstdint>

  namespace actcraft::features {

  // Zoom de page (F-260) : appui maintenu = zoom continu autour du point (gauche : avant ; droit : arrière), glisser =
  // panoramique (l'appui qui devient panoramique annule son zoom), double-clic = « Ajuster ». Temps en nanosecondes
  // fourni par l'appelant (core::monotonicNowNs()).
  class PageZoom
  {
  public:
      enum class Direction { In, Out };
      static constexpr double kRatePerSecond = 2.0;   // le zoom double en une seconde (valeur à nous ; L7-T0 peut la corriger)
      static constexpr double kMinZoom = 0.25;        // = ui::kMinZoom (ui/include/actcraft/ui/viewtransform.h)
      static constexpr double kMaxZoom = 8.0;         // = ui::kMaxZoom
      static constexpr double kPanThreshold = 8;      // pixels de vue avant qu'un appui devienne un panoramique
      static constexpr std::int64_t kDoubleClickNs = 400'000'000; // deux appuis en 400 ms au plus
      static constexpr double kDoubleClickPixels = 6;              // et à 6 pixels au plus

      explicit PageZoom(double startZoom);
      void setZoom(double zoom) noexcept; // zoom courant de la vue, lu avant chaque appui
      void press(QPointF viewPoint, Direction direction, std::int64_t nowNs);
      bool move(QPointF viewPoint, std::int64_t nowNs); // vrai quand l'appui est (ou devient) un panoramique
      QPointF panDelta() noexcept;                       // déplacement depuis le dernier appel ; remis à zéro
      void release(std::int64_t nowNs);
      double zoomAt(std::int64_t nowNs) const noexcept;  // borné à [kMinZoom, kMaxZoom]
      QPointF anchor() const noexcept;                   // point de vue fixe pendant le zoom
      bool panning() const noexcept;
      bool pressed() const noexcept;
      bool isDoubleClick(QPointF viewPoint, std::int64_t nowNs); // à appeler à chaque appui ; vrai au second

  private:
      double m_startZoom;
      double m_frozenZoom;
      Direction m_direction = Direction::In;
      QPointF m_anchor;
      QPointF m_lastPoint;
      QPointF m_panDelta;
      std::int64_t m_pressNs = 0;
      bool m_pressed = false;
      bool m_panning = false;
      bool m_hasLastClick = false;
      QPointF m_lastClickPoint;
      std::int64_t m_lastClickNs = 0;
  };

  } // namespace actcraft::features
  ```
  `features/src/pagezoom.cpp` : `zoomAt` = `m_frozenZoom` hors appui ou en panoramique ; pendant l'appui
  `clamp(m_startZoom × kRatePerSecond^(±(now − m_pressNs) / 1e9))` (+ pour `In`, − pour `Out`) ; `press` pose
  `m_startZoom = m_frozenZoom`, l'ancre, l'instant ; `move` : si distance à l'ancre > `kPanThreshold`, `m_panning =
  true` et `m_frozenZoom = m_startZoom` (zoom annulé) ; en panoramique, `m_panDelta += point − m_lastPoint` ; `release`
  : `m_frozenZoom = zoomAt(nowNs)` puis `m_pressed = false` ; `isDoubleClick` : vrai si le précédent appui est à
  ≤ `kDoubleClickNs` et ≤ `kDoubleClickPixels`, puis oublie ce précédent (un triple appui ne compte pas deux fois).
  Commande ciblée `-R tst_features_pagezoom` : `8 passed … 0 skipped`.
- [ ] **Étape 3 : rouge des effets.** `tests/tst_features_transitions.cpp` : les six fonctions de 7a, L7-T7, étape 1,
  avec les identifiants du catalogue de L7-T0 (`fade` → `dissolve`, `slide-left` → `slide-right`), et deux de plus :
  ```cpp
  #include <actcraft/features/transitions.h>

  #include <QRandomGenerator>
  #include <QSet>
  #include <QtTest>

  using namespace actcraft::features;

  class TstFeaturesTransitions : public QObject
  {
      Q_OBJECT

  private slots:
      void catalogueStartsWithNone()
      {
          const auto& list = transitionCatalogue();
          QVERIFY(list.size() >= 3);
          QCOMPARE(list.front().id, QStringLiteral("none"));
          for (const auto& e : list)
              QVERIFY(!e.label.isEmpty());
      }
      void catalogueHasSixteenEffectsPlusTwo()
      {
          // L7-T0 (docs/verifications/2026-10-18-lot7-principes.md) : 16 mouvements retenus (17 relevés, l'effet à nom
          // de marque écarté), plus « aucun » et « au hasard ».
          const auto& list = transitionCatalogue();
          QCOMPARE(list.size(), std::size_t{18});
          QCOMPARE(list[1].id, QStringLiteral("random"));
          QSet<QString> ids;
          for (const auto& e : list)
              ids.insert(e.id);
          QCOMPARE(ids.size(), qsizetype{18}); // identifiants uniques
      }
      void pageEffectWinsOverGlobal()
      {
          TransitionSettings s;
          s.global = QStringLiteral("dissolve");
          QCOMPARE(effectForLeaving(s, std::string("slide-right"), NavigationKind::Next, {}), QStringLiteral("slide-right"));
          QCOMPARE(effectForLeaving(s, std::string(), NavigationKind::Next, {}), QStringLiteral("dissolve"));
      }
      void effectPlayedWhenLeavingThePage()
      {
          TransitionSettings s; // global « none »
          // l'effet de la page QUITTÉE compte, pas celui de la page d'arrivée
          QCOMPARE(effectForLeaving(s, std::string("dissolve"), NavigationKind::Previous, {}), QStringLiteral("dissolve"));
      }
      void disabledWhileCurtainActive()
      {
          TransitionSettings s;
          s.global = QStringLiteral("dissolve");
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
      void randomPicksARealEffect()
      {
          QRandomGenerator generator(2026);
          for (int i = 0; i < 50; ++i) {
              const QString id = pickEffect(QStringLiteral("random"), generator);
              QVERIFY(id != QStringLiteral("random"));
              QVERIFY(id != QStringLiteral("none"));
              QVERIFY(id != QStringLiteral("does-not-exist"));
          }
          QCOMPARE(pickEffect(QStringLiteral("dissolve"), generator), QStringLiteral("dissolve")); // pas de tirage
      }
  };

  QTEST_GUILESS_MAIN(TstFeaturesTransitions)
  #include "tst_features_transitions.moc"
  ```
  Rouge.
- [ ] **Étape 4 : effets.** `features/include/actcraft/features/transitions.h` = l'en-tête de 7a, L7-T7, étape 2, mot
  pour mot, encadré par `#pragma once`, `#include <QRandomGenerator>`, `#include <QString>`, `#include <string>`,
  `#include <vector>` et `namespace actcraft::features { … }`, plus :
  ```cpp
  // « random » : un effet du catalogue tiré à chaque changement, hors « none » et « random » ; tout autre identifiant
  // est rendu tel quel. Appelé par le lecteur (phase 2) sur le résultat d'effectForLeaving.
  QString pickEffect(const QString& id, QRandomGenerator& generator);
  ```
  `features/src/transitions.cpp` :
  ```cpp
  #include <actcraft/features/transitions.h>

  #include <algorithm>

  namespace actcraft::features {

  const std::vector<TransitionEffect>& transitionCatalogue()
  {
      // Liste de L7-T0 (docs/verifications/2026-10-18-lot7-principes.md, N2-A6) : 16 mouvements retenus (17 relevés,
      // l'effet à nom de marque écarté), plus « aucun » et « au hasard » ; identifiants et noms à nous. « none »
      // toujours en premier, « random » ensuite : 18 entrées.
      static const std::vector<TransitionEffect> catalogue{
          {QStringLiteral("none"), QStringLiteral("Sans effet")},
          {QStringLiteral("random"), QStringLiteral("Au hasard")},
          {QStringLiteral("dissolve"), QStringLiteral("Fondu")},
          {QStringLiteral("corner-peel"), QStringLiteral("Coin soulevé")},
          {QStringLiteral("box-turn"), QStringLiteral("Boîte qui tourne")},
          {QStringLiteral("card-flip"), QStringLiteral("Carte retournée")},
          {QStringLiteral("tiles-spin"), QStringLiteral("Carreaux")},
          {QStringLiteral("fold-half"), QStringLiteral("Pliage")},
          {QStringLiteral("slide-right"), QStringLiteral("Glissement")},
          {QStringLiteral("strips-drop"), QStringLiteral("Bandes")},
          {QStringLiteral("corner-swing"), QStringLiteral("Pivot d'angle")},
          {QStringLiteral("spiral-out"), QStringLiteral("Tourbillon")},
          {QStringLiteral("turn-reveal"), QStringLiteral("Dévoilement tournant")},
          {QStringLiteral("drapes"), QStringLiteral("Tentures")},
          {QStringLiteral("star-iris"), QStringLiteral("Étoile")},
          {QStringLiteral("shards"), QStringLiteral("Éclats")},
          {QStringLiteral("sweep"), QStringLiteral("Balayage")},
          {QStringLiteral("funnel"), QStringLiteral("Entonnoir")},
      };
      return catalogue;
  }

  QString effectForLeaving(const TransitionSettings& settings, const std::string& leavingPageEffect,
                           NavigationKind navigation, TransitionBlockers blockers)
  {
      const QString none = QStringLiteral("none");
      if (navigation == NavigationKind::Jump || blockers.curtainOrSpotlight || blockers.desktopAnnotation ||
          blockers.designMode)
          return none;
      // L'effet de la page QUITTÉE prime sur l'effet global (F-640, confirmé par L7-T0).
      const QString id = leavingPageEffect.empty() ? settings.global : QString::fromStdString(leavingPageEffect);
      const auto& list = transitionCatalogue();
      const bool known =
          std::any_of(list.begin(), list.end(), [&id](const TransitionEffect& e) { return e.id == id; });
      return known ? id : none;
  }

  QString pickEffect(const QString& id, QRandomGenerator& generator)
  {
      if (id != QStringLiteral("random"))
          return id;
      const auto& list = transitionCatalogue(); // [0] none, [1] random, puis les 16 mouvements
      const auto index = 2 + static_cast<std::size_t>(generator.bounded(static_cast<quint32>(list.size() - 2)));
      return list[index].id;
  }

  int clampDurationTenths(int tenths) noexcept
  {
      return std::clamp(tenths, 5, 30); // 0,5 à 3 s (F-640, confirmé par L7-T0)
  }

  } // namespace actcraft::features
  ```
  Commande ciblée `-R 'tst_features_(pagezoom|transitions)'` : deux `Totals` à `0 failed, 0 skipped`.
- [ ] **Étape 5 : preuves et commit.** `format.ps1`, preuve de tâche, `NOMS OK`, talons absents, commit
  « Lot 7, phase 1 : calculs du zoom de page, catalogue et règles des effets (T5, T7) (L7-P2) ».

<a id="tache-l7-p3"></a>
### Tâche L7-P3 — Guides de tracé, règle et équerre (T9, T10)

**Objectif.** Le contrat des guides (`StrokeGuide`, `GuideSet`) que la phase 2 branchera sur
`PageView::setSampleFilter` : à l'appui, le guide le plus proche (à la tolérance près) saisit le trait ; il le garde
jusqu'au relâché, même loin ; un nouvel appui loin est libre. La règle (deux bords) et l'équerre (trois bords)
projettent sur leurs segments par une fonction partagée.

**Fichiers.** Ligne L7-P3 de la table.

**Test et commande bornée.** Commande ciblée `-R tst_features_mathtools`, preuve de tâche.

**Preuve visuelle.** Aucune.

**Interdits.** Règles communes. Patron à **ne pas** reprendre : un guide qui se re-saisit à chaque échantillon
(le trait sauterait d'un bord à l'autre) ; un aperçu (`snapPreview`) qui change le bord saisi pendant un trait.
`GuideSet` ne modifie que `x` et `y` de l'échantillon (pression, instant, appareil, phase, `seq` gardés).

**Terminé quand.** `Totals: 13 passed, 0 failed, 0 skipped`, preuve, `NOMS OK`, commit.

**Étapes.**

- [ ] **Étape 1 : rouge.** `tests/tst_features_mathtools.cpp` = le fichier de 7a, L7-T9, étape 1, mot pour mot (aide
  `at`, `rulerSnapsNearEdge`, `rulerIgnoresFarSamples`, `strokeStaysOnTheEdgeOnceSnapped`, `rotatedRulerProjects`,
  `angleDisplayedInWholeDegrees`, `centimetreScale`), plus, dans la classe :
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
      void guideSetPicksTheNearestGuide()
      {
          Ruler r;
          r.setFrame(QPointF(500, 440), 600, 80, 0); // bord haut à y = 400
          SetSquare q;
          q.setFrame(QPointF(300, 410), 300, 0);    // côté horizontal à y = 410
          GuideSet guides;
          guides.add(&r);
          guides.add(&q);
          guides.setTolerance(16);
          QCOMPARE(guides.filter(at(350, 402, input::Phase::Press)).y, 400.0); // 2 px de la règle, 8 de l'équerre
          QVERIFY(guides.guiding());
          QCOMPARE(guides.filter(at(360, 412, input::Phase::Move)).y, 400.0);  // garde la règle
          guides.filter(at(370, 400, input::Phase::Release));
          QVERIFY(!guides.guiding());
      }
      void snapPreviewIsQuietWhileGuiding()
      {
          Ruler r;
          r.setFrame(QPointF(500, 440), 600, 80, 0);
          GuideSet guides;
          guides.add(&r);
          guides.setTolerance(16);
          QVERIFY(guides.snapPreview(at(300, 405, input::Phase::Move)).has_value()); // survol : cercle d'accroche
          guides.filter(at(300, 405, input::Phase::Press));
          QVERIFY(!guides.snapPreview(at(300, 475, input::Phase::Move)).has_value()); // pendant le trait : rien
          QCOMPARE(guides.filter(at(400, 470, input::Phase::Move)).y, 400.0);        // toujours le bord haut
      }
      void sampleKeepsItsOtherFields()
      {
          Ruler r;
          r.setFrame(QPointF(500, 440), 600, 80, 0);
          GuideSet guides;
          guides.add(&r);
          auto s = at(300, 405, input::Phase::Press);
          s.pressure = 0.25f;
          s.timestampNs = 42;
          s.seq = 7;
          const auto out = guides.filter(s);
          QCOMPARE(out.pressure, 0.25f);
          QCOMPARE(out.timestampNs, std::int64_t{42});
          QCOMPARE(out.seq, std::uint64_t{7});
          QCOMPARE(out.phase, input::Phase::Press);
      }
  ```
  (11 fonctions, donc `Totals: 13`.) Rouge.
- [ ] **Étape 2 : guides.** `features/include/actcraft/features/strokeguide.h` = l'en-tête de 7a, L7-T9, étape 2, mot
  pour mot, avec ce contrat de `snapPreview` en commentaire : « rien pendant un trait guidé ; ne change jamais le
  guide saisi ». `features/src/strokeguide.cpp` :
  ```cpp
  #include <actcraft/features/strokeguide.h>

  #include <algorithm>
  #include <cmath>

  namespace actcraft::features {

  void GuideSet::add(StrokeGuide* guide)
  {
      if (guide && std::find(m_guides.begin(), m_guides.end(), guide) == m_guides.end())
          m_guides.push_back(guide);
  }

  void GuideSet::remove(StrokeGuide* guide)
  {
      m_guides.erase(std::remove(m_guides.begin(), m_guides.end(), guide), m_guides.end());
      if (m_active == guide)
          m_active = nullptr;
  }

  void GuideSet::setTolerance(double pagePixels) noexcept
  {
      m_tolerance = pagePixels;
  }

  input::Sample GuideSet::filter(const input::Sample& sample)
  {
      switch (sample.phase) {
      case input::Phase::Press: {
          m_active = nullptr;
          double best = 0;
          input::Sample result = sample;
          for (StrokeGuide* guide : m_guides) {
              const auto snapped = guide->capture(sample, m_tolerance);
              if (!snapped)
                  continue;
              const double d = std::hypot(snapped->x - sample.x, snapped->y - sample.y);
              if (!m_active || d < best) {
                  m_active = guide;
                  best = d;
                  result = *snapped;
              }
          }
          return result; // chaque guide garde le bord de son propre capture() : rien à re-saisir
      }
      case input::Phase::Move:
          return m_active ? m_active->follow(sample) : sample;
      case input::Phase::Release: {
          const input::Sample out = m_active ? m_active->follow(sample) : sample;
          m_active = nullptr;
          return out;
      }
      }
      return sample;
  }

  bool GuideSet::guiding() const noexcept
  {
      return m_active != nullptr;
  }

  std::optional<input::Sample> GuideSet::snapPreview(const input::Sample& hover)
  {
      if (m_active)
          return std::nullopt;
      std::optional<input::Sample> best;
      double bestDistance = 0;
      for (StrokeGuide* guide : m_guides) {
          const auto snapped = guide->capture(hover, m_tolerance);
          if (!snapped)
              continue;
          const double d = std::hypot(snapped->x - hover.x, snapped->y - hover.y);
          if (!best || d < bestDistance) {
              best = snapped;
              bestDistance = d;
          }
      }
      return best;
  }

  } // namespace actcraft::features
  ```
  (Chaque guide mémorise le bord saisi par son propre `capture` ; seul `m_active` est suivi ensuite.)
- [ ] **Étape 3 : règle et équerre.** `features/include/actcraft/features/mathtools.h` :
  ```cpp
  #pragma once

  #include <actcraft/features/strokeguide.h>

  #include <QLineF>
  #include <QPointF>

  #include <optional>
  #include <vector>

  namespace actcraft::features {

  // Échelle des graduations : 96 pixels de page par pouce (référence de l'ancien dépôt, reformulée).
  inline constexpr double kPixelsPerCentimetre = 96.0 / 2.54;
  inline constexpr double kPixelsPerInch = 96.0;
  // Translucidité par défaut des outils de maths, sur 255 (lue par les délégués QML, phase 2) : 100, réglage embarqué
  // relevé par L7-T0 (docs/verifications/2026-10-18-lot7-principes.md), et non 255.
  inline constexpr int kDefaultToolOpacity = 100;

  enum class RulerUnit { Cm, Mm, Inch, None };

  // Projection bornée de `point` sur le segment le plus proche de `segments` ; index -1 si la liste est vide.
  struct SegmentHit
  {
      int index = -1;
      QPointF point;
      double distance = 0;
  };
  SegmentHit projectOnSegments(const std::vector<QLineF>& segments, QPointF point);

  // Règle (F-280) : rectangle tourné ; ses deux grands bords guident le stylo.
  class Ruler final : public StrokeGuide
  {
  public:
      void setFrame(QPointF center, double length, double width, double angleDegrees);
      QPointF center() const noexcept;
      double length() const noexcept;
      double width() const noexcept;
      double angleDegrees() const noexcept;
      std::vector<QLineF> edges() const; // [0] bord haut, [1] bord bas (à l'angle 0)
      int displayAngle() const noexcept; // degrés entiers, 0 à 359
      void setUnits(RulerUnit top, RulerUnit bottom) noexcept;
      RulerUnit topUnit() const noexcept;
      RulerUnit bottomUnit() const noexcept;
      std::optional<input::Sample> capture(const input::Sample& sample, double tolerance) override;
      input::Sample follow(const input::Sample& sample) override;

  private:
      QPointF m_center{0, 0};
      double m_length = 600; // défaut d'ouverture (7a, L7-T9)
      double m_width = 80;
      double m_angle = 0;
      RulerUnit m_topUnit = RulerUnit::Cm;
      RulerUnit m_bottomUnit = RulerUnit::Inch;
      int m_edge = -1; // bord saisi au dernier capture() réussi
  };

  // Équerre (F-283) : triangle rectangle isocèle ; côtés de l'angle droit vers +x et +y depuis le sommet à l'angle 0.
  class SetSquare final : public StrokeGuide
  {
  public:
      void setFrame(QPointF rightAngleVertex, double side, double angleDegrees);
      std::vector<QLineF> edges() const; // [0] côté +x, [1] côté +y, [2] hypoténuse
      int displayAngle() const noexcept;
      std::optional<input::Sample> capture(const input::Sample& sample, double tolerance) override;
      input::Sample follow(const input::Sample& sample) override;

  private:
      QPointF m_vertex{0, 0};
      double m_side = 300;
      double m_angle = 0;
      int m_edge = -1;
  };

  } // namespace actcraft::features
  ```
  `features/src/mathtools.cpp` : fonction interne `rotated(QPointF local, QPointF origin, double degrees)` (repère de
  page, y vers le bas : `x' = x cos − y sin`, `y' = x sin + y cos`) ; `Ruler::edges()` : coins locaux
  `(±length/2, −width/2)` et `(±length/2, +width/2)` tournés autour du centre ; `SetSquare::edges()` : sommet,
  `vertex + side·u`, `vertex + side·n` (`u` = direction de l'angle, `n` = `u` tourné de +90°) ; `projectOnSegments` :
  projection bornée (paramètre `t` borné à [0, 1]) sur chaque segment, la plus proche gagne ; `capture` : `hit =
  projectOnSegments(edges(), {x, y})`, rien si `hit.index < 0` ou `hit.distance > tolerance`, sinon `m_edge =
  hit.index` et l'échantillon copié avec `x`, `y` projetés ; `follow` : projection bornée sur `edges()[m_edge]` (ou
  l'échantillon inchangé si `m_edge < 0`) ; `displayAngle` : `((qRound(angle) % 360) + 360) % 360`. Commande ciblée
  `-R tst_features_mathtools` : `Totals: 13 passed, 0 failed, 0 skipped`.
- [ ] **Étape 4 : preuves et commit.** `format.ps1`, preuve de tâche, `NOMS OK`, talon absent, commit
  « Lot 7, phase 1 : guides de tracé, règle et équerre (T9, T10) (L7-P3) ».

<a id="tache-l7-p4"></a>
### Tâche L7-P4 — Rapporteur, compas et mémoire de l'origine XY (T11, T12, T13)

**Objectif.** Rapporteur complet ou demi dont le pourtour guide un arc parfait (angle balayé lu sans saut à ±180°),
tracés d'arc (seul, fermé, plein) ; compas dont la mine guide un arc ou un cercle fermé exactement ; mémoire de
l'origine XY par page.

**Fichiers.** Ligne L7-P4 de la table (fichiers de L7-P3 : partir de `feat/livraison-complete-b` **après** le merge de
L7-P3).

**Test et commande bornée.** Commande ciblée `-R tst_features_mathtools`, preuve de tâche.

**Preuve visuelle.** Aucune.

**Interdits.** Règles communes ; ne pas modifier le comportement de `Ruler`, `SetSquare`, `GuideSet` ni leurs tests
(ajouts seulement). Patron à **ne pas** reprendre : l'angle balayé calculé par différence d'`atan2` brute (saut de
360° au passage de ±180°) ; un `sweptDegrees` qui compte le sens (le test attend une valeur positive).

**Terminé quand.** `Totals: 22 passed, 0 failed, 0 skipped` (20 fonctions + 2), preuve, `NOMS OK`, commit.

**Étapes.**

- [ ] **Étape 1 : rouge.** Ajouter à `tests/tst_features_mathtools.cpp` (en-têtes `<cmath>`, `<QtMath>`) les
  fonctions de 7a, L7-T11, étape 1 (`protractorArcFollowsTheRim`, `halfProtractorStopsAt180`,
  `closedArcAddsTwoRadii`) et L7-T12, étape 1 (`compassDrawsAnArcAtItsRadius`, `fullTurnClosesTheCircle`,
  `radiusInCentimetres`), mot pour mot, plus :
  ```cpp
      void filledArcIsClosedOnTheCentre()
      {
          const auto path = arcPath(QPointF(10, 20), 50, 0, 90, ArcStyle::Filled);
          QCOMPARE(path.front(), QPointF(10, 20));
          QCOMPARE(path.back(), QPointF(10, 20));
          QCOMPARE(path.size(), std::size_t{48}); // centre + 46 points de l'arc (pas de 2°) + centre
      }
      void compassClosesOnItsFirstPoint()
      {
          Compass c;
          c.setPivot(QPointF(0, 0));
          c.setRadius(100);
          const auto first = c.capture(at(100, 0, input::Phase::Press), 16);
          QVERIFY(first.has_value());
          input::Sample last;
          for (int d = 10; d <= 370; d += 10)
              last = c.follow(at(100 * std::cos(qDegreesToRadians(double(d))), 100 * std::sin(qDegreesToRadians(double(d))), input::Phase::Move));
          QVERIFY(c.closedCircle());
          QCOMPARE(last.x, first->x); // fermé exactement sur le premier point
          QCOMPARE(last.y, first->y);
      }
      void originRememberedPerPage()
      {
          OriginMemory m(QSizeF(1000, 800));
          m.set(0, QPointF(100, 100));
          m.set(1, QPointF(300, 200));
          QCOMPARE(m.at(0), QPointF(100, 100));
          QCOMPARE(m.at(1), QPointF(300, 200));
          QVERIFY(!m.has(2));
          QCOMPARE(m.at(2), QPointF(500, 400)); // défaut : centre de la page
      }
  ```
  Rouge.
- [ ] **Étape 2 : rapporteur et arcs.** Ajouter à `mathtools.h` :
  ```cpp
  enum class ArcStyle { Arc, Closed, Filled };
  // Points d'un arc, pas de 2° (au moins un pas), angles en degrés dans le repère de page (y vers le bas).
  // Closed et Filled : le centre en premier et en dernier (deux rayons) ; Filled se remplit tel quel.
  std::vector<QPointF> arcPath(QPointF center, double radius, double startDegrees, double sweepDegrees, ArcStyle style);

  // Rapporteur (F-281) : le pourtour guide le stylo ; demi : seulement le demi-cercle « au-dessus » du diamètre
  // (y local ≤ 0 à l'angle 0).
  class Protractor final : public StrokeGuide
  {
  public:
      enum class Kind { Full, Half };
      void setFrame(QPointF center, double radius, double angleDegrees, Kind kind);
      double sweptDegrees() const noexcept;     // valeur absolue de l'angle cumulé depuis la saisie
      double startDegrees() const noexcept;     // angle de page du point saisi
      std::optional<input::Sample> capture(const input::Sample& sample, double tolerance) override;
      input::Sample follow(const input::Sample& sample) override;

  private:
      QPointF m_center{0, 0};
      double m_radius = 200;
      double m_angle = 0;
      Kind m_kind = Kind::Full;
      double m_startDegrees = 0;
      double m_lastDegrees = 0;
      double m_swept = 0;
  };
  ```
  `mathtools.cpp` : fonction interne `unwrapDelta(double from, double to)` qui rend `to − from` ramené dans
  ]−180, 180] ; `capture` : distance `|‖p − c‖ − r| ≤ tolerance`, et pour `Half` angle local (angle de page −
  `m_angle`) dans [−180, 0] ; rend la projection `c + r·(p − c)/‖p − c‖` ; remet `m_swept = 0`, `m_startDegrees =
  m_lastDegrees` = angle de la projection ; `follow` : angle du point (pour `Half`, borné à [−180, 0] en local),
  `m_swept += unwrapDelta(m_lastDegrees, angle)`, projection sur le cercle ; `sweptDegrees` = `std::abs(m_swept)` ;
  `arcPath` : `n = max(1, ceil(|sweep| / 2))` pas, points `c + r·(cos, sin)` de `start` à `start + sweep` inclus.
- [ ] **Étape 3 : compas.** Ajouter à `mathtools.h` :
  ```cpp
  // Compas (F-282) : la mine (pointe + rayon à l'angle courant) saisit le stylo ; le trait suit le cercle de rayon
  // courant ; au-delà d'un tour, il se ferme exactement sur son premier point.
  class Compass final : public StrokeGuide
  {
  public:
      static constexpr double kMinRadius = 20;
      static constexpr double kMaxRadius = 2000;
      void setPivot(QPointF pivot) noexcept;
      void setRadius(double radius) noexcept; // borné à [kMinRadius, kMaxRadius]
      void setAngle(double degrees) noexcept;
      void setMirrored(bool mirrored) noexcept;
      bool mirrored() const noexcept;
      QPointF pivot() const noexcept;
      double radius() const noexcept;
      QPointF pencil() const noexcept;         // position de la mine
      double sweptDegrees() const noexcept;    // valeur absolue de l'angle cumulé
      bool closedCircle() const noexcept;      // balayage ≥ 360°
      QString radiusText() const;              // « 3,0 cm » : une décimale, virgule française
      std::optional<input::Sample> capture(const input::Sample& sample, double tolerance) override;
      input::Sample follow(const input::Sample& sample) override;

  private:
      QPointF m_pivot{0, 0};
      double m_radius = 150;
      double m_angle = 0;
      bool m_mirrored = false;
      double m_lastDegrees = 0;
      double m_swept = 0;
      QPointF m_first;
  };
  ```
  (`#include <QString>` dans l'en-tête.) `capture` : distance à `pencil()` ≤ tolerance → projection sur le cercle,
  `m_first` = cette projection, `m_swept = 0` ; `follow` : `m_swept += unwrapDelta(...)`, la mine suit (`m_angle` =
  angle du point) ; si `closedCircle()`, rend `m_first` (copié dans l'échantillon) ; `radiusText` :
  `QLocale(QLocale::French).toString(m_radius / kPixelsPerCentimetre, 'f', 1) + QStringLiteral(" cm")`.
- [ ] **Étape 4 : mémoire de l'origine.** Ajouter à `mathtools.h` (`#include <QSizeF>`, `<cstddef>`, `<map>`) :
  ```cpp
  // Origine XY (F-284) par page ; défaut : centre de la page. La phase 2 la remplit depuis le modèle de la page.
  class OriginMemory
  {
  public:
      explicit OriginMemory(QSizeF pageSize);
      void setPageSize(QSizeF pageSize) noexcept;
      void set(std::size_t page, QPointF origin);
      void forget(std::size_t page);
      bool has(std::size_t page) const;
      QPointF at(std::size_t page) const;

  private:
      QSizeF m_size; // taille de la page (nom court : la forme longue est refusée par le contrôle des noms)
      std::map<std::size_t, QPointF> m_origins;
  };
  ```
  Corps évidents dans `mathtools.cpp`. Commande ciblée `-R tst_features_mathtools` : `22 passed, 0 failed, 0 skipped`
  (20 fonctions + 2).
- [ ] **Étape 5 : preuves et commit.** `format.ps1`, preuve de tâche, `NOMS OK`, commit
  « Lot 7, phase 1 : rapporteur, compas et origine XY par page (T11 à T13) (L7-P4) ».

<a id="tache-l7-p5"></a>
### Tâche L7-P5 — Dés, calculatrice et compte à rebours (T14, T15, T16)

**Objectif.** Dés reproductibles par graine (F-285, E5) ; calculatrice de poche à nous (F-286) ; compte à rebours,
répétition, pause et minuterie (F-290, E9), fonctions pures du temps.

**Fichiers.** Ligne L7-P5 de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_features_(dice|calculator|countdown)'`, preuve de tâche.

**Preuve visuelle.** Aucune.

**Interdits.** Règles communes ; aucun programme lancé (le lanceur de la calculatrice du système est en phase 2) ;
aucune minuterie ni horloge lue dans la logique (instant fourni par l'appelant).

**Terminé quand.** Trois tests verts sans saut, preuve, `NOMS OK`, commit.

**Étapes.**

- [ ] **Étape 1 : rouge des dés.** `tests/tst_features_dice.cpp` (montage, `#include <actcraft/features/dice.h>`,
  classe `TstFeaturesDice`) : les quatre fonctions de 7a, L7-T14, étape 1, mot pour mot, plus :
  ```cpp
      void defaultIsOneDie()
      {
          // E5, L7-T0 (docs/verifications/2026-10-18-lot7-principes.md) : 1 dé au premier lancement ; vitesse 7 s.
          const Dice d(3);
          QCOMPARE(d.count(), 1);
          QCOMPARE(Dice::kDefaultSeconds, 7);
      }
  ```
  Rouge.
- [ ] **Étape 2 : dés.** `features/include/actcraft/features/dice.h` :
  ```cpp
  #pragma once

  #include <QRandomGenerator>

  #include <vector>

  namespace actcraft::features {

  // Dés (F-285) : 1 à 5 dés à six faces, lancer reproductible pour une graine donnée.
  class Dice
  {
  public:
      static constexpr int kMinCount = 1;
      static constexpr int kMaxCount = 5;
      // E5, tranché par L7-T0 (docs/verifications/2026-10-18-lot7-principes.md) : 1 dé au premier lancement ; ensuite
      // le dernier nombre choisi, mémorisé dans les réglages par le contrôleur (phase 2). Vitesse par défaut : 7 s.
      static constexpr int kDefaultCount = 1;
      static constexpr int kDefaultSeconds = 7;

      explicit Dice(quint32 seed);
      void setCount(int count) noexcept; // borné à [kMinCount, kMaxCount]
      int count() const noexcept;
      std::vector<int> roll();           // une face de 1 à 6 par dé
      const std::vector<int>& faces() const noexcept;
      int total() const noexcept;        // somme du dernier lancer
      static int animationMs(int seconds) noexcept; // vitesse : 3 à 11 s

  private:
      QRandomGenerator m_generator;
      int m_count = kDefaultCount;
      std::vector<int> m_faces;
  };

  } // namespace actcraft::features
  ```
  `features/src/dice.cpp` : `roll` remplit `m_count` faces par `static_cast<int>(m_generator.bounded(1, 7))` ;
  `total` = somme ; `animationMs` = `std::clamp(seconds, 3, 11) * 1000`. Commande ciblée `-R tst_features_dice` :
  `7 passed … 0 skipped`.
- [ ] **Étape 3 : rouge de la calculatrice.** `tests/tst_features_calculator.cpp` (montage, classe
  `TstFeaturesCalculator`) : les cinq fonctions de 7a, L7-T15, étape 1, mot pour mot, plus :
  ```cpp
      void percentOfASum()
      {
          Calculator c;
          for (const char* k : {"2", "0", "0", "+", "5", "%"})
              c.press(QString::fromLatin1(k));
          QCOMPARE(c.display(), QStringLiteral("210")); // 200 + 5 % de 200
      }
      // P1, P2, P3 de L7-T0 (docs/verifications/2026-10-18-lot7-principes.md) : calcul affiché au-dessus du résultat,
      // deux effacements, calcul et résultat posés sur la page.
      void expressionShownAboveTheResult()
      {
          Calculator c;
          for (const char* k : {"1", "2", "+", "3"})
              c.press(QString::fromLatin1(k));
          QCOMPARE(c.expression(), QStringLiteral("12 +"));
          QCOMPARE(c.display(), QStringLiteral("3"));
          c.press(QStringLiteral("*"));
          QCOMPARE(c.expression(), QStringLiteral("12 + 3 ×"));
          c.press(QStringLiteral("/")); // deux opérations de suite : la seconde remplace la première
          QCOMPARE(c.expression(), QStringLiteral("12 + 3 ÷"));
      }
      void clearEntryKeepsTheCalculation()
      {
          Calculator c;
          for (const char* k : {"2", "+", "5", "CE", "3", "="})
              c.press(QString::fromLatin1(k));
          QCOMPARE(c.display(), QStringLiteral("5"));
          QCOMPARE(c.expression(), QStringLiteral("2 + 3 ="));
          c.press(QStringLiteral("C"));
          QCOMPARE(c.expression(), QString());
          QCOMPARE(c.display(), QStringLiteral("0"));
      }
      void pageTextHasTheCalculationAndItsResult()
      {
          Calculator c;
          QCOMPARE(c.pageText(), QStringLiteral("0"));
          for (const char* k : {"1", ",", "5", "+", "1", "="})
              c.press(QString::fromLatin1(k));
          QCOMPARE(c.pageText(), QStringLiteral("1,5 + 1 = 2,5"));
          c.press(QStringLiteral("4")); // nouveau calcul
          QCOMPARE(c.pageText(), QStringLiteral("4"));
      }
  ```
  Rouge.
- [ ] **Étape 4 : calculatrice.** `features/include/actcraft/features/calculator.h` :
  ```cpp
  #pragma once

  #include <QString>

  namespace actcraft::features {

  // Calculatrice de poche (F-286) : calcul immédiat de gauche à droite, virgule française, 12 chiffres significatifs.
  // Touches : "0"…"9", ",", "+", "-", "*", "/", "=", "%", "±", "C" (tout le calcul), "CE" (dernière saisie),
  // "M+", "M-", "MR", "MC". Principes P1 à P3 de L7-T0 (docs/verifications/2026-10-18-lot7-principes.md).
  class Calculator
  {
  public:
      void press(const QString& key);
      QString display() const;    // « Erreur » après une division par zéro ou un résultat non fini, jusqu'à « C »
      QString expression() const; // calcul en cours, montré au-dessus du résultat (P1) : « 12 + 3 × », « 2 + 3 = »
      QString pageText() const;   // texte posé sur la page (P3) : « 1,5 + 1 = 2,5 » après « = », sinon l'affichage

  private:
      double entry() const;
      void showValue(double value);
      void apply();            // m_accumulator = m_accumulator <m_pending> entry()

      QString m_expression;    // jetons séparés par une espace ; opérateurs affichés + − × ÷
      bool m_done = false;     // vrai juste après « = » ou « % »
      QString m_display = QStringLiteral("0");
      double m_accumulator = 0;
      QChar m_pending;         // nul : aucune opération en attente
      bool m_newEntry = true;  // vrai : le prochain chiffre remplace l'affichage
      bool m_error = false;
      double m_memory = 0;
  };

  } // namespace actcraft::features
  ```
  `features/src/calculator.cpp`, règles : en erreur, seule `C` agit ; chiffre : remplace l'affichage si
  `m_newEntry` (« , » donne « 0, »), sinon s'ajoute (12 chiffres au plus, « 0 » de tête remplacé, une seule virgule) ;
  opération : si une opération attend et que `!m_newEntry`, `apply()` et affichage du résultat ; si aucune n'attend,
  `m_accumulator = entry()` ; puis `m_pending = op`, `m_newEntry = true` ; « = » : `apply()` si une opération
  attend, puis plus d'opération ; « % » : avec `*` → `acc × e / 100`, `/` → `acc / (e / 100)`, `+` → `acc + acc × e / 100`,
  `-` → `acc − acc × e / 100`, sans opération → `e / 100` ; résultat affiché, plus d'opération ; « ± » : signe de
  l'affichage ; « C » : tout remis à zéro sauf la mémoire ; « M+ », « M- » : mémoire ± `entry()`, `m_newEntry = true` ;
  « MR » : affiche la mémoire ; « MC » : mémoire à 0. `showValue` : non fini → « Erreur » ; sinon
  `QString::number(value, 'g', 12)` avec « . » remplacé par « , » ; `entry()` : l'inverse. Calcul affiché : une
  opération ajoute à `m_expression` l'affichage puis l'opérateur (`*` → « × », `/` → « ÷ », `-` → « − ») ; deux
  opérations de suite remplacent le dernier opérateur ; l'affichage ajouté est celui d'**avant** `apply()` ; « = »
  ajoute l'affichage puis « = » et pose `m_done` ; « % » ajoute l'affichage puis « % = » et pose `m_done` ; un chiffre après `m_done` vide `m_expression` (nouveau calcul) ;
  « CE » remet l'affichage à « 0 » (`m_newEntry = true`) sans toucher l'accumulateur, l'opération en attente ni
  `m_expression` ; « C » vide aussi `m_expression`. `pageText()` : `m_done` → `m_expression + " " + m_display`, sinon
  `m_display`. « = » remet aussi `m_newEntry` à vrai (le chiffre suivant commence un nouveau calcul : « 4 », jamais
  « 2,54 »). Commande ciblée `-R tst_features_calculator` : `11 passed … 0 skipped`.
- [ ] **Étape 5 : rouge du compte à rebours.** `tests/tst_features_countdown.cpp` (montage, classe
  `TstFeaturesCountdown`) : les cinq fonctions de 7a, L7-T16, étape 1, mot pour mot, plus :
  ```cpp
      void latePollFiresOnce()
      {
          Countdown c;
          c.setRepeat(true);
          c.start(2, 0);
          QVERIFY(c.poll(7'000'000'000));  // trois fins manquées : une seule annoncée
          QVERIFY(!c.poll(7'100'000'000));
          QCOMPARE(c.remainingSeconds(7'000'000'000), 1); // prochaine fin à 8 s
      }
  ```
  Rouge.
- [ ] **Étape 6 : compte à rebours.** `features/include/actcraft/features/countdown.h` :
  ```cpp
  #pragma once

  #include <QString>

  #include <cstdint>

  namespace actcraft::features {

  // Compte à rebours (F-290) : temps en nanosecondes fourni par l'appelant.
  class Countdown
  {
  public:
      void start(int seconds, std::int64_t nowNs);
      void pause(std::int64_t nowNs);
      void resume(std::int64_t nowNs);
      void reset(std::int64_t nowNs); // repart de la durée complète, sans changer pause ni répétition
      void setRepeat(bool repeat) noexcept;
      bool repeat() const noexcept;
      bool running() const noexcept;
      bool paused() const noexcept;
      int remainingSeconds(std::int64_t nowNs) const noexcept; // arrondi au supérieur ; 0 une fois fini
      bool poll(std::int64_t nowNs); // vrai une fois par fin ; en répétition, repart de l'échéance prévue

  private:
      std::int64_t m_durationNs = 0;
      std::int64_t m_endNs = 0;
      std::int64_t m_pausedRemainingNs = 0;
      bool m_running = false;
      bool m_paused = false;
      bool m_repeat = false;
  };

  // Minuterie (compte en avant).
  class Stopwatch
  {
  public:
      void start(std::int64_t nowNs);
      void pause(std::int64_t nowNs);
      void resume(std::int64_t nowNs);
      int elapsedSeconds(std::int64_t nowNs) const noexcept; // arrondi à l'inférieur

  private:
      std::int64_t m_startNs = 0;
      std::int64_t m_pausedElapsedNs = 0;
      bool m_running = false;
      bool m_paused = false;
  };

  QString formatHms(int seconds);    // « hh:mm:ss »
  int parseHms(const QString& text); // « hh:mm:ss » (heures sur 1 ou 2 chiffres) ; -1 si invalide

  } // namespace actcraft::features
  ```
  `features/src/countdown.cpp` : `poll` : faux si `!m_running` ou en pause ou `now <
  m_endNs` ; sinon, en répétition, `m_endNs += m_durationNs` tant que `m_endNs <= now`, sinon `m_running = false` ;
  rend vrai ; `remainingSeconds` : reste = (en pause : `m_pausedRemainingNs`, sinon `max(0, m_endNs − now)`), arrondi
  au supérieur par `(reste + 999'999'999) / 1'000'000'000` converti en `int` ; `parseHms` par
  `QRegularExpression(QStringLiteral("^(\\d{1,2}):([0-5]\\d):([0-5]\\d)$"))`. Commande ciblée
  `-R 'tst_features_(dice|calculator|countdown)'` : trois `Totals` à `0 failed, 0 skipped`.
- [ ] **Étape 7 : preuves et commit.** `format.ps1`, preuve de tâche, `NOMS OK`, talons absents, commit
  « Lot 7, phase 1 : dés, calculatrice et compte à rebours (T14 à T16) (L7-P5) ».

<a id="tache-l7-p6"></a>
### Tâche L7-P6 — Modèle du message défilant et fichier de messages (T18, T19)

**Objectif.** Réglages du message (texte brut, police, couleurs, ombre, fond, position, boucle, vitesse), défilement
en fonction du temps, lecture et écriture du fichier `actcraft-ticker/1` (64 Kio au plus, clés inconnues ignorées).

**Fichiers.** Ligne L7-P6 de la table.

**Test et commande bornée.** Commande ciblée `-R tst_features_ticker`, preuve de tâche.

**Preuve visuelle.** Aucune.

**Interdits.** Règles communes ; le texte reste une `QString` brute (jamais interprétée en texte riche) ; fichier
refusé au-delà de 64 Kio **avant** l'analyse JSON.

**Terminé quand.** `Totals: 9 passed, 0 failed, 0 skipped`, preuve, `NOMS OK`, commit.

**Étapes.**

- [ ] **Étape 1 : rouge.** `tests/tst_features_ticker.cpp` (montage, classe `TstFeaturesTicker`) : les quatre fonctions
  de 7a, L7-T18, étape 1, mot pour mot, plus :
  ```cpp
      void messageFileRoundTrip()
      {
          TickerSettings s;
          s.text = QStringLiteral("Récréation à 10 h <b>pas de gras</b>");
          s.fontFamily = QStringLiteral("Arial");
          s.fontPointSize = 40;
          s.textColor = QColor(0x12, 0x34, 0x56);
          s.shadowOffset = 3;
          s.screenBackground = false;
          s.position = TickerSettings::Position::Bottom;
          s.loop = true;
          s.speed = 7;
          const auto back = tickerFromJson(tickerToJson(s));
          QVERIFY(back.has_value());
          QVERIFY(*back == s);

          TickerSettings big;
          big.text = QString(70 * 1024, QLatin1Char('a'));
          QVERIFY(!tickerFromJson(tickerToJson(big)).has_value()); // plus de 64 Kio : refusé

          QVERIFY(!tickerFromJson(R"({"format": "autre/1", "text": "x"})").has_value());
          QVERIFY(!tickerFromJson("pas du json").has_value());
      }
      void unknownKeysAreIgnored()
      {
          const auto s = tickerFromJson(R"({"format": "actcraft-ticker/1", "text": "Bonjour", "inconnue": 3, "speed": 99})");
          QVERIFY(s.has_value());
          QCOMPARE(s->text, QStringLiteral("Bonjour"));
          QCOMPARE(s->speed, 10);                                       // borné
          QCOMPARE(s->position, TickerSettings::Position::Top);         // absente : défaut
      }
      void loopNeverFinishes()
      {
          TickerRun run(1000, 200, 100.0, true);
          QCOMPARE(run.xAt(6000), 400.0);
          QVERIFY(!run.finishedAt(1'000'000));
      }
  ```
  Rouge.
- [ ] **Étape 2 : modèle.** `features/include/actcraft/features/ticker.h` :
  ```cpp
  #pragma once

  #include <QByteArray>
  #include <QColor>
  #include <QString>

  #include <cstdint>
  #include <optional>

  namespace actcraft::features {

  // Réglages du message défilant (F-291). Le texte est du texte brut, jamais interprété.
  struct TickerSettings
  {
      enum class Position { Top, Center, Bottom };

      QString text = QStringLiteral("Votre message ici");
      QString fontFamily;               // vide : police de l'interface
      double fontPointSize = 32;
      QColor textColor = QColor(0x1F, 0x29, 0x37);
      int shadowOffset = 0;             // pixels ; 0 : sans ombre
      QColor shadowColor = QColor(0x00, 0x00, 0x00);
      bool screenBackground = true;     // vrai : fond transparent sur l'écran ; faux : backgroundColor
      QColor backgroundColor = QColor(0xFF, 0xF4, 0xC2);
      Position position = Position::Top;
      bool loop = false;
      int speed = 5;                    // 1 à 10

      static double pixelsPerSecond(int speed) noexcept; // 40 × vitesse bornée à 1..10
      static Position positionFromText(const QString& text) noexcept; // "top", "center", "bottom" ; sinon Top
      static QString positionToText(Position position);

      friend bool operator==(const TickerSettings&, const TickerSettings&) = default;
  };

  // Un passage du message : abscisse du bord gauche du texte en fonction du temps écoulé (ms).
  class TickerRun
  {
  public:
      TickerRun(double screenWidth, double textWidth, double pixelsPerSecond, bool loop);
      int durationMs() const noexcept;                    // (écran + texte) / vitesse
      double xAt(std::int64_t elapsedMs) const noexcept;  // de screenWidth à -textWidth ; en boucle, modulo la durée
      bool finishedAt(std::int64_t elapsedMs) const noexcept; // jamais en boucle

  private:
      double m_screenWidth;
      double m_pixelsPerSecond;
      bool m_loop;
      int m_durationMs;
  };

  inline constexpr qsizetype kMaxTickerFileBytes = 64 * 1024;
  QByteArray tickerToJson(const TickerSettings& settings);                 // format « actcraft-ticker/1 »
  std::optional<TickerSettings> tickerFromJson(const QByteArray& data);   // rien si trop gros, illisible ou autre format

  } // namespace actcraft::features
  ```
  `features/src/ticker.cpp` : clés JSON (noms anglais à nous) `format`, `text`, `fontFamily`, `fontPointSize`,
  `textColor`, `shadowOffset`, `shadowColor`, `screenBackground`, `backgroundColor`, `position`, `loop`, `speed` ;
  couleurs en `QColor::name(QColor::HexArgb)`, relues par `QColor::fromString` (invalide → défaut) ; clé absente →
  défaut ; `speed` bornée à 1..10 ; `tickerFromJson` : `data.size() > kMaxTickerFileBytes` → rien, puis
  `QJsonDocument::fromJson` (erreur ou pas un objet → rien), `format` différent de `actcraft-ticker/1` → rien ;
  `TickerRun` : durée `qRound((écran + texte) / vitesse × 1000)` (0 si vitesse ≤ 0) ; `xAt` : temps ramené à
  `elapsed % durée` en boucle, borné à la durée sinon, puis `écran − vitesse × temps / 1000`. Commande ciblée
  `-R tst_features_ticker` : `9 passed, 0 failed, 0 skipped`.
- [ ] **Étape 3 : preuves et commit.** `format.ps1`, preuve de tâche, `NOMS OK`, talon absent, commit
  « Lot 7, phase 1 : modèle du message défilant et fichier de messages (T18, T19) (L7-P6) ».

<a id="tache-l7-p7"></a>
### Tâche L7-P7 — Reconnaissance de forme `shapefinder` et session de reconnaissance (T20, minuterie de T21)

**Objectif.** L'algorithme à nous de 7b (L7-T20) : ligne, rectangle ou carré (droits ou inclinés), ellipse ou cercle
(droits ou inclinés), triangle ; sinon rien. Et la session qui regroupe les traits et les rend mûrs après le délai.

**Fichiers.** Ligne L7-P7 de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_features_(shapefinder|recognitionsession)'`, preuve de tâche.

**Preuve visuelle.** Aucune.

**Interdits.** Ceux de 7b, L7-T20 (aucun code, constante ni seuil recopiés d'un logiciel tiers ni de la référence
Python ; seuils à nous, nommés, avec leur raison ; QtCore seulement : `QPointF`, `QRectF`) ; règles communes. Patron à
**ne pas** reprendre : la classification par « boîte englobante + rapport des côtés » (elle rate les formes
inclinées) ; un seuil global unique pour toutes les formes.

**Terminé quand.** Deux tests verts sans saut, preuve, `NOMS OK`, commit.

**Étapes.**

- [ ] **Étape 1 : rouge de la forme.** `tests/tst_features_shapefinder.cpp` = le fichier de 7b, L7-T20, étape 1, avec
  trois corrections de portabilité : `#include <numbers>` et `2 * std::numbers::pi * turns * i / n` au lieu de
  `M_PI` ; `static_cast<double>(pts.size())` dans `spiralIsNothing` ; `double(k) / perSide` écrit
  `static_cast<double>(k) / perSide`. S'y ajoutent deux fonctions pour la classe « polygone » (P16 de L7-T0,
  `docs/verifications/2026-10-18-lot7-principes.md` : rectangle seulement si les angles sont droits ; un quadrilatère
  approximatif devient un polygone) :
  ```cpp
      void roughQuadrilateralIsAPolygon()
      {
          // trapèze : angles de 76° et 104°, hors de 90° ± 10°
          const auto r = recognizeShape(noisy(polyline({{100, 100}, {300, 100}, {350, 300}, {50, 300}}, 25), 2, 8));
          QVERIFY(r.has_value());
          QCOMPARE(r->kind, RecognizedShape::Kind::Polygon);
          QCOMPARE(r->corners.size(), std::size_t{4});
      }
      void pentagonIsAPolygon()
      {
          const auto r = recognizeShape(noisy(polyline({{300, 100}, {440, 200}, {390, 360}, {210, 360}, {160, 200}}, 25), 2, 9));
          QVERIFY(r.has_value());
          QCOMPARE(r->kind, RecognizedShape::Kind::Polygon);
          QCOMPARE(r->corners.size(), std::size_t{5});
      }
  ```
  (11 fonctions, donc `Totals: 13`.) Rouge.
- [ ] **Étape 2 : en-tête.** `features/include/actcraft/features/shapefinder.h` = l'en-tête de 7b, L7-T20, étape 2,
  avec un seul écart : `enum class Kind { Line, Rectangle, Square, Ellipse, Circle, Triangle, Polygon };` (`Polygon`
  ajouté en dernier ; `corners` porte alors ses sommets).
- [ ] **Étape 3 : algorithme.** `features/src/shapefinder.cpp` = l'étape 3 de 7b, L7-T20 (fonctions internes
  nommées `resample`, `extent`, `isLine`, `closure`, `corners`, `sidesStraight`, `principalAxes`, `radialError` ; ordre
  et seuils de départ de 7b, chacun une constante nommée commentée). Pour la ligne : `corners` = les deux extrémités ;
  rectangle : `bounds` non tourné centré sur le centre de la forme, `angleDegrees` = angle du premier côté ramené dans
  ]−45, 45] ; ellipse : `bounds` = demi-axes × 2, `angleDegrees` = axe principal ramené dans ]−90, 90]. Si un tracé
  bruité de test échoue, ajuster **le seuil** et sa raison, jamais le test. **Écart à 7b (P16)** dans l'ordre de
  décision : 3 coins et côtés droits → triangle ; 4 coins, côtés droits **et** angles à 90° ± 10° (`kRightAngleSlack
  = 10`, raison : un trapèze dessiné à main levée ne doit pas devenir un rectangle) → rectangle ou carré ; 4 à
  `kMaxPolygonCorners = 6` coins et côtés droits → polygone (raison : un coin est un virage ≥ 55° ; l'angle extérieur
  d'un polygone de 7 côtés ou plus est sous 55°, ses sommets ne sont pas des coins : la borne suit le seuil des coins
  et change avec lui) ; puis l'ellipse. Le seuil des angles de 7b (± 20°) passe à ± 10° : vérifier que
  `squareIsASquare` et `tiltedRectangleKeepsItsCorners` restent verts (leurs angles sont droits). Commande ciblée
  `-R tst_features_shapefinder` : `13 passed, 0 failed, 0 skipped`.
- [ ] **Étape 4 : rouge de la session.** `tests/tst_features_recognitionsession.cpp` :
  ```cpp
  #include <actcraft/features/recognitionsession.h>

  #include <QtTest>

  using namespace actcraft::features;

  class TstFeaturesRecognitionSession : public QObject
  {
      Q_OBJECT

  private slots:
      // Les trois fonctions de 7b, L7-T21, étape 1, mot pour mot :
      // sessionFiresAfterTheDelay, newStrokeRestartsTheDelay, zeroDelayIsImmediate.
      void defaultDelayIsOneSecond()
      {
          // L7-T0 (docs/verifications/2026-10-18-lot7-principes.md) : délai par défaut 1 s, bornes 0 à 5 s.
          const RecognitionSession s;
          QCOMPARE(s.delayMs(), 1000);
      }
      void delayClampedTo0To5000()
      {
          RecognitionSession s(9000);
          QCOMPARE(s.delayMs(), 5000);
          s.setDelayMs(-3);
          QCOMPARE(s.delayMs(), 0);
      }
  };

  QTEST_GUILESS_MAIN(TstFeaturesRecognitionSession)
  #include "tst_features_recognitionsession.moc"
  ```
  (recopier les trois fonctions à la place du commentaire.) Rouge.
- [ ] **Étape 5 : session.** `features/include/actcraft/features/recognitionsession.h` :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>

  #include <cstdint>
  #include <vector>

  namespace actcraft::features {

  // Session de reconnaissance (F-295, F-296) : les traits versés s'accumulent ; quand le délai s'est écoulé depuis le
  // DERNIER trait, ils sont rendus ensemble (un mot, une forme), une seule fois.
  class RecognitionSession
  {
  public:
      static constexpr int kMaxDelayMs = 5000;
      // Délai par défaut 1 s, bornes 0 à 5 s : réglage embarqué relevé par L7-T0 (la spécification disait 5 s, 1 à 5).
      static constexpr int kDefaultDelayMs = 1000;
      explicit RecognitionSession(int delayMs = kDefaultDelayMs);
      void setDelayMs(int delayMs) noexcept; // borné à [0, kMaxDelayMs]
      int delayMs() const noexcept;
      void addStroke(core::ObjectId id, std::int64_t nowNs);
      std::vector<core::ObjectId> due(std::int64_t nowNs); // mûrs : rendus puis oubliés
      const std::vector<core::ObjectId>& pending() const noexcept;
      void clear() noexcept;

  private:
      int m_delayMs = 0;
      std::vector<core::ObjectId> m_pending;
      std::int64_t m_lastNs = 0;
  };

  } // namespace actcraft::features
  ```
  `features/src/recognitionsession.cpp` : `due` rend `{}` si rien n'attend ou si `now − m_lastNs <
  m_delayMs × 1'000'000` ; sinon rend `m_pending` et le vide. Commande ciblée
  `-R 'tst_features_(shapefinder|recognitionsession)'` : deux `Totals` à `0 failed, 0 skipped`.
- [ ] **Étape 6 : preuves et commit.** `format.ps1`, preuve de tâche, `NOMS OK`, talons absents, commit
  « Lot 7, phase 1 : reconnaissance de forme à nous et session de reconnaissance (T20, T21) (L7-P7) ».

<a id="tache-l7-p8"></a>
### Tâche L7-P8 — Reconnaissance d'écriture : moteur de Windows et fil dédié en STA (T22, fil de T23)

**Objectif.** L'interface `HandwritingRecognizer`, sa réalisation « indisponible » (avec son explication en français)
et celle de Windows par COM (7b, L7-T22, étape 2) ; **et** le fil dédié de T23 (relecture I7) : un `QThread` créé une
fois et gardé, sur lequel COM est initialisé en appartement unique (STA), le reconnaisseur créé, utilisé et détruit,
puis COM libéré. Le fil appelant n'attend jamais.

**Fichiers.** Ligne L7-P8 de la table.

**Test et commande bornée.** Commande ciblée `-R tst_features_handwriting`, preuve de tâche.

**Preuve visuelle.** Aucune.

**Interdits.** Ceux de 7b, L7-T22 : `<windows.h>`, `<msinkaut.h>` et tout appel COM seulement dans
`features/src/handwriting_win.cpp` ; aucune bibliothèque tierce ; `CoInitializeEx`/`CoUninitialize` appariés **sur
le même fil**. Patron à **ne pas** reprendre : `QtConcurrent::run` ou `QThreadPool` (pas le même fil d'un appel à
l'autre) ; COM initialisé sur le fil principal ; reconnaisseur créé sur un fil et utilisé sur un autre ; attente
bloquante du fil appelant (`wait` hors destruction, `BlockingQueuedConnection`). Pas de `Q_OBJECT`.

**Terminé quand.** Sous Windows, `Totals: 7 passed, 0 failed, 0 skipped` (ou `6 passed, 0 failed, 1 skipped` avec le seul
`recognizesSyntheticInk` sauté pour « reconnaisseur absent : … », signalé au rapport) ; preuve ; `NOMS OK` ; commit.

**Étapes.**

- [ ] **Étape 1 : rouge.** `tests/tst_features_handwriting.cpp` :
  ```cpp
  #include <actcraft/features/handwriting.h>

  #include <QtTest>

  #include <memory>
  #include <mutex>

  #ifdef Q_OS_WIN
  #include <objbase.h> // test seulement (pas du code livré) : type d'appartement COM du fil
  #endif

  using namespace actcraft::features;

  namespace {

  // Reconnaisseur factice : note le fil de chaque appel.
  struct ThreadLog
  {
      std::mutex mutex;
      std::vector<QThread*> threads;
      bool sta = false;
  };

  class FakeRecognizer final : public HandwritingRecognizer
  {
  public:
      explicit FakeRecognizer(std::shared_ptr<ThreadLog> log, bool isAvailable)
          : m_log(std::move(log)), m_available(isAvailable)
      {
          note();
      }
      bool available() const override { return m_available; }
      QString status() const override
      {
          return m_available ? QStringLiteral("factice") : QStringLiteral("indisponible : test");
      }
      QStringList recognize(const InkStrokes&) override
      {
          note();
  #ifdef Q_OS_WIN
          APTTYPE type{};
          APTTYPEQUALIFIER qualifier{};
          if (SUCCEEDED(CoGetApartmentType(&type, &qualifier))) {
              const std::lock_guard lock(m_log->mutex);
              m_log->sta = type == APTTYPE_STA || type == APTTYPE_MAINSTA;
          }
  #endif
          return {QStringLiteral("bonjour"), QStringLiteral("bonjours")};
      }

  private:
      void note()
      {
          const std::lock_guard lock(m_log->mutex);
          m_log->threads.push_back(QThread::currentThread());
      }
      std::shared_ptr<ThreadLog> m_log;
      bool m_available;
  };

  } // namespace

  class TstFeaturesHandwriting : public QObject
  {
      Q_OBJECT

  private slots:
      void factoryNeverReturnsNull()
      {
          auto r = makeHandwritingRecognizer(); // fil principal, sans COM : retombe sur « indisponible » au besoin
          QVERIFY(r != nullptr);
          if (!r->available())
              QVERIFY(!r->status().isEmpty());
  #ifndef Q_OS_WIN
          QVERIFY(!r->available()); // hors de Windows, jamais de moteur (reprend unavailableOffWindows de 7b)
  #endif
      }
      void recognitionRunsOnOneDedicatedThread()
      {
          auto log = std::make_shared<ThreadLog>();
          HandwritingThread thread([log] { return std::make_unique<FakeRecognizer>(log, true); });
          std::vector<QThread*> callbackThreads;
          int replies = 0;
          const InkStrokes ink{{{0, 0}, {0, 10}}};
          for (int i = 0; i < 2; ++i)
              thread.recognize(ink, this, [&](const RecognitionReply& reply) {
                  QCOMPARE(reply.texts.value(0), QStringLiteral("bonjour"));
                  callbackThreads.push_back(QThread::currentThread());
                  ++replies;
              });
          QTRY_COMPARE(replies, 2);
          const std::lock_guard lock(log->mutex);
          QCOMPARE(log->threads.size(), std::size_t{3}); // création + deux reconnaissances
          for (QThread* t : log->threads) {
              QCOMPARE(t, static_cast<QThread*>(&thread)); // toujours le même fil, le fil dédié
              QVERIFY(t != QThread::currentThread());       // jamais le fil principal
          }
          for (QThread* t : callbackThreads)
              QCOMPARE(t, QThread::currentThread());        // réponse rendue sur le fil du receveur
      }
      void recognitionThreadIsSingleThreadedApartment()
      {
  #ifdef Q_OS_WIN
          auto log = std::make_shared<ThreadLog>();
          HandwritingThread thread([log] { return std::make_unique<FakeRecognizer>(log, true); });
          bool done = false;
          thread.recognize(InkStrokes{{{0, 0}, {5, 5}}}, this, [&](const RecognitionReply&) { done = true; });
          QTRY_VERIFY(done);
          const std::lock_guard lock(log->mutex);
          QVERIFY(log->sta);
  #else
          QSKIP("Pas de COM hors de Windows : l'appartement ne se vérifie que sous Windows (preuve locale).");
  #endif
      }
      void unavailableReplyCarriesTheExplanation()
      {
          auto log = std::make_shared<ThreadLog>();
          HandwritingThread thread([log] { return std::make_unique<FakeRecognizer>(log, false); });
          std::optional<RecognitionReply> got;
          thread.probe(this, [&](const RecognitionReply& reply) { got = reply; });
          QTRY_VERIFY(got.has_value());
          QVERIFY(!got->available);
          QVERIFY(!got->status.isEmpty());
          QVERIFY(got->texts.isEmpty());
      }
      void recognizesSyntheticInk()
      {
          HandwritingThread thread; // vrai reconnaisseur, sur le fil STA
          std::optional<RecognitionReply> probe;
          thread.probe(this, [&](const RecognitionReply& reply) { probe = reply; });
          QTRY_VERIFY(probe.has_value());
          if (!probe->available)
              QSKIP(qPrintable(QStringLiteral("reconnaisseur absent : ") + probe->status));
          // « l » : un trait vertical ; le contenu n'est pas comparé (S10), seulement la forme du résultat
          std::optional<RecognitionReply> got;
          thread.recognize(InkStrokes{{{100, 100}, {100, 140}, {100, 180}}}, this,
                           [&](const RecognitionReply& reply) { got = reply; });
          QTRY_VERIFY_WITH_TIMEOUT(got.has_value(), 10000);
          QVERIFY(!got->texts.isEmpty());
          QStringList copy = got->texts;
          QCOMPARE(copy.removeDuplicates(), qsizetype{0});
      }
  };

  QTEST_GUILESS_MAIN(TstFeaturesHandwriting)
  #include "tst_features_handwriting.moc"
  ```
  Rouge.
- [ ] **Étape 2 : en-tête.** `features/include/actcraft/features/handwriting.h` :
  ```cpp
  #pragma once

  #include <QPointF>
  #include <QString>
  #include <QStringList>
  #include <QThread>

  #include <functional>
  #include <memory>
  #include <vector>

  namespace actcraft::features {

  using InkStrokes = std::vector<std::vector<QPointF>>; // traits en pixels de page

  // Reconnaisseur d'écriture (F-295). Créé, utilisé et détruit sur un seul fil (celui de HandwritingThread).
  class HandwritingRecognizer
  {
  public:
      virtual ~HandwritingRecognizer() = default;
      virtual bool available() const = 0;
      virtual QString status() const = 0; // explication si indisponible ; nom du reconnaisseur sinon
      virtual QStringList recognize(const InkStrokes& strokes) = 0; // meilleur texte puis 10 alternatives au plus, sans doublon
  };

  // Réalisation de la plateforme : handwriting_win.cpp (Windows) ou handwriting_other.cpp (nullptr).
  std::unique_ptr<HandwritingRecognizer> makePlatformHandwritingRecognizer();
  // Jamais nul : la réalisation de la plateforme, sinon « indisponible » avec son explication.
  std::unique_ptr<HandwritingRecognizer> makeHandwritingRecognizer();
  // Appartement COM du fil courant : Windows, CoInitializeEx en appartement unique ; ailleurs, rien.
  // Vrai si leaveRecognitionApartment() doit être appelé sur ce même fil.
  bool enterRecognitionApartment();
  void leaveRecognitionApartment();

  struct RecognitionReply
  {
      bool available = false;
      QString status;
      QStringList texts; // vide si indisponible ou pour probe()
  };

  // Fil dédié de la reconnaissance (relecture I7) : créé une fois, gardé ; run() entre dans l'appartement, crée le
  // reconnaisseur, sert les demandes, puis le détruit et sort de l'appartement, sur ce seul fil. Aucune attente du
  // fil appelant hors du destructeur. Le receveur d'une réponse doit vivre plus longtemps que ce fil (le contrôleur
  // de la phase 2 possède le fil et reçoit ses réponses).
  class HandwritingThread final : public QThread
  {
  public:
      using Factory = std::function<std::unique_ptr<HandwritingRecognizer>()>;
      using Callback = std::function<void(const RecognitionReply&)>;

      explicit HandwritingThread(Factory factory = &makeHandwritingRecognizer);
      ~HandwritingThread() override; // quit() puis wait()
      HandwritingThread(const HandwritingThread&) = delete;
      HandwritingThread& operator=(const HandwritingThread&) = delete;

      // Non bloquants ; `done` est appelé sur le fil de `receiver` (en file).
      void recognize(InkStrokes strokes, QObject* receiver, Callback done);
      void probe(QObject* receiver, Callback done);

  protected:
      void run() override;

  private:
      void post(QObject* receiver, Callback done, std::optional<InkStrokes> strokes);

      Factory m_factory;
      QObject* m_context; // vit sur ce fil (moveToThread avant start) ; détruit à la fin de run()
      std::unique_ptr<HandwritingRecognizer> m_recognizer; // touché seulement par ce fil
  };

  } // namespace actcraft::features
  ```
  (ajouter `#include <optional>`.)
- [ ] **Étape 3 : fil et réalisation « indisponible ».** `features/src/handwriting.cpp` :
  ```cpp
  #include <actcraft/features/handwriting.h>

  #include <QMetaObject>

  namespace actcraft::features {

  namespace {

  class UnavailableRecognizer final : public HandwritingRecognizer
  {
  public:
      explicit UnavailableRecognizer(QString why)
          : m_why(std::move(why))
      {
      }
      bool available() const override { return false; }
      QString status() const override { return m_why; }
      QStringList recognize(const InkStrokes&) override { return {}; }

  private:
      QString m_why;
  };

  QString unavailableText()
  {
      return QStringLiteral("La reconnaissance d'écriture utilise le moteur d'encre de Windows ; il n'est pas "
                            "disponible sur ce système.");
  }

  } // namespace

  std::unique_ptr<HandwritingRecognizer> makeHandwritingRecognizer()
  {
      auto platform = makePlatformHandwritingRecognizer();
      if (platform)
          return platform;
      return std::make_unique<UnavailableRecognizer>(unavailableText());
  }

  HandwritingThread::HandwritingThread(Factory factory)
      : m_factory(std::move(factory)), m_context(new QObject)
  {
      m_context->moveToThread(this); // les demandes attendent dans la file jusqu'à exec()
      start();
  }

  HandwritingThread::~HandwritingThread()
  {
      quit();
      wait();
  }

  void HandwritingThread::run()
  {
      const bool apartment = enterRecognitionApartment();
      m_recognizer = m_factory ? m_factory() : nullptr;
      if (!m_recognizer)
          m_recognizer = std::make_unique<UnavailableRecognizer>(unavailableText());
      exec();
      m_recognizer.reset(); // détruit sur ce fil, avant de sortir de l'appartement
      delete m_context;     // emporte les demandes restées en file
      m_context = nullptr;
      if (apartment)
          leaveRecognitionApartment();
  }

  void HandwritingThread::recognize(InkStrokes strokes, QObject* receiver, Callback done)
  {
      post(receiver, std::move(done), std::move(strokes));
  }

  void HandwritingThread::probe(QObject* receiver, Callback done)
  {
      post(receiver, std::move(done), std::nullopt);
  }

  void HandwritingThread::post(QObject* receiver, Callback done, std::optional<InkStrokes> strokes)
  {
      QMetaObject::invokeMethod(
          m_context,
          [this, receiver, done = std::move(done), strokes = std::move(strokes)]() {
              RecognitionReply reply{m_recognizer->available(), m_recognizer->status(), {}};
              if (reply.available && strokes)
                  reply.texts = m_recognizer->recognize(*strokes);
              QMetaObject::invokeMethod(receiver, [done, reply]() { done(reply); }, Qt::QueuedConnection);
          },
          Qt::QueuedConnection);
  }

  } // namespace actcraft::features
  ```
  `features/src/handwriting_other.cpp` : `makePlatformHandwritingRecognizer()` rend `nullptr` ;
  `enterRecognitionApartment()` rend `false` ; `leaveRecognitionApartment()` ne fait rien.
- [ ] **Étape 4 : moteur de Windows.** `features/src/handwriting_win.cpp` : `#define WIN32_LEAN_AND_MEAN`,
  `#define NOMINMAX`, puis `<windows.h>`, `<objbase.h>`, `<msinkaut.h>`, `<wrl/client.h>` ; `enterRecognitionApartment`
  : `CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)` → vrai si `SUCCEEDED` (S_OK et S_FALSE appellent tous deux
  `CoUninitialize`), faux sur `RPC_E_CHANGED_MODE` ; `leaveRecognitionApartment` : `CoUninitialize()` ;
  `makePlatformHandwritingRecognizer` : `CoCreateInstance(__uuidof(InkRecognizers), …, IID_PPV_ARGS(&recognizers))`
  (échec, y compris `CO_E_NOTINITIALIZED` sur un fil sans appartement → `nullptr`) ; parcours `get_Count` / `Item` ;
  `get_Languages` (`SAFEARRAY` de LCID) : premier reconnaisseur dont une langue a `PRIMARYLANGID == LANG_FRENCH`,
  sinon le premier qui a une langue, sinon `nullptr` ; la classe garde `ComPtr<IInkRecognizer>` et son nom
  (`get_Name`) pour `status()`. `recognize` : `CoCreateInstance(__uuidof(InkDisp))` → `IInkDisp` ; par trait, un
  `SAFEARRAY` `VT_I4` de `x0, y0, x1, y1, …` en HIMETRIC (`kHimetricPerPixel = 2540.0 / 96.0`, arrondi par
  `std::lround`, converti en `LONG`), `VARIANT` `VT_ARRAY | VT_I4`, `IInkDisp::CreateStroke` ; `get_Strokes` ;
  `IInkRecognizer::CreateRecognizerContext` ; `putref_Strokes` ; `Recognize` ; `get_TopString` ;
  `AlternatesFromSelection(0, -1, 10, …)` puis, pour chaque alternative (`get_Count`, `Item`), son texte lu **par
  `IDispatch`** : `GetIDsOfNames` sur `L"String"` puis `Invoke` en `DISPATCH_PROPERTYGET` (l'accesseur C++ direct de
  cette propriété porte un nom refusé par le contrôle des noms : ne jamais l'écrire, ni en code ni en commentaire ;
  cas permis tranché dans les règles communes, « Contrôle des noms » : le code ne contient que `GetIDsOfNames`,
  `Invoke`, `DISPATCH_PROPERTYGET`, `DISPPARAMS`, `VARIANT` et la chaîne `L"String"`, tous contrôlés `NOMS OK`) ;
  doublons retirés, meilleur texte en premier ;
  `BSTR` libérés (`SysFreeString`), `SAFEARRAY` détruits, interfaces par `Microsoft::WRL::ComPtr`. Toute erreur COM
  → liste vide (jamais d'exception ni d'arrêt). `QString::fromWCharArray(bstr, static_cast<qsizetype>(SysStringLen(bstr)))`.
  Si `/W4 /WX /permissive-` refuse un en-tête du SDK : le rapporter (ne pas désactiver d'avertissement sans
  l'accord de l'orchestrateur).
- [ ] **Étape 5 : vert.** Commande ciblée `-R tst_features_handwriting` (Windows) : `Totals: 7 passed, 0 failed,
  0 skipped`, ou `6 passed, 0 failed, 1 skipped` avec `SKIP   : TstFeaturesHandwriting::recognizesSyntheticInk() reconnaisseur absent : …`
  (rapporter la ligne).
- [ ] **Étape 6 : preuves et commit.** `format.ps1`, preuve de tâche, `NOMS OK`, talon absent ; vérifier
  `git -C '<wt>' grep -n "CoInitializeEx\|windows.h\|msinkaut" -- features` : seulement `handwriting_win.cpp` ;
  commit « Lot 7, phase 1 : reconnaissance d'écriture par le moteur d'encre de Windows, fil dédié en STA (T22, T23)
  (L7-P8) ».

<a id="tache-l7-p9"></a>
### Tâche L7-P9 — Géométrie de la caméra, disposition du clavier à l'écran, roue des outils de bureau (T28, T29, T31)

**Objectif.** Sélections de la caméra (point par point, main levée, rectangle), découpe hors polygone transparente,
pixels physiques ramenés en pixels logiques (F-275) ; clavier AZERTY à nous, trois dispositions, majuscule et
verrouillage (F-294) ; positions de la roue dans le sens horaire depuis le haut et roue par défaut (F-080).

**Fichiers.** Ligne L7-P9 de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_features_(camera|keyboard|desktopwheel)'`, preuve de tâche.

**Preuve visuelle.** Aucune.

**Interdits.** Règles communes ; aucune capture d'écran réelle, aucun `SendInput`, aucune énumération de fenêtres
(phase 2 : `camera_win.cpp`, `keyinjector_win.cpp` et leurs jumeaux) ; aucune entrée de vote ni de service tiers dans
la roue ; libellés à nous.

**Terminé quand.** Trois tests verts sans saut, preuve, `NOMS OK`, commit.

**Étapes.**

- [ ] **Étape 1 : rouge de la caméra.** `tests/tst_features_camera.cpp` (`#include <actcraft/features/camera.h>`,
  `#include <QtTest>`, `#include <QColor>`, `#include <QImage>`, classe `TstFeaturesCamera`, **`QTEST_MAIN`** : le
  peintre sur `QImage` veut une application graphique, hors écran par l'environnement de `actcraft_add_test`) : les
  quatre fonctions de 7b, L7-T28, étape 1, avec `QCOMPARE(p.polygon().size(), qsizetype{3});`, plus :
  ```cpp
      void windowRectsInLogicalPixels()
      {
          QCOMPARE(toLogicalPixels(QRect(300, 150, 900, 600), 1.5), QRectF(200, 100, 600, 400));
          QCOMPARE(toLogicalPixels(QRect(10, 20, 30, 40), 0), QRectF(10, 20, 30, 40)); // rapport invalide : 1
      }
  ```
  Rouge.
- [ ] **Étape 2 : caméra.** `features/include/actcraft/features/camera.h` :
  ```cpp
  #pragma once

  #include <QImage>
  #include <QPointF>
  #include <QPolygonF>
  #include <QRect>
  #include <QRectF>

  namespace actcraft::features {

  inline constexpr double kClosePixels = 10; // un clic à moins de 10 px du premier point ferme le polygone

  // Capture point par point (F-275) : chaque clic ajoute un sommet ; un clic près du premier (3 sommets au moins) ferme.
  class PolygonPicker
  {
  public:
      void click(QPointF point);
      bool closed() const noexcept;
      QPolygonF polygon() const; // sommets, sans le clic de fermeture
      void clear();

  private:
      QPolygonF m_points;
      bool m_closed = false;
  };

  // Capture à main levée : le relâché ferme par une droite (3 points au moins).
  class FreehandPicker
  {
  public:
      void press(QPointF point);
      void move(QPointF point);
      void release();
      bool closed() const noexcept;
      QPolygonF polygon() const; // fermé : dernier point = premier
      void clear();

  private:
      QPolygonF m_points;
      bool m_closed = false;
  };

  // Image de la boîte du polygone (bornée à l'image) ; hors du polygone : transparent.
  QImage cropToPolygon(const QImage& screen, const QPolygonF& polygon);
  QRectF normalizedRect(QPointF a, QPointF b);
  // Rectangle en pixels physiques (DwmGetWindowAttribute) ramené en pixels logiques ; rapport ≤ 0 : 1.
  QRectF toLogicalPixels(const QRect& physical, qreal devicePixelRatio);

  } // namespace actcraft::features
  ```
  `features/src/camera.cpp` : `cropToPolygon` : boîte `polygon.boundingRect().toAlignedRect() & screen.rect()`,
  `QImage out(boîte.size(), QImage::Format_ARGB32_Premultiplied)`, `out.fill(Qt::transparent)`, `QPainter` avec
  anticrénelage, `QPainterPath` du polygone translaté de `-boîte.topLeft()`, `setClipPath`, `drawImage(-boîte.topLeft(),
  screen)` ; `normalizedRect` : `QRectF(a, b).normalized()`. Commande ciblée `-R tst_features_camera` : `7 passed …
  0 skipped`.
- [ ] **Étape 3 : rouge du clavier.** `tests/tst_features_keyboard.cpp` (montage, classe `TstFeaturesKeyboard`) : les
  quatre fonctions de 7b, L7-T29, étape 1, mot pour mot, plus :
  ```cpp
      void everyKeyHasALabel()
      {
          for (auto layout : {ScreenKeyboard::Layout::Compact, ScreenKeyboard::Layout::Standard, ScreenKeyboard::Layout::Extended}) {
              const ScreenKeyboard k(layout);
              for (const auto& cap : k.keys()) {
                  QVERIFY(!cap.id.isEmpty());
                  QVERIFY(!cap.label.isEmpty());
                  QVERIFY(cap.width > 0);
              }
          }
      }
  ```
  Rouge.
- [ ] **Étape 4 : clavier.** `features/include/actcraft/features/screenkeyboard.h` :
  ```cpp
  #pragma once

  #include <QString>
  #include <QtCore/qnamespace.h>

  #include <vector>

  namespace actcraft::features {

  struct KeyCap
  {
      QString id;       // caractère tapé (minuscule) ou "shift", "caps", "backspace", "enter", "space"
      QString label;    // libellé à nous
      double width = 1; // en largeurs de touche
      int row = 0;
  };

  struct KeyOutput
  {
      QString text;                  // vide pour une touche de commande
      Qt::Key key = Qt::Key_unknown;
  };

  // Clavier à l'écran (F-294), AZERTY, écrit par nous.
  class ScreenKeyboard
  {
  public:
      enum class Layout { Compact, Standard, Extended };
      explicit ScreenKeyboard(Layout layout);
      void setLayout(Layout layout);
      Layout layout() const noexcept;
      const std::vector<KeyCap>& keys() const noexcept;
      KeyOutput press(const QString& id); // identifiant absent de la disposition : sortie vide
      bool shifted() const noexcept;      // majuscule pour la prochaine touche seulement
      bool capsLocked() const noexcept;

  private:
      Layout m_layout;
      std::vector<KeyCap> m_keys;
      bool m_shift = false;
      bool m_caps = false;
  };

  } // namespace actcraft::features
  ```
  `features/src/screenkeyboard.cpp`, dispositions (rangées dans l'ordre, largeur 1 sauf mention) :
  - **Compact** (30 touches) : `a z e r t y u i o p` ; `q s d f g h j k l m` ; `shift`(1,5) `w x c v b n`
    `backspace`(1,5) ; `space`(6) `enter`(2).
  - **Standard** (35) : `a z e r t y u i o p` ; `q s d f g h j k l m` ; `shift`(1,5) `w x c v b n , ; : !`
    `backspace`(1,5) ; `caps`(1,5) `space`(6) `enter`(2).
  - **Extended** (57) : `1 2 3 4 5 6 7 8 9 0` ; `é è à ç ù ' - ( ) " ? .` ; puis les quatre rangées de Standard.
  Libellés : le caractère lui-même ; `Maj`, `Verr. maj`, `Effacer`, `Entrée`, `Espace`. `press` : `shift` bascule
  la majuscule d'une touche (`Qt::Key_Shift`) ; `caps` bascule le verrouillage (`Qt::Key_CapsLock`) ; `backspace` →
  `Qt::Key_Backspace` ; `enter` → `Qt::Key_Return` ; `space` → `" "`, `Qt::Key_Space` ; un caractère : en
  majuscule si `m_shift || m_caps` (`toUpper()`), puis `m_shift = false` ; `key` = `static_cast<Qt::Key>(c.unicode())`
  pour `c = text[0].toUpper()` si `c.unicode() < 0x100` (les touches Latin-1 de Qt valent leur majuscule : `Key_A`,
  `Key_Eacute`, `Key_Ccedilla`…), sinon `Qt::Key_unknown`. Commande ciblée `-R tst_features_keyboard` : `7 passed …
  0 skipped`.
- [ ] **Étape 5 : rouge de la roue.** `tests/tst_features_desktopwheel.cpp` :
  ```cpp
  #include <actcraft/features/desktopwheel.h>

  #include <QtTest>

  #include <cmath>

  using namespace actcraft::features;

  class TstFeaturesDesktopWheel : public QObject
  {
      Q_OBJECT

  private slots:
      void wheelIsClockwiseFromTop()
      {
          const auto p = wheelPositions(4, 100);
          QCOMPARE(p.size(), std::size_t{4});
          QVERIFY(std::abs(p[0].x() - 0) < 0.01 && std::abs(p[0].y() + 100) < 0.01); // en haut
          QVERIFY(std::abs(p[1].x() - 100) < 0.01 && std::abs(p[1].y() - 0) < 0.01); // puis à droite (y vers le bas)
      }
      void defaultWheelUsesDesktopAndToolCommands()
      {
          const auto wheel = defaultWheel();
          QVERIFY(!wheel.empty());
          QCOMPARE(wheel.front().command, QStringLiteral("desktop.restore"));
          std::function<void(const std::vector<WheelEntry>&)> check = [&](const std::vector<WheelEntry>& ring) {
              for (const auto& e : ring) {
                  QVERIFY(!e.label.isEmpty());
                  if (e.ring.empty())
                      QVERIFY(e.command.startsWith(QStringLiteral("desktop.")) || e.command.startsWith(QStringLiteral("tool.")));
                  else {
                      QVERIFY(e.command.isEmpty()); // un anneau secondaire n'est pas une commande
                      check(e.ring);
                  }
              }
          };
          check(wheel);
      }
  };

  QTEST_GUILESS_MAIN(TstFeaturesDesktopWheel)
  #include "tst_features_desktopwheel.moc"
  ```
  (`#include <functional>`.) Rouge.
- [ ] **Étape 6 : roue.** `features/include/actcraft/features/desktopwheel.h` :
  ```cpp
  #pragma once

  #include <QPointF>
  #include <QString>

  #include <vector>

  namespace actcraft::features {

  // Positions de `count` entrées sur un cercle de centre (0, 0), sens horaire depuis le haut (y vers le bas).
  std::vector<QPointF> wheelPositions(int count, double radius);

  // Entrée de la roue des outils de bureau (F-080) : une commande nommée, ou un anneau secondaire.
  struct WheelEntry
  {
      QString command; // vide pour un anneau secondaire
      QString label;
      std::vector<WheelEntry> ring;
  };

  std::vector<WheelEntry> defaultWheel();

  } // namespace actcraft::features
  ```
  `features/src/desktopwheel.cpp` : angle de l'entrée `i` = `−90° + i × 360° / count` (`count ≤ 0` → liste vide) ;
  `defaultWheel()`, dans l'ordre : `desktop.restore` « Revenir à ActCraft » ; anneau « Caméra » (`tool.camera-area`
  « Zone », `tool.camera-polygon` « Point par point », `tool.camera-freehand` « Main levée », `tool.camera-window`
  « Fenêtre », `tool.camera-screen` « Écran entier ») ; `tool.keyboard` « Clavier à l'écran » ; anneau « Autres
  outils » (`tool.ticker` « Message défilant », `tool.clock` « Horloge », `tool.calculator` « Calculatrice »,
  `tool.dice` « Dés »). Commande ciblée `-R 'tst_features_(camera|keyboard|desktopwheel)'` : trois `Totals` à
  `0 failed, 0 skipped`.
- [ ] **Étape 7 : preuves et commit.** `format.ps1`, preuve de tâche, `NOMS OK`, talons absents, commit
  « Lot 7, phase 1 : géométrie de la caméra, clavier à l'écran et roue de bureau (T28, T29, T31) (L7-P9) ».

---

## Ce que la phase 2 reprend (entrée de l'amendement de phase 2)

Règles générales. Base : `feat/livraison-complete-b` après fusion de `feat/livraison-complete` (socle mergé). Le
compte de tests devient celui de `feat/livraison-complete` socle compris, plus les 16 tests de cette phase (plus ceux
que la phase 2 déclare). `features/CMakeLists.txt` n'est plus touché (sources par motif, bibliothèques posées) ;
`tests/presentation.cmake` reçoit, à la suite des 16 lignes, les tests d'interface, `tst_features_equationeditor`,
`tst_core_equation`, `tst_render_equation` et les `tst_captures_*` du lot. Le reste de l'ancienne L0-S8 : talons des
contrôleurs de `ui/src/`, `ui/presentation.cmake` avec les noms figés `ACTCRAFT_UI_LOT7_QML` et
`ACTCRAFT_UI_LOT7_SOURCES` (sans relister `ui/qml/Presentation.qml`, créé par le socle L0-S9 avec ses six propriétés
`commands`, `documentController`, `tools`, `pageArea`, `pageOverlays`, `layer`), QML des surcouches,
`DocumentInsertions` (`ui/src/presentationdocument.*`, après L0-S5), couvertures `addInputCover` et tests au stylet,
au doigt et à la souris (B2), chacun vérifiant qu'aucun trait n'est versé **et** que le gestionnaire de la surcouche reçoit l'appui (socle 1ffb07b, point 5). Attentes : `addInputCover`, `setSampleFilter`, `zoomAround`, `sceneToPage` après L0-S7 ; `pageOverlays`, `presentationLayer`, `presentation` après L0-S9 ; les sous-clés inconnues de `"presentation"` sont gardées par `PagePresentation::unknownKeys` (rien à faire côté `features/`). Dans chaque section `L7-T*` ci-dessous, les étapes « test rouge de la logique » et
« logique » sont **faites** : les retirer, et retirer les fichiers `features/…` et `tests/tst_features_*` de la colonne
« fichiers touchés ».

| Tâche | Fait en phase 1 | Reste en phase 2 |
|---|---|---|
| L7-T3 | `Curtain`, `tst_features_curtain` (L7-P1) | `CurtainController`, `CurtainOverlay.qml`, couverture, `curtainNeverRefreshesTheCache`, `curtainBlocksTheStylus`, scénario `rideau`, `tst_captures_rideau` |
| L7-T4 | `Spotlight`, `tst_features_spotlight` (L7-P1) | `SpotlightController` (conversion `Spotlight::Shape` ↔ `core::SpotlightShape`, mêmes valeurs, `static_assert`), `SpotlightOverlay.qml` (contour `Spotlight::kDefaultOutline` = 10), curseurs selon la zone (P7), couverture, tests d'interface, scénario `spot` ; **capture de la zone éclairée** vers la page, entrée du menu du spot (P8 de L7-T0 ; réutilise `cropToPolygon` de L7-P9 ; ou avec la caméra en 7b) |
| L7-T5 | `PageZoom`, `tst_features_pagezoom` (L7-P2) | `PageZoomController` (`static_assert(PageZoom::kMinZoom == ui::kMinZoom)` et `kMaxZoom`), `PageZoomOverlay.qml`, `pageZoomRefreshesTheCacheOnceAtRelease` (exactement 1), `pageZoomLeavesTheDocumentUnchanged`, `pageZoomBlocksTheStylus` |
| L7-T6 | `resolvePageTools` sur `PageToolsSettings`, `tst_features_pagetools` (L7-P1) | `PageToolsSettings toPageToolsSettings(const core::PagePresentation&)` dans `pagetools.*` (`PageToolsMode` → `PageToolChoice` par valeur vérifiée ; `CurtainMode::Off` et `Full` → `Curtain::Start::Full`, `Top/Bottom/Left/Right` → même côté ; `spotlightFrame`), test `curtainModeOffMeansFull` dans `tst_features_pagetools` ; `PageToolsController`, lecture `.flipchart` (`pagePresentationRead`) **y compris la propriété de page 27** (outils de page, F-591 ; relevée par L7-T0, absente de notre documentation du format), mémorisation de la position du rideau pour la page (P9), `pageChangeAppliesPageTools` |
| L7-T7 | catalogue de 18 entrées (16 mouvements, `none`, `random`), `effectForLeaving`, `pickEffect`, durée, `tst_features_transitions` (L7-P2) | `TransitionController` (effet de la page quittée lu dans `PagePresentation::transition`, puis `pickEffect` pour `random`), `TransitionLayer.qml` (un mouvement par identifiant du catalogue), tests d'interface avec les identifiants du catalogue (`dissolve`, `slide-right` ; plus `fade` ni `slide-left`), lecture `.flipchart` de l'effet si L7-T0 l'établit |
| L7-T8 | rien | identifiants du scénario `effets` **déjà amendés dans 7a** (`view.page-effect dissolve`, `page.effect slide-right`, lecture `transition.played=dissolve`) ; `PageEffectsPicker.qml` : un délégué par entrée du catalogue (18) |
| L7-T9, L7-T10 | `StrokeGuide`, `GuideSet`, `Ruler`, `SetSquare`, `projectOnSegments` (L7-P3) | `MathToolsController` (`PageView::setSampleFilter`, tolérance = 16 px d'écran ÷ zoom, `UserState::setOpenMathTools`), `MathToolsLayer.qml`, `RulerTool.qml`, `SetSquareTool.qml` (translucidité `kDefaultToolOpacity` = 100 sur 255 ; curseurs par zone, P21), tests d'interface (dont « fermer la règle garde les traits », P6), scénario `regle` |
| L7-T11, L7-T12 | `Protractor`, `arcPath`, `ArcStyle`, `Compass` (L7-P4) | délégués QML, insertion de l'arc fermé ou plein et de la croix du centre (`DocumentInsertions`), `filledArcBecomesOneShape`, `markCentreAddsACross`, `compassStrokeIsACircle` |
| L7-T13 | `OriginMemory` (L7-P4) | alimentation depuis `PagePresentation::hasOrigin` / `origin`, `SetPagePresentationCommand` au relâché, `OriginMarker.qml`, `originSavedWithThePage` |
| L7-T14 | `Dice`, `tst_features_dice` (L7-P5) | `DiceController`, `DicePanel.qml`, `diceTotalWrittenOnThePage`, `diceBlocksTheStylus` ; nombre de dés et vitesse **mémorisés** dans les réglages (E5 : 1 dé au premier lancement, puis le dernier choisi), croix de fermeture (P4) |
| L7-T15 | `Calculator`, `tst_features_calculator` (L7-P5) | `CalculatorController` (lanceur injectable, `calc.exe`), `CalculatorPanel.qml` (deux lignes : `expression()` au-dessus de `display()`, touches « C » et « CE »), **transfert du calcul sur la page** par un bouton (`pageText()` posé par `DocumentInsertions::insertText`, une étape ; glisser vers la page : 7b, P3), `systemCalculatorLaunchedOnWindows`, `calculatorBlocksTheStylus` |
| L7-T16 | `Countdown`, `Stopwatch`, `formatHms`, `parseHms`, `tst_features_countdown` (L7-P5) | `ClockController` (actions de fin selon la spécification, E9 non tranché par L7-T0 : aucune, page suivante, page précédente ; « remettre la page dans son état enregistré » en L7-T34), `ClockPanel.qml`, `CountdownDialog.qml`, tests d'interface, scénario `compte-a-rebours` |
| L7-T18, L7-T19 | `TickerSettings`, `TickerRun`, `tickerToJson`, `tickerFromJson`, `tst_features_ticker` (L7-P6) | `TickerController` (`QSettings` `tools/ticker`), `TickerOverlay.qml`, `TickerDialog.qml` (E11 non tranché par L7-T0 : spécification, « Essayer » et « Appliquer » qui garde et ferme), `tickerNeverRefreshesTheCache`, `applyKeepsTheDefault`, scénario `message` |
| L7-T20 | **toute la tâche** (L7-P7), classe `Polygon` comprise (P16) | rien : la marquer faite ; L7-T21 range `RecognizedShape::Kind::Polygon` en `core::Shape` `Polygon` (ses `corners`) |
| L7-T21 | `RecognitionSession` (L7-P7) ; ses tests sont dans **`tst_features_recognitionsession`**, plus dans `tst_features_shapefinder` (preuve à corriger) | `RecognitionController` (délai commun par défaut `RecognitionSession::kDefaultDelayMs` = 1 s, réglable de 0 à 5 s ; 7b disait 5 s, 1 à 5), `replaceObjects` par `DocumentInsertions`, tests d'interface, scénario `reconnaissance-forme` |
| L7-T22 | **toute la tâche** (L7-P8), moteur COM compris | rien : la marquer faite |
| L7-T23 | le fil dédié STA `HandwritingThread` ; `recognitionRunsOnOneDedicatedThread` est dans **`tst_features_handwriting`** (à retirer de la preuve de `tst_ui_presentation`) | `RecognitionController` en mode écriture qui **possède** un `HandwritingThread` (aucun autre fil ni `moveToThread` à écrire), commande grisée par `probe()`, `RecognitionAlternatives.qml`, tests d'interface |
| L7-T26 | rien | toute la tâche (`equationeditor.*` créés par elle, pris par le motif ; `tst_features_equationeditor` à déclarer) |
| L7-T28 | `PolygonPicker`, `FreehandPicker`, `cropToPolygon`, `normalizedRect`, `toLogicalPixels`, `windowRectsInLogicalPixels` (L7-P9) | `topLevelWindows()` dans `camera.h`, `camera_win.cpp` **et** `camera_other.cpp` (créés ensemble ; `dwmapi` déjà lié), `CameraController`, QML, tests d'interface, contrôle à l'écran |
| L7-T29 | `ScreenKeyboard`, `tst_features_keyboard` (L7-P9) | `keyinjector_win.cpp` **et** `keyinjector_other.cpp` (ensemble), `KeyboardController`, `ScreenKeyboard.qml`, tests d'interface |
| L7-T31 | `wheelPositions`, `WheelEntry`, `defaultWheel` ; `wheelIsClockwiseFromTop` est dans **`tst_features_desktopwheel`** (plus dans `tst_ui_desktop`) | `wheelHasNoVoteNorVendorEntry` (identifiants contre le registre des commandes), `DesktopController` (roue), `DesktopToolsWheel.qml`, `restoreClosesTheWheel`, `wheelClosesWhenTheWindowIsRestored` |
| L7-T34 | rien | en plus de 7b : **symétries autour d'un axe qui passe par l'origine XY** quand elle est active (P20 de L7-T0 ; attend le lot 3 et les symétries du lot 5) |
| L7-T0 à L7-T2, L7-T17, L7-T24, L7-T25, L7-T27, L7-T30, L7-T32, L7-T33 | rien (L7-T0 : fait, rapport 0be9a77) | selon les amendements du socle (« Ce que la session B doit savoir », socle 1ffb07b) |

## Risques

1. **Motif au lieu d'une liste** dans `features/CMakeLists.txt` : écart à la convention des modules ; un `_win.cpp`
   sans son `_other.cpp` ne se voit qu'en CI macOS et Android.
2. **Avertissements de Clang** (`-Wconversion`, `-Wshadow`) invisibles en local : rattrapés par la CI de la PR finale.
3. **COM sous `/W4 /WX /permissive-`** (`msinkaut.h`) : un en-tête du SDK peut refuser ; et le reconnaisseur peut
   manquer sur le poste (Windows 11 sans pack d'écriture) : `recognizesSyntheticInk` sauté et signalé.
4. **Dix constructions complètes** (une par worktree neuf) sur une seule place de tests : la durée de la phase dépend
   d'elles plus que du code.
5. **Valeurs de L7-T0 lues en statique** (réglages embarqués, aucun désassemblage) : délai de reconnaissance,
   translucidité, contour du spot, catalogue. Une observation à l'écran qui les contredirait change une constante
   nommée, sans toucher aux tests de structure.
