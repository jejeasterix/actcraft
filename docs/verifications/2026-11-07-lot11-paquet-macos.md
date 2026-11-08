# Lot 11, L11-T29m : paquet macOS non signé (image disque) et son essai

Relevé du 2026-11-08 sur le Mac mini Apple Silicon (macOS 27.2), Qt 6.12.0 de Homebrew, branche
`feat/livraison-complete-l11--L11-T29m`. Amendement 3 du plan 11b, volet 1 ; H11-19 (le `.dmg` dans la release, à revoir
par Jérôme) ; H11-18 (chemin aqt non prouvé).

## Image disque

- Fichier : `build/paquet/ActCraft-0.6.0-macos-arm64.dmg` (local, non commité, jamais publié).
- Version : celle du projet, `0.6.0` (`CMakeLists.txt:4`). Arbitrage H11-O10 de l'orchestrateur, d'après H8-19 :
  `project(VERSION)` ne passe pas à 1.0.0 dans cette branche ; partout où le plan attend « 1.0.0 », lire 0.6.0.
- Taille : 61 Mo (64 328 601 octets) ; bundle `ActCraft.app` déployé : 143 Mo.
- SHA-256 : `0d5594a82114a1cc91dfc4eb0f923856e804d88e09bd76099324c5a588d76cfb`.
- Signature ad hoc (`codesign -s -`), ni Developer ID ni notarisation. Contenu : `ActCraft.app`, lien `Applications`,
  `LICENSE.txt`, `THIRD_PARTY_NOTICES.md`, `LISEZ-MOI.txt` (premier lancement sous Gatekeeper).

## Lignes relevées

```
scripts/paquet-mac.sh --sans-construire
  LICENCES OK
  Taille :  61M
  SHA-256 : 0d5594a82114a1cc91dfc4eb0f923856e804d88e09bd76099324c5a588d76cfb
  DMG OK : <wt>/build/paquet/ActCraft-0.6.0-macos-arm64.dmg
scripts/essai-dmg.sh --lancer
  VERSION OK : ActCraft 0.6.0
  Fenêtre d'ActCraft ouverte 3 s sur l'écran du Mac.
  LANCEMENT OK
  ESSAI DMG OK
```

- Au lancement, 77 des 1 244 images chargées viennent du bundle monté (`grep -c '/ActCraft.app/'`), dont
  `PlugIns/platforms/libqcocoa.dylib` ; les autres viennent du système. `.tmp/essai-dmg-refuses-version.txt` et
  `.tmp/essai-dmg-refuses-lancement.txt` : vides (0 octet). Aucun module QML signalé manquant dans le journal.
- Contre-épreuves sur le `.app` non déployé (`build/macos-paquet/app/actcraft.app`) : `otool` voit **10** chemins
  `/opt/homebrew` ou `/usr/local`, `dyld` en charge **35** depuis `/opt/homebrew` à `--version`. Ce bundle incomplet
  rend pourtant `ActCraft 0.6.0` à `--version` : un essai réduit au lancement sur le Mac qui construit l'accepterait.
- Rouge vu d'abord : `ESSAI DMG EN ECHEC : image absente : build/paquet/ActCraft-0.6.0-macos-arm64.dmg`.
- Mutation : ligne `libglib-2.0.*.dylib` retirée de `packaging/autres-autorises.txt` →
  `LICENCES EN ECHEC : libglib-2.0.0.dylib` ; remise par copie, `cmp` muet, puis `LICENCES OK`.
- `tst_app_notices` : `100% tests passed out of 1`. `YAML OK` (job `macos` de `release.yml`).
- Suite complète sur `build-mac` (Debug, sous le jeton) : `100% tests passed out of 278`, aucun test neuf ;
  `tst_app_notices` `Passed` ; tests sautés existants (`tst_flipchart_miniatures_samples`,
  `tst_captures_raccourcis_programmes`, `tst_captures_classe_protegee_complete`).

