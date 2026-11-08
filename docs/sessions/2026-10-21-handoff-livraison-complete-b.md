# Handoff — Reprise sur livraison-complete-b (lot 7, phase 2) en NOUVELLE session

## Mode de travail

Premier geste : **« reprends le chantier livraison-complete-b »** (skill `nouvelle-feature`, entrée « reprise »). La pause du quota est levée. Le fichier d'état fait autorité : `docs/sessions/livraison-complete-b-etat.md` (lignes de suivi en tête, section « Point de reprise (pause du quota, 2026-10-21) »).

- **Rôle.** Session B du chantier ActCraft `livraison-complete` : elle exécute le lot 7 « Présentation et outils » en parallèle de la session A (« Livraison complète »). Orchestrateur de la chaîne `nouvelle-feature`, étage 3, mode autonome : tu ne codes pas, tu lances des sous-agents (`implementeur-tdd`, `concepteur-critique`, `reviewer-adversarial`), tu rejoues leurs preuves, tu merges `--no-ff` dans `feat/livraison-complete-b`, tu es le seul rédacteur du fichier d'état.
- **Sources.** Brief : `docs/sessions/livraison-complete-b-brief.md`. Plan : `docs/plans/2026-10-18-livraison-complete-lot7-phase2-amendement.md` @ 141411d (table l. 282+, vagues l. 328-345, une section `### Tâche L7-…` par tâche ; plans 7a/7b cités, l'amendement prime). Profil : `.claude/chantier.md` (build, tests, preuve visuelle, banc, interdits).
- **Prudence (demande de Jérôme, « mais soit prudent »).** Au plus 3 sous-agents à la fois, une seule construction à la fois.
- **Une seule place de tests pour B.** Toute construction, suite, capture ou banc passe par le verrou B : `pwsh -NoProfile -File C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b\.tmp\outils-b\b-jeton.ps1 -Proprietaire livraison-complete-b/<id> -Commande "pwsh -NoProfile -File '<script>'"`. Il prend le mutex `Global\actcraft-session-b-tests`, attend une RAM libre ≥ 1,5 Go quand une autre place est tenue (consigne du Pilote), puis le jeton machine `tests`. A garde 2 places au plus et reste prioritaire. **Jamais de tube de la sortie du jeton vers grep ou Select-String.**
- **Outils** (`.tmp/outils-b/`, non versionné, même poste) :
  - `consignes-T.md` : consignes communes à donner à chaque agent de tâche (chemin à citer dans son prompt) ;
  - `rejeu.ps1 -Tache L7-<id> -Tests tst_ui_lot7:<classe>,<autre_exe>` : rejoue la preuve d'une tâche dans son worktree, sous le verrou ; lire `<wt>\.tmp\rejeu.txt` et `.tmp\rejeu-<exe>-<classe>.txt` (Totals) ;
  - `suite-chantier.ps1` : suite complète du worktree du chantier → `.tmp\suite-chantier.txt`. **Avant chaque suite**, supprimer `build/debug/ui/actcraft_ui_autogen/timestamp`, `build/debug/ui/actcraft_uiplugin_autogen/timestamp`, `build/debug/tests/tst_ui_lot7_autogen/timestamp` (piège AUTOMOC) ;
  - `revue-l7-<id>.md` : rapports des revues ; `bancs/<tâche>/` : rejeux et JSON du banc des tâches mergées.
- **Après chaque merge** : `graphify update .` puis commit de `graphify-out/` ; mise à jour de l'état (table, ligne `Lot :`, retrait de la ligne `Tâche :`) ; copie de `.tmp/rejeu*.txt` et `bench/results/*.json` du worktree dans `.tmp/outils-b/bancs/<tâche>/` ; puis `git worktree remove --force` du worktree de la tâche (règle depuis le disque plein du 21/10).
- **Revue `reviewer-adversarial` obligatoire** avant le merge de toute tâche `concepteur-critique`. Rapport dans `.tmp/outils-b/revue-l7-<id>.md`. Si le verdict est « corriger », essai 2. Les agents s'arrêtent souvent à 40 tours : les relancer par SendMessage (commit wip, une seule attente longue par construction, sans moniteur).
- **Preuves.** Un vert rapporté n'est jamais cru : rouge vu, un mutant par test qui discrimine (remis par `git checkout --`, jamais `Copy-Item`), test tué lu dans la sortie de l'exécutable seul (`-o fichier,txt`), rejeu par l'orchestrateur. Reste zéro dans le périmètre. Banc : « résultat, pas porte » pendant le chantier, porte de non-régression en fin de lot.
- **Tests instables sous charge** (relancer seuls s'ils sont les seuls rouges) : `tst_captures_aller_retour_natif`, `tst_ui_files`, `tst_pageview`, `tst_bench_verdict`, `tst_bench_result`. Trois tests toujours ignorés : `tst_formats_oracle`, `tst_formats_oracle_synth`, `tst_flipchart_miniatures_samples`.
- **Interdits.**
  - Ni push, ni PR, ni tag, ni release : A fusionne la branche B. Pas de rebase de `feat/livraison-complete-b` (refusé au Pilote : branche à merges, A la fusionne) ; le lot 3 arrive par un merge en L7-T33.
  - Rien de Promethean (ni icône, ni image, ni texte d'aide, ni algorithme copié) ; aucun nom de paperboard réel ; aucun titre de vidéo (V01… seulement) ; ne jamais écrire dans `C:\Users\JeromeDenis\REA - Activinspire\samples\` ni dans `%APPDATA%\Promethean`.
  - API publique de Qt seulement ; `core/` sans interface graphique ; `/W4 /WX`.
  - `pwsh -NoProfile` seulement ; arrêt de processus par PID, jamais par nom ; attentes bornées.
  - Commits en français, chemins explicites (jamais `git add -A`), terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- **Communication.** A = session « Livraison complète », Pilote = session « Pilote » ; leurs identifiants sont dans `~/pilotage/etat.json` (ne pas les recopier). Tenir aussi la liste `update_plan` à jour : le panneau du Pilote la lit.

## État au 2026-10-26 vers 11 h (arrêt propre pour bascule)

La pause du quota est levée (« reprend actcraft », le 26/10). Jérôme a ensuite demandé la bascule, relayée mot pour mot par le Pilote : « Oui : arrêt propre des deux sessions, handoff à jour, deux nouvelles sessions, puis archivage des anciennes. » Reprendre sans attendre d'autre message.

Lot 7 : 19/33 tâches mergées (phase 1, L7-S8, L7-T1 à L7-T18, L7-T14c). Branche du chantier verte : suite V10, 158/158, à 5cf0a0e.

## ✅ Fait

Phase 2 du lot 7, de L7-T1 à L7-T18 (table de l'état, `.tmp/outils-b/`). Le 26/10 : revues adversariales de L7-T24 et L7-T21 (toutes deux « corriger »), essais 2 commencés puis arrêtés proprement. Pièges : section « Pièges trouvés » de l'état.

## ⏭️ Prochain

Suivre les gestes 1 à 4 du « Point de reprise (bascule vers une nouvelle session, 2026-10-26 vers 11 h) » de l'état. Résumé :

1. **L7-T19** (2f4eea6, verte selon l'agent) : rejeu `-Tests tst_ui_lot7:ticker`, merge. Ferme le lot P2-A.
2. **L7-T21** essai 2 (5271d4c, rouge M1 vu) : correction dans `onChanges` (origine `Undo`/`Redo` → `resetSession()`), mutant, suite, `NOMS OK`, commit final ; rejeu `-Tests tst_ui_lot7:recognition`, merge.
3. **L7-T24** essai 2 (c2fcecb, six tests rouges écrits, non construits) : rouge à voir, corrections F1, F2 et mineurs décrites dans l'état, mutants, suite, `NOMS OK` ; rejeu `-Tests tst_core_equation`, merge.
4. Suite V11, puis L7-T23 ‖ L7-T25 ; puis T26‖T30, T27‖T31, T28‖T29 (T28 et T30 jamais ensemble : jeton `ecran`), T32, T33 (merge du lot 3), T34. Clôture du lot : sections « À vérifier à la clôture du lot » et « À faire en L7-T34 » de l'état. Enfin, prévenir A « lot 7 prêt à fusionner », avec le sha et le piège AUTOMOC.

Un essai 2 se confie à un nouveau `concepteur-critique` : citer dans son prompt le worktree, les consignes `.tmp/outils-b/consignes-T.md`, le rapport de revue, et le geste exact de l'état.

Fini quand : 33/33 tâches mergées, suite verte, clôture du lot faite, A prévenu.

## Travail en vol

Aucun. Agents arrêtés, worktrees propres, rejeu de L7-T19 arrêté avant de démarrer. B ne tient ni le verrou B ni le jeton `tests` (au moment de l'arrêt, une place était marquée ABANDONNÉ au nom de `livraison-complete/L4-T31`, c'est-à-dire de A, pas de B). RAM libre souvent sous 1,5 Go : le verrou B attend, c'est normal. Question en attente de Jérôme : aucune.
## 🔧 Git

- `feat/livraison-complete-b` dans `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b`, locale, jamais poussée (A la fusionne).
- Branches de tâche avec worktree : `feat/livraison-complete-b--L7-T19`, `--L7-T21`, `--L7-T24`. Les autres worktrees sont retirés ; leurs branches sont gardées.
- Lot 3 livré dans main (70c8601) : il arrive dans B par le merge prévu en L7-T33.

## 🛠️ Skills utiles

`nouvelle-feature` (références `etage-3-execution.md` pour la reprise et la fin de lot, `etage-4-cloture.md` pour la clôture, `fichier-etat.md` pour les lignes de suivi).
