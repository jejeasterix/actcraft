# État — livraison-complete-b

Statut : en cours — ouvert le 2026-10-18 par l'orchestrateur de `livraison-complete`, sur la décision de Jérôme (parallélisation A, B, C : « Ok, pour À,B et C. Je te laisse entièrement à la manœuvre », relayé mot pour mot par la session Pilote)
Gabarit : lots (lot 7 du chantier `livraison-complete`, exécuté en parallèle du lot 3)
Étage : 3 (phase 2 du lot 7, vagues 6 à 8 ; phase 1 terminée et mergée ; plan de phase 2 : docs/plans/2026-10-18-livraison-complete-lot7-phase2-amendement.md @ 141411d)
Worktree : C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b   Branche : feat/livraison-complete-b   Base : feat/livraison-complete @ 7bb551c (+ merge du plan du lot 7 bf8c930 → 2d76ba6)
Branche principale locale au lancement : main @ 9563919
Cadrage : docs/specs/2026-10-14-livraison-complete-cadrage.md + amendement 1 (2026-10-17)   Plans : docs/plans/2026-10-18-livraison-complete-lot7-PLAN.md (7a) et …-lot7b-PLAN.md (7b), bf8c930, relus et corrigés hors socle
Chantier parent : `livraison-complete` (état : C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete\docs\sessions\livraison-complete-etat.md, lecture seule)
Lot : 7 « Présentation et outils » — 13/33 tâches terminées
Tâche : L7-T13 — Origine XY (F-284) — depuis 2026-10-21 01:17
Tâche : L7-T8 — Effets de changement de page II (navigateur, scénario effets) — depuis 2026-10-21 02:37
Questions à Jérôme : —

## Redémarrage du PC (2026-10-18 vers 22h53, cause probable : mémoire)
Toutes les tâches de fond mortes. Intégrité contrôlée : aucun index.lock, fsck propre, jetons libres ; .cpp sortis pour un rouge remis dans P1, P5, P9 ; wip sains recommités (P1 742ae60, P3 4aa56a5, P7 047f5e1). Dossiers build gardés : à effacer et reconstruire à froid à la première erreur suspecte (LNK1136, fichier tronqué, ninja corrompu).
Parallélisme réduit (demande de Jérôme « mais soit prudent », relayée par Pilote) : au plus 3 sous-agents à la fois, un seul qui construit, jusqu'aux conclusions de l'étude mémoire.
Suite de la branche du chantier après P0+P2+P3+P7 : 110 tests `TESTS OK` (aucune collision entre tâches). En cours : P1, P5, P8 reprises (constructions une à une par le verrou B). Restent P6 et P9.
- Sous forte charge, `tst_bench_verdict` (chien de garde 300 s) et `tst_bench_result` ont aussi échoué une fois (vu par P7) ; `tst_captures_aller_retour_natif` échoue souvent : le rejeu le relance seul pour trancher.
Amendement phase 2 : `docs/plans/2026-10-18-livraison-complete-lot7-phase2-amendement.md` @ 141411d, confronté au socle réel et relu trois fois contre lui (scratchpad revue-phase2-socle-t1..t3, borne atteinte, derniers correctifs appliqués sans 4e tour) ; 33 tâches (P2-A : L7-S8, T1–T19 ; P2-B : T21, T23–T34) ; exécutable unique `tst_ui_lot7` ; N_F = 129 puis 158 après L7-S8 ; démarre quand L7-P0 à P9 sont toutes mergées et la suite du chantier verte.

