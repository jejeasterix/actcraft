#!/usr/bin/env bash
# Paquet macOS non signé (amendement 3 du plan du lot 11) : construction Release, déploiement de Qt par macdeployqt,
# notices, licences, signature ad hoc (ni Developer ID ni notarisation), image disque.
# Usage : scripts/paquet-mac.sh [--sans-construire]
# Qt : $ACTCRAFT_QT_DIR, sinon $(brew --prefix qt). Produit build/paquet/ActCraft-<version>-macos-arm64.dmg : local,
# jamais publié ici (publication = go écrit de Jérôme).
# Dernière ligne et code : « DMG OK : <chemin> » (0) ou « DMG EN ECHEC : <raison> » (1).
set -u
cd "$(dirname "$0")/.." || exit 1
racine="$(pwd)"
echec() {
  echo "DMG EN ECHEC : $1"
  exit 1
}

construire=1
for option in "$@"; do
  case "$option" in
    --sans-construire) construire=0 ;;
    *) echec "option inconnue : $option" ;;
  esac
done

. scripts/version-projet.sh
[ -n "$version" ] || echec "version introuvable dans CMakeLists.txt (project(ActCraft VERSION x.y.z))"
[ "$(uname -m)" = arm64 ] || echec "Mac Apple Silicon requis (uname -m = $(uname -m))"
qt="${ACTCRAFT_QT_DIR:-$(brew --prefix qt 2>/dev/null)}"
macdeployqt="$qt/bin/macdeployqt"
[ -x "$macdeployqt" ] || echec "macdeployqt introuvable ($macdeployqt) : ACTCRAFT_QT_DIR ou brew install qt"

build="$racine/build/macos-paquet"
if [ "$construire" = 1 ]; then
  lanceur=""
  if command -v ccache >/dev/null; then
    lanceur="-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
  fi
  # shellcheck disable=SC2086 # $lanceur : zéro ou deux options
  cmake -S . -B "$build" -G Ninja -DCMAKE_PREFIX_PATH="$qt" -DCMAKE_BUILD_TYPE=Release $lanceur ||
    echec "configuration CMake"
  cmake --build "$build" --target actcraft --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}" || echec "compilation Release"
fi
# Lecteur, enregistreurs et cadre web sont livrés : une construction qui les a perdus (module Qt manquant) est refusée.
for option_cmake in ACTCRAFT_MEDIA ACTCRAFT_WEBFRAME; do
  grep -qx "$option_cmake:BOOL=ON" "$build/CMakeCache.txt" 2>/dev/null ||
    echec "$option_cmake n'est pas ON dans $build/CMakeCache.txt (Qt Multimedia ou Qt WebEngine manquant)"
done
source_app="$(find "$build" -maxdepth 2 -name actcraft.app -type d | head -1)"
[ -n "$source_app" ] || echec "actcraft.app absent de $build (construire d'abord, sans --sans-construire)"

paquet="$racine/build/paquet"
stage="$paquet/macos"
app="$stage/ActCraft.app"
rm -rf "$stage"
mkdir -p "$stage"
cp -R "$source_app" "$app" || echec "copie du bundle"

journal="$paquet/macdeployqt.txt"
"$macdeployqt" "$app" -qmldir="$racine/ui/qml" -verbose=1 > "$journal" 2>&1 || echec "macdeployqt (voir $journal)"
# macdeployqt rend 0 même quand il n'a pas tout copié : ses lignes ERROR font foi. Le module ActCraft.Ui est lié
# statiquement (actcraft_uiplugin) : qmlimportscanner ne le trouve pas sur disque, ce n'est pas un manque.
# « Cannot resolve rpath "@rpath/…" » (et sa ligne « using QList(…) ») : Homebrew range chaque module dans sa formule
# et le rpath d'un greffon y est relatif à sa place (@loader_path/../../../../lib), que macdeployqt évalue depuis le
# bundle. Ces lignes ne sont pas un verdict : la référence est réécrite plus bas si son framework est dans le bundle,
# son greffon est retiré si son module n'est pas importé, et toute référence @rpath restée sans cible est refusée
# après la boucle des retraits.
# Qt officiel (aqt) : greffons SQL des bases externes (Mimer, ODBC, PostgreSQL), liés à des clients absents du Mac ;
# macdeployqt signale chacun de ces clients (« can't open file », « no file at »). ActCraft n'utilise que SQLite (stockage
# local de Qt Quick) : ces greffons sont retirés, et seules les lignes qui nomment un de leurs clients sont écartées.
sql_externes="libqsqlmimer.dylib libqsqlodbc.dylib libqsqlpsql.dylib"
clients_sql="$journal.clients-sql"
: > "$clients_sql"
for greffon in $sql_externes; do
  [ -f "$qt/plugins/sqldrivers/$greffon" ] || continue
  otool -arch arm64 -L "$qt/plugins/sqldrivers/$greffon" | awk 'NR > 1 && $1 ~ /^\// && $1 !~ /^\/(usr\/lib|System)\// { print $1 }' >> "$clients_sql"
  rm -f "$app/Contents/PlugIns/sqldrivers/$greffon"
