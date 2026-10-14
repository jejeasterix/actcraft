# Cadrage — livraison complète (étapes 3 et 4 du BRIEF, installateur Windows, APK Android)

Cadrage autonome (étage 1 de `nouvelle-feature`), 2026-10-14. Jérôme ne répond à aucune question avant la fin :
chaque point est tranché depuis les sources, ou par un défaut prudent marqué « à revoir ».
Sources : `docs/BRIEF.md`, `CLAUDE.md`, `.claude/chantier.md`, cadrage `docs/specs/2026-10-13-prototype-mesure-cadrage.md`,
mesure `docs/mesures/2026-10-13-prototype-mesure.md`, coffre (décisions 0069, 0074 à 0076, 0079 à 0086, journal
`2026-10-14-actcraft-prototype-mesure-clos`, piège `aqtinstall-3-3-ne-sait-pas-installer-qt-6-12`), code de `main` @ 175b336
(graphe du dépôt), ancien dépôt `REA - Activinspire` (`docs/spec/fonctionnalites.md`, `docs/spec/backlog.md`,
`docs/tests/plan-comparatif.md`, `docs/re/format-flipchart.md`, `app/actcraft/`), poste de développement (Qt 6.12
`msvc2022_64` installé, aucun SDK Android, JRE 21 seul, 104 Go libres).

## 1. Demande et gabarit

**Demande.** Livrer ActCraft utilisable : lecteur `.flipchart` et format natif (étape 3), interface complète et
toutes les fonctions retenues, un module à la fois (étape 4), avec un installateur Windows `.exe` et un `.apk`
Android, en autonomie (Jérôme, 2026-10-14 : « Ajoute la release .exe et la release .apk »).

**Gabarit.** `lots` (plus de 20 tâches), budget de 40 questions : **37 tranchées**, aucune ouverte. Un cadrage, un
plan par lot, lots exécutés l'un après l'autre, une PR par lot depuis `feat/livraison-complete` remise à jour sur
`main` après chaque merge.

**Lots et estimation** (détail : décisions 3 à 12).

| Lot | Titre | Livrable visible par Jérôme | Version | Tâches |
|---|---|---|---|---|
| 1 | Socle ouvrable et paquets | installateur local + APK : une interface, le stylo en couleurs, plusieurs pages | 0.1.0 | ≈ 15 |
| 2 | Document, format natif, lecteur `.flipchart` | ouvrir un paperboard de `samples/`, enregistrer en `.actcraft`, rouvrir | 0.2.0 | ≈ 16 |
| 3 | Cache incrémental (H-mes-1) | rapport de mesure : plus aucun figement hors limite | 0.2.1 | ≈ 4 |
| 4 | Outils I : sélection, stylo, marqueur, gomme, annulation, gestes | écrire, gommer, sélectionner, annuler ; pincer pour zoomer | 0.3.0 | ≈ 12 |
| 5 | Outils II : formes, texte, pages, grille | formes, texte, navigateur de pages, grille aimantée | 0.4.0 | ≈ 13 |
| 6 | Édition et fichiers | copier-coller entre documents, grouper, aligner, PDF, images | 0.5.0 | ≈ 15 |
| 7 | Navigateurs, propriétés, profils | navigateurs d'objet / propriétés / notes / ressources, réglages gardés | 0.6.0 | ≈ 15 |
| 8 | Présentation et outils | rideau, spot, zoom de page, outils de maths, horloge, bureau | 0.7.0 | ≈ 14 |
| 9 | Interactivité | actions, conteneurs, limiteurs, liens, encre révélatrice | 0.8.0 | ≈ 11 |
| 10 | Tablette, finitions, version 1.0 | APK confortable sur tablette, notes de version, release prête | 1.0.0 | ≈ 8 |

Total ≈ 123 tâches. Chaque lot se clôt par la double relecture, le banc (DA7), les captures (DA8), l'installateur
local et l'APK, et une porte de fin où l'orchestrateur propose la release du lot (publiée seulement sur go écrit).

## 2. Décisions

