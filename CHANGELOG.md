# Journal des versions

Les versions d'ActCraft, de la plus récente à la plus ancienne. Une section par version, rédigée à la clôture de
chaque lot (décision 31 du cadrage `docs/specs/2026-10-14-livraison-complete-cadrage.md`) ; la section sert de notes
à la release correspondante.

## 0.6.0 — 2026-11-04

Édition des objets et fichiers : couper, copier, coller, grouper, organiser et aligner ; les connecteurs ; le pot de
peinture ; plusieurs paperboards en onglets ; l'export en image et en PDF, l'import d'un PDF ou d'une présentation, et
l'impression. Il n'y a pas eu de version 0.5.0 publiée : ses nouveautés (formes, texte, pages, grille, page allongée ;
section 0.5.0 du journal des versions) sont aussi dans cette version.

- Édition d'objet : un menu (clic droit sur la sélection) et la barre d'objet donnent couper, copier, coller,
  dupliquer, supprimer et tout sélectionner, par le presse-papiers du système, groupes et connecteurs compris.
  Les objets se groupent, se dégroupent (groupes de groupes acceptés), passent devant ou derrière, s'alignent,
  s'ajustent, reprennent leur taille d'origine et se retournent. Chaque geste est une étape d'annulation.
- Verrouiller, masquer, glisser une copie : bascules d'objet, propagées aux membres d'un groupe.
- Pot de peinture : il remplit l'intérieur ou le contour d'une forme, le fond de la page ou une zone d'une image,
  avec une tolérance.
- Connecteur : il relie deux objets par leurs points d'attache (accrochage à 20 px) et suit les objets quand ils
  bougent. Le panneau propose 14 styles (ligne, courbes, marches, coude, oblique, pointillés, flèches) avec leur
  vignette ; chaque extrémité se règle à part (aucune, rond, flèche).
- Modification de stylo : le stylo trace aussi des lignes et des formes géométriques, gardées comme annotations. Les
  points d'une forme se déplacent un à un.
- Insérer une image ou un média : par fichier ou par glisser-déposer, décodé sans figer la fenêtre.
- Onglets : plusieurs paperboards s'ouvrent dans une barre d'onglets ; on bascule, on ferme, et on glisse des pages
  d'un onglet à l'autre.
- Export : la page en image, le paperboard en PDF, sans figer la fenêtre.
- Import : un PDF devient un paperboard, une page par page, chaque page en image verrouillée sur le fond. Une
  présentation PowerPoint s'importe en images quand PowerPoint ou LibreOffice est installé.
- Impression : une boîte règle l'imprimante, la couleur ou le gris, les pages par feuille, les cadres, les numéros,
  la plage, les copies, la marge et les notes, avec l'aperçu de la feuille ; les réglages sont gardés. Sans
  imprimante, l'impression produit le PDF des feuilles.
- Banc de mesure : nouveaux scénarios `edition`, `edition-export`, `onglets`, `export-fond`, `exporter`, `imprimer`
  et `importer`.

## 0.5.0 — 2026-11-02

Outils de la page : les formes, le texte, le navigateur de pages, le menu de page, le fond, la grille, la page
allongée et ses zones.

- Formes : une palette propose les formes (rectangle, cercle, polygones et autres préréglages), un nuancier,
  le choix « sans remplissage » et le tracé depuis le coin ou depuis le centre. La ligne et la courbe brisées se
  tracent au multi-clic, la forme à main levée d'un seul geste. Les points intelligents d'une forme se glissent pour
  la modifier, en une étape d'annulation.
- Texte : l'outil Texte ouvre un cadre d'édition et une barre de mise en forme (gras, taille, couleur et autres
  réglages) ; un appui hors du cadre valide le texte. Le texte de haut en bas s'écrit en
  colonne, un caractère par ligne, avec sa propre barre.
- Accrochage à la grille : un objet déposé se pose par son coin bas gauche, un texte par sa première ligne de base.
- Sélection : Ctrl+clic ajoute un objet à la sélection ; Maj+clic reste accepté.
- Navigateur de pages : le panneau des navigateurs (Ctrl+B) montre les miniatures des pages, rendues sans ralentir
  le tracé. On y sélectionne plusieurs pages (Ctrl+clic, Maj), on les déplace, les duplique, les copie, les colle,
  les insère et les supprime.
- Menu de page : il s'applique aux pages sélectionnées et ouvre la boîte « Fond de la page ».
- Grille : le créateur de grille règle des niveaux, le pas, la couleur et la position au-dessus ou au-dessous de
  l'encre ; la grille se masque et l'accrochage se coupe depuis le menu.
- Page allongée : une poignée et une commande allongent la page d'un cadre, une étape par clic. Les zones de page
  découpent la page en cadres, montrés par des séparations pointillées en édition et en plein écran.
- Échap ferme une boîte ouverte comme « Annuler », sans changer d'outil.
- Formats : le format natif garde les préréglages et paramètres des formes, le texte de haut en bas, la ligne de
  base des textes et la hauteur d'origine de la page ; le texte de haut en bas des paperboards `.flipchart` est lu.
- Banc de mesure : nouveaux scénarios `forme-page-annotee`, `grille-dessus`, `miniatures` et `miniatures-ferme`.

