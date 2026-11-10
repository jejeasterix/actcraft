# Fonctions communes aux essais Android sur un appareil adb (essai-android.ps1 : S3 ; essai-android-fichiers.ps1 :
# S5) : appel borné d'adb, attente bornée du démarrage, installation et lancement de l'APK, boîte « Application Not
# Responding » du système écartée. Chargé par point : . "$PSScriptRoot/android-commun.ps1".
# Module autonome : il ne lit aucune variable ni fonction de ses appelants. Ses refus sont des exceptions marquées
# (Lever-ActCraftAndroid) : chaque appelant appelle Initialize-ActCraftAdb en tête de son try et traduit, dans son
# catch, la raison rendue par Get-ActCraftAndroidRaison par SA dernière ligne (« ANDROID EN ECHEC : … »,
# « ANDROID FICHIERS EN ECHEC : … »). Aucune fonction d'ici n'écrit de verdict ni n'appelle exit.

# Refus du module : exception marquée, pour que l'appelant rende la raison telle quelle (mêmes messages qu'avant
# l'extraction) et garde « erreur imprévue » pour le reste.
function Lever-ActCraftAndroid([string]$Raison, [switch]$DelaiAdb) {
    $exception = [System.Exception]::new($Raison)
    $exception.Data['ActCraftAndroid'] = $true
    if ($DelaiAdb) { $exception.Data['ActCraftAdbDelai'] = $true }
    throw $exception
}

# Raison d'un refus du module, ou $null pour toute autre erreur (à rendre comme « erreur imprévue »).
function Get-ActCraftAndroidRaison($Erreur) {
    $e = $Erreur.Exception
    while ($e) {
        if ($e.Data.Contains('ActCraftAndroid')) { return $e.Message }
        $e = $e.InnerException
    }
    return $null
}

# adb du PATH (dev-env-android.ps1, ou l'émulateur de la CI), gardé dans une variable propre au module.
function Initialize-ActCraftAdb {
    $script:ActCraftAdb = (Get-Command adb -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1).Source
    if (-not $script:ActCraftAdb) { Lever-ActCraftAndroid 'adb introuvable dans le PATH' }
}

# Adb <secondes> <arguments…> : lignes de la sortie standard, $LASTEXITCODE positionné, sortie d'erreur recueillie
# (Get-ActCraftAdbErreur). Au-delà du délai, l'arbre du processus adb est arrêté et l'essai échoue. Fonction simple
# ($args) : « -p », « -r »… restent des arguments d'adb.
function Adb {
    if (-not $script:ActCraftAdb) { Lever-ActCraftAndroid 'adb non initialisé (Initialize-ActCraftAdb)' }
    $script:ActCraftAdbErreur = ''
    $secondes = [int]$args[0]
    $arguments = @($args | Select-Object -Skip 1 | ForEach-Object { "$_" })
    $info = [Diagnostics.ProcessStartInfo]::new($script:ActCraftAdb)
    foreach ($a in $arguments) { $info.ArgumentList.Add($a) }
    $info.UseShellExecute = $false
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.StandardOutputEncoding = [Text.Encoding]::UTF8
    $info.StandardErrorEncoding = [Text.Encoding]::UTF8
    $p = [Diagnostics.Process]::Start($info)
    $sortie = $p.StandardOutput.ReadToEndAsync()
    $erreur = $p.StandardError.ReadToEndAsync()
    if (-not $p.WaitForExit($secondes * 1000)) {
        try { $p.Kill($true) } catch { $null = $_ }
        Lever-ActCraftAndroid "adb $(($arguments | Select-Object -First 2) -join ' ') sans réponse ($secondes s)" -DelaiAdb
    }
    $p.WaitForExit()
    $global:LASTEXITCODE = $p.ExitCode
    $script:ActCraftAdbErreur = (@($erreur.Result -split '\r?\n' | Where-Object { $_.Trim() -ne '' }) -join ' ').Trim()
    return @($sortie.Result -split '\r?\n' | Where-Object { $_ -ne '' })
}

# Sortie d'erreur du dernier appel Adb, prête à suivre une raison (« : … », 300 caractères au plus), ou ''.
function Get-ActCraftAdbErreur {
    $texte = "$script:ActCraftAdbErreur"
    if (-not $texte) { return '' }
    if ($texte.Length -gt 300) { $texte = $texte.Substring(0, 300) + '…' }
    return " : $texte"
}

# Boîte « Application Not Responding » du système (bouton « Wait », repéré par uiautomator) : touchée, trois fois au
# plus.
function Ecarter-Anr {
    for ($essai = 0; $essai -lt 3; $essai++) {
        if ("$(Adb 30 shell dumpsys window | Select-String 'mCurrentFocus')" -notmatch 'Application Not Responding') { break }
        $null = Adb 60 shell uiautomator dump /sdcard/ui.xml
        $xml = "$(Adb 30 shell cat /sdcard/ui.xml)"
        if ($xml -match 'text="Wait"[^>]*bounds="\[(\d+),(\d+)\]\[(\d+),(\d+)\]"') {
            $null = Adb 30 shell input tap ([int](([int]$Matches[1] + [int]$Matches[3]) / 2)) ([int](([int]$Matches[2] + [int]$Matches[4]) / 2))
        }
        Start-Sleep -Seconds 3
    }
}

