# Lot 7 — principes confrontés (vidéos, spécification, ancien dépôt, binaire)

Relevé brut (hors dépôt) : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot7.md`. Références `Vnn` :
numéros des fiches du corpus (table hors dépôt). Constats écrits par nous. Base : N0 = <à reporter par
l'orchestrateur> tests ctest. Tâche L7-T0, scénario S30.

## Méthode et limites

- **Corpus.** Synthèse du corpus et fiches des fonctions du lot ; 23 constats (outils 11, interface 4, texte et
  reconnaissance 4, sélection 2, pages 2).
- **Binaire.** En statique seulement : réglages par défaut embarqués dans l'exécutable, traductions françaises,
  chaînes et symboles. Le serveur d'analyse habituel n'a pas répondu ; aucun désassemblage, aucune observation à
  l'écran. Ce qui l'exigerait est marqué « non tranché » : la spécification s'applique.
- **Ancien dépôt.** Spécification et documentation de rétro-ingénierie lues. Requêtes du graphe et relecture du code
  Python des outils **non faites** (limite de la tâche) : les six constats du plan restent non revérifiés.

## Écarts tranchés

| # | Sujet | Valeur retenue pour ActCraft | Source (statique) | Tâche |
|---|---|---|---|---|
| E5 | Nombre de dés à l'ouverture | **1 dé** au premier lancement, puis le dernier nombre choisi (mémorisé dans les réglages) ; bornes 1 à 5, vitesse 3 à 11 s, défaut 7 | réglage par défaut embarqué : 1, bornes 1 à 5 ; la fenêtre réécrit le réglage à chaque changement (déduit) ; V127 (quatre dés, compatible avec une valeur mémorisée). Le défaut 5 de la spécification est le maximum. Reste inconnu : décalage entre rang de la liste et nombre | L7-T14 |
| E9 | Actions de l'horloge en fin de délai | **non tranché : spécification appliquée** (aucune action, aller à la page d'après, à la page d'avant, remettre la page dans son état enregistré) | la liste est construite par le code (une action = une chaîne, supposé : un nom de commande) ; aucune ressource ne la liste ; rien ne relie le rideau à cette liste (V83 ambiguë) | L7-T16 |
| E11 | Validation du message défilant | **non tranché : spécification appliquée** (un bouton d'essai, et un bouton qui garde le message comme défaut et ferme la boîte) | les boutons de la boîte ne sont pas lisibles en statique ; V131 entend un autre libellé, sans effet sur le comportement | L7-T19 |

## Effets de page retenus (N2-A6)

17 mouvements relevés dans le binaire, plus « aucun » et « au hasard ». Identifiants et noms à nous ; l'effet à
l'image d'une marque n'est pas repris.

| Identifiant | Nom (à nous) | Mouvement | Tâche |
|---|---|---|---|
| `none` | Sans effet | changement immédiat (défaut) | L7-T7 |
| `random` | Au hasard | un effet du catalogue tiré à chaque changement | L7-T7 |
| `dissolve` | Fondu | l'ancienne page s'efface dans la nouvelle | L7-T7 |
| `corner-peel` | Coin soulevé | la page se décolle depuis un coin | L7-T7 |
| `box-turn` | Boîte qui tourne | les deux pages sur deux faces d'un volume en rotation | L7-T7 |
| `card-flip` | Carte retournée | la page pivote sur son axe vertical | L7-T7 |
| `tiles-spin` | Carreaux | la page se découpe en carrés qui pivotent | L7-T7 |
| `fold-half` | Pliage | la page se plie en deux | L7-T7 |
| `slide-right` | Glissement | la nouvelle page pousse l'ancienne vers la droite | L7-T7 |
| `strips-drop` | Bandes | la page tombe en bandes verticales | L7-T7 |
| `corner-swing` | Pivot d'angle | la page tourne autour d'un angle | L7-T7 |
| `spiral-out` | Tourbillon | la page tourne en s'éloignant | L7-T7 |
| `turn-reveal` | Dévoilement tournant | l'ancienne page pivote et découvre la nouvelle | L7-T7 |
| `drapes` | Tentures | deux pans s'écartent vers les bords | L7-T7 |
| `star-iris` | Étoile | la nouvelle page apparaît dans une étoile qui grandit | L7-T7 |
| `shards` | Éclats | l'ancienne page se brise en morceaux | L7-T7 |
| `sweep` | Balayage | un front droit découvre la nouvelle page | L7-T7 |
| `funnel` | Entonnoir | l'ancienne page se resserre vers un point du bas | L7-T7 |

Comportement : durée 5 à 30 dixièmes de seconde, défaut 15 (confirmé) ; effet propre à la page prioritaire sur
l'effet global (V45, confirmé) ; moment du jeu (en quittant la page) et cas de désactivation : non lisibles en
statique, spécification appliquée.

## Modèles d'équation retenus

Représentation des équations des `.flipchart` : **MathML** dans une chaîne, accompagné d'une image de rendu (binaire :
prise en charge MathML de la bibliothèque d'équations). Confirme la réservation de L7-T1 (MathML brut gardé).

| Identifiant | Nom (à nous) | Forme MathML écrite | Tâche |
|---|---|---|---|
| `fraction` | Fraction | `<mfrac><mrow/><mrow/></mfrac>` | L7-T24 à L7-T27 |
| `power` | Puissance | `<msup><mrow/><mrow/></msup>` | L7-T24 à L7-T27 |
| `subscript` | Indice | `<msub><mrow/><mrow/></msub>` | L7-T24 à L7-T27 |
| `square-root` | Racine carrée | `<msqrt><mrow/></msqrt>` | L7-T24 à L7-T27 |
| `nth-root` | Racine n-ième | `<mroot><mrow/><mrow/></mroot>` | L7-T24 à L7-T27 |
| `parentheses` | Parenthèses | `<mrow><mo>(</mo><mrow/><mo>)</mo></mrow>` | L7-T24 à L7-T27 |
| `sum` | Somme | `<munderover><mo>∑</mo><mrow/><mrow/></munderover><mrow/>` | L7-T24 à L7-T27 |
| `integral` | Intégrale | `<msubsup><mo>∫</mo><mrow/><mrow/></msubsup><mrow/>` | L7-T24 à L7-T27 |
| `matrix-2x2` | Matrice 2 × 2 | `<mrow><mo>(</mo><mtable><mtr><mtd/><mtd/></mtr><mtr><mtd/><mtd/></mtr></mtable><mo>)</mo></mrow>` | L7-T24 à L7-T27 |
| `greek` | Lettres grecques | `<mi>α</mi>` … `<mi>ω</mi>`, `<mi>Δ</mi>`, `<mi>Ω</mi>` | L7-T24 à L7-T27 |

Palettes de l'éditeur d'origine : non relevées une à une ; le jeu ci-dessus est le nôtre.

## Principes

| # | Principe (nos mots) | Source | Exigence pour ActCraft | Tâche | Scénario | Écart, raison ou lot destinataire |
|---|---|---|---|---|---|---|
| P1 | La calculatrice montre le calcul en cours au-dessus du résultat. | V37, V135 | deux lignes : expression, résultat | L7-T15 | S10 | à vérifier dans la section de L7-T15 |
| P2 | Deux effacements : la dernière saisie ou tout le calcul. | V135 | deux touches d'effacement distinctes | L7-T15 | S10 | à vérifier dans la section de L7-T15 |
| P3 | Le calcul et son résultat passent sur la page, par un bouton ou en le glissant. | V37, V135 | commande qui écrit le calcul sur la page (un texte, annulable) ; glisser vers la page | L7-T15 | S10 | **section à corriger** : transfert sur la page ; glisser hors de la fenêtre (proposé : bouton seul en 7a, glisser en 7b) |
| P4 | Les fenêtres de la calculatrice et des dés se ferment par leur croix. | V37, V127 | bouton de fermeture sur chaque panneau | L7-T14, L7-T15 | S10 | — |
| P5 | Le stylo qui approche du porte-mine du compas fait apparaître une mine de sa couleur. | V128 | indication visuelle de l'accrochage, à la couleur du stylo | L7-T12 | S10 | — |
| P6 | La règle se gradue en millimètres, centimètres ou pouces ; fermer un outil laisse les traits tracés. | V110 | trois unités ; fermer l'outil ne retire aucun trait | L7-T9 | S10 | proposition : test « fermer la règle garde les traits » |
| P7 | Sur le spot, le curseur indique le redimensionnement au bord et le déplacement à l'intérieur. | V108 | curseurs selon la zone | L7-T4 | S10 | — |
| P8 | Le menu du spot change de forme, capture la zone éclairée, ferme. | V108 | menu du spot : forme, capture vers la page, fermeture | L7-T4 | S10 | capture de la zone : **section à corriger** (L7-T4) ou 7b avec la caméra |
| P9 | Le rideau peut mémoriser sa position pour la page. | V101 | commande qui enregistre l'état du rideau dans les réglages de la page | L7-T6 | S10 | — |
| P10 | Sans boucle, le message défile une seule fois. | V131 | passage unique puis masquage | L7-T18 | S23 | confirmé (`singlePassEndsHidden`) |
| P11 | La zone de capture partielle reste en place d'une capture à l'autre. | V48 (déduit) | position de la zone gardée pendant la session | 7b (caméra) | S10 | — |
| P12 | Sur le bureau, les annotations restent fixes quand l'application dessous défile. | V97 | aucune | 7b (bureau) | S10 | conséquence de l'architecture, sans suivi des fenêtres |
| P13 | Sur le bureau, passer à la page d'après donne une feuille transparente vierge. | V97 | passer à la page d'après donne une nouvelle page transparente | 7b (bureau) | S10 | — |
| P14 | Le paperboard du bureau est conservé d'une activation à l'autre. | V97 | contenu gardé pendant la session | 7b (bureau) | S10 | — |
| P15 | Un onglet propre au paperboard du bureau y ramène. | V97 | retour au bureau depuis la liste des paperboards ouverts | 7b (bureau) | S10 | dépend des onglets de paperboards (lot 2) |
| P16 | La reconnaissance de forme distingue triangle, cercle, ellipse et polygone ; un rectangle approximatif devient un polygone. | V107 | classe « polygone » à ajouter ; rectangle seulement si les angles sont droits | L7-T20 | S10 | **section à corriger** : le plan dit ligne, rectangle, ellipse, triangle (constat 3) |
| P17 | Plusieurs mots écrits à la suite sont reconnus séparément. | V116 | un objet texte par mot (coupure à la pause) | L7-T22, L7-T23 | S10 | — |
| P18 | Un point manuscrit devient un signe de ponctuation. | V116 | ponctuation rendue par le moteur du système | L7-T22 | S10 | dépend du moteur d'encre |
| P19 | Un clic sur la page valide le texte reconnu. | V116 | clic hors du texte = validation | L7-T23 | S10 | — |
| P20 | Avec l'origine active, les symétries se font autour d'un axe qui passe par elle. | V36 | réflexion autour de l'origine | L7-T34 | S10 | attend le lot 3 et les symétries du lot 5 : **section à corriger** (L7-T34) |
| P21 | Sur un outil de maths, le curseur change selon la zone (déplacer, tourner, redimensionner) ; le rapporteur tourne. | V34 | curseurs par zone ; rotation du rapporteur | L7-T9 à L7-T11 | S10 | — |
| P22 | La superposition du bureau ne vaut que pour sa page. | V68 | fond transparent sur le bureau limité à la page qui le porte | 7b (bureau) | S10 | F-071 |
| P23 | La sélection agit sur le bureau à travers une page en superposition. | V68 | entrées transmises au bureau hors des objets | 7b (bureau) | S10 | F-071 ; Windows seulement |

## Écarts pour l'orchestrateur

À verser au plan avant V3 :
- L7-T14 : défaut 1 dé au lieu de 5, nombre et vitesse mémorisés (E5).
- L7-T15 : transfert du calcul sur la page (P3), deux effacements (P2), affichage du calcul (P1).
- L7-T4 : capture de la zone éclairée (P8) ; largeur du contour par défaut 10 et non 15 (réglage embarqué).
- L7-T9 à L7-T13 : translucidité des outils par défaut 100 et non 255 (réglage embarqué).
- L7-T20 : classe polygone (P16). L7-T34 : symétries autour de l'origine (P20).
- 7b, reconnaissance : délai par défaut 1 s, bornes 0 à 5 (réglage embarqué) contre 5 s, 1 à 5 (spécification).
Vers d'autres lots : moteur des effets par défaut logiciel (lot 6, réglages) ; onglet du bureau (lot 2).
