# Essai de l'APK sur un émulateur déjà démarré (scénario S3) : installation, lancement, processus vivant après 10 s,
# accueil ouvert au démarrage puis fermé d'un toucher, palette visible sur la capture, trait d'un doigt visible sur la
# capture suivante ; puis relances en mode essai HTML lourd : cadre de HTML de plus de 3 Mo sans base, affiché, gardes
# coupées (OB68) puis allumées (OB75).
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

# Le processus vivant ne dit pas que l'accueil est affiché : sur une machine chargée, le chargement dure plus que les
# 10 s d'Install-ActCraftApk (cause probable, non prouvée, du rouge du run 42764427092). Attente bornée à 120 s du premier
# point de contrôle ; le contrôle reste celui, inchangé, qui suit la capture définitive.
$voile = @($fondPalette | ForEach-Object { [int][math]::Floor($_ / 2) })
$limiteAccueil = (Get-Date).AddSeconds(120)
do {
    $attente = Capture 'accueil'
    $dimensions = ("$(& $python -I scripts/lire-pixel.py $attente --taille)".Trim()).Split(' ')
    if (Proche (Pixel $attente ([int]$dimensions[0] - 3) ([int]([int]$dimensions[1] * 0.25))) $voile 16) { break }
    Start-Sleep -Seconds 3
} while ((Get-Date) -lt $limiteAccueil)

# Journal : l'application ne doit avoir ni plantage fatal ni SecurityException (permissions retirées, L1-T17).
$journal = Join-Path $Sortie 'logcat.txt'
Adb 60 logcat -d -v threadtime | Set-Content -Path $journal
$filtre = 'FATAL EXCEPTION|SecurityException'
$fatals = @(Get-Content $journal | Where-Object { ($_ -match $filtre) -and ($_ -match " $processus " -or $_ -match [regex]::Escape($paquet)) })
$fatals += @(Get-Content $journal | Where-Object { $_ -match "Process: $([regex]::Escape($paquet))" })
if ($fatals.Count -gt 0) { Echec "plantage dans logcat : $($fatals[0])" }
Write-Output "logcat : aucun $filtre pour $paquet (pid $processus)"

$ecranAccueil = Capture 'accueil'
$taille = ("$(& $python -I scripts/lire-pixel.py $ecranAccueil --taille)".Trim()).Split(' ')
$largeur = [int]$taille[0]; $hauteur = [int]$taille[1]
Write-Output "écran : $largeur × $hauteur, $("$(Adb 30 shell wm density)".Trim())"
# Les points de contrôle de la palette (décalages en pixels depuis le bord droit) sont calés sur l'appareil
# pixel_tablet (2560 × 1600, packaging/android/versions.env, aussi celui de la CI) : un autre écran est refusé.
if ($largeur -ne 2560 -or $hauteur -ne 1600) { Echec "écran $largeur × $hauteur : 2560 × 1600 attendu (profil pixel_tablet)" }

