# Lot 6 — principes, décisions D1 à D14 et API revérifiées (ouverture sur le Mac)

Tâche L6-T0, scénario S30. Base du relevé : `06a3f8f` (`feat/livraison-complete`, « lot 6 lancé sur le Mac » :
socle L0-S, lots 1 à 5 et 10 fusionnés ; **lot 7 non fusionné**, voir A15). Constats écrits par nous. Références
`Vnn` : numéros des fiches du corpus (table hors dépôt), reprises seulement des rapports déjà commités. Relevé brut
hors dépôt (`…\docs\re\lots\lot6.md`) : **non écrit, reporté au PC**.

## Méthode et limites

- **Poste.** Relevé fait sur le Mac mini, qui n'a ni l'ancien dépôt, ni le corpus des vidéos, ni le binaire 3.5.18,
  ni l'outil de contrôle des noms. Même conduite qu'au lot 5 (H5-11), reprise ici en H6-11.
- **Étapes 1 à 3 du plan** (corpus, ancien dépôt et son graphe, REA en statique) : non faites. Chaque constat et
  chaque décision porte la mention « à confronter au PC ».
- **Décisions.** Chacune prend le **défaut du plan** (6a ou 6b) ou du cadrage. Elle n'est tranchée autrement que si
  une source **de ce dépôt** (code, `docs/`, rapports des lots précédents) contredit le défaut ou suffit à trancher.
- **API.** Revérifiées dans le code de ce worktree (en-têtes, sources, QML, données), noms et lignes relevés à
  `06a3f8f`. Les numéros de ligne du plan sont indicatifs ; ceux de ce rapport font foi.
- **Non fait sur le Mac (reporté au PC).** Relevé brut `lot6.md` ; README du corpus et fiches ; ancien dépôt et son
  graphe (fichiers de l'étape 2 du plan, spécification) ; REA en statique (boîtes de profils et de réglages, valeurs
  des cinq profils, fonctions du verrouillage, chaînes du correcteur) ; `NOMS OK` ; contrôle des titres du corpus.
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
| P3 | Le curseur du stylo se dessine au-dessus (défaut), en dessous ou pas du tout. | lot 3, P31 (V50) ; F-201 ; lot 10, P17 | réglage à trois valeurs, défaut « au-dessus » | L6-T27 (section « Outils », supposé) | — | renvoyé au lot 6 par le lot 3 ; aucune ligne du plan ne le nomme : à écrire dans le schéma de L6-T27 |
| P4 | Un réglage regroupe les traits successifs en un seul objet. | lot 10, P10 (spécification : inactif) ; lot 3, P41 (binaire : **actif**) | réglage, défaut à trancher | L6-T27 (plan 6b, l. 526) | — | contradiction non levée (H3-8) ; D14 |
| P5 | Un troisième réglage de qualité du rendu vise l'édition d'objets (déplacer, redimensionner). | lot 10, P2 (binaire) | réglage à ajouter à la spécification | L6-T27 (plan 6b, l. 524-526) | — | D14 ; défaut de la référence non relevé |
| P6 | Le mode Création libère les objets verrouillés et montre les masqués. | lot 3, P28 (V114, V103) ; lot 5, D9 | D6-12 | L6-T22 | S9.8 | conforme au plan ; D11 |
| P7 | Une forme neuve s'appelle « Forme » suivi d'un numéro compté par page ; le navigateur d'objet l'affiche. | lot 4, P7 (V130), H4-11 | nom affiché dans le navigateur d'objet | L6-T14 | S9.3 | D10 |
| P8 | Le tracé depuis le centre est, dans l'original, un réglage du profil. | lot 4, P9, D5 | aucune : notre palette garde sa bascule | — | — | écart déclaré au lot 4, maintenu (D1 : le profil ne porte pas ce réglage) |
| P9 | Le panneau des navigateurs offre, dans l'ordre : pages, ressources, objets, notes, propriétés, actions ; il peut flotter. | lot 4, P30, D12 (V113, V25) ; F-040 à F-042 | ordre des navigateurs ; panneau flottant | L6-T2 (`browserTabsInOrder`), L6-T15 | S9.3 | flottant renvoyé au lot 6 par le lot 4 (D12) ; noms réels : voir A8 |
| P10 | Le fond de page se choisit aussi dans les ressources. | lot 4, P35, D15 (V68) | fond depuis la bibliothèque | L6-T19 à L6-T21 | S9.6 | D13 |
| P11 | Une couleur choisie quand des textes sont sélectionnés recolore leur texte. | lot 4, C4 (V103) ; lot 5, P4 | couleur appliquée aux textes sélectionnés | — | — | laissé à l'orchestrateur par le lot 5 ; aucune tâche du lot 5 ne l'a pris (`L5-C4` traite le redessin) ; proposé en L6-P1 |
| P12 | Les gestes du toucher se coupent par un réglage. | lot 3 (hypothèses : réglages du toucher, lots 6 et 11) | réglage du toucher | L6-T27 (section « Saisie », supposé) | — | ou lot 11, à trancher par l'orchestrateur |
| P13 | Le menu d'édition d'objet suit l'ordre de F-770 ; les entrées d'un lot n'apparaissent qu'avec lui. | lot 5, D20 (F-770) | entrées du lot 6 insérées dans `objectMenu` (`actions/data/layout.json:39`) | — | — | aucune ligne des deux plans ne touche `objectMenu` ; proposé en L6-P2 |

