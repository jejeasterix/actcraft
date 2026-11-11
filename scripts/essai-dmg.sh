#!/usr/bin/env bash
# Essai de l'image disque macOS (amendement 3 du plan du lot 11) : montée sans Finder, bundle complet, signature ad hoc
# valide, qt.conf, aucune dépendance hors du bundle (otool), `actcraft --version`, lancement de 20 s si --lancer, démontée.
# Bibliothèques réellement chargées (DYLD_PRINT_LIBRARIES) : toutes dans le bundle ou le système, à --version (cadres
# liés) et au lancement (greffons cocoa, images, QML, chargés après QGuiApplication) ; `--lancer` ouvre aussi un vrai
# cadre web depuis l'image. Journaux sous .tmp/. Une application qui ne se termine pas seule (fenêtre en attente d'une
# réponse) est arrêtée et l'essai échoue : ACTCRAFT_ESSAI_DELAI (secondes, 90 par défaut) borne le lancement.
# --h264 (implique --lancer) : le cadre ouvre tests/data/media/essai-h264.html, qui lit essai-h264.mp4 (H.264 et AAC,
# 4 s, mire et son synthétiques produits par ffmpeg) ; il doit en décoder 2 s (« ACTCRAFT H264 w=… h=… t=… ») et annoncer AAC lisible (codecs
# propriétaires de Qt WebEngine, release).
# Usage : scripts/essai-dmg.sh [<image.dmg>] [--lancer] [--h264]   (fenêtre ouverte 20 s sur l'écran du Mac)
# Dernière ligne et code : « ESSAI DMG OK » (0) ou « ESSAI DMG EN ECHEC : <raison> » (1).
set -u
cd "$(dirname "$0")/.." || exit 1
echec() {
  echo "ESSAI DMG EN ECHEC : $1"
  exit 1
}
. scripts/version-projet.sh
[ -n "$version" ] || echec "version introuvable dans CMakeLists.txt"
dmg=""
lancer=0
h264=0
for option in "$@"; do
  case "$option" in
    --lancer) lancer=1 ;;
    --h264) lancer=1 h264=1 ;;
    -*) echec "option inconnue : $option" ;;
    *) dmg="$option" ;;
  esac
done
[ -n "$dmg" ] || dmg="build/paquet/ActCraft-$version-macos-arm64.dmg"
[ -f "$dmg" ] || echec "image absente : $dmg"

mkdir -p .tmp
# Chemin réel (/private/tmp) : c'est celui qu'écrit dyld.
point="$(cd "$(mktemp -d /tmp/actcraft-dmg.XXXXXX)" && pwd -P)"
monte=0
enfant=""
fin() {
  if [ -n "$enfant" ]; then
    kill -KILL "$enfant" 2>/dev/null
    wait "$enfant" 2>/dev/null
  fi
  if [ "$monte" = 1 ]; then
    hdiutil detach "$point" -quiet || hdiutil detach "$point" -force -quiet
  fi
  rmdir "$point" 2>/dev/null
}
trap fin EXIT
trap 'exit 143' TERM INT HUP

# Lance une commande et l'arrête si elle dépasse <secondes> : code 124. Sans processus de garde à côté, rien ne survit
# à l'essai, et le jeton est rendu.
borne() {
  local limite=$(($1 * 2)) tours=0 code=0
  shift
  "$@" &
  enfant=$!
  while kill -0 "$enfant" 2>/dev/null; do
    if [ "$tours" -ge "$limite" ]; then
      kill -TERM "$enfant" 2>/dev/null
      sleep 3
      kill -KILL "$enfant" 2>/dev/null
      wait "$enfant" 2>/dev/null
      enfant=""
      return 124
    fi
    sleep 0.5
    tours=$((tours + 1))
  done
  wait "$enfant" || code=$?
  enfant=""
  return "$code"
}

# Chemins chargés, complets (espaces compris) : préfixe « dyld[pid]: <UUID> » retiré de chaque ligne du journal.
chemins_dyld() {
  sed -nE 's/^dyld\[[0-9]+\]: <[^>]*> //p' "$1"
}
# Chemins hors du bundle monté et du système, écrits dans $2 ; vrai si aucun. Admis : le bundle, /System/, /usr/lib/,
# /Library/ hors /Library/Frameworks/ (greffons du système, méthodes de saisie) ; refusés : tout le reste, dont
# /opt/homebrew, /usr/local et /Library/Frameworks.
hors_systeme() {
  awk -v b="$point/" 'index($0, b) != 1 && index($0, "/System/") != 1 && index($0, "/usr/lib/") != 1 &&
    !(index($0, "/Library/") == 1 && index($0, "/Library/Frameworks/") != 1)' "$1" > "$2"
  [ ! -s "$2" ]
}
hdiutil attach -nobrowse -readonly -noautoopen -mountpoint "$point" "$dmg" > /dev/null || echec "hdiutil attach"
monte=1

app="$point/ActCraft.app"
exe="$app/Contents/MacOS/actcraft"
[ -x "$exe" ] || echec "ActCraft.app/Contents/MacOS/actcraft absent"
[ -L "$point/Applications" ] || echec "lien Applications absent"
[ -f "$app/Contents/Resources/THIRD_PARTY_NOTICES.md" ] || echec "notices absentes du bundle"
nom="$(plutil -extract CFBundleName raw "$app/Contents/Info.plist" 2>/dev/null)"
[ "$nom" = ActCraft ] || echec "CFBundleName = « $nom », attendu « ActCraft »"
grep -q 'PlugIns' "$app/Contents/Resources/qt.conf" 2>/dev/null || echec "qt.conf absent ou sans chemin des greffons"
codesign --verify --deep --strict "$app" 2> /dev/null || echec "signature ad hoc invalide"
# Minimum macOS déclaré et respecté par chaque binaire du bundle monté.
minos="$(scripts/verifier-minos-mac.sh "$app")" || echec "minimum macOS : $minos"
echo "$minos"

