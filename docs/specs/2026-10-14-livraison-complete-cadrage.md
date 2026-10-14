# Cadrage — livraison complète (étapes 3 et 4 du BRIEF, installateur Windows, APK Android)

Cadrage autonome (étage 1 de `nouvelle-feature`), 2026-10-14, révisé après le tour 1 de contre-relecture et les
arbitrages de l'orchestrateur. Jérôme ne répond à aucune question avant la fin du lot 1 : chaque point est tranché
depuis les sources, ou par un défaut prudent marqué « à revoir ».
Sources : `docs/BRIEF.md`, `CLAUDE.md`, `.claude/chantier.md`, cadrage `docs/specs/2026-10-13-prototype-mesure-cadrage.md`,
mesure `docs/mesures/2026-10-13-prototype-mesure.md`, coffre (décisions 0069, 0074 à 0076, 0079 à 0086, journal
`2026-10-14-actcraft-prototype-mesure-clos`, piège `aqtinstall-3-3-ne-sait-pas-installer-qt-6-12`), code de `main` @ 175b336
(graphe du dépôt), ancien dépôt `REA - Activinspire` (`docs/spec/fonctionnalites.md`, `docs/spec/backlog.md`,
`docs/tests/plan-comparatif.md`, `docs/re/format-flipchart.md`, `app/actcraft/`, corpus `docs/videos-tanguay/`),
poste de développement (Qt 6.12 `msvc2022_64` installé, aucun SDK Android, JRE 21 seul, 104 Go libres).

## 1. Demande et gabarit

**Demande.** Livrer ActCraft utilisable : lecteur `.flipchart` et format natif (étape 3), interface complète et
toutes les fonctions retenues, un module à la fois (étape 4), avec un installateur Windows `.exe` et un `.apk`
Android, en autonomie (Jérôme, 2026-10-14 : « Ajoute la release .exe et la release .apk »).

**Gabarit.** `lots` (plus de 20 tâches), budget de 40 questions : **38 tranchées**, aucune ouverte. Un cadrage, un
plan par lot, lots exécutés l'un après l'autre, une PR par lot depuis `feat/livraison-complete` remise à jour sur
`main` après chaque merge.

**Lots et estimation** (détail : décisions 4 à 14).

| Lot | Titre | Livrable visible par Jérôme | Version | Tâches |
|---|---|---|---|---|
| 1 | Socle ouvrable et paquets | installateur local + APK : une interface, le stylo en couleurs, plusieurs pages | 0.1.0 | ≈ 16 |
| 2 | Document, format natif, lecteur `.flipchart` | ouvrir un paperboard de `samples/`, enregistrer en `.actcraft`, rouvrir | 0.2.0 | ≈ 16 |
| 3 | Outils I : sélection, stylo, marqueur, gomme, annulation, gestes | écrire, gommer, sélectionner, annuler ; pincer pour zoomer | 0.3.0 | ≈ 12 |
| 4 | Outils II : formes, texte, pages, grille | formes, texte, navigateur de pages, grille aimantée | 0.4.0 | ≈ 13 |
| 5 | Édition et fichiers | copier-coller entre documents, grouper, aligner, PDF, images | 0.5.0 | ≈ 16 |
| 6 | Navigateurs, propriétés, profils | navigateurs d'objet / propriétés / notes / ressources, réglages gardés | 0.6.0 | ≈ 15 |
| 7 | Présentation et outils | rideau, spot, zoom de page, outils de maths, horloge, bureau, reconnaissance | 0.7.0 | ≈ 15 |
| 8 | Interactivité | actions, conteneurs, limiteurs, liens, encre révélatrice | 0.8.0 | ≈ 11 |
| 9 | Confrontation aux vidéos de Nadine Tanguay | écarts d'usage relevés dans le corpus, corrigés | 0.9.0 | ≈ 8 |
| 10 | Cache incrémental (H-mes-1) | rapport de mesure : plus aucun figement hors limite | 0.9.1 | ≈ 4 |
| 11 | Tablette, finitions, version 1.0 | APK confortable sur tablette, notes de version, release prête | 1.0.0 | ≈ 8 |

Total ≈ 134 tâches. Chaque lot se clôt par la double relecture, le banc (DA7), les captures (DA8), l'installateur
local et l'APK, et une porte de fin où l'orchestrateur propose la release du lot (publiée seulement sur go écrit).
La porte de fin du lot 1 pose en plus deux questions à Jérôme, avec un défaut appliqué sans réponse (décisions 2
et 11).

## 2. Décisions

