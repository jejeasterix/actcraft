# Compile puis lance tous les tests, et vérifie le format du code.
# Usage : pwsh -NoProfile -File ./scripts/test.ps1 [-Preset debug|release]
# Dernière ligne : « TESTS OK » ou « TESTS EN ECHEC » (motif lu par la chaîne nouvelle-feature).
param([ValidateSet('debug', 'release')][string]$Preset = 'debug')
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

& "$PSScriptRoot/build.ps1" -Preset $Preset
if ($LASTEXITCODE -ne 0) { Write-Output 'TESTS EN ECHEC (compilation)'; exit 1 }
. "$PSScriptRoot/dev-env.ps1"

ctest --preset $Preset
$ctestCode = $LASTEXITCODE

& "$PSScriptRoot/format.ps1" -Check
$formatCode = $LASTEXITCODE

if ($ctestCode -ne 0 -or $formatCode -ne 0) { Write-Output 'TESTS EN ECHEC'; exit 1 }
Write-Output 'TESTS OK'
