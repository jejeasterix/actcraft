#!/usr/bin/env bash
# Minimum macOS d'un ActCraft.app déployé : l'Info.plist déclare LSMinimumSystemVersion, et aucun binaire Mach-O du
# bundle (LC_BUILD_VERSION ou LC_VERSION_MIN_MACOSX, par otool -l) n'exige un macOS plus récent. Sans ce contrôle, une
# bibliothèque de Homebrew construite pour un macOS récent plante au lancement sur un macOS plus ancien au lieu de
# laisser le Finder afficher un refus clair.
# Usage : scripts/verifier-minos-mac.sh <ActCraft.app>
# Dernière ligne et code : « MINOS OK : minimum <version> » (0) ou « MINOS EN ECHEC : <raison> » (1).
set -u
app="${1:-}"
echec() {
  echo "MINOS EN ECHEC : $1"
  exit 1
}
[ -f "$app/Contents/Info.plist" ] || echec "Info.plist absent (${app:-aucun chemin})"
minimum="$(plutil -extract LSMinimumSystemVersion raw "$app/Contents/Info.plist" 2> /dev/null)" ||
  echec "LSMinimumSystemVersion absent de Contents/Info.plist : macOS ne refuse pas un système trop ancien"
case "$minimum" in
  [0-9]*) ;;
  *) echec "LSMinimumSystemVersion illisible : « $minimum »" ;;
esac

# Vrai si la version $1 est strictement plus récente que $2 (comparaison par composants : 27 = 27.0 = 27.0.0).
plus_recente() {
  awk -v a="$1" -v b="$2" 'BEGIN {
    na = split(a, x, "."); nb = split(b, y, ".")
    for (i = 1; i <= (na > nb ? na : nb); i++) {
      if (x[i] + 0 > y[i] + 0) exit 0
      if (x[i] + 0 < y[i] + 0) exit 1
    }
    exit 1
  }'
}

# Un Mach-O par ligne « <chemin><tab><minos> », tranche arm64 seule (le paquet ne vise que Apple Silicon) ; un fichier sans commande de version, ou qui n'est
# pas Mach-O (« is not an object file »), n'écrit rien.
releve="$(mktemp /tmp/actcraft-minos.XXXXXX)" || echec "fichier temporaire"
trap 'rm -f "$releve"' EXIT
find "$app" -type f -exec otool -arch arm64 -l {} + 2> /dev/null |
  awk '/^[^[:space:]].*:$/ { f = substr($0, 1, length($0) - 1); next }
       $1 == "cmd" { c = $2; next }
       c == "LC_BUILD_VERSION" && $1 == "minos" { print f "\t" $2; c = "" }
       c == "LC_VERSION_MIN_MACOSX" && $1 == "version" { print f "\t" $2; c = "" }' > "$releve"
# Liste non aveugle : l'exécutable est un Mach-O, il doit y figurer.
[ -s "$releve" ] || echec "aucun minos lu dans $app (otool -l muet)"

trop_recents=""
while IFS=$'\t' read -r fichier minos; do
  if plus_recente "$minos" "$minimum"; then
    trop_recents="$trop_recents ${fichier#"$app"/} (minos $minos)"
  fi
done < "$releve"
[ -z "$trop_recents" ] ||
  echec "binaires qui exigent un macOS plus récent que le minimum déclaré $minimum :$trop_recents"
echo "MINOS OK : minimum $minimum"
