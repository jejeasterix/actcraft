#!/usr/bin/env bash
# Essai de l'image disque macOS (amendement 3 du plan du lot 11) : montée sans Finder, bundle complet, signature ad hoc
# valide, qt.conf, aucune dépendance hors du bundle (otool), `actcraft --version`, lancement de 3 s si --lancer, démontée.
# Bibliothèques réellement chargées (DYLD_PRINT_LIBRARIES) : toutes dans le bundle ou le système, à --version (cadres
# liés) et au lancement (greffons cocoa, images, webview, QML, chargés après QGuiApplication). Journaux sous .tmp/.
# Usage : scripts/essai-dmg.sh [<image.dmg>] [--lancer]   (--lancer : fenêtre ouverte 3 s sur l'écran du Mac)
# Dernière ligne et code : « ESSAI DMG OK » (0) ou « ESSAI DMG EN ECHEC : <raison> » (1).
set -u
cd "$(dirname "$0")/.." || exit 1
echec() {
  echo "ESSAI DMG EN ECHEC : $1"
  exit 1
}
version="$(perl -0777 -ne 'print "$1.$2.$3" if /project\(\s*ActCraft\s+VERSION\s+(\d+)\.(\d+)\.(\d+)/' CMakeLists.txt)"
[ -n "$version" ] || echec "version introuvable dans CMakeLists.txt"
dmg=""
lancer=0
for option in "$@"; do
  case "$option" in
    --lancer) lancer=1 ;;
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
fin() {
  if [ "$monte" = 1 ]; then
    hdiutil detach "$point" -quiet || hdiutil detach "$point" -force -quiet
  fi
  rmdir "$point" 2>/dev/null
}
trap fin EXIT

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

# Tous les fichiers (binaires de framework sans extension ni droit x) ; les non Mach-O sont ignorés.
restes="$(find "$app" -type f -exec otool -L {} + 2>/dev/null |
  grep -E '^[[:space:]]+(/opt/homebrew|/usr/local)/' | sort -u)"
[ -z "$restes" ] || echec "dépendances hors du bundle : $(echo "$restes" | head -3 | tr -s '[:space:]' ' ')"

sortie="$("$exe" --version)" || echec "--version : code $?"
[ "$sortie" = "ActCraft $version" ] || echec "--version a rendu « $sortie », attendu « ActCraft $version »"
DYLD_PRINT_LIBRARIES=1 "$exe" --version > /dev/null 2> .tmp/essai-dmg-dyld-version.txt
chemins_dyld .tmp/essai-dmg-dyld-version.txt > .tmp/essai-dmg-chemins-version.txt
grep -qF "$app/Contents/Frameworks/QtCore.framework/" .tmp/essai-dmg-chemins-version.txt ||
  echec "journal de dyld muet ou QtCore chargé hors du bundle (.tmp/essai-dmg-dyld-version.txt)"
hors_systeme .tmp/essai-dmg-chemins-version.txt .tmp/essai-dmg-refuses-version.txt ||
  echec "bibliothèques chargées hors du bundle : $(head -3 .tmp/essai-dmg-refuses-version.txt | tr '\n' ';') (liste : .tmp/essai-dmg-refuses-version.txt)"
echo "VERSION OK : $sortie"

if [ "$lancer" = 1 ]; then
  echo "Fenêtre d'ActCraft ouverte 3 s sur l'écran du Mac."
  DYLD_PRINT_LIBRARIES=1 "$exe" --quitter-apres 3000 > /dev/null 2> .tmp/essai-dmg-dyld-lancement.txt ||
    echec "lancement : code $? (.tmp/essai-dmg-dyld-lancement.txt)"
  chemins_dyld .tmp/essai-dmg-dyld-lancement.txt > .tmp/essai-dmg-chemins-lancement.txt
  grep -qF "$app/Contents/PlugIns/platforms/libqcocoa.dylib" .tmp/essai-dmg-chemins-lancement.txt ||
    echec "greffon cocoa non chargé depuis le bundle (.tmp/essai-dmg-dyld-lancement.txt)"
  hors_systeme .tmp/essai-dmg-chemins-lancement.txt .tmp/essai-dmg-refuses-lancement.txt ||
    echec "greffons ou bibliothèques chargés hors du bundle : $(head -3 .tmp/essai-dmg-refuses-lancement.txt | tr '\n' ';') (liste : .tmp/essai-dmg-refuses-lancement.txt)"
  echo "LANCEMENT OK"
fi
echo "ESSAI DMG OK"
