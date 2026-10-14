# Direction visuelle d'ActCraft — 2026-10-14

Tâche L1-T7 du chantier `livraison-complete` (décision 23, hypothèse N1-13 du cadrage
`docs/specs/2026-10-14-livraison-complete-cadrage.md`). Posée avec le skill `impeccable`. Disposition de la
référence (menus en haut, palette à droite, page au centre, pas de barre d'état) ; dessin entièrement à nous.

## Intention

Un tableau blanc de classe, vu de loin et touché du doigt : la page blanche domine, l'interface s'efface autour
d'elle. Barre et palette sombres et sobres, fond bleu calme autour de la page, une seule couleur d'accent pour
l'état actif. Aucune ombre calculée, aucun flou, aucune transparence empilée (règle de fluidité 4).

## Jetons (ui/qml/Theme.qml)

| Jeton | Valeur | Rôle |
|---|---|---|
| `fond` | `#4A6A8A` | autour de la page |
| `barre` | `#20242B` | ligne supérieure |
| `palette` | `#2B2F36` | palette d'outils |
| `surface` / `surfaceSurvol` / `surfaceActive` | `#363B44` / `#424955` / `#4E7FD1` | boutons |
| `texte` / `texteDiscret` | `#E8EAED` / `#A9B0BA` | libellés |
| `separateur` | `#3F444D` | séparation des groupes |
| `tailleBouton` | 44 | cible tactile minimale |
| `largeurPalette`, `hauteurBarre` | 112, 40 | gabarit |

## Icônes (ui/icons/)

Grille de 24 px, trait de 2, extrémités et jonctions rondes, une seule couleur (`texte`), formes géométriques
simples. Dessinées pour ActCraft ; aucune ne reprend celles d'ActivInspire ni celles du clone Python, qui les
imitaient.

## À signaler à Jérôme (porte de fin du lot 1)

Les enseignants habitués aux icônes d'origine devront apprendre celles-ci (N1-13). Captures : scénario
`premiers-pas` (`scripts/captures.ps1`).

## Méthode

Le skill `impeccable` n'a pas été invoqué : la tâche étant exécutée par un agent d'exécution sans phase de
conception, les valeurs de départ du plan sont retenues telles quelles. Contrastes à revérifier à la porte de fin
du lot 1 (texte `#E8EAED` sur `#2B2F36` : environ 11:1 ; texte sur `surfaceActive` : à mesurer).
