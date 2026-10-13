# Profil de chantier — ActCraft

> Lu par le skill `nouvelle-feature` (`~/.claude/skills/`) et par le hook `interdits.js`. Le skill porte la
> méthode ; ce fichier porte ce qui est propre au projet. Il fait autorité sur les deux.
> Format : `~/.claude/skills/nouvelle-feature/references/profil-chantier.md`. Écrit le 2026-10-13.

## Dépôt et chemins

- Branche principale : `main` (remote `origin` = github.com/jejeasterix/actcraft, public)
- Worktrees : `.worktrees/<sujet>` (chantier), `.worktrees/<sujet>--T<n>` (tâche isolée)
- Fichier d'état : `docs/sessions/<sujet>-etat.md`
- Cadrage : `docs/specs/<date>-<sujet>-cadrage.md`
- Plans : `docs/plans/<date>-<sujet>-PLAN.md`
- Fichiers locaux à copier dans un worktree neuf : aucun (chaque worktree a son propre `build/`)
- Installation : aucune par worktree ; Qt, Ninja, MSVC et clang-format sont au niveau du poste (`CLAUDE.md`)
- Parallélisme : 2 tâches à la fois au plus (une compilation C++ occupe tous les cœurs)

## Build et tests

Toujours `pwsh -NoProfile` (jamais `powershell.exe` 5.1).

- Build : `pwsh -NoProfile -File ./scripts/build.ps1` — 0 avertissement attendu (`/W4 /WX`)
- Tests rapides (référence de départ, preuve des tâches) : `pwsh -NoProfile -File ./scripts/test.ps1`
- Tests complets (clôture) : `pwsh -NoProfile -File ./scripts/test.ps1 -Preset release` puis
  `pwsh -NoProfile -File ./scripts/test.ps1`
- Forme bornée :
  `pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2`
- Motif d'échec : `TESTS EN ECHEC` (dernière ligne de `scripts/test.ps1`) ; ctest imprime aussi
  `N tests failed`.
- Pièges de preuve :
  - ctest sort en erreur si aucun test n'est trouvé (`noTestsAction: error` dans les presets) : un « 0 test »
    ne passe jamais pour un succès.
  - Un nouveau test doit être déclaré par `actcraft_add_test` dans `tests/CMakeLists.txt`, sinon ctest l'ignore.
  - Les mesures de fluidité se font en preset `release` seulement ; un chiffre mesuré en `debug` ne prouve rien.
- Tests obligatoires : tout code de `core/`, `formats/`, `actions/` modifié a ses tests. Rendu et interface :
  test de chargement + passage au banc.

## Preuve visuelle

Aucune commande automatique pour l'instant. Lancer `build/debug/actcraft.exe --rendu d3d11` puis
`--rendu logiciel` ; les captures et ce qu'il faut regarder vont à la porte de fin. Les fenêtres doivent
s'ouvrir sur l'écran principal (1920 × 1080, 4 moniteurs).

## Banc de fluidité

