# L11-T13 (D11-8) sous Android : reprise sans perte après un processus tué en arrière-plan, sur l'émulateur.
# Usage : pwsh -NoProfile -File ./scripts/essai-android-cycle.ps1 -Apk <…-x86_64.apk> [-Avd <nom>]
# Démarre l'émulateur (sans fenêtre, -gpu host), installe l'APK, le relance avec l'option d'essai
# `--journal-empreinte` (« ACTCRAFT EMPREINTE <h> » dans logcat à chaque changement du document), trace un trait au
# doigt (contrôlé sur la capture), relève l'empreinte, met l'application en arrière-plan (touche HOME) : la copie de
# reprise est écrite (« ACTCRAFT COPIE DE REPRISE <h> ») ; attend 2 s, tue le processus comme le système
# (`am kill`, processus en arrière-plan), relance : « ACTCRAFT REPRISE <h> », même empreinte, trait visible (capture,
# pixel). Refuse de démarrer si un émulateur occupe déjà le port de l'essai. Appels adb : scripts/android-commun.ps1.
# Dernière ligne : « ANDROID CYCLE OK » (0) ou « ANDROID CYCLE EN ECHEC : <raison> » (1), même sur une erreur
# imprévue. Sorties de diagnostic sous le .tmp/ du dépôt du script (cycle-*.png, cycle-logcat.txt). Ce que l'essai
# a lancé (émulateur et ses descendants, client et serveur adb) est arrêté par les processus qu'il tient, jamais par
# nom ni par un PID relu, y compris sur Ctrl+C (bloc finally) ; un serveur adb que l'essai n'a pas lancé n'est jamais
# arrêté. Écrit sur le Mac (aucun SDK) : à jouer au PC.
# n'est jamais arrêté.
param([Parameter(Mandatory)][string]$Apk, [string]$Avd = '')
$ErrorActionPreference = 'Stop'
# Chemins absolus, calculés depuis l'emplacement du script : adb et python écrivent un chemin relatif dans le dossier
# courant du processus, que Set-Location ne change pas (lancé depuis un autre dossier, l'essai écrivait ses captures
# hors du dépôt : rejeu de la revue, 2026-10-17). Un APK donné en relatif l'est par rapport au dossier d'appel.
$Apk = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Apk)
$racine = Split-Path -Parent $PSScriptRoot
$tmp = Join-Path $racine '.tmp'
Set-Location $racine
$paquet = 'io.github.jejeasterix.actcraft'
$activite = "$paquet/org.qtproject.qt.android.bindings.QtActivity"
# Port propre à l'essai : tout appel adb vise cet émulateur (ANDROID_SERIAL), jamais un autre appareil branché.
$port = 5590
# Processus lancés par l'essai, tenus par un handle ouvert (Process.Handle) : tant qu'il est tenu, un PID ne peut pas
# être réattribué, et HasExited, Kill et WaitForExit visent ce processus-là.
$emulateur = $null
$lancementEmulateur = $null
$descendantsEmulateur = @()
$clientAdb = $null
$lancementClientAdb = $null
$serveurAdb = $null
$identiteServeurAdb = $null
$arretsRates = @()
$python = if (-not $IsWindows -and (Get-Command python3 -ErrorAction SilentlyContinue)) { 'python3' } else { 'python' }