done
erreur="$(grep '^ERROR' "$journal" | grep -v 'ActCraft\.Ui' |
  grep -vE '^ERROR: +using QList\(|^ERROR: Cannot resolve rpath "@rpath/' |
  { if [ -s "$clients_sql" ]; then grep -vF -f "$clients_sql"; else cat; fi; } | head -1)"
rm -f "$clients_sql"
[ -z "$erreur" ] || echec "macdeployqt : $erreur"

# Qt WebEngine (cadrage DA4) : le greffon référence @rpath/QtWebEngine…, que macdeployqt ne résout pas chez Homebrew, et
# le processus auxiliaire exige Helpers/ et Resources/ complets : frameworks copiés entiers, en-têtes retirés.
copier_framework() { # <formule Homebrew> <nom du framework>
  local source="$qt/lib/$2.framework"
  [ -d "$source" ] || source="$(brew --prefix "$1" 2>/dev/null)/lib/$2.framework"
  [ -d "$source" ] || echec "$2.framework introuvable (ni $qt/lib, ni formule $1)"
  rm -rf "$app/Contents/Frameworks/$2.framework"
  # -H : chez Homebrew, $qt/lib/<nom>.framework est un lien ; c'est le framework qu'il faut copier.
  cp -RH "$source" "$app/Contents/Frameworks/" || echec "copie de $2.framework"
  chmod -R u+w "$app/Contents/Frameworks/$2.framework"
  rm -rf "$app/Contents/Frameworks/$2.framework/Headers" "$app/Contents/Frameworks/$2.framework/Versions/A/Headers"
}
copier_framework qtwebengine QtWebEngineCore
copier_framework qtwebengine QtWebEngineQuick
copier_framework qtwebengine QtWebEngineQuickDelegatesQml
copier_framework qtwebchannel QtWebChannel
copier_framework qtwebchannel QtWebChannelQuick
copier_framework qtpositioning QtPositioning

# Qt officiel (aqt) : binaires universels (x86_64 et arm64). L'image ne vise que les Mac Apple Silicon : chaque Mach-O
# universel est réduit à arm64 (moitié du poids), et otool -L n'écrit plus qu'une section par fichier, comme les
# analyses plus bas l'attendent (sinon « fichier (architecture x86_64): » rend leurs listes aveugles).
while IFS= read -r -d '' fichier; do
  archs="$(lipo -archs "$fichier" 2>/dev/null)" || continue
  [ "$archs" = arm64 ] && continue
  case " $archs " in
    *" arm64 "*) ;;
    *) echec "binaire sans arm64 : ${fichier#"$app"/} ($archs)" ;;
  esac
  chmod u+w "$fichier"
  lipo -thin arm64 "$fichier" -output "$fichier" || echec "réduction à arm64 : ${fichier#"$app"/}"
done < <(find "$app" -type f -print0)

