# Correctifs de la double relecture du lot 7 (fin de lot 3.8, tour 1)

Brief commun aux tâches correctives L7-T35 à L7-T38 du chantier ActCraft `livraison-complete-b`. Rapport final en français.

## Cadre

- **Worktree** : `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b--L7-corr`. Il est partagé par les tâches, l'une après l'autre, pour garder une construction chaude. Chaque tâche a sa branche `feat/livraison-complete-b--L7-T3x`, créée par l'orchestrateur.
- Toute commande vise ce worktree : `git -C <wt>`, ou `Set-Location <wt>;` en tête de chaque commande PowerShell.
- Ne touche ni `docs/sessions/livraison-complete-b-etat.md` ni `.tmp/outils-b/` du worktree du chantier.
- **Consignes communes**, à lire et à suivre : `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b\.tmp\outils-b\consignes-T.md`. Elles couvrent le verrou B, les pièges, les tests instables, le contrôle des noms et le rapport.
- **Constats** : le relevé de l'orchestrateur est `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b\.tmp\outils-b\relecture-lot7.md`. Il nomme les rapports complets des relecteurs, avec leurs chemins. Pour chaque constat de ta liste, lis son passage dans le rapport : fichier:ligne, scénario d'échec, correction proposée.
  - Vérifie chaque constat contre le code avant d'agir.
  - Un constat faux ou déjà réglé : dis-le avec la preuve, et n'y touche pas.
- **Arbitrage de comportement** : l'original, via l'ancien dépôt `C:\Users\JeromeDenis\REA - Activinspire` (`graphify query` lancé là-bas), et le plan du lot (`docs/plans/2026-10-18-livraison-complete-lot7-phase2-amendement.md`, qui prime, puis 7a et 7b). Pour localiser du code, interroge d'abord le graphe de ton worktree.

## Règles

- **TDD.** Un défaut de comportement commence par un test rouge, qui reproduit le scénario d'échec du relecteur. Puis vient le correctif. Un commit par constat, ou par petit groupe de constats sur le même fichier.
- **Un test faible se renforce, il ne se retire pas.** Aucun test retiré, aucun `QSKIP` ajouté, aucune assertion affaiblie. Exemples de tests faibles : attente fixe, assertion qui passe sans le code, coordonnées fragiles.
- Un test existant modifié doit encore prouver ce que son nom dit. Dis-le, test par test.
- **Performance.** Un constat de fluidité marqué « à mesurer » se mesure au banc hors écran (`.claude/chantier.md`, forme bornée) avant toute correction.
  - Mesure qui le confirme : corrige, puis mesure de nouveau.
  - Mesure qui ne le confirme pas : ne corrige pas, et rends le chiffre.
- **Fichiers des lots 3 et 4.** N'y touche qu'en dernier recours. Diff minimal, sans réindenter, chaque ligne listée dans le rapport.
- **Ce qui sort de ta tâche** ou demande une décision de Jérôme : ne le corrige pas, relève-le dans « hors périmètre ».

## Preuves, par le verrou B, une à la fois

- Toujours `$env:CMAKE_BUILD_PARALLEL_LEVEL = '4'`.
- Au moins 2 Go de RAM libre avant une suite complète. Sinon, attends par une tâche de fond, jamais par une boucle ni par `sleep`.

Preuves attendues :
- debug `TESTS OK`, avec N = 183 ;
- release `TESTS OK`, même N ;
- les classes touchées de `tst_ui_lot7`, avec `-o <fichier>,txt` ;
- `CAPTURES OK` (`scripts/captures.ps1 -Scenario tous -Rendu logiciel -HorsEcran`) ;
- `NOMS OK` ;
- `graphify update .` commité.

Attente sans sondage : tâche de fond, notification, puis lecture du fichier. Lance tes relecteurs éventuels au premier plan (`run_in_background: false`). Si tu approches ta limite de tours, commite chaque étape saine et rends un rapport partiel avec la liste exacte de ce qui reste.

## Rapport

15 lignes au plus :
- verdict ;
- sha de chaque commit ;
- pour chaque constat : fait (fichier:ligne et test), faux ou déjà réglé (preuve), ou hors périmètre (raison) ;
- tests modifiés, avec leur justification ;
- lignes touchées dans les fichiers des lots 3 et 4 ;
- N en debug et en release, `CAPTURES OK`, mesures du banc s'il y en a, `NOMS OK`.

## Listes par tâche

**L7-T35 — bureau, roue et transitions**
- **M2** : transitions de page bloquées pendant l'annotation du bureau.
  - Code : `TransitionLayer.qml:33`, `transitioncontroller.cpp:308-309`, `TransitionBlockers::desktopAnnotation`.
  - Test : `transitionsBlockedWhileAnnotating` (plan 7b l. 978).
  - Décide aussi du sort de `designMode` : posé, ou retiré s'il n'a pas de source dans ce lot.
