# Outillage des paquets — vérifications du 2026-10-14

Tâche L1-T2 du chantier `livraison-complete` (décisions 26, 27, 28 ; N2-8, N2-12 du cadrage
`docs/specs/2026-10-14-livraison-complete-cadrage.md`). Une ligne `VERIF` par point ; les commandes exactes sont
celles du plan `docs/plans/2026-10-14-livraison-complete-lot1-PLAN.md`, tâche L1-T2. Les tâches L1-T17 (APK) et
L1-T18 (émulateur) ajoutent leurs lignes plus bas.

## Résultats

VERIF jdk17 = OK (Temurin 17.0.20.1, ~/.tools/jdk-17 ; JDK 21.0.12.1 aussi installé, ~/.tools/jdk-21, exigé par Qt 6.12)
VERIF sdk = OK (cmdline-tools 12.0, sdkmanager 11076708, ~/.tools/android-sdk ; platform-tools 37.0.1)
VERIF build-tools = 36.0.0
VERIF plateforme = android-36
VERIF ndk = 27.2.12479018 (source : doc Qt 6.12, android-configure-dev-environment)
VERIF api-min = 28 (source : C:/Qt/6.12.0/android_arm64_v8a/lib/cmake/Qt6Core/Qt6AndroidGradleHelpers.cmake:241)
VERIF qt-android-arm64 = OK (C:/Qt/6.12.0/android_arm64_v8a, qt.toolchain.cmake présent, aqtinstall de développement, hôte all_os)
VERIF qt-android-x86_64 = OK (C:/Qt/6.12.0/android_x86_64, qt.toolchain.cmake présent)
VERIF qt-android-modules = Svg, QuickControls2, QuickTemplates2 présents (arm64-v8a)
VERIF inno = OK (6.7.3, C:/Users/JeromeDenis/AppData/Local/Programs/Inno Setup 6/ISCC.exe, winget --scope user)
VERIF whpx = utilisable (emulator -accel-check : WHPX(10.0.26300) is installed and usable)
VERIF emulateur-local = démarre (boot en 235 s, sans fenêtre, swiftshader_indirect)
VERIF avd = actcraft-x86_64, android-34 google_apis x86_64, appareil pixel_tablet
VERIF pillow = OK (12.1.0)

Tâche L1-T17 (2026-10-15, `pwsh -NoProfile -File ./scripts/paquet-android.ps1`, sortie `.tmp/apk.txt`) :

VERIF apk-arm64 = OK (ActCraft-0.1.0-arm64-v8a.apk, 48.5 Mo)
VERIF apk-x86_64 = OK (ActCraft-0.1.0-x86_64.apk, 49.4 Mo)
VERIF apk-identifiant = io.github.jejeasterix.actcraft, versionCode 100, versionName 0.1.0 (propriétés de cible)
VERIF apk-api-cible = targetSdkVersion 36 (= ANDROID_PLATFORM, QT_ANDROID_TARGET_SDK_VERSION ; défaut de Qt 6.12 : 37), minSdkVersion 28
VERIF apk-permissions = aucune (manifeste à nous, android/AndroidManifest.xml)
VERIF apk-signature = clé de débogage locale (CN=Android Debug)
VERIF apk-licences = LICENCES OK
VERIF s3-local = ANDROID OK (émulateur actcraft-x86_64, 2560 × 1600 ; palette visible, trait lu sombre, aucun FATAL EXCEPTION ni SecurityException dans logcat pour le processus ; émulateur neuf : attendre charge < 4 et écarter une boîte ANR du système)

## Conséquences

- Émulateur : local (WHPX utilisable) ; la CI (KVM sur `ubuntu-latest`, tâche L1-T19) le double.
- Écart au plan : la doc de Qt 6.12 exige JDK 21 ou plus (et non 17), plateforme android-36, build-tools 36.0.0.
  Les deux JDK sont installés ; `dev-env-android.ps1` prend `jdk-21` (repli `jdk-17`). La CI doit installer JDK 21.
- `sdk_definitions.json` de Qt Creator n'a pas d'entrée pour Qt 6.x récent (dernière : 6.4, NDK 23) : la doc de Qt
  fait foi, aucune valeur n'en est reprise.
- APK (L1-T17) : les propriétés de cible `QT_ANDROID_PACKAGE_NAME`, `QT_ANDROID_VERSION_NAME` et
  `QT_ANDROID_VERSION_CODE` suffisent pour l'identifiant et la version. Les permissions, non : sans manifeste à nous,
  `aapt2 dump badging` relève `ACCESS_NETWORK_STATE` (Qt6Network), `INTERNET` et `WRITE_EXTERNAL_STORAGE` (Qt6Core,
  `lib/Qt6Core_arm64-v8a-android-dependencies.xml`), `READ_EXTERNAL_STORAGE` (implicite) et
  `io.github.jejeasterix.actcraft.DYNAMIC_RECEIVER_NOT_EXPORTED_PERMISSION` (androidx.core 1.17.0, fusion des
  manifestes). Repli : `android/AndroidManifest.xml`, gabarit de Qt sans le marqueur `%%INSERT_PERMISSIONS`, plus
  `tools:node="remove"` pour la permission d'androidx.core (le code Java de Qt appelle `Context.registerReceiver`,
  jamais `ContextCompat`).
- Chaîne Android sur le poste : `JAVA_HOME` du poste désigne un JRE (`~/.tools/jdk-21.0.11+10-jre`, sans `javac`),
  refusé par Gradle ; `dev-env-android.ps1` l'écarte au profit de `~/.tools/jdk-21`. Ninja de pip 1.13 est dans le
  dossier `Scripts` de Python (`ninja.BIN_DIR`), plus sous `ninja/data/bin`.
- Les outils en ligne de commande (build 11076708) ont la somme SHA-256 publiée par Qt Creator (4d6931209eebb1bfb7c7e8b240a6a3cb3ab24479ea294f3539429574b1eec862).
