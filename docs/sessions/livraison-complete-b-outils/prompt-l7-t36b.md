# L7-T36b — intégrer le lot 5 de A (main @ c1046b9) dans le chantier B

Rapport en français. Lis d'abord, dans `.tmp/outils-b/prompt-l7-correctifs.md` (worktree
`C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b`), les sections « Cadre », « Règles », « Preuves » et « Rapport » :
elles s'appliquent ici, sauf ce que ce brief change.

## Où
- Worktree : `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b--L7-corr`.
- Branche : `feat/livraison-complete-b--L7-T36b` (tête 5417de8 = branche du chantier après L7-T36).
- État du chantier (lecture seule, tu ne l'écris pas) : `docs/sessions/livraison-complete-b-etat.md`, section
  « Lot 5 de A dans main (2026-11-04, message du Pilote) », et les lignes H-T33 et H-T34 de la table (L7-T33, L7-T34).

## Quoi
1. `git merge --no-ff c1046b9` (le commit de `main` déjà présent localement ; `git fetch` interdit de modifier `main`).
   **Jamais de rebase**, jamais de push de `main`, jamais de `git add -A`.
2. Résous les conflits des 17 fichiers touchés des deux côtés en gardant **les deux comportements** : celui du lot 5
   de A et celui du lot 7 de B. Pour chaque fichier en conflit, lis l'intention des deux côtés (`git log` des deux
   branches sur le fichier) avant de choisir. `graphify-out/` : reprends la version de B puis `graphify update .` à la fin.
3. Revue de H-T33 et H-T34 contre le lot 5 : les lignes du lot 3 touchées par B (`toolcontext.h:93-95`,
   `selecttool.cpp:526,528`, `tooladapters.h:88`, `tooladapters.cpp:133-141`, et celles notées en H-T33) restent-elles
   correctes après le lot 5 ? Corrige ce qui casse ; consigne ce qui change de sens.
4. Comptes : N après merge = tests de B + tests ajoutés par A. Donne le nombre exact et sa justification
   (`ctest -N`), les comptes de scénarios du banc (`allScenariosInOrder` et voisins), et `commands.json`.

## Garde-fous propres à cette tâche
- RAM du PC basse (1,23 Go libres au lancement) : la résolution peut commencer tout de suite, mais aucune
  construction ni suite avant **2 Go libres** (`(Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory/1MB`),
  rien sous 1,5 Go, `CMAKE_BUILD_PARALLEL_LEVEL=4`, une seule suite lourde à la fois, sous le jeton
  (`.tmp/outils-b/b-jeton.ps1 -Proprietaire livraison-complete-b/L7-T36b -Commande "…"`).
- Piège prouvé en L7-T36 : le lien incrémental MSVC peut laisser des exécutables corrompus que ninja juge à jour
  (plantage 0xC0000005 au démarrage). Avant la première construction de chaque preset, supprime toutes les paires
  `build/<preset>/*.exe` + `*.ilk`. Piège AUTOMOC : supprimer `build/<preset>/ui/actcraft_ui_autogen/timestamp`.
- Test instable connu : `tst_captures_page_allongee_zones` (tâche L7-T40, ne le corrige pas ici) ; s'il est le seul
  rouge, relance-le seul et rapporte.
- Tu lances tes relecteurs éventuels au premier plan (`run_in_background: false`).
- Graphe : interroge `graphify query` avant de fouiller les fichiers.

## Preuves (à rendre)
- debug et release `TESTS OK` avec le N annoncé ;
- `tst_ui_lot7` classes desktop, presentation, recognition, mathtools vertes, et les tests du lot 5 de A nommés dans
  son état (`docs/sessions/livraison-complete-etat.md`) ;
- captures : `pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario tous -Rendu logiciel -HorsEcran -Sortie .tmp/captures-t36b` → `CAPTURES OK` ;
- `python -I "C:/Users/JeromeDenis/REA - Activinspire/tools/controle_noms_actcraft.py" .` → `NOMS OK` ;
- graphe commité.

## Rapport (une page)
Commit de merge et commits suivants, fichiers en conflit et choix fait pour chacun, résultat de la revue H-T33 / H-T34,
N et comptes, preuves avec les chemins des fichiers de sortie, hors périmètre, RESTE (doit être vide).
