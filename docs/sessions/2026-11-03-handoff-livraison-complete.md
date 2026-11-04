# Handoff — Reprise sur livraison-complete (lot 5) en NOUVELLE session

## MODE DE TRAVAIL

- **Chaîne `nouvelle-feature`, session orchestratrice.** Premier geste : « reprends le chantier livraison-complete », depuis le dossier principal du dépôt (`~/projets/actcraft`, branche `main`). La session ne code pas. Elle lance des sous-agents (`implementeur-tdd` en `model: sonnet`, `concepteur-critique` pour la conception), rejoue elle-même chaque preuve, merge `--no-ff` et tient seule le fichier d'état. Quatre agents au plus à la fois.
- **Décision de Jérôme (2026-11-02)** : enchaîner les lots sans pause ni go. Toute question bloquante est tranchée par l'orchestrateur et notée en hypothèse H5-xx (dernière : H5-25). Seule la release attend un go écrit de Jérôme.
- **Fichier d'état, qui fait autorité** : `.worktrees/livraison-complete/docs/sessions/livraison-complete-etat.md` (branche `feat/livraison-complete`). Lire d'abord les lignes `Lot :` et `Reprise`, puis les entrées « L5-xx mergée » : elles portent les notes à reprendre dans les briefs suivants.
- **Plan du sous-lot 5b** : `docs/plans/2026-10-18-livraison-complete-lot5b-PLAN.md` (table des tâches ligne 51, vagues ligne 70). Ne lire que la table et la section de la tâche.
- **Briefs** : les règles communes du lot (`commun-lot5.md`) et les briefs récents servent de modèles. Ils sont copiés dans `.worktrees/livraison-complete/.tmp/briefs-lot5/`, dossier non suivi. Chaque brief donne : worktree neuf, fichiers permis, **fichiers interdits des tâches parallèles**, notes des entrées « mergée », preuve en un seul script `.tmp/preuve-L5-xx.sh`, N = 158, « test rouge d'abord, vu sur le code de base, commité seul », « jamais `&` ni `nohup` ».
- **Profil du projet** : `.claude/chantier.md`. Sur le Mac, `~/pilotage-mac/consignes-actcraft.md` remplace les commandes `pwsh`.
  - Toute compilation ou suite passe sous le verrou commun `~/.verrou-suite`, par une boucle bornée : `for i in $(seq 1 240); do mkdir ~/.verrou-suite 2>/dev/null && break; sleep 30; done`. Un script qui prend le verrou n'en appelle jamais un autre qui le reprend.
  - Rejeu d'une preuve, toujours par l'outil Bash en tâche de fond : `bash .tmp/preuve-L5-xx.sh > <fichier> 2>&1`, puis vérifier `PREUVE OK`, `100% tests passed out of 158` dans `.tmp/test.txt` (horodatage du rejeu), mutations rouges puis vertes, arbre propre.
  - Contrôle avant merge : `B=$(git merge-base HEAD feat/livraison-complete)`, puis `git diff $B..HEAD -- tests bench | grep -E '^-.*(QSKIP|QCOMPARE|QVERIFY|void |expect|waitFor)'`. Seul le talon de la tâche doit partir ; toute autre ligne retirée doit avoir son remplacement.
  - Merge : `merge --no-ff`, `merge-base --is-ancestor`, `worktree remove --force`, `branch -D`, puis entrée « L5-xx mergée » dans l'état et commit `docs(etat): …`.
  - Contrôle d'avertissement : les lignes `warning:` du compilateur. Le message du générateur QML sur `DocumentTabs` est connu et exclu.
- **Interdits** : jamais de rebase ; ne pas pousser `main` ; aucune release sans go écrit ; ne pas toucher `feat/livraison-complete-b` ; jamais `git add -A`, `git add .` ni `git commit -a`. Relectures sur le Mac ; preuve Windows par la CI `preuves-windows.yml`.
- **Règles de la chaîne** : un vert rapporté n'est jamais cru ; reste zéro dans le périmètre ; une instabilité qui revient appelle une enquête avant tout merge.

Le 2026-11-03 à 10 h 15, la vague V14 du sous-lot 5b est close. Aucune tâche, aucun agent, aucun worktree de tâche ni verrou n'est en vol. La session précédente tournait par SSH depuis le PC, éteint à 11 h (décision de Jérôme : mode Mac seul).

## ✅ Fait cette session (2026-11-02 et 2026-11-03)

