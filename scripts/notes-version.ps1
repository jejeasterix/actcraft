# Notes d'une version tirées de CHANGELOG.md (section « ## <version> … », jusqu'à la section suivante) : corps de la
# release de cette version (décision 31).
# Usage : pwsh -NoProfile -File ./scripts/notes-version.ps1 -Version <x.y.z> -Sortie <fichier> [-Journal <fichier>]
# (-Journal : CHANGELOG.md du dépôt par défaut ; autre fichier pour les essais.)
# Dernière ligne et code : « NOTES OK : <fichier> » (0) ou « NOTES EN ECHEC : <raison> » (1) ; en échec, rien n'est
# écrit.
param(
    [Parameter(Mandatory)][string]$Version,
    [Parameter(Mandatory)][string]$Sortie,
    [string]$Journal = (Join-Path $PSScriptRoot '../CHANGELOG.md')
)
$ErrorActionPreference = 'Stop'
try {
    $lignes = @(Get-Content $Journal)
    $debut = -1
    for ($i = 0; $i -lt $lignes.Count; $i++) {
        if ($lignes[$i] -match "^## $([regex]::Escape($Version))(\s|$)") { $debut = $i; break }
    }
    if ($debut -lt 0) { throw "pas de section « ## $Version » dans $(Split-Path $Journal -Leaf)" }
    $fin = $lignes.Count
    for ($j = $debut + 1; $j -lt $lignes.Count; $j++) {
        if ($lignes[$j] -match '^## ') { $fin = $j; break }
    }
    $texte = if ($fin -gt $debut + 1) { ($lignes[($debut + 1)..($fin - 1)] -join "`n").Trim() } else { '' }
    if (-not $texte) { throw "section « ## $Version » vide" }
    [IO.File]::WriteAllText([IO.Path]::GetFullPath($Sortie, (Get-Location).Path), "$texte`n")
    Write-Output "NOTES OK : $Sortie"
} catch {
    Write-Output "NOTES EN ECHEC : $($_.Exception.Message)"
    exit 1
}
