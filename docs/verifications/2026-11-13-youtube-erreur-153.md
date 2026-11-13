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
- un HTML au-delà de la limite de `loadHtml` (lu depuis un fichier `file:`) et un fichier local ouvert par son adresse
  restent sans `Referer` ;
- toutes les pages du lecteur partagent l'origine `https://org.actcraft.actcraft` dans le profil par défaut (éphémère) ;
- Android (`WebFrameSystemView.qml`) reçoit la même base, non essayé sur un appareil ; l'identifiant Android est
  `io.github.jejeasterix.actcraft`.

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
