#!/bin/bash
# Chaîne Android du Mac mini (runner auto-hébergé) : tout dans le cache du runner (~/actions-runner-actcraft/_cache),
# rien d'installé ailleurs sur le Mac. Installé une fois, puis réutilisé d'un job à l'autre ; chaque élément a son
# témoin (.complet), écrit en dernier : une installation interrompue est refaite au job suivant.
# - Python (venv) : aqtinstall épinglé comme dans ci.yml, Pillow (lire-pixel.py de l'essai) ;
# - Qt $QT_VERSION hôte macOS et Qt pour Android des ABI demandées, mêmes modules que la CI Linux ;
# - SDK Android : cmdline-tools, NDK, build-tools, plateforme, platform-tools de versions.env ; avec l'ABI arm64-v8a
#   demandée pour l'essai (-Emulateur), l'émulateur épinglé et son image système arm64-v8a ;
# - Gradle, les réglages Android et l'AVD rangés dans le cache (GRADLE_USER_HOME, ANDROID_PREFS_ROOT, ANDROID_AVD_HOME).
#   ANDROID_PREFS_ROOT et non ANDROID_USER_HOME : le plugin Gradle d'Android écrit ~/.android/analytics.settings malgré
#   ANDROID_USER_HOME, et refuse les deux variables ensemble quand elles diffèrent.
# Le JDK 21 vient de actions/setup-java (JAVA_HOME). Écrit les variables du job dans $GITHUB_ENV et $GITHUB_PATH.
# Usage : scripts/ci/outils-android-mac.sh [--emulateur] <abi>...   (abi : arm64_v8a, x86_64)
set -euo pipefail
cd "$(dirname "$0")/../.."
# shellcheck disable=SC1091
. packaging/android/versions.env
: "${QT_VERSION:?}" "${JAVA_HOME:?}" "${GITHUB_ENV:?}" "${GITHUB_PATH:?}"

