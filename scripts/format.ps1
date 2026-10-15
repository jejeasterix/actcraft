# Formate le C++ avec clang-format (python -m pip install --user clang-format).
# Usage : pwsh -NoProfile -File ./scripts/format.ps1 [-Check]
#   -Check : ne modifie rien, échoue si un fichier n'est pas au format.
param([switch]$Check)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

$clangFormat = (Get-Command clang-format -ErrorAction SilentlyContinue).Source
if (-not $clangFormat) {
    # Le paquet pip range l'exécutable dans son dossier data/bin, hors du PATH.
    $clangFormat = python -c "import clang_format, os, glob; print(glob.glob(os.path.join(os.path.dirname(clang_format.__file__), 'data', 'bin', 'clang-format*'))[0])"
}
if (-not $clangFormat) { throw 'clang-format introuvable : python -m pip install --user clang-format' }

# Code tiers (third_party/) : tel que publié, jamais reformaté.
$files = git ls-files --cached --others --exclude-standard -- '*.cpp' '*.h' '*.hpp' ':!third_party/**'
if (-not $files) { exit 0 }

if ($Check) {
    & $clangFormat --dry-run --Werror @files
} else {
    & $clangFormat -i @files
}
exit $LASTEXITCODE
