#!/usr/bin/env bash
# Essai de scripts/verifier-minos-mac.sh sur de petits bundles construits ici : un vrai Mach-O (clang) dont le minos est
# fixé par -mmacosx-version-min, un Info.plist avec ou sans LSMinimumSystemVersion.
# Dernière ligne et code : « MINOS ESSAI OK » (0) ou « MINOS ESSAI EN ECHEC : <raison> » (1).
set -u
racine="$(cd "$(dirname "$0")/.." && pwd)"
verif="$racine/scripts/verifier-minos-mac.sh"
echec() {
  echo "MINOS ESSAI EN ECHEC : $1"
  exit 1
}
tmp="$(mktemp -d /tmp/actcraft-minos.XXXXXX)" || echec "dossier temporaire"
trap 'rm -rf "$tmp"' EXIT
printf 'int main(void) { return 0; }\n' > "$tmp/main.c"

# bundle <nom> <minimum déclaré ou -> <minos de l'exécutable> <minos d'une bibliothèque ou ->
bundle() {
  local app="$tmp/$1.app"
  mkdir -p "$app/Contents/MacOS" "$app/Contents/Frameworks"
  {
    echo '<?xml version="1.0" encoding="UTF-8"?><plist version="1.0"><dict>'
    echo '<key>CFBundleName</key><string>Essai</string>'
    [ "$2" = - ] || echo "<key>LSMinimumSystemVersion</key><string>$2</string>"
    echo '</dict></plist>'
  } > "$app/Contents/Info.plist"
  clang -mmacosx-version-min="$3" "$tmp/main.c" -o "$app/Contents/MacOS/essai" || echec "clang ($1)"
  if [ "$4" != - ]; then
    clang -dynamiclib -mmacosx-version-min="$4" "$tmp/main.c" -o "$app/Contents/Frameworks/libessai.dylib" 2> /dev/null ||
      clang -dynamiclib -mmacosx-version-min="$4" -x c - -o "$app/Contents/Frameworks/libessai.dylib" <<< 'int f(void){return 1;}' ||
      echec "clang dylib ($1)"
  fi
}
# universel <nom> <minos arm64> <minos x86_64> : bibliothèque à deux tranches, dans un bundle à minimum déclaré 14.4.
universel() {
  bundle "$1" 14.4 14.4 -
  clang -dynamiclib -target "arm64-apple-macos$2" -x c - -o "$tmp/a.dylib" <<< 'int f(void){return 1;}' &&
    clang -dynamiclib -target "x86_64-apple-macos$3" -x c - -o "$tmp/x.dylib" <<< 'int f(void){return 1;}' &&
    lipo -create "$tmp/a.dylib" "$tmp/x.dylib" -output "$tmp/$1.app/Contents/Frameworks/libuniv.dylib" || echec "universel ($1)"
}
# attendu <nom> <code> <texte de la dernière ligne>
attendu() {
  local sortie code
  sortie="$("$verif" "$tmp/$1.app")"
  code=$?
  [ "$code" = "$2" ] || echec "$1 : code $code, attendu $2 ($sortie)"
  echo "$sortie" | tail -1 | grep -q -- "$3" || echec "$1 : dernière ligne « $(echo "$sortie" | tail -1) », attendu « $3 »"
}

bundle ok 14.4 14.4 14.4
bundle exact 14.4 14.4 -
bundle trop-recent 14.4 14.4 27.0
bundle sans-cle - 14.4 14.4
bundle exe-trop-recent 14.4 27.0 -
bundle declare-plus 27.0 14.4 27.0
universel univ-x86-recent 14.4 27.0
universel univ-arm64-recent 27.0 14.4
attendu univ-x86-recent 0 '^MINOS OK'
attendu univ-arm64-recent 1 'MINOS EN ECHEC : .*libuniv.dylib.*27.0'
attendu ok 0 '^MINOS OK'
attendu exact 0 '^MINOS OK'
attendu declare-plus 0 '^MINOS OK'
attendu trop-recent 1 'MINOS EN ECHEC : .*14.4.*libessai.dylib.*27.0'
attendu sans-cle 1 'MINOS EN ECHEC : LSMinimumSystemVersion absent'
attendu exe-trop-recent 1 'MINOS EN ECHEC : .*14.4.*MacOS/essai.*27.0'
"$verif" "$tmp/inexistant.app" | tail -1 | grep -q 'MINOS EN ECHEC' || echec "bundle absent non refusé"
echo "MINOS ESSAI OK"
