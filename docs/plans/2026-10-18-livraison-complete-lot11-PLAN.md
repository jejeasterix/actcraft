# Livraison complète — lot 11 « Tablette, toucher, finitions, 1.0 », sous-lot 11a — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

Sous-lot **11a** : ouverture, toucher (quatre modes), deux utilisateurs, tablette Android (mise en page étroite, cibles
tactiles, rotation, clavier logiciel, menus au doigt, cycle de vie, portail de documents, mesure sur la tablette).
Le sous-lot **11b** (aide, langue, mises à jour, raccourcis, accessibilité, licences, S19 et S32 rejoués, version 1.0.0,
paquets, release prête, porte de fin) est dans `docs/plans/2026-10-18-livraison-complete-lot11b-PLAN.md`. Une seule PR
et une seule version (1.0.0) pour les deux sous-lots, comme les lots 2 et 6.

## Table des tâches (11a)

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L11-T0 | Ouverture : vidéos, REA et ancien dépôt confrontés au lot ; écarts tranchés ; API A1 à A21 revérifiées ; N11 relevé | — | `docs/verifications/2026-10-19-lot11-principes.md` (hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot11.md`) | isolé | concepteur-critique | rapport commité ; relevé brut hors dépôt présent ; D11-1 à D11-16 confirmées ou amendées ; table A remplie ; `Total Tests: <N11>` ; contrôle des titres du corpus vide ; `NOMS OK` | S30 | #tache-l11-t0 |
| L11-T1 | Réservation : 32 tests enregistrés (talons `QSKIP`), fichiers neufs des deux sous-lots (talons marqués), commandes et disposition du lot, icônes | — | `actions/data/commands.json`, `actions/data/layout.json`, `input/CMakeLists.txt`, `tools/CMakeLists.txt`, `ui/CMakeLists.txt`, `bench/app/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/{input,tools,ui,app,bench}.cmake`, les 32 fichiers de test de la liste R1, les talons de la liste R2, les 11 scénarios de la liste R3, `ui/icons/{toucher,deux-utilisateurs,mises-a-jour,reprise}.svg` | isolé | implementeur-tdd | `TESTS OK`, N = N11 + 32, 32 talons `SKIP` nommés ; `(git -C <wt> grep -l "// Talon L11" -- '*.h' '*.cpp' ':!tests/').Count` = 22 (R2) | S28, S29, S3.1 à S3.5 (réservation) | #tache-l11-t1 |
| L11-T2 | Joueur de scénarios : étapes `parallel`, `resize`, `appState`, `longPress`, `visibleTexts`, `expectChanged` (voies publiques de Qt seulement) | L11-T1 | `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`, `bench/include/actcraft/bench/injector.h`, `bench/src/injector.cpp`, `tests/tst_bench_uiscenario.cpp` | isolé | implementeur-tdd | `TESTS OK` ; `tst_bench_uiscenario` : `parallelStepParsed`, `resizeStepParsed`, `appStateStepParsed`, `longPressStepParsed`, `visibleTextsStepParsed`, `expectChangedStepParsed`, `parallelInterleavesTwoDevices` en `PASS` ; aucun symbole privé de Qt ajouté (`git -C <wt> diff feat/livraison-complete...HEAD -- bench/ tests/ \| Select-String '^\+(?!\+\+).*(QWindowSystemInterface\|qpa/\|_p\.h\|GuiPrivate)'` vide) | S28, S29, S3.2, S3.3, S3.4 | #tache-l11-t2 |
| L11-T3 | Coquille QML du lot : instances des contrôleurs, emplacements (deuxième palette, menu du toucher, menu au doigt, bandeau de reprise, boîte des mises à jour), gestionnaires talons | L11-T1 | `ui/qml/Main.qml`, `ui/qml/TopBar.qml`, `ui/qml/ToolPalette.qml`, `ui/qml/{SecondPalette,TouchModeMenu,FingerMenu,RecoveryBanner,UpdateDialog}.qml`, `ui/include/actcraft/ui/{touchmode,dualmode,layoutmetrics,lifecycle,documentportal,updatechecker}.h`, `ui/src/{touchmode,dualmode,layoutmetrics,lifecycle,documentportal,updatechecker}.cpp`, `tests/tst_ui_shell.cpp` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_shell` : `lot11SlotsPresent`, `lot11ControllersWired` en `PASS` ; banc sans écran joint | S28, S29, S3.1 à S3.4 | #tache-l11-t3 |
| L11-T4 | `input` : quatre politiques du toucher, outil imposé au doigt, appuis d'un même événement décidés ensemble (H3-16), champ `Sample::deviceId` | L11-T0, L11-T1 | `input/include/actcraft/input/touchmodes.h`, `input/src/touchmodes.cpp`, `input/include/actcraft/input/routing.h`, `input/src/routing.cpp`, `input/include/actcraft/input/sample.h`, `tests/tst_input_touchmodes.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_input_touchmodes` : 9 fonctions en `PASS` (dont `sameEventTwoFingersGoToGesture`, `noGestureOutsideFullMode`) ; mutation rouge puis remise | S28 | #tache-l11-t4 |
| L11-T5 | `PageView` : politique remplaçable, utilisateur transmis à l'hôte d'outils, `Sample::deviceId` rempli, appuis groupés, fin des contacts, appui long signalé, ancre au redimensionnement | L11-T2, L11-T4 | `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`, `tests/tst_ui_pageinput.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_pageinput` : 9 fonctions en `PASS` (dont `decisionUserReachesTheView`, `finishContactsKeepsTheStroke`, `resizeEndsTheContact`) ; mutation rouge puis remise ; `tst_ui_shell`, `tst_ui_drawing` toujours `Passed` ; banc sans écran joint | S28, S3.2, S3.3, S3.4 | #tache-l11-t5 |
| L11-T6 | Réglage du mode du toucher (F-930) : contrôleur, commande `touch.mode`, menu, réglage du poste, changement différé hors contact ; scénario `modes-toucher` | L11-T3, L11-T5 | `ui/include/actcraft/ui/touchmode.h`, `ui/src/touchmode.cpp`, `ui/qml/TouchModeMenu.qml`, `tests/tst_ui_touchmode.cpp`, `bench/scenarios/modes-toucher.json` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_touchmode` 6 fonctions `PASS` ; `tst_captures_modes_toucher` Passed ; `LU appTouchMode.mode=full OK`, `CAPTURES OK` | S28 | #tache-l11-t6 |
| L11-T7 | Second utilisateur I : politique à deux utilisateurs (schéma d'entrée D11-2), gestes suspendus, un `UserState` par utilisateur | L11-T4 | `input/include/actcraft/input/twouserpolicy.h`, `input/src/twouserpolicy.cpp`, `tools/include/actcraft/tools/userstate.h`, `tools/src/userstate.cpp`, `ui/include/actcraft/ui/toolhost.h`, `ui/src/toolhost.cpp`, `input/include/actcraft/input/routing.h`, `tests/tst_input_twousers.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_input_twousers` 7 fonctions `PASS` (dont `noGestureInDualMode`, `eachUserKeepsItsTool`) ; test enregistré en F4 (lie `ActCraft::ui`) ; mutation rouge puis remise | S28 | #tache-l11-t7 |
| L11-T8 | Second utilisateur II : sélection propre, objet tenu inaccessible à l'autre, outils de maths par utilisateur | L11-T7 | `tools/include/actcraft/tools/grabs.h`, `tools/src/grabs.cpp`, `tools/include/actcraft/tools/selecttool.h`, `tools/src/selecttool.cpp`, fichiers des outils de maths du lot 7 relevés par L11-T0 (A11, écrits dans cette ligne par l'orchestrateur), `tests/tst_tools_twousers.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_tools_twousers` 6 fonctions `PASS` (dont `heldObjectRefusedToTheOther`, `rulerBelongsToItsUser`) ; `tst_tools_select` toujours `Passed` | S28 | #tache-l11-t8 |
| L11-T9 | Deux utilisateurs III : attribut `opensDialog`, commandes à boîte refusées au second, mode à deux utilisateurs (F-110, F-303), deuxième palette ; scénario S28 `deux-utilisateurs` | L11-T3, L11-T6, L11-T8, L11-T10 | `actions/include/actcraft/actions/commandregistry.h`, `actions/src/commandregistry.cpp`, `actions/data/commands.json`, `ui/include/actcraft/ui/commands.h`, `ui/src/commands.cpp`, `ui/include/actcraft/ui/dualmode.h`, `ui/src/dualmode.cpp`, `ui/include/actcraft/ui/toolhost.h`, `ui/src/toolhost.cpp`, `ui/qml/Main.qml`, `ui/qml/SecondPalette.qml`, `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`, `tests/tst_bench_uiscenario.cpp`, `tests/tst_ui_secondpalette.cpp`, `bench/scenarios/deux-utilisateurs.json` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_secondpalette` 8 fonctions `PASS` (dont `secondUserReceivesInDualMode`), `tst_bench_uiscenario` : `commandStepCarriesTheUser` `PASS` ; `tst_captures_deux_utilisateurs` Passed ; `LU appDualMode.active=false OK`, deux `PIXEL … OK` de couleurs différentes, zoom inchangé en mode allumé (`expect pageView.zoom = ${z0}`) puis `CHANGE pageView.zoom OK` éteint, `CAPTURES OK` | S28 | #tache-l11-t9 |
| L11-T10 | Tablette I : mise en page étroite (navigateurs en tiroir, palette repliable, ligne supérieure en débordement), cibles tactiles de 48 dp | L11-T2, L11-T3 | `ui/include/actcraft/ui/layoutmetrics.h`, `ui/src/layoutmetrics.cpp`, `ui/qml/Main.qml`, `ui/qml/Theme.qml`, `ui/qml/PaletteFrame.qml`, `ui/qml/BrowserPanel.qml`, `tests/tst_ui_narrowlayout.cpp`, `bench/scenarios/tablette-etroite.json` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_narrowlayout` 6 fonctions `PASS` (dont `everyTargetAtLeast48dp`) ; `tst_captures_tablette_etroite` Passed (déclaration ctest inchangée, 1280 × 800 au départ) ; `LU appLayout.narrow=false OK` (lecture finale, après le retour en 1280 × 800), `CAPTURES OK` ; banc sans écran joint | S3.1 | #tache-l11-t10 |
| L11-T11 | Tablette II : rotation de l'écran (ancre, contact terminé), clavier logiciel (curseur de texte gardé au-dessus) ; essai sur l'émulateur de tablette | L11-T5, L11-T10, L11-T14 | `ui/include/actcraft/ui/keyboardinset.h`, `ui/src/keyboardinset.cpp`, `ui/qml/PageArea.qml`, `tests/tst_ui_orientation.cpp`, `bench/scenarios/rotation-clavier.json`, `scripts/essai-android-tablette.ps1` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_orientation` 5 fonctions `PASS` ; `tst_captures_rotation_clavier` Passed ; `ANDROID TABLETTE OK` | S3.2 | #tache-l11-t11 |
| L11-T12 | Tablette III : menus et palette au doigt (appui long → menu de l'objet ou de la page ; libellé d'un bouton par appui long ; rangées de 48 dp) | L11-T3, L11-T5, L11-T10 | `ui/include/actcraft/ui/fingermenus.h`, `ui/src/fingermenus.cpp`, `ui/qml/FingerMenu.qml`, `ui/qml/CommandMenu.qml`, `ui/qml/ToolPalette.qml`, `tests/tst_ui_fingermenus.cpp`, `bench/scenarios/au-doigt.json` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_fingermenus` 6 fonctions `PASS` ; `tst_captures_au_doigt` Passed ; `LU fingerMenu.visible=true OK`, `CAPTURES OK` | S3.3 | #tache-l11-t12 |
| L11-T13 | Tablette IV : cycle de vie — copie de reprise à la mise en arrière-plan, reprise sans perte au retour ou au relancement ; essai sur l'émulateur | L11-T2, L11-T3, L11-T5, L11-T9, L11-T14 | `ui/include/actcraft/ui/lifecycle.h`, `ui/src/lifecycle.cpp`, `ui/qml/RecoveryBanner.qml`, `ui/qml/Main.qml`, `app/startup.h`, `app/startup.cpp`, `app/main.cpp`, `bench/captures/main.cpp`, `tests/tst_ui_lifecycle.cpp`, `bench/scenarios/cycle-de-vie.json`, `scripts/essai-android-cycle.ps1` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_lifecycle` 7 fonctions `PASS` (dont `processKilledInBackgroundLosesNothing`) ; `tst_captures_cycle_de_vie` Passed ; `ANDROID CYCLE OK` | S3.4 | #tache-l11-t13 |
| L11-T14 | Tablette V : portail de documents — permissions gardées des `content://`, documents récents, image et PDF par le portail ; essai sur l'émulateur ; démon Gradle coupé dans `paquet-android.ps1` | L11-T0, L11-T1, L11-T3 | `ui/include/actcraft/ui/documentportal.h`, `ui/src/documentportal.cpp`, `ui/src/recentdocuments.cpp`, `tests/tst_ui_documentportal.cpp`, `scripts/essai-android-portail.ps1`, `scripts/paquet-android.ps1` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_documentportal` 6 fonctions `PASS` ; `tst_ui_recents` toujours `Passed` ; `paquet-android.ps1 > .tmp/apk.txt 2>&1` rend la main moins de 60 s après `APK OK` ; `ANDROID PORTAIL OK` | S5.1 | #tache-l11-t14 |
| L11-T15 | Mesure sur la tablette : APK de banc séparé, lancement et relevé par adb, procédure exacte pour Jérôme | L11-T1, L11-T14 | `bench/app/CMakeLists.txt`, `bench/app/main.cpp`, `bench/app/android/AndroidManifest.xml`, `scripts/paquet-banc-android.ps1`, `scripts/banc-android.ps1`, `docs/banc-tablette.md`, `tests/tst_bench_resultsdir.cpp` | isolé | implementeur-tdd | `TESTS OK` ; `tst_bench_resultsdir` 4 fonctions `PASS` ; `APK BANC OK` ; sur l'émulateur `MESURE INDICATIVE` puis `BUDGET TENU` ou `BUDGET DEPASSE` | S3.5 | #tache-l11-t15 |
| L11-T16 | Point d'étape 11a : suites debug et release, scénarios de 11a, APK et essais Android (S3, S5, tablette, cycle, portail), talons restants comptés, graphe | L11-T2 à L11-T15 | `tests/tst_ui_shell.cpp`, `docs/verifications/2026-10-21-lot11a-etape.md`, `graphify-out/` | partagé | implementeur-tdd | debug et release `TESTS OK`, N = N11 + 32 ; `git -C <wt> grep -l "Talon L11" -- . ':!docs/' ':!graphify-out/' ':!tests/tst_ui_shell.cpp'` = fichiers de 11b seulement ; `APK OK`, `ANDROID OK`, `ANDROID FICHIERS OK`, `ANDROID TABLETTE OK`, `ANDROID CYCLE OK`, `ANDROID PORTAIL OK` | S28, S3, S5, S3.1 à S3.5 | #tache-l11-t16 |

## Amendement 1 (relecture tour 1)

Relecture sur `86faccc` : 2 bloquants, 16 majeurs, 18 mineurs, tous appliqués dans 11a et 11b, sous les arbitrages de
l'orchestrateur (M1, B2, mineurs 3, 15 et 18). **Cet amendement fait autorité** sur le texte d'origine ; les sections
touchées sont corrigées en place. Noms vérifiés par la relecture à la tête du chantier (`27ec7ba`).

| # | Constat | Correction | Où |
|---|---|---|---|
| B1 | `tst_app_version` existe déjà (`tests/app.cmake:4-5`) | test unitaire renommé `tst_app_versioninfo` ; l'ancien est gardé ; N = N11 + 32 | R1, L11-T1, L11-T28 |
| B2 | `workflow_dispatch` « reel » créerait l'environnement `release` sans protection | point 7 de L11-T29 supprimé ; interdit « aucun `gh workflow run release.yml -f mode=reel` » ; preuve lue dans le run d'essai (`SECRETS ABSENTS`, `CONTROLE OK`) | L11-T29 |
| M1 | API privées dans le joueur | `QPointingDevice` public et `postEvent` pour `parallel` ; `QApplicationStateChangeEvent` par `sendEvent(qApp)` pour `appState` ; repli `applyState` ; vérification A22 dans L11-T0 ; règle commune « aucune API privée nouvelle » | commandes communes, A22, L11-T0, L11-T2, L11-T13 |
| M2 | `-Lire "appLayout.portrait=true"` lu après la fin en paysage | `-Lire …=false` ; `expect` après le `resize` | L11-T11 |
| M3 | `-Lire` ne passe rien au scénario ; ctest en 1280 × 800 | scénario qui fait ses deux `resize`, un seul passage | L11-T10 |
| M4 | `pageView.scale` toujours 1 | `pageView.zoom`, `remember`, `expectChanged` (étape ajoutée à L11-T2) | L11-T2, L11-T9 |
| M5 | `Sample::deviceId` absent quand L11-T7 compile | champ ajouté par L11-T4 (V2) | L11-T4, L11-T5 |
| M6 | `userReachesTheToolHost` impossible avant deux `UserState` | L11-T5 prouve la transmission (`lastContactUser`) ; L11-T7 prouve la réception (`secondUserReceivesInDualMode`) | L11-T5, L11-T7 |
| M7 | l'utilisateur n'atteint pas les gestionnaires QML | gestionnaires `(argument, user)`, `appTools.setColor(color, user)` ; `Main.qml` et `toolhost.*` à L11-T9 | L11-T3, L11-T9 |
| M8 | réglages d'essai limités aux tests qui chargent `Main.qml` | règle étendue à tout test qui crée un objet à réglages ; contrôle `reg query`, remplacé par l'assertion `QSettings probe` de l'amendement 2 (N1) | commandes communes |
| M9 | dossier de reprise du joueur non posé | `StartupOptions::recoveryDir`, joueur, `Main.qml` dans L11-T13 | L11-T13 |
| M10 | comptes de talons impossibles | `git grep -l "// Talon L11" -- '*.h' '*.cpp' ':!tests/'` = 22 ; fin : exclusion de `docs/`, `graphify-out/`, `tst_ui_shell.cpp` ; marqueur construit | L11-T1, L11-T16, L11-T30 |
| M11 | `Totals` faux (`initTestCase`, `cleanupTestCase`) | k = fonctions nommées + 2 + fonctions existantes | commandes communes |
| M12 | `run-as` refusé à un APK release | dossier propre externe, `adb pull` | A21, L11-T15 |
| M13 | démon Gradle qui bloque après `APK OK` | `GRADLE_OPTS=-Dorg.gradle.daemon=false` dans les deux scripts, preuve < 60 s | L11-T14, L11-T15 |
| M14 | `appCommands` n'est pas un `objectName` | `commands.lockedCount` | 11b, L11-T26 |
| M15 | `appUpdates.start()` jamais appelé | `Component.onCompleted: appUpdates.start()` ; `startCalledOnceByTheShell` | 11b, L11-T22 |
| M16 | `-Scenario tous` échoue et contacte l'API réelle | `bench/scenarios/controle/pseudo-langue.json` ; `ACTCRAFT_UPDATES_URL` posée avant `tous` | R3, 11b (L11-T19, L11-T20, L11-T30) |
| m1 | forme de `parseUiScenario` | forme réelle à trois arguments ; `contacts` en fin de `UiStep` | L11-T2 |
| m2 | `injector.*` absent ; journal sans API ; espace des points | fichiers ajoutés ; `Injector::eventLog()` ; points de page | L11-T2 |
| m3 | noms faux | `tool.shape` + `shape.choose`, `appTools.selectionCount` | L11-T6 |
| m4 | `pageView.inkTool` absent | `appTools.currentTool` | L11-T12 |
| m5 | fichiers R2 et montage de L11-T3 | 12 fichiers ajoutés ; `Shell s; QVERIFY(start(s));` | L11-T3 |
| m6 | `uiscenario.h` absent de L11-T9 | ajouté | L11-T9 |
| m7 | `help.check-updates` non marqué ; requête réseau en L11-T24 | nommé dans `everyDialogCommandMarked` ; `ACTCRAFT_UPDATES_URL` dans `tst_ui_accessibility` | L11-T9, 11b (L11-T24) |
| m8 | `paletteLabel` sans `objectName` | posé par L11-T12 | L11-T12 |
| m9 | `--journal-empreinte` hors fichiers | `app/main.cpp`, `app/startup.*` à L11-T13 | L11-T13 |
| m10 | `pm clear` ne retire pas les permissions | `content delete --uri`, vérifié par L11-T0 | L11-T14 |
| m11 | AVD sans nom | `actcraft-tablette`, taille au rapport | L11-T11 |
| m12 à m18 | 11b | voir l'amendement de 11b | 11b |

## Amendement 2 (relecture tour 2)

Relecture sur `7292f49` : 4 majeurs (N1 à N4) et 6 mineurs, tous appliqués en place dans 11a et 11b. **Cet amendement
fait autorité**, puis l'amendement 1, puis le texte d'origine.

| # | Constat | Correction | Où |
|---|---|---|---|
| N1 | le contrôle `reg query HKCU\Software\ActCraft` ne voit pas les écritures des tests (aucun `setOrganizationName` sous `tests/`) et lit les vrais réglages de Jérôme | contrôle retiré ; assertion de fin d'`initTestCase` dans chaque test de la règle (`QSettings probe; QCOMPARE(probe.format(), QSettings::IniFormat); QVERIFY(probe.fileName().startsWith(m_settings.path()));`) ; une mutation par tâche concernée qui retire `setDefaultFormat` et rend ce test rouge | commandes communes (réglages d'essai) |
| N2 | `secondUserReceivesInDualMode` emploie `PageView::setRoutingPolicy` (L11-T5, même vague) et une vue, dans un test `input` sans écran | fonction déplacée dans `tst_ui_secondpalette` (L11-T9, V5), qui passe à 8 fonctions ; `tst_input_twousers` revient à 7 et lie `ActCraft::ui` (il crée un `ui::ToolHost`) | R1, L11-T7, L11-T9 |
| N3 | les tests qui chargent `Main.qml` ne se lient pas sans `app/startup.cpp`, le dossier `app` et `actcraft_uiplugin` | colonne « sources et bibliothèques » dans R1 ; formes F1 à F7 ; L11-T1 enregistre ces tests sur la forme de `tests/ui.cmake:33-36` | R1, L11-T1, 11b |
| N4 | la preuve « aucune API privée nouvelle » lisait le contexte de `git diff` et toute la branche | lignes ajoutées seulement, depuis la base de la tâche : `git -C <wt> diff feat/livraison-complete...HEAD -- bench/ tests/ \| Select-String '^\+(?!\+\+).*(QWindowSystemInterface\|qpa/\|_p\.h\|GuiPrivate)'` vide | commandes communes, L11-T2 |
| n1 | `appTools.setTool` n'existe pas | `appTools.activate(<outil>, user)` (lot 3 : `appTools.activate("pen")`) | L11-T9 |
| n2 | l'outil de l'utilisateur 2 n'est jamais choisi dans S28 ; outil de départ d'un `UserState` neuf non dit | `{"command": "tool.pen", "user": 1}` avant ses réglages ; un `UserState` neuf commence au stylo (`pen`, ou valeur tranchée par L11-T0) | L11-T7, L11-T9 |
| n3 | `routing.h` absent de la ligne de L11-T7 | ajouté ; condition « si L11-T4 ne l'a pas fait » retirée | L11-T7 |
| n4 | « Cinq étapes nouvelles » | « Six » | L11-T2 |
| n5 | `recoveryDir` vide dans les tests qui chargent `Main.qml` | `recoveryDir` temporaire dans `StartupOptions` et `QStandardPaths::setTestModeEnabled(true)` dans la règle | commandes communes |
| n6 | `Get-Process java` voit les Java des autres sessions | seuls les `java.exe` dont la ligne de commande contient le worktree sont comptés | L11-T14 |

