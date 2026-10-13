# Paquet du banc pour une machine sans outils de construction (Dell 3510, étape 2 du BRIEF).
# Usage : pwsh -NoProfile -File ./scripts/paquet-banc.ps1
# Produit build/paquet-banc/actcraft-bench-<commit>.zip : local, jamais publié (pas de distribution avant le go).
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

& "$PSScriptRoot/build.ps1" -Preset release
if ($LASTEXITCODE -ne 0) { throw 'compilation release en échec' }
. "$PSScriptRoot/dev-env.ps1"
if (-not $env:VCToolsRedistDir) { throw 'VCToolsRedistDir absent : ouvrir un environnement Visual Studio (dev-env.ps1).' }

$commit = (git rev-parse --short=12 HEAD).Trim()
$racine = Join-Path (Get-Location) 'build/paquet-banc'
$dossier = Join-Path $racine 'actcraft-bench'
if (Test-Path $dossier) { Remove-Item -Recurse -Force $dossier }
New-Item -ItemType Directory -Force $dossier | Out-Null
Copy-Item build/release/actcraft-bench.exe $dossier

$windeployqt = Join-Path $env:ACTCRAFT_QT_DIR 'bin/windeployqt.exe'
& $windeployqt --release --no-compiler-runtime --qmldir ui/qml (Join-Path $dossier 'actcraft-bench.exe')
if ($LASTEXITCODE -ne 0) { throw 'windeployqt en échec' }

# Bibliothèques d'exécution de MSVC à côté de l'exécutable : --compiler-runtime livrerait l'installeur vc_redist,
# qui demande les droits d'administrateur.
$crt = Join-Path $env:VCToolsRedistDir 'x64\Microsoft.VC143.CRT'
foreach ($dll in 'vcruntime140.dll', 'vcruntime140_1.dll', 'msvcp140.dll') {
    Copy-Item (Join-Path $crt $dll) $dossier
}
if (Test-Path docs/banc.md) { Copy-Item docs/banc.md (Join-Path $dossier 'LISEZMOI-banc.md') }

$zip = Join-Path $racine "actcraft-bench-$commit.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path $dossier -DestinationPath $zip
Write-Output "Paquet : $zip"
