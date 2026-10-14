# Version du projet lue dans « project(ActCraft VERSION x.y.z) » du CMakeLists.txt racine (décision 31).
# Usage : . ./scripts/version-projet.ps1 ; Get-ActCraftVersion ; Get-ActCraftVersionCode
function Get-ActCraftVersion {
    $texte = Get-Content -Raw (Join-Path $PSScriptRoot '../CMakeLists.txt')
    if ($texte -notmatch 'project\(\s*ActCraft\s+VERSION\s+(\d+)\.(\d+)\.(\d+)') {
        throw 'version introuvable dans CMakeLists.txt (project(ActCraft VERSION x.y.z))'
    }
    return "$($Matches[1]).$($Matches[2]).$($Matches[3])"
}

# versionCode Android : majeur × 10000 + mineur × 100 + correctif (0.1.0 → 100).
function Get-ActCraftVersionCode {
    $parties = (Get-ActCraftVersion).Split('.') | ForEach-Object { [int]$_ }
    return $parties[0] * 10000 + $parties[1] * 100 + $parties[2]
}
