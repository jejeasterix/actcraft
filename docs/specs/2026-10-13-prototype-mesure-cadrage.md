# Cadrage — prototype de mesure (étape 1 du BRIEF)

Cadrage autonome (étage 1 de `nouvelle-feature`), 2026-10-13, révisé après la contre-relecture (tour 1).
Sources : `docs/BRIEF.md`, `CLAUDE.md`, `.claude/chantier.md`, code du squelette (commit 95e73e1), coffre
(décisions 0074, 0075, 0076), passation `REA - Activinspire/docs/sessions/2026-10-13-handoff-reecriture-qt6.md`,
application Python de référence, binaires de Qt 6.12 installés (`C:\Qt\6.12.0\msvc2022_64`).

## 1. Demande et gabarit

**Demande.** Construire le prototype de mesure de l'étape 1 du BRIEF : une fenêtre QML montrant une page de
1024 × 768 à 300 objets (formes, images, traits), avec tracé au stylo incrémental, zoom, défilement et contenu
fixe en cache, plus un banc qui rejoue tracé, zoom et défilement en Direct3D 11 et en rendu logiciel, écrit temps
par image, latence et mémoire dans un fichier, et rend possible la mesure de l'étape 2 sur Dell 3510 et MacBook M1.

**Gabarit.** `chantier` confirmé. 38 questions tranchées sur 40 ; les 2 restantes ne sont pas consommées, aucune
question ne reste ouverte.

**Répartition par module et estimation.** `core/` modèle, générateur, sonde · `render/` registre, moteurs
forme / image / trait (chemins RHI et logiciel), page fixe en cache, encre (chemins RHI et logiciel) · `input/`
échantillons · `tools/` `PenTool` · `ui/` `PageView`, `Main.qml` · `bench/` scénarios, collecteurs, mémoire,
statistiques, JSON, contrôle de contenu, `actcraft-bench` · `scripts/` `bench.ps1`, `bench.sh`,
`paquet-banc.ps1` · CI : passage macOS · `docs/` `banc.md`, résumé de mesure. Environ 15 tâches : vérifications
(nœuds logiciels, couche en logiciel, livraison du stylet), modèle + générateur, moteurs forme + trait, images,
page fixe + cache, entrée, `PenTool`, encre RHI, encre logicielle, `PageView` + `Main.qml`, statistiques + JSON
+ mémoire, scénarios + exécutable + captures + contrôle de contenu, fumée ctest + passage CI macOS, scripts + doc
+ profil, mesure finale.

## 2. Décisions