- **M3** : passage des clics quand `tool.select` est déjà actif à l'ouverture de l'annotation. Code : `followTool(currentTool)` à l'ouverture. Test : le cas « select avant l'ouverture ».
- **M4** : retirer `QThread::msleep(250)` du fil principal (`desktopcontroller.cpp:174-182`). La capture devient asynchrone.
- **M5** : plein écran perdu après « roue ouverte, fermeture de l'annotation, restaurer » (`desktopcontroller.cpp:454-470`, `restore()`). Test : plein écran, roue, fermer, restaurer, puis plein écran attendu.
- **M8** : horloge, dés, calculatrice, et le message défilant si c'est le cas, lancés depuis la roue alors que la fenêtre principale est réduite : ils restent invisibles (`DesktopToolsWheel.qml:121,151`). Choisis d'après l'original. Test : l'outil lancé depuis la roue est visible.
- **Mineurs du bureau** :
  - roue décalée de 224 px (`DesktopToolsWheel.qml:54-58`) ;
  - `wheelEntries` figé (`DesktopToolsWheel.qml:20`) ;
  - `fitPagesToScreen` crée une étape d'annulation (`desktopcontroller.cpp:613-624`) : corrige, ou justifie d'après l'original ;
  - la capture du bureau ferme la sélection de la caméra (`desktopcontroller.cpp:636-641`) : vérifie, puis corrige si c'est vrai ;
  - repli dupliqué de `DesktopAnnotation.qml:170-196` ;
  - `tst_ui_desktop.cpp` : 424-431 (`QTRY_COMPARE`), 577-580 (garde), 868 (attente fixe) ;
  - `tst_ui_transitions.cpp:335-336` (délai de versement) ;
  - S22 « navigateur de pages : aucun effet » (`page.goto`) : ajouter le test.

**L7-T36 — panneaux : spot, horloge, calculatrice, dés, rideau, message défilant**
- **M6** : coches du spot après un changement de page (`SpotlightOverlay.qml:203-231`). Test : l'état coché après `nextPage()`. Ajoute le `qsTr` de `SpotlightOverlay.qml:149`.
- **M7** : panneau de l'horloge sorti de la fenêtre par les poignées de coin (`ClockPanel.qml:257-268`).
- **Horloge** :
  - commentaires `clockcontroller.h:77` et `CountdownDialog.qml:6` ;
  - `countdownCanRevertThePage` doit partir d'une page avec du contenu (`tst_ui_clock.cpp:128-154`) ;
  - vérifie que la minuterie tourne quand le panneau est caché (`clockcontroller.cpp:86-94`, 331-338). Si elle s'arrête, corrige et ajoute un test.
- **Calculatrice** :
  - glisser du calcul hors de la page, et garde `canWrite` (`CalculatorPanel.qml:120,131-139`) ;
  - cible de test `calculatorExpression` (`tst_ui_calculator.cpp:361-365`) ;
  - `canUndo` vérifié (`tst_ui_calculator.cpp:320-321`, `tst_ui_othertools.cpp:73-74`).
- **Rideau** :
  - libellé `CurtainOverlay.qml:142` ;
  - commentaire `tst_ui_curtain.cpp:32-33` ;
  - points en dur `tst_ui_curtain.cpp:51,86` ;
  - `QTRY` dans `tst_ui_curtain.cpp:53-54` ;
  - test de la distance mémorisée au zoom 2 (`tst_ui_pagetools.cpp:244-276`) ;
  - `static_assert` de `Curtain::Start` (`curtaincontroller.cpp:90-112`).
- **Message défilant** :
  - aller-retour de `shadowColor`, `backgroundColor` et `Position::Center` (`tst_features_ticker.cpp:42-54`) ;
  - bornes hautes de `fontPointSize` et `shadowOffset` (`features/src/ticker.cpp:131,133`) ;
  - `objectName` en double, à renommer `tickerShadowOffset` (`TickerDialog.qml:255`) ;
  - horloge injectée à la place des bornes temporelles (`tst_ui_ticker.cpp:308-317, 369-383, 453-460, 479-495`) ;
  - S23 « en boucle, il recommence » prouvé (`tst_ui_ticker.cpp:238-239`) ;
  - `QSettings::setPath` remis en état (`tst_ui_ticker.cpp:163-164, 196-197, 412-413`).
- **Attentes fixes** : `tst_ui_dice.cpp:249-264`, `tst_ui_spotlight.cpp:88,97,127,145,180`, `tst_ui_clock.cpp:184-186, 285-310`, `tst_ui_calculator.cpp:307,350,357,364`.

