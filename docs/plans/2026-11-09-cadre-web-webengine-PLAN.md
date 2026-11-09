# Plan d'exécution — cadre-web-webengine (Qt WebEngine dans le cadre web)

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**Objectif :** remplacer Qt WebView par Qt WebEngine sur PC et Mac (JavaScript actif, glisser, sélection, capture), garder Qt WebView sur Android, couper les gardes de sécurité par défaut derrière un seul réglage, et accepter le dépôt et le collage d'images web.

**Architecture :** deux composants QML au même contrat (`WebFrameEngineView.qml`, `WebFrameSystemView.qml`), un seul construit par plateforme, chargés par un `Loader` asynchrone au premier cadre ; WebEngine n'est pas lié à l'application. L'interrupteur `WebFrameItems.guardsEnabled` choisit entre le chemin direct (adresse ou HTML donnés tels quels à la vue) et le chemin actuel (lecture et réécriture par le modèle). Le téléchargement des images vit dans une classe `WebImageFetch` de `ui/`.

**Tech Stack :** C++20, Qt 6.12 Quick, Qt WebEngine (formule Homebrew 6.11.2_1 reconstruite contre 6.12), Qt WebView (Android), Qt Network, Qt Test, bash.

**Spec :** `docs/specs/2026-11-09-cadre-web-webengine-cadrage.md` (commit ea065489).

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| T1 | Contrat, Loader, construction, premier vrai cadre | — | CMakeLists.txt, ui/CMakeLists.txt, ui/qml/WebFrameLayer.qml, ui/qml/WebFrameEngineView.qml, ui/qml/WebFrameSystemView.qml, ui/include/actcraft/ui/webframeitem.h, ui/src/webframeitem.cpp, ui/include/actcraft/ui/webframeengine.h, ui/src/webframeengine.cpp, app/main.cpp, bench/app/main.cpp, bench/captures/main.cpp, tests/support/testserver.h, tests/support/webframehost.h, tests/qml/WebFrameHost.qml, tests/qml/FakeWebFrameView.qml, tests/qml/AbsentModuleView.qml, tests/tst_ui_webframe.cpp, tests/tst_ui_webengine.cpp, tests/tst_ui_webframelayer.cpp, tests/ui.cmake | partagé | concepteur-critique | tst_ui_webengine ≥ 4 tests, `realFrameLoadsThroughTheLoader` PASS ; tst_ui_webframelayer `contractsMatch` PASS ; suite complète ; JSON du banc | S1 | #tache-t1 |
| T2 | Interrupteur des gardes et réglage de profil | T1 | ui/include/actcraft/ui/webframeitem.h, ui/src/webframeitem.cpp, ui/src/settingsschema.cpp, ui/qml/WebFrameLayer.qml, ui/qml/PageArea.qml, ui/qml/Main.qml, ui/qml/DesktopAnnotation.qml, ui/qml/Presentation.qml, ui/i18n/actcraft_fr.ts, tests/tst_ui_webframe.cpp, tests/tst_ui_desktop.cpp | partagé | implementeur-tdd | tst_ui_webframe `guardsFollowTheProfileSetting` PASS ; tst_ui_desktop `desktopLayerFollowsTheSetting` PASS ; suite complète | S3 | #tache-t2 |
| T14 | Étape `setting` du joueur et scénario cadre-web | T1 | bench/include/actcraft/bench/uiscenario.h, bench/src/uiscenario.cpp, bench/scenarios/cadre-web.json, tests/tst_bench_uiscenario.cpp | partagé | implementeur-tdd | tst_bench_uiscenario `settingStepIsParsed` PASS ; tst_captures_cadre_web `SCENARIO OK` ; suite complète | S3 | #tache-t14 |
| T15 | Licences et notices de Qt WebEngine | T1 | packaging/qt-autorises.txt, THIRD_PARTY_NOTICES.md, scripts/verifier-licences-mac.sh, CLAUDE.md | partagé | implementeur-tdd | tst_app_notices `everyAllowedQtModuleHasANotice` PASS ; `LICENCES OK` sur bundle factice | S4 | #tache-t15 |
| T3 | Modèle, gardes coupées : adresse directe, HTML lourd | T2, T14 | ui/include/actcraft/ui/webframeitem.h, ui/src/webframeitem.cpp, tests/tst_ui_webframe.cpp | partagé | concepteur-critique | tst_ui_webframe `guardsOffSendsNoRequest`, `heavyHtmlGoesToAFile` PASS ; suite complète | S1 | #tache-t3 |
| T4 | Historique du moteur rapporté au modèle | T3 | ui/include/actcraft/ui/webframeitem.h, ui/src/webframeitem.cpp, tests/tst_ui_webframe.cpp | partagé | implementeur-tdd | tst_ui_webframe `reportFillsTitleAndHistory`, `guardsOffCommandsGoToTheView` PASS ; suite complète | S1 | #tache-t4 |
| T9 | Téléchargement borné `WebImageFetch` | T1 | ui/include/actcraft/ui/webimagefetch.h, ui/src/webimagefetch.cpp, ui/CMakeLists.txt, tests/tst_ui_webimagefetch.cpp, tests/ui.cmake | partagé | implementeur-tdd | tst_ui_webimagefetch ≥ 7 tests, `oversizeWithoutLengthIsRefused` PASS ; suite complète | S2 | #tache-t9 |
| T10 | Insertion : dépôt web, `Mode::Downloaded` | T4, T9 | ui/include/actcraft/ui/insertion.h, ui/src/insertion.cpp, ui/include/actcraft/ui/webframeitem.h, ui/src/webframeitem.cpp, tests/tst_ui_insertion.cpp | partagé | concepteur-critique | tst_ui_insertion `webDropDownloadsAnImage`, `downloadNeverLandsInAnotherTab` PASS ; suite complète | S2 | #tache-t10 |
| T11 | Coller une URL d'image | T10 | ui/include/actcraft/ui/objectediting.h, ui/src/objectediting.cpp, ui/qml/Main.qml, tests/tst_ui_objectediting.cpp | partagé | implementeur-tdd | tst_ui_objectediting `pasteImageAddressDownloads`, `pasteNonImagePageBecomesText` PASS ; suite complète | S2 | #tache-t11 |
| T5 | Couche branchée sur le vrai moteur | T2, T4 | ui/qml/WebFrameLayer.qml, ui/qml/WebFrameEngineView.qml, tests/qml/FakeWebFrameView.qml, tests/tst_ui_webframelayer.cpp, tests/tst_ui_webengine.cpp | partagé | concepteur-critique | tst_ui_webengine `javaScriptRunsGuardsOff`, `cookieStaysInItsDocument`, `heavyHtmlLoads` PASS ; suite complète ; JSON du banc ; lectures du joueur | S1, S3 | #tache-t5 |
| T6 | Capture du cadre (`insertSnapshot`) | T5 | ui/include/actcraft/ui/webframeitem.h, ui/src/webframeitem.cpp, ui/qml/WebFrameLayer.qml, tests/tst_ui_webframe.cpp, tests/tst_ui_webengine.cpp | partagé | implementeur-tdd | tst_ui_webengine `snapshotInsertsAnImage` PASS ; tst_ui_webframe `insertSnapshotIsOneStep` PASS ; suite complète | S1 | #tache-t6 |
| T7 | Stylet : couverture d'entrée de la vue | T6 | ui/qml/WebFrameLayer.qml, tests/tst_ui_pageinput.cpp, tests/ui.cmake | partagé | implementeur-tdd | tst_ui_pageinput `stylusOnWebFrameDrawsNothing` PASS ; suite complète ; JSON du banc | S1 | #tache-t7 |
| T8 | Textes selon les gardes | T7 | ui/qml/WebFrameLayer.qml, ui/qml/WebFrameDialog.qml, ui/i18n/actcraft_fr.ts, tests/tst_ui_webframe.cpp | partagé | implementeur-tdd | tst_ui_webframe `dialogNoteFollowsTheGuards` PASS ; suite complète | S3 | #tache-t8 |
| T12 | Récepteur de dépôt sur la page | T8, T11 | ui/qml/WebFrameLayer.qml, ui/qml/PageArea.qml, ui/qml/Main.qml, tests/tst_ui_webdrop.cpp, tests/tst_ui_desktop.cpp, tests/ui.cmake | partagé | implementeur-tdd | tst_ui_webdrop ≥ 5 tests, `droppedRemoteImageSurvivesReopening` PASS ; suite complète ; JSON du banc | S1 | #tache-t12 |
| T13 | Copier la sélection du cadre | T12 | ui/qml/WebFrameLayer.qml, ui/qml/PageArea.qml, ui/qml/Main.qml, tests/tst_ui_webengine.cpp, tests/tst_ui_shell.cpp | partagé | implementeur-tdd | tst_ui_webengine `editCopyCopiesTheSelection`, `inputFieldKeepsPaste` PASS ; tst_ui_shell `editCopyWithoutFocusedFrameCopiesObjects` PASS ; suite complète | S1 | #tache-t13 |
| T16 | Paquet Mac et essai de l'image | T13, T15 | scripts/paquet-mac.sh, scripts/essai-dmg.sh, app/startup.h, app/startup.cpp, tests/tst_app_open.cpp | partagé | concepteur-critique | tst_app_open `parsesWebFrameProbe` PASS ; `DMG OK` ; `ESSAI DMG OK` avec `CADRE WEB OK` | S4 | #tache-t16 |
| T17 | Modules de la CI et APK sans WebEngine | T13 | .github/workflows/ci.yml, .github/workflows/preuves-windows.yml | isolé | implementeur-tdd | `APK OK` ; `unzip -l` : greffon `QtWebView` présent, aucun `QtWebEngine` | S5 | #tache-t17 |
| T18 | Mesures avant/après et preuve finale | T16, T17 | docs/mesures/2026-11-09-cadre-web-webengine.md | partagé | implementeur-tdd | `.dmg` avant et après ; `ESSAI DMG OK` ; relevé H.264 ; suite complète | S4 | #tache-t18 |

**Vagues** (dans cet ordre) : T1 · T2 · T14 · T15 · T3 · T4 · T9 · T10 · T11 · T5 · T6 · T7 · T8 · T12 · T13 · T16 + T17 · T18.

`partagé` désigne le worktree du chantier (`build-mac` réutilisé, construction incrémentale). Une tâche seule dans sa vague y travaille aussi : sur 8 Go, un worktree isolé par tâche coûterait une reconstruction complète à `-j2`, sans gain puisque le jeton `tests` sérialise toute construction. Seule T17 est isolée : ses fichiers ne recoupent pas ceux de T16 et l'APK se construit à part.

## Global Constraints

- Preuves sur ce Mac uniquement, avec `build-mac` : `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1`, sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/<id> --attente-secondes 3000 --commande "<commande>"`. Une seule tâche qui construit à la fois (8 Go de RAM).
- Format : `~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror` sur les fichiers touchés.
- Le rouge préexistant instable `tst_ui_linklauncher` (`htaRefused`) n'est pas une régression.
- `.github/workflows/release.yml` n'est jamais modifié (O98) ; ce qu'il devra recevoir est dans « Sortie pour O98 ».
- Aucune machine GitHub, aucun job sur le PC, pas de release ni de tag ; ce qui exige Windows ou Android part en « Restes pour le PC ».
- `core/`, `formats/`, `actions/` modifiés ont leurs tests (aucune tâche de ce plan n'y touche) ; tâche d'interface : test de chargement, et JSON du banc hors écran si le banc tourne sur Mac (`scripts/bench.sh --rendu logiciel --hors-ecran`).
- Le code ne contient aucun nom repris d'un produit existant ; aucune ressource de Promethean.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. `graphify-out/` n'est mis à jour qu'à la clôture, par l'orchestrateur.
- Banc de `ui/` : résultat, pas porte (cadrage n° 36). Pour le coût machine, il tourne sur les tâches qui changent la scène ou l'entrée (T1, T5, T7, T12) et à T18, pas sur les tâches du modèle.

## Review Focus

1. Contenu changé pendant le chargement asynchrone du `Loader` (cadrage n° 5) : la vue reçoit le dernier contenu. Test `deliversTheLatestSerialAtReady`, T1.
2. Réglage basculé pendant qu'un HTML lourd s'écrit (n° 10, n° 38) : le résultat périmé est abandonné, son fichier retiré. Test `toggleDuringHeavyWriteDropsTheFile`, T3.
3. Réponse sans `Content-Length` qui dépasse 25 Mo (n° 25) : arrêt et message. Test `oversizeWithoutLengthIsRefused`, T9.
4. Fichier temporaire du moteur disparu avant la lecture (n° 20) : message, document intact. Test `vanishedDropFileFails`, T10.
5. Texte collé qui ressemble à une URL sans en être une : deux lignes, espace, schéma non web, hôte vide (n° 22, E9) : objet texte, sans téléchargement. Test `pasteNotQuiteAnAddressStaysText`, T11.

## Carte des fichiers

| Fichier | Rôle |
|---|---|
| `ui/qml/WebFrameEngineView.qml` (créé) | vue PC et Mac : `WebEngineView`, gardes, rapport |
| `ui/qml/WebFrameSystemView.qml` (créé) | vue Android : `WebView`, cookies vidés |
| `ui/qml/WebFrameLayer.qml` | `Loader`, livraison du contenu, couverture, capture, dépôt |
| `ui/include/actcraft/ui/webframeengine.h`, `ui/src/webframeengine.cpp` (créés) | `prepareWebFrameEngine()` avant `QGuiApplication` |
| `ui/include/actcraft/ui/webframeitem.h`, `ui/src/webframeitem.cpp` | interrupteur, chemin direct, rapport de vue, `insertSnapshot` |
| `ui/include/actcraft/ui/webimagefetch.h`, `ui/src/webimagefetch.cpp` (créés) | téléchargement borné |
| `ui/include/actcraft/ui/insertion.h`, `ui/src/insertion.cpp` | `acceptWebDrop`, `Mode::Downloaded`, suivi de l'origine |
| `ui/src/objectediting.cpp` | collage d'une URL |
| `tests/support/testserver.h` (créé) | serveur HTTP local extrait de `tst_ui_webframe.cpp` |
| `tests/support/webframehost.h`, `tests/qml/*` (créés) | hôte minimal de la couche, faux composant |
| `tests/tst_ui_webengine.cpp` (créé) | seul fichier de tests dédié au vrai moteur |
| `tests/tst_ui_webframelayer.cpp`, `tests/tst_ui_webimagefetch.cpp`, `tests/tst_ui_webdrop.cpp` (créés) | couche sans moteur, téléchargement, dépôt |

## Sortie pour O98

`.github/workflows/release.yml` n'est pas modifié par ce chantier. O98 devra y ajouter `qtwebengine qtwebchannel qtpositioning` :

- ligne 194 (Windows) ;
- ligne 289 (`aqt` Mac) ;
- ligne 314 (`modules=` du repli Mac mini).

Commentaires à relire : lignes 273, 294, 312, 339. Lignes 415-416 (hôte et Android) : rien à changer. T18 recopie cette liste dans `docs/mesures/2026-11-09-cadre-web-webengine.md`.

## Restes pour le PC et pour Jérôme

Listés au cadrage, section 7 (PC 1 à 8, R1 à R3). Aucune tâche de ce plan ne les exécute. S'y ajoute au Reste PC 4 : retirer le repli de `WebFrameItems::engineKind` sous Windows hors écran si la preuve PC passe (T1).

---

### Tâche T1 — Contrat, Loader, construction, premier vrai cadre
<a id="tache-t1"></a>

**Objectif.** Poser le contrat commun des deux vues, le `Loader` asynchrone et la détection de WebEngine, avec une preuve réelle dans le projet : un cadre inséré dans la vraie fenêtre charge sa page dans le vrai moteur, chargé depuis un module statique (cadrage n° 3 à 6, 8, 32, DA1, DA2, DA3 ; la sonde `Loader` n'existait qu'hors dépôt).

**Fichiers.**
- Créer : `ui/qml/WebFrameEngineView.qml`, `ui/qml/WebFrameSystemView.qml`, `ui/include/actcraft/ui/webframeengine.h`, `ui/src/webframeengine.cpp`, `tests/support/testserver.h`, `tests/support/webframehost.h`, `tests/qml/WebFrameHost.qml`, `tests/qml/FakeWebFrameView.qml`, `tests/qml/AbsentModuleView.qml`, `tests/tst_ui_webengine.cpp`, `tests/tst_ui_webframelayer.cpp`.
- Modifier : `CMakeLists.txt:49-56`, `ui/CMakeLists.txt` (liste QML, liste des sources, lignes du lien WebView), `ui/qml/WebFrameLayer.qml:150-201`, `ui/include/actcraft/ui/webframeitem.h:18-28,75`, `ui/src/webframeitem.cpp`, `app/main.cpp:14-16,38-40`, `bench/app/main.cpp:21-23,64-66`, `bench/captures/main.cpp:29-31,110-112`, `tests/tst_ui_webframe.cpp:44-46,53-139,845-850,1222-1224`, `tests/ui.cmake`. Les numéros de ligne cités sont ceux de la base 76315dbc.

**Interfaces.**
- Consomme : `actcraft_add_test` (`tests/CMakeLists.txt:2`), `presentationharness::MainWindow`, `presentationharness::findItem` (`tests/support/presentationharness.h:112,134`).
- Produit :
  - `void actcraft::ui::prepareWebFrameEngine();` (`<actcraft/ui/webframeengine.h>`) ;
  - `Q_PROPERTY(QString engineKind READ engineKind CONSTANT)`, `static QString WebFrameItems::engineKind();` valeurs `"engine"`, `"system"`, `"none"` ;
  - contrat des vues (membres racine identiques dans les trois fichiers QML ; tout membre dont le nom commence par `_` est privé et n'en fait pas partie) :
    `property string html`, `property url baseUrl`, `property url directUrl`, `property url contentFile`, `property int serial`, `property bool guardsEnabled`, `readonly property bool overlaysScene`, `readonly property bool viewFocused`, `signal viewReported(url pageUrl, bool canBack, bool canForward, string pageTitle, bool pageLoading)`, `signal navigationTaken(url pageUrl)`, `signal pageLoaded()`, `function load()`, `function stop()`, `function goBack()`, `function goForward()`, `function reload()`, `function capture(callback)`, `function copySelection()` ;
  - sur `WebFrameLayer` : `property url viewSource`, `property int loadedViews`, `property int snapshotCount` ; `objectName` : `webFrameView` (racine de la vue), `webFrameEngine` (`WebEngineView` interne), `webFrameViewHost`, `webFrameLoader` ;
  - `TestServer` (`tests/support/testserver.h`, espace global) : les méthodes actuelles, plus `void header(const QString& path, const QByteArray& line)`, `QByteArray lastRequest(const QString& path) const`, `void rawResponse(const QString& path, const QByteArray& response)` ;
  - `struct WebFrameHost` (`tests/support/webframehost.h`) : `bool start()`, `void useFakeView()`, membres `document`, `window`, `root`, `layer`, `frames`.

**Test et commande bornée.**
```bash
mkdir -p .tmp
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T1 --attente-secondes 3000 --commande "cmake -S . -B build-mac && cmake --build build-mac -j2"
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T1 --attente-secondes 3000 --commande "QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_ui_(webengine|webframelayer|webframe)$' --output-on-failure --timeout 300 -V > .tmp/t1.txt 2>&1"
grep -E 'PASS   : TstUiWebEngine::realFrameLoadsThroughTheLoader|PASS   : TstUiWebFrameLayer::contractsMatch|^Totals:|tests passed|tests failed' .tmp/t1.txt
```
Motif attendu : les deux lignes `PASS`, aucun `tests failed`, `Totals:` avec au moins 1 passé par fichier.

**Preuve visuelle.** Aucune capture à cette tâche : `loadedViews` et `title` lus par le test valent plus qu'une image.

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, avec `build-mac` ; `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1`, sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T1 --attente-secondes 3000 --commande "<commande>"`. Une seule tâche qui construit à la fois (8 Go de RAM).
- Format : `~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror` sur les fichiers touchés.
- `tst_ui_linklauncher` (`htaRefused`) : rouge préexistant instable, pas une régression.
- Aucune machine GitHub, aucun job sur le PC ; ce qui exige Windows ou Android part en « Restes pour le PC ».
- Tâche d'interface : test de chargement et JSON du banc hors écran (`scripts/bench.sh --rendu logiciel --hors-ecran`), résultat et non porte.
- Aucun nom repris d'un produit existant ; aucune ressource de Promethean.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** Ne pas lier `Qt6::WebEngineQuick`, ne pas appeler `QtWebEngineQuick::initialize()`, ne pas changer `CMAKE_PREFIX_PATH`. Ne pas brancher ici les gardes du modèle : la couche passe `guardsEnabled: true` jusqu'à T5.

**Patron à ne pas reprendre.** `Qt.createQmlObject` avec `import QtWebView` dans une chaîne (`ui/qml/WebFrameLayer.qml:164-176`) : l'import devient invisible de `qmlimportscanner`, la création est synchrone (199 à 797 ms de fil bloqué, DA2) et le contenu part au seul changement de `serial`, donc se perdrait pendant le chargement. Autre patron écarté : une classe C++ autour de `QQuickWebEngineView`, API privée (`qquickwebengineview_p.h:98`, cadrage n° 3).

**Critère « terminé ».** Les quatre tests de `tst_ui_webengine` et ceux de `tst_ui_webframelayer` passent, `tst_ui_webframe` reste vert, suite complète sans autre rouge que `tst_ui_linklauncher`, banc joué, commits faits.

**Étapes.**

- [ ] **Étape 1 : extraire le serveur d'essai.** Créer `tests/support/testserver.h` avec en tête `#pragma once` et les includes `<QByteArray> <QHash> <QHostAddress> <QList> <QObject> <QPointer> <QString> <QStringList> <QTcpServer> <QTcpSocket> <QUrl> <utility>`. Y déplacer tel quel `class TestServer` (`tests/tst_ui_webframe.cpp:53-139`), hors de tout espace de noms. Y ajouter :

```cpp
    // Ligne d'en-tête ajoutée à la réponse de ce chemin (« Set-Cookie: k=v »).
    void header(const QString& path, const QByteArray& line) { m_headers.insert(path, line); }
    // Requête brute reçue en dernier pour ce chemin (tests de cookies).
    QByteArray lastRequest(const QString& path) const { return m_lastRequests.value(path); }
    // Réponse complète, en-têtes compris, envoyée telle quelle.
    void rawResponse(const QString& path, const QByteArray& response) { m_raw.insert(path, response); }
```
Dans `read()`, garder la requête avant le retrait du tampon, servir `m_raw` d'abord et insérer l'en-tête :

```cpp
        const QByteArray raw = buffer;
        const QList<QByteArray> line = buffer.left(buffer.indexOf("\r\n")).split(' ');
        m_buffers.remove(socket);
        const QString path = line.size() > 1 ? QString::fromLatin1(line.at(1)) : QString();
        m_paths.append(path);
        m_lastRequests.insert(path, raw);
        QByteArray response;
        if (m_raw.contains(path)) {
            response = m_raw.value(path);
        } else if (m_redirects.contains(path)) {
            response = "HTTP/1.1 302 Found\r\nLocation: " + m_redirects.value(path).toEncoded() +
                       "\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        } else if (m_pages.contains(path)) {
            const QByteArray body = m_pages.value(path);
            const QByteArray extra =
                m_headers.contains(path) ? m_headers.value(path) + QByteArray("\r\n") : QByteArray();
            response = "HTTP/1.1 200 OK\r\n" + extra +
                       "Content-Type: text/html; charset=utf-8\r\nCache-Control: no-store\r\nContent-Length: " +
                       QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n" + body;
        } else {
```
Membres ajoutés : `QHash<QString, QByteArray> m_headers; QHash<QString, QByteArray> m_lastRequests; QHash<QString, QByteArray> m_raw;`. Dans `tests/tst_ui_webframe.cpp`, remplacer les lignes 53-139 par rien et ajouter `#include "support/testserver.h"` avec les includes locaux.

- [ ] **Étape 2 : écrire l'hôte et le faux composant.** `tests/qml/WebFrameHost.qml` :

```qml
import QtQuick
import ActCraft.Ui

// Zone de page minimale : ce que WebFrameLayer lit de PageArea, sans page ni encre.
Item {
    id: host
    objectName: "pageArea"
    width: 1024
    height: 768
    property DocumentController documentController: null
    property bool inputBlocked: false
    property var profile: null
    signal popupOpened()
    signal popupClosed()
    readonly property alias view: fakeView

    Item {
        id: fakeView
        anchors.fill: parent
        readonly property real zoom: 1
        readonly property point pan: Qt.point(0, 0)
        function pageToScene(p) { return mapToItem(null, p.x, p.y) }
        function sceneToPage(p) { return mapFromItem(null, p.x, p.y) }
        function addInputCover(item) {}
        function removeInputCover(item) {}
    }
    WebFrameLayer {
        objectName: "webFrameLayer"
    }
}
```
`tests/qml/FakeWebFrameView.qml` :

```qml
import QtQuick

// Même contrat que WebFrameEngineView.qml et WebFrameSystemView.qml ; membres privés préfixés de « _ ».
Item {
    id: root
    objectName: "webFrameView"
    property string html: ""
    property url baseUrl: ""
    property url directUrl: ""
    property url contentFile: ""
    property int serial: 0
    property bool guardsEnabled: false
    readonly property bool overlaysScene: _overlays
    readonly property bool viewFocused: _focused

    signal viewReported(url pageUrl, bool canBack, bool canForward, string pageTitle, bool pageLoading)
    signal navigationTaken(url pageUrl)
    signal pageLoaded()

    property int _loads: 0
    property var _calls: []
    property bool _overlays: false
    property bool _focused: false

    function load() { _loads += 1 }
    function stop() { _calls.push("stop") }
    function goBack() { _calls.push("goBack") }
    function goForward() { _calls.push("goForward") }
    function reload() { _calls.push("reload") }
    function capture(callback) { callback(null) }
    function copySelection() { _calls.push("copySelection") }
}
```
`tests/qml/AbsentModuleView.qml` :

```qml
import QtQuick
import ActCraftModuleQuiNExistePas

Item {}
```
`tests/support/webframehost.h` :

```cpp
#pragma once

#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/webframeitem.h>

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>
#include <QUrl>

#include <memory>

// WebFrameLayer dans une fenêtre hors écran, sans Main.qml. ACTCRAFT_SOURCE_DIR : posé par tests/ui.cmake.
struct WebFrameHost
{
    QQmlEngine engine;
    QQuickWindow window;
    actcraft::ui::DocumentController document;
    std::unique_ptr<QQuickItem> root;
    QQuickItem* layer = nullptr;
    actcraft::ui::WebFrameItems* frames = nullptr;

    bool start()
    {
        document.newDocument(1024, 768);
        QQmlComponent component(&engine,
                                QUrl::fromLocalFile(QStringLiteral(ACTCRAFT_SOURCE_DIR "/tests/qml/WebFrameHost.qml")));
        root.reset(qobject_cast<QQuickItem*>(component.create()));
        if (!root) {
            qWarning() << component.errors();
            return false;
        }
        root->setProperty("documentController", QVariant::fromValue(&document));
        window.resize(1024, 768);
        root->setParentItem(window.contentItem());
        window.show();
        if (!QTest::qWaitForWindowExposed(&window))
            return false;
        layer = root->findChild<QQuickItem*>(QStringLiteral("webFrameLayer"));
        frames = layer ? layer->findChild<actcraft::ui::WebFrameItems*>(QStringLiteral("appWebFrames")) : nullptr;
        return frames != nullptr;
    }
    void useFakeView()
    {
        layer->setProperty("viewSource",
                           QUrl::fromLocalFile(QStringLiteral(ACTCRAFT_SOURCE_DIR "/tests/qml/FakeWebFrameView.qml")));
    }
};
```

- [ ] **Étape 3 : écrire les tests qui échouent.** `tests/tst_ui_webframelayer.cpp` :

```cpp
// Couche des cadres web sans moteur : contrat des vues, livraison du contenu au Loader (cadrage n° 3, 5, 30).
#include "support/presentationharness.h"
#include "support/webframehost.h"

#include <QFile>
#include <QRegularExpression>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using presentationharness::findItem;

namespace {

QStringList members(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    static const QRegularExpression member(
        QStringLiteral(R"re(^    (?:readonly )?(property \S+ (?!_)\w+|signal (?!_)\w+\([^)]*\)|function (?!_)\w+\([^)]*\)))re"),
        QRegularExpression::MultilineOption);
    QStringList found;
    const QString text = QString::fromUtf8(file.readAll());
    for (auto it = member.globalMatch(text); it.hasNext();)
        found << it.next().captured(1);
    found.sort();
    return found;
}

QString source(const char* relative)
{
    return QStringLiteral(ACTCRAFT_SOURCE_DIR "/") + QString::fromLatin1(relative);
}

} // namespace

class TstUiWebFrameLayer : public QObject
{
    Q_OBJECT
private slots:
    // Le contrat vit en QML, sans contrôle du compilateur (DA1) : les trois composants sont comparés.
    void contractsMatch()
    {
        const QStringList engine = members(source("ui/qml/WebFrameEngineView.qml"));
        QVERIFY(engine.contains(QStringLiteral("function load()")));
        QVERIFY(engine.contains(QStringLiteral("property url directUrl")));
        QCOMPARE(members(source("ui/qml/WebFrameSystemView.qml")), engine);
        QCOMPARE(members(source("tests/qml/FakeWebFrameView.qml")), engine);
    }

    void contentGoesOutAtLoaderReady()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        host.useFakeView();
        QVERIFY(host.frames->insertFrame(QStringLiteral("<p>bonjour</p>"), {}));
        QTRY_VERIFY(findItem(host.root.get(), QStringLiteral("webFrameView")));
        QQuickItem* view = findItem(host.root.get(), QStringLiteral("webFrameView"));
        QTRY_COMPARE(view->property("_loads").toInt(), 1);
        QVERIFY(view->property("html").toString().contains(QStringLiteral("bonjour")));
    }

    // Review Focus 1 (cadrage n° 5) : la vue reçoit le dernier contenu, pas celui du premier changement.
    void deliversTheLatestSerialAtReady()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        host.useFakeView();
        QVERIFY(host.frames->insertFrame(QStringLiteral("<p>a</p>"), {}));
        host.frames->reload(0);
        host.frames->reload(0);
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameView")), 5000);
        QQuickItem* view = findItem(host.root.get(), QStringLiteral("webFrameView"));
        const QModelIndex row = host.frames->index(0, 0);
        QTRY_COMPARE(view->property("serial").toInt(),
                     host.frames->data(row, actcraft::ui::WebFrameItems::SerialRole).toInt());
        QVERIFY(view->property("_loads").toInt() >= 1);
    }
};

QTEST_MAIN(TstUiWebFrameLayer)
#include "tst_ui_webframelayer.moc"
```
`tests/tst_ui_webengine.cpp` :

```cpp
// Le vrai moteur (cadrage n° 30) : lancé hors écran sur Mac, saut nommé sous Windows hors écran (Reste PC 4).
#include "startup.h"
#include "support/presentationharness.h"
#include "support/testserver.h"
#include "support/webframehost.h"

#include <actcraft/ui/webframeengine.h>

#include <QGuiApplication>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using presentationharness::findItem;

class TstUiWebEngine : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
#ifdef Q_OS_WIN
        if (QGuiApplication::platformName() == QLatin1String("offscreen"))
            QSKIP("Qt WebEngine hors écran sous Windows : preuve au PC (Reste PC 4)");
#endif
    }

    // Premier test du fichier : l'environnement est relu au lancement du premier moteur.
    void prepareSetsTheProcessLimitOnlyWhenEmpty()
    {
        qunsetenv("QTWEBENGINE_CHROMIUM_FLAGS");
        actcraft::ui::prepareWebFrameEngine();
        QCOMPARE(qgetenv("QTWEBENGINE_CHROMIUM_FLAGS"), QByteArray("--renderer-process-limit=2"));
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--autre");
        actcraft::ui::prepareWebFrameEngine();
        QCOMPARE(qgetenv("QTWEBENGINE_CHROMIUM_FLAGS"), QByteArray("--autre"));
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--renderer-process-limit=2");
    }

    // Aucun coût tant qu'aucun cadre n'est montré (O146).
    void noViewBeforeTheFirstFrame()
    {
        presentationharness::MainWindow main;
        QVERIFY(main.window());
        QVERIFY(main.item(QStringLiteral("webFrameLayer")));
        QCOMPARE(main.item(QStringLiteral("webFrameEngine")), nullptr);
    }

    // Preuve réelle : Loader dans le module statique, import différé, vraie page chargée.
    void realFrameLoadsThroughTheLoader()
    {
        presentationharness::MainWindow main;
        QVERIFY(main.window());
        QQuickItem* layer = main.item(QStringLiteral("webFrameLayer"));
        QVERIFY(layer);
        QObject* frames = layer->findChild<QObject*>(QStringLiteral("appWebFrames"));
        QVERIFY(frames);
        bool inserted = false;
        QVERIFY(QMetaObject::invokeMethod(frames, "insertFrame", Q_RETURN_ARG(bool, inserted),
                                          Q_ARG(QString, QStringLiteral("<title>CADRE OK</title><p>x</p>")),
                                          Q_ARG(QString, QString())));
        QVERIFY(inserted);
        QTRY_COMPARE_WITH_TIMEOUT(layer->property("loadedViews").toInt(), 1, 60000);
        QQuickItem* web = main.item(QStringLiteral("webFrameEngine"));
        QVERIFY(web);
        QCOMPARE(web->property("title").toString(), QStringLiteral("CADRE OK"));
    }

    // Module absent du paquet : même branche qu'aujourd'hui, avertissement et cadre vide, sans plantage (n° 5).
    void loaderErrorLeavesAnEmptyFrame()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        host.layer->setProperty(
            "viewSource", QUrl::fromLocalFile(QStringLiteral(ACTCRAFT_SOURCE_DIR "/tests/qml/AbsentModuleView.qml")));
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral(".*Cadre web : vue indisponible.*")));
        QVERIFY(host.frames->insertFrame(QStringLiteral("<p>a</p>"), {}));
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameLoader")), 5000);
        QQuickItem* loader = findItem(host.root.get(), QStringLiteral("webFrameLoader"));
        QTRY_COMPARE_WITH_TIMEOUT(loader->property("status").toInt(), 3 /* Loader.Error */, 10000);
        QCOMPARE(findItem(host.root.get(), QStringLiteral("webFrameView")), nullptr);
    }
};

