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

## Sections des tâches

<a id="tache-l6-t19"></a>
### Tâche L6-T19 — `formats` : fragment de ressource

**Objectif.** D6-14 : une ressource ajoutée à la bibliothèque est un `.actcraft` d'**une page** qui ne contient que
les objets choisis et les seules ressources qu'ils référencent (images par empreinte). Écrire un fragment depuis un
document, le relire, et préparer son insertion (identifiants neufs, ressources à ajouter au document cible). Le patron
évident est le mauvais : écrire le document entier puis supprimer les pages et objets en trop garderait toutes les
ressources du document (taille, fuite de contenu d'un autre cours).

**Fichiers.** `formats/include/actcraft/formats/fragment.h`, `formats/src/fragment.cpp`, `tests/tst_formats_fragment.cpp`.

**Test et commande bornée.** Commande ciblée `-R 'tst_formats_fragment|tst_formats_nativefile'`.

**Interdits.** QtCore seulement (`formats/` sans QtGui) ; réutiliser l'écrivain et le lecteur natifs (aucun second
format) ; un objet de type inconnu reste gardé tel quel (L2-C8).

**Terminé quand** les cinq fonctions passent.

**Étapes.**
- [ ] **Étape 1 : en-tête** `fragment.h` :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>

  #include <QIODevice>
  #include <QString>

  #include <optional>
  #include <vector>

  namespace actcraft::formats {

  struct Fragment
  {
      core::Page page;          // une page qui ne contient que les objets du fragment
      core::Document resources; // document porteur des seules ressources référencées (même type que le natif)
  };

  // Écrit un .actcraft d'une page avec les objets `objects` de la page `page` de `document`.
  bool writeFragment(const core::Document& document, int page, const std::vector<core::ObjectId>& objects,
                     QIODevice& out, QString& error);
  std::optional<Fragment> readFragment(QIODevice& in, QString& error); // refuse un fichier de plus d'une page
  // Objets prêts à insérer : identifiants neufs pris dans `target`, ressources manquantes copiées dans `target`.
  std::vector<core::PageObject> prepareInsertion(const Fragment& fragment, core::Document& target);

  } // namespace actcraft::formats
  ```
  Le type qui porte les ressources (`core::Document` ou une table de ressources) : celui du lecteur natif du lot 2 ;
  lire `formats/include/actcraft/formats/` et adapter **ce seul membre**, en le notant au rapport.
- [ ] **Étape 2 : test rouge** `tst_formats_fragment.cpp` (document de 2 pages, une image A en page 1, une image B en
  page 2, un trait et une forme en page 1) : `objectsAndResourcesRoundTrip` (fragment du trait et de l'image A, relu :
  deux objets, mêmes empreintes d'objet) ; `onlyReferencedResourcesAreWritten` (archive du fragment : `resources/` ne
  contient que l'empreinte de A) ; `insertGivesFreshIdentifiers` (`prepareInsertion` dans le document d'origine :
  identifiants absents de toutes ses pages ; la ressource A n'est pas dupliquée) ; `unknownObjectKept` (un objet de
  type inconnu dans le fragment : relu, réécrit à l'identique) ; `notAFragmentRefused` (un `.actcraft` de 2 pages →
  `std::nullopt`, message).
- [ ] **Étape 3 : implémentation, vert, preuve de tâche** ; commit « Lot 6 : fragments de ressource au format natif
  (L6-T19) ».

<a id="tache-l6-t20"></a>
### Tâche L6-T20 — `ui` : bibliothèque de ressources

**Objectif.** `ui::ResourceLibrary` (F-810 à F-813, F-772) : trois racines (D6-14 : mes ressources, ressources
partagées, autre dossier), arborescence des dossiers, éléments d'un dossier (fragments `.actcraft`, images `.png`
`.jpg` `.svg`), compteur, liste ou grille, petite ou grande taille (réglages du profil), recherche par nom de fichier
et par mots-clés (lus dans les fragments, L6-T10) sur un **fil secondaire, annulable** (principe du corpus contraire à
la fluidité : jamais d'indexation bloquante, amendement 1, § 4), vignettes rendues sur `QThreadPool` par le peintre
partagé (A13) et mises en cache dans `<AppCacheLocation>/vignettes/<empreinte>.png`, renommer, supprimer, créer un
dossier, ajouter la sélection (`Ressource<N>.actcraft`, N = premier libre), insérer un élément dans la page courante
(image → commande d'insertion du lot 5, A11 ; fragment → `prepareInsertion` puis une commande d'ajout, une étape).

**Fichiers.** `ui/include/actcraft/ui/resourcelibrary.h`, `ui/src/resourcelibrary.cpp`, `tests/tst_ui_resources.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_resources` ; banc sans écran.

**Interdits.** Aucun accès disque sur le fil principal pendant un contact ; aucun pack de l'original ; aucune recherche
en ligne ; suppression = `QFile::moveToTrash` quand la plateforme le permet, sinon suppression après confirmation
(la commande QML demande la confirmation).

**Terminé quand** les neuf fonctions passent.

**Étapes.**
- [ ] **Étape 1 : en-tête** : `ui::ResourceLibrary : QAbstractListModel`, `QML_ELEMENT` ; propriétés `document`
  (`DocumentController*`), `profile` (`ProfileController*`), `root` (`mine`, `shared`, `other`), `folder` (chemin
  courant), `folders` (`QVariantList` [{path, name, depth}]), `itemCount`, `searching` (bool), `thumbnailsRendered`
  (int, tests) ; rôles `name`, `path`, `kind` (`fragment`, `image`), `thumbnail` (URL `file:` du cache ou vide) ;
  `Q_INVOKABLE` `setRoot`, `setOtherFolder(QUrl)`, `openFolder(path)`, `search(text, byName, byKeywords)`,
  `cancelSearch()`, `rename(path, newName)`, `remove(path)`, `createFolder(name)`, `QString addSelection()`,
  `bool insert(path, QPointF pagePoint)` ; fonctions statiques `mineRoot()`, `sharedRoot()`.
- [ ] **Étape 2 : test rouge** (`QStandardPaths::setTestModeEnabled(true)` ; racines dans un dossier temporaire par
  `setRootPathsForTests`) : `threeRoots` ; `foldersListed` ; `itemsOfAFolder` (2 images et 1 fragment → `itemCount` 3,
  genres justes) ; `thumbnailsOffTheMainThread` (rendu sur un fil dont l'identifiant diffère du fil principal,
  enregistré par le test ; `thumbnailsRendered` atteint 3) ; `searchByNameAndKeywords` (fragment aux mots-clés
  « soleil ») ; `searchIsCancellable` (200 fichiers, `search` puis `cancelSearch` → `searching` faux en moins de
  200 ms) ; `renameAndDelete` ; `addSelectionCreatesTheNextName` (`Ressource1.actcraft`, puis `Ressource2.actcraft`) ;
  `insertImageAndFragment` (deux insertions → deux étapes d'annulation, objets présents).
- [ ] **Étape 3 : implémentation, vert, banc, preuve de tâche** ; commit « Lot 6 : bibliothèque de ressources (L6-T20) ».

<a id="tache-l6-t21"></a>
### Tâche L6-T21 — QML : navigateur de ressources ; scénario `ressources`

**Objectif.** `ResourceBrowser.qml` : moitié haute (trois boutons de racine, chemin, bouton « … », recherche, arbre des
dossiers, menu des dossiers : nouveau, renommer, supprimer) ; moitié basse (compteur, vignettes ou liste, menu des
éléments : insérer, renommer, retirer ; bouton tampon). Insertion : glisser une vignette sur la page (point de dépôt),
double-clic (centre de la page), `resources.insert`. Tampon (D13) : tant qu'il est actif, chaque clic sur la page
insère une copie de l'élément choisi (la page est une couverture d'entrée du navigateur pendant le tampon, A7).
Ajout : glisser la sélection de la page sur un dossier ou sur la moitié basse → `addSelection`. Destinations
« Mes ressources » et « Ressources partagées » ajoutées à la capture de la caméra du lot 7 (A17, H7-3).

**Fichiers.** `ui/qml/ResourceBrowser.qml`, `bench/scenarios/ressources.json`, fichier de la caméra du lot 7 (A17).

**Test et commande bornée.** Commande ciblée `-R 'tst_captures_ressources|tst_ui_shell'` ; banc sans écran.

**Preuve visuelle.** `captures.ps1 -Scenario ressources … -Lire "appResources.itemCount=1" -Lire "appDocument.shownObjectCount=4"` :
`LU … OK`, `CAPTURES OK` ; décrire `bibliotheque.png`, `insere.png`, `tampon.png`.

**Interdits.** Le tampon ne trace jamais de trait ; aucune ressource de l'original ; aucun nom de paperboard réel dans
les éléments d'essai.

**Terminé quand** le scénario est vert et le banc joint.

**Étapes.**
- [ ] **Étape 1 : scénario rouge** `ressources.json` : `waitReady` ; une forme posée et sélectionnée ;
  `browser.resources` ; `resources.mine` ; `resources.add` ; `expect appResources.itemCount = 1` ; capture
  `bibliotheque.png` ; `page.next` (page neuve) ; `resources.insert <chemin de Ressource1.actcraft>` ; capture
  `insere.png` ; `resources.stamp` ; trois clics injectés sur la page ; `expect appDocument.shownObjectCount = 4` ;
  capture `tampon.png` ; `resources.stamp`.
- [ ] **Étape 2 : QML** (`ResourceLibrary { id: appResources; objectName: "appResources"; document: appDocument;
  profile: appProfile }`, gestionnaires `resources.*`).
- [ ] **Étape 3 : caméra.** Dans le fichier relevé (A17), ajouter les deux destinations ; leur action appelle
  `appResources.setRoot(...)` puis l'enregistrement de l'image capturée dans le dossier courant de cette racine.
- [ ] **Étape 4 : vert**, preuve visuelle, banc, preuve de tâche ; commit « Lot 6 : navigateur de ressources (L6-T21) ».

<a id="tache-l6-t22"></a>
### Tâche L6-T22 — Mode conception (F-560)

**Objectif.** D6-12 : `ui::DesignMode` porte la bascule (`mode.design`, F2, icône de la ligne supérieure) et neuf
réglages, tous vrais par défaut (F-560 ; D11 de L6-T0 fait foi) : objets verrouillés modifiables (`editLocked`),
objets masqués affichés (`showHidden`, en translucide à 40 %), actions mises en évidence (`highlightActions`),
actions et glisser-une-copie désactivés (`disableActions`), restrictions désactivées (`disableRestrictions`), objets
bloquants désactivés (`disableBlockers`), accrochage désactivé (`disableSnap`), interaction du bureau désactivée
(`disableDesktop`), astuces affichées (`showTips`) ; effets de changement de page suspendus. Effets branchés dans ce
lot : `editLocked` (outil de sélection, A21), `showHidden` (peintre partagé, option de rendu, A20), effets de page
(A18), `disableSnap` (accrochage des outils du lot 4, par l'option de page lue par `ToolHost`). Les autres sont
publiés (propriétés) pour le lot 8. Réglages gardés dans le profil (section `design-mode`).

**Fichiers.** `ui/include/actcraft/ui/designmode.h`, `ui/src/designmode.cpp`, `tests/tst_ui_designmode.cpp`,
`ui/src/toolhost.cpp`, `render/include/actcraft/render/pagepainter.h`, `render/src/pagepainter.cpp`,
`ui/qml/Main.qml`, `bench/scenarios/mode-creation.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_designmode|tst_render_painter|tst_tools_select|tst_captures_mode_creation'` ;
banc sans écran.

