# Handoff — Reprise sur livraison-complete (lot 4) en NOUVELLE session

## MODE DE TRAVAIL

- **Chaîne `nouvelle-feature`, session orchestratrice.** Premier geste : « reprends le chantier livraison-complete », depuis le dossier principal du dépôt (`main`). La session ne code pas : elle lance des sous-agents nommés (`implementeur-tdd`, `concepteur-critique`), rejoue elle-même chaque preuve, fait relire (`reviewer-adversarial`, statique, 30 tours, « conclus avant »), merge et tient seule le fichier d'état.
- **Fichier d'état, qui fait autorité** : `.worktrees/livraison-complete/docs/sessions/livraison-complete-etat.md` (branche `feat/livraison-complete`). L'en-tête porte la pause et le **point de reprise exact** de chaque tâche interrompue.
- **Profil du projet** : `.claude/chantier.md`. Branche principale `main`, worktrees sous `.worktrees/`, merge `--no-ff`, **jamais de rebase**. La preuve Windows se fait à la demande par `preuves-windows.yml`, sous la forme bornée bash décrite au § Build et tests. Elle s'applique à une branche poussée qui contient `main`. Pas de « Porte avant merge ». La release reste toujours au go écrit de Jérôme.
- **Sur le Mac mini** : `~/pilotage-mac/consignes-actcraft.md` remplace les commandes `pwsh` du profil. Il n'y a ni `pwsh` ni `jeton.ps1` ici.
  - Configuration `build-mac` une fois par worktree (Qt de Homebrew, `-DACTCRAFT_BUILD_BENCH=ON`).
  - Toute compilation ou suite passe sous le verrou commun `~/.verrou-suite`. Ce verrou est partagé avec les autres sessions, qui y posent un fichier `pid`. Ne jamais le supprimer sans avoir vu son détenteur mort.
  - Suite : `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 600`.
  - Format : `~/.local/venvs/clang-format/bin/clang-format` 23.1.3, la version de la CI.
  - Ne pas commiter un `graphify update .` fait sur le Mac : il réécrit les chemins.
  - Notes et manques du Mac : `~/pilotage-mac/notes-actcraft.md`.
- **Interdits** : ne pas pousser `main` ; aucune release ; pas de rebase ; aucune ressource ni aucun nom de l'original (dépôt public) ; pas d'API privée de Qt ; pas de délai fixe comme attente dans un test.
- **Règles de la chaîne** :
  - Un vert rapporté n'est jamais cru : on rejoue la preuve soi-même.
  - Reste zéro dans le périmètre : ce qui se décrit se fait.
  - Une instabilité qui revient appelle une enquête avant tout merge.
  - Chaque commit est en français, avec des chemins explicites et les lignes d'attribution données par l'environnement.
- **Leçons du lot 4, à rappeler dans chaque brief QML** :
  - Toute barre ou boîte posée au-dessus de la vue se déclare couverture (`addInputCover`) et émet `popupOpened`/`popupClosed`. La preuve se fait au stylet.
  - Une prise de touche se prouve par une lecture qui la distingue du repli global.
  - Pas de `Repeater` recréé à chaque frappe ou à chaque zoom.
  - `textFormat` explicite.
  - Un contrôle de pixel sur un trait fin ou lissé échoue sous Windows : utiliser `checkEdge`.

Le 2026-10-27 au soir, ActCraft est **en pause sur décision de Jérôme**, sans attendre la fin du lot 4. Le lot 4 compte 37 tâches sur 38. Trois correctifs restent ouverts. Aucun agent ni aucune tâche de fond ne tourne.

## ✅ Fait cette session (Mac mini, 2026-10-27)

- **Mergé dans `feat/livraison-complete`** : main (PR #7, lot B11 : CI à la demande), L4-C16, L4-T26, L4-T19, L4-T27, L4-T23, L4-T20, L4-T29, L4-T25, L4-T28. Pour chacune, le fichier d'état garde la preuve rejouée, la relecture et les décisions.
- **L4-C16** : `tst_captures_formes` était instable. La cause était une mise en page en retard qui abandonnait un geste. Corrigé dans le joueur de scénarios.
- **Preuve Windows verte** jusqu'à `e3464a1` (run 40582280003).

## ⏭️ Prochain, dans l'ordre

Le détail exact de chaque point est dans l'en-tête du fichier d'état, sous « Point de reprise exact ».

1. **L4-C1**
   - Où : worktree `.worktrees/livraison-complete--L4-C1`, tête `213f7d9`, propre.
   - Fait : relecture et corrections.
   - Reste : jouer `.tmp/preuve-L4-C1.sh`, que la pause a interrompue, la rejouer soi-même, merger, pousser, lancer la preuve Windows.
2. **L4-C17**
   - Où : worktree `--L4-C17`, tête wip `a99a986`.
   - Fait : tests rouges `62a7a1e`, correctif en cours.
   - Reste : construire, prouver, faire relire, merger.
3. **L4-C6** : Échap ferme les boîtes modales. Non lancée.
4. Simplification du lot 4, dont la liste figure dans le journal du fichier d'état.
5. Clôture du lot 4 selon `etage-4-cloture.md` et le plan 4b. Ce qui exige le PC (captures et banc D3D11, `NOMS OK`) reste noté dans « Preuves Windows en attente ».
6. **Ne pas lancer le lot 5 sans Jérôme.**

## 🔧 Git

- `feat/livraison-complete` est poussée sur `origin` à `4e4d3df`. `main` est à `a50d9e6`, inchangé et jamais poussé par cette session.
- Les branches de tâche `feat/livraison-complete--L4-C1` et `--L4-C17` sont locales, non poussées.
- Ce handoff vit sur la branche du chantier, pas sur `main`, puisque `main` n'est pas poussé.

## 🔑 Accès

- `gh` 2.103.2 est authentifié sur le Mac et sert aux runs `preuves-windows.yml`.
- Aucun secret n'est nécessaire.

## 🧩 Pièges

- Voir « Pièges trouvés » dans le fichier d'état, et `~/pilotage-mac/notes-actcraft.md`.
- `sed` BSD peut échouer sans rien dire : vérifier chaque modification de fichier.
- Le verdict de la suite n'est pas toujours dans `.tmp/test.txt` : certains scripts écrivent dans `.tmp/p-suite.txt`.
- Arrêter son propre rejeu de preuve avant qu'un agent modifie le même worktree.

## ♻️ Reliquats hors chantier

- La CI signale des actions en Node.js 20, forcées en Node.js 24 (`checkout`, `setup-python`, `msvc-dev-cmd`). Elles sont à mettre à jour hors de ce chantier.

## 🛠️ Skills utiles

- `nouvelle-feature`, entrée « reprise » ; références `etage-3-execution.md` et `etage-4-cloture.md`.
- `superpowers:verification-before-completion`.
