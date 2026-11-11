# Qt WebEngine avec codecs propriétaires (H.264, AAC) pour la release Windows (décision de Jérôme du 2026-11-11,
# docs/etudes/2026-11-11-h264-webengine.md). Construit une fois sur le PC, hors CI :
# - copie le Qt officiel ($QtSource, celui de ci.yml, jamais modifié) dans $Cible ;
# - construit Qt WebEngine 6.11.3 (sources publiées par Qt, empreinte SHA-256 vérifiée) contre cette copie, avec
#   -DFEATURE_webengine_proprietary_codecs=ON, sans jumbo, en Release sans symboles, $Taches compilations de Chromium
#   à la fois (NINJAFLAGS), puis l'installe dans $Cible (Qt WebEngine officiel de la copie remplacé) ;
# - vérifie que la copie annonce webengine-proprietary-codecs, puis écrit le témoin $Cible/.actcraft-h264, que lit
#   release.yml.
# Reprise : relancer la même commande ; ninja reprend où il s'est arrêté (sources et construction gardées dans $Travail).
# Prérequis (vérifiés avant tout) : Build Tools 2022 (MSVC 14.36 ou plus, SDK Windows 10.0.26100 ou plus), CMake 3.19+,
# Ninja, Python 3.8+ avec html5lib, Node.js 20+ 64 bits, gperf, bison et flex dans le PATH ; 80 Go libres sur le disque
# de $Travail ; fichier d'échange de 32 Go conseillé.
# Usage : pwsh -NoProfile -File ./scripts/qtwebengine-h264.ps1 [-Verifier] [-Taches 4] [-QtSource <Qt>] [-Cible <Qt>]
#         [-Travail <dossier court>]   (-Verifier : prérequis seulement, rien n'est construit)
# Dernière ligne et code : « QTWEBENGINE H264 OK : <Cible> » (0) ou « QTWEBENGINE H264 EN ECHEC : <raison> » (1).
param([switch]$Verifier, [ValidateRange(1, 16)][int]$Taches = 4,
    [string]$QtSource = 'C:\Qt\6.12.0\msvc2022_64', [string]$Cible = 'C:\Qt\6.12.0-h264\msvc2022_64',
    [string]$Travail = 'C:\qw')
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

$versionWebEngine = '6.11.3'
$archive = "qtwebengine-everywhere-src-$versionWebEngine.zip"
$url = "https://download.qt.io/official_releases/qt/6.11/$versionWebEngine/submodules/$archive"
# Empreinte publiée par Qt à côté de l'archive ($url.sha256), relevée le 2026-11-11.
$somme = '3ecc245c2a1de5af963560779a7775a9ad1063d392f9f369f3bad07dd67e8572'

