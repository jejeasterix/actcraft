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
| L1-T14 | T12 | banc adapté | crit | — | à faire | 0/3 | — | — |
| L1-T15 | T12,T13 | scénario premiers-pas | impl | --L1-T15 | en cours | 1/3 | — | — |
| L1-T16 | T2,T11 | installateur Inno Setup | impl | (mergé) | terminée | 2/3 | 6bdf6a4 → merge fd9ffee | `LICENCES OK`, `PAQUET OK : …\build\paquet\ActCraft-0.1.0-setup.exe`, `--version : ActCraft 0.1.0`, `INSTALLATEUR OK` (code 0) ; rouge vu (DLL fictive) ; essai en échec nettoie tout (5 Test-Path False, 6bdf6a4) ; aucun C++ ni test touché |
| L1-T17 | T2,T11,T16 | chaîne Android | crit | --L1-T17 | en cours | 1/3 | — | — |
| L1-T18 | T12,T17 | essai APK émulateur | impl | — | à faire | 0/3 | — | — |
| L1-T19 | T14,T15,T16,T18 | CI paquets | crit | — | à faire | 0/3 | — | — |
| L1-T20 | T19 | release.yml, CHANGELOG | crit | — | à faire | 0/3 | — | — |

## Terminé quand
lot final « confrontation aux vidéos » fait (écarts corrigés ou consignés) · tous les lots terminés (aucune tâche bloquée) · double relecture par lot sans bloquant · suite complète sans
régression (release + debug) · scénarios S1…Sn réussis · application avec interface jouée à l'écran (D3D11 et
logiciel) · installateur Windows `.exe` et `.apk` Android produits par la CI et vérifiés · PR(s) mergée(s) ·
CI verte · releases publiées seulement sur go écrit de Jérôme (action finale)

## Scénarios et captures
—

## Hypothèses à revoir (niveau 1 d'abord)
—

## Hors périmètre
- (L1-T8) `canStartStroke` ne vérifie que la zone de la vue : une palette flottante au-dessus de la page laisserait le stylet tracer dessous → lot 3.
- (L1-T8) Rejet de la paume au-delà du refus du toucher pendant un trait de stylet (corrigé par la revue de L1-T8) → lot 3 (gestes).
- (L1-T8) `scrollChanged` non émis quand la taille du contenu change seule.

## Portes
ouvertes : — · passées : —

## Pièges trouvés
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
