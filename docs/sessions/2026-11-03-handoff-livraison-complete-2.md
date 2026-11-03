# Handoff — Reprise sur livraison-complete (lot 5, clôture) en NOUVELLE session

## MODE DE TRAVAIL

- **Chaîne `nouvelle-feature`, session orchestratrice.** Premier geste : « reprends le chantier livraison-complete », depuis le dossier principal du dépôt (`~/projets/actcraft`, branche `main`). La session ne code pas. Elle lance des sous-agents (`implementeur-tdd` en `model: sonnet`, `concepteur-critique` pour la conception), rejoue elle-même chaque preuve, merge `--no-ff` et tient seule le fichier d'état. Quatre agents au plus à la fois.
- **Décision de Jérôme (2026-11-02)** : enchaîner les lots sans pause ni go. Toute question bloquante est tranchée par l'orchestrateur et notée en hypothèse H5-xx (dernière : H5-32). Seule la release attend un go écrit de Jérôme.
- **Fichier d'état, qui fait autorité** : `.worktrees/livraison-complete/docs/sessions/livraison-complete-etat.md` (branche `feat/livraison-complete`). Lire d'abord les lignes `Lot :` et `Reprise`, puis les entrées « L5-xx mergée » : elles portent les notes à reprendre dans les briefs suivants.
- **Plan du sous-lot 5b** : `docs/plans/2026-10-18-livraison-complete-lot5b-PLAN.md` (table des tâches ligne 51, vagues ligne 70). Ne lire que la table et la section de la tâche.
- **Briefs** : les règles communes du lot (`commun-lot5.md`) et les briefs récents servent de modèles. Ils sont copiés dans `.worktrees/livraison-complete/.tmp/briefs-lot5/`, dossier non suivi. Chaque brief donne : worktree neuf, fichiers permis, **fichiers interdits des tâches parallèles**, notes des entrées « mergée », preuve en un seul script `.tmp/preuve-L5-xx.sh`, N = 164, « test rouge d'abord, vu sur le code de base, commité seul », « jamais `&` ni `nohup` ».
- **Profil du projet** : `.claude/chantier.md`. Sur le Mac, `~/pilotage-mac/consignes-actcraft.md` remplace les commandes `pwsh`.
  - Toute compilation ou suite passe sous le verrou commun `~/.verrou-suite`, par une boucle bornée : `for i in $(seq 1 240); do mkdir ~/.verrou-suite 2>/dev/null && break; sleep 30; done`. Un script qui prend le verrou n'en appelle jamais un autre qui le reprend.
  - Rejeu d'une preuve, toujours par l'outil Bash en tâche de fond : `bash .tmp/preuve-L5-xx.sh > <fichier> 2>&1`, puis vérifier `PREUVE OK`, `100% tests passed out of 158` dans `.tmp/test.txt` (horodatage du rejeu), mutations rouges puis vertes, arbre propre.
  - Contrôle avant merge : `B=$(git merge-base HEAD feat/livraison-complete)`, puis `git diff $B..HEAD -- tests bench | grep -E '^-.*(QSKIP|QCOMPARE|QVERIFY|void |expect|waitFor)'`. Seul le talon de la tâche doit partir ; toute autre ligne retirée doit avoir son remplacement.
  - Merge : `merge --no-ff`, `merge-base --is-ancestor`, `worktree remove --force`, `branch -D`, puis entrée « L5-xx mergée » dans l'état et commit `docs(etat): …`.
  - Contrôle d'avertissement : les lignes `warning:` du compilateur. Le message du générateur QML sur `DocumentTabs` est connu et exclu.
- **Interdits** : jamais de rebase ; ne pas pousser `main` ; aucune release sans go écrit ; ne pas toucher `feat/livraison-complete-b` ; jamais `git add -A`, `git add .` ni `git commit -a`. Relectures sur le Mac ; preuve Windows par la CI `preuves-windows.yml`.
- **Règles de la chaîne** : un vert rapporté n'est jamais cru ; reste zéro dans le périmètre ; une instabilité qui revient appelle une enquête avant tout merge.

- **Sur le PC** : le profil `.claude/chantier.md` s'applique tel quel (`pwsh`, `scripts/test.ps1`, jetons) ; `~/pilotage-mac/consignes-actcraft.md` ne sert que sur le Mac.

Le 2026-11-03 vers 21 h 40, la session du Mac s'arrête à un point propre : tout le lot 5 est mergé dans `feat/livraison-complete` (tête 0158616 + ce handoff), la PR #9 du lot 5 est ouverte et prête, sa CI tourne. Aucun agent, aucun worktree de tâche, aucun verrou, aucune tâche de fond en vol.

## ✅ Fait cette session (2026-11-03, Mac seul)

