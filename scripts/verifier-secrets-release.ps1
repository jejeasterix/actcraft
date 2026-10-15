# Mode « reel » de release.yml (décision 32) : échoue, avant toute construction et sans rien créer, si un secret de
# signature manque. Jamais d'APK de débogage dans une release.
# Un secret absent arrive dans GitHub Actions comme une variable DÉFINIE ET VIDE (« env: X: ${{ secrets.X }} ») :
# seule la valeur compte, jamais la présence de la variable.
# Usage : pwsh -NoProfile -File ./scripts/verifier-secrets-release.ps1
# Dernière ligne et code : « SECRETS OK » (0) ou « SECRETS ABSENTS : <noms> … » (1).
$noms = 'ACTCRAFT_KEYSTORE_BASE64', 'ACTCRAFT_KEYSTORE_PASSWORD', 'ACTCRAFT_KEY_ALIAS', 'ACTCRAFT_KEY_PASSWORD'
$manquants = @($noms | Where-Object { [string]::IsNullOrWhiteSpace([Environment]::GetEnvironmentVariable($_)) })
if ($manquants.Count -gt 0) {
    Write-Output ("SECRETS ABSENTS : $($manquants -join ', ') — release impossible : un APK de release exige la clé " +
        'de release (créée au go de Jérôme, décision 30) ; aucune release créée.')
    exit 1
}
Write-Output 'SECRETS OK'
