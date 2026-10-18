# Livraison complète — lot 7 « Présentation et outils » — amendement « phase 2 » (après le socle) — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**Statut : BROUILLON.** Écrit avant que le socle L0-S (exécuté par la session A sur `feat/livraison-complete`) soit
mergé. Il sera **revu contre le socle réel à la fusion**, puis relu (hook `relire-plan.js`) avant toute exécution.
Chaque point qui dépend d'un code du socle non encore vérifiable porte la marque **[SOCLE?]** ; la liste complète est
en fin de plan (« À vérifier contre le socle réel à la fusion »).

**Ce que c'est.** Un amendement aux plans du lot 7 : `docs/plans/2026-10-18-livraison-complete-lot7-PLAN.md` (« 7a »)
et `docs/plans/2026-10-18-livraison-complete-lot7b-PLAN.md` (« 7b »). Il **ne les réécrit pas** : pour chaque tâche
`L7-T*`, il dit ce qu'elle garde, ce qui disparaît (fait par le socle ou par la phase 1) et ce qui s'ajoute (rapport
L7-T0, amendements du socle). À l'exécution, la section d'une tâche = **sa section de 7a ou 7b, corrigée par sa
section ici** ; en cas de contradiction, ce plan l'emporte. Il ajoute une tâche, **L7-S8**, qui reprend l'ancienne
L0-S8 du socle (squelette du lot 7).

**Autorités, dans l'ordre.**
1. Le plan du socle, version de `feat/livraison-complete` (794cf84), sections « Contrats figés par le socle (noms) »,
   « Amendements à porter une fois le socle mergé », partie « Plans du lot 7 », et « Ce que la session B doit
   savoir » (copie : `C:\Users\JEROME~1\AppData\Local\Temp\claude\C--Users-JeromeDenis-pilotage\67662d88-b0d4-48e1-9ce6-e529d6829fab\scratchpad\socle-PLAN-794cf84.md`,
   citée « socle:<ligne> ») ; **à la fusion, le code mergé fait foi sur le plan**.
2. L'addendum 1 de A (`…\fd5ef06a-7b9b-46ba-a8c3-b06809fefd99\scratchpad\brief-b-addendum-1.md`) et tout addendum
   ultérieur du même dossier.
3. Le brief de B, section « Phase 2 » (`docs/sessions/livraison-complete-b-brief.md`).
4. Ce plan.
5. Le plan de la phase 1 (`docs/plans/2026-10-18-livraison-complete-lot7-phase1-PLAN.md`), section « Ce que la
   phase 2 reprend » ; L7-P0 à L7-P9 sont tenues pour **faites** telles que décrites par leur section.
6. Le rapport L7-T0 (`docs/verifications/2026-10-18-lot7-principes.md`), section « Écarts pour l'orchestrateur ».
7. Les sections `L7-T*` de 7a et 7b, pour tout ce que ce plan ne redit pas.

**Ancienne L0-S8** (texte de référence, retirée du socle par 1880a6f) : `git -C <wt> show aa621f1:docs/plans/2026-10-18-livraison-complete-socle-3-7-PLAN.md`,
section « Tâche L0-S8 — Squelette du lot 7 » (citée « S8a:<ligne> »). L7-S8 en reprend le contrat de
`DocumentInsertions` mot pour mot et la liste des QML ; le reste est adapté (phase 1 faite, noms figés du socle).

**Branche et worktrees.** Base : `feat/livraison-complete-b` **après** la fusion de `feat/livraison-complete` (socle
mergé, conflit unique attendu de `tests/CMakeLists.txt` résolu : commentaire du socle gardé, une seule ligne
d'inclusion), suites debug `TESTS OK`, `NOMS OK` (brief, phase 2, point 1 ; fait par l'orchestrateur, hors tâches).
Une tâche = un worktree `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b--L7-<id>`, branche
`feat/livraison-complete-b--L7-<id>`, partie de `feat/livraison-complete-b` à jour des tâches mergées. Merge
`--no-ff` par l'orchestrateur, puis `graphify update .` et commit de `graphify-out/`.

**Pile.** C++20, Qt 6.12 (Core, Gui, Quick, QuickControls2, Svg, Test), CMake + Ninja, MSVC 2022, PowerShell 7.

## Décisions de cet amendement

