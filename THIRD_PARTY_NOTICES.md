# Avis concernant les composants tiers

ActCraft est distribué sous licence MIT (fichier `LICENSE`). Ses paquets (installateur Windows, APK Android)
contiennent aussi les composants tiers ci-dessous, chacun sous sa propre licence.

## Qt 6.12

ActCraft utilise le framework Qt (The Qt Company Ltd. et autres contributeurs), version 6.12, sous la **GNU Lesser
General Public License version 3** (LGPL-3.0-only ; texte dans `LICENSES/LGPL-3.0.txt`, qui renvoie à la GNU
General Public License version 3, texte dans `LICENSES/GPL-3.0.txt`).

- Qt est lié **dynamiquement** : bibliothèques `Qt6*.dll` à côté de `actcraft.exe` sous Windows, `libQt6*.so` dans
  l'APK sous Android. Vous pouvez remplacer ces bibliothèques par une version de Qt modifiée et compatible.
- Code source de Qt 6.12 : https://download.qt.io/official_releases/qt/6.12/ et https://code.qt.io/.
- Code source d'ActCraft : https://github.com/jejeasterix/actcraft.
- Modules livrés : ceux de `packaging/qt-autorises.txt`, contrôlés à chaque paquet par
  `scripts/verifier-licences.ps1`. Aucun module disponible seulement sous GPL n'est livré.

### Composants tiers inclus dans Qt

D'après la page « Licenses Used in Qt » (https://doc.qt.io/qt-6/licenses-used-in-qt.html), pour les modules
livrés. Selon la plateforme, certains ne sont pas présents dans le paquet.

| Module | Composant | Licence |
|---|---|---|
| Qt Core | Apache Tika MimeType Definitions | Apache-2.0 |
| Qt Core | BLAKE2 (implémentation de référence) | CC0-1.0 ou Apache-2.0 |
| Qt Core | zlib | Zlib |
| Qt Core | Easing Equations (Robert Penner) | BSD-3-Clause |
| Qt Core | double-conversion | BSD-3-Clause |
| Qt Core | MD4, MD5, SHA-1 | domaine public |
| Qt Core | PCRE2 et son compilateur SLJIT | BSD-3-Clause (avec exception PCRE2), BSD-2-Clause |
| Qt Core | SHA-3 Keccak, brg_endian | CC0-1.0, BSD-2-Clause |
| Qt Core | SHA-384 et SHA-512 (RFC 6234) | BSD-3-Clause |
| Qt Core | SipHash, tl::expected | CC0-1.0 |
| Qt Core | TinyCBOR, forkfd | MIT |
| Qt Core | Unicode Character Database, Unicode CLDR | Unicode-DFS-2016, Unicode-3.0 |
| Qt Gui | FreeType 2 (et son rastériseur) | FTL ou GPL-2.0-only (FTL retenue) |
| Qt Gui | FreeType 2 : BDF, PCF, zlib | MIT, Zlib |
| Qt Gui | HarfBuzz-NG | MIT |
| Qt Gui | libjpeg-turbo | IJG et BSD-3-Clause |
| Qt Gui | libpng | Libpng-2.0 |
| Qt Gui | MD4C | MIT |
| Qt Gui | Pixman | MIT |
| Qt Gui | Emoji Segmenter | Apache-2.0 |
| Qt Gui | Adobe Glyph List For New Fonts | BSD-3-Clause |
| Qt Gui | DejaVu Fonts | Bitstream Vera |
| Qt Gui | D3D12 Memory Allocator, Vulkan Memory Allocator, générateur de mipmaps D3D12 | MIT |
| Qt Gui | Vulkan API Registry | Apache-2.0 ou MIT |
| Qt Gui | en-têtes OpenGL et OpenGL ES 2 | MIT |
| Qt Gui | Smooth Scaling Algorithm | BSD-2-Clause et Imlib2 |
| Qt Gui | WebGradients | MIT |
| Qt Gui | Wintab API (Windows) | licence LCS-Telegraphics |
| Qt Gui | Native Style for Android (Android) | Apache-2.0 |
| Qt Gui | profil de couleur sRGB (fichier ICC) | licence de l'International Color Consortium |
| Qt Image Formats | libtiff 4.7.2 (TIFF Software Distribution) | Libtiff License |
| Qt Image Formats | libwebp 1.6.0 (WebP) | BSD-3-Clause |
| Qt Qml | JavaScriptCore Macro Assembler | BSD-2-Clause |
| Qt Quick | Yoga | MIT |
| Qt Quick Controls | valeurs d'ombre d'Angular Material | MIT |
| Qt Network | Public Suffix List, libpsl | MPL-2.0, BSD-3-Clause |

