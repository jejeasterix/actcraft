# Lot 8 — principes, décisions D1 à D16 et API revérifiées (ouverture sur le Mac)

Tâche L8-T0, scénario S30. Base du relevé : `c0eae718` (`feat/livraison-complete`, « état: reprise du lot 8 sur le
Mac (main fusionné) » : socle L0-S, lots 1 à 7 et 10, lot 6 clos par la PR #11). Constats écrits par nous. Références
`Vnn` : numéros des fiches du corpus (table hors dépôt), reprises seulement de l'en-tête du plan 8a et de ses décisions
D1 à D16. Relevé brut hors dépôt (`…\docs\re\lots\lot8.md`) : **non écrit, reporté au PC**.

**Brouillon (jalon 1)** : constats, N8 et tailles relevés ; table A en cours (A1 à A13 relevées) ; décisions au défaut.

## Méthode et limites

- **Poste.** Relevé fait sur le Mac mini, qui n'a ni l'ancien dépôt, ni le corpus des vidéos, ni le binaire 3.5.18,
  ni l'outil de contrôle des noms. Même conduite qu'aux lots 5 et 6 (H5-11, H6-11), reprise ici en H8-11.
- **Étapes 1 à 3 du plan** (corpus, ancien dépôt et son graphe, REA en statique) : non faites. Chaque constat et
  chaque décision porte la mention « à confronter au PC ».
- **Décisions.** Chacune prend le **défaut du plan** (8a ou 8b). Elle n'est tranchée autrement que si le code livré
  la contredit ; la source est alors citée (fichier:ligne).
- **API.** Revérifiées dans le code de ce worktree (en-têtes, sources, QML, données) à `c0eae718`. Les numéros de
  ligne du plan sont indicatifs ; ceux de ce rapport font foi.
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
lot 10. Sans le corpus, le contenu de V17, V55 et du second V42 n'est pas identifiable sur le Mac. Tous : **à
confronter au PC**.

| # | Principe (nos mots) | Source | Exigence pour ActCraft | Tâche | Scénario | Écart, raison ou décision |
|---|---|---|---|---|---|---|
| C1 | Un clic sur un objet verrouillé le traverse : il atteint l'objet placé dessous. | V28 (famille « sélection, verrouillage, masquage ») | sélection et déclenchement à travers un verrouillé | L8-T9, L8-T16 | S11 (`interactivite`) | D4 |
| C2 | L'action d'un objet verrouillé part quand même au clic. | V103 (même famille) | F-890 sur un objet verrouillé | L8-T9 | S11 | D4 |
| C3 | Un objet masqué reste cliquable (premier cas). | V40 (même famille) | déclenchement sur un masqué | L8-T9 | S11 | D5 ; l'ancienne application fait l'inverse |
| C4 | Un objet masqué reste cliquable (second cas). | V42 (même famille) | idem | L8-T9 | S11 | D5 |
| C5 | Constat « actions et navigateurs » de la fiche V42 : contenu à identifier au PC. | V42 | — | L8-T6 à L8-T12 (supposé) | — | à identifier au PC |
| C6 | Constat « actions et navigateurs » de la fiche V17 : contenu à identifier au PC. | V17 | — | L8-T6 à L8-T12 (supposé) | — | à identifier au PC |
| C7 | Diffusion protégée : la copie est interdite et un message le dit. | V58 (1/4) ; F-680 | `edit.copy`, `edit.cut`, `edit.duplicate` refusés, message | 8b L8-T31, L8-T32 | S18 | D10 |
| C8 | Diffusion protégée : l'impression est interdite. | V58 (2/4) ; F-680 | `file.print` refusé, message | 8b L8-T31, L8-T32 | S18 | D10 |
| C9 | Diffusion protégée : l'enregistrement est interdit. | V58 (3/4) ; F-680 | `file.save`, `file.save-as`, `export.*` refusés, message | 8b L8-T31, L8-T32 | S18 | D10 |
| C10 | Diffusion protégée : la caméra est interdite. | V58 (4/4) ; F-680 | `tool.camera-*` refusés, message | 8b L8-T31, L8-T32 | S18 | D10 ; identifiants : voir « Identifiants de la caméra » |
| C11 | Le panneau des navigateurs porte l'onglet des actions, dernier de la rangée. | V113 ; F-850, F-851 | onglet `actions` actif | L8-T2, L8-T12 | S11 (`navigateur-actions`) | A15 (identifiants livrés) |
| C12 | L'enregistreur d'écran enregistre l'écran ou une zone dans un format vidéo. | V43 ; F-292 | enregistreur d'écran et de zone | 8b (enregistreurs) | S26 | D11 |
| C13 | Un GIF animé s'anime sur la page. | V57, V66 | animation d'une image GIF | 8b (médias) | S25 | D14 |
| C14 | Cadre web : premier constat (contenu de la page web affichée). | V63 (1/2) ; F-302 | cadre web | 8b (cadre web) | S27 | D12 |
| C15 | Cadre web : second constat (image fixe, texte glissé, rideau). | V63 (2/2) ; F-302 ; N1-A3 | cadre web sous le rideau | 8b (cadre web) | S27 | D12 |
| C16 | Constat « multimédia » de la fiche V55 : contenu à identifier au PC. | V55 | — | 8b (supposé) | S25 (supposé) | à identifier au PC |
| C17 | Un second média se pose à côté du premier, sans le recouvrir. | V126 ; F-552 | placement d'un second média | 8b (médias) | S25 | D13 |
| C18 | Un conteneur recouvert par une image ne reçoit plus le dépôt (piège). | V39 ; F-577, F-580 | règle de conteneur avec recouvrement | L8-T14, L8-T16 | S11 (`conteneurs`) | D16 |
| C19 | Cas limite hors lot. | V64 | — | — | — | hors lot (plan 8a, en-tête) |
| C20 | Cas limite hors lot. | V98 | — | — | — | hors lot (plan 8a, en-tête) |
| E6 | Le limiteur de dimension a une valeur « bloqué ». | V136 ; F-579 | sens de « bloqué » pour la dimension | L8-T15, L8-T16 | S11 (`conteneurs`) | D6 |
| P8 | Encre révélatrice : le cache est redessiné tout de suite au relâché, jamais différé. | lot 10, P8 ; D8-8 | `canWait` refuse l'encre révélatrice | L8-T18 | S11 (`encre-revelatrice`) | A2 (observé : déjà en place) |

## Décisions

Sauf mention contraire : **défaut du plan, à confronter au PC.** (Jalon 1 : défauts à compléter.)

## API

Code lu à `c0eae718`. « Conforme » : nom et signature présents ; seules les lignes changent. (Jalon 1 : A1 à A13
relevées, A14 à A30 en cours.)

| # | API supposée par le plan | Code livré (fichier:ligne) | Conforme | Tâche à amender |
|---|---|---|---|---|
| A1 | `Action`, `ActionKind`, `Restrictions`, `ObjectCommon::actions`, `StrokeKind::Revealer`, `Media`, `ObjectKind`, `UnknownObject` | `core/include/actcraft/core/model.h:150-152` (`movable`, `resizable`, `rotatable`, vrais par défaut), `:203` (`std::vector<Action> actions`), `:220` (`kObjectKindCount = 10`), `:285` (`Revealer = 2`) ; reste en cours | en cours | — |
| A1b | `Document(w, h)`, `page(i)`, `pageCount()`, `fingerprint`, `documentToJson`, `documentFromJson` | `core/include/actcraft/core/document.h:38` (`Document(double pageWidth, double pageHeight)`), `:42`, `:44-45`, `:106` ; `core/src/document.cpp:232` (`fingerprint`, `mix`) ; `formats/include/actcraft/formats/nativejson.h:34`, `:83` (`documentFromJson(const QJsonObject&, const ResourceLoader&, …)`) ; `broadcast()` absent (posé par L8-T3) | oui | — |
| A2 | `canWait(page, object)` | `render/src/fixedpageitem.cpp:56` (`bool canWait(const core::PageObject& object)` : **un seul argument**), `:59` (encre révélatrice) | en cours | — |
| A3 | `paintPage`, `paintObject`, `paintObjectLocal`, `paintStroke` | `render/include/actcraft/render/pagepainter.h:69`, `:71`, `:78` ; `render/src/pagepainter.cpp:199` (`paintStroke`), `:820`, `:864`, `:911` | oui | — |
| A4 | lecteur `.flipchart` : `actionFrom` | `formats/src/flipchart.cpp:563`, `:667` (`actionFrom(const IndexNode&, const StreamObject&)`), `:1038` (actions de page), `:822` (mode 2 → `Revealer`) | en cours | — |
| A5 | `tests/support/flipchartwriter.h` | présent | oui | — |
| A6 | `CommandRegistry` : `loadDeclarations`, `find`, `isEnabled`, `trigger`, `setLocked`, `isLocked`, `isOwnEnabled` ; `lockable`, `restrictable` | `actions/include/actcraft/actions/commandregistry.h:56` (`lockable = true`), `:57` (`restrictable = false`), `:70`, `:73`, `:82`, `:83`, `:89`, `:90`, `:96` (`trigger(id, argument, Platform)`) | oui | — |
| A7 | `ui::Commands` : `registry()`, `setNativeHandler`, `stateChanged` | `ui/include/actcraft/ui/commands.h:68`, `:74`, `:102` ; `appCommands` : en cours | en cours | — |
| A8 | `execute`, `document()`, `currentPageIndex()`, `currentPageChanged`, `addChangeObserver`, `ChangeOrigin` | `ui/include/actcraft/ui/documentcontroller.h:83`, `:80`, `:170-177` (`Lift`, `Drop`), `:186` ; **`currentPageIndex()` absent** : `currentIndex()` `:70` (`Q_PROPERTY … NOTIFY pagesChanged` `:40`) ; **`currentPageChanged` absent** : `pagesChanged()` `:210` | **écart** | en cours |
| A9 | `liftObjects(std::vector<ObjectId>)`, `dropLifted(std::unique_ptr<UndoCommand>)` | `documentcontroller.h:143` (`std::vector<core::PageObject> liftObjects(const std::vector<core::ObjectId>&)`), `:146` | oui | — |
| A10 | `TransformObjectsCommand(page, ids, …)` | `core/include/actcraft/core/objectedits.h:17` ; signature en cours | en cours | — |
| A12 | `ToolHost` : `user()`, `activeTool()`, `setSelection(QList<quint64>)`, `selectionChanged` | `ui/include/actcraft/ui/toolhost.h:130`, `:129`, `:154` ; **`setSelection(QList<quint64>)` absent** : `selectObjects(const std::vector<core::ObjectId>&)` `:112` (C++ seulement) | **écart** | en cours |
| A13 | `PageView::contactActive`, `contactActiveChanged` | `ui/include/actcraft/ui/pageview.h:348`, `:351` | oui | — |

## N8

Commande (lecture seule, sans configuration) :

```
ctest --test-dir /Users/jeromedenis/projets/actcraft/.worktrees/livraison-complete/build-mac -N
```

Résultat (tête du chantier `c0eae718`) : `Total Tests: 208`. **N8 = 208** (Mac, banc actif).

Comparaison à la fin du lot 6 : N6 = 164 (L6-T0) ; L6-T1 enregistre 43 tests (207 = 164 + 43, état du chantier) ;
p6 = 0 (L6-P1 et L6-P2 n'en enregistrent aucun : suite à 207/207 après elles). Attendu N6 + 43 + p6 = **207**. Relevé
**208** : écart **+1**, expliqué : `tst_ui_launcherspage` (`tests/ui.cmake`, commit `aa9ddb8e`, onglet des raccourcis
vers des programmes du lot 6), hors réservation de L6-T1. Rapporté, pas corrigé. Conséquences : L8-T1 attend
N8 + 21 = **229** ; L8-T21 (8b) **245**. À recompter sous Windows : un test propre à une plateforme peut faire varier
le total.

## Poids des moteurs web et du multimédia (Mac)

Qt Homebrew (`/opt/homebrew/opt/qt` → `Cellar/qt/6.12.0`), tailles sur disque (`du -skL`), Ko :

| Module | Formule Homebrew | Taille (Ko) |
|---|---|---|
| QtWebView | `qtwebview` 6.12.0_1 | 516 |
| QtWebViewQuick | `qtwebview` 6.12.0_1 | 516 |
| greffons `webview` (`libqtwebview_darwin`, `libqtwebview_webengine`) | `qtwebview` | 236 |
| QtWebEngineCore | `qtwebengine` **6.11.2_1** | 1 054 476 |
| QtWebEngineQuick | `qtwebengine` 6.11.2_1 | 4 104 |
| QtWebEngineQuickDelegatesQml | `qtwebengine` 6.11.2_1 | 996 |
| QtWebEngineWidgets | `qtwebengine` 6.11.2_1 | 924 |
| QtWebChannel, QtWebChannelQuick | `qtwebchannel` 6.12.0_1 | 1 152, 408 |
| QtWebSockets | `qtwebsockets` 6.12.0_1 | 1 284 |
| QtPositioning | `qtpositioning` 6.12.0_1 | 3 012 |
| QtMultimedia | `qtmultimedia` 6.12.0_1 | 8 388 |
| QtMultimediaQuick | `qtmultimedia` 6.12.0_1 | 2 028 |
| greffon `multimedia` (`libdarwinmediaplugin`) | `qtmultimedia` | 468 |
| repères : QtCore, QtQuick | `qtbase`, `qtdeclarative` | 40 128, 28 776 |

Lecture : sous macOS, Qt WebView a un greffon natif (`libqtwebview_darwin`, moteur du système) : un cadre web y pèse
environ 1,3 Mo (deux frameworks et le greffon), sans Qt WebEngine. Qt WebEngine pèse à lui seul environ 1 Go.
Sous Windows (WebView2 du système ou WebEngine), archives et dépendances : **reste PC** (`aqt list-qt`).