# macdeployqt copie en entier les dossiers d'import de Qt (QtQuick/, QtQml/) : il y entraîne des modules qu'ActCraft
# n'importe pas, dont deux sous GPL seule (Qt Virtual Keyboard, Qt Quick Timeline : « Jamais » de qt-autorises.txt),
# Qt 3D (Scene2D, Scene3D) et State Machine. Chaque module part avec les greffons que ses liens désignent
# (PlugIns/quick), le clavier virtuel avec son greffon de saisie ; leurs frameworks, plus référencés, partent dans la
# boucle suivante. Un module oublié ici ne passe pas : verifier-licences-mac.sh refuse tout framework absent de
# qt-autorises.txt.
qml="$app/Contents/Resources/qml"
for module in QtQuick/VirtualKeyboard QtQuick/Timeline QtQuick/Scene2D QtQuick/Scene3D QtQml/StateMachine QtWebView; do
  [ -d "$qml/$module" ] || continue
  find "$qml/$module" -type l -name '*.dylib' | while read -r lien; do
    rm -f "$(dirname "$lien")/$(readlink "$lien")"
  done
  rm -rf "${qml:?}/$module"
  echo "Retiré, non importé : $module"
done
rm -f "$app/Contents/PlugIns/platforminputcontexts/libqtvirtualkeyboardplugin.dylib"
rm -rf "$app/Contents/PlugIns/webview" # Qt WebView : Android seulement (cadrage n° 8)
# Greffon FFmpeg de Qt Multimedia (Qt officiel) : il embarque les bibliothèques de FFmpeg, décodeur H.264 compris, que la
# release ne redistribue pas sur Mac (décision de Jérôme : release sans H.264). Le greffon darwin reste (AVFoundation,
# codecs du système), comme avec Qt Homebrew, qui n'a que lui ; les bibliothèques de FFmpeg, plus référencées, partent
# dans la boucle plus bas.
rm -f "$app/Contents/PlugIns/multimedia/libffmpegmediaplugin.dylib"
[ -f "$app/Contents/PlugIns/multimedia/libdarwinmediaplugin.dylib" ] ||
  echec "greffon multimédia darwin absent du bundle (PlugIns/multimedia)"
# Greffon NMEA de Qt Positioning (récepteur GPS série) : il lie Qt Serial Port, hors de qt-autorises.txt, que la boucle
# plus bas retire une fois ce greffon parti. Qt Positioning reste : Qt WebEngine le lie.
rm -f "$app/Contents/PlugIns/position/libqtposition_nmea.dylib"

# Références @rpath laissées par macdeployqt (voir plus haut) : réécrites sous la forme des siennes
# (@executable_path/../Frameworks/…) quand le framework est dans le bundle ; les autres restent, refusées plus bas.
# install_name_tool casse la signature du fichier : la signature ad hoc finale la refait.
find "$app" -type f -exec otool -L {} + 2>/dev/null |
  awk '/^[^[:space:]].*:$/ { f = substr($0, 1, length($0) - 1); next } /^[[:space:]]+@rpath\// { print f "\t" $1 }' |
  while IFS=$'\t' read -r fichier ref; do
    cible="${ref#@rpath/}"
    [ -e "$app/Contents/Frameworks/$cible" ] || continue
    install_name_tool -change "$ref" "@executable_path/../Frameworks/$cible" "$fichier" 2>/dev/null &&
      echo "Réécrit : ${fichier#"$app"/} → Frameworks/${cible%%/*}"
  done

# Chemins absolus de Homebrew dans les frameworks copiés à la main (QtWebEngineProcess, QtWebEngineCore, greffons) :
# même forme que les autres. L'auxiliaire tourne ailleurs : @executable_path/../Frameworks y désigne un dossier qui
# n'existe pas ; un lien l'y fait pointer vers les frameworks du bundle (DA4).
find "$app" -type f -exec otool -L {} + 2>/dev/null |
  awk '/^[^[:space:]].*:$/ { f = substr($0, 1, length($0) - 1); next }
       /^[[:space:]]+\/opt\/homebrew\/opt\/[^\/]+\/lib\/[^\/]+\.framework\// { print f "\t" $1 }' |
  while IFS=$'\t' read -r fichier ref; do
    cible="${ref#*/lib/}"
    [ -e "$app/Contents/Frameworks/$cible" ] || continue
    install_name_tool -change "$ref" "@executable_path/../Frameworks/$cible" "$fichier" 2>/dev/null &&
      echo "Réécrit : ${fichier#"$app"/} → Frameworks/${cible%%/*}"
  done