## Ce que fait le déploiement, au-delà du plan

Écarts constatés sur Qt 6.12.0 de Homebrew, chacun traité dans `scripts/paquet-mac.sh` et commenté sur place :

1. **macdeployqt copie en entier les dossiers d'import `QtQuick/` et `QtQml/`** : il y entraîne des modules
   qu'ActCraft n'importe pas, dont deux sous GPL seule. Retirés avec les greffons que désignent leurs liens :
   `QtQuick/VirtualKeyboard` (et `platforminputcontexts/libqtvirtualkeyboardplugin.dylib`), `QtQuick/Timeline`,
   `QtQuick/Scene2D`, `QtQuick/Scene3D` (Qt 3D), `QtQml/StateMachine`. Un module oublié dans cette liste ne passerait
   pas : `verifier-licences-mac.sh` refuse tout framework absent de `qt-autorises.txt`.
2. **`Cannot resolve rpath "@rpath/…"`** : Homebrew range chaque module dans sa formule et le rpath d'un greffon y est
   relatif à sa place (`@loader_path/../../../../lib`), que macdeployqt évalue depuis le bundle (`-libpath` essayé :
   sans effet). Ces lignes ne sont plus un verdict à elles seules : chaque référence `@rpath` restée est réécrite en
   `@executable_path/../Frameworks/…` si son framework est dans le bundle (QtSvg pour `libqsvg` et `libqsvgicon`),
   et toute référence `@rpath` sans cible est refusée (`DMG EN ECHEC : @rpath sans cible dans le bundle`). Toute autre
   ligne `ERROR` de macdeployqt reste fatale.
3. **Identifiants** : 44 frameworks gardaient leur identifiant (`LC_ID_DYLIB`) sur `/opt/homebrew` ; aucune dépendance
   n'y était (vérifié : seules les lignes d'identifiant). Réécrits sous la forme des autres.
