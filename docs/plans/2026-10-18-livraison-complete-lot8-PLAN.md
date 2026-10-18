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
| L8-T15 | `core` : limiteurs et rotation (F-578, F-579, E6) : déplacement contraint, chemin, accrochage, dimension bloquée, pivot, pas, sens, objet bloquant | L8-T3 | `core/include/actcraft/core/restrictors.h`, `core/src/restrictors.cpp`, `tests/tst_core_restrictors.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_core_restrictors` (dont `verticalKeepsX`, `horizontalKeepsY`, `fixedTravelZero`, `alongTrackProjects`, `jumpsOntoTrackOnPress`, `snapToAnchor`, `fixedSizeHasNoHandles`, `clockwiseOnly`, `turnStepRounds`, `pivotOnOtherObject`, `blockerStopsAtItsEdge`) Passed ; mutation de la projection rouge puis remise | S11 | #tache-l8-t15 |
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
(V58), commandes de chacune des quatre interdictions (identifiants de la caméra compris, A29) ; **D11** enregistreur
d'écran (V43), formats ; **D12** cadre web : image fixe, glisser du texte, comportement sous le rideau (N1-A3) ;
**D13** second média à côté du premier (V126) ; **D14** GIF animé (V57, V66) ; **D15** objet Flash : repère seulement
(N1-A15) ; **D16** conteneur recouvert par une image (V39).

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

<!-- Sections L8-T1 à L8-T19 : à écrire (plan en cours de rédaction, rapport intermédiaire au tour 35). -->
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
(V58), commandes de chacune des quatre interdictions (identifiants de la caméra compris, A29) ; **D11** enregistreur
d'écran (V43), formats ; **D12** cadre web : image fixe, glisser du texte, comportement sous le rideau (N1-A3) ;
**D13** second média à côté du premier (V126) ; **D14** GIF animé (V57, V66) ; **D15** objet Flash : repère seulement
(N1-A15) ; **D16** conteneur recouvert par une image (V39).

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

<!-- Sections L8-T1 à L8-T19 : à écrire (plan en cours de rédaction, rapport intermédiaire au tour 35). -->