## Décisions

Sauf mention contraire : **défaut du plan, à confronter au PC.**

| # | Question | Décision | Source | Tâche amendée |
|---|---|---|---|---|
| D1 | Réglages de chaque profil fourni | `preparation` (défaut) : navigateur de pages ouvert et épinglé, palette de couleurs étendue. `en-classe` : page ajustée, navigateurs masqués, corbeille visible, `pinned` = `browser.resources` puis la présentation de la liste de L7-T32. `lecture-ecriture` : clavier à l'écran, reconnaissance d'écriture, message défilant, `spell.check`. `calcul-geometrie` : `math.*`, `tool.dice`, `tool.calculator`. `son-image` : caméra **`tool.camera-*`** (D10 du lot 7) et `insert.image`. Les quatre autres que `preparation` : page ajustée, navigateurs masqués, corbeille visible, 16 pastilles. Aucun ne porte le tracé depuis le centre (P8). | D6-2 ; L6-T6 (plan, l. 1855-1860) ; D10 du lot 7 | L6-T6 (identifiants de la caméra ; liste de L7-T32 absente, A15) |
| D2 | E4 : taille de page par défaut | 1024 × 768 dans tous les profils (décision 15). Taille de l'écran pour `en-classe` : **non tranché** (exige la 3.5.18). Le code sait faire les deux : `file.new` → `newDocument(1024, 768)`, `file.new-screen` → taille de l'écran (`ui/qml/Main.qml:543-544`, observé). | D6-13 ; code | aucune |
| D3 | Verrou de classe : commandes libres | Libres (`lockable: false`) : `file.quit`, le déverrouillage (L6-T31), l'aide, `page.next`, `page.previous`, `view.fullscreen`, `view.fit`, `view.zoom`. Toutes les autres verrouillables. `restrictable: true` : `edit.copy`, `edit.cut`, `edit.duplicate`, `file.save`, `file.save-as`, `file.print`, `export.*` et la caméra **`tool.camera-*`**. Écart interne au plan : H6-7 ne cite ni `view.fit` ni `view.zoom`, L6-T3 (l. 1000) les cite ; la liste de L6-T3 est retenue. | H6-7 ; L6-T3 ; D10 du lot 7 | L6-T3 (caméra) |
| D4 | E3 : bouton d'édition des raccourcis ; suppression | Bouton d'édition en bas de la barre de raccourcis, visible si la palette n'est ni réduite ni réduite au maximum ; suppression par « retirer » dans l'onglet de l'éditeur de profils. | plan 6b, L6-T30 (l. 677-680) | aucune |
| D5 | Correcteur : options, parcours, dictionnaire de l'utilisateur | Défaut du plan 6b (L6-T32 à L6-T34). Hunspell 1.7.2 accordé (2026-10-18) ; dictionnaire de Grammalecte (MPL 2.0) traité comme couvert (H6-6) ; **licence de Hunspell : MPL 1.1** (Jérôme, 2026-10-21, H6-6b) : bibliothèque statique, aucun fichier de plus à livrer. | H6-6, H6-6b | L6-T32 n'attend plus (H6-6 et H6-6b réglées) |
| D6 | Grille étendue de couleurs | 36 couleurs (S9.2) ; ordre du plan (L6-T8) ; une couleur choisie s'ajoute à la palette et reste après relance. Les pastilles de base existent : `Commands::colors` (`ui/include/actcraft/ui/commands.h:31`, lues de `layout.json`). | S9.2 ; L6-T8 | aucune |
| D7 | Repli d'une palette désépinglée | Repli 600 ms après la sortie du pointeur (`HoverHandler` + `Timer`), animation 120 ms ; marge : non relevée. | L6-T7 (plan, l. 1972-1973) | aucune |
| D8 | Tableau d'accueil | Documents récents ; ouvrir le premier ; F11 le rouvre ; entrées exclues absentes. | S9.9 ; plan 6b (L6-T23, L6-T24) | aucune |
| D9 | Sections de propriétés par type | Trait : Identification, Apparence, Contour, Position ; rien de sélectionné : propriétés de la page ; autres types : défaut du plan (L6-T10, L6-T11). | S9.4 | aucune |
| D10 | Colonnes et icônes du navigateur d'objet | Quatre nœuds de couche ; objets du plus haut au plus bas ; colonnes « masquer » et « verrouiller » ; nom « Forme n » (P7). | S9.3 ; lot 4, P7 | L6-T14 : verrouiller n'émet aucun `Change` (A9) |
| D11 | Mode Création | Bascule F2, neuf réglages ; objets verrouillés modifiables, masqués visibles en translucide, accrochage désactivé, effets de page suspendus ; réglages des actions et des restrictions publiés pour le lot 8. | D6-12 ; lot 3, P28 ; lot 5, D9 | aucune |
| D12 | Étiquette | Au survol, style réglé ; sans légende, rien. | S9.10 | aucune |
| D13 | Tampon ; modèle, fond, grille depuis les ressources | Tampon actif : un clic, une copie ; glisser une ressource l'insère ; fond, grille et modèle de page : défaut du plan 6b (L6-T19 à L6-T21). | S9.6 ; D6-14 ; lot 4, D15 | aucune |
| D14 | Troisième réglage de qualité ; regroupement des traits | Haute qualité pendant l'édition d'objets : **faux** par défaut, même règle que pendant l'écriture (supposé ; défaut de la référence non relevé, lot 10, P2). Regroupement automatique : **faux** par défaut (spécification ; plan 6b, l. 526), **non tranché** : le binaire le dit actif (lot 3, P41). | lot 10, P2, P10 ; lot 3, P41 ; L6-T27 | L6-T27 (défaut de l'édition) |

