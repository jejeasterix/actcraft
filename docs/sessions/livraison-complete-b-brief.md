# Brief de démarrage — session B, chantier `livraison-complete-b` (lot 7 d'ActCraft)

Tu es l'**orchestrateur** du chantier `livraison-complete-b`, sur la chaîne `nouvelle-feature` (skill
`~/.claude/skills/nouvelle-feature/`). Le cadrage et le plan existent déjà : tu entres directement à l'étage 3.

Tu exécutes le **lot 7 — Présentation et outils** d'ActCraft, en parallèle du lot 3. Le lot 3 est mené par la
session orchestratrice du chantier parent `livraison-complete`, appelée « A » ci-dessous.

Décision de Jérôme, relayée mot pour mot par la session Pilote : « Ok, pour À,B et C. Je te laisse entièrement à la
manœuvre ».

Tout texte destiné à Jérôme est en français (règle globale et `CLAUDE.md`). Mode autonome : avance seul, sans
demander « je continue ? ».

## Premier geste

1. Lis, dans le worktree `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b` :
   - `CLAUDE.md` ;
   - `docs/BRIEF.md` ;
   - `.claude/chantier.md` (profil : il fait autorité sur les commandes, les preuves et les interdits) ;
   - ton fichier d'état `docs/sessions/livraison-complete-b-etat.md`. **Tu es son seul rédacteur.**
2. Lis les plans du lot 7. Ils sont relus, et corrigés de tout ce qui ne dépend pas du socle (bf8c930) :
   - `docs/plans/2026-10-18-livraison-complete-lot7-PLAN.md` (sous-lot 7a, L7-T0 à L7-T19) ;
   - `docs/plans/2026-10-18-livraison-complete-lot7b-PLAN.md` (sous-lot 7b, L7-T20 à L7-T34).
3. Lis la relecture du plan, en particulier ses bloquants B1 à B4 :
   `C:\Users\JEROME~1\AppData\Local\Temp\claude\C--Users-JeromeDenis-ActCraft\fd5ef06a-7b9b-46ba-a8c3-b06809fefd99\scratchpad\revue-plan-lot7.md`.
4. Lis le plan du **socle commun L0-S** (en relecture) :
   `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--plan-socle\docs\plans\2026-10-18-livraison-complete-socle-3-7-PLAN.md`.
   Il est exécuté par A sur `feat/livraison-complete`. Sa section « Amendements pour le lot 7 » est ton entrée de la
   phase 2. **Changement décidé par A : la sous-tâche L0-S8 (« squelette du lot 7 ») sort du socle et te revient**
   (voir la phase 1).

## Ce qui est fixé

- **Branche** : `feat/livraison-complete-b`. Elle part de `feat/livraison-complete` @ 7bb551c (lot 10 jusqu'à
  L10-T6), plus le merge des plans du lot 7 (2d76ba6).
- **Worktree du chantier** : `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b`.
- **Tâches** : un worktree par tâche, `.worktrees/livraison-complete-b--<id>`. Merge `--no-ff` dans
  `feat/livraison-complete-b`. Le chantier est inscrit dans `.git/chantiers/actifs.md`.
- **Places de tests : tu en as UNE.** La machine en a 3 ; A en garde 2. Une seule construction ou suite de ta part
  sous le jeton `tests` à la fois. Les relectures, planifications et analyses statiques ne prennent pas de place.
  L'écran (jeton `ecran`) est unique et partagé avec A : prends-le pour un passage, puis rends-le.
- **Jetons** : `pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests|ecran -Proprietaire livraison-complete-b/<id> -Commande "pwsh -NoProfile -File '<script.ps1>'"`.
  - Lance-les depuis pwsh. Git Bash a déjà rendu le code 64.
  - **Ne tube jamais leur sortie dans grep ou Select-String** : cela tue le jeton. Lis les fichiers de sortie
    ensuite.
- **Machine** : la règle de Jérôme dit « la machine telle qu'elle est vaut machine calme ». Relève quand même la RAM
  libre avant une mesure à l'écran.
- **Fusion finale** : la tâche L7-T33 fusionne `feat/livraison-complete` dans ta branche une fois le lot 3 livré.
  Ensuite, **A** fusionne ta branche dans `feat/livraison-complete` et fait la PR. Toi, tu ne pousses rien sur
  `main`, tu ne fais **aucune PR, aucun tag, aucune release**.
