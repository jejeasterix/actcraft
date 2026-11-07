# Compare les bibliothèques d'un dossier déployé ou d'un APK aux listes de packaging/ (décision 34, L11-T25) :
# - Qt6*.dll, libQt6*.so : nom du module dans packaging/qt-autorises.txt (un nom par ligne) ;
# - toute autre *.dll ou *.so (FFmpeg, runtime MSVC, greffons de Qt, libc++…) : un motif de
#   packaging/autres-autorises.txt, au format « <motif glob> <expression SPDX> » (amendement 3 du plan 11b).
# Usage : pwsh -NoProfile -File ./scripts/verifier-licences.ps1 -Dossier <dossier> | -Apk <fichier.apk>
# Dernière ligne : « LICENCES OK » (0) ou « LICENCES EN ECHEC : <noms> » (1).
param([string]$Dossier, [string]$Apk)
$ErrorActionPreference = 'Stop'

function Read-Liste([string]$Nom) {
    $chemin = Join-Path $PSScriptRoot "../packaging/$Nom"
    @(Get-Content $chemin | ForEach-Object { $_.Trim() } | Where-Object { $_ -and -not $_.StartsWith('#') })
}

$autorises = Read-Liste 'qt-autorises.txt'

# Le PREMIER blanc sépare le motif de l'expression SPDX, qui peut contenir des blancs (« FTL OR GPL-2.0-or-later ») :
# ni égalité exacte sur la ligne entière (forme de qt-autorises.txt), ni découpage à chaque blanc.
$autres = @(Read-Liste 'autres-autorises.txt' | ForEach-Object {
        $blanc = $_.IndexOfAny([char[]]@(' ', "`t"))
        if ($blanc -lt 1) {
            Write-Output "LICENCES EN ECHEC : ligne sans licence dans autres-autorises.txt : $_"
            exit 1
        }
        [pscustomobject]@{ Motif = $_.Substring(0, $blanc); Licence = $_.Substring($blanc).Trim() }
    })

if ($Apk) {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [System.IO.Compression.ZipFile]::OpenRead((Resolve-Path $Apk).Path)
    try { $noms = @($zip.Entries | ForEach-Object { $_.Name }) } finally { $zip.Dispose() }
} elseif ($Dossier) {
    $noms = @(Get-ChildItem -Recurse -File $Dossier | ForEach-Object { $_.Name })
} else {
    Write-Output 'LICENCES EN ECHEC : ni -Dossier ni -Apk'
    exit 1
}

# Qt6Core.dll (Windows), libQt6Core_arm64-v8a.so (Android) → Qt6Core.
$modules = @($noms | ForEach-Object {
        if ($_ -match '^(?:lib)?(Qt6[A-Za-z0-9]+?)(?:_[A-Za-z0-9_-]+)?\.(?:dll|so)$') { $Matches[1] }
    } | Sort-Object -Unique)
if ($modules.Count -eq 0) {
    Write-Output 'LICENCES EN ECHEC : aucune bibliothèque Qt trouvée'
    exit 1
}
Write-Output "Modules Qt : $($modules -join ', ')"
$refuses = @($modules | Where-Object { $autorises -notcontains $_ })

# Bibliothèques non Qt : *.dll hors Qt6*, *.so hors libQt6* ; motif glob (-like, sans casse comme Windows).
$autresNoms = @($noms | Where-Object { $_ -match '\.(?:dll|so)$' -and $_ -notmatch '^(?:lib)?Qt6' } | Sort-Object -Unique)
$retenues = @()
foreach ($nom in $autresNoms) {
    $ligne = $autres | Where-Object { $nom -like $_.Motif } | Select-Object -First 1
    if ($ligne) { $retenues += "$nom ($($ligne.Licence))" } else { $refuses += $nom }
}
if ($retenues.Count -gt 0) { Write-Output "Autres bibliothèques : $($retenues -join ', ')" }

if ($refuses.Count -gt 0) {
    Write-Output "LICENCES EN ECHEC : $($refuses -join ', ')"
    exit 1
}
Write-Output 'LICENCES OK'
