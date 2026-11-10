# État — cadre-web-webengine

Statut : en cours
Gabarit : chantier
Avancement : 23/23 tâches terminées
Étage : 4 — depuis 2026-11-10 15:00
Questions à Jérôme : aucune
Worktree : /Users/jeromedenis/projets/actcraft/.worktrees/cadre-web-webengine   Branche : feat/cadre-web-webengine   Base : origin/feat/livraison-complete-l11 @ 76315dbc
Branche principale locale au lancement : 8a293f30
Cadrage : docs/specs/2026-11-09-cadre-web-webengine-cadrage.md   Plan(s) : docs/plans/2026-11-09-cadre-web-webengine-PLAN.md
Relectures : mac

Demande : O146 (Jérôme, 2026-11-09, canal ActCraft : « Tu peux commencer la tâche Qt WebEngine, dernière du chantier ? »). Décision de référence : `~/SecondBrain/02_Decisions/0217-actcraft-liberte-avant-securite-et-qt-webengine.md`. Pas de porte humaine : une question bloquante se tranche depuis 0217 et devient une hypothèse à revoir.

Interdits de la tâche : aucune release, aucun tag ; `release.yml` non touché (O98) ; pas de fusion, PR en brouillon vers `feat/livraison-complete-l11` ; aucun job sur le PC ; aucune machine GitHub ; pas de contact avec O84, O86, O89 ; jeton `tests`, une seule compilation lourde ; `NOMS OK`.

