# Essai de l'APK sur un émulateur déjà démarré (scénario S3) : installation, lancement, processus vivant après 10 s,
# palette visible sur la capture, trait d'un doigt visible sur la capture suivante.
# Usage : pwsh -NoProfile -File ./scripts/essai-android.ps1 -Apk <fichier.apk> [-Sortie <dossier>]
# adb doit être dans le PATH (dev-env-android.ps1, ou l'émulateur de la CI). Captures sous -Sortie (.tmp/android).
# Émulateur : -gpu host en local ; -gpu lavapipe sur un exécuteur sans GPU (CI) — SwiftShader rend faux (voir
# docs/verifications/2026-10-14-outillage-paquets.md, VERIF s3-rendu et s3-ci).
# Dernière ligne et code : « ANDROID OK » (0) ou « ANDROID EN ECHEC : <raison> » (1), même sur une erreur imprévue ;
# chaque commande adb a un délai (au-delà : échec, processus adb arrêté). Appel borné d'adb, attente du démarrage,
# installation et lancement : scripts/android-commun.ps1 (partagé avec essai-android-fichiers.ps1).
# Ce script ne démarre pas l'émulateur : un appareil est déjà là (en CI, android-emulator-runner le démarre).
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
    $null = Adb 60 shell screencap -p "/sdcard/actcraft-$Nom.png"
    $null = Adb 60 pull "/sdcard/actcraft-$Nom.png" $fichier
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $fichier)) { Echec "capture $Nom" }
    return $fichier
}

try {
. "$PSScriptRoot/android-commun.ps1"
Initialize-ActCraftAdb
if (-not (Test-Path -LiteralPath $Apk)) { Echec "APK introuvable : $Apk" }

# Repère : couleur de la palette, lue dans le thème (décision 23).
$theme = Get-Content -Raw ui/qml/Theme.qml
if ($theme -notmatch 'property color palette:\s*"#([0-9A-Fa-f]{6})"') { Echec 'couleur « palette » introuvable dans ui/qml/Theme.qml' }
$hex = $Matches[1]
$fondPalette = @([Convert]::ToInt32($hex.Substring(0, 2), 16), [Convert]::ToInt32($hex.Substring(2, 2), 16),
    [Convert]::ToInt32($hex.Substring(4, 2), 16))

New-Item -ItemType Directory -Force $Sortie | Out-Null
Wait-ActCraftDemarrage
Ecarter-Anr
$processus = Install-ActCraftApk -Apk $Apk -Paquet $paquet
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
    # Refus du module commun : sa raison telle quelle (mêmes messages qu'avant l'extraction) ; le reste est imprévu,
    # y compris un module introuvable.
    if (Get-Command Get-ActCraftAndroidRaison -ErrorAction SilentlyContinue) {
        $raison = Get-ActCraftAndroidRaison $_
        if ($raison) { Echec $raison }
    }
    Echec "erreur imprévue : $($_.Exception.Message) (ligne $($_.InvocationInfo.ScriptLineNumber))"
}
Write-Output 'ANDROID OK'