**Preuve visuelle.** `captures.ps1 -Scenario mode-creation … -Lire "appDesign.active=true"` : `LU … OK`,
`PIXEL masque-visible OK` (le centre d'un objet masqué est teinté), `CAPTURES OK` ; décrire `conception.png`,
`normal.png`.

**Interdits.** Le mode ne modifie aucun objet (verrouillé et masqué restent des drapeaux du document) ; un objet
masqué dessiné en mode conception change le cache : basculer le mode redessine **une** fois, jamais pendant
l'écriture.

**Terminé quand** les six fonctions passent et le scénario est vert.

**Étapes.**
- [ ] **Étape 1 : en-tête** : `ui::DesignMode : QObject`, `QML_ELEMENT`, `active` (bool, lié à `mode.design`), les neuf
  propriétés booléennes ci-dessus, `profile` (`ProfileController*` : lecture et écriture de la section
  `design-mode`), signal `effectsChanged`.
- [ ] **Étape 2 : test rouge** `tst_ui_designmode.cpp` : `toggleIsCheckable` (`trigger("mode.design")` → `active`,
  coche) ; `nineSettingsWithDefaults` ; `lockedObjectMovesWhileDesigning` (objet verrouillé : glisser de l'outil de
  sélection sans effet, puis avec `active` → déplacé, une étape) ; `hiddenObjectsDrawnTranslucent` (peintre partagé
  avec l'option : pixel au centre d'un objet masqué ≠ fond ; sans l'option : = fond) ; `transitionsSuspended`
  (propriété du lot 7 relevée, A18, fausse quand `active`) ; `settingsPublishedForLot8` (les quatre propriétés
  d'actions et de restrictions existent et suivent `active`).
- [ ] **Étape 3 : implémentation** (option `showHiddenTranslucent` ajoutée aux options de `paintPage`, défaut faux ;
  `ToolHost` consulte `DesignMode::editLocked()` avant de refuser un objet verrouillé ; `Main.qml` : instance
  `DesignMode { id: appDesign; objectName: "appDesign"; profile: appProfile }`, liaisons vers la vue, les effets de page
  et l'accrochage), **scénario** `mode-creation.json` (une forme masquée par `browser.properties`, `mode.design`,
  contrôle `masque-visible`, capture `conception.png`, `mode.design`, capture `normal.png`).
- [ ] **Étape 4 : vert**, preuve visuelle, banc, preuve de tâche ; commit « Lot 6 : mode conception (L6-T22) ».

<a id="tache-l6-t23"></a>
### Tâche L6-T23 — Tableau d'accueil

**Objectif.** F-090 sans les entrées exclues (vote, services en ligne de Promethean) et avec les seuls imports
retenus (PDF, N1-A8) : fenêtre d'accueil devant la fenêtre principale au démarrage (désactivable par une case,
`appPreferences.dashboardAtStart`), rouvrable par `view.dashboard` (F11) ; colonne gauche : Paperboards, Annoter le
bureau (commande du lot 7, absente d'Android), Réglages (`profile.edit`), Aide (`help.contents`) ; panneau
Paperboards : créer, ouvrir, importer un PDF (commande du lot 5), puis les documents récents (L6-T24) en vignettes
(miniature du fichier natif, `thumbnails/page-1.png`, lue hors du fil principal). Le tableau ne s'ouvre au démarrage
que lancé par `app/main.cpp` (propriété `appShowDashboard` posée là) : jamais dans le joueur de captures ni le banc.

**Fichiers.** `ui/qml/Dashboard.qml`, `app/main.cpp`, `bench/scenarios/tableau-de-bord.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_captures_tableau_de_bord|tst_ui_shell'`.

**Preuve visuelle.** `captures.ps1 -Scenario tableau-de-bord … -Lire "dashboard.visible=true" -Lire "dashboard.recentCount=2"` :
`LU … OK`, `LU appDocument.pageCount=3 OK`, `CAPTURES OK` ; décrire `accueil.png`.

**Interdits.** Aucun nom ni texte de l'original (pas de « Dashboard » visible : « Accueil ») ; aucun réseau.

**Terminé quand** le scénario est vert.

**Étapes.**
- [ ] **Étape 1 : scénario rouge** `tableau-de-bord.json` : `waitReady` ; deux documents d'essai synthétiques (3 et 1
  pages) écrits sous `.tmp/` par `file.save-as` puis fermés, ce qui les met dans les récents ; `view.dashboard` ;
  `expect dashboard.visible = true` ; `expect dashboard.recentCount = 2` ; capture `accueil.png` ; clic sur
  `recent0` ; `expect appDocument.pageCount = 3` ; `expect dashboard.visible = false`.
- [ ] **Étape 2 : QML et `app/main.cpp`** (propriété de contexte `appShowDashboard` vraie dans l'application
  seulement ; `Dashboard.qml` ouvert au démarrage si `appShowDashboard && appPreferences.dashboardAtStart` ;
  délégués `recent<n>` ; gestionnaire `view.dashboard`).
- [ ] **Étape 3 : vert**, preuve visuelle, preuve de tâche ; contrôle des titres du corpus ; commit « Lot 6 : tableau
  d'accueil (L6-T23) ».

<a id="tache-l6-t24"></a>
### Tâche L6-T24 — Documents récents et résumé à l'enregistrement

**Objectif.** `ui::RecentDocuments` : liste du poste (`workstation/recent-documents`, D6-1), dix au plus, le dernier
ouvert ou enregistré en tête, fichiers locaux absents cachés (une URL `content://` est gardée : son existence ne se teste
pas), vidée par `file.recent-clear` ; sous-menu « Documents récents » du menu Fichier (entrées dynamiques qui appellent
`file.open <url>`) ; résumé à l'enregistrement (réglage `document/summary-on-save` du profil, faux par défaut,
F-930 section Paperboard) : boîte qui donne le nom, le nombre de pages, d'objets, de ressources et la taille du
fichier, après la fin de l'enregistrement (A16).

**Fichiers.** `ui/include/actcraft/ui/recentdocuments.h`, `ui/src/recentdocuments.cpp`, `tests/tst_ui_recents.cpp`,
`ui/qml/SaveSummaryDialog.qml`, `ui/qml/FileActions.qml`, `ui/qml/CommandMenu.qml`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_recents|tst_ui_files|tst_ui_shell'`.

**Interdits.** Le résumé se calcule sur le modèle en mémoire, pas en relisant le fichier ; aucun chemin d'essai réel
dans les tests (dossiers temporaires).

**Terminé quand** les six fonctions passent.

**Étapes.**
- [ ] **Étape 1 : en-tête** : `ui::RecentDocuments : QObject`, `QML_ELEMENT`, `preferences` (`Preferences*`), `entries`
  (`QVariantList` [{url, name, when}]), `count` ; `Q_INVOKABLE` `note(QUrl)`, `clear()`, `QVariantMap summary(DocumentController*, QUrl)`
  (`pages`, `objects`, `resources`, `bytes`) ; `static constexpr int kMax = 10`.
- [ ] **Étape 2 : test rouge** `tst_ui_recents.cpp` : `openedFileGoesFirst`, `boundedToTen`, `missingFilesHidden`,
  `contentUrlsKept`, `clearEmptiesTheList`, `summaryCountsPagesAndObjects` (document de 2 pages et 3 objets).
- [ ] **Étape 3 : implémentation et QML** (`FileActions.qml` appelle `appRecents.note(url)` à la fin d'une ouverture et
  d'un enregistrement, puis ouvre `SaveSummaryDialog` si le réglage est vrai ; `CommandMenu.qml` insère, après
  `file.open`, un sous-menu dynamique « Documents récents » quand `appRecents.count > 0`).
- [ ] **Étape 4 : vert**, preuve de tâche ; commit « Lot 6 : documents récents et résumé (L6-T24) ».

<a id="tache-l6-t25"></a>
### Tâche L6-T25 — Éditeur de profils, choix du profil, onglet format

**Objectif.** F-123 : `profile.choose` ouvre `ProfileChooser.qml` (liste `appProfile.profiles`, un clic active et
ferme). F-882 : `profile.edit` ouvre `ProfileEditor.qml` : en haut, liste du profil courant (changer l'active) et
boutons importer, enregistrer, enregistrer sous, supprimer (grisé pour un profil fourni), revenir à l'origine (profil
fourni modifié) ; quatre onglets : Format (`FormatPage.qml`, ci-dessous), Commandes (L6-T26), Raccourcis vers des
programmes (L6-T30, Windows), Paramètres (L6-T27) ; bouton « Terminé » (enregistre). F-900, onglet Format : taille
des icônes (16 à 32), palette (ancrage, 8/16/24 couleurs, épinglée, réduite, réduite au maximum), navigateurs (côté,
épinglés, visibles), corbeille visible, plein écran au démarrage.

**Fichiers.** `ui/qml/ProfileChooser.qml`, `ui/qml/ProfileEditor.qml`, `ui/qml/FormatPage.qml`,
`tests/tst_ui_profileeditor.cpp`, `bench/scenarios/profils.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_profileeditor|tst_captures_profils|tst_ui_shell'`.

**Preuve visuelle.** `captures.ps1 -Scenario profils … -Lire "appProfile.currentId=mon-profil"` : `LU … OK`,
`CAPTURES OK` ; décrire `editeur.png`, `format.png`.

**Interdits.** Tout passe par les commandes `profile.*` et les setters de `appProfile` ; aucun accès disque en QML.

**Terminé quand** les cinq fonctions et le scénario passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_profileeditor.cpp` (montage QML de `tst_ui_shell`) : `chooserActivatesAndCloses`,
  `saveAsCreatesAUserProfile` (`profile.save-as "mon-profil"` → dans `profiles`, `builtin` faux),
  `deleteRefusedForBuiltin` (bouton grisé, commande sans effet), `resetRestoresTheBuiltin`, `formatPageEditsTheProfile`
  (réglette des icônes → `appProfile.iconPixels`).
- [ ] **Étape 2 : QML** et gestionnaires `profile.*` (`profile.import` et `profile.export` : `FileDialog` si sans
  argument, sinon l'URL ; `profile.save-as` sans argument : champ de nom).
- [ ] **Étape 3 : scénario** : ajouter à la fin de `profils.json` : `profile.edit` ; capture `editeur.png` ;
  `profile.save-as "mon-profil"` ; onglet Format ; capture `format.png` ; `expect appProfile.currentId = mon-profil`.
- [ ] **Étape 4 : vert**, preuve visuelle, preuve de tâche ; commit « Lot 6 : éditeur de profils et onglet format
  (L6-T25) ».

<a id="tache-l6-t26"></a>
### Tâche L6-T26 — Onglet commandes (F-910)

**Objectif.** `ui::LayoutEditor` lit la disposition effective (profil, sinon défaut ; A1), présente à gauche toutes
les commandes disponibles sur la plateforme par catégorie (préfixe de l'identifiant), à droite une cible (palette
principale, barre d'objet, menu contextuel, chaque menu) et son contenu ordonné, séparateurs et sous-menus compris ;
ajouter, retirer, monter, descendre, insérer un séparateur ; « Terminé » écrit **seulement**
`<dossier du profil>/layout.json` (jamais `actions/data/layout.json`), puis `Commands::reload()`. Le patron évident est
le mauvais : réécrire la disposition entière à partir du seul contenu affiché perdrait les clés que l'éditeur ne
connaît pas ; on part du JSON effectif et on ne remplace que la liste de la cible.

**Fichiers.** `ui/include/actcraft/ui/layouteditor.h`, `ui/src/layouteditor.cpp`, `tests/tst_ui_layouteditor.cpp`,
`ui/qml/CommandsPage.qml`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_layouteditor|tst_ui_commands|tst_actions_layout'`.

**Interdits.** Le fichier de disposition par défaut n'est jamais écrit ; une commande absente de la plateforme n'est
pas proposée.

**Terminé quand** les huit fonctions passent.

**Étapes.**
- [ ] **Étape 1 : en-tête** : `ui::LayoutEditor : QObject`, `QML_ELEMENT`, `commands` (`Commands*`), `profile`
  (`ProfileController*`), `targets` (`QVariantList` [{id, label}]), `target`, `items` (`QVariantList` du contenu de la
  cible), `catalog` (`QVariantList` [{id, label, icon, category}]) ; `Q_INVOKABLE` `add(id)`, `removeAt(i)`,
  `moveUp(i)`, `moveDown(i)`, `addSeparator()`, `bool apply()`.
- [ ] **Étape 2 : test rouge** `tst_ui_layouteditor.cpp` : `targetsListed`, `addAppendsToTheTarget`, `moveUpAndDown`,
  `separatorAllowed`, `writesTheProfileLayoutOnly` (fichier écrit dans `store().directory(id)`, empreinte du fichier de
  ressources inchangée), `defaultLayoutUntouched`, `reloadShowsTheChange` (`Commands::palette()` contient la commande
  ajoutée), `unavailableCommandsHidden` (plateforme Android : `launcher.edit` absent du catalogue).
- [ ] **Étape 3 : implémentation, QML** (`CommandsPage.qml` : deux listes, quatre boutons), **vert**, preuve de tâche ;
  commit « Lot 6 : onglet commandes de l'éditeur de profils (L6-T26) ».

<a id="tache-l6-t27"></a>
### Tâche L6-T27 — Onglet paramètres (F-930, F-935)

**Objectif.** `ui::SettingsSchema` décrit, en données à nous, les sections de F-930 **des fonctions incluses** (aucune
section du vote) : Horloge, Effets (F-935), Objets de paperboard, Langue (L6-T36), Réseau (L6-T28, Windows),
Orthographe (L6-T34), Outils, Saisie, Enregistrements (lot 8 : section déclarée, réglages lus par le lot 8), Profil et
ressources, Paperboard, Multimédia (lot 8), Mode conception (L6-T22). Chaque réglage : clé, libellé à nous, type,
défaut (F-930 ; D de L6-T0), bornes, **magasin** (`profile` : `appProfile.setting(section, clé)` ; `workstation` : la
clé `QSettings` déjà employée par le lot 7, A19 — reprise telle quelle, jamais dupliquée). P2 du lot 10 : trois
réglages de qualité du rendu (haute qualité, haute qualité pendant l'écriture — faux par défaut —, haute qualité
pendant l'édition d'objets — défaut de D14) ; P10 : regroupement automatique des traits (faux par défaut ; lu par le
stylo s'il existe, sinon publié pour le lot 11 et noté). E4 : taille de page par défaut (`defaults/page-size`, D6-13).
`SettingsPage.qml` : sections à gauche, réglages à droite, éditeurs par type.

**Fichiers.** `ui/include/actcraft/ui/settingsschema.h`, `ui/src/settingsschema.cpp`, `tests/tst_ui_settingsschema.cpp`,
`ui/qml/SettingsPage.qml`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_settingsschema`.

**Interdits.** Aucun libellé de l'original ; aucune section du vote ; aucune clé `QSettings` neuve en `camelCase`.

**Terminé quand** les huit fonctions passent.

**Étapes.**
- [ ] **Étape 1 : en-tête** : `ui::SettingsSchema : QObject`, `QML_ELEMENT`, `profile` (`ProfileController*`),
  `preferences` (`Preferences*`), `sections` (`QVariantList` [{id, label, platforms}]) ;
  `Q_INVOKABLE QVariantList entries(section)` ([{key, label, type, defaultValue, min, max, choices, store}]),
  `Q_INVOKABLE QVariant value(section, key)`, `Q_INVOKABLE void setValue(section, key, value)` ; table statique des
  réglages dans `settingsschema.cpp`.
- [ ] **Étape 2 : test rouge** : `sectionsOfIncludedFunctionsOnly`, `noVoteSection`, `defaultsFromTheSpecification`
  (horloge : chiffres oui, aiguille des secondes oui, 24 h non ; effets : largeur du spot 15, vitesse du rideau 15 ;
  objets : tolérance de centrage 10), `lot7KeysReused` (les clés du lot 7 de A19 apparaissent avec `store:
  workstation` et la même clé), `profileOrWorkstationStore`, `renderQualityThreeSettings`, `autoGroupDefaultsOff`,
  `defaultPageSize` (« 1024x768 »).
- [ ] **Étape 3 : implémentation, QML, vert**, preuve de tâche ; contrôle des titres du corpus ; commit « Lot 6 : onglet
  paramètres (L6-T27) ».

<a id="tache-l6-t28"></a>
### Tâche L6-T28 — Trousseau de Windows et réglages réseau

**Objectif.** D6-8 : `ui::SecretStore` range un secret sous un nom (`ActCraft/<nom>`) dans le Gestionnaire
d'identification de Windows (`CredWriteW`, `CredReadW`, `CredDeleteW`, type `CRED_TYPE_GENERIC`, persistance
`CRED_PERSIST_LOCAL_MACHINE` : propre au poste) ; ailleurs, `available()` est faux. `ui::NetworkSettings` (F-930,
réseau, Windows) : hôte, port, compte du mandataire dans `QSettings` (`network/proxy-host`, `network/proxy-port`,
`network/proxy-user`), mot de passe dans le trousseau, appliqués par `QNetworkProxy::setApplicationProxy` au démarrage
et à chaque changement ; aucun mandataire par défaut. `NetworkPage.qml` : formulaire, chargé par l'onglet paramètres
(section Réseau, Windows seulement).

**Fichiers.** `ui/include/actcraft/ui/secretstore.h`, `ui/src/secretstore_win.cpp`, `ui/src/secretstore_other.cpp`,
`ui/include/actcraft/ui/networksettings.h`, `ui/src/networksettings.cpp`, `tests/tst_ui_network.cpp`,
`ui/qml/NetworkPage.qml`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_network`.

**Interdits.** Le mot de passe n'est jamais écrit dans `QSettings`, un journal ou un message ; aucun `#ifdef` hors des
fichiers `*_win` / `*_other` (DA10) ; les tests nettoient leurs secrets (`remove`) même en échec (`cleanup()`).

**Terminé quand** les six fonctions passent (sous Windows ; `unavailableOffWindows` s'exécute partout, les autres
`QSKIP` hors Windows avec la raison).

**Étapes.**
- [ ] **Étape 1 : en-tête** `secretstore.h` :
  ```cpp
  #pragma once

  #include <QByteArray>
  #include <QString>

  #include <optional>

  namespace actcraft::ui {

  // Secrets propres au poste (D6-8). Windows : Gestionnaire d'identification ; ailleurs : indisponible.
  class SecretStore
  {
  public:
      static bool available();
      static bool write(const QString& name, const QByteArray& secret);
      static std::optional<QByteArray> read(const QString& name);
      static bool remove(const QString& name);
  };

  } // namespace actcraft::ui
  ```
  `networksettings.h` : `ui::NetworkSettings : QObject`, `QML_ELEMENT`, propriétés `host`, `port`, `user`, `available`,
  `Q_INVOKABLE void setPassword(QString)`, `Q_INVOKABLE void apply()`, `Q_INVOKABLE void clear()` ; constructeur de test
  qui prend un `Preferences*`.
- [ ] **Étape 2 : test rouge** : `secretRoundTrip` (nom `essai-<uuid>`) ; `secretAbsentIsEmpty` ; `proxyApplied` (hôte
  `proxy.test`, port 3128 → `QNetworkProxy::applicationProxy()` les reprend) ; `passwordNeverInSettings` (après
  `setPassword("s3cret")`, aucune valeur de `Preferences::allKeys()` ne contient `s3cret`) ; `noProxyByDefault`
  (`QNetworkProxy::NoProxy`) ; `unavailableOffWindows` (sous Windows : `available()` vrai ; ailleurs : faux).
- [ ] **Étape 3 : implémentation** (`secretstore_win.cpp` : `#include <windows.h>`, `#include <wincred.h>`, lien
  `advapi32` — ajouter `target_link_libraries(actcraft_ui PRIVATE advapi32)` **seulement** si l'édition de liens
  échoue : alors le noter, c'est un fichier CMake hors de la ligne et l'orchestrateur amende), QML, **vert**, preuve
  de tâche ; commit « Lot 6 : trousseau du poste et réglages réseau (L6-T28) ».

<a id="tache-l6-t29"></a>
### Tâche L6-T29 — Raccourcis vers des programmes : modèle, signature, lancement

**Objectif.** F-920 et N1-A6 : un raccourci (programme ou fichier, paramètres, info-bulle) est gardé dans le profil
(`launchers`), donc transportable ; il est **signé** par HMAC-SHA256 (`QMessageAuthenticationCode`) avec une clé de
32 octets propre au poste, créée au premier usage et rangée dans le trousseau (L6-T28). Au lancement, une signature qui
ne vérifie pas (profil venu d'un autre poste, ou chemin ou paramètres modifiés à la main) demande une confirmation :
refusée, rien n'est lancé ; acceptée, le raccourci est signé à nouveau. Le chemin est affiché à la création.
Lancement : `QProcess::startDetached(programme, paramètres)` pour un exécutable, `QDesktopServices::openUrl` pour un
fichier. Programme d'essai `launcher_probe` : écrit le fichier dont le chemin est son premier argument, puis sort.

**Fichiers.** `ui/include/actcraft/ui/launchers.h`, `ui/src/launchers.cpp`, `tests/launcher_probe.cpp`,
`tests/tst_ui_launchers.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_launchers` (le test trouve `launcher_probe.exe` à côté de lui).

**Interdits.** Jamais de lancement sans signature valide ni confirmation ; aucune interprétation par un shell
(`cmd /c` interdit : arguments passés en liste) ; aucun raccourci sous Android (commandes déclarées Windows).

**Terminé quand** les huit fonctions passent.

**Étapes.**
- [ ] **Étape 1 : programme d'essai** `tests/launcher_probe.cpp` :
  ```cpp
  #include <cstdio>

  int main(int argc, char** argv)
  {
      if (argc < 2)
          return 2;
      std::FILE* f = std::fopen(argv[1], "wb");
      if (!f)
          return 1;
      std::fputs("lance", f);
      std::fclose(f);
      return 0;
  }
  ```
- [ ] **Étape 2 : en-tête** : `ui::Launchers : QObject`, `QML_ELEMENT`, `profile` (`ProfileController*`), `entries`
  (`QVariantList` [{id, path, arguments, toolTip, trusted}]) ; `Q_INVOKABLE QString add(path, arguments, toolTip)`,
  `Q_INVOKABLE bool remove(id)`, `Q_INVOKABLE int run(id)` (0 lancé, 1 confirmation demandée par le signal
  `confirmationNeeded(id, path)`, 2 échec), `Q_INVOKABLE bool confirm(id, bool accepted)` ; `static QByteArray
  signatureOf(path, arguments, key)`.
- [ ] **Étape 3 : test rouge** `tst_ui_launchers.cpp` : `addShowsThePath` ; `runStartsTheProgram` (fichier témoin
  sous un dossier temporaire créé en moins de 5 s) ; `signedOnThisWorkstation` ; `foreignSignatureAsksFirst`
  (signature remplacée dans le profil → `run` rend 1, aucun fichier) ; `refusedForeignRunsNothing` ;
  `acceptedForeignIsResigned` (confirmé → lancé, puis `run` rend 0 sans demander) ; `removeDeletes` ;
  `tamperedArgumentsAskAgain` (paramètres modifiés dans le profil → 1). Hors Windows : `QSKIP("raccourcis : Windows seulement")`.
- [ ] **Étape 4 : implémentation, vert**, preuve de tâche ; commit « Lot 6 : raccourcis vers des programmes signés par
  poste (L6-T29) ».

<a id="tache-l6-t30"></a>
### Tâche L6-T30 — Raccourcis vers des programmes : barre et onglet ; scénario S20

**Objectif.** F-142 et E3 (place tranchée par L6-T0, D4 ; défaut : bouton d'édition en bas de la barre de raccourcis,
visible si la palette n'est ni réduite ni réduite au maximum) : les raccourcis s'affichent comme boutons de la barre
(`launcherButton<n>`, info-bulle) ; `LaunchersPage.qml` (onglet de l'éditeur de profils, Windows) : liste, ajouter
(choix du fichier, chemin affiché, paramètres, info-bulle), retirer ; boîte de confirmation d'un raccourci étranger.
S20 de bout en bout.

**Fichiers.** `ui/qml/LaunchersPage.qml`, `ui/qml/ShortcutBar.qml`, `bench/scenarios/raccourcis-programmes.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_captures_raccourcis_programmes|tst_ui_shortcutbar|tst_ui_shell'`.

**Preuve visuelle.** `captures.ps1 -Scenario raccourcis-programmes … -Lire "launcherButton0.toolTip=Programme d'essai"` :
`LU … OK`, puis `Test-Path .tmp/temoin-1.txt` vrai et `Test-Path .tmp/temoin-2.txt` faux, `CAPTURES OK` ; décrire
`barre.png`, `confirmation.png`. Sous Android (captures de l'APK, fin de lot) : commande d'édition absente.

**Interdits.** Le scénario n'écrit que sous `.tmp/` ; il lance seulement `launcher_probe`.

**Terminé quand** le scénario est vert.

**Étapes.**
- [ ] **Étape 1 : scénario rouge** : `waitReady` ; `launcher.edit` ; ajout du raccourci vers
  `build/debug/tests/launcher_probe.exe` avec l'argument `.tmp/temoin-1.txt` et l'info-bulle « Programme d'essai » (par
  le gestionnaire de commande avec arguments, sans dialogue de fichier) ; capture `barre.png` ; clic sur
  `launcherButton0` ; attente 2 s ; puis le profil d'un « autre poste » (fichier d'essai écrit par le scénario sous
  `.tmp/`, signature fausse, argument `.tmp/temoin-2.txt`) importé par `profile.import` et activé ; clic sur son
  bouton ; capture `confirmation.png` ; refus ; retrait du premier raccourci ; `expect shortcutBar.launcherCount = 0`.
- [ ] **Étape 2 : QML**, **vert**, preuve visuelle, preuve de tâche ; commit « Lot 6 : raccourcis vers des programmes
  dans la barre et l'éditeur (L6-T30) ».

<a id="tache-l6-t31"></a>
### Tâche L6-T31 — Mode classe protégé (F-298) ; scénario S19

**Objectif.** D6-7 et N1-A5 : `ui::ClassroomLock` — aucun mot de passe par défaut ; au premier `classroom.lock`, la
boîte demande de créer un mot de passe (deux saisies) ; empreinte PBKDF2-SHA256 (`QPasswordDigestor::deriveKeyPbkdf2`,
200 000 itérations, 32 octets) avec un sel aléatoire de 16 octets (`QRandomGenerator::system()`), gardés en Base64
dans `classroom/hash`, `classroom/salt`, `classroom/iterations` ; état `classroom/locked` gardé ; verrouiller =
`CommandRegistry::setLocked(true)` (porte de L6-T3) par `Commands::setLocked` ; déverrouiller demande le mot de
passe, comparé en temps constant. `classroom.password` change le mot de passe (l'ancien d'abord). Oubli : procédure
dans l'aide (supprimer les clés `classroom/*`). `Commands` gagne `locked` (propriété), `setLocked`, `lockedCount`
(nombre de commandes grisées par le verrou, pour les scénarios). `QtNetwork` ajouté à la liste autorisée des paquets.

**Fichiers.** `ui/include/actcraft/ui/classroomlock.h`, `ui/src/classroomlock.cpp`, `ui/include/actcraft/ui/commands.h`,
`ui/src/commands.cpp`, `tests/tst_ui_classroomlock.cpp`, `ui/qml/ClassroomLockDialog.qml`,
`bench/scenarios/classe-protegee.json`, `packaging/qt-autorises.txt`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_classroomlock|tst_ui_commands|tst_actions_lockable|tst_captures_classe_protegee'`.

**Preuve visuelle.** `captures.ps1 -Scenario classe-protegee … -Lire "appClassroom.locked=true"` : `LU … OK`,
`LU appCommands.lockedCount>0 OK` (si le joueur ne lit pas `>`, lire la valeur exacte relevée et l'écrire), `CAPTURES OK` ;
décrire `verrouille.png` (menus grisés) et `deverrouille.png`.

**Interdits.** Aucun mot de passe en clair nulle part (réglages, journal, capture) ; aucune voie QML qui rouvre une
commande verrouillée (le registre décide) ; aucun mot de passe par défaut.

**Terminé quand** les huit fonctions et le scénario passent.

**Étapes.**
- [ ] **Étape 1 : en-tête** : `ui::ClassroomLock : QObject`, `QML_ELEMENT`, `commands` (`Commands*`), `preferences`
  (`Preferences*`), `locked` (bool), `hasPassword` (bool) ; `Q_INVOKABLE bool createPassword(QString, QString)`,
  `Q_INVOKABLE bool lock()`, `Q_INVOKABLE bool unlock(QString)`, `Q_INVOKABLE bool changePassword(QString, QString)` ;
  `static constexpr int kIterations = 200000`.
- [ ] **Étape 2 : test rouge** `tst_ui_classroomlock.cpp` : `firstLockCreatesThePassword` ; `everyLockableGreyed`
  (pour chaque commande disponible : `isEnabled` faux ⇔ `lockable`) ; `wrongPasswordKeepsTheLock` ;
  `rightPasswordUnlocks` ; `lockSurvivesRestart` (nouvel objet sur le même INI → `locked`, commandes grisées) ;
  `passwordNeverStoredInClear` (aucune valeur de `allKeys()` ne contient le mot de passe ni son Base64) ;
  `saltDiffersPerPassword` ; `qmlCannotBypass` (`Commands::setEnabled(id, true)` sur une verrouillable pendant le
  verrou → `trigger` faux).
- [ ] **Étape 3 : implémentation** (`Commands::setLocked` appelle `m_registry.setLocked` puis `bumpState()` ;
  `lockedCount` compte les commandes disponibles `lockable`), `packaging/qt-autorises.txt` (ligne `Qt6Network`),
  QML, **scénario** `classe-protegee.json` (`classroom.lock` → création « essai-1234 » deux fois, `expect
  appClassroom.locked = true`, capture `verrouille.png`, déverrouiller avec « faux » → toujours verrouillé, avec le bon →
  libre, capture `deverrouille.png`, verrouiller à nouveau, `restart`, `expect appClassroom.locked = true`).
- [ ] **Étape 4 : vert**, preuve visuelle, preuve de tâche, `scripts/verifier-licences.ps1` (`LICENCES OK` ou son motif
  réel) ; commit « Lot 6 : mode classe protégé par mot de passe (L6-T31) ».

<a id="tache-l6-t32"></a>
### Tâche L6-T32 — Orthographe I : Hunspell et dictionnaire

**Objectif.** D6-9 : compiler Hunspell 1.7.2 dans `third_party/hunspell` (bibliothèque statique, sans ses outils), sous
Windows (MSVC, `/W4 /WX` **désactivés pour la cible tierce seulement**), macOS (CI) et Android (presets
`android-arm64`, `android-x86_64`) ; dictionnaire français de Grammalecte (variante « classique », MPL 2.0) dans
`third_party/dictionaries/fr`, intégré aux ressources de `ui` (`:/actcraft/dictionaries/fr.aff`, `fr.dic`) et copié dans
un dossier lisible par Hunspell au premier usage (Hunspell lit des chemins de fichiers). Notices dans
`THIRD_PARTY_NOTICES.md` (Hunspell sous LGPL 2.1 au choix de sa triple licence ; dictionnaire sous MPL 2.0).

**Fichiers.** Ceux de la ligne L6-T32.

**Test et commande bornée.** Commande ciblée `-R tst_core_spellcheck` (lié, toujours talon) ; preuve de tâche ;
construction Android : `pwsh -NoProfile -File ./scripts/paquet-android.ps1 > .tmp/apk.txt 2>&1; Select-String -Path .tmp/apk.txt -Pattern 'APK OK|ECHEC' | Select-Object -Last 1`
(ou, si la chaîne locale manque, push d'une branche `essai-ci/lot6-hunspell` et lecture du job `android`, puis
suppression de la branche).

**Interdits.** **Aucun téléchargement sans l'accord écrit de Jérôme (H6-6), relayé par l'orchestrateur.** Sources prises
au dépôt officiel, version étiquetée, archive vérifiée par sa somme SHA-256 notée au rapport ; décompression dans un
dossier neuf sous `.tmp/`, puis copie des seuls fichiers sources (`src/hunspell/*.{cxx,hxx,h}`, licences) ; aucun script
du paquet exécuté.

**Terminé quand** Windows et Android construisent, `TESTS OK`, notices écrites.

**Étapes.**
- [ ] **Étape 1 : accord** — vérifier dans le fichier d'état que l'accord H6-6 est écrit ; sinon, s'arrêter et le
  rendre au rapport.
- [ ] **Étape 2 : sources** (archive de la version 1.7.2 et dictionnaire), somme SHA-256 relevée, fichiers copiés.
- [ ] **Étape 3 : CMake** : `third_party/hunspell/CMakeLists.txt` (`add_library(hunspell STATIC …)`,
  `target_compile_definitions(hunspell PUBLIC HUNSPELL_STATIC)`, avertissements de la cible ramenés au niveau par défaut) ;
  `CMakeLists.txt` racine : `add_subdirectory(third_party/hunspell)` à côté de miniz ; `core/CMakeLists.txt` :
  `target_link_libraries(actcraft_core PRIVATE hunspell)` ; `ui/CMakeLists.txt` : ressources du dictionnaire.
- [ ] **Étape 4 : construire** (debug, release, Android), preuve de tâche, `NOMS OK` ; commit « Lot 6 : Hunspell et
  dictionnaire français en bibliothèques tierces (L6-T32) ».

<a id="tache-l6-t33"></a>
### Tâche L6-T33 — Orthographe II : vérificateur

**Objectif.** F-297 dans `core` : parcourir les pages depuis la première, les objets texte dans l'ordre des couches
(du fond vers le dessus) puis de profondeur, extraire les mots du texte riche (balises retirées, apostrophes et traits
d'union gardés dans le mot), demander au moteur (`SpellEngine`, interface ; `HunspellEngine` dans
`hunspellengine.cpp`) s'ils sont justes ; règles d'ignorance (D5 ; défauts F-930 : majuscule initiale, MAJUSCULES,
chiffres, casse mixte, adresses — toutes non ; mots répétés signalés — oui) ; « ignorer », « ignorer toujours » (liste de
la session), « ajouter au dictionnaire » (dictionnaire de l'utilisateur, un mot par ligne, fichier passé par l'appelant),
« remplacer » par une commande d'édition de texte du lot 4 (une étape).

**Fichiers.** `core/include/actcraft/core/spellcheck.h`, `core/src/spellcheck.cpp`, `core/src/hunspellengine.cpp`,
`tests/tst_core_spellcheck.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_core_spellcheck`.

**Interdits.** QtCore seulement ; le dictionnaire de l'utilisateur n'est jamais écrit dans `third_party/`.

**Terminé quand** les huit fonctions passent.

**Étapes.**
- [ ] **Étape 1 : en-tête** `spellcheck.h` : `struct SpellRules { bool ignoreCapitalized = false, ignoreUppercase =
  false, ignoreDigits = false, ignoreMixedCase = false, ignoreAddresses = false, flagRepeated = true; };`
  `class SpellEngine { public: virtual ~SpellEngine() = default; virtual bool isCorrect(const std::string&) = 0;
  virtual std::vector<std::string> suggest(const std::string&) = 0; virtual void addWord(const std::string&) = 0; };`
  `std::unique_ptr<SpellEngine> makeHunspellEngine(const std::string& affPath, const std::string& dicPath);`
  `struct SpellHit { int page; ObjectId object; std::size_t start; std::size_t length; std::string word; bool repeated; };`
  `class SpellChecker { public: SpellChecker(SpellEngine&, SpellRules); std::optional<SpellHit> next(const Document&);
  void ignore(); void ignoreAlways(const std::string&); void restart(); std::unique_ptr<UndoCommand> replace(const
  Document&, const SpellHit&, const std::string&) const; };`
- [ ] **Étape 2 : test rouge** (moteur réel sur le dictionnaire de L6-T32, copié dans un dossier temporaire ; document
  de 2 pages, « bonjour mosieur » en page 2) : `findsTheMisspelledWord` (`mosieur`, page index 1) ;
  `suggestsTheCorrection` (« monsieur » parmi les suggestions) ; `walksPagesFromTheFirst` ; `ignoreAlwaysSkipsTheWord` ;
  `userDictionaryKept` ; `repeatedWordsFlagged` (« le le ») ; `replaceIsUndoneAtOnce` ; `htmlTagsAreNotWords` (`<b>`).
- [ ] **Étape 3 : implémentation, vert**, preuve de tâche ; commit « Lot 6 : vérificateur d'orthographe (L6-T33) ».

<a id="tache-l6-t34"></a>
### Tâche L6-T34 — Orthographe III : boîte, F7 ; scénario S21

**Objectif.** `ui::Spelling` relie `SpellChecker` au document : `spell.check` (F7) part de la première page ; à chaque
mot, la page qui le contient devient la page courante et le mot est surligné ; `SpellingDialog.qml` : mot en rouge,
suggestions, champ de remplacement, boutons ignorer, ignorer toujours, ajouter au dictionnaire, remplacer, remplacer
tout, fermer ; message de fin (« Vérification terminée »). Dictionnaire de l'utilisateur :
`<AppDataLocation>/dictionnaire-utilisateur.txt` (réglage de la section Orthographe, L6-T27).

**Fichiers.** `ui/include/actcraft/ui/spelling.h`, `ui/src/spelling.cpp`, `tests/tst_ui_spelling.cpp`,
`ui/qml/SpellingDialog.qml`, `bench/scenarios/orthographe.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_spelling|tst_captures_orthographe'`.

**Preuve visuelle.** `captures.ps1 -Scenario orthographe … -Lire "spellingDialog.word=mosieur" -Lire "spellingDialog.suggestion0=monsieur"` :
`LU … OK`, `LU appSpelling.finished=true OK`, `CAPTURES OK` ; décrire `mot.png` (page 2, mot surligné) et `fin.png`.

**Interdits.** La vérification ne change pas la page en cours d'écriture (elle attend la fin du contact) ; aucun texte
de l'original.

**Terminé quand** les deux fonctions et le scénario passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_spelling.cpp` : `showsThePageOfTheWord` ; `endMessage`.
- [ ] **Étape 2 : implémentation et QML** (`Spelling { id: appSpelling; objectName: "appSpelling"; document: appDocument }`),
  **scénario** (document synthétique de 2 pages, texte « bonjour mosieur » en page 2 et un mot « exemplle » ignoré
  toujours ; `spell.check` ; `expect spellingDialog.word = mosieur` ; capture `mot.png` ; remplacer ; texte relu
  « bonjour monsieur » ; ignorer toujours « exemplle » ; `expect appSpelling.finished = true` ; capture `fin.png`).
- [ ] **Étape 3 : vert**, preuve visuelle, preuve de tâche ; commit « Lot 6 : vérification de l'orthographe (L6-T34) ».

<a id="tache-l6-t35"></a>
### Tâche L6-T35 — Lecteur d'aide hors ligne

**Objectif.** D6-10 : `help.contents` (F1) ouvre `HelpViewer.qml` sur la page de l'outil courant
(`qrc:/actcraft/help/<outil>.html` si elle existe, sinon `index.html`) ; liens internes suivis, liens `http(s)` ouverts
par le navigateur du système, aucun chargement réseau dans le lecteur ; `ui/help/index.html` écrit par nous (sommaire
des fonctions et procédure « mot de passe de la classe oublié »). Les autres pages arrivent au lot 11 (amendement 1,
§ 2.2).

**Fichiers.** `ui/include/actcraft/ui/helpviewer.h`, `ui/src/helpviewer.cpp`, `ui/help/index.html`,
`tests/tst_ui_help.cpp`, `ui/qml/HelpViewer.qml`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_help`.

**Interdits.** Aucun texte, image ni capture de l'aide d'ActivInspire (dépôt public) ; aucun moteur web.

**Terminé quand** les cinq fonctions passent.

**Étapes.**
- [ ] **Étape 1 : en-tête** : `ui::HelpViewer : QObject`, `QML_ELEMENT`, `source` (`QUrl`), `html` (`QString`),
  `Q_INVOKABLE void openFor(QString toolId)`, `Q_INVOKABLE void follow(QUrl link)`.
- [ ] **Étape 2 : test rouge** : `indexLoads` ; `currentToolPageOrIndex` (`tool.pen` sans page → index) ;
  `noNetworkUrl` (`follow("https://…")` ne change pas `source`, émet `externalLink`) ; `internalLinkFollows` ;
  `textsAreOurs` (aucune des chaînes « ActivInspire », « Promethean », « ExpressPoll » dans `index.html`).
- [ ] **Étape 3 : implémentation, page d'index, QML** (`TextArea { readOnly: true; textFormat: TextEdit.RichText }`),
  **vert**, preuve de tâche ; contrôle des titres du corpus ; commit « Lot 6 : lecteur d'aide hors ligne (L6-T35) ».

<a id="tache-l6-t36"></a>
### Tâche L6-T36 — Langue ; scénario `aide-langue`

**Objectif.** D6-11 : `qt_add_translations(actcraft_ui TS_FILES i18n/actcraft_fr.ts …)` (fichier `.ts` produit par
`lupdate` sur les sources et le QML ; français livré : traduction identité) ; `ui::Language` : langues disponibles (les
`.qm` des ressources), réglage `workstation/language` (L6-T5), application au démarrage par `QTranslator` installé dans
`app/main.cpp` **avant** le chargement du QML ; un changement annonce le redémarrage (message) et ne recharge rien. Le
scénario couvre aussi F1 (S9.11).

**Fichiers.** `ui/include/actcraft/ui/language.h`, `ui/src/language.cpp`, `ui/i18n/actcraft_fr.ts`, `ui/CMakeLists.txt`,
`app/main.cpp`, `tests/tst_ui_language.cpp`, `bench/scenarios/aide-langue.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_language|tst_captures_aide_langue'`.

**Preuve visuelle.** `captures.ps1 -Scenario aide-langue … -Lire "helpViewer.visible=true" -Lire "appPreferences.language=fr"` :
`LU … OK`, `CAPTURES OK` ; décrire `aide.png`.

**Interdits.** Aucune traduction automatique livrée ; la pseudo-langue de contrôle est au lot 11 (S29).

**Terminé quand** les cinq fonctions et le scénario passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** : `systemLanguageByDefault` ; `unknownFallsBackToFrench` ; `settingWritten` ;
  `restartAnnounced` (signal `restartNeeded`) ; `translatorInstalledAtStart` (fonction `Language::install(QCoreApplication&,
  Preferences&)` rend vrai pour `fr`).
- [ ] **Étape 2 : implémentation**, `ui/CMakeLists.txt` (appel `qt_add_translations`, `lupdate` cible `update_translations`),
  `app/main.cpp`, **scénario** (`help.contents` → `expect helpViewer.visible = true` → capture `aide.png` → Échap →
  `settings.language fr` → `expect appPreferences.language = fr`).
- [ ] **Étape 3 : vert**, preuve visuelle, preuve de tâche ; commit « Lot 6 : textes traduisibles et réglage de la langue
  (L6-T36) ».

<a id="tache-l6-t37"></a>
### Tâche L6-T37 — Contrôle du lot

**Objectif.** Clore le lot 6 : talons vides, suites debug et release, captures de tous les scénarios hors écran puis à
l'écran en D3D11 (jeton `ecran`, annoncé à Jérôme d'une ligne), banc complet D3D11 plein écran (porte de
non-régression de DA7 : tout critère tenu par la référence l'est encore, sinon tâche de correction avant clôture),
installateur et APK, version **0.7.0** (`project(VERSION)`), section 0.7.0 de `CHANGELOG.md` (en français, à nous),
mesure de fin de lot commitée, graphe.

**Fichiers.** `CMakeLists.txt`, `CHANGELOG.md`, `docs/mesures/2026-10-20-lot6-fin.md`, `graphify-out/`.

**Test et commande bornée.** Preuve de tâche en debug puis `-Preset release` ; captures `-Scenario tous` hors écran,
puis à l'écran (`-Rendu d3d11`, sans `-HorsEcran`) ; banc `pwsh -NoProfile -File ./scripts/bench.ps1` (forme bornée du
profil) ; `scripts/paquet.ps1` (`PAQUET OK`), `scripts/paquet-android.ps1` (`APK OK`).

**Preuve visuelle.** Toutes les lignes `LU` des scénarios du lot en `OK` ; décrire les captures à l'écran de
`reglages-gardes` (S9), `classe-protegee` (S19), `raccourcis-programmes` (S20), `orthographe` (S21) ; capture de l'APK sur
l'émulateur : `launcher.edit` absent des menus.

**Interdits.** Aucune publication (release sous go écrit seulement) ; fermer par PID ce qui a été lancé.

**Terminé quand** le critère de la ligne L6-T37 est tenu.

**Étapes.**
- [ ] **Étape 1 : talons** : `git -C <wt> grep -n "Talon L6 : rempli par" -- . ':(exclude)docs/'` vide ;
  `ctest --preset debug -V | Select-String 'SKIP   :.*Talon L6'` vide.
- [ ] **Étape 2 : suites** debug et release (`TESTS OK` deux fois, N = N6 + 43).
- [ ] **Étape 3 : captures** hors écran puis à l'écran ; **banc** plein écran ; résultats résumés dans
  `docs/mesures/2026-10-20-lot6-fin.md` (chiffres, comparaison à la référence en vigueur, aucune capture commitée).
- [ ] **Étape 4 : paquets** et taille ajoutée (Hunspell, dictionnaire, QtNetwork) rapportée (N1-A10).
- [ ] **Étape 5 : version et notes** : `project(ActCraft VERSION 0.7.0 …)`, section `## 0.7.0` de `CHANGELOG.md`
  (navigateurs, propriétés, profils, réglages, classe protégée, orthographe, aide).
- [ ] **Étape 6 : graphe et commit** : `graphify update .` ; `git add` des fichiers de la ligne ; `NOMS OK` ; contrôle
  des titres du corpus ; commit « Lot 6 : contrôle du lot, version 0.7.0 (L6-T37) ».

