# Épinglages du lot 8 — profil « Sons et images »

Règle (plan 8a, amendement 1, § 1) : le lot qui livre un outil épinglé par un profil complète ce profil. Le lot 8
complète `actions/data/profiles/son-image.json`. Les identifiants sont ceux déclarés dans
`actions/data/commands.json` ; chacun est filtré par plateforme à l'affichage (D6-2).

| Commande | Pourquoi elle est épinglée | Plateformes |
|---|---|---|
| `media.sound-controller` | ouvrir le lecteur de sons depuis la barre, sans passer par un menu | toutes |
| `record.sound` | lancer un enregistrement de voix, geste central d'un profil consacré au son | toutes |
| `record.screen-full` | enregistrer tout l'écran pour une démonstration | Windows seulement |
| `record.screen-area` | enregistrer une zone choisie de l'écran | Windows seulement |
| `insert.web-frame` | poser une page web vivante dans la page, au même titre qu'une image | toutes |
| `link.file` | lier un objet à un fichier (son, vidéo, document) | toutes |
| `link.website` | lier un objet à une adresse web | toutes |

Sous Android, `record.screen-full` et `record.screen-area` sont filtrés : la palette n'affiche que cinq des sept
ajouts, aucun bouton mort.

L'ordre dans le profil suit l'ordre ci-dessus, après les épinglages du lot 6 (appareils photo, `insert.image`).

## Épinglages du lot 7

Le lot 7 n'est pas fusionné sur cette branche (H8-7) : ses outils (rideau, projecteur, outils de maths, ouvreur du
navigateur web) et son rapport d'épinglages n'existent pas ici. Rien du lot 7 n'est épinglé par cette tâche.
À raccorder à la fusion du lot 7 : joindre ses épinglages à ce rapport (même forme) et revérifier
`pinnedListsAreTheAgreedOnes` si le lot 7 touche `son-image`.
