# Banc de fluidité : construit en release, puis joue tracé, zoom et défilement (D3D11 puis logiciel).
# Usage : pwsh -NoProfile -File ./scripts/bench.ps1 [-Rendu d3d11|logiciel|tous] [-SansCache] [-Captures <dossier>]
#         [-Scenario complet|court] [-Taille plein-ecran|LxH] [-HorsEcran] [-SansConstruire]
# Résultats : bench/results/<horodatage>-<modèle>-<rendu>.json (ignoré par git).
# Dernière ligne et code de sortie : verdict du rendu par défaut (d3d11) — BUDGET TENU (0), BUDGET DEPASSE (1),
# MESURE INVALIDE (2). Le rendu logiciel est informatif (N1-3).
param(
    [ValidateSet('d3d11', 'logiciel', 'tous')][string]$Rendu = 'tous',
    [switch]$SansCache,
    [string]$Captures,
    [ValidateSet('complet', 'court')][string]$Scenario = 'complet',
    [string]$Taille = 'plein-ecran',
    [switch]$HorsEcran,
    [switch]$SansConstruire
)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

if (-not $SansConstruire) {
    & "$PSScriptRoot/build.ps1" -Preset release
    if ($LASTEXITCODE -ne 0) { Write-Output 'MESURE INVALIDE (compilation)'; exit 2 }
}
. "$PSScriptRoot/dev-env.ps1"

$exe = Join-Path (Get-Location) 'build/release/actcraft-bench.exe'
if (-not (Test-Path $exe)) { Write-Output "MESURE INVALIDE ($exe introuvable)"; exit 2 }
$resultats = Join-Path (Get-Location) 'bench/results'
New-Item -ItemType Directory -Force $resultats | Out-Null

if ($HorsEcran) {
    $env:QT_QPA_PLATFORM = 'offscreen'
} elseif ($Taille -eq 'plein-ecran') {
    Write-Output 'La fenêtre du banc couvre l''écran principal pendant 30 à 60 s par rendu.'
}

$rendus = @(if ($Rendu -eq 'tous') { 'd3d11'; 'logiciel' } else { $Rendu })
$codes = @{}
foreach ($r in $rendus) {
    $arguments = @('--rendu', $r, '--scenario', $Scenario, '--taille', $Taille, '--sortie', "$resultats/")
    if ($SansCache) { $arguments += '--sans-cache' }
    if ($Captures) {
        New-Item -ItemType Directory -Force $Captures | Out-Null
        $arguments += @('--captures', (Resolve-Path $Captures).Path)
    }
    $code = 2
    for ($essai = 1; $essai -le 2; $essai++) {
        & $exe @arguments
        $code = $LASTEXITCODE
        if ($code -ne 2) { break }
        if ($essai -eq 1) { Write-Output "Passage $r invalide : nouvel essai." }
    }
    $codes[$r] = $code
}

$final = if ($codes.ContainsKey('d3d11')) { $codes['d3d11'] } else { $codes[$rendus[0]] }
$verdict = switch ($final) { 0 { 'BUDGET TENU' } 1 { 'BUDGET DEPASSE' } default { 'MESURE INVALIDE' } }
Write-Output $verdict
exit $final
