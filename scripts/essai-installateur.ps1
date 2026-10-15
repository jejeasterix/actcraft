# Essai de l'installateur (scénario S1) : installation silencieuse par utilisateur, --version, lancement de 5 s avec
# contrôle du runtime MSVC chargé depuis le dossier d'installation, désinstallation silencieuse, puis contrôle qu'il
# ne reste ni dossier, ni raccourci du menu Démarrer, ni clé dans HKCU\Software\Classes.
# Le lancement est hors écran (QT_QPA_PLATFORM=offscreen) : aucune fenêtre n'est ouverte.
# Usage : pwsh -NoProfile -File ./scripts/essai-installateur.ps1 [-Setup <chemin>] [-Rendu d3d11|logiciel]
# Dernière ligne et code : « INSTALLATEUR OK » (0) ou « INSTALLATEUR EN ECHEC : <raison> » (1).
param([string]$Setup, [ValidateSet('d3d11', 'logiciel')][string]$Rendu = 'logiciel')
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
. "$PSScriptRoot/version-projet.ps1"
$version = Get-ActCraftVersion
if (-not $Setup) { $Setup = "build/paquet/ActCraft-$version-setup.exe" }

function Echec([string]$Raison) {
    Write-Output "INSTALLATEUR EN ECHEC : $Raison"
    exit 1
}

if (-not (Test-Path $Setup)) { Echec "$Setup introuvable" }
$dossier = Join-Path ([IO.Path]::GetTempPath()) ('actcraft-essai-' + [guid]::NewGuid().ToString('N').Substring(0, 8))
$journal = "$dossier.log"

$p = Start-Process -FilePath (Resolve-Path $Setup).Path -PassThru -WindowStyle Hidden -ArgumentList @(
    '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/CURRENTUSER', "/DIR=`"$dossier`"", "/LOG=`"$journal`"")
if (-not $p.WaitForExit(180000)) { Echec 'installation : pas de fin après 180 s' }
if ($p.ExitCode -ne 0) { Echec "installation : code $($p.ExitCode) (journal $journal)" }
$exe = Join-Path $dossier 'actcraft.exe'
if (-not (Test-Path $exe)) { Echec 'actcraft.exe absent après installation' }
Write-Output "Installé dans $dossier"

# Les clés d'association et le raccourci existent après l'installation (sinon le contrôle final ne prouve rien).
$menu = Join-Path ([Environment]::GetFolderPath('Programs')) 'ActCraft.lnk'
$cles = 'HKCU:\Software\Classes\.actcraft', 'HKCU:\Software\Classes\ActCraft.Document'
if (-not (Test-Path $menu)) { Echec "raccourci du menu Démarrer absent après installation ($menu)" }
foreach ($c in $cles) { if (-not (Test-Path $c)) { Echec "clé $c absente après installation" } }

# Aucune fenêtre : plateforme hors écran. Le paquet ne livre que qwindows ; le greffon qoffscreen de Qt est copié
# dans le dossier d'essai le temps du lancement, puis retiré avant la désinstallation (il ne fait pas partie du paquet).
$qt = if ($env:ACTCRAFT_QT_DIR) { $env:ACTCRAFT_QT_DIR } else { 'C:/Qt/6.12.0/msvc2022_64' }
$hors = Join-Path $qt 'plugins/platforms/qoffscreen.dll'
if (-not (Test-Path $hors)) { Echec "$hors introuvable (lancement hors écran impossible)" }
Copy-Item $hors (Join-Path $dossier 'platforms')
$env:QT_QPA_PLATFORM = 'offscreen'

# --version : application graphique, sortie standard redirigée vers un fichier.
$sortie = "$dossier-version.txt"
$v = Start-Process -FilePath $exe -ArgumentList '--version' -RedirectStandardOutput $sortie -Wait -PassThru
$texte = (Get-Content -Raw $sortie).Trim()
Write-Output "--version : $texte"
if ($v.ExitCode -ne 0 -or $texte -ne "ActCraft $version") { Echec "--version a donné « $texte » (attendu ActCraft $version)" }

# Lancement de 5 s : le processus lancé ici est suivi par son PID ; runtime MSVC chargé depuis $dossier.
$run = Start-Process -FilePath $exe -ArgumentList '--rendu', $Rendu, '--quitter-apres', '5000' -PassThru
$null = $run.Handle # garde le descripteur : ExitCode lisible après la fin
Start-Sleep -Seconds 2
$modules = @((Get-Process -Id $run.Id).Modules | ForEach-Object { $_.FileName })
foreach ($dll in 'vcruntime140.dll', 'vcruntime140_1.dll', 'msvcp140.dll') {
    $chemin = $modules | Where-Object { (Split-Path $_ -Leaf) -ieq $dll } | Select-Object -First 1
    Write-Output "$dll : $chemin"
    if (-not $chemin -or -not $chemin.StartsWith($dossier, [StringComparison]::OrdinalIgnoreCase)) {
        Stop-Process -Id $run.Id -Force
        Echec "$dll non chargé depuis $dossier"
    }
}
if (-not $run.WaitForExit(30000)) { Stop-Process -Id $run.Id -Force; Echec 'pas de fermeture après --quitter-apres 5000' }
if ($run.ExitCode -ne 0) { Echec "code de sortie $($run.ExitCode)" }

Remove-Item (Join-Path $dossier 'platforms/qoffscreen.dll') -Force

# Désinstallation silencieuse : le dossier est vidé (le désinstallateur finit après son retour).
$u = Start-Process -FilePath (Join-Path $dossier 'unins000.exe') -ArgumentList '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART' -PassThru -WindowStyle Hidden
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
Write-Output 'Après désinstallation : ni dossier, ni raccourci du menu Démarrer, ni clé d''association.'
Write-Output 'INSTALLATEUR OK'
