# Compare les bibliothèques d'un dossier déployé ou d'un APK aux listes de packaging/ (décision 34, L11-T25) :
# - Qt6*.dll, libQt6*.so : nom du module dans packaging/qt-autorises.txt (un nom par ligne) ; un nom Qt6… qui
#   n'a pas la forme d'un module est refusé ;
# - toute autre *.dll ou *.so (FFmpeg, runtime MSVC, greffons de Qt, libc++…) : un motif de
#   packaging/autres-autorises.txt, au format « <motif glob> <expression SPDX> » (amendement 3 du plan 11b) ;
# - une bibliothèque rangée sous LGPL-3.0-only (greffon de Qt) doit venir de l'installation de Qt : même chemin
#   relatif sous <Qt>/plugins ou <Qt>/qml (dossier), même nom sous <Qt>/plugins ou <Qt>/qml (APK, chemins perdus) ;
# - une bibliothèque FFmpeg doit porter la licence compilée « LGPL version 2.1 or later » (ni GPL, ni « nonfree »).
# Usage : pwsh -NoProfile -File ./scripts/verifier-licences.ps1 -Dossier <dossier> | -Apk <fichier.apk> [-QtDir <Qt>]
#   <Qt> par défaut : $env:ACTCRAFT_QT_DIR (dossier), $env:ACTCRAFT_QT_ANDROID_ARM64_DIR (APK).
# Dernière ligne : « LICENCES OK » (0) ou « LICENCES EN ECHEC : <noms> » (1).
param([string]$Dossier, [string]$Apk, [string]$QtDir)
$ErrorActionPreference = 'Stop'

# Appelée hors de toute affectation : son message va sur la sortie du script (jamais capturé par une variable).
function Stop-Echec([string]$Motif) {
    Write-Output "LICENCES EN ECHEC : $Motif"
    exit 1
}

function Read-Liste([string]$Nom) {
    $chemin = Join-Path $PSScriptRoot "../packaging/$Nom"
    @(Get-Content $chemin | ForEach-Object { $_.Trim() } | Where-Object { $_ -and -not $_.StartsWith('#') })
}

$autorises = Read-Liste 'qt-autorises.txt'

# Le PREMIER blanc sépare le motif de l'expression SPDX, qui peut contenir des blancs (« FTL OR GPL-2.0-or-later ») :
# ni égalité exacte sur la ligne entière (forme de qt-autorises.txt), ni découpage à chaque blanc.
$autres = [System.Collections.Generic.List[object]]::new()
foreach ($ligne in (Read-Liste 'autres-autorises.txt')) {
    $blanc = $ligne.IndexOfAny([char[]]@(' ', "`t"))
    if ($blanc -lt 1) { Stop-Echec "ligne sans licence dans autres-autorises.txt : $ligne" }
    $autres.Add([pscustomobject]@{ Motif = $ligne.Substring(0, $blanc); Licence = $ligne.Substring($blanc).Trim() })
}

# Fichiers du paquet : nom, chemin relatif (dossier seulement), et de quoi relire leurs octets.
$fichiers = [System.Collections.Generic.List[object]]::new()
$apkChemin = $null
if ($Apk) {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $apkChemin = (Resolve-Path $Apk).Path
    if (-not $QtDir) { $QtDir = $env:ACTCRAFT_QT_ANDROID_ARM64_DIR }
    $zip = [System.IO.Compression.ZipFile]::OpenRead($apkChemin)
    try {
        foreach ($entree in $zip.Entries) {
            if ($entree.Name) {
                $fichiers.Add([pscustomobject]@{ Nom = $entree.Name; Relatif = $null; Chemin = $null; Entree = $entree.FullName })
            }
        }
    } finally { $zip.Dispose() }
} elseif ($Dossier) {
    $racine = (Resolve-Path $Dossier).Path
    if (-not $QtDir) { $QtDir = $env:ACTCRAFT_QT_DIR }
    foreach ($fichier in (Get-ChildItem -Recurse -File $racine)) {
        $relatif = [System.IO.Path]::GetRelativePath($racine, $fichier.FullName) -replace '\\', '/'
        $fichiers.Add([pscustomobject]@{ Nom = $fichier.Name; Relatif = $relatif; Chemin = $fichier.FullName; Entree = $null })
    }
} else {
    Stop-Echec 'ni -Dossier ni -Apk'
}

