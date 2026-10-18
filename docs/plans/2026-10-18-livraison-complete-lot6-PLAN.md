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
| L6-T1 | Réservation : 43 tests enregistrés, fichiers neufs des deux sous-lots (talons marqués), commandes et disposition du lot, icônes | — | `actions/data/commands.json`, `actions/data/layout.json`, `actions/CMakeLists.txt`, `core/CMakeLists.txt`, `formats/CMakeLists.txt`, `ui/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/{actions,core,formats,ui,bench}.cmake`, `tests/tst_actions_layout.cpp`, `ui/icons/{palette-gauche,palette-droite,palette-haut,palette-bas,palette-flottante,palette-reduire,palette-reduire-tout,palette-epingler,couleurs-plus,pipette,nav-objets-masque,nav-objets-verrou,notes-page,profil,profils,mode-creation,accueil,recents,ressources-miennes,ressources-partagees,ressources-autre,tampon,classe-verrou,orthographe,aide-sommaire,programme}.svg`, talons : `actions/include/actcraft/actions/{profile,profilestore}.h`, `actions/src/{profile,profilestore}.cpp`, `actions/data/profiles/{en-classe,preparation,lecture-ecriture,calcul-geometrie,son-image}.json`, `core/include/actcraft/core/{propertyedits,notesedits,spellcheck}.h`, `core/src/{propertyedits,notesedits,spellcheck,hunspellengine}.cpp`, `formats/include/actcraft/formats/fragment.h`, `formats/src/fragment.cpp`, `ui/include/actcraft/ui/{preferences,profilecontroller,colorpalette,shortcutbar,propertymodel,objecttree,notescontroller,labeltips,resourcelibrary,designmode,recentdocuments,layouteditor,settingsschema,secretstore,networksettings,launchers,classroomlock,spelling,helpviewer,language}.h`, `ui/src/{preferences,profilecontroller,colorpalette,shortcutbar,propertymodel,objecttree,notescontroller,labeltips,resourcelibrary,designmode,recentdocuments,layouteditor,settingsschema,secretstore_win,secretstore_other,networksettings,launchers,classroomlock,spelling,helpviewer,language}.cpp`, `ui/help/index.html`, `ui/i18n/actcraft_fr.ts`, `tests/launcher_probe.cpp`, 28 tests talons `tests/tst_{actions_profile,actions_lockable,ui_preferences,ui_profiles,ui_palette,ui_colors,ui_shortcutbar,formats_lot6,core_propertyedits,ui_propertymodel,ui_objecttree,core_notesedits,ui_notes,ui_labels,formats_fragment,ui_resources,ui_designmode,ui_recents,ui_profileeditor,ui_layouteditor,ui_settingsschema,ui_network,ui_launchers,ui_classroomlock,core_spellcheck,ui_spelling,ui_help,ui_language}.cpp`, 15 scénarios talons `bench/scenarios/{palette,couleurs,navigateur-objets,proprietes,notes,etiquettes,reglages-gardes,ressources,mode-creation,tableau-de-bord,profils,raccourcis-programmes,classe-protegee,orthographe,aide-langue}.json` | isolé | implementeur-tdd | `100% tests passed … out of N`, N = N6 + 43, `TESTS OK`, `tst_actions_layout` (dont `lot6EntriesDeclared`, `noShortcutTwice`) Passed, 43 tests neufs `SKIP` nommant leur tâche ; `premiers-pas` hors écran `CAPTURES OK` ; `NOMS OK` | S9, S19, S20, S21 | #tache-l6-t1 |
| L6-T2 | Coquille QML du lot : emplacements de la palette, de la barre de raccourcis, des quatre navigateurs, des fenêtres du lot ; gestionnaires des commandes | L6-T1 | `ui/qml/Main.qml`, `ui/qml/ToolPalette.qml`, `ui/qml/BrowserPanel.qml`, `ui/qml/TopBar.qml`, `ui/qml/{PaletteFrame,ShortcutBar,ExtendedColors,ColorLoupe,ObjectBrowser,PropertyBrowser,PropertyEditor,NotesBrowser,ResourceBrowser,LabelTip,Dashboard,ProfileChooser,ProfileEditor,FormatPage,CommandsPage,LaunchersPage,SettingsPage,NetworkPage,ClassroomLockDialog,SpellingDialog,HelpViewer,SaveSummaryDialog}.qml`, `tests/tst_ui_shell.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N6 + 43, `tst_ui_shell` (dont `lot6ShellPlaceholders`, `everyLot6CommandHasAHandler`, `browserTabsInOrder`) Passed ; `premiers-pas` hors écran `CAPTURES OK` ; banc hors écran `content_check.ok = True` | S9 | #tache-l6-t2 |
| L6-T3 | `actions` : attributs `lockable` et `restrictable`, porte du verrou dans le registre ; remplis pour toutes les commandes présentes (lots 1 à 5, 7, socle) | L6-T1 | `actions/include/actcraft/actions/commandregistry.h`, `actions/src/commandregistry.cpp`, `actions/data/commands.json`, `tests/tst_actions_lockable.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_actions_lockable` Passed (dont `attributesRead`, `lockGreysOnlyLockable`, `lockedTriggerCallsNothing`, `enableCannotBypassTheLock`, `everyCommandDeclaresBoth`, `fileAndQuitStayFree`), `tst_actions_registry` Passed ; mutation de la porte (`isEnabled` sans le verrou) rouge puis remise | S19 | #tache-l6-t3 |
| L6-T4 | `actions` : modèle de profil `actcraft-profile/1` et magasin (fournis, utilisateur, enregistrer sous, supprimer, réinitialiser, exporter, importer) | L6-T1 | `actions/include/actcraft/actions/profile.h`, `actions/src/profile.cpp`, `actions/include/actcraft/actions/profilestore.h`, `actions/src/profilestore.cpp`, `tests/tst_actions_profile.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_actions_profile` Passed (dont `roundTripKeepsEveryField`, `unknownKeysKept`, `majorVersionRefused`, `iconPixelsClamped`, `builtinFirstThenUser`, `userCopyShadowsBuiltin`, `resetRemovesTheUserCopy`, `builtinCannotBeRemoved`, `saveIsAtomic`, `exportImportRoundTrip`, `importNeverOverwrites`) | S9, S9.7 | #tache-l6-t4 |
| L6-T5 | `ui` : réglages du poste (`Preferences`, QSettings) et `ProfileController` (profil courant, enregistrement implicite différé hors contact, dossier de disposition) | L6-T3, L6-T4 | `ui/include/actcraft/ui/preferences.h`, `ui/src/preferences.cpp`, `ui/include/actcraft/ui/profilecontroller.h`, `ui/src/profilecontroller.cpp`, `app/main.cpp`, `tests/tst_ui_preferences.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_ui_preferences` Passed (dont `keysAreOurs`, `currentProfileRestored`, `implicitSaveIsDebounced`, `noSaveDuringContact`, `activateSwitchesTheLayoutDir`, `brokenProfileFallsBack`, `settingDefaultsComeFromTheSchema`) | S9, S9.7 | #tache-l6-t5 |
| L6-T6 | Cinq profils fournis, à nos noms, avec les épinglages du lot 7 et du lot 8 annoncé | L6-T4, L6-T5 | `actions/data/profiles/{en-classe,preparation,lecture-ecriture,calcul-geometrie,son-image}.json`, `tests/tst_ui_profiles.cpp`, `bench/scenarios/profils.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_profiles` (dont `fiveBuiltinsInOrder`, `pinnedCommandsExist`, `pinnedFilteredByPlatform`, `namesAreOurs`, `preparationOpensThePageBrowser`), `tst_captures_profils` Passed ; `captures.ps1 -Scenario profils` : `LU appProfile.currentId=calcul-geometrie OK`, `CAPTURES OK` | S9.7, S32 | #tache-l6-t6 |
| L6-T7 | Palette : ancrage (4 bords, flottante), réduite, réduite au maximum, épinglée (repli au retrait du pointeur), taille des icônes 16 à 32 ; persistance | L6-T2, L6-T5 | `ui/qml/PaletteFrame.qml`, `ui/qml/ToolPalette.qml`, `ui/qml/Theme.qml`, `tests/tst_ui_palette.cpp`, `bench/scenarios/palette.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_palette` (dont `dockFollowsTheProfile`, `dragToCenterFloats`, `collapsedShowsFirstRowOnly`, `fullCollapseHidesShortcuts`, `unpinnedHidesAfterLeave`, `iconPixelsScaleEveryIcon`), `tst_captures_palette` Passed ; `captures.ps1 -Scenario palette` : `LU appProfile.paletteDock=left OK`, `LU toolPalette.x=0 OK`, `CAPTURES OK` ; banc hors écran joint | S9, S9.1 | #tache-l6-t7 |
| L6-T8 | Palette de couleurs étendue (6 × 6), copieur de couleur, boîte de couleur, couleurs ajoutées gardées ; 8, 16 ou 24 pastilles | L6-T2, L6-T5 | `ui/include/actcraft/ui/colorpalette.h`, `ui/src/colorpalette.cpp`, `ui/qml/ExtendedColors.qml`, `ui/qml/ColorLoupe.qml`, `ui/qml/ColorSwatches.qml`, `tests/tst_ui_colors.cpp`, `bench/scenarios/couleurs.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_colors` (dont `extendedGridIsSixBySix`, `addedColorIsKept`, `duplicatesIgnored`, `swatchCountFollowsTheProfile`, `loupeSamplesOnceOnOpen`), `tst_captures_couleurs` Passed ; `captures.ps1 -Scenario couleurs` : `LU appColors.extraCount=1 OK`, `PIXEL pastille-ajoutee OK`, `CAPTURES OK` | S9, S9.2 | #tache-l6-t8 |
| L6-T9 | Barre de raccourcis : historique des outils hors palette (F-140), objets glissés devenus boutons (F-141) | L6-T7 | `ui/include/actcraft/ui/shortcutbar.h`, `ui/src/shortcutbar.cpp`, `ui/qml/ShortcutBar.qml`, `tests/tst_ui_shortcutbar.cpp` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_shortcutbar` Passed (dont `outOfPaletteToolIsRecorded`, `paletteToolIsNotRecorded`, `historyIsBoundedAndMovesToFront`, `droppedObjectBecomesAButton`, `clickInsertsACopyAtTheCenter`, `insertIsOneUndoStep`) ; banc hors écran joint | S9, S9.1 | #tache-l6-t9 |
| L6-T10 | `core` + `formats` : mots-clés (objet, page), description de page, style d'étiquette ; format natif sans perte, empreintes inchangées sans ces champs | L6-T0, L6-T1 | `core/include/actcraft/core/model.h`, `core/src/model.cpp`, `formats/src/nativejson.cpp`, `tests/tst_core_model.cpp`, `tests/tst_formats_lot6.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_formats_lot6` (dont `keywordsRoundTrip`, `pageDescriptionRoundTrip`, `labelStyleRoundTrip`, `defaultsAreOmitted`, `olderFileReadsWithDefaults`), `tst_core_model` (dont `lot6FieldsKeepOldFingerprints`), `tst_formats_nativejson` Passed | S9.4, S9.10 | #tache-l6-t10 |
| L6-T11 | `core` : `SetObjectPropertiesCommand` (une étape, plusieurs objets, fusion des glissés) et `SetPagePropertiesCommand` | L6-T10 | `core/include/actcraft/core/propertyedits.h`, `core/src/propertyedits.cpp`, `tests/tst_core_propertyedits.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_core_propertyedits` Passed (dont `oneStepForManyObjects`, `undoRestoresEachOldValue`, `sliderDragMergesIntoOneStep`, `differentPropertyDoesNotMerge`, `nameOnlyChangeSparesTheCache`, `layerChangeMovesTheObject`, `pageWidthChangeIsOneStep`) ; mutation de `mergeWith` rouge puis remise | S9, S9.4 | #tache-l6-t11 |
| L6-T12 | `ui` : `PropertyModel` (page, un objet, plusieurs objets ; sections F-570 à F-576, F-581, F-583 ; valeurs communes ; éditeurs typés) | L6-T11 | `ui/include/actcraft/ui/propertymodel.h`, `ui/src/propertymodel.cpp`, `tests/tst_ui_propertymodel.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_ui_propertymodel` Passed (dont `pageWhenNothingSelected`, `sectionsOfAStroke`, `sectionsOfAShape`, `commonValuesOnly`, `editGoesThroughTheUndoStack`, `refreshIsDeferredDuringContact`, `noCacheRefreshOnSelectionChange`) ; banc hors écran joint | S9, S9.4 | #tache-l6-t12 |
| L6-T13 | QML : navigateur de propriétés (sections repliables, éditeurs) ; scénario `proprietes` | L6-T2, L6-T12 | `ui/qml/PropertyBrowser.qml`, `ui/qml/PropertyEditor.qml`, `bench/scenarios/proprietes.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_captures_proprietes` Passed ; `captures.ps1 -Scenario proprietes` : `LU appProperties.title=Trait OK`, `LU appDocument.canUndo=true OK`, `PIXEL contour-epais OK`, `PIXEL contour-annule OK`, `CAPTURES OK` ; banc hors écran joint | S9, S9.4 | #tache-l6-t13 |
| L6-T14 | `ui` : `ObjectTreeModel` (couches, ordre de superposition, clic qui sélectionne, glisser qui réordonne, masqué, verrouillé), mises à jour regroupées | L6-T0, L6-T1 | `ui/include/actcraft/ui/objecttree.h`, `ui/src/objecttree.cpp`, `tests/tst_ui_objecttree.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_ui_objecttree` Passed (dont `fourLayerNodesTopFirst`, `objectsTopmostFirst`, `clickSelects`, `dragReordersInOneStep`, `dragToBackgroundRefused`, `toggleHiddenAndLocked`, `resetsAreBatched`, `noRefreshDuringContact`) ; banc hors écran joint | S9, S9.3 | #tache-l6-t14 |
| L6-T15 | QML : navigateur d'objet ; scénario `navigateur-objets` | L6-T2, L6-T14 | `ui/qml/ObjectBrowser.qml`, `bench/scenarios/navigateur-objets.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_captures_navigateur_objets` Passed ; `captures.ps1 -Scenario navigateur-objets` : `LU objectBrowser.visible=true OK`, `LU appObjectTree.rowCount=4 OK`, `LU appSelection.count=1 OK`, `CAPTURES OK` ; banc hors écran joint | S9, S9.3 | #tache-l6-t15 |
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

<!-- suite -->
