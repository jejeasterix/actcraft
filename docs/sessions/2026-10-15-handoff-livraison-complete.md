# Handoff — Reprise sur livraison-complete (lot 2a) en NOUVELLE session

## MODE DE TRAVAIL (source : `.claude/chantier.md` de la branche du chantier, qui fait autorité)

- **Premier geste : « reprends le chantier livraison-complete »** (skill `nouvelle-feature`, entrée « reprise »
  de `references/etage-3-execution.md`). Le fichier d'état fait autorité :
  `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete\docs\sessions\livraison-complete-etat.md`.
- **Autonomie totale jusqu'à la livraison finale** (Jérôme, 2026-10-14 et 2026-10-15) : aucune question avant les
  portes de fin de lot, sauf blocage matériel. Les mesures sur machine précise (Dell 3510, MacBook M1) ne sont
  **pas** une porte (coffre `02_Decisions/0087`). Déléguer tout travail lourd à des sous-agents.
- **Branche principale `main`** ; chantier sur `feat/livraison-complete` (worktree `.worktrees/livraison-complete`) ;
  une tâche = un worktree `.worktrees/livraison-complete--L<lot>-T<n>`, mergé `--no-ff` dans la branche du chantier
  après preuve **rejouée par l'orchestrateur** ; 2 tâches en parallèle au plus.
- **Commandes** (toujours `pwsh -NoProfile`) : build `./scripts/build.ps1 [-Preset release]` ; tests sous jeton
  `pwsh -NoProfile -File ~/.claude/scripts/jeton.ps1 -Nom tests -Proprietaire <x> -Commande "Set-Location <wt>; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1"`,
  puis `Select-String … 'tests passed|TESTS (OK|EN ECHEC)'`. Preuve visuelle : `scripts/captures.ps1` (forme bornée
  au profil). Banc hors écran : toujours `-Taille 1280x800`.
- **Jamais `ctest` ni `tst_*.exe` sans `. ./scripts/dev-env.ps1`**, jamais depuis Git Bash (boîte « DLL introuvable »).
- **Orchestrateur : jamais `Set-Location` dans la session** (verrouille le worktree) → `git -C`, chemins absolus,
  ou `pwsh -NoProfile -Command "Set-Location …; …"`. Mutation de contrôle remise par `git checkout --`, jamais
  `Copy-Item`. Arrêter par PID ce qu'on lance (démon Gradle, émulateur).
- **Releases** : tag `v…`, `gh release …`, `gh api` en écriture sur releases = action finale, **jamais sans go écrit
  de Jérôme**. Au go : clé de signature Android de release + environnement GitHub `release` limité aux tags `v*`
  (profil § Paquets et release).
- **Dépôt public** : rien de Promethean (icônes, libellés, textes, noms, paperboards réels). `samples/` de l'ancien
  dépôt jamais modifié ni commité. Pas d'API privée de Qt dans le code livré.
- **Règles de la chaîne** : test rouge vu d'abord ; un vert rapporté n'est jamais cru (rejouer chaque preuve) ;
  revue adversariale de chaque tâche `concepteur-critique` ; reste zéro dans le périmètre ; exercer avant de clore.
  Les règles communes des tâches sont à **recréer dans le scratchpad** de la nouvelle session à partir de la
  section « Pièges trouvés » du fichier d'état et des « Règles communes » du plan 2a.

**État au 2026-10-15 (soir)** : lot 1 livré et mergé sur `main` (PR #3) ; lot 2a à mi-parcours (5 tâches sur 10
mergées dans `feat/livraison-complete`, branche intégrée 47/47 TESTS OK) ; aucun worktree de tâche ouvert, rien en cours.

