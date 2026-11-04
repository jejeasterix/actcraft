# Lot 6 — principes, décisions D1 à D14 et API revérifiées (ouverture sur le Mac)

Tâche L6-T0, scénario S30. Base du relevé : `06a3f8f` (`feat/livraison-complete`, « lot 6 lancé sur le Mac »).
Constats écrits par nous. Références `Vnn` : numéros des fiches du corpus (table hors dépôt), reprises seulement des
rapports déjà commités. Relevé brut hors dépôt (`…\docs\re\lots\lot6.md`) : **non écrit, reporté au PC**.

## Méthode et limites

- **Poste.** Relevé fait sur le Mac mini, qui n'a ni l'ancien dépôt, ni le corpus des vidéos, ni le binaire 3.5.18,
  ni l'outil de contrôle des noms. Même conduite qu'au lot 5 (H5-11), reprise ici en H6-11.
- **Étapes 1 à 3 du plan** (corpus, ancien dépôt et son graphe, REA en statique) : non faites. Chaque constat et
  chaque décision porte la mention « à confronter au PC ».
- **Décisions.** Chacune prend le **défaut du plan** (6a ou 6b) ou du cadrage. Elle n'est tranchée autrement que si
  une source **de ce dépôt** (code, `docs/`, rapports des lots précédents) contredit le défaut ou suffit à trancher.
- **API.** Revérifiées dans le code de ce worktree (en-têtes, sources, QML, données), noms et lignes relevés à
  `06a3f8f`. Les numéros de ligne du plan sont indicatifs ; ceux de ce rapport font foi.
- **Non fait sur le Mac (reporté au PC).** Relevé brut `lot6.md` ; README du corpus et fiches ; ancien dépôt
  (`profiles_dialog.py`, `dashboard.py`, `shortcut_bar.py`, `toolbox.py`, `browsers/*`, `fonctionnalites.md`) ; REA
  en statique (boîtes de profils et de réglages, valeurs des cinq profils, fonctions du verrouillage, chaînes du
  correcteur) ; `NOMS OK` ; contrôle des titres du corpus.
- **Légende.** « observé » : lu dans le code ou un document du dépôt ; « supposé » : déduit ; « défaut » : valeur du
  plan reprise sans nouvelle confrontation.

## Constats

Le § 4 de l'amendement 1 du cadrage affecte **17 constats** au lot 6 (7 « interface et profils », 1 « stylo, formes,
couleurs », 1 « pages, fond, grille », 5 « ressources », 1 « actions et navigateurs », 2 « multimédia et import »).
Sans le corpus, ils ne peuvent pas être identifiés. Ce qui suit reprend les constats que les rapports déjà commités
(lots 3, 4, 5 et 10) **renvoient au lot 6** ; l'identification des dix-sept reste à faire au PC. Tous : **à confronter
au PC**.

