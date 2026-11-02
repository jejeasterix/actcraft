# Lot 4 — principes confrontés (vidéos, ancien dépôt, binaire) et API revérifiées

Relevé brut (hors dépôt) : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot4.md`. Références `Vnn` :
numéros des fiches du corpus (table hors dépôt). Constats écrits par nous. Tâche L4-T0, scénario S30. Base du
relevé : `54abe0c` (lots 1, 2, 10, 3 et socle L0-S fusionnés ; lot 7 non fusionné).

## Méthode et limites

- **Corpus.** 145 fiches cherchées par mots-clés (formes, remplissage, contour, texte, police, navigateur,
  miniature, page, grille, accrochage, fond, allongement, cadre, zone) : 121 sortent, 33 touchent vraiment le lot.
  Les fiches viennent de sous-titres, sur des versions anciennes de l'original : elles montrent des gestes et des
  emplacements, rarement des valeurs. Rien sur les zones de page, le texte de haut en bas, les raccourcis ni la
  place des outils dans la palette.
- **Ancien dépôt.** Outils des formes et du texte, barre de texte, grille, navigateur de pages, menu de page, fond,
  taille personnalisée ; plan comparatif (lignes 18, 20, 24, 33) ; spécification (F-040 à F-803) ; son graphe
  (requêtes en français sans résultat, en anglais quatre pistes), complété par lecture directe.
- **Binaire 3.5.18.** En statique seulement : réglages embarqués, ressources embarquées (descriptions des formes et
  de leurs contrôles), traductions, symboles et constantes. Le serveur d'analyse habituel n'a pas répondu ; aucun
  désassemblage complet, aucune observation à l'écran. Ce qui l'exigerait est marqué « non tranché ».
- **Légende.** « observé » : lu dans le code, dans un réglage, dans une ressource ou vu dans une fiche ;
  « supposé » : déduit.

## Constats

Les huit constats que le tableau du § 4 de l'amendement 1 du cadrage affecte au lot 4 (nombres seulement : 1, 3, 1,
3) ont été identifiés par élimination dans le corpus ; ils sont marqués **C1** à **C8**. C1 et C4 sont probables, les
six autres sûrs.

| # | Principe (nos mots) | Source | Exigence pour ActCraft | Tâche | Scénario | Écart, raison ou lot destinataire |
|---|---|---|---|---|---|---|
| P1 | Un appui-relâché presque immobile avec l'outil des formes ne pose rien. | ancien dépôt (seuil de 3 unités de page sur les deux axes) | aucune forme sous le seuil, sur les deux axes à la fois | L4-T8 | S7.1 | le plan compte 3 px d'écran : même valeur à 100 %, gardé (geste) |
| P2 | L'original compte 39 formes ; la palette montre d'abord 22 d'entre elles. Aux rangs 4 à 6 : une forme à main levée, une ligne brisée posée point par point, une courbe posée point par point. Pas de courbe de Bézier. | binaire (ordre et familles des formes) ; V117 ; ancien dépôt (capture de la palette, 22 formes) | voir D1 : rangs 4 à 6 corrigés ; les 17 autres formes hors du lot | L4-T3, L4-T8, L4-T31 | S7.1 | **corrige** « ligne brisée, zigzag, chevron » (noms lus sur des icônes) ; F-240 cite à tort une Bézier |
| P3 | La forme par défaut est la ligne. | binaire ; ancien dépôt | défaut `line` | L4-T3 | S7.1 | conforme |
| P4 | Carré et cercle restent carrés ; les lignes horizontale et verticale fixent un axe ; Maj garde un carré ou contraint une ligne à 45°. | binaire (méthode de création « uniforme », axes) ; ancien dépôt (Maj) | `squarePresetsStaySquare`, `linePresetsConstrained`, `shiftKeepsSquare` | L4-T3, L4-T8 | S7.1 | Maj non visible en statique : comportement de l'ancien dépôt gardé (D5) |
| P5 | **C2.** Une forme neuve est remplie, d'une couleur propre à l'outil des formes, orange clair par défaut ; un choix « sans remplissage » existe. | binaire (style plein, couleur par défaut) ; V16, V61, V76, V117 ; F-241 ; E1 | rempli par défaut, `#FFAA00`, choix sans remplissage | L4-T1, L4-T11, L4-T12 | S7.1 | **corrige** le blanc par défaut du plan (D2) ; l'ancien dépôt ne remplissait pas |
| P6 | Le contour est noir, d'épaisseur 4, et se règle pour l'outil des formes seul. | binaire ; ancien dépôt ; F-241 | contour `#000000`, 4, mémoire de l'outil (lot 3) | L4-T11 | S7.1 | V76 et V117 divergent sur l'origine de la couleur du contour : le binaire tranche (réglage propre) |
| P7 | Une forme neuve reçoit le nom « Forme » suivi d'un numéro, compté par page. | binaire ; V130 ; ancien dépôt | nom à la création, compteur par page | lot 6 | — | le navigateur d'objets (lot 6) l'affiche ; hypothèse H4-11 nouvelle |
| P8 | Parmi nos 22 formes, ont des points intelligents : le rectangle arrondi (rayon des coins), la flèche (épaisseur du corps, début de la tête), la bulle (pointe, arrondi). Les étoiles n'en ont pas (supposé). | binaire (contrôles décrits dans les ressources des formes, bornes en fraction de la boîte) ; F-242 | trois formes à points intelligents, bornées | L4-T3, L4-T10 | S7.1 | **corrige** le plan, qui en donnait aux étoiles (D4) |
| P9 | Tracer depuis le centre est un réglage de l'application, depuis le coin par défaut. | binaire (réglage embarqué) ; F-241 ; spécification (profil) | `fromCenter = false`, mémorisé | L4-T11, L4-T12 | S7.1 | l'original le met dans le profil (lot 6) : la bascule de notre palette reste, écart déclaré (D5) |
| P10 | **C6.** Pendant le tracé, le point d'appui et le point courant s'accrochent : les dimensions sautent de case en case. | V119 ; binaire ; ancien dépôt | accrochage à l'appui et à chaque mouvement | L4-T8 | S7.2 | conforme (`snapsPressAndRelease`) |
| P11 | « Aimanter à la grille » est une propriété de la grille de la page, et la basculer s'annule. | binaire ; ancien dépôt | `grid.snap` écrit `Grid::snapEnabled` par une commande | L4-T4, L4-T13 | S7.2 | conforme (D6) |
| P12 | Un texte déplacé s'aimante par sa première ligne ; un texte neuf ne s'accroche pas à la création. | binaire (ancrage « première ligne de texte ») ; F-622 | ligne de base pour un texte, coin bas gauche pour le reste | L4-T5, L4-T9 | S7.2 | conforme ; `textUsesItsBaseline` gardé (D6) |
| P13 | L'accrochage d'une grille invisible n'est pas établi. | F-622 (oui) ; binaire (supposé : non) ; ancien dépôt (case grisée) | F-622 gardé | L4-T5 | S7.2 | non tranché, observation à l'écran demandée (H4-12) |
| P14 | La grille normale passe derrière les objets ; la grille « toujours visible » passe devant, même devant les objets ajoutés ensuite. | V21, V119 ; binaire | grille du dessus en surcouche | L4-T7 | S7.2 | conforme |
| P15 | La grille du dessus couvre aussi le trait en cours. | ancien dépôt (grille peinte au premier plan de la scène) | `gridOverlay` entre `ink` et `pageOverlays` | L4-T2 | S7.5 | binaire muet ; D17 |
| P16 | Le créateur de grille s'ouvre par le menu d'édition, le menu de page et le menu du navigateur de pages ; « + » et « − » changent la case avec aperçu immédiat ; couleur, style normal ou à 45°, style de ligne. | V21, V66, V115, V119, V134 | ouverture par `grid.designer` depuis ces menus | L4-T13 | S7.2 | menu du navigateur : L4-T19 |
| P17 | L'outil Texte pose un texte au point cliqué et ouvre la barre ; sur un texte existant, il l'édite ; un texte verrouillé n'est pas édité (un texte neuf se pose par-dessus). | ancien dépôt ; V111, V56 | `pressAsksToEditAt` ; verrouillé ignoré | L4-T22, L4-T21 | S7.3 | ajout : cas du texte verrouillé |
| P18 | Texte par défaut : police de l'application, 14 points, noir, sans fond, sans contour. | binaire (défauts de l'outil Texte) ; ancien dépôt (relevé à l'écran) ; V111, V102 | `TextSettings` du plan | L4-T1, L4-T21 | S7.3 | conforme (D7) ; un réglage de démarrage dit 20 : profil vierge non vérifiable |
| P19 | Un texte neuf laissé vide disparaît sans étape d'annulation ; un texte existant vidé est supprimé en une étape. | ancien dépôt ; F-252 ; binaire (supposé) | les deux cas | L4-T21 | S7.3 | ajout : le second cas (D7) |
| P20 | Le texte se valide par un clic hors du texte (sans en poser un autre), un changement d'outil, un changement de page ou Échap. | ancien dépôt ; binaire (fin d'édition au clic) | validation sur ces quatre gestes | L4-T21 | S7.3 | changement de page : signal `pageAboutToChange` (A6, écart V6) |
| P21 | La barre de texte se place en haut et offre, dans l'ordre : police, taille, plus grand, plus petit, gras, italique, souligné, exposant, indice, couleur, fond, trois alignements, interligne plus et moins, retrait plus et moins, puces, clavier, tout sélectionner, symboles. | ancien dépôt (relevé à l'écran) ; binaire (liste des fonctions) ; V111, V56 ; F-100 | cet ordre, en haut | L4-T23 | S7.3 | position basse (profil) : lot 6 (D8) |
| P22 | La taille change de 2 points (minimum 8) ; l'interligne ajoute ou retire 2 points d'espace au-dessus du paragraphe ; le retrait change d'un niveau (minimum 0). | binaire (constantes des commandes) ; ancien dépôt | `sizeUpStepsByTwo` ; interligne en espace au-dessus | L4-T21 | S7.3 | **corrige** un interligne en pourcentage (ancien dépôt) et un minimum de 4 (D8) |
| P23 | Pendant l'édition, Ctrl+B, Ctrl+I et Ctrl+U mettent en gras, en italique, soulignent ; les raccourcis de commande sont coupés. | binaire (touches traitées par l'élément de texte, raccourcis globaux désactivés) | D8 : l'éditeur prend ces trois touches | L4-T23 | S7.3 | confirme la recommandation du plan |
| P24 | **C4.** Une couleur choisie quand des textes sont sélectionnés recolore leur texte. | V103 | — | lot 5 | — | action sur une sélection d'objets, pas sur l'édition : lot 5 (propriétés) |
| P25 | **C5.** Une portion de texte sélectionnée, glissée hors du cadre, devient un texte séparé. | V56 (geste peu lisible) | — | lot 5 | — | glisser-déposer ; observation nécessaire |
| P26 | Le texte de haut en bas est un objet à part, édité dans une palette propre (police, taille, gras, italique, souligné, couleurs, orientation) ; le fichier garde son rendu en image et une source opaque. | binaire ; F-301 (b) | cinq commandes au lot 4 ; lecture du fichier par le cadre et l'image | L4-T24, L4-T25 | S16 | couleurs et orientation : hypothèse H4-13 ; texte non lisible en clair (D9) |
| P27 | **C1, E12.** L'allongement de page se montre par une commande du menu d'affichage, cachée par défaut : une poignée sous la page, gardée d'une page à l'autre, cliquable même avec une sélection ; un clic, une étape d'annulation. | binaire (commande, étape d'annulation) ; V50, V49 ; F-301 (a) | `page.extend` au menu d'affichage ; `extendKeepsTheCurrentTool` | L4-T26 | S15 | la case du profil (V50) est celle d'une version ancienne ; F-301 suit la 3.5.18 (D10) |
| P28 | Chaque clic allonge d'environ 10 % ; la règle (pas fixe de la hauteur d'origine ou pas cumulé) n'est pas lisible en statique. | binaire (constantes 1,1 et 1,0) ; F-301 | pas fixe de 10 % de la hauteur d'origine | L4-T26 | S15 | non tranché, observation demandée : trois clics depuis 768 donnent 998,4 (fixe) ou 1022,2 (cumulé) (H4-14) |
| P29 | Les « zones » de page sont des cadres : la page compte un nombre de cadres en largeur (0 à 40) et en hauteur (0 à 53), chaque cadre ayant une taille en pixels. L'allongement écrit la même donnée (supposé). Le dessin des cadres sur la page n'est pas établi. | binaire (propriété de page, bornes) ; F-603 ; ancien dépôt (deux champs en lecture seule) | modèle, commande, natif et lecture `.flipchart` d'abord ; dessin après observation | L4-T27, L4-T28 | S15 | D11 tranché pour la donnée, non tranché pour le comportement |
| P30 | Le panneau des navigateurs offre, dans l'ordre : pages, ressources, objets, notes, propriétés, actions (le vote est hors du périmètre) ; Ctrl+B le montre ou le cache ; il est à gauche, épinglé et visible par défaut, et peut passer à droite ou flotter. | binaire (ordre, raccourci, ancrage) ; V113, V25 ; F-040, F-041 | six icônes, Ctrl+B, gauche et droite | L4-T19 | S7.4 | flottant : lot 6 ; largeur : notre thème (D12) |
| P31 | Glisser une vignette réordonne : déposée sur une autre, elle passe avant elle ; dans le vide, à la fin ; la page déplacée devient courante, en une étape. Le défilement est gardé à l'insertion. | ancien dépôt ; V113 ; plan comparatif (ligne 24) | `moveKeepsSelection`, page déplacée courante | L4-T18, L4-T19 | S7.4 | l'ancien dépôt remet le défilement en haut après un déplacement : non repris (rien ne l'impose) |
| P32 | Plusieurs pages se sélectionnent (plage et bascule) ; leur menu offre couper, copier, coller, dupliquer, supprimer. | binaire (menu de sélection multiple) ; F-801, F-803 | `shiftSelectsRange`, `ctrlToggles`, menu sur les pages choisies | L4-T18, L4-T20 | S7.4 | conforme (D13) |
| P33 | La taille des miniatures se change par colonnes (grille de plusieurs colonnes ou liste d'une), une colonne par pas, avec une largeur minimale. | binaire (boutons et commandes du navigateur, constantes) | réglette du plan gardée | L4-T19 | S7.4 | écart déclaré : réglette continue (F-800) au lieu de colonnes (H4-15) |
| P34 | Le menu de page suit, pour les fonctions du lot : insérer une page (avant, après), le fond, le créateur de grille, cacher et aimanter, puis couper, copier, coller, dupliquer, supprimer. Dans l'original, ces cinq dernières sont les commandes d'édition générales. | binaire ; ancien dépôt ; V113 ; F-771 | ordre de L4-T1 conforme | L4-T1, L4-T20 | S7.4 | nos entrées agissent sur les pages (F-803) : écart déclaré (D14) |
| P35 | **C7, E2.** La boîte du fond propose une couleur, deux couleurs en dégradé (radial déduit d'une fiche), une image (mosaïque, ajustée, étirée, centrée, neuf positions), les ressources, la capture et la superposition du bureau, une case « toutes les pages » ; elle se ferme par annuler, appliquer ou valider. Le fond vaut pour la page active. | binaire (boîte, réglages du fond) ; V68, V125, V67 ; F-610 | trois boutons ; couleur, dégradé, image | L4-T20 | S7.4 | **corrige** le bouton unique de F-610 (D15) ; capture et superposition : lot 7 ; ressources : lot 6 ; case « toutes les pages » et dégradé radial : H4-16 |
| P36 | **C8, E10.** La taille personnalisée d'un paperboard neuf se donne en pixels pour un cadre (préréglages et autre) et en nombre de cadres en largeur et en hauteur. | binaire (boîte de taille personnalisée) ; V23 ; F-700 | ajout du nombre de cadres à la taille personnalisée | L4-C1 | S15 | **corrige** F-700, en pixels seuls (D16) |
| P37 | **C3.** La ligne brisée se pose point par point, un clic par sommet, et se termine par un double-clic ; la courbe brisée de même. | V16 ; binaire (état « multi-clic » de l'outil) | ligne et courbe brisées au multi-clic | L4-T31 | S7.1 | absent du plan : tâche ajoutée (D1) |
| P38 | **C2 (suite).** La palette des formes est un panneau ancré à droite qui réduit la zone des pages, avec un nuancier de 2 × 4 dont une case « sans remplissage » ; des flèches font défiler les formes qui ne tiennent pas. | ancien dépôt ; plan comparatif (ligne 18) ; V117 | nuancier 2 × 4 ; défilement si nécessaire | L4-T12 | S7.1 | nos dimensions viennent de notre thème |

## Décisions

| # | Question | Décision | Source | Tâche amendée |
|---|---|---|---|---|
| D1 | Liste et ordre des formes ; défaut ; Bézier et forme libre | Les 22 premières formes de la 3.5.18, dans son ordre d'affichage : `line`, `line-horizontal`, `line-vertical`, **`freeform`** (main levée), **`polyline`** (points posés au clic, double-clic final), **`curve-chain`** (même geste, lissée), `rectangle`, `rounded-rectangle`, `square`, `ellipse`, `circle`, `triangle`, `diamond`, `pentagon`, `hexagon`, `star`, `arrow`, `callout`, `octagon`, `star-four`, `burst`, `burst-twelve`. Défaut `line`. Pas de Bézier ; la forme libre existe. Les 17 autres formes : hors du lot (H4-17). | binaire ; ancien dépôt (capture) ; V16, V117 | L4-T3 (catalogue, `catalogOrderAndDefault`), L4-T8 (`polyline` et `curve-chain` ne se tracent plus dans une boîte), L4-T31 ajoutée |
| D2 | Remplissage à la création (E1) | Rempli par défaut ; couleur de remplissage par défaut `#FFAA00` ; choix « sans remplissage » offert. | binaire ; V16, V61, V117 ; F-241 | L4-T1 ou L4-T11 : `ShapeSettings::fill = 0xFFFFAA00u` (le plan dit `0xFFFFFFFFu`) |
| D3 | Contour, nom | Contour noir, épaisseur 4, réglage propre à l'outil des formes. Nom « Forme n », compté par page. | binaire ; ancien dépôt ; V130 | L4-T11 (contour) ; nom : lot 6 (H4-11) |
| D4 | Points intelligents | Rectangle arrondi : rayon de 0,01 à 0,5 du plus petit côté. Flèche : épaisseur du corps de 0 à 0,5 de la hauteur, début de la tête de 0 à 1 de la largeur (0,6 au départ, soit une tête de 0,4). Bulle : position de la pointe de 0 à 1 de la largeur (1 au départ), arrondi de 0,01 à 0,3 (0,1). Étoiles et explosions : aucun point (supposé). | binaire (contrôles décrits dans les ressources) | L4-T3 (`arrow` : tête 0,4 au départ ; étoiles sans paramètre exposé), L4-T10, L4-T11 (`smartPointsInSelectionFrame` sur ces trois formes) |
| D5 | Tracé depuis le centre ; Maj | Réglage de l'application, coin par défaut, gardé dans les réglages de l'outil ; notre palette garde sa bascule (l'original la met dans le profil, lot 6). Maj : comportement de l'ancien dépôt (carré, 45°), non tranché en statique. | binaire ; F-241 ; ancien dépôt | aucune (écart déclaré) ; H4-12 pour Maj |
| D6 | Accrochage (F-622) | La bascule est la propriété « accrochage autorisé » de la grille de la page, annulable. Au dépôt : coin bas gauche ; un texte par sa ligne de base (**oui, aimanté**). La forme en cours de tracé s'accroche à l'appui et à chaque mouvement. Grille invisible : F-622 gardé, non tranché. | binaire ; ancien dépôt ; V119 ; F-622 | aucune ; `textUsesItsBaseline` gardé |
| D7 | Texte par défaut | Police de l'application, 14 points, noir, sans fond ni contour ; cadre et poignées du lot 3 ; texte neuf vide retiré sans étape, texte existant vidé supprimé en une étape ; validation au clic ailleurs, au changement d'outil, de page, et à Échap. Texte de départ : le nôtre (« Votre texte »). | binaire ; ancien dépôt ; V111 ; F-252 | L4-T21 (cas « existant vidé » ajouté à `emptyTextIsNotKept` ; validation sur `pageAboutToChange`) |
| D8 | Barre de mise en forme ; raccourcis pendant la frappe | Contenu et ordre de P21, barre en haut. Taille ±2 points, minimum 8. Interligne : ±2 points d'espace au-dessus du paragraphe, minimum 0. Retrait : ±1 niveau, minimum 0. Puces : une sorte au lot 4. Symboles : notre liste. Pendant l'édition, l'éditeur prend Ctrl+B, Ctrl+I, Ctrl+U (gras, italique, souligné) ; hors édition, les raccourcis de commande restent. | binaire ; ancien dépôt | L4-T21 (interligne, minimum 8), L4-T23 (raccourcis, recommandation confirmée) |
| D9 | Texte de haut en bas | Lettres empilées en colonne (supposé : écriture verticale asiatique) ; barre propre à cinq commandes (F-301) ; lecture `.flipchart` : l'objet garde une image rendue et une source opaque, le texte n'y est pas en clair. | binaire ; F-301 (b) | L4-T24 (`flipchartVerticalTextRead` lit le cadre et l'image s'il y en a ; sinon objet non lu déclaré), L4-T25 |
| D10 | Page allongée (E12) | Commande au menu d'affichage, poignée cachée par défaut, gardée d'une page à l'autre ; une étape d'annulation par clic ; l'outil courant ne change pas. Pas : 10 % de la hauteur d'origine (plan), non tranché entre fixe et cumulé. | binaire ; V50 ; F-301 (a) | aucune ; H4-14 |
| D11 | Zones de page (F-603) | Une « zone » est un cadre : nombre de cadres en largeur (0 à 40) et en hauteur (0 à 53), taille d'un cadre en pixels. Donnée tranchée ; dessin des séparations, effet sur les objets, l'accrochage et la présentation **non tranchés**. | binaire ; F-603 | L4-T27 (bornes 0 à 40 et 0 à 53 pour `zonesAreBounded`) ; **L4-T28 suspendue** jusqu'à l'observation à l'écran |
| D12 | Panneau des navigateurs | Pages, ressources, objets, notes, propriétés, actions ; Ctrl+B ; à gauche, épinglé, visible ; à droite par `browsers.dock-right` ; flottant au lot 6 ; largeurs : notre thème ; un menu par navigateur. | binaire ; V113, V25 ; F-040 à F-042 | aucune |
| D13 | Navigateur de pages | Sélection multiple (plage, bascule) ; menu de plusieurs pages : couper, copier, coller, dupliquer, supprimer ; fantôme d'insertion, pages restées sélectionnées, coller avant la page active : F-601 et F-802, non contredits. Taille : réglette continue gardée (l'original procède par colonnes). | binaire ; F-800 à F-803 | aucune ; H4-15 |
| D14 | Menu de page | Ordre de L4-T1 conforme à la 3.5.18 pour les fonctions du lot. Les autres entrées (questions, export, bibliothèque, capture, modèles) restent à leurs lots. | binaire ; ancien dépôt ; F-771 | aucune (pas de `L4-C0`) |
| D15 | Fond de page (E2) | Trois boutons : annuler, appliquer, valider ; pas de bouton unique. Lot 4 : couleur, deux couleurs en dégradé, image. Capture et superposition : lot 7 ; ressources : lot 6. | binaire ; V68 | L4-T20 (boîte à trois boutons ; « appliquer » fait une étape, « valider » applique et ferme, « annuler » ferme sans rien changer de plus) |
| D16 | Taille personnalisée (E10) | Pixels pour la taille d'un cadre, plus le nombre de cadres en largeur et en hauteur. F-700 diffère. | binaire ; V23 | **`L4-C1` nécessaire**, à jouer en 4b après L4-T27 (même donnée) |
| D17 | Encre en cours et grille du dessus | `gridOverlay` entre `ink` et `pageOverlays` (la grille couvre le trait en cours, rien ne change au relâché). | ancien dépôt ; binaire muet | L4-T2 applique (amendement de `socleLayerOrder`) |
| D18 | Raccourci de `tool.shape` | Aucun (Ctrl+Shift+S, celui de l'original, est à `file.save-as` chez nous). `tool.text` garde Ctrl+T, comme l'original. | binaire ; `actions/data/commands.json` | aucune |
| D19 | Place du groupe `formes-texte` | Fait établi : l'original range formes et texte parmi les outils de dessin. Défaut retenu : **en fin de palette** (L4-T1). Proximité : choix de Jérôme, relayé par l'orchestrateur ; s'il la veut, `L4-C0` (`layout.json`) et listes attendues amendées. | binaire (boîte principale) | aucune par défaut |

## API des lots 3, 7 et L0-S

Code lu à `54abe0c`. Les numéros de ligne du plan sont indicatifs ; ceux-ci font foi.

| # | API supposée par le plan | Code livré (fichier:ligne) | Conforme | Tâche à amender |
|---|---|---|---|---|
| A1 | `Tool`, `ToolContext`, `DocumentEditor`, `LiftSurface` | `tools/include/actcraft/tools/toolcontext.h:20-45`, `:100-110` : `page()` sans les objets soulevés, `lift(ids)` ajoute, `drop(nullptr)` sans étape ; `show(std::vector<core::PageObject>)` remplace et remet la transformation à l'identité | oui | — |
| A2 | `SelectionView` | `toolcontext.h:66-74` : `visible` en tête, `rubberBand` (`std::optional<core::RectF>`) en dernier | oui (`smartPoints` après `rubberBand`) | — |
| A3 | `UserState`, `defaultSettings` | `tools/include/actcraft/tools/userstate.h:23`, `:27-47` | oui | — |
| A4 | `ToolRegistry::add`, `registerBuiltinTools` | `toolregistry.h:64-78`, `tools/src/toolregistry.cpp:64-74` | non : `builtinsRegistered` (`tests/tst_tools_registry.cpp:107`) compare la liste exacte des quatre outils | L4-T1 (ajouter `shape`, `text`, `text-vertical` à la liste attendue) |
| A5 | `ToolHost` | `ui/include/actcraft/ui/toolhost.h:35`, `:47`, `:51`, `:79` ; `ui/qml/Main.qml:72-74` | oui | — |
| A6 | `DocumentController`, `addChangeObserver` | `ui/include/actcraft/ui/documentcontroller.h:57`, `:79`, `:82`, `:139`, `:142`, `:170-171`, `:198` ; `setCacheObserver` absent | oui | — |
| A7 | `PageView` | `ui/include/actcraft/ui/pageview.h:81-83`, `:160`, `:183`, `:191` (`sceneToPage` déjà posé) | oui | — |
| A8 | `objectAt`, `objectsTouching`, `kHitToleranceScreen` | `core/include/actcraft/core/hittest.h:11`, `:22`, `:24` | oui | — |
| A9 | `TransformObjectsCommand`, `RemoveObjectsCommand`, `ReplaceObjectsCommand` | `core/include/actcraft/core/objectedits.h:17`, `:33` ; `replace.h:33-42` | oui | — |
| A10 | Sous-menus, `objectBar`, `contextMenu` | `actions/include/actcraft/actions/layout.h:16-22`, `:40-41` (chaînes), `parseLayout` `:45` (et non `:42`), `unknownCommands` `:52` ; jeton `>id` `actions/src/layout.cpp:70` ; filtrage `ui/src/commands.cpp:109-150` | oui, ligne fausse | L4-T1 (citation) |
| A11 | Joueur de scénarios | `bench/include/actcraft/bench/uiscenario.h:31-62`, `:94` ; `stepKindRanksFrozen` `tests/tst_bench_uiscenario.cpp:745-752` ; `POINTEUR` `bench/src/uiscenario.cpp:968`, `DOUBLECLIC` `:1005` | oui | — |
| A12 | Couches de `PageArea.qml` | `ui/qml/PageArea.qml:49-117` (`fixedPageItem` `:56`, alias `fixedPage` `:11`, `area.documentController` `:25`, `view` `:29`, encre d'id `inkLayer`) ; `socleLayerOrder` `tests/tst_ui_shell.cpp:773-786` (exige aussi `z() == 0`) ; `canWait` `render/src/fixedpageitem.cpp:54-59`, appelé en `:195` et `:241` ; garde `render/src/pagepainter.cpp:73` | oui, lignes fausses | L4-T2 (`z() == 0` pour `gridOverlay`), L4-T7, L4-T21 (citations) |
| A13 | `PageSettingsChanged` | `core/include/actcraft/core/undo.h:26` (dernier) ; ignoré `core/src/pagecache.cpp:51` ; `/we4062` `cmake/ActCraftModule.cmake:8` | oui | — |
| A14 | `PageContextMenu.qml` | `ui/qml/PageContextMenu.qml:16`, `:30-33` ; `CommandMenu.qml:26-38` dessine déjà les sous-menus | oui | — |

**Vérifications ajoutées (amendement 1).**

| # | Point | Code livré | Verdict | Tâche |
|---|---|---|---|---|
| V1 | `ArgumentKind::Text`, `"optional"`, `Color` | `actions/include/actcraft/actions/commandregistry.h:33-40` ; Text = convertible en texte non blanc (`commandregistry.cpp:120-121`, un nombre passe) ; `"optional"` lu en `:184`, appel sans argument accepté en `:286-287` ; Color `#RRGGBB` ou `#AARRGGBB` (`:97-100`) | conforme, lignes décalées | L4-T1 (`editAtArgumentOptional` n'attend pas le refus d'un nombre) |
| V2 | `Layout::contextMenu` et sous-menus | chaînes seulement (`layout.cpp:159-165`) ; `Commands::contextMenu()` passe `nullptr` comme sous-menus (`ui/src/commands.cpp:187-190`) | écart prévu par L4-T1 | L4-T1 (changer aussi `commands.cpp:189`) |
| V3 | Tests figés | non cités par le plan et rouges dès L4-T1 : `lot3EntriesDeclared` (`tests/tst_actions_layout.cpp:205-206`, menu de page exact), `defaultCommandsFileDeclaresLotOne` (`tests/tst_actions_registry.cpp:197`, `23 + 62` commandes), `paletteColorsAndWidths` (`tests/tst_ui_commands.cpp:96-98`, liste exacte des 7 groupes) ; `toolsMenuFollowsF750` (`tst_actions_layout.cpp:246-260`) ne lit que le premier niveau | **écart** | L4-T1 |
| V4 | Données de commandes | 85 commandes ; Ctrl+Shift+S = `file.save-as` ; Ctrl+B, Ctrl+I, Ctrl+U, Ctrl+T, Ctrl+Shift+G libres ; le groupe de palette `pages` existe déjà (lot 1) | conforme | texte du plan (« groupes `formes-texte` et `pages` » : seul `formes-texte` est neuf) |
| V5 | État d'un contact publié | introuvable : `input::PointerRouter` garde son état privé (`input/include/actcraft/input/routing.h:67`, `:78`) ; `PageView::gestureActive` ne couvre que deux doigts | absent | L4-T17 : ajout en fin de `pageview.h`, signalé |
| V6 | Dépôts forcés du soulèvement | enregistrer, annuler, rétablir, ouvrir, supprimer, effacer : `editAboutToApply` puis dépôt (`ui/src/documentcontroller.cpp:322-326`) ; changer de page : `pageAboutToChange` puis dépôt (`:328-332`) ; `replaceDocument` (`:503`) : `pageAboutToChange` sans dépôt | **écart** pour le changement de page | L4-T21 (valider sur les deux signaux, comme `ui/src/toolhost.cpp:121-122` ; `documentcontroller.cpp` inchangé pour ce motif) |
| V7 | Ce que le lot 3 a changé | relais de Maj et Ctrl déjà livré (`toolhost.cpp:157-162`, `:586-635`) : `onKey` reçoit `Key_Shift`, `Key_Control`, et `Key_unknown` avec les modificateurs à chaque appui ; Échap n'atteint jamais l'outil (raccourci → `tool.select`, `Main.qml:298-302`) ; dépôt de la sélection dans `SelectTool::endDrag` (`tools/src/selecttool.cpp:509-531`), translation en `:485` ; une étape par `mergeId()` et `mergeWith()` (`core/include/actcraft/core/undo.h:44-52`), `closeMerge()` pose un plancher et ne fusionne rien (`core/src/undo.cpp:62-65`) ; `kMergeGrid = 4` libre | écarts | L4-T11 (ne pas réécrire le relais, garder `shiftReachesTheShapeTool`), L4-T8 (`onKey` reçoit aussi Ctrl et `Key_unknown` ; Échap sans effet dans l'application), L4-T9 (brancher en `:485`), L4-T4 et L4-T13 (`closeMerge()` à l'ouverture et à la fermeture de la séance du créateur) |
| V8 | Modèle | `core::Page` sans `baseHeight` (`core/include/actcraft/core/model.h:480-492`) ; `Grid` (`:418-427`) a `visible`, `snapEnabled`, `drawnAbove`, niveaux ; `ObjectKind::Equation = 8` (`:185-197`) ; `Shape` sans préréglage (`:208-225`) ; `Text` sans `vertical` (`:302-307`) | conforme à ce qu'ajoute L4-T1 | — |
| V9 | `AddObjectCommand`, aperçu | `core/include/actcraft/core/edits.h:30-42` ; `LiftLayerItem::setObjects` reconstruit les nœuds à chaque appel (`render/include/actcraft/render/liftlayeritem.h:31`, `:50`) | conforme ; coût de l'aperçu non mesuré | L4-T8 (mesurer l'aperçu au banc) |
| V10 | Fichiers partagés avec le lot 7 | la branche du lot 7 ajoute le signal `documentReplaced` à `documentcontroller.h` et `removeResource` à `core::Document` | à juxtaposer | L4-T1 (même bloc de signaux que `showPage`, `pageContentChanged`) |

## Tests au départ du lot

- **N3 = 113** : `ctest --preset debug -N` sur le worktree configuré à `54abe0c` → `Total Tests: 113` (recompte
  statique concordant : 69 tests d'unité et 44 autres enregistrements). Concorde avec la ligne L0-S0 du fichier
  d'état. **N = 136** dès L4-T1.
- **n_B** : le lot 7 n'est pas fusionné à `54abe0c` → **n_B = 0** à ce jour. Relevé indicatif sur sa branche :
  45 tests ajoutés (`tests/presentation.cmake`), soit N = 181 après sa fusion si ce compte tient ; à mesurer de
  nouveau par l'orchestrateur à la fusion (amendement 2, N12).

## Ce qui amende le plan (pour l'orchestrateur)

1. **L4-T1** (en cours en V1) : quatre tests figés à amender, non cités par le plan (V3, A4) ; `commands.cpp:189`
   (V2) ; défaut `ShapeSettings::fill = 0xFFFFAA00u` (D2), ou à défaut par L4-T11 ; `editAtArgumentOptional`
   (V1) ; citations (A10, V1).
2. **L4-T2** : `gridOverlay` entre `ink` et `pageOverlays`, `z() == 0` (D17, A12).
3. **L4-T3** : catalogue de D1 (rangs 4 à 6) ; flèche à tête 0,4 au départ ; étoiles sans paramètre exposé (D4).
4. **L4-T4, L4-T13** : `closeMerge()` à l'ouverture et à la fermeture du créateur ; `kMergeGrid = 4` (V7).
5. **L4-T8** : `polyline` et `curve-chain` hors du tracé en boîte (D1) ; `onKey` reçoit Ctrl et `Key_unknown` ;
   Échap n'atteint pas l'outil ; coût de l'aperçu au banc (V7, V9).
6. **L4-T9** : point de branchement de l'accrochage (V7). **L4-T10, L4-T11** : points intelligents des trois
   formes de D4 ; relais de Maj déjà livré (V7).
7. **L4-T17** : état de contact absent, ajout en fin de `pageview.h` (V5).
8. **L4-T20** : trois boutons (D15). **L4-T21** : validation sur `pageAboutToChange` (V6), texte existant vidé
   (D7), interligne et minimum de taille (D8). **L4-T24** : lecture du texte de haut en bas (D9).
   **L4-T27** : bornes des cadres (D11). **L4-T28** : suspendue jusqu'à l'observation (D11).
9. **Tâche ajoutée L4-T31** — « `tools` : ligne et courbe brisées au multi-clic, forme à main levée » (P37, C3,
   D1). Dépend de L4-T8. Fichiers : `core/include/actcraft/core/shapegeometry.h` (ajout en fin, signalé),
   `core/src/shapegeometry.cpp`, `tools/src/shapetool.cpp`, `tests/tst_core_shapes.cpp`,
   `tests/tst_tools_shape.cpp`. Preuve : `TESTS OK`, N inchangé (aucun test enregistré), `tst_tools_shape`
   (dont `polylineByClicksEndsOnDoubleClick`, `curveChainIsSmoothed`, `freeformFollowsTheDrag`) et
   `tst_core_shapes` Passed ; banc hors écran joint.
10. **Correctifs.** `L4-C0` : **non nécessaire avant V2** (D14, D18 conformes) ; seulement si Jérôme choisit la
    proximité en D19. `L4-C1` : **nécessaire** (D16), en 4b après L4-T27.
11. **Observation à l'écran demandée** (une seule séance, annoncée à Jérôme) : Maj pendant le tracé (D5),
    accrochage d'une grille invisible (D6), règle de l'allongement (D10), dessin des cadres (D11), encre en cours
    sous la grille du dessus (D17).

## Hypothèses nouvelles ou modifiées

- **H4-11** — Nom « Forme n » posé à la création, compteur par page ; affiché par le lot 6.
- **H4-12** — Maj pendant le tracé et accrochage d'une grille invisible : comportement de l'ancien dépôt et F-622,
  à confirmer à l'écran.
- **H4-13** — Couleurs et orientation du texte de haut en bas : hors du lot 4.
- **H4-14** — Allongement par pas fixe de 10 % de la hauteur d'origine, à confirmer à l'écran.
- **H4-15** — Taille des miniatures en réglette continue, l'original procédant par colonnes.
- **H4-16** — Case « toutes les pages » et dégradé radial de la boîte du fond : hors du lot 4.
- **H4-17** — Les 17 formes au-delà des 22 premières (croix, coche, trapèze, parallélogramme, demi-cercle, arcs,
  flèches coudées…) : hors du lot 4.
