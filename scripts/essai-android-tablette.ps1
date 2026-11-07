# Tablette sous Android (D11-10, D11-11, S3.2) : rotation de l'écran et clavier logiciel sur un émulateur de tablette.
# Usage : pwsh -NoProfile -File ./scripts/essai-android-tablette.ps1 -Apk <…-x86_64.apk> [-Avd <nom>]
# AVD : sans -Avd, « actcraft-tablette ». S'il n'existe pas, il est créé (avdmanager create avd -n actcraft-tablette
# -d pixel_tablet) sur l'image système DÉJÀ installée, sans téléchargement ; sans image x86_64 installée, échec
# « image système absente » (l'orchestrateur demande alors l'accord de Jérôme pour la télécharger). La création et la
# taille du dossier de l'AVD (plusieurs Go sous %USERPROFILE%\.android\avd) sont écrites au rapport.
# Déroulé : émulateur sans fenêtre (-gpu host), APK installé et lancé, trait au doigt (capture : pixels noirs en plus) ;
# écran tourné (settings accelerometer_rotation 0, user_rotation 1) : la capture change de sens, le trait est toujours
# là, la palette est visible ; outil texte (Ctrl+T), appui sur la page : « mInputShown=true » dans
# `dumpsys input_method` et le bas du cadre du texte (couleur du cadre de l'éditeur) est au-dessus du haut du clavier
# (`dumpsys window InputMethod`). Aucune orientation forcée au manifeste. Appels adb : scripts/android-commun.ps1.
# Dernière ligne : « ANDROID TABLETTE OK » (0) ou « ANDROID TABLETTE EN ECHEC : <raison> » (1), même sur une erreur
# imprévue. Diagnostics sous le .tmp/ du dépôt (tablette-*.png, tablette-logcat.txt). Ce que l'essai a lancé
# (émulateur et ses descendants, client et serveur adb) est arrêté par les processus tenus, jamais par nom ; un
# serveur adb que l'essai n'a pas lancé n'est jamais arrêté. Jamais joué sur le Mac (pas de SDK) : essai sur le PC.
param([Parameter(Mandatory)][string]$Apk, [string]$Avd = '')
$ErrorActionPreference = 'Stop'
# Chemins absolus, calculés depuis l'emplacement du script : adb et python écrivent un chemin relatif dans le dossier
# courant du processus, que Set-Location ne change pas (lancé depuis un autre dossier, l'essai écrivait ses captures
# hors du dépôt : rejeu de la revue, 2026-10-17). Un APK donné en relatif l'est par rapport au dossier d'appel.
$Apk = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Apk)
$racine = Split-Path -Parent $PSScriptRoot
$tmp = Join-Path $racine '.tmp'
$arbreUi = Join-Path $tmp 'tablette-ui.xml'
Set-Location $racine
$paquet = 'io.github.jejeasterix.actcraft'
# Port propre à l'essai : tout appel adb vise cet émulateur (ANDROID_SERIAL), jamais un autre appareil branché.
$port = 5586
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
    $journal = Join-Path $script:tmp 'tablette-emulateur.txt'
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
            Adb 60 logcat -d | Set-Content -Encoding utf8 -LiteralPath (Join-Path $script:tmp 'tablette-echec-logcat.txt')
            $null = Adb 60 shell screencap -p /sdcard/actcraft-echec.png
            $null = Adb 60 pull /sdcard/actcraft-echec.png (Join-Path $script:tmp 'tablette-echec.png')
            $raison += ' (journal .tmp/tablette-echec-logcat.txt, capture .tmp/tablette-echec.png)'
        } catch { $null = $_ }
    }
    Stop-ActCraftLances
    if ($script:arretsRates) { $raison += " ; $($script:arretsRates -join ' ; ')" }
    Write-Output "ANDROID TABLETTE EN ECHEC : $raison"
    exit 1
}
# Adaptateur sur l'appel borné du module commun (Adb) : aucune copie ; ses exceptions arrivent au catch final.
# Nœuds de l'arbre d'interface (uiautomator), en tableau.
function Get-Noeuds($arbre) { return @($arbre.SelectNodes('//node')) }
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