| # | Principe (nos mots) | Source | Exigence pour ActCraft | Tâche | Scénario | Écart, raison ou lot destinataire |
|---|---|---|---|---|---|---|
| P1 | Une rangée d'icônes se personnalise (commandes ajoutées ; liste vide : aucune icône) et se garde dans le profil. | lot 3, P19 (V30, V36, V136, V137) | contenu de la rangée porté par le profil | L6-T4, L6-T9 | S9.1, S9.7 | probable constat « interface et profils » ; rangée exacte (palette ou barre de raccourcis) à préciser au PC |
| P2 | La corbeille se montre ou se masque depuis Affichage et depuis le profil. | lot 3, P15 (V71, V106, V25) ; F-050 | réglage de la corbeille dans le profil | L6-T4, L6-T6 | S9.7 | D1 : « corbeille visible » dans les quatre profils autres que `preparation` |
| P3 | Le curseur du stylo se dessine au-dessus (défaut), en dessous ou pas du tout. | lot 3, P31 (V50) ; F-201 ; lot 10, P17 | réglage à trois valeurs, défaut « au-dessus » | — | — | renvoyé au lot 6 par le lot 3 ; voir « Ce qui amende le plan » |
| P4 | Un réglage regroupe les traits successifs en un seul objet. | lot 10, P10 (spécification : inactif) ; lot 3, P41 (binaire : **actif**) | réglage, défaut à trancher | — | — | contradiction non levée (H3-8) ; D14 |
| P5 | Un troisième réglage de qualité du rendu vise l'édition d'objets (déplacer, redimensionner). | lot 10, P2 (binaire) | réglage absent de la spécification | — | — | D14 ; défaut non relevé |
| P6 | Le mode Création libère les objets verrouillés et montre les masqués. | lot 3, P28 (V114, V103) ; lot 5, D9 | D6-12 | L6-T22 | S9.8 | conforme au plan ; D11 |
| P7 | Une forme neuve s'appelle « Forme » suivi d'un numéro compté par page ; le navigateur d'objet l'affiche. | lot 4, P7 (V130), H4-11 | nom affiché dans le navigateur d'objet | L6-T14 | S9.3 | D10 |
| P8 | Le tracé depuis le centre est, dans l'original, un réglage du profil. | lot 4, P9, D5 | aucune : notre palette garde sa bascule | — | — | écart déclaré au lot 4, maintenu (D1 : le profil ne porte pas ce réglage) |
| P9 | Le panneau des navigateurs offre, dans l'ordre : pages, ressources, objets, notes, propriétés, actions ; il peut flotter. | lot 4, P30, D12 (V113, V25) ; F-040 à F-042 | ordre des onglets ; panneau flottant | L6-T2 (`browserTabsInOrder`), L6-T15 | S9.3 | flottant renvoyé au lot 6 par le lot 4 (D12) ; voir A8 |
| P10 | Le fond de page se choisit aussi dans les ressources. | lot 4, P35, D15 (V68) | fond depuis la bibliothèque | L6-T19 à L6-T21 | S9.6 | D13 |
| P11 | Une couleur choisie quand des textes sont sélectionnés recolore leur texte. | lot 4, C4 (V103) ; lot 5, P4 | couleur appliquée à la sélection | — | — | laissé à l'orchestrateur par le lot 5 (« `L5-C<n>` ou lot 6 ») ; aucune tâche relevée ; voir L6-P2 |
| P12 | Les gestes du toucher se coupent par un réglage. | lot 3 (« Hypothèses », réglages du toucher : lots 6 et 11) | réglage du toucher | — | — | lot 6 ou 11, à trancher par l'orchestrateur |
| P13 | Le menu d'édition d'objet suit l'ordre de F-770 ; les entrées d'un lot n'apparaissent qu'avec lui. | lot 5, D20 (F-770) | entrées du lot 6 (propriétés, ajout aux ressources) insérées à leur place | — | — | tâche non relevée dans les sections lues |

## Décisions

Sauf mention contraire : **défaut du plan, à confronter au PC.**

