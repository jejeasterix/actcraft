# Livraison complète — lot 8 « Interactivité et multimédia », sous-lot 8a — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**But.** Version **0.9.0** (amendement 1, § 6), livrée à la fin du sous-lot 8b
(`docs/plans/2026-10-18-livraison-complete-lot8b-PLAN.md`) : une seule PR, une seule version pour le lot. Le sous-lot
8a livre la tâche d'ouverture, la réservation, la coquille du lot, le modèle de l'interactivité (pour les deux
sous-lots), le catalogue et le moteur des actions, l'exécuteur d'actions au clic, l'ouverture des liens après
confirmation, le navigateur d'actions et son onglet glisser-déposer, les conteneurs, les limiteurs et la rotation,
leurs sections de propriétés, l'encre révélatrice, et le scénario **S11** de bout en bout. Le sous-lot 8b livre
l'installation des modules Qt du lot (porte de téléchargement), les dialogues de liens, les médias intégrés (S25),
les enregistreurs (S26), le cadre web (S27), la diffusion protégée (S18), les épinglages du profil « Son et image » et
le contrôle du lot.

**Exigences du lot** (cadrage, décision 11 ; amendement 1, § 2.1 à § 2.3) : F-255, F-550, F-551, F-552 (lecteur
intégré, amendement 1, § 2.2 ; l'application du système, N2-9, devient le repli), F-577, F-578, F-579, F-580, F-582,
F-850, F-851, F-890, F-891 et F-892 **sans les actions de vote**, F-680 (S18), F-292 et F-293 (S26), F-302 (S27).
Exigences de 8a : F-255, F-550 et F-551 (ouverture ; dialogues en 8b), F-577 à F-580, F-582 (données et section),
F-850, F-851, F-890 à F-892.

**Constats du corpus rattachés au lot 8** : 14 selon l'amendement 1, § 4 (3 « sélection, verrouillage, masquage »,
6 « actions et navigateurs », 1 « outils », 4 « multimédia ») ; le relevé de préparation de ce plan en compte **17**
en séparant les sous-puces (V28, V103, V40, V42 ; V42, V17, V58 ×4, V113 ; V43 ; V57/V66, V63 ×2, V55, V126), plus le
piège des conteneurs recouverts (V39) et deux cas limites (V64, V98 : hors lot). **L8-T0 recompte** et écrit une ligne
par constat. Écart du lot : **E6** (valeur « bloqué » du limiteur de dimension, V136). Principes du lot 10 versés
ici : **P8** (encre révélatrice : redessin immédiat, prédicat `canWait` déjà en place).

**Place dans le chantier.** Le lot 8 s'exécute après le lot 6 (ordre de l'état : L0-S, lots 3, 7 en session B, 4, 5,
6, puis 8). Les plans de ces lots font autorité sur les API ; chaque API qu'ils posent et que ce plan emploie est
dans la table « API supposées » (A1 à A30) et **à revérifier par L8-T0** sur le code livré. Écrit sur `5373f04`
(plans des lots 5 et 6 mergés ; code livré : lots 1, 2, 10 et une partie du socle). Noms vérifiés dans le code à
`5373f04` : `core::Action`, `ActionKind`, `Restrictions`, `ObjectCommon::actions`, `StrokeKind::Revealer`,
`core::Media`, `core::Equation`, `UnknownObject`, `ObjectKind` (`core/include/actcraft/core/model.h:143-198,257-359`) ;
`canWait` (`render/src/fixedpageitem.cpp:55-64`, refuse images, encre révélatrice et grille du dessus) ; clés de
scénario `command`, `argument`, `expect`, `value`, `stroke`, `pressure`, `checkPixel`, `at`, `color`, `tolerance`,
`key`, `wait`, `waitReady`, `waitFor`, `capture`, `remember`, `fixture` (`bench/src/uiscenario.cpp`). Les fichiers
propres au lot portent des noms neufs (`interactivity*`, `objectactions*`, `actionrunner*`, `linklauncher*`,
`actionbrowser*`, `actionedits*`, `dragdropactions*`, `containers*`, `restrictors*`, `revealinktool*`,
`pagehistory*`) ; les fichiers communs à tous les lots (`commands.json`, `layout.json`, `Main.qml`, `CMakeLists.txt`,
`tests/*.cmake`) sont repris en série, jamais dans une même vague.