int main(int argc, char** argv)
{
    actcraft::ui::prepareWebFrameEngine();
    QGuiApplication app(argc, argv);
    TstUiWebEngine test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_ui_webengine.moc"
```
Dans `tests/ui.cmake`, ajouter à la fin :

```cmake
# Cadre web (chantier cadre-web-webengine) : la couche sans moteur, puis le vrai moteur, seul à la fois (mémoire).
actcraft_add_test(tst_ui_webframelayer
    SOURCES tst_ui_webframelayer.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_webframelayer PRIVATE ${PROJECT_SOURCE_DIR}/app)
target_compile_definitions(tst_ui_webframelayer PRIVATE ACTCRAFT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
if(ACTCRAFT_WEBFRAME AND NOT ANDROID)
    actcraft_add_test(tst_ui_webengine
        SOURCES tst_ui_webengine.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
        LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
    target_include_directories(tst_ui_webengine PRIVATE ${PROJECT_SOURCE_DIR}/app)
    target_compile_definitions(tst_ui_webengine PRIVATE ACTCRAFT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
    set_tests_properties(tst_ui_webengine tst_ui_webframe PROPERTIES RUN_SERIAL TRUE TIMEOUT 300)
endif()
```

- [ ] **Étape 4 : vérifier le rouge.** Configuration et construction : échec attendu sur `actcraft/ui/webframeengine.h` absent (`fatal error: 'actcraft/ui/webframeengine.h' file not found`).

- [ ] **Étape 5 : détection et construction.** Dans `CMakeLists.txt`, remplacer les lignes 49-55 :

```cmake
# Cadre web : PC et Mac, Qt WebEngine, seulement détecté (le QML le charge au premier cadre, DA2) ; Android, Qt WebView.
if(ANDROID)
    find_package(Qt6 6.12 QUIET COMPONENTS WebView)
    set(_actcraft_web_found ${Qt6WebView_FOUND})
else()
    find_package(Qt6 6.12 QUIET COMPONENTS WebEngineQuick)
    set(_actcraft_web_found ${Qt6WebEngineQuick_FOUND})
endif()
if(_actcraft_web_found)
    set(_actcraft_webframe_default ON)
else()
    set(_actcraft_webframe_default OFF)
endif()
option(ACTCRAFT_WEBFRAME "Cadre web : un moteur est construit (Qt WebEngine sur PC et Mac, Qt WebView sous Android)" ${_actcraft_webframe_default})
```
Dans `ui/CMakeLists.txt`, avant `qt_add_qml_module` :

```cmake
# Un seul composant de vue du cadre web par plateforme (DA1). Le composant du moteur est chargé depuis sa source :
# QtWebEngine n'est pas lié, aucun cache compilé n'est possible.
set(_actcraft_webframe_qml "")
if(ACTCRAFT_WEBFRAME)
    if(ANDROID)
        set(_actcraft_webframe_qml qml/WebFrameSystemView.qml)
    else()
        set(_actcraft_webframe_qml qml/WebFrameEngineView.qml)
        set_source_files_properties(qml/WebFrameEngineView.qml PROPERTIES QT_QML_SKIP_CACHEGEN TRUE)
    endif()
endif()
```
Ajouter `${_actcraft_webframe_qml}` à la liste QML juste après `qml/WebFrameDialog.qml`. Ajouter `include/actcraft/ui/webframeengine.h` et `src/webframeengine.cpp` à la liste où `include/actcraft/ui/webframeitem.h` et `src/webframeitem.cpp` figurent. Remplacer le bloc des lignes 427-429 :

```cmake
if(ACTCRAFT_WEBFRAME)
    target_compile_definitions(actcraft_ui PUBLIC ACTCRAFT_WEBFRAME=1)
    if(ANDROID)
        target_link_libraries(actcraft_ui PUBLIC Qt6::WebView)
    endif()
endif()
```

- [ ] **Étape 6 : l'initialisation.** `ui/include/actcraft/ui/webframeengine.h` :

```cpp
#pragma once

namespace actcraft::ui {

// À appeler avant QGuiApplication, dans chaque main : PC et Mac, partage des contextes et limite des processus de
// rendu (seule une importation QML différée charge le moteur, DA2) ; Android, QtWebView::initialize().
void prepareWebFrameEngine();

} // namespace actcraft::ui
```
`ui/src/webframeengine.cpp` :

```cpp
#include <actcraft/ui/webframeengine.h>
#include <actcraft/ui/webframeitem.h> // ACTCRAFT_WEBFRAME

#include <QCoreApplication>
#include <QtGlobal>

#if ACTCRAFT_WEBFRAME && defined(Q_OS_ANDROID)
#include <QtWebView/QtWebView>
#endif

namespace actcraft::ui {

void prepareWebFrameEngine()
{
#if ACTCRAFT_WEBFRAME
#ifdef Q_OS_ANDROID
    QtWebView::initialize();
#else
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    if (qEnvironmentVariableIsEmpty("QTWEBENGINE_CHROMIUM_FLAGS"))
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--renderer-process-limit=2");
#endif
#endif
}

} // namespace actcraft::ui
```
Dans `app/main.cpp`, retirer les lignes 14-16 et `#include <actcraft/ui/webframeitem.h> // ACTCRAFT_WEBFRAME`, ajouter `#include <actcraft/ui/webframeengine.h>`, remplacer les lignes 38-40 par `actcraft::ui::prepareWebFrameEngine(); // avant l'application (cadre web, DA2)`. Même remplacement dans `bench/app/main.cpp` (lignes 21-23 et 64-66) et `bench/captures/main.cpp` (lignes 29-31 et 110-112, sous `AA_DontUseNativeDialogs`). Dans `tests/tst_ui_webframe.cpp`, lignes 44-46 : `#include <actcraft/ui/webframeengine.h>`, lignes 1222-1224 : `actcraft::ui::prepareWebFrameEngine();`.

- [ ] **Étape 7 : la macro et `engineKind`.** Dans `webframeitem.h`, remplacer les lignes 19-28 :

```cpp
// Moteur web construit : posé par ui/CMakeLists.txt (public) quand ACTCRAFT_WEBFRAME est ON.
#ifndef ACTCRAFT_WEBFRAME
#define ACTCRAFT_WEBFRAME 0
#endif
```
Après la propriété `engineBuilt` : `Q_PROPERTY(QString engineKind READ engineKind CONSTANT)`. Après `engineBuilt()` : `static QString engineKind();`. Dans `webframeitem.cpp`, ajouter `#include <QGuiApplication>` et :

```cpp
QString WebFrameItems::engineKind()
{
#if !ACTCRAFT_WEBFRAME
    return QStringLiteral("none");
#elif defined(Q_OS_ANDROID)
    return QStringLiteral("system");
#else
#ifdef Q_OS_WIN
    // Reste PC 4 : le moteur n'est pas prouvé hors écran sous Windows (cadrage n° 30).
    if (QGuiApplication::platformName() == QLatin1String("offscreen"))
        return QStringLiteral("none");
#endif
    return QStringLiteral("engine");
#endif
}
```

- [ ] **Étape 8 : la vue WebEngine.** `ui/qml/WebFrameEngineView.qml` :

```qml
import QtQuick
import QtWebEngine

// Vue du cadre web sur PC et Mac (cadrage DA1, n° 11, 12, 33). Même contrat que WebFrameSystemView.qml, comparé par
// tst_ui_webframelayer. Racine Item : les noms du contrat (goBack, reload…) sont déjà ceux de WebEngineView. L'import est
// écrit en clair pour qmlimportscanner ; le module ne se charge qu'au premier cadre (Loader de WebFrameLayer.qml).
Item {
    id: root
    objectName: "webFrameView"
    property string html: ""
    property url baseUrl: ""
    property url directUrl: ""
    property url contentFile: ""
    property int serial: 0
    // Fixé à la création (initial de Loader.setSource) : profil et JavaScript ne se changent pas à chaud.
    property bool guardsEnabled: false
    readonly property bool overlaysScene: false
    readonly property bool viewFocused: web.activeFocus

    signal viewReported(url pageUrl, bool canBack, bool canForward, string pageTitle, bool pageLoading)
    signal navigationTaken(url pageUrl)
    signal pageLoaded()

    function load() {
        if (directUrl.toString() !== "")
            web.url = directUrl
        else if (contentFile.toString() !== "")
            web.url = contentFile
        else
            web.loadHtml(html, guardsEnabled ? "" : baseUrl)
    }
    function stop() { web.stop() }
    function goBack() { web.goBack() }
    function goForward() { web.goForward() }
    function reload() { web.reload() }
    function capture(callback) {
        if (!web.grabToImage(function (result) { callback(result.image) }))
            callback(null)
    }
    function copySelection() { web.triggerWebAction(WebEngineView.Copy) }
    function _report() {
        root.viewReported(web.url, web.canGoBack, web.canGoForward, web.title, web.loading)
    }

    Component {
        id: guardedProfile
        WebEngineProfile {}
    }

    WebEngineView {
        id: web
        objectName: "webFrameEngine"
        anchors.fill: parent
        // Gardes allumées : profil neuf et éphémère par vue (H8-13) ; sinon le profil par défaut, éphémère lui aussi.
        profile: root.guardsEnabled ? guardedProfile.createObject(root) : WebEngine.defaultProfile
        settings.javascriptEnabled: !root.guardsEnabled
        settings.localStorageEnabled: !root.guardsEnabled
        settings.localContentCanAccessFileUrls: !root.guardsEnabled
        settings.localContentCanAccessRemoteUrls: !root.guardsEnabled
        onUrlChanged: {
            if (root.guardsEnabled)
                root.navigationTaken(url)
            root._report()
        }
        onTitleChanged: root._report()
        onCanGoBackChanged: root._report()
        onCanGoForwardChanged: root._report()
        onLoadingChanged: function (info) {
            if (root.guardsEnabled && info.status === WebEngineView.LoadStartedStatus)
                root.navigationTaken(info.url)
            if (info.status === WebEngineView.LoadSucceededStatus)
                root.pageLoaded()
            root._report()
        }
    }
}
```

- [ ] **Étape 9 : la vue Android.** `ui/qml/WebFrameSystemView.qml` :

```qml
import QtQuick
import QtWebView

// Vue du cadre web sous Android (cadrage n° 2, 13, 33, 38) : la vue native passe au-dessus de tout le QML. Déclaré
// sous Android seulement : l'analyse des imports de l'APK voit ainsi QtWebView.
Item {
    id: root
    objectName: "webFrameView"
    property string html: ""
    property url baseUrl: ""
    property url directUrl: ""
    property url contentFile: ""
    property int serial: 0
    property bool guardsEnabled: false
    readonly property bool overlaysScene: true
    readonly property bool viewFocused: false

    signal viewReported(url pageUrl, bool canBack, bool canForward, string pageTitle, bool pageLoading)
    signal navigationTaken(url pageUrl)
    signal pageLoaded()

    function load() {
        if (directUrl.toString() !== "")
            web.url = directUrl
        else
            web.loadHtml(html, guardsEnabled ? "" : baseUrl)
    }
    function stop() { web.stop() }
    function goBack() { web.goBack() }
    function goForward() { web.goForward() }
    function reload() { web.reload() }
    function capture(callback) { callback(null) } // Android : capture par la fenêtre, n° 18
    function copySelection() {}
    function _report() {
        root.viewReported(web.url, web.canGoBack, web.canGoForward, web.title, web.loading)
    }

    // Magasin de cookies commun et durable : vidé à la création et à la destruction (H8-13).
    Component.onCompleted: web.deleteAllCookies()
    Component.onDestruction: web.deleteAllCookies()

    WebView {
        id: web
        anchors.fill: parent
        settings.javaScriptEnabled: !root.guardsEnabled
        settings.allowFileAccess: !root.guardsEnabled
        settings.localContentCanAccessFileUrls: !root.guardsEnabled
        settings.localStorageEnabled: !root.guardsEnabled
        onUrlChanged: {
            if (root.guardsEnabled)
                root.navigationTaken(url)
            root._report()
        }
        onTitleChanged: root._report()
        onLoadingChanged: function (request) {
            if (root.guardsEnabled && request.status === WebView.LoadStartedStatus)
                root.navigationTaken(request.url)
            if (request.status === WebView.LoadSucceededStatus)
                root.pageLoaded()
            root._report()
        }
    }
}
```

- [ ] **Étape 10 : le Loader dans la couche.** Dans `WebFrameLayer.qml`, ajouter après `property var dialog: null` :

```qml
    // Composant de la vue : celui du moteur construit ; un test en pose un autre avant le premier cadre.
    property url viewSource: appWebFrames.engineKind === "engine" ? Qt.resolvedUrl("WebFrameEngineView.qml")
                           : appWebFrames.engineKind === "system" ? Qt.resolvedUrl("WebFrameSystemView.qml") : ""
    property int loadedViews: 0   // vues dont une page a fini de charger (lu par les tests et le banc)
    property int snapshotCount: 0 // images fixes terminées
```
Remplacer le bloc `Item { id: viewHost … }` (lignes 150-199) par :

```qml
            Item {
                id: viewHost
                objectName: "webFrameViewHost"
                anchors.fill: parent
                clip: true
                visible: frame.showsView && !(frameLayer.covered && loader.item !== null && loader.item.overlaysScene)
                property bool reported: false

                Loader {
                    id: loader
                    objectName: "webFrameLoader"
                    anchors.fill: parent
                    asynchronous: true // import QtWebEngine lu à la première vue (DA2)
                    onStatusChanged: {
                        if (status === Loader.Ready)
                            viewHost.deliver()
                        else if (status === Loader.Error)
                            console.warn("Cadre web : vue indisponible (" + source + ")")
                    }
                }
                Connections {
                    target: loader.item
                    ignoreUnknownSignals: true
                    function onNavigationTaken(pageUrl) {
                        // Toute adresse prise par la vue repasse par le modèle ; s'il la prend en charge, la vue s'arrête.
                        if (appWebFrames.followLink(frame.index, pageUrl))
                            loader.item.stop()
                    }
                    function onPageLoaded() {
                        if (viewHost.reported)
                            return
                        viewHost.reported = true
                        frameLayer.loadedViews += 1
                        console.info("ACTCRAFT CADRE WEB CHARGE")
                    }
                }
                function deliver() {
                    if (loader.status !== Loader.Ready || !frame.showsView)
                        return
                    const view = loader.item
                    view.html = frame.content
                    view.baseUrl = frame.baseUrl
                    view.serial = frame.serial
                    view.load()
                }
                function show() {
                    if (!frame.showsView || frameLayer.viewSource.toString() === "")
                        return
                    if (loader.status === Loader.Null) {
                        viewHost.reported = false
                        loader.setSource(frameLayer.viewSource, { guardsEnabled: true }) // T5 : réglage réel
                        return
                    }
                    deliver()
                }
            }
```
Dans le gestionnaire `webframe.snapshot` (ligne 113), ajouter `frameLayer.snapshotCount += 1` après `console.info`. Dans `tests/tst_ui_webframe.cpp:845-850`, remplacer le composant par :

```cpp
    component.setData("import QtQuick\nimport QtWebEngine\nWebEngineView { width: 400; height: 300; "
                      "settings.javascriptEnabled: false; settings.localContentCanAccessFileUrls: false; "
                      "settings.localContentCanAccessRemoteUrls: false }",
                      QUrl());
```
Lignes 866-867 : sur Mac le moteur est présent, plus de saut silencieux :

```cpp
#ifdef Q_OS_MACOS
    QVERIFY2(loaded, "le moteur n'a demandé aucune sous-ressource");
#else
    if (!loaded)
        QSKIP("moteur web indisponible hors écran");
#endif
```

- [ ] **Étape 11 : vérifier le vert.** Commande bornée ci-dessus. Attendu : `PASS` des deux tests nommés. Si `import QtWebEngine` échoue hors écran en rendu logiciel, ne pas contourner : remonter à l'orchestrateur avec le message exact (hypothèse du cadrage démentie).

- [ ] **Étape 12 : format, suite complète, banc.**

```bash
~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror ui/include/actcraft/ui/webframeengine.h ui/src/webframeengine.cpp ui/include/actcraft/ui/webframeitem.h ui/src/webframeitem.cpp app/main.cpp bench/app/main.cpp bench/captures/main.cpp tests/support/testserver.h tests/support/webframehost.h tests/tst_ui_webengine.cpp tests/tst_ui_webframelayer.cpp tests/tst_ui_webframe.cpp
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T1 --attente-secondes 3000 --commande "QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1"
tail -3 .tmp/test.txt
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T1 --attente-secondes 3000 --duree-max-minutes 180 --commande "ACTCRAFT_QT_DIR=$(brew --prefix qt) CMAKE_BUILD_PARALLEL_LEVEL=2 scripts/bench.sh --rendu logiciel --hors-ecran --scenario court --taille 1280x800 > .tmp/bench.txt 2>&1"
tail -1 .tmp/bench.txt
```
Attendu : `100% tests passed` ou seul `tst_ui_linklauncher` en échec. Noter le chemin du JSON (`bench/results/`) et le verdict dans le rapport de tâche.

- [ ] **Étape 13 : commit.**

```bash
git add CMakeLists.txt ui/CMakeLists.txt ui/qml/WebFrameLayer.qml ui/qml/WebFrameEngineView.qml ui/qml/WebFrameSystemView.qml ui/include/actcraft/ui/webframeengine.h ui/src/webframeengine.cpp ui/include/actcraft/ui/webframeitem.h ui/src/webframeitem.cpp app/main.cpp bench/app/main.cpp bench/captures/main.cpp tests/support/testserver.h tests/support/webframehost.h tests/qml/WebFrameHost.qml tests/qml/FakeWebFrameView.qml tests/qml/AbsentModuleView.qml tests/tst_ui_webframe.cpp tests/tst_ui_webengine.cpp tests/tst_ui_webframelayer.cpp tests/ui.cmake
git commit -m "feat(ui): contrat des vues du cadre web et Loader de Qt WebEngine

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T2 — Interrupteur des gardes et réglage de profil
<a id="tache-t2"></a>

**Objectif.** Un seul interrupteur, `WebFrameItems.guardsEnabled`, faux par défaut, lu dans la clé `web-frame-guards` du profil, au départ et à chaque `profileChanged` (cadrage n° 9, 10, E12, E13).

**Fichiers.** Modifier : `ui/include/actcraft/ui/webframeitem.h`, `ui/src/webframeitem.cpp`, `ui/src/settingsschema.cpp:193-200`, `ui/qml/WebFrameLayer.qml`, `ui/qml/PageArea.qml:13-14,240-243`, `ui/qml/Main.qml:389,1011`, `ui/qml/DesktopAnnotation.qml:17,105`, `ui/qml/Presentation.qml:73`, `ui/i18n/actcraft_fr.ts`, `tests/tst_ui_webframe.cpp:152-161`, `tests/tst_ui_desktop.cpp`.

**Interfaces.**
- Consomme : `ProfileController::setting`, `setSetting`, signal `profileChanged` (`profilecontroller.h:84-85,139`) ; T1 : couche et `Loader`.
- Produit :
  - `Q_PROPERTY(bool guardsEnabled READ guardsEnabled WRITE setGuardsEnabled NOTIFY guardsEnabledChanged)` ;
  - `Q_PROPERTY(actcraft::ui::ProfileController* profile READ profile WRITE setProfile NOTIFY profileChanged)` ;
  - `property var profile` sur `PageArea`, `Presentation`, `DesktopAnnotation`, `WebFrameLayer` ;
  - clé de réglage : section `"tools"`, clé `"web-frame-guards"`, booléen, défaut faux, magasin profil.

**Test et commande bornée.**
```bash
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T2 --attente-secondes 3000 --commande "cmake --build build-mac -j2 && QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_ui_(webframe|desktop|settingsschema|webpolicy)$' --output-on-failure --timeout 300 -V > .tmp/t2.txt 2>&1"
grep -E 'PASS   : .*(guardsFollowTheProfileSetting|unrelatedProfileChangeReloadsNothing|desktopLayerFollowsTheSetting)|tests passed|tests failed' .tmp/t2.txt
```
Motif : trois lignes `PASS` ; `tst_ui_settingsschema` et `tst_ui_webpolicy`, à garder verts, passent.

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, avec `build-mac` : `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1` (suite complète, lue par `tail -3 .tmp/test.txt`), sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T2 --attente-secondes 3000 --commande "<commande>"` ; une seule tâche qui construit à la fois (8 Go).
- Format : `~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror` sur les fichiers touchés.
- `tst_ui_linklauncher` (`htaRefused`) : rouge préexistant instable, pas une régression.
- Tâche d'interface : test de chargement ; pas de banc ici, le rendu ne change pas (le banc tourne sur T1, T5, T7, T12, T18).
- Aucun nom repris d'un produit existant ; aucune ressource de Promethean.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** Ne pas toucher `WebContentPolicy` ni ses tests (DA5). Pas de nouveau signal de `ProfileController`. `prepareRow` garde son chemin actuel : la branche gardes coupées est T3.

**Critère « terminé ».** Tests nommés verts, suite complète sans autre rouge que `tst_ui_linklauncher`, commit fait.

**Étapes.**

- [ ] **Étape 1 : tests rouges.** Dans `tests/tst_ui_webframe.cpp`, changer la `Fixture` :

```cpp
struct Fixture
{
    ui::DocumentController document;
    ui::WebFrameItems frames;
    // Tout ce fichier éprouve le chemin de la politique : gardes allumées, sauf demande (cadrage n° 9, DA5).
    explicit Fixture(bool guards = true)
    {
        frames.setGuardsEnabled(guards);
        document.newDocument(1024, 768);
        frames.setDocument(&document);
    }
};
```
Déclarer dans `private slots:` `void guardsFollowTheProfileSetting(); void unrelatedProfileChangeReloadsNothing();`, ajouter `#include <actcraft/actions/profilestore.h>`, `<actcraft/ui/commands.h>`, `<actcraft/ui/preferences.h>`, `<actcraft/ui/profilecontroller.h>`, `<QSettings>` et :

```cpp
namespace {

// Profil d'essai, construit comme tst_ui_classroomlock : aucun réglage du poste touché.
struct ProfileFixture
{
    QTemporaryDir dir;
    ui::Commands commands;
    std::unique_ptr<ui::Preferences> preferences;
    std::unique_ptr<ui::ProfileController> profile;
    ProfileFixture()
    {
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.path());
        preferences = std::make_unique<ui::Preferences>();
        actions::ProfileStore store(dir.filePath(QStringLiteral("profiles")),
                                    QStringLiteral(":/actcraft/actions/profiles"),
                                    ui::ProfileController::builtinOrder());
        profile = std::make_unique<ui::ProfileController>(std::move(store), preferences.get(), &commands);
        profile->start();
    }
};

} // namespace

// Effet immédiat, sans relancer (S3, E12) : la ligne est relue et son `serial` avance.
void TstUiWebFrame::guardsFollowTheProfileSetting()
{
    ProfileFixture p;
    Fixture f(false);
    QVERIFY(f.frames.insertFrame(QStringLiteral("<p>a</p>"), {}));
    QTRY_VERIFY(content(f.frames, 0).contains(QStringLiteral("a")));
    f.frames.setProfile(p.profile.get());
    QVERIFY(!f.frames.guardsEnabled());
    QSignalSpy changed(&f.frames, &ui::WebFrameItems::guardsEnabledChanged);
    const int before = role(f.frames, 0, ui::WebFrameItems::SerialRole).toInt();
    p.profile->setSetting(QStringLiteral("tools"), QStringLiteral("web-frame-guards"), true);
    QVERIFY(f.frames.guardsEnabled());
    QCOMPARE(changed.count(), 1);
    QTRY_VERIFY(role(f.frames, 0, ui::WebFrameItems::SerialRole).toInt() > before);
}

// `profileChanged` part aussi à chaque couleur ou outil récent (cadrage n° 10) : valeur inchangée, rien.
void TstUiWebFrame::unrelatedProfileChangeReloadsNothing()
{
    ProfileFixture p;
    Fixture f(false);
    QVERIFY(f.frames.insertFrame(QStringLiteral("<p>a</p>"), {}));
    QTRY_VERIFY(content(f.frames, 0).contains(QStringLiteral("a")));
    f.frames.setProfile(p.profile.get());
    QTest::qWait(100);
    const int before = role(f.frames, 0, ui::WebFrameItems::SerialRole).toInt();
    p.profile->pushRecentTool(QStringLiteral("pen"));
    QTest::qWait(200);
    QCOMPARE(role(f.frames, 0, ui::WebFrameItems::SerialRole).toInt(), before);
}
```
Dans `tests/tst_ui_desktop.cpp`, ajouter (`#include <actcraft/ui/profilecontroller.h>`) :

```cpp
    // E13 : la couche de l'annotation du bureau suit le même réglage.
    void desktopLayerFollowsTheSetting()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        auto* profile =
            main.window()->findChild<actcraft::ui::ProfileController*>(QStringLiteral("appProfile"));
        QVERIFY(profile);
        QQuickItem* layer =
            presentationharness::findItem(desktop->window()->contentItem(), QStringLiteral("webFrameLayer"));
        QVERIFY(layer);
        QObject* frames = layer->findChild<QObject*>(QStringLiteral("appWebFrames"));
        QVERIFY(frames);
        QCOMPARE(frames->property("guardsEnabled").toBool(), false);
        profile->setSetting(QStringLiteral("tools"), QStringLiteral("web-frame-guards"), true);
        QTRY_COMPARE(frames->property("guardsEnabled").toBool(), true);
        profile->setSetting(QStringLiteral("tools"), QStringLiteral("web-frame-guards"), false);
    }
```

- [ ] **Étape 2 : rouge.** Construction : échec attendu `no member named 'setGuardsEnabled'`.

- [ ] **Étape 3 : le modèle.** Dans `webframeitem.h` : `#include "actcraft/ui/profilecontroller.h"` (moc exige le type complet) ; après `engineKind` :

```cpp
    Q_PROPERTY(bool guardsEnabled READ guardsEnabled WRITE setGuardsEnabled NOTIFY guardsEnabledChanged)
    Q_PROPERTY(actcraft::ui::ProfileController* profile READ profile WRITE setProfile NOTIFY profileChanged)
```
Publique : `bool guardsEnabled() const noexcept { return m_guardsEnabled; }`, `void setGuardsEnabled(bool enabled);`, `ProfileController* profile() const noexcept;`, `void setProfile(ProfileController* profile);`. Signaux : `void guardsEnabledChanged(); void profileChanged();`. Privés : `void readGuards(); bool m_guardsEnabled = false; QPointer<ProfileController> m_profile; QMetaObject::Connection m_profileConnection;`. Dans `webframeitem.cpp` :

```cpp
ProfileController* WebFrameItems::profile() const noexcept
{
    return m_profile;
}

void WebFrameItems::setProfile(ProfileController* profile)
{
    if (m_profile == profile)
        return;
    disconnect(m_profileConnection);
    m_profile = profile;
    if (profile)
        m_profileConnection =
            connect(profile, &ProfileController::profileChanged, this, &WebFrameItems::readGuards);
    readGuards();
    emit profileChanged();
}

// Le seul signal disponible, `profileChanged`, part aussi pour une couleur : setGuardsEnabled ne fait rien si la
// valeur ne change pas (cadrage n° 10).
void WebFrameItems::readGuards()
{
    if (m_profile)
        setGuardsEnabled(
            m_profile->setting(QStringLiteral("tools"), QStringLiteral("web-frame-guards"), false).toBool());
}

void WebFrameItems::setGuardsEnabled(bool enabled)
{
    if (m_guardsEnabled == enabled)
        return;
    m_guardsEnabled = enabled;
    for (int i = 0; i < m_rows.size(); ++i) {
        Row& row = m_rows[i];
        row.history.clear(); // E12 : la session du cadre est oubliée
        row.historyIndex = -1;
        prepareRow(row);
        rowChanged(i);
    }
    emit guardsEnabledChanged();
    emit countChanged(); // blockedCount
}
```

- [ ] **Étape 4 : le réglage.** Dans `ui/src/settingsschema.cpp`, avant le commentaire « Orthographe » :

```cpp
        boolean("tools", "web-frame-guards",
                QT_TRANSLATE_NOOP("SettingsSchema",
                                  "Cadres web : appliquer la politique de contenu (JavaScript coupé, adresses filtrées)"),
                false),
```
Dans `ui/i18n/actcraft_fr.ts`, contexte `SettingsSchema`, ajouter :

```xml
    <message>
        <source>Cadres web : appliquer la politique de contenu (JavaScript coupé, adresses filtrées)</source>
        <translation>Cadres web : appliquer la politique de contenu (JavaScript coupé, adresses filtrées)</translation>
    </message>
```

- [ ] **Étape 5 : le relais QML.** `WebFrameLayer.qml` : `property var profile: null` après `commands`, et dans `WebFrameItems` : `profile: frameLayer.profile`. `PageArea.qml` : après `property Commands commands: null`, `property var profile: null` ; dans le `WebFrameLayer` : `profile: area.profile`. `Main.qml` : sur `PageArea { id: pageArea`, ligne 389 : `profile: appProfile` ; sur `Presentation { id: presentationRoot`, ligne 1011 : `profile: appProfile`. `Presentation.qml` : après `property Tools tools: null`, `property var profile: null` ; sur `DesktopAnnotation { id: desktopRoot` (ligne 73) : `profile: presentation.profile`. `DesktopAnnotation.qml` : après `property Tools tools: null`, `property var profile: null` ; dans `PageArea { id: desktopArea` : `profile: root.profile`.

- [ ] **Étape 6 : vérifier, format, suite complète.** Commande bornée, puis `clang-format --dry-run --Werror ui/include/actcraft/ui/webframeitem.h ui/src/webframeitem.cpp ui/src/settingsschema.cpp tests/tst_ui_webframe.cpp tests/tst_ui_desktop.cpp` et la suite complète (`ctest … > .tmp/test.txt`, `tail -3 .tmp/test.txt`, sous le jeton).

- [ ] **Étape 7 : commit.**

```bash
git add ui/include/actcraft/ui/webframeitem.h ui/src/webframeitem.cpp ui/src/settingsschema.cpp ui/qml/WebFrameLayer.qml ui/qml/PageArea.qml ui/qml/Main.qml ui/qml/DesktopAnnotation.qml ui/qml/Presentation.qml ui/i18n/actcraft_fr.ts tests/tst_ui_webframe.cpp tests/tst_ui_desktop.cpp
git commit -m "feat(ui): interrupteur des gardes du cadre web, réglé par le profil

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T14 — Étape `setting` du joueur et scénario cadre-web
<a id="tache-t14"></a>

**Objectif.** Le joueur pose un réglage de profil ; `cadre-web.json` allume les gardes en tête et mesure le trou de boucle autour du premier cadre (cadrage n° 36, S3).

**Fichiers.** Modifier : `bench/include/actcraft/bench/uiscenario.h:80,251`, `bench/src/uiscenario.cpp` (`parseStep`, `runStep`), `bench/scenarios/cadre-web.json:5-22`, `tests/tst_bench_uiscenario.cpp`.

**Interfaces.**
- Consomme : T1 : `webFrameLayer.loadedViews`, `webFrameLayer.snapshotCount` ; `appProfile.setSetting` (`profilecontroller.h:85`).
- Produit : étape `{"setting": "<section>.<clé>", "value": …}`, `UiStepKind::Setting`, `bool UiScenarioPlayer::setting(const UiStep&, QStringList&)`, ligne de rapport `REGLAGE <section>.<clé> OK`.

**Test et commande bornée.**
```bash
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T14 --attente-secondes 3000 --commande "cmake --build build-mac -j2 && QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_(bench_uiscenario|captures_cadre_web)$' --output-on-failure --timeout 300 -V > .tmp/t14.txt 2>&1"
grep -E 'PASS   : .*settingStepIsParsed|REGLAGE tools.web-frame-guards OK|BOUCLE max=|SCENARIO OK|tests passed|tests failed' .tmp/t14.txt
```
Motif : `PASS` nommé, `REGLAGE … OK`, `BOUCLE max=`, `SCENARIO OK`, aucun `tests failed`. `tst_captures_cadre_web` est le test à garder vert.

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, avec `build-mac` : `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1` (suite complète, lue par `tail -3 .tmp/test.txt`), sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T14 --attente-secondes 3000 --commande "<commande>"` ; une seule tâche qui construit à la fois (8 Go).
- Format : `~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror` sur les fichiers touchés.
- `tst_ui_linklauncher` (`htaRefused`) : rouge préexistant instable, pas une régression.
- Aucun nom repris d'un produit existant ; aucune ressource de Promethean.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** Ne pas toucher `tests/bench.cmake` (cadrage : non touché), ni `bench/src/fixtures.cpp`.

**Critère « terminé ».** Motifs ci-dessus, suite complète sans autre rouge que `tst_ui_linklauncher`, commit fait.

**Étapes.**

- [ ] **Étape 1 : test rouge.** Dans `tests/tst_bench_uiscenario.cpp`, à côté de `newStepsAreParsed` :

```cpp
    // Réglage de profil posé par le scénario (chantier cadre-web-webengine, cadrage n° 36).
    void settingStepIsParsed()
    {
        UiScenario s;
        QString error;
        QVERIFY2(parseUiScenario("{\"format\":\"actcraft-scenario/1\",\"name\":\"x\",\"steps\":["
                                 "{\"setting\":\"tools.web-frame-guards\",\"value\":true}]}",
                                 s, error),
                 qPrintable(error));
        QCOMPARE(s.steps.size(), std::size_t{1});
        QVERIFY(s.steps[0].kind == UiStepKind::Setting);
        QCOMPARE(s.steps[0].target, QStringLiteral("tools"));
        QCOMPARE(s.steps[0].property, QStringLiteral("web-frame-guards"));
        QCOMPARE(s.steps[0].argument, QVariant(true));
    }
```
Et dans le `_data` des étapes invalides, après `"remember nom invalide"` :

```cpp
        QTest::newRow("setting sans valeur")
            << QByteArray("{\"setting\":\"a.b\"}") << QStringLiteral("réglage invalide");
        QTest::newRow("setting sans section")
            << QByteArray("{\"setting\":\"ab\",\"value\":1}") << QStringLiteral("réglage invalide");
```
Rouge attendu : `no member named 'Setting' in 'UiStepKind'`.

- [ ] **Étape 2 : le joueur.** Dans `uiscenario.h`, à la fin de `UiStepKind` : `Setting, // {"setting": "tools.web-frame-guards", "value": true} — réglage du profil, appProfile.setSetting`. Près de `restart` : `bool setting(const UiStep& step, QStringList& report);`. Dans `parseStep`, avant `if (o.contains(QStringLiteral("watchEventLoop")))` :

```cpp
    if (o.contains(QStringLiteral("setting"))) {
        step.kind = UiStepKind::Setting;
        if (!splitTarget(o.value(QStringLiteral("setting")).toString(), step.target, step.property) ||
            !o.contains(QStringLiteral("value"))) {
            why = QStringLiteral("réglage invalide (setting « section.clé », value)");
            return false;
        }
        step.argument = o.value(QStringLiteral("value")).toVariant();
        return true;
    }
```
Dans `runStep`, avant `case UiStepKind::WatchEventLoop:` : `case UiStepKind::Setting: return setting(step, report);`. Près de `restart` :

```cpp
// Réglage de profil, comme l'onglet paramètres (SettingsSchema, magasin « profil »).
bool UiScenarioPlayer::setting(const UiStep& step, QStringList& report)
{
    QObject* profile = find(QStringLiteral("appProfile"));
    if (!profile ||
        !QMetaObject::invokeMethod(profile, "setSetting", Q_ARG(QString, step.target),
                                   Q_ARG(QString, step.property), Q_ARG(QVariant, step.argument))) {
        report << QStringLiteral("REGLAGE %1.%2 ECHEC (appProfile introuvable)").arg(step.target, step.property);
        return false;
    }
    report << QStringLiteral("REGLAGE %1.%2 OK").arg(step.target, step.property);
    return true;
}
```

- [ ] **Étape 3 : le scénario.** Dans `bench/scenarios/cadre-web.json`, remplacer, de la première étape `{"waitReady": true}` à celle qui suit `webframe.snapshot` incluse, par, en gardant le texte HTML actuel de `insert.web-frame` :

```json
        {"waitReady": true},
        {"setting": "tools.web-frame-guards", "value": true},
        {"watchEventLoop": true},
        {"command": "insert.web-frame", "argument": {"html": "<!doctype html><html><head><title>Cadre d'essai</title></head><body style=\"margin:0;background:#1E88E5;color:#FFFFFF;font:32px sans-serif\"><p>Cadre d'essai ActCraft</p></body></html>"}},
        {"waitFor": "webFrameLayer.loadedViews", "value": 1, "timeoutMs": 60000, "skipOn": ["windows"], "skipReason": "moteur non lancé hors écran sous Windows (Reste PC 4)"},
        {"maxEventLoopGapMs": 120, "informative": true},
        {"waitReady": true},
        {"expect": "webFrameLayer.count", "value": 1},
        {"expect": "webFrameLayer.blockedCount", "value": 0},
        {"capture": "cadre"},
        {"command": "webframe.snapshot"},
        {"waitFor": "webFrameLayer.snapshotCount", "value": 1, "timeoutMs": 30000},
        {"waitReady": true},
```
La suite (`capture image-fixe`, fixture, `file.open`, …) reste inchangée.

- [ ] **Étape 4 : vérifier.** Commande bornée. Puis format de `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`, `tests/tst_bench_uiscenario.cpp` ; suite complète sous le jeton.

- [ ] **Étape 5 : commit.**

```bash
git add bench/include/actcraft/bench/uiscenario.h bench/src/uiscenario.cpp bench/scenarios/cadre-web.json tests/tst_bench_uiscenario.cpp
git commit -m "feat(bench): étape setting du joueur, gardes allumées dans cadre-web

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T15 — Licences et notices de Qt WebEngine
<a id="tache-t15"></a>

**Objectif.** Autoriser les six modules, citer Chromium, retirer le refus de WebEngine (cadrage n° 35, carte « Paquets et inventaires »).

**Fichiers.** Modifier : `packaging/qt-autorises.txt:50,70` et fin, `THIRD_PARTY_NOTICES.md:84-85,196,237-238,311`, `scripts/verifier-licences-mac.sh:4-5,50-52,74`, `CLAUDE.md:32,35`.

**Interfaces.** Consomme : T1 (rien d'exécuté). Produit : noms `Qt6WebEngineCore`, `Qt6WebEngineQuick`, `Qt6WebEngineQuickDelegatesQml`, `Qt6WebChannel`, `Qt6WebChannelQuick`, `Qt6Positioning` autorisés. `LICENSES/BSD-3-Clause.txt` existe déjà : aucun fichier de `LICENSES/` n'est ajouté (contrôle de la carte d'impact).

**Test et commande bornée.**
```bash
mkdir -p .tmp/faux.app/Contents/Frameworks/QtCore.framework .tmp/faux.app/Contents/Frameworks/QtWebEngineCore.framework
bash scripts/verifier-licences-mac.sh .tmp/faux.app | tail -1   # attendu : LICENCES OK
mkdir -p .tmp/faux.app/Contents/Frameworks/QtInconnu.framework
bash scripts/verifier-licences-mac.sh .tmp/faux.app | tail -1   # attendu : LICENCES EN ECHEC : Qt6Inconnu
rm -rf .tmp/faux.app
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T15 --attente-secondes 3000 --commande "cmake --build build-mac -j2 && QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_app_notices$' --output-on-failure --timeout 300 -V > .tmp/t15.txt 2>&1"
grep -E 'PASS   : .*(everyAllowedQtModuleHasANotice|everyCitedLicenseHasItsText)|tests passed|tests failed' .tmp/t15.txt
```

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, avec `build-mac` : `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1` (suite complète, lue par `tail -3 .tmp/test.txt`), sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T15 --attente-secondes 3000 --commande "<commande>"` ; une seule tâche qui construit à la fois (8 Go).
- `tst_ui_linklauncher` (`htaRefused`) : rouge préexistant instable, pas une régression.
- Le code ne contient aucun nom repris d'un produit existant ; aucune ressource de Promethean. Textes de notice écrits en nos mots.
- `.github/workflows/release.yml` n'est jamais modifié (O98).
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** Ne pas modifier `tests/tst_app_notices.cpp` (garde). Pas de nouvelle bibliothèque non Qt.

**Critère « terminé ».** `LICENCES OK` sur le bundle factice, refus du nom inconnu, `tst_app_notices` vert, commit fait.

**Étapes.**

- [ ] **Étape 1 : rouge.** Lancer la première commande de `verifier-licences-mac.sh` : attendu `LICENCES EN ECHEC : QtWebEngineCore.framework(WebEngine)`.

- [ ] **Étape 2 : `verifier-licences-mac.sh`.** Lignes 4-5 du commentaire : retirer « , aucun Qt WebEngine » (écrire « aucune sous GPL seule. Les greffons »). Supprimer les lignes 50-53 (`QtWebEngine*) … ;;`). Supprimer la ligne 74 (`case "$nom" in *WebEngine*) … esac`).

- [ ] **Étape 3 : `qt-autorises.txt`.** Ligne 50 du commentaire : « module Qt6Pdf seul ; lié dynamiquement » (retirer « , jamais Qt WebEngine »). Ligne 70 : remplacer « Jamais Qt WebEngine, ni Qt6WebEngine*, ni Qt6Pdf avec lui. » par « Qt WebEngine : voir la fin du fichier. ». À la fin :

```text
# Qt WebEngine (cadre web, PC et Mac, décision 0217 du coffre) : chargé au premier cadre, sans lien à l'application.
# Licence vérifiée dans l'en-tête qquickwebengineprofile.h (ligne SPDX) : « LicenseRef-Qt-Commercial OR LGPL-3.0-only OR
# GPL-2.0-only OR GPL-3.0-only », la même que QuickControls2Basic. Dépendances relevées par otool -L de QtWebEngineCore :
# QtCore, QtGui, QtOpenGL, QtQml, QtQuick, QtNetwork, QtWebChannel, QtPositioning et le système. Chromium : voir
# THIRD_PARTY_NOTICES.md.
Qt6WebEngineCore
Qt6WebEngineQuick
Qt6WebEngineQuickDelegatesQml
Qt6WebChannel
Qt6WebChannelQuick
Qt6Positioning
```

- [ ] **Étape 4 : notices.** Dans `THIRD_PARTY_NOTICES.md` : lignes 84-85, compléter : « - `Qt6WebView` : cadre web intégré sous Android, par le moteur web du système. » et « - `Qt6WebViewQuick` : cadre web de l'interface sous Android. ». Après `Qt6WebViewQuick`, ajouter :

```markdown
- `Qt6WebEngineCore` : moteur Chromium du cadre web (PC et Mac), chargé au premier cadre.
- `Qt6WebEngineQuick` : vue web de l'interface (PC et Mac).
- `Qt6WebEngineQuickDelegatesQml` : éléments d'interface du moteur web.
- `Qt6WebChannel` : canal entre le moteur web et l'application, tiré par Qt WebEngine.
- `Qt6WebChannelQuick` : même canal en QML, tiré par Qt WebEngine.
- `Qt6Positioning` : géolocalisation, tirée par Qt WebEngine (les demandes des pages sont refusées).
```
Ligne 196 : retirer « ; jamais Qt WebEngine ». Lignes 237-238 : remplacer par « - **Moteur du cadre web** : PC et Mac, Qt WebEngine (Chromium), livré avec ActCraft (voir « Qt WebEngine et Chromium ») ; Android, navigateur du système. ». Ligne 311 : retirer « ou Qt WebEngine » (phrase : « une bibliothèque sous GPL seule. Quand »). Après la section « Qt PDF et PDFium », ajouter :

```markdown
## Qt WebEngine et Chromium

Le cadre web sur PC et Mac passe par le module Qt WebEngine (`LGPL-3.0-only`, lié dynamiquement au moment du premier cadre), qui embarque **Chromium** (https://www.chromium.org/) et ses bibliothèques. Liste exacte : fichier `sbom.spdx.json` de la version de Qt WebEngine utilisée. Le paquet macOS livre le texte de la licence de Chromium dans `Contents/Resources/LICENSE.Chromium`.

- Chromium : Licence SPDX : `BSD-3-Clause`. Copyright 2015 The Chromium Authors.

Certains codecs vidéo (H.264, AAC) dépendent de la provenance de Qt WebEngine ; leur redistribution est une décision prise avant toute publication. Les brevets sur ces codecs relèvent des utilisateurs et des distributeurs.
```

- [ ] **Étape 5 : `CLAUDE.md`.** Ligne 32 : `-m qtshadertools qtimageformats qtpdf qtmultimedia qtwebview qtwebengine qtwebchannel qtpositioning`. Ligne 35 : « `qtpdf` rend `ACTCRAFT_IMPORT_PDF` automatique (`ON` si le module est trouvé) ; `qtwebengine` rend `ACTCRAFT_WEBFRAME` automatique sur PC et Mac (Android : `qtwebview`) ; sur Mac, `brew install qt` les apporte. »

- [ ] **Étape 6 : vérifier, commit.** Commandes bornées, puis :

```bash
git add packaging/qt-autorises.txt THIRD_PARTY_NOTICES.md scripts/verifier-licences-mac.sh CLAUDE.md
git commit -m "chore(licences): Qt WebEngine et Chromium autorisés et cités

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T3 — Modèle, gardes coupées : adresse directe, HTML lourd
<a id="tache-t3"></a>

**Objectif.** Gardes coupées, le modèle ne lit plus la page : l'adresse part telle quelle à la vue, un HTML saisi aussi, avec la base des ressources du document ; au-delà de la limite de `loadHtml`, un fichier temporaire, gardes coupées ou allumées (cadrage n° 11, 38, E1, E11).

**Fichiers.** Modifier : `ui/include/actcraft/ui/webframeitem.h:88-94,170-190`, `ui/src/webframeitem.cpp:30-45,1057-1069,1222-1288,1464-1505,1585-1612`, `tests/tst_ui_webframe.cpp`.

**Interfaces.**
- Consomme : T2 : `guardsEnabled()`, `Fixture(bool guards = true)`.
- Produit : rôles `DirectUrlRole` (`"directUrl"`, `QUrl`), `ContentFileRole` (`"contentFile"`, `QUrl`), ajoutés après `SerialRole`. `followLink` rend faux pour `about:blank`, un `data:`, le fichier de `contentFile`.

**Test et commande bornée.**
```bash
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T3 --attente-secondes 3000 --commande "cmake --build build-mac -j2 && QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_(ui_webframe|ui_webpolicy|captures_cadre_web)$' --output-on-failure --timeout 300 -V > .tmp/t3.txt 2>&1"
grep -E 'PASS   : .*(guardsOffSendsNoRequest|heavyHtmlGoesToAFile|toggleDuringHeavyWriteDropsTheFile|switchingGuardsOnDropsDirectContent)|SCENARIO OK|tests passed|tests failed' .tmp/t3.txt
```
`tst_ui_webpolicy`, à garder vert, passe ; `javascriptOffByDefault` reste vrai pour la politique.

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, avec `build-mac` : `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1` (suite complète, lue par `tail -3 .tmp/test.txt`), sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T3 --attente-secondes 3000 --commande "<commande>"` ; une seule tâche qui construit à la fois (8 Go).
- Format : `~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror` sur les fichiers touchés.
- `tst_ui_linklauncher` (`htaRefused`) : rouge préexistant instable, pas une régression.
- Aucun nom repris d'un produit existant ; aucune ressource de Promethean.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** Ne pas changer `prepareHtml`, `navigate`, `fetch`, `fetched` ni `kMaxBody`. Aucune écriture de fichier ni calcul de taille sur le fil de l'interface pour un HTML long (BRIEF § 4.1).

**Patron à ne pas reprendre.** Calculer `toPercentEncoding` puis écrire le fichier dans `prepareRow`, sur le fil de l'interface : 3 Mo y bloqueraient la boucle. Et traiter seulement le chemin gardes coupées : le chemin gardes allumées (`ui/src/webframeitem.cpp:1245-1288`) a la même limite, avec son jeton `preparing` qu'il faut respecter dans les deux cas.

**Critère « terminé ».** Tests nommés verts (dont le test Review Focus 2), `tst_captures_cadre_web` vert, suite complète, commit fait.

**Étapes.**

- [ ] **Étape 1 : tests rouges.** Dans `tests/tst_ui_webframe.cpp`, déclarer et écrire :

```cpp
// E1 : gardes coupées, le modèle ne lit rien et ne bloque rien, même pour un document reçu.
void TstUiWebFrame::guardsOffSendsNoRequest()
{
    TestServer server;
    QVERIFY(server.listening());
    server.page(QStringLiteral("/page"), "<p>distant</p>");
    Fixture f(false);
    const QUrl address = server.url(QStringLiteral("/page"));
    QVERIFY(f.frames.insertFrame({}, address.toString()));
    QTRY_COMPARE(role(f.frames, 0, ui::WebFrameItems::DirectUrlRole).toUrl(), address);
    QVERIFY(content(f.frames, 0).isEmpty());
    QTest::qWait(200);
    QCOMPARE(server.requests(), 0);
    QCOMPARE(f.frames.blockedCount(), 0);
}

// n° 11 : un HTML saisi n'est pas réécrit ; base au dossier du document.
void TstUiWebFrame::guardsOffKeepsTypedHtmlWithTheResourceBase()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString html = QStringLiteral("<script>var a=1</script><a href=\"https://exemple.invalide/\">x</a>");
    Fixture writer(false);
    QVERIFY(writer.frames.insertFrame(html, {}));
    QTRY_COMPARE(content(writer.frames, 0), html);
    QCOMPARE(role(writer.frames, 0, ui::WebFrameItems::BaseUrlRole).toUrl(), QUrl());
    Fixture reader(false);
    QVERIFY(saveAndReopen(writer, reader, dir.filePath(QStringLiteral("a.actcraft"))));
    QTRY_COMPARE(content(reader.frames, 0), html);
    QCOMPARE(role(reader.frames, 0, ui::WebFrameItems::BaseUrlRole).toUrl(),
             QUrl::fromLocalFile(QFileInfo(dir.filePath(QStringLiteral("a.actcraft"))).absolutePath() + u'/'));
    QCOMPARE(reader.frames.blockedCount(), 0);
}

// n° 38 : au-delà de la limite de loadHtml, un fichier temporaire ; en dessous, aucun.
void TstUiWebFrame::heavyHtmlGoesToAFile_data()
{
    QTest::addColumn<bool>("guards");
    QTest::newRow("gardes coupees") << false;
    QTest::newRow("gardes allumees") << true;
}

void TstUiWebFrame::heavyHtmlGoesToAFile()
{
    QFETCH(bool, guards);
    Fixture f(guards);
    const QString heavy = QStringLiteral("<title>GROS</title><body>") + QString(3 * 1024 * 1024, u'a');
    QVERIFY(f.frames.insertFrame(heavy, {}));
    QTRY_VERIFY_WITH_TIMEOUT(!role(f.frames, 0, ui::WebFrameItems::LoadingRole).toBool() &&
                                 !content(f.frames, 0).isEmpty(),
                             60000);
    const QUrl file = role(f.frames, 0, ui::WebFrameItems::ContentFileRole).toUrl();
    QVERIFY2(file.isLocalFile(), qPrintable(file.toString()));
    QVERIFY(QFileInfo::exists(file.toLocalFile()));
    QVERIFY(!f.frames.followLink(0, file)); // le fichier est à la vue, pas une navigation
    QVERIFY(f.frames.insertFrame(QStringLiteral("<p>court</p>"), {}));
    QTRY_VERIFY(content(f.frames, 1).contains(QStringLiteral("court")));
    QVERIFY(role(f.frames, 1, ui::WebFrameItems::ContentFileRole).toUrl().isEmpty());
}

// Un `<` encodé tient trois octets : 1 Mo de `<` dépasse la limite, 1 Mo de lettres non (n° 38).
void TstUiWebFrame::escapedHtmlCountsAfterEncoding()
{
    Fixture f(false);
    QVERIFY(f.frames.insertFrame(QString(1024 * 1024, u'<'), {}));
    QTRY_VERIFY_WITH_TIMEOUT(!role(f.frames, 0, ui::WebFrameItems::ContentFileRole).toUrl().isEmpty(), 30000);
    QVERIFY(f.frames.insertFrame(QString(1024 * 1024, u'a'), {}));
    QTRY_VERIFY_WITH_TIMEOUT(!content(f.frames, 1).isEmpty(), 30000);
    QVERIFY(role(f.frames, 1, ui::WebFrameItems::ContentFileRole).toUrl().isEmpty());
}

// Review Focus 2 (n° 10, n° 38) : réglage basculé pendant l'écriture : résultat périmé abandonné, fichier retiré.
void TstUiWebFrame::toggleDuringHeavyWriteDropsTheFile()
{
    Fixture f(false);
    QVERIFY(f.frames.insertFrame(QString(3 * 1024 * 1024, u'a'), {}));
    f.frames.setGuardsEnabled(true); // avant la fin de l'écriture : la boucle d'événements n'a pas tourné
    QVERIFY(QThreadPool::globalInstance()->waitForDone(60000));
    QTRY_VERIFY_WITH_TIMEOUT(!role(f.frames, 0, ui::WebFrameItems::LoadingRole).toBool(), 60000);
    QTest::qWait(100);
    const QDir folder(QFileInfo(role(f.frames, 0, ui::WebFrameItems::ContentFileRole).toUrl().toLocalFile()).absolutePath());
    QCOMPARE(folder.entryList(QDir::Files).size(), qsizetype(1)); // le seul fichier est celui du contenu courant
}

void TstUiWebFrame::viewOwnUrlsAreNotNavigations()
{
    Fixture f;
    QVERIFY(f.frames.insertFrame(QStringLiteral("<p>a</p>"), {}));
    QTRY_VERIFY(content(f.frames, 0).contains(QStringLiteral("a")));
    QVERIFY(!f.frames.followLink(0, QUrl(QStringLiteral("data:text/html,x#haut"))));
    QVERIFY(!f.frames.followLink(0, QUrl(QStringLiteral("about:blank"))));
}

// Gardes rallumées : ni l'adresse ni le HTML brut ne restent, le temps de la lecture par le modèle (n° 12).
void TstUiWebFrame::switchingGuardsOnDropsDirectContent()
{
    Fixture f(false);
    QVERIFY(f.frames.insertFrame({}, QStringLiteral("https://exemple.invalide/")));
    QVERIFY(!role(f.frames, 0, ui::WebFrameItems::DirectUrlRole).toUrl().isEmpty());
    f.frames.setGuardsEnabled(true);
    QVERIFY(role(f.frames, 0, ui::WebFrameItems::DirectUrlRole).toUrl().isEmpty());
    QVERIFY(content(f.frames, 0).isEmpty());
}
```
Rouge attendu : `no member named 'DirectUrlRole'`.

- [ ] **Étape 2 : l'en-tête.** Rôles après `SerialRole` : `DirectUrlRole, // adresse donnée telle quelle à la vue (gardes coupées)` et `ContentFileRole // HTML au-delà de la limite de loadHtml : fichier du modèle`. Dans `Row` : `QUrl directUrl; QUrl contentFile;`. Privés :

```cpp
    void showDirect(Row& row);
    void finishDirect(std::uint64_t id, std::uint64_t ticket, const QUrl& file);
    void releaseContentFile(Row& row);
    QString heavyDir();
```
et `QUrl m_resourceBase; std::unique_ptr<QTemporaryDir> m_heavy;` ; déclaration avancée `class QTemporaryDir;` ; `#include <memory>`. Changer `finishContent` : `void finishContent(std::uint64_t id, std::uint64_t ticket, const QUrl& base, const Prepared& prepared, const QUrl& heavy);`.

- [ ] **Étape 3 : les aides.** Dans l'espace anonyme du début de `webframeitem.cpp` (`#include <QDir>`, `<QTemporaryDir>`) :

```cpp
// Limite de loadHtml (url::kMaxURLChars = 2 097 152, sur l'adresse data: encodée) ; seuil sous la limite, pour le préfixe.
constexpr qsizetype kLoadHtmlLimit = 2000000;
// Un QChar UTF-16 donne au plus 3 octets UTF-8, chacun au plus 3 caractères encodés : en dessous, inutile de compter.
constexpr qsizetype kSurelyShortHtml = kLoadHtmlLimit / 9;

// Fil secondaire seulement. Fichier écrit avec sa marque d'ordre : sans déclaration de charset, UTF-8 reste lu comme tel.
QUrl writeHeavyHtml(const QString& html, const QString& dir, std::uint64_t ticket)
{
    if (dir.isEmpty() || html.size() <= kSurelyShortHtml)
        return {};
    const QByteArray utf8 = html.toUtf8();
    if (utf8.toPercentEncoding().size() <= kLoadHtmlLimit)
        return {};
    const QString path = QDir(dir).filePath(QStringLiteral("cadre-%1.html").arg(ticket));
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return {};
    file.write("\xEF\xBB\xBF");
    file.write(utf8);
    return QUrl::fromLocalFile(path);
}
```

- [ ] **Étape 4 : la base du document.** Dans `documentReplaced()`, après `m_policy.setResourceFolder(...)` :

```cpp
    m_resourceBase = file.isLocalFile() ? QUrl::fromLocalFile(QFileInfo(file.toLocalFile()).absolutePath() + u'/') : QUrl();
```

- [ ] **Étape 5 : `prepareRow` et le chemin direct.** Dans `prepareRow`, après `row.pendingUrl = QUrl();` :

```cpp
    if (!m_guardsEnabled) {
        showDirect(row);
        return;
    }
```
Dans `setGuardsEnabled`, juste avant `prepareRow(row);` :

```cpp
        row.content.clear(); // une vue neuve ne reçoit jamais le contenu de l'autre régime
        row.directUrl = QUrl();
        releaseContentFile(row);
```

Après `prepareRow` :

```cpp
QString WebFrameItems::heavyDir()
{
    if (!m_heavy)
        m_heavy = std::make_unique<QTemporaryDir>();
    return m_heavy->isValid() ? m_heavy->path() : QString();
}

void WebFrameItems::releaseContentFile(Row& row)
{
    if (!row.contentFile.isEmpty())
        QFile::remove(row.contentFile.toLocalFile());
    row.contentFile = QUrl();
}

// Gardes coupées (n° 11) : rien n'est lu ni réécrit. Un HTML court part tout de suite ; un long passe d'abord par un
// fil secondaire, qui compte et écrit le fichier si la limite de loadHtml est dépassée.
void WebFrameItems::showDirect(Row& row)
{
    row.links.clear();
    row.directUrl = QUrl();
    const QString address = row.address.trimmed();
    if (!address.isEmpty()) {
        row.preparing = 0;
        row.content.clear();
        releaseContentFile(row);
        row.directUrl = QUrl::fromUserInput(address);
        row.baseUrl = row.directUrl;
        row.loading = false;
        ++row.serial;
        return;
    }
    if (row.html.size() <= kSurelyShortHtml) {
        row.preparing = 0;
        releaseContentFile(row);
        row.content = row.html;
        row.baseUrl = m_resourceBase;
        row.loading = false;
        ++row.serial;
        return;
    }
    const std::uint64_t ticket = ++m_tickets;
    row.preparing = ticket;
    row.loading = true;
    const QPointer<WebFrameItems> self(this);
    QThreadPool::globalInstance()->start([self, id = row.id, ticket, html = row.html, dir = heavyDir()] {
        const QUrl file = writeHeavyHtml(html, dir, ticket);
        if (QCoreApplication* home = QCoreApplication::instance()) {
            QMetaObject::invokeMethod(
                home,
                [self, id, ticket, file] {
                    if (self)
                        self->finishDirect(id, ticket, file);
                    else if (!file.isEmpty())
                        QFile::remove(file.toLocalFile());
                },
                Qt::QueuedConnection);
        }
    });
}

void WebFrameItems::finishDirect(std::uint64_t id, std::uint64_t ticket, const QUrl& file)
{
    const int index = rowOf(id);
    if (index < 0 || m_rows[index].preparing != ticket) {
        if (!file.isEmpty())
            QFile::remove(file.toLocalFile()); // Review Focus 2 : résultat périmé
        return;
    }
    Row& row = m_rows[index];
    row.preparing = 0;
    releaseContentFile(row);
    row.contentFile = file;
    row.content = row.html;
    row.baseUrl = m_resourceBase;
    row.loading = false;
    ++row.serial;
    rowChanged(index);
    emit countChanged();
}
```

- [ ] **Étape 6 : le chemin gardes allumées.** Dans `prepareContent`, après `row.loading = true;` : `const QString dir = heavyDir();`. Dans la capture du travail : `dir`. Corps :

```cpp
        const Prepared prepared =
            prepareHtml(source(), base, [&](const QUrl& url) { return policy.decide(url, key, page); });
        const QUrl heavy = writeHeavyHtml(prepared.html, dir, ticket); // même limite (n° 38)
        if (QCoreApplication* home = QCoreApplication::instance()) {
            QMetaObject::invokeMethod(
                home,
                [self, id, ticket, base, prepared, heavy] {
                    if (self)
                        self->finishContent(id, ticket, base, prepared, heavy);
                },
                Qt::QueuedConnection);
        }
```
Dans `finishContent`, nouvelle signature, et au début :

```cpp
    if (index < 0 || m_rows[index].preparing != ticket) {
        if (!heavy.isEmpty())
            QFile::remove(heavy.toLocalFile());
        return; // cadre parti, ou passé à un autre contenu
    }
    Row& row = m_rows[index];
    row.preparing = 0;
    releaseContentFile(row);
    row.contentFile = heavy;
    row.directUrl = QUrl();
```
(le reste de la fonction ne change pas). Dans `navigate`, branches `Blocked` et `Refuse`, après `row.content.clear();` : `releaseContentFile(row); row.directUrl = QUrl();`.

- [ ] **Étape 7 : `data()` et `roleNames()`.** Dans `data()` : `case DirectUrlRole: return row.directUrl;` et `case ContentFileRole: return row.contentFile;`. Dans `roleNames()` : ajouter `{DirectUrlRole, "directUrl"}, {ContentFileRole, "contentFile"}`.

- [ ] **Étape 8 : `followLink`.** Remplacer la fin de `followLink` (lignes 1601-1607) :

```cpp
    // Rendus à la vue : about:blank et data: (contenu donné par loadHtml, avec ses renvois de fragment) ; le fichier du
    // HTML lourd. Toute autre adresse, même celle de la page affichée, serait chargée sans la règle de sécurité du
    // contenu : rendue au modèle, la vue s'arrête.
    const QUrl page = viewUrl.adjusted(QUrl::RemoveFragment);
    if (page.isEmpty() || page == QUrl(QStringLiteral("about:blank")) || page.scheme() == QLatin1String("data"))
        return false;
    if (!r.contentFile.isEmpty() && page.isLocalFile() &&
        QFileInfo(page.toLocalFile()).canonicalFilePath() == QFileInfo(r.contentFile.toLocalFile()).canonicalFilePath())
        return false;
```
Ajouter en tête de fonction, après le contrôle de `row` : `if (!m_guardsEnabled) return false; // gardes coupées : la vue navigue seule`.

- [ ] **Étape 9 : vérifier, format, suite complète.** Commande bornée, `clang-format --dry-run --Werror ui/include/actcraft/ui/webframeitem.h ui/src/webframeitem.cpp tests/tst_ui_webframe.cpp`, suite complète sous le jeton.

- [ ] **Étape 10 : commit.**

```bash
git add ui/include/actcraft/ui/webframeitem.h ui/src/webframeitem.cpp tests/tst_ui_webframe.cpp
git commit -m "feat(ui): gardes coupées, adresse et HTML donnés tels quels à la vue

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T4 — Historique du moteur rapporté au modèle
<a id="tache-t4"></a>

**Objectif.** Gardes coupées, précédent/suivant/recharger passent à la vue par `viewCommand`, l'état de la vue revient par `reportView` (cadrage n° 15).

**Fichiers.** Modifier : `ui/include/actcraft/ui/webframeitem.h`, `ui/src/webframeitem.cpp:1483-1490,1614-1645`, `tests/tst_ui_webframe.cpp`.

**Interfaces.**
- Consomme : T3 : `Row`, chemin direct.
- Produit : `Q_INVOKABLE void reportView(int row, const QUrl& url, bool canBack, bool canForward, const QString& title, bool loading);` ; `signal void viewCommand(int row, const QString& action);` valeurs d'`action` : `"back"`, `"forward"`, `"reload"`.

**Test et commande bornée.**
```bash
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T4 --attente-secondes 3000 --commande "cmake --build build-mac -j2 && QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_ui_webframe$' --output-on-failure --timeout 300 -V > .tmp/t4.txt 2>&1"
grep -E 'PASS   : .*(reportFillsTitleAndHistory|guardsOffCommandsGoToTheView)|^Totals:|tests passed|tests failed' .tmp/t4.txt
```

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, avec `build-mac` : `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1` (suite complète, lue par `tail -3 .tmp/test.txt`), sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T4 --attente-secondes 3000 --commande "<commande>"` ; une seule tâche qui construit à la fois (8 Go).
- Format : `~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror` sur les fichiers touchés.
- `tst_ui_linklauncher` (`htaRefused`) : rouge préexistant instable, pas une régression.
- Aucun nom repris d'un produit existant ; aucune ressource de Promethean.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** Gardes allumées : historique du modèle inchangé. `reportView` n'avance pas `serial` (un rapport ne recharge pas la vue).

**Critère « terminé ».** Tests nommés verts, suite complète sans autre rouge que `tst_ui_linklauncher`, commit fait.

**Étapes.**

- [ ] **Étape 1 : tests rouges.**

```cpp
void TstUiWebFrame::guardsOffCommandsGoToTheView()
{
    Fixture f(false);
    QVERIFY(f.frames.insertFrame({}, QStringLiteral("https://exemple.invalide/")));
    QSignalSpy commands(&f.frames, &ui::WebFrameItems::viewCommand);
    f.frames.goBack(0);
    f.frames.goForward(0);
    f.frames.reload(0);
    QCOMPARE(commands.count(), 3);
    QCOMPARE(commands.at(0).at(1).toString(), QStringLiteral("back"));
    QCOMPARE(commands.at(1).at(1).toString(), QStringLiteral("forward"));
    QCOMPARE(commands.at(2).at(1).toString(), QStringLiteral("reload"));
}

void TstUiWebFrame::guardsOnCommandsStayInTheModel()
{
    Fixture f;
    QVERIFY(f.frames.insertFrame(QStringLiteral("<p>a</p>"), {}));
    QSignalSpy commands(&f.frames, &ui::WebFrameItems::viewCommand);
    f.frames.reload(0);
    QCOMPARE(commands.count(), 0);
}

void TstUiWebFrame::reportFillsTitleAndHistory()
{
    Fixture f(false);
    QVERIFY(f.frames.insertFrame({}, QStringLiteral("https://exemple.invalide/")));
    const int serial = role(f.frames, 0, ui::WebFrameItems::SerialRole).toInt();
    f.frames.reportView(0, QUrl(QStringLiteral("https://exemple.invalide/b")), true, false, QStringLiteral("Page B"),
                        false);
    QCOMPARE(role(f.frames, 0, ui::WebFrameItems::TitleRole).toString(), QStringLiteral("Page B"));
    QVERIFY(role(f.frames, 0, ui::WebFrameItems::CanGoBackRole).toBool());
    QVERIFY(!role(f.frames, 0, ui::WebFrameItems::CanGoForwardRole).toBool());
    QCOMPARE(role(f.frames, 0, ui::WebFrameItems::SerialRole).toInt(), serial);
}

void TstUiWebFrame::reportIsIgnoredGuardsOn()
{
    Fixture f;
    QVERIFY(f.frames.insertFrame(QStringLiteral("<p>a</p>"), {}));
    f.frames.reportView(0, QUrl(QStringLiteral("https://exemple.invalide/")), true, true, QStringLiteral("X"), false);
    QVERIFY(!role(f.frames, 0, ui::WebFrameItems::CanGoBackRole).toBool());
    QVERIFY(role(f.frames, 0, ui::WebFrameItems::TitleRole).toString() != QStringLiteral("X"));
}
```
Rouge attendu : `no member named 'viewCommand'`.

- [ ] **Étape 2 : l'en-tête.** Dans `Row` : `QUrl viewUrl; QString viewTitle; bool viewCanBack = false; bool viewCanForward = false;`. Public : la méthode `reportView`. Signal : `void viewCommand(int row, const QString& action);`.

- [ ] **Étape 3 : le code.** Dans `webframeitem.cpp` :

```cpp
void WebFrameItems::reportView(int row, const QUrl& url, bool canBack, bool canForward, const QString& title,
                               bool loading)
{
    if (m_guardsEnabled || row < 0 || row >= m_rows.size())
        return; // gardes allumées : l'historique et le titre sont ceux du modèle
    Row& r = m_rows[row];
    if (r.viewUrl == url && r.viewCanBack == canBack && r.viewCanForward == canForward && r.viewTitle == title &&
        r.loading == loading)
        return;
    r.viewUrl = url;
    r.viewCanBack = canBack;
    r.viewCanForward = canForward;
    r.viewTitle = title;
    r.loading = loading;
    rowChanged(row);
}
```
En tête de `goBack`, `goForward` et `reload`, juste après leur contrôle de `row` (pour `goBack`, séparer le contrôle de `historyIndex <= 0` : borne de `row` d'abord) :

```cpp
    if (!m_guardsEnabled) {
        emit viewCommand(row, QStringLiteral("back")); // "forward", "reload" pour les deux autres
        return;
    }
```
Dans `data()` :

```cpp
    case TitleRole: {
        if (!m_guardsEnabled && !row.viewTitle.isEmpty())
            return row.viewTitle;
        const QUrl shown = row.blocked ? row.pendingUrl : (!m_guardsEnabled && row.viewUrl.isValid() ? row.viewUrl : row.baseUrl);
        return shown.host().isEmpty() ? tr("Cadre web") : shown.host();
    }
    case CanGoBackRole:
        return m_guardsEnabled ? row.historyIndex > 0 : row.viewCanBack;
    case CanGoForwardRole:
        return m_guardsEnabled ? row.historyIndex >= 0 && row.historyIndex + 1 < row.history.size() : row.viewCanForward;
```

- [ ] **Étape 4 : vérifier, format, suite complète, commit.**

```bash
git add ui/include/actcraft/ui/webframeitem.h ui/src/webframeitem.cpp tests/tst_ui_webframe.cpp
git commit -m "feat(ui): historique du moteur rapporté au modèle du cadre web

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T9 — Téléchargement borné `WebImageFetch`
<a id="tache-t9"></a>

**Objectif.** Télécharger une image : 25 Mo, 20 s sans transfert, redirections sûres, politique consultée si les gardes sont allumées (cadrage n° 23 à 26, H6).

**Fichiers.** Créer : `ui/include/actcraft/ui/webimagefetch.h`, `ui/src/webimagefetch.cpp`, `tests/tst_ui_webimagefetch.cpp`. Modifier : `ui/CMakeLists.txt` (liste des sources), `tests/ui.cmake`.

**Interfaces.**
- Consomme : `WebContentPolicy::Verdict` (`webpolicy.h`) ; T1 : `TestServer` (`rawResponse`, `redirect`, `page`).
- Produit : `class WebImageFetch final : public QObject` ; `using Decide = std::function<WebContentPolicy::Verdict(const QUrl&)>;` ; `static constexpr qint64 kMaxBytes = 25 * 1024 * 1024;` ; `void start(const QUrl& url, Decide decide = {});` ; `void cancel();` ; signal `void finished(const QByteArray& data, const QString& error);` (`error` vide : succès).

**Test et commande bornée.**
```bash
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T9 --attente-secondes 3000 --commande "cmake -S . -B build-mac && cmake --build build-mac -j2 && QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_ui_webimagefetch$' --output-on-failure --timeout 300 -V > .tmp/t9.txt 2>&1"
grep -E 'PASS   : .*(fetchesTheBytes|oversizeWithoutLengthIsRefused|policyRefusesARedirect)|^Totals:|tests passed|tests failed' .tmp/t9.txt
```
Motif : trois `PASS` nommés, `Totals:` ≥ 7 passés (`initTestCase` et `cleanupTestCase` compris).

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, avec `build-mac` : `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1` (suite complète, lue par `tail -3 .tmp/test.txt`), sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T9 --attente-secondes 3000 --commande "<commande>"` ; une seule tâche qui construit à la fois (8 Go).
- Format : `~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror` sur les fichiers touchés.
- `tst_ui_linklauncher` (`htaRefused`) : rouge préexistant instable, pas une régression.
- Tests sans réseau : serveur local 127.0.0.1 seulement (cadrage n° 29).
- Aucun nom repris d'un produit existant ; aucune ressource de Promethean.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** Ni `core/` ni `formats/` (`readUrl` reste sans réseau). Aucun décodage ici. Pas de cookies partagés avec le moteur (H6).

**Critère « terminé ».** Tests verts, suite complète sans autre rouge que `tst_ui_linklauncher`, commit fait.

**Étapes.**

- [ ] **Étape 1 : tests rouges.** `tests/tst_ui_webimagefetch.cpp` :

```cpp
// Téléchargement borné d'une image (cadrage n° 23 à 26), sur un serveur local seulement.
#include "support/testserver.h"

#include <actcraft/ui/webimagefetch.h>

#include <QSignalSpy>
#include <QtTest>

using actcraft::ui::WebContentPolicy;
using actcraft::ui::WebImageFetch;

namespace {

struct Run
{
    WebImageFetch fetch;
    QSignalSpy finished{&fetch, &WebImageFetch::finished};
    QByteArray data() const { return finished.at(0).at(0).toByteArray(); }
    QString error() const { return finished.at(0).at(1).toString(); }
};

} // namespace

class TstUiWebImageFetch : public QObject
{
    Q_OBJECT
private slots:
    void fetchesTheBytes()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/i.png"), "ABC");
        Run run;
        run.fetch.start(server.url(QStringLiteral("/i.png")));
        QTRY_COMPARE(run.finished.count(), 1);
        QCOMPARE(run.error(), QString());
        QCOMPARE(run.data(), QByteArray("ABC"));
    }

    void missingPageFails()
    {
        TestServer server;
        QVERIFY(server.listening());
        Run run;
        run.fetch.start(server.url(QStringLiteral("/absent.png")));
        QTRY_COMPARE(run.finished.count(), 1);
        QVERIFY(run.error().contains(QStringLiteral("404")));
        QVERIFY(run.data().isEmpty());
    }

    void oversizeIsRefused()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/gros"), QByteArray(int(WebImageFetch::kMaxBytes) + 1024, 'x'));
        Run run;
        run.fetch.start(server.url(QStringLiteral("/gros")));
        QTRY_COMPARE_WITH_TIMEOUT(run.finished.count(), 1, 30000);
        QVERIFY2(run.error().contains(QStringLiteral("25 Mo")), qPrintable(run.error()));
    }

    // Review Focus 3 (n° 25) : aucun Content-Length ; la borne tient sur ce qui arrive.
    void oversizeWithoutLengthIsRefused()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.rawResponse(QStringLiteral("/flux"),
                           "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nConnection: close\r\n\r\n" +
                               QByteArray(int(WebImageFetch::kMaxBytes) + 1024, 'x'));
        Run run;
        run.fetch.start(server.url(QStringLiteral("/flux")));
        QTRY_COMPARE_WITH_TIMEOUT(run.finished.count(), 1, 30000);
        QVERIFY2(run.error().contains(QStringLiteral("25 Mo")), qPrintable(run.error()));
    }

    void redirectIsFollowed()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/fin"), "OK");
        server.redirect(QStringLiteral("/depart"), server.url(QStringLiteral("/fin")));
        Run run;
        run.fetch.start(server.url(QStringLiteral("/depart")));
        QTRY_COMPARE(run.finished.count(), 1);
        QCOMPARE(run.data(), QByteArray("OK"));
    }

    void redirectLoopFails()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.redirect(QStringLiteral("/a"), server.url(QStringLiteral("/a")));
        Run run;
        run.fetch.start(server.url(QStringLiteral("/a")));
        QTRY_COMPARE_WITH_TIMEOUT(run.finished.count(), 1, 10000);
        QVERIFY(!run.error().isEmpty());
    }

    void policyRefusesTheAddress()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/i"), "x");
        Run run;
        run.fetch.start(server.url(QStringLiteral("/i")),
                        [](const QUrl&) { return WebContentPolicy::Verdict::Refuse; });
        QTRY_COMPARE(run.finished.count(), 1);
        QVERIFY(!run.error().isEmpty());
        QCOMPARE(server.requests(), 0);
    }

    void policyRefusesARedirect()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/autre"), "x");
        server.redirect(QStringLiteral("/depart"), server.url(QStringLiteral("/autre")));
        Run run;
        run.fetch.start(server.url(QStringLiteral("/depart")), [](const QUrl& url) {
            return url.path() == QLatin1String("/depart") ? WebContentPolicy::Verdict::Load
                                                          : WebContentPolicy::Verdict::Refuse;
        });
        QTRY_COMPARE(run.finished.count(), 1);
        QVERIFY(!run.error().isEmpty());
        QVERIFY(!server.paths().contains(QStringLiteral("/autre")));
    }

    void cancelStaysSilent()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.hold(QStringLiteral("/lent"));
        server.page(QStringLiteral("/lent"), "x");
        Run run;
        run.fetch.start(server.url(QStringLiteral("/lent")));
        QTRY_COMPARE(server.requests(), 1);
        run.fetch.cancel();
        QTest::qWait(200);
        QCOMPARE(run.finished.count(), 0);
        server.release();
    }
};

QTEST_MAIN(TstUiWebImageFetch)
#include "tst_ui_webimagefetch.moc"
```
Dans `tests/ui.cmake`, à la fin :

```cmake
actcraft_add_test(tst_ui_webimagefetch
    SOURCES tst_ui_webimagefetch.cpp
    LIBS ActCraft::ui)
```

- [ ] **Étape 2 : l'en-tête.** `ui/include/actcraft/ui/webimagefetch.h` :

```cpp
#pragma once

#include <actcraft/ui/webpolicy.h>

#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>

#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

namespace actcraft::ui {

// Téléchargement d'une image glissée ou collée (cadrage n° 23 à 26), sur le fil de l'interface et sans blocage : le
// décodage reste au fil de travail d'Insertion. Une seule requête par objet. `decide` vide : gardes coupées.
class WebImageFetch final : public QObject
{
    Q_OBJECT
public:
    using Decide = std::function<WebContentPolicy::Verdict(const QUrl&)>;
    static constexpr qint64 kMaxBytes = 25 * 1024 * 1024;
    static constexpr int kTimeoutMs = 20000; // sans transfert ; comme kTransferTimeoutMs du cadre
    static constexpr int kMaxRedirects = 10;

    explicit WebImageFetch(QObject* parent = nullptr);
    void start(const QUrl& url, Decide decide = {});
    void cancel();

signals:
    void finished(const QByteArray& data, const QString& error); // error vide : succès

private:
    void request(const QUrl& url);
    void answered(QNetworkReply* reply);
    void fail(const QString& error);

    QNetworkAccessManager* m_network = nullptr;
    QPointer<QNetworkReply> m_reply;
    Decide m_decide;
    int m_redirects = 0;
    bool m_tooLarge = false;
};

} // namespace actcraft::ui
```

- [ ] **Étape 3 : le code.** `ui/src/webimagefetch.cpp` :

```cpp
#include <actcraft/ui/webimagefetch.h>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace actcraft::ui {

namespace {
bool isWeb(const QUrl& url)
{
    return url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https");
}
} // namespace

WebImageFetch::WebImageFetch(QObject* parent) : QObject(parent) {}

void WebImageFetch::start(const QUrl& url, Decide decide)
{
    m_decide = std::move(decide);
    m_redirects = 0;
    m_tooLarge = false;
    if (m_decide && m_decide(url) != WebContentPolicy::Verdict::Load) {
        // Différé : l'appelant a le temps de se brancher sur `finished`.
        QTimer::singleShot(0, this, [this] { fail(tr("Adresse refusée par la politique du cadre web.")); });
        return;
    }
    request(url);
}

void WebImageFetch::cancel()
{
    if (!m_reply)
        return;
    m_reply->disconnect(this);
    m_reply->abort();
    m_reply->deleteLater();
    m_reply = nullptr;
}

void WebImageFetch::request(const QUrl& url)
{
    if (!m_network)
        m_network = new QNetworkAccessManager(this);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setAttribute(QNetworkRequest::CookieSaveControlAttribute, QNetworkRequest::Manual);
    request.setAttribute(QNetworkRequest::CookieLoadControlAttribute, QNetworkRequest::Manual);
    request.setTransferTimeout(kTimeoutMs);
    QNetworkReply* reply = m_network->get(request);
    m_reply = reply;
    connect(reply, &QNetworkReply::downloadProgress, this, [this, reply](qint64 received, qint64) {
        if (received > kMaxBytes) {
            m_tooLarge = true;
            reply->abort();
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply] { answered(reply); });
}

void WebImageFetch::answered(QNetworkReply* reply)
{
    reply->deleteLater();
    m_reply = nullptr;
    if (m_tooLarge) {
        fail(tr("Image trop lourde (plus de 25 Mo)."));
        return;
    }
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QUrl target = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (status >= 300 && status < 400 && target.isValid()) {
        const QUrl next = reply->url().resolved(target);
        const bool lessSafe = reply->url().scheme() == QLatin1String("https") && next.scheme() != QLatin1String("https");
        if (++m_redirects > kMaxRedirects || !isWeb(next) || lessSafe ||
            (m_decide && m_decide(next) != WebContentPolicy::Verdict::Load)) {
            fail(tr("Redirection refusée."));
            return;
        }
        request(next);
        return;
    }
    if (status >= 400) {
        fail(tr("Image indisponible (erreur %1).").arg(status));
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        fail(tr("Image indisponible : %1").arg(reply->errorString()));
        return;
    }
    emit finished(reply->readAll(), QString());
}

void WebImageFetch::fail(const QString& error)
{
    emit finished(QByteArray(), error);
}

} // namespace actcraft::ui
```
Ajouter `include/actcraft/ui/webimagefetch.h` et `src/webimagefetch.cpp` dans `ui/CMakeLists.txt`, à côté de `webframeengine`.

- [ ] **Étape 4 : vérifier, format, suite complète, commit.**

```bash
git add ui/include/actcraft/ui/webimagefetch.h ui/src/webimagefetch.cpp ui/CMakeLists.txt tests/tst_ui_webimagefetch.cpp tests/ui.cmake
git commit -m "feat(ui): téléchargement borné d'une image web

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T10 — Insertion : dépôt web, `Mode::Downloaded`
<a id="tache-t10"></a>

**Objectif.** `Insertion::acceptWebDrop` : URL `http(s)` téléchargée, `data:image` décodée, `file:` par le chemin existant, `blob:` en message ; repli sur le texte si ce n'est pas une image ; suivi de la page d'origine pendant le téléchargement, une seule étape (cadrage n° 21, 24, 26 à 28, E9, Review Focus 4).

**Fichiers.** Modifier : `ui/include/actcraft/ui/insertion.h`, `ui/src/insertion.cpp:28-33,78-128,247-305,381-424`, `ui/include/actcraft/ui/webframeitem.h`, `ui/src/webframeitem.cpp`, `tests/tst_ui_insertion.cpp`.

**Interfaces.**
- Consomme : T9 : `WebImageFetch` ; T1 : `TestServer` ; T2 : `WebFrameItems::guardsEnabled()`.
- Produit :
  - `Q_INVOKABLE bool Insertion::acceptWebDrop(const QVariantList& urls, const QString& text, const QString& html, QPointF pagePoint);` ;
  - `Q_PROPERTY(actcraft::ui::WebFrameItems* webFrames READ webFrames WRITE setWebFrames NOTIFY webFramesChanged)` ;
  - `WebContentPolicy::Verdict WebFrameItems::decideAddress(const QUrl& url) const;` (publique, même réponse que `decide`).

**Test et commande bornée.**
```bash
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T10 --attente-secondes 3000 --commande "cmake --build build-mac -j2 && QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_ui_(insertion|fileexchange)$' --output-on-failure --timeout 300 -V > .tmp/t10.txt 2>&1"
grep -E 'PASS   : .*(webDropDownloadsAnImage|downloadNeverLandsInAnotherTab|vanishedDropFileFails|droppedHtmlLoadsNothing)|tests passed|tests failed' .tmp/t10.txt
```
`droppedHtmlLoadsNothing` (M13) et `tst_ui_fileexchange`, à garder verts, passent.

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, avec `build-mac` : `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1` (suite complète, lue par `tail -3 .tmp/test.txt`), sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T10 --attente-secondes 3000 --commande "<commande>"` ; une seule tâche qui construit à la fois (8 Go).
- Format : `~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror` sur les fichiers touchés.
- `tst_ui_linklauncher` (`htaRefused`) : rouge préexistant instable, pas une régression.
- Tests sans réseau : serveur local 127.0.0.1 seulement (cadrage n° 29).
- Aucun nom repris d'un produit existant ; aucune ressource de Promethean.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** `acceptDrop` et `insertImage` gardent leur refus de `http(s)` (`insertion.cpp:99-101`). Le paramètre `html` n'est jamais interprété (M13). Pas de bouton d'annulation (H7).

**Patron à ne pas reprendre.** Poser l'observateur de page dans `start()` comme aujourd'hui (`ui/src/insertion.cpp:261-280`) : le téléchargement dure plusieurs secondes avant `start`, l'origine serait prise trop tard. Elle se prend à la demande, se transmet à `start`. Et lire `m_document->currentIndex()` à l'arrivée pour le texte de repli : un changement de page verserait le texte ailleurs (`acceptDrop` ligne 420).

**Critère « terminé ».** Tests nommés verts, suite complète sans autre rouge que `tst_ui_linklauncher`, commit fait.

**Étapes.**

- [ ] **Étape 1 : tests rouges.** Dans `tests/tst_ui_insertion.cpp` (`#include "support/testserver.h"`, `#include <actcraft/ui/webframeitem.h>`, `<QBuffer>`, `<QVariantList>`) :

```cpp
namespace {
QByteArray pngBytes(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::blue);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return bytes;
}
} // namespace
```
Dans la classe de tests :

```cpp
    // S1, S2 : image http(s) téléchargée, extension lue dans les octets (le type annoncé ment), une étape.
    void webDropDownloadsAnImage()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/image-sans-extension"), pngBytes(120, 60)); // annoncé text/html
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/image-sans-extension"))}, {}, {}, {300, 200}));
        QTRY_COMPARE_WITH_TIMEOUT(inserted.count(), 1, 10000);
        const auto* image = lastAs<core::ImageObject>(controller);
        QVERIFY(image != nullptr);
        QCOMPARE(image->bounds.center().x, 300.0);
        QCOMPARE(controller.document().resource(image->resource)->extension, std::string("png"));
        QVERIFY(insertion.lastDecodeOffMainThread());
        controller.undo();
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
        QVERIFY(!controller.canUndo());
    }

    void webDropOfLocalFileUsesTheFilePath()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({QUrl::fromLocalFile(writePng(dir, "t.png", 40, 40))}, {}, {}, {100, 100}));
        QTRY_COMPARE(inserted.count(), 1);
        QVERIFY(lastAs<core::ImageObject>(controller) != nullptr);
    }

    void webDropOfDataUrlDecodesInPlace()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        const QUrl url(QStringLiteral("data:image/png;base64,") + QString::fromLatin1(pngBytes(10, 10).toBase64()));
        QVERIFY(insertion.acceptWebDrop({url}, {}, {}, {50, 50}));
        QTRY_COMPARE(inserted.count(), 1);
        QVERIFY(lastAs<core::ImageObject>(controller) != nullptr);
    }

    void webDropOfBlobFails()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QVERIFY(insertion.acceptWebDrop({QUrl(QStringLiteral("blob:https://x/abc"))}, {}, {}, {1, 1}));
        QCOMPARE(failed.count(), 1);
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
    }

    // E9 : page qui n'est pas une image → objet texte, à la même place, après la réponse.
    void webDropOfNonImageFallsBackToText()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/page"), "<p>pas une image</p>");
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/page"))}, QStringLiteral("Titre du lien"), {},
                                        {200, 150}));
        QTRY_COMPARE_WITH_TIMEOUT(inserted.count(), 1, 10000);
        const auto* text = lastAs<core::Text>(controller);
        QVERIFY(text != nullptr);
        QVERIFY(text->html.find("Titre du lien") != std::string::npos);
        QCOMPARE(text->bounds.x, 200.0);
    }

    // S2, H14 : réseau en échec : bandeau, aucun objet.
    void webDropOnMissingPageFails()
    {
        TestServer server;
        QVERIFY(server.listening());
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/absent"))}, QStringLiteral("x"), {}, {1, 1}));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 10000);
        QVERIFY(!insertion.isBusy());
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
    }

    // S2, n° 27 : changer d'onglet pendant le téléchargement.
    void downloadNeverLandsInAnotherTab()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.hold(QStringLiteral("/lente"));
        server.page(QStringLiteral("/lente"), pngBytes(20, 20));
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::DocumentTabs tabs;
        tabs.setDocument(&controller);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/lente"))}, {}, {}, {10, 10}));
        QTRY_COMPARE(server.requests(), 1);
        tabs.newTab();
        server.release();
        QTRY_VERIFY_WITH_TIMEOUT(!insertion.isBusy(), 10000);
        QCOMPARE(inserted.count(), 0);
        QCOMPARE(failed.count(), 1);
        QVERIFY(firstPage(controller).objects.empty());
        tabs.activate(0);
        QVERIFY(firstPage(controller).objects.empty());
    }

    // n° 28 : Ctrl+Z pendant le téléchargement agit sur l'étape précédente ; l'image arrive quand même.
    void undoDuringDownloadKeepsTheDownload()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.hold(QStringLiteral("/lente"));
        server.page(QStringLiteral("/lente"), pngBytes(20, 20));
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QVERIFY(insertion.acceptDrop({}, QStringLiteral("avant"), {1, 1}));
        QSignalSpy inserted(&insertion, &ui::Insertion::inserted);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/lente"))}, {}, {}, {10, 10}));
        QTRY_COMPARE(server.requests(), 1);
        controller.undo();
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
        server.release();
        QTRY_COMPARE_WITH_TIMEOUT(inserted.count(), 1, 10000);
        QVERIFY(lastAs<core::ImageObject>(controller) != nullptr);
    }

    // n° 25 : gardes allumées, une adresse non saisie est refusée sans requête.
    void guardsRefuseAWebDrop()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/i"), pngBytes(10, 10));
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::WebFrameItems frames;
        frames.setGuardsEnabled(true);
        frames.setDocument(&controller);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        insertion.setWebFrames(&frames);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QVERIFY(insertion.acceptWebDrop({server.url(QStringLiteral("/i"))}, {}, {}, {1, 1}));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 5000);
        QCOMPARE(server.requests(), 0);
    }

    // Review Focus 4 (n° 20) : le fichier temporaire du moteur a disparu : message, document intact.
    void vanishedDropFileFails()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Insertion insertion;
        insertion.setDocument(&controller);
        QSignalSpy failed(&insertion, &ui::Insertion::failed);
        QVERIFY(insertion.acceptWebDrop({QUrl::fromLocalFile(QStringLiteral("/chemin/inexistant/glisse.png"))}, {}, {},
                                        {10, 10}));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 5000);
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
    }
```
Rouge attendu : `no member named 'acceptWebDrop'`.

- [ ] **Étape 2 : les en-têtes.** `insertion.h` : `#include <actcraft/ui/webframeitem.h>`, `#include <actcraft/ui/webimagefetch.h>`, `#include <memory>`. Avant la classe : `struct InsertionOrigin; // insertion.cpp`. Propriété :

```cpp
    Q_PROPERTY(actcraft::ui::WebFrameItems* webFrames READ webFrames WRITE setWebFrames NOTIFY webFramesChanged)
```
Publique, après `acceptDrop` :

```cpp
    // Dépôt ou collage venu d'une page web (cadrage n° 21) : `http(s)` téléchargé, `data:image` décodé, `file:` par
    // acceptDrop, `blob:` refusé. `html` n'est jamais interprété (M13) : il est reçu pour que l'appelant n'ait pas à
    // trier les formats. Vrai si quelque chose a été pris en charge.
    Q_INVOKABLE bool acceptWebDrop(const QVariantList& urls, const QString& text, const QString& html,
                                   QPointF pagePoint);
    WebFrameItems* webFrames() const { return m_webFrames; }
    void setWebFrames(WebFrameItems* frames);
```
Signal : `void webFramesChanged();`. Privé, à la place de `void start(std::vector<InsertionJob> jobs);` :

```cpp
    std::shared_ptr<InsertionOrigin> trackOrigin();
    void releaseOrigin(InsertionOrigin& origin);
    void start(std::vector<InsertionJob> jobs, std::shared_ptr<InsertionOrigin> origin = {});
    void startDownload(const QUrl& url, const QString& fallback, QPointF center);
    bool insertDataUrl(const QUrl& url, QPointF center);
    void placeText(const QString& text, QPointF point, std::size_t page);
    WebImageFetch::Decide webDecider() const;
    QPointer<WebFrameItems> m_webFrames;
```
`webframeitem.h`, public, près de `policy()` : `WebContentPolicy::Verdict decideAddress(const QUrl& url) const { return decide(url); }`.

- [ ] **Étape 3 : le travail.** Dans `insertion.cpp`, ajouter `#include <QImageReader>` ; `Mode` gagne `Downloaded, // octets déjà là (téléchargement ou data:)` ; `InsertionJob` gagne `QByteArray bytes; QString fallbackText;` ; `Result` gagne :

```cpp
    struct TextFallback
    {
        QString text;
        QPointF center;
    };
    std::vector<TextFallback> texts;
```
Dans `work()`, après le bloc `Mode::Pasted` :

```cpp
        if (job.mode == Mode::Downloaded) {
            QByteArray bytes = job.bytes;
            QBuffer input(&bytes);
            input.open(QIODevice::ReadOnly);
            QImageReader reader(&input); // format déduit des octets, jamais du type annoncé (n° 24)
            const QImage image = reader.read();
            if (image.isNull()) {
                if (job.fallbackText.trimmed().isEmpty())
                    result.error = QStringLiteral("Image illisible");
                else
                    result.texts.push_back({job.fallbackText, job.center});
                continue;
            }
            item.name = job.name.toStdString();
            item.extension = reader.format().toLower().toStdString();
            if (item.extension == "jpeg")
                item.extension = "jpg";
            item.width = image.width();
            item.height = image.height();
            item.data = job.bytes;
            result.items.push_back(std::move(item));
            continue;
        }
```

- [ ] **Étape 4 : l'origine.** Avant `void Insertion::start`, ajouter :

```cpp
// Document et page devant lesquels l'insertion est demandée. La page est suivie par les changements de pages : son
// indice se décale, ou elle disparaît et l'objet est abandonné (H5-32, L5-C23). Prise à la demande : un téléchargement
// dure bien plus longtemps qu'un décodage.
struct InsertionOrigin
{
    std::uint64_t generation = 0;
    std::shared_ptr<std::optional<std::size_t>> page;
    QPointer<DocumentController> watched;
    int token = -1;
};

std::shared_ptr<InsertionOrigin> Insertion::trackOrigin()
{
    auto origin = std::make_shared<InsertionOrigin>();
    origin->generation = m_document->generation();
    origin->page =
        std::make_shared<std::optional<std::size_t>>(static_cast<std::size_t>(m_document->currentIndex()));
    origin->watched = m_document;
    origin->token = m_document->addChangeObserver([page = origin->page](const core::Changes& changes, auto) {
        for (const core::Change& change : changes) {
            if (!*page)
                return;
            if (change.kind == core::Change::Kind::PageInserted && change.page <= **page)
                ++**page;
            else if (change.kind == core::Change::Kind::PageRemoved) {
                if (change.page == **page)
                    page->reset();
                else if (change.page < **page)
                    --**page;
            }
        }
    });
    return origin;
}

void Insertion::releaseOrigin(InsertionOrigin& origin)
{
    if (origin.watched && origin.token >= 0)
        origin.watched->removeChangeObserver(origin.token);
    origin.token = -1;
}
```
Remplacer `Insertion::start` (lignes 247-305) :

```cpp
void Insertion::start(std::vector<InsertionJob> jobs, std::shared_ptr<InsertionOrigin> origin)
{
    if (m_document == nullptr) {
        if (origin)
            releaseOrigin(*origin);
        return;
    }
    for (InsertionJob& job : jobs) {
        if (job.mode != Mode::Pasted && job.mode != Mode::Downloaded)
            job.name = DocumentPortal::nameOf(m_portal, job.url);
    }
    if (m_pending++ == 0)
        emit busyChanged();
    auto* watcher = new QFutureWatcher<Result>(this);
    QThread* mainThread = QThread::currentThread();
    if (!origin)
        origin = trackOrigin();
    connect(watcher, &QFutureWatcher<Result>::finished, this, [this, watcher, origin]() {
        Result result = watcher->result();
        watcher->deleteLater();
        releaseOrigin(*origin);
        m_lastOff = result.offMainThread;
        const bool sameOrigin = m_document != nullptr && m_document->generation() == origin->generation &&
                                *origin->page && **origin->page < m_document->document().pageCount();
        const QString abandoned = QStringLiteral("Insertion abandonnée : le document affiché a changé.");
        int count = 0;
        if (m_document != nullptr && !result.items.empty()) {
            if (sameOrigin)
                count = place(*m_document, result.items, **origin->page);
            else if (result.error.isEmpty())
                result.error = abandoned;
        }
        for (const Result::TextFallback& text : result.texts) {
            if (sameOrigin) {
                placeText(text.text, text.center, **origin->page);
                ++count;
            } else if (result.error.isEmpty()) {
                result.error = abandoned;
            }
        }
        if (--m_pending == 0)
            emit busyChanged();
        if (!result.error.isEmpty())
            emit failed(result.error);
        if (count > 0)
            emit inserted(count);
    });
    watcher->setFuture(
        QtConcurrent::run([moved = std::move(jobs), mainThread]() { return work(moved, mainThread); }));
}
```

- [ ] **Étape 5 : le texte commun et le dépôt web.** Remplacer la fin d'`acceptDrop` (à partir de `// Texte brut seulement`) :

```cpp
    placeText(text, pagePoint, static_cast<std::size_t>(m_document->currentIndex()));
    emit inserted(1);
    return true;
}

// Texte brut seulement : jamais d'HTML interprété, donc rien à charger (M13).
void Insertion::placeText(const QString& text, QPointF point, std::size_t page)
{
    QTextDocument doc;
    doc.setPlainText(text);
    core::Text object;
    object.html = doc.toHtml().toStdString();
    const QSizeF size = doc.size();
    object.bounds = {point.x(), point.y(), std::max(size.width(), 1.0), std::max(size.height(), 1.0)};
    core::PageObject entry = std::move(object);
    entry.common.layer = core::Layer::Middle;
    m_document->execute(std::make_unique<core::ReplaceObjectsCommand>(page, std::vector<core::ObjectId>{},
                                                                      std::vector<core::PageObject>{std::move(entry)}));
}

void Insertion::setWebFrames(WebFrameItems* frames)
{
    if (m_webFrames == frames)
        return;
    m_webFrames = frames;
    emit webFramesChanged();
}

WebImageFetch::Decide Insertion::webDecider() const
{
    if (!m_webFrames || !m_webFrames->guardsEnabled())
        return {}; // gardes coupées : aucune politique
    return [frames = m_webFrames](const QUrl& url) {
        return frames ? frames->decideAddress(url) : WebContentPolicy::Verdict::Refuse;
    };
}

void Insertion::startDownload(const QUrl& url, const QString& fallback, QPointF center)
{
    if (m_pending++ == 0)
        emit busyChanged();
    auto origin = trackOrigin();
    auto* fetch = new WebImageFetch(this);
    connect(fetch, &WebImageFetch::finished, this,
            [this, fetch, url, fallback, center, origin](const QByteArray& data, const QString& error) {
                fetch->deleteLater();
                if (!error.isEmpty()) {
                    releaseOrigin(*origin);
                    if (--m_pending == 0)
                        emit busyChanged();
                    emit failed(error);
                    return;
                }
                InsertionJob job;
                job.url = url;
                job.mode = Mode::Downloaded;
                job.bytes = data;
                job.fallbackText = fallback;
                job.name = QFileInfo(url.path()).completeBaseName();
                job.hasCenter = true;
                job.center = center;
                std::vector<InsertionJob> jobs;
                jobs.push_back(std::move(job));
                start(std::move(jobs), origin); // compte sa propre attente : pas de clignotement de `busy`
                if (--m_pending == 0)
                    emit busyChanged();
            });
    fetch->start(url, webDecider());
}

bool Insertion::insertDataUrl(const QUrl& url, QPointF center)
{
    const QByteArray encoded = url.toEncoded();
    const qsizetype comma = encoded.indexOf(',');
    if (comma < 0 || !encoded.startsWith("data:image/"))
        return false;
    QByteArray payload = QByteArray::fromPercentEncoding(encoded.mid(comma + 1));
    if (encoded.left(comma).endsWith(";base64"))
        payload = QByteArray::fromBase64(payload);
    if (payload.isEmpty() || payload.size() > WebImageFetch::kMaxBytes)
        return false;
    InsertionJob job;
    job.url = url;
    job.mode = Mode::Downloaded;
    job.bytes = std::move(payload);
    job.hasCenter = true;
    job.center = center;
    std::vector<InsertionJob> jobs;
    jobs.push_back(std::move(job));
    start(std::move(jobs));
    return true;
}

bool Insertion::acceptWebDrop(const QVariantList& urls, const QString& text, const QString& /*html*/,
                              QPointF pagePoint)
{
    if (m_document == nullptr)
        return false;
    const QString unsupported = QStringLiteral("Adresse non prise en charge");
    QVariantList local;
    bool handled = false;
    for (const QVariant& value : urls) {
        const QUrl url = value.canConvert<QUrl>() ? value.toUrl() : QUrl::fromUserInput(value.toString());
        if (!url.isValid() || url.isEmpty())
            continue;
        const QString scheme = url.scheme().toLower();
        if (scheme == QLatin1String("http") || scheme == QLatin1String("https")) {
            startDownload(url, text.trimmed().isEmpty() ? url.toString() : text, pagePoint);
            handled = true;
        } else if (scheme == QLatin1String("data")) {
            if (!insertDataUrl(url, pagePoint))
                emit failed(unsupported);
            handled = true;
        } else if (scheme == QLatin1String("blob")) {
            emit failed(unsupported);
            handled = true;
        } else {
            local.append(url);
        }
    }
    if (handled) {
        if (!local.isEmpty())
            acceptDrop(local, {}, pagePoint);
        return true;
    }
    return acceptDrop(local, text, pagePoint);
}
```

- [ ] **Étape 6 : vérifier, format, suite complète, commit.**

```bash
git add ui/include/actcraft/ui/insertion.h ui/src/insertion.cpp ui/include/actcraft/ui/webframeitem.h ui/src/webframeitem.cpp tests/tst_ui_insertion.cpp
git commit -m "feat(ui): insertion d'une image déposée depuis une page web

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T11 — Coller une URL d'image
<a id="tache-t11"></a>

**Objectif.** `paste()` et `pasteAt()` : image du presse-papiers d'abord, puis URL `http(s)` téléchargée, puis texte. Le bandeau montre l'échec (cadrage n° 22, E9, S2). Le cadrage suppose le bandeau déjà relié, or `ObjectEditing` a sa propre `Insertion` (`objectediting.cpp:128`), non reliée à `Main.qml:840` : cette tâche ajoute le relais.

**Fichiers.** Modifier : `ui/include/actcraft/ui/objectediting.h`, `ui/src/objectediting.cpp:128-140,335-375`, `ui/qml/Main.qml`, `tests/tst_ui_objectediting.cpp`.

**Interfaces.**
- Consomme : T10 : `Insertion::acceptWebDrop`, `setWebFrames` ; T1 : `TestServer`.
- Produit : `Q_PROPERTY(actcraft::ui::WebFrameItems* webFrames READ webFrames WRITE setWebFrames NOTIFY webFramesChanged)` sur `ObjectEditing` ; signal `void insertionFailed(const QString& message);`.

**Test et commande bornée.**
```bash
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T11 --attente-secondes 3000 --commande "cmake --build build-mac -j2 && QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_ui_(objectediting|insertion)$' --output-on-failure --timeout 300 -V > .tmp/t11.txt 2>&1"
grep -E 'PASS   : .*(pasteImageAddressDownloads|pasteNonImagePageBecomesText|pasteNotQuiteAnAddressStaysText|pasteHtmlLoadsNothing)|tests passed|tests failed' .tmp/t11.txt
```

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, avec `build-mac` : `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1` (suite complète, lue par `tail -3 .tmp/test.txt`), sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T11 --attente-secondes 3000 --commande "<commande>"` ; une seule tâche qui construit à la fois (8 Go).
- Format : `~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror` sur les fichiers touchés.
- `tst_ui_linklauncher` (`htaRefused`) : rouge préexistant instable, pas une régression.
- Aucun nom repris d'un produit existant ; aucune ressource de Promethean.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** Un HTML collé reste du texte (`pasteHtmlLoadsNothing`). Pas de repli texte sur échec réseau (H14).

**Critère « terminé ».** Tests nommés verts, suite complète sans autre rouge que `tst_ui_linklauncher`, commit fait.

**Étapes.**

- [ ] **Étape 1 : tests rouges.** Dans `tests/tst_ui_objectediting.cpp` (`#include "support/testserver.h"`, `<QBuffer>`, `<QSignalSpy>`) :

```cpp
    // S2 : l'URL d'une image, copiée puis collée, devient une image téléchargée.
    void pasteImageAddressDownloads()
    {
        TestServer server;
        QVERIFY(server.listening());
        QImage picture(50, 40, QImage::Format_RGB32);
        picture.fill(Qt::green);
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        picture.save(&buffer, "PNG");
        server.page(QStringLiteral("/photo.png"), bytes);
        Fixture f(0, false);
        QGuiApplication::clipboard()->setText(server.url(QStringLiteral("/photo.png")).toString());
        f.editing.paste();
        QTRY_VERIFY_WITH_TIMEOUT(f.objectCount() == 1 && f.lastObjectIs<core::ImageObject>(), 10000);
        f.document.undo();
        QCOMPARE(f.objectCount(), 0);
    }

    // E9 : page non image → objet texte, après la réponse.
    void pasteNonImagePageBecomesText()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/page"), "<p>texte</p>");
        Fixture f(0, false);
        QGuiApplication::clipboard()->setText(server.url(QStringLiteral("/page")).toString());
        f.editing.paste();
        QCOMPARE(f.objectCount(), 0); // rien tant que la réponse n'est pas là
        QTRY_VERIFY_WITH_TIMEOUT(f.objectCount() == 1 && f.lastObjectIs<core::Text>(), 10000);
    }

    // S2 : 404 → message, document inchangé.
    void pasteMissingAddressReportsTheFailure()
    {
        TestServer server;
        QVERIFY(server.listening());
        Fixture f(0, false);
        QSignalSpy failed(&f.editing, &ui::ObjectEditing::insertionFailed);
        QGuiApplication::clipboard()->setText(server.url(QStringLiteral("/absent.png")).toString());
        f.editing.pasteAt({100, 100});
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 10000);
        QCOMPARE(f.objectCount(), 0);
    }

    // Review Focus 5 (n° 22, E9) : compte pour une URL le texte entier, sur une ligne, en http(s), avec un hôte.
    void pasteNotQuiteAnAddressStaysText_data()
    {
        QTest::addColumn<QString>("text");
        QTest::newRow("deux lignes") << QStringLiteral("https://a.example/x\nhttps://b.example/y");
        QTest::newRow("espace") << QStringLiteral("https://a.example/x y");
        QTest::newRow("phrase") << QStringLiteral("voir https://a.example/x");
        QTest::newRow("schema") << QStringLiteral("ftp://a.example/x");
        QTest::newRow("sans hote") << QStringLiteral("https:///x");
    }

    void pasteNotQuiteAnAddressStaysText()
    {
        QFETCH(QString, text);
        Fixture f(0, false);
        QGuiApplication::clipboard()->setText(text);
        f.editing.paste();
        QTRY_COMPARE(f.objectCount(), 1); // sans passer par le réseau
        QVERIFY(f.lastObjectIs<core::Text>());
    }
```
`lastObjectIs` ne s'appelle qu'après `objectCount() == 1` (fait ci-dessus). Rouge attendu : `no member named 'insertionFailed'`.

- [ ] **Étape 2 : l'en-tête.** `objectediting.h` : `#include <actcraft/ui/webframeitem.h>` ; propriété `webFrames` ; publique : `WebFrameItems* webFrames() const;`, `void setWebFrames(WebFrameItems* frames);` ; signaux : `void insertionFailed(const QString& message); void webFramesChanged();`.

- [ ] **Étape 3 : le code.** Dans le constructeur, après `m_insertion = new Insertion(this);` :

```cpp
    connect(m_insertion, &Insertion::failed, this, &ObjectEditing::insertionFailed);
    connect(m_insertion, &Insertion::webFramesChanged, this, &ObjectEditing::webFramesChanged);
```
Et :

```cpp
WebFrameItems* ObjectEditing::webFrames() const
{
    return m_insertion->webFrames();
}

void ObjectEditing::setWebFrames(WebFrameItems* frames)
{
    m_insertion->setWebFrames(frames);
}
```
Dans l'espace anonyme précédant `paste()` :

```cpp
// Une URL n'est reconnue que si le texte entier, sur une ligne, en est une (n° 22).
bool singleWebAddress(const QString& text, QUrl& url)
{
    const QString trimmed = text.trimmed();
    static const QRegularExpression space(QStringLiteral("\\s"));
    if (trimmed.isEmpty() || trimmed.contains(space))
        return false;
    url = QUrl(trimmed, QUrl::StrictMode);
    return url.isValid() && !url.host().isEmpty() &&
           (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https"));
}
```
Remplacer le bloc `mime->hasText()` de `paste()` :

```cpp
    if (mime->hasText()) { // texte brut seulement : un HTML venu d'ailleurs n'est jamais interprété
        const QSizeF size = m_document->pageSize();
        const QPointF center(size.width() / 2, size.height() / 2);
        QUrl address;
        if (singleWebAddress(mime->text(), address))
            m_insertion->acceptWebDrop({address}, mime->text(), QString(), center);
        else
            m_insertion->acceptDrop({}, mime->text(), center);
    }
```
Dans `pasteAt()`, remplacer les lignes 370-371 :

```cpp
        if (mime != nullptr && !mime->hasImage() && mime->hasText()) {
            QUrl address;
            if (singleWebAddress(mime->text(), address))
                m_insertion->acceptWebDrop({address}, mime->text(), QString(), pagePoint);
            else
                m_insertion->acceptDrop({}, mime->text(), pagePoint);
        } else {
            paste();
        }
```
(`#include <QRegularExpression>`.) Dans `Main.qml`, après le `Connections` de `onFailed` d'`appExchange` (ligne 876-880) :

```qml
    Connections {
        target: appEditing
        function onInsertionFailed(message) { messageBanner.show(message) } // collage d'une URL (S2)
    }
```

- [ ] **Étape 4 : vérifier, format, suite complète, commit.**

```bash
git add ui/include/actcraft/ui/objectediting.h ui/src/objectediting.cpp ui/qml/Main.qml tests/tst_ui_objectediting.cpp
git commit -m "feat(ui): coller l'URL d'une image la télécharge

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T5 — Couche branchée sur le vrai moteur
<a id="tache-t5"></a>

**Objectif.** La couche suit le réglage réel : gardes coupées par défaut, adresse et fichier lourd transmis, vue recréée au changement de réglage et détruite avec sa page, rapport et commandes branchés ; preuve sur le vrai moteur (cadrage n° 5, 10 à 12, 15, 16, 32, 33, 38, E1, E2, E8, E11, E12, S1, S3).

**Fichiers.** Modifier : `ui/qml/WebFrameLayer.qml`, `ui/qml/WebFrameEngineView.qml`, `tests/qml/FakeWebFrameView.qml`, `tests/tst_ui_webframelayer.cpp`, `tests/tst_ui_webengine.cpp`.

**Interfaces.**
- Consomme : T1 : contrat, `WebFrameHost` ; T2 : `guardsEnabled` ; T3 : rôles `directUrl`, `contentFile` ; T4 : `reportView`, `viewCommand`.
- Produit : `objectName` conservés ; `WebFrameLayer` : délégué avec alias `property alias webView: viewHost.webView` (`readonly property var webView` sur `viewHost`) ; `Repeater { id: frames }`.

**Test et commande bornée.**
```bash
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T5 --attente-secondes 3000 --commande "cmake --build build-mac -j2 && QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_(ui_webengine|ui_webframelayer|ui_webframe|captures_cadre_web)$' --output-on-failure --timeout 300 -V > .tmp/t5.txt 2>&1"
grep -E 'PASS   : .*(javaScriptRunsGuardsOff|cookieStaysInItsDocument|heavyHtmlLoads|toggleGuardsRecreatesTheView|pageChangeDestroysTheView)|SCENARIO OK|tests passed|tests failed' .tmp/t5.txt
```

**Preuve visuelle.**
```bash
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T5 --attente-secondes 3000 --commande "QT_QPA_PLATFORM=offscreen build-mac/actcraft-captures --scenario bench/scenarios/cadre-web.json --rendu logiciel --taille 1280x800 --sortie .tmp/captures-t5 --lire webFrameLayer.loadedViews=1 --lire webFrameLayer.count=1 > .tmp/captures-t5.txt 2>&1"
grep -E '^LU |SCENARIO|CAPTURES (OK|EN ECHEC)' .tmp/captures-t5.txt
```
Lire d'abord les lignes `LU` (`webFrameLayer.loadedViews=1`, `webFrameLayer.count=1`, sans `ECART`). Puis décrire `.tmp/captures-t5/cadre.png` (cadre bleu « Cadre d'essai ActCraft » avec sa barre) dans le rapport de tâche. Sortie sous `.tmp/`, jamais commitée.

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, avec `build-mac` : `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1` (suite complète, lue par `tail -3 .tmp/test.txt`), sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T5 --attente-secondes 3000 --commande "<commande>"` ; une seule tâche qui construit à la fois (8 Go).
- Format : `~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror` sur les fichiers touchés.
- `tst_ui_linklauncher` (`htaRefused`) : rouge préexistant instable, pas une régression.
- Tâche d'interface : test de chargement et JSON du banc hors écran (`scripts/bench.sh --rendu logiciel --hors-ecran`), résultat et non porte.
- Tests sans réseau : serveur local 127.0.0.1 seulement.
- Aucun nom repris d'un produit existant ; aucune ressource de Promethean.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** Ne pas filtrer par intercepteur de requêtes (hors périmètre). Pas de menu contextuel propre (H13). Ne pas modifier `WebContentPolicy`.

**Patron à ne pas reprendre.** Changer `guardsEnabled` ou le profil sur une vue vivante (`ui/qml/WebFrameLayer.qml:156-160, 191-194` gardait la vue et vidait les cookies) : ni le profil ni JavaScript ne se changent à chaud, la vue est recréée. Et garder une vue à JavaScript actif quand le cadre passe au repère de blocage : elle tournerait, cachée.

**Critère « terminé ».** Tests nommés verts, preuve visuelle, banc joué, suite complète, commit fait.

**Étapes.**

- [ ] **Étape 1 : tests rouges, faux composant.** Dans `tests/qml/FakeWebFrameView.qml`, ajouter :

```qml
    function _emitReport(pageUrl, canBack, canForward, pageTitle, pageLoading) {
        root.viewReported(pageUrl, canBack, canForward, pageTitle, pageLoading)
    }
    function _emitLoaded() { root.pageLoaded() }
```
Dans `tests/tst_ui_webframelayer.cpp`, après `deliversTheLatestSerialAtReady` :

```cpp
    // Sans gardes, l'adresse part telle quelle (showsView accepte une directUrl, n° 11).
    void directUrlGoesToTheView()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        host.useFakeView();
        const QUrl address(QStringLiteral("https://exemple.invalide/"));
        QVERIFY(host.frames->insertFrame({}, address.toString()));
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameView")), 5000);
        QQuickItem* view = findItem(host.root.get(), QStringLiteral("webFrameView"));
        QTRY_COMPARE(view->property("directUrl").toUrl(), address);
        QCOMPARE(view->property("guardsEnabled").toBool(), false);
        QTRY_COMPARE(view->property("_loads").toInt(), 1);
    }

    // E2 : seule une vue native au-dessus de la scène est masquée par une boîte ouverte (n° 16).
    void coveredOnlyHidesAnOverlayView()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        host.useFakeView();
        QVERIFY(host.frames->insertFrame(QStringLiteral("<p>a</p>"), {}));
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameView")), 5000);
        QQuickItem* viewHost = findItem(host.root.get(), QStringLiteral("webFrameViewHost"));
        QQuickItem* view = findItem(host.root.get(), QStringLiteral("webFrameView"));
        QTRY_VERIFY(viewHost->isVisible());
        host.root->setProperty("inputBlocked", true);
        QVERIFY(viewHost->isVisible());
        view->setProperty("_overlays", true);
        QTRY_VERIFY(!viewHost->isVisible());
    }

    // E12 : changer le réglage recrée la vue (profil et JavaScript ne se changent pas à chaud, n° 10).
    void toggleGuardsRecreatesTheView()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        host.useFakeView();
        QVERIFY(host.frames->insertFrame(QStringLiteral("<p>a</p>"), {}));
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameView")), 5000);
        QPointer<QQuickItem> first = findItem(host.root.get(), QStringLiteral("webFrameView"));
        QCOMPARE(first->property("guardsEnabled").toBool(), false);
        host.frames->setGuardsEnabled(true);
        QTRY_VERIFY_WITH_TIMEOUT(first.isNull(), 5000);
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameView")), 5000);
        QCOMPARE(findItem(host.root.get(), QStringLiteral("webFrameView"))->property("guardsEnabled").toBool(), true);
    }

    // n° 32 : une vue par cadre de la page affichée, détruite au changement de page.
    void pageChangeDestroysTheView()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        host.useFakeView();
        QVERIFY(host.frames->insertFrame(QStringLiteral("<p>a</p>"), {}));
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameView")), 5000);
        QPointer<QQuickItem> first = findItem(host.root.get(), QStringLiteral("webFrameView"));
        host.document.nextPage();
        QTRY_VERIFY_WITH_TIMEOUT(first.isNull(), 5000);
    }

    void reportAndCommandsCrossTheContract()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        host.useFakeView();
        QVERIFY(host.frames->insertFrame({}, QStringLiteral("https://exemple.invalide/")));
        QTRY_VERIFY_WITH_TIMEOUT(findItem(host.root.get(), QStringLiteral("webFrameView")), 5000);
        QQuickItem* view = findItem(host.root.get(), QStringLiteral("webFrameView"));
        QVERIFY(QMetaObject::invokeMethod(view, "_emitReport",
                                          Q_ARG(QVariant, QUrl(QStringLiteral("https://exemple.invalide/b"))),
                                          Q_ARG(QVariant, true), Q_ARG(QVariant, false),
                                          Q_ARG(QVariant, QStringLiteral("Page B")), Q_ARG(QVariant, false)));
        const QModelIndex row = host.frames->index(0, 0);
        QTRY_COMPARE(host.frames->data(row, actcraft::ui::WebFrameItems::TitleRole).toString(), QStringLiteral("Page B"));
        host.frames->goBack(0);
        QTRY_VERIFY(view->property("_calls").toStringList().contains(QStringLiteral("goBack")));
    }
```
(`#include <QPointer>`.)

- [ ] **Étape 2 : tests rouges, vrai moteur.** Dans `tests/tst_ui_webengine.cpp`, ajouter :

```cpp
namespace {

// La vue seule : réglages du moteur, sans modèle ni couche.
struct DirectView
{
    QQmlEngine engine;
    QQuickWindow window;
    std::unique_ptr<QQuickItem> root;
    QQuickItem* web = nullptr;

    bool start(bool guards)
    {
        window.resize(640, 480);
        window.show();
        if (!QTest::qWaitForWindowExposed(&window))
            return false;
        QQmlComponent component(&engine, "ActCraft.Ui", "WebFrameEngineView");
        root.reset(qobject_cast<QQuickItem*>(
            component.createWithInitialProperties({{QStringLiteral("guardsEnabled"), guards}})));
        if (!root) {
            qWarning() << component.errors();
            return false;
        }
        root->setSize(QSizeF(640, 480));
        root->setParentItem(window.contentItem());
        web = findItem(root.get(), QStringLiteral("webFrameEngine"));
        return web != nullptr;
    }
    void load(const QString& html, const QUrl& base = {})
    {
        root->setProperty("html", html);
        root->setProperty("baseUrl", base);
        QMetaObject::invokeMethod(root.get(), "load");
    }
    QString title() const { return web->property("title").toString(); }
};

const int kLoadMs = 60000;

} // namespace
```
Ajouter `#include <QQmlComponent>`, `<QQmlEngine>`, `<QTemporaryDir>`, `<memory>`. Dans la classe, après `loaderErrorLeavesAnEmptyFrame` :

```cpp
    // S1 : le script d'une page locale s'exécute (gardes coupées, le défaut).
    void javaScriptRunsGuardsOff()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        QVERIFY(host.frames->insertFrame(QStringLiteral("<title>avant</title><script>document.title='JS OK'</script>"), {}));
        QTRY_COMPARE_WITH_TIMEOUT(host.layer->property("loadedViews").toInt(), 1, kLoadMs);
        QQuickItem* web = findItem(host.root.get(), QStringLiteral("webFrameEngine"));
        QVERIFY(web);
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(), QStringLiteral("JS OK"), 10000);
    }

    // S3 : gardes allumées, JavaScript coupé sur le vrai moteur.
    void javaScriptIsOffGuardsOn()
    {
        DirectView view;
        QVERIFY(view.start(true));
        QSignalSpy loaded(view.root.get(), SIGNAL(pageLoaded()));
        view.load(QStringLiteral("<title>avant</title><script>document.title='JS'</script>"));
        QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, kLoadMs);
        QTest::qWait(300);
        QCOMPARE(view.title(), QStringLiteral("avant"));
    }

    // n° 11, 12 : base file: ; image distante chargée gardes coupées, refusée gardes allumées.
    void remoteImageWithFileBase_data()
    {
        QTest::addColumn<bool>("guards");
        QTest::addColumn<int>("requests");
        QTest::newRow("gardes coupees") << false << 1;
        QTest::newRow("gardes allumees") << true << 0;
    }

    void remoteImageWithFileBase()
    {
        QFETCH(bool, guards);
        QFETCH(int, requests);
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/p.png"), "x");
        QTemporaryDir dir;
        DirectView view;
        QVERIFY(view.start(guards));
        QSignalSpy loaded(view.root.get(), SIGNAL(pageLoaded()));
        view.load(QStringLiteral("<img src=\"%1\">").arg(server.url(QStringLiteral("/p.png")).toString()),
                  QUrl::fromLocalFile(dir.path() + u'/'));
        QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, kLoadMs);
        QTest::qWait(800);
        QCOMPARE(server.requests(), requests);
    }

    // n° 38, E11 : 3 Mo chargés par le fichier temporaire, gardes coupées et allumées.
    void heavyHtmlLoads_data()
    {
        QTest::addColumn<bool>("guards");
        QTest::newRow("gardes coupees") << false;
        QTest::newRow("gardes allumees") << true;
    }

    void heavyHtmlLoads()
    {
        QFETCH(bool, guards);
        WebFrameHost host;
        QVERIFY(host.start());
        host.frames->setGuardsEnabled(guards);
        QVERIFY(host.frames->insertFrame(QStringLiteral("<title>GROS</title><body>") + QString(3 * 1024 * 1024, u'a'), {}));
        QTRY_COMPARE_WITH_TIMEOUT(host.layer->property("loadedViews").toInt(), 1, kLoadMs);
        QQuickItem* web = findItem(host.root.get(), QStringLiteral("webFrameEngine"));
        QVERIFY(web);
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(), QStringLiteral("GROS"), 10000);
    }

    // S3 : un cookie posé dans le document A est absent du document B, gardes allumées.
    void cookieStaysInItsDocument()
    {
        TestServer server;
        QVERIFY(server.listening());
        const QByteArray image = server.url(QStringLiteral("/c.png")).toEncoded();
        server.page(QStringLiteral("/page"), "<img src=\"" + image + "\">");
        server.page(QStringLiteral("/c.png"), "x");
        server.header(QStringLiteral("/c.png"), "Set-Cookie: marque=oui");
        WebFrameHost host;
        QVERIFY(host.start());
        host.frames->setGuardsEnabled(true);
        QVERIFY(host.frames->insertFrame({}, server.url(QStringLiteral("/page")).toString()));
        QTRY_VERIFY_WITH_TIMEOUT(server.paths().contains(QStringLiteral("/c.png")), kLoadMs);
        server.reset();
        host.frames->reload(0); // même document : même profil, le cookie repart
        QTRY_VERIFY_WITH_TIMEOUT(server.paths().contains(QStringLiteral("/c.png")), 20000);
        QVERIFY(server.lastRequest(QStringLiteral("/c.png")).contains("marque=oui"));
        server.reset();
        host.document.newDocument(1024, 768);
        QVERIFY(host.frames->insertFrame({}, server.url(QStringLiteral("/page")).toString()));
        QTRY_VERIFY_WITH_TIMEOUT(server.paths().contains(QStringLiteral("/c.png")), kLoadMs);
        QVERIFY(!server.lastRequest(QStringLiteral("/c.png")).contains("marque=oui"));
    }

    // E2 : la vue reste visible sous une boîte ouverte.
    void realViewStaysVisibleUnderAPopup()
    {
        WebFrameHost host;
        QVERIFY(host.start());
        QVERIFY(host.frames->insertFrame(QStringLiteral("<p>x</p>"), {}));
        QTRY_COMPARE_WITH_TIMEOUT(host.layer->property("loadedViews").toInt(), 1, kLoadMs);
        host.root->setProperty("inputBlocked", true);
        QVERIFY(findItem(host.root.get(), QStringLiteral("webFrameViewHost"))->isVisible());
    }

    // E8 : target=_blank s'ouvre dans la même vue.
    void popupOpensInTheSameView()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/popup"), "<title>SURGIE</title>");
        DirectView view;
        QVERIFY(view.start(false));
        view.load(QStringLiteral("<a id=l target=_blank href=\"%1\">x</a><script>l.click()</script>")
                      .arg(server.url(QStringLiteral("/popup")).toString()));
        QTRY_COMPARE_WITH_TIMEOUT(view.title(), QStringLiteral("SURGIE"), kLoadMs);
    }

    // E8 : permission refusée (géolocalisation, erreur 1 = PERMISSION_DENIED).
    void permissionIsDenied()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/geo"),
                    "<script>navigator.geolocation.getCurrentPosition(function(){document.title='OK'},"
                    "function(e){document.title='REFUS'+e.code})</script>");
        DirectView view;
        QVERIFY(view.start(false));
        view.root->setProperty("directUrl", server.url(QStringLiteral("/geo")));
        QMetaObject::invokeMethod(view.root.get(), "load");
        QTRY_COMPARE_WITH_TIMEOUT(view.title(), QStringLiteral("REFUS1"), kLoadMs);
    }

    // E8 : un téléchargement lancé par la page est ignoré ; la page reste.
    void pageDownloadIsIgnored()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/f.bin"), "x");
        DirectView view;
        QVERIFY(view.start(false));
        QSignalSpy loaded(view.root.get(), SIGNAL(pageLoaded()));
        view.load(QStringLiteral("<title>RESTE</title><a id=l download href=\"%1\">x</a><script>l.click()</script>")
                      .arg(server.url(QStringLiteral("/f.bin")).toString()));
        QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, kLoadMs);
        QTest::qWait(500);
        QCOMPARE(view.title(), QStringLiteral("RESTE"));
    }
```
(`#include <QSignalSpy>`.) Rouge attendu : `toggleGuardsRecreatesTheView` et `directUrlGoesToTheView` échouent, `popupOpensInTheSameView` et `permissionIsDenied` échouent.

- [ ] **Étape 3 : la couche.** Dans `WebFrameLayer.qml` :
  - `Repeater` : ajouter `id: frames`.
  - Délégué : ajouter aux propriétés requises `required property url directUrl` et `required property url contentFile`, et `readonly property var webView: viewHost.webView`.
  - Remplacer la ligne 144 :

```qml
            readonly property bool showsView: !blocked && !refused && (content.length > 0 || directUrl.toString().length > 0)
```
  - Dans `viewHost`, remplacer `property bool reported: false` par :

```qml
                property bool reported: false
                // Réglage pour lequel la vue a été créée. Un autre document : le modèle est remis à zéro (rebuild), le
                // délégué et sa vue sont recréés, donc un profil neuf par document (H8-13).
                property bool madeGuards: false
                readonly property var webView: loader.status === Loader.Ready ? loader.item : null
```
  - Dans `Connections { target: loader.item`, ajouter :

```qml
                    function onViewReported(pageUrl, canBack, canForward, pageTitle, pageLoading) {
                        appWebFrames.reportView(frame.index, pageUrl, canBack, canForward, pageTitle, pageLoading)
                    }
```
  - Remplacer `deliver()` et `show()` :

```qml
                function deliver() {
                    if (loader.status !== Loader.Ready || !frame.showsView)
                        return
                    const view = loader.item
                    view.html = frame.content
                    view.baseUrl = frame.baseUrl
                    view.directUrl = frame.directUrl
                    view.contentFile = frame.contentFile
                    view.serial = frame.serial
                    view.load()
                }
                function show() {
                    const guards = appWebFrames.guardsEnabled
                    // Profil et JavaScript ne se changent pas à chaud (n° 10, 33) ; avant le test de showsView : une
                    // vue active ne survit pas, cachée, au passage d'un cadre au repère de blocage.
                    if (loader.status !== Loader.Null && madeGuards !== guards)
                        loader.source = ""
                    if (!frame.showsView || frameLayer.viewSource.toString() === "")
                        return
                    if (loader.status === Loader.Null) {
                        madeGuards = guards
                        reported = false
                        loader.setSource(frameLayer.viewSource, { guardsEnabled: guards })
                        return
                    }
                    deliver()
                }
```
  - Après le `Item { id: viewHost … }`, dans le délégué :

```qml
            Connections {
                target: appWebFrames
                function onViewCommand(row, action) {
                    if (row !== frame.index || !viewHost.webView)
                        return
                    if (action === "back")
                        viewHost.webView.goBack()
                    else if (action === "forward")
                        viewHost.webView.goForward()
                    else if (action === "reload")
                        viewHost.webView.reload()
                }
            }
```

- [ ] **Étape 4 : le moteur, E8.** Dans `WebFrameEngineView.qml`, dans `WebEngineView`, après `settings.localContentCanAccessRemoteUrls` :

```qml
        // E8 : une fenêtre surgissante reste dans le cadre ; les permissions de la page sont refusées.
        onNewWindowRequested: function (request) { web.url = request.requestedUrl }
        onPermissionRequested: function (permission) { permission.deny() }
```

- [ ] **Étape 5 : vérifier.** Commande bornée, puis preuve visuelle.

- [ ] **Étape 6 : format, suite complète, banc.** `clang-format --dry-run --Werror tests/tst_ui_webengine.cpp tests/tst_ui_webframelayer.cpp` ; suite complète sous le jeton ; le banc sous le jeton : `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T5 --attente-secondes 3000 --duree-max-minutes 180 --commande "ACTCRAFT_QT_DIR=$(brew --prefix qt) CMAKE_BUILD_PARALLEL_LEVEL=2 scripts/bench.sh --rendu logiciel --hors-ecran --scenario court --taille 1280x800 > .tmp/bench.txt 2>&1"`, puis `tail -1 .tmp/bench.txt` ; verdict et chemin du JSON au rapport (résultat, pas porte).

- [ ] **Étape 7 : commit.**

```bash
git add ui/qml/WebFrameLayer.qml ui/qml/WebFrameEngineView.qml tests/qml/FakeWebFrameView.qml tests/tst_ui_webframelayer.cpp tests/tst_ui_webengine.cpp
git commit -m "feat(ui): cadre web branché sur le vrai moteur et le réglage des gardes

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T6 — Capture du cadre (`insertSnapshot`)
<a id="tache-t6"></a>

**Objectif.** Sous WebEngine, l'image fixe vient de `grabToImage` et se pose en une étape ; la capture par fenêtre ne reste que sous Android (cadrage n° 18).

**Fichiers.** Modifier : `ui/include/actcraft/ui/webframeitem.h`, `ui/src/webframeitem.cpp:1647-1670`, `ui/qml/WebFrameLayer.qml:94,99,113-116`, `tests/tst_ui_webframe.cpp`, `tests/tst_ui_webengine.cpp`.

**Interfaces.**
- Consomme : T5 : `frames.itemAt(row).webView`, contrat `capture(callback)`, `overlaysScene`.
- Produit : `Q_INVOKABLE bool WebFrameItems::insertSnapshot(int row, const QImage& image);` (une étape, faux si `row` invalide ou image nulle) ; `function WebFrameLayer.finishSnapshot(ok)`.

**Test et commande bornée.**
```bash
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T6 --attente-secondes 3000 --commande "cmake --build build-mac -j2 && QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_(ui_webengine|ui_webframe|captures_cadre_web)$' --output-on-failure --timeout 300 -V > .tmp/t6.txt 2>&1"
grep -E 'PASS   : .*(snapshotInsertsAnImage|insertSnapshotIsOneStep|snapshotPlacesAnImage)|SCENARIO OK|tests passed|tests failed' .tmp/t6.txt
```

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, avec `build-mac` : `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1` (suite complète, lue par `tail -3 .tmp/test.txt`), sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T6 --attente-secondes 3000 --commande "<commande>"` ; une seule tâche qui construit à la fois (8 Go).
- Format : `~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror` sur les fichiers touchés.
- `tst_ui_linklauncher` (`htaRefused`) : rouge préexistant instable, pas une régression.
- Ce qui exige Android part en « Restes pour le PC » ou R2 (capture `grabWindow` non vérifiée ici).
- Aucun nom repris d'un produit existant ; aucune ressource de Promethean.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** `Grabber` et `snapshot()` restent (tests, Android).

**Critère « terminé ».** Tests nommés verts, suite complète, commit fait.

**Étapes.**

- [ ] **Étape 1 : tests rouges.** `tests/tst_ui_webframe.cpp` :

```cpp
void TstUiWebFrame::insertSnapshotIsOneStep()
{
    Fixture f;
    QVERIFY(f.frames.insertFrame(QStringLiteral("<p>a</p>"), {}));
    QImage image(32, 16, QImage::Format_RGB32);
    image.fill(Qt::red);
    QVERIFY(!f.frames.insertSnapshot(0, QImage()));
    QVERIFY(!f.frames.insertSnapshot(7, image));
    const std::size_t before = f.document.document().page(0).objects.size();
    QVERIFY(f.frames.insertSnapshot(0, image));
    QCOMPARE(f.document.document().page(0).objects.size(), before + 1);
    f.document.undo();
    QCOMPARE(f.document.document().page(0).objects.size(), before);
}
```
`tests/tst_ui_webengine.cpp` :

```cpp
    // n° 18 : image réelle du cadre, posée par la commande, une étape.
    void snapshotInsertsAnImage()
    {
        presentationharness::MainWindow main;
        QVERIFY(main.window());
        QQuickItem* layer = main.item(QStringLiteral("webFrameLayer"));
        QVERIFY(layer);
        QObject* frames = layer->findChild<QObject*>(QStringLiteral("appWebFrames"));
        bool inserted = false;
        QVERIFY(QMetaObject::invokeMethod(frames, "insertFrame", Q_RETURN_ARG(bool, inserted),
                                          Q_ARG(QString, QStringLiteral("<body style='background:#c81e1e'>x")),
                                          Q_ARG(QString, QString())));
        QTRY_COMPARE_WITH_TIMEOUT(layer->property("loadedViews").toInt(), 1, 60000);
        QTest::qWait(500);
        QVERIFY(main.waitReady());
        const int before = main.pageObjectCount();
        QVERIFY(main.run(QStringLiteral("webframe.snapshot")));
        QTRY_COMPARE_WITH_TIMEOUT(layer->property("snapshotCount").toInt(), 1, 20000);
        QTRY_COMPARE_WITH_TIMEOUT(main.pageObjectCount(), before + 1, 10000);
    }
```
Rouge attendu : `no member named 'insertSnapshot'`.

- [ ] **Étape 2 : le modèle.** `webframeitem.h`, après `snapshot` : `Q_INVOKABLE bool insertSnapshot(int row, const QImage& image);`. Dans `webframeitem.cpp`, extraire la fin de `snapshot` :

```cpp
bool WebFrameItems::snapshot(int row)
{
    if (!m_document || !m_grabber || row < 0 || row >= m_rows.size())
        return false;
    return insertSnapshot(row, m_grabber(m_rows[row].bounds));
}

// Image obtenue hors du modèle (grabToImage de la vue, n° 18) : posée en une étape sur la zone du cadre.
bool WebFrameItems::insertSnapshot(int row, const QImage& image)
{
    if (!m_document || row < 0 || row >= m_rows.size() || image.isNull())
        return false;
    const QRectF zone = m_rows[row].bounds;
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "PNG"))
        return false;
    core::ImageObject picture;
    picture.bounds = {zone.x(), zone.y(), zone.width(), zone.height()};
    picture.sourceWidth = image.width();
    picture.sourceHeight = image.height();
    core::PageObject object = picture;
    object.common.layer = core::Layer::Middle;
    object.common.name = tr("Image du cadre web").toStdString();
    m_document->execute(std::make_unique<SnapshotCommand>(static_cast<std::size_t>(m_page), std::move(object),
                                                          std::move(png)));
    return true;
}
```

- [ ] **Étape 3 : la couche.** Dans `WebFrameLayer.qml`, ligne 94 : `onViewChanged: if (view && Qt.platform.os === "android") appWebFrames.useWindowCapture(view)`. Lignes 98-99 : `if (view && Qt.platform.os === "android") appWebFrames.useWindowCapture(view)`. Remplacer le gestionnaire `webframe.snapshot` :

```qml
        commands.setHandler("webframe.snapshot", function (arg) {
            const row = frameLayer.rowOf(arg)
            const delegate = row >= 0 ? frames.itemAt(row) : null
            const webView = delegate ? delegate.webView : null
            if (webView && !webView.overlaysScene) {
                webView.capture(function (image) {
                    frameLayer.finishSnapshot(image ? appWebFrames.insertSnapshot(row, image) : false)
                })
                return
            }
            frameLayer.finishSnapshot(appWebFrames.snapshot(row)) // vue native : capture par la fenêtre
        })
```
Après `uncover(item)` :

```qml
    function finishSnapshot(ok) {
        console.info("ACTCRAFT IMAGE DU CADRE " + (ok ? "OK" : "ECHEC"))
        snapshotCount += 1
    }
```

- [ ] **Étape 4 : vérifier, format, suite complète, commit.**

```bash
git add ui/include/actcraft/ui/webframeitem.h ui/src/webframeitem.cpp ui/qml/WebFrameLayer.qml tests/tst_ui_webframe.cpp tests/tst_ui_webengine.cpp
git commit -m "feat(ui): image fixe du cadre par la vue WebEngine

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T7 — Stylet : couverture d'entrée de la vue
<a id="tache-t7"></a>

**Objectif.** Le stylet ne dessine pas à travers un cadre : la vue WebEngine est déclarée couverture (cadrage n° 17, E3).

**Fichiers.** Modifier : `ui/qml/WebFrameLayer.qml`, `tests/tst_ui_pageinput.cpp`, `tests/ui.cmake`.

**Interfaces.**
- Consomme : T5 : `Loader`, `viewHost` ; `frameLayer.cover`, `uncover` (`WebFrameLayer.qml:85-92`) ; `PageView::addInputCover`.
- Produit : rien de nouveau.

**Test et commande bornée.**
```bash
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T7 --attente-secondes 3000 --commande "cmake --build build-mac -j2 && QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_ui_pageinput$' --output-on-failure --timeout 300 -V > .tmp/t7.txt 2>&1"
grep -E 'PASS   : .*stylusOnWebFrameDrawsNothing|^Totals:|tests passed|tests failed' .tmp/t7.txt
```

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, avec `build-mac` : `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1` (suite complète, lue par `tail -3 .tmp/test.txt`), sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T7 --attente-secondes 3000 --commande "<commande>"` ; une seule tâche qui construit à la fois (8 Go).
- Format : `~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror` sur les fichiers touchés.
- `tst_ui_linklauncher` (`htaRefused`) : rouge préexistant instable, pas une régression.
- Tâche d'interface (entrée) : test de chargement et JSON du banc hors écran (`scripts/bench.sh --rendu logiciel --hors-ecran`), résultat et non porte.
- Aucun nom repris d'un produit existant ; aucune ressource de Promethean.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** Ne pas modifier `PageView`.

**Critère « terminé ».** Test nommé vert (et rouge avant le correctif), banc joué, suite complète, commit fait.

**Étapes.**

- [ ] **Étape 1 : test rouge.** Dans `tests/tst_ui_pageinput.cpp`, ajouter à la classe (suivre l'accès à `App` et `m_stylus11` des tests voisins, ex. ligne 272 ; `#include <QUrl>`) :

```cpp
    // E3 : le stylet sur un cadre web ne trace rien (vue WebEngine déclarée couverture, n° 17).
    void stylusOnWebFrameDrawsNothing()
    {
        App app;
        QVERIFY(start(app));
        QQuickItem* layer = presentationharness::findItem(app.window->contentItem(), QStringLiteral("webFrameLayer"));
        QVERIFY(layer);
        layer->setProperty("viewSource", QUrl::fromLocalFile(QStringLiteral(ACTCRAFT_SOURCE_DIR
                                                                            "/tests/qml/FakeWebFrameView.qml")));
        QObject* frames = layer->findChild<QObject*>(QStringLiteral("appWebFrames"));
        QVERIFY(frames);
        bool inserted = false;
        QVERIFY(QMetaObject::invokeMethod(frames, "insertFrame", Q_RETURN_ARG(bool, inserted),
                                          Q_ARG(QString, QStringLiteral("<p>a</p>")), Q_ARG(QString, QString())));
        QVERIFY(inserted);
        QTRY_VERIFY_WITH_TIMEOUT(presentationharness::findItem(app.window->contentItem(), QStringLiteral("webFrameView")),
                                 5000);
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->objectCount(), 1, 5000); // le cadre
        const QPointF inside(512, 384); // centre du cadre, au centre de la page
        tablet(app, m_stylus11.get(), QEvent::TabletPress, inside);
        tablet(app, m_stylus11.get(), QEvent::TabletMove, inside + QPointF(40, 10));
        tablet(app, m_stylus11.get(), QEvent::TabletRelease, inside + QPointF(40, 10));
        QTest::qWait(300);
        QCOMPARE(app.fixed->objectCount(), 1);
    }
```
Ajouter `#include "support/presentationharness.h"` si absent. Dans `tests/ui.cmake`, après le `target_include_directories` de `tst_ui_pageinput` : `target_compile_definitions(tst_ui_pageinput PRIVATE ACTCRAFT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")`. Rouge attendu : `objectCount` vaut 2.

- [ ] **Étape 2 : la couverture.** Dans `WebFrameLayer.qml`, dans le `Loader` :

```qml
                    onStatusChanged: {
                        if (status === Loader.Ready) {
                            if (!item.overlaysScene) {
                                viewHost.covering = item
                                frameLayer.cover(item) // sans cela le filtre de PageView prend le stylet (n° 17)
                            }
                            viewHost.deliver()
                        } else if (status === Loader.Error) {
                            console.warn("Cadre web : vue indisponible (" + source + ")")
                        } else if (status === Loader.Null && viewHost.covering) {
                            frameLayer.uncover(viewHost.covering)
                            viewHost.covering = null
                        }
                    }
```
Dans `viewHost` : `property var covering: null` et `Component.onDestruction: if (covering) frameLayer.uncover(covering)`.

- [ ] **Étape 3 : vérifier, format, suite complète, banc, commit.** Commande bornée, `clang-format --dry-run --Werror tests/tst_ui_pageinput.cpp`, suite complète et banc sous le jeton (`cadre-web-webengine/T7`, commande complète du banc : `ACTCRAFT_QT_DIR=$(brew --prefix qt) CMAKE_BUILD_PARALLEL_LEVEL=2 scripts/bench.sh --rendu logiciel --hors-ecran --scenario court --taille 1280x800 > .tmp/bench.txt 2>&1`).

```bash
git add ui/qml/WebFrameLayer.qml tests/tst_ui_pageinput.cpp tests/ui.cmake
git commit -m "fix(ui): le stylet ne dessine pas à travers un cadre web

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T8 — Textes selon les gardes
<a id="tache-t8"></a>

**Objectif.** « JavaScript reste coupé » n'apparaît que gardes allumées (cadrage E7, carte `WebFrameLayer.qml:335`, `WebFrameDialog.qml:96`, `actcraft_fr.ts:3169,3207`).

**Fichiers.** Modifier : `ui/qml/WebFrameLayer.qml:335`, `ui/qml/WebFrameDialog.qml:92-97`, `ui/i18n/actcraft_fr.ts`, `tests/tst_ui_webframe.cpp`.

**Interfaces.** Consomme : T2 : `guardsEnabled` ; `WebFrameDialog.frames`. Produit : `objectName: "webFrameDialogNote"` sur le libellé de la boîte.

**Test et commande bornée.**
```bash
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T8 --attente-secondes 3000 --commande "cmake --build build-mac -j2 && QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_ui_webframe$' --output-on-failure --timeout 300 -V > .tmp/t8.txt 2>&1"
grep -E 'PASS   : .*dialogNoteFollowsTheGuards|^Totals:|tests passed|tests failed' .tmp/t8.txt
```

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, avec `build-mac` : `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1` (suite complète, lue par `tail -3 .tmp/test.txt`), sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T8 --attente-secondes 3000 --commande "<commande>"` ; une seule tâche qui construit à la fois (8 Go).
- Format : `~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror` sur les fichiers touchés.
- `tst_ui_linklauncher` (`htaRefused`) : rouge préexistant instable, pas une régression.
- Aucun nom repris d'un produit existant ; aucune ressource de Promethean ; textes à nous.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** Aucune commande neuve.

**Critère « terminé ».** Test vert, suite complète, commit fait.

**Étapes.**

- [ ] **Étape 1 : test rouge.** Sur le modèle de `dialogRefusesEmptyFields` (`tests/tst_ui_webframe.cpp:435`), déclarer et écrire :

```cpp
// E7 : le message de la boîte suit le réglage.
void TstUiWebFrame::dialogNoteFollowsTheGuards()
{
    const QString qmlDir =
        QDir::cleanPath(QFileInfo(QStringLiteral(__FILE__)).absolutePath() + QStringLiteral("/../ui/qml"));
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArrayLiteral(R"qml(
import QtQuick
import QtQuick.Controls.Basic
Item {
    width: 800
    height: 600
    property alias dialog: d
    property bool guards: false
    QtObject { id: fakeFrames; property bool guardsEnabled: guards }
    WebFrameDialog { id: d; frames: fakeFrames }
}
)qml"),
                      QUrl::fromLocalFile(qmlDir + QStringLiteral("/essai-note.qml")));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QObject* dialog = root->property("dialog").value<QObject*>();
    QVERIFY(dialog);
    const auto note = [&] {
        for (QObject* object : dialog->findChildren<QObject*>()) {
            if (object->objectName() == QLatin1String("webFrameDialogNote"))
                return object->property("text").toString();
        }
        return QString();
    };
    QVERIFY2(note().contains(QStringLiteral("JavaScript actif")), qPrintable(note()));
    root->setProperty("guards", true);
    QVERIFY2(note().contains(QStringLiteral("JavaScript reste coupé")), qPrintable(note()));
}
```
Si le libellé n'est pas trouvé par `findChildren` parce qu'il est dans un `Dialog` déporté, lire les enfants à partir de `dialog->findChild<QObject*>(QStringLiteral("webFrameDialogNote"), Qt::FindChildrenRecursively)`, comme le fait `dialogRefusesEmptyFields` pour son champ.

- [ ] **Étape 2 : les textes.** `WebFrameDialog.qml`, le `Label` des lignes 92-97 :

```qml
            Label {
                objectName: "webFrameDialogNote"
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                opacity: 0.7
                text: root.frames && root.frames.guardsEnabled
                      ? qsTr("JavaScript reste coupé. Dans un document reçu, le contenu distant reste bloqué jusqu'à ce que vous débloquiez sa page.")
                      : qsTr("Le contenu du cadre s'exécute tel quel : JavaScript actif, aucune adresse filtrée.")
            }
```
`WebFrameLayer.qml:335` :

```qml
            text: appWebFrames.guardsEnabled
                  ? qsTr("Le cadre web chargera le contenu distant de cette page, pour ce document seulement. JavaScript reste coupé.")
                  : qsTr("Le cadre web chargera le contenu distant de cette page, pour ce document seulement.")
```
`ui/i18n/actcraft_fr.ts`, contextes `WebFrameDialog` et `WebFrameLayer`, ajouter chacun :

```xml
    <message>
        <source>Le contenu du cadre s&apos;exécute tel quel : JavaScript actif, aucune adresse filtrée.</source>
        <translation>Le contenu du cadre s&apos;exécute tel quel : JavaScript actif, aucune adresse filtrée.</translation>
    </message>
```
(dans `WebFrameLayer`, le message de la seconde chaîne, sans « JavaScript reste coupé. »).

- [ ] **Étape 3 : vérifier, format, suite complète, commit.**

```bash
git add ui/qml/WebFrameLayer.qml ui/qml/WebFrameDialog.qml ui/i18n/actcraft_fr.ts tests/tst_ui_webframe.cpp
git commit -m "fix(ui): textes du cadre web selon les gardes

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T12 — Récepteur de dépôt sur la page
<a id="tache-t12"></a>

**Objectif.** Un `DropArea` sous les vues reçoit texte, URL, image et fichiers du système ; preuve hors écran avec les formats exacts du moteur (cadrage n° 19, 20, 31, E4, E13, S1).

**Fichiers.** Créer : `tests/tst_ui_webdrop.cpp`. Modifier : `ui/qml/WebFrameLayer.qml`, `ui/qml/PageArea.qml`, `ui/qml/Main.qml`, `tests/tst_ui_desktop.cpp`, `tests/ui.cmake`.

**Interfaces.**
- Consomme : T10 : `Insertion.acceptWebDrop`, `webFrames` ; T11 : `ObjectEditing.webFrames` ; T1 : `TestServer`, `presentationharness::MainWindow`.
- Produit : `property var insertion` et `property var editing` sur `PageArea` et `WebFrameLayer` ; `objectName: "webFrameDropArea"`.

**Test et commande bornée.**
```bash
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T12 --attente-secondes 3000 --commande "cmake -S . -B build-mac && cmake --build build-mac -j2 && QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_ui_(webdrop|desktop)$' --output-on-failure --timeout 300 -V > .tmp/t12.txt 2>&1"
grep -E 'PASS   : .*(droppedRemoteImageSurvivesReopening|droppedSystemFileIsInserted|desktopDropAreaStaysOff)|^Totals:|tests passed|tests failed' .tmp/t12.txt
```

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, avec `build-mac` : `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1` (suite complète, lue par `tail -3 .tmp/test.txt`), sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T12 --attente-secondes 3000 --commande "<commande>"` ; une seule tâche qui construit à la fois (8 Go).
- Format : `~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror` sur les fichiers touchés.
- `tst_ui_linklauncher` (`htaRefused`) : rouge préexistant instable, pas une régression.
- Tâche d'interface : test de chargement et JSON du banc hors écran (`scripts/bench.sh --rendu logiciel --hors-ecran`), résultat et non porte.
- Tests sans réseau : serveur local 127.0.0.1 seulement. Le glisser réel à la souris reste R1.
- Aucun nom repris d'un produit existant ; aucune ressource de Promethean.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** Ni l'agent ni un test ne jouent la souris réelle (H5, `CLAUDE.md` Mode autonome 3). `DropArea` inactif si `commands` est nul (seconde `PageArea`, cadrage n° 19) et sous Android.

**Critère « terminé ».** Tests nommés verts, banc joué, suite complète, commit fait.

**Étapes.**

- [ ] **Étape 1 : tests rouges.** `tests/tst_ui_webdrop.cpp` :

```cpp
// Dépôt sur la page, côté page (cadrage n° 31) : QDropEvent envoyé à la fenêtre, avec les formats exacts du moteur.
// Le glisser réel de la vue vers la page exige une vraie souris : Reste pour Jérôme R1.
#include "startup.h"
#include "support/presentationharness.h"
#include "support/testserver.h"

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/insertion.h>

#include <QBuffer>
#include <QDropEvent>
#include <QImage>
#include <QMimeData>
#include <QTemporaryDir>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include <variant>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;
using presentationharness::MainWindow;

namespace {

void dropAt(QQuickWindow* window, QMimeData* mime, QPointF point)
{
    QDragEnterEvent enter(point.toPoint(), Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &enter);
    QDragMoveEvent move(point.toPoint(), Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &move);
    QDropEvent drop(point, Qt::CopyAction, mime, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &drop);
}

QByteArray png(int width, int height)
{
    QImage image(width, height, QImage::Format_RGB32);
    image.fill(Qt::red);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return bytes;
}

ui::DocumentController* documentOf(const MainWindow& main)
{
    return main.window()->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
}

template <typename T> const T* lastAs(const ui::DocumentController& controller)
{
    const auto& objects = controller.document().page(0).objects;
    return objects.empty() ? nullptr : std::get_if<T>(&core::body(objects.back()));
}

} // namespace

class TstUiWebDrop : public QObject
{
    Q_OBJECT
private slots:
    // Texte seul : un objet texte, Ctrl+Z en une étape.
    void droppedTextBecomesAText()
    {
        MainWindow main;
        QVERIFY(main.window());
        ui::DocumentController* document = documentOf(main);
        QVERIFY(document);
        QMimeData mime;
        mime.setText(QStringLiteral("Texte glissé"));
        dropAt(main.window(), &mime, main.pageView()->pageToScene(QPointF(300, 200)));
        QTRY_VERIFY(lastAs<core::Text>(*document) != nullptr);
        QVERIFY(lastAs<core::Text>(*document)->html.find("Texte glissé") != std::string::npos);
        document->undo();
        QCOMPARE(document->document().page(0).objects.size(), std::size_t{0});
    }

    // Image en URL file: (fichier temporaire du moteur) : une image locale.
    void droppedTemporaryFileImageIsInserted()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("glisse.png"));
        QVERIFY(QImage::fromData(png(60, 40)).save(path, "PNG"));
        MainWindow main;
        QVERIFY(main.window());
        ui::DocumentController* document = documentOf(main);
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(path)});
        dropAt(main.window(), &mime, main.pageView()->pageToScene(QPointF(300, 200)));
        QTRY_VERIFY_WITH_TIMEOUT(lastAs<core::ImageObject>(*document) != nullptr, 10000);
    }

    // E4 : un fichier glissé du système s'insère (même chemin qu'un fichier du moteur).
    void droppedSystemFileIsInserted()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("du-bureau.png"));
        QVERIFY(QImage::fromData(png(30, 30)).save(path, "PNG"));
        MainWindow main;
        QVERIFY(main.window());
        ui::DocumentController* document = documentOf(main);
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(path)});
        dropAt(main.window(), &mime, main.pageView()->pageToScene(QPointF(500, 400)));
        QTRY_VERIFY_WITH_TIMEOUT(lastAs<core::ImageObject>(*document) != nullptr, 10000);
        QCOMPARE(lastAs<core::ImageObject>(*document)->bounds.center().x, 500.0);
    }

    // URL avec son titre, HTML en plus : le HTML n'est jamais interprété (M13), la page non image donne le titre.
    void droppedLinkWithTitleFallsBackToItsTitle()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/page"), "<p>page</p>");
        MainWindow main;
        QVERIFY(main.window());
        ui::DocumentController* document = documentOf(main);
        QMimeData mime;
        mime.setUrls({server.url(QStringLiteral("/page"))});
        mime.setText(QStringLiteral("Titre du lien"));
        mime.setHtml(QStringLiteral("<a href=\"/page\"><img src=\"/espion.png\">Titre</a>"));
        dropAt(main.window(), &mime, main.pageView()->pageToScene(QPointF(200, 150)));
        QTRY_VERIFY_WITH_TIMEOUT(lastAs<core::Text>(*document) != nullptr, 10000);
        QVERIFY(lastAs<core::Text>(*document)->html.find("Titre du lien") != std::string::npos);
        QVERIFY(!server.paths().contains(QStringLiteral("/espion.png")));
    }

    // S1 : image http(s), téléchargée ; document enregistré puis rouvert serveur coupé : elle est là.
    void droppedRemoteImageSurvivesReopening()
    {
        QTemporaryDir dir;
        const QString file = dir.filePath(QStringLiteral("depot.actcraft"));
        const QByteArray bytes = png(80, 50);
        MainWindow main;
        QVERIFY(main.window());
        ui::DocumentController* document = documentOf(main);
        {
            TestServer server;
            QVERIFY(server.listening());
            server.page(QStringLiteral("/photo.png"), bytes);
            QMimeData mime;
            mime.setUrls({server.url(QStringLiteral("/photo.png"))});
            dropAt(main.window(), &mime, main.pageView()->pageToScene(QPointF(300, 200)));
            QTRY_VERIFY_WITH_TIMEOUT(lastAs<core::ImageObject>(*document) != nullptr, 10000);
            QVERIFY(document->canUndo());
            document->saveToUrl(QUrl::fromLocalFile(file));
            QTRY_VERIFY_WITH_TIMEOUT(!document->isBusy(), 10000);
        } // serveur coupé
        document->openUrl(QUrl::fromLocalFile(file));
        QTRY_VERIFY_WITH_TIMEOUT(!document->isBusy() && document->fileUrl().isValid(), 10000);
        const auto* image = lastAs<core::ImageObject>(*document);
        QVERIFY(image != nullptr);
        const core::Resource* resource = document->document().resource(image->resource);
        QVERIFY(resource != nullptr);
        QCOMPARE(resource->data, bytes);
    }

    // S1, n° 31 : image sans octets, URL http(s) en échec : bandeau, document intact.
    void droppedUnreachableImageReportsTheFailure()
    {
        TestServer server;
        QVERIFY(server.listening());
        MainWindow main;
        QVERIFY(main.window());
        ui::DocumentController* document = documentOf(main);
        auto* insertion = main.window()->findChild<ui::Insertion*>(QStringLiteral("appInsertion"));
        QVERIFY(insertion);
        QSignalSpy failed(insertion, &ui::Insertion::failed);
        QMimeData mime;
        mime.setUrls({server.url(QStringLiteral("/absente.png"))});
        dropAt(main.window(), &mime, main.pageView()->pageToScene(QPointF(300, 200)));
        QTRY_COMPARE_WITH_TIMEOUT(failed.count(), 1, 10000);
        QCOMPARE(document->document().page(0).objects.size(), std::size_t{0});
    }
};

QTEST_MAIN(TstUiWebDrop)
#include "tst_ui_webdrop.moc"
```
`tests/ui.cmake`, à la fin :

```cmake
actcraft_add_test(tst_ui_webdrop
    SOURCES tst_ui_webdrop.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_webdrop PRIVATE ${PROJECT_SOURCE_DIR}/app)
```
Dans `tests/tst_ui_desktop.cpp`, à la suite de `desktopLayerFollowsTheSetting` :

```cpp
    // E13 : la seconde PageArea n'accepte aucun dépôt (`appInsertion` vise `appDocument`, cadrage n° 19).
    void desktopDropAreaStaysOff()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        QQuickItem* area =
            presentationharness::findItem(desktop->window()->contentItem(), QStringLiteral("webFrameDropArea"));
        QVERIFY(area);
        QVERIFY(!area->isEnabled());
    }
```
Rouge attendu : `droppedTextBecomesAText` échoue (aucun récepteur) ; `desktopDropAreaStaysOff` échoue (`area` nul).

- [ ] **Étape 2 : la couche.** Dans `WebFrameLayer.qml`, après `property var profile: null` :

```qml
    property var insertion: null // appInsertion ; null : aucun dépôt possible (annotation du bureau)
    property var editing: null   // appEditing : ses collages suivent la politique
```
Dans `Component.onCompleted`, avant `if (!commands)` :

```qml
        if (insertion)
            insertion.webFrames = appWebFrames
        if (editing)
            editing.webFrames = appWebFrames
```
Avant le `Repeater`, pour que le récepteur soit sous les vues :

```qml
    // Dépôt sur la page, hors des vues (n° 19) : texte, URL, image, fichier du système (E4). Inactif sans `commands`
    // (seconde PageArea : `appInsertion` viserait le mauvais document) et sous Android.
    DropArea {
        id: pageDrop
        objectName: "webFrameDropArea"
        anchors.fill: parent
        enabled: frameLayer.commands !== null && frameLayer.insertion !== null && Qt.platform.os !== "android"
        onEntered: function (drag) { drag.accepted = drag.hasUrls || drag.hasText }
        onDropped: function (drop) {
            if (!frameLayer.view || !frameLayer.insertion)
                return
            const scene = pageDrop.mapToItem(null, drop.x, drop.y)
            const point = frameLayer.view.sceneToPage(Qt.point(scene.x, scene.y))
            const ok = frameLayer.insertion.acceptWebDrop(drop.hasUrls ? drop.urls : [],
                                                          drop.hasText ? drop.text : "",
                                                          drop.hasHtml ? drop.html : "", point)
            if (ok)
                drop.accept(Qt.CopyAction)
        }
    }
```

- [ ] **Étape 3 : le relais.** `PageArea.qml`, après `property var profile: null` : `property var insertion: null` et `property var editing: null` ; dans le `WebFrameLayer` : `insertion: area.insertion` et `editing: area.editing`. `Main.qml`, sur `PageArea { id: pageArea` : `insertion: appInsertion` et `editing: appEditing`.

- [ ] **Étape 4 : vérifier, format, suite complète, banc, commit.** Commande bornée, `clang-format --dry-run --Werror tests/tst_ui_webdrop.cpp tests/tst_ui_desktop.cpp`, suite complète et banc sous le jeton (propriétaire `cadre-web-webengine/T12`, commande complète du banc : `ACTCRAFT_QT_DIR=$(brew --prefix qt) CMAKE_BUILD_PARALLEL_LEVEL=2 scripts/bench.sh --rendu logiciel --hors-ecran --scenario court --taille 1280x800 > .tmp/bench.txt 2>&1`).

```bash
git add ui/qml/WebFrameLayer.qml ui/qml/PageArea.qml ui/qml/Main.qml tests/tst_ui_webdrop.cpp tests/tst_ui_desktop.cpp tests/ui.cmake
git commit -m "feat(ui): dépôt de texte, d'image et de fichier sur la page

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T13 — Copier la sélection du cadre
<a id="tache-t13"></a>

**Objectif.** `edit.copy` copie la sélection du cadre au focus ; sinon les objets de la page ; un champ de saisie garde ses touches (cadrage n° 37, E10, S1).

**Fichiers.** Modifier : `ui/qml/WebFrameLayer.qml`, `ui/qml/PageArea.qml`, `ui/qml/Main.qml:1175`, `tests/tst_ui_webengine.cpp`, `tests/tst_ui_shell.cpp`.

**Interfaces.**
- Consomme : T5 : `frames.itemAt(i).webView`, contrat `viewFocused`, `copySelection()`.
- Produit : `function WebFrameLayer.copyFromFocusedView()` (booléen), `function PageArea.copyFromWebFrame()` (booléen) ; `id: webFrameLayerItem` sur la couche.

**Test et commande bornée.**
```bash
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T13 --attente-secondes 3000 --commande "cmake --build build-mac -j2 && QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_ui_(webengine|shell)$' --output-on-failure --timeout 300 -V > .tmp/t13.txt 2>&1"
grep -E 'PASS   : .*(editCopyCopiesTheSelection|inputFieldKeepsPaste|editCopyWithoutFocusedFrameCopiesObjects)|tests passed|tests failed' .tmp/t13.txt
```

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, avec `build-mac` : `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1` (suite complète, lue par `tail -3 .tmp/test.txt`), sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T13 --attente-secondes 3000 --commande "<commande>"` ; une seule tâche qui construit à la fois (8 Go).
- Format : `~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror` sur les fichiers touchés.
- `tst_ui_linklauncher` (`htaRefused`) : rouge préexistant instable, pas une régression.
- Aucun nom repris d'un produit existant ; aucune ressource de Promethean.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** `actions/data/commands.json` ne change pas (aucune commande ni raccourci nouveaux). Ctrl+V : pas de changement de gestionnaire.

**Critère « terminé ».** Tests nommés verts, suite complète, commit fait. Si le moteur ne reçoit pas le clavier hors écran après `requestActivate`, ne pas sauter le test : remonter à l'orchestrateur avec le journal.

**Étapes.**

- [ ] **Étape 1 : tests rouges.** `tests/tst_ui_webengine.cpp` (`#include <QClipboard>`) :

```cpp
    // S1, E10 : du texte sélectionné, vue au focus : edit.copy le met au presse-papiers.
    void editCopyCopiesTheSelection()
    {
        QGuiApplication::clipboard()->clear();
        presentationharness::MainWindow main;
        QVERIFY(main.window());
        QQuickItem* layer = main.item(QStringLiteral("webFrameLayer"));
        QObject* frames = layer->findChild<QObject*>(QStringLiteral("appWebFrames"));
        bool inserted = false;
        QVERIFY(QMetaObject::invokeMethod(
            frames, "insertFrame", Q_RETURN_ARG(bool, inserted),
            Q_ARG(QString, QStringLiteral("<body id=b>copie-moi<script>getSelection().selectAllChildren(b)</script>")),
            Q_ARG(QString, QString())));
        QTRY_COMPARE_WITH_TIMEOUT(layer->property("loadedViews").toInt(), 1, 60000);
        QQuickItem* web = main.item(QStringLiteral("webFrameEngine"));
        QVERIFY(web);
        main.window()->requestActivate();
        web->forceActiveFocus();
        QTRY_VERIFY(main.item(QStringLiteral("webFrameView"))->property("viewFocused").toBool());
        QVERIFY(main.run(QStringLiteral("edit.copy")));
        QTRY_COMPARE_WITH_TIMEOUT(QGuiApplication::clipboard()->text().trimmed(), QStringLiteral("copie-moi"), 10000);
    }

    // E10 : dans un champ de saisie, le moteur garde ses touches (ShortcutOverride) : Ctrl+V n'atteint pas la page.
    void inputFieldKeepsPaste()
    {
        QGuiApplication::clipboard()->setText(QStringLiteral("xyz"));
        presentationharness::MainWindow main;
        QVERIFY(main.window());
        QQuickItem* layer = main.item(QStringLiteral("webFrameLayer"));
        QObject* frames = layer->findChild<QObject*>(QStringLiteral("appWebFrames"));
        bool inserted = false;
        QVERIFY(QMetaObject::invokeMethod(
            frames, "insertFrame", Q_RETURN_ARG(bool, inserted),
            Q_ARG(QString, QStringLiteral("<input id=i autofocus oninput=\"document.title='VAL:'+value\">")),
            Q_ARG(QString, QString())));
        QTRY_COMPARE_WITH_TIMEOUT(layer->property("loadedViews").toInt(), 1, 60000);
        QQuickItem* web = main.item(QStringLiteral("webFrameEngine"));
        QVERIFY(web);
        QVERIFY(main.waitReady());
        const int objects = main.pageObjectCount();
        main.window()->requestActivate();
        web->forceActiveFocus();
        QTest::qWait(300);
        QTest::keyClick(main.window(), Qt::Key_V, Qt::ControlModifier);
        QTRY_COMPARE_WITH_TIMEOUT(web->property("title").toString(), QStringLiteral("VAL:xyz"), 10000);
        QCOMPARE(main.pageObjectCount(), objects);
    }
```
`tests/tst_ui_shell.cpp`, à côté de `convertEntriesOnlyForAnnotations` :

```cpp
    // E10 : sans vue web au focus, Ctrl+C copie toujours les objets.
    void editCopyWithoutFocusedFrameCopiesObjects()
    {
        Shell s;
        QVERIFY(start(s));
        QGuiApplication::clipboard()->clear();
        QVERIFY(run(s, QStringLiteral("pen.width"), 8));
        const QPoint a = s.view->pageToScene(QPointF(300, 400)).toPoint();
        const QPoint b = s.view->pageToScene(QPointF(700, 400)).toPoint();
        QTest::mousePress(s.window, Qt::LeftButton, Qt::NoModifier, a);
        for (int i = 1; i <= 20; ++i)
            QTest::mouseMove(s.window, a + (b - a) * i / 20);
        QTest::mouseRelease(s.window, Qt::LeftButton, Qt::NoModifier, b);
        QTRY_COMPARE_WITH_TIMEOUT(s.fixed->objectCount(), 1, 3000);
        QVERIFY(run(s, QStringLiteral("tool.select")));
        QVERIFY(run(s, QStringLiteral("edit.select-all")));
        QVERIFY(run(s, QStringLiteral("edit.copy")));
        QVERIFY(QGuiApplication::clipboard()->mimeData()->hasFormat(formats::kObjectsMimeType));
    }
```
(`#include <QClipboard>`, `#include <actcraft/formats/clipboard.h>`.)

- [ ] **Étape 2 : le code.** `WebFrameLayer.qml`, après `finishSnapshot` :

```qml
    // edit.copy (n° 37) : vrai si une vue web au focus a pris la copie.
    function copyFromFocusedView() {
        for (let i = 0; i < frames.count; ++i) {
            const delegate = frames.itemAt(i)
            const webView = delegate ? delegate.webView : null
            if (webView && webView.viewFocused) {
                webView.copySelection()
                return true
            }
        }
        return false
    }
```
`PageArea.qml` : `id: webFrameLayerItem` sur `WebFrameLayer` et, à côté des autres fonctions :

```qml
    function copyFromWebFrame() { return webFrameLayerItem.copyFromFocusedView() }
```
`Main.qml:1175` :

```qml
        appCommands.setHandler("edit.copy", function () {
            if (!pageArea.copyFromWebFrame())
                appEditing.copy()
        })
```

- [ ] **Étape 3 : vérifier, format, suite complète, commit.**

```bash
git add ui/qml/WebFrameLayer.qml ui/qml/PageArea.qml ui/qml/Main.qml tests/tst_ui_webengine.cpp tests/tst_ui_shell.cpp
git commit -m "feat(ui): edit.copy copie la sélection du cadre web au focus

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T16 — Paquet Mac et essai de l'image
<a id="tache-t16"></a>

**Objectif.** Le bundle livre Qt WebEngine ; `essai-dmg.sh` ouvre un vrai cadre depuis l'image disque et le prouve (cadrage n° 34, DA4, S4).

**Fichiers.** Modifier : `scripts/paquet-mac.sh:43-46,58,99-125`, `scripts/essai-dmg.sh:5,80-90`, `app/startup.h`, `app/startup.cpp`, `tests/tst_app_open.cpp`.

**Interfaces.**
- Consomme : T1 : `webFrameLayer`, `appWebFrames`, journal `ACTCRAFT CADRE WEB CHARGE` ; T15 : modules autorisés.
- Produit : `QString StartupOptions::webFrameProbe;` ; option `--essai-cadre-web <adresse ou fichier>` ; ligne `CADRE WEB OK` de `essai-dmg.sh --lancer`.

**Test et commande bornée.**
```bash
bash -n scripts/paquet-mac.sh && bash -n scripts/essai-dmg.sh && shellcheck -S warning scripts/paquet-mac.sh scripts/essai-dmg.sh
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T16 --attente-secondes 3000 --commande "cmake --build build-mac -j2 && QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_app_open$' --output-on-failure --timeout 300 -V > .tmp/t16.txt 2>&1"
grep -E 'PASS   : .*parsesWebFrameProbe|tests passed|tests failed' .tmp/t16.txt
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T16 --attente-secondes 3000 --duree-max-minutes 180 --commande "CMAKE_BUILD_PARALLEL_LEVEL=2 scripts/paquet-mac.sh > .tmp/paquet.txt 2>&1"
tail -3 .tmp/paquet.txt   # attendu en dernière ligne : DMG OK : <chemin>
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T16 --attente-secondes 3000 --commande "scripts/essai-dmg.sh --lancer > .tmp/essai-dmg.txt 2>&1"
tail -4 .tmp/essai-dmg.txt   # attendu : CADRE WEB OK, LANCEMENT OK, ESSAI DMG OK
```
L'essai ouvre une fenêtre 20 s sur l'écran du Mac ; aucune souris n'est pilotée.

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T16 --attente-secondes 3000 --commande "<commande>"` ; une seule tâche qui construit à la fois (8 Go).
- Format : `~/.local/venvs/clang-format/bin/clang-format --dry-run --Werror` sur `app/startup.h`, `app/startup.cpp`, `tests/tst_app_open.cpp`.
- `.github/workflows/release.yml` n'est jamais modifié (O98). Aucune machine GitHub, aucun job sur le PC, pas de release ni de tag.
- Ce qui exige Windows part en « Restes pour le PC ».
- Aucun nom repris d'un produit existant ; aucune ressource de Promethean.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** Ne rien publier : l'image reste sous `build/paquet/`. Ne pas affaiblir les gardes finales (`/opt/homebrew`, `@rpath` sans cible, `verifier-licences-mac.sh`, `codesign --verify --strict`).

**Patron à ne pas reprendre.** Se fier à macdeployqt pour WebEngine (`scripts/paquet-mac.sh:61-65`) : il ne résout pas le `@rpath` des greffons Homebrew, ne copie ni `Helpers` ni `Resources` au complet. Et le retrait « greffon WebEngine, framework plus référencé » des lignes 99-125, à renverser : il faut l'inverse.

**Critère « terminé ».** `DMG OK`, `ESSAI DMG OK` avec `CADRE WEB OK` (page réelle dans le bundle monté), test `parsesWebFrameProbe` vert, commit fait.

**Étapes.**

- [ ] **Étape 1 : test rouge.** Dans `tests/tst_app_open.cpp`, à côté du test de `parseStartupOptions` (ligne 47) :

```cpp
    void parsesWebFrameProbe()
    {
        const StartupOptions options = parseStartupOptions(
            {QStringLiteral("actcraft"), QStringLiteral("--essai-cadre-web"), QStringLiteral("/tmp/page.html")});
        QCOMPARE(options.webFrameProbe, QStringLiteral("/tmp/page.html"));
        QVERIFY(parseStartupOptions({QStringLiteral("actcraft")}).webFrameProbe.isEmpty());
    }
```
Rouge attendu : `no member named 'webFrameProbe'`.

- [ ] **Étape 2 : l'option.** `app/startup.h`, dans `StartupOptions` :

```cpp
    // `--essai-cadre-web <adresse ou fichier>` (essai de l'image disque, S4) : un cadre posé au démarrage.
    QString webFrameProbe;
```
Mettre à jour le commentaire de `parseStartupOptions`. `app/startup.cpp`, dans `parseStartupOptions` :

```cpp
    const qsizetype probe = args.indexOf(QStringLiteral("--essai-cadre-web"));
    if (probe >= 0 && probe + 1 < args.size()) {
        options.webFrameProbe = args.at(probe + 1);
    }
```
Dans la liste `withValue` d'`openFileArgument`, ajouter `QStringLiteral("--essai-cadre-web")`. Dans `loadMainWindow`, avant le dernier `return window;` (ajouter `#include <QTimer>` et `#include <QQuickItem>`) :

```cpp
    if (window && !options.webFrameProbe.isEmpty()) {
        const QString address = options.webFrameProbe;
        QTimer::singleShot(1000, window, [window, address] {
            const std::function<QQuickItem*(QQuickItem*)> find = [&](QQuickItem* item) -> QQuickItem* {
                if (item->objectName() == QLatin1String("webFrameLayer"))
                    return item;
                for (QQuickItem* child : item->childItems()) {
                    if (QQuickItem* found = find(child))
                        return found;
                }
                return nullptr;
            };
            QQuickItem* layer = find(window->contentItem());
            QObject* frames = layer ? layer->findChild<QObject*>(QStringLiteral("appWebFrames")) : nullptr;
            bool ok = false;
            if (frames)
                QMetaObject::invokeMethod(frames, "insertFrame", Q_RETURN_ARG(bool, ok), Q_ARG(QString, QString()),
                                          Q_ARG(QString, QUrl::fromUserInput(address).toString()));
            qInfo("ACTCRAFT ESSAI CADRE WEB %s", ok ? "POSE" : "ECHEC");
        });
    }
```
(`#include <functional>`.) Pour un chemin absolu, `QUrl::fromUserInput` rend une URL `file:` ; `insertFrame` l'accepte (gardes coupées, le défaut).

- [ ] **Étape 3 : `paquet-mac.sh`, message.** Ligne 45 : `echec "$option_cmake n'est pas ON dans $build/CMakeCache.txt (Qt Multimedia ou Qt WebEngine manquant)"`.

- [ ] **Étape 4 : `paquet-mac.sh`, copie.** Juste après la vérification du journal de macdeployqt (après la ligne 68 `[ -z "$erreur" ] || …`) :

```bash
# Qt WebEngine (cadrage DA4) : le greffon référence @rpath/QtWebEngine…, que macdeployqt ne résout pas chez Homebrew, et
# le processus auxiliaire exige Helpers/ et Resources/ complets : frameworks copiés entiers, en-têtes retirés.
copier_framework() { # <formule Homebrew> <nom du framework>
  local source="$qt/lib/$2.framework"
  [ -d "$source" ] || source="$(brew --prefix "$1" 2>/dev/null)/lib/$2.framework"
  [ -d "$source" ] || echec "$2.framework introuvable (ni $qt/lib, ni formule $1)"
  rm -rf "$app/Contents/Frameworks/$2.framework"
  cp -R "$source" "$app/Contents/Frameworks/" || echec "copie de $2.framework"
  chmod -R u+w "$app/Contents/Frameworks/$2.framework"
  rm -rf "$app/Contents/Frameworks/$2.framework/Headers" "$app/Contents/Frameworks/$2.framework/Versions/A/Headers"
}
copier_framework qtwebengine QtWebEngineCore
copier_framework qtwebengine QtWebEngineQuick
copier_framework qtwebengine QtWebEngineQuickDelegatesQml
copier_framework qtwebchannel QtWebChannel
copier_framework qtwebchannel QtWebChannelQuick
copier_framework qtpositioning QtPositioning
```

- [ ] **Étape 5 : `paquet-mac.sh`, Qt WebView retiré.** Dans la boucle `for module in … ; do` de la ligne 77, ajouter `QtWebView` à la liste (`ui/qml/WebFrameSystemView.qml`, lu par `-qmldir`, ferait livrer ce module sur le Mac). Après la ligne 85 :

```bash
rm -rf "$app/Contents/PlugIns/webview" # Qt WebView : Android seulement (cadrage n° 8)
```

- [ ] **Étape 6 : `paquet-mac.sh`, références.** Après la boucle de réécriture `@rpath` (après la ligne 97), remplacer le bloc des lignes 99-104 par :

```bash
# Chemins absolus de Homebrew dans les frameworks copiés à la main (QtWebEngineProcess, QtWebEngineCore, greffons) :
# même forme que les autres. L'auxiliaire tourne ailleurs : @executable_path/../Frameworks y désigne un dossier qui
# n'existe pas ; un lien l'y fait pointer vers les frameworks du bundle (DA4).
find "$app" -type f -exec otool -L {} + 2>/dev/null |
  awk '/^[^[:space:]].*:$/ { f = substr($0, 1, length($0) - 1); next }
       /^[[:space:]]+\/opt\/homebrew\/opt\/[^\/]+\/lib\/[^\/]+\.framework\// { print f "\t" $1 }' |
  while IFS=$'\t' read -r fichier ref; do
    cible="${ref#*/lib/}"
    [ -e "$app/Contents/Frameworks/$cible" ] || continue
    install_name_tool -change "$ref" "@executable_path/../Frameworks/$cible" "$fichier" 2>/dev/null &&
      echo "Réécrit : ${fichier#"$app"/} → Frameworks/${cible%%/*}"
  done
auxiliaire="$app/Contents/Frameworks/QtWebEngineCore.framework/Versions/A/Helpers/QtWebEngineProcess.app"
[ -x "$auxiliaire/Contents/MacOS/QtWebEngineProcess" ] || echec "QtWebEngineProcess absent du bundle"
ln -s ../../../../../.. "$auxiliaire/Contents/Frameworks" || echec "lien Frameworks de l'auxiliaire"
```

- [ ] **Étape 7 : `paquet-mac.sh`, garde.** Après la boucle `while :; do … done` de la ligne 125 :

```bash
# La boucle ci-dessus retire ce que plus rien ne référence : le moteur doit y avoir survécu.
for fw in QtWebEngineCore QtWebEngineQuick QtWebEngineQuickDelegatesQml QtWebChannel QtWebChannelQuick QtPositioning; do
  [ -d "$app/Contents/Frameworks/$fw.framework" ] || echec "$fw.framework retiré du bundle"
done
```
Après la ligne 163 (`cp -R LICENSES …`) :

```bash
cp "$(brew --prefix qtwebengine 2>/dev/null)/LICENSE.Chromium" "$app/Contents/Resources/LICENSE.Chromium" ||
  echec "LICENSE.Chromium absent de la formule qtwebengine"
```

- [ ] **Étape 8 : repli si la signature refuse le lien.** Lancer `scripts/paquet-mac.sh --sans-construire` si la construction Release existe. Si `codesign --verify --deep --strict` échoue sur le lien (`invalid destination for symbolic link`), appliquer la réécriture prévue par DA4, à la place du `ln -s` de l'étape 6 :

```bash
# Réécriture en @rpath (DA4, lien refusé par la signature stricte) : le même nom se résout depuis l'exécutable
# principal et depuis l'auxiliaire.
find "$app/Contents/Frameworks" "$app/Contents/PlugIns" "$app/Contents/Resources/qml" -type f -exec otool -L {} + 2>/dev/null |
  awk '/^[^[:space:]].*:$/ { f = substr($0, 1, length($0) - 1); next }
       /^[[:space:]]+@executable_path\/\.\.\/Frameworks\// { print f "\t" $1 }' |
  while IFS=$'\t' read -r fichier ref; do
    install_name_tool -change "$ref" "@rpath/${ref#@executable_path/../Frameworks/}" "$fichier" 2>/dev/null
  done
install_name_tool -add_rpath @executable_path/../Frameworks "$app/Contents/MacOS/actcraft" 2>/dev/null
install_name_tool -add_rpath @executable_path/../../../../../../.. "$auxiliaire/Contents/MacOS/QtWebEngineProcess" 2>/dev/null
```
Noter dans le rapport de tâche laquelle des deux voies a été retenue.

- [ ] **Étape 9 : `essai-dmg.sh`.** Ligne 5 : ajouter « ; `--lancer` ouvre aussi un vrai cadre web depuis l'image ». Remplacer le bloc `if [ "$lancer" = 1 ]; then … fi` (lignes 80-90) :

```bash
if [ "$lancer" = 1 ]; then
  echo "Fenêtre d'ActCraft ouverte 20 s sur l'écran du Mac (un vrai cadre web)."
  page="$(pwd -P)/.tmp/essai-cadre.html"
  printf '%s\n' '<title>ESSAI</title><script>console.warn("ACTCRAFT JS OK")</script><p>Cadre chargé.</p>' > "$page"
  QT_FORCE_STDERR_LOGGING=1 DYLD_PRINT_LIBRARIES=1 "$exe" --essai-cadre-web "$page" --quitter-apres 20000 > /dev/null 2> .tmp/essai-dmg-dyld-lancement.txt ||
    echec "lancement : code $? (.tmp/essai-dmg-dyld-lancement.txt)"
  chemins_dyld .tmp/essai-dmg-dyld-lancement.txt > .tmp/essai-dmg-chemins-lancement.txt
  grep -qF "$app/Contents/PlugIns/platforms/libqcocoa.dylib" .tmp/essai-dmg-chemins-lancement.txt ||
    echec "greffon cocoa non chargé depuis le bundle (.tmp/essai-dmg-dyld-lancement.txt)"
  grep -qF "$app/Contents/Frameworks/QtWebEngineCore.framework/" .tmp/essai-dmg-chemins-lancement.txt ||
    echec "QtWebEngineCore non chargé depuis le bundle (.tmp/essai-dmg-dyld-lancement.txt)"
  grep -q 'ACTCRAFT CADRE WEB CHARGE' .tmp/essai-dmg-dyld-lancement.txt ||
    echec "le cadre web n'a pas fini de charger (.tmp/essai-dmg-dyld-lancement.txt)"
  grep -q 'ACTCRAFT JS OK' .tmp/essai-dmg-dyld-lancement.txt ||
    echec "le script de la page n'a pas tourné (.tmp/essai-dmg-dyld-lancement.txt)"
  echo "CADRE WEB OK"
  hors_systeme .tmp/essai-dmg-chemins-lancement.txt .tmp/essai-dmg-refuses-lancement.txt ||
    echec "greffons ou bibliothèques chargés hors du bundle : $(head -3 .tmp/essai-dmg-refuses-lancement.txt | tr '\n' ';') (liste : .tmp/essai-dmg-refuses-lancement.txt)"
  echo "LANCEMENT OK"
fi
```

- [ ] **Étape 10 : vérifier.** Commandes bornées. `DMG OK` puis `CADRE WEB OK`, `LANCEMENT OK`, `ESSAI DMG OK`. Format des trois fichiers C++, suite complète sous le jeton.

- [ ] **Étape 11 : commit.**

```bash
git add scripts/paquet-mac.sh scripts/essai-dmg.sh app/startup.h app/startup.cpp tests/tst_app_open.cpp
git commit -m "feat(paquet): Qt WebEngine dans le paquet Mac, vrai cadre à l'essai de l'image

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T17 — Modules de la CI et APK sans WebEngine
<a id="tache-t17"></a>

**Objectif.** Les exécuteurs de bureau installent WebEngine ; Android construit sans lui et embarque le greffon `QtWebView` (cadrage carte CI, S5, n° 2).

**Fichiers.** Modifier : `.github/workflows/ci.yml:172,197,221,335,324`, `.github/workflows/preuves-windows.yml:92`.

**Interfaces.** Consomme : T1 : détection CMake, `WebFrameSystemView.qml` déclaré sous Android ; `scripts/ci/outils-android-mac.sh`, `scripts/paquet-android.ps1`. Produit : rien.

**Test et commande bornée.** Dans le worktree isolé :
```bash
ruby -ryaml -e "ARGV.each { |f| YAML.load_file(f) }; puts 'YAML OK'" .github/workflows/ci.yml .github/workflows/preuves-windows.yml
git diff --name-only HEAD | grep -c 'release.yml'   # attendu : 0
export JAVA_HOME="$(ls -d ~/actions-runner-actcraft/_work/_tool/Java_Temurin-Hotspot_jdk/21*/arm64/Contents/Home | tail -1)" QT_VERSION=6.12.0
mkdir -p .tmp && GITHUB_ENV="$PWD/.tmp/android-env" GITHUB_PATH="$PWD/.tmp/android-path" ./scripts/ci/outils-android-mac.sh arm64_v8a x86_64
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T17 --attente-secondes 3000 --duree-max-minutes 180 --commande "set -a; . .tmp/android-env; set +a; PATH=\"\$(tr '\n' ':' < .tmp/android-path)\$PATH\" ~/actions-runner-actcraft/_cache/pwsh-7.5.4/pwsh -NoProfile -File ./scripts/paquet-android.ps1 > .tmp/apk.txt 2>&1"
tail -1 .tmp/apk.txt   # attendu : APK OK
unzip -l build/paquet/ActCraft-*-arm64-v8a.apk | grep -c 'qml/QtWebView'   # attendu : ≥ 1
unzip -l build/paquet/ActCraft-*-arm64-v8a.apk | grep -c 'WebEngine'       # attendu : 0
```
JDK 21 et `pwsh` sont déjà dans le cache du runner (relevé du 2026-11-09) ; rien n'est à installer hors de ce cache. Si la chaîne manque malgré tout : S5 passe au job `android` du Mac mini à la PR, à noter dans le rapport, sans lancer de workflow.

**Contraintes (recopiées).**
- `.github/workflows/release.yml` n'est jamais modifié (O98) : la liste de ce qu'il devra recevoir est dans « Sortie pour O98 » du plan.
- Aucune machine GitHub, aucun job sur le PC, pas de release ni de tag : aucun `gh workflow run` depuis cette tâche ; ce qui exige Windows ou un appareil Android part en « Restes pour le PC » ou R2.
- Une seule tâche qui construit à la fois (8 Go) : toute construction sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T17 --attente-secondes 3000 --commande "<commande>"`.
- Aucun nom repris d'un produit existant ; aucune ressource de Promethean.
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Pas de mise à jour de `graphify-out/`.

**Interdits.** Ne pas modifier `ci.yml:476-478` (job Android : pas de WebEngine), ni `scripts/ci/outils-android-mac.sh`. Conflits attendus avec les PR #21 et #22 : à laisser à la clôture, sans contact avec O84.

**Critère « terminé ».** `YAML OK`, `release.yml` non touché, `APK OK` et les deux `unzip` conformes, commit fait.

**Étapes.**

- [ ] **Étape 1 : modules.** Dans `.github/workflows/ci.yml`, lignes 172 et 324, et `.github/workflows/preuves-windows.yml:92` : ajouter en fin de liste `-m …` les mots `qtwebengine qtwebchannel qtpositioning` (garder `qtwebview` : la ligne 172 sert aussi l'hôte des jobs Android). Ligne 197 (repli `aqt` du Mac) : `-m qtshadertools qtimageformats qtpdf qtwebengine qtwebchannel qtpositioning`.

- [ ] **Étape 2 : contrôle au PC, en avertissement.** Lignes 221 et 335 : après la boucle `foreach ($module in 'Qt6', 'Qt6ShaderTools', 'Qt6Pdf') { … }`, ajouter sur la ligne suivante (mêmes retraits) :

```powershell
          if (-not (Test-Path "$qt\lib\cmake\Qt6WebEngineQuick")) { "::warning::Qt WebEngine absent de $qt : construction sans cadre web (Reste PC 1)" }
```

- [ ] **Étape 3 : vérifier et construire l'APK.** Commandes bornées.

- [ ] **Étape 4 : commit.**

```bash
git add .github/workflows/ci.yml .github/workflows/preuves-windows.yml
git commit -m "ci: modules de Qt WebEngine pour les exécuteurs de bureau

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```

---

### Tâche T18 — Mesures avant/après et preuve finale
<a id="tache-t18"></a>

**Objectif.** Prouver S4 sur le code final : `.dmg` avant/après, provenance de Qt, H.264 réellement décodé, aucun processus Chromium sans cadre, mémoire, coût du premier cadre, relevés écrits dans `docs/mesures/` (cadrage n° 36, E5, H2, H12, H15).

**Fichiers.** Créer : `docs/mesures/2026-11-09-cadre-web-webengine.md`.

**Interfaces.** Consomme : T16 : `scripts/paquet-mac.sh`, `scripts/essai-dmg.sh`, `--essai-cadre-web` ; T14 : `BOUCLE max=` du joueur. Produit : le document de mesures, que l'orchestrateur reporte au fichier d'état.

**Test et commande bornée.** Les étapes ci-dessous. Motifs : `DMG OK` (avant, après), `ESSAI DMG OK`, `ACTCRAFT H264 w=320 h=240`, `SCENARIO OK`, `100% tests passed` ou le seul `tst_ui_linklauncher` en échec.

**Contraintes (recopiées).**
- Preuves sur ce Mac uniquement, avec `build-mac` : `cmake --build build-mac -j2` et `QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac --output-on-failure -j2 --timeout 120 > .tmp/test.txt 2>&1`, sous `node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T18 --attente-secondes 3000 --commande "<commande>"`. Une seule tâche qui construit à la fois (8 Go de RAM).
- `tst_ui_linklauncher` (`htaRefused`) : rouge préexistant instable, pas une régression.
- `.github/workflows/release.yml` n'est jamais modifié (O98).
- Aucune machine GitHub, aucun job sur le PC, pas de release ni de tag ; ce qui exige Windows part en « Restes pour le PC ».
- JSON du banc hors écran : résultat et non porte (`scripts/bench.sh --rendu logiciel --hors-ecran`).
- Commits en français, chemins explicites, jamais `git add -A`, `.` ni `commit -a`, terminés par `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. `graphify-out/` n'est mis à jour qu'à la clôture, par l'orchestrateur.

**Interdits.** Ne pas publier l'image. Ne pas toucher au fichier d'état (l'orchestrateur le tient).

**Critère « terminé ».** Le document répond à chaque point de S4, avec des chiffres relevés ici. Tout repli (H12) y est écrit comme tel.

**Étapes.**

- [ ] **Étape 1 : image « après » et provenance.**

```bash
mkdir -p .tmp
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T18 --attente-secondes 3000 --duree-max-minutes 180 --commande "CMAKE_BUILD_PARALLEL_LEVEL=2 scripts/paquet-mac.sh > .tmp/paquet-apres.txt 2>&1"
tail -3 .tmp/paquet-apres.txt
ls -l build/paquet/ActCraft-*-macos-arm64.dmg
du -sm build/paquet/macos/ActCraft.app
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T18 --attente-secondes 3000 --commande "scripts/essai-dmg.sh --lancer > .tmp/essai-dmg.txt 2>&1"
tail -4 .tmp/essai-dmg.txt
brew info --json=v2 qtwebengine | python3 -c "import json,sys; f=json.load(sys.stdin)['formulae'][0]; print('Homebrew', f['name'], f['versions']['stable'], 'révision', f['revision'], f['installed'][0]['version'])"
```

- [ ] **Étape 2 : image « avant ».**

```bash
git worktree add ../cadre-web-webengine--avant 76315dbc
cd ../cadre-web-webengine--avant
mkdir -p .tmp
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T18 --attente-secondes 3000 --duree-max-minutes 180 --commande "CMAKE_BUILD_PARALLEL_LEVEL=2 scripts/paquet-mac.sh > .tmp/paquet-avant.txt 2>&1"
ls -l build/paquet/ActCraft-*-macos-arm64.dmg; du -sm build/paquet/macos/ActCraft.app
cd - && git worktree remove --force ../cadre-web-webengine--avant
```
Repli (H12) si le jeton manque (`code 3`) ou la construction échoue : 61 Mo (64 328 601 octets) et bundle de 143 Mo, relevés de `docs/verifications/2026-11-07-lot11-paquet-macos.md`, écrits avec la mention « relevé du 2026-11-07, non rejoué ».

- [ ] **Étape 3 : H.264.** Monter l'image, lancer un cadre sur une vidéo de synthèse :

```bash
ffmpeg -y -loglevel error -f lavfi -i testsrc=size=320x240:rate=25 -t 3 -c:v libx264 -pix_fmt yuv420p .tmp/h264.mp4
cat > .tmp/h264.html <<'HTML'
<video id=v src="h264.mp4" autoplay muted></video>
<script>setTimeout(function(){console.warn("ACTCRAFT H264 w="+v.videoWidth+" h="+v.videoHeight+" t="+v.currentTime)},2500)</script>
HTML
point="$(mktemp -d /tmp/actcraft-t18.XXXXXX)"
hdiutil attach -nobrowse -readonly -noautoopen -mountpoint "$point" build/paquet/ActCraft-*-macos-arm64.dmg > /dev/null
QT_FORCE_STDERR_LOGGING=1 "$point/ActCraft.app/Contents/MacOS/actcraft" --essai-cadre-web "$PWD/.tmp/h264.html" --quitter-apres 12000 2> .tmp/h264.txt
grep -E 'ACTCRAFT H264' .tmp/h264.txt
```
Attendu : `w=320 h=240`, `t` non nul.

- [ ] **Étape 4 : processus et mémoire**, depuis la même image montée :

```bash
"$point/ActCraft.app/Contents/MacOS/actcraft" --quitter-apres 8000 > /dev/null 2>&1 & sleep 6
echo "sans cadre : $(ps -axo command | grep -c '[Q]tWebEngineProcess') processus du moteur ; $(ps -axo rss,command | grep "$point" | grep -v grep | awk '{s+=$1} END {print int(s/1024)}') Mo"
wait
"$point/ActCraft.app/Contents/MacOS/actcraft" --essai-cadre-web "$PWD/.tmp/essai-cadre.html" --quitter-apres 15000 > /dev/null 2>&1 & sleep 12
echo "avec un cadre : $(ps -axo rss,command | grep "$point" | grep -v grep | awk '{s+=$1} END {print int(s/1024)}') Mo"
wait
hdiutil detach "$point" -quiet; rmdir "$point"
```

- [ ] **Étape 5 : coût du premier cadre et banc** (`release`) :

```bash
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T18 --attente-secondes 3000 --duree-max-minutes 180 --commande "ACTCRAFT_QT_DIR=$(brew --prefix qt) CMAKE_BUILD_PARALLEL_LEVEL=2 scripts/bench.sh --rendu logiciel --hors-ecran --scenario court --taille 1280x800 > .tmp/bench.txt 2>&1"
tail -1 .tmp/bench.txt
node ~/.claude/scripts/chaine/jeton.cjs --nom tests --proprietaire cadre-web-webengine/T18 --attente-secondes 3000 --commande "QT_QPA_PLATFORM=offscreen build/release/actcraft-captures --scenario bench/scenarios/cadre-web.json --rendu logiciel --taille 1280x800 --sortie .tmp/captures-release > .tmp/captures-release.txt 2>&1"
grep -E 'BOUCLE max=|HORS SEUIL|SCENARIO' .tmp/captures-release.txt
```

- [ ] **Étape 6 : suite complète.** `cmake --build build-mac -j2` puis la commande `ctest` des contraintes, sous le jeton ; `tail -3 .tmp/test.txt`. Tests à garder verts de la carte d'impact (`tst_ui_webpolicy`, `tst_ui_fileexchange`, `tst_ui_shell`, `tst_ui_settingsschema`, `tst_app_notices`, `tst_core_hittest`, `tst_core_model`, `tst_core_interactivity`, `tst_core_broadcast`, `tst_formats_interactivity`, `tst_formats_nativejson`, `tst_formats_clipboard`, `tst_render_painted`, `tst_actions_layout`, `tst_actions_lockable`, `tst_ui_secondpalette`, `tst_captures_cadre_web`) : `grep -E 'tst_(ui_webpolicy|ui_fileexchange|ui_shell|ui_settingsschema|app_notices|core_hittest|core_model|core_interactivity|core_broadcast|formats_interactivity|formats_nativejson|formats_clipboard|render_painted|actions_layout|actions_lockable|ui_secondpalette|captures_cadre_web) ' .tmp/test.txt`, tous `Passed`.

- [ ] **Étape 7 : le document.** `docs/mesures/2026-11-09-cadre-web-webengine.md`, avec les valeurs relevées à la place des `<…>` :

```markdown
# Mesures — cadre-web-webengine (2026-11-09, Mac mini Apple Silicon, 8 Go)

## Provenance de Qt
<formule, version et révision relevées> ; Qt WebEngine fondé sur Chromium <version>. Voie `aqt` non essayée (H2).

## Poids (S4, E5)
| | Avant (76315dbc) | Après |
|---|---|---|
| `.dmg` | <octets> (<Mo>) <mention si repli H12> | <octets> (<Mo>) |
| `ActCraft.app` | <Mo> | <Mo> |

## H.264 (H2)
`ACTCRAFT H264 w=<w> h=<h> t=<t>` : vidéo décodée dans le cadre, depuis l'image montée. Redistribution : décision R3, non prise.

## Sans cadre / avec un cadre (DA2, n° 36)
- Sans cadre web : <n> processus du moteur, <Mo> résidents (somme).
- Avec un cadre : <Mo> résidents (somme, tous processus du bundle).

## Coût du premier cadre (n° 36, H15), `release`, rendu logiciel, hors écran
Plus grand trou de la boucle d'événements : <BOUCLE max= n ms>. Valeur sur Dell 3510 : Reste PC 8.

## Banc court hors écran
Verdict : <BUDGET TENU ou DEPASSE>, résultat et non porte. JSON : <chemin>.

## Essai de l'image
`DMG OK`, `ESSAI DMG OK`, `CADRE WEB OK`, aucune bibliothèque hors du bundle.

## À ajouter à `release.yml` par O98
`qtwebengine qtwebchannel qtpositioning` lignes 194 (Windows), 289 (`aqt` Mac), 314 (`modules=` du repli Mac mini). Commentaires à relire : lignes 273, 294, 312, 339. Lignes 415-416 : rien.
```

- [ ] **Étape 8 : commit.**

```bash
git add docs/mesures/2026-11-09-cadre-web-webengine.md
git commit -m "docs: mesures du chantier cadre-web-webengine

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
```