# Arbre de l'interface Android (uiautomator), ou $null s'il n'a pas pu être pris (« could not get idle state » tant
# qu'une fenêtre s'anime). Dernier arbre lu : .tmp/tablette-ui.xml.
# Une boîte « … ne répond pas » du système (machine chargée : « System UI isn't responding » a couvert le sélecteur,
# essai 3) est écartée par son bouton « attendre » (android:id/aerr_wait, indépendant de la langue) et l'arbre relu,
# trois fois au plus : toute attente qui relit l'arbre la traverse.
function Read-ArbreUi {
    for ($boite = 0; $boite -le 3; $boite++) {
        Remove-Item -ErrorAction SilentlyContinue -LiteralPath $script:arbreUi
        $null = Invoke-AdbTour @('shell', 'rm', '-f', '/sdcard/actcraft-ui.xml')
        $null = Invoke-AdbTour @('shell', 'uiautomator', 'dump', '/sdcard/actcraft-ui.xml') 60
        $null = Invoke-AdbTour @('pull', '/sdcard/actcraft-ui.xml', $script:arbreUi) 30
        if (-not (Test-Path -LiteralPath $script:arbreUi)) { return $null }
        try { $arbre = [xml](Get-Content -Raw -Encoding utf8 -LiteralPath $script:arbreUi) } catch { return $null }
        $attendre = Get-Noeuds $arbre | Where-Object { $_.'resource-id' -eq 'android:id/aerr_wait' } | Select-Object -First 1
        if (-not $attendre -or $boite -eq 3) { break }
        Write-Host "boîte « ne répond pas » du système écartée : $(Get-Noeuds $arbre | Where-Object { $_.'resource-id' -eq 'android:id/alertTitle' } | ForEach-Object { $_.text })"
        Invoke-Tap $attendre
        Start-Sleep -Seconds 2
    }
    return , $arbre # virgule : un XmlDocument rendu tel quel serait énuméré nœud par nœud
}
# Touche un nœud en son centre (ligne d'information écrite à l'hôte : rien n'est rendu).
function Invoke-Tap($noeud) {
    $b = @([regex]::Matches($noeud.bounds, '\d+') | ForEach-Object { [int]$_.Value })
    $x = [int](($b[0] + $b[2]) / 2); $y = [int](($b[1] + $b[3]) / 2)
    Write-Host "touché : « $($noeud.text) » ($($noeud.'resource-id'), $($noeud.package)) en ($x, $y)"
    $null = Invoke-Adb @('shell', 'input', 'tap', "$x", "$y")
}

# Image système x86_64 déjà installée (aucun téléchargement), ou $null.
function Get-ImageSysteme {
    $sdk = if ($env:ANDROID_HOME) { $env:ANDROID_HOME } else { $env:ANDROID_SDK_ROOT }
    if (-not $sdk) { return $null }
    $dossiers = @(Get-ChildItem -Directory -ErrorAction SilentlyContinue (Join-Path $sdk 'system-images/*/*/x86_64'))
    if ($dossiers.Count -eq 0) { return $null }
    $d = $dossiers | Sort-Object FullName -Descending | Select-Object -First 1
    return "system-images;$($d.Parent.Parent.Name);$($d.Parent.Name);x86_64"
}
# Capture de l'écran sous .tmp/tablette-<nom>.png ; rend son chemin.
function Get-Capture([string]$nom) {
    $chemin = Join-Path $script:tmp "tablette-$nom.png"
    Remove-Item -ErrorAction SilentlyContinue -LiteralPath $chemin
    $null = Invoke-Adb @('shell', 'screencap', '-p', '/sdcard/actcraft-tablette.png') 60
    $null = Invoke-Adb @('pull', '/sdcard/actcraft-tablette.png', $chemin) 60
    if (-not (Test-Path -LiteralPath $chemin)) { Stop-Essai "capture $nom absente" }
    return $chemin
}
# Mesures d'une capture par PIL (python -I) : « <largeur> <hauteur> <pixels noirs> <ligne la plus basse du bleu du cadre> ».
# Noir : trois canaux sous 30 (le trait au doigt) ; bleu du cadre de l'éditeur : #1F4E9E à 24 près, -1 s'il est absent.
function Get-Mesures([string]$capture) {
    $code = @'
import sys
from PIL import Image
im = Image.open(sys.argv[1]).convert("RGB")
w, h = im.size
px = im.load()
noir = 0
bas = -1
for y in range(h):
    for x in range(0, w, 2):
        r, g, b = px[x, y]
        if r < 30 and g < 30 and b < 30:
            noir += 1
        elif abs(r - 0x1F) < 24 and abs(g - 0x4E) < 24 and abs(b - 0x9E) < 24:
            bas = y
print(w, h, noir, bas)
'@
    return @("$(& $script:python -I -c $code $capture)".Trim().Split(' ') | ForEach-Object { [int]$_ })
}
function Wait-Condition([scriptblock]$test, [int]$secondes) {
    $fin = (Get-Date).AddSeconds($secondes)
    do { if (& $test) { return $true }; Start-Sleep -Seconds 2 } while ((Get-Date) -lt $fin)
    return $false
}

