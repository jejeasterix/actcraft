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
| D4 | Groupes | Couche du groupe = la plus basse des membres ; rang = le plus haut ; Ctrl+G bascule (grouper si au moins 2 objets, dégrouper si un groupe) ; Ctrl+G puis Ctrl+Z vide la sélection. | défaut (ancien dépôt ; plan comparatif) ; Ctrl+G libre (`actions/data/commands.json` : seul Ctrl+Shift+G est pris, `grid.designer`, l. 130) | L5-T5 : groupe à cheval sur deux couches (voir A6) |
| D5 | Organiser | Avancer, reculer d'un cran parmi les objets de la même couche ; premier plan, arrière-plan ; changer de couche met au sommet de la couche d'arrivée. | défaut (ancien dépôt, F-513) | aucune |
| D6 | Aligner | Au moins 2 objets non verrouillés ; bords et centres sur l'union ; largeur, hauteur, angle, complet sur le premier créé. **Un objet seul (ou un seul groupe) : sans effet** — conséquence directe du défaut sourcé « au moins 2 objets » ; S8 aligne deux objets depuis l'amendement 1 (M12). L'alignement sur la page reste à présenter à Jérôme (H5-10). | défaut (ancien dépôt) ; tranché ici par cohérence avec ce défaut | L5-T6 (un objet seul : aucune étape) |
| D7 | Ajuster, taille initiale | Ajuster réduit seulement les objets plus grands que la page ; taille initiale = transformation sans échelle. | défaut (F-536) | aucune |
| D8 | Projeter, inverser, symétries | Inverser retourne l'objet, angle opposé ; projeter X : copie miroir au-dessus, Y : à gauche ; symétries autour de l'origine XY quand elle est affichée. | défaut (ancien dépôt ; P20 de L7-T0) | aucune |
| D9 | Verrouiller, masquer, glisser une copie | Verrouillé : ni déplacé ni groupé par un clic, seule la poignée « Déplacer » le déplace ; masqué : non rendu (mode Création au lot 6) ; glisser une copie : chaque glissement laisse l'original et dépose une copie. Le constat P1 (clic qui traverse un verrouillé) n'est pas contradictoire avec le déplacement, mais touche la désignation : non tranché (H5-12). | défaut (F-585, F-586, F-587) ; lot 3, P28 et P29 | aucune avant observation |
| D10 | Pot de peinture | Intérieur d'une forme fermée → remplissage ; contour → couleur du contour ; vide → fond de la page ; image → zone voisine, tolérance 5 sur 100 ; s'applique aux verrouillés ; intersection de formes : remplissage de la forme du dessus (H5-4). | défaut (F-235 ; plan comparatif) | aucune |
| D11 | Connecteur | 14 styles, ligne simple par défaut ; points d'attache aux sommets de la forme ; accrochage à moins de 20 px ; couche supérieure ; extrémités A et B : aucune, rayon, flèche. | défaut (F-245 ; plan comparatif) | aucune |
| D12 | Modification de stylo | Ligne libre, horizontale, verticale, carré, rectangle, cercle, ellipse, arc, ligne brisée, flèche, zigzag. Liste exacte à confirmer au PC. | défaut (F-221) | aucune |
| D13 | Points de forme | Huit points pour un carré (sommets et milieux) ; un point déplacé fait de la forme un polygone ; une forme intelligente garde ses points dédiés. | défaut (F-527) ; A16 conforme | aucune |
| D14 | Onglets (5b) | Onglets en haut par défaut (position réglable par `appTabs.position`, plan 5b T21) ; nom en gras pour l'actif ; astérisque si modifié ; croix ; « Sans titre » ; pages déposées sur un onglet copiées avant sa première page. | défaut (F-010 à F-012) | aucune |
| D15 | Export de page et PDF (5b) | BMP, JPEG, PNG, TIFF à la taille de la page en pixels (1024 × 768 par défaut) ; PDF : une page par page, à la taille de la page. | défaut (F-671, F-672, S8) | aucune |
| D16 | Import PDF (5b) | Document neuf, une page par page du PDF, page à la taille du PDF à 96 ppp, image verrouillée sur la couche du fond. | défaut (plan 5b T24) | aucune |
| D17 | Impression (5b) | Règle du nombre de feuilles du plan 5b (T26) ; notes ; page allongée entière, jamais coupée ; gris ; aperçu ; réglages gardés. | défaut (F-670, amendement 1 du cadrage § 2.2) | aucune ; le cadrage d'origine (N1-15, « Autres exclusions ») exclut la boîte d'impression : le plan s'appuie sur l'amendement 1 § 2.2 et DA-A2, dont le texte n'a pas été relu sur le Mac (non vérifié) |
| D18 | PowerPoint en images (5b) | Une diapositive = une page, en image de fond, par la chaîne de D16. | défaut (plan 5b) | aucune |
| D19 | Insérer une image ou un média | Au plus 80 % de la page, centrée, couche intermédiaire, nommée d'après le fichier ; média au centre. | défaut (ancien dépôt ; F-129) | aucune |
| D20 | Menu d'édition d'objet | Entrées et ordre de F-770 pour les fonctions livrées (lots 3 à 5 et 7) ; entrées des lots 6 et 8 absentes jusqu'à leur lot. | défaut (F-770) ; lot 3, P34 | aucune |
| D21 | Ce que compte `cacheRefreshCount` | **Rendus terminés.** La propriété existe déjà avec ce sens, sous ce nom : `Q_PROPERTY(int cacheRefreshCount … NOTIFY cacheUpdated)` (`ui/include/actcraft/ui/pageview.h:82`, `:180`), incrémentée **seulement** dans `PageView::notifyCacheUpdated` quand le redessin rend le cache prêt (`ui/src/pageview.cpp:360`) ; appelée par `PageArea.qml:91` (`onScheduledUpdateCompleted`). Un redessin fini pendant lequel une demande est arrivée relance sans compter (`pageview.cpp:352-357`) ; sans cache (`cacheEnabled` faux, option du banc), aucun compte (`pageview.cpp:314-320`). Les émissions de `cacheRefreshRequested` (`pageview.cpp:333`, `:356`) ne comptent pas. Tests et banc attendent la stabilité (`cacheReady`, puis `kCacheFoldIdleMs` + 300 ms sans redessin) et ne demandent jamais de redessin pendant un autre. **Aucune propriété neuve.** | recommandation de l'amendement 3 du plan ; code observé | L5-T19, L5-T20, L5-T31 (voir « Ce qui amende le plan », point 3) |

