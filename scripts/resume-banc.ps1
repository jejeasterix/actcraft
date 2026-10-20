# Résumé chiffré de passages du banc, en Markdown (S3 ; docs/mesures/).
# Usage : pwsh -NoProfile -File ./scripts/resume-banc.ps1 -Entrees <fichier.json|dossier>[,...] [-Sortie <fichier.md>]
# Dernière ligne de console : « RESUME OK (<n> fichiers) ». Aucun nom d'hôte : le JSON n'en contient pas et le
# script ne lit jamais l'environnement de la machine.
param(
    [Parameter(Mandatory)][string[]]$Entrees,
    [string]$Sortie
)
$ErrorActionPreference = 'Stop'

$fichiers = @(foreach ($e in $Entrees) {
        if (Test-Path $e -PathType Container) { Get-ChildItem $e -Filter *.json | Sort-Object Name }
        else { Get-Item $e }
    })
if ($fichiers.Count -eq 0) { throw 'aucun fichier JSON' }

$inv = [cultureinfo]::InvariantCulture
function Nombre($valeur, [int]$decimales = 1) {
    if ($null -eq $valeur) { return '—' }
    return ([double]$valeur).ToString("F$decimales", $inv)
}

$lignes = [System.Collections.Generic.List[string]]::new()
$lignes.Add('# Résumé du banc de fluidité')
$lignes.Add('')
$lignes.Add('| Rendu | Cache | Scénario | Images/s | p95 intervalle (ms) | > 25 ms | Figements / reprises > limite | Latence p50 / p95 (ms) | Pic mémoire (Mo) | Verdict | Valide |')
$lignes.Add('|---|---|---|---|---|---|---|---|---|---|---|')
$sources = [System.Collections.Generic.List[string]]::new()
foreach ($f in $fichiers) {
    $j = Get-Content $f.FullName -Raw -Encoding utf8 | ConvertFrom-Json
    if ($j.format -ne 'actcraft-bench/1') { throw "$($f.Name) : format inconnu ($($j.format))" }
    $verdict = if (-not $j.valid_for_measurement) { 'invalide' } elseif ($j.within_budget) { 'tenu' } else { 'dépassé' }
    if ($j.informative) { $verdict += ' (informatif)' }
    $cache = if ($j.cache) { 'oui' } else { 'non' }
    $valide = if ($j.valid_for_measurement) { 'oui' } else { 'non' }
    foreach ($s in $j.scenarios) {
        $latence = if ($s.latency) { "$(Nombre $s.latency.total_ms.p50) / $(Nombre $s.latency.total_ms.p95)" } else { '—' }
        $gpu = if ($null -ne $s.memory.gpu_peak_bytes) { [double]$s.memory.gpu_peak_bytes } else { 0.0 }
        $memoire = ([double]$s.memory.process_private_peak_bytes + $gpu) / 1MB
        $part = 100.0 * [double]$s.frames.over_25ms_ratio
        $reprises = "$($s.frames.freeze_over_limit) / $($s.frames.resume_over_limit)"
        $lignes.Add("| $($j.graphics_api) | $cache | $($s.name) | $(Nombre $s.frames.fps) | $(Nombre $s.frames.p95_ms) | $(Nombre $part 2) % | $reprises | $latence | $(Nombre $memoire 0) | $verdict | $valide |")
    }
    $m = $j.meta
    $ligne = "- ``$($f.Name)`` : $($j.graphics_api), cache $cache, $($m.machine_model), $($m.cpu), puce $($m.gpu), écran $($m.screen.width) × $($m.screen.height) à $(Nombre $m.screen.refresh_hz 0) Hz, fenêtre $($m.window.mode), boucle $($m.render_loop), construction $($m.build_type), commit $($m.commit)"
    $raisons = @($j.verdict.invalid_reasons)
    if ($null -ne $j.content_check.page_flash) {
        # Scénario « page » (L2-T23) : éclairs blancs au changement de page, relevés du zoom posé sur la page peinte.
        $ligne += ", page : $($j.content_check.page_flash) éclair(s) sur $($j.content_check.page_flash_probes) capture(s) sans cache, zoom posé : $($j.content_check.page_rebuilds_during_zoom) reconstruction(s), $($j.content_check.page_rasterizations_during_zoom) nœud(s) rastérisé(s)"
    }
    if ($raisons.Count -gt 0) { $ligne += ", raisons d'invalidité : $($raisons -join ', ')" }
    $depasses = @($j.verdict.criteria | Where-Object { -not $_.ok } | ForEach-Object { "$($_.scenario) $($_.criterion) = $(Nombre $_.value 2) (seuil $(Nombre $_.limit 2))" })
    if ($depasses.Count -gt 0) { $ligne += ", critères dépassés : $($depasses -join ' ; ')" }
    $exclus = @($j.verdict.criteria | Where-Object { $_.excluded } | ForEach-Object { "$($_.scenario) $($_.criterion) = $(Nombre $_.value 0)" })
    if ($exclus.Count -gt 0) { $ligne += ", hors critère : $($exclus -join ' ; ')" }
    $sources.Add($ligne)
}
$lignes.Add('')
$lignes.Add('Fichiers :')
$lignes.Add('')
foreach ($s in $sources) { $lignes.Add($s) }

$texte = ($lignes -join "`n") + "`n"
if ($Sortie) {
    $dossier = Split-Path -Parent ([System.IO.Path]::GetFullPath($Sortie, (Get-Location).Path))
    New-Item -ItemType Directory -Force $dossier | Out-Null
    [System.IO.File]::WriteAllText($Sortie, $texte, [System.Text.UTF8Encoding]::new($false))
} else {
    Write-Output $texte
}
Write-Output "RESUME OK ($($fichiers.Count) fichiers)"
