# Installateur Windows (décision 26, DA9) : construction release, déploiement Qt, runtime MSVC local, licences,
# compilation Inno Setup. Usage : pwsh -NoProfile -File ./scripts/paquet.ps1 [-SansConstruire]
# Produit build/paquet/ActCraft-<version>-setup.exe : local, jamais publié (publication = go écrit de Jérôme).
# Dernière ligne : « PAQUET OK : <chemin> » ; toute erreur arrête le script (exception, code 1).
param([switch]$SansConstruire)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
. "$PSScriptRoot/version-projet.ps1"
$version = Get-ActCraftVersion

if (-not $SansConstruire) {
    & "$PSScriptRoot/build.ps1" -Preset release
    if ($LASTEXITCODE -ne 0) { throw 'compilation release en échec' }
}
. "$PSScriptRoot/dev-env.ps1"
if (-not $env:VCToolsRedistDir) { throw 'VCToolsRedistDir absent : environnement Visual Studio requis (dev-env.ps1).' }

$candidats = @($env:ACTCRAFT_ISCC,
    (Join-Path $env:LOCALAPPDATA 'Programs/Inno Setup 6/ISCC.exe'),
    (Join-Path $HOME '.tools/InnoSetup6/ISCC.exe'),
    (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6/ISCC.exe'),
    (Join-Path $env:ProgramFiles 'Inno Setup 6/ISCC.exe'))
$iscc = $candidats | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
if (-not $iscc) { $iscc = (Get-Command iscc -ErrorAction SilentlyContinue).Source }
if (-not $iscc) { throw 'ISCC.exe introuvable : Inno Setup 6 (docs/verifications/2026-10-14-outillage-paquets.md) ou ACTCRAFT_ISCC.' }

$racine = Join-Path (Get-Location) 'build/paquet'
$stage = Join-Path $racine 'windows'
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item build/release/actcraft.exe $stage

$windeployqt = Join-Path $env:ACTCRAFT_QT_DIR 'bin/windeployqt.exe'
& $windeployqt --release --no-compiler-runtime --no-translations --qmldir ui/qml (Join-Path $stage 'actcraft.exe')
if ($LASTEXITCODE -ne 0) { throw 'windeployqt en échec' }
Get-ChildItem -Recurse -Filter *.pdb $stage | Remove-Item -Force # N2-7 : pas de PDB dans l'installateur

# Runtime MSVC à côté de l'exécutable : vc_redist exigerait l'administrateur.
$crt = Join-Path $env:VCToolsRedistDir 'x64\Microsoft.VC143.CRT'
foreach ($dll in 'vcruntime140.dll', 'vcruntime140_1.dll', 'msvcp140.dll') { Copy-Item (Join-Path $crt $dll) $stage }
Copy-Item LICENSE (Join-Path $stage 'LICENSE.txt')
Copy-Item THIRD_PARTY_NOTICES.md $stage
Copy-Item -Recurse LICENSES (Join-Path $stage 'LICENSES')

& "$PSScriptRoot/verifier-licences.ps1" -Dossier $stage
if ($LASTEXITCODE -ne 0) { throw 'licences : bibliothèque Qt hors de packaging/qt-autorises.txt' }

& $iscc /Qp "/DAppVersion=$version" "/DSourceDir=$stage" "/DOutputDir=$racine" packaging/windows/actcraft.iss
if ($LASTEXITCODE -ne 0) { throw 'ISCC en échec' }
$setup = Join-Path $racine "ActCraft-$version-setup.exe"
if (-not (Test-Path $setup)) { throw "$setup absent" }
Write-Output ""   # ISCC /Qp ne termine pas sa ligne
Write-Output "PAQUET OK : $setup"
