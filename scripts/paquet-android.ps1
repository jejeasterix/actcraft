# APK Android (décisions 27, 28, 30, 31 ; DA9) : presets android-arm64 et android-x86_64, alignement, signature
# (clé de débogage locale par défaut, clé de release seulement en paramètres), contrôles.
# Usage : pwsh -NoProfile -File ./scripts/paquet-android.ps1 [-Abi arm64-v8a|x86_64|tous]
#         [-Keystore <fichier> -KeyAlias <alias>]
# Mots de passe d'une clé de release : variables ACTCRAFT_KEYSTORE_PASS et ACTCRAFT_KEY_PASS (ou -KeystorePass,
# -KeyPass) ; transmis à apksigner par « env: », jamais sur sa ligne de commande. Lues au démarrage puis retirées de
# l'environnement du processus : ni CMake ni Gradle (qui télécharge et exécute sa distribution) ne les héritent.
# Produit build/paquet/ActCraft-<version>-<abi>.apk ; local, jamais publié.
# Dernière ligne : « APK OK » (0) ou « APK EN ECHEC : <raison> » (1).
param(
    [ValidateSet('arm64-v8a', 'x86_64', 'tous')][string]$Abi = 'tous',
    [string]$Keystore,
    [string]$KeystorePass = $env:ACTCRAFT_KEYSTORE_PASS,
    [string]$KeyAlias,
    [string]$KeyPass = $env:ACTCRAFT_KEY_PASS
)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

# Secrets de signature déjà lus par les paramètres : retirés de l'environnement avant toute construction (noms de ce
# script et noms des secrets de release.yml). Seul apksigner reçoit les mots de passe, le temps de la signature.
foreach ($nom in 'ACTCRAFT_KEYSTORE_PASS', 'ACTCRAFT_KEY_PASS', 'ACTCRAFT_KEYSTORE_PASSWORD', 'ACTCRAFT_KEY_PASSWORD',
    'ACTCRAFT_KEYSTORE_BASE64', 'ACTCRAFT_KEY_ALIAS') {
    Remove-Item -ErrorAction SilentlyContinue "Env:$nom"
}