- **Sous-lot 5b** : T20 à T31 mergées. Correctifs C2 et C4 à C13 mergés. C3 est reportée au PC (H5-18).
- Ce matin : C11 (Quitter et fermeture demandent pour chaque onglet modifié), T30 (import PowerPoint), C12 (référence de `export-fond`), C13 (export pendant un soulèvement, entrées d'import masquées) et T28 (boîte d'impression). Chaque preuve a été rejouée par l'orchestrateur : 158/158.
- Hypothèses H5-21 à H5-25 : dans les entrées « mergée » correspondantes du fichier d'état.

## ⏭️ Prochain, dans l'ordre

La ligne `Reprise` du fichier d'état fait foi.

1. **L5-C14** ‖ **L5-C15**, en parallèle (fichiers disjoints) :
   - **C14** : la référence de `export-fond` mesure 20,0 ms d'encre p95, contre 11,2 ms avec l'export (rapport 0,56, invraisemblable). Cause probable : démarrage à froid. Ajouter un échauffement ou alterner l'ordre, puis obtenir un rapport ≥ 0,9 ou l'expliquer. Fichiers : `bench/src/runner.cpp`, `bench/src/scenarios.cpp`, `tests/tst_bench_scenarios.cpp`. Détail : entrée « L5-C12 mergée ».
   - **C15** : brancher la boîte d'impression. Ajouter `exchange: appExchange` au bloc `PrintSetupDialog` de `Main.qml`. Faire ouvrir la boîte par `file.print` (`FileActions.qml`) aussi quand `appPrint.available`. Puis jouer S17 boîte ouverte dans `bench/scenarios/imprimer.json`. Détail : entrée « L5-T28 mergée ».
2. Suite complète sur la tête du chantier.
3. V15 = **L5-T32** (scénario `edition-export`, S17). Sur le Mac, hors écran seulement ; « à l'écran en D3D11 » revient au PC ou à la CI.
4. V16 = **L5-T33** (contrôle de fin) : trancher `ACTCRAFT_PRINT` (T28 le juge non nécessaire ; chemin `ON` jamais joué) ; enregistrer en ctest les scénarios `onglets`, `exporter` et `importer`.
5. Clôture du lot 5 :
   - simplification : avertissement QML `DocumentTabs`, `appTabs.position`, `QTest::qWait(200)` restants ;
   - double relecture ;
   - `finalize-session-chaine` : push, PR en brouillon, CI `preuves-windows.yml`, `LICENCES OK` (H5-20).
6. Au PC seulement, quand il revient : L5-C3, `scripts/dev-env-android.ps1 -m qtpdf`, `NOMS OK`, exécution réelle de `pptx-to-pdf.ps1`, `nativeFormatKeptForPdfName`, `importer` sous Windows.

## 🔧 Git

- `feat/livraison-complete` est poussée sur `origin`. Ce handoff y est commité. `main` est à `042df64`, inchangé.
- Aucune branche de tâche ne reste, ni en local ni sur `origin`.
- Ce handoff vit sur la branche du chantier, pas sur `main`, puisque `main` ne se pousse pas pendant le chantier.

## 🔑 Accès

- `gh` est authentifié sur le Mac. Aucun secret n'est nécessaire.

## 🧩 Pièges

- `sed -i` BSD peut échouer sans rien dire : vérifier chaque modification (consigné dans `commun-lot5.md`).
- Un agent qui finit son tour sur « j'attends » sans tâche vivante : lui renvoyer le résultat de sa tâche de fond par `SendMessage`.
- Un agent peut écrire son code avant d'avoir vu le rouge : le brief le lui interdit, le contrôler dans ses commits.
- Voir aussi « Pièges trouvés » dans le fichier d'état et `~/pilotage-mac/notes-actcraft.md`.

## ♻️ Reliquats hors chantier

- La note `~/SecondBrain/04_Pieges/qtest-clic-a-width-moins-demi-pixel-hors-element-sous-windows.md` et son lien dans `06_MOC/actcraft.md` ne sont pas commités dans le coffre.
- **Question en attente de Jérôme, R4** : publier la release v0.5.0 (lot 4 mergé dans `main`, CI verte) ? Le lot 5 avance sans attendre la réponse.

## 🛠️ Skills utiles

- `nouvelle-feature`, entrée « reprise » ; références `etage-3-execution.md`, puis `etage-4-cloture.md`.
- `finalize-session-chaine` pour la PR du lot 5.
- `superpowers:verification-before-completion`.
