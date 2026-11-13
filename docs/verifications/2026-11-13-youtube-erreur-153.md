# Erreur 153 du lecteur YouTube intégré au cadre web (OA92)

Mac (Qt 6.12.0 et Qt WebEngine 6.11.2 de Homebrew), 2026-11-13. Essai réseau : hors de la CI, à rejouer à la main.

## Cause, prouvée

Le lecteur intégré de YouTube (`youtube.com/embed/<id>`) exige un en-tête `Referer` https qui nomme l'application
(exigences minimales de YouTube, « Embedded player identification » :
https://developers.google.com/youtube/terms/required-minimum-functionality). Pour une vue web, la page donne ce
référent par sa base (`loadHtml(html, base)`). Sans lui, le lecteur affiche « Erreur de configuration du lecteur
vidéo — Erreur 153 ».

Gardes coupées, le cadre web chargeait le lecteur de trois façons, toutes sans `Referer` :

| Chargement | Base de la page | `Referer` de la requête `/embed/` | Résultat |
|---|---|---|---|
| HTML saisi, document neuf | vide (`about:blank`) | absent | Erreur 153 |
| HTML saisi, document enregistré | `file:///<dossier du document>/` | absent | Erreur 153 (capture) |
| Adresse `https://www.youtube.com/embed/<id>` saisie | page de premier niveau | absent | Erreur 153 |
| HTML saisi, base `https://…` (seule variable changée) | `https://actcraft.invalid/` | `https://actcraft.invalid/` | lecture, `currentTime` avance |

Harnais : un `WebEngineView` QML, un `QWebEngineUrlRequestInterceptor` qui imprime les en-têtes de la requête
`/embed/`, et un script de page (`runsOnSubFrames`) qui imprime `document.referrer`, `currentTime` de la vidéo et
le texte du lecteur. Relevé, base vide puis base https :

```
REQ https://www.youtube.com/embed/M7lc1UVf-VE?autoplay=1&mute=1
  firstParty=about:blank initiator=
ETAT ref= t=0.00 texte=Erreur de configuration du lecteur vidéo Erreur 153 Regarder la vidéo sur YouTube …
---
  H Referer: https://actcraft.invalid/
ETAT ref=https://actcraft.invalid/ t=11.30 texte=JEFF POSNICK: Hey, everybody.
ETAT ref=https://actcraft.invalid/ t=14.31 texte=I'm Jeff Posnick coming to you from New York City.
```

## Correctif

Le modèle (`WebFrameItems`, rôle `viewBase`) donne à la vue la base de `loadHtml`, gardes coupées :

- `https://org.actcraft.actcraft/` (identifiant de l'application, `MACOSX_BUNDLE_GUI_IDENTIFIER` en minuscules) quand
  le HTML intègre un lecteur YouTube : iframe `/embed/`, API IFrame (`iframe_api`), adresse échappée en JSON ;
- sinon la base d'avant : le dossier du document, ou rien pour un document neuf (n° 11 du cadrage, amendé).

Une adresse de lecteur saisie seule (`http` ou `https`) devient une page d'une iframe en `https`, de base l'identité.
Le titre et « Ouvrir dans le navigateur » ne montrent jamais l'identité : ils gardent l'adresse du lecteur.

Pourquoi l'identité est réservée aux pages du lecteur : une page de base `https` peint plus tard qu'une page `data:`.
Mesure au harnais, hors écran et en rendu logiciel, même HTML : capture identique à 0 ms ; à 300 ms la page `data:`
est peinte et la page `https` ne l'est pas encore ; à 1 500 ms les deux le sont. Une première version qui donnait
l'identité à tout document neuf faisait échouer `tst_ui_webengine` (`snapshotWaitsForPaintedPixels`,
`inputFieldKeepsPaste`), deux fois de suite.

Limites connues :

- un HTML qui intègre un lecteur YouTube ne lit plus ses ressources relatives au dossier du document ; une simple
  mention de l'adresse d'un lecteur dans le texte suffit à ce changement ;
- un fichier local ouvert par son adresse reste sans `Referer` ; un HTML au-delà de la limite de `loadHtml` est
  corrigé sur PC et Mac (section suivante), pas sous Android ;
- toutes les pages du lecteur partagent l'origine `https://org.actcraft.actcraft` dans le profil par défaut (éphémère) ;
- Android (`WebFrameSystemView.qml`) reçoit la même base, non essayé sur un appareil ; l'identifiant Android est
  `io.github.jejeasterix.actcraft`.

## HTML lourd : corrigé sur PC et Mac (OB52)

Un HTML saisi dont l'adresse `data:` encodée dépasse la limite de `loadHtml` (`kLoadHtmlLimit`, 2 000 000
caractères, `ui/src/webframeitem.cpp`) est écrit dans un fichier temporaire (`writeHeavyHtml`). Avant OB52, la vue
le chargeait par `web.url = contentFile` : origine `file:`, aucun `Referer`, erreur 153. Le modèle donnait pourtant
l'identité à `viewBase` (`finishDirect`), mais la vue ne la lisait pas quand `contentFile` était posé.

