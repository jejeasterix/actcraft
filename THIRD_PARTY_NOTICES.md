# Avis concernant les composants tiers

ActCraft est distribué sous licence MIT (fichier `LICENSE`, Licence SPDX : `MIT`, Copyright (c) 2026 Jérôme Denis).
Ses paquets (installateur Windows, APK Android) contiennent aussi les composants tiers ci-dessous, chacun sous sa
propre licence. Chaque licence est citée par son identifiant SPDX (https://spdx.org/licenses/) ; son texte complet,
`LICENSES/<identifiant>.txt`, est livré dans le dossier `LICENSES/` à côté de l'exécutable sous Windows et, sur toutes
les plateformes (APK compris), dans les ressources de l'application (`:/actcraft/LICENSES/`). Ce sont les textes de la
liste SPDX ; ceux dont l'identifiant commence par `LicenseRef-` sont repris du projet Qt ou renvoient aux conditions
de l'éditeur.

Les lignes de copyright viennent des déclarations de Qt (nomenclature SPDX de Qt 6.12.0) ou de la source du
composant. Une ligne de copyright qui renvoie au relevé à faire au PC signale une ligne absente de la nomenclature disponible lors de
la rédaction : elle sera reprise des fichiers `qt_attribution.json` de Qt 6.12.0 pour Windows et pour Android.

Ce document décrit ce que les paquets contiennent et sous quelles licences ; il n'a pas été validé par un juriste et
ne constitue pas un avis juridique.

## Qt 6.12

ActCraft utilise le framework Qt (Copyright (C) The Qt Company Ltd. et autres contributeurs), version 6.12, sous la **GNU Lesser
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
- `Qt6WebView` : cadre web intégré sous Android, par le moteur web du système.
- `Qt6WebViewQuick` : cadre web de l'interface sous Android.
- `Qt6WebEngineCore` : moteur Chromium du cadre web (PC et Mac), chargé au premier cadre.
- `Qt6WebEngineQuick` : vue web de l'interface (PC et Mac).
- `Qt6WebEngineQuickDelegatesQml` : éléments d'interface du moteur web.
- `Qt6WebChannel` : canal entre le moteur web et l'application, tiré par Qt WebEngine.
- `Qt6WebChannelQuick` : même canal en QML, tiré par Qt WebEngine.
- `Qt6Positioning` : géolocalisation, tirée par Qt WebEngine (les demandes des pages sont refusées).
- `Qt6DBus` : bus de messages D-Bus, tiré par QtGui de Homebrew (paquet macOS).
- `Qt6PdfQuick` : affichage PDF en QML, copié avec le module QML de Qt PDF (paquet macOS).
- `Qt6QmlDesignSupport` : aides de Qt Design Studio, copiées avec le dossier QtQml (paquet macOS).
- `Qt6QmlLocalStorage` : stockage local en QML, copié avec le dossier QtQuick (paquet macOS).
- `Qt6QmlXmlListModel` : modèle de liste XML en QML, copié avec le dossier QtQml (paquet macOS).
- `Qt6QuickControls2IOS` : style iOS de Qt Quick Controls (paquet macOS).
- `Qt6QuickControls2IOSStyleImpl` : implémentation du style iOS (paquet macOS).
- `Qt6QuickControls2Macos` : style macOS de Qt Quick Controls (paquet macOS).
- `Qt6QuickControls2MacOSStyleImpl` : implémentation du style macOS (paquet macOS).
- `Qt6QuickControls2Native` : base des styles natifs de Qt Quick Controls (paquet macOS).
- `Qt6QuickParticles` : particules de Qt Quick, copiées avec le dossier QtQuick (paquet macOS).
- `Qt6QuickShapesDesignHelpers` : aides de conception des formes de Qt Quick (paquet macOS).
- `Qt6QuickVectorImage` : images vectorielles de Qt Quick (paquet macOS).
- `Qt6QuickVectorImageGenerator` : génération des images vectorielles (paquet macOS).
- `Qt6QuickVectorImageHelpers` : aides des images vectorielles (paquet macOS).
- `Qt6Sql` : accès aux bases de données, tiré par le stockage local QML (paquet macOS).

### Composants tiers inclus dans Qt

Relevés dans la page « Licenses Used in Qt » (https://doc.qt.io/qt-6/licenses-used-in-qt.html) et dans la
nomenclature SPDX livrée avec Qt 6.12.0 (dossier `sbom/` de l'installation), pour les modules livrés. Selon la
plateforme, certains ne sont pas présents dans le paquet (Qt peut aussi s'appuyer sur la version du système).

- Qt Core, Apache Tika MimeType Definitions : Licence SPDX : `Apache-2.0`. Copyright 2026 The Apache Software
  Foundation.
- Qt Core, BLAKE2 (implémentation de référence) : Licence SPDX : `CC0-1.0 OR Apache-2.0`. Copyright 2012, Samuel
  Neves.
- Qt Core, zlib 1.3.2 : Licence SPDX : `Zlib`. Copyright : à relever au PC.
- Qt Core, Easing Equations : Licence SPDX : `BSD-3-Clause`. Copyright (c) 2001 Robert Penner.
- Qt Core, double-conversion 3.4.0 : Licence SPDX : `BSD-3-Clause`. Copyright : à relever au PC.
- Qt Core, MD4 et MD5 : Licence SPDX : `CC0-1.0`. Copyright : aucun (MD4 écrit par Alexander Peslyak en 2001 et
  placé dans le domaine public ; MD5 écrit par Colin Plumb en 1993, aucun droit revendiqué).
- Qt Core, SHA-1 : Licence SPDX : `LicenseRef-SHA1-Public-Domain`. Copyright (C) Dominik Reichl ; Copyright (C) 2016
  The Qt Company Ltd.
- Qt Core, PCRE2 10.48 : Licence SPDX : `LicenseRef-BSD-3-Clause-with-PCRE2-Binary-Like-Packages-Exception`.
  Copyright : à relever au PC.
- Qt Core, compilateur à la volée de PCRE2 (SLJIT) : Licence SPDX : `BSD-2-Clause`. Copyright : à relever au PC.
- Qt Core, SHA-3 Keccak 3.2 : Licence SPDX : `CC0-1.0`. Copyright : aucun (Guido Bertoni, Joan Daemen, Michaël
  Peeters et Gilles Van Assche ont renoncé à leurs droits).
- Qt Core, SHA-3 brg_endian 1.0.0 : Licence SPDX : `BSD-2-Clause`. Copyright (c) 1998-2013, Brian Gladman, Worcester,
  UK. All rights reserved.
- Qt Core, SHA-384 et SHA-512 (RFC 6234) : Licence SPDX : `BSD-3-Clause`. Copyright (c) 2011 IETF Trust and the
  persons identified as authors of the code.
- Qt Core, SipHash : Licence SPDX : `CC0-1.0`. Copyright (C) 2012-2014 Jean-Philippe Aumasson ; Copyright (C)
  2012-2014 Daniel J. Bernstein ; Copyright (C) 2016 Intel Corporation.
- Qt Core, TinyCBOR 7.0 : Licence SPDX : `MIT`. Copyright (C) 2015-2025 Intel Corporation.
- Qt Core, forkfd : Licence SPDX : `MIT`. Copyright (C) 2016 Intel Corporation ; Copyright (C) 2015 Klarälvdalens
  Datakonsult AB, a KDAB Group company.
- Qt Core, tl::expected : Licence SPDX : `CC0-1.0`. Copyright : aucun (Sy Brand a renoncé à ses droits).
- Qt Core, Unicode Character Database 36 et Unicode CLDR 48.2 : Licence SPDX : `Unicode-3.0`. Copyright (C)
  1991-2025 Unicode, Inc. ; Copyright (C) 2004-2025 Unicode, Inc.
- Qt Gui, FreeType 2.14.3 : Licence SPDX : `FTL OR GPL-2.0-only` (ActCraft retient la FTL ; textes
  `LICENSES/FTL.txt` et `LICENSES/GPL-2.0-only.txt`). Mention demandée par la FTL : « Portions of this software are
  copyright © The FreeType Project (https://freetype.org). All rights reserved. » (année de la version livrée à
  ajouter au relevé du PC). Copyright : à relever au PC.
- Qt Gui, rastériseur anticrénelé de FreeType 2 : Licence SPDX : `FTL OR GPL-2.0-only`. Copyright 2000-2016 by David
  Turner, Robert Wilhelm, and Werner Lemberg.
- Qt Gui, FreeType 2, prise en charge BDF : Licence SPDX : `MIT`. Copyright : à relever au PC.
- Qt Gui, FreeType 2, prise en charge PCF : Licence SPDX : `MIT AND MIT-open-group`. Copyright : à relever au PC.
- Qt Gui, FreeType 2, zlib : Licence SPDX : `Zlib`. Copyright : à relever au PC.
- Qt Gui, HarfBuzz-NG 14.5.0 : Licence SPDX : `MIT`. Copyright : à relever au PC.
- Qt Gui, libjpeg-turbo 3.2.0 : Licence SPDX : `IJG AND BSD-3-Clause`. Mention demandée par l'IJG : « This software
  is based in part on the work of the Independent JPEG Group. » Copyright : à relever au PC.
- Qt Gui, libpng 1.6.58 : Licence SPDX : `Libpng AND libpng-2.0`. Copyright : à relever au PC.
- Qt Gui, MD4C 0.5.3 : Licence SPDX : `MIT`. Copyright : à relever au PC.
- Qt Gui, Pixman 0.17.12 : Licence SPDX : `MIT`. Copyright : à relever au PC.
- Qt Gui, Emoji Segmenter 0.4.0 : Licence SPDX : `Apache-2.0`. Copyright 2019 Google LLC.
- Qt Gui, Adobe Glyph List For New Fonts 1.7 : Licence SPDX : `BSD-3-Clause`. Copyright 2002, 2003, 2005, 2006,
  2008, 2010, 2015 Adobe Systems.
- Qt Gui, polices DejaVu 2.37 : Licence SPDX : `Bitstream-Vera`. Copyright : à relever au PC.
- Qt Gui, D3D12 Memory Allocator : Licence SPDX : `MIT`. Copyright : à relever au PC.
- Qt Gui, Vulkan Memory Allocator 3.3.0 : Licence SPDX : `MIT`. Copyright (c) 2017-2025 Advanced Micro Devices, Inc.
  All rights reserved.
- Qt Gui, générateur de mipmaps D3D12 : Licence SPDX : `MIT`. Copyright (c) 2015 Microsoft.
- Qt Gui, Vulkan API Registry 1.4.308 : Licence SPDX : `Apache-2.0 OR MIT`. Copyright (c) 2015-2025 The Khronos
  Group Inc.
- Qt Gui, en-têtes OpenGL et OpenGL ES 2 : Licence SPDX : `MIT`. Copyright (c) 2013-2014 The Khronos Group Inc.
- Qt Gui, algorithme de mise à l'échelle lissée : Licence SPDX : `BSD-2-Clause AND Imlib2`. Copyright (C) 2004,
  2005 Daniel M. Duley ; (C) Carsten Haitzler and various contributors ; (C) Willem Monsuwe.
- Qt Gui, WebGradients : Licence SPDX : `MIT`. Copyright (c) 2017 itmeo.
- Qt Gui, interface Wintab (Windows) : Licence SPDX : `LicenseRef-Lcs-Telegraphics`. Copyright : à relever au PC.
- Qt Gui, style natif pour Android (Android) : Licence SPDX : `Apache-2.0`. Copyright : à relever au PC.
- Qt Gui, profil de couleur sRGB (fichier ICC) : Licence SPDX : `LicenseRef-ICC-License`. Copyright International
  Color Consortium, 2015.
- Qt Image Formats, libtiff 4.7.2 : Licence SPDX : `libtiff`. Copyright : à relever au PC.
- Qt Image Formats, libwebp 1.6.0 : Licence SPDX : `BSD-3-Clause`. Copyright : à relever au PC.
- Qt Network, Public Suffix List : Licence SPDX : `MPL-2.0`. Copyright : aucun déclaré (liste fournie à l'origine
  par Jo Hermans, tenue sur https://github.com/publicsuffix/list).
- Qt Network, libpsl : Licence SPDX : `BSD-3-Clause`. Copyright 2014-2016 The Chromium Authors. All rights reserved.
- Qt Qml, JavaScriptCore Macro Assembler : Licence SPDX : `BSD-2-Clause`. Copyright (C) 2003-2018 Apple Inc. All
  rights reserved. ; Copyright (C) 2007 Justin Haygood ; Copyright (C) 2007-2009 Torch Mobile, Inc. All rights
  reserved. ; Copyright (C) 2009, 2010 University of Szeged ; Copyright (C) 2009-2011 STMicroelectronics. All rights
  reserved. ; Copyright (C) 2010 MIPS Technologies, Inc. All rights reserved. ; Copyright (C) 2010 Peter Varga,
  University of Szeged ; Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved. ; Copyright (C)
  2011 Google Inc. All rights reserved. ; Copyright (C) 2013 Samsung Electronics. All rights reserved. ; Copyright
  (C) 2015 Cisco Systems, Inc. All rights reserved. ; Copyright (c) 2002-2009 Vivek Thampi.
- Qt Quick, Yoga 2.0.1 : Licence SPDX : `MIT`. Copyright (c) Facebook, Inc. and its affiliates ; Copyright (c) Meta
  Platforms, Inc. and affiliates.
- Qt Quick Controls, valeurs d'ombre d'Angular Material : Licence SPDX : `MIT`. Copyright (c) 2014-2016 Google, Inc.
- Qt Multimedia, dr_libs 0.14.6 : Licence SPDX : `Unlicense OR MIT-0`. Copyright 2023 David Reid.
- Qt Multimedia, Signalsmith Stretch 1.1.0 : Licence SPDX : `MIT`. Copyright : à relever au PC.
- Qt Multimedia, Two-Level Segregated Fit 3.1 : Licence SPDX : `BSD-3-Clause`. Copyright (c) 2006-2016, Matthew
  Conte.
- Qt Multimedia, Boost : Licence SPDX : `BSL-1.0`. Copyright : à relever au PC.
- Qt Multimedia, libjpeg : Licence SPDX : `IJG`. Mention demandée par l'IJG : « This software is based in part on
  the work of the Independent JPEG Group. » Copyright : à relever au PC.
- Qt Multimedia, zlib : Licence SPDX : `Zlib`. Copyright : à relever au PC.

## Qt PDF et PDFium

L'import PDF passe par le module Qt PDF (`Qt6Pdf`, `LGPL-3.0-only`, lié dynamiquement). Qt
PDF embarque **PDFium**, le moteur PDF du projet Chromium (https://pdfium.googlesource.com/pdfium/), et les
bibliothèques qu'il utilise. Composants relevés dans la page « Licenses Used in Qt » (section Qt PDF) et dans le
binaire de Qt PDF (symboles de Little CMS et d'OpenJPEG), recopiés en nos mots ; les lignes de copyright et la
liste exacte sont à relever au PC dans le fichier `qt_attribution.json` de Qt PDF 6.12.0.

- Qt PDF, PDFium : Licence SPDX : `BSD-3-Clause OR Apache-2.0`. Copyright : à relever au PC.
- Qt PDF, Abseil (https://abseil.io/) : Licence SPDX : `Apache-2.0`. Copyright : à relever au PC.
- Qt PDF, FreeType (https://freetype.org/) : Licence SPDX : `FTL`. Mention demandée par la FTL : voir FreeType
  ci-dessus. Copyright : à relever au PC.
- Qt PDF, code commun du projet Chromium (https://www.chromium.org/) : Licence SPDX : `BSD-3-Clause`. Copyright : à
  relever au PC.
- Qt PDF, fast_float (https://github.com/fastfloat/fast_float) : Licence SPDX : `MIT`. Copyright : à relever au PC.
- Qt PDF, libjpeg-turbo (https://libjpeg-turbo.org/) : Licence SPDX : `IJG AND BSD-3-Clause AND Zlib`. Mention
  demandée par l'IJG : « This software is based in part on the work of the Independent JPEG Group. » Copyright : à
  relever au PC.
- Qt PDF, libpng (http://www.libpng.org/) : Licence SPDX : `libpng-2.0 AND Libpng`. Copyright : à relever au PC.
- Qt PDF, zlib (https://zlib.net/) : Licence SPDX : `Zlib`. Copyright : à relever au PC.
- Qt PDF, Little CMS (https://www.littlecms.com/) : Licence SPDX : `MIT`. Copyright : à relever au PC.
- Qt PDF, OpenJPEG (https://www.openjpeg.org/) : Licence SPDX : `BSD-2-Clause`. Copyright : à relever au PC.
- Qt PDF, Anti-Grain Geometry (AGG) : licence et copyright à relever au PC (`qt_attribution.json` de Qt PDF ; la
  nomenclature de Qt PDF disponible sur le Mac ne déclare aucun composant).

## Qt WebEngine et Chromium

Le cadre web sur PC et Mac passe par le module Qt WebEngine (`LGPL-3.0-only`, lié dynamiquement au moment du premier cadre), qui embarque **Chromium** (https://www.chromium.org/) et ses bibliothèques. Liste exacte : fichier `sbom.spdx.json` de la version de Qt WebEngine utilisée. Le paquet macOS livre le texte de la licence de Chromium dans `Contents/Resources/LICENSE.Chromium`.

- Chromium : Licence SPDX : `BSD-3-Clause`. Copyright 2015 The Chromium Authors.

Certains codecs vidéo (H.264, AAC) dépendent de la provenance de Qt WebEngine ; leur redistribution est une décision prise avant toute publication. Les brevets sur ces codecs relèvent des utilisateurs et des distributeurs.

## Qt Multimedia, FFmpeg et Qt WebView

Lecture et enregistrement de médias (son, image, vidéo) et cadre web intégré, à partir du lot 8.

- **Qt Multimedia** (`Qt6Multimedia`, `Qt6MultimediaQuick`) et **Qt WebView** (`Qt6WebView`, `Qt6WebViewQuick`) :
  même éditeur, même version et même licence que Qt ci-dessus (`LGPL-3.0-only`), liés dynamiquement ; mêmes
  obligations (bibliothèques remplaçables, texte de la licence dans `LICENSES/`).
- **FFmpeg** (https://ffmpeg.org/) : sous Windows et Android, Qt Multimedia lit et écrit les médias par son greffon
  `ffmpeg`, qui charge les bibliothèques dynamiques de FFmpeg livrées par Qt ; l'utilisateur peut les remplacer.
  ActCraft ne modifie pas FFmpeg. Licence SPDX : `LGPL-2.1-or-later AND BSD-3-Clause AND BSD-2-Clause AND
  BSD-Source-Code AND ISC AND MIT AND MPL-2.0` (déclaration de Qt ; licence principale : LGPL 2.1 ou ultérieure, les
  autres couvrent des fichiers inclus). Copyright : les développeurs de FFmpeg ; détail à relever au PC. Version et
  options de compilation des bibliothèques livrées : à relever au PC (rapport
  `docs/verifications/2026-10-22-lot8-modules.md`) ; `scripts/verifier-licences.ps1` refuse toute bibliothèque FFmpeg
  dont la licence compilée n'est pas « LGPL version 2.1 or later » (option GPL ou « nonfree »). **Code source** : le
  code source exact des bibliothèques FFmpeg livrées est publié à côté des paquets de chaque version d'ActCraft (page
  de la version sur GitHub). Sous macOS, le Qt de Homebrew s'appuie sur les composants du système (AVFoundation) et
  n'embarque pas FFmpeg.
- **Moteur du cadre web** : PC et Mac, Qt WebEngine (Chromium), livré avec ActCraft (voir « Qt WebEngine et
  Chromium ») ; Android, navigateur du système.
- Les brevets sur certains codecs vidéo relèvent des utilisateurs et des distributeurs, non de la licence du logiciel.

## Bibliothèques non Qt livrées

Une entrée par ligne de `packaging/autres-autorises.txt` : `scripts/verifier-licences.ps1` refuse tout paquet qui
contient une bibliothèque `*.dll` (hors `Qt6*`) ou `*.so` (hors `libQt6*`) sans ligne correspondante, tout greffon
rangé sous `LGPL-3.0-only` qui ne vient pas de l'installation de Qt, et toute bibliothèque FFmpeg compilée hors
LGPL 2.1 ou ultérieure.

- `vcruntime140*.dll` (Windows) : bibliothèques d'exécution de Microsoft Visual C++ 2022 (`vcruntime140.dll`,
  `vcruntime140_1.dll`), redistribuées à côté de l'exécutable selon les conditions des « Distributable Code » de
  Microsoft Visual Studio 2022. Licence SPDX : `LicenseRef-Microsoft-Redistributable`. Copyright (c) Microsoft
  Corporation.
- `msvcp140*.dll` (Windows) : bibliothèque standard C++ de Microsoft Visual C++ 2022 (`msvcp140.dll`), mêmes
  conditions. Licence SPDX : `LicenseRef-Microsoft-Redistributable`. Copyright (c) Microsoft Corporation.
- `D3Dcompiler_47.dll` (Windows) : compilateur de shaders Direct3D du kit de développement Windows, déposé par
  windeployqt. Licence SPDX : `LicenseRef-Microsoft-Redistributable`. Copyright (c) Microsoft Corporation.
- `dxcompiler.dll` (Windows) : compilateur de shaders DirectX (DXC) du kit de développement Windows, déposé par
  windeployqt s'il est présent. Licence SPDX : `LicenseRef-Microsoft-Redistributable`. Copyright (c) Microsoft
  Corporation.
- `dxil.dll` (Windows) : validateur de shaders DirectX du kit de développement Windows, déposé avec DXC. Licence
  SPDX : `LicenseRef-Microsoft-Redistributable`. Copyright (c) Microsoft Corporation.
- `avcodec*.dll` (Windows) : FFmpeg, codecs ; licence compilée contrôlée. Licence SPDX : `LGPL-2.1-or-later AND
  BSD-3-Clause AND BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`. Copyright : les développeurs de
  FFmpeg.
- `avformat*.dll` (Windows) : FFmpeg, formats de fichiers ; licence compilée contrôlée. Licence SPDX :
  `LGPL-2.1-or-later AND BSD-3-Clause AND BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`. Copyright :
  les développeurs de FFmpeg.
- `avutil*.dll` (Windows) : FFmpeg, outils communs ; licence compilée contrôlée. Licence SPDX : `LGPL-2.1-or-later AND
  BSD-3-Clause AND BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`. Copyright : les développeurs de
  FFmpeg.
- `swresample*.dll` (Windows) : FFmpeg, rééchantillonnage du son ; licence compilée contrôlée. Licence SPDX :
  `LGPL-2.1-or-later AND BSD-3-Clause AND BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`. Copyright :
  les développeurs de FFmpeg.
- `swscale*.dll` (Windows) : FFmpeg, mise à l'échelle des images ; licence compilée contrôlée. Licence SPDX :
  `LGPL-2.1-or-later AND BSD-3-Clause AND BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`. Copyright :
  les développeurs de FFmpeg.
- `libavcodec*.so` (Android) : FFmpeg, codecs ; licence compilée contrôlée. Licence SPDX : `LGPL-2.1-or-later AND
  BSD-3-Clause AND BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`. Copyright : les développeurs de
  FFmpeg.
- `libavformat*.so` (Android) : FFmpeg, formats de fichiers ; licence compilée contrôlée. Licence SPDX :
  `LGPL-2.1-or-later AND BSD-3-Clause AND BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`. Copyright :
  les développeurs de FFmpeg.
- `libavutil*.so` (Android) : FFmpeg, outils communs ; licence compilée contrôlée. Licence SPDX : `LGPL-2.1-or-later
  AND BSD-3-Clause AND BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`. Copyright : les développeurs de
  FFmpeg.
- `libswresample*.so` (Android) : FFmpeg, rééchantillonnage du son ; licence compilée contrôlée. Licence SPDX :
  `LGPL-2.1-or-later AND BSD-3-Clause AND BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`. Copyright :
  les développeurs de FFmpeg.
- `libswscale*.so` (Android) : FFmpeg, mise à l'échelle des images ; licence compilée contrôlée. Licence SPDX :
  `LGPL-2.1-or-later AND BSD-3-Clause AND BSD-2-Clause AND BSD-Source-Code AND ISC AND MIT AND MPL-2.0`. Copyright :
  les développeurs de FFmpeg.
- `q*.dll` (Windows) : greffons de Qt déposés par windeployqt (plateforme, formats d'image, modules QML…), chacun
  présent au même chemin dans l'installation de Qt. Licence SPDX : `LGPL-3.0-only`. Copyright (C) The Qt Company Ltd.
  et autres contributeurs.
- `*plugin.dll` (Windows) : autres greffons de Qt déposés par windeployqt (modules QML, greffons multimédia), mêmes
  conditions de provenance. Licence SPDX : `LGPL-3.0-only`. Copyright (C) The Qt Company Ltd. et autres
  contributeurs.
- `libplugins_*.so` (Android) : greffons de Qt déposés par androiddeployqt, chacun présent sous le même nom dans
  l'installation de Qt pour Android. Licence SPDX : `LGPL-3.0-only`. Copyright (C) The Qt Company Ltd. et autres
  contributeurs.
- `libqml_*.so` (Android) : modules QML de Qt déposés par androiddeployqt, mêmes conditions de provenance. Licence
  SPDX : `LGPL-3.0-only`. Copyright (C) The Qt Company Ltd. et autres contributeurs.
- `libc++_shared.so` (Android) : bibliothèque standard C++ du kit Android NDK (libc++ du projet LLVM,
  https://libcxx.llvm.org/). Licence SPDX : `Apache-2.0 WITH LLVM-exception`. Copyright : les contributeurs du
  projet LLVM.
- `libactcraft_*.so` (Android) : ActCraft lui-même. Licence SPDX : `MIT`. Copyright (c) 2026 Jérôme Denis.

## Bibliothèques du paquet macOS

Une entrée par ligne « Paquet macOS » de `packaging/autres-autorises.txt` : bibliothèques de Homebrew dont dépend Qt,
copiées par macdeployqt dans `ActCraft.app/Contents/Frameworks`. `scripts/verifier-licences-mac.sh` refuse un bundle
qui contient une bibliothèque sans ligne correspondante, une bibliothèque sous GPL seule. Quand
l'expression offre un choix, ActCraft retient la licence qui n'est pas la GPL. Versions de Homebrew du 2026-11-08.

- `libb2.*.dylib` (macOS) : libb2, formule `libb2`, BLAKE2 (fonction de hachage), tirée par QtCore ; version 0.98.1, https://blake2.net/. Licence SPDX : `CC0-1.0`. Copyright : Samuel Neves et les auteurs de BLAKE2, dédié au domaine public (CC0).
- `libbrotli*.dylib` (macOS) : brotli, formule `brotli`, décompression Brotli (`libbrotlicommon`, `libbrotlidec`), tirée par FreeType ; version 1.2.1, https://github.com/google/brotli. Licence SPDX : `MIT`. Copyright (c) 2009, 2010, 2013-2016 by the Brotli Authors.
- `libcrypto.*.dylib` (macOS) : openssl@4, formule `openssl@4`, OpenSSL, cryptographie, chargée par le greffon TLS de Qt ; version 4.1.0, https://openssl-library.org. Licence SPDX : `Apache-2.0`. Copyright (c) 1998-2026 The OpenSSL Project Authors.
- `libssl.*.dylib` (macOS) : openssl@4, formule `openssl@4`, OpenSSL, protocole TLS, chargée par le greffon TLS de Qt ; version 4.1.0, https://openssl-library.org. Licence SPDX : `Apache-2.0`. Copyright (c) 1998-2026 The OpenSSL Project Authors.
- `libdbus-1.*.dylib` (macOS) : dbus, formule `dbus`, client D-Bus, tiré par QtDBus ; licence retenue : `AFL-2.1` ; version 1.16.2, https://wiki.freedesktop.org/www/Software/dbus. Licence SPDX : `AFL-2.1 OR GPL-2.0-or-later`. Copyright (C) 2002, 2003 Red Hat Inc. et les auteurs de D-Bus.
- `libdouble-conversion.*.dylib` (macOS) : double-conversion, formule `double-conversion`, conversion des nombres décimaux, tirée par QtCore ; version 3.4.0, https://github.com/google/double-conversion. Licence SPDX : `BSD-3-Clause`. Copyright 2006-2011, the V8 project authors.
- `libfreetype.*.dylib` (macOS) : freetype, formule `freetype`, rendu des polices, tiré par QtGui ; licence retenue : FreeType License ; version 2.15.0, https://www.freetype.org/. Licence SPDX : `FTL`. Copyright (C) 1996-2026 by David Turner, Robert Wilhelm, and Werner Lemberg.
- `libglib-2.0.*.dylib` (macOS) : glib, formule `glib`, GLib, boucle d'événements et utilitaires, tirée par QtCore ; version 2.90.1, https://docs.gtk.org/glib/. Licence SPDX : `LGPL-2.1-or-later`. Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald, et les auteurs de GLib.
- `libgio-2.0.*.dylib` (macOS) : glib, formule `glib`, GIO de GLib, tirée par libglib ; version 2.90.1, https://docs.gtk.org/gio/. Licence SPDX : `LGPL-2.1-or-later`. Copyright : les auteurs de GLib (voir libglib ci-dessus).
- `libgmodule-2.0.*.dylib` (macOS) : glib, formule `glib`, GModule de GLib ; version 2.90.1, https://docs.gtk.org/gmodule/. Licence SPDX : `LGPL-2.1-or-later`. Copyright : les auteurs de GLib.
- `libgobject-2.0.*.dylib` (macOS) : glib, formule `glib`, GObject de GLib ; version 2.90.1, https://docs.gtk.org/gobject/. Licence SPDX : `LGPL-2.1-or-later`. Copyright : les auteurs de GLib.
- `libgthread-2.0.*.dylib` (macOS) : glib, formule `glib`, GThread de GLib ; version 2.90.1, https://docs.gtk.org/glib/. Licence SPDX : `LGPL-2.1-or-later`. Copyright : les auteurs de GLib.
- `libgraphite2.*.dylib` (macOS) : graphite2, formule `graphite2`, moteur de mise en forme Graphite, tiré par HarfBuzz ; licence retenue : `MIT` ; version 1.3.15, https://graphite.sil.org/. Licence SPDX : `MIT OR MPL-2.0 OR LGPL-2.1-or-later OR GPL-2.0-or-later`. Copyright 2010, SIL International.
- `libharfbuzz.*.dylib` (macOS) : harfbuzz, formule `harfbuzz`, mise en forme du texte, tirée par QtGui et FreeType ; version 15.0.0, https://github.com/harfbuzz/harfbuzz. Licence SPDX : `MIT`. Copyright © 2010-2022 Google, Inc., et les auteurs de HarfBuzz.
- `libicu*.dylib` (macOS) : icu4c@79, formule `icu4c@79`, ICU (`libicudata`, `libicui18n`, `libicuuc`), Unicode et langues, tirée par QtCore ; version 79.1, https://icu.unicode.org/home. Licence SPDX : `Unicode-3.0`. Copyright © 1991-2026 Unicode, Inc..
- `libintl.*.dylib` (macOS) : gettext, formule `gettext`, libintl de gettext, tirée par GLib ; la formule couvre aussi les outils de gettext (GPL), la bibliothèque seule est sous LGPL (en-tête `libintl.h`) ; version 1.0, https://www.gnu.org/software/gettext/. Licence SPDX : `LGPL-2.1-or-later`. Copyright (C) 1995-2026 Free Software Foundation, Inc..
- `libjpeg.*.dylib` (macOS) : jpeg-turbo, formule `jpeg-turbo`, lecture et écriture JPEG, tirée par le greffon d'images et libtiff ; version 3.2.0, https://www.libjpeg-turbo.org/. Licence SPDX : `IJG AND Zlib AND BSD-3-Clause`. Copyright (C) 2009-2026 D. R. Commander, Copyright (C) 1991-2020 Thomas G. Lane, Guido Vollbeding.
- `liblzma.*.dylib` (macOS) : xz, formule `xz`, compression LZMA, tirée par libtiff ; la formule couvre aussi les outils de xz (GPL), la bibliothèque seule est sous `0BSD` (en-tête `lzma.h`) ; version 5.8.4, https://tukaani.org/xz/. Licence SPDX : `0BSD`. Copyright (C) The XZ Utils authors and contributors.
- `libmd4c.*.dylib` (macOS) : md4c, formule `md4c`, analyse du Markdown, tirée par QtGui ; version 0.6.0, https://github.com/mity/md4c. Licence SPDX : `MIT`. Copyright © 2016-2026 Martin Mitáš.
- `libpcre2-*.dylib` (macOS) : pcre2, formule `pcre2`, expressions régulières (`libpcre2-8`, `libpcre2-16`), tirées par QtCore et GLib ; version 10.49, https://www.pcre.org/. Licence SPDX : `BSD-3-Clause`. Copyright (c) 1997-2007 University of Cambridge, et les auteurs de PCRE2.
- `libpng16.*.dylib` (macOS) : libpng, formule `libpng`, lecture et écriture PNG, tirée par QtGui et FreeType ; version 1.6.59, https://www.libpng.org/pub/png/libpng.html. Licence SPDX : `libpng-2.0`. Copyright (c) 1995-2026 The PNG Reference Library Authors.
- `libsharpyuv.*.dylib` (macOS) : webp, formule `webp`, conversion de couleurs de libwebp ; version 1.6.0, https://developers.google.com/speed/webp/. Licence SPDX : `BSD-3-Clause`. Copyright (c) 2010, Google Inc..
- `libwebp*.dylib` (macOS) : webp, formule `webp`, images WebP (`libwebp`, `libwebpdemux`, `libwebpmux`), tirées par le greffon d'images et libtiff ; version 1.6.0, https://developers.google.com/speed/webp/. Licence SPDX : `BSD-3-Clause`. Copyright (c) 2010, Google Inc..
- `libtiff.*.dylib` (macOS) : libtiff, formule `libtiff`, lecture TIFF, tirée par le greffon d'images ; version 4.7.2, https://libtiff.gitlab.io/libtiff/. Licence SPDX : `libtiff`. Copyright © 1988-1997 Sam Leffler, Copyright © 1991-1997 Silicon Graphics, Inc..
- `libzstd.*.dylib` (macOS) : zstd, formule `zstd`, compression Zstandard, tirée par QtCore et libtiff ; licence retenue : `BSD-3-Clause` ; version 1.5.7, https://facebook.github.io/zstd/. Licence SPDX : `BSD-3-Clause OR GPL-2.0-only`. Copyright (c) Meta Platforms, Inc. and affiliates.

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
à l'adresse ci-dessus.

## Dictionnaire français (Grammalecte)

Dictionnaires orthographiques français, version 7.7, variante « classique » (fichiers `fr.aff` et `fr.dic`, copies
inchangées de `fr-classique.aff` et `fr-classique.dic`), dans `third_party/dictionaries/fr/`. Olivier R. et
contributeurs (Copyright : Olivier R. et contributeurs), https://grammalecte.net/. Licence SPDX : `MPL-2.0` (texte dans `LICENSES/MPL-2.0.txt` et
`third_party/dictionaries/fr/LICENSE.txt`). Le dictionnaire est livré dans les ressources de l'application.
