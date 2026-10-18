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
- **Ancien dépôt.** Spécification, documentation de rétro-ingénierie, code Python des outils
  (`app/actcraft/extras/`, `canvas/recognition.py`, `canvas/transitions.py`) et son graphe (les cinq requêtes du
  plan : elles ne renvoient que rideau, spot et effets ; outils de maths, reconnaissance et horloge ne sortent
  pas du graphe et ont été lus directement).
- **Contrôle des titres.** La commande du plan sort deux titres génériques du corpus (18 et 33 caractères), déjà
  présents sur la base dans 7 fichiers que cette tâche ne touche pas : hors périmètre, consigné au rapport de tâche.
  Le présent fichier n'en contient aucun.

## Écarts tranchés

| # | Sujet | Valeur retenue pour ActCraft | Source (statique) | Tâche |
|---|---|---|---|---|
| E5 | Nombre de dés à l'ouverture | **1 dé** au premier lancement, puis le dernier nombre choisi (mémorisé dans les réglages) ; bornes 1 à 5, vitesse 3 à 11 s, défaut 7 | réglage par défaut embarqué : 1, bornes 1 à 5 ; la fenêtre réécrit le réglage à chaque changement (déduit) ; V127 (quatre dés, compatible avec une valeur mémorisée) ; ancien dépôt : un dé à l'ouverture, observé sur l'original (`app/actcraft/extras/dice.py:18`). Le défaut 5 de la spécification est le maximum. Reste inconnu : décalage entre rang de la liste et nombre | L7-T14 |
| E9 | Actions de l'horloge en fin de délai | **non tranché en statique → hypothèse H7-14 : spécification appliquée** (aucune action, aller à la page d'après, à la page d'avant, remettre la page dans son état enregistré) | la liste est construite par le code (une action = une chaîne, supposé : un nom de commande) ; aucune ressource ne la liste ; rien ne relie le rideau à cette liste (V83 ambiguë) ; l'ancien dépôt garde les quatre actions de la spécification (`app/actcraft/extras/timer.py:17`) | L7-T16 ; **hypothèse H7-14** |
| E11 | Validation du message défilant | **non tranché en statique → hypothèse H7-15 : spécification appliquée** (un bouton d'essai, et un bouton qui garde le message comme défaut et ferme la boîte) | les boutons de la boîte ne sont pas lisibles en statique ; V131 entend un autre libellé, sans effet sur le comportement | L7-T19 ; **hypothèse H7-15** |

## Effets de page retenus (N2-A6)

17 mouvements relevés dans le binaire, plus « aucun » et « au hasard » ; **16 retenus** : l'effet à l'image d'une
marque n'est pas repris. Identifiants et noms à nous. La colonne « Mouvement » est **déduite** du nom de chaque effet
(le binaire ne décrit pas l'animation en statique) : à préciser à l'écran au plus tard à la clôture du lot (S22).

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

Représentation des équations des `.flipchart` : **MathML** dans une propriété de type chaîne de l'objet équation
(type d'objet 460, propriété 52, chaîne : `app/actcraft/io/_schema.py`, objet équation ; aucun exemplaire dans les
échantillons, `docs/re/format-flipchart.md:254`). La bibliothèque d'équations de l'original reconnaît et découpe ce
MathML (binaire). Pour l'affichage sur la page, l'éditeur calcule une image (message d'erreur de l'éditeur quand elle
serait trop grande) ; qu'elle soit enregistrée dans le fichier n'est pas établi. Confirme la réservation de L7-T1
(MathML brut gardé).

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

Palettes de l'éditeur d'origine (traduction française de l'éditeur : 180 messages, un seul contexte), relevées par
catégorie, en nos mots : fractions et divisions posées ; racines (carrée, avec indice) ; indices et exposants, y
compris à gauche ; au-dessus et au-dessous ; accents ; délimiteurs (parenthèses, crochets, accolades, chevrons,
valeur absolue, parties entières, norme) ; grands opérateurs (somme, produit, limite) ; intégrales (simple, avec
bornes, double, triple, curviligne) ; dérivées ; matrices (1 × 2, 2 × 1, 2 × 2, 3 × 3) ; fonctions
trigonométriques ; opérateurs ; relations ; ensembles ; logique ; flèches ; lettres grecques minuscules et
majuscules ; symboles divers ; espaces. Point « à élucider » de l'amendement 1 (§ 2) : jeu de modèles relevé,
représentation MathML confirmée. Le jeu ci-dessus (le minimum du plan) est le nôtre ; proposition pour L7-T24 à
L7-T27 : ajouter valeur absolue, matrices 1 × 2, 2 × 1 et 3 × 3, intégrales double et triple, fonctions
trigonométriques.

## Constats du plan sur l'ancien dépôt (§ « Ce que l'ancien dépôt dit déjà »)

| # | Constat du plan | Verdict | Preuve |
|---|---|---|---|
| 1 | Outils de maths = aides hors du document ; accrochage à 16 pixels d'écran ; 1 cm = 96 / 2,54 pixels de page | confirmé | `app/actcraft/extras/mathtools.py:1-9`, `:22-25` |
| 2 | Rideau et spot = surcouches avec leur modèle (couverture par bord ; cadre redimensionnable en ovale ou rectangle) | confirmé ; précision : contour du spot mesuré à 9 px sur l'original, proche du réglage embarqué (10), pas de la spécification (15) | `app/actcraft/extras/curtain.py:1-4`, `:22-30` ; `app/actcraft/extras/spotlight.py:1-6`, `:41-58` |
| 3 | Reconnaissance de forme : ligne, rectangle ou carré, ellipse ou cercle, triangle | confirmé pour la référence Python ; **corrigé** par V107 (classe polygone, P16) | `app/actcraft/canvas/recognition.py:1-6`, `:21` |
| 4 | Écriture : moteur d'encre de Windows par COM, français préféré, meilleur texte puis alternatives ; commande grisée sinon | confirmé | `app/actcraft/canvas/recognition.py:170-215`, `:240` |
| 5 | Réglages de page lus des `.flipchart` : rideau (mode, distance), spot (mode, cadre), origine, effet propre à la page | confirmé : propriétés de page 12, 13, 16, 18, 29, 30 ; **précision** : la propriété 27 (outils de page, F-591) existe dans le schéma mais manque à notre documentation du format ; le lecteur Python ne lit aucune de ces propriétés | `app/actcraft/io/_schema.py` (page) ; `docs/re/format-flipchart.md:157` |
| 6 | Clavier à l'écran : la référence lance celui du système ; le nôtre sera écrit par nous | confirmé | `app/actcraft/extras/keyboard.py:1` |

Effets : la référence Python n'en a que cinq, plus aucun et au hasard (`app/actcraft/canvas/transitions.py:13-21`),
avec des noms repris de l'original ; notre catalogue (16 mouvements) et nos noms la remplacent.

## Principes

| # | Principe (nos mots) | Source | Exigence pour ActCraft | Tâche | Scénario | Écart, raison ou lot destinataire |
|---|---|---|---|---|---|---|
| P1 | La calculatrice montre le calcul en cours au-dessus du résultat. | V37, V135 | deux lignes : expression, résultat | L7-T15 | S10 | à vérifier dans la section de L7-T15 |
| P2 | Deux effacements : la dernière saisie ou tout le calcul. | V135 | deux touches d'effacement distinctes | L7-T15 | S10 | à vérifier dans la section de L7-T15 |
| P3 | Le calcul et son résultat passent sur la page, par un bouton ou en le glissant. | V37, V135 | commande qui écrit le calcul sur la page (un texte, annulable) ; glisser vers la page | L7-T15 | S10 | **section à corriger** : transfert sur la page ; glisser hors de la fenêtre (proposé : bouton seul en 7a, glisser en 7b) |
| P4 | La fenêtre des dés se ferme par sa croix ; la synthèse du corpus le dit aussi de la calculatrice, sans fiche qui le montre. | V37 | bouton de fermeture sur chaque panneau | L7-T14, L7-T15 | S10 | — |
| P5 | Le stylo qui approche du porte-mine du compas fait apparaître une mine de sa couleur. | V128 | indication visuelle de l'accrochage, à la couleur du stylo | L7-T12 | S10 | **section à corriger** : L7-T12 n'a pas d'indication de mine à la couleur du stylo |
| P6 | La règle se gradue en millimètres, centimètres ou pouces ; fermer un outil laisse les traits tracés. | V110 | trois unités ; fermer l'outil ne retire aucun trait | L7-T9 | S10 | couvert en partie par `rulerGuidesThePenWithoutChangingIt` (le trait guidé entre au document par le stylo) ; proposition : y ajouter « fermer la règle garde le trait » |
| P7 | Sur le spot, le curseur indique le redimensionnement au bord et le déplacement à l'intérieur. | V108 | curseurs selon la zone | L7-T4 | S10 | **section à corriger** : L7-T4 n'a pas de curseur par zone |
| P8 | Le menu du spot change de forme, capture la zone éclairée, ferme. | V108 | menu du spot : forme, capture vers la page, fermeture | L7-T4 | S10 | capture de la zone : **section à corriger** (L7-T4) ou 7b avec la caméra |
| P9 | Le rideau peut mémoriser sa position pour la page. | V101 | commande qui enregistre l'état du rideau dans les réglages de la page | L7-T6 | S10 | **section à corriger** : ni L7-T3 ni L7-T6 n'ont de commande qui enregistre la position du rideau pour la page |
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
| P20 | Avec l'origine active, les symétries se font autour d'un axe qui passe par elle. | V36 | réflexion autour de l'origine | — (lot 5) | — | **lot 5** : les symétries d'objet y sont livrées (menu de transformation) ; le lot 5 vient avant le lot 7 dans l'ordre du chantier parent, mais la session B livre le lot 7 sans lui ; le lot 5 lit l'origine par page posée par L7-T13 |
| P21 | Sur un outil de maths, le curseur change selon la zone (déplacer, tourner, redimensionner) ; le rapporteur tourne. | V34 | curseurs par zone ; rotation du rapporteur | L7-T9 à L7-T11 | S10 | **section à corriger** : L7-T9 à L7-T11 n'ont pas de curseur par zone |
| P22 | La superposition du bureau ne vaut que pour sa page. | V68 | fond transparent sur le bureau limité à la page qui le porte | 7b (bureau) | S10 | F-071 |
| P23 | La sélection agit sur le bureau à travers une page en superposition. | V68 | entrées transmises au bureau hors des objets | 7b (bureau) | S10 | F-071 ; Windows seulement |

## Écarts pour l'orchestrateur

À verser au plan avant V3 :
- L7-T14 : défaut 1 dé au lieu de 5, nombre et vitesse mémorisés (E5).
- L7-T15 : transfert du calcul sur la page (P3), deux effacements (P2), affichage du calcul (P1).
- L7-T4 : capture de la zone éclairée (P8) ; largeur du contour par défaut 10 et non 15 (réglage embarqué).
- L7-T9 à L7-T13 : translucidité des outils par défaut 100 et non 255 (réglage embarqué).
- L7-T20 : classe polygone (P16).
- L7-T12 : indication de la mine à la couleur du stylo quand il approche (P5).
- L7-T4 et L7-T9 à L7-T11 : curseur selon la zone de l'outil (P7, P21).
- L7-T6 (ou L7-T3) : commande qui enregistre la position du rideau dans les réglages de la page (P9).
- L7-T16 et L7-T19 : hypothèses H7-14 (actions de fin, E9) et H7-15 (validation du message, E11) à ajouter.
- L7-T6 : lire aussi la propriété de page 27 (outils de page, F-591), absente de notre documentation du format.
- 7b, reconnaissance (L7-T21, question du plan) : l'original ne convertit pas les formes au relâché par défaut ; un
  seul délai sert aux formes et à l'écriture, 0 à 5 s, défaut 1 s (réglage embarqué) contre 1 à 5 s, défaut 5 s
  (spécification). Que 0 veuille dire « au relâché » : non déterminable en statique (supposé).
Vers le lot 5 : symétries autour de l'origine (P20).
Vers d'autres lots : moteur des effets par défaut logiciel (lot 6, réglages) ; onglet du bureau (lot 2).
