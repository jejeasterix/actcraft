# Double relecture du lot 7 (3.8), tour 1 — diff 042df64..0ba3637

Zones : z1 socle et fonctions, z2 contrôleurs ui/src, z3 QML et tests d'interface. Prompts dans le scratchpad (`relecture-l7-<axe>-<zone>.md`). Transport : SSH (le push GitHub de la branche de relecture est refusé, « remote rejected (failed) »).

## Rapports
- qualité z1 : `C:\Users\JeromeDenis\relectures-mac\actcraft-local-20261103053643-4cec\rapport.md` — non bloquant. Non couvert : tests, en-têtes, JSON → relance z1b (42 fichiers) en cours.
- conformité z1 : `C:\Users\JeromeDenis\relectures-mac\actcraft-local-20261103053640-587f\rapport.md` — non bloquant, 2 majeurs. Non couvert : une partie de features/src, de render/equationlayout et des tests → à relancer.
- conformité z2 : en cours.

## Constats à traiter (tâche corrective)
- M1 (conf z1) : S22 et S23 sans mesure au banc (`bench/src/scenarios.cpp:572`) : scénarios effets et message défilant à ajouter, ou à inscrire explicitement au banc de clôture à l'écran.
- M2 (conf z1) : `TransitionBlockers::desktopAnnotation` et `designMode` jamais posés (`ui/src/transitioncontroller.cpp:308-309`) ; `transitionsBlockedWhileAnnotating` absent (plan 7b l. 978).
- m1 (conf z1, qual z1 n°7) : commentaire de `tests/tst_actions_registry.cpp:197-200` (« de 65 à 112 », L7-T32).
- m2 (conf z1) : écart non consigné de L7-T25, `tst_render_painter.cpp:944` → à consigner dans l'état.
- m3 (conf z1) : 13, 18, 29 lus sans effet tant que 27 n'est pas lu → à dire dans H7-16 à la porte.
- À vérifier (conf z1) : unité de la distance mémorisée du rideau (`ui/src/curtaincontroller.cpp`) au zoom différent de 1.
- qual z1 mineurs : cache des équations vidé en entier à 64 entrées (`render/src/equationlayout.cpp:613-636`) ; allocation par événement stylet dans `edges()` (`features/src/mathtools.cpp:172,215,230,253`) ; `OriginMemory` spéculative ; `fontPointSize`/`shadowOffset` du message défilant sans borne haute (`features/src/ticker.cpp:131,133`) ; test dépendant du poste `recognizesSyntheticInk` ; `static_assert` incomplet `pagetools.cpp:11-19`.

## Simplification (tâche unique après les correctifs)
- marque `actcraft-cursor` en trois exemplaires + découpage des classes dupliqué (`features/src/equationeditor.cpp:20`, `render/src/equationlayout.cpp:57`, `ui/src/equationcontroller.cpp:38`).
- recherche du bord le plus proche en double (`features/src/spotlight.cpp:70-89`, `features/src/curtain.cpp:49-66`).

## Relance qualité z1b (tests, en-têtes, JSON) — `C:\Users\JeromeDenis\relectures-mac\actcraft-local-20261103054326-163c\rapport.md` — non bloquant, tout lu (qualité z1 COMPLÈTE)
- mineurs : aller-retour du message défilant ne protège pas `shadowColor`, `backgroundColor`, `Position::Center` (`tests/tst_features_ticker.cpp:42-54`) ; attente fixe `{"wait": 1300}` dans `bench/scenarios/reconnaissance-forme.json:10` → `waitFor recognition.lastKind` ; compte à rebours copié dans `presentation.json:130-210` ; commentaires périmés `pagetools.h:13-15`, `spotlight.h:15-16` ; `disabledWhileCurtainActive` teste aussi `Jump` et bloqueurs en position (`tst_features_transitions.cpp:60-66`).
- simplification : 17 modèles d'équation recopiés dans 3 tests → `tests/support/equationtemplates.h`.