À partir de l'étape 1 du BRIEF : toute tâche qui touche `render/`, `input/`, `tools/` ou `ui/` passe au banc
(preset `release`) ; un dépassement du budget du BRIEF § 3 (60 images/s, latence d'encre < 20 ms) refuse la
tâche.

- Commande : `pwsh -NoProfile -File ./scripts/bench.ps1` (release ; D3D11 puis logiciel ; plein écran sur
  l'écran principal ; JSON dans `bench/results/`). Dernière ligne : `BUDGET TENU` (0), `BUDGET DEPASSE` (1),
  `MESURE INVALIDE` (2) ; code de sortie = verdict du rendu par défaut (d3d11). Options : `-Rendu
  d3d11|logiciel|tous`, `-SansCache`, `-Captures <dossier>`, `-Scenario complet|court`, `-Taille
  plein-ecran|LxH`, `-HorsEcran`, `-SansConstruire`.
- Forme bornée : `pwsh -NoProfile -File ./scripts/bench.ps1 > .tmp/bench.txt 2>&1 ; Select-String -Path
  .tmp/bench.txt -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE' | Select-Object -Last 1`
- Preuve de tâche sans écran (aucune fenêtre visible) : ajouter `-Rendu logiciel -Scenario court -Taille
  1280x800 -HorsEcran`. Un passage en plein écran se joue sous le jeton `ecran`, annoncé d'une ligne à Jérôme.
- macOS : `scripts/bench.sh` (`--rendu auto|logiciel|tous`, `--sans-cache`, `--captures`, `--hors-ecran`).
- Paquet pour le Dell 3510 : `pwsh -NoProfile -File ./scripts/paquet-banc.ps1` (zip local dans
  `build/paquet-banc/`, jamais publié). Procédure : `docs/banc.md`.

**Pendant le chantier `prototype-mesure`** (décision 38 du cadrage
`docs/specs/2026-10-13-prototype-mesure-cadrage.md`, hypothèse N1-10) : le verdict du banc est un
**résultat, pas une porte**. Les tâches antérieures au banc sont validées par leurs tests seuls ; les tâches postérieures
passent au banc et joignent le JSON, sans refus sur dépassement. Un `BUDGET DEPASSE` produit un rapport chiffré
(critère, valeur, seuil, scénario) et une hypothèse N1 pour Jérôme, sans bloquer le merge. La règle de refus
ci-dessus s'applique après la décision go / no-go de l'étape 2 du BRIEF.

## Sources du cadrage (après le coffre)

1. `docs/BRIEF.md` (document maître) et `CLAUDE.md`
2. `docs/specs/` (cadrages)
3. Graphe du code : `graphify query "…"`, `graphify affected "…"`, `graphify path A B` (dans ce dépôt)
4. Référence de comportement : l'ancien dépôt `C:\Users\JeromeDenis\REA - Activinspire` — son graphe
   (`graphify query` lancé dans ce dossier), `docs/spec/fonctionnalites.md`, `docs/tests/plan-comparatif.md`,
   `docs/re/`, et l'application Python (`cd app && python -m actcraft`)
5. Documentation d'ActivInspire : `C:\Users\JeromeDenis\REA - Activinspire\Documentation Activinspire\README.md`
- Design : pas de ressemblance au pixel imposée ; à défaut de design existant, le skill `impeccable`.
- Rien de Promethean n'entre dans le dépôt (public) : on comprend, puis on écrit nos propres données.

## Interdits

Lus par `~/.claude/hooks/interdits.js`, qui refuse la commande. Portée : `toujours`, `worktree`, `chaine`.

```interdits
toujours :: \bgit\s+push\b(?=.*\s--force(\s|$)) :: push --force (utiliser --force-with-lease)
toujours :: \bgit\s+push\s+\S+\s+(HEAD:)?(refs/heads/)?main\b :: push direct sur main (passer par une PR)
toujours :: \bgit\s+(add\s+(-A|--all|\.)(\s|$)|commit\b[^|;&\n]*(?<![-\w])-[A-Za-z]*a[A-Za-z]*(?=\s|$)|commit\b[^|;&\n]*\s--all\b) :: indexation globale (nommer les fichiers, relire git diff --cached)
toujours :: (Remove-Item|\brm\b|\bdel\b|Move-Item|\bmv\b|Set-Content|Out-File|Clear-Content|Rename-Item)[^|;&\n]*REA - Activinspire[\\/]+samples :: écriture dans les paperboards réels de Jérôme (samples/)
toujours :: (Remove-Item|\brm\b|\bdel\b|Move-Item|\bmv\b|Set-Content|Out-File|Clear-Content|Rename-Item|Copy-Item\s+\S+\s+)[^|;&\n]*(APPDATA[%)]?[\\/]+Promethean|Roaming[\\/]+Promethean) :: écriture dans le profil Promethean
toujours :: (Copy-Item|\bcp\b|\bcopy\b|robocopy|xcopy)[^|;&\n]*(Activ Software|docs[\\/]+re[\\/]|Documentation Activinspire) :: copie de ressources Promethean ou extraites des binaires (le dépôt est public)
toujours :: (taskkill|Stop-Process)[^|;&\n]*\b(Inspire|actcraft)\b(?![^|;&\n]*-Id) :: arrêt d'application sans PID (fermer par PID)
```

## Action finale irréversible

```action-finale
\bgit\s+tag\s+v\d :: tag de release
\bgit\s+push\s+\S+\s+(refs/tags/)?v\d :: publication du tag de release
\bgh\s+release\s+create :: release GitHub publique
```

- Go requis : oui — une release est publique et téléchargeable ; jamais sans accord explicite de Jérôme.
- Due seulement quand Jérôme la demande (pas de distribution avant la décision go / no-go de l'étape 2).
- Porte avant merge : non — rien ne part en production au merge.
- CI couverte par la suite complète : non — la CI compile et teste aussi sous macOS, que la suite locale ne
  couvre pas. Le merge attend la CI verte.

## CLI

Aucune.

## Règles propres au projet

- Fluidité d'abord : une tâche de rendu ou d'entrée sans mesure du banc n'est pas finie.
- `core/` sans interface graphique (CMake le vérifie). Un module ne dépend que de ceux déclarés avant lui dans le
  `CMakeLists.txt` racine.
- Graphe : `graphify update .` + commit de `graphify-out/` avant le merge.
- Pièges de Qt, CMake, MSVC ou pilotes résolus et compris : note `04_Pieges/` du coffre + `06_MOC/actcraft.md`.
- Git : commits en français ; merge par PR (`gh pr merge --merge`).
- Ne jamais tuer un `actcraft.exe` ou `Inspire.exe` lancé par Jérôme : fermer par PID ce qu'on a lancé.
