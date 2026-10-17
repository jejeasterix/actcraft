# S5 sous Android (décision 17) : enregistrer puis rouvrir par content:// sur l'émulateur, empreintes égales.
# Usage : pwsh -NoProfile -File ./scripts/essai-android-fichiers.ps1 -Apk <…-x86_64.apk> [-Avd <nom>]
# Démarre l'émulateur (sans fenêtre, -gpu host : SwiftShader rend faux, VERIF s3-rendu de
# docs/verifications/2026-10-14-outillage-paquets.md), installe et lance l'APK, trace un trait au doigt, puis
# « Enregistrer sous » (Ctrl+Maj+S) → sélecteur du système (DocumentsUI, ACTION_CREATE_DOCUMENT) → « ACTCRAFT
# ENREGISTRE <empreinte> » dans logcat ; Fermer (Ctrl+W), Ouvrir (Ctrl+O) → sélecteur → le fichier → « ACTCRAFT OUVERT
# <empreinte> ». Arrête l'émulateur par son PID, quoi qu'il arrive. Appels adb : scripts/android-commun.ps1.
# Dernière ligne : « ANDROID FICHIERS OK » (0) ou « ANDROID FICHIERS EN ECHEC : <raison> » (1), même sur une erreur
# imprévue. Sorties de diagnostic sous .tmp/ (fichiers-*.png, fichiers-*.xml, fichiers-logcat.txt) ; le serveur adb
# et l'émulateur démarrés par l'essai sont arrêtés par leur PID.
param([Parameter(Mandatory)][string]$Apk, [string]$Avd = '')
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
$paquet = 'io.github.jejeasterix.actcraft'
# Port propre à l'essai : tout appel adb vise cet émulateur (ANDROID_SERIAL), jamais un autre appareil branché.
$port = 5584
$emulateur = $null
$serveurAdb = $null
$python = if (-not $IsWindows -and (Get-Command python3 -ErrorAction SilentlyContinue)) { 'python3' } else { 'python' }

