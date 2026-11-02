# Lot 5 — principes, décisions D1 à D21 et API revérifiées (ouverture sur le Mac)

Tâche L5-T0, scénario S8.6. Base du relevé : `ec3b210` (`feat/livraison-complete` après la PR #8 du lot 4 : socle
L0-S, lots 1, 2, 3, 4, 7 et 10 fusionnés). Constats écrits par nous. Références `Vnn` : numéros des fiches du corpus
(table hors dépôt), reprises seulement des rapports déjà commités.

## Méthode et limites

- **Poste.** Relevé fait sur le Mac mini, qui n'a ni l'ancien dépôt, ni le corpus des vidéos, ni le binaire 3.5.18,
  ni l'outil de contrôle des noms. Décision de l'orchestrateur, notée en hypothèse à revoir (H5-11) : le lot 5 ne
  s'arrête pas pour autant.
- **Décisions.** Chaque décision prend le **défaut sourcé du plan** (ancien dépôt, spécification F-xxx, plan
  comparatif, lus quand le plan a été écrit). Une décision n'est tranchée autrement que si une source **de ce dépôt**
  (code, `docs/`, cadrage, rapports des lots précédents) contredit le défaut. Mention portée sur chacune : « défaut
  appliqué (source du plan) ; confrontation aux vidéos et à REA reportée au PC ».
- **API.** Revérifiées dans le code de ce worktree (en-têtes, sources, QML, données), noms et lignes relevés à
  `ec3b210`. Les numéros de ligne du plan sont indicatifs ; ceux de ce rapport font foi.
- **Non fait sur le Mac (reporté au PC).** Relevé brut hors dépôt `…\docs\re\lots\lot5.md` ; lecture du README du
  corpus et des fiches ; recherches dans l'ancien dépôt et son graphe ; REA en statique ; `NOMS OK` ; commande
  « Titres du corpus » (elle lit les fiches du corpus, absentes du Mac).
- **Légende.** « observé » : lu dans le code ou un document du dépôt ; « supposé » : déduit ; « défaut » : valeur
  du plan reprise sans nouvelle confrontation.

## Constats

Le § 4 de l'amendement 1 du cadrage affecte **5 constats** au lot 5 (3 « Stylo, formes, couleurs », 1 « Sélection,
verrouillage, masquage », 1 « Pages, fond, grille »). Sans le corpus, ils ne peuvent pas être identifiés par
élimination comme aux lots 3 et 4. Ce qui suit reprend les constats que les rapports déjà commités **renvoient au
lot 5** ; l'identification des cinq reste à faire au PC.

| # | Principe (nos mots) | Source | Exigence pour ActCraft | Tâche | Scénario | Écart, raison ou lot destinataire |
|---|---|---|---|---|---|---|
| P1 | Un clic sur un objet verrouillé le traverse et atteint l'objet placé dessous. | lot 3, P29 (V28) | à confronter à D9 : un objet verrouillé reste désigné par sa poignée « Déplacer » | L5-T16 | S8.3 | probable constat « Sélection, verrouillage, masquage » ; renvoyé aux lots 5, 7, 8 par le lot 3 ; **non tranché** : observation demandée au PC (H5-12) |
| P2 | Le clic droit sur la page offre toujours « Coller ». | lot 3, P33 (V49, V48, V68, V119) ; F-202 | `edit.paste` dans `contextMenu` | L5-T1 | S8.1 | renvoyé au lot 5 par le lot 3 |
| P3 | La poignée « menu » de la barre d'objet ouvre le menu du clic droit sur l'objet. | lot 3, P34 (V82, V122) ; F-770 | `objectMenu` ouvert par la barre et par le clic droit sur un objet | L5-T2, L5-T18 | S8 | renvoyé au lot 5 par le lot 3 |
| P4 | Une couleur choisie quand des textes sont sélectionnés recolore leur texte. | lot 4, C4 (V103) | action sur la sélection d'objets (propriétés) | — | — | renvoyé au lot 5 par le lot 4 ; **aucune tâche du plan 5a ne le porte** : candidat `L5-C<n>` ou lot 6 (propriétés), à trancher par l'orchestrateur |
| P5 | Une portion de texte sélectionnée, glissée hors du cadre, devient un texte séparé. | lot 4, C5 (V56, geste peu lisible) | glisser-déposer d'une portion de texte | — | — | renvoyé au lot 5 par le lot 4 ; observation nécessaire ; **aucune tâche du plan 5a ne le porte** : candidat `L5-C<n>` après observation |

## Décisions

Sauf mention contraire : **défaut appliqué (source du plan) ; confrontation aux vidéos et à REA reportée au PC.**