auxiliaire="$app/Contents/Frameworks/QtWebEngineCore.framework/Versions/A/Helpers/QtWebEngineProcess.app"
[ -x "$auxiliaire/Contents/MacOS/QtWebEngineProcess" ] || echec "QtWebEngineProcess absent du bundle"
ln -s ../../../../../.. "$auxiliaire/Contents/Frameworks" || echec "lien Frameworks de l'auxiliaire"

# Tout framework et toute bibliothèque que plus rien ne référence s'en va, jusqu'à point fixe.
liens="$paquet/liens.txt"
while :; do
  # Tous les fichiers : un binaire de framework n'a ni extension ni droit d'exécution (-r--r--r-- chez Homebrew) ; un
  # fichier qui n'est pas Mach-O écrit « is not an object file », ligne sans « : » final, ignorée par awk.
  find "$app" -type f -exec otool -L {} + 2>/dev/null |
    awk '/^[^[:space:]].*:$/ { f = substr($0, 1, length($0) - 1); next } /^[[:space:]]/ { print f "\t" $1 }' > "$liens"
  # Liste non aveugle : le binaire de QtCore y est une source, sinon rien ne serait jugé référencé et tout serait effacé.
  awk -F'\t' '$1 ~ /\/QtCore\.framework\/Versions\/A\/QtCore$/ { t = 1 } END { exit !t }' "$liens" ||
    echec "liste des liens aveugle : binaire de QtCore absent de $liens"
  retire=0
  for cible in "$app"/Contents/Frameworks/*.framework "$app"/Contents/Frameworks/*.dylib; do
    [ -e "$cible" ] || continue
    nom="$(basename "$cible")"
    if ! awk -F'\t' -v c="$cible" -v n="/$nom" 'index($1, c) != 1 && index($2, n) { trouve = 1; exit } END { exit !trouve }' "$liens"; then
      rm -rf "$cible"
      echo "Retiré, plus référencé : $nom"
      retire=1
    fi
  done
  [ "$retire" = 1 ] || break
done
# La boucle ci-dessus retire ce que plus rien ne référence : le moteur doit y avoir survécu.
for fw in QtWebEngineCore QtWebEngineQuick QtWebEngineQuickDelegatesQml QtWebChannel QtWebChannelQuick QtPositioning; do
  [ -d "$app/Contents/Frameworks/$fw.framework" ] || echec "$fw.framework retiré du bundle"
done

# Identifiant (LC_ID_DYLIB) resté sur le chemin de Homebrew (44 frameworks de Qt 6.12 Homebrew, relevé du 2026-11-08) :
# remis sous la forme des autres. dyld ne charge rien par lui, mais otool -L le montre comme une ligne de dépendance.
find "$app/Contents/Frameworks" -type f | while read -r fichier; do
  id="$(otool -D "$fichier" 2>/dev/null | sed -n 2p)"
  case "$id" in
    /opt/homebrew/* | /usr/local/*)
      install_name_tool -id "@executable_path/../Frameworks/${fichier#"$app/Contents/Frameworks/"}" "$fichier" 2>/dev/null ||
        echo "Identifiant non réécrit : $fichier"
      ;;
  esac
done

# Tous les fichiers (binaires de framework sans extension ni droit x) ; les non Mach-O sont ignorés. Tout chemin absolu
# hors du système (/usr/lib, /System) est refusé : Homebrew, /usr/local, ou un client installé ailleurs (Postgres.app).
restes="$(find "$app" -type f -exec otool -L {} + 2>/dev/null |
  grep -E '^[[:space:]]+/' | grep -vE '^[[:space:]]+/(usr/lib|System)/' | sort -u)"
[ -z "$restes" ] || echec "dépendances hors du bundle : $(echo "$restes" | head -3 | tr -s '[:space:]' ' ')"
# Une référence @rpath que macdeployqt n'a pas réécrite doit trouver sa cible dans Contents/Frameworks : sinon dyld la
# cherche ailleurs (rpaths de Homebrew restés dans un greffon) ou ne la trouve pas.
manquants="$(find "$app" -type f -exec otool -L {} + 2>/dev/null |
  awk '/^[[:space:]]+@rpath\// { print substr($1, 8) }' | sort -u |
  while read -r cible; do [ -e "$app/Contents/Frameworks/$cible" ] || echo "$cible"; done)"
[ -z "$manquants" ] || echec "@rpath sans cible dans le bundle : $(echo "$manquants" | head -3 | tr '\n' ' ')"
[ "$(lipo -archs "$app/Contents/MacOS/actcraft")" = arm64 ] || echec "binaire autre que arm64"

# macdeployqt de Qt 6.12 Homebrew n'écrit pas qt.conf (relevé du 2026-11-08 : Contents/Resources ne contient que qml/,
# son journal ne le nomme pas) ; sans lui, Qt ne trouve pas les modules QML du bundle. Écrit s'il manque, avec les clés
# de Qt 6 (QmlImports ; Qml2Imports et Imports ne figurent pas parmi les chaînes de QtCore 6.12), puis contrôlé.
conf="$app/Contents/Resources/qt.conf"
if [ ! -f "$conf" ]; then
  printf '[Paths]\nPlugins = PlugIns\nQmlImports = Resources/qml\n' > "$conf"
  echo "qt.conf écrit (absent après macdeployqt)"
fi
grep -q '^Plugins = PlugIns$' "$conf" && grep -q '^QmlImports = Resources/qml$' "$conf" ||
  echec "qt.conf sans chemin des greffons ou des modules QML ($conf)"
cp LICENSE "$app/Contents/Resources/LICENSE.txt"
cp THIRD_PARTY_NOTICES.md "$app/Contents/Resources/"
cp -R LICENSES "$app/Contents/Resources/LICENSES"
# Texte de la licence de Chromium : celui de la formule quand Qt vient de Homebrew ; Qt installé par aqt n'a pas de
# formule, la copie exacte du dépôt (LICENSES/LICENSE.Chromium) le remplace, même si Homebrew a aussi qtwebengine.
licence_chromium="LICENSES/LICENSE.Chromium"
brew_qt="$(brew --prefix qt 2>/dev/null)"
if [ -n "$brew_qt" ] && [ "$qt" = "$brew_qt" ]; then
  licence_brew="$(brew --prefix qtwebengine 2>/dev/null)/LICENSE.Chromium"
  [ -f "$licence_brew" ] && licence_chromium="$licence_brew"
fi
cp "$licence_chromium" "$app/Contents/Resources/LICENSE.Chromium" || echec "copie de LICENSE.Chromium"

licences="$(scripts/verifier-licences-mac.sh "$app")"
code=$?
echo "$licences"
[ "$code" = 0 ] && [ "$(echo "$licences" | tail -1)" = "LICENCES OK" ] || echec "licences (voir ci-dessus)"

# Signature ad hoc, après toute écriture dans le bundle (elle scelle Contents/Resources).
codesign --force --deep -s - "$app" || echec "signature ad hoc"
codesign --verify --deep --strict "$app" || echec "signature ad hoc invalide"

ln -s /Applications "$stage/Applications"
cp LICENSE "$stage/LICENSE.txt"
cp THIRD_PARTY_NOTICES.md "$stage/"
cat > "$stage/LISEZ-MOI.txt" <<'TEXTE'
ActCraft pour macOS (Mac Apple Silicon)

Installer : glisser ActCraft dans le dossier Applications.

Cette version n'est pas signée par un certificat de développeur Apple. Au premier lancement, macOS la bloque :
- macOS 14 et avant : clic droit (ou Contrôle + clic) sur ActCraft, « Ouvrir », puis « Ouvrir » encore ;
- macOS 15 et après : ouvrir ActCraft une fois, puis Réglages Système > Confidentialité et sécurité,
  « Ouvrir quand même ».
Autre manière, dans le Terminal : xattr -dr com.apple.quarantine /Applications/ActCraft.app
(retire la marque que macOS pose sur les fichiers téléchargés).
TEXTE

dmg="$paquet/ActCraft-$version-macos-arm64.dmg"
rm -f "$dmg"
hdiutil create -volname "ActCraft $version" -srcfolder "$stage" -format UDZO -ov "$dmg" > /dev/null ||
  echec "hdiutil create"
hdiutil verify "$dmg" > /dev/null 2>&1 || echec "hdiutil verify"
echo "Taille : $(du -h "$dmg" | cut -f1)"
echo "SHA-256 : $(shasum -a 256 "$dmg" | cut -d' ' -f1)"
echo "DMG OK : $dmg"
