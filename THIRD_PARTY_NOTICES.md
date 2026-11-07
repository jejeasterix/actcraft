# Avis concernant les composants tiers

ActCraft est distribué sous licence MIT (fichier `LICENSE`, Licence SPDX : `MIT`). Ses paquets (installateur
Windows, APK Android) contiennent aussi les composants tiers ci-dessous, chacun sous sa propre licence. Chaque
licence est citée par son identifiant SPDX (https://spdx.org/licenses/) ; son texte complet est dans le dossier
`LICENSES/`, sous le nom `LICENSES/<identifiant>.txt` (textes de la liste SPDX ; ceux dont l'identifiant commence par
`LicenseRef-` sont repris du projet Qt ou renvoient aux conditions de l'éditeur).

Ce document décrit ce que les paquets contiennent et sous quelles licences ; il n'a pas été validé par un juriste et
ne constitue pas un avis juridique.

## Qt 6.12

ActCraft utilise le framework Qt (The Qt Company Ltd. et autres contributeurs), version 6.12, sous la **GNU Lesser
General Public License version 3** (Licence SPDX : `LGPL-3.0-only`, texte dans `LICENSES/LGPL-3.0-only.txt`). Cette
licence complète la GNU General Public License version 3 (Licence SPDX : `GPL-3.0-only`, texte dans
`LICENSES/GPL-3.0-only.txt`). Chaque module de Qt est proposé par ses auteurs sous plusieurs licences ; ActCraft
retient la LGPL version 3.

- Qt est lié **dynamiquement** : bibliothèques `Qt6*.dll` à côté de `actcraft.exe` sous Windows, `libQt6*.so` dans
  l'APK sous Android. Vous pouvez remplacer ces bibliothèques par une version de Qt modifiée et compatible.
- Code source de Qt 6.12 : https://download.qt.io/official_releases/qt/6.12/ et https://code.qt.io/.
- Code source d'ActCraft : https://github.com/jejeasterix/actcraft.
- Aucun module disponible seulement sous GPL ou sous licence commerciale n'est livré : la liste des modules
  autorisés est `packaging/qt-autorises.txt`, contrôlée à chaque paquet par `scripts/verifier-licences.ps1`.
- Les greffons de Qt (plateforme, formats d'image, modules QML, multimédia, cadre web) suivent la licence de leur
  module.

### Modules livrés

Tous sous `LGPL-3.0-only`, liés dynamiquement.

- `Qt6Core` : noyau (types de base, fichiers, événements, fils d'exécution).
- `Qt6Gui` : fenêtres, images, polices, rendu graphique.
- `Qt6Network` : accès réseau (recherche de mises à jour, ressources en ligne).
- `Qt6OpenGL` : rendu OpenGL.
- `Qt6Svg` : images vectorielles SVG.
- `Qt6Qml` : moteur du langage QML.
- `Qt6QmlMeta` : métadonnées des types QML.
- `Qt6QmlModels` : modèles de données pour QML.
- `Qt6QmlWorkerScript` : scripts QML en tâche de fond.
- `Qt6Quick` : scène graphique de l'interface.
- `Qt6QuickLayouts` : mises en page de l'interface.
- `Qt6QuickShapes` : formes vectorielles de l'interface.
- `Qt6QuickEffects` : effets graphiques de l'interface.
- `Qt6QuickTemplates2` : modèles des contrôles de l'interface.
- `Qt6QuickControls2` : contrôles de l'interface (boutons, menus, listes).
- `Qt6QuickControls2Impl` : implémentation commune des contrôles.
- `Qt6QuickControls2Basic` : style de base des contrôles.
- `Qt6QuickControls2BasicStyleImpl` : implémentation du style de base.
- `Qt6QuickControls2Fusion` : style Fusion des contrôles.
- `Qt6QuickControls2FusionStyleImpl` : implémentation du style Fusion.
- `Qt6QuickControls2Material` : style Material des contrôles.
- `Qt6QuickControls2MaterialStyleImpl` : implémentation du style Material.
- `Qt6QuickControls2Universal` : style Universal des contrôles.
- `Qt6QuickControls2UniversalStyleImpl` : implémentation du style Universal.
- `Qt6QuickControls2Imagine` : style Imagine des contrôles.
- `Qt6QuickControls2ImagineStyleImpl` : implémentation du style Imagine.
- `Qt6QuickControls2FluentWinUI3` : style FluentWinUI3 des contrôles (tiré par les boîtes de dialogue).
- `Qt6QuickControls2FluentWinUI3StyleImpl` : implémentation du style FluentWinUI3.
- `Qt6QuickControls2Windows` : style Windows des contrôles (tiré par les boîtes de dialogue).
- `Qt6QuickControls2WindowsStyleImpl` : implémentation du style Windows.
- `Qt6QuickNativeStyle` : style natif, dont dépend le style Windows.
- `Qt6QuickDialogs2` : boîtes de dialogue (fichiers, couleurs, messages).
- `Qt6QuickDialogs2QuickImpl` : implémentation des boîtes de dialogue.
- `Qt6QuickDialogs2Utils` : outils communs des boîtes de dialogue.
- `Qt6LabsFolderListModel` : liste des fichiers d'un dossier.
- `Qt6LabsQmlModels` : modèles de données complémentaires.
- `Qt6LabsSettings` : réglages enregistrés depuis QML.
- `Qt6LabsAnimation` : animations complémentaires.
- `Qt6LabsPlatform` : menus et boîtes de dialogue du système.
- `Qt6Pdf` : lecture des documents PDF (import PDF), avec PDFium (voir plus bas).
- `Qt6PrintSupport` : impression.
- `Qt6Widgets` : composants classiques, tirés par l'impression (l'application reste une interface Qt Quick).
- `Qt6Concurrent` : calculs en parallèle.
- `Qt6Multimedia` : lecture et enregistrement du son et de la vidéo, avec FFmpeg (voir plus bas).
- `Qt6MultimediaQuick` : lecteur multimédia de l'interface.
- `Qt6WebView` : cadre web intégré, par le moteur web du système.
- `Qt6WebViewQuick` : cadre web de l'interface.

### Composants tiers inclus dans Qt

Relevés dans la page « Licenses Used in Qt » (https://doc.qt.io/qt-6/licenses-used-in-qt.html) et dans la
nomenclature SPDX livrée avec Qt 6.12.0 (dossier `sbom/` de l'installation), pour les modules livrés. Selon la
plateforme, certains ne sont pas présents dans le paquet (Qt peut aussi s'appuyer sur la version du système).

- Qt Core, Apache Tika MimeType Definitions : Licence SPDX : `Apache-2.0`.
- Qt Core, BLAKE2 (implémentation de référence) : Licence SPDX : `CC0-1.0 OR Apache-2.0`.
- Qt Core, zlib 1.3.2 : Licence SPDX : `Zlib`.
- Qt Core, Easing Equations (Robert Penner) : Licence SPDX : `BSD-3-Clause`.
- Qt Core, double-conversion 3.4.0 : Licence SPDX : `BSD-3-Clause`.
- Qt Core, MD4 et MD5 : Licence SPDX : `CC0-1.0`.
- Qt Core, SHA-1 (domaine public) : Licence SPDX : `LicenseRef-SHA1-Public-Domain`.
- Qt Core, PCRE2 10.48 : Licence SPDX : `LicenseRef-BSD-3-Clause-with-PCRE2-Binary-Like-Packages-Exception`.
- Qt Core, compilateur à la volée de PCRE2 (SLJIT) : Licence SPDX : `BSD-2-Clause`.
- Qt Core, SHA-3 Keccak 3.2 : Licence SPDX : `CC0-1.0`.
- Qt Core, SHA-3 brg_endian 1.0.0 : Licence SPDX : `BSD-2-Clause`.
- Qt Core, SHA-384 et SHA-512 (RFC 6234) : Licence SPDX : `BSD-3-Clause`.
- Qt Core, SipHash : Licence SPDX : `CC0-1.0`.
- Qt Core, TinyCBOR 7.0 : Licence SPDX : `MIT`.
- Qt Core, forkfd : Licence SPDX : `MIT`.
- Qt Core, tl::expected : Licence SPDX : `CC0-1.0`.
- Qt Core, Unicode Character Database 36 et Unicode CLDR 48.2 : Licence SPDX : `Unicode-3.0`.
- Qt Gui, FreeType 2.14.3 et son rastériseur : Licence SPDX : `FTL OR GPL-2.0-only` (ActCraft retient la FTL ;
  textes `LICENSES/FTL.txt` et `LICENSES/GPL-2.0-only.txt`).
- Qt Gui, FreeType 2, prise en charge BDF : Licence SPDX : `MIT`.
- Qt Gui, FreeType 2, prise en charge PCF : Licence SPDX : `MIT AND MIT-open-group`.
- Qt Gui, FreeType 2, zlib : Licence SPDX : `Zlib`.
- Qt Gui, HarfBuzz-NG 14.5.0 : Licence SPDX : `MIT`.
- Qt Gui, libjpeg-turbo 3.2.0 : Licence SPDX : `IJG AND BSD-3-Clause`.
- Qt Gui, libpng 1.6.58 : Licence SPDX : `Libpng AND libpng-2.0`.
- Qt Gui, MD4C 0.5.3 : Licence SPDX : `MIT`.
- Qt Gui, Pixman 0.17.12 : Licence SPDX : `MIT`.
- Qt Gui, Emoji Segmenter 0.4.0 : Licence SPDX : `Apache-2.0`.
- Qt Gui, Adobe Glyph List For New Fonts 1.7 : Licence SPDX : `BSD-3-Clause`.
- Qt Gui, polices DejaVu 2.37 : Licence SPDX : `Bitstream-Vera`.
- Qt Gui, D3D12 Memory Allocator, Vulkan Memory Allocator 3.3.0 et générateur de mipmaps D3D12 : Licence SPDX :
  `MIT`.
- Qt Gui, Vulkan API Registry 1.4.308 : Licence SPDX : `Apache-2.0 OR MIT`.
- Qt Gui, en-têtes OpenGL et OpenGL ES 2 : Licence SPDX : `MIT`.
- Qt Gui, algorithme de mise à l'échelle lissée : Licence SPDX : `BSD-2-Clause AND Imlib2`.
- Qt Gui, WebGradients : Licence SPDX : `MIT`.
- Qt Gui, interface Wintab (Windows) : Licence SPDX : `LicenseRef-Lcs-Telegraphics`.
- Qt Gui, style natif pour Android (Android) : Licence SPDX : `Apache-2.0`.
- Qt Gui, profil de couleur sRGB (fichier ICC) : Licence SPDX : `LicenseRef-ICC-License`.
- Qt Image Formats, libtiff 4.7.2 : Licence SPDX : `libtiff`.
- Qt Image Formats, libwebp 1.6.0 : Licence SPDX : `BSD-3-Clause`.
- Qt Network, Public Suffix List : Licence SPDX : `MPL-2.0`.
- Qt Network, libpsl : Licence SPDX : `BSD-3-Clause`.
- Qt Qml, JavaScriptCore Macro Assembler : Licence SPDX : `BSD-2-Clause`.
- Qt Quick, Yoga 2.0.1 : Licence SPDX : `MIT`.
- Qt Quick Controls, valeurs d'ombre d'Angular Material : Licence SPDX : `MIT`.
- Qt Multimedia, dr_libs 0.14.6 : Licence SPDX : `Unlicense OR MIT-0`.
- Qt Multimedia, Signalsmith Stretch 1.1.0 : Licence SPDX : `MIT`.
- Qt Multimedia, Two-Level Segregated Fit 3.1 : Licence SPDX : `BSD-3-Clause`.
- Qt Multimedia, Boost : Licence SPDX : `BSL-1.0`.
- Qt Multimedia, libjpeg : Licence SPDX : `IJG`.
- Qt Multimedia, zlib : Licence SPDX : `Zlib`.

## Qt PDF et PDFium

L'import PDF passe par le module Qt PDF (`Qt6Pdf`, `LGPL-3.0-only`, lié dynamiquement ; jamais Qt WebEngine). Qt
PDF embarque **PDFium**, le moteur PDF du projet Chromium (https://pdfium.googlesource.com/pdfium/), Licence SPDX :
`BSD-3-Clause`. Composants que Qt déclare pour Qt PDF (page « Licenses Used in Qt », section Qt PDF), recopiés en nos
mots :

- Qt PDF, Abseil (https://abseil.io/) : Licence SPDX : `Apache-2.0`.
- Qt PDF, FreeType (https://freetype.org/) : Licence SPDX : `FTL`.
- Qt PDF, code commun du projet Chromium (https://www.chromium.org/) : Licence SPDX : `BSD-3-Clause`.
- Qt PDF, fast_float (https://github.com/fastfloat/fast_float) : Licence SPDX : `MIT`.
- Qt PDF, ICU (https://icu.unicode.org/) : Licence SPDX : `MIT`.
- Qt PDF, libjpeg-turbo (https://libjpeg-turbo.org/) : Licence SPDX : `IJG AND BSD-3-Clause AND Zlib`.
- Qt PDF, libpng (http://www.libpng.org/) : Licence SPDX : `libpng-2.0 AND Libpng`.
- Qt PDF, zlib (https://zlib.net/) : Licence SPDX : `Zlib`.

## Qt Multimedia, FFmpeg et Qt WebView

Lecture et enregistrement de médias (son, image, vidéo) et cadre web intégré, à partir du lot 8.

- **Qt Multimedia** (`Qt6Multimedia`, `Qt6MultimediaQuick`) et **Qt WebView** (`Qt6WebView`, `Qt6WebViewQuick`) :
  même éditeur, même version et même licence que Qt ci-dessus (`LGPL-3.0-only`), liés dynamiquement ; mêmes
  obligations (bibliothèques remplaçables, texte de la licence dans `LICENSES/`).
- **FFmpeg** 9.0.1 (Les développeurs de FFmpeg, https://ffmpeg.org/) : sous Windows et Android, Qt Multimedia lit et
  écrit les médias par son greffon `ffmpeg`, qui charge les bibliothèques dynamiques de FFmpeg livrées par Qt,
  compilées sans l'option GPL et sans composant « non libre ». L'utilisateur peut les remplacer. Le code source de la
  version livrée est publié par le projet Qt (https://code.qt.io/) et par FFmpeg. ActCraft ne modifie pas FFmpeg.
  Licence SPDX : `LGPL-2.1-or-later AND BSD-3-Clause AND BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`
  (licence principale : LGPL 2.1 ou ultérieure ; les autres couvrent des fichiers inclus). Sous macOS, le Qt de
  Homebrew s'appuie sur les composants du système (AVFoundation) et n'embarque pas FFmpeg.
- **Moteur du cadre web** : Windows, composant WebView2 de Microsoft, installé sur le poste, non livré avec
  ActCraft ; macOS et Android, navigateur du système. ActCraft ne livre **pas** Qt WebEngine (Chromium).
- Les brevets sur certains codecs vidéo relèvent des utilisateurs et des distributeurs, non de la licence du logiciel.

## Bibliothèques non Qt livrées

Une entrée par ligne de `packaging/autres-autorises.txt` : `scripts/verifier-licences.ps1` refuse tout paquet qui
contient une bibliothèque `*.dll` (hors `Qt6*`) ou `*.so` (hors `libQt6*`) sans ligne correspondante.

- `vcruntime140*.dll` (Windows) : bibliothèques d'exécution de Microsoft Visual C++ 2022 (`vcruntime140.dll`,
  `vcruntime140_1.dll`), redistribuées à côté de l'exécutable selon les conditions des « Distributable Code » de
  Microsoft Visual Studio 2022. Licence SPDX : `LicenseRef-Microsoft-Redistributable`.
- `msvcp140*.dll` (Windows) : bibliothèque standard C++ de Microsoft Visual C++ 2022 (`msvcp140.dll`), mêmes
  conditions. Licence SPDX : `LicenseRef-Microsoft-Redistributable`.
- `D3Dcompiler_47.dll` (Windows) : compilateur de shaders Direct3D du kit de développement Windows, déposé par
  windeployqt. Licence SPDX : `LicenseRef-Microsoft-Redistributable`.
- `dxcompiler.dll` (Windows) : compilateur de shaders DirectX (DXC) du kit de développement Windows, déposé par
  windeployqt s'il est présent. Licence SPDX : `LicenseRef-Microsoft-Redistributable`.
- `dxil.dll` (Windows) : validateur de shaders DirectX du kit de développement Windows, déposé avec DXC. Licence
  SPDX : `LicenseRef-Microsoft-Redistributable`.
- `opengl32sw.dll` (Windows) : rendu OpenGL logiciel (Mesa llvmpipe, https://mesa3d.org/) livré par Qt, déposé par
  windeployqt. Licence SPDX : `MIT`.
- `avcodec*.dll` (Windows) : FFmpeg, codecs. Licence SPDX : `LGPL-2.1-or-later AND BSD-3-Clause AND BSD-2-Clause AND
  BSD-Source-Code AND ISC AND MIT AND MPL-2.0`.
- `avformat*.dll` (Windows) : FFmpeg, formats de fichiers. Licence SPDX : `LGPL-2.1-or-later AND BSD-3-Clause AND
  BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`.
- `avutil*.dll` (Windows) : FFmpeg, outils communs. Licence SPDX : `LGPL-2.1-or-later AND BSD-3-Clause AND
  BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`.
- `swresample*.dll` (Windows) : FFmpeg, rééchantillonnage du son. Licence SPDX : `LGPL-2.1-or-later AND BSD-3-Clause
  AND BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`.
- `swscale*.dll` (Windows) : FFmpeg, mise à l'échelle des images. Licence SPDX : `LGPL-2.1-or-later AND BSD-3-Clause
  AND BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`.
- `libavcodec*.so` (Android) : FFmpeg, codecs. Licence SPDX : `LGPL-2.1-or-later AND BSD-3-Clause AND BSD-2-Clause
  AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`.
- `libavformat*.so` (Android) : FFmpeg, formats de fichiers. Licence SPDX : `LGPL-2.1-or-later AND BSD-3-Clause AND
  BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`.
- `libavutil*.so` (Android) : FFmpeg, outils communs. Licence SPDX : `LGPL-2.1-or-later AND BSD-3-Clause AND
  BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`.
- `libswresample*.so` (Android) : FFmpeg, rééchantillonnage du son. Licence SPDX : `LGPL-2.1-or-later AND
  BSD-3-Clause AND BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`.
- `libswscale*.so` (Android) : FFmpeg, mise à l'échelle des images. Licence SPDX : `LGPL-2.1-or-later AND
  BSD-3-Clause AND BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`.
- `q*.dll` (Windows) : greffons de Qt déposés par windeployqt (plateforme, formats d'image, modules QML…). Licence
  SPDX : `LGPL-3.0-only`.
- `*plugin.dll` (Windows) : autres greffons de Qt déposés par windeployqt (modules QML, greffons multimédia). Licence
  SPDX : `LGPL-3.0-only`.
- `libplugins_*.so` (Android) : greffons de Qt déposés par androiddeployqt. Licence SPDX : `LGPL-3.0-only`.
- `libqml_*.so` (Android) : modules QML de Qt déposés par androiddeployqt. Licence SPDX : `LGPL-3.0-only`.
- `libc++_shared.so` (Android) : bibliothèque standard C++ du kit Android NDK (libc++ du projet LLVM,
  https://libcxx.llvm.org/). Licence SPDX : `Apache-2.0 WITH LLVM-exception`.
- `libactcraft_*.so` (Android) : ActCraft lui-même. Licence SPDX : `MIT`.

## miniz 3.0.2

Bibliothèque de compression et d'archives zip, liée statiquement à ActCraft pour le format `.actcraft`. Source :
https://github.com/richgel999/miniz (version 3.0.2), copie dans `third_party/miniz/` (licence dans
`third_party/miniz/LICENSE`). Licence SPDX : `MIT`. Mention de la licence :

```text
Copyright 2013-2014 RAD Game Tools and Valve Software
Copyright 2010-2014 Rich Geldreich and Tenacious Software LLC

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

## Hunspell 1.7.2

Correcteur orthographique, lié statiquement à ActCraft. Copyright 2002-2022 Németh László et contributeurs.
Source : https://github.com/hunspell/hunspell (version étiquetée v1.7.2), copie dans `third_party/hunspell/`.
Hunspell est proposé sous une triple licence (MPL 1.1, GPL 2.0 ou LGPL 2.1) ; ActCraft l'utilise sous l'option
**Mozilla Public License 1.1** (décision H6-6b), Licence SPDX : `MPL-1.1` (texte dans `LICENSES/MPL-1.1.txt` et
`third_party/hunspell/COPYING.MPL` ; mentions dans `third_party/hunspell/license.hunspell` et `license.myspell`). Les
sources de Hunspell livrées avec ActCraft sont celles de la version 1.7.2, sans modification ; elles sont disponibles
à l'adresse ci-dessus et dans le dépôt d'ActCraft.

## Dictionnaire français (Grammalecte)

Dictionnaires orthographiques français, version 7.7, variante « classique » (fichiers `fr.aff` et `fr.dic`, copies
inchangées de `fr-classique.aff` et `fr-classique.dic`), dans `third_party/dictionaries/fr/`. Olivier R. et
contributeurs, https://grammalecte.net/. Licence SPDX : `MPL-2.0` (texte dans `LICENSES/MPL-2.0.txt` et
`third_party/dictionaries/fr/LICENSE.txt`). Le dictionnaire est livré dans les ressources de l'application.
