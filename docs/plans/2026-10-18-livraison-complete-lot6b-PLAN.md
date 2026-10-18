# Livraison complète — lot 6 « Navigateurs, propriétés, profils », sous-lot 6b — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**But.** Fin du lot 6 et version **0.7.0** : navigateur de ressources, mode conception (mode Création de F-560), tableau
d'accueil, documents récents et résumé, éditeur de profils (choix du profil, onglets format, commandes, paramètres),
trousseau et réseau, raccourcis vers des programmes, mode classe protégé, orthographe, lecteur d'aide, langue, contrôle
du lot. Le sous-lot 6a (`docs/plans/2026-10-18-livraison-complete-lot6-PLAN.md`) est fait : ses décisions D6-1 à D6-14,
ses hypothèses H6-1 à H6-8, sa table A1 à A16 et sa règle des talons valent ici ; les fichiers et les 43 tests de 6b sont
déjà réservés par L6-T1, les emplacements QML posés par L6-T2. Exigences de 6b : F-090, F-123, F-142, F-297, F-298,
F-560, F-760 (lecteur), F-772, F-810 à F-813, F-882, F-900, F-910, F-920, F-930 (sections des fonctions incluses,
langue, réseau), F-935. Scénarios : S9.6, S9.7, S9.8, S9.9, S9.11, S9.12 (sous-scénarios de S9 posés par 6a), **S19**,
**S20**, **S21**, S9 rejoué à l'écran en fin de lot.

**API supposées propres à 6b** (en plus de A1 à A16 ; L6-T0 les revérifie) :

