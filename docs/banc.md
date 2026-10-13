# Banc de fluidité — mode d'emploi

Le banc `actcraft-bench` charge la vraie interface d'ActCraft, rejoue un tracé, un zoom et un défilement par des
entrées synthétiques (aucune souris pilotée), et écrit temps par image, latence d'encre et mémoire dans un JSON.
Il sert l'étape 2 du BRIEF : décider go / no-go sur un Dell Inspiron 15 3510 et un MacBook Air M1.

Ce document est écrit pour être suivi tel quel, machine par machine. Un passage ne demande aucune saisie : on
lance la commande, on attend, on renvoie le JSON et la dernière ligne affichée.

## Avant tout passage

- Fermer toutes les autres fenêtres et applications (navigateur, messagerie, visioconférence, synchronisation de
  fichiers). Elles volent du processeur et fausseraient le résultat.
- Brancher le secteur. Sous Windows : mode d'alimentation « Performances optimales ».
- La fenêtre du banc couvre **l'écran principal en plein écran**, 30 à 60 s par rendu. Si plusieurs écrans sont
  branchés, le banc se place sur l'écran principal : c'est celui-là qui est mesuré.
- **Ne toucher ni à la souris, ni au clavier, ni au trackpad, ni au stylet pendant le passage.** Un clic ou un
  mouvement déplace le focus ou perturbe la fenêtre, et le passage devient invalide (`exposure-lost`) ou faux.
- Ne pas verrouiller la session ni laisser l'écran se mettre en veille pendant le passage.

## Commandes

| Besoin | Commande |
|---|---|
| Passage complet sous Windows (D3D11 puis logiciel, plein écran) | `pwsh -NoProfile -File ./scripts/bench.ps1` |
| Sans cache de la page fixe | `pwsh -NoProfile -File ./scripts/bench.ps1 -SansCache` |
| Avec captures | `pwsh -NoProfile -File ./scripts/bench.ps1 -Captures .tmp/captures` |
| Essai rapide sans fenêtre | `pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu logiciel -Scenario court -Taille 1280x800 -HorsEcran` |
| macOS (Metal puis logiciel) | `scripts/bench.sh` |
| Paquet pour une machine sans outils | `pwsh -NoProfile -File ./scripts/paquet-banc.ps1` |
| Résumé Markdown | `pwsh -NoProfile -File ./scripts/resume-banc.ps1 -Entrees bench/results -Sortie resume.md` |
| Exécutable seul | `actcraft-bench [--rendu d3d11\|logiciel\|auto] [--scenario complet\|court] [--sortie <fichier.json\|dossier>] [--sans-cache] [--taille plein-ecran\|LxH] [--captures <dossier>]` |

Options de `bench.ps1` : `-Rendu d3d11|logiciel|tous`, `-SansCache`, `-Captures <dossier>`,
`-Scenario complet|court`, `-Taille plein-ecran|LxH`, `-HorsEcran`, `-SansConstruire`. Options de `bench.sh` :
`--rendu auto|logiciel|tous`, `--sans-cache`, `--captures <dossier>`, `--scenario complet|court`,
`--taille plein-ecran|LxH`, `--hors-ecran`, `--sans-construire`.

Sans `--sortie`, le JSON va dans `resultats-banc/` à côté de l'exécutable ; les scripts écrivent dans
`bench/results/` (ignoré par git). Un passage complet dure environ 70 à 90 s pour les deux rendus.

## Ce qui est joué

Après « page prête » (images générées, premier cache dessiné) et 2 s de chauffe :

| Scénario | Contenu | Niveau imposé |
|---|---|---|
| `stroke` | 10 traits de 1,5 s à 200 points/s, pression variable, stylo levé 300 ms entre deux traits | 100 % |
| `zoom` | 100 → 800 → 25 → 100 % en 6 s, molette fine Ctrl à 120 Hz | 100 % |
| `scroll` | aller-retour vertical puis horizontal en 6 s | 400 %, page calée en haut à gauche |