# Accueil au démarrage (L6-T23, plan 6b : « fenêtre d'accueil devant la fenêtre principale au démarrage ») : son voile
# (#80000000) assombrit la palette de moitié. On le vérifie aux points de contrôle de la palette, puis on le ferme
# comme un utilisateur, d'un toucher sur le voile à gauche de la carte (880 points de large, centrée) ; les contrôles
# suivants restent ceux d'avant l'accueil. tests/tst_ui_shell.cpp (s3StartupDashboardClosedByTheScriptTap) lit
# `$accueil` et rejoue ce toucher hors écran.
foreach ($f in 0.25, 0.5, 0.75) {
    $y = [int]($hauteur * $f)
    $lu = Pixel $ecranAccueil ($largeur - 3) $y
    if (-not (Proche $lu $voile 16)) { Echec "accueil absent au démarrage, y = $y (lu $($lu -join ' '), attendu $($voile -join ' '))" }
}
Write-Output 'accueil : ouvert au démarrage'
$accueil = @{ X = [int]($largeur * 0.08); Y = [int]($hauteur * 0.5) }
# Une boîte « Application Not Responding » d'une autre application (le lanceur du système, sous la charge du démarrage
# de l'émulateur) peut prendre le toucher : vue au journal de l'essai rouge rejoué sur l'émulateur du Mac le
# 2026-11-11 (« Application Not Responding: com.google.android.apps.nexuslauncher »), comme aux runs 42829974229,
# 42835158658 et 42981911851. Elle est écartée juste avant le toucher. Si elle tient le focus juste après, le toucher
# n'a pas atteint ActCraft : elle est écartée, et le toucher n'est rejoué que si l'accueil est encore affiché, deux
# fois au plus. Une boîte ANR d'ActCraft, ou un accueil resté ouvert sans boîte du système, reste un échec.
$anrActCraft = "Application Not Responding: $([regex]::Escape($paquet))"
$touchers = 0
do {
    # Une boîte ANR d'ActCraft n'est jamais écartée ici : c'est un défaut de l'application.
    $focus = "$(Adb 30 shell dumpsys window | Select-String 'mCurrentFocus')".Trim()
    if ($focus -match $anrActCraft) { Echec "boîte ANR d'ActCraft avant le toucher de l'accueil ($focus)" }
    Ecarter-Anr
    $null = Adb 30 shell input tap $($accueil.X) $($accueil.Y)
    if ($LASTEXITCODE -ne 0) { Echec 'toucher de l''accueil refusé' }
    $touchers++
    $focus = "$(Adb 30 shell dumpsys window | Select-String 'mCurrentFocus')".Trim()
    if ($focus -match $anrActCraft) { Echec "boîte ANR d'ActCraft après le toucher de l'accueil ($focus)" }
    $pris = ($focus -match 'Application Not Responding') -and ($focus -notmatch [regex]::Escape($paquet))
    if ($pris) {
        Write-Output "accueil : toucher pris par une boîte du système ($focus)"
        $reprise = Capture 'accueil-reprise'
        $pris = Proche (Pixel $reprise ($largeur - 3) ([int]($hauteur * 0.25))) $voile 16
    }
} while ($pris -and $touchers -lt 3)
# Sous charge, l'accueil peut rester affiché plus de 2 s après le toucher (cause probable, non prouvée, du rouge du run
# 42829974229). Attente bornée à 60 s, nouvelle capture toutes les 2 s, sans nouveau toucher : il masquerait un défaut
# (seule la boîte ANR d'une autre application, ci-dessus, justifie de rejouer le toucher).
$limitePalette = (Get-Date).AddSeconds(60)
do {
    Start-Sleep -Seconds 2
    $avant = Capture 'avant'
    $defautPalette = $null
    foreach ($f in 0.25, 0.5, 0.75) {
        $y = [int]($hauteur * $f)
        $lu = Pixel $avant ($largeur - 3) $y
        if (-not (Proche $lu $fondPalette 16)) {
            $defautPalette = "palette absente au bord droit, y = $y (lu $($lu -join ' '), attendu $($fondPalette -join ' '))"
            break
        }
    }
} while ($defautPalette -and (Get-Date) -lt $limitePalette)
if ($defautPalette) { Echec $defautPalette }
Write-Output 'palette : visible au bord droit'

# Rectangles entiers : un rendu défectueux (émulateur à GPU logiciel SwiftShader, voir docs/verifications) ne dessine
# que la moitié de chaque rectangle, en triangle. Ces pixels tombent dans la moitié qui manquerait (tablette 2560 × 1600).
# Pastille : point étalonné sur l'émulateur au lot 5 (0,26 H = 416 px, barre d'état comprise), décalé du seul
# déplacement de la pastille dans la fenêtre depuis : +76 points (+152 px) = barre de titre de la palette (28, L6-T7)
# + deuxième rangée du groupe « commandes » (48, profile.choose et mode.design) + deuxième rangée du groupe
# « pages » (48, page.history-back et page.history-forward, L8-T2) : +124 points au total, 0,415 H = 664 px à la
# densité 320 (2 px par point). Même place dans la pastille quelle
# que soit la barre d'état ; tests/tst_ui_shell.cpp (s3StartupDashboardClosedByTheScriptTap) le rejoue hors écran.
# Coin de page : 0,755 L, 23 px en dedans du bord droit relevé le 2026-11-11 (page de x = 347 à 1956, y = 208 à 1415,
# zoom 79 %) ; 0,765 L, posé au ras du bord, tombait 2 px dehors, sur le bureau, après la mise en page du lot 11.
# Pastille, depuis le lot 11 : Android est toujours en interface tactile (LayoutMetrics::touchUi, L11-T10 : cibles de
# 48 dp, palette élargie) ; relevée sur l'émulateur le 2026-11-11 de x = 2322 à 2357, y = 702 à 737 ; point au centre.
$coins = @(
    @{ Nom = 'page, coin bas-droit'; X = [int]($largeur * 0.755); Y = [int]($hauteur * 0.87); Attendu = @(255, 255, 255); Tol = 12 },
    @{ Nom = 'fond de palette, bas-gauche'; X = $largeur - 192; Y = [int]($hauteur * 0.72); Attendu = $fondPalette; Tol = 16 },
    @{ Nom = 'pastille rouge, coin bas-droit'; X = $largeur - 220; Y = [int]($hauteur * 0.45); Attendu = @(229, 57, 53); Tol = 40 }
)
foreach ($coin in $coins) {
    $lu = Pixel $avant $coin.X $coin.Y
    Write-Output "coin $($coin.Nom) ($($coin.X), $($coin.Y)) : lu $($lu -join ' ')"
    if (-not (Proche $lu $coin.Attendu $coin.Tol)) {
        Echec "rectangle à moitié dessiné : $($coin.Nom) (lu $($lu -join ' '), attendu $($coin.Attendu -join ' ')) : rendu GPU de l'émulateur défectueux ?"
    }
}
Write-Output 'rectangles : entiers (coins opposés à la diagonale)'