## API

Code lu à `ec3b210`. « Conforme » : nom et signature présents ; seules les lignes changent.

| # | API supposée par le plan | Code livré (fichier:ligne) | Conforme | Tâche à amender |
|---|---|---|---|---|
| A1 | `Tool`, `ToolContext` (8 membres), `DocumentEditor`, `LiftSurface` | `tools/include/actcraft/tools/tool.h:10` ; `toolcontext.h:20-33` (`page`, `pageIndex`, `execute`, `lift(ids)`, `drop(command)`, `isLifting`), `:37-45` (`show`, `setTransform`, `clear`), `:102-112` (`document`, `ink`, `strokes`, `lift`, `selection`, `view`, `commands`, `user`) | oui | — |
| A2 | `SelectionView`, `selectiongeometry.h` | `toolcontext.h:66-76` (`visible`, `corners`, `handles`, `bar`, `handlesVisible`, `rubberBand`, `smartPoints` en dernier, l. 74-75) ; `selectiongeometry.h` présent | oui | L5-T1 : `shapePoints` après `smartPoints` |
| A3 | `UserState`, `defaultSettings`, `ToolRegistry::add`, `registerBuiltinTools` | `tools/include/actcraft/tools/userstate.h:38` (`defaultSettings`), `:42-57` (`currentTool`, `settings`/`setSettings` en `InkSettings`, `selection`, plus `shapeSettings`, `textSettings`) ; `toolregistry.h:19`, `:29` ; `tools/src/toolregistry.cpp:66-80` (sept outils, noms du plan) | oui | L5-T1, L5-T17 : `UserState` ne garde que `InkSettings`, `ShapeSettings`, `TextSettings` ; la mémoire du style de connecteur n'a pas de place dans `UserState`, et `userstate.h` n'est pas dans les fichiers de T1 (supposé : `ToolHost` la garde, à fixer par T1) |
| A4 | `ToolHost` (`Tools`, `appTools`), `activate`, `selectionFrame`, `user()`, `settingsFile` | `ui/include/actcraft/ui/toolhost.h:37` (`QML_NAMED_ELEMENT(Tools)`), `:49`/`:79`, `:53`/`:83`, `:88`, `:104` ; `ui/qml/Main.qml:99` (`appTools`) | oui | — |
| A5 | `DocumentController` | `ui/include/actcraft/ui/documentcontroller.h` : `isModified` 72, `document` 79, `execute` 82, `showPage` 103, `openUrl` 127, `saveToUrl` 128, `liftObjects` 142, `dropLifted` 145, `isLifting` 146, `deleteObjects` 149, `ChangeOrigin` 157, `addChangeObserver` 173, signal `editAboutToApply` 214 ; `parkSession` / `resumeSession` absents (T1 les déclare) | oui | — |
| A6 | `ReplaceObjectsCommand`, `ObjectReplacement`, `addedIds`, placement | `core/include/actcraft/core/replace.h:14-18`, `:33-42` ; règle exacte (l. 21-25) : ajouts au rang du plus haut retiré **seulement s'ils sont de sa couche** ; sinon, ou sans retrait, au sommet de leur couche | oui, règle plus précise | L5-T5 : un groupe dont les membres sont sur deux couches prend la plus basse (D4) et va donc **au sommet de cette couche**, pas au rang du plus haut membre ; L5-T4 : un collage sans retrait va au sommet de la couche |
| A7 | `TransformObjectsCommand(page, ids, delta)` ; `RemoveObjectsCommand`, `AddObjectCommand`, `SetTranslucencyCommand` ; `TransformEachCommand` | `core/include/actcraft/core/objectedits.h:17-20`, `:33`, `:89` ; `edits.h:30` ; `TransformEachCommand` absent (T1 le réserve) | oui | — |
| A8 | `objectAt`, `objectsTouching`, `kHitToleranceScreen` | `core/include/actcraft/core/hittest.h:11` (6,0), `:22`, `:24` | oui | — |
| A9 | `liftChanges`, `changesAfterLift`, `restoreObjects` | `core/include/actcraft/core/lift.h:21`, `:23`, `:30` | oui | — |
| A10 | `PageCache`, `applyToCache`, `Change::Kind`, `foldPending`, `PendingLayerItem` | `core/include/actcraft/core/pagecache.h:18-31` (`insert(id)`, `remove(id, area)`, `invalidate(id, area)`), `:38` ; `undo.h:16-29` (plus `PageChanged`, `PageSettingsChanged`) ; `render/include/actcraft/render/fixedpageitem.h:31`, `:53`, `:76`, `:78`, `restart` `:146` ; `render/src/fixedpageitem.cpp:96` (`m_pendingIds.clear()`, et non 100) ; `render/include/actcraft/render/pendinglayeritem.h:22` | oui | — |
| A11 | `paintPage`, `renderPage`, `decodeImages`, `ImageLookup`, `PaintOptions` | `render/include/actcraft/render/pagepainter.h:26`, `:35` (`DecodedImages::lookup`), `:44`, `:72-73`, `:75-76` | oui | — |
| A12 | Disposition : sous-menus, `objectBar`, `contextMenu`, plateformes ; `argument`, `optional` | `actions/include/actcraft/actions/layout.h:16-21` (`MenuSpec`, un niveau), `:40` (`objectBar`), `:41-43` (`contextMenu` **et `contextSubmenus`**, ajouté au lot 4), `:47` (`parseLayout`, et non 42), `:54` (`unknownCommands`, et non 49) ; `actions/src/layout.cpp:41-72` (sous-menu déclaré **en ligne** `{"submenu", "label", "items"}`, rangé à part, jeton `>id`), `:88-89` (format exigé, et non 45) ; `commands.json` : 132 commandes, 25 `argument`, 4 `optional` | **écart** | **L5-T1, étape 4** : la règle du plan (« un `>id` d'`objectMenu` désigne un sous-menu déclaré dans `menus` ») diverge du patron du lot 4, où le menu contextuel déclare ses sous-menus en ligne (`contextSubmenus`, test `contextMenuAcceptsSubmenus`, `tests/tst_actions_layout.cpp:274`). Choisir avant V2 ; recommandation : même patron (`objectSubmenus` en ligne, même lecteur), tests de M2 réécrits en conséquence. `ui::Commands` expose `contextMenu` (`ui/include/actcraft/ui/commands.h:27`) : `objectMenu` à rendre de la même façon |
| A13 | `UiStepKind` `Pointer`, `DoubleClick`, `Touch`, `Pinch`, `expect` ; `typeText` | `bench/include/actcraft/bench/uiscenario.h:31` ; `Expect` 40, `Touch` 58, `Pinch` 59, `Pointer` 60, `DoubleClick` 62, `TypeText` 64 ; `typeText` 185 ; tolérance relevée sur `checkPixel` seulement (39, 79) | oui | — |
| A14 | Couches de `PageArea.qml`, `appTools`, `appCommands`, `ObjectBar.qml`, `PageContextMenu.qml` (`popupAt`) | noms du plan = `objectName` ; identifiants réels : `pageContent` 65, `pendingLayerItem` 99, `liftLayerItem` 108, `inkLayer` 117, `gridOverlayItem` 126 (lot 4), `pageOverlaysItem` 146, `selectionOverlayItem` 173, `toolCursor` 189 (`pointerAt`, 217-218), `pageContextMenu` 268 ; `Main.qml:57` (`appCommands`), `:86` (`appPages`), `:99` (`appTools`) ; `ui/qml/PageContextMenu.qml:30` (`popupAt`) ; `PageArea.qml:61` (`cache.scheduleUpdate()`, et non 30), `:91` (`view.notifyCacheUpdated()`, et non 60) | oui (noms par `objectName`) | L5-T2, L5-T18 : viser les couches par `objectName` dans les tests, par les identifiants réels dans le QML ; lignes de `PageArea.qml` |
| A15 | `pageedits`, `PagesController`, `PageNavigator.qml` | `core/include/actcraft/core/pageedits.h:22` (`withFreshIds`), `:31`, `:45`, `:63-66` (`InsertPagesCommand`) ; `ui/include/actcraft/ui/pagescontroller.h:22` (`Pages`), `:44-48` (`copySelection`, `cutSelection`, `paste`, `duplicateSelection`, `deleteSelection`), presse-papiers **interne** et privé (`:58`, `:83`) ; `ui/qml/PageNavigator.qml` présent | oui | L5-T14 (D3) : ces commandes agissent sur la **sélection du navigateur**, pas sur la page affichée : sélectionner d'abord la page affichée (`focusPage`, `:40`) ou ajouter un membre ; le presse-papiers des pages n'est pas celui du système |
| A16 | Catalogue des formes, points intelligents, `SetShapeParametersCommand` | `core/include/actcraft/core/shapegeometry.h:43-44`, `:61`, `:65` (`smartPoints`), `:69` ; `shapeedits.h:13` | oui | — |
| A17 | `recognition.convert-to-shape`, `recognition.convert-to-text` ; `PagePresentation::hasOrigin`, `origin` | `actions/data/commands.json:76-77` (le second sous Windows) ; `core/include/actcraft/core/model.h:472`, `:479`, `:480` (et non 469-470) | oui pour les noms ; **le lot 7 n'est pas fusionné** (voir « Tests au départ ») : commandes déclarées par le socle, sans le code du lot 7 | L5-T6 (symétries d'origine), L5-T18 (`convertEntriesOnlyForAnnotations`) : vérifier ce qui répond à ces commandes au moment de leur vague |
| A18 | `PageView` : `tools`, `sceneToPage`, `addInputCover`, `cacheRefreshCount` | `ui/include/actcraft/ui/pageview.h:81`/`:174`, `:195`, `:187`, `:82`/`:180` ; `cacheReady` 62/132, `notifyCacheUpdated` 162, signal `cacheRefreshRequested` 205 ; `ui/src/pageview.cpp:305-334` (`requestCacheRefresh`), `:348-362` (`notifyCacheUpdated`) | oui | voir D21 |
| A19 | `PageThumbnails` | `ui/include/actcraft/ui/pagethumbnails.h:26`, `:68-71` (`pendingCount`, `renderCount`, `lastCopyMs`, `launchCount`) | oui | — |
| A20 | Copie de `Document`, ressources SHA-256, `Page::notes` | `core/include/actcraft/core/document.h:34`, `:64-69` (`addResource` → clé SHA-256), aucun constructeur de copie supprimé (copie implicite, supposé) ; `model.h:511` (`notes`, et non 434) | oui | — |
| A21 | `object.enlarge`, `object.reduce`, `edit.delete`, `edit.select-all` ; `CommandRegistry::find`, `ids` ; sous-menu imbriqué refusé | `commands.json:44-45`, `:32-33` ; `actions/include/actcraft/actions/commandregistry.h:69-70` (et non 68-69) ; `layout.cpp:38` (`InvalidSubmenu`), test `nestedSubmenuRefused` (`tst_actions_layout.cpp:174`) | oui | — |