# Tous les fichiers (binaires de framework sans extension ni droit x) ; les non Mach-O sont ignorés.
restes="$(find "$app" -type f -exec otool -L {} + 2>/dev/null |
  grep -E '^[[:space:]]+(/opt/homebrew|/usr/local)/' | sort -u)"
[ -z "$restes" ] || echec "dépendances hors du bundle : $(echo "$restes" | head -3 | tr -s '[:space:]' ' ')"

sortie="$(borne 60 "$exe" --version)" || echec "--version : code $?"
[ "$sortie" = "ActCraft $version" ] || echec "--version a rendu « $sortie », attendu « ActCraft $version »"
borne 60 env DYLD_PRINT_LIBRARIES=1 "$exe" --version > /dev/null 2> .tmp/essai-dmg-dyld-version.txt ||
  echec "--version sous dyld : code $?"
chemins_dyld .tmp/essai-dmg-dyld-version.txt > .tmp/essai-dmg-chemins-version.txt
grep -qF "$app/Contents/Frameworks/QtCore.framework/" .tmp/essai-dmg-chemins-version.txt ||
  echec "journal de dyld muet ou QtCore chargé hors du bundle (.tmp/essai-dmg-dyld-version.txt)"
hors_systeme .tmp/essai-dmg-chemins-version.txt .tmp/essai-dmg-refuses-version.txt ||
  echec "bibliothèques chargées hors du bundle : $(head -3 .tmp/essai-dmg-refuses-version.txt | tr '\n' ';') (liste : .tmp/essai-dmg-refuses-version.txt)"
echo "VERSION OK : $sortie"

if [ "$lancer" = 1 ]; then
  echo "Fenêtre d'ActCraft ouverte 20 s sur l'écran du Mac (un vrai cadre web)."
  page="$(pwd -P)/.tmp/essai-cadre.html"
  printf '%s\n' '<title>ESSAI</title><script>console.warn("ACTCRAFT JS OK")</script><p>Cadre chargé.</p>' > "$page"
  if [ "$h264" = 1 ]; then
    cp tests/data/media/essai-h264.html tests/data/media/essai-h264.mp4 .tmp/ || echec "copie de tests/data/media/essai-h264.*"
    page="$(pwd -P)/.tmp/essai-h264.html"
  fi
  delai="${ACTCRAFT_ESSAI_DELAI:-90}"
  borne "$delai" env QT_FORCE_STDERR_LOGGING=1 DYLD_PRINT_LIBRARIES=1 "$exe" --essai-cadre-web "$page" --quitter-apres 20000 > /dev/null 2> .tmp/essai-dmg-dyld-lancement.txt
  code=$?
  [ "$code" != 124 ] || echec "l'application ne s'est pas terminée seule en $delai s, arrêtée (.tmp/essai-dmg-dyld-lancement.txt)"
  [ "$code" = 0 ] || echec "lancement : code $code (.tmp/essai-dmg-dyld-lancement.txt)"
  chemins_dyld .tmp/essai-dmg-dyld-lancement.txt > .tmp/essai-dmg-chemins-lancement.txt
  grep -qF "$app/Contents/PlugIns/platforms/libqcocoa.dylib" .tmp/essai-dmg-chemins-lancement.txt ||
    echec "greffon cocoa non chargé depuis le bundle (.tmp/essai-dmg-dyld-lancement.txt)"
  grep -qF "$app/Contents/Frameworks/QtWebEngineCore.framework/" .tmp/essai-dmg-chemins-lancement.txt ||
    echec "QtWebEngineCore non chargé depuis le bundle (.tmp/essai-dmg-dyld-lancement.txt)"
  grep -q 'ACTCRAFT CADRE WEB CHARGE' .tmp/essai-dmg-dyld-lancement.txt ||
    echec "le cadre web n'a pas fini de charger (.tmp/essai-dmg-dyld-lancement.txt)"
  grep -q 'ACTCRAFT JS OK' .tmp/essai-dmg-dyld-lancement.txt ||
    echec "le script de la page n'a pas tourné (.tmp/essai-dmg-dyld-lancement.txt)"
  echo "CADRE WEB OK"
  if [ "$h264" = 1 ]; then
    ligne="$(grep -o 'ACTCRAFT H264 w=[1-9][0-9]* h=[1-9][0-9]* t=[0-9.]*' .tmp/essai-dmg-dyld-lancement.txt | head -1)"
    [ -n "$ligne" ] || echec "vidéo H.264 non décodée dans le cadre : $(grep -o 'ACTCRAFT H264 ECHEC[^"]*' .tmp/essai-dmg-dyld-lancement.txt | head -1) (.tmp/essai-dmg-dyld-lancement.txt)"
    grep -q 'ACTCRAFT AAC probably' .tmp/essai-dmg-dyld-lancement.txt ||
      echec "AAC non lisible dans le cadre : $(grep -o 'ACTCRAFT AAC[^"]*' .tmp/essai-dmg-dyld-lancement.txt | head -1)"
    echo "$ligne"
    echo "AAC OK"
  fi
  hors_systeme .tmp/essai-dmg-chemins-lancement.txt .tmp/essai-dmg-refuses-lancement.txt ||
    echec "greffons ou bibliothèques chargés hors du bundle : $(head -3 .tmp/essai-dmg-refuses-lancement.txt | tr '\n' ';') (liste : .tmp/essai-dmg-refuses-lancement.txt)"
  echo "LANCEMENT OK"
fi
echo "ESSAI DMG OK"
