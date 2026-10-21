# Handoff — Reprise sur livraison-complete (A, lot 4) en NOUVELLE session

## MODE DE TRAVAIL (source : `.claude/chantier.md` de la branche du chantier, qui fait autorité)

- **Premier geste, à la relance de Jérôme seulement : « reprends le chantier livraison-complete »**, dans une
  session ouverte sur `C:\Users\JeromeDenis\ActCraft`, branche `main` (skill `nouvelle-feature`, entrée
  « reprise »). Le chantier est **en pause** sur demande de Jérôme (quota de la semaine) : rien ne se lance avant
  son message de reprise. Le fichier d'état fait autorité :
  `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete\docs\sessions\livraison-complete-etat.md`
  (entrée « PAUSE demandée par Jérôme » : point de reprise).
- **Autonomie totale jusqu'à la livraison finale.** Décisions du coffre 0100, 0121, 0122 (Hunspell en MPL 1.1),
  0125 (aucune mesure sur le Dell, la tablette réelle ni le MacBook). Les choix de conception qu'une référence
  permet de trancher se décident seul, d'après l'original, puis se notent dans l'état (Jérôme : « tu dois prendre
  ce genre de décisions en autonomie. Tu as des reference pour t'aider (activinspire) »).
- **Orchestrateur** : ne code pas (sauf une retouche d'une ligne), délègue ; relit puis **rejoue chaque preuve**
  (`.tmp/preuve-<id>.ps1`) — un vert rapporté n'est jamais cru ; merge `--no-ff` dans `feat/livraison-complete` ;
  seul rédacteur de l'état. Une tâche = un worktree `.worktrees/livraison-complete--L4-<id>`.
- **Jeton** : toujours `-Places 1` pour A (consigne du Pilote : une seule suite à la fois, ≥ 1,5 Go libres) :
  `pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Places 1 -Proprietaire livraison-complete/<id> -AttenteSecondes 7200 -Commande "pwsh -NoProfile -File '<preuve>'"`, en tâche de fond.
- **Consignes communes des agents du lot 4**, copiées hors du dépôt (non versionné) :
  `C:\Users\JeromeDenis\ActCraft\.git\chantiers\livraison-complete-A\` — `brief-lot4.md` (graphe, commits par
  jalon, Clang, mémoire, forme de la preuve, pièges des preuves), `modele-preuve-l4.ps1`, `regles-lot3.md`,
  `clang-tests.ps1`, `titres.txt`, et `out/` (sorties des rejeux). Les briefs d'agents doivent citer ce dossier.
- **`concepteur-critique`** : liste fermée des fichiers à lire, « Lecture k/15 », tests rouges commités au plus
  tard à la 15e lecture. Un agent arrêté à 40 tours : commit `wip:` du filet, rapport dans
  `%LOCALAPPDATA%\claude-filet\<id>.md`.
- **Avant chaque merge** : `NOMS OK` (`python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" <wt>`),
  `git grep -l -i -F -f <dossier ci-dessus>\titres.txt` vide, rien de suivi sous `.tmp/`.
- **Interdits** : rien de Promethean dans le dépôt public (ni icône, image, capture, code, texte de l'aide) ; aucun
  titre de vidéo ; aucun nom de paperboard réel ni de l'original ; ne jamais modifier
  `C:\Users\JeromeDenis\REA - Activinspire\samples\` ni `%APPDATA%\Promethean` (séance sur l'original : copie du
  profil avant, `robocopy /MIR` après, empreinte vérifiée) ; jamais `cd` (utiliser `git -C`) ; arrêter un
  processus seulement par son PID et s'il est prouvé à soi ; une séance à l'écran s'annonce d'une ligne à Jérôme.
- **Releases** : seulement sur go écrit de Jérôme. Commits en français, chemins explicites, ligne
  `Co-Authored-By` de l'environnement.

**État au 2026-10-21, soir** : lot 4 « Outils II » à 22 tâches sur 38, en pause. Mergées en fin de session :
L4-C2 (2e71fd6), L4-T8 (00ff92e), L4-C3 (49b08c6), L4-T24 (a72bd3d). R2 de 4a interrompue.

## ✅ Fait cette session
- Lot 4 : L4-T0 à L4-T6, L4-T8 à L4-T10, L4-T15 à L4-T18, L4-T21, L4-T22, L4-O1, L4-O2, L4-P1, L4-P2, L4-C2,
  L4-C3, L4-T24 mergées (détail et preuves : entrées de l'état). Amendement 5 du plan (38 tâches).
- Décisions d'après l'original (L4-O2) : Maj sans effet sur le tracé (D5) ; grille du dessus sur tout, trait en
  cours compris (H4-22) ; séparations de cadres (D11) ; formes et texte en fin de palette (D19, choix de Jérôme).

## ⏭️ Prochain (à la relance de Jérôme)
1. **R2 de 4a**, trois worktrees avec tests rouges commités, agents interrompus par la fin de session :
   - L4-T7 (`--L4-T7`, 87bff43, `concepteur-critique`, arrêté à 40 tours) ;
   - L4-T11 (`--L4-T11`, 0ada19a, `implementeur-tdd`) ;
   - L4-T31 (`--L4-T31`, 074d5c6, `implementeur-tdd`).
   Regarder `git -C <wt> log` et `status` (du travail a pu être commité après ces sha), puis relancer un agent du
   même type par tâche, repartant de ses commits, avec le brief commun ; relire et rejouer chaque preuve.
2. Puis, dans l'ordre : R3 (L4-T12, L4-T13, L4-T14), point d'étape de 4a, L4-C4 (débloquée), L4-T19, L4-T26
   (après L4-T13), V10 à V12 de 4b, correctif **L4-C5** (H4-24 : Ctrl+clic ajoute à la sélection comme
   l'original, au lieu de Maj+clic ; `tools/src/selecttool.cpp`).
3. Avant la PR du lot : `clang-tests.ps1` (adapter `$wt`) sur `tests/` et `bench/`, construction Android
   (`cmake --build --preset android-arm64 -- -k 0`), graphe (`git clean -fq -- graphify-out/cache/ast`, puis
   `GRAPHIFY_VIZ_NODE_LIMIT=8000 graphify update .`). Puis clôture du lot (`finalize-session-chaine`).
- Fini quand (lot 4) : 38 tâches mergées, relecture du lot, PR mergée, CI verte, porte de fin passée.

## 🔧 Git
- `main` = 70c8601 (PR #6), à jour sur `origin`. `feat/livraison-complete` poussée (d34e4ef puis ce handoff).
- Worktrees vivants de A : `livraison-complete`, `--L4-T7`, `--L4-T11`, `--L4-T31`. Ceux de B
  (`livraison-complete-b`, lot 7) ne sont pas à A : n'y toucher jamais.
- Fichier non suivi dans le dossier principal, pas à A : `docs/sessions/prototype-mesure-etat.md`.

## 🔑 Accès
Aucune clé dans le dépôt. Signature de la release 0.4.0 : point de porte du lot 3, voir l'état.

## 🧩 Pièges
Pièges des scripts de preuve : `brief-lot4.md` (`-match` sur un tableau, casse de `FAIL!`, `NOMS OK` suivi du
compte, mutation qui doit compiler sous `/W4 /WX`). Un agent qui lance un banc à part sous le jeton bloque le
retrait de son worktree : le brief dit « pas de banc lancé à part ». Le jeton n'est pas une file : une tâche peut
attendre longtemps (L4-T24 : 2 h 30). Coffre : `C:\SecondBrain\06_MOC\actcraft.md`.

## ♻️ Reliquats hors chantier
Aucun.

## 🛠️ Skills utiles
`nouvelle-feature` (reprise, étage 3), `finalize-session-chaine` (clôture du lot 4), `transfert-session`.
