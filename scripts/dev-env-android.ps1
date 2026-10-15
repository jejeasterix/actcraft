# Prépare la chaîne Android dans la session courante (décision 27) : JDK, SDK, NDK, Qt hôte et Qt pour Android.
# Une variable déjà définie (CI) est gardée ; sinon, valeur du poste (~/.tools, C:/Qt). Échoue si un dossier manque.
# JDK : Qt 6.12 exige JDK 21 ou plus ; jdk-17 sert de repli (voir docs/verifications/2026-10-14-outillage-paquets.md).
# Usage : . ./scripts/dev-env-android.ps1
$ErrorActionPreference = 'Stop'
$outils = Join-Path $HOME '.tools'
$qtRacine = if ($IsWindows) { 'C:/Qt/6.12.0' } else { Join-Path $HOME 'Qt/6.12.0' }
$hote = if ($IsWindows) { 'msvc2022_64' } elseif ($IsMacOS) { 'macos' } else { 'gcc_64' }

function Set-ActCraftDefault([string]$Nom, [string]$Valeur) {
    if (-not [Environment]::GetEnvironmentVariable($Nom)) {
        [Environment]::SetEnvironmentVariable($Nom, $Valeur.Replace('\', '/'))
    }
}

# Un JAVA_HOME qui désigne un JRE (sans javac) n'est pas gardé : Gradle exige un JDK (« does not provide the
# required capabilities: [JAVA_COMPILER] »).
$javac = if ($IsWindows) { 'javac.exe' } else { 'javac' }
if ($env:JAVA_HOME -and -not (Test-Path (Join-Path $env:JAVA_HOME "bin/$javac"))) { $env:JAVA_HOME = $null }
$jdk = Join-Path $outils 'jdk-21'
if (-not (Test-Path $jdk)) { $jdk = Join-Path $outils 'jdk-17' }
Set-ActCraftDefault 'JAVA_HOME' $jdk
Set-ActCraftDefault 'ANDROID_SDK_ROOT' (Join-Path $outils 'android-sdk')
if (-not $env:ANDROID_NDK_ROOT) {
    $versions = Join-Path $PSScriptRoot '../packaging/android/versions.env'
    $voulu = (Get-Content $versions | Where-Object { $_ -match '^ANDROID_NDK_VERSION=' }) -replace '^ANDROID_NDK_VERSION=', ''
    Set-ActCraftDefault 'ANDROID_NDK_ROOT' (Join-Path $env:ANDROID_SDK_ROOT "ndk/$voulu")
}
Set-ActCraftDefault 'ACTCRAFT_QT_HOST_DIR' (Join-Path $qtRacine $hote)
Set-ActCraftDefault 'ACTCRAFT_QT_ANDROID_ARM64_DIR' (Join-Path $qtRacine 'android_arm64_v8a')
Set-ActCraftDefault 'ACTCRAFT_QT_ANDROID_X86_64_DIR' (Join-Path $qtRacine 'android_x86_64')

foreach ($nom in 'JAVA_HOME', 'ANDROID_SDK_ROOT', 'ANDROID_NDK_ROOT', 'ACTCRAFT_QT_HOST_DIR',
    'ACTCRAFT_QT_ANDROID_ARM64_DIR', 'ACTCRAFT_QT_ANDROID_X86_64_DIR') {
    $valeur = [Environment]::GetEnvironmentVariable($nom)
    if (-not $valeur -or -not (Test-Path $valeur)) {
        throw "$nom introuvable ($valeur) : voir docs/verifications/2026-10-14-outillage-paquets.md"
    }
}

$sep = [IO.Path]::PathSeparator
$dossiers = @((Join-Path $env:JAVA_HOME 'bin'), (Join-Path $env:ANDROID_SDK_ROOT 'platform-tools'),
    (Join-Path $env:ANDROID_SDK_ROOT 'emulator'), (Join-Path $env:ANDROID_SDK_ROOT 'cmdline-tools/latest/bin'))
foreach ($d in $dossiers) {
    if ((Test-Path $d) -and -not (($env:PATH -split [regex]::Escape($sep)) -contains $d)) { $env:PATH = "$d$sep$env:PATH" }
}
# Ninja installé par pip, comme dans dev-env.ps1.
if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
    # `python3` est un raccourci du Microsoft Store sous Windows : `python` d'abord.
    $python = if (Get-Command python -ErrorAction SilentlyContinue) { 'python' } else { 'python3' }
    # ninja ≥ 1.11.1.2 (pip) range ninja.exe dans le dossier Scripts et l'annonce par ninja.BIN_DIR ; avant, sous
    # ninja/data/bin. Sans Visual Studio (qui fournit le sien à dev-env.ps1), seul ce chemin le trouve.
    $ninjaDir = & $python -c "import ninja, os; print(getattr(ninja, 'BIN_DIR', '') or os.path.join(os.path.dirname(ninja.__file__), 'data', 'bin'))"
    $env:PATH = "$ninjaDir$sep$env:PATH"
    if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) { throw "ninja introuvable ($ninjaDir) : python -m pip install --user ninja" }
}