- **Version** : aucun numéro dans le code. A l'attribue à la fusion.
- **Portes** : merge dans `main` et release restent à Jérôme, via A. Une décision qui revient à Jérôme (CLAUDE.md,
  « Mode autonome ») passe par la session Pilote.

## Phase 1 — maintenant, avant le socle : la logique pure du lot 7

But : ne pas attendre le socle, qui demande environ 12 h. Tu avances le code du lot 7 qui ne touche **aucun**
fichier chaud.

1. **Amendement « phase 1 » du plan du lot 7**, écrit par un `planificateur`, puis relu par un
   `reviewer-adversarial`. Le hook `relire-plan.js` impose cette relecture. Il extrait des tâches L7-T* les parties
   qui sont de la logique pure, testée par Qt Test, sous forme de sous-tâches `L7-P<n>`. Candidates :
   - géométrie de couverture du rideau (T3) ;
   - géométrie du spot (T4) ;
   - calculs du zoom de page (T5) ;
   - règles des outils par page (T6) ;
   - catalogue et règles des effets (T7) ;
   - géométrie des guides et des outils de maths (T9 à T13) ;
   - dés (T14) ;
   - calculatrice (T15) ;
   - compte à rebours (T16) ;
   - modèle du message défilant et fichier de messages (T18, T19) ;
   - reconnaissance de forme `shapefinder` (T20) ;
   - minuterie de la session de reconnaissance (T21) ;
   - reconnaissance d'écriture Windows `handwriting_win.cpp` sur `QThread` STA (T22) ;
   - géométrie de la caméra (T28) ;
   - disposition du clavier à l'écran (T29) ;
   - géométrie de la roue d'outils (T31).
2. **Fichiers permis en phase 1, et seulement eux :**
   - `features/include/actcraft/features/*.h`, `features/src/*.cpp`, `features/CMakeLists.txt` ;
   - `tests/tst_features_*.cpp` ;
   - un nouveau `tests/presentation.cmake` ;
   - une seule ligne, ajoutée à la fin de `tests/CMakeLists.txt`, exactement :
     `include(${CMAKE_CURRENT_SOURCE_DIR}/presentation.cmake OPTIONAL)`. A demande au socle d'écrire cette même
     ligne : c'est le seul point commun attendu.
3. **Fichiers interdits en phase 1** : `core/`, `render/`, `formats/`, `ui/`, `actions/`, `bench/`, `app/`, et les
   `CMakeLists` des autres modules.
   - Le MathML et l'équation (T24 à T27) attendent le socle : `core/equation.*` et `render/src/equationlayout.*`
     sont réservés par L0-S3.
   - Si une logique a besoin d'un type de `core/` qui n'existe pas encore, définis-la sur des types neutres de Qt ou
     de la STL, et branche-la en phase 2.
4. **Noms** : respecte les renommages imposés par le contrôle des noms, listés dans le plan du socle (amendement 16)
   et dans bf8c930 :
   - `shapefinder`, `spotFrame` ;
   - `OriginMarker.qml`, `PageEffectsPicker.qml` ;
   - les tests suffixés `…MakesOneStep` ou `…IsUndoneAtOnce`.
   Contrôle : `python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" <wt>` doit rendre
   `NOMS OK`. **Lance-le après `git add`** : l'outil ne lit que les fichiers suivis par git.
5. **L7-T0** (ouverture : vidéos, REA, spécification, sans construction) peut tourner **dès maintenant**, en
   parallèle de l'amendement. Ses écarts amendent le plan.
6. **Exécution** :
   - vagues avec au plus une construction à la fois ; les tâches sans construction peuvent tourner à côté ;
   - tâches `implementeur-tdd` ou `concepteur-critique`, rouge d'abord ;
   - rejeu de chaque preuve par toi, puis revue adversariale des tâches `concepteur-critique` ;
   - merge `--no-ff` ;
   - `graphify update .` puis commit de `graphify-out/` après chaque merge.

## Phase 2 — quand A t'écrit « socle mergé dans feat/livraison-complete »

