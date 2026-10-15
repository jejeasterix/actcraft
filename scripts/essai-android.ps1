# Essai de l'APK sur un émulateur déjà démarré (scénario S3) : installation, lancement, processus vivant après 10 s,
# palette visible sur la capture, trait d'un doigt visible sur la capture suivante.
# Usage : pwsh -NoProfile -File ./scripts/essai-android.ps1 -Apk <fichier.apk> [-Sortie <dossier>]
# adb doit être dans le PATH (dev-env-android.ps1, ou l'émulateur de la CI). Captures sous -Sortie (.tmp/android).
# Dernière ligne et code : « ANDROID OK » (0) ou « ANDROID EN ECHEC : <raison> » (1).
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
    & adb shell screencap -p "/sdcard/actcraft-$Nom.png" | Out-Null
    & adb pull "/sdcard/actcraft-$Nom.png" $fichier | Out-Null
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $fichier)) { Echec "capture $Nom" }
    return $fichier
}

function Ecarter-Anr {
    for ($essai = 0; $essai -lt 3; $essai++) {
        if ("$(& adb shell dumpsys window | Select-String 'mCurrentFocus')" -notmatch 'Application Not Responding') { break }
        & adb shell uiautomator dump /sdcard/ui.xml | Out-Null
        $xml = "$(& adb shell cat /sdcard/ui.xml)"
        if ($xml -match 'text="Wait"[^>]*bounds="\[(\d+),(\d+)\]\[(\d+),(\d+)\]"') {
            & adb shell input tap ([int](([int]$Matches[1] + [int]$Matches[3]) / 2)) ([int](([int]$Matches[2] + [int]$Matches[4]) / 2)) | Out-Null
        }
        Start-Sleep -Seconds 3
    }
}

# Repère : couleur de la palette, lue dans le thème (décision 23).
$theme = Get-Content -Raw ui/qml/Theme.qml
if ($theme -notmatch 'property color palette:\s*"#([0-9A-Fa-f]{6})"') { Echec 'couleur « palette » introuvable dans ui/qml/Theme.qml' }
$hex = $Matches[1]
$fondPalette = @([Convert]::ToInt32($hex.Substring(0, 2), 16), [Convert]::ToInt32($hex.Substring(2, 2), 16),
    [Convert]::ToInt32($hex.Substring(4, 2), 16))

New-Item -ItemType Directory -Force $Sortie | Out-Null
& adb wait-for-device
$limite = (Get-Date).AddMinutes(6)
while ("$(& adb shell getprop sys.boot_completed)".Trim() -ne '1') {
    if ((Get-Date) -gt $limite) { Echec 'émulateur non démarré après 6 min' }
    Start-Sleep -Seconds 2
}
& adb shell input keyevent 82 | Out-Null # déverrouille l'écran
# Un émulateur tout juste démarré est saturé (charge > 20) : on attend qu'il se calme, puis on écarte une boîte
# « Application Not Responding » éventuelle du système (bouton « Wait », repéré par uiautomator).
$limite = (Get-Date).AddMinutes(6)
while ([double]("$(& adb shell cat /proc/loadavg)".Trim().Split(' ')[0]) -gt 4 -and (Get-Date) -lt $limite) { Start-Sleep -Seconds 5 }
Ecarter-Anr
& adb install -r (Resolve-Path $Apk).Path
if ($LASTEXITCODE -ne 0) { Echec 'adb install' }
& adb logcat -c | Out-Null
& adb shell monkey -p $paquet -c android.intent.category.LAUNCHER 1 | Out-Null
Start-Sleep -Seconds 10
$processus = "$(& adb shell pidof $paquet)".Trim()
if (-not $processus) { Echec 'processus absent 10 s après le lancement' }
Write-Output "processus : $processus"

# Journal : l'application ne doit avoir ni plantage fatal ni SecurityException (permissions retirées, L1-T17).
$journal = Join-Path $Sortie 'logcat.txt'
& adb logcat -d -v threadtime > $journal
$filtre = 'FATAL EXCEPTION|SecurityException'
$fatals = @(Get-Content $journal | Where-Object { ($_ -match $filtre) -and ($_ -match " $processus " -or $_ -match [regex]::Escape($paquet)) })
$fatals += @(Get-Content $journal | Where-Object { $_ -match "Process: $([regex]::Escape($paquet))" })
if ($fatals.Count -gt 0) { Echec "plantage dans logcat : $($fatals[0])" }
Write-Output "logcat : aucun $filtre pour $paquet (pid $processus)"

$avant = Capture 'avant'
$taille = ("$(& $python -I scripts/lire-pixel.py $avant --taille)".Trim()).Split(' ')
$largeur = [int]$taille[0]; $hauteur = [int]$taille[1]
Write-Output "écran : $largeur × $hauteur"
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
& adb shell input swipe $x0 $y $x1 $y 600 | Out-Null
Start-Sleep -Seconds 2
$apres = Capture 'apres'
$trait = Pixel $apres $xm $y
Write-Output "trait : lu $($trait -join ' ')"
if (-not ($trait[0] -lt 100 -and $trait[1] -lt 100 -and $trait[2] -lt 100)) { Echec 'trait absent après le glissement' }
if ("$(& adb shell pidof $paquet)".Trim() -ne $processus) { Echec 'le processus a redémarré ou s''est arrêté' }
Write-Output 'ANDROID OK'