| # | Point | Décision | Source | Réversible ? |
|---|---|---|---|---|
| 1 | Sens de « livraison complète » | La référence de comportement est ce que fait l'application Python : les 26 lignes du plan comparatif (toutes ✅ ou ➖). La spécification compte **200 exigences** (197 en tête de paragraphe, plus F-200 à F-202 écrites en liste) : **175 sont incluses**, chacune affectée à un lot ci-dessous, dont certaines réduites ou réservées à une plateforme (F-090, F-655, F-760, F-930 en partie ; F-070, F-071, F-080, F-275, F-294 au bureau ; F-295 à Windows) ; **25 sont exclues**, liste et raisons en § 6. Plateformes livrées : Windows (installateur) et Android (APK) ; macOS reste construit et testé par la CI. | `docs/BRIEF.md:116-119` ; `REA - Activinspire/docs/tests/plan-comparatif.md:6-33` ; `docs/spec/fonctionnalites.md` ; demande du 2026-10-14 | oui |
| 2 | Fonctions à revoir complètement | Défaut appliqué : aucune, chaque fonction reproduit le comportement de référence. La question est **posée à Jérôme à la porte de fin du lot 1** (avant les outils du lot 3) ; sans réponse, le défaut reste. Toute fonction passe par une commande nommée et un outil remplaçable : la revoir plus tard reste local. Hypothèse N1-1. | `docs/BRIEF.md:116,127` ; `CLAUDE.md` (mode autonome, point 2) ; arbitrage de l'orchestrateur ; `docs/BRIEF.md:69-72` | oui |
| 3 | Découpage | 11 lots séquentiels (tableau du § 1). Livrer tôt : le lot 1 donne une interface ouvrable **et** les deux paquets, ce qui purge d'abord le risque le plus inconnu (chaîne Android). Le modèle de document (squelette au lot 1, complet au lot 2) précède les outils. La confrontation aux vidéos et le cache incrémental viennent juste avant la release 1.0. | demande de Jérôme (« aucune interface ») ; méthode (modèle avant affichage) ; arbitrages de l'orchestrateur | oui |
| 4 | Lot 1 — Socle ouvrable et paquets | Tâche 1 : mise à jour du profil (décision 38). Squelette du modèle de DA2 dans `core/` (`Document` → pages ; `core/include/actcraft/core/model.h:105` ne connaît qu'une `Page`) ; registre de commandes et fichiers de données (DA1) ; coquille QML : ligne supérieure (menus Fichier, Éditer, Affichage, Insérer, Outils, Aide ; indicateur « page n / N » ; échelle ; plein écran), palette d'outils à droite, zone de page avec barres de défilement, pas de barre d'état ; thème et icônes à nous (décision 23) ; stylo branché : 16 pastilles, 4 épaisseurs + réglette, indicateur de largeur ; plusieurs pages (suivante crée une page à la fin, précédente) ; F5 ; `--version` et boîte « À propos » avec notices (décision 34) ; banc adapté à la nouvelle interface (DA7) ; outil de captures (DA8) ; installateur Windows (DA9) ; chaîne Android avec APK de débogage et essai sur émulateur (DA9, décision 27). F-001, F-002, F-003, F-020, F-021, F-022, F-060, F-120, F-121 (boutons des fonctions livrées), F-122, F-125, F-128, F-133, F-134 (stylo), F-137 (stylo), F-138, F-200, F-220, F-630 (suivante, précédente), F-720 (partie), F-760 (« À propos », aide = page du projet). | décisions 22-24, DA1, DA2, DA7-DA9 | oui |
| 5 | Lot 2 — Document, format natif, lecteur `.flipchart` | Modèle de document complet et pile d'annulation dans `core/` (DA2) ; format natif `.actcraft` (DA3) ; lecteur `.flipchart` et son oracle (DA4) ; rendu des nouveaux types (texte, image décodée sur fil secondaire, ligne, fond couleur / dégradé / image, grille à niveaux, quatre couches, ordre couche puis profondeur puis arbre) ; peintre partagé (DA5) ; Fichier ▸ Nouveau (5 tailles), Ouvrir, Enregistrer, Enregistrer sous, Fermer, Quitter avec demande d'enregistrement ; commandes de fichier qui prennent un chemin (décision 22) ; avertissements de lecture affichés sans bloquer. Un `.flipchart` ouvert s'enregistre en `.actcraft`. F-500, F-510, F-511, F-512 (données), F-574, F-583 (données), F-602, F-610 (rendu), F-621 (rendu), F-655 (`.flipchart` en lecture), F-656, F-657, F-700 (partie). | `docs/BRIEF.md:112-114` ; coffre 0069 ; `docs/re/format-flipchart.md` § 1-7 | oui |
| 6 | Lot 3 — Outils I | Sélection (clic, rectangle qui touche, déplacer, poignées, redimensionner, rotation, barre d'objet), stylo et marqueur (mémoire par outil, couleur et épaisseur, translucidité du marqueur), gomme (coupe et scinde les traits, n'efface que les annotations), Annuler / Rétablir, Supprimer, corbeille, Effacer ▸ et Rétablir la page ; gestes à deux doigts (décision 25). F-050, F-130, F-131, F-132, F-136, F-137, F-201, F-202 (partie), F-210, F-225, F-230, F-520, F-521, F-522, F-523, F-524, F-528, F-541, F-951-F-953 (fonctions du lot). | `docs/BRIEF.md:117-118` (ordre) ; `plan-comparatif.md:14-16,21-23` | oui |
| 7 | Lot 4 — Outils II | Formes (palette à nous, tracé depuis le coin ou le centre, points intelligents), texte (création, édition, barre de mise en forme, décision 37), panneau des navigateurs avec le navigateur de pages (miniatures à jour, sélection multiple, réordonner, insérer, couper / copier / coller des pages), menu de page, grille (créateur à niveaux, accrochage, masquer). F-040, F-041, F-042, F-100, F-240, F-241, F-242, F-250, F-251, F-252, F-253, F-600, F-601, F-620, F-621, F-622, F-623, F-771, F-800, F-801, F-802, F-803. | `docs/BRIEF.md:117-118` ; `plan-comparatif.md:18,20,24,33` | oui |
| 8 | Lot 5 — Édition et fichiers | Couper / copier / coller / dupliquer, grouper, organiser et couches, transformer et aligner, menu d'édition d'objet, points de forme, pot de peinture, connecteur, modification de stylo, verrouiller / masquer / glisser une copie, onglets de documents (plusieurs ouverts, pages glissées d'un onglet à l'autre), insérer une image ; module d'export (DA12) : page en image, PDF, impression réduite au PDF (N1-15), import PDF. F-010, F-011, F-012, F-129, F-221, F-235, F-245, F-513, F-525, F-526, F-527, F-535, F-536, F-537, F-540, F-585, F-586, F-587, F-660, F-664, F-670 (réduit), F-671, F-672, F-710, F-740 (partie), F-770. | `plan-comparatif.md:17,19,22,25,29` | oui |
| 9 | Lot 6 — Navigateurs, propriétés, profils | Navigateurs d'objet, de propriétés (sections F-570 à F-583), de notes, de ressources (dossiers locaux, ajout d'un objet à la bibliothèque) ; palette ancrable, flottante, réduite, épinglée, taille des icônes ; palette de couleurs étendue ; barre de raccourcis ; profils (les nôtres, N2-6), réglages et persistance ; mode Création ; tableau de bord d'accueil ; documents récents et résumé ; étiquettes. F-030 à F-034, F-090 (sans les entrées exclues), F-123, F-135, F-140, F-141, F-560, F-570 à F-573, F-575, F-576, F-581, F-772, F-810, F-811, F-812, F-813, F-820, F-821, F-830, F-840, F-880, F-881, F-882, F-900, F-910, F-930 (sections des fonctions incluses), F-935. | `plan-comparatif.md:13,28-30` | oui |
| 10 | Lot 7 — Présentation et outils | Zoom de page, rideau et spot (et leurs réglages par page), effets de changement de page réduits (N2-10), outils de maths (règle, rapporteur, compas, équerre, origine XY), dés, calculatrice, horloge et compte à rebours, date et heure, clavier à l'écran (bureau), reconnaissance de forme (algorithme à nous), reconnaissance d'écriture (Windows seulement, moteur d'encre du système comme la référence ; plateforme déclarée, DA10), caméra (bureau), annotation de bureau et outils de bureau (bureau). F-070, F-071, F-080, F-124, F-260, F-265, F-270, F-275, F-280 à F-286, F-290, F-294, F-295, F-296, F-300, F-591, F-592, F-593, F-640 (réduit), F-750 (entrées incluses). | `plan-comparatif.md:14` (reconnaissance d'écriture ✅, `canvas/recognition.py`), `:26-27,31` | oui |
| 11 | Lot 8 — Interactivité | Navigateur d'action et actions (commande, page, objet, glisser-déposer ; sans les actions de vote), objets d'action, conteneurs, rotation et limiteurs, liens vers un fichier ou un site, médias ouverts par l'application du système (N2-9), encre révélatrice (encre magique de la référence). F-255, F-550, F-551, F-552 (réduit), F-577, F-578, F-579, F-580, F-582, F-850, F-851, F-890, F-891 (sans vote), F-892 (sans vote). | `plan-comparatif.md:28` ; `backlog.md` lot 3 | oui |
| 12 | Lot 9 — Confrontation aux vidéos de Nadine Tanguay | Comparer le produit aux usages réels d'enseignants décrits par le corpus `REA - Activinspire/docs/videos-tanguay/` (une fiche par vidéo, `README.md` : synthèse et principes absents de `fonctionnalites.md`), hors dépôt public ; relever chaque écart (geste, enchaînement, réglage par défaut, fonction manquante incluse au périmètre) ; corriger ceux qui relèvent du périmètre, noter les autres en hypothèses pour Jérôme. **Aucune capture, transcription ni citation de ces vidéos n'entre dans le dépôt** ; les constats commités sont écrits par nous, en termes de comportement. Si le corpus est absent au démarrage du lot, le lot se réduit à ce constat et passe. | arbitrage de l'orchestrateur ; `CLAUDE.md:65-69` (dépôt public) | oui |
| 13 | Lot 10 — Cache incrémental (H-mes-1) | Chantier dédié à H-mes-1 : supprimer le figement au versement d'un objet dans le cache (DA11), mesuré par le banc en D3D11 plein écran, avec un scénario de pauses longues ajouté pour ne pas tromper la mesure. Placé **juste avant le lot de release**, conformément à la décision écrite de Jérôme (« après la mesure sur le Dell 3510 ») ; **avancé** si Jérôme le demande à la porte du lot 1 ou s'il a fourni la mesure du Dell. Hypothèse N1-10. | journal du coffre `2026-10-14-actcraft-prototype-mesure-clos` (porte de fin) ; `docs/mesures/2026-10-13-prototype-mesure.md` ; arbitrage de l'orchestrateur | oui |
| 14 | Lot 11 — Tablette, finitions, 1.0 | Mise en page étroite (navigateurs masqués, palette repliable), dialogues de fichiers Android, essais sur émulateur et sur tablette réelle si Jérôme en fournit une (N1-11), passe de fluidité finale, raccourcis restants (F-950 à F-953 des fonctions incluses), `CHANGELOG.md` et guide de démarrage dans `README.md`, flux de release complet en essai à blanc (décision 32). | demande du 2026-10-14 ; `docs/BRIEF.md:13` | oui |
| 15 | Modèle de document | Dans `core/` (sans QtGui, contrôlé par CMake) : document, pages (taille, fond, grille, notes, propriétés), quatre couches (fond, inférieure, intermédiaire, supérieure), objets à identifiant stable (annotation stylo / marqueur / révélatrice, forme à polygones, ligne, texte en HTML riche + mise en forme, image et média par ressource, groupe par références, connecteur, action), transformation affine 2D, propriétés communes (nom, visible, verrouillé, translucidité, restrictions). Unités : pixels de page (1024 × 768 par défaut), comme les `.flipchart`. Squelette au lot 1, complet au lot 2. Voir DA2. | `docs/BRIEF.md:51,74-75` ; `docs/re/format-flipchart.md` § 6-7 ; `fonctionnalites.md` § 7 | oui |
| 16 | Annulation | Pile d'annulation propre dans `core/` (`QUndoStack` est dans QtGui, interdit à `core/`) : une commande par modification, fusion des glissements continus, par document, 500 étapes au plus (N2-1). Toute modification du document passe par elle. | `docs/BRIEF.md:51` ; `fonctionnalites.md` F-132, F-840 | oui |
| 17 | Format natif | `.actcraft` : archive zip (miniz, MIT, dans `third_party/`) contenant `document.json` (`"format": "actcraft-document/1"`, clés en anglais), `resources/<sha256>.<ext>` (octets d'origine), `thumbnails/page-<n>.png` (octets PNG produits par `render/`, simplement rangés par `formats/`). Lecture et écriture sur un `QIODevice`, sur fil secondaire ; sur un fichier local, écriture dans un temporaire voisin puis renommage ; repli Android (URI `content://` sans dossier voisin inscriptible) tranché au lot 2 (non vérifié sous Qt 6.12). Lecture et écriture testées par aller-retour. Extension : N1-3. Voir DA3. | `docs/BRIEF.md:52,78-79,114` ; coffre 0069 (format ouvert zip + JSON du clone Python) | oui avant la première release, non ensuite |
| 18 | Lecteur `.flipchart` | Dans `formats/` (QtCore seulement : `qUncompress` pour le conteneur), lecture seule, d'après notre documentation `docs/re/format-flipchart.md`. Les tables d'identifiants sont **écrites à la main** pour les seuls types et propriétés lus, jamais copiées de `_schema.py` (généré depuis le binaire). Un `.flipchart` ne se réécrit pas : Enregistrer propose `.actcraft`. Voir DA4. | `CLAUDE.md:65-69` ; coffre 0069 et 0076 ; `REA - Activinspire/app/actcraft/io/_schema.py:1-7` | oui |
| 19 | Oracle du lecteur | Script dans l'ancien dépôt qui exporte en JSON canonique (clés et arrondis fixés par nous) ce que lit le lecteur Python pour chacun des 17 paperboards ; outil C++ `actcraft-flipchart-dump` qui produit le même JSON ; test comparatif lancé **localement** seulement (variable `ACTCRAFT_SAMPLES_DIR`). Les JSON d'oracle restent dans l'ancien dépôt ; les sorties C++ et les captures de pages vont sous `.tmp/` ou `build/`, **jamais commitées** (les `samples/` sont des paperboards réels de Jérôme, dont des listes de présence). En CI : paperboards synthétiques produits par un écrivain de test écrit par nous d'après notre documentation. | `docs/BRIEF.md:112-114` ; `CLAUDE.md:62-69` (samples jamais modifiés, dépôt public) | oui |
| 20 | Commandes nommées | Registre dans `actions/` (QtCore) : identifiant anglais à points (`page.next`, `tool.pen`, `file.open`), libellé, icône, raccourci par défaut, cochable, état actif, plateformes, **arguments** (un chemin pour les commandes de fichier). Menus, palette, barre d'objet et raccourcis décrits en JSON, lus d'abord dans le dossier du profil puis dans les ressources intégrées : réorganiser sans recompiler. Voir DA1. | `docs/BRIEF.md:53,69-73` | oui |
| 21 | Outils et fonctions | Un outil implémente une interface `Tool` (activation, échantillons d'entrée, clavier, surcouche) et s'inscrit dans un registre de `tools/` ; une fonction de `features/` s'inscrit dans le sien et se retire par une option CMake (`ACTCRAFT_FEATURES`). Les outils ne voient que des interfaces (document, encre, vue). Voir DA6. | `docs/BRIEF.md:56-57,71-72` ; `tools/include/actcraft/tools/pentool.h:10-27` | oui |
| 22 | Interface QML et dialogues | Qt Quick Controls, style `Basic` personnalisé (le plus léger), `QtQuick.Dialogs` pour fichiers et couleurs (natifs dans l'application), éléments C++ pour la page. Les dialogues natifs ne reçoivent pas les événements postés (coffre 0084) : le dialogue ne fait que fournir un chemin à la commande `file.open` / `file.save-as`, que les scénarios et la ligne de commande appellent directement ; `actcraft-captures` force les dialogues Qt Quick non natifs quand un scénario doit en voir un (moyen exact relevé au lot 2). Disposition de la référence (menus en haut, palette à droite, navigateurs à gauche, corbeille sous la palette, pas de barre d'état) ; aspect à nous. Textes en français écrits par nous (`qsTr`), sans les termes propres à Promethean (N1-12). | `docs/BRIEF.md:2-11,58` ; `CLAUDE.md:65-69` ; `fonctionnalites.md` F-001 à F-003 ; modules présents dans `C:\Qt\6.12.0\msvc2022_64\bin` (QuickControls2, QuickDialogs2, Svg) ; coffre 0084 | oui |
| 23 | Design visuel et icônes | Direction posée par le skill `impeccable` en première tâche de design : jetons (couleurs, espacements, typographie) dans un singleton QML `Theme`, jeu d'icônes SVG **neuf** dessiné par nous sur une grille de 24 px (pas les icônes du clone Python, qui imitent l'original). Ombres en images (règle 4). Choix signalé à la porte visuelle de fin de lot 1 (N1-13). | `.claude/chantier.md:79-80` ; `plan-comparatif.md:32` ; `docs/BRIEF.md:85-86` | oui |
| 24 | Peintre partagé | Les chemins `QPainter` des moteurs de rendu (déjà requis par le rendu logiciel) servent aussi aux miniatures, à l'export d'image et de PDF, sur fils secondaires. Voir DA5. | coffre 0080 ; `docs/BRIEF.md:78-79` | oui |
| 25 | Tactile | Un doigt agit comme l'outil courant (comme la souris) ; deux doigts font défiler et zoomer (pincement) ; le stylet garde la pression. Même règle sous Windows (écrans et tableaux tactiles) et Android. Hypothèse N1-11. | prototype N2-12 (tactile hors périmètre jusque-là) ; `fonctionnalites.md` F-930 (« Gestion de la fonction tactile », défaut non documenté) | oui |
| 26 | Installateur Windows | Inno Setup 6, installation **par utilisateur sans droits d'administrateur** par défaut (choix « pour tous » proposé), dossier `%LOCALAPPDATA%\Programs\ActCraft`, menu Démarrer, désinstallation, installation silencieuse possible ; contenu : preset `release` (la construction mesurée) passé par `windeployqt --release --no-compiler-runtime`, runtime MSVC copié à côté de l'exécutable, licences ; association `.actcraft` pour l'utilisateur, `.flipchart` proposée décochée. Nom `ActCraft-<version>-setup.exe`. Non signé (N1-4). | prototype décision 36 (runtime local, `vc_redist` exige l'administrateur) ; `scripts/paquet-banc.ps1` | oui |
| 27 | Chaîne Android | Qt 6.12 pour Android installé par aqtinstall de développement (hôte Qt de bureau de même version), JDK 17, SDK et NDK à la version exigée par Qt 6.12 (relevée en première tâche Android, non vérifiée ici). La CI (Ubuntu) est la référence de construction ; l'agent installe aussi la chaîne sur le poste (dans `~/.tools` et `C:\Qt`, environ 10 Go). **Émulateur** : la première tâche Android vérifie si l'accélération WHPX est utilisable sans droits d'administrateur (hyperviseur présent, état lisible seulement en administrateur) ; sinon l'émulateur tourne en CI (KVM sur `ubuntu-latest`) ; si aucun des deux ne marche, porte « installation » (§ 7). Presets `android-arm64` et `android-x86_64` ; banc et tests exclus de ces constructions (`ACTCRAFT_BUILD_BENCH=OFF`, `ACTCRAFT_BUILD_TESTS=OFF`). | piège `aqtinstall-3-3-ne-sait-pas-installer-qt-6-12` ; `CMakeLists.txt:24-25` ; `.github/workflows/ci.yml` ; constat de relecture 13 | oui |
| 28 | Cible Android | APK `arm64-v8a` pour la release ; `x86_64` pour l'émulateur. API minimale = défaut de Qt 6.12 (relevé en première tâche). Identifiant `io.github.jejeasterix.actcraft` (espace de noms que Jérôme possède). Aucune permission demandée (fichiers par le sélecteur du système, pas de réseau). Hypothèse N1-5. | défaut prudent | non après la première release (identifiant) |
| 29 | Rendu Android et code de plateforme | Android : rendu par défaut de Qt (RHI, OpenGL ES), pas d'option `--rendu`. Le code propre à Windows reste dans `app/renderbackend.cpp` et `bench/` (`gpumemory_win.cpp`, mémoire, machine) ou dans des fichiers `*_win` (reconnaissance d'écriture). Chaque commande déclare ses plateformes : caméra, annotation et outils de bureau, clavier à l'écran ne s'affichent pas sous Android, la reconnaissance d'écriture seulement sous Windows. Voir DA10. | `app/renderbackend.cpp:8-12` ; `bench/src/gpumemory_win.cpp` | oui |
| 30 | Signature et forme du paquet Android | `.apk` seulement (pas de `.aab`, pas de Play Store, qui coûte un compte payant). APK de la CI signés avec la clé de débogage. La clé de release n'est créée qu'au go de la première release : keystore hors dépôt, mot de passe et keystore en secrets GitHub, copie de sauvegarde remise à Jérôme. Hypothèse N1-6. | demande (« .apk ») ; méthode (argent → défaut prudent) | non (perdre la clé empêche les mises à jour) |
| 31 | Versions | SemVer depuis `project(VERSION)` de `CMakeLists.txt` : 0.1.0 au lot 1, mineure + 1 par lot jusqu'à 0.9.0 (lot 9), 0.9.1 (lot 10), 1.0.0 (lot 11) ; `versionCode` Android = majeur × 10000 + mineur × 100 + correctif. `CHANGELOG.md` en français, une section par version, rédigé à la clôture de chaque lot. | `CMakeLists.txt:3-6` | oui |
| 32 | Releases | Workflow `release.yml` déclenché par un tag `v<version>` : construit l'installateur et l'APK signé, crée une release GitHub **brouillon** avec les deux fichiers, les sommes SHA-256 et la section du `CHANGELOG.md`. Sans les secrets de signature, il **échoue** avec un message explicite (jamais d'APK de débogage dans une release). Le tag, la poussée du tag et la **publication du brouillon** (`gh release edit --draft=false` ou appel d'API) relèvent du **même go écrit** de Jérôme (action finale). Un essai à blanc (`workflow_dispatch`, `dry-run`) produit tout, signé par la clé de débogage et marqué « non publiable », sans créer de release. | `.claude/chantier.md:97-106` (action finale) ; constat de relecture 1 | oui |
| 33 | Distribution avant le go / no-go | Construire et vérifier les paquets à chaque PR **sans publier d'artefact** ; l'installateur et l'APK de chaque lot sont remis à Jérôme localement (`build/paquet/`) à la porte de fin ; la release est proposée à chaque porte. Le profil (« due seulement quand Jérôme la demande ; pas de distribution avant le go / no-go », 2026-10-13) est réécrit en tâche 1 (décision 38) : Jérôme a demandé les releases le 2026-10-14, chaque publication reste sous son go écrit (N1-7). | `.claude/chantier.md:105` ; demande du 2026-10-14 ; prototype N1-4 | oui |
| 34 | Licences | Code MIT ; Qt en LGPLv3, lié dynamiquement (DLL sous Windows, `.so` dans l'APK) ; aucun module Qt sous GPL seule (ni Qt Virtual Keyboard : clavier à l'écran écrit par nous, ni Qt Charts) ; fichier `THIRD_PARTY_NOTICES.md` (Qt, ses dépendances, miniz) livré dans l'installateur et l'APK et affiché par « À propos » ; un script compare les bibliothèques Qt déployées à une liste autorisée. Hypothèse N1-8. | coffre 0076 (« Validation juridique : non faite ») ; `docs/BRIEF.md:14` | oui |
| 35 | Banc pendant le chantier | Le régime de la décision 38 du prototype continue (verdict = résultat, pas de refus sur le budget absolu) tant que l'étape 2 n'est pas faite. S'y ajoute une **porte de non-régression par lot** sur la machine de développement (DA7). Hypothèse N1-9. | coffre 0085 (« cesse d'elle-même à la décision go / no-go ») ; `.claude/chantier.md:44-68` | oui |
| 36 | Preuve visuelle | Outil `actcraft-captures` et `scripts/captures.ps1` : scénarios d'interface nommés, joués par événements injectés et par commandes, captures PNG et contrôles de pixels ; hors écran en rendu logiciel pour chaque tâche d'interface, à l'écran en D3D11 en fin de lot sous le jeton `ecran`. Sorties sous `.tmp/` ou `build/`, jamais commitées. Voir DA8. | `.claude/chantier.md:38-42` ; coffre 0084 (injection) | oui |
| 37 | Texte | Contenu en HTML riche (`QTextDocument`), dessiné par le peintre partagé dans une texture pour le rendu RHI (redessinée après un zoom, comme le cache) ; édition dans un élément d'édition QML pendant la frappe. Police par défaut des nouveaux textes : N2-3. Les `.flipchart` recalent la taille sur la largeur des runs, comme la référence. | `docs/re/format-flipchart.md` § 7.5 ; `plan-comparatif.md:20` | oui |
| 38 | Profil du chantier | Le cadreur ne modifie pas `.claude/chantier.md`. **Tâche 1 du lot 1** : (a) réécrire la ligne « Due seulement quand Jérôme la demande… » (Jérôme a demandé les releases le 2026-10-14 ; chaque publication reste sous son go écrit, et la publication du brouillon fait partie du même go) ; (b) ajouter au bloc `action-finale` les motifs `\bgh\s+release\s+edit\b.*--draft[= ]?false` et `\bgh\s+api\b.*releases` ; (c) écrire le régime du banc (décision 35) et la commande de preuve visuelle (décision 36). Une tâche de fin de lot 1 y ajoute les commandes réelles (captures, paquets). | `.claude/chantier.md:3-4,97-106` ; `~/.claude/skills/nouvelle-feature/references/etage-4-cloture.md` § 4.10 (go consommé au tag puis fichier supprimé) ; contrat du cadreur ; constat de relecture 1 | oui |

