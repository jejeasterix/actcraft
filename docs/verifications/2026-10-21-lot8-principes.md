# Lot 8 — principes, décisions D1 à D16 et API revérifiées (ouverture sur le Mac)

Tâche L8-T0, scénario S30. Base du relevé : `c0eae718` (`feat/livraison-complete`, « état: reprise du lot 8 sur le
Mac (main fusionné) » : socle L0-S, lots 1 à 6 et 10 ; lot 6 clos par la PR #11 ; **lot 7 non fusionné**, voir
« Lot 7 absent de la tête »). Constats écrits par nous. Références `Vnn` : numéros des fiches du corpus (table hors
dépôt), reprises seulement de l'en-tête du plan 8a, de ses décisions D1 à D16 et du plan 8b. Relevé brut hors dépôt
(`…\docs\re\lots\lot8.md`) : **non écrit, reporté au PC**.

## Méthode et limites

- **Poste.** Relevé fait sur le Mac mini, qui n'a ni l'ancien dépôt, ni le corpus des vidéos, ni le binaire 3.5.18,
  ni l'outil de contrôle des noms. Même conduite qu'aux lots 5 et 6 (H5-11, H6-11), reprise ici en H8-11.
- **Étapes 1 à 3 du plan** (corpus, ancien dépôt et son graphe, REA en statique) : non faites. Chaque constat et
  chaque décision porte la mention « à confronter au PC ».
- **Décisions.** Chacune prend le **défaut du plan** (8a ou 8b). Elle n'est tranchée autrement que si le code livré
  la contredit ; la source est alors citée (fichier:ligne). Aucune ne l'est : le code ne contredit aucun défaut.
- **API.** Revérifiées dans le code de ce worktree (en-têtes, sources, QML, données) à `c0eae718`. Les numéros de
  ligne du plan sont indicatifs ; ceux de ce rapport font foi. Le code du lot 7 est lu sur sa branche
  (`feat/livraison-complete-b`, `8807a6eb`), seulement pour dire où il est.
- **N8.** Lu par `ctest -N` sur la construction du worktree du chantier, en lecture seule : ni configuration ni
  construction (une construction y tournait).
- **Tailles.** Mesurées sur Qt Homebrew (macOS) ; la liste des archives Windows (`aqt list-qt`) est un reste PC.
- **Légende.** « observé » : lu dans le code ou un document du dépôt ; « supposé » : déduit ; « défaut » : valeur du
  plan reprise sans nouvelle confrontation.

## Constats

L'en-tête du plan 8a rattache au lot **14 constats** (amendement 1, § 4 : 3 « sélection, verrouillage, masquage »,
6 « actions et navigateurs », 1 « outils », 4 « multimédia ») ; en séparant les sous-puces, **17**. Recompte sur la
liste du plan : 4 + 7 + 1 + 5 = **17** (V42 compté deux fois, dans deux familles ; V58 quatre fois). S'y ajoutent le
piège des conteneurs recouverts (V39), deux cas limites hors lot (V64, V98), l'écart E6 (V136) et le principe P8 du
lot 10. Sans le corpus, le contenu de V17, V55 et du second V42 n'est pas identifiable sur le Mac ; les autres
principes sont écrits d'après la question que le plan leur attache. Tous : **à confronter au PC**.

