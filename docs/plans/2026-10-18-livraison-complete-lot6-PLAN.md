# Livraison complète — lot 6 « Navigateurs, propriétés, profils », sous-lot 6a — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**But.** Version **0.7.0** (amendement 1, § 6), livrée à la fin du sous-lot 6b
(`docs/plans/2026-10-18-livraison-complete-lot6b-PLAN.md`) : une seule PR, une seule version pour le lot. Le
sous-lot 6a livre la tâche d'ouverture, les réservations, les attributs « verrouillable » et « restreignable » des
commandes, les profils (modèle, magasin, cinq profils fournis à nos noms, enregistrement implicite), la palette
(ancrage, flottante, réduite, réduite au maximum, épinglée, taille des icônes), la palette de couleurs étendue, la
barre de raccourcis (historique, objets glissés), les navigateurs de propriétés, d'objet et de notes, les étiquettes,
et le scénario S9 de bout en bout. Le sous-lot 6b livre le navigateur de ressources, le mode Création, le tableau de
bord, les documents récents et le résumé, l'éditeur de profils (onglets format, commandes, paramètres), le trousseau
et le réseau, les raccourcis vers des programmes, le mode classe protégé, l'orthographe, le lecteur d'aide, la langue
et le contrôle du lot.

Exigences du lot (cadrage, décision 9 ; amendement 1, § 2.1 à § 2.3) : F-030 à F-034, F-090 (sans les entrées
exclues), F-123, F-135, F-140, F-141, F-142, F-297, F-298, F-560, F-570 à F-573, F-575, F-576, F-581, F-583 (page),
F-760 (lecteur d'aide), F-772, F-810 à F-813, F-820, F-821, F-830, F-840, F-880, F-881, F-882, F-900, F-910, F-920,
F-930 (sections des fonctions incluses, langue, réseau), F-935. Constats du corpus : 17 (amendement 1, § 4), relevés par
L6-T0. Écarts : E3 (place du bouton d'édition des raccourcis vers des programmes), E4 (taille de page par défaut).
Principes du lot 10 versés ici : P2 (troisième réglage de qualité du rendu), P10 (regroupement automatique des traits).

**Place dans le chantier.** Le lot 6 s'exécute après L0-S, les lots 3, 7 (session B, fusionné), 4 et 5. Les plans de
ces lots font autorité sur les API ; chaque API qu'ils posent et que ce plan emploie est listée dans la table
« API supposées » (A1 à A16) et **revérifiée par L6-T0** sur le code livré. Écrit sur `794cf84` (fin de L0-S1) : les
numéros de ligne du code sont indicatifs, les noms font foi. Le plan du lot 5 s'écrit en même temps
(`.worktrees/livraison-complete--plan-lot5`) : rien n'y était commité à l'écriture de ce plan ; les fichiers propres
au lot 6 portent des noms neufs (`profile*`, `preferences*`, `propertyedits*`, `objecttree*`, `notes*`,
`resourcelibrary*`, `fragment*`, `launchers*`, `classroomlock*`, `spell*`, `helpviewer*`, `language*`) pour ne
recouper aucun fichier du lot 5 ; les fichiers communs à tous les lots (`commands.json`, `layout.json`, `Main.qml`,
`CMakeLists.txt`) sont repris en série, jamais dans une même vague.

**Pile.** C++20, Qt 6.12 (Core, Gui, Quick, QuickControls2, QuickDialogs2, Svg, Network pour le seul hachage et le
mandataire, Test), CMake + Ninja, MSVC 2022, PowerShell 7. Bibliothèque tierce ajoutée en 6b : Hunspell (L6-T32).

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L6-T0 | Ouverture : vidéos, REA et ancien dépôt confrontés au lot ; API A1 à A16 revérifiées ; N6 relevé | — | `docs/verifications/2026-10-19-lot6-principes.md` (hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot6.md`) | isolé | concepteur-critique | rapport commité, relevé brut hors dépôt présent, décisions D1 à D14 et table A1 à A16 écrites, `git grep` des titres du corpus vide, `NOMS OK`, N6 relevé | S30 | #tache-l6-t0 |
| L6-T1 | Réservation : 43 tests enregistrés, fichiers neufs des deux sous-lots (talons marqués), commandes et disposition du lot, icônes | — | `actions/data/commands.json`, `actions/data/layout.json`, `actions/CMakeLists.txt`, `core/CMakeLists.txt`, `formats/CMakeLists.txt`, `ui/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/{actions,core,formats,ui,bench}.cmake`, `tests/tst_actions_layout.cpp`, `ui/icons/{palette-gauche,palette-droite,palette-haut,palette-bas,palette-flottante,palette-reduire,palette-reduire-tout,palette-epingler,couleurs-plus,pipette,nav-objets-masque,nav-objets-verrou,notes-page,profil,profils,conception,accueil,recents,ressources-miennes,ressources-partagees,ressources-autre,tampon,classe-verrou,orthographe,aide-sommaire,programme}.svg`, `tests/tst_ui_theme.cpp`, talons : `actions/include/actcraft/actions/{profile,profilestore}.h`, `actions/src/{profile,profilestore}.cpp`, `actions/data/profiles/{en-classe,preparation,lecture-ecriture,calcul-geometrie,son-image}.json`, `core/include/actcraft/core/{propertyedits,notesedits,spellcheck}.h`, `core/src/{propertyedits,notesedits,spellcheck,hunspellengine}.cpp`, `formats/include/actcraft/formats/fragment.h`, `formats/src/fragment.cpp`, `ui/include/actcraft/ui/{preferences,profilecontroller,colorpalette,shortcutbar,propertymodel,objecttree,notescontroller,labeltips,resourcelibrary,designmode,recentdocuments,layouteditor,settingsschema,secretstore,networksettings,launchers,classroomlock,spelling,helpviewer,language}.h`, `ui/src/{preferences,profilecontroller,colorpalette,shortcutbar,propertymodel,objecttree,notescontroller,labeltips,resourcelibrary,designmode,recentdocuments,layouteditor,settingsschema,secretstore_win,secretstore_other,networksettings,launchers,classroomlock,spelling,helpviewer,language}.cpp`, `ui/help/index.html`, `ui/i18n/actcraft_fr.ts`, `tests/launcher_probe.cpp`, 28 tests talons `tests/tst_{actions_profile,actions_lockable,ui_preferences,ui_profiles,ui_palette,ui_colors,ui_shortcutbar,formats_lot6,core_propertyedits,ui_propertymodel,ui_objecttree,core_notesedits,ui_notes,ui_labels,formats_fragment,ui_resources,ui_designmode,ui_recents,ui_profileeditor,ui_layouteditor,ui_settingsschema,ui_network,ui_launchers,ui_classroomlock,core_spellcheck,ui_spelling,ui_help,ui_language}.cpp`, 15 scénarios talons `bench/scenarios/{palette,couleurs,navigateur-objets,proprietes,notes,etiquettes,reglages-gardes,ressources,mode-creation,tableau-de-bord,profils,raccourcis-programmes,classe-protegee,orthographe,aide-langue}.json` | isolé | implementeur-tdd | `100% tests passed … out of N`, N = N6 + 43, `TESTS OK`, `tst_actions_layout` (dont `lot6EntriesDeclared`, `noShortcutTwice`) Passed, 43 tests neufs `SKIP` nommant leur tâche ; `premiers-pas` hors écran `CAPTURES OK` ; `NOMS OK` | S9, S19, S20, S21 | #tache-l6-t1 |
| L6-T2 | Coquille QML du lot : emplacements de la palette, de la barre de raccourcis, des quatre navigateurs, des fenêtres du lot ; gestionnaires des commandes | L6-T1 | `ui/qml/Main.qml`, `ui/qml/ToolPalette.qml`, `ui/qml/BrowserPanel.qml`, `ui/qml/TopBar.qml`, `ui/qml/{PaletteFrame,ShortcutBar,ExtendedColors,ColorLoupe,ObjectBrowser,PropertyBrowser,PropertyEditor,NotesBrowser,ResourceBrowser,LabelTip,Dashboard,ProfileChooser,ProfileEditor,FormatPage,CommandsPage,LaunchersPage,SettingsPage,NetworkPage,ClassroomLockDialog,SpellingDialog,HelpViewer,SaveSummaryDialog}.qml`, `tests/tst_ui_shell.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N6 + 43, `tst_ui_shell` (dont `lot6ShellPlaceholders`, `everyLot6CommandHasAHandler`, `browserTabsInOrder`) Passed ; `premiers-pas` hors écran `CAPTURES OK` ; banc hors écran `content_check.ok = True` | S9 | #tache-l6-t2 |
| L6-T3 | `actions` : attributs `lockable` et `restrictable`, porte du verrou dans le registre ; remplis pour toutes les commandes présentes (lots 1 à 5, 7, socle) | L6-T1 | `actions/include/actcraft/actions/commandregistry.h`, `actions/src/commandregistry.cpp`, `actions/data/commands.json`, `tests/tst_actions_lockable.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_actions_lockable` Passed (dont `attributesRead`, `lockGreysOnlyLockable`, `lockedTriggerCallsNothing`, `enableCannotBypassTheLock`, `everyCommandDeclaresBoth`, `fileAndQuitStayFree`), `tst_actions_registry` Passed ; mutation de la porte (`isEnabled` sans le verrou) rouge puis remise | S19 | #tache-l6-t3 |
| L6-T4 | `actions` : modèle de profil `actcraft-profile/1` et magasin (fournis, utilisateur, enregistrer sous, supprimer, réinitialiser, exporter, importer) | L6-T1 | `actions/include/actcraft/actions/profile.h`, `actions/src/profile.cpp`, `actions/include/actcraft/actions/profilestore.h`, `actions/src/profilestore.cpp`, `tests/tst_actions_profile.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_actions_profile` Passed (dont `roundTripKeepsEveryField`, `unknownKeysKept`, `majorVersionRefused`, `iconPixelsClamped`, `builtinFirstThenUser`, `userCopyShadowsBuiltin`, `resetRemovesTheUserCopy`, `builtinCannotBeRemoved`, `saveIsAtomic`, `exportImportRoundTrip`, `importNeverOverwrites`) | S9, S9.7 | #tache-l6-t4 |
| L6-T5 | `ui` : réglages du poste (`Preferences`, QSettings) et `ProfileController` (profil courant, enregistrement implicite différé hors contact, dossier de disposition) | L6-T3, L6-T4 | `ui/include/actcraft/ui/preferences.h`, `ui/src/preferences.cpp`, `ui/include/actcraft/ui/profilecontroller.h`, `ui/src/profilecontroller.cpp`, `app/main.cpp`, `tests/tst_ui_preferences.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_ui_preferences` Passed (dont `keysAreOurs`, `currentProfileRestored`, `implicitSaveIsDebounced`, `noSaveDuringContact`, `activateSwitchesTheLayoutDir`, `brokenProfileFallsBack`, `settingDefaultsComeFromTheSchema`) | S9, S9.7 | #tache-l6-t5 |
| L6-T6 | Cinq profils fournis, à nos noms, avec les épinglages du lot 7 et du lot 8 annoncé | L6-T4, L6-T5 | `actions/data/profiles/{en-classe,preparation,lecture-ecriture,calcul-geometrie,son-image}.json`, `tests/tst_ui_profiles.cpp`, `bench/scenarios/profils.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_profiles` (dont `fiveBuiltinsInOrder`, `pinnedCommandsExist`, `pinnedFilteredByPlatform`, `namesAreOurs`, `preparationOpensThePageBrowser`), `tst_captures_profils` Passed ; `captures.ps1 -Scenario profils` : `LU appProfile.currentId=calcul-geometrie OK`, `CAPTURES OK` | S9.7, S32 | #tache-l6-t6 |
| L6-T7 | Palette : ancrage (4 bords, flottante), réduite, réduite au maximum, épinglée (repli au retrait du pointeur), taille des icônes 16 à 32 ; persistance | L6-T2, L6-T5 | `ui/qml/PaletteFrame.qml`, `ui/qml/ToolPalette.qml`, `ui/qml/Theme.qml`, `tests/tst_ui_palette.cpp`, `bench/scenarios/palette.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_palette` (dont `dockFollowsTheProfile`, `dragToCenterFloats`, `collapsedShowsFirstRowOnly`, `fullCollapseHidesShortcuts`, `unpinnedHidesAfterLeave`, `iconPixelsScaleEveryIcon`), `tst_captures_palette` Passed ; `captures.ps1 -Scenario palette` : `LU appProfile.paletteDock=left OK`, `LU toolPalette.x=0 OK`, `CAPTURES OK` ; banc hors écran joint | S9, S9.1 | #tache-l6-t7 |
| L6-T8 | Palette de couleurs étendue (6 × 6), copieur de couleur, boîte de couleur, couleurs ajoutées gardées ; 8, 16 ou 24 pastilles | L6-T2, L6-T5 | `ui/include/actcraft/ui/colorpalette.h`, `ui/src/colorpalette.cpp`, `ui/qml/ExtendedColors.qml`, `ui/qml/ColorLoupe.qml`, `ui/qml/ColorSwatches.qml`, `tests/tst_ui_colors.cpp`, `bench/scenarios/couleurs.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_colors` (dont `extendedGridIsSixBySix`, `addedColorIsKept`, `duplicatesIgnored`, `swatchCountFollowsTheProfile`, `loupeSamplesOnceOnOpen`), `tst_captures_couleurs` Passed ; `captures.ps1 -Scenario couleurs` : `LU appColors.extraCount=1 OK`, `PIXEL pastille-ajoutee OK`, `CAPTURES OK` | S9, S9.2 | #tache-l6-t8 |
| L6-T9 | Barre de raccourcis : historique des outils hors palette (F-140), objets glissés devenus boutons (F-141) | L6-T7 | `ui/include/actcraft/ui/shortcutbar.h`, `ui/src/shortcutbar.cpp`, `ui/qml/ShortcutBar.qml`, `tests/tst_ui_shortcutbar.cpp` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_shortcutbar` Passed (dont `outOfPaletteToolIsRecorded`, `paletteToolIsNotRecorded`, `historyIsBoundedAndMovesToFront`, `droppedObjectBecomesAButton`, `clickInsertsACopyAtTheCenter`, `insertIsUndoneAtOnce`) ; banc hors écran joint | S9, S9.1 | #tache-l6-t9 |
| L6-T10 | `core` + `formats` : mots-clés (objet, page), description de page, style d'étiquette ; format natif sans perte, empreintes inchangées sans ces champs | L6-T0, L6-T1 | `core/include/actcraft/core/model.h`, `core/src/model.cpp`, `formats/src/nativejson.cpp`, `tests/tst_core_model.cpp`, `tests/tst_formats_lot6.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_formats_lot6` (dont `keywordsRoundTrip`, `pageDescriptionRoundTrip`, `labelStyleRoundTrip`, `defaultsAreOmitted`, `olderFileReadsWithDefaults`), `tst_core_model` (dont `lot6FieldsKeepOldFingerprints`), `tst_formats_nativejson` Passed | S9.4, S9.10 | #tache-l6-t10 |
| L6-T11 | `core` : `SetObjectPropertiesCommand` (une étape, plusieurs objets, fusion des glissés) et `SetPagePropertiesCommand` | L6-T10 | `core/include/actcraft/core/propertyedits.h`, `core/src/propertyedits.cpp`, `tests/tst_core_propertyedits.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_core_propertyedits` Passed (dont `oneStepForManyObjects`, `undoRestoresEachOldValue`, `sliderDragMergesIntoOneStep`, `differentPropertyDoesNotMerge`, `nameOnlyChangeSparesTheCache`, `layerChangeMovesTheObject`, `pageWidthChangeIsOneStep`) ; mutation de `mergeWith` rouge puis remise | S9, S9.4 | #tache-l6-t11 |
| L6-T12 | `ui` : `PropertyModel` (page, un objet, plusieurs objets ; sections F-570 à F-576, F-581, F-583 ; valeurs communes ; éditeurs typés) | L6-T11 | `ui/include/actcraft/ui/propertymodel.h`, `ui/src/propertymodel.cpp`, `tests/tst_ui_propertymodel.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_ui_propertymodel` Passed (dont `pageWhenNothingSelected`, `sectionsOfAStroke`, `sectionsOfAShape`, `commonValuesOnly`, `editGoesThroughTheUndoStack`, `refreshIsDeferredDuringContact`, `noCacheRefreshOnSelectionChange`) ; banc hors écran joint | S9, S9.4 | #tache-l6-t12 |
| L6-T13 | QML : navigateur de propriétés (sections repliables, éditeurs) ; scénario `proprietes` | L6-T2, L6-T12 | `ui/qml/PropertyBrowser.qml`, `ui/qml/PropertyEditor.qml`, `bench/scenarios/proprietes.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_captures_proprietes` Passed ; `captures.ps1 -Scenario proprietes` : `LU appProperties.title=Trait OK`, `LU appDocument.canUndo=true OK`, `PIXEL contour-epais OK`, `PIXEL contour-annule OK`, `CAPTURES OK` ; banc hors écran joint | S9, S9.4 | #tache-l6-t13 |
| L6-T14 | `ui` : `ObjectTreeModel` (couches, ordre de superposition, clic qui sélectionne, glisser qui réordonne, masqué, verrouillé), mises à jour regroupées | L6-T0, L6-T1 | `ui/include/actcraft/ui/objecttree.h`, `ui/src/objecttree.cpp`, `tests/tst_ui_objecttree.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_ui_objecttree` Passed (dont `fourLayerNodesTopFirst`, `objectsTopmostFirst`, `clickSelects`, `dragReordersInOneStep`, `dragToBackgroundRefused`, `toggleHiddenAndLocked`, `resetsAreBatched`, `noRefreshDuringContact`) ; banc hors écran joint | S9, S9.3 | #tache-l6-t14 |
| L6-T15 | QML : navigateur d'objet ; scénario `navigateur-objets` | L6-T2, L6-T14 | `ui/qml/ObjectBrowser.qml`, `bench/scenarios/navigateur-objets.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_captures_navigateur_objets` Passed ; `captures.ps1 -Scenario navigateur-objets` : `LU objectBrowser.visible=true OK`, `LU appObjectTree.rowCount=6 OK`, `LU appSelection.count=1 OK`, `CAPTURES OK` ; banc hors écran joint | S9, S9.3 | #tache-l6-t15 |
| L6-T16 | Navigateur de notes : `SetPageNotesCommand` (fusionnée), éditeur riche, icône de notes dans la ligne supérieure | L6-T2 | `core/include/actcraft/core/notesedits.h`, `core/src/notesedits.cpp`, `tests/tst_core_notesedits.cpp`, `ui/include/actcraft/ui/notescontroller.h`, `ui/src/notescontroller.cpp`, `tests/tst_ui_notes.cpp`, `ui/qml/NotesBrowser.qml`, `bench/scenarios/notes.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_core_notesedits` (dont `typingMergesIntoOneStep`, `notesTouchNoCache`), `tst_ui_notes` (dont `followsTheCurrentPage`, `hasNotesFlag`), `tst_captures_notes` Passed ; `captures.ps1 -Scenario notes` : `LU appNotes.hasNotes=true OK`, `LU notesIcon.visible=true OK`, `CAPTURES OK` | S9, S9.5 | #tache-l6-t16 |
| L6-T17 | Étiquettes : légende d'un objet montrée au survol, selon son style | L6-T10, L6-T2 | `ui/include/actcraft/ui/labeltips.h`, `ui/src/labeltips.cpp`, `ui/qml/LabelTip.qml`, `tests/tst_ui_labels.cpp`, `bench/scenarios/etiquettes.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_labels` (dont `hoverShowsTheLabel`, `noLabelNoTip`, `hiddenObjectHasNoTip`, `styleIsApplied`), `tst_captures_etiquettes` Passed ; `captures.ps1 -Scenario etiquettes` : `LU labelTip.text=Soleil OK`, `CAPTURES OK` | S9.10 | #tache-l6-t17 |
| L6-T18 | Point d'étape 6a : scénario S9 `reglages-gardes` (quitter, relancer), comptes des talons, graphe | L6-T6 à L6-T9, L6-T13, L6-T15 à L6-T17 | `bench/scenarios/reglages-gardes.json`, `bench/src/uiscenario.cpp`, `bench/include/actcraft/bench/uiscenario.h`, `tests/tst_bench_uiscenario.cpp`, `graphify-out/` | partagé | implementeur-tdd | `TESTS OK`, N = N6 + 43, `tst_bench_uiscenario` (dont `restartStepParsed`), `tst_captures_reglages_gardes` Passed ; `captures.ps1 -Scenario reglages-gardes` : `LU appProfile.paletteDock=left OK` **après** l'étape `restart`, `LU appColors.extraCount=1 OK`, `LU browserPanel.current=objects OK`, `CAPTURES OK` ; talons « par L6-T2 » à « par L6-T17 » absents ; `NOMS OK` ; `graphify update .` commité | S9 | #tache-l6-t18 |

19 tâches dans ce sous-lot (19 dans 6b : 38 pour le lot). Les tâches de principes que L6-T0 demanderait (au plus 2,
amendement 1, § 4) s'ajoutent par amendement du plan, comme au lot 7 ; elles prennent les identifiants `L6-P1`,
`L6-P2`. Tests ctest : L6-T1 enregistre d'un coup les **43** tests des deux sous-lots (28 unitaires, 15 de captures),
talons `QSKIP` jusqu'à ce que leur tâche les remplisse : **N6 + 43** dès L6-T1, où N6 est le nombre de tests au départ
du lot (relevé par L6-T0). Aucune autre tâche n'en enregistre. Un talon sauté ne prouve rien : chaque preuve nomme les
fonctions attendues `PASS`.

Worktrees : `.worktrees/livraison-complete--L6-T<n>`, branche `feat/livraison-complete--L6-T<n>`, issue de
`feat/livraison-complete` à jour des tâches déjà mergées. Tous isolés sauf L6-T18 (point d'étape, sur le worktree du
chantier).

**Écran.** Aucune tâche de 6a n'ouvre de fenêtre visible : tout se joue hors écran (rendu logiciel). La clôture du
lot (L6-T37, fin de 6b) joue les captures et le banc à l'écran sous le jeton `ecran`, annoncés à Jérôme d'une ligne.

## Vagues (largeur 3 au plus ; 2 places de tests)

V1 : L6-T0 ‖ L6-T1 · V2 : L6-T2 ‖ L6-T3 ‖ L6-T4 · V3 : L6-T5 ‖ L6-T10 ‖ L6-T14 · V4 : L6-T6 ‖ L6-T7 ‖ L6-T11 ·
V5 : L6-T8 ‖ L6-T12 ‖ L6-T15 · V6 : L6-T9 ‖ L6-T13 ‖ L6-T16 · V7 : L6-T17 · V8 : L6-T18 · puis 6b (son plan).

Vérification des recoupements (colonne « fichiers touchés ») :
- V1 : L6-T0 (`docs/verifications/`) / L6-T1 (CMake, données, icônes, talons) — disjoints.
- V2 : L6-T2 (QML de la coquille, `tst_ui_shell.cpp`) / L6-T3 (`commandregistry.*`, `commands.json`,
  `tst_actions_lockable.cpp`) / L6-T4 (`profile.*`, `profilestore.*`, `tst_actions_profile.cpp`) — disjoints.
- V3 : L6-T5 (`preferences.*`, `profilecontroller.*`, `app/main.cpp`, un test) / L6-T10 (`model.*`,
  `nativejson.cpp`, deux tests) / L6-T14 (`objecttree.*`, un test) — disjoints.
- V4 : L6-T6 (`actions/data/profiles/*.json`, un test, `profils.json`) / L6-T7 (`PaletteFrame.qml`, `ToolPalette.qml`,
  `Theme.qml`, un test, `palette.json`) / L6-T11 (`propertyedits.*`, un test) — disjoints.
- V5 : L6-T8 (`colorpalette.*`, `ExtendedColors.qml`, `ColorLoupe.qml`, `ColorSwatches.qml`, un test, `couleurs.json`)
  / L6-T12 (`propertymodel.*`, un test) / L6-T15 (`ObjectBrowser.qml`, `navigateur-objets.json`) — disjoints.
- V6 : L6-T9 (`shortcutbar.*`, `ShortcutBar.qml`, un test) / L6-T13 (`PropertyBrowser.qml`, `PropertyEditor.qml`,
  `proprietes.json`) / L6-T16 (`notesedits.*`, `notescontroller.*`, `NotesBrowser.qml`, deux tests, `notes.json`) —
  disjoints.
- Fichiers repris d'une vague à l'autre, tenus par « dépend de » : talons de L6-T1 → leur tâche ; `commands.json`
  (L6-T1 → L6-T3) ; `ToolPalette.qml` (L6-T2 → L6-T7) ; QML des emplacements (L6-T2 → leur tâche).

**Places.** Le jeton `tests` a 3 places, dont **2 pour ce lot** (les revues statiques tournent à côté). Une vague de
3 tâches qui construisent en a toujours une qui attend la notification du jeton : c'est prévu, l'agent attend, il ne
sonde pas. L6-T0 ne construit rien. Ordre de lancement dans une vague : la tâche du chemin critique d'abord (V3 :
L6-T5 ; V4 : L6-T11 ; V5 : L6-T12).

**L6-T0 et les autres tâches.** L6-T1 (données, mécanique) ne l'attend pas ; toutes les tâches de comportement
l'attendent (V2 commence quand V1 est finie). Ses décisions D1 à D14 et sa table A1 à A16 **amendent ce plan et celui
de 6b** avant V2 (relecture du plan exigée). Une décision qui change une donnée de commande (raccourci, libellé,
ordre) passe par un correctif `L6-C0` (`actions/data/commands.json`, `actions/data/layout.json` seulement), joué seul
avant V2. Une API supposée qui diffère du code livré amende la section de la tâche qui l'emploie.

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
  Attendu : `100% tests passed, 0 tests failed out of N` (N = N6 + 43 dès L6-T1), `TESTS OK`, puis la ligne `Passed`
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

## Décisions appliquées (tranchées par ce plan, amendement 1 faisant autorité ; L6-T0 peut les amender)

- **D6-1 — Deux magasins, deux portées.** Ce qui suit l'enseignant d'un poste à l'autre (F-881 : profils
  transportables) est dans **un fichier à nous par profil**, `actcraft-profile/1` (JSON, clés anglaises en
  `kebab-case`), rangé sous `<AppConfigLocation>/profiles/<id>.json` ; sa disposition éditée (F-910) dans
  `<AppConfigLocation>/profiles/<id>/layout.json`, lue par le mécanisme existant (`Commands::setProfileDir`,
  `actions::readLayeredFile`, N2-5). Ce qui est propre au poste (profil courant, documents récents, tableau de bord au
  démarrage, empreinte du mot de passe de classe, mandataire, clé de signature des raccourcis) est dans `QSettings`
  (format natif : registre sous Windows, fichier sous Android et macOS). `QStandardPaths::AppConfigLocation` et
  `QSettings` fonctionnent sous Android (dossier privé de l'application) : aucune permission. Clés `QSettings` en
  `kebab-case` à nous, groupées (`workstation/…`, `classroom/…`, `network/…`) ; un identifiant `kebab-case` se découpe
  en mots simples, ce qui tient le contrôle `NOMS OK` (aucun nom composé ne peut coïncider).
- **D6-2 — Profils fournis à nos noms (N2-6 amendée).** Cinq profils, repris dans leur usage : `en-classe` « En
  classe » (présenter : page ajustée, navigateurs masqués, corbeille visible, épinglages de présentation),
  `preparation` « Préparer un cours » (navigateur de pages ouvert et épinglé, palette de couleurs étendue — profil par
  défaut), `lecture-ecriture` « Lire et écrire » (clavier à l'écran, reconnaissance d'écriture, message défilant,
  orthographe), `calcul-geometrie` « Calcul et géométrie » (outils de maths, dés, calculatrice), `son-image` « Son et
  image » (caméra, médias et enregistreurs annoncés au lot 8). Les épinglages viennent de la liste écrite par L7-T32
  (`docs/verifications/*-lot7-epinglages.md`) ; ceux du lot 8 y sont ajoutés par le lot 8 (règle de l'amendement) ;
  une commande épinglée absente de la plateforme ou pas encore déclarée est filtrée à l'affichage, jamais une erreur.
- **D6-3 — Épinglages sans recopier la disposition.** Un profil porte une liste `pinned` d'identifiants de
  commandes, affichée comme groupe « profil » à la fin de la palette ; la disposition par défaut reste unique
  (`actions/data/layout.json`) ; seul l'onglet Commandes (F-910, L6-T26) écrit un `layout.json` propre au profil.
- **D6-4 — Enregistrement implicite (F-881) hors écriture.** Toute modification d'interface passe par
  `ProfileController`, qui enregistre 500 ms après le dernier changement, et jamais pendant un contact (la minuterie
  est reportée tant que `inputActive` est vrai) ; écriture atomique (`QSaveFile`).
- **D6-5 — Navigateurs et cache (règle du lot 10).** Les modèles des navigateurs (objets, propriétés, notes,
  ressources) observent les modifications versées par l'observateur unique (`DocumentController::addChangeObserver`,
  D-S7) et se **regroupent** : une seule remise à jour par tour de boucle, reportée tant qu'un contact est en cours
  (même garde que les miniatures du lot 4, A6). Aucun ne demande de redessin du cache ; une sélection depuis un
  navigateur ne modifie pas le document. Les vignettes de ressources se rendent sur `QThreadPool`, jamais sur le fil
  principal.
- **D6-6 — Une commande générique de propriétés.** Toute modification du navigateur de propriétés passe par
  `core::SetObjectPropertiesCommand` (ou `SetPagePropertiesCommand`) : une étape par modification, plusieurs objets
  d'un coup, glissés d'une réglette fusionnés (même propriété, mêmes objets, `mergeId` dédié). Les `Change` émis
  suivent le contrat de L0-S3 : nom, mots-clés, légende et description n'atteignent pas le cache ; apparence,
  position, couche le redessinent.
- **D6-7 — Verrou de classe sans contournement (F-298).** Chaque commande déclare `lockable` et `restrictable`
  (amendement 1, § 1). Le **registre** porte la porte du verrou : `isEnabled` et `trigger` refusent une commande
  verrouillable quand le verrou est posé, quelle que soit la valeur de `setEnabled` ; aucun gestionnaire ne peut la
  rouvrir. Restent libres : Fichier ▸ Quitter, déverrouiller, aide, navigation de page (liste tranchée par L6-T0, D3).
  Mot de passe haché avec sel : PBKDF2-SHA256 (`QPasswordDigestor`, QtNetwork, LGPLv3), 200 000 itérations, sel de 16
  octets ; aucun mot de passe par défaut (N1-A5).
- **D6-8 — Trousseau du système sans bibliothèque tierce.** Les deux secrets du lot (mot de passe du mandataire, clé
  HMAC des raccourcis) sont Windows seulement (amendement 1 : réseau et raccourcis vers des programmes au bureau
  Windows). Ils vont dans le Gestionnaire d'identification de Windows par l'API `CredWriteW` / `CredReadW`
  (`advapi32`, `ui/src/secretstore_win.cpp`) ; ailleurs, `secretstore_other.cpp` répond « indisponible » et les
  commandes qui en dépendent sont absentes. Réalisation plus moderne retenue (décision 0100, complément) : aucune
  bibliothèque de trousseau à ajouter, N1-A10 allégée.
- **D6-9 — Orthographe par Hunspell.** Hunspell 1.7.2 (triple licence MPL 1.1 / GPL 2 / LGPL 2.1, prise en LGPL 2.1)
  compilé dans `third_party/hunspell`, dictionnaire français de Grammalecte (MPL 2.0) dans
  `third_party/dictionaries/fr` ; même moteur sous Windows et Android. Le téléchargement des deux sources demande
  l'accord de Jérôme (hypothèse H6-6) ; sans accord, L6-T32 à L6-T34 attendent et le reste du lot continue.
- **D6-10 — Aide hors ligne.** Pages HTML à nous dans les ressources (`ui/help/`), lues par un `TextArea` en texte
  riche (pas de moteur web) ; F1 ouvre la page de l'outil courant ; la page du projet passe dans le menu Aide sans
  raccourci. Les pages elles-mêmes sont au lot 11 ; le lot 6 livre le lecteur et la page d'index.
- **D6-11 — Langue.** `qt_add_translations` (fichiers `ui/i18n/actcraft_<code>.ts`), français livré, réglage
  `workstation/language` (défaut : langue du système si un fichier existe, sinon français), appliqué au redémarrage
  (comme la référence), message en ce sens.
- **D6-12 — Mode Création (F-560) est un état, pas un outil.** `ui::DesignMode` porte la bascule (F2) et ses neuf
  réglages ; les lots déjà livrés lisent les réglages qui les concernent (objets verrouillés modifiables, objets masqués
  visibles, accrochage désactivé, effets de page suspendus) ; les réglages des actions et des restrictions sont
  publiés pour le lot 8, qui les lit.
- **D6-13 — Taille de page par défaut (E4).** Réglage `defaults/page-size` du profil : 1024 × 768 (décision 15) tant
  que L6-T0 n'a pas tranché ; le profil `en-classe` prend la taille de l'écran si L6-T0 le confirme sur la 3.5.18.
- **D6-14 — Ressources : notre format.** Une ressource ajoutée à la bibliothèque (F-813) est un `.actcraft` d'une
  page qui contient les objets et leurs ressources (images par empreinte), nommé `Ressource<N>.actcraft` ; les images
  et fonds sont aussi lus tels quels (`.png`, `.jpg`, `.svg`). Aucun pack de l'original n'est lu (imports en attente,
  amendement 1, § 5). Dossiers : « Mes ressources » = `Documents/ActCraft/Ressources` (Android : dossier de
  l'application), « Ressources partagées » = dossier réglable (défaut : `<AppDataLocation>/ressources-partagees`),
  « Autre dossier » = choisi par l'utilisateur.

## API supposées (posées par les lots précédents ; L6-T0 les revérifie une par une)

| # | API | Lot qui la pose | Employée par | À revérifier |
|---|---|---|---|---|
| A1 | `ui::Commands::setProfileDir(QString)`, `reload()`, `actions::readLayeredFile` | lot 1 (vérifié sur `794cf84`) | L6-T5, L6-T26 | non |
| A2 | `actions::CommandInfo`, `CommandRegistry::loadDeclarations`, `isEnabled`, `trigger` | lot 1 (vérifié) | L6-T3 | non |
| A3 | `ui::DocumentController::addChangeObserver(std::function<void(const core::Changes&)>)`, `removeChangeObserver` | L0-S5 | L6-T12, L6-T14, L6-T16, L6-T20 | oui |
| A4 | `DocumentController::execute(std::unique_ptr<core::UndoCommand>)`, `document()`, `currentPageIndex()`, signal `currentPageChanged` | L0-S5, lot 2 | L6-T9, L6-T11 à L6-T16, L6-T19, L6-T21 | oui |
| A5 | sélection : `tools::UserState::selection` (identifiants), `ui::ToolHost::user()`, `ToolHost::setSelection(QList<quint64>)`, signal `selectionChanged` | L0-S6, lot 3 | L6-T12, L6-T14, L6-T15 | oui |
| A6 | garde « pas pendant un contact » : `ui::PageView::inputActive` (propriété) ou équivalent posé par `PageThumbnails` (L4-T17) | lot 4 | L6-T5, L6-T12, L6-T14, L6-T20 | oui |
| A7 | `PageView::addInputCover(QQuickItem*)` | L0-S7 | L6-T7, L6-T8, L6-T21 | oui |
| A8 | `BrowserPanel.qml` : propriété `current` (`pages`, `resources`, `objects`, `notes`, `properties`, `actions`), côté, largeur, `view.browsers` | L4-T19 | L6-T2, L6-T15, L6-T18 | oui |
| A9 | arrangement et drapeaux : commande de rang dans la couche et de changement de couche, commande « masquer » et « verrouiller » | lot 5 | L6-T14 | oui (noms inconnus à l'écriture) |
| A10 | copie d'objets : sérialisation d'une liste d'objets avec leurs ressources (presse-papiers du lot 5) | lot 5 | L6-T9, L6-T19 | oui |
| A11 | insertion d'image : commande `insert.image <url>` | lot 5 | L6-T21 | oui |
| A12 | `core::ReplaceObjectsCommand`, contrat de `Change` (`ObjectChanged`, `PageChanged`, `PageSettingsChanged`) | L0-S3 | L6-T10, L6-T11, L6-T16 | oui |
| A13 | `render::thumbnailPng(page, width)` ou peintre partagé `paintPage` hors fil principal | lots 2 et 4 | L6-T20 | oui |
| A14 | joueur de scénarios : étapes `command`, `expect`, `pointer`, `typeText`, `waitReady` | lot 1, L0-S2, L4-T14 | L6-T7 à L6-T18 | oui |
| A15 | commandes du lot 7 (`math.*`, `present.*`, `tool.dice`, `tool.calculator`, `tool.clock`, `desktop.*`, `camera.*`) et fichier des épinglages de L7-T32 | lot 7 | L6-T6 | oui |
| A16 | `ui::DocumentIo` : enregistrement (signal de fin d'enregistrement avec le chemin), ouverture par URL | lot 2 | L6-T23, L6-T24 | oui |

## Talons : règle d'honnêteté

1. **Marqueur unique** `Talon L6 : rempli par <tâche>.` — en commentaire C++ dans chaque `.h`/`.cpp` talon, en
   commentaire QML dans chaque `.qml` talon, dans le champ `description` de chaque scénario JSON talon, et comme texte
   du `QSKIP` de chaque test talon : `QSKIP("Talon L6 : rempli par L6-T4.");`. `<tâche>` est **une** tâche de ce plan
   ou de celui de 6b.
2. Un test talon n'a qu'une fonction sautée, `reserved()` ; la tâche propriétaire la supprime et ajoute les siennes.
3. Compteur : N = N6 + 43 dès L6-T1 ; chaque preuve donne N exact **et** les fonctions attendues en `PASS`.
4. Contrôle (L6-T18 pour 6a, L6-T37 pour le lot) : `git -C <wt> grep -n "Talon L6 : rempli par" -- . ':(exclude)docs/'`
   ne rend plus que des tâches de 6b à la fin de 6a, et rien à la fin du lot ; sur la sortie de
   `ctest --preset debug -V`, aucune ligne `SKIP   :` qui contienne `Talon L6`.

## Scénarios

Scénarios du cadrage et de l'amendement servis par ce lot : **S9** (réglages gardés, cadrage), **S19** (mode classe
protégé), **S20** (raccourci vers un programme), **S21** (orthographe), **S30** (ouverture), **S32** (préparé : les
profils fournis et leurs épinglages ; contrôle au lot 11). Sous-scénarios de S9 posés par ce plan :

- **S9.1 — Palette.** Ancrer à gauche, réduire, épingler puis désépingler (la palette se replie quand le pointeur
  s'éloigne et revient au survol), taille d'icônes 32 : chaque icône mesure 32 px ; un outil hors palette utilisé
  apparaît dans la barre de raccourcis ; réduire au maximum masque la barre.
- **S9.2 — Couleurs.** Clic droit sur une pastille : grille de 36 couleurs ; une couleur choisie s'ajoute à la
  palette et reste après relance ; le copieur de couleur prend la couleur sous le pointeur.
- **S9.3 — Navigateur d'objet.** Quatre nœuds de couche, objets du plus haut au plus bas ; clic : l'objet est
  sélectionné ; glisser une ligne en dessous d'une autre : l'ordre change en une étape d'annulation ; masquer et
  verrouiller par les colonnes.
- **S9.4 — Propriétés.** Un trait sélectionné : sections Identification, Apparence, Contour, Position ; largeur de
  contour 4 → 12 : le trait s'épaissit (pixels) ; Annuler : il revient (pixels) ; rien de sélectionné : propriétés de
  la page.
- **S9.5 — Notes.** Taper une note sur la page 2 : l'icône de notes apparaît dans la ligne supérieure sur la page 2,
  pas sur la page 1 ; la note survit à l'aller-retour `.actcraft`.
- **S9.6 — Ressources** (6b). Ajouter un objet de la page à « Mes ressources » : `Ressource1.actcraft` apparaît ; le
  glisser sur une autre page l'insère ; tampon actif : trois clics, trois copies.
- **S9.7 — Profils.** Changer de profil (F-123) : palette et navigateurs suivent ; enregistrer sous, supprimer,
  réinitialiser un profil fourni ; exporter puis importer sur un profil vierge.
- **S9.8 — Mode Création** (6b). F2 : les objets masqués apparaissent en translucide, un objet verrouillé se
  déplace ; F2 : retour.
- **S9.9 — Accueil, récents, résumé** (6b). Au démarrage, le tableau de bord liste les documents récents ; ouvrir le
  premier ; F11 le rouvre ; réglage « résumé à l'enregistrement » : la boîte de résumé compte pages et objets.
- **S9.10 — Étiquettes.** Un objet légendé « Soleil » : survol → étiquette au style réglé ; sans légende, rien.
- **S9.11 — Aide et langue** (6b). F1 ouvre l'aide hors ligne sur la page de l'outil courant, sans réseau ; changer
  la langue écrit le réglage et annonce le redémarrage.
- **S9.12 — Réseau** (6b, Windows). Régler un mandataire : `QNetworkProxy::applicationProxy()` le reprend ; le mot de
  passe n'est pas dans `QSettings` (lecture du fichier ou du registre) mais dans le trousseau.

| Scénario | Tâches (6a et 6b) |
|---|---|
| S9 | L6-T1, L6-T2, L6-T4, L6-T5, L6-T7 à L6-T9, L6-T11 à L6-T16, L6-T18, L6-T37 |
| S9.1 | L6-T7, L6-T9 |
| S9.2 | L6-T8 |
| S9.3 | L6-T14, L6-T15 |
| S9.4 | L6-T10 à L6-T13 |
| S9.5 | L6-T16 |
| S9.6 | L6-T19 à L6-T21 |
| S9.7 | L6-T4 à L6-T6, L6-T25, L6-T26 |
| S9.8 | L6-T22 |
| S9.9 | L6-T23, L6-T24 |
| S9.10 | L6-T10, L6-T17 |
| S9.11 | L6-T35, L6-T36 |
| S9.12 | L6-T27, L6-T28 |
| S19 | L6-T1, L6-T3, L6-T31, L6-T37 |
| S20 | L6-T1, L6-T29, L6-T30 |
| S21 | L6-T1, L6-T32 à L6-T34 |
| S30 | L6-T0 |
| S32 (préparé) | L6-T6 |

## Hypothèses pour Jérôme (créées par ce plan)

- **H6-1 (N2-6 amendée)** Noms des cinq profils à nous : « En classe », « Préparer un cours », « Lire et écrire »,
  « Calcul et géométrie », « Son et image » ; « Préparer un cours » est le profil par défaut.
- **H6-2** Profils dans des fichiers JSON à nous, transportables ; le reste (profil courant, récents, verrou de
  classe, mandataire) reste sur le poste.
- **H6-3** Trousseau de Windows par son API, sans bibliothèque tierce ; réseau et raccourcis vers des programmes
  absents d'Android, comme l'amendement le prévoit.
- **H6-4** Ressources ajoutées à la bibliothèque au format `.actcraft` d'une page ; aucun pack de l'original lu.
- **H6-5** Aide hors ligne en HTML simple, sans moteur web ; changement de langue appliqué au redémarrage.
- **H6-6** Téléchargement de Hunspell 1.7.2 (dépôt officiel `github.com/hunspell/hunspell`, archive de la version) et
  du dictionnaire français de Grammalecte : accord de Jérôme demandé par l'orchestrateur avant la vague V12 (6b) ;
  taille ajoutée aux paquets rapportée à la porte de lot.
- **H6-7** Verrou de classe : liste des commandes laissées libres tranchée par L6-T0 ; défaut : quitter, déverrouiller,
  aide, page précédente et suivante, plein écran.
- **H6-8** N1-A14 (apparence simplifiée pour les petites classes) : non reprise ; le profil « En classe » en tient
  lieu. Question posée à la porte du lot 6.

## Sections des tâches

<a id="tache-l6-t0"></a>
### Tâche L6-T0 — Ouverture : vidéos, REA et ancien dépôt confrontés au lot

**Objectif.** Appliquer la règle de la tâche d'ouverture (amendement 1, § 4, points 1 à 6) au lot 6, sur le modèle de
`docs/verifications/2026-10-18-lot10-principes.md` : relever dans le corpus les principes des fonctions du lot (les
17 constats du § 4 : 7 « interface et profils », 1 « stylo, formes, couleurs », 1 « pages, fond, grille », 5
« ressources », 1 « actions et navigateurs », 2 « multimédia et import ») et ceux de la spécification ; trancher sur la
3.5.18, par REA en statique (ressources des boîtes, valeurs par défaut des réglages et des profils), les points
« à élucider » de l'amendement (F-298 : liste des fonctions grisées ; F-920 et F-142 : place du bouton d'édition,
suppression d'un raccourci ; F-297 : options, parcours, dictionnaire ; F-880 : ce que chaque profil ouvre, épingle et
règle) et les écarts **E3** et **E4** ; revérifier les API A1 à A16 sur le code de `feat/livraison-complete` ; relever
**N6**.

**Fichiers.** Créer `docs/verifications/2026-10-19-lot6-principes.md` (dépôt). Hors dépôt, créer
`C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot6.md` (relevé brut : constats REA, noms de l'original,
renvois aux fiches). Rien d'autre.

**Test et commande bornée.** Aucun code : preuve = rapport commité, relevé brut présent, `NOMS OK`, contrôle des
titres du corpus vide (section « Commandes de preuve communes »).

**Interdits.** Aucun nom de classe, de propriété ni de fichier de l'original dans le dépôt ; aucun titre, aucune
citation, aucune capture du corpus ; aucun nom de paperboard réel (`samples/`) ; observation de l'original à l'écran
seulement après avoir prévenu Jérôme (elle pilote la souris de l'écran principal) — en statique d'abord ; ne pas
modifier ce plan (l'orchestrateur l'amende).

**Terminé quand** le rapport contient : la table principe → exigence → tâche → scénario (ou raison de l'écart) pour
chaque constat du lot ; les décisions **D1 à D14** ci-dessous, chacune tranchée ou marquée « non tranché » avec le
défaut du plan ; la table A1 à A16 (conforme, ou signature réelle et tâche à amender) ; N6 ; au plus deux tâches de
principes proposées (`L6-P1`, `L6-P2`) avec fichiers et scénario.

Décisions à trancher : **D1** liste des réglages de chaque profil fourni (ouvre, épingle, règle) ; **D2** E4 (taille de
page par défaut selon le profil) ; **D3** liste des commandes grisées par le verrou de classe (F-298) et de celles qui
restent libres ; **D4** E3 (place du bouton d'édition des raccourcis) et suppression d'un raccourci ; **D5** options
du correcteur, parcours des pages, dictionnaire de l'utilisateur ; **D6** contenu de la grille étendue de couleurs
(nombre, ordre) ; **D7** comportement exact du repli d'une palette désépinglée (délai, marge) ; **D8** contenu du
tableau d'accueil hors entrées exclues ; **D9** sections de propriétés par type d'objet (lesquelles pour un trait,
une forme, un texte, une image, un groupe) ; **D10** colonnes et icônes du navigateur d'objet ; **D11** réglages du
mode Création et effets sur les lots déjà livrés ; **D12** forme d'une étiquette (au survol, toujours visible, style) ;
**D13** comportement du tampon et de l'insertion d'un modèle de page, d'un fond, d'une grille depuis les ressources ;
**D14** troisième réglage de qualité du rendu (P2 du lot 10) et regroupement automatique des traits (P10) : défauts.

**Étapes.**
- [ ] **Étape 1 : corpus.** Lire `C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\README.md` (sections
  « Principes absents » et « Écarts avec la spec ») ; lister les constats des thèmes du lot ; ouvrir chaque fiche citée
  (`Vnn`) ; noter les principes en nos mots dans le relevé brut.
- [ ] **Étape 2 : spécification et ancien dépôt.** `graphify query "profils palette navigateur propriétés ressources tableau de bord"`
  lancé dans `C:\Users\JeromeDenis\REA - Activinspire` ; lire `app/actcraft/ui/profiles_dialog.py`,
  `app/actcraft/ui/dashboard.py`, `app/actcraft/ui/shortcut_bar.py`, `app/actcraft/ui/toolbox.py`,
  `app/actcraft/ui/browsers/{objects,properties,resources}.py` ; `docs/spec/fonctionnalites.md` (F-030 à F-034,
  F-090, F-123, F-135, F-140 à F-142, F-297, F-298, F-560, F-570 à F-583, F-772, F-810 à F-813, F-820, F-821, F-830,
  F-840, F-880 à F-882, F-900 à F-935).
- [ ] **Étape 3 : REA en statique.** Skill `reverse-engineer-anything` sur `C:\Program Files\Activ Software\Inspire\` :
  ressources des boîtes de profils et de réglages, valeurs par défaut des cinq profils livrés, liste des fonctions du
  verrouillage (D3), chaînes du correcteur (D5). Si le serveur d'analyse ne répond pas, le noter et garder les
  défauts du plan (comme au lot 10).
- [ ] **Étape 4 : API.** Dans le worktree de la tâche : `graphify query "DocumentController addChangeObserver execute selection"`,
  `graphify query "BrowserPanel current PageThumbnails inputActive"`, `graphify query "arrange layer hide lock commands lot 5"`,
  puis lecture des en-têtes cités ; remplir la table A1 à A16.
- [ ] **Étape 5 : N6.**
  ```powershell
  pwsh -NoProfile -Command "Set-Location 'C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L6-T0'; New-Item -ItemType Directory -Force .tmp | Out-Null; . ./scripts/dev-env.ps1; cmake --preset debug *> .tmp/cfg.txt; ctest --preset debug -N | Select-String 'Total Tests'"
  ```
  Attendu : `Total Tests: <N6>` ; écrire N6 dans le rapport.
- [ ] **Étape 6 : rapport.** Écrire `docs/verifications/2026-10-19-lot6-principes.md` : en-tête (relevé brut cité),
  méthode et limites, table des constats (colonnes du rapport du lot 10), décisions D1 à D14, table A1 à A16, N6,
  tâches de principes proposées, écarts pour l'orchestrateur.
- [ ] **Étape 7 : contrôles et commit.** `git -C <wt> add docs/verifications/2026-10-19-lot6-principes.md` ; `NOMS OK` ;
  contrôle des titres du corpus vide ; commit « Lot 6 : principes confrontés, décisions et API revérifiées (L6-T0) ».

<a id="tache-l6-t1"></a>
### Tâche L6-T1 — Réservation : tests, fichiers neufs, commandes et disposition du lot, icônes

**Objectif.** Poser en une fois tout ce que les deux sous-lots se partageraient : enregistrement CMake de tous les
fichiers neufs (créés vides et marqués), des 28 tests unitaires et des 15 tests de captures (talons `QSKIP`), du
programme d'essai `launcher_probe` ; déclaration des commandes du lot dans `commands.json` ; leur place dans
`layout.json` ; 26 icônes à nous. Après L6-T1, aucune autre tâche du lot ne touche un `CMakeLists.txt` ni un
`tests/*.cmake`.

**Fichiers.** Ceux de la ligne L6-T1, plus `tests/tst_ui_theme.cpp` (lignes de données des icônes). Lire d'abord les
`CMakeLists.txt` et `tests/*.cmake` pour reprendre leur forme exacte (`actcraft_add_module`, `actcraft_add_test`,
bloc `RESOURCES` de `qt_add_qml_module`).

**Test et commande bornée.** Commande ciblée `-R 'tst_actions_layout|tst_ui_theme|tst_ui_commands'`, puis preuve de
tâche : `100% tests passed … out of N` avec **N = N6 + 43**, `TESTS OK`, les 43 tests neufs `Passed` en `SKIP`
nommant leur tâche (`ctest --preset debug -V -R 'tst_(actions_profile|ui_profiles|captures_palette)' | Select-String 'SKIP   :'`
montre `Talon L6 : rempli par L6-T4.` etc.).

**Preuve visuelle.** `captures.ps1 -Scenario premiers-pas -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "pageIndicator.text=1 / 2"` : `LU pageIndicator.text=1 / 2 OK`, `CAPTURES OK` (rien ne change à l'écran).

**Interdits.** Aucun comportement : un `.cpp` talon ne contient que le marqueur et, s'il le faut pour lier, les
définitions vides des fonctions déjà déclarées ailleurs (aucune ici : les en-têtes neufs sont vides). Aucune icône
reprise de l'original ni du clone Python ; aucun libellé de l'original (N1-12) ; aucun raccourci en double.

**Terminé quand** N = N6 + 43, `TESTS OK`, `tst_actions_layout` passe `lot6EntriesDeclared` et `noShortcutTwice`,
`tst_ui_theme` passe `iconsAreOurSvgOnA24Grid` avec les 26 nouvelles lignes, `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge** — ajouter à `tests/tst_actions_layout.cpp` :
  ```cpp
  void lot6EntriesDeclared()
  {
      actions::CommandRegistry registry;
      QString error;
      QVERIFY2(registry.loadDeclarations(actions::defaultCommandsJson(), error), qPrintable(error));
      const QStringList ids = {
          QStringLiteral("palette.dock-left"),   QStringLiteral("palette.dock-right"),
          QStringLiteral("palette.dock-top"),    QStringLiteral("palette.dock-bottom"),
          QStringLiteral("palette.float"),       QStringLiteral("palette.collapse"),
          QStringLiteral("palette.collapse-full"), QStringLiteral("palette.pin"),
          QStringLiteral("palette.icon-size"),   QStringLiteral("color.more"),
          QStringLiteral("color.pick"),          QStringLiteral("color.dialog"),
          QStringLiteral("color.add"),           QStringLiteral("browser.objects"),
          QStringLiteral("browser.properties"),  QStringLiteral("browser.notes"),
          QStringLiteral("browser.resources"),   QStringLiteral("profile.choose"),
          QStringLiteral("profile.activate"),    QStringLiteral("profile.edit"),
          QStringLiteral("profile.save-as"),     QStringLiteral("profile.import"),
          QStringLiteral("profile.export"),      QStringLiteral("profile.delete"),
          QStringLiteral("profile.reset"),       QStringLiteral("mode.design"),
          QStringLiteral("view.dashboard"),      QStringLiteral("file.recent-clear"),
          QStringLiteral("resources.mine"),      QStringLiteral("resources.shared"),
          QStringLiteral("resources.other"),     QStringLiteral("resources.insert"),
          QStringLiteral("resources.add"),       QStringLiteral("resources.stamp"),
          QStringLiteral("resources.new-folder"), QStringLiteral("resources.rename"),
          QStringLiteral("resources.delete"),    QStringLiteral("classroom.lock"),
          QStringLiteral("classroom.password"),  QStringLiteral("spell.check"),
          QStringLiteral("launcher.run"),        QStringLiteral("launcher.edit"),
          QStringLiteral("help.contents"),       QStringLiteral("shortcutbar.insert"),
          QStringLiteral("settings.language"),
      };
      for (const QString& id : ids)
          QVERIFY2(registry.find(id), qPrintable(id));
      QCOMPARE(registry.find(QStringLiteral("help.contents"))->shortcut, QStringLiteral("F1"));
      QVERIFY(registry.find(QStringLiteral("help.website"))->shortcut.isEmpty());
      QCOMPARE(registry.find(QStringLiteral("launcher.edit"))->platforms,
               actions::Platforms(actions::Platform::Windows));
      QVERIFY(registry.find(QStringLiteral("mode.design"))->checkable);
  }
  ```
  Si `noShortcutTwice` n'existe pas encore dans ce test (posé par L0-S4 dans `tst_actions_registry`), ne pas le
  dupliquer : il est relancé par la commande ciblée.
- [ ] **Étape 2 : rouge.** Commande ciblée `-R tst_actions_layout` : `FAIL!  : TstActionsLayout::lot6EntriesDeclared`
  (`palette.dock-left`).
- [ ] **Étape 3 : commandes.** Ajouter à la fin du tableau `commands` de `actions/data/commands.json`, en un bloc
  contigu « lot 6 » (même forme que les entrées existantes ; `help.website` perd son `"shortcut": "F1"`) :
  ```json
  { "id": "palette.dock-left", "label": "Ancrer la palette à gauche", "icon": "palette-gauche" },
  { "id": "palette.dock-right", "label": "Ancrer la palette à droite", "icon": "palette-droite" },
  { "id": "palette.dock-top", "label": "Ancrer la palette en haut", "icon": "palette-haut" },
  { "id": "palette.dock-bottom", "label": "Ancrer la palette en bas", "icon": "palette-bas" },
  { "id": "palette.float", "label": "Palette flottante", "icon": "palette-flottante" },
  { "id": "palette.collapse", "label": "Réduire la palette", "icon": "palette-reduire", "checkable": true },
  { "id": "palette.collapse-full", "label": "Ne garder que la palette", "icon": "palette-reduire-tout", "checkable": true },
  { "id": "palette.pin", "label": "Garder la palette affichée", "icon": "palette-epingler", "checkable": true },
  { "id": "palette.icon-size", "label": "Taille des icônes", "argument": "number" },
  { "id": "color.more", "label": "Plus de couleurs…", "icon": "couleurs-plus" },
  { "id": "color.pick", "label": "Prendre une couleur à l'écran", "icon": "pipette" },
  { "id": "color.dialog", "label": "Choisir une couleur…" },
  { "id": "color.add", "label": "Ajouter la couleur à la palette", "argument": "color" },
  { "id": "browser.objects", "label": "Objets de la page", "icon": "nav-objets", "checkable": true },
  { "id": "browser.properties", "label": "Propriétés", "icon": "nav-proprietes", "checkable": true },
  { "id": "browser.notes", "label": "Notes de la page", "icon": "nav-notes", "checkable": true },
  { "id": "browser.resources", "label": "Bibliothèque", "icon": "nav-ressources", "checkable": true },
  { "id": "profile.choose", "label": "Changer de profil…", "icon": "profil" },
  { "id": "profile.activate", "label": "Activer le profil", "argument": "text" },
  { "id": "profile.edit", "label": "Profils et réglages…", "icon": "profils" },
  { "id": "profile.save-as", "label": "Enregistrer le profil sous…", "argument": "text", "optional": true },
  { "id": "profile.import", "label": "Importer un profil…", "argument": "url", "optional": true },
  { "id": "profile.export", "label": "Exporter le profil…", "argument": "url", "optional": true },
  { "id": "profile.delete", "label": "Supprimer le profil" },
  { "id": "profile.reset", "label": "Revenir au profil d'origine" },
  { "id": "mode.design", "label": "Mode conception", "icon": "conception", "shortcut": "F2", "checkable": true },
  { "id": "view.dashboard", "label": "Accueil", "icon": "accueil", "shortcut": "F11" },
  { "id": "file.recent-clear", "label": "Vider la liste des documents récents", "icon": "recents" },
  { "id": "resources.mine", "label": "Mes ressources", "icon": "ressources-miennes" },
  { "id": "resources.shared", "label": "Ressources partagées", "icon": "ressources-partagees" },
  { "id": "resources.other", "label": "Autre dossier…", "icon": "ressources-autre", "argument": "url", "optional": true },
  { "id": "resources.insert", "label": "Insérer dans le paperboard", "argument": "url" },
  { "id": "resources.add", "label": "Ajouter la sélection à la bibliothèque", "argument": "url", "optional": true },
  { "id": "resources.stamp", "label": "Tampon", "icon": "tampon", "checkable": true },
  { "id": "resources.new-folder", "label": "Nouveau dossier", "argument": "text", "optional": true },
  { "id": "resources.rename", "label": "Renommer…", "argument": "text", "optional": true },
  { "id": "resources.delete", "label": "Retirer de la bibliothèque", "argument": "url" },
  { "id": "classroom.lock", "label": "Verrouiller pour la classe", "icon": "classe-verrou", "checkable": true },
  { "id": "classroom.password", "label": "Mot de passe de la classe…" },
  { "id": "spell.check", "label": "Vérifier l'orthographe", "icon": "orthographe", "shortcut": "F7" },
  { "id": "launcher.run", "label": "Lancer le raccourci", "argument": "text", "platforms": ["windows"] },
  { "id": "launcher.edit", "label": "Raccourcis vers des programmes…", "icon": "programme", "platforms": ["windows"] },
  { "id": "help.contents", "label": "Aide", "icon": "aide-sommaire", "shortcut": "F1" },
  { "id": "shortcutbar.insert", "label": "Insérer l'objet de la barre", "argument": "text" },
  { "id": "settings.language", "label": "Langue de l'interface", "argument": "text" }
  ```
  Si L6-T0 (D3, D4) ou `noShortcutTwice` signale un raccourci déjà pris (F2, F7, F11), garder l'entrée sans raccourci et
  le noter au rapport (correctif `L6-C0`).
- [ ] **Étape 4 : disposition.** Dans `actions/data/layout.json`, même forme que les entrées existantes : menu
  Affichage : `view.dashboard`, sous-menu `>palette` (`palette.dock-left`, `palette.dock-right`, `palette.dock-top`,
  `palette.dock-bottom`, `palette.float`, séparateur, `palette.collapse`, `palette.collapse-full`, `palette.pin`), puis
  `browser.objects`, `browser.properties`, `browser.notes`, `browser.resources`, `mode.design` ; menu Fichier :
  `file.recent-clear` sous l'entrée d'ouverture ; menu Outils : `spell.check`, `launcher.edit`, `classroom.lock`,
  `classroom.password` ; menu Aide : `help.contents` en tête ; ligne supérieure : `profile.choose`, `mode.design` ;
  menu principal de la palette : `profile.edit`. Aucune autre entrée n'est déplacée.
- [ ] **Étape 5 : CMake et fichiers vides.** Créer chaque fichier talon de la ligne L6-T1 avec le marqueur de sa tâche
  propriétaire (table ci-dessous), et l'enregistrer : en-têtes et sources dans le `CMakeLists.txt` de leur module
  (`secretstore_win.cpp` seulement sous `WIN32`, `secretstore_other.cpp` sinon ; `ui` lié à `Qt6::Network`),
  `actions/data/profiles/*.json` dans les ressources de `actions`, `ui/help/index.html` et `ui/i18n/` dans celles de
  `ui`, QML dans `qt_add_qml_module` de `ui` (fichiers créés par L6-T2, noms enregistrés ici), tests par
  `actcraft_add_test` dans le `.cmake` de leur module, captures par la même fonction que `tst_captures_premiers_pas`
  (`tst_captures_<nom avec soulignés>`), `launcher_probe` par `add_executable(launcher_probe launcher_probe.cpp)` dans
  `tests/ui.cmake` (sortie dans `build/<preset>/tests/`).
  Propriétaires : `profile*`, `profilestore*` → L6-T4 ; `preferences*`, `profilecontroller*` → L6-T5 ;
  `profiles/*.json` → L6-T6 ; `colorpalette*` → L6-T8 ; `shortcutbar*` → L6-T9 ; `propertyedits*` → L6-T11 ;
  `propertymodel*` → L6-T12 ; `objecttree*` → L6-T14 ; `notesedits*`, `notescontroller*` → L6-T16 ; `labeltips*` →
  L6-T17 ; `fragment*` → L6-T19 ; `resourcelibrary*` → L6-T20 ; `designmode*` → L6-T22 ; `recentdocuments*` → L6-T24 ;
  `layouteditor*` → L6-T26 ; `settingsschema*` → L6-T27 ; `secretstore*`, `networksettings*` → L6-T28 ; `launchers*`,
  `launcher_probe.cpp` → L6-T29 ; `classroomlock*` → L6-T31 ; `spellcheck*`, `hunspellengine.cpp` → L6-T33 ;
  `spelling*` → L6-T34 ; `helpviewer*`, `ui/help/index.html` → L6-T35 ; `language*`, `ui/i18n/actcraft_fr.ts` → L6-T36.
  Un JSON talon de profil : `{ "format": "actcraft-profile/1", "id": "<id>", "label": "<id>", "description": "Talon L6 : rempli par L6-T6." }`.
  `launcher_probe.cpp` talon : `int main() { return 0; } // Talon L6 : rempli par L6-T29.` `index.html` talon :
  `<!-- Talon L6 : rempli par L6-T35. --><html><body></body></html>`. `actcraft_fr.ts` talon : un `<TS version="2.1" language="fr_FR"></TS>`
  précédé du commentaire XML du marqueur.
- [ ] **Étape 6 : tests talons.** Chaque `tests/tst_<nom>.cpp` :
  ```cpp
  #include <QtTest>

  class Tst<Nom> : public QObject
  {
      Q_OBJECT
  private slots:
      void reserved() { QSKIP("Talon L6 : rempli par L6-T<n>."); }
  };

  QTEST_MAIN(Tst<Nom>)
  #include "tst_<nom>.moc"
  ```
  Chaque scénario talon `bench/scenarios/<nom>.json` : forme de `premiers-pas.json`, `"description": "Talon L6 : rempli par L6-T<n>."`,
  une seule étape `waitReady` (propriétaires : `profils` L6-T6, `palette` L6-T7, `couleurs` L6-T8, `proprietes` L6-T13,
  `navigateur-objets` L6-T15, `notes` L6-T16, `etiquettes` L6-T17, `reglages-gardes` L6-T18, `ressources` L6-T21,
  `mode-creation` L6-T22, `tableau-de-bord` L6-T23, `raccourcis-programmes` L6-T30, `classe-protegee` L6-T31,
  `orthographe` L6-T34, `aide-langue` L6-T36). Le test de captures d'un scénario talon est sauté par la même voie que
  ceux des lots précédents (lire `tests/bench.cmake` : propriété `SKIP_REGULAR_EXPRESSION` ou variable du scénario) ;
  sinon, l'enregistrer dans la même forme et laisser `waitReady` passer.
- [ ] **Étape 7 : icônes.** 26 SVG à nous, grille de 24 px, traits de `Theme` (même gabarit que les icônes de L0-S1),
  noms de la ligne L6-T1 ; ajouter leurs 26 lignes de données à
  `iconsAreOurSvgOnA24Grid` de `tests/tst_ui_theme.cpp`.
- [ ] **Étape 8 : vert.** Commande ciblée `-R 'tst_actions_layout|tst_ui_theme|tst_ui_commands'` verte ; preuve de tâche
  (N = N6 + 43) ; preuve visuelle `premiers-pas`.
- [ ] **Étape 9 : commit.** `git -C <wt> add` de chaque fichier de la ligne (chemins explicites ; jamais `-A`),
  `git -C <wt> diff --cached --stat` relu ; `NOMS OK` ; commit « Lot 6 : réservations, commandes et icônes (L6-T1) ».

<a id="tache-l6-t2"></a>
### Tâche L6-T2 — Coquille QML du lot

**Objectif.** Créer les 22 fichiers QML neufs du lot (talons qui s'affichent vides ou masqués), les placer dans
`Main.qml`, ajouter les quatre navigateurs comme contenus de `BrowserPanel.qml` (A8), poser l'emplacement de la
barre de raccourcis dans `ToolPalette.qml`, l'icône de notes et le bouton de profil dans `TopBar.qml`, et un
gestionnaire pour **chaque** commande du lot (talon : `console.info` + rien, sauf les bascules de navigateur et de
palette qui mettent déjà à jour une propriété QML).

**Fichiers.** Ceux de la ligne L6-T2.

**Test et commande bornée.** Commande ciblée `-R tst_ui_shell` ; preuve de tâche ; banc sans écran.

**Preuve visuelle.** `premiers-pas` hors écran : `LU pageIndicator.text=1 / 2 OK`, `CAPTURES OK` ; décrire la capture
finale : aucune différence visible avec la fin du lot 5 (les emplacements sont masqués).

**Interdits.** Aucun comportement au-delà des emplacements ; aucune couverture d'entrée posée (les tâches propriétaires
s'en chargent) ; aucun texte de l'original.

**Terminé quand** `tst_ui_shell` passe `lot6ShellPlaceholders`, `everyLot6CommandHasAHandler`, `browserTabsInOrder` et
ses tests existants, `shellLoadsWithoutQmlWarning` compris.

**Étapes.**
- [ ] **Étape 1 : tests rouges** dans `tests/tst_ui_shell.cpp` (même montage que les tests existants du fichier :
  moteur QML chargé sur `Main.qml`, `findChild` par `objectName`) :
  ```cpp
  void lot6ShellPlaceholders()
  {
      auto* root = loadMain();
      for (const char* name : {"paletteFrame", "shortcutBar", "extendedColors", "colorLoupe", "objectBrowser",
                               "propertyBrowser", "notesBrowser", "resourceBrowser", "labelTip", "dashboard",
                               "profileChooser", "profileEditor", "classroomLockDialog", "spellingDialog",
                               "helpViewer", "saveSummaryDialog", "notesIcon"})
          QVERIFY2(root->findChild<QObject*>(QString::fromLatin1(name)), name);
  }
  void everyLot6CommandHasAHandler()
  {
      auto* root = loadMain();
      Q_UNUSED(root);
      auto* commands = appCommands();
      const QByteArray json = actcraft::actions::defaultCommandsJson();
      const QJsonArray all = QJsonDocument::fromJson(json).object().value(QStringLiteral("commands")).toArray();
      bool inLot6 = false;
      for (const QJsonValue& v : all) {
          const QString id = v.toObject().value(QStringLiteral("id")).toString();
          if (id == QStringLiteral("palette.dock-left"))
              inLot6 = true;
          if (inLot6 && commands->isAvailable(id))
              QVERIFY2(commands->registry().hasHandler(id), qPrintable(id));
      }
      QVERIFY(inLot6);
  }
  void browserTabsInOrder()
  {
      auto* panel = loadMain()->findChild<QObject*>(QStringLiteral("browserPanel"));
      QVERIFY(panel);
      const QStringList tabs = panel->property("tabs").toStringList();
      QCOMPARE(tabs, (QStringList{QStringLiteral("pages"), QStringLiteral("resources"), QStringLiteral("objects"),
                                  QStringLiteral("notes"), QStringLiteral("properties"), QStringLiteral("actions")}));
  }
  ```
  `loadMain()` et `appCommands()` : reprendre les aides déjà présentes dans le fichier (sinon les écrire en tête du
  fichier, sur le modèle de `socleSlotsExist`). Si `BrowserPanel.qml` du lot 4 nomme autrement sa liste d'onglets
  (A8), adapter au nom réel et le noter.
- [ ] **Étape 2 : rouge** (`-R tst_ui_shell`) : `lot6ShellPlaceholders` échoue sur `paletteFrame`.
- [ ] **Étape 3 : talons QML.** Chaque fichier neuf :
  ```qml
  // Talon L6 : rempli par L6-T<n>.
  import QtQuick

  Item {
      objectName: "<nom>"
      visible: false
  }
  ```
  Propriétaires : `PaletteFrame` L6-T7 ; `ShortcutBar` L6-T9 ; `ExtendedColors`, `ColorLoupe` L6-T8 ; `ObjectBrowser`
  L6-T15 ; `PropertyBrowser`, `PropertyEditor` L6-T13 ; `NotesBrowser` L6-T16 ; `ResourceBrowser` L6-T21 ; `LabelTip`
  L6-T17 ; `Dashboard` L6-T23 ; `SaveSummaryDialog` L6-T24 ; `ProfileChooser`, `ProfileEditor`, `FormatPage` L6-T25 ;
  `CommandsPage` L6-T26 ; `SettingsPage` L6-T27 ; `NetworkPage` L6-T28 ; `LaunchersPage` L6-T30 ;
  `ClassroomLockDialog` L6-T31 ; `SpellingDialog` L6-T34 ; `HelpViewer` L6-T35. `objectName` en `camelCase` du nom de
  fichier (`paletteFrame`, `shortcutBar`…).
- [ ] **Étape 4 : placement.** `Main.qml` : `PaletteFrame { id: paletteFrame }` enveloppe la palette existante (aucun
  changement de position tant que L6-T7 n'est pas fait : la palette reste ancrée à droite) ; dialogues et fenêtres du
  lot instanciés une fois, masqués ; `LabelTip` au-dessus de la page, sous les surcouches de présentation.
  `BrowserPanel.qml` : propriété `tabs` (liste ci-dessus), contenus `ResourceBrowser`, `ObjectBrowser`, `NotesBrowser`,
  `PropertyBrowser` par onglet ; `actions` reste grisé (lot 8). `ToolPalette.qml` : `ShortcutBar { objectName:
  "shortcutBar" }` en troisième colonne, masquée. `TopBar.qml` : `CommandButton` de `profile.choose` et de
  `mode.design`, et une icône `notesIcon` (`notes-page.svg`), masquée.
- [ ] **Étape 5 : gestionnaires.** Dans `Main.qml`, un `appCommands.setHandler` par commande du bloc lot 6 :
  `browser.*` → `browserPanel.visible = true; browserPanel.current = "<onglet>"` ; `palette.*` →
  `console.info("palette", id)` (rempli par L6-T7) ; les autres → `console.info(id)` avec un commentaire
  `// Talon L6 : rempli par L6-T<n>.` qui nomme la tâche.
- [ ] **Étape 6 : vert.** `-R tst_ui_shell` vert ; preuve de tâche ; preuve visuelle `premiers-pas` ; banc sans écran.
- [ ] **Étape 7 : commit.** `git add` des fichiers de la ligne ; `NOMS OK` ; commit « Lot 6 : coquille QML et
  gestionnaires (L6-T2) ».

<a id="tache-l6-t3"></a>
### Tâche L6-T3 — `actions` : attributs « verrouillable » et « restreignable », porte du verrou

**Objectif.** Le registre porte les deux attributs de l'amendement 1 (§ 1) et la **porte du verrou** (D6-7) :
quand le verrou de classe est posé, `isEnabled` et `trigger` refusent toute commande `lockable`, quel que soit
`setEnabled`. Les attributs sont remplis pour **toutes** les commandes présentes dans `commands.json` (lots 1 à 5,
socle, lot 7, lot 6). Le patron évident est le mauvais : griser en appelant `setEnabled(false)` sur chaque commande
serait défait par le premier contrôleur qui réactive une commande (Annuler, Coller…) ; la porte est un état à part.

**Fichiers.** `actions/include/actcraft/actions/commandregistry.h`, `actions/src/commandregistry.cpp`,
`actions/data/commands.json`, `tests/tst_actions_lockable.cpp`.

**Test et commande bornée.** Commande ciblée `-R 'tst_actions_lockable|tst_actions_registry|tst_actions_layout'` ;
fonctions de la table en `PASS` ; mutation : retirer le terme du verrou de `isEnabled` → `lockGreysOnlyLockable` et
`enableCannotBypassTheLock` rouges, remise par `git checkout -- actions/src/commandregistry.cpp`.

**Interdits.** Ne pas toucher à `ui/` (le pont QML lit `isEnabled` du registre, A2 : le grisage s'y voit sans
changement) ; aucun mot de passe ici (L6-T31).

**Terminé quand** les six fonctions passent, la mutation a été vue rouge, et chaque commande de `commands.json`
déclare explicitement `lockable` et `restrictable`.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_actions_lockable.cpp` (remplace le talon) :
  ```cpp
  #include <actcraft/actions/commandregistry.h>

  #include <QJsonArray>
  #include <QJsonDocument>
  #include <QJsonObject>
  #include <QtTest>

  using namespace actcraft::actions;

  class TstActionsLockable : public QObject
  {
      Q_OBJECT
      static QByteArray two()
      {
          return R"({"format":"actcraft-commands/1","commands":[
              {"id":"a.b","label":"A","lockable":true,"restrictable":true},
              {"id":"c.d","label":"C","lockable":false,"restrictable":false}]})";
      }
  private slots:
      void attributesRead()
      {
          CommandRegistry r;
          QString error;
          QVERIFY2(r.loadDeclarations(two(), error), qPrintable(error));
          QVERIFY(r.find(QStringLiteral("a.b"))->lockable);
          QVERIFY(r.find(QStringLiteral("a.b"))->restrictable);
          QVERIFY(!r.find(QStringLiteral("c.d"))->lockable);
      }
      void lockGreysOnlyLockable()
      {
          CommandRegistry r;
          QString error;
          QVERIFY(r.loadDeclarations(two(), error));
          r.setLocked(true);
          QVERIFY(r.isLocked());
          QVERIFY(!r.isEnabled(QStringLiteral("a.b")));
          QVERIFY(r.isEnabled(QStringLiteral("c.d")));
          r.setLocked(false);
          QVERIFY(r.isEnabled(QStringLiteral("a.b")));
      }
      void lockedTriggerCallsNothing()
      {
          CommandRegistry r;
          QString error;
          QVERIFY(r.loadDeclarations(two(), error));
          int calls = 0;
          r.setHandler(QStringLiteral("a.b"), [&](const QVariant&) { ++calls; });
          r.setLocked(true);
          QVERIFY(!r.trigger(QStringLiteral("a.b"), QVariant(), currentPlatform()));
          QCOMPARE(calls, 0);
      }
      void enableCannotBypassTheLock()
      {
          CommandRegistry r;
          QString error;
          QVERIFY(r.loadDeclarations(two(), error));
          r.setLocked(true);
          r.setEnabled(QStringLiteral("a.b"), true);
          QVERIFY(!r.isEnabled(QStringLiteral("a.b")));
          r.setEnabled(QStringLiteral("a.b"), false);
          r.setLocked(false);
          QVERIFY(!r.isEnabled(QStringLiteral("a.b"))); // l'état propre reste celui posé
      }
      void everyCommandDeclaresBoth()
      {
          const QJsonArray all =
              QJsonDocument::fromJson(defaultCommandsJson()).object().value(QStringLiteral("commands")).toArray();
          QVERIFY(!all.isEmpty());
          for (const QJsonValue& v : all) {
              const QJsonObject o = v.toObject();
              const QString id = o.value(QStringLiteral("id")).toString();
              QVERIFY2(o.value(QStringLiteral("lockable")).isBool(), qPrintable(id));
              QVERIFY2(o.value(QStringLiteral("restrictable")).isBool(), qPrintable(id));
          }
      }
      void fileAndQuitStayFree()
      {
          CommandRegistry r;
          QString error;
          QVERIFY(r.loadDeclarations(defaultCommandsJson(), error));
          for (const char* id : {"file.quit", "classroom.lock", "help.contents", "page.next", "page.previous",
                                 "view.fullscreen"})
              QVERIFY2(!r.find(QString::fromLatin1(id))->lockable, id);
          for (const char* id : {"file.open", "file.save", "profile.edit", "palette.float", "mode.design"})
              QVERIFY2(r.find(QString::fromLatin1(id))->lockable, id);
      }
  };

  QTEST_MAIN(TstActionsLockable)
  #include "tst_actions_lockable.moc"
  ```
- [ ] **Étape 2 : rouge** (`-R tst_actions_lockable`) : erreur de compilation `lockable` n'est pas membre de
  `CommandInfo`.
- [ ] **Étape 3 : registre.** Dans `CommandInfo`, après `argumentOptional` :
  ```cpp
      // Amendement 1, § 1 : le mode classe protégé peut la griser (F-298) ; une diffusion protégée peut
      // l'interdire (F-680, lot 8). Défaut sûr : verrouillable, non restreignable.
      bool lockable = true;
      bool restrictable = false;
  ```
  Dans `CommandRegistry` (public) :
  ```cpp
      // Verrou de classe (D6-7) : tant qu'il est posé, une commande verrouillable est désactivée et refusée par
      // trigger(), quel que soit setEnabled() ; l'état propre de chaque commande est gardé à part.
      void setLocked(bool locked) noexcept { m_locked = locked; }
      bool isLocked() const noexcept { return m_locked; }
  ```
  et `bool m_locked = false;` en membre privé. `loadDeclarations` lit `"lockable"` et `"restrictable"` (booléens
  facultatifs ; une valeur non booléenne est une erreur « commande n (id) : lockable n'est pas un booléen »).
  `isEnabled` :
  ```cpp
  bool CommandRegistry::isEnabled(const QString& id) const
  {
      const Entry* e = entry(id);
      if (!e)
          return false;
      return e->enabled && !(m_locked && e->info.lockable);
  }
  ```
  `trigger` refuse déjà une commande désactivée en passant par `isEnabled` : le vérifier ; sinon, y ajouter le même
  terme.
- [ ] **Étape 4 : données.** Dans `commands.json`, ajouter à **chaque** entrée `"lockable"` et `"restrictable"`. Règle :
  `lockable: false` pour `file.quit`, `classroom.lock`, `classroom.password`, `help.contents`, `help.website`,
  `help.about` (s'il existe), `page.next`, `page.previous`, `view.fullscreen`, `view.fit`, `view.zoom` et les commandes
  que L6-T0 (D3) range parmi les libres ; `true` pour toutes les autres. `restrictable: true` pour les commandes de copie,
  d'enregistrement, d'impression et de caméra présentes (`edit.copy`, `edit.cut`, `edit.duplicate`, `file.save`,
  `file.save-as`, `file.print`, `export.*`, `camera.*` — identifiants réels relus dans le fichier) ; `false` sinon. Écrire
  au rapport la liste des `lockable: false` et des `restrictable: true`.
- [ ] **Étape 5 : vert** (`-R 'tst_actions_lockable|tst_actions_registry|tst_actions_layout'`), puis mutation (rouge,
  `git -C <wt> checkout -- actions/src/commandregistry.cpp`… **après** avoir commité le code vert, pour que la remise
  rende le vert), puis preuve de tâche.
- [ ] **Étape 6 : commit.** `git add` des quatre fichiers ; `NOMS OK` ; commit « Lot 6 : commandes verrouillables et
  restreignables, porte du verrou dans le registre (L6-T3) ». La mutation se fait après ce commit, la remise par
  `git checkout --`.

<a id="tache-l6-t4"></a>
### Tâche L6-T4 — `actions` : modèle de profil et magasin

**Objectif.** Le profil, fichier à nous `actcraft-profile/1` (D6-1), et son magasin : profils fournis (ressources
`:/actcraft/actions/profiles/<id>.json`, lecture seule) et profils de l'utilisateur (`<racine>/<id>.json`) ; une copie
utilisateur d'un profil fourni le masque ; réinitialiser supprime la copie ; enregistrer sous crée un profil
utilisateur ; supprimer refuse un profil fourni ; exporter écrit un seul fichier (profil + disposition éditée) ;
importer ne remplace jamais un profil existant (suffixe `-2`, `-3`…). Clés inconnues gardées et réécrites telles
quelles (règle de compatibilité de l'amendement 1, § 1). Le patron évident est le mauvais : sérialiser par
`QJsonDocument` de la structure seule perdrait les clés d'une version plus récente.

**Fichiers.** `actions/include/actcraft/actions/profile.h`, `actions/src/profile.cpp`,
`actions/include/actcraft/actions/profilestore.h`, `actions/src/profilestore.cpp`, `tests/tst_actions_profile.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_actions_profile` ; onze fonctions en `PASS`.

**Interdits.** QtCore seulement (`actions/` sans QtGui) ; aucune lecture de `QSettings` ici ; aucun nom de profil de
l'original.

**Terminé quand** les onze fonctions passent et la preuve de tâche est verte.

**Étapes.**
- [ ] **Étape 1 : en-tête** `actions/include/actcraft/actions/profile.h` (remplace le talon ; figé pour le lot) :
  ```cpp
  #pragma once

  #include <QByteArray>
  #include <QJsonObject>
  #include <QString>
  #include <QStringList>

  #include <cstdint>
  #include <optional>

  namespace actcraft::actions {

  enum class Dock : std::uint8_t
  {
      Left,
      Right,
      Top,
      Bottom,
      Floating,
  };
  QString dockName(Dock dock);                         // « left », « right », « top », « bottom », « floating »
  std::optional<Dock> dockFromName(const QString& name);

  struct PaletteSettings
  {
      Dock dock = Dock::Right;
      double floatX = 0; // position flottante, pixels de fenêtre
      double floatY = 0;
      bool collapsed = false;       // F-031 : première rangée seulement
      bool shortcutsHidden = false; // F-032 : barre de raccourcis masquée
      bool pinned = true;           // F-031 : faux = repli au retrait du pointeur
      int swatches = 16;            // F-900 : 8, 16 ou 24
  };

  struct BrowserSettings
  {
      bool visible = false;
      QString side = QStringLiteral("left");
      bool pinned = true;
      QString current = QStringLiteral("pages");
      int width = 240;
  };

  inline constexpr int kIconPixelsMin = 16;
  inline constexpr int kIconPixelsMax = 32;
  int clampIconPixels(int pixels) noexcept;

  struct Profile
  {
      QString id;    // [a-z0-9-]+
      QString label; // français, écrit par nous
      int iconPixels = 24;
      PaletteSettings palette;
      BrowserSettings browsers;
      QStringList extraColors; // « #RRGGBB » ajoutées par l'utilisateur (F-135)
      QStringList recentTools; // F-140, 8 au plus, plus récent d'abord
      QStringList pinned;      // D6-3 : commandes ajoutées à la palette par le profil
      bool fitPage = false;
      bool trashVisible = true;
      QJsonObject shortcutObjects; // F-141 : { "<id>": { "label", "objects" (fragment) } }
      QJsonObject launchers;       // F-920 : écrit par L6-T29
      QJsonObject settings;        // sections de F-930 : { "<section>": { "<clé>": valeur } }
      QJsonObject unknownKeys;     // clés de premier niveau inconnues de cette version, réécrites telles quelles
  };

  inline constexpr char kProfileFormat[] = "actcraft-profile/1";

  QByteArray profileToJson(const Profile& profile);
  // Faux si le JSON est invalide, si « format » manque ou si sa version majeure n'est pas 1 ; `error` dit pourquoi.
  bool profileFromJson(const QByteArray& json, Profile& profile, QString& error);

  } // namespace actcraft::actions
  ```
- [ ] **Étape 2 : en-tête** `actions/include/actcraft/actions/profilestore.h` :
  ```cpp
  #pragma once

  #include <actcraft/actions/profile.h>

  #include <QString>
  #include <QStringList>

  #include <optional>

  namespace actcraft::actions {

  // Profils fournis (lecture seule) et de l'utilisateur (D6-1). Une copie utilisateur masque le profil fourni
  // de même identifiant.
  class ProfileStore
  {
  public:
      // `userRoot` : dossier des profils de l'utilisateur (créé à la première écriture) ;
      // `builtinRoot` : dossier des profils fournis (défaut : « :/actcraft/actions/profiles ») ;
      // `builtinOrder` : ordre d'affichage des profils fournis.
      ProfileStore(QString userRoot, QString builtinRoot, QStringList builtinOrder);

      QStringList ids() const; // fournis dans `builtinOrder`, puis ceux de l'utilisateur seulement, triés
      bool isBuiltin(const QString& id) const;
      bool hasUserCopy(const QString& id) const;
      std::optional<Profile> load(const QString& id, QString& error) const;
      // Dossier propre au profil (disposition éditée, F-910) : <userRoot>/<id>.
      QString directory(const QString& id) const;

      bool save(const Profile& profile, QString& error); // QSaveFile, écriture atomique
      bool saveAs(const Profile& profile, const QString& newId, const QString& label, QString& error);
      bool remove(const QString& id, QString& error);    // refuse un profil fourni
      bool reset(const QString& id, QString& error);     // retire la copie utilisateur d'un profil fourni
      // Un seul fichier JSON : { "format": "actcraft-profile-export/1", "profile": {…}, "layout": {…} | null }.
      bool exportTo(const QString& id, const QString& filePath, QString& error) const;
      // Rend l'identifiant créé (jamais un identifiant existant : suffixe -2, -3…).
      std::optional<QString> importFrom(const QString& filePath, QString& error);

  private:
      QString userFile(const QString& id) const;
      QString m_userRoot;
      QString m_builtinRoot;
      QStringList m_builtinOrder;
  };

  } // namespace actcraft::actions
  ```
- [ ] **Étape 3 : test rouge** `tests/tst_actions_profile.cpp` (remplace le talon ; dossiers temporaires pour les deux
  racines, profils fournis écrits par le test) :
  ```cpp
  #include <actcraft/actions/profilestore.h>

  #include <QFile>
  #include <QJsonDocument>
  #include <QTemporaryDir>
  #include <QtTest>

  using namespace actcraft::actions;

  namespace {
  bool writeFile(const QString& path, const QByteArray& bytes)
  {
      QFile f(path);
      return f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size();
  }
  Profile sample()
  {
      Profile p;
      p.id = QStringLiteral("essai");
      p.label = QStringLiteral("Essai");
      p.iconPixels = 32;
      p.palette.dock = Dock::Left;
      p.palette.collapsed = true;
      p.palette.pinned = false;
      p.palette.swatches = 24;
      p.browsers.visible = true;
      p.browsers.current = QStringLiteral("objects");
      p.extraColors = {QStringLiteral("#12AB34")};
      p.recentTools = {QStringLiteral("present.curtain")};
      p.pinned = {QStringLiteral("math.ruler")};
      p.fitPage = true;
      p.settings.insert(QStringLiteral("objects"), QJsonObject{{QStringLiteral("auto-group"), false}});
      return p;
  }
  } // namespace

  class TstActionsProfile : public QObject
  {
      Q_OBJECT
      QTemporaryDir m_user;
      QTemporaryDir m_builtin;
      ProfileStore store() const
      {
          return ProfileStore(m_user.path(), m_builtin.path(), {QStringLiteral("b1"), QStringLiteral("b2")});
      }
  private slots:
      void init()
      {
          m_user.~QTemporaryDir();
          new (&m_user) QTemporaryDir();
          QVERIFY(writeFile(m_builtin.filePath(QStringLiteral("b1.json")),
                            R"({"format":"actcraft-profile/1","id":"b1","label":"Un"})"));
          QVERIFY(writeFile(m_builtin.filePath(QStringLiteral("b2.json")),
                            R"({"format":"actcraft-profile/1","id":"b2","label":"Deux"})"));
      }
      void roundTripKeepsEveryField()
      {
          Profile back;
          QString error;
          QVERIFY2(profileFromJson(profileToJson(sample()), back, error), qPrintable(error));
          QCOMPARE(profileToJson(back), profileToJson(sample()));
          QCOMPARE(back.palette.dock, Dock::Left);
          QCOMPARE(back.iconPixels, 32);
          QCOMPARE(back.extraColors, QStringList{QStringLiteral("#12AB34")});
      }
      void unknownKeysKept()
      {
          Profile p;
          QString error;
          QVERIFY(profileFromJson(R"({"format":"actcraft-profile/1","id":"x","label":"X","future":{"a":1}})", p, error));
          const QJsonObject again = QJsonDocument::fromJson(profileToJson(p)).object();
          QCOMPARE(again.value(QStringLiteral("future")).toObject().value(QStringLiteral("a")).toInt(), 1);
      }
      void majorVersionRefused()
      {
          Profile p;
          QString error;
          QVERIFY(!profileFromJson(R"({"format":"actcraft-profile/2","id":"x","label":"X"})", p, error));
          QVERIFY(!error.isEmpty());
      }
      void iconPixelsClamped()
      {
          QCOMPARE(clampIconPixels(8), 16);
          QCOMPARE(clampIconPixels(40), 32);
          Profile p;
          QString error;
          QVERIFY(profileFromJson(R"({"format":"actcraft-profile/1","id":"x","label":"X","icon-pixels":99})", p, error));
          QCOMPARE(p.iconPixels, 32);
      }
      void builtinFirstThenUser()
      {
          auto s = store();
          QString error;
          QVERIFY(s.saveAs(sample(), QStringLiteral("a-moi"), QStringLiteral("À moi"), error));
          QCOMPARE(s.ids(), (QStringList{QStringLiteral("b1"), QStringLiteral("b2"), QStringLiteral("a-moi")}));
      }
      void userCopyShadowsBuiltin()
      {
          auto s = store();
          QString error;
          Profile p = *s.load(QStringLiteral("b1"), error);
          p.iconPixels = 20;
          QVERIFY(s.save(p, error));
          QVERIFY(s.hasUserCopy(QStringLiteral("b1")));
          QCOMPARE(s.load(QStringLiteral("b1"), error)->iconPixels, 20);
          QCOMPARE(s.ids().count(QStringLiteral("b1")), 1);
      }
      void resetRemovesTheUserCopy()
      {
          auto s = store();
          QString error;
          Profile p = *s.load(QStringLiteral("b2"), error);
          p.iconPixels = 18;
          QVERIFY(s.save(p, error));
          QVERIFY(s.reset(QStringLiteral("b2"), error));
          QCOMPARE(s.load(QStringLiteral("b2"), error)->iconPixels, 24);
      }
      void builtinCannotBeRemoved()
      {
          auto s = store();
          QString error;
          QVERIFY(!s.remove(QStringLiteral("b1"), error));
          QVERIFY(s.ids().contains(QStringLiteral("b1")));
      }
      void saveIsAtomic()
      {
          auto s = store();
          QString error;
          QVERIFY(s.saveAs(sample(), QStringLiteral("c"), QStringLiteral("C"), error));
          const QStringList left = QDir(m_user.path()).entryList(QDir::Files);
          QCOMPARE(left, QStringList{QStringLiteral("c.json")}); // aucun fichier temporaire laissé
      }
      void exportImportRoundTrip()
      {
          auto s = store();
          QString error;
          QVERIFY(s.saveAs(sample(), QStringLiteral("d"), QStringLiteral("D"), error));
          QVERIFY(writeFile(s.directory(QStringLiteral("d")) + QStringLiteral("/layout.json"), "{\"x\":1}") ||
                  (QDir().mkpath(s.directory(QStringLiteral("d"))) &&
                   writeFile(s.directory(QStringLiteral("d")) + QStringLiteral("/layout.json"), "{\"x\":1}")));
          const QString file = m_user.filePath(QStringLiteral("export.json"));
          QVERIFY2(s.exportTo(QStringLiteral("d"), file, error), qPrintable(error));
          QTemporaryDir other;
          ProfileStore fresh(other.path(), m_builtin.path(), {QStringLiteral("b1"), QStringLiteral("b2")});
          const auto id = fresh.importFrom(file, error);
          QVERIFY2(id, qPrintable(error));
          QCOMPARE(*id, QStringLiteral("d"));
          QCOMPARE(fresh.load(*id, error)->iconPixels, 32);
          QVERIFY(QFile::exists(fresh.directory(*id) + QStringLiteral("/layout.json")));
      }
      void importNeverOverwrites()
      {
          auto s = store();
          QString error;
          QVERIFY(s.saveAs(sample(), QStringLiteral("e"), QStringLiteral("E"), error));
          const QString file = m_user.filePath(QStringLiteral("e-export.json"));
          QVERIFY(s.exportTo(QStringLiteral("e"), file, error));
          QCOMPARE(*s.importFrom(file, error), QStringLiteral("e-2"));
          QCOMPARE(*s.importFrom(file, error), QStringLiteral("e-3"));
      }
  };

  QTEST_MAIN(TstActionsProfile)
  #include "tst_actions_profile.moc"
  ```
  (`init()` recrée le dossier utilisateur : si le placement `new` gêne `/W4`, garder un `std::unique_ptr<QTemporaryDir>`
  recréé dans `init()` — même comportement.)
- [ ] **Étape 4 : rouge** (`-R tst_actions_profile`) : édition de liens en échec (`profileToJson` non défini).
- [ ] **Étape 5 : implémentation** `profile.cpp` : clés JSON `kebab-case` à nous — `format`, `id`, `label`,
  `icon-pixels`, `palette` (`dock`, `float-x`, `float-y`, `collapsed`, `shortcuts-hidden`, `pinned`, `swatches`),
  `browsers` (`visible`, `side`, `pinned`, `current`, `width`), `extra-colors`, `recent-tools`, `pinned`, `fit-page`,
  `trash-visible`, `shortcut-objects`, `launchers`, `settings`. Lecture : chaque clé connue est retirée d'une copie de
  l'objet lu, ce qui reste va dans `unknownKeys` ; écriture : clés connues puis `unknownKeys` (sans écraser une clé
  connue), `QJsonDocument::Indented`. Valeurs hors domaine ramenées au défaut (`swatches` ∉ {8,16,24} → 16 ; `dock`
  inconnu → `right`) ; `icon-pixels` par `clampIconPixels`. `recent-tools` tronqué à 8.
  `profilestore.cpp` : `ids()` = `m_builtinOrder` filtré par l'existence du fichier, puis `QDir(m_userRoot)` `*.json`
  dont le nom n'est pas fourni, triés ; `load` lit la copie utilisateur si elle existe, sinon le fourni ;
  `save` par `QSaveFile` après `QDir().mkpath(m_userRoot)` ; `saveAs` vérifie `^[a-z0-9-]+$` et l'absence de
  l'identifiant ; `remove` supprime le fichier et le dossier `directory(id)` ; `reset` supprime la copie et le
  dossier ; `exportTo` lit `directory(id)/layout.json` s'il existe ; `importFrom` choisit le premier identifiant libre
  `<id>`, `<id>-2`, `<id>-3`…, écrit le profil (avec cet identifiant) et la disposition.
- [ ] **Étape 6 : vert** (`-R tst_actions_profile`), fonctions du test toutes en `PASS` ; preuve de tâche.
- [ ] **Étape 7 : commit.** `git add` des cinq fichiers ; `NOMS OK` ; commit « Lot 6 : profils à nous et leur magasin
  (L6-T4) ».

<a id="tache-l6-t5"></a>
### Tâche L6-T5 — `ui` : réglages du poste et contrôleur de profil

**Objectif.** Deux objets exposés au QML : `ui::Preferences` (réglages du poste par `QSettings`, D6-1) et
`ui::ProfileController` (profil courant chargé par `actions::ProfileStore`, setters de chaque réglage d'interface,
enregistrement implicite différé de 500 ms et **jamais pendant un contact**, D6-4 ; activation d'un profil qui
recharge la disposition par `Commands::setProfileDir`, A1). Le patron évident est le mauvais : enregistrer à chaque
setter écrirait sur disque pendant un glisser de palette ou de réglette, sur le fil principal.

**Fichiers.** `ui/include/actcraft/ui/preferences.h`, `ui/src/preferences.cpp`,
`ui/include/actcraft/ui/profilecontroller.h`, `ui/src/profilecontroller.cpp`, `app/main.cpp`,
`tests/tst_ui_preferences.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_preferences` ; sept fonctions en `PASS` ; banc sans écran.

**Interdits.** Aucune clé `QSettings` en `camelCase` (D6-1) ; aucun accès disque sur le fil principal pendant un
contact ; ne pas modifier `commands.*` (A1 suffit).

**Terminé quand** les sept fonctions passent ; `app/main.cpp` crée les deux objets et les expose (`appPreferences`,
`appProfile`) ; au démarrage, le profil courant est relu ou, à défaut, `preparation`.

**Étapes.**
- [ ] **Étape 1 : en-têtes.** `preferences.h` :
  ```cpp
  #pragma once

  #include <QObject>
  #include <QSettings>
  #include <QStringList>
  #include <QtQml/qqmlregistration.h>

  namespace actcraft::ui {

  // Réglages propres au poste (D6-1), clés kebab-case à nous. Le profil, lui, est dans ProfileController.
  class Preferences : public QObject
  {
      Q_OBJECT
      QML_ELEMENT
      QML_UNCREATABLE("créé par l'application")
      Q_PROPERTY(QString currentProfile READ currentProfile WRITE setCurrentProfile NOTIFY changed)
      Q_PROPERTY(bool dashboardAtStart READ dashboardAtStart WRITE setDashboardAtStart NOTIFY changed)
      Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY changed)
  public:
      static constexpr const char* kCurrentProfile = "workstation/current-profile";
      static constexpr const char* kDashboardAtStart = "workstation/dashboard-at-start";
      static constexpr const char* kLanguage = "workstation/language";
      static constexpr const char* kRecent = "workstation/recent-documents";

      explicit Preferences(QObject* parent = nullptr); // QSettings par défaut (organisation et application)
      Preferences(const QString& iniPath, QObject* parent); // tests : fichier INI

      QString currentProfile() const;
      void setCurrentProfile(const QString& id);
      bool dashboardAtStart() const; // vrai par défaut (F-090)
      void setDashboardAtStart(bool on);
      QString language() const; // vide : langue du système
      void setLanguage(const QString& code);

      QVariant value(const QString& key, const QVariant& fallback = {}) const;
      void setValue(const QString& key, const QVariant& value);
      void remove(const QString& key);
      QStringList allKeys() const;

  signals:
      void changed();

  private:
      std::unique_ptr<QSettings> m_settings;
  };

  } // namespace actcraft::ui
  ```
  `profilecontroller.h` :
  ```cpp
  #pragma once

  #include <actcraft/actions/profilestore.h>

  #include <QObject>
  #include <QTimer>
  #include <QVariantList>
  #include <QVariantMap>
  #include <QtQml/qqmlregistration.h>

  namespace actcraft::ui {

  class Commands;
  class Preferences;

  class ProfileController : public QObject
  {
      Q_OBJECT
      QML_ELEMENT
      QML_UNCREATABLE("créé par l'application")
      Q_PROPERTY(QString currentId READ currentId NOTIFY profileChanged)
      Q_PROPERTY(QString label READ label NOTIFY profileChanged)
      Q_PROPERTY(QVariantList profiles READ profiles NOTIFY listChanged) // [{id, label, builtin, modified}]
      Q_PROPERTY(int iconPixels READ iconPixels WRITE setIconPixels NOTIFY profileChanged)
      Q_PROPERTY(QString paletteDock READ paletteDock WRITE setPaletteDock NOTIFY profileChanged)
      Q_PROPERTY(QPointF paletteFloat READ paletteFloat WRITE setPaletteFloat NOTIFY profileChanged)
      Q_PROPERTY(bool paletteCollapsed READ paletteCollapsed WRITE setPaletteCollapsed NOTIFY profileChanged)
      Q_PROPERTY(bool shortcutsHidden READ shortcutsHidden WRITE setShortcutsHidden NOTIFY profileChanged)
      Q_PROPERTY(bool palettePinned READ palettePinned WRITE setPalettePinned NOTIFY profileChanged)
      Q_PROPERTY(int swatches READ swatches WRITE setSwatches NOTIFY profileChanged)
      Q_PROPERTY(bool browsersVisible READ browsersVisible WRITE setBrowsersVisible NOTIFY profileChanged)
      Q_PROPERTY(QString browsersSide READ browsersSide WRITE setBrowsersSide NOTIFY profileChanged)
      Q_PROPERTY(QString browsersCurrent READ browsersCurrent WRITE setBrowsersCurrent NOTIFY profileChanged)
      Q_PROPERTY(QStringList extraColors READ extraColors NOTIFY profileChanged)
      Q_PROPERTY(QStringList recentTools READ recentTools NOTIFY profileChanged)
      Q_PROPERTY(QStringList pinned READ pinned NOTIFY profileChanged)
      Q_PROPERTY(bool fitPage READ fitPage NOTIFY profileChanged)
      Q_PROPERTY(bool trashVisible READ trashVisible NOTIFY profileChanged)
      Q_PROPERTY(bool inputActive READ inputActive WRITE setInputActive NOTIFY inputActiveChanged)
      Q_PROPERTY(int saveCount READ saveCount NOTIFY saved) // tests et scénarios
  public:
      ProfileController(actions::ProfileStore store, Preferences* preferences, Commands* commands,
                        QObject* parent = nullptr);

      static QStringList builtinOrder(); // en-classe, preparation, lecture-ecriture, calcul-geometrie, son-image
      static QString defaultProfileId(); // « preparation »

      // Lit le profil courant des préférences ; à défaut, ou s'il ne se lit pas, defaultProfileId().
      void start();
      Q_INVOKABLE bool activate(const QString& id);
      Q_INVOKABLE void saveNow();
      const actions::Profile& profile() const noexcept;
      actions::Profile& editProfile(); // modifie puis appelle touch()
      void touch();                    // modification : signal, puis enregistrement différé
      actions::ProfileStore& store() noexcept;

      Q_INVOKABLE void addColor(const QString& color);       // F-135, sans doublon
      Q_INVOKABLE void pushRecentTool(const QString& id);    // F-140
      Q_INVOKABLE QVariant setting(const QString& section, const QString& key, const QVariant& fallback) const;
      Q_INVOKABLE void setSetting(const QString& section, const QString& key, const QVariant& value);

      QString currentId() const;
      QString label() const;
      QVariantList profiles() const;
      int iconPixels() const;
      void setIconPixels(int pixels);
      QString paletteDock() const;
      void setPaletteDock(const QString& dock);
      QPointF paletteFloat() const;
      void setPaletteFloat(QPointF point);
      bool paletteCollapsed() const;
      void setPaletteCollapsed(bool on);
      bool shortcutsHidden() const;
      void setShortcutsHidden(bool on);
      bool palettePinned() const;
      void setPalettePinned(bool on);
      int swatches() const;
      void setSwatches(int count);
      bool browsersVisible() const;
      void setBrowsersVisible(bool on);
      QString browsersSide() const;
      void setBrowsersSide(const QString& side);
      QString browsersCurrent() const;
      void setBrowsersCurrent(const QString& tab);
      QStringList extraColors() const;
      QStringList recentTools() const;
      QStringList pinned() const;
      bool fitPage() const;
      bool trashVisible() const;
      bool inputActive() const noexcept;
      void setInputActive(bool active); // lié par Main.qml à la garde de contact de la vue (A6)
      int saveCount() const noexcept;

      static constexpr int kSaveDelayMs = 500;

  signals:
      void profileChanged();
      void listChanged();
      void inputActiveChanged();
      void saved();
      void failed(const QString& message);

  private:
      void scheduleSave();
      actions::ProfileStore m_store;
      Preferences* m_preferences;
      Commands* m_commands;
      actions::Profile m_profile;
      QTimer m_saveTimer;
      bool m_inputActive = false;
      bool m_dirty = false;
      int m_saveCount = 0;
  };

  } // namespace actcraft::ui
  ```
- [ ] **Étape 2 : test rouge** `tests/tst_ui_preferences.cpp` (remplace le talon ; `QStandardPaths::setTestModeEnabled(true)`
  dans `initTestCase`, dossiers temporaires, `Commands` réel) :
  ```cpp
  #include <actcraft/ui/commands.h>
  #include <actcraft/ui/preferences.h>
  #include <actcraft/ui/profilecontroller.h>

  #include <QFile>
  #include <QRegularExpression>
  #include <QSignalSpy>
  #include <QTemporaryDir>
  #include <QtTest>

  using namespace actcraft;

  class TstUiPreferences : public QObject
  {
      Q_OBJECT
      QTemporaryDir m_dir;
      QString ini() const { return m_dir.filePath(QStringLiteral("poste.ini")); }
      std::unique_ptr<ui::ProfileController> controller(ui::Preferences* prefs, ui::Commands* commands)
      {
          actions::ProfileStore store(m_dir.filePath(QStringLiteral("profiles")), QStringLiteral(":/actcraft/actions/profiles"),
                                      ui::ProfileController::builtinOrder());
          auto c = std::make_unique<ui::ProfileController>(std::move(store), prefs, commands);
          c->start();
          return c;
      }
  private slots:
      void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
      void keysAreOurs()
      {
          ui::Preferences prefs(ini(), nullptr);
          prefs.setCurrentProfile(QStringLiteral("en-classe"));
          prefs.setDashboardAtStart(false);
          prefs.setLanguage(QStringLiteral("fr"));
          const QRegularExpression kebab(QStringLiteral("^[a-z]+(-[a-z]+)*(/[a-z]+(-[a-z]+)*)+$"));
          for (const QString& key : prefs.allKeys())
              QVERIFY2(kebab.match(key).hasMatch(), qPrintable(key));
      }
      void currentProfileRestored()
      {
          ui::Commands commands;
          {
              ui::Preferences prefs(ini(), nullptr);
              auto c = controller(&prefs, &commands);
              QVERIFY(c->activate(QStringLiteral("calcul-geometrie")));
          }
          ui::Preferences prefs(ini(), nullptr);
          auto c = controller(&prefs, &commands);
          QCOMPARE(c->currentId(), QStringLiteral("calcul-geometrie"));
      }
      void implicitSaveIsDebounced()
      {
          ui::Commands commands;
          ui::Preferences prefs(ini(), nullptr);
          auto c = controller(&prefs, &commands);
          const int before = c->saveCount();
          for (int i = 16; i <= 32; ++i)
              c->setIconPixels(i);
          QCOMPARE(c->saveCount(), before);
          QTRY_COMPARE_WITH_TIMEOUT(c->saveCount(), before + 1, 2000);
          QTest::qWait(ui::ProfileController::kSaveDelayMs + 200);
          QCOMPARE(c->saveCount(), before + 1);
      }
      void noSaveDuringContact()
      {
          ui::Commands commands;
          ui::Preferences prefs(ini(), nullptr);
          auto c = controller(&prefs, &commands);
          const int before = c->saveCount();
          c->setInputActive(true);
          c->setPaletteDock(QStringLiteral("left"));
          QTest::qWait(ui::ProfileController::kSaveDelayMs * 3);
          QCOMPARE(c->saveCount(), before);
          c->setInputActive(false);
          QTRY_COMPARE_WITH_TIMEOUT(c->saveCount(), before + 1, 2000);
      }
      void activateSwitchesTheLayoutDir()
      {
          ui::Commands commands;
          ui::Preferences prefs(ini(), nullptr);
          auto c = controller(&prefs, &commands);
          QDir().mkpath(c->store().directory(QStringLiteral("en-classe")));
          QFile f(c->store().directory(QStringLiteral("en-classe")) + QStringLiteral("/layout.json"));
          QVERIFY(f.open(QIODevice::WriteOnly));
          f.write("{");            // disposition illisible : erreur visible dans Commands::errors
          f.close();
          QVERIFY(c->activate(QStringLiteral("en-classe")));
          QVERIFY(!commands.errors().isEmpty());
          QVERIFY(c->activate(QStringLiteral("preparation")));
          QVERIFY(commands.errors().isEmpty());
      }
      void brokenProfileFallsBack()
      {
          ui::Commands commands;
          ui::Preferences prefs(ini(), nullptr);
          prefs.setCurrentProfile(QStringLiteral("absent"));
          auto c = controller(&prefs, &commands);
          QCOMPARE(c->currentId(), ui::ProfileController::defaultProfileId());
      }
      void settingDefaultsComeFromTheSchema()
      {
          ui::Commands commands;
          ui::Preferences prefs(ini(), nullptr);
          auto c = controller(&prefs, &commands);
          QCOMPARE(c->setting(QStringLiteral("objects"), QStringLiteral("auto-group"), false).toBool(), false);
          c->setSetting(QStringLiteral("objects"), QStringLiteral("auto-group"), true);
          QCOMPARE(c->setting(QStringLiteral("objects"), QStringLiteral("auto-group"), false).toBool(), true);
      }
  };

  QTEST_MAIN(TstUiPreferences)
  #include "tst_ui_preferences.moc"
  ```
  Note : tant que L6-T6 n'a pas rempli les profils fournis, les talons JSON de L6-T1 sont lisibles (ils ont `format`,
  `id`, `label`) : ce test ne dépend pas de leur contenu.
- [ ] **Étape 3 : rouge** (`-R tst_ui_preferences`) : édition de liens en échec.
- [ ] **Étape 4 : implémentation.** `Preferences` : `QSettings` par défaut (organisation « ActCraft », application
  « ActCraft », déjà posées par `app/main.cpp`) ou `QSettings(iniPath, QSettings::IniFormat)` ; chaque setter écrit puis
  émet `changed`. `ProfileController` : `start()` lit `currentProfile`, `activate` ; `activate(id)` charge par le
  magasin (échec : `failed(message)`, profil inchangé, rend faux), puis
  `m_commands->setProfileDir(QFileInfo::exists(store.directory(id) + "/layout.json") ? store.directory(id) : QString())`
  — si `Commands::setProfileDir` refuse une chaîne vide (A1), passer un dossier vide existant
  (`<AppConfigLocation>/profiles/.vide`) ; écrit `currentProfile`, émet `profileChanged` et `listChanged`. Chaque setter
  modifie `m_profile` puis `touch()` ; `touch()` émet `profileChanged`, pose `m_dirty`, et `scheduleSave()` relance
  `m_saveTimer` (simple, 500 ms) ; à l'échéance, si `m_inputActive`, ne rien faire (le relâché relancera) ; sinon
  `saveNow()` : `store.save(m_profile)`, `++m_saveCount`, `saved()`. `setInputActive(false)` relance la minuterie si
  `m_dirty`. Le profil fourni modifié devient une copie utilisateur (le magasin s'en charge).
- [ ] **Étape 5 : branchement.** `app/main.cpp` : après la création de `Commands`, créer `Preferences` puis
  `ProfileController` (racine utilisateur `QStandardPaths::writableLocation(AppConfigLocation) + "/profiles"`),
  `start()`, exposer `appPreferences` et `appProfile` comme les autres objets de l'application (même voie que
  `appCommands` : propriétés de contexte ou initialisation des propriétés requises, lire le fichier). `Main.qml` sera
  relié à `inputActive` par L6-T7 (tâche QML) : ne pas toucher au QML ici.
- [ ] **Étape 6 : vert** (`-R 'tst_ui_preferences|tst_ui_commands'`), preuve de tâche, banc sans écran.
- [ ] **Étape 7 : commit.** `git add` des six fichiers ; `NOMS OK` ; commit « Lot 6 : réglages du poste et profil
  courant, enregistrement implicite hors écriture (L6-T5) ».

<a id="tache-l6-t6"></a>
### Tâche L6-T6 — Cinq profils fournis, à nos noms

**Objectif.** Écrire les cinq profils de D6-2 (usage et disposition de l'original, nos noms et nos contenus ; D1 de
L6-T0 fait foi pour ce que chacun ouvre, épingle et règle), avec les épinglages du lot 7 lus dans le fichier de
L7-T32 (`docs/verifications/*-lot7-epinglages.md`, A15). Scénario `profils` : activer chaque profil par
`profile.activate`.

**Fichiers.** `actions/data/profiles/{en-classe,preparation,lecture-ecriture,calcul-geometrie,son-image}.json`,
`tests/tst_ui_profiles.cpp`, `bench/scenarios/profils.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_profiles|tst_captures_profils'` ; preuve de tâche.

**Preuve visuelle.** `captures.ps1 -Scenario profils -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "appProfile.currentId=calcul-geometrie"` :
`LU appProfile.currentId=calcul-geometrie OK`, `CAPTURES OK` ; décrire `preparation.png` (navigateur de pages ouvert à
gauche) et `calcul-geometrie.png` (groupe « profil » de la palette avec les outils de maths).

**Interdits.** Aucun nom de profil de l'original (« Au tableau », « Création », « Langues », « Mathématiques »,
« Médias » restent hors du dépôt, sauf comme renvoi F-880 dans ce plan) ; aucune commande inventée : un épinglage
non déclaré est **retiré** et listé au rapport (le lot 8 l'ajoutera).

**Terminé quand** les cinq fonctions passent et le scénario est vert.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_ui_profiles.cpp` :
  ```cpp
  #include <actcraft/actions/commandregistry.h>
  #include <actcraft/actions/profilestore.h>
  #include <actcraft/ui/profilecontroller.h>

  #include <QtTest>

  using namespace actcraft;

  class TstUiProfiles : public QObject
  {
      Q_OBJECT
      static actions::ProfileStore builtins()
      {
          return actions::ProfileStore(QString(), QStringLiteral(":/actcraft/actions/profiles"),
                                       ui::ProfileController::builtinOrder());
      }
  private slots:
      void fiveBuiltinsInOrder()
      {
          QCOMPARE(builtins().ids(), (QStringList{QStringLiteral("en-classe"), QStringLiteral("preparation"),
                                                  QStringLiteral("lecture-ecriture"), QStringLiteral("calcul-geometrie"),
                                                  QStringLiteral("son-image")}));
      }
      void pinnedCommandsExist()
      {
          actions::CommandRegistry registry;
          QString error;
          QVERIFY(registry.loadDeclarations(actions::defaultCommandsJson(), error));
          auto store = builtins();
          for (const QString& id : store.ids()) {
              const auto p = store.load(id, error);
              QVERIFY2(p, qPrintable(error));
              QVERIFY2(!p->label.isEmpty(), qPrintable(id));
              for (const QString& command : p->pinned)
                  QVERIFY2(registry.find(command), qPrintable(id + QStringLiteral(" : ") + command));
          }
      }
      void pinnedFilteredByPlatform()
      {
          actions::CommandRegistry registry;
          QString error;
          QVERIFY(registry.loadDeclarations(actions::defaultCommandsJson(), error));
          auto store = builtins();
          int filtered = 0;
          for (const QString& id : store.ids())
              for (const QString& command : store.load(id, error)->pinned)
                  if (!registry.isAvailable(command, actions::Platform::Android))
                      ++filtered;
          QVERIFY(filtered >= 1); // au moins le clavier à l'écran ou la reconnaissance d'écriture
      }
      void namesAreOurs()
      {
          auto store = builtins();
          QString error;
          const QStringList theirs = {QStringLiteral("au tableau"), QStringLiteral("création"),
                                      QStringLiteral("langues"), QStringLiteral("mathématiques"),
                                      QStringLiteral("médias")};
          for (const QString& id : store.ids())
              QVERIFY2(!theirs.contains(store.load(id, error)->label.toLower()), qPrintable(id));
      }
      void preparationOpensThePageBrowser()
      {
          QString error;
          const auto p = builtins().load(QStringLiteral("preparation"), error);
          QVERIFY(p->browsers.visible);
          QCOMPARE(p->browsers.current, QStringLiteral("pages"));
          QVERIFY(p->browsers.pinned);
          QCOMPARE(p->palette.swatches, 24);
          const auto c = builtins().load(QStringLiteral("en-classe"), error);
          QVERIFY(c->fitPage);
          QVERIFY(!c->browsers.visible);
          QVERIFY(c->trashVisible);
      }
  };

  QTEST_MAIN(TstUiProfiles)
  #include "tst_ui_profiles.moc"
  ```
- [ ] **Étape 2 : rouge** (`-R tst_ui_profiles`) : `preparationOpensThePageBrowser` échoue (talons).
- [ ] **Étape 3 : données.** Exemple complet de `preparation.json` (les quatre autres sur la même forme, valeurs de
  D6-2 et de D1) :
  ```json
  {
    "format": "actcraft-profile/1",
    "id": "preparation",
    "label": "Préparer un cours",
    "icon-pixels": 24,
    "palette": { "dock": "right", "collapsed": false, "shortcuts-hidden": false, "pinned": true, "swatches": 24 },
    "browsers": { "visible": true, "side": "left", "pinned": true, "current": "pages", "width": 240 },
    "fit-page": false,
    "trash-visible": true,
    "pinned": [],
    "settings": { "defaults": { "page-size": "1024x768" } }
  }
  ```
  `en-classe` : `fit-page` vrai, navigateurs masqués, `pinned` = `browser.resources` puis les commandes de
  présentation de la liste de L7-T32 (`present.*`, `view.page-zoom`, `tool.clock` selon la liste) ; `lecture-ecriture` :
  clavier à l'écran, reconnaissance d'écriture, message défilant (identifiants du lot 7), `spell.check` ;
  `calcul-geometrie` : `math.*`, `tool.dice`, `tool.calculator` ; `son-image` : `camera.*` du lot 7 et `insert.image`
  (lot 5, A11). Les quatre profils autres que `preparation` : `fit-page` vrai, navigateurs masqués, corbeille visible,
  `swatches` 16.
- [ ] **Étape 4 : scénario** `bench/scenarios/profils.json` : `waitReady` ; pour chaque profil :
  `{"command": "profile.activate", "argument": "<id>"}`, `{"expect": "appProfile.currentId", "equals": "<id>"}`,
  capture `<id>.png` ; terminer sur `calcul-geometrie`. Forme des étapes : celle du joueur (A14), relue dans
  `bench/scenarios/premiers-pas.json` ; description en français sans marqueur de talon.
- [ ] **Étape 5 : vert** (commande ciblée), preuve visuelle, preuve de tâche.
- [ ] **Étape 6 : commit.** `git add` des sept fichiers ; `NOMS OK` ; contrôle des titres du corpus vide ; commit
  « Lot 6 : cinq profils fournis à nos noms (L6-T6) ».

<a id="tache-l6-t7"></a>
### Tâche L6-T7 — Palette : ancrage, réduite, épinglée, taille des icônes

**Objectif.** `PaletteFrame.qml` enveloppe la palette : ancrée à gauche, à droite, en haut, en bas, ou flottante
(F-030 ; glisser sa barre de titre vers le centre la rend flottante, vers un bord l'y ancre) ; barre de titre à trois
icônes (F-031 : menu d'options avec les ancrages, réduire, épingler) ; réduite = première rangée seulement ; réduite au
maximum = barre de raccourcis masquée (F-032) ; désépinglée et ancrée = repli dans la marge quand le pointeur s'en
éloigne, retour au survol (délai de D7, défaut 600 ms) ; taille des icônes 16 à 32 appliquée à toute la fenêtre par
`Theme.iconPixels` (F-034). Tout passe par `appProfile` (persistance F-033, L6-T5). Gestionnaires des commandes
`palette.*`. `Main.qml` relie `appProfile.inputActive` à la garde de contact de la vue (A6) — une ligne, dans le
bloc posé par L6-T2 pour cette tâche (fichier hors de la ligne : la liaison s'écrit dans `PaletteFrame.qml` par un
`Binding { target: appProfile; property: "inputActive"; value: pageView.inputActive }`).

**Fichiers.** `ui/qml/PaletteFrame.qml`, `ui/qml/ToolPalette.qml`, `ui/qml/Theme.qml`, `tests/tst_ui_palette.cpp`,
`bench/scenarios/palette.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_palette|tst_captures_palette|tst_ui_shell'` ; banc sans écran.

**Preuve visuelle.** `captures.ps1 -Scenario palette … -Lire "appProfile.paletteDock=left" -Lire "toolPalette.x=0"` :
`LU … OK` (deux lignes), `CAPTURES OK` ; décrire `gauche.png`, `reduite.png`, `repliee.png`, `icones-32.png`.

**Interdits.** La palette flottante ne recouvre pas la page sans être déclarée couverture d'entrée
(`pageView.addInputCover(paletteFrame)`, A7) ; aucune animation qui redessine la page (règle 4 de fluidité :
`opacity` et `x` seulement, sur la palette) ; aucun accès disque en QML.

**Terminé quand** les six fonctions passent, le scénario est vert et le banc sans écran est joint.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_ui_palette.cpp` : montage QML de `tst_ui_shell` (charger `Main.qml`, objets
  de l'application réels, profil dans un dossier temporaire par `QStandardPaths::setTestModeEnabled(true)`), fonctions :
  ```cpp
  void dockFollowsTheProfile()
  {
      auto* root = loadMain();
      auto* palette = root->findChild<QQuickItem*>(QStringLiteral("toolPalette"));
      appProfile()->setPaletteDock(QStringLiteral("left"));
      QTRY_COMPARE(palette->mapToScene(QPointF(0, 0)).x(), 0.0);
      appProfile()->setPaletteDock(QStringLiteral("right"));
      QTRY_COMPARE(palette->mapToScene(QPointF(palette->width(), 0)).x(), double(window()->width()));
  }
  void dragToCenterFloats()
  {
      auto* root = loadMain();
      auto* title = root->findChild<QQuickItem*>(QStringLiteral("paletteTitle"));
      const QPoint from = title->mapToScene(QPointF(title->width() / 2, title->height() / 2)).toPoint();
      const QPoint to(window()->width() / 2, window()->height() / 2);
      QTest::mousePress(window(), Qt::LeftButton, {}, from);
      QTest::mouseMove(window(), to);
      QTest::mouseRelease(window(), Qt::LeftButton, {}, to);
      QCOMPARE(appProfile()->paletteDock(), QStringLiteral("floating"));
  }
  void collapsedShowsFirstRowOnly()
  {
      auto* root = loadMain();
      appProfile()->setPaletteCollapsed(true);
      auto* rows = root->findChild<QQuickItem*>(QStringLiteral("paletteRows"));
      QTRY_COMPARE(rows->property("shownRows").toInt(), 1);
  }
  void fullCollapseHidesShortcuts()
  {
      auto* root = loadMain();
      appProfile()->setShortcutsHidden(true);
      QTRY_VERIFY(!root->findChild<QQuickItem*>(QStringLiteral("shortcutBar"))->isVisible());
  }
  void unpinnedHidesAfterLeave()
  {
      auto* root = loadMain();
      appProfile()->setPaletteDock(QStringLiteral("right"));
      appProfile()->setPalettePinned(false);
      QTest::mouseMove(window(), QPoint(10, window()->height() / 2));
      auto* frame = root->findChild<QQuickItem*>(QStringLiteral("paletteFrame"));
      QTRY_VERIFY_WITH_TIMEOUT(frame->property("folded").toBool(), 3000);
      QTest::mouseMove(window(), QPoint(window()->width() - 2, window()->height() / 2));
      QTRY_VERIFY_WITH_TIMEOUT(!frame->property("folded").toBool(), 3000);
  }
  void iconPixelsScaleEveryIcon()
  {
      auto* root = loadMain();
      appProfile()->setIconPixels(32);
      const auto buttons = root->findChildren<QQuickItem*>(QStringLiteral("commandIcon"));
      QVERIFY(!buttons.isEmpty());
      for (auto* icon : buttons)
          QTRY_COMPARE(icon->width(), 32.0);
  }
  ```
  (`loadMain`, `appProfile`, `window` : aides du fichier, sur le modèle de `tst_ui_shell.cpp`. Si `CommandButton.qml`
  ne nomme pas son image `commandIcon`, lui donner ce nom fait partie de la tâche — fichier `ui/qml/CommandButton.qml`
  ajouté à la ligne par amendement si besoin, signalé au rapport.)
- [ ] **Étape 2 : rouge.**
- [ ] **Étape 3 : QML.** `Theme.qml` : `property int iconPixels: appProfile ? appProfile.iconPixels : 24`, utilisé par
  toutes les tailles d'icônes (remplacer les constantes `24` des icônes par `Theme.iconPixels`). `PaletteFrame.qml` :
  `property bool folded`, ancrage par états (`left`, `right`, `top`, `bottom`, `floating`) lus de
  `appProfile.paletteDock`, barre `paletteTitle` (glisser : `DragHandler` ; relâché à moins de 48 px d'un bord → ce
  bord, sinon `floating` + `appProfile.paletteFloat`), menu d'options (`CommandMenu` des commandes `palette.*`),
  `HoverHandler` + `Timer` (600 ms) pour le repli, `Behavior on x` (120 ms). `ToolPalette.qml` : `paletteRows` avec
  `shownRows` (1 si `appProfile.paletteCollapsed`), groupe « profil » final = `appProfile.pinned` filtré par
  `appCommands.isAvailable`, `shortcutBar.visible = !appProfile.shortcutsHidden && !appProfile.paletteCollapsed`. Les
  gestionnaires `palette.*` (dans `PaletteFrame.qml`, `Component.onCompleted`) appellent les setters de `appProfile` ;
  `palette.collapse`, `palette.collapse-full`, `palette.pin` tiennent leur coche par `appCommands.setChecked`.
- [ ] **Étape 4 : scénario** `palette.json` : `waitReady` ; `palette.dock-left` ; `expect appProfile.paletteDock = left` ;
  capture `gauche.png` ; `palette.collapse` ; capture `reduite.png` ; `palette.collapse` ; `palette.pin` (désépingle) ;
  `pointer` déplacé loin de la palette, attente 1 s ; `expect paletteFrame.folded = true` ; capture `repliee.png` ;
  `palette.icon-size 32` ; capture `icones-32.png`.
- [ ] **Étape 5 : vert**, preuve visuelle, banc sans écran, preuve de tâche.
- [ ] **Étape 6 : commit** « Lot 6 : palette ancrable, flottante, réduite, épinglée ; taille des icônes (L6-T7) ».

<a id="tache-l6-t8"></a>
### Tâche L6-T8 — Palette de couleurs étendue et copieur de couleur

**Objectif.** Clic droit sur une pastille (F-135) : grille étendue de 6 × 6 couleurs (contenu de D6, défaut : six
teintes × six clartés calculées par nous), avec deux boutons : copieur de couleur (une loupe suit le pointeur dans la
fenêtre et montre la couleur visée ; un clic l'ajoute) et boîte de couleur (`QtQuick.Dialogs.ColorDialog`). La couleur
choisie devient la couleur du stylo et s'ajoute à la palette (`appProfile.addColor`), sans doublon ; nombre de
pastilles 8, 16 ou 24 (F-900, profil). `ui::ColorPalette` (C++) calcule la grille, la liste affichée et
l'échantillonnage de la loupe : **une seule** capture de la fenêtre à l'ouverture du copieur (`QQuickWindow::grabWindow`),
lue ensuite au pixel — jamais une capture par mouvement.

**Fichiers.** `ui/include/actcraft/ui/colorpalette.h`, `ui/src/colorpalette.cpp`, `ui/qml/ExtendedColors.qml`,
`ui/qml/ColorLoupe.qml`, `ui/qml/ColorSwatches.qml`, `tests/tst_ui_colors.cpp`, `bench/scenarios/couleurs.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_colors|tst_captures_couleurs'` ; banc sans écran.

**Preuve visuelle.** `captures.ps1 -Scenario couleurs … -Lire "appColors.extraCount=1"` : `LU appColors.extraCount=1 OK`,
`PIXEL pastille-ajoutee OK` (la dernière pastille a la couleur ajoutée), `CAPTURES OK` ; décrire `etendue.png`,
`loupe.png`.

**Interdits.** Le copieur est une couverture d'entrée tant qu'il est ouvert (A7) : aucun trait n'est tracé par son
clic ; aucune capture par mouvement de pointeur.

**Terminé quand** les cinq fonctions passent, scénario vert, banc joint.

**Étapes.**
- [ ] **Étape 1 : en-tête** `colorpalette.h` :
  ```cpp
  #pragma once

  #include <QColor>
  #include <QImage>
  #include <QObject>
  #include <QStringList>
  #include <QtQml/qqmlregistration.h>

  namespace actcraft::ui {

  class ProfileController;

  class ColorPalette : public QObject
  {
      Q_OBJECT
      QML_ELEMENT // instancié par ColorSwatches.qml (aucun fichier de l'application touché)
      Q_PROPERTY(ProfileController* profile MEMBER m_profile NOTIFY changed)
      Q_PROPERTY(QStringList baseColors MEMBER m_base NOTIFY changed) // liée à appCommands.colors
      Q_PROPERTY(QStringList swatches READ swatches NOTIFY changed)   // pastilles affichées (8, 16 ou 24)
      Q_PROPERTY(QStringList extended READ extended CONSTANT)         // 36 couleurs « #RRGGBB »
      Q_PROPERTY(int extraCount READ extraCount NOTIFY changed)
  public:
      explicit ColorPalette(QObject* parent = nullptr);
      static QStringList extendedGrid(); // 6 teintes × 6 clartés, ligne par ligne
      QStringList swatches() const;      // couleurs de base puis ajoutées, tronquées au nombre du profil
      QStringList extended() const;
      int extraCount() const;
      Q_INVOKABLE void add(const QString& color); // vers le profil, sans doublon (casse ignorée)
      // Copieur : une capture à l'ouverture, puis lecture au pixel.
      void setSnapshot(QImage image);
      Q_INVOKABLE void beginPick(); // demande la capture à la fenêtre (signal snapshotRequested)
      Q_INVOKABLE QString colorAt(int x, int y) const; // vide hors image
      int snapshotCount() const noexcept;
  signals:
      void changed();
      void snapshotRequested();
  private:
      QStringList m_base;
      ProfileController* m_profile = nullptr;
      QImage m_snapshot;
      int m_snapshots = 0;
  };

  } // namespace actcraft::ui
  ```
- [ ] **Étape 2 : test rouge** `tst_ui_colors.cpp` : `extendedGridIsSixBySix` (36 couleurs distinctes, valides) ;
  `addedColorIsKept` (`add("#12AB34")` → `extraCount` 1, présent dans `appProfile.extraColors`, relu par un second
  `ProfileController` sur le même dossier après `saveNow()`) ; `duplicatesIgnored` (`add("#12ab34")` deux fois et une
  couleur de base → `extraCount` 1) ; `swatchCountFollowsTheProfile` (`setSwatches(8)` → 8 pastilles ; 24 → 24 quand
  il y a assez de couleurs ; une valeur 10 → 16) ; `loupeSamplesOnceOnOpen` (`setSnapshot` d'une image 4 × 4 rouge a- [ ] **Étape 3 : rouge, puis implémentation** (`extendedGrid` : teintes 0, 60, 120, 180, 240, 300 ; clartés 0,15 à
  0,90 par pas de 0,15 ; `QColor::fromHslF`). `ColorPalette` est créable en QML (aucun fichier de l'application
  touché) : `ColorSwatches.qml` l'instancie avec `profile: appProfile` et `baseColors: appCommands.colors` ; il
  s'abonne à `profileChanged` de son profil pour émettre `changed`.
mplacer le constructeur par un constructeur par défaut et des propriétés `profile`
  (`ProfileController*`) et `baseColors` (`QStringList`, liée à `appCommands.colors`).
- [ ] **Étape 4 : QML.** `ColorSwatches.qml` : instance `ColorPalette { id: appColors; objectName: "appColors" }`,
  pastilles de `appColors.swatches`, clic droit → `extendedColors.open()`. `ExtendedColors.qml` : grille 6 × 6, deux
  boutons (`color.pick`, `color.dialog`). `ColorLoupe.qml` : sur `color.pick`, `window.grabWindow` → `setSnapshot`, loupe
  de 7 × 7 pixels agrandis qui suit le pointeur, clic → `pen.color` + `appColors.add`, puis fermeture ; couverture
  d'entrée tant qu'ouverte. Gestionnaires `color.more`, `color.pick`, `color.dialog`, `color.add`.
- [ ] **Étape 5 : scénario** `couleurs.json` : `waitReady` ; clic droit injecté sur la première pastille ; capture
  `etendue.png` ; `color.add #12AB34` ; `expect appColors.extraCount = 1` ; contrôle `pastille-ajoutee` (pixel du centre
  de la dernière pastille = `#12AB34`, tolérance 8) ; `color.pick` ; capture `loupe.png` ; touche Échap.
- [ ] **Étape 6 : vert**, preuve visuelle, banc sans écran, preuve de tâche ; commit « Lot 6 : palette de couleurs
  étendue et copieur de couleur (L6-T8) ».

<a id="tache-l6-t9"></a>
### Tâche L6-T9 — Barre de raccourcis : historique des outils, objets glissés

**Objectif.** Troisième colonne de la palette (F-140) : chaque outil ou fonction déclenché **hors palette** (menu,
raccourci clavier) s'y ajoute en tête, 8 au plus, sans doublon (`appProfile.pushRecentTool`) ; un clic le relance.
F-141 : déposer sur la barre un objet ou un groupe sélectionné crée un bouton à son image ; chaque clic insère une copie
au centre de la page active, **en une étape d'annulation**. Les objets du bouton sont gardés dans le profil
(`shortcutObjects`) par la sérialisation du presse-papiers du lot 5 (A10).

**Fichiers.** `ui/include/actcraft/ui/shortcutbar.h`, `ui/src/shortcutbar.cpp`, `ui/qml/ShortcutBar.qml`,
`tests/tst_ui_shortcutbar.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_shortcutbar` ; banc sans écran.

**Interdits.** Pas de copie du document entier ; l'insertion passe par `DocumentController::execute` (A4) d'**une**
commande (`core::ReplaceObjectsCommand` sans objets retirés, A12, ou la commande d'ajout groupée du lot 5) ; le
bouton n'est pas une ressource (navigateur de ressources : L6-T21).

**Terminé quand** les six fonctions passent.

**Étapes.**
- [ ] **Étape 1 : en-tête** `shortcutbar.h` :
  ```cpp
  #pragma once

  #include <QObject>
  #include <QVariantList>
  #include <QtQml/qqmlregistration.h>

  namespace actcraft::ui {

  class Commands;
  class DocumentController;
  class ProfileController;

  class ShortcutBar : public QObject
  {
      Q_OBJECT
      QML_ELEMENT
      Q_PROPERTY(ProfileController* profile MEMBER m_profile NOTIFY changed)
      Q_PROPERTY(Commands* commands READ commands WRITE setCommands NOTIFY changed)
      Q_PROPERTY(DocumentController* document MEMBER m_document NOTIFY changed)
      Q_PROPERTY(QVariantList buttons READ buttons NOTIFY changed) // [{kind: "tool"|"object", id, label, icon}]
  public:
      explicit ShortcutBar(QObject* parent = nullptr);
      Commands* commands() const noexcept;
      void setCommands(Commands* commands); // écoute Commands::triggered
      QVariantList buttons() const;
      // Appelé par la palette quand elle déclenche une commande : celle-là n'entre pas dans l'historique.
      Q_INVOKABLE void notePaletteTrigger(const QString& id);
      // F-141 : objets sélectionnés de la page courante → bouton ; rend son identifiant.
      Q_INVOKABLE QString addObjects(const QList<quint64>& objectIds);
      Q_INVOKABLE bool insert(const QString& buttonId); // copie au centre de la page active, une étape
      Q_INVOKABLE void removeButton(const QString& buttonId);
  signals:
      void changed();
  private:
      void onTriggered(const QString& id);
      ProfileController* m_profile = nullptr;
      Commands* m_commands = nullptr;
      DocumentController* m_document = nullptr;
      QString m_lastPaletteTrigger;
  };

  } // namespace actcraft::ui
  ```
  Règle de l'historique : une commande entre si elle est un outil ou une fonction (identifiant commençant par
  `tool.`, `present.`, `math.`, `desktop.`, `camera.`, `record.`, `view.page-zoom`), si elle n'est pas dans la
  palette par défaut, et si elle ne vient pas de la palette (`notePaletteTrigger` posé juste avant).
- [ ] **Étape 2 : test rouge** `tst_ui_shortcutbar.cpp` (`DocumentController` réel, `Commands` réel, profil temporaire) :
  `outOfPaletteToolIsRecorded` (déclencher `present.curtain` — ou la première commande de présentation du lot 7 hors
  palette — par `Commands::trigger` : `buttons[0].id` = elle) ; `paletteToolIsNotRecorded` (`notePaletteTrigger("tool.pen")`
  puis `trigger("tool.pen")` : rien) ; `historyIsBoundedAndMovesToFront` (10 outils → 8 ; rejouer le 5ᵉ → en tête,
  toujours 8) ; `droppedObjectBecomesAButton` (un trait ajouté, `addObjects({id})` → bouton `kind: "object"`, profil
  `shortcutObjects` non vide après `saveNow`) ; `clickInsertsACopyAtTheCenter` (`insert(button)` : la page compte un
  objet de plus, nouvel identifiant, centre de sa boîte au centre de la page à 0,5 px près) ;
  `insertIsUndoneAtOnce` (`insert` puis un Annuler du document : nombre d'objets d'avant).
- [ ] **Étape 3 : rouge, implémentation** (sérialisation des objets par l'API du lot 5, A10 ; à défaut, par
  `formats::` du format natif d'une page temporaire — à décider sur la signature réelle relevée par L6-T0, noté au
  rapport), **QML** `ShortcutBar.qml` (colonne de `CommandButton` pour `kind: "tool"`, vignette pour `kind: "object"`,
  `DropArea` qui accepte la sélection glissée de la page — signal de glisser de la sélection du lot 5, sinon commande
  `shortcutbar.insert` réservée à l'ajout par menu contextuel ; clic → `insert`).
- [ ] **Étape 4 : vert**, banc sans écran, preuve de tâche ; commit « Lot 6 : barre de raccourcis, historique et
  objets glissés (L6-T9) ».

<a id="tache-l6-t10"></a>
### Tâche L6-T10 — `core` + `formats` : mots-clés, description de page, style d'étiquette

**Objectif.** Ajouter au modèle les champs de F-570 et F-576 absents (vérifié sur `794cf84` :
`core/include/actcraft/core/model.h:170-182` n'a que `name` et `label`) : `ObjectCommon::keywords`
(`std::vector<std::string>`), `ObjectCommon::labelStyle` (`LabelStyle` : police, taille, couleur, contour, fond),
`Page::keywords`, `Page::description`. Format natif : clés neuves, **omises** à la valeur par défaut, relues sans perte ;
empreinte (`fingerprint`) : les champs n'y entrent qu'en fin de flux **et** s'ils diffèrent du défaut, pour qu'une page
sans eux garde son empreinte (même règle que les clés inconnues de L2-C8). Le patron évident est le mauvais : ajouter
les champs au milieu du flux de l'empreinte changerait toutes les empreintes des tests et de l'oracle.

**Fichiers.** `core/include/actcraft/core/model.h`, `core/src/model.cpp`, `formats/src/nativejson.cpp`,
`tests/tst_core_model.cpp`, `tests/tst_formats_lot6.cpp`.

**Test et commande bornée.** Commande ciblée `-R 'tst_core_model|tst_formats_lot6|tst_formats_nativejson|tst_formats_nativefile'`.

**Interdits.** Aucun nom de propriété de l'original (`NOMS OK`) ; aucune lecture `.flipchart` changée (les mots-clés
des `.flipchart` sont hors de ce lot, à noter au rapport si L6-T0 les demande).

**Terminé quand** les fonctions passent et les tests de format existants sont inchangés et verts.

**Étapes.**
- [ ] **Étape 1 : modèle.** Dans `model.h`, avant `ObjectCommon` :
  ```cpp
  // Style de la légende d'un objet (F-576), écrit par nous. Valeurs par défaut = pas de clé dans le fichier.
  struct LabelStyle
  {
      std::string fontFamily; // vide : police de l'interface
      double pointSize = 12;
      std::uint32_t color = 0xFF000000; // ARGB
      PenStyle outline = PenStyle::None;
      std::uint32_t outlineColor = 0xFF000000;
      std::uint32_t fill = 0x00000000; // transparent
      bool operator==(const LabelStyle&) const = default;
  };
  ```
  Dans `ObjectCommon`, après `label` : `LabelStyle labelStyle;` et `std::vector<std::string> keywords;`. Dans `Page`,
  après `notes` : `std::string description;` et `std::vector<std::string> keywords;`. Si `PenStyle` n'a pas de valeur
  `None` (lire l'énumération), employer sa valeur « aucun trait » réelle et le noter.
- [ ] **Étape 2 : tests rouges.** `tst_core_model.cpp` :
  ```cpp
  void lot6FieldsKeepOldFingerprints()
  {
      core::Page page = samplePage(); // aide existante du fichier, sinon une page avec un trait et une forme
      const auto before = core::fingerprint(page);
      page.description.clear();
      page.keywords.clear();
      QCOMPARE(core::fingerprint(page), before);
      page.keywords = {"soleil"};
      QVERIFY(core::fingerprint(page) != before);
      page.keywords.clear();
      page.objects.front().common().labelStyle.pointSize = 18;
      QVERIFY(core::fingerprint(page) != before);
  }
  ```
  (accès à `ObjectCommon` d'un `PageObject` : lire `model.h` pour le nom réel, `common()` ou membre.)
  `tst_formats_lot6.cpp` (remplace le talon) : `keywordsRoundTrip`, `pageDescriptionRoundTrip`, `labelStyleRoundTrip`
  (document → `nativejson` écriture → lecture → mêmes valeurs et même empreinte), `defaultsAreOmitted` (JSON écrit d'une
  page sans ces champs : aucune des clés `keywords`, `description`, `label-style`), `olderFileReadsWithDefaults` (JSON
  d'un objet sans ces clés : valeurs par défaut, aucun avertissement de lecture). Fonctions d'écriture et de lecture :
  celles qu'emploie déjà `tst_formats_nativejson.cpp` (les relire).
- [ ] **Étape 3 : rouge, implémentation.** `model.cpp` : à la fin du flux de `fingerprint`, après les clés inconnues
  et seulement si non vide ou différent du défaut, un octet de marque (`0x6A`) puis les champs, dans un ordre fixé ;
  `nativejson.cpp` : clés `keywords` (tableau de chaînes), `description` (page), `label-style`
  (`{ "font", "size", "color", "outline", "outline-color", "fill" }`, couleurs en `#AARRGGBB`), chacune omise à sa valeur
  par défaut, et **retirée des clés inconnues** à la lecture (sinon elle serait réécrite deux fois).
- [ ] **Étape 4 : vert** (commande ciblée), preuve de tâche.
- [ ] **Étape 5 : commit** « Lot 6 : mots-clés, description de page, style d'étiquette (L6-T10) ».

<a id="tache-l6-t11"></a>
### Tâche L6-T11 — `core` : commandes de propriétés

**Objectif.** D6-6 : `SetObjectPropertiesCommand` applique une même modification à un ou plusieurs objets (une
étape), garde chaque ancienne valeur pour Annuler, fusionne les modifications successives d'une même propriété sur les
mêmes objets (glisser d'une réglette), et émet des `Change` justes (A12) : nom, mots-clés, légende et son style
n'atteignent pas le cache ; couche, translucidité, contour, remplissage, position, visibilité le redessinent ;
`SetPagePropertiesCommand` pour la page (nom, description, mots-clés : sans cache ; largeur, hauteur : `PageChanged`).
Le patron évident est le mauvais : une commande par propriété et par objet ferait N étapes d'annulation pour une
sélection de N objets, et une étape par pas de réglette.

**Fichiers.** `core/include/actcraft/core/propertyedits.h`, `core/src/propertyedits.cpp`, `tests/tst_core_propertyedits.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_core_propertyedits` ; mutation : `mergeWith` qui rend toujours
faux → `sliderDragMergesIntoOneStep` rouge, remise par `git checkout --`.

**Interdits.** QtCore seulement ; aucune propriété qui n'existe pas au modèle (F-577 à F-580, F-582 : lot 8).

**Terminé quand** les sept fonctions passent et la mutation a été vue rouge.

**Étapes.**
- [ ] **Étape 1 : en-tête** `propertyedits.h` :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>
  #include <actcraft/core/undo.h>

  #include <string>
  #include <variant>
  #include <vector>

  namespace actcraft::core {

  // Propriétés modifiables par le navigateur de propriétés (F-570 à F-576, F-581).
  enum class ObjectProperty : std::uint8_t
  {
      Name,
      Keywords,
      Label,
      LabelStyle,
      Layer,
      Translucency,
      Visible,
      Locked,
      OutlineStyle,
      OutlineColor,
      OutlineWidth,
      FillStyle,
      FillColor,
      Left,
      Top,
      Width,
      Height,
      Angle,
      Mirrored,
  };
  bool propertyTouchesTheCache(ObjectProperty property) noexcept;

  using PropertyValue =
      std::variant<std::string, std::vector<std::string>, double, bool, std::uint32_t, Layer, PenStyle, FillKind,
                   LabelStyle>;

  class SetObjectPropertiesCommand final : public UndoCommand
  {
  public:
      // `page` : index de page ; `objects` : identifiants ; la valeur est la même pour tous.
      SetObjectPropertiesCommand(int page, std::vector<ObjectId> objects, ObjectProperty property, PropertyValue value);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;
      int mergeId() const noexcept override; // constant propre à cette commande
      bool mergeWith(const UndoCommand& other) override; // même page, mêmes objets, même propriété
  private:
      int m_page;
      std::vector<ObjectId> m_objects;
      ObjectProperty m_property;
      PropertyValue m_value;
      std::vector<PropertyValue> m_old; // une par objet, remplie au premier redo
  };

  enum class PageProperty : std::uint8_t
  {
      Name,
      Description,
      Keywords,
      Width,
      Height,
  };

  class SetPagePropertiesCommand final : public UndoCommand
  {
  public:
      SetPagePropertiesCommand(int page, PageProperty property, PropertyValue value);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;
      int mergeId() const noexcept override;
      bool mergeWith(const UndoCommand& other) override;
  private:
      int m_page;
      PageProperty m_property;
      PropertyValue m_value;
      PropertyValue m_old;
  };

  // Valeur courante, pour le modèle du navigateur (L6-T12) ; std::nullopt si la propriété ne s'applique pas.
  std::optional<PropertyValue> readProperty(const PageObject& object, ObjectProperty property);

  } // namespace actcraft::core
  ```
  Position, taille, angle et miroir se lisent et s'écrivent par la transformation (`Transform`) et la boîte de l'objet
  (fonction de boîte du lot 3, `hittest.h`, A12 du lot 3) : la valeur affichée est celle de la boîte transformée.
- [ ] **Étape 2 : test rouge** `tst_core_propertyedits.cpp` (document de 1 page, deux formes et un trait, `UndoStack`
  de `undo.h`) : `oneStepForManyObjects` (outline width 12 sur les deux formes → `stack.count()` + 1, les deux à 12) ;
  `undoRestoresEachOldValue` (largeurs 2 et 5 avant → après Annuler, 2 et 5) ; `sliderDragMergesIntoOneStep`
  (translucidité 10, 20, 30 poussées de suite → une étape, valeur 30, Annuler → valeur d'origine) ;
  `differentPropertyDoesNotMerge` (translucidité puis visibilité → deux étapes) ; `nameOnlyChangeSparesTheCache`
  (les `Change` rendus par `push` pour `Name`, `Keywords`, `Label`, `LabelStyle` : aucun n'est d'un genre qui atteint le
  cache — même prédicat que `presentationTouchesNoCache` de L0-S3) ; `layerChangeMovesTheObject` (couche `Top` → l'objet
  est rangé en tête de la couche du dessus, `Change` de redessin présent) ; `pageWidthChangeIsOneStep` (largeur 1024 →
  1280 → une étape, `PageChanged`, Annuler → 1024).
- [ ] **Étape 3 : rouge, implémentation, vert** ; mutation de `mergeWith` (après un commit vert ; remise par
  `git -C <wt> checkout -- core/src/propertyedits.cpp`) ; preuve de tâche.
- [ ] **Étape 4 : commit** « Lot 6 : commandes de propriétés d'objet et de page, une étape (L6-T11) ».

<a id="tache-l6-t12"></a>
### Tâche L6-T12 — `ui` : modèle du navigateur de propriétés

**Objectif.** `ui::PropertyModel` : liste à deux colonnes groupée en sections repliables (F-840) selon la sélection
(A5) : rien de sélectionné → propriétés de la page (F-583 : Identification, Page) ; un objet → sections de son type
(D9 ; défaut : Identification, Apparence, Contour pour trait, forme, ligne, connecteur ; Remplissage pour forme ;
Position pour tous ; Étiquette pour tous ; Divers pour forme, ligne, image) ; plusieurs objets → seules les propriétés
communes, valeur affichée si elle est la même pour tous, sinon vide. Éditeurs typés (`text`, `number`, `color`,
`slider`, `choice`, `bool`, `keywords`). `setValue(row, value)` exécute **une** commande de L6-T11 par le
`DocumentController` (A4). Remise à jour regroupée et reportée pendant un contact (D6-5).

**Fichiers.** `ui/include/actcraft/ui/propertymodel.h`, `ui/src/propertymodel.cpp`, `tests/tst_ui_propertymodel.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_propertymodel` ; banc sans écran.

**Interdits.** Le modèle ne modifie jamais le document hors d'une commande ; aucune remise à jour pendant un contact ;
aucune demande de redessin du cache (un changement de sélection ne touche pas la page).

**Terminé quand** les sept fonctions passent.

**Étapes.**
- [ ] **Étape 1 : en-tête** `propertymodel.h` :
  ```cpp
  #pragma once

  #include <QAbstractListModel>
  #include <QTimer>
  #include <QtQml/qqmlregistration.h>

  namespace actcraft::ui {

  class DocumentController;
  class ToolHost;

  class PropertyModel : public QAbstractListModel
  {
      Q_OBJECT
      QML_ELEMENT
      Q_PROPERTY(DocumentController* document READ document WRITE setDocument NOTIFY sourcesChanged)
      Q_PROPERTY(ToolHost* tools READ tools WRITE setTools NOTIFY sourcesChanged)
      Q_PROPERTY(QString title READ title NOTIFY refreshed) // « Page », « Trait », « Forme », « 3 objets »…
      Q_PROPERTY(bool inputActive MEMBER m_inputActive NOTIFY inputActiveChanged)
      Q_PROPERTY(int refreshCount READ refreshCount NOTIFY refreshed)
  public:
      enum Role
      {
          SectionRole = Qt::UserRole + 1, // identifiant de section : identification, appearance, outline, fill,
                                          // position, label, misc, page
          SectionLabelRole,
          KeyRole,     // identifiant de la propriété (kebab-case)
          LabelRole,   // libellé français
          EditorRole,  // text, number, color, slider, choice, bool, keywords
          ValueRole,   // QVariant ; invalide si les objets diffèrent
          ChoicesRole, // QStringList pour « choice »
          MinRole,
          MaxRole,
      };
      explicit PropertyModel(QObject* parent = nullptr);
      int rowCount(const QModelIndex& parent = {}) const override;
      QVariant data(const QModelIndex& index, int role) const override;
      QHash<int, QByteArray> roleNames() const override;
      Q_INVOKABLE bool setValue(int row, const QVariant& value);
      Q_INVOKABLE int rowOf(const QString& key) const; // -1 si absente
      DocumentController* document() const noexcept;
      void setDocument(DocumentController* document); // addChangeObserver (A3)
      ToolHost* tools() const noexcept;
      void setTools(ToolHost* tools);                 // selectionChanged (A5)
      QString title() const;
      int refreshCount() const noexcept;
  signals:
      void sourcesChanged();
      void refreshed();
      void inputActiveChanged();
  private:
      void requestRefresh(); // regroupe : un seul refresh() par tour de boucle, rien pendant un contact
      void refresh();
      struct Row;
      std::vector<Row> m_rows;
      DocumentController* m_document = nullptr;
      ToolHost* m_tools = nullptr;
      QTimer m_timer;
      bool m_inputActive = false;
      bool m_pending = false;
      int m_refreshes = 0;
      QString m_title;
  };

  } // namespace actcraft::ui
  ```
- [ ] **Étape 2 : test rouge** `tst_ui_propertymodel.cpp` (`DocumentController` et `ToolHost` réels, document de test
  avec un trait et deux formes) : `pageWhenNothingSelected` (`title` « Page », clés `name`, `description`, `keywords`,
  `width`, `height`) ; `sectionsOfAStroke` (sélection du trait : sections `identification`, `appearance`, `outline`,
  `position`, `label`, pas `fill`) ; `sectionsOfAShape` (`fill` présent) ; `commonValuesOnly` (deux formes de largeurs
  différentes : `ValueRole` de `outline-width` invalide ; même couleur : valeur présente) ; `editGoesThroughTheUndoStack`
  (`setValue(rowOf("outline-width"), 12)` → `canUndo`, une étape, valeur relue 12) ; `refreshIsDeferredDuringContact`
  (`inputActive` vrai, trois modifications du document → `refreshCount` inchangé ; faux → +1 seulement) ;
  `noCacheRefreshOnSelectionChange` (compteur de redessins du cache de la vue, `cacheRefreshCount` posé par L0-S7,
  inchangé après trois changements de sélection).
- [ ] **Étape 3 : rouge, implémentation, vert** ; banc sans écran ; preuve de tâche.
- [ ] **Étape 4 : commit** « Lot 6 : modèle du navigateur de propriétés (L6-T12) ».

<a id="tache-l6-t13"></a>
### Tâche L6-T13 — QML : navigateur de propriétés ; scénario `proprietes`

**Objectif.** `PropertyBrowser.qml` : `ListView` sur une instance `PropertyModel { id: appProperties; objectName:
"appProperties"; document: appDocument; tools: appTools; inputActive: pageView.inputActive }`, sections repliables
(`section.property: "section"`, en-tête cliquable qui replie), titre `appProperties.title` ; `PropertyEditor.qml` :
délégué qui choisit l'éditeur par `editor` (`TextField`, `SpinBox` réel, pastille + `ColorDialog`, `Slider` qui appelle
`setValue` à chaque pas — la fusion de L6-T11 en fait une étape —, `ComboBox`, `Switch`, éditeur de mots-clés séparés
par des espaces). Valeur invalide (objets différents) : champ vide. Scénario S9.4.

**Fichiers.** `ui/qml/PropertyBrowser.qml`, `ui/qml/PropertyEditor.qml`, `bench/scenarios/proprietes.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_captures_proprietes|tst_ui_shell'` ; banc sans écran.

**Preuve visuelle.** `captures.ps1 -Scenario proprietes … -Lire "appProperties.title=Trait" -Lire "appDocument.canUndo=true"` :
deux `LU … OK`, `PIXEL contour-epais OK`, `PIXEL contour-annule OK`, `CAPTURES OK` ; décrire `page.png` (propriétés de
la page), `trait.png` (sections d'un trait), `epais.png`, `annule.png`.

**Interdits.** Aucun appel au document depuis le QML hors de `setValue` ; pas de liaison bidirectionnelle sur
`ValueRole` (la valeur relue vient du modèle après `refreshed`).

**Terminé quand** le scénario est vert, `tst_ui_shell` inchangé vert, banc joint.

**Étapes.**
- [ ] **Étape 1 : scénario rouge** `proprietes.json` : `waitReady` ; `browser.properties` ; capture `page.png` ;
  `expect appProperties.title = Page` ; un trait au stylet injecté (étape de trait de `premiers-pas`) ; sélection du
  trait par la commande ou le clic de l'outil de sélection du lot 3 ; `expect appProperties.title = Trait` ; capture
  `trait.png` ; saisie de 12 dans l'éditeur `outline-width` (`pointer` sur le délégué nommé `prop-outline-width`, puis
  `typeText "12"` et Entrée) ; contrôle `contour-epais` (bande de pixels au milieu du trait : au moins 10 px sombres) ;
  capture `epais.png` ; `edit.undo` ; contrôle `contour-annule` (au plus 5 px sombres) ; capture `annule.png`. Rouge :
  `SCENARIO EN ECHEC` sur `expect appProperties.title` (navigateur talon).
- [ ] **Étape 2 : QML** des deux fichiers ; délégués nommés `prop-<clé>`.
- [ ] **Étape 3 : vert**, preuve visuelle, banc sans écran, preuve de tâche ; `NOMS OK` ; commit « Lot 6 : navigateur
  de propriétés (L6-T13) ».

<a id="tache-l6-t14"></a>
### Tâche L6-T14 — `ui` : modèle du navigateur d'objet

**Objectif.** `ui::ObjectTreeModel` (F-820, F-821) : arbre à deux niveaux — quatre nœuds de couche (du dessus,
intermédiaire, inférieure, fond), puis les objets de la page courante du plus haut au plus bas dans l'ordre de
superposition, avec le nom, le genre (icône de l'outil créateur), masqué, verrouillé. Clic sur une ligne → sélection
(A5) ; glisser une ligne → nouveau rang ou autre couche (pas vers le fond), **une** étape (commande d'arrangement du
lot 5, A9 ; à défaut, `core::ReplaceObjectsCommand`, A12) ; colonnes masqué et verrouillé → commandes de drapeaux du
lot 5 (A9) ou `SetObjectPropertiesCommand` (`Visible`, `Locked`, L6-T11 — **dépendance de repli** : si A9 manque,
cette tâche attend L6-T11 et le note). Remises à jour regroupées (une `beginResetModel` par tour de boucle) et
reportées pendant un contact (D6-5).

**Fichiers.** `ui/include/actcraft/ui/objecttree.h`, `ui/src/objecttree.cpp`, `tests/tst_ui_objecttree.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_objecttree` ; banc sans écran.

**Interdits.** Pas de `QAbstractItemModel` arborescent complet (QML `TreeView` n'est pas requis) : un modèle de liste
à plat avec `depth` (0 couche, 1 objet) et `layer` suffit et reste simple ; aucune remise à jour pendant un contact.

**Terminé quand** les huit fonctions passent.

**Étapes.**
- [ ] **Étape 1 : en-tête** `objecttree.h` :
  ```cpp
  #pragma once

  #include <QAbstractListModel>
  #include <QTimer>
  #include <QtQml/qqmlregistration.h>

  namespace actcraft::ui {

  class DocumentController;
  class ToolHost;

  class ObjectTreeModel : public QAbstractListModel
  {
      Q_OBJECT
      QML_ELEMENT
      Q_PROPERTY(DocumentController* document READ document WRITE setDocument NOTIFY sourcesChanged)
      Q_PROPERTY(ToolHost* tools READ tools WRITE setTools NOTIFY sourcesChanged)
      Q_PROPERTY(bool inputActive MEMBER m_inputActive NOTIFY inputActiveChanged)
      Q_PROPERTY(int resetCount READ resetCount NOTIFY reset)
  public:
      enum Role
      {
          DepthRole = Qt::UserRole + 1, // 0 : couche ; 1 : objet
          LayerRole,                    // « top », « middle », « bottom », « background »
          ObjectIdRole,                 // 0 pour une couche
          NameRole,
          KindRole,                     // « stroke », « shape », « text », « image », « group »…
          HiddenRole,
          LockedRole,
          SelectedRole,
      };
      explicit ObjectTreeModel(QObject* parent = nullptr);
      int rowCount(const QModelIndex& parent = {}) const override;
      QVariant data(const QModelIndex& index, int role) const override;
      QHash<int, QByteArray> roleNames() const override;
      Q_INVOKABLE void select(int row, int modifiers);       // Ctrl : ajoute ou retire
      Q_INVOKABLE bool move(int row, int beforeRow);         // une étape ; faux vers le fond ou une couche seule
      Q_INVOKABLE bool setHidden(int row, bool hidden);
      Q_INVOKABLE bool setLocked(int row, bool locked);
      DocumentController* document() const noexcept;
      void setDocument(DocumentController* document);
      ToolHost* tools() const noexcept;
      void setTools(ToolHost* tools);
      int resetCount() const noexcept;
  signals:
      void sourcesChanged();
      void inputActiveChanged();
      void reset();
  private:
      void requestReset();
      void rebuild();
      struct Row;
      std::vector<Row> m_rows;
      DocumentController* m_document = nullptr;
      ToolHost* m_tools = nullptr;
      QTimer m_timer;
      bool m_inputActive = false;
      bool m_pending = false;
      int m_resets = 0;
  };

  } // namespace actcraft::ui
  ```
- [ ] **Étape 2 : test rouge** `tst_ui_objecttree.cpp` (document : un trait couche du dessus, deux formes
  intermédiaires, une image inférieure) : `fourLayerNodesTopFirst` (lignes de `depth` 0 : top, middle, bottom,
  background, dans cet ordre) ; `objectsTopmostFirst` (sous `middle`, la forme ajoutée en dernier d'abord) ;
  `clickSelects` (`select(row)` → `user().selection` = cet identifiant ; aucun `Change` du document) ;
  `dragReordersInOneStep` (`move` de la seconde forme au-dessus de la première → ordre inversé, une étape, Annuler le
  rend) ; `dragToBackgroundRefused` (`move` vers une ligne sous `background` → faux, document inchangé) ;
  `toggleHiddenAndLocked` (`setHidden` puis `setLocked` → drapeaux relus, deux étapes) ; `resetsAreBatched` (trois
  ajouts d'objets dans le même tour → `resetCount` + 1 après `QCoreApplication::processEvents`) ;
  `noRefreshDuringContact` (`inputActive` vrai, ajout → `resetCount` inchangé ; faux → + 1).
- [ ] **Étape 3 : rouge, implémentation, vert** ; banc sans écran ; preuve de tâche ; commit « Lot 6 : modèle du
  navigateur d'objet (L6-T14) ».

<a id="tache-l6-t15"></a>
### Tâche L6-T15 — QML : navigateur d'objet ; scénario `navigateur-objets`

**Objectif.** `ObjectBrowser.qml` : `ListView` sur `ObjectTreeModel { id: appObjectTree; objectName: "appObjectTree";
document: appDocument; tools: appTools; inputActive: pageView.inputActive }` ; ligne de couche en gras, ligne d'objet
indentée avec icône du genre, nom, deux colonnes à icônes (`nav-objets-masque.svg`, `nav-objets-verrou.svg`) ; clic →
`select`, Ctrl+clic → ajout ; glisser vertical → `move` (fantôme d'insertion) ; ligne sélectionnée surlignée par
`SelectedRole`. Scénario S9.3. `appSelection.count` : lire le nom réel de l'objet QML qui expose la sélection (A5) ; à
défaut, `appTools.selectionCount`.

**Fichiers.** `ui/qml/ObjectBrowser.qml`, `bench/scenarios/navigateur-objets.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_captures_navigateur_objets|tst_ui_shell'` ; banc sans écran.

**Preuve visuelle.** `captures.ps1 -Scenario navigateur-objets … -Lire "objectBrowser.visible=true" -Lire "appObjectTree.rowCount=6"`
(4 couches + 2 objets) : `LU … OK`, `LU appSelection.count=1 OK` après le clic, `CAPTURES OK` ; décrire `arbre.png`,
`selection.png`, `deplace.png`.

**Interdits.** Aucune ligne de navigateur ne déclenche de redessin pendant l'écriture (banc : `cache.refreshes`
inchangé par rapport à la référence du lot pour `court`).

**Terminé quand** scénario vert, banc joint.

**Étapes.**
- [ ] **Étape 1 : scénario rouge** `navigateur-objets.json` : `waitReady` ; deux formes posées par la commande ou
  l'outil des formes (lot 4) ; `browser.objects` ; `expect objectBrowser.visible = true` ; `expect appObjectTree.rowCount = 6` ;
  capture `arbre.png` ; `pointer` sur le délégué `objRow4` ; `expect appSelection.count = 1` ; capture `selection.png` ;
  glisser `objRow5` au-dessus de `objRow4` ; capture `deplace.png` ; `edit.undo`.
- [ ] **Étape 2 : QML** (délégués nommés `objRow<n>`, n à partir de 0).
- [ ] **Étape 3 : vert**, preuve visuelle, banc, preuve de tâche ; commit « Lot 6 : navigateur d'objet (L6-T15) ».

<a id="tache-l6-t16"></a>
### Tâche L6-T16 — Navigateur de notes

**Objectif.** F-830 : texte riche propre à chaque page (`core::Page::notes`, HTML, déjà au modèle et au format
natif), texte d'invite à nous (« Écrivez ici les notes de cette page… »), barre de mise en forme du lot 4 réutilisée
(`TextFormatBar.qml`, lecture seule ici : elle se pose sur l'éditeur de notes par sa propriété de cible si elle en a
une, sinon le gras, l'italique et le souligné par les raccourcis de `TextEdit`) ; l'icône `notesIcon` de la ligne
supérieure est visible quand la page courante a des notes. Modification par `core::SetPageNotesCommand` : la frappe
continue se fusionne en une étape (même page, moins de 2 s entre deux frappes) et n'atteint pas le cache.

**Fichiers.** `core/include/actcraft/core/notesedits.h`, `core/src/notesedits.cpp`, `tests/tst_core_notesedits.cpp`,
`ui/include/actcraft/ui/notescontroller.h`, `ui/src/notescontroller.cpp`, `tests/tst_ui_notes.cpp`,
`ui/qml/NotesBrowser.qml`, `bench/scenarios/notes.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_core_notesedits|tst_ui_notes|tst_captures_notes'` ; banc sans
écran.

**Preuve visuelle.** `captures.ps1 -Scenario notes … -Lire "appNotes.hasNotes=true" -Lire "notesIcon.visible=true"` :
`LU … OK`, `CAPTURES OK` ; décrire `notes-p2.png` et `p1-sans.png`.

**Interdits.** `TextFormatBar.qml` n'est pas modifié (lot 4) ; aucune propriété de l'original.

**Terminé quand** les quatre fonctions et le scénario passent.

**Étapes.**
- [ ] **Étape 1 : en-têtes.** `notesedits.h` :
  ```cpp
  #pragma once

  #include <actcraft/core/undo.h>

  #include <chrono>
  #include <string>

  namespace actcraft::core {

  class SetPageNotesCommand final : public UndoCommand
  {
  public:
      using Clock = std::chrono::steady_clock;
      SetPageNotesCommand(int page, std::string html, Clock::time_point when = Clock::now());
      void redo(Document& document, Changes& changes) override; // Change de page qui n'atteint pas le cache
      void undo(Document& document, Changes& changes) override;
      int mergeId() const noexcept override;
      bool mergeWith(const UndoCommand& other) override; // même page et moins de kMergeWindow depuis la dernière
      static constexpr std::chrono::milliseconds kMergeWindow{2000};
  private:
      int m_page;
      std::string m_html;
      std::string m_old;
      Clock::time_point m_when;
      bool m_filled = false;
  };

  } // namespace actcraft::core
  ```
  `notescontroller.h` : `ui::NotesController : QObject`, `QML_ELEMENT`, propriétés `document`
  (`DocumentController*`), `html` (lecture : notes de la page courante ; écriture : `SetPageNotesCommand`), `hasNotes`
  (vrai si le texte visible, balises retirées, n'est pas vide) ; suit `currentPageChanged` (A4) et les modifications
  versées (A3).
- [ ] **Étape 2 : tests rouges.** `tst_core_notesedits.cpp` : `typingMergesIntoOneStep` (trois commandes à 100 ms
  d'écart → une étape ; une quatrième à 3 s → deux étapes) ; `notesTouchNoCache` (les `Change` rendus n'atteignent pas
  le cache : même prédicat que L0-S3). `tst_ui_notes.cpp` : `followsTheCurrentPage` (notes « A » en page 1, « B » en
  page 2, `html` suit la page courante) ; `hasNotesFlag` (`"<p></p>"` → faux ; `"<p>x</p>"` → vrai).
- [ ] **Étape 3 : implémentation, QML** (`NotesBrowser.qml` : `NotesController { id: appNotes; objectName: "appNotes";
  document: appDocument }`, `TextArea { textFormat: TextEdit.RichText }` écrit `appNotes.html` à chaque changement de
  texte ; `notesIcon.visible: appNotes.hasNotes` posé par une `Binding` dans ce fichier), **scénario** `notes.json`
  (page 2 créée, `browser.notes`, `typeText "Rappeler la consigne"`, `expect appNotes.hasNotes = true`, capture
  `notes-p2.png`, `page.previous`, `expect notesIcon.visible = false`, capture `p1-sans.png`, `page.next`).
- [ ] **Étape 4 : vert**, preuve visuelle, banc, preuve de tâche ; commit « Lot 6 : navigateur de notes (L6-T16) ».

<a id="tache-l6-t17"></a>
### Tâche L6-T17 — Étiquettes

**Objectif.** F-576 : la légende d'un objet (`ObjectCommon::label`, style `labelStyle` de L6-T10) s'affiche dans une
bulle quand le pointeur (souris ou stylet en survol) reste sur l'objet (D12 ; défaut : au survol, 400 ms, près du
pointeur). `ui::LabelTips` trouve l'objet visible le plus haut sous le point (test de toucher du lot 3,
`core::hitTest…`, A12 du lot 3) et rend `text`, `style` ; `LabelTip.qml` dessine la bulle (police, taille, couleur,
contour, fond). Aucun objet masqué ni sans légende ne montre de bulle.

**Fichiers.** `ui/include/actcraft/ui/labeltips.h`, `ui/src/labeltips.cpp`, `ui/qml/LabelTip.qml`,
`tests/tst_ui_labels.cpp`, `bench/scenarios/etiquettes.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_labels|tst_captures_etiquettes'` ; banc sans écran.

**Preuve visuelle.** `captures.ps1 -Scenario etiquettes … -Lire "labelTip.text=Soleil"` : `LU … OK`, `CAPTURES OK` ;
décrire `bulle.png`.

**Interdits.** Le survol ne passe pas par le routeur d'entrée et ne crée aucun trait ; la bulle n'entre pas dans le
cache (élément QML au-dessus de la page).

**Terminé quand** les quatre fonctions et le scénario passent.

**Étapes.**
- [ ] **Étape 1 : en-tête** `labeltips.h` : `ui::LabelTips : QObject`, `QML_ELEMENT`, propriétés `document`
  (`DocumentController*`), `text` (`QString`), `visible` (`bool`), `fontFamily`, `pointSize`, `color`, `outlineColor`,
  `fill` (`QColor`) ; `Q_INVOKABLE void hoverAt(QPointF pagePoint)` ; `Q_INVOKABLE void leave()`.
- [ ] **Étape 2 : test rouge** `tst_ui_labels.cpp` : `hoverShowsTheLabel` (forme légendée « Soleil » ; `hoverAt` au
  centre → `visible`, `text` « Soleil ») ; `noLabelNoTip` (forme sans légende → pas visible) ; `hiddenObjectHasNoTip`
  (objet légendé masqué → pas visible) ; `styleIsApplied` (style 18 pt rouge → `pointSize` 18, `color` rouge).
- [ ] **Étape 3 : implémentation, QML** (`LabelTip.qml` : `HoverHandler` sur la zone de page, minuterie de 400 ms, point
  converti en coordonnées de page par la vue — fonction de conversion de `PageView`, lire son nom), **scénario**
  `etiquettes.json` (une forme posée, légende « Soleil » par `browser.properties` et l'éditeur `label`, `pointer` en
  survol au centre de la forme, attente 600 ms, `expect labelTip.text = Soleil`, capture `bulle.png`).
- [ ] **Étape 4 : vert**, preuve visuelle, banc, preuve de tâche ; `NOMS OK` ; commit « Lot 6 : étiquettes des objets
  (L6-T17) ».

<a id="tache-l6-t18"></a>
### Tâche L6-T18 — Point d'étape 6a : S9 de bout en bout

**Objectif.** Le scénario `reglages-gardes` joue S9 du cadrage : ancrer la palette à gauche, ajouter une couleur
personnalisée, ouvrir le navigateur d'objet, **quitter et relancer**, tout est restauré ; une largeur de contour
changée dans le navigateur de propriétés s'annule. Le joueur de scénarios gagne une étape `restart` : il ferme la
fenêtre, attend l'enregistrement du profil (`appProfile.saveCount` stable), recrée le moteur QML sur le **même** dossier
de profils et de réglages, et attend `waitReady`. Puis contrôle des talons de 6a et graphe.

**Fichiers.** `bench/scenarios/reglages-gardes.json`, `bench/src/uiscenario.cpp`,
`bench/include/actcraft/bench/uiscenario.h`, `tests/tst_bench_uiscenario.cpp`, `graphify-out/`.

**Test et commande bornée.** Commande ciblée `-R 'tst_bench_uiscenario|tst_captures_reglages_gardes'` ; puis preuve de
tâche en debug **et** en release (`-Preset release`) ; captures hors écran `-Scenario tous`.

**Preuve visuelle.** `captures.ps1 -Scenario reglages-gardes … -Lire "appProfile.paletteDock=left" -Lire "appColors.extraCount=1" -Lire "browserPanel.current=objects"` :
les trois `LU … OK` **après** la ligne `RELANCE`, `PIXEL contour-annule OK`, `CAPTURES OK` ; décrire `avant.png` et
`apres-relance.png` (même disposition).

**Interdits.** L'étape `restart` n'écrit pas dans le profil réel de l'utilisateur : le joueur emploie déjà un dossier
d'essai (`QStandardPaths::setTestModeEnabled` ou dossier de sortie) — le vérifier et le garder.

**Terminé quand** N = N6 + 43 en debug et release, `TESTS OK` deux fois, S9 vert, `git -C <wt> grep -n "Talon L6 : rempli par L6-T([2-9]|1[0-7])\." -- . ':(exclude)docs/'`
vide, `NOMS OK`, `graphify update .` commité.

**Étapes.**
- [ ] **Étape 1 : test rouge** dans `tst_bench_uiscenario.cpp` :
  ```cpp
  void restartStepParsed()
  {
      const QByteArray json = R"({"name":"r","steps":[{"waitReady":true},{"restart":true},{"waitReady":true}]})";
      QString error;
      const auto scenario = bench::parseUiScenario(json, error); // nom réel de la fonction : lire uiscenario.h
      QVERIFY2(scenario, qPrintable(error));
      QCOMPARE(scenario->steps.size(), std::size_t(3));
      QCOMPARE(scenario->steps[1].kind, bench::UiStepKind::Restart);
  }
  ```
  `Restart` s'ajoute **à la fin** de l'énumération des genres d'étapes (le test `stepKindRanksFrozen` de L0-S2 fige les
  rangs existants).
- [ ] **Étape 2 : joueur.** Analyse de `{"restart": true}` ; exécution : ligne `RELANCE` dans la sortie, fermeture
  propre de la fenêtre (la même voie que la fin d'un scénario), attente de 2 s au plus que `appProfile.saveCount` ne
  bouge plus, recréation de l'application dans le même processus sur les mêmes dossiers (la même fonction que le
  démarrage du joueur).
- [ ] **Étape 3 : scénario** `reglages-gardes.json` : `waitReady` ; `palette.dock-left` ; `color.add #12AB34` ;
  `browser.objects` ; un trait ; capture `avant.png` ; `restart` ; `waitReady` ; `expect appProfile.paletteDock = left` ;
  `expect appColors.extraCount = 1` ; `expect browserPanel.current = objects` ; capture `apres-relance.png` ; puis la
  suite de `proprietes` (sélection, largeur 12, `edit.undo`, contrôle `contour-annule`).
- [ ] **Étape 4 : vert**, preuves (debug et release), captures `-Scenario tous` hors écran, banc sans écran.
- [ ] **Étape 5 : contrôle des talons** (commande du critère) ; `graphify update .` ; `git add` des fichiers de la ligne
  et de `graphify-out/` ; `NOMS OK` ; commit « Lot 6 : point d'étape 6a, réglages gardés de bout en bout (L6-T18) ».

---

Suite du lot : `docs/plans/2026-10-18-livraison-complete-lot6b-PLAN.md` (L6-T19 à L6-T37).

