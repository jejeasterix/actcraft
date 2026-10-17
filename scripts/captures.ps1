# Preuve visuelle (DA8, décision 36) : joue des scénarios d'interface et écrit captures PNG et index.md.
# Usage : pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario <nom>|tous [-Rendu d3d11|logiciel] [-HorsEcran]
#         -Sortie <dossier> [-Lire "<objet>.<propriété>=<attendu>[;…]"] [-Taille LxH|plein-ecran]
#         [-Preset debug|release] [-SansConstruire] [-Ouvrir <fichier>]
# Scénarios : bench/scenarios/<nom>.json ; « tous » ne prend pas ceux de bench/scenarios/local/ (paperboards du
# poste : -Scenario local/<nom>). Sorties sous .tmp/ ou build/, jamais commitées.
# -Ouvrir <fichier> : la fenêtre s'ouvre sur ce fichier (--ouvrir d'actcraft-captures, lu au lancement comme par
# actcraft.exe) ; un paperboard réel est lu en place, jamais copié.
# Dernière ligne et code : « CAPTURES OK » (0) ou « CAPTURES EN ECHEC : <raison> » (1), y compris sur une
# erreur imprévue et quand aucun scénario n'est trouvé.
param(
    [string]$Scenario = 'tous',
    [ValidateSet('d3d11', 'logiciel')][string]$Rendu = 'logiciel',
    [switch]$HorsEcran,
    [Parameter(Mandatory)][string]$Sortie,
    [string]$Lire = '',
    [string]$Taille = '1280x800',
    [ValidateSet('debug', 'release')][string]$Preset = 'debug',
    [switch]$SansConstruire,
    [string]$Ouvrir = ''
)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

function Stop-Captures([string]$raison) {
    Write-Output "CAPTURES EN ECHEC : $raison"
    exit 1
}

try {
    if (-not $SansConstruire) {
        & "$PSScriptRoot/build.ps1" -Preset $Preset
        if ($LASTEXITCODE -ne 0) { Stop-Captures 'compilation' }
    }
    . "$PSScriptRoot/dev-env.ps1"

    $exe = Join-Path (Get-Location) "build/$Preset/actcraft-captures.exe"
    if (-not (Test-Path $exe)) { Stop-Captures "$exe introuvable" }
    # Stop-Captures jamais dans une expression capturée (@(…), $x = …) : sa ligne y serait avalée.
    if ($Scenario -eq 'tous') {
        $fichiers = @(Get-ChildItem bench/scenarios/*.json)
    } else {
        $chemin = "bench/scenarios/$Scenario.json"
        if (-not (Test-Path $chemin)) { Stop-Captures "scénario inconnu « $Scenario » ($chemin)" }
        $fichiers = @(Get-Item $chemin)
    }
    if ($fichiers.Count -eq 0) { Stop-Captures "aucun scénario « $Scenario » dans bench/scenarios" }

    if ($HorsEcran) {
        $env:QT_QPA_PLATFORM = 'offscreen'
        # Hors écran, Qt n'a aucune police sous Windows : sans ceci, tous les textes sont des carrés vides.
        # Ailleurs, WINDIR n'existe pas et la plateforme trouve ses polices seule (comme tests/CMakeLists.txt).
        if ($IsWindows -and -not $env:QT_QPA_FONTDIR) { $env:QT_QPA_FONTDIR = Join-Path $env:WINDIR 'Fonts' }
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
        if ($Ouvrir) { $arguments += @('--ouvrir', $Ouvrir) }
        & $exe @arguments
        if ($LASTEXITCODE -ne 0) { $echecs++ }
    }
} catch {
    Stop-Captures "erreur : $($_.Exception.Message)"
}
if ($echecs -gt 0) { Stop-Captures "$echecs scénario(s) sur $($fichiers.Count)" }
Write-Output 'CAPTURES OK'
