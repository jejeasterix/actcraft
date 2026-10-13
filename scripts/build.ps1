# Configure et compile. Usage : pwsh -NoProfile -File ./scripts/build.ps1 [-Preset debug|release]
param([ValidateSet('debug', 'release')][string]$Preset = 'debug')
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
. "$PSScriptRoot/dev-env.ps1"

cmake --preset $Preset
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build --preset $Preset
exit $LASTEXITCODE
