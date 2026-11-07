# Mesure du banc sur un appareil Android (L11-T15, S3.5) : installe l'APK de banc, le lance, attend sa fin, rapatrie le
# JSON par « adb pull » (dossier propre externe de l'application : lisible sans « run-as », que refuse un APK release),
# écrit « MESURE INDICATIVE (émulateur) » si l'appareil est un émulateur (ro.kernel.qemu=1), puis le verdict du banc
# lu dans logcat (BUDGET TENU, BUDGET DEPASSE, MESURE INVALIDE). Désinstalle l'APK de banc à la fin.
# Usage : pwsh -NoProfile -File ./scripts/banc-android.ps1 -Apk <fichier.apk> [-Serie <numéro adb>]
#         [-Scenario court|complet] [-Sortie <dossier>]
# Appels adb : scripts/android-commun.ps1. Une mesure d'émulateur n'est jamais une mesure de tablette.
# Dernière ligne : le verdict du banc (0 : BUDGET TENU), ou « BANC ANDROID EN ECHEC : <raison> » (1).
param(
    [Parameter(Mandatory)][string]$Apk,
    [string]$Serie = '',
    [ValidateSet('court', 'complet')][string]$Scenario = 'complet',
    [string]$Sortie = '.tmp/banc-tablette'
)
$ErrorActionPreference = 'Stop'
$Apk = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Apk)
$racine = Split-Path -Parent $PSScriptRoot
Set-Location $racine
$Sortie = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Sortie)
. "$PSScriptRoot/android-commun.ps1"
$paquet = 'io.github.jejeasterix.actcraft.banc'
$dossierAppareil = "/sdcard/Android/data/$paquet/files/bench-results"
$installe = $false
$verdict = $null
try {
    Initialize-ActCraftAdb
    if ($Serie) { $env:ANDROID_SERIAL = $Serie }
    Wait-ActCraftDemarrage
    $emulateur = "$(Adb 15 shell getprop ro.kernel.qemu)".Trim() -eq '1'
    if (-not (Test-Path -LiteralPath $Apk)) { Lever-ActCraftAndroid "APK introuvable : $Apk" }

    $null = Adb 60 uninstall $paquet # état initial : pas de résultats d'un passage précédent
    Adb 240 install -r $Apk | ForEach-Object { Write-Host "install : $_" }
    if ($LASTEXITCODE -ne 0) { Lever-ActCraftAndroid "adb install$(Get-ActCraftAdbErreur)" }
    $installe = $true
    $null = Adb 30 logcat -c
    # Options en une seule chaîne (applicationArguments) : le banc la découpe lui-même (expandSingleStringArguments).
    $null = Adb 60 shell am start -n "$paquet/org.qtproject.qt.android.bindings.QtActivity" -e applicationArguments "--scenario $Scenario"
    if ($LASTEXITCODE -ne 0) { Lever-ActCraftAndroid "adb am start$(Get-ActCraftAdbErreur)" }
    Start-Sleep -Seconds 10
    if (-not "$(Adb 30 shell pidof $paquet)".Trim()) { Lever-ActCraftAndroid 'banc absent 10 s après le lancement' }

    # Attente bornée de la fin : le banc se termine seul (complet : environ 3 min ; court : environ 1 min).
    $limite = (Get-Date).AddMinutes($(if ($Scenario -eq 'complet') { 15 } else { 8 }))
    while ("$(Adb 30 shell pidof $paquet)".Trim()) {
        if ((Get-Date) -gt $limite) { Lever-ActCraftAndroid 'le banc ne s''est pas terminé dans le délai' }
        Ecarter-Anr
        Start-Sleep -Seconds 5
    }

    $journal = @(Adb 60 logcat -d)
    New-Item -ItemType Directory -Force $Sortie | Out-Null
    $journal | Set-Content -Path (Join-Path $Sortie 'logcat.txt')
    $null = Adb 120 pull $dossierAppareil $Sortie
    if ($LASTEXITCODE -ne 0) { Lever-ActCraftAndroid "adb pull$(Get-ActCraftAdbErreur)" }
    $json = @(Get-ChildItem -Recurse -Filter '*.json' $Sortie)
    if ($json.Count -eq 0) { Lever-ActCraftAndroid "aucun JSON rapatrié dans $Sortie" }
    foreach ($f in $json) { Write-Host "JSON : $($f.FullName)" }
    $lignes = @($journal | Select-String -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE' | Select-Object -Last 1)
    if ($lignes.Count -eq 0) { Lever-ActCraftAndroid 'aucun verdict du banc dans logcat' }
    $verdict = ($lignes[0].Line | Select-String -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE').Matches[0].Value
    if ($emulateur) { Write-Output 'MESURE INDICATIVE (émulateur)' }
} catch {
    $raison = Get-ActCraftAndroidRaison $_
    if (-not $raison) { $raison = "erreur imprévue : $($_.Exception.Message)" }
    $verdict = $null
    Write-Output "BANC ANDROID EN ECHEC : $raison"
    exit 1
} finally {
    if ($installe) { try { $null = Adb 60 uninstall $paquet } catch { $null = $_ } }
}
Write-Output $verdict
if ($verdict -eq 'BUDGET TENU') { exit 0 }
exit 1
