# Cadrage — cadre-web-webengine

## 1. Demande et gabarit

**Demande** (Jérôme, 2026-11-09, O146) : « Tu peux commencer la tâche Qt WebEngine, dernière du chantier ? » ; décision
de référence : coffre `02_Decisions/0217-actcraft-liberte-avant-securite-et-qt-webengine.md`.

**Gabarit** : chantier. Base : `origin/feat/livraison-complete-l11` @ 76315dbc. Mode autonome, sans porte humaine.

**Budget de questions** : 38 sur 40.

## 2. Décisions

Une ligne dont la source dit « hypothèse Hn » est tranchée par défaut et reprise en section 4.

| # | Point | Décision | Source | Réversible ? |
|---|---|---|---|---|
| 1 | Moteur PC et Mac | Qt WebEngine, JavaScript actif, glisser, sélection, capture. | coffre 0217 | oui, derrière le contrat (n° 3) |
| 2 | Moteur Android | Qt WebView reste. Copier-coller et capture à la place du glisser. Le code Android compile sans WebEngine. | coffre 0217 ; O146 point 4 | oui |
| 3 | Patron de l'interface commune | Deux composants QML au même contrat ; pas de classe C++ abstraite autour de la vue. Voir DA1. | `QQuickWebEngineView` n'est que dans `QtWebEngineQuick.framework/Versions/A/Headers/6.12.0/QtWebEngineQuick/private/qquickwebengineview_p.h:98` ; les en-têtes publics n'ont que `qquickwebengineprofile.h`, `qquickwebenginedownloadrequest.h`, `qtwebenginequickglobal.h` | oui |
| 4 | Choix du moteur | À la construction : `ui/CMakeLists.txt` ne déclare qu'un des deux fichiers (`WebFrameEngineView.qml` hors Android, `WebFrameSystemView.qml` sous Android). `WebFrameItems.engineKind` rend le nom au QML. L'option `ACTCRAFT_WEBFRAME` reste et signifie « un moteur est construit ». | `CMakeLists.txt:49-55` ; `webframeitem.h:18-28` ; hypothèse H9 pour la détection | oui |
| 5 | Chargement du moteur | Un `Loader { asynchronous: true }` par cadre visible ; `import QtWebEngine` n'est lu qu'à la première vue. Le contenu part à `Loader.Ready`, pas au seul changement de `serial`. `Loader.Error` (module absent du paquet) : même branche qu'aujourd'hui, `console.warn` et cadre vide, sans plantage. Aucun processus Chromium tant qu'aucun cadre web n'est montré. Voir DA2. | O146 (« pas de coût tant qu'aucun cadre n'est ouvert si possible ») ; `WebFrameLayer.qml:173-176` ; sonde `Loader` du 2026-11-09 (import manquant : `Loader.Error`, sortie normale) | oui |
| 6 | Initialisation | L'application **ne lie pas** `Qt6::WebEngineQuick`. Avant `QGuiApplication` : `AA_ShareOpenGLContexts`, dans une fonction commune de `ui/` appelée par les trois `main`. Android : `QtWebView::initialize()`, comme aujourd'hui. | essai `lazy` sur Mac, hors écran et à l'écran ; hypothèse H3 pour Windows | oui : lier et appeler `initialize()` |
| 7 | Trouver WebEngine sur le Mac | Le préfixe reste `opt/qt` partout. `find_package` ne fait que détecter ; la cible n'étant pas consommée, l'erreur de chemin d'en-têtes ne se produit pas. Voir DA3. | essais de configuration de ce cadrage ; `export/CMakeLists.txt:15-25` | oui |
| 8 | Qt WebView sur le bureau | Retiré : `ui/CMakeLists.txt:427-429` ne lie `Qt6::WebView` que sous Android. | coffre 0217 ; `scripts/paquet-mac.sh:99-104` | oui |
| 9 | Gardes de sécurité | Gardées dans le code, coupées par défaut. Un seul interrupteur : `WebFrameItems.guardsEnabled`, faux par défaut. `WebContentPolicy` ne change pas. | coffre 0217 ; O146 point 5 | oui |
| 10 | Réglage des gardes | Clé `web-frame-guards` de la section « tools » de `SettingsSchema`, magasin « profil » (défaut du schéma). `WebFrameItems.profile` reçoit le `ProfileController` (par `PageArea.profile`, sur les deux `PageArea`). Lecture au départ et à chaque `profileChanged`, le seul signal disponible : `setSetting` finit par `touch()`, qui part aussi à chaque couleur ou outil récent. Valeur inchangée : rien. Valeur changée : `prepareRow` de chaque ligne et vues recréées (profil et JavaScript ne se changent pas à chaud). | coffre 0217 (« à rallumer par profil ») ; `settingsschema.h:16-20` ; `profilecontroller.h:84-85` ; `profilecontroller.cpp:285-289, 689` | oui |
| 11 | Gardes coupées | Le modèle ne lit plus la page. Une adresse va à la vue telle quelle (nouveau rôle `directUrl`) ; un HTML saisi va à `loadHtml` sans réécriture. Base : dossier de ressources du document (amendé par OA92, 2026-11-13 : un HTML qui intègre un lecteur YouTube, et une adresse de lecteur saisie seule, ont pour base l'identité `https://org.actcraft.actcraft/`, sans quoi le lecteur rend l'erreur 153 ; leurs adresses relatives ne visent plus le dossier ; `docs/verifications/2026-11-13-youtube-erreur-153.md`). Rien n'est bloqué : `settings.localContentCanAccessRemoteUrls` vrai au contrat, sinon une base `file:` perd ses images distantes (défaut faux). `showsView` accepte une `directUrl`. | coffre 0217 ; `web_engine_settings.cpp:236, 377` (Qt WebEngine 6.11) ; `WebFrameLayer.qml:144` ; hypothèse H10 pour la base | oui |
| 12 | Gardes allumées | Le chemin actuel, intact : `fetch` → `prepareHtml` → `loadHtml`, JavaScript coupé, repère de blocage, déblocage. Vaut sur les deux moteurs. WebEngine : `localContentCanAccessFileUrls` et `localContentCanAccessRemoteUrls` faux. | `webframeitem.cpp:1222-1431` ; O146 point 5 | oui |
| 13 | JavaScript sur Android | Suit les gardes : actif quand elles sont coupées. | hypothèse H1 | oui |
| 14 | Adresses distantes sur Android | Permises quand les gardes sont coupées. La permission `INTERNET` existe déjà ; avec gardes, la règle « local seulement » de `webpolicy.h` reste. | hypothèse H1 ; coffre 0217 contre N1-A3 ; `android/AndroidManifest.xml:13` | oui |
| 15 | Précédent, suivant, recharger | Gardes coupées : historique du moteur, rapporté au modèle par `reportView(row, url, canBack, canForward, title, loading)` ; les commandes `webframe.*` passent par un signal `viewCommand(row, action)`. Gardes allumées : historique du modèle, inchangé. | `webframeitem.cpp:1614-1645` ; `WebFrameLayer.qml:110-112` | oui |
| 16 | Ce qui masque la vue | Propriété `overlaysScene` du contrat : vraie sous Android (vue native au-dessus), fausse sous WebEngine (élément de la scène). `covered` ne masque que dans le premier cas. | `WebFrameLayer.qml:30-31` ; essai de faisabilité (`grabToImage` rend de vrais pixels) | oui |
| 17 | Stylet sur le cadre | La vue WebEngine est déclarée couverture d'entrée (`view.addInputCover`). Sans cela le filtre de fenêtre de `PageView` prend le stylet et dessine à travers le cadre. | `pageview.cpp:1024-1060` ; `pageview.cpp:1090-1094` | oui |
| 18 | Capture | Contrat `capture(callback)` : `grabToImage` sous WebEngine, puis nouveau `WebFrameItems::insertSnapshot(row, image)`, une étape. Android : `useWindowCapture`, inchangé. Le `Grabber` reste pour les tests. | essai de faisabilité ; `webframeitem.cpp:1647-1696` | oui |
| 19 | Récepteur du glisser | Un `DropArea` neuf dans `WebFrameLayer.qml`, sous les vues, hors Android : il n'en existe aucun sur la page aujourd'hui. Coordonnées par `view.sceneToPage`. Il accepte aussi les fichiers venus du système (effet de bord E4). Actif seulement si `commands` est posé : la seconde `PageArea` (`DesktopAnnotation.qml:105`, `commands: null`) ne passerait pas par le bon document, `appInsertion` visant `appDocument`. | recherche texte (`DropArea`, `acceptDrop`) ; hypothèse H8 pour les fichiers du système | oui |
| 20 | Ce que porte le glisser | Texte et URL par `QMimeData`. **Image** : le moteur écrit ses octets dans un fichier temporaire et donne une URL `file:` ; sans octets, l'URL `http(s)` reste. Les deux cas sont traités. | `web_contents_adapter.cpp`, branche 6.11 (`mimeDataFromDropData`, `handleDropDataFileContents`, `startDragging`), https://code.qt.io/cgit/qt/qtwebengine.git/plain/src/core/web_contents_adapter.cpp?h=6.11 ; `QQuickWebEngineView` reçoit aussi (`qquickwebengineview_p.h:585-588`) | oui |
| 21 | Point d'entrée de l'insertion | Nouveau `Insertion::acceptWebDrop(urls, text, html, pagePoint)`. `acceptDrop` et `insertImage` gardent leur refus de `http(s)` (`insertion.cpp:99-101`). HTML jamais interprété, comme aujourd'hui. | `insertion.cpp:381-415` ; `tst_ui_insertion.cpp:307-322` (M13) | oui |
| 22 | Collage | `paste()` et `pasteAt()` : image du presse-papiers d'abord (déjà là), puis URL `http(s)` téléchargée, puis texte. Compte pour une URL : le texte entier, sur une ligne. Page qui n'est pas une image : objet texte, comme aujourd'hui (E9). | `objectediting.cpp:335-375` ; O146 point 3 | oui |
| 23 | Où vit le téléchargement | Nouvelle classe `WebImageFetch` dans `ui/` (`Qt6::Network` y est déjà). Ni `core/` (sans interface), ni `formats/` (`readUrl` « n'ouvre que fichier et content:// »). | `ui/CMakeLists.txt:88` ; `insertion.cpp:76-77` ; `CLAUDE.md` | oui |
| 24 | Asynchrone et décodage | `QNetworkAccessManager` sur le fil de l'interface, sans blocage ; décodage par `work()` sur son fil. Nouveau `Mode::Downloaded` ; extension déduite des octets par `QImageReader`, jamais du type annoncé. | `insertion.cpp:77-128` ; BRIEF § 4.1 | oui |
| 25 | Limites | 25 Mo, 20 s sans transfert, redirections sûres seulement. Gardes allumées : chaque adresse et redirection passe par `decide`, sinon refus. | hypothèse H6 ; `webframeitem.cpp:36-40` | oui |
| 26 | Types et échecs | Accepté si `QImage` décode. Sinon, repli sur le texte du dépôt. `data:image/…` décodé sur place. `blob:` et réseau en échec : message par `failed`, déjà relié au bandeau. | `Main.qml:878` ; hypothèse H6 | oui |
| 27 | Annulation | Un document remplacé, un onglet changé, une page retirée abandonnent le téléchargement, comme le décodage aujourd'hui. Pas de bouton d'annulation. | `insertion.cpp:247-296` ; hypothèse H7 | oui |
| 28 | Ctrl+Z | Une seule étape, à l'arrivée de l'image. Ctrl+Z pendant le téléchargement agit sur l'étape précédente et n'annule pas le téléchargement. | `insertion.cpp:208-245` ; hypothèse H7 | oui |
| 29 | Tests sans réseau | Serveur local `QTcpServer` extrait de `tst_ui_webframe.cpp:54` vers `tests/support/`. 127.0.0.1 seulement. | `tst_ui_webframe.cpp:1-8` | oui |
| 30 | Tests du contrat et du moteur | Un faux composant QML teste `WebFrameLayer` sans moteur ; un test compare les membres des trois composants. Un seul test lance le vrai moteur (`tst_ui_webengine`, sous `RUN_SERIAL`) : prouvé hors écran sur Mac. Sous Windows hors écran : saut nommé jusqu'à la preuve PC. | essai `lazy` hors écran ; `tst_ui_webframe.cpp:839-844` ; `CLAUDE.md` (« test de chargement ») | oui |
| 31 | Preuve du vrai glisser | Côté page : `QDropEvent` envoyé à la fenêtre, hors écran, avec les formats exacts du moteur : texte seul ; URL (`setUrls`) avec son titre en texte, et HTML s'il y en a ; image avec fichier temporaire en URL `file:` ; image sans octets en URL `http(s)`. Glisser réel de la vue vers la page : boucle native, souris réelle ; ni l'agent ni un test ne le jouent, Reste pour Jérôme R1. | `web_contents_adapter.cpp:1596-1615, 1664` ; `AXIsProcessTrusted` faux et `cliclick` absent (relevés du 2026-11-09) ; `CLAUDE.md`, Mode autonome 3 ; hypothèse H5 | oui |
| 32 | Mémoire | Une vue par cadre de la page affichée, détruite au changement de page. `--renderer-process-limit=2` posé dans `QTWEBENGINE_CHROMIUM_FLAGS` s'il est vide. | `WebFrameLayer.qml:125` ; BRIEF § 3 ; hypothèse H4 | oui |
| 33 | Cookies et profil | Gardes coupées : profil par défaut de QML, éphémère, commun aux vues. Gardes allumées : chaque vue porte son `WebEngineProfile {}` neuf, éphémère, et la vue est recréée quand `documentKey` change : rien ne passe d'un document à l'autre (H8-13). Android : `deleteAllCookies` à la création et à la destruction de la vue, comme aujourd'hui. | `WebFrameLayer.qml:156-160, 191-194` ; `qquickwebengineprofile.h:111` (`cookieStore()` non invocable depuis QML) ; sonde du 2026-11-09 (cookie posé dans un profil, absent du second ; `offTheRecord` vrai pour un profil neuf et pour le profil par défaut) ; hypothèse H11 | oui |
| 34 | Paquet Mac | `paquet-mac.sh` copie WebEngine dans le bundle et supprime le retrait du greffon et l'exigence du greffon `darwin`. `essai-dmg.sh` ouvre un vrai cadre depuis l'image. Voir DA4. | `paquet-mac.sh:99-125` ; `otool` de ce cadrage | oui |
| 35 | Licences | Six noms ajoutés à `qt-autorises.txt`, notices dans `THIRD_PARTY_NOTICES.md` avec la licence de Chromium. Le refus de WebEngine disparaît de `verifier-licences-mac.sh`. Aucune bibliothèque non Qt de plus. | `qquickwebengineprofile.h:2` (SPDX) ; `otool -L` de `QtWebEngineCore` ; `tst_app_notices.cpp:230-241` | oui |
| 36 | Mesures | Livrables du chantier, écrits au fichier d'état et au rapport : poids du `.dmg` et du bundle avant et après ; provenance de Qt ; décodage réel d'une vidéo H.264 ; mémoire sans cadre et avec un cadre ; coût du premier cadre (plus grand trou de la boucle d'événements : `watchEventLoop` puis `maxEventLoopGapMs` informatif dans `cadre-web.json`, en `release`). Banc court hors écran (`scripts/bench.sh`) joint aux tâches de `ui/`, en résultat et non en porte. Mesure « avant » rejouée à la base 76315dbc. | O146 point 7 ; `uiscenario.cpp:314-320` ; hypothèse H12 | oui |
| 37 | Copier depuis le cadre | Ctrl+C reste un raccourci global. `edit.copy` (`Main.qml:1175`) interroge d'abord la couche : si une vue web a le focus (`viewFocused` au contrat), `copySelection()` ; sinon `appEditing.copy()`. Sans effet sous Android : `viewFocused` toujours faux, `copySelection` vide, la copie reste à l'appui long de la vue native. | coffre 0217 (« sélection ») ; `Main.qml:1044-1047` (`Qt.ApplicationShortcut`) ; `actions/data/commands.json:140` ; Qt WebEngine 6.11, `forwardEvent`, cas `ShortcutOverride` : hors champ de saisie, la touche va au raccourci | oui |
| 38 | HTML lourd sous WebEngine | HTML dont l'encodage (`toPercentEncoding` de l'UTF-8) dépasse 2 000 000 octets : fichier temporaire du modèle (rôle `contentFile`, `QTemporaryDir` de `WebFrameItems`), chargé par `url`, gardes coupées ou allumées, avec les réglages des n° 11 et 12. `loadHtml` en dessous. `kMaxBody` reste à 4 Mo. Android : `loadHtml` seul. | `web_contents_adapter.cpp:757` (`url::kMaxURLChars`) ; `webframeitem.cpp:39` ; sonde du 2026-11-09 (1,9 Mo de lettres chargés ; 3 Mo de lettres et 1 Mo de `<` : échec, code −3, ancien contenu resté à l'écran ; fichier de 3 Mo chargé, accès fichier et distant coupés) ; hypothèse H10 | oui |

## 3. Carte d'impact

Le profil n'a pas de ligne « Surfaces d'impact » : rubriques déduites du graphe et de la recherche. Les points 1 à 8 d'O146 sont
repérés entre crochets.

### Affichage et interface [O146-1]

| Endroit | Touché ? | Pourquoi | V/S |
|---|---|---|---|
| `ui/qml/WebFrameLayer.qml:151-199` | oui | `Qt.createQmlObject` de `WebView` remplacé par un `Loader` du composant du moteur ; contenu envoyé à `Loader.Ready` | V |
| `ui/qml/WebFrameLayer.qml:144` | oui | `showsView` exige `content.length > 0` : une `directUrl`, sans contenu, ne s'afficherait jamais | V |
| `ui/qml/WebFrameLayer.qml:156-160, 191-194` | oui | cookies : ces lignes partent dans le composant Android ; sous WebEngine, profil par vue (n° 33) | V |
| `ui/qml/WebFrameLayer.qml:173-176` | oui | l'erreur de création devient la branche `Loader.Error` | V |
| `ui/qml/WebFrameLayer.qml:30-31` | oui | `covered` conditionné par `overlaysScene` | V |
| `ui/qml/WebFrameLayer.qml:94, 99` | oui | capture par fenêtre : Android seulement | V |
| `ui/qml/WebFrameLayer.qml:110-120` | oui | commandes vers la vue quand les gardes sont coupées | V |
| `ui/qml/WebFrameLayer.qml:335` | oui | texte « JavaScript reste coupé » : vrai seulement gardes allumées | V |
| `ui/qml/WebFrameLayer.qml` (neuf) | oui | `DropArea` [O146-3] ; couverture d'entrée de la vue | V |
| `ui/qml/WebFrameEngineView.qml` | créé | composant WebEngine ; `import QtWebEngine` écrit en clair, lu par `qmlimportscanner` | S |
| `ui/qml/WebFrameSystemView.qml` | créé | code de `WebView` déplacé, `import QtWebView` écrit en clair : le fichier n'est déclaré que sous Android, et l'analyse des imports de l'APK le voit (rien ne le lui montre aujourd'hui, l'import étant dans une chaîne) | S |
| `ui/qml/WebFrameDialog.qml:96` | oui | même texte sur JavaScript | V |
| `ui/qml/Main.qml:728` | non | la boîte ne change pas d'emplacement | V |
| `ui/qml/Main.qml:389, 1011` | oui | `profile: appProfile` sur `PageArea` et `Presentation` | V |
| `ui/qml/Main.qml:1044-1047` | non | raccourcis globaux : ils prennent Ctrl+C avant la vue, d'où le n° 37 | V |
| `ui/qml/Main.qml:1175` | oui | gestionnaire de `edit.copy` (n° 37) | V |
| `ui/qml/Main.qml:188-193, 878` | non | `appInsertion` et son bandeau d'échec servent tels quels | V |
| `ui/qml/PageArea.qml:13-14, 240-243` | oui | nouvelle propriété `profile`, transmise à la couche ; celle-ci garde sa place, `z: 3` | V |
| `ui/qml/DesktopAnnotation.qml:17, 105` | oui | seconde `PageArea`, sa propre couche et son propre `WebFrameItems` : `profile` à lui passer | V |
| `ui/qml/Presentation.qml:73` | oui | transmet `profile` à `DesktopAnnotation` | V |
| `ui/i18n/actcraft_fr.ts:3169, 3207` | oui | textes modifiés, entrée du réglage | V |
| `actions/data/commands.json:256-260` | non | mêmes commandes | V |
| `actions/data/commands.json:140` | non | `edit.copy`, `Ctrl+C` : ni commande ni raccourci nouveaux | V |

### Modèle et moteur [O146-1, O146-5]

| Endroit | Touché ? | Pourquoi | V/S |
|---|---|---|---|
| `ui/include/actcraft/ui/webframeitem.h:18-28` | oui | la macro ne se déduit plus de `QT_WEBVIEW_LIB` | V |
| `ui/include/actcraft/ui/webframeitem.h:66-200` | oui | `guardsEnabled`, `profile`, `engineKind`, `directUrl`, `contentFile`, `reportView`, `insertSnapshot`, `viewCommand` | V |
| `ui/src/webframeitem.cpp:39` | non | `kMaxBody` reste à 4 Mo, au-dessus de la limite de `loadHtml` : le n° 38 traite l'écart | V |
| `ui/src/profilecontroller.cpp:680-689` | non | `setSetting` et `touch()` servent tels quels ; aucun signal par clé | V |
| `ui/src/webframeitem.cpp:1222-1239` | oui | `prepareRow` : branche gardes coupées | V |
| `ui/src/webframeitem.cpp:1290-1431` | non | lecture et réécriture, gardées pour le chemin avec gardes | V |
| `ui/src/webframeitem.cpp:1585-1645` | oui | `followLink` et historique : branche gardes coupées | V |
| `ui/src/webframeitem.cpp:1647-1696` | oui | capture asynchrone ajoutée ; chemin par fenêtre gardé | V |
| `ui/include/actcraft/ui/webpolicy.h`, `ui/src/webpolicy.cpp` | non | politique inchangée, branchée seulement si les gardes sont allumées | V |
| `ui/src/settingsschema.cpp:193-200` | oui | une entrée booléenne | V |
| `core/include/actcraft/core/model.h`, `core/src/model.cpp` | non | le modèle `WebFrame` (HTML, adresse, cadre) ne change pas | V |
| `core/src/broadcast.cpp`, `core/src/edits.cpp` | non | ne dépendent pas du moteur | V |
| `formats/src/nativejson.cpp` | non | format enregistré inchangé | V |
| `render/src/pagepainter.cpp`, `render/src/paintedrenderer.cpp` | non | repère neutre des exports et miniatures inchangé | V |
| `ui/src/propertymodel.cpp`, `ui/src/objecttree.cpp` | non | libellés du type d'objet | V |

### Initialisation [O146-1, O146-4]

| Endroit | Touché ? | Pourquoi | V/S |
|---|---|---|---|
| `app/main.cpp:14-16, 38-40` | oui | fonction commune d'initialisation ; `QtWebView::initialize()` sous Android seulement | V |
| `bench/app/main.cpp:21-23, 64-66` | oui | idem | V |
| `bench/captures/main.cpp:29-31, 110-112` | oui | idem | V |
| `ui/include/actcraft/ui/` (neuf), fonction `prepareWebFrameEngine` | créé | un seul endroit, sans lien à WebEngine | S |

### Insertion, téléchargement, presse-papiers [O146-3]

| Endroit | Touché ? | Pourquoi | V/S |
|---|---|---|---|
| `ui/src/insertion.cpp:28-33, 78-128` | oui | `Mode::Downloaded` | V |
| `ui/src/insertion.cpp:99-101` | non | le refus reste pour `insertImage` et `acceptDrop` | V |
| `ui/src/insertion.cpp:247-296` | oui | suivi du document étendu à la durée du téléchargement | V |
| `ui/include/actcraft/ui/insertion.h:48` | oui | `acceptWebDrop`, garde-fou de `decide` | V |
| `ui/src/webimagefetch.cpp`, `ui/include/actcraft/ui/webimagefetch.h` | créés | téléchargement borné | S |
| `ui/src/objectediting.cpp:335-375` | oui | collage d'une URL ; aujourd'hui elle devient un objet texte (lignes 357-360 et 370-371), E9 | V |
| `ui/src/resourcelibrary.cpp:855` | non | dépôt de fichier local | V |
| `formats/` `urlio` | non | reste sans réseau | V |

### Construction [O146-6]

| Endroit | Touché ? | Pourquoi | V/S |
|---|---|---|---|
| `CMakeLists.txt:49-56` | oui | détection : `WebEngineQuick` hors Android, `WebView` sous Android ; libellé de l'option | V |
| `ui/CMakeLists.txt:175-176` | oui | un fichier QML par plateforme, `QT_QML_SKIP_CACHEGEN` sur le composant moteur | S |
| `ui/CMakeLists.txt:427-429` | oui | `Qt6::WebView` sous Android seulement | V |
| `export/CMakeLists.txt:13-27` | non | Qt PDF inchangé | V |
| `cmake/ActCraftModule.cmake` | non | aucun module nouveau | V |

### Paquets et inventaires [O146-6]

| Endroit | Touché ? | Pourquoi | V/S |
|---|---|---|---|
| `scripts/paquet-mac.sh:43-46` | oui | message : Qt WebEngine manquant | V |
| `scripts/paquet-mac.sh:58` | oui | frameworks WebEngine copiés à la main (DA4) | S |
| `scripts/paquet-mac.sh:99-104` | oui | retrait du greffon et exigence `darwin` supprimés | V |
| `scripts/paquet-mac.sh:105-125` | oui | la boucle « plus référencé » doit garder ces frameworks | V |
| `scripts/paquet-mac.sh:140-147` | non | contrôle `/opt/homebrew` et `@rpath` sans cible : il sert de garde | V |
| `scripts/essai-dmg.sh:5, 80-90` | oui | ouvrir un cadre, attendre son signal | V |
| `scripts/verifier-licences-mac.sh:4-5, 50-52, 74` | oui | refus de WebEngine retiré | V |
| `scripts/verifier-licences.ps1:26` | non | lit la liste ; aucune mention de WebEngine | V |
| `scripts/paquet.ps1:37` | non, à prouver | `windeployqt --qmldir ui/qml` doit trouver WebEngine seul | S |
| `scripts/paquet-android.ps1:124` | non | `INTERNET` déjà permise ; pas de WebEngine dans l'APK | V |
| `packaging/qt-autorises.txt:50, 70` | oui | commentaires « jamais Qt WebEngine » | V |
| `packaging/qt-autorises.txt` (fin) | oui | `Qt6WebEngineCore`, `Qt6WebEngineQuick`, `Qt6WebEngineQuickDelegatesQml`, `Qt6WebChannel`, `Qt6WebChannelQuick`, `Qt6Positioning` (aucun n'y figure ; les autres dépendances, oui) | V |
| `packaging/autres-autorises.txt` | non | `otool -L` de `QtWebEngineCore` : système et Qt seulement | V |
| `THIRD_PARTY_NOTICES.md:196, 237-238, 311` | oui | « jamais Qt WebEngine » devient faux | V |
| `THIRD_PARTY_NOTICES.md:84-85` | oui | WebView : Android seulement | V |
| `THIRD_PARTY_NOTICES.md` (neuf) | oui | notice Qt WebEngine, Chromium (`LICENSE.Chromium` de la formule, SBOM) | V |
| `CLAUDE.md:35` | oui | « Qt PDF seul, jamais Qt WebEngine » | V |
| `CLAUDE.md:32` | oui | modules `aqt` de l'installation | V |
| `LICENSES/` | à contrôler | texte de chaque identifiant cité (`tst_app_notices.cpp:292`) | V |

### CI [O146-6]

| Endroit | Touché ? | Pourquoi | V/S |
|---|---|---|---|
| `.github/workflows/ci.yml:172` | oui | `-m qtwebengine qtwebchannel qtpositioning`, exécuteurs GitHub | S |
| `.github/workflows/ci.yml:197` | oui | repli `aqt` du Mac | S |
| `.github/workflows/ci.yml:221, 335` | oui | contrôle du module au PC, en avertissement | V |
| `.github/workflows/ci.yml:324` | oui | job `paquet-windows` | S |
| `.github/workflows/ci.yml:476-478` | non | job Android : pas de WebEngine | V |
| `.github/workflows/preuves-windows.yml:92` | oui | modules | V |
| `scripts/ci/outils-android-mac.sh:26` | non | Android seulement | V |
| `scripts/ci/emulateur-mac.sh` | non | aucune mention | V |
| `.github/workflows/release.yml` | **interdit** | liste à donner à O98 ci-dessous | V |

**À ajouter à `release.yml` par O98** (rien n'est modifié ici) : `qtwebengine qtwebchannel qtpositioning` lignes 194 (Windows), 289 (`aqt` Mac),
314 (`modules=` du repli Mac mini) ; commentaires des lignes 273, 294, 312, 339 à relire. Lignes 415-416 (hôte et Android) : rien.

### Tests [O146-1 à 5]

Une entrée non touchée est un test à garder vert.

| Endroit | Touché ? | Pourquoi | V/S |
|---|---|---|---|
| `tests/tst_ui_webframe.cpp:152-161` | oui | la `Fixture` allume les gardes : tout le fichier teste ce chemin, qui reste | V |
| `tests/tst_ui_webframe.cpp:44-46, 1222-1224` | oui | initialisation commune | V |
| `tests/tst_ui_webframe.cpp:821-875` | oui | la vue devient `WebEngineView` sur bureau, gardes allumées ; saut Windows gardé | V |
| `tests/tst_ui_webframe.cpp` (neufs) | oui | gardes coupées : aucune requête du modèle, `directUrl`, rapport de vue ; `contentFile` posé au-delà du seuil, vide en dessous ; `guardsEnabled` suit un `setSetting`, un `profileChanged` sans changement ne recharge rien | V |
| `tests/tst_ui_webpolicy.cpp` | non | politique inchangée ; `javascriptOffByDefault` (l. 15) reste vrai pour la politique | V |
| `tests/tst_ui_insertion.cpp:252-340` | non | `droppedHtmlLoadsNothing` reste la garde M13 | V |
| `tests/tst_ui_insertion.cpp` (neufs) | oui | URL : fichier temporaire, téléchargement, trop gros, pas une image, annulation, Ctrl+Z | V |
| `tests/tst_ui_fileexchange.cpp:511` | non | `acceptDrop` inchangé | V |
| `tests/tst_ui_shell.cpp:965-967, 1028` | non | objets de la coquille inchangés | V |
| `tests/tst_ui_settingsschema.cpp:163` | non | clé de profil de plus dans une section existante : liste des sections inchangée | V |
| `tests/tst_app_notices.cpp:230-241` | non | garde ; passe seulement si les notices suivent | V |
| `tests/tst_ui_webengine.cpp`, contrat | créés | vrai moteur : HTML de 3 Mo chargé, gardes coupées et allumées ; image distante avec base `file:`, chargée gardes coupées, refusée gardes allumées ; cookie du document A absent du document B, gardes allumées ; `edit.copy` copie la sélection ; champ de saisie qui garde Ctrl+V ; `Loader.Error` sur un module absent. Comparaison des contrats | S |
| `tests/tst_ui_desktop.cpp` | oui | la couche de l'annotation du bureau suit le réglage, son `DropArea` reste coupé | V |
| `tests/tst_core_hittest.cpp:146`, `tst_core_model.cpp:526`, `tst_core_interactivity.cpp:82`, `tst_core_broadcast.cpp:246` | non | modèle | V |
| `tests/tst_formats_interactivity.cpp:212`, `tst_formats_nativejson.cpp`, `tst_formats_clipboard.cpp:254` | non | format | V |
| `tests/tst_render_painted.cpp:504` | non | repère neutre | V |
| `tests/tst_actions_layout.cpp:582`, `tst_actions_lockable.cpp:179` | non | commandes | V |
| `tests/tst_ui_secondpalette.cpp` | non | palette | V |
| `tests/tst_ui_pageinput.cpp` | oui | stylet sur le cadre : pas de trait | V |

### Banc et captures [O146-7]

| Endroit | Touché ? | Pourquoi | V/S |
|---|---|---|---|
| `bench/scenarios/cadre-web.json:5-22` | oui | la seconde moitié attend `blockedCount` 1 : étape `setting` en tête ; fenêtre `watchEventLoop` autour du premier cadre (n° 36) | V |
| `tests/bench.cmake:617` | non | `tst_captures_cadre_web` à garder vert | V |
| `bench/src/fixtures.cpp:461-473` | non | fixture du document reçu | V |
| `bench/src/uiscenario.cpp:170-320, 2067` | oui | nouvelle étape `setting` (aucune clé n'en fait office aujourd'hui) ; elle appelle `setSetting` sur `appProfile`, déjà trouvé par son `objectName` | V |
| `docs/banc.md` | non | description générale | V |

### Autres dépôts

| Endroit | Touché ? | Pourquoi | V/S |
|---|---|---|---|
| ancien dépôt `REA - Activinspire` | non | absent du Mac, référence de comportement seulement | V |
| `claude-config`, `~/pilotage` | non | aucun lien | V |

### Effets de bord sur l'existant

- **E1. JavaScript actif et aucun filtre par défaut**, documents reçus compris. Test : gardes coupées, `tst_ui_webframe` (aucune requête du modèle, `blockedCount` nul) ; gardes allumées, tout `tst_ui_webframe` et `tst_captures_cadre_web`.
- **E2. Le cadre passe sous les menus et boîtes** sur le bureau au lieu d'être masqué. Test : `tst_ui_webengine` (fenêtre ouverte, `visible` reste vrai) et faux composant `overlaysScene`.
- **E3. Le stylet n'écrit pas à travers le cadre** : comportement inchangé, cause différente. Test : `tst_ui_pageinput`, stylet sur un cadre, aucun trait.
- **E4. Un fichier glissé du système sur la page s'insère** : la méthode existait, sans récepteur. Test : `QDropEvent` d'un `file:` sur la fenêtre, `tst_ui_insertion` ou test de la couche.
- **E5. Paquet de bureau plus lourd.** Bundle Mac : 143 Mo aujourd'hui, `QtWebEngineCore` ajoute 288 Mo de binaire (288 382 768 octets) et 68 Mo de ressources. Mesure : S4.
- **E6. `ACTCRAFT_WEBFRAME` à OFF sur un poste sans le module** : construction sans cadre web attendue (H9). Aucun test : `engineBuilt` n'en a pas (`webframeitem.h:108`), aucun job ne construit à OFF, `paquet-mac.sh` refuse OFF. Le PC y passera tant que le module manque (`ci.yml:221-222` ne le contrôle pas) : Reste PC 1.
- **E7. Les textes sur JavaScript changent.** Test : `tst_ui_webframe` (neuf), le message de la boîte suit `guardsEnabled` ; `actcraft_fr.ts` mis à jour.
- **E8. Fenêtres surgissantes** (`target=_blank`) : ouvertes dans la même vue ; permissions refusées ; téléchargements de page ignorés. Test : `tst_ui_webengine`.
- **E9. Une URL collée ne crée plus d'objet texte d'emblée** (`objectediting.cpp:357-360, 370-371`) : elle part au téléchargement. Page non image : objet texte, à la même place, après la réponse. Réseau en échec : bandeau, aucun objet (H14). Test : `tst_ui_insertion`, « page non image → texte » et réseau en échec.
- **E10. Ctrl+C dans un cadre au focus copie la sélection du cadre**, plus les objets de la page. Hors champ de saisie seulement ; dans un champ, le moteur garde ses touches. `edit.copy` verrouillé par un profil bloque aussi cette copie. Test : `tst_ui_webengine`, et `tst_ui_shell` pour Ctrl+C sans vue au focus.
- **E11. HTML lourd chargé depuis un fichier temporaire** (n° 38). Gardes allumées : ses images distantes autorisées ne se chargent pas (origine `file:`, accès distant coupé). Test : `tst_ui_webengine`, 3 Mo.
- **E12. Changer le réglage recharge les cadres de la page** et oublie leur session. Test : `tst_ui_webframe`.
- **E13. L'annotation du bureau suit le même réglage**, sans dépôt possible. Test : `tst_ui_desktop`.

### Chantiers en cours

- **PR #23** `feat/livraison-complete-l11` (brouillon) : base de ce chantier, touche `ui/qml/PageArea.qml`, `ui/qml/WebFrameDialog.qml`, `ui/src/insertion.cpp`, `scripts/paquet-mac.sh`, `scripts/verifier-licences-mac.sh`, `packaging/qt-autorises.txt`, `THIRD_PARTY_NOTICES.md`, `ui/CMakeLists.txt`, `ci.yml`. La PR de ce chantier la vise.
- **PR #21** `ci/sans-minutes-disque` : `.github/workflows/ci.yml`, job `paquet-windows`. Conflit attendu lignes 324-335.
- **PR #22** `ci/ci-ciblee` : `.github/workflows/ci.yml`. Conflit possible ; O84 ne doit pas être contacté.
- **PR #2** `docs/licence-polyform-nc` (brouillon du 2026-10-14) : touche `CLAUDE.md` et `CMakeLists.txt`. Conflit possible, sans lien de fond.
- Worktrees locaux `livraison-complete`, `livraison-complete-b`, `o142-go-release` : `git status` vide. `lc11--L11-C18` : `build-mac-release/` seul, non suivi. `livraison-complete-l11` : même tête que ce chantier, seuls des fichiers de `graphify-out/cache` non suivis.
- Autre machine : aucun worktree donné par l'orchestrateur ; aucun job sur le PC (interdit de la tâche).

### Termes cherchés

Graphe (`--graph ~/.cache/graphes/actcraft/graphify-out/graph.json`, suit `main`, sans le lot 11) : `insertion`, `DropArea acceptDrop insertImageData insertImage`, `acceptDrop dragEnter dropEvent DropArea setAcceptDrops`.

Texte, sur le worktree : `webframe|webengine|WebView|QtWebView` (dépôt entier, sans `build`, `graphify-out`) ; `DropArea|acceptDrop` (`ui/qml`, `features`, puis `ui`, `tests`) ; `dragEnter|dropEvent|setAcceptDrops|ItemAcceptsDrops` ; `addInputCover|m_covers|eventFilter` ; `ACTCRAFT_WEBFRAME|QT_WEBVIEW_LIB` ; `JavaScript` (`ui/qml`, `ui/i18n`) ; `qtwebview|qtpdf|Qt6Pdf` (`scripts`, `.github/workflows`, `packaging`) ; `cadre-web` (`tests`, `bench`) ; `kMax|kTransferTimeoutMs` ; `H8-13|H8-14|H8-15|H8-18` ; `edit\.copy` ; `engineBuilt` ; `appProfile|\.setting\(` ; `DesktopAnnotation|desktopController` ; `hasText|acceptDrop\(\{\}` ; `watchEventLoop|maxEventLoopGapMs` ; `o.contains\(QStringLiteral` (`bench/src/uiscenario.cpp`) ; `qmlimportscanner|QtWebView` (`scripts/paquet-android.ps1`) ; `WEBFRAME|install-qt|brew` (`.github/workflows`).

En-têtes et binaires de `/opt/homebrew/opt/qtwebengine` : `startDrag|dragEnter|selectedText|triggerWebAction|profile` sur `qquickwebengineview_p.h` ; `plugins.qmltypes` ; `strings -a` sur `QtWebEngineCore` et `QtWebEngineQuick` (`QTWEBENGINE_DISABLE_SANDBOX`, `QTWEBENGINE_CHROMIUM_FLAGS`, `initialize`) ; `otool -L` et `otool -l` sur `QtWebEngineCore`, `QtWebEngineQuick`, `QtWebEngineProcess`, `libqtwebenginequickplugin.dylib` ; `cookieStore` dans `qquickwebengineprofile.h`. Sources de Qt WebEngine 6.11 : `kMaxURLChars`, `mimeDataFromDropData`, `LocalContentCanAccessRemoteUrls`, `ShortcutOverride`.

Coffre : `Qt WebEngine cadre web actcraft` ; `cadre web WebView sécurité sous-ressources` ; `paquet Mac dmg Qt licences inventaire` ; `piège Qt Quick WebEngine offscreen sandbox`. Recherche non dégradée.

## 4. Hypothèses à revoir

### Niveau 1 : difficiles à défaire ou visibles des utilisateurs

- **H1. JavaScript et adresses distantes actifs aussi sur Android.** 0217 ne dit JavaScript actif que « sur PC et Mac », mais coupe les gardes partout. N1-A3 (rang inférieur) disait « contenu local seulement ». Défaut : un seul interrupteur, même effet partout.
- **H2. H.264 : le paquet dépend de la provenance de Qt, et la redistribution est une décision de licence.** Windows : la version officielle de Qt WebEngine n'a en principe pas H.264 ; une vidéo ne jouerait pas, sans que rien l'annonce (Reste PC 7). Mac : Qt de Homebrew (`release.yml:304-307`) décode H.264 (`canPlayType` « probably », fichier d'état) ; la voie `aqt` (`release.yml:289, 314`) prend la version officielle, en principe sans ces codecs, non essayée ici. Deux `.dmg` de même version peuvent donc différer. Défaut : S4 note la provenance, rien n'est publié dans ce chantier. Redistribuer des décodeurs H.264 revient à Jérôme (`CLAUDE.md`, Mode autonome 2), avant toute release : R3. Une reconstruction serait un autre chantier.
- **H3. Pas d'appel à `QtWebEngineQuick::initialize()`.** Prouvé sur Mac seulement, à l'écran et hors écran. La documentation de Qt demande cet appel. Sous D3D11, Windows peut l'exiger. Défaut : attribut seul partout ; si la preuve PC échoue, lien et appel sous Windows seulement, en quelques lignes (DA2).
- **H4. Mémoire sur le Dell 3510 (4 Go).** Un cadre : 235 à 274 Mo cumulés, sonde hors écran sur Mac (DA2). Limite de deux processus de rendu choisie sans mesure sur cette machine. Reste PC.
- **H5. Glisser réel non prouvé par le chantier.** La boucle native exige une souris réelle : `AXIsProcessTrusted` faux, `cliclick` absent, souris réservée à Jérôme. Défaut : le côté dépôt est prouvé hors écran avec les formats du moteur (S1) ; le geste complet est le Reste R1, sans bloquer la PR en brouillon. Il reste possible que le moteur rende un format non prévu : R1 le montrera.
- **H8. Le récepteur accepte aussi les fichiers du système.** Fonction nouvelle, non demandée, mais la méthode l'attendait. Défaut : accepté (E4), retirable en filtrant le schéma.
- **H14. URL collée, réseau en échec : bandeau, sans objet.** Hors ligne, un lien collé ne crée plus de texte (E9). Défaut : suit S2, l'échec reste visible et le document intact. Autre choix : repli sur le texte en plus du bandeau, quelques lignes.

### Niveau 2 : les autres

- **H6. Limites du téléchargement.** 25 Mo, 20 s (reprise de `kTransferTimeoutMs`). Pas de partage de cookies avec le moteur : une image protégée échoue, l'utilisateur prend « Image fixe ».
- **H7. Annulation automatique seulement**, sans bouton ; Ctrl+Z n'agit pas sur un téléchargement en cours.
- **H9. Détection CMake.** Bureau : `WebEngineQuick` trouvé donne `ACTCRAFT_WEBFRAME` ON, comme aujourd'hui avec `WebView`. Module absent : construction sans cadre web, non prouvée par ce chantier (E6).
- **H10. Base du HTML saisi et HTML lourd.** Limite de `loadHtml` vérifiée (n° 38). Gardes coupées : base au dossier de ressources du document ; au-delà de la limite, le fichier temporaire change cette base, et ses adresses relatives visent le dossier temporaire. Gardes allumées : ses images distantes tombent (E11). Seuil de 2 000 000 octets choisi sous les 2 097 152 de la limite, pour le préfixe `data:`.
- **H11. Gardes coupées : un seul profil.** Cookies communs à tous les cadres et documents, perdus à la fermeture ; le profil par défaut est bien éphémère (sonde du 2026-11-09). Gardes allumées : un profil par vue, donc pas de cookie partagé entre deux cadres d'une même page.
- **H15. Premier cadre asynchrone.** Le cadre reste vide le temps du `Loader`. Un trou de 87 ms subsiste à la création de la vue, qu'aucun `Loader` ne découpe. Valeur sur Dell 3510 inconnue : Reste PC 8.
- **H12. Mesure « avant » rejouée** à la base, au prix d'une construction Release sous le jeton. Repli si le jeton manque : 61 Mo, relevé de `docs/verifications/2026-11-07-lot11-paquet-macos.md`.
- **H13. Menus contextuels** : ceux du moteur, sans menu à nous. Le glisser au stylet sur écran interactif reste à juger à la reprise par Jérôme.

## 5. Décisions d'architecture

**DA1. Deux composants QML, un contrat.**
Contexte : l'UI ne doit pas dépendre du moteur. `QQuickWebEngineView` n'a pas d'en-tête public : une classe C++ autour de lui dépendrait d'une API privée de Qt.
Décision : `WebFrameEngineView.qml` et `WebFrameSystemView.qml`, mêmes propriétés (`html`, `baseUrl`, `directUrl`, `contentFile`, `serial`, `guardsEnabled`, `overlaysScene`, `viewFocused`), mêmes méthodes (`goBack`, `goForward`, `reload`, `capture`, `copySelection`), même signal de rapport. `guardsEnabled` donne JavaScript, accès fichier et distant, et profil.
Conséquences : pas de lien à une API privée ; un test compare les membres et un faux composant teste la couche ; le contrat vit en QML, sans contrôle du compilateur.

**DA2. WebEngine non lié, chargé au premier cadre.**
Contexte : 288 Mo de binaire, Celeron à 4 Go, fluidité d'abord.
Décision : seule une importation QML différée le charge, par un `Loader` asynchrone ; l'attribut `AA_ShareOpenGLContexts` est posé avant l'application.
Relevé du 2026-11-09, sonde hors dépôt, Mac, hors écran, `Loader` dans un module statique, à refaire au plan (n° 36) : 33 Mo sans lien et 45 Mo avec lien avant tout cadre ; un processus sans cadre ; 235 à 274 Mo avec un cadre (somme des mémoires résidentes, deux processus). Fil de l'interface bloqué 199 à 797 ms en `Loader` synchrone, 87 à 88 ms en asynchrone. Page chargée dans les deux cas.
Conséquences : aucun coût au démarrage ; le coût passe au premier cadre, mesuré au banc (H15). Repose sur H3. Retour arrière : une ligne CMake et un appel.

**DA3. Préfixe `opt/qt` conservé.**
Contexte : `Qt6::WebEngineQuick` annonce `opt/qt/include/QtWebEngineQuick`, absent de Homebrew.
Décision : ne pas lier supprime le problème : configuration réussie sur `opt/qt` (`Qt6WebEngineQuick_DIR` trouvé), import QML trouvé sans variable d'environnement, avec `Loader` dans un module statique (sonde du 2026-11-09 ; à rejouer dans le projet au plan).
Conséquences : le préfixe ne bouge ni dans `ci.yml`, ni dans `release.yml`, ni dans les consignes du Mac ; seules les listes de modules `aqt` changent (carte CI, liste pour O98). Le constat du fichier d'état, « le préfixe doit être `/opt/homebrew` », vaut pour un exécutable lié, plus pour ce montage. Si un lien revient : même retrait des dossiers absents que `export/CMakeLists.txt:15-25`, essayé ici avec succès.

**DA4. Copie explicite de WebEngine dans le paquet Mac.**
Contexte : le greffon référence `@rpath/QtWebEngineQuick` et `@rpath/QtWebEngineCore`, ce que macdeployqt ne résout pas chez Homebrew (`paquet-mac.sh:61-65`). `QtWebEngineProcess` et `QtWebEngineCore` gardent des chemins `/opt/homebrew/…`. Le processus auxiliaire tourne à un autre endroit : `@executable_path/../Frameworks` y désigne un dossier absent.
Décision : copier les frameworks (avec `Helpers` et `Resources`), réécrire leurs références, ajouter à l'auxiliaire un lien `Contents/Frameworks` vers les frameworks du bundle. Le lien sera remplacé par une réécriture en `@rpath` si l'essai échoue.
Conséquences : `essai-dmg.sh` doit charger une vraie page ; les gardes existantes (`/opt/homebrew`, `@rpath` sans cible) refusent tout oubli.

**DA5. Gardes dans le modèle, politique intacte.**
Contexte : tout `tst_ui_webpolicy` et la moitié de `tst_ui_webframe` prouvent la politique ; le travail du lot 8 doit rester rallumable.
Décision : l'interrupteur est dans `WebFrameItems`, en amont de `decide` ; la classe de politique ne change pas.
Conséquences : aucun test de politique touché, la `Fixture` de `tst_ui_webframe` allume les gardes ; le filtrage par `QWebEngineUrlRequestInterceptor`, plus fin, reste possible pour les profils futurs.

## 6. Scénarios d'acceptation

- **S1 — Dépôt et copie, hors écran (Mac, ctest).** Un cadre charge une page locale dont le script écrit « JS OK » dans le vrai moteur. Du texte sélectionné, vue au focus, `edit.copy` le met au presse-papiers ; sans ce focus, Ctrl+C copie toujours les objets. La fenêtre reçoit un `QDropEvent` aux formats exacts du moteur (n° 31) : texte seul, un objet texte ; image en URL `file:`, une image locale ; image `http(s)` du serveur local, téléchargée, et le document enregistré puis rouvert serveur coupé la montre. Ctrl+Z retire chaque objet en une étape. Le geste à la souris : R1.
- **S2 — Coller et échecs.** Copier l'URL d'une image, coller sur la page : l'image est téléchargée et insérée comme ressource. Une URL en 404 ou de plus de 25 Mo : bandeau d'erreur, document inchangé. Changer d'onglet pendant le téléchargement : l'image n'atterrit pas dans l'autre document.
- **S3 — Gardes rallumées.** Activer le réglage (effet immédiat, sans relancer), rouvrir un document reçu avec un cadre distant : repère de blocage, aucune requête avant le déblocage, JavaScript coupé, sous-ressources filtrées (`tst_captures_cadre_web`). Un cookie posé dans le document A est absent du document B (`tst_ui_webengine`).
- **S4 — Paquet Mac.** `scripts/paquet-mac.sh` rend `DMG OK`, `scripts/essai-dmg.sh --lancer` rend `ESSAI DMG OK` en chargeant un vrai cadre depuis l'image, aucune bibliothèque hors du bundle. Relevés écrits au fichier d'état et au rapport : provenance de Qt (Homebrew ou `aqt`, version de la formule) ; poids du `.dmg` et du bundle avant et après ; une vidéo H.264 décodée (`videoWidth` non nul, `currentTime` qui avance) ; sans cadre web ouvert, aucun processus Chromium ; coût du premier cadre.
- **S5 — Android, construction.** Le job `android` du Mac mini construit l'APK sans WebEngine, et l'APK contient le greffon QML de `QtWebView` (`unzip -l`). L'appui long et le collage : R2.

## 7. Hors périmètre

- Profils d'interface et sécurité par profil au-delà de l'interrupteur (reprise de Jérôme, 0217).
- Modifier `.github/workflows/release.yml` (O98) ; toute release, tout tag ; signature et notarisation.
- Machine GitHub payante ; job sur le PC.
- Reconstruction de Qt WebEngine avec codecs propriétaires.
- Menu contextuel propre, insertion au stylet par menu, zoom du contenu du cadre avec la page.
- Filtrage par intercepteur de requêtes.
- Téléchargements déclenchés par les pages, permissions de caméra, micro et position.
- Autres dépôts : aucun endroit touché.
- Noms repris d'un produit existant dans le code : aucun (`NOMS OK` reste exigé).

### Restes pour le PC

Faits après le chantier, sans attendre.

1. Installer `qtwebengine qtwebchannel qtpositioning` dans `C:\Qt\6.12.0\msvc2022_64`.
2. H3 : démarrer sous D3D11 sans `initialize()` ; repli si échec.
3. MSVC `/W4 /WX`, preset `release`, suite complète.
4. `tst_ui_webengine` et saut hors écran Windows : décider du maintien.
5. `scripts/paquet.ps1` et `scripts/essai-installateur.ps1` : `windeployqt` trouve-t-il `QtWebEngineProcess.exe` et les ressources ; noms exacts à ajouter à `qt-autorises.txt`.
6. Poids de l'installateur avant et après.
7. H2 : `canPlayType` H.264 sur la version officielle.
8. H4 : mémoire et banc sur le Dell 3510 ; captures D3D11 sous le jeton `ecran`.

### Restes pour Jérôme

Ni l'agent ni un test ne jouent la souris ou le doigt. Aucun de ces restes ne bloque la PR en brouillon.

**R1. Glisser réel (Mac, à l'écran).**

1. Ouvrir `build/paquet/ActCraft-<version>-macos-arm64.dmg` (produit par S4), lancer ActCraft.
2. Insérer un cadre web : menu « Insérer », « Lien », « Cadre web… », champ « Ou code HTML », coller ce texte, OK :

   ```html
   <p id="m"></p><script>m.textContent="JS OK"</script><p>Texte à glisser</p><img width="160" src="https://www.gstatic.com/webp/gallery/1.jpg">
   ```

3. Attendu : « JS OK » s'affiche dans le cadre.
4. Sélectionner « Texte à glisser », glisser la sélection hors du cadre, sur la page. Attendu : un objet texte.
5. Glisser l'image hors du cadre, sur la page. Attendu : une image.
6. Couper le Wi-Fi, enregistrer, fermer, rouvrir. Attendu : l'image est là.
7. Annuler deux fois. Attendu : chaque objet part en une étape.
8. Noter ce qui diffère, avec le numéro de l'étape.

**R2. Android (appareil ou émulateur, APK de S5).**

1. Insérer un cadre web, adresse `https://example.org`, OK.
2. Appui long sur un mot du cadre, poignées, « Copier ».
3. Coller sur la page. Attendu : un objet texte.
4. Bouton « Image fixe » du cadre. Attendu : une image du cadre sur la page (`grabWindow` sous Android, non vérifié ici).

**R3. Décision de licence (H2).** Faut-il distribuer un paquet qui contient des décodeurs H.264 ? Recommandation : trancher avant la première release qui embarque WebEngine, en fixant une provenance unique de Qt pour le Mac.

## 8. Portes immédiates

Aucune. Le glisser réel, l'appui long et la décision de licence ne bloquent pas la suite : R1, R2, R3 (section 7).