## API

Code lu à `06a3f8f`. « Conforme » : nom et signature présents ; seules les lignes changent.

| # | API supposée par le plan | Code livré (fichier:ligne) | Conforme | Tâche à amender |
|---|---|---|---|---|
| A1 | `Commands::setProfileDir`, `reload`, `actions::readLayeredFile` | `ui/include/actcraft/ui/commands.h:41` (`setProfileDir(const QString&)`), `:44` (`Q_INVOKABLE reload()`) ; `actions/include/actcraft/actions/layout.h:52` (`readLayeredFile(fileName, profileDir, resourceDir, …)`) | oui | — |
| A2 | `CommandInfo`, `loadDeclarations`, `isEnabled`, `trigger` | `actions/include/actcraft/actions/commandregistry.h:42`, `:66` (`bool loadDeclarations(const QByteArray&, QString& error)`), `:76` (vrai par défaut, faux pour une inconnue), `:84` (`trigger(id, argument, Platform)`) ; `isOwnEnabled` absent (L6-T3 l'ajoute) | oui | — |
| A3 | `addChangeObserver`, `ChangeOrigin` | `ui/include/actcraft/ui/documentcontroller.h:169` (`ChangeOrigin`), `:184` (`ChangeObserver = std::function<void(const core::Changes&, ChangeOrigin)>`), `:185` (`int addChangeObserver`, jeton > 0), `:186` (`removeChangeObserver(int)`) | oui | — |
| A4 | `execute`, `document()`, `currentPageIndex()`, signal `currentPageChanged` | `documentcontroller.h:82` (`execute`), `:79` (`document()`) ; **`currentPageIndex()` absent** : `Q_PROPERTY(int currentIndex … NOTIFY pagesChanged)` `:39` ; **`currentPageChanged` absent** : `pagesChanged()` `:209` (une fois par changement de page affichée ou du nombre de pages), aussi `pageAboutToChange()` `:206`, `pageContentChanged(int page)` `:229`, `showPage(int)` `:103` | **écart** | L6-T16 (l. 2749 : « suit `currentPageChanged` » → `pagesChanged` + `currentIndex`) ; ligne A4 de la table, lue par L6-T9, L6-T11 à L6-T15, L6-T19, L6-T21 |
| A5 | `UserState::selection`, `ToolHost::user()`, `ToolHost::setSelection(QList<quint64>)`, `selectionChanged` | `ui/include/actcraft/ui/toolhost.h:121` (`tools::UserState& user(input::UserId = input::kFirstUser)`) ; `tools/include/actcraft/tools/userstate.h:52` (`setSelection(std::vector<core::ObjectId>)`) ; **`ToolHost::setSelection` absent** : `selectObjects(const std::vector<core::ObjectId>&)` `toolhost.h:109` (C++ seulement, pas `Q_INVOKABLE`), `selectAll()` `:106`, `clearSelection()` `:107` ; signal `selectionChanged()` `:141` | **écart** | L6-T14 (le clic du navigateur sélectionne par `ToolHost::selectObjects`, exposé à QML par une méthode `Q_INVOKABLE` du modèle), L6-T15, L6-T12 |
| A6 | `PageView::contactActive`, `contactActiveChanged` | `ui/include/actcraft/ui/pageview.h:348` (`Q_PROPERTY(bool contactActive READ contactActive NOTIFY contactActiveChanged)`), `:351`, `:354` | oui | — |
| A7 | `PageView::addInputCover(QQuickItem*)` | `pageview.h:187` (`Q_INVOKABLE`) | oui | — |
| A8 | `BrowserPanel.qml` : `current`, côté, largeur, `view.browsers` | `ui/qml/BrowserPanel.qml:13` (`objectName: "browserPanel"`), `:18` (`side`, « left » ou « right »), `:21` (`panelWidth`, bornes `minWidth` 160 `:19`, `maxWidth` 480 `:20`), `:66-73` (`readonly property var navigators`, objets `{id, label, icon, enabled}`, identifiants **en français** : `pages`, `ressources`, `objets`, `notes`, `proprietes`, `actions` ; seul `pages` actif), `:87` (icônes `navigator-<id>`), `:58-60` (`view.browsers` est une commande cochée, pas une propriété). **Ni `current` ni `tabs`.** | **écart** | L6-T2 (l. 781-782 : `property("tabs")` et identifiants anglais ; l. 814 : `browserPanel.current`), L6-T15 (déclare `current`), L6-T4 (l. 1192 : `browsers.current = "objects"`), L6-T6 (l. 1825), L6-T18 (l. 2829, 2860 : `browserPanel.current=objects`). Recommandation : garder les identifiants livrés (`objets`, …) partout, `navigators` pour l'ordre |
| A9 | `arrangedOrder`, `ReorderObjectsCommand`, `SetLayerCommand`, `ObjectFlag`, `SetObjectFlagCommand`, commandes `arrange.*`, `object.hide`, `object.lock` | `core/include/actcraft/core/arrange.h:21`, `:29`, `:43` ; `core/include/actcraft/core/objectflags.h:9-14` (`Locked`, `Hidden`, `DragCopy`), `:15` (`flagOfAll`), `:21` ; `actions/data/commands.json:143-144`, `:147-153` | oui | **L6-T14, L6-T12** : `SetObjectFlagCommand` n'émet **aucun `Change` pour `Locked` et `DragCopy`** (`objectflags.h:17`) : un modèle qui ne suit que `addChangeObserver` ne voit pas un verrouillage ; relire aussi `documentChanged` ou la page après chaque étape |
| A10 | `ClipboardPayload`, `encodeClipboard`, `decodeClipboard`, `resourcesFor`, `kObjectsMimeType` | `formats/include/actcraft/formats/clipboard.h:15`, `:17-25` (`objects`, `resources`, **plus `extensions`**, ajout de L5-T4), `:29`, `:30` (`std::optional`), `:31-32` (`resourcesFor(const core::Document&, const std::vector<core::PageObject>&)`) | oui | L6-T9, L6-T19 : remplir `extensions` (sinon « bin ») |
| A11 | `insert.image` (URL facultative) | `commands.json:180` (`"argument": "url", "optional": true`) | oui | — |
| A12 | `ReplaceObjectsCommand`, `ObjectChanged`, `PageChanged`, `PageSettingsChanged` | `core/include/actcraft/core/replace.h:33` ; `core/include/actcraft/core/undo.h:22`, `:25`, `:26` (aucun pixel du cache) | oui | — |
| A13 | `render::thumbnailPng(page, width)` ou `paintPage` | `render/include/actcraft/render/pagepainter.h:79` (`thumbnailPng(const core::Page&, const ImageLookup&)` : **sans largeur**, `kThumbnailWidth` fixe), `:72-73` (`paintPage(QPainter&, const core::Page&, const ImageLookup&, const PaintOptions& = {})`), `:75-76` (`renderPage(page, images, double scale, options)` → `QImage`) | **écart** | L6-T20 : vignette à largeur choisie par `renderPage(page, images, largeur / largeur de la page)` sur `QThreadPool`, `ImageLookup` à fournir |
| A14 | étapes `command`, `expect`, `pointer`, `typeText`, `waitReady` | `bench/include/actcraft/bench/uiscenario.h:33` (`UiStepKind`), `Command` `:35`, `WaitReady` `:39`, `Expect` `:42`, `Pointer` `:62`, `TypeText` `:66` ; `above` `:107` (lu strictement supérieur, L4-T30) | oui | — |
| A15 | commandes du lot 7 et fichier des épinglages de L7-T32 | `commands.json` (déclarées par le socle, L0-S0) : `view.page-zoom` `:46`, `present.*` `:55-60`, `math.*` `:61-65`, `tool.dice` `:66`, `tool.calculator` `:67`, `tool.clock` `:68`, **`tool.camera-area`, `-polygon`, `-freehand`, `-window`, `-screen` `:78-82`**, `desktop.annotate`, `.tools`, `.restore` `:84-86` ; **aucun `camera.*`**. Lot 7 **non fusionné** à `06a3f8f` (aucun merge `L7-`, travail sur `feat/livraison-complete-b`) ; `docs/verifications/*-lot7-epinglages.md` **absent** | **écart** | L6-T6 (épinglages de `en-classe` et `son-image` : liste absente, filtrée à l'affichage selon D6-2 ; le test ne doit pas exiger le fichier), L6-T3, L6-T9 (caméra, voir plus bas) |
| A16 | `ui::DocumentIo` : fin d'enregistrement avec le chemin, ouverture par URL | **`DocumentIo` absent.** `documentcontroller.h:127` (`openUrl(const QUrl&)`), `:128` (`saveToUrl(const QUrl&)`, fil secondaire), `:222` (`loadFinished(bool ok, const QString& message)`), `:223` (`saveFinished(bool ok, const QString& message)`) : **sans chemin** ; le chemin est `fileUrl` (`Q_PROPERTY(QUrl fileUrl … NOTIFY fileUrlChanged)` `:44`) | **écart** | L6-T23 (récents : à `loadFinished` / `saveFinished` vrais, lire `fileUrl`), L6-T24 (résumé à l'enregistrement : `saveFinished`) |
| A22 | `ArgumentKind::Text`, `"argument": "text"` | `commandregistry.h:39` ; `commands.json` : 12 commandes (`:48-54`, `:90`, `:96`, `:116-117`, `:135`, `:177`, `:179`, `:184`) | oui | — |
| A23 | `SetTranslucencyCommand` | `core/include/actcraft/core/objectedits.h:92` (`(std::size_t page, std::vector<ObjectId> ids, std::uint8_t value)`, 0 opaque … 255 invisible), `:95-96` (`mergeId`, `mergeWith`) ; fusion : même famille, mêmes objets ; la fermeture de la réglette clôt l'étape par `UndoStack::closeMerge()` (`:85-88`) | oui | L6-T11, L6-T12 : `closeMerge()` au relâché de la réglette |

## Identifiants de la caméra (D10 du lot 7)

L'orchestrateur du lot 7 garde `tool.camera-*` ; `camera.*` n'existe pas (`commands.json:78-82`). Endroits à
corriger, tous dans le plan 6a (aucun dans 6b) :

- l. 363, table A, ligne A15 : `camera.*` → `tool.camera-*` ;
- l. 1003, L6-T3, étape 4 : `restrictable: true` pour `camera.*` → `tool.camera-*` ;
- l. 1858, L6-T6 : épinglages de `son-image` → `tool.camera-*` ;
- l. 2161, L6-T9 : préfixe `camera.` de la règle de l'historique → retiré (le préfixe `tool.` couvre déjà `tool.camera-*`).

## `appCommands` n'est pas un objectName

`ui/qml/Main.qml:57-58` : `id: appCommands`, `objectName: "commands"`. L'identifiant ne vaut que dans `Main.qml` ; les
tests trouvent l'objet par `findChild<ui::Commands*>(QStringLiteral("commands"))` (`tests/tst_ui_shell.cpp:493`,
`:513`, et six autres fichiers) ; aucun fichier QML autre que `Main.qml` ne nomme `appCommands` : chaque composant
reçoit une propriété `commands` (`ui/qml/BrowserPanel.qml:17`, `Main.qml:101`, `:245`…).