emulateur=""
if [ "${1:-}" = "--emulateur" ]; then emulateur=1; shift; fi
[ $# -gt 0 ] || { echo "::error::Aucune ABI demandée"; exit 1; }

cache="$HOME/actions-runner-actcraft/_cache"
aqtinstall="git+https://github.com/miurahr/aqtinstall@076e1659807d0b362a3ed684d54c2e9c775eb9c7"
modules=(qtshadertools qtimageformats qtpdf qtmultimedia qtwebview)
outils_ligne=14742923 # cmdline-tools, même version que android-emulator-runner v2.38.0

venv="$cache/venv-android"
if [ ! -f "$venv/.complet" ]; then
  rm -rf "$venv"
  python3 -m venv "$venv"
  "$venv/bin/pip" install -q pillow "$aqtinstall"
  touch "$venv/.complet"
fi

qt="$cache/Qt-android"
installer_qt() { # <témoin> <arguments d'aqt install-qt>
  local temoin="$qt/$QT_VERSION/.complet-$1"
  shift
  if [ ! -f "$temoin" ]; then
    "$venv/bin/python" -m aqt install-qt "$@" "$QT_VERSION" "${arch_qt}" -m "${modules[@]}" --outputdir "$qt"
    touch "$temoin"
  fi
}
arch_qt=clang_64 installer_qt macos mac desktop
for abi in "$@"; do
  arch_qt="android_$abi" installer_qt "android_$abi" all_os android
done

sdk="$cache/android-sdk"
sdkmanager="$sdk/cmdline-tools/latest/bin/sdkmanager"
if [ ! -x "$sdkmanager" ]; then
  rm -rf "$sdk/cmdline-tools"
  mkdir -p "$sdk/cmdline-tools"
  curl -sfLo "$RUNNER_TEMP/outils.zip" "https://dl.google.com/android/repository/commandlinetools-mac-${outils_ligne}_latest.zip"
  unzip -q "$RUNNER_TEMP/outils.zip" -d "$sdk/cmdline-tools"
  mv "$sdk/cmdline-tools/cmdline-tools" "$sdk/cmdline-tools/latest"
  rm -f "$RUNNER_TEMP/outils.zip"
fi
prefs="$cache/android-user"
mkdir -p "$prefs/avd"
paquets=(platform-tools "ndk;$ANDROID_NDK_VERSION" "build-tools;$ANDROID_BUILD_TOOLS" "platforms;android-$ANDROID_PLATFORM")
if [ -n "$emulateur" ]; then
  paquets+=(emulator "system-images;android-$ANDROID_EMULATOR_API;google_apis;arm64-v8a")
fi
temoin_sdk="$sdk/.complet-$(printf '%s\n' "${paquets[@]}" | shasum | cut -c1-12)"
if [ ! -f "$temoin_sdk" ]; then
  yes | "$sdkmanager" --sdk_root="$sdk" --licenses > /dev/null || true
  "$sdkmanager" --sdk_root="$sdk" "${paquets[@]}" > "$RUNNER_TEMP/sdkmanager.log" || { tail -n 30 "$RUNNER_TEMP/sdkmanager.log"; exit 1; }
  touch "$temoin_sdk"
fi
# Émulateur épinglé (versions.env), posé sur le paquet « emulator » de sdkmanager comme le fait android-emulator-runner.
if [ -n "$emulateur" ] && [ ! -f "$sdk/emulator/.build-$ANDROID_EMULATOR_BUILD" ]; then
  curl -sfLo "$RUNNER_TEMP/emulateur.zip" "https://dl.google.com/android/repository/emulator-darwin_aarch64-$ANDROID_EMULATOR_BUILD.zip"
  unzip -o -q "$RUNNER_TEMP/emulateur.zip" -d "$sdk"
  rm -f "$RUNNER_TEMP/emulateur.zip"
  touch "$sdk/emulator/.build-$ANDROID_EMULATOR_BUILD"
fi

arm64="$qt/$QT_VERSION/android_arm64_v8a"
x86_64="$qt/$QT_VERSION/android_x86_64"
# Une seule ABI (release.yml) : l'autre variable désigne le même dossier pour que dev-env-android.ps1 ne s'arrête pas.
[ -d "$arm64" ] || arm64="$x86_64"
[ -d "$x86_64" ] || x86_64="$arm64"
{
  echo "ANDROID_SDK_ROOT=$sdk"
  echo "ANDROID_HOME=$sdk"
  echo "ANDROID_NDK_ROOT=$sdk/ndk/$ANDROID_NDK_VERSION"
  echo "ANDROID_PREFS_ROOT=$prefs"
  echo "ANDROID_AVD_HOME=$prefs/avd"
  echo "GRADLE_USER_HOME=$cache/gradle"
  echo "ACTCRAFT_QT_HOST_DIR=$qt/$QT_VERSION/macos"
  echo "ACTCRAFT_QT_ANDROID_ARM64_DIR=$arm64"
  echo "ACTCRAFT_QT_ANDROID_X86_64_DIR=$x86_64"
  # 4 compilations à la fois, comme le build macOS : le Mac (8 Go) garde de la mémoire pour les sessions.
  echo "CMAKE_BUILD_PARALLEL_LEVEL=4"
} >> "$GITHUB_ENV"
if command -v ccache > /dev/null; then
  {
    echo "CMAKE_C_COMPILER_LAUNCHER=ccache"
    echo "CMAKE_CXX_COMPILER_LAUNCHER=ccache"
    echo "CCACHE_DIR=$cache/ccache"
  } >> "$GITHUB_ENV"
fi
# python3 du venv (Pillow) avant celui du système ; adb dans le PATH (essai-android.ps1).
echo "$sdk/platform-tools" >> "$GITHUB_PATH"
echo "$venv/bin" >> "$GITHUB_PATH"
du -sh "$cache" | sed 's/^/Cache du runner : /'
df -h "$cache" | tail -n 1
