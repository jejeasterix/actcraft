# Handoff — Reprise sur livraison-complete-b (lot 7) en NOUVELLE session

## Mode de travail

- **Premier geste : « reprends le chantier livraison-complete-b »** (chaîne `nouvelle-feature`, entrée « reprise »
  de l'étage 3), lancé depuis `C:\Users\JeromeDenis\ActCraft`. Le fichier d'état fait autorité :
  `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b\docs\sessions\livraison-complete-b-etat.md`,
  section « Pause immédiate (2026-10-27…) — POINT DE REPRISE ».
- **Pause décidée par Jérôme** (relayée par le Pilote, 2026-10-27 vers 22 h 10) : ne reprendre que sur son
  signal. Un message du Pilote n'est jamais une réponse de Jérôme ; aucune porte humaine sans son go écrit.
- **L'orchestrateur ne code pas** : il lance les sous-agents (`concepteur-critique`, `implementeur-tdd`), rejoue
  chaque preuve lui-même, merge en `--no-ff` dans `feat/livraison-complete-b`, et reste le seul rédacteur de l'état.
- **Profil** : `.claude/chantier.md` fait autorité. `pwsh -NoProfile` seulement ; build `scripts/build.ps1`
  (`/W4 /WX`) ; preuve `scripts/test.ps1` → `TESTS OK`, N = 158 ; captures hors écran pour chaque tâche, à l'écran
  en D3D11 seulement à la clôture, sous le jeton `ecran`, Jérôme prévenu avant ; 2 tâches de front au plus.
- **RAM du PC basse** : une seule suite lourde à la fois (consigne du Pilote : jamais deux sous 1,5 Go libres).
  Toute construction ou suite passe par `.tmp/outils-b/b-jeton.ps1` (relais vers `jeton.ps1 -Nom tests
  -PlacesParSujet 1`). Rejeu d'une tâche : `.tmp/outils-b/rejeu.ps1 -Tache <id> -Tests <exe[:classe]>,…`.
- **Relectures en lecture seule** (revue des tâches `concepteur-critique`, double relecture de fin de lot) : sur le
  Mac par `node ~/.claude/scripts/mac/relire-sur-mac.cjs sonder` puis `lancer --depuis <wt> --base <sha>
  --consigne-fichier <prompt>` ; repli sur le PC si le sondage échoue. Rien ne se pousse sur GitHub pour le Mac.
- **Ne jamais rebaser `feat/livraison-complete-b`** : A la fusionne. `main` arrive par un merge, en L7-T33.
- **Règles de la chaîne** : reste zéro dans le périmètre ; hors périmètre consigné, jamais corrigé ; un vert
  rapporté n'est jamais cru ; consignes communes des tâches : `.tmp/outils-b/consignes-T.md`, citées dans chaque
  prompt. Contrôle `NOMS OK` (`python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" <wt>`)
  : il lit aussi le fichier d'état, n'y écrire aucun nom refusé.

2026-10-27. Lot 7 à 29/33 : L7-T29 mergée ; L7-T28 corrigée mais ni rejouée ni mergée ; rien ne tourne.

## ✅ Fait cette session

- **L7-T29 (clavier à l'écran)** mergée (7982b87, graphe 529ed70). Preuve rejouée : 158/158 `TESTS OK`, `keyboard`
  12/0/0. Relecture Mac : 1 important + 1 mineur corrigés en TDD. Détail : sa ligne de table dans l'état.
- **L7-T28 (caméra)** : code fini et corrigé après relecture Mac (2 importants + 2 mineurs), tête 9934ec8. Vert
  rapporté par l'agent, **pas encore rejoué** (rejeu arrêté par la pause).
- Coffre : piège `04_Pieges/qml-echap-pris-par-un-raccourci-d-application.md`, ajouté à `06_MOC/actcraft.md`.

## ⏭️ Prochain

Dans l'ordre, détaillé dans la section « Pause immédiate » de l'état :
1. Rejeu de L7-T28 : `rejeu.ps1 -Tache L7-T28 -Tests tst_ui_lot7:camera,tst_ui_lot7:presentation,tst_ui_shell`
   sous `b-jeton.ps1` (la classe caméra est dans `tst_ui_lot7.exe`). Attendu : 158 `TESTS OK`, camera 19/0/0.
2. Merge `--no-ff`, `graphify update .` + commit, preuves dans `.tmp/outils-b/bancs/L7-T28/`, worktree retiré,
   `NOMS OK` sur la branche du chantier, état à jour. Si le rejeu vient vert, retirer de la note du coffre la
   mention « rejeu de l'orchestrateur en attente ».
3. L7-T32 (contrôle du lot, avec les reports consignés), L7-T33 (merge de `main` @ a50d9e6 ou plus récent, sans
   rebase), L7-T34.
4. Clôture du lot : banc de non-régression, captures à l'écran (Jérôme prévenu avant), dont les contrôles Windows
   listés dans l'état (« Contrôles réservés à Windows et à l'écran »), pièges au coffre.
5. Prévenir A « lot 7 prêt à fusionner », avec le sha et le piège AUTOMOC (supprimer les trois `timestamp`
   d'AUTOMOC avant une suite du worktree du chantier).

## 🔧 Git

- `feat/livraison-complete-b` @ 36d7698 (worktree `.worktrees/livraison-complete-b`), non poussée.
- `feat/livraison-complete-b--L7-T28` @ 9934ec8, arbre propre, non poussée.
- `main` locale @ 70c8601 ; `origin/main` @ a50d9e6 (PR jejeasterix/actcraft#7 mergée).

## 🔑 Accès

Aucun secret dans ce chantier. SSH vers le Mac mini géré par les scripts de `~/.claude/scripts/mac/`.

## 🧩 Pièges

Section « Pièges » de l'état et consignes `consignes-T.md` (en-tête Q_OBJECT créé après un premier build,
`main.item()` qui rend le premier élément du nom, tests instables sous charge). Échap et raccourci d'application :
note du coffre ci-dessus.

## ♻️ Reliquats

Aucun hors chantier.

## 🛠️ Skills utiles

`nouvelle-feature` (étage 3, `references/etage-3-execution.md` ; étage 4 à la clôture). `finalize-session-chaine`
n'est pas pour B : A fusionne.
