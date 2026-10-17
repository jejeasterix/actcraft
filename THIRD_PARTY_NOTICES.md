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