# Ligne de la table des processus (Windows : ProcessId, ParentProcessId, CreationDate, Name, ExecutablePath), ou
# $null si le PID n'est pas celui d'un processus vivant.
function Get-LigneProcessus([int]$Id) {
    return Get-CimInstance Win32_Process -Filter "ProcessId = $Id" -ErrorAction SilentlyContinue | Select-Object -First 1
}
# Processus d'un PID, tenu (handle ouvert), ou $null s'il est mort ou si le PID a été réattribué entre la lecture de
# la table (création $Creation) et l'ouverture du handle : relue une fois le handle ouvert, la table doit donner la
# même création.
function Get-ProcessusTenu([int]$Id, [datetime]$Creation) {
    try { $p = Get-Process -Id $Id -ErrorAction Stop; $null = $p.Handle } catch { return $null }
    $ligne = Get-LigneProcessus $Id
    if (-not $ligne -or $ligne.CreationDate -ne $Creation) { return $null }
    return $p
}
function Get-NomProcessus($Processus) { try { return $Processus.ProcessName } catch { return '?' } }
# Descendants d'un processus tenu (Windows), tenus à leur tour : parent attendu et création postérieure à celle du
# parent ($Lancement : heure relevée juste avant le lancement de la racine) ; un PID de parent réattribué ne fait pas
# d'un processus étranger un descendant. Un orphelin (qemu dont l'émulateur s'est arrêté) garde son ParentProcessId :
# il est trouvé tant que son parent est tenu.
function Get-Descendants($Racine, $Lancement) {
    if (-not $IsWindows -or -not $Racine -or -not $Lancement) { return @() }
    $table = @(Get-CimInstance Win32_Process -Property ProcessId, ParentProcessId, CreationDate -ErrorAction SilentlyContinue)
    $trouves = [Collections.Generic.List[object]]::new()
    $parents = [Collections.Generic.Queue[object]]::new()
    $parents.Enqueue([pscustomobject]@{ Id = $Racine.Id; Debut = [datetime]$Lancement })
    while ($parents.Count -gt 0) {
        $parent = $parents.Dequeue()
        foreach ($ligne in $table) {
            if ($ligne.ParentProcessId -ne $parent.Id -or $ligne.ProcessId -eq $parent.Id -or -not $ligne.CreationDate -or
                $ligne.CreationDate -lt $parent.Debut) { continue }
            if (@($trouves | Where-Object { $_.Id -eq $ligne.ProcessId }).Count -gt 0) { continue }
            $p = Get-ProcessusTenu $ligne.ProcessId $ligne.CreationDate
            if (-not $p) { continue }
            $trouves.Add($p)
            $parents.Enqueue([pscustomobject]@{ Id = $p.Id; Debut = $ligne.CreationDate })
        }
    }
    return @($trouves)
}

