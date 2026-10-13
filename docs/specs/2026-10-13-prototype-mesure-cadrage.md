# Cadrage — prototype de mesure (étape 1 du BRIEF)

Cadrage autonome (étage 1 de `nouvelle-feature`), 2026-10-13. Sources : `docs/BRIEF.md`, `CLAUDE.md`,
`.claude/chantier.md`, code du squelette (commit 95e73e1), coffre (décisions 0074, 0075, 0076), passation
`REA - Activinspire/docs/sessions/2026-10-13-handoff-reecriture-qt6.md`, application Python de référence.

## 1. Demande et gabarit

**Demande.** Construire le prototype de mesure de l'étape 1 du BRIEF : une fenêtre QML montrant une page de
1024 × 768 à 300 objets (formes, images, traits), avec tracé au stylo incrémental, zoom, défilement et contenu
fixe en cache, plus un banc qui rejoue tracé, zoom et défilement en Direct3D 11 et en rendu logiciel, écrit temps
par image, latence et mémoire dans un fichier, et rend possible la mesure de l'étape 2 sur Dell 3510 et MacBook M1.

**Gabarit.** `chantier` confirmé. 37 questions tranchées sur 40.

**Répartition par module et estimation.** `core/` modèle, générateur, sonde · `render/` registre, moteurs
forme / image / trait, page fixe en cache, encre · `input/` échantillons · `tools/` `PenTool` · `ui/` `PageView`,
`Main.qml` · `bench/` scénarios, collecteurs, mémoire, statistiques, JSON, `actcraft-bench` · `scripts/`
`bench.ps1`, `paquet-banc.ps1` · `docs/` `banc.md`, résumé de mesure. Environ 13 tâches : vérifications DA2 /
DA3 / livraison du stylet, modèle + générateur, moteurs forme + trait, images, page fixe + cache, entrée,
`PenTool`, encre, `PageView` + `Main.qml`, statistiques + JSON + mémoire, scénarios + exécutable, fumée ctest,
scripts + doc + profil, mesure finale.

## 2. Décisions