| # | Question | Décision | Source | Tâche amendée |
|---|---|---|---|---|
| D1 | Réglages de chaque profil fourni | `preparation` (défaut) : navigateur de pages ouvert et épinglé, palette de couleurs étendue. `en-classe` : page ajustée, navigateurs masqués, corbeille visible, `pinned` = `browser.resources` puis la présentation de la liste de L7-T32. `lecture-ecriture` : clavier à l'écran, reconnaissance d'écriture, message défilant, `spell.check`. `calcul-geometrie` : `math.*`, `tool.dice`, `tool.calculator`. `son-image` : caméra (**`tool.camera-*`**, D10 du lot 7) et `insert.image`. Les quatre autres que `preparation` : page ajustée, navigateurs masqués, corbeille visible, 16 pastilles. Aucun ne porte le tracé depuis le centre (P8). | D6-2 ; L6-T6 (plan, l. 1855-1860) ; D10 du lot 7 | L6-T6 (identifiants de la caméra) ; voir A15 (liste de L7-T32) |
| D2 | E4 : taille de page par défaut | 1024 × 768 dans tous les profils (décision 15). Taille de l'écran pour `en-classe` : **non tranché** (exige la 3.5.18). Le code sait déjà faire les deux : `file.new` → `newDocument(1024, 768)`, `file.new-screen` → taille de l'écran (`ui/qml/Main.qml:543-544`). | D6-13 ; code observé | aucune |
| D3 | Verrou de classe : commandes libres | Libres (`lockable: false`) : `file.quit`, le déverrouillage (L6-T31), l'aide, `page.next`, `page.previous`, `view.fullscreen`, `view.fit`, `view.zoom`. Toutes les autres verrouillables. `restrictable: true` : `edit.copy`, `edit.cut`, `edit.duplicate`, `file.save`, `file.save-as`, `file.print`, `export.*` et la caméra **`tool.camera-*`**. Écart interne au plan : H6-7 ne cite ni `view.fit` ni `view.zoom`, L6-T3 (l. 1000) les cite ; la liste de L6-T3 est retenue. | H6-7 ; L6-T3 ; D10 du lot 7 | L6-T3 (caméra) |
| D4 | E3 : bouton d'édition des raccourcis ; suppression | Défaut du plan 6b (L6-T29, L6-T30). | plan 6b | aucune |
| D5 | Correcteur : options, parcours, dictionnaire de l'utilisateur | Défaut du plan 6b (L6-T32 à L6-T34). Hunspell 1.7.2 accordé (2026-10-18) ; dictionnaire de Grammalecte (MPL 2.0) traité comme couvert (H6-6) ; **licence de Hunspell : MPL 1.1** (Jérôme, 2026-10-21, H6-6b) : bibliothèque statique, aucun fichier de plus à livrer. | H6-6, H6-6b | L6-T32 n'attend plus (H6-6b tranchée) |
| D6 | Grille étendue de couleurs | 36 couleurs (S9.2) ; ordre du plan (L6-T8) ; une couleur choisie s'ajoute à la palette et reste après relance. | S9.2 ; L6-T8 | aucune |
| D7 | Repli d'une palette désépinglée | Repli 600 ms après la sortie du pointeur (`HoverHandler` + `Timer`), animation 120 ms ; marge : non relevée. | L6-T7 (plan, l. 1972-1973) | aucune |
| D8 | Tableau d'accueil | Documents récents ; ouvrir le premier ; F11 le rouvre ; entrées exclues absentes. | S9.9 ; plan 6b (L6-T23, L6-T24) | aucune |
| D9 | Sections de propriétés par type | Trait : Identification, Apparence, Contour, Position ; rien de sélectionné : propriétés de la page ; autres types : défaut du plan (L6-T10, L6-T11). | S9.4 | aucune |
| D10 | Colonnes et icônes du navigateur d'objet | Quatre nœuds de couche ; objets du plus haut au plus bas ; colonnes « masquer » et « verrouiller » ; nom « Forme n » (P7). | S9.3 ; lot 4, P7 | aucune |
| D11 | Mode Création | Bascule F2, neuf réglages ; objets verrouillés modifiables, masqués visibles en translucide, accrochage désactivé, effets de page suspendus ; réglages des actions et des restrictions publiés pour le lot 8. | D6-12 ; lot 3, P28 ; lot 5, D9 | aucune |
| D12 | Étiquette | Au survol, style réglé ; sans légende, rien. | S9.10 | aucune |
| D13 | Tampon ; modèle, fond, grille depuis les ressources | Tampon actif : un clic, une copie ; glisser une ressource l'insère ; fond, grille et modèle de page : défaut du plan 6b (L6-T19 à L6-T21). | S9.6 ; D6-14 ; lot 4, D15 | aucune |
| D14 | Troisième réglage de qualité ; regroupement des traits | Troisième réglage : à ajouter aux réglages, défaut « qualité gardée » (supposé : notre rendu n'abaisse pas la qualité pendant l'édition). Regroupement : inactif par défaut (spécification), **non tranché** : le binaire le dit actif (lot 3, P41). | lot 10, P2, P10 ; lot 3, P41 | voir L6-P1 |

## API

**Jalon 1 : table en cours de relevé.**

## N6

Commande Mac (configuration seule, sans construction, hors jeton) :

```
cd <wt> && cmake -S . -B build-mac -G Ninja -DCMAKE_PREFIX_PATH=$(brew --prefix qt) -DCMAKE_BUILD_TYPE=Debug -DACTCRAFT_BUILD_BENCH=ON > .tmp/cfg.txt 2>&1 && ctest --test-dir build-mac -N | tail -1
```

Résultat à `06a3f8f` : `Total Tests: 164`. **N6 = 164** (Mac, banc actif). À recompter sous Windows (`ctest --preset
debug -N`) : les tests propres à une plateforme peuvent faire varier le total.