| # | Point | Décision | Source | Réversible ? |
|---|---|---|---|---|
| 1 | Sens de « livraison complète » | La référence de comportement est ce que fait l'application Python : les 26 lignes du plan comparatif (toutes ✅ ou ➖). La spécification compte **200 exigences** (197 en tête de paragraphe, plus F-200 à F-202 écrites en liste) : **174 sont incluses**, chacune affectée à un lot ci-dessous, dont certaines réduites ou réservées aux postes de bureau (F-090, F-655, F-760, F-930 en partie) ; **26 sont exclues**, liste et raisons en § 6. Plateformes livrées : Windows (installateur) et Android (APK) ; macOS reste construit et testé par la CI. | `docs/BRIEF.md:116-119` ; `REA - Activinspire/docs/tests/plan-comparatif.md:6-33` ; `docs/spec/fonctionnalites.md` ; demande du 2026-10-14 | oui |
| 2 | Fonctions à revoir complètement | Aucune : chaque fonction reproduit le comportement de référence. Toute fonction passe par une commande nommée et un outil remplaçable, donc la revoir plus tard reste local. Hypothèse N1-1. | `docs/BRIEF.md:116,127` ; consigne de l'orchestrateur ; `docs/BRIEF.md:69-72` | oui |
| 3 | Découpage | 10 lots séquentiels (tableau du § 1). Livrer tôt : le lot 1 donne une interface ouvrable **et** les deux paquets, ce qui purge d'abord le risque le plus inconnu (chaîne Android). Le modèle de document (lot 2) précède les outils (lots 4 et suivants). | demande de Jérôme (« aucune interface ») ; méthode (modèle avant affichage) | oui |
| 4 | Lot 1 — Socle ouvrable et paquets | Registre de commandes et fichiers de données (DA1) ; coquille QML : ligne supérieure (menus Fichier, Éditer, Affichage, Insérer, Outils, Aide ; indicateur « page n / N » ; échelle ; plein écran), palette d'outils à droite, zone de page avec barres de défilement, pas de barre d'état ; thème et icônes à nous (décision 22) ; stylo branché : 16 pastilles, 4 épaisseurs + réglette, indicateur de largeur ; pages multiples en mémoire (suivante crée une page à la fin, précédente) ; F5 ; commande `--version` et boîte « À propos » avec notices (décision 33) ; banc adapté à la nouvelle interface et nouvelle référence mesurée (DA7) ; outil de captures (DA8) ; installateur Windows (DA9) ; chaîne Android avec APK de débogage et essai sur émulateur (DA9) ; première tâche : mise à jour du profil (décision 37). F-001, F-002, F-003, F-020, F-021, F-022, F-060, F-120, F-121 (boutons des fonctions livrées), F-122, F-125, F-128, F-133, F-134 (stylo), F-137 (stylo), F-138, F-200, F-220, F-630 (suivante, précédente), F-720 (partie), F-760 (« À propos », aide = page du projet). | décisions 21-23, DA1, DA7-DA9 | oui |
| 5 | Lot 2 — Document, format natif, lecteur `.flipchart` | Modèle de document et pile d'annulation dans `core/` (DA2) ; format natif `.actcraft` (DA3) ; lecteur `.flipchart` et son oracle (DA4) ; rendu des nouveaux types (texte, image décodée sur fil secondaire, ligne, fond couleur / dégradé / image, grille à niveaux, quatre couches, ordre couche puis profondeur puis arbre) ; peintre partagé (DA5) ; Fichier ▸ Nouveau (5 tailles), Ouvrir, Enregistrer, Enregistrer sous, Fermer, Quitter avec demande d'enregistrement ; avertissements de lecture affichés sans bloquer. Un `.flipchart` ouvert s'enregistre en `.actcraft`. F-500, F-510, F-511, F-512 (données), F-574, F-583 (données), F-602, F-610 (rendu), F-621 (rendu), F-655 (`.flipchart` en lecture), F-656, F-657, F-700 (partie). | `docs/BRIEF.md:112-114` ; coffre 0069 ; `docs/re/format-flipchart.md` § 1-7 | oui |
| 6 | Lot 3 — Cache incrémental | Chantier dédié à H-mes-1 : supprimer le figement au versement d'un objet dans le cache (DA11), mesuré par le banc en D3D11 plein écran, avec un scénario de pauses longues ajouté pour ne pas tromper la mesure. Placé après le lot 2 (couches réelles) et avant les outils, qui versent tous au cache. Hypothèse N1-10 (Jérôme voulait ce chantier après la mesure du Dell). | journal du coffre `2026-10-14-actcraft-prototype-mesure-clos` (porte de fin) ; `docs/mesures/2026-10-13-prototype-mesure.md` ; consigne de l'orchestrateur | oui |
| 7 | Lot 4 — Outils I | Sélection (clic, rectangle qui touche, déplacer, poignées, redimensionner, rotation, barre d'objet), stylo et marqueur (mémoire par outil, couleur et épaisseur, translucidité du marqueur), gomme (coupe et scinde les traits, n'efface que les annotations), Annuler / Rétablir, Supprimer, corbeille, Effacer ▸ et Rétablir la page ; gestes à deux doigts (décision 24). F-050, F-130, F-131, F-132, F-136, F-137, F-201, F-202 (partie), F-210, F-225, F-230, F-520, F-521, F-522, F-523, F-524, F-528, F-541, F-951-F-953 (fonctions du lot). | `docs/BRIEF.md:117-118` (ordre) ; `plan-comparatif.md:14-16,21-23` | oui |
| 8 | Lot 5 — Outils II | Formes (palette à nous, tracé depuis le coin ou le centre, points intelligents), texte (création, édition, barre de mise en forme, décision 36), panneau des navigateurs avec le navigateur de pages (miniatures à jour, sélection multiple, réordonner, insérer, couper / copier / coller des pages), menu de page, grille (créateur à niveaux, accrochage, masquer). F-040, F-041, F-042, F-100, F-240, F-241, F-242, F-250, F-251, F-252, F-253, F-600, F-601, F-620, F-621, F-622, F-623, F-771, F-800, F-801, F-802, F-803. | `docs/BRIEF.md:117-118` ; `plan-comparatif.md:18,20,24,33` | oui |
| 9 | Lot 6 — Édition et fichiers | Couper / copier / coller / dupliquer, grouper, organiser et couches, transformer et aligner, menu d'édition d'objet, points de forme, pot de peinture, connecteur, modification de stylo, verrouiller / masquer / glisser une copie, onglets de documents (plusieurs ouverts, pages glissées d'un onglet à l'autre), insérer une image, export de page en image, export PDF, impression réduite au PDF (N1-15), import PDF. F-010, F-011, F-012, F-129, F-221, F-235, F-245, F-513, F-525, F-526, F-527, F-535, F-536, F-537, F-540, F-585, F-586, F-587, F-660, F-664, F-670 (réduit), F-671, F-672, F-710, F-740 (partie), F-770. | `plan-comparatif.md:17,19,22,25,29` | oui |
| 10 | Lot 7 — Navigateurs, propriétés, profils | Navigateurs d'objet, de propriétés (sections F-570 à F-583), de notes, de ressources (dossiers locaux, ajout d'un objet à la bibliothèque) ; palette ancrable, flottante, réduite, épinglée, taille des icônes ; barre de raccourcis ; profils (les nôtres, N2-6), réglages et persistance ; mode Création ; tableau de bord d'accueil ; documents récents et résumé ; étiquettes. F-030 à F-034, F-090 (sans les entrées exclues), F-123, F-135, F-140, F-141, F-560, F-570 à F-573, F-575, F-576, F-581, F-810, F-811, F-812, F-813, F-820, F-772, F-821, F-830, F-840, F-880, F-881, F-882, F-900, F-910, F-930 (sections des fonctions incluses), F-935. | `plan-comparatif.md:13,28-30` | oui |
| 11 | Lot 8 — Présentation et outils | Zoom de page, rideau et spot (et leurs réglages par page), effets de changement de page réduits (N2-10), outils de maths (règle, rapporteur, compas, équerre, origine XY), dés, calculatrice, horloge et compte à rebours, date et heure, clavier à l'écran (bureau), reconnaissance de forme (algorithme à nous), caméra (bureau), annotation de bureau et outils de bureau (bureau). F-070, F-071, F-080, F-124, F-260, F-265, F-270, F-275, F-280 à F-286, F-290, F-294, F-296, F-300, F-591, F-592, F-593, F-640 (réduit), F-750 (entrées incluses). | `plan-comparatif.md:26-27,31` | oui |
| 12 | Lot 9 — Interactivité | Navigateur d'action et actions (commande, page, objet, glisser-déposer ; sans les actions de vote), objets d'action, conteneurs, rotation et limiteurs, liens vers un fichier ou un site, médias ouverts par l'application du système (N2-9), encre révélatrice (encre magique de la référence). F-255, F-550, F-551, F-552 (réduit), F-577, F-578, F-579, F-580, F-582, F-850, F-851, F-890, F-891 (sans vote), F-892 (sans vote). | `plan-comparatif.md:28` ; `backlog.md` lot 3 | oui |
| 13 | Lot 10 — Tablette, finitions, 1.0 | Mise en page étroite (navigateurs masqués, palette repliable), dialogues de fichiers Android, essais sur émulateur et sur tablette réelle si Jérôme en fournit une (N1-11), passe de fluidité finale, raccourcis restants (F-950 à F-953 des fonctions incluses), `CHANGELOG.md` et guide de démarrage dans `README.md`, flux de release complet en essai à blanc (décision 31). | demande du 2026-10-14 ; `docs/BRIEF.md:13` | oui |
| 14 | Modèle de document | Dans `core/` (sans QtGui, contrôlé par CMake) : document, pages (taille, fond, grille, notes, propriétés), quatre couches (fond, inférieure, intermédiaire, supérieure), objets à identifiant stable (annotation stylo / marqueur / révélatrice, forme à polygones, ligne, texte en HTML riche + mise en forme, image et média par ressource, groupe par références, connecteur, action), transformation affine 2D, propriétés communes (nom, visible, verrouillé, translucidité, restrictions). Unités : pixels de page (1024 × 768 par défaut), comme les `.flipchart`. Voir DA2. | `docs/BRIEF.md:51,74-75` ; `docs/re/format-flipchart.md` § 6-7 ; `fonctionnalites.md` § 7 | oui |
| 15 | Annulation | Pile d'annulation propre dans `core/` (`QUndoStack` est dans QtGui, interdit à `core/`) : une commande par modification, fusion des glissements continus, par document, 500 étapes au plus (N2-1). Toute modification du document passe par elle. | `docs/BRIEF.md:51` ; `fonctionnalites.md` F-132, F-840 | oui |
| 16 | Format natif | `.actcraft` : archive zip (miniz, MIT, dans `third_party/`) contenant `document.json` (`"format": "actcraft-document/1"`, clés en anglais), `resources/<sha256>.<ext>` (octets d'origine), `thumbnails/page-<n>.png`. Écriture sur fil secondaire dans un fichier temporaire puis renommage. Lecture et écriture testées par aller-retour. Extension : N1-3. Voir DA3. | `docs/BRIEF.md:52,78-79,114` ; coffre 0069 (format ouvert zip + JSON du clone Python) | oui avant la première release, non ensuite |
| 17 | Lecteur `.flipchart` | Dans `formats/` (QtCore seulement : `qUncompress` pour le conteneur), lecture seule, d'après notre documentation `docs/re/format-flipchart.md`. Les tables d'identifiants sont **écrites à la main** pour les seuls types et propriétés lus, jamais copiées de `_schema.py` (généré depuis le binaire). Un `.flipchart` ne se réécrit pas : Enregistrer propose `.actcraft`. Voir DA4. | `CLAUDE.md:65-69` ; coffre 0069 et 0076 ; `REA - Activinspire/app/actcraft/io/_schema.py:1-7` | oui |
| 18 | Oracle du lecteur | Script dans l'ancien dépôt qui exporte en JSON canonique (clés et arrondis fixés par nous) ce que lit le lecteur Python pour chacun des 17 paperboards ; outil C++ `actcraft-flipchart-dump` qui produit le même JSON ; test comparatif lancé **localement** seulement (variable `ACTCRAFT_SAMPLES_DIR`), sans aucun fichier issu de `samples/` dans le dépôt. En CI : paperboards synthétiques produits par un écrivain de test écrit par nous d'après notre documentation. | `docs/BRIEF.md:112-114` ; `CLAUDE.md:62-68` (samples jamais modifiés, dépôt public) | oui |
| 19 | Commandes nommées | Registre dans `actions/` (QtCore) : identifiant anglais à points (`page.next`, `tool.pen`), libellé, icône, raccourci par défaut, cochable, état actif, plateformes. Menus, palette, barre d'objet et raccourcis décrits en JSON, lus d'abord dans le dossier du profil puis dans les ressources intégrées : réorganiser sans recompiler. Voir DA1. | `docs/BRIEF.md:53,69-73` | oui |
| 20 | Outils et fonctions | Un outil implémente une interface `Tool` (activation, échantillons d'entrée, clavier, surcouche) et s'inscrit dans un registre de `tools/` ; une fonction de `features/` s'inscrit dans le sien et se retire par une option CMake (`ACTCRAFT_FEATURES`). Les outils ne voient que des interfaces (document, encre, vue). Voir DA6. | `docs/BRIEF.md:56-57,71-72` ; `tools/include/actcraft/tools/pentool.h:10-27` | oui |
| 21 | Interface QML | Qt Quick Controls, style `Basic` personnalisé (le plus léger), `QtQuick.Dialogs` pour fichiers et couleurs, éléments C++ pour la page. Disposition de la référence (menus en haut, palette à droite, navigateurs à gauche, corbeille sous la palette, pas de barre d'état), connue des enseignants ; aspect à nous. Textes en français écrits par nous (`qsTr`), vocabulaire courant, sans les termes propres à Promethean (N1-12). | `docs/BRIEF.md:2-11,58` ; `CLAUDE.md:65-68` ; `fonctionnalites.md` F-001 à F-003 ; modules présents dans `C:\Qt\6.12.0\msvc2022_64\bin` (QuickControls2, QuickDialogs2, Svg) | oui |
| 22 | Design visuel et icônes | Direction posée par le skill `impeccable` en première tâche de design : jetons (couleurs, espacements, typographie) dans un singleton QML `Theme`, jeu d'icônes SVG **neuf** dessiné par nous sur une grille de 24 px (pas les icônes du clone Python, qui imitent l'original). Ombres en images (règle 4). Choix signalé à la porte visuelle de fin de lot 1 (N1-13). | `.claude/chantier.md:79-80` ; `plan-comparatif.md:32` ; `docs/BRIEF.md:85-86` | oui |
| 23 | Peintre partagé | Les chemins `QPainter` des moteurs de rendu (déjà requis par le rendu logiciel) servent aussi aux miniatures, à l'export d'image et de PDF, sur fils secondaires. Voir DA5. | coffre 0080 ; `docs/BRIEF.md:78-79` | oui |
| 24 | Tactile | Un doigt agit comme l'outil courant (comme la souris) ; deux doigts font défiler et zoomer (pincement) ; le stylet garde la pression. Même règle sous Windows (écrans et tableaux tactiles) et Android. Hypothèse N1-11. | prototype N2-12 (tactile hors périmètre jusque-là) ; `fonctionnalites.md` F-930 (« Gestion de la fonction tactile », défaut non documenté) | oui |
| 25 | Installateur Windows | Inno Setup 6, installation **par utilisateur sans droits d'administrateur** par défaut (choix « pour tous » proposé), dossier `%LOCALAPPDATA%\Programs\ActCraft`, menu Démarrer, désinstallation, installation silencieuse possible ; contenu : preset `release` (la construction mesurée) passé par `windeployqt --release --no-compiler-runtime`, runtime MSVC copié à côté de l'exécutable, licences ; association `.actcraft` pour l'utilisateur, `.flipchart` proposée décochée. Nom `ActCraft-<version>-setup.exe`. Non signé (N1-4). | prototype décision 36 (runtime local, `vc_redist` exige l'administrateur) ; `scripts/paquet-banc.ps1` | oui |
| 26 | Chaîne Android | Qt 6.12 pour Android installé par aqtinstall de développement (hôte Qt de bureau de même version), JDK 17, SDK et NDK à la version exigée par Qt 6.12 (relevée en première tâche Android, non vérifiée ici). La CI (Ubuntu) est la référence de construction ; l'agent installe aussi la chaîne sur le poste (dans `~/.tools` et `C:\Qt`, environ 10 Go) pour déboguer vite. Presets `android-arm64` et `android-x86_64` ; banc et tests exclus de ces constructions (`ACTCRAFT_BUILD_BENCH=OFF`, `ACTCRAFT_BUILD_TESTS=OFF`). | piège `aqtinstall-3-3-ne-sait-pas-installer-qt-6-12` ; `CMakeLists.txt:24-25` ; `.github/workflows/ci.yml` | oui |
| 27 | Cible Android | APK `arm64-v8a` pour la release ; `x86_64` pour l'émulateur de la CI. API minimale = défaut de Qt 6.12 (relevé en première tâche). Identifiant `io.github.jejeasterix.actcraft` (espace de noms que Jérôme possède). Aucune permission demandée (fichiers par le sélecteur du système, pas de réseau). Hypothèse N1-5. | défaut prudent | non après la première release (identifiant) |
| 28 | Rendu Android et code de plateforme | Android : rendu par défaut de Qt (RHI, OpenGL ES), pas d'option `--rendu`. Le code propre à Windows reste dans `app/renderbackend.cpp` et `bench/` (`gpumemory_win.cpp`, mémoire, machine). Chaque commande déclare ses plateformes : caméra, annotation et outils de bureau, clavier à l'écran ne s'affichent pas sous Android. Voir DA10. | `app/renderbackend.cpp:8-12` ; `bench/src/gpumemory_win.cpp` | oui |
| 29 | Signature et forme du paquet Android | `.apk` seulement (pas de `.aab`, pas de Play Store, qui coûte un compte payant). APK de la CI signés avec la clé de débogage. La clé de release n'est créée qu'au go de la première release : keystore hors dépôt, mot de passe et keystore en secrets GitHub, copie de sauvegarde remise à Jérôme. Hypothèse N1-6. | demande (« .apk ») ; méthode (argent → défaut prudent) | non (perdre la clé empêche les mises à jour) |
| 30 | Versions | SemVer depuis `project(VERSION)` de `CMakeLists.txt` : 0.1.0 au lot 1, mineure + 1 par lot, 1.0.0 au lot 10 ; `versionCode` Android = majeur × 10000 + mineur × 100 + correctif. `CHANGELOG.md` en français, une section par version, rédigé à la clôture de chaque lot. | `CMakeLists.txt:3-6` | oui |
| 31 | Releases | Workflow `release.yml` déclenché par un tag `v<version>` : construit l'installateur et l'APK signé, crée une release GitHub **brouillon** avec les deux fichiers, les sommes SHA-256 et la section du `CHANGELOG.md`. Le tag et la publication sont l'action finale : jamais sans go écrit de Jérôme. Un essai à blanc (`workflow_dispatch`, `dry-run`) produit tout sans créer de release. | `.claude/chantier.md:97-106` (action finale) | oui |
| 32 | Distribution avant le go / no-go | Construire et vérifier les paquets à chaque PR **sans publier d'artefact** ; l'installateur et l'APK de chaque lot sont remis à Jérôme localement (`build/paquet/`) à la porte de fin ; la release est proposée à chaque porte. Conflit de même rang entre le profil (« pas de distribution avant le go / no-go », 2026-10-13) et la demande du 2026-10-14 : la plus récente gagne, à revoir (N1-7). | `.claude/chantier.md:105` ; demande du 2026-10-14 ; prototype N1-4 | oui |
| 33 | Licences | Code MIT ; Qt en LGPLv3, lié dynamiquement (DLL sous Windows, `.so` dans l'APK) ; aucun module Qt sous GPL seule (ni Qt Virtual Keyboard : clavier à l'écran écrit par nous, ni Qt Charts) ; fichier `THIRD_PARTY_NOTICES.md` (Qt, ses dépendances, miniz) livré dans l'installateur et l'APK et affiché par « À propos » ; un script compare les bibliothèques Qt déployées à une liste autorisée. Hypothèse N1-8. | coffre 0076 (« Validation juridique : non faite ») ; `docs/BRIEF.md:14` | oui |
| 34 | Banc pendant le chantier | Le régime de la décision 38 du prototype continue (verdict = résultat, pas de refus sur le budget absolu) tant que l'étape 2 n'est pas faite. S'y ajoute une **porte de non-régression par lot** sur la machine de développement (DA7). Hypothèse N1-9. | coffre 0085 (« cesse d'elle-même à la décision go / no-go ») ; `.claude/chantier.md:44-68` | oui |
| 35 | Preuve visuelle | Outil `actcraft-captures` et `scripts/captures.ps1` : scénarios d'interface nommés, joués par événements injectés, captures PNG et contrôles de pixels ; hors écran en rendu logiciel pour chaque tâche d'interface, à l'écran en D3D11 en fin de lot sous le jeton `ecran`. Voir DA8. | `.claude/chantier.md:38-42` ; coffre 0084 (injection) | oui |
| 36 | Texte | Contenu en HTML riche (`QTextDocument`), dessiné par le peintre partagé dans une texture pour le rendu RHI (redessinée après un zoom, comme le cache) ; édition dans un élément d'édition QML pendant la frappe. Police par défaut des nouveaux textes : N2-3. Les `.flipchart` recalent la taille sur la largeur des runs, comme la référence. | `docs/re/format-flipchart.md` § 7.5 ; `plan-comparatif.md:20` | oui |
| 37 | Profil du chantier | Le cadreur ne modifie pas `.claude/chantier.md`. **Première tâche du plan du lot 1** : y écrire le régime du banc (décision 34), la commande de preuve visuelle (décision 35) et, quand ils existent, les scripts de paquets ; une tâche de fin de lot 1 y ajoute les commandes réelles. | `.claude/chantier.md:3-4` ; contrat du cadreur ; précédent : prototype décision 35 | oui |

