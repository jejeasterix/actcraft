# Preuve visuelle (DA8, décision 36) : joue des scénarios d'interface et écrit captures PNG et index.md.
# Usage : pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario <nom>|tous [-Rendu d3d11|logiciel] [-HorsEcran]
#         -Sortie <dossier> [-Lire "<objet>.<propriété>=<attendu>[;…]"] [-Taille LxH|plein-ecran]
#         [-Preset debug|release] [-SansConstruire]
# Scénarios : bench/scenarios/<nom>.json. Sorties sous .tmp/ ou build/, jamais commitées.
# Dernière ligne et code : « CAPTURES OK » (0) ou « CAPTURES EN ECHEC » (1).
param(
    [string]$Scenario = 'tous',
    [ValidateSet('d3d11', 'logiciel')][string]$Rendu = 'logiciel',
    [switch]$HorsEcran,
    [Parameter(Mandatory)][string]$Sortie,
    [string]$Lire = '',
    [string]$Taille = '1280x800',
    [ValidateSet('debug', 'release')][string]$Preset = 'debug',
    [switch]$SansConstruire
)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

if (-not $SansConstruire) {
    & "$PSScriptRoot/build.ps1" -Preset $Preset
    if ($LASTEXITCODE -ne 0) { Write-Output 'CAPTURES EN ECHEC (compilation)'; exit 1 }
}
. "$PSScriptRoot/dev-env.ps1"

$exe = Join-Path (Get-Location) "build/$Preset/actcraft-captures.exe"
if (-not (Test-Path $exe)) { Write-Output "CAPTURES EN ECHEC ($exe introuvable)"; exit 1 }
$fichiers = @(if ($Scenario -eq 'tous') { Get-ChildItem bench/scenarios/*.json } else { Get-Item "bench/scenarios/$Scenario.json" })

if ($HorsEcran) {
    $env:QT_QPA_PLATFORM = 'offscreen'
    # Hors écran, Qt n'a aucune police sous Windows : sans ceci, tous les textes sont des carrés vides.
    if (-not $env:QT_QPA_FONTDIR) { $env:QT_QPA_FONTDIR = Join-Path $env:WINDIR 'Fonts' }
} else {
    Write-Output 'Les scénarios s''affichent sur l''écran principal (souris non pilotée).'
}
New-Item -ItemType Directory -Force $Sortie | Out-Null
$racine = (Resolve-Path $Sortie).Path
$lectures = @($Lire.Split(';', [System.StringSplitOptions]::RemoveEmptyEntries) | ForEach-Object { $_.Trim() })

$echecs = 0
foreach ($f in $fichiers) {
    $dossier = Join-Path $racine "$($f.BaseName)-$Rendu"
    $arguments = @('--scenario', $f.FullName, '--rendu', $Rendu, '--taille', $Taille, '--sortie', $dossier)
    foreach ($l in $lectures) { $arguments += @('--lire', $l) }
    & $exe @arguments
    if ($LASTEXITCODE -ne 0) { $echecs++ }
}
if ($echecs -gt 0) { Write-Output 'CAPTURES EN ECHEC'; exit 1 }
Write-Output 'CAPTURES OK'