**Pile.** C++20, Qt 6.12 (Core, Gui, Quick, QuickControls2, QuickDialogs2, Svg, Test ; en 8b : Multimedia, WebView),
CMake + Ninja, MSVC 2022, PowerShell 7. Aucune bibliothèque tierce en 8a.

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L8-T0 | Ouverture : vidéos, REA et ancien dépôt confrontés au lot ; API A1 à A30 revérifiées ; N8 relevé ; poids des moteurs web mesuré | — | `docs/verifications/2026-10-21-lot8-principes.md` (hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot8.md`) | isolé | concepteur-critique | rapport commité ; relevé brut hors dépôt présent ; décisions D1 à D16 et table A1 à A30 écrites ; une ligne par constat ; `git grep` des titres du corpus vide ; `NOMS OK` ; N8 relevé | S30 | #tache-l8-t0 |
| L8-T1 | Réservation 8a : 21 tests enregistrés, fichiers neufs de 8a (talons marqués), commandes et disposition des **deux** sous-lots, icônes | — | `actions/data/commands.json`, `actions/data/layout.json`, `actions/CMakeLists.txt`, `core/CMakeLists.txt`, `formats/CMakeLists.txt`, `tools/CMakeLists.txt`, `render/CMakeLists.txt`, `ui/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/{actions,core,formats,flipchart,tools,render,ui,bench}.cmake`, `tests/tst_actions_layout.cpp`, `tests/tst_ui_theme.cpp`, `ui/icons/{actions,encre-revelatrice,lien-fichier,lien-site,action-objet,action-glisser,conteneur,limiteur,rotation,historique-precedent,historique-suivant,son,video,lecteur-sons,enregistrer-son,enregistrer-ecran,enregistrer-zone,cadre-web,diffuser}.svg`, talons : `actions/data/object-actions.json`, `actions/include/actcraft/actions/objectactions.h`, `actions/src/objectactions.cpp`, `core/include/actcraft/core/{interactivity,objectactions,actionedits,containers,restrictors}.h`, `core/src/{interactivity,objectactions_geometry,objectactions_edit,actionedits,containers,restrictors}.cpp`, `tools/include/actcraft/tools/{revealinktool,interactionpolicy}.h`, `tools/src/revealinktool.cpp`, `ui/include/actcraft/ui/{actionrunner,linklauncher,actionbrowser,dragdropactions,pagehistory,interactivityprops}.h`, `ui/src/{actionrunner,linklauncher,actionbrowser,dragdropactions,pagehistory,interactivityprops}.cpp`, 21 tests talons `tests/tst_{core_interactivity,formats_interactivity,formats_flipchart_interactivity,actions_objectactions,core_objectactions,core_objectactions_edit,ui_actionrunner,ui_linklauncher,ui_actionbrowser,ui_dragdropactions,core_containers,core_restrictors,tools_interactivity,ui_interactivityprops,render_reveal,formats_samples_interactivity}.cpp` et 5 scénarios talons `bench/scenarios/{navigateur-actions,glisser-deposer,conteneurs,encre-revelatrice,interactivite}.json` | isolé | implementeur-tdd | `100% tests passed … out of N`, N = N8 + 21, `TESTS OK` ; `tst_actions_layout` (dont `lot8EntriesDeclared`, `lot8AttributesDeclared`) et `tst_actions_registry` (dont `noShortcutTwice`) Passed ; 21 tests neufs `SKIP` nommant leur tâche ; `premiers-pas` hors écran `CAPTURES OK` ; `NOMS OK` | S11, S18, S25, S26, S27 | #tache-l8-t1 |
| L8-T2 | Coquille QML du lot (8a et 8b) : onglet `actions`, surcouches de la page, emplacements des fenêtres du lot ; un gestionnaire par commande du lot | L8-T1 | `ui/qml/Main.qml`, `ui/qml/BrowserPanel.qml`, `ui/qml/PageArea.qml`, `ui/qml/{ActionBrowser,DragDropActions,ActionOverlays,LinkConfirmDialog,LinkDialog,MediaLayer,SoundController,RecorderPalette,RecordingIndicator,WebFrameLayer,WebFrameDialog,PublishDialog}.qml`, `tests/tst_ui_shell.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N8 + 21 ; `tst_ui_shell` (dont `lot8ShellPlaceholders`, `everyLot8CommandHasAHandler`, `actionsTabEnabled`) Passed ; `premiers-pas` hors écran `CAPTURES OK` ; banc hors écran `content_check.ok = True` | S11 | #tache-l8-t2 |
| L8-T3 | `core` : modèle de l'interactivité pour les deux sous-lots (action à nous, conteneur, limiteurs, rotation, options de média, cadre web, restrictions de diffusion) ; empreintes inchangées sans ces champs | L8-T0, L8-T1 | `core/include/actcraft/core/model.h`, `core/src/model.cpp`, `core/include/actcraft/core/interactivity.h`, `core/src/interactivity.cpp`, `tests/tst_core_interactivity.cpp`, `tests/tst_core_model.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_core_interactivity` (dont `defaultsChangeNoFingerprint`, `fixedTravelMeansNotMovable`, `fixedTurningMeansNotRotatable`, `webFrameKindBeforeUnknown`, `actionKeepsFlipchartFields`, `broadcastDefaultsAllowEverything`) et `tst_core_model` (dont `lot8FieldsKeepOldFingerprints`) Passed ; `NOMS OK` (aucun nom composé de l'original) | S11, S18, S25, S27 | #tache-l8-t3 |
| L8-T4 | `formats` : format natif des champs de L8-T3, sans perte, valeurs par défaut omises, clés inconnues gardées | L8-T3 | `formats/src/nativejson.cpp`, `tests/tst_formats_interactivity.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_formats_interactivity` (dont `actionRoundTrip`, `containerRoundTrip`, `restrictorsRoundTrip`, `mediaOptionsRoundTrip`, `webFrameRoundTrip`, `broadcastRoundTrip`, `defaultsAreOmitted`, `olderFileReadsWithDefaults`, `unknownActionKeyKept`), `tst_formats_nativejson` Passed | S11, S18, S27 | #tache-l8-t4 |
| L8-T5 | `formats` : lecteur `.flipchart` — actions vers notre catalogue (table écrite à la main), limiteurs en modes, rotation, conteneurs, options de média, encre révélatrice ; écrivain synthétique étendu | L8-T3 | `formats/src/flipchart.cpp`, `formats/src/flipchartids.h`, `tests/support/flipchartwriter.h`, `tests/tst_formats_flipchart_interactivity.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_formats_flipchart_interactivity` (dont `toggleHiddenMapsToOurId`, `unknownCommandKeptWithoutId`, `travelModesKept`, `containerRead`, `revealInkRead`, `mediaOptionsRead`, `noNameOfTheOriginal`), `tst_formats_flipchart`, `tst_formats_oracle_synth` Passed | S11 | #tache-l8-t5 |
| L8-T6 | `actions` : catalogue des actions d'objet (données à nous, quatre catégories, sans vote) et son chargeur | L8-T1 | `actions/data/object-actions.json`, `actions/include/actcraft/actions/objectactions.h`, `actions/src/objectactions.cpp`, `tests/tst_actions_objectactions.cpp` | isolé | implementeur-tdd | `TESTS OK` ; `tst_actions_objectactions` (dont `fourCategoriesNoVote`, `everyCommandActionNamesARegisteredCommand`, `parametersAreTyped`, `dragDropListHasEighteen`, `labelsAreOurs`, `idsAreUnique`) Passed | S11 | #tache-l8-t6 |
| L8-T7 | `core` : moteur des actions d'objet I — géométrie (angle, angle incrémentiel, placer, dimensionner et étirer depuis neuf ancrages, refléter, inverser, taille d'origine, ajuster) | L8-T3 | `core/include/actcraft/core/objectactions.h`, `core/src/objectactions_geometry.cpp`, `tests/tst_core_objectactions.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_core_objectactions` (dont `angleSetsAbsoluteRotation`, `incrementNegativeIsCounterClockwise`, `placeCenterUsesThePage`, `sizeKeepsProportions`, `stretchDoesNot`, `nineAnchors`, `mirrorIsOneStep`, `fixedRestrictorsAreRespected`, `unknownIdReturnsNull`) Passed ; mutation du signe de l'incrément rouge puis remise | S11 | #tache-l8-t7 |
| L8-T8 | `core` : moteur des actions d'objet II — superposition, couches, masquer, verrouiller, translucidité, textes (ajouter, modifier, valeur arithmétique, nouveau, extraire), effacements, copier, couper, coller, dupliquer, supprimer | L8-T7 | `core/src/objectactions_edit.cpp`, `tests/tst_core_objectactions_edit.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_core_objectactions_edit` (dont `toggleHiddenTwiceRestores`, `toggleHiddenDelegatesToObjectFlags`, `textValueArithmetic`, `textValueNonNumericUnchanged`, `addTextIsPlainEscaped`, `extractTextOneObjectPerWord`, `translucencyClamped`, `clearAnnotationsOnlyTopStrokes`, `everyEditIsOneStep`) Passed | S11 | #tache-l8-t8 |
| L8-T9 | `ui` : exécuteur d'actions — clic, clic droit, survol, mode conception, verrou de classe, garde de contact, historique des pages, actions de page et de commande | L8-T2, L8-T6, L8-T8 | `ui/include/actcraft/ui/actionrunner.h`, `ui/src/actionrunner.cpp`, `ui/include/actcraft/ui/pagehistory.h`, `ui/src/pagehistory.cpp`, `ui/src/toolhost.cpp`, `ui/qml/ActionOverlays.qml`, `tests/tst_ui_actionrunner.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_actionrunner` (dont `clickRunsEveryActionInOrder`, `dragDoesNotRun`, `rightClickSelectsWithoutRunning`, `designModeSelectsInstead`, `lockedCommandActionRefused`, `pageActionCreatesNoStep`, `objectActionIsOneStep`, `neverDuringContact`, `hiddenObjectStillClickable`, `lockedObjectStillRuns`, `historyBackAndForward`, `unsupportedActionDoesNothing`, `refusedWithoutCommands`) Passed ; banc hors écran joint | S11 | #tache-l8-t9 |
| L8-T10 | `ui` : ouverture des liens et fichiers après confirmation (`QDesktopServices`), règles d'adresse, fichier rangé dans le document | L8-T1 | `ui/include/actcraft/ui/linklauncher.h`, `ui/src/linklauncher.cpp`, `ui/qml/LinkConfirmDialog.qml`, `tests/tst_ui_linklauncher.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_linklauncher` (dont `noOpenWithoutConfirmation`, `refusalOpensNothing`, `onlyHttpAndHttps`, `networkPathRefused`, `executableRefused`, `missingFileRefused`, `storedFileExtractedUnderASafeName`, `confirmationShowsTheFullTarget`, `openingADocumentLaunchesNothing`) Passed ; mutation (ouvrir sans attendre la réponse) rouge puis remise | S11 | #tache-l8-t10 |
| L8-T11 | `core` + `ui` : modèle du navigateur d'actions (catégories, propriétés, appliquer à la sélection en une étape, retirer) | L8-T3, L8-T6 | `core/include/actcraft/core/actionedits.h`, `core/src/actionedits.cpp`, `ui/include/actcraft/ui/actionbrowser.h`, `ui/src/actionbrowser.cpp`, `tests/tst_ui_actionbrowser.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_actionbrowser` (dont `greyedWithoutSelection`, `categoriesInOrder`, `applyToManyIsOneStep`, `removeExistingIsOneStep`, `propertiesFollowTheAction`, `targetPickedFromThePage`, `refreshDeferredDuringContact`, `refusedWhileLocked`, `refusedWithoutCommands`) Passed | S11 | #tache-l8-t11 |
| L8-T12 | QML : navigateur d'actions (onglet « Sélection ») ; scénario `navigateur-actions` | L8-T2, L8-T9, L8-T11 | `ui/qml/ActionBrowser.qml`, `bench/scenarios/navigateur-actions.json` | isolé | implementeur-tdd | `TESTS OK` ; `tst_captures_navigateur_actions` Passed ; `captures.ps1 -Scenario navigateur-actions` : `LU browserPanel.current=actions OK`, `LU appActionBrowser.enabled=true OK`, `LU appDocument.canUndo=true OK`, `PIXEL cible-masquee OK`, `PIXEL cible-revenue OK`, `CAPTURES OK` ; banc hors écran joint | S11 | #tache-l8-t12 |
| L8-T13 | Actions glisser-déposer (F-892) : onglet, dépôt sur un objet, icône en surimpression, retrait par la corbeille ; scénario `glisser-deposer` | L8-T12 | `ui/include/actcraft/ui/dragdropactions.h`, `ui/src/dragdropactions.cpp`, `ui/qml/DragDropActions.qml`, `tests/tst_ui_dragdropactions.cpp`, `bench/scenarios/glisser-deposer.json` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_dragdropactions` (dont `eighteenEntriesNoVote`, `dropAssignsInOneStep`, `overlayOnlyForDragDrop`, `trashRemovesInOneStep`), `tst_captures_glisser_deposer` Passed ; `captures.ps1 -Scenario glisser-deposer` : `LU appTools.current=pen OK`, `PIXEL icone-surimpression OK`, `CAPTURES OK` | S11 | #tache-l8-t13 |
| L8-T14 | `core` : règles de conteneur (F-577, F-580) : accepter, contenir, imbriquer, retour, sortie en un geste | L8-T3 | `core/include/actcraft/core/containers.h`, `core/src/containers.cpp`, `tests/tst_core_containers.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_core_containers` (dont `anythingAcceptsInFront`, `behindIsNeverContained`, `oneObjectOnly`, `commonKeyword`, `centresWithinTolerance`, `fullyInside`, `refusedBouncesBack`, `refusedWithoutBounceStays`, `nestedTopmostWins`, `strokeDrawnInsideIsContained`, `leavingIsOneGesture`, `hiddenContainerIgnored`) Passed ; mutation de la tolérance (`<` en `<=` inversé) rouge puis remise | S11 | #tache-l8-t14 |
| L8-T15 | `core` : limiteurs et rotation (F-578, F-579, E6) : déplacement contraint, chemin, accrochage, dimension bloquée, pivot, pas, sens, objet bloquant | L8-T3 | `core/include/actcraft/core/restrictors.h`, `core/src/restrictors.cpp`, `tests/tst_core_restrictors.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_core_restrictors` (dont `verticalKeepsX`, `horizontalKeepsY`, `fixedTravelZero`, `alongTrackProjects`, `jumpsOntoTrackOnPress`, `landsOnTheMark`, `fixedSizeHasNoHandles`, `clockwiseOnly`, `turnStepRounds`, `pivotOnOtherObject`, `blockerStopsAtItsEdge`) Passed ; mutation de la projection rouge puis remise | S11 | #tache-l8-t15 |
| L8-T16 | `tools` : outil de sélection — limiteurs et rotation pendant le glisser (soulèvement), conteneurs au relâché (retour sans étape), objets bloquants, politique du mode conception ; scénario `conteneurs` | L8-T9, L8-T14, L8-T15 | `tools/include/actcraft/tools/selecttool.h`, `tools/src/selecttool.cpp`, `tools/include/actcraft/tools/interactionpolicy.h`, `tests/tst_tools_interactivity.cpp`, `bench/scenarios/conteneurs.json` | isolé | concepteur-critique | `TESTS OK` ; `tst_tools_interactivity` (dont `bounceBackDropsWithoutAStep`, `acceptedDropIsOneStep`, `verticalDragKeepsX`, `noCacheRebuildDuringDrag`, `fixedSizeShowsNoSizeHandles`, `rotationFollowsTheStep`, `designModeLiftsRestrictions`, `blockerStopsTheDrag`, `containerSignalEmitted`), `tst_tools_select` (lot 3), `tst_captures_conteneurs` Passed ; `captures.ps1 -Scenario conteneurs` : `PIXEL revenu-au-depart OK`, `LU appDocument.canUndo=false OK`, `PIXEL contenu-accepte OK`, `CAPTURES OK` ; banc hors écran joint | S11 | #tache-l8-t16 |
| L8-T17 | `ui` : sections de propriétés Actions, Conteneur, Rotation, Limiteurs, Multimédia (F-577 à F-579, F-582) dans le navigateur de propriétés | L8-T3, L8-T11 | `ui/include/actcraft/ui/interactivityprops.h`, `ui/src/interactivityprops.cpp`, `ui/src/propertymodel.cpp`, `tests/tst_ui_interactivityprops.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_interactivityprops` (dont `fiveSectionsInOrder`, `multimediaOnlyForMedia`, `editIsOneStep`, `fixedTravelGreysTheTrack`, `keywordsFromLot6`, `refusedWhileLocked`), `tst_ui_propertymodel` Passed ; banc hors écran joint | S11 | #tache-l8-t17 |
| L8-T18 | Encre révélatrice (F-255) : outil, découpe de la couche du dessus (logiciel et RHI), redessin immédiat ; scénario `encre-revelatrice` | L8-T1, L8-T2 | `tools/include/actcraft/tools/revealinktool.h`, `tools/src/revealinktool.cpp`, `render/src/pagepainter.cpp`, `render/src/fixedpageitem.cpp`, `tests/tst_render_reveal.cpp`, `bench/scenarios/encre-revelatrice.json` | isolé | concepteur-critique | `TESTS OK` ; `tst_render_reveal` (dont `revealsMiddleLayer`, `onlyTopObjectsBelowIt`, `objectAboveItStays`, `noOwnPaint`, `movingMovesTheHole`, `revealStrokeIsNotDeferred`, `softwareAndRhiAgree`), `tst_captures_encre_revelatrice` Passed ; `captures.ps1 -Scenario encre-revelatrice` : `LU appTools.current=reveal-ink OK`, `PIXEL fond-revele OK`, `PIXEL dessus-garde OK`, `CAPTURES OK` ; banc hors écran joint | S11 | #tache-l8-t18 |
| L8-T19 | Point d'étape 8a : S11 synthétique `interactivite` de bout en bout ; S11 local sur le paperboard réel n° 13 ; talons de 8a ; contrôles de sécurité ; graphe | L8-T4, L8-T5, L8-T10, L8-T12, L8-T13, L8-T16 à L8-T18 | `bench/scenarios/interactivite.json`, `tests/tst_formats_samples_interactivity.cpp`, `graphify-out/` | partagé | implementeur-tdd | `TESTS OK`, N = N8 + 21 ; `tst_captures_interactivite` Passed ; `captures.ps1 -Scenario interactivite` : `PIXEL etiquette-masquee OK`, `PIXEL etiquette-revenue OK`, `PIXEL revenu-hors-conteneur OK`, `CAPTURES OK` ; localement `tst_formats_samples_interactivity` Passed (sinon `SKIP` « ACTCRAFT_SAMPLES_DIR absent », jamais « Talon ») ; talons « rempli par L8-T2 » à « par L8-T18 » absents ; contrôle d'occurrences de `QDesktopServices` conforme ; `NOMS OK` ; `graphify update .` commité | S11 | #tache-l8-t19 |

20 tâches dans ce sous-lot (15 dans 8b : 35 pour le lot, plus `L8-P1` si L8-T0 en propose une, amendement 1, § 4 :
« lot 8 + 1 »). Tests ctest : L8-T1 enregistre d'un coup les **21** tests de 8a (16 unitaires, 5 de captures),
talons `QSKIP` jusqu'à ce que leur tâche les remplisse : **N8 + 21** dès L8-T1, où N8 est le nombre de tests au départ
du lot, relevé par L8-T0 et comparé à la fin du lot 6 (N6 + 43 + p6, état du chantier) ; un écart est rapporté, pas
corrigé. L8-T21 (8b) enregistre les 16 tests de 8b : **N8 + 37** ensuite, **N8 + 37 + p** si `L8-P1` enregistre `p`
tests. Aucune autre tâche n'en enregistre. Un talon sauté ne prouve rien : chaque preuve nomme les fonctions attendues
`PASS`.

Worktrees : `.worktrees/livraison-complete--L8-T<n>`, branche `feat/livraison-complete--L8-T<n>`, issue de
`feat/livraison-complete` à jour des tâches déjà mergées. Tous isolés sauf L8-T19 (point d'étape, sur le worktree du
chantier).

**Écran.** Aucune tâche de 8a n'ouvre de fenêtre visible : tout se joue hors écran (rendu logiciel). La clôture du lot
(L8-T34, fin de 8b) joue les captures et le banc à l'écran sous le jeton `ecran`, annoncés à Jérôme d'une ligne.

## Vagues (largeur 3 au plus ; 2 places de tests)

V1 : L8-T0 ‖ L8-T1 · V2 : L8-T2 ‖ L8-T3 ‖ L8-T6 · V3 : L8-T4 ‖ L8-T5 ‖ L8-T10 · V4 : L8-T7 ‖ L8-T14 ‖ L8-T15 ·
V5 : L8-T8 ‖ L8-T11 ‖ L8-T18 · V6 : L8-T9 ‖ L8-T17 · V7 : L8-T12 ‖ L8-T16 · V8 : L8-T13 · V9 : L8-T19 · puis 8b.

Vérification des recoupements (colonne « fichiers touchés ») :
- V1 : L8-T0 (`docs/verifications/`) / L8-T1 (CMake, `tests/*.cmake`, données, icônes, talons) — disjoints.
- V2 : L8-T2 (QML de la coquille, `tst_ui_shell.cpp`) / L8-T3 (`model.*`, `interactivity.*`, deux tests) / L8-T6
  (`object-actions.json`, `objectactions.*` d'`actions/`, un test) — disjoints.
- V3 : L8-T4 (`nativejson.cpp`, un test) / L8-T5 (`flipchart.cpp`, `flipchartids.h`, un test) / L8-T10
  (`linklauncher.*`, `LinkConfirmDialog.qml`, un test) — disjoints.
- V4 : L8-T7 (`core/objectactions.h`, `objectactions_geometry.cpp`, un test) / L8-T14 (`containers.*`, un test) /
  L8-T15 (`restrictors.*`, un test) — disjoints.
- V5 : L8-T8 (`objectactions_edit.cpp`, un test) / L8-T11 (`actionedits.*`, `actionbrowser.*`, un test) / L8-T18
  (`revealinktool.*`, `pagepainter.cpp`, `fixedpageitem.cpp`, un test, un scénario) — disjoints.
- V6 : L8-T9 (`actionrunner.*`, `pagehistory.*`, `toolhost.cpp`, `ActionOverlays.qml`, un test) / L8-T17
  (`interactivityprops.*`, `propertymodel.cpp`, un test) — disjoints.
- V7 : L8-T12 (`ActionBrowser.qml`, un scénario) / L8-T16 (`selecttool.*`, `interactionpolicy.h`, un test, un
  scénario) — disjoints.
- Fichiers repris d'une vague à l'autre, tenus par « dépend de » : talons de L8-T1 → leur tâche ; QML des
  emplacements (L8-T2 → leur tâche) ; `core/objectactions.h` (L8-T7 → L8-T8) ; `tools/interactionpolicy.h` (talon
  L8-T1 → L8-T16 ; L8-T9 l'inclut sans le modifier : il lit la politique par l'interface déjà déclarée par L8-T1).

**Places.** Le jeton `tests` a 3 places, dont **2 pour ce lot** (décision 0121 : les revues statiques tournent à côté).
Une vague de 3 tâches qui construisent en a toujours une qui attend la notification du jeton : c'est prévu, l'agent
attend, il ne sonde pas. L8-T0 ne construit rien. Ordre de lancement dans une vague : la tâche du chemin critique
d'abord (V2 : L8-T3 ; V4 : L8-T7 ; V5 : L8-T8).

**L8-T0 et les autres tâches.** L8-T1 (données, mécanique) ne l'attend pas ; toutes les tâches de comportement
l'attendent (V2 commence quand V1 est finie). Ses décisions D1 à D16 et sa table A1 à A30 **amendent ce plan et celui
de 8b** avant V2 (relecture du plan exigée). Une décision qui change une donnée de commande (raccourci, libellé, ordre)
passe par un correctif `L8-C0` (`actions/data/commands.json`, `actions/data/layout.json` seulement), joué seul avant
V2. Une API supposée qui diffère du code livré amende la section de la tâche qui l'emploie.

## Commandes de preuve communes

- `<wt>` = `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L8-T<n>` ; `<id>` = `L8-T<n>`.
- **Commande ciblée** (rouge, vert d'un seul test) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande '. ./scripts/dev-env.ps1; cmake --build --preset debug *> .tmp/build.txt; ctest --preset debug -R ''<motif>'' --output-on-failure *> .tmp/ctest.txt'; Select-String -Path .tmp/build.txt,.tmp/ctest.txt -Pattern 'error|tests passed|tests failed|No tests were found' | Select-Object -Last 4"
  ```
  Premier passage dans un worktree neuf : la même commande avec `-Commande 'pwsh -NoProfile -File ./scripts/build.ps1 *> .tmp/build.txt'`.
  Attendu au vert : `100% tests passed, 0 tests failed out of <k>`, k ≥ 1 (jamais `No tests were found`).
- **Fonctions d'un test** (chaque test écrit **son propre** fichier, `-o .tmp/<test>.txt,txt`, jamais `f`) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; . ./scripts/dev-env.ps1; & build/debug/tests/<test>.exe -o .tmp/<test>.txt,txt; Select-String -Path .tmp/<test>.txt -Pattern '^PASS   :|^FAIL!  :|^SKIP   :|^Totals' | Select-Object -Last 40"
  ```
  Attendu pour un talon rempli : chaque fonction nommée par la table en `PASS`, aucun `SKIP`,
  `Totals: <k> passed, 0 failed, 0 skipped`, k égal au nombre de fonctions écrites dans la section.
  Liste des fonctions sans les lancer : `& build/debug/tests/<test>.exe -functions` ; un talon y montre `reserved()`,
  un test rempli ne la montre plus (règle 4 des talons, ci-dessous).
- **Preuve de tâche** (forme bornée du profil, sous le jeton) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1'; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2; Select-String -Path .tmp/test.txt -Pattern '<nom_du_test> .*Passed' | Select-Object -Last 1"
  ```
  Attendu : `100% tests passed, 0 tests failed out of N` (N = N8 + 21 dès L8-T1), `TESTS OK`, puis la ligne `Passed`
  du test nommé. Selon la décision 0121, une tâche joue la commande ciblée **plus** cette preuve une fois avant son
  commit final ; la suite complète en debug et release est lancée par l'orchestrateur une fois par vague, après les
  merges.
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
  `ECART`, toute ligne d'étape en `ECHEC`. Dans les scénarios et les lectures, la vue s'appelle `pageView` (objet
  trouvé par son `objectName`) ; **dans le code QML**, elle s'atteint par `pageArea.view` (piège de l'état du chantier).
- **Contrôle des noms** (après `git add`, avant le commit, puis après le commit) :
  ```powershell
  python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" "<wt>" | Select-Object -Last 1
  ```
  Attendu : `NOMS OK : <n> fichiers, 0 nom composé d'Inspire`. Les identifiants de ce plan évitent les noms composés
  des propriétés de l'original (conteneur, accrochage, type d'encre, options de média) : un échec renomme, il ne
  s'exclut pas.
- **Contrôle des titres du corpus** (toute tâche qui écrit un texte visible ou un document) : pour chaque fiche du
  corpus, la première ligne `# …` lue hors dépôt, cherchée dans le worktree par `git grep -l -i -F` ; **sortie vide**
  attendue :
  ```powershell
  pwsh -NoProfile -Command "Get-ChildItem 'C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay' -Filter *.md -Recurse | Where-Object Name -ne 'README.md' | ForEach-Object { (Get-Content `$_.FullName -TotalCount 1) -replace '^#\s*','' } | Where-Object { `$_.Trim().Length -ge 8 } | ForEach-Object { git -C '<wt>' grep -l -i -F -e `$_ -- . ':(exclude)graphify-out' }"
  ```
- **Contrôle des paperboards réels** (L8-T5, L8-T19, toute tâche qui écrit un document) : noms des fichiers de
  `samples/` **lus hors dépôt**, cherchés dans le worktree ; sortie vide attendue :
  ```powershell
  pwsh -NoProfile -Command "Get-ChildItem 'C:\Users\JeromeDenis\REA - Activinspire\samples' -File | ForEach-Object { `$_.BaseName } | Where-Object { `$_.Length -ge 5 } | ForEach-Object { git -C '<wt>' grep -l -i -F -e `$_ -- . ':(exclude)graphify-out' }"
  ```
- **Mutations** : une mutation demandée est faite dans le fichier, prouvée rouge par la commande ciblée, puis remise
  par `git -C <wt> checkout -- <fichier>` (jamais par `Copy-Item`), et la commande ciblée rejouée verte.
- **Tests de captures** qui dépendent d'une plateforme (rendu D3D11, enregistreur d'écran) : garde
  `#ifdef Q_OS_WIN` ou `QSKIP("Windows seulement")` dans la fonction, jamais « Talon ».

## Décisions appliquées (tranchées par ce plan ; L8-T0 peut les amender)

- **D8-1 — Une action = un identifiant de notre catalogue.** Le modèle garde `core::Action` du lot 2 (lu des
  `.flipchart` : `kind`, `sourceType`, `command`, `target`, `resource`, `unknownKeys`) et gagne trois champs :
  `catalogId` (identifiant à nous, en anglais à points, par exemple `page.next`, `object.toggle-hidden`,
  `command.tool.pen`, `document.open-website`), `parameters` (objet JSON compact : numéro de page, angle, texte,
  URL…) et `dragDrop` (action posée par l'onglet « Glisser-déposer », F-892). Le catalogue est une **donnée** à nous,
  `actions/data/object-actions.json` (`actcraft-object-actions/1`), quatre catégories : `command`, `page`, `object`,
  `document` ; **aucune action de vote** (amendement 1, § 1). Une action de catégorie `command` porte l'identifiant
  d'une commande du registre et s'exécute par `CommandRegistry::trigger` : le verrou de classe (D6-7) et la diffusion
  protégée (8b) s'y appliquent sans code de plus. Une action d'un `.flipchart` dont le numéro de commande n'est pas
  dans notre table de correspondance (écrite à la main, L8-T5) garde `catalogId` vide : elle est conservée, affichée
  « action non prise en charge » dans le navigateur, et ne fait rien au clic.
- **D8-2 — Déclenchement (F-890).** Un clic (appui puis relâché à moins de 4 px de l'appui, sans glisser) sur un
  objet qui porte une action déclenche **toutes** ses actions dans l'ordre, au relâché, hors mode conception, avec
  l'outil de sélection ; le clic droit sélectionne sans déclencher ; au survol, le curseur change. Avec un autre outil
  que la sélection : défaut à trancher par L8-T0 (décision **D3** ci-dessous ; défaut du plan : seule la sélection
  déclenche, les autres outils écrivent par-dessus, comme la référence en Python). Une action ne s'exécute jamais
  pendant un contact : elle part au relâché, quand `contactActive` est faux.
- **D8-3 — Exécution par la pile d'annulation.** Une action d'objet ou de page qui modifie le document produit **une**
  commande d'annulation (`DocumentController::execute`) ; « Annuler » la défait en une étape. Les actions de
  navigation (page suivante, historique) ne modifient pas le document et ne créent aucune étape.
- **D8-4 — Liens et fichiers : jamais sans confirmation.** Un lien vers un site (F-551), un fichier (F-550), une action
  `document.open-file` ou `document.open-website`, ou un média confié à l'application du système (N2-9) passe par
  `ui::LinkLauncher` : il **demande toujours une confirmation** (boîte à nous qui montre l'adresse ou le chemin
  complet) puis appelle `QDesktopServices::openUrl`. Règles d'adresse : site = `http` ou `https` seulement ; fichier
  = chemin local existant, jamais un chemin réseau (`\\hôte\…`, `file://hôte/…`, SMB), jamais un exécutable
  (`.exe`, `.bat`, `.cmd`, `.com`, `.msi`, `.ps1`, `.vbs`, `.js`, `.lnk`, `.scr`) — un fichier refusé affiche son
  motif ; fichier rangé dans le document (« stocker dans le paperboard ») : extrait dans un dossier temporaire propre
  au document, sous son nom d'origine assaini, puis confirmé. **Un fichier d'un `.flipchart` ne lance rien tout
  seul** : ni à l'ouverture, ni à l'affichage d'une page, ni par « lecture auto » ; la lecture automatique d'un média
  ne vaut que pour le lecteur intégré (8b), jamais pour l'application du système.
- **D8-5 — HTML de document.** Toute ressource HTML (texte d'un objet, cadre web, aide) suit la règle de l'état du
  chantier : « Tout rendu de HTML de document passe par un `QTextDocument` dont `loadResource` est neutralisé
  (L2-T11). Règle pour les lots suivants (outil texte, lot 4 ; export) : jamais `setHtml`/`Text.RichText` sur du
  contenu de document sans ce garde. » Les actions « Ajouter du texte », « Modifier le texte », « Nouvel objet de
  texte » et « Extraire le texte » (L8-T8) écrivent du texte brut échappé, jamais du HTML reçu ; le cadre web (8b) a
  sa propre politique (N1-A3), plus stricte.
- **D8-6 — Conteneurs (F-577, F-580).** Logique pure dans `core/containers.*` : au relâché d'un glisser, le contenu
  candidat (devant le conteneur : couche plus haute, ou même couche et plus haut dans la superposition) est
  « contenu » s'il satisfait la règle (`Anything`, objet précis, mot-clé commun) et la condition (entièrement contenu,
  ou centres alignés à 10 px près, réglable). Sinon, si « retour si non contenu » est vrai, le glisser est **annulé**
  : les objets soulevés reviennent à leur place, **aucune étape** d'annulation, aucun redessin du cache autre que la
  fin du soulèvement. Conteneurs imbriqués : le plus haut dans la superposition qui accepte l'emporte. Signal de
  confirmation : émis (`containerAccepted`, son `acceptSound`), le son est lu par le lecteur intégré au sous-lot 8b.
- **D8-7 — Limiteurs et rotation (F-578, F-579, E6).** Logique pure dans `core/restrictors.*` : déplacement libre,
  vertical, horizontal, le long d'un chemin (projection sur la polyligne d'un objet trait ou ligne), ou bloqué ;
  accrochage à un point (centre, coins, côtés, point précis) ; dimension libre ou bloquée (« bloqué » = E6, tranché
  par L8-T0, défaut : aucune poignée de dimension) ; rotation libre, sens horaire, anti-horaire, bloquée, pas
  d'incrément, pivot (centre, autre point, autre objet) ; objet bloquant (un autre objet glissé s'arrête à son bord).
  Les booléens du lot 2 (`Restrictions::movable`, `resizable`, `rotatable`) restent la source du « bloqué » : un mode
  « bloqué » les met à faux, tout autre mode à vrai (invariant testé par L8-T3). L'outil de sélection les applique
  **pendant** le glisser, sur les objets soulevés (lot 3) : aucun redessin du cache pendant l'écriture.
- **D8-8 — Encre révélatrice (F-255).** Outil `tool.reveal-ink` (Ctrl+Maj+M), variante du stylo qui pose un
  `core::Stroke` de genre `StrokeKind::Revealer` sur la couche du dessus. Rendu : partout où passe le trait, les
  objets de la couche du dessus placés **sous** lui dans la superposition deviennent transparents (découpe), les
  couches intermédiaire, inférieure et le fond restent. Réalisation retenue (décision 0100, complément, à signaler
  au rapport de lot) : pendant le tracé, le trait vivant est montré comme un contour translucide neutre (aucune
  découpe en direct) ; au relâché, le trait est versé et le cache redessiné **tout de suite** — le prédicat `canWait`
  du lot 10 le refuse déjà (images et encre révélatrice non différées) et ce plan ne le change pas. Le navigateur
  d'objet le liste ; on peut le sélectionner, le déplacer (la découpe suit), le supprimer.
- **D8-9 — Mode conception (F-560, L6-T22).** Ses réglages « mettre en évidence les actions » (cadre rouge à nous sur
  les objets d'action, surcouche QML hors cache), « désactiver les actions et glisser une copie », « désactiver les
  restrictions d'objet », « désactiver les objets bloquants » sont lus par l'exécuteur d'actions (L8-T9) et par
  l'outil de sélection (L8-T16) à travers une interface (`tools::InteractionPolicy`) : `tools/` ne connaît pas `ui/`.
- **D8-10 — Navigateur d'actions (F-850, F-851).** Onglet `actions` du panneau des navigateurs (posé grisé par le lot 6,
  A8) : catégorie, liste des actions du catalogue, propriétés de l'action, « Appliquer » à tous les objets
  sélectionnés en **une** étape (`core::SetObjectActionsCommand`), « Retirer les actions existantes ». Le modèle se
  regroupe et se reporte pendant un contact, comme les navigateurs du lot 6 (D6-5).
- **D8-11 — Glisser-déposer (F-892).** Une icône de l'onglet déposée sur un objet lui donne l'action correspondante
  (`dragDrop = true`) en une étape ; l'icône de l'action est dessinée en surimpression, dans une surcouche QML
  au-dessus de la page (jamais dans le cache) ; glisser cette icône vers la corbeille retire l'action. Liste à nous,
  sans les deux entrées de vote.
- **D8-12 — Historique des pages.** Les actions « historique précédent / suivant » lisent un historique des pages
  affichées tenu par `ui::PageHistory` (pile bornée à 100, navigation par historique non réinscrite).

## Sécurité (rappel pour chaque tâche)

- `QDesktopServices::openUrl` n'est appelé **que** par `ui::LinkLauncher::confirmAndOpen`, après la confirmation ; un
  test (`noOpenWithoutConfirmation`) remplace l'ouvreur par un faux et compte les appels ; le contrôle d'occurrences
  (section « Sécurité : contrôle d'occurrences ») le vérifie en L8-T19 et L8-T34.
- Un `.flipchart` ou un `.actcraft` ouvert ne lance rien : aucun lien, fichier, média par l'application du système,
  ni cadre web distant, sans un clic de l'utilisateur **et** une confirmation.
- Toute ressource HTML : règle `loadResource` de l'état du chantier, citée en D8-5 par son texte.
- Aucun chemin de fichier lu d'un document n'est passé à un interpréteur ni à `QProcess`.

## Cache (règles du lot 10)

- Actions, conteneurs, limiteurs : aucun redessin du cache pendant un contact ; une action part au relâché ; un retour
  hors conteneur annule le soulèvement sans étape.
- Un déplacement passe par le **soulèvement** du lot 3 (objets soulevés au-dessus du cache pendant le glisser).
- Encre révélatrice : `canWait` inchangé (images et encre révélatrice non différées) ; test `revealStrokeIsNotDeferred`
  (L8-T18).
- Surimpressions (cadre rouge du mode conception, icônes des actions glisser-déposer, repère « action non prise en
  charge ») : surcouches QML au-dessus de la page, jamais dans le cache.

## API supposées (posées par les lots précédents ; chacune **à revérifier par L8-T0** sauf mention « vérifié »)

| # | API | Lot qui la pose | Employée par | À revérifier |
|---|---|---|---|---|
| A1 | `core::Action { kind, sourceType, command, target, resource, unknownKeys }`, `ActionKind { Other, Navigate, OpenExternal, OnObject }`, `Restrictions { movable, resizable, rotatable }`, `ObjectCommon::actions`, `StrokeKind::Revealer`, `core::Media`, `ObjectKind` (Equation 8, Unknown 9, `kObjectKindCount` 10), `UnknownObject` | lot 2, L2-C8, L0-S3 | L8-T3 à L8-T5 | non (vérifié sur `5373f04`, `model.h:143-359`) |
| A2 | `canWait(page, object)` refuse images, encre révélatrice, grille du dessus | lot 10 | L8-T18 | non (vérifié, `render/src/fixedpageitem.cpp:55-64`) |
| A3 | `paintPage`, `paintObject`, `paintObjectLocal`, `paintStroke` : l'encre révélatrice y est dessinée comme un trait ordinaire | lot 2, L0-S3 | L8-T18 | non (vérifié, `render/src/pagepainter.cpp:193-213,626-723`) |
| A4 | lecteur `.flipchart` : actions 22 à 28, 50, 463 lues et rattachées au dernier objet (`formats/src/flipchart.cpp:562-568,614-621`, `actionFrom` 667-719) ; communs lus : visible, verrou, trois booléens (998-1002) ; conteneurs et limiteurs non lus | lot 2 | L8-T5 | non (vérifié) |
| A5 | écrivain synthétique `tests/support/flipchartwriter.h` | lot 7 | L8-T5 | oui |
| A6 | `actions::CommandRegistry` : `loadDeclarations`, `find`, `isEnabled`, `trigger`, `setLocked`, `isLocked`, `isOwnEnabled` ; `CommandInfo::lockable` (défaut vrai), `restrictable` (défaut faux) | lot 1, L6-T3 | L8-T1, L8-T6, L8-T9, 8b | oui (absents de `5373f04`) |
| A7 | `ui::Commands` (`registry()`, `setNativeHandler`, `stateChanged`) ; instance QML `appCommands` ; contrôleurs « fermés par défaut » (`commands` nul : refus) | lot 1, L6-T5, amendement 2 du lot 6 (B1) | L8-T9, L8-T11, L8-T17 | oui |
| A8 | `ui::DocumentController::execute(std::unique_ptr<core::UndoCommand>)`, `document()`, `currentPageIndex()`, `currentPageChanged`, `addChangeObserver`, `ChangeOrigin` (`Lift`, `Drop`) | L0-S5, lot 2 | L8-T9, L8-T11, L8-T13, L8-T16 | oui |
| A9 | soulèvement : `DocumentController::liftObjects(std::vector<core::ObjectId>)` (copies retirées de la page affichée, le cache reçoit `remove`, sans `editAboutToApply`) et `dropLifted(std::unique_ptr<core::UndoCommand>)` (`nullptr` : dépôt sans étape) | lot 3 | L8-T16 | oui |
| A10 | `core::TransformObjectsCommand(std::size_t page, std::vector<ObjectId>, …)` | lot 3 | L8-T7, L8-T16 | oui |
| A11 | outil de sélection : `tools/include/actcraft/tools/selecttool.h`, `tools/src/selecttool.cpp`, `tools/src/selectiongeometry.cpp` | lot 3 | L8-T16 | oui |
| A12 | `ui::ToolHost` (`ui/src/toolhost.cpp`) : `user()`, `activeTool()`, `setSelection(QList<quint64>)`, `selectionChanged` | L0-S6, lot 3 | L8-T9 | oui |
| A13 | garde de contact `ui::PageView::contactActive` / `contactActiveChanged` (L4-T17) ; en QML `pageArea.view.contactActive` | lot 4 | L8-T9, L8-T11, L8-T16 | oui |
| A14 | `pageArea.view.addInputCover(QQuickItem*)`, `removeInputCover` ; emplacement `pageOverlays` (rideau z 10, spot z 20, outils de maths z 30) | L0-S7, L0-S9, lot 7 | L8-T2, L8-T9 | oui |
| A15 | `BrowserPanel.qml` : `current` (`pages`, `resources`, `objects`, `notes`, `properties`, `actions`) ; onglet `actions` grisé par le lot 6 | L4-T19, L6-T2 | L8-T2, L8-T12 | oui |
| A16 | `core::ObjectFlag { Locked, Hidden, DragCopy }`, `SetObjectFlagCommand`, `ReorderObjectsCommand`, `SetLayerCommand`, `arrangedOrder` | lot 5 | L8-T8 | oui |
| A17 | `core::SetTranslucencyCommand` | lot 3 | L8-T8 | oui |
| A18 | presse-papiers d'objets (`formats::encodeClipboard`, `decodeClipboard`, `resourcesFor`) ; commandes `edit.copy`, `edit.cut`, `edit.paste`, `edit.duplicate`, `edit.delete` | lot 5 | L8-T6, L8-T8 | oui |
| A19 | commandes de page : `page.first`, `page.last`, `page.goto`, `page.insert-after`, `page.insert-before`, `page.duplicate`, `page.clear`, `page.reset`, `page.copy`, `page.cut`, `page.background` | lots 3 et 4 | L8-T6, L8-T9 | oui |
| A20 | `core::ReplaceObjectsCommand`, contrat de `Change` | L0-S3 | L8-T8, L8-T11 | oui |
| A21 | mots-clés d'objet (`ObjectCommon::keywords`, L6-T10) | lot 6 | L8-T14, L8-T17 | oui |
| A22 | `ui::PropertyModel` (sections, éditeurs, `setValue`), `core::SetObjectPropertiesCommand` | L6-T11, L6-T12 | L8-T17 | oui |
| A23 | mode conception `ui::DesignMode` et ses réglages (actions et glisser une copie désactivés, restrictions désactivées, objets bloquants désactivés, actions mises en évidence) | L6-T22 | L8-T9, L8-T16 | oui |
| A24 | stylo : `tools::PenTool`, mémoire par outil, `activate("pen-shape", …)` (encre révélatrice annoncée par le lot 5) ; registre `tools::ToolRegistry` | lots 3 et 5 | L8-T18 | oui |
| A25 | `formats/src/nativejson.cpp` : `writeCommon` (`restrictions{move,size,rotate}`, `actions[]`), `KnownKeys`, `readUnknown`, `followsVariantOrder` | lots 2, 6, socle | L8-T3, L8-T4 | non pour les noms (vérifié, `nativejson.cpp:379-390,528-548,693-712,831-841,1397-1430`) ; oui pour les ajouts du lot 6 |
| A26 | joueur de scénarios : étapes `pointer` (appui, glisser, relâché, clic droit), `typeText`, `drag` vers un objet QML, `restart` (L6-T18) | lot 1, L0-S2, L4-T14, L6-T18 | L8-T12, L8-T13, L8-T16, L8-T18, L8-T19 | oui |
| A27 | corbeille sous la palette (dépôt d'un élément glissé) | lot 3 | L8-T13 | oui |
| A28 | navigateur d'objet `ui::ObjectTreeModel` | L6-T14 | L8-T18 | oui |
| A29 | commandes du lot 7 : `tool.camera-area`, `tool.camera-polygon`, `tool.camera-freehand`, `tool.camera-window`, `tool.camera-screen` (le plan du lot 6 écrit `camera.*` : à trancher par L8-T0), `present.curtain`, `math.ruler`, `math.protractor`, `math.compass` ; ouvreur injectable de `tool.web-browser` (`ui/src/presentationcommands.*`, `QDesktopServices::openUrl`) ; épinglages `docs/verifications/*-lot7-epinglages.md` | lot 7 | L8-T6, L8-T19, 8b | oui |
| A30 | réglages publiés du lot 6 : sections « Enregistrements » et « Multimédia » (`ui/settingsschema.h`, L6-T27) ; bibliothèque de ressources `ui::ResourceLibrary` (L6-T20) | lot 6 | 8b | oui |

## Talons : règle d'honnêteté

1. **Marqueur unique** `Talon L8 : rempli par <tâche>.` — en commentaire C++ dans chaque `.h`/`.cpp` talon, en
   commentaire QML dans chaque `.qml` talon, dans le champ `description` de chaque scénario JSON talon, et comme texte
   du `QSKIP` de chaque test talon : `QSKIP("Talon L8 : rempli par L8-T14.");`.
2. Un test talon n'a qu'une fonction sautée, `reserved()` ; la tâche propriétaire la supprime et ajoute les siennes.
3. Compteur : N = N8 + 21 dès L8-T1 (N8 + 37 dès L8-T21) ; chaque preuve donne N exact **et** les fonctions attendues
   en `PASS`.
4. Contrôle (L8-T19 pour 8a, L8-T34 pour le lot) : `& build/debug/tests/<test>.exe -functions` ne montre `reserved`
   pour aucun test du sous-lot ; `git -C <wt> grep -n "Talon L8 : rempli par" -- . ':(exclude)docs/'` ne rend plus
   que des tâches de 8b à la fin de 8a, et rien à la fin du lot ; sur `ctest --preset debug -V`, aucune ligne
   `SKIP   :` qui contienne `Talon` (restent permis : « ACTCRAFT_SAMPLES_DIR absent », « Windows seulement », et les
   sauts déjà connus des tests d'oracle).

## Sécurité : contrôle d'occurrences (L8-T19, L8-T34)

`git -C <wt> grep -n -E "QDesktopServices|openUrlExternally" -- core formats actions tools render ui app media features`
ne rend que : `ui/src/linklauncher.cpp` (ouvreur des liens du document, après confirmation), `ui/src/presentationcommands.cpp`
(navigateur web du lot 7, adresse d'accueil réglée par l'utilisateur, jamais tirée d'un document), `ui/qml/AboutDialog.qml`
et `ui/qml/Main.qml` (page du projet, adresse fixe). Toute autre ligne refuse la tâche.

## Scénarios

Scénarios servis par le lot : **S11** (cadrage), **S18**, **S25**, **S26**, **S27** (amendement 1), **S30**
(ouverture), **S32** (préparé : épinglages du profil « Son et image »). Sous-scénarios de S11 posés par ce plan, joués
sur des documents synthétiques écrits par nos tests (jamais un paperboard réel dans le dépôt) :

- **S11.1 — Action d'objet.** Un rond porte l'action « masquer ou afficher » visant une étiquette : un clic avec la
  sélection masque l'étiquette (pixels), un second la réaffiche ; un glisser du rond ne déclenche rien ; Annuler
  défait le dernier masquage en une étape ; en mode conception, le clic sélectionne le rond.
- **S11.2 — Navigateur d'actions.** Sans sélection, la liste est grisée ; un objet sélectionné, catégorie « Page »,
  action « page suivante », Appliquer : un clic sur l'objet passe à la page 2 ; « retirer les actions » : le clic ne
  fait plus rien.
- **S11.3 — Glisser-déposer.** L'icône « Stylo » déposée sur un objet : l'icône s'affiche en surimpression ; un clic
  sur l'objet active le stylo ; l'icône glissée vers la corbeille retire l'action.
- **S11.4 — Conteneur.** Un panier accepte les objets de mot-clé « fruit », retour si non contenu : une pomme déposée
  dedans y reste (une étape) ; une carotte (« légume ») revient à sa place, aucune étape ; un objet derrière le panier
  n'est jamais contenu.
- **S11.5 — Limiteurs.** Un objet limité « verticalement » glissé en diagonale garde son X ; un objet à dimension
  bloquée n'a pas de poignée de dimension ; une rotation au pas de 45° s'arrête sur un multiple de 45.
- **S11.6 — Encre révélatrice.** Une image sous un rectangle de la couche du dessus : un trait d'encre révélatrice
  sur le rectangle révèle l'image (pixels) ; un rectangle posé ensuite au-dessus reste opaque ; déplacer le trait
  déplace la zone révélée.
- **S11.7 — Lien.** Un objet lié à `https://example.org/` : le clic affiche la confirmation avec l'adresse complète ;
  refuser n'ouvre rien ; accepter appelle l'ouvreur une fois (faux ouvreur des tests) ; à l'ouverture d'un document
  dont un objet porte un lien et un média « lecture auto », rien ne s'ouvre.
- **S11 local.** Le paperboard réel n° 13 de `samples/` (lu hors dépôt, jamais nommé) : un clic sur un rond rouge
  masque puis réaffiche l'étiquette visée ; un objet déposé hors de son conteneur revient à sa place. Test
  `tst_formats_samples_interactivity`, sauté sans `ACTCRAFT_SAMPLES_DIR`.

| Scénario | Tâches (8a et 8b) |
|---|---|
| S11 | L8-T1 à L8-T19 |
| S11.1 | L8-T3 à L8-T9, L8-T19 |
| S11.2 | L8-T6, L8-T11, L8-T12 |
| S11.3 | L8-T13 |
| S11.4 | L8-T14, L8-T16, L8-T17 |
| S11.5 | L8-T15, L8-T16, L8-T17 |
| S11.6 | L8-T18 |
| S11.7 | L8-T10, L8-T19 ; dialogues : L8-T22 |
| S18 | L8-T3, L8-T4, L8-T31, L8-T32 |
| S25 | L8-T3, L8-T4, L8-T20, L8-T23 à L8-T25 |
| S26 | L8-T20, L8-T26 à L8-T28 |
| S27 | L8-T3, L8-T4, L8-T20, L8-T29, L8-T30 |
| S30 | L8-T0 |
| S32 (préparé) | L8-T33 |

## Hypothèses pour Jérôme (créées par ce plan)

- **H8-1** Catalogue d'actions à nous, sans les 8 actions de vote ni les 2 entrées de vote du glisser-déposer ; une
  action de commande déclenche une commande d'ActCraft ; une commande que nous n'avons pas (services en ligne de
  Promethean) n'a pas d'action.
- **H8-2** Seule la sélection déclenche les actions (défaut, D3 de L8-T0) ; avec le stylo, on écrit par-dessus.
- **H8-3** Toute ouverture d'un site, d'un fichier ou d'un média par l'application du système demande une
  confirmation, à chaque fois ; exécutables et chemins réseau refusés, même confirmés. Écart voulu avec la référence.
- **H8-4** Encre révélatrice : contour translucide pendant le tracé, découpe au relâché (décision 0100, complément).
- **H8-5** Conteneur : retour instantané à la place de départ, sans animation ni étape d'annulation.
- **H8-6** Téléchargement des modules Qt Multimedia (avec FFmpeg) et Qt WebView pour Windows et Android, par
  aqtinstall depuis les miroirs de Qt (absents de `C:\Qt\6.12.0\msvc2022_64`, vérifié le 2026-10-19) : **accord de
  Jérôme à demander** avant L8-T20 ; taille ajoutée aux paquets rapportée (N1-A10). Sans accord : médias par
  l'application du système après confirmation (N2-9), enregistreurs et cadre web absents, 8b replanifié.
- **H8-7** Poids du cadre web (N1-A3) mesuré par L8-T0 puis L8-T20 ; F-302 derrière l'option `ACTCRAFT_WEBFRAME`.

H8-6 est posée dès la fin de L8-T0 ; les autres à la porte de fin du lot 8.

## Sections des tâches

<a id="tache-l8-t0"></a>
### Tâche L8-T0 — Ouverture : vidéos, REA et ancien dépôt confrontés au lot

**Objectif.** Appliquer la règle de la tâche d'ouverture (amendement 1, § 4, points 1 à 6 ; décisions 0100 et 0121)
au lot 8, sur le modèle de `docs/verifications/2026-10-18-lot10-principes.md` : relever dans le corpus les principes
des fonctions du lot (14 constats de l'amendement, 17 en séparant les sous-puces, V39 en plus) et ceux de la
spécification ; trancher sur la 3.5.18, par REA en statique, les points « à élucider » de l'amendement (F-680 : V58 ;
F-292 : V43 ; F-302 : V63 ; F-552 : V126, V57) et l'écart **E6** ; revérifier A1 à A30 ; relever **N8** ; estimer le
poids des modules du sous-lot 8b.

**Fichiers.** Créer `docs/verifications/2026-10-21-lot8-principes.md`. Hors dépôt, créer
`C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot8.md` (relevé brut, même forme que `lot7.md` et `lot10.md`).
Rien d'autre.

**Test et commande bornée.** Aucun code : preuve = rapport commité, `Test-Path` du relevé brut, `NOMS OK`, contrôles
des titres du corpus et des paperboards réels vides.

**Interdits.** Aucun nom de classe, de propriété ni de fichier de l'original dans le dépôt (le nom de l'original de
l'encre révélatrice compris) ; aucun titre, aucune citation, aucune capture du corpus ; aucun nom de paperboard réel ;
observation à l'écran seulement après avoir prévenu Jérôme ; aucun téléchargement (H8-6) ; ne pas modifier ce plan.

**Terminé quand** le rapport contient la table principe → exigence → tâche → scénario (ou raison) pour chaque
constat ; D1 à D16 tranchées ou « non tranchées » avec le défaut du plan ; la table A1 à A30 ; N8 comparé à
N6 + 43 + p6 ; les tailles des modules ; au plus une tâche de principes proposée (`L8-P1`).

Décisions à trancher : **D1** correspondance des numéros de commande et d'action du format vers notre catalogue ;
**D2** valeurs des modes de déplacement, de rotation, de pivot, d'accrochage et de conteneur dans le format ; **D3**
outils qui déclenchent les actions (défaut : sélection seule) ; **D4** objet verrouillé : clic qui le traverse (V28),
action qui part quand même (V103) ; **D5** objet masqué cliquable (V40, V42 ; le clone fait l'inverse) ; **D6** E6 ;
**D7** sens de « bloquer » (arrêt au bord, ou refus du dépôt) ; **D8** pas de rotation par défaut, pivot « première
ligne de texte » ; **D9** liste du glisser-déposer hors vote (18) ; **D10** messages et refus de la diffusion protégée
(V58), commandes de chacune des quatre interdictions (identifiants de la caméra compris : voir « D10 — noms de la caméra » ci-dessous) ; **D11** enregistreur
d'écran (V43), formats ; **D12** cadre web : image fixe, glisser du texte, comportement sous le rideau (N1-A3) ;
**D13** second média à côté du premier (V126) ; **D14** GIF animé (V57, V66) ; **D15** objet Flash : repère seulement
(N1-A15) ; **D16** conteneur recouvert par une image (V39).

**D10 — noms de la caméra (contradiction consignée, non tranchée par ce plan).** Le plan du lot 7 déclare cinq
commandes `tool.camera-area`, `tool.camera-polygon`, `tool.camera-freehand`, `tool.camera-window`,
`tool.camera-screen` (`…-lot7-PLAN.md`, session B, l. 690-694) ; le plan du lot 6 (L6-T3, D6-7) écrit `camera.*`
pour les déclarer restreignables et dans les préfixes de la barre de raccourcis (`lot6-PLAN.md:998-1004,2161`), tout
en disant « identifiants réels relus dans le fichier ». L8-T0 relève les identifiants **réellement déclarés** dans
`actions/data/commands.json` à la tête du chantier et les écrit au rapport. **Recommandation** : retenir ceux du lot 7
(`tool.camera-*`), parce que c'est le lot qui crée ces commandes et que le lot 6 renvoie lui-même au fichier ; la
diffusion protégée (L8-T31, L8-T32) vise alors le préfixe `tool.camera-`, et un correctif `L8-C0` corrige le préfixe
de la barre de raccourcis si L6-T9 a codé `camera.` en dur. La décision revient à l'orchestrateur, sur le rapport de
L8-T0.

**Étapes.**
- [ ] **Étape 1 : corpus.** Lire `C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\README.md` (« Principes
  absents », « Écarts avec la spec », synthèse « Conteneurs ») ; ouvrir chaque fiche citée ; principes en nos mots dans
  le relevé brut.
- [ ] **Étape 2 : spécification et ancien dépôt.** `graphify query "actions conteneurs limiteurs encre magique médias"`
  dans `C:\Users\JeromeDenis\REA - Activinspire` ; le graphe relie mal ces fichiers : lire ensuite
  `app/actcraft/ui/browsers/actions.py`, `app/actcraft/ui/browsers/hooks.py`, `app/actcraft/canvas/items.py`
  (découpe de l'encre, l. 197-221 et 353-410), `app/actcraft/canvas/page.py:114-124`, `app/actcraft/canvas/view.py:228-249,391-400`,
  `app/actcraft/io/flipchart_reader.py:212-221,453-458,557-558,600-652` ; `docs/spec/fonctionnalites.md` (F-255,
  F-292, F-293, F-302, F-550 à F-552, F-577 à F-582, F-680, F-850, F-851, F-890 à F-892) ;
  `docs/re/format-flipchart.md` § 4.2, § 6, § 7.2, § 7.6.
- [ ] **Étape 3 : REA en statique.** Skill `reverse-engineer-anything` sur `C:\Program Files\Activ Software\Inspire\` :
  énumérations de D2, numéros de D1, chaînes des refus (D10), réglages des enregistrements (D11). Serveur
  indisponible : le noter, garder les défauts.
- [ ] **Étape 4 : API.** Dans le worktree : `graphify query "DocumentController liftObjects dropLifted TransformObjectsCommand"`,
  `graphify query "SelectTool ToolHost selection handles rotation"`, `graphify query "PropertyModel DesignMode contactActive"`,
  `graphify query "CommandRegistry lockable restrictable camera"`, puis lecture des en-têtes ; remplir A1 à A30.
- [ ] **Étape 5 : N8.**
  ```powershell
  pwsh -NoProfile -Command "Set-Location 'C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L8-T0'; New-Item -ItemType Directory -Force .tmp | Out-Null; . ./scripts/dev-env.ps1; cmake --preset debug *> .tmp/cfg.txt; ctest --preset debug -N | Select-String 'Total Tests'"
  ```
  Attendu : `Total Tests: <N8>`.
- [ ] **Étape 6 : tailles.** Sans télécharger : `python -m aqt list-qt windows desktop --modules 6.12.0 win64_msvc2022_64`
  et `python -m aqt list-qt windows desktop --archives 6.12.0 win64_msvc2022_64 qtmultimedia qtwebview` ; moteur de
  Qt WebView 6.12 sous Windows (WebView2 du système ou WebEngine) ; tailles et dépendances au rapport.
- [ ] **Étape 7 : rapport** (en-tête, méthode et limites, constats, D1 à D16, A, N8, tailles, `L8-P1`, écarts pour
  l'orchestrateur).
- [ ] **Étape 8 : contrôles et commit.** `git -C <wt> add docs/verifications/2026-10-21-lot8-principes.md` ;
  `NOMS OK` ; contrôles des titres et des paperboards vides ; commit « Lot 8 : principes confrontés, décisions et API
  revérifiées (L8-T0) ».

<a id="tache-l8-t1"></a>
### Tâche L8-T1 — Réservation 8a : tests, fichiers neufs, commandes et disposition du lot, icônes

**Objectif.** Poser en une fois ce que les tâches de 8a se partageraient : enregistrement CMake des fichiers neufs de
8a (créés vides et marqués), des 16 tests unitaires et des 5 tests de captures de 8a (talons `QSKIP`) ; déclaration
dans `commands.json` des commandes des **deux** sous-lots ; leur place dans `layout.json` ; 19 icônes à nous. Après
L8-T1, aucune tâche de 8a ne touche un `CMakeLists.txt`, un `tests/*.cmake` ni `commands.json` (8b : L8-T21).

**Fichiers.** Ceux de la ligne L8-T1. Lire d'abord les `CMakeLists.txt` et `tests/*.cmake` pour reprendre leur forme
exacte (`actcraft_add_module`, `actcraft_add_test(<nom> SOURCES … LIBS …)`, entrées `add_test` des
`tst_captures_*` de `tests/bench.cmake`, bloc `RESOURCES` de `qt_add_qml_module`).

**Test et commande bornée.** Commande ciblée `-R 'tst_actions_layout|tst_actions_registry|tst_actions_lockable|tst_ui_theme'`,
puis preuve de tâche : `100% tests passed … out of N` avec **N = N8 + 21**, `TESTS OK` ; les 21 tests neufs passent
en `SKIP` nommant leur tâche (`ctest --preset debug -V -R 'tst_(core_containers|ui_actionrunner|captures_conteneurs)' | Select-String 'SKIP   :'`
montre `Talon L8 : rempli par L8-T14.`, etc.).

**Preuve visuelle.** `captures.ps1 -Scenario premiers-pas -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "pageIndicator.text=1 / 2"` :
`LU pageIndicator.text=1 / 2 OK`, `CAPTURES OK` (rien ne change à l'écran).

**Interdits.** Aucun comportement : un `.cpp` talon ne contient que le marqueur ; un en-tête talon ne contient que sa
garde, son espace de noms et le marqueur. Aucune icône reprise de l'original ni du clone Python ; aucun libellé de
l'original (N1-12) ; aucun raccourci en double ; aucun identifiant de commande déjà déclaré redéclaré (si
`browser.actions` existe déjà, le garder tel quel).

**Terminé quand** N = N8 + 21, `TESTS OK`, `tst_actions_layout` passe `lot8EntriesDeclared` et `lot8AttributesDeclared`,
`tst_actions_registry` passe `noShortcutTwice`, `tst_actions_lockable` passe `everyCommandDeclaresBoth`, `tst_ui_theme`
passe `iconsAreOurSvgOnA24Grid` avec les 19 nouvelles lignes, `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge** — ajouter à `tests/tst_actions_layout.cpp` :
  ```cpp
  void lot8EntriesDeclared()
  {
      actions::CommandRegistry registry;
      QString error;
      QVERIFY2(registry.loadDeclarations(actions::defaultCommandsJson(), error), qPrintable(error));
      const QStringList ids = {
          QStringLiteral("tool.reveal-ink"),      QStringLiteral("browser.actions"),
          QStringLiteral("action.apply"),         QStringLiteral("action.remove"),
          QStringLiteral("action.pick-target"),   QStringLiteral("link.file"),
          QStringLiteral("link.website"),         QStringLiteral("page.history-back"),
          QStringLiteral("page.history-forward"), QStringLiteral("media.sound-controller"),
          QStringLiteral("record.sound"),         QStringLiteral("record.screen-full"),
          QStringLiteral("record.screen-area"),   QStringLiteral("record.pause"),
          QStringLiteral("record.stop"),          QStringLiteral("record.cancel"),
          QStringLiteral("insert.web-frame"),     QStringLiteral("webframe.back"),
          QStringLiteral("webframe.forward"),     QStringLiteral("webframe.reload"),
          QStringLiteral("webframe.snapshot"),    QStringLiteral("webframe.unblock"),
          QStringLiteral("file.publish"),
      };
      for (const QString& id : ids)
          QVERIFY2(registry.find(id), qPrintable(id));
      QCOMPARE(registry.find(QStringLiteral("tool.reveal-ink"))->shortcut, QStringLiteral("Ctrl+Shift+M"));
      QCOMPARE(registry.find(QStringLiteral("record.sound"))->shortcut, QStringLiteral("Ctrl+Shift+R"));
      QCOMPARE(registry.find(QStringLiteral("record.screen-full"))->platforms,
               actions::Platforms(actions::Platform::Windows));
      QCOMPARE(registry.find(QStringLiteral("record.screen-area"))->platforms,
               actions::Platforms(actions::Platform::Windows));
      QVERIFY(registry.find(QStringLiteral("media.sound-controller"))->checkable);
  }
  void lot8AttributesDeclared()
  {
      actions::CommandRegistry registry;
      QString error;
      QVERIFY(registry.loadDeclarations(actions::defaultCommandsJson(), error));
      // Navigation et écoute restent libres sous le verrou de classe ; diffuser enregistre : restreignable.
      for (const char* id : {"page.history-back", "page.history-forward", "media.sound-controller"})
          QVERIFY2(!registry.find(QString::fromLatin1(id))->lockable, id);
      for (const char* id : {"tool.reveal-ink", "action.apply", "action.remove", "link.file", "link.website",
                             "record.sound", "record.screen-full", "insert.web-frame", "file.publish"})
          QVERIFY2(registry.find(QString::fromLatin1(id))->lockable, id);
      QVERIFY(registry.find(QStringLiteral("file.publish"))->restrictable);
      QVERIFY(!registry.find(QStringLiteral("tool.reveal-ink"))->restrictable);
  }
  ```
- [ ] **Étape 2 : rouge.** Commande ciblée `-R tst_actions_layout` : `FAIL!  : TstActionsLayout::lot8EntriesDeclared`
  (`tool.reveal-ink`).
- [ ] **Étape 3 : commandes.** Ajouter à la fin du tableau `commands` de `actions/data/commands.json`, en un bloc
  contigu « lot 8 », même forme que les entrées existantes (chaque entrée déclare `lockable` et `restrictable`) :
  ```json
  { "id": "tool.reveal-ink", "label": "Encre révélatrice", "icon": "encre-revelatrice", "shortcut": "Ctrl+Shift+M", "checkable": true, "lockable": true, "restrictable": false },
  { "id": "browser.actions", "label": "Navigateur d'actions", "icon": "actions", "checkable": true, "lockable": true, "restrictable": false },
  { "id": "action.apply", "label": "Appliquer l'action", "icon": "action-objet", "lockable": true, "restrictable": false },
  { "id": "action.remove", "label": "Retirer les actions", "icon": "action-objet", "lockable": true, "restrictable": false },
  { "id": "action.pick-target", "label": "Choisir l'objet visé", "icon": "action-objet", "lockable": true, "restrictable": false },
  { "id": "link.file", "label": "Lien vers un fichier…", "icon": "lien-fichier", "shortcut": "Ctrl+L", "argument": "url", "optional": true, "lockable": true, "restrictable": false },
  { "id": "link.website", "label": "Lien vers un site…", "icon": "lien-site", "argument": "text", "optional": true, "lockable": true, "restrictable": false },
  { "id": "page.history-back", "label": "Page vue avant", "icon": "historique-precedent", "lockable": false, "restrictable": false },
  { "id": "page.history-forward", "label": "Page vue après", "icon": "historique-suivant", "lockable": false, "restrictable": false },
  { "id": "media.sound-controller", "label": "Lecteur de sons", "icon": "lecteur-sons", "checkable": true, "lockable": false, "restrictable": false },
  { "id": "record.sound", "label": "Enregistrer du son", "icon": "enregistrer-son", "shortcut": "Ctrl+Shift+R", "lockable": true, "restrictable": false },
  { "id": "record.screen-full", "label": "Enregistrer tout l'écran", "icon": "enregistrer-ecran", "platforms": ["windows"], "lockable": true, "restrictable": false },
  { "id": "record.screen-area", "label": "Enregistrer une zone de l'écran", "icon": "enregistrer-zone", "platforms": ["windows"], "lockable": true, "restrictable": false },
  { "id": "record.pause", "label": "Pause de l'enregistrement", "icon": "enregistrer-son", "lockable": true, "restrictable": false },
  { "id": "record.stop", "label": "Arrêter l'enregistrement", "icon": "enregistrer-son", "lockable": false, "restrictable": false },
  { "id": "record.cancel", "label": "Abandonner l'enregistrement", "icon": "enregistrer-son", "lockable": false, "restrictable": false },
  { "id": "insert.web-frame", "label": "Cadre web…", "icon": "cadre-web", "lockable": true, "restrictable": false },
  { "id": "webframe.back", "label": "Page web précédente", "icon": "cadre-web", "lockable": false, "restrictable": false },
  { "id": "webframe.forward", "label": "Page web suivante", "icon": "cadre-web", "lockable": false, "restrictable": false },
  { "id": "webframe.reload", "label": "Recharger le cadre", "icon": "cadre-web", "lockable": false, "restrictable": false },
  { "id": "webframe.snapshot", "label": "Poser une image du cadre", "icon": "cadre-web", "lockable": true, "restrictable": true },
  { "id": "webframe.unblock", "label": "Débloquer le contenu du cadre", "icon": "cadre-web", "lockable": true, "restrictable": false },
  { "id": "file.publish", "label": "Diffuser en protégé…", "icon": "diffuser", "lockable": true, "restrictable": true }
  ```
  `record.stop` et `record.cancel` restent libres : on doit toujours pouvoir arrêter un enregistrement (N1-A16).
  `webframe.snapshot` est restreignable (copie d'un contenu, D10 de L8-T0). Si `browser.actions` existe déjà
  (L4-T19), retirer sa ligne de ce bloc et ajouter seulement `lockable` et `restrictable` s'ils manquent.
- [ ] **Étape 4 : disposition.** `actions/data/layout.json` : menu Insérer, sous-menu « Lien » (`link.file`,
  `link.website`, `insert.web-frame`) ; menu Outils : `tool.reveal-ink` après le marqueur, sous-menu « Enregistreurs »
  (`record.sound`, `record.screen-full`, `record.screen-area`), `media.sound-controller` ; menu Fichier :
  `file.publish` après « Enregistrer sous » ; palette : `tool.reveal-ink` dans le groupe du stylo ; barre d'objet :
  `action.apply`, `action.remove` ; la sous-ligne de navigation : `page.history-back`, `page.history-forward`.
- [ ] **Étape 5 : icônes.** 19 SVG à nous, grille de 24 px, trait du `Theme` (même gabarit que les icônes du lot 6) ;
  une ligne par icône dans les données de `iconsAreOurSvgOnA24Grid` (`tests/tst_ui_theme.cpp`).
- [ ] **Étape 6 : fichiers talons.** Chaque en-tête talon :
  ```cpp
  #pragma once
  // Talon L8 : rempli par L8-T14.
  namespace actcraft::core {
  } // namespace actcraft::core
  ```
  (espace de noms du module, tâche de la table des talons ci-dessous) ; chaque `.cpp` talon : une ligne
  `// Talon L8 : rempli par L8-T<n>.` ; `actions/data/object-actions.json` talon :
  `{"format": "actcraft-object-actions/1", "description": "Talon L8 : rempli par L8-T6.", "actions": []}` ; chaque
  scénario talon :
  ```json
  {"format": "actcraft-scenario/1", "name": "conteneurs", "description": "Talon L8 : rempli par L8-T16.", "steps": [{"waitReady": true}]}
  ```
  Table des talons : `interactivity.*` → L8-T3 ; `actions/objectactions.*`, `object-actions.json` → L8-T6 ;
  `core/objectactions.h`, `objectactions_geometry.cpp` → L8-T7 ; `objectactions_edit.cpp` → L8-T8 ; `actionedits.*`,
  `actionbrowser.*` → L8-T11 ; `actionrunner.*`, `pagehistory.*` → L8-T9 ; `linklauncher.*` → L8-T10 ;
  `dragdropactions.*` → L8-T13 ; `containers.*` → L8-T14 ; `restrictors.*` → L8-T15 ; `interactionpolicy.h` → L8-T16 ;
  `interactivityprops.*` → L8-T17 ; `revealinktool.*` → L8-T18. Scénarios : `navigateur-actions` → L8-T12,
  `glisser-deposer` → L8-T13, `conteneurs` → L8-T16, `encre-revelatrice` → L8-T18, `interactivite` → L8-T19.
- [ ] **Étape 7 : tests talons.** Pour chaque test (tâche propriétaire = celle de la ligne de la table qui le cite) :
  ```cpp
  #include <QtTest>

  class TstCoreContainers : public QObject
  {
      Q_OBJECT
  private slots:
      void reserved() { QSKIP("Talon L8 : rempli par L8-T14."); }
  };

  QTEST_MAIN(TstCoreContainers)
  #include "tst_core_containers.moc"
  ```
  Enregistrement : `tests/core.cmake` (`tst_core_interactivity`, `tst_core_objectactions`, `tst_core_objectactions_edit`,
  `tst_core_containers`, `tst_core_restrictors`), `tests/formats.cmake` (`tst_formats_interactivity`),
  `tests/flipchart.cmake` (`tst_formats_flipchart_interactivity`, `tst_formats_samples_interactivity`),
  `tests/actions.cmake` (`tst_actions_objectactions`), `tests/tools.cmake` (`tst_tools_interactivity`),
  `tests/render.cmake` (`tst_render_reveal`), `tests/ui.cmake` (`tst_ui_actionrunner`, `tst_ui_linklauncher`,
  `tst_ui_actionbrowser`, `tst_ui_dragdropactions`, `tst_ui_interactivityprops`), `tests/bench.cmake` (les 5
  `tst_captures_*` de 8a, même forme que `tst_captures_premiers_pas`). 16 + 5 = **21**.
- [ ] **Étape 8 : vert.** Commande ciblée, puis preuve de tâche : N = N8 + 21, `TESTS OK` ; `premiers-pas`.
- [ ] **Étape 9 : contrôles et commit.** `git -C <wt> add` des fichiers de la ligne (nommés, jamais `-A`) ; `NOMS OK` ;
  contrôle des titres vide ; commit « Lot 8 : réservation de 8a, commandes du lot, icônes (L8-T1) ».

<a id="tache-l8-t2"></a>
### Tâche L8-T2 — Coquille QML du lot

**Objectif.** Poser les emplacements QML des deux sous-lots et un gestionnaire **talon** par commande du lot, chacun
dans le fichier où sa tâche propriétaire écrira le vrai (règle du lot 6, amendement 1, point 4) ; rendre l'onglet
`actions` du panneau actif (il était grisé depuis le lot 6, A15).

**Fichiers.** `ui/qml/Main.qml` (instances : `ActionRunner { id: appActionRunner; objectName: "appActionRunner"; commands: appCommands; contactActive: pageArea.view.contactActive }`,
`LinkLauncher { id: appLinks; objectName: "appLinks" }`, `ActionBrowserModel { id: appActionBrowser; objectName: "appActionBrowser"; commands: appCommands; contactActive: pageArea.view.contactActive }`,
`DragDropActions { id: appDragDrop; objectName: "appDragDrop"; commands: appCommands }` — types déclarés en talon
par L8-T1 dans `ui/`, enregistrés en QML par `QML_ELEMENT` dans leur tâche ; tant que le type n'existe pas, l'instance
est un `QtObject { objectName: "appActionRunner" }` marqué talon), `ui/qml/BrowserPanel.qml` (onglet `actions` actif,
contenu `ActionBrowser`), `ui/qml/PageArea.qml` (dans `pageOverlays`, sous le rideau : `ActionOverlays { z: 5 }`,
`MediaLayer { z: 4 }`, `WebFrameLayer { z: 3 }`), les 12 fichiers QML neufs de la ligne (racine `Item` visible
faux, `objectName` = nom du fichier en minuscule initiale, marqueur `// Talon L8 : rempli par L8-T<n>.`),
`tests/tst_ui_shell.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_shell` ; fonctions `lot8ShellPlaceholders`,
`everyLot8CommandHasAHandler`, `actionsTabEnabled` en `PASS`.

**Preuve visuelle.** `premiers-pas` hors écran : `CAPTURES OK` ; banc hors écran `content_check.ok = True` (les
surcouches vides ne changent aucun pixel).

**Interdits.** Dans le QML, la vue est `pageArea.view` (jamais `pageView.`) ; aucune surcouche dans le cache ; aucun
comportement (les gestionnaires talons ne font que `console.info("Talon L8 : rempli par L8-T<n>.")`).

**Terminé quand** les trois fonctions passent et `tst_ui_shell` reste vert en entier.

**Étapes.**
- [ ] **Étape 1 : test rouge** dans `tests/tst_ui_shell.cpp` (même montage `Shell`, `start`, réglages d'essai du lot 6,
  amendement 3, N1) :
  ```cpp
  void lot8ShellPlaceholders()
  {
      Shell shell;
      QVERIFY(shell.start());
      for (const char* name : {"appActionRunner", "appLinks", "appActionBrowser", "appDragDrop", "actionOverlays",
                               "mediaLayer", "webFrameLayer", "soundController", "recorderPalette",
                               "recordingIndicator", "linkDialog", "linkConfirmDialog", "webFrameDialog",
                               "publishDialog", "actionBrowser", "dragDropActions"})
          QVERIFY2(shell.find(QString::fromLatin1(name)), name);
  }
  void everyLot8CommandHasAHandler()
  {
      Shell shell;
      QVERIFY(shell.start());
      for (const char* id : {"tool.reveal-ink", "browser.actions", "action.apply", "action.remove",
                             "action.pick-target", "link.file", "link.website", "page.history-back",
                             "page.history-forward", "media.sound-controller", "record.sound", "record.pause",
                             "record.stop", "record.cancel", "insert.web-frame", "webframe.back",
                             "webframe.forward", "webframe.reload", "webframe.snapshot", "webframe.unblock",
                             "file.publish"})
          QVERIFY2(shell.commands()->registry().hasHandler(QString::fromLatin1(id)), id);
  #ifdef Q_OS_WIN
      QVERIFY(shell.commands()->registry().hasHandler(QStringLiteral("record.screen-full")));
      QVERIFY(shell.commands()->registry().hasHandler(QStringLiteral("record.screen-area")));
  #endif
  }
  void actionsTabEnabled()
  {
      Shell shell;
      QVERIFY(shell.start());
      QObject* panel = shell.find(QStringLiteral("browserPanel"));
      QVERIFY(panel);
      QVERIFY(QMetaObject::invokeMethod(panel, "showTab", Q_ARG(QVariant, QStringLiteral("actions"))));
      QCOMPARE(panel->property("current").toString(), QStringLiteral("actions"));
      QVERIFY(shell.find(QStringLiteral("actionBrowser"))->property("enabled").toBool());
  }
  ```
  (`Shell::find`, `Shell::commands()` : aides du test existant, A7 ; `showTab` : fonction de `BrowserPanel.qml` posée
  par L4-T19, A15 — si son nom diffère, suivre le nom réel relevé par L8-T0.)
- [ ] **Étape 2 : rouge** : `FAIL!  : TstUiShell::lot8ShellPlaceholders()` (`appActionRunner`).
- [ ] **Étape 3 : QML.** Écrire les instances, l'onglet, les trois surcouches dans `pageOverlays` et les 12 fichiers ;
  dans chaque fichier propriétaire, `Component.onCompleted: appCommands.setHandler("<id>", function(a) { console.info("Talon L8 : rempli par L8-T<n>.") })`
  pour ses commandes (table : `tool.reveal-ink` → `ActionOverlays.qml` puis L8-T18 ; `browser.actions`,
  `action.*` → `ActionBrowser.qml` ; `page.history-*` → `ActionOverlays.qml` ; `link.*` → `LinkDialog.qml` ;
  `media.sound-controller` → `SoundController.qml` ; `record.*` → `RecorderPalette.qml` ; `insert.web-frame`,
  `webframe.*` → `WebFrameLayer.qml` ; `file.publish` → `PublishDialog.qml`).
- [ ] **Étape 4 : vert** : commande ciblée ; `premiers-pas` ; banc hors écran.
- [ ] **Étape 5 : contrôles et commit** « Lot 8 : coquille QML du lot (L8-T2) » ; `NOMS OK` ; titres vides ;
  `git -C <wt> grep -n "pageView\." -- ui/qml` vide.

<a id="tache-l8-t3"></a>
### Tâche L8-T3 — `core` : modèle de l'interactivité

**Objectif.** Ajouter au modèle, en une fois pour les deux sous-lots, les champs de l'interactivité, avec trois
invariants : (1) un document sans ces champs garde **exactement** son empreinte ; (2) les booléens du lot 2
(`movable`, `rotatable`) et les modes neufs disent la même chose ; (3) `UnknownObject` reste le dernier type. Le
patron évident est le mauvais : remplacer les booléens par les modes casserait la lecture des `.flipchart` et les
outils du lot 3 qui lisent `movable` ; ajouter le cadre web après `UnknownObject` casserait `kindOf`.

**Fichiers.** `core/include/actcraft/core/model.h`, `core/src/model.cpp`, `core/include/actcraft/core/interactivity.h`,
`core/src/interactivity.cpp`, `tests/tst_core_interactivity.cpp`, `tests/tst_core_model.cpp`. Si
`tests/support/richdocument.h` déstructure `ObjectCommon` ou `Action` (A25 : `richDocumentFillsEveryField`), il est
ajouté à la tâche par l'orchestrateur avant le lancement.

**Test et commande bornée.** Commande ciblée `-R 'tst_core_interactivity|tst_core_model|tst_formats_nativejson|tst_formats_nativefile'`
; fonctions de la ligne en `PASS` ; mutation : retirer l'appel à `normalize` dans `setTravel` → `fixedTravelMeansNotMovable`
rouge, remise par `git checkout -- core/src/interactivity.cpp`.

**Interdits.** QtCore seulement (`core/`) ; aucun nom composé de l'original (ni les noms de propriétés de déplacement, d'accrochage, de type d'encre ou
d'options de média du format : les noms de ce plan sont à nous) ; aucun `default:` dans un `switch` sur `ObjectKind` (`/we4062`) ; ne pas toucher à
`formats/` (L8-T4) ni au rendu.

**Terminé quand** les fonctions passent, les tests du format natif restent verts sans changer leurs fichiers de
référence (`writerMatchesTheReference`), et `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_core_interactivity.cpp` (remplace le talon) :
  ```cpp
  #include <actcraft/core/interactivity.h>
  #include <actcraft/core/model.h>

  #include <QtTest>

  using namespace actcraft::core;

  class TstCoreInteractivity : public QObject
  {
      Q_OBJECT
      static Document oneShape()
      {
          Document d = Document::withOnePage(1024, 768);
          PageObject o{Shape{}};
          o.common.id = 7;
          d.pages[0].objects.push_back(o);
          return d;
      }
  private slots:
      void defaultsChangeNoFingerprint()
      {
          const Document d = oneShape();
          const auto before = fingerprint(d);
          Document e = d;
          e.pages[0].objects[0].common.container = ContainerRule{};
          e.pages[0].objects[0].common.restrictions = Restrictions{};
          e.broadcast = BroadcastRules{};
          QCOMPARE(fingerprint(e), before);
          e.pages[0].objects[0].common.container.accepts = Accepts::Anything;
          QVERIFY(fingerprint(e) != before);
          e.pages[0].objects[0].common.container.accepts = Accepts::Nothing;
          QCOMPARE(fingerprint(e), before);
      }
      void fixedTravelMeansNotMovable()
      {
          Restrictions r;
          setTravel(r, Travel::Fixed);
          QVERIFY(!r.movable);
          setTravel(r, Travel::Vertical);
          QVERIFY(r.movable);
          Restrictions fromFile;
          fromFile.movable = false; // lu d'un fichier du lot 2
          normalize(fromFile);
          QCOMPARE(fromFile.travel, Travel::Fixed);
      }
      void fixedTurningMeansNotRotatable()
      {
          Restrictions r;
          setTurning(r, Turning::Fixed);
          QVERIFY(!r.rotatable);
          setTurning(r, Turning::Clockwise);
          QVERIFY(r.rotatable);
          Restrictions fromFile;
          fromFile.rotatable = false;
          normalize(fromFile);
          QCOMPARE(fromFile.turning, Turning::Fixed);
      }
      void webFrameKindBeforeUnknown()
      {
          QCOMPARE(static_cast<int>(ObjectKind::WebFrame), 9);
          QCOMPARE(static_cast<int>(ObjectKind::Unknown), 10);
          QCOMPARE(kObjectKindCount, std::size_t(11));
          PageObject w{WebFrame{}};
          QCOMPARE(kindOf(w), ObjectKind::WebFrame);
          PageObject u{UnknownObject{}};
          QCOMPARE(kindOf(u), ObjectKind::Unknown);
      }
      void actionKeepsFlipchartFields()
      {
          Action a;
          a.kind = ActionKind::OnObject;
          a.sourceType = 28;
          a.command = 184;
          a.target = 9;
          QVERIFY(a.catalogId.empty());
          QVERIFY(!isSupported(a));
          a.catalogId = "object.toggle-hidden";
          QVERIFY(isSupported(a));
          QCOMPARE(a.command, 184); // jamais réécrit
      }
      void broadcastDefaultsAllowEverything()
      {
          const BroadcastRules b;
          QVERIFY(b.isDefault());
          QVERIFY(!b.noCopy && !b.noSave && !b.noPrint && !b.noCamera);
          QVERIFY(b.author.empty());
      }
  };

  QTEST_MAIN(TstCoreInteractivity)
  #include "tst_core_interactivity.moc"
  ```
  (`Document::withOnePage` et `fingerprint` : fonctions existantes de `model.h` ; si leurs noms diffèrent, suivre
  ceux de `tst_core_model.cpp`.)
- [ ] **Étape 2 : rouge** : erreur de compilation `ContainerRule` inconnu.
- [ ] **Étape 3 : modèle** (`model.h`) — dans `Restrictions`, après les trois booléens :
  ```cpp
  // Lot 8 (F-578, F-579, D8-7). Les booléens restent la source du « bloqué » : interactivity.h les tient d'accord.
  Travel travel = Travel::Free;
  ObjectId track = 0;            // objet trait ou ligne suivi (Travel::AlongTrack)
  Turning turning = Turning::Free;
  double turnStep = 0;           // degrés ; 0 : continu
  Mark pivot = Mark::Center;
  PointF pivotPoint;             // Mark::Point, coordonnées de l'objet
  ObjectId pivotObject = 0;      // Mark::OtherObject
  bool blocksOthers = false;
  bool magnet = false;           // l'objet se pose sur `magnetMark` du dernier conteneur ou de `magnetPoint`
  Mark magnetMark = Mark::Center;
  PointF magnetPoint;
  ```
  avant `Restrictions`, les énumérations `Travel { Free, Vertical, Horizontal, AlongTrack, Fixed }`,
  `Turning { Free, Clockwise, CounterClockwise, Fixed }`, `Mark { Center, TopLeft, Top, TopRight, Right, BottomRight,
  Bottom, BottomLeft, Left, FirstTextLine, Point, OtherObject }` (`std::uint8_t`, valeurs dans cet ordre à partir de
  0). Après `Restrictions` :
  ```cpp
  enum class Accepts : std::uint8_t { Nothing = 0, Anything = 1, OneObject = 2, Keywords = 3 };
  enum class FitRule : std::uint8_t { CentresMatch = 0, FullyInside = 1 };
  // Conteneur (F-577, F-580, D8-6).
  struct ContainerRule
  {
      Accepts accepts = Accepts::Nothing;
      ObjectId object = 0;               // Accepts::OneObject
      std::vector<std::string> keywords; // Accepts::Keywords
      FitRule fit = FitRule::CentresMatch;
      double tolerance = 10;             // pixels de page (centres)
      bool bounceBack = false;           // retour si non contenu
      bool chime = false;                // son de confirmation
      std::string chimeSound;            // clé de ressource du son
  };
  ```
  `Action` gagne `std::string catalogId; std::string parameters; bool dragDrop = false;` ; `ObjectCommon` gagne
  `ContainerRule container;` ; `Media` gagne `bool controlsHidden = false; bool aspectLocked = false; bool
  clearBackground = false; bool wrapText = false;` ; nouveau type, avant `UnknownObject` :
  ```cpp
  // Cadre web (F-302, S27) : code HTML saisi, ou adresse ; jamais chargé sans la politique de L8-T29.
  struct WebFrame
  {
      RectF bounds;
      std::string html;
      std::string address;
  };
  ```
  `ObjectKind::WebFrame = 9`, `Unknown = 10`, `kObjectKindCount = 11`, `WebFrame` dans `ObjectVariant` juste avant
  `UnknownObject` ; `Document` gagne :
  ```cpp
  // Diffusion protégée (F-680, S18) : courtoisie sur un format ouvert (N1-A4).
  struct BroadcastRules
  {
      bool noCopy = false;
      bool noSave = false;
      bool noPrint = false;
      bool noCamera = false;
      std::string author; // informations de l'auteur, jamais affichées aux utilisateurs
      bool isDefault() const noexcept { return !noCopy && !noSave && !noPrint && !noCamera && author.empty(); }
  };
  ```
  et le membre `BroadcastRules broadcast;`.
- [ ] **Étape 4 : `interactivity.h` / `.cpp`** :
  ```cpp
  #pragma once
  #include <actcraft/core/model.h>

  namespace actcraft::core {
  void setTravel(Restrictions& r, Travel t) noexcept;   // movable = t != Fixed
  void setTurning(Restrictions& r, Turning t) noexcept; // rotatable = t != Fixed
  void normalize(Restrictions& r) noexcept;             // booléen faux → mode Fixed ; mode Fixed → booléen faux
  bool isDefault(const Restrictions& r) noexcept;       // ne regarde que les champs du lot 8
  bool isDefault(const ContainerRule& c) noexcept;
  bool isSupported(const Action& a) noexcept;           // catalogId non vide
  } // namespace actcraft::core
  ```
  Corps : affectations directes ; `isDefault(Restrictions)` compare chaque champ du lot 8 à `Restrictions{}`.
- [ ] **Étape 5 : empreinte** (`model.cpp`) : `hashType(Fnv1a&, const WebFrame&)` (bounds, html, address) ; pour un
  objet, un bloc ouvert par l'octet `0xB8` en **fin** de flux de l'objet, seulement si `!isDefault(restrictions)`,
  `!isDefault(container)`, une action a `catalogId`, `parameters` ou `dragDrop`, ou une option de média du lot 8 est
  vraie ; pour le document, un bloc `0xB9` en fin de flux seulement si `!broadcast.isDefault()` (même patron que
  `PagePresentation`, octet `0xA7`, `model.cpp:410-415`). `static_assert` du variant mis à jour ; `kindOf` suit.
- [ ] **Étape 6 : `tst_core_model`** : ajouter `lot8FieldsKeepOldFingerprints` (l'empreinte du document riche de
  référence, calculée avant et après avoir posé tous les champs du lot 8 à leur défaut, est identique) ;
  `unknownObjectIsTheLastKind` mis à jour (11 types).
- [ ] **Étape 7 : vert**, mutation rouge puis remise ; preuve de tâche.
- [ ] **Étape 8 : contrôles et commit** « Lot 8 : modèle de l'interactivité (L8-T3) » ; `NOMS OK`.

<a id="tache-l8-t4"></a>
### Tâche L8-T4 — `formats` : format natif des champs de l'interactivité

**Objectif.** Écrire et relire sans perte, dans `actcraft-document/1`, les champs de L8-T3 ; valeurs par défaut
omises ; une clé omise retirée des inconnues à la lecture (règle de L4-T6 et L6-T10, A25) ; fichier plus ancien lu
avec les défauts ; fichier plus récent : clés inconnues d'une action gardées.

**Fichiers.** `formats/src/nativejson.cpp`, `tests/tst_formats_interactivity.cpp`.

**Test et commande bornée.** Commande ciblée `-R 'tst_formats_interactivity|tst_formats_nativejson|tst_formats_nativefile'`.
Mutation : ne plus omettre `container` à sa valeur par défaut → `defaultsAreOmitted` rouge, remise par
`git checkout -- formats/src/nativejson.cpp`.

**Interdits.** QtCore seulement ; clés en anglais, en `kebab-case` comme au lot 6 (`catalog-id`, `drag-drop`,
`bounce-back`, `blocks-others`, `turn-step`, `controls-hidden`, `aspect-locked`, `clear-background`, `wrap-text`,
`no-copy`, `no-save`, `no-print`, `no-camera`) ; type d'objet `"web-frame"` ; ne pas régénérer le fichier de référence
`tests/data/nativejson/richdocument.json` (il ne contient aucun champ du lot 8 : il doit rester identique octet pour
octet).

**Terminé quand** les neuf fonctions passent et `tst_formats_nativejson` reste vert sans changer de référence.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_formats_interactivity.cpp` :
  ```cpp
  #include <actcraft/core/interactivity.h>
  #include <actcraft/formats/nativejson.h>

  #include <QJsonDocument>
  #include <QJsonObject>
  #include <QtTest>

  using namespace actcraft;

  class TstFormatsInteractivity : public QObject
  {
      Q_OBJECT
      static core::Document base()
      {
          core::Document d = core::Document::withOnePage(1024, 768);
          core::PageObject o{core::Shape{}};
          o.common.id = 1;
          d.pages[0].objects.push_back(o);
          return d;
      }
      static core::Document roundTrip(const core::Document& d)
      {
          QString error;
          const QByteArray json = formats::writeDocumentJson(d);
          core::Document back;
          [&] { QVERIFY2(formats::readDocumentJson(json, back, error), qPrintable(error)); }();
          return back;
      }
  private slots:
      void actionRoundTrip()
      {
          core::Document d = base();
          core::Action a;
          a.catalogId = "page.goto";
          a.parameters = R"({"page":3})";
          a.dragDrop = true;
          d.pages[0].objects[0].common.actions.push_back(a);
          const auto back = roundTrip(d);
          QCOMPARE(core::fingerprint(back), core::fingerprint(d));
          QCOMPARE(back.pages[0].objects[0].common.actions[0].catalogId, std::string("page.goto"));
      }
      void containerRoundTrip()
      {
          core::Document d = base();
          auto& c = d.pages[0].objects[0].common.container;
          c.accepts = core::Accepts::Keywords;
          c.keywords = {"fruit"};
          c.fit = core::FitRule::FullyInside;
          c.bounceBack = true;
          QCOMPARE(core::fingerprint(roundTrip(d)), core::fingerprint(d));
      }
      void restrictorsRoundTrip()
      {
          core::Document d = base();
          auto& r = d.pages[0].objects[0].common.restrictions;
          core::setTravel(r, core::Travel::AlongTrack);
          r.track = 4;
          core::setTurning(r, core::Turning::Clockwise);
          r.turnStep = 45;
          r.blocksOthers = true;
          QCOMPARE(core::fingerprint(roundTrip(d)), core::fingerprint(d));
      }
      void mediaOptionsRoundTrip()
      {
          core::Document d = base();
          core::Media m;
          m.controlsHidden = true;
          m.aspectLocked = true;
          m.clearBackground = true;
          core::PageObject o{m};
          o.common.id = 2;
          d.pages[0].objects.push_back(o);
          QCOMPARE(core::fingerprint(roundTrip(d)), core::fingerprint(d));
      }
      void webFrameRoundTrip()
      {
          core::Document d = base();
          core::WebFrame w;
          w.bounds = {10, 10, 300, 200};
          w.html = "<p>essai</p>";
          core::PageObject o{w};
          o.common.id = 3;
          d.pages[0].objects.push_back(o);
          const auto back = roundTrip(d);
          QCOMPARE(core::kindOf(back.pages[0].objects[1]), core::ObjectKind::WebFrame);
          QCOMPARE(core::fingerprint(back), core::fingerprint(d));
      }
      void broadcastRoundTrip()
      {
          core::Document d = base();
          d.broadcast.noCopy = true;
          d.broadcast.noPrint = true;
          QCOMPARE(core::fingerprint(roundTrip(d)), core::fingerprint(d));
      }
      void defaultsAreOmitted()
      {
          const QByteArray json = formats::writeDocumentJson(base());
          for (const char* key : {"container", "catalog-id", "drag-drop", "travel", "turning", "blocks-others",
                                  "broadcast", "controls-hidden"})
              QVERIFY2(!json.contains(key), key);
      }
      void olderFileReadsWithDefaults()
      {
          const QByteArray json = formats::writeDocumentJson(base()); // forme du lot 2 : aucun champ du lot 8
          core::Document back;
          QString error;
          QVERIFY(formats::readDocumentJson(json, back, error));
          QVERIFY(core::isDefault(back.pages[0].objects[0].common.container));
          QVERIFY(back.broadcast.isDefault());
          QVERIFY(back.pages[0].objects[0].unknownKeys.empty());
      }
      void unknownActionKeyKept()
      {
          core::Document d = base();
          core::Action a;
          a.catalogId = "page.next";
          d.pages[0].objects[0].common.actions.push_back(a);
          QByteArray json = formats::writeDocumentJson(d);
          json.replace("\"catalog-id\"", "\"future-key\":42,\"catalog-id\"");
          core::Document back;
          QString error;
          QVERIFY(formats::readDocumentJson(json, back, error));
          QVERIFY(formats::writeDocumentJson(back).contains("\"future-key\":42"));
      }
  };

  QTEST_MAIN(TstFormatsInteractivity)
  #include "tst_formats_interactivity.moc"
  ```
  (`writeDocumentJson` / `readDocumentJson` : noms publics du format natif relevés dans `formats/nativejson.h` ; si
  différents, suivre ceux de `tst_formats_nativejson.cpp`.)
- [ ] **Étape 2 : rouge** : `actionRoundTrip` échoue (empreintes différentes : `catalog-id` non écrit).
- [ ] **Étape 3 : écriture.** `actionJson` (`nativejson.cpp:517-526`) : `catalog-id`, `parameters` (objet JSON
  inséré tel quel s'il se lit, sinon chaîne), `drag-drop` si vrais ou non vides ; `writeCommon` : `restrictions`
  gagne `travel`, `track`, `turning`, `turn-step`, `pivot`, `pivot-point`, `pivot-object`, `blocks-others`,
  `magnet`, `magnet-mark`, `magnet-point` hors défaut (noms d'énumération en `kebab-case` : `free`, `vertical`,
  `horizontal`, `along-track`, `fixed`, `clockwise`, `counter-clockwise`, marques `center`, `top-left`…,
  `first-text-line`, `point`, `other-object`) ; `container` (objet) si `!isDefault` ; média : quatre booléens si
  vrais ; `"web-frame"` : `bounds`, `html`, `address` ; document : `broadcast` si `!isDefault()`.
- [ ] **Étape 4 : lecture** symétrique, `normalize(restrictions)` après lecture, clés omises retirées des inconnues
  (gabarits `KnownKeys` : ajouter les clés du lot 8 au gabarit d'objet et d'action, puisqu'un objet vierge ne les
  écrit pas).
- [ ] **Étape 5 : vert**, mutation rouge puis remise ; preuve de tâche.
- [ ] **Étape 6 : commit** « Lot 8 : format natif de l'interactivité (L8-T4) » ; `NOMS OK`.

<a id="tache-l8-t5"></a>
### Tâche L8-T5 — `formats` : lecteur `.flipchart` de l'interactivité

**Objectif.** Le lecteur du lot 2 garde les actions et trois booléens ; il gagne : (a) une table **écrite à la main**
des numéros de commande et d'action du format vers nos identifiants (`catalogId`), à partir des numéros relevés par
L8-T0 (D1) — au minimum 184 → `object.toggle-hidden`, l'action de navigation vers `page.goto` (avec `{"page":n}`),
l'ouverture externe (270) vers `document.open-file` ; (b) les modes de déplacement, de rotation, le pivot,
l'accrochage, le blocage et le conteneur (valeurs de D2) ; (c) le trait de type d'encre 2 en `StrokeKind::Revealer`
(déjà lu, `flipchart.cpp:821-822` : à vérifier) ; (d) les options de média (propriétés de l'objet média, D2).

**Fichiers.** `formats/src/flipchart.cpp`, `formats/src/flipchartids.h`, `tests/support/flipchartwriter.h`,
`tests/tst_formats_flipchart_interactivity.cpp`.

**Test et commande bornée.** Commande ciblée `-R 'tst_formats_flipchart|tst_formats_oracle_synth'` (le motif prend
aussi `tst_formats_flipchart_interactivity`).

**Interdits.** Aucun nom de l'original comme identifiant : les constantes de `flipchartids.h` gardent la forme
existante (nombres et noms à nous, par exemple `kTravelProperty`, `kContainerAccepts`) ; aucune table copiée de
`_schema.py` (générée depuis le binaire) ; aucun paperboard réel dans un test (documents écrits par
`flipchartwriter.h`) ; une action dont le numéro n'est pas dans la table garde `catalogId` vide, sans avertissement
bloquant.

**Terminé quand** les sept fonctions passent, `tst_formats_flipchart` et `tst_formats_oracle_synth` restent verts,
`NOMS OK`, contrôle des paperboards vide.

**Étapes.**
- [ ] **Étape 1 : écrivain.** Étendre `tests/support/flipchartwriter.h` (lot 7) : propriétés communes numérotées
  13 à 31, 37, 44, 45 (valeurs de D2) et nœud d'action frère avec type, cible et numéro de commande — mêmes aides que
  celles du lot 7 (`mathmlObjectBecomesEquation`).
- [ ] **Étape 2 : test rouge** `tests/tst_formats_flipchart_interactivity.cpp` : fonctions
  `toggleHiddenMapsToOurId` (un rond suivi d'une action sur objet de commande 184 visant une étiquette : l'action du
  rond a `catalogId == "object.toggle-hidden"` et `target` = identifiant de l'étiquette), `unknownCommandKeptWithoutId`
  (commande 9999 : action gardée, `catalogId` vide, `command == 9999`), `travelModesKept` (mode « vertical » lu :
  `travel == Travel::Vertical`, `movable` vrai ; mode « non » : `Travel::Fixed`, `movable` faux),
  `containerRead` (« mots-clés », mot « fruit », retour vrai), `revealInkRead` (type d'encre 2 :
  `StrokeKind::Revealer`, couche du dessus), `mediaOptionsRead` (commandes masquées, aspect gardé),
  `noNameOfTheOriginal` (lit `formats/src/flipchartids.h` depuis `ACTCRAFT_SOURCE_DIR` et vérifie qu'aucune ligne
  ajoutée par ce lot ne contient un identifiant au préfixe `As` ou `kAs` : `QRegularExpression("\\bk?As[A-Z]")`
  ne trouve rien). Chaque test construit le fichier par l'écrivain, le lit par `formats::readFlipchart` (A23) et
  compare les champs.
- [ ] **Étape 3 : rouge** : `toggleHiddenMapsToOurId` échoue (`catalogId` vide).
- [ ] **Étape 4 : table et lecture.** Dans `flipchartids.h`, la table `kCommandToCatalog` (paires nombre → chaîne) ;
  dans `actionFrom` (`flipchart.cpp:667-719`), `catalogId` et `parameters` remplis ; dans la lecture des communs
  (`flipchart.cpp:998-1002`), modes et conteneur, puis `core::normalize(restrictions)`.
- [ ] **Étape 5 : vert** ; `tst_formats_flipchart` et `tst_formats_oracle_synth` verts ; preuve de tâche.
- [ ] **Étape 6 : contrôles et commit** « Lot 8 : lecteur .flipchart de l'interactivité (L8-T5) » ; `NOMS OK` ;
  contrôle des paperboards vide.

<a id="tache-l8-t6"></a>
### Tâche L8-T6 — `actions` : catalogue des actions d'objet

**Objectif.** Écrire le catalogue à nous (D8-1) et son chargeur : quatre catégories (`command`, `page`, `object`,
`document`), libellés en français à nous, paramètres typés, drapeau `drag-drop` des entrées de l'onglet F-892 ;
aucune action de vote. Une entrée de catégorie `command` (et toute entrée qui délègue à une commande) nomme une
commande **déclarée** du registre ; une commande absente du registre (services en ligne de Promethean, vote) n'a pas
d'entrée (H8-1).

**Fichiers.** `actions/data/object-actions.json`, `actions/include/actcraft/actions/objectactions.h`,
`actions/src/objectactions.cpp`, `tests/tst_actions_objectactions.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_actions_objectactions`.

**Interdits.** Aucun libellé de l'original recopié (N1-12 : « Encre révélatrice », jamais le nom de l'original) ;
QtCore seulement ; pas de QML.

**Terminé quand** les six fonctions passent.

**Étapes.**
- [ ] **Étape 1 : en-tête** `objectactions.h` (remplace le talon) :
  ```cpp
  #pragma once
  #include <QByteArray>
  #include <QList>
  #include <QString>

  namespace actcraft::actions {
  enum class ActionCategory { Command, Page, Object, Document };
  struct ActionParameter
  {
      QString name;     // clé dans l'objet JSON des paramètres de core::Action
      QString type;     // "number", "angle", "text", "url", "file", "page", "translucency"
      double minimum = 0;
      double maximum = 0; // 0 : sans borne
  };
  struct ObjectActionInfo
  {
      QString id;       // identifiant à nous, porté par core::Action::catalogId
      ActionCategory category = ActionCategory::Command;
      QString label;
      QString icon;
      QString command;  // commande du registre déclenchée ; vide : exécutée par le moteur de core/ (L8-T7, L8-T8)
      bool target = false;   // vise un objet (choisi par le sélecteur ; défaut : l'objet cliqué)
      bool dragDrop = false; // proposée dans l'onglet glisser-déposer (F-892)
      QList<ActionParameter> parameters;
  };
  class ObjectActionCatalog
  {
  public:
      bool load(const QByteArray& json, QString& error);
      const ObjectActionInfo* find(const QString& id) const;
      QList<const ObjectActionInfo*> inCategory(ActionCategory category) const;
      QList<const ObjectActionInfo*> dragDropList() const;
      qsizetype size() const noexcept { return m_actions.size(); }
  private:
      QList<ObjectActionInfo> m_actions;
  };
  QByteArray defaultObjectActionsJson(); // actions/data/object-actions.json, ressource intégrée
  } // namespace actcraft::actions
  ```
- [ ] **Étape 2 : test rouge** `tests/tst_actions_objectactions.cpp` :
  ```cpp
  #include <actcraft/actions/commandregistry.h>
  #include <actcraft/actions/objectactions.h>

  #include <QSet>
  #include <QtTest>

  using namespace actcraft::actions;

  class TstActionsObjectActions : public QObject
  {
      Q_OBJECT
      ObjectActionCatalog m_catalog;
      CommandRegistry m_registry;
  private slots:
      void initTestCase()
      {
          QString error;
          QVERIFY2(m_catalog.load(defaultObjectActionsJson(), error), qPrintable(error));
          QVERIFY2(m_registry.loadDeclarations(defaultCommandsJson(), error), qPrintable(error));
      }
      void fourCategoriesNoVote()
      {
          for (auto c : {ActionCategory::Command, ActionCategory::Page, ActionCategory::Object, ActionCategory::Document})
              QVERIFY(!m_catalog.inCategory(c).isEmpty());
          for (const auto* a : m_catalog.inCategory(ActionCategory::Command))
              QVERIFY2(!a->id.contains(QStringLiteral("vote")) && !a->id.contains(QStringLiteral("poll")),
                       qPrintable(a->id));
      }
      void everyCommandActionNamesARegisteredCommand()
      {
          for (auto c : {ActionCategory::Command, ActionCategory::Page, ActionCategory::Object, ActionCategory::Document})
              for (const auto* a : m_catalog.inCategory(c))
                  if (!a->command.isEmpty())
                      QVERIFY2(m_registry.find(a->command), qPrintable(a->id + QStringLiteral(" → ") + a->command));
      }
      void parametersAreTyped()
      {
          const QSet<QString> types = {QStringLiteral("number"), QStringLiteral("angle"), QStringLiteral("text"),
                                       QStringLiteral("url"),    QStringLiteral("file"),  QStringLiteral("page"),
                                       QStringLiteral("translucency")};
          const auto* gotoPage = m_catalog.find(QStringLiteral("page.goto"));
          QVERIFY(gotoPage);
          QCOMPARE(gotoPage->parameters.size(), 1);
          QCOMPARE(gotoPage->parameters[0].type, QStringLiteral("page"));
          for (auto c : {ActionCategory::Command, ActionCategory::Page, ActionCategory::Object, ActionCategory::Document})
              for (const auto* a : m_catalog.inCategory(c))
                  for (const auto& p : a->parameters)
                      QVERIFY2(types.contains(p.type), qPrintable(a->id));
      }
      void dragDropListHasEighteen()
      {
          QCOMPARE(m_catalog.dragDropList().size(), 18); // F-892 sans ses deux entrées de vote (D9)
          QVERIFY(m_catalog.find(QStringLiteral("command.tool.pen"))->dragDrop);
      }
      void labelsAreOurs()
      {
          QVERIFY(m_catalog.find(QStringLiteral("command.tool.reveal-ink")));
          QCOMPARE(m_catalog.find(QStringLiteral("command.tool.reveal-ink"))->label, QStringLiteral("Encre révélatrice"));
          QVERIFY(!m_catalog.find(QStringLiteral("command.planet")));
      }
      void idsAreUnique()
      {
          QSet<QString> seen;
          for (auto c : {ActionCategory::Command, ActionCategory::Page, ActionCategory::Object, ActionCategory::Document})
              for (const auto* a : m_catalog.inCategory(c))
                  QVERIFY2(!seen.contains(a->id), qPrintable(a->id)), seen.insert(a->id);
      }
  };

  QTEST_MAIN(TstActionsObjectActions)
  #include "tst_actions_objectactions.moc"
  ```
- [ ] **Étape 3 : rouge** : `fourCategoriesNoVote` échoue (catalogue talon vide).
- [ ] **Étape 4 : données.** `object-actions.json` (`"format": "actcraft-object-actions/1"`). Forme d'une entrée :
  ```json
  { "id": "page.goto", "category": "page", "label": "Aller à la page", "icon": "action-objet", "command": "page.goto", "parameters": [ { "name": "page", "type": "page", "minimum": 1 } ] }
  ```
  Contenu : **commande** — une entrée `command.<id>` par commande déclarée du registre qui a un sens au clic (outils,
  navigateurs, présentation, maths, nouveaux documents, plein écran, annuler, rétablir, mode conception, lecteur de
  sons, enregistreurs, quitter…) ; **page** — `page.goto`, `page.clear`, `page.copy`, `page.cut`,
  `page.duplicate-next`, `page.duplicate-previous`, `page.duplicate-last`, `page.first`, `page.last`,
  `page.insert-after`, `page.insert-before`, `page.next`, `page.previous`, `page.history-back`,
  `page.history-forward`, `page.reset`, `page.background` (chacune nomme sa commande, A19 ; une commande absente
  retire l'entrée et la liste au rapport) ; **objet** (exécutées par le moteur, `command` vide, `target` vrai) —
  `object.angle` (`angle`), `object.angle-step` (`angle`), `object.place-<a>` et `object.size-<a>` (`number` :
  largeur), `object.size-step-<a>` (`number` : facteur), `object.stretch-<a>` (`number`, `number`),
  `object.stretch-step-<a>` pour les neuf ancrages `top-left`, `top`, `top-right`, `left`, `center`, `right`,
  `bottom-left`, `bottom`, `bottom-right`, `object.place-step` (`number` dx, dy), `object.mirror-x`,
  `object.mirror-y`, `object.flip-x`, `object.flip-y`, `object.original-size`, `object.fit-height`,
  `object.fit-page`, `object.fit-width`, `object.align-<a>` (les dix alignements du lot 5), `object.forward`,
  `object.backward`, `object.front`, `object.back`, `object.layer-top`, `object.layer-middle`, `object.layer-bottom`,
  `object.toggle-hidden`, `object.toggle-locked`, `object.toggle-group`, `object.translucency` (`translucency`),
  `object.more-translucent`, `object.less-translucent`, `object.add-text` (`text`), `object.set-text` (`text`),
  `object.text-value` (`number`), `object.new-text` (`text`), `object.extract-text`, `object.clear-annotations`,
  `object.clear-background`, `object.clear-grid`, `object.clear-objects`, `object.copy`, `object.cut`,
  `object.paste`, `object.delete`, `object.duplicate`, `object.select-all` ; **document** —
  `document.open-file` (`file`), `document.open-website` (`url`), `document.play-sound` (`file` ; lecteur du
  sous-lot 8b). Onglet glisser-déposer (`"drag-drop": true`) : sélection, stylo, marqueur, gomme, texte, formes, pot
  de peinture, navigateur de notes, page suivante, page précédente, première page, dernière page, rideau, rétablir la
  page, règle, rapporteur, compas, accrochage à la grille (18, liste de D9).
- [ ] **Étape 5 : chargeur** (`objectactions.cpp`) : `QJsonDocument`, format vérifié, refus d'un `id` en double ou
  d'une catégorie inconnue (message « action n (id) : … »), ressource `:/actcraft/actions/object-actions.json`
  (même mécanisme que `defaultCommandsJson`).
- [ ] **Étape 6 : vert** ; preuve de tâche ; commit « Lot 8 : catalogue des actions d'objet (L8-T6) » ; `NOMS OK`.

<a id="tache-l8-t7"></a>
### Tâche L8-T7 — `core` : moteur des actions d'objet I (géométrie)

**Objectif.** Une fonction pure de `core/` transforme une action d'objet de géométrie en **une** commande
d'annulation sur sa cible, en respectant les limiteurs de la cible (un objet à rotation bloquée ne tourne pas ; à
dimension bloquée, ne change pas de taille ; à déplacement bloqué, ne bouge pas).

**Fichiers.** `core/include/actcraft/core/objectactions.h`, `core/src/objectactions_geometry.cpp`,
`tests/tst_core_objectactions.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_core_objectactions` (le motif prend aussi
`tst_core_objectactions_edit`, encore talon). Mutation : inverser le signe de l'angle incrémentiel →
`incrementNegativeIsCounterClockwise` rouge, remise par `git checkout -- core/src/objectactions_geometry.cpp`.

**Interdits.** QtCore seulement ; aucune modification directe du document (tout passe par la commande rendue) ;
déléguer à `TransformObjectsCommand` (A10) plutôt que d'écrire une commande de plus.

**Terminé quand** les neuf fonctions passent.

**Étapes.**
- [ ] **Étape 1 : en-tête** (remplace le talon) :
  ```cpp
  #pragma once
  #include <actcraft/core/model.h>
  #include <actcraft/core/undo.h>

  #include <memory>
  #include <string>

  namespace actcraft::core {
  struct ActionContext
  {
      std::size_t page = 0;
      ObjectId source = 0;  // objet cliqué, porteur de l'action
      ObjectId target = 0;  // cible ; 0 : l'objet cliqué
      double pageWidth = 1024;
      double pageHeight = 768;
  };
  // Commande d'annulation qui exécute l'action d'objet `id` (catalogue, L8-T6) avec ses paramètres (objet JSON
  // compact) ; nullptr si l'identifiant est inconnu, la cible absente, ou si l'action ne change rien.
  std::unique_ptr<UndoCommand> objectActionCommand(const Document& document, const std::string& id,
                                                   const std::string& parameters, const ActionContext& context);
  bool isGeometryAction(const std::string& id) noexcept;
  bool isEditAction(const std::string& id) noexcept; // L8-T8
  } // namespace actcraft::core
  ```
- [ ] **Étape 2 : test rouge** `tests/tst_core_objectactions.cpp` :
  ```cpp
  #include <actcraft/core/interactivity.h>
  #include <actcraft/core/objectactions.h>

  #include <QtTest>
  #include <cmath>

  using namespace actcraft::core;

  class TstCoreObjectActions : public QObject
  {
      Q_OBJECT
      static Document page()
      {
          Document d = Document::withOnePage(1024, 768);
          Shape s;
          s.bounds = {0, 0, 100, 50};
          PageObject o{s};
          o.common.id = 1;
          o.common.transform = Transform::translation(200, 100);
          d.pages[0].objects.push_back(o);
          return d;
      }
      static Document after(Document d, const char* id, const char* params)
      {
          auto c = objectActionCommand(d, id, params, ActionContext{0, 1, 0, 1024, 768});
          [&] { QVERIFY2(c, id); }();
          Changes changes;
          c->redo(d, changes);
          return d;
      }
      static RectF box(const Document& d) { return boundsOnPage(d.pages[0].objects[0]); }
      static double angleOf(const Document& d) { return d.pages[0].objects[0].common.transform.rotationDegrees(); }
  private slots:
      void angleSetsAbsoluteRotation()
      {
          QCOMPARE(qRound(angleOf(after(after(page(), "object.angle", R"({"angle":30})"), "object.angle",
                                        R"({"angle":30})"))), 30);
      }
      void incrementNegativeIsCounterClockwise()
      {
          QCOMPARE(qRound(angleOf(after(page(), "object.angle-step", R"({"angle":-15})"))), -15);
      }
      void placeCenterUsesThePage()
      {
          const RectF b = box(after(page(), "object.place-center", "{}"));
          QCOMPARE(qRound(b.x + b.width / 2), 512);
          QCOMPARE(qRound(b.y + b.height / 2), 384);
      }
      void sizeKeepsProportions()
      {
          const RectF b = box(after(page(), "object.size-top-left", R"({"width":200})"));
          QCOMPARE(qRound(b.width), 200);
          QCOMPARE(qRound(b.height), 100);
          QCOMPARE(qRound(b.x), 200); // ancrage en haut à gauche gardé
      }
      void stretchDoesNot()
      {
          const RectF b = box(after(page(), "object.stretch-center", R"({"width":200,"height":50})"));
          QCOMPARE(qRound(b.width), 200);
          QCOMPARE(qRound(b.height), 50);
      }
      void nineAnchors()
      {
          for (const char* a : {"top-left", "top", "top-right", "left", "center", "right", "bottom-left", "bottom",
                                "bottom-right"}) {
              const std::string id = std::string("object.place-") + a;
              QVERIFY2(objectActionCommand(page(), id, "{}", ActionContext{0, 1, 0, 1024, 768}), a);
          }
      }
      void mirrorIsOneStep()
      {
          Document d = page();
          auto c = objectActionCommand(d, "object.mirror-x", "{}", ActionContext{0, 1, 0, 1024, 768});
          QVERIFY(c);
          Changes changes;
          c->redo(d, changes);
          c->undo(d, changes);
          QCOMPARE(fingerprint(d), fingerprint(page()));
      }
      void fixedRestrictorsAreRespected()
      {
          Document d = page();
          setTurning(d.pages[0].objects[0].common.restrictions, Turning::Fixed);
          setTravel(d.pages[0].objects[0].common.restrictions, Travel::Fixed);
          QVERIFY(!objectActionCommand(d, "object.angle", R"({"angle":30})", ActionContext{0, 1, 0, 1024, 768}));
          QVERIFY(!objectActionCommand(d, "object.place-center", "{}", ActionContext{0, 1, 0, 1024, 768}));
      }
      void unknownIdReturnsNull()
      {
          QVERIFY(!objectActionCommand(page(), "object.nothing", "{}", ActionContext{0, 1, 0, 1024, 768}));
          QVERIFY(!objectActionCommand(page(), "object.angle", "{}", ActionContext{0, 99, 0, 1024, 768}));
      }
  };

  QTEST_MAIN(TstCoreObjectActions)
  #include "tst_core_objectactions.moc"
  ```
  (`Transform::translation`, `rotationDegrees`, `boundsOnPage`, `Changes` : noms de `model.h`, `edits.h`, `undo.h` ;
  suivre les noms réels s'ils diffèrent, relevés par L8-T0.)
- [ ] **Étape 3 : rouge** (talon : `objectActionCommand` non défini → erreur d'édition des liens).
- [ ] **Étape 4 : moteur.** Table `identifiant → fonction` (un `std::unordered_map<std::string, Builder>` local) ;
  chaque fonction calcule la nouvelle transformation de la cible (rotation autour du pivot de la cible, D8-7 ; ancrage
  sur la boîte englobante en coordonnées de page) puis rend `std::make_unique<TransformObjectsCommand>(page, {target},
  delta)` ; refus si le limiteur correspondant est bloqué ; `isGeometryAction` lit la même table.
- [ ] **Étape 5 : vert**, mutation rouge puis remise ; preuve de tâche ; commit « Lot 8 : moteur des actions
  d'objet, géométrie (L8-T7) » ; `NOMS OK`.

<a id="tache-l8-t8"></a>
### Tâche L8-T8 — `core` : moteur des actions d'objet II (édition)

**Objectif.** Compléter `objectActionCommand` pour les actions d'édition (`isEditAction`), chacune en **une** étape,
en déléguant aux commandes des lots 3 et 5 : superposition (`ReorderObjectsCommand`), couches (`SetLayerCommand`),
masquer et verrouiller (`SetObjectFlagCommand`), translucidité (`SetTranslucencyCommand`, bornée 0 à 255, ± 32 par
pas), effacements (`ClearPageCommand`), suppression, duplication, couper, coller (`ReplaceObjectsCommand`), textes.

**Fichiers.** `core/src/objectactions_edit.cpp`, `tests/tst_core_objectactions_edit.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_core_objectactions`.

**Interdits.** Aucun HTML reçu écrit dans un texte (D8-5) : « ajouter », « modifier », « nouveau » échappent le
texte (`QString::toHtmlEscaped`) ; « valeur arithmétique » n'agit que si le texte entier et la valeur sont des nombres
(`5` puis `-2` donne `3`) ; copier, couper et coller passent par le presse-papiers du lot 5 (A18) **hors** `core/`
: le moteur rend pour eux `nullptr` et l'exécuteur (L8-T9) déclenche les commandes `edit.*` sur la cible
sélectionnée ; ne pas toucher à `objectactions.h` sauf pour un commentaire.

**Terminé quand** les neuf fonctions passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_core_objectactions_edit.cpp`, même montage que L8-T7 (une forme 1 et un
  texte 2 contenant `5`) : `toggleHiddenTwiceRestores` (deux exécutions rendent `visible` à vrai, deux étapes) ;
  `toggleHiddenDelegatesToObjectFlags` (`dynamic_cast<SetObjectFlagCommand*>(c.get())` non nul) ;
  `textValueArithmetic` (texte « 5 », valeur −2 → « 3 ») ; `textValueNonNumericUnchanged` (texte « cinq » → `nullptr`) ;
  `addTextIsPlainEscaped` (texte « <b>x</b> » ajouté : le HTML du texte contient `&lt;b&gt;`) ;
  `extractTextOneObjectPerWord` (texte « un deux trois » → une commande qui ajoute trois objets texte) ;
  `translucencyClamped` (valeur 300 → 255 ; « moins translucide » à 0 → `nullptr`) ;
  `clearAnnotationsOnlyTopStrokes` (un trait de la couche du dessus et une forme : seul le trait part) ;
  `everyEditIsOneStep` (pour chaque identifiant d'édition du catalogue rendu non nul : `redo` puis `undo` rendent
  l'empreinte de départ). Code de chaque fonction sur le patron de `mirrorIsOneStep` (L8-T7).
- [ ] **Étape 2 : rouge.** **Étape 3 : moteur** (seconde table, `isEditAction`). **Étape 4 : vert** ; preuve de
  tâche ; commit « Lot 8 : moteur des actions d'objet, édition (L8-T8) » ; `NOMS OK`.

<a id="tache-l8-t9"></a>
### Tâche L8-T9 — `ui` : exécuteur d'actions

**Objectif.** Déclencher les actions au clic (D8-2, D8-3) sans contourner le verrou ni la garde de contact :
`ui::ActionRunner` reçoit du `ToolHost` les appuis et relâchés de l'outil de sélection ; un clic (relâché à moins de
4 px, sans glisser) sur un objet qui porte une action exécute ses actions dans l'ordre ; un glisser est rendu à
l'outil de sélection ; le clic droit sélectionne ; en mode conception (réglage « désactiver les actions », A23),
l'outil de sélection reçoit tout. Le patron évident est le mauvais : exécuter dans le gestionnaire d'appui (la
garde de contact est encore vraie, et un glisser commencerait par une action).

**Fichiers.** `ui/include/actcraft/ui/actionrunner.h`, `ui/src/actionrunner.cpp`, `ui/include/actcraft/ui/pagehistory.h`,
`ui/src/pagehistory.cpp`, `ui/src/toolhost.cpp`, `ui/qml/ActionOverlays.qml`, `tests/tst_ui_actionrunner.cpp`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_actionrunner|tst_ui_tools|tst_tools_select'` ; banc hors
écran joint.

**Interdits.** Aucune action pendant un contact ; une action de catégorie `command` passe par
`Commands::registry().trigger` (verrou et diffusion protégée compris) ; une action `document.*` passe par
`LinkLauncher::request` (jamais d'ouverture directe) ; une action d'objet passe par `objectActionCommand` puis
`DocumentController::execute` ; aucun redessin du cache demandé par l'exécuteur ; dans le QML, `pageArea.view`.

**Terminé quand** les treize fonctions passent et `tst_ui_tools`, `tst_tools_select` restent verts.

**Étapes.**
- [ ] **Étape 1 : en-tête** :
  ```cpp
  #pragma once
  #include <actcraft/actions/objectactions.h>
  #include <actcraft/core/model.h>

  #include <QObject>
  #include <QPointF>
  #include <QPointer>
  #include <QtQml/qqmlregistration.h>

  namespace actcraft::ui {
  class Commands;
  class DocumentController;
  class LinkLauncher;
  class PageHistory;
  class ActionRunner : public QObject
  {
      Q_OBJECT
      QML_ELEMENT
      Q_PROPERTY(actcraft::ui::Commands* commands READ commands WRITE setCommands NOTIFY commandsChanged)
      Q_PROPERTY(bool contactActive READ contactActive WRITE setContactActive NOTIFY contactActiveChanged)
      Q_PROPERTY(QObject* document READ document WRITE setDocument NOTIFY documentChanged)
      Q_PROPERTY(QObject* designMode READ designMode WRITE setDesignMode NOTIFY designModeChanged)
      Q_PROPERTY(actcraft::ui::LinkLauncher* links READ links WRITE setLinks NOTIFY linksChanged)
      Q_PROPERTY(bool hovering READ hovering NOTIFY hoveringChanged) // curseur d'action (F-890)
  public:
      explicit ActionRunner(QObject* parent = nullptr);
      // Appelés par ToolHost quand l'outil courant est la sélection ; rendent vrai si l'échantillon est pris.
      bool press(QPointF pagePoint, Qt::MouseButton button);
      bool move(QPointF pagePoint);    // au-delà de 4 px : rend faux et laisse le glisser à l'outil
      bool release(QPointF pagePoint); // exécute, une fois contactActive retombé (file d'attente d'un tour)
      Q_INVOKABLE void hover(QPointF pagePoint);
      // … accesseurs des propriétés
  signals:
      void actionRefused(const QString& reason); // verrou, diffusion protégée, action non prise en charge
      void commandsChanged();
      void contactActiveChanged();
      void documentChanged();
      void designModeChanged();
      void linksChanged();
      void hoveringChanged();
  };
  } // namespace actcraft::ui
  ```
  `PageHistory` : `void visited(int page)`, `int back()`, `int forward()` (−1 si rien), pile bornée à 100, une
  navigation par l'historique n'est pas réinscrite (D8-12).
- [ ] **Étape 2 : test rouge** `tests/tst_ui_actionrunner.cpp` (montage : `DocumentController` réel, `Commands`
  réel, faux `LinkLauncher` par son ouvreur injectable, `DesignMode` réel ; document : rond 1 portant
  `object.toggle-hidden` visant l'étiquette 2) — fonctions de la ligne de la table :
  `clickRunsEveryActionInOrder` (deux actions : la seconde voit l'effet de la première),
  `dragDoesNotRun` (appui, déplacement de 10 px, relâché : l'étiquette reste visible, `press`/`move` rendent faux
  après 4 px), `rightClickSelectsWithoutRunning`, `designModeSelectsInstead`, `lockedCommandActionRefused`
  (`registry().setLocked(true)` : action `command.tool.pen` refusée, `actionRefused` émis),
  `pageActionCreatesNoStep` (`page.next` : `canUndo` inchangé), `objectActionIsOneStep` (un `undo` rend l'empreinte),
  `neverDuringContact` (relâché pendant que `contactActive` est vrai : rien ; à la retombée : exécution),
  `hiddenObjectStillClickable` (D5), `lockedObjectStillRuns` (D4), `historyBackAndForward`,
  `unsupportedActionDoesNothing` (`catalogId` vide : rien, `actionRefused`), `refusedWithoutCommands`.
- [ ] **Étape 3 : rouge.** **Étape 4 : exécuteur et `ToolHost`** : `ToolHost::setActionRunner(ActionRunner*)` ;
  dans l'aiguillage des échantillons, si l'outil courant est la sélection, l'appui va d'abord à `press`, les
  déplacements à `move` tant qu'il les prend, le relâché à `release` ; sinon l'outil reçoit l'appui retenu puis les
  suivants (aucun échantillon perdu). `ActionOverlays.qml` : cadre rouge à nous sur les objets d'action quand
  `appDesign.highlightActions` est vrai, curseur `Qt.PointingHandCursor` quand `appActionRunner.hovering`.
- [ ] **Étape 5 : vert** ; banc hors écran ; preuve de tâche ; commit « Lot 8 : exécuteur d'actions (L8-T9) ».

<a id="tache-l8-t10"></a>
### Tâche L8-T10 — `ui` : ouverture des liens et des fichiers après confirmation

**Objectif.** Seul chemin vers `QDesktopServices::openUrl` pour le contenu d'un document (D8-4) : valider la cible,
demander la confirmation, n'ouvrir qu'après « oui ».

**Fichiers.** `ui/include/actcraft/ui/linklauncher.h`, `ui/src/linklauncher.cpp`, `ui/qml/LinkConfirmDialog.qml`,
`tests/tst_ui_linklauncher.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_linklauncher`. Mutation : appeler l'ouvreur dans `request`
au lieu d'`answer` → `noOpenWithoutConfirmation` rouge, remise par `git checkout -- ui/src/linklauncher.cpp`.

**Interdits.** Aucun `QProcess` ; aucun chemin réseau ; aucun exécutable ; aucune ouverture sans réponse ; le
fichier extrait ne garde que les caractères `[A-Za-z0-9._ -]` de son nom d'origine.

**Terminé quand** les neuf fonctions passent et la mutation a été vue rouge.

**Étapes.**
- [ ] **Étape 1 : en-tête** :
  ```cpp
  #pragma once
  #include <QObject>
  #include <QUrl>
  #include <QtQml/qqmlregistration.h>

  #include <functional>

  namespace actcraft::ui {
  class LinkLauncher : public QObject
  {
      Q_OBJECT
      QML_ELEMENT
  public:
      enum Kind { Website, File, StoredFile, SystemMedia };
      Q_ENUM(Kind)
      using Opener = std::function<bool(const QUrl&)>;
      explicit LinkLauncher(QObject* parent = nullptr);
      void setOpener(Opener opener); // tests ; défaut : QDesktopServices::openUrl
      // Valide la cible ; émet confirmationNeeded (jeton) ou refused (motif). N'ouvre jamais.
      Q_INVOKABLE void request(const QString& target, int kind);
      // Pour un fichier rangé dans le document : octets, nom d'origine, clé du document.
      void requestStored(const QByteArray& bytes, const QString& originalName, const QString& documentKey);
      Q_INVOKABLE void answer(int token, bool accepted); // seul point qui ouvre
      static QString refusalFor(const QUrl& url, int kind); // vide : acceptée
  signals:
      void confirmationNeeded(const QString& shownTarget, int token);
      void refused(const QString& reason);
      void opened(const QUrl& url);
  };
  } // namespace actcraft::ui
  ```
- [ ] **Étape 2 : test rouge** `tests/tst_ui_linklauncher.cpp` :
  ```cpp
  #include <actcraft/ui/linklauncher.h>

  #include <QSignalSpy>
  #include <QTemporaryDir>
  #include <QtTest>

  using actcraft::ui::LinkLauncher;

  class TstUiLinkLauncher : public QObject
  {
      Q_OBJECT
      QList<QUrl> m_opened;
      void fake(LinkLauncher& l)
      {
          m_opened.clear();
          l.setOpener([this](const QUrl& u) { m_opened << u; return true; });
      }
  private slots:
      void noOpenWithoutConfirmation()
      {
          LinkLauncher l;
          fake(l);
          QSignalSpy ask(&l, &LinkLauncher::confirmationNeeded);
          l.request(QStringLiteral("https://example.org/"), LinkLauncher::Website);
          QCOMPARE(ask.size(), 1);
          QVERIFY(m_opened.isEmpty());
          l.answer(ask[0][1].toInt(), true);
          QCOMPARE(m_opened.size(), 1);
          l.answer(ask[0][1].toInt(), true); // un jeton ne sert qu'une fois
          QCOMPARE(m_opened.size(), 1);
      }
      void refusalOpensNothing()
      {
          LinkLauncher l;
          fake(l);
          QSignalSpy ask(&l, &LinkLauncher::confirmationNeeded);
          l.request(QStringLiteral("https://example.org/"), LinkLauncher::Website);
          l.answer(ask[0][1].toInt(), false);
          QVERIFY(m_opened.isEmpty());
      }
      void onlyHttpAndHttps()
      {
          for (const char* u : {"ftp://example.org/", "javascript:alert(1)", "file:///C:/x.txt", "mailto:a@b.c"})
              QVERIFY2(!LinkLauncher::refusalFor(QUrl(QString::fromLatin1(u)), LinkLauncher::Website).isEmpty(), u);
          QVERIFY(LinkLauncher::refusalFor(QUrl(QStringLiteral("http://example.org/")), LinkLauncher::Website).isEmpty());
      }
      void networkPathRefused()
      {
          QVERIFY(!LinkLauncher::refusalFor(QUrl::fromLocalFile(QStringLiteral("//serveur/partage/a.pdf")),
                                            LinkLauncher::File).isEmpty());
          QVERIFY(!LinkLauncher::refusalFor(QUrl(QStringLiteral("file://serveur/partage/a.pdf")), LinkLauncher::File)
                       .isEmpty());
      }
      void executableRefused()
      {
          QTemporaryDir dir;
          for (const char* ext : {"exe", "bat", "cmd", "com", "msi", "ps1", "vbs", "js", "lnk", "scr"}) {
              const QString path = dir.filePath(QStringLiteral("a.") + QString::fromLatin1(ext));
              QFile f(path);
              QVERIFY(f.open(QIODevice::WriteOnly));
              f.close();
              QVERIFY2(!LinkLauncher::refusalFor(QUrl::fromLocalFile(path), LinkLauncher::File).isEmpty(), ext);
          }
      }
      void missingFileRefused()
      {
          QVERIFY(!LinkLauncher::refusalFor(QUrl::fromLocalFile(QStringLiteral("C:/nulle-part/a.pdf")),
                                            LinkLauncher::File).isEmpty());
      }
      void storedFileExtractedUnderASafeName()
      {
          LinkLauncher l;
          fake(l);
          QSignalSpy ask(&l, &LinkLauncher::confirmationNeeded);
          l.requestStored(QByteArray("%PDF-1.4"), QStringLiteral("..\\..\\a<b>.pdf"), QStringLiteral("doc1"));
          QCOMPARE(ask.size(), 1);
          const QString shown = ask[0][0].toString();
          QVERIFY(shown.endsWith(QStringLiteral("ab.pdf")));
          QVERIFY(!shown.contains(QStringLiteral("..")));
      }
      void confirmationShowsTheFullTarget()
      {
          LinkLauncher l;
          fake(l);
          QSignalSpy ask(&l, &LinkLauncher::confirmationNeeded);
          l.request(QStringLiteral("https://example.org/chemin?q=1"), LinkLauncher::Website);
          QCOMPARE(ask[0][0].toString(), QStringLiteral("https://example.org/chemin?q=1"));
      }
      void openingADocumentLaunchesNothing()
      {
          // Aucun appel sans request : un document ouvert n'appelle jamais request (L8-T19 le rejoue de bout en bout).
          LinkLauncher l;
          fake(l);
          QCoreApplication::processEvents();
          QVERIFY(m_opened.isEmpty());
      }
  };

  QTEST_MAIN(TstUiLinkLauncher)
  #include "tst_ui_linklauncher.moc"
  ```
- [ ] **Étape 3 : rouge.** **Étape 4 : code** : jetons croissants gardés dans un `QHash<int, QUrl>` retiré à la
  réponse ; ouvreur par défaut `QDesktopServices::openUrl` (seule occurrence pour le contenu d'un document) ;
  extraction sous `QStandardPaths::writableLocation(TempLocation)/actcraft-<clé>/` ; `LinkConfirmDialog.qml` montre
  la cible complète, deux boutons « Ouvrir » et « Annuler », branché sur `appLinks.confirmationNeeded`.
- [ ] **Étape 5 : vert**, mutation rouge puis remise ; preuve de tâche ; commit « Lot 8 : ouverture des liens
  après confirmation (L8-T10) » ; `NOMS OK`.

> **État de rédaction (rapport intermédiaire).** Les sections L8-T1 à L8-T19 et le plan de 8b (`…-lot8b-PLAN.md`, L8-T20 à L8-T34) restent à écrire. Table, vagues, règles, API, talons, scénarios, hypothèses et L8-T0 font foi dès maintenant.
## Décisions appliquées (tranchées par ce plan ; L8-T0 peut les amender)

- **D8-1 — Une action = un identifiant de notre catalogue.** Le modèle garde `core::Action` du lot 2 (lu des
  `.flipchart` : `kind`, `sourceType`, `command`, `target`, `resource`, `unknownKeys`) et gagne trois champs :
  `catalogId` (identifiant à nous, en anglais à points, par exemple `page.next`, `object.toggle-hidden`,
  `command.tool.pen`, `document.open-website`), `parameters` (objet JSON compact : numéro de page, angle, texte,
  URL…) et `dragDrop` (action posée par l'onglet « Glisser-déposer », F-892). Le catalogue est une **donnée** à nous,
  `actions/data/object-actions.json` (`actcraft-object-actions/1`), quatre catégories : `command`, `page`, `object`,
  `document` ; **aucune action de vote** (amendement 1, § 1). Une action de catégorie `command` porte l'identifiant
  d'une commande du registre et s'exécute par `CommandRegistry::trigger` : le verrou de classe (D6-7) et la diffusion
  protégée (8b) s'y appliquent sans code de plus. Une action d'un `.flipchart` dont le numéro de commande n'est pas
  dans notre table de correspondance (écrite à la main, L8-T5) garde `catalogId` vide : elle est conservée, affichée
  « action non prise en charge » dans le navigateur, et ne fait rien au clic.
- **D8-2 — Déclenchement (F-890).** Un clic (appui puis relâché à moins de 4 px de l'appui, sans glisser) sur un
  objet qui porte une action déclenche **toutes** ses actions dans l'ordre, au relâché, hors mode conception, avec
  l'outil de sélection ; le clic droit sélectionne sans déclencher ; au survol, le curseur change. Avec un autre outil
  que la sélection : défaut à trancher par L8-T0 (décision **D3** ci-dessous ; défaut du plan : seule la sélection
  déclenche, les autres outils écrivent par-dessus, comme la référence en Python). Une action ne s'exécute jamais
  pendant un contact : elle part au relâché, quand `contactActive` est faux.
- **D8-3 — Exécution par la pile d'annulation.** Une action d'objet ou de page qui modifie le document produit **une**
  commande d'annulation (`DocumentController::execute`) ; « Annuler » la défait en une étape. Les actions de
  navigation (page suivante, historique) ne modifient pas le document et ne créent aucune étape.
- **D8-4 — Liens et fichiers : jamais sans confirmation.** Un lien vers un site (F-551), un fichier (F-550), une action
  `document.open-file` ou `document.open-website`, ou un média confié à l'application du système (N2-9) passe par
  `ui::LinkLauncher` : il **demande toujours une confirmation** (boîte à nous qui montre l'adresse ou le chemin
  complet) puis appelle `QDesktopServices::openUrl`. Règles d'adresse : site = `http` ou `https` seulement ; fichier
  = chemin local existant, jamais un chemin réseau (`\\hôte\…`, `file://hôte/…`, SMB), jamais un exécutable
  (`.exe`, `.bat`, `.cmd`, `.com`, `.msi`, `.ps1`, `.vbs`, `.js`, `.lnk`, `.scr`) — un fichier refusé affiche son
  motif ; fichier rangé dans le document (« stocker dans le paperboard ») : extrait dans un dossier temporaire propre
  au document, sous son nom d'origine assaini, puis confirmé. **Un fichier d'un `.flipchart` ne lance rien tout
  seul** : ni à l'ouverture, ni à l'affichage d'une page, ni par « lecture auto » ; la lecture automatique d'un média
  ne vaut que pour le lecteur intégré (8b), jamais pour l'application du système.
- **D8-5 — HTML de document.** Toute ressource HTML (texte d'un objet, cadre web, aide) suit la règle de l'état du
  chantier : « Tout rendu de HTML de document passe par un `QTextDocument` dont `loadResource` est neutralisé
  (L2-T11). Règle pour les lots suivants (outil texte, lot 4 ; export) : jamais `setHtml`/`Text.RichText` sur du
  contenu de document sans ce garde. » Les actions « Ajouter du texte », « Modifier le texte », « Nouvel objet de
  texte » et « Extraire le texte » (L8-T8) écrivent du texte brut échappé, jamais du HTML reçu ; le cadre web (8b) a
  sa propre politique (N1-A3), plus stricte.
- **D8-6 — Conteneurs (F-577, F-580).** Logique pure dans `core/containers.*` : au relâché d'un glisser, le contenu
  candidat (devant le conteneur : couche plus haute, ou même couche et plus haut dans la superposition) est
  « contenu » s'il satisfait la règle (`Anything`, objet précis, mot-clé commun) et la condition (entièrement contenu,
  ou centres alignés à 10 px près, réglable). Sinon, si « retour si non contenu » est vrai, le glisser est **annulé**
  : les objets soulevés reviennent à leur place, **aucune étape** d'annulation, aucun redessin du cache autre que la
  fin du soulèvement. Conteneurs imbriqués : le plus haut dans la superposition qui accepte l'emporte. Signal de
  confirmation : émis (`containerAccepted`, son `acceptSound`), le son est lu par le lecteur intégré au sous-lot 8b.
- **D8-7 — Limiteurs et rotation (F-578, F-579, E6).** Logique pure dans `core/restrictors.*` : déplacement libre,
  vertical, horizontal, le long d'un chemin (projection sur la polyligne d'un objet trait ou ligne), ou bloqué ;
  accrochage à un point (centre, coins, côtés, point précis) ; dimension libre ou bloquée (« bloqué » = E6, tranché
  par L8-T0, défaut : aucune poignée de dimension) ; rotation libre, sens horaire, anti-horaire, bloquée, pas
  d'incrément, pivot (centre, autre point, autre objet) ; objet bloquant (un autre objet glissé s'arrête à son bord).
  Les booléens du lot 2 (`Restrictions::movable`, `resizable`, `rotatable`) restent la source du « bloqué » : un mode
  « bloqué » les met à faux, tout autre mode à vrai (invariant testé par L8-T3). L'outil de sélection les applique
  **pendant** le glisser, sur les objets soulevés (lot 3) : aucun redessin du cache pendant l'écriture.
- **D8-8 — Encre révélatrice (F-255).** Outil `tool.reveal-ink` (Ctrl+Maj+M), variante du stylo qui pose un
  `core::Stroke` de genre `StrokeKind::Revealer` sur la couche du dessus. Rendu : partout où passe le trait, les
  objets de la couche du dessus placés **sous** lui dans la superposition deviennent transparents (découpe), les
  couches intermédiaire, inférieure et le fond restent. Réalisation retenue (décision 0100, complément, à signaler
  au rapport de lot) : pendant le tracé, le trait vivant est montré comme un contour translucide neutre (aucune
  découpe en direct) ; au relâché, le trait est versé et le cache redessiné **tout de suite** — le prédicat `canWait`
  du lot 10 le refuse déjà (images et encre révélatrice non différées) et ce plan ne le change pas. Le navigateur
  d'objet le liste ; on peut le sélectionner, le déplacer (la découpe suit), le supprimer.
- **D8-9 — Mode conception (F-560, L6-T22).** Ses réglages « mettre en évidence les actions » (cadre rouge à nous sur
  les objets d'action, surcouche QML hors cache), « désactiver les actions et glisser une copie », « désactiver les
  restrictions d'objet », « désactiver les objets bloquants » sont lus par l'exécuteur d'actions (L8-T9) et par
  l'outil de sélection (L8-T16) à travers une interface (`tools::InteractionPolicy`) : `tools/` ne connaît pas `ui/`.
- **D8-10 — Navigateur d'actions (F-850, F-851).** Onglet `actions` du panneau des navigateurs (posé grisé par le lot 6,
  A8) : catégorie, liste des actions du catalogue, propriétés de l'action, « Appliquer » à tous les objets
  sélectionnés en **une** étape (`core::SetObjectActionsCommand`), « Retirer les actions existantes ». Le modèle se
  regroupe et se reporte pendant un contact, comme les navigateurs du lot 6 (D6-5).
- **D8-11 — Glisser-déposer (F-892).** Une icône de l'onglet déposée sur un objet lui donne l'action correspondante
  (`dragDrop = true`) en une étape ; l'icône de l'action est dessinée en surimpression, dans une surcouche QML
  au-dessus de la page (jamais dans le cache) ; glisser cette icône vers la corbeille retire l'action. Liste à nous,
  sans les deux entrées de vote.
- **D8-12 — Historique des pages.** Les actions « historique précédent / suivant » lisent un historique des pages
  affichées tenu par `ui::PageHistory` (pile bornée à 100, navigation par historique non réinscrite).

## Sécurité (rappel pour chaque tâche)

- `QDesktopServices::openUrl` n'est appelé **que** par `ui::LinkLauncher::confirmAndOpen`, après la confirmation ; un
  test (`noOpenWithoutConfirmation`) remplace l'ouvreur par un faux et compte les appels ; le contrôle d'occurrences
  (section « Sécurité : contrôle d'occurrences ») le vérifie en L8-T19 et L8-T34.
- Un `.flipchart` ou un `.actcraft` ouvert ne lance rien : aucun lien, fichier, média par l'application du système,
  ni cadre web distant, sans un clic de l'utilisateur **et** une confirmation.
- Toute ressource HTML : règle `loadResource` de l'état du chantier, citée en D8-5 par son texte.
- Aucun chemin de fichier lu d'un document n'est passé à un interpréteur ni à `QProcess`.

## Cache (règles du lot 10)

- Actions, conteneurs, limiteurs : aucun redessin du cache pendant un contact ; une action part au relâché ; un retour
  hors conteneur annule le soulèvement sans étape.
- Un déplacement passe par le **soulèvement** du lot 3 (objets soulevés au-dessus du cache pendant le glisser).
- Encre révélatrice : `canWait` inchangé (images et encre révélatrice non différées) ; test `revealStrokeIsNotDeferred`
  (L8-T18).
- Surimpressions (cadre rouge du mode conception, icônes des actions glisser-déposer, repère « action non prise en
  charge ») : surcouches QML au-dessus de la page, jamais dans le cache.

## API supposées (posées par les lots précédents ; chacune **à revérifier par L8-T0** sauf mention « vérifié »)

| # | API | Lot qui la pose | Employée par | À revérifier |
|---|---|---|---|---|
| A1 | `core::Action { kind, sourceType, command, target, resource, unknownKeys }`, `ActionKind { Other, Navigate, OpenExternal, OnObject }`, `Restrictions { movable, resizable, rotatable }`, `ObjectCommon::actions`, `StrokeKind::Revealer`, `core::Media`, `ObjectKind` (Equation 8, Unknown 9, `kObjectKindCount` 10), `UnknownObject` | lot 2, L2-C8, L0-S3 | L8-T3 à L8-T5 | non (vérifié sur `5373f04`, `model.h:143-359`) |
| A2 | `canWait(page, object)` refuse images, encre révélatrice, grille du dessus | lot 10 | L8-T18 | non (vérifié, `render/src/fixedpageitem.cpp:55-64`) |
| A3 | `paintPage`, `paintObject`, `paintObjectLocal`, `paintStroke` : l'encre révélatrice y est dessinée comme un trait ordinaire | lot 2, L0-S3 | L8-T18 | non (vérifié, `render/src/pagepainter.cpp:193-213,626-723`) |
| A4 | lecteur `.flipchart` : actions 22 à 28, 50, 463 lues et rattachées au dernier objet (`formats/src/flipchart.cpp:562-568,614-621`, `actionFrom` 667-719) ; communs lus : visible, verrou, trois booléens (998-1002) ; conteneurs et limiteurs non lus | lot 2 | L8-T5 | non (vérifié) |
| A5 | écrivain synthétique `tests/support/flipchartwriter.h` | lot 7 | L8-T5 | oui |
| A6 | `actions::CommandRegistry` : `loadDeclarations`, `find`, `isEnabled`, `trigger`, `setLocked`, `isLocked`, `isOwnEnabled` ; `CommandInfo::lockable` (défaut vrai), `restrictable` (défaut faux) | lot 1, L6-T3 | L8-T1, L8-T6, L8-T9, 8b | oui (absents de `5373f04`) |
| A7 | `ui::Commands` (`registry()`, `setNativeHandler`, `stateChanged`) ; instance QML `appCommands` ; contrôleurs « fermés par défaut » (`commands` nul : refus) | lot 1, L6-T5, amendement 2 du lot 6 (B1) | L8-T9, L8-T11, L8-T17 | oui |
| A8 | `ui::DocumentController::execute(std::unique_ptr<core::UndoCommand>)`, `document()`, `currentPageIndex()`, `currentPageChanged`, `addChangeObserver`, `ChangeOrigin` (`Lift`, `Drop`) | L0-S5, lot 2 | L8-T9, L8-T11, L8-T13, L8-T16 | oui |
| A9 | soulèvement : `DocumentController::liftObjects(std::vector<core::ObjectId>)` (copies retirées de la page affichée, le cache reçoit `remove`, sans `editAboutToApply`) et `dropLifted(std::unique_ptr<core::UndoCommand>)` (`nullptr` : dépôt sans étape) | lot 3 | L8-T16 | oui |
| A10 | `core::TransformObjectsCommand(std::size_t page, std::vector<ObjectId>, …)` | lot 3 | L8-T7, L8-T16 | oui |
| A11 | outil de sélection : `tools/include/actcraft/tools/selecttool.h`, `tools/src/selecttool.cpp`, `tools/src/selectiongeometry.cpp` | lot 3 | L8-T16 | oui |
| A12 | `ui::ToolHost` (`ui/src/toolhost.cpp`) : `user()`, `activeTool()`, `setSelection(QList<quint64>)`, `selectionChanged` | L0-S6, lot 3 | L8-T9 | oui |
| A13 | garde de contact `ui::PageView::contactActive` / `contactActiveChanged` (L4-T17) ; en QML `pageArea.view.contactActive` | lot 4 | L8-T9, L8-T11, L8-T16 | oui |
| A14 | `pageArea.view.addInputCover(QQuickItem*)`, `removeInputCover` ; emplacement `pageOverlays` (rideau z 10, spot z 20, outils de maths z 30) | L0-S7, L0-S9, lot 7 | L8-T2, L8-T9 | oui |
| A15 | `BrowserPanel.qml` : `current` (`pages`, `resources`, `objects`, `notes`, `properties`, `actions`) ; onglet `actions` grisé par le lot 6 | L4-T19, L6-T2 | L8-T2, L8-T12 | oui |
| A16 | `core::ObjectFlag { Locked, Hidden, DragCopy }`, `SetObjectFlagCommand`, `ReorderObjectsCommand`, `SetLayerCommand`, `arrangedOrder` | lot 5 | L8-T8 | oui |
| A17 | `core::SetTranslucencyCommand` | lot 3 | L8-T8 | oui |
| A18 | presse-papiers d'objets (`formats::encodeClipboard`, `decodeClipboard`, `resourcesFor`) ; commandes `edit.copy`, `edit.cut`, `edit.paste`, `edit.duplicate`, `edit.delete` | lot 5 | L8-T6, L8-T8 | oui |
| A19 | commandes de page : `page.first`, `page.last`, `page.goto`, `page.insert-after`, `page.insert-before`, `page.duplicate`, `page.clear`, `page.reset`, `page.copy`, `page.cut`, `page.background` | lots 3 et 4 | L8-T6, L8-T9 | oui |
| A20 | `core::ReplaceObjectsCommand`, contrat de `Change` | L0-S3 | L8-T8, L8-T11 | oui |
| A21 | mots-clés d'objet (`ObjectCommon::keywords`, L6-T10) | lot 6 | L8-T14, L8-T17 | oui |
| A22 | `ui::PropertyModel` (sections, éditeurs, `setValue`), `core::SetObjectPropertiesCommand` | L6-T11, L6-T12 | L8-T17 | oui |
| A23 | mode conception `ui::DesignMode` et ses réglages (actions et glisser une copie désactivés, restrictions désactivées, objets bloquants désactivés, actions mises en évidence) | L6-T22 | L8-T9, L8-T16 | oui |
| A24 | stylo : `tools::PenTool`, mémoire par outil, `activate("pen-shape", …)` (encre révélatrice annoncée par le lot 5) ; registre `tools::ToolRegistry` | lots 3 et 5 | L8-T18 | oui |
| A25 | `formats/src/nativejson.cpp` : `writeCommon` (`restrictions{move,size,rotate}`, `actions[]`), `KnownKeys`, `readUnknown`, `followsVariantOrder` | lots 2, 6, socle | L8-T3, L8-T4 | non pour les noms (vérifié, `nativejson.cpp:379-390,528-548,693-712,831-841,1397-1430`) ; oui pour les ajouts du lot 6 |
| A26 | joueur de scénarios : étapes `pointer` (appui, glisser, relâché, clic droit), `typeText`, `drag` vers un objet QML, `restart` (L6-T18) | lot 1, L0-S2, L4-T14, L6-T18 | L8-T12, L8-T13, L8-T16, L8-T18, L8-T19 | oui |
| A27 | corbeille sous la palette (dépôt d'un élément glissé) | lot 3 | L8-T13 | oui |
| A28 | navigateur d'objet `ui::ObjectTreeModel` | L6-T14 | L8-T18 | oui |
| A29 | commandes du lot 7 : `tool.camera-area`, `tool.camera-polygon`, `tool.camera-freehand`, `tool.camera-window`, `tool.camera-screen` (le plan du lot 6 écrit `camera.*` : à trancher par L8-T0), `present.curtain`, `math.ruler`, `math.protractor`, `math.compass` ; ouvreur injectable de `tool.web-browser` (`ui/src/presentationcommands.*`, `QDesktopServices::openUrl`) ; épinglages `docs/verifications/*-lot7-epinglages.md` | lot 7 | L8-T6, L8-T19, 8b | oui |
| A30 | réglages publiés du lot 6 : sections « Enregistrements » et « Multimédia » (`ui/settingsschema.h`, L6-T27) ; bibliothèque de ressources `ui::ResourceLibrary` (L6-T20) | lot 6 | 8b | oui |

## Talons : règle d'honnêteté

1. **Marqueur unique** `Talon L8 : rempli par <tâche>.` — en commentaire C++ dans chaque `.h`/`.cpp` talon, en
   commentaire QML dans chaque `.qml` talon, dans le champ `description` de chaque scénario JSON talon, et comme texte
   du `QSKIP` de chaque test talon : `QSKIP("Talon L8 : rempli par L8-T14.");`.
2. Un test talon n'a qu'une fonction sautée, `reserved()` ; la tâche propriétaire la supprime et ajoute les siennes.
3. Compteur : N = N8 + 21 dès L8-T1 (N8 + 37 dès L8-T21) ; chaque preuve donne N exact **et** les fonctions attendues
   en `PASS`.
4. Contrôle (L8-T19 pour 8a, L8-T34 pour le lot) : `& build/debug/tests/<test>.exe -functions` ne montre `reserved`
   pour aucun test du sous-lot ; `git -C <wt> grep -n "Talon L8 : rempli par" -- . ':(exclude)docs/'` ne rend plus
   que des tâches de 8b à la fin de 8a, et rien à la fin du lot ; sur `ctest --preset debug -V`, aucune ligne
   `SKIP   :` qui contienne `Talon` (restent permis : « ACTCRAFT_SAMPLES_DIR absent », « Windows seulement », et les
   sauts déjà connus des tests d'oracle).

## Sécurité : contrôle d'occurrences (L8-T19, L8-T34)

`git -C <wt> grep -n -E "QDesktopServices|openUrlExternally" -- core formats actions tools render ui app media features`
ne rend que : `ui/src/linklauncher.cpp` (ouvreur des liens du document, après confirmation), `ui/src/presentationcommands.cpp`
(navigateur web du lot 7, adresse d'accueil réglée par l'utilisateur, jamais tirée d'un document), `ui/qml/AboutDialog.qml`
et `ui/qml/Main.qml` (page du projet, adresse fixe). Toute autre ligne refuse la tâche.

## Scénarios

Scénarios servis par le lot : **S11** (cadrage), **S18**, **S25**, **S26**, **S27** (amendement 1), **S30**
(ouverture), **S32** (préparé : épinglages du profil « Son et image »). Sous-scénarios de S11 posés par ce plan, joués
sur des documents synthétiques écrits par nos tests (jamais un paperboard réel dans le dépôt) :

- **S11.1 — Action d'objet.** Un rond porte l'action « masquer ou afficher » visant une étiquette : un clic avec la
  sélection masque l'étiquette (pixels), un second la réaffiche ; un glisser du rond ne déclenche rien ; Annuler
  défait le dernier masquage en une étape ; en mode conception, le clic sélectionne le rond.
- **S11.2 — Navigateur d'actions.** Sans sélection, la liste est grisée ; un objet sélectionné, catégorie « Page »,
  action « page suivante », Appliquer : un clic sur l'objet passe à la page 2 ; « retirer les actions » : le clic ne
  fait plus rien.
- **S11.3 — Glisser-déposer.** L'icône « Stylo » déposée sur un objet : l'icône s'affiche en surimpression ; un clic
  sur l'objet active le stylo ; l'icône glissée vers la corbeille retire l'action.
- **S11.4 — Conteneur.** Un panier accepte les objets de mot-clé « fruit », retour si non contenu : une pomme déposée
  dedans y reste (une étape) ; une carotte (« légume ») revient à sa place, aucune étape ; un objet derrière le panier
  n'est jamais contenu.
- **S11.5 — Limiteurs.** Un objet limité « verticalement » glissé en diagonale garde son X ; un objet à dimension
  bloquée n'a pas de poignée de dimension ; une rotation au pas de 45° s'arrête sur un multiple de 45.
- **S11.6 — Encre révélatrice.** Une image sous un rectangle de la couche du dessus : un trait d'encre révélatrice
  sur le rectangle révèle l'image (pixels) ; un rectangle posé ensuite au-dessus reste opaque ; déplacer le trait
  déplace la zone révélée.
- **S11.7 — Lien.** Un objet lié à `https://example.org/` : le clic affiche la confirmation avec l'adresse complète ;
  refuser n'ouvre rien ; accepter appelle l'ouvreur une fois (faux ouvreur des tests) ; à l'ouverture d'un document
  dont un objet porte un lien et un média « lecture auto », rien ne s'ouvre.
- **S11 local.** Le paperboard réel n° 13 de `samples/` (lu hors dépôt, jamais nommé) : un clic sur un rond rouge
  masque puis réaffiche l'étiquette visée ; un objet déposé hors de son conteneur revient à sa place. Test
  `tst_formats_samples_interactivity`, sauté sans `ACTCRAFT_SAMPLES_DIR`.

| Scénario | Tâches (8a et 8b) |
|---|---|
| S11 | L8-T1 à L8-T19 |
| S11.1 | L8-T3 à L8-T9, L8-T19 |
| S11.2 | L8-T6, L8-T11, L8-T12 |
| S11.3 | L8-T13 |
| S11.4 | L8-T14, L8-T16, L8-T17 |
| S11.5 | L8-T15, L8-T16, L8-T17 |
| S11.6 | L8-T18 |
| S11.7 | L8-T10, L8-T19 ; dialogues : L8-T22 |
| S18 | L8-T3, L8-T4, L8-T31, L8-T32 |
| S25 | L8-T3, L8-T4, L8-T20, L8-T23 à L8-T25 |
| S26 | L8-T20, L8-T26 à L8-T28 |
| S27 | L8-T3, L8-T4, L8-T20, L8-T29, L8-T30 |
| S30 | L8-T0 |
| S32 (préparé) | L8-T33 |

## Hypothèses pour Jérôme (créées par ce plan)

- **H8-1** Catalogue d'actions à nous, sans les 8 actions de vote ni les 2 entrées de vote du glisser-déposer ; une
  action de commande déclenche une commande d'ActCraft ; une commande que nous n'avons pas (services en ligne de
  Promethean) n'a pas d'action.
- **H8-2** Seule la sélection déclenche les actions (défaut, D3 de L8-T0) ; avec le stylo, on écrit par-dessus.
- **H8-3** Toute ouverture d'un site, d'un fichier ou d'un média par l'application du système demande une
  confirmation, à chaque fois ; exécutables et chemins réseau refusés, même confirmés. Écart voulu avec la référence.
- **H8-4** Encre révélatrice : contour translucide pendant le tracé, découpe au relâché (décision 0100, complément).
- **H8-5** Conteneur : retour instantané à la place de départ, sans animation ni étape d'annulation.
- **H8-6** Téléchargement des modules Qt Multimedia (avec FFmpeg) et Qt WebView pour Windows et Android, par
  aqtinstall depuis les miroirs de Qt (absents de `C:\Qt\6.12.0\msvc2022_64`, vérifié le 2026-10-19) : **accord de
  Jérôme à demander** avant L8-T20 ; taille ajoutée aux paquets rapportée (N1-A10). Sans accord : médias par
  l'application du système après confirmation (N2-9), enregistreurs et cadre web absents, 8b replanifié.
- **H8-7** Poids du cadre web (N1-A3) mesuré par L8-T0 puis L8-T20 ; F-302 derrière l'option `ACTCRAFT_WEBFRAME`.

H8-6 est posée dès la fin de L8-T0 ; les autres à la porte de fin du lot 8.

## Sections des tâches

<a id="tache-l8-t0"></a>
### Tâche L8-T0 — Ouverture : vidéos, REA et ancien dépôt confrontés au lot

**Objectif.** Appliquer la règle de la tâche d'ouverture (amendement 1, § 4, points 1 à 6 ; décisions 0100 et 0121)
au lot 8, sur le modèle de `docs/verifications/2026-10-18-lot10-principes.md` : relever dans le corpus les principes
des fonctions du lot (14 constats de l'amendement, 17 en séparant les sous-puces, V39 en plus) et ceux de la
spécification ; trancher sur la 3.5.18, par REA en statique, les points « à élucider » de l'amendement (F-680 : V58 ;
F-292 : V43 ; F-302 : V63 ; F-552 : V126, V57) et l'écart **E6** ; revérifier A1 à A30 ; relever **N8** ; estimer le
poids des modules du sous-lot 8b.

**Fichiers.** Créer `docs/verifications/2026-10-21-lot8-principes.md`. Hors dépôt, créer
`C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot8.md` (relevé brut, même forme que `lot7.md` et `lot10.md`).
Rien d'autre.

**Test et commande bornée.** Aucun code : preuve = rapport commité, `Test-Path` du relevé brut, `NOMS OK`, contrôles
des titres du corpus et des paperboards réels vides.

**Interdits.** Aucun nom de classe, de propriété ni de fichier de l'original dans le dépôt (le nom de l'original de
l'encre révélatrice compris) ; aucun titre, aucune citation, aucune capture du corpus ; aucun nom de paperboard réel ;
observation à l'écran seulement après avoir prévenu Jérôme ; aucun téléchargement (H8-6) ; ne pas modifier ce plan.

**Terminé quand** le rapport contient la table principe → exigence → tâche → scénario (ou raison) pour chaque
constat ; D1 à D16 tranchées ou « non tranchées » avec le défaut du plan ; la table A1 à A30 ; N8 comparé à
N6 + 43 + p6 ; les tailles des modules ; au plus une tâche de principes proposée (`L8-P1`).

Décisions à trancher : **D1** correspondance des numéros de commande et d'action du format vers notre catalogue ;
**D2** valeurs des modes de déplacement, de rotation, de pivot, d'accrochage et de conteneur dans le format ; **D3**
outils qui déclenchent les actions (défaut : sélection seule) ; **D4** objet verrouillé : clic qui le traverse (V28),
action qui part quand même (V103) ; **D5** objet masqué cliquable (V40, V42 ; le clone fait l'inverse) ; **D6** E6 ;
**D7** sens de « bloquer » (arrêt au bord, ou refus du dépôt) ; **D8** pas de rotation par défaut, pivot « première
ligne de texte » ; **D9** liste du glisser-déposer hors vote (18) ; **D10** messages et refus de la diffusion protégée
(V58), commandes de chacune des quatre interdictions (identifiants de la caméra compris : voir « D10 — noms de la caméra » ci-dessous) ; **D11** enregistreur
d'écran (V43), formats ; **D12** cadre web : image fixe, glisser du texte, comportement sous le rideau (N1-A3) ;
**D13** second média à côté du premier (V126) ; **D14** GIF animé (V57, V66) ; **D15** objet Flash : repère seulement
(N1-A15) ; **D16** conteneur recouvert par une image (V39).

**D10 — noms de la caméra (contradiction consignée, non tranchée par ce plan).** Le plan du lot 7 déclare cinq
commandes `tool.camera-area`, `tool.camera-polygon`, `tool.camera-freehand`, `tool.camera-window`,
`tool.camera-screen` (`…-lot7-PLAN.md`, session B, l. 690-694) ; le plan du lot 6 (L6-T3, D6-7) écrit `camera.*`
pour les déclarer restreignables et dans les préfixes de la barre de raccourcis (`lot6-PLAN.md:998-1004,2161`), tout
en disant « identifiants réels relus dans le fichier ». L8-T0 relève les identifiants **réellement déclarés** dans
`actions/data/commands.json` à la tête du chantier et les écrit au rapport. **Recommandation** : retenir ceux du lot 7
(`tool.camera-*`), parce que c'est le lot qui crée ces commandes et que le lot 6 renvoie lui-même au fichier ; la
diffusion protégée (L8-T31, L8-T32) vise alors le préfixe `tool.camera-`, et un correctif `L8-C0` corrige le préfixe
de la barre de raccourcis si L6-T9 a codé `camera.` en dur. La décision revient à l'orchestrateur, sur le rapport de
L8-T0.

**Étapes.**
- [ ] **Étape 1 : corpus.** Lire `C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\README.md` (« Principes
  absents », « Écarts avec la spec », synthèse « Conteneurs ») ; ouvrir chaque fiche citée ; principes en nos mots dans
  le relevé brut.
- [ ] **Étape 2 : spécification et ancien dépôt.** `graphify query "actions conteneurs limiteurs encre magique médias"`
  dans `C:\Users\JeromeDenis\REA - Activinspire` ; le graphe relie mal ces fichiers : lire ensuite
  `app/actcraft/ui/browsers/actions.py`, `app/actcraft/ui/browsers/hooks.py`, `app/actcraft/canvas/items.py`
  (découpe de l'encre, l. 197-221 et 353-410), `app/actcraft/canvas/page.py:114-124`, `app/actcraft/canvas/view.py:228-249,391-400`,
  `app/actcraft/io/flipchart_reader.py:212-221,453-458,557-558,600-652` ; `docs/spec/fonctionnalites.md` (F-255,
  F-292, F-293, F-302, F-550 à F-552, F-577 à F-582, F-680, F-850, F-851, F-890 à F-892) ;
  `docs/re/format-flipchart.md` § 4.2, § 6, § 7.2, § 7.6.
- [ ] **Étape 3 : REA en statique.** Skill `reverse-engineer-anything` sur `C:\Program Files\Activ Software\Inspire\` :
  énumérations de D2, numéros de D1, chaînes des refus (D10), réglages des enregistrements (D11). Serveur
  indisponible : le noter, garder les défauts.
- [ ] **Étape 4 : API.** Dans le worktree : `graphify query "DocumentController liftObjects dropLifted TransformObjectsCommand"`,
  `graphify query "SelectTool ToolHost selection handles rotation"`, `graphify query "PropertyModel DesignMode contactActive"`,
  `graphify query "CommandRegistry lockable restrictable camera"`, puis lecture des en-têtes ; remplir A1 à A30.
- [ ] **Étape 5 : N8.**
  ```powershell
  pwsh -NoProfile -Command "Set-Location 'C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L8-T0'; New-Item -ItemType Directory -Force .tmp | Out-Null; . ./scripts/dev-env.ps1; cmake --preset debug *> .tmp/cfg.txt; ctest --preset debug -N | Select-String 'Total Tests'"
  ```
  Attendu : `Total Tests: <N8>`.
- [ ] **Étape 6 : tailles.** Sans télécharger : `python -m aqt list-qt windows desktop --modules 6.12.0 win64_msvc2022_64`
  et `python -m aqt list-qt windows desktop --archives 6.12.0 win64_msvc2022_64 qtmultimedia qtwebview` ; moteur de
  Qt WebView 6.12 sous Windows (WebView2 du système ou WebEngine) ; tailles et dépendances au rapport.
- [ ] **Étape 7 : rapport** (en-tête, méthode et limites, constats, D1 à D16, A, N8, tailles, `L8-P1`, écarts pour
  l'orchestrateur).
- [ ] **Étape 8 : contrôles et commit.** `git -C <wt> add docs/verifications/2026-10-21-lot8-principes.md` ;
  `NOMS OK` ; contrôles des titres et des paperboards vides ; commit « Lot 8 : principes confrontés, décisions et API
  revérifiées (L8-T0) ».

> **État de rédaction (rapport intermédiaire).** Les sections L8-T1 à L8-T19 et le plan de 8b (`…-lot8b-PLAN.md`, L8-T20 à L8-T34) restent à écrire. Table, vagues, règles, API, talons, scénarios, hypothèses et L8-T0 font foi dès maintenant.
