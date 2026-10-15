# Essai de l'APK sur un émulateur déjà démarré (scénario S3) : installation, lancement, processus vivant après 10 s,
# palette visible sur la capture, trait d'un doigt visible sur la capture suivante.
# Usage : pwsh -NoProfile -File ./scripts/essai-android.ps1 -Apk <fichier.apk> [-Sortie <dossier>]
# adb doit être dans le PATH (dev-env-android.ps1, ou l'émulateur de la CI). Captures sous -Sortie (.tmp/android).
# Émulateur : -gpu host en local ; -gpu lavapipe sur un exécuteur sans GPU (CI) — SwiftShader rend faux (voir
# docs/verifications/2026-10-14-outillage-paquets.md, VERIF s3-rendu et s3-ci).
# Dernière ligne et code : « ANDROID OK » (0) ou « ANDROID EN ECHEC : <raison> » (1), même sur une erreur imprévue ;
# chaque commande adb a un délai (au-delà : échec, processus adb arrêté).
param([Parameter(Mandatory)][string]$Apk, [string]$Sortie = '.tmp/android')
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
$paquet = 'io.github.jejeasterix.actcraft'
# Sous Windows, « python3 » peut n'être que le raccourci du Store : on prend « python ».
$python = if (-not $IsWindows -and (Get-Command python3 -ErrorAction SilentlyContinue)) { 'python3' } else { 'python' }

function Echec([string]$Raison) {
    Write-Output "ANDROID EN ECHEC : $Raison"
    exit 1
}
# Adb <secondes> <arguments…> : lignes de la sortie standard, $LASTEXITCODE positionné. Au-delà du délai, l'arbre du
# processus adb est arrêté et l'essai échoue. Fonction simple ($args) : « -p », « -r »… restent des arguments d'adb.
function Adb {
    $secondes = [int]$args[0]
    $arguments = @($args | Select-Object -Skip 1 | ForEach-Object { "$_" })
    $info = [Diagnostics.ProcessStartInfo]::new($script:adb)
    foreach ($a in $arguments) { $info.ArgumentList.Add($a) }
    $info.UseShellExecute = $false
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.StandardOutputEncoding = [Text.Encoding]::UTF8
    $p = [Diagnostics.Process]::Start($info)
    $sortie = $p.StandardOutput.ReadToEndAsync()
    $null = $p.StandardError.ReadToEndAsync()
    if (-not $p.WaitForExit($secondes * 1000)) {
        try { $p.Kill($true) } catch { $null = $_ }
        Echec "adb $(($arguments | Select-Object -First 2) -join ' ') sans réponse ($secondes s)"
    }
    $p.WaitForExit()
    $global:LASTEXITCODE = $p.ExitCode
    return @($sortie.Result -split '\r?\n' | Where-Object { $_ -ne '' })
}
function Pixel([string]$Capture, [int]$X, [int]$Y) {
    $texte = & $python -I scripts/lire-pixel.py $Capture $X $Y
    if ($LASTEXITCODE -ne 0) { Echec "lecture du pixel ($X, $Y)" }
    return @("$texte".Trim().Split(' ') | ForEach-Object { [int]$_ })
}
function Proche($A, $B, [int]$Tolerance) {
    for ($i = 0; $i -lt 3; $i++) { if ([math]::Abs($A[$i] - $B[$i]) -gt $Tolerance) { return $false } }
    return $true
}
function Capture([string]$Nom) {
    Ecarter-Anr
    $fichier = Join-Path $Sortie "$Nom.png"
    $null = Adb 60 shell screencap -p "/sdcard/actcraft-$Nom.png"
    $null = Adb 60 pull "/sdcard/actcraft-$Nom.png" $fichier
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $fichier)) { Echec "capture $Nom" }
    return $fichier
}

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

