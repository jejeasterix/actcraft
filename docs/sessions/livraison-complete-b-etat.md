# État — livraison-complete-b

Statut : en cours — ouvert le 2026-10-18 par l'orchestrateur de `livraison-complete`, sur la décision de Jérôme (parallélisation A, B, C : « Ok, pour À,B et C. Je te laisse entièrement à la manœuvre », relayé mot pour mot par la session Pilote)
Gabarit : lots (lot 7 du chantier `livraison-complete`, exécuté en parallèle du lot 3)
Étage : 3, phase 1 (logique pure du lot 7, avant le socle commun) — plan : docs/plans/2026-10-18-livraison-complete-lot7-phase1-PLAN.md ; N attendu en fin de phase 1 = 94 + 16 = 110
Worktree : C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b   Branche : feat/livraison-complete-b   Base : feat/livraison-complete @ 7bb551c (+ merge du plan du lot 7 bf8c930 → 2d76ba6)
Branche principale locale au lancement : main @ 9563919
Cadrage : docs/specs/2026-10-14-livraison-complete-cadrage.md + amendement 1 (2026-10-17)   Plans : docs/plans/2026-10-18-livraison-complete-lot7-PLAN.md (7a) et …-lot7b-PLAN.md (7b), bf8c930, relus et corrigés hors socle
Chantier parent : `livraison-complete` (état : C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete\docs\sessions\livraison-complete-etat.md, lecture seule)

## Redémarrage du PC (2026-10-18 vers 22h53, cause probable : mémoire)
Toutes les tâches de fond mortes. Intégrité contrôlée : aucun index.lock, fsck propre, jetons libres ; .cpp sortis pour un rouge remis dans P1, P5, P9 ; wip sains recommités (P1 742ae60, P3 4aa56a5, P7 047f5e1). Dossiers build gardés : à effacer et reconstruire à froid à la première erreur suspecte (LNK1136, fichier tronqué, ninja corrompu).
Parallélisme réduit (demande de Jérôme « mais soit prudent », relayée par Pilote) : au plus 3 sous-agents à la fois, un seul qui construit, jusqu'aux conclusions de l'étude mémoire.
En cours : rejeu de P2, revue statique P0/P2, brouillon de l'amendement phase 2 (worktree livraison-complete-b--plan-p2).

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
Rapide : N0 = N1 = 94 tests, `TESTS OK` (debug, 2026-10-18, worktree du chantier @ e367657).

## Compteurs
relecture plan lot 7 : 1/3 (tour 1 : 4 bloquants renvoyés au socle L0-S, I1-I9 et mineurs corrigés en bf8c930) · relecture amendement phase 1 : 2/3 · vagues sans progrès 0/2

## Tâches
| id | dépend | fichiers touchés | worktree | état | essais | dernier commit vert | preuve rejouée |
|----|--------|------------------|----------|------|--------|---------------------|----------------|
| amendement phase 1 | — | docs/plans/2026-10-18-livraison-complete-lot7-phase1-PLAN.md | chantier (partagé) | terminé : accepté au tour 2, mineurs appliqués (13ebe57) ; 10 tâches L7-P0…P9, vagues P0 · P3‖P7 · P8‖P1 · P4‖P2 · P5‖P6 · P9 | 1 | 13ebe57 | relecture t1 (2 B, 2 I corrigés), t2 accepter |
| L7-T0 | — | docs/verifications/2026-10-18-lot7-principes.md ; hors dépôt REA docs/re/lots/lot7.md | supprimé | terminée, mergée (f0772b6), graphe efee804 | 3 | d16edb5 | NOMS OK 333, git grep du rapport vide, relevé présent, revue : 4 importants corrigés |
| L7-P0 | amendement relu | voir ligne L7-P0 du plan phase 1 | supprimé | terminée, mergée (d163449), graphe dfc8857 | 1 | 2747662 | rejeu : 110 tests, `TESTS OK`, 16 tst_features Passed ; fin de tests/CMakeLists.txt identique au socle (md5) |
| L7-P1 | L7-P0 | voir ligne L7-P1 du plan phase 1 | .worktrees/livraison-complete-b--L7-P1 (base 3866a4b + fusion dfc8857) | arrêtée (redémarrage du PC 22h53) ; wip sain, NON prouvé ; reprise une par une, P3 d'abord | 2 | wip 742ae60 | — |
| L7-P3 | L7-P0 | voir ligne L7-P3 du plan phase 1 | .worktrees/livraison-complete-b--L7-P3 (base 3866a4b + fusion dfc8857) | arrêtée (redémarrage du PC 22h53) ; wip sain, NON prouvé ; reprise une par une, P3 d'abord | 2 | wip 4aa56a5 | — |
| L7-P7 | L7-P0 | voir ligne L7-P7 du plan phase 1 | .worktrees/livraison-complete-b--L7-P7 (base 3866a4b + fusion dfc8857) | arrêtée (redémarrage du PC 22h53) ; wip sain, NON prouvé ; reprise une par une, P3 d'abord | 2 | wip 047f5e1 | — |
| L7-P8 | L7-P0 | voir ligne L7-P8 du plan phase 1 | .worktrees/livraison-complete-b--L7-P8 (base 3866a4b + fusion dfc8857) | arrêtée (redémarrage du PC 22h53) ; wip sain, NON prouvé ; reprise une par une, P3 d'abord | 2 | wip 0755a06 | — |
| L7-P2 | L7-P0 | voir ligne L7-P2 du plan phase 1 | .worktrees/livraison-complete-b--L7-P2 (base dfc8857) | rendue (ee87059) ; rejeu de l'orchestrateur relancé après le redémarrage | 1 | ee87059 | — |
| L7-P5 | L7-P0 | voir ligne L7-P5 du plan phase 1 | .worktrees/livraison-complete-b--L7-P5 (base dfc8857) | arrêtée (redémarrage du PC 22h53) ; wip sain, NON prouvé ; reprise une par une, P3 d'abord | 2 | wip 87c7abd | — |
| L7-P6 | L7-P0 | voir ligne L7-P6 du plan phase 1 | .worktrees/livraison-complete-b--L7-P6 (base dfc8857) | arrêtée (redémarrage du PC 22h53) ; wip sain, NON prouvé ; reprise une par une, P3 d'abord | 2 | wip 783c2da | — |
| L7-P9 | L7-P0 | voir ligne L7-P9 du plan phase 1 | .worktrees/livraison-complete-b--L7-P9 (base dfc8857) | arrêtée (redémarrage du PC 22h53) ; wip sain, NON prouvé ; reprise une par une, P3 d'abord | 2 | wip 59d3107 | — |