## 3. Hypothèses à revoir

### Niveau 1 — difficiles à défaire ou visibles des utilisateurs

- **N1-1 — Aucune fonction à revoir (défaut).** Jérôme n'a pas donné la liste (`docs/BRIEF.md:127`) ; la question lui
  est posée à la porte de fin du lot 1 ; sans réponse, tout reproduit la référence. Revoir une fonction plus tard =
  remplacer sa commande et son outil.
- **N1-2 — 25 exigences exclues** (§ 6) et quatre réduites : impression → PDF (N1-15), médias → application du
  système (N2-9), effets de changement de page réduits (N2-10), fonctions de bureau absentes d'Android,
  reconnaissance d'écriture sous Windows seulement.
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
  (2026-10-13), réécrite en tâche 1 ; publication toujours soumise au go écrit.
- **N1-8 — Licences non validées juridiquement.** LGPL de Qt respectée par liaison dynamique et notices ; pas d'avis
  d'un juriste.
- **N1-9 — Fluidité jugée sur la machine de développement.** La porte du lot 1 compare à la mesure du prototype
  (`docs/mesures/2026-10-13-prototype-mesure.md`) ; la mesure de fin de lot 1, commitée, devient la référence des lots
  suivants (DA7). Ni le Dell 3510 ni le M1 ; Android n'a pas de machine de référence (`docs/BRIEF.md` § 3) et n'est
  pas mesuré par le banc.