try {
    $debut = Get-Date
    # Chemins courts : Chromium dépasse vite 260 caractères sous un dossier profond.
    if ($Travail.Length -gt 12) { throw "dossier de travail trop long ($Travail) : 12 caractères au plus, par exemple C:\qw" }
    if (-not (Test-Path (Join-Path $QtSource 'lib/cmake/Qt6/Qt6Config.cmake'))) { throw "Qt officiel absent : $QtSource" }
    # La copie est effacée puis refaite : jamais le Qt de ci.yml, ni un dossier qui le contient ou qu'il contient.
    $source = [System.IO.Path]::GetFullPath($QtSource).TrimEnd('\') + '\'
    $copie = [System.IO.Path]::GetFullPath($Cible).TrimEnd('\') + '\'
    if ($copie.StartsWith($source, [StringComparison]::OrdinalIgnoreCase) -or $source.StartsWith($copie, [StringComparison]::OrdinalIgnoreCase)) {
        throw "cible $Cible confondue avec le Qt officiel $QtSource"
    }

    . "$PSScriptRoot/dev-env.ps1"
    $manque = [System.Collections.Generic.List[string]]::new()
    foreach ($outil in 'cl', 'cmake', 'ninja', 'python', 'node', 'gperf', 'bison', 'flex') {
        if (-not (Get-Command $outil -ErrorAction SilentlyContinue)) { $manque.Add($outil) }
    }
    if ($manque.Count -gt 0) { throw "outils absents du PATH : $($manque -join ', ')" }
    $sdk = $env:WindowsSDKVersion.TrimEnd('\')
    if ([version]$sdk -lt [version]'10.0.26100.0') { throw "SDK Windows $sdk : 10.0.26100.0 ou plus requis" }
    $node = (& node --version).TrimStart('v')
    if ([version]$node -lt [version]'20.0.0') { throw "Node.js $node : 20 ou plus requis" }
    & python -c 'import html5lib' 2>$null
    if ($LASTEXITCODE -ne 0) { throw 'module Python html5lib absent (python -m pip install --user html5lib)' }
    New-Item -ItemType Directory -Force $Travail | Out-Null
    $libre = (Get-PSDrive ([System.IO.Path]::GetPathRoot((Resolve-Path $Travail).Path)).TrimEnd(':\')).Free / 1GB
    $memoire = (Get-CimInstance Win32_ComputerSystem).TotalPhysicalMemory / 1GB
    $echange = (Get-CimInstance Win32_PageFileUsage | Measure-Object AllocatedBaseSize -Sum).Sum / 1KB
    "Disque libre : {0:N0} Go ; mémoire : {1:N1} Go ; fichier d'échange : {2:N0} Go ; {3} compilations à la fois" -f $libre, $memoire, $echange, $Taches
    if ($libre -lt 80 -and -not (Test-Path "$Travail/b/build.ninja")) { throw ("{0:N0} Go libres : 80 Go requis" -f $libre) }
    if ($memoire + $echange -lt 1.5 * $Taches + 24) { "Attention : mémoire et échange courts pour $Taches compilations (32 Go d'échange conseillés)" }
    if ($Verifier) {
        Write-Output "QTWEBENGINE H264 OK : prérequis seulement ($Cible non construit)"
        exit 0
    }

    # Copie du Qt officiel : faite une fois (témoin .copie-complete écrit en dernier).
    if (-not (Test-Path (Join-Path $Cible '.copie-complete'))) {
        if (Test-Path $Cible) { Remove-Item -Recurse -Force $Cible }
        New-Item -ItemType Directory -Force $Cible | Out-Null
        Copy-Item -Recurse (Join-Path $QtSource '*') $Cible
        New-Item -ItemType File (Join-Path $Cible '.copie-complete') | Out-Null
        "Qt officiel copié : $QtSource -> $Cible"
    }

    $zip = Join-Path $Travail $archive
    $sources = Join-Path $Travail 's'
    if (-not (Test-Path (Join-Path $sources 'CMakeLists.txt'))) {
        if (-not (Test-Path $zip) -or (Get-FileHash $zip -Algorithm SHA256).Hash.ToLower() -ne $somme) {
            "Téléchargement : $url"
            Invoke-WebRequest $url -OutFile $zip
        }
        $obtenue = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLower()
        if ($obtenue -ne $somme) { throw "$archive : somme inattendue $obtenue" }
        $depli = Join-Path $Travail 'x'
        if (Test-Path $depli) { Remove-Item -Recurse -Force $depli }
        New-Item -ItemType Directory -Force $depli | Out-Null
        & tar.exe -xf $zip -C $depli
        if ($LASTEXITCODE -ne 0) { throw "dépliage de $archive en échec" }
        Move-Item (Join-Path $depli "qtwebengine-everywhere-src-$versionWebEngine") $sources
        Remove-Item -Recurse -Force $depli
    }

    $construction = Join-Path $Travail 'b'
    # NINJAFLAGS borne la construction de Chromium (ninja lancé par CMake) ; il est posé avant la configuration, qui
    # peut le relever. Changer -Taches après coup : supprimer $Travail\b\CMakeCache.txt (les objets restent).
    $env:NINJAFLAGS = "-j$Taches"
    if (-not (Test-Path (Join-Path $construction 'build.ninja'))) {
        & cmake -S $sources -B $construction -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_PREFIX_PATH=$Cible" `
            "-DCMAKE_INSTALL_PREFIX=$Cible" -DFEATURE_webengine_proprietary_codecs=ON -DFEATURE_webengine_jumbo_build=OFF `
            -DFEATURE_qtpdf_build=OFF -DFEATURE_qtwebengine_widgets_build=OFF
        if ($LASTEXITCODE -ne 0) { throw 'configuration CMake de Qt WebEngine en échec' }
    }
    & cmake --build $construction --parallel $Taches
    if ($LASTEXITCODE -ne 0) {
        throw "construction de Qt WebEngine en échec (reprendre : même commande ; manque de mémoire : supprimer $construction\CMakeCache.txt et -Taches $([Math]::Max(1, [int]($Taches / 2))))"
    }
    & cmake --install $construction
    if ($LASTEXITCODE -ne 0) { throw 'installation de Qt WebEngine en échec' }

    $pri = Join-Path $Cible 'mkspecs/modules/qt_lib_webenginecore_private.pri'
    $actives = Select-String -LiteralPath $pri -Pattern '^QT\.webenginecore_private\.enabled_features\s*=' -ErrorAction SilentlyContinue
    if (-not $actives -or $actives.Line -notmatch '(^|\s)webengine-proprietary-codecs(\s|$)') {
        throw "Qt WebEngine installé sans codecs propriétaires ($pri)"
    }
    foreach ($fichier in 'resources/qtwebengine_resources.pak', 'translations/qtwebengine_locales/en-US.pak') {
        if (-not (Test-Path (Join-Path $Cible $fichier))) { throw "$fichier absent de $Cible après installation" }
    }
    # La copie avait déjà le Qt WebEngine officiel : les binaires du moteur doivent différer de ceux de $QtSource.
    foreach ($fichier in 'bin/Qt6WebEngineCore.dll', 'bin/Qt6WebEngineQuick.dll', 'bin/QtWebEngineProcess.exe') {
        $chemin = Join-Path $Cible $fichier
        if (-not (Test-Path $chemin)) { throw "$fichier absent de $Cible après installation" }
        $officiel = Join-Path $QtSource $fichier
        if ((Test-Path $officiel) -and (Get-FileHash $chemin).Hash -eq (Get-FileHash $officiel).Hash) {
            throw "$fichier identique à celui du Qt officiel : Qt WebEngine non remplacé par l'installation"
        }
    }
    # find_package(Qt6 6.12 … WebEngineQuick) d'ActCraft exige que le module annonce la version de qtbase (vrai chez
    # Homebrew : Qt WebEngine 6.11.2 sur qtbase 6.12.0 annonce 6.12.0).
    $versionCmake = Join-Path $Cible 'lib/cmake/Qt6WebEngineQuick/Qt6WebEngineQuickConfigVersionImpl.cmake'
    $annoncee = (Select-String -LiteralPath $versionCmake -Pattern '^set\(PACKAGE_VERSION "([0-9.]+)"\)' -ErrorAction SilentlyContinue).Matches.Groups[1].Value
    $versionQt = Split-Path (Split-Path $QtSource -Parent) -Leaf
    if ($annoncee -ne $versionQt) { throw "Qt WebEngine annonce la version « $annoncee », pas $versionQt : find_package d'ActCraft le refuserait" }
    $duree = (Get-Date) - $debut
    Set-Content -LiteralPath (Join-Path $Cible '.actcraft-h264') -Encoding utf8 -Value @(
        "qtwebengine=$versionWebEngine", "sha256=$somme", "qt=$QtSource", "taches=$Taches",
        "fin=$((Get-Date).ToString('s'))", ("duree_min={0:N0}" -f $duree.TotalMinutes))
    "Durée : {0:N0} min ; dossier de construction : {1:N1} Go" -f $duree.TotalMinutes,
        ((Get-ChildItem -Recurse -File $construction | Measure-Object Length -Sum).Sum / 1GB)
    Write-Output "QTWEBENGINE H264 OK : $Cible"
} catch {
    Write-Output "QTWEBENGINE H264 EN ECHEC : $($_.Exception.Message)"
    exit 1
}
