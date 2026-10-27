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
- Preuve Windows sans poste Windows (session sur le Mac, ou toute tâche dont la preuve exige MSVC) : job à la demande
  `.github/workflows/preuves-windows.yml` sur `windows-latest` (MSVC `/W4 /WX` contrôlé dans `build.ninja`, preset
  `release` par défaut, `ctest` ; ni banc ni rendu sur vrai GPU : ceux-là restent au PC). Piège : `workflow_dispatch`
  prend le fichier dans la ref visée, la branche doit donc contenir `main` fusionné (après le lot B11 du chantier
  mac-autonome) et être poussée sur `origin`. `tests` vide (option omise) = toute la suite ; sinon expression de
  `ctest -R` (ex. `-f tests=tst_bureau`) ; un motif qui ne trouve aucun test échoue (`noTestsAction: error`).
  Le run dure 20 à 40 min : lancer la forme bornée en tâche de fond (`run_in_background: true` de l'outil Bash), jamais
  au premier plan (l'appel coupe à 10 min) ; elle lance le run elle-même et rend la main à sa fin. Le journal recopie
  le texte des `run:` : seule une ligne `<horodatage>Z PREUVES WINDOWS …` est un verdict, d'où l'ancre `[0-9]Z`.
  - bash (Mac), forme bornée :
    `mkdir -p .tmp && url=$(gh workflow run preuves-windows.yml --repo jejeasterix/actcraft --ref <branche> -f preset=release 2>&1 | grep -Eo 'https://github.com/[^ ]+/actions/runs/[0-9]+' | tail -1); id=${url##*/}; if [ -z "$id" ]; then echo 'code 2'; echo 'RUN INTROUVABLE : aucune URL rendue par gh workflow run, aucun run deviné'; else echo "run $id"; gh run watch "$id" --repo jejeasterix/actcraft --exit-status --interval 60 > .tmp/preuve-windows.txt 2>&1; echo "code $?"; gh run view "$id" --repo jejeasterix/actcraft --log | grep -E '[0-9]Z PREUVES WINDOWS (OK|EN ECHEC)' | tail -1; fi`
  - pwsh (PC), forme bornée :
    `New-Item -ItemType Directory -Force .tmp | Out-Null; $s = gh workflow run preuves-windows.yml --repo jejeasterix/actcraft --ref <branche> -f preset=release 2>&1 | Out-String; $id = [regex]::Match($s, 'actions/runs/(\d+)').Groups[1].Value; if (-not $id) { 'code 2'; 'RUN INTROUVABLE : aucune URL rendue par gh workflow run, aucun run deviné' } else { "run $id"; gh run watch $id --repo jejeasterix/actcraft --exit-status --interval 60 > .tmp/preuve-windows.txt 2>&1; "code $LASTEXITCODE"; gh run view $id --repo jejeasterix/actcraft --log | Select-String '[0-9]Z PREUVES WINDOWS (OK|EN ECHEC)' | Select-Object -Last 1 }`
  - Réussite : `code 0` et dernière ligne de verdict `PREUVES WINDOWS OK`. Échec : `code` ≠ 0 et
    `PREUVES WINDOWS EN ECHEC : <étape>` (l'étape tombée : `Compiler` pour un avertissement sous `/WX`, `Vérifier /W4 /WX`,
    `Tester`… ; `<étape> (annulé)` ou `run annulé` pour une annulation), détail par `gh run view <id> --log-failed`
    (`<id>` : ligne `run <id>`). `code` ≠ 0 sans ligne de verdict : job coupé avant sa dernière étape (délai de 40 min
    dépassé, ou annulé par un run plus récent sur la même ref, le même preset et le même motif : `cancel-in-progress`) ;
    `gh run view <id> --repo jejeasterix/actcraft --json conclusion` dit `cancelled` ou `failure` ; ce n'est pas un
    verdict sur le code : relancer seul, une preuve à la fois par ref. `code 2` et `RUN INTROUVABLE` : `gh workflow run` n'a rendu
    aucune URL ; ne pas prendre « le dernier run » de `gh run list` (il peut être plus ancien), relancer.

## Preuve visuelle

Commande (décision 36 et DA8 du cadrage `docs/specs/2026-10-14-livraison-complete-cadrage.md`), livrée par la tâche
L1-T15 du chantier `livraison-complete` :
`pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario <nom>|tous [-Rendu d3d11|logiciel] [-HorsEcran] -Sortie <dossier> [-Lire "<objet>.<propriété>=<attendu>"]`.
Elle joue un scénario d'interface nommé (`bench/scenarios/<nom>.json` : commandes, stylet injecté, touches),
contrôle des pixels, lit des propriétés (lignes `LU …`, à lire avant les images) et écrit les captures PNG et un
`index.md`. Sorties sous `.tmp/` ou `build/`, jamais commitées. Hors écran en rendu logiciel pour chaque tâche
d'interface ; à l'écran en D3D11 en fin de lot, sous le jeton `ecran`.

- Forme bornée :
  `New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario premiers-pas -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "pageIndicator.text=1 / 2" > .tmp/captures.txt 2>&1 ; Select-String -Path .tmp/captures.txt -Pattern '^(COMMANDE|TRAIT|TOUCHE|ATTENTE|PRET|FENETRE|LU|PIXEL|CAPTURE) |^SCENARIO |CAPTURES (OK|EN ECHEC)' | Select-Object -Last 16`
- Motifs d'échec : `CAPTURES EN ECHEC`, `SCENARIO EN ECHEC`, `SCENARIO INVALIDE`, toute ligne d'étape en `ECART`
  (`LU`, `PIXEL`) ou en `ECHEC` (`COMMANDE`, `TRAIT`, `TOUCHE`, `PRET`, `CAPTURE`), `FENETRE …`. Le scénario
  s'arrête à la première étape en écart : elle reste dans les 16 dernières lignes, suivie des lectures `-Lire` et
  des verdicts.
- À l'écran (fin de lot, jeton `ecran`) : même commande avec `-Rendu d3d11` et sans `-HorsEcran`.
- Le scénario `premiers-pas` est aussi un test ctest (`tst_captures_premiers_pas`, hors écran).

Les fenêtres doivent s'ouvrir sur l'écran principal (1920 × 1080, 4 moniteurs).

## Banc de fluidité

À partir de l'étape 1 du BRIEF : toute tâche qui touche `render/`, `input/`, `tools/` ou `ui/` passe au banc
(preset `release`) ; un dépassement du budget du BRIEF § 3 (60 images/s, latence d'encre < 20 ms) refuse la
tâche.

- Commande : `pwsh -NoProfile -File ./scripts/bench.ps1` (release ; D3D11 puis logiciel ; plein écran sur
  l'écran principal ; JSON dans `bench/results/`). Dernière ligne : `BUDGET TENU` (0), `BUDGET DEPASSE` (1),
  `MESURE INVALIDE` (2) ; code de sortie = verdict du rendu par défaut (d3d11). Options : `-Rendu
  d3d11|logiciel|tous`, `-SansCache`, `-Captures <dossier>`, `-Scenario complet|court`, `-Taille
  plein-ecran|LxH`, `-HorsEcran`, `-SansConstruire`.
- Forme bornée : `New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/bench.ps1 > .tmp/bench.txt 2>&1 ; Select-String -Path
  .tmp/bench.txt -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE' | Select-Object -Last 1`
- Preuve de tâche sans écran (aucune fenêtre visible) : ajouter `-Rendu logiciel -Scenario court -Taille
  1280x800 -HorsEcran`. Un passage en plein écran se joue sous le jeton `ecran`, annoncé d'une ligne à Jérôme.
- macOS : `scripts/bench.sh` (`--rendu auto|logiciel|tous`, `--sans-cache`, `--captures`, `--hors-ecran`).
- Paquet pour le Dell 3510 : `pwsh -NoProfile -File ./scripts/paquet-banc.ps1` (zip local dans
  `build/paquet-banc/`, jamais publié). Procédure : `docs/banc.md`.

**Pendant le chantier `livraison-complete`** (décision 35 et DA7 du cadrage
`docs/specs/2026-10-14-livraison-complete-cadrage.md`, hypothèse N1-9) : le régime de la décision 38 du prototype
continue — le verdict du banc est un **résultat, pas une porte** — tant que l'étape 2 du BRIEF n'est pas faite.
Chaque tâche qui touche `render/`, `input/`, `tools/` ou `ui/` joint le JSON du banc sans écran (`-Rendu logiciel
-Scenario court -Taille 1280x800 -HorsEcran`). S'y ajoute une **porte de non-régression par lot** : en fin de lot,
banc complet D3D11 en plein écran sous le jeton `ecran` ; tout critère tenu par la référence doit l'être encore,
sinon une tâche de correction précède la clôture. Référence du lot 1 : `docs/mesures/2026-10-13-prototype-mesure.md` ;
la mesure de fin de lot 1, commitée dans `docs/mesures/`, devient la référence des lots suivants ; après le lot 10,
« figements » et « reprises » y entrent. La règle de refus ci-dessus s'applique après la décision go / no-go de
l'étape 2 du BRIEF.

## Paquets (installateur Windows, APK Android) et release

Locaux, jamais publiés hors du go de la release (décisions 26 à 33 du cadrage
`docs/specs/2026-10-14-livraison-complete-cadrage.md`).

- Installateur : `pwsh -NoProfile -File ./scripts/paquet.ps1` → `build/paquet/ActCraft-<version>-setup.exe`
  (`PAQUET OK`) ; essai S1 : `pwsh -NoProfile -File ./scripts/essai-installateur.ps1` (`INSTALLATEUR OK` ; ouvre une
  fenêtre 5 s : l'annoncer).
- APK : `pwsh -NoProfile -File ./scripts/paquet-android.ps1` (chaîne : `scripts/dev-env-android.ps1`) →
  `build/paquet/ActCraft-<version>-arm64-v8a.apk` et `…-x86_64.apk`, clé de débogage locale (`APK OK`) ; essai S3 sur
  un émulateur démarré : `pwsh -NoProfile -File ./scripts/essai-android.ps1 -Apk <…-x86_64.apk>` (`ANDROID OK`).
  Émulateur local ou CI : `docs/verifications/2026-10-14-outillage-paquets.md`.
- Licences : `scripts/verifier-licences.ps1` (appelé par les deux scripts de paquet ; liste
  `packaging/qt-autorises.txt`).
- CI : jobs `paquet-windows` (S1) et `android` (S3, émulateur KVM) à chaque PR, sans téléversement. Plus de CI sur
  push (ni `main` ni `essai-ci/**`, lot B11 du chantier mac-autonome). Essai de CI sans PR :
  `gh workflow run ci.yml --repo jejeasterix/actcraft --ref <branche>` (branche poussée, avec `main` fusionné) ;
  preuve Windows seule : commande « Preuve Windows » (section Build et tests).
- Release : `.github/workflows/release.yml`. Tag `v<version>` (action finale, go écrit) → brouillon avec
  installateur, APK `arm64-v8a` signé par la clé de release, `SHA256SUMS.txt`, notes de `CHANGELOG.md`
  (`scripts/notes-version.ps1`, `NOTES OK`). Sans les secrets de signature, le mode réel échoue avant toute
  construction (`scripts/verifier-secrets-release.ps1`, `SECRETS ABSENTS`). Essai sans tag : `workflow_dispatch`
  `mode=essai` (workflow sur `main`) ou push d'une branche `essai-ci/release-<sujet>` (APK signé par une clé de
  débogage, annoncé `APK NON PUBLIABLE` ; aucune release, aucun téléversement), puis suppression de la branche.
  Une release déjà présente pour le tag (brouillon compris) arrête le workflow (`RELEASE EXISTE DEJA`).
  Publication du brouillon : `gh release edit v<version> --draft=false`, sous le même go.
- **Au go, avant de pousser le tag** (Jérôme ou l'orchestrateur, une seule fois) : créer l'environnement GitHub
  `release` (Settings → Environments), règle de déploiement limitée aux tags `v*` (« Selected branches and tags »,
  motif de tag `v*`), et y placer les quatre secrets `ACTCRAFT_KEYSTORE_BASE64`, `ACTCRAFT_KEYSTORE_PASSWORD`,
  `ACTCRAFT_KEY_ALIAS`, `ACTCRAFT_KEY_PASSWORD` comme secrets d'environnement, jamais comme secrets de dépôt
  (`gh secret set <nom> --env release`). `release.yml` référence cet environnement en mode réel seulement (jobs
  `secrets`, `android`, `release`) : sans lui, le premier tag le créerait sans protection. Le mode essai n'y touche
  pas (nom d'environnement vide).

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
\bgit(\.exe)?\b(\s+-[Cc]\s+("[^"]*"|'[^']*'|\S+))*\s+tag\b(?=.*[\s"']v\d) :: tag de release
\bgit(\.exe)?\b(\s+-[Cc]\s+("[^"]*"|'[^']*'|\S+))*\s+push\b(?=.*(refs/tags/|[\s"'+:]v\d|\s--tags\b|\s--follow-tags\b|\s--mirror\b|\stag\s)) :: publication du tag de release
\bgh(\.exe)?\s+release\s+(create|edit|upload|delete)\b :: écriture d'une release GitHub (création, publication du brouillon, fichiers, suppression)
\bgh(\.exe)?\s+api\b(?=.*(releases|refs/tags))(?!.*(-X|--method)[\s=]*(GET|get)\b)(?=.*(\s-X|\s--method|\s(-f|-F|--field|--raw-field|--input)[\s=])) :: écriture dans les releases ou les tags par l'API
```

- Go requis : oui — une release est publique et téléchargeable ; jamais sans accord explicite de Jérôme.
- Due : proposée à la porte de fin de chaque lot du chantier `livraison-complete` (Jérôme a demandé les releases
  `.exe` et `.apk` le 2026-10-14 ; décisions 32, 33 et 38 du cadrage
  `docs/specs/2026-10-14-livraison-complete-cadrage.md`). Chaque publication reste soumise à son go écrit. Le tag,
  sa poussée et la publication du brouillon (`gh release edit --draft=false` ou appel d'API) relèvent du **même**
  go. Aucune distribution hors de ce go : les paquets de chaque lot sont remis localement (`build/paquet/`), la CI
  les construit et les essaie sans les téléverser.
- Fichier go : `<git-common-dir>/chantiers/go-action-finale` est écrit au go, **reste en place après le tag
  jusqu'à la publication du brouillon** (le brouillon n'existe qu'à la fin du workflow `release.yml`), dans la
  limite de 6 h que lit `interdits.js`, puis est supprimé aussitôt la publication faite. Passé 6 h sans publication, le go est
  redemandé à Jérôme.
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