**Noms de l'amendement 3.** `parseLayout` (`layout.h:47`), `unknownCommands` (`:54`, fonction libre), format exigé
(`layout.cpp:88-89`), `TstActionsLayout` (`tests/tst_actions_layout.cpp:31`), `kLayout` (`:13`), aucune aide
`loadLayout` / `loadRegistry` : conformes, lignes décalées. `PageArea.qml` : `:61` et `:91`. `bench/src/runner.cpp` :
l. 212 n'est que le versement (`pendingCount() > 0`) ; l'attente `pendingCount() == 0 && cacheReady` est l. 217-218
(et 413, 572). `.gitattributes` : aucune règle `*.pdf` (conforme, T30 l'ajoute).

## Tests au départ du lot

- **Le lot 7 n'est pas fusionné à `ec3b210`** : aucun merge `L7-` dans l'historique (241 merges lus), aucun
  `tests/presentation.cmake`, aucun test de présentation, de maths ni de reconnaissance. Donc **n_B = 0**, alors que
  le plan suppose n_B ≥ 16 et fait s'arrêter L5-T1 sur cet écart.
- Recompte statique (indicatif : conditions de plateforme et boucles non dépliées) : 135 appels d'enregistrement
  (`tests/bench.cmake` 52, `ui.cmake` 21, `core.cmake` 17, `render.cmake` 13, autres 32). `ctest -N` non lancé (le
  Mac ne construit pas pour cette tâche) : N_départ à mesurer sous Windows par L5-T1.

## Ce qui amende le plan (pour l'orchestrateur)

1. **Avant V2 — point de départ.** Lot 7 non fusionné : soit attendre sa fusion, soit amender N_départ
   (= N mesuré, n_B = 0) et la cellule de preuve de L5-T1. Sans cela, L5-T1 s'arrête à son étape 1.
2. **L5-T1, étape 4** (A12) : choisir le patron des sous-menus d'`objectMenu` (en ligne comme `contextSubmenus`,
   recommandé, ou recherche dans `menus` comme le plan) ; corriger les lignes citées (`layout.h:47`, `:54`,
   `layout.cpp:88`). Également : `shapePoints` après `smartPoints` (A2) ; place de la mémoire du style de connecteur
   (A3 : `ToolHost` ou `userstate.h`, à ajouter aux fichiers si c'est `UserState`).
3. **L5-T19, L5-T20, L5-T31** (D21) : compter par l'écart de `PageView::cacheRefreshCount()`, cache actif. **Ne pas
   reprendre le patron du banc** : son `cache.refreshes` et le critère S36 `cache_refreshes` comptent les
   **demandes** (`cacheRequestNs.size()`, `bench/src/result.cpp:138`, `bench/src/verdict.cpp:104`). Cas qui les
   distingue : une demande pendant un redessin → 2 demandes, 1 rendu compté. `refreshes_by_gesture` porte donc un
   autre nom de source que `cache.refreshes`, qui reste tel quel (incohérence consignée, non corrigée ici).
4. **L5-T5** (A6, D4) : groupe sur deux couches placé au sommet de la plus basse ; écrire l'attendu en conséquence.
5. **L5-T14** (A15, D3) : sans sélection d'objet, sélectionner la page affichée dans `appPages` avant ses commandes
   de pages.
6. **L5-T2, L5-T18** (A14) : couches visées par `objectName` ; lignes de `PageArea.qml` (`:61`, `:91`).
7. **L5-T6, L5-T18** (A17) : le code du lot 7 manque tant qu'il n'est pas fusionné.
8. **Constats P4 et P5** (lot 4, C4 et C5) : aucune tâche du plan ne les porte ; `L5-C<n>` ou lot 6, à trancher.
9. **L5-T6** (D6) : un objet seul (ou un seul groupe) : aucune étape.

## Hypothèses nouvelles ou modifiées

- **H5-10** — Aligner un objet seul : sans effet (D6), l'alignement sur la page reste à présenter à Jérôme.
- **H5-11** — Ouverture faite sur le Mac sans corpus, ancien dépôt ni binaire : D1 à D21 au défaut du plan ;
  confrontation aux vidéos et à REA, relevé `lot5.md`, `NOMS OK` et titres du corpus reportés au PC.
- **H5-12** — Clic qui traverse un objet verrouillé (P1) : à observer ; s'il se confirme, la désignation de L5-T16
  change (poignée « Déplacer » atteinte autrement).
- **H5-4** — Pot de peinture sur une intersection de formes : forme du dessus (D10), inchangé.