| # | Principe (nos mots) | Source | Exigence pour ActCraft | Tâche | Scénario | Écart, raison ou décision |
|---|---|---|---|---|---|---|
| C1 | Un clic sur un objet verrouillé le traverse : il atteint l'objet placé dessous. | V28 (famille « sélection, verrouillage, masquage ») | sélection et déclenchement à travers un verrouillé | L8-T9, L8-T16 | S11 (`interactivite`) | D4 |
| C2 | L'action d'un objet verrouillé part quand même au clic. | V103 (même famille) | F-890 sur un objet verrouillé | L8-T9 | S11 | D4 |
| C3 | Un objet masqué reste cliquable (premier cas). | V40 (même famille) | déclenchement sur un masqué | L8-T9 | S11 | D5 ; l'ancienne application fait l'inverse |
| C4 | Un objet masqué reste cliquable (second cas). | V42 (même famille) | idem | L8-T9 | S11 | D5 |
| C5 | Constat « actions et navigateurs » de la fiche V42 : contenu à identifier au PC. | V42 | — | L8-T6 à L8-T12 (supposé) | — | à identifier au PC |
| C6 | Constat « actions et navigateurs » de la fiche V17 : contenu à identifier au PC. | V17 | — | L8-T6 à L8-T12 (supposé) | — | à identifier au PC |
| C7 | Diffusion protégée : la copie est interdite et un message le dit. | V58 (1/4) ; F-680 | `edit.copy`, `edit.cut`, `edit.duplicate` refusés, message | 8b L8-T31, L8-T32 | S18 | D10 (`page.copy`, `page.cut` : point ouvert) |
| C8 | Diffusion protégée : l'impression est interdite. | V58 (2/4) ; F-680 | `file.print` refusé, message | 8b L8-T31, L8-T32 | S18 | D10 |
| C9 | Diffusion protégée : l'enregistrement est interdit. | V58 (3/4) ; F-680 | `file.save`, `file.save-as`, `export.*` refusés, message | 8b L8-T31, L8-T32 | S18 | D10 |
| C10 | Diffusion protégée : la caméra est interdite. | V58 (4/4) ; F-680 | `tool.camera-*` refusés, message | 8b L8-T31, L8-T32 | S18 | D10 ; identifiants : voir « Identifiants de la caméra » |
| C11 | Le panneau des navigateurs porte l'onglet des actions, dernier de la rangée. | V113 ; F-850, F-851 | onglet `actions` actif | L8-T2, L8-T12 | S11 (`navigateur-actions`) | A15 |
| C12 | La palette d'un enregistreur, masquée, réapparaît. | V43 ; F-292 | réapparition de la palette, indicateur visible | 8b L8-T28 | S26 | D11 |
| C13 | Un GIF animé s'anime sur la page. | V57, V66 | animation d'une image GIF | 8b (médias) | S25 | D14 |
| C14 | Cadre web : premier constat (page web vivante dans la page). | V63 (1/2) ; F-302 | cadre web | 8b L8-T29, L8-T30 | S27 | D12 |
| C15 | Cadre web : second constat (image fixe, texte glissé, rideau). | V63 (2/2) ; F-302 ; N1-A3 | cadre web sous le rideau | 8b L8-T29, L8-T30 | S27 | D12 |
| C16 | Constat « multimédia » de la fiche V55 : contenu à identifier au PC. | V55 | — | 8b (supposé) | S25 (supposé) | à identifier au PC |
| C17 | Un second média se pose à côté du premier, sans le recouvrir. | V126 ; F-552 | placement d'un second média | 8b (médias) | S25 | D13 |
| C18 | Un conteneur recouvert par une image : piège du dépôt. | V39 ; F-577, F-580 | règle de conteneur avec recouvrement | L8-T14, L8-T16 | S11 (`conteneurs`) | D16 |
| C19 | Cas limite hors lot. | V64 | — | — | — | hors lot (plan 8a, en-tête) |
| C20 | Cas limite hors lot. | V98 | — | — | — | hors lot (plan 8a, en-tête) |
| E6 | Le limiteur de dimension a une valeur « bloqué ». | V136 ; F-579 | sens de « bloqué » pour la dimension | L8-T15, L8-T16 | S11 (`conteneurs`) | D6 |
| P8 | Encre révélatrice : le cache est redessiné tout de suite au relâché, jamais différé. | lot 10, P8 ; D8-8 | `canWait` refuse l'encre révélatrice | L8-T18 | S11 (`encre-revelatrice`) | A2 (observé : déjà en place) |

## Décisions

Toutes : **défaut du plan, confrontation au PC.** Le code livré n'en contredit aucune ; il précise D1, D2, D6 et D10.