try {
    New-Item -ItemType Directory -Force $tmp | Out-Null
    . "$PSScriptRoot/dev-env-android.ps1"
    . "$PSScriptRoot/android-commun.ps1"
    Initialize-ActCraftAdb
    if (-not (Test-Path -LiteralPath $Apk)) { Stop-Essai "APK introuvable : $Apk" }
    if (-not $Avd) { $Avd = 'actcraft-tablette' }
    $exe = (Get-Command emulator -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1).Source
    if (-not $exe) { Stop-Essai 'emulator introuvable dans le PATH (dev-env-android.ps1)' }
    if (@(& $exe -list-avds) -cnotcontains $Avd) {
        $image = Get-ImageSysteme
        if (-not $image) { Stop-Essai 'image système absente' }
        $gestionnaire = (Get-Command avdmanager -ErrorAction SilentlyContinue | Select-Object -First 1).Source
        if (-not $gestionnaire) { Stop-Essai 'avdmanager introuvable dans le PATH' }
        'no' | & $gestionnaire create avd -n $Avd -d pixel_tablet -k $image --force | ForEach-Object { Write-Host "avdmanager : $_" }
        if ($LASTEXITCODE -ne 0) { Stop-Essai "avdmanager create avd (code $LASTEXITCODE)" }
        $dossierAvd = Join-Path $HOME ".android/avd/$Avd.avd"
        $octets = (Get-ChildItem -Recurse -File -ErrorAction SilentlyContinue $dossierAvd | Measure-Object Length -Sum).Sum
        Write-Output "AVD créé : $Avd sur $image, $([math]::Round($octets / 1MB)) Mo"
    }
    Assert-PortEssaiLibre
    $env:ANDROID_SERIAL = "emulator-$port"
    Start-ActCraftServeurAdb
    if (@(Invoke-Adb @('devices') 30) | Where-Object { $_ -cmatch "^emulator-$port\s" }) {
        Stop-Essai "emulator-$port déjà connu d'adb avant le démarrage de l'essai : émulateur d'un passage précédent ?"
    }
    Start-ActCraftEmulateur -Avd $Avd
    Wait-ActCraftDemarrage
    $descendantsEmulateur = @(Get-Descendants $emulateur $lancementEmulateur)
    Ecarter-Anr
    Write-Output "appareil : API $("$(Invoke-Adb @('shell', 'getprop', 'ro.build.version.sdk'))".Trim()), $("$(Invoke-Adb @('shell', 'getprop', 'ro.product.cpu.abi'))".Trim())"
    $null = Invoke-Adb @('shell', 'settings', 'put', 'secure', 'immersive_mode_confirmations', 'confirmed')
    # Orientation : paysage au départ, rotation automatique coupée (on tourne à la main).
    $null = Invoke-Adb @('shell', 'settings', 'put', 'system', 'accelerometer_rotation', '0')
    $null = Invoke-Adb @('shell', 'settings', 'put', 'system', 'user_rotation', '0')
    $null = Invoke-Adb @('uninstall', $paquet) 60
    $processus = Install-ActCraftApk -Apk $Apk -Paquet $paquet
    Write-Output "processus : $processus"
    Start-Sleep -Seconds 5

    $base = Get-Mesures (Get-Capture 'base')
    Write-Output "base : $($base -join ' ')"
    if ($base[0] -le $base[1]) { Stop-Essai "l'AVD n'est pas en paysage au départ ($($base[0])x$($base[1]))" }

    # Trait au doigt au milieu de la page (rejoué trois fois au plus s'il tombe pendant un changement de disposition).
    $trait = $null
    for ($essai = 0; $essai -lt 3; $essai++) {
        Start-Sleep -Seconds 3
        $null = Invoke-AdbTour @('shell', 'input', 'swipe', '768', '800', '1408', '800', '600')
        Start-Sleep -Seconds 2
        $trait = Get-Mesures (Get-Capture 'trait')
        Write-Output "trait : $($trait -join ' ')"
        if ($trait[2] -ge $base[2] + 200) { break }
    }
    if ($trait[2] -lt $base[2] + 200) { Stop-Essai 'trait absent après trois glissements (capture .tmp/tablette-trait.png)' }

    # Rotation : l'écran passe en portrait, le trait reste (D11-10), la palette reste visible.
    $null = Invoke-Adb @('shell', 'settings', 'put', 'system', 'user_rotation', '1')
    $tourne = $null
    $ok = Wait-Condition {
        $script:tourne = Get-Mesures (Get-Capture 'portrait')
        $script:tourne[0] -lt $script:tourne[1]
    } 60
    if (-not $ok) { Stop-Essai "l'écran n'a pas tourné (capture .tmp/tablette-portrait.png)" }
    Write-Output "portrait : $($tourne -join ' ')"
    if ($tourne[2] -lt $base[2] + 200) { Stop-Essai 'trait perdu après la rotation (capture .tmp/tablette-portrait.png)' }
    $dump = @(Invoke-Adb @('shell', 'dumpsys', 'window')) -join "`n"
    if ($dump -notmatch 'mCurrentFocus=.*io\.github\.jejeasterix\.actcraft') { Stop-Essai "l'application n'a plus le focus après la rotation" }
    $arbre = Read-ArbreUi
    if (-not $arbre -or -not @(Get-Noeuds $arbre | Where-Object { $_.package -ceq $paquet }).Count) {
        Stop-Essai 'interface de l''application absente après la rotation (palette non visible)'
    }

    # Outil texte (Ctrl+T), appui bas sur la page : le clavier s'ouvre et le cadre du texte reste au-dessus de lui.
    $null = Invoke-Adb @('shell', 'input', 'keycombination', '113', '48')
    Start-Sleep -Seconds 1
    $w = $tourne[0]; $h = $tourne[1]
    $null = Invoke-Adb @('shell', 'input', 'tap', "$([int]($w / 2))", "$([int]($h * 0.7))")
    $clavier = $null
    $ok = Wait-Condition {
        $im = @(Invoke-AdbTour @('shell', 'dumpsys', 'input_method'))
        ($im -join "`n") -match 'mInputShown=true'
    } 60
    if (-not $ok) { Stop-Essai 'mInputShown=true absent de dumpsys input_method (clavier non ouvert)' }
    Start-Sleep -Seconds 3
    $fenetre = @(Invoke-Adb @('shell', 'dumpsys', 'window', 'InputMethod') 60) -join "`n"
    if ($fenetre -notmatch 'Rect\((\d+), (\d+) - (\d+), (\d+)\)') { Stop-Essai 'rectangle du clavier illisible dans dumpsys window InputMethod' }
    $hautClavier = [int]$Matches[2]
    $avecClavier = Get-Mesures (Get-Capture 'clavier')
    Write-Output "clavier : haut $hautClavier, bas du cadre du texte $($avecClavier[3])"
    if ($avecClavier[3] -lt 0) { Stop-Essai 'cadre du texte introuvable sur la capture (.tmp/tablette-clavier.png)' }
    if ($avecClavier[3] -ge $hautClavier) { Stop-Essai "cadre du texte sous le clavier (bas $($avecClavier[3]) >= haut $hautClavier)" }
    Invoke-Adb @('logcat', '-d') | Set-Content -Encoding utf8 -LiteralPath (Join-Path $tmp 'tablette-logcat.txt')

    Stop-ActCraftLances
    if ($arretsRates) {
        Write-Output "ANDROID TABLETTE EN ECHEC : $($arretsRates -join ' ; ')"
        exit 1
    }
    Write-Output 'ANDROID TABLETTE OK'
    exit 0
} catch {
    # Refus du module commun : sa raison telle quelle ; le reste est imprévu, y compris un module introuvable.
    $raison = $null
    if (Get-Command Get-ActCraftAndroidRaison -ErrorAction SilentlyContinue) { $raison = Get-ActCraftAndroidRaison $_ }
    if (-not $raison) { $raison = "erreur imprévue : $($_.Exception.Message) (ligne $($_.InvocationInfo.ScriptLineNumber))" }
    Stop-Essai $raison
} finally {
    # Ctrl+C arrête le pipeline : le catch est sauté, pas ce bloc. Ce qui reste lancé est arrêté ici (rien après
    # Stop-Essai ou une fin réussie) ; la sortie est peut-être déjà fermée, d'où les écritures à l'hôte protégées.
    if ($emulateur -or $clientAdb -or $serveurAdb) {
        $lignes = @(Stop-ActCraftLances) + @($arretsRates)
        foreach ($ligne in $lignes) { try { Write-Host "arrêt (fin interrompue) : $ligne" } catch { $null = $_ } }
    }
}
