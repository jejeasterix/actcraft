#!/usr/bin/env bash
# Licences d'un ActCraft.app déployé (décision 34 ; amendement 3 du plan du lot 11). Chaque framework Qt
# (QtCore.framework → Qt6Core) est dans packaging/qt-autorises.txt ; chaque autre bibliothèque de Contents/Frameworks a
# sa ligne « <motif glob> <expression SPDX> » dans packaging/autres-autorises.txt ; aucune sous GPL seule. Les greffons
# (Contents/PlugIns) suivent leur module et ne sont pas comptés ; leurs dépendances sont dans Contents/Frameworks.
# Usage : scripts/verifier-licences-mac.sh <ActCraft.app>
# Dernière ligne et code : « LICENCES OK » (0) ou « LICENCES EN ECHEC : <noms> » (1).
set -u
racine="$(cd "$(dirname "$0")/.." && pwd)"
app="${1:-}"
if [ -z "$app" ] || [ ! -d "$app/Contents/Frameworks" ]; then
  echo "LICENCES EN ECHEC : bundle absent ou sans Contents/Frameworks (${app:-aucun chemin})"
  exit 1
fi
qt_liste="$racine/packaging/qt-autorises.txt"
autres_liste="$racine/packaging/autres-autorises.txt"

# Vrai si l'expression SPDX offre au moins une licence qui n'est pas la GPL (la LGPL convient, liée dynamiquement).
sans_gpl_seule() {
  local alt
  local IFS='|'
  for alt in ${1// OR /|}; do
    case "$alt" in
      *LGPL*) return 0 ;;
      *GPL*) ;;
      *) return 0 ;;
    esac
  done
  return 1
}

# Expression SPDX de la première ligne dont le motif couvre le nom, vide sinon.
licence_de() {
  local motif expression
  while read -r motif expression; do
    case "$motif" in '' | '#'*) continue ;; esac
    # shellcheck disable=SC2254 # le motif est un glob voulu
    case "$1" in $motif) echo "$expression"; return 0 ;; esac
  done < "$autres_liste"
  return 1
}

refuses=""
modules=""
for fw in "$app"/Contents/Frameworks/*.framework; do
  [ -d "$fw" ] || continue
  nom="$(basename "$fw" .framework)"
  case "$nom" in
    Qt*)
      module="Qt6${nom#Qt}"
      modules="$modules $module"
      grep -qxF "$module" "$qt_liste" || refuses="$refuses $module"
      ;;
    *)
      licence="$(licence_de "$nom.framework")" || { refuses="$refuses $nom.framework"; continue; }
      sans_gpl_seule "$licence" || refuses="$refuses $nom.framework(GPL)"
      echo "  $nom.framework : $licence"
      ;;
  esac
done
if [ -z "$modules" ]; then
  echo "LICENCES EN ECHEC : aucun framework Qt trouvé"
  exit 1
fi
echo "Modules Qt :$modules"
for lib in "$app"/Contents/Frameworks/*.dylib; do
  [ -f "$lib" ] || continue
  nom="$(basename "$lib")"
  licence="$(licence_de "$nom")" || { refuses="$refuses $nom"; continue; }
  sans_gpl_seule "$licence" || refuses="$refuses $nom(GPL)"
  echo "  $nom : $licence"
done
# Qt WebEngine livré avec les codecs propriétaires (release) : l'avis sur les brevets H.264 et AAC doit être dans le
# bundle (packaging/qt-autorises.txt, fin du fichier).
if [ -d "$app/Contents/Frameworks/QtWebEngineCore.framework" ] &&
  ! grep -qx '### Brevets des codecs H.264 et AAC' "$app/Contents/Resources/THIRD_PARTY_NOTICES.md" 2>/dev/null; then
  refuses="$refuses avis-brevets-H.264(Contents/Resources/THIRD_PARTY_NOTICES.md)"
fi
if [ -n "$refuses" ]; then
  echo "LICENCES EN ECHEC :$refuses"
  exit 1
fi
echo "LICENCES OK"
