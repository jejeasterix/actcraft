# État — livraison-complete

Statut : en cours — repris le 2026-10-15 (nouvelle session) ; règles communes recréées dans le scratchpad de la session (`regles-tache-livraison.md`)
Gabarit : lots (provisoire, à confirmer à l'étage 2)
Étage : 3 (lot 1, vague 1)
Worktree : C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete   Branche : feat/livraison-complete   Base : origin/main @ 175b336
Branche principale locale au lancement : 175b336   (vérifiée à chaque rapport de tâche)
Cadrage : docs/specs/2026-10-14-livraison-complete-cadrage.md (663644e ; 38 questions, 16 N1, 12 DA, S1–S14, 11 lots ≈ 134 tâches dont lot 9 vidéos et lot 10 cache ; 175 exigences incluses, 25 exclues)   Plans : un par lot — lot 1 : docs/plans/2026-10-14-livraison-complete-lot1-PLAN.md (7e4c743 ; 20 tâches L1-T1…T20, 12 vagues)
Mode du cadrage : autonome (consigne de Jérôme, 2026-10-14 : « travailler en autonomie jusqu'à livraison complète »)

## Demande
Jérôme, 2026-10-14 : « tu dois travailler en autonomie jusqu'à livraison complète. je t'ai déjà donné les
objectifs. Ajoute la release .exe et la release .apk ». Objectifs = `docs/BRIEF.md`.
Périmètre : étape 3 (lecteur `.flipchart` C++, format natif ouvert) ; étape 4 (interface et tous les outils et
fonctions, un module à la fois, référence de comportement = application Python et docs de l'ancien dépôt) ;
release Windows `.exe` (installateur) ; release Android `.apk`.
Étape 2 (mesures Dell 3510 / MacBook M1) : attend Jérôme, ne bloque pas le chantier. Mesure de la machine de
développement : `docs/mesures/2026-10-13-prototype-mesure.md` (budget tenu sauf 1 figement sur 10, H-mes-1).
Ajout (Jérôme, 2026-10-14) : « À la fin, étudie chaque vidéo de la chaîne https://www.youtube.com/@YouNadineTanguay/videos, confronte les principes de fonctionnement avec ce que tu as produit et corrige en conséquence » → dernier lot du chantier. Corpus préparé dès maintenant par un sous-agent indépendant dans `REA - Activinspiredocsideos-tanguay` (hors dépôt public). Consigne : déléguer à des sous-agents pour ne pas encombrer le contexte. Jérôme a fermé ses autres travaux (jetons libres).
Contexte de la demande : Jérôme a vu le prototype de mesure et attendait une interface (« aucune interface »).

## Référence
Rapide (worktree neuf, debug) : 29/29 TESTS OK
Complète (origin/main @ 175b336) : release 29/29 TESTS OK · debug 29/29 TESTS OK

## Compteurs
contre-relecture cadrage 2/2 (tour 2 : 0 bloquant, 6 imp. + 4 min. appliqués en 663644e ; tour 1 : 1 bloquant, 11 importants, 3 mineurs → renvoyés au cadreur ; arbitrages : questions « fonctions à revoir » et « cache avant la mesure Dell » posées à la porte du lot 1 avec défaut ; lot cache déplacé avant la release 1.0 ; lot vidéos avant la release 1.0) · relecture plan lot 1 : 2/3 (tour 1 : 1 bloquant sécurité → 1b96289 ; tour 2 : 0 bloquant, 1 imp. + 3 min. → 7e4c743) · vagues sans progrès 0/2 · correctifs de fin 0/2

## Tâches
Lot 1 (plan 7e4c743) — fichiers touchés exacts et preuves : table du plan.
| id | dépend | zone | agent | worktree | état | essais | dernier commit vert | preuve rejouée |
|----|--------|------|-------|----------|------|--------|---------------------|----------------|
| L1-T1 | — | profil : release due, action-finale, go, banc, preuve visuelle | impl | (mergé) | terminée | 1/3 | e006e99 → merge 68141b3 | PROFIL OK 53 cas ; hook réel : 8 publications refusées, 4 lectures laissées (profil de main : 6 écarts) ; 29/29 TESTS OK |
| L1-T2 | — | outillage JDK, SDK/NDK, Qt Android, Inno Setup, émulateur | impl | (mergé) | terminée | 1/3 | 8109047 → merge 19faae3 | ENV ANDROID OK ; JDK 21 ; adb ; ISCC répond ; 14 VERIF ; WHPX utilisable, émulateur local démarre |
| L1-T3 | T1 | core Document | impl | (mergé) | terminée | 1/3 | 5cbd091 → merge a187279 | 30/30 TESTS OK, tst_core_document Passed |
| L1-T4 | T1 | actions registre | impl | (mergé) | terminée | 1/3 | 9eec448 → merge bdfd7e4 | 31/31 TESTS OK, tst_actions_registry Passed |
| L1-T5 | T4 | actions disposition | impl | (mergé) | terminée | 1/3 | d022191 → merge da59f4c | 33/33 TESTS OK, tst_actions_layout Passed |
| L1-T6 | T1 | input doigt | impl | (mergé) | terminée | 1/3 | 85f6f5a → merge 74dc063 | 33/33 TESTS OK ; tst_input.exe : 2 PASS nouvelles ; banc hors écran logiciel court : BUDGET DEPASSE (2 reprises), informatif — même configuration déjà DEPASSE au prototype |
| L1-T7 | T1 | ui thème, icônes | impl | (mergé) | terminée | 1/3 | 9e5c90f → merge | 32/32 TESTS OK, tst_ui_theme Passed ; 10 SVG à nous ; contrastes AA non mesurés |
| L1-T8 | T6 | ui PageView | crit | (mergé) | terminée | 1/3 | 217b039 → merge c86bf8c | 35/35 TESTS OK ; tst_pageview 22 cas, répété 20× en parallèle sans échec ; revue : doigt+stylet mêlés corrigé + 4 mineurs |
| L1-T9 | T3,T7 | ui DocumentController | crit | (mergé) | terminée | 1/3 | 34b06c9 → merge feca79c | 34/34 TESTS OK, tst_ui_document Passed (13 cas) ; revue : 2 mineurs corrigés |
| L1-T10 | T5,T9 | ui pont Commands | impl | (mergé) | terminée | 1/3 | a5293d3 → merge 31c438e | 35/35 TESTS OK, tst_ui_commands Passed ; rouge non vu → 3 mutations détectées (palette non mutée) |
| L1-T11 | T10 | version, À propos, licences | impl | (mergé) | terminée | 2/3 | f5371ea → merge e8e38b7 | 40/40 TESTS OK, 4 tests nommés + tst_licences_sans_fixture Passed ; banc 1280x800 content OK (la « régression » venait d'un `-Taille` oublié) |
| L1-T12 | T8,T11 | coquille QML | impl | (mergé) | terminée | 2/3 | b1318f9 → merge 96e99c3 | après intégration du chantier (b44a0ce) : 42/42 TESTS OK, tst_ui_shell, tst_ui, tst_ui_drawing, tst_bench_uiscenario Passed ; aucun test retiré ni QSKIP ; lancement debug hors écran `--quitter-apres 3000` → code 0 en 4,6 s ; banc (agent) BUDGET DEPASSE, content_check.ok = true. Cause du blocage : boucle `while (count > 0) removeItem(itemAt(0))` (Main.qml, CommandMenu.qml) → `Instantiator` (écart au plan). Release non jouée : à la clôture du lot |
| L1-T13 | T9 | actcraft-captures | crit | (mergé) | terminée | 1/3 | 59f789d → merge e7d4a3a | 41/41 TESTS OK, tst_bench_uiscenario Passed ; revue : touche acceptée réellement, module benchui (QtTest hors du banc), captures uniformes refusées |
| L1-T14 | T12 | banc adapté | crit | (mergé) | terminée | 1/3 | 16b6fa4 → merge 6fac247 | release 43/43 TESTS OK, debug 43/43 TESTS OK (tst_bench_verdict 24 s / 65 s sous charge, sans limite ctest propre), banc 1366×768 → content_check.ok = True ; tst_bench_verdict 0 échec / 48 en parallèle (agent) ; revue : 0 bloquant, 1 imp. + 3 min. corrigés (rouges vus : `runDemo` sans recentrage, hors écran sans taille fixe). Écarts au plan (porte de fin) : hors écran → 1280×800 ; test de l'étape 1 enrichi (4 tailles) ; centrage sur les 4 points ; `runner.h` et `docs/banc.md` hors table. Procédure Dell : échelle Windows 100 % (`docs/banc.md`) |
| L1-T15 | T12,T13 | scénario premiers-pas | impl | (mergé) | terminée | 1/3 | 344e12f → merge 0420a9e | 43/43 TESTS OK, tst_captures_premiers_pas Passed ; `captures.ps1 -Scenario premiers-pas -Rendu logiciel -HorsEcran` → 3 PIXEL OK, `LU pageIndicator.text=1 / 2 OK`, `CAPTURES OK` (code 0) ; rouge vu (test : SCENARIO INVALIDE ; script : `-Lire 9 / 9` → CAPTURES EN ECHEC) |
| L1-T16 | T2,T11 | installateur Inno Setup | impl | (mergé) | terminée | 2/3 | 6bdf6a4 → merge fd9ffee | `LICENCES OK`, `PAQUET OK : …\build\paquet\ActCraft-0.1.0-setup.exe`, `--version : ActCraft 0.1.0`, `INSTALLATEUR OK` (code 0) ; rouge vu (DLL fictive) ; essai en échec nettoie tout (5 Test-Path False, 6bdf6a4) ; aucun C++ ni test touché |
| L1-T17 | T2,T11,T16 | chaîne Android | crit | (mergé) | terminée | 1/3 | 2b18836 → merge 4ff5654 | `APK OK` (arm64-v8a, x86_64 : identifiant, versionCode 100, targetSdkVersion 36, 0 permission, signature débogage, LICENCES OK ; `build/paquet` sans `.idsig`) ; 43/43 TESTS OK (un premier passage 42/43 sous charge : `tst_bench_smoke` Timeout) ; revue adversariale : 0 bloquant, 1 imp. + 3 min. corrigés (0556485). Écart au plan : manifeste de repli + `tools:node="remove"` sur `DYNAMIC_RECEIVER_NOT_EXPORTED_PERMISSION` (porte de fin) ; effet à l'exécution → L1-T18 |
| L1-T18 | T12,T17 | essai APK émulateur | impl | (mergé) | terminée | 1/3 | 404d52c → merge e1dfdc7 | rejoué par l'orchestrateur (émulateur sans fenêtre, `-gpu host`, PID 4348 arrêté) : logcat sans FATAL/SecurityException (pid 4241), page (1958, 1392) 255 255 255, palette 43 47 54, pastille 229 57 53, trait 0 0 0, `ANDROID OK` ; capture : page entière, palette complète, textes lisibles. Rouge vu (agent) : SwiftShader → `rectangle à moitié dessiné`. Cause : GPU logiciel de l'émulateur (projet Qt minimal idem) → coffre `04_Pieges/qt-quick-emulateur-android-swiftshader-rectangle-en-un-triangle` |
| L1-T19 | T14,T15,T16,T18 | CI paquets | crit | (mergé) | terminée (sa part) | 1/3 | a420442 → merge 743dbe0 | run 38690918062 vérifié par l'orchestrateur : android, format, paquet-windows, build windows verts — `APK OK`, `rectangles : entiers`, `ANDROID OK` (lavapipe, émulateur 16428233 épinglé), `PAQUET OK`, `INSTALLATEUR OK` (vraie fenêtre D3D11/WARP, `qwindows.dll` du dossier installé), 43 tests ; macOS rouge sur 2 tests de L1-T13/T15 (`TOUCHE F5/F7 ECHEC`) → L1-C4 ; branche `essai-ci/L1-T19` supprimée. Revue : 0 faux vert ; constats intégrés (timeout par job, adb borné, actions épinglées par SHA, `permissions: contents: read`, artefacts de recherche supprimés, verdicts sur toute erreur, `-HorsEcran` ⇒ logiciel). Écarts (porte de fin) : upload-artifact pendant la recherche (supprimés), 7 pushes, S1 en d3d11, JDK 21, lavapipe, `tst_actions_registry.cpp` hors table (correctif macOS d'une ligne, e61db8b) |
| L1-T20 | T19 | release.yml, CHANGELOG | crit | — | à faire | 0/3 | — | — |
| L1-C3 | — | correctifs interface de la relecture (stylet sous les barres, trait pendant changement de page/vue, preuve F5 sur `visibility`, coche plein écran, molette/Ajuster, `reload()`) | crit | (mergé) | terminée | 1/3 | 09fc71c → merge 861cdcc | debug 44/44 et release 44/44 TESTS OK ; `CAPTURES OK` avec `LU window.visibility=FullScreen/Windowed OK` ; chaque constat a son test vu rouge (agent) ; revue : 0 bloquant, 0 imp., 3 min. corrigés ; API privée de Qt (`QuickPrivate`) refusée par l'orchestrateur dans le code livré → API publique (`QObject::inherits`), `git grep Private ui/` vide ; limite documentée par test : `Item` quelconque avec bouton gauche propre + HoverHandler → le stylet y trace (aucun élément actuel) |
| L1-C4 | L1-C3,T19 | touches F5/F7 injectées refusées sous macOS (CI) + correctifs scripts de capture/banc (garde WINDIR, polices des tests bench.cmake, `CAPTURES EN ECHEC`) ; preuve par run CI sur `essai-ci/L1-C4` (autorisé, règles de L1-T19) | crit | (mergé) | terminée | 1/3 | 6077f0d → merge f557861 | **run CI 38697953843 : 5/5 jobs verts** (macOS 44/44, Windows 44/44, android, paquet-windows, format) — vérifié par l'orchestrateur ; local 44/44 TESTS OK, `CAPTURES OK`, scénario inconnu → `CAPTURES EN ECHEC` code 1. Cause : Qt 6.12 ne consulte pas les raccourcis à la livraison d'une touche sous macOS (`qguiapplication.cpp:2612`) → `handleShortcutEvent` avant la livraison ; coffre `04_Pieges/qt-macos-touche-injectee-ne-declenche-pas-les-shortcut`. Revue : 0 bloquant/imp. ; 3 min. consignés (pas de `#Requires -Version 7` dans `bench.ps1`/`captures.ps1` ; double `ShortcutOverride` hors macOS, sans gestionnaire aujourd'hui ; horloge propre de `keyTimestamp`). Branche `essai-ci/L1-C4` supprimée |
| L1-C5 | L1-C3 | décision 23 : direction visuelle par le skill `impeccable` (critique + contrastes AA) | crit | --L1-C5 | en cours | 1/3 | — | — |
| L1-C2 | — | correctif : textes en carrés vides hors écran (captures, CI) | impl | (mergé) | terminée | 1/3 | 207c660 → merge 41a02ae | 44/44 TESTS OK (format compris), tst_ui_police Passed (rouge 3/3 avant) ; `CAPTURES OK`, `01-interface` : « Fichier Affichage Outils Aide », « 1 / 1 », « 91 % » lisibles. Cause : plateforme `offscreen` sous Windows sans dossier de polices (`Cannot find font directory …/lib/fonts`) → `QT_QPA_FONTDIR=%WINDIR%\Fonts` hors écran (captures.ps1, bench.ps1, actcraft_add_test). Limites (relecture) : police étroite au lieu de Segoe UI hors écran ; Linux/macOS non vérifiés ; `actcraft.exe` lancé à la main en offscreen toujours sans police |
| L1-C1 | — | correctif : curseur d'épaisseur qui ne suit pas `penWidth` (constat des captures Windows et Android) | impl | (mergé) | terminée | 1/3 | bf88764 → merge 945f7f9 | 43/43 TESTS OK, tst_ui_shell Passed (cas `widthSliderFollowsThePenWidth`), 0 « Binding loop » ; rouge vu (position 0,07 pour 8) ; `CAPTURES OK`, capture `02-trait` : curseur ≈ 45 %, aperçu rouge lisible. Cause : réglette linéaire 1-100, préréglages 2-8 → échelle log (écart à F-137 « linéaire », porte de fin ; préréglage max ≠ curseur tout à droite) |

## Terminé quand
lot final « confrontation aux vidéos » fait (écarts corrigés ou consignés) · tous les lots terminés (aucune tâche bloquée) · double relecture par lot sans bloquant · suite complète sans
régression (release + debug) · scénarios S1…Sn réussis · application avec interface jouée à l'écran (D3D11 et
logiciel) · installateur Windows `.exe` et `.apk` Android produits par la CI et vérifiés · PR(s) mergée(s) ·
CI verte · releases publiées seulement sur go écrit de Jérôme (action finale)

## Scénarios et captures
- S2 hors écran (L1-T15, rendu logiciel, 1280×800, `.tmp/captures-orch/` du worktree L1-T15, supprimé) : `02-trait` — barre du haut, palette à droite (16 pastilles, 4 épaisseurs, curseur, stylo actif), page blanche sur fond bleu, trait rouge horizontal ; pastille rouge entourée, épaisseur max choisie.
- **À traiter à la relecture du lot (constats de l'orchestrateur sur la capture)** : (a) tous les textes en carrés vides (« tofu ») hors écran — police absente de la plateforme `offscreen` (supposé, à vérifier à l'écran D3D11 ; si la CI capture hors écran, il faudra une police embarquée) ; (b) curseur d'épaisseur `widthSlider` tout à gauche alors que l'épaisseur max (`widthPreset-3`) est choisie — liaison suspecte dans `ui/qml/WidthPicker.qml` (L1-T12) ; (c) `05-plein-ecran` 800×800 hors écran (taille de l'écran virtuel).

## Hypothèses à revoir (niveau 1 d'abord)
- ~~N1 S3 en CI sans GPU~~ — levée par L1-T19 : `-gpu lavapipe` rend juste sur `ubuntu-latest` (run 38686895501 : `rectangles : entiers`, `ANDROID OK`) ; essai strict, aucun défaut prudent, aucune dépense.
- Trait en cours pendant un changement de page ou de vue (L1-C3) : le trait se termine à son dernier point et reste sur sa page ; la suite du même contact ne dessine plus. L'app Python garde le trait sur sa page mais continue d'y ajouter les points (invisibles) ; ActivInspire non interrogé. Décision de l'orchestrateur (défaut prudent), à présenter à la porte de fin.
- Scénario `premiers-pas` : suppose un départ fenêtré (défaut 1280×800) ; avec `-Taille plein-ecran`, le premier F5 sort du plein écran → ÉCART attendu.
- Réglette d'épaisseur logarithmique (L1-C1) au lieu de linéaire 1-100 (F-137) : à présenter à la porte de fin (autre choix : préréglages ou plage revus).

## Relecture de fin de lot 1 (tour 1, sur 8492731, sans L1-T19/T20)
Conformité : 0 bloquant. Constats et suite donnée :
- Imp. S1 joué hors écran (`essai-installateur.ps1` : offscreen + logiciel, `qoffscreen.dll` copié) au lieu de D3D11 + vraie fenêtre 5 s (plan l. 7060, 7100-7109) → confié à L1-T19 (fichier de sa table).
- Imp. décision 23 non tenue : skill `impeccable` non invoqué pour la direction visuelle (`docs/design/2026-10-14-direction-visuelle.md:4` contre `:39`) → tâche corrective (après la relecture du code).
- Imp. F5 : la preuve lit la propriété QML `fullScreen`, affectée avant `showFullScreen()` (`Main.qml:42`) ; rien ne lit `visibility` → tâche corrective.
- Min. `.gitignore` (*.jks, *.keystore, L1-T17) hors table : accepté par l'orchestrateur (protection de la signature).
- Min. DA8 annonce clics, molette, gestes : le joueur (`uiscenario.h:25-35`) ne fait que commande, trait, touche, attente, capture, pixel, lecture → écart déclaré ici, reporté au lot qui en aura besoin (S2 n'en a pas besoin).
- Min. JSON du banc hors écran absent de l'état pour L1-T7…T10 (session précédente) et L1-C1 (`2026-10-15_061746-…-software.json`, content_check.ok) : couvert par le banc complet de clôture.
- Min. `paquet.ps1` sans ligne `PAQUET EN ECHEC` (contrat l. 181-182) → confié à L1-T19.
- Non vérifié par la relecture : licence de `Qt6QuickControls2FluentWinUI3StyleImpl` ; traduisibilité des libellés JSON (N2-2).
Défauts du code : 0 bloquant. Suite donnée :
- Imp. stylet capté sous les barres de défilement (`pageview.cpp:317-322`, `PageArea.qml:257-285`) → L1-C3.
- Imp. changement de page (PgDown, doigt sur « page suivante ») ou de vue (F5, ajuster) pendant un trait : trait versé à la nouvelle page / qui saute (`Main.qml:138-153`, `documentcontroller.cpp:121-160`, `penadapters.cpp:28-31`) → L1-C3.
- Imp. `essai-installateur.ps1:74,66` : sortie rapide de l'app → exception, pas de nettoyage ni de ligne EN ECHEC → L1-T19.
- Min. molette qui quitte « Ajuster » sans mouvement (`pageview.cpp:509`) ; `fullScreen` ne suit pas `visibility` (`Main.qml:41-48`) ; `reload()` remet les coches à zéro (`commands.cpp:39`) → L1-C3.
- Min. attentes sans borne (`essai-android.ps1:55`, `essai-installateur.ps1:65`) → L1-T19.
- Min. `Join-Path $env:WINDIR` sans garde Windows (`bench.ps1:33`, `captures.ps1:33`) ; `tst_captures_premiers_pas` et `tst_bench_smoke` sans `QT_QPA_FONTDIR` (`tests/bench.cmake`) ; `captures.ps1:28` scénario inconnu sans `CAPTURES EN ECHEC`, dossier vide → `CAPTURES OK` → tâche corrective L1-C4 (après L1-C3/L1-T19).
- Décision 23 (`impeccable`) → tâche corrective L1-C5 (après L1-C3 : Theme/QML).

## Hors périmètre
- (L1-T8) `canStartStroke` ne vérifie que la zone de la vue : une palette flottante au-dessus de la page laisserait le stylet tracer dessous → lot 3.
- (L1-T8) Rejet de la paume au-delà du refus du toucher pendant un trait de stylet (corrigé par la revue de L1-T8) → lot 3 (gestes).
- (L1-T8) `scrollChanged` non émis quand la taille du contenu change seule.

## Portes
ouvertes : — · passées : —

## Pièges trouvés
- Construction d'APK : un démon Gradle (`java.exe` de `~/.tools/jdk-21`) reste vivant après `paquet-android.ps1` et verrouille le worktree (`git worktree remove` → « Invalid argument »). L'arrêter par PID (`Stop-Process -Id`) avant de supprimer le worktree. Vu le 2026-10-15 (L1-T17). Pour la CI : `--no-daemon` ou `org.gradle.daemon=false` à envisager.
- `tst_bench_smoke` (debug) prend ≈ 25 s et a un délai de 60 s ; sous charge (autre agent qui construit en release et joue le banc), il dépasse et `tst_bench_result` (qui en dépend par fixture) échoue sans message dans la sortie de `test.ps1`. Vu le 2026-10-15 sur L1-T17. Rejouer la suite quand la machine est calme ; pour le détail : `ctest -R '^tst_bench_result$' --output-on-failure`.
- Orchestrateur : un `Set-Location <worktree de tâche>` dans une commande déplace le dossier courant de la SESSION ; le dossier reste alors verrouillé et `git worktree remove` échoue (« Invalid argument » / « Permission denied »). Vu 2026-10-15 sur L1-T16 et L1-T12. Toujours `git -C` et chemins absolus ; si besoin d'un dossier courant, `pwsh -NoProfile -Command "Set-Location …; …"` (process enfant).
- QML : `while (count > 0) removeItem(itemAt(0))` sur un `Menu` boucle sans fin (fil principal bloqué, aucune fenêtre, `--quitter-apres` sans effet) ; entrées créées par `createObject(null)` survivent au menu. Remplacé par `Instantiator` (L1-T12, b1318f9). Candidat au coffre (`04_Pieges/`) — cause exacte non isolée entre les deux boucles.
- Hook `interdits.js` : `git -C <dossier> add .` N'EST PAS bloqué (le motif `bgits+adds+(-A|--all|.)` ne voit pas `-C`) — même trou que les tags avant L1-T1. Vu le 2026-10-15 (commits WIP, contenu vérifié conforme). À corriger dans le bloc `interdits` comme L1-T1 l'a fait pour `action-finale`.
- Banc `-HorsEcran` sans `-Taille` : la fenêtre « plein-ecran » hors écran n'a pas la taille attendue, `witness-image` = `hors-image` → `content-check-failed`. Toujours passer `-Taille 1280x800` hors écran ; à corriger dans le banc (L1-T14 : `-HorsEcran` implique une taille fixe).
- Qt Quick retarde les `TouchUpdate` jusqu'à l'image suivante : un test qui mêle doigt et stylet doit laisser passer une image (L1-T8, 50 ms ; répété 20× sans échec).
- `tst_bench_verdict` instable (revue L1-T9, reproduit : 8 échecs sur 16 en parallèle) : `tests/tst_bench_verdict.cpp:589,593` exige `checked == 3`, dépendant du temps réel ; `releaseCoverage` écarte déjà ce cas. Correction pour L1-T14 : injecter un trait à la fois et attendre (QTRY_VERIFY) qu'une synchronisation couvre le relâché ; à défaut `checked >= 1`, `lastSeq == 19` conditionnel, `RUN_SERIAL`.
- Pour le plan du lot 2 (revue L1-T9) : défaire le relais `DocumentController` (syncFromView, page possédée par l'élément, `document()` non constant, cible de `FixedPageSink`, ajout de page en commande annulable) ; réécrire `documentFollowsEveryWriterOfTheShownPage` ; ajouter un scénario de banc « changement de page » (éclair blanc tant que les images ne sont pas prêtes).
- Qt 6.12 Android exige JDK 21 (pas 17), plateforme android-36, build-tools 36.0.0, NDK 27.2.12479018, API min 28 (L1-T2). La CI devra installer JDK 21.
- Motifs `action-finale` du profil en vigueur sur main (vérifié sous node, 2026-10-14) : `git -C wt tag v0.1.0`, `git tag -a v…`, `git push origin --tags`, `git push -u origin v…` NE sont PAS bloqués par `bgits+tags+vd` ni `bgits+pushs+S+s+(refs/tags/)?vd`. Corrigé par L1-T1. En attendant : ne jamais taguer, quel que soit l'outil. Candidat au coffre (vaut pour tous les projets qui ont ce motif).
- Corpus vidéos (2026-10-14) : 144 vidéos, 144 fiches + README dans `REA - Activinspiredocsideos-tanguay` ; 18 sans contenu accessible (titre + description seulement) ; 631 principes relevés, 168 absents de `fonctionnalites.md` (137 sur le logiciel), 12 écarts avec la spec. Vidéos de 2011-2013 (ActivInspire 1.4-1.6), la spec décrit la 3.5.18 : un écart peut venir de la version. Lot 9 : corpus complet.
- Leçons du chantier précédent (prototype-mesure) : règles communes des tâches dans le scratchpad
  (`regles-tache.md`) ; jamais `ctest` ni `tst_*.exe` sans `dev-env.ps1` (boîte DLL bloquante) ; supprimer un
  worktree depuis un autre dossier courant ; relire les tâches `concepteur-critique` ; limite de 40 tours des
  relecteurs : leur demander de conclure.