À corriger :

- 6a l. 764 et 786, L6-T2 : aide `appCommands()` supposée « déjà présente » dans `tst_ui_shell.cpp` : elle n'existe
  pas ; patron du fichier : `s.window->findChild<ui::Commands*>(QStringLiteral("commands"))` (de même, `loadMain()`
  n'existe pas : `Shell`, `start`, amendement 3) ;
- 6b l. 726, L6-T31 : `LU appCommands.lockedCount>0 OK` → `commands.lockedCount` (étape `expect` avec `above`,
  `uiscenario.h:107`, si le joueur ne lit pas `>`) ;
- 6a l. 1975 et 1977, L6-T7 : `appCommands.isAvailable` dans `ToolPalette.qml`, `appCommands.setChecked` dans
  `PaletteFrame.qml` → la propriété `commands` du composant ;
- 6a l. 2042 et 2079, L6-T8 : `baseColors: appCommands.colors` écrit dans `ColorSwatches.qml` → `commands.colors`.

Justes (identifiant employé dans `Main.qml`) : 6a l. 57, 63, 64, 813, 1692-1693, 1717-1718, 2187, 2542, 2689, 2770 ;
6b l. 307, 773.

Même famille, observé : l'instance `ProfileController { id: appProfile; … }` (6a l. 1693, 1718, L6-T5) n'a pas
d'`objectName`, alors que `expect appProfile.paletteDock` (l. 1978, L6-T7), `-Lire "appProfile.paletteDock=left"`
(l. 2829, L6-T18) et `lot6ControllersWired` (amendement 3, N2) la cherchent par son nom : ajouter
`objectName: "appProfile"` à L6-T5.

## N6

Commande Mac (configuration seule, sans construction, hors jeton) :

```
cd <wt> && cmake -S . -B build-mac -G Ninja -DCMAKE_PREFIX_PATH=$(brew --prefix qt) -DCMAKE_BUILD_TYPE=Debug -DACTCRAFT_BUILD_BENCH=ON > .tmp/cfg.txt 2>&1 && ctest --test-dir build-mac -N | tail -1
```

Résultat à `06a3f8f` : `Total Tests: 164`. **N6 = 164** (Mac, banc actif). À recompter sous Windows (`ctest --preset
debug -N`) : un test propre à une plateforme peut faire varier le total.

## Tâches de principes proposées

- **L6-P1 — Couleur choisie appliquée aux textes sélectionnés (P11).** Si des objets texte sont sélectionnés, la
  couleur de la palette (`pen.color`) recolore leur texte en une étape (`core/textedits.h`). Fichiers (supposé) :
  `ui/src/toolhost.cpp` (`setColor`), `tests/tst_ui_tools.cpp`. Scénario : S9.4 (sous-scénario « couleur sur
  sélection »). Condition : geste confirmé au PC (V103).
- **L6-P2 — Entrées du lot 6 dans le menu d'édition d'objet (P13).** Ouvrir les propriétés de l'objet (6a), l'ajouter
  à « Mes ressources » (6b), à leur place de F-770. Fichiers : `actions/data/layout.json` (`objectMenu`, l. 39),
  `tests/tst_actions_layout.cpp`. Scénario : S9.4 (propriétés ouvertes depuis le menu de l'objet). Ordre exact à
  relever au PC.

## Ce qui amende le plan (pour l'orchestrateur)

1. **Caméra** (D10 du lot 7) : 6a l. 363, 1003, 1858, 2161 (section ci-dessus).
2. **`appCommands`** : 6a l. 764, 786, 1975, 1977, 2042, 2079 ; 6b l. 726 ; `objectName: "appProfile"` à L6-T5.
3. **A4** : L6-T16 et la ligne A4 (`currentIndex`, `pagesChanged`).
4. **A5** : L6-T12, L6-T14, L6-T15 (`ToolHost::selectObjects`, C++ seulement).
5. **A8** : L6-T2, L6-T4, L6-T6, L6-T15, L6-T18 (`navigators`, identifiants en français, `current` à déclarer).
6. **A9** : L6-T12, L6-T14 (verrouiller n'émet aucun `Change`).
7. **A13** : L6-T20 (`renderPage` à l'échelle voulue).
8. **A15** : L6-T6 (lot 7 non fusionné, liste de L7-T32 absente).
9. **A16** : L6-T23, L6-T24 (`DocumentController`, `saveFinished`, `fileUrl`).
10. **D3** : liste de L6-T3 retenue (`view.fit`, `view.zoom` libres) ; H6-7 à aligner.
11. **D5** : L6-T32 n'attend plus (H6-6, H6-6b « MPL 1.1 »).
12. **D14** : L6-T27, défaut de l'édition d'objets (faux).
13. **P3, P12** : réglages du curseur du stylo et du toucher à écrire dans le schéma de L6-T27, ou renvoyés au lot 11.

## Hypothèses nouvelles ou modifiées

- **H6-11** — Ouverture faite sur le Mac sans corpus, ancien dépôt ni binaire : D1 à D14 au défaut du plan ;
  confrontation aux vidéos et à REA, relevé `lot6.md`, `NOMS OK` et titres du corpus reportés au PC.
- **H6-6** — Dictionnaire de Grammalecte (MPL 2.0) traité comme couvert par l'accord du 2026-10-18.
- **H6-6b** — Licence de Hunspell : MPL 1.1 (Jérôme, 2026-10-21).
- **H6-12** — Haute qualité pendant l'édition d'objets : faux par défaut (D14), à confronter au PC.

## À confronter au PC

Identification des 17 constats et confrontation de P1 à P13 ; D1 à D14, en priorité D2 (taille de l'écran pour
`en-classe`), D3 (fonctions grisées par le verrou), D4, D5 (chaînes du correcteur), D14 (regroupement : binaire actif
contre spécification inactive) ; relevé brut `lot6.md` ; `NOMS OK` ; contrôle des titres du corpus ; N6 sous Windows.

## Confrontation au PC (2026-11-06)

Faite au PC avec le corpus (README et 26 fiches), l'ancien dépôt (spécification, documents de rétro-ingénierie) et
les extractions statiques du binaire 3.5.18 déjà faites par L4-T0 (réglages embarqués, profils fournis, magasin des
commandes, propriétés, traductions). **Binaire non interrogé** : serveur REA coupé, aucune observation à l'écran.
Relevé brut hors dépôt : `…\docs\re\lots\lot6.md`. Rien n'a été modifié dans ce dépôt ; le code livré a seulement
été lu. Le lot 6 étant déjà livré, chaque amendement ci-dessous est une **correction** du livré.

### Les 17 constats (identifiés par élimination)

Retirés : ceux que nomment les lots 3, 4, 5, 7 et 8, les imports (packs, présentation) et l'exclusion. Restent :

| # | Constat (nos mots) | Source | Exigence pour ActCraft | Tâche | Scénario | État du livré ou écart |
|---|---|---|---|---|---|---|
| C1 | Survoler un bouton de la palette, à la souris ou au stylet en proximité, montre son info-bulle. | V100 | info-bulle aussi au survol du stylet | L6-T7 | S9.1 | à vérifier |
| C2 | La palette ancrée peut cacher la barre de défilement de la page. | V49 | la palette ne masque jamais la barre de défilement | L6-T7 | S9.1 | piège de l'original, à ne pas reproduire |
| C3 | Verrou de classe : on déverrouille par le même chemin et le même mot de passe ; une partie seulement des outils est grisée. | V20 | voir D3 | L6-T3, L6-T31 | S19 | **écart** (D3) |
| C4 | Un document s'ouvre dans l'une ou l'autre apparence de l'original. | V84 | — | — | — | sans objet : une seule apparence (apparence simplifiée exclue, H6-8) |
| C5 | Ajouter une commande l'insère en fin de liste ; la palette suit tout de suite. | V26 | ajout en fin, effet immédiat | L6-T26 | S9.7 | à vérifier |
| C6 | Un raccourci vers un programme se supprime depuis l'éditeur. | V94 | suppression | L6-T30 | S20 | conforme au scénario |
| C7 | Une rangée de poignées vidée n'affiche plus rien au-dessus de l'objet. | V137 | liste vide : aucune poignée | L6-T26 | S9.7 | P1 |
| C8 | Une forme au fond opaque remplit de la couleur du fond les espaces de son contour tireté. | V114 ; F-574 | propriété de fond (couleur, opaque) et rendu entre les tirets | aucune | S9.4 | **absent** : le fond opaque ne vaut que pour les motifs de remplissage ; le navigateur de propriétés n'a pas de section fond. Attribution probable (autre candidat : V103, déjà P11) |
| C9 | La couleur de page par défaut ne vaut que pour les documents créés ensuite et leurs pages. | V54 | réglage sans effet sur le document ouvert | L6-T27 | S9.7 | à vérifier ; la couleur autour de la page est grise en 3.5.18 (la spécification dit bleue) |
| C10 | Le dossier personnel des ressources est un dossier ordinaire du disque ; il range aussi les profils et les messages défilants. | V27, V75, V113 ; F-881 | racine personnelle sur le disque, vue à jour | L6-T20 | S9.6 | à vérifier |
| C11 | Plusieurs objets glissés ensemble dans un dossier forment une seule ressource. | V28 | fragment de plusieurs objets | L6-T19, L6-T21 | S9.6 | conforme au plan (fragment d'une page) |
| C12 | Le tampon de la palette sert aussi à copier un objet sélectionné de la page. | V29 | tampon sur la sélection | L6-T21 | S9.6 | à vérifier |
| C13 | Choisir l'outil Sélection arrête le tampon. | V29 | arrêt par Sélection | L6-T21 | S9.6 | à vérifier |
| C14 | Les dossiers de ressources sont triés par ordre alphabétique ; un élément ajouté va dans le dossier sélectionné. | V33, V67 | tri, destination de l'ajout | L6-T20, L6-T21 | S9.6 | à vérifier |
| C15 | Chaque ligne du navigateur d'objet ouvre le même menu que l'objet. | V113 ; F-770 | menu de ligne = menu d'objet | L6-T15 | S9.3 | rejoint P13 et L6-P2 |
| C16 | La couleur clé d'une image se règle dans les propriétés. La vidéo croit à trois réglages liés ; en 3.5.18 deux suffisent (couleur, activation), le troisième est une propriété sans rapport. | V64 ; F-581 | couleur clé et activation dans la section des propriétés diverses | L6-T12 | S9.4 | **absent du navigateur** : le modèle porte déjà la couleur clé, aucune propriété ne l'édite |
| C17 | La couleur clé n'a pas de tolérance réglable ; le détourage d'un fond blanc est médiocre. | V64 (déduit) | correspondance exacte, sans réglage | — | — | non tranché : rendu exact à confirmer à l'écran |

Écarts du cadrage : E3 → D4, E4 → D2.

### P1 à P13

| # | Verdict | Ce qui change ou la preuve |
|---|---|---|
| P1 | confirmé, précisé | le profil porte deux rangées : la palette et les poignées de l'objet (V26, V30, V137 ; profils fournis) |
| P2 | confirmé | V106 ; corbeille visible dans les cinq profils fournis |
| P3 | confirmé | réglage à trois valeurs, défaut au-dessus (réglages embarqués ; F-201 ; V50) |
| P4 | **amendé** | regroupement automatique **actif** par défaut en 3.5.18 → D14 |
| P5 | **amendé** | le troisième réglage vise le déplacement d'objets ; il n'est pas exposé dans les réglages de la 3.5.18 → D14 |
| P6 | confirmé | V114 ; neuf réglages du mode Création, tous actifs |
| P7 | confirmé | compteur par page (relevé du lot 4) |
| P8 | confirmé, précisé | réglage de la section Outils (F-930), absent des profils fournis ; écart maintenu |
| P9 | confirmé | ordre du lot 4 ; panneau flottant (V25) ; à gauche par défaut |
| P10 | confirmé | V67 |
| P11 | confirmé | V103, plusieurs textes à la fois ; L6-P1 maintenu |
| P12 | confirmé | section des réglages tactiles : gestes de tourne-page actifs, zoom à deux doigts inactif, outil tactile par défaut Sélection ; lot 6 ou 11 : orchestrateur |
| P13 | confirmé, élargi | C15 : L6-P2 couvre aussi le menu des lignes du navigateur d'objet |

### D1 à D14

| # | Verdict | Ce qui diffère, et source |
|---|---|---|
| D1 | **amendé** | profils fournis de la 3.5.18 : corbeille visible dans les **cinq** ; pastilles **16** pour `preparation` (livré 24) et **8** pour les quatre autres (livré 16) ; plein écran au démarrage pour les quatre autres (livré : non) ; icônes de 32 px (livré 24) ; navigateurs masqués **et non épinglés** dans les quatre autres ; `en-classe` épingle les navigateurs de pages et de ressources (livré : ressources et quatre outils de présentation) ; `lecture-ecriture` ajoute le texte et le navigateur web ; `son-image` ajoute les enregistreurs d'écran et de son et l'insertion d'un fichier ; `preparation` épingle l'insertion d'un fichier et le texte ; `calcul-geometrie` conforme |
| D2 | confirmé | 1024 × 768 partout : aucun profil fourni ne change la taille de page ; V144 montre un profil modifié par l'auteure |
| D3 | **amendé** | sous verrou, l'original garde actifs les outils de l'élève (sélection, stylo, marqueur, gomme, texte, formes, pot, édition d'objets, couper-copier-coller, annuler, pages suivante et précédente, navigateur de pages, outils de mesure, orthographe, effacer) et grise la gestion (fichiers, insertions, profils, réglages, autres navigateurs, plein écran, bureau, caméra, enregistreurs, web, mode Création ; quitter et aide supposés grisés). Le livré fait l'inverse : 19 commandes libres, tout le reste verrouillé |
| D4 | non tranché | place du bouton non lisible en statique ; V94 (haut, version ancienne) contre F-142 (bas) ; F-142 gardé ; suppression confirmée (V94) |
| D5 | confirmé | cinq règles d'ignorance à non, mots répétés à oui, vérification à la demande seulement ; écart mineur : la 3.5.18 offre l'édition du dictionnaire de l'utilisateur dans ses réglages, le livré non |
| D6 | non tranché | aucune source sur la grille étendue ; 8, 16 ou 24 pastilles confirmé (V25) |
| D7 | **amendé** | la palette désépinglée se replie quand on touche la page (V25), pas après la sortie du pointeur ; délai non relevé |
| D8 | non tranché | V84 confirme l'accès au tableau de bord ; rien sur les récents ; défaut gardé |
| D9 | confirmé, précisé | contour, remplissage et fond sont des sous-sections d'Apparence ; section fond et couleur clé absentes du livré (C8, C16) |
| D10 | confirmé | changement de couche par glisser (V113) ; menu de ligne : C15 ; colonnes : aucune source |
| D11 | confirmé | neuf réglages, tous actifs ; tourne-page suspendu (F-560) |
| D12 | confirmé, précisé | deux comportements (toujours visible, au survol), délai 0 à 5 s, police 8 à 64 points, contour et fond de l'étiquette |
| D13 | confirmé, complété | C12 et C13 (tampon sur la sélection, arrêt par Sélection) ; fond glissé depuis les ressources (V67) |
| D14 | **amendé** | regroupement automatique **vrai** par défaut : la 3.5.18 fait foi (cadrage, amendement 1, § 3) ; haute qualité pendant l'édition d'objets : défaut non relevable, « faux » gardé comme choix d'ActCraft |

### Ce qui amende le plan ou le livré (pour l'orchestrateur)

1. **D3** : refaire la liste des commandes libres sous verrou sur le modèle de l'original (outils de l'élève libres,
   gestion verrouillée) ; `actions/data/commands.json` et `tests/tst_actions_lockable.cpp`, scénario S19.
2. **D1** : aligner les cinq profils fournis (`actions/data/profiles/*.json`) ; ou consigner chaque écart gardé.
3. **D14** : regroupement automatique actif par défaut (L6-T27, H6-12).
4. **D7** : repli au contact de la page (L6-T7).
5. **C8, C16** : proposition **L6-P3** — section fond (couleur, opaque, rendu entre les tirets) et couleur clé des
   images dans le navigateur de propriétés (`core`, `render`, `ui`), scénario S9.4.
6. **D5** : édition du dictionnaire de l'utilisateur depuis les réglages (L6-T27, L6-T34).
7. **C1, C2, C5, C9, C10, C12 à C14** : à contrôler sur le livré ; corriger ce qui manque.

### Reste

`NOMS OK` et contrôle des titres du corpus : rejoués par la passe Windows (`docs/mesures/2026-11-06-lots-6-7-passe-windows.md`).
N sous Windows : mesuré sur la tête de la branche, lot 8 en cours compris (même rapport). À l'écran : D4, D7, C17.
Par désassemblage : sort exact des commandes sans marquage sous verrou (D3). Les corrections du livré (D1, D3, D5,
D7, D14, L6-P3) ne sont pas faites par la passe Windows : elles reviennent à l'orchestrateur du chantier.
