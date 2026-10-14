# Compare les bibliothèques Qt d'un dossier déployé ou d'un APK à packaging/qt-autorises.txt (décision 34).
# Usage : pwsh -NoProfile -File ./scripts/verifier-licences.ps1 -Dossier <dossier> | -Apk <fichier.apk>
# Dernière ligne : « LICENCES OK » (0) ou « LICENCES EN ECHEC : <modules> » (1).
param([string]$Dossier, [string]$Apk)
$ErrorActionPreference = 'Stop'

$liste = Join-Path $PSScriptRoot '../packaging/qt-autorises.txt'
$autorises = @(Get-Content $liste | ForEach-Object { $_.Trim() } | Where-Object { $_ -and -not $_.StartsWith('#') })

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

# Qt6Core.dll (Windows), libQt6Core_arm64-v8a.so (Android) → Qt6Core. Les greffons (qwindows.dll…) suivent leur
# module et ne sont pas comptés.
$modules = @($noms | ForEach-Object {
        if ($_ -match '^(?:lib)?(Qt6[A-Za-z0-9]+?)(?:_[A-Za-z0-9_-]+)?\.(?:dll|so)$') { $Matches[1] }
    } | Sort-Object -Unique)
if ($modules.Count -eq 0) {
    Write-Output 'LICENCES EN ECHEC : aucune bibliothèque Qt trouvée'
    exit 1
}
Write-Output "Modules Qt : $($modules -join ', ')"
$refuses = @($modules | Where-Object { $autorises -notcontains $_ })
if ($refuses.Count -gt 0) {
    Write-Output "LICENCES EN ECHEC : $($refuses -join ', ')"
    exit 1
}
Write-Output 'LICENCES OK'