## Reprise après le temps calme (2026-10-18 vers 22h30, redémarrage de l'app demandé par Jérôme)
Mergé dans `feat/livraison-complete-b` : plan phase 1 (13ebe57), L7-T0 (f0772b6), L7-P0 (d163449, graphe dfc8857).
Tous les sous-agents et constructions de B sont arrêtés ; chaque worktree L7-P a un commit (final pour P2, wip pour les autres), rien n'existe seulement en mémoire.
Outils de la session (copie hors scratchpad, non suivie) : `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b\.tmp\outils-b` — `consignes-P.md` (consignes communes des tâches L7-P), `b-jeton.ps1` (verrou à une place pour B, puis jeton), `rejeu.ps1` (suite + Totals de tests nommés).
Premier geste à la reprise :
1. Relire ce fichier, `consignes-P.md`, et le dossier des addendums de A (`C:\Users\JEROME~1\AppData\Local\Temp\claude\C--Users-JeromeDenis-ActCraft\fd5ef06a-7b9b-46ba-a8c3-b06809fefd99\scratchpad\brief-b-addendum-*.md`).
2. `jeton.ps1 -Nom tests -Etat` (les places de PID mort se reprennent seules).
3. Rejouer L7-P2 (ee87059, `.worktrees/livraison-complete-b--L7-P2`) : `b-jeton.ps1 -Proprietaire livraison-complete-b/L7-P2-rejeu -Commande "pwsh -NoProfile -File '<outils>\rejeu.ps1' -Tache L7-P2 -Tests tst_features_pagezoom,tst_features_transitions"`, lire `.tmp/rejeu.txt` et `.tmp/rejeu-*.txt` (Totals 8 et 10), puis merge `--no-ff`, graphe.
4. Relancer un agent par tâche wip (P1 implementeur-tdd, P3/P7/P8 concepteur-critique, P5/P6/P9 implementeur-tdd), chacun dans son worktree : « reprends depuis ton commit wip ; rouge à montrer (construction sans tes .cpp), vert, suite par `b-jeton.ps1`, `NOMS OK`, nouveau commit (pas d'amend) », avec les consignes communes (chemins du scratchpad à remplacer par `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b\.tmp\outils-b`). Une seule place de tests pour B jusqu'au message de A pour la 2e.
5. Après le merge de L7-P3 : lancer L7-P4 (worktree neuf depuis la branche du chantier).

## Référence
Rapide (phase 2) : 158 tests `TESTS OK` après L7-S8 ; N_F = 129 tests `TESTS OK` (socle + phase 1, 2026-10-19). Avant le socle : N0 = N1 = 94 tests, `TESTS OK` (debug, 2026-10-18, worktree du chantier @ e367657).

## Compteurs
relecture plan lot 7 : 1/3 (tour 1 : 4 bloquants renvoyés au socle L0-S, I1-I9 et mineurs corrigés en bf8c930) · relecture amendement phase 1 : 2/3 · relecture amendement phase 2 (brouillon) : 3/3 · relecture contre le socle réel : 3/3 · vagues sans progrès 0/2

## Tâches
| id | dépend | fichiers touchés | worktree | état | essais | dernier commit vert | preuve rejouée |
|----|--------|------------------|----------|------|--------|---------------------|----------------|
| amendement phase 1 | — | docs/plans/2026-10-18-livraison-complete-lot7-phase1-PLAN.md | chantier (partagé) | terminé : accepté au tour 2, mineurs appliqués (13ebe57) ; 10 tâches L7-P0…P9, vagues P0 · P3‖P7 · P8‖P1 · P4‖P2 · P5‖P6 · P9 | 1 | 13ebe57 | relecture t1 (2 B, 2 I corrigés), t2 accepter |
| L7-T0 | — | docs/verifications/2026-10-18-lot7-principes.md ; hors dépôt REA docs/re/lots/lot7.md | supprimé | terminée, mergée (f0772b6), graphe efee804 | 3 | d16edb5 | NOMS OK 333, git grep du rapport vide, relevé présent, revue : 4 importants corrigés |
| L7-P0 | amendement relu | voir ligne L7-P0 du plan phase 1 | supprimé | terminée, mergée (d163449), graphe dfc8857 | 1 | 2747662 | rejeu : 110 tests, `TESTS OK`, 16 tst_features Passed ; fin de tests/CMakeLists.txt identique au socle (md5) |
| L7-P1 | L7-P0 | voir ligne L7-P1 du plan phase 1 | supprimé | terminée, mergée | 2 | c9fb98d | rejeu : 110 tests `TESTS OK`, Totals 11/8/7 ; 7 mutants rouges |
| L7-P3 | L7-P0 | voir ligne L7-P3 du plan phase 1 | supprimé | terminée, mergée (28a3334), graphe 85e9148 | 2 | 65047e0 | rejeu : 110 tests `TESTS OK`, Totals 13, `NOMS OK` ; 7 mutants tués ; revue adversariale « corriger, peu » appliquée |
| L7-P7 | L7-P0 | voir ligne L7-P7 du plan phase 1 | supprimé | terminée, mergée (ad52605), graphe 30b9c8b | 3 | 6038df0 | rejeu : 110 tests `TESTS OK`, Totals 13 et 7 ; 8 mutants rouges ; 2 revues adversariales appliquées |
| L7-P8 | L7-P0 | voir ligne L7-P8 du plan phase 1 | supprimé | terminée, mergée | 3 | 2302dc6 | rejeu : 110 tests `TESTS OK`, Totals 7 sans SKIP (reconnaisseur français du poste), grep COM limité à handwriting_win.cpp, `NOMS OK` ; mutants QThreadPool et alternatives vides rouges ; P8-m6 (reconnaisseur muet → échec, pas SKIP) consigné |
| L7-P4 | L7-P3 | voir ligne L7-P4 du plan phase 1 | supprimé | terminée, mergée | 1 | 55f45e6 | rejeu : 110 tests `TESTS OK`, Totals 22, `NOMS OK` ; revue adversariale appliquée (essai 2) |
| L7-P2 | L7-P0 | voir ligne L7-P2 du plan phase 1 | supprimé | terminée, mergée (827f028), graphe c606f05 | 2 | df41743 | rejeu : 110 tests `TESTS OK`, Totals 10 (pagezoom) et 12 (transitions) ; rouge vu sur ee87059 (3 échecs) |
| L7-P5 | L7-P0 | voir ligne L7-P5 du plan phase 1 | supprimé | terminée, mergée | 2 | e5f5874 | rejeu : 110 tests `TESTS OK`, Totals 7/11/8, `NOMS OK` ; 6 mutants rouges |
| L7-P6 | L7-P0 | voir ligne L7-P6 du plan phase 1 | supprimé | terminée, mergée (33e9b1a), graphe ea055dd | 2 | 5c2fd21 | rejeu : 110 tests `TESTS OK`, Totals 9, `NOMS OK` ; mutants rouges |
| L7-P9 | L7-P0 | voir ligne L7-P9 du plan phase 1 | supprimé | terminée, mergée (aef50b8), graphe cd73acb | 2 | a65e9b2 | rejeu : Totals 7/7/4, `NOMS OK` ; suite 108/110 sous charge, `tst_ui_files` et `tst_captures_aller_retour_natif` verts relancés seuls (P9 ne touche que features/) |
| L7-S8 | phase 1 mergée | voir ligne L7-S8 de l'amendement phase 2 | supprimé | terminée, mergée (0dfd461), graphe 44e8871 | 2 | 9f1604c | rejeu : 158 tests `TESTS OK`, talons `SKIP … Talon L7-S8 : rempli par L7-T<n>`, `NOMS OK` ; revue adversariale appliquée (essai 2) |
| L7-T1 | L7-S8 | voir ligne L7-T1 de l'amendement phase 2 | .worktrees/livraison-complete-b--L7-T1 (base 44e8871) | mergée : rejeu 157/158 (tst_bench_result rouge sous charge, vert seul), Totals 26 | 1 | 0c07ced | 6d0f999 (graphe f8e4678) |
| L7-T2 | L7-S8 | voir ligne L7-T2 de l'amendement phase 2 | .worktrees/livraison-complete-b--L7-T2 (base 44e8871) | mergée : rejeu TESTS OK 158/158, tst_ui_presentation Totals 21 (0 skipped), tst_core_document 21 ; revue : 3 mutants sans test → tests ajoutés (essai 2) ; banc hors écran BUDGET DEPASSE (p95 page 268 ms sous charge, sans référence jouée : résultat, pas porte) ; hors périmètre : police « sans-serif 14pt » en dur dans insertText (à arbitrer avec N2-3 du lot 4), observateur reconnu par indice de page (fragile si « supprimer la page ») | 2 | edd3763 | 738678b (graphe 4da6902) |
| suite V2 | L7-T1, L7-T2 | branche du chantier à b78afad | — | TESTS OK 158/158 (fin de la vague 2) | 1 | — | — |
| L7-T3 | L7-S8 | voir ligne L7-T3 de l'amendement phase 2 | .worktrees/livraison-complete-b--L7-T3 (base 7877179) | mergée : rejeu TESTS OK 158/158, curtain Totals 6 (0 skipped), captures rideau OK ; banc BUDGET DEPASSE (résultat) | 1 | 4e08495 | 8622ebb (graphe a5320e9) |
| L7-T9 | L7-S8 | voir ligne L7-T9 de l'amendement phase 2 | .worktrees/livraison-complete-b--L7-T9 (base 7877179) | mergée : rejeu 157/158 (tst_pageview rouge sous charge, vert seul), mathtools Totals 12 (0 skipped) ; revue : 4 importants corrigés à l'essai 2, test de la sélection reporté en L7-T34 ; couverture au bord extérieur (lettre du contrat, décision orchestrateur) | 2 | c85e866 | e373edb (graphe 319d88c) |
| suite V3 | L7-T3, L7-T9 | branche du chantier à 0c0b0e2 | — | TESTS OK 158/158 (2e passage ; 1er : LNK2001 AUTOMOC, timestamp ui supprimés) | 2 | — | — |
| L7-T4 | L7-T2 | voir ligne L7-T4 de l'amendement phase 2 | .worktrees/livraison-complete-b--L7-T4 (base 0c0b0e2) | mergée : rejeu TESTS OK 158/158, spotlight Totals 9 (0 skipped), captures spot OK, 5 mutants ; banc BUDGET DEPASSE (résultat) ; HoverHandler pleine page de MathToolsLayer contourné → correction demandée à L7-T11 | 2 | 3182bcc | 928db8e (graphe 78ac3fb) |
| L7-T10 | L7-T9 | voir ligne L7-T10 de l'amendement phase 2 | .worktrees/livraison-complete-b--L7-T10 (base 0c0b0e2) | mergée : rejeu TESTS OK 158/158, mathtools Totals 18 (0 skipped), 4 mutants un par construction ; Repeater → ListModel ; banc BUDGET DEPASSE (résultat) | 1 | d9b6b36 | dd7f2f2 (graphe 2ca164f) |
| L7-T5 | L7-S8 | voir ligne L7-T5 de l'amendement phase 2 | .worktrees/livraison-complete-b--L7-T5 (base e8d01f0) | mergée : rejeu TESTS OK 158/158, pagezoom Totals 9 (0 skipped) ; mutants A, C, D rouges, B équivalent (coveredAt compte le PointHandler, addInputCover redondant mais gardé) ; banc BUDGET DEPASSE (résultat) ; commit réécrit par --amend (non poussé) | 2 | e2cad14 | 990380b (graphe ca238a1) |
| L7-T11 | L7-T10, L7-T2 | voir ligne L7-T11 de l'amendement phase 2 | .worktrees/livraison-complete-b--L7-T11 (base c3d3ed1) | mergée : rejeu TESTS OK 158/158, mathtools Totals 26 (0 skipped) ; revue « on peut merger », retouches faites à l'essai 2 ; hors périmètre : page quittée pendant un trait fermé → arc sans rayons (:668, sans test) | 2 | f12fe3c | df98a5f (graphe 6568c10) |
| suite V4 | L7-T4, L7-T10 | branche du chantier à 63b2ad8 (timestamp AUTOMOC ui supprimés avant) | — | TESTS OK 158/158 (2026-10-20, après le redémarrage) | 1 | — | — |
| suite V6 | L7-T11, L7-T5, L7-T6 | branche du chantier à 48fc294 (timestamp AUTOMOC supprimés avant) | — | TESTS OK 158/158 | 1 | — | — |
| L7-T6 | L7-T1, T2, T3, T4 | voir ligne L7-T6 de l'amendement phase 2 (+ « spot ferme le rideau ») | .worktrees/livraison-complete-b--L7-T6 (base f0b30ce) | mergée : rejeu TESTS OK 158/158, pagetools 9, features_pagetools 9, flipchart 27 (0 skipped) ; revue corriger → essai 2 → contre-revue merger ; H7-16 tenue ; restent mineurs (preuves) : passage à -1 sans mutant propre, relâché sans déplacement, garde « page en mode spot », page changée pendant le glisser ; écarts : cadre non appliqué au spot (setFrame absent), coches present.spotlight-* périmées | 2 | 8d6e94f | 615e65a (graphe 238d2a6) |
| L7-T12 | L7-T11 | voir ligne L7-T12 de l'amendement phase 2 | .worktrees/livraison-complete-b--L7-T12 (base 530e95b) | mergée : rejeu TESTS OK 158/158, mathtools Totals 34 (0 skipped) ; revue corriger → essai 2 ; charnière déplace (décision orchestrateur) ; hors périmètre : rotation, rayon, « Réfléchi », longueur et relecture des réglages non testés ; mine allumée pour tout outil (lot 3, L7-T34) | 2 | de4c4a5 | c298c5e (graphe 339fa48) |
| L7-T7 | L7-T3 | voir ligne L7-T7 de l'amendement phase 2 | .worktrees/livraison-complete-b--L7-T7 (base cd09af4) | mergée : rejeu TESTS OK 158/158, transitions Totals 16 (0 skipped) ; revue corriger → essai 2 ; Presentation.qml : une liaison spotlight ajoutée (exception de l'orchestrateur) ; points S22 notés (mouvements approximés) | 2 | fc0e9d0 | 9458c70 (graphe c74c9ca) |
| L7-T14 | L7-T2 | voir ligne L7-T14 de l'amendement phase 2 | .worktrees/livraison-complete-b--L7-T14 (base b3f0e76) | mergée : rejeu TESTS OK 158/158, dice Totals 9 (0 skipped), 5 mutants ; TapHandler stylet refusé par l'orchestrateur (double clic), appui au stylet testé comme Qt | 1 | 95f3bc3 | abae189 (graphe 2b12cb7) |
| L7-T13 | L7-T12 | voir ligne L7-T13 de l'amendement phase 2 | .worktrees/livraison-complete-b--L7-T13 (base ee24d12) | en cours (implementeur-tdd), V7 | 1 | — | — |
| suite V7 | L7-T12, L7-T7 | branche du chantier à 6367ade (timestamp AUTOMOC supprimés avant) | — | TESTS OK 158/158 | 1 | — | — |
| L7-T8 | L7-T7, L7-T2 | voir ligne L7-T8 de l'amendement phase 2 | .worktrees/livraison-complete-b--L7-T8 (base 6367ade) | en cours (implementeur-tdd), V8 | 1 | — | — |

