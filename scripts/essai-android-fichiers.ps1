# S5 sous Android (décision 17) : enregistrer puis rouvrir par content:// sur l'émulateur, empreintes égales.
# Usage : pwsh -NoProfile -File ./scripts/essai-android-fichiers.ps1 -Apk <…-x86_64.apk> [-Avd <nom>]
# Démarre l'émulateur (sans fenêtre, -gpu host : SwiftShader rend faux, VERIF s3-rendu de
# docs/verifications/2026-10-14-outillage-paquets.md), efface par MediaStore les fichiers des passages précédents
# (état initial vérifié), installe et lance l'APK, trace un trait au doigt, puis « Enregistrer sous » (Ctrl+Maj+S) →
# sélecteur du système (DocumentsUI, ACTION_CREATE_DOCUMENT), nom du passage saisi et relu → « ACTCRAFT ENREGISTRE
# <empreinte> » dans logcat, fichier de ce nom dans MediaStore ; Fermer (Ctrl+W), Ouvrir (Ctrl+O) → sélecteur → le
# fichier, par son nom exact → « ACTCRAFT OUVERT <empreinte> ». Refuse de démarrer si un émulateur occupe déjà le port
# de l'essai. Appels adb : scripts/android-commun.ps1.
# Dernière ligne : « ANDROID FICHIERS OK » (0) ou « ANDROID FICHIERS EN ECHEC : <raison> » (1), même sur une erreur
# imprévue. Sorties de diagnostic sous le .tmp/ du dépôt du script, quel que soit le dossier courant (fichiers-*.png,
# fichiers-*.xml, fichiers-logcat.txt). Ce que l'essai a
# lancé (émulateur et ses descendants, qemu compris ; client et serveur adb) est arrêté par les processus qu'il tient,
# jamais par nom ni par un PID relu, y compris sur Ctrl+C (bloc finally) ; un serveur adb que l'essai n'a pas lancé
# n'est jamais arrêté.
param([Parameter(Mandatory)][string]$Apk, [string]$Avd = '')
$ErrorActionPreference = 'Stop'
# Chemins absolus, calculés depuis l'emplacement du script : adb et python écrivent un chemin relatif dans le dossier
# courant du processus, que Set-Location ne change pas (lancé depuis un autre dossier, l'essai écrivait ses captures
# hors du dépôt : rejeu de la revue, 2026-10-17). Un APK donné en relatif l'est par rapport au dossier d'appel.
$Apk = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Apk)
$racine = Split-Path -Parent $PSScriptRoot
$tmp = Join-Path $racine '.tmp'
$arbreUi = Join-Path $tmp 'fichiers-ui.xml'
Set-Location $racine
$paquet = 'io.github.jejeasterix.actcraft'
# Port propre à l'essai : tout appel adb vise cet émulateur (ANDROID_SERIAL), jamais un autre appareil branché.
$port = 5584
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
    $journal = Join-Path $script:tmp 'fichiers-emulateur.txt'
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
            Adb 60 logcat -d | Set-Content -Encoding utf8 -LiteralPath (Join-Path $script:tmp 'fichiers-echec-logcat.txt')
            $null = Adb 60 shell screencap -p /sdcard/actcraft-echec.png
            $null = Adb 60 pull /sdcard/actcraft-echec.png (Join-Path $script:tmp 'fichiers-echec.png')
            $raison += ' (journal .tmp/fichiers-echec-logcat.txt, capture .tmp/fichiers-echec.png)'
        } catch { $null = $_ }
    }
    Stop-ActCraftLances
    if ($script:arretsRates) { $raison += " ; $($script:arretsRates -join ' ; ')" }
    Write-Output "ANDROID FICHIERS EN ECHEC : $raison"
    exit 1
}
# Adaptateur sur l'appel borné du module commun (Adb) : aucune copie ; ses exceptions arrivent au catch final.
function Invoke-Adb([string[]]$arguments, [int]$secondes = 30) {
    return @(Adb $secondes @arguments)
}
# Appel d'un tour d'attente bornée : un adb sans réponse (émulateur figé une minute sous charge, boîtes « ne répond
# pas » de gms et du lanceur : passage de la mutation, essai 3) compte comme un tour manqué, pas comme un échec ;
# l'attente bornée de l'étape décide. Rend $null après un tel refus ; toute autre erreur remonte.
function Invoke-AdbTour([string[]]$arguments, [int]$secondes = 30) {
    try { return @(Invoke-Adb $arguments $secondes) } catch {
        $raison = Get-ActCraftAndroidRaison $_
        if (-not $raison) { throw }
        Write-Host "adb : $raison, tour suivant"
        return $null
    }
}
# Arbre de l'interface Android (uiautomator), ou $null s'il n'a pas pu être pris (« could not get idle state » tant
# qu'une fenêtre s'anime). Dernier arbre lu : .tmp/fichiers-ui.xml.
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
        $attendre = @($arbre.SelectNodes('//node')) | Where-Object { $_.'resource-id' -eq 'android:id/aerr_wait' } | Select-Object -First 1
        if (-not $attendre -or $boite -eq 3) { break }
        Write-Host "boîte « ne répond pas » du système écartée : $(@($arbre.SelectNodes('//node')) | Where-Object { $_.'resource-id' -eq 'android:id/alertTitle' } | ForEach-Object { $_.text })"
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
# Arbre et capture de l'écran laissés sous .tmp/fichiers-<quoi>.xml|png ; rend le texte des chemins pour la raison.
function Save-Diagnostic([string]$quoi) {
    $nom = "fichiers-$quoi"
    if (Test-Path -LiteralPath $script:arbreUi) { Copy-Item -LiteralPath $script:arbreUi (Join-Path $script:tmp "$nom.xml") -Force }
    $null = Invoke-Adb @('shell', 'screencap', '-p', '/sdcard/actcraft-ecran.png') 60
    $null = Invoke-Adb @('pull', '/sdcard/actcraft-ecran.png', (Join-Path $script:tmp "$nom.png")) 60
    return "arbre .tmp/$nom.xml, capture .tmp/$nom.png"
}
# Ouvre un sélecteur du système par un raccourci de l'application, puis touche le nœud voulu. Critères indépendants
# de la langue et de la vue (liste ou grille) : paquet DocumentsUI, identifiant de ressource, texte choisi par nous
# (nom du fichier, cherché par son texte exact quel que soit l'identifiant du nœud). Attente bornée
# qui relit l'arbre ; l'indication « plein écran » du système (immersive cling, qui prend les touches) est écartée
# par son identifiant et le raccourci renvoyé ; renvoyé aussi une fois si aucune fenêtre de DocumentsUI n'est
# apparue en 20 s. Rend $null, ou la raison de l'échec avec les chemins de l'arbre et de la capture laissés sous .tmp/.
function Invoke-Selecteur([string[]]$touches, [string]$quoi, [string]$id, [string]$texte = '', [string]$classe = '',
    [int]$secondes = 120) {
    if ($touches) { $null = Invoke-Adb (@('shell', 'input', 'keycombination') + $touches) }
    $envoi = Get-Date
    $renvoye = $false
    $fin = $envoi.AddSeconds($secondes)
    while ((Get-Date) -lt $fin) {
        $arbre = Read-ArbreUi
        if ($arbre) {
            $noeuds = @($arbre.SelectNodes('//node'))
            if ($noeuds | Where-Object { $_.'resource-id' -eq 'android:id/immersive_cling_title' }) {
                $ok = $noeuds | Where-Object { $_.'resource-id' -eq 'android:id/ok' -and $_.package -eq 'android' } | Select-Object -First 1
                if ($ok) { Invoke-Tap $ok }
                Write-Host 'indication « plein écran » du système écartée, raccourci renvoyé'
                Start-Sleep -Seconds 2
                if ($touches) { $null = Invoke-Adb (@('shell', 'input', 'keycombination') + $touches) }
                $envoi = Get-Date
                continue
            }
            $selecteur = @($noeuds | Where-Object { $_.package -cmatch 'documentsui' })
            $noeud = $selecteur | Where-Object {
                $_.enabled -eq 'true' -and $_.'resource-id' -cmatch $id -and $_.text -cmatch $texte -and $_.class -cmatch $classe
            } | Select-Object -First 1
            if ($noeud) { Invoke-Tap $noeud; return $null }
            if ($touches -and -not $renvoye -and $selecteur.Count -eq 0 -and ((Get-Date) - $envoi).TotalSeconds -gt 20) {
                Write-Host 'aucun sélecteur après 20 s : raccourci renvoyé'
                $null = Invoke-Adb (@('shell', 'input', 'keycombination') + $touches)
                $envoi = Get-Date
                $renvoye = $true
            }
        }
        Start-Sleep -Seconds 2
    }
    $critere = (@($id, $texte) | Where-Object { $_ }) -join ' de texte '
    return "$quoi : nœud $critere introuvable en $secondes s ($(Save-Diagnostic $quoi))"
}
# Champ du nom du sélecteur d'enregistrement (EditText android:id/title de DocumentsUI), ou $null.
function Get-ChampNom($arbre) {
    return @($arbre.SelectNodes('//node')) | Where-Object {
        $_.package -cmatch 'documentsui' -and $_.'resource-id' -eq 'android:id/title' -and $_.class -cmatch 'EditText'
    } | Select-Object -First 1
}
# Saisit le nom dans le champ du sélecteur, puis le relit dans l'arbre jusqu'à le trouver exact. « input text » passe
# par le clavier du système : le champ se remplit après le retour de la commande, et « SAVE » touché trop tôt
# enregistre un nom tronqué (« ess », essai 2 du 2026-10-17). Le champ est touché, tout sélectionné (Ctrl+A) et le nom
# ressaisi tant qu'il n'est ni exact ni en train de se remplir (début du nom qui s'allonge d'une lecture à l'autre) ;
# exact seulement à deux lectures de suite (un reste de saisie arrivé entre-temps le fausserait). Attente bornée.
# Rend $null, ou la raison de l'échec avec les chemins de l'arbre et de la capture.
function Set-NomFichier([string]$nom, [int]$secondes = 180) {
    $fin = (Get-Date).AddSeconds($secondes)
    $saisies = 0
    $lu = $null
    while ((Get-Date) -lt $fin) {
        $arbre = Read-ArbreUi
        $champ = if ($arbre) { Get-ChampNom $arbre }
        if (-not $champ) { $lu = $null; Start-Sleep -Seconds 2; continue }
        $precedent = $lu
        $lu = "$($champ.text)"
        if ($lu -ceq $nom) {
            if ($precedent -ceq $nom) { Write-Host "nom : « $nom » relu deux fois ($saisies saisie(s))"; return $null }
            Start-Sleep -Seconds 1
            continue
        }
        if ($saisies -gt 0 -and $lu -and $nom.StartsWith($lu, [StringComparison]::Ordinal) -and
            $lu.Length -gt "$precedent".Length) {
            Start-Sleep -Seconds 1
            continue
        }
        if ($saisies -gt 0) { Write-Host "nom : lu « $lu » au lieu de « $nom », ressaisi" }
        Invoke-Tap $champ
        $null = Invoke-Adb @('shell', 'input', 'keycombination', '113', '29') # Ctrl+A : le contenu du champ est remplacé
        $null = Invoke-Adb @('shell', 'input', 'text', $nom)
        $saisies++
        $lu = $null
    }
    return "nom : champ « $lu » au lieu de « $nom » après $saisies saisie(s) en $secondes s ($(Save-Diagnostic 'nom'))"
}
# Fichier de l'essai : le nom exact que l'essai génère (essai-AAAAMMJJ-HHMMSS.actcraft, chiffres ASCII), à la racine
# d'un dossier où le sélecteur enregistre (Download, Documents), vu par /sdcard ou par /storage/emulated/0. Rien
# d'autre n'est effacé : ni un autre paperboard, ni un fichier de l'utilisateur qui commence par « essai », ni un
# sous-dossier, ni Android/data ; un nom qui porte un caractère du shell (« ' », « ; ») est hors du motif.
function Test-NomEssai([string]$nom) { return $nom -cmatch '^essai-[0-9]{8}-[0-9]{6}\.actcraft\z' }
function Test-CheminEssai([string]$chemin) {
    return $chemin -cmatch '^/(sdcard|storage/emulated/0)/(Download|Documents)/essai-[0-9]{8}-[0-9]{6}\.actcraft\z'
}
# Fichiers de l'essai vus par MediaStore, la base d'où le fournisseur des téléchargements (racine « Downloads » de
# DocumentsUI) tire sa liste : objets { Id ; Nom ; Chemin }. Exception si le fournisseur ne répond pas par des
# lignes (« Row: … ») ou par « No result found. ».
function Get-FichiersMediaStore {
    $sortie = @(Invoke-Adb @('shell', 'content', 'query', '--uri', 'content://media/external/file', '--projection',
            '_id:_display_name:_data', '--where', "`"_data LIKE '/storage/emulated/0/%'`"") 60)
    if (-not ($sortie | Where-Object { $_ -cmatch '^Row: |^No result found\.' })) {
        Lever-ActCraftAndroid "MediaStore illisible : $(($sortie | Select-Object -First 2) -join ' ')"
    }
    foreach ($ligne in $sortie) {
        if ($ligne -cmatch '_id=([0-9]+), _display_name=(.*), _data=(.*)$') {
            $nom = $Matches[2].Trim(); $chemin = $Matches[3].Trim()
            if ((Test-NomEssai $nom) -and (Test-CheminEssai $chemin) -and $chemin.EndsWith("/$nom", [StringComparison]::Ordinal)) {
                [pscustomobject]@{ Id = $Matches[1]; Nom = $nom; Chemin = $chemin }
            }
        }
    }
}
# Fichiers de l'essai présents sur le stockage partagé, à la racine des dossiers du sélecteur, en chemins.
function Get-FichiersStockage {
    return @(Invoke-Adb @('shell', 'find', '/sdcard/Download', '/sdcard/Documents', '-maxdepth', '1', '-type', 'f') 60 |
            Where-Object { Test-CheminEssai $_ })
}
# État initial propre : chaque fichier de l'essai est effacé par MediaStore (content delete : la ligne et le fichier),
# ce qui le retire de la liste du sélecteur, puis du stockage s'il en restait un hors de MediaStore. Vérifié par une
# nouvelle lecture des deux ; attente bornée (MediaStore peut ne pas répondre juste après le démarrage).
function Clear-FichiersEssai([int]$secondes = 120) {
    $fin = (Get-Date).AddSeconds($secondes)
    while ($true) {
        $restes = $null
        try {
            foreach ($f in @(Get-FichiersMediaStore)) {
                $null = Invoke-Adb @('shell', 'content', 'delete', '--uri', "content://media/external/file/$($f.Id)") 60
                Write-Output "état initial : « $($f.Nom) » effacé par MediaStore (id $($f.Id))"
            }
            foreach ($chemin in Get-FichiersStockage) {
                # Garde : seul un chemin du motif de l'essai atteint le shell de l'appareil, et entre apostrophes
                # échappées (« ' » → « '\'' »), même si le filtre ci-dessus venait à changer.
                if (-not (Test-CheminEssai $chemin)) { Stop-Essai "état initial : chemin hors du motif de l'essai refusé : $chemin" }
                $null = Invoke-Adb @('shell', 'rm', '-f', "'$($chemin.Replace("'", "'\''"))'")
                Write-Output "état initial : $chemin effacé du stockage"
            }
            $restes = @(@(Get-FichiersMediaStore | ForEach-Object { $_.Nom }) + @(Get-FichiersStockage))
            if ($restes.Count -eq 0) { Write-Output 'état initial : aucun fichier d''essai (MediaStore et stockage)'; return }
        } catch {
            $restes = @(Get-ActCraftAndroidRaison $_)
            if (-not $restes[0]) { throw }
        }
        if ((Get-Date) -gt $fin) { Stop-Essai "état initial : fichiers d'essai restants en $secondes s : $($restes -join ', ')" }
        Start-Sleep -Seconds 3
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
# Empreintes des lignes ENREGISTRE et OUVERT (casse comprise) : rend $null si elles sont égales et non vides, sinon la
# raison de l'échec. Deux empreintes illisibles ne sont pas « égales ».
function Compare-Empreintes([string]$enregistre, [string]$ouvert) {
    $empreinteEnregistree = [regex]::Match($enregistre, 'ACTCRAFT ENREGISTRE ([0-9a-f]{16})').Groups[1].Value
    $empreinteRelue = [regex]::Match($ouvert, 'ACTCRAFT OUVERT ([0-9a-f]{16})').Groups[1].Value
    if (-not $empreinteEnregistree -or $empreinteEnregistree -cne $empreinteRelue) {
        return "empreintes différentes : $(if ($empreinteEnregistree) { $empreinteEnregistree } else { '(vide)' }) enregistrée, $(if ($empreinteRelue) { $empreinteRelue } else { '(vide)' }) relue"
    }
    return $null
}

try {
    New-Item -ItemType Directory -Force $tmp | Out-Null
    . "$PSScriptRoot/dev-env-android.ps1"
    . "$PSScriptRoot/android-commun.ps1"
    Initialize-ActCraftAdb
    if (-not (Test-Path -LiteralPath $Apk)) { Stop-Essai "APK introuvable : $Apk" }
    Assert-PortEssaiLibre
    $env:ANDROID_SERIAL = "emulator-$port"
    Start-ActCraftServeurAdb
    # Même refus vu d'adb (seul contrôle hors de Windows) : un appareil emulator-<port> déjà connu servirait d'appareil.
    if (@(Invoke-Adb @('devices') 30) | Where-Object { $_ -cmatch "^emulator-$port\s" }) {
        Stop-Essai "emulator-$port déjà connu d'adb avant le démarrage de l'essai : émulateur d'un passage précédent ?"
    }
    Start-ActCraftEmulateur -Avd $Avd
    Wait-ActCraftDemarrage
    $descendantsEmulateur = @(Get-Descendants $emulateur $lancementEmulateur)
    Write-Output "émulateur : descendants $(($descendantsEmulateur | ForEach-Object { "$(Get-NomProcessus $_) $($_.Id)" }) -join ', ')"
    Ecarter-Anr
    Write-Output "appareil : API $("$(Invoke-Adb @('shell', 'getprop', 'ro.build.version.sdk'))".Trim()), $("$(Invoke-Adb @('shell', 'getprop', 'ro.product.cpu.abi'))".Trim()), langue $("$(Invoke-Adb @('shell', 'getprop', 'persist.sys.locale'))$(Invoke-Adb @('shell', 'getprop', 'ro.product.locale'))".Trim())"
    # Indication « plein écran » du système déjà confirmée : sinon elle prend les touches au premier plein écran.
    $null = Invoke-Adb @('shell', 'settings', 'put', 'secure', 'immersive_mode_confirmations', 'confirmed')
    # Fichiers des passages précédents effacés par MediaStore, d'où le sélecteur tire sa liste (l'AVD garde ses
    # données d'un démarrage à l'autre) : le sélecteur ne montrera que celui de ce passage. Vérifié, sinon échec.
    Clear-FichiersEssai
    # La clé de débogage est propre à chaque dossier de construction (build/android-debug.keystore) : un APK installé
    # depuis un autre worktree a une autre signature, et « install -r » le refuse (INSTALL_FAILED_UPDATE_INCOMPATIBLE).
    # Désinstallé d'abord (code ignoré s'il est absent) ; l'émulateur n'a aucune donnée réelle.
    $null = Invoke-Adb @('uninstall', $paquet) 60
    $processus = Install-ActCraftApk -Apk $Apk -Paquet $paquet
    Write-Output "processus : $processus"

    # Un trait au doigt au milieu de la page (tablette 2560 × 1600, comme essai-android.ps1), contrôlé sur la capture :
    # un document vide s'enregistrerait et se relirait aussi à l'identique.
    # Glissement rejoué (trois fois au plus) s'il tombe pendant un changement de disposition (passage en plein écran).
    $captureTrait = Join-Path $tmp 'fichiers-trait.png'
    for ($essai = 0; $essai -lt 3; $essai++) {
        Start-Sleep -Seconds 3
        $null = Invoke-AdbTour @('shell', 'input', 'swipe', '768', '800', '1408', '800', '600')
        Start-Sleep -Seconds 2
        Remove-Item -ErrorAction SilentlyContinue -LiteralPath $captureTrait # jamais la capture d'un passage précédent
        $null = Invoke-AdbTour @('shell', 'screencap', '-p', '/sdcard/actcraft-trait.png') 60
        $null = Invoke-AdbTour @('pull', '/sdcard/actcraft-trait.png', $captureTrait) 60
        if (-not (Test-Path -LiteralPath $captureTrait)) { $trait = @(); continue }
        $trait = @("$(& $python -I (Join-Path $PSScriptRoot 'lire-pixel.py') $captureTrait 1088 800)".Trim().Split(' ') | ForEach-Object { [int]$_ })
        Write-Output "trait : lu $($trait -join ' ')"
        if ($trait.Count -eq 3 -and $trait[0] -lt 100 -and $trait[1] -lt 100 -and $trait[2] -lt 100) { break }
    }
    if ($trait.Count -ne 3 -or $trait[0] -ge 100 -or $trait[1] -ge 100 -or $trait[2] -ge 100) { Stop-Essai 'trait absent après trois glissements (capture .tmp/fichiers-trait.png)' }

    # Ctrl+Maj+S → DocumentsUI (ACTION_CREATE_DOCUMENT). Nom propre au passage dans le champ du nom (EditText
    # android:id/title), relu avant « SAVE » : un nom déjà pris change le déroulé (« (1) », remplacement), un nom
    # tronqué rend le fichier introuvable à l'ouverture.
    $nomFichier = "essai-$(Get-Date -Format 'yyyyMMdd-HHmmss').actcraft"
    Write-Output "nom : $nomFichier"
    $echec = Invoke-Selecteur @('113', '59', '47') 'enregistrer' '^android:id/title$' '' 'EditText'
    if ($echec) { Stop-Essai $echec }
    $echec = Set-NomFichier $nomFichier
    if ($echec) { Stop-Essai $echec }
    # Bouton d'enregistrement (android:id/button1), touché jusqu'à la fermeture du sélecteur (trois fois au plus).
    for ($essai = 0; $essai -lt 3; $essai++) {
        $echec = Invoke-Selecteur @() 'enregistrer' '^android:id/button1$' '' '' 30
        if ($echec) { if ($essai -eq 0) { Stop-Essai $echec } else { break } }
        Start-Sleep -Seconds 3
        $arbre = Read-ArbreUi
        if ($arbre -and -not @($arbre.SelectNodes('//node') | Where-Object { $_.package -cmatch 'documentsui' })) { break }
    }
    $enregistre = Wait-Journal 'ACTCRAFT ENREGISTRE [0-9a-f]{16}'
    if (-not $enregistre) { Stop-Essai 'aucune ligne ACTCRAFT ENREGISTRE' }
    Write-Output $enregistre
    # Le fichier écrit porte le nom exact, seul fichier de l'essai dans MediaStore (attente bornée de l'indexation).
    $fin = (Get-Date).AddSeconds(30)
    do {
        $ecrits = @(try { Get-FichiersMediaStore } catch { if (-not (Get-ActCraftAndroidRaison $_)) { throw }; Write-Host "MediaStore : $(Get-ActCraftAndroidRaison $_)" })
        if ($ecrits.Count -eq 1 -and $ecrits[0].Nom -ceq $nomFichier) { break }
        Start-Sleep -Seconds 2
    } while ((Get-Date) -lt $fin)
    if ($ecrits.Count -ne 1 -or $ecrits[0].Nom -cne $nomFichier) {
        Stop-Essai "fichiers de l'essai après l'enregistrement : « $(($ecrits | ForEach-Object { $_.Nom }) -join ' », « ') » au lieu de « $nomFichier » seul"
    }
    Write-Output "fichier : $($ecrits[0].Chemin) (MediaStore id $($ecrits[0].Id))"
    # Journal vidé : la ligne OUVERT attendue ne peut venir que de l'ouverture qui suit.
    $null = Invoke-Adb @('logcat', '-c')
    if ($LASTEXITCODE -ne 0) { Stop-Essai "adb logcat -c (code $LASTEXITCODE) : journal non vidé avant l'ouverture$(Get-ActCraftAdbErreur)" }

    $null = Invoke-Adb @('shell', 'input', 'keycombination', '113', '51') # Ctrl+W
    Start-Sleep -Seconds 2
    # Ctrl+O, puis le paperboard de cet essai dans DocumentsUI (ACTION_OPEN_DOCUMENT), par son texte exact, quel que
    # soit l'identifiant du nœud : même critère en vue liste et en vue grille.
    $echec = Invoke-Selecteur @('113', '43') 'ouvrir' '' "^$([regex]::Escape($nomFichier))$"
    if ($echec) { Stop-Essai $echec }
    $ouvert = Wait-Journal 'ACTCRAFT OUVERT [0-9a-f]{16}'
    if (-not $ouvert) { Stop-Essai 'aucune ligne ACTCRAFT OUVERT' }
    Write-Output $ouvert
    Invoke-Adb @('logcat', '-d') | Set-Content -Encoding utf8 -LiteralPath (Join-Path $tmp 'fichiers-logcat.txt')

    $echec = Compare-Empreintes $enregistre $ouvert
    if ($echec) { Stop-Essai $echec }
    Stop-ActCraftLances
    if ($arretsRates) {
        Write-Output "ANDROID FICHIERS EN ECHEC : $($arretsRates -join ' ; ')"
        exit 1
    }
    Write-Output 'ANDROID FICHIERS OK'
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