## 3. Hypothèses à revoir

### Niveau 1 — difficiles à défaire ou visibles des utilisateurs

- **N1-1 — Aucune fonction à revoir.** Jérôme n'a pas donné la liste (`docs/BRIEF.md:127`) ; tout reproduit la
  référence. Revoir une fonction plus tard = remplacer sa commande et son outil.
- **N1-2 — 26 exigences exclues** (§ 6) et quatre réduites : impression → PDF (N1-15), médias → application du
  système (N2-9), effets de changement de page réduits (N2-10), fonctions de bureau absentes d'Android.
- **N1-3 — Extension `.actcraft`** (nouvelle, sans collision connue) ; l'association `.flipchart` est proposée
  décochée pour ne pas prendre la place d'ActivInspire installé sur le même poste. Une fois des fichiers distribués,
  changer d'extension coûte une migration.
- **N1-4 — Installateur non signé.** Windows affiche l'avertissement SmartScreen (« Windows a protégé votre
  ordinateur ») ; un certificat ou un service de signature est une dépense qui revient à Jérôme. Installation par
  utilisateur sans droits d'administrateur, pensée pour les postes d'école.
- **N1-5 — Cible Android.** `arm64-v8a` seulement (les tablettes 32 bits ne l'installeront pas) ; identifiant
  `io.github.jejeasterix.actcraft`, définitif après la première release ; API minimale imposée par Qt 6.12.
- **N1-6 — Clé de signature de l'APK.** Créée au go de la première release ; si elle est perdue, les utilisateurs
  devront désinstaller pour mettre à jour. Jérôme garde une copie hors ligne.
- **N1-7 — Distribution avant la décision go / no-go.** La demande du 2026-10-14 l'emporte sur la règle du profil
  (2026-10-13) ; publication toujours soumise au go écrit.
- **N1-8 — Licences non validées juridiquement.** LGPL de Qt respectée par liaison dynamique et notices ; pas d'avis
  d'un juriste.
- **N1-9 — Fluidité jugée sur la machine de développement.** La porte de non-régression compare au prototype mesuré
  sur le Latitude 5450, pas sur le Dell 3510 ni le M1 ; Android n'a pas de machine de référence (`docs/BRIEF.md` § 3)
  et n'est pas mesuré par le banc.
- **N1-10 — H-mes-1 traité avant la mesure du Dell.** Le journal de la porte de fin dit « après la mesure sur le
  Dell 3510 » ; l'orchestrateur demande de l'inclure. Le lot 3 se juge sur la machine de développement ; le Dell le
  revérifiera à l'étape 2.
- **N1-11 — Tactile.** Un doigt = outil courant, deux doigts = défilement et pincement. Sans tablette Android réelle,
  seul l'émulateur est essayé (le stylet Android n'est pas vérifié).
