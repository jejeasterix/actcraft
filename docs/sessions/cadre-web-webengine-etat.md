# État — cadre-web-webengine

Statut : en cours
Gabarit : chantier
Avancement : 8/18 tâches terminées
Étage : 3 (vague 2)
Tâche : T11 — Coller une URL d'image, et mineurs d'insertion — depuis 2026-11-09 23:17
Questions à Jérôme : —
Worktree : /Users/jeromedenis/projets/actcraft/.worktrees/cadre-web-webengine   Branche : feat/cadre-web-webengine   Base : origin/feat/livraison-complete-l11 @ 76315dbc
Branche principale locale au lancement : 8a293f30
Cadrage : docs/specs/2026-11-09-cadre-web-webengine-cadrage.md   Plan(s) : docs/plans/2026-11-09-cadre-web-webengine-PLAN.md
Relectures : mac

Demande : O146 (Jérôme, 2026-11-09, canal ActCraft : « Tu peux commencer la tâche Qt WebEngine, dernière du chantier ? »). Décision de référence : `~/SecondBrain/02_Decisions/0217-actcraft-liberte-avant-securite-et-qt-webengine.md`. Pas de porte humaine : une question bloquante se tranche depuis 0217 et devient une hypothèse à revoir.

Interdits de la tâche : aucune release, aucun tag ; `release.yml` non touché (O98) ; pas de fusion, PR en brouillon vers `feat/livraison-complete-l11` ; aucun job sur le PC ; aucune machine GitHub ; pas de contact avec O84, O86, O89 ; jeton `tests`, une seule compilation lourde ; `NOMS OK`.

