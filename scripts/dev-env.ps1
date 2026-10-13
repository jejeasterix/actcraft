# Prépare l'environnement de compilation dans la session PowerShell courante :
# MSVC (cl.exe) via Visual Studio, Ninja, et le dossier de Qt dans ACTCRAFT_QT_DIR.
# Usage : . ./scripts/dev-env.ps1   (avec le point : modifie la session appelante)
$ErrorActionPreference = 'Stop'

if (-not $env:ACTCRAFT_QT_DIR) {
    $env:ACTCRAFT_QT_DIR = 'C:\Qt\6.12.0\msvc2022_64'
}
if (-not (Test-Path (Join-Path $env:ACTCRAFT_QT_DIR 'lib\cmake\Qt6'))) {
    throw "Qt introuvable dans $env:ACTCRAFT_QT_DIR (voir CLAUDE.md, section Installation)."
}

# Ninja installé par pip (python -m pip install --user ninja).
if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
    $ninjaDir = python -c "import ninja, os; print(os.path.join(os.path.dirname(ninja.__file__), 'data', 'bin'))"
    $env:PATH = "$ninjaDir;$env:PATH"
}

if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    $installerDir = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer'
    $vswhere = Join-Path $installerDir 'vswhere.exe'
    # VsDevCmd appelle vswhere.exe sans chemin : sans ce dossier dans le PATH, il imprime une erreur.
    $env:PATH = "$installerDir;$env:PATH"
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsPath) { throw 'MSVC introuvable (Visual Studio Build Tools 2022 requis).' }
    Import-Module (Join-Path $vsPath 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
    Enter-VsDevShell -VsInstallPath $vsPath -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
}

# Les DLL de Qt doivent être trouvées par les tests et l'application.
$qtBin = Join-Path $env:ACTCRAFT_QT_DIR 'bin'
if (-not ($env:PATH -split ';' | Where-Object { $_ -eq $qtBin })) {
    $env:PATH = "$qtBin;$env:PATH"
}
