# Conversion d'une présentation PowerPoint en PDF, appelée par ActCraft (export/src/officeconvert.cpp).
# Les chemins arrivent par l'environnement (ACTCRAFT_CONVERT_IN, ACTCRAFT_CONVERT_OUT) : aucun n'est
# interpolé dans une ligne de commande ni dans ce script.
# Sortie : 0 si le PDF est écrit, 1 sinon (message sur la sortie d'erreur).
$ErrorActionPreference = 'Stop'
$inputPath = $env:ACTCRAFT_CONVERT_IN
$outputPath = $env:ACTCRAFT_CONVERT_OUT
if ([string]::IsNullOrEmpty($inputPath) -or [string]::IsNullOrEmpty($outputPath)) {
    [Console]::Error.WriteLine('ACTCRAFT_CONVERT_IN et ACTCRAFT_CONVERT_OUT doivent être définies.')
    exit 1
}

$code = 1
$app = $null
$presentation = $null
$startedByUs = $false
try {
    # Instance déjà ouverte de l'utilisateur, sinon la nôtre (et on le note pour la fermer).
    try {
        $app = [System.Runtime.InteropServices.Marshal]::GetActiveObject('PowerPoint.Application')
    } catch {
        $app = $null
    }
    if ($null -eq $app) {
        $app = New-Object -ComObject PowerPoint.Application
        $startedByUs = $true
    }
    # Open(FileName, ReadOnly, Untitled, WithWindow) : lecture seule, sans titre, sans fenêtre.
    $presentation = $app.Presentations.Open($inputPath, $true, $false, $false)
    $ppSaveAsPDF = 32
    $presentation.SaveAs($outputPath, $ppSaveAsPDF)
    $code = 0
} catch {
    [Console]::Error.WriteLine("Conversion impossible : $($_.Exception.Message)")
    $code = 1
} finally {
    # Ferme seulement notre présentation ; n'arrête PowerPoint que s'il a été lancé ici.
    if ($null -ne $presentation) {
        try { $presentation.Close() } catch { }
        [void][System.Runtime.InteropServices.Marshal]::ReleaseComObject($presentation)
    }
    if ($null -ne $app) {
        if ($startedByUs) {
            try { $app.Quit() } catch { }
        }
        [void][System.Runtime.InteropServices.Marshal]::ReleaseComObject($app)
    }
}
exit $code