## 0.4.0 — 2026-10-20

Dessiner et corriger : sélectionner, déplacer et transformer les objets, le marqueur et la gomme, annuler et
rétablir, effacer, la corbeille, les gestes à deux doigts ; et un tracé qui ne fige plus la fenêtre. Il n'y a pas eu
de version 0.3.0 publiée : son contenu (le cache incrémental) est dans cette version.

- Sélection : un clic désigne un objet, un rectangle en désigne plusieurs ; la sélection se déplace, et des poignées
  permettent de la redimensionner. Une barre d'objet donne accès à la rotation, à la translucidité, à l'agrandissement
  et à la réduction, et au verrouillage : un objet verrouillé ne se déplace plus et n'est jamais retiré par
  « Effacer ».
- Stylo et marqueur : le marqueur trace un trait translucide qui laisse voir ce qu'il couvre.
- Gomme : elle coupe les traits qu'elle croise et n'enlève que les morceaux touchés.
- Annuler et Rétablir (Ctrl+Z, Ctrl+Y) : chaque geste est une étape, y compris une gomme, un déplacement ou un
  effacement.
- Effacer : le sous-menu retire les annotations, les objets, la grille, le fond, ou la page entière ; l'effacement
  se défait d'une étape.
- « Rétablir la page » remet la page dans l'état où le paperboard a été ouvert ou enregistré, en une seule étape.
- Corbeille : « Afficher la corbeille » ajoute une corbeille sous la palette ; on y dépose les objets pour les retirer.
- Gestes à deux doigts : pincer pour zoomer, glisser à deux doigts pour faire défiler la page.
- Les traits terminés sont versés au cache de la page sans figer l'écran : plus de saccade quand on lève le stylet
  après un tracé long.
- Direct3D 11 : les traits sont lissés (anticrénelage) et la grille reste nette.
- Banc de mesure : nouveaux scénarios `pauses` (traits séparés par des pauses), `gommer` et `soulever` (soulever
  et déplacer une sélection d'annotations).

## 0.2.0 — 2026-10-17

Ouvrir et enregistrer : le format natif `.actcraft`, la lecture des fichiers `.flipchart`, le menu Fichier, et
l'enregistrement par le sélecteur du système sur Android.

- Format natif `.actcraft` : « Enregistrer » et « Enregistrer sous » écrivent le paperboard (pages, objets, images)
  dans un seul fichier ; « Ouvrir » le relit à l'identique. L'écriture se fait par un fichier temporaire : un
  enregistrement interrompu ne détruit pas l'ancien fichier.
- Compatibilité ascendante : un fichier écrit par une version plus récente s'ouvre ; les objets, les clés et les
  entrées d'archive que cette version ne connaît pas sont gardés et réécrits tels quels à l'enregistrement. Un
  fichier d'une version majeure plus récente est refusé avec un message.
- Fichiers `.flipchart` : lecture des pages, des objets, des images et des miniatures. Ce qui ne peut pas être lu
  est écarté sans bloquer l'ouverture. Un `.flipchart` n'est jamais réécrit : « Enregistrer » propose un nom en
  `.actcraft`.
- Menu Fichier : Créer un paperboard (taille de l'écran, 1024 × 768, 1152 × 864, 1280 × 1024 ou taille
  personnalisée), Ouvrir, Enregistrer, Enregistrer sous, Fermer le paperboard et Quitter.
- « Enregistrer les modifications ? » avant Nouveau, Ouvrir, Fermer et Quitter quand le paperboard a changé
  depuis son dernier enregistrement.
- Bandeau d'avertissements : ce qui a été écarté ou remplacé à la lecture d'un fichier est signalé en haut de la
  fenêtre.
- Le changement de page n'affiche plus d'éclair blanc pendant que les images se décodent.
- Ouvrir, enregistrer et décoder les images se font hors du fil principal : la fenêtre reste fluide pendant une
  opération longue.
- Un fichier passé au lancement s'ouvre (« Ouvrir avec », association de fichiers, `--ouvrir <fichier>`).
- Android : enregistrer et ouvrir par le sélecteur de fichiers du système (adresses `content://`).
- Installateur Windows : option d'association des fichiers `.actcraft` ; les `.flipchart` s'ouvrent par « Ouvrir
  avec » sans changer l'application par défaut.
- APK Android `arm64-v8a` (et `x86_64` pour l'émulateur).

## 0.1.0 — 2026-10-15

Premier socle utilisable : une interface, le stylo en couleurs, plusieurs pages, et les deux paquets.

- Fenêtre principale : menus Fichier, Affichage, Outils et Aide ; indicateur « page n / N », échelle et plein
  écran (F5) dans la ligne supérieure ; palette d'outils à droite ; pas de barre d'état.
- Stylo : 16 couleurs, 4 épaisseurs et une réglette, indicateur de largeur ; tracé au stylet, à la souris ou d'un
  doigt.
- Pages : « Page suivante » avance d'une page, et depuis la dernière ajoute une page vierge ; « Page précédente ».
- « À propos » : version, licence MIT, notices des composants tiers ; `actcraft --version`.
- Installateur Windows sans droits d'administrateur ; APK Android (arm64-v8a).