| # | Question | Décision | Source | Tâche amendée |
|---|---|---|---|---|
| D1 | Numéros de commande et d'action du format vers notre catalogue | Table écrite à la main par L8-T5 : au minimum 184 → `object.toggle-hidden`, l'action « aller à une page » → `page.goto` (`{"page": n}`), l'ouverture externe 270 → `document.open-file` ; numéro absent de la table : `catalogId` vide, action gardée, « action non prise en charge ». Observé : le lecteur reconnaît les types d'action 22, 23, 24, 25, 26, 28, 50, 463 (`formats/src/flipchartids.h:25-36`, `:42-46`) ; **27 n'en fait pas partie**, alors que le plan écrit « 22 à 28 ». Numéros complets : REA au PC. | D8-1 ; L8-T5 (plan, l. 1245-1250) | L8-T5 : ne pas supposer le type 27 lu |
| D2 | Valeurs des modes (déplacement, rotation, pivot, accrochage, conteneur) dans le format | **Non tranché** (REA au PC). Défaut : L8-T5 écrit et lit les propriétés communes 13 à 31, 37, 44, 45 par l'écrivain de test (plan, l. 1267-1269). Observé : seules les trois restrictions sont lues aujourd'hui (`formats/src/flipchart.cpp:1008-1010`, défaut vrai). | L8-T5 | aucune |
| D3 | Outils qui déclenchent les actions | Sélection seule ; avec un autre outil, on écrit par-dessus. | D8-2 ; H8-2 | aucune |
| D4 | Objet verrouillé : clic qui traverse (V28), action qui part (V103) | L'action d'un verrouillé part au clic (test `lockedObjectStillRuns`, L8-T9). Traversée du clic pour la sélection : **non tranchée**, le comportement livré par le lot 5 est gardé. | plan, l. 1735 | aucune |
| D5 | Objet masqué cliquable (V40, V42) | Cliquable (test `hiddenObjectStillClickable`, L8-T9), à l'inverse de l'ancienne application. | plan, l. 1735 | aucune |
| D6 | E6 : dimension « bloquée » | Aucune poignée de dimension. Observé : `Restrictions::resizable` (`core/include/actcraft/core/model.h:151`, vrai par défaut) reste la source du « bloqué » (D8-7). | D8-7 | aucune |
| D7 | Sens de « bloquer » | Arrêt au bord de l'objet bloquant. | plan, l. 2366 (L8-T15) | aucune |
| D8 | Rotation par défaut ; pivot « première ligne de texte » | Pas d'incrément par défaut, pivot au centre (supposé) ; le pivot « première ligne de texte » : **non tranché**, REA au PC. | D8-7 | aucune |
| D9 | Liste du glisser-déposer hors vote | 18 entrées, sans les deux entrées de vote. | D8-11 ; plan, l. 1407, 1455 | aucune |
| D10 | Refus de la diffusion protégée ; commandes des quatre interdictions ; caméra | Identifiants de la caméra **`tool.camera-*`** (arbitrage), déclarés et restreignables : voir la section suivante. Commandes restreignables observées (`actions/data/commands.json`) : copie `edit.cut` `:136`, `edit.copy` `:137`, `edit.duplicate` `:140` ; impression `file.print` `:189` ; enregistrement `file.save` `:11`, `file.save-as` `:12`, `export.*` (`:185-186`) ; caméra `:78-82`. `webframe.snapshot` restreignable (8b, l. 672). **Point ouvert** : `page.copy` `:125` et `page.cut` `:124` ne sont pas restreignables (revue de L6-T3, renvoyé au lot 8, F-680) ; défaut gardé (non restreignables). Messages : défaut de 8b (L8-T32) ; chaînes de la référence au PC. | D10 du plan ; revue de L6-T3 (état du chantier) | aucune (voir « Amendements proposés », 8) |
| D11 | Enregistreur d'écran (V43), formats | Défaut de 8b (L8-T28) : palette masquée qui réapparaît, indicateur toujours visible, mention à la première utilisation, dossier et format lus de la section « Enregistrements ». Formats : **non tranchés** (REA au PC). | plan 8b, l. 81, 460, 475 | aucune |
| D12 | Cadre web : image fixe, texte glissé, rideau | Image fixe par `webframe.snapshot` ; sous le rideau, la vue est masquée tant que le rideau la couvre ; texte glissé : défaut de 8b. | plan 8b, l. 628-631 ; N1-A3 | aucune |
| D13 | Second média (V126) | Posé à côté du premier. | plan 8b, l. 286 | aucune |
| D14 | GIF animé (V57, V66) | Animé par `AnimatedImage`. | plan 8b, l. 372 | aucune |
| D15 | Objet Flash | Espace réservé seulement. | plan 8b, l. 287 ; N1-A15 | aucune |
| D16 | Conteneur recouvert par une image (V39) | Règle D8-6 telle quelle : seul l'objet glissé est candidat, s'il est devant le conteneur ; l'image qui recouvre ne change rien (supposé). À confronter au PC. | D8-6 | aucune |

## Identifiants de la caméra (D10)

Déclarés à la tête : `tool.camera-area`, `tool.camera-polygon`, `tool.camera-freehand`, `tool.camera-window`,
`tool.camera-screen` (`actions/data/commands.json:78-82`, `"restrictable": true`) ; **aucun `camera.*`**. Préfixe
codé en dur : aucun `camera.` dans le code (`git grep` vide hors `docs/`) ; la barre de raccourcis filtre par
`tool.`, `present.`, `math.`, `desktop.` (`ui/src/shortcutbar.cpp:50-54`), qui couvre déjà `tool.camera-*`. **Aucun
correctif `L8-C0` pour la caméra.** Le préfixe de la diffusion protégée (L8-T31, L8-T32) est `tool.camera-`.

## API

