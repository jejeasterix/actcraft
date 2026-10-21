# Handoff — Reprise sur livraison-complete-b (lot 7, phase 2) en NOUVELLE session

## Mode de travail

Premier geste : **« reprends le chantier livraison-complete-b »** (skill `nouvelle-feature`, entrée « reprise »), **seulement après le message de reprise de Jérôme** (pause du quota, voir plus bas). Le fichier d'état fait autorité : `docs/sessions/livraison-complete-b-etat.md` (lignes de suivi en tête, section « Point de reprise (pause du quota, 2026-10-21) »).

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

## État au 2026-10-21 vers 19 h 05

**Pause du quota en cours.** Jérôme, relayé mot pour mot par le Pilote : « afin de ne pas epuisé tout mon quota, organise une pause jusqu'à ce que je relance ». Ne rien lancer avant son message de reprise.

Lot 7 : 19/33 tâches mergées (phase 1, L7-S8, L7-T1 à L7-T18, L7-T14c). Branche du chantier verte : suite V10, 158/158, à 5cf0a0e (état ensuite jusqu'à e5d0d08).

## ✅ Fait cette session

Phase 2 du lot 7, de L7-T1 à L7-T18 (détail dans la table de l'état et dans `.tmp/outils-b/`). Incidents traités et règles qui en sortent : section « Pièges trouvés » de l'état (disque plein, PDB corrompus, AUTOMOC, `main.item`, `characterOffset()`, `Select-String`).

## ⏭️ Prochain

Suivre les gestes 1 à 4 du « Point de reprise (pause du quota, 2026-10-21) » de l'état. Résumé :

1. **L7-T24** (équation I, `concepteur-critique`, worktree `--L7-T24`, c29a5e7) : la revue dit « corriger » (`.tmp/outils-b/revue-l7-t24.md`), avec F1 et F2 bloquants. Essai 2, puis nouveau rejeu `-Tests tst_core_equation`, puis merge.
2. **L7-T21** (forme II, worktree `--L7-T21`, 3c540c6) : vert, 6 mutants tués. Restent la revue adversariale, le rejeu `-Tests tst_ui_lot7:recognition` et le merge.
3. **L7-T19** (message défilant II, `implementeur-tdd`, worktree `--L7-T19`) : interrompue en cours de code (voir « Travail en vol »). Ensuite rejeu `-Tests tst_ui_lot7:ticker` et merge, qui ferme le lot P2-A.
4. Suite V11, puis L7-T23 ‖ L7-T25 ; puis T26‖T30, T27‖T31, T28‖T29 (T28 et T30 jamais ensemble : jeton `ecran`), T32, T33 (merge du lot 3), T34. Clôture du lot : sections « À vérifier à la clôture du lot » et « À faire en L7-T34 » de l'état. Enfin, prévenir A « lot 7 prêt à fusionner », avec le sha et le piège AUTOMOC.

Fini quand : 33/33 tâches mergées, suite verte, clôture du lot faite, A prévenu.

## Travail en vol à la fermeture de cette session

- **Agent L7-T19** : il codait encore à 19 h 03 (rouge commité en 8f785df, `TickerOverlay.qml` et `tickercontroller.cpp` modifiés, non commités). La nouvelle session ne peut pas le reprendre par SendMessage : relancer un `implementeur-tdd` sur le worktree `--L7-T19`, à partir de son dernier commit. S'il reste des fichiers modifiés non commités, les relire et les commiter en wip avant de lancer.
- **Rejeu de L7-T24 sur c29a5e7** : lancé avant la pause, il tenait le verrou B à 19 h. Il ne vaut plus, puisque le code change à l'essai 2. Sortie : `<wt T24>\.tmp\rejeu.txt`.
- **Verrou B et jeton `tests`** : libres quand ces deux travaux auront fini. Vérifier par `jeton.ps1 -Nom tests -Etat` (sortie lue telle quelle, sans tube) avant la première construction.
- **Question en attente de Jérôme** : aucune, hors le message de reprise de la pause.

## 🔧 Git

- `feat/livraison-complete-b` dans `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b`, locale, jamais poussée (A la fusionne).
- Branches de tâche avec worktree : `feat/livraison-complete-b--L7-T19`, `--L7-T21`, `--L7-T24`. Les autres worktrees sont retirés ; leurs branches sont gardées.
- Lot 3 livré dans main (70c8601) : il arrive dans B par le merge prévu en L7-T33.

## 🛠️ Skills utiles

`nouvelle-feature` (références `etage-3-execution.md` pour la reprise et la fin de lot, `etage-4-cloture.md` pour la clôture, `fichier-etat.md` pour les lignes de suivi).