L'identifiant `L11-C0` est réservé au correctif de données qui suit L11-T0 (même règle qu'au lot 6) : il ne touche que
`actions/data/commands.json` et `actions/data/layout.json` et se joue seul avant V2.

## But, version et périmètre

**But.** Version **1.0.0** (amendement 1, § 6 : lot 11 → 1.0.0), livrée à la fin du sous-lot 11b. À la fin de 11a, le
produit est confortable sur une tablette Android et sait le toucher et les deux utilisateurs ; la version reste celle
du lot 9 (0.10.0) jusqu'à L11-T28.

**Architecture.** Rien n'est réécrit : le lot 11 change la **politique** du routeur (DA-A1, `input/routing.h`), ajoute
des utilisateurs à l'hôte d'outils (`ToolHost::user(UserId)`, socle D-S10), et adapte la coquille QML à un écran étroit
et au doigt. Toutes les modifications de `ui/src/pageview.cpp` sont regroupées dans **une** tâche (L11-T5), pour que les
vagues suivantes ne s'y recoupent pas.

**Stack.** C++20, Qt 6.12 Quick, Qt Test et ctest, joueur `actcraft-captures`, émulateur Android (scripts
`scripts/android-commun.ps1`), banc `bench/`.

**Sources.** Profil `.claude/chantier.md` ; cadrage `docs/specs/2026-10-14-livraison-complete-cadrage.md` (§ 1, ligne du
lot 11 ; décision 14 ; S3, S5, S14 ; N1-5, N1-9, N1-11) ; amendement 1
`docs/specs/2026-10-17-livraison-complete-cadrage-amendement-1.md` (§ 2.1 F-110, F-303 ; § 2.2 F-930 toucher ; § 2.3
lot 11 ; § 4 règle `Lx-T0` ; § 6 versions ; S19, S28, S29, S30, S32 ; N2-A5, N2-A7 ; DA-A1) ; `docs/BRIEF.md` § 3
(matériel cible, budget) ; coffre 0100 et 0121 ; plans mergés (socle 3-7, lots 3 à 6, lot 10) et plans du lot 7
(`C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b\docs\plans\`).

**Couverture de l'amendement (§ 2.3, lot 11 : « deux utilisateurs, modes du toucher, pages d'aide, mises à jour,
contrôle des traductions »).** Deux utilisateurs : L11-T7, L11-T8, L11-T9 (et L11-T5 pour la vue). Modes du toucher :
L11-T4, L11-T6. Pages d'aide, mises à jour, traductions : 11b. **Décision 14 du cadrage** : mise en page étroite
(L11-T10), dialogues de fichiers Android (L11-T14), essais sur émulateur et tablette réelle (L11-T11, L11-T13 à
L11-T16), passe de fluidité finale (L11-T30 de 11b), raccourcis F-950 à F-953 (L11-T23), `CHANGELOG.md` et guide de
`README.md` (L11-T28), flux de release en essai à blanc (L11-T29).

**Paquet macOS.** Aucun : exclu par le § 6 du cadrage, maintenu par l'amendement 1 (§ 1). macOS reste construit et
testé par la CI.

## Liste de réservation (L11-T1)

**R1 — 32 tests enregistrés** (21 unitaires, 11 de captures), tous talons `QSKIP("Talon L11 : rempli par L11-T<n>.")`
jusqu'à leur tâche (le test `tst_app_version` existant, `tests/app.cmake:4-5`, qui lance `actcraft --version`, est
gardé ; le test unitaire de version du lot s'appelle donc `tst_app_versioninfo`) :

| # | test | tâche | sources et bibliothèques | | # | test | tâche | sources et bibliothèques |
|---|---|---|---|---|---|---|---|---|
| 1 | `tst_input_touchmodes` | L11-T4 | F3 | | 17 | `tst_app_notices` | L11-T25 | F6 |
| 2 | `tst_ui_pageinput` | L11-T5 | F1 | | 18 | `tst_ui_lockall` | L11-T26 | F1 |
| 3 | `tst_ui_touchmode` | L11-T6 | F1 | | 19 | `tst_ui_profilesall` | L11-T27 | F1 |
| 4 | `tst_input_twousers` | L11-T7 | F4 | | 20 | `tst_app_versioninfo` | L11-T28 | F6 |
| 5 | `tst_tools_twousers` | L11-T8 | F5 | | 21 | `tst_ui_helppages` | L11-T17, L11-T18 | F2 |
| 6 | `tst_ui_secondpalette` | L11-T9 | F1 | | 22 | `tst_captures_modes_toucher` | L11-T6 | F8 |
| 7 | `tst_ui_narrowlayout` | L11-T10 | F1 | | 23 | `tst_captures_deux_utilisateurs` | L11-T9 | F8 |
| 8 | `tst_ui_orientation` | L11-T11 | F1 | | 24 | `tst_captures_tablette_etroite` | L11-T10 | F8 |
| 9 | `tst_ui_fingermenus` | L11-T12 | F1 | | 25 | `tst_captures_rotation_clavier` | L11-T11 | F8 |
| 10 | `tst_ui_lifecycle` | L11-T13 | F1 | | 26 | `tst_captures_au_doigt` | L11-T12 | F8 |
| 11 | `tst_ui_documentportal` | L11-T14 | F2 | | 27 | `tst_captures_cycle_de_vie` | L11-T13 | F8 |
| 12 | `tst_bench_resultsdir` | L11-T15 | F7 | | 28 | `tst_captures_aide_complete` | L11-T18 | F8 |
| 13 | `tst_ui_pseudolocale` | L11-T19 | F2 | | 29 | `tst_captures_pseudo_langue` | L11-T19 (activé par L11-T20) | F8 |
| 14 | `tst_ui_updatechecker` | L11-T21 | F1 | | 30 | `tst_captures_mises_a_jour` | L11-T22 | F8 |
| 15 | `tst_ui_shortcuts` | L11-T23 | F1 | | 31 | `tst_captures_classe_protegee_complete` | L11-T26 | F8 |
| 16 | `tst_ui_accessibility` | L11-T24 | F1 | | 32 | `tst_captures_profils_complets` | L11-T27 | F8 |

**Formes d'enregistrement (amendement 2, N3).** Lues dans `tests/*.cmake` à la tête du chantier (`27ec7ba`) :
- **F1 — charge `Main.qml`** (forme de `tst_ui_shell`, `tests/ui.cmake:33-36`), dans `tests/ui.cmake` :
  ```cmake
  actcraft_add_test(<test>
      SOURCES <test>.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
      LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
  target_include_directories(<test> PRIVATE ${PROJECT_SOURCE_DIR}/app)
  ```
- **F2 — `ui` sans `Main.qml`** : `SOURCES <test>.cpp` et `LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin`
  (forme de `tst_ui_about`), dans `tests/ui.cmake`.
- **F3 — `input`** : `LIBS ActCraft::input Qt6::Gui` (forme de `tst_input_routing`), dans `tests/input.cmake`,
  `QTEST_GUILESS_MAIN`.
- **F4 — `input` avec l'hôte d'outils** : `LIBS ActCraft::input ActCraft::ui Qt6::Gui`, dans `tests/input.cmake`,
  `QTEST_MAIN` (`eachUserKeepsItsTool` crée un `ui::ToolHost`).
- **F5 — `tools`** : forme des tests `tst_tools_*` de `tests/tools.cmake`, plus `ActCraft::ui` si le test crée un
  `ToolHost`.
- **F6 — `app`** : `LIBS ActCraft::ui`, définitions `ACTCRAFT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"` (et, pour
  `tst_app_versioninfo`, `ACTCRAFT_VERSION_CODE=${ACTCRAFT_VERSION_CODE}`), dans `tests/app.cmake`.
- **F7 — `bench`** : forme de `tst_bench_uiscenario` dans `tests/bench.cmake`.
- **F8 — captures** : forme de `tst_captures_premiers_pas` (`tests/bench.cmake:107-112`), décrite ci-dessous.
Une tâche qui découvre un besoin de plus (source, bibliothèque, définition) ajoute le fichier `tests/<module>.cmake`
concerné à sa ligne, sans recoupement dans sa vague, et le dit au rapport.

Un test de captures est un `add_test(NAME tst_captures_<nom> …)` dans `tests/bench.cmake`, sur le modèle de
`tst_captures_premiers_pas` (`tests/bench.cmake:107-112`) ; son scénario talon (R3) ne contient que `waitReady` et
une étape `{"skip": "Talon L11 : rempli par L11-T<n>."}` si le joueur la connaît (A12), sinon le test est déclaré avec
`DISABLED TRUE` jusqu'à sa tâche (la tâche retire la propriété) ; la forme retenue est écrite par L11-T1 dans son
rapport. **N = N11 + 32 dès L11-T1**, N11 relevé par L11-T0. Aucune autre tâche n'enregistre de test.

**R2 — talons de sources** (une fonction déclarée = un corps minimal qui compile ; marqueur `// Talon L11 : rempli par
L11-T<n>.`) : `input/include/actcraft/input/touchmodes.h`, `input/src/touchmodes.cpp`,
`input/include/actcraft/input/twouserpolicy.h`, `input/src/twouserpolicy.cpp`, `tools/include/actcraft/tools/grabs.h`,
`tools/src/grabs.cpp`, `ui/include/actcraft/ui/{touchmode,dualmode,layoutmetrics,keyboardinset,fingermenus,lifecycle,documentportal,updatechecker}.h`,
`ui/src/{touchmode,dualmode,layoutmetrics,keyboardinset,fingermenus,lifecycle,documentportal,updatechecker}.cpp`.
Les en-têtes posent les signatures de leur section (contrat figé) ; un écart découvert à l'exécution amende la section.

**R3 — scénarios talons** : `bench/scenarios/{modes-toucher,deux-utilisateurs,tablette-etroite,rotation-clavier,au-doigt,cycle-de-vie,aide-complete,mises-a-jour,classe-protegee-complete,profils-complets}.json`
et `bench/scenarios/controle/pseudo-langue.json` (sous-dossier que `captures.ps1 -Scenario tous` ne prend pas, comme
`local/` ; joué à part avec `-Traduction`) ; la déclaration ctest de `tst_captures_pseudo_langue` vise ce chemin.

**R4 — commandes du lot** (ajoutées à `actions/data/commands.json`, libellés à nous, `lockable` et `restrictable`
renseignés, `opensDialog` ajouté par L11-T9) :

```json
{ "id": "touch.mode", "label": "Mode du toucher", "argument": "text", "icon": "toucher", "lockable": true, "restrictable": false },
{ "id": "users.dual", "label": "Mode à deux utilisateurs", "checkable": true, "icon": "deux-utilisateurs", "lockable": true, "restrictable": false },
{ "id": "help.check-updates", "label": "Rechercher des mises à jour…", "icon": "mises-a-jour", "platforms": ["windows"], "lockable": true, "restrictable": false },
{ "id": "recovery.discard", "label": "Abandonner la copie de reprise", "lockable": false, "restrictable": false }
```

Forme exacte des clés (`checkable`, `argument`, `platforms`) : celle de `actions/data/commands.json` à la tête du
chantier (A5), relue par L11-T1 ; un nom de clé différent est repris tel quel.

## Vagues (largeur 3 au plus ; 2 places de tests ; aucun fichier partagé dans une vague)

V1 : L11-T0 ‖ L11-T1 · V2 : L11-T2 ‖ L11-T3 ‖ L11-T4 · V3 : L11-T5 ‖ L11-T7 ‖ L11-T10 ·
V4 : L11-T6 ‖ L11-T8 ‖ L11-T14 · V5 : L11-T9 ‖ L11-T11 ‖ L11-T15 · V6 : L11-T12 ‖ L11-T13 · V7 : L11-T16 ·
puis 11b (son plan).

Vérification des recoupements (colonne « fichiers touchés ») :
- V1 : `docs/verifications/` / données, CMake, talons — disjoints.
- V2 : L11-T2 (`uiscenario.*`, `injector.*`, `tst_bench_uiscenario.cpp`) / L11-T3 (QML de la coquille, en-têtes et
  sources R2 de `ui/`, `tst_ui_shell.cpp`) / L11-T4 (`touchmodes.*`, `routing.*`, `sample.h`, `tst_input_touchmodes.cpp`)
  — disjoints. L11-T4 attend L11-T0 (V1 finie).
- V3 : L11-T5 (`pageview.*`, `tst_ui_pageinput.cpp`) / L11-T7 (`twouserpolicy.*`, `userstate.*`,
  `toolhost.*`, `tst_input_twousers.cpp`) / L11-T10 (`layoutmetrics.*`, `Main.qml`, `Theme.qml`, `PaletteFrame.qml`,
  `BrowserPanel.qml`, un test, un scénario) — disjoints.
- V4 : L11-T6 (`touchmode.*`, `TouchModeMenu.qml`, un test, un scénario) / L11-T8 (`grabs.*`, `selecttool.*`, fichiers
  des outils de maths, un test) / L11-T14 (`documentportal.*`, `recentdocuments.cpp`, un test, un script, `paquet-android.ps1`) — disjoints.
- V5 : L11-T9 (`commandregistry.*`, `commands.json`, `commands.*`, `dualmode.*`, `toolhost.*`, `Main.qml`, `SecondPalette.qml`,
  `uiscenario.*`, `tst_bench_uiscenario.cpp`, un test, un scénario) / L11-T11 (`keyboardinset.*`, `PageArea.qml`, un test, un scénario, un script) / L11-T15 (`bench/app/*`,
  deux scripts, `docs/banc-tablette.md`, un test) — disjoints.
- V6 : L11-T12 (`fingermenus.*`, `FingerMenu.qml`, `CommandMenu.qml`, `ToolPalette.qml`, un test, un scénario) /
  L11-T13 (`lifecycle.*`, `RecoveryBanner.qml`, `Main.qml`, `app/startup.*`, `app/main.cpp`, `bench/captures/main.cpp`,
  un test, un scénario, un script) — disjoints.
- Fichiers repris d'une vague à l'autre, tenus par « dépend de » : talons de L11-T1 → leur tâche ; `commands.json`
  (L11-T1 → L11-C0 → L11-T9) ; `uiscenario.cpp` (L11-T2 → L11-T9) ; `ToolPalette.qml` (L11-T3 → L11-T12) ; `Main.qml` (L11-T3 → L11-T10 → L11-T9 → L11-T13) ; R2 de `ui/` (L11-T3 → L11-T6, L11-T9, L11-T10,
  L11-T13, L11-T14, L11-T21) ; `toolhost.*` (L11-T7 → L11-T9) ; `paquet-android.ps1` (L11-T14, puis employé par
  L11-T11, L11-T13, L11-T15, L11-T16) ; QML des
  emplacements (L11-T3 → leur tâche) ; `pageview.*` (L11-T5 seul).

Worktrees : `.worktrees/livraison-complete--L11-T<n>`, branche `feat/livraison-complete--L11-T<n>`, issue de
`feat/livraison-complete` à jour des tâches déjà mergées. Tous isolés sauf L11-T16 (worktree du chantier).

**Places.** Le jeton `tests` a 3 places, 2 pour ce lot (décision 0121). Une vague de 3 a toujours une tâche qui attend
le jeton : l'agent attend la notification, il ne sonde pas. Chemin critique lancé d'abord : V3 → L11-T5 ; V4 → L11-T8 ;
V5 → L11-T9.

**Écran.** Aucune tâche de 11a n'ouvre de fenêtre visible : captures hors écran en rendu logiciel. L'émulateur est
lancé sans fenêtre (`-no-window`, `-gpu host`, comme `scripts/essai-android-fichiers.ps1`). Les captures et le banc à
l'écran (jeton `ecran`) sont joués par L11-T30, en fin de lot.

**L11-T0 et les autres tâches.** L11-T1 ne l'attend pas ; toutes les tâches de comportement l'attendent. Ses décisions
amendent ce plan et celui de 11b avant V2 (relecture du plan exigée). Une API supposée qui diffère du code livré amende
la section de la tâche qui l'emploie.

## Commandes de preuve communes

- `<wt>` = `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L11-T<n>` ; `<id>` = `L11-T<n>`.
- **Commande ciblée** (rouge, puis vert d'un seul test) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande '. ./scripts/dev-env.ps1; cmake --build --preset debug *> .tmp/build.txt; ctest --preset debug -R ''<motif>'' --output-on-failure *> .tmp/ctest.txt'; Select-String -Path .tmp/build.txt,.tmp/ctest.txt -Pattern 'error|tests passed|tests failed|No tests were found' | Select-Object -Last 4"
  ```
  Premier passage dans un worktree neuf : même commande avec `-Commande 'pwsh -NoProfile -File ./scripts/build.ps1 *> .tmp/build.txt'`.
  Attendu au vert : `100% tests passed, 0 tests failed out of <k>`, k ≥ 1 (jamais `No tests were found`).
- **Fonctions d'un test** (chaque test écrit dans **son propre** fichier) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; . ./scripts/dev-env.ps1; & build/debug/tests/<test>.exe -o .tmp/<test>.txt,txt; Select-String -Path .tmp/<test>.txt -Pattern '^PASS   :|^FAIL!  :|^SKIP   :|^Totals' | Select-Object -Last 40"
  ```
  Attendu pour un talon rempli : chaque fonction nommée par la section en `PASS`, aucun `FAIL!`, aucun `SKIP`,
  `Totals: <k> passed, 0 failed, 0 skipped`, avec k = fonctions nommées par la section + 2 (`initTestCase` et
  `cleanupTestCase`, que Qt Test compte toujours), plus les fonctions déjà présentes si le fichier existait
  (`tst_bench_uiscenario`, `tst_ui_shell`) : k exact, relevé avant la tâche et écrit au rapport.
- **Preuve de tâche** (forme bornée du profil, sous le jeton) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1'; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2; Select-String -Path .tmp/test.txt -Pattern '<nom_du_test> .*Passed' | Select-Object -Last 1"
  ```
  Attendu : `100% tests passed, 0 tests failed out of N` avec **N = N11 + 32** exactement (dès L11-T1), `TESTS OK`, puis
  la ligne `Passed` du test nommé. Selon la décision 0121, une tâche joue la commande ciblée pendant son travail et
  cette preuve une fois avant son commit final ; la suite complète debug et release est lancée par l'orchestrateur une
  fois par vague, après les merges.
- **Preuve visuelle** (toute tâche d'interface ; lectures d'abord) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario <nom> -Rendu logiciel -HorsEcran -Sortie .tmp/captures [-Taille <LxH>] -Lire ''<objet>.<propriété>=<attendu>'' > .tmp/captures.txt 2>&1'; Select-String -Path .tmp/captures.txt -Pattern '^(COMMANDE|TRAIT|TOUCHE|ATTENTE|PRET|FENETRE|LU|PIXEL|CAPTURE) |^SCENARIO |CAPTURES (OK|EN ECHEC)' | Select-Object -Last 16"
  ```
  Attendu : chaque `LU … OK` et `PIXEL … OK` de la section, `SCENARIO OK`, `CAPTURES OK` ; aucune ligne en `ECART` ou
  `ECHEC`. Dans un scénario, la vue s'écrit `pageView.<propriété>` (objet trouvé par son `objectName`) ; dans le QML et
  le C++, elle s'atteint par `pageArea.view`. Décrire ensuite chaque capture nommée par la section.
- **Banc sans écran** (toute tâche qui touche `render/`, `input/`, `tools/` ou `ui/` ; résultat, pas porte) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu logiciel -Scenario court -Taille 1280x800 -HorsEcran > .tmp/bench.txt 2>&1'; Select-String -Path .tmp/bench.txt -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE' | Select-Object -Last 1"
  ```
  Attendu : `BUDGET TENU` ou `BUDGET DEPASSE` (jamais `MESURE INVALIDE`) ; nom du JSON de `bench/results/` joint au
  rapport (jamais commité).
- **Réglages d'essai** (amendement 3 du lot 6) : le joueur et le banc écrivent leurs réglages dans un fichier INI propre
  au lancement (`<sortie>/reglages`, profils sous `<sortie>/profils`) ; **tout test** qui crée un `Preferences`, un
  `ProfileController`, un `ClassroomLock`, un `RecentDocuments`, un `UpdateChecker`, un `TouchModeController` ou un
  `LifecycleKeeper`, ou qui charge `Main.qml`, pose dans `initTestCase` : `QStandardPaths::setTestModeEnabled(true)`,
  `QSettings::setDefaultFormat(QSettings::IniFormat)` et `QSettings::setPath(QSettings::IniFormat,
  QSettings::UserScope, m_settings.path())` (`QTemporaryDir m_settings`), et, s'il charge `Main.qml`, un `profileRoot`
  et un `recoveryDir` temporaires dans `StartupOptions` (amendement 2, n5). Jamais le registre Windows réel, jamais les
  réglages de Jérôme, jamais le vrai dossier de reprise. **Preuve (amendement 2, N1)** : `initTestCase` se termine par
  ```cpp
  QSettings probe;
  QCOMPARE(probe.format(), QSettings::IniFormat);
  QVERIFY(probe.fileName().startsWith(m_settings.path()));
  ```
  et chaque tâche qui écrit ou remplit un tel test joue une mutation : retirer l'appel à `setDefaultFormat` de
  `initTestCase` → la commande ciblée montre `initTestCase` en `FAIL!` ; remise par `git -C <wt> checkout -- <test>` ;
  vert.
- **Émulateur** : scripts de ce plan bâtis sur `scripts/android-commun.ps1` (appels adb bornés, arrêt par les
  processus tenus, jamais par nom) ; même règle que `scripts/essai-android-fichiers.ps1` : refus de démarrer si le port
  de l'essai est occupé ; sorties sous `.tmp/`.
- **Mutations** : la mutation demandée est faite dans le fichier, prouvée rouge par la commande ciblée, puis remise par
  `git -C <wt> checkout -- <fichier>` (jamais par `Copy-Item`), et la commande ciblée rejouée verte.
- **API de Qt** (arbitrage M1 de l'orchestrateur) : aucune API privée de Qt nouvelle, ni dans le code livré ni dans
  le joueur. Le banc garde son lien existant à `Qt6::GuiPrivate` pour les touches ; le lot 11 n'en ajoute aucun usage.
  Contrôle (amendement 2, N4), sur les seules lignes **ajoutées** depuis la base de la tâche ; sortie vide attendue :
  ```powershell
  git -C <wt> diff feat/livraison-complete...HEAD -- bench/ tests/ | Select-String '^\+(?!\+\+).*(QWindowSystemInterface|qpa/|_p\.h|GuiPrivate)'
  ```
- **`NOMS OK`** (après `git add`, avant chaque commit) :
  ```powershell
  python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" "<wt>" | Select-Object -Last 1
  ```
  Attendu : `NOMS OK`.
- **Contrôle des titres du corpus** (toute tâche qui écrit un texte visible ou un document ; sortie vide attendue) :
  ```powershell
  pwsh -NoProfile -Command "Get-ChildItem 'C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay' -Filter *.md -Recurse | Where-Object Name -ne 'README.md' | ForEach-Object { (Get-Content `$_.FullName -TotalCount 1) -replace '^#\s*','' } | Where-Object { `$_.Trim().Length -ge 8 } | ForEach-Object { git -C '<wt>' grep -l -i -F -e `$_ -- . ':(exclude)graphify-out' }"
  ```
- **Commit** : `git -C <wt> add <chemins explicites>` (jamais `-A`, `--all`, `.`), `git -C <wt> diff --cached --stat`
  relu, `NOMS OK`, puis `git -C <wt> commit -m "<message en français>"` terminé par la ligne
  `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.

## Propriété intellectuelle (toutes les tâches)

Aucune ressource de Promethean (icône, image, capture, code, texte de l'aide, fichier extrait des binaires) ; aucun
titre, aucune transcription, aucune citation de vidéo (références `Vnn` seulement) ; aucun nom de paperboard réel
(références neutres) ; aucun nom de classe, de propriété ou de commande de l'original. Le relevé brut de L11-T0 reste
hors dépôt.

## Décisions appliquées (tranchées par ce plan ; L11-T0 peut les amender)

- **D11-1 — Deux sous-lots.** 31 tâches estimées (17 en 11a, 14 en 11b) : au-delà de 20, le lot est découpé
  (amendement 1, § 6). Une PR, une version, une porte de fin.
- **D11-2 — Schéma d'entrée du second utilisateur** (DA-A1 ; à remplacer par D1 de `L3-T0` s'il a été tranché).
  Mode à deux utilisateurs éteint : tout pointeur → utilisateur 1. Allumé : souris → utilisateur 1 ; stylets → le
  premier appareil de stylet vu (`Sample::deviceId`) est l'utilisateur 1, un second appareil distinct l'utilisateur 2 ;
  doigts → par moitié de la vue : la moitié de la deuxième palette (à gauche) est l'utilisateur 2, l'autre
  l'utilisateur 1. La deuxième palette est au bord gauche, la palette d'origine reste à droite.
- **D11-3 — Mode du toucher.** Quatre valeurs `ignore`, `pen`, `select`, `full` ; défaut `full` (N2-A5, décision 25) ;
  réglage du poste `workstation/touch-mode` (le toucher dépend de l'écran, pas de l'enseignant). Paume (doigt pendant un
  contact de stylet) ignorée dans tous les modes. Gestes à deux doigts en mode `full` seulement.
- **D11-4 — Doigt en mode `pen` ou `select`.** Le doigt agit avec l'outil imposé (stylo avec les réglages du stylo de
  l'utilisateur, ou sélecteur), quel que soit l'outil courant ; le stylet et la souris gardent l'outil courant.
- **D11-5 — Cibles tactiles.** 48 dp sous Android et quand le réglage du poste `workstation/touch-ui` est vrai (Windows,
  tableau interactif) ; jamais moins de 44 dp ; 8 dp entre deux cibles. Sinon, les tailles du lot 6 (`tailleIcone`).
- **D11-6 — Mise en page étroite.** Largeur logique de la fenêtre < 960 dp : navigateurs en tiroir masqué (ouverts par
  leur bouton, refermés par un appui hors du tiroir), palette repliable en un bouton ; < 600 dp : menus de la ligne
  supérieure réunis dans un menu de débordement. Les seuils sont des constantes de `LayoutMetrics`.
- **D11-7 — Appui long.** 500 ms, tolérance de 10 dp. Avec le sélecteur (outil courant ou imposé) : menu de l'objet
  sous le doigt, sinon menu de la page. Sur un bouton de palette : son libellé dans une bulle 1,5 s, sans le déclencher.
  Avec un outil d'encre, aucun menu (le doigt immobile pose un point). Manière moderne (coffre 0100, complément).
- **D11-8 — Reprise.** À la mise en arrière-plan (`Qt::ApplicationSuspended`, et `Qt::ApplicationHidden` sous
  Android), si le document a changé depuis son dernier enregistrement : contacts terminés et gardés
  (`PageView::finishContacts`), copie de reprise `<AppDataLocation>/reprise/reprise.actcraft` et
  `reprise.json` (URL d'origine, page courante, échelle, horodatage), écrites hors du fil principal. Au lancement : une
  copie plus récente que le dernier enregistrement est rouverte, le bandeau le dit, le document garde son URL d'origine
  et son état « modifié ». Toutes plateformes (sur le bureau, l'état n'arrive presque jamais ; sans effet sinon).
- **D11-9 — Portail de documents (Android).** `content://` ouvert ou enregistré par le sélecteur du système : la
  permission est gardée (`takePersistableUriPermission`) ; documents récents rouverts sans sélecteur ; une permission
  perdue retire l'entrée avec un message. Insérer une image et importer un PDF passent par le même portail.
- **D11-10 — Rotation.** Orientation libre (`android:screenOrientation="unspecified"`, déjà au manifeste) ; au
  redimensionnement, le point de page au centre de la vue reste au centre ; un contact en cours est terminé et gardé.
- **D11-11 — Clavier logiciel.** Pendant la saisie d'un texte, la vue défile pour garder le curseur 16 dp au-dessus du
  clavier (`Qt.inputMethod.keyboardRectangle`) et revient à sa position à la fermeture du clavier.
- **D11-12 — Mesure sur la tablette.** APK de banc séparé, identifiant `io.github.jejeasterix.actcraft.banc`, clé de
  débogage, jamais publié ; résultats JSON lus par `adb` ; même budget que le BRIEF § 3 ; sur l'émulateur, la mesure
  est indicative (ligne `MESURE INDICATIVE`).
- **D11-13 à D11-16** : dans le plan de 11b (mises à jour, pseudo-langue, macOS, `versionCode`).

## Table A — API supposées, toutes « à revérifier par L11-T0 »

| # | API supposée | Source | Employée par |
|---|---|---|---|
| A1 | `input::RoutingPolicy::decide(const Sample&, const RouterState&) const`, `FullTouchPolicy`, `PointerRouter::{setPolicy, route, release, reset, setCoverTest, gestureTouches}`, `Decision{route, user, endToolContacts}` | vérifié à `5373f04` (`input/include/actcraft/input/routing.h`) ; rempli par L0-S7 et le lot 3 | L11-T4, L11-T5, L11-T7 |
| A2 | `input::Sample{x, y, pressure, timestampNs, device (DeviceKind::Stylus, Mouse, Touch, Other), phase (Phase::Press, Move, Release), pointer, seq}` | `input/include/actcraft/input/sample.h` à `5373f04` (`pointer` cité par `routing.h`) ; aucun identifiant d'appareil | L11-T4, L11-T5, L11-T7 |
| A3 | `tools::UserState{id(), openMathTools(), setOpenMathTools()}`, `ui::ToolHost::user(input::UserId)` (crée l'unique état), `ToolHost::activeTool()` | socle L0-S6 (`docs/plans/2026-10-18-livraison-complete-socle-3-7-PLAN.md:950-960`), rempli par le lot 3 | L11-T5, L11-T7, L11-T8 |
| A4 | `ui::PageView` : `objectName: "pageView"`, `pageArea.view`, `contactActive`/`contactActiveChanged` (L4-T17), conversion page ↔ vue, échelle, défilement | amendement 3 du lot 6 ; `ui/qml/PageArea.qml` | L11-T5, L11-T6, L11-T11 |
| A5 | `actions::CommandRegistry` (`find`, `platforms`, `lockable`, `restrictable`, `setLocked`, `isLocked`, `isOwnEnabled`), `ui::Commands` (`trigger`, `setEnabled`, `isEnabled`, `setLocked`, `lockedCount`, `setNativeHandler`) ; clés de `commands.json` | L6-T3, L6-T31 (`docs/plans/2026-10-18-livraison-complete-lot6b-PLAN.md:708-775`) | L11-T1, L11-T9 |
| A6 | `ui::Preferences` (QSettings par défaut ; clés `workstation/*`) | L6-T5 | L11-T6, L11-T10, L11-T13 |
| A7 | `ui::ProfileController` (`currentId`, `activate`), `actions::ProfileStore`, profils `actions/data/profiles/{en-classe,preparation,lecture-ecriture,calcul-geometrie,son-image}.json` | L6-T4, L6-T5, L6-T6 | 11b (L11-T27) |
| A8 | `ui::HelpViewer` (`openFor`, `follow`, `qrc:/actcraft/help/<outil>.html`) | L6-T35 | 11b |
| A9 | `ui::Language::install(QCoreApplication&, Preferences&)`, `ui/i18n/actcraft_fr.ts`, `qt_add_translations` | L6-T36 | 11b |
| A10 | `ui::ClassroomLock`, `appClassroom`, `bench/scenarios/classe-protegee.json` | L6-T31 | 11b |
| A11 | outils de maths du lot 7 (`features/`, commandes `math.*`), rattachés à `UserState::openMathTools()` | plans du lot 7 (`livraison-complete-b`) | L11-T8 |
| A12 | joueur : étapes `waitReady`, `expect`, `capture`, `command`, `key`, `stroke`, `checkPixel`, `wait`, `waitFor` (5373f04) ; `touch`, `pinch`, `pointer` (socle, `socle-3-7-PLAN.md:401-448`) ; `restart` (L6-T18) | `bench/src/uiscenario.cpp` | L11-T2 et tous les scénarios |
| A13 | déclaration d'un test de captures | `tests/bench.cmake:107-112` | L11-T1 |
| A14 | `ui::RecentDocuments` (liste du poste) | L6-T24 | L11-T14 |
| A15 | `tools::SelectTool` (désignation, rectangle, glisser) | `tools/include/actcraft/tools/selecttool.h`, lot 3 | L11-T8 |
| A16 | `ui::NetworkSettings` (mandataire) | L6-T28 | 11b (L11-T21) |
| A17 | `PaletteFrame.qml`, `BrowserPanel.qml`, `Theme.qml` (`tailleIcone` modifiable) | L6-T7, lot 4 | L11-T10 |
| A18 | menu d'édition d'objet et menu de page (`ui/qml/PageContextMenu.qml`, menu d'objet du lot 5) | 5373f04 (`PageContextMenu.qml`) ; lot 5 | L11-T12 |
| A19 | outil texte : rectangle du curseur en coordonnées de page | lot 4 | L11-T11 |
| A20 | `app/startup.h` (`StartupOptions`, `profileRoot`, `loadMainWindow`) | amendement 3 du lot 6 | L11-T13 |
| A21 | `bench/app/main.cpp` (dossier des résultats `defaultDir`, options) ; sous Android, dossier propre externe `/sdcard/Android/data/io.github.jejeasterix.actcraft.banc/files` (lisible par `adb pull` sans `run-as`) | 5373f04 ; L6 (`defaultDir` avant `loadMainWindow`) | L11-T15 |
| A22 | un `QApplicationStateChangeEvent` envoyé par `QCoreApplication::sendEvent(qApp, &ev)` atteint les filtres d'événements posés sur `qApp`, et la plateforme (Windows, Android) envoie cet événement à `qApp` quand l'état change | `QtGui/qevent.h:1029-1038` (classe publique) ; envoi par la plateforme non vérifié | L11-T2, L11-T13 |
| A23 | étape `remember` du joueur (`{"remember": "<objet>.<propriété>", "as": "<nom>"}`) et substitution `${<nom>}` dans `expect` | `bench/src/uiscenario.cpp` (clés `remember`, `as` présentes à `5373f04`) ; forme de L0-S2 | L11-T2, L11-T9 |
| A24 | coupure du démon Gradle lancé par `androiddeployqt` (`GRADLE_OPTS=-Dorg.gradle.daemon=false`) | piège de l'état du chantier (rejeu figé 2 h 30 après `APK OK`) | L11-T14, L11-T15 |

---

<a id="tache-l11-t0"></a>
### Tâche L11-T0 — Ouverture : vidéos, REA et ancien dépôt confrontés au lot

**Objectif.** Règle `Lx-T0` de l'amendement 1 (§ 4) pour le lot 11 (décisions 0100 et 0121) : relever dans le corpus
tous les principes des fonctions du lot (deux utilisateurs, modes du toucher, aide, langue, mises à jour, tablette) et
ceux de la spécification ; trancher sur ActivInspire 3.5.18, par REA en statique, les points « à élucider » du § 2 :
contenu de la deuxième palette (F-303, N2-A7), objet tenu par un utilisateur, outil par défaut du toucher et mode
d'interaction par défaut avec les objets (F-930) ; confirmer ou amender D11-1 à D11-16 ; remplir la table A ; relever
N11.

**Fichiers.** `docs/verifications/2026-10-19-lot11-principes.md` (dépôt) ; relevé brut hors dépôt
`C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot11.md`.

**Test et commande bornée.** Aucun test. Commande de N11 (étape 5).

**Interdits.** Aucune ligne de code ; aucun titre, aucune transcription ni citation de vidéo dans le dépôt ; aucun nom
de classe ou de propriété de l'original dans le dépôt ; aucune observation de l'original à l'écran sans avoir prévenu
Jérôme (pilotage de la souris de l'écran principal, `CLAUDE.md`, mode autonome, point 3) ; aucune écriture dans
`samples/` ni dans `%APPDATA%\Promethean`.

**Terminé quand** le rapport et le relevé existent, chaque constat du lot y a une ligne (exigence, tâche, scénario, ou
raison), D11-1 à D11-16 sont confirmées ou amendées, la table A est remplie, N11 est écrit, `NOMS OK` et le contrôle
des titres du corpus est vide.

**Étapes.**
- [ ] **Étape 1 : corpus.** Lire `C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\README.md` (sections
  « Principes absents » et « Écarts avec la spec ») et les fiches des fonctions du lot ; dans le relevé brut, une ligne
  par principe (`Vnn`, fonction, principe, accord avec la spec).
- [ ] **Étape 2 : spécification.** `C:\Users\JeromeDenis\REA - Activinspire\docs\spec\fonctionnalites.md` : F-110,
  F-303 (dont `:385`), F-930 (toucher, langue, mises à jour), F-760, F-950 à F-953 ; graphe de l'ancien dépôt :
  ```powershell
  pwsh -NoProfile -Command "Set-Location 'C:\Users\JeromeDenis\REA - Activinspire'; graphify query 'dual user second palette touch mode' | Select-Object -First 60"
  ```
- [ ] **Étape 3 : REA en statique** (skill `reverse-engineer-anything` sur `C:\Program Files\Activ Software\Inspire\`) :
  ressources des boîtes de dialogue et valeurs par défaut des réglages du toucher et du mode à deux utilisateurs,
  contenu de la deuxième palette. Résultats en nos mots dans le rapport ; noms de l'original seulement dans le relevé
  brut. Si le statique ne suffit pas : écrire la question pour l'orchestrateur, qui prévient Jérôme avant toute séance
  à l'écran ; en attendant, les défauts de ce plan s'appliquent.
- [ ] **Étape 4 : API.** Dans `<wt>` : `graphify query "PointerRouter RoutingPolicy Sample device pointer"`,
  `graphify query "ToolHost UserState user openMathTools"`, `graphify query "PageView contactActive pageArea"`,
  `graphify query "CommandRegistry lockable restrictable platforms"`, `graphify query "HelpViewer Language ClassroomLock ProfileController"`,
  `graphify query "uiscenario steps touch pinch restart"` ; lire les en-têtes cités ; remplir la table A (conforme, ou
  signature réelle et section à amender). Relever la forme des clés de `actions/data/commands.json` et la déclaration
  d'un test de captures (`tests/bench.cmake`). Vérifier aussi : **A22** (un `QApplicationStateChangeEvent` envoyé à
  `qApp` atteint un filtre posé sur `qApp`, et la plateforme envoie cet événement à `qApp` sous Windows et sous Android ;
  sinon, le repli `applyState` de L11-T13 s'applique et la section est amendée) ; A23 ; A24 ; la voie de retrait d'un
  document pour `essai-android-portail.ps1` (`adb shell content delete --uri <uri>`) ; le nom de `shape.choose` et de
  son argument « rectangle » (lot 4) ; la forme de la valeur de `appTools.currentTool` (lot 3) ; le chemin réel du
  rapport des épinglages du lot 7 (11b, L11-T27 ; ou « absent : liste lue dans les profils du lot 7 ») ; écrire dans
  le relevé hors dépôt `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot11-noms.txt` les noms de l'original à
  refuser dans les textes (ceux de `lot6-noms-profils.txt` et ceux des outils), un par ligne, jamais dans le dépôt.
- [ ] **Étape 5 : N11.**
  ```powershell
  pwsh -NoProfile -Command "Set-Location 'C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L11-T0'; New-Item -ItemType Directory -Force .tmp | Out-Null; . ./scripts/dev-env.ps1; cmake --preset debug *> .tmp/cfg.txt; ctest --preset debug -N | Select-String 'Total Tests'"
  ```
  Attendu : `Total Tests: <N11>` ; écrire N11 dans le rapport (N = N11 + 32 à partir de L11-T1).
- [ ] **Étape 6 : commandes Android du lot 2.** Relever dans `actions/data/commands.json` toute commande de fichier
  encore déclarée sans `android` (repli de la décision 17, `docs/plans/2026-10-15-livraison-complete-lot2b-PLAN.md:5787`) ;
  la liste va au correctif `L11-C0` s'il y en a.
- [ ] **Étape 7 : rapport.** Écrire `docs/verifications/2026-10-19-lot11-principes.md` : en-tête (chemin du relevé brut
  cité), méthode et limites, table des constats (principe → exigence → tâche → scénario, ou raison de l'écart), D11-1 à
  D11-16 (confirmée, ou amendée et pourquoi), table A, N11, hypothèses pour Jérôme, correctif `L11-C0` proposé.
- [ ] **Étape 8 : contrôles et commit.** `git -C <wt> add docs/verifications/2026-10-19-lot11-principes.md` ; `NOMS OK` ;
  contrôle des titres du corpus vide ; commit « Lot 11 : principes confrontés, décisions et API revérifiées (L11-T0) ».

<a id="tache-l11-t1"></a>
### Tâche L11-T1 — Réservation des tests, des fichiers et des commandes

**Objectif.** Enregistrer d'un coup les 32 tests de R1 (talons sautés), créer les talons R2 et R3, ajouter les
commandes R4 et leurs entrées de disposition, et les quatre icônes à nous ; aucune logique.

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Preuve de tâche ; puis, pour trois talons pris au hasard, « fonctions d'un test ».

**Interdits.** Aucun comportement ; aucune icône dérivée d'une icône de l'original (dessin à nous, SVG simple sur la
grille de `ui/icons/`) ; aucune autre tâche n'enregistre de test.

**Terminé quand** N = N11 + 32, `TESTS OK`, chaque talon unitaire rend `SKIP   :` avec « Talon L11 : rempli par
L11-T<n>. », et `(git -C <wt> grep -l "// Talon L11" -- '*.h' '*.cpp' ':!tests/').Count` vaut 22 (les fichiers R2).

**Étapes.**
- [ ] **Étape 1 : talon de test** (modèle pour les 21 unitaires ; un fichier par test de R1, classe `Tst<NomCamel>`) :
  ```cpp
  #include <QtTest>

  class TstInputTouchModes : public QObject
  {
      Q_OBJECT
  private slots:
      void reserved() { QSKIP("Talon L11 : rempli par L11-T4."); }
  };

  QTEST_GUILESS_MAIN(TstInputTouchModes)
  #include "tst_input_touchmodes.moc"
  ```
  `QTEST_MAIN` pour les tests `tst_ui_*` (comme `tst_ui_shell`) et pour `tst_input_twousers` (F4),
  `QTEST_GUILESS_MAIN` pour les autres tests `input`, `tools`, `app` et `bench`.
- [ ] **Étape 2 : enregistrement.** Dans `tests/{input,tools,ui,app,bench}.cmake`, un `actcraft_add_test(<nom> …)` par
  test unitaire, sur la forme F1 à F7 que lui donne la colonne « sources et bibliothèques » de R1 (fonction de
  `tests/CMakeLists.txt:2-16` ; F1 = forme de `tests/ui.cmake:33-36`), et un `add_test(NAME tst_captures_<nom> …)` par test de captures
  (forme de `tests/bench.cmake:107-112`), scénario R3 correspondant.
- [ ] **Étape 3 : talons de sources.** Chaque en-tête R2 reçoit les déclarations écrites dans la section de sa tâche
  (signatures recopiées telles quelles) ; chaque `.cpp` définit chaque fonction par le corps le plus simple qui compile,
  sous le marqueur `// Talon L11 : rempli par L11-T<n>.` ; ajout aux `CMakeLists.txt` de module.
- [ ] **Étape 4 : commandes et disposition.** R4 dans `actions/data/commands.json` ; `touch.mode` et `users.dual` dans
  le menu Affichage de `actions/data/layout.json`, `help.check-updates` dans le menu Aide ; icônes
  `ui/icons/{toucher,deux-utilisateurs,mises-a-jour,reprise}.svg`.
- [ ] **Étape 5 : vert.** Preuve de tâche : `100% tests passed, 0 tests failed out of <N11 + 32>`, `TESTS OK`. Puis
  « fonctions d'un test » sur `tst_input_touchmodes`, `tst_ui_lifecycle`, `tst_app_versioninfo` : `SKIP` attendu.
- [ ] **Étape 6 : commit.** `git add` des chemins de la ligne (liste nommée) ; `NOMS OK` ; contrôle des titres du corpus
  vide ; commit « Lot 11 : réservation des tests, des fichiers et des commandes (L11-T1) ».

<a id="tache-l11-t2"></a>
### Tâche L11-T2 — Joueur de scénarios : étapes du lot

**Objectif.** Six étapes nouvelles du joueur (`actcraft-captures`), analysées et jouées :
- `{"parallel": [{"device": 1, "kind": "stylus"|"touch"|"mouse", "points": [[x, y], …]}, …], "pressure": 0.5}` :
  plusieurs contacts injectés **entrelacés** (appuis, puis mouvements alternés, puis relâchés), chacun avec son
  appareil ; points en coordonnées de **page**, comme `stroke` ; un `QPointingDevice` **public** par numéro, créé à la
  première utilisation par son constructeur (`systemId` = 6000 + n, `QPointingDeviceUniqueId::fromNumericId(n)`), ou
  `QTest::createTouchDevice()` pour un doigt ; événements postés par `QCoreApplication::postEvent`, comme `postTablet`
  (`bench/src/injector.cpp`) ; aucun enregistrement auprès de la plateforme ; ligne `PARALLELE <n> contacts OK|ECHEC`.
  Journal : `Injector::setLogging(bool)` et `QStringList Injector::eventLog() const` (« P1 », « M1 », « R1 » : phase et
  numéro d'appareil de chaque événement posté).
- `{"resize": [w, h]}` : la fenêtre prend la taille logique donnée (rotation simulée) ; ligne `TAILLE <w>x<h> OK`.
- `{"appState": "suspended"|"hidden"|"inactive"|"active"}` : état de l'application simulé par la voie publique
  `QApplicationStateChangeEvent ev(state); QCoreApplication::sendEvent(qApp, &ev);` (A22) ; repli si L11-T0 montre que
  la plateforme ne passe pas par `qApp` : appel de `appLifecycle.applyState(state)` (L11-T13) ; ligne
  `ETAT <état> OK`.
- `{"expectChanged": "<objet>.<propriété>", "from": "${<nom>}"}` : écart si la valeur lue est égale à la valeur
  retenue par `remember` (A23) ; ligne `CHANGE <objet>.<propriété> OK|ECART`.
- `{"longPress": [x, y], "kind": "touch"|"mouse", "holdMs": 600, "space": "page"|"window"}` : appui tenu sans mouvement,
  puis relâché ; ligne `APPUI LONG OK`.
- `{"visibleTexts": "pseudo", "marks": ["⟦", "⟧"], "ignore": ["pageIndicator"]}` : parcourt les éléments visibles de
  toutes les fenêtres (popups et menus ouverts compris), lit leur propriété `text`, et rend une ligne
  `TEXTE ECART <objectName ou classe> "<texte>"` par texte non vide qui n'est pas encadré des marques (chiffres seuls
  et éléments ignorés exclus), puis `TEXTES OK <n>` ou `TEXTES EN ECART <k>` (étape en écart s'il y en a).

**Fichiers.** `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`,
`bench/include/actcraft/bench/injector.h`, `bench/src/injector.cpp`, `tests/tst_bench_uiscenario.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_bench_uiscenario` ; « fonctions d'un test » sur
`tst_bench_uiscenario`.

**Interdits.** Aucune étape existante changée ; aucun appel de commande de l'application depuis ces étapes (elles
injectent des événements, comme `touch`) ; aucune API privée de Qt nouvelle (`QWindowSystemInterface`, en-têtes
`*_p.h`).

**Terminé quand** les sept fonctions nouvelles et toutes les anciennes passent, et le contrôle des API privées
(commandes communes, N4) sort vide.

**Étapes.**
- [ ] **Étape 1 : test rouge** (ajouté à `tests/tst_bench_uiscenario.cpp`, sur le modèle de `touchStepParsed`) :
  ```cpp
  void parallelStepParsed()
  {
      const QByteArray json = R"({"name":"p","steps":[{"parallel":[{"device":1,"kind":"stylus","points":[[10,10],[50,10]]},{"device":2,"kind":"touch","points":[[10,90],[50,90]]}]}]})";
      actcraft::bench::UiScenario scenario;
      QString error;
      QVERIFY2(actcraft::bench::parseUiScenario(json, scenario, error), qPrintable(error));
      QCOMPARE(scenario.steps.size(), 1);
      QCOMPARE(scenario.steps[0].kind, actcraft::bench::UiStepKind::Parallel);
      QCOMPARE(scenario.steps[0].contacts.size(), 2);
      QCOMPARE(scenario.steps[0].contacts[1].device, 2);
  }
  void resizeStepParsed();       // {"resize":[800,1280]} → UiStepKind::Resize, size == QSize(800, 1280)
  void appStateStepParsed();     // {"appState":"suspended"} → UiStepKind::AppState, Qt::ApplicationSuspended ; "zzz" → parseUiScenario rend faux
  void longPressStepParsed();    // {"longPress":[100,100],"holdMs":600} → UiStepKind::LongPress, holdMs 600, kind touch par défaut
  void visibleTextsStepParsed(); // marques par défaut ⟦ ⟧ ; "ignore" lu
  void expectChangedStepParsed(); // {"expectChanged":"pageView.zoom","from":"${z0}"} → UiStepKind::ExpectChanged
  void parallelInterleavesTwoDevices(); // Injector::eventLog() : P1 P2 M1 M2 … R1 R2
  ```
  Forme réelle : `bool parseUiScenario(const QByteArray& json, UiScenario& out, QString& error)`
  (`bench/include/actcraft/bench/uiscenario.h`) ; le champ `contacts` est ajouté **à la fin** de `UiStep`.
- [ ] **Étape 2 : rouge vu** (commande ciblée : `tests failed`).
- [ ] **Étape 3 : implémentation.** `UiStepKind::{Parallel, Resize, AppState, LongPress, VisibleTexts, ExpectChanged}` ; structure
  `UiContact{int device; QString kind; QList<QPointF> points;}` ; analyse ; exécution (injection par `Injector`, même chemin que
  `touch` et `stroke` ; un `QPointingDevice` public par numéro, créé à la première utilisation) ; lignes de sortie ci-dessus.
- [ ] **Étape 4 : vert**, « fonctions d'un test » (`Totals` sans échec ni saut), preuve de tâche.
- [ ] **Étape 5 : commit** « Lot 11 : étapes du joueur pour le toucher, la rotation et les textes (L11-T2) ».

<a id="tache-l11-t3"></a>
### Tâche L11-T3 — Coquille QML du lot

**Objectif.** Poser une fois les instances et les emplacements du lot, pour que les vagues suivantes n'écrivent plus
dans `Main.qml` (sauf L11-T10, L11-T9 et L11-T13, dans cet ordre de vagues) : dans `Main.qml`, à côté de `appCommands` et `appPreferences` (L6-T5) :
```qml
TouchModeController { id: appTouchMode; objectName: "appTouchMode"; commands: appCommands; preferences: appPreferences; view: pageArea.view }
DualMode { id: appDualMode; objectName: "appDualMode"; commands: appCommands; view: pageArea.view }
LayoutMetrics { id: appLayout; objectName: "appLayout"; windowWidth: root.width; windowHeight: root.height; preferences: appPreferences }
LifecycleKeeper { id: appLifecycle; objectName: "appLifecycle"; document: appDocument; view: pageArea.view }
DocumentPortal { id: appPortal; objectName: "appPortal" }
UpdateChecker { id: appUpdates; objectName: "appUpdates"; preferences: appPreferences }
```
emplacements `SecondPalette { visible: appDualMode.active }` (bord gauche), `RecoveryBanner`, `FingerMenu`,
`TouchModeMenu` (sous-menu d'Affichage), `UpdateDialog` ; gestionnaire talon de chaque commande R4 posé **dans le
fichier QML où sa tâche écrira le vrai** (règle de l'amendement 1 du lot 6).

**Fichiers.** `ui/qml/Main.qml`, `ui/qml/TopBar.qml`, `ui/qml/ToolPalette.qml`,
`ui/qml/{SecondPalette,TouchModeMenu,FingerMenu,RecoveryBanner,UpdateDialog}.qml`,
`ui/include/actcraft/ui/{touchmode,dualmode,layoutmetrics,lifecycle,documentportal,updatechecker}.h`,
`ui/src/{touchmode,dualmode,layoutmetrics,lifecycle,documentportal,updatechecker}.cpp` (propriétés déclarées et
corps talons gardés, marqueur inchangé), `tests/tst_ui_shell.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_shell` ; « fonctions d'un test » sur `tst_ui_shell`.

**Preuve visuelle.** `captures.ps1 -Scenario premiers-pas …` inchangé : `CAPTURES OK` (non-régression).

**Interdits.** Aucune logique dans les talons QML (un `Item` visible à la taille de son contenu, ou masqué selon la
liaison ci-dessus) ; aucune modification de `PageArea.qml`.

**Terminé quand** `lot11SlotsPresent` et `lot11ControllersWired` passent, avec toutes les fonctions existantes de
`tst_ui_shell`.

**Étapes.**
- [ ] **Étape 1 : test rouge** (dans `tests/tst_ui_shell.cpp`, montage `Shell` existant) :
  ```cpp
  void lot11SlotsPresent()
  {
      Shell s;
      QVERIFY(start(s));
      for (const char* name : {"appTouchMode", "appDualMode", "appLayout", "appLifecycle", "appPortal", "appUpdates",
                               "secondPalette", "recoveryBanner", "fingerMenu", "touchModeMenu", "updateDialog"})
          QVERIFY2(find(s, name) != nullptr, name);
      QCOMPARE(find(s, "secondPalette")->property("visible").toBool(), false);
  }
  void lot11ControllersWired()
  {
      Shell s;
      QVERIFY(start(s));
      QVERIFY(find(s, "appTouchMode")->property("commands").value<QObject*>() != nullptr);
      QVERIFY(find(s, "appTouchMode")->property("view").value<QObject*>() == find(s, "pageView"));
      QVERIFY(find(s, "appDualMode")->property("view").value<QObject*>() == find(s, "pageView"));
      QVERIFY(find(s, "appLifecycle")->property("document").value<QObject*>() != nullptr);
  }
  ```
  Montage réel (`tests/tst_ui_shell.cpp:32-55`) : `Shell s; QVERIFY(start(s));` ; aide locale du test
  `static QObject* find(Shell& s, const char* name) { return s.window->findChild<QObject*>(QString::fromLatin1(name)); }`.
- [ ] **Étape 2 : rouge vu**, puis **implémentation** (types C++ des talons R2 exposés à QML par `QML_ELEMENT`, propriétés
  déclarées dans leurs en-têtes ; instances et emplacements ci-dessus).
- [ ] **Étape 3 : vert**, preuve visuelle, banc sans écran, preuve de tâche ; commit « Lot 11 : coquille QML du lot
  (L11-T3) ».

<a id="tache-l11-t4"></a>
### Tâche L11-T4 — `input` : quatre politiques du toucher

**Objectif.** D11-3 et D11-4 ; H3-16. Politiques `IgnoreTouchPolicy`, `PenTouchPolicy`, `SelectTouchPolicy` à côté de
`FullTouchPolicy` ; outil imposé au doigt porté par la décision ; appuis d'un même événement décidés ensemble (deux
doigts posés ensemble → tous deux au geste en mode `full`).

**Fichiers.** `input/include/actcraft/input/touchmodes.h`, `input/src/touchmodes.cpp`,
`input/include/actcraft/input/routing.h`, `input/src/routing.cpp`, `input/include/actcraft/input/sample.h`,
`tests/tst_input_touchmodes.cpp`.

**Champ ajouté (amendement 1, M5).** `std::uint64_t deviceId = 0; // QPointingDevice::uniqueId, rempli par la vue
(L11-T5)` ajouté **en dernier** dans `input::Sample` ; aucun autre changement de `sample.h`. L11-T7 l'emploie dès V3.

**Contrat (en-tête figé).**
```cpp
// input/include/actcraft/input/touchmodes.h
#pragma once

#include <actcraft/input/routing.h>

#include <memory>
#include <string_view>

namespace actcraft::input {

enum class TouchMode : std::uint8_t
{
    Ignore, // le doigt ne fait rien
    Pen,    // le doigt écrit avec le stylo de l'utilisateur
    Select, // le doigt sélectionne
    Full,   // gestion complète : outil courant, gestes à deux doigts (décision 25)
};

std::string_view touchModeId(TouchMode mode) noexcept;            // "ignore", "pen", "select", "full"
bool touchModeFromId(std::string_view id, TouchMode& out) noexcept; // faux pour un identifiant inconnu

class IgnoreTouchPolicy final : public RoutingPolicy
{
public:
    Decision decide(const Sample& press, const RouterState& state) const override;
};
class PenTouchPolicy final : public RoutingPolicy
{
public:
    Decision decide(const Sample& press, const RouterState& state) const override;
};
class SelectTouchPolicy final : public RoutingPolicy
{
public:
    Decision decide(const Sample& press, const RouterState& state) const override;
};

std::unique_ptr<RoutingPolicy> makeTouchPolicy(TouchMode mode);

} // namespace actcraft::input
```
Dans `routing.h` : `enum class FingerTool : std::uint8_t { Current, Pen, Select };`, champ
`FingerTool fingerTool = FingerTool::Current;` dans `Decision`, champ `int pressesInEvent = 1;` dans `RouterState`
(nombre d'appuis de doigt du même événement), et
`std::vector<Decision> routePresses(std::span<const Sample> presses); // appuis d'un même événement`.

**Règles.** Ignore : stylet et souris → `Tool` ; doigt → `Ignore`. Pen / Select : stylet et souris → `Tool` (outil
courant) ; doigt pendant un contact de stylet ou de souris → `Ignore` ; premier doigt → `Tool` avec `fingerTool`
`Pen` ou `Select` ; doigt suivant → `Ignore` (aucun geste hors `full`). Full : comportement du lot 3, plus : si
`state.pressesInEvent >= 2`, chaque doigt de l'événement → `Gesture` (aucun point versé à l'outil).

**Test et commande bornée.** Commande ciblée `-R tst_input_touchmodes` ; « fonctions d'un test ».

**Interdits.** Ne pas toucher à la vue (L11-T5) ; ne pas changer le comportement de `FullTouchPolicy` pour un appui
seul (les tests du lot 3 restent verts) ; aucune allocation par échantillon dans `route` (fluidité).

**Terminé quand** les 9 fonctions passent, les tests `tst_input_*` existants aussi, et la mutation est vue rouge.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_input_touchmodes.cpp` :
  ```cpp
  #include <actcraft/input/touchmodes.h>

  #include <QtTest>

  using namespace actcraft::input;

  namespace {
  Sample press(DeviceKind kind, std::uint64_t pointer)
  {
      Sample s;
      s.device = kind;
      s.pointer = pointer;
      s.phase = Phase::Press;
      s.x = 10;
      s.y = 10;
      return s;
  }
  } // namespace

  class TstInputTouchModes : public QObject
  {
      Q_OBJECT
  private slots:
      void ignoreModeDropsFingers()
      {
          PointerRouter router;
          router.setPolicy(makeTouchPolicy(TouchMode::Ignore));
          QCOMPARE(router.route(press(DeviceKind::Touch, 7)).route, Route::Ignore);
          QCOMPARE(router.route(press(DeviceKind::Stylus, 0)).route, Route::Tool);
      }
      void penModeFingerDrawsWithPen()
      {
          PointerRouter router;
          router.setPolicy(makeTouchPolicy(TouchMode::Pen));
          const Decision d = router.route(press(DeviceKind::Touch, 7));
          QCOMPARE(d.route, Route::Tool);
          QCOMPARE(d.fingerTool, FingerTool::Pen);
      }
      void selectModeFingerSelects()
      {
          PointerRouter router;
          router.setPolicy(makeTouchPolicy(TouchMode::Select));
          QCOMPARE(router.route(press(DeviceKind::Touch, 7)).fingerTool, FingerTool::Select);
      }
      void fullModeIsTheLot3Behaviour()
      {
          PointerRouter router; // FullTouchPolicy par défaut
          QCOMPARE(router.route(press(DeviceKind::Touch, 7)).route, Route::Tool);
          const Decision second = router.route(press(DeviceKind::Touch, 8));
          QCOMPARE(second.route, Route::Gesture);
          QVERIFY(second.endToolContacts);
      }
      void noGestureOutsideFullMode()
      {
          for (TouchMode mode : {TouchMode::Pen, TouchMode::Select}) {
              PointerRouter router;
              router.setPolicy(makeTouchPolicy(mode));
              router.route(press(DeviceKind::Touch, 7));
              QCOMPARE(router.route(press(DeviceKind::Touch, 8)).route, Route::Ignore);
          }
      }
      void palmIgnoredInEveryMode()
      {
          for (TouchMode mode : {TouchMode::Ignore, TouchMode::Pen, TouchMode::Select, TouchMode::Full}) {
              PointerRouter router;
              router.setPolicy(makeTouchPolicy(mode));
              router.route(press(DeviceKind::Stylus, 0));
              QCOMPARE(router.route(press(DeviceKind::Touch, 7)).route, Route::Ignore);
          }
      }
      void sameEventTwoFingersGoToGesture()
      {
          PointerRouter router;
          const Sample both[] = {press(DeviceKind::Touch, 7), press(DeviceKind::Touch, 8)};
          const auto decisions = router.routePresses(both);
          QCOMPARE(decisions.size(), std::size_t(2));
          QCOMPARE(decisions[0].route, Route::Gesture);
          QCOMPARE(decisions[1].route, Route::Gesture);
          QCOMPARE(router.gestureTouches(), 2);
      }
      void idsRoundTrip()
      {
          for (TouchMode mode : {TouchMode::Ignore, TouchMode::Pen, TouchMode::Select, TouchMode::Full}) {
              TouchMode back = TouchMode::Full;
              QVERIFY(touchModeFromId(touchModeId(mode), back));
              QCOMPARE(back, mode);
          }
          TouchMode unknown = TouchMode::Pen;
          QVERIFY(!touchModeFromId("zzz", unknown));
          QCOMPARE(unknown, TouchMode::Pen);
      }
      void policyChangeKeepsContacts()
      {
          PointerRouter router;
          router.route(press(DeviceKind::Touch, 7)); // Tool, mode full
          router.setPolicy(makeTouchPolicy(TouchMode::Ignore));
          Sample move = press(DeviceKind::Touch, 7);
          move.phase = Phase::Move;
          QCOMPARE(router.route(move).route, Route::Tool); // décision retenue jusqu'au relâché
      }
  };

  QTEST_GUILESS_MAIN(TstInputTouchModes)
  #include "tst_input_touchmodes.moc"
  ```
- [ ] **Étape 2 : rouge vu** (commande ciblée).
- [ ] **Étape 3 : implémentation.**
  ```cpp
  // input/src/touchmodes.cpp (extrait : la politique du stylo ; Select est identique avec FingerTool::Select)
  Decision PenTouchPolicy::decide(const Sample& press, const RouterState& state) const
  {
      if (press.device != DeviceKind::Touch)
          return Decision{Route::Tool};
      if (state.stylusDown || state.toolTouches > 0 || state.gestureTouches > 0)
          return Decision{Route::Ignore};
      Decision d{Route::Tool};
      d.fingerTool = FingerTool::Pen;
      return d;
  }
  ```
  `routePresses` : pose `m_state.pressesInEvent = <nombre de doigts parmi presses>` pendant la décision de chaque appui,
  puis le remet à 1 ; `FullTouchPolicy::decide` : `if (press.device == DeviceKind::Touch && state.pressesInEvent >= 2)
  return Decision{Route::Gesture};` avant ses règles actuelles.
- [ ] **Étape 4 : vert**, puis **mutation** : dans `FullTouchPolicy::decide`, retirer la règle `pressesInEvent` → la
  commande ciblée montre `sameEventTwoFingersGoToGesture` en `FAIL!` ; `git -C <wt> checkout -- input/src/routing.cpp`
  (ou le fichier où elle est écrite) ; vert à nouveau.
- [ ] **Étape 5 : preuve de tâche**, banc sans écran ; commit « Lot 11 : quatre modes du toucher dans le routeur
  (L11-T4) ».

<a id="tache-l11-t5"></a>
### Tâche L11-T5 — `PageView` : changements de la vue pour le lot

**Objectif.** Toutes les modifications de la vue du lot, en une tâche (patron à éviter : chaque fonction qui retouche
`pageview.cpp` de son côté) :
1. `void setRoutingPolicy(std::unique_ptr<input::RoutingPolicy> policy)` (C++) : remplace la politique du routeur ; les
   contacts en cours gardent leur décision ;
2. `Sample::deviceId` (champ posé par L11-T4) rempli depuis
   `QPointingDevice::uniqueId().numericId()` (à défaut `systemId()`) ;
3. appuis d'un même `QTouchEvent` envoyés ensemble à `PointerRouter::routePresses` ;
4. la décision porte l'utilisateur : les échantillons vont à `ToolHost::user(decision.user)` ; `fingerTool` `Pen` ou
   `Select` fait agir l'outil imposé pour ce contact, sans changer l'outil courant ; lecture
   `int lastContactUser() const` (utilisateur de la dernière décision d'appui, pour les tests) ;
5. `Q_INVOKABLE void finishContacts()` : chaque contact en cours se termine comme un relâché à sa dernière position
   (trait gardé), le routeur est remis à zéro ;
6. signal `touchHeld(QPointF pagePoint, QPointF windowPoint, int user)` : un doigt routé vers l'outil, immobile
   (≤ 10 dp) pendant 500 ms (D11-7) ; aucun minuteur armé hors d'un contact de doigt ;
7. au redimensionnement de la vue : le point de page au centre reste au centre (D11-10), et un contact en cours est
   terminé par `finishContacts()`.

**Fichiers.** `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`, `tests/tst_ui_pageinput.cpp`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_pageinput|tst_ui_shell|tst_ui_drawing'` ; « fonctions d'un
test » sur `tst_ui_pageinput`.

**Interdits.** Aucun changement de comportement en mode `full` à un seul utilisateur (les tests du lot 3 et
`tst_ui_drawing` restent verts) ; aucune lecture des réglages dans la vue (le contrôleur de L11-T6 pose la politique) ;
aucun minuteur par échantillon ; aucune modification d'un autre fichier QML.

**Terminé quand** les 9 fonctions passent, `tst_ui_shell` et `tst_ui_drawing` restent `Passed`, et le banc sans écran
est joint (pas de recul de la latence d'encre au-delà du bruit du banc ; chiffre rapporté).

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_ui_pageinput.cpp` (montage : `Main.qml` chargé comme `tst_ui_drawing`, réglages
  d'essai en INI temporaire), fonctions :
  - `policyReplaceable` : `setRoutingPolicy(makeTouchPolicy(TouchMode::Ignore))`, un doigt injecté
    (`QTest::touchEvent`) → aucun objet ajouté à la page ;
  - `deviceIdFilled` : deux `QPointingDevice` de test (`QPointingDevice(…, uniqueId 11)` et `22`) ; les échantillons
    relevés par un `SampleSink` espion portent `deviceId` 11 puis 22 ;
  - `sameEventPressesGrouped` : un `QTouchEvent` à deux appuis → `gestureTouches() == 2`, aucun point d'encre ;
  - `decisionUserReachesTheView` : politique d'essai (classe locale du test) qui rend `Decision{Route::Tool, 1}` →
    `lastContactUser() == 1` après l'appui (« l'utilisateur 1 reçoit, l'utilisateur 0 rien » est prouvé par L11-T7,
    mode à deux utilisateurs allumé : avant lui, `ToolHost` n'a qu'un `UserState`) ;
  - `imposedPenDrawsWhileSelectIsCurrent` : outil courant sélecteur, `PenTouchPolicy`, un doigt trace → un trait
    ajouté, outil courant toujours le sélecteur ;
  - `finishContactsKeepsTheStroke` : appui et deux mouvements au stylet, `finishContacts()` → le trait est dans la page
    et `contactActive` est faux ;
  - `touchHeldAfter500ms` : doigt immobile 600 ms (`QTest::qWait`) → signal émis une fois ; doigt qui bouge de 20 dp →
    aucun signal ;
  - `resizeKeepsTheCentre` : point de page au centre relevé, vue 1280 × 800 → 800 × 1280 → même point au centre à 1 px
    près ;
  - `resizeEndsTheContact` : contact de stylet en cours au redimensionnement → trait gardé dans la page,
    `contactActive` faux.
- [ ] **Étape 2 : rouge vu.**
- [ ] **Étape 3 : implémentation**, dans l'ordre 2, 1, 3, 4, 5, 7, 6 ; chaque point vert avant le suivant (commande
  ciblée).
- [ ] **Étape 4 : mutation** : supprimer l'appel à `finishContacts()` dans le gestionnaire de redimensionnement →
  `resizeEndsTheContact` en `FAIL!` (commande ciblée) ; `git -C <wt> checkout -- ui/src/pageview.cpp` ; vert.
- [ ] **Étape 5 : preuve de tâche**, banc sans écran ; commit « Lot 11 : vue — politique remplaçable, utilisateur,
  appui long, rotation (L11-T5) ».

<a id="tache-l11-t6"></a>
### Tâche L11-T6 — Réglage du mode du toucher ; scénario `modes-toucher`

**Objectif.** F-930 (toucher). `ui::TouchModeController : QObject`, `QML_ELEMENT` : propriétés `commands`
(`Commands*`), `preferences` (`Preferences*`), `view` (`PageView*`), `mode` (`QString`, `"full"` par défaut) ; lit
`workstation/touch-mode` au démarrage ; `touch.mode <id>` (gestionnaire natif, `Commands::setNativeHandler`) change le
mode, l'écrit, et pose la politique par `view->setRoutingPolicy(input::makeTouchPolicy(…))` ; un changement **pendant un
contact** (`view->contactActive()`) est différé jusqu'au relâché ; identifiant inconnu refusé. `TouchModeMenu.qml` :
quatre choix exclusifs, cochés selon `mode`.

**Fichiers.** `ui/include/actcraft/ui/touchmode.h`, `ui/src/touchmode.cpp`, `ui/qml/TouchModeMenu.qml`,
`tests/tst_ui_touchmode.cpp`, `bench/scenarios/modes-toucher.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_touchmode|tst_captures_modes_toucher'`.

**Preuve visuelle.** `captures.ps1 -Scenario modes-toucher -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "appTouchMode.mode=full"` :
`LU appTouchMode.mode=full OK`, les quatre `PIXEL` du scénario `OK`, `CAPTURES OK` ; décrire `pen.png` (trait au
doigt) et `select.png` (cadre de sélection autour du rectangle).

**Interdits.** Le contrôleur ne touche ni au routeur ni aux échantillons : il passe par `setRoutingPolicy`.

**Terminé quand** les 6 fonctions et le scénario passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_touchmode.cpp` : `fullByDefault` ; `commandChangesModeAndSetting` (`touch.mode
  pen` → `mode == "pen"`, INI `workstation/touch-mode=pen`) ; `settingReadAtStart` (nouvel objet sur le même INI →
  `pen`) ; `unknownRefused` (`touch.mode zzz` → `trigger` faux, mode inchangé) ; `changeDeferredDuringContact`
  (`contactActive` vrai → mode inchangé jusqu'à `contactActiveChanged(false)`) ; `policyApplied` (vue d'essai : la
  politique posée est celle du mode, vérifiée par un doigt injecté).
- [ ] **Étape 2 : implémentation et QML.**
- [ ] **Étape 3 : scénario** `bench/scenarios/modes-toucher.json` (S28, seconde partie : un doigt par mode) :
  ```json
  {
    "format": "actcraft-scenario/1",
    "name": "modes-toucher",
    "description": "S28 (modes du toucher) : pour chacun des quatre modes, un doigt injecté produit l'effet attendu.",
    "steps": [
      {"waitReady": true},
      {"command": "tool.shape"},
      {"command": "shape.choose", "argument": "rectangle"},
      {"stroke": [[600, 500], [800, 650]], "pressure": 0.5},
      {"command": "tool.pen"},
      {"command": "touch.mode", "argument": "ignore"},
      {"expect": "appTouchMode.mode", "value": "ignore"},
      {"touch": [[200, 200], [400, 200]]},
      {"waitReady": true},
      {"checkPixel": "ignore-rien", "at": [300, 200], "color": "#FFFFFF", "tolerance": 8},
      {"command": "touch.mode", "argument": "pen"},
      {"command": "tool.select"},
      {"touch": [[200, 300], [400, 300]]},
      {"waitReady": true},
      {"checkPixel": "pen-trait", "at": [300, 300], "color": "#000000", "tolerance": 90},
      {"capture": "pen"},
      {"command": "touch.mode", "argument": "select"},
      {"command": "tool.pen"},
      {"touch": [[700, 575], [700, 575]]},
      {"waitReady": true},
      {"expect": "appTools.selectionCount", "value": 1},
      {"capture": "select"},
      {"command": "touch.mode", "argument": "full"},
      {"touch": [[200, 400], [400, 400]]},
      {"waitReady": true},
      {"checkPixel": "full-trait", "at": [300, 400], "color": "#000000", "tolerance": 90},
      {"expect": "appTouchMode.mode", "value": "full"}
    ]
  }
  ```
  Identifiants réels : `tool.shape` puis `shape.choose` (lot 4, `docs/plans/2026-10-18-livraison-complete-lot4-PLAN.md:407` ;
  forme de l'argument relue par L11-T0), `tool.select`, `tool.pen` ; sélection lue par `appTools.selectionCount`
  (lot 3) ; couleur d'encre par défaut relue par L11-T0.
- [ ] **Étape 4 : vert**, preuve visuelle, banc sans écran, preuve de tâche ; contrôle des titres du corpus vide ;
  `NOMS OK` ; commit « Lot 11 : réglage du mode du toucher (L11-T6) ».

<a id="tache-l11-t7"></a>
### Tâche L11-T7 — Second utilisateur I : politique et état par utilisateur

**Objectif.** D11-2, DA-A1, F-303. `input::TwoUserPolicy final : RoutingPolicy` : construite avec la moitié de la vue
attribuée à l'utilisateur 2 (`QRectF secondUserArea` en coordonnées de vue, mis à jour par `setSecondUserArea`) ;
décide l'utilisateur selon D11-2 (souris → 0 ; premier `deviceId` de stylet vu → 0, second distinct → 1 ; doigt → 1 dans
`secondUserArea`, sinon 0) ; route `Tool`, ou `Ignore` pour la paume (doigt pendant un contact de stylet **du même
utilisateur**) et pour tout doigt de plus par utilisateur ; **jamais `Gesture`**. `ToolHost` tient deux `UserState`
(identifiants 0 et 1), chacun son outil courant et ses réglages d'outil ; `ToolHost::user(1)` n'existe qu'en mode à deux
utilisateurs (sinon il rend l'utilisateur 0, comme le socle).

**Contrat.**
```cpp
// input/include/actcraft/input/twouserpolicy.h
class TwoUserPolicy final : public RoutingPolicy
{
public:
    explicit TwoUserPolicy(QRectF secondUserArea);
    void setSecondUserArea(QRectF area);
    Decision decide(const Sample& press, const RouterState& state) const override;
    // Appareils de stylet vus, dans l'ordre (le premier est l'utilisateur 0) ; réinitialisé à la construction.
    std::uint64_t firstStylusDevice() const noexcept;

private:
    QRectF m_area;
    mutable std::uint64_t m_firstStylus = 0; // 0 : aucun vu
};
```
`RouterState` gagne `bool stylusDownByUser[2] = {false, false};` (au lieu d'un seul booléen pour cette politique ;
`stylusDown` reste pour les autres), écrit **par cette tâche** dans `input/include/actcraft/input/routing.h` (L11-T4,
mergée en V2, n'y touche plus ; ni L11-T5 ni L11-T10 n'y touchent en V3). `ToolHost::setDualUsers(bool)` (créneau
ajouté ici, appelé par `DualMode` en L11-T9) crée le second `UserState` ; **un `UserState` neuf commence au stylo**
(`pen`, ou la valeur tranchée par L11-T0 ; amendement 2, n2).

**Fichiers.** Ceux de la ligne de la table. Enregistrement de `tst_input_twousers` : forme F4 (R1).

**Test et commande bornée.** Commande ciblée `-R 'tst_input_twousers|tst_input_'`.

**Interdits.** Aucun changement à `FullTouchPolicy` ni aux trois politiques de L11-T4 ; aucune boîte de dialogue ni
QML ici.

**Terminé quand** les 7 fonctions passent et la mutation est vue rouge.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_input_twousers.cpp` : `mouseIsFirstUser` ; `secondStylusIsSecondUser` (deux appuis
  de stylet `deviceId` 11 puis 22 → utilisateurs 0 puis 1 ; un troisième appui 11 → 0) ; `fingerByHalf` (aire
  `QRectF(0, 0, 640, 800)` ; doigt en (100, 100) → 1, en (900, 100) → 0) ; `noGestureInDualMode` (deux doigts de
  l'utilisateur 0 → `Tool` puis `Ignore`, jamais `Gesture`) ; `palmOfOneUserOnly` (stylet de l'utilisateur 0 posé ;
  doigt dans l'aire de l'utilisateur 1 → `Tool`, utilisateur 1) ; `eachUserKeepsItsTool` (`ToolHost` : `user(0)`
  stylo rouge, `user(1)` gomme ; relus distincts) ; `singleUserWhenOff` (`ToolHost::user(1)` sans mode → même objet que
  `user(0)`). `eachUserKeepsItsTool` allume `setDualUsers(true)` et vérifie d'abord que `user(1)` neuf est au stylo.
  (« L'utilisateur 1 reçoit, l'utilisateur 0 rien » est prouvé par `secondUserReceivesInDualMode`, dans L11-T9 :
  amendement 2, N2.)
- [ ] **Étape 2 : implémentation.**
- [ ] **Étape 3 : mutation** : faire rendre `Gesture` au deuxième doigt → `noGestureInDualMode` rouge ;
  `git -C <wt> checkout -- input/src/twouserpolicy.cpp` ; vert.
- [ ] **Étape 4 : preuve de tâche**, banc sans écran ; commit « Lot 11 : deux utilisateurs — routage et état (L11-T7) ».

<a id="tache-l11-t8"></a>
### Tâche L11-T8 — Second utilisateur II : sélection, objets tenus, outils de maths

**Objectif.** F-303 : chaque utilisateur sa sélection (le sélecteur de l'utilisateur 1 ne voit ni ne change celle de
l'utilisateur 0) ; `tools::Grabs` : registre des objets tenus (`bool hold(core::ObjectId, input::UserId)`,
`void release(input::UserId)`, `std::optional<input::UserId> holder(core::ObjectId) const`) ; la désignation d'un
utilisateur saute les objets tenus par l'autre ; un outil de maths ouvert par un utilisateur appartient à son
`UserState::openMathTools()` et ne guide pas l'encre de l'autre.

**Fichiers.** Ceux de la ligne de la table ; les fichiers des outils de maths sont relevés par L11-T0 (A11) et écrits
dans la ligne par l'orchestrateur avant le lancement.

**Test et commande bornée.** Commande ciblée `-R 'tst_tools_twousers|tst_tools_select'`.

**Interdits.** Aucune sélection globale gardée dans la vue ; aucun changement de comportement à un seul utilisateur
(`tst_tools_select` reste vert).

**Terminé quand** les 6 fonctions passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_tools_twousers.cpp` : `selectionsAreSeparate` ; `heldObjectRefusedToTheOther`
  (utilisateur 0 glisse un rectangle et le tient ; l'utilisateur 1 appuie dessus → aucune sélection, l'objet ne bouge
  pas) ; `releasedObjectIsFreeAgain` ; `rulerBelongsToItsUser` (règle ouverte par 0 → `user(0).openMathTools()` la
  contient, `user(1)` non) ; `inkOfTheOtherNotGuided` (trait de l'utilisateur 1 le long de la règle de 0 → non
  redressé) ; `singleUserUnchanged`.
- [ ] **Étape 2 : implémentation** (le sélecteur reçoit son `UserState` par `ToolContext`, A3, A15).
- [ ] **Étape 3 : vert**, preuve de tâche, banc sans écran ; commit « Lot 11 : deux utilisateurs — sélection, objets
  tenus, outils de maths (L11-T8) ».

<a id="tache-l11-t9"></a>
### Tâche L11-T9 — Deux utilisateurs III : mode, deuxième palette, boîtes refusées ; scénario S28

**Objectif.** F-110, F-303, N2-A7.
- `actions` : attribut `opensDialog` (booléen, `false` par défaut) lu de `commands.json` et rempli pour **toutes** les
  commandes des lots 1 à 11 qui ouvrent une boîte (liste établie en lisant les gestionnaires QML : `Dialog`, `Popup`
  modal, sélecteur de fichier) ;
- `ui::Commands::trigger(id, argument, user)` (surcharge ; l'ancienne signature vaut `user = 0`) refuse une commande
  `opensDialog` pour un utilisateur autre que 0 (rend faux, aucun effet, signal `refusedForSecondUser(id)`) ;
- `ui::DualMode : QObject`, `QML_ELEMENT` : `active` (bool), `commands`, `view` ; `users.dual` bascule ; activer pose
  `TwoUserPolicy` sur la vue (aire = moitié gauche, mise à jour au redimensionnement), désactiver remet la politique du
  mode du toucher courant (`appTouchMode.mode`) ; bascule différée hors contact ;
- joueur : champ `"user": <n>` de l'étape `command` (`Commands::trigger(id, argument, user)` ; 0 par défaut) ;
  fonction `commandStepCarriesTheUser` ajoutée à `tst_bench_uiscenario` ;
- `SecondPalette.qml` : contenu fixe établi par L11-T0 (à défaut : stylo, surligneur, gomme, sélecteur, couleurs
  rouge, bleu, vert, noir, épaisseurs 2, 4, 8), chaque bouton déclenche sa commande avec `user = 1` ; non
  personnalisable.

**Gestionnaires et utilisateur (amendement 1, M7).** `Commands::trigger(id, argument, user)` appelle le gestionnaire
JS avec `(argument, user)` ; dans `Main.qml`, les gestionnaires des outils et du stylo passent l'utilisateur :
`pen.color` → `appTools.setColor(color, user)`, `pen.width` → `appTools.setWidth(width, user)`, `tool.*` →
`appTools.activate(<outil>, user)` (forme du lot 3 : `appTools.activate("pen")`) ; `ToolHost` (instance `appTools`) gagne ces surcharges (`user` = 0 par défaut :
les appels existants ne changent pas). `DualMode` appelle `ToolHost::setDualUsers(bool)` (L11-T7).

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_secondpalette|tst_captures_deux_utilisateurs|tst_actions_'`.

**Preuve visuelle.** `captures.ps1 -Scenario deux-utilisateurs -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "appDualMode.active=false"` :
lectures, `PIXEL` et `CHANGE pageView.zoom OK` du scénario, `CAPTURES OK` ; décrire `deux-traits.png` (deux traits simultanés, deux
couleurs, deux épaisseurs ; deuxième palette à gauche).

**Interdits.** Aucune voie QML qui contourne `trigger` pour le second ; aucune personnalisation de la deuxième palette.

**Terminé quand** les 8 fonctions et le scénario passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_secondpalette.cpp` : `opensDialogParsed` (`file.open` vrai, `pen.color` faux) ;
  `everyDialogCommandMarked` (pour chaque commande dont le gestionnaire est relevé comme ouvrant une boîte — liste
  écrite dans le test, `help.check-updates` compris (sa boîte arrive en L11-T22) — `opensDialog` vrai) ; `secondUserCannotOpenADialog` (`trigger("file.open", {}, 1)` faux,
  signal émis) ; `firstUserStillCan` ; `toggleSetsTheTwoUserPolicy` ; `offRestoresTheTouchMode` (mode `pen`, deux
  utilisateurs allumé puis éteint → politique du stylo) ; `pinchZoomsOnlyWhenOff` (pincement injecté : échelle
  inchangée en mode allumé, changée éteint) ; `secondUserReceivesInDualMode` (déplacé de L11-T7, amendement 2, N2 :
  `Main.qml` chargé (F1), `users.dual` allumé, donc `ToolHost::setDualUsers(true)` et `TwoUserPolicy` posée par
  `PageView::setRoutingPolicy` ; un appui de stylet de l'appareil 22 après un de l'appareil 11 atteint l'outil d'essai
  de `user(1)` et pas celui de `user(0)` — compteurs des deux outils d'essai).
- [ ] **Étape 2 : implémentation**, données, QML.
- [ ] **Étape 3 : scénario** `bench/scenarios/deux-utilisateurs.json` (S28, première partie) :
  ```json
  {
    "format": "actcraft-scenario/1",
    "name": "deux-utilisateurs",
    "description": "S28 : deux pointeurs de deux utilisateurs tracent en même temps ; sélections séparées ; boîte refusée au second ; pincement suspendu puis rétabli.",
    "steps": [
      {"waitReady": true},
      {"command": "users.dual"},
      {"expect": "appDualMode.active", "value": true},
      {"expect": "secondPalette.visible", "value": true},
      {"command": "tool.pen"},
      {"command": "pen.color", "argument": "#E53935"},
      {"command": "pen.width", "argument": 8},
      {"command": "tool.pen", "user": 1},
      {"command": "pen.color", "argument": "#1E88E5", "user": 1},
      {"command": "pen.width", "argument": 2, "user": 1},
      {"parallel": [
        {"device": 1, "kind": "stylus", "points": [[700, 300], [1000, 300]]},
        {"device": 2, "kind": "stylus", "points": [[100, 500], [400, 500]]}
      ], "pressure": 0.5},
      {"waitReady": true},
      {"checkPixel": "trait-utilisateur-1", "at": [850, 300], "color": "#E53935", "tolerance": 60},
      {"checkPixel": "trait-utilisateur-2", "at": [250, 500], "color": "#1E88E5", "tolerance": 60},
      {"capture": "deux-traits"},
      {"remember": "pageView.zoom", "as": "z0"},
      {"pinch": [640, 400], "from": 100, "to": 200, "steps": 10},
      {"waitReady": true},
      {"expect": "pageView.zoom", "value": "${z0}"},
      {"command": "users.dual"},
      {"expect": "appDualMode.active", "value": false},
      {"pinch": [640, 400], "from": 100, "to": 200, "steps": 10},
      {"waitReady": true},
      {"expectChanged": "pageView.zoom", "from": "${z0}"},
      {"expect": "secondPalette.visible", "value": false}
    ]
  }
  ```
  `pageView.zoom` est le zoom de la vue (`ui/include/actcraft/ui/pageview.h:41, 46`), pas `scale` (propriété de
  `QQuickItem`, toujours 1) ; formes `remember` et `${z0}` : A23. Le bleu `#1E88E5` et l'épaisseur 2 font partie du contenu par défaut de la deuxième palette (amendé par L11-T0 si
  elle diffère). La sélection séparée, l'objet
  tenu et la règle sont prouvés par `tst_tools_twousers` (L11-T8) ; la boîte refusée par `tst_ui_secondpalette`.
- [ ] **Étape 4 : vert**, preuve visuelle, banc sans écran, preuve de tâche ; `NOMS OK` ; contrôle des titres du corpus
  vide ; commit « Lot 11 : mode à deux utilisateurs et deuxième palette (L11-T9) ».

<a id="tache-l11-t10"></a>
### Tâche L11-T10 — Tablette I : mise en page étroite et cibles tactiles

**Objectif.** D11-5, D11-6 ; décision 14 du cadrage. `ui::LayoutMetrics : QObject`, `QML_ELEMENT` : entrées
`windowWidth`, `windowHeight` (dp logiques), `preferences` ; sorties `narrow` (< 960), `compact` (< 600), `portrait`
(hauteur > largeur), `touchUi` (Android, ou `workstation/touch-ui` vrai), `targetSize` (48 si `touchUi`, sinon la
taille du lot 6), `spacing` (8 si `touchUi`). `Theme.qml` lie ses tailles de boutons à `appLayout.targetSize` ;
`BrowserPanel.qml` devient un tiroir (`Drawer`) quand `narrow`, fermé par défaut ; `PaletteFrame.qml` offre un bouton
de repli quand `narrow` ; `Main.qml` regroupe les menus de la ligne supérieure dans un menu « … » quand `compact`.

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_narrowlayout|tst_captures_tablette_etroite|tst_ui_shell'`.

**Preuve visuelle.** Un passage (les `-Lire` sont lus **après** le scénario, `bench/captures/main.cpp:134-137` ; le
scénario change lui-même la taille par `resize`) :
`captures.ps1 -Scenario tablette-etroite -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "appLayout.narrow=false"` :
`TAILLE 800x1280 OK`, `TAILLE 1280x800 OK`, `LU … OK`, `CAPTURES OK` ; décrire `portrait.png` (navigateurs masqués,
palette repliée en un bouton, page entière visible), `tiroir-ouvert.png` et `large.png` (disposition d'origine).

**Interdits.** Aucune taille en pixels physiques (dp logiques seulement) ; aucun changement de la disposition Windows
en largeur ≥ 960 sans `touch-ui` (comportement et disposition de l'original, décision 0100).

**Terminé quand** les 6 fonctions et le scénario passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_narrowlayout.cpp` : `thresholds` (959 → `narrow`, 960 → non ; 599 → `compact`) ;
  `touchUiFromSetting` ; `everyTargetAtLeast48dp` (fenêtre 800 × 1280, `touchUi` vrai : chaque `CommandButton` visible
  de la palette, de la ligne supérieure et du tiroir a `width` et `height` ≥ 48, et deux voisins sont espacés d'au
  moins 8) ; `browsersHiddenWhenNarrow` ; `paletteCollapsible` ; `wideLayoutUnchanged` (1280 × 800 sans `touchUi` :
  positions des éléments de la palette identiques à celles relevées avant la tâche, liste écrite dans le test).
- [ ] **Étape 2 : implémentation** et QML.
- [ ] **Étape 3 : scénario** `tablette-etroite.json` : `waitReady` ; `{"resize": [800, 1280]}` ;
  `{"expect": "appLayout.narrow", "value": true}` ; capture `portrait` ; ouvrir le tiroir des navigateurs par son bouton (`pointer`, coordonnées de fenêtre
  relevées) ; `expect browserPanel.opened = true` ; capture `tiroir-ouvert` ; appui hors du tiroir ;
  `expect browserPanel.opened = false` ; un trait au doigt (`touch`) visible (`checkPixel`) ;
  `{"resize": [1280, 800]}` ; `{"expect": "appLayout.narrow", "value": false}` ; capture `large`.
- [ ] **Étape 4 : vert**, preuve visuelle, banc sans écran, preuve de tâche ; commit « Lot 11 : mise en
  page étroite et cibles tactiles (L11-T10) ».

<a id="tache-l11-t11"></a>
### Tâche L11-T11 — Tablette II : rotation et clavier logiciel

**Objectif.** D11-10, D11-11. `ui::KeyboardInset : QObject`, `QML_ELEMENT` : entrée `keyboardRect` (liée à
`Qt.inputMethod.keyboardRectangle`, en coordonnées de fenêtre), `caretRect` (rectangle du curseur de l'outil texte en
fenêtre, A19), `margin` (16) ; sortie `shift` (défilement vertical à appliquer à la vue, ≥ 0 ; 0 quand le clavier est
masqué). `PageArea.qml` applique `shift` à la vue pendant l'édition d'un texte et revient à 0 à la fermeture. La
rotation est portée par la vue (L11-T5, point 7) ; cette tâche la prouve de bout en bout et sur l'émulateur.

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_orientation|tst_captures_rotation_clavier'`.

**Preuve visuelle.** `captures.ps1 -Scenario rotation-clavier -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Taille 1280x800 -Lire "appLayout.portrait=false"` (lu
après le scénario, qui finit en paysage) :
`TAILLE 800x1280 OK`, `LU … OK`, `CAPTURES OK` ; décrire `paysage.png` et `portrait.png` (même point de page au
centre, trait rouge présent).

**Essai Android.** `scripts/essai-android-tablette.ps1 -Apk <…-x86_64.apk> [-Avd <nom>]` : crée au besoin un AVD de
tablette nommé `actcraft-tablette` (`avdmanager create avd -n actcraft-tablette -d pixel_tablet` ; plusieurs Go sous
`%USERPROFILE%\.android\avd`, création et taille dites au rapport) **sur l'image système déjà installée** (sans téléchargement ; si
aucune image ne convient, échec `ANDROID TABLETTE EN ECHEC : image système absente`, et l'orchestrateur demande
l'accord de Jérôme pour la télécharger), démarre sans fenêtre, installe et lance, trace au doigt, tourne l'écran
(`adb shell settings put system accelerometer_rotation 0` puis `user_rotation 1`), relève la capture (trait présent,
palette visible), ouvre l'outil texte et touche la page, vérifie `mInputShown=true` dans
`adb shell dumpsys input_method` et que le pixel du curseur est au-dessus du clavier (`adb shell dumpsys window
InputMethod` : rectangle du clavier). Dernière ligne : `ANDROID TABLETTE OK` (0) ou `ANDROID TABLETTE EN ECHEC :
<raison>` (1).

**Interdits.** Aucune orientation forcée au manifeste ; ce qui a été lancé est arrêté par les processus tenus, jamais
par nom.

**Terminé quand** les 5 fonctions, le scénario et `ANDROID TABLETTE OK` passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_orientation.cpp` : `shiftZeroWithoutKeyboard` ; `shiftKeepsTheCaretAbove`
  (clavier y = 500, curseur bas = 600 → `shift` = 116) ; `shiftResetsWhenHidden` ; `rotationKeepsTheStroke` (trait,
  1280 × 800 → 800 × 1280 → trait toujours dans la page, même nombre d'objets) ; `rotationDuringStrokeKeepsIt`.
- [ ] **Étape 2 : implémentation**, `PageArea.qml`.
- [ ] **Étape 3 : scénario** `rotation-clavier.json` : trait rouge ; `{"resize": [800, 1280]}` ;
  `{"expect": "appLayout.portrait", "value": true}` ; `checkPixel` du trait
  (coordonnées de page inchangées) ; capture `portrait` ; `{"resize": [1280, 800]}` ; capture `paysage`.
- [ ] **Étape 4 : script** `scripts/essai-android-tablette.ps1` (en-tête de commentaires sur le modèle de
  `scripts/essai-android-fichiers.ps1` ; appels par `scripts/android-commun.ps1`) ; APK par
  `pwsh -NoProfile -File ./scripts/paquet-android.ps1` (`APK OK`).
- [ ] **Étape 5 : vert**, preuve visuelle, essai Android, preuve de tâche ; commit « Lot 11 : rotation et clavier
  logiciel (L11-T11) ».

<a id="tache-l11-t12"></a>
### Tâche L11-T12 — Tablette III : menus et palette au doigt

**Objectif.** D11-7. `ui::FingerMenus : QObject`, `QML_ELEMENT` : écoute `PageView::touchHeld` ; avec le sélecteur
(outil courant ou imposé), ouvre `FingerMenu.qml` au point du doigt : menu de l'objet sous le doigt (le menu d'édition
d'objet du lot 5, A18), sinon le menu de la page (`PageContextMenu.qml`) ; avec un outil d'encre : rien.
`ToolPalette.qml` : appui long d'un doigt sur un bouton → bulle de libellé 1,5 s (`objectName: "paletteLabel"`), le bouton n'est pas
déclenché.
`CommandMenu.qml` : rangées de `appLayout.targetSize` de haut, sous-menus ouverts par un appui (pas au survol) quand
`touchUi`.

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_fingermenus|tst_captures_au_doigt'`.

**Preuve visuelle.** `captures.ps1 -Scenario au-doigt -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "fingerMenu.visible=false"` :
`APPUI LONG OK`, lectures du scénario `OK`, `CAPTURES OK` ; décrire `menu-objet.png` et `libelle.png`.

**Interdits.** Aucun menu ouvert par un appui long de stylet ou de souris (le clic droit existe) ; aucune commande
déclenchée par l'appui long d'un bouton.

**Terminé quand** les 6 fonctions et le scénario passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_fingermenus.cpp` : `longPressOnObjectOpensObjectMenu` ;
  `longPressOnEmptyPageOpensPageMenu` ; `noMenuWithInkTool` ; `paletteLongPressShowsLabelOnly` (compteur de
  déclenchements inchangé) ; `menuRowsFollowTargetSize` ; `submenuOpensOnTap`.
- [ ] **Étape 2 : implémentation** et QML.
- [ ] **Étape 3 : scénario** `au-doigt.json` : rectangle tracé ; `tool.select` ; `{"longPress": [700, 575], "kind":
  "touch", "holdMs": 600}` ; `expect fingerMenu.visible = true` ; capture `menu-objet` ; Échap ; appui long sur le
  bouton du stylo de la palette (fenêtre) ; `expect paletteLabel.visible = true` ; `expect appTools.currentTool = select` (outil courant inchangé ;
  forme de la valeur relue par L11-T0) ; capture `libelle`.
- [ ] **Étape 4 : vert**, preuve visuelle, banc sans écran, preuve de tâche ; commit « Lot 11 : menus et palette au
  doigt (L11-T12) ».

<a id="tache-l11-t13"></a>
### Tâche L11-T13 — Tablette IV : cycle de vie, reprise sans perte

**Objectif.** D11-8. `ui::LifecycleKeeper : QObject`, `QML_ELEMENT` : propriétés `document` (contrôleur de document,
A4), `view`, `recoveryDir` (`<AppDataLocation>/reprise` par défaut) ; `StartupOptions` gagne `QString recoveryDir` (vide =
défaut), posé en propriété initiale `recoveryDir` de `Main.qml` puis sur `appLifecycle` ; le joueur de captures vide
`<sortie>/reprise` **au lancement** (pas à `restart`) et le pose, comme les réglages d'essai ; installe un filtre
d'événements sur `qApp` et réagit à `QEvent::ApplicationStateChange`
(`static_cast<QApplicationStateChangeEvent*>(e)->applicationState()`, voie publique, A22) ; repli si L11-T0 montre que
la plateforme n'envoie pas cet événement à `qApp` : créneau public `Q_INVOKABLE void applyState(Qt::ApplicationState)`
relié à `QGuiApplication::applicationStateChanged` et appelé par le joueur sur `appLifecycle` ; à `Suspended` (et `Hidden` sous Android) : si le document est
modifié, `view->finishContacts()`, puis écriture de `reprise.actcraft` et `reprise.json` hors du fil principal (même
écriture atomique que l'enregistrement, DA3) ; au démarrage (`start()`, appelé par `componentComplete`) : une copie plus
récente que le dernier enregistrement du document d'origine est rouverte, `RecoveryBanner.qml` l'annonce (« Travail
repris après une interruption »), le document garde son URL d'origine et son état « modifié » ; `recovery.discard`
supprime la copie ; un enregistrement réussi supprime la copie.

**Sérialisation (patron à éviter).** Ne pas écrire un second format : la copie est un `.actcraft` écrit par
l'écrivain natif existant ; `reprise.json` ne porte que l'URL d'origine, la page, l'échelle, l'horodatage et l'empreinte
du modèle. Ne pas écrire sur le fil principal (Android tue le processus peu après `onStop`).

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_lifecycle|tst_captures_cycle_de_vie'`.

**Preuve visuelle.** `captures.ps1 -Scenario cycle-de-vie -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "recoveryBanner.visible=true"` :
`ETAT suspended OK`, `RELANCE`, `LU … OK`, `PIXEL … OK`, `CAPTURES OK` ; décrire `reprise.png`.

**Essai Android.** `scripts/essai-android-cycle.ps1 -Apk <…-x86_64.apk>` : installe, lance, trace un trait, relève
l'empreinte (`ACTCRAFT EMPREINTE <h>` écrit dans logcat par une option d'essai `--journal-empreinte`, ajoutée par cette tâche
(`app/main.cpp`, `app/startup.*`) si elle n'existe pas — sinon relire la ligne `ACTCRAFT ENREGISTRE` de L2-T22), `adb shell input keyevent HOME`, attend 2 s,
`adb shell am kill io.github.jejeasterix.actcraft` (processus en arrière-plan tué comme par le système), relance,
vérifie la ligne de reprise et la même empreinte, et que le trait est visible (capture, pixel). Dernière ligne :
`ANDROID CYCLE OK` ou `ANDROID CYCLE EN ECHEC : <raison>`.

**Interdits.** Aucun fichier de reprise hors du dossier privé de l'application ; aucune écriture au démarrage tant que
rien n'a changé ; aucun réseau.

**Terminé quand** les 7 fonctions, le scénario et `ANDROID CYCLE OK` passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_lifecycle.cpp` : `nothingWrittenWhenClean` ; `suspendWritesRecovery`
  (document modifié, état `Suspended` simulé par
  `QApplicationStateChangeEvent ev(Qt::ApplicationSuspended); QCoreApplication::sendEvent(qApp, &ev);` → fichiers présents, empreinte du `.actcraft` relu = empreinte du modèle) ;
  `writeIsOffTheMainThread` (identifiant du fil de l'écriture ≠ fil principal) ; `strokeInProgressIsKept` (contact en
  cours au moment de la suspension → trait dans la copie) ; `processKilledInBackgroundLosesNothing` (copie écrite ;
  nouvel objet sur le même dossier → document rouvert, même empreinte, URL d'origine, `modified` vrai, bannière) ;
  `saveRemovesRecovery` ; `discardRemovesRecovery`.
- [ ] **Étape 2 : implémentation**, QML.
- [ ] **Étape 3 : scénario** `cycle-de-vie.json` : trait rouge ; `{"appState": "suspended"}` ; `waitFor` sur
  `appLifecycle.lastWriteOk = true` ; `{"restart": true}` ; `waitReady` ; `expect recoveryBanner.visible = true` ;
  `checkPixel` du trait ; capture `reprise`. Dossier de reprise sous `<sortie>/reprise` (posé par le joueur comme les
  réglages d'essai).
- [ ] **Étape 4 : script** `scripts/essai-android-cycle.ps1` ; APK `APK OK`.
- [ ] **Étape 5 : vert**, preuve visuelle, essai Android, banc sans écran, preuve de tâche ; commit « Lot 11 : reprise
  sans perte après une interruption (L11-T13) ».

<a id="tache-l11-t14"></a>
### Tâche L11-T14 — Tablette V : portail de documents

**Objectif.** D11-9 ; décision 14 (finition des dialogues de fichiers Android). `ui::DocumentPortal : QObject`,
`QML_ELEMENT` : `Q_INVOKABLE bool keep(QUrl url)` (sous Android : `takePersistableUriPermission` par `QJniObject` avec
les droits de lecture et d'écriture ; ailleurs : vrai sans effet), `Q_INVOKABLE bool canOpen(QUrl url) const`
(permission encore valide ; fichier local : existe), `Q_INVOKABLE void forget(QUrl url)` ; `RecentDocuments` (A14)
appelle `keep` à l'ajout d'une URL `content://`, filtre par `canOpen` à la lecture et retire une entrée perdue avec un
message (`staleRemoved(QString name)`). Insérer une image et importer un PDF passent par le même sélecteur et la même
garde. Le correctif `L11-C0` (si L11-T0 en a proposé un) a déjà rendu les commandes de fichier présentes sous Android.

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_documentportal|tst_ui_recents'`.

**Essai Android.** `scripts/essai-android-portail.ps1 -Apk <…-x86_64.apk>` : comme `essai-android-fichiers.ps1`, enregistre
par le sélecteur, ferme, **relance l'application** (`am force-stop` puis lancement), rouvre le document par la liste des
documents récents (sans sélecteur), empreinte égale ; puis retire le document (`adb shell content delete --uri <uri>` ; `pm clear` de DocumentsUI ne
retire pas les permissions gardées par le système ; voie vérifiée par L11-T0) et vérifie l'entrée retirée et le message
(logcat `ACTCRAFT RECENT RETIRE`). Dernière ligne : `ANDROID PORTAIL OK` ou `ANDROID PORTAIL EN ECHEC : <raison>`.

**Interdits.** Aucune permission de stockage large au manifeste (décision 28) ; aucun chemin de fichier tiré d'une URL
`content://`.

**Terminé quand** les 6 fonctions et `ANDROID PORTAIL OK` passent.

**Étapes.**
- [ ] **Étape 0 : démon Gradle (amendement 1, M13).** Dans `scripts/paquet-android.ps1`, avant l'appel
  d'`androiddeployqt` : `$env:GRADLE_OPTS = '-Dorg.gradle.daemon=false'` (A24) ; preuve : `Measure-Command` de
  `pwsh -NoProfile -File ./scripts/paquet-android.ps1 > .tmp/apk.txt 2>&1`, la commande rend la main moins de 60 s
  après la ligne `APK OK` (horodatage de la ligne relevé par `(Get-Item .tmp/apk.txt).LastWriteTime`) ; aucun
  `java.exe` de ce worktree laissé par l'essai — seuls ceux dont la ligne de commande contient le worktree sont
  comptés, sortie vide attendue après la commande :
  `Get-CimInstance Win32_Process -Filter "Name='java.exe'" | Where-Object CommandLine -like '*<wt>*'`.
- [ ] **Étape 1 : test rouge** `tst_ui_documentportal.cpp` (bureau ; JNI remplacé par une interface
  `PermissionBackend` injectée, factice dans le test) : `contentUrlKeptOnAdd` ; `localFileNeedsNoPermission` ;
  `lostPermissionRemovesTheEntry` (signal émis avec le nom affiché, pas l'URL) ; `forgetReleases` ;
  `insertImageUsesThePortal` ; `noPathFromContentUrl` (aucun `toLocalFile()` non vide pour `content://`).
- [ ] **Étape 2 : implémentation** (`documentportal.cpp` ; partie Android sous `#ifdef Q_OS_ANDROID`) ;
  `recentdocuments.cpp`.
- [ ] **Étape 3 : script** `scripts/essai-android-portail.ps1` ; APK `APK OK` ; `ANDROID PORTAIL OK`.
- [ ] **Étape 4 : vert**, preuve de tâche ; commit « Lot 11 : portail de documents sous Android (L11-T14) ».

<a id="tache-l11-t15"></a>
### Tâche L11-T15 — Mesure sur la tablette

**Objectif.** D11-12 ; BRIEF § 3 (budget : 60 images/s, latence d'encre < 20 ms) ; demande « performance mesurée sur la
tablette » ; N1-9 (Android sans machine de référence). L'exécutable du banc (`bench/app`) est empaqueté en APK séparé ;
sous Android, son dossier de résultats par défaut est le dossier propre externe de l'application
(`/sdcard/Android/data/io.github.jejeasterix.actcraft.banc/files/bench-results`, lisible par `adb pull` sans `run-as`,
A21) ; l'APK de banc reste construit en release, **non débogable** (la mesure doit rester en release) ; ses options passent par `applicationArguments` (`adb shell am start … -e applicationArguments
"--scenario court"`, à revérifier sur Qt 6.12).

**Fichiers.** Ceux de la ligne de la table.

**Scripts.**
- `scripts/paquet-banc-android.ps1` → `build/paquet-banc/ActCraft-banc-<version>-arm64-v8a.apk` et `…-x86_64.apk`
  (clé de débogage ; démon Gradle coupé comme dans `paquet-android.ps1`, L11-T14) ; dernière ligne `APK BANC OK` ou `APK BANC EN ECHEC : <raison>`.
- `scripts/banc-android.ps1 -Apk <fichier> [-Serie <numéro adb>] [-Scenario court|complet] [-Sortie <dossier>]` :
  installe, lance, attend la fin, rapatrie le JSON (`adb pull /sdcard/Android/data/io.github.jejeasterix.actcraft.banc/files/bench-results <Sortie>`),
  écrit `MESURE INDICATIVE (émulateur)` si l'appareil est un émulateur (`ro.kernel.qemu=1`), puis la ligne de verdict du
  banc (`BUDGET TENU`, `BUDGET DEPASSE`, `MESURE INVALIDE`). Désinstalle l'APK de banc à la fin.

**Procédure pour Jérôme** (`docs/banc-tablette.md`, texte exact à livrer, sur le modèle de `docs/banc.md`) :
1. Sur la tablette : Paramètres ▸ À propos ▸ appuyer 7 fois sur « Numéro de build » ; Options pour les développeurs ▸
   activer « Débogage USB ».
2. Brancher la tablette au poste, accepter l'empreinte du poste sur la tablette.
3. Dans `pwsh -NoProfile`, à la racine du dépôt :
   ```powershell
   . ./scripts/dev-env-android.ps1
   adb devices
   ```
   noter le numéro de série de la ligne `device`.
4. ```powershell
   pwsh -NoProfile -File ./scripts/paquet-banc-android.ps1
   pwsh -NoProfile -File ./scripts/banc-android.ps1 -Apk build/paquet-banc/ActCraft-banc-<version>-arm64-v8a.apk -Serie <numéro> -Scenario complet -Sortie .tmp/banc-tablette
   ```
   Laisser la tablette posée, écran allumé, sans la toucher, pendant toute la mesure (≈ 3 min).
5. Envoyer à l'orchestrateur : la dernière ligne affichée, le fichier `.tmp/banc-tablette/*.json`, le modèle de la
   tablette (Paramètres ▸ À propos).
Option sans poste de développement : l'APK de banc remis par l'orchestrateur, installé à la main ; le JSON est alors
partagé depuis l'application (bouton « Partager le résultat » de l'écran de fin du banc, à ajouter **seulement** si
Jérôme le demande ; hors périmètre par défaut).

**Test et commande bornée.** Commande ciblée `-R tst_bench_resultsdir` ; « fonctions d'un test ».

**Interdits.** L'APK de banc n'entre ni dans `build/paquet/`, ni dans la release, ni dans la CI de release ; aucune
permission ajoutée ; aucune mesure de l'émulateur présentée comme une mesure de tablette.

**Terminé quand** les 4 fonctions passent, `APK BANC OK`, et `banc-android.ps1` sur l'émulateur rend
`MESURE INDICATIVE (émulateur)` puis un verdict (pas `MESURE INVALIDE`).

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_bench_resultsdir.cpp` : `desktopDefaultUnchanged` (`bench/results`) ;
  `androidDefaultIsExternalAppFiles` (fonction pure `resultsDirFor(Platform::Android, externalFilesPath)` →
  `<externalFilesPath>/bench-results`) ; `argumentsParsedFromSingleString`
  (`"--scenario court --taille 1280x800"` → options) ; `explicitOutputWins`.
- [ ] **Étape 2 : implémentation** (`bench/app/main.cpp`, `bench/app/CMakeLists.txt` : `qt_add_executable` déjà là,
  propriétés Android `QT_ANDROID_PACKAGE_SOURCE_DIR`, identifiant `.banc`), manifeste à nous sans permission.
- [ ] **Étape 3 : scripts** et `docs/banc-tablette.md` ; `APK BANC OK` ; essai sur l'émulateur.
- [ ] **Étape 4 : vert**, preuve de tâche ; contrôle des titres du corpus vide ; `NOMS OK` ; commit « Lot 11 : mesure
  du banc sur une tablette Android (L11-T15) ».

<a id="tache-l11-t16"></a>
### Tâche L11-T16 — Point d'étape 11a

**Objectif.** Contrôle de 11a avant 11b, sur le worktree du chantier, après le merge de L11-T2 à L11-T15.

**Fichiers.** `tests/tst_ui_shell.cpp` (fonction `lot11aStubsGone` : aucun talon de 11a ne reste — liste des fichiers
R2 et des tests de 11a, aucun marqueur ; le marqueur est construit dans le test, `QStringLiteral("Talon ") +
QStringLiteral("L11")`, pour que le test ne se compte pas lui-même), `docs/verifications/2026-10-21-lot11a-etape.md`, `graphify-out/`.

**Commandes.**
- Suites : `pwsh -NoProfile -File ./scripts/test.ps1 -Preset release` puis `pwsh -NoProfile -File ./scripts/test.ps1`
  (formes bornées, sous le jeton) : `TESTS OK` deux fois, N = N11 + 32, `SKIP` restants = talons de 11b seulement
  (`git -C <wt> grep -l "Talon L11" -- . ':!docs/' ':!graphify-out/' ':!tests/tst_ui_shell.cpp'` : seulement des
  fichiers de 11b : `ui/include/actcraft/ui/updatechecker.h`, `ui/src/updatechecker.cpp` et les tests de 11b).
- Captures de 11a, chacune dans **son** dossier : `modes-toucher`, `deux-utilisateurs`, `tablette-etroite`, `rotation-clavier`, `au-doigt`, `cycle-de-vie`, et `premiers-pas` (S2, non-régression) : `CAPTURES OK`.
- Paquets et Android : `paquet-android.ps1` (`APK OK`), `verifier-licences.ps1 -Apk …` (`LICENCES OK`),
  `essai-android.ps1` (S3, `ANDROID OK`), `essai-android-fichiers.ps1` (S5, `ANDROID FICHIERS OK`),
  `essai-android-tablette.ps1`, `essai-android-cycle.ps1`, `essai-android-portail.ps1` (`… OK`), tailles des deux APK
  rapportées (N1-A10).
- Banc sans écran ; `graphify update .` puis commit de `graphify-out/`.

**Interdits.** Aucune fenêtre visible (le passage à l'écran est en L11-T30) ; aucun push de tag, aucune release.

**Terminé quand** tout ce qui précède est vert et le rapport commité (tailles, JSON du banc, scénarios, essais).

**Étapes.**
- [ ] **Étape 1 :** `lot11aStubsGone` rouge si un talon reste, puis vert.
- [ ] **Étape 2 :** suites, captures, Android, banc.
- [ ] **Étape 3 :** rapport ; `graphify update .` ; `git add` des chemins nommés ; `NOMS OK` ; contrôle des titres du
  corpus vide ; commit « Lot 11 : point d'étape 11a (L11-T16) ».

## Scénarios et tâches (11a)

| Scénario | Tâches |
|---|---|
| S28 — Deux utilisateurs et toucher | L11-T4, L11-T5, L11-T6, L11-T7, L11-T8, L11-T9 (réservation et joueur : L11-T1, L11-T2, L11-T3) |
| S30 — Tâche d'ouverture | L11-T0 |
| S3.1 — Mise en page étroite (posé par ce plan) : fenêtre de 800 × 1280, navigateurs en tiroir, palette repliable, cibles ≥ 48 dp | L11-T10 |
| S3.2 — Rotation et clavier (posé par ce plan) : trait gardé et point de page au centre après rotation ; curseur au-dessus du clavier ; sur l'émulateur de tablette | L11-T11 (vue : L11-T5) |
| S3.3 — Au doigt (posé par ce plan) : appui long → menu de l'objet ; libellé d'un bouton sans le déclencher | L11-T12 |
| S3.4 — Cycle de vie (posé par ce plan) : processus tué en arrière-plan, relancé, travail retrouvé à l'identique | L11-T13 |
| S5.1 — Portail (posé par ce plan) : document rouvert par la liste des récents après relancement, permission perdue gérée | L11-T14 |
| S3.5 — Mesure sur la tablette (posé par ce plan) : APK de banc, verdict lu par adb ; procédure pour Jérôme | L11-T15 |
| S3, S5 (rejoués) | L11-T16 |

## Hypothèses pour Jérôme (posées à la porte de fin du lot)

### Niveau 1
- **H11-1 — Tablette réelle.** Mesure et essais sur une vraie tablette seulement si Jérôme en branche une (procédure
  de L11-T15) ; sinon émulateur, mesure indicative (N1-11, N1-9).
- **H11-2 — Schéma d'entrée du second utilisateur** (D11-2), s'il n'a pas été tranché par `L3-T0`.
- **H11-3 — Copie de reprise** dans le dossier privé de l'application, effacée à l'enregistrement ou par
  « Abandonner la copie de reprise ».

### Niveau 2
- **H11-4** Appui long comme menu au doigt (D11-7) : manière moderne, à la place du clic droit.
- **H11-5** Seuils de la mise en page étroite (960 et 600 dp) et cibles de 48 dp (D11-5, D11-6).
- **H11-6** Mode du toucher rangé dans les réglages du poste, pas dans le profil (D11-3).

## Relecture par l'auteur (writing-plans)

- Couverture : chaque point du § 2.3 de l'amendement et de la décision 14 a sa tâche (« Couverture » plus haut) ;
  S28 et S30 ont leurs tâches ; les points de 11b sont dans son plan.
- Aucune étape « à compléter » : les formes « à revérifier par L11-T0 » sont des API nommées dans la table A.
- Noms cohérents : `TouchMode`, `makeTouchPolicy`, `FingerTool`, `routePresses`, `TwoUserPolicy`, `Grabs`,
  `finishContacts`, `touchHeld`, `setRoutingPolicy`, `deviceId`, `appTouchMode`, `appDualMode`, `appLayout`,
  `appLifecycle`, `appPortal`, `appUpdates`, mêmes dans toutes les sections.