## ✅ Fait cette session
- **Lot 1 terminé** : 20 tâches + 5 correctifs, double relecture, S1/S2/S3 joués (S2 à l'écran en D3D11), banc de
  fin de lot sans régression (`docs/mesures/2026-10-15-lot1-socle.md`), paquets construits, PR #3 mergée
  (`3de027a`), CI verte sur `main` (5 jobs dont macOS et Android `-gpu lavapipe`).
- **Porte de fin du lot 1** passée : release 0.1.0 « pas maintenant » (reproposée en 0.2.0 à la fin du lot 2) ;
  fonctions à revoir : aucune ; lot « cache incrémental » avancé juste après le lot 2 ; choix du lot 1 validés.
  Détail : section « Portes » du fichier d'état.
- **Plan du lot 2** écrit et relu en 3 tours : `docs/plans/2026-10-15-livraison-complete-lot2a-PLAN.md`
  (L2-T1…T10) et `…-lot2b-PLAN.md` (L2-T11…T23), @ `328ff9a`. Une seule PR à la fin de 2b.
- **Lot 2a** : L2-T1 (modèle complet), L2-T3 (commandes de fichier), L2-T4 (miniz), L2-T7 (Bamboo / index culm),
  L2-T8 (flux objet) mergées, chacune rejouée ; L2-T1 relue (hachage exhaustif par construction).
- **Coffre** : pièges `qt-quick-emulateur-android-swiftshader-rectangle-en-un-triangle`,
  `qt-macos-touche-injectee-ne-declenche-pas-les-shortcut`, complément de `03_Process/copy-item-horodatage-tests-menteurs` ;
  décision `0087-actcraft-mesures-sur-machine-de-reference-pas-une-porte`.

## ⏭️ Prochain (lot 2a, puis 2b)
- **Vague suivante** : L2-T2 (pile d'annulation, `implementeur-tdd` ; contrat : `AddObjectCommand` traite le 0 que rend
  `allocateId()` quand l'espace d'identifiants est épuisé) ‖ L2-T5 (`document.json`, `concepteur-critique`).
- Puis L2-T6 ‖ L2-T9, puis L2-T10 (oracle : commite un seul fichier `tools/oracle_flipchart.py` dans l'ancien dépôt,
  `git commit -- <fichier>`), puis le point d'étape 2a (section du plan 2a), puis le lot 2b.
- **Fini quand** (lot 2) : section « Clôture du lot 2 » du plan 2b (suite complète release + debug, S4 et S5 joués,
  banc comparé à `docs/mesures/2026-10-15-lot1-socle.md` avec `page_rebuilds_during_zoom ≥ 2` en D3D11, CHANGELOG
  0.2.0, graphe), PR du lot 2, CI verte, porte de fin (release 0.2.0 proposée).
- Après le lot 2 : lot « cache incrémental », puis les autres lots dans l'ordre du cadrage, lot vidéos (corpus
  `REA - Activinspire\docs\videos-tanguay\`) avant la release 1.0.

## 🔧 Git
- `main` = `3de027a` (PR #3), à jour sur `origin`. `feat/livraison-complete` = `6bc5636` (branche alignée sur `main`
  puis lot 2a), **non poussée** depuis la PR #3.
- Aucune branche de tâche vivante, aucune branche `essai-ci/*` sur `origin`, aucune release ni tag.

## 🔑 Accès
Aucune clé dans le dépôt. Outils Android dans `~/.tools` (`scripts/dev-env-android.ps1`). La clé de signature
Android de release n'existe pas encore (créée au go de la première release).

## 🧩 Pièges
Section « Pièges trouvés » du fichier d'état, et coffre `C:\SecondBrain` (carte `06_MOC/actcraft.md`). Nouveau ce
soir : le hook `~/.claude/hooks/garde-mail-roomflow.js` bloquait toutes les commandes shell sous forte charge
mémoire (node > 5 s) ; délai passé à 30 s dans `~/.claude/settings.json:119` (accord de Jérôme). Si le shell est de
nouveau refusé par ce hook : regarder la mémoire libre avant toute autre chose.

## ♻️ Reliquats hors chantier
- Étape 2 du BRIEF (Dell 3510, MacBook M1) : plus une porte ; procédure `docs/banc.md` disponible si Jérôme mesure.

## 🛠️ Skills utiles
`nouvelle-feature` (reprise), `finalize-session-chaine` (PR du lot 2), `transfert-session`.
