# APK Android (décisions 27, 28, 30, 31 ; DA9) : presets android-arm64 et android-x86_64, alignement, signature
# (clé de débogage locale par défaut, clé de release seulement en paramètres), contrôles.
# Usage : pwsh -NoProfile -File ./scripts/paquet-android.ps1 [-Abi arm64-v8a|x86_64|tous]
#         [-Keystore <fichier> -KeystorePass <p> -KeyAlias <alias> -KeyPass <p>]
# Produit build/paquet/ActCraft-<version>-<abi>.apk ; local, jamais publié.
# Dernière ligne : « APK OK » (0) ou « APK EN ECHEC : <raison> » (1).
param(
    [ValidateSet('arm64-v8a', 'x86_64', 'tous')][string]$Abi = 'tous',
    [string]$Keystore,
    [string]$KeystorePass,
    [string]$KeyAlias,
    [string]$KeyPass
)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

try {
    . "$PSScriptRoot/version-projet.ps1"
    . "$PSScriptRoot/dev-env-android.ps1"
    $version = Get-ActCraftVersion
    $versionCode = Get-ActCraftVersionCode
    $identifiant = 'io.github.jejeasterix.actcraft'
    $sortie = Join-Path (Get-Location) 'build/paquet'
    New-Item -ItemType Directory -Force $sortie | Out-Null

    $outils = Get-ChildItem (Join-Path $env:ANDROID_SDK_ROOT 'build-tools') -Directory |
        Sort-Object { [version]($_.Name -replace '-.*$', '') } | Select-Object -Last 1
    if (-not $outils) { throw 'build-tools Android introuvables' }
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
        & $zipalign -p -f 4 $brut.FullName $aligne
        if ($LASTEXITCODE -ne 0) { throw 'zipalign en échec' }
        & $apksigner sign --ks $Keystore --ks-pass "pass:$KeystorePass" --ks-key-alias $KeyAlias `
            --key-pass "pass:$KeyPass" --out $final $aligne
        if ($LASTEXITCODE -ne 0) { throw 'apksigner en échec' }
        Remove-Item -Force $aligne

        # Contrôles : identifiant, version, aucune permission (décision 28), signature, licences (décision 34).
        $badging = @(& $aapt2 dump badging $final)
        if ($LASTEXITCODE -ne 0) { throw 'aapt2 dump badging en échec' }
        $tete = ($badging | Select-String "^package: name='([^']+)' versionCode='(\d+)' versionName='([^']+)'").Matches
        if (-not $tete) { throw "en-tête de paquet illisible : $($badging[0])" }
        Write-Output "$a : identifiant = $($tete[0].Groups[1].Value)"
        if ($tete[0].Groups[1].Value -ne $identifiant) { throw "identifiant inattendu : $($badging[0])" }
        Write-Output "$a : versionCode = $($tete[0].Groups[2].Value), versionName = $($tete[0].Groups[3].Value)"
        if ($tete[0].Groups[2].Value -ne "$versionCode" -or $tete[0].Groups[3].Value -ne $version) {
            throw "version inattendue : $($badging[0]) (attendu $versionCode / $version)"
        }
        $permissions = @($badging | Select-String "^uses-permission(?:-sdk-23)?: name='([^']+)'" |
            ForEach-Object { $_.Matches[0].Groups[1].Value })
        Write-Output "$a : permissions = $($permissions -join ', ')"
        if ($permissions.Count -gt 0) { throw "permissions demandées : $($permissions -join ', ') (décision 28 : aucune)" }

        $certificats = @(& $apksigner verify --print-certs $final)
        if ($LASTEXITCODE -ne 0) { throw 'signature invalide' }
        $certificats | Write-Output
        $signataire = $certificats | Select-String '^Signer #1 certificate DN: (.+)$' | Select-Object -First 1
        if (-not $signataire) { throw 'signature : aucun certificat de signataire' }
        Write-Output "$a : signature = $($signataire.Matches[0].Groups[1].Value)"

        & "$PSScriptRoot/verifier-licences.ps1" -Apk $final
        if ($LASTEXITCODE -ne 0) { throw 'licences en échec' }
        Write-Output "APK : $final"
    }
    Write-Output 'APK OK'
} catch {
    Write-Output "APK EN ECHEC : $($_.Exception.Message)"
    exit 1
}
