# Lot 8, L8-T20 : modules Qt Multimedia et Qt WebView

## Accord H8-6

Jérôme, le 2026-10-21, à la question « installer Qt Multimedia (avec FFmpeg) et Qt WebView ? » : « H8-6 > ok ».
Pas de repli (L8-C1, L8-T21r… ne se font pas). Aucun module Qt WebEngine n'est ajouté.

## Fait sur le Mac (2026-11-06)

- `ci.yml` (5 lignes `aqt install-qt` : Windows, kit macOS `clang_64` de la matrice, Linux, Android arm64 et x86_64) et
  `release.yml` (3 lignes : Windows, Linux, Android arm64) : `-m … qtmultimedia qtwebview`. YAML valide (ruby).
  `release.yml` n'installe pas `android_x86_64` : inchangé. `preuves-windows.yml` installe aussi `qtmultimedia` et `qtwebview` (c28ababf).
- `CLAUDE.md` : ligne d'installation du poste.
- `packaging/qt-autorises.txt` : `Qt6Multimedia`, `Qt6MultimediaQuick`, `Qt6WebView`, `Qt6WebViewQuick`. Licence lue
  dans les en-têtes SPDX de Qt 6.12.0 (Homebrew) : `LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only`.
- `THIRD_PARTY_NOTICES.md` : section Qt Multimedia, FFmpeg (LGPL-2.1-or-later tel que livré par Qt) et Qt WebView.

## Relevés sur le Mac (Qt 6.12.0 Homebrew)

| Élément | Taille |
|---|---|
| `QtMultimedia.framework` | 2,7 Mo |
| `QtMultimediaQuick.framework` | 676 Ko |
| `QtWebView.framework` | 172 Ko |
| `QtWebViewQuick.framework` | 172 Ko |
| greffon `multimedia/libdarwinmediaplugin.dylib` | 468 Ko |
| greffons `webview/` (`darwin`, `webengine`) | 132 Ko, 104 Ko |

- Sous macOS, Qt Multimedia passe par AVFoundation (greffon `darwinmediaplugin`) : **pas de FFmpeg** sur ce poste.
- Le greffon `libqtwebview_webengine` existe dans Homebrew parce que `qtwebengine` y est installé ; il ne doit
  jamais être déployé (pas de WebEngine).
- Dépendances (`otool -L`) : QtCore, QtGui, QtNetwork, QtOpenGL, QtQml, QtQuick et leurs modules internes, déjà autorisés.
- Noms de modules `aqt` : `qtmultimedia`, `qtwebview` (usage de la documentation de l'installateur et du plan).
  `aqt` n'est pas installé sur le Mac : **à confirmer au PC** par `python -m aqt list-qt windows desktop --modules 6.12.0 win64_msvc2022_64`
  (et `list-qt all_os android --modules 6.12.0 android_arm64_v8a`).

## Restes pour le PC et la CI de la PR

- `Test-Path` de `lib/cmake/Qt6Multimedia` et `lib/cmake/Qt6WebView` des trois kits (Windows, `android_arm64_v8a`, `android_x86_64`).
- `pwsh -NoProfile -File ./scripts/verifier-licences.ps1` sans écart ; relever les noms des greffons FFmpeg et WebView
  déployés par `windeployqt` et l'APK, et la licence du greffon FFmpeg de Qt (SBOM `sbom/qtmultimedia-6.12.0.spdx`).
- Moteur de WebView sous Windows (WebView2) : à confirmer.
- Tailles de l'installateur et de l'APK, avant (tête de 8a) et après (construction avec `ACTCRAFT_MEDIA=ON`, L8-T21) : à mesurer.
- `TESTS OK` inchangé.