**L7-T37 — outils de maths, reconnaissance, zoom, caméra, équation**
- **M9** : test de la caméra au rapport de pixels 2 (`cameracontroller.cpp:649,685`, `tst_ui_camera.cpp:76-85,286-290`). Ajoute aussi « en pixels logiques » à `tst_ui_camera.cpp:628-676`.
- **M10** : dézoom au bouton droit vraiment prouvé (`tst_ui_pagezoom.cpp:115-122`), et attentes fixes (`tst_ui_pagezoom.cpp:85-88,109-112`).
- **Fluidité, à mesurer au banc d'abord** :
  - Canvas du rapporteur pendant l'agrandissement (`ProtractorTool.qml:62-63,87-88`) ;
  - `snapAt` dans `containmentMask` (`MathToolsLayer.qml:44-48`) ;
  - masque du spot (`SpotlightOverlay.qml:45-52`) ;
  - zoom continu par QTimer de 16 ms (`pagezoomcontroller.cpp:14-21`) ;
  - allocation de `edges()` à chaque événement du stylet (`features/src/mathtools.cpp:172,215,230,253`) ;
  - cache des équations vidé à 64 entrées (`render/src/equationlayout.cpp:613-636`).
- **Outils de maths** :
  - coches du rapporteur sans groupe exclusif (`ProtractorTool.qml:204-234`) ;
  - un test par menu d'outil de maths (`rulerMenu`, `protractorMenu`, `compassMenu`, `setSquareMenu`) ;
  - assertion négative « gomme : compas seulement » ;
  - littéral `"actcraftRotationPivot"` à remplacer par `kRotationPivotProperty` (`tooladapters.cpp:136`) ;
  - appui d'un autre utilisateur qui coupe le guidage (`mathtoolscontroller.cpp:822-832`) : vérifie si deux saisies à la fois sont possibles, puis corrige ou justifie ;
  - `setHandler` sans `onCommandsChanged` (`MathToolsLayer.qml:143-155`) ;
  - `OriginMemory` spéculative : à juger.
- **Reconnaissance** :
  - exclusion périmée de `recognition.convert-to-text` (`tst_ui_presentation.cpp:757-759`) ;
  - « Convertir en texte » grisée sans moteur (`RecognitionAlternatives.qml:89,110`) ;
  - test « stylo seulement », avec un trait de marqueur sélectionné qui n'est pas converti ;
  - « Convertir en texte » sur une sélection de plusieurs mots (`recognitioncontroller.cpp:610-621`) : corrige d'après l'original, ou justifie ;
  - `m_arrivedAt` jamais vidé en mode formes (`recognitioncontroller.cpp:528`) ;
  - trait en attente converti par la sélection mais resté dans la session : vérifie, teste, corrige ;
  - bornage des autres lectures (`RecognitionAlternatives.qml:113-116`) ;
  - `tst_ui_recognition.cpp:686-687`.
- **Divers** :
  - commentaire `tst_actions_registry.cpp:197-200` (« de 65 à 112 », L7-T32) ;
  - `static_assert` incomplet `features/src/pagetools.cpp:11-19` ;
  - commentaires périmés `pagetools.h:13-15`, `spotlight.h:15-16`, et ceux du rapport qualité-q2 ;
  - `disabledWhileCurtainActive` : sortir le cas `Jump`, et initialiseurs désignés (`tst_features_transitions.cpp:60-66`) ;
  - `QObject::tr` (`presentationcommands.cpp:93,99`) ;
  - observateurs `watchReturn` qui s'accumulent (`presentationdocument.cpp:211-241`) ;
  - pointeurs nuls qui font planter `tst_ui_lot7` (rapport qualité-z3qb, n° 5) ;
  - le reste des mineurs du rapport qualité-q2 ;
  - `recognizesSyntheticInk`, qui dépend du poste (`tst_features_handwriting.cpp:167-189`).

**L7-T38 — banc : S22 et S23**
- **M1** : scénarios de banc pour S22 (chaque effet de `transitionCatalogue()` joué par `page.next`, images mesurées) et S23 (écriture au stylet pendant le message défilant en boucle), dans `bench/src/scenarios.cpp`. Mets à jour les comptes de scénarios, en disant pourquoi.
- Remplacer l'attente fixe de `bench/scenarios/reconnaissance-forme.json:10` par un `waitFor` sur `recognition.lastKind`.
- Le compte à rebours copié dans `presentation.json:130-210` : une seule source, ou une note d'alignement dans les deux descriptions.
- Preuve en plus : le banc hors écran avec les nouveaux scénarios. Relève leurs chiffres, et `BUDGET DEPASSE` sans le corriger.
