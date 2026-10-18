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