| # | Point | Décision | Source | Réversible ? |
|---|---|---|---|---|
| 1 | Périmètre | Étape 1 seulement. La mesure sur Dell 3510 / M1 et la décision go / no-go (étape 2) restent hors chantier ; le chantier livre les commandes, le format de résultats et la procédure écrite qui les rendent possibles. | `docs/BRIEF.md:101-110` | oui |
| 2 | Modèle de la page | Modèle minimal dans `core/` : `Page` + objets `Shape` (rectangle, ellipse, polygone ; remplissage, contour, opacité), `ImageObject` (rectangle, taille source, paramètres du motif), `Stroke` (points page + pression, largeur, couleur). Couleur en entier ARGB 32 bits : `QColor` est dans QtGui, interdit à `core/`. | `docs/BRIEF.md:51,74-75` ; `CMakeLists.txt:44` | oui |
| 3 | Générateur des 300 objets | Fonction pure de `core/`, graine fixe, **arithmétique entière seulement** (coordonnées en 1/16 de pixel, `std::mt19937`, aucune distribution de la bibliothèque standard, aucun `sin`/`cos`, conversion en réels à la fin) : MSVC et Clang (contraction FMA par défaut sur arm64) donnent alors le même résultat. Un test compare l'empreinte du modèle à une constante, sous Windows et macOS en CI. Trois **objets témoins** (une forme, une image à centre uni, un trait épais), de couleurs connues, au premier plan à des positions connues, font partie des 300. | `docs/BRIEF.md:102` ; `.github/workflows/ci.yml` (Windows + macOS) ; constat de relecture 20 | oui |
| 4 | Images des objets | Générées par notre code (dégradés et motifs procéduraux), jamais lues sur disque ; aucun fichier image dans le dépôt. Génération sur un fil secondaire (`QThreadPool`, QtCore), affichage quand prêtes. | `CLAUDE.md` (propriété intellectuelle) ; `docs/BRIEF.md:78-79` | oui |
| 5 | Rendu des objets | Un moteur de rendu par type d'objet dans `render/`, choisi dans un registre (3 entrées : forme, image, trait), produisant des nœuds du graphe de scène — pas un élément QML par objet. | `docs/BRIEF.md:55` | oui |
| 6 | Rendu logiciel des formes, traits **et de l'encre** | Le rendu logiciel n'a pas de nœud de géométrie générique : `Qt6Quick.dll` ne contient que `QSGSoftware{Glyph,Image,InternalImage,InternalRectangle,NinePatch,Painter,Rectangle,Renderable,Sprite}Node`. Chaque moteur, et l'élément d'encre, a donc deux chemins : nœuds de géométrie pour le RHI, `QSGRenderNode` peint au `QPainter` (avec rectangle borné, pour garder les mises à jour partielles) pour le logiciel. Un test de pixels hors écran le confirme en première tâche. Voir DA2. | symboles de `C:\Qt\6.12.0\msvc2022_64\bin\Qt6Quick.dll` (relevés le 2026-10-13) ; doc Qt `QSGRenderNode` (« via QPainter ») ; `docs/BRIEF.md:28` | oui |
| 7 | Cache du contenu fixe | Toute la page fixe dessinée dans une texture par le mécanisme de couche de Qt Quick, à la résolution zoom × rapport de pixels, plafonnée à 4096 px de côté, redessinée 150 ms après la fin d'un zoom et à chaque trait figé ; l'élément signale la fin de chaque redessin. Voir DA3. | `docs/BRIEF.md:80-81,87` | oui |
| 8 | Comparaison avec et sans cache | Option `--sans-cache` du banc et de l'application : mêmes nœuds, sans couche. Le banc chiffre ainsi la règle 2 de fluidité au lieu de la supposer. | `CLAUDE.md` (« mesure du banc, jamais une intuition ») | oui |
| 9 | Encre incrémentale | Trait en cours dans un élément à part (`render/`), au-dessus du cache. RHI : blocs de géométrie de taille fixe, seul le dernier change à chaque point. Logiciel : `QSGRenderNode` qui ne repeint que le rectangle des nouveaux segments. Voir DA5. | `docs/BRIEF.md:82-84` | oui |
| 10 | Trait terminé | Versé au modèle (`core::Stroke`), dessiné par le moteur « trait » dans la page fixe, cache redessiné une fois ; l'élément du trait en cours se vide dans la même image. | `docs/BRIEF.md:83-84` (traits figés, leçon d'OpenBoard) | oui |
| 11 | Prédiction du stylet | Écart à la règle 3 du BRIEF : absente à l'étape 1, étage « prédiction » (identité) gardé dans `input/`. Hypothèse N1-9. | `docs/BRIEF.md:82` (« avec prédiction du stylet ») ; hypothèse | oui |
| 12 | Chaîne d'entrée | `input/` : événement Qt → échantillon (position, pression, horodatage, type de périphérique, numéro de séquence) → filtre (points identiques) → prédiction (identité) → outil. `tools/` : `PenTool`, qui ne connaît que des interfaces (document, encre). | `docs/BRIEF.md:54,56,71-72` | oui |
| 13 | Livraison du stylet à l'élément C++ | La première tâche d'entrée vérifie par un test que `QTabletEvent` (pression comprise) atteint un `QQuickItem` C++ sous Qt 6.12 ; repli : filtre d'événements sur la fenêtre. Ne jamais activer ensemble les deux compressions d'événements. | `docs/BRIEF.md:90-93` ; `CLAUDE.md` (pièges) | oui |
| 14 | Vue (zoom, défilement) | Élément C++ `PageView` dans `ui/` (exposé au QML), qui transforme un élément de contenu ; pas de `Flickable` (le glisser appartient au stylo). | `docs/BRIEF.md:58` | oui |
| 15 | Zoom | Ctrl + molette, de 25 % à 800 %, 1,15 par cran. Ancrage et molettes fines : hypothèse N2-11. | référence `app/actcraft/canvas/view.py:25,402-405` | oui |
| 16 | Défilement | Molette = vertical ; horizontal = composante x de la molette, et Maj + molette seulement si x est nul (macOS convertit déjà Maj + molette en horizontal, non vérifié ici : on n'inverse jamais une composante x déjà présente). Page bornée dans la vue avec une marge, sans barres de défilement. | référence `view.py:412` (défilement natif hors mode « Ajuster ») ; hypothèse N2-9 | oui |
| 17 | Zoom à l'ouverture | Application : « Ajuster » (page entière visible), comme la référence. Banc : niveau imposé avant chaque scénario, puis attente de la fin du redessin du cache avant d'ouvrir la fenêtre de mesure. | référence `view.py:172` ; constat de relecture 17 | oui |
| 18 | Commandes nommées | Aucune à l'étape 1 : ni bouton, ni menu, ni raccourci ; la molette et le stylo sont des gestes. Le registre `actions/` arrive à l'étape 4. | `docs/BRIEF.md:69-70,116-119` | oui |
| 19 | Interface visible | Fond actuel de `Main.qml` (`#3c3f45`), page blanche ; nom du rendu dans le titre (pas de texte à l'écran, qui coûterait une image par mise à jour). Aucun compteur d'images à l'écran. Le rendu effectif figure dans le nom des captures et dans le JSON (le titre est invisible en plein écran). | `ui/qml/Main.qml:9,20-24` ; `docs/BRIEF.md:85-86` | oui |
| 20 | Forme du banc | Exécutable console séparé `actcraft-bench` dans `bench/`, qui charge la vraie interface (`ActCraft.Ui` / `Main`) et réutilise `app/renderbackend.cpp`. Voir DA1. | `CMakeLists.txt:14,33-35` ; précédent `tests/CMakeLists.txt:17-20` | oui |
| 21 | Options du banc | `--rendu d3d11|logiciel|auto` (inchangé ; `auto` = Metal sous macOS, le rendu effectif est relevé), `--scenario complet|court`, `--sortie <fichier>`, `--sans-cache`, `--taille plein-ecran|LxH`, `--captures <dossier>`. Sortie par défaut : dossier `resultats-banc/` à côté de l'exécutable (le zip du Dell n'a pas d'arborescence de dépôt). | `app/main.cpp:33-37` ; `app/renderbackend.cpp:22-30` | oui |
| 22 | Scénarios rejoués | Après « page prête » (images chargées, premier cache dessiné) et 2 s de chauffe : **tracé** (10 traits de 1,5 s à 200 points/s, pression variable), **zoom** (100 → 800 → 25 → 100 % en 6 s, molette fine à 120 Hz), **défilement** (à **400 %**, aller-retour vertical puis horizontal en 6 s ; à 200 % en 1920 px, l'horizontal ne parcourrait qu'environ 128 px). `court` : durées divisées par 5. Durées : N2-1. | `docs/BRIEF.md:105` ; constat de relecture 17 | oui |
| 23 | Injection des entrées | Événements Qt postés à la fenêtre (`QTabletEvent`, `QWheelEvent`) depuis un périphérique synthétique, cadencés par un minuteur précis. Chaque échantillon porte son **instant prévu** t0 + i × 5 ms ; le retard réel d'injection (prévu → posté) et de réception (prévu → reçu par la vue) est écrit dans le JSON. Pendant un scénario, la vue ignore les autres périphériques. Pas de `SendInput` : la souris de Jérôme n'est jamais pilotée. Voir DA6. | `CLAUDE.md` (mode autonome, point 3) ; constat de relecture 8 | oui |
| 24 | Temps par image | Horodatage monotone de chaque `frameSwapped`. Ne comptent que les **intervalles actifs** : au moins une entrée injectée depuis l'image précédente (les pauses entre traits n'entrent pas dans les statistiques). Statistiques : p50, p95, p99, max, part des intervalles > 25 ms ; **images/s = 1000 / intervalle médian**. Les intervalles qui contiennent un figement de trait sont comptés à part. Fréquence de l'écran relevée. | `docs/BRIEF.md:60,105` ; constat de relecture 3 | oui |
| 25 | Latence d'encre | Latence **interne**, de l'instant prévu de l'échantillon au `frameSwapped` de la première image qui le contient (par numéro de séquence), en deux composantes enregistrées : **prévu → synchronisation** (attente et travail du fil principal) et **synchronisation → `frameSwapped`** (rendu et présentation, attente de la synchronisation verticale comprise). N'inclut ni le numériseur, ni la pile d'entrée du système, ni la composition (DWM), ni le balayage de l'écran. | `docs/BRIEF.md:45,105` ; constats de relecture 2 et 8 | oui |
| 26 | Mémoire | Mémoire du processus au début et à la fin de chaque scénario, et pic : Windows `GetProcessMemoryInfo` (privée, ensemble de travail, pics), macOS `task_info` (empreinte physique). Mémoire graphique : D3D11 par `IDXGIAdapter3::QueryVideoMemoryInfo` (périphérique obtenu par `QSGRendererInterface::getResource`), Metal par `currentAllocatedSize` du `MTLDevice` ; absente en logiciel. Elle entre dans le verdict (N1-7). | `docs/BRIEF.md:45-46` ; coffre 0074 (budget 4 Go) ; constat de relecture 4 | oui |
| 27 | Fichier de résultats | Un JSON par passage, `"format": "actcraft-bench/1"`, clés en anglais. Nom : `<AAAA-MM-JJ_HHMMSS>-<modèle de machine>-<rendu>.json` (modèle matériel, **jamais le nom d'hôte ni le nom d'utilisateur**). Contenu : métadonnées (commit, version de Qt, système, processeur, puce graphique par les objets natifs de `QSGRendererInterface` — pas `QRhi::driverInfo`, qui exige `Qt6::GuiPrivate` —, écran, fréquence, rapport de pixels, rendu effectif, boucle de rendu, type de construction, options de compilation, cache oui/non, taille et mode de fenêtre), puis par scénario les séries brutes et les statistiques, le contrôle de contenu, puis le verdict. `scripts/bench.ps1` écrit dans `bench/results/` (ignoré par git) ; seuls les résumés commités dans `docs/` font foi. | `.gitignore:19-20` ; `CLAUDE.md` (identifiants en anglais) ; constats de relecture 16 et 19 | oui |
| 28 | Préset, compilation et validité | Mesures en `release` (RelWithDebInfo). Sous MSVC, `/Ob1` remplacé par `/Ob2` pour cette configuration (inlining complet, comme une vraie version) ; les options effectives sont écrites dans le JSON. `"valid_for_measurement": false` si la construction n'est pas `release`, ou si la fenêtre perd l'exposition ou l'activation pendant un scénario ; le banc l'annonce et ne refuse pas de tourner (la CI tourne en debug). | `.claude/chantier.md:34` ; `build/release/CMakeCache.txt:55` (`/Zi /O2 /Ob1 /DNDEBUG`) ; constats de relecture 9 et 15 | oui |
| 29 | Verdict et code de sortie | Le banc sort à 0 si le passage s'est déroulé, 2 sinon ; le verdict (`within_budget`, détaillé par critère) est dans le JSON et en dernière ligne : `BUDGET TENU` ou `BUDGET DEPASSE`. `scripts/bench.ps1` lance d3d11 puis logiciel et sort à 1 si le verdict **du rendu par défaut** (d3d11) est dépassé, 0 sinon ; le logiciel est informatif (N1-3). Seuils : N1-1, N1-7. | `.claude/chantier.md:44-48` ; constat de relecture 21 | oui |
| 30 | Fenêtre | Plein écran sur l'écran principal par défaut (`QGuiApplication::primaryScreen`), taille et mode écrits dans le JSON ; `--taille LxH` pour une fenêtre. Avant un passage plein écran sur le poste de Jérôme, l'orchestrateur l'annonce d'une ligne (sans attendre de réponse) : la fenêtre couvre l'écran principal 30 à 60 s. | `CLAUDE.md` (pièges : écran principal ; mode autonome) ; passation § 4 (« images par seconde en 1080p ») | oui |
| 31 | Captures et contrôle de contenu | Avec `--captures`, une séquence de démonstration **non mesurée** suit les scénarios : capture au milieu d'un trait, à la fin du trait, à 800 %, puis de retour à 100 % (le trait est toujours là). Noms : `<rendu>-<étape>.png`. Sur la capture à 100 %, le banc lit la couleur des 3 objets témoins (tolérance bornée) et écrit `content_check` (ok / échec par témoin) dans le JSON ; un échec rend le passage invalide. C'est ce qui prouve qu'un chemin D3D11 dessine réellement. | `CLAUDE.md` (mode autonome, point 3) ; `.claude/chantier.md:40-42` ; constats de relecture 6 et 9 | oui |
| 32 | Tests | `core` : modèle et générateur (déterminisme, empreinte, comptes par type, témoins). `input` : échantillons, séquence, filtre. `tools` : `PenTool`. `bench` : statistiques (intervalles actifs, figements), JSON, mémoire. `render` / `ui` : chargement en rendu logiciel hors écran + pixels attendus (un trait, une forme, une image, l'encre en cours). `tst_ui` adapté (l'élément `page` reste, 1024 × 768). | `CLAUDE.md` (tests) ; `.claude/chantier.md:35-36` ; `tests/tst_ui.cpp:27-31` | oui |
| 33 | Banc en CI | Fumée par ctest, sans `actcraft_add_test` (qui construit son propre exécutable, `tests/CMakeLists.txt:2-11`) : `add_test` lance `$<TARGET_FILE:actcraft-bench> --scenario court --rendu logiciel --sortie <build>/bench-smoke.json`, `TIMEOUT 60`, avec `QT_QPA_PLATFORM=offscreen` ; un test Qt `tst_bench_result` (fixture ctest dépendante) contrôle le JSON : 3 scénarios, images > 0, `graphics_api = software`, `valid_for_measurement = false`. Hors écran, Qt charge toujours le rendu logiciel, même avec `--rendu d3d11` : aucun seuil de performance ni test D3D11 en CI. | `tests/CMakeLists.txt:2-11` ; constat de relecture 10 (vérifié par le relecteur, non rejoué) | oui |
| 34 | macOS | `scripts/bench.sh` (bash, présent par défaut sur macOS ; `pwsh` ne l'est pas) : construction release par les presets puis passage du banc en `auto`. La CI macOS ajoute un pas **informatif** (`continue-on-error`) qui lance `actcraft-bench --scenario court --rendu auto` sur la plateforme `cocoa` et relève `graphics_api` ; s'il ne tourne pas en Metal, N1-8 s'applique. | `scripts/dev-env.ps1:6-32` (propre à Windows) ; `app/renderbackend.cpp:22-30` ; constat de relecture 7 | oui |
| 35 | Commande du banc dans le profil | Une tâche écrit dans `.claude/chantier.md` § « Banc de fluidité » : les commandes (`scripts/bench.ps1`, `scripts/bench.sh`) et le point de départ de la règle de refus (après la décision go / no-go de l'étape 2, voir décision 38). | `.claude/chantier.md:44-48` (« Commande à écrire avec le banc ») | oui |
| 36 | Procédure pour l'étape 2 | `docs/banc.md` : commandes, options, format du fichier, ce que la latence interne ne prouve pas, mesure bout à bout possible par caméra rapide (non outillée), procédure exacte Dell 3510 et MacBook M1, commandes d'identification du matériel. Paquet Dell : `scripts/paquet-banc.ps1` (release, `windeployqt --release --compiler-runtime`, zip local). | `docs/BRIEF.md:108-110` ; passation § 4 ; constat de relecture 19 | oui |
| 37 | Mesure de fin de chantier | Banc `complet` en release sur la machine de développement, D3D11 puis logiciel, avec et sans cache, avec captures ; résumé chiffré commité dans `docs/mesures/2026-10-13-prototype-mesure.md`, sans nom d'hôte. La machine de développement n'est pas une référence. | `.claude/chantier.md:34` ; passation § 4 | oui |
| 38 | Verdict : résultat ou porte ? | **Pendant ce chantier, le verdict du banc est un résultat, pas une porte.** Les tâches antérieures au banc sont validées par leurs tests seuls ; les tâches postérieures passent au banc et joignent le JSON, sans refus sur dépassement. Un `BUDGET DEPASSE` en fin de chantier produit un rapport chiffré (critère, valeur, seuil, scénario) et une hypothèse N1 pour Jérôme, sans bloquer le merge. La règle de refus du profil et du BRIEF s'applique après la décision go / no-go. | `.claude/chantier.md:46-48` et `docs/BRIEF.md:88` (règle 6, qui suppose un banc et une décision d'étape 2 existants) ; arbitrage de l'orchestrateur, option la plus réversible | oui |

## 3. Hypothèses à revoir

### Niveau 1 — difficiles à défaire ou visibles de Jérôme

- **N1-1 — Seuils du verdict.** Par scénario, sur les intervalles actifs hors figements : images/s ≥ 58
  (intervalle médian ≤ 17,2 ms), p95 de l'intervalle ≤ 18,5 ms, part des intervalles > 25 ms ≤ 1 %. Figements
  de trait comptés à part : au plus une image perdue par figement (intervalle ≤ 33,4 ms). Latence : **p50 de la
  latence interne < 20 ms**, p95 et les deux composantes rapportés sans seuil. Conséquence : à 60 Hz, l'attente de
  la synchronisation verticale suit à peu près une loi uniforme de 0 à 16,7 ms (p95 ≈ 15,8 ms) ; un seuil sur le
  p95 total ne laisserait qu'environ 4 ms de travail, irréaliste sans encre à faible latence. Sans comptage à part,
  10 figements sur environ 900 images feraient déjà 1,1 % d'images perdues.
- **N1-2 — Latence interne seulement.** La composition et l'affichage ajoutent en général une à deux images
  (17 à 33 ms à 60 Hz), non vérifié ici. Une latence interne < 20 ms ne prouve pas une latence perçue < 20 ms.
- **N1-3 — Verdict porté par le rendu par défaut.** D3D11 (Windows) et Metal (macOS) décident du go. Le rendu
  logiciel est mesuré, son verdict calculé, mais marqué « secours, informatif ».
- **N1-4 — Distribution du banc pour l'étape 2.** Windows : zip local à copier sur le Dell, sans outil de
  construction. macOS : construction sur le M1 (outils en ligne de commande Xcode, CMake, Ninja, Qt par
  aqtinstall). Aucun artefact publié par la CI, pour ne rien distribuer avant la décision go / no-go
  (`.claude/chantier.md:85`).
- **N1-5 — Représentativité de la page.** 150 formes (20 % semi-transparentes), 30 images de 256 à 1024 px,
  120 traits de 50 à 300 points, sans texte. Si les vrais paperboards sont plus lourds (photos 4K, texte
  abondant), le go sera optimiste.
- **N1-6 — Netteté au fort zoom.** Cache plafonné à 4096 px : page fixe nette jusqu'à 400 % (rapport de pixels 1),
  un peu floue au-delà ; pendant un geste de zoom, la texture est étirée jusqu'au redessin.
- **N1-7 — Seuil mémoire.** Le budget « tenir dans 4 Go avec Windows et un navigateur » devient : pic de mémoire
  privée du processus + pic de mémoire graphique déclarée ≤ 1 Go. Sur une puce intégrée, une partie peut être
  comptée deux fois : le critère est prudent. Le cache seul pèse jusqu'à 4096 × 3072 × 4 = 48 Mo, le double
  pendant un redessin.
- **N1-8 — Chemin Metal jamais exécuté avant le M1.** Si le pas CI macOS ne tourne pas en Metal (machine
  virtuelle sans accès Metal, non vérifié), la première exécution Metal et celle de `scripts/bench.sh` auront lieu
  chez Jérôme à l'étape 2.
- **N1-9 — Pas de prédiction du stylet.** Écart à la règle 3 du BRIEF (`docs/BRIEF.md:82`) : le banc mesure la
  latence sans prédiction ; la prédiction ne réduirait que la latence perçue, pas la latence interne mesurée.

### Niveau 2 — les autres

- **N2-1** Durées et cadences des scénarios (décision 22) ; 200 points/s pour le stylet.
- **N2-2** Largeur du trait : 3 unités de page × (0,5 à 1,5 selon la pression), noir ; pas de lissage.
- **N2-3** Blocs de géométrie de l'encre RHI : 256 points par bloc.
- **N2-4** Délai de redessin du cache après un zoom : 150 ms.
- **N2-5** Interface de sonde déclarée dans `core/` (C++ pur), faute de module d'instrumentation dédié (DA4).
- **N2-6** Plein écran par défaut pour le banc : il peut changer le chemin de présentation sous Windows ; taille et
  mode sont écrits dans le JSON.
- **N2-7** Clés JSON en anglais, options de ligne de commande en français.
- **N2-8** Boucle de rendu laissée au défaut de Qt pour chaque rendu (relevée dans le JSON, pas imposée).
- **N2-9** Pas de barres de défilement, pas de défilement cinétique ; conversion Maj + molette de macOS non vérifiée.
- **N2-10** Formes sans anticrénelage multi-échantillon dans le cache RHI ; à revoir si les captures montrent des
  bords crénelés gênants (on mesurera alors `layer.samples`).
- **N2-11** Zoom centré sur le curseur, facteur 1,15^(delta/120) pour les molettes fines. La référence centre sur
  la page (`view.py:167` `AnchorViewCenter`, `view.py:340` `centerOn(pr.center())`) et applique 1,15 par
  événement sans tenir compte de l'amplitude (`view.py:404`).
- **N2-12** Périphériques qui tracent : stylet et souris (bouton gauche) ; tactile et pincement hors périmètre.
- **N2-13** `/Ob2` sous MSVC pour la configuration RelWithDebInfo (décision 28) ; Clang garde ses options.
- **N2-14** Pas de fil d'injection dédié par `QWindowSystemInterface::handleTabletEvent` (API privée,
  `Qt6::GuiPrivate`) : l'horodatage à l'instant prévu suffit à ne pas sous-estimer la latence quand le fil
  principal est occupé.

## 4. Décisions d'architecture

**DA1 — Le banc est un exécutable séparé qui charge la vraie interface.**
Contexte : `bench/` est déclaré après `app/` et désactivable (`CMakeLists.txt:14,33-35`) ; l'application ne doit pas en dépendre.
Décision : `actcraft-bench` (console, sans paquet macOS) lie `bench/`, `ui/` et compile `app/renderbackend.cpp`, charge `ActCraft.Ui/Main`.
Conséquences : on mesure la même interface que l'application ; le démarrage commun tient en une petite fonction partagée.
Réversible : passer à une option `--banc` de l'application demanderait de déplacer `bench/` avant `app/`.

**DA2 — Deux chemins de rendu par moteur et pour l'encre.**
Contexte : le rendu logiciel est le secours obligatoire et n'a pas de nœud de géométrie générique (symboles de `Qt6Quick.dll`).
Décision : chaque moteur et l'élément d'encre fournissent un nœud RHI et un `QSGRenderNode` peint au `QPainter`, choisis selon `rendererInterface()->graphicsApi()` ; un test de pixels hors écran le confirme d'abord.
Conséquences : double travail par type ; tests de pixels en logiciel hors écran, contrôle de contenu D3D11 par le banc (décision 31).
Réversible : le chemin `QPainter` se retire sans toucher aux autres moteurs si une version de Qt rend la géométrie en logiciel.

**DA3 — Cache de la page fixe par la couche de Qt Quick.**
Contexte : règle 2 de fluidité ; zoom de 25 à 800 % ; règle 5 (résolution adaptée).
Décision : couche sur l'élément « page fixe », taille = page × zoom × rapport de pixels plafonnée à 4096 px, redessin après zoom et par trait figé, signal de fin de redessin ; `--sans-cache` la retire.
Conséquences : zoom et défilement ne coûtent qu'une texture transformée ; pic mesuré au redessin ; prise en charge de la couche en logiciel vérifiée en première tâche (repli : pas de cache en logiciel, noté dans le JSON).
Réversible : remplacer par des tuiles ne touche que l'élément « page fixe ».

**DA4 — Instrumentation par sonde et numéros de séquence.**
Contexte : la latence exige de savoir quel échantillon est entré dans quelle image, sans que `render/` ni `input/` dépendent de `bench/`.
Décision : interface `InkProbe` dans `core/` (échantillon reçu n°, échantillon synchronisé n°) ; `input/` numérote, l'élément d'encre signale le dernier numéro synchronisé, `bench/` implémente la sonde et relie à `frameSwapped` et aux instants prévus.
Conséquences : coût nul sans sonde ; horloge monotone unique ; fils de rendu et principal protégés par des atomiques ou un verrou court.
Réversible : l'interface se déplace dans un module d'instrumentation dédié sans changer les appelants.

**DA5 — Encre incrémentale, un tampon par chemin.**
Contexte : règle 3 (chaque point s'ajoute au tampon GPU) ; réallouer une géométrie qui grandit recopie tout le trait.
Décision : RHI, blocs de 256 points, seul le dernier marqué modifié, motif de données « dynamique » ; logiciel, repeinte du seul rectangle des nouveaux segments.
Conséquences : coût par point constant quelle que soit la longueur du trait ; jonctions entre blocs à soigner (point partagé).
Réversible : taille des blocs et stratégie restent internes à l'élément d'encre.

**DA6 — Entrées synthétiques par événements Qt postés, horodatées à l'instant prévu.**
Contexte : rejouer sans piloter la souris de Jérôme, hors écran en CI, identique sous Windows et macOS, sans sous-estimer la latence quand le fil principal est occupé.
Décision : périphérique synthétique (`QPointingDevice` stylet), `QTabletEvent` et `QWheelEvent` postés à la fenêtre ; latence mesurée depuis l'instant prévu t0 + i × 5 ms, retards d'injection et de réception enregistrés.
Conséquences : la pile d'entrée du système et la compression des événements de Qt ne sont pas mesurées (limite écrite dans le JSON et `docs/banc.md`).
Réversible : un injecteur natif (`SendInput`) ou un fil dédié (`QWindowSystemInterface`) peut s'ajouter derrière la même interface de scénario.

## 5. Scénarios d'acceptation

- **S1 — Banc D3D11 en release.** `pwsh -NoProfile -File ./scripts/bench.ps1` construit en release, ouvre la
  fenêtre en plein écran sur l'écran principal, joue d3d11 puis logiciel et écrit
  `bench/results/<horodatage>-<modèle>-d3d11.json`. Ce JSON contient `format = actcraft-bench/1`,
  `graphics_api = direct3d11`, `valid_for_measurement = true`, des options de compilation contenant `/Ob2`, les
  scénarios `stroke`, `zoom`, `scroll` avec chacun plus de 100 intervalles actifs, les statistiques d'intervalle,
  la latence en deux composantes (`stroke`), la mémoire du processus et la mémoire graphique, `content_check` ok
  sur les 3 témoins. La dernière ligne de console est `BUDGET TENU` ou `BUDGET DEPASSE` ; les deux sont acceptés
  (décision 38), le second avec un rapport chiffré.
- **S2 — Banc logiciel et captures.** Le JSON `<…>-software.json` du même passage a `graphics_api = software`,
  trois scénarios mesurés, un verdict marqué informatif et `content_check` ok ; dans `--captures`, les fichiers
  `software-mi-trait.png`, `software-fin-trait.png`, `software-zoom-800.png`, `software-retour-100.png` (et leurs
  équivalents `direct3d11-…`) montrent l'encre en cours au milieu du trait, puis les 300 objets et les traits
  ajoutés, net à 100 %.
- **S3 — Cache chiffré.** Deux passages D3D11, avec et sans `--sans-cache` : `cache` vaut `true` puis `false`
  dans les JSON ; `docs/mesures/2026-10-13-prototype-mesure.md` (sans nom d'hôte) donne côte à côte images/s, p95
  d'intervalle, latence p50 / p95 et mémoire, pour les deux rendus et les deux réglages du cache.
- **S4 — Fumée en CI.** `pwsh -NoProfile -File ./scripts/test.ps1` finit par `TESTS OK` ; `tst_bench_smoke`
  tourne sous son `TIMEOUT` et `tst_bench_result` vérifie `graphics_api = software`,
  `valid_for_measurement = false` et les trois scénarios ; l'empreinte du générateur est la même sous Windows et
  macOS ; la CI de la PR est verte, le pas macOS informatif rapporte son `graphics_api`.
- **S5 — Prototype rejoué sans souris.** `build/release/actcraft-bench.exe --scenario court --rendu d3d11
  --taille 1280x800 --captures .tmp/s5` puis `--rendu logiciel` : les captures montrent le trait pendant et après
  le geste, la page à 800 %, et le trait toujours présent au retour à 100 %. L'essai à la souris réelle
  (`build/release/actcraft.exe`) est proposé à Jérôme comme vérification humaine de fin, avec les captures ; il ne
  bloque pas le merge (`.claude/chantier.md:86`).

## 6. Hors périmètre

- Étape 2 : mesures sur Dell 3510 et MacBook M1, décision go / no-go et sa note au coffre.
- Mesure de latence bout à bout outillée (caméra rapide, capteur) ; mode de mesure au vrai stylet.
- Prédiction du stylet, encre à faible latence native (`IDCompositionInkTrailDevice`, `androidx.graphics.lowlatency`).
- Texte, sélection, gomme, autres outils ; annulation ; plusieurs pages ; enregistrement et lecture de fichiers
  (`.flipchart`, format natif).
- Registre `actions/`, menus, barres d'outils, raccourcis.
- Tactile, pincement, défilement cinétique, barres de défilement.
- Cache par tuiles ; netteté au-delà de 400 %.
- Linux, Android, iOS ; option `--rendu metal` explicite.
- Publication d'un paquet ou d'une release ; artefacts de banc dans la CI.
- Pilotage de la souris réelle par l'agent.

## 7. Portes immédiates

Aucune.
