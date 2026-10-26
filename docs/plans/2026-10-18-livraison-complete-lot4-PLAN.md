# Livraison complète — lot 4 « Outils II », sous-lot 4a (formes, grille, réservations) — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

## Amendement 5 (observations L4-O1 et L4-O2, découvertes des tâches, 2026-10-21)

Sources : `docs/verifications/2026-10-21-lot4-observation.md` (L4-O1) et `docs/verifications/2026-10-21-lot4-observation-2.md`
(L4-O2), lus en entier ; rapports de L4-T6, L4-T15, L4-T17 et L4-T21 (fichier d'état). Chaque décision ci-dessous est
tranchée par l'orchestrateur. Les paragraphes « Amendement 5 » des sections priment sur les amendements 1 à 4.
**Tâches mergées** (L4-T0 à L4-T6, L4-T9, L4-T10, L4-T15, L4-T16, L4-T22, L4-O1, L4-O2, et L4-T17, L4-T18, L4-T21 :
ccd826b, 0a54fab, 8a64872) : leur contrat n'est pas réécrit ; leur section reçoit au plus une note « écart constaté ».
**En cours** : L4-T24, lancée sur e9e6e42 ; cet amendement ne la change pas (sa note ne fixe que l'ordre de
`tests/tst_formats_lot4.cpp` avec L4-C4). Chaque
changement, son fondement, les tâches touchées :

1. **D5 — Maj sans effet** (L4-O2, contredit avec un témoin : Maj injectée est bien reçue pendant un geste de souris,
   la sélection d'un texte s'étend). Dans l'original, Maj ne garde pas le carré et ne contraint pas la ligne à 45°,
   tenue avant l'appui comme enfoncée au milieu du glisser. → **L4-T8** : `shiftKeepsSquare` et
   `shiftReleaseFreesTheRatio` retirés, ainsi que `relayedModifiersKeepShiftState`, qui attendait lui aussi un carré ;
   un seul test les remplace, `shiftLeavesTheShapeUnchanged` (Maj ne change ni l'aperçu ni la forme posée, rectangle et
   ligne à 35°). `onKey` ne change rien et rend `false` ; `dragLine` reçoit toujours `constrain45 = false`. **L4-T11** :
   `shiftReachesTheShapeTool` adapté (le relais du lot 3 reste, Maj n'interrompt pas le tracé, la forme posée n'est pas
   un carré). **Inchangés** : les préréglages carré et cercle et les lignes horizontale et verticale (P4, tests
   `squarePresetsStaySquare` et `linePresetsConstrained` de L4-T3). Les points 8 et 10 de l'amendement 4 restent vrais
   pour le relais ; ce qu'ils disent de l'effet de Maj tombe.
2. **D17 — la grille du dessus couvre tout ce qui est posé**, quel que soit le calque : traits, formes, textes (L4-O2).
   L4-O2 explique l'écart avec L4-O1 : L4-O1 cochait l'affichage et la grille du dessus dans la même ouverture de la
   boîte de réglage. **Trait en cours** : couvert lui aussi, décision de l'orchestrateur, pour deux raisons : rien ne
   change au relâché (L4-O1) et l'ancien dépôt peint la grille au premier plan (P15). → **correctif L4-C3 ajouté**
   (correctif de L4-T2) : `gridOverlay` revient entre `ink` et `pageOverlays`, `z() == 0` ; fichiers
   `ui/qml/PageArea.qml` et `tests/tst_ui_shell.cpp` (`socleLayerOrder`) ; il annule l'écart que L4-T2 avait pris
   d'après L4-O1. **L4-T7** dépend de L4-C3 ; sa surcouche couvre la page fixe entière, objets posés compris (conception
   d'origine) ; test ajouté `gridAboveCoversAPlacedShape`. Le contrat « Ordre des couches » ne change pas : L4-C3 le
   rétablit.
3. **D10 — allongement cumulé** (L4-O1) : chaque clic donne × 1,1 sur la hauteur courante. **L4-T4** l'a déjà appliqué
   (test `extendStepsByTenPercentOfCurrent`, 1022,208 après trois clics) : note « écart constaté » seulement.
   **L4-T26** (plan 4b) : attendu **1022,208** au lieu de 998,4 après trois clics depuis 768.
4. **D11 — cadres** (L4-O2) : séparations fines en pointillé aux limites des cadres, en édition comme en présentation ;
   « page suivante » en présentation change de page et ne parcourt pas les cadres ; les séparations n'attirent pas les
   objets (ni l'accrochage : pas de point en plus) ; taille de la page = nombre de cadres × taille d'un cadre ; la boîte
   de taille personnalisée ramène 400 × 300 à 500 × 500 (minimum vu, pas cherché plus loin : **H4-20**). → **L4-T28**
   n'est plus suspendue : séparations pointillées en surcouche, en édition et en plein écran, couleur tirée de notre
   thème ; elle dépend de L4-O2 au lieu de L4-O1. **L4-T27** : bornes, et taille de cadre d'au moins 500 (H4-20) ; la
   grille d'accrochage part de l'origine de la page, pas de chaque cadre (supposé, **H4-21**), test
   `separatorsDoNotAttract`. **L4-C1** : taille de page = cadres × taille de cadre (observée), cadre de 500 au moins.
5. **L4-T27 — gabarit du format natif** (trouvé par L4-T6) : la clé `zones`, omise à zéro, entre dans le gabarit
   `everyPageKey` de `formats/src/nativejson.cpp` (`:1692-1694`) avec ses quatre sous-clés hors défaut ; sinon elle est
   lue comme clé inconnue, gardée dans `unknownKeys` et l'empreinte change. Test ajouté `zonesKeyIsKnown`.
6. **L4-T23 — Échap** (trouvé par L4-T21) : Échap n'atteint pas `TextTool::cancel()` : `ToolHost::cancelGesture`
   n'appelle `cancel()` que si l'outil est occupé (`ui/src/toolhost.cpp:370-378`), et `TextTool` ne l'est jamais
   (`neverBusy`, L4-T22). L4-T23 valide donc le texte par `Keys.onShortcutOverride` → `appText.commit()` ; test ajouté
   « Échap valide le texte » (segment final du scénario `texte`, capture `echap-valide`).
7. **Correctif L4-C2 ajouté au plan 4b** (trouvé par L4-T21) : `DocumentController::execute()` n'annonce rien ; une
   commande de pages lancée pendant l'édition d'un texte (supprimer, couper, dupliquer, déplacer sans changer de page)
   ne valide pas ce texte d'abord. Contrat : `PagesController` valide l'édition en cours avant toute commande de pages
   (et avant de lire les pages pour copier), par le chemin d'`announceEdit` : `editAboutToApply`, puis dépôt forcé de ce
   qui resterait soulevé. Chemin le plus étroit : `ui/src/pagescontroller.cpp` seul (justification dans sa section).
   Dépend de L4-T18 et L4-T21. Tests rouges aujourd'hui : `duplicateDuringEditKeepsTheText`,
   `cutDuringEditKeepsTheText`, `deleteDuringEditCommitsFirst` (`tests/tst_ui_text.cpp`).
8. **Miniatures** (trouvé par L4-T17) : `Main.qml` crée `appThumbnails` en permanence ; les miniatures sont rendues
   panneau fermé, banc compris. → **L4-T19** suspend le rendu quand le panneau des navigateurs est caché (propriété
   `active` ajoutée en fin de `PageThumbnails`, liée à `browserPanel.visible`) ; test `inactiveRendersNothing`.
   **L4-T29** mesure au banc les deux cas, panneau fermé (`miniatures-ferme`) et ouvert (`miniatures`). Tant que
   L4-T19 n'est pas mergée, un banc joué après le merge de L4-T17 compte des rendus de miniatures panneau fermé :
   l'orchestrateur le note à côté de toute mesure de cet intervalle.
9. **Correctif L4-C4 ajouté au plan 4b** (trouvé par L4-T15) : le format natif réécrit un `UnknownObject` depuis son
   JSON d'origine (`formats/src/nativejson.cpp:710-718`). Une page collée qui contient un objet inconnu garde l'ancien
   identifiant dans le fichier : doublon après enregistrement puis relecture. Contrat : écrire l'identifiant courant à
   la place de celui du JSON d'origine. Dépend de L4-T15 et L4-T6, et de L4-T24 (même fichier de test). Test rouge aujourd'hui
   `unknownObjectWritesItsCurrentId`. Fichiers : `formats/src/nativejson.cpp`, `tests/tst_formats_lot4.cpp`.
10. **Vagues.** 4a reprend en trois vagues, L4-T7 et L4-T8 débloquées : **R1** : L4-C3 ‖ L4-T8 · **R2** : L4-T7 ‖
    L4-T11 ‖ L4-T31 · **R3** : L4-T12 ‖ L4-T13 ‖ L4-T14 · puis le point d'étape de 4a (section « Vagues »). 4b : **L4-C4
    en V9 bis**, seule, après le merge de L4-T24 (en cours) et avant L4-T27 ; **L4-C2 en V12** avec L4-T30 et L4-C1 (section « Vagues » du plan 4b).
    Aucun fichier commun dans une même vague (vérifié sur la colonne « fichiers touchés »).
11. **Hypothèses.** H4-12 et H4-14 sont tranchées (D5 contredit, D6 confirmé ; D10 cumulé) ; H4-4 mise à jour ;
    H4-20 à H4-24 ajoutées (section « Hypothèses à revoir »). La réserve de L4-O2 sur Maj+clic (n'ajoute pas à la
    sélection, n'étend pas la sélection d'un texte de la page) n'est tranchée par aucune tâche : **H4-24**, portée à la
    porte de fin du lot.
12. **Comptes.** 4a passe à **19 lignes** : 17 tâches (L4-T0 à L4-T14, L4-T31, L4-C3) et les deux séances
    d'observation (L4-O1, L4-O2, cette dernière ajoutée à la table, mergée). 4b passe à **19 tâches** (L4-T15 à L4-T30,
    L4-C1, L4-C2, L4-C4). Le lot compte **38 lignes** (34 avant). **N ne change pas** (N = 136 + n_B) : aucune tâche
    ajoutée ou amendée n'enregistre de test ; les fonctions nouvelles vont dans des tests déjà enregistrés
    (`tst_ui_shell`, `tst_render_gridoverlay`, `tst_tools_shape`, `tst_ui_shapes`, `tst_core_zones`,
    `tst_formats_lot4`, `tst_ui_text`, `tst_ui_pagethumbnails`, `tst_ui_pagesettings`, `tst_ui_filemenu`,
    `tst_bench_scenarios`, `tst_captures_texte`).

## Amendement 4 (résultats de L4-T0, 2026-10-21)

Source : `docs/verifications/2026-10-18-lot4-principes.md` (L4-T0, fusionnée en 98b57d7), section « Ce qui amende le
plan » (points 1 à 11) et tables D1 à D19, A1 à A14, V1 à V10. Les paragraphes « Amendement 4 » des sections priment
sur les amendements 1 à 3 ; les numéros de ligne de la source font foi. Chaque changement, la décision qui le fonde,
les tâches touchées :

1. **L4-T1 déjà en cours** : son contrat n'est pas réécrit ; sa section reçoit la note « amendements transmis en cours
   d'exécution » (V1, V2, V3, A4, D2, et les citations de A10 et V1). Fondement : points 1 de la source.
2. **Rang de la grille du dessus** : `gridOverlay` entre `ink` et `pageOverlays`, chaque enfant de `page` à
   `z() == 0` ; l'autre rang envisagé tombe. Fondement : D17, A12. Tâches : L4-T2, contrat « Ordre des couches ».
3. **Catalogue des formes** : rangs 4 à 6 = `freeform` (main levée), `polyline` (sommets posés au clic), `curve-chain`
   (même geste, lissée), à la place de `polyline`, `zigzag`, `chevron` ; défaut `line` ; pas de courbe de Bézier ; les
   17 formes suivantes hors du lot (H4-17). Fondement : D1, P2. Tâches : L4-T3, L4-T8, contrat des préréglages, H4-1,
   H4-2.
4. **Points intelligents** : rectangle arrondi (rayon de 0,01 à 0,5 du plus petit côté), flèche (épaisseur du corps de
   0 à 0,5 de la hauteur ; début de la tête de 0 à 1 de la largeur, 0,6 au départ, soit une tête de 0,4), bulle
   (pointe de 0 à 1 de la largeur, 1 au départ ; arrondi de 0,01 à 0,3, 0,1 au départ) ; étoiles et explosions sans
   paramètre exposé. Fondement : D4, P8. Tâches : L4-T3, L4-T10, L4-T11.
5. **Geste de chaque préréglage** : `enum class ShapeGesture` et `shapeGesture()` ajoutés **en fin** de
   `shapegeometry.h` par L4-T3 (ajout signalé), pour que L4-T8 écarte du tracé en boîte les trois formes de D1.
   Fondement : D1 (« `polyline` et `curve-chain` ne se tracent plus dans une boîte »). Tâches : L4-T3, L4-T8.
6. **Tâche ajoutée L4-T31** — `tools` : ligne et courbe brisées au multi-clic, forme à main levée ; dépend de L4-T8 ;
   vague V5. Fondement : P37, C3, D1, point 9 de la source.
7. **Fusion du créateur de grille** : `UndoStack::closeMerge()` (par `DocumentController::closeMerge()`) à l'ouverture
   et à la fermeture de chaque séance du créateur ; `kMergeGrid = 4` est libre. Fondement : V7. Tâches : L4-T4,
   L4-T13.
8. **Outil des formes et touches** : `onKey` reçoit Maj, Ctrl et `Qt::Key_unknown` (modificateurs relayés à chaque
   appui de pointeur) ; Échap n'atteint jamais `onKey` : raccourci de `tool.select`, dont le gestionnaire appelle
   `cancelGesture()`, donc `cancel()` de l'outil occupé, puis active Sélectionner (`ui/qml/Main.qml:298-302`,
   `ui/src/toolhost.cpp:370-378`) ; le coût de l'aperçu
   (`LiftLayerItem::setObjects` reconstruit ses nœuds à chaque appel) se mesure au banc. Fondement : V7, V9. Tâches :
   L4-T8, L4-T14 (la mesure est portée par `forme-page-annotee`, seul scénario de banc qui trace des formes).
9. **Accrochage de la sélection** : appliqué dans `SelectTool::endDrag` (`tools/src/selecttool.cpp:509-531`) à la
   translation calculée par `gestureTransform` (`:485`), jamais dans `gestureTransform`, qui sert aussi pendant le
   geste. Fondement : V7. Tâche : L4-T9.
10. **Relais de Maj déjà livré** par le lot 3 (`ui/src/toolhost.cpp:157-162`, `:586-635`) : L4-T11 ne le réécrit pas
    (l'amendement 1 de L4-T11 tombe pour le filtre d'événements) et garde `shiftReachesTheShapeTool`. Fondement : V7.
11. **Style d'une forme neuve** : remplie, `#FFAA00` par défaut (`ShapeSettings::fill = 0xFFFFAA00u`), contour noir
    de 4, réglage propre à l'outil. Fondement : D2, D3. Tâches : L4-T1 (transmis), à défaut L4-T11.
12. **Sous-lot 4b** (plan 4b, « Amendement 4 ») : `contactActive` ajouté en fin de `pageview.h` (V5) → L4-T17 ; boîte du
    fond à trois boutons (D15) → L4-T20 ; validation du texte sur `pageAboutToChange`, texte existant vidé supprimé en
    une étape, interligne et taille minimale (V6, D7, D8) → L4-T21 ; Échap valide le texte, Ctrl+B, Ctrl+I, Ctrl+U
    pris par l'éditeur (D7, D8) → L4-T23 ; texte de haut en bas lu par son cadre et son image (D9) → L4-T24 ; bornes
    des cadres 0 à 40 et 0 à 53 et taille d'un cadre stockée (D11) → L4-T27 ; **L4-T28 suspendue jusqu'à l'observation à l'écran** (D11).
13. **Correctifs** : `L4-C0` est **abandonné** : D14 et D18 sont conformes, D1 ne touche aucune donnée de commande,
    et D19 reste au défaut (groupe `formes-texte` en fin de palette), choix de Jérôme du 2026-10-21. Il ne figure ni
    dans la table ni dans les vagues. `L4-C1` **nécessaire** (D16) : ajouté au plan 4b, vague V12, après L4-T26, L4-T27 et L4-O1.
14. **Séance d'observation à l'écran** : tâche **L4-O1**, une seule séance, annoncée à Jérôme, qui couvre D5 (Maj
    pendant le tracé), D6 (accrochage d'une grille invisible), D10 (règle de l'allongement), D11 (dessin des cadres),
    D17 (encre en cours sous la grille du dessus). Hors des vagues (ne construit rien), au plus tard avant V9 de 4b :
    L4-T26, L4-T28 et L4-C1 l'attendent. Chaque verdict « contredit » ouvre un correctif `L4-C<n>` nommé par la tâche
    touchée. Fondement : point 11 de la source.
15. **Hypothèses** : H4-11 à H4-17 ajoutées ; l'hypothèse du plan 4b numérotée H4-11 (fond en image) devient **H4-18**
    (collision de numéros avec la source). H4-1, H4-2 et H4-4 sont mises à jour.
16. **Citations fausses corrigées** (A10, A12, V1) : `parseLayout` en `actions/include/actcraft/actions/layout.h:45`
    (et non `:42`) ; `"optional"` lu en `actions/src/commandregistry.cpp:184`, appel sans argument accepté en `:287`
    (et non `:180`, `:283`) ; garde `loadResource` en `render/src/pagepainter.cpp:73` (et non `:71`) ; `canWait` en
    `render/src/fixedpageitem.cpp:54-59`, appelé en `:195` et `:241`, commentaires `:50-53` et `:246-247` (et non
    `:60-64`, `:194`, `:246`, `:57-59`, `:251`) ; `socleLayerOrder` en `tests/tst_ui_shell.cpp:773-786`.
17. **Comptes** : 4a passe à 17 lignes (16 tâches et la séance L4-O1), 4b à 17 tâches (L4-C1 compris), le lot à 34
    lignes. N ne change pas (N = 136 + n_B) : L4-T31 et L4-C1 ajoutent des fonctions à des tests déjà enregistrés.

## Amendement 3 (relecture tour 3, 2026-10-19)

Relecture de 12bf522 (dernier tour) : un majeur (M1) et six mineurs (m1 à m6) ; ils finissent la correction de N3,
N5 et N10. Les paragraphes « Amendement 3 » priment sur les amendements 1 et 2.

- **M1 (4b, L4-T21).** `editingLoadsNoExternalResource` échouait même avec un code juste : `toHtml()` commence par un
  DOCTYPE qui cite `http://www.w3.org/…`, et les images de fond passent aussi par `loadResource`. → l'attendu devient
  « ni `file:`, ni `exemple.invalid`, ni `hote/` » et « appelé seulement pour des URL de l'entrée ».
- **m1 (4b, L4-T21).** Le test reçoit aussi la forme CSS `<p style="background-image:url(file://hote/v.png)">`.
- **m2.** Plus d'`appDocument` dans le branchement de `gridOverlay` : `area.documentController` (section
  « Amendement 1 », défaut 8, et commentaire de `refresh()` dans l'en-tête de `GridOverlayItem`, recopié par L4-T1).
- **m3 (L4-T1).** `tests/tst_actions_layout.cpp:9` fait `using namespace actcraft::actions;` : `Layout` et
  `parseLayout` s'écrivent sans préfixe dans `contextMenuAcceptsSubmenus`.
- **m4 (L4-T1, L4-T17, L4-T29).** `renderCount` compte les rendus **terminés**, revenus d'un fil secondaire. → L4-T1
  ajoute à la fin de l'en-tête `PageThumbnails` `Q_PROPERTY(int launchCount …)` et `int launchCount() const
  noexcept;` (rendus lancés, compté sur le fil principal) ; L4-T17 le remplit ; le banc compte `during_contact` sur
  les hausses de `launchCount` entre un appui et son relâché.
- **m5 (table, L4-T1).** `objectBarAndContextMenu` (`tst_ui_commands`) et `defaultLayoutMatchesLotOne`
  (`tst_actions_layout`), amendés, entrent dans la colonne de preuve.
- **m6 → D19 (L4-T0).** Place du groupe `formes-texte` : en fin de palette ou juste après `outils`. L4-T1 applique la
  recommandation (en fin) ; si D19 tranche autrement, correctif `L4-C0` (`layout.json`) et liste attendue de
  `defaultLayoutMatchesLotOne` amendée en conséquence.

## Amendement 2 (relecture tour 2, 2026-10-18)

Relecture de 860d7a0 : aucun bloquant, 5 majeurs (N1 à N5), 8 mineurs (N6 à N13). Citations revérifiées sur la tête
du chantier (`.worktrees/livraison-complete`, 2b4caea). D6, D8, D17 et D18 restent à L4-T0. Les paragraphes
« Amendement 2 » des sections priment sur « Amendement 1 ».

**Majeurs.**
- **N1.** `objectBarAndContextMenu` (socle, L0-S4 étape 6, `tests/tst_ui_commands.cpp`) attend 4 entrées de
  `contextMenu()` sous Windows et 2 sous Android ; L4-T1 en ajoute 14. → L4-T1 l'amende : il vérifie que les 4
  premières entrées (Windows) et les 2 premières (Android) sont inchangées, avec le commentaire « amendement 1 du
  lot 4 : entrées du menu de page ajoutées en fin ».
- **N2.** `defaultLayoutMatchesLotOne` (`tests/tst_actions_layout.cpp`) compare la liste exacte des groupes de
  palette ; `formes-texte` placé « après `outils` » tombait entre `outils` et `edition` (socle). → groupe
  `formes-texte` **en fin de palette** ; L4-T1 ajoute `formes-texte` en fin de la liste attendue, avec commentaire.
- **N3.** Branchement QML de `gridOverlay`. L'identifiant de la page fixe est `fixedPageItem` (`PageArea.qml`, l'élément
  `FixedPage`) ; `fixedPage` est l'alias de la racine. L'objet du document dans `PageArea.qml` est
  `area.documentController`, pas `appDocument` ; la vue a l'identifiant `view`. → `fixedPage: fixedPageItem`,
  `zoom: view.zoom`, `Connections { target: area.documentController; function onPageContentChanged(index) { if (index
  === area.documentController.currentIndex) gridOverlay.refresh() } }`. La page se lit par
  `FixedPageItem::page()` (`fixedpageitem.h:46`) ; `sourcePage()` (`:55`) rend un indice. Changé : en-tête de
  `GridOverlayItem` (L4-T1), L4-T2, L4-T7, section « Amendement 1 ».
- **N4.** `git add` incomplets : `ui/qml/CommandMenu.qml` (L4-T2), `ui/src/documentcontroller.cpp` (L4-T21),
  `tests/tst_ui_theme.cpp` (L4-T19) ajoutés aux commandes de commit, « si modifié » pour les deux premiers.
- **N5.** `safeTextHtml` laissait passer `QTextFormat::BackgroundImageUrl` (`<table background="file://…">`,
  `<td background=…>`). → la propriété est retirée des formats de bloc, de cadre (dont tableau) et de cellule ; le cas
  entre dans `editingLoadsNoExternalResource` (L4-T21).

**Mineurs.**
- **N6.** Section L4-T4 : « Dans les deux cas », `tst_core_pagecache` dans `-R`, fichiers `undo.h` et `pagecache.cpp`,
  `git add` de ces fichiers → retirés.
- **N7.** Phrase « N = 136 + n_B devient N = 136 + n_B » (remplacement aveugle) → réécrite dans les deux plans.
- **N8.** « Les trois tests nommés » → « les six tests de la commande ciblée » (L4-T1, « Terminé quand » et étape 9).
- **N9.** Plus de numéro de ligne pour le fichier d'état : la règle est citée par son texte (« **Sécurité** :
  `QTextDocument::setHtml`… ») et N3 par la ligne L0-S0 de son tableau (« 113 tests listés »).
- **N10.** `thumbnails.during_contact` reçoit une source : le banc lit `appThumbnails.renderCount` à chaque appui et
  à chaque relâché ; `during_contact` = somme des hausses entre un appui et son relâché (L4-T29).
- **N11.** L4-T26 dépend de L4-T21 (fichier partagé `ui/src/documentcontroller.cpp`) : dépendance ajoutée (V8 → V9,
  les vagues ne changent pas).
- **N12.** n_B relevé de nouveau si le lot 7 fusionne en cours de lot : l'orchestrateur relance `ctest --preset debug
  -N` sur la base à jour et écrit le nouveau N dans le fichier d'état ; toute tâche lancée ensuite prouve avec lui
  (point d'étape de 4a, clôture de 4b, règle « Preuve de tâche »).
- **N13.** `contextMenuAcceptsSubmenus` appelait `Layout::fromJson`, qui n'existe pas : l'API est
  `parseLayout(json, layout, error)` (`actions/include/actcraft/actions/layout.h:45`, amendement 4) ; JSON aligné sur
  `contextMenuRead` du socle (`colors`, `penWidths`). De même, `unknownCommands(layout, registry)` est une fonction
  libre (`lot4EntriesDeclared`).

## Amendement 1 (relecture tour 1, 2026-10-18)

Relecture du plan sur 2014b18 ; références revérifiées sur `feat/livraison-complete` (c34ec77 pour les plans,
code réel du worktree du chantier). Chaque défaut, puis ce qui a changé dans ce plan (4a) et dans celui de 4b.
Les sections des tâches portent chacune un paragraphe « Amendement 1 » : il prime sur le reste de la section.

**Bloquants.**
1. **Raccourci en double.** `tool.shape` avait Ctrl+Shift+S, déjà pris par `file.save-as`
   (`actions/data/commands.json:12`). → L4-T1 pose `tool.shape` **sans raccourci** ; le raccourci éventuel est la
   décision **D18** de L4-T0 (correctif `L4-C0` s'il en faut un).
2. **Argument de `text.edit-at` refusé.** `ArgumentKind::Text` n'accepte qu'une chaîne non vide (plan du socle,
   L0-S4 étape 3) ; une `QVariantMap` et l'appel sans argument étaient refusés. → argument **texte** `"x;y;v"`
   (`x`, `y` en coordonnées de page, `v` = `0` ou `1`) et `"optional": true` (clé lue par
   `actions/src/commandregistry.cpp:184`, appel sans argument accepté en `:287`, amendement 4). Changé : L4-T1 (données, test
   `editAtArgumentOptional`), L4-T2 (gestionnaire), L4-T22 et L4-T21 (4b).
3. **Ordre des couches figé.** `socleLayerOrder` (L0-S9, plan du socle `:1249`) attend la liste exacte
   `fixedPage, pageCache, pendingLayer, liftLayer, ink, pageOverlays`. → L4-T2 **amende `socleLayerOrder`**, de façon
   explicite et motivée, au lieu d'ajouter `pageLayersInOrder` ; la place de `gridOverlay` est la décision **D17**.
4. **Titres de vidéos dans `$exclus`.** → la liste d'exclusion n'est plus dans le dépôt : la commande de L4-T0 la lit
   dans `C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\exclus-controle-titres.txt` (hors dépôt,
   facultatif, un titre par ligne).
5. **HTML chargé sans garde.** Règle du fichier d'état (la règle « **Sécurité** : `QTextDocument::setHtml` charge les ressources citées… » du fichier d'état (`docs/sessions/livraison-complete-etat.md`)) : tout HTML de document passe par un `QTextDocument` dont `loadResource` est neutralisé (garde de L2-T11,
   `render/src/pagepainter.cpp:73`, amendement 4). → L4-T21 charge un HTML **assaini** (test `editingLoadsNoExternalResource`),
   L4-T23 n'affecte jamais `text` en texte riche sans lui. Règle ajoutée aux règles communes.

**Majeurs.**
6. **`setCacheObserver` n'existe plus** (D-S7) : remplacé par `DocumentController::addChangeObserver`, puis
   `core::applyToCache(changes, currentIndex, spy)` dans les espions. Changé : A6, L4-T11, L4-T13, L4-T21 (4b),
   L4-T20 (4b), et tout espion de cache des deux plans.
7. **A13 est posé par le socle** : `Change::Kind::PageSettingsChanged` (L0-S3, D-S5). → `PageOverlayChanged` retiré ;
   `undo.h`, `pagecache.cpp`, `tst_core_pagecache.cpp` sortis de L4-T4 ; `SetGridCommand` (grille du dessus) et
   `SetPageZonesCommand` émettent `PageSettingsChanged`. Règle `/we4062` ajoutée aux règles communes.
8. **La surcouche de grille n'était jamais prévenue** : `PageSettingsChanged` n'atteint pas `FixedPageItem`
   (`presentationChangeRedrawsNothing`). → `GridOverlayItem` perd `setPage(const core::Page*)` (pointeur brut qui
   pend) et `pageChanged()` ; il reçoit `fixedPage` (lecture de la page par `FixedPageItem::page()`, amendement 2) et
   `Q_INVOKABLE refresh()`, appelée par `PageArea.qml` sur `area.documentController.pageContentChanged(index)` de la page
   affichée. Changé : en-tête de L4-T1, L4-T2, L4-T7 (test `refreshRereadsTheGrid`).
9. **Menus.** (a) `insert` : entrées ajoutées **en fin**, `insert.date-time` et `insert.equation` gardés
   (`lot7EntriesPlaced`) ; (b) `tools` suit F-750 : `tool.text-vertical` en fin du sous-menu `other-tools`
   (« Autres outils »), ni `tool.shape` ni `tool.text` dans ce menu ; `toolsMenuFollowsF750` amendé ; (c) **une seule
   clé** `contextMenu`, étendue aux sous-menus (F-771), au lieu de `pageMenu` ; (d) `objectBar` reste un tableau de
   chaînes ; (e) `noShortcutTwice` est celui du socle, dans `tst_actions_registry`. Changé : L4-T1, L4-T2, L4-T20 (4b).
10. **Maj n'arrivait pas à l'outil des formes** : `Tool::onKey` ne reçoit pas les touches (M9), `Sample` n'a pas de
    modificateurs. → `ToolHost` relaie l'appui et le relâché de Maj (filtre d'événements de l'application, API
    publique) ; tests `shiftReleaseFreesTheRatio` (L4-T8) et `shiftReachesTheShapeTool` (L4-T11).
11. **Miniatures décalées après un déplacement de page** (4b). → remise à zéro sur tout `PageInserted` ou
    `PageRemoved`, test `moveRerendersShiftedRows` (L4-T17).
12. **Texte édité pendant un dépôt forcé** (4b) : un dépôt forcé du soulèvement (enregistrer, annuler, changer de
    page) n'émettait pas `editAboutToApply`. → L4-T21 valide le texte avant tout dépôt forcé ; tests
    `saveDuringEditCommitsFirst`, `undoDuringEditCommitsFirst`, `pageChangeDuringEditCommitsFirst`.
13. **N exact.** N = **136** (113 + 23 ; 113 = « 113 tests listés », ligne L0-S0 du tableau du fichier d'état,
    à confirmer par L4-T0), puis **136 + n_B** si le lot 7 a fusionné (n_B = tests enregistrés par le lot 7, relevé par
    L4-T0 après L7-T33). L'ancienne règle « N au moins égal à N3 + 23 » est remplacée partout par « N = 136 + n_B ».
14. **Règle 4 des talons** (plan du socle, `:1412-1413`) reprise à la clôture : `-functions` sans `reserved`, au moins
    une ligne `TRAIT`, `POINTEUR` ou `TEXTE` par scénario rempli, aucun `SKIP` « Talon » dans `ctest -V`.
15. **Raccourcis pendant la frappe** (non vérifié par la relecture) : Ctrl+I et Ctrl+B restent liés à `page.insert-after`
    et `view.browsers` quand le `TextEdit` a le focus. → décision **D8** de L4-T0 (étendue) ; L4-T23 l'applique, avec
    une lecture sous `appText.active`.
16. **Attendus chiffrés du banc.** `forme-page-annotee` : `cache.refreshes = k` (k formes posées) ; `grille-dessus` :
    `cache.refreshes` = nombre de versements ; `miniatures` : rendus comptés dans la fenêtre mesurée, variante sur
    `pauses`. Changé : L4-T14, L4-T29 (4b).

**Mineurs.**
- Encre en cours au-dessus de `gridOverlay` : le trait change d'aspect au relâché → décision **D17** de L4-T0.
- L4-T7 dépend de L4-T4 (ajouté). `pagezones.*` réservés par L4-T1 (talons), `core/CMakeLists.txt` y était déjà ;
  L4-T27 ne touche plus `core/CMakeLists.txt`.
- Les 39 icônes neuves entrent dans `kIcons` de `tests/tst_ui_theme.cpp` (`:17`, boucle `:162`) : 34 par L4-T1, 5 par
  L4-T19.
- `baseHeight` : « à la fin de `Page` » (et non « après `height` »).
- `ArgumentKind::Color` (« #RRGGBB » ou « #AARRGGBB ») pour `shape.fill-color`, `text.color`, `text.background`.
- F-622 et le texte : la spécification dit « un texte par sa ligne de base » (`fonctionnalites.md:768`) ; la relecture
  dit « non aimanté ». → précisé dans **D6** (L4-T0 tranche sur la 3.5.18).
- S15 : l'attendu « la poignée suit » reçoit une lecture `expect` (L4-T26, 4b).
- `formats/src/flipchart.cpp` et `flipchartids.h` (L7-T1) ajoutés aux fichiers chauds communs avec le lot 7.
- Hors du plan, laissés à la porte : libellé « Page vierge avant / après » (contrôle des noms) ; miniatures hors du fil
  principal sous Android (`QFontDatabase::supportsThreadedFontRendering()`, H4-7).

**Amendements de la relecture.** 1 (sources des API : A1-A3 de L0-S6, A11 de L0-S2, A12 de L0-S9) : table A. 2 (A6) :
table A, L4-T21, L4-T20. 3 (A13) : table A, L4-T1, L4-T4, L4-T27. 4 (L4-T7 : prédicat `canWait`,
`render/src/fixedpageitem.cpp:54-59`, appelé en `:195` et `:241`, commentaires `:50-53` et `:246-247`, amendement 4 ; la ligne `:180`
citée avant était fausse) : L4-T7. 5 (L4-T1) : L4-T1. 6 (L4-T2) : L4-T2. 7 (L4-T17 : `contactActive`) : L4-T17. 8
(L4-T21, L4-T23) : 4b. 9 (relais de Maj) : L4-T8, L4-T11. 10 (exclusions hors dépôt) : L4-T0. 11 (clôture) : point
d'étape de 4a, clôture de 4b.

**Renvoyé à L4-T0** (décisions, avec recommandation ; L4-T0 tranche) : D6 étendue (texte aimanté ou non), D8 étendue
(raccourcis pendant la frappe), D17 (encre en cours et grille du dessus), D18 (raccourci de `tool.shape`).

**But.** Version **0.5.0** (amendement 1, § 6), livrée à la fin du sous-lot 4b
(`docs/plans/2026-10-18-livraison-complete-lot4b-PLAN.md`) : une seule PR, une seule version pour le lot. Le
sous-lot 4a livre la tâche d'ouverture, les réservations, les formes (palette à nous, tracé depuis le coin ou le
centre, carré, lignes, points intelligents), l'accrochage à la grille, la grille du dessus en surcouche, le créateur
de grille et les scénarios de banc demandés par le lot 10. Le sous-lot 4b livre le panneau des navigateurs, le
navigateur de pages, le menu de page, le fond de page, le texte et sa barre, le texte de haut en bas, la page
allongée et les zones de page. Exigences : F-040, F-041, F-042, F-100, F-240, F-241, F-242, F-250 à F-253, F-301
(a, b), F-600, F-601, F-603, F-610 (boîte, écart E2), F-620 à F-623, F-771, F-800 à F-803 (cadrage, décision 7 ;
amendement 1, § 2.1 et § 2.3). Scénarios : **S7** (cadrage), **S15**, **S16** (amendement), **S30** (ouverture),
S7.1 à S7.5 ajoutés par ce plan.

**Architecture.** Tout passe par une commande nommée (`actions/`) ; un outil (`tools/`) ne voit que
`tools::ToolContext` (lot 3) ; toute modification passe par la pile de `core/`. Quatre contraintes :

1. **Cache par son interface figée (DA11), limites du lot 10 respectées.** Une forme en cours de tracé n'entre pas
   dans le document : l'outil la montre par la **couche soulevée** du lot 3 (`tools::LiftSurface::show`), sans
   aucun redessin du cache ; au relâché, **une** commande `core::AddObjectCommand`. Une forme et un texte vont sur
   la couche intermédiaire, donc **sous** les annotations : sur une page annotée, l'insertion est « hors fin de
   page » et garde le redessin immédiat (limite déclarée, écart 5 du lot 10). Le lot 10 exige un scénario de banc
   « forme sur page annotée » : L4-T14 le livre et le **mesure**. Un texte existant est **soulevé** pendant son
   édition (un redessin au début, un à la fin, aucun pendant la frappe).
2. **Grille du dessus en surcouche, grille du dessous dans le fond.** La grille « toujours visible » (devant les
   objets) sort de la scène que le cache photographie : un élément `render::GridOverlayItem` la dessine au-dessus
   de la couche soulevée. Masquer, montrer ou régler une grille du dessus ne redessine plus le cache, et un trait
   posé sur une page à grille du dessus redevient différé (limite de DA11 levée pour ce cas, L4-T7). La grille du
   dessous reste dans la page fixe, avec le fond : la régler redessine le cache **une fois par réglage**, jamais
   pendant un déplacement. Accrocher un objet déplacé se calcule au dépôt : un redessin au soulèvement, un au dépôt,
   aucun entre les deux (preuve : `snapDragRefreshesTheCacheTwice`, scénario de banc `grille-dessus`).
3. **Miniatures sans figer l'écriture (4b).** Rendues par le peintre partagé (DA5) sur un fil secondaire, à partir
   d'une copie de la page faite sur le fil principal, regroupées (400 ms après la dernière modification de la page)
   et jamais pendant un contact. Mesurées par le scénario de banc `miniatures` (L4-T29).
4. **Fichiers chauds réservés en tête.** `L4-T1` pose d'un coup commandes, disposition, icônes, champs du modèle,
   enregistrements CMake, tests (talons `QSKIP`) et en-têtes figés des deux sous-lots ; `L4-T2` pose la coquille QML.
   Deux tâches d'une même vague ne partagent aucun fichier.

**Pile.** C++20, Qt 6.12 (Core, Gui, Quick, QuickControls2, Test), CMake + Ninja, MSVC 2022, PowerShell 7.

**Point de départ.** Le lot 4 démarre sur `feat/livraison-complete` **après la clôture du lot 3** et de la tâche
de socle commun **L0-S** (plan `docs/plans/2026-10-18-livraison-complete-socle-3-7-PLAN.md`, en cours d'écriture à
la date de ce plan ; proposition lue : relecture du plan du lot 7, § « Socle commun proposé »). Le lot 7 tourne en
parallèle sur `feat/livraison-complete-b`. Ce plan est écrit sur 333c811, avant le code des lots 3 et L0-S : **toute
API de ces lots citée ici est une hypothèse « à revérifier à l'ouverture du lot »** ; L4-T0 la revérifie contre le
code livré et l'écart amende ce plan, comme au lot 10. Les numéros de ligne sont indicatifs, les noms font foi.

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L4-T0 | Ouverture : vidéos, REA et ancien dépôt confrontés au lot ; API des lots 3, 7 et L0-S revérifiées | — | `docs/verifications/2026-10-18-lot4-principes.md` (hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot4.md`) | isolé | concepteur-critique | rapport commité, relevé brut hors dépôt présent, décisions D1 à D19 et table A1 à A14 écrites, `NOMS OK` (plan ajouté à l'index avant le contrôle), `git grep` des titres du corpus vide (exclusions lues hors dépôt), N3 et n_B relevés | S30 | #tache-l4-t0 |
| L4-T1 | Réservation I : commandes, disposition (`contextMenu` étendu aux sous-menus), icônes, champs du modèle, CMake, 23 tests enregistrés, en-têtes figés et talons des deux sous-lots | — | `actions/data/commands.json`, `actions/data/layout.json`, `actions/include/actcraft/actions/layout.h`, `actions/src/layout.cpp`, `tests/tst_actions_layout.cpp`, `tests/tst_actions_registry.cpp`, `tests/tst_tools_registry.cpp` (amendement 4, A4), `ui/include/actcraft/ui/commands.h`, `ui/src/commands.cpp`, `tests/tst_ui_commands.cpp`, `tests/tst_ui_theme.cpp`, `ui/icons/{forme,texte,texte-vertical,grille,grille-masquer,aimant,createur-grille,navigateurs,page-avant,page-apres,page-couper,page-copier,page-coller,page-dupliquer,page-supprimer,fond,allonger,zones,gras,italique,souligne,exposant,indice,aligner-gauche,centrer,aligner-droite,interligne-plus,interligne-moins,retrait-plus,retrait-moins,puces,symboles,taille-plus,taille-moins}.svg`, `core/include/actcraft/core/model.h`, `core/src/model.cpp`, `tests/tst_core_model.cpp`, `core/CMakeLists.txt`, `tools/CMakeLists.txt`, `render/CMakeLists.txt`, `ui/CMakeLists.txt`, `tests/core.cmake`, `tests/formats.cmake`, `tests/render.cmake`, `tests/tools.cmake`, `tests/ui.cmake`, `tests/bench.cmake`, talons `core/include/actcraft/core/{shapegeometry,shapeedits,gridsnap,pagesettings,pageedits,textedits,pagezones}.h`, `core/src/{shapegeometry,shapeedits,gridsnap,pagesettings,pageedits,textedits,pagezones}.cpp`, `tools/include/actcraft/tools/{shapetool,texttool}.h`, `tools/src/{shapetool,texttool}.cpp`, `tools/include/actcraft/tools/userstate.h`, `tools/src/userstate.cpp`, `tools/include/actcraft/tools/toolcontext.h`, `tools/src/toolregistry.cpp`, `render/include/actcraft/render/gridoverlayitem.h`, `render/src/gridoverlayitem.cpp`, `ui/include/actcraft/ui/{pagethumbnails,pagescontroller,pageoptions,textediting}.h`, `ui/src/{pagethumbnails,pagescontroller,pageoptions,textediting}.cpp`, `ui/include/actcraft/ui/toolhost.h`, `ui/src/toolhost.cpp`, `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`, `tests/tst_ui_document.cpp`, 16 tests talons `tests/tst_{core_shapes,core_gridsnap,core_pagesettings,core_pageedits,core_textedits,core_zones,formats_lot4,render_gridoverlay,render_verticaltext,tools_shape,tools_text,ui_shapes,ui_pagesettings,ui_pagethumbnails,ui_pages,ui_text}.cpp`, 7 scénarios talons `bench/scenarios/{formes,grille,texte,navigateur-pages,page-allongee-zones,texte-vertical,formes-texte-pages-grille}.json` | isolé | implementeur-tdd | `100% tests passed, 0 tests failed out of 136` (136 + n_B si le lot 7 a fusionné), `TESTS OK`, `tst_actions_layout` (dont `contextMenuAcceptsSubmenus`, `toolsMenuFollowsF750` vert et inchangé (V3), `defaultLayoutMatchesLotOne` amendé, `lot3EntriesDeclared` amendé (V3), `lot7EntriesPlaced`), `tst_tools_registry` (dont `builtinsRegistered` amendé, A4), `defaultCommandsFileDeclaresLotOne` et `paletteColorsAndWidths` amendés (V3), `tst_actions_registry` (dont `lot4EntriesDeclared`, `editAtArgumentOptional`, `noShortcutTwice` du socle), `tst_ui_commands` (dont `contextMenuWithSubmenu`, `objectBarAndContextMenu` amendé), `tst_ui_theme` (`iconsAreOurSvgOnA24Grid` : 34 lignes neuves en `PASS`), `tst_core_model` (dont `lot4FieldsKeepOldFingerprints`), `tst_ui_document` (dont `pageContentChangedNamesThePage`, `showPageShowsIt`) Passed ; `premiers-pas` hors écran `CAPTURES OK` | S7, S15, S16 | #tache-l4-t1 |
| L4-T2 | Réservation II : coquille QML (panneau des navigateurs, palette des formes, barre de texte, surcouches de page, gestionnaires des commandes du lot) | L4-T1 | `ui/qml/Main.qml`, `ui/qml/PageArea.qml`, `ui/qml/ToolPalette.qml`, `ui/qml/PageContextMenu.qml`, `ui/qml/CommandMenu.qml` (si le délégué de sous-menu y manque), `ui/qml/{BrowserPanel,PageNavigator,ShapePalette,TextFormatBar,TextEditor,VerticalTextBar,GridDesigner,BackgroundDialog,PageExtensionHandle,PageZonesOverlay}.qml`, `ui/CMakeLists.txt`, `tests/tst_ui_shell.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = 136 + n_B, `tst_ui_shell` (dont `lot4ShellPlaceholders`, `everyLot4CommandHasAHandler`, `socleLayerOrder` amendé selon D17 : `gridOverlay` entre `ink` et `pageOverlays`, `z() == 0`, `editAtHandlerParsesItsText`) Passed ; `premiers-pas` hors écran `CAPTURES OK` ; banc hors écran `content_check.ok = True` | S7, S15, S16 | #tache-l4-t2 |
| L4-T3 | `core` : catalogue des formes, boîte tracée (coin, centre, carré), lignes, points intelligents, commande de paramètres | L4-T0, L4-T1 | `core/include/actcraft/core/shapegeometry.h`, `core/src/shapegeometry.cpp`, `core/include/actcraft/core/shapeedits.h`, `core/src/shapeedits.cpp`, `tests/tst_core_shapes.cpp` | isolé | implementeur-tdd | `TESTS OK`, `tst_core_shapes` Passed (dont `catalogOrderAndDefault` (rangs 4 à 6 de D1), `dragFromCornerAndCenter`, `squarePresetsStaySquare`, `linePresetsConstrained`, `arrowHeadSmartPointBounded`, `arrowHeadStartsAtFourTenths`, `smartPointsOnlyForThreePresets`, `gestureOfEachPreset`, `setParametersIsOneStepAndRebuilds`) | S7, S7.1 | #tache-l4-t3 |
| L4-T4 | `core` : réglages de page (grille, fond, extension) et contrat de `Change` ignoré par le cache | L4-T0, L4-T1 | `core/include/actcraft/core/pagesettings.h`, `core/src/pagesettings.cpp`, `tests/tst_core_pagesettings.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_core_pagesettings` (dont `gridAboveChangeIsIgnoredByTheCache` — `PageSettingsChanged` du socle —, `gridBelowChangeRedrawsOnce`, `extendStepsByTenPercentOfBase`, `threeUndosRestoreTheHeight`, `designerChangesMerge`, `closeMergeSeparatesTwoSessions`) Passed ; mutation de `gridDrawsUnderObjects` rouge puis remise | S7.2, S15 | #tache-l4-t4 |
| L4-T5 | `core` : accrochage à la grille (intersection la plus proche, coin bas gauche, ligne de base d'un texte) | L4-T0, L4-T1 | `core/include/actcraft/core/gridsnap.h`, `core/src/gridsnap.cpp`, `tests/tst_core_gridsnap.cpp` | isolé | implementeur-tdd | `TESTS OK`, `tst_core_gridsnap` Passed (dont `nearestOnNormalGrid`, `onlySnappingLevelsCount`, `invisibleGridStillSnaps`, `snapDisabledReturnsNothing`, `bottomLeftCornerLandsOnCrossing`, `textUsesItsBaseline`, `diagonalGridCrossings`) | S7, S7.2 | #tache-l4-t5 |
| L4-T6 | `formats` : natif — préréglage et paramètres des formes, texte de haut en bas, hauteur d'origine de la page | L4-T1 | `formats/src/nativejson.cpp`, `tests/tst_formats_lot4.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_formats_lot4` Passed (dont `presetAndParametersRoundTrip`, `defaultsAreOmitted`, `verticalTextRoundTrip`, `baseHeightRoundTrip`, `olderFileReadsWithDefaults`), `tst_formats_nativejson` et `tst_formats_nativefile` Passed | S7, S15, S16 | #tache-l4-t6 |
| L4-T7 | `render` : grille du dessus en surcouche `GridOverlayItem`, retirée de la scène du cache, qui couvre tout ce qui est posé (amendement 5) ; trait différé sur page à grille du dessus | L4-T1, L4-T2, L4-T4, L4-C3 | `render/include/actcraft/render/gridoverlayitem.h`, `render/src/gridoverlayitem.cpp`, `render/include/actcraft/render/fixedpageitem.h`, `render/src/fixedpageitem.cpp`, `ui/qml/PageArea.qml`, `tests/tst_render_gridoverlay.cpp`, `tests/tst_render_pendinglayer.cpp`, `tests/tst_render_fixedpage.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_render_gridoverlay` (dont `drawsOnlyTheGridAbove`, `hidingTheGridAboveRebuildsNoCache`, `sharpOnePixelLines`, `refreshRereadsTheGrid`, `gridAboveCoversAPlacedShape`), `tst_render_pendinglayer` (dont `strokeOnGridAbovePageIsDeferred`), `tst_render_fixedpage` Passed ; banc hors écran `content_check.ok = True` | S7.2, S7.5 | #tache-l4-t7 |
| L4-T8 | `tools` : outil des formes (aperçu par la couche soulevée, coin ou centre, carré et cercle par leur préréglage, Maj sans effet (amendement 5, D5), accrochage, clic simple sans forme) | L4-T3, L4-T5 | `tools/include/actcraft/tools/shapetool.h`, `tools/src/shapetool.cpp`, `tests/tst_tools_shape.cpp` | isolé | implementeur-tdd | `TESTS OK`, `tst_tools_shape` Passed (dont `dragPreviewsWithoutExecuting`, `releaseExecutesOneAdd`, `singleClickAddsNothing`, `fromCenterSetting`, `shiftLeavesTheShapeUnchanged`, `snapsPressAndRelease`, `cancelClearsThePreview`, `unfilledChoiceHonoured`, `chainPresetsExecuteNothingOnRelease` ; `Totals: 11 passed`) ; banc hors écran joint | S7, S7.1 | #tache-l4-t8 |
| L4-T9 | `tools` : sélection — accrochage au dépôt (coin bas gauche, ligne de base) | L4-T5 | `tools/src/selecttool.cpp`, `tools/include/actcraft/tools/selectiongeometry.h`, `tools/src/selectiongeometry.cpp`, `tests/tst_tools_select.cpp` | isolé | implementeur-tdd | `TESTS OK`, `tst_tools_select` Passed (dont `dropSnapsBottomLeftCorner`, `noSnapWithoutGridSnap`, `snapDragRefreshesTheCacheTwice`, `noSnapDuringTheDrag`, tests du lot 3 inchangés) | S7, S7.2 | #tache-l4-t9 |
| L4-T13 | Réglages de grille : contrôleur de page (masquer, accrocher, créateur à niveaux), boîte du créateur, scénario `grille` | L4-T2, L4-T4, L4-T7 | `ui/include/actcraft/ui/pageoptions.h`, `ui/src/pageoptions.cpp`, `tests/tst_ui_pagesettings.cpp`, `ui/qml/GridDesigner.qml`, `bench/scenarios/grille.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_pagesettings` (dont `hideGridAboveRefreshesNoCache`, `hideGridBelowRefreshesOnce`, `snapToggleFollowsThePage`, `addRemoveResetLevels`, `designerSessionMakesASingleStep`, `twoDesignerSessionsMakeTwoSteps`), `tst_captures_grille` Passed ; `captures.ps1 -Scenario grille` hors écran : `LU appPageSettings.gridVisible=false OK`, `LU pageView.cacheRefreshCount=${avant} OK`, `CAPTURES OK` | S7.2 | #tache-l4-t13 |
| L4-T10 | `tools` : sélection — points intelligents d'une forme (affichés, glissés, une étape) | L4-T3, L4-T9 | `tools/src/selecttool.cpp`, `tools/include/actcraft/tools/selectiongeometry.h`, `tools/src/selectiongeometry.cpp`, `tests/tst_tools_select.cpp`, `ui/qml/SelectionOverlay.qml` | isolé | implementeur-tdd | `TESTS OK`, `tst_tools_select` Passed (dont `smartPointsShownForArrow`, `smartPointDragLiftsOnceAndDropsOneStep`, `noSmartPointForReadShapes`, `noSmartPointForStars`) ; banc hors écran joint | S7.1 | #tache-l4-t10 |
| L4-T11 | `ui::ToolHost` : outil des formes branché (choix, remplissage, coin ou centre, mémoire), contour des préréglages, points intelligents dans le cadre | L4-T2, L4-T8 | `ui/src/toolhost.cpp`, `ui/src/tooladapters.h`, `ui/src/tooladapters.cpp`, `tests/tst_ui_shapes.cpp`, `tools/include/actcraft/tools/userstate.h`, `tools/src/userstate.cpp` (seulement si L4-T1 n'a pas posé le remplissage `#FFAA00` ou le contour noir de 4, D2, D3) | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_shapes` Passed (dont `shapeToolActivatesWithPalette`, `presetChoiceSurvivesRestart`, `outlineOfEveryPreset`, `smartPointsInSelectionFrame` (rectangle arrondi, flèche, bulle), `shapeGoesThroughTheCacheInterface`, `shiftReachesTheShapeTool` (amendement 5 : Maj relayée sans effet, forme posée 100 × 70), `newShapeIsOrangeWithBlackOutline`) ; banc hors écran joint | S7, S7.1 | #tache-l4-t11 |
| L4-T12 | QML : palette des formes (nuancier, formes, sans remplissage, coin ou centre), curseur à miniature ; scénario `formes` | L4-T10, L4-T11 | `ui/qml/ShapePalette.qml`, `ui/qml/ToolCursor.qml`, `bench/scenarios/formes.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_captures_formes` Passed ; `captures.ps1 -Scenario formes` hors écran : `LU appTools.currentTool=shape OK`, `LU appDocument.shownObjectCount=2 OK`, `PIXEL rectangle-contour OK`, `CAPTURES OK` ; banc hors écran joint | S7.1 | #tache-l4-t12 |
| L4-T14 | Banc et joueur : scénarios de banc `forme-page-annotee` et `grille-dessus` ; étape `typeText` du joueur | L4-T7, L4-T11 | `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp`, `bench/src/runner.cpp`, `bench/include/actcraft/bench/result.h`, `bench/src/result.cpp` (clé `shape_preview`, amendement 4), `tests/tst_bench_scenarios.cpp`, `tests/tst_bench_result.cpp`, `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`, `tests/tst_bench_uiscenario.cpp`, `docs/banc.md` | isolé | implementeur-tdd | `TESTS OK`, `tst_bench_scenarios` (dont `shapeOnAnnotatedPageScenario`, `gridAboveScenarioDrawsStrokes`), `tst_bench_result` (dont `lot4ScenariosWithFrames`, qui vérifie la clé `shape_preview`), `tst_bench_uiscenario` (dont `typeTextStepParsed`, `stepKindRanksFrozen` étendu à `TypeText`) Passed ; banc hors écran : scénarios `forme-page-annotee` (`cache.refreshes = k`, k formes posées ; clé `shape_preview` : trames pendant les glissés, coût de l'aperçu, V9) et `grille-dessus` (`cache.refreshes` = nombre de versements, 0 entre deux versements) présents | S7.5 | #tache-l4-t14 |
| L4-T31 | `tools` : ligne et courbe brisées au multi-clic, forme à main levée | L4-T8 | `core/include/actcraft/core/shapegeometry.h` (ajout en fin, signalé), `core/src/shapegeometry.cpp`, `tools/include/actcraft/tools/shapetool.h` (membres privés et `abandon()`, signalé), `tools/src/shapetool.cpp`, `tests/tst_core_shapes.cpp`, `tests/tst_tools_shape.cpp` | isolé | implementeur-tdd | `TESTS OK`, N inchangé (= 136 + n_B, aucun test enregistré), `tst_tools_shape` (dont `polylineByClicksEndsOnDoubleClick`, `curveChainIsSmoothed`, `freeformFollowsTheDrag`, `chainCancelKeepsTwoVerticesOrMore`, `chainAbandonAddsNothing`, `escapeVersesTheChainInOneStep`) et `tst_core_shapes` (dont `smoothChainPassesThroughVertices`, `chainNeedsTwoDistinctPoints`) Passed ; banc hors écran joint | S7.1 | #tache-l4-t31 |
| L4-O1 | Séance d'observation de l'original à l'écran, annoncée à Jérôme (D5, D6, D10, D11, D17) | L4-T0 | `docs/verifications/2026-10-21-lot4-observation.md` (relevé brut hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot4-observation.md`) | isolé | concepteur-critique | annonce à Jérôme faite par l'orchestrateur d'une ligne avant ; jeton `ecran` ; rapport commité : verdict « confirmé », « contredit » ou « non observé » pour chacun des cinq points, avec la liste des tâches à amender ; `NOMS OK` ; `git grep` des titres du corpus vide | S30, S7.1, S7.2, S7.5, S15 | #tache-l4-o1 |
| L4-O2 | Seconde séance d'observation de l'original (D5 avec un témoin de Maj, D17 bis, reste de D11) — **mergée** (209b8fe) | L4-O1 | `docs/verifications/2026-10-21-lot4-observation-2.md` (relevé brut hors dépôt) | isolé | concepteur-critique | rapport commité : verdicts de D5 (contredit avec témoin), D17 bis (la grille du dessus couvre tout ce qui est posé) et D11 (séparations dessinées, présentation, objet déplacé, accrochage, boîte de taille personnalisée) ; profil de l'original copié puis remis, 0 écart ; `NOMS OK` ; `git grep` des titres du corpus vide | S30, S7.1, S7.5, S15 | #tache-l4-o2 |
| L4-C3 | Correctif de L4-T2 : grille du dessus remise entre `ink` et `pageOverlays` (D17, L4-O2, amendement 5) | L4-T2, L4-O2 | `ui/qml/PageArea.qml`, `tests/tst_ui_shell.cpp` | isolé | implementeur-tdd | `socleLayerOrder` rouge avant le déplacement (liste attendue amendée), puis `TESTS OK`, N = 136 + n_B inchangé (aucun test enregistré), `tst_ui_shell` Passed (dont `socleLayerOrder` : `fixedPage, pageCache, pendingLayer, liftLayer, ink, gridOverlay, pageOverlays`, chaque enfant à `z() == 0`, `lot4ShellPlaceholders` inchangé) ; `premiers-pas` hors écran `CAPTURES OK` ; banc hors écran `content_check.ok = True` | S7.2, S7.5 | #tache-l4-c3 |

**Amendement 4 :** 17 lignes dans ce sous-lot — 16 tâches (L4-T0 à L4-T14, L4-T31) et la séance d'observation L4-O1 — et 17 tâches dans 4b (L4-T15 à L4-T30, L4-C1) : 34 lignes pour le lot (15 + 16 = 31 avant l'amendement 4). L4-T31 et L4-C1 n'enregistrent aucun test : N ne change pas. Tests ctest : L4-T1 enregistre d'un coup les **23** tests des
deux sous-lots (16 unitaires, 7 de captures), talons `QSKIP` jusqu'à ce que leur tâche les remplisse : **N3 + 23**
dès L4-T1, où N3 est le nombre de tests au départ du lot. **Amendement 1 : N3 = 113** (fin du lot 3 et de L0-S,
ligne L0-S0 du fichier d'état), donc **N = 136** dès L4-T1 ; si le lot 7 a fusionné (L7-T33), **N = 136 + n_B** (n_B relevé par L4-T0,
étape 6). Aucune autre tâche n'en enregistre. N est exact : un écart se signale, il ne se couvre pas par « ≥ ». Un talon sauté ne prouve rien : chaque preuve nomme les fonctions attendues `PASS`.

**Amendement 5 :** 19 lignes dans ce sous-lot — 17 tâches (L4-T0 à L4-T14, L4-T31, L4-C3) et les deux séances
d'observation (L4-O1, L4-O2) — et 19 tâches dans 4b (L4-T15 à L4-T30, L4-C1, L4-C2, L4-C4) : **38 lignes** pour le lot.
L4-C3, L4-C2 et L4-C4 n'enregistrent aucun test : **N ne change pas** (N = 136 + n_B).

Worktrees : `.worktrees/livraison-complete--L4-T<n>` (profil), tous isolés : deux tâches d'une même vague ne
partagent aucun fichier (vérifié sur la colonne « fichiers touchés »).

**Écran.** Aucune tâche de 4a n'ouvre de fenêtre d'ActCraft visible ; seules L4-O1 et L4-O2 (mergées) ont ouvert l'original à l'écran, sous le jeton `ecran`, annoncée à Jérôme d'une ligne avant par l'orchestrateur (amendement 4). La clôture du lot (fin de 4b) joue les captures et le banc
à l'écran sous le jeton `ecran`, annoncés à Jérôme d'une ligne avant.

## Vagues (largeur 3 au plus ; 2 places de tests pour ce lot)

V1 : L4-T0 ‖ L4-T1 · V2 : L4-T2 ‖ L4-T3 ‖ L4-T4 · V3 : L4-T5 ‖ L4-T6 ‖ L4-T7 · V4 : L4-T8 ‖ L4-T9 ‖ L4-T13 ·
V5 : L4-T10 ‖ L4-T11 ‖ L4-T31 · V6 : L4-T12 ‖ L4-T14 · puis le point d'étape de 4a, puis 4b (son plan).

**Amendement 5 — reprise de 4a.** Mergées : L4-T0 à L4-T6, L4-T9, L4-T10, L4-O1, L4-O2. L4-T7 et L4-T8 ne sont plus
bloquées (L4-O2 a rendu son rapport). Le reste se joue en trois vagues, qui remplacent V3 à V6 pour ce qui n'est pas fait :

**R1** : L4-C3 ‖ L4-T8 · **R2** : L4-T7 ‖ L4-T11 ‖ L4-T31 · **R3** : L4-T12 ‖ L4-T13 ‖ L4-T14 · puis le point d'étape de 4a.

- R1 : L4-C3 (`PageArea.qml`, `tst_ui_shell.cpp`) et L4-T8 (`shapetool.*`, `tst_tools_shape.cpp`) ne partagent rien.
- R2 : L4-T7 attend L4-C3 (`PageArea.qml` : L4-T2 → L4-C3 → L4-T7) ; L4-T11 et L4-T31 attendent L4-T8. Fichiers :
  L4-T7 (`gridoverlayitem.*`, `fixedpageitem.*`, `PageArea.qml`, `tst_render_gridoverlay`, `tst_render_pendinglayer`,
  `tst_render_fixedpage`), L4-T11 (`toolhost.cpp`, `tooladapters.*`, `tst_ui_shapes.cpp`, `userstate.*` éventuels),
  L4-T31 (`shapegeometry.*`, `shapetool.*`, `tst_core_shapes.cpp`, `tst_tools_shape.cpp`) : aucun commun.
- R3 : L4-T13 attend L4-T7 ; L4-T12 attend L4-T10 et L4-T11 ; L4-T14 attend L4-T7 et L4-T11. Fichiers : L4-T13
  (`pageoptions.*`, `tst_ui_pagesettings.cpp`, `GridDesigner.qml`, `grille.json`), L4-T12 (`ShapePalette.qml`,
  `ToolCursor.qml`, `formes.json`), L4-T14 (`bench/…`, `tst_bench_*`, `docs/banc.md`) : aucun commun.
- 4b avance en parallèle sur ses seules dépendances (plan 4b, « Vagues », amendement 5) ; aucune tâche de R1 à R3 ne
  touche un fichier d'une tâche de 4b lancée en même temps (`ui/src/pageoptions.cpp` : L4-T13 → L4-T26, dépendance
  déclarée).

**Amendement 4.** L4-T31 entre en V5, après L4-T8 (V4) : ses fichiers (`shapegeometry.*`, `shapetool.*`, `tst_core_shapes.cpp`, `tst_tools_shape.cpp`) ne recoupent ni ceux de L4-T10 (`selecttool.cpp`, `selectiongeometry.*`, `tst_tools_select.cpp`, `SelectionOverlay.qml`) ni ceux de L4-T11 (`toolhost.cpp`, `tooladapters.*`, `tst_ui_shapes.cpp`, `userstate.*` éventuels). **L4-O1** est hors des vagues : elle ne construit rien et n'occupe aucune place de tests ; l'orchestrateur l'annonce à Jérôme et la lance dès que V2 démarre, ou au premier moment où Jérôme est disponible, et **au plus tard avant V9 de 4b** : L4-T26, L4-T28 et L4-C1 l'attendent (colonne « dépend de ») ; l'orchestrateur ne lance pas V9 avant son rapport. Pendant L4-O1, aucune fenêtre d'ActCraft n'est ouverte à l'écran (le jeton `ecran` est pris).

**Places.** Le jeton `tests` a 3 places sur la machine, dont **2 pour ce lot** (la session B, lot 7, garde la
troisième). Une vague de 3 tâches qui construisent en a toujours une qui attend la notification du jeton : c'est
prévu, l'agent attend, il ne sonde pas. L4-T0 ne construit rien.

**L4-T0 et les autres tâches.** L4-T1 (données, mécanique) et L4-T6 (sérialisation à nos noms) ne l'attendent pas
pour leur contenu ; toutes les tâches de comportement l'attendent (V2 commence quand V1 est finie). Ses décisions
D1 à D19 et sa table A1 à A14 **amendent ce plan et celui de 4b** avant V2 (relecture du plan exigée). Une décision
qui change une donnée de commande (raccourci, libellé, ordre de la palette des formes) passe par un correctif
`L4-C0` (`actions/data/commands.json`, `actions/data/layout.json` seulement), joué seul avant V2. **Amendement 4 :**
`L4-C0` est **abandonné** : D14 et D18 conformes ; l'ordre des formes de D1 vit dans `core::shapeCatalog`, pas dans
les données de commande ; D19 reste au défaut, groupe `formes-texte` en fin de palette (choix de Jérôme du
2026-10-21). Aucune vague ne le joue. Une API supposée
qui diffère du code livré (A1 à A14) amende la section de la tâche qui l'emploie.

Fichiers partagés tenus par les dépendances : `ui/qml/Main.qml`, `ui/qml/ToolPalette.qml`, `ui/qml/PageContextMenu.qml`, `ui/CMakeLists.txt`
(L4-T1 → L4-T2 → 4b) ; `ui/qml/PageArea.qml` (L4-T2 → L4-C3 → L4-T7 → L4-T28 de 4b, amendement 5) ; `tools/src/selecttool.cpp`,
`selectiongeometry.*`, `tests/tst_tools_select.cpp` (L4-T9 → L4-T10) ; `ui/src/toolhost.cpp` (L4-T1 → L4-T11) ;
`ui/src/pageoptions.cpp` (L4-T1 → L4-T13 → 4b) ; `ui/qml/GridDesigner.qml` (L4-T2 → L4-T13) ;
`ui/qml/ShapePalette.qml` (L4-T2 → L4-T12) ; `ui/qml/SelectionOverlay.qml` (lot 3 → L4-T10) ;
`core/include/actcraft/core/shapegeometry.h`, `core/src/shapegeometry.cpp`, `tests/tst_core_shapes.cpp` (L4-T1 → L4-T3
→ L4-T31) ; `tools/include/actcraft/tools/shapetool.h`, `tools/src/shapetool.cpp`, `tests/tst_tools_shape.cpp` (L4-T1
→ L4-T8 → L4-T31) ; `tools/include/actcraft/tools/userstate.h`, `tools/src/userstate.cpp` (L4-T1 → L4-T11, si D2 ou D3 reste à poser) ;
`bench/src/scenarios.cpp`, `bench/src/runner.cpp`, `bench/include/actcraft/bench/result.h`, `bench/src/result.cpp`
(L4-T14 → L4-T29) ; `core/include/actcraft/core/model.h`
(L4-T1 → L4-T27) ; `formats/src/nativejson.cpp` (L4-T6 → L4-T27) ; les talons de L4-T1 → leur tâche.

## Dépendances aux autres lots — API supposées (à revérifier à l'ouverture du lot, L4-T0)

| # | API supposée | Source (plan, tâche) | Employée par |
|---|---|---|---|
| A1 | `tools::Tool`, `tools::ToolContext` (`document`, `ink`, `strokes`, `lift`, `selection`, `view`, `commands`, `user`), `DocumentEditor::page/pageIndex/execute/lift/drop/isLifting`, `LiftSurface::show/setTransform/clear` | socle L0-S6 (squelette du lot 3), rempli par L3-T6 | L4-T8, L4-T9, L4-T10, L4-T22 |
| A2 | `tools::SelectionView` (`corners`, `handles`, `bar`, `handlesVisible`, `rubberBand`) ; `L4-T1` y ajoute `smartPoints` à la fin | socle L0-S6, lot 3 L3-T1 | L4-T10, L4-T11 |
| A3 | `tools::UserState` (`settings`, `setSettings`, `currentTool`, `selection`) et `tools::defaultSettings` ; `L4-T1` y ajoute `shapeSettings`, `textSettings` | socle L0-S6 (D-S10), lot 3 L3-T6 | L4-T8, L4-T11, L4-T21 |
| A4 | `tools::ToolRegistry::add`, `registerBuiltinTools` (« select », « pen », « highlighter », « eraser ») ; `L4-T1` y ajoute « shape », « text », « text-vertical » | lot 3, L3-T6 | L4-T1, L4-T11 |
| A5 | `ui::ToolHost` (type QML `Tools`, objet `appTools`), `activate`, `selectionFrame`, `settingsFile` | lot 3, L3-T14 | L4-T11, L4-T12 |
| A6 | `ui::DocumentController::execute`, `document()`, `liftObjects`, `dropLifted`, `editAboutToApply`, `shownObjectCount`, **`addChangeObserver`** (D-S7 ; remplace `setCacheObserver`, espions par `core::applyToCache(changes, currentIndex, spy)`) ; `L4-T1` y ajoute `showPage`, `pageContentChanged` ; `insertText` et l'ajout d'une ressource sont l'API d'insertion du lot 7, **hors** de `documentcontroller.*` (D-S8) | lots 2 et 3 (L3-T12), socle L0-S5 | L4-T1, L4-T11, L4-T13, 4b (H4-18, ancien H4-11) |
| A7 | `ui::PageView::tools`, `cacheRefreshCount`, `pageToScene`, `sceneToPage` (L7-T2, posé par L0-S ?), `addInputCover(QQuickItem*)` (L0-S) | lot 3 (L3-T13), socle L0-S | L4-T2, L4-T13, 4b |
| A8 | `core::objectAt`, `core::objectsTouching`, `core::kHitToleranceScreen` | lot 3, L3-T3 | L4-T10, L4-T21 |
| A9 | `core::TransformObjectsCommand`, `RemoveObjectsCommand`, `ReplaceObjectsCommand` (socle L0-S : une seule commande de remplacement) | lot 3, L3-T4 ; socle L0-S | L4-T9, 4b |
| A10 | Sous-menus `{"submenu": id, "label", "items"}` (jeton `>id`, `MenuSpec::submenus`) ; `Layout::objectBar` et `Layout::contextMenu` (tableaux de chaînes au socle) ; filtrage par plateforme | socle L0-S4 (forme du lot 3) | L4-T1, L4-T20 |
| A11 | Joueur de scénarios : `UiStepKind` … `CheckEdge, Touch, Pinch, Pointer, DoubleClick` (rangs figés par `stepKindRanksFrozen`, que L4-T14 étend à `TypeText`), `expect` avec tolérance ; lignes `POINTEUR`, `DOUBLECLIC` | socle L0-S2 | L4-T12, L4-T14, 4b |
| A12 | Ordre des couches de `PageArea.qml` (`page` : `fixedPage`, `pageCache`, `pendingLayer`, `liftLayer`, `ink`, `pageOverlays`, liste exacte de `socleLayerOrder`) ; `selectionOverlay`, `toolCursor` ; `Main.qml` : `appTools` | socle L0-S9 | L4-T2 (amende `socleLayerOrder`) |
| A13 | `Change::Kind::PageSettingsChanged` (dernier de l'énumération), ignoré par `applyToCache` (I3) ; tout `switch` sur `Change::Kind` le traite (`/we4062`) | socle L0-S3 (D-S5) | L4-T4, L4-T27 |
| A14 | `PageContextMenu.qml` (`popupAt(x, y)`) lit `commands.contextMenu` (`["tool.select", "tool.pen", "-", "recognition.convert-to-text"]`, filtré par plateforme) | socle L0-S4, L0-S9 ; lot 3 (L3-T16) | L4-T1, L4-T2, L4-T20 |

**Lot 7 (session B, en parallèle).** Fichiers chauds communs : `actions/data/commands.json`, `layout.json`,
`Main.qml`, `PageArea.qml`, `ToolPalette.qml`, `PageContextMenu.qml`, `ui/CMakeLists.txt`, `tests/*.cmake`,
`documentcontroller.*`, `formats/src/flipchart.cpp`, `formats/src/flipchartids.h` (L7-T1). Règle du lot 3, reprise : les entrées du lot 4 forment des **blocs contigus** (commandes du
lot 4 groupées en fin de liste, groupes de palette nommés `formes-texte` et `pages`, entrées de menu en fin de
menu) ; le lot qui fusionne le second se remet à jour sur l'autre et juxtapose, sans réordonner. Les textes posés
par le lot 7 (API d'insertion du lot 7, hors de `documentcontroller.*`, D-S8) sont des `core::Text` ordinaires : l'outil Texte du lot 4
les rouvre et les modifie (`textOfLot7IsEditable`, L4-T21). L'effet de changement de page du lot 7 ne joue pas sur
les sauts du navigateur de pages (`PagesController::goTo`), comme le dit le plan du lot 7.

**Lot 10.** Interface de cache inchangée. Limite de DA11 tenue (contrainte 1) ; levée pour la grille du dessus
(L4-T7) ; scénarios de banc demandés (« forme sur page annotée », « grille au-dessus ») livrés par L4-T14.

**Lots 5, 6, 11.** Points de forme (F-527), couper / copier / coller d'objets, onglets de documents (glisser des
pages vers un autre onglet, F-011, F-803) : lot 5. Navigateurs d'objet, de notes, de propriétés, de ressources,
d'action, panneau flottant, persistance des réglages, profils : lot 6. Les icônes de ces navigateurs sont posées
**grisées** dans la rangée du panneau dès le lot 4 (F-040), sans navigateur derrière. Mise en page étroite : lot 11.

## Ce que l'ancien dépôt dit déjà (relevé pour ce plan, reformulé)

Lu dans `C:\Users\JeromeDenis\REA - Activinspire` ; L4-T0 le complète par le corpus et par le binaire.

1. **Formes.** L'application de référence offre 22 formes, dans cet ordre : ligne, ligne horizontale, ligne
   verticale, ligne brisée, zigzag, chevron, rectangle, rectangle arrondi, carré, ellipse, cercle, triangle,
   losange, pentagone, hexagone, étoile, flèche, bulle, octogone, étoile à quatre branches, explosion, explosion à
   douze pointes (`app/actcraft/canvas/items.py:421-428`). Défaut : la ligne. Carré et cercle restent carrés ;
   Maj garde un carré, ou contraint une ligne à des multiples de 45° ; ligne horizontale et verticale fixent un
   axe (`canvas/tools.py:822-868`). Un simple clic (moins de 3 px) ne pose aucune forme. Le point d'appui et le
   point courant sont accrochés à la grille pendant le tracé. La forme va sur la couche intermédiaire.
   **Amendement 4 (D1) :** les rangs 4 à 6 de cette liste viennent de noms lus sur des icônes ; la 3.5.18 y met une
   forme à main levée, une ligne brisée posée point par point et une courbe posée point par point.
   **Amendement 5 (D5) :** dans la 3.5.18, Maj n'a aucun effet sur le tracé (L4-O2, avec un témoin) : ni carré, ni
   45° ; carré, cercle et lignes à axe fixe restent tenus par leur préréglage.
2. **Texte.** Cliquer avec l'outil Texte sur un texte l'édite ; ailleurs, un texte neuf au point cliqué, texte de
   départ tout sélectionné, police et taille courantes (`canvas/tools.py:942-960`). Barre de mise en forme :
   `canvas/textbar.py`. Le texte de départ de la référence n'est pas repris : le nôtre est « Votre texte » (N1-12).
3. **Grille.** Accrochage : intersection la plus proche parmi les niveaux qui accrochent ; sans accrochage autorisé,
   le point reste tel quel ; grilles en losanges : recherche parmi les croisements voisins
   (`canvas/grid.py:221-251`). Lignes nettes de 1 px (lot 10, P2).
4. **Pages.** Réordonner : une vignette déposée sur une autre prend sa place et devient courante ; insertion qui
   garde le défilement des vignettes (`docs/tests/plan-comparatif.md:24`).
5. **Formes palette.** Panneau ancré à droite qui réduit la zone des pages, nuancier de 2 × 4, formes au pas de
   38 px (`docs/tests/plan-comparatif.md:18`). Nos dimensions viennent de notre thème (décision 23).

## Écarts au cadrage (repris à la porte de fin du lot 4)

1. **Volume.** 20 tâches estimées (amendement § 6) → 31, en deux sous-lots : deux réservations, la vérification
   des API, la grille en surcouche (limite de DA11), deux scénarios de banc, le contrôleur de miniatures, la boîte
   du fond (écart E2). Une tâche « principes » de plus est probable (amendement § 4 : « lot 4 + 1 ») : L4-T0 la
   crée par l'amendement si un principe du corpus n'entre dans aucune tâche (fichiers fixés par l'amendement).
   **Amendement 4 :** 34 lignes — L4-T31 (P37), L4-C1 (D16) et la séance d'observation L4-O1 (point 11 de L4-T0).
   **Amendement 5 :** 38 lignes — la seconde séance L4-O2 et les correctifs L4-C3 (D17), L4-C2 (validation du texte
   avant une commande de pages) et L4-C4 (identifiant d'un objet inconnu).
2. **Courbe de Bézier et forme libre** (F-240 les cite ; la référence ne les a pas) : L4-T0 relève si la 3.5.18
   les a dans sa palette ; si oui, l'amendement ajoute une tâche `tools` (tracé en plusieurs clics), sinon H4-2.
   **Amendement 4 (D1) :** pas de Bézier ; la forme à main levée existe, avec la ligne et la courbe brisées : L4-T31.
3. **Panneau flottant** (F-041) : ancrage à gauche ou à droite au lot 4 ; flottant au lot 6 avec la palette
   flottante (même mécanique) — H4-5.
4. **Presse-papiers des pages** interne au document au lot 4 ; entre documents et onglets au lot 5 (F-011).
5. **Entrées du menu de page hors lot** (capture de bureau : lot 7 ; exporter la page : lot 5 ; ajouter à la
   bibliothèque, plus de modèles : lot 6 ; question : exclue) : absentes du menu jusqu'à leur lot.
6. **Écart E10** (taille personnalisée en hauteurs de page) : tranché par L4-T0 ; s'il diffère de F-700, correctif
   `L4-C1` (boîte `ui/qml/PageSizeDialog.qml` et sa commande), ajouté par l'amendement. **Amendement 4 (D16) :**
   F-700 diffère (pixels pour un cadre et nombre de cadres en largeur et en hauteur) : `L4-C1` ajouté au plan 4b.

## Scénarios ajoutés

- **S7.1 — Formes (lot 4).** `captures.ps1 -Scenario formes` : palette ouverte par `tool.shape` ; rectangle tracé
  du coin (contour noir de 4 relevé par `PIXEL`) ; cercle tracé du centre (`shape.draw-from-center`), resté rond ;
  un clic simple ne pose rien (nombre d'objets relu) ; « sans remplissage » : l'intérieur reste blanc ; flèche
  sélectionnée : son point intelligent est visible et le glisser élargit la tête (une étape, `appDocument.canUndo`).
- **S7.2 — Grille (lot 4).** `captures.ps1 -Scenario grille` : créateur de grille ouvert, pas de 50, accrochage ;
  un rectangle déplacé pose son coin bas gauche sur une intersection (`PIXEL`) ; « Masquer la grille » sur une
  grille du dessus : `pageView.cacheRefreshCount` inchangé ; sur une grille du dessous : + 1.
- **S7.3 — Texte (lot 4, 4b).** Texte posé par touches injectées, mis en gras par la barre, validé hors de la zone ;
  un texte vide n'est pas gardé ; un texte existant rouvert et modifié en une étape.
- **S7.4 — Navigateur de pages (lot 4, 4b).** Quatre pages ; Maj+clic sélectionne une plage, Ctrl+clic en retire
  une ; glisser deux pages en tête ; couper, coller avant la page active ; dupliquer ; menu de page ▸ Insérer après.
- **S7.5 — Banc du lot 4.** `bench.ps1` : `forme-page-annotee`, `grille-dessus`, `miniatures` (4b) mesurés comme
  les autres, `cache.refreshes` et figements relevés ; résultat, pas porte (DA7) ; hors limite en D3D11 à la
  clôture : H4-8 décide (tuiles L10-T7, cache par groupe de couches, ou tâche corrective).

## Couverture des scénarios (4a et 4b)

| Scénario | Tâches | Joué à la clôture |
|---|---|---|
| S7 — formes, texte, pages, grille | L4-T1, T2, T3, T5, T6, T8, T9, T11 ; 4b : T15 à T19, T21 à T23, T30, C2, C4 | oui : hors écran et à l'écran en D3D11 (`formes-texte-pages-grille`) |
| S7.1 — formes | L4-T3, T8, T10, T11, T12, T31 ; L4-O1, L4-O2 (Maj, D5) | oui |
| S7.2 — grille | L4-T4, T5, T7, T9, T13, C3 ; L4-O1 (grille invisible, D6) | oui |
| S7.3 — texte | 4b : L4-T16, T21, T22, T23 | oui |
| S7.4 — navigateur de pages | 4b : L4-T15, T17, T18, T19, T20 | oui |
| S7.5 — banc du lot 4 | L4-T7, T14, C3 ; 4b : T29 ; L4-O1, L4-O2 (encre en cours et grille du dessus, D17) | oui : banc complet D3D11 plein écran |
| S15 — page allongée et zones | L4-T1, T2, T4, T6 ; L4-O1 (D10, D11), L4-O2 (D11) ; 4b : T26, T27, T28, C1 | oui |
| S16 — texte de haut en bas | L4-T1, T2, T6 ; 4b : T24, T25 | oui |
| S30 — tâche d'ouverture | L4-T0, L4-O1, L4-O2 (observations demandées par L4-T0 et par L4-O1) | contrôlé à la relecture : rapport, relevé brut, `NOMS OK` |
| S13 (lot 10) — non-régression du cache | toutes les tâches de `render/ ui/ tools/` | oui : `pauses`, `cache-differe`, S6 rejoués |

## Carte des fichiers

| Fichier | Responsabilité | Talon / API (L4-T1) | Rempli par |
|---|---|---|---|
| `docs/verifications/2026-10-18-lot4-principes.md` | constats, décisions D1 à D19, table A1 à A14, N3 | — | L4-T0 |
| `actions/data/commands.json`, `layout.json` | 47 commandes du lot ; menus Insérer (sous-menu « Page », en fin), Outils (`other-tools`), Affichage ; palette ; `contextMenu` | L4-T1 | L4-T1 |
| `actions/…/layout.h`, `actions/src/layout.cpp` | `Layout::contextMenu` accepte les sous-menus | L4-T1 | L4-T1 |
| `core/…/model.h`, `core/src/model.cpp` | `Shape::preset`, `Shape::parameters`, `Text::vertical`, `Page::baseHeight` ; empreinte | L4-T1 | L4-T1 (zones : L4-T27) |
| `core/…/shapegeometry.h`, `shapeedits.h` | catalogue, boîte, lignes, points intelligents, `SetShapeParametersCommand` ; geste de chaque préréglage (amendement 4) ; lignes posées au clic et main levée | talon | L4-T3, L4-T31 |
| `core/…/pagesettings.h` | `SetGridCommand`, `SetBackgroundCommand`, `ExtendPageCommand`, `gridDrawsUnderObjects` | talon | L4-T4 |
| `core/…/gridsnap.h` | `nearestGridPoint`, `snapAnchor`, `snapDelta` | talon | L4-T5 |
| `core/…/pageedits.h` | `MovePagesCommand`, `RemovePagesCommand`, `InsertPagesCommand`, `withFreshIds` | talon | L4-T15 |
| `core/…/textedits.h` | `SetTextCommand`, `textBaseline` | talon | L4-T16 |
| `core/…/pagezones.h` | `SetPageZonesCommand` | talon vide (fichier réservé) | L4-T27 |
| `formats/src/nativejson.cpp` | clés `preset`, `parameters`, `vertical`, `baseHeight` | — | L4-T6 (zones : L4-T27) |
| `formats/src/flipchart.cpp`, `flipchartids.h` | texte de haut en bas lu | — | L4-T24 |
| `render/…/gridoverlayitem.h` | surcouche de la grille du dessus (type QML `GridOverlay`) | talon | L4-T7 |
| `render/…/fixedpageitem.*` | grille du dessus retirée ; différé rétabli | — | L4-T7 |
| `render/src/pagepainter.cpp`, `paintedrenderer.cpp` | texte de haut en bas dessiné | — | L4-T24 |
| `tools/…/shapetool.h`, `texttool.h` | outils Forme, Texte, Texte de haut en bas | talon | L4-T8, L4-T31, L4-T22 |
| `docs/verifications/2026-10-21-lot4-observation.md` | verdicts de la séance d'observation (D5, D6, D10, D11, D17) | — | L4-O1 |
| `docs/verifications/2026-10-21-lot4-observation-2.md` | verdicts de la seconde séance (D5 avec témoin, D17 bis, D11) | — | L4-O2 |
| `ui/qml/PageSizeDialog.qml`, `ui/qml/FileActions.qml` | taille personnalisée en pixels d'un cadre et en nombre de cadres (D16) | — | L4-C1 (4b) |
| `tools/…/userstate.h`, `toolcontext.h`, `toolregistry.cpp` | `ShapeSettings`, `TextSettings`, `SelectionView::smartPoints`, outils inscrits | L4-T1 | L4-T1 |
| `tools/src/selecttool.cpp`, `selectiongeometry.*` | accrochage au dépôt, points intelligents | — | L4-T9, L4-T10 |
| `ui/…/toolhost.h`, `ui/src/toolhost.cpp`, `tooladapters.*` | réglages de forme, contour des préréglages, points intelligents | API | L4-T11 |
| `ui/…/pageoptions.h` | grille, fond, extension, zones (type QML `PageSettings`, objet `appPageSettings`) | API | L4-T13, L4-T20, L4-T26, L4-T28 |
| `ui/…/pagethumbnails.h` | miniatures (types QML `PageThumbnails`, `ThumbnailImage`) | API | L4-T17 |
| `ui/…/pagescontroller.h` | pages (type QML `Pages`, objet `appPages`) | API | L4-T18 |
| `ui/…/textediting.h` | édition du texte (type QML `TextEditing`, objet `appText`) | API | L4-T21, L4-T25 |
| `ui/…/documentcontroller.h` | `showPage`, `pageContentChanged` | L4-T1 | L4-T1 |
| `ui/qml/*.qml` du lot | coquille, puis contenu | talon `Item {}` | L4-T2, puis leur tâche |
| `bench/src/scenarios.cpp`, `runner.cpp`, `uiscenario.*` | banc du lot, étape `typeText` | — | L4-T14, L4-T29 |
| `bench/scenarios/*.json` (7) | scénarios d'interface du lot | talon (`waitReady` seul) | leur tâche |
| `tests/tst_*.cpp` (16 neufs) | un par fichier neuf | talon `QSKIP` | la tâche du fichier |

## Contrats partagés entre tâches (noms figés)

Tous les noms des lots 1, 2, 3, 10 et de L0-S restent figés ; on ajoute, on ne renomme pas. Les en-têtes publics
du lot sont **créés tels quels par L4-T1** (sa section, étape 5, donne leur texte complet) ; une tâche suivante les
lit dans son worktree, ne change pas une signature publique sans amendement du plan, et ajoute librement des
membres **privés**.

- Commandes (47, identifiants anglais, libellés à nous) : `tool.shape` (sans raccourci, D18 ; cochable), `shape.choose`
  (texte), `shape.filled` (cochable), `shape.fill-color` (couleur), `shape.draw-from-center` (cochable), `tool.text`
  (Ctrl+T, cochable), `tool.text-vertical` (cochable), `text.font` (texte), `text.size` (nombre), `text.size-up`,
  `text.size-down`, `text.bold`, `text.italic`, `text.underline`, `text.superscript`, `text.subscript`,
  `text.color` (couleur), `text.background` (couleur), `text.align-left`, `text.align-center`, `text.align-right`,
  `text.spacing-up`, `text.spacing-down`, `text.indent`, `text.outdent`, `text.bullets`, `text.select-all`,
  `text.symbol` (texte), `text.edit-at` (dans aucun menu ; argument texte facultatif `"x;y;v"`, coordonnées de page et `v` = `0` ou `1` ; sans argument : valider l'édition), `view.browsers`
  (Ctrl+B, cochable), `browsers.dock-left`, `browsers.dock-right`, `page.goto` (nombre), `page.insert-before`,
  `page.insert-after` (Ctrl+I), `page.cut`, `page.copy`, `page.paste`, `page.duplicate`, `page.delete`,
  `page.background`, `grid.designer` (Ctrl+Maj+G), `grid.hide` (cochable), `grid.snap` (cochable),
  `view.page-extension` (cochable), `page.extend`, `page.zones` (texte `"<horizontales>x<verticales>"`). Aucune
  commande de page n'a Ctrl+X/C/V/D ni Suppr : ces raccourcis restent à l'édition d'objets (lots 3 et 5) ; dans le
  navigateur de pages, le QML les traduit en commandes de page quand il a le focus.
- Identifiants d'outil : `shape`, `text`, `text-vertical` (en plus de ceux du lot 3).
- Préréglages de forme (ordre de la palette, D1 ; amendement 4) : `line`, `line-horizontal`, `line-vertical`,
  `freeform`, `polyline`, `curve-chain`, `rectangle`, `rounded-rectangle`, `square`, `ellipse`, `circle`, `triangle`, `diamond`,
  `pentagon`, `hexagon`, `star`, `arrow`, `callout`, `octagon`, `star-four`, `burst`, `burst-twelve`.
- Objets QML lus par les scénarios : `appTools` (`currentTool`, `shapePreset`, `shapeFilled`,
  `shapeFromCenter`), `appDocument` (`shownObjectCount`, `pageCount`, `currentIndex`, `pageSize`, `canUndo`),
  `appPageSettings` (`gridVisible`, `gridSnap`, `gridAbove`, `pageHeight`, `zones`), `appPages`
  (`selection`, `canPaste`), `appThumbnails` (`renderCount`, `pendingCount`), `appText` (`active`, `vertical`,
  `bold`, `plainText`), `pageView` (`cacheRefreshCount`), `browserPanel` (`visible`, `side`), `shapePalette`,
  `textFormatBar`, `verticalTextBar`, `gridDesigner`, `backgroundDialog`, `pageExtensionHandle`, `pageZones`,
  `gridOverlay`.
- Ordre des couches dans `PageArea.qml` (A12) : `fixedPage`, `pageCache`, `pendingLayer`, `liftLayer`, `ink`,
  `pageOverlays` (socle) ; `gridOverlay` s'y insère **entre `ink` et `pageOverlays`** (D17, amendement 4), chaque
  enfant de `page` à `z() == 0`, `socleLayerOrder` amendé par L4-T2 (L4-T2 a posé `gridOverlay` sous `ink` d'après
  L4-O1 ; L4-C3 remet ce rang, amendement 5) ; `pageZones` et
  `pageExtensionHandle` dans `pageOverlays` ; `textEditor` à côté de `selectionOverlay`.
- Constantes : `core::kShapeMinDragScreen = 3.0` ; `core::kExtendRatio = 0.10` ; `core::kMergeGrid = 4` (libre, V7) ; `core::kChainDoubleClickScreen = 6.0`, `core::kChainDoubleClickNs = 500'000'000`, `core::kCurveChainSamples = 12` (L4-T31) ;
  `ui::PageThumbnails::kSettleMs = 400` ; `ui::PageThumbnails::kDefaultWidth = 160`.

## Règles communes à toutes les tâches (jointes à chaque section par l'orchestrateur)

Celles des plans 2a, 2b, 10 et 3 s'appliquent toutes, mot pour mot
(`docs/plans/2026-10-15-livraison-complete-lot2a-PLAN.md`, `…-lot2b-PLAN.md`, `…-lot10-PLAN.md`,
`…-lot3-PLAN.md`, « Règles communes ») : processus enfant pour toute commande PowerShell, `git -C`, `format.ps1`
avant la preuve, pièges de Clang (`-Wshadow`, `-Wswitch` : tout `switch` sur une énumération étendue est complété,
conversions explicites, pas d'agrégat partiel par position), pas de `std::visit` direct sur `PageObject`, **aucune
API privée de Qt**, propriété intellectuelle, `samples/` en lecture seule, commits explicites en français terminés
par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`, arrêt par PID, fils de Qt Quick (tout état lu par
`updatePaintNode` est écrit sur le fil principal), moc et chaînes brutes à apostrophe, talons remplacés en gardant
les déclarations publiques de L4-T1 (la fonction `reserved()` du test talon est supprimée). S'y ajoutent :

- **Amendement 1 — énumérations** : tout `switch` sur `core::ObjectKind` traite `Equation` (comme `Text` pour
  l'emprise et la désignation) ; tout `switch` sur `core::Change::Kind` traite `PageSettingsChanged` (sans effet de
  cache). C4062 est une erreur (`/we4062`, L0-S3) : un `switch` incomplet échoue en local comme en CI.
- **Amendement 1 — HTML de document** (la règle « **Sécurité** : `QTextDocument::setHtml` charge les ressources citées… » du fichier d'état (`docs/sessions/livraison-complete-etat.md`)) : jamais `setHtml`, ni
  `TextEdit.RichText` / `Text.RichText`, sur un HTML venu d'un document sans passer par un `QTextDocument` dont
  `loadResource` est neutralisé (garde de L2-T11, `render/src/pagepainter.cpp:73`) ou par un HTML assaini (L4-T21).
  Un `<img src="file://hôte/…">` d'un `.flipchart` déclencherait une requête SMB.
- **Amendement 1 — espions de cache** : `DocumentController::addChangeObserver` (D-S7) et
  `core::applyToCache(changes, controller.currentIndex(), spy)` ; `setCacheObserver` n'existe plus.
- **Graphe d'abord** : `graphify query "…"` dans le worktree avant de lire les fichiers ; pour l'application de
  référence, `graphify query "…"` lancé **dans** `C:\Users\JeromeDenis\REA - Activinspire`.
- **Jeton** `C:\Users\JeromeDenis\.claude\scripts\jeton.ps1` : `-Nom tests` pour construire, tester, jouer le banc
  ou les captures hors écran ; `-Nom ecran` pour toute fenêtre visible ; une commande par prise ; attendre la
  notification, ne pas sonder.
- `<wt>` = `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L4-T<n>` ; `<id>` = `L4-T<n>`.
- **Commande ciblée** (rouge, vert d'un seul test) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande '. ./scripts/dev-env.ps1; cmake --build --preset debug *> .tmp/build.txt; ctest --preset debug -R ''<motif>'' --output-on-failure *> .tmp/ctest.txt'; Select-String -Path .tmp/build.txt,.tmp/ctest.txt -Pattern 'error|tests passed|tests failed|No tests were found' | Select-Object -Last 4"
  ```
  Premier passage dans un worktree neuf : la même commande avec `-Commande 'pwsh -NoProfile -File ./scripts/build.ps1 *> .tmp/build.txt'`.
- **Fonctions d'un test** :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; . ./scripts/dev-env.ps1; & build/debug/tests/<test>.exe -o .tmp/<test>.txt,txt; Select-String -Path .tmp/<test>.txt -Pattern '^PASS   :|^FAIL!  :|^SKIP   :|^Totals' | Select-Object -Last 40"
  ```
  Attendu pour un talon rempli : chaque fonction nommée par la table en `PASS`, aucun `SKIP`,
  `Totals: <k> passed, 0 failed, 0 skipped`, k = nombre de fonctions du test + 2 (Qt Test compte `initTestCase` et
  `cleanupTestCase`), k ≥ 3 (amendement 4).
- **Preuve de tâche** (forme bornée du profil, sous le jeton) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1'; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2; Select-String -Path .tmp/test.txt -Pattern '<nom_du_test> .*Passed' | Select-Object -Last 1"
  ```
  Attendu : `100% tests passed, 0 tests failed out of N` (N = 136 + n_B dès L4-T1, amendement 1 § 13 ; n_B relevé de nouveau par l'orchestrateur si le lot 7 fusionne en
  cours de lot, amendement 2), `TESTS OK`, puis la ligne
  `Passed` du test nommé.
- **Banc sans écran** (toute tâche qui touche `render/`, `input/`, `tools/` ou `ui/` ; résultat, pas porte) : la
  commande et la lecture du JSON du plan du lot 3 (« Banc sans écran »), avec `-Proprietaire <id>`. Attendu :
  `BUDGET TENU` ou `BUDGET DEPASSE` (jamais `MESURE INVALIDE`), `content_check.ok = True` ; nom du JSON joint.
- **Preuve visuelle hors écran** (jeton `tests`) : lectures `-Lire "<objet>.<propriété>=<attendu>"` d'abord, puis
  description des captures :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario <nom> -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire ''<objet.propriété=attendu>'' > .tmp/captures.txt 2>&1'; Select-String -Path .tmp/captures.txt -Pattern '^(COMMANDE|TRAIT|TOUCHE|TEXTE|DOIGT|PINCEMENT|POINTEUR|DOUBLECLIC|ATTENTE|PRET|FENETRE|LU|PIXEL|BORD|CAPTURE) |^SCENARIO |CAPTURES (OK|EN ECHEC)' | Select-Object -Last 16"
  ```
  Motifs d'échec : `CAPTURES EN ECHEC`, `SCENARIO EN ECHEC`, `SCENARIO INVALIDE`, toute ligne `ECART` ou `ECHEC`,
  `FENETRE …`. La ligne `TEXTE` existe après L4-T14.
- **Mutation** (toute tâche qui en demande une) : `Copy-Item` garde l'ancienne date du fichier et ninja ne
  recompile pas. Après avoir posé ou retiré la mutation, remettre la date
  (`(Get-Item <fichier>).LastWriteTime = Get-Date`) ou remettre le fichier par `git -C <wt> checkout -- <fichier>`
  (qui la change). Constater le rouge, remettre, constater le vert.
- **Dépôt public** : le contrôle des noms ne lit que les fichiers suivis par git. Avant de le lancer, `git -C <wt>
  add` de chaque fichier neuf ou modifié ; puis
  `python -I "C:/Users/JeromeDenis/REA - Activinspire/tools/controle_noms_actcraft.py" <wt>` → `NOMS OK` ; le
  relancer après le commit. Libellés à nous (N1-12), aucun nom de l'original, aucune icône d'origine, aucun titre de
  vidéo, aucun nom de paperboard réel.

---

## Tâches

<a id="tache-l4-t0"></a>
### Tâche L4-T0 — Ouverture : vidéos, REA et ancien dépôt confrontés au lot ; API revérifiées

**Objectif.** Décision 0100 et règle `Lx-T0` (amendement § 4), sur le modèle de L10-T0
(`docs/verifications/2026-10-18-lot10-principes.md`) : relever dans le corpus tous les principes des fonctions du
lot (formes, palette, points intelligents, texte, barre de mise en forme, texte de haut en bas, panneau des
navigateurs, navigateur de pages, menu de page, fond de page, grille, accrochage, page allongée, zones), plus les
**8 constats** affectés au lot 4 par le tableau du § 4 de l'amendement (1 « Interface et profils », 3 « Stylo,
formes, couleurs », 1 « Texte et reconnaissance », 3 « Pages, fond, grille ») et les écarts **E1, E2, E10, E12** ;
trancher sur la 3.5.18 par REA ; écrire en nos mots les exigences, les **décisions D1 à D19**, puis **revérifier
les API A1 à A14** contre le code livré par les lots 3, L0-S (et 7 s'il a fusionné).

**Décisions à rendre** (chacune : valeur retenue et source, ou « non tranché » → hypothèse de niveau 1) :
- D1 — Liste et ordre des formes de la palette de la 3.5.18 ; défaut ; courbe de Bézier et forme libre présentes
  ou non (écart 2).
- D2 — Remplissage à la création (E1) : rempli par défaut, choix « sans remplissage », couleur de remplissage par
  défaut.
- D3 — Contour par défaut (couleur courante ou noir ; 4 px) ; nom par défaut d'une forme.
- D4 — Points intelligents : quelles formes en ont, combien, bornes (tête de flèche, rayon des coins, creux des
  étoiles, pointe de la bulle).
- D5 — Tracé depuis le centre : réglage gardé où (réglage de l'application ou de la palette) ; Maj.
- D6 — Accrochage (F-622) : bascule « Accrocher à la grille » = « Autoriser l'accrochage » de la grille de la page,
  ou état propre de l'application ; ancrage (coin bas gauche ; ligne de base d'un texte) ; forme en cours de tracé.
  **Amendement 1 :** un texte est-il aimanté ? La spécification dit oui, par sa ligne de base
  (`fonctionnalites.md:768`) ; la relecture du plan dit non. *Recommandation :* garder la ligne de base (F-622) sauf
  si la 3.5.18 montre en statique qu'un texte n'est pas aimanté ; *raison :* la spécification est notre seule source
  écrite, et `textUsesItsBaseline` (L4-T5) se retire sans toucher l'API si D6 dit non (L4-T9 n'appelle alors
  `snapAnchor` que pour les objets autres que `Text`).
- D7 — Texte : police et taille par défaut (N2-3), couleur, fond ; cadre et poignées ; texte vide ; validation.
- D8 — Barre de mise en forme (F-100) : contenu et ordre, pas de la taille (2 points), interligne et retrait
  (pas), symboles offerts, place de la barre (haut ou bas). **Amendement 1 :** raccourcis pendant la frappe —
  Ctrl+B (`view.browsers`) et Ctrl+I (`page.insert-after`) quand le `TextEdit` a le focus. *Recommandation :* pendant
  l'édition, l'éditeur prend Ctrl+B, Ctrl+I et Ctrl+U (`Keys.onShortcutOverride` du `TextEdit`, `event.accepted =
  true`) et les traduit en `text.bold`, `text.italic`, `text.underline` ; hors édition, les raccourcis de commande
  restent. *Raison :* c'est l'attente de tout éditeur de texte, sans changer aucune donnée de commande (pas de
  `L4-C0`) ; preuve par L4-T23 sous `appText.active`.
- D9 — Texte de haut en bas (F-301 b) : lettres empilées ou ligne tournée ; barre propre ; lecture du type dans les
  `.flipchart` (identifiants à décrire dans notre documentation hors dépôt).
- D10 — Page allongée (F-301 a, E12) : place de la commande, pas de 10 % fixe (hauteur d'origine) ou cumulé, la
  poignée reste-t-elle d'une page à l'autre, une étape d'annulation par clic.
- D11 — Zones de page (F-603) : comportement entier (dessin des séparations, effet sur les objets, l'accrochage, la
  présentation), réglage, valeurs bornées. **Aucune ligne de code avant cette décision.**
- D12 — Panneau des navigateurs : icônes et ordre (sans le vote), ancrage, Ctrl+B, largeur minimale et maximale,
  menu propre à chaque navigateur.
- D13 — Navigateur de pages : taille des miniatures (réglette), sélection multiple, fantôme d'insertion, pages
  restées sélectionnées après le dépôt, coller avant la page active, menu sur plusieurs pages.
- D14 — Menu de page (F-771) : entrées et ordre de la 3.5.18 pour les fonctions du lot.
- D15 — Fond de page (F-610, E2) : un bouton « Terminé » ou appliquer puis valider ; options du lot 4 (couleur,
  dégradé, image) ; les autres (capture, superposition de bureau) au lot 7.
- D16 — E10 : taille personnalisée d'un paperboard neuf (pixels ou hauteurs de page) ; correctif `L4-C1` si la
  3.5.18 diffère de F-700.
- **D17 (amendement 1)** — Encre en cours et grille du dessus : la couche `ink` (trait en cours) est au-dessus de
  `gridOverlay` si celle-ci suit `liftLayer` ; le trait passe alors sous la grille au relâché et change d'aspect.
  *Recommandation :* `gridOverlay` entre `ink` et `pageOverlays` ; *raison :* la grille « toujours visible » (F-620)
  couvre aussi le trait en cours, l'aspect ne change plus au relâché, et rien n'est redessiné (la surcouche ne
  dépend pas de l'encre). Si la 3.5.18 montre l'inverse, `gridOverlay` entre `liftLayer` et `ink`. L4-T2 applique.
- **D18 (amendement 1)** — Raccourci de `tool.shape` : Ctrl+Shift+S est à `file.save-as`. *Recommandation :* aucun
  raccourci ; *raison :* la palette et le menu y suffisent, et toute combinaison Ctrl+lettre libre reste disponible
  pour les lots 5 et 6 ; si la 3.5.18 en a un libre chez nous, correctif `L4-C0` (`commands.json` seul) avec
  `noShortcutTwice` vert.
- **D19 (amendement 3)** — Place du groupe de palette `formes-texte` : en fin de palette, ou juste après `outils`
  (avant `edition`, posé par le socle). *Recommandation :* en fin de palette ; *raison :* la règle des blocs contigus
  avec le lot 7 (entrées du lot 4 en fin, « le second qui fusionne juxtapose sans réordonner ») évite un conflit avec
  les groupes `edition` et `presentation` du socle, et `defaultLayoutMatchesLotOne` n'a qu'un élément à ajouter en
  fin. Si la 3.5.18 montre formes et texte à côté des outils de dessin et que la proximité compte pour Jérôme : après
  `outils`, par `L4-C0` (`layout.json`) et la liste attendue amendée. L4-T1 applique la recommandation.

**Sources.**
- Corpus : `C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\README.md` (« Principes absents »,
  « Écarts avec la spec ») et ses fiches `*.md`.
- Spécification : `C:\Users\JeromeDenis\REA - Activinspire\docs\spec\fonctionnalites.md` (F-040 à F-042, F-100,
  F-240 à F-242, F-250 à F-253, F-301, F-600 à F-603, F-610, F-620 à F-623, F-700, F-771, F-800 à F-803).
- Plan comparatif : `docs\tests\plan-comparatif.md`, lignes 18, 20, 24 et 33 (formes, texte, pages, grille).
- Ancien dépôt : `app/actcraft/canvas/grid.py`, `canvas/items.py`, `canvas/tools.py`, `canvas/textbar.py`,
  `canvas/poly.py`, `ui/navigator.py`, `ui/grid_designer.py` ; son graphe.
- Format : `docs\re\format-flipchart.md` (§ 6, § 7.4, § 7.5) — notre documentation.
- Binaire : skill `reverse-engineer-anything` sur `C:\Program Files\Activ Software\Inspire\`, **en statique
  seulement** (ressources des boîtes, chaînes, valeurs par défaut, raccourcis, types du flux objet). Aucune
  observation à l'écran sans prévenir Jérôme (mode autonome, point 3) ; si une décision l'exige, la marquer
  « non tranché, observation à l'écran demandée » et continuer.
- API : `graphify query "ToolContext LiftSurface SelectionView"`, `graphify query "ToolHost selectionFrame"`,
  `graphify query "UiStepKind Pointer DoubleClick"`, `graphify query "PageArea couches pendingLayer liftLayer"`,
  `graphify query "SetPagePresentationCommand Change"` dans le worktree, puis lecture des en-têtes trouvés.

**Fichiers.**
- Créer : `docs/verifications/2026-10-18-lot4-principes.md` (dans le dépôt, nos mots).
- Créer hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot4.md` (relevé brut).

**Test et commande bornée.** Pas de code. Preuves :
```powershell
Test-Path 'C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot4.md'   # True
git -C "C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L4-T0" add docs/verifications/2026-10-18-lot4-principes.md
python -I "C:/Users/JeromeDenis/REA - Activinspire/tools/controle_noms_actcraft.py" "C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L4-T0" | Select-Object -Last 1   # NOMS OK : <n> fichiers
pwsh -NoProfile -Command "`$f = 'C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\exclus-controle-titres.txt'; `$exclus = @(); if (Test-Path -LiteralPath `$f) { `$exclus = @(Get-Content -LiteralPath `$f -Encoding utf8 | Where-Object { `$_.Trim() }) }; Get-ChildItem 'C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\*.md' -Exclude README.md | ForEach-Object { `$t = (Get-Content -LiteralPath `$_.FullName -TotalCount 5 -Encoding utf8 | Where-Object { `$_ -like '# *' } | Select-Object -First 1); if (`$t) { `$t = `$t.Substring(2).Trim(); if (`$exclus -notcontains `$t) { git -C 'C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L4-T0' grep -l -i -F -- `$t } } }"   # aucune sortie
```

**Amendement 1.** La liste d'exclusion des titres n'est **jamais** écrite dans le dépôt : elle est lue dans
`exclus-controle-titres.txt` (hors dépôt, facultatif, un titre par ligne, UTF-8). Si la commande sort un fichier
parce qu'un titre est aussi un libellé courant à nous, la tâche réécrit d'abord notre texte ; si le mot est
inévitable, elle ajoute le titre à ce fichier hors dépôt et le note dans le relevé brut. Décisions à rendre : D1 à
**D19** (D6 et D8 étendues, D17 et D18 ajoutées). À la table A, vérifier en plus : `ArgumentKind::Text` et la clé
`"optional"` (L0-S4), `Layout::contextMenu` (L0-S4), `PageSettingsChanged` (L0-S3), `addChangeObserver` (L0-S5),
`socleLayerOrder` (L0-S9), `stepKindRanksFrozen` (L0-S2), l'état d'un contact publié par `input::PointerRouter` ou
`PageView` (pour L4-T17), les chemins de dépôt forcé d'un soulèvement dans `DocumentController` (pour L4-T21), et
**n_B** (tests du lot 7 fusionnés). Étape 6 : N3 attendu = 113.

**Interdits.** Aucun titre, aucune transcription, aucune citation de vidéo dans le dépôt (identifiants `Vnn`).
Aucun nom de l'original (classes, propriétés, libellés), aucun texte de l'aide, aucune capture. Aucune ligne de
code. Ne modifier ni `samples/` ni les fiches du corpus (seul `exclus-controle-titres.txt` peut être créé, hors
dépôt). Pas d'observation à l'écran sans annonce.

**Terminé quand.** Le rapport a une ligne par constat (principe → exigence en nos mots → tâche du lot 4 →
scénario, ou raison et lot destinataire), une section « Décisions » D1 à D19, une section « API » A1 à A14
(conforme, ou écart : signature réelle et tâche à amender), et la valeur **N3** (`ctest --preset debug -N` sur
`feat/livraison-complete` à jour : `Total Tests: <N3>`) ; le relevé brut existe et est cité ; `NOMS OK` ; la commande
des titres ne sort rien ; le rapport de tâche liste pour l'orchestrateur les amendements (tâche visée), les
correctifs `L4-C0` et `L4-C1`, et la tâche « principes » éventuelle (titre, fichiers, preuve).

**Étapes.**
- [ ] **Étape 1 : relever le corpus.**
  ```powershell
  Select-String -Path 'C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\*.md' -Pattern 'forme|rectangle|cercle|flèche|ligne|remplissage|contour|texte|police|gras|souligné|vertical|navigateur|miniature|vignette|page|réordonn|insérer|dupliquer|grille|accroch|aimant|fond|allong|extension|cadre|zone' -List | Select-Object -ExpandProperty Path
  ```
  Pour chaque fiche retenue : numéro `Vnn` (colonne « # » du `README.md`) et principe observé, en une phrase à nous.
- [ ] **Étape 2 : relever l'ancien dépôt.** Dans `C:\Users\JeromeDenis\REA - Activinspire` :
  `graphify query "outil des formes tracé carré ligne"`, `graphify query "outil texte barre de mise en forme"`,
  `graphify query "navigateur de pages vignettes réordonner"`, `graphify query "grille accrochage niveaux"` ;
  confirmer ou corriger le § « Ce que l'ancien dépôt dit déjà ».
- [ ] **Étape 3 : REA statique.** Palette des formes (liste, ordre, défaut, remplissage), points intelligents,
  valeurs par défaut du texte, contenu de la barre, texte de haut en bas (type du flux objet, propriétés), commande
  d'extension de page et son pas, zones de page (propriétés de page, dessin), boîte du fond, taille personnalisée.
  Constats bruts, noms de l'original compris, dans le relevé hors dépôt seulement.
- [ ] **Étape 4 : écrire le rapport.**
  ```markdown
  # Lot 4 — principes confrontés (vidéos, ancien dépôt, binaire) et API revérifiées

  Relevé brut (hors dépôt) : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot4.md`. Références `Vnn` :
  numéros des fiches du corpus. Constats écrits par nous. Tâche L4-T0, scénario S30.

  ## Constats

  | # | Principe (nos mots) | Source | Exigence pour ActCraft | Tâche | Scénario | Écart, raison ou lot destinataire |
  |---|---|---|---|---|---|---|
  | P1 | Un simple clic avec l'outil des formes ne pose aucune forme | ancien dépôt (outil des formes), Vnn… | aucune forme sous 3 px d'écran | L4-T8 | S7.1 | — |
  | … | | | | | | |

  ## Décisions

  | # | Question | Décision | Source | Tâche amendée |
  |---|---|---|---|---|
  | D1 | Liste et ordre des formes | … | … | L4-T3, L4-C0 |
  | … | | | | |

  ## API des lots 3, 7 et L0-S

  | # | API supposée par le plan | Code livré (fichier:ligne) | Conforme | Tâche à amender |
  |---|---|---|---|---|
  | A1 | `tools::ToolContext` … | `tools/include/actcraft/tools/toolcontext.h:…` | oui / non : … | … |
  | … | | | | |

  Tests au départ du lot : N3 = … (`ctest --preset debug -N`).
  ```
- [ ] **Étape 5 : API.** Pour A1 à A14 : `graphify query` (sources ci-dessus), puis lecture de l'en-tête ;
  recopier la signature réelle quand elle diffère.
- [ ] **Étape 6 : N3.**
  ```powershell
  pwsh -NoProfile -Command "Set-Location 'C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L4-T0'; . ./scripts/dev-env.ps1; cmake --preset debug *> .tmp/cfg.txt; ctest --preset debug -N | Select-String 'Total Tests'"
  ```
  (sous le jeton `tests`, comme la commande ciblée).
- [ ] **Étape 7 : contrôles.** Les commandes de « Test et commande bornée » ; corriger jusqu'à `NOMS OK` et sortie
  vide.
- [ ] **Étape 8 : commit**, puis relancer le contrôle des noms.
  ```powershell
  git -C <wt> add docs/verifications/2026-10-18-lot4-principes.md
  git -C <wt> commit -m "Lot 4 : principes des formes, du texte, des pages et de la grille confrontés aux vidéos, à l'ancien dépôt et au binaire ; API revérifiées (L4-T0)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t1"></a>
### Tâche L4-T1 — Réservation I : commandes, disposition, icônes, modèle, CMake, tests, en-têtes figés et talons

**Amendement 1 (relecture tour 1 : défauts 1, 2, 7, 8, 9, 13 ; mineurs icônes, `pagezones`, `baseHeight`,
couleurs).** Prime sur les étapes ci-dessous.
- Pas de clé `pageMenu` : le menu de page est **`contextMenu`** du socle (L0-S4), étendu aux sous-menus (F-771).
  `layout.h` : à la fin de `Layout`, `std::vector<MenuSpec> contextSubmenus; // sous-menus du menu contextuel (jeton
  « >id »)`. `layout.cpp` : `contextMenu` accepte des chaînes et des objets `{"submenu": id, "label", "items"}` (mêmes
  règles que les menus : jeton `">" + id`, sous-menu imbriqué refusé) ; l'erreur du socle « contextMenu : tableau de
  chaînes attendu » devient « contextMenu : tableau attendu » ; `objectBar` reste un tableau de chaînes.
  `ui/src/commands.cpp` : `contextMenu()` rend les sous-menus au format de `menus()` (`{submenu: true, id, label,
  items}`), filtrés par plateforme. Test `contextMenuWithSubmenu` dans `tests/tst_ui_commands.cpp` (un sous-menu
  `page-insert` de 2 éléments rendu après les 4 entrées du socle sous Windows).
- `lot4EntriesDeclared` et `editAtArgumentOptional` vont dans **`tests/tst_actions_registry.cpp`** ;
  `noShortcutTwice` est celui du socle (même fichier) : ne pas le recopier, il couvre les 47 commandes du lot.
- `tool.shape` **sans raccourci** (D18 tranchera ; `L4-C0` s'il en faut un). `shape.fill-color`, `text.color`,
  `text.background` : `"argument": "color"`. `text.edit-at` : `"argument": "text", "optional": true`.
- `layout.json` : `insert` → **ajouter en fin** `"-", {"submenu": "insert-page", "label": "Page", "items":
  ["page.insert-before", "page.insert-after"]}` (`insert.date-time` et `insert.equation` gardés en tête,
  `lot7EntriesPlaced` vert) ; `tools` → **rien au premier niveau** ; `tool.text-vertical` ajouté **en fin** des
  `items` du sous-menu `other-tools` ; `toolsMenuFollowsF750` (socle) amendé explicitement : sa liste attendue reçoit
  `tool.text-vertical` en fin d'`other-tools`, commentaire « amendement 1 du lot 4, F-750 » ; `contextMenu` → les
  entrées du socle gardées, puis en fin `"-", {"submenu": "page-insert", "label": "Insérer une page", "items":
  ["page.insert-before", "page.insert-after"]}, "-", "page.background", "grid.designer", "grid.hide", "grid.snap",
  "-", "page.cut", "page.copy", "page.paste", "page.duplicate", "page.delete"` (ordre revu par D14 via `L4-C0`).
- Icônes : les 34 noms de l'étape 4 ajoutés en fin de `kIcons` (`tests/tst_ui_theme.cpp:17`) ;
  `iconsAreOurSvgOnA24Grid` les teste (34 lignes `PASS` de plus).
- Talons réservés en plus : `core/include/actcraft/core/pagezones.h` et `core/src/pagezones.cpp` (fichiers vides
  marqués « Talon L4-T1 : rempli par L4-T27 », sans déclaration ; L4-T27 écrit l'en-tête de sa section), et
  `src/pagezones.cpp` dans `core/CMakeLists.txt` (étape 7).
- Modèle : `baseHeight` **à la fin** de `Page` (ajouts à la fin seulement).
- En-tête de `GridOverlayItem` : version de cet amendement (étape 6, ci-dessous) ; plus de `setPage`.
- Commentaire de `SetGridCommand` (en-tête `pagesettings.h`) : « sinon `PageSettingsChanged` (ignoré par le cache,
  L0-S3) ».
- Preuve : N = **136** (136 + n_B si le lot 7 a fusionné).

**Amendement 2 (N1, N2, N8, N13).**
- `objectBarAndContextMenu` (`tests/tst_ui_commands.cpp`, socle) est amendé : au lieu du compte exact, il vérifie que
  les 4 premières entrées de `contextMenu()` sous Windows, et les 2 premières sous Android, sont celles du socle ;
  commentaire « amendement 1 du lot 4 : entrées du menu de page ajoutées en fin ».
- Palette : le groupe `{"group": "formes-texte", "items": ["tool.shape", "tool.text"]}` va **en fin** de `palette`
  (recommandation de D19, amendement 3 ; `L4-C0` si D19 tranche « après `outils` »)
  (et non après `outils`) ; `defaultLayoutMatchesLotOne` (`tests/tst_actions_layout.cpp`) reçoit
  `QStringLiteral("formes-texte")` en fin de sa liste attendue de groupes, avec le même commentaire.
- `contextMenuAcceptsSubmenus` s'écrit avec `parseLayout(json, layout, error)` (`layout.h:42`) ; `lot4EntriesDeclared`
  appelle la fonction libre `unknownCommands(layout, registry)`.
- Ces trois tests du socle et du lot 1 sont amendés, pas contournés : la cause (entrées ajoutées en fin) est écrite
  dans le commentaire.

**Amendements transmis en cours d'exécution (amendement 4, résultats de L4-T0).** L4-T1 tournait déjà quand L4-T0 a
rendu ses résultats : le contrat ci-dessous n'est pas réécrit. L'orchestrateur transmet ces points à l'agent de L4-T1 ;
son rapport dit lesquels il a appliqués, et un point non appliqué passe à la tâche nommée entre parenthèses.
- **V3 et A4 — tests figés rouges dès L4-T1, non cités plus haut.** `lot3EntriesDeclared`
  (`tests/tst_actions_layout.cpp:205-206`, menu de page exact) ; `defaultCommandsFileDeclaresLotOne`
  (`tests/tst_actions_registry.cpp:197`, compte `23 + 62` des commandes, qui devient `23 + 62 + 47`) ;
  `paletteColorsAndWidths` (`tests/tst_ui_commands.cpp:96-98`, liste exacte des 7 groupes, `formes-texte` ajouté en
  fin) ; `builtinsRegistered` (`tests/tst_tools_registry.cpp:107`, liste exacte des quatre outils du lot 3 :
  `shape`, `text`, `text-vertical` ajoutés en fin, ce qui ajoute `tests/tst_tools_registry.cpp` aux fichiers de
  L4-T1). Chacun est amendé, pas contourné, avec le commentaire « amendement 1 du lot 4 : entrées ajoutées en fin ».
  `toolsMenuFollowsF750` (`tests/tst_actions_layout.cpp:246-260`) ne lit que le premier niveau du menu `tools` : il
  reste vert sans changement, et la place de `tool.text-vertical` en fin d'`other-tools` se vérifie dans
  `lot4EntriesDeclared` ; les mentions « `toolsMenuFollowsF750` amendé » de l'amendement 1 et de « Terminé quand »
  ci-dessous tombent (la table dit « inchangé »).
- **V2 — menu contextuel.** `Layout::contextMenu` n'est aujourd'hui qu'un tableau de chaînes
  (`actions/src/layout.cpp:159-165`) et `Commands::contextMenu()` passe `nullptr` comme sous-menus
  (`ui/src/commands.cpp:187-190`) : changer aussi `ui/src/commands.cpp:189`, qui reçoit `Layout::contextSubmenus`.
- **V1 — argument texte.** `ArgumentKind::Text` accepte toute valeur convertible en texte non blanc
  (`actions/src/commandregistry.cpp:120-121`) : un nombre passe. `editAtArgumentOptional` n'attend donc **pas** le
  refus d'un nombre ; il vérifie l'appel sans argument (accepté, `:286-287`) et le refus d'une chaîne blanche.
  Citations justes : `"optional"` lu en `:184` (et non `:180`), `ArgumentKind::Color` `#RRGGBB` ou `#AARRGGBB` en
  `:97-100`.
- **D2 — remplissage par défaut.** Dans l'ajout à `userstate.h` (étape 6), `core::Argb fill = 0xFFFFAA00u; // D2 :
  orange clair` au lieu de `0xFFFFFFFFu` (à défaut : L4-T11). `ShapeStyle::fill` de `shapegeometry.h` garde
  `0xFFFFFFFFu` : style neutre des tests de géométrie, l'outil prend le remplissage dans `ShapeSettings`.
- **A10 — citations.** `parseLayout` est en `actions/include/actcraft/actions/layout.h:45` (et non `:42`, amendement 2
  ci-dessus) ; `unknownCommands` en `:52` ; jeton `>id` en `actions/src/layout.cpp:71` ; filtrage par plateforme en
  `ui/src/commands.cpp:109-150`.
- **D19 — place du groupe `formes-texte`** : tranchée au défaut, en fin de palette (choix de Jérôme du 2026-10-21) ;
  ce que L4-T1 pose déjà (amendement 2) ne change pas, et `L4-C0` est abandonné.
- **V10 — lot 7** (même tâche, hors de la liste du point 1 de L4-T0). La branche du lot 7 ajoute le signal
  `documentReplaced` à `documentcontroller.h` : `showPage` et `pageContentChanged` vont dans le même bloc de signaux,
  pour une juxtaposition sans conflit à la fusion.

**Objectif.** Poser d'un coup tout ce que les tâches des deux sous-lots partagent, pour qu'aucune vague n'écrive
ensuite dans un fichier chaud en même temps qu'une autre : les 47 commandes et leur disposition (`contextMenu`
étendu compris), 34 icônes à nous, les champs du modèle (avec leur empreinte), les enregistrements CMake, les 23 tests
(talons sautés), les en-têtes publics figés et leurs talons, deux ajouts réels à `DocumentController`.

**Fichiers.** Ceux de la ligne L4-T1 de la table.

**Test et commande bornée.** Commande ciblée
`-R 'tst_actions_layout|tst_actions_registry|tst_ui_commands|tst_ui_theme|tst_core_model|tst_ui_document'`, puis preuve
de tâche (N = 136 + n_B, `TESTS OK`), puis `premiers-pas` hors écran (`-Lire "pageIndicator.text=1 / 1"`) :
`CAPTURES OK`.

**Interdits.** Aucun comportement dans un talon (valeur par défaut seulement). Aucune modification d'une
signature publique des lots 1 à 3, 10 et L0-S (ajouts seulement, à la fin). Aucune icône copiée.

**Terminé quand.** Les six tests de la commande ciblée passent avec leurs fonctions neuves (et les tests du socle
amendés : `objectBarAndContextMenu`, `defaultLayoutMatchesLotOne`, `toolsMenuFollowsF750`) ; les 23 tests du lot sont listés par
`ctest -N` (16 talons `SKIP`, 7 scénarios réduits à `waitReady` qui passent) ; N = 136 + n_B ; `TESTS OK` ;
`premiers-pas` passe ; `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge de la disposition.** Ajouter à `tests/tst_actions_layout.cpp` :
  ```cpp
  // tests/tst_actions_layout.cpp
  void contextMenuAcceptsSubmenus()
  {
      const QByteArray json = R"({"format": "actcraft-layout/1", "menus": [], "palette": [], "colors": [],
          "penWidths": [], "contextMenu": ["tool.select", "-", {"submenu": "page-insert", "label": "Insérer une page",
          "items": ["page.insert-before", "page.insert-after"]}]})";
      Layout layout; // using namespace actcraft::actions (tst_actions_layout.cpp:9)
      QString error;
      QVERIFY2(parseLayout(json, layout, error), qPrintable(error));
      QCOMPARE(layout.contextMenu, (QStringList{QStringLiteral("tool.select"), QStringLiteral("-"), QStringLiteral(">page-insert")}));
      QCOMPARE(layout.contextSubmenus.size(), std::size_t{1});
      QCOMPARE(layout.contextSubmenus.front().items, (QStringList{QStringLiteral("page.insert-before"), QStringLiteral("page.insert-after")}));
  }
  // tests/tst_actions_registry.cpp
  void lot4EntriesDeclared()
  {
      const auto registry = loadBuiltinRegistry(); // aide du fichier (lot 1)
      for (const char* id : {"tool.shape", "shape.choose", "tool.text", "tool.text-vertical", "text.bold", "view.browsers",
                             "page.insert-after", "page.paste", "grid.designer", "grid.snap", "page.extend", "page.zones"})
          QVERIFY2(registry.contains(QString::fromLatin1(id)), id);
      const auto layout = loadBuiltinLayout();
      QVERIFY(layout.contextMenu.contains(QStringLiteral(">page-insert")));
      QVERIFY(unknownCommands(layout, registry).isEmpty()); // fonction libre (layout.h)
  }
  void editAtArgumentOptional()
  {
      const auto registry = loadBuiltinRegistry();
      const auto* info = registry.find(QStringLiteral("text.edit-at")); // accès du fichier (lot 1)
      QVERIFY(info);
      QCOMPARE(info->argument, actcraft::actions::ArgumentKind::Text);
      QVERIFY(info->argumentOptional);
      QVERIFY(registry.find(QStringLiteral("shape.fill-color"))->argument == actcraft::actions::ArgumentKind::Color);
      QVERIFY(registry.find(QStringLiteral("tool.shape"))->shortcut.isEmpty()); // D18
  }
  ```
  (Noms des aides, de la fabrique et de l'accès par identifiant : ceux des fichiers. `noShortcutTwice` du socle
  n'est pas recopié.) Commande ciblée `-R 'tst_actions_layout|tst_actions_registry'` → rouge (sous-menu refusé dans
  `contextMenu`, commandes absentes).
- [ ] **Étape 2 : `contextMenu` à sous-menus** (amendement 1). `layout.h`, à la fin de `Layout` :
  `std::vector<MenuSpec> contextSubmenus;`. `layout.cpp` : `contextMenu` lu comme un menu (chaînes ou objets de
  sous-menu, erreur « contextMenu : tableau attendu ») ; `unknownCommands` parcourt aussi `contextSubmenus`. Aucune
  propriété neuve dans `ui/…/commands.h` : `contextMenu()` (socle) rend les sous-menus au format de `menus()`
  (`{submenu: true, id, label, items}`), filtrés par plateforme ; test `contextMenuWithSubmenu`. Vert.
- [ ] **Étape 3 : données.** En fin de `commands` de `actions/data/commands.json`, un bloc :
  ```json
      { "id": "tool.shape", "label": "Formes", "icon": "forme", "checkable": true },
      { "id": "shape.choose", "label": "Choisir une forme", "argument": "text" },
      { "id": "shape.filled", "label": "Forme remplie", "checkable": true },
      { "id": "shape.fill-color", "label": "Couleur de remplissage", "argument": "color" },
      { "id": "shape.draw-from-center", "label": "Tracer depuis le centre", "checkable": true },
      { "id": "tool.text", "label": "Texte", "icon": "texte", "shortcut": "Ctrl+T", "checkable": true },
      { "id": "tool.text-vertical", "label": "Texte de haut en bas", "icon": "texte-vertical", "checkable": true },
      { "id": "text.font", "label": "Police", "argument": "text" },
      { "id": "text.size", "label": "Taille", "argument": "number" },
      { "id": "text.size-up", "label": "Agrandir le texte", "icon": "taille-plus" },
      { "id": "text.size-down", "label": "Réduire le texte", "icon": "taille-moins" },
      { "id": "text.bold", "label": "Gras", "icon": "gras", "checkable": true },
      { "id": "text.italic", "label": "Italique", "icon": "italique", "checkable": true },
      { "id": "text.underline", "label": "Souligné", "icon": "souligne", "checkable": true },
      { "id": "text.superscript", "label": "Exposant", "icon": "exposant", "checkable": true },
      { "id": "text.subscript", "label": "Indice", "icon": "indice", "checkable": true },
      { "id": "text.color", "label": "Couleur du texte", "argument": "color" },
      { "id": "text.background", "label": "Fond du texte", "argument": "color" },
      { "id": "text.align-left", "label": "À gauche", "icon": "aligner-gauche", "checkable": true },
      { "id": "text.align-center", "label": "Au centre", "icon": "centrer", "checkable": true },
      { "id": "text.align-right", "label": "À droite", "icon": "aligner-droite", "checkable": true },
      { "id": "text.spacing-up", "label": "Plus d'interligne", "icon": "interligne-plus" },
      { "id": "text.spacing-down", "label": "Moins d'interligne", "icon": "interligne-moins" },
      { "id": "text.indent", "label": "Plus de retrait", "icon": "retrait-plus" },
      { "id": "text.outdent", "label": "Moins de retrait", "icon": "retrait-moins" },
      { "id": "text.bullets", "label": "Puces", "icon": "puces", "checkable": true },
      { "id": "text.select-all", "label": "Tout le texte" },
      { "id": "text.symbol", "label": "Symboles", "icon": "symboles", "argument": "text" },
      { "id": "text.edit-at", "label": "Écrire ici", "argument": "text", "optional": true },
      { "id": "view.browsers", "label": "Navigateurs", "icon": "navigateurs", "shortcut": "Ctrl+B", "checkable": true },
      { "id": "browsers.dock-left", "label": "À gauche", "checkable": true },
      { "id": "browsers.dock-right", "label": "À droite", "checkable": true },
      { "id": "page.goto", "label": "Aller à la page", "argument": "number" },
      { "id": "page.insert-before", "label": "Page vierge avant", "icon": "page-avant" },
      { "id": "page.insert-after", "label": "Page vierge après", "icon": "page-apres", "shortcut": "Ctrl+I" },
      { "id": "page.cut", "label": "Couper les pages", "icon": "page-couper" },
      { "id": "page.copy", "label": "Copier les pages", "icon": "page-copier" },
      { "id": "page.paste", "label": "Coller les pages", "icon": "page-coller" },
      { "id": "page.duplicate", "label": "Dupliquer les pages", "icon": "page-dupliquer" },
      { "id": "page.delete", "label": "Supprimer les pages", "icon": "page-supprimer" },
      { "id": "page.background", "label": "Fond de la page…", "icon": "fond" },
      { "id": "grid.designer", "label": "Grille…", "icon": "createur-grille", "shortcut": "Ctrl+Shift+G" },
      { "id": "grid.hide", "label": "Cacher la grille", "icon": "grille-masquer", "checkable": true },
      { "id": "grid.snap", "label": "Aimanter à la grille", "icon": "aimant", "checkable": true },
      { "id": "view.page-extension", "label": "Poignée d'allongement", "icon": "allonger", "checkable": true },
      { "id": "page.extend", "label": "Allonger la page", "icon": "allonger" },
      { "id": "page.zones", "label": "Zones de la page", "icon": "zones", "argument": "text" }
  ```
  `text.edit-at` n'apparaît dans aucun menu (outil Texte seulement ; argument `"x;y;v"`, facultatif). Dans
  `actions/data/layout.json` (amendement 1) : menu `insert` → **ajouter en fin** `"-", {"submenu": "insert-page",
  "label": "Page", "items": ["page.insert-before", "page.insert-after"]}` ; menu `tools` → `tool.text-vertical` en fin
  du sous-menu `other-tools`, rien d'autre ; menu `view` → ajouter `"-",
  "view.browsers", "view.page-extension"` en fin ; palette : groupe `{"group": "formes-texte", "items":
  ["tool.shape", "tool.text"]}` **en fin de palette** (amendement 2, N2) ; clé `contextMenu` du socle : ses 4 entrées gardées, puis
  `"-", {"submenu": "page-insert", "label": "Insérer une page", "items": ["page.insert-before", "page.insert-after"]}, "-", "page.background", "grid.designer", "grid.hide", "grid.snap", "-", "page.cut", "page.copy", "page.paste", "page.duplicate", "page.delete"`.
- [ ] **Étape 4 : icônes.** Dessiner les 34 fichiers de la table (24 × 24, mêmes attributs de trait que
  `ui/icons/stylo.svg`), formes simples à nous : triangle et cercle superposés (`forme`), « T » (`texte`), « T » sur
  une flèche descendante (`texte-vertical`), damier de lignes (`grille`), damier barré (`grille-masquer`), aimant en
  U (`aimant`), damier et crayon (`createur-grille`), panneau à rangée d'icônes (`navigateurs`), page et flèche à
  gauche / à droite (`page-avant`, `page-apres`), page et ciseaux / deux pages / presse-papiers / page doublée /
  page barrée (`page-couper` … `page-supprimer`), pot renversé (`fond`), page et flèche vers le bas (`allonger`),
  page coupée de traits (`zones`), « G », « I », « S » soulignés (`gras`, `italique`, `souligne`), « x² », « x₂ »
  (`exposant`, `indice`), lignes alignées (`aligner-gauche`, `centrer`, `aligner-droite`), lignes écartées ou
  serrées avec flèche (`interligne-plus`, `interligne-moins`), lignes décalées et flèche (`retrait-plus`,
  `retrait-moins`), points et lignes (`puces`), « Ω » (`symboles`), « A+ », « A− » (`taille-plus`,
  `taille-moins`). Les ajouter à la liste des ressources de `ui/CMakeLists.txt` (bloc de `icons/stylo.svg`) et en fin
  de `kIcons` (`tests/tst_ui_theme.cpp:17`).
- [ ] **Étape 5 : modèle (réel, pas un talon).** Test rouge dans `tests/tst_core_model.cpp` :
  ```cpp
  void lot4FieldsKeepOldFingerprints()
  {
      using namespace actcraft::core;
      Page page;
      PageObject shape{Shape{}};
      shape.common.id = 1;
      PageObject text{Text{}};
      text.common.id = 2;
      page.objects = {shape, text};
      const auto before = fingerprint(page);
      std::get<Shape>(body(page.objects[0])).preset = "arrow";
      QVERIFY(fingerprint(page) != before);
      std::get<Shape>(body(page.objects[0])).preset.clear();
      QCOMPARE(fingerprint(page), before);
      std::get<Shape>(body(page.objects[0])).parameters = {0.3};
      QVERIFY(fingerprint(page) != before);
      std::get<Shape>(body(page.objects[0])).parameters.clear();
      std::get<Text>(body(page.objects[1])).vertical = true;
      QVERIFY(fingerprint(page) != before);
      std::get<Text>(body(page.objects[1])).vertical = false;
      page.baseHeight = 768;
      QVERIFY(fingerprint(page) != before);
      page.baseHeight = 0;
      QCOMPARE(fingerprint(page), before);
  }
  ```
  Puis, dans `model.h`, à la fin de `Shape` : `std::string preset; // préréglage de la palette (« rectangle »…) ;
  vide : forme lue ou générique` et `std::vector<double> parameters; // points intelligents du préréglage (F-242)` ;
  à la fin de `Text` : `bool vertical = false; // texte de haut en bas (F-301) : colonne d'un caractère` ; à la
  fin de `Page` : `double baseHeight = 0; // hauteur avant la première extension (F-301) ; 0 : jamais
  allongée`. Dans `model.cpp`, `fingerprint(const Page&)` : après le flux actuel (clés inconnues comprises), une
  **queue** écrite seulement s'il y a une donnée du lot 4 : pour chaque objet, dans l'ordre, qui a un `preset` non
  vide, des `parameters` ou `vertical` vrai, l'octet `0xA4`, son rang (`std::uint64_t`), `preset` (longueur puis
  octets), le nombre de paramètres puis leurs motifs IEEE 754, `vertical` (un octet) ; puis, si
  `baseHeight != 0`, l'octet `0xA5` et son motif. Une page sans donnée du lot 4 garde son empreinte. Vert.
- [ ] **Étape 6 : en-têtes figés et talons.** Créer les en-têtes ci-dessous **tels quels** ; chaque `.cpp` talon
  inclut son en-tête et définit chaque fonction par le corps le plus simple qui compile, précédé de
  `// Talon L4-T1 : rempli par L4-T<n>.` ; une commande talon ne change rien (`redo`/`undo` vides).

  `core/include/actcraft/core/shapegeometry.h` (L4-T3) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>

  #include <cstddef>
  #include <string>
  #include <string_view>
  #include <utility>
  #include <vector>

  namespace actcraft::core {

  inline constexpr double kShapeMinDragScreen = 3.0; // en deçà, un tracé est un clic : aucune forme (F-241)

  enum class ShapeFamily : std::uint8_t
  {
      Line,   // objet core::Line (ligne, ligne brisée, zigzag, chevron)
      Closed, // objet core::Shape (polygone ou chemin rempli)
  };

  struct ShapePreset
  {
      std::string id;                  // « rectangle »… (contrat du plan)
      ShapeFamily family = ShapeFamily::Closed;
      bool square = false;             // carré, cercle : boîte carrée
      int axis = 0;                    // 1 : horizontale seulement ; 2 : verticale seulement ; 0 : libre
      std::vector<double> defaults;    // paramètres par défaut des points intelligents
      std::vector<double> minimum;     // bornes, même taille que defaults
      std::vector<double> maximum;
  };

  struct ShapeStyle
  {
      Argb outline = 0xFF000000u;
      double width = 4;      // F-241
      bool filled = true;    // E1, D2
      Argb fill = 0xFFFFFFFFu;
  };

  // Ordre de la palette (D1) ; le premier est le défaut.
  const std::vector<ShapePreset>& shapeCatalog();
  const ShapePreset* findShapePreset(std::string_view id) noexcept;

  // Boîte tracée de `press` à `current` (coordonnées de page) : depuis le coin, ou depuis le centre (`press` au
  // milieu) ; `square` : côté = le plus grand des deux écarts, dans le sens du geste. Toujours normalisée.
  RectF dragBox(PointF press, PointF current, bool fromCenter, bool square) noexcept;

  // Extrémités d'une ligne tracée : axe fixé par le préréglage, `constrain45` (Maj) arrondit l'angle à 45°.
  std::pair<PointF, PointF> dragLine(PointF press, PointF current, const ShapePreset& preset, bool fromCenter,
                                     bool constrain45) noexcept;

  // Objet neuf (sans identifiant, couche intermédiaire, nom vide) : Shape (preset, parameters = defaults, géométrie
  // construite) ou Line, selon la famille. Préréglage inconnu : rectangle.
  PageObject makeShapeObject(std::string_view preset, const RectF& box, const ShapeStyle& style);
  PageObject makeLineObject(std::string_view preset, PointF from, PointF to, const ShapeStyle& style);

  // Reconstruit `polygon` ou `paths` depuis preset, parameters et bounds. Faux (forme inchangée) si le préréglage est
  // inconnu ou vide (forme lue).
  bool rebuildShape(Shape& shape);

  // Points intelligents (F-242) en coordonnées de l'objet ; vide pour une forme sans préréglage ou sans paramètre.
  std::vector<PointF> smartPoints(const Shape& shape);

  // Paramètres quand le point `index` est amené en `objectPoint` (coordonnées de l'objet), bornés par le préréglage.
  std::vector<double> dragSmartPoint(const Shape& shape, std::size_t index, PointF objectPoint);

  } // namespace actcraft::core
  ```
  `core/include/actcraft/core/shapeedits.h` (L4-T3) :
  ```cpp
  #pragma once

  #include <actcraft/core/document.h>
  #include <actcraft/core/undo.h>

  #include <cstddef>
  #include <vector>

  namespace actcraft::core {

  // Point intelligent déplacé (F-242) : paramètres remplacés et géométrie reconstruite. Changes : un ObjectChanged,
  // zone = emprise avant ∪ emprise après. Objet absent ou sans préréglage : rien.
  class SetShapeParametersCommand final : public UndoCommand
  {
  public:
      SetShapeParametersCommand(std::size_t page, ObjectId id, std::vector<double> parameters);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_page;
      ObjectId m_id;
      std::vector<double> m_after;
      std::vector<double> m_before;
  };

  } // namespace actcraft::core
  ```
  `core/include/actcraft/core/pagesettings.h` (L4-T4) :
  ```cpp
  #pragma once

  #include <actcraft/core/document.h>
  #include <actcraft/core/undo.h>

  #include <cstddef>
  #include <cstdint>

  namespace actcraft::core {

  inline constexpr double kExtendRatio = 0.10; // F-301 : 10 % par clic (D10)
  inline constexpr int kMergeGrid = 4;          // famille de fusion de SetGridCommand (une séance du créateur)

  // Vrai si la grille dessine quelque chose sous les objets : visible, non dessinée au-dessus, un niveau visible.
  bool gridDrawsUnderObjects(const Grid& grid) noexcept;
  // Vrai si la grille dessine quelque chose au-dessus des objets (surcouche, L4-T7).
  bool gridDrawsAboveObjects(const Grid& grid) noexcept;

  // Grille de la page remplacée (F-620 à F-623). Changes : PageChanged (toute la page) si la grille du dessous
  // change (avant ou après) ; sinon PageSettingsChanged (ignoré par le cache, L0-S3). `merge` vrai :
  // fusion avec la précédente SetGridCommand de la même page (kMergeGrid).
  class SetGridCommand final : public UndoCommand
  {
  public:
      SetGridCommand(std::size_t page, Grid grid, bool merge = false);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_page;
      Grid m_after;
      Grid m_before;
      bool m_merge;
  };

  // Fond de la page remplacé (F-610). Changes : PageChanged (toute la page).
  class SetBackgroundCommand final : public UndoCommand
  {
  public:
      SetBackgroundCommand(std::size_t page, Background background);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_page;
      Background m_after;
      Background m_before;
  };

  // Hauteur après un clic d'extension : height + kExtendRatio × base, base = baseHeight, ou height si 0 (D10).
  double nextExtendedHeight(const Page& page) noexcept;

  // Un clic d'extension (F-301 a) : baseHeight posé au premier clic, height = nextExtendedHeight. Une étape par
  // clic. Changes : PageChanged (toute la page).
  class ExtendPageCommand final : public UndoCommand
  {
  public:
      explicit ExtendPageCommand(std::size_t page);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_page;
      double m_beforeHeight = 0;
      double m_beforeBase = 0;
  };

  } // namespace actcraft::core
  ```
  `core/include/actcraft/core/gridsnap.h` (L4-T5) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>

  #include <optional>
  #include <vector>

  namespace actcraft::core {

  // Intersection la plus proche de `point` parmi les niveaux qui accrochent ; std::nullopt si la grille n'autorise
  // pas l'accrochage (snapEnabled faux) ou si aucun niveau n'accroche. Grille invisible : accroche quand même.
  std::optional<PointF> nearestGridPoint(const Grid& grid, PointF point, double pageWidth,
                                         double pageHeight) noexcept;

  // Point d'ancrage d'un objet pour l'accrochage (F-622), en coordonnées de page : ligne de base d'un texte
  // (core::textBaseline), sinon coin inférieur gauche de son emprise.
  PointF snapAnchor(const PageObject& object);

  // Déplacement corrigé pour que l'ancrage du premier objet de `ids` (ordre de la page) tombe, après `delta`, sur
  // l'intersection la plus proche ; `delta` inchangé sans accrochage ou sans objet.
  PointF snapDelta(const Page& page, const std::vector<ObjectId>& ids, PointF delta) noexcept;

  } // namespace actcraft::core
  ```
  `core/include/actcraft/core/pageedits.h` (L4-T15) :
  ```cpp
  #pragma once

  #include <actcraft/core/document.h>
  #include <actcraft/core/undo.h>

  #include <cstddef>
  #include <vector>

  namespace actcraft::core {

  // Copie de `page` dont chaque objet reçoit un identifiant neuf du document ; membres de groupes, extrémités de
  // connecteurs et cibles d'actions qui visent un objet de la page suivent leur nouvel identifiant.
  Page withFreshIds(Document& document, Page page);

  // Indices des pages déplacées après MovePagesCommand(pages, before) (F-802 : elles restent sélectionnées).
  std::vector<std::size_t> positionsAfterMove(std::size_t pageCount, const std::vector<std::size_t>& pages,
                                              std::size_t before);

  // Réordonner (F-802) : `pages` (triés, uniques) retirées puis insérées dans leur ordre devant la page qui était à
  // l'indice `before` (pageCount : à la fin). Changes : PageRemoved puis PageInserted par page. Sans effet si rien
  // ne bouge.
  class MovePagesCommand final : public UndoCommand
  {
  public:
      MovePagesCommand(std::vector<std::size_t> pages, std::size_t before);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::vector<std::size_t> m_pages;
      std::size_t m_before;
  };

  // Supprimer, couper (F-601) : refusée (aucun effet) si elle retirerait toutes les pages. Changes : PageRemoved.
  class RemovePagesCommand final : public UndoCommand
  {
  public:
      explicit RemovePagesCommand(std::vector<std::size_t> pages);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::vector<std::size_t> m_pages;
      std::vector<Page> m_removed;
  };

  // Coller, dupliquer, insérer (F-600, F-601) : copies à identifiants neufs (withFreshIds au premier redo, gardés
  // ensuite) insérées à `index`. Changes : PageInserted.
  class InsertPagesCommand final : public UndoCommand
  {
  public:
      InsertPagesCommand(std::size_t index, std::vector<Page> pages);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_index;
      std::vector<Page> m_pages;
      bool m_fresh = false;
  };

  } // namespace actcraft::core
  ```
  `core/include/actcraft/core/textedits.h` (L4-T16) :
  ```cpp
  #pragma once

  #include <actcraft/core/document.h>
  #include <actcraft/core/undo.h>

  #include <cstddef>
  #include <optional>

  namespace actcraft::core {

  // Texte remplacé (F-251) : HTML, bornes, lignes lues et sens. Changes : un ObjectChanged, zone = emprise avant ∪
  // emprise après. Objet absent ou qui n'est pas un texte : rien.
  class SetTextCommand final : public UndoCommand
  {
  public:
      SetTextCommand(std::size_t page, ObjectId id, Text text);
      void redo(Document& document, Changes& changes) override;
      void undo(Document& document, Changes& changes) override;

  private:
      std::size_t m_page;
      ObjectId m_id;
      Text m_after;
      Text m_before;
  };

  // Ligne de base de la première ligne, coordonnées de l'objet : celle de la première ligne lue (runs) ; sinon
  // std::nullopt (l'appelant prend le coin inférieur gauche de la première ligne qu'il a mesurée, ou de l'emprise).
  std::optional<PointF> textBaseline(const Text& text) noexcept;

  } // namespace actcraft::core
  ```
  `tools/include/actcraft/tools/shapetool.h` (L4-T8) et `texttool.h` (L4-T22) :
  ```cpp
  #pragma once

  #include <actcraft/tools/tool.h>
  #include <actcraft/tools/toolcontext.h>

  #include <optional>

  namespace actcraft::tools {

  // Outil des formes (F-240, F-241) : aperçu par la couche soulevée, une AddObjectCommand au relâché.
  class ShapeTool final : public Tool
  {
  public:
      explicit ShapeTool(ToolContext& context);
      QString id() const override;
      void onSample(const input::Sample& sample) override;
      bool isBusy() const noexcept override;
      void cancel() override;
      // Maj relayée par ToolHost (M9) : (Qt::Key_Shift, Qt::ShiftModifier) à l'appui, (Qt::Key_Shift,
      // Qt::NoModifier) au relâché ; l'aperçu est refait aussitôt. Échap annule.
      bool onKey(int key, int modifiers) override;

  private:
      ToolContext& m_context;
      std::optional<core::PointF> m_press;
      core::PointF m_current;
      bool m_shift = false;
  };

  } // namespace actcraft::tools
  ```
  ```cpp
  #pragma once

  #include <actcraft/tools/tool.h>
  #include <actcraft/tools/toolcontext.h>

  namespace actcraft::tools {

  // Outils Texte et Texte de haut en bas (F-250, F-301 b) : un appui demande l'édition au point touché par la
  // commande « text.edit-at » ({x, y, vertical}) ; l'édition elle-même est dans ui::TextEditing.
  class TextTool final : public Tool
  {
  public:
      TextTool(ToolContext& context, bool vertical);
      QString id() const override; // « text » ou « text-vertical »
      void onSample(const input::Sample& sample) override;
      bool isBusy() const noexcept override;
      void cancel() override;
      void deactivate() override; // valide l'édition en cours (F-252) : commande « text.edit-at » sans argument

  private:
      ToolContext& m_context;
      bool m_vertical;
  };

  } // namespace actcraft::tools
  ```
  Ajouts à `tools/include/actcraft/tools/userstate.h` (corps réels dans `userstate.cpp`, simples accesseurs) :
  ```cpp
  struct ShapeSettings
  {
      QString preset = QStringLiteral("line"); // D1 : défaut de la palette
      bool filled = true;                       // E1
      core::Argb fill = 0xFFFFFFFFu;
      bool fromCenter = false;                  // F-241 : depuis le coin par défaut
  };

  struct TextSettings
  {
      QString fontFamily;   // vide : police par défaut de l'application (N2-3, D7)
      double pointSize = 14;
      core::Argb color = 0xFF000000u;
  };
  // dans UserState, public, à la suite :
  ShapeSettings shapeSettings() const;
  void setShapeSettings(const ShapeSettings& settings);
  TextSettings textSettings() const;
  void setTextSettings(const TextSettings& settings);
  // privé : ShapeSettings m_shape; TextSettings m_text;
  ```
  Ajout à `tools/include/actcraft/tools/toolcontext.h`, à la fin de `SelectionView` :
  `std::vector<core::PointF> smartPoints; // F-242 : points intelligents de la forme sélectionnée seule (page)`.
  Dans `tools/src/toolregistry.cpp`, `registerBuiltinTools` ajoute, après les quatre du lot 3, « shape »
  (`ShapeTool`), « text » (`TextTool(context, false)`), « text-vertical » (`TextTool(context, true)`).

  `render/include/actcraft/render/gridoverlayitem.h` (L4-T7) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>

  #include <QQuickItem>
  #include <QtQml/qqmlregistration.h>

  namespace actcraft::render {

  // Grille dessinée au-dessus des objets (« toujours visible », F-620), hors de la scène que le cache photographie.
  // Coordonnées de page ; lignes nettes de 1 px d'écran (lot 10, P2). Ne dessine rien si la grille de la page n'est
  // pas au-dessus.
  class GridOverlayItem : public QQuickItem
  {
      Q_OBJECT
      QML_NAMED_ELEMENT(GridOverlay)
      Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
      Q_PROPERTY(int rebuildCount READ rebuildCount NOTIFY rebuilt) // tests
      // Amendements 1 et 2 : la page est lue par la page fixe (FixedPageItem::page()), jamais par un pointeur gardé.
      Q_PROPERTY(actcraft::render::FixedPageItem* fixedPage READ fixedPage WRITE setFixedPage NOTIFY fixedPageChanged)

  public:
      explicit GridOverlayItem(QQuickItem* parent = nullptr);
      FixedPageItem* fixedPage() const noexcept;
      void setFixedPage(FixedPageItem* page); // fil principal
      // La grille de la page affichée a pu changer (PageSettingsChanged n'atteint pas la page fixe) : relue au
      // prochain rendu. Appelée par PageArea.qml sur area.documentController.pageContentChanged de la page
      // affichée.
      Q_INVOKABLE void refresh();
      qreal zoom() const noexcept;
      void setZoom(qreal zoom);
      int rebuildCount() const noexcept;

  signals:
      void zoomChanged();
      void rebuilt();
      void fixedPageChanged();

  protected:
      QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData* data) override;

  private:
      QPointer<FixedPageItem> m_fixedPage; // <QPointer>, <actcraft/render/fixedpageitem.h>
      core::Grid m_grid;
      double m_width = 0;
      double m_height = 0;
      qreal m_zoom = 1;
      bool m_dirty = true;
      int m_rebuilds = 0;
  };

  } // namespace actcraft::render
  ```
  `ui/include/actcraft/ui/pageoptions.h` (L4-T13, L4-T20, L4-T26, L4-T28) :
  ```cpp
  #pragma once

  #include <actcraft/ui/documentcontroller.h>

  #include <QObject>
  #include <QPointer>
  #include <QString>
  #include <QVariantMap>
  #include <QtQml/qqmlregistration.h>

  namespace actcraft::ui {

  // Réglages de la page affichée : grille (F-620 à F-623), fond (F-610), extension (F-301 a), zones (F-603).
  class PageOptions : public QObject
  {
      Q_OBJECT
      QML_NAMED_ELEMENT(PageSettings)
      Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                     setDocumentController NOTIFY wiringChanged)
      Q_PROPERTY(bool gridVisible READ gridVisible NOTIFY changed)
      Q_PROPERTY(bool gridSnap READ gridSnap NOTIFY changed)
      Q_PROPERTY(bool gridAbove READ gridAbove NOTIFY changed)
      // {visible, snap, above, previewZoom, baseZoom, zoomIncrement, levels: [{visible, snap, startX, startY,
      //  spacingX, spacingY, color, pattern, angle, line, alongX, alongY}]} ; noms de nos clés
      Q_PROPERTY(QVariantMap grid READ grid NOTIFY changed)
      Q_PROPERTY(QVariantMap background READ background NOTIFY changed) // {kind, color, color2, gradient, image}
      Q_PROPERTY(qreal pageHeight READ pageHeight NOTIFY changed)
      Q_PROPERTY(QString zones READ zones NOTIFY changed) // « <horizontales>x<verticales> », « 0x0 » par défaut

  public:
      explicit PageOptions(QObject* parent = nullptr);
      DocumentController* documentController() const noexcept;
      void setDocumentController(DocumentController* controller);
      bool gridVisible() const;
      bool gridSnap() const;
      bool gridAbove() const;
      QVariantMap grid() const;
      QVariantMap background() const;
      qreal pageHeight() const;
      QString zones() const;

      Q_INVOKABLE void toggleGridHidden();                 // F-623
      Q_INVOKABLE void toggleSnap();                       // F-622
      Q_INVOKABLE void beginDesigner();                    // ouvre une séance : les réglages suivants fusionnent
      Q_INVOKABLE void setGrid(const QVariantMap& grid);   // aperçu immédiat (F-620)
      Q_INVOKABLE void addLevel();
      Q_INVOKABLE void removeLevel(int level);
      Q_INVOKABLE void resetGrid();                        // un niveau sans accrochage
      Q_INVOKABLE void stepSpacing(int sign);              // « + » et « - » : × (1 ± zoomIncrement)
      Q_INVOKABLE void endDesigner();
      Q_INVOKABLE void setBackground(const QVariantMap& background); // F-610 (L4-T20)
      Q_INVOKABLE void extendPage();                                  // F-301 a (L4-T26)
      Q_INVOKABLE void setZones(const QString& zones);                // F-603 (L4-T28)

  signals:
      void wiringChanged();
      void changed();

  private:
      QPointer<DocumentController> m_controller;
      bool m_designer = false;
  };

  } // namespace actcraft::ui
  ```
  `ui/include/actcraft/ui/pagethumbnails.h` (L4-T17) :
  ```cpp
  #pragma once

  #include <actcraft/ui/documentcontroller.h>
  #include <actcraft/ui/pageview.h>

  #include <QAbstractListModel>
  #include <QImage>
  #include <QPointer>
  #include <QQuickPaintedItem>
  #include <QtQml/qqmlregistration.h>

  #include <vector>

  namespace actcraft::ui {

  // Miniatures des pages (F-800), à jour après chaque modification sans figer l'écriture : copie de la page sur le
  // fil principal, rendu par le peintre partagé (DA5) sur un fil secondaire, kSettleMs après la dernière
  // modification de la page, jamais pendant un contact sur la vue.
  class PageThumbnails : public QAbstractListModel
  {
      Q_OBJECT
      QML_NAMED_ELEMENT(PageThumbnails)
      Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                     setDocumentController NOTIFY wiringChanged)
      Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY wiringChanged)
      Q_PROPERTY(int thumbnailWidth READ thumbnailWidth WRITE setThumbnailWidth NOTIFY thumbnailWidthChanged)
      Q_PROPERTY(int pendingCount READ pendingCount NOTIFY renderingChanged)
      Q_PROPERTY(int renderCount READ renderCount NOTIFY renderingChanged)
      Q_PROPERTY(double lastCopyMs READ lastCopyMs NOTIFY renderingChanged) // fil principal, dernier rendu
      // Amendement 3 : rendus lancés (compté sur le fil principal au lancement) ; renderCount compte les terminés.
      Q_PROPERTY(int launchCount READ launchCount NOTIFY renderingChanged)

  public:
      static constexpr int kSettleMs = 400;
      static constexpr int kDefaultWidth = 160;
      enum Role
      {
          PageNumberRole = Qt::UserRole + 1, // 1 à N
          RevisionRole,                      // change à chaque nouvelle image
          AspectRole,                        // hauteur / largeur de la page
          CurrentRole,                       // page affichée
      };

      explicit PageThumbnails(QObject* parent = nullptr);
      ~PageThumbnails() override;
      int rowCount(const QModelIndex& parent = QModelIndex()) const override;
      QVariant data(const QModelIndex& index, int role) const override;
      QHash<int, QByteArray> roleNames() const override;

      DocumentController* documentController() const noexcept;
      void setDocumentController(DocumentController* controller);
      PageView* view() const noexcept;
      void setView(PageView* view);
      int thumbnailWidth() const noexcept;
      void setThumbnailWidth(int width); // 80 à 320
      int pendingCount() const noexcept;
      int renderCount() const noexcept;
      double lastCopyMs() const noexcept;
      int launchCount() const noexcept; // amendement 3
      QImage image(int page) const; // dernière image rendue ; nulle avant le premier rendu
      Q_INVOKABLE void refreshAll();

  signals:
      void wiringChanged();
      void thumbnailWidthChanged();
      void renderingChanged();

  private:
      QPointer<DocumentController> m_controller;
      QPointer<PageView> m_view;
      std::vector<QImage> m_images;
      std::vector<int> m_revisions;
      int m_width = kDefaultWidth;
      int m_pending = 0;
      int m_renders = 0;
      double m_lastCopyMs = 0;
  };

  // Image d'une miniature dans le QML (type ThumbnailImage) : peint PageThumbnails::image(page).
  class ThumbnailItem : public QQuickPaintedItem
  {
      Q_OBJECT
      QML_NAMED_ELEMENT(ThumbnailImage)
      Q_PROPERTY(actcraft::ui::PageThumbnails* source READ source WRITE setSource NOTIFY sourceChanged)
      Q_PROPERTY(int page READ page WRITE setPage NOTIFY pageChanged)          // indice 0 à N − 1
      Q_PROPERTY(int revision READ revision WRITE setRevision NOTIFY revisionChanged)

  public:
      explicit ThumbnailItem(QQuickItem* parent = nullptr);
      PageThumbnails* source() const noexcept;
      void setSource(PageThumbnails* source);
      int page() const noexcept;
      void setPage(int page);
      int revision() const noexcept;
      void setRevision(int revision);
      void paint(QPainter* painter) override;

  signals:
      void sourceChanged();
      void pageChanged();
      void revisionChanged();

  private:
      QPointer<PageThumbnails> m_source;
      int m_page = -1;
      int m_revision = 0;
  };

  } // namespace actcraft::ui
  ```
  `ui/include/actcraft/ui/pagescontroller.h` (L4-T18) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>
  #include <actcraft/ui/documentcontroller.h>

  #include <QList>
  #include <QObject>
  #include <QPointer>
  #include <QtQml/qqmlregistration.h>

  #include <vector>

  namespace actcraft::ui {

  // Pages du document vues par le navigateur de pages et le menu de page (F-600, F-601, F-771, F-801 à F-803).
  class PagesController : public QObject
  {
      Q_OBJECT
      QML_NAMED_ELEMENT(Pages)
      Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                     setDocumentController NOTIFY wiringChanged)
      Q_PROPERTY(QList<int> selection READ selection NOTIFY selectionChanged) // indices triés, jamais vide
      Q_PROPERTY(bool canPaste READ canPaste NOTIFY clipboardChanged)

  public:
      explicit PagesController(QObject* parent = nullptr);
      DocumentController* documentController() const noexcept;
      void setDocumentController(DocumentController* controller);
      QList<int> selection() const;
      bool canPaste() const noexcept;

      // F-801 : sans modificateur, sélection = {index} et la page s'affiche ; Maj : plage depuis l'ancre ; Ctrl :
      // bascule (la dernière page sélectionnée ne se retire pas). `modifiers` : Qt::KeyboardModifiers.
      Q_INVOKABLE void click(int index, int modifiers);
      Q_INVOKABLE void goTo(int index);                 // F-800 : sans effet de changement de page
      Q_INVOKABLE void moveSelection(int before);       // F-802 : pages gardées sélectionnées
      Q_INVOKABLE void insertBlank(bool after);         // F-600, par rapport à la page affichée
      Q_INVOKABLE void copySelection();                 // F-601 : presse-papiers interne
      Q_INVOKABLE void cutSelection();
      Q_INVOKABLE void paste();                         // F-601, F-803 : avant la page active
      Q_INVOKABLE void duplicateSelection();            // F-600 : après la dernière page sélectionnée
      Q_INVOKABLE void deleteSelection();

  signals:
      void wiringChanged();
      void selectionChanged();
      void clipboardChanged();

  private:
      QPointer<DocumentController> m_controller;
      QList<int> m_selection;
      int m_anchor = 0;
      std::vector<core::Page> m_clipboard;
  };

  } // namespace actcraft::ui
  ```
  `ui/include/actcraft/ui/textediting.h` (L4-T21, L4-T25) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>
  #include <actcraft/ui/documentcontroller.h>
  #include <actcraft/ui/pageview.h>

  #include <QObject>
  #include <QPointer>
  #include <QQuickTextDocument>
  #include <QRectF>
  #include <QString>
  #include <QVariant>
  #include <QtQml/qqmlregistration.h>

  namespace actcraft::ui {

  // Édition d'un texte (F-250 à F-253, F-100, F-301 b) dans le TextEdit de TextEditor.qml : un texte existant est
  // soulevé pendant l'édition (DA11) ; validation = une étape (AddObjectCommand ou SetTextCommand) ; texte vide :
  // rien n'est gardé.
  class TextEditing : public QObject
  {
      Q_OBJECT
      QML_NAMED_ELEMENT(TextEditing)
      Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                     setDocumentController NOTIFY wiringChanged)
      Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY wiringChanged)
      Q_PROPERTY(QQuickTextDocument* textDocument READ textDocument WRITE setTextDocument NOTIFY wiringChanged)
      Q_PROPERTY(int selectionStart READ selectionStart WRITE setSelectionStart NOTIFY formatChanged)
      Q_PROPERTY(int selectionEnd READ selectionEnd WRITE setSelectionEnd NOTIFY formatChanged)
      Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
      Q_PROPERTY(bool vertical READ isVertical NOTIFY activeChanged)
      Q_PROPERTY(QRectF frame READ frame NOTIFY frameChanged) // coordonnées de page
      Q_PROPERTY(QString plainText READ plainText NOTIFY formatChanged)
      Q_PROPERTY(QString fontFamily READ fontFamily NOTIFY formatChanged)
      Q_PROPERTY(qreal fontSize READ fontSize NOTIFY formatChanged)
      Q_PROPERTY(bool bold READ bold NOTIFY formatChanged)
      Q_PROPERTY(bool italic READ italic NOTIFY formatChanged)
      Q_PROPERTY(bool underline READ underline NOTIFY formatChanged)

  public:
      explicit TextEditing(QObject* parent = nullptr);
      DocumentController* documentController() const noexcept;
      void setDocumentController(DocumentController* controller);
      PageView* view() const noexcept;
      void setView(PageView* view);
      QQuickTextDocument* textDocument() const noexcept;
      void setTextDocument(QQuickTextDocument* document);
      int selectionStart() const noexcept;
      void setSelectionStart(int position);
      int selectionEnd() const noexcept;
      void setSelectionEnd(int position);
      bool isActive() const noexcept;
      bool isVertical() const noexcept;
      QRectF frame() const noexcept;
      QString plainText() const;
      QString fontFamily() const;
      qreal fontSize() const;
      bool bold() const;
      bool italic() const;
      bool underline() const;

      // Texte touché au point (page) : édition ; sinon texte neuf « Votre texte » tout sélectionné, coin du cadre au
      // point. Une édition en cours est d'abord validée. Vrai si une édition commence.
      Q_INVOKABLE bool beginAt(QPointF pagePoint, bool vertical);
      Q_INVOKABLE void commit(); // F-252
      Q_INVOKABLE void cancel();
      // Commandes « text.* » de la barre (F-100) sur la sélection ; argument selon la commande.
      Q_INVOKABLE void apply(const QString& command, const QVariant& argument);
      Q_INVOKABLE void setFrameWidth(qreal width); // poignée d'extension (page)
      Q_INVOKABLE void moveFrame(QPointF delta);   // poignée de déplacement (page)

  signals:
      void wiringChanged();
      void activeChanged();
      void frameChanged();
      void formatChanged();
      void focusRequested();

  private:
      QPointer<DocumentController> m_controller;
      QPointer<PageView> m_view;
      QPointer<QQuickTextDocument> m_document;
      core::ObjectId m_editing = 0; // 0 : texte neuf
      bool m_active = false;
      bool m_vertical = false;
      QRectF m_frame;
      int m_selectionStart = 0;
      int m_selectionEnd = 0;
  };

  } // namespace actcraft::ui
  ```
  Ajouts à `ui/include/actcraft/ui/toolhost.h` (corps talons dans `toolhost.cpp`, `// Talon L4-T1 : rempli par
  L4-T11.`) :
  ```cpp
  Q_PROPERTY(QString shapePreset READ shapePreset NOTIFY settingsChanged)
  Q_PROPERTY(bool shapeFilled READ shapeFilled NOTIFY settingsChanged)
  Q_PROPERTY(bool shapeFromCenter READ shapeFromCenter NOTIFY settingsChanged)
  Q_PROPERTY(QStringList shapePresets READ shapePresets CONSTANT) // ordre de la palette
  // public
  QString shapePreset() const;
  bool shapeFilled() const;
  bool shapeFromCenter() const;
  QStringList shapePresets() const;
  Q_INVOKABLE void chooseShape(const QString& preset); // active aussi l'outil « shape »
  Q_INVOKABLE void setShapeFilled(bool filled);
  Q_INVOKABLE void setShapeFillColor(const QColor& color);
  Q_INVOKABLE void setShapeFromCenter(bool fromCenter);
  // Contour normalisé du préréglage dans un carré 1 × 1, pour la palette et le curseur : [{x, y}, …] fermé.
  Q_INVOKABLE QVariantList shapeOutline(const QString& preset) const;
  ```
  Ajouts réels à `ui/include/actcraft/ui/documentcontroller.h` et `.cpp` (pas des talons) :
  ```cpp
  Q_INVOKABLE void showPage(int index);          // borne, puis show(index) : comme nextPage / pageBack
  // signaux
  void pageContentChanged(int page);             // une fois par page touchée par une commande, après settle()
  ```
  `showPage` termine d'abord le trait en cours et les gestes comme `nextPage` (même chemin). `pageContentChanged`
  est émis dans `settle` pour chaque indice de page distinct des `Change` (objet ou page ; page retirée : rien).
  Test rouge puis vert dans `tests/tst_ui_document.cpp` :
  ```cpp
  void pageContentChangedNamesThePage()
  {
      actcraft::ui::DocumentController controller;
      controller.newDocument(1024, 768);
      controller.nextPage(); // deux pages, la 2e affichée
      QSignalSpy spy(&controller, &actcraft::ui::DocumentController::pageContentChanged);
      actcraft::core::Stroke stroke;
      stroke.points = {{10, 10, 0.5f}, {50, 50, 0.5f}};
      controller.commitStroke(stroke);
      QCOMPARE(spy.count(), 1);
      QCOMPARE(spy.at(0).at(0).toInt(), 1);
  }
  void showPageShowsIt()
  {
      actcraft::ui::DocumentController controller;
      controller.newDocument(1024, 768);
      controller.nextPage();
      controller.nextPage();
      controller.showPage(0);
      QCOMPARE(controller.currentIndex(), 0);
      controller.showPage(9);
      QCOMPARE(controller.currentIndex(), 2);
  }
  ```
  Talons QML (créés par L4-T2) : rien ici.
- [ ] **Étape 7 : CMake.** Ajouter les sources : `core/CMakeLists.txt` (`src/pagezones.cpp`, `src/shapegeometry.cpp`,
  `src/shapeedits.cpp`, `src/gridsnap.cpp`, `src/pagesettings.cpp`, `src/pageedits.cpp`, `src/textedits.cpp`),
  `tools/CMakeLists.txt` (`src/shapetool.cpp`, `src/texttool.cpp`), `render/CMakeLists.txt`
  (`src/gridoverlayitem.cpp`, en-tête dans la liste QML du module comme `pendinglayeritem.h`), `ui/CMakeLists.txt`
  (`src/pageoptions.cpp`, `src/pagethumbnails.cpp`, `src/pagescontroller.cpp`, `src/textediting.cpp` et
  leurs en-têtes dans la liste QML). Enregistrer par `actcraft_add_test` : `tests/core.cmake` (`tst_core_shapes`,
  `tst_core_gridsnap`, `tst_core_pagesettings`, `tst_core_pageedits`, `tst_core_textedits`, `tst_core_zones`),
  `tests/formats.cmake` (`tst_formats_lot4`), `tests/render.cmake` (`tst_render_gridoverlay`,
  `tst_render_verticaltext`), `tests/tools.cmake` (`tst_tools_shape`, `tst_tools_text`), `tests/ui.cmake`
  (`tst_ui_shapes`, `tst_ui_pagesettings`, `tst_ui_pagethumbnails`, `tst_ui_pages`, `tst_ui_text`),
  `tests/bench.cmake` (`tst_captures_formes`, `tst_captures_grille`, `tst_captures_texte`,
  `tst_captures_navigateur_pages`, `tst_captures_page_allongee_zones`, `tst_captures_texte_vertical`,
  `tst_captures_formes_texte_pages_grille`, sur le modèle de `tst_captures_cache_differe`).
- [ ] **Étape 8 : tests et scénarios talons.** Chacun des 16 fichiers de test :
  ```cpp
  #include <QtTest>

  class TstCoreShapes : public QObject
  {
      Q_OBJECT

  private slots:
      void reserved() { QSKIP("Talon L4-T1 : rempli par L4-T3."); }
  };

  QTEST_GUILESS_MAIN(TstCoreShapes)
  #include "tst_core_shapes.moc"
  ```
  (`QTEST_MAIN` pour les tests de `render/` et `ui/` ; nom de classe, de `.moc` et tâche adaptés.) Chacun des 7
  scénarios : `{"format": "actcraft-scenario/1", "name": "<nom>", "description": "Talon L4-T1 : rempli par
  L4-T<n>.", "steps": [{"waitReady": true}]}`.
- [ ] **Étape 9 : preuves.** `format.ps1`, commande ciblée, preuve de tâche (N = 136 + n_B), fonctions des six tests
  nommés en `PASS`, `premiers-pas` hors écran `CAPTURES OK`, `git add` des fichiers, `NOMS OK`.
- [ ] **Étape 10 : commit** (chemins explicites, la liste de la table), puis `NOMS OK` relancé.
  ```powershell
  git -C <wt> commit -m "Lot 4 : réservation des commandes, de la disposition, des icônes, du modèle, des tests et des API (L4-T1)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t2"></a>
### Tâche L4-T2 — Réservation II : coquille QML

**Écart constaté (amendement 5).** Mergée avec `gridOverlay` **sous** `ink` (`fixedPage, pageCache, pendingLayer,
liftLayer, gridOverlay, ink, pageOverlays`, `ui/qml/PageArea.qml:102-119`, `tests/tst_ui_shell.cpp:784-788`), d'après le
verdict de L4-O1 sur D17. L4-O2 l'a contredit (la grille du dessus couvre tout ce qui est posé) : le correctif **L4-C3**
remet le rang de D17, entre `ink` et `pageOverlays`. Rien d'autre à reprendre dans cette tâche.

**Amendement 4 (D17, A12).** D17 tranche : `gridOverlay` entre `ink` et `pageOverlays` (la grille du dessus couvre
le trait en cours ; rien ne change au relâché). La liste attendue de `socleLayerOrder` (`tests/tst_ui_shell.cpp:773-786`)
est `fixedPage, pageCache, pendingLayer, liftLayer, ink, gridOverlay, pageOverlays` ; la boucle `z() == 0`, déjà exigée
par le test du socle, couvre `gridOverlay` : sa déclaration QML ne pose aucun `z:`. L'autre rang (`liftLayer,
gridOverlay, ink`) tombe, et le commentaire du test le dit. Citations (A12) : couches de `ui/qml/PageArea.qml:49-117`,
`fixedPageItem` en `:56`, alias `fixedPage` en `:11`, `area.documentController` en `:25`, `view` en `:29`. L'encre en
cours sous la grille du dessus est observée à l'écran par L4-O1 : si l'original la montre au-dessus, correctif `L4-C<n>`
(liste attendue et rang QML).

**Amendement 1 (défauts 2, 3, 8, 9 ; D17).** `pageLayersInOrder` n'est pas ajouté : **`socleLayerOrder`** (L0-S9,
`tests/tst_ui_shell.cpp`) est amendé, avec ce commentaire au-dessus de la liste : « Amendement 1 du lot 4 : la grille
du dessus sort de la scène du cache (L4-T7) ; rang fixé par D17. » Liste attendue selon D17 : recommandé
`fixedPage, pageCache, pendingLayer, liftLayer, ink, gridOverlay, pageOverlays` ; autre choix de D17 : `…, liftLayer,
gridOverlay, ink, pageOverlays`. `gridOverlay` reçoit `fixedPage: fixedPageItem` et une `Connections` (amendement 2)
`{ target: area.documentController; function onPageContentChanged(index) { if (index ===
area.documentController.currentIndex) gridOverlay.refresh() } }`
(L4-T7 n'a plus à toucher `PageArea.qml`). `PageContextMenu.qml` lit **une seule liste**, `commands.contextMenu`
(socle) ; il reçoit le délégué de sous-menu s'il ne l'a pas (`CommandMenu.qml`, même délégué que les menus).
Gestionnaire de `text.edit-at` dans `Main.qml` : fonction `editAtArgument(arg)` (`"x;y;v"` → `{x, y, vertical}`,
argument absent → `null`) ; `null` → `appText.commit()`, sinon `appText.beginAt(Qt.point(x, y), vertical)` ; test
`editAtHandlerParsesItsText` (`QMetaObject::invokeMethod` sur la racine : `"120;80;1"` → `x` 120, `y` 80, `vertical`
vrai ; `QVariant()` → `null`).

**Objectif.** Poser les éléments QML du lot à leur place, en talons nommés, et un gestionnaire pour chaque commande
du lot, pour que les tâches suivantes ne touchent plus `Main.qml`, `PageArea.qml`, `ToolPalette.qml` ni
`PageContextMenu.qml`.

**Fichiers.** Ceux de la ligne L4-T2.

**Test et commande bornée.** Commande ciblée `-R tst_ui_shell` ; preuve de tâche ; `premiers-pas` hors écran ;
banc sans écran.

**Interdits.** Aucun comportement dans les talons. Ne pas réordonner les couches du lot 3 et de L0-S (A12) :
`gridOverlay` s'insère, les autres gardent leur ordre ; `socleLayerOrder` est amendé, pas contourné.

**Terminé quand.** `tst_ui_shell` passe avec `lot4ShellPlaceholders`, `everyLot4CommandHasAHandler`,
`socleLayerOrder` amendé, `editAtHandlerParsesItsText` ; `premiers-pas` `CAPTURES OK` ; banc `content_check.ok = True`.

**Étapes.**
- [ ] **Étape 1 : test rouge.** Dans `tests/tst_ui_shell.cpp` (fabrique du fichier : moteur QML chargé sur
  `Main.qml`) :
  ```cpp
  void lot4ShellPlaceholders()
  {
      auto window = loadMainWindow(); // aide du fichier
      for (const char* name : {"browserPanel", "pageNavigator", "shapePalette", "textFormatBar", "textEditor",
                               "verticalTextBar", "gridDesigner", "backgroundDialog", "pageExtensionHandle",
                               "pageZones", "gridOverlay", "appPageSettings", "appThumbnails", "appPages", "appText"})
          QVERIFY2(findObject(window.get(), QString::fromLatin1(name)) != nullptr, name);
  }
  void everyLot4CommandHasAHandler()
  {
      auto window = loadMainWindow();
      const QStringList lot4 = lot4CommandIds(); // lit le bloc du lot 4 de commands.json (de tool.shape à page.zones)
      QCOMPARE(lot4.size(), 47);
      for (const auto& id : lot4)
          QVERIFY2(hasHandler(window.get(), id), qPrintable(id)); // gestionnaire déclaré dans Main.qml (aide du lot 3)
  }
  void socleLayerOrder() // test du socle (L0-S9), amendé : seule la liste attendue change
  {
      QQuickWindow* window = loadMainWindow(); // nom de l'aide du fichier
      QVERIFY(window);
      auto* page = window->findChild<QQuickItem*>(QStringLiteral("page"));
      QVERIFY(page);
      QStringList names;
      for (QQuickItem* child : page->childItems()) {
          names << child->objectName();
      }
      // Amendement 1 du lot 4 : la grille du dessus sort de la scène du cache (L4-T7) ; rang fixé par D17 :
      // entre l'encre et les surcouches (amendement 4).
      QCOMPARE(names, (QStringList{"fixedPage", "pageCache", "pendingLayer", "liftLayer", "ink", "gridOverlay", "pageOverlays"}));
      for (QQuickItem* child : page->childItems()) {
          QCOMPARE(child->z(), 0.0); // l'ordre des enfants fait l'empilement
      }
  }
  ```
  (Aides `loadMainWindow`, `findObject`, `hasHandler` : celles du fichier ; `lot4CommandIds` s'ajoute à côté.)
- [ ] **Étape 2 : talons QML.** Créer `BrowserPanel.qml`, `PageNavigator.qml`, `ShapePalette.qml`,
  `TextFormatBar.qml`, `TextEditor.qml`, `VerticalTextBar.qml`, `GridDesigner.qml`, `BackgroundDialog.qml`,
  `PageExtensionHandle.qml`, `PageZonesOverlay.qml` : `import QtQuick` puis `Item { visible: false }` ; ajouter les
  fichiers à la liste QML de `ui/CMakeLists.txt`.
- [ ] **Étape 3 : `Main.qml`.** Instancier `PageSettings { id: appPageSettings; objectName: "appPageSettings";
  documentController: appDocument }`, `PageThumbnails { id: appThumbnails; objectName: "appThumbnails";
  documentController: appDocument; view: pageArea.view }`, `Pages { id: appPages; objectName: "appPages";
  documentController: appDocument }`, `TextEditing { id: appText; objectName: "appText"; documentController:
  appDocument; view: pageArea.view }` ; `BrowserPanel { objectName: "browserPanel" }` à gauche de la zone de page
  (ancre à régler par L4-T19) ; `ShapePalette { objectName: "shapePalette" }` à droite, avant la palette d'outils ;
  `TextFormatBar { objectName: "textFormatBar" }` et `VerticalTextBar { objectName: "verticalTextBar" }` sous la
  ligne supérieure ; `GridDesigner { objectName: "gridDesigner" }` et `BackgroundDialog { objectName:
  "backgroundDialog" }` en fin de fenêtre. Gestionnaires des 47 commandes, sur le modèle du lot 3, chacun d'une
  ligne vers sa cible : `tool.shape` → `appTools.activate("shape")` ; `shape.choose` → `appTools.chooseShape(arg)` ;
  `shape.filled` → `appTools.setShapeFilled(!appTools.shapeFilled)` ; `shape.fill-color` →
  `appTools.setShapeFillColor(arg)` ; `shape.draw-from-center` → `appTools.setShapeFromCenter(!appTools.shapeFromCenter)` ;
  `tool.text` → `appTools.activate("text")` ; `tool.text-vertical` → `appTools.activate("text-vertical")` ;
  `text.edit-at` → `arg ? appText.beginAt(Qt.point(arg.x, arg.y), arg.vertical === true) : appText.commit()` ;
  chaque autre `text.*` → `appText.apply(id, arg)` ; `view.browsers` → bascule de `browserPanel.visible` ;
  `browsers.dock-left` / `-right` → `browserPanel.side = "left"` / `"right"` ; `page.goto` → `appPages.goTo(arg - 1)` ;
  `page.insert-before` / `-after` → `appPages.insertBlank(false / true)` ; `page.cut`, `page.copy`, `page.paste`,
  `page.duplicate`, `page.delete` → `appPages.cutSelection()`, `copySelection()`, `paste()`,
  `duplicateSelection()`, `deleteSelection()` ; `page.background` → `backgroundDialog.open()` ; `grid.designer` →
  `gridDesigner.open()` ; `grid.hide` → `appPageSettings.toggleGridHidden()` ; `grid.snap` →
  `appPageSettings.toggleSnap()` ; `view.page-extension` → bascule de `pageExtensionHandle.visible` ; `page.extend`
  → `appPageSettings.extendPage()` ; `page.zones` → `appPageSettings.setZones(arg)`. Les talons reçoivent une
  fonction vide `open()` (`GridDesigner`, `BackgroundDialog`) et une propriété `property string side: "left"`
  (`BrowserPanel`).
- [ ] **Étape 4 : `PageArea.qml`.** Dans `page`, au rang de D17 : juste après `ink`, sans `z:` : `GridOverlay {
  id: gridOverlay; objectName: "gridOverlay"; anchors.fill: parent; zoom: view.zoom; fixedPage: fixedPageItem }` et
  sa `Connections` (amendement 1) ; dans `pageOverlays` :
  `PageZonesOverlay { objectName: "pageZones"; anchors.fill: parent }` et `PageExtensionHandle { objectName:
  "pageExtensionHandle" }` ; à côté de `selectionOverlay` : `TextEditor { objectName: "textEditor" }`. La
  surcouche est un talon (L4-T7 la remplit) ; aucun pointeur de page n'est passé.
- [ ] **Étape 5 : `ToolPalette.qml`, `PageContextMenu.qml`.** Palette : le groupe `formes-texte` s'affiche comme les
  autres groupes (même délégué). Menu de page : il lit `commands.contextMenu` seul (A14) ; délégué de sous-menu
  ajouté s'il manque (même délégué que `CommandMenu.qml`).
- [ ] **Étape 6 : preuves.** `format.ps1`, commande ciblée verte, preuve de tâche, `premiers-pas` hors écran
  `CAPTURES OK`, banc sans écran, `git add`, `NOMS OK`.
- [ ] **Étape 7 : commit.**
  ```powershell
  git -C <wt> add ui/qml/Main.qml ui/qml/PageArea.qml ui/qml/ToolPalette.qml ui/qml/PageContextMenu.qml ui/qml/BrowserPanel.qml ui/qml/PageNavigator.qml ui/qml/ShapePalette.qml ui/qml/TextFormatBar.qml ui/qml/TextEditor.qml ui/qml/VerticalTextBar.qml ui/qml/GridDesigner.qml ui/qml/BackgroundDialog.qml ui/qml/PageExtensionHandle.qml ui/qml/PageZonesOverlay.qml ui/CMakeLists.txt tests/tst_ui_shell.cpp
  git -C <wt> add ui/qml/CommandMenu.qml   # seulement si modifié (délégué de sous-menu, amendement 2)
  git -C <wt> commit -m "Lot 4 : coquille QML des formes, du texte, des navigateurs et de la grille (L4-T2)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t3"></a>
### Tâche L4-T3 — `core` : catalogue des formes, boîte tracée, lignes, points intelligents

**Amendement 4 (D1, D4).** Prime sur l'objectif et les étapes ci-dessous.
- **Catalogue (D1).** Rangs 4 à 6 : `freeform` (main levée), `polyline` (sommets posés au clic, double-clic final),
  `curve-chain` (même geste, lissée), à la place de `polyline`, `zigzag`, `chevron` ; tous trois de famille
  `ShapeFamily::Line`. Défaut `line`. `zigzag` et `chevron` n'existent plus (`findShapePreset` rend `nullptr`).
  `makeLineObject` rend pour eux un segment de `from` à `to` (aperçu de la palette, boucle du test) ; leur vraie
  géométrie est celle de L4-T31 (`makeChainObject`). Le commentaire de `ShapeFamily::Line` dans l'en-tête devient
  « objet core::Line (lignes, main levée, ligne et courbe brisées) » (commentaire seul).
- **Points intelligents (D4)**, fractions de la boîte, un point par paramètre : `rounded-rectangle` {rayon 0,15 (0,01 à
  0,5 du plus petit côté)} ; `arrow` {début de la tête 0,6 (0 à 1 de la largeur), soit une tête de 0,4 ; épaisseur du
  corps 0,25 (0 à 0,5 de la hauteur)}, dans cet ordre ; `callout` {position de la pointe 1 (0 à 1 de la largeur) ;
  arrondi 0,1 (0,01 à 0,3)}, la pointe restant sous la boîte à 1,3 de la hauteur (à nous, non exposé). `star`,
  `star-four`, `burst`, `burst-twelve` : **aucun paramètre exposé** (`defaults` vide) ; leur rayon intérieur est fixe
  (0,45 ; 0,35 ; 0,6 ; 0,7). Les valeurs de départ du rayon et de l'épaisseur du corps sont à nous (D4 ne les donne pas).
- **Geste de chaque préréglage (ajout en fin de `shapegeometry.h`, signalé, autorisé par cet amendement)**, juste avant
  la fermeture de l'espace de noms :
  ```cpp
  // Amendement 4 du lot 4 (D1) : ajout en fin. Geste de pose d'un préréglage : boîte (glisser d'un coin à l'autre),
  // clics (un sommet par clic, double-clic final : polyline, curve-chain) ou main levée (freeform).
  enum class ShapeGesture : std::uint8_t
  {
      Box,
      Clicks,
      Freehand,
  };
  ShapeGesture shapeGesture(std::string_view preset) noexcept; // préréglage inconnu : Box
  ```
  L4-T8 s'en sert pour écarter du tracé en boîte les trois formes de D1 ; L4-T31 pour leur geste.
- **Tests** : `catalogOrderAndDefault` vérifie les rangs 4 à 6 et l'absence de `zigzag` et `chevron` ; trois fonctions
  de plus, `arrowHeadStartsAtFourTenths`, `smartPointsOnlyForThreePresets`, `gestureOfEachPreset` (code à l'étape 1).
  Neuf fonctions en tout.

**Objectif.** Remplir `shapegeometry` et `shapeedits` (en-têtes de L4-T1) : catalogue de 22 préréglages à nos
identifiants dans l'ordre D1, boîte depuis le coin ou le centre, carré, lignes contraintes, géométrie de chaque
préréglage (polygone de `Shape` en coordonnées de l'objet, origine au coin haut gauche de la boîte, transformation
= translation de la boîte), points intelligents de `arrow` (longueur et largeur de la tête), `rounded-rectangle`
(rayon), `star`, `star-four`, `burst`, `burst-twelve` (rayon intérieur), `callout` (pointe), commande
`SetShapeParametersCommand`. Paramètres et bornes : ceux de D4 ; à défaut, ceux du test ci-dessous.

**Fichiers.** `core/include/actcraft/core/shapegeometry.h`, `core/src/shapegeometry.cpp`,
`core/include/actcraft/core/shapeedits.h`, `core/src/shapeedits.cpp`, `tests/tst_core_shapes.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_core_shapes` ; fonctions du test : `catalogOrderAndDefault`,
`dragFromCornerAndCenter`, `squarePresetsStaySquare`, `linePresetsConstrained`, `arrowHeadSmartPointBounded`,
`arrowHeadStartsAtFourTenths`, `smartPointsOnlyForThreePresets`, `gestureOfEachPreset`,
`setParametersIsOneStepAndRebuilds` en `PASS` (neuf fonctions, `Totals: 11 passed` : k = fonctions + 2) ; preuve
de tâche.

**Interdits.** Pas de QtGui dans `core/`. Pas de géométrie dupliquée dans `tools/` ou `ui/` (ils appellent ce
fichier). Ne pas changer l'en-tête public, hormis l'ajout en fin et le commentaire de `ShapeFamily::Line` permis par
l'amendement 4.

**Terminé quand.** Les neuf fonctions passent ; `TESTS OK` ; chaque préréglage du catalogue rend une géométrie non
vide dans une boîte de 100 × 60 (boucle du test `catalogOrderAndDefault`) ; `ShapeGesture` et `shapeGesture` posés en
fin d'en-tête, ajout signalé au rapport.

**Étapes.**
- [ ] **Étape 1 : test rouge.** Remplacer le talon de `tests/tst_core_shapes.cpp` :
  ```cpp
  #include <actcraft/core/document.h>
  #include <actcraft/core/edits.h>
  #include <actcraft/core/shapeedits.h>
  #include <actcraft/core/shapegeometry.h>
  #include <actcraft/core/undo.h>

  #include <QtTest>

  using namespace actcraft::core;

  class TstCoreShapes : public QObject
  {
      Q_OBJECT

  private slots:
      void catalogOrderAndDefault()
      {
          const auto& catalog = shapeCatalog();
          QCOMPARE(catalog.size(), std::size_t{22});
          QCOMPARE(QString::fromStdString(catalog.front().id), QStringLiteral("line"));
          QCOMPARE(QString::fromStdString(catalog[3].id), QStringLiteral("freeform"));    // D1, amendement 4
          QCOMPARE(QString::fromStdString(catalog[4].id), QStringLiteral("polyline"));
          QCOMPARE(QString::fromStdString(catalog[5].id), QStringLiteral("curve-chain"));
          QCOMPARE(QString::fromStdString(catalog[6].id), QStringLiteral("rectangle"));
          QVERIFY(findShapePreset("zigzag") == nullptr);
          QVERIFY(findShapePreset("chevron") == nullptr);
          QVERIFY(findShapePreset("circle")->square);
          QVERIFY(findShapePreset("nothing") == nullptr);
          for (const auto& preset : catalog) {
              const PageObject o = preset.family == ShapeFamily::Line
                                       ? makeLineObject(preset.id, {0, 0}, {100, 60}, {})
                                       : makeShapeObject(preset.id, {0, 0, 100, 60}, {});
              QCOMPARE(o.common.layer, Layer::Middle);
              if (preset.family == ShapeFamily::Line)
                  QVERIFY2(std::get<Line>(body(o)).points.size() >= 2, preset.id.c_str());
              else
                  QVERIFY2(!std::get<Shape>(body(o)).polygon.empty() || !std::get<Shape>(body(o)).paths.empty(), preset.id.c_str());
          }
      }
      void dragFromCornerAndCenter()
      {
          const RectF corner = dragBox({10, 20}, {110, 80}, false, false);
          QCOMPARE(corner.x, 10.0);
          QCOMPARE(corner.width, 100.0);
          const RectF backwards = dragBox({110, 80}, {10, 20}, false, false);
          QCOMPARE(backwards.x, 10.0);
          QCOMPARE(backwards.height, 60.0);
          const RectF center = dragBox({100, 100}, {150, 130}, true, false);
          QCOMPARE(center.x, 50.0);
          QCOMPARE(center.y, 70.0);
          QCOMPARE(center.width, 100.0);
          QCOMPARE(center.height, 60.0);
      }
      void squarePresetsStaySquare()
      {
          const RectF box = dragBox({0, 0}, {100, -30}, false, true);
          QCOMPARE(box.width, 100.0);
          QCOMPARE(box.height, 100.0);
          QCOMPARE(box.y, -100.0);
      }
      void linePresetsConstrained()
      {
          const auto h = dragLine({0, 0}, {100, 40}, *findShapePreset("line-horizontal"), false, false);
          QCOMPARE(h.second.y, 0.0);
          const auto v = dragLine({0, 0}, {100, 40}, *findShapePreset("line-vertical"), false, false);
          QCOMPARE(v.second.x, 0.0);
          const auto d = dragLine({0, 0}, {100, 90}, *findShapePreset("line"), false, true);
          QVERIFY(std::abs(d.second.x - d.second.y) < 1e-9); // 45°
      }
      void arrowHeadSmartPointBounded()
      {
          PageObject o = makeShapeObject("arrow", {0, 0, 200, 100}, {});
          auto& shape = std::get<Shape>(body(o));
          const auto points = smartPoints(shape);
          QVERIFY(!points.empty());
          const auto preset = findShapePreset("arrow");
          const auto far = dragSmartPoint(shape, 0, {-1000, -1000});
          for (std::size_t i = 0; i < far.size(); ++i) {
              QVERIFY(far[i] >= preset->minimum[i]);
              QVERIFY(far[i] <= preset->maximum[i]);
          }
          PageObject read{Shape{}};
          QVERIFY(smartPoints(std::get<Shape>(body(read))).empty()); // forme lue : aucun
      }
      void arrowHeadStartsAtFourTenths() // D4, amendement 4
      {
          const ShapePreset* arrow = findShapePreset("arrow");
          QVERIFY(arrow != nullptr);
          QCOMPARE(arrow->defaults.size(), std::size_t{2});
          QCOMPARE(arrow->defaults[0], 0.6); // début de la tête : tête de 0,4 de la largeur
          QCOMPARE(arrow->minimum[0], 0.0);
          QCOMPARE(arrow->maximum[0], 1.0);
          QCOMPARE(arrow->minimum[1], 0.0); // épaisseur du corps, fraction de la hauteur
          QCOMPARE(arrow->maximum[1], 0.5);
      }
      void smartPointsOnlyForThreePresets() // D4, amendement 4
      {
          for (const auto& preset : shapeCatalog()) {
              const bool expected = preset.id == "rounded-rectangle" || preset.id == "arrow" || preset.id == "callout";
              QVERIFY2(preset.defaults.empty() != expected, preset.id.c_str());
              if (preset.family == ShapeFamily::Closed) {
                  PageObject o = makeShapeObject(preset.id, {0, 0, 100, 60}, {});
                  QVERIFY2(smartPoints(std::get<Shape>(body(o))).empty() != expected, preset.id.c_str());
              }
          }
          const ShapePreset* rounded = findShapePreset("rounded-rectangle");
          QCOMPARE(rounded->minimum[0], 0.01);
          QCOMPARE(rounded->maximum[0], 0.5);
          const ShapePreset* callout = findShapePreset("callout");
          QVERIFY(callout->defaults == (std::vector<double>{1.0, 0.1}));
          QVERIFY(callout->minimum == (std::vector<double>{0.0, 0.01}));
          QVERIFY(callout->maximum == (std::vector<double>{1.0, 0.3}));
      }
      void gestureOfEachPreset() // D1, amendement 4
      {
          QVERIFY(shapeGesture("freeform") == ShapeGesture::Freehand);
          QVERIFY(shapeGesture("polyline") == ShapeGesture::Clicks);
          QVERIFY(shapeGesture("curve-chain") == ShapeGesture::Clicks);
          QVERIFY(shapeGesture("line") == ShapeGesture::Box);
          QVERIFY(shapeGesture("rectangle") == ShapeGesture::Box);
          QVERIFY(shapeGesture("nothing") == ShapeGesture::Box);
      }
      void setParametersIsOneStepAndRebuilds()
      {
          Document document;
          UndoStack stack;
          stack.push(document, std::make_unique<AddObjectCommand>(0, makeShapeObject("arrow", {0, 0, 200, 100}, {})));
          const ObjectId id = document.page(0).objects.back().common.id;
          const auto before = std::get<Shape>(body(document.page(0).objects.back())).polygon;
          auto params = findShapePreset("arrow")->defaults;
          params[0] = findShapePreset("arrow")->maximum[0];
          const Changes changes = stack.push(document, std::make_unique<SetShapeParametersCommand>(0, id, params));
          QCOMPARE(changes.size(), std::size_t{1});
          QCOMPARE(changes[0].kind, Change::Kind::ObjectChanged);
          QVERIFY(std::get<Shape>(body(document.page(0).objects.back())).polygon != before);
          stack.undo(document);
          QCOMPARE(std::get<Shape>(body(document.page(0).objects.back())).polygon, before);
      }
  };

  QTEST_GUILESS_MAIN(TstCoreShapes)
  #include "tst_core_shapes.moc"
  ```
  (Si `PointF` n'a pas d'`operator==`, comparer les sommets un à un par une aide locale.) Commande ciblée → rouge.
- [ ] **Étape 2 : catalogue.** Dans `shapegeometry.cpp`, un `static const std::vector<ShapePreset>` construit une
  fois : familles `Line` pour `line`, `line-horizontal` (axe 1), `line-vertical` (axe 2), `freeform`, `polyline`,
  `curve-chain` (amendement 4) ; `square` pour `square` et `circle` ; paramètres de l'amendement 4 (D4) :
  `rounded-rectangle` {0,15 (0,01 à 0,5)}, `arrow` {0,6 (0 à 1) ; 0,25 (0 à 0,5)}, `callout` {1 (0 à 1) ; 0,1 (0,01 à
  0,3)} ; aucun pour les étoiles et les explosions. `findShapePreset` : recherche linéaire. `shapeGesture` :
  `Freehand` pour `freeform`, `Clicks` pour `polyline` et `curve-chain`, `Box` sinon.
- [ ] **Étape 3 : boîtes et lignes.** `dragBox` : coin = boîte de `press` à `current` normalisée ; centre =
  boîte centrée sur `press` de demi-côtés `|current − press|` ; `square` : le plus grand des deux, signe du geste.
  `dragLine` : depuis le centre, `from = press − (current − press)` ; axe 1 : `to.y = from.y` ; axe 2 :
  `to.x = from.x` ; `constrain45` : angle arrondi au multiple de 45° le plus proche, longueur gardée.
- [ ] **Étape 4 : géométrie.** `rebuildShape` : `ShapeKind::Polygon` pour tous les préréglages fermés, sommets
  dans la boîte `{0, 0, w, h}` (ellipse et cercle : 64 sommets ; rectangle arrondi : arcs de 8 sommets par coin ;
  polygones réguliers inscrits ; étoiles et explosions alternées entre rayon extérieur et rayon intérieur fixe ;
  flèche : corps puis tête, la tête commençant au paramètre 0 ; bulle : rectangle arrondi plus pointe). `bounds = {0, 0, w, h}` ; `makeShapeObject` pose
  `common.transform = translation(box.x, box.y)`, `outline`, `outlineWidth = style.width`, `fill =
  style.filled ? style.fill : 0` (alpha nul), `pen`, `brush`, `preset`, `parameters = defaults`.
  `makeLineObject` : `Line` à deux points pour toute la famille (lignes et axes ; `freeform`, `polyline`,
  `curve-chain` : segment de remplacement, amendement 4), plume = contour.
- [ ] **Étape 5 : points intelligents.** `smartPoints` et `dragSmartPoint` : un point par paramètre (D4 : rayon du
  rectangle arrondi ; début de la tête et épaisseur du corps de la flèche ; pointe et arrondi de la bulle), positions
  dans la boîte, conversion inverse bornée par `minimum` / `maximum` ; aucun point pour un préréglage sans paramètre.
- [ ] **Étape 6 : commande.** `SetShapeParametersCommand::redo` : objet trouvé par identifiant sur la page, `Shape`
  avec préréglage ; garde `m_before` au premier passage, pose `m_after`, `rebuildShape`, un `ObjectChanged` de zone
  emprise avant ∪ après (emprise calculée comme `AddObjectCommand` le fait, fonction d'emprise de `edits`) ; `undo`
  symétrique.
- [ ] **Étape 7 : vert**, `format.ps1`, preuve de tâche.
- [ ] **Étape 8 : commit.**
  ```powershell
  git -C <wt> add core/include/actcraft/core/shapegeometry.h core/src/shapegeometry.cpp core/include/actcraft/core/shapeedits.h core/src/shapeedits.cpp tests/tst_core_shapes.cpp
  git -C <wt> commit -m "core : catalogue des formes, tracé depuis le coin ou le centre, points intelligents (L4-T3)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t4"></a>
### Tâche L4-T4 — `core` : réglages de page et contrat de `Change` ignoré par le cache

**Écart constaté (amendement 5, D10).** Le pas cumulé observé par L4-O1 (× 1,1 sur la hauteur courante) a été
appliqué pendant l'exécution, sans correctif : `kExtendRatio` et `nextExtendedHeight`
(`core/include/actcraft/core/pagesettings.h:11`, `:56-57`) ; le test s'appelle `extendStepsByTenPercentOfCurrent`
(`tests/tst_core_pagesettings.cpp:136`, 1022,208 attendu) au lieu de `extendStepsByTenPercentOfBase` (nom de la table,
gardé pour l'historique). Aucun `L4-C<n>` : L4-T26 (plan 4b) reçoit l'attendu 1022,208.

**Amendement 4 (V7).** La fusion d'une séance du créateur passe par le mécanisme existant : `mergeId()` et
`mergeWith()` de `UndoCommand` (`core/include/actcraft/core/undo.h:44-52`) ; `UndoStack::closeMerge()` pose un
plancher et ne fusionne rien (`core/src/undo.cpp:62-65`). L4-T13 appelle `DocumentController::closeMerge()`
(`ui/include/actcraft/ui/documentcontroller.h:88`) à l'ouverture et à la fermeture de chaque séance ; cette tâche
prouve que deux séances séparées par `closeMerge()` font deux étapes (`closeMergeSeparatesTwoSessions`, ci-dessous).
`kMergeGrid = 4` est libre (V7) : vérifier par `git -C <wt> grep -n "mergeId" -- core tools ui` qu'aucune commande ne
rend 4, et l'écrire au rapport. La règle de l'allongement (pas fixe ou cumulé, D10) est observée par L4-O1 : si elle
est cumulée, `extendStepsByTenPercentOfBase` attend `768 × 1,1³` (≈ 1022,2) par un correctif `L4-C<n>`.

**Objectif.** Remplir `pagesettings` : `SetGridCommand` (fusion d'une séance du créateur), `SetBackgroundCommand`,
`ExtendPageCommand`, `nextExtendedHeight`, `gridDrawsUnderObjects`, `gridDrawsAboveObjects`. **Le patron évident
est le mauvais** : émettre `PageChanged` pour toute grille ferait redessiner tout le cache pour une grille du dessus,
qui n'est plus dans le cache (L4-T7). Contrat : une grille du dessous qui change (avant ou après) → `PageChanged`
(zone vide = toute la page) ; sinon, un changement de page **ignoré par le cache** mais vu par l'interface
(miniature, surcouche).

**Contrat A13 (amendement 1, défaut 7).** Le socle a posé `Change::Kind::PageSettingsChanged` (L0-S3, D-S5),
ignoré par `applyToCache`. `SetGridCommand` l'émet pour toute grille qui ne dessine rien sous les objets avant comme
après ; **rien** n'est ajouté à `undo.h` ni à `pagecache.cpp` (`PageOverlayChanged` n'existe pas) ; `undo.h`,
`pagecache.cpp` et `tst_core_pagecache.cpp` ne sont pas touchés par cette tâche. `DocumentController::settle`
émet `pageContentChanged` pour ce changement (contrat de L4-T1 : « objet ou page »).

**Fichiers.** `core/include/actcraft/core/pagesettings.h`, `core/src/pagesettings.cpp`,
`tests/tst_core_pagesettings.cpp` (amendement 2, N6).

**Test et commande bornée.** Commande ciblée `-R tst_core_pagesettings` ; fonctions
`gridAboveChangeIsIgnoredByTheCache`, `gridBelowChangeRedrawsOnce`, `extendStepsByTenPercentOfBase`,
`threeUndosRestoreTheHeight`, `designerChangesMerge`, `closeMergeSeparatesTwoSessions` en `PASS` (`Totals: 8
passed` : six fonctions + 2) ; mutation ; preuve de tâche.

**Interdits.** Ne pas changer le sens des `Change` existants. Ne pas toucher à `edits.cpp`.

**Terminé quand.** Fonctions vertes ; mutation de `gridDrawsUnderObjects` (rendre toujours `true`) rouge sur
`gridAboveChangeIsIgnoredByTheCache`, puis remise et verte (règle « Mutation ») ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge.** `tests/tst_core_pagesettings.cpp` :
  ```cpp
  #include <actcraft/core/document.h>
  #include <actcraft/core/pagecache.h>
  #include <actcraft/core/pagesettings.h>
  #include <actcraft/core/undo.h>

  #include <QtTest>

  using namespace actcraft::core;

  namespace {
  struct Spy final : PageCache
  {
      int inserts = 0, removes = 0, invalidations = 0;
      void insert(ObjectId) override { ++inserts; }
      void remove(ObjectId, const RectF&) override { ++removes; }
      void invalidate(ObjectId, const CacheArea&) override { ++invalidations; }
  };
  Grid aboveGrid(bool visible)
  {
      Grid g;
      g.visible = visible;
      g.drawnAbove = true;
      g.levels.push_back(GridLevel{});
      return g;
  }
  } // namespace

  class TstCorePageSettings : public QObject
  {
      Q_OBJECT

  private slots:
      void gridAboveChangeIsIgnoredByTheCache()
      {
          Document document;
          document.page(0).grid = aboveGrid(true);
          UndoStack stack;
          const Changes changes = stack.push(document, std::make_unique<SetGridCommand>(0, aboveGrid(false)));
          QCOMPARE(changes.size(), std::size_t{1}); // l'interface est prévenue
          Spy spy;
          applyToCache(changes, 0, spy);
          QCOMPARE(spy.invalidations, 0);
          QVERIFY(!document.page(0).grid.visible);
      }
      void gridBelowChangeRedrawsOnce()
      {
          Document document;
          Grid below = aboveGrid(true);
          below.drawnAbove = false;
          UndoStack stack;
          const Changes changes = stack.push(document, std::make_unique<SetGridCommand>(0, below));
          Spy spy;
          applyToCache(changes, 0, spy);
          QCOMPARE(spy.invalidations, 1);
      }
      void extendStepsByTenPercentOfBase()
      {
          Document document; // 1024 × 768
          UndoStack stack;
          stack.push(document, std::make_unique<ExtendPageCommand>(0));
          QCOMPARE(document.page(0).baseHeight, 768.0);
          QVERIFY(std::abs(document.page(0).height - 844.8) < 1e-9);
          stack.push(document, std::make_unique<ExtendPageCommand>(0));
          stack.push(document, std::make_unique<ExtendPageCommand>(0));
          QVERIFY(std::abs(document.page(0).height - 998.4) < 1e-9); // pas fixe (D10)
      }
      void threeUndosRestoreTheHeight()
      {
          Document document;
          UndoStack stack;
          for (int i = 0; i < 3; ++i)
              stack.push(document, std::make_unique<ExtendPageCommand>(0));
          for (int i = 0; i < 3; ++i)
              stack.undo(document);
          QCOMPARE(document.page(0).height, 768.0);
          QCOMPARE(document.page(0).baseHeight, 0.0);
      }
      void designerChangesMerge()
      {
          Document document;
          UndoStack stack;
          Grid g = aboveGrid(true);
          stack.push(document, std::make_unique<SetGridCommand>(0, g, false));
          g.levels[0].spacingX = 50;
          stack.push(document, std::make_unique<SetGridCommand>(0, g, true));
          g.levels[0].spacingX = 60;
          stack.push(document, std::make_unique<SetGridCommand>(0, g, true));
          QCOMPARE(stack.count(), std::size_t{1});
          stack.undo(document);
          QVERIFY(!document.page(0).grid.visible);
      }
      void closeMergeSeparatesTwoSessions() // amendement 4 (V7) : une étape par séance du créateur
      {
          Document document;
          UndoStack stack;
          Grid g = aboveGrid(true);
          stack.push(document, std::make_unique<SetGridCommand>(0, g, true));
          g.levels[0].spacingX = 50;
          stack.push(document, std::make_unique<SetGridCommand>(0, g, true));
          stack.closeMerge(); // fermeture de la séance (L4-T13)
          g.levels[0].spacingX = 60;
          stack.push(document, std::make_unique<SetGridCommand>(0, g, true));
          QCOMPARE(stack.count(), std::size_t{2});
          stack.undo(document);
          QCOMPARE(document.page(0).grid.levels[0].spacingX, 50.0);
      }
  };

  QTEST_GUILESS_MAIN(TstCorePageSettings)
  #include "tst_core_pagesettings.moc"
  ```
  Si D10 retient un pas cumulé, `extendStepsByTenPercentOfBase` attend `768 × 1,1³` (amendement).
- [ ] **Étape 2 : contrat A13.** `gridAboveChangeIsIgnoredByTheCache` attend `PageSettingsChanged` (page 0) et un
  espion de `core::applyToCache` sans appel ; aucun fichier hors de la ligne L4-T4 de la table.
- [ ] **Étape 3 : commandes.** `gridDrawsUnderObjects` : `visible && !drawnAbove && un niveau visible` ;
  `gridDrawsAboveObjects` : `visible && drawnAbove && un niveau visible`. `SetGridCommand::redo` : garde la grille
  d'avant au premier passage, pose la nouvelle ; un seul `Change` de page : `PageChanged` si
  `gridDrawsUnderObjects(avant) || gridDrawsUnderObjects(après)`, sinon le changement ignoré par le cache.
  Fusion : la famille `kMergeGrid` par le mécanisme de fusion de la pile (`mergeId` et `mergeWith` de `UndoCommand`,
  lot 2), seulement si `merge` est vrai et la page la même ; la commande fusionnée garde le « avant » de la première.
  `SetBackgroundCommand` : `PageChanged`. `ExtendPageCommand::redo` : `m_beforeHeight`, `m_beforeBase`, puis
  `baseHeight = base` (premier clic), `height = nextExtendedHeight(page)` ; `PageChanged` ; `undo` remet les deux.
- [ ] **Étape 4 : vert**, puis mutation (règle « Mutation ») : `gridDrawsUnderObjects` rend `true` → rouge sur
  `gridAboveChangeIsIgnoredByTheCache` ; remettre → vert.
- [ ] **Étape 5 : preuve de tâche, commit.**
  ```powershell
  git -C <wt> add core/include/actcraft/core/pagesettings.h core/src/pagesettings.cpp tests/tst_core_pagesettings.cpp
  git -C <wt> commit -m "core : réglages de page (grille, fond, extension), grille du dessus ignorée par le cache (L4-T4)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t5"></a>
### Tâche L4-T5 — `core` : accrochage à la grille

**Objectif.** Remplir `gridsnap` (F-622) d'après la règle reformulée de la référence (`canvas/grid.py:221-251`) :
pour chaque niveau qui accroche, grille normale = arrondi de `(p − début) / pas` sur chaque axe dessiné ; grilles
« 45° » et « angle » = croisement le plus proche parmi les croisements voisins (rayon de deux pas) ; puis le plus
proche de tous les niveaux. Pas effectif = pas × `baseZoom` (« Échelle par défaut »).

**Fichiers.** `core/include/actcraft/core/gridsnap.h`, `core/src/gridsnap.cpp`, `tests/tst_core_gridsnap.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_core_gridsnap` ; fonctions `nearestOnNormalGrid`,
`onlySnappingLevelsCount`, `invisibleGridStillSnaps`, `snapDisabledReturnsNothing`,
`bottomLeftCornerLandsOnCrossing`, `textUsesItsBaseline`, `diagonalGridCrossings` en `PASS` ; preuve de tâche.

**Interdits.** Pas de QtGui. Ne pas dupliquer le calcul des croisements de `render/` : si le peintre en a un, le
laisser où il est ; `core/` a le sien (le peintre pourra l'appeler au lot 11, passe de nettoyage).

**Terminé quand.** Fonctions vertes, `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge.** `tests/tst_core_gridsnap.cpp` :
  ```cpp
  #include <actcraft/core/gridsnap.h>

  #include <QtTest>

  using namespace actcraft::core;

  namespace {
  Grid grid(double step, bool snapEnabled = true)
  {
      Grid g;
      g.visible = true;
      g.snapEnabled = snapEnabled;
      GridLevel level;
      level.spacingX = step;
      level.spacingY = step;
      g.levels.push_back(level);
      return g;
  }
  } // namespace

  class TstCoreGridSnap : public QObject
  {
      Q_OBJECT

  private slots:
      void nearestOnNormalGrid()
      {
          const auto p = nearestGridPoint(grid(50), {74, 26}, 1024, 768);
          QVERIFY(p.has_value());
          QCOMPARE(p->x, 50.0);
          QCOMPARE(p->y, 50.0);
      }
      void onlySnappingLevelsCount()
      {
          Grid g = grid(50);
          GridLevel fine;
          fine.spacingX = fine.spacingY = 10;
          fine.snap = false;
          g.levels.push_back(fine);
          QCOMPARE(nearestGridPoint(g, {74, 26}, 1024, 768)->x, 50.0);
      }
      void invisibleGridStillSnaps()
      {
          Grid g = grid(50);
          g.visible = false;
          QVERIFY(nearestGridPoint(g, {74, 26}, 1024, 768).has_value());
      }
      void snapDisabledReturnsNothing() { QVERIFY(!nearestGridPoint(grid(50, false), {74, 26}, 1024, 768).has_value()); }
      void bottomLeftCornerLandsOnCrossing()
      {
          Page page;
          page.grid = grid(50);
          PageObject o{Shape{}};
          o.common.id = 7;
          std::get<Shape>(body(o)).bounds = {0, 0, 40, 30};
          o.common.transform = translation(12, 12); // coin bas gauche en (12, 42)
          page.objects.push_back(o);
          const PointF d = snapDelta(page, {7}, {30, 0}); // ancrage visé (42, 42) → (50, 50)
          QCOMPARE(d.x, 38.0);
          QCOMPARE(d.y, 8.0);
          page.grid.snapEnabled = false;
          QCOMPARE(snapDelta(page, {7}, {30, 0}).x, 30.0);
      }
      void textUsesItsBaseline()
      {
          PageObject o{Text{}};
          auto& text = std::get<Text>(body(o));
          text.bounds = {0, 0, 100, 40};
          TextSpan span;
          span.baseline = {0, 30};
          text.runs.push_back(span);
          o.common.transform = translation(10, 10);
          const PointF a = snapAnchor(o);
          QCOMPARE(a.x, 10.0);
          QCOMPARE(a.y, 40.0);
      }
      void diagonalGridCrossings()
      {
          Grid g = grid(50);
          g.levels[0].pattern = GridPattern::Diagonal45;
          const auto p = nearestGridPoint(g, {26, 24}, 1024, 768);
          QVERIFY(p.has_value());
          QVERIFY(std::hypot(p->x - 26, p->y - 24) <= 50 * std::sqrt(2.0));
      }
  };

  QTEST_GUILESS_MAIN(TstCoreGridSnap)
  #include "tst_core_gridsnap.moc"
  ```
- [ ] **Étape 2 : implémenter.** `nearestGridPoint` ; `snapAnchor` : `textBaseline` (en-tête de L4-T16, talon qui
  rend `std::nullopt` tant que L4-T16 n'est pas faite : le test `textUsesItsBaseline` lit alors la ligne lue
  directement — implémenter ici la lecture de `runs[0].baseline` dans une fonction locale, et appeler
  `textBaseline` seulement si elle rend une valeur) ; sinon coin bas gauche de l'emprise transformée (fonction
  d'emprise de `edits`) ; `snapDelta` : ancrage du premier objet de `ids` dans l'ordre de la page, `+ delta`, puis
  écart à l'intersection la plus proche.
- [ ] **Étape 3 : vert**, `format.ps1`, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add core/include/actcraft/core/gridsnap.h core/src/gridsnap.cpp tests/tst_core_gridsnap.cpp
  git -C <wt> commit -m "core : accrochage à la grille, coin bas gauche et ligne de base (L4-T5)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t6"></a>
### Tâche L4-T6 — `formats` : natif — champs du lot 4

**Écart constaté (amendement 5).** Deux suites, sans reprise de cette tâche : (1) toute clé facultative ajoutée plus
tard doit entrer dans le gabarit `everyPageKey` (`formats/src/nativejson.cpp:1692-1694`) hors défaut, sinon elle est
lue comme inconnue : L4-T27 y met `zones` (test `zonesKeyIsKnown`) ; (2) un objet de type inconnu est réécrit depuis
son JSON d'origine, identifiant compris (`:710-718`) : correctif **L4-C4** (plan 4b).

**Objectif.** Lire et écrire en `.actcraft` les champs posés par L4-T1 : `"preset"` et `"parameters"` d'une forme,
`"vertical"` d'un texte, `"baseHeight"` d'une page, **omis quand ils valent leur défaut** (un fichier du lot 3
réécrit sans modification garde ses octets de `document.json`). **Le patron évident est le mauvais** : ajouter la
clé toujours, ou la ranger parmi les clés inconnues gardées (L2-C8) — une clé connue lue doit sortir de
`unknownKeys`, sinon elle serait écrite deux fois. Un fichier plus ancien se lit avec les défauts ; une valeur
invalide (préréglage qui n'est pas une chaîne, paramètre non fini) donne un avertissement de lecture et le défaut.

**Fichiers.** `formats/src/nativejson.cpp`, `tests/tst_formats_lot4.cpp`.

**Test et commande bornée.** Commande ciblée `-R 'tst_formats_lot4|tst_formats_nativejson|tst_formats_nativefile'` ;
fonctions `presetAndParametersRoundTrip`, `defaultsAreOmitted`, `verticalTextRoundTrip`, `baseHeightRoundTrip`,
`olderFileReadsWithDefaults` en `PASS` ; preuve de tâche.

**Interdits.** Pas de QtGui dans `formats/`. Pas de version de format changée (`actcraft-document/1` : clés
facultatives ajoutées). Ne pas toucher au lecteur `.flipchart` (L4-T24).

**Terminé quand.** Fonctions vertes ; les tests existants du format natif inchangés et verts ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge.** `tests/tst_formats_lot4.cpp` (fonctions de lecture et d'écriture JSON du module :
  `formats::documentToJson` / `formats::documentFromJson` ou leurs noms réels dans `nativejson.h`, à lire d'abord) :
  ```cpp
  #include <actcraft/core/document.h>
  #include <actcraft/formats/nativejson.h>

  #include <QtTest>

  using namespace actcraft;

  namespace {
  core::Document sample()
  {
      core::Document document;
      core::PageObject shape{core::Shape{}};
      shape.common.id = 1;
      std::get<core::Shape>(core::body(shape)).preset = "arrow";
      std::get<core::Shape>(core::body(shape)).parameters = {0.4, 0.2};
      core::PageObject text{core::Text{}};
      text.common.id = 2;
      std::get<core::Text>(core::body(text)).html = "<p>ABC</p>";
      std::get<core::Text>(core::body(text)).vertical = true;
      document.page(0).objects = {shape, text};
      document.page(0).baseHeight = 768;
      document.page(0).height = 844.8;
      document.refreshNextId();
      return document;
  }
  core::Document roundTrip(const core::Document& d)
  {
      QStringList warnings;
      auto back = formats::documentFromJson(formats::documentToJson(d), &warnings);
      Q_ASSERT(back.has_value());
      return std::move(*back);
  }
  } // namespace

  class TstFormatsLot4 : public QObject
  {
      Q_OBJECT

  private slots:
      void presetAndParametersRoundTrip()
      {
          const auto d = roundTrip(sample());
          const auto& s = std::get<core::Shape>(core::body(d.page(0).objects[0]));
          QCOMPARE(QString::fromStdString(s.preset), QStringLiteral("arrow"));
          QCOMPARE(s.parameters.size(), std::size_t{2});
          QCOMPARE(core::fingerprint(d), core::fingerprint(sample()));
      }
      void defaultsAreOmitted()
      {
          core::Document plain;
          plain.page(0).objects.push_back(core::PageObject{core::Shape{}});
          plain.page(0).objects.back().common.id = 1;
          const QByteArray json = formats::documentToJson(plain);
          QVERIFY(!json.contains("\"preset\""));
          QVERIFY(!json.contains("\"parameters\""));
          QVERIFY(!json.contains("\"vertical\""));
          QVERIFY(!json.contains("\"baseHeight\""));
      }
      void verticalTextRoundTrip()
      {
          QVERIFY(std::get<core::Text>(core::body(roundTrip(sample()).page(0).objects[1])).vertical);
      }
      void baseHeightRoundTrip() { QCOMPARE(roundTrip(sample()).page(0).baseHeight, 768.0); }
      void olderFileReadsWithDefaults()
      {
          core::Document plain;
          plain.page(0).objects.push_back(core::PageObject{core::Text{}});
          plain.page(0).objects.back().common.id = 1;
          const auto back = roundTrip(plain);
          QVERIFY(!std::get<core::Text>(core::body(back.page(0).objects[0])).vertical);
          QVERIFY(back.page(0).objects[0].unknownKeys.empty());
          QCOMPARE(back.page(0).baseHeight, 0.0);
      }
  };

  QTEST_GUILESS_MAIN(TstFormatsLot4)
  #include "tst_formats_lot4.moc"
  ```
  (Adapter les deux appels aux signatures réelles de `nativejson.h`, sans changer ce que le test vérifie.)
- [ ] **Étape 2 : implémenter** dans `nativejson.cpp`, à côté des clés des mêmes objets : écriture conditionnelle,
  lecture avec contrôle de type, retrait des clés connues de l'ensemble « inconnues ».
- [ ] **Étape 3 : vert**, `format.ps1`, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add formats/src/nativejson.cpp tests/tst_formats_lot4.cpp
  git -C <wt> commit -m "formats : préréglage et points des formes, texte de haut en bas, hauteur d'origine en .actcraft (L4-T6)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-c3"></a>
### Tâche L4-C3 — Correctif de L4-T2 : grille du dessus remise entre `ink` et `pageOverlays`

**Origine.** Amendement 5, point 2. L4-T2 a posé `gridOverlay` sous `ink` d'après L4-O1 (« le trait en cours passe sur
les lignes »). L4-O2 montre que la grille du dessus couvre tout ce qui est posé, quel que soit le calque (trait,
rectangle rempli, texte), et explique l'écart : L4-O1 cochait l'affichage et la grille du dessus dans la même ouverture
de la boîte de réglage. Le trait **en cours** est couvert lui aussi, par décision de l'orchestrateur : rien ne change au
relâché (L4-O1), et l'ancien dépôt peint la grille au premier plan (P15). Ce correctif remet le rang de D17 (amendement
4) : `gridOverlay` entre `ink` et `pageOverlays`, chaque enfant de `page` à `z() == 0`. L4-T7 en dépend.

**Objectif.** Dans `ui/qml/PageArea.qml`, déplacer le bloc `GridOverlay { id: gridOverlayItem … }` **et** sa
`Connections` (aujourd'hui `:102-119`, entre `liftLayerItem` et `inkLayer`) juste après `InkLayer { id: inkLayer … }`,
avant `Item { id: pageOverlaysItem … }`. Rien d'autre ne bouge : ni les propriétés de la surcouche, ni l'alias
`gridOverlay`, ni les autres couches. Aucun `z:`.

**Fichiers.** `ui/qml/PageArea.qml`, `tests/tst_ui_shell.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_shell` (`100% tests passed, 0 tests failed out of 1`) ;
fonctions du test : `socleLayerOrder` et `lot4ShellPlaceholders` en `PASS`, aucun `FAIL!`, `Totals: … 0 failed,
0 skipped` ; preuve de tâche (N = 136 + n_B **inchangé**, `TESTS OK`, ligne `tst_ui_shell … Passed`) ; `premiers-pas`
hors écran `-Lire "pageIndicator.text=1 / 1"` → `CAPTURES OK` ; banc sans écran `content_check.ok = True`.

**Interdits.** Ne réordonner aucune autre couche (A12). Ne pas toucher `GridOverlayItem` (L4-T7 la remplit). Pas de
`z:`. Ne pas contourner `socleLayerOrder` : sa liste attendue change, sa boucle `z() == 0` reste.

**Terminé quand.** `socleLayerOrder` rouge avec la liste amendée sur le QML d'avant, puis vert après le déplacement ;
`TESTS OK` ; N inchangé ; `premiers-pas` `CAPTURES OK` ; banc joint ; `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge.** Dans `tests/tst_ui_shell.cpp`, fonction `socleLayerOrder` (`:773-789`), remplacer le
  commentaire et la liste attendue par :
  ```cpp
        // Amendement 1 du lot 4 : la grille du dessus sort de la scène du cache (L4-T7) ; rang de D17, remis par
        // L4-C3 : entre l'encre et les surcouches. L4-O2 : la grille du dessus couvre tout ce qui est posé ; le
        // trait en cours aussi (rien ne change au relâché, L4-O1 ; grille peinte au premier plan, P15).
        QCOMPARE(names, (QStringList{"fixedPage", "pageCache", "pendingLayer", "liftLayer", "ink", "gridOverlay",
                                     "pageOverlays"}));
  ```
  La boucle `QCOMPARE(child->z(), 0.0)` reste telle quelle. Commande ciblée → `socleLayerOrder` en `FAIL!` (liste
  reçue : `…, "liftLayer", "gridOverlay", "ink", …`).
- [ ] **Étape 2 : déplacer** le bloc de la surcouche dans `PageArea.qml`, après `inkLayer`, avec ce commentaire au-dessus :
  ```qml
            // Lot 4 : grille du dessus (F-620), au-dessus de l'encre en cours et de tout ce qui est posé (D17 ;
            // observation de l'original L4-O2, correctif L4-C3), hors de la scène que le cache photographie.
            // Talon : L4-T7 la remplit. Sans `z:` : l'ordre des enfants fait l'empilement.
            GridOverlay {
                id: gridOverlayItem
                objectName: "gridOverlay"
                anchors.fill: parent
                zoom: view.zoom
                fixedPage: fixedPageItem
            }
            // PageSettingsChanged n'atteint pas la page fixe : la grille est relue sur le changement de la page
            // affichée.
            Connections {
                target: area.documentController
                function onPageContentChanged(page) {
                    if (area.documentController && page === area.documentController.currentIndex)
                        gridOverlayItem.refresh()
                }
            }
  ```
- [ ] **Étape 3 : vert.** `format.ps1`, commande ciblée verte, fonctions du test, preuve de tâche, `premiers-pas` hors
  écran, banc sans écran, `git add`, `NOMS OK`.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add ui/qml/PageArea.qml tests/tst_ui_shell.cpp
  git -C <wt> commit -m "ui : grille du dessus au-dessus de l'encre en cours, rang de D17 rétabli (L4-C3)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t7"></a>
### Tâche L4-T7 — `render` : grille du dessus en surcouche, retirée du cache

**Amendement 5 (D17, L4-O2).** Prime sur les amendements 1 et 4 et sur l'objectif. Dépend de **L4-C3**, qui remet la
surcouche entre `ink` et `pageOverlays` (`ui/qml/PageArea.qml` : L4-T2 → L4-C3 → cette tâche) ; ne pas déplacer la
surcouche ici. Contrat : la grille du dessus est dessinée sur **toute la page fixe**, objets posés compris (traits,
formes, textes, quel que soit leur calque) et trait en cours compris : la surcouche ne masque rien et ne découpe
rien autour des objets ; c'est la conception d'origine (grille peinte au premier plan, P15). « Au-dessus de
`liftLayer` » dans l'objectif se lit « au-dessus de `ink` ». Test de plus dans `tst_render_gridoverlay` :
`gridAboveCoversAPlacedShape` — fenêtre hors écran de 320 × 320 en rendu logiciel (montage des autres fonctions),
page fixe puis surcouche dans cet ordre, sur la même page ; page à grille du dessus (un niveau, pas de 32, noir,
lignes pleines) portant un rectangle rempli rouge (`core::Shape`, préréglage `rectangle`, remplissage `0xFFFF0000`,
contour transparent) de (100, 100) à (300, 300) ; image saisie : le pixel (128, 150), sur la ligne `x = 32 × 4`,
est noir à 8 niveaux près ; le pixel (140, 150), entre deux lignes, est rouge à 8 niveaux près ; même contrôle en
(150, 160), sur la ligne `y = 32 × 5`. Les fonctions de la table restent ; `Totals` = fonctions + 2.

**Amendement 4 (A12).** Citations corrigées dans le paragraphe suivant et dans l'objectif : `canWait` en
`render/src/fixedpageitem.cpp:54-59`, appelé en `:195` et `:241`, commentaires à revoir en `:50-53` et `:246-247`.
Rang de la surcouche : entre `ink` et `pageOverlays` (D17, posé par L4-T2) ; la surcouche couvre donc aussi le trait
en cours.

**Amendement 1 (défaut 8, amendement 4).** Le prédicat est `canWait(page, object)`
(`render/src/fixedpageitem.cpp:54-59`, appelé en `:195` et `:241`, amendement 4) ; la page se lit par `FixedPageItem::page()`
(`fixedpageitem.h:46`, amendement 2) : retirer `!page.grid.drawnAbove` en gardant les
images et l'encre révélatrice ; mettre à jour les commentaires `:50-53` et `:246-247` (« grille au-dessus » n'est plus une
cause). L'en-tête de `GridOverlayItem` (L4-T1, amendé) porte déjà `fixedPage` et `refresh()` ; `PageArea.qml` les
branche déjà (L4-T2). `PageSettingsChanged` n'atteint pas `FixedPageItem` (`presentationChangeRedrawsNothing`) :
la surcouche ne relit la grille que sur `refresh()` ou sur un changement de source de la page fixe. Test de plus dans
`tst_render_gridoverlay` : `refreshRereadsTheGrid` (grille du dessus masquée dans la page source, sans `refresh()` :
`rebuildCount` inchangé ; après `refresh()` et un rendu : + 1 et l'image transparente). Dépend de L4-T4
(`SetGridCommand`).

**Objectif.** La grille « toujours visible » est dessinée par `GridOverlayItem` (au-dessus de `ink`, rang remis par
L4-C3, amendement 5) et **plus** par `FixedPageItem` ; la condition `!shown.grid.drawnAbove` de la règle de différé
(`canWait`, `render/src/fixedpageitem.cpp:54-59`, lot 10) tombe, puisque rien n'est plus dessiné au-dessus des objets dans la
scène du cache. **Le patron évident est le mauvais** : garder la grille du dessus dans la page fixe et y insérer
les objets « avant » elle (lot 2) ; cela fige la limite de DA11 et redessine tout à chaque bascule.
Branchement : `render/` ne connaît pas `ui/`, et le QML ne passe pas de pointeur de page. La surcouche lit
donc la page par la page fixe : `GridOverlayItem` reçoit une propriété
`Q_PROPERTY(actcraft::render::FixedPageItem* fixedPage READ fixedPage WRITE setFixedPage NOTIFY fixedPageChanged)`
(posée par l'en-tête de L4-T1, amendement 1), se branche sur le signal de changement de source et sur le signal de
contenu de `FixedPageItem` (existants, ou ajoutés à la fin de son en-tête), et relit la grille de la page affichée.
`PageArea.qml` a déjà `fixedPage: fixedPageItem` et la `Connections` qui appelle `refresh()` (L4-T2) ; n'y toucher
que s'ils manquent.

**Fichiers.** Ceux de la ligne L4-T7 de la table (`ui/qml/PageArea.qml` : une ligne).

**Test et commande bornée.** Commande ciblée
`-R 'tst_render_gridoverlay|tst_render_pendinglayer|tst_render_fixedpage'` ; fonctions `drawsOnlyTheGridAbove`,
`hidingTheGridAboveRebuildsNoCache`, `sharpOnePixelLines`, `refreshRereadsTheGrid`, `gridAboveCoversAPlacedShape`
(amendement 5), `strokeOnGridAbovePageIsDeferred` en `PASS` ; banc sans
écran ; preuve de tâche.

**Interdits.** Ne pas modifier un test du lot 10 sans comprendre : `tst_render_pendinglayer` a pu asserter qu'un
trait sur page à grille du dessus n'est pas différé ; cette assertion est **remplacée** par
`strokeOnGridAbovePageIsDeferred`, avec un commentaire qui renvoie à cette tâche (changement de règle voulu). Les
miniatures et le peintre partagé dessinent toujours la grille du dessus (inchangés).

**Terminé quand.** Fonctions vertes ; `tst_render_fixedpage` vert ; banc `content_check.ok = True` ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** `tests/tst_render_gridoverlay.cpp` (fenêtre hors écran en rendu logiciel, aides
  de `tst_render_fixedpage.cpp` recopiées : `QQuickWindow` + `QQuickRenderControl` ou `grabWindow`) :
  `drawsOnlyTheGridAbove` (page à grille du dessous : l'image de la surcouche est transparente ; grille du dessus :
  pixels noirs sur les lignes `x = 32 × k`) ; `hidingTheGridAboveRebuildsNoCache` (page fixe et surcouche sur la
  même page ; `SetGridCommand` qui cache la grille du dessus, `applyToCache` sur la page fixe : son compteur de
  reconstructions — celui des tests du lot 10 — n'a pas bougé ; `rebuildCount` de la surcouche + 1) ;
  `sharpOnePixelLines` (zoom 2 : une ligne de grille fait exactement 1 pixel d'écran, aucun pixel intermédiaire, comme
  `BORD grille-nette` du lot 10) ; `gridAboveCoversAPlacedShape` (amendement 5). Dans `tst_render_pendinglayer.cpp` : `strokeOnGridAbovePageIsDeferred` (page à
  grille du dessus, différé actif, trait inséré en fin de page : `pendingCount() == 1`, aucun redessin).
- [ ] **Étape 2 : surcouche.** `updatePaintNode` : nœuds de lignes de 1 px d'écran (épaisseur `1 / zoom` en
  coordonnées de page, ou géométrie en pixels d'écran sous un nœud de transformation), sans lissage, seulement si
  `gridDrawsAboveObjects` ; réutiliser `createGridNode` (`render/include/actcraft/render/paintedrenderer.h:49`) s'il
  dessine déjà au pixel, sinon une fonction locale ; `rebuildCount` + 1 à chaque reconstruction.
- [ ] **Étape 3 : page fixe.** Retirer le dessin de la grille du dessus et `m_gridDrawnAbove` ; la grille du dessous
  reste ; condition de différé sans `drawnAbove`.
- [ ] **Étape 4 : vert**, banc sans écran, preuve de tâche.
- [ ] **Étape 5 : commit.**
  ```powershell
  git -C <wt> add render/include/actcraft/render/gridoverlayitem.h render/src/gridoverlayitem.cpp render/include/actcraft/render/fixedpageitem.h render/src/fixedpageitem.cpp ui/qml/PageArea.qml tests/tst_render_gridoverlay.cpp tests/tst_render_pendinglayer.cpp tests/tst_render_fixedpage.cpp
  git -C <wt> commit -m "render : grille du dessus en surcouche hors du cache, trait différé sur page à grille du dessus (L4-T7)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t8"></a>
### Tâche L4-T8 — `tools` : outil des formes

**Amendement 5 (D5, L4-O2).** Prime sur les amendements 1 et 4 et sur le reste de la section. Dans l'original, Maj
n'a **aucun effet** sur le tracé (contredit avec un témoin : Maj injectée est reçue pendant un geste de souris) : ni
carré, ni 45°, que Maj soit tenue avant l'appui ou enfoncée au milieu du glisser.
- `onKey` ne change rien et rend `false` pour toute touche ; le membre privé `m_shift` est retiré ; la signature
  publique de L4-T1 reste. Commentaire de `onKey` dans `shapetool.h` remplacé (commentaire seul) par :
  `// Maj et Ctrl relayés par ToolHost (lot 3), et le relais d'avant appui : sans effet sur le tracé (D5, L4-O2).` puis
  `// Échap n'arrive pas ici : raccourci de tool.select, qui appelle cancel() si l'outil est occupé.`
- Boîte : `core::dragBox(press, current, fromCenter, preset.square)` — le carré et le cercle restent carrés par leur
  préréglage (P4). Ligne : `core::dragLine(press, current, preset, fromCenter, false)` — `constrain45` toujours faux
  (paramètre de L4-T3 gardé, inutilisé par l'outil) ; les lignes horizontale et verticale gardent leur axe (P4).
- Tests : `shiftKeepsSquare`, `shiftReleaseFreesTheRatio` et `relayedModifiersKeepShiftState` (qui attendait lui
  aussi un carré) sont **retirés** ; **`shiftLeavesTheShapeUnchanged`** les remplace (code à l'étape 1). Neuf
  fonctions : `Totals: 11 passed`.

**Amendement 4 (V7, V9, D1, D5).** Prime sur l'amendement 1 et sur le reste de la section.
- **Touches (V7).** `ToolHost` relaie déjà (lot 3, `ui/src/toolhost.cpp:586-635`) `onKey(Qt::Key_Shift, m)` et
  `onKey(Qt::Key_Control, m)` à l'appui et au relâché, et `onKey(Qt::Key_unknown, m)` avant chaque appui de pointeur,
  `m` étant les modificateurs tenus (masqués par `kRelayedModifiers`). L'état de Maj se lit donc sur
  `modifiers & Qt::ShiftModifier` **quelle que soit la touche reçue** ; pendant un tracé, tout changement de cet état
  refait l'aperçu ; `onKey` rend `true` pour `Qt::Key_Shift` seulement. **Échap n'atteint jamais `onKey`** : c'est le
  raccourci de `tool.select`, dont le gestionnaire appelle `cancelGesture()` — donc `cancel()` de l'outil s'il est
  occupé — puis active Sélectionner (`ui/qml/Main.qml:298-302`, `ui/src/toolhost.cpp:370-378`). Pendant une boîte,
  Échap annule donc par `cancel()` (aperçu effacé, rien de posé : `cancelClearsThePreview`) ; le commentaire de `onKey`
  dans `shapetool.h` est mis à jour (commentaire seul). Le test `relayedModifiersKeepShiftState` est retiré par
  l'amendement 5 (il attendait un carré) ; `shiftLeavesTheShapeUnchanged` couvre le relais de Ctrl et d'avant appui.
- **Formes sans boîte (D1).** Si `core::shapeGesture(preset) != core::ShapeGesture::Box` (L4-T3) — `freeform`,
  `polyline`, `curve-chain` —, le préréglage ne passe ni par `dragBox` ni par `dragLine` : L4-T8 ne montre rien et
  n'exécute rien pour lui ; son geste est celui de L4-T31. Test `chainPresetsExecuteNothingOnRelease`, qui reste vrai
  après L4-T31 (une ligne brisée n'est posée qu'au double-clic).
- **Coût de l'aperçu (V9).** `LiftLayerItem::setObjects` reconstruit ses nœuds à chaque `lift.show`
  (`render/include/actcraft/render/liftlayeritem.h:31`, `:50`) : aucun scénario de banc ne trace encore de forme, le
  banc sans écran de cette tâche est une non-régression ; la mesure du coût pendant un tracé est portée par
  `forme-page-annotee` (L4-T14). Au-delà du budget : H4-8.
- **Maj (D5)** : remplacé par l'amendement 5 (Maj sans effet) ; l'ancien dépôt gardait le carré et les 45°.
- **Remplissage (D2)** : les tests ci-dessous passent des réglages explicites et ne dépendent pas du défaut `#FFAA00`.

**Amendement 1 (défaut 10).** `Tool::onKey` ne reçoit rien de la vue (M9) et `input::Sample` n'a pas de
modificateurs : Maj arrive par `ToolHost` (L4-T11), `onKey(Qt::Key_Shift, Qt::ShiftModifier)` à l'appui,
`onKey(Qt::Key_Shift, Qt::NoModifier)` au relâché. `onKey` met à jour l'état de Maj et, pendant un tracé, refait
l'aperçu au dernier point (`lift.show`) ; il rend `true` pour Maj et Échap. Test de plus :
`shiftReleaseFreesTheRatio` (appui, Maj enfoncée, mouvement vers (40, 10) : carré ; Maj relâchée sans nouveau
point : l'aperçu montré redevient un rectangle 40 × 10 ; relâché : la forme posée est le rectangle).

**Objectif.** Remplir `ShapeTool` (en-tête de L4-T1) : à l'appui, point accroché (`core::nearestGridPoint` si la
grille de la page accroche) ; au mouvement, aperçu par `ToolContext::lift.show({objet})` — **aucune** commande
pendant le geste ; au relâché, si l'écart dépasse `kShapeMinDragScreen / view.zoom()`, `lift.clear()` puis
`document.execute(std::make_unique<core::AddObjectCommand>(document.pageIndex(), objet))`, sinon `lift.clear()`
seul (F-241, P1). Réglages : `user.shapeSettings()` (préréglage, remplissage, centre) et
`user.settings("shape")` (couleur et épaisseur du contour). Maj : sans effet (amendement 5, D5). Échap : annule
(par `cancel()`, amendement 4).

**Fichiers.** `tools/include/actcraft/tools/shapetool.h`, `tools/src/shapetool.cpp`, `tests/tst_tools_shape.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_tools_shape` ; fonctions `dragPreviewsWithoutExecuting`,
`releaseExecutesOneAdd`, `singleClickAddsNothing`, `fromCenterSetting`, `shiftLeavesTheShapeUnchanged`,
`snapsPressAndRelease`, `cancelClearsThePreview`, `unfilledChoiceHonoured`, `chainPresetsExecuteNothingOnRelease` en
`PASS` (`Totals: 11 passed` : neuf fonctions + 2, amendement 5) ; banc sans écran ; preuve de tâche.

**Interdits.** Aucun en-tête de `render/` ni de `ui/` dans `tools/`. Aucune écriture dans le document hors
`execute`. Pas de géométrie recopiée (appeler `shapegeometry`).

**Terminé quand.** Fonctions vertes ; `TESTS OK` ; banc joint.

**Étapes.**
- [ ] **Étape 1 : test rouge.** `tests/tst_tools_shape.cpp`, avec des faux de chaque interface du contexte :
  ```cpp
  #include <actcraft/core/document.h>
  #include <actcraft/core/edits.h>
  #include <actcraft/tools/shapetool.h>
  #include <actcraft/tools/userstate.h>

  #include <QtTest>

  #include <cstdint>
  #include <vector>

  using namespace actcraft;

  namespace {
  struct FakeDocument final : tools::DocumentEditor
  {
      core::Document document;
      int executed = 0;
      const core::Page& page() const override { return document.page(0); }
      std::size_t pageIndex() const override { return 0; }
      void execute(std::unique_ptr<core::UndoCommand> command) override
      {
          core::Changes changes;
          command->redo(document, changes);
          ++executed;
      }
      std::vector<core::PageObject> lift(const std::vector<core::ObjectId>&) override { return {}; }
      void drop(std::unique_ptr<core::UndoCommand>) override {}
      bool isLifting() const override { return false; }
  };
  struct FakeLift final : tools::LiftSurface
  {
      std::vector<core::PageObject> shown;
      void show(std::vector<core::PageObject> objects) override { shown = std::move(objects); }
      void setTransform(const core::Transform&) override {}
      void clear() override { shown.clear(); }
  };
  // FakeInk, FakeStrokes, FakeSelection, FakeView (zoom 1, pas de corbeille), FakeCommands : méthodes vides, sur le
  // modèle de tests/tst_tools_select.cpp (lot 3) ; un `Harness` les possède et construit le ToolContext.
  input::Sample at(double x, double y, input::Phase phase)
  {
      input::Sample s;
      s.x = x;
      s.y = y;
      s.phase = phase;
      s.device = input::DeviceKind::Mouse;
      return s;
  }
  } // namespace

  class TstToolsShape : public QObject
  {
      Q_OBJECT

  private slots:
      void dragPreviewsWithoutExecuting()
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("rectangle"), true, 0xFFFFFFFFu, false});
          tools::ShapeTool tool(h.context);
          tool.onSample(at(100, 100, input::Phase::Press));
          tool.onSample(at(200, 160, input::Phase::Move));
          QCOMPARE(h.lift.shown.size(), std::size_t{1});
          QCOMPARE(h.doc.executed, 0);
          QVERIFY(tool.isBusy());
      }
      void releaseExecutesOneAdd()
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("rectangle"), true, 0xFFFFFFFFu, false});
          tools::ShapeTool tool(h.context);
          tool.onSample(at(100, 100, input::Phase::Press));
          tool.onSample(at(200, 160, input::Phase::Move));
          tool.onSample(at(200, 160, input::Phase::Release));
          QCOMPARE(h.doc.executed, 1);
          QVERIFY(h.lift.shown.empty());
          const auto& o = h.doc.document.page(0).objects.back();
          QCOMPARE(o.common.layer, core::Layer::Middle);
          QCOMPARE(QString::fromStdString(std::get<core::Shape>(core::body(o)).preset), QStringLiteral("rectangle"));
      }
      void singleClickAddsNothing()
      {
          Harness h;
          tools::ShapeTool tool(h.context);
          tool.onSample(at(100, 100, input::Phase::Press));
          tool.onSample(at(101, 102, input::Phase::Release));
          QCOMPARE(h.doc.executed, 0);
      }
      void fromCenterSetting()
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("rectangle"), true, 0xFFFFFFFFu, true});
          tools::ShapeTool tool(h.context);
          tool.onSample(at(100, 100, input::Phase::Press));
          tool.onSample(at(150, 130, input::Phase::Release));
          const auto& o = h.doc.document.page(0).objects.back();
          QCOMPARE(o.common.transform.dx, 50.0);
          QCOMPARE(o.common.transform.dy, 70.0);
      }
      // Amendement 5 (D5 contredit avec témoin, L4-O2) : Maj, tenue avant l'appui ou enfoncée au milieu du glisser,
      // ne change ni l'aperçu ni la forme posée (ni carré, ni 45°) ; le relais de Ctrl et d'avant appui non plus.
      void shiftLeavesTheShapeUnchanged()
      {
          const auto previewPrint = [](const std::vector<core::PageObject>& shown) {
              core::Page page;
              page.objects = shown;
              return core::fingerprint(page);
          };
          for (const char* preset : {"rectangle", "line"}) {
              std::uint64_t shownWithout = 0; // témoin sans Maj
              std::uint64_t placedWithout = 0;
              // 0 : sans Maj ; 1 : Maj tenue avant l'appui ; 2 : Maj enfoncée au milieu du glisser.
              for (int variant = 0; variant < 3; ++variant) {
                  Harness h;
                  h.user.setShapeSettings({QString::fromLatin1(preset), true, 0xFFFFFFFFu, false});
                  tools::ShapeTool tool(h.context);
                  if (variant == 1) {
                      QVERIFY(!tool.onKey(Qt::Key_Shift, Qt::ShiftModifier));
                      tool.onKey(Qt::Key_unknown, Qt::ShiftModifier); // relais d'avant appui (ToolHost)
                  }
                  tool.onSample(at(0, 0, input::Phase::Press));
                  tool.onSample(at(50, 35, input::Phase::Move));
                  if (variant == 2) {
                      QVERIFY(!tool.onKey(Qt::Key_Shift, Qt::ShiftModifier));
                      tool.onKey(Qt::Key_Control, (Qt::ShiftModifier | Qt::ControlModifier).toInt());
                  }
                  tool.onSample(at(100, 70, input::Phase::Move)); // ligne à 35°, rectangle de 100 × 70
                  QCOMPARE(h.lift.shown.size(), std::size_t{1});
                  const std::uint64_t shown = previewPrint(h.lift.shown);
                  tool.onSample(at(100, 70, input::Phase::Release));
                  QCOMPARE(h.doc.executed, 1);
                  const std::uint64_t placed = core::fingerprint(h.doc.document.page(0));
                  if (variant == 0) {
                      shownWithout = shown;
                      placedWithout = placed;
                      continue;
                  }
                  QCOMPARE(shown, shownWithout);
                  QCOMPARE(placed, placedWithout);
                  if (QString::fromLatin1(preset) == QStringLiteral("rectangle")) {
                      const auto& s = std::get<core::Shape>(core::body(h.doc.document.page(0).objects.back()));
                      QCOMPARE(s.bounds.width, 100.0); // pas un carré
                      QCOMPARE(s.bounds.height, 70.0);
                  }
              }
          }
      }
      void snapsPressAndRelease()
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("rectangle"), true, 0xFFFFFFFFu, false});
          auto& grid = h.doc.document.page(0).grid;
          grid.snapEnabled = true;
          core::GridLevel level;
          level.spacingX = level.spacingY = 50;
          grid.levels.push_back(level);
          tools::ShapeTool tool(h.context);
          tool.onSample(at(48, 52, input::Phase::Press));
          tool.onSample(at(147, 103, input::Phase::Release));
          const auto& o = h.doc.document.page(0).objects.back();
          QCOMPARE(o.common.transform.dx, 50.0);
          QCOMPARE(std::get<core::Shape>(core::body(o)).bounds.width, 100.0);
      }
      void cancelClearsThePreview()
      {
          Harness h;
          tools::ShapeTool tool(h.context);
          tool.onSample(at(0, 0, input::Phase::Press));
          tool.onSample(at(100, 100, input::Phase::Move));
          tool.cancel();
          QVERIFY(h.lift.shown.empty());
          QVERIFY(!tool.isBusy());
          QCOMPARE(h.doc.executed, 0);
      }
      void unfilledChoiceHonoured()
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("rectangle"), false, 0xFFFFFFFFu, false});
          tools::ShapeTool tool(h.context);
          tool.onSample(at(0, 0, input::Phase::Press));
          tool.onSample(at(100, 100, input::Phase::Release));
          QCOMPARE(std::get<core::Shape>(core::body(h.doc.document.page(0).objects.back())).fill >> 24, 0u);
      }
      void chainPresetsExecuteNothingOnRelease() // amendement 4 (D1) : leur geste est celui de L4-T31
      {
          for (const char* preset : {"polyline", "curve-chain"}) {
              Harness h;
              h.user.setShapeSettings({QString::fromLatin1(preset), true, 0xFFFFFFFFu, false});
              tools::ShapeTool tool(h.context);
              tool.onSample(at(100, 100, input::Phase::Press));
              tool.onSample(at(200, 160, input::Phase::Move));
              tool.onSample(at(200, 160, input::Phase::Release));
              QCOMPARE(h.doc.executed, 0);
          }
      }
  };

  QTEST_GUILESS_MAIN(TstToolsShape)
  #include "tst_tools_shape.moc"
  ```
  (Le `Harness` et les faux sont écrits en entier dans le fichier, d'après les signatures réelles de
  `toolcontext.h` ; `id()` rend « shape ».)
- [ ] **Étape 2 : implémenter** `shapetool.cpp` : `objetCourant()` construit par `makeShapeObject` /
  `makeLineObject` (famille du préréglage) depuis `dragBox` / `dragLine` ; accrochage de l'appui et du point courant
  par `nearestGridPoint(page.grid, p, page.width, page.height)` ; préréglage dont `shapeGesture` n'est pas `Box` :
  aucun aperçu, aucune commande (amendement 4) ; `onKey` sans effet, rend `false` (amendement 5) ; `dragBox` reçoit
  `preset.square`, `dragLine` reçoit `constrain45 = false`.
- [ ] **Étape 3 : vert**, banc sans écran, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add tools/include/actcraft/tools/shapetool.h tools/src/shapetool.cpp tests/tst_tools_shape.cpp
  git -C <wt> commit -m "tools : outil des formes, aperçu sans redessin du cache, une étape au relâché (L4-T8)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t9"></a>
### Tâche L4-T9 — `tools` : sélection — accrochage au dépôt

**Amendement 4 (V7, D6).** Point de branchement : la translation du glissement est calculée par
`SelectTool::gestureTransform` (`tools/src/selecttool.cpp:485`, `core::translation(point.x - m_start.x, …)`) et
déposée dans `SelectTool::endDrag` (`:509-531`). L'accrochage s'applique **dans `endDrag`**, à la translation issue de
`:485`, juste avant la commande ; jamais dans `gestureTransform`, qui sert aussi pendant le geste (décision 4 du plan :
accrochage au dépôt). Test de plus `noSnapDuringTheDrag` (même montage que `dropSnapsBottomLeftCorner` : pendant les
mouvements, la transformation montrée par la couche soulevée — `setTransform` du faux `LiftSurface` — est la
translation brute (30, 0) ; seule la commande déposée porte (38, 8)). D6 : un texte s'aimante par sa ligne de base ;
l'accrochage d'une grille invisible (F-622) est gardé, observé par L4-O1.

**Objectif.** Au dépôt d'un glissement de la sélection (lot 3, L3-T8), la translation finale passe par
`core::snapDelta(page, sélection, delta)` avant la commande `TransformObjectsCommand` (F-622) ; pendant le geste,
la couche soulevée suit le pointeur sans accrochage (comme la référence, qui aligne « au relâchement »). Coût cache
inchangé : un redessin au soulèvement, un au dépôt.

**Fichiers.** `tools/src/selecttool.cpp`, `tools/include/actcraft/tools/selectiongeometry.h`,
`tools/src/selectiongeometry.cpp`, `tests/tst_tools_select.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_tools_select` ; fonctions `dropSnapsBottomLeftCorner`,
`noSnapWithoutGridSnap`, `snapDragRefreshesTheCacheTwice`, `noSnapDuringTheDrag` en `PASS` et toutes celles du lot 3
vertes ; banc sans écran ; preuve de tâche.

**Interdits.** Ne pas changer une signature publique du lot 3. Ne pas modifier un test du lot 3.

**Terminé quand.** Fonctions vertes, lot 3 intact, `TESTS OK`, banc joint.

**Étapes.**
- [ ] **Étape 1 : tests rouges** dans `tests/tst_tools_select.cpp` (faux du fichier) : `dropSnapsBottomLeftCorner`
  (rectangle 40 × 30 en (12, 12), grille de 50 qui accroche, glissé de (30, 0) : la commande déposée a pour
  translation (38, 8)) ; `noSnapWithoutGridSnap` (même geste, `snapEnabled` faux : (30, 0)) ;
  `snapDragRefreshesTheCacheTwice` (faux `DocumentEditor` qui compte `lift` et `drop` : un `lift`, un `drop`,
  aucun `execute` pendant dix mouvements).
- [ ] **Étape 2 : implémenter.** Dans `SelectTool::endDrag` (`:509-531`), sur la translation issue de
  `gestureTransform` (`:485`) : `delta = core::snapDelta(page, ids, delta)` (fonction d'aide dans `selectiongeometry`
  si la géométrie y est déjà) ; `gestureTransform` inchangé (amendement 4).
- [ ] **Étape 3 : vert**, banc sans écran, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add tools/src/selecttool.cpp tools/include/actcraft/tools/selectiongeometry.h tools/src/selectiongeometry.cpp tests/tst_tools_select.cpp
  git -C <wt> commit -m "tools : la sélection déposée s'accroche à la grille (L4-T9)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t13"></a>
### Tâche L4-T13 — Réglages de grille : contrôleur, créateur, scénario `grille`

**Amendement 4 (V7).** `beginDesigner()` appelle `DocumentController::closeMerge()`
(`ui/include/actcraft/ui/documentcontroller.h:88`) **avant** le premier `setGrid` de la séance, et `endDesigner()`
l'appelle **après** le dernier : un réglage fait juste avant l'ouverture ne fusionne pas avec la séance, et deux
séances font deux étapes (`kMergeGrid = 4`, libre, contrat de L4-T4). Test de plus `twoDesignerSessionsMakeTwoSteps`
(`beginDesigner`, deux `setGrid`, `endDesigner`, `beginDesigner`, un `setGrid`, `endDesigner` : un `undo()` rend la
grille de la fin de la première séance, un second la grille d'origine).

**Objectif.** Remplir `PageOptions` pour la grille (propriétés `gridVisible`, `gridSnap`, `gridAbove`,
`grid` ; `toggleGridHidden`, `toggleSnap`, `beginDesigner`, `setGrid`, `addLevel`, `removeLevel`, `resetGrid`,
`stepSpacing`, `endDesigner`), chaque réglage par `core::SetGridCommand` sur la page affichée (`merge` vrai entre
`beginDesigner` et `endDesigner`) ; `changed` émis sur `pageContentChanged` de la page affichée et sur
`pagesChanged`. Remplir `GridDesigner.qml` (fenêtre à nous : cadre Grille, cadre des niveaux, niveau courant,
aperçu immédiat, F-620) et `bench/scenarios/grille.json` (S7.2 sans la partie « rectangle déplacé », jouée par S7).

**Fichiers.** `ui/include/actcraft/ui/pageoptions.h`, `ui/src/pageoptions.cpp`,
`tests/tst_ui_pagesettings.cpp`, `ui/qml/GridDesigner.qml`, `bench/scenarios/grille.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_pagesettings|tst_captures_grille'` ; fonctions
`hideGridAboveRefreshesNoCache`, `hideGridBelowRefreshesOnce`, `snapToggleFollowsThePage`,
`addRemoveResetLevels`, `designerSessionMakesASingleStep`, `twoDesignerSessionsMakeTwoSteps` en `PASS` ; preuve
visuelle hors écran (ci-dessous) ;
banc sans écran ; preuve de tâche.

**Preuve visuelle.** `captures.ps1 -Scenario grille -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire
"appPageSettings.gridVisible=false"` : lignes `LU appPageSettings.gridVisible=false OK`,
`LU pageView.cacheRefreshCount=${avant} OK` (après « Cacher la grille » sur la grille du dessus), `CAPTURES OK` ;
décrire `grille-du-dessus.png` (lignes nettes au-dessus d'un rectangle) et `grille-cachee.png`.

**Interdits.** Aucun libellé de l'original dans la boîte. Aucune écriture du document hors commandes.

**Terminé quand.** Fonctions vertes ; scénario `CAPTURES OK` avec les deux lectures ; `TESTS OK` ; banc joint.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_ui_pagesettings.cpp` (contrôleur réel, `DocumentController` réel,
  espion de cache par `addChangeObserver` et `core::applyToCache`, D-S7) : `hideGridAboveRefreshesNoCache` (page à grille du dessus
  visible, `toggleGridHidden()` : l'espion n'a reçu aucun `invalidate`, `gridVisible` faux) ;
  `hideGridBelowRefreshesOnce` (grille du dessous : un `invalidate` de toute la page) ; `snapToggleFollowsThePage`
  (`toggleSnap()` sur la page 1, `showPage(1)` : `gridSnap` relit la page 2) ; `addRemoveResetLevels` (deux niveaux
  après `addLevel`, un après `removeLevel(1)`, `resetGrid()` : un niveau, `snap` faux) ;
  `designerSessionMakesASingleStep` (`beginDesigner`, trois `setGrid`, `endDesigner`, un `undo()` : grille d'origine) ;
  `twoDesignerSessionsMakeTwoSteps` (amendement 4).
- [ ] **Étape 2 : implémenter** le contrôleur ; conversion `QVariantMap` ↔ `core::Grid` à nos clés (contrat de
  L4-T1).
- [ ] **Étape 3 : boîte.** `GridDesigner.qml` : `function open()` (montre la boîte, `appPageSettings.beginDesigner()`),
  fermeture → `endDesigner()` ; champs liés à `appPageSettings.grid` ; chaque changement appelle `setGrid` ; objets
  nommés `gridDesigner`, `gridSpacingX`, `gridAboveCheck`, `gridSnapCheck`.
- [ ] **Étape 4 : scénario** `bench/scenarios/grille.json` :
  ```json
  {
    "format": "actcraft-scenario/1",
    "name": "grille",
    "description": "S7.2 : créateur de grille, grille du dessus cachée sans redessin du cache, grille du dessous cachée avec un redessin.",
    "steps": [
      {"waitReady": true},
      {"command": "grid.designer"},
      {"expect": "gridDesigner.visible", "value": true},
      {"command": "grid.snap"},
      {"expect": "appPageSettings.gridSnap", "value": true},
      {"capture": "createur"},
      {"key": "Escape"},
      {"waitReady": true},
      {"remember": "pageView.cacheRefreshCount", "as": "avant"},
      {"command": "grid.hide"},
      {"waitReady": true},
      {"expect": "appPageSettings.gridVisible", "value": false},
      {"expect": "pageView.cacheRefreshCount", "value": "${avant}"},
      {"capture": "grille-cachee"}
    ]
  }
  ```
  (La fixture de départ — page à grille du dessus visible — se pose par une étape `fixture` existante du joueur si
  elle offre une page à grille, sinon par la commande `grid.designer` et la case « au-dessus » cochée par `pointer`
  (A11) ; le choix est écrit dans le rapport.)
- [ ] **Étape 5 : vert**, preuve visuelle, banc sans écran, preuve de tâche.
- [ ] **Étape 6 : commit.**
  ```powershell
  git -C <wt> add ui/include/actcraft/ui/pageoptions.h ui/src/pageoptions.cpp tests/tst_ui_pagesettings.cpp ui/qml/GridDesigner.qml bench/scenarios/grille.json
  git -C <wt> commit -m "ui : réglages de grille et créateur à niveaux, grille du dessus cachée sans redessin du cache (L4-T13)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t10"></a>
### Tâche L4-T10 — `tools` : sélection — points intelligents

**Amendement 4 (D4).** Seules trois formes ont des points intelligents : `rounded-rectangle` (un point : rayon),
`arrow` (deux : début de la tête, épaisseur du corps), `callout` (deux : pointe, arrondi), bornés par le catalogue de
L4-T3. Les étoiles et les explosions n'en ont aucun. Test de plus `noSmartPointForStars` (`star` puis `burst`
sélectionnés seuls : `smartPoints` vide, et un appui à l'endroit où une version antérieure aurait mis un point
commence un déplacement ordinaire).

**Objectif.** Une forme seule sélectionnée qui a des points intelligents (`core::smartPoints`) les montre
(`SelectionView::smartPoints`, coordonnées de page) ; un appui sur l'un d'eux (tolérance
`tools::kHandleRadiusScreen / zoom`) **soulève** la forme, l'aperçu suit (copie reconstruite montrée par
`lift.show`), et le relâché dépose avec **une** `core::SetShapeParametersCommand`. Les points se dessinent dans
`SelectionOverlay.qml` (losanges jaunes à contour sombre, à nous).

**Fichiers.** `tools/src/selecttool.cpp`, `tools/include/actcraft/tools/selectiongeometry.h`,
`tools/src/selectiongeometry.cpp`, `tests/tst_tools_select.cpp`, `ui/qml/SelectionOverlay.qml`.

**Test et commande bornée.** Commande ciblée `-R tst_tools_select` ; fonctions `smartPointsShownForArrow`,
`smartPointDragLiftsOnceAndDropsOneStep`, `noSmartPointForReadShapes`, `noSmartPointForStars` en `PASS`, lot 3 et
L4-T9 verts ; banc sans
écran ; preuve de tâche.

**Interdits.** Comme L4-T9. Les poignées du lot 3 gardent la priorité si elles se recouvrent (ordre de test :
point intelligent seulement hors des huit poignées).

**Terminé quand.** Fonctions vertes, `TESTS OK`, banc joint.

**Étapes.**
- [ ] **Étape 1 : tests rouges** : `smartPointsShownForArrow` (flèche sélectionnée seule : `smartPoints` non vide
  dans la vue montrée ; deux objets sélectionnés : vide) ; `smartPointDragLiftsOnceAndDropsOneStep` (un `lift`, un
  `drop` portant une `SetShapeParametersCommand`, paramètres changés) ; `noSmartPointForReadShapes` (forme sans
  préréglage : vide) ; `noSmartPointForStars` (amendement 4).
- [ ] **Étape 2 : implémenter** (désignation du point, état de geste « point intelligent », aperçu, dépôt).
- [ ] **Étape 3 : QML.** `SelectionOverlay.qml` : `Repeater` sur `appTools.selectionFrame.smartPoints` (rempli par
  L4-T11 ; tableau vide d'ici là), placés par `pageView.pageToScene`.
- [ ] **Étape 4 : vert**, banc sans écran, preuve de tâche.
- [ ] **Étape 5 : commit.**
  ```powershell
  git -C <wt> add tools/src/selecttool.cpp tools/include/actcraft/tools/selectiongeometry.h tools/src/selectiongeometry.cpp tests/tst_tools_select.cpp ui/qml/SelectionOverlay.qml
  git -C <wt> commit -m "tools : points intelligents des formes sélectionnées, une étape au dépôt (L4-T10)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t11"></a>
### Tâche L4-T11 — `ui::ToolHost` : outil des formes branché

**Amendement 5 (D5, L4-O2).** Prime sur les amendements 1 et 4. Le relais de Maj du lot 3 reste tel quel ; Maj n'a
aucun effet sur la forme. `shiftReachesTheShapeTool` est **adapté** (même nom) : outil `shape` actif par
`chooseShape("rectangle")` ; appui en (100, 100) puis mouvement vers (200, 170) injectés par
`activeTool()->onSample` ; `QTest::keyPress(<fenêtre de la vue>, Qt::Key_Shift, Qt::ShiftModifier)` (le relais du
lot 3 transmet Maj à l'outil actif) ; `QVERIFY(activeTool()->isBusy())` (Maj n'interrompt pas le tracé) ; nouveau
mouvement vers (200, 170) ; `QTest::keyRelease(<fenêtre>, Qt::Key_Shift, Qt::NoModifier)` ; relâché en (200, 170) :
une forme posée, `bounds.width == 100` et `bounds.height == 70` (pas un carré). Que Maj atteigne bien l'outil actif
est prouvé par le lot 3 (`shiftAndControlReachTheActiveTool`, `tests/tst_ui_tools.cpp:423`) ; ce test prouve qu'elle
ne dérange pas l'outil des formes. L'attendu de l'amendement 1 (« l'aperçu est carré ; relâché de Maj : il ne l'est
plus ») tombe.

**Amendement 4 (V7, D2, D3, D4).** Prime sur l'amendement 1.
- **Relais de Maj déjà livré (V7)** : `ToolHost` filtre déjà les touches de la fenêtre de la vue et relaie Maj, Ctrl
  et `Qt::Key_unknown` à l'outil actif (`ui/src/toolhost.cpp:157-162`, `:586-635`). Ne pas le réécrire ni poser un
  second filtre : la partie « relais de Maj » de l'amendement 1 tombe. `shiftReachesTheShapeTool` reste, tel quel ;
  il prouve le relais existant pour l'outil `shape`.
- **Style d'une forme neuve (D2, D3)** : remplie, `#FFAA00` par défaut, contour noir de 4, réglage propre à l'outil
  des formes. Si le rapport de L4-T1 dit que `ShapeSettings::fill` vaut encore `0xFFFFFFFFu`, cette tâche le pose à
  `0xFFFFAA00u` dans `tools/include/actcraft/tools/userstate.h` ; si `tools::defaultSettings` ne rend pas, pour
  `shape`, un contour `#000000` de 4, elle le corrige dans `tools/src/userstate.cpp` (deux fichiers ajoutés à sa ligne,
  seulement dans ces cas). Mémoire : `tools/shape/fill` vaut `#FFAA00` par défaut. Test de plus
  `newShapeIsOrangeWithBlackOutline` (fichier de réglages vide, `chooseShape("rectangle")`, tracé injecté : la forme
  posée a `fill == 0xFFFFAA00u`, `outline == 0xFF000000u`, `outlineWidth == 4`).
- **Points intelligents (D4)** : `smartPointsInSelectionFrame` vérifie `selectionFrame["smartPoints"]` non vide pour
  `rounded-rectangle`, `arrow` et `callout` sélectionnés seuls, vide pour `star`.
- `outlineOfEveryPreset` couvre les 22 préréglages de D1 (`freeform`, `polyline`, `curve-chain` : segment de
  remplacement de L4-T3, deux points dans [0, 1]).

**Amendement 1 (défauts 6 et 10).** Espion de cache : `DocumentController::addChangeObserver` puis
`core::applyToCache(changes, controller.currentIndex(), spy)` (D-S7) ; plus de `setCacheObserver`. Relais de Maj :
`ToolHost` pose un filtre d'événements sur `QGuiApplication` (API publique) tant que l'outil actif est `shape` ; un
`QKeyEvent` `KeyPress` ou `KeyRelease` de `Qt::Key_Shift` (sans répétition automatique) est transmis à
`activeTool()->onKey(Qt::Key_Shift, Qt::ShiftModifier)` ou `(Qt::Key_Shift, Qt::NoModifier)` ; l'événement n'est pas
consommé. Test de plus dans `tst_ui_shapes` : `shiftReachesTheShapeTool` (outil `shape` actif, appui injecté par
`onSample`, `QKeyEvent` Maj envoyé par `QCoreApplication::sendEvent` à la fenêtre : l'aperçu est carré ; relâché de
Maj : il ne l'est plus).

**Objectif.** Remplir les ajouts de L4-T1 à `ToolHost` : `chooseShape` (préréglage connu seulement, active
« shape », commande `tool.shape` cochée), `setShapeFilled`, `setShapeFillColor`, `setShapeFromCenter`, propriétés
`shapePreset`, `shapeFilled`, `shapeFromCenter`, `shapePresets` (ordre de `core::shapeCatalog`), `shapeOutline`
(contour normalisé par `makeShapeObject` / `makeLineObject` dans une boîte 1 × 1) ; mémoire des réglages de forme
dans le même `QSettings` que la mémoire par outil du lot 3 (clés `tools/shape/preset`, `tools/shape/filled`,
`tools/shape/fill`, `tools/shape/fromCenter`) ; `selectionFrame` ajoute la clé `smartPoints` (`[{x, y}]`, page).

**Fichiers.** `ui/src/toolhost.cpp`, `ui/src/tooladapters.h`, `ui/src/tooladapters.cpp`, `tests/tst_ui_shapes.cpp` ;
`tools/include/actcraft/tools/userstate.h` et `tools/src/userstate.cpp` seulement dans les cas de l'amendement 4.

**Test et commande bornée.** Commande ciblée `-R tst_ui_shapes` ; fonctions `shapeToolActivatesWithPalette`,
`presetChoiceSurvivesRestart`, `outlineOfEveryPreset`, `smartPointsInSelectionFrame`,
`shapeGoesThroughTheCacheInterface`, `shiftReachesTheShapeTool`, `newShapeIsOrangeWithBlackOutline` en `PASS` ; banc
sans écran ; preuve de tâche.

**Interdits.** Aucun outil n'appelle la page fixe ni les couches : `shapeGoesThroughTheCacheInterface` le prouve
(le cache ne reçoit qu'un `insert` au relâché, aucun appel pendant le tracé).

**Terminé quand.** Fonctions vertes, `TESTS OK`, banc joint.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_ui_shapes.cpp` (montage de `tst_ui_tools.cpp` du lot 3 : `ToolHost` avec
  contrôleur, vue et couche soulevée réels, fichier de réglages temporaire) : `shapeToolActivatesWithPalette`
  (`chooseShape("circle")` : `currentTool == "shape"`, `shapePreset == "circle"`) ; `presetChoiceSurvivesRestart`
  (second `ToolHost` sur le même fichier : `circle`, `shapeFromCenter` gardé) ; `outlineOfEveryPreset` (chaque
  préréglage : au moins 2 points, tous dans [0, 1]) ; `smartPointsInSelectionFrame` (flèche sélectionnée :
  `selectionFrame["smartPoints"]` non vide) ; `shapeGoesThroughTheCacheInterface` (tracé injecté par
  `activeTool()->onSample` : espion par `addChangeObserver` et `core::applyToCache` — zéro appel avant le relâché,
  un `insert` après) ; `shiftReachesTheShapeTool` (amendement 5 : Maj relayée sans effet) ; `newShapeIsOrangeWithBlackOutline`
  (amendement 4).
- [ ] **Étape 2 : implémenter.**
- [ ] **Étape 3 : vert**, banc sans écran, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add ui/src/toolhost.cpp ui/src/tooladapters.h ui/src/tooladapters.cpp tests/tst_ui_shapes.cpp
  git -C <wt> add tools/include/actcraft/tools/userstate.h tools/src/userstate.cpp   # seulement si modifiés (D2, D3)
  git -C <wt> commit -m "ui : outil des formes branché, réglages gardés, contours de la palette (L4-T11)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t12"></a>
### Tâche L4-T12 — QML : palette des formes, curseur ; scénario `formes`

**Objectif.** Remplir `ShapePalette.qml` (panneau vertical ancré à droite, visible quand l'outil courant est
« shape » : nuancier de contour 2 × 4 tiré des couleurs de la disposition, case « sans remplissage », bascule
« depuis le centre », grille des 22 formes dessinées par `Shape`/`PathPolyline` depuis `appTools.shapeOutline`,
forme courante encadrée ; objet `shapePalette`, boutons `shape-<preset>`) ; `ToolCursor.qml` : flèche suivie de la
miniature de la forme courante quand l'outil est « shape » ; scénario `formes` (S7.1).

> **Amendement du 2026-10-26 (L4-C7, constats M-A, N1, C-M2 du point d'étape 4a).** Le nuancier 2 × 4 est celui
> du **remplissage**, pas du contour (P38 ; dans l'original, « sans remplissage » est une valeur de la couleur de
> remplissage) : 1re case = remplissage par défaut de l'outil (#FFAA00, P5/D2, absent des couleurs de la
> disposition), 6 couleurs de la disposition, 8e case « sans remplissage » ; la bascule « Sans remplissage » à
> part disparaît. Le **contour** se règle par la palette principale, outil des formes actif : `ToolHost::setColor`
> écrit les réglages de l'outil « shape » au lieu de basculer sur le stylo (P6/D3). Preuves : `swatchSetsTheFill`
> et `mainPaletteSetsTheShapeOutline` (`tst_ui_shapes`), scénario `formes` (annulation du glisser du point
> intelligent).

**Fichiers.** `ui/qml/ShapePalette.qml`, `ui/qml/ToolCursor.qml`, `bench/scenarios/formes.json`.

**Test et commande bornée.** Commande ciblée `-R tst_captures_formes` ; preuve visuelle ; banc sans écran ; preuve
de tâche.

**Preuve visuelle.** `captures.ps1 -Scenario formes -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire
"appTools.currentTool=shape" -Lire "appDocument.shownObjectCount=2"` : `LU … OK` pour les deux, `PIXEL
rectangle-contour OK`, `PIXEL cercle-interieur-blanc OK`, `CAPTURES OK` ; décrire `palette.png`, `rectangle.png`,
`cercle-centre.png`, `fleche-point.png`.

**Interdits.** Aucune icône ni miniature de l'original ; formes dessinées depuis notre géométrie.

**Terminé quand.** `tst_captures_formes` vert ; preuve visuelle conforme ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : scénario rouge** `bench/scenarios/formes.json` :
  ```json
  {
    "format": "actcraft-scenario/1",
    "name": "formes",
    "description": "S7.1 : palette, rectangle depuis le coin, clic simple sans forme, cercle depuis le centre sans remplissage, point intelligent d'une flèche.",
    "steps": [
      {"waitReady": true},
      {"command": "tool.shape"},
      {"expect": "shapePalette.visible", "value": true},
      {"capture": "palette"},
      {"command": "shape.choose", "argument": "rectangle"},
      {"stroke": [[200, 200], [400, 320]], "pressure": 0.5},
      {"waitReady": true},
      {"checkPixel": "rectangle-contour", "at": [200, 260], "color": "#000000", "tolerance": 60},
      {"capture": "rectangle"},
      {"stroke": [[600, 600], [601, 601]], "pressure": 0.5},
      {"waitReady": true},
      {"expect": "appDocument.shownObjectCount", "value": 1},
      {"command": "shape.fill-color", "argument": "#43A047"},
      {"command": "shape.choose", "argument": "circle"},
      {"command": "shape.draw-from-center"},
      {"command": "shape.filled"},
      {"stroke": [[700, 300], [760, 360]], "pressure": 0.5},
      {"waitReady": true},
      {"expect": "appDocument.shownObjectCount", "value": 2},
      {"checkPixel": "cercle-interieur-blanc", "at": [700, 300], "color": "#FFFFFF", "tolerance": 8},
      {"capture": "cercle-centre"},
      {"expect": "appTools.currentTool", "value": "shape"}
    ]
  }
  ```
  La partie « flèche et point intelligent » s'ajoute à la fin : `shape.choose arrow`, tracé, `tool.select`, clic
  sur la flèche par `pointer` (A11), `expect` d'un point intelligent visible (`selectionOverlay` : objet
  `smartPoint0`), glisser le point, `expect appDocument.canUndo true`, `capture fleche-point`.
- [ ] **Étape 2 : QML** (palette, curseur).
- [ ] **Étape 3 : vert**, preuve visuelle, banc sans écran, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add ui/qml/ShapePalette.qml ui/qml/ToolCursor.qml bench/scenarios/formes.json
  git -C <wt> commit -m "ui : palette des formes et curseur à miniature ; scénario formes (L4-T12)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t14"></a>
### Tâche L4-T14 — Banc et joueur : `forme-page-annotee`, `grille-dessus`, étape `typeText`

**Amendement 4 (V9).** Fichiers en plus : `bench/include/actcraft/bench/result.h` (champ ajouté en fin de
`ScenarioResult`) et `bench/src/result.cpp` (écriture et relecture de la clé dans `scenarioJson`, `:68`, qui ne pose
qu'un jeu de clés fixe) ; `lot4ScenariosWithFrames` vérifie que `forme-page-annotee` sérialise et relit
`shape_preview`. `forme-page-annotee` mesure aussi le coût de l'aperçu des formes, que L4-T8 ne peut pas
mesurer (aucun scénario de banc ne trace de forme avant celui-ci) : `LiftLayerItem::setObjects` reconstruit ses nœuds
à chaque mouvement. Le JSON relève les trames pendant les glissés sous la clé `shape_preview` (`frames`, `p95_ms`,
`max_ms` ; mêmes critères de figement que `stroke`, DA7) ;
`docs/banc.md` le dit. Résultat, pas porte ; hors budget : H4-8.

**Amendement 1 (défaut 16, amendement 1 pour A11).** Attendus chiffrés, écrits dans `docs/banc.md` et vérifiés au
JSON : `forme-page-annotee` pose **k** formes (k ≥ 10, fixé par le scénario) et `cache.refreshes = k` (chaque
insertion hors fin de page redessine une fois, limite de DA11) ; `grille-dessus` : `cache.refreshes` égal au nombre de
versements de la minuterie (aucun redessin entre deux versements, comme `stroke` sans grille), relevés tous deux dans
le JSON. `TypeText` est ajouté **après** `DoubleClick` ; `stepKindRanksFrozen` (L0-S2, `tests/tst_bench_uiscenario.cpp`)
est **étendu** d'une ligne pour son rang, les rangs existants inchangés.

**Objectif.** Deux scénarios de banc demandés par le lot 10 (écart 5) : `forme-page-annotee` (page générée avec
ses annotations ; rectangles posés un à un par `tool.shape` et tracés injectés — chaque insertion est hors fin de
page et redessine : on **mesure** le figement) ; `grille-dessus` (page générée à grille du dessus ; traits du
scénario `stroke` : différés depuis L4-T7, `cache.refreshes = 0` pendant les traits). Étape `typeText` du joueur
(`{"typeText": "Bonjour"}` : un `QKeyEvent` par caractère avec son texte, ligne `TEXTE <texte> OK`), ajoutée à la
**fin** de `UiStepKind` après les étapes de L0-S.

**Fichiers.** `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp`, `bench/src/runner.cpp`,
`tests/tst_bench_scenarios.cpp`, `tests/tst_bench_result.cpp`, `bench/include/actcraft/bench/uiscenario.h`,
`bench/src/uiscenario.cpp`, `tests/tst_bench_uiscenario.cpp`, `docs/banc.md`.

**Test et commande bornée.** Commande ciblée `-R 'tst_bench_scenarios|tst_bench_result|tst_bench_uiscenario'` ;
fonctions `shapeOnAnnotatedPageScenario`, `gridAboveScenarioDrawsStrokes`, `lot4ScenariosWithFrames`,
`typeTextStepParsed` en `PASS` ; banc sans écran : les deux scénarios présents dans le JSON, `cache.refreshes`
relevé, et pour `forme-page-annotee` la clé `shape_preview` (`{"frames": n, "p95_ms": x, "max_ms": y}` : trames
pendant les glissés de forme, amendement 4, V9) présente avec n ≥ 1 ; preuve de tâche.

**Interdits.** Ne pas changer les scénarios existants ni leurs critères. Le verdict reste un résultat (DA7).

**Terminé quand.** Fonctions vertes ; JSON du banc joint avec les deux scénarios et la clé `shape_preview` de
`forme-page-annotee` ; `docs/banc.md` décrit les deux scénarios, `shape_preview` et l'étape `typeText` ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : tests rouges** : `shapeOnAnnotatedPageScenario` (le scénario existe, a au moins 10 formes, sur
  une page dont la couche du dessus a des annotations) ; `gridAboveScenarioDrawsStrokes` (page à grille du dessus,
  traits) ; `lot4ScenariosWithFrames` (sur le modèle de `sevenScenariosWithFrames` du lot 3 : compte des scénarios
  + 2, chacun avec ses images ; amendement 4 : `shape_preview` de `forme-page-annotee` écrit puis relu) ; `typeTextStepParsed` (`{"typeText": "Bonjour"}` → `UiStepKind::TypeText`, texte
  gardé ; chaîne vide : `SCENARIO INVALIDE`).
- [ ] **Étape 2 : implémenter** (scénarios, choix de l'outil par commande avant la mesure comme L3-T19, étape du
  joueur et sa ligne `TEXTE`).
- [ ] **Étape 3 : vert**, banc sans écran (lire `forme-page-annotee` et `grille-dessus` dans le JSON), preuve de
  tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add bench/include/actcraft/bench/scenarios.h bench/src/scenarios.cpp bench/src/runner.cpp bench/include/actcraft/bench/result.h bench/src/result.cpp tests/tst_bench_scenarios.cpp tests/tst_bench_result.cpp bench/include/actcraft/bench/uiscenario.h bench/src/uiscenario.cpp tests/tst_bench_uiscenario.cpp docs/banc.md
  git -C <wt> commit -m "banc : scénarios forme sur page annotée et grille au-dessus ; étape typeText du joueur (L4-T14)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t31"></a>
### Tâche L4-T31 — `tools` : ligne et courbe brisées au multi-clic, forme à main levée

**Origine.** Amendement 4 : P37 et C3 (la ligne brisée se pose point par point, un clic par sommet, et se termine par un
double-clic ; la courbe brisée de même), D1 (rangs 4 à 6 du catalogue), point 9 de L4-T0. Dépend de L4-T8 (l'outil
des formes, qui écarte ces trois préréglages du tracé en boîte) et, par elle, de L4-T3 (`ShapeGesture`).

**Objectif.** Donner leur geste aux trois préréglages que L4-T8 laisse de côté (`core::shapeGesture` différent de
`Box`) :
- **`polyline` et `curve-chain` (geste `Clicks`).** Chaque appui pose un sommet (point accroché par
  `core::nearestGridPoint` si la grille de la page accroche, comme L4-T8) ; un relâché à plus de
  `kShapeMinDragScreen / zoom` de son appui pose aussi un sommet (un glissé trace un segment). L'aperçu montre les
  sommets posés (`lift.show`, aucune commande) ; il est refait **à chaque sommet**, et pendant un glissé bouton tenu ;
  entre deux clics, il ne suit pas le pointeur : le survol n'arrive pas à l'outil (`input/src/qtevents.cpp:20-22`,
  `:40-43` le jettent), écart déclaré par H4-19. Un appui à moins de
  `kChainDoubleClickScreen / zoom` du dernier sommet **et** moins de `kChainDoubleClickNs` après l'appui précédent
  (`input::Sample::timestampNs`) est un double-clic : il ne pose pas de sommet, il termine — `lift.clear()`, puis
  **une** `core::AddObjectCommand` de `core::makeChainObject(preset, sommets, style)` si elle rend un objet, rien
  sinon. `curve-chain` passe ses sommets à `core::smoothChain` (dans `makeChainObject`) : la géométrie lissée vit dans
  `core/`.
- **`freeform` (geste `Freehand`).** Appui, mouvements, relâché comme un trait : les points du geste (sans accrochage,
  supposé ; un point à moins d'un pixel d'écran du précédent est sauté) ; aperçu à chaque mouvement ; au relâché, si
  l'emprise du geste dépasse `kShapeMinDragScreen / zoom`, une `AddObjectCommand` de `makeChainObject`, sinon rien.
  Tracé ouvert, sans remplissage (supposé : objet `core::Line`).
- **Fin anticipée.** `cancel()` (changement d'outil, de page, Annuler : ToolHost l'appelle) verse la ligne commencée si
  elle a au moins deux sommets distincts, comme le stylo verse son trait ; `abandon()` efface l'aperçu et ne verse
  rien. Échap n'atteint jamais `onKey` : `tool.select` appelle `cancelGesture()` → `cancel()`, puis active
  Sélectionner (`ui/qml/Main.qml:298-302`, `ui/src/toolhost.cpp:370-378`) ; une ligne brisée de deux sommets ou plus
  est donc versée en une étape d'annulation, comme le trait du stylo, et l'outil passe à Sélectionner (test
  `escapeVersesTheChainInOneStep`). `isBusy()` est vrai tant qu'une ligne est commencée ou qu'un geste à main
  levée est en cours.
- **Style.** Celui de L4-T8 : contour et épaisseur de `user.settings("shape")` (noir, 4 par défaut, D3) ; la plume de
  la `Line` est le contour.

**Ajout en fin de `core/include/actcraft/core/shapegeometry.h`** (après `ShapeGesture` de L4-T3, signalé ; l'ajout de
`<cstdint>` et `<optional>` au bloc d'inclusions, s'ils manquent, est autorisé par l'amendement 4 et signalé au
rapport) :
```cpp
// Amendement 4 du lot 4 (L4-T31, P37) : ajout en fin. Lignes posées au clic et forme à main levée.
inline constexpr double kChainDoubleClickScreen = 6.0;           // second appui à moins de 6 px d'écran du dernier sommet
inline constexpr std::int64_t kChainDoubleClickNs = 500'000'000; // et moins de 500 ms après l'appui précédent
inline constexpr int kCurveChainSamples = 12;                    // points par segment de la courbe brisée

// Courbe de Catmull-Rom centripète passant par chaque sommet : (n − 1) × samplesPerSegment + 1 points, chaque
// sommet au rang i × samplesPerSegment. Moins de trois sommets : les sommets tels quels.
std::vector<PointF> smoothChain(const std::vector<PointF>& vertices, int samplesPerSegment = kCurveChainSamples);

// Line posée par les gestes Clicks et Freehand : polyline → sommets tels quels ; curve-chain → smoothChain ;
// freeform → points du geste. Points consécutifs confondus retirés ; moins de deux points distincts, ou préréglage
// dont le geste est Box : std::nullopt. Même convention de coordonnées que makeLineObject (L4-T3), couche
// intermédiaire, plume = contour du style.
std::optional<PageObject> makeChainObject(std::string_view preset, const std::vector<PointF>& points,
                                          const ShapeStyle& style);
```
**Ajout à `tools/include/actcraft/tools/shapetool.h`** (signalé) : en public, `void abandon() override; // rien n'est
versé` ; en privé, `std::vector<core::PointF> m_chain; // sommets posés (Clicks) ou points du geste (Freehand)` et
`std::int64_t m_lastPressNs = 0;` (ajout de `<cstdint>` et `<vector>` au bloc d'inclusions s'ils manquent :
autorisé, signalé).

**Fichiers.** `core/include/actcraft/core/shapegeometry.h`, `core/src/shapegeometry.cpp`,
`tools/include/actcraft/tools/shapetool.h`, `tools/src/shapetool.cpp`, `tests/tst_core_shapes.cpp`,
`tests/tst_tools_shape.cpp`. (Le point 9 de L4-T0 ne cite pas `shapetool.h` : il s'y ajoute pour l'état privé du
multi-clic et `abandon()`.)

**Test et commande bornée.** Commande ciblée `-R 'tst_core_shapes|tst_tools_shape'` (deux tests, `100% tests passed,
0 tests failed out of 2`) ; fonctions `smoothChainPassesThroughVertices`, `chainNeedsTwoDistinctPoints` (avec les neuf
de L4-T3 : onze fonctions, `Totals: 13 passed`), `polylineByClicksEndsOnDoubleClick`, `curveChainIsSmoothed`,
`freeformFollowsTheDrag`, `chainCancelKeepsTwoVerticesOrMore`, `chainAbandonAddsNothing`, `escapeVersesTheChainInOneStep`
(avec les neuf de L4-T8 après l'amendement 5 : quinze fonctions, `Totals: 17 passed`) en
`PASS` ; banc sans écran ; preuve de tâche : `100% tests passed, 0 tests failed out of N`, N = 136 + n_B **inchangé**
(aucun test enregistré), `TESTS OK`, lignes `tst_core_shapes … Passed` et `tst_tools_shape … Passed`.

**Interdits.** Aucun en-tête de `render/` ni de `ui/` dans `tools/`. Pas de géométrie dans `tools/` : le lissage et la
construction de l'objet sont dans `core/`. Aucune écriture dans le document hors `execute`. Aucun test de L4-T3 ni de
L4-T8 modifié : `chainPresetsExecuteNothingOnRelease` (L4-T8) reste vert tel quel. Pas de QtGui dans `core/`.

**Terminé quand.** Les huit fonctions neuves passent, les fonctions de L4-T3 et de L4-T8 restent vertes ; `TESTS OK` ;
N inchangé ; banc joint ; ajouts aux deux en-têtes signalés au rapport.

**Étapes.**
- [ ] **Étape 1 : tests rouges de `core`.** Ajouter à `tests/tst_core_shapes.cpp`, dans `private slots` :
  ```cpp
      void smoothChainPassesThroughVertices() // L4-T31
      {
          const std::vector<PointF> vertices{{0, 0}, {100, 0}, {100, 100}, {200, 100}};
          const std::vector<PointF> curve = smoothChain(vertices, 8);
          QCOMPARE(curve.size(), std::size_t{3 * 8 + 1});
          for (std::size_t i = 0; i < vertices.size(); ++i) {
              QVERIFY(std::abs(curve[i * 8].x - vertices[i].x) < 1e-9);
              QVERIFY(std::abs(curve[i * 8].y - vertices[i].y) < 1e-9);
          }
          bool leavesTheStraightSegment = false; // entre (100, 0) et (100, 100), la courbe n'est pas droite
          for (std::size_t k = 1; k < 8; ++k)
              leavesTheStraightSegment = leavesTheStraightSegment || std::abs(curve[8 + k].x - 100) > 1e-6;
          QVERIFY(leavesTheStraightSegment);
          QCOMPARE(smoothChain({{0, 0}, {10, 0}}, 8).size(), std::size_t{2});
      }
      void chainNeedsTwoDistinctPoints() // L4-T31
      {
          QVERIFY(!makeChainObject("polyline", {{10, 10}}, {}).has_value());
          QVERIFY(!makeChainObject("polyline", {{10, 10}, {10, 10}}, {}).has_value());
          QVERIFY(!makeChainObject("rectangle", {{0, 0}, {50, 50}}, {}).has_value()); // geste Box
          const std::optional<PageObject> o = makeChainObject("polyline", {{0, 0}, {0, 0}, {50, 0}, {50, 40}}, {});
          QVERIFY(o.has_value());
          QCOMPARE(o->common.layer, Layer::Middle);
          QCOMPARE(std::get<Line>(body(*o)).points.size(), std::size_t{3}); // doublon retiré
          QVERIFY(makeChainObject("freeform", {{0, 0}, {3, 1}, {6, 4}}, {}).has_value());
          QVERIFY(std::get<Line>(body(*makeChainObject("curve-chain", {{0, 0}, {50, 0}, {50, 50}}, {}))).points.size()
                  == std::size_t{2 * kCurveChainSamples + 1});
      }
  ```
  Commande ciblée `-R tst_core_shapes` → rouge (fonctions non déclarées : erreur de compilation attendue).
- [ ] **Étape 2 : `core`.** Ajouter le bloc d'en-tête ci-dessus ; dans `shapegeometry.cpp`, `smoothChain` (Catmull-Rom
  centripète, alpha 0,5 ; extrémités prolongées par symétrie du premier et du dernier segment ; nœuds confondus :
  segment droit) et `makeChainObject` (dédoublonnage, `shapeGesture`, construction comme `makeLineObject`). Commande
  ciblée → vert.
- [ ] **Étape 3 : tests rouges de `tools`.** Ajouter à `tests/tst_tools_shape.cpp`, dans l'espace de noms anonyme
  (après `at`) :
  ```cpp
  input::Sample atTime(double x, double y, input::Phase phase, std::int64_t ms)
  {
      input::Sample s = at(x, y, phase);
      s.timestampNs = ms * 1'000'000;
      return s;
  }
  void click(tools::ShapeTool& tool, double x, double y, std::int64_t ms)
  {
      tool.onSample(atTime(x, y, input::Phase::Press, ms));
      tool.onSample(atTime(x, y, input::Phase::Release, ms + 50));
  }
  std::vector<core::PointF> pagePoints(const core::PageObject& o) // translation seule (convention de L4-T3)
  {
      std::vector<core::PointF> out;
      for (const core::PointF& p : std::get<core::Line>(core::body(o)).points)
          out.push_back({p.x + o.common.transform.dx, p.y + o.common.transform.dy});
      return out;
  }
  ```
  puis dans `private slots` :
  ```cpp
      void polylineByClicksEndsOnDoubleClick() // L4-T31
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("polyline"), true, 0xFFFFAA00u, false});
          tools::ShapeTool tool(h.context);
          click(tool, 100, 100, 0);
          click(tool, 200, 100, 1000);
          QCOMPARE(h.lift.shown.size(), std::size_t{1}); // aperçu refait au sommet posé (pas de survol, H4-19)
          QCOMPARE(std::get<core::Line>(core::body(h.lift.shown.front())).points.size(), std::size_t{2});
          QCOMPARE(h.doc.executed, 0);
          QVERIFY(tool.isBusy());
          click(tool, 200, 200, 2000);
          QCOMPARE(std::get<core::Line>(core::body(h.lift.shown.front())).points.size(), std::size_t{3});
          click(tool, 201, 201, 2200); // double-clic : à moins de 6 px et de 500 ms, ne pose pas de sommet
          QCOMPARE(h.doc.executed, 1);
          QVERIFY(h.lift.shown.empty());
          QVERIFY(!tool.isBusy());
          const std::vector<core::PointF> points = pagePoints(h.doc.document.page(0).objects.back());
          QCOMPARE(points.size(), std::size_t{3});
          QCOMPARE(points[0].x, 100.0);
          QCOMPARE(points[1].x, 200.0);
          QCOMPARE(points[2].y, 200.0);
      }
      void curveChainIsSmoothed() // L4-T31
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("curve-chain"), true, 0xFFFFAA00u, false});
          tools::ShapeTool tool(h.context);
          click(tool, 0, 0, 0);
          click(tool, 100, 0, 1000);
          click(tool, 100, 100, 2000);
          click(tool, 200, 100, 3000);
          click(tool, 200, 100, 3100); // double-clic
          QCOMPARE(h.doc.executed, 1);
          const std::vector<core::PointF> points = pagePoints(h.doc.document.page(0).objects.back());
          QCOMPARE(points.size(), std::size_t{3 * core::kCurveChainSamples + 1});
          QCOMPARE(points.front().x, 0.0);
          QCOMPARE(points.back().x, 200.0);
          QCOMPARE(points.back().y, 100.0);
      }
      void freeformFollowsTheDrag() // L4-T31
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("freeform"), true, 0xFFFFAA00u, false});
          tools::ShapeTool tool(h.context);
          tool.onSample(at(0, 0, input::Phase::Press));
          for (int i = 1; i <= 10; ++i)
              tool.onSample(at(10.0 * i, (i % 2) * 5.0, input::Phase::Move));
          QCOMPARE(h.lift.shown.size(), std::size_t{1});
          QCOMPARE(h.doc.executed, 0);
          tool.onSample(at(100, 0, input::Phase::Release));
          QCOMPARE(h.doc.executed, 1);
          QVERIFY(h.lift.shown.empty());
          QVERIFY(pagePoints(h.doc.document.page(0).objects.back()).size() >= std::size_t{11});
          tool.onSample(at(300, 300, input::Phase::Press)); // sous le seuil : rien
          tool.onSample(at(301, 301, input::Phase::Release));
          QCOMPARE(h.doc.executed, 1);
      }
      void chainCancelKeepsTwoVerticesOrMore() // L4-T31
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("polyline"), true, 0xFFFFAA00u, false});
          tools::ShapeTool tool(h.context);
          click(tool, 100, 100, 0);
          tool.cancel(); // un seul sommet : rien n'est versé
          QCOMPARE(h.doc.executed, 0);
          QVERIFY(!tool.isBusy());
          click(tool, 100, 100, 1000);
          click(tool, 200, 100, 2000);
          tool.cancel(); // changement d'outil ou de page : la ligne commencée est versée, comme le trait du stylo
          QCOMPARE(h.doc.executed, 1);
          QVERIFY(h.lift.shown.empty());
          QVERIFY(!tool.isBusy());
      }
      void escapeVersesTheChainInOneStep() // L4-T31 : Échap → tool.select → cancelGesture() → cancel()
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("polyline"), true, 0xFFFFAA00u, false});
          tools::ShapeTool tool(h.context);
          click(tool, 100, 100, 0);
          click(tool, 200, 100, 1000);
          click(tool, 200, 200, 2000);
          // Le chemin Échap → tool.select → cancelGesture() relève de ToolHost (lot 3, ui/src/toolhost.cpp:370-378) et
          // n'est pas rejoué ici ; ce test vérifie ce qu'il déclenche : cancel() verse la ligne en une étape.
          if (tool.isBusy())
              tool.cancel();
          QCOMPARE(h.doc.executed, 1); // une seule AddObjectCommand : une étape d'annulation
          QCOMPARE(pagePoints(h.doc.document.page(0).objects.back()).size(), std::size_t{3});
          QVERIFY(h.lift.shown.empty());
          QVERIFY(!tool.isBusy());
      }
      void chainAbandonAddsNothing() // L4-T31
      {
          Harness h;
          h.user.setShapeSettings({QStringLiteral("polyline"), true, 0xFFFFAA00u, false});
          tools::ShapeTool tool(h.context);
          click(tool, 100, 100, 0);
          click(tool, 200, 100, 1000);
          tool.abandon();
          QCOMPARE(h.doc.executed, 0);
          QVERIFY(h.lift.shown.empty());
          QVERIFY(!tool.isBusy());
      }
  ```
  Commande ciblée `-R tst_tools_shape` → rouge.
- [ ] **Étape 4 : `ShapeTool`.** Ajouts à `shapetool.h` ci-dessus ; dans `shapetool.cpp`, aiguillage par
  `core::shapeGesture` au début de `onSample` : `Box` → chemin de L4-T8 inchangé ; `Clicks` et `Freehand` → règles de
  l'objectif ; aperçu par `lift.show({*makeChainObject(…)})` quand il rend un objet, `lift.clear()` sinon ; `cancel()`
  et `abandon()` selon l'objectif. Commande ciblée → vert, fonctions de L4-T8 comprises.
- [ ] **Étape 5 : preuves.** `format.ps1`, preuve de tâche (N inchangé), banc sans écran (non-régression, aucun
  scénario neuf), `git add`, `NOMS OK`.
- [ ] **Étape 6 : commit.**
  ```powershell
  git -C <wt> add core/include/actcraft/core/shapegeometry.h core/src/shapegeometry.cpp tools/include/actcraft/tools/shapetool.h tools/src/shapetool.cpp tests/tst_core_shapes.cpp tests/tst_tools_shape.cpp
  git -C <wt> commit -m "tools : ligne et courbe brisées posées au clic, forme à main levée (L4-T31)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-o1"></a>
### Tâche L4-O1 — Séance d'observation de l'original à l'écran (D5, D6, D10, D11, D17)

**Écart constaté (amendement 5).** Mergée. Deux de ses verdicts sont repris par L4-O2 : D5 (contredit de nouveau,
cette fois avec un témoin de Maj) et D17 (L4-O2 : la grille du dessus couvre tout ce qui est posé ; le geste de
L4-O1 cochait les deux cases dans la même ouverture de la boîte). Son verdict D11 « rien sur la page en édition » est
contredit par L4-O2 (séparations pointillées). D6 (confirmé) et D10 (cumulé) restent.

**Origine.** Point 11 de L4-T0 : cinq comportements ne se lisent pas en statique ; une seule séance à l'écran, annoncée
à Jérôme. Ce n'est pas une tâche de code : elle ne construit rien, n'occupe aucune place de tests et se joue hors des
vagues (section « Vagues », amendement 4), au plus tard avant V9 du plan 4b : L4-T26, L4-T28 et L4-C1 l'attendent.

**Rôle de l'orchestrateur (avant de lancer le subagent).** Prévenir Jérôme d'une ligne : la souris et le clavier de
l'écran principal vont être pilotés, l'original va s'ouvrir, durée estimée. Attendre qu'aucune fenêtre d'ActCraft ne
soit ouverte à l'écran (`tools/cmp.py` de l'ancien dépôt ferme toute fenêtre « ActCraft » et `Inspire.exe`). Le subagent
prend le jeton `ecran` pour toute la séance (`jeton.ps1 -Nom ecran -Proprietaire L4-O1`). À la remise du rapport,
l'orchestrateur ouvre un correctif `L4-C<n>` par tâche déjà faite que touche un verdict « contredit », amende les
sections des tâches pas encore lancées (colonne « si contredit » ci-dessous), fait relire, puis lance les tâches qui
l'attendent (L4-T26 en V9, L4-T28, L4-C1).

**Objectif.** Observer la 3.5.18 (`C:\Program Files\Activ Software\Inspire\`) sur l'écran principal (1920 × 1080), sur
un paperboard neuf non enregistré, et rendre pour chaque point un verdict « confirmé », « contredit » ou « non
observé », avec ce qui a été vu, écrit en nos mots :

| Point | Geste à faire | Ce que le plan suppose | Si contredit : tâches à amender |
|---|---|---|---|
| D5 — Maj pendant le tracé | outil des formes, rectangle glissé Maj tenue, puis Maj relâchée avant le relâché ; ligne glissée Maj tenue | carré ; ratio libéré au relâché de Maj ; ligne contrainte à 45° (H4-12) | L4-T8 (`shiftKeepsSquare`, `shiftReleaseFreesTheRatio`), L4-T11 |
| D6 — grille invisible | grille avec accrochage, rectangle déplacé ; grille cachée, rectangle déplacé de nouveau | l'accrochage continue grille cachée (F-622, H4-12) | L4-T5 (`invisibleGridStillSnaps`), L4-T9 |
| D10 — règle de l'allongement | page de 768 de haut, trois clics sur la poignée d'allongement ; hauteur relevée | pas fixe : 998,4 ; cumulé : 1022,2 (H4-14) | L4-T4 (`extendStepsByTenPercentOfBase`), L4-T26 (attendus 998,4) |
| D11 — dessin des cadres | page réglée à 2 cadres en largeur et 3 en hauteur ; séparations dessinées ou non, aspect, en présentation ; effet sur un objet déplacé et sur l'accrochage ; boîte de taille personnalisée d'un paperboard neuf : taille de page obtenue pour 2 × 3 cadres de 400 × 300 | dessin non tranché ; taille de page = taille d'un cadre × nombre de cadres (supposé, à observer) ; la taille d'un cadre est une donnée de la page (D11) | L4-T27 (si la donnée diffère), L4-T28 (contrat du dessin), L4-C1 (règle de la taille) |
| D17 — encre en cours et grille du dessus | grille « toujours visible », trait au stylo tracé lentement ; capture pendant le tracé | la grille couvre le trait en cours, rien ne change au relâché | L4-T2 (`socleLayerOrder`), L4-T7 |

**Fichiers.** `docs/verifications/2026-10-21-lot4-observation.md` (dans le dépôt : verdicts, gestes et constats en nos
mots, aucune capture). Hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot4-observation.md` et ses
captures (relevé brut).

**Outils.** Ceux de l'ancien dépôt pour rejouer de vraies entrées (`tools/drive.py`, `tools/scenarios.py`,
`tools/zoom.py`, lancés **dans** `C:\Users\JeromeDenis\REA - Activinspire`), ou la commande de l'écran ; graphe de
l'ancien dépôt (`graphify query "…"` dans ce dossier) pour retrouver les gestes déjà joués par l'application de
référence.

**Preuve.** Rapport commité avec ses cinq verdicts ; relevé brut présent hors dépôt ;
`git -C <wt> add docs/verifications/2026-10-21-lot4-observation.md` puis
`python -I "C:/Users/JeromeDenis/REA - Activinspire/tools/controle_noms_actcraft.py" <wt>` → `NOMS OK` ; commande des
titres du corpus (section de L4-T0) sur le worktree → sortie vide.

**Interdits.** Aucune capture, aucun texte de l'aide, aucun nom de commande ou de ressource de l'original dans le
dépôt : nos identifiants (`tool.shape`, `page.extend`…) et nos mots. Ne jamais modifier
`C:\Users\JeromeDenis\REA - Activinspire\samples\` ni le profil `%APPDATA%\Promethean` : ne changer aucun réglage de
l'application ; une bascule d'affichage par un menu est remise dans son état avant la fermeture ; relever avant et
après la séance la date de modification des fichiers du profil (lecture seule) et, si elle a changé, le dire tout de
suite à l'orchestrateur (Jérôme décide). Ne rien enregistrer dans `samples/`. Aucune fenêtre ailleurs que sur l'écran
principal.

**Terminé quand.** Cinq verdicts écrits, chacun avec le geste joué, ce qui a été vu et les tâches à amender ; `NOMS OK` ;
sortie du contrôle des titres vide ; jeton `ecran` rendu ; l'original fermé.

**Étapes.**
- [ ] **Étape 1 : préparer.** Dates des fichiers du profil relevées ; aucune fenêtre d'ActCraft ouverte ; jeton `ecran`
  pris ; original lancé sur l'écran principal, paperboard neuf.
- [ ] **Étape 2 : observer** les cinq points du tableau, dans l'ordre, avec une capture hors dépôt pour chacun et les
  valeurs lues (hauteur de page, dimensions d'une forme) quand l'original les affiche.
- [ ] **Étape 3 : fermer** l'original sans enregistrer ; relever de nouveau les dates du profil ; rendre le jeton.
- [ ] **Étape 4 : écrire** le relevé brut hors dépôt puis `docs/verifications/2026-10-21-lot4-observation.md` (tableau
  « point, geste, constat en nos mots, verdict, tâches à amender »).
- [ ] **Étape 5 : contrôles et commit.**
  ```powershell
  git -C <wt> add docs/verifications/2026-10-21-lot4-observation.md
  git -C <wt> commit -m "Lot 4 : observation à l'écran de cinq comportements non tranchés en statique (L4-O1)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-o2"></a>
### Tâche L4-O2 — Seconde séance d'observation de l'original (D5 avec témoin, D17 bis, reste de D11) — mergée

**Origine.** Rapport de L4-O1 : D5 sans témoin de Maj, ordre de la grille du dessus par rapport à l'encre posée et aux
formes non étudié, D11 en partie observé. Séance jouée le 2026-10-21, mergée en 209b8fe ; rapport
`docs/verifications/2026-10-21-lot4-observation-2.md`, relevé brut et captures hors dépôt. Profil de l'original copié
avant le premier lancement et remis à l'identique après (choix de Jérôme), 0 écart fichier par fichier.

**Verdicts, et ce qu'ils changent (amendement 5).**
- D5 : témoin positif (Maj injectée reçue pendant un geste de souris) ; Maj tenue avant l'appui ou enfoncée au milieu
  du glisser ne change ni le rectangle ni la ligne à 35° → L4-T8, L4-T11.
- D17 bis : la grille du dessus passe sur le trait, le rectangle rempli et le texte, quel que soit le calque ; trait en
  cours non capturé ici → L4-C3, L4-T7 (décision de l'orchestrateur pour le trait en cours, amendement 5, point 2).
- D11 : séparations fines, pointillées, claires, aux limites des cadres, en édition comme en présentation ; page
  suivante passe à la page suivante sans parcourir les cadres ; un objet déplacé n'est pas attiré ; l'accrochage
  n'ajoute aucun point ; boîte de taille personnalisée : 2 × 3 cadres de 400 × 300 ramenés à 500 × 500, page de
  1000 × 1500 → L4-T27, L4-T28, L4-C1 (plan 4b).
- Réserves : Maj+clic n'ajoute pas à la sélection (c'est Ctrl+clic) et n'étend pas la sélection d'un texte de la page
  → H4-24 ; couleur exacte des séparations non mesurée (la nôtre vient de notre thème) ; minimum de 500 vu, pas cherché
  plus loin → H4-20 ; origine de la grille d'accrochage dans un cadre non tranchée → H4-21.

**Rien à exécuter** : la ligne de la table sert aux dépendances (L4-C3, L4-T28, L4-C1).

---

## Hypothèses à revoir (porte de fin du lot 4)

### Niveau 1
- **H4-1** — Catalogue des formes : les 22 premières formes de la 3.5.18 dans son ordre (D1, amendement 4), dont
  `freeform`, `polyline`, `curve-chain` aux rangs 4 à 6.
- **H4-2** — Pas de courbe de Bézier ; la forme à main levée, la ligne et la courbe brisées existent (D1) : L4-T31.
- **H4-3** — Texte de haut en bas = lettres empilées (colonne d'un caractère, retour à la ligne partout) : D9.
- **H4-4** — Zones de page : la donnée est fixée par D11 (cadres, 0 à 40 en largeur, 0 à 53 en hauteur) et L4-T27
  la pose ; le dessin et l'effet sont observés par L4-O2 (amendement 5) : séparations pointillées en édition et en
  présentation, aucun effet sur les objets ni sur l'accrochage ; L4-T28 les dessine.
- **H4-5** — Panneau des navigateurs ancré seulement au lot 4 ; flottant au lot 6.

### Niveau 2
- **H4-6** — Ancrage d'un texte créé par ActCraft (sans lignes lues) : coin bas gauche de la première ligne mesurée
  par `ui::TextEditing` ; à défaut, de l'emprise.
- **H4-7** — Miniatures regroupées à 400 ms et suspendues pendant un contact : à revoir si `miniatures` montre un
  coût de copie de page supérieur à 2 ms sur la page générée.
- **H4-8** — Formes et textes sous les annotations : redessin immédiat (limite de DA11). Si `forme-page-annotee`
  dépasse la limite en D3D11 : tuiles (L10-T7) ou cache par groupe de couches, décision de la porte.
- **H4-9** — « Accrocher à la grille » bascule l'autorisation d'accrochage de la grille de la page (D6).
- **H4-10** — Texte de départ « Votre texte » (à nous) ; police par défaut N2-3.

### Ajoutées par L4-T0 (amendement 4)
- **H4-11** — Nom « Forme n » posé à la création, compteur par page ; affiché par le lot 6.
- **H4-12** — Maj pendant le tracé et accrochage d'une grille invisible : **tranchée** (amendement 5). Maj sans effet
  (L4-O1, puis L4-O2 avec témoin) ; l'accrochage continue grille cachée (L4-O1, D6 confirmé).
- **H4-13** — Couleurs et orientation du texte de haut en bas : hors du lot 4.
- **H4-14** — Allongement : **tranchée** (amendement 5). Pas cumulé, × 1,1 sur la hauteur courante (L4-O1 :
  1 022,21 après trois clics depuis 768) ; appliqué par L4-T4.
- **H4-15** — Taille des miniatures en réglette continue, l'original procédant par colonnes.
- **H4-16** — Case « toutes les pages » et dégradé radial de la boîte du fond : hors du lot 4.
- **H4-17** — Les 17 formes au-delà des 22 premières (croix, coche, trapèze, parallélogramme, demi-cercle, arcs,
  flèches coudées…) : hors du lot 4.
- (**H4-18**, au plan 4b : fond en image, numérotée H4-11 avant l'amendement 4.)
- **H4-19** (niveau 2, amendement 4) — Entre deux clics, l'aperçu d'une ligne ou d'une courbe brisée ne suit pas le
  pointeur : il est refait à chaque sommet seulement. Le suivre demanderait un relais du survol jusqu'à l'outil, que
  `input/src/qtevents.cpp:20-22` (stylet) et `:40-43` (souris) jettent aujourd'hui ; écart déclaré à la porte de fin du lot.

### Ajoutées par l'amendement 5
- **H4-20** (niveau 2) — Taille d'un cadre d'au moins 500 pixels par côté : vu une fois (400 × 300 ramenés à
  500 × 500 par la boîte de taille personnalisée, L4-O2), ni maximum ni pas cherchés. Appliqué par `clampZones`
  (L4-T27) à une taille non nulle et par la boîte de L4-C1 quand un nombre de cadres est donné.
- **H4-21** (niveau 2) — La grille d'accrochage part de l'origine de la page, pas de celle de chaque cadre : non
  tranché par L4-O2 (cadres multiples de 32 dans l'essai réussi, essai sur cadres de 500 raté) ; supposé, figé par
  `separatorsDoNotAttract` (L4-T27).
- **H4-22** (niveau 1) — Trait **en cours** couvert par la grille du dessus : décision de l'orchestrateur (rien ne
  change au relâché, L4-O1 ; grille peinte au premier plan, P15), contraire au constat de L4-O1 fait avec un geste
  que L4-O2 a mis en doute, et que L4-O2 n'a pas capturé. Porté à la porte de fin du lot.
- **H4-23** (niveau 2) — Les séparations des cadres se placent à `k × largeur / n` et `k × hauteur / m` de la page
  courante (observé sur une page de 1024 × 768 sans taille de cadre donnée) ; après un allongement, elles suivent
  la nouvelle hauteur (supposé).
- **H4-24** (niveau 2) — Maj+clic (réserve de L4-O2) : dans l'original, il n'ajoute pas à la sélection (Ctrl+clic
  le fait) et n'étend pas la sélection d'un texte de la page ; chez nous, Maj+clic ajoute à la sélection (lot 3,
  `shiftAndControlReachTheActiveTool`) et l'éditeur de texte de Qt étend la sélection. Aucune tâche du lot 4 ne le
  change ; écart porté à la porte de fin du lot.

## Décisions tranchées par ce plan

1. **Version 0.5.0**, à la fin de 4b ; un seul plan de réservation pour les deux sous-lots.
2. **Aperçu des formes par la couche soulevée** : aucun redessin pendant le tracé, une étape au relâché.
3. **Grille du dessus en surcouche, hors du cache** : bascule sans redessin, traits différés rétablis.
4. **Accrochage au dépôt**, pas pendant le glissement (comme la référence).
5. **Texte soulevé pendant son édition** (4b) : aucun redessin pendant la frappe.
6. **Miniatures rendues hors du fil principal** (4b), regroupées, jamais pendant un contact.
7. **Presse-papiers des pages interne** au lot 4 ; entre documents au lot 5.
8. **Réalisation libre** (décision 0100) : miniatures peintes par un élément C++ plutôt qu'un fournisseur d'images
   enregistré dans le moteur (aucun fichier de `app/` touché) ; à signaler au rapport de lot.
9. **Maj sans effet sur le tracé d'une forme** (amendement 5, D5, L4-O2) ; carré, cercle et lignes à axe fixe tenus
   par leur préréglage.
10. **Grille du dessus au-dessus de tout**, trait en cours compris (amendement 5, D17, L4-O2 ; H4-22).
11. **Miniatures suspendues panneau des navigateurs fermé** (amendement 5, L4-T19).

## Point d'étape du sous-lot 4a (orchestrateur, hors tâches)

Joué après V6 (L4-T31 comprise, en V5), dans le worktree du chantier, avant de lancer 4b. **Amendement 5 :** joué
après R3 (section « Vagues »), avant V9 de 4b ; les verdicts de L4-O1 et L4-O2 ont déjà été reportés (amendement 5) :
D5 → L4-T8, L4-T11 (sections amendées) ; D17 → L4-C3 (correctif de L4-T2), L4-T7 ; D10 → L4-T4 (appliqué en cours,
sans correctif), L4-T26 ; D11 → L4-T27, L4-T28, L4-C1. **Amendement 4 :** chaque
verdict « contredit » de L4-O1 ouvre un correctif `L4-C<n>` nommé par la tâche touchée (titre « correctif de
L4-T<k> : … », fichiers et preuve de cette tâche) : D5 → L4-T8, L4-T11 ; D6 → L4-T5, L4-T9 ; D17 → L4-T2, L4-T7 ;
D10 → L4-T4 (correctif ici) et L4-T26 (section amendée avant V9) ; D11 → L4-T27, L4-T28, L4-C1 (sections amendées avant
leur lancement). Si L4-O1 n'a pas encore rendu son rapport, l'orchestrateur la planifie avec Jérôme avant V9 de 4b.

1. **Double relecture** (conformité, qualité) du diff de 4a ; correctifs `L4-C<n>` (deux tours au plus).
2. **Suite** (jeton `tests`) : `scripts/test.ps1`, forme bornée, N = 136 + n_B, `TESTS OK` ; talons restants : ceux
   de 4b seulement (`git -C <wt> grep -n "Talon L4-T1" -- . ':(exclude)docs/'` ne cite que des tâches de 4b) ;
   règle 4 des talons du socle (amendement 1, défaut 14) pour les talons remplis en 4a : `-functions` de chaque test
   rempli sans `reserved`, aucun `SKIP` « Talon » dans `ctest -V` hors tests de 4b. Si le lot 7 a fusionné depuis
   L4-T0, n_B est relevé de nouveau (`ctest --preset debug -N` sur la base à jour), le nouveau N est écrit dans le
   fichier d'état et sert à toute preuve suivante (amendement 2, N12).
3. **Captures hors écran** : `formes`, `grille`, `premiers-pas`, `selection` (lot 3), `cache-differe` (lot 10) →
   `CAPTURES OK`.
4. **Banc hors écran** : `forme-page-annotee`, `grille-dessus` relevés ; résultat noté dans le fichier d'état.
5. `graphify update .`, commit de `graphify-out/`. Pas de version, pas de PR : la clôture du lot est à la fin de 4b.