| # | API | Lot qui la pose | Employée par | À revérifier |
|---|---|---|---|---|
| A17 | caméra du lot 7 : liste des destinations d'une capture (page actuelle, nouvelle page, presse-papiers) et fichier QML ou contrôleur qui la porte (H7-3 : « ressources » au lot 6) | lot 7 | L6-T21 | oui |
| A18 | effets de changement de page du lot 7 : propriété ou réglage qui les suspend (`presentation/transition`) | lot 7 | L6-T22 | oui |
| A19 | réglages du lot 7 gardés par `QSettings` : `tools/ticker`, `tools/calculator`, `tools/webHome`, `presentation/transition`, réglages des outils de maths | lot 7 | L6-T27 | oui |
| A20 | peintre partagé : options de rendu de `paintPage` (objets masqués) et `render/include/actcraft/render/pagepainter.h` | lots 2, L0-S3 | L6-T22 | oui |
| A21 | `ui::ToolHost` : règle « objet verrouillé non déplaçable » de l'outil de sélection | lot 3 | L6-T22 | oui |

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L6-T19 | `formats` : fragment de ressource (objets d'une page et leurs ressources, en `.actcraft` d'une page) | L6-T18 | `formats/include/actcraft/formats/fragment.h`, `formats/src/fragment.cpp`, `tests/tst_formats_fragment.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_formats_fragment` Passed (dont `objectsAndResourcesRoundTrip`, `onlyReferencedResourcesAreWritten`, `insertGivesFreshIdentifiers`, `unknownObjectKept`, `notAFragmentRefused`) | S9.6 | #tache-l6-t19 |
| L6-T20 | `ui` : `ResourceLibrary` (trois racines, dossiers, éléments, vignettes et recherche hors du fil principal, renommer, supprimer, ajouter) | L6-T19 | `ui/include/actcraft/ui/resourcelibrary.h`, `ui/src/resourcelibrary.cpp`, `tests/tst_ui_resources.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_ui_resources` Passed (dont `threeRoots`, `foldersListed`, `itemsOfAFolder`, `thumbnailsOffTheMainThread`, `searchByNameAndKeywords`, `searchIsCancellable`, `renameAndDelete`, `addSelectionCreatesTheNextName`, `insertImageAndFragment`) ; banc hors écran joint | S9.6 | #tache-l6-t20 |
| L6-T21 | QML : navigateur de ressources, insertion (glisser, double-clic, tampon), ajout à la bibliothèque, destinations de la caméra ; scénario `ressources` | L6-T20 | `ui/qml/ResourceBrowser.qml`, `bench/scenarios/ressources.json`, fichier QML de la caméra du lot 7 (A17 : chemin relevé par L6-T0, écrit dans cette ligne par l'amendement du plan) | isolé | implementeur-tdd | `TESTS OK`, `tst_captures_ressources` Passed ; `captures.ps1 -Scenario ressources` : `LU appResources.itemCount=1 OK`, `LU appDocument.shownObjectCount=4 OK` (une copie + trois tampons), `CAPTURES OK` ; banc hors écran joint | S9.6 | #tache-l6-t21 |
| L6-T22 | Mode conception (F-560) : bascule F2, neuf réglages, effets sur les lots livrés | L6-T18 | `ui/include/actcraft/ui/designmode.h`, `ui/src/designmode.cpp`, `tests/tst_ui_designmode.cpp`, `ui/src/toolhost.cpp`, `render/include/actcraft/render/pagepainter.h`, `render/src/pagepainter.cpp`, `ui/qml/Main.qml`, `bench/scenarios/mode-creation.json` | isolé | concepteur-critique | `TESTS OK`, `tst_ui_designmode` Passed (dont `toggleIsCheckable`, `nineSettingsWithDefaults`, `lockedObjectMovesWhileDesigning`, `hiddenObjectsDrawnTranslucent`, `transitionsSuspended`, `settingsPublishedForLot8`), `tst_render_painter` Passed ; `captures.ps1 -Scenario mode-creation` : `LU appDesign.active=true OK`, `PIXEL masque-visible OK`, `CAPTURES OK` ; banc hors écran joint | S9.8 | #tache-l6-t22 |
| L6-T23 | Tableau d'accueil (F-090) : créer, ouvrir, importer un PDF, documents récents, annoter le bureau, réglages, aide ; au démarrage (désactivable), F11 | L6-T24, L6-T25 | `ui/qml/Dashboard.qml`, `app/main.cpp`, `bench/scenarios/tableau-de-bord.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_captures_tableau_de_bord` Passed ; `captures.ps1 -Scenario tableau-de-bord` : `LU dashboard.visible=true OK`, `LU dashboard.recentCount=2 OK`, `LU appDocument.pageCount=3 OK` après l'ouverture du premier, `CAPTURES OK` | S9.9 | #tache-l6-t23 |
| L6-T24 | Documents récents (liste du poste, menu Fichier) et résumé à l'enregistrement | L6-T18 | `ui/include/actcraft/ui/recentdocuments.h`, `ui/src/recentdocuments.cpp`, `tests/tst_ui_recents.cpp`, `ui/qml/SaveSummaryDialog.qml`, `ui/qml/FileActions.qml`, `ui/qml/CommandMenu.qml` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_recents` Passed (dont `openedFileGoesFirst`, `boundedToTen`, `missingFilesHidden`, `contentUrlsKept`, `clearEmptiesTheList`, `summaryCountsPagesAndObjects`) | S9.9 | #tache-l6-t24 |
| L6-T25 | Éditeur de profils : choix du profil (F-123), fenêtre (F-882 : liste, importer, enregistrer, enregistrer sous, supprimer, réinitialiser), onglet format (F-900) | L6-T18 | `ui/qml/ProfileChooser.qml`, `ui/qml/ProfileEditor.qml`, `ui/qml/FormatPage.qml`, `tests/tst_ui_profileeditor.cpp`, `bench/scenarios/profils.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_profileeditor` (dont `chooserActivatesAndCloses`, `saveAsCreatesAUserProfile`, `deleteRefusedForBuiltin`, `resetRestoresTheBuiltin`, `formatPageEditsTheProfile`), `tst_captures_profils` Passed ; `captures.ps1 -Scenario profils` : `LU appProfile.currentId=mon-profil OK`, `CAPTURES OK` | S9.7 | #tache-l6-t25 |
| L6-T26 | Onglet commandes (F-910) : toutes les commandes, cibles (palette, barre d'objet, menus), ajouter, retirer, monter, descendre, séparateur ; `layout.json` du profil | L6-T25 | `ui/include/actcraft/ui/layouteditor.h`, `ui/src/layouteditor.cpp`, `tests/tst_ui_layouteditor.cpp`, `ui/qml/CommandsPage.qml` | isolé | concepteur-critique | `TESTS OK`, `tst_ui_layouteditor` Passed (dont `targetsListed`, `addAppendsToTheTarget`, `moveUpAndDown`, `separatorAllowed`, `writesTheProfileLayoutOnly`, `defaultLayoutUntouched`, `reloadShowsTheChange`, `unavailableCommandsHidden`) | S9.7 | #tache-l6-t26 |
| L6-T27 | Onglet paramètres (F-930, F-935) : sections des fonctions incluses, valeurs par défaut, magasin de chaque réglage (profil ou poste), P2 et P10 du lot 10, E4 | L6-T25 | `ui/include/actcraft/ui/settingsschema.h`, `ui/src/settingsschema.cpp`, `tests/tst_ui_settingsschema.cpp`, `ui/qml/SettingsPage.qml` | isolé | concepteur-critique | `TESTS OK`, `tst_ui_settingsschema` Passed (dont `sectionsOfIncludedFunctionsOnly`, `noVoteSection`, `defaultsFromTheSpecification`, `lot7KeysReused`, `profileOrWorkstationStore`, `renderQualityThreeSettings`, `autoGroupDefaultsOff`, `defaultPageSize`) | S9.7, S9.12 | #tache-l6-t27 |
| L6-T28 | Trousseau de Windows (`CredWriteW`/`CredReadW`) et réglages réseau (mandataire, port, compte) | L6-T18 | `ui/include/actcraft/ui/secretstore.h`, `ui/src/secretstore_win.cpp`, `ui/src/secretstore_other.cpp`, `ui/include/actcraft/ui/networksettings.h`, `ui/src/networksettings.cpp`, `tests/tst_ui_network.cpp`, `ui/qml/NetworkPage.qml` | isolé | concepteur-critique | `TESTS OK`, `tst_ui_network` Passed (dont `secretRoundTrip`, `secretAbsentIsEmpty`, `proxyApplied`, `passwordNeverInSettings`, `noProxyByDefault`, `unavailableOffWindows`) | S9.12 | #tache-l6-t28 |
| L6-T29 | Raccourcis vers des programmes : modèle dans le profil, signature HMAC par clé du poste, lancement, confirmation d'un raccourci étranger ; programme d'essai | L6-T28 | `ui/include/actcraft/ui/launchers.h`, `ui/src/launchers.cpp`, `tests/launcher_probe.cpp`, `tests/tst_ui_launchers.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_ui_launchers` Passed (dont `addShowsThePath`, `runStartsTheProgram`, `signedOnThisWorkstation`, `foreignSignatureAsksFirst`, `refusedForeignRunsNothing`, `acceptedForeignIsResigned`, `removeDeletes`, `tamperedArgumentsAskAgain`) | S20 | #tache-l6-t29 |
| L6-T30 | Raccourcis vers des programmes : boutons de la barre de raccourcis, onglet d'édition (F-920, F-142, E3) ; scénario S20 | L6-T29 | `ui/qml/LaunchersPage.qml`, `ui/qml/ShortcutBar.qml`, `bench/scenarios/raccourcis-programmes.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_captures_raccourcis_programmes` Passed ; `captures.ps1 -Scenario raccourcis-programmes` : `LU launcherButton0.toolTip=Programme d'essai OK`, fichier témoin présent après le clic, absent après le refus, `CAPTURES OK` | S20 | #tache-l6-t30 |
| L6-T31 | Mode classe protégé (F-298) : mot de passe créé au premier verrouillage, PBKDF2-SHA256 avec sel, verrou du registre, état gardé ; scénario S19 | L6-T18 | `ui/include/actcraft/ui/classroomlock.h`, `ui/src/classroomlock.cpp`, `ui/include/actcraft/ui/commands.h`, `ui/src/commands.cpp`, `tests/tst_ui_classroomlock.cpp`, `ui/qml/ClassroomLockDialog.qml`, `bench/scenarios/classe-protegee.json`, `packaging/qt-autorises.txt` | isolé | concepteur-critique | `TESTS OK`, `tst_ui_classroomlock` Passed (dont `firstLockCreatesThePassword`, `everyLockableGreyed`, `wrongPasswordKeepsTheLock`, `rightPasswordUnlocks`, `lockSurvivesRestart`, `passwordNeverStoredInClear`, `saltDiffersPerPassword`, `qmlCannotBypass`) ; `captures.ps1 -Scenario classe-protegee` : `LU appClassroom.locked=true OK`, `LU appCommands.lockedCount>0 OK`, `CAPTURES OK` | S19 | #tache-l6-t31 |
| L6-T32 | Orthographe I : Hunspell 1.7.2 et dictionnaire français dans `third_party/`, licences, construction Windows et Android | L6-T18, **accord de Jérôme (H6-6)** | `third_party/hunspell/` (sources de la version), `third_party/hunspell/CMakeLists.txt`, `third_party/dictionaries/fr/{fr.aff,fr.dic,LICENSE.txt}`, `CMakeLists.txt`, `core/CMakeLists.txt`, `ui/CMakeLists.txt`, `THIRD_PARTY_NOTICES.md` | isolé | implementeur-tdd | `TESTS OK` ; `tst_core_spellcheck` compile et lie Hunspell (talon toujours `SKIP`) ; construction `android-x86_64` `BUILD OK` (ou CI de la branche d'essai) ; `NOMS OK` (`third_party/` exclu par le contrôle) | S21 | #tache-l6-t32 |
| L6-T33 | Orthographe II : vérificateur dans `core` (parcours des pages, mots des textes, règles d'ignorance, dictionnaire de l'utilisateur) | L6-T32 | `core/include/actcraft/core/spellcheck.h`, `core/src/spellcheck.cpp`, `core/src/hunspellengine.cpp`, `tests/tst_core_spellcheck.cpp` | isolé | implementeur-tdd | `TESTS OK`, `tst_core_spellcheck` Passed (dont `findsTheMisspelledWord`, `suggestsTheCorrection`, `walksPagesFromTheFirst`, `ignoreAlwaysSkipsTheWord`, `userDictionaryKept`, `repeatedWordsFlagged`, `replaceIsUndoneAtOnce`, `htmlTagsAreNotWords`) | S21 | #tache-l6-t33 |
| L6-T34 | Orthographe III : boîte de vérification, F7, page affichée pendant le parcours ; scénario S21 | L6-T33 | `ui/include/actcraft/ui/spelling.h`, `ui/src/spelling.cpp`, `tests/tst_ui_spelling.cpp`, `ui/qml/SpellingDialog.qml`, `bench/scenarios/orthographe.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_spelling` (dont `showsThePageOfTheWord`, `endMessage`), `tst_captures_orthographe` Passed ; `captures.ps1 -Scenario orthographe` : `LU spellingDialog.word=mosieur OK`, `LU spellingDialog.suggestion0=monsieur OK`, `LU appSpelling.finished=true OK`, `CAPTURES OK` | S21 | #tache-l6-t34 |
| L6-T35 | Lecteur d'aide hors ligne : F1 sur la page de l'outil courant, index, liens internes | L6-T18 | `ui/include/actcraft/ui/helpviewer.h`, `ui/src/helpviewer.cpp`, `ui/help/index.html`, `tests/tst_ui_help.cpp`, `ui/qml/HelpViewer.qml` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_help` Passed (dont `indexLoads`, `currentToolPageOrIndex`, `noNetworkUrl`, `internalLinkFollows`, `textsAreOurs`) | S9.11 | #tache-l6-t35 |
| L6-T36 | Langue : traductions préparées (`qt_add_translations`), réglage de la langue, appliqué au redémarrage ; scénario `aide-langue` | L6-T35 | `ui/include/actcraft/ui/language.h`, `ui/src/language.cpp`, `ui/i18n/actcraft_fr.ts`, `ui/CMakeLists.txt`, `app/main.cpp`, `tests/tst_ui_language.cpp`, `bench/scenarios/aide-langue.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_language` (dont `systemLanguageByDefault`, `unknownFallsBackToFrench`, `settingWritten`, `restartAnnounced`, `translatorInstalledAtStart`), `tst_captures_aide_langue` Passed ; `captures.ps1 -Scenario aide-langue` : `LU helpViewer.visible=true OK`, `LU appPreferences.language=fr OK`, `CAPTURES OK` | S9.11 | #tache-l6-t36 |
| L6-T37 | Contrôle du lot : talons, suites debug et release, captures et banc à l'écran (jeton `ecran`), paquets, version 0.7.0, `CHANGELOG.md`, graphe | L6-T19 à L6-T36 | `CMakeLists.txt`, `CHANGELOG.md`, `docs/mesures/2026-10-20-lot6-fin.md`, `graphify-out/` | partagé | implementeur-tdd | debug et release `TESTS OK`, N = N6 + 43 ; aucun talon « Talon L6 » hors `docs/` ; captures `-Scenario tous` hors écran puis à l'écran D3D11 `CAPTURES OK` ; banc complet D3D11 plein écran (jeton `ecran`) sans régression des critères de la référence ; `PAQUET OK`, `APK OK` ; `NOMS OK` ; contrôle des titres du corpus vide ; `graphify update .` commité | S9, S19, S20, S21 | #tache-l6-t37 |

19 tâches dans ce sous-lot (38 pour le lot). Aucune tâche n'enregistre de test : L6-T1 l'a fait (N = N6 + 43).
L6-T32 est la seule tâche qui touche des fichiers CMake (bibliothèque tierce) ; L6-T36 ajoute à `ui/CMakeLists.txt`
le seul appel `qt_add_translations`. Worktrees : `.worktrees/livraison-complete--L6-T<n>`, isolés sauf L6-T37.

**Écran.** Aucune tâche avant L6-T37 n'ouvre de fenêtre visible. L6-T30 lance un programme d'essai sans fenêtre
(`launcher_probe` écrit un fichier et sort) ; L6-T37 joue les captures et le banc à l'écran sous le jeton `ecran`,
annoncés à Jérôme d'une ligne.

## Vagues (largeur 3 au plus ; 2 places de tests)

V9 : L6-T19 ‖ L6-T22 ‖ L6-T24 · V10 : L6-T20 ‖ L6-T25 ‖ L6-T28 · V11 : L6-T21 ‖ L6-T26 ‖ L6-T29 ·
V12 : L6-T23 ‖ L6-T27 ‖ L6-T32 · V13 : L6-T30 ‖ L6-T31 ‖ L6-T33 · V14 : L6-T34 ‖ L6-T35 ‖ L6-T36 · V15 : L6-T37.

Vérification des recoupements :
- V9 : L6-T19 (`fragment.*`, un test) / L6-T22 (`designmode.*`, `toolhost.cpp`, `pagepainter.*`, `Main.qml`, un test,
  un scénario) / L6-T24 (`recentdocuments.*`, `SaveSummaryDialog.qml`, `FileActions.qml`, `CommandMenu.qml`, un test)
  — disjoints.
- V10 : L6-T20 (`resourcelibrary.*`, un test) / L6-T25 (`ProfileChooser.qml`, `ProfileEditor.qml`, `FormatPage.qml`,
  un test, `profils.json`) / L6-T28 (`secretstore*`, `networksettings.*`, `NetworkPage.qml`, un test) — disjoints.
- V11 : L6-T21 (`ResourceBrowser.qml`, `ressources.json`, fichier de la caméra du lot 7) / L6-T26 (`layouteditor.*`,
  `CommandsPage.qml`, un test) / L6-T29 (`launchers.*`, `launcher_probe.cpp`, un test) — disjoints.
- V12 : L6-T23 (`Dashboard.qml`, `app/main.cpp`, un scénario) / L6-T27 (`settingsschema.*`, `SettingsPage.qml`, un
  test) / L6-T32 (`third_party/`, trois `CMakeLists.txt`, `THIRD_PARTY_NOTICES.md`) — disjoints.
- V13 : L6-T30 (`LaunchersPage.qml`, `ShortcutBar.qml`, un scénario) / L6-T31 (`classroomlock.*`, `commands.*`,
  `ClassroomLockDialog.qml`, un test, un scénario, `packaging/qt-autorises.txt`) / L6-T33 (`spellcheck.*`,
  `hunspellengine.cpp`, un test) — disjoints.
- V14 : L6-T34 (`spelling.*`, `SpellingDialog.qml`, un test, un scénario) / L6-T35 (`helpviewer.*`, `ui/help/index.html`,
  `HelpViewer.qml`, un test) / L6-T36 (`language.*`, `actcraft_fr.ts`, `ui/CMakeLists.txt`, `app/main.cpp`, un test, un
  scénario) — disjoints.
- Repris d'une vague à l'autre : `app/main.cpp` (L6-T5 en 6a → L6-T23 → L6-T36) ; `ShortcutBar.qml` (L6-T9 → L6-T30) ;
  `profils.json` (L6-T6 → L6-T25) ; `ui/CMakeLists.txt` (L6-T1 → L6-T32 → L6-T36) ; `Main.qml` (L6-T2 → L6-T22).

**Accord H6-6.** Si Jérôme n'a pas répondu quand V12 démarre, L6-T32 sort de V12, L6-T33 de V13 et L6-T34 de V14 ; ces
trois tâches attendent l'accord, la clôture L6-T37 aussi (S21 est un scénario du lot). Les autres vagues continuent.

(Recopiées du plan 6a pour que chaque section se suffise avec elles.)

## Commandes de preuve communes

- `<wt>` = `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L6-T<n>` ; `<id>` = `L6-T<n>`.
- **Commande ciblée** (rouge, vert d'un seul test) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande '. ./scripts/dev-env.ps1; cmake --build --preset debug *> .tmp/build.txt; ctest --preset debug -R ''<motif>'' --output-on-failure *> .tmp/ctest.txt'; Select-String -Path .tmp/build.txt,.tmp/ctest.txt -Pattern 'error|tests passed|tests failed|No tests were found' | Select-Object -Last 4"
  ```
  Premier passage dans un worktree neuf : la même commande avec `-Commande 'pwsh -NoProfile -File ./scripts/build.ps1 *> .tmp/build.txt'`.
  Attendu au vert : `100% tests passed, 0 tests failed out of <k>`, k ≥ 1 (jamais `No tests were found`).
- **Fonctions d'un test** :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; . ./scripts/dev-env.ps1; & build/debug/tests/<test>.exe -o .tmp/<test>.txt,txt; Select-String -Path .tmp/<test>.txt -Pattern '^PASS   :|^FAIL!  :|^SKIP   :|^Totals' | Select-Object -Last 40"
  ```
  Attendu pour un talon rempli : chaque fonction nommée par la table en `PASS`, aucun `SKIP`,
  `Totals: <k> passed, 0 failed, 0 skipped`, k ≥ 1.
- **Preuve de tâche** (forme bornée du profil, sous le jeton) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1'; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2; Select-String -Path .tmp/test.txt -Pattern '<nom_du_test> .*Passed' | Select-Object -Last 1"
  ```
  Attendu : `100% tests passed, 0 tests failed out of N` (N = N6 + 43), `TESTS OK`, puis la ligne `Passed`
  du test nommé. Selon la décision 0121, une tâche peut se contenter de la commande ciblée **plus** cette preuve une
  fois avant son commit final ; la suite complète en debug et release est lancée par l'orchestrateur une fois par vague,
  après les merges.
- **Banc sans écran** (toute tâche qui touche `render/`, `input/`, `tools/` ou `ui/` ; résultat, pas porte) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu logiciel -Scenario court -Taille 1280x800 -HorsEcran > .tmp/bench.txt 2>&1'; Select-String -Path .tmp/bench.txt -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE' | Select-Object -Last 1; Get-ChildItem bench/results/*.json | Sort-Object LastWriteTime | Select-Object -Last 1 | ForEach-Object { `$j = Get-Content `$_.FullName -Raw | ConvertFrom-Json; 'content_check.ok = ' + `$j.content_check.ok; `$_.Name }"
  ```
  Attendu : `BUDGET TENU` ou `BUDGET DEPASSE` (jamais `MESURE INVALIDE`), `content_check.ok = True` ; nom du JSON joint
  au rapport (jamais commité).
- **Preuve visuelle hors écran** (jeton `tests`) : lectures `-Lire "<objet>.<propriété>=<attendu>"` d'abord, puis
  description des captures :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario <nom> -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire ''<objet.propriété=attendu>'' > .tmp/captures.txt 2>&1'; Select-String -Path .tmp/captures.txt -Pattern '^(COMMANDE|TRAIT|TOUCHE|TEXTE|DOIGT|PINCEMENT|POINTEUR|DOUBLECLIC|ATTENTE|PRET|FENETRE|LU|PIXEL|BORD|CAPTURE|RELANCE) |^SCENARIO |CAPTURES (OK|EN ECHEC)' | Select-Object -Last 16"
  ```
  Motifs d'échec : `CAPTURES EN ECHEC`, `SCENARIO EN ECHEC`, `SCENARIO INVALIDE`, toute ligne `LU` ou `PIXEL` en
  `ECART`, toute ligne d'étape en `ECHEC`.
- **Contrôle des noms** (après `git add`, avant le commit, puis après le commit) :
  ```powershell
  python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" "<wt>" | Select-Object -Last 1
  ```
  Attendu : `NOMS OK : <n> fichiers, 0 nom composé d'Inspire`.
- **Contrôle des titres du corpus** (toute tâche qui écrit un texte visible ou un document) : pour chaque fiche du
  corpus, la première ligne `# …` lue hors dépôt, cherchée dans le worktree ; sortie vide attendue :
  ```powershell
  pwsh -NoProfile -Command "Get-ChildItem 'C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay' -Filter *.md -Recurse | Where-Object Name -ne 'README.md' | ForEach-Object { (Get-Content `$_.FullName -TotalCount 1) -replace '^#\s*','' } | Where-Object { `$_.Trim().Length -ge 8 } | ForEach-Object { git -C '<wt>' grep -l -i -F -e `$_ -- . ':(exclude)graphify-out' }"
  ```
- **Mutations** : une mutation demandée est faite dans le fichier, prouvée rouge par la commande ciblée, puis remise
  par `git -C <wt> checkout -- <fichier>` (jamais par `Copy-Item`), et la commande ciblée rejouée verte.

## Talons : règle d'honnêteté

1. **Marqueur unique** `Talon L6 : rempli par <tâche>.` — en commentaire C++ dans chaque `.h`/`.cpp` talon, en
   commentaire QML dans chaque `.qml` talon, dans le champ `description` de chaque scénario JSON talon, et comme texte
   du `QSKIP` de chaque test talon : `QSKIP("Talon L6 : rempli par L6-T4.");`. `<tâche>` est **une** tâche de ce plan
   ou de celui de 6b.
2. Un test talon n'a qu'une fonction sautée, `reserved()` ; la tâche propriétaire la supprime et ajoute les siennes.
3. Compteur : N = N6 + 43 dès L6-T1 ; chaque preuve donne N exact **et** les fonctions attendues en `PASS`.
4. Contrôle (L6-T37 pour le lot) : `git -C <wt> grep -n "Talon L6 : rempli par" -- . ':(exclude)docs/'`
   ne rend plus que des tâches de 6b à la fin de 6a, et rien à la fin du lot ; sur la sortie de
   `ctest --preset debug -V`, aucune ligne `SKIP   :` qui contienne `Talon L6`.

<!-- suite -->
