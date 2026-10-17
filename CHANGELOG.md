# Journal des versions

Les versions d'ActCraft, de la plus récente à la plus ancienne. Une section par version, rédigée à la clôture de
chaque lot (décision 31 du cadrage `docs/specs/2026-10-14-livraison-complete-cadrage.md`) ; la section sert de notes
à la release correspondante.

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
- Menu Fichier : Nouveau paperboard (taille de l'écran, 1024 × 768, 1152 × 864, 1280 × 1024 ou taille
  personnalisée), Ouvrir, Enregistrer, Enregistrer sous, Fermer le paperboard et Quitter.
- « Enregistrer les modifications ? » avant Nouveau, Ouvrir, Fermer et Quitter quand le paperboard a changé
  depuis son dernier enregistrement.
- Bandeau d'avertissements : ce qui a été écarté ou remplacé à la lecture d'un fichier est signalé en haut de la
  fenêtre.
- La page prend la taille de la page affichée ; le changement de page n'affiche plus d'éclair blanc pendant que
  les images se décodent.
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
