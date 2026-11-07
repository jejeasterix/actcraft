# Lot 11 — ouverture (L11-T0) : principes, décisions et API revérifiées

- Tâche : L11-T0 du chantier `livraison-complete`, lot 11 (plans
  `docs/plans/2026-10-18-livraison-complete-lot11-PLAN.md` et `…-lot11b-PLAN.md`).
- Branche : `feat/livraison-complete-l11--L11-T0`, base `feat/livraison-complete-l11` à `c263919d` (lots 1 à 6, 8 et
  10 livrés ; **lot 7 absent**).
- Poste : Mac (Qt 6.12.0 Homebrew, `build-mac`, Debug, banc activé), 2026-11-07.
- Relevé brut hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot11.md` — **non écrit** (ancien dépôt
  absent du Mac) : reste du PC (dernière section).

## Méthode et limites

Le plan date du 2026-10-18, avant les lots 4 à 8. Le patron écarté est celui qui recopierait la table A du plan en
« conforme » : chaque ligne ci-dessous a été relue sur le code de la tête (`c263919d`), par le graphe du worktree puis
par les en-têtes, le QML, `actions/data/commands.json`, les profils et `tests/bench.cmake`. Un nom absent est dit
absent, avec la tâche qui doit le créer ; un nom qui existe déjà sous une autre forme est un écart, avec la section à
amender.

Joué sur le Mac : étapes 4 (API), 5 (N11), 6 (commandes de fichier sans `android`), 7 (ce rapport).

Non joué sur le Mac (décision de l'orchestrateur) : étapes 1 (corpus vidéo), 2 (spécification `fonctionnalites.md`),
3 (REA en statique sur le binaire), le relevé brut et la liste des noms à refuser. Chaque point « à élucider » prend
ici le défaut du plan, écrit comme hypothèse « à confronter au PC » (H11-12 à H11-17 ; H11-1 à H11-11 sont prises par
11a et 11b).

Conventions : « utilisateur 1 » et « utilisateur 2 » du plan sont les `input::UserId` 0 et 1
(`input/include/actcraft/input/routing.h:12-13`, `kFirstUser = 0`).

## Constats du corpus, de la spécification et de REA

Reportés au PC (étapes 1 à 3). Aucune ligne de constat n'est écrite ici : sans le corpus ni la spécification, une
ligne serait une invention. Les points « à élucider » du plan et le défaut qui s'applique en attendant :

| Point à élucider | Défaut appliqué (plan ou code) | Hypothèse |
|---|---|---|
| Contenu de la deuxième palette (F-303, N2-A7) | celui de L11-T9 ; palette au bord gauche (D11-2) | H11-12 |
| Objet tenu par un utilisateur | inaccessible à l'autre tant qu'il est tenu (L11-T8) | H11-13 |
| Outil par défaut du toucher (F-930) | mode `full` par défaut ; en `pen` ou `select`, outil imposé au doigt (D11-3, D11-4) | H11-14 |
| Mode d'interaction par défaut avec les objets (F-930) | en `full`, le doigt agit avec l'outil courant de son utilisateur (D11-3, D11-4) | H11-15 |
| Outil de départ d'un `UserState` neuf (amendement 2, n2) | `pen` : déjà le code (`tools/include/actcraft/tools/userstate.h:47`, `m_tool = "pen"` :62) | H11-16 |
| Envoi de l'état de l'application par la plateforme (A22) | API publique présente (table A) ; envoi sous Windows et Android supposé | H11-17 |

## Décisions D11-1 à D11-16

| Décision | Verdict | Motif ou amendement |
|---|---|---|
| D11-1 | confirmée | 31 tâches : L11-T0 à L11-T16 (17) dans 11a, L11-T17 à L11-T30 (14) dans 11b |
| D11-2 | confirmée (H11-2) | le socle porte déjà l'utilisateur : `Sample::user` (`sample.h:34`), `Decision::user` (`routing.h:33`), `ToolHost::user(UserId)` et `activeTool(UserId)` (`toolhost.h:143-144`) ; `Sample::deviceId` absent, ajouté par L11-T4 (M5) |
| D11-3 | confirmée | `DeviceKind::Touch` existe (`sample.h:12`) ; clé `workstation/touch-mode` absente (à créer par L11-T6) ; clés du poste existantes : `recent-documents`, `language`, `dashboard-at-start`, `current-profile` |
| D11-4 | confirmée | réglages par outil et par utilisateur déjà dans `UserState::settings(toolId)` (`userstate.h:49`) |
| D11-5 | confirmée | `Theme.qml:38` `property int tailleIcone: 24` (modifiable) ; clé `workstation/touch-ui` absente (L11-T10) |
| D11-6 | confirmée | `LayoutMetrics` absent (L11-T3, L11-T10) |
| D11-7 | confirmée | le joueur sait déjà tenir un appui (`pointer` + `holdMs`, `bench/include/actcraft/bench/uiscenario.h:62`) ; menu d'objet = `ui/qml/ObjectEditMenu.qml`, menu de page = `ui/qml/PageContextMenu.qml` |
| D11-8 | amendée (précision) | `PageView::finishContacts` absent (L11-T5) ; ne pas le confondre avec `PageView::endToolContacts(const input::Decision&)` (`pageview.h:269`), propre au routeur ; voir A22 pour la voie de l'événement d'état |
| D11-9 | amendée (précision) | `RecentDocuments` n'a que `note(url)` et `clear()` (`recentdocuments.h:56-57`) : retirer une entrée dont la permission est perdue demande une fonction nouvelle, à L11-T14 |
| D11-10 | confirmée | `android/AndroidManifest.xml:38` `screenOrientation="unspecified"` (le manifeste est sous `android/`, pas `app/android/`) |
| D11-11 | confirmée | aucune API de rectangle du curseur : `TextEditor.qml` emploie deux `TextEdit` (:106, :162), dont `cursorRectangle` (Qt) est la source (A19) |
| D11-12 | amendée (fichiers) | le banc est la cible `actcraft-bench` de `bench/CMakeLists.txt:63` : `bench/app/CMakeLists.txt` n'existe pas ; `defaultDir` = `applicationDirPath() + "/resultats-banc"` (`bench/app/main.cpp:62`), dont dérivent réglages, profils et ressources (:63-68) |
| D11-13 | confirmée | `help.check-updates` absent de `commands.json` (`help.*` : `website`, `about`, `contents`) ; `updates/check-at-start` absent ; `ui::NetworkSettings` existe (`networksettings.h:18`) |
| D11-14 | confirmée | option `--traduction` absente du joueur (`bench/captures/main.cpp`) : à créer par L11-T19 |
| D11-15 | confirmée | — |
| D11-16 | amendée (texte) | la tête est en `project(ActCraft VERSION 0.6.0)` (`CMakeLists.txt:3-4`), `CHANGELOG.md:7` a « 0.9.0 — non publiée » ; 0.10.0 n'est pas posée. La formule (`app/CMakeLists.txt:28-31`) et la conclusion tiennent : 10000 dépasse tout `versionCode` 0.x (≤ 9999) |

## Table A — API revérifiées sur le code réel (`c263919d`)

| # | Verdict | Signature ou état réel (fichier:ligne) | Écart et section à amender |
|---|---|---|---|
| A1 | conforme | `routing.h` : `UserId = std::uint8_t`, `kFirstUser = 0` (:12-13) ; `Route{Tool, Gesture, Ignore, Cover}` (:15-21) ; `RouterState{stylusDown, toolTouches, gestureTouches}` (:23-28) ; `Decision{route = Ignore, user = kFirstUser, endToolContacts}` (:30-37) ; `RoutingPolicy::decide(const Sample& press, const RouterState&) const = 0` (:44) ; `FullTouchPolicy final` (:50-53) ; `PointerRouter::setPolicy(std::unique_ptr<RoutingPolicy>)` (:60), `route(const Sample&)` (:66), `gestureTouches()` (:67), `reset()` (:68), `setCoverTest(std::function<bool(const Sample&)>)` (:70), `release(std::uint64_t pointer)` (:73) | aucun |
| A2 | écart | `sample.h:23-35` : `x, y, pressure, timestampNs, device, phase, seq, pointer, user` ; `DeviceKind{Stylus, Mouse, Other, Touch}` (:7-13, `Touch` en dernier) ; `Phase{Press, Move, Release}` | `Sample::user` existe déjà (rempli par le routeur) ; l'ordre de `DeviceKind` n'est pas celui du plan ; `deviceId` à ajouter à la fin par L11-T4 (L11-T4, L11-T5, L11-T7) |
| A3 | écart | `toolhost.h:143` `tools::Tool* activeTool(input::UserId user = kFirstUser) const` ; `:144` `tools::UserState& user(input::UserId id = kFirstUser)` ; `userstate.h:42-68` : `UserState(UserId)`, `id()`, `currentTool()`, `settings`, `selection`, `openMathTools()` (« réservé au lot 7 »), `setOpenMathTools(std::vector<QString>)`, `shapeSettings`, `textSettings` ; `ToolContext::user` (`toolcontext.h:125`) | `activeTool` prend déjà l'utilisateur ; sélection et réglages de forme et de texte sont déjà par utilisateur (L11-T7, L11-T8) |
| A4 | écart | `PageArea.qml:9` `objectName: "pageArea"`, `:61` `objectName: "pageView"` ; `pageview.h` : `zoom` (:53), `pan` (:54), `gestureActive` (:83), `pointerPosition` (:86), `contactActive` (:348, :351) ; pas de `scale` | `finishContacts`, `setRoutingPolicy`, `lastContactUser` absents : à créer par L11-T5 (conforme) ; échelle = `zoom` (M4, conforme) |
| A5 | écart | `commandregistry.h` : `struct CommandInfo{id, label, icon, shortcut, checkable, platforms = kAllPlatforms, argument, argumentOptional, lockable = true, restrictable = false}` (:44-60) ; `Platform{Windows=1, Android=2, MacOS=4, Linux=8}` ; `find(id) → const CommandInfo*`, `isAvailable(id, Platform)`, `setEnabled`, `isEnabled`, `isOwnEnabled`, `setLocked`, `isLocked`, `setRestricted`, `isRestricted` (lot 8) ; `commands.h` : `trigger(id, argument = QVariant())` (:70), `setEnabled`, `isEnabled` (:84-85), `setNativeHandler` (:74), `setLocked`, `isLocked`, `lockedCount` (:91-93, et propriété `lockedCount` :39), `setRestricted` (:101) | registre étendu par le lot 8 (restriction de diffusion) ; rien ne manque au plan |
| A5 bis | écart | `actions/data/commands.json` : racine `{format, commands}`, 257 commandes ; clés `id`, `label`, `lockable` et `restrictable` (sur les 257), `shortcut`, `icon`, `checkable`, `argument` (`url`, `text`, `color`, `number`), **`optional`** (et non `argumentOptional`), `platforms` (liste de `windows`, `android`, `macos`, `linux` ; absente = toutes, 18 commandes la portent) | toute commande nouvelle (L11-T1, L11-T6, L11-T9, L11-T22) écrit `optional` et les deux attributs de verrou |
| A6 | conforme | `preferences.h:31-32` : `Preferences(QObject*)` (QSettings au format par défaut), `Preferences(const QString& iniPath, QObject*)` (tests) | — |
| A7 | conforme | `ProfileController::activate(const QString&) → bool` (`profilecontroller.h:70`), `currentId()` (:94) ; `actions::ProfileStore` (`profilestore.h:14`) ; les cinq profils existent et portent une clé `pinned` | — |
| A8 | écart | `HelpViewer::openFor(QString toolId)` (`helpviewer.h:26`), `follow(QUrl)` (:28) ; base `qrc:/actcraft/help/` (`helpviewer.cpp:8`) ; `ui/help/` ne contient que `index.html` | toutes les pages par outil sont à créer (11b : L11-T17, L11-T18) ; `tst_ui_help` existe déjà à côté du futur `tst_ui_helppages` |
| A9 | conforme | `static bool Language::install(QCoreApplication&, Preferences&)` (`language.h:35`) ; `ui/i18n/actcraft_fr.ts` ; `qt_add_translations(actcraft_ui …)` (`ui/CMakeLists.txt:427`) | — |
| A10 | conforme | `ui::ClassroomLock` (`classroomlock.h:22`) ; `objectName: "appClassroom"` (`ClassroomLockDialog.qml:28`) ; scénario `classe-protegee` joué par `tst_captures_classe_protegee` | — |
| A11 | absent | lot 7 absent : `features/` n'est qu'un talon (`features.h`, `features.cpp`, 10 lignes chacun) ; `math.ruler`, `math.set-square`, `math.protractor`, `math.compass`, `math.xy-origin` et six `present.*` sont déclarées dans `commands.json` sans gestionnaire | L11-T8 se limite à l'état par utilisateur (`openMathTools`) ; le rattachement aux outils réels est « à raccorder à la fusion du lot 7 » |
| A12 | écart | `bench/include/actcraft/bench/uiscenario.h:33-73` (et non `bench/src/`) : `command`, `stroke`, `key`, `wait`, `waitReady`, `capture`, `checkPixel`, `expect` (`above`, `tolerance`), `fixture`, `waitFor`, `watchEventLoop`, `maxEventLoopGapMs`, `remember`/`as`, `archive`, `checkEdge`, `touch`, `pinch`, `pointer` (`button`, `modifiers`, `holdMs`, `space`), `doubleClick`, `typeText`, `tap`, `file`, `restart` ; `parseUiScenario(json, out, error)` (:128) | absents à la tête : `parallel`, `resize`, `appState`, `expectChanged`, champs `user` et `contacts` (L11-T2) ; `Injector` (`bench/include/actcraft/bench/injector.h:40`) sans `eventLog()` (L11-T2, m2) |
| A13 | écart | les lignes `tests/bench.cmake:107-112` du plan ont bougé : deux modèles à la tête — boucle du lot 6 (`:499-510`, `add_test` + `--rendu logiciel --taille 1280x800`, `SKIP_RETURN_CODE 77`) et `actcraft_add_lot8_capture(name scenario)` (`:519-538`), dont le talon se lit dans le scénario (« Talon L8 : rempli par L8-Tn. ») | L11-T1 : modèle du lot 8, mais la fonction ne reconnaît que « Talon L8 » ; une fonction du lot 11 (ou un paramètre de marqueur) est à poser dans `tests/bench.cmake` |
| A14 | écart | `RecentDocuments` : `entries` (`[{url, name, when}]`), `count`, `note(url)`, `clear()`, `summary(…)` (`recentdocuments.h:33-60`) | pas de retrait d'une entrée : à ajouter par L11-T14 (D11-9) |
| A15 | conforme | `tools::SelectTool final : public Tool` (`selecttool.h:18`) | — |
| A16 | conforme | `ui::NetworkSettings` (`networksettings.h:18`) | — |
| A17 | conforme | `ui/qml/PaletteFrame.qml`, `ui/qml/BrowserPanel.qml`, `Theme.qml:38` `tailleIcone: 24` | — |
| A18 | écart | menu de page `ui/qml/PageContextMenu.qml` ; menu d'objet `ui/qml/ObjectEditMenu.qml` ; listes `contextMenu` et `objectMenu` de `ui::Commands` (`commands.h:28-29`) | L11-T12 nomme `ObjectEditMenu.qml` |
| A19 | absent | aucune API de rectangle du curseur en coordonnées de page ; `TextEditor.qml:106`, `:162` (`TextEdit`) | L11-T11 lit `TextEdit.cursorRectangle` et le porte dans la vue |
| A20 | écart | `app/startup.h:10-23` : `StartupOptions{cacheEnabled, generatedPage, openUrl, profileRoot, showDashboard}` ; `parseStartupOptions`, `openFileArgument`, `loadMainWindow(QQmlApplicationEngine&, const StartupOptions&)` (:26-32) | `recoveryDir` à ajouter par L11-T13 (M9, n5) |
| A21 | écart | `bench/app/main.cpp:62-68` (`defaultDir`, `settingsDir`, `profilesDir`, `useIsolatedResourceRoots`) ; cible dans `bench/CMakeLists.txt:63` | sous Android, `applicationDirPath()` n'est pas inscriptible : L11-T15 remplace `defaultDir` par le dossier propre externe ; `bench/app/CMakeLists.txt` et `bench/app/android/` sont à créer ou à remplacer par `bench/CMakeLists.txt` (L11-T1, L11-T15) |
| A22 | API conforme ; envoi au PC | Qt 6.12.0 installé : `class Q_GUI_EXPORT QApplicationStateChangeEvent : public QEvent`, constructeur public `explicit QApplicationStateChangeEvent(Qt::ApplicationState)`, `applicationState()` (`QtGui.framework/Headers/qevent.h:1029-1039`) ; `QEvent::ApplicationStateChange = 214` (`QtCore.framework/Headers/qcoreevent.h:274`) ; `QGuiApplication::applicationState()` (`qguiapplication.h:127`) et signal `applicationStateChanged(Qt::ApplicationState)` (:154) | un événement posté par `sendEvent(qApp, &ev)` atteint les filtres posés sur `qApp` (règle de `QCoreApplication::notify`). **Piège pour L11-T13 (non vérifié sur les sources de Qt, absentes du Mac)** : l'événement synthétique ne change pas `QGuiApplication::applicationState()` et n'émet pas `applicationStateChanged` ; un code qui écoute le signal (ou `Qt.application.state` en QML) passerait sous la plateforme et resterait sourd au joueur. L11-T13 écoute donc l'événement (filtre sur `qApp`), ou passe par le repli `applyState` ; son test rouge le tranche. Envoi par la plateforme sous Windows et Android : H11-17 |
| A23 | conforme, déjà fait | `remember`/`as` (`uiscenario.cpp:311-314`, `uiscenario.h:54`) et substitution `${nom}` (`uiscenario.cpp:1302-1325`, mémoire vidée entre deux scénarios :1742) | L11-T2 n'ajoute que `expectChanged` (M4) |
| A24 | absent | aucun script `scripts/*.ps1` ne pose `GRADLE_OPTS` ni `org.gradle.daemon` | L11-T14 et L11-T15 l'ajoutent (M13) |

Autres relevés de l'étape 4 :

- `shape.choose` : argument `text` (`commands.json`) ; forme `"rectangle"` (`core/src/shapegeometry.cpp:54`) ;
  `ToolHost::chooseShape(preset)` active aussi l'outil `shape` (`toolhost.h:159`). Conforme à m3.
- `appTools.currentTool` : propriété `QString` (`toolhost.h:49`), valeur = identifiant d'outil sans préfixe (`"pen"`
  au départ, `userstate.h:47`) ; la commande correspondante est `tool.pen`. `objectName: "appTools"` (`Main.qml:142`).
- Rapport des épinglages du lot 7 (L11-T27) : absent. Liste lue dans les profils (clé `pinned`) : toutes les
  commandes épinglées sont déclarées ; celles du lot 7 (`math.*`, `present.curtain`, `present.spotlight-circle`,
  `tool.dice`, `tool.calculator`, `tool.clock`, `tool.ticker`) n'ont pas de gestionnaire.
- Voie de retrait d'un document pour `essai-android-portail.ps1` (`adb shell content delete --uri <uri>`) : non
  vérifiable sans SDK Android (reste PC).

## Écarts au plan par tâche (noms réels)

| Tâche | Écart |
|---|---|
| L11-T1 | `bench/app/CMakeLists.txt` n'existe pas (banc : `bench/CMakeLists.txt:63`) ; `actcraft_add_lot8_capture` ne lit que « Talon L8 » (A13) ; icônes `ui/icons/{toucher,deux-utilisateurs,mises-a-jour,reprise}.svg` à créer ; aucun des 22 noms de tests nouveaux du lot ne recoupe un test enregistré ; `help.check-updates` absent si L11-T9 doit le nommer avant L11-T22 |
| L11-T2 | `remember` et `${nom}` existent (A23) ; en-têtes sous `bench/include/actcraft/bench/` ; `pointer.holdMs` existe déjà |
| L11-T3 | `tst_ui_shell` existe ; `appLayout`, `appUpdates`, `paletteLabel` absents |
| L11-T4 | `Sample::user` déjà là ; `DeviceKind` dans l'ordre `Stylus, Mouse, Other, Touch` ; `deviceId` à ajouter en fin de structure |
| L11-T5 | `endToolContacts(Decision)` privé existe ; `finishContacts`, `setRoutingPolicy`, `lastContactUser` à créer |
| L11-T7 | `UserState(UserId)`, `id()`, outil de départ `pen` existent ; `ToolHost::user(id)` ne tient qu'un état (D-S10) |
| L11-T8 | lot 7 absent : seul l'état par utilisateur (`openMathTools`) se teste ; outils de maths « à raccorder à la fusion du lot 7 » |
| L11-T9 | **`appTools.activate(<outil>, user)` (n1) entre en collision avec `ToolHost::activate(const QString& toolId, const QVariant& argument)` (`toolhost.h:112`)** : en QML, `activate("pen", 1)` prendrait `1` pour l'argument de l'outil. Nom ou forme à amender (par exemple `activateFor(toolId, user)`) ; `setColor(const QColor&)` n'a qu'un argument (`toolhost.h:115`) |
| L11-T11 | pas d'API de curseur (A19) |
| L11-T12 | menu d'objet = `ObjectEditMenu.qml` (A18) |
| L11-T13 | `StartupOptions` sans `recoveryDir` ; piège de A22 (écouter l'événement, pas le signal) |
| L11-T14 | `RecentDocuments` sans retrait d'entrée (A14) ; `GRADLE_OPTS` absent (A24) |
| L11-T15 | banc dans `bench/CMakeLists.txt` ; `defaultDir` lié à `applicationDirPath()` (A21) |
| L11-T17, L11-T18 | `ui/help/` = `index.html` seul ; `tst_ui_help` existe déjà |
| L11-T19 | `--traduction` absent du joueur |
| L11-T21, L11-T22 | `help.check-updates` et `updates/check-at-start` absents ; `NetworkSettings` existe |
| L11-T23 | `tst_ui_shortcutbar` existe, distinct de `tst_ui_shortcuts` |
| L11-T25 | `THIRD_PARTY_NOTICES.md` et `packaging/` existent ; `packaging/autres-autorises.txt` absent |
| L11-T26 | `tst_actions_lockable` existe ; `lockable` et `restrictable` écrits sur les 257 commandes |
| L11-T27 | épinglages lus dans `pinned` ; commandes du lot 7 déclarées sans gestionnaire (bout en bout impossible pour elles avant la fusion du lot 7) |
| L11-T28 | base de version 0.6.0 et section « 0.9.0 — non publiée » (D11-16) ; `tst_app_version` existe (B1) |

## N11

`QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -N` à `c263919d` : **`Total Tests: 246`** (Mac, Debug, banc
activé). N = N11 + 32 = 278 sur ce poste à partir de L11-T1. À recompter au PC (`ctest --preset debug -N`) : les
modules optionnels peuvent y changer le compte.

## Étape 6 — commandes de fichier sans Android

Le repli de la décision 17 (`docs/plans/2026-10-15-livraison-complete-lot2b-PLAN.md:5784-5787`) n'a pas été appliqué :
`file.open`, `file.save` et `file.save-as` n'ont pas de clé `platforms` (toutes plateformes). La seule commande `file.*`
sans `android` est `file.import-powerpoint-images` (`["windows", "macos", "linux"]`, posée par le lot 5), hors du repli.

**Correctif `L11-C0` : aucun.**

## Hypothèses pour Jérôme (suite de H11-1 à H11-11)

Toutes « à confronter au PC » (corpus, spécification, REA) :

- **H11-12** — Contenu de la deuxième palette : celui de L11-T9, au bord gauche (F-303, N2-A7).
- **H11-13** — Un objet tenu par un utilisateur est inaccessible à l'autre jusqu'au relâchement (L11-T8).
- **H11-14** — Mode du toucher `full` par défaut ; outil imposé au doigt en `pen` et `select` (F-930, D11-3, D11-4).
- **H11-15** — En mode `full`, le doigt agit avec l'outil courant de son utilisateur (F-930).
- **H11-16** — Un utilisateur neuf commence au stylo (`pen`), comme le code actuel.
- **H11-17** — Windows et Android envoient l'état de l'application à `qApp` ; sinon, repli `applyState` (L11-T13).

## Restes pour le PC (L11-T0-PC)

1. Étape 1 : corpus vidéo (`docs/videos-tanguay/README.md`, sections « Principes absents » et « Écarts avec la
   spec », fiches des fonctions du lot) ; une ligne par principe dans le relevé brut, puis la table des constats de
   ce rapport.
2. Étape 2 : `fonctionnalites.md`, F-110, F-303 (dont `:385`), F-930 (toucher, langue, mises à jour), F-760, F-950 à
   F-953 ; requête du graphe de l'ancien dépôt « dual user second palette touch mode ».
3. Étape 3 : REA en statique sur le binaire (boîtes de dialogue, défauts du toucher et du mode à deux utilisateurs,
   contenu de la deuxième palette) ; trancher H11-12 à H11-16.
4. Relevé brut `docs\re\lots\lot11.md` et liste `docs\re\lots\lot11-noms.txt` (noms de `lot6-noms-profils.txt` et
   des outils), hors dépôt.
5. N11 au PC ; A22 sous Windows et sur l'émulateur Android (H11-17) ; voie `adb shell content delete --uri`.
6. `NOMS OK` et contrôle des titres du corpus sur ce rapport.