# Trait d'un doigt au milieu de la page (blanche avant le trait). Témoin : un point de la page sur la même ligne, hors
# du trait (0,65 L, 256 px après sa fin, page de x = 347 à 1956), blanc avant et après le glissement. Sans lui, une
# capture toute noire passerait pour un trait.
$y = [int]($hauteur * 0.5)
$x0 = [int]($largeur * 0.30); $x1 = [int]($largeur * 0.55); $xm = [int](($x0 + $x1) / 2)
$xTemoin = [int]($largeur * 0.65)
$blanc = Pixel $avant $xm $y
if (-not (Proche $blanc @(255, 255, 255) 12)) { Echec "page non blanche avant le trait (lu $($blanc -join ' '))" }
$temoin = Pixel $avant $xTemoin $y
if (-not (Proche $temoin @(255, 255, 255) 12)) { Echec "témoin ($xTemoin, $y) non blanc avant le trait (lu $($temoin -join ' '))" }
$null = Adb 30 shell input swipe $x0 $y $x1 $y 600
if ($LASTEXITCODE -ne 0) { Echec 'glissement du doigt refusé' }
# Même attente bornée que pour la palette : sous charge, le trait peut s'afficher plus de 2 s après le glissement.
$limiteTrait = (Get-Date).AddSeconds(60)
do {
    Start-Sleep -Seconds 2
    $apres = Capture 'apres'
    $trait = Pixel $apres $xm $y
    $temoin = Pixel $apres $xTemoin $y
    $traitVu = $trait[0] -lt 100 -and $trait[1] -lt 100 -and $trait[2] -lt 100
} while (-not $traitVu -and (Get-Date) -lt $limiteTrait)
Write-Output "trait : lu $($trait -join ' ') ; témoin ($xTemoin, $y) : lu $($temoin -join ' ')"
if (-not $traitVu) { Echec 'trait absent après le glissement' }
if (-not (Proche $temoin @(255, 255, 255) 12)) { Echec "témoin ($xTemoin, $y) non blanc après le trait (lu $($temoin -join ' ')) : capture noire ?" }
if ("$(Adb 30 shell pidof $paquet)".Trim() -ne $processus) { Echec 'le processus a redémarré ou s''est arrêté' }