# Octets d'un fichier lus en Latin-1 (un caractère par octet) : chaînes ASCII compilées dans les bibliothèques.
function Read-Texte($fichier) {
    if ($fichier.Chemin) { return [System.Text.Encoding]::Latin1.GetString([System.IO.File]::ReadAllBytes($fichier.Chemin)) }
    $zip = [System.IO.Compression.ZipFile]::OpenRead($apkChemin)
    try {
        $flux = $zip.GetEntry($fichier.Entree).Open()
        $memoire = [System.IO.MemoryStream]::new()
        try { $flux.CopyTo($memoire) } finally { $flux.Dispose() }
        return [System.Text.Encoding]::Latin1.GetString($memoire.ToArray())
    } finally { $zip.Dispose() }
}

# Greffons de l'installation de Qt : chemins relatifs sous plugins/ et qml/, et noms (pour l'APK). Lus une fois.
$script:qtGreffons = $null
function Test-Provenance($fichier) {
    if (-not $QtDir -or -not (Test-Path -LiteralPath $QtDir)) { return $false }
    if ($null -eq $script:qtGreffons) {
        $script:qtGreffons = @{ Chemins = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
            Noms = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase) }
        foreach ($sous in 'plugins', 'qml') {
            $base = Join-Path $QtDir $sous
            if (-not (Test-Path -LiteralPath $base)) { continue }
            foreach ($greffon in (Get-ChildItem -Recurse -File $base -Include '*.dll', '*.so')) {
                $relatif = [System.IO.Path]::GetRelativePath($base, $greffon.FullName) -replace '\\', '/'
                # windeployqt range plugins/<type>/x.dll sous <type>/x.dll, et qml/<module>/x.dll sous qml/<module>/x.dll.
                [void]$script:qtGreffons.Chemins.Add($(if ($sous -eq 'qml') { "qml/$relatif" } else { $relatif }))
                [void]$script:qtGreffons.Noms.Add($greffon.Name)
            }
        }
    }
    if ($fichier.Relatif) { return $script:qtGreffons.Chemins.Contains($fichier.Relatif) }
    return $script:qtGreffons.Noms.Contains($fichier.Nom)
}

$refuses = [System.Collections.Generic.List[string]]::new()

# Qt6Core.dll (Windows), libQt6Core_arm64-v8a.so (Android) → Qt6Core.
$modules = [System.Collections.Generic.List[string]]::new()
$qtNoms = @($fichiers | Where-Object { $_.Nom -match '^(?:lib)?Qt6.*\.(?:dll|so)$' } | ForEach-Object { $_.Nom } | Sort-Object -Unique)
foreach ($nom in $qtNoms) {
    if ($nom -match '^(?:lib)?(Qt6[A-Za-z0-9]+?)(?:_[A-Za-z0-9_-]+)?\.(?:dll|so)$') {
        if (-not $modules.Contains($Matches[1])) { $modules.Add($Matches[1]) }
    } else {
        $refuses.Add("$nom (nom de module illisible)")
    }
}
if ($modules.Count -eq 0) { Stop-Echec 'aucune bibliothèque Qt trouvée' }
Write-Output "Modules Qt : $($modules -join ', ')"
foreach ($module in $modules) {
    if ($autorises -notcontains $module) { $refuses.Insert(0, $module) }
}

# Bibliothèques non Qt : *.dll hors Qt6*, *.so hors libQt6* ; motif glob (-like, sans casse comme Windows).
$retenues = [System.Collections.Generic.List[string]]::new()
foreach ($fichier in @($fichiers | Where-Object { $_.Nom -match '\.(?:dll|so)$' -and $_.Nom -notmatch '^(?:lib)?Qt6' })) {
    $trouvee = $null
    foreach ($ligne in $autres) {
        if ($fichier.Nom -like $ligne.Motif) { $trouvee = $ligne; break }
    }
    $nomAffiche = $(if ($fichier.Relatif) { $fichier.Relatif } else { $fichier.Nom })
    if (-not $trouvee) { $refuses.Add($nomAffiche); continue }
    if ($trouvee.Licence -eq 'LGPL-3.0-only' -and -not (Test-Provenance $fichier)) {
        $refuses.Add("$nomAffiche (absent des greffons de Qt : $QtDir)")
        continue
    }
    if ($fichier.Nom -match '^(?:lib)?(?:avcodec|avformat|avutil|swresample|swscale)') {
        $texte = Read-Texte $fichier
        if ($texte -cnotmatch 'LGPL version 2\.1 or later' -or $texte -cmatch '(?<!L)GPL version|nonfree and unredistributable|--enable-gpl|--enable-nonfree') {
            $refuses.Add("$nomAffiche (FFmpeg hors LGPL 2.1 ou ultérieure)")
            continue
        }
    }
    $retenues.Add("$nomAffiche ($($trouvee.Licence))")
}
if ($retenues.Count -gt 0) { Write-Output "Autres bibliothèques : $($retenues -join ', ')" }

if ($refuses.Count -gt 0) { Stop-Echec ($refuses -join ', ') }
Write-Output 'LICENCES OK'
