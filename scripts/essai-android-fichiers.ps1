# S5 sous Android (décision 17) : enregistrer puis rouvrir par content:// sur l'émulateur, empreintes égales.
# Usage : pwsh -NoProfile -File ./scripts/essai-android-fichiers.ps1 -Apk <…-x86_64.apk> [-Avd <nom>]
# Démarre l'émulateur (sans fenêtre, -gpu host : SwiftShader rend faux, VERIF s3-rendu de
# docs/verifications/2026-10-14-outillage-paquets.md), installe et lance l'APK, trace un trait au doigt, puis
# « Enregistrer sous » (Ctrl+Maj+S) → sélecteur du système (DocumentsUI, ACTION_CREATE_DOCUMENT) → « ACTCRAFT
# ENREGISTRE <empreinte> » dans logcat ; Fermer (Ctrl+W), Ouvrir (Ctrl+O) → sélecteur → le fichier → « ACTCRAFT OUVERT
# <empreinte> ». Arrête l'émulateur par son PID, quoi qu'il arrive. Appels adb : scripts/android-commun.ps1.
# Dernière ligne : « ANDROID FICHIERS OK » (0) ou « ANDROID FICHIERS EN ECHEC : <raison> » (1), même sur une erreur
# imprévue. Sorties de diagnostic sous .tmp/ (fichiers-*.png, fichiers-ui.xml, fichiers-logcat.txt).
param([Parameter(Mandatory)][string]$Apk, [string]$Avd = '')
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
$paquet = 'io.github.jejeasterix.actcraft'
# Port propre à l'essai : tout appel adb vise cet émulateur (ANDROID_SERIAL), jamais un autre appareil branché.
$port = 5584
$emulateur = $null
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
function Stop-Essai([string]$raison) {
    Stop-ActCraftEmulateur
    Write-Output "ANDROID FICHIERS EN ECHEC : $raison"
    exit 1
}
# Adaptateur sur l'appel borné du module commun (Adb) : aucune copie ; ses exceptions arrivent au catch final.
function Invoke-Adb([string[]]$arguments, [int]$secondes = 30) {
    return @(Adb $secondes @arguments)
}
# Nœud de l'interface Android (uiautomator) dont le texte ou l'identifiant correspond, cherché jusqu'au délai ;
# touché en son centre. Rend le nœud (lignes d'information écrites à l'hôte), ou $null (copie de l'écran et de l'arbre sous .tmp/ pour le diagnostic).
function Invoke-Toucher([string]$motif, [int]$secondes = 45) {
    $fin = (Get-Date).AddSeconds($secondes)
    while ($true) {
        # Arbre absent (« could not get idle state » tant qu'une fenêtre s'anime) : nouvel essai jusqu'au délai.
        Remove-Item -ErrorAction SilentlyContinue .tmp/fichiers-ui.xml
        $null = Invoke-Adb @('shell', 'rm', '-f', '/sdcard/actcraft-ui.xml')
        $null = Invoke-Adb @('shell', 'uiautomator', 'dump', '/sdcard/actcraft-ui.xml') 60
        $null = Invoke-Adb @('pull', '/sdcard/actcraft-ui.xml', '.tmp/fichiers-ui.xml') 30
        $noeud = $null
        if (Test-Path .tmp/fichiers-ui.xml) {
            [xml]$ui = Get-Content -Raw -Encoding utf8 .tmp/fichiers-ui.xml
            $noeud = $ui.SelectNodes('//node') | Where-Object {
                $_.enabled -eq 'true' -and ($_.text -match $motif -or $_.'resource-id' -match $motif)
            } | Select-Object -First 1
        }
        if ($noeud) { break }
        if ((Get-Date) -gt $fin) {
            $null = Invoke-Adb @('shell', 'screencap', '-p', '/sdcard/actcraft-ecran.png') 60
            $null = Invoke-Adb @('pull', '/sdcard/actcraft-ecran.png', '.tmp/fichiers-ecran.png') 60
            return $null
        }
        Start-Sleep -Seconds 2
    }
    $b = @([regex]::Matches($noeud.bounds, '\d+') | ForEach-Object { [int]$_.Value })
    $x = [int](($b[0] + $b[2]) / 2); $y = [int](($b[1] + $b[3]) / 2)
    Write-Host "touché : « $($noeud.text) » ($($noeud.'resource-id')) en ($x, $y)"
    $null = Invoke-Adb @('shell', 'input', 'tap', "$x", "$y")
    return $noeud
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
    # Serveur adb démarré à part (sans handle hérité, dossier courant hors du dépôt) : démarré par le premier appel
    # borné, il hériterait de la sortie redirigée de ce script (« > fichier » resterait ouvert après la fin) et
    # verrouillerait le worktree. Sans effet s'il tourne déjà.
    $serveur = Start-Process -FilePath (Get-Command adb -CommandType Application | Select-Object -First 1).Source `
        -ArgumentList 'start-server' -WorkingDirectory ([IO.Path]::GetTempPath()) -WindowStyle Hidden -PassThru
    if (-not $serveur.WaitForExit(60000)) { Stop-Essai 'adb start-server sans réponse (60 s)' }
    $emulateur = Start-ActCraftEmulateur -Avd $Avd
    Wait-ActCraftDemarrage
    Ecarter-Anr
    Write-Output "appareil : API $("$(Invoke-Adb @('shell', 'getprop', 'ro.build.version.sdk'))".Trim()), $("$(Invoke-Adb @('shell', 'getprop', 'ro.product.cpu.abi'))".Trim()), langue $("$(Invoke-Adb @('shell', 'getprop', 'persist.sys.locale'))".Trim())"
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
    $null = Invoke-Adb @('shell', 'input', 'swipe', '768', '800', '1408', '800', '600')
    Start-Sleep -Seconds 2
    $null = Invoke-Adb @('shell', 'screencap', '-p', '/sdcard/actcraft-trait.png') 60
    $null = Invoke-Adb @('pull', '/sdcard/actcraft-trait.png', '.tmp/fichiers-trait.png') 60
    $trait = @("$(& $python -I scripts/lire-pixel.py .tmp/fichiers-trait.png 1088 800)".Trim().Split(' ') | ForEach-Object { [int]$_ })
    Write-Output "trait : lu $($trait -join ' ')"
    if ($trait.Count -ne 3 -or $trait[0] -ge 100 -or $trait[1] -ge 100 -or $trait[2] -ge 100) { Stop-Essai 'trait absent après le glissement' }

    $null = Invoke-Adb @('shell', 'input', 'keycombination', '113', '59', '47') # Ctrl+Maj+S
    if (-not (Invoke-Toucher '(?i)^(save|enregistrer)$|android:id/button1')) { Stop-Essai 'bouton d''enregistrement du sélecteur introuvable' }
    $enregistre = Wait-Journal 'ACTCRAFT ENREGISTRE [0-9a-f]{16}'
    if (-not $enregistre) { Stop-Essai 'aucune ligne ACTCRAFT ENREGISTRE' }
    Write-Output $enregistre
    # Journal vidé : la ligne OUVERT attendue ne peut venir que de l'ouverture qui suit.
    $null = Invoke-Adb @('logcat', '-c')

    $null = Invoke-Adb @('shell', 'input', 'keycombination', '113', '51') # Ctrl+W
    Start-Sleep -Seconds 2
    $null = Invoke-Adb @('shell', 'input', 'keycombination', '113', '43') # Ctrl+O
    if (-not (Invoke-Toucher '\.actcraft$')) { Stop-Essai 'fichier enregistré absent du sélecteur' }
    $ouvert = Wait-Journal 'ACTCRAFT OUVERT [0-9a-f]{16}'
    if (-not $ouvert) { Stop-Essai 'aucune ligne ACTCRAFT OUVERT' }
    Write-Output $ouvert
    Invoke-Adb @('logcat', '-d') | Set-Content -Encoding utf8 .tmp/fichiers-logcat.txt

    $a = [regex]::Match($enregistre, 'ACTCRAFT ENREGISTRE ([0-9a-f]{16})').Groups[1].Value
    $b = [regex]::Match($ouvert, 'ACTCRAFT OUVERT ([0-9a-f]{16})').Groups[1].Value
    if ($a -ne $b) { Stop-Essai "empreintes différentes : $a enregistrée, $b relue" }
    Stop-ActCraftEmulateur
    Write-Output 'ANDROID FICHIERS OK'
    exit 0
} catch {
    # Refus du module commun : sa raison telle quelle ; le reste est imprévu, y compris un module introuvable.
    $raison = $null
    if (Get-Command Get-ActCraftAndroidRaison -ErrorAction SilentlyContinue) { $raison = Get-ActCraftAndroidRaison $_ }
    if (-not $raison) { $raison = "erreur imprévue : $($_.Exception.Message) (ligne $($_.InvocationInfo.ScriptLineNumber))" }
    Stop-Essai $raison
}