# Démarrage local de l'émulateur : sans fenêtre, -gpu host, démarrage à froid sans instantané (rien d'enregistré à
# l'arrêt), dossier courant hors du dépôt (le worktree n'est pas verrouillé). Le processus est gardé pour l'arrêt.
function Start-ActCraftEmulateur([string]$Avd) {
    $exe = (Get-Command emulator -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1).Source
    if (-not $exe) { Stop-Essai 'emulator introuvable dans le PATH (dev-env-android.ps1)' }
    if (-not $Avd) {
        $Avd = @(& $exe -list-avds | Where-Object { $_ -match '^[\w.-]+$' }) | Select-Object -First 1
        if (-not $Avd) { Stop-Essai 'aucun AVD (emulator -list-avds)' }
    }
    $journal = Join-Path (Get-Location) '.tmp/fichiers-emulateur.txt'
    $p = Start-Process -FilePath $exe -PassThru -NoNewWindow -WorkingDirectory (Split-Path $exe) `
        -RedirectStandardOutput $journal -RedirectStandardError "$journal.err" `
        -ArgumentList @('-avd', $Avd, '-port', "$port", '-no-window', '-gpu', 'host', '-no-snapshot', '-no-boot-anim', '-no-audio')
    Write-Host "émulateur : $Avd, PID $($p.Id), port $port"
    Start-Sleep -Seconds 5
    if ($p.HasExited) { Stop-Essai "émulateur arrêté au démarrage (code $($p.ExitCode)) : $((Get-Content "$journal.err", $journal -ErrorAction SilentlyContinue | Select-Object -Last 2) -join ' ')" }
    return $p
}
# Arrêt par PID : l'émulateur et ses processus enfants (qemu), sans rien enregistrer.
function Stop-ActCraftEmulateur {
    if (-not $script:emulateur) { return }
    $p = $script:emulateur
    $script:emulateur = $null
    try { $p.Kill($true) } catch { $null = $_ }
    $null = $p.WaitForExit(30000)
    Write-Output "émulateur : PID $($p.Id) arrêté"
}
# Serveur adb démarré à part (sans handle hérité, dossier courant hors du dépôt) : démarré par le premier appel borné,
# il hériterait de la sortie redirigée de ce script (« > fichier » resterait ouvert après la fin) et verrouillerait le
# worktree. S'il ne tournait pas avant, son PID (port 5037, Windows) est gardé pour l'arrêter à la fin.
function Start-ActCraftServeurAdb {
    $avant = if ($IsWindows) { @(Get-NetTCPConnection -LocalPort 5037 -State Listen -ErrorAction SilentlyContinue) } else { @() }
    $client = Start-Process -FilePath (Get-Command adb -CommandType Application | Select-Object -First 1).Source `
        -ArgumentList 'start-server' -WorkingDirectory ([IO.Path]::GetTempPath()) -WindowStyle Hidden -PassThru
    if (-not $client.WaitForExit(60000)) { Stop-Essai 'adb start-server sans réponse (60 s)' }
    if ($IsWindows -and $avant.Count -eq 0) {
        $script:serveurAdb = (Get-NetTCPConnection -LocalPort 5037 -State Listen -ErrorAction SilentlyContinue | Select-Object -First 1).OwningProcess
        Write-Output "adb : serveur démarré, PID $script:serveurAdb"
    }
}
function Stop-ActCraftServeurAdb {
    if (-not $script:serveurAdb) { return }
    $id = $script:serveurAdb
    $script:serveurAdb = $null
    Stop-Process -Id $id -Force -ErrorAction SilentlyContinue
    Write-Output "adb : serveur PID $id arrêté"
}
function Stop-Essai([string]$raison) {
    if ($script:emulateur -and $script:ActCraftAdb) {
        try {
            Adb 60 logcat -d | Set-Content -Encoding utf8 .tmp/fichiers-echec-logcat.txt
            $null = Adb 60 shell screencap -p /sdcard/actcraft-echec.png
            $null = Adb 60 pull /sdcard/actcraft-echec.png .tmp/fichiers-echec.png
            $raison += ' (journal .tmp/fichiers-echec-logcat.txt, capture .tmp/fichiers-echec.png)'
        } catch { $null = $_ }
    }
    Stop-ActCraftEmulateur
    Stop-ActCraftServeurAdb
    Write-Output "ANDROID FICHIERS EN ECHEC : $raison"
    exit 1
}
# Adaptateur sur l'appel borné du module commun (Adb) : aucune copie ; ses exceptions arrivent au catch final.
function Invoke-Adb([string[]]$arguments, [int]$secondes = 30) {
    return @(Adb $secondes @arguments)
}
# Arbre de l'interface Android (uiautomator), ou $null s'il n'a pas pu être pris (« could not get idle state » tant
# qu'une fenêtre s'anime). Dernier arbre lu : .tmp/fichiers-ui.xml.
function Read-ArbreUi {
    Remove-Item -ErrorAction SilentlyContinue .tmp/fichiers-ui.xml
    $null = Invoke-Adb @('shell', 'rm', '-f', '/sdcard/actcraft-ui.xml')
    $null = Invoke-Adb @('shell', 'uiautomator', 'dump', '/sdcard/actcraft-ui.xml') 60
    $null = Invoke-Adb @('pull', '/sdcard/actcraft-ui.xml', '.tmp/fichiers-ui.xml') 30
    if (-not (Test-Path .tmp/fichiers-ui.xml)) { return $null }
    try { $arbre = [xml](Get-Content -Raw -Encoding utf8 .tmp/fichiers-ui.xml) } catch { return $null }
    return , $arbre # virgule : un XmlDocument rendu tel quel serait énuméré nœud par nœud
}
# Touche un nœud en son centre (ligne d'information écrite à l'hôte : rien n'est rendu).
function Invoke-Tap($noeud) {
    $b = @([regex]::Matches($noeud.bounds, '\d+') | ForEach-Object { [int]$_.Value })
    $x = [int](($b[0] + $b[2]) / 2); $y = [int](($b[1] + $b[3]) / 2)
    Write-Host "touché : « $($noeud.text) » ($($noeud.'resource-id'), $($noeud.package)) en ($x, $y)"
    $null = Invoke-Adb @('shell', 'input', 'tap', "$x", "$y")
}
# Ouvre un sélecteur du système par un raccourci de l'application, puis touche le nœud voulu. Critères indépendants
# de la langue : paquet DocumentsUI, identifiant de ressource, texte choisi par nous (nom du fichier). Attente bornée
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
            $selecteur = @($noeuds | Where-Object { $_.package -match 'documentsui' })
            $noeud = $selecteur | Where-Object {
                $_.enabled -eq 'true' -and $_.'resource-id' -match $id -and $_.text -match $texte -and $_.class -match $classe
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
    $nom = "fichiers-$quoi"
    if (Test-Path .tmp/fichiers-ui.xml) { Copy-Item .tmp/fichiers-ui.xml ".tmp/$nom.xml" -Force }
    $null = Invoke-Adb @('shell', 'screencap', '-p', '/sdcard/actcraft-ecran.png') 60
    $null = Invoke-Adb @('pull', '/sdcard/actcraft-ecran.png', ".tmp/$nom.png") 60
    return "$quoi : nœud $id introuvable en $secondes s (arbre .tmp/$nom.xml, capture .tmp/$nom.png)"
}
# Dernière ligne du journal qui correspond au motif, attendue jusqu'au délai.
function Wait-Journal([string]$motif, [int]$secondes = 60) {
    $fin = (Get-Date).AddSeconds($secondes)
    while ((Get-Date) -lt $fin) {
        $ligne = Invoke-Adb @('logcat', '-d') | Where-Object { $_ -match $motif } | Select-Object -Last 1
        if ($ligne) { return $ligne }
        Start-Sleep -Milliseconds 500
    }
    return $null
}

try {
    New-Item -ItemType Directory -Force .tmp | Out-Null
    . "$PSScriptRoot/dev-env-android.ps1"
    . "$PSScriptRoot/android-commun.ps1"
    Initialize-ActCraftAdb
    if (-not (Test-Path -LiteralPath $Apk)) { Stop-Essai "APK introuvable : $Apk" }
    $env:ANDROID_SERIAL = "emulator-$port"
    Start-ActCraftServeurAdb
    $emulateur = Start-ActCraftEmulateur -Avd $Avd
    Wait-ActCraftDemarrage
    Ecarter-Anr
    Write-Output "appareil : API $("$(Invoke-Adb @('shell', 'getprop', 'ro.build.version.sdk'))".Trim()), $("$(Invoke-Adb @('shell', 'getprop', 'ro.product.cpu.abi'))".Trim()), langue $("$(Invoke-Adb @('shell', 'getprop', 'persist.sys.locale'))$(Invoke-Adb @('shell', 'getprop', 'ro.product.locale'))".Trim())"
    # Indication « plein écran » du système déjà confirmée : sinon elle prend les touches au premier plein écran.
    $null = Invoke-Adb @('shell', 'settings', 'put', 'secure', 'immersive_mode_confirmations', 'confirmed')
    # Paperboards d'un essai précédent retirés : le sélecteur n'en montre qu'un, celui de cet essai.
    # « /sdcard/ » : /sdcard est un lien symbolique, que find ne suit pas sans la barre finale.
    $null = Invoke-Adb @('shell', 'find', '/sdcard/', '-name', "'*.actcraft'", '-delete')
    # La clé de débogage est propre à chaque dossier de construction (build/android-debug.keystore) : un APK installé
    # depuis un autre worktree a une autre signature, et « install -r » le refuse (INSTALL_FAILED_UPDATE_INCOMPATIBLE).
    # Désinstallé d'abord (code ignoré s'il est absent) ; l'émulateur n'a aucune donnée réelle.
    $null = Invoke-Adb @('uninstall', $paquet) 60
    $processus = Install-ActCraftApk -Apk $Apk -Paquet $paquet
    Write-Output "processus : $processus"

    # Un trait au doigt au milieu de la page (tablette 2560 × 1600, comme essai-android.ps1), contrôlé sur la capture :
    # un document vide s'enregistrerait et se relirait aussi à l'identique.
    # Glissement rejoué (trois fois au plus) s'il tombe pendant un changement de disposition (passage en plein écran).
    for ($essai = 0; $essai -lt 3; $essai++) {
        Start-Sleep -Seconds 3
        $null = Invoke-Adb @('shell', 'input', 'swipe', '768', '800', '1408', '800', '600')
        Start-Sleep -Seconds 2
        $null = Invoke-Adb @('shell', 'screencap', '-p', '/sdcard/actcraft-trait.png') 60
        $null = Invoke-Adb @('pull', '/sdcard/actcraft-trait.png', '.tmp/fichiers-trait.png') 60
        $trait = @("$(& $python -I scripts/lire-pixel.py .tmp/fichiers-trait.png 1088 800)".Trim().Split(' ') | ForEach-Object { [int]$_ })
        Write-Output "trait : lu $($trait -join ' ')"
        if ($trait.Count -eq 3 -and $trait[0] -lt 100 -and $trait[1] -lt 100 -and $trait[2] -lt 100) { break }
    }
    if ($trait.Count -ne 3 -or $trait[0] -ge 100 -or $trait[1] -ge 100 -or $trait[2] -ge 100) { Stop-Essai 'trait absent après trois glissements (capture .tmp/fichiers-trait.png)' }

    # Ctrl+Maj+S → DocumentsUI (ACTION_CREATE_DOCUMENT). Nom propre à l'essai dans le champ du nom (EditText
    # android:id/title) : les paperboards des essais précédents restent listés par le fournisseur des téléchargements
    # (un « find -delete » ne les retire pas de sa liste) et un nom déjà pris change le déroulé (« (1) », remplacement).
    $nomFichier = "essai-$(Get-Date -Format 'yyyyMMdd-HHmmss').actcraft"
    $echec = Invoke-Selecteur @('113', '59', '47') 'enregistrer' '^android:id/title$' '' 'EditText'
    if ($echec) { Stop-Essai $echec }
    $null = Invoke-Adb @('shell', 'input', 'keycombination', '113', '29') # Ctrl+A : le nom proposé est remplacé
    $null = Invoke-Adb @('shell', 'input', 'text', $nomFichier)
    Write-Output "nom : $nomFichier"
    # Bouton d'enregistrement (android:id/button1), touché jusqu'à la fermeture du sélecteur (trois fois au plus).
    for ($essai = 0; $essai -lt 3; $essai++) {
        $echec = Invoke-Selecteur @() 'enregistrer' '^android:id/button1$' '' '' 30
        if ($echec) { if ($essai -eq 0) { Stop-Essai $echec } else { break } }
        Start-Sleep -Seconds 3
        $arbre = Read-ArbreUi
        if ($arbre -and -not @($arbre.SelectNodes('//node') | Where-Object { $_.package -match 'documentsui' })) { break }
    }
    $enregistre = Wait-Journal 'ACTCRAFT ENREGISTRE [0-9a-f]{16}'
    if (-not $enregistre) { Stop-Essai 'aucune ligne ACTCRAFT ENREGISTRE' }
    Write-Output $enregistre
    # Journal vidé : la ligne OUVERT attendue ne peut venir que de l'ouverture qui suit.
    $null = Invoke-Adb @('logcat', '-c')

    $null = Invoke-Adb @('shell', 'input', 'keycombination', '113', '51') # Ctrl+W
    Start-Sleep -Seconds 2
    # Ctrl+O, puis le paperboard de cet essai, par son nom, dans DocumentsUI (ACTION_OPEN_DOCUMENT).
    $echec = Invoke-Selecteur @('113', '43') 'ouvrir' '^android:id/title$' "^$([regex]::Escape($nomFichier))$"
    if ($echec) { Stop-Essai $echec }
    $ouvert = Wait-Journal 'ACTCRAFT OUVERT [0-9a-f]{16}'
    if (-not $ouvert) { Stop-Essai 'aucune ligne ACTCRAFT OUVERT' }
    Write-Output $ouvert
    Invoke-Adb @('logcat', '-d') | Set-Content -Encoding utf8 .tmp/fichiers-logcat.txt

    $a = [regex]::Match($enregistre, 'ACTCRAFT ENREGISTRE ([0-9a-f]{16})').Groups[1].Value
    $b = [regex]::Match($ouvert, 'ACTCRAFT OUVERT ([0-9a-f]{16})').Groups[1].Value
    if ($a -ne $b) { Stop-Essai "empreintes différentes : $a enregistrée, $b relue" }
    Stop-ActCraftEmulateur
    Stop-ActCraftServeurAdb
    Write-Output 'ANDROID FICHIERS OK'
    exit 0
} catch {
    # Refus du module commun : sa raison telle quelle ; le reste est imprévu, y compris un module introuvable.
    $raison = $null
    if (Get-Command Get-ActCraftAndroidRaison -ErrorAction SilentlyContinue) { $raison = Get-ActCraftAndroidRaison $_ }
    if (-not $raison) { $raison = "erreur imprévue : $($_.Exception.Message) (ligne $($_.InvocationInfo.ScriptLineNumber))" }
    Stop-Essai $raison
}