# Démarrage local de l'émulateur : sans fenêtre, -gpu host, démarrage à froid sans instantané (rien d'enregistré à
# l'arrêt), dossier courant hors du dépôt (le worktree n'est pas verrouillé). Le processus est tenu dès son lancement :
# un arrêt pendant l'attente du démarrage l'arrête aussi.
function Start-ActCraftEmulateur([string]$Avd) {
    $exe = (Get-Command emulator -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1).Source
    if (-not $exe) { Stop-Essai 'emulator introuvable dans le PATH (dev-env-android.ps1)' }
    if (-not $Avd) {
        $Avd = @(& $exe -list-avds | Where-Object { $_ -cmatch '^[\w.-]+$' }) | Select-Object -First 1
        if (-not $Avd) { Stop-Essai 'aucun AVD (emulator -list-avds)' }
    }
    $journal = Join-Path $script:tmp 'cycle-emulateur.txt'
    $script:lancementEmulateur = Get-Date
    $p = Start-Process -FilePath $exe -PassThru -NoNewWindow -WorkingDirectory (Split-Path $exe) `
        -RedirectStandardOutput $journal -RedirectStandardError "$journal.err" `
        -ArgumentList @('-avd', $Avd, '-port', "$port", '-no-window', '-gpu', 'host', '-no-snapshot', '-no-boot-anim', '-no-audio')
    $null = $p.Handle
    $script:emulateur = $p
    Write-Host "émulateur : $Avd, PID $($p.Id), port $port"
    Start-Sleep -Seconds 5
    if ($p.HasExited) { Stop-Essai "émulateur arrêté au démarrage (code $($p.ExitCode)) : $((Get-Content "$journal.err", $journal -ErrorAction SilentlyContinue | Select-Object -Last 2) -join ' ')" }
}
# Refus net si un processus écoute déjà sur le port de l'essai (console $port, adb $port + 1) : l'émulateur d'un
# passage interrompu servirait d'appareil à la place de celui que l'essai démarre.
function Assert-PortEssaiLibre {
    if (-not $IsWindows) { return }
    $occupants = @(Get-NetTCPConnection -LocalPort $port, ($port + 1) -State Listen -ErrorAction SilentlyContinue |
            ForEach-Object { $_.OwningProcess } | Select-Object -Unique)
    if ($occupants.Count -gt 0) {
        Stop-Essai "port $port ou $($port + 1) déjà écouté (PID $($occupants -join ', ')) : émulateur d'un passage précédent ? à arrêter avant l'essai"
    }
}
# Serveur adb démarré à part (sans handle hérité, dossier courant hors du dépôt) : démarré par le premier appel borné,
# il hériterait de la sortie redirigée de ce script (« > fichier » resterait ouvert après la fin) et verrouillerait le
# worktree. Le client « adb start-server » est tenu dès son lancement (arrêté s'il ne rend pas la main) ; le serveur
# qui écoute ensuite sur le port 5037 n'est gardé, pour l'arrêter à la fin, que s'il descend de ce client : un serveur
# déjà là, ou démarré entre-temps par une autre session, n'est jamais arrêté.
function Start-ActCraftServeurAdb {
    $script:lancementClientAdb = Get-Date
    $client = Start-Process -FilePath $script:ActCraftAdb -ArgumentList 'start-server' `
        -WorkingDirectory ([IO.Path]::GetTempPath()) -WindowStyle Hidden -PassThru
    $null = $client.Handle
    $script:clientAdb = $client
    $fin = (Get-Date).AddSeconds(60)
    # Attente par tranches : un Ctrl+C est traité entre deux tranches, pas au bout des 60 s.
    while (-not $client.WaitForExit(500)) {
        if ((Get-Date) -gt $fin) { Stop-Essai 'adb start-server sans réponse (60 s)' }
    }
    Find-ServeurAdbLance $client $script:lancementClientAdb
    if ($script:serveurAdb) { Write-Output "adb : serveur démarré par l'essai, PID $($script:serveurAdb.Id)" }
    elseif ($IsWindows) { Write-Output 'adb : serveur lancé hors de l''essai (déjà là, ou par une autre session), laissé vivant' }
    else { Write-Output 'adb : serveur non identifiable hors de Windows, laissé vivant' }
}
# Serveur adb lancé par le client tenu : processus qui écoute sur le port 5037 et descend de ce client. Son identité
# (nom, heure de création, chemin) est relevée pour être revérifiée avant l'arrêt.
function Find-ServeurAdbLance($Client, $Lancement) {
    if ($script:serveurAdb -or -not $IsWindows -or -not $Client) { return }
    $ecoute = @(Get-NetTCPConnection -LocalPort 5037 -State Listen -ErrorAction SilentlyContinue |
            ForEach-Object { $_.OwningProcess } | Select-Object -Unique)
    if ($ecoute.Count -eq 0) { return }
    $serveur = @(Get-Descendants $Client $Lancement | Where-Object { $ecoute -contains $_.Id }) | Select-Object -First 1
    $ligne = if ($serveur) { Get-LigneProcessus $serveur.Id }
    if (-not $ligne) { return }
    $script:serveurAdb = $serveur
    $script:identiteServeurAdb = @{ Nom = $ligne.Name; Creation = $ligne.CreationDate; Chemin = $ligne.ExecutablePath }
}
# Arrêt de ce que l'essai a lancé, par les processus tenus : l'émulateur et ses descendants (qemu…) sans rien
# enregistrer, le client « start-server » s'il tourne encore, le serveur adb lancé par l'essai après revérification de
# son identité. Rend les lignes d'état ; un arrêt manqué va dans $script:arretsRates (l'essai échoue) et rien n'est
# dit « arrêté » qui ne soit mort. Sans effet la deuxième fois (Stop-Essai, fin réussie, puis bloc finally).
function Stop-ActCraftLances {
    if ($script:emulateur) {
        $racine = $script:emulateur
        $script:emulateur = $null
        $cibles = @(@($racine) + @($script:descendantsEmulateur | Where-Object { $_ }) + @(Get-Descendants $racine $script:lancementEmulateur) |
                Sort-Object Id -Unique)
        $noms = @($cibles | Where-Object { $_.Id -ne $racine.Id } | ForEach-Object { "$(Get-NomProcessus $_) $($_.Id)" })
        foreach ($c in $cibles) { try { if (-not $c.HasExited) { $c.Kill($true) } } catch { $null = $_ } }
        $vivants = @($cibles | Where-Object { -not $_.WaitForExit(30000) })
        if ($vivants.Count -gt 0) {
            $script:arretsRates += "émulateur : PID $(($vivants | ForEach-Object { $_.Id }) -join ', ') encore vivant(s) 30 s après l'arrêt"
        } else {
            "émulateur : PID $($racine.Id) arrêté$(if ($noms) { ", descendants arrêtés : $($noms -join ', ')" })"
        }
    }
    if ($script:clientAdb) {
        $client = $script:clientAdb
        $script:clientAdb = $null
        Find-ServeurAdbLance $client $script:lancementClientAdb # avant l'arrêt du client : un serveur qu'il a lancé revient à l'essai
        if (-not $client.HasExited) {
            try { $client.Kill($true) } catch { $null = $_ }
            if ($client.WaitForExit(10000)) { "adb : client start-server PID $($client.Id) arrêté" }
            else { $script:arretsRates += "adb : client start-server PID $($client.Id) encore vivant 10 s après l'arrêt" }
        }
    }
    if ($script:serveurAdb) {
        $serveur = $script:serveurAdb
        $identite = $script:identiteServeurAdb
        $script:serveurAdb = $null
        if ($serveur.HasExited) { "adb : serveur PID $($serveur.Id) déjà arrêté" }
        else {
            $actuel = Get-LigneProcessus $serveur.Id
            if (-not $actuel -or $actuel.Name -ne $identite.Nom -or $actuel.CreationDate -ne $identite.Creation -or
                $actuel.ExecutablePath -ne $identite.Chemin) {
                $script:arretsRates += "adb : PID $($serveur.Id) n'a plus l'identité du serveur lancé par l'essai, laissé vivant"
            } else {
                try { $serveur.Kill() } catch { $null = $_ }
                if ($serveur.WaitForExit(10000)) { "adb : serveur PID $($serveur.Id) arrêté" }
                else { $script:arretsRates += "adb : serveur PID $($serveur.Id) encore vivant 10 s après l'arrêt" }
            }
        }
    }
}
function Stop-Essai([string]$raison) {
    if ($script:emulateur -and $script:ActCraftAdb) {
        try {
            Adb 60 logcat -d | Set-Content -Encoding utf8 -LiteralPath (Join-Path $script:tmp 'cycle-echec-logcat.txt')
            $null = Adb 60 shell screencap -p /sdcard/actcraft-echec.png
            $null = Adb 60 pull /sdcard/actcraft-echec.png (Join-Path $script:tmp 'cycle-echec.png')
            $raison += ' (journal .tmp/cycle-echec-logcat.txt, capture .tmp/cycle-echec.png)'
        } catch { $null = $_ }
    }
    Stop-ActCraftLances
    if ($script:arretsRates) { $raison += " ; $($script:arretsRates -join ' ; ')" }
    Write-Output "ANDROID CYCLE EN ECHEC : $raison"
    exit 1
}
# Adaptateur sur l'appel borné du module commun (Adb) : aucune copie ; ses exceptions arrivent au catch final.
# Raison d'un refus du module Android que l'essai tolère (adb sans réponse, MediaStore momentanément muet) ;
# toute autre erreur remonte.
function Get-RaisonTolerable($erreur) {
    $raison = Get-ActCraftAndroidRaison $erreur
    if (-not $raison) { throw $erreur }
    return $raison
}
function Invoke-Adb([string[]]$arguments, [int]$secondes = 30) {
    return @(Adb $secondes @arguments)
}
# Appel d'un tour d'attente bornée : un adb sans réponse (émulateur figé une minute sous charge, boîtes « ne répond
# pas » de gms et du lanceur : passage de la mutation, essai 3) compte comme un tour manqué, pas comme un échec ;
# l'attente bornée de l'étape décide. Rend $null après un tel refus ; toute autre erreur remonte.
function Invoke-AdbTour([string[]]$arguments, [int]$secondes = 30) {
    try { return @(Invoke-Adb $arguments $secondes) } catch {
        $raison = Get-RaisonTolerable $_
        Write-Host "adb : $raison, tour suivant"
        return $null
    }
}
# Dernière ligne du journal qui correspond au motif (casse comprise), attendue jusqu'au délai.
function Wait-Journal([string]$motif, [int]$secondes = 60) {
    $fin = (Get-Date).AddSeconds($secondes)
    while ((Get-Date) -lt $fin) {
        $ligne = Invoke-AdbTour @('logcat', '-d') | Where-Object { $_ -cmatch $motif } | Select-Object -Last 1
        if ($ligne) { return $ligne }
        Start-Sleep -Milliseconds 500
    }
    return $null
}
# Empreinte (16 chiffres hexadécimaux) d'une ligne « ACTCRAFT <quoi> <h> », vide sinon.
function Get-Empreinte([string]$ligne, [string]$quoi) {
    return [regex]::Match("$ligne", "ACTCRAFT $quoi ([0-9a-f]{16})").Groups[1].Value
}
# Lancement de l'activité avec l'option d'essai (applicationArguments, lu par Qt) ; rend le PID, ou échoue.
function Start-ActCraftAvecEmpreinte {
    $null = Invoke-Adb @('shell', 'am', 'start', '-W', '-n', $activite, '--es', 'applicationArguments', '--journal-empreinte') 60
    Start-Sleep -Seconds 8
    $lance = "$(Invoke-Adb @('shell', 'pidof', $paquet))".Trim()
    if (-not $lance) { Stop-Essai 'processus absent 8 s après le lancement avec --journal-empreinte' }
    return $lance
}
# Pixel (r g b) d'une capture de l'écran prise maintenant, sous .tmp/cycle-<quoi>.png ; tableau vide sans capture.
function Read-PixelEcran([string]$quoi, [int]$x, [int]$y) {
    $capture = Join-Path $tmp "cycle-$quoi.png"
    Remove-Item -ErrorAction SilentlyContinue -LiteralPath $capture # jamais la capture d'un passage précédent
    $null = Invoke-AdbTour @('shell', 'screencap', '-p', "/sdcard/actcraft-$quoi.png") 60
    $null = Invoke-AdbTour @('pull', "/sdcard/actcraft-$quoi.png", $capture) 60
    if (-not (Test-Path -LiteralPath $capture)) { return @() }
    return @("$(& $python -I (Join-Path $PSScriptRoot 'lire-pixel.py') $capture $x $y)".Trim().Split(' ') | ForEach-Object { [int]$_ })
}
function Test-Sombre($pixel) { return $pixel.Count -eq 3 -and $pixel[0] -lt 100 -and $pixel[1] -lt 100 -and $pixel[2] -lt 100 }

try {
    New-Item -ItemType Directory -Force $tmp | Out-Null
    . "$PSScriptRoot/dev-env-android.ps1"
    . "$PSScriptRoot/android-commun.ps1"
    Initialize-ActCraftAdb
    if (-not (Test-Path -LiteralPath $Apk)) { Stop-Essai "APK introuvable : $Apk" }
    Assert-PortEssaiLibre
    $env:ANDROID_SERIAL = "emulator-$port"
    Start-ActCraftServeurAdb
    if (@(Invoke-Adb @('devices') 30) | Where-Object { $_ -cmatch "^emulator-$port\s" }) {
        Stop-Essai "emulator-$port déjà connu d'adb avant le démarrage de l'essai : émulateur d'un passage précédent ?"
    }
    Start-ActCraftEmulateur -Avd $Avd
    Wait-ActCraftDemarrage
    $descendantsEmulateur = @(Get-Descendants $emulateur $lancementEmulateur)
    Write-Output "émulateur : descendants $(($descendantsEmulateur | ForEach-Object { "$(Get-NomProcessus $_) $($_.Id)" }) -join ', ')"
    Ecarter-Anr
    $null = Invoke-Adb @('shell', 'settings', 'put', 'secure', 'immersive_mode_confirmations', 'confirmed')
    # Clé de débogage propre à chaque dossier de construction : désinstallé d'abord ; la désinstallation efface le
    # dossier privé, donc toute copie de reprise d'un passage précédent (état initial sans copie).
    $null = Invoke-Adb @('uninstall', $paquet) 60
    $null = Install-ActCraftApk -Apk $Apk -Paquet $paquet
    $null = Invoke-Adb @('shell', 'am', 'force-stop', $paquet) 30
    $null = Invoke-Adb @('logcat', '-c')
    $processus = Start-ActCraftAvecEmpreinte
    Write-Output "processus : $processus (--journal-empreinte)"

    # Un trait au doigt au milieu de la page (tablette 2560 × 1600, comme essai-android-fichiers.ps1), contrôlé sur
    # la capture ; glissement rejoué (trois fois au plus) s'il tombe pendant un changement de disposition.
    $traitVu = $false
    for ($essai = 0; $essai -lt 3 -and -not $traitVu; $essai++) {
        Start-Sleep -Seconds 3
        $null = Invoke-AdbTour @('shell', 'input', 'swipe', '768', '800', '1408', '800', '600')
        Start-Sleep -Seconds 2
        $trait = Read-PixelEcran 'trait' 1088 800
        Write-Output "trait : lu $($trait -join ' ')"
        $traitVu = Test-Sombre $trait
    }
    if (-not $traitVu) { Stop-Essai 'trait absent après trois glissements (capture .tmp/cycle-trait.png)' }
    Start-Sleep -Seconds 1
    $avant = Get-Empreinte (Wait-Journal 'ACTCRAFT EMPREINTE [0-9a-f]{16}') 'EMPREINTE'
    if (-not $avant) { Stop-Essai 'aucune ligne ACTCRAFT EMPREINTE après le trait' }
    Write-Output "empreinte avant : $avant"

    # Arrière-plan (onStop : Qt::ApplicationSuspended, et Hidden sous Android) : copie de reprise écrite.
    $null = Invoke-Adb @('shell', 'input', 'keyevent', 'HOME')
    $copie = Get-Empreinte (Wait-Journal 'ACTCRAFT COPIE DE REPRISE [0-9a-f]{16}' 30) 'COPIE DE REPRISE'
    if (-not $copie) { Stop-Essai 'aucune ligne ACTCRAFT COPIE DE REPRISE après HOME' }
    if ($copie -cne $avant) { Stop-Essai "copie de reprise $copie, empreinte du document $avant" }
    Start-Sleep -Seconds 2
    # Processus en arrière-plan tué comme par le système (am kill ne tue qu'un processus en arrière-plan).
    $null = Invoke-Adb @('shell', 'am', 'kill', $paquet) 30
    $fin = (Get-Date).AddSeconds(20)
    while ("$(Invoke-AdbTour @('shell', 'pidof', $paquet))".Trim() -and (Get-Date) -lt $fin) { Start-Sleep -Milliseconds 500 }
    if ("$(Invoke-AdbTour @('shell', 'pidof', $paquet))".Trim()) { Stop-Essai 'processus encore vivant 20 s après am kill' }
    Write-Output 'processus : tué en arrière-plan'

    $null = Invoke-Adb @('logcat', '-c')
    if ($LASTEXITCODE -ne 0) { Stop-Essai "adb logcat -c (code $LASTEXITCODE) : journal non vidé avant la relance$(Get-ActCraftAdbErreur)" }
    $processus = Start-ActCraftAvecEmpreinte
    Write-Output "processus : $processus (relancé)"
    $reprise = Get-Empreinte (Wait-Journal 'ACTCRAFT REPRISE [0-9a-f]{16}') 'REPRISE'
    if (-not $reprise) { Stop-Essai 'aucune ligne ACTCRAFT REPRISE au relancement' }
    Start-Sleep -Seconds 2
    $apres = Get-Empreinte (Wait-Journal 'ACTCRAFT EMPREINTE [0-9a-f]{16}') 'EMPREINTE'
    Write-Output "reprise : $reprise ; empreinte après : $apres"
    if ($reprise -cne $avant -or $apres -cne $avant) {
        Stop-Essai "empreintes différentes : $avant avant, $reprise reprise, $(if ($apres) { $apres } else { '(vide)' }) après"
    }
    $repris = @()
    for ($essai = 0; $essai -lt 3 -and -not (Test-Sombre $repris); $essai++) {
        Start-Sleep -Seconds 2
        $repris = Read-PixelEcran 'reprise' 1088 800
    }
    Write-Output "trait repris : lu $($repris -join ' ')"
    if (-not (Test-Sombre $repris)) { Stop-Essai 'trait absent après la reprise (capture .tmp/cycle-reprise.png)' }
    Invoke-Adb @('logcat', '-d') | Set-Content -Encoding utf8 -LiteralPath (Join-Path $tmp 'cycle-logcat.txt')

    Stop-ActCraftLances
    if ($arretsRates) {
        Write-Output "ANDROID CYCLE EN ECHEC : $($arretsRates -join ' ; ')"
        exit 1
    }
    Write-Output 'ANDROID CYCLE OK'
    exit 0
} catch {
    $raison = $null
    if (Get-Command Get-ActCraftAndroidRaison -ErrorAction SilentlyContinue) { $raison = Get-ActCraftAndroidRaison $_ }
    if (-not $raison) { $raison = "erreur imprévue : $($_.Exception.Message) (ligne $($_.InvocationInfo.ScriptLineNumber))" }
    Stop-Essai $raison
} finally {
    if ($emulateur -or $clientAdb -or $serveurAdb) {
        $lignes = @(Stop-ActCraftLances) + @($arretsRates)
        foreach ($ligne in $lignes) { try { Write-Host "arrêt (fin interrompue) : $ligne" } catch { $null = $_ } }
    }
}