- **N1-10 — H-mes-1 traité après les fonctions.** Le lot cache est l'avant-dernier, comme Jérôme l'a écrit (« après la
  mesure sur le Dell 3510 ») ; d'ici là, chaque trait figé peut coûter un figement de 64 ms au plus sur la machine de
  développement. La question « l'avancer ? » est posée à la porte du lot 1 ; il est avancé sur demande ou dès que la
  mesure du Dell est fournie.
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
- **N1-16 — Corpus vidéo.** Le lot 9 dépend d'un corpus préparé hors dépôt ; ses constats peuvent ajouter des
  corrections que Jérôme n'a pas vues, toutes dans le périmètre du § 2.

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
- **N2-12** Émulateur Android : WHPX local s'il est utilisable sans administrateur, sinon CI sur KVM (décision 27).
- **N2-13** Pas de paquet macOS (pas demandé ; la signature Apple est payante) ; la CI macOS continue.
- **N2-14** Pas d'enregistrement automatique ni de reprise après plantage (absents de la référence).
- **N2-15** Import PDF par le module Qt PDF (à installer : absent de `C:\Qt\6.12.0\msvc2022_64`) ; sous Android
  seulement s'il est disponible pour cette cible.

## 4. Décisions d'architecture

**DA1 — Commandes nommées décrites par des données.**
Contexte : règles 1 et 3 du BRIEF (`docs/BRIEF.md:69-73`) ; `actions/` est vide (`actions/include/actcraft/actions/actions.h`).
Décision : registre `CommandRegistry` (QtCore) d'identifiants anglais, arguments, état actif / coché / plateformes ; menus, palette, barre d'objet, raccourcis en JSON (profil d'abord, ressources ensuite) ; `ui/` expose un modèle QML.
Conséquences : chaque bouton déclenche une commande par son nom ; les scénarios appellent les commandes de fichier avec un chemin, sans dialogue ; une commande absente d'une plateforme disparaît des menus.
Réversible : le format des fichiers est versionné ; un autre chargeur peut lire le même registre.

**DA2 — Modèle de document et annulation dans `core/`.**
Contexte : le modèle actuel (`core/include/actcraft/core/model.h:105`) n'a qu'une `Page` et trois types du prototype ; `core/` est interdit de QtGui.
Décision : document → pages → quatre couches → objets à identifiant stable et transformation affine, ressources par empreinte ; modifications seulement par commandes de la pile d'annulation de `core/` ; squelette document → pages au lot 1, le reste au lot 2.
Conséquences : formats, outils et rendu partagent un seul modèle testable sans écran ; le générateur de 300 objets du banc est porté sur le nouveau modèle (empreinte recalculée).
Réversible : les types d'objets sont des variantes ajoutables ; la pile reste interne au document.

**DA3 — Format natif `.actcraft` : zip + JSON + ressources.**
Contexte : `docs/BRIEF.md:52,114` ; le zip de Qt (`QZipReader` / `QZipWriter`, `QtCore/6.12.0/QtCore/private/qzipreader_p.h`) est une API **privée** (`Qt6::CorePrivate`), sans garantie entre versions.
Décision : miniz (MIT) dans `third_party/miniz`, `document.json` versionné `actcraft-document/1`, ressources à leur octet près sous leur SHA-256, miniatures PNG ; lecture et écriture sur un `QIODevice` sur fil secondaire, écriture atomique sur fichier local, repli Android tranché au lot 2.
Conséquences : format lisible par d'autres outils ; images non recompressées ; lecture tolérante aux clés inconnues, refus d'une version majeure inconnue.
Réversible : avant la première release ; ensuite, tout changement passe par `actcraft-document/2` et un lecteur des deux.

**DA4 — Lecteur `.flipchart` sans QtGui, vérifié par un oracle local.**
Contexte : étape 3 (`docs/BRIEF.md:112-114`) ; `samples/` contient des contenus de Jérôme (et de tiers) qui ne doivent pas entrer dans un dépôt public ; `_schema.py` est extrait du binaire.
Décision : `formats/` (QtCore) lit conteneur, index, flux objet et modèle avec des tables écrites à la main, et range le format natif ; oracle JSON du lecteur Python comparé localement ; paperboards synthétiques écrits par notre écrivain de test pour la CI ; `actcraft_forbid_links(actcraft_formats Qt6::Gui …)`. PDF et images vont dans le module d'export (DA12).
Conséquences : décodage d'images dans `render/` sur fils secondaires ; le test d'oracle est sauté sans `ACTCRAFT_SAMPLES_DIR` et signalé comme tel ; ses sorties restent sous `.tmp/` ou `build/`.
Réversible : un écrivain `.flipchart` pourra s'ajouter s'il devient validable (coffre 0069).

**DA5 — Un peintre partagé pour le logiciel, les miniatures et les exports.**
Contexte : chaque moteur a déjà un chemin `QPainter` (coffre 0080) ; règle 1 de fluidité (vignettes et exports hors du fil principal).
Décision : une fonction `paintPage(QPainter&, page, options)` dans `render/`, réutilisée par le rendu logiciel, les miniatures (sur `QImage`, fils secondaires), l'export d'image et `QPdfWriter` (DA12).
Conséquences : un seul code à tester au pixel pour l'apparence des objets ; le chemin RHI reste comparé à lui par les contrôles de contenu.
Réversible : un export peut garder son propre peintre sans toucher aux autres.

**DA6 — Outils et fonctions par registres.**
Contexte : règles 1 et 2 du BRIEF ; `PenTool` ne connaît que `InkSurface` et `DocumentSink` (`tools/include/actcraft/tools/pentool.h:10-27`).
Décision : interface `Tool` et registre dans `tools/` ; registre de fonctions dans `features/`, chaque fonction retirable par `ACTCRAFT_FEATURES` ; outils et fonctions agissent sur le document par la pile d'annulation et sur l'écran par des surcouches.
Conséquences : un outil se réécrit sans toucher aux autres ; Android retire les fonctions de bureau à la construction.
Réversible : registres internes, interfaces petites.

**DA7 — Banc : résultat par tâche, non-régression par lot.**
Contexte : l'étape 2 n'est pas faite (coffre 0085) ; la priorité absolue reste la fluidité (`docs/BRIEF.md:10,88`).
Décision : chaque tâche de `render/ input/ tools/ ui/` joint le JSON du banc hors écran (résultat) ; en fin de lot, banc complet D3D11 plein écran (jeton `ecran`) : tout critère tenu par la référence doit l'être encore, sinon tâche de correction avant clôture. Référence du lot 1 : `docs/mesures/2026-10-13-prototype-mesure.md` ; la mesure de fin de lot 1 (nouvelle interface autour de la page), commitée dans `docs/mesures/`, est la référence des lots suivants ; après le lot 10, « figements » et « reprises » y entrent.
Conséquences : une régression est vue au plus tard en fin de lot ; le banc continue de charger la vraie interface (coffre 0079) avec la page générée.
Réversible : la règle de refus du BRIEF remplace cette porte dès la décision go / no-go.

**DA8 — Captures et scénarios d'interface joués par l'agent.**
Contexte : le profil n'a pas de preuve visuelle automatique (`.claude/chantier.md:38-42`) ; la souris de Jérôme n'est jamais pilotée ; les dialogues natifs ne reçoivent pas d'événements postés (coffre 0084).
Décision : exécutable `actcraft-captures` (dans `bench/`, réutilise l'injecteur d'événements) : scénarios nommés (clics, touches, stylet, molette, gestes, commandes avec arguments), `grabWindow` à des étapes données, contrôles de pixels, PNG et `index.md` sous `.tmp/` ou `build/` ; dialogues Qt Quick non natifs forcés ; `scripts/captures.ps1 -Scenario <nom>|tous [-Rendu d3d11|logiciel] [-HorsEcran] -Sortie <dossier>`.
Conséquences : scénarios d'acceptation rejouables en CI (hors écran) et à l'écran ; les captures de la porte de fin sont produites, pas faites à la main.
Réversible : un pilote natif peut s'ajouter derrière la même interface de scénario.

**DA9 — Chaîne de paquets sans publication implicite.**
Contexte : profil (release = action finale) ; demande de `.exe` et `.apk`.
Décision : `scripts/paquet.ps1` (installateur Inno Setup, local) et `scripts/paquet-android.ps1` ; CI de PR : construction de l'installateur + installation silencieuse + lancement, APK `arm64-v8a` et `x86_64` + émulateur, sans téléversement ; `release.yml` sur tag `v*` crée un brouillon, échoue sans secrets de signature ; publier le brouillon est couvert par le go (décision 38).
Conséquences : les paquets sont prouvés à chaque PR ; rien n'est téléchargeable avant un go.
Réversible : MSI ou `.aab` peuvent s'ajouter au même flux.

**DA10 — Plateformes isolées par capacités et options de construction.**
Contexte : code Windows seulement dans `app/renderbackend.cpp` et `bench/` ; Android n'a ni bureau ni capture d'écran simple ; la reconnaissance d'écriture de la référence utilise le moteur de Windows.
Décision : chaque commande déclare ses plateformes ; les fonctions de bureau (caméra, annotation, outils de bureau, clavier à l'écran) sont exclues de la construction Android, la reconnaissance d'écriture n'est construite que sous Windows ; `#ifdef` limités à `app/` et aux fichiers `*_win`, `*_mac`, `*_android`.
Conséquences : un seul arbre QML ; menus Android plus courts ; la CI construit les trois cibles.
Réversible : une fonction portée plus tard ne change que sa déclaration de plateformes.

**DA11 — Versement au cache sans redessin complet.**
Contexte : 1 figement sur 10 à 64,4 ms au passage encre → cache (`docs/mesures/2026-10-13-prototype-mesure.md`) ; coffre 0081.
Décision : les objets versés depuis le dernier redessin restent des nœuds au-dessus du cache et le cache est redessiné hors de l'écriture (inactivité, nombre d'objets, fin de zoom) ; si la reprise d'un trait pendant ce redessin dépasse sa limite, passage à un cache par tuiles qui ne redessine que les tuiles touchées. Le banc gagne un scénario à pauses longues pour que le redessin différé soit mesuré.
Conséquences : critère d'acceptation : 0 figement et 0 reprise hors limite, trois passages D3D11 plein écran consécutifs.
Réversible : l'élément « page fixe » est seul concerné (coffre 0081).

**DA12 — Module d'export après `render/`.**
Contexte : `docs/BRIEF.md:52` range « PDF, images » dans `formats/`, mais l'export a besoin de QtGui (`QPdfWriter`, `QImage`) et du peintre de `render/` (DA5), alors que `formats/` reste sans QtGui (DA4) et avant `render/` dans l'ordre des modules.
Décision : module `export/` (`actcraft_export`), déclaré après `render/` : export de page en image, PDF, « Imprimer » par PDF, import PDF (Qt PDF) ; la tâche qui le crée amende `docs/BRIEF.md` § 4.
Conséquences : `formats/` reste testable sans interface ; l'export réutilise exactement le rendu logiciel.
Réversible : le module se scinde ou se fond sans toucher aux appelants (commandes `export.*`).

## 5. Scénarios d'acceptation

Joués par l'agent sans souris réelle : événements injectés et commandes appelées par `actcraft-captures` (DA8),
ligne de commande, installation silencieuse, émulateur Android. `<samples>` =
`C:\Users\JeromeDenis\REA - Activinspire\samples` (lecture seule). Toutes les sorties vont sous `.tmp/` ou `build/`.

- **S1 — Installateur (lot 1).** `pwsh -NoProfile -File ./scripts/paquet.ps1` produit
  `build/paquet/ActCraft-0.1.0-setup.exe` ; `ActCraft-0.1.0-setup.exe /VERYSILENT /CURRENTUSER /DIR=<temp>` installe
  sans droits d'administrateur ; `<temp>\actcraft.exe --version` affiche `0.1.0` ; lancé avec `--quitter-apres 5000`,
  le processus (suivi par son PID) a chargé `vcruntime140.dll`, `vcruntime140_1.dll` et `msvcp140.dll` **depuis
  `<temp>`** (liste des modules du processus) et sort à 0 ; la désinstallation silencieuse vide le dossier. La CI de
  la PR joue la même chose sur `windows-latest`.
- **S2 — Premiers pas (lot 1).** `scripts/captures.ps1 -Scenario premiers-pas -Rendu logiciel -HorsEcran` : la fenêtre
  montre menus, palette et page ; le scénario choisit le rouge et l'épaisseur 8 par leurs commandes, trace un trait au
  stylet injecté, passe à la page suivante (l'indicateur dit « 2 / 2 »), revient : le trait rouge est toujours là
  (contrôle de pixels) ; F5 bascule le plein écran. Rejoué à l'écran en D3D11 en fin de lot.
- **S3 — APK (lot 1).** La CI produit `ActCraft-0.1.0-arm64-v8a.apk` et `…-x86_64.apk` ; sur l'émulateur (local ou
  CI, décision 27), `adb install`, `adb shell am start`, le processus vit encore après 10 s,
  `adb exec-out screencap -p` montre la palette (contrôle de pixels) ; `adb shell input swipe` d'un doigt laisse un
  trait visible sur la capture suivante.
- **S4 — Ouvrir un `.flipchart` (lot 2).** Localement, le test d'oracle passe sur les 17 paperboards de `<samples>`
  (124 pages) ; `actcraft.exe "<samples>\tri des formes.flipchart"` ouvre le document, l'indicateur montre le bon
  nombre de pages, et le rendu de chaque page, réduit à 80 × 60, est comparé à sa miniature `.thm` d'origine :
  **écart médian ≤ 2,0 sur 255**, et au plus les 3 pages déjà connues au-dessus de 7 (deux à miniature présumée
  périmée, `T'Choupi` p5), comme la référence (`docs/re/format-flipchart.md:264-265`). En CI, les paperboards
  synthétiques passent.
- **S5 — Aller-retour natif (lot 2).** Par les commandes `file.save-as <chemin>` et `file.open <chemin>` (sans
  dialogue) : un document de 3 pages (trait, forme, texte, image) enregistré en `.actcraft`, fermé, rouvert : même
  empreinte du modèle ; l'archive contient `document.json`, `resources/`, `thumbnails/` ; l'enregistrement s'est fait
  hors du fil principal (aucune image > 25 ms pendant l'enregistrement, relevé par la sonde).
- **S6 — Écrire, gommer, annuler (lot 3).** Deux traits ; la gomme coupe le milieu du premier : 3 objets ; un
  rectangle de sélection prend les deux morceaux ; glisser les déplace ; trois Annuler rendent les 2 traits
  d'origine à leur place ; Rétablir rejoue ; un pincement à deux doigts injecté zoome autour de son centre.
- **S7 — Formes, texte, pages, grille (lot 4).** Tracer un rectangle, poser un texte « Bonjour » par touches
  injectées, la miniature de la page se met à jour ; glisser la page 3 avant la page 1 dans le navigateur ; grille
  aimantée : le rectangle déplacé pose son coin sur une intersection.
- **S8 — Édition et export (lot 5).** Copier un groupe vers un deuxième onglet, l'aligner à gauche, annuler ;
  `export.pdf <chemin>` sur 3 pages : le fichier compte 3 pages ; `export.page-image <chemin>` donne un PNG de
  1024 × 768.
- **S9 — Réglages gardés (lot 6).** Ancrer la palette à gauche, choisir une couleur personnalisée, ouvrir le
  navigateur d'objet, quitter, relancer : tout est restauré ; une largeur de contour changée dans le navigateur de
  propriétés s'annule.
- **S10 — Présentation (lot 7).** Le rideau couvre la page, un glisser depuis le haut la découvre ; un trait au stylo
  le long de la règle est droit ; un compte à rebours de 3 s se termine en passant à la page suivante ; sous
  Windows, la commande de reconnaissance d'écriture existe, et elle est absente du menu de l'APK.
- **S11 — Interactivité (lot 8).** Localement, `<samples>\présences.flipchart` : un clic sur un rond rouge masque
  puis réaffiche l'étiquette visée ; un objet déposé hors de son conteneur revient à sa place.
- **S12 — Confrontation au corpus (lot 9).** Chaque fiche du corpus a une ligne dans le rapport du lot (écart trouvé
  ou non, corrigé, ou hypothèse pour Jérôme) ; chaque écart corrigé a son scénario de captures ;
  `git grep` sur le dépôt ne trouve aucun titre de vidéo, aucune citation ni aucune capture issue du corpus.
- **S13 — Plus de figement (lot 10).** Trois passages `scripts/bench.ps1` (D3D11, plein écran, cache) : 0 figement et
  0 reprise hors limite sur tous les scénarios, dont celui à pauses longues ; résumé dans
  `docs/mesures/<date>-cache-incremental.md`.
- **S14 — Release prête (lot 11).** `release.yml` en essai à blanc produit `ActCraft-1.0.0-setup.exe`,
  `ActCraft-1.0.0-arm64-v8a.apk`, `SHA256SUMS.txt` et les notes tirées du `CHANGELOG.md`, sans créer de release ;
  `apksigner verify --print-certs` montre le certificat de débogage et le paquet est marqué « non publiable ». Lancé
  sur un tag sans secrets de signature, le workflow échoue avec son message. Une fois la clé créée (au go),
  l'empreinte SHA-256 du certificat de l'APK égale celle de la clé de release notée dans la procédure. Le tag et la
  publication attendent le go écrit de Jérôme.

## 6. Hors périmètre

**Exigences exclues (25), avec la raison :**

- Vote et boîtiers Promethean : F-126, F-127, F-1000, F-1010, F-1020, F-1030 (matériel propriétaire ; le
  navigateur de votes n'apparaît pas).
- Mode à deux utilisateurs : F-110, F-303 (deux pointeurs ou tactile multipoint, `backlog.md:153` : en conflit avec
  la décision 25, où deux doigts font défiler et zoomer ; absent de la référence).
- Imports de formats propriétaires : F-661 (PowerPoint), F-662 (SMART), F-663 (ExamView, QTI), F-665 et F-814
  (packs de ressources `.as2`) ; et dans F-655, les anciens `.flp`.
- Enregistreurs d'écran et de son : F-292, F-293 (capture et codecs, absents de la référence).
- Vérificateur orthographique F-297, verrouillage du formateur F-298, téléimprimante F-291 (absents de la
  référence).
- HTML incorporé F-302 (moteur web) ; texte vertical, équation, extension de page F-301.
- Publier F-680 (restrictions propres au `.flipchart`, que nous n'écrivons pas).
- Boutons définis par l'utilisateur F-920, F-142 (lancement de programmes ; impossible sous Android).
- Navigateur Web F-299 et liens vers les services en ligne de Promethean (dans F-760, F-090, F-930).
- Cadres de page F-603 (comportement non élucidé).

**Autres exclusions :** écriture de `.flipchart` ; boîte d'impression ; mise à jour automatique ; paquet macOS et
`.aab` / Play Store ; armv7 ; signature de code ; Linux et iOS ; encre à faible latence native et prédiction du
stylet ; reconnaissance d'écriture hors Windows ; mesures sur Dell 3510 et MacBook M1 (étape 2, à faire par
Jérôme) ; toute ressource de Promethean (icônes, textes de l'aide, données extraites des binaires, ressources
partagées d'origine) ; tout contenu des vidéos du corpus (captures, transcriptions, citations).

## 7. Portes immédiates

Aucune aujourd'hui. Une seule porte conditionnelle : **installation**, si la première tâche Android montre que ni
l'émulateur local (WHPX sans droits d'administrateur) ni l'émulateur de la CI (KVM sur `ubuntu-latest`) ne
fonctionne ; activer WHPX demande les droits d'administrateur et un redémarrage, que seul Jérôme peut faire.
Tout le reste est installable par l'agent (JDK 17, SDK et NDK Android, Qt pour Android, Inno Setup, module Qt PDF)
ou se fait dans la CI gratuite d'un dépôt public. Les points qui coûtent ou engagent (signature de code, clé de
release Android, publication) n'arrivent qu'à la porte de release.
