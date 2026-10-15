# Essai de l'installateur (scénario S1) : installation silencieuse par utilisateur, --version, lancement de 5 s dans une
# vraie fenêtre (plateforme windows, rendu D3D11 par défaut) avec contrôle des modules chargés depuis le dossier
# d'installation (runtime MSVC, greffon qwindows.dll) et du moteur de rendu obtenu, désinstallation silencieuse, puis
# contrôle qu'il ne reste ni dossier, ni raccourci du menu Démarrer, ni clé dans HKCU\Software\Classes.
# La fenêtre de 5 s s'ouvre sur l'écran principal (en local : prendre le jeton « ecran »).
# -HorsEcran : plateforme offscreen (greffon qoffscreen de Qt copié le temps du lancement), pour une machine sans
# session graphique seulement ; le journal l'écrit (« HORS ECRAN ») et qwindows.dll n'est alors pas contrôlé.
# Usage : pwsh -NoProfile -File ./scripts/essai-installateur.ps1 [-Setup <chemin>] [-Rendu d3d11|logiciel] [-HorsEcran]
# Dernière ligne et code : « INSTALLATEUR OK » (0) ou « INSTALLATEUR EN ECHEC : <raison> » (1). Tout échec, même
# imprévu (exception), arrête les processus lancés, désinstalle et nettoie avant cette ligne.
param([string]$Setup, [ValidateSet('d3d11', 'logiciel')][string]$Rendu = 'd3d11', [switch]$HorsEcran)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

$dossier = $null
$exe = $null
$lances = [System.Collections.Generic.List[System.Diagnostics.Process]]::new()
$menu = Join-Path ([Environment]::GetFolderPath('Programs')) 'ActCraft.lnk'
$cles = 'HKCU:\Software\Classes\.actcraft', 'HKCU:\Software\Classes\ActCraft.Document'
$cleDesinstallation = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{5F0C2E7A-3B4D-4C8E-9A61-ACC0C4AF7A10}_is1'

# Un essai en échec ne laisse rien : processus lancés arrêtés par leur PID, désinstallation silencieuse si
# l'installation a eu lieu, puis suppression des restes.
function Nettoyer {
    foreach ($p in $lances) {
        if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue; $null = $p.WaitForExit(10000) }
    }
    if (-not $dossier) { return }
    $unins = Join-Path $dossier 'unins000.exe'
    if (Test-Path $unins) {
        $u = Start-Process -FilePath $unins -ArgumentList '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART' -PassThru -WindowStyle Hidden
        if (-not $u.WaitForExit(120000)) { Stop-Process -Id $u.Id -Force }
        $limite = (Get-Date).AddSeconds(20)
        while ((Test-Path $dossier) -and (Get-Date) -lt $limite) { Start-Sleep -Milliseconds 500 }
    }
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $dossier, "$dossier.log", "$dossier-version.txt", "$dossier-qt.txt", $menu
    foreach ($c in $cles + $cleDesinstallation) { Remove-Item -LiteralPath $c -Recurse -Force -ErrorAction SilentlyContinue }
}

function Echec([string]$Raison) {
    Nettoyer
    Write-Output "INSTALLATEUR EN ECHEC : $Raison"
    exit 1
}

# Lance l'application installée ; le processus est suivi par son PID (Nettoyer l'arrête s'il vit encore).
function Lancer([string[]]$Arguments, [string]$Sortie, [string]$Erreurs) {
    $parametres = @{ FilePath = $exe; ArgumentList = $Arguments; PassThru = $true }
    if ($Sortie) { $parametres.RedirectStandardOutput = $Sortie }
    if ($Erreurs) { $parametres.RedirectStandardError = $Erreurs }
    $p = Start-Process @parametres
    $null = $p.Handle # garde le descripteur : ExitCode lisible après la fin
    $lances.Add($p)
    return $p
}