| # | Question | Décision | Source | Tâche amendée |
|---|---|---|---|---|
| D1 | Coller : position, nom, verrou | +20 unités en x et en y ; nom vide ; déverrouillé ; copie sélectionnée, outil Sélection. « Coller » du menu d'objet : superposé à l'objet, même couche. | défaut (ancien dépôt, F-540) | aucune |
| D2 | Dupliquer | +10 unités ; le double est sélectionné. | défaut (ancien dépôt ; plan comparatif) | aucune |
| D3 | Sans sélection | Couper, copier, coller, dupliquer, supprimer agissent sur la page (presse-papiers des pages du lot 4). Concorde avec le lot 4 (P34 : ces cinq entrées du menu de page sont les commandes d'édition générales). | défaut (F-710) ; lot 4, P34 | aucune |
| D4 | Groupes | Couche du groupe = la plus basse des membres ; rang = le plus haut ; Ctrl+G bascule (grouper si au moins 2 objets, dégrouper si un groupe) ; Ctrl+G puis Ctrl+Z vide la sélection. | défaut (ancien dépôt ; plan comparatif) | voir § API (raccourci) |
| D5 | Organiser | Avancer, reculer d'un cran parmi les objets de la même couche ; premier plan, arrière-plan ; changer de couche met au sommet de la couche d'arrivée. | défaut (ancien dépôt, F-513) | aucune |
| D6 | Aligner | Au moins 2 objets non verrouillés ; bords et centres sur l'union ; largeur, hauteur, angle, complet sur le premier créé. **Un objet seul (ou un seul groupe) : sans effet** — conséquence directe du défaut sourcé « au moins 2 objets » ; S8 aligne deux objets depuis l'amendement 1 (M12). L'alignement sur la page reste à présenter à Jérôme (H5-10). | défaut (ancien dépôt) ; tranché ici par cohérence avec ce défaut | L5-T6 (un objet seul : aucune étape) |
| D7 | Ajuster, taille initiale | Ajuster réduit seulement les objets plus grands que la page ; taille initiale = transformation sans échelle. | défaut (F-536) | aucune |
| D8 | Projeter, inverser, symétries | Inverser retourne l'objet, angle opposé ; projeter X : copie miroir au-dessus, Y : à gauche ; symétries autour de l'origine XY quand elle est affichée. | défaut (ancien dépôt ; P20 de L7-T0) | aucune |
| D9 | Verrouiller, masquer, glisser une copie | Verrouillé : ni déplacé ni groupé par un clic, seule la poignée « Déplacer » le déplace ; masqué : non rendu (mode Création au lot 6) ; glisser une copie : chaque glissement laisse l'original et dépose une copie. Le constat P1 (clic qui traverse un verrouillé) n'est pas contradictoire avec le déplacement, mais touche la désignation : non tranché (H5-12). | défaut (F-585, F-586, F-587) ; lot 3, P28 et P29 | aucune avant observation |
| D10 | Pot de peinture | Intérieur d'une forme fermée → remplissage ; contour → couleur du contour ; vide → fond de la page ; image → zone voisine, tolérance 5 sur 100 ; s'applique aux verrouillés ; intersection de formes : remplissage de la forme du dessus (H5-4). | défaut (F-235 ; plan comparatif) | aucune |
| D11 | Connecteur | 14 styles, ligne simple par défaut ; points d'attache aux sommets de la forme ; accrochage à moins de 20 px ; couche supérieure ; extrémités A et B : aucune, rayon, flèche. | défaut (F-245 ; plan comparatif) | aucune |
| D12 | Modification de stylo | Ligne libre, horizontale, verticale, carré, rectangle, cercle, ellipse, arc, ligne brisée, flèche, zigzag. Liste exacte à confirmer au PC. | défaut (F-221) | aucune |
| D13 | Points de forme | Huit points pour un carré (sommets et milieux) ; un point déplacé fait de la forme un polygone ; une forme intelligente garde ses points dédiés. | défaut (F-527) | voir § API (A16) |
| D14 | Onglets (5b) | Onglets en haut par défaut (position réglable par `appTabs.position`, plan 5b T21) ; nom en gras pour l'actif ; astérisque si modifié ; croix ; « Sans titre » ; pages déposées sur un onglet copiées avant sa première page. | défaut (F-010 à F-012) | aucune |
| D15 | Export de page et PDF (5b) | BMP, JPEG, PNG, TIFF à la taille de la page en pixels (1024 × 768 par défaut) ; PDF : une page par page, à la taille de la page. | défaut (F-671, F-672, S8) | aucune |
| D16 | Import PDF (5b) | Document neuf, une page par page du PDF, page à la taille du PDF à 96 ppp, image verrouillée sur la couche du fond. | défaut (plan 5b T24) | aucune |
| D17 | Impression (5b) | Règle du nombre de feuilles du plan 5b (T26) ; notes ; page allongée entière, jamais coupée ; gris ; aperçu ; réglages gardés. | défaut (F-670, amendement 1 du cadrage § 2.2) | voir § API (cadrage) |
| D18 | PowerPoint en images (5b) | Une diapositive = une page, en image de fond, par la chaîne de D16. | défaut (plan 5b) | aucune |
| D19 | Insérer une image ou un média | Au plus 80 % de la page, centrée, couche intermédiaire, nommée d'après le fichier ; média au centre. | défaut (ancien dépôt ; F-129) | aucune |
| D20 | Menu d'édition d'objet | Entrées et ordre de F-770 pour les fonctions livrées (lots 3 à 5 et 7) ; entrées des lots 6 et 8 absentes jusqu'à leur lot. | défaut (F-770) ; lot 3, P34 | aucune |
| D21 | Ce que compte `cacheRefreshCount` | **Rendus terminés** : un appel de `PageView::notifyCacheUpdated` (QML, à la fin d'un rendu programmé) = +1 ; les émissions de `cacheRefreshRequested`, que `scheduleUpdate` regroupe, ne comptent pas. Tests et banc attendent la stabilité (`cacheReady`, puis `kCacheFoldIdleMs` + 300 ms sans redessin). | recommandation de l'amendement 3 du plan, appliquée | voir § API (A18) |

## API

En cours de relevé.