- **L5-C14** (référence d'`export-fond` avec charge témoin, H5-28), **L5-C15** (boîte d'impression branchée, H5-26, H5-27, H5-29), **L5-C16** (`tst_ui_import` sans Qt PDF), **L5-T32** (S8 `edition-export`), **L5-T33** (contrôle de fin, 6 `tst_captures_*`, graphe), H5-30 (`ACTCRAFT_PRINT` gardée).
- **Fin de lot 3.8** : double relecture en 4 zones (tour 1 : 21 majeurs), correctifs **L5-C17 à C22** ; tour 2 sur les correctifs (2 majeurs), correctif **L5-C23** ; simplification **L5-T34** (avertissement QML `DocumentTabs` disparu) ; **L5-C24** (C4458 MSVC trouvé par la preuve Windows, et captures de lambda).
- Suite Mac debug **164/164**, 0 avertissement, 22 scénarios hors écran `SCENARIO OK`. Chaque preuve rejouée par l'orchestrateur.
- **Rapport du lot 5** et **PR du lot 5** (conditions de merge) : sections du fichier d'état. Entrées « L5-xx mergée » : détail de chaque tâche.

## ⏭️ Prochain, dans l'ordre

La ligne `Reprise` du fichier d'état fait foi.

1. **CI de la PR #9** (https://github.com/jejeasterix/actcraft/pull/9, run 41713303776 sur 0158616, lancé à 21 h 31 ; `format` vert au départ) : attendre le verdict par `attendre-ci` **sans** `-Pret` (la PR est déjà prête). Rouge : correctif `L5-C25` par la boucle de l'étage 3, push, attente de nouveau. Note : le push de ce handoff relance la CI sur la nouvelle tête (`synchronize`) ; c'est ce run-là qui compte.
2. **Merge de la PR #9** (`gh pr merge 9 --merge`) si toutes les conditions de la section « PR du lot 5 » sont vraies et CI verte (sortie 0 ou 6). Puis synchronisation de `main` locale (étape 9 de `finalize-session-chaine`).
3. **Restes pour le PC** (section « Rapport du lot 5 », puce « Restes pour le PC ») : captures D3D11 de S8 et S17, banc de fluidité et banc de clôture (dont H5-28 sans et avec charge), `scripts/test.ps1` debug et release, N Windows, `NOMS OK`, titres du corpus, `captures.ps1 -Scenario tous`, S17 complet, contraste de `PrintSetupDialog`, **L5-C3**, `dev-env-android.ps1 -m qtpdf`, `pptx-to-pdf.ps1` réel, `nativeFormatKeptForPdfName`, `importer` avec PowerPoint réel.
4. **Porte de fin** (étage 4.9) : questions **R4** et **R5** (ligne `Questions à Jérôme` de l'état), release du lot 5 due.

## ❓ Question R4 — un go relayé, non exécuté ici

Le pilote a relayé le 2026-11-03, mot pour mot, la réponse de Jérôme tapée dans la session pilote à R4 (« publier la release v0.5.0 ? ») : « go release ». La session du Mac ne l'a **pas** exécutée : le go est arrivé par relais d'une autre session (pas écrit dans la session du chantier), et l'ordre suivant du pilote était de s'arrêter à un point propre. La release est l'action finale (tag `v0.5.0` depuis `origin/main`, `release.yml`, fichier `go-action-finale`) : la session de relève la fait si le pilote ou Jérôme le confirme dans sa session. `main` est à 042df64 (lot 4), CI de la PR #8 verte.

## 🔧 Git

- `feat/livraison-complete` poussée sur `origin` (ce handoff compris). `main` = 042df64, inchangé ; la branche le contient (fusion sans contenu, aucun rebase).
- Aucune branche de tâche ne reste, ni locale ni distante. Worktree `.worktrees/build-mac` (`feat/build-mac`) : hors chantier, non touché.

## 🔑 Accès

- `gh` authentifié sur le Mac et le PC. Aucun secret nécessaire.

## 🧩 Pièges

- `graphify update .` sur le Mac : peut échouer (`list index out of range`) et avale `build-mac/` ; déplacer `build-mac` hors de l'arbre pendant la mise à jour (entrée « L5-T33 mergée »).
- CI de PR : juste après `gh pr ready`, un run peut tout sauter (le run de 19 h 54 l'a fait) ; un push sur la PR prête relance un vrai run.
- Un `build-mac` ancien peut garder `ACTCRAFT_IMPORT_PDF=OFF` en cache (entrée « L5-C16 mergée »).
- `sed -i` BSD : voir `commun-lot5.md`. Agents `implementeur-tdd` : limite de 40 tours souvent atteinte sur les gros briefs ; scinder (L5-C20) ou reprendre par `SendMessage`.
- Voir aussi « Pièges trouvés » dans le fichier d'état.

## ♻️ Reliquats hors chantier

- `render/src/imagestore.cpp:133` : capture de lambda qui masque une locale (hors lot 5).
- La note `~/SecondBrain/04_Pieges/qtest-clic-a-width-moins-demi-pixel-hors-element-sous-windows.md` et son lien dans `06_MOC/actcraft.md` ne sont pas commités dans le coffre.

## 🛠️ Skills utiles

- `nouvelle-feature` (entrée « reprise », puis `references/etage-4-cloture.md` à partir de 4.4) ; `finalize-session-chaine` (étapes 8 à 11) ; `superpowers:verification-before-completion`.