try {
    . "$PSScriptRoot/version-projet.ps1"
    $version = Get-ActCraftVersion
    if (-not $Setup) { $Setup = "build/paquet/ActCraft-$version-setup.exe" }
    if (-not (Test-Path $Setup)) { Echec "$Setup introuvable" }
    $dossier = Join-Path ([IO.Path]::GetTempPath()) ('actcraft-essai-' + [guid]::NewGuid().ToString('N').Substring(0, 8))
    $journal = "$dossier.log"

    $p = Start-Process -FilePath (Resolve-Path $Setup).Path -PassThru -WindowStyle Hidden -ArgumentList @(
        '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/CURRENTUSER', "/DIR=`"$dossier`"", "/LOG=`"$journal`"")
    $lances.Add($p)
    if (-not $p.WaitForExit(180000)) { Echec 'installation : pas de fin après 180 s' }
    if ($p.ExitCode -ne 0) { Echec "installation : code $($p.ExitCode) (journal $journal)" }
    $exe = Join-Path $dossier 'actcraft.exe'
    if (-not (Test-Path $exe)) { Echec 'actcraft.exe absent après installation' }
    Write-Output "Installé dans $dossier"

    # Les clés d'association et le raccourci existent après l'installation (sinon le contrôle final ne prouve rien).
    if (-not (Test-Path $menu)) { Echec "raccourci du menu Démarrer absent après installation ($menu)" }
    foreach ($c in $cles) { if (-not (Test-Path $c)) { Echec "clé $c absente après installation" } }

    # Plateforme : la vraie (windows), sauf -HorsEcran. Une variable QT_QPA_PLATFORM héritée ne doit pas fausser l'essai.
    Remove-Item -ErrorAction SilentlyContinue Env:QT_QPA_PLATFORM
    $plateforme = 'qwindows.dll'
    if ($HorsEcran) {
        # Le paquet ne livre que qwindows ; le greffon qoffscreen de Qt est copié le temps du lancement, puis retiré.
        $qt = if ($env:ACTCRAFT_QT_DIR) { $env:ACTCRAFT_QT_DIR } else { 'C:/Qt/6.12.0/msvc2022_64' }
        $hors = Join-Path $qt 'plugins/platforms/qoffscreen.dll'
        if (-not (Test-Path $hors)) { Echec "$hors introuvable (lancement hors écran impossible)" }
        Copy-Item $hors (Join-Path $dossier 'platforms')
        $env:QT_QPA_PLATFORM = 'offscreen'
        $plateforme = 'qoffscreen.dll'
        Write-Output 'HORS ECRAN : plateforme offscreen, aucune fenêtre ; qwindows.dll non contrôlé'
    }

    # --version : application graphique, sortie standard redirigée vers un fichier ; attente bornée.
    $sortie = "$dossier-version.txt"
    $v = Lancer @('--version') $sortie
    if (-not $v.WaitForExit(30000)) { Echec '--version : pas de fin après 30 s' }
    $texte = if (Test-Path $sortie) { "$(Get-Content -Raw $sortie)".Trim() } else { '' }
    Write-Output "--version : $texte"
    if ($v.ExitCode -ne 0 -or $texte -ne "ActCraft $version") { Echec "--version a donné « $texte » (attendu ActCraft $version)" }

    # Lancement de 5 s : fenêtre, modules chargés depuis $dossier (runtime MSVC, greffon de plateforme) et moteur de
    # rendu obtenu (QSG_INFO, journal de Qt sur la sortie d'erreur).
    $env:QSG_INFO = '1'
    $env:QT_FORCE_STDERR_LOGGING = '1'
    $traces = "$dossier-qt.txt"
    $run = Lancer @('--rendu', $Rendu, '--quitter-apres', '5000') $null $traces
    $attendus = @('vcruntime140.dll', 'vcruntime140_1.dll', 'msvcp140.dll', $plateforme)
    $modules = @()
    $fenetre = $false
    # Le délai de 5 s court depuis le chargement de la fenêtre ; le premier lancement après l'installation peut être lent
    # (analyse antivirus des fichiers neufs) : 30 s au plus pour voir la fenêtre et les modules.
    $limite = (Get-Date).AddSeconds(30)
    do {
        Start-Sleep -Milliseconds 250
        if ($run.HasExited) { Echec "l'application s'est arrêtée sans fenêtre ni modules attendus (code $($run.ExitCode))" }
        $run.Refresh()
        try { $modules = @($run.Modules | ForEach-Object { $_.FileName }) } catch { $modules = @() }
        $fenetre = $HorsEcran -or ($run.MainWindowHandle -ne [IntPtr]::Zero)
        $presents = @($attendus | Where-Object { $nom = $_; @($modules | Where-Object { (Split-Path $_ -Leaf) -ieq $nom }).Count -gt 0 })
    } until (($fenetre -and $presents.Count -eq $attendus.Count) -or (Get-Date) -gt $limite)
    foreach ($dll in $attendus) {
        $chemin = $modules | Where-Object { (Split-Path $_ -Leaf) -ieq $dll } | Select-Object -First 1
        Write-Output "$dll : $chemin"
        if (-not $chemin -or -not $chemin.StartsWith($dossier, [StringComparison]::OrdinalIgnoreCase)) {
            Echec "$dll non chargé depuis $dossier"
        }
    }
    if (-not $fenetre) { Echec 'aucune fenêtre ouverte en 30 s' }
    if (-not $HorsEcran) { Write-Output "fenêtre : « $($run.MainWindowTitle) »" }
    if (-not $run.WaitForExit(30000)) { Echec 'pas de fermeture après --quitter-apres 5000' }
    if ($run.ExitCode -ne 0) { Echec "code de sortie $($run.ExitCode)" }
    $qsg = @(Get-Content -ErrorAction SilentlyContinue $traces)
    $qsg | Where-Object { $_ -match 'backend|Adapter|Using' } | Select-Object -First 4 | ForEach-Object { Write-Output "qt : $_" }
    if ($Rendu -eq 'd3d11' -and -not ($qsg -match 'backend D3D11')) { Echec "rendu D3D11 non obtenu (journal de Qt : $traces)" }
    if ($HorsEcran) { Remove-Item (Join-Path $dossier 'platforms/qoffscreen.dll') -Force }

    # Désinstallation silencieuse : le dossier est vidé (le désinstallateur finit après son retour).
    $u = Start-Process -FilePath (Join-Path $dossier 'unins000.exe') -ArgumentList '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART' -PassThru -WindowStyle Hidden
    $lances.Add($u)
    if (-not $u.WaitForExit(180000)) { Echec 'désinstallation : pas de fin après 180 s' }
    if ($u.ExitCode -ne 0) { Echec "désinstallation : code $($u.ExitCode)" }
    $limite = (Get-Date).AddSeconds(30)
    while (((Test-Path $dossier) -or (Test-Path $menu)) -and (Get-Date) -lt $limite) {
        Start-Sleep -Milliseconds 500
    }
    $restes = if (Test-Path $dossier) { @(Get-ChildItem -Recurse -File $dossier) } else { @() }
    if ($restes.Count -gt 0) { Echec "$($restes.Count) fichier(s) restant(s) après désinstallation" }
    if (Test-Path $dossier) { Echec "dossier $dossier restant après désinstallation" }
    if (Test-Path $menu) { Echec "raccourci $menu restant après désinstallation" }
    foreach ($c in $cles) { if (Test-Path $c) { Echec "clé $c restante après désinstallation" } }
    Remove-Item -Force -ErrorAction SilentlyContinue "$dossier.log", "$dossier-version.txt", $traces
    Write-Output 'Après désinstallation : ni dossier, ni raccourci du menu Démarrer, ni clé d''association.'
} catch {
    Echec "erreur imprévue : $($_.Exception.Message) (ligne $($_.InvocationInfo.ScriptLineNumber))"
}
Write-Output 'INSTALLATEUR OK'