try {
$script:adb = (Get-Command adb -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1).Source
if (-not $script:adb) { Echec 'adb introuvable dans le PATH' }
if (-not (Test-Path -LiteralPath $Apk)) { Echec "APK introuvable : $Apk" }

# Repère : couleur de la palette, lue dans le thème (décision 23).
$theme = Get-Content -Raw ui/qml/Theme.qml
if ($theme -notmatch 'property color palette:\s*"#([0-9A-Fa-f]{6})"') { Echec 'couleur « palette » introuvable dans ui/qml/Theme.qml' }
$hex = $Matches[1]
$fondPalette = @([Convert]::ToInt32($hex.Substring(0, 2), 16), [Convert]::ToInt32($hex.Substring(2, 2), 16),
    [Convert]::ToInt32($hex.Substring(4, 2), 16))

New-Item -ItemType Directory -Force $Sortie | Out-Null
# Attente bornée (6 min) : « adb get-state » répond tout de suite, contrairement à « adb wait-for-device ».
$limite = (Get-Date).AddMinutes(6)
while ("$(Adb 15 get-state)".Trim() -ne 'device') {
    if ((Get-Date) -gt $limite) { Echec 'aucun appareil adb après 6 min' }
    Start-Sleep -Seconds 2
}
while ("$(Adb 15 shell getprop sys.boot_completed)".Trim() -ne '1') {
    if ((Get-Date) -gt $limite) { Echec 'émulateur non démarré après 6 min' }
    Start-Sleep -Seconds 2
}
$null = Adb 30 shell input keyevent 82 # déverrouille l'écran
# Un émulateur tout juste démarré est saturé (charge > 20) : on attend qu'il se calme, puis on écarte une boîte
# « Application Not Responding » éventuelle du système (bouton « Wait », repéré par uiautomator).
$limite = (Get-Date).AddMinutes(6)
while ([double]("$(Adb 15 shell cat /proc/loadavg)".Trim().Split(' ')[0]) -gt 4 -and (Get-Date) -lt $limite) { Start-Sleep -Seconds 5 }
Ecarter-Anr
Adb 240 install -r (Resolve-Path -LiteralPath $Apk).Path | ForEach-Object { Write-Output "install : $_" }
if ($LASTEXITCODE -ne 0) { Echec 'adb install' }
$null = Adb 30 logcat -c
$null = Adb 60 shell monkey -p $paquet -c android.intent.category.LAUNCHER 1
Start-Sleep -Seconds 10
$processus = "$(Adb 30 shell pidof $paquet)".Trim()
if (-not $processus) { Echec 'processus absent 10 s après le lancement' }
Write-Output "processus : $processus"

# Journal : l'application ne doit avoir ni plantage fatal ni SecurityException (permissions retirées, L1-T17).
$journal = Join-Path $Sortie 'logcat.txt'
Adb 60 logcat -d -v threadtime | Set-Content -Path $journal
$filtre = 'FATAL EXCEPTION|SecurityException'
$fatals = @(Get-Content $journal | Where-Object { ($_ -match $filtre) -and ($_ -match " $processus " -or $_ -match [regex]::Escape($paquet)) })
$fatals += @(Get-Content $journal | Where-Object { $_ -match "Process: $([regex]::Escape($paquet))" })
if ($fatals.Count -gt 0) { Echec "plantage dans logcat : $($fatals[0])" }
Write-Output "logcat : aucun $filtre pour $paquet (pid $processus)"

$avant = Capture 'avant'
$taille = ("$(& $python -I scripts/lire-pixel.py $avant --taille)".Trim()).Split(' ')
$largeur = [int]$taille[0]; $hauteur = [int]$taille[1]
Write-Output "écran : $largeur × $hauteur, $("$(Adb 30 shell wm density)".Trim())"
# Les points de contrôle de la palette (décalages en pixels depuis le bord droit) sont calés sur l'appareil
# pixel_tablet (2560 × 1600, packaging/android/versions.env, aussi celui de la CI) : un autre écran est refusé.
if ($largeur -ne 2560 -or $hauteur -ne 1600) { Echec "écran $largeur × $hauteur : 2560 × 1600 attendu (profil pixel_tablet)" }
foreach ($f in 0.25, 0.5, 0.75) {
    $y = [int]($hauteur * $f)
    $lu = Pixel $avant ($largeur - 3) $y
    if (-not (Proche $lu $fondPalette 16)) { Echec "palette absente au bord droit, y = $y (lu $($lu -join ' '), attendu $($fondPalette -join ' '))" }
}
Write-Output 'palette : visible au bord droit'

# Rectangles entiers : un rendu défectueux (émulateur à GPU logiciel SwiftShader, voir docs/verifications) ne dessine
# que la moitié de chaque rectangle, en triangle. Ces pixels tombent dans la moitié qui manquerait (tablette 2560 × 1600).
$coins = @(
    @{ Nom = 'page, coin bas-droit'; X = [int]($largeur * 0.765); Y = [int]($hauteur * 0.87); Attendu = @(255, 255, 255); Tol = 12 },
    @{ Nom = 'fond de palette, bas-gauche'; X = $largeur - 192; Y = [int]($hauteur * 0.72); Attendu = $fondPalette; Tol = 16 },
    @{ Nom = 'pastille rouge, coin bas-droit'; X = $largeur - 178; Y = [int]($hauteur * 0.26); Attendu = @(229, 57, 53); Tol = 40 }
)
foreach ($coin in $coins) {
    $lu = Pixel $avant $coin.X $coin.Y
    Write-Output "coin $($coin.Nom) ($($coin.X), $($coin.Y)) : lu $($lu -join ' ')"
    if (-not (Proche $lu $coin.Attendu $coin.Tol)) {
        Echec "rectangle à moitié dessiné : $($coin.Nom) (lu $($lu -join ' '), attendu $($coin.Attendu -join ' ')) : rendu GPU de l'émulateur défectueux ?"
    }
}
Write-Output 'rectangles : entiers (coins opposés à la diagonale)'

# Trait d'un doigt au milieu de la page (blanche avant le trait).
$y = [int]($hauteur * 0.5)
$x0 = [int]($largeur * 0.30); $x1 = [int]($largeur * 0.55); $xm = [int](($x0 + $x1) / 2)
$blanc = Pixel $avant $xm $y
if (-not (Proche $blanc @(255, 255, 255) 12)) { Echec "page non blanche avant le trait (lu $($blanc -join ' '))" }
$null = Adb 30 shell input swipe $x0 $y $x1 $y 600
Start-Sleep -Seconds 2
$apres = Capture 'apres'
$trait = Pixel $apres $xm $y
Write-Output "trait : lu $($trait -join ' ')"
if (-not ($trait[0] -lt 100 -and $trait[1] -lt 100 -and $trait[2] -lt 100)) { Echec 'trait absent après le glissement' }
if ("$(Adb 30 shell pidof $paquet)".Trim() -ne $processus) { Echec 'le processus a redémarré ou s''est arrêté' }
} catch {
    Echec "erreur imprévue : $($_.Exception.Message) (ligne $($_.InvocationInfo.ScriptLineNumber))"
}
Write-Output 'ANDROID OK'