Correctif, dans la vue seule (`ui/qml/WebFrameEngineView.qml`) : quand `contentFile` est posé avec une base `https`
(gardes coupées), `loadHtml` charge une page d'amorce de cette base, puis le HTML y est écrit par `runJavaScript`
(`document.open`, `document.write`, `document.close`). La page garde l'origine `https://org.actcraft.actcraft`.

- L'amorce porte un repère (`data-actcraft-primer`). Tout chargement réussi qui la montre la réécrit : `load()`, le
  rechargement du moteur ou de son menu, un retour dans l'historique. Aucune autre page n'est écrite.
- L'écriture part par morceaux de 524 288 caractères, un par tour de boucle. Un jeton posé sur la fenêtre de
  l'amorce, à son ouverture, refuse d'écrire dans une page qui l'aurait remplacée. Une navigation dans le même
  document pendant l'écriture (`location.hash` d'un script) ne l'interrompt pas.
- Une coupure ne tombe jamais au milieu d'une paire de substitution : l'analyseur laisserait ses deux moitiés dans
  deux nœuds de texte (constaté par le test).
- Chaque `load()` change de génération : un rappel de `runJavaScript` parti avant lui est ignoré.
- `pageLoaded` part quand `document.readyState` vaut `complete`, comme le `load` d'une page chargée.
- L'amorce n'est pas vide : `loadHtml("", base)` charge l'adresse de base elle-même (« site inaccessible »).

Coût sur le fil de l'interface, mesuré au harnais (Mac, Qt 6.12) : d'un bloc, 15 ms pour 2,5 Mo et 170 ms pour 25 Mo ;
par morceaux, 4 ms au plus par morceau pour 25 Mo (48 morceaux). Non mesuré au banc ni sur le Dell 3510.

Piste écartée : un `QWebEngineUrlRequestInterceptor` qui pose `Referer`. Il marche (lecture vue au harnais), mais il
demande de lier Qt WebEngine à l'application, alors que le QML le charge seulement au premier cadre (DA2).

Relevé du 2026-11-13, harnais : la vraie vue `WebFrameEngineView.qml`, fichier de 2,5 Mo avec une iframe
`youtube-nocookie.com/embed/M7lc1UVf-VE?autoplay=1&mute=1`, base `https://org.actcraft.actcraft/`, capture à 15 s.

```
avant : REQ …/embed/M7lc1UVf-VE…  firstParty=file:///…/cadre-lourd.html   (aucun Referer)  → capture : Erreur 153
après : REQ …/embed/M7lc1UVf-VE…  firstParty=https://org.actcraft.actcraft/
          H Referer: https://org.actcraft.actcraft/
        capture à 9 s : logo d'introduction de la vidéo ; à 15 s : l'intervenant, sous-titres affichés
```

Même résultat avec un fichier de 25 Mo.

Part déterministe, dans la CI : `tst_ui_webengine` (`heavyPlayerHtmlKeepsTheIdentityOrigin` : HTML de 3 Mo par
`contentFile`, origine `https://org.actcraft.actcraft`, contenu entier malgré un `location.hash` posé pendant
l'écriture, paire de substitution à la coupure d'un morceau restée dans un seul nœud, puis même résultat, titre déjà écrit au `pageLoaded` et un seul `pageLoaded` après un rechargement du moteur).

Limites : Android (`WebFrameSystemView.qml`) charge encore le fichier, sans `Referer`. Une page lourde du lecteur ne
lit pas ses ressources relatives au dossier du document, comme une page moyenne du lecteur.

## Essai réel, dans l'application

```sh
QTWEBENGINE_REMOTE_DEBUGGING=9333 build/debug/app/actcraft.app/Contents/MacOS/actcraft \
  --essai-cadre-web "https://www.youtube.com/embed/M7lc1UVf-VE?autoplay=1&mute=1"
```

Puis lire l'état du lecteur par le protocole DevTools (`Runtime.enable`, contexte d'origine `https://www.youtube.com`,
`Runtime.evaluate`). Relevé du 2026-11-13, toutes les 4 s :

```
ACTCRAFT ESSAI CADRE WEB POSE
qml: ACTCRAFT CADRE WEB CHARGE
[0] https://www.youtube.com ref=https://org.actcraft.actcraft/ t=3.24 (lecture) texte=YouTube Developers Live: …
[2] https://www.youtube.com ref=https://org.actcraft.actcraft/ t=11.26 (lecture) texte=JEFF POSNICK: Hey, everybody.
[5] https://www.youtube.com ref=https://org.actcraft.actcraft/ t=23.28 (lecture) texte=different ways of customizing the YouTube-embedded player.
```

La capture de la page (`Page.captureScreenshot`) montre la vidéo en cours, sous-titres affichés.

Part déterministe, dans la CI : `tst_ui_webframe` (base donnée à la vue, détection du lecteur, HTML moyen, page du
lecteur, adresse sans schéma, gardes, titre et adresse externe) et `tst_ui_webframelayer` (`viewBaseGoesToTheView` : la couche passe cette base à la vue).