Puis une séquence de démonstration non mesurée : un trait capturé à mi-parcours, à la fin, à 800 % et de retour à
100 %. Quatre contrôles de couleur (trois objets témoins, l'encre en cours) prouvent que le rendu dessine
vraiment ; `--captures` écrit les quatre images `<rendu>-<étape>.png`. `--scenario court` divise les durées par 5.

## Fichier de résultats

Un JSON par passage, `"format": "actcraft-bench/1"`, nommé `<AAAA-MM-JJ_HHMMSS>-<modèle>-<rendu>.json`. Il ne
contient ni nom d'hôte ni nom d'utilisateur : le modèle de la machine, le processeur, la puce graphique, l'écran
et le commit y figurent, rien qui identifie une personne. Clés principales :

- `graphics_api` (`direct3d11`, `metal`, `software`…), `cache`, `valid_for_measurement`, `within_budget`,
  `informative` ;
- `meta` : commit, Qt, système, processeur, modèle, puce graphique, écran et fréquence, rapport de pixels, boucle
  de rendu, type de construction, options de compilation, fenêtre, et `refresh` (période d'écran de référence et
  période mesurée) ;
- `scenarios` : statistiques d'intervalles, figements et reprises, latence, entrées, mémoire, séries brutes ;
- `content_check` : les contrôles de couleur ;
- `verdict` : `invalid_reasons` et `criteria` (valeur, seuil, réussi ou non, par scénario).

## Lire le verdict

Dernière ligne affichée et code de sortie :

| Ligne | Code | Sens |
|---|---|---|
| `BUDGET TENU` | 0 | Passage valide, tous les critères respectés. |
| `BUDGET DEPASSE` | 1 | Passage valide, au moins un critère hors seuil : c'est un **résultat**, pas une panne. |
| `MESURE INVALIDE` | 2 | Le passage ne prouve rien (ni tenu ni dépassé) : à refaire, ou à signaler. |

Les scripts sortent sur le verdict du rendu par défaut (Direct3D 11 sous Windows, Metal sous macOS) ; le rendu
logiciel est mesuré mais **informatif** : son verdict ne compte pas. Les scripts relancent une fois un passage
invalide.

### Seuils

| Critère | Seuil |
|---|---|
| Images/s (1000 / intervalle médian des images actives) | ≥ 58 |
| p95 de l'intervalle entre images actives | ≤ 18,5 ms |
| Part des intervalles > 25 ms | ≤ 1 % |
| Figements d'un trait et reprises après une pause (comptés à part) | aucun de 2,5 périodes d'écran ou plus |
| Latence interne d'encre, p50 | < 20 ms |
| Pic de mémoire privée + pic de mémoire graphique | ≤ 1 Gio |

Un intervalle n'est « actif » que si une entrée y était prévue depuis l'image précédente, au plus 2 périodes
d'écran après elle (instant prévu, pas instant d'envoi : un fil principal bloqué ne masque pas son retard). Les
pauses du stylo et l'attente des redessins ne comptent pas. Un figement est l'arrêt de l'encre entre deux
points d'un trait ; une reprise est la première image après une pause du stylo.

### Raisons d'un `MESURE INVALIDE`

La liste est dans le JSON (`verdict.invalid_reasons`) et dans la sortie du passage.

| Raison | Cause probable | Que faire |
|---|---|---|
| `build-not-release` | Exécutable compilé en debug | Utiliser le paquet ou `bench.ps1` / `bench.sh` (ils construisent en release). |
| `invalid-refresh-period` | L'écran ne présente pas à sa période : synchronisation verticale coupée, écran en veille, machine surchargée, ou images plus rapides que l'écran | Fermer les autres applications, vérifier le secteur, refaire ; si cela persiste, le signaler avec le JSON. |
| `exposure-lost` | La fenêtre a perdu l'affichage pendant un scénario (clic, notification, verrouillage, autre fenêtre devant) | Refaire sans toucher à rien et sans notification. |
| `latency-unmatched` | Des échantillons d'encre ne sont jamais apparus dans une image | Refaire ; si cela se répète, le signaler. |
| `release-not-synced` | Une fin de trait a été rattachée au trait suivant | Refaire ; si cela se répète, le signaler. |
| `gpu-memory-unavailable` | La mémoire graphique n'a pas pu être lue (rendu matériel seulement) | Le signaler avec le JSON : le pic de mémoire ne serait jugé que sur la mémoire privée. |
| `content-check-failed` | Le rendu n'a pas dessiné les couleurs attendues : le chiffre ne mesure pas la vraie interface | Relancer avec `--captures` et joindre les images. |

## Ce qu'il faut renvoyer

Pour chaque machine :

