# Lot 7 — commandes proposées à l'épinglage des profils (pour le lot 6)

Tâche L7-T32. Aucune source ne décrit de profil fourni pour le lot 7 (L7-T0 et F-880 n'en fixent pas la
composition) : les listes ci-dessous sont **proposées** par ce rapport, avec leur raison. Les identifiants sont
ceux de `actions/data/commands.json` (stables ; le lot 6 les épingle par identifiant, jamais par libellé).

## Profil « mathématiques »

| Commande | Raison |
|---|---|
| `math.ruler`, `math.set-square`, `math.protractor`, `math.compass` | les quatre instruments de tracé |
| `math.xy-origin` | repère cartésien posé sur la page |
| `tool.dice` | probabilités, tirages |
| `tool.calculator` | calcul (le résultat se glisse sur la page) |
| `insert.equation` | écriture d'équations à modèles |
| `tool.shape-recognition` | formes tracées à main levée redressées |

## Profil « au tableau »

| Commande | Raison |
|---|---|
| `present.curtain` | cacher puis découvrir la page |
| `present.spotlight-circle` | attirer l'attention sur une zone |
| `view.page-zoom` | agrandir une partie de la page |
| `view.page-effects` | choix de l'effet de changement de page |
| `tool.clock` | horloge et compte à rebours |
| `tool.ticker` | message défilant |

## Hors profils fournis (disponibles par les menus)

Commandes de bureau (`desktop.*`, `tool.keyboard`, `tool.camera-*`) : absentes d'Android, donc à ne jamais
épingler dans un profil destiné à cette plateforme (`androidMenusHaveNoDesktopCommand`).
`tool.handwriting` et `recognition.convert-to-text` : Windows seulement, grisées sans moteur de reconnaissance.
