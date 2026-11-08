#!/bin/bash
# Émulateur Android du Mac mini (runner auto-hébergé, Apple Silicon) : démarre l'AVD arm64-v8a, attend la fin du
# démarrage, lance la commande donnée, puis arrête l'émulateur quoi qu'il arrive. Remplace android-emulator-runner
# sur le Mac : l'action range ses AVD dans ~/.android/avd ; ici tout reste dans le cache du runner (ANDROID_PREFS_ROOT,
# ANDROID_AVD_HOME posés par outils-android-mac.sh). L'image x86_64 de la CI Linux ne tourne pas sur un Mac ARM :
# l'essai porte sur l'APK arm64-v8a, rendu par le GPU du Mac (-gpu host, comme en local, voir essai-android.ps1).
# Usage : scripts/ci/emulateur-mac.sh <commande> [arguments...]   (code de sortie : celui de la commande)
# Variables : ANDROID_SDK_ROOT, ANDROID_PREFS_ROOT, ANDROID_AVD_HOME, et celles de packaging/android/versions.env.
set -euo pipefail
cd "$(dirname "$0")/../.."
# shellcheck disable=SC1091
. packaging/android/versions.env
: "${ANDROID_SDK_ROOT:?}" "${ANDROID_AVD_HOME:?}" "${ANDROID_PREFS_ROOT:?}"
# adb range sa clé dans $HOME/.android, quel que soit ANDROID_PREFS_ROOT : HOME pointe dans le cache du runner pour
# tout le script, commande comprise (pwsh y range aussi sa configuration).
export HOME="$ANDROID_PREFS_ROOT/home"
mkdir -p "$HOME"

avd=actcraft-ci
# Isolé des sessions du Mac : serveur adb propre au job (port distinct de 5037, que le trap peut arrêter sans couper
# l'adb des sessions), port d'émulateur libre (une session peut tenir 5554), appareil désigné par ANDROID_SERIAL
# (android-commun.ps1 appelle adb sans -s).
export ANDROID_ADB_SERVER_PORT=5047
port=""
for p in $(seq 5584 -2 5554); do
  if ! lsof -nP -iTCP:"$p" -iTCP:"$((p + 1))" -sTCP:LISTEN > /dev/null 2>&1; then port=$p; break; fi
done
[ -n "$port" ] || { echo "::error::Aucun port d'émulateur libre entre 5554 et 5585"; exit 1; }
serie="emulator-$port"
export ANDROID_SERIAL="$serie"
image="system-images;android-$ANDROID_EMULATOR_API;google_apis;arm64-v8a"
proprietes="$ANDROID_SDK_ROOT/system-images/android-$ANDROID_EMULATOR_API/google_apis/arm64-v8a/source.properties"
adb="$ANDROID_SDK_ROOT/platform-tools/adb"
emulateur="$ANDROID_SDK_ROOT/emulator/emulator"

# Image système épinglée (versions.env) : le rendu attendu par l'essai est prouvé sur cette révision.
grep Pkg.Revision "$proprietes"
grep -qx "Pkg.Revision=$ANDROID_SYSTEM_IMAGE_REVISION" "$proprietes"

# AVD recréé à chaque job : profil de versions.env, 2 Go de mémoire (le Mac a 8 Go et porte les sessions Claude).
mkdir -p "$ANDROID_AVD_HOME"
echo no | "$ANDROID_SDK_ROOT/cmdline-tools/latest/bin/avdmanager" create avd --force -n "$avd" \
  --package "$image" --device "$ANDROID_AVD_PROFILE" > /dev/null
printf 'hw.ramSize=2048\nhw.keyboard=yes\n' >> "$ANDROID_AVD_HOME/$avd.avd/config.ini"

journal="${RUNNER_TEMP:-/tmp}/emulateur.log"
"$adb" start-server
"$emulateur" -avd "$avd" -port "$port" -no-window -gpu host -noaudio -no-boot-anim -no-snapshot -wipe-data \
  -no-metrics > "$journal" 2>&1 &
pid=$!
arreter() {
  "$adb" -s "$serie" emu kill > /dev/null 2>&1 || true
  for _ in $(seq 1 30); do kill -0 "$pid" 2> /dev/null || break; sleep 1; done
  kill -9 "$pid" 2> /dev/null || true
  # Le serveur adb de ce job (port ANDROID_ADB_SERVER_PORT) ne survit pas au job ; celui des sessions n'est pas touché.
  "$adb" kill-server > /dev/null 2>&1 || true
}
trap arreter EXIT

# Démarrage : 5 min au plus.
demarre=""
for _ in $(seq 1 150); do
  if ! kill -0 "$pid" 2> /dev/null; then echo "::error::L'émulateur s'est arrêté au démarrage"; tail -n 40 "$journal"; exit 1; fi
  if [ "$("$adb" -s "$serie" shell getprop sys.boot_completed 2> /dev/null | tr -d '\r')" = "1" ]; then demarre=1; break; fi
  sleep 2
done
if [ -z "$demarre" ]; then echo "::error::Émulateur non démarré après 5 min"; tail -n 40 "$journal"; exit 1; fi
echo "Émulateur démarré ($serie)."
"$adb" -s "$serie" shell input keyevent 82 || true
for reglage in window_animation_scale transition_animation_scale animator_duration_scale; do
  "$adb" -s "$serie" shell settings put global "$reglage" 0.0
done

set +e
"$@"
code=$?
set -e
exit "$code"