## Essai de faisabilité (2026-11-09, avant tout plan) — RÉUSSI
Application Qt Quick d'essai (hors dépôt, scratchpad de session), préfixe `CMAKE_PREFIX_PATH=/opt/homebrew` :
- construit et lié contre Qt 6.12.0 (qbase, qtdeclarative) et qtwebengine Homebrew 6.11.2_1 (Chromium 140). Cette formule est reconstruite contre Qt 6.12 : ses frameworks se déclarent 6.12.0.
- `CMAKE_PREFIX_PATH=/opt/homebrew/opt/qt` échoue (chemins d'en-têtes `opt/qt/include/QtWebEngine*` inexistants) : le préfixe doit être `/opt/homebrew`, ou un chemin équivalent pour WebEngine.
- à l'exécution : `qVersion` 6.12.0, page chargée, `runJavaScript` rend le résultat, code de sortie 0.
- capture `grabToImage` d'un `WebEngineView` : PNG 640 × 480, pixel relevé rouge attendu (RGB 200,30,30).
- H.264 (Homebrew, Mac) : `canPlayType` « probably », `MediaSource.isTypeSupported` vrai ; AAC, MP3, VP9 aussi. Décodage réel d'une vidéo : à faire.
Mélange retenu : Homebrew tel quel. `aqt` non nécessaire.

## Référence
Rapide et complète (base 76315dbc, Mac debug, `ctest -j2 --timeout 120` hors écran) : 329/330. Seul rouge : `tst_ui_linklauncher` (`htaRefused`, 184 dossiers d'extraction contre 183 attendus : le test compte les dossiers du temp commun, une autre suite du Mac en crée en même temps). Rouge préexistant, instable, pas une régression.

## Compteurs
contre-relecture cadrage 1/2 (1 bloquant, 5 majeurs, 6 mineurs corrigés, ea065489) · relecture plan 2/3 · vagues sans progrès 0/2 · correctifs de fin 0/2

## Scénarios
S1 dépôt et copie hors écran, formats exacts du moteur · S2 coller une URL d'image, échecs, changement d'onglet · S3 gardes rallumées par le réglage, effet immédiat, cookie A/B · S4 paquet Mac : poids avant/après, H.264, provenance de Qt, pas de Chromium sans cadre, coût du premier cadre · S5 Android sans WebEngine, greffon dans l'APK

## Tâches (détail : table du plan)
| id | dépend | subagent | état | essais | dernier commit vert | preuve rejouée |
|----|--------|----------|------|--------|---------------------|----------------|
| T1 | — | concepteur-critique | terminée | 1/3 | ccb45113 | 332/332 ; `realFrameLoadsThroughTheLoader`, `contractsMatch` PASS ; revue : 0 bloquant, 0 majeur |
| T2 | T1 | implementeur-tdd | terminée | 1/3 | 060684b6 | 332, seul rouge linklauncher préexistant ; guardsFollowTheProfileSetting, desktopLayerFollowsTheSetting PASS |
| T14 | T1 | implementeur-tdd | terminée | 1/3 | 766f1b1d | 332/332 ; settingStepIsParsed PASS ; SCENARIO OK (boucle 237 ms, informatif) |
| T15 | T1 | implementeur-tdd | terminée | 1/3 | d808bc4a | tst_app_notices 6/6 ; everyAllowedQtModuleHasANotice PASS ; LICENCES OK sur bundle factice |
| T3 | T2, T14 | concepteur-critique | terminée | 1/3 | 41829385 | 332/332 ; guardsOffSendsNoRequest, heavyHtmlGoesToAFile PASS ; revue : 2 majeurs, 5 mineurs corrigés dans T4 |
| T4 | T3 | implementeur-tdd | terminée | 1/3 | 816fb61b | 332/332 ; reportFillsTitleAndHistory, guardsOffCommandsGoToTheView PASS |
| T9 | T1 | implementeur-tdd | terminée | 1/3 | a581f85d | 333/333 ; tst_ui_webimagefetch 10/10, oversizeWithoutLengthIsRefused PASS |
| T10 | T4, T9 | concepteur-critique | terminée | 1/3 | b0ab9ab6 | 333/333 ; webDropDownloadsAnImage, downloadNeverLandsInAnotherTab, webDropOfOversizeFails PASS ; revue : 0 bloquant, 0 majeur |
| T11 | T10 | implementeur-tdd | à faire | 0/3 | — | — |
| T5 | T2, T4 | concepteur-critique | à faire | 0/3 | — | — |
| T6 | T5 | implementeur-tdd | à faire | 0/3 | — | — |
| T7 | T6 | implementeur-tdd | à faire | 0/3 | — | — |
| T8 | T7 | implementeur-tdd | à faire | 0/3 | — | — |
| T12 | T8, T11 | implementeur-tdd | à faire | 0/3 | — | — |
| T13 | T12 | implementeur-tdd | à faire | 0/3 | — | — |
| T16 | T13, T15 | concepteur-critique | à faire | 0/3 | — | — |
| T17 | T13 | implementeur-tdd (isolé) | à faire | 0/3 | — | — |
| T18 | T16, T17 | implementeur-tdd | à faire | 0/3 | — | — |

## Terminé quand
toutes les tâches terminées (aucune bloquée) · double relecture sans bloquant ni majeur · suite complète Mac verte en debug et en release · S1…S5 réussis · poids du `.dmg` et H.264 écrits · PR en brouillon ouverte vers `feat/livraison-complete-l11`, non fusionnée

## Décisions de l'orchestrateur
- Banc limité à T1, T5, T7, T12, T18 (Mac, Release, 8 Go) : T6, T8, T13 touchent le QML du cadre, pas le chemin d'encre ; T18 mesure le cumul. Écart assumé au n° 36 du cadrage.
- Relectures par agents au premier plan (ordre O146), sans `relire-sur-mac.cjs`.
- Mineurs de la revue de T1 reportés en T2 (`engineBuilt` retiré, assertion `AA_ShareOpenGLContexts`, commentaires périmés) et à T5 (dépréciation de `WebEngineProfile {}`).
- Reprise du 2026-11-09 19:56 (processus arrêté pendant la construction de T2) : implémentation non commitée de T2 gardée et agent repris, au lieu du rejet prévu par la chaîne, car l'ordre de reprise dit de ne pas refaire ce qui est fait. La preuve reste rejouée au rapport.
- **Consigne du canal ActCraft (2026-11-09, jusqu'à la fin du chantier)** : aucun `run_in_background`, aucun sous-agent en fond, ne jamais terminer le tour pour attendre (en `claude -p`, la fin du tour tue le fond). Construction longue : `nohup … > .tmp/x.txt 2>&1 &` sous le jeton, attente au premier plan par appels Bash (`timeout` 600000, boucle sur un fichier de fin). Même règle pour les sous-agents, écarte « tâche de fond » des références de la chaîne.
- Revue de T10 : 4 mineurs. Câblage `webFrames` sans test au plan de T12 : test ajouté à T12 (`MainWindow`, gardes allumées, dépôt puis collage, `requests() == 0`). Autres mineurs d'`insertion` joints à T11.
- Phase 0 (3.1) : la preuve `ctest` détecte un échec, vu sur la référence (`tests failed`, code de `tst_ui_linklauncher`).

## Hypothèses à revoir
Voir le cadrage : 15 hypothèses (H1 JavaScript et adresses distantes sur Android, H2 codecs selon la provenance de Qt, H3 `initialize()` sous Windows, H4 mémoire du Dell, H5 glisser réel à l'écran, H8 fichiers du système acceptés, H14 URL collée sans réseau, H15 premier cadre vide le temps du `Loader`).

## Restes pour le PC
À compléter depuis le cadrage § 7 à la clôture. Restes pour Jérôme : R1 glisser réel sur Mac, R2 appui long, collage et capture sous Android, R3 décision de licence sur la redistribution des décodeurs H.264 avant la première release avec WebEngine.

## Portes
ouvertes : — · passées : —

## Pièges trouvés
- `CMAKE_PREFIX_PATH` sur `opt/qt` casse le lien à WebEngine (voir essai) ; l'application ne lie pas WebEngine (chargement QML au premier cadre, DA2), donc `opt/qt` reste.
- Prémisses démenties : aucun `DropArea` de page n'existait ; une image glissée arrive souvent en `file:` temporaire ; `selectedText` n'existe pas sur `WebEngineView` ; `QtWebEngineCore` 275 Mo + 68 Mo de ressources, « 150 Mo » sous-estimé.
- Chantiers sur les mêmes fichiers : PR #23 (base), #21 et #22 (`ci.yml`), #2 (`CLAUDE.md`, `CMakeLists.txt`).
