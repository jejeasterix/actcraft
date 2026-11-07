# APK de banc pour une tablette Android (L11-T15, D11-12) : l'exécutable du banc (bench/), empaqueté à part, en
# release, non débogable (la mesure doit rester en release), sans aucune permission. Presets android-arm64 et
# android-x86_64 ; clé de débogage locale (build/android-debug.keystore, partagée avec paquet-android.ps1).
# Usage : pwsh -NoProfile -File ./scripts/paquet-banc-android.ps1 [-Abi arm64-v8a|x86_64|tous]
# Produit build/paquet-banc/ActCraft-banc-<version>-<abi>.apk : local, jamais dans build/paquet/, ni dans la release,
# ni dans la CI de release.
# Dernière ligne : « APK BANC OK » (0) ou « APK BANC EN ECHEC : <raison> » (1).
param([ValidateSet('arm64-v8a', 'x86_64', 'tous')][string]$Abi = 'tous')
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

try {
    . "$PSScriptRoot/version-projet.ps1"
    . "$PSScriptRoot/dev-env-android.ps1"
    $version = Get-ActCraftVersion
    $identifiant = 'io.github.jejeasterix.actcraft.banc'
    $versions = Join-Path $PSScriptRoot '../packaging/android/versions.env'
    $voulus = (Get-Content $versions | Where-Object { $_ -match '^ANDROID_BUILD_TOOLS=' }) -replace '^ANDROID_BUILD_TOOLS=', ''
    $outils = Get-Item -ErrorAction SilentlyContinue (Join-Path $env:ANDROID_SDK_ROOT "build-tools/$voulus")
    if (-not $voulus -or -not $outils) { throw "build-tools Android $voulus introuvables (packaging/android/versions.env)" }
    $bat = if ($IsWindows) { '.bat' } else { '' }
    $exe = if ($IsWindows) { '.exe' } else { '' }
    $apksigner = Join-Path $outils.FullName "apksigner$bat"
    $zipalign = Join-Path $outils.FullName "zipalign$exe"
    $aapt2 = Join-Path $outils.FullName "aapt2$exe"
    $sortie = Join-Path (Get-Location) 'build/paquet-banc'
    New-Item -ItemType Directory -Force $sortie | Out-Null

    # Clé de débogage locale (décision 30), la même que paquet-android.ps1 : jamais versionnée.
    $keystore = Join-Path (Get-Location) 'build/android-debug.keystore'
    if (-not (Test-Path $keystore)) {
        New-Item -ItemType Directory -Force (Split-Path $keystore) | Out-Null
        & (Join-Path $env:JAVA_HOME "bin/keytool$exe") -genkeypair -keystore $keystore -storepass android `
            -alias androiddebugkey -keypass android -keyalg RSA -keysize 2048 -validity 10000 `
            -dname 'CN=Android Debug,O=Android,C=US'
        if ($LASTEXITCODE -ne 0) { throw 'keytool en échec' }
    }

    $abis = if ($Abi -eq 'tous') { @('arm64-v8a', 'x86_64') } else { @($Abi) }
    foreach ($a in $abis) {
        $preset = if ($a -eq 'arm64-v8a') { 'android-arm64' } else { 'android-x86_64' }
        cmake --preset $preset
        if ($LASTEXITCODE -ne 0) { throw "configuration $preset en échec" }
        # A24 (amendement 1, M13) : même coupure du démon Gradle que paquet-android.ps1, rendue telle quelle après.
        $gradleOptsAvant = $env:GRADLE_OPTS
        $env:GRADLE_OPTS = (@($env:GRADLE_OPTS, '-Dorg.gradle.daemon=false') | Where-Object { $_ }) -join ' '
        try {
            # Cible propre au banc : la cible « apk » de paquet-android.ps1 reste celle de l'application.
            cmake --build --preset $preset --target actcraft-bench_make_apk
        } finally {
            $env:GRADLE_OPTS = $gradleOptsAvant
        }
        if ($LASTEXITCODE -ne 0) { throw "construction $preset en échec" }
        $brut = Get-ChildItem -Recurse -Filter '*.apk' "build/$preset" |
            Where-Object { $_.FullName -match 'outputs[\\/]apk' -and $_.FullName -match 'actcraft-bench' } |
            Sort-Object LastWriteTime | Select-Object -Last 1
        if (-not $brut) { throw "aucun APK de banc produit sous build/$preset" }

        $aligne = Join-Path $sortie "ActCraft-banc-$version-$a-aligne.apk"
        $final = Join-Path $sortie "ActCraft-banc-$version-$a.apk"
        $enControle = $final
        & $zipalign -p -f 4 $brut.FullName $aligne
        if ($LASTEXITCODE -ne 0) { throw 'zipalign en échec' }
        Remove-Item -Force -ErrorAction SilentlyContinue "$final.idsig"
        $env:ACTCRAFT_APKSIGNER_KS_PASS = 'android'
        $env:ACTCRAFT_APKSIGNER_KEY_PASS = 'android'
        try {
            & $apksigner sign --ks $keystore --ks-pass env:ACTCRAFT_APKSIGNER_KS_PASS --ks-key-alias androiddebugkey `
                --key-pass env:ACTCRAFT_APKSIGNER_KEY_PASS --v4-signing-enabled false --out $final $aligne
        } finally {
            Remove-Item -ErrorAction SilentlyContinue Env:ACTCRAFT_APKSIGNER_KS_PASS, Env:ACTCRAFT_APKSIGNER_KEY_PASS
        }
        if ($LASTEXITCODE -ne 0) { throw 'apksigner en échec' }
        Remove-Item -Force $aligne

        # Contrôles : identifiant du banc, aucune permission, non débogable (la mesure doit rester en release).
        $badging = @(& $aapt2 dump badging $final)
        if ($LASTEXITCODE -ne 0) { throw 'aapt2 dump badging en échec' }
        $tete = ($badging | Select-String "^package: name='([^']+)'").Matches
        if (-not $tete -or $tete[0].Groups[1].Value -cne $identifiant) { throw "identifiant inattendu : $($badging[0])" }
        Write-Output "$a : identifiant = $identifiant"
        $permissions = @($badging | Select-String "^uses-permission(?:-sdk-23)?: name='([^']+)'" |
            ForEach-Object { $_.Matches[0].Groups[1].Value })
        Write-Output "$a : permissions = $($permissions -join ', ')"
        if ($permissions.Count -gt 0) { throw "permissions non autorisées dans l'APK de banc : $($permissions -join ', ')" }
        if ($badging | Select-String -CaseSensitive "^application-debuggable") { throw 'APK de banc débogable : la mesure doit rester en release' }
        $certificats = @(& $apksigner verify --print-certs $final)
        if ($LASTEXITCODE -ne 0) { throw 'signature invalide' }
        Write-Output "APK : $final"
        $enControle = $null
    }
    Write-Output 'APK BANC OK'
} catch {
    if ($enControle) { Remove-Item -Force -ErrorAction SilentlyContinue $enControle, "$enControle.idsig" }
    Write-Output "APK BANC EN ECHEC : $($_.Exception.Message)"
    exit 1
}