Code lu à `c0eae718`. « Conforme » : nom et signature présents ; seules les lignes changent.

| # | API supposée par le plan | Code livré (fichier:ligne) | Conforme | Tâche à amender |
|---|---|---|---|---|
| A1 | `Action { kind, sourceType, command, target, resource, unknownKeys }`, `ActionKind`, `Restrictions`, `ObjectCommon::actions`, `StrokeKind::Revealer`, `Media`, `ObjectKind`, `UnknownObject` | `core/include/actcraft/core/model.h:148-153` (`Restrictions`, trois booléens vrais par défaut), `:156-162` (`Other` 0, `Navigate` 1, `OpenExternal` 2, `OnObject` 3), `:164-172` (`Action` : `sourceType` `uint16`, `command` `int32`, `target` `ObjectId`), `:189` (`ObjectCommon`), `:202` (`keywords`), `:203` (`actions`), `:207-218` (`Equation` 8, `Unknown` 9), `:220` (`kObjectKindCount = 10`), `:281-285` (`Revealer = 2`), `:339` (`Media`), `:390` (`UnknownObject`) | oui | — |
| A1b | `Document(w, h)`, `page(i)`, `pageCount()`, `fingerprint`, `documentToJson`, `documentFromJson` | `core/include/actcraft/core/document.h:38` (`Document(double, double)`), `:42`, `:44-45`, `:106` ; `core/src/document.cpp:232` (`fingerprint`, `mix`) ; `formats/include/actcraft/formats/nativejson.h:34`, `:83` ; `broadcast()` absent (posé par L8-T3) | oui | — |
| A2 | `canWait(page, object)` refuse images, encre révélatrice, grille du dessus | `render/src/fixedpageitem.cpp:56-61` : `canWait(const core::PageObject&)`, **un seul argument**, espace de noms anonyme ; refuse images et encre révélatrice ; la grille du dessus n'en est plus une cause (surcouche hors scène depuis L4-T7, commentaire `:50-55`) | oui (fond) | ligne A2 (texte) ; L8-T18 : rien à changer |
| A3 | `paintPage`, `paintObject`, `paintObjectLocal`, `paintStroke` | `render/include/actcraft/render/pagepainter.h:69`, `:71`, `:78` ; `render/src/pagepainter.cpp:199` (`paintStroke`), `:820`, `:864`, `:911` | oui | — |
| A4 | actions 22 à 28, 50, 463 rattachées au dernier objet ; `actionFrom` ; trois booléens | `formats/src/flipchart.cpp:562-568` (rattachement au dernier objet, sinon avertissement), `:667-690` (`actionFrom(const IndexNode&, const StreamObject&)`), `:1037-1038` (actions de page), `:1008-1010` (trois booléens) ; types : `formats/src/flipchartids.h:25-36`, `:42-46` (**27 absent**) | oui (27 : voir D1) | L8-T5 (type 27) |
| A5 | `tests/support/flipchartwriter.h` | présent | oui | — |
| A6 | `CommandRegistry` : `loadDeclarations`, `find`, `isEnabled`, `trigger`, `setLocked`, `isLocked`, `isOwnEnabled` ; `lockable`, `restrictable` | `actions/include/actcraft/actions/commandregistry.h:56` (`lockable = true`), `:57` (`restrictable = false`), `:63`, `:70`, `:73`, `:82`, `:83`, `:89`, `:90`, `:96` (`trigger(id, argument, Platform)`) | oui | — |
| A7 | `ui::Commands` : `registry()`, `setNativeHandler`, `stateChanged` ; `appCommands` | `ui/include/actcraft/ui/commands.h:21`, `:68`, `:70` (`Q_INVOKABLE trigger`), `:74`, `:84`, `:91-92` (`setLocked`, `isLocked`), `:102` ; `ui/qml/Main.qml:62-63` (`id: appCommands`, `objectName: "commands"` : l'identifiant ne vaut que dans `Main.qml`) ; « fermés par défaut » non relu | oui | — |
| A8 | `execute`, `document()`, `currentPageIndex()`, `currentPageChanged`, `addChangeObserver`, `ChangeOrigin` | `ui/include/actcraft/ui/documentcontroller.h:83`, `:80`, `:170-177` (`Lift`, `Drop`), `:186` ; **`currentPageIndex()` absent** : `currentIndex()` `:70` (`Q_PROPERTY … NOTIFY pagesChanged` `:40`) ; **`currentPageChanged` absent** : `pagesChanged()` `:210` ; aussi `showPage(int)` `:104`, `nextPage()` `:100`, `canUndo` `:47`, `replaceDocument(document, fileUrl, title)` `:94`, `objectFlagsChanged(int, QList<quint64>)` `:237` ; `objectName: "documentController"` (`Main.qml:104`) | **écart** | L8-T9, L8-T11 (historique : `pagesChanged` + `currentIndex`), L8-T13, L8-T16 |
| A9 | `liftObjects(std::vector<ObjectId>)`, `dropLifted(std::unique_ptr<UndoCommand>)` | `documentcontroller.h:143` (`std::vector<core::PageObject> liftObjects(const std::vector<core::ObjectId>&)`), `:146` | oui | — |
| A10 | `TransformObjectsCommand(page, ids, …)` | `core/include/actcraft/core/objectedits.h:17`, `:20` (`(std::size_t page, std::vector<ObjectId> ids, Transform delta)`) | oui | — |
| A11 | `selecttool.h`, `selecttool.cpp`, `selectiongeometry.cpp` | `tools/include/actcraft/tools/selecttool.h:14` ; `tools/src/selecttool.cpp`, `tools/src/selectiongeometry.cpp` présents | oui | — |
| A12 | `ToolHost` : `user()`, `activeTool()`, `setSelection(QList<quint64>)`, `selectionChanged` | `ui/include/actcraft/ui/toolhost.h:130`, `:129`, `:154` ; **`setSelection(QList<quint64>)` absent** : `selectObjects(const std::vector<core::ObjectId>&)` `:112` (C++ seulement, active l'outil Sélection) ; `currentTool` `:46` (`objectName: "appTools"`, `Main.qml:142`) | **écart** | L8-T9 |
| A13 | `PageView::contactActive`, `contactActiveChanged` | `ui/include/actcraft/ui/pageview.h:348`, `:351` | oui | — |
| A14 | `addInputCover`, `removeInputCover` ; `pageOverlays` (rideau z 10, spot z 20, outils de maths z 30) | `pageview.h:187-188` (`Q_INVOKABLE`) ; `ui/qml/PageArea.qml:20` (alias), `:168-169` (`objectName: "pageOverlays"`), `Main.qml:690` ; **rideau, spot et outils de maths absents de la tête** : seul `PageZonesOverlay` y est ; leurs contrôleurs sont sur la branche du lot 7 (`ui/src/curtaincontroller.*`, `mathtoolscontroller.*`) | **écart** | L8-T2, L8-T9 (ordre z et couvertures : à relire après la fusion du lot 7) |
| A15 | `BrowserPanel.qml` : `current` (`pages`, `resources`, `objects`, `notes`, `properties`, `actions`) ; onglet `actions` grisé | `ui/qml/BrowserPanel.qml:22` (`property string current: "pages"`), `:97-104` (`navigators` : `pages`, `ressources`, `objets`, `notes`, `proprietes`, `actions`, ce dernier `enabled: false`) | oui pour `actions` (seul employé par le lot) | ligne A15 (texte : identifiants en français) |
| A16 | `ObjectFlag`, `SetObjectFlagCommand`, `ReorderObjectsCommand`, `SetLayerCommand`, `arrangedOrder` | `core/include/actcraft/core/objectflags.h:9-14` (`Locked`, `Hidden`, `DragCopy`), `:17-18` (aucun `Change` pour `Locked` et `DragCopy` ; signal `objectFlagsChanged`, A8) ; `core/include/actcraft/core/arrange.h:21`, `:26`, `:40` | oui | — |
| A17 | `SetTranslucencyCommand` | `objectedits.h:89` | oui | — |
| A18 | `encodeClipboard`, `decodeClipboard`, `resourcesFor` ; `edit.copy`, `edit.cut`, `edit.paste`, `edit.duplicate`, `edit.delete` | `formats/include/actcraft/formats/clipboard.h:29`, `:30`, `:31` ; `commands.json:137`, `:136`, `:138`, `:140`, `:32` | oui | — |
| A19 | `page.first`, `page.last`, `page.goto`, `page.insert-after`, `page.insert-before`, `page.duplicate`, `clear.page`, `page.revert`, `page.copy`, `page.cut`, `page.background` | `commands.json:121` (`page.goto`, argument `number`), `:123`, `:122`, `:127`, `:38`, `:39`, `:125`, `:124`, `:129` ; **`page.first`, `page.last` absents** ; absents aussi, et cités par le catalogue de L8-T6 : `page.duplicate-next`, `page.duplicate-previous`, `page.duplicate-last` (`page.history-back`, `page.history-forward` : déclarés par L8-T1) | **écart** | L8-T1 ou L8-T6 (voir « Amendements proposés », 4) |
| A20 | `ReplaceObjectsCommand`, `Change` | `core/include/actcraft/core/replace.h:33` ; `core/include/actcraft/core/undo.h:16` (`Change`), `:34` (`Changes`), `:38` | oui | — |
| A21 | `ObjectCommon::keywords` | `model.h:202` (objet), `:543` (page) | oui | — |
| A22 | `PropertyModel` (`setValue`), `SetObjectPropertiesCommand` | `ui/include/actcraft/ui/propertymodel.h:34`, `:79` (`Q_INVOKABLE bool setValue(int row, const QVariant&)`), `:82` (`endEdit`), `:83` (`rowOf`) ; `core/include/actcraft/core/propertyedits.h:60` | oui | — |
| A23 | `DesignMode` : actions et glisser une copie désactivés, restrictions désactivées, objets bloquants désactivés, actions mises en évidence | `ui/include/actcraft/ui/designmode.h:24` ; réglages `:58-68` (`EditLocked`, `ShowHidden`, `HighlightActions`, `DisableActions`, `DisableRestrictions`, `DisableBlockers`, `DisableSnap`, `DisableDesktop`, `ShowTips`) ; effets en `Q_PROPERTY` : `actionsHighlighted` `:46`, `actionsDisabled` `:47`, `restrictionsDisabled` `:48`, `blockersDisabled` `:49` ; un seul réglage pour « actions et glisser une copie » (`DisableActions`) ; instance `objectName: "appDesign"` (`Main.qml:82-84`) | oui | — |
| A24 | `PenTool`, mémoire par outil, `activate("pen-shape", …)`, `ToolRegistry` | `tools/include/actcraft/tools/pentool.h:44` ; `tools/include/actcraft/tools/toolregistry.h:15` ; outil `pen-shape` (`tools/include/actcraft/tools/penshapetool.h:18`, `ui/src/toolhost.cpp:54`, activation avec argument `:342`) ; mémoire par outil non relue | oui | — |
| A25 | `writeCommon` (`restrictions{move,size,rotate}`, `actions[]`), `KnownKeys`, `readUnknown`, `followsVariantOrder` | `formats/src/nativejson.cpp:570`, `:584-585`, `:598`, lecture `:1369-1379`, `:943`, `:1619`, `:397-409` | oui | — |
| A26 | étapes `pointer` (appui, glisser, relâché, clic droit), `typeText`, `drag` vers un objet QML, `restart` | `bench/include/actcraft/bench/uiscenario.h:62` (`Pointer` : chemin de points, `button`, `modifiers`, `holdMs`, `space`), `bench/src/uiscenario.cpp:415-418` (`right`), `uiscenario.h:66` (`TypeText`), `:72` (`Restart`), aussi `DoubleClick`, `Tap` ; **aucune étape « glisser vers un objet QML nommé »** | **écart** | L8-T13 (et toute étape de L8-T12, L8-T16, L8-T18 qui viserait un objet QML par son nom) |
| A27 | corbeille sous la palette | `ui/qml/ToolPalette.qml:130` (`id: trash`) ; dépôt non relu | oui | — |
| A28 | `ui::ObjectTreeModel` | `ui/include/actcraft/ui/objecttree.h:39` (fichier `objecttree.h`) | oui | — |
| A29 | commandes du lot 7 ; ouvreur injectable de `tool.web-browser` (`ui/src/presentationcommands.*`) ; épinglages `docs/verifications/*-lot7-epinglages.md` | déclarées par le socle : `commands.json:55` (`present.curtain`), `:61`, `:63-64` (`math.*`), `:70` (`tool.web-browser`), `:78-82` (`tool.camera-*`) ; **lot 7 non fusionné** : `ui/src/presentationcommands.*` (ouvreur `m_opener`, `presentationcommands.cpp:97`, `.h:44`) et `docs/verifications/2026-11-02-lot7-epinglages.md` n'existent que sur `feat/livraison-complete-b` | **écart** | L8-T6, L8-T19, 8b (voir « Lot 7 absent de la tête ») |
| A30 | sections « Enregistrements » et « Multimédia » (`ui/settingsschema.h`) ; `ui::ResourceLibrary` | `ui/src/settingsschema.cpp:125` (`recordings`), `:128` (`multimedia`), déclarées sans clés (`:112`) ; en-tête `ui/include/actcraft/ui/settingsschema.h` ; `ui/include/actcraft/ui/resourcelibrary.h:33` | oui | ligne A30 (chemin) ; 8b L8-T28 et médias ajoutent les clés |

Bilan : **31 lignes (A1, A1b, A2 à A30) : 25 conformes, 6 écarts** (A8, A12, A14, A19, A26, A29), dont deux (A14,
A29) dus au lot 7 non fusionné.

## Lot 7 absent de la tête

`feat/livraison-complete-b` (`8807a6eb`, 2026-11-05, « fin du lot 7 côté Mac, passe PC groupée à faire ») n'est
ancêtre ni de `c0eae718` ni d'`origin/main` : **613 commits** absents de la tête. Ce qui en dépend dans le lot 8 :

- A14 : rideau, spot et outils de maths dans `pageOverlays` (ordre z, couvertures d'entrée) — L8-T2, L8-T9 ;
- A29 : ouvreur injectable de `tool.web-browser`, gestionnaires des commandes du lot 7 (catalogue `command.*` de
  L8-T6 : une commande sans gestionnaire ne fait rien au clic), épinglages du profil « Son et image » (8b) ;
- N8 : la branche ajoute au moins **19** enregistrements `actcraft_add_*` dans `tests/*.cmake` (compte des lignes
  ajoutées, à recompter après fusion par `ctest -N`) ;
- D12 (8b) : comportement du cadre web sous le rideau.

## N8

Commande (lecture seule, sans configuration) :

```
ctest --test-dir /Users/jeromedenis/projets/actcraft/.worktrees/livraison-complete/build-mac -N
```

Résultat (tête du chantier `c0eae718`) : `Total Tests: 208`. **N8 = 208** (Mac, banc actif, lot 7 non fusionné).

Comparaison à la fin du lot 6 : N6 = 164 (L6-T0) ; L6-T1 enregistre 43 tests (207 = 164 + 43, état du chantier) ;
p6 = 0 (L6-P1 et L6-P2 n'en enregistrent aucun : suite à 207/207 après elles). Attendu N6 + 43 + p6 = **207**. Relevé
**208** : écart **+1**, expliqué : `tst_ui_launcherspage` (`tests/ui.cmake`, commit `aa9ddb8e`, onglet des raccourcis
vers des programmes du lot 6), enregistré hors de la réservation de L6-T1. Rapporté, pas corrigé. Conséquences si le
lot 7 n'est pas fusionné d'ici là : L8-T1 attend N8 + 21 = **229**, L8-T21 (8b) **245** ; s'il l'est, N8 est à
relever de nouveau sur la base de L8-T1. À recompter sous Windows : un test propre à une plateforme peut faire
varier le total.

## Poids des moteurs web et du multimédia (Mac)

Qt Homebrew (`/opt/homebrew/opt/qt` → `Cellar/qt/6.12.0`), taille sur disque de chaque framework résolu (`du -sk`
sans suivre les liens internes : `du -L` compte trois fois chaque binaire, par `Versions/Current` et le lien de
tête), Ko :

| Module | Formule Homebrew | Taille (Ko) |
|---|---|---|
| QtWebView | `qtwebview` 6.12.0_1 | 172 |
| QtWebViewQuick | `qtwebview` 6.12.0_1 | 172 |
| greffon `libqtwebview_darwin` (moteur du système) | `qtwebview` | 130 |
| greffon `libqtwebview_webengine` (lié à QtWebEngine **6.11.2**, `otool -L`) | `qtwebview` | 104 |
| QtWebEngineCore (binaire seul : 281 624 Ko) | `qtwebengine` **6.11.2_1** | 351 492 |
| QtWebEngineQuick | `qtwebengine` 6.11.2_1 | 1 368 |
| QtWebEngineQuickDelegatesQml | `qtwebengine` 6.11.2_1 | 332 |
| QtWebEngineWidgets | `qtwebengine` 6.11.2_1 | 308 |
| QtWebChannel, QtWebChannelQuick | `qtwebchannel` 6.12.0_1 | 384, 136 |
| QtWebSockets | `qtwebsockets` 6.12.0_1 | 428 |
| QtPositioning | `qtpositioning` 6.12.0_1 | 1 004 |
| QtMultimedia | `qtmultimedia` 6.12.0_1 | 2 796 |
| QtMultimediaQuick | `qtmultimedia` 6.12.0_1 | 676 |
| greffon `libdarwinmediaplugin` | `qtmultimedia` | 466 |
| repères : QtCore, QtQuick | `qtbase`, `qtdeclarative` | 13 376, 9 592 |

Lecture : sous macOS, un cadre web par Qt WebView et son greffon natif pèse environ **0,5 Mo** (deux frameworks et le
greffon `darwin`), sans Qt WebEngine ; Qt WebEngine pèse environ **345 Mo** à lui seul (Core, Quick, dépendances
WebChannel et Positioning). Les médias (QtMultimedia, Quick et le greffon) pèsent environ **3,8 Mo**. Sous Windows
(WebView2 du système ou WebEngine pour Qt WebView 6.12), archives et dépendances : **reste PC** (`aqt list-qt`).

## Tâche de principes proposée

Aucune (`L8-P1` non proposée) : sans le corpus, aucun constat ne montre un principe que les tâches du plan ne
couvrent pas. À reprendre au PC avec C5, C6 et C16.

## Amendements proposés (pour l'orchestrateur)

1. **Lot 7 non fusionné** (A14, A29, N8, D12) : décider avant V2 si `feat/livraison-complete-b` est fusionné dans
   `feat/livraison-complete` ; sinon, L8-T2, L8-T6, L8-T9, L8-T19 et 8b (cadre web, caméra, épinglages) partent
   sans le code du lot 7. Après fusion : relever N8 de nouveau avant L8-T1 (N = N8 + 21).
2. **A8** : L8-T9, L8-T11, L8-T13, L8-T16 : `currentPageIndex()` → `currentIndex()`, `currentPageChanged` →
   `pagesChanged` (une fois par changement de page affichée ou du nombre de pages) ; ligne A8 de la table.
3. **A12** : L8-T9 : `ToolHost::setSelection(QList<quint64>)` → `ToolHost::selectObjects(const
   std::vector<core::ObjectId>&)` (C++ seulement ; active l'outil Sélection).
4. **A19** : `page.first`, `page.last`, `page.duplicate-next`, `page.duplicate-previous`, `page.duplicate-last` ne
   sont pas déclarées. Recommandation : L8-T1 déclare `page.first` et `page.last` (elle écrit déjà
   `commands.json` ; gestionnaire `showPage(0)` et `showPage(pageCount() - 1)`) ; les trois `page.duplicate-*`
   tombent du catalogue par la règle de L8-T6 (« une commande absente retire l'entrée »). Si L8-T1 est déjà lancée,
   c'est une donnée de commande ajoutée : `L8-C0`.
5. **A26** : L8-T13 : aucune étape ne glisse vers un objet QML nommé ; le glisser-déposer se joue par `pointer` à
   coordonnées fixes (option `space` pour les coordonnées de fenêtre), recalculées si la disposition bouge ; ou une
   étape neuve du joueur, à placer dans une tâche (hors de L8-T13 si elle touche `bench/`).
6. **A4 / D1** : L8-T5 : le type 27 n'est pas reconnu comme action (`flipchartids.h:42-46`) ; ne pas l'écrire dans
   les tests sans confirmation au PC (REA, documentation du format).
7. **Textes de la table A** : A2 (`canWait(object)`, grille du dessus retirée), A15 (identifiants `ressources`,
   `objets`, `proprietes` ; `current` existe), A28 (`objecttree.h`), A30 (`ui/include/actcraft/ui/settingsschema.h`).
8. **D10, point ouvert** : `page.copy` et `page.cut` non restreignables (défaut gardé) ; si l'orchestrateur les rend
   restreignables (copier une page contourne l'interdiction de copier), c'est une donnée de commande
   (`"restrictable": true`, `commands.json:124-125`) : L8-T1 ou `L8-C0`, plus `tst_actions_lockable` à relever.
9. **Caméra** : aucun correctif `L8-C0` (identifiants `tool.camera-*` déclarés, aucun préfixe `camera.`).

## Hypothèses nouvelles

- **H8-11** — Ouverture faite sur le Mac sans corpus, ancien dépôt ni binaire : D1 à D16 au défaut du plan ;
  confrontation aux vidéos et à REA, relevé `lot8.md`, `NOMS OK` et titres du corpus reportés au PC.
- **H8-12** — Sur le Mac, le cadre web passe par le greffon natif de Qt WebView ; la formule `qtwebengine` de
  Homebrew est en 6.11.2 face à Qt 6.12.0 (son greffon `libqtwebview_webengine` s'y lie) : ne pas le choisir sur le
  Mac sans essai.

## À confronter au PC

Identification de C5, C6, C16 et confrontation de C1 à C18, E6, P8 ; D1 (numéros, type 27), D2 (valeurs des modes),
D4 (traversée), D8 (pivot « première ligne de texte »), D10 (chaînes des refus, `page.copy`/`page.cut`), D11
(formats), D16 ; relevé brut `lot8.md` ; `NOMS OK` ; contrôle des titres du corpus et des paperboards réels ; N8 sous
Windows ; archives Windows de Qt Multimedia et Qt WebView 6.12 (`aqt list-qt`) et moteur de Qt WebView sous Windows.