| # | Point | Décision | Source | Réversible ? |
|---|---|---|---|---|
| 1 | Périmètre | Étape 1 seulement. La mesure sur Dell 3510 / M1 et la décision go / no-go (étape 2) restent hors chantier ; le chantier livre la commande, le format de résultats et la procédure écrite qui les rendent possibles. | `docs/BRIEF.md:101-110` | oui |
| 2 | Modèle de la page | Modèle minimal dans `core/` : `Page` + objets `Shape` (rectangle, ellipse, polygone ; remplissage, contour, opacité), `ImageObject` (rectangle, taille source, paramètres du motif), `Stroke` (points page + pression, largeur, couleur). Couleur en entier ARGB 32 bits : `QColor` est dans QtGui, interdit à `core/`. | `docs/BRIEF.md:51,74-75` ; `CMakeLists.txt:46` | oui |
| 3 | Générateur des 300 objets | Fonction pure de `core/`, graine fixe, résultat identique sur MSVC et Clang : `std::mt19937` seul, sans `std::uniform_*_distribution` (implémentations différentes selon la bibliothèque standard), conversion en réels écrite par nous. | `docs/BRIEF.md:102` ; CI Windows + macOS `.github/workflows/ci.yml` | oui |
| 4 | Composition de la page | 150 formes, 30 images, 120 traits ; 20 % des formes semi-transparentes ; traits de 50 à 300 points. Voir hypothèse N1-5. | défaut raisonnable | oui |
| 5 | Images des objets | Générées par notre code (dégradés et motifs procéduraux, 256 à 1024 px de côté), jamais lues sur disque ; aucun fichier image dans le dépôt. Génération sur un fil secondaire (`QThreadPool`, QtCore), affichage quand prêtes. | `CLAUDE.md` (propriété intellectuelle) ; `docs/BRIEF.md:78-79` | oui |
| 6 | Rendu des objets | Un moteur de rendu par type d'objet dans `render/`, choisi dans un registre (3 entrées : forme, image, trait), produisant des nœuds du graphe de scène — pas un élément QML par objet. | `docs/BRIEF.md:55` | oui |
| 7 | Rendu logiciel des formes et traits | Chaque moteur a deux chemins : nœuds de géométrie pour le RHI, `QSGRenderNode` peint au `QPainter` pour le rendu logiciel. La première tâche vérifie par un test si un `QSGGeometryNode` s'affiche en rendu logiciel ; s'il s'affiche, le chemin `QPainter` est abandonné. Voir DA2. | `docs/BRIEF.md:28` ; doc Qt `QSGRenderNode` (« via QPainter » en rendu logiciel) ; support des nœuds de géométrie non vérifié | oui |
| 8 | Cache du contenu fixe | Toute la page fixe dessinée dans une texture par le mécanisme de couche de Qt Quick, à la résolution zoom × rapport de pixels, plafonnée à 4096 px de côté, redessinée 150 ms après la fin d'un zoom et à chaque trait figé. Voir DA3. | `docs/BRIEF.md:80-81,87` | oui |
| 9 | Comparaison avec et sans cache | Option `--sans-cache` du banc et de l'application : mêmes nœuds, sans couche. Le banc chiffre ainsi la règle 2 de fluidité au lieu de la supposer. | `CLAUDE.md` (« mesure du banc, jamais une intuition ») | oui |
| 10 | Encre incrémentale | Trait en cours dans un élément à part (`render/`), au-dessus du cache, découpé en blocs de géométrie de taille fixe ; seul le dernier bloc change à chaque point. Voir DA5. | `docs/BRIEF.md:82-84` | oui |
| 11 | Trait terminé | Versé au modèle (`core::Stroke`), dessiné par le moteur « trait » dans la page fixe, cache redessiné une fois ; l'élément du trait en cours se vide dans la même image. | `docs/BRIEF.md:83-84` (traits figés, leçon d'OpenBoard) | oui |
| 12 | Prédiction du stylet | Absente à l'étape 1 ; la chaîne `input/` garde l'étage « prédiction » (identité) pour la brancher plus tard. Le banc mesure la latence sans prédiction. | `docs/BRIEF.md:54,123-126` | oui |
| 13 | Chaîne d'entrée | `input/` : événement Qt → échantillon (position, pression, horodatage, type de périphérique, numéro de séquence) → filtre (points identiques) → prédiction (identité) → outil. `tools/` : `PenTool`, qui ne connaît que des interfaces (document, encre). | `docs/BRIEF.md:54,56,71-72` | oui |
| 14 | Livraison du stylet à l'élément C++ | La première tâche d'entrée vérifie par un test que `QTabletEvent` (pression comprise) atteint un `QQuickItem` C++ sous Qt 6.12 ; repli : filtre d'événements sur la fenêtre. Ne jamais activer ensemble les deux compressions d'événements. | `docs/BRIEF.md:90-93` ; `CLAUDE.md` (pièges) | oui |
| 15 | Périphériques qui tracent | Stylet et souris (bouton gauche). Tactile et pincement : hors périmètre. | défaut raisonnable | oui |
| 16 | Vue (zoom, défilement) | Élément C++ `PageView` dans `ui/` (exposé au QML), qui transforme un élément de contenu ; pas de `Flickable` (le glisser appartient au stylo). | `docs/BRIEF.md:58` | oui |
| 17 | Zoom | Ctrl + molette, facteur 1,15 par cran (1,15^(delta/120) pour les molettes fines), centré sur le curseur, de 25 % à 800 %. | référence `app/actcraft/canvas/view.py:25,402-406` | oui |
| 18 | Défilement | Molette = vertical, Maj + molette = horizontal, page bornée dans la vue avec une marge. Pas de barres de défilement. | référence `view.py:407-412` (comportement hors mode « Ajuster ») ; défaut | oui |
| 19 | Zoom à l'ouverture | Application : « Ajuster » (page entière visible), comme la référence. Banc : 100 % imposé avant chaque scénario, pour comparer les passages. | référence `view.py:172` | oui |
| 20 | Commandes nommées | Aucune à l'étape 1 : ni bouton, ni menu, ni raccourci ; la molette et le stylo sont des gestes. Le registre `actions/` arrive à l'étape 4. | `docs/BRIEF.md:69-70,116-119` | oui |
| 21 | Interface visible | Fond actuel de `Main.qml` (`#3c3f45`), page blanche ; nom du rendu déplacé dans le titre de la fenêtre (un texte à l'écran coûterait une image par mise à jour). Aucun compteur d'images à l'écran. Porte visuelle de fin de lot : captures D3D11 et logiciel. | `ui/qml/Main.qml:9,20-24` ; `docs/BRIEF.md:85-86` | oui |
| 22 | Forme du banc | Exécutable séparé `actcraft-bench` dans `bench/`, qui charge la vraie interface (`ActCraft.Ui` / `Main`) et réutilise `app/renderbackend.cpp`. Voir DA1. | `CMakeLists.txt:24-37` (`ACTCRAFT_BUILD_BENCH`, `bench/` après `app/`) ; précédent `tests/CMakeLists.txt:17-20` | oui |
| 23 | Options du banc | `--rendu d3d11|logiciel|auto`, `--scenario complet|court`, `--sortie <fichier>`, `--sans-cache`, `--taille plein-ecran|LxH`, `--captures <dossier>`. Options en français, comme `--rendu` et `--quitter-apres`. | `app/main.cpp:33-37` ; `app/renderbackend.h:17` | oui |
| 24 | Scénarios rejoués | Après « page prête » (images chargées, premier cache dessiné) et 2 s de chauffe : **tracé** (10 traits de 1,5 s à 200 points/s, pression variable), **zoom** (100 → 800 → 25 → 100 % en 6 s, molette fine à 120 Hz), **défilement** (à 200 %, aller-retour vertical puis horizontal en 6 s). `court` : mêmes scénarios divisés par 5 (CI). | `docs/BRIEF.md:105` ; durées : défaut | oui |
| 25 | Injection des entrées | Événements Qt postés à la fenêtre (`QTabletEvent`, `QWheelEvent`) depuis un périphérique synthétique déclaré par le banc, cadencés par un minuteur précis indépendant des images. Pendant un scénario, la vue ignore les autres périphériques (la vraie souris ne fausse pas la mesure). Pas de `SendInput` : la souris de Jérôme n'est jamais pilotée. Voir DA6. | `CLAUDE.md` (mode autonome, point 3) | oui |
| 26 | Temps par image | Horodatage monotone de chaque `frameSwapped` pendant les scénarios ; on garde l'intervalle entre deux images et le temps de préparation (synchronisation + rendu) ; statistiques p50, p95, p99, max, images/s, part des images > 25 ms. Fréquence de l'écran relevée. | `docs/BRIEF.md:60,105` | oui |
| 27 | Latence d'encre | Latence **interne** : de l'injection d'un échantillon à la présentation (`frameSwapped`) de la première image qui le contient, par numéro de séquence. N'inclut ni le numériseur, ni la pile d'entrée du système, ni la composition (DWM), ni le balayage de l'écran. Voir N1-2. | `docs/BRIEF.md:45,105` ; limite : défaut | oui |
| 28 | Mémoire | Mémoire du processus au début et à la fin de chaque scénario, et pic : Windows `GetProcessMemoryInfo` (privée, ensemble de travail, pics), macOS `task_info` (empreinte physique). Mémoire de la puce graphique non mesurée (indiqué dans le fichier). | `docs/BRIEF.md:46,60` | oui |
| 29 | Fichier de résultats | Un JSON par passage, `"format": "actcraft-bench/1"`, clés en anglais ; dans `bench/results/` (ignoré par git) sous `<AAAA-MM-JJ_HHMMSS>-<machine>-<rendu>.json`. Contenu : métadonnées (commit, version de Qt, système, processeur, puce graphique via `QRhi::driverInfo`, écran, fréquence, rapport de pixels, rendu effectif, boucle de rendu, type de construction, cache oui/non), puis par scénario les séries brutes et les statistiques, puis le verdict. Seuls les résumés commités dans `docs/` font foi. | `.gitignore:20-21` ; `CLAUDE.md` (identifiants en anglais) | oui |
| 30 | Préset et validité | Une construction autre que `release` (RelWithDebInfo) écrit `"valid_for_measurement": false` et l'annonce à la console ; elle ne refuse pas de tourner (la CI tourne en debug). | `.claude/chantier.md:34` ; `CMakePresets.json:20-24` | oui |
| 31 | Verdict et code de sortie | Le banc sort à 0 si le passage s'est déroulé, 2 sinon ; le verdict (`within_budget`) est dans le JSON et en dernière ligne : `BUDGET TENU` ou `BUDGET DEPASSE`. `scripts/bench.ps1` (release, d3d11 puis logiciel) transforme ce verdict en code de sortie. Seuils : N1-1. | `.claude/chantier.md:44-48` | oui |
| 32 | Fenêtre | Plein écran sur l'écran principal par défaut (`QGuiApplication::primaryScreen`), taille effective écrite dans le JSON ; `--taille LxH` pour une fenêtre. | `CLAUDE.md` (pièges : écran principal) ; passation § 4 (« images par seconde en 1080p ») | oui |
| 33 | Tests | `core` : modèle et générateur (déterminisme, comptes par type). `input` : échantillons, séquence, filtre. `tools` : `PenTool`. `bench` : statistiques, JSON, mémoire. `render` / `ui` : chargement en rendu logiciel hors écran + pixels attendus sur une capture (un trait, une forme, une image). `tst_ui` adapté (l'élément `page` reste, 1024 × 768). | `CLAUDE.md` (tests) ; `.claude/chantier.md:35-36` ; `tests/tst_ui.cpp:27-31` | oui |
| 34 | Banc en CI | Test ctest `tst_bench_smoke` : `actcraft-bench --scenario court --rendu logiciel --sortie <tmp>` hors écran, puis contrôle du JSON (clés, 3 scénarios, images > 0). Aucun seuil de performance en CI (machines partagées). `--rendu logiciel` explicite : sans lui, le défaut Windows impose D3D11 malgré `QT_QUICK_BACKEND`. | `tests/CMakeLists.txt:8-10` ; `app/renderbackend.cpp:8-9` | oui |
| 35 | Commande du banc dans le profil | Une tâche écrit la commande du banc dans `.claude/chantier.md` § « Banc de fluidité ». | `.claude/chantier.md:48` (« Commande à écrire avec le banc ») | oui |
| 36 | Procédure pour l'étape 2 | `docs/banc.md` : commande, options, format du fichier, ce que la latence interne ne prouve pas, procédure exacte Dell 3510 (paquet) et MacBook M1 (construction sur place), commandes d'identification du matériel. | `docs/BRIEF.md:108-110` ; passation § 4 (commandes `Get-CimInstance`, `system_profiler`) | oui |
| 37 | Mesure de fin de chantier | Banc `complet` en release sur la machine de développement, D3D11 puis logiciel, avec et sans cache ; résumé chiffré commité dans `docs/mesures/2026-10-13-prototype-mesure.md`. Machine de développement : pas une référence (le résultat est nécessaire, pas suffisant). | `.claude/chantier.md:34` ; passation § 4 | oui |

## 3. Hypothèses à revoir

### Niveau 1 — difficiles à défaire ou visibles de Jérôme

- **N1-1 — Seuils du verdict.** « 60 images/s » devient : images/s médiane ≥ 58 **et** p95 de l'intervalle ≤ 18,5 ms
  **et** part des images > 25 ms ≤ 1 %, sur chaque scénario. « Latence < 20 ms » devient : p95 de la latence
  interne < 20 ms. Ces seuils décident du go / no-go de l'étape 2.
- **N1-2 — Latence interne seulement.** Sans matériel externe, on ne mesure pas la latence bout à bout ; la
  composition et l'affichage ajoutent en général une à deux images (17 à 33 ms à 60 Hz), non vérifié ici. Une
  latence interne < 20 ms ne prouve donc pas une latence perçue < 20 ms. Mesure bout à bout (caméra rapide du
  téléphone filmant la pointe et l'écran) décrite dans `docs/banc.md` comme option de l'étape 2, pas outillée.
- **N1-3 — Verdict porté par le rendu par défaut.** Le budget s'applique à D3D11 (Windows) et Metal (macOS). Le
  rendu logiciel est mesuré et son verdict calculé, mais marqué « secours, informatif » : il ne décide pas du go.
- **N1-4 — Distribution du banc pour l'étape 2.** Windows : `scripts/paquet-banc.ps1` produit un zip local
  (release + `windeployqt`) à copier sur le Dell, sans outil de construction. macOS : construction sur le M1
  (outils en ligne de commande Xcode, CMake, Ninja, Qt par aqtinstall). Aucun artefact publié par la CI, pour ne
  rien distribuer avant la décision go / no-go (`.claude/chantier.md:85`).
- **N1-5 — Représentativité de la page.** 150 formes, 30 images (256 à 1024 px), 120 traits, sans texte. Si les
  vrais paperboards de Jérôme sont plus lourds (photos 4K, texte abondant), le go sera optimiste. Le texte est
  hors périmètre (absent de la liste du BRIEF).
- **N1-6 — Netteté au fort zoom.** Cache plafonné à 4096 px : la page fixe reste nette jusqu'à 400 % (rapport de
  pixels 1), légèrement floue au-delà ; pendant un geste de zoom, la texture est étirée jusqu'au redessin
  (150 ms après). Le dessin par tuiles est reporté.

### Niveau 2 — les autres

- **N2-1** Durées et cadences des scénarios (décision 24) ; 200 points/s pour le stylet.
- **N2-2** Largeur du trait : 3 unités de page × (0,5 à 1,5 selon la pression), noir ; pas de lissage.
- **N2-3** Blocs de géométrie de l'encre : 256 points par bloc.
- **N2-4** Délai de redessin du cache après un zoom : 150 ms.
- **N2-5** Interface de sonde déclarée dans `core/` (C++ pur, sans Qt graphique), faute de module
  d'instrumentation dédié (voir DA4).
- **N2-6** Plein écran par défaut pour le banc (décision 32) : le mode plein écran peut changer le chemin de
  présentation sous Windows ; la taille et le mode sont écrits dans le JSON.
- **N2-7** Clés JSON en anglais, options de ligne de commande en français.
- **N2-8** Boucle de rendu laissée au défaut de Qt pour chaque rendu (elle est relevée dans le JSON, pas
  imposée).
- **N2-9** Pas de barres de défilement, pas de défilement cinétique, pas de pincement.
- **N2-10** Rendu des formes sans anticrénelage multi-échantillon dans le cache en RHI ; à revoir si les
  captures de la porte visuelle montrent des bords crénelés gênants (on mesurera alors le coût de
  `layer.samples`).

## 4. Décisions d'architecture

**DA1 — Le banc est un exécutable séparé qui charge la vraie interface.**
Contexte : `bench/` est déclaré après `app/` et désactivable (`ACTCRAFT_BUILD_BENCH`) ; l'application ne doit pas en dépendre.
Décision : `actcraft-bench` (console, sans paquet macOS) lie `bench/`, `ui/` et compile `app/renderbackend.cpp`, charge `ActCraft.Ui/Main`.
Conséquences : on mesure la même interface que l'application ; le démarrage commun tient en une petite fonction partagée.
Réversible : passer à une option `--banc` de l'application demanderait de déplacer `bench/` avant `app/`.

**DA2 — Deux chemins de rendu par moteur, tranchés par un test d'abord.**
Contexte : le rendu logiciel est le secours obligatoire ; la doc Qt dit que `QSGRenderNode` y passe par `QPainter`, rien sur les nœuds de géométrie.
Décision : tâche 1 = test qui affiche un `QSGGeometryNode` en rendu logiciel hors écran et lit les pixels ; s'il manque, chaque moteur fournit un nœud `QPainter` pour le logiciel.
Conséquences : le registre choisit le nœud selon `rendererInterface()->graphicsApi()` ; tests de pixels en logiciel, captures pour D3D11.
Réversible : le chemin `QPainter` se retire sans toucher aux autres moteurs.

**DA3 — Cache de la page fixe par la couche de Qt Quick.**
Contexte : règle 2 de fluidité ; zoom de 25 à 800 % ; règle 5 (résolution adaptée).
Décision : couche sur l'élément « page fixe », taille = page × zoom × rapport de pixels plafonnée à 4096 px, redessin après zoom et par trait figé ; `--sans-cache` la retire.
Conséquences : zoom et défilement ne coûtent qu'une texture transformée ; pic mesuré au redessin ; prise en charge de la couche en rendu logiciel à vérifier en même temps que DA2 (repli : pas de cache en logiciel, noté dans le JSON).
Réversible : remplacer par des tuiles ne touche que l'élément « page fixe ».

**DA4 — Instrumentation par sonde et numéros de séquence.**
Contexte : la latence exige de savoir quel échantillon est entré dans quelle image, sans que `render/` ni `input/` dépendent de `bench/`.
Décision : interface `InkProbe` dans `core/` (échantillon reçu n°, échantillon synchronisé n°) ; `input/` numérote, l'élément d'encre signale le dernier numéro synchronisé, `bench/` implémente la sonde et relie à `frameSwapped`.
Conséquences : coût nul sans sonde ; horloge monotone unique ; fils de rendu et principal protégés par un verrou court ou des atomiques.
Réversible : l'interface se déplace dans un module d'instrumentation dédié sans changer les appelants.

**DA5 — Encre en blocs de géométrie fixes.**
Contexte : règle 3 (chaque point s'ajoute au tampon GPU) ; réallouer une géométrie qui grandit recopie tout le trait.
Décision : blocs de 256 points ; seul le dernier est marqué modifié ; motif de données « dynamique » pour éviter le regroupement avec les nœuds fixes.
Conséquences : coût par point constant quelle que soit la longueur du trait ; jonctions entre blocs à soigner (point partagé).
Réversible : la taille des blocs et la stratégie restent internes à l'élément d'encre.

**DA6 — Entrées synthétiques par événements Qt postés.**
Contexte : rejouer sans piloter la souris de Jérôme, hors écran en CI, identique sous Windows et macOS.
Décision : périphérique synthétique (`QPointingDevice` stylet), `QTabletEvent` et `QWheelEvent` postés à la fenêtre, horodatés à l'injection, cadencés par un minuteur précis.
Conséquences : la pile d'entrée du système et la compression des événements de Qt ne sont pas mesurées (limite écrite dans le JSON et `docs/banc.md`).
Réversible : un injecteur natif (`SendInput`) peut s'ajouter derrière la même interface de scénario.

## 5. Scénarios d'acceptation

- **S1 — Banc D3D11 en release.** `pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu d3d11` construit en release,
  ouvre la fenêtre en plein écran sur l'écran principal et écrit `bench/results/<horodatage>-<machine>-d3d11.json`.
  Le fichier contient `format = actcraft-bench/1`, `graphics_api = direct3d11`, `valid_for_measurement = true`,
  les scénarios `stroke`, `zoom`, `scroll` avec chacun plus de 100 images, des statistiques d'intervalle, une
  latence (scénario `stroke`) et la mémoire ; la dernière ligne de la console est `BUDGET TENU` ou `BUDGET DEPASSE`.
- **S2 — Banc logiciel.** Même commande avec `-Rendu logiciel` : `graphics_api = software`, trois scénarios
  mesurés, verdict marqué informatif ; avec `--captures`, la capture de fin du tracé montre la page, ses objets
  et les 10 traits ajoutés.
- **S3 — Cache chiffré.** Deux passages D3D11, avec et sans `--sans-cache` : `cache` vaut `true` puis `false`
  dans les JSON ; le résumé `docs/mesures/2026-10-13-prototype-mesure.md` donne côte à côte p95 d'intervalle et
  latence des deux passages, et des deux rendus.
- **S4 — Fumée en CI.** `pwsh -NoProfile -File ./scripts/test.ps1` finit par `TESTS OK` ; `tst_bench_smoke`
  lance le banc `court` en logiciel hors écran en moins de 30 s, et le JSON produit a
  `valid_for_measurement = false` (debug) et ses trois scénarios ; la CI de la PR est verte sous Windows et macOS.
- **S5 — Prototype à la main.** `build/release/actcraft.exe --rendu d3d11` puis `--rendu logiciel` : la page de
  300 objets s'affiche en entier (« Ajuster ») ; un trait à la souris apparaît pendant le geste et reste après ;
  Ctrl + molette zoome jusqu'à 800 %, la molette fait défiler ; le trait est toujours là au retour à 100 %.
  Captures des deux rendus pour la porte visuelle.

## 6. Hors périmètre

- Étape 2 : mesures sur Dell 3510 et MacBook M1, décision go / no-go et sa note au coffre.
- Mesure de latence bout à bout outillée (caméra rapide, capteur) ; mode de mesure au vrai stylet.
- Prédiction du stylet, encre à faible latence native (`IDCompositionInkTrailDevice`, `androidx.graphics.lowlatency`).
- Texte, sélection, gomme, autres outils ; annulation ; plusieurs pages ; enregistrement et lecture de fichiers
  (`.flipchart`, format natif).
- Registre `actions/`, menus, barres d'outils, raccourcis.
- Tactile, pincement, défilement cinétique, barres de défilement.
- Cache par tuiles ; netteté au-delà de 400 %.
- Linux, Android, iOS.
- Publication d'un paquet ou d'une release ; artefacts de banc dans la CI.

## 7. Portes immédiates

Aucune.
