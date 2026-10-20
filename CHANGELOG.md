# Journal des versions

Les versions d'ActCraft, de la plus récente à la plus ancienne. Une section par version, rédigée à la clôture de
chaque lot (décision 31 du cadrage `docs/specs/2026-10-14-livraison-complete-cadrage.md`) ; la section sert de notes
à la release correspondante.

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