Les textes complets de ces licences accompagnent le code source de Qt (fichiers `LICENSES/` et
`qt_attribution.json` de chaque module).

## Qt Multimedia, FFmpeg et Qt WebView (Qt 6.12)

Lecture et enregistrement de médias (son, image, vidéo) et cadre web intégré, à partir du lot 8.

- **Qt Multimedia** (modules `Qt6Multimedia` et `Qt6MultimediaQuick`) et **Qt WebView** (`Qt6WebView` et
  `Qt6WebViewQuick`) : même éditeur, même version et même licence que Qt ci-dessus (**LGPL-3.0-only**), liés
  dynamiquement ; mêmes obligations (bibliothèques remplaçables, texte de la licence dans `LICENSES/`).
- **FFmpeg** : sous Windows et Android, Qt Multimedia lit et écrit les médias par son greffon `ffmpeg`, qui embarque
  des bibliothèques FFmpeg (Les développeurs de FFmpeg, https://ffmpeg.org/). Qt les livre compilées sous la
  **GNU Lesser General Public License version 2.1 ou ultérieure** (LGPL-2.1-or-later), sans l'option GPL et sans
  composant « non libre », et les charge comme bibliothèques dynamiques : l'utilisateur peut les remplacer. Le code
  source de la version livrée est publié par le projet Qt (https://code.qt.io/) et par FFmpeg. ActCraft ne modifie
  pas FFmpeg. Versions et options de compilation exactes : à relever sur le poste Windows (rapport
  `docs/verifications/2026-10-22-lot8-modules.md`). Sous macOS, Qt Multimedia s'appuie sur les composants du système
  (AVFoundation) et n'embarque pas FFmpeg.
- **Moteur du cadre web** : Windows, composant WebView2 de Microsoft, installé sur le poste, non livré avec
  ActCraft ; macOS et Android, navigateur du système. ActCraft ne livre **pas** Qt WebEngine (Chromium).
- Les brevets sur certains codecs vidéo relèvent des utilisateurs et des distributeurs, non de la licence du logiciel.

## Bibliothèques d'exécution de Microsoft Visual C++ (Windows)

`vcruntime140.dll`, `vcruntime140_1.dll` et `msvcp140.dll` sont redistribués à côté de l'exécutable selon les
conditions des « Distributable Code » de Microsoft Visual Studio 2022.

## miniz 3.0.2

Bibliothèque de compression et d'archives zip, liée statiquement à ActCraft pour le format `.actcraft`.
Source : https://github.com/richgel999/miniz (version 3.0.2). Licence MIT :

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
Source : https://github.com/hunspell/hunspell (version étiquetée v1.7.2). Hunspell est proposé sous une triple licence
(MPL 1.1, GPL 2.0 ou LGPL 2.1) ; ActCraft l'utilise sous l'option **Mozilla Public License 1.1** (une des trois
licences proposées par ses auteurs). Texte dans `third_party/hunspell/COPYING.MPL`, mentions dans
`third_party/hunspell/license.hunspell` et `license.myspell`. Les sources de Hunspell livrées avec ActCraft sont celles
de la version 1.7.2, sans modification ; elles sont disponibles à l'adresse ci-dessus.

## Dictionnaire français (Grammalecte)

Dictionnaires orthographiques français, version 7.7, variante « classique » (fichiers `fr.aff` et `fr.dic`, copies
inchangées de `fr-classique.aff` et `fr-classique.dic`). Olivier R. et contributeurs, https://grammalecte.net/.
Licence **Mozilla Public License 2.0** (texte dans `third_party/dictionaries/fr/LICENSE.txt`,
http://www.mozilla.org/MPL/2.0/). Le dictionnaire est livré dans les ressources de l'application.