1. **Tous les fichiers JSON** du dossier de résultats (`resultats-banc\` pour le zip Windows, `bench/results/`
   sous macOS).
2. **La dernière ligne** affichée par chaque commande (`BUDGET TENU`, `BUDGET DEPASSE` ou `MESURE INVALIDE`).
3. Le dossier de captures si `--captures` a été utilisé.
4. La sortie des commandes de relevé du matériel (voir les procédures).

Le résumé se produit ensuite sur le poste de développement :
`pwsh -NoProfile -File ./scripts/resume-banc.ps1 -Entrees <dossier des JSON reçus> -Sortie docs/mesures/<date>-<sujet>.md`.
Il donne un tableau rendu × cache × scénario (images/s, p95, part > 25 ms, figements et reprises, latence p50 /
p95, pic de mémoire, verdict, validité) et la liste des fichiers avec leur matériel, leurs raisons d'invalidité et
leurs critères dépassés. Il ne publie jamais de nom d'hôte. Seuls les résumés commités dans `docs/mesures/` font
foi ; les JSON restent hors du dépôt.

## Limites connues

- **La latence est interne (N1-2).** Elle va de l'instant prévu de l'échantillon à la présentation
  (`frameSwapped`) de la première image qui le contient, en deux composantes (prévu → synchronisation,
  synchronisation → présentation). Elle ne contient ni le numériseur du stylet, ni la pile d'entrée du système,
  ni la composition (DWM sous Windows), ni le balayage de l'écran, qui ajoutent en général une à deux images (17
  à 33 ms à 60 Hz). Une latence interne < 20 ms ne prouve donc pas une latence perçue < 20 ms. Les entrées sont
  des événements Qt postés à la fenêtre : la compression des événements par Qt et le pilote du stylet ne sont
  pas mesurés.
- **Le rendu logiciel est informatif (N1-3).** Il sert de plancher de repli et de comparaison ; son verdict n'est
  jamais le verdict du passage.
- **Le chemin Metal n'a jamais été exécuté avant le MacBook M1 (N1-8).** Le premier passage sur le M1 peut
  révéler un défaut de la chaîne, pas seulement un chiffre : vérifier que `graphics_api` vaut `metal` dans le
  JSON. S'il vaut autre chose, le signaler avant toute lecture des chiffres.
- **Machine de développement ≠ machine de référence.** Seuls les passages sur le Dell 3510 et le M1 comptent
  pour le go / no-go.
- **Mesure bout à bout (non outillée).** Filmer à 240 images/s ou plus la pointe du stylet et l'encre, avec un
  trait rapide, puis compter les images entre le contact et l'apparition de l'encre. À faire à la main si le
  verdict interne est proche du seuil.

## Procédure Dell Inspiron 15 3510 (Windows 11, zip local)

Sur le poste de développement :

1. `pwsh -NoProfile -File ./scripts/paquet-banc.ps1` → `build/paquet-banc/actcraft-bench-<commit>.zip`.
2. Copier le zip sur une clé USB. Le paquet reste local : il n'est jamais publié.

Sur le Dell (secteur branché, mode d'alimentation « Performances optimales », toutes les autres applications
fermées ; écran externe 1080p si c'est l'usage visé, branché avant le lancement) :

1. Extraire le zip dans `C:\banc`.
2. Ouvrir PowerShell, puis : `cd C:\banc\actcraft-bench`.
3. Si SmartScreen bloque l'exécutable (non signé) : « Informations complémentaires » → « Exécuter quand même ».
4. Lancer les quatre passages l'un après l'autre, sans toucher à rien pendant chacun, et noter la dernière ligne
   de chacun :
   ```powershell
   .\actcraft-bench.exe
   .\actcraft-bench.exe --rendu logiciel
   .\actcraft-bench.exe --sans-cache
   .\actcraft-bench.exe --sans-cache --rendu logiciel
   ```
   Le premier donne le verdict de référence (Direct3D 11, avec cache). Les deux « sans cache » servent à chiffrer
   le gain du cache (S3).
5. Si un passage est `MESURE INVALIDE`, le refaire une fois en suivant la table des raisons ci-dessus.
6. Optionnel, pour comprendre un chiffre surprenant : `.\actcraft-bench.exe --captures captures`.
7. Relever le matériel :
   ```powershell
   Get-CimInstance Win32_ComputerSystem | Select-Object Manufacturer, Model, TotalPhysicalMemory
   Get-CimInstance Win32_Processor | Select-Object Name
   Get-CimInstance Win32_VideoController | Select-Object Name, DriverVersion, CurrentRefreshRate
   ```
8. Renvoyer le dossier `resultats-banc\` (JSON), les dernières lignes notées, le dossier `captures\` s'il existe,
   et la sortie de l'étape 7.

## Procédure MacBook Air M1 (macOS 14.4 ou plus, construction sur place)

1. Installer les outils : `xcode-select --install`, puis CMake et Ninja (`brew install cmake ninja`, ou les
   paquets officiels), et Python 3.12.
2. Installer Qt 6.12 :
   ```bash
   python3 -m pip install --user "git+https://github.com/miurahr/aqtinstall"
   python3 -m aqt install-qt mac desktop 6.12.0 clang_64 -m qtshadertools qtimageformats --outputdir ~/Qt
   export ACTCRAFT_QT_DIR=~/Qt/6.12.0/macos
   ```
3. Récupérer le code :
   ```bash
   git clone https://github.com/jejeasterix/actcraft.git && cd actcraft
   ```
4. Secteur branché, toutes les autres applications fermées. Lancer, sans toucher au trackpad ni au clavier
   pendant chaque passage, et noter la dernière ligne de chacun (le premier appel construit en release : compter
   quelques minutes de plus) :
   ```bash
   scripts/bench.sh
   scripts/bench.sh --sans-cache --sans-construire
   ```
   Le premier donne le verdict de référence (Metal, avec cache, puis logiciel). Le second chiffre le gain du
   cache.5. Vérifier que le JSON Metal porte bien `"graphics_api": "metal"` (N1-8) :
   ```bash
   grep '"graphics_api"' bench/results/*.json
   ```
6. Optionnel : `scripts/bench.sh --captures captures --sans-construire`.
7. Relever le matériel :
   ```bash
   sysctl -n hw.model machdep.cpu.brand_string hw.memsize
   system_profiler SPDisplaysDataType
   ```
8. Renvoyer le dossier `bench/results/` (JSON), les dernières lignes notées, le dossier `captures/` s'il existe,
   et la sortie de l'étape 7.
