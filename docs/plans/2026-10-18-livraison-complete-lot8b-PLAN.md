# Livraison complète — lot 8 « Interactivité et multimédia », sous-lot 8b — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**But.** Fin du lot 8 et version **0.9.0** : installation des modules Qt du lot (porte H8-6), dialogues de liens,
médias intégrés (S25), enregistreurs de son et d'écran (S26), cadre web (S27), diffusion protégée (S18), épinglages
du profil « Son et image » (S32 préparé), contrôle du lot. Le sous-lot 8a (`docs/plans/2026-10-18-livraison-complete-lot8-PLAN.md`)
est fait : ses décisions D8-1 à D8-12, ses règles de sécurité et de cache, ses hypothèses H8-1 à H8-7, sa table A1 à
A30, sa règle des talons, ses commandes de preuve communes et les décisions D1 à D16 de L8-T0 valent ici ; les
commandes des deux sous-lots sont déclarées par L8-T1, les emplacements QML posés par L8-T2, le modèle (médias,
cadre web, restrictions de diffusion) par L8-T3 et L8-T4. Exigences de 8b : F-292, F-293, F-302, F-550 et F-551
(dialogues), F-552 (lecteur intégré ; N2-9 en repli), F-582 (effet sur la lecture), F-680. Constats : V43 (palette
de l'enregistreur), V57 et V66 (GIF animé), V58 (diffusion protégée, quatre constats), V63 (cadre web, deux), V55
(objet Flash), V126 (second média).

**API supposées propres à 8b** (en plus de A1 à A30 ; L8-T0 les revérifie, L8-T20 vérifie A31 à A34 après
l'installation) :

| # | API | Lot qui la pose | Employée par | À revérifier |
|---|---|---|---|---|
| A31 | Qt Multimedia 6.12 : `QMediaPlayer`, `QAudioOutput`, `QMediaCaptureSession`, `QAudioInput`, `QMediaRecorder`, `QScreenCapture`, `QVideoSink`, élément QML `VideoOutput`, permission `QMicrophonePermission` | Qt | L8-T24 à L8-T28 | oui (module absent du poste au 2026-10-19) |
| A32 | Qt WebView 6.12 : élément QML `WebView` (`url`, `loadHtml`, `goBack`, `goForward`, `reload`, `settings.javaScriptEnabled`, `settings.localContentCanAccessFileUrls`, `settings.allowFileAccess`), moteur sous Windows | Qt | L8-T30 | oui |
| A33 | insertion d'un média : commande `insert.media` (argument URL facultatif), objet `core::Media` à image d'espace réservé | lot 5 | L8-T23 | oui |
| A34 | `ui::DocumentIo` : enregistrer sous un chemin donné, signal de fin ; dossier des ressources du document | lot 2 | L8-T29, L8-T31 | oui |
| A35 | `actions/data/profiles/son-image.json` et `tst_ui_profiles::pinnedCommandsExist` | L6-T6 | L8-T33 | oui |

## Porte H8-6 : sans décision, la tâche ne commence pas

L8-T20 télécharge les modules Qt Multimedia (avec le moteur FFmpeg qu'il embarque) et Qt WebView, pour Windows et
pour Android (`android_arm64_v8a`, `android_x86_64`), par aqtinstall depuis les miroirs de Qt. C'est un
téléchargement : **sans l'accord écrit de Jérôme, L8-T20 ne commence pas**, comme L6-T32 attend H6-6. L'orchestrateur
pose la question dès la fin de L8-T0, avec les tailles relevées. Les tâches de 8b qui n'en dépendent pas (L8-T22,
L8-T23, L8-T31, L8-T32, L8-T33) peuvent avancer, après une réservation réduite (ci-dessous).

<a id="repli-sans-accord"></a>
**Repli sans accord (amendement 1, B3)** — appliqué par l'orchestrateur, sans autre amendement, si Jérôme refuse H8-6
ou ne répond pas avant la fin de 8a. À chaque étape, le registre se charge et la suite reste verte.

1. **`L8-C1` — correctif de retrait** (ligne de la table, joué seul, avant `L8-T21r`) :
   - `actions/data/commands.json` : **suppression** des 13 entrées `media.sound-controller`, `record.sound`,
     `record.screen-full`, `record.screen-area`, `record.pause`, `record.stop`, `record.cancel`, `insert.web-frame`,
     `webframe.back`, `webframe.forward`, `webframe.reload`, `webframe.snapshot`, `webframe.unblock` — jamais
     `"platforms": []`, que `loadDeclarations` refuse (`actions/src/commandregistry.cpp:174-176`) ;
     `actions/data/layout.json` : leurs places (sous-menu « Enregistreurs », « Cadre web… » du sous-menu « Lien »,
     lecteur de sons) ; `actions/data/object-actions.json` : les entrées `command.*` qui les nomment (sinon
     `everyCommandActionNamesARegisteredCommand` rougit) ;
   - **même commit** : `lot8EntriesDeclared` (les 13 identifiants sortent de la liste, ainsi que le contrôle de
     plateforme des `record.screen-*` et le contrôle `checkable` du lecteur de sons), `lot8AttributesDeclared`
     (`record.sound` et `insert.web-frame` sortent des verrouillables, `media.sound-controller` des libres),
     `everyLot8CommandHasAHandler` et `lot8ShellPlaceholders` (les 13 identifiants et `soundController`,
     `recorderPalette`, `recordingIndicator`, `webFrameLayer`, `webFrameDialog` sortent) ;
   - **talons orphelins** : `ui/qml/SoundController.qml`, `ui/qml/RecorderPalette.qml`, `ui/qml/RecordingIndicator.qml`,
     `ui/qml/WebFrameLayer.qml`, `ui/qml/WebFrameDialog.qml` sont **supprimés** (`git rm`), avec leurs instances
     (`ui/qml/PageArea.qml`, `ui/qml/Main.qml`) et leurs lignes de `ui/CMakeLists.txt` ; aucun gestionnaire
     « indisponible » n'est gardé (les commandes n'existent plus) ; les icônes restent (dessins à nous) ; les fichiers
     de 8b des tâches retirées ne sont jamais créés (`L8-T21r` ne les pose pas) ;
   - preuve : `tst_actions_layout`, `tst_actions_registry`, `tst_actions_lockable`, `tst_actions_objectactions`,
     `tst_ui_shell` Passed ; `TESTS OK`, N = N8 + 21 ; `captures.ps1 -Scenario premiers-pas` `CAPTURES OK` ;
     `git -C <wt> grep -n -E "Talon L8 : rempli par L8-T(2[4-9]|30)\." -- . ':(exclude)docs/'` vide.
2. **`L8-T21r`** — réservation réduite (ligne de la table) : 7 tests, **N = N8 + 28**, aucun module `media`.
3. **Tâches gardées** : L8-T22, L8-T23, L8-T31, L8-T32 telles quelles ; **L8-T25r** et **L8-T33r** (lignes de la
   table) ; L8-T34 avec N = N8 + 28 et les seuls scénarios gardés. **Retirées** : L8-T20, L8-T21, L8-T24, L8-T26 à
   L8-T30.
4. **Vagues du repli** : V10r : L8-C1 · V11r : L8-T21r · V12r : L8-T22 ‖ L8-T31 · V13r : L8-T23 ‖ L8-T33r ·
   V14r : L8-T25r · V15r : L8-T32 · V16r : L8-T34 (une seule tâche par vague touche `Main.qml` : T22, T23, T25r, T32).
5. **Hypothèse présentée à la porte** : F-292, F-293, F-302 et le lecteur intégré de F-552 restent dus (décision 0100)
   et reviennent dès l'accord, par un amendement de ce plan ; le son de confirmation des conteneurs n'est pas joué
   (aucun lecteur intégré ; rien n'est confié au système sans clic).

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L8-T20 | Installation de Qt Multimedia et Qt WebView (Windows, Android) — **porte H8-6** ; CI, licences, notices | L8-T19, accord H8-6 | `.github/workflows/ci.yml`, `.github/workflows/release.yml`, `CLAUDE.md`, `packaging/qt-autorises.txt`, `THIRD_PARTY_NOTICES.md`, `docs/verifications/2026-10-22-lot8-modules.md` | isolé | implementeur-tdd | accord de Jérôme cité ; `Test-Path` des dossiers `lib/cmake/Qt6Multimedia` et `lib/cmake/Qt6WebView` des trois kits ; `scripts/verifier-licences.ps1` sans écart ; rapport des tailles (installateur, APK, avant et après) commité ; `TESTS OK` inchangé | S25, S26, S27 | #tache-l8-t20 |
| L8-T21 | Réservation 8b : module `media`, options `ACTCRAFT_MEDIA` et `ACTCRAFT_WEBFRAME`, 16 tests enregistrés, fichiers neufs de 8b (talons marqués) | L8-T20 | `CMakeLists.txt`, `media/CMakeLists.txt`, `core/CMakeLists.txt`, `ui/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/{media,core,ui,bench}.cmake`, talons : `media/include/actcraft/media/{player,soundrecorder,displayrecorder}.h`, `media/src/{player,soundrecorder,displayrecorder_win,displayrecorder_other}.cpp`, `core/include/actcraft/core/broadcast.h`, `core/src/broadcast.cpp`, `ui/include/actcraft/ui/{linkdialogs,mediainsert,mediaitems,recorders,webpolicy,webframeitem,broadcastgate}.h`, `ui/src/{linkdialogs,mediainsert,mediaitems,recorders,webpolicy,webframeitem,broadcastgate}.cpp`, 11 tests talons `tests/tst_{ui_linkdialogs,ui_mediainsert,media_player,ui_mediaitems,media_soundrecorder,media_displayrecorder,ui_recorders,ui_webpolicy,ui_webframe,core_broadcast,ui_broadcast}.cpp`, 5 scénarios talons `bench/scenarios/{liens,medias,enregistreurs,cadre-web,diffusion-protegee}.json` | isolé | implementeur-tdd | `100% tests passed … out of N`, N = N8 + 37, `TESTS OK` ; 16 tests neufs `SKIP` nommant leur tâche ; `tst_modules` Passed (ordre des modules : `media` après `core` et avant `ui`) ; construction Android `APK OK` sous `GRADLE_OPTS=-Dorg.gradle.daemon=false` (la commande rend la main moins de 60 s après `APK OK`) ; `NOMS OK` | S18, S25, S26, S27 | #tache-l8-t21 |
| L8-T22 | Dialogues de liens (F-550, F-551) : fichier ou site, quatre formes de lien, stockage externe ou dans le document ; scénario `liens` | L8-T21 | `ui/include/actcraft/ui/linkdialogs.h`, `ui/src/linkdialogs.cpp`, `ui/qml/LinkDialog.qml`, `ui/qml/Main.qml`, `tests/tst_ui_linkdialogs.cpp`, `bench/scenarios/liens.json`, `bench/scenarios/fixtures/liens.json` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_linkdialogs` (dont `textFormCreatesATextObject`, `existingObjectGetsTheAction`, `storedFileBecomesAResource`, `externalPathKeptAsIs`, `commandLineArgumentsNeverPassed`, `websiteNeedsHttpOrHttps`, `oneStep`), `tst_captures_liens` Passed ; `captures.ps1 -Scenario liens` : `LU linkConfirmDialog.visible=true OK`, `LU appLinks.openedCount=0 OK`, `CAPTURES OK` | S11 | #tache-l8-t22 |
| L8-T23 | Insertion des médias : son ou vidéo depuis un fichier ou les ressources, image d'espace réservé, action de lecture, second média à côté du premier | L8-T21 | `ui/include/actcraft/ui/mediainsert.h`, `ui/src/mediainsert.cpp`, `ui/qml/Main.qml`, `tests/tst_ui_mediainsert.cpp` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_mediainsert` (dont `soundBecomesAPlaceholderWithAPlayAction`, `videoKeepsItsAspect`, `secondMediaGoesBesideTheFirst`, `resourceDropInsertsInOneStep`, `flashShowsOnlyItsPlaceholder`, `refusedWhileLocked`) Passed | S25 | #tache-l8-t23 |
| L8-T24 | `media` : lecteur intégré (deux pistes, formats lus par Qt Multimedia, repli vers l'application du système) | L8-T21 | `media/include/actcraft/media/player.h`, `media/src/player.cpp`, `tests/tst_media_player.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_media_player` (dont `wavPlaysAndPositionAdvances`, `twoTracksAtOnce`, `secondSoundOnTrackOneReplacesTheFirst`, `unreadableFallsBackToTheSystem`, `fallbackNeedsConfirmation`, `neverOnTheRenderThread`, `stopAllOnDocumentClose`) Passed | S25 | #tache-l8-t24 |
| L8-T25 | QML : médias sur la page (vidéo dans son cadre au-dessus du cache, lecteur de sons, GIF animé, options F-582, son de confirmation des conteneurs) ; scénario `medias` ; banc | L8-T16, L8-T23, L8-T24 | `ui/include/actcraft/ui/mediaitems.h`, `ui/src/mediaitems.cpp`, `ui/qml/MediaLayer.qml`, `ui/qml/SoundController.qml`, `ui/qml/Main.qml`, `tests/tst_ui_mediaitems.cpp`, `bench/scenarios/medias.json`, `bench/scenarios/fixtures/medias.json` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_mediaitems` (dont `videoIsAboveTheCache`, `videoNeverEntersTheCache`, `controlsHiddenOption`, `loopOption`, `playOnOpenOnlyInternal`, `gifAnimates`, `chimePlaysOnAcceptedDrop`, `soundControllerToggles`), `tst_captures_medias` Passed ; `captures.ps1 -Scenario medias` : `waitFor` interne `soundController.playing` vrai, `LU soundController.visible=true OK` (état final), `PIXEL gif-image-2 OK`, `CAPTURES OK` ; banc hors écran joint (vidéo jouée pendant l'écriture) | S25 | #tache-l8-t25 |
| L8-T26 | `media` : enregistreur de son (F-293) : WAV, icône de lecture posée sur la page, permission du micro sous Android | L8-T24 | `media/include/actcraft/media/soundrecorder.h`, `media/src/soundrecorder.cpp`, `android/AndroidManifest.xml`, `tests/tst_media_soundrecorder.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_media_soundrecorder` (dont `writesAWav`, `pauseAndResume`, `stopPlacesAPlayIconOnThePage`, `cancelLeavesNothing`, `refusedPermissionDisablesWithAMessage`, `indicatorStateFollows`) Passed ; `APK OK` (même règle `GRADLE_OPTS`) ; essai Android `ANDROID OK` | S26 | #tache-l8-t26 |
| L8-T27 | `media` : enregistreur d'écran (F-292, Windows) : plein écran ou zone, MP4, pause, arrêt, abandon | L8-T24 | `media/include/actcraft/media/displayrecorder.h`, `media/src/displayrecorder_win.cpp`, `media/src/displayrecorder_other.cpp`, `tests/tst_media_displayrecorder.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_media_displayrecorder` (dont `fullScreenFiveSeconds`, `areaIsCropped`, `pauseExcludesThePause`, `cancelDeletesTheFile`, `notBuiltOutsideWindows`) Passed (sous Windows ; ailleurs `SKIP` « Windows seulement ») | S26 | #tache-l8-t27 |
| L8-T28 | Enregistreurs : palettes, indicateur toujours visible, mention à la première utilisation, réapparition de la palette (V43), réglages « Enregistrements » ; scénario `enregistreurs` ; banc | L8-T26, L8-T27 | `ui/include/actcraft/ui/recorders.h`, `ui/src/recorders.cpp`, `ui/qml/RecorderPalette.qml`, `ui/qml/RecordingIndicator.qml`, `tests/tst_ui_recorders.cpp`, `bench/scenarios/enregistreurs.json` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_recorders` (dont `indicatorVisibleWhileRecording`, `firstUseShowsTheNotice`, `hiddenPaletteComesBackOnRestart`, `settingsSectionRead`, `stopAlwaysAvailableUnderTheLock`), `tst_captures_enregistreurs` Passed ; `captures.ps1 -Scenario enregistreurs` : `expect` interne `recordingIndicator.visible` vrai pendant, `LU recordingIndicator.visible=false OK`, `LU appRecorders.lastDurationOk=true OK` (état final), `CAPTURES OK` ; banc hors écran joint (écriture pendant l'enregistrement) | S26 | #tache-l8-t28 |
| L8-T29 | `ui` : politique de contenu du cadre web (N1-A3) : JavaScript coupé, aucun `file://` hors des ressources du document, aucun chemin réseau, contenu distant d'un document reçu bloqué jusqu'au déblocage de la page | L8-T21 | `ui/include/actcraft/ui/webpolicy.h`, `ui/src/webpolicy.cpp`, `tests/tst_ui_webpolicy.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_webpolicy` (dont `javascriptOffByDefault`, `fileOutsideResourcesRefused`, `fileInsideResourcesAllowed`, `networkPathRefused`, `remoteBlockedInAReceivedDocument`, `unblockIsPerPageAndDocument`, `typedAddressIsNotReceived`, `htmlIsNeverPassedToQTextDocumentWithoutTheGuard`) Passed ; mutation (déblocage pour tout le document) rouge puis remise | S27 | #tache-l8-t29 |
| L8-T30 | Cadre web : saisie du code, vue redimensionnable, précédent, suivant, recharger, image fixe, repère de blocage, repère neutre dans les miniatures et les exports ; scénario `cadre-web` | L8-T29 | `ui/include/actcraft/ui/webframeitem.h`, `ui/src/webframeitem.cpp`, `ui/qml/WebFrameLayer.qml`, `ui/qml/WebFrameDialog.qml`, `app/main.cpp`, `bench/captures/main.cpp`, `bench/app/main.cpp`, `tests/tst_ui_webframe.cpp`, `bench/scenarios/cadre-web.json`, `bench/scenarios/fixtures/cadre-web.json` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_webframe` (dont `insertFromTypedHtml`, `resizeIsOneStep`, `receivedRemoteFrameSendsNoRequest`, `unblockThenRequestReachesTheServer`, `otherPageStaysBlocked`, `snapshotPlacesAnImage`, `roundTripKeepsTheFrame`, `paintedAsANeutralFrame`), `tst_render_painter`, `tst_captures_cadre_web` Passed ; `captures.ps1 -Scenario cadre-web` : `LU webFrameLayer.count=1 OK`, `LU webFrameLayer.blockedCount=1 OK`, `CAPTURES OK` | S27 | #tache-l8-t30 |
| L8-T31 | `core` + QML : diffusion protégée I — règles, copie protégée écrite dans un **nouveau** fichier, boîte « Diffuser en protégé » | L8-T21 | `core/include/actcraft/core/broadcast.h`, `core/src/broadcast.cpp`, `ui/qml/PublishDialog.qml`, `tests/tst_core_broadcast.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_core_broadcast` (dont `publishedCopyCarriesTheRules`, `originalIsUntouched`, `rulesApplyToTheAuthorToo`, `defaultRulesAreOmitted`, `categoryOfEveryRestrictableCommand`) Passed | S18 | #tache-l8-t31 |
| L8-T32 | Diffusion protégée II — porte dans le registre (`restrictable`), messages de refus, glisser vers les ressources refusé, actions toujours actives ; scénario `diffusion-protegee` | L8-T31 | `actions/include/actcraft/actions/commandregistry.h`, `actions/src/commandregistry.cpp`, `ui/include/actcraft/ui/broadcastgate.h`, `ui/src/broadcastgate.cpp`, `ui/src/resourcelibrary.cpp`, `ui/qml/Main.qml`, `tests/tst_ui_broadcast.cpp`, `bench/scenarios/diffusion-protegee.json`, `bench/scenarios/fixtures/diffusion-protegee.json` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_broadcast` (dont `restrictedCommandsGreyed`, `triggerShowsTheRefusal`, `enableCannotBypassTheRestriction`, `lockAndRestrictionCombine`, `resourceDragRefused`, `pageActionStillRuns`, `rulesFollowTheOpenDocument`), `tst_actions_lockable`, `tst_ui_resources`, `tst_captures_diffusion_protegee` Passed ; mutation (`isEnabled` sans la restriction) rouge puis remise ; `captures.ps1 -Scenario diffusion-protegee` : `LU appBroadcast.active=true OK`, `LU menuItem-edit.copy.enabled=false OK`, `CAPTURES OK` | S18 | #tache-l8-t32 |
| L8-T33 | Profil « Son et image » : épinglages du lot 8 (règle de l'amendement 1, § 1) ; liste des épinglages | L8-T21 | `actions/data/profiles/son-image.json`, `docs/verifications/2026-10-23-lot8-epinglages.md`, `tests/tst_ui_profiles.cpp` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_profiles` (dont `pinnedCommandsExist`, `pinnedFilteredByPlatform`, `soundAndImagePinsLot8`) Passed | S32 | #tache-l8-t33 |
| L8-T34 | Contrôle du lot : talons vides, suites debug et release, captures hors écran puis à l'écran, banc D3D11 plein écran, paquets, version 0.9.0, notes, mesure de fin de lot, graphe | L8-T20 à L8-T33 | `CMakeLists.txt`, `CHANGELOG.md`, `docs/mesures/2026-10-24-lot8-fin.md`, `tests/tst_ui_shell.cpp`, `graphify-out/` | partagé | implementeur-tdd | `TESTS OK` en debug et en release, N = N8 + 37 (+ p) ; `tst_ui_shell` (dont `lot8bControllersWired`) Passed ; `CAPTURES OK` hors écran et à l'écran (jeton `ecran`) ; banc `BUDGET TENU` ou tout critère de la référence tenu (porte de non-régression) ; `PAQUET OK`, `APK OK` (sous `GRADLE_OPTS=-Dorg.gradle.daemon=false`, main rendue < 60 s) ; tailles des paquets rapportées ; règle 4 des talons ; contrôle d'occurrences ; `NOMS OK` ; `graphify update .` commité | S11, S18, S25, S26, S27 | #tache-l8-t34 |
| L8-C1 | Repli seulement : retrait des commandes sans moteur, tests amendés dans le même commit, talons orphelins supprimés | L8-T19, refus ou silence sur H8-6 | `actions/data/commands.json`, `actions/data/layout.json`, `actions/data/object-actions.json`, `tests/tst_actions_layout.cpp`, `tests/tst_ui_shell.cpp`, `ui/qml/Main.qml`, `ui/qml/PageArea.qml`, `ui/qml/{SoundController,RecorderPalette,RecordingIndicator,WebFrameLayer,WebFrameDialog}.qml` (supprimés), `ui/CMakeLists.txt` | isolé | implementeur-tdd | `TESTS OK`, N = N8 + 21 ; `tst_actions_layout` (dont `lot8EntriesDeclared`, `lot8AttributesDeclared`), `tst_actions_registry`, `tst_actions_lockable`, `tst_actions_objectactions`, `tst_ui_shell` (dont `everyLot8CommandHasAHandler`, `lot8ShellPlaceholders`) Passed ; `premiers-pas` `CAPTURES OK` ; aucun talon de tâche retirée ; `NOMS OK` | S11 | #repli-sans-accord |
| L8-T21r | Repli seulement : réservation réduite de 8b (7 tests, sans module `media`) | L8-T19, L8-C1 | `core/CMakeLists.txt`, `ui/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/{core,ui,bench}.cmake`, talons : `core/include/actcraft/core/broadcast.h`, `core/src/broadcast.cpp`, `ui/include/actcraft/ui/{linkdialogs,mediainsert,mediaitems,broadcastgate}.h`, `ui/src/{linkdialogs,mediainsert,mediaitems,broadcastgate}.cpp`, 5 tests talons `tests/tst_{ui_linkdialogs,ui_mediainsert,ui_mediaitems,core_broadcast,ui_broadcast}.cpp`, 2 scénarios talons `bench/scenarios/{liens,diffusion-protegee}.json` | isolé | implementeur-tdd | `100% tests passed … out of N`, N = N8 + 28, `TESTS OK` ; 7 tests neufs `SKIP` nommant leur tâche (L8-T22, L8-T23, L8-T25r, L8-T31, L8-T32) ; aucun dossier `media/` ; `APK OK` sous `GRADLE_OPTS=-Dorg.gradle.daemon=false` (main rendue < 60 s) ; `NOMS OK` | S11, S18 | #repli-sans-accord |
| L8-T25r | Repli seulement : médias sur la page sans lecteur intégré (espace réservé, GIF animé, clic → application du système après confirmation) | L8-T16, L8-T21r, L8-T23 | `ui/include/actcraft/ui/mediaitems.h`, `ui/src/mediaitems.cpp`, `ui/qml/MediaLayer.qml`, `ui/qml/Main.qml`, `tests/tst_ui_mediaitems.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_mediaitems` (dont `clickAsksTheSystemWithConfirmation`, `playOnOpenNeverLaunches`, `gifAnimates`, `placeholderOnlyInTheCache`, `refusedWithoutCommands`) Passed ; banc hors écran joint | S11 | #repli-sans-accord |
| L8-T33r | Repli seulement : profil « Son et image », épinglages sans enregistreurs ni cadre web | L8-T21r | `actions/data/profiles/son-image.json`, `docs/verifications/2026-10-23-lot8-epinglages.md`, `tests/tst_ui_profiles.cpp` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_profiles` (dont `pinnedCommandsExist`, `soundAndImagePinsLot8` sur `link.file`, `link.website`, `insert.media`) Passed | S32 | #repli-sans-accord |


### Amendement 1 (relecture du tour 1)

Mêmes constats et même autorité que l'amendement 1 de 8a (table complète dans `…-lot8-PLAN.md`, sous la table des
tâches). Propres à 8b :

| # | Correction dans 8b | Où |
|---|---|---|
| B2 | `broadcast()` / `setBroadcast` de la classe `Document` | L8-T31 |
| B3 | repli réécrit : `L8-C1` (retrait et tests dans le même commit, talons orphelins supprimés), lignes `L8-T21r`, `L8-T25r`, `L8-T33r`, vagues du repli | « Porte H8-6 », table |
| M1, M4 | `menuItem-edit.copy.enabled` ; `appBroadcast`, `appRecorders` posés par leur tâche ; états intermédiaires en `expect` interne, état final en `-Lire` | L8-T25, L8-T28, L8-T32 |
| M2 | `Main.qml` : L8-T22 (V12), L8-T23 (V13), L8-T25 (V14), L8-T32 (V15) ; `appRecorders` dans `RecorderPalette.qml`, `appWebFrames` dans `WebFrameLayer.qml` ; `lot8bControllersWired` (L8-T34) | lignes, sections |
| M7 | réglages d'essai, `QStandardPaths::setTestModeEnabled` | « Commandes propres à 8b » |
| M10 | `QtWebView::initialize()` dans les trois `main` ; `main` propre à `tst_ui_webframe` ; saut nommé hors écran | L8-T30 |
| M11 | `GRADLE_OPTS=-Dorg.gradle.daemon=false`, main rendue < 60 s, `java.exe` du worktree comptés | « Commandes propres à 8b », L8-T21, L8-T21r, L8-T26, L8-T34 |
| m7 | garde HTML sur les fichiers du cadre | L8-T29 |
| m12 | `tool.camera-*` | L8-T31 |
| m14 | `pagepainter.cpp` retiré de L8-T30 (repère posé par L8-T3) | L8-T30, vagues |

15 tâches dans ce sous-lot. Tests ctest : L8-T21 enregistre les **16** tests de 8b (11 unitaires, 5 de captures) :
**N = N8 + 37** dès L8-T21 (N8 + 37 + p avec `L8-P1`) ; sans l'accord H8-6, **N8 + 28** (repli ci-dessus).

## Vagues (largeur 3 au plus ; 2 places de tests)

V10 : L8-T20 (porte H8-6) · V11 : L8-T21 · V12 : L8-T22 ‖ L8-T24 ‖ L8-T29 · V13 : L8-T23 ‖ L8-T26 ‖ L8-T31 ·
V14 : L8-T25 ‖ L8-T27 ‖ L8-T30 · V15 : L8-T28 ‖ L8-T32 ‖ L8-T33 · V16 : L8-T34.

Vérification des recoupements :
- V12 : L8-T22 (`linkdialogs.*`, `LinkDialog.qml`, `Main.qml`, un test, `liens.json` et son document) / L8-T24 (`media/player.*`,
  un test) / L8-T29 (`webpolicy.*`, un test) — disjoints.
- V13 : L8-T23 (`mediainsert.*`, `Main.qml`, un test) / L8-T26 (`soundrecorder.*`, `AndroidManifest.xml`, un test) / L8-T31
  (`core/broadcast.*`, `PublishDialog.qml`, un test) — disjoints.
- V14 : L8-T25 (`mediaitems.*`, `MediaLayer.qml`, `SoundController.qml`, `Main.qml`, un test, `medias.json`) / L8-T27
  (`displayrecorder*`, un test) / L8-T30 (`webframeitem.*`, `WebFrameLayer.qml`, `WebFrameDialog.qml`, `app/main.cpp`, `bench/captures/main.cpp`, `bench/app/main.cpp`,
  un test, `cadre-web.json`) — disjoints.
- V15 : L8-T28 (`recorders.*`, `RecorderPalette.qml`, `RecordingIndicator.qml`, un test, `enregistreurs.json`) /
  L8-T32 (`commandregistry.*`, `broadcastgate.*`, `resourcelibrary.cpp`, `Main.qml`, un test, `diffusion-protegee.json`) / L8-T33
  (`son-image.json`, rapport d'épinglages, `tst_ui_profiles.cpp`) — disjoints.
- Repris d'une vague à l'autre, tenus par « dépend de » : talons de L8-T21 → leur tâche ; `pagepainter.cpp` (L8-T3 → L8-T18, en 8a ; L8-T30 n'y touche plus) ; `commandregistry.*` (L6-T3 → L8-T32).

**Places.** Comme en 8a. L8-T20 ne construit que pour la vérification finale. Chemin critique : V12 L8-T24, V14 L8-T25.

## Commandes propres à 8b (amendement 1)

- **Réglages d'essai (M7)** : la règle de 8a (« Commandes de preuve communes ») vaut pour les tests de 8b qu'elle
  nomme, avec `QStandardPaths::setTestModeEnabled(true)` pour les enregistreurs (dossier des enregistrements).
- **APK (M11)** : toute construction d'APK (L8-T21, L8-T21r, L8-T26, L8-T34) se fait ainsi :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; `$env:GRADLE_OPTS='-Dorg.gradle.daemon=false'; `$t0=Get-Date; pwsh -NoProfile -File ./scripts/paquet-android.ps1 *> .tmp/apk.txt; `$t1=Get-Date; Select-String -Path .tmp/apk.txt -Pattern 'APK OK|ERREUR|FAILED' | Select-Object -Last 2; 'duree apres APK OK (s) : ' + [int](`$t1 - (Get-Item .tmp/apk.txt).LastWriteTime).TotalSeconds; @(Get-CimInstance Win32_Process -Filter \"Name='java.exe'\" | Where-Object { `$_.CommandLine -like '*<wt>*' }).Count"
  ```
  Attendu : `APK OK`, la commande rend la main moins de 60 s après `APK OK`, et **0** `java.exe` dont la ligne de
  commande contient le worktree (les autres `java.exe` du poste ne comptent pas).
- **Tests qui chargent `Main.qml`** : `lot8bControllersWired` va dans `tst_ui_shell` (forme de `tests/ui.cmake:33-36`
  déjà en place) ; aucun test neuf de 8b ne charge `Main.qml`.

## Sections des tâches

<a id="tache-l8-t20"></a>
### Tâche L8-T20 — Installation de Qt Multimedia et Qt WebView

**Objectif.** Installer, **après l'accord écrit de Jérôme (H8-6)**, les modules `qtmultimedia` et `qtwebview` de Qt
6.12.0 pour `win64_msvc2022_64`, `android_arm64_v8a` et `android_x86_64` ; les ajouter à la CI (Windows, macOS,
Android) et au flux de release ; mettre à jour la liste des bibliothèques autorisées et les notices (N1-A10) ;
mesurer les paquets avant et après.

**Fichiers.** Ceux de la ligne ; hors dépôt : `C:\Qt\6.12.0\…` (installation).

**Test et commande bornée.**
```powershell
python -m aqt install-qt windows desktop 6.12.0 win64_msvc2022_64 -m qtmultimedia qtwebview qtshadertools qtimageformats --outputdir C:\Qt *> .tmp/aqt.txt; Select-String -Path .tmp/aqt.txt -Pattern 'Finished installation|ERROR' | Select-Object -Last 2
```
(même commande pour les deux kits Android, sous-commande `android` et hôte de bureau, comme pour le lot 1) ; puis
`Test-Path C:\Qt\6.12.0\msvc2022_64\lib\cmake\Qt6Multimedia`, `…\Qt6WebView` (vrai), même chose pour les kits Android ;
`pwsh -NoProfile -File ./scripts/verifier-licences.ps1` sans écart ; `scripts/paquet.ps1` et `scripts/paquet-android.ps1`
avant (tête de 8a) et après (une construction de démonstration avec `ACTCRAFT_MEDIA=ON`, faite en L8-T21) : tailles au
rapport.

**Interdits.** **Ne pas commencer sans l'accord de Jérôme** (question posée par l'orchestrateur à la fin de L8-T0) ;
aucun autre téléchargement ; aucune bibliothèque sous GPL seule (Qt WebEngine refusé s'il y entraîne une licence
incompatible : le relever au rapport) ; aucun module ajouté à `packaging/qt-autorises.txt` sans sa licence lue.

**Terminé quand** les trois kits ont les deux modules, la CI les installe, les licences sont contrôlées, le rapport
`docs/verifications/2026-10-22-lot8-modules.md` donne versions, moteur de `WebView` sous Windows, licences (LGPLv3
de Qt Multimedia ; FFmpeg en LGPL tel que livré par Qt), tailles.

**Étapes.**
- [ ] **Étape 1 : accord.** Citer l'accord de Jérôme (date, texte) en tête du rapport ; sans lui, s'arrêter.
- [ ] **Étape 2 : installation** des trois kits (commande ci-dessus) ; contrôles `Test-Path`.
- [ ] **Étape 3 : CI** : ajouter `qtmultimedia qtwebview` à la liste `-m` des étapes d'installation de Qt dans
  `ci.yml` et `release.yml` (Windows, macOS, Android) ; `CLAUDE.md` : ligne d'installation du poste.
- [ ] **Étape 4 : licences** : `packaging/qt-autorises.txt` (bibliothèques et greffons déployés par `windeployqt` et
  par l'APK), `THIRD_PARTY_NOTICES.md` (Qt Multimedia, FFmpeg, Qt WebView) ; `verifier-licences.ps1`.
- [ ] **Étape 5 : rapport et commit** « Lot 8 : modules Qt Multimedia et Qt WebView installés (L8-T20) » ; `NOMS OK` ;
  titres vides.

<a id="tache-l8-t21"></a>
### Tâche L8-T21 — Réservation 8b

**Objectif.** Même rôle que L8-T1 pour 8b : module `media/` (`actcraft_add_module(media … LIBS actcraft_core
Qt6::Multimedia)`, déclaré après `core` et avant `ui`, construit si `ACTCRAFT_MEDIA` — défaut : vrai si
`Qt6Multimedia` est trouvé), option `ACTCRAFT_WEBFRAME` (défaut : vrai si `Qt6WebView` est trouvé ; `ui` lie
`Qt6::WebView`). `QtWebView::initialize()` doit être appelé avant la création de `QGuiApplication` : L8-T21 ne
le pose pas : L8-T30, qui a `app/main.cpp` dans sa ligne, place l'appel sous `#if ACTCRAFT_WEBFRAME` avant
`QGuiApplication` ; 16 tests talons ; fichiers talons ; documents d'essai vides.

**Fichiers.** Ceux de la ligne.

**Test et commande bornée.** Preuve de tâche : N = N8 + 37, `TESTS OK` ; `tst_modules` Passed ; les 16 tests neufs en
`SKIP` « Talon L8 : rempli par L8-T<n>. » ; `scripts/paquet-android.ps1` : `APK OK` (le module se construit pour
Android).

**Interdits.** Aucun comportement ; `media/` ne dépend que de `core` et de Qt (aucun `ui/`) ; `actcraft_forbid_links`
inchangé pour `core/`.

**Terminé quand** N = N8 + 37, `TESTS OK`, `APK OK`, `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : CMake** (racine : `add_subdirectory(media)` après `core`, sous `if(ACTCRAFT_MEDIA)` ; options ;
  `tst_modules` gagne `media`).
- [ ] **Étape 2 : talons** (même forme qu'en L8-T1 ; marqueur `Talon L8 : rempli par L8-T<n>.` ; table : `linkdialogs.*`
  → L8-T22, `mediainsert.*` → L8-T23, `player.*` → L8-T24, `mediaitems.*` → L8-T25, `soundrecorder.*` → L8-T26,
  `displayrecorder*` → L8-T27, `recorders.*` → L8-T28, `webpolicy.*` → L8-T29, `webframeitem.*` → L8-T30,
  `core/broadcast.*` → L8-T31, `broadcastgate.*` → L8-T32 ; scénarios : `liens` → L8-T22, `medias` → L8-T25,
  `enregistreurs` → L8-T28, `cadre-web` → L8-T30, `diffusion-protegee` → L8-T32).
- [ ] **Étape 3 : tests talons** enregistrés : `tests/media.cmake` (`tst_media_player`, `tst_media_soundrecorder`,
  `tst_media_displayrecorder`), `tests/core.cmake` (`tst_core_broadcast`), `tests/ui.cmake` (`tst_ui_linkdialogs`,
  `tst_ui_mediainsert`, `tst_ui_mediaitems`, `tst_ui_recorders`, `tst_ui_webpolicy`, `tst_ui_webframe`,
  `tst_ui_broadcast`), `tests/bench.cmake` (5 captures). 11 + 5 = **16**.
- [ ] **Étape 4 : vert** ; `APK OK` ; commit « Lot 8 : réservation de 8b, module media (L8-T21) » ; `NOMS OK`.

**Amendement 1 (L8-T21).** M11 : construction de l'APK par la commande de « Commandes propres à 8b ». m7 : `tst_ui_webpolicy`
est enregistré avec `ENVIRONMENT ACTCRAFT_SOURCE_DIR=${CMAKE_SOURCE_DIR}`. M10 : `tst_ui_webframe` est enregistré avec
son propre `main` (pas de `QTEST_MAIN`, voir L8-T30).

<a id="tache-l8-t22"></a>
### Tâche L8-T22 — Dialogues de liens

**Objectif.** F-550 et F-551 par la boîte `LinkDialog.qml` et son contrôleur `ui::LinkDialogs` : cible (fichier ou
site), forme du lien (texte, icône, objet d'action, objet existant choisi sur la page), stockage d'un fichier
(externe : chemin gardé ; dans le document : ressource à son SHA-256), image d'espace réservé ; création en **une**
étape ; ouverture au clic par L8-T9 et L8-T10 (confirmation). Les « paramètres de ligne de commande » de la
référence sont gardés dans les paramètres de l'action mais **jamais passés** (aucun `QProcess`, H8-3).

**Fichiers.** Ceux de la ligne.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_linkdialogs|tst_captures_liens'`.

**Preuve visuelle.** `LU linkConfirmDialog.visible=true OK`, `LU appLinks.openedCount=0 OK`, `CAPTURES OK` ; décrire
la capture `confirmation`.

**Interdits.** Aucun lancement ; refus d'une adresse qui n'est pas `http` ou `https` à la saisie (même règle que
`LinkLauncher::refusalFor`) ; fermé par défaut et refus sous le verrou.

**Terminé quand** les sept fonctions et le scénario passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_ui_linkdialogs.cpp` : `textFormCreatesATextObject` (lien vers un site, forme
  texte : un objet texte portant `document.open-website` avec `{"url":…}`) ; `existingObjectGetsTheAction` ;
  `storedFileBecomesAResource` (la ressource est dans le document, l'action porte sa clé) ; `externalPathKeptAsIs` ;
  `commandLineArgumentsNeverPassed` (paramètres gardés, faux ouvreur appelé sans eux) ; `websiteNeedsHttpOrHttps` ;
  `oneStep`.
- [ ] **Étape 2 : code** (`LinkDialogs::createFileLink(QUrl, int form, int storage, quint64 existing)`,
  `createWebsiteLink(QString, int form, quint64 existing)`) ; QML ; gestionnaires `link.file` et `link.website`.
- [ ] **Étape 3 : scénario** `liens.json` : `{"command": "link.website", "argument": "https://example.org/"}`,
  forme texte validée, clic sur le texte créé, `expect` `linkConfirmDialog.visible` vrai, `key` `Escape`, `expect`
  `appLinks.openedCount` 0, capture `confirmation`.
- [ ] **Étape 4 : vert** ; commit « Lot 8 : dialogues de liens (L8-T22) » ; `NOMS OK` ; titres vides.

**Amendement 1 (L8-T22).** M2 : `ui/qml/Main.qml` gagne
`LinkDialogs { id: appLinkDialogs; objectName: "appLinkDialogs"; commands: appCommands; document: appDocument; toolHost: appTools; links: appLinks }`.

<a id="tache-l8-t23"></a>
### Tâche L8-T23 — Insertion des médias

**Objectif.** Un son ou une vidéo (fichier, ressource glissée, commande `insert.media` du lot 5, A33) devient un
`core::Media` à image d'espace réservée (à nous : icône de son ou première image de la vidéo, calculée hors du fil
principal) portant l'action `document.play-sound` (F-552) ; un second média se pose à côté du premier (V126, D13) ;
un objet Flash lu (`.swf`, `.flv`) n'est montré que par son espace réservé (N1-A15, D15).

**Fichiers.** Ceux de la ligne.

**Test et commande bornée.** Commande ciblée `-R tst_ui_mediainsert`.

**Interdits.** Aucune lecture ici (L8-T24) ; aucune image de l'original ; une étape par insertion ; décodage de la
première image sur `QThreadPool`.

**Terminé quand** les six fonctions passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_ui_mediainsert.cpp` : `soundBecomesAPlaceholderWithAPlayAction` (un WAV inséré : un `core::Media` portant `document.play-sound`, une étape) ; `videoKeepsItsAspect` (vidéo 16:9 : boîte au même rapport) ; `secondMediaGoesBesideTheFirst` (le second à droite du premier, sans recouvrement) ; `resourceDropInsertsInOneStep` ; `flashShowsOnlyItsPlaceholder` (`.swf` : espace réservé, aucune action de lecture) ; `refusedWhileLocked` ( WAV de 0,5 s écrit par le test, en-tête RIFF à la main ;
  vidéo : sautée avec `QSKIP("ACTCRAFT_MEDIA absent")` si le module manque, jamais « Talon »).
- [ ] **Étape 2 : code** `MediaInsert::insertFile(QUrl)`, `insertResource(QString key)` ; placement : à droite du
  dernier média de la page, sinon au centre. **Étape 3 : vert** ; commit « Lot 8 : insertion des médias (L8-T23) ».

**Amendement 1 (L8-T23).** M2 : `ui/qml/Main.qml` gagne
`MediaInsert { id: appMediaInsert; objectName: "appMediaInsert"; commands: appCommands; document: appDocument }`.

<a id="tache-l8-t24"></a>
### Tâche L8-T24 — `media` : lecteur intégré

**Objectif.** `media::Player` : deux pistes simultanées (piste 1 et piste 2, F-552 ; un son de la même piste remplace
le précédent), lecture par `QMediaPlayer` + `QAudioOutput` (A31) sur leur propre fil de Qt Multimedia, jamais sur le
fil de rendu ; un format illisible bascule vers l'application du système **par `LinkLauncher`** (confirmation, N2-9) ;
arrêt de tout à la fermeture du document. Le patron évident est le mauvais : appeler `QDesktopServices` depuis
`media/` (contournerait la confirmation) ou bloquer le fil principal en attendant `LoadedMedia`.

**Fichiers.** Ceux de la ligne.

**Test et commande bornée.** Commande ciblée `-R tst_media_player`.

**Interdits.** `media/` n'inclut rien de `ui/` : le repli passe par un rappel injecté (`std::function<void(QString path)>`
posé par `ui/`) ; aucune attente bloquante (signaux seulement) ; aucun fichier écrit hors du dossier temporaire du
document.

**Terminé quand** les sept fonctions passent.

**Étapes.**
- [ ] **Étape 1 : en-tête** :
  ```cpp
  #pragma once
  #include <QObject>
  #include <QString>

  #include <functional>

  namespace actcraft::media {
  class Player : public QObject
  {
      Q_OBJECT
  public:
      enum Track { First = 0, Second = 1 };
      explicit Player(QObject* parent = nullptr);
      void setFallback(std::function<void(const QString& path)> fallback); // ui/ : LinkLauncher::SystemMedia
      void play(const QString& path, Track track, bool loop);
      void pause(Track track);
      void stopAll();
      qint64 position(Track track) const;
      bool isPlaying(Track track) const;
  signals:
      void positionChanged(int track, qint64 position);
      void finished(int track);
      void fellBack(const QString& path);
  };
  } // namespace actcraft::media
  ```
- [ ] **Étape 2 : test rouge** `tests/tst_media_player.cpp` (WAV synthétique de 1 s écrit par le test ; sortie audio
  factice si aucun périphérique : `QSKIP("aucune sortie audio")` dans les seules fonctions qui écoutent) :
  `wavPlaysAndPositionAdvances` (`QTRY_VERIFY(player.position(Player::First) > 0)`) ; `twoTracksAtOnce` ;
  `secondSoundOnTrackOneReplacesTheFirst` ; `unreadableFallsBackToTheSystem` (fichier texte renommé `.mp3` :
  `fellBack` émis, rappel appelé une fois) ; `fallbackNeedsConfirmation` (le rappel posé par un `LinkLauncher` réel à
  faux ouvreur : aucune ouverture sans `answer`) ; `neverOnTheRenderThread` (`QThread::currentThread()` des rappels =
  fil du test) ; `stopAllOnDocumentClose`.
- [ ] **Étape 3 : code.** **Étape 4 : vert** ; commit « Lot 8 : lecteur intégré (L8-T24) » ; `NOMS OK`.

<a id="tache-l8-t25"></a>
### Tâche L8-T25 — QML : médias sur la page ; scénario `medias`

**Objectif.** S25 : une vidéo joue **dans son cadre** au-dessus du cache (élément `VideoOutput` dans `MediaLayer.qml`,
placé par `view.pageToScene`, jamais versé dans le cache : la page fixe ne dessine que l'espace réservé) pendant
qu'on écrit au stylet ; le lecteur de sons (`SoundController.qml`, commande `media.sound-controller`) montre la
position qui avance ; options F-582 (`controlsHidden`, `loop`, `aspectLocked`, `clearBackground`, `wrapText`) ;
« lecture auto » (`playOnOpen`) ne vaut que pour le lecteur intégré, à l'affichage de la page, jamais à l'ouverture
d'un fichier par le système ; GIF animé par `AnimatedImage` (V57, D14) ; son de confirmation des conteneurs
(`containerAccepted` de L8-T16, `chime`) joué sur la piste 2.

**Fichiers.** Ceux de la ligne.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_mediaitems|tst_captures_medias'` ; banc hors écran joint,
puis banc D3D11 à l'écran en fin de lot (L8-T34) avec la vidéo qui joue (S25 : aucune régression des critères).

**Preuve visuelle.** `waitFor` interne sur `soundController.playing` vrai (jamais en `-Lire`), puis en `-Lire` l'état final `LU soundController.visible=true OK`, `PIXEL gif-image-2 OK`,
`CAPTURES OK` ; décrire les captures `video`, `lecteur`, `gif`.

**Interdits.** Aucun dessin de la vidéo dans le cache ni dans les miniatures (espace réservé seulement) ; aucune
lecture pendant un contact qui démarre une vidéo (le clic sur un média passe par L8-T9, au relâché) ; `pageArea.view`
dans le QML.

**Terminé quand** les huit fonctions et le scénario passent, et le JSON du banc est joint.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_ui_mediaitems.cpp` : `videoIsAboveTheCache` (l'élément vidéo est enfant de `pageOverlays`, au-dessus de la page fixe) ; `videoNeverEntersTheCache` ; `controlsHiddenOption` (commandes invisibles si `controlsHidden`) ; `loopOption` (fin de lecture : reprise) ; `playOnOpenOnlyInternal` (`playOnOpen` démarre le lecteur intégré à l'affichage de la page ; jamais `LinkLauncher`) ; `gifAnimates` (deux images différentes à 600 ms d'écart) ; `chimePlaysOnAcceptedDrop` (`containerAccepted` avec `chime` : piste 2 lancée) ; `soundControllerToggles` (`media.sound-controller` montre puis masque le lecteur) ( `videoNeverEntersTheCache` :
  `QSignalSpy` sur la demande de redessin de la page fixe pendant 2 s de lecture → 0).
- [ ] **Étape 2 : code** (`MediaItems` : modèle des médias visibles de la page courante, regroupé, reporté pendant un
  contact ; `MediaLayer.qml` ; `SoundController.qml`).
- [ ] **Étape 3 : scénario** `medias.json` : fixture (un son WAV, une vidéo courte écrite par nous, un GIF de deux
  images à nous) ; clic sur le son, `waitFor` `soundController.playing` vrai ; clic sur la vidéo, trait au stylet
  pendant la lecture (`stroke`), `checkPixel` du trait ; `wait` 600, `checkPixel` `gif-image-2` ; captures.
- [ ] **Étape 4 : vert** ; banc ; commit « Lot 8 : médias sur la page (L8-T25) » ; `NOMS OK`.

**Amendement 1 (L8-T25).** M2 : `ui/qml/Main.qml` gagne
`MediaItems { id: appMediaItems; objectName: "appMediaItems"; document: appDocument; contactActive: pageArea.view.contactActive; links: appLinks }`
(le `media::Player` appartient à `MediaItems`). M4 : `soundController.playing` lu par un `waitFor` interne ; en
`-Lire`, `soundController.visible=true`. M7 : réglages d'essai.

<a id="tache-l8-t26"></a>
### Tâche L8-T26 — `media` : enregistreur de son

**Objectif.** F-293 : `media::SoundRecorder` (`QMediaCaptureSession`, `QAudioInput`, `QMediaRecorder`, format WAV) ;
enregistrer, second appui = pause puis reprise, arrêt ; à l'arrêt, le fichier devient une ressource et une icône de
lecture est posée sur la page (une étape, par `MediaInsert` de L8-T23 à travers un rappel) ; sous Android, permission
du micro demandée à la première utilisation (`QMicrophonePermission`, N1-A2) ; refusée : la commande se désactive avec
un message, sans plantage.

**Fichiers.** Ceux de la ligne (`android/AndroidManifest.xml` : `android.permission.RECORD_AUDIO`).

**Test et commande bornée.** Commande ciblée `-R tst_media_soundrecorder` ; `scripts/paquet-android.ps1` (`APK OK`) ;
`scripts/essai-android.ps1 -Apk <…-x86_64.apk>` (`ANDROID OK`) — si l'émulateur manque, l'essai passe en CI.

**Interdits.** Aucun envoi réseau ; le fichier reste dans le dossier temporaire du document jusqu'à son versement en
ressource ; indicateur d'enregistrement toujours visible (L8-T28).

**Terminé quand** les six fonctions passent (entrée audio absente : `QSKIP("aucune entrée audio")` dans les seules
fonctions qui enregistrent ; `refusedPermissionDisablesWithAMessage` passe partout par une permission simulée).

**Étapes.** Test rouge `tests/tst_media_soundrecorder.cpp` : `writesAWav` (en-tête RIFF/WAVE, durée ≥ 2,5 s pour 3 s) ; `pauseAndResume` (la pause n'entre pas dans la durée) ; `stopPlacesAPlayIconOnThePage` (rappel d'insertion appelé une fois) ; `cancelLeavesNothing` (aucun fichier, aucun rappel) ; `refusedPermissionDisablesWithAMessage` ; `indicatorStateFollows` (`recording` vrai pendant, faux après) ; code ; manifeste ; vert ; `APK OK` ; commit « Lot 8 : enregistreur
de son (L8-T26) » ; `NOMS OK`.

**Amendement 1 (L8-T26).** M11 : APK par la commande de « Commandes propres à 8b ». M7 : réglages d'essai et
`QStandardPaths::setTestModeEnabled(true)`.

<a id="tache-l8-t27"></a>
### Tâche L8-T27 — `media` : enregistreur d'écran (Windows)

**Objectif.** F-292 : `media::DisplayRecorder` (`QScreenCapture` de l'écran principal, `QMediaRecorder` en MP4, N2-A9) ;
plein écran ou zone (recadrage de la zone : moyen relevé par L8-T20 sur Qt 6.12 — recadrage des images par
`QVideoSink` avant l'encodeur, ou capture de fenêtre ; écrit au rapport) ; pause, arrêt, abandon (le fichier est
supprimé) ; `displayrecorder_other.cpp` rend « indisponible » hors Windows et les commandes sont absentes (plateformes
de L8-T1).

**Fichiers.** Ceux de la ligne.

**Test et commande bornée.** Commande ciblée `-R tst_media_displayrecorder` (garde `#ifdef Q_OS_WIN` ; ailleurs,
`QSKIP("Windows seulement")`).

**Interdits.** Aucune fenêtre visible pendant les tests (capture hors écran de l'écran principal seulement, durée ≤ 5 s,
annoncée d'une ligne à Jérôme) ; aucun envoi ; aucun fichier hors `.tmp/`.

**Terminé quand** les cinq fonctions passent sous Windows ; `fullScreenFiveSeconds` : fichier présent, durée 5 s ± 0,5
(lue par `QMediaPlayer::duration`).

**Étapes.** Test rouge `tests/tst_media_displayrecorder.cpp` : `fullScreenFiveSeconds` ; `areaIsCropped` (zone de 320 × 240 : images du fichier à cette taille) ; `pauseExcludesThePause` (5 s dont 2 s de pause : durée 3 s ± 0,5) ; `cancelDeletesTheFile` ; `notBuiltOutsideWindows` (hors Windows, `isAvailable()` faux) ; code ; vert ; commit « Lot 8 : enregistreur d'écran (L8-T27) » ; `NOMS OK`.

<a id="tache-l8-t28"></a>
### Tâche L8-T28 — Enregistreurs : palettes, indicateur, mention ; scénario `enregistreurs`

**Objectif.** S26 et N1-A16 : `ui::Recorders` (`QML_ELEMENT`, `appRecorders`) relie les deux enregistreurs aux
commandes `record.*` ; `RecorderPalette.qml` (enregistrer, pause, arrêter, abandonner) ; `RecordingIndicator.qml`
visible pendant **tout** enregistrement, au-dessus de tout (fenêtre à part pour l'écran : elle n'entre pas dans
l'image enregistrée si Qt le permet, sinon elle y entre et c'est écrit au rapport) ; mention à la première
utilisation (ce qui est enregistré, où le fichier est rangé, rien n'est envoyé) ; relancer un enregistrement d'écran
fait réapparaître une palette masquée (V43) ; réglages de la section « Enregistrements » (A30) ; `record.stop` et
`record.cancel` restent libres sous le verrou (L8-T1).

**Fichiers.** Ceux de la ligne.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_recorders|tst_captures_enregistreurs'` ; banc hors écran
joint (écriture au stylet pendant un enregistrement de son).

**Preuve visuelle.** `expect` interne `recordingIndicator.visible` vrai pendant l'enregistrement (jamais en `-Lire`), puis en `-Lire` l'état final `LU recordingIndicator.visible=false OK`, `LU appRecorders.lastDurationOk=true OK` (durée 5 s ± 0,5), `CAPTURES OK` ; décrire les captures `indicateur` et `icone-son`.

**Interdits.** Aucun enregistrement sans indicateur ; aucun enregistrement démarré par un document ou une action sans
clic de l'utilisateur (une action `command.record.sound` reste un clic) ; `pageArea.view` dans le QML.

**Terminé quand** les cinq fonctions et le scénario passent.

**Étapes.** Test rouge `tests/tst_ui_recorders.cpp` : `indicatorVisibleWhileRecording` ; `firstUseShowsTheNotice` (mention une seule fois, gardée dans les réglages d'essai) ; `hiddenPaletteComesBackOnRestart` (V43) ; `settingsSectionRead` (dossier et format lus de la section « Enregistrements ») ; `stopAlwaysAvailableUnderTheLock` ; code et QML ; scénario `enregistreurs.json` (son de 3 s : indicateur visible, icône posée ;
sous Windows, zone de 5 s : indicateur visible pendant toute la durée, fichier de 5 s ± 0,5) ; vert ; banc ; commit
« Lot 8 : palettes et indicateur des enregistreurs (L8-T28) » ; `NOMS OK` ; titres vides.

**Amendement 1 (L8-T28).** M1, M2 : l'instance est posée **dans `RecorderPalette.qml`** (fichier de la tâche, aucune autre
tâche de V15 n'y touche) : `Recorders { id: appRecorders; objectName: "appRecorders"; commands: appCommands; document: appDocument; mediaInsert: appMediaInsert }`
; propriété `lastDurationOk` (vrai si la dernière durée est dans 5 s ± 0,5). M4 : indicateur lu pendant par un
`expect` interne ; en `-Lire`, `recordingIndicator.visible=false` et `appRecorders.lastDurationOk=true`. M7 :
réglages d'essai (mention « une seule fois »).

<a id="tache-l8-t29"></a>
### Tâche L8-T29 — `ui` : politique de contenu du cadre web

**Objectif.** Toute la politique de N1-A3, en logique pure testable sans moteur web : `ui::WebContentPolicy` décide,
pour une adresse demandée par un cadre, « chargée », « refusée » ou « bloquée jusqu'au déblocage ». Le patron évident
est le mauvais : régler la politique dans le QML de la vue (non testable, contournable par une redirection) ; ici,
chaque requête de la vue passe par `decide` (A32 : interception relevée par L8-T20 ; à défaut, le code HTML est
réécrit avant chargement pour retirer toute ressource refusée).

**Fichiers.** Ceux de la ligne.

**Test et commande bornée.** Commande ciblée `-R tst_ui_webpolicy`. Mutation : enregistrer le déblocage pour le
document entier → `unblockIsPerPageAndDocument` rouge, remise par `git checkout -- ui/src/webpolicy.cpp`.

**Interdits.** JavaScript coupé par défaut, aucune option pour l'activer dans ce lot ; aucun `file://` hors du dossier
de ressources du document ; aucun chemin réseau (`\\hôte`, `file://hôte/`) ; aucun contenu distant sans une adresse
**saisie par l'utilisateur dans cette session** ; un document **reçu** (ouvert depuis un fichier, pas créé dans la
session) dont un cadre embarque du distant l'ouvre bloqué ; le HTML d'un cadre ne passe jamais par un
`QTextDocument` sans le garde `loadResource` (D8-5).

**Terminé quand** les huit fonctions passent et la mutation a été vue rouge.

**Étapes.**
- [ ] **Étape 1 : en-tête** :
  ```cpp
  #pragma once
  #include <QHash>
  #include <QSet>
  #include <QString>
  #include <QUrl>

  namespace actcraft::ui {
  class WebContentPolicy
  {
  public:
      enum class Verdict { Load, Refuse, Blocked };
      void setResourceFolder(const QString& folder);   // dossier des ressources du document ouvert
      void setReceived(bool received);                 // vrai : document ouvert depuis un fichier
      void typedByUser(const QUrl& address);           // adresse saisie dans la boîte du cadre (session)
      void unblock(const QString& documentKey, int page);
      Verdict decide(const QUrl& url, const QString& documentKey, int page) const;
      static bool javascriptEnabled() noexcept { return false; }
  private:
      QString m_folder;
      bool m_received = false;
      QSet<QString> m_typed;
      QSet<QString> m_unblocked; // « clé#page »
  };
  } // namespace actcraft::ui
  ```
- [ ] **Étape 2 : test rouge** `tests/tst_ui_webpolicy.cpp` :
  ```cpp
  #include <actcraft/ui/webpolicy.h>

  #include <QTemporaryDir>
  #include <QtTest>

  using actcraft::ui::WebContentPolicy;
  using V = WebContentPolicy::Verdict;

  class TstUiWebPolicy : public QObject
  {
      Q_OBJECT
  private slots:
      void javascriptOffByDefault() { QVERIFY(!WebContentPolicy::javascriptEnabled()); }
      void fileOutsideResourcesRefused()
      {
          QTemporaryDir dir;
          WebContentPolicy p;
          p.setResourceFolder(dir.path());
          QCOMPARE(p.decide(QUrl::fromLocalFile(QStringLiteral("C:/Windows/win.ini")), QStringLiteral("d"), 0), V::Refuse);
          QCOMPARE(p.decide(QUrl::fromLocalFile(dir.filePath(QStringLiteral("../x.png"))), QStringLiteral("d"), 0), V::Refuse);
      }
      void fileInsideResourcesAllowed()
      {
          QTemporaryDir dir;
          WebContentPolicy p;
          p.setResourceFolder(dir.path());
          QCOMPARE(p.decide(QUrl::fromLocalFile(dir.filePath(QStringLiteral("a.png"))), QStringLiteral("d"), 0), V::Load);
      }
      void networkPathRefused()
      {
          WebContentPolicy p;
          QCOMPARE(p.decide(QUrl(QStringLiteral("file://serveur/partage/a.png")), QStringLiteral("d"), 0), V::Refuse);
          QCOMPARE(p.decide(QUrl::fromLocalFile(QStringLiteral("//serveur/partage/a.png")), QStringLiteral("d"), 0), V::Refuse);
      }
      void remoteBlockedInAReceivedDocument()
      {
          WebContentPolicy p;
          p.setReceived(true);
          QCOMPARE(p.decide(QUrl(QStringLiteral("http://127.0.0.1:8765/")), QStringLiteral("d"), 2), V::Blocked);
      }
      void unblockIsPerPageAndDocument()
      {
          WebContentPolicy p;
          p.setReceived(true);
          p.unblock(QStringLiteral("d"), 2);
          QCOMPARE(p.decide(QUrl(QStringLiteral("http://127.0.0.1:8765/")), QStringLiteral("d"), 2), V::Load);
          QCOMPARE(p.decide(QUrl(QStringLiteral("http://127.0.0.1:8765/")), QStringLiteral("d"), 3), V::Blocked);
          QCOMPARE(p.decide(QUrl(QStringLiteral("http://127.0.0.1:8765/")), QStringLiteral("e"), 2), V::Blocked);
      }
      void typedAddressIsNotReceived()
      {
          WebContentPolicy p;
          p.setReceived(true);
          p.typedByUser(QUrl(QStringLiteral("https://example.org/")));
          QCOMPARE(p.decide(QUrl(QStringLiteral("https://example.org/")), QStringLiteral("d"), 0), V::Load);
          QCOMPARE(p.decide(QUrl(QStringLiteral("https://autre.example/")), QStringLiteral("d"), 0), V::Blocked);
      }
      void htmlIsNeverPassedToQTextDocumentWithoutTheGuard()
      {
          // m7 : balaye les fichiers qui manient le HTML d'un cadre (écrits par L8-T30) ; rouge dès qu'un setHtml ou un
          // Text.RichText y apparaît (mutation de L8-T30). ACTCRAFT_SOURCE_DIR : posé à l'enregistrement par L8-T21.
          for (const char* rel : {"/ui/src/webframeitem.cpp", "/ui/qml/WebFrameLayer.qml", "/ui/qml/WebFrameDialog.qml"}) {
              QFile f(qEnvironmentVariable("ACTCRAFT_SOURCE_DIR") + QString::fromLatin1(rel));
              QVERIFY2(f.open(QIODevice::ReadOnly), rel);
              const QString source = QString::fromUtf8(f.readAll());
              QVERIFY2(!source.contains(QStringLiteral("setHtml")), rel);
              QVERIFY2(!source.contains(QStringLiteral("RichText")), rel);
          }
      }
  };

  QTEST_MAIN(TstUiWebPolicy)
  #include "tst_ui_webpolicy.moc"
  ```
  Hors document reçu, un contenu distant n'est chargé que s'il a été saisi (`typedByUser`) ; une page web
  `http`/`https` saisie est la seule voie distante.
- [ ] **Étape 3 : code** (chemins comparés après `QDir::cleanPath` et `QFileInfo::canonicalFilePath` ; schémas permis :
  `file` local dans le dossier, `http`, `https`, `about:blank`, `data:` d'images seulement ; tout le reste refusé).
- [ ] **Étape 4 : vert**, mutation rouge puis remise ; commit « Lot 8 : politique de contenu du cadre web (L8-T29) ».

**Amendement 1 (L8-T29).** m7 : `htmlIsNeverPassedToQTextDocumentWithoutTheGuard` balaye `ui/src/webframeitem.cpp`,
`ui/qml/WebFrameLayer.qml` et `ui/qml/WebFrameDialog.qml` (talons à L8-T29, écrits par L8-T30) ; L8-T30 joue la
mutation (un `setHtml` ajouté dans `webframeitem.cpp` → rouge, remise par `git checkout --`).

<a id="tache-l8-t30"></a>
### Tâche L8-T30 — Cadre web ; scénario `cadre-web`

**Objectif.** S27 : `insert.web-frame` ouvre `WebFrameDialog.qml` (saisie du code HTML ou d'une adresse) et crée un
`core::WebFrame` en une étape ; `WebFrameLayer.qml` pose une vue `WebView` (A32) par cadre visible de la page,
redimensionnable (une étape au relâché), avec précédent, suivant, recharger ; chaque requête passe par
`WebContentPolicy` (L8-T29) ; un cadre bloqué montre un repère et le bouton « Débloquer » (`webframe.unblock`,
confirmation, cette page de ce document seulement) ; « Poser une image du cadre » (`webframe.snapshot`, moyen de D12 :
capture de la zone de la fenêtre, puisque la vue native échappe à `grabToImage`, N1-A3) crée une image sur la page ;
dans les miniatures, les exports et le rendu logiciel, le cadre est un repère neutre (cadre et titre), dessiné par
`paintPage` ; comportement sous le rideau : celui écrit par L8-T0 (D12 ; défaut : la vue est masquée tant que le
rideau ou le spot couvrent sa zone).

**Fichiers.** Ceux de la ligne.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_webframe|tst_render_painter|tst_captures_cadre_web'`.

**Preuve visuelle.** `LU webFrameLayer.count=1 OK`, `LU webFrameLayer.blockedCount=1 OK`, `CAPTURES OK` ; décrire les
captures `cadre`, `bloque`, `image-fixe`.

**Interdits.** JavaScript coupé (`settings.javaScriptEnabled: false`, `settings.allowFileAccess: false`,
`settings.localContentCanAccessFileUrls: false`, A32) ; aucune requête avant la décision de la politique ; aucun
dessin de la vue dans le cache ; `pageArea.view` dans le QML.

**Terminé quand** les huit fonctions et le scénario passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_ui_webframe.cpp` : serveur local d'essai `QTcpServer` sur 127.0.0.1, port
  libre, qui compte les requêtes ; `receivedRemoteFrameSendsNoRequest` (document « reçu » dont le cadre pointe vers
  le serveur : 0 requête après 1 s, `blockedCount == 1`) ; `unblockThenRequestReachesTheServer` (après `answer(true)`,
  `QTRY_VERIFY(requests >= 1)`) ; `otherPageStaysBlocked` ; `insertFromTypedHtml` ; `resizeIsOneStep` ;
  `snapshotPlacesAnImage` ; `roundTripKeepsTheFrame` (`.actcraft` enregistré puis rouvert) ;
  `paintedAsANeutralFrame` (`paintPage` : cadre dessiné, aucun contenu web) ; sans moteur (`ACTCRAFT_WEBFRAME`
  faux) : `QSKIP("ACTCRAFT_WEBFRAME absent")` dans les fonctions qui montrent une vue.
- [ ] **Étape 2 : code** (`WebFrameItems` : modèle des cadres de la page, `blockedCount`, `count` ; `WebFrameLayer.qml` ;
  `WebFrameDialog.qml` ) ; le repère neutre de `paintPage` est posé par L8-T3 (chaîne de `get_if` de `paintObjectLocal`) : L8-T30 ne touche pas au peintre.
- [ ] **Étape 3 : scénario** `cadre-web.json` (fixture : HTML local d'essai à nous ; puis un document « reçu » pointant
  vers une adresse locale) ; lectures ; captures.
- [ ] **Étape 4 : vert** ; banc hors écran ; commit « Lot 8 : cadre web (L8-T30) » ; `NOMS OK`.

**Amendement 1 (L8-T30).**
- **M10.** `QtWebView::initialize()` est appelé avant `QGuiApplication` dans `app/main.cpp`, `bench/captures/main.cpp`
  et `bench/app/main.cpp` (tous trois dans la ligne), sous `#if ACTCRAFT_WEBFRAME`. `tst_ui_webframe` a son propre
  `main` :
  ```cpp
  int main(int argc, char** argv)
  {
  #if ACTCRAFT_WEBFRAME
      QtWebView::initialize();
  #endif
      QGuiApplication app(argc, argv);
      TstUiWebFrame test;
      return QTest::qExec(&test, argc, argv);
  }
  ```
  Hors écran (`QT_QPA_PLATFORM=offscreen`), si la vue native ne se crée pas : `QSKIP("moteur web indisponible hors écran")`
  dans les seules fonctions qui montrent une vue ; `receivedRemoteFrameSendsNoRequest`, `otherPageStaysBlocked`,
  `roundTripKeepsTheFrame` et `paintedAsANeutralFrame` ne dépendent pas de la vue et ne sautent jamais.
- **M2.** L'instance `WebFrameItems { id: appWebFrames; objectName: "appWebFrames"; document: appDocument }` est posée
  dans `WebFrameLayer.qml` (fichier de la tâche), pas dans `Main.qml`.
- **m14, B1.** `paintedAsANeutralFrame` vérifie le repère posé par L8-T3 ; L8-T30 ne touche pas à `pagepainter.cpp`.

<a id="tache-l8-t31"></a>
### Tâche L8-T31 — Diffusion protégée I : règles et copie protégée

**Objectif.** F-680 et S18 (première moitié) : `file.publish` ouvre `PublishDialog.qml` (informations de l'auteur,
quatre interdictions : copie de pages et d'objets, enregistrement et export, impression, caméra) ; « Diffuser… »
écrit un **nouveau** fichier `.actcraft` (par `DocumentIo`, A34) qui porte `BroadcastRules` ; l'original reste
intact, ouvert, sans règles ; les règles valent aussi pour l'auteur ; table des catégories : chaque commande
`restrictable` a une catégorie (`copy`, `save`, `print`, `camera`).

**Fichiers.** Ceux de la ligne.

**Test et commande bornée.** Commande ciblée `-R tst_core_broadcast`.

**Interdits.** `core/` sans `actions/` : la table des catégories est une fonction pure de `core/` sur des
identifiants (`restrictionFor(std::string_view id)`) ; aucun chiffrement (N1-A4) ; identifiants de la caméra :
`tool.camera-*`, préfixe `tool.camera-` (D10, arbitré par l'orchestrateur).

**Terminé quand** les cinq fonctions passent.

**Étapes.**
- [ ] **Étape 1 : en-tête** `core/broadcast.h` :
  ```cpp
  #pragma once
  #include <actcraft/core/model.h>

  #include <optional>
  #include <string_view>

  namespace actcraft::core {
  enum class Restriction { Copy, Save, Print, Camera };
  // Catégorie d'une commande restreignable ; nullopt : la diffusion protégée ne la touche pas.
  std::optional<Restriction> restrictionFor(std::string_view commandId) noexcept;
  bool forbids(const BroadcastRules& rules, Restriction restriction) noexcept;
  Document published(const Document& source, const BroadcastRules& rules); // copie qui porte les règles
  } // namespace actcraft::core
  ```
- [ ] **Étape 2 : test rouge** `tests/tst_core_broadcast.cpp` : `publishedCopyCarriesTheRules` ;
  `originalIsUntouched` (empreinte de la source inchangée) ; `rulesApplyToTheAuthorToo` (aucune exception d'auteur
  dans `forbids`) ; `defaultRulesAreOmitted` (`published(d, BroadcastRules{})` : `broadcast().isDefault()`) ;
  `categoryOfEveryRestrictableCommand` (lit `actions/data/commands.json` par `QFINDTESTDATA` ou
  `ACTCRAFT_SOURCE_DIR` : chaque commande `"restrictable": true` a une catégorie, et chaque identifiant de la table a
  `"restrictable": true`).
- [ ] **Étape 3 : code** ; `PublishDialog.qml` et gestionnaire `file.publish` (chemin choisi par la boîte de fichiers,
  ou argument dans les scénarios). **Étape 4 : vert** ; commit « Lot 8 : diffusion protégée, règles (L8-T31) ».

**Amendement 1 (L8-T31).** B2 : les règles passent par `Document::broadcast()` et `setBroadcast` (L8-T3) ;
`published` copie le document et appelle `setBroadcast`. m12 : identifiants de la caméra `tool.camera-*` (D10 arbitré).

<a id="tache-l8-t32"></a>
### Tâche L8-T32 — Diffusion protégée II : porte du registre ; scénario `diffusion-protegee`

**Objectif.** S18 (seconde moitié) : quand le document ouvert porte des règles, le **registre** refuse les
commandes restreignables de leurs catégories (`isEnabled` et `trigger`), comme le verrou de classe (D6-7) ; un essai
affiche le message de refus (V58) ; glisser un objet vers les ressources est refusé (V58, `ResourceLibrary`, A30) ;
les actions de page et d'objet continuent (V58) ; ouvrir un autre document sans règles rouvre tout.

**Fichiers.** Ceux de la ligne.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_broadcast|tst_actions_lockable|tst_ui_resources|tst_captures_diffusion_protegee'`.
Mutation : retirer le terme de la restriction d'`isEnabled` → `restrictedCommandsGreyed` et
`enableCannotBypassTheRestriction` rouges, remise par `git checkout -- actions/src/commandregistry.cpp`.

**Preuve visuelle.** `LU appBroadcast.active=true OK`, `LU menuItem-edit.copy.enabled=false OK` (`ui/qml/CommandEntry.qml:15`), `CAPTURES OK` ; décrire la capture
`refus`.

**Interdits.** Aucune voie de contournement : les contrôleurs du lot 6 et les actions passent par le registre (une
action `command.edit.copy` est refusée comme le menu) ; la porte ne touche pas l'état propre (`isOwnEnabled`) ;
aucun texte de l'original dans les messages.

**Terminé quand** les sept fonctions, `tst_actions_lockable` et le scénario passent, la mutation a été vue rouge.

**Étapes.**
- [ ] **Étape 1 : registre** (même patron que L6-T3) : `void setRestricted(QSet<QString> ids)`,
  `bool isRestricted(const QString& id) const` ; `isEnabled` = état propre ∧ ¬(verrou ∧ `lockable`) ∧ ¬(restreint ∧
  `restrictable`) ; `trigger` refuse de même ; signal ou rappel `refusedByRestriction(id)`.
- [ ] **Étape 2 : porte** `ui::BroadcastGate` (`QML_ELEMENT`, `appBroadcast`, `commands`, `document`) : à chaque
  document ouvert, calcule l'ensemble des identifiants restreints par `core::restrictionFor` et `forbids`, l'envoie
  au registre ; propriété `active` ; message de refus.
- [ ] **Étape 3 : test rouge puis vert** `tests/tst_ui_broadcast.cpp` : `restrictedCommandsGreyed` (`edit.copy`, `file.print` : `isEnabled` faux) ; `triggerShowsTheRefusal` (message émis, gestionnaire non appelé) ; `enableCannotBypassTheRestriction` ; `lockAndRestrictionCombine` (verrou levé, restriction tenue) ; `resourceDragRefused` ; `pageActionStillRuns` (action `page.next` d'un objet : la page change) ; `rulesFollowTheOpenDocument` (document sans règles ouvert : tout revient).
- [ ] **Étape 4 : ressources** : `ResourceLibrary::addSelection` refuse quand `noCopy` (message).
- [ ] **Étape 5 : scénario** `diffusion-protegee.json` (fixture : document protégé « copie » et « impression », un rond
  portant `page.next`) : `expect` `appBroadcast.active` vrai, `command` `edit.copy` → refus, `command` `file.print` →
  refus, glisser vers les ressources → refus, clic sur le rond → `pageIndicator.text` « 2 / 2 ».
- [ ] **Étape 6 : vert**, mutation rouge puis remise ; commit « Lot 8 : diffusion protégée, porte du registre
  (L8-T32) » ; `NOMS OK`.

**Amendement 1 (L8-T32).** M1, M2 : `ui/qml/Main.qml` gagne
`BroadcastGate { id: appBroadcast; objectName: "appBroadcast"; commands: appCommands; document: appDocument }` ;
lecture de l'état d'une commande : `menuItem-edit.copy.enabled` (`ui/qml/CommandEntry.qml:15`). M7 : réglages d'essai.

<a id="tache-l8-t33"></a>
### Tâche L8-T33 — Profil « Son et image » : épinglages du lot 8

**Objectif.** Règle de l'amendement 1, § 1 : le lot qui livre un outil épinglé par un profil complète ce profil.
`son-image.json` épingle `media.sound-controller`, `record.sound`, `record.screen-full`, `record.screen-area`,
`insert.web-frame`, `link.file`, `link.website` (filtrés par plateforme à l'affichage, D6-2) ; liste écrite dans
`docs/verifications/2026-10-23-lot8-epinglages.md` (forme du fichier du lot 7, A29) ; S32 préparé.

**Fichiers.** Ceux de la ligne.

**Test et commande bornée.** Commande ciblée `-R tst_ui_profiles` ; fonctions `pinnedCommandsExist`,
`pinnedFilteredByPlatform` (lot 6) et `soundAndImagePinsLot8` (neuve : les sept identifiants sont épinglés ; sous
Android, `record.screen-*` filtrés) en `PASS`.

**Interdits.** Aucun nom de profil de l'original ; ne toucher à aucun autre profil.

**Terminé quand** les trois fonctions passent. **Étapes.** Test rouge ; données ; vert ; commit « Lot 8 : épinglages
du profil Son et image (L8-T33) » ; `NOMS OK` ; titres vides.

<a id="tache-l8-t34"></a>
### Tâche L8-T34 — Contrôle du lot

**Objectif.** Clore le lot 8 : talons vides, suites debug et release, captures de tous les scénarios hors écran puis
à l'écran en D3D11 (jeton `ecran`, annoncé à Jérôme d'une ligne), banc complet D3D11 plein écran avec la vidéo de S25
(porte de non-régression de DA7 : tout critère tenu par la référence l'est encore, sinon tâche de correction avant
clôture), installateur et APK, tailles ajoutées par Qt Multimedia et Qt WebView (N1-A10), version **0.9.0**
(`project(VERSION)`), section 0.9.0 de `CHANGELOG.md` (en français, à nous), mesure de fin de lot commitée, graphe.

**Fichiers.** Ceux de la ligne.

**Test et commande bornée.** Preuve de tâche en debug puis `-Preset release` ; captures `-Scenario tous` hors écran,
puis à l'écran (`-Rendu d3d11`, sans `-HorsEcran`) ; banc `pwsh -NoProfile -File ./scripts/bench.ps1` (forme bornée
du profil) ; `scripts/paquet.ps1` (`PAQUET OK`), `scripts/paquet-android.ps1` (`APK OK`).

**Preuve visuelle.** Toutes les lignes `LU` des scénarios du lot en `OK` ; décrire les captures à l'écran de
`interactivite` (S11), `medias` (S25), `enregistreurs` (S26), `cadre-web` (S27), `diffusion-protegee` (S18) ; capture
de l'APK sur l'émulateur : `record.screen-*` absents des menus.

**Interdits.** Aucune publication (release sous go écrit seulement) ; fermer par PID ce qui a été lancé.

**Terminé quand** le critère de la ligne L8-T34 est tenu.

**Étapes.**
- [ ] **Étape 1 : talons** (règle 4) : `-functions` sans `reserved` pour les 37 tests du lot ;
  `git -C <wt> grep -n "Talon L8 : rempli par" -- . ':(exclude)docs/'` vide ;
  `ctest --preset debug -V | Select-String 'SKIP   :.*Talon'` vide.
- [ ] **Étape 2 : sécurité** : contrôle d'occurrences de `QDesktopServices` et `openUrlExternally` ; aucune ligne
  neuve `setHtml` ou `RichText` hors du garde `loadResource`.
- [ ] **Étape 3 : suites** debug et release (`TESTS OK` deux fois, N = N8 + 37 + p, ou N8 + 28 dans le repli).
- [ ] **Étape 4 : captures et banc** ; résultats résumés dans `docs/mesures/2026-10-24-lot8-fin.md`.
- [ ] **Étape 5 : paquets** et tailles.
- [ ] **Étape 6 : version et notes** : `project(ActCraft VERSION 0.9.0 …)`, section `## 0.9.0` de `CHANGELOG.md`
  (actions, conteneurs, limiteurs, liens, encre révélatrice, médias, enregistreurs, cadre web, diffusion protégée).
- [ ] **Étape 7 : graphe et commit** : `graphify update .` ; `git add` des fichiers de la ligne ; `NOMS OK` ; titres
  et paperboards vides ; commit « Lot 8 : contrôle du lot, version 0.9.0 (L8-T34) ».

**Amendement 1 (L8-T34).** M2 : `tests/tst_ui_shell.cpp` gagne `lot8bControllersWired`, même forme que
`lot8ControllersWired` (L8-T19), sur `appLinkDialogs` (`actcraft::ui::LinkDialogs`), `appMediaInsert`
(`actcraft::ui::MediaInsert`), `appMediaItems` (`actcraft::ui::MediaItems`), `appBroadcast`
(`actcraft::ui::BroadcastGate`), `appRecorders` (`actcraft::ui::Recorders`), `appWebFrames`
(`actcraft::ui::WebFrameItems`) — dans le repli, les quatre premiers seulement. M11 : APK par la commande de
« Commandes propres à 8b ».

## Couverture (relecture de 8b)

| Exigence ou constat | Tâches |
|---|---|
| F-550, F-551 (dialogues) | L8-T22 |
| F-552 (lecteur intégré, deux pistes, lecteur de sons) ; N2-9 en repli | L8-T23, L8-T24, L8-T25 ; repli : L8-T10, L8-T25 réduite |
| F-582 (effet sur la lecture) | L8-T25 |
| F-293, N1-A2, N1-A16 | L8-T26, L8-T28 |
| F-292, N2-A9, N1-A16, V43 | L8-T27, L8-T28 |
| F-302, N1-A3, N1-A11, V63, D12 | L8-T29, L8-T30 |
| F-680, N1-A4, V58 (quatre constats), D10 | L8-T31, L8-T32 |
| V57, V66 (GIF), V126 (second média), V55 (Flash) | L8-T25, L8-T23 |
| F-880 (profil « Son et image »), S32 préparé | L8-T33 |
| N1-A10 (tailles et licences) | L8-T20, L8-T34 |
| Son de confirmation des conteneurs (F-580) | L8-T16 (signal), L8-T25 (lecture) |
