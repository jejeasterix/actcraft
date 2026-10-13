# CLAUDE.md — ActCraft (C++ / Qt 6 Quick)

## Langue
Tout texte destiné à Jérôme en français. Code, identifiants et commandes en anglais ; messages de commit en
français.

## Qu'est-ce qu'ActCraft
Logiciel libre de tableau blanc interactif, réécriture en C++20 / Qt 6.12 Quick de l'application Python
d'ActCraft (clone d'ActivInspire Studio 3.5.18 construit par rétro-ingénierie). **Lire `docs/BRIEF.md` avant
toute session** : objectifs, décisions, matériel cible, budget de fluidité, architecture, plan.

Priorité absolue : **la fluidité**. Toute affirmation de performance s'appuie sur une mesure du banc (`bench/`),
jamais sur une intuition.

## Stack et commandes
- C++20, Qt 6.12.0 LTS (`C:\Qt\6.12.0\msvc2022_64`), CMake 4 + Ninja, MSVC 2022 (Build Tools 17.14).
- Tests Qt Test lancés par ctest, hors écran et en rendu logiciel.
- Toujours `pwsh -NoProfile`, jamais `powershell.exe` 5.1.

```powershell
pwsh -NoProfile -File ./scripts/build.ps1 [-Preset debug|release]   # configure + compile
pwsh -NoProfile -File ./scripts/test.ps1  [-Preset debug|release]   # compile + ctest + format ; finit par « TESTS OK »
pwsh -NoProfile -File ./scripts/format.ps1 [-Check]                 # clang-format
build/debug/actcraft.exe --rendu d3d11|logiciel|auto [--quitter-apres <ms>]
```

Dans une session PowerShell interactive : `. ./scripts/dev-env.ps1` charge MSVC, Ninja et Qt.

**Installation sur un nouveau poste Windows** : Visual Studio Build Tools 2022 (C++), CMake, Python 3.12, puis
```powershell
python -m pip install --user ninja clang-format "git+https://github.com/miurahr/aqtinstall"
python -m aqt install-qt windows desktop 6.12.0 win64_msvc2022_64 -m qtshadertools qtimageformats --outputdir C:\Qt
```
aqtinstall 3.3.0 (PyPI) ne sait pas installer Qt 6.12 : la version du dépôt GitHub, oui (vérifié le 2026-10-13).
Autre dossier de Qt : variable `ACTCRAFT_QT_DIR`.

## Architecture et règles
Modules, règles de modularité et de fluidité : `docs/BRIEF.md` § 4. L'essentiel :
- un module = une bibliothèque statique `actcraft_<nom>` déclarée par `actcraft_add_module`
  (`cmake/ActCraftModule.cmake`) ; en-têtes publics sous `<module>/include/actcraft/<module>/` ;
- `core/` sans interface graphique : CMake échoue sinon (`actcraft_forbid_links`) ;
- tout passe par une commande nommée (`actions/`) ; un outil ne connaît que des interfaces ;
- avertissements = erreurs (`/W4 /WX`, `-Werror`) ; `clang-format` vérifié par `scripts/test.ps1` et la CI.
- Tests : tout code de `core/`, `formats/`, `actions/` modifié ou ajouté a ses tests. Ne jamais modifier un
  test pour le faire passer sans comprendre pourquoi il échoue.

## Base de connaissance — l'ancien dépôt et la rétro-ingénierie
Le travail fait avec REA sur ActivInspire et l'application Python sont la **référence de comportement**.
Tout est dans l'ancien dépôt `C:\Users\JeromeDenis\REA - Activinspire` (local, branche `master`, sans remote) :

| Besoin | Où chercher |
|---|---|
| Comment l'app Python fait X ? | **graphe** : `graphify query "…"` lancé **dans l'ancien dépôt** (2 633 nœuds, code + docs Markdown) |
| Format `.flipchart` | `docs/re/format-flipchart.md`, `app/actcraft/io/_schema.py`, `app/actcraft/io/flipchart_reader.py` |
| Commandes, menus, barres | `docs/re/commands.json` (353 commandes), `docs/re/interface.md` |
| Exigences fonctionnelles | `docs/spec/fonctionnalites.md` (197 exigences F-xxx) |
| Ordre et détails des fonctions, cotes relevées | `docs/tests/plan-comparatif.md` (26 lignes) |
| Règles de rendu relevées au pixel | `app/actcraft/canvas/grid.py`, `canvas/tools.py`, `canvas/items.py` |
| Rejouer un scénario à vraies entrées (Windows) | `tools/drive.py`, `tools/cmp.py`, `tools/scenarios.py`, `tools/zoom.py` |
| Lancer la référence | `cd app && python -m actcraft` (tests : `python -E -P -m pytest -q -p no:cacheprovider`) |
| Documentation utilisateur d'ActivInspire | `Documentation Activinspire/README.md` (index par besoin, ~430 fichiers) |
| Paperboards réels (jeux d'essai) | `samples/` — **ne jamais modifier** |
| Interroger le binaire d'origine | skill `reverse-engineer-anything` (REA) sur `C:\Program Files\Activ Software\Inspire\` |

**Dépôt public — propriété intellectuelle (NON négociable)** : aucune ressource de Promethean n'entre ici.
Ni icône, ni image, ni capture, ni code, ni texte de l'aide, ni fichier extrait des binaires
(`commands.json`, `ui-tokens.json`, `docs/re/shots/`…). On s'en sert pour **comprendre** un comportement ;
on écrit nos propres données, nos propres textes, nos propres noms de commandes. Le format `.flipchart` est lu
pour l'interopérabilité, à partir de notre propre documentation.

## Knowledge graph (graphify)
- `graphify-out/` (commité) : carte du code de ce dépôt. Pour une question d'architecture (« qui appelle X ? »,
  « qu'est-ce qu'impacte Y ? »), interroger le graphe **avant** de relire les fichiers :
  `graphify query "…"`, `graphify path A B`, `graphify affected "Nœud"`, `graphify explain "Nœud"`.