- **N1-12 — Vocabulaire à nous.** Libellés écrits par nous ; termes génériques gardés (« Stylo », « Gomme »,
  « Paperboard » au sens courant) ; termes et marques propres à Promethean remplacés (ex. « Encre magique » →
  « Encre révélatrice », pas d'« ActivInspire », d'« ExpressPoll », de « Promethean Planet »).
- **N1-13 — Aspect visuel et icônes neufs.** Disposition de la référence, dessin à nous (impeccable) ; les
  enseignants habitués aux icônes d'origine devront les réapprendre.
- **N1-14 — Fidélité `.flipchart`.** Lecture seule ; les objets sans exemplaire dans `samples/` (connecteurs, lignes,
  fonds image, MathML, texte vertical) sont lus sans validation (`docs/re/format-flipchart.md` § 8) ; les médias
  s'affichent par leur image d'espace réservé.
- **N1-15 — Impression par PDF.** `QPrinter` exige QtWidgets et n'existe pas sous Android : « Imprimer » produit un
  PDF ouvert par l'application du système. La boîte d'impression de la référence n'est pas reproduite.

### Niveau 2 — les autres

- **N2-1** Pile d'annulation limitée à 500 étapes par document.
- **N2-2** Interface en français seulement (`qsTr` en place pour une traduction future).
- **N2-3** Police des nouveaux textes : police sans empattement du système, 14 pt (la référence dit Arial 14,
  absent d'Android) ; les `.flipchart` gardent leurs polices, avec repli.
- **N2-4** Dossier par défaut : `Documents/ActCraft` (Windows) ; dossier de l'application (Android).
- **N2-5** Fichiers de disposition (menus, palette, raccourcis) cherchés d'abord dans le dossier du profil.
- **N2-6** Profils fournis à nous (noms et contenus), pas les cinq profils de la référence.
- **N2-7** Les PDB ne vont pas dans l'installateur ; ils restent dans la construction.
- **N2-8** Chaîne Android aussi installée sur le poste ; la CI fait foi.
- **N2-9** Sons et vidéos des documents ouverts par l'application du système (pas de Qt Multimedia).
- **N2-10** Effets de changement de page : fondu et glissement seulement (règle 4 de fluidité).
- **N2-11** Pas de lecture des `.flipx` du clone Python.
- **N2-12** Émulateur Android de la CI sur KVM (Ubuntu) ; non vérifié ici, repli : émulateur local.
- **N2-13** Pas de paquet macOS (pas demandé ; la signature Apple est payante) ; la CI macOS continue.
- **N2-14** Pas d'enregistrement automatique ni de reprise après plantage (absents de la référence).
- **N2-15** Import PDF par le module Qt PDF (à installer : absent de `C:\Qt\6.12.0\msvc2022_64`) ; sous Android
  seulement s'il est disponible pour cette cible.

## 4. Décisions d'architecture

**DA1 — Commandes nommées décrites par des données.**
Contexte : règles 1 et 3 du BRIEF (`docs/BRIEF.md:69-73`) ; `actions/` est vide (`actions/include/actcraft/actions/actions.h`).
Décision : registre `CommandRegistry` (QtCore) d'identifiants anglais, état actif / coché / plateformes ; menus, palette, barre d'objet, raccourcis en JSON (profil d'abord, ressources ensuite) ; `ui/` expose un modèle QML.
Conséquences : chaque bouton déclenche une commande par son nom ; tests du registre et des fichiers sans interface ; une commande absente d'une plateforme disparaît des menus.
Réversible : le format des fichiers est versionné ; un autre chargeur peut lire le même registre.

**DA2 — Modèle de document et annulation dans `core/`.**
Contexte : le modèle actuel (`core/include/actcraft/core/model.h`) ne connaît que trois types du prototype ; `core/` est interdit de QtGui.
Décision : document → pages → quatre couches → objets à identifiant stable et transformation affine, ressources par empreinte ; modifications seulement par commandes de la pile d'annulation de `core/`.
Conséquences : formats, outils et rendu partagent un seul modèle testable sans écran ; le générateur de 300 objets du banc est porté sur le nouveau modèle (empreinte recalculée).
Réversible : les types d'objets sont des variantes ajoutables ; la pile reste interne au document.

**DA3 — Format natif `.actcraft` : zip + JSON + ressources.**
Contexte : `docs/BRIEF.md:52,114` ; Qt n'a pas d'API zip publique (`QZipReader` est privé, QtGui).
Décision : miniz (MIT) dans `third_party/miniz`, `document.json` versionné `actcraft-document/1`, ressources à leur octet près sous leur SHA-256, miniatures PNG ; écriture atomique sur fil secondaire.
Conséquences : format lisible par d'autres outils ; images non recompressées ; lecture tolérante aux clés inconnues, refus d'une version majeure inconnue.
Réversible : avant la première release ; ensuite, tout changement passe par `actcraft-document/2` et un lecteur des deux.

**DA4 — Lecteur `.flipchart` sans QtGui, vérifié par un oracle local.**
Contexte : étape 3 (`docs/BRIEF.md:112-114`) ; `samples/` contient des contenus de Jérôme (et de tiers) qui ne doivent pas entrer dans un dépôt public ; `_schema.py` est extrait du binaire.
Décision : `formats/` (QtCore) lit conteneur, index, flux objet et modèle avec des tables écrites à la main ; oracle JSON du lecteur Python comparé localement ; paperboards synthétiques écrits par notre écrivain de test pour la CI ; `actcraft_forbid_links(actcraft_formats Qt6::Gui …)`.
Conséquences : décodage d'images dans `render/` sur fils secondaires ; le test d'oracle est sauté sans `ACTCRAFT_SAMPLES_DIR` et signalé comme tel.
Réversible : un écrivain `.flipchart` pourra s'ajouter s'il devient validable (coffre 0069).

**DA5 — Un peintre partagé pour le logiciel, les miniatures et les exports.**
Contexte : chaque moteur a déjà un chemin `QPainter` (coffre 0080) ; règle 1 de fluidité (vignettes et exports hors du fil principal).
Décision : une fonction `paintPage(QPainter&, page, options)` dans `render/`, réutilisée par le rendu logiciel, les miniatures (sur `QImage`, fils secondaires), l'export d'image et `QPdfWriter`.
Conséquences : un seul code à tester au pixel pour l'apparence des objets ; le chemin RHI reste comparé à lui par les contrôles de contenu.
Réversible : un export peut garder son propre peintre sans toucher aux autres.

**DA6 — Outils et fonctions par registres.**
Contexte : règles 1 et 2 du BRIEF ; `PenTool` ne connaît que `InkSurface` et `DocumentSink` (`tools/include/actcraft/tools/pentool.h:10-27`).
Décision : interface `Tool` et registre dans `tools/` ; registre de fonctions dans `features/`, chaque fonction retirable par `ACTCRAFT_FEATURES` ; outils et fonctions agissent sur le document par la pile d'annulation et sur l'écran par des surcouches.
Conséquences : un outil se réécrit sans toucher aux autres ; Android retire les fonctions de bureau à la construction.
Réversible : registres internes, interfaces petites.

**DA7 — Banc : résultat par tâche, non-régression par lot.**
Contexte : l'étape 2 n'est pas faite (coffre 0085) ; la priorité absolue reste la fluidité (`docs/BRIEF.md:10,88`).
Décision : chaque tâche de `render/ input/ tools/ ui/` joint le JSON du banc hors écran (résultat) ; en fin de lot, banc complet D3D11 plein écran (jeton `ecran`) : tout critère tenu par la référence doit l'être encore, sinon tâche de correction avant clôture. Référence remesurée à la fin du lot 1 (nouvelle interface autour de la page) ; après le lot 3, « figements » et « reprises » y entrent.
Conséquences : une régression est vue au plus tard en fin de lot ; le banc continue de charger la vraie interface (coffre 0079) avec la page générée.
Réversible : la règle de refus du BRIEF remplace cette porte dès la décision go / no-go.

**DA8 — Captures et scénarios d'interface joués par l'agent.**
Contexte : le profil n'a pas de preuve visuelle automatique (`.claude/chantier.md:38-42`) ; la souris de Jérôme n'est jamais pilotée.
Décision : exécutable `actcraft-captures` (dans `bench/`, réutilise l'injecteur d'événements) : scénarios nommés (clics, touches, stylet, molette, gestes), `grabWindow` à des étapes données, contrôles de pixels, PNG et `index.md` ; `scripts/captures.ps1 -Scenario <nom>|tous [-Rendu d3d11|logiciel] [-HorsEcran] -Sortie <dossier>`.
Conséquences : scénarios d'acceptation rejouables en CI (hors écran) et à l'écran ; les captures de la porte de fin sont produites, pas faites à la main.
Réversible : un pilote natif peut s'ajouter derrière la même interface de scénario.

**DA9 — Chaîne de paquets sans publication implicite.**
Contexte : profil (release = action finale) ; demande de `.exe` et `.apk`.
Décision : `scripts/paquet.ps1` (installateur Inno Setup, local) et `scripts/paquet-android.ps1` ; CI de PR : construction de l'installateur + installation silencieuse + lancement, APK `arm64-v8a` et `x86_64` + émulateur, sans téléversement ; `release.yml` sur tag `v*` crée un brouillon.
Conséquences : les paquets sont prouvés à chaque PR ; rien n'est téléchargeable avant un go.
Réversible : MSI ou `.aab` peuvent s'ajouter au même flux.

**DA10 — Plateformes isolées par capacités et options de construction.**
Contexte : code Windows seulement dans `app/renderbackend.cpp` et `bench/` ; Android n'a ni bureau ni capture d'écran simple.
Décision : chaque commande déclare ses plateformes ; les fonctions de bureau (caméra, annotation, outils de bureau, clavier à l'écran) sont exclues de la construction Android ; `#ifdef` limités à `app/` et aux fichiers `*_win`, `*_mac`, `*_android`.
Conséquences : un seul arbre QML ; menus Android plus courts ; la CI construit les trois cibles.
Réversible : une fonction portée plus tard ne change que sa déclaration de plateformes.

**DA11 — Versement au cache sans redessin complet.**
Contexte : 1 figement sur 10 à 64,4 ms au passage encre → cache (`docs/mesures/2026-10-13-prototype-mesure.md`) ; coffre 0081.
Décision : les objets versés depuis le dernier redessin restent des nœuds au-dessus du cache et le cache est redessiné hors de l'écriture (inactivité, nombre d'objets, fin de zoom) ; si la reprise d'un trait pendant ce redessin dépasse sa limite, passage à un cache par tuiles qui ne redessine que les tuiles touchées. Le banc gagne un scénario à pauses longues pour que le redessin différé soit mesuré.
Conséquences : critère d'acceptation : 0 figement et 0 reprise hors limite, trois passages D3D11 plein écran consécutifs.
Réversible : l'élément « page fixe » est seul concerné (coffre 0081).

## 5. Scénarios d'acceptation

Joués par l'agent sans souris réelle : événements injectés par `actcraft-captures` (DA8), ligne de commande,
installation silencieuse, émulateur Android. `<samples>` = `C:\Users\JeromeDenis\REA - Activinspire\samples`
(lecture seule).

- **S1 — Installateur (lot 1).** `pwsh -NoProfile -File ./scripts/paquet.ps1` produit
  `build/paquet/ActCraft-0.1.0-setup.exe` ; `ActCraft-0.1.0-setup.exe /VERYSILENT /CURRENTUSER /DIR=<temp>` installe
  sans droits d'administrateur ; `<temp>\actcraft.exe --version` affiche `0.1.0` ; `--quitter-apres 3000` sort à 0
  sur un poste sans Visual Studio (runtime local) ; la désinstallation silencieuse vide le dossier. La CI de la PR
  joue la même chose sur `windows-latest`.
- **S2 — Premiers pas (lot 1).** `scripts/captures.ps1 -Scenario premiers-pas -Rendu logiciel -HorsEcran` : la fenêtre
  montre menus, palette et page ; le scénario choisit le rouge et l'épaisseur 8 par leurs commandes, trace un trait au
  stylet injecté, passe à la page suivante (l'indicateur dit « 2 / 2 »), revient : le trait rouge est toujours là
  (contrôle de pixels) ; F5 bascule le plein écran. Rejoué à l'écran en D3D11 en fin de lot.
- **S3 — APK (lot 1).** La CI produit `ActCraft-0.1.0-arm64-v8a.apk` et `…-x86_64.apk` ; sur l'émulateur,
  `adb install`, `adb shell am start`, le processus vit encore après 10 s, `adb exec-out screencap -p` montre la
  palette (contrôle de pixels) ; `adb shell input swipe` d'un doigt laisse un trait visible sur la capture suivante.
- **S4 — Ouvrir un `.flipchart` (lot 2).** Localement, le test d'oracle passe sur les 17 paperboards de `<samples>`
  (124 pages) ; `actcraft.exe "<samples>\tri des formes.flipchart"` ouvre le document, l'indicateur montre le bon
  nombre de pages, et les captures de chaque page sont comparées aux miniatures `.thm` d'origine (écart médian
  ≤ 7 sur 255, comme la référence). En CI, les paperboards synthétiques passent.
- **S5 — Aller-retour natif (lot 2).** Un document de 3 pages (trait, forme, texte, image) enregistré en `.actcraft`,
  fermé, rouvert : même empreinte du modèle ; l'archive contient `document.json`, `resources/`, `thumbnails/` ;
  l'enregistrement s'est fait hors du fil principal (aucune image > 25 ms pendant l'enregistrement, relevé par la sonde).
- **S6 — Plus de figement (lot 3).** Trois passages `scripts/bench.ps1` (D3D11, plein écran, cache) : 0 figement et
  0 reprise hors limite sur tous les scénarios, dont celui à pauses longues ; résumé dans
  `docs/mesures/<date>-cache-incremental.md`.
- **S7 — Écrire, gommer, annuler (lot 4).** Deux traits ; la gomme coupe le milieu du premier : 3 objets ; un
  rectangle de sélection prend les deux morceaux ; glisser les déplace ; trois Annuler rendent les 2 traits
  d'origine à leur place ; Rétablir rejoue ; un pincement à deux doigts injecté zoome autour de son centre.
- **S8 — Formes, texte, pages, grille (lot 5).** Tracer un rectangle, poser un texte « Bonjour » par touches
  injectées, la miniature de la page se met à jour ; glisser la page 3 avant la page 1 dans le navigateur ; grille
  aimantée : le rectangle déplacé pose son coin sur une intersection.
- **S9 — Édition et export (lot 6).** Copier un groupe vers un deuxième onglet, l'aligner à gauche, annuler ;
  exporter 3 pages en PDF : le fichier compte 3 pages ; exporter une page en PNG de 1024 × 768.
- **S10 — Réglages gardés (lot 7).** Ancrer la palette à gauche, choisir une couleur personnalisée, ouvrir le
  navigateur d'objet, quitter, relancer : tout est restauré ; une largeur de contour changée dans le navigateur de
  propriétés s'annule.
- **S11 — Présentation (lot 8).** Le rideau couvre la page, un glisser depuis le haut la découvre ; un trait au stylo
  le long de la règle est droit ; un compte à rebours de 3 s se termine en passant à la page suivante.
- **S12 — Interactivité (lot 9).** Localement, `<samples>\présences.flipchart` : un clic sur un rond rouge masque
  puis réaffiche l'étiquette visée ; un objet déposé hors de son conteneur revient à sa place.
- **S13 — Release prête (lot 10).** `release.yml` en essai à blanc produit `ActCraft-1.0.0-setup.exe`,
  `ActCraft-1.0.0-arm64-v8a.apk` (`apksigner verify` réussi), `SHA256SUMS.txt` et les notes tirées du
  `CHANGELOG.md`, sans créer de release ; le tag et la publication attendent le go écrit de Jérôme.

## 6. Hors périmètre

**Exigences exclues (26), avec la raison :**

- Vote et boîtiers Promethean : F-126, F-127, F-1000, F-1010, F-1020, F-1030 (matériel propriétaire ; le
  navigateur de votes n'apparaît pas).
- Mode à deux utilisateurs : F-110, F-303 (tableau à double stylet de Promethean).
- Imports de formats propriétaires : F-661 (PowerPoint), F-662 (SMART), F-663 (ExamView, QTI), F-665 et F-814
  (packs de ressources `.as2`) ; et dans F-655, les anciens `.flp`.
- Enregistreurs d'écran et de son : F-292, F-293 (capture et codecs, absents de la référence).
- Reconnaissance d'écriture : F-295 (moteur de Windows seulement dans la référence, aucun sous Android).
- Vérificateur orthographique F-297, verrouillage du formateur F-298, téléimprimante F-291 (absents de la
  référence).
- HTML incorporé F-302 (moteur web) ; texte vertical, équation, extension de page F-301.
- Publier F-680 (restrictions propres au `.flipchart`, que nous n'écrivons pas).
- Boutons définis par l'utilisateur F-920, F-142 (lancement de programmes ; impossible sous Android).
- Navigateur Web F-299 et liens vers les services en ligne de Promethean (dans F-760, F-090, F-930).
- Cadres de page F-603 (comportement non élucidé).

**Autres exclusions :** écriture de `.flipchart` ; boîte d'impression ; mise à jour automatique ; paquet macOS et
`.aab` / Play Store ; armv7 ; signature de code ; Linux et iOS ; encre à faible latence native et prédiction du
stylet ; mesures sur Dell 3510 et MacBook M1 (étape 2, à faire par Jérôme) ; toute ressource de Promethean (icônes,
textes de l'aide, données extraites des binaires, ressources partagées d'origine).

## 7. Portes immédiates

Aucune. Tout ce que le chantier exige est installable par l'agent (JDK 17, SDK et NDK Android, Qt pour Android,
Inno Setup, module Qt PDF) ou se fait dans la CI gratuite d'un dépôt public. Les points qui coûtent ou engagent
(signature de code, clé de release Android, publication) n'arrivent qu'à la porte de release.
