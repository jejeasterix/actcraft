# BRIEF — ActCraft (réécriture C++ / Qt 6 Quick)

Document maître. À lire avant toute session de travail. Il reprend la passation du 2026-10-13
(`C:\Users\JeromeDenis\REA - Activinspire\docs\sessions\2026-10-13-handoff-reecriture-qt6.md`) avec les
décisions tranchées depuis.

## 1. Objectif

Réécrire complètement ActCraft, logiciel de tableau blanc interactif aujourd'hui en Python + PySide6, dans une
nouvelle stack. Exigences de Jérôme, dans l'ordre :

1. **Fluidité, priorité absolue.**
2. **Multiplateforme** : Windows et macOS d'abord ; Linux, Android et iOS ensuite.
3. **Open source** (licence MIT).
4. **Très modulaire** : chaque fonction peut être modifiée ou remplacée sans toucher aux autres.

L'application Python sert de référence de comportement, pas de modèle visuel imposé.

## 2. Décisions

| Sujet | Décision | Date |
|---|---|---|
| Langage | C++20 | 2026-10-13 |
| Interface | Qt 6 Quick (QML), rendu GPU par RHI | 2026-10-13 |
| Version de Qt | **6.12 LTS** (support jusqu'au 2031-09-30). Le plus vieux Mac du parc est un M1 : macOS 14.4 exigé par Qt 6.12 est atteignable. Qt 6.12 est la dernière version à supporter Windows 10 | 2026-10-13 |
| Rendu Windows | Direct3D 11 imposé (`--rendu d3d11`, défaut). C'est aussi le défaut de Qt | 2026-10-13 |
| Rendu macOS | Metal (défaut de Qt) | 2026-10-13 |
| Secours | rendu logiciel de Qt Quick (`--rendu logiciel`), testé à chaque passage des tests | 2026-10-13 |
| Construction | CMake + Ninja, presets `debug` et `release` | 2026-10-13 |
| Licence | **MIT** | 2026-10-13 |
| Dépôt | `C:\Users\JeromeDenis\ActCraft`, public sur GitHub (`jejeasterix/actcraft`) dès le premier commit | 2026-10-13 |

Écartés : Rust + wgpu/Slint/egui, Flutter, Web/Tauri (raisons dans le coffre, décision 0074).

## 3. Matériel cible et budget

| Système | Machine de référence (la plus faible) | Vérifié |
|---|---|---|
| Windows | Dell Inspiron 15 3510 : Celeron N4020 (2 cœurs), Intel UHD 600, 4 Go en un canal, écran 1366 × 768, sortie 1080p, Windows 11 | fiches Dell, 2026-10-13 |
| macOS | MacBook Air M1 | réponse de Jérôme, 2026-10-13 |

L'UHD 600 prend en charge Direct3D 12_1, donc Direct3D 11. Ses pilotes Intel sont « legacy » : correctifs de
sécurité seulement.

**Budget** sur les machines de référence : 60 images par seconde, latence d'encre sous 20 ms, mémoire tenant
dans 4 Go avec Windows et un navigateur ouverts.

## 4. Architecture

```
core/        modèle de document, pages, objets, pile d'annulation, géométrie, accrochage — sans QtGui ni QtQuick
formats/     lecture/écriture : .flipchart (lecture), format natif ouvert, PDF, images
actions/     registre central des commandes nommées ; menus, raccourcis et barres décrits par fichiers de données
input/       chaîne stylet : événements bruts → filtrage → prédiction → outil ; un module natif par système
render/      un moteur de rendu par type d'objet (nœuds du graphe de scène Qt Quick), choisi dans un registre
tools/       un outil = une extension autonome (stylo, gomme, formes, texte, sélection, grille…)
features/    règle, rapporteur, dés, horloge, rideau, spot… chacun activable ou retirable
ui/          interface QML (module ActCraft.Ui) : boîte à outils, navigateurs, menus, thèmes
app/         exécutable : choix du rendu, chargement de l'interface
bench/       banc de fluidité : temps par image, latence d'encre, mémoire
tests/       tests Qt Test, lancés par ctest (hors écran, rendu logiciel)
```

Chaque module est une bibliothèque statique `actcraft_<nom>` (alias `ActCraft::<nom>`), déclarée par
`actcraft_add_module` (`cmake/ActCraftModule.cmake`). L'ordre des `add_subdirectory` du `CMakeLists.txt` racine
est le sens des dépendances.

**Règles de modularité**
1. Tout passe par une **commande nommée** : bouton, menu ou raccourci déclenchent une action par son nom.
   Revoir une fonction = remplacer son action.
2. Un outil ne connaît que des interfaces (document, chaîne d'entrée, rendu) ; on le réécrit ou le retire sans
   casser les autres.
3. Menus et palettes dans des fichiers de données ; réorganiser l'interface ne demande pas de recompiler.
4. `core/` ne dépend d'aucune interface graphique. **Vérifié à la configuration** : `actcraft_forbid_links`
   fait échouer CMake si `actcraft_core` lie QtGui, QtQuick, QtQml ou QtWidgets.

**Règles de fluidité**
1. Le fil principal ne fait que l'interface ; fichiers, vignettes, import d'images, sauvegarde passent sur des
   fils secondaires.
2. Le contenu fixe d'une page est dessiné une fois dans une texture ; chaque image ne recalcule que ce qui
   bouge (trait en cours, sélection, objet déplacé).
3. L'encre est incrémentale : chaque point s'ajoute au tampon GPU du trait en cours, avec prédiction du stylet.
   Les traits terminés sont figés (leçon d'OpenBoard, ticket #1108 : traits jamais mis en cache, écriture de
   plus en plus lente).
4. Pas d'effets coûteux par image (flou, ombres calculées, transparences empilées) ; ombres d'interface en
   images.
5. Résolution de rendu adaptée à la sortie (4K et Retina : ne pas tout calculer en pleine résolution).
6. Le banc tourne à chaque modification ; une modification qui dépasse le budget du § 3 est refusée.

**Stylet (vérifié dans le source de Qt 6.8, 2026-10-13)** : sous Windows, Qt lit les messages `WM_POINTER`
natifs (pression, inclinaison, rotation). Les points intermédiaires arrivent tant que
`AA_CompressHighFrequencyEvents` **et** `AA_CompressTabletEvents` ne sont pas actifs tous les deux : ne pas les
activer. WinTab n'est pas utilisé par défaut.

## 5. Plan

**Étape 0 — Vérifications et outillage.** Fait le 2026-10-13 : recherches web (versions de Qt, Dell 3510,
rendu logiciel, stylet, encre à faible latence, OpenBoard), Qt 6.12 + Ninja + clang-format installés, dépôt,
modules vides, tests, CI GitHub (Windows + macOS).

**Étape 1 — Prototype de mesure.** Une fenêtre QML avec :
- une page de 1024 × 768 contenant 300 objets (formes, images, traits) ;
- le tracé au stylo incrémental, le zoom et le défilement ;
- le contenu fixe de la page mis en cache ;
- le banc (`bench/`) qui rejoue un tracé, un zoom et un défilement et écrit temps par image, latence et mémoire
  dans un fichier ; lancement en Direct3D 11 et en rendu logiciel.

**Étape 2 — Décision go / no-go.** Mesurer sur la machine de développement, puis donner à Jérôme la procédure
pour mesurer sur un Dell 3510 et un MacBook M1. Si le budget du § 3 tient : on s'engage. Sinon : rapport chiffré
et alternatives. Consigner la décision dans le coffre.

**Étape 3 — Cœur et formats.** Porter le lecteur `.flipchart` en C++ dans `formats/`. Oracle : exporter en JSON
ce que le lecteur Python lit dans chaque fichier de `samples/` (script à écrire dans l'ancien dépôt), puis exiger
que le lecteur C++ produise le même JSON. Format natif ouvert (archive zip + JSON + ressources).

**Étape 4 — Outils et fonctions, un module à la fois.** Demander d'abord à Jérôme la liste des fonctions à
revoir complètement. Pour chaque fonction : module, tests, passage au banc. Ordre proposé : sélection, stylo et
surligneur, gomme, formes, texte, navigateur de pages, annulation, grille, puis le reste dans l'ordre de
`docs/tests/plan-comparatif.md` de l'ancien dépôt.

## 6. Points ouverts

- **Encre à faible latence** (après l'étape 2, si la latence ne tient pas) : Android a
  `androidx.graphics.lowlatency` (`GLFrontBufferedRenderer`, API 29+), intégrable par un `SurfaceView` natif
  superposé (non documenté par Qt, à prototyper). Windows a `IDCompositionInkTrailDevice` (prérelease), mais Qt
  n'expose pas sa chaîne d'échange : difficile.
- **Liste des fonctions à revoir complètement** : à demander à Jérôme avant l'étape 4.
- **aqtinstall** : la version 3.3.0 ne sait pas installer Qt 6.12 ; la version de développement (dépôt GitHub)
  le sait. Revenir à une version publiée quand elle sortira.