1. **Fichiers propres à chaque fonction, pour tenir deux tâches de front.** Le profil permet deux tâches à la fois
   (`.claude/chantier.md`, « Parallélisme ») ; 7a et 7b les enchaînaient toutes parce qu'elles partageaient
   `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `tests/presentation.cmake` et `tests/tst_ui_presentation.cpp`.
   Après L7-S8, **aucune tâche ne touche plus ces quatre fichiers**, sauf L7-T2 (`tst_ui_presentation.cpp`, ses
   fonctions), L7-T17 (`Presentation.qml`, un bloc) et L7-T32/L7-T34 (en bout de chaîne, seules). Pour cela :
   - **Une racine QML par fonction**, instanciée une fois par L7-S8 dans `Presentation.qml`, qui porte le contrôleur
     de la fonction et **ses gestionnaires de commandes** (`commands.setHandler` dans son `Component.onCompleted`).
     Les sections de 7a/7b qui disent « gestionnaire dans `Presentation.qml` » se lisent « dans la racine de la
     fonction » (table de la décision 3).
   - **Un test d'interface par fonction** (`tst_ui_<fonction>`), au lieu des fonctions ajoutées une à une dans
     `tst_ui_presentation` : deux tâches de front ne peuvent pas écrire dans le même fichier de test.
     `tst_ui_presentation` ne garde que les fonctions de L7-S8, de L7-T2 et de L7-T32.
   - **Sources C++ du lot listées par motifs nommés** dans `ui/presentation.cmake` (`file(GLOB … CONFIGURE_DEPENDS)`
     sur les noms exacts `src/<nom>controller.h`, `src/<nom>controller.cpp`, …) : la tâche crée son contrôleur, CMake le
     voit à la construction suivante, et aucun `.cpp` talon vide n'est nécessaire (un `.cpp` sans symbole ferait dire
     `LNK4221` à `lib.exe`, constat de la phase 1, décision 5). Les QML, eux, sont listés en entier (les racines
     doivent exister dès L7-S8 pour que `Presentation.qml` se charge).
   - **Tous les tests du lot déclarés par L7-S8** dans `tests/presentation.cmake`, chacun talon `reserved()` sauté :
     le compte est figé dès L7-S8 (« = », pas « ≥ »).
2. **Talons du lot 7.** Marqueur unique `Talon L7-S8 : rempli par L7-T<n>.` (commentaire C++ ou QML, champ
   `description` d'un scénario, texte du `QSKIP` de `reserved()`), sur le modèle de la règle d'honnêteté du socle
   (socle:131-153). La tâche propriétaire supprime le marqueur et `reserved()`. Contrôle de clôture du lot :
   `git -C <wt> grep -n "Talon L7-S8 : rempli par" -- . ':(exclude)docs/'` vide, et
   `git -C <wt> grep -n "Talon L0-S : rempli par L7-" -- . ':(exclude)docs/'` vide (talons du lot 7 posés par le
   socle, s'il y en a : **[SOCLE?]** `core/equation.*`, `render/src/equationlayout.*`).
3. **Racines et emplacements** (posés par L7-S8 ; parent tiré des sections de 7a/7b) :

   | Racine (`ui/qml/`) | Tâche qui la remplit | Parent | Contrôleur(s) qu'elle porte |
   |---|---|---|---|
   | `CurtainOverlay.qml` | L7-T3 | `presentation.layer` (7a:1062) | `CurtainController` |
   | `SpotlightOverlay.qml` | L7-T4 | `presentation.layer` (mêmes règles que le rideau, 7a:1192) | `SpotlightController` |
   | `PageZoomOverlay.qml` | L7-T5 | `presentation.pageArea.presentationSlot` (coordonnées de la zone, socle L0-S9) | `PageZoomController` |
   | `PageToolsHost.qml` | L7-T6 | `presentation` (sans dessin) | `PageToolsController` |
   | `TransitionLayer.qml` | L7-T7 | `presentation.layer` (7a:1477) | `TransitionController` ; enveloppe `page.next` / `page.previous` |
   | `PageEffectsPicker.qml` | L7-T8 | `presentation.layer` | — (lit `transitionRoot.controller`) |
   | `MathToolsLayer.qml` | L7-T9 (puis T10 à T13) | `presentation.pageOverlays` (7a:1708) | `MathToolsController` |
   | `DicePanel.qml` | L7-T14 | `presentation.layer` | `DiceController` |
   | `CalculatorPanel.qml` | L7-T15 | `presentation.layer` | `CalculatorController` |
   | `ClockPanel.qml` | L7-T16 | `presentation.layer` | `ClockController` |
   | `TickerOverlay.qml` | L7-T18 (puis T19) | `presentation.layer` (7a:2245) | `TickerController` |
   | `RecognitionAlternatives.qml` | L7-T21 (puis T23) | `presentation.layer` | `RecognitionController` |
   | `EquationEditor.qml` | L7-T26 (puis T27) | `presentation.layer` | `EquationController` |
   | `CameraOverlay.qml` | L7-T28 | `presentation.layer` | `CameraController` |
   | `ScreenKeyboard.qml` | L7-T29 | sa fenêtre propre (7b, L7-T29 : ne prend jamais le focus) | `KeyboardController` |
   | `DesktopAnnotation.qml` | L7-T30 (puis T31) | sa fenêtre propre | `DesktopController` |
   | `OtherToolsHost.qml` | L7-T17 | `presentation` (sans dessin) | `PresentationCommands` |

   QML non racines (créés en talon par L7-S8, instanciés par leur racine) : `FloatingPanel.qml` (T2),
   `RulerTool.qml` (T9), `SetSquareTool.qml` (T10), `ProtractorTool.qml` (T11), `CompassTool.qml` (T12),
   `OriginMarker.qml` (T13), `CountdownDialog.qml` (T16), `TickerDialog.qml` (T19), `CameraPalette.qml` (T28),
   `DesktopToolsWheel.qml` (T31). `PageToolsHost.qml` et `OtherToolsHost.qml` sont deux noms neufs (fonctions sans
   QML dans 7a) : contrôle des noms dans L7-S8. Avec `OtherToolsHost.qml`, L7-T17 ne touche plus `Presentation.qml`
   (la décision 1 garde l'exception par prudence, si le contrôleur doit y être nommé).
4. **Contrat d'une racine talon** (L7-S8) — chaque racine déclare, et seulement cela :
   ```qml
   import QtQuick
   import ActCraft.Ui

   // Talon L7-S8 : rempli par L7-T<n>.
   Item {
       id: root
       property Commands commands: null
       property DocumentController documentController: null
       property Tools tools: null
       property Item pageArea: null
       property QtObject controller: null // la tâche y lie son contrôleur : controller: curtainController
   }
   ```
   plus, pour les racines qui lisent une autre fonction : `property QtObject curtain: null` et
   `property QtObject spotlight: null` (`PageToolsHost.qml`), `property QtObject curtain: null`
   (`TransitionLayer.qml`), `property QtObject transitions: null` (`PageEffectsPicker.qml`). La tâche **ne retire
   aucune** de ces propriétés (`Presentation.qml` les pose) ; elle en ajoute à sa guise.
5. **Couvertures (B2, socle « Changent » 5).** Toute surcouche ou tout panneau qui prend l'entrée appelle
   `pageArea.view.addInputCover(<item>)` à son ouverture et `removeInputCover` à sa fermeture **[SOCLE?]** (signature
   `addInputCover(QQuickItem*)`, accessible en QML depuis `pageArea.view`). `FloatingPanel.qml` le fait pour tous les
   panneaux (L7-T2). Chaque tâche a son test `<fonction>BlocksTheStylus` **à données**, trois lignes `stylet`
   (`QTabletEvent`), `doigt` (`QTouchEvent`), `souris` (`QMouseEvent`), injectées sous la surcouche par les aides de
   `tests/support/presentationharness.h` (L7-S8) ; attendu : nombre d'objets de la page inchangé, encre vide,
   **et** l'effet du gestionnaire de la surcouche (bord tiré, outil déplacé, panneau déplacé, bouton pressé), lu par
   `objectName`. Un appui couvert du stylet laisse les doigts libres (socle D-S1) : vérifié une fois, dans
   `floatingPanelIsACover` (L7-T2).
6. **Ce qui sort de toutes les colonnes « fichiers touchés »** : `ui/CMakeLists.txt`, `ui/icons/*` (dessinées par
   L0-S1 ; une tâche qui redessine **son** icône l'ajoute à sa ligne avant lancement, socle « Changent » 4),
   `features/CMakeLists.txt` (sources par motif, bibliothèques posées : phase 1, décision 5), `ui/qml/Presentation.qml`
   (sauf L7-S8, L7-T17, L7-T34), `ui/presentation.cmake`, `tests/presentation.cmake` (L7-S8 seulement),
   `tests/tst_ui_presentation.cpp` (L7-S8, L7-T2, L7-T32), `features/…` et `tests/tst_features_*` déjà faits en
   phase 1 (sauf ajout nommé dans une section).
7. **Interdits de B posés par le socle** (socle:1478-1482) : `core/`, `formats/`, `render/`, `actions/data/*`,
   `ui/icons/*`, `ui/CMakeLists.txt`, `tests/{core,input,tools,render,ui,bench}.cmake`, `Main.qml`, `PageArea.qml`,
   `ToolPalette.qml`, `pageview.*`, `documentcontroller.*` ; ne jamais recréer `PagePresentation`, `CurtainMode`,
   `SpotlightShape`, `PageToolsMode`, `Equation`, `ReplaceObjectsCommand`, `SetPagePresentationCommand`.
   **Exceptions nommées**, et seulement elles : `formats/src/flipchart.cpp`, `formats/src/flipchartids.h`,
   `tests/support/flipchartwriter.h`, `tests/tst_formats_flipchart.cpp` (L7-T1 réduite, socle « Disparaissent » 2 ;
   L7-T6) ; `core/include/actcraft/core/equation.h` et `core/src/equation.cpp` pour l'**arbre** d'équation seulement
   (L7-T24 ; `struct Equation` du modèle inchangée) ; `render/src/equationlayout.h`, `render/src/equationlayout.cpp`
   (L7-T25). **[SOCLE?]** : que ces fichiers existent, soient enregistrés, et ce que L0-S3 y a mis.
8. **Commandes.** Les commandes du lot sont dans `actions/data/commands.json` (L0-S4) ; B n'en ajoute aucune. Les
   actions neuves de L7-T0 sans commande au socle (mémoriser le rideau pour la page, P9 ; capturer la zone du spot,
   P8 ; poser le calcul sur la page, P3) sont des entrées du **menu propre** de la surcouche ou des boutons du
   panneau ; leur effet passe par une commande d'annulation nommée de `core` (`SetPagePresentationCommand`,
   `AddObjectCommand`) via `DocumentInsertions`, une étape. **[SOCLE?]** Si L0-S4 a posé une commande pour l'une
   d'elles, la tâche l'utilise et branche son gestionnaire dans sa racine.
9. **Valeurs de L7-T0 déjà dans la logique** (phase 1, décision 9) : rien à refaire côté `features/` ; les tâches
   d'interface les **lisent** par leurs constantes nommées (`Spotlight::kDefaultOutline`, `kDefaultToolOpacity`,
   `RecognitionSession::kDefaultDelayMs`, catalogue de `transitions.h`, `Dice` à 1 dé) et ne les recopient jamais.
10. **L7-T20 et L7-T22 sont faites** (L7-P7, L7-P8) : elles n'ont plus de ligne. Leurs dépendants (L7-T21, L7-T23)
    dépendent de L7-S8.
11. **Hypothèses ajoutées** (rapport L7-T0, E9 et E11) : **H7-14** — actions de fin du compte à rebours : celles de la
    spécification (aucune, page d'après, page d'avant ; « remettre la page dans son état enregistré » en L7-T34) ;
    **H7-15** — validation du message défilant : un bouton « Essayer » et un bouton « Appliquer » qui garde le message
    comme défaut et ferme la boîte. Elles s'ajoutent à « Hypothèses à revoir » de 7b (porte de fin du lot 7).
12. **Deux lots** (plus de 20 tâches) : **P2-A** = L7-S8, L7-T1 à L7-T19 (20 tâches, ancien 7a) ; **P2-B** = L7-T21,
    L7-T23 à L7-T34 (13 tâches, ancien 7b). Un seul fichier, à la demande de l'orchestrateur ; les lots s'exécutent
    l'un après l'autre.

## Compte de tests

- **N_F** = nombre de tests ctest de `feat/livraison-complete-b` juste après la fusion du socle, relevé par
  l'orchestrateur et inscrit dans l'état. Attendu : **N_F = N_départ + 19 (socle) + 16 (phase 1)**, soit 129 si
  N_départ = 94 **[SOCLE?]** (N_départ et tout test ajouté par A depuis : à relever, pas à supposer).
- **L7-S8 déclare 29 tests** : **N = N_F + 29** pour L7-S8 et **toutes** les tâches suivantes jusqu'à L7-T32
  (« = »). n_B = 16 + 29 = **45**.
- Les 29 : 17 d'interface — `tst_ui_presentation` (L7-S8, L7-T2, L7-T32), `tst_ui_curtain` (T3),
  `tst_ui_spotlight` (T4), `tst_ui_pagezoom` (T5), `tst_ui_pagetools` (T6), `tst_ui_transitions` (T7, T8),
  `tst_ui_mathtools` (T9 à T13), `tst_ui_dice` (T14), `tst_ui_calculator` (T15), `tst_ui_clock` (T16),
  `tst_ui_othertools` (T17), `tst_ui_ticker` (T18, T19), `tst_ui_recognition` (T21, T23), `tst_ui_equation` (T26,
  T27), `tst_ui_camera` (T28), `tst_ui_keyboard` (T29), `tst_ui_desktop` (T30, T31) ; 3 de logique —
  `tst_core_equation` (T24), `tst_render_equation` (T25), `tst_features_equationeditor` (T26) ; 9 de captures —
  `tst_captures_rideau` (T3), `tst_captures_spot` (T4), `tst_captures_effets` (T8), `tst_captures_regle` (T9),
  `tst_captures_compte_a_rebours` (T16), `tst_captures_message` (T19), `tst_captures_reconnaissance_forme` (T21),
  `tst_captures_equation` (T27), `tst_captures_presentation` (T32).
- `tst_core_presentation` (7a, L7-T1) disparaît (socle « Disparaissent » 2) : il n'est pas déclaré.
- Après L7-T33 : **N ≥ N_A + 45**, où N_A = compte de `feat/livraison-complete` à la livraison du lot 3 (socle
  compris ; le lot 3 n'enregistre aucun test, socle:49, mais A peut en avoir ajouté).

## Table des tâches — lot P2-A (L7-S8, L7-T1 à L7-T19)

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L7-S8 | Squelette du lot 7 (ancienne L0-S8) : `Presentation.qml` et 17 racines, 10 QML talons, `ui/presentation.cmake` aux noms figés, `DocumentInsertions` (en-tête figé, corps talon), aides de test, 29 tests et 9 scénarios talons | fusion du socle dans `feat/livraison-complete-b` | `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `tests/presentation.cmake`, `ui/src/presentationdocument.h`, `ui/src/presentationdocument.cpp`, `ui/qml/{CurtainOverlay,SpotlightOverlay,PageZoomOverlay,PageToolsHost,TransitionLayer,PageEffectsPicker,MathToolsLayer,DicePanel,CalculatorPanel,ClockPanel,TickerOverlay,RecognitionAlternatives,EquationEditor,CameraOverlay,ScreenKeyboard,DesktopAnnotation,OtherToolsHost}.qml`, `ui/qml/{FloatingPanel,RulerTool,SetSquareTool,ProtractorTool,CompassTool,OriginMarker,CountdownDialog,TickerDialog,CameraPalette,DesktopToolsWheel}.qml`, `tests/support/presentationharness.h`, `tests/tst_ui_{presentation,curtain,spotlight,pagezoom,pagetools,transitions,mathtools,dice,calculator,clock,othertools,ticker,recognition,equation,camera,keyboard,desktop}.cpp`, `tests/tst_core_equation.cpp`, `tests/tst_render_equation.cpp`, `tests/tst_features_equationeditor.cpp`, `bench/scenarios/{rideau,spot,effets,regle,compte-a-rebours,message,reconnaissance-forme,equation,presentation}.json` | isolé | concepteur-critique | `TESTS OK`, **N = N_F + 29**, les 29 tests neufs Passed ; 19 lignes `SKIP   : …reserved() Talon L7-S8 : rempli par L7-T<n>.` (16 `tst_ui_*` hors `tst_ui_presentation`, 3 de logique) ; `tst_ui_presentation` : PASS de `lot7RootsExist`, `presentationLoadsWithoutQmlWarning`, plus son `reserved()` (L7-T2) ; 9 `tst_captures_*` Passed (`waitReady`) ; `premiers-pas` hors écran `CAPTURES OK` (`LU pageIndicator.text=1 / 2 OK`) ; `NOMS OK` après `git add` et après le commit | S10, S22, S23, S24 (support) | #tache-l7-s8 |
| L7-T1 | Objet MathML des `.flipchart` lu en équation (reste de L7-T1) | L7-S8 | `formats/src/flipchart.cpp`, `formats/src/flipchartids.h`, `tests/support/flipchartwriter.h`, `tests/tst_formats_flipchart.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_formats_flipchart` Passed, PASS de `mathmlObjectBecomesEquation` | S24 | #tache-l7-t1 |
| L7-T2 | Insertions du lot (`DocumentInsertions`), panneau flottant couverture, observateur des traits | L7-S8 | `ui/src/presentationdocument.cpp`, `ui/qml/FloatingPanel.qml`, `tests/tst_ui_presentation.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_presentation` 0 skipped, PASS de `insertTextIsUndoneAtOnce`, `insertObjectOnNewPageIsUndoneAtOnce`, `replaceObjectsIsUndoneAtOnce`, `setPagePresentationIsUndoneAtOnce`, `changeObserverSeesEveryStroke`, `floatingPanelIsACover(stylet)`, `(doigt)`, `(souris)`, `coveredStylusLeavesFingersFree` ; banc hors écran joint | S10 | #tache-l7-t2 |
| L7-T3 | Rideau (F-265) : contrôleur, surcouche couverture, scénario | L7-S8 | `ui/src/curtaincontroller.h`, `ui/src/curtaincontroller.cpp`, `ui/qml/CurtainOverlay.qml`, `tests/tst_ui_curtain.cpp`, `bench/scenarios/rideau.json` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_curtain` 0 skipped, PASS de `curtainNeverRefreshesTheCache`, `curtainBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; `tst_captures_rideau` Passed ; captures `rideau` hors écran `LU curtain.coverage=…` puis `PIXEL` couvert et découvert OK ; banc hors écran joint | S10 | #tache-l7-t3 |
| L7-T4 | Spot (F-270) : contrôleur, surcouche (contour 10, curseurs par zone), capture de la zone éclairée | L7-T2 | `ui/src/spotlightcontroller.h`, `ui/src/spotlightcontroller.cpp`, `ui/qml/SpotlightOverlay.qml`, `tests/tst_ui_spotlight.cpp`, `bench/scenarios/spot.json` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_spotlight` 0 skipped, PASS de `spotlightNeverRefreshesTheCache`, `spotlightBlocksTheStylus(stylet)`, `(doigt)`, `(souris)`, `spotlightOutlineIsTheDefault`, `spotlightCursorFollowsTheZone`, `spotlightCaptureIsUndoneAtOnce` ; `tst_captures_spot` Passed ; banc hors écran joint | S10 | #tache-l7-t4 |
| L7-T5 | Zoom de page (F-260) : contrôleur sur `zoomAround`, surcouche couverture (appui long droit compris) | L7-S8 | `ui/src/pagezoomcontroller.h`, `ui/src/pagezoomcontroller.cpp`, `ui/qml/PageZoomOverlay.qml`, `tests/tst_ui_pagezoom.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_pagezoom` 0 skipped, PASS de `pageZoomRefreshesTheCacheOnceAtRelease` (exactement 1), `pageZoomLeavesTheDocumentUnchanged`, `pageZoomBlocksTheStylus(stylet)`, `(doigt)`, `(souris)`, `rightHoldIsTakenByTheCover` ; banc hors écran joint | S10 | #tache-l7-t5 |
| L7-T6 | Outils de page par page (F-591 à F-593) : conversion depuis `PagePresentation`, contrôleur, lecture `.flipchart` (propriété 27 comprise), mémoriser le rideau pour la page | L7-T1, L7-T2, L7-T3, L7-T4 | `features/include/actcraft/features/pagetools.h`, `features/src/pagetools.cpp`, `tests/tst_features_pagetools.cpp`, `ui/src/pagetoolscontroller.h`, `ui/src/pagetoolscontroller.cpp`, `ui/qml/PageToolsHost.qml`, `tests/tst_ui_pagetools.cpp`, `formats/src/flipchart.cpp`, `formats/src/flipchartids.h`, `tests/support/flipchartwriter.h`, `tests/tst_formats_flipchart.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_features_pagetools` 0 skipped dont PASS de `curtainModeOffMeansFull`, `conversionKeepsEveryValue` ; `tst_formats_flipchart` PASS de `pagePresentationRead`, `pageToolsPropertyRead` ; `tst_ui_pagetools` PASS de `pageChangeAppliesPageTools`, `rememberCurtainIsUndoneAtOnce` ; banc hors écran joint | S10 | #tache-l7-t6 |
| L7-T7 | Effets de changement de page I (F-640) : lecteur en surcouche, effet de la page quittée, enveloppe de `page.next` et `page.previous` | L7-T3 | `ui/src/transitioncontroller.h`, `ui/src/transitioncontroller.cpp`, `ui/qml/TransitionLayer.qml`, `tests/tst_ui_transitions.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_transitions` 0 skipped, PASS de `transitionDoesNotRefreshTheCacheBeforeTheEnd`, `snapshotShowsAPendingStroke`, `fastRepeatLosesNoPage`, `noEffectWhenNone`, `pageEffectReadFromThePresentation`, `noEffectWhileCurtainActive` ; banc hors écran joint | S22 | #tache-l7-t7 |
| L7-T8 | Effets II : navigateur `PageEffectsPicker.qml` (18 entrées), scénario `effets` | L7-T7 | `ui/qml/PageEffectsPicker.qml`, `tests/tst_ui_transitions.cpp`, `bench/scenarios/effets.json` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_transitions` PASS de `browserListsEveryEffect` (18 délégués), `browserChoiceSetsTheGlobalEffect`, `pageEffectsPickerBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; `tst_captures_effets` Passed ; captures `effets` hors écran `LU transition.played=dissolve OK` | S22 | #tache-l7-t8 |
| L7-T9 | Règle (F-280) : contrôleur des outils de maths sur `setSampleFilter`, couche, curseurs par zone, translucidité 100 | L7-S8 | `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/MathToolsLayer.qml`, `ui/qml/RulerTool.qml`, `tests/tst_ui_mathtools.cpp`, `bench/scenarios/regle.json` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_mathtools` 0 skipped, PASS de `rulerGuidesThePenWithoutChangingIt`, `syntheticReleaseStaysOnTheGuide`, `mathToolIsNotSaved`, `closingTheRulerKeepsTheStroke`, `openToolsKeptInTheUserState`, `mathToolOpacityIsTheDefault`, `mathToolCursorFollowsTheZone`, `rulerBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; `tst_captures_regle` Passed ; banc hors écran joint | S10 | #tache-l7-t9 |
| L7-T10 | Équerre (F-283) : délégué et guidage | L7-T9 | `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/MathToolsLayer.qml`, `ui/qml/SetSquareTool.qml`, `tests/tst_ui_mathtools.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_mathtools` PASS de `setSquareGuidesThePen`, `setSquareBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; banc hors écran joint | S10 | #tache-l7-t10 |
| L7-T11 | Rapporteur (F-281) : délégué, rotation, arc fermé ou plein inséré | L7-T10, L7-T2 | `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/MathToolsLayer.qml`, `ui/qml/ProtractorTool.qml`, `tests/tst_ui_mathtools.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_mathtools` PASS de `filledArcBecomesOneShape`, `closedArcIsUndoneAtOnce`, `protractorTurnsByItsHandle`, `protractorBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; banc hors écran joint | S10 | #tache-l7-t11 |
| L7-T12 | Compas (F-282) : délégué, mine à la couleur du stylo, croix du centre | L7-T11 | `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/MathToolsLayer.qml`, `ui/qml/CompassTool.qml`, `tests/tst_ui_mathtools.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_mathtools` PASS de `markCentreAddsACross`, `compassStrokeIsACircle`, `compassLeadTakesThePenColour`, `compassBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; banc hors écran joint | S10 | #tache-l7-t12 |
| L7-T13 | Origine XY (F-284) : marqueur, mémoire par page dans `PagePresentation` | L7-T12 | `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/MathToolsLayer.qml`, `ui/qml/OriginMarker.qml`, `tests/tst_ui_mathtools.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_mathtools` PASS de `originSavedWithThePage`, `originMoveIsUndoneAtOnce`, `originMarkerBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` | S10 | #tache-l7-t13 |
| L7-T14 | Dés (F-285, E5) : panneau, total écrit, nombre et vitesse mémorisés | L7-T2 | `ui/src/dicecontroller.h`, `ui/src/dicecontroller.cpp`, `ui/qml/DicePanel.qml`, `tests/tst_ui_dice.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_dice` 0 skipped, PASS de `diceTotalWrittenOnThePage`, `oneDieTheFirstTime`, `diceCountAndSpeedRemembered`, `dicePanelClosesByItsCross`, `diceBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` | S10 | #tache-l7-t14 |
| L7-T15 | Calculatrice (F-286) : panneau à deux lignes, deux effacements, calcul posé sur la page | L7-T2 | `ui/src/calculatorcontroller.h`, `ui/src/calculatorcontroller.cpp`, `ui/qml/CalculatorPanel.qml`, `tests/tst_ui_calculator.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_calculator` 0 skipped, PASS de `systemCalculatorLaunchedOnWindows`, `panelShowsTheCalculationAboveTheResult`, `clearEntryAndClearAllAreTwoKeys`, `calculationToPageIsUndoneAtOnce`, `calculatorBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` | S10 | #tache-l7-t15 |
| L7-T16 | Horloge et compte à rebours (F-290, H7-14) : panneau, boîte, actions de fin, scénario | L7-T2 | `ui/src/clockcontroller.h`, `ui/src/clockcontroller.cpp`, `ui/qml/ClockPanel.qml`, `ui/qml/CountdownDialog.qml`, `tests/tst_ui_clock.cpp`, `bench/scenarios/compte-a-rebours.json` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_clock` 0 skipped, PASS de `countdownEndGoesToNextPage`, `endActionsAreTheSpecification`, `clockBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; `tst_captures_compte_a_rebours` Passed (`LU pageIndicator.text=2 / 2 OK` après 3 s) | S10 | #tache-l7-t16 |
| L7-T17 | Date et heure (F-300), navigateur web (F-299) | L7-T2 | `ui/src/presentationcommands.h`, `ui/src/presentationcommands.cpp`, `ui/qml/OtherToolsHost.qml`, `tests/tst_ui_othertools.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_othertools` 0 skipped, PASS de `dateTimeInsertsOneText`, `webBrowserOpensTheHomeUrl`, `webBrowserRefusesOtherSchemes` | S10 | #tache-l7-t17 |
| L7-T18 | Message défilant I (F-291) : contrôleur, surcouche animée, banc | L7-S8 | `ui/src/tickercontroller.h`, `ui/src/tickercontroller.cpp`, `ui/qml/TickerOverlay.qml`, `tests/tst_ui_ticker.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_ticker` 0 skipped, PASS de `tickerNeverRefreshesTheCache`, `tickerBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; banc hors écran avec message en boucle joint | S23 | #tache-l7-t18 |
| L7-T19 | Message défilant II (F-291, H7-15) : boîte d'édition, « Essayer » et « Appliquer », fichiers de messages, scénario | L7-T18, L7-T2 | `ui/src/tickercontroller.h`, `ui/src/tickercontroller.cpp`, `ui/qml/TickerOverlay.qml`, `ui/qml/TickerDialog.qml`, `tests/tst_ui_ticker.cpp`, `bench/scenarios/message.json` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_ticker` PASS de `applyKeepsTheDefault`, `tryDoesNotKeepTheDefault`, `messageFileLoadedFromTheDialog`, `tickerDialogBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; `tst_captures_message` Passed | S23 | #tache-l7-t19 |

20 tâches dans le lot P2-A.

## Table des tâches — lot P2-B (L7-T21, L7-T23 à L7-T34)

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L7-T21 | Reconnaissance de forme II : contrôleur (délai 1 s, 0 à 5), trait remplacé par la forme en une étape, polygone, scénario | lot P2-A | `ui/src/recognitioncontroller.h`, `ui/src/recognitioncontroller.cpp`, `ui/qml/RecognitionAlternatives.qml`, `tests/tst_ui_recognition.cpp`, `bench/scenarios/reconnaissance-forme.json` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_recognition` PASS de `recognizedStrokeMakesOneStep`, `fillOptionFillsTheShape`, `roughQuadrilateralBecomesAPolygonShape`, `controllerDelayIsTheSessionDefault` ; `tst_captures_reconnaissance_forme` Passed ; banc hors écran joint | S10 | #tache-l7-t21 |
| L7-T23 | Reconnaissance d'écriture II : mode écriture qui possède un `HandwritingThread`, mot → texte en une étape, alternatives couverture, commande grisée, absente d'Android | L7-T21 | `ui/src/recognitioncontroller.h`, `ui/src/recognitioncontroller.cpp`, `ui/qml/RecognitionAlternatives.qml`, `tests/tst_ui_recognition.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_recognition` PASS de `handwritingMakesOneStep` (ou SKIP « reconnaisseur absent », signalé), `handwritingDisabledWithExplanation`, `handwritingAbsentOnAndroid`, `controllerOwnsOneHandwritingThread`, `clickOutsideValidatesTheText`, `alternativesBlockTheStylus(stylet)`, `(doigt)`, `(souris)` ; banc hors écran joint | S10 | #tache-l7-t23 |
| L7-T24 | Équation I (F-301) : arbre et MathML (17 modèles) | lot P2-A | `core/include/actcraft/core/equation.h`, `core/src/equation.cpp`, `tests/tst_core_equation.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_core_equation` 0 skipped, PASS de `templatesRoundTrip` (17 lignes de données), `unknownElementKeptVerbatim`, `entitiesDecoded`, `malformedIsAnErrorNotACrash`, `nonAsciiRawKeptVerbatim` | S24 | #tache-l7-t24 |
| L7-T25 | Équation II : mise en page et dessin | L7-T24 | `render/src/equationlayout.h`, `render/src/equationlayout.cpp`, `tests/tst_render_equation.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_render_equation` 0 skipped, PASS de `fractionIsTallerThanItsParts`, `superscriptIsRaised`, `rootCoversItsContent`, `malformedDrawsTheNeutralFrame`, `paintedMatchesSoftware`, `paintFromTwoThreads`, `fixedPageDrawsTheEquation` ; banc hors écran joint | S24 | #tache-l7-t25 |
| L7-T26 | Équation III : éditeur à modèles, palettes du rapport, aperçu, insertion | L7-T25 | `features/include/actcraft/features/equationeditor.h`, `features/src/equationeditor.cpp`, `tests/tst_features_equationeditor.cpp`, `ui/src/equationcontroller.h`, `ui/src/equationcontroller.cpp`, `ui/src/equationpreviewitem.h`, `ui/src/equationpreviewitem.cpp`, `ui/qml/EquationEditor.qml`, `tests/tst_ui_equation.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_features_equationeditor` 0 skipped, PASS de `fractionTemplateThenTwoDigits`, `backspaceLeavesAPlaceholder`, `arrowsWalkTheTree`, `everyTemplateHasAPaletteEntry` ; `tst_ui_equation` PASS de `insertEquationIsUndoneAtOnce`, `editorHasItsOwnKeys`, `equationEditorBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` | S24 | #tache-l7-t26 |
| L7-T27 | Équation IV : réédition, aller-retour, `.flipchart`, scénario S24 | L7-T26, L7-T1 | `ui/src/equationcontroller.h`, `ui/src/equationcontroller.cpp`, `ui/qml/EquationEditor.qml`, `tests/tst_ui_equation.cpp`, `bench/scenarios/equation.json` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_equation` PASS de `editKeepsTheId`, `equationRoundTripIdentical`, `untouchedEquationKeepsItsBytes`, `flipchartEquationIsDrawn` ; `tst_captures_equation` Passed ; captures `equation` hors écran `LU documentController.fingerprint=${avant} OK` | S24 | #tache-l7-t27 |
| L7-T28 | Caméra (F-275, bureau) : liste des fenêtres (`camera_win.cpp`, `camera_other.cpp`), contrôleur, surcouche et palette couvertures | lot P2-A | `features/include/actcraft/features/camera.h`, `features/src/camera_win.cpp`, `features/src/camera_other.cpp`, `ui/src/cameracontroller.h`, `ui/src/cameracontroller.cpp`, `ui/qml/CameraOverlay.qml`, `ui/qml/CameraPalette.qml`, `tests/tst_ui_camera.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_camera` 0 skipped, PASS de `captureToPageAddsOneImage`, `captureToNewPageAddsAPage`, `captureToClipboard`, `cameraAbsentOnAndroid`, `emptyGrabShowsAMessage`, `captureAreaStaysInPlace`, `topLevelWindowsListsOurWindow`, `cameraPaletteBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; banc hors écran joint ; contrôle à l'écran annoncé (jeton `ecran`) | S10 | #tache-l7-t28 |
| L7-T29 | Clavier à l'écran (F-294, bureau) : injecteur (`keyinjector_win.cpp`, `keyinjector_other.cpp`), contrôleur, fenêtre sans focus | lot P2-A | `features/include/actcraft/features/screenkeyboard.h`, `features/src/keyinjector_win.cpp`, `features/src/keyinjector_other.cpp`, `ui/src/keyboardcontroller.h`, `ui/src/keyboardcontroller.cpp`, `ui/qml/ScreenKeyboard.qml`, `tests/tst_ui_keyboard.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_keyboard` 0 skipped, PASS de `keyboardTypesIntoTheFocusedField`, `keyboardWindowNeverTakesFocus`, `keyboardBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; banc hors écran joint | S10 | #tache-l7-t29 |
| L7-T30 | Annotation de bureau (F-070, F-071, bureau) | lot P2-A | `ui/src/desktopcontroller.h`, `ui/src/desktopcontroller.cpp`, `ui/qml/DesktopAnnotation.qml`, `tests/tst_ui_desktop.cpp`, `app/main.cpp` (repli de la transparence seulement) | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_desktop` PASS de `annotationWindowIsTransparentAndAbove`, `strokeGoesToTheDesktopPaperboard`, `passThroughMakesTheWindowTransparentForInput`, `desktopCaptureBecomesTheBackground`, `liveDesktopBackgroundIsNotSaved`, `nextPageIsANewTransparentPage`, `desktopPaperboardKeptBetweenActivations`, `overlayBackgroundOnlyOnItsPage` ; banc hors écran joint ; contrôle à l'écran annoncé | S10 | #tache-l7-t30 |
| L7-T31 | Outils de bureau (F-080, bureau) : roue et anneaux | L7-T30 | `ui/src/desktopcontroller.h`, `ui/src/desktopcontroller.cpp`, `ui/qml/DesktopAnnotation.qml`, `ui/qml/DesktopToolsWheel.qml`, `tests/tst_ui_desktop.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_desktop` 0 skipped, PASS de `wheelHasNoVoteNorVendorEntry`, `restoreClosesTheWheel`, `wheelClosesWhenTheWindowIsRestored` | S10 | #tache-l7-t31 |
| L7-T32 | Contrôle du lot : gestionnaires, menus Android, couvertures, scénario `presentation`, épinglages, talons comptés | L7-T19, L7-T23, L7-T27, L7-T28, L7-T29, L7-T31 | `tests/tst_ui_presentation.cpp`, `bench/scenarios/presentation.json`, `docs/verifications/AAAA-MM-JJ-lot7-epinglages.md` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_presentation` PASS de `everyLot7CommandHasAHandler`, `androidMenusHaveNoDesktopCommand`, `everyInputPanelIsACover` ; `tst_captures_presentation` Passed ; captures `presentation` hors écran `CAPTURES OK` ; `git grep "Talon L7-S8 : rempli par"` vide | S10 | #tache-l7-t32 |
| L7-T33 | Fusion avec le lot 3 | L7-T32 et **livraison du lot 3 dans `feat/livraison-complete`** | fichiers en conflit (aucun attendu hors `graphify-out/`) ; `graphify-out/` | partagé | concepteur-critique | aucun marqueur restant ; debug et release `TESTS OK`, **N ≥ N_A + 45** ; captures hors écran `-Scenario tous` `CAPTURES OK` ; `NOMS OK` ; `graphify update .` commité | S10, S22, S23, S24 (rejoués) | #tache-l7-t33 |
| L7-T34 | Après fusion du lot 3 : pivot sur l'origine, conversions de la sélection, double-clic sur une équation, « rétablir la page », guides du marqueur et de la gomme, clics qui traversent | L7-T33 | `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/MathToolsLayer.qml`, `ui/src/recognitioncontroller.h`, `ui/src/recognitioncontroller.cpp`, `ui/src/equationcontroller.h`, `ui/src/equationcontroller.cpp`, `ui/src/clockcontroller.h`, `ui/src/clockcontroller.cpp`, `ui/src/desktopcontroller.h`, `ui/src/desktopcontroller.cpp`, `ui/qml/Presentation.qml`, `tests/tst_ui_mathtools.cpp`, `tests/tst_ui_recognition.cpp`, `tests/tst_ui_equation.cpp`, `tests/tst_ui_clock.cpp`, `tests/tst_ui_desktop.cpp`, `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp`, `tests/tst_bench_scenarios.cpp`, `tests/tst_bench_result.cpp` | isolé | concepteur-critique | `TESTS OK`, N ≥ celui de L7-T33 ; PASS de `rotationPivotsOnTheOrigin`, `markerFollowsTheRuler`, `eraserFollowsTheCompass` (`tst_ui_mathtools`), `convertSelectionToShape`, `convertSelectionToText` (sauté sans reconnaisseur) (`tst_ui_recognition`), `doubleClickEditsTheEquation` (`tst_ui_equation`), `countdownCanRevertThePage` (`tst_ui_clock`), `selectToolPassesClicksThrough` (`tst_ui_desktop`), `recognitionScenarioListed` (`tst_bench_scenarios`) ; banc hors écran : scénario `reconnaissance` présent, `cache.refreshes` relevé | S10, S24 | #tache-l7-t34 |

13 tâches dans le lot P2-B ; **33 tâches** en tout (L7-T0, L7-T20 et L7-T22 faites). « lot P2-A » en dépendance =
toutes les tâches du lot P2-A mergées (les lots s'exécutent l'un après l'autre). Deux noms de 7b changent pour suivre
le suffixe imposé (`…MakesOneStep`) : `recognizedStrokeBecomesAShapeInOneStep` → `recognizedStrokeMakesOneStep`,
`handwritingBecomesTextInOneStep` → `handwritingMakesOneStep`.

## Vagues (deux tâches à la fois au plus ; le code s'écrit en parallèle, les constructions passent une à une)

Profil `.claude/chantier.md`, « Parallélisme » : deux tâches de front au plus ; toute construction, suite, banc ou
capture passe par le jeton `tests` (une place pour B sauf accord de A par la session Pilote ; un agent qui attend la
notification ne sonde pas).

**Lot P2-A.** V1 : L7-S8 (seule) · V2 : L7-T2 ‖ L7-T1 · V3 : L7-T3 ‖ L7-T9 · V4 : L7-T4 ‖ L7-T10 ·
V5 : L7-T5 ‖ L7-T11 · V6 : L7-T6 ‖ L7-T12 · V7 : L7-T7 ‖ L7-T13 · V8 : L7-T8 ‖ L7-T14 · V9 : L7-T15 ‖ L7-T16 ·
V10 : L7-T17 ‖ L7-T18 · V11 : L7-T19.

**Lot P2-B.** V12 : L7-T21 ‖ L7-T24 · V13 : L7-T23 ‖ L7-T25 · V14 : L7-T26 ‖ L7-T28 · V15 : L7-T27 ‖ L7-T29 ·
V16 : L7-T30 · V17 : L7-T31 · V18 : L7-T32 · V19 : L7-T33 (attend le lot 3) · V20 : L7-T34.

Recoupements vérifiés sur la colonne « fichiers touchés » : dans chaque vague, les deux tâches n'ont aucun fichier
commun. Chaînes tenues par « dépend de » : `mathtoolscontroller.*`, `MathToolsLayer.qml`, `tst_ui_mathtools.cpp`
(T9 → T10 → T11 → T12 → T13 → T34) ; fichiers de `formats/` (T1 → T6) ; `tst_ui_transitions.cpp` (T7 → T8) ;
`tickercontroller.*`, `TickerOverlay.qml`, `tst_ui_ticker.cpp` (T18 → T19) ; `recognitioncontroller.*`,
`RecognitionAlternatives.qml`, `tst_ui_recognition.cpp` (T21 → T23 → T34) ; `equationcontroller.*`,
`EquationEditor.qml`, `tst_ui_equation.cpp` (T26 → T27 → T34) ; `desktopcontroller.*`, `DesktopAnnotation.qml`,
`tst_ui_desktop.cpp` (T30 → T31 → T34) ; `tst_ui_presentation.cpp` (S8 → T2 → T32) ; `clockcontroller.*`,
`tst_ui_clock.cpp` (T16 → T34) ; fichiers créés en talon par L7-S8 → leur tâche. Dans une vague, la tâche
`concepteur-critique` part la première (sa revue adversariale chevauche la vague suivante). Une tâche peut avancer
d'une vague si ses dépendances sont mergées et qu'elle ne recoupe pas sa voisine.

**Écran.** Seules L7-T28 et L7-T30 ouvrent une fenêtre visible (jeton `ecran`, annoncé à Jérôme d'une ligne avant le
lancement, 7b « Écran ») ; tout le reste se joue hors écran.

## Couverture des scénarios du cadrage

| Scénario | Tâches |
|---|---|
| S10 — présentation | L7-S8 (support), L7-T2 à L7-T6, L7-T9 à L7-T17, L7-T21, L7-T23, L7-T28 à L7-T34 |
| S22 — effets de page | L7-S8 (support), L7-T7, L7-T8, L7-T33 (rejoué) |
| S23 — message défilant | L7-S8 (support), L7-T18, L7-T19, L7-T33 (rejoué) |
| S24 — équation | L7-S8 (support), L7-T1, L7-T24 à L7-T27, L7-T34 |
| S30 — tâche d'ouverture | L7-T0 (faite, f0772b6) |

## Règles communes à toutes les tâches (à joindre à chaque section)

Celles de 7a (« Règles communes », 7a:263-327) et celles du socle (socle:204-266 : processus enfant, `git -C`, jamais
de `cd`, `format.ps1` avant la preuve, pièges de Clang, pas de `std::visit` direct sur `PageObject`, aucune API
privée de Qt, propriété intellectuelle, `samples/` en lecture seule, arrêt par PID, fil de Qt Quick, mutation remise
par `git checkout --`, pas d'apostrophe dans un `R"(...)"` d'un test moc) s'appliquent pour tout ce qui suit et
qu'elles ne contredisent pas. S'y ajoutent ou s'y substituent :

- `<wt>` = `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b--<id>` ; `<id>` = `L7-S8` ou `L7-T<n>`.
  Toute commande PowerShell commence par `Set-Location '<wt>';` ou passe par `git -C '<wt>'`. Toujours
  `pwsh -NoProfile`.
- **Fichiers** : ceux de la ligne de la table, et aucun autre (décisions 6 et 7). Un besoin hors de la liste :
  s'arrêter et le rapporter à l'orchestrateur.
- **Section** = section de la tâche dans 7a ou 7b **corrigée par sa section ici**. Les étapes « test rouge de la
  logique » et « logique » des tâches reprises par la phase 1 sont faites : ne pas les rejouer, ne pas réécrire
  `features/` sauf ajout nommé ici.
- **Talon rempli** : la tâche réécrit chaque fichier talon de sa ligne ; marqueur `Talon L7-S8 : rempli par <id>.`
  et `reserved()` disparaissent. Contrôle : `git -C '<wt>' grep -n "Talon L7-S8 : rempli par <id>\." -- . ':(exclude)docs/'` vide.
- **Racine** : la tâche remplit sa racine (décisions 3 et 4) : contrôleur instancié dans la racine, `controller:`
  lié à lui, gestionnaires de commandes dans `Component.onCompleted` de la racine ; elle ne touche ni
  `Presentation.qml` ni la racine d'une autre fonction.
- **Couverture** (décision 5) : `<fonction>BlocksTheStylus` à données (`stylet`, `doigt`, `souris`), aides de
  `tests/support/presentationharness.h` ; aucun trait versé **et** effet du gestionnaire lu.
- **Noms imposés** : `shapefinder`, `spotlightFrame`, `OriginMarker.qml`, `PageEffectsPicker.qml` (et non
  `EffectPicker.qml` de 7a, L7-T8), tests d'annulation suffixés `…MakesOneStep` ou `…IsUndoneAtOnce`. Un nom refusé par
  le contrôle se renomme et le renommage est rapporté (amendement de ce plan).
- **Premier passage dans un worktree neuf** (construction complète) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire livraison-complete-b/<id> -Commande 'pwsh -NoProfile -File ./scripts/build.ps1 *> .tmp/build.txt'; Select-String -Path .tmp/build.txt -Pattern 'error|FAILED' | Select-Object -Last 4"
  ```
- **Commande ciblée** (rouge puis vert, `<motif>` donné par la section ; sortie de Qt Test lue par `-V`) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire livraison-complete-b/<id> -Commande '. ./scripts/dev-env.ps1; cmake --build --preset debug *> .tmp/build.txt; ctest --preset debug -R ''<motif>'' -V *> .tmp/ctest.txt'; Select-String -Path .tmp/build.txt,.tmp/ctest.txt -Pattern 'error C|error:|FAIL!  :|SKIP   :|Totals:|tests passed|tests failed|No tests were found' | Select-Object -Last 12"
  ```
  Rouge attendu : `error C…` ou `error:` sur le nom testé, ou `FAIL!  :`. Vert attendu : une ligne `Totals: <k>
  passed, 0 failed, 0 skipped` par test du motif (sauf SKIP nommé par la table), `100% tests passed`. Une fonction se
  lit par `Select-String -Path .tmp/ctest.txt -Pattern 'PASS   : .*<fonction>\('`. Jamais de `|` sur la sortie du
  jeton : on lit les fichiers après.
- **Preuve de tâche** (forme bornée du profil) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire livraison-complete-b/<id> -Commande 'pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1'; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2; Select-String -Path .tmp/test.txt -Pattern '<nom_du_test> .*Passed' | Select-Object -Last 4"
  ```
  Attendu : `100% tests passed, 0 tests failed out of N` avec **N = N_F + 29** exactement (N_F lu dans l'état),
  `TESTS OK`, la ligne `Passed` de chaque test de la tâche. Un test a tourné : N ≥ 1, nom présent, et la commande
  ciblée verte a montré les fonctions nommées en `PASS` (un SKIP ne prouve rien).
- **Banc sans écran** et **preuve visuelle hors écran** : formes de 7a (7a:303-322), `-Proprietaire
  livraison-complete-b/<id>`, motif de lecture du socle (lignes `DOIGT`, `PINCEMENT` comprises, socle:247). Lectures
  `-Lire "Id=attendu"` d'abord, puis une phrase par capture nommée.
- **Contrôle des noms**, après `git -C '<wt>' add <fichiers>` puis après le commit :
  `python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" '<wt>'` → dernière ligne
  `NOMS OK` (l'outil ne lit que les fichiers suivis ; il dure plus de deux minutes : le lancer en arrière-plan).
- **Commit** : `git -C '<wt>' add` de chaque fichier de la ligne, nommé (jamais `-A`, `--all`, `.`), relire
  `git -C '<wt>' diff --cached --stat`, puis `git -C '<wt>' commit -m "<message de la section>" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"`.
- **Rapport** (avant la limite de tours) : sha, sorties bornées (ciblée verte, preuve, `NOMS OK` ×2), écarts.

---

## Tâches

<a id="tache-l7-s8"></a>
### Tâche L7-S8 — Squelette du lot 7 (ancienne L0-S8)

**Objectif.** Créer et enregistrer d'un coup tous les fichiers neufs d'interface du lot 7 que la phase 1 n'a pas
posés : `Presentation.qml` complété de ses 17 racines, 27 QML talons, `ui/presentation.cmake` (noms figés
`ACTCRAFT_UI_LOT7_QML`, `ACTCRAFT_UI_LOT7_SOURCES`), `DocumentInsertions` (en-tête figé, corps talon), aides de test
au stylet, au doigt et à la souris, 29 tests et 9 scénarios talons. Après elle, aucune tâche ne touche un fichier
CMake, et `Presentation.qml` n'est plus touché qu'en L7-T17 (au besoin) et L7-T34. Aucun comportement visible ne
change. `concepteur-critique` : l'invariant partagé est le chargement du module QML (un doublon de `Presentation.qml`
ou une racine absente casse **toute** l'interface, pas seulement le lot 7).

**Fichiers.** Voir la table. Créer tous les fichiers de la ligne sauf `ui/qml/Presentation.qml` (existe, socle L0-S9)
et `tests/presentation.cmake` (existe, phase 1) : ces deux-là sont complétés.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_|tst_core_equation|tst_render_equation|tst_features_equationeditor|tst_captures_'`
(toutes les suites `tst_ui_*` du dépôt, celles du socle comprises : aucune ne doit régresser) ; preuve de tâche
(N = N_F + 29) ; preuve visuelle hors écran `premiers-pas` avec `-Lire "pageIndicator.text=1 / 2"` (la fenêtre n'a pas
changé) ; « Fonctions d'un test » (socle:231-235) sur `tst_ui_curtain` → une ligne
`SKIP   : TstUiCurtain::reserved() Talon L7-S8 : rempli par L7-T3.`

**Preuve visuelle.** `premiers-pas` hors écran : `LU pageIndicator.text=1 / 2 OK`, `CAPTURES OK` ; capture
`interface` décrite en une phrase (menus, palette et page comme avant ; aucune surcouche visible).

**Interdits.** Aucun comportement dans un talon (racines : propriétés de la décision 4 seulement) ; aucun fichier de la
décision 7 ; ne pas relister `qml/Presentation.qml` dans `ACTCRAFT_UI_LOT7_QML` (doublon de module, socle:1474-1476) ;
ni `qt_target_qml_sources` ni `qt_add_qml_module` dans `ui/presentation.cmake` (socle:1471-1473) ; aucune des 16
lignes de la phase 1 de `tests/presentation.cmake` modifiée ; aucun nom de classe dans un talon C++ ; libellés à nous.

**Terminé quand.** Tous les fichiers existent, compilent et sont enregistrés ; N = N_F + 29 ; chaque talon porte le
marqueur de sa tâche ; `lot7RootsExist` et `presentationLoadsWithoutQmlWarning` en `PASS` ; `premiers-pas`
`CAPTURES OK` ; `TESTS OK` ; `NOMS OK` deux fois.

**Étapes.**

- [ ] **Étape 1 : base.** Vérifier que la base contient le socle : `git -C '<wt>' grep -n "ACTCRAFT_UI_LOT7_QML" -- ui/CMakeLists.txt`
  (au moins 2 lignes), `git -C '<wt>' grep -n "property Item layer" -- ui/qml/Presentation.qml` (1 ligne),
  `git -C '<wt>' grep -n "addChangeObserver" -- ui/include/actcraft/ui/documentcontroller.h` (≥ 1),
  `git -C '<wt>' grep -n "addInputCover" -- ui/include/actcraft/ui/pageview.h` (≥ 1). Une sortie vide : s'arrêter et
  le rapporter (socle non mergé). Premier passage (construction complète) ; preuve de tâche : `out of N_F` (la valeur
  de l'état), `TESTS OK`.
- [ ] **Étape 2 : test rouge.** `tests/tst_ui_presentation.cpp` (montage de `tests/tst_ui_shell.cpp` : le lire et
  reprendre son chargement de `Main.qml` hors écran, par `tests/support/presentationharness.h` de l'étape 6) :
  ```cpp
  #include "support/presentationharness.h"

  #include <QtTest>

  class TstUiPresentation : public QObject
  {
      Q_OBJECT

  private slots:
      void lot7RootsExist()
      {
          presentationharness::MainWindow main;
          QVERIFY(main.window() != nullptr);
          auto* presentation = main.window()->findChild<QQuickItem*>(QStringLiteral("presentation"));
          QVERIFY(presentation != nullptr);
          const char* roots[] = {"curtainRoot",      "spotlightRoot", "pageZoomRoot",    "pageToolsRoot",
                                 "transitionRoot",   "effectsRoot",   "mathToolsRoot",   "diceRoot",
                                 "calculatorRoot",   "clockRoot",     "tickerRoot",      "recognitionRoot",
                                 "equationRoot",     "cameraRoot",    "keyboardRoot",    "desktopRoot",
                                 "otherToolsRoot"};
          for (const char* name : roots) {
              QObject* root = main.window()->findChild<QObject*>(QString::fromLatin1(name));
              QVERIFY2(root != nullptr, name);
              QVERIFY2(root->property("commands").value<QObject*>() != nullptr, name);
          }
          auto* overlays = main.window()->findChild<QQuickItem*>(QStringLiteral("pageOverlays"));
          auto* mathTools = main.window()->findChild<QQuickItem*>(QStringLiteral("mathToolsRoot"));
          QVERIFY(overlays != nullptr && mathTools != nullptr);
          QCOMPARE(mathTools->parentItem(), overlays);
          auto* layer = main.window()->findChild<QQuickItem*>(QStringLiteral("presentationLayer"));
          auto* curtain = main.window()->findChild<QQuickItem*>(QStringLiteral("curtainRoot"));
          QVERIFY(layer != nullptr && curtain != nullptr);
          QCOMPARE(curtain->parentItem(), layer);
      }

      void presentationLoadsWithoutQmlWarning()
      {
          QTest::failOnWarning(QRegularExpression(QStringLiteral("qrc:.*\\.qml")));
          presentationharness::MainWindow main;
          QVERIFY(main.window() != nullptr);
      }

      void reserved() { QSKIP("Talon L7-S8 : rempli par L7-T2."); }
  };

  QTEST_MAIN(TstUiPresentation)
  #include "tst_ui_presentation.moc"
  ```
  Commande ciblée `-R 'tst_ui_presentation'` → rouge (`tst_ui_presentation` inconnu, ou fichier manquant).
- [ ] **Étape 3 : `DocumentInsertions`.** `ui/src/presentationdocument.h`, **tel quel** (contrat de S8a, D-S8 du socle) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>
  #include <actcraft/ui/documentcontroller.h>

  #include <QByteArray>
  #include <QRectF>
  #include <QString>

  #include <cstddef>
  #include <string>
  #include <vector>

  namespace actcraft::ui {

  // Insertions du lot 7 (F-285, F-295, F-296, F-300, F-301, F-275) : par la pile du contrôleur (execute), donc par
  // l'interface de cache figée. Hors de documentcontroller.* (socle, D-S8). Fil principal.
  class DocumentInsertions
  {
  public:
      explicit DocumentInsertions(DocumentController& controller);
      core::ObjectId insertObject(core::PageObject object);          // page affichée, une étape ; 0 si refusé
      core::ObjectId insertObjectOnNewPage(core::PageObject object); // page ajoutée en fin, affichée ; une étape
      core::ObjectId insertText(const QString& plainText, const QRectF& bounds); // couche intermédiaire, une étape
      std::string addResource(QByteArray data, std::string extension);
      // core::ReplaceObjectsCommand, un groupe ; identifiants des ajouts.
      std::vector<core::ObjectId> replaceObjects(std::vector<core::ObjectId> removed,
                                                 std::vector<core::PageObject> added);
      void setPagePresentation(core::PagePresentation value); // core::SetPagePresentationCommand, page affichée
      const core::Page* shownPage() const noexcept;
      std::size_t shownIndex() const noexcept;

  private:
      DocumentController& m_controller;
  };

  } // namespace actcraft::ui
  ```
  `ui/src/presentationdocument.cpp` : `#include "presentationdocument.h"`, `// Talon L7-S8 : rempli par L7-T2.`, puis les
  corps les plus simples : constructeur qui pose `m_controller`, `return 0;` (identifiants), `return {};` (chaîne,
  vecteur), corps vide pour `setPagePresentation` (paramètre marqué `[[maybe_unused]]`), `return nullptr;`,
  `return 0;` pour `shownIndex` (sans conversion : aucun avertissement `-Wconversion`). **[SOCLE?]** les noms
  `core::ObjectId`, `core::PageObject`, `core::PagePresentation`, `core::Page` dans `model.h` après L0-S3 ; sinon, le
  contrat suit le socle et l'écart est rapporté.
- [ ] **Étape 4 : QML talons.** Les 17 racines (décision 3), contenu de la décision 4 mot pour mot, marqueur de leur
  tâche ; `PageToolsHost.qml`, `TransitionLayer.qml`, `PageEffectsPicker.qml` avec leurs propriétés de lecture en
  plus. Les 10 non-racines :
  ```qml
  import QtQuick

  // Talon L7-S8 : rempli par L7-T<n>.
  Item {
  }
  ```
  sauf `FloatingPanel.qml` (talon de L7-T2) :
  ```qml
  import QtQuick

  // Talon L7-S8 : rempli par L7-T2.
  Item {
      id: panel
      property string title
      property Item pageArea: null
      default property alias content: body.data
      signal closed()
      Item {
          id: body
          anchors.fill: parent
      }
  }
  ```
- [ ] **Étape 5 : `Presentation.qml`.** Remplacer le contenu posé par L0-S9 en gardant ses six propriétés **mot pour
  mot** (lire le fichier mergé ; si L0-S9 a écrit autre chose que socle:1330-1346, garder le sien et le rapporter) :
  ```qml
  import QtQuick
  import ActCraft.Ui

  // Racine des fonctions du lot 7 : une racine par fonction (contrôleur, surcouche ou panneau, gestionnaires de
  // commandes). Une surcouche qui prend l'entrée se déclare couverture : pageArea.view.addInputCover(<item>).
  // Après L7-S8, ce fichier ne change plus (amendement de phase 2, décision 1).
  Item {
      id: presentation
      property Commands commands: null
      property DocumentController documentController: null
      property Tools tools: null
      property Item pageArea: null
      property Item pageOverlays: null
      property Item layer: null

      CurtainOverlay { id: curtainRoot; objectName: "curtainRoot"; parent: presentation.layer; anchors.fill: parent
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      SpotlightOverlay { id: spotlightRoot; objectName: "spotlightRoot"; parent: presentation.layer; anchors.fill: parent
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      PageZoomOverlay { id: pageZoomRoot; objectName: "pageZoomRoot"
          parent: presentation.pageArea ? presentation.pageArea.presentationSlot : null; anchors.fill: parent
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      PageToolsHost { id: pageToolsRoot; objectName: "pageToolsRoot"
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea
          curtain: curtainRoot.controller; spotlight: spotlightRoot.controller }
      TransitionLayer { id: transitionRoot; objectName: "transitionRoot"; parent: presentation.layer; anchors.fill: parent
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea; curtain: curtainRoot.controller }
      PageEffectsPicker { id: effectsRoot; objectName: "effectsRoot"; parent: presentation.layer
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea; transitions: transitionRoot.controller }
      MathToolsLayer { id: mathToolsRoot; objectName: "mathToolsRoot"; parent: presentation.pageOverlays; anchors.fill: parent
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      DicePanel { id: diceRoot; objectName: "diceRoot"; parent: presentation.layer
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      CalculatorPanel { id: calculatorRoot; objectName: "calculatorRoot"; parent: presentation.layer
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      ClockPanel { id: clockRoot; objectName: "clockRoot"; parent: presentation.layer
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      TickerOverlay { id: tickerRoot; objectName: "tickerRoot"; parent: presentation.layer; anchors.fill: parent
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      RecognitionAlternatives { id: recognitionRoot; objectName: "recognitionRoot"; parent: presentation.layer
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      EquationEditor { id: equationRoot; objectName: "equationRoot"; parent: presentation.layer
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      CameraOverlay { id: cameraRoot; objectName: "cameraRoot"; parent: presentation.layer; anchors.fill: parent
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      ScreenKeyboard { id: keyboardRoot; objectName: "keyboardRoot"
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      DesktopAnnotation { id: desktopRoot; objectName: "desktopRoot"
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      OtherToolsHost { id: otherToolsRoot; objectName: "otherToolsRoot"
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
  }
  ```
  `clang-format` ne met pas en forme le QML : garder ce texte tel quel ; si `qmlformat` fait partie de `format.ps1`,
  accepter sa sortie.
- [ ] **Étape 6 : aides de test.** `tests/support/presentationharness.h` (en-tête seul, `inline`, pour tous les
  `tst_ui_*` du lot) : `class MainWindow` (charge `Main.qml` hors écran comme `tests/tst_ui_shell.cpp` — reprendre son
  aide de chargement et `app/startup.cpp` à l'identique, lire le fichier ; `window()`, `item(objectName)`,
  `pageView()`, `pageObjectCount()` lu sur `fixedPage` comme les tests du socle), `enum class Pointer { Stylus,
  Finger, Mouse }`, `void addPointerRows()` (colonne `Pointer pointer`, lignes `stylet`, `doigt`, `souris`) et
  `void strokeAt(QQuickWindow* window, Pointer pointer, QPointF from, QPointF to, int steps = 8)` :
  ```cpp
  #pragma once

  // Aides des tests d'interface du lot 7 : fenêtre principale hors écran, trait injecté au stylet, au doigt ou à la
  // souris. Stylet : QTabletEvent envoyé à la fenêtre (le filtre de la vue le prend avant les éléments QML, socle
  // H-S1, tests/tst_pageview.cpp) ; doigt : QTest::touchEvent sur un périphérique tactile de test ; souris :
  // QTest::mousePress / mouseMove / mouseRelease. API publique de Qt seulement.
  #include <QPointF>
  #include <QPointingDevice>
  #include <QQuickItem>
  #include <QQuickWindow>
  #include <QTabletEvent>
  #include <QtTest>

  namespace presentationharness {

  enum class Pointer { Stylus, Finger, Mouse };

  inline void addPointerRows()
  {
      QTest::addColumn<Pointer>("pointer");
      QTest::newRow("stylet") << Pointer::Stylus;
      QTest::newRow("doigt") << Pointer::Finger;
      QTest::newRow("souris") << Pointer::Mouse;
  }

  inline const QPointingDevice* stylusDevice()
  {
      // Même périphérique que tests/tst_pageview.cpp : le lire et reprendre sa construction (identifiant, type
      // Stylus, pointeur Pen). Jamais d enregistrement par QWindowSystemInterface (API privée).
      static const QPointingDevice device(QStringLiteral("stylet de test"), 1001, QInputDevice::DeviceType::Stylus,
                                          QPointingDevice::PointerType::Pen, QInputDevice::Capability::Position |
                                              QInputDevice::Capability::Pressure,
                                          1, 3);
      return &device;
  }

  inline void sendTablet(QQuickWindow* window, QEvent::Type type, QPointF point)
  {
      const Qt::MouseButtons buttons = type == QEvent::TabletRelease ? Qt::NoButton : Qt::LeftButton;
      QTabletEvent event(type, stylusDevice(), point, window->mapToGlobal(point), type == QEvent::TabletRelease ? 0.0 : 0.6,
                         0.0f, 0.0f, 0.0f, 0.0, 0.0f, Qt::NoModifier, Qt::LeftButton, buttons);
      QCoreApplication::sendEvent(window, &event);
  }

  inline void strokeAt(QQuickWindow* window, Pointer pointer, QPointF from, QPointF to, int steps = 8)
  {
      const auto at = [&](int i) { return from + (to - from) * (static_cast<qreal>(i) / steps); };
      switch (pointer) {
      case Pointer::Stylus:
          sendTablet(window, QEvent::TabletPress, from);
          for (int i = 1; i <= steps; ++i) {
              sendTablet(window, QEvent::TabletMove, at(i));
          }
          sendTablet(window, QEvent::TabletRelease, to);
          break;
      case Pointer::Finger: {
          static QPointingDevice* touch = QTest::createTouchDevice();
          QTest::touchEvent(window, touch).press(0, from.toPoint(), window);
          for (int i = 1; i <= steps; ++i) {
              QTest::touchEvent(window, touch).move(0, at(i).toPoint(), window);
          }
          QTest::touchEvent(window, touch).release(0, to.toPoint(), window);
          break;
      }
      case Pointer::Mouse:
          QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, from.toPoint());
          for (int i = 1; i <= steps; ++i) {
              QTest::mouseMove(window, at(i).toPoint());
          }
          QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, to.toPoint());
          break;
      }
      QCoreApplication::processEvents();
  }

  // class MainWindow : chargement de Main.qml repris de tests/tst_ui_shell.cpp (lire le fichier, même montage) ;
  // window(), item(name), pageView(), pageObjectCount().

  } // namespace presentationharness

  Q_DECLARE_METATYPE(presentationharness::Pointer)
  ```
  Si `tests/tst_pageview.cpp` ou un test du socle (`tst_ui_inputpath`) a déjà une aide d'injection du stylet, la
  **reprendre** plutôt que ce constructeur de périphérique (qui doit, lui, être relu contre Qt 6.12 :
  **[SOCLE?]** forme exacte retenue par L0-S7 dans `tst_ui_inputpath`).
- [ ] **Étape 7 : tests talons.** Les 16 `tests/tst_ui_<fonction>.cpp` hors `tst_ui_presentation`, et
  `tst_core_equation.cpp`, `tst_render_equation.cpp`, `tst_features_equationeditor.cpp`, sur ce modèle (classe et
  `.moc` adaptés ; `QTEST_GUILESS_MAIN` pour `core` et `features`, `QTEST_MAIN` pour `render` et `ui`) :
  ```cpp
  #include <QtTest>

  class TstUiCurtain : public QObject
  {
      Q_OBJECT

  private slots:
      void reserved() { QSKIP("Talon L7-S8 : rempli par L7-T3."); }
  };

  QTEST_MAIN(TstUiCurtain)
  #include "tst_ui_curtain.moc"
  ```
  Tâches : `curtain` T3, `spotlight` T4, `pagezoom` T5, `pagetools` T6, `transitions` T7, `mathtools` T9, `dice` T14,
  `calculator` T15, `clock` T16, `othertools` T17, `ticker` T18, `recognition` T21, `equation` T26, `camera` T28,
  `keyboard` T29, `desktop` T30 ; `tst_core_equation` T24, `tst_render_equation` T25, `tst_features_equationeditor`
  T26. **[SOCLE?]** Si L0-S3 a déjà créé `tst_core_equation.cpp` ou `tst_render_equation.cpp` (talons « attendent
  L0-S3 », socle:1483), ne pas les recréer : les retirer de la ligne et de cette étape, garder leur déclaration du
  socle, et diminuer le compte d'autant (N = N_F + 29 − k, rapporté).
- [ ] **Étape 8 : scénarios talons.** Les 9 `bench/scenarios/<nom>.json`, sur le modèle exact d'un scénario talon du
  socle (`bench/scenarios/selection.json`, L0-S0 : le lire et reprendre ses clés), avec `"name": "<nom>"` et
  `"description": "Talon L7-S8 : rempli par L7-T<n> (S<k>)."` — `rideau` T3 (S10), `spot` T4 (S10), `effets` T8
  (S22), `regle` T9 (S10), `compte-a-rebours` T16 (S10), `message` T19 (S23), `reconnaissance-forme` T21 (S10),
  `equation` T27 (S24), `presentation` T32 (S10) ; une seule étape `{"waitReady": true}`.
- [ ] **Étape 9 : `ui/presentation.cmake`.**
  ```cmake
  # Lot 7 (présentation et outils) : sources QML et C++ du lot, lues par ui/CMakeLists.txt (crochet du socle L0-S0,
  # avant qt_add_qml_module). Ce fichier ne fait que remplir les deux listes. Presentation.qml est déclaré par le
  # socle : ne pas le relister ici.
  set(ACTCRAFT_UI_LOT7_QML
      qml/FloatingPanel.qml
      qml/CurtainOverlay.qml
      qml/SpotlightOverlay.qml
      qml/PageZoomOverlay.qml
      qml/PageToolsHost.qml
      qml/TransitionLayer.qml
      qml/PageEffectsPicker.qml
      qml/MathToolsLayer.qml
      qml/RulerTool.qml
      qml/SetSquareTool.qml
      qml/ProtractorTool.qml
      qml/CompassTool.qml
      qml/OriginMarker.qml
      qml/DicePanel.qml
      qml/CalculatorPanel.qml
      qml/ClockPanel.qml
      qml/CountdownDialog.qml
      qml/OtherToolsHost.qml
      qml/TickerOverlay.qml
      qml/TickerDialog.qml
      qml/RecognitionAlternatives.qml
      qml/EquationEditor.qml
      qml/CameraOverlay.qml
      qml/CameraPalette.qml
      qml/ScreenKeyboard.qml
      qml/DesktopAnnotation.qml
      qml/DesktopToolsWheel.qml)
  # Composants chargés depuis leur source, comme TopBar.qml (C4702 des appels à Commands.trigger).
  set_source_files_properties(${ACTCRAFT_UI_LOT7_QML} PROPERTIES QT_QML_SKIP_CACHEGEN TRUE)

  # Sources C++ du lot par noms exacts : chaque tâche crée son contrôleur, CMake le voit à la construction suivante
  # (CONFIGURE_DEPENDS) ; aucun .cpp talon vide (LNK4221). Seuls ces noms sont pris, jamais un fichier du socle.
  file(GLOB ACTCRAFT_UI_LOT7_SOURCES CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/src/presentationdocument.h
      ${CMAKE_CURRENT_SOURCE_DIR}/src/presentationdocument.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/curtaincontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/spotlightcontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/pagezoomcontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/pagetoolscontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/transitioncontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/mathtoolscontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/dicecontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/calculatorcontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/clockcontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/presentationcommands.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/tickercontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/recognitioncontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/equationcontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/equationpreviewitem.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/cameracontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/keyboardcontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/desktopcontroller.*)
  ```
  **[SOCLE?]** Le crochet du socle fait `set(ACTCRAFT_UI_LOT7_SOURCES "")` **avant** l'`include` (socle:316-322) :
  `file(GLOB …)` remplace la valeur, c'est voulu. Vérifier que `qt_add_qml_module` reçoit des chemins absolus sans
  erreur (sinon : `list(TRANSFORM … REPLACE "^${CMAKE_CURRENT_SOURCE_DIR}/" "")`) et que les fichiers de la liste
  `SKIP_CACHEGEN` du socle ne sont pas repris ici.
- [ ] **Étape 10 : `tests/presentation.cmake`.** À la suite des 16 lignes de la phase 1 (inchangées) :
  ```cmake
  # Phase 2 (L7-S8) : tests d'interface, de logique de l'équation et de captures du lot 7, déclarés ici seulement ;
  # aucune autre tâche n'en ajoute (compte figé : N = N_F + 29).
  foreach(_lot7_ui_test IN ITEMS presentation curtain spotlight pagezoom pagetools transitions mathtools dice
                                 calculator clock othertools ticker recognition equation camera keyboard desktop)
      actcraft_add_test(tst_ui_${_lot7_ui_test}
          SOURCES tst_ui_${_lot7_ui_test}.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
          LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
      target_include_directories(tst_ui_${_lot7_ui_test} PRIVATE ${PROJECT_SOURCE_DIR}/app ${CMAKE_CURRENT_SOURCE_DIR})
  endforeach()
  actcraft_add_test(tst_core_equation SOURCES tst_core_equation.cpp LIBS ActCraft::core)
  actcraft_add_test(tst_render_equation SOURCES tst_render_equation.cpp LIBS ActCraft::render Qt6::Gui)
  actcraft_add_test(tst_features_equationeditor SOURCES tst_features_equationeditor.cpp LIBS ActCraft::features)
  foreach(_lot7_scenario IN ITEMS rideau spot effets regle compte-a-rebours message reconnaissance-forme equation
                                  presentation)
      string(REPLACE "-" "_" _lot7_test "${_lot7_scenario}")
      add_test(NAME tst_captures_${_lot7_test}
          COMMAND $<TARGET_FILE:actcraft-captures>
              --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/${_lot7_scenario}.json
              --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-${_lot7_scenario})
      set_tests_properties(tst_captures_${_lot7_test} PROPERTIES
          TIMEOUT 300
          ENVIRONMENT "${_bench_env_offscreen}"
          PASS_REGULAR_EXPRESSION "SCENARIO OK"
          FAIL_REGULAR_EXPRESSION "ECART|SCENARIO EN ECHEC|SCENARIO INVALIDE")
  endforeach()
  ```
  `LIBS` de `tst_ui_*` : alignés sur `tst_ui_shell` dans `tests/ui.cmake` (relu à la fusion) ; `tst_render_equation` :
  aligné sur `tst_render_painter` dans `tests/render.cmake` (le lire). `_bench_env_offscreen` est posé par
  `tests/bench.cmake`, inclus avant ce fichier (même portée) : **[SOCLE?]** ordre des `include` de
  `tests/CMakeLists.txt` après la fusion.
- [ ] **Étape 11 : vert et preuves.** Commande ciblée → vert (16 `tst_ui_*` en `SKIP` nommé, `tst_ui_presentation`
  `Totals: 4 passed, 0 failed, 1 skipped`, suites du socle `tst_ui_shell`, `tst_ui_inputpath` sans régression,
  9 `tst_captures_*` Passed) ; `format.ps1` ; preuve de tâche (N = N_F + 29) ; « Fonctions d'un test » sur
  `tst_ui_curtain` ; preuve visuelle `premiers-pas` ; `git -C '<wt>' add` des fichiers de la ligne, nommés ;
  `NOMS OK`.
- [ ] **Étape 12 : commit.**
  ```powershell
  git -C '<wt>' diff --cached --stat
  git -C '<wt>' commit -m "Lot 7 : squelette de la phase 2 (racines des fonctions, QML talons, sources du lot, insertions, aides de test, tests et scénarios réservés) (L7-S8)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```
  puis `NOMS OK` relancé après le commit.
