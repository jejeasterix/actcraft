# Lot 4 — observation de l'original à l'écran (L4-O1)

Séance du 2026-10-21 sur la version 3.5.18 de l'original, écran principal 1920 × 1080, fenêtre maximisée, paperboard
neuf jamais enregistré, vraies entrées souris et clavier rejouées par l'outillage de l'ancien dépôt. Cinq points que
la lecture statique ne tranchait pas (D5, D6, D10, D11, D17 de `2026-10-18-lot4-principes.md`). Le relevé brut et les
captures restent hors du dépôt, avec l'ancien dépôt.

## Verdicts

| Point | Geste joué | Constat | Verdict | Tâches à amender |
|---|---|---|---|---|
| D5 — Maj pendant le tracé | outil des formes : rectangle glissé sans Maj (témoin), Maj tenue jusqu'au relâché, Maj relâchée bouton tenu ; ligne glissée à 35° sans Maj puis Maj tenue | les trois rectangles sortent identiques, 200 × 60 px, pendant le glisser comme au relâché ; les deux lignes gardent 35° | **contredit** : Maj ne garde pas de carré et ne contraint pas la ligne à 45° | L4-T8 (`shiftKeepsSquare`, `shiftReleaseFreesTheRatio`), L4-T11 |
| D6 — grille invisible | grille visible, accrochage autorisé ; rectangle posé puis déplacé de (+37, +23) ; grille rendue invisible (accrochage laissé autorisé) ; déplacé de (+41, +19) | grille visible : le rectangle tombe sur les lignes (pas de 32 unités) ; grille invisible : il avance d'un pas en x et revient sur la même ligne en y, les 19 px sont perdus | **confirmé** : l'accrochage continue quand la grille est cachée | aucune |
| D10 — règle de l'allongement | page de 1024 × 768 ; poignée d'allongement montrée ; trois clics sur la poignée | hauteur lue dans les propriétés de la page : **1 022,21** ; mesurée sur la capture : 967 × 966 px, soit **1022,9 ± 1,1** | **contredit** : pas cumulé (×1,1 par clic), pas le pas fixe de 10 % de la hauteur d'origine (998,4) | L4-T4 (`extendStepsByTenPercentOfBase` : correctif), L4-T26 (attendus 1022,208 au lieu de 998,4) |
| D11 — dessin des cadres | sur une page, cadres réglés à 2 en largeur et 3 en hauteur dans les propriétés de la page | largeur et hauteur inchangées ; aucune séparation dessinée sur la page en édition ; valeurs par défaut d'une page neuve : 0 et 0 | **en partie observé** : pas de dessin des séparations en édition, et le nombre de cadres ne change pas la taille de la page. **Non observé** : l'aspect en présentation (capturé, pas dépouillé), l'effet sur un objet déplacé et sur l'accrochage, la taille de page obtenue par la boîte de taille personnalisée | L4-T28 (contrat du dessin : rien sur la page en édition) ; L4-T27 et L4-C1 en attente d'une seconde séance |
| D17 — encre en cours et grille du dessus | grille visible et au-dessus des objets ; stylo rouge épais, trait lent ; capture bouton tenu puis après le relâché | pendant le tracé, le rouge passe sur les lignes de la grille ; après le relâché, image identique au pixel (aucun pixel rouge recouvert) | **contredit** : la grille du dessus ne couvre pas le trait en cours ; « rien ne change au relâché » est confirmé | L4-T2 (`socleLayerOrder` : encre en cours au-dessus de `gridOverlay`), L4-T7 |

## Réserves

- **D5** : la touche Maj a été injectée par le système (même voie que la souris). Un essai au clavier physique
  lèverait le doute ; la constatation vaut pour les deux formes et pour les deux moments (glisser, relâché).
- **D10** : pendant les clics, le panneau des propriétés n'a pas rafraîchi sa hauteur (768 affiché) et la vue a gardé
  son zoom ; la valeur 1 022,21 est lue après rafraîchissement. 768 × 1,1³ = 1022,208 ; le pas fixe aurait donné une
  page de 943 px de haut à ce zoom, l'écart est hors de la précision de la mesure.
- **D17** : le constat vaut aussi pour le trait fini : après le relâché le trait reste au-dessus de la grille du
  dessus. L'ordre des couches de la grille du dessus par rapport à l'encre posée n'est pas l'objet de cette séance.
- **Profil de l'original** : relevé en lecture seule avant et après (empreinte de chaque fichier). Il a changé :
  271 fichiers supprimés, 244 ajoutés, 44 modifiés, tous dans le magasin de réglages de l'utilisateur, renumérotés à
  chaque lancement (cinq lancements, chacun fermé de force sans enregistrer). Aucun réglage n'a été changé
  volontairement ; rien n'a été corrigé. À trancher par Jérôme.

## Suite

Une seconde séance courte, sous le même jeton, couvrirait le reste de D11 : présentation, objet déplacé sur une page
à cadres, accrochage, et taille de page de la boîte de taille personnalisée pour 2 × 3 cadres de 400 × 300.
