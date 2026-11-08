# Consignes communes aux tâches de la phase 2 du lot 7 (chantier ActCraft livraison-complete-b)

Rapport final en français.

## Dossier
Ton worktree <wt> est donné dans ton prompt. Toute commande le vise : `git -C <wt>`, et `Set-Location <wt>;` en tête de chaque commande PowerShell (ton dossier courant revient ailleurs entre deux commandes). Jamais d'écriture hors de <wt>.

## À lire
- `<wt>\docs\plans\2026-10-18-livraison-complete-lot7-phase2-amendement.md` : l'en-tête, les décisions, les « Règles communes à toutes les tâches », la section « Vérifié contre le socle réel », ta ligne de la table, et SEULEMENT ta section `### Tâche L7-<id> — …`. Quand ta section renvoie aux plans 7a/7b (`docs/plans/2026-10-18-livraison-complete-lot7-PLAN.md`, `…-lot7b-PLAN.md`), lis la section citée ; l'amendement prime en cas d'écart.
- `<wt>\CLAUDE.md`, `<wt>\.claude\chantier.md`.

## Méthode
TDD : test rouge d'abord (montré), puis le code, puis le vert. Pour chaque test qui doit discriminer, une construction mutante au moins, remise par `git checkout --` (jamais `Copy-Item` : la date ne change pas et ninja ne recompile pas). Reste zéro dans le périmètre ; hors périmètre consigné dans le rapport, jamais corrigé. Fichiers : seulement ceux de ta ligne de table ; si un autre fichier est indispensable, arrête-toi et dis pourquoi.

## Constructions, suites, captures, banc : par le verrou de la session B (une seule place pour B)
- Écris un script, ex. `<wt>\.tmp\run.ps1` : `Set-Location <wt>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1` (construit puis teste).
- Lance depuis pwsh : `pwsh -NoProfile -File C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b\.tmp\outils-b\b-jeton.ps1 -Proprietaire livraison-complete-b/L7-<id> -Commande "pwsh -NoProfile -File '<wt>\.tmp\run.ps1'"`. N'appelle jamais `jeton.ps1` directement. L'attente peut être longue (RAM basse, autres tâches) : ne la contourne jamais ; si elle expire (code 3), relance.
- **Ne tube jamais la sortie du verrou ni du jeton dans grep ni Select-String.** Lis ensuite : `Select-String -Path <wt>/.tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2`.
- Le premier build d'un worktree neuf est froid (environ 20 min).
- Disque : un worktree construit prend 3 à 7 Go. Le disque a été plein le 2026-10-21 (LNK1116 code 112, « Espace insuffisant »). Une construction qui échoue ainsi ne prouve rien : relance-la. Ne garde qu'un seul dossier de construction (debug) sauf si ta section demande le release.
- Piège vu en L7-T14c : `main.item("<nom>")` rend le PREMIER élément de ce nom dans tout l'arbre. Plusieurs panneaux du lot portent les mêmes noms (`floatingPanelClose`, `floatingPanelTitleBar`…). Cherche toujours un élément dans ta propre racine (`presentationharness::findItem(<ta racine>, "<nom>")`).
- Piège vu en L7-T3 : un en-tête `Q_OBJECT`/`QML_ELEMENT` créé APRÈS un premier build n'est pas vu par l'étape moc (`… is not a type`). Remède : supprimer `build/debug/ui/actcraft_ui_autogen/timestamp` (ou l'`_autogen` de la cible concernée) avant de reconstruire.
- `scripts/test.ps1` contrôle aussi le format : avant chaque suite, `pwsh -NoProfile -File ./scripts/format.ps1`.
- 0 avertissement (`/W4 /WX`). Motifs d'échec : `TESTS EN ECHEC`, `N tests failed`. Attendu : `TESTS OK` et le N de ta ligne de table (N_F = 129 à l'entrée de la phase 2).
- Lire un échec Qt : exécutable seul après `. ./scripts/dev-env.ps1`, `QT_QPA_PLATFORM=offscreen`, `QT_QUICK_BACKEND=software`, `-o fichier,txt` (ctest ne garde pas la sortie). Relève les `Totals` ainsi.
- Tests instables sous charge, connus : `tst_captures_aller_retour_natif`, `tst_ui_files`, `tst_pageview`, `tst_bench_verdict`, `tst_captures_page_allongee_zones` (release, lot 4), `tst_bench_result` (il dépend de `tst_bench_smoke` : relance les deux par `-R '^tst_bench_result$'`). S'ils sont tes SEULS échecs, relance-les seuls (`ctest --preset debug -R '^<nom>$'`) et donne les deux résultats ; ne les corrige pas, ne les désactive pas.
- Preuve visuelle et banc : commandes et formes bornées de `.claude/chantier.md`, hors écran, par le verrou B. Le jeton `ecran` seulement si ta section l'exige, annoncé dans ton rapport.

## Règles
- Pas de type ni de nom du socle recréé ; noms figés : `ACTCRAFT_UI_LOT7_QML`, `ACTCRAFT_UI_LOT7_SOURCES` ; `ui/qml/Presentation.qml` a une racine `QtObject` : tes racines se posent par `parent: presentation.layer` ou `parent: presentation.pageOverlays` ; tout QML neuf protège ses accès à `commands` (null au démontage) ; aucun avertissement QML (`tst_ui_shell::shellLoadsWithoutQmlWarning`).
- Toute surcouche ou tout panneau qui prend des entrées s'enregistre par `addInputCover`, avec un test au stylet (`QTabletEvent`), au doigt (`QTest::createTouchDevice`/`touchEvent`) et à la souris.
- Contrôle des noms APRÈS `git add` : `python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" <wt>` → `NOMS OK`.
- `pwsh -NoProfile` seulement ; arrêt de processus par PID ; API publique de Qt ; rien de Promethean (ni icône, ni libellé, ni algorithme copié) ; aucun titre de vidéo ; ne jamais écrire dans `C:\Users\JeromeDenis\REA - Activinspire\samples\` ni dans `%APPDATA%\Promethean`.
- Commits en français, chemins explicites (jamais `git add -A` ni `git add .`), message terminé par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Commite à chaque étape saine (un arrêt de la machine est possible).
- Aucun test retiré, aucun `QSKIP` hors de ceux que ta section prévoit.

## Rapport (15 lignes au plus, avant ta limite de tours, même partiel)
Verdict, commit, preuve en 3 lignes (N, `Totals` de chaque test touché, `NOMS OK`), rouge vu, mutants, écarts au plan, hors périmètre.