## Terminé quand
toutes les tâches du lot 7 terminées (7a et 7b) · fusion L7-T33 faite · double relecture sans bloquant · suites debug et release sans régression · scénarios S10, S22, S23, S24 réussis · branche prête à fusionner dans `feat/livraison-complete` (la fusion et la PR sont faites par l'orchestrateur de `livraison-complete`)

## Socle fusionné (2026-10-19, message de A) — points à reporter dans l'amendement de phase 2
1. L0-S5 : `replaceDocument`, `loadGeneratedFirstPage`, `loadGeneratedPages` ne passent pas par `applyToCache` ; l'observateur (`addChangeObserver`) n'est pas appelé : le lot 7 écoute aussi `pagesChanged` ou `documentChanged`.
2. L0-S9 : la racine de `Presentation.qml` est un `QtObject` (conflit de `layer` avec `Item.layer` FINAL) ; les surcouches se posent par `parent: presentation.layer` ou `parent: presentation.pageOverlays`, jamais comme enfants visuels directs.
3. Démontage QML : tout QML neuf protège ses accès à `commands` (null au démontage) ; `tst_ui_shell::shellLoadsWithoutQmlWarning` échoue sur tout avertissement QML.
4. L0-S2 : étapes `touch`, `pinch`, `pointer`, `doubleClick` par `QTest::createTouchDevice`/`QTest::touchEvent` (un `QTouchEvent` posté n'atteint pas Qt Quick) ; `expect` accepte `tolerance` ; C++ de qmlcachegen de `actcraft_ui` en `/wd4702`.
5. Accord de A (L7-T2) : `documentReplaced` et `Document::removeResource` restent dans L7-T2 (lot 3 : seule L3-T12 touche `documentcontroller.cpp`, hors des trois fonctions). Règles : chaque `emit documentReplaced();` sur sa propre ligne à la fin des trois fonctions, sans réindenter ni déplacer de code voisin ; signal déclaré à la fin du bloc `signals:` de `documentcontroller.h`. À reporter dans l'amendement.
Preuve de A : debug et release 113/113 ; talons « par L7- » : 3 lignes (equation.h, equation.cpp, equationlayout.cpp). N attendu ici : 113 + 16 = 129 — confirmé : suite debug du chantier après fusion 129/129 `TESTS OK` (2e passe ; la 1re a échoué sur `qmlimportscanner` de `tst_ui_lift` sans message, passager), `NOMS OK` 487 fichiers.

## Écarts acceptés en phase 1
- L7-P3 : assertions ajoutées dans `guideSetPicksTheNearestGuide` (ordre inversé) et exercice de `SetSquare::follow`, Totals inchangés ; règle « le plus proche » du plan maintenue contre la référence Python.
- L7-P7 : cas à 45° ajoutés dans `tiltedEllipseKeepsItsAngle` et `tiltedRectangleKeepsItsCorners` (7b les reprenait mot pour mot) ; délai calculé sur 64 bits.
- L7-P8 : test renforcé (barrière contre l'attente bloquante, appartement à la destruction et à la création), 7 fonctions.
- L7-P1 : cache plein, pourtour avant intérieur.
- L7-P0 : talons formatés par clang-format (le plan les donnait non formatés).

- L7-P2 : 4 tests en plus gardés (Totals 10 pagezoom, 12 transitions au lieu de 8 et 10) ; le déplacement sous le seuil de panoramique n'est plus perdu (écart au plan).
- L7-P0 : revue statique « accepter » (3 mineurs sans effet : `\.` dans le motif CMake, 3 lignes au lieu d'une dans tests/CMakeLists.txt, préfixe `feat:` des messages).

- L7-P3 : angle affiché en sens trigonométrique, comme la référence Python (le plan l. 1273 disait l'inverse) ; vaut aussi pour rapporteur et compas (L7-P4).
- Revue statique des wip P7/P8 (scratchpad revue-wip-p7-p8.md) à transmettre aux agents de reprise : P7 géométrie rendue non vérifiée, `major`/`minor` macros ; P8 commentaires COM hors `_win.cpp`, receveur local dans les tests.

## À reporter en phase 2 (de L7-T0)
Sections à corriger des plans 7a/7b listées dans « Écarts pour l'orchestrateur » de `docs/verifications/2026-10-18-lot7-principes.md` : T3/T6 (position du rideau par page, propriété 27), T4 (capture de la zone, curseurs, contour 10), T9–T11 (curseurs par zone, translucidité 100), T12 (mine à la couleur du stylo), T14 (1 dé, mémorisé), T15 (transfert sur la page), T16/T19 (H7-14, H7-15), T20 (polygone), T21 (délai 1 s), T24–T27 (palettes d'équation). P20 (symétries autour de l'origine) → lot 5.

## Reports entre tâches de la phase 2
- « Ouvrir le spot ferme le rideau » (plan 7a, étape 3) : fait dans L7-T6 (PageToolsController), mergé.

## À vérifier à la clôture du lot (S22, captures à l'écran)
- Appui et survol au vrai stylet sur les panneaux (dés L7-T14, compas L7-T12, navigateur des effets L7-T8) : prouvés hors écran par le rejeu souris de Qt seulement ; à voir à l'écran. Navigateur des effets (L7-T8) : aucun moyen de retirer l'effet propre d'une page pour revenir à l'effet global.
- Effets de page (L7-T7) : six mouvements approximés sans rotation (box-turn, card-flip, tiles-spin, corner-swing, spiral-out, turn-reveal) ; slide-right sans poussée ; star-iris en quatre quarts. À regarder à l'écran, à corriger dans une tâche dédiée si besoin.
- Effets de page (L7-T7) : `known()` duplique la logique de `features` (un `isKnownEffect()` dans features serait la bonne place).

## À faire en L7-T34 (après la fusion du lot 3)
- Survol au stylet des outils de maths (L7-T12) : prouvé hors écran seulement par le rejeu en souris de Qt (`QTabletEvent` sans bouton n'atteint pas les `HoverHandler`) → à voir sur un vrai stylet dans les captures à l'écran (DA8) de la clôture ; et la mine du compas lit `PageView::inkColor` : à revoir quand le lot 3 remplira `ToolHost::color`.
- `selectionIsNotGuided` (L7-T9, revue) : le guide de la règle est réservé au stylo (`kGuidedTool = "pen"`), mais non testable avant le lot 3 : `ToolHost::activate` et `UserState::setCurrentTool` sont des talons. À ajouter avec `markerFollowsTheRuler`, `eraserFollowsTheCompass`.
- Constat L7-T9, hors périmètre : la commande `tool.pen` de `Main.qml:292` coche la commande sans activer le stylo (talon du lot 3 ?) : à revoir à la fusion.

## Reprise du 2026-10-20 à 16 h 00 (après le redémarrage du PC, message de Pilote)
Points 1 à 4 du point de reprise lancés : suite V4 sous le verrou (timestamp AUTOMOC supprimés), agents L7-T11 essai 2, L7-T5 reprise (point souris tranché : garder les tests, prouver par un mutant du chemin souris, couverture discriminée au stylet et au doigt seulement), L7-T6 reprise (run.ps1 sans garde d'heure). RAM libre 0,6 Go : longues attentes du jeton.

## Point de reprise (écrit le 2026-10-20 vers 04 h 30, avant l'arrêt du PC)
Mergé dans `feat/livraison-complete-b` : phase 1, L7-S8, vague 2 (T1, T2), vague 3 (T3, T9), vague 4 (T4, T10). Suites du chantier vertes après V2 et V3 ; **suite V4 non jouée**.
Aucune construction ni attente de B ne tourne ; tous les worktrees sont commités. Outils : `.tmp/outils-b/` (b-jeton.ps1, rejeu.ps1 avec la forme `exe:classe`, suite-chantier.ps1, consignes-T.md, revues).
Premiers gestes, dans l'ordre :
1. Revue de L7-T11 rendue (`.tmp/outils-b/revue-l7-t11.md`) : « on peut merger », aucun bloquant. Essai 2 court avant le rejeu : couche du trait reprise (`common.layer`, :143/:151) au lieu de `Layer::Top` ; tests de l'épaisseur des rayons et de l'absence de contour du secteur ; vrai mutant M1 (stylo modifié, armement gardé) ; arc armé sans trait versé à désarmer (sinon un surligneur recevrait les rayons) ; réindentation de `MathToolsLayer.qml` :54/:91. Question à trancher en relisant la section : un arc fermé coûte deux annulations (arc puis rayons) — conforme à la lettre selon la revue. Puis rejeu `-Tests tst_ui_lot7:mathtools` et merge.
2. Avant toute suite dans le worktree du chantier : supprimer `build/debug/ui/actcraft_ui_autogen/timestamp`, `build/debug/ui/actcraft_uiplugin_autogen/timestamp`, `build/debug/tests/tst_ui_lot7_autogen/timestamp` (piège AUTOMOC), puis suite V4 par `b-jeton.ps1 … suite-chantier.ps1`.
3. L7-T5 : relancer un agent implementeur-tdd sur 4cc93e0 (vert du dernier test, mutants B et C, suite, banc, NOMS OK) ; trancher le point « souris ».
4. L7-T6 : retirer la garde d'heure (04 h 30) de `.tmp/run.ps1` du worktree L7-T6, puis `b-jeton.ps1 -Proprietaire livraison-complete-b/L7-T6 -Commande "pwsh -NoProfile -File '<wt>\.tmp\run.ps1'"` ; lire `.tmp/etapes.txt` ; revue adversariale ensuite (concepteur-critique).
5. Rejouer L7-T11 (`-Tests tst_ui_lot7:mathtools`), merger ; puis L7-T12 (dépend de T11) en parallèle de L7-T7 (dépend de T3).

## Arrêt du PC (annonce de Jérôme du 2026-10-19 vers 20 h 05, relayée par Pilote 2)
« Organise le travail pour faire une pause à 5h30 du matin au plus tard demain car je dois prendre mon PC et je pars à 5h45 du matin. » Après 04 h 30 le 20/10 : rien de long lancé. À 05 h 15 : rien en cours, tout commité, point de reprise exact ici. Rappels de session posés à 04 h 27 et 05 h 03. Heure de reprise inconnue.

## Portes
ouvertes : — · passées : —

## Pièges trouvés
- Qt (à porter au coffre, 04_Pieges) : une propriété par défaut déclarée dans un fichier QML ne sert qu'aux objets écrits là où le type est utilisé (ici `Main.qml`) ; dans le fichier lui-même, assigner par nom (`data: [ … ]`). Erreur vue : `Cannot assign to non-existent default property` (L7-S8).
- CI Clang : `-Wunused-private-field` sur `m_controller` du talon `ui/src/presentationdocument.cpp` levé par L7-T2 (mergée).
- Qt/CMake (à porter au coffre, 04_Pieges) : un en-tête `QML_ELEMENT` créé après un premier build n'est pas vu par AUTOMOC (`CurtainController is not a type`) ; remède : supprimer `build/debug/ui/actcraft_ui_autogen/timestamp` (L7-T3). QML : `Item.mapFromScene` absent, utiliser `mapFromItem(null, p)` (L7-T3). Revu aussi dans le build incrémental du chantier après le merge de L7-T9 (LNK2001 `MathToolsController::staticMetaObject`) : touche tout build existant qui reçoit un nouvel en-tête, donc aussi A à la fusion du lot 7 → à dire à A dans le message « lot 7 prêt à fusionner ».
- Consignes communes de la phase 2 : scratchpad `consignes-T.md` (verrou B, tests instables sous charge à relancer seuls : `tst_captures_aller_retour_natif`, `tst_ui_files`, `tst_pageview`, `tst_bench_verdict`, `tst_bench_result`).
- Places de tests : 2e place accordée par Pilote le 2026-10-18 vers 22h (Jérôme lui a laissé la manœuvre) ; A reste prioritaire, retour à une place dès qu'A en a besoin.
- Parallélisme (demande de rythme de Pilote, consigne de Jérôme dans la tâche planifiée) : 4 agents écrivent en parallèle, les constructions passent une à une par le jeton ; consignes communes : scratchpad `consignes-P.md` (temps 1 sans P0, temps 2 après merge de P0).
- Jeton `tests` : exige 3,5 Go de RAM libre ; la machine tourne autour de 2-3 Go pendant les suites de A, attente longue (une attente expirée, code 3).
- Le dépôt contenait deux titres de vidéos dans 7 fichiers : corrigé par A (f167eb9 sur feat/livraison-complete, `file.new` = « Créer un paperboard ») ; arrive ici par la fusion de phase 2 (ne pas fusionner avant le socle).
- `tst_captures_aller_retour_natif` échoue sous charge (`BOUCLE max=… ECART`, seuil 25 ms), base comprise : pas une régression de B ; A le fiabilise.
(voir aussi « Pièges trouvés » du chantier parent)