try {
    . "$PSScriptRoot/version-projet.ps1"
    . "$PSScriptRoot/dev-env-android.ps1"
    $version = Get-ActCraftVersion
    $versionCode = Get-ActCraftVersionCode
    $identifiant = 'io.github.jejeasterix.actcraft'
    # API cible = plateforme de compilation (packaging/android/versions.env), figée dans app/CMakeLists.txt.
    $versions = Join-Path $PSScriptRoot '../packaging/android/versions.env'
    $apiCible = (Get-Content $versions | Where-Object { $_ -match '^ANDROID_PLATFORM=' }) -replace '^ANDROID_PLATFORM=', ''
    if (-not $apiCible) { throw "ANDROID_PLATFORM absent de $versions" }
    $sortie = Join-Path (Get-Location) 'build/paquet'
    New-Item -ItemType Directory -Force $sortie | Out-Null

    # build-tools de versions.env, pas les plus récents installés : l'exécuteur de la CI en a de plus récents, dont
    # apksigner écrit autrement ses certificats (« V3.0 Signer: certificate DN »).
    $voulus = (Get-Content $versions | Where-Object { $_ -match '^ANDROID_BUILD_TOOLS=' }) -replace '^ANDROID_BUILD_TOOLS=', ''
    $outils = Get-Item -ErrorAction SilentlyContinue (Join-Path $env:ANDROID_SDK_ROOT "build-tools/$voulus")
    if (-not $voulus -or -not $outils) { throw "build-tools Android $voulus introuvables (packaging/android/versions.env)" }
    $bat = if ($IsWindows) { '.bat' } else { '' }
    $exe = if ($IsWindows) { '.exe' } else { '' }
    $apksigner = Join-Path $outils.FullName "apksigner$bat"
    $zipalign = Join-Path $outils.FullName "zipalign$exe"
    $aapt2 = Join-Path $outils.FullName "aapt2$exe"

    if (-not $Keystore) {
        # Clé de débogage locale (décision 30) : jamais versionnée, jamais utilisée pour une release.
        $Keystore = Join-Path (Get-Location) 'build/android-debug.keystore'
        $KeystorePass = 'android'; $KeyAlias = 'androiddebugkey'; $KeyPass = 'android'
        if (-not (Test-Path $Keystore)) {
            & (Join-Path $env:JAVA_HOME "bin/keytool$exe") -genkeypair -keystore $Keystore -storepass android `
                -alias androiddebugkey -keypass android -keyalg RSA -keysize 2048 -validity 10000 `
                -dname 'CN=Android Debug,O=Android,C=US'
            if ($LASTEXITCODE -ne 0) { throw 'keytool en échec' }
        }
    }

    $abis = if ($Abi -eq 'tous') { @('arm64-v8a', 'x86_64') } else { @($Abi) }
    foreach ($a in $abis) {
        $preset = if ($a -eq 'arm64-v8a') { 'android-arm64' } else { 'android-x86_64' }
        cmake --preset $preset
        if ($LASTEXITCODE -ne 0) { throw "configuration $preset en échec" }
        cmake --build --preset $preset --target apk
        if ($LASTEXITCODE -ne 0) { throw "construction $preset en échec" }
        $brut = Get-ChildItem -Recurse -Filter '*.apk' "build/$preset" |
            Where-Object { $_.FullName -match 'outputs[\\/]apk' } | Sort-Object LastWriteTime | Select-Object -Last 1
        if (-not $brut) { throw "aucun APK produit sous build/$preset" }

        $aligne = Join-Path $sortie "ActCraft-$version-$a-aligne.apk"
        $final = Join-Path $sortie "ActCraft-$version-$a.apk"
        $enControle = $final # supprimé si un contrôle échoue : build/paquet/ ne garde que des APK conformes
        & $zipalign -p -f 4 $brut.FullName $aligne
        if ($LASTEXITCODE -ne 0) { throw 'zipalign en échec' }
        # Mots de passe par l'environnement (invisibles dans la liste des processus) ; pas de signature v4 : son
        # fichier .idsig sortirait des noms du contrat de build/paquet/.
        Remove-Item -Force -ErrorAction SilentlyContinue "$final.idsig"
        $env:ACTCRAFT_APKSIGNER_KS_PASS = $KeystorePass
        $env:ACTCRAFT_APKSIGNER_KEY_PASS = $KeyPass
        try {
            & $apksigner sign --ks $Keystore --ks-pass env:ACTCRAFT_APKSIGNER_KS_PASS --ks-key-alias $KeyAlias `
                --key-pass env:ACTCRAFT_APKSIGNER_KEY_PASS --v4-signing-enabled false --out $final $aligne
        } finally {
            Remove-Item -ErrorAction SilentlyContinue Env:ACTCRAFT_APKSIGNER_KS_PASS, Env:ACTCRAFT_APKSIGNER_KEY_PASS
        }
        if ($LASTEXITCODE -ne 0) { throw 'apksigner en échec' }
        Remove-Item -Force $aligne

        # Contrôles : identifiant, version, API cible, aucune permission (décision 28), signature, licences (décision
        # 34). Comparaisons sensibles à la casse : Android distingue les majuscules.
        $badging = @(& $aapt2 dump badging $final)
        if ($LASTEXITCODE -ne 0) { throw 'aapt2 dump badging en échec' }
        $tete = ($badging | Select-String "^package: name='([^']+)' versionCode='(\d+)' versionName='([^']+)'").Matches
        if (-not $tete) { throw "en-tête de paquet illisible : $($badging[0])" }
        Write-Output "$a : identifiant = $($tete[0].Groups[1].Value)"
        if ($tete[0].Groups[1].Value -cne $identifiant) { throw "identifiant inattendu : $($badging[0])" }
        Write-Output "$a : versionCode = $($tete[0].Groups[2].Value), versionName = $($tete[0].Groups[3].Value)"
        if ($tete[0].Groups[2].Value -cne "$versionCode" -or $tete[0].Groups[3].Value -cne $version) {
            throw "version inattendue : $($badging[0]) (attendu $versionCode / $version)"
        }
        $cible = ($badging | Select-String -CaseSensitive "^targetSdkVersion:'(\d+)'").Matches
        $cibleLue = if ($cible) { $cible[0].Groups[1].Value } else { '' }
        Write-Output "$a : targetSdkVersion = $cibleLue"
        if ($cibleLue -cne $apiCible) { throw "API cible inattendue : '$cibleLue' (attendu $apiCible, android-$apiCible)" }
        $permissions = @($badging | Select-String "^uses-permission(?:-sdk-23)?: name='([^']+)'" |
            ForEach-Object { $_.Matches[0].Groups[1].Value })
        Write-Output "$a : permissions = $($permissions -join ', ')"
        if ($permissions.Count -gt 0) { throw "permissions demandées : $($permissions -join ', ') (décision 28 : aucune)" }

        $certificats = @(& $apksigner verify --print-certs $final)
        if ($LASTEXITCODE -ne 0) { throw 'signature invalide' }
        $certificats | Write-Output
        $signataire = $certificats | Select-String '^(?:Signer #1 certificate DN|V[\d.]+ Signer(?: #1)?: certificate DN): (.+)$' |
            Select-Object -First 1
        if (-not $signataire) { throw 'signature : aucun certificat de signataire' }
        Write-Output "$a : signature = $($signataire.Matches[0].Groups[1].Value)"

        & "$PSScriptRoot/verifier-licences.ps1" -Apk $final
        if ($LASTEXITCODE -ne 0) { throw 'licences en échec' }
        Write-Output "APK : $final"
        $enControle = $null
    }
    Write-Output 'APK OK'
} catch {
    if ($enControle) { Remove-Item -Force -ErrorAction SilentlyContinue $enControle, "$enControle.idsig" }
    Write-Output "APK EN ECHEC : $($_.Exception.Message)"
    exit 1
}