# HTML lourd sans base (OB68, OB75, docs/verifications/2026-11-13-youtube-erreur-153.md) : relance en mode essai
# HTML lourd, qui pose au démarrage un cadre de HTML saisi de plus de 3 Mo, sans lecteur, dans un document neuf (base
# vide). Deux passages : gardes coupées (« html-lourd », défaut), puis gardes allumées (« html-lourd-gardes » : le
# modèle réécrit le HTML sous sa règle de sécurité, finishContent ; la vue ne donne jamais de base). Sans base,
# QtWebView passe à loadData : adresse data: au-delà de la limite de 2 Mo de la WebView, page vide. Repère : la page
# de l'essai est toute verte (#00c853, app/startup.cpp heavyHtmlProbePage) ; trois points autour du centre de la page
# (centre du cadre, 640 × 400 points) doivent être verts, et la vue doit signaler sa page chargée (pageLoaded).
# Qt ignore applicationArguments dans un APK non débogable : l'option passe par la propriété debug.actcraft.essai,
# lue par app/main.cpp sous Android, puis remise à « aucun » sur tous les chemins (finally : exit l'exécute aussi),
# sinon chaque lancement suivant, jusqu'au redémarrage de l'appareil, poserait le cadre.
function Essai-HtmlLourd([string]$Mode, [bool]$Gardes) {
    $nom = if ($Gardes) { 'html lourd gardes' } else { 'html lourd' }
    $null = Adb 30 shell am force-stop $paquet
    $journalHtml = Join-Path $Sortie "logcat-$Mode.txt"
    try {
        $null = Adb 30 shell setprop debug.actcraft.essai $Mode
        if ($LASTEXITCODE -ne 0) { Echec "setprop debug.actcraft.essai $Mode refusé$(Get-ActCraftAdbErreur)" }
        $null = Adb 30 logcat -c
        $null = Adb 60 shell monkey -p $paquet -c android.intent.category.LAUNCHER 1
        if ($LASTEXITCODE -ne 0) { Echec "lancement de l'essai $nom refusé$(Get-ActCraftAdbErreur)" }
        $limitePose = (Get-Date).AddSeconds(120)
        do {
            Start-Sleep -Seconds 2
            $pose = @(Adb 60 logcat -d | Select-String 'ACTCRAFT ESSAI CADRE WEB (POSE|ECHEC)')
        } while ($pose.Count -eq 0 -and (Get-Date) -lt $limitePose)
        # « POSE GARDES » : le modèle avait ses gardes allumées au moment de la pose ; « POSE » seul : coupées.
        $attendu = if ($Gardes) { 'POSE GARDES' } else { 'POSE(?! GARDES)' }
        if ($pose.Count -eq 0 -or "$($pose[0])" -notmatch $attendu) {
            Adb 60 logcat -d -v threadtime | Set-Content -Path $journalHtml
            Get-Content $journalHtml | Select-String 'ACTCRAFT|chromium|cr_|WebView|qml' | Select-Object -Last 30 | ForEach-Object { Write-Output "logcat : $_" }
            Echec "cadre de l'essai $nom non posé ou gardes inattendues ($($pose -join ' | '))"
        }
    } finally {
        try { $null = Adb 30 shell setprop debug.actcraft.essai aucun } catch { Write-Output "setprop debug.actcraft.essai aucun : $($_.Exception.Message)" }
    }
    Write-Output "${nom} : cadre posé"
    # Accueil ouvert au démarrage : fermé du même toucher que plus haut (hors du cadre).
    Ecarter-Anr
    $null = Adb 30 shell input tap $($accueil.X) $($accueil.Y)
    $vert = @(0, 200, 83)
    $pointsHtml = @(@(1151, 811), @(951, 711), @(1351, 911))
    $limiteHtml = (Get-Date).AddSeconds(90)
    do {
        Start-Sleep -Seconds 3
        $ecranHtml = Capture $Mode
        $lus = @($pointsHtml | ForEach-Object { , (Pixel $ecranHtml $_[0] $_[1]) })
        $vu = @($lus | Where-Object { Proche $_ $vert 40 }).Count -eq $pointsHtml.Count
    } while (-not $vu -and (Get-Date) -lt $limiteHtml)
    Adb 60 logcat -d -v threadtime | Set-Content -Path $journalHtml
    $charge = [bool](Select-String -Path $journalHtml -Pattern 'ACTCRAFT CADRE WEB CHARGE' -Quiet)
    Write-Output "${nom} : lu $(($lus | ForEach-Object { $_ -join ' ' }) -join ' ; ') ; pageLoaded : $charge"
    if (-not $vu -or -not $charge) {
        Get-Content $journalHtml | Select-String 'ACTCRAFT|chromium|cr_|WebView|qml' | Select-Object -Last 30 | ForEach-Object { Write-Output "logcat : $_" }
        Echec "HTML lourd sans base, $nom, non affiché (attendu $($vert -join ' ') aux trois points et pageLoaded)"
    }
    $null = Adb 30 shell am force-stop $paquet
}
Essai-HtmlLourd 'html-lourd' $false
Essai-HtmlLourd 'html-lourd-gardes' $true
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