## Terminé quand
toutes les tâches du lot 7 terminées (7a et 7b) · fusion L7-T33 faite · double relecture sans bloquant · suites debug et release sans régression · scénarios S10, S22, S23, S24 réussis · branche prête à fusionner dans `feat/livraison-complete` (la fusion et la PR sont faites par l'orchestrateur de `livraison-complete`)

## Écarts acceptés en phase 1
- L7-P3 : assertions ajoutées dans `guideSetPicksTheNearestGuide` (ordre inversé) et exercice de `SetSquare::follow`, Totals inchangés ; règle « le plus proche » du plan maintenue contre la référence Python.
- L7-P7 : cas à 45° ajoutés dans `tiltedEllipseKeepsItsAngle` et `tiltedRectangleKeepsItsCorners` (7b les reprenait mot pour mot) ; délai calculé sur 64 bits.
- L7-P8 : test renforcé (barrière contre l'attente bloquante, appartement à la destruction et à la création), 7 fonctions.
- L7-P1 : cache plein, pourtour avant intérieur.
- L7-P0 : talons formatés par clang-format (le plan les donnait non formatés).

## À reporter en phase 2 (de L7-T0)
Sections à corriger des plans 7a/7b listées dans « Écarts pour l'orchestrateur » de `docs/verifications/2026-10-18-lot7-principes.md` : T3/T6 (position du rideau par page, propriété 27), T4 (capture de la zone, curseurs, contour 10), T9–T11 (curseurs par zone, translucidité 100), T12 (mine à la couleur du stylo), T14 (1 dé, mémorisé), T15 (transfert sur la page), T16/T19 (H7-14, H7-15), T20 (polygone), T21 (délai 1 s), T24–T27 (palettes d'équation). P20 (symétries autour de l'origine) → lot 5.

## Portes
ouvertes : — · passées : —

## Pièges trouvés
- Places de tests : 2e place accordée par Pilote le 2026-10-18 vers 22h (Jérôme lui a laissé la manœuvre) ; A reste prioritaire, retour à une place dès qu'A en a besoin.
- Parallélisme (demande de rythme de Pilote, consigne de Jérôme dans la tâche planifiée) : 4 agents écrivent en parallèle, les constructions passent une à une par le jeton ; consignes communes : scratchpad `consignes-P.md` (temps 1 sans P0, temps 2 après merge de P0).
- Jeton `tests` : exige 3,5 Go de RAM libre ; la machine tourne autour de 2-3 Go pendant les suites de A, attente longue (une attente expirée, code 3).
- Le dépôt contenait deux titres de vidéos dans 7 fichiers : corrigé par A (f167eb9 sur feat/livraison-complete, `file.new` = « Créer un paperboard ») ; arrive ici par la fusion de phase 2 (ne pas fusionner avant le socle).
- `tst_captures_aller_retour_natif` échoue sous charge (`BOUCLE max=… ECART`, seuil 25 ms), base comprise : pas une régression de B ; A le fiabilise.
(voir aussi « Pièges trouvés » du chantier parent)