## Conformité z2 — `C:\Users\JeromeDenis\relectures-mac\actcraft-local-20261103054515-4ea7\rapport.md` — non bloquant, 1 majeur, couverture partielle → relance z2b
- M3 : passage automatique des clics absent si `tool.select` est déjà actif à l'ouverture de l'annotation (`ui/src/desktopcontroller.cpp:261-266`, `ui/qml/DesktopAnnotation.qml:257-262`, `setAnnotating(true)` ne relit pas l'outil) ; le test `selectToolPassesClicksThrough` lance `tool.pen` d'abord (`tests/tst_ui_desktop.cpp:842-843`) → `followTool(currentTool)` à l'ouverture + cas de test.
- mineur : « Convertir en texte » sur une sélection hors session : tous les traits forment un seul mot (`ui/src/recognitioncontroller.cpp:610-621`, `m_arrivedAt`) → grouper par écart horizontal, ou consigner.
- mineur (= M2) : `transitionsBlockedWhileAnnotating` absent.
- à trancher à la porte : conversions limitées au stylo, le plan dit « annotations sélectionnées » (marqueur inclus).

## Qualité z2 — `C:\Users\JeromeDenis\relectures-mac\actcraft-local-20261103054657-e23a\rapport.md` — non bloquant, 1 majeur. Non couvert : en-têtes des contrôleurs, `recognitioncontroller.cpp` l.1-150, `mathtoolscontroller.cpp` l.620-783 et 985-1000 → relance z2q
- M4 : `QThread::msleep(250)` sur le fil principal dans `grabPrimaryScreen()` (`ui/src/desktopcontroller.cpp:174-182`, capture du bureau) → `QTimer::singleShot(250, …)`, action asynchrone.
- mineurs : observateurs `watchReturn` qui s'accumulent (`presentationdocument.cpp:211-241`) ; `m_arrivedAt` jamais vidé en mode formes (`recognitioncontroller.cpp:528`) ; appui d'un autre utilisateur coupe le guidage (`mathtoolscontroller.cpp:822-832`, à vérifier) ; littéral `"actcraftRotationPivot"` recopié (`tooladapters.cpp:136`) au lieu de `kRotationPivotProperty` ; zoom continu par QTimer 16 ms (`pagezoomcontroller.cpp:14-21`) ; `fitPagesToScreen` crée une étape d'annulation (`desktopcontroller.cpp:613-624`) ; capture du bureau ferme la sélection de caméra (`desktopcontroller.cpp:636-641`, à vérifier) ; `Curtain::Start` sans `static_assert` (`curtaincontroller.cpp:90-112`) ; `settleTouch()` = `qWait(50)` (`tests/support/presentationharness.h:73-76`) ; `QObject::tr` (`presentationcommands.cpp:93,99`).
- à vérifier : `clockcontroller.cpp:86-94`, 331-338 : la minuterie s'arrête-t-elle quand le panneau est caché (fin du compte à rebours retardée) ?
- simplifications : observateur d'indice + `IndexOnlyCache` inutiles (`mathtoolscontroller.cpp:362-371`, 23-29) ; `IndexOnlyCache` dupliqué (`pagetoolscontroller.cpp:70-76`) ; anneaux calculés deux fois (`desktopcontroller.cpp:442-468`).

## Conformité z2b — `C:\Users\JeromeDenis\relectures-mac\actcraft-local-20261103054943-d088\rapport.md` — non bloquant, 2 majeurs. Non couvert : `cameracontroller.h/.cpp`, `tests/support/flipchartwriter.h`, `tests/support/presentationharness.h`
- (= M2) transitions non bloquées pendant l'annotation (`transitioncontroller.cpp:308-309`, `TransitionLayer.qml:33`).
- M5 : fermer l'annotation pendant que la roue est ouverte fait perdre le plein écran (`desktopcontroller.cpp:454-470`, `restore()` 336-344) ; consigné l. 129 de l'état « vers L7-T34 » mais non fait ; test plein écran → roue → fermer → restaurer.
- (= M3) passage des clics à l'ouverture.
- mineurs : commentaire `clockcontroller.h:77` (trois actions → quatre) ; aucun test du rideau à zoom ≠ 1 (`tst_ui_pagetools.cpp:244-276`) — le code est juste (distance en pixels de page, vérifié).

## Restes à relire (relance groupée)
- conf z1 : `features/src/{mathtools,spotlight,ticker,pagezoom,countdown,camera*,handwriting*,keyinjector*,screenkeyboard,shapefinder,strokeguide}.cpp`, en-têtes `{mathtools,spotlight,ticker,pagezoom}.h`, reste de `equationeditor.cpp` et `equationlayout.cpp`, tests `tst_features_*`, `tst_render_equation`, `tst_core_equation` après l. 258.
- qual z2 : en-têtes des contrôleurs, `recognitioncontroller.cpp` l.1-150, `mathtoolscontroller.cpp` l.620-783 et 985-1000.
- conf z2 : `cameracontroller.*`, `tests/support/flipchartwriter.h`, `tests/support/presentationharness.h`.

## Qualité z3 — `C:\Users\JeromeDenis\relectures-mac\actcraft-local-20261103055221-263b\rapport.md` — non bloquant, 2 majeurs. Non couvert : 6 QML, 2 QML en partie, la plupart des tests tst_ui_* → relances z3qa, z3qb
- M6 : coches du spot fausses après un changement de page (`SpotlightOverlay.qml:203-216, 228-231`, `syncChecks()` seulement sur commande) → appeler `syncChecks()` sur `onChanged` ; test des coches.
- M7 : panneau de l'horloge sorti de la fenêtre sans retour par les poignées de coin (`ClockPanel.qml:257-268`) → borner x/y ou l'échelle au parent.
- simplifications : `MathToolZone.qml` / `MathToolMenuButton.qml` communs aux 4 outils de maths ; `Main.qml:398-406` fouille `presentationRoot.data` → propriété exposée.
- mineurs : `snapAt` dans `containmentMask` à chaque événement (`MathToolsLayer.qml:44-48`, à mesurer) ; masque du spot retriangulé à chaque déplacement (`SpotlightOverlay.qml:45-52`, à mesurer) ; graduations redessinées pendant un glisser ; message de la caméra raccourci (`CameraOverlay.qml:61-65`) ; `setHandler` sans `onCommandsChanged` (`MathToolsLayer.qml:143-155`) ; repli dupliqué (`DesktopAnnotation.qml:170-196`) ; bornage des autres lectures (`RecognitionAlternatives.qml:113-116`) ; tests sensibles au temps (`tst_ui_ticker.cpp:308-317`, `tst_ui_pagezoom.cpp:85-88,109-112`).

## Conformité z3 — `C:\Users\JeromeDenis\relectures-mac\actcraft-local-20261103055526-318e\rapport.md` — non bloquant, 2 majeurs (= M2, M3). Non couvert : 23 QML, 8 tests
- mineurs : exclusion périmée de `recognition.convert-to-text` dans `everyLot7CommandHasAHandler` (`tst_ui_presentation.cpp:757-759`) ; « Convertir en texte » non grisée sans moteur (`RecognitionAlternatives.qml:89,110`) ; aucun test « stylo seulement » (marqueur sélectionné non converti) ; aucune assertion négative « gomme : compas seulement » ; `countdownCanRevertThePage` ne distingue pas rétablir de vider (`tst_ui_clock.cpp:128-154`) ; S22 « navigateur de pages : aucun effet » (`page.goto`) et S23 « en boucle, il recommence » sans test.
- à vérifier : trait en attente converti par la sélection mais resté dans la session de reconnaissance.

## Qualité z3qa — `C:\Users\JeromeDenis\relectures-mac\actcraft-local-20261103055833-d346\rapport.md` — non bloquant, tout lu
- majeur non mesuré : Canvas du rapporteur redessinés à chaque mouvement d'agrandissement (`ProtractorTool.qml:62-63, 87-88`) → mesurer au banc d'abord.
- mineurs : roue des outils de bureau décalée de 224 px après un rendu par la barre des tâches (`DesktopToolsWheel.qml:54-58` → `setExpanded(false)`) ; `QCOMPARE` positif après `qWait(50)` (`tst_ui_desktop.cpp:424-431`) ; attentes fixes avant des affirmations négatives (calculator, clock, curtain, dice, desktop) ; cible `calculatorDisplay` porte un DragHandler (`tst_ui_calculator.cpp:361-365` → `calculatorExpression`) ; coches du rapporteur sans groupe exclusif (`ProtractorTool.qml:204-234`) ; points en dur `tst_ui_curtain.cpp:51,86` ; chaîne sans garde `tst_ui_desktop.cpp:577-580` ; `wheelEntries` figé (`DesktopToolsWheel.qml:20`).
- simplifications : `stylusTap`, `centreOf`, `controllerOf` copiés dans 4 tests → `presentationharness.h` ; `property Tools tools` morte (`CalculatorPanel.qml:14`, `DicePanel.qml:13`, `Presentation.qml:49,52`).

## Reprises (file de 7) — toutes lues en entier, aucun bloquant
- conformite-c3a : `C:\Users\JeromeDenis\relectures-mac\actcraft-local-20261103060241-3ad6\rapport.md`
- conformite-c3b : `C:\Users\JeromeDenis\relectures-mac\actcraft-local-20261103060241-e12f\rapport.md`
- conformite-c3c : `C:\Users\JeromeDenis\relectures-mac\actcraft-local-20261103060655-7fc1\rapport.md`
- conformite-c1a : `C:\Users\JeromeDenis\relectures-mac\actcraft-local-20261103060810-8e99\rapport.md`
- qualite-q2 : `C:\Users\JeromeDenis\relectures-mac\actcraft-local-20261103060949-3370\rapport.md`
- conformite-c1b : `C:\Users\JeromeDenis\relectures-mac\actcraft-local-20261103061059-edf0\rapport.md`
- qualite-z3qb : `C:\Users\JeromeDenis\relectures-mac\actcraft-local-20261103061413-8cc6\rapport.md`
- majeurs nouveaux : M8 outils de la roue (horloge, dés, calculatrice, peut-être message défilant) ouverts dans la fenêtre réduite, invisibles (`DesktopToolsWheel.qml:121,151`) ; M9 aucun test de la caméra au rapport de pixels 2 (`cameracontroller.cpp:649,685`, `tst_ui_camera.cpp:76-85`) ; M10 dézoom au bouton droit non prouvé, pris pour un double appui (`tst_ui_pagezoom.cpp:115-122`).