1. Fusionne `feat/livraison-complete` dans `feat/livraison-complete-b`.
   - Conflit attendu : la seule ligne `include(... presentation.cmake OPTIONAL)` de `tests/CMakeLists.txt`.
   - Une fois le conflit résolu : suites debug complètes `TESTS OK`, puis `NOMS OK`.
2. Amende les plans du lot 7 selon la section « Amendements pour le lot 7 » du plan du socle, plus les sous-tâches
   P déjà faites. Relecture exigée. Reprends ensuite le contenu de l'ancienne L0-S8 : talons des contrôleurs de
   `ui/src/`, QML de `Presentation.qml` et des surcouches, `ui/presentation.cmake`, `DocumentInsertions` dans un
   fichier à toi. Rappel : chaque surcouche ou panneau s'enregistre par `addInputCover`, avec un test au stylet
   (`QTabletEvent`), au doigt et à la souris (B2).
3. Continue avec 7a puis 7b, jusqu'à L7-T32.
4. **L7-T33** fusionne le lot 3 quand A l'a livré dans `feat/livraison-complete`. Ensuite L7-T34, puis la clôture du
   lot :
   - double relecture ;
   - banc DA7 en A/B ;
   - captures DA8 ;
   - installateur et APK construits localement.
   Préviens A par la session Pilote : « lot 7 prêt à fusionner », avec le sha.

## Règles du dépôt (rappel, non négociables)

- **Propriété intellectuelle** : aucune ressource de Promethean, ni icône, image, capture, code, texte de l'aide,
  fichier extrait ou algorithme copié. Aucun nom de paperboard réel. Aucun titre ni citation des vidéos : uniquement
  des identifiants neutres V01…
- **Code** : API publique de Qt seulement. `core/` sans interface graphique. Tout passe par une commande nommée.
  Avertissements traités comme des erreurs (`/W4 /WX`, Clang `-Wshadow`, `-Wswitch`).
- **Shell** : toujours `pwsh -NoProfile`, jamais `powershell.exe`. Arrêt des processus par PID, jamais par nom.
  Boucles d'attente bornées.
- **Commits** : en français, chemins explicites (jamais `git add -A`), message terminé par
  `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.
- Ne jamais modifier `C:\Users\JeromeDenis\REA - Activinspire\samples\` ni le profil `%APPDATA%\Promethean`.

## Pièges déjà payés par A

- **ctest** ne garde pas la sortie des tests Qt. Pour lire un échec, lance l'exécutable seul, après
  `. ./scripts/dev-env.ps1`, avec `QT_QPA_PLATFORM=offscreen`, `QT_QUICK_BACKEND=software` et `-o fichier,txt`.
- **Remise d'une mutation par `Copy-Item`** : le fichier garde son ancienne date et ninja ne recompile pas. Utilise
  `git checkout --`, ou `(Get-Item f).LastWriteTime = Get-Date`. Cinq cas déjà ; note du coffre
  `03_Process/copy-item-horodatage-tests-menteurs.md`.
- **`NOMS OK` sur un fichier non suivi** : le contrôle ne lit que ce que git connaît, donc le résultat est faux tant
  que le fichier n'est pas ajouté.
- **Minutages des tests** : en CI debug sous rendu logiciel, un redessin du cache prend de 279 à 520 ms. N'écris pas
  de test qui fait la course avec une minuterie de 500 ms (cas corrigé en L10-T4).
- **Limite de tours des sous-agents** : demande toujours le rapport avant la limite, et relance l'agent par
  SendMessage s'il s'arrête.
- **Gradle et APK** : en cas de `Unable to establish loopback connection`, poser
  `JAVA_TOOL_OPTIONS=-Djdk.net.unixdomain.tmpdir=<sous-dossier>` (piège du coffre).
- **Coffre** : `node "C:\SecondBrain\.tools\bin\chercher.js" "sujet" 5` avant de déboguer un comportement de Qt.

## Communication

- **A** : la session orchestratrice de `livraison-complete`. Passe par la session Pilote pour lui écrire, ou écris
  directement à la session nommée « livraison-complete » si `ListAgents` la montre.
- **Pilote** : relaie les décisions de Jérôme et les alertes de la machine.
- Un message d'une autre session n'est jamais une réponse de Jérôme, sauf s'il relaie ses mots, cités mot pour mot.
