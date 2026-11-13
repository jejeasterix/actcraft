#!/usr/bin/env bash
# Tests au label ctest « materiel » sur un Mac réel : ceux qui exigent une vraie machine (sortie audio de
# tst_media_player, cadence d'encodage de tst_media_displayrecorder, toucher sur la fenêtre du clavier de
# tst_ui_keyboard) et que le job macOS de la CI, sur une machine de GitHub, exclut (-LE materiel). Porte avant
# chaque release (docs/tests-materiel.md). Preset debug, Qt de ACTCRAFT_QT_DIR ou, à défaut, de Homebrew ; seuls les
# exécutables de ces tests sont compilés (un test ctest peut viser un exécutable commun : tst_ui_keyboard → tst_ui_lot7).
# Usage : scripts/tests-materiel.sh
# Dernière ligne et code : « TESTS MATERIEL OK : <n> test(s) » (0) ou « TESTS MATERIEL EN ECHEC : <raison> » (1).
set -u
cd "$(dirname "$0")/.." || exit 1
echec() {
  echo "TESTS MATERIEL EN ECHEC : $1"
  exit 1
}
[ "$(uname -s)" = Darwin ] || echec "Mac requis (uname -s = $(uname -s))"
export ACTCRAFT_QT_DIR="${ACTCRAFT_QT_DIR:-$(brew --prefix qt 2>/dev/null)}"
[ -n "$ACTCRAFT_QT_DIR" ] || echec "Qt introuvable : ACTCRAFT_QT_DIR ou brew install qt"
# La sortie prouve un commit : aucun fichier suivi modifié.
git diff --quiet HEAD -- || echec "arbre modifié : la sortie ne prouverait pas le commit $(git rev-parse --short HEAD)"
echo "Commit : $(git rev-parse HEAD) ; macOS $(sw_vers -productVersion) ; Qt : $ACTCRAFT_QT_DIR"
cmake --preset debug > /dev/null || echec "configuration CMake"
tests=$(ctest --preset debug -L materiel -N | sed -n 's/^ *Test *#[0-9]*: *\([^ ]*\).*/\1/p')
[ -n "$tests" ] || echec "aucun test au label materiel"
# Cibles : nom de fichier de l'exécutable de chaque test, lu dans `ctest -N -V`. Exécutable déjà construit : premier
# mot de « Test command: » ; pas encore construit : ctest l'écrit dans « Could not find executable <chemin> » juste
# avant, et « Test command: » ne porte plus que les arguments (le JSON de ctest n'a alors pas de commande du tout).
cibles=$(ctest --preset debug -L materiel -N -V 2>&1 | python3 -I -c '
import os, re, sys
manquant, cibles = None, set()
for ligne in sys.stdin:
    m = re.match(r"Could not find executable (\S+)", ligne)
    if m:
        manquant = m.group(1)
        continue
    m = re.match(r"\d+: Test command: *(.*)$", ligne)
    if m:
        cibles.add(os.path.basename(manquant or m.group(1).split()[0].strip("\"")))
        manquant = None
print(" ".join(sorted(cibles)))') || echec "commandes des tests illisibles (ctest -N -V)"
[ -n "$cibles" ] || echec "aucune cible pour les tests au label materiel"
echo "Tests : $(echo $tests) ; cibles : $cibles"
# shellcheck disable=SC2086 # $cibles : un nom de cible par exécutable
cmake --build --preset debug --target $cibles || echec "compilation de : $cibles"
ctest --preset debug -L materiel --output-on-failure || echec "ctest -L materiel (voir ci-dessus)"
echo "TESTS MATERIEL OK : $(echo "$tests" | wc -w | tr -d ' ') test(s)"