4. **`qt.conf` absent** après macdeployqt (son journal ne le nomme pas) : écrit s'il manque, `[Paths]`,
   `Plugins = PlugIns`, `QmlImports = Resources/qml` (clé de Qt 6, présente parmi les chaînes de QtCore 6.12 ;
   `Qml2Imports` et `Imports` n'y sont pas), puis contrôlé.
5. **Chemin du bundle Release** : `build/macos-paquet/app/actcraft.app` (le plan écrivait `build/macos-paquet/actcraft.app`) ;
   `find -maxdepth 2` le trouve.

Retirés comme plus référencés après le retrait du greffon `libqtwebview_webengine.dylib` : `QtWebEngineQuick`,
`QtWebViewQuick`, `QtWebChannelQuick`, `QtWebEngineCore`, `QtPositioning`, `QtWebChannel`. Aucun `QtWebEngine*` ne
reste ; Qt WebView passe par le greffon `libqtwebview_darwin.dylib`.

## Frameworks Qt du bundle (56)

Tous sous `LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only`, liés dynamiquement (LGPL-3.0-only
retenue). 16 sont neufs dans `packaging/qt-autorises.txt` (licence lue dans la ligne SPDX de leurs en-têtes de Qt 6.12
Homebrew ; QtPdfQuick, sans ligne SPDX, dans le SBOM de qtwebengine 6.11.2) : Qt6DBus, Qt6PdfQuick,
Qt6QmlDesignSupport, Qt6QmlLocalStorage, Qt6QmlXmlListModel, Qt6QuickControls2IOS, Qt6QuickControls2IOSStyleImpl,
Qt6QuickControls2Macos, Qt6QuickControls2MacOSStyleImpl, Qt6QuickControls2Native, Qt6QuickParticles,
Qt6QuickShapesDesignHelpers, Qt6QuickVectorImage, Qt6QuickVectorImageGenerator, Qt6QuickVectorImageHelpers, Qt6Sql.
QtPdf et QtPdfQuick viennent de la formule qtwebengine 6.11.2, les autres de Qt 6.12.0.

## Bibliothèques non Qt du bundle (31 fichiers, 25 lignes de `autres-autorises.txt`)

| Bibliothèque | Formule Homebrew | Version | Licence SPDX |
|---|---|---|---|
| libb2.1 | libb2 | 0.98.1 | CC0-1.0 |
| libbrotlicommon.1, libbrotlidec.1 | brotli | 1.2.1 | MIT |
| libcrypto.4, libssl.4 | openssl@4 | 4.1.0 | Apache-2.0 |
| libdbus-1.3 | dbus | 1.16.2_1 | AFL-2.1 OR GPL-2.0-or-later (AFL-2.1 retenue) |
| libdouble-conversion.3 | double-conversion | 3.4.0 | BSD-3-Clause |
| libfreetype.6 | freetype | 2.15.0 | FTL |
| libglib-2.0.0, libgio-2.0.0, libgmodule-2.0.0, libgobject-2.0.0, libgthread-2.0.0 | glib | 2.90.1 | LGPL-2.1-or-later |
| libgraphite2.3 | graphite2 | 1.3.15 | MIT OR MPL-2.0 OR LGPL-2.1-or-later OR GPL-2.0-or-later (MIT retenue) |
| libharfbuzz.0 | harfbuzz | 15.0.0_1 | MIT |
| libicudata.79, libicui18n.79, libicuuc.79 | icu4c@79 | 79.1 | Unicode-3.0 |
| libintl.8 | gettext | 1.0 | LGPL-2.1-or-later (bibliothèque seule, en-tête `libintl.h` ; formule : `GPL-3.0-or-later AND LGPL-2.1-or-later`, outils compris) |
| libjpeg.8 | jpeg-turbo | 3.2.0 | IJG AND Zlib AND BSD-3-Clause |
| liblzma.5 | xz | 5.8.4 | 0BSD (bibliothèque seule, en-tête `lzma.h` ; formule : `0BSD AND GPL-2.0-or-later`, outils compris) |
| libmd4c.0 | md4c | 0.6.0 | MIT |
| libpcre2-8.0, libpcre2-16.0 | pcre2 | 10.49 | BSD-3-Clause |
| libpng16.16 | libpng | 1.6.59 | libpng-2.0 |
| libsharpyuv.0, libwebp.7, libwebpdemux.2, libwebpmux.3 | webp | 1.6.0 | BSD-3-Clause |
| libtiff.6 | libtiff | 4.7.2 | libtiff |
| libzstd.1 | zstd | 1.5.7_1 | BSD-3-Clause OR GPL-2.0-only (BSD-3-Clause retenue) |

Aucune sous GPL seule. Textes neufs sous `LICENSES/` (liste SPDX, dossier `text/`) : `AFL-2.1`, `GPL-2.0-or-later`
(même texte que `GPL-2.0-only`), `0BSD` ; leurs empreintes sont ajoutées à `tests/tst_app_notices.cpp`, comme le
demande la règle du test. Notices : section « Bibliothèques du paquet macOS » de `THIRD_PARTY_NOTICES.md`, modules Qt
dans « Modules livrés ». Licences relevées dans la formule (`brew info --json=v2`) et les en-têtes ; non validé par un
juriste (N1-8).

## Restes

- Pour Jérôme : premier lancement d'un `.dmg` **téléchargé** sur le MacBook Air M1 (Gatekeeper, `LISEZ-MOI.txt`),
  mesure de L11-T30 ; décision H11-19 (le `.dmg` dans la release).
- Chemin aqt non prouvé (H11-18) : seul le Qt de Homebrew est essayé ici ; essai à blanc de `release.yml` sur
  l'exécuteur que prendra le tag (L11-T29).
- PC : `NOMS OK`, contrôle des titres du corpus ; essai à blanc de `release.yml` (points 6 à 8 de L11-T29).
- `hdiutil create -volname … -format …` est annoncé déprécié par macOS 27 (avertissement, sans effet sur le résultat).
