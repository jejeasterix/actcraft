# Preuve L4-T3 : à rejouer sous le jeton. Finit par PREUVE OK ou PREUVE EN ECHEC.
$ErrorActionPreference = 'Continue'
$wt = 'C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L4-T3'
Set-Location $wt
$echecs = @()
$libre = (Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1MB
if ($libre -lt 1.5) { 'PREUVE EN ECHEC : RAM libre insuffisante ({0:N2} Go)' -f $libre; exit 1 }
$actifs = Get-CimInstance Win32_Process -Filter "Name='ninja.exe' or Name='cl.exe' or Name='ctest.exe'" |
    Where-Object { $_.CommandLine -match 'livraison-complete--L4-' -and $_.CommandLine -notmatch 'L4-T3' }
if ($actifs) { 'PREUVE EN ECHEC : ninja/cl/ctest d''un autre worktree L4 actif (attendre, ne rien tuer)'; exit 1 }
. ./scripts/dev-env.ps1

# 1. Suite complète
$sortie = pwsh -NoProfile -File ./scripts/test.ps1 2>&1 | Out-String
$sortie -split "`n" | Where-Object { $_ -match 'tests passed|TESTS OK' } | ForEach-Object { $_.Trim() }
if ($sortie -notmatch '100% tests passed, 0 tests failed out of 136') { $echecs += 'suite : 136/136 non atteint' }
if ($sortie -notmatch 'TESTS OK') { $echecs += 'suite : TESTS OK absent' }

# 2. tst_core_shapes en texte : 9 fonctions PASS, 11 passed
$exe = Join-Path $wt 'build\debug\tst_core_shapes.exe'
$txt = Join-Path $wt '.tmp\shapes.txt'
& $exe -o "$txt,txt" | Out-Null
$t = Get-Content $txt -Raw
$fonctions = 'catalogOrderAndDefault','dragFromCornerAndCenter','squarePresetsStaySquare','linePresetsConstrained',
    'arrowHeadSmartPointBounded','arrowHeadStartsAtFourTenths','smartPointsOnlyForThreePresets','gestureOfEachPreset',
    'setParametersIsOneStepAndRebuilds'
foreach ($f in $fonctions) { if ($t -notmatch "PASS\s*:\s*\S+::$f\(") { $echecs += "fonction non PASS : $f" } }
if ($t -notmatch 'Totals: 11 passed') { $echecs += 'Totals: 11 passed absent' }
($t -split "`n" | Select-String 'Totals').Line

# 3. Mutation : début de tête 0,6 -> 0,5 ; doit rougir arrowHeadStartsAtFourTenths
$src = 'core/src/shapegeometry.cpp'
(Get-Content $src -Raw).Replace('{0.6, 0.25}', '{0.5, 0.25}') | Set-Content $src -NoNewline
try {
    cmake --build --preset debug --target tst_core_shapes *> .tmp/mutation-build.txt
    & $exe -o "$txt,txt" | Out-Null
    $m = Get-Content $txt -Raw
    if ($m -notmatch "FAIL!\s*:\s*\S+::arrowHeadStartsAtFourTenths\(") { $echecs += 'mutation : arrowHeadStartsAtFourTenths pas rouge' }
    else { 'mutation : arrowHeadStartsAtFourTenths rouge (attendu)' }
} finally {
    git checkout -- $src
}
cmake --build --preset debug --target tst_core_shapes *> .tmp/remise-build.txt
& $exe -o "$txt,txt" | Out-Null
if ((Get-Content $txt -Raw) -notmatch 'Totals: 11 passed, 0 failed') { $echecs += 'remise : tst_core_shapes pas revenu au vert' } else { 'remise : vert' }
if (git status --porcelain -- $src) { $echecs += 'mutation non annulée' }

# 4. Format et noms
pwsh -NoProfile -File ./scripts/format.ps1 -Check *> .tmp/format.txt
if ($LASTEXITCODE -ne 0) { $echecs += 'format.ps1 -Check en échec' } else { 'format : OK' }
$noms = python -I 'C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py' $wt | Select-Object -Last 1
$noms
if ($noms -notmatch 'NOMS OK') { $echecs += 'NOMS OK absent' }

# 5. Verdict
if ($echecs.Count -eq 0) { 'PREUVE OK' } else { $echecs | ForEach-Object { "  - $_" }; 'PREUVE EN ECHEC'; exit 1 }