# Attente bornée (6 min) de l'appareil puis de sys.boot_completed ; écran déverrouillé ; puis attente (6 min au plus,
# sans échec) que la charge d'un émulateur tout juste démarré (> 20) repasse sous 4.
function Wait-ActCraftDemarrage {
    # « adb get-state » répond tout de suite, contrairement à « adb wait-for-device ».
    $limite = (Get-Date).AddMinutes(6)
    while ("$(Adb 15 get-state)".Trim() -ne 'device') {
        if ((Get-Date) -gt $limite) { Lever-ActCraftAndroid 'aucun appareil adb après 6 min' }
        Start-Sleep -Seconds 2
    }
    while ("$(Adb 15 shell getprop sys.boot_completed)".Trim() -ne '1') {
        if ((Get-Date) -gt $limite) { Lever-ActCraftAndroid 'émulateur non démarré après 6 min' }
        Start-Sleep -Seconds 2
    }
    $null = Adb 30 shell input keyevent 82 # déverrouille l'écran
    $limite = (Get-Date).AddMinutes(6)
    while ([double]("$(Adb 15 shell cat /proc/loadavg)".Trim().Split(' ')[0]) -gt 4 -and (Get-Date) -lt $limite) { Start-Sleep -Seconds 5 }
}

# 80 dernières lignes de « adb logcat -d », écrites à l'hôte avant un refus. Un échec d'adb ici n'efface pas la raison
# du refus qui suit.
function Write-ActCraftLogcat {
    Write-Host 'logcat : 80 dernières lignes'
    try {
        Adb 60 logcat -d | Select-Object -Last 80 | ForEach-Object { Write-Host "logcat : $_" }
    } catch {
        Write-Host "logcat : illisible ($($_.Exception.Message))"
    }
}

# PID rendu par pidof, ou '' si aucun processus. Un dépassement du délai d'Adb compte comme une lecture vide : sous
# charge, adb peut geler un tour. Toute autre erreur reste levée.
function Read-ActCraftPidof([string]$Paquet, [int]$Secondes) {
    try {
        return "$(Adb $Secondes shell pidof $Paquet)".Trim()
    } catch {
        if (-not $_.Exception.Data.Contains('ActCraftAdbDelai')) { throw }
        Write-Host 'pidof : délai dépassé'
        return ''
    }
}

# Installe l'APK (install -r), vide le journal (logcat -c : refus si adb échoue), lance l'application et rend son
# PID : attendu 120 s au plus (pidof toutes les 2 s), car une machine chargée peut lancer l'application après plus de
# 10 s (cause probable, non prouvée, du rouge du run 42790149282), puis le même PID vivant 10 s plus tard, pour qu'un
# plantage au démarrage reste un refus. Lignes « install : … » et « logcat : … » écrites à l'hôte : la valeur rendue
# n'est que le PID.
function Install-ActCraftApk([Parameter(Mandatory)][string]$Apk, [Parameter(Mandatory)][string]$Paquet) {
    if (-not (Test-Path -LiteralPath $Apk)) { Lever-ActCraftAndroid "APK introuvable : $Apk" }
    Adb 240 install -r (Resolve-Path -LiteralPath $Apk).Path | ForEach-Object { Write-Host "install : $_" }
    if ($LASTEXITCODE -ne 0) { Lever-ActCraftAndroid "adb install$(Get-ActCraftAdbErreur)" }
    $null = Adb 30 logcat -c
    if ($LASTEXITCODE -ne 0) { Lever-ActCraftAndroid "adb logcat -c (code $LASTEXITCODE)$(Get-ActCraftAdbErreur)" }
    $null = Adb 60 shell monkey -p $Paquet -c android.intent.category.LAUNCHER 1
    $limite = (Get-Date).AddSeconds(120)
    $processus = Read-ActCraftPidof $Paquet 30
    while (-not $processus -and (Get-Date).AddSeconds(2) -lt $limite) {
        Start-Sleep -Seconds 2
        # Limite totale : une lecture ne dépasse pas le temps restant.
        $reste = [int][math]::Ceiling(($limite - (Get-Date)).TotalSeconds)
        $processus = Read-ActCraftPidof $Paquet ([math]::Max(1, [math]::Min(30, $reste)))
    }
    if (-not $processus) {
        Write-ActCraftLogcat
        Lever-ActCraftAndroid 'processus absent 120 s après le lancement'
    }
    Start-Sleep -Seconds 10
    $encore = Read-ActCraftPidof $Paquet 30
    if ($encore -ne $processus) {
        Write-ActCraftLogcat
        Lever-ActCraftAndroid "processus $processus disparu 10 s après son apparition (pidof : « $encore »)"
    }
    return $processus
}