## Essai de faisabilité (2026-11-09, avant tout plan) — RÉUSSI
Application Qt Quick d'essai (hors dépôt, scratchpad de session), préfixe `CMAKE_PREFIX_PATH=/opt/homebrew` :
- construit et lié contre Qt 6.12.0 (qbase, qtdeclarative) et qtwebengine Homebrew 6.11.2_1 (Chromium 140). Cette formule est reconstruite contre Qt 6.12 : ses frameworks se déclarent 6.12.0.
- `CMAKE_PREFIX_PATH=/opt/homebrew/opt/qt` échoue (chemins d'en-têtes `opt/qt/include/QtWebEngine*` inexistants) : le préfixe doit être `/opt/homebrew`, ou un chemin équivalent pour WebEngine.
- à l'exécution : `qVersion` 6.12.0, page chargée, `runJavaScript` rend le résultat, code de sortie 0.
- capture `grabToImage` d'un `WebEngineView` : PNG 640 × 480, pixel relevé rouge attendu (RGB 200,30,30).
- H.264 (Homebrew, Mac) : `canPlayType` « probably », `MediaSource.isTypeSupported` vrai ; AAC, MP3, VP9 aussi. Décodage réel d'une vidéo : à faire.
Mélange retenu : Homebrew tel quel. `aqt` non nécessaire.

## Référence
Rapide et complète (base 76315dbc, Mac debug, `ctest -j2 --timeout 120` hors écran) : 329/330. Seul rouge : `tst_ui_linklauncher` (`htaRefused`, 184 dossiers d'extraction contre 183 attendus : le test compte les dossiers du temp commun, une autre suite du Mac en crée en même temps). Rouge préexistant, instable, pas une régression.

## Compteurs
contre-relecture cadrage 1/2 (1 bloquant, 5 majeurs, 6 mineurs corrigés, ea065489) · relecture plan 2/3 · vagues sans progrès 0/2 · correctifs de fin 2/2

## Scénarios
S1 dépôt et copie hors écran, formats exacts du moteur · S2 coller une URL d'image, échecs, changement d'onglet · S3 gardes rallumées par le réglage, effet immédiat, cookie A/B · S4 paquet Mac : poids avant/après, H.264, provenance de Qt, pas de Chromium sans cadre, coût du premier cadre · S5 Android sans WebEngine, greffon dans l'APK

## Tâches (détail : table du plan)
| id | dépend | subagent | état | essais | dernier commit vert | preuve rejouée |
|----|--------|----------|------|--------|---------------------|----------------|
| T1 | — | concepteur-critique | terminée | 1/3 | ccb45113 | 332/332 ; `realFrameLoadsThroughTheLoader`, `contractsMatch` PASS ; revue : 0 bloquant, 0 majeur |
| T2 | T1 | implementeur-tdd | terminée | 1/3 | 060684b6 | 332, seul rouge linklauncher préexistant ; guardsFollowTheProfileSetting, desktopLayerFollowsTheSetting PASS |
| T14 | T1 | implementeur-tdd | terminée | 1/3 | 766f1b1d | 332/332 ; settingStepIsParsed PASS ; SCENARIO OK (boucle 237 ms, informatif) |
| T15 | T1 | implementeur-tdd | terminée | 1/3 | d808bc4a | tst_app_notices 6/6 ; everyAllowedQtModuleHasANotice PASS ; LICENCES OK sur bundle factice |
| T3 | T2, T14 | concepteur-critique | terminée | 1/3 | 41829385 | 332/332 ; guardsOffSendsNoRequest, heavyHtmlGoesToAFile PASS ; revue : 2 majeurs, 5 mineurs corrigés dans T4 |
| T4 | T3 | implementeur-tdd | terminée | 1/3 | 816fb61b | 332/332 ; reportFillsTitleAndHistory, guardsOffCommandsGoToTheView PASS |
| T9 | T1 | implementeur-tdd | terminée | 1/3 | a581f85d | 333/333 ; tst_ui_webimagefetch 10/10, oversizeWithoutLengthIsRefused PASS |
| T10 | T4, T9 | concepteur-critique | terminée | 1/3 | b0ab9ab6 | 333/333 ; webDropDownloadsAnImage, downloadNeverLandsInAnotherTab, webDropOfOversizeFails PASS ; revue : 0 bloquant, 0 majeur |
| T11 | T10 | implementeur-tdd | terminée | 1/3 | 36470cae | 333/333 ; pasteImageAddressDownloads, pasteNonImagePageBecomesText PASS ; tst_ui_insertion 41/41 |
| T5 | T2, T4 | concepteur-critique | terminée | 1/3 | d721ebaf | 333/333 ; javaScriptRunsGuardsOff, cookieStaysInItsDocument, heavyHtmlLoads PASS ; banc résultat ; revue : 1 majeur repris en T6 |
| T6 | T5 | implementeur-tdd | terminée | 1/3 | 25255137 | 333/333 ; snapshotInsertsAnImage, insertSnapshotIsOneStep PASS ; preuves moteur de revue T5 ajoutées, mutations rouges |
| T7 | T6 | implementeur-tdd | terminée | 1/3 | 8592a98b | 333/333 ; stylusOnWebFrameDrawsNothing PASS ; banc résultat (BUDGET DEPASSE, logiciel) |
| T8 | T7 | implementeur-tdd | terminée | 1/3 | d4283cda | 333/333 ; dialogNoteFollowsTheGuards PASS ; tst_ui_desktop 60/60 |
| T12 | T8, T11 | implementeur-tdd | terminée | 1/3 | 20ffd48b | 334/334 ; tst_ui_webdrop 9/9, droppedRemoteImageSurvivesReopening, guardsCoverDropAndPasteThroughTheWiring PASS ; banc résultat |
| T13 | T12 | implementeur-tdd | terminée | 1/3 | 0885ca8d | 334/334 ; editCopyCopiesTheSelection, inputFieldKeepsPaste, editCopyWithoutFocusedFrameCopiesObjects PASS |
| T16 | T13, T15 | concepteur-critique | terminée | 1/3 | 944ba429 | 334/334 ; DMG OK 188 680 277 octets ; CADRE WEB OK, ESSAI DMG OK ; borne 5 s en échec ; tst_app_open 5/5 |
| T17 | T13 | implementeur-tdd (isolé) | terminée | 1/3 | 9c385284 | YAML OK ; release.yml non touché ; APK construit : libqml_QtWebView_ 1, WebEngine 0 ; APK OK non atteint, licences Qt6FFmpegStub (Reste PC 9, cause écrite) |
| T18 | T16, T17 | implementeur-tdd | terminée | 1/3 | 99cc0c24 | `docs/mesures/2026-11-09-cadre-web-webengine.md` : `.dmg` 61,5 → 180,6 Mo ; `ACTCRAFT H264 w=320 h=240 t=2.362454` ; DMG OK, ESSAI DMG OK, CADRE WEB OK |
| T19 | T13 | implementeur-tdd | terminée | 1/3 | 7f8d4aa9 | 332/334, tst_ui_shell et tst_ui_mathtools en délai dépassé sous charge 14-18, verts seuls ; downloadedImageDoesNotOverride, webDropOfHugeDataUrl, refusedRedirectDropsTheHeavyFile PASS |
| T20 | T13 | implementeur-tdd | terminée | 1/3 | 775f6bb0 | 334/334 (ctest --timeout 300, charge 30-40) ; clickOnThePageTakesTheFocusBack, tst_ui_webengine 33/33, tst_ui_webdrop 11/11 PASS |
| T21 | T19, T20 | implementeur-tdd | terminée | 1/1 | 8931fbc2 | simplifications sans test modifié ; rejeu sous charge 26-135 : 328/334, 5 délais dépassés et `tst_ui_drawing` (attente de 3 s), repris par la suite de T22 |
| T22 | T21 | implementeur-tdd | terminée | 1/3 | af40b9f8 | 334/334 (ctest --timeout 300, 4 sautés) ; rightClickOnThePageTakesTheFocusBack, snapshotIsCancelledWhenTheGuardsToggle PASS ; correctifs de la relecture finale |
| T23 | T22 | implementeur-tdd | terminée | 1/3 | 6f796c6f | debug 334/334, release 334/334 (ctest --timeout 300, 4 sautés préexistants) ; tst_ui_webimagefetch 5 fois vert ; leftClickOnACoverKeepsTheFocus, redirectAnnouncingALargeBodyIsFollowed PASS |

## Terminé quand
toutes les tâches terminées (aucune bloquée) · double relecture sans bloquant ni majeur · suite complète Mac verte en debug et en release · S1…S5 réussis · poids du `.dmg` et H.264 écrits · PR en brouillon ouverte vers `feat/livraison-complete-l11`, non fusionnée

## Double relecture, tête 0885ca8d (4 relecteurs, 0 bloquant, 6 majeurs)
T19 (C++) : `objectediting.cpp:134-142`, sélection et changement d'outil 20 s après le collage (Majeur) ; `insertion.cpp:549-555`, `data:` décodé sur le fil de l'interface, borne avant décodage (Majeur) ; `fetched` garde `contentFile` sur redirection refusée (`clearShown`) ; `sameAsPreviousCapture` et `m_lastCapture` ; tests : un enregistrement dans le même dossier ne recharge pas, `row.history.clear`, redirection vers `file:`.
T20 (QML) : `Main.qml:858`, `appInsertion.failed` non relié au bandeau (Majeur) ; tests du bandeau de dépôt et de collage (Majeur) ; `viewFocused` reste vrai après un clic sur la page (`WebFrameEngineView.qml:18`, Majeur, deux relecteurs) ; `pendingSnapshot` jamais annulé (`WebFrameLayer.qml:116-136`, Majeur) ; `navigateOnDropEnabled` gardes allumées à vérifier ; commentaires périmés ; `qWait(300)` ; `CHANGELOG.md:45-46` ; `docs/banc.md:302` ; `skipReason` de `cadre-web.json`.
T21 : simplifications C++ sûres, tests inchangés, un essai.
Relecture finale T16 à T21, tête 8931fbc2 : 0 bloquant, 2 majeurs (clic droit sans `pointerPressed`, repris en T22 ; S5 sans preuve, repris par l'APK de T17), mineurs repris en T22.

## Rapport (clôture, 2026-11-10)
Tête `6f796c6f` + graphe. Debug et release sur le Mac : 334/334, `ctest -j2 --timeout 300` (écart au 120 du profil : charge 25 à 135 des autres ouvriers, la limite de 120 s donnait des dépassements verts seuls). Sautés : 4, préexistants. Aucun test retiré ; 3 `QSKIP` ajoutés, Windows hors écran, prévus (Reste PC 4).
Relectures : double sur T1-T13 (6 majeurs fermés) ; finale sur T16-T21 ; ciblées sur T22 puis T23 : à la dernière, 0 bloquant, 0 majeur, 2 mineurs non corrigés (branche du doigt de `pointerPressed` sans test ; refus sur taille annoncée prouvé sans socket ouvert). Plafond de 2 tours de correctifs atteint.
Simplifications non appliquées, pour le prochain audit : doublons `showDirect`/`prepareContent`, `finishDirect`/`finishContent`, base de dossier de `fileChanged`/`documentReplaced`.
Scénarios : S1, S2, S3 par les suites nommées ; S4 par T18 (mesures, sur 1c15aa4c, avant T22 et T23 qui ne changent pas le poids) ; S5 partiel : APK construit sans WebEngine, contrôle de licences en échec, voir Reste PC 9.
Banc : `BUDGET DEPASSE` en logiciel hors écran sur machine chargée, résultat et non porte ; aucune comparaison avant/après (non mesuré).
Instabilité observée : `tst_ui_webimagefetch` `oversizeIsRefused` (course « Connection closed ») : refus désormais sur la taille annoncée, 5 fois vert.
Essai du `.dmg` : `scripts/essai-dmg.sh` borne maintenant l'application (90 s) et échoue sans garder le verrou, preuve à 5 s.
Release due : aucune décidée ici (interdit par O146). `release.yml` : voir « Sortie pour O98 » du plan et `docs/mesures/2026-11-09-cadre-web-webengine.md`.

## Décisions de l'orchestrateur
- Banc limité à T1, T5, T7, T12, T18 (Mac, Release, 8 Go) : T6, T8, T13 touchent le QML du cadre, pas le chemin d'encre ; T18 mesure le cumul. Écart assumé au n° 36 du cadrage.
- Relectures par agents au premier plan (ordre O146), sans `relire-sur-mac.cjs`.
- Mineurs de la revue de T1 reportés en T2 (`engineBuilt` retiré, assertion `AA_ShareOpenGLContexts`, commentaires périmés) et à T5 (dépréciation de `WebEngineProfile {}`).
- Reprise du 2026-11-09 19:56 (processus arrêté pendant la construction de T2) : implémentation non commitée de T2 gardée et agent repris, au lieu du rejet prévu par la chaîne, car l'ordre de reprise dit de ne pas refaire ce qui est fait. La preuve reste rejouée au rapport.
- **Consigne du canal ActCraft (2026-11-09, jusqu'à la fin du chantier)** : aucun `run_in_background`, aucun sous-agent en fond, ne jamais terminer le tour pour attendre (en `claude -p`, la fin du tour tue le fond). Construction longue : `nohup … > .tmp/x.txt 2>&1 &` sous le jeton, attente au premier plan par appels Bash (`timeout` 600000, boucle sur un fichier de fin). Même règle pour les sous-agents, écarte « tâche de fond » des références de la chaîne.
- Revue de T10 : 4 mineurs. Câblage `webFrames` sans test au plan de T12 : test ajouté à T12 (`MainWindow`, gardes allumées, dépôt puis collage, `requests() == 0`). Autres mineurs d'`insertion` joints à T11.
- Revue de T5 : 1 majeur (historique et bascule non prouvés sur le vrai moteur, gardes coupées) et mineurs de test joints à T6 ; titre illisible joint à T8. Hors périmètre consigné : ajouter ou retirer un cadre recrée toutes les vues (`webframeitem.cpp:1350`). Banc T1 → T5 : 11 → 13 dépassements en logiciel, non imputable à T5 (aucun scénario court n'a de cadre).
- T16 : essai borné (`ACTCRAFT_ESSAI_DELAI`, 90 s) sur ordre du général du 2026-11-10 : un essai qui ne finit pas seul échoue sans garder le verrou. Preuve : borne de 5 s en échec, aucune application restante. Revue de T16 jointe à la relecture finale. Q1 close par le pilote après Go, Q2 annulée. Reste un auxiliaire orphelin PID 92196 de l'ancien essai, refusé par `garde-arret`, sans effet sur le verrou.
- Phase 0 (3.1) : la preuve `ctest` détecte un échec, vu sur la référence (`tests failed`, code de `tst_ui_linklauncher`).

## Hypothèses à revoir
Voir le cadrage : 15 hypothèses (H1 JavaScript et adresses distantes sur Android, H2 codecs selon la provenance de Qt, H3 `initialize()` sous Windows, H4 mémoire du Dell, H5 glisser réel à l'écran, H8 fichiers du système acceptés, H14 URL collée sans réseau, H15 premier cadre vide le temps du `Loader`).

## Restes pour le PC
1. Installer `qtwebengine qtwebchannel qtpositioning` dans `C:\Qt\6.12.0\msvc2022_64`.
2. H3 : démarrer sous D3D11 sans `initialize()` ; repli si échec.
3. MSVC `/W4 /WX`, preset `release`, suite complète.
4. `tst_ui_webengine` et saut hors écran Windows : décider du maintien ; retirer le repli de `WebFrameItems::engineKind` sous Windows hors écran si la preuve PC passe.
5. `scripts/paquet.ps1` et `scripts/essai-installateur.ps1` : `windeployqt` trouve-t-il `QtWebEngineProcess.exe` et les ressources ; noms exacts à ajouter à `qt-autorises.txt`.
6. Poids de l'installateur avant et après.
7. H2 : `canPlayType` H.264 sur la version officielle.
8. H4 : mémoire et banc sur le Dell 3510 ; captures D3D11 sous le jeton `ecran`.
9. S5, fin : obtenir `APK OK`. L'APK se construit sur le Mac sans WebEngine (`libqml_QtWebView_` : 1, `WebEngine` : 0, `build/android-arm64/app/android-build/actcraft.apk`, 110 Mo). `scripts/verifier-licences.ps1` le refuse : « LICENCES EN ECHEC : libQt6FFmpegStub-crypto_arm64-v8a.so (nom de module illisible), libQt6FFmpegStub-ssl_arm64-v8a.so (nom de module illisible) ». Ces bibliothèques viennent de Qt Multimedia et le script n'a pas changé dans ce chantier : décider de leur licence, puis accepter ce nom (`Qt6FFmpegStub-…`) dans le script et dans `packaging/qt-autorises.txt`.

Restes pour Jérôme : R1 glisser réel sur Mac, R2 appui long, collage et capture sous Android, R3 décision de licence sur la redistribution des décodeurs H.264 avant la première release avec WebEngine.

## Portes
ouvertes : — · passées : —

## Pièges trouvés
- `CMAKE_PREFIX_PATH` sur `opt/qt` casse le lien à WebEngine (voir essai) ; l'application ne lie pas WebEngine (chargement QML au premier cadre, DA2), donc `opt/qt` reste.
- Prémisses démenties : aucun `DropArea` de page n'existait ; une image glissée arrive souvent en `file:` temporaire ; `selectedText` n'existe pas sur `WebEngineView` ; `QtWebEngineCore` 275 Mo + 68 Mo de ressources, « 150 Mo » sous-estimé.
- Chantiers sur les mêmes fichiers : PR #23 (base), #21 et #22 (`ci.yml`), #2 (`CLAUDE.md`, `CMakeLists.txt`).