- **Tenir le graphe à jour (OBLIGATOIRE)** : après toute modification de code, au plus tard avant le commit de
  fin de chantier, `graphify update .` puis commit de `graphify-out/`. Après un merge : `graphify update .`
  (voir `.gitattributes`). Exclusions : `.graphifyignore`.
- Graphe de l'ancien dépôt : `graphify update .` **dans l'ancien dépôt** s'il a changé.

## Second cerveau (coffre `C:\SecondBrain`)
Mémoire longue commune à tous les projets ; contrat : `C:\SecondBrain\CLAUDE.md`. Carte du domaine :
`06_MOC/actcraft.md`. Fiche projet : `01_Projets/inspire-clone.md`.
- **Chercher avant d'écrire** : `node "C:\SecondBrain\.tools\bin\chercher.js" "le sujet" 5`. Avant de déboguer un
  comportement de Qt, chercher le piège : il est peut-être déjà consigné.
- **Écrire seulement sur trois déclencheurs** : décision tranchée → `02_Decisions/` ; piège résolu et compris →
  `04_Pieges/` (preuve obligatoire : fichier:ligne, commande ou mesure) ; chantier clos → `00_Journal/` + fiche
  projet. Étiquettes : `projet/actcraft`, `tech/qt`, `tech/cpp`, `tech/cmake`, `performance`.
- **Collecter les pièges est un réflexe** : un comportement de Qt, de CMake, de MSVC ou d'un pilote qui a coûté
  plus de quelques minutes et dont la cause est comprise devient une note `04_Pieges/`, ajoutée à
  `06_MOC/actcraft.md`.

## Démarrer une fonctionnalité — chaîne `nouvelle-feature`
Skill **`~/.claude/skills/nouvelle-feature/`**, commun à tous les projets (ne pas le recréer ici). Cinq étages,
une session orchestratrice : lancement → cadrage (`cadreur`) → plan (`planificateur`) → exécution en vagues dans
des worktrees (`implementeur-tdd`, `concepteur-critique`) → relecture (`reviewer-adversarial`) → clôture
(`finalize-session-chaine`). Ce qui est propre à ce dépôt (commandes, preuves, interdits, action finale) est
dans **`.claude/chantier.md`**, qui fait autorité. Lancer depuis le dossier principal, sur `main`.
Ne pas l'appliquer à un correctif ciblé. Hors chaîne : `grill-me` pour un cadrage interactif.

Hooks (niveau utilisateur, `~/.claude/hooks/` et `C:\SecondBrain\.tools\hooks\`) : `interdits.js` refuse les
commandes du bloc `interdits` du profil ; `relire-plan.js` impose la relecture d'un plan écrit dans `docs/plans/` ;
`post-compact-relire-etat.js` rappelle le fichier d'état du chantier ; `graphify-nudge.js` rappelle le graphe ;
les hooks du coffre injectent les cartes, refusent les doublons et valident les notes.

## Mode autonome
Avancer seul, étape par étape, sans demander « je continue ? ». Solliciter Jérôme seulement pour :
1. une installation impossible à faire soi-même ;
2. une décision qui lui revient (liste des fonctions à revoir, licence, publication, dépense) ;
3. piloter la souris de l'écran principal (le prévenir avant) ;
4. mesurer sur une machine de référence (Dell 3510, MacBook M1) : lui donner une procédure exacte.

## Git
- Branche principale `main` ; remote `origin` = github.com/jejeasterix/actcraft (**public**). Merge par PR.
- Commits fréquents, en français, chemins explicites (jamais `git add -A`), terminés par la ligne
  `Co-Authored-By` donnée par l'environnement.
- Fichiers en LF (`.gitattributes`) ; build + tests verts (`scripts/test.ps1` → `TESTS OK`) avant tout commit.

## Pièges connus
- Écran principal 1920 × 1080 sur 4 moniteurs : les fenêtres testées doivent s'ouvrir sur l'écran principal.
- `tools/cmp.py` (ancien dépôt) tue `Inspire.exe` et toute fenêtre « ActCraft » : fermer l'application avant.
- Écrire des fichiers depuis Python sous Windows : `write_bytes`, sinon CRLF.
- L'exécutable est dans `build/<preset>/actcraft.exe` (pas dans `build/<preset>/app/`) :
  `qt_standard_project_setup` regroupe les binaires à la racine du dossier de construction.
- Qt : ne pas activer ensemble `AA_CompressHighFrequencyEvents` et `AA_CompressTabletEvents` (perte des points
  intermédiaires du stylet).
- Ne jamais modifier `C:\Users\JeromeDenis\REA - Activinspire\samples\` ni le profil `%APPDATA%\Promethean`.
