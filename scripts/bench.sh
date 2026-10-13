#!/usr/bin/env bash
# Banc de fluidité sous macOS : construit en release, puis joue tracé, zoom et défilement.
# Usage : scripts/bench.sh [--rendu auto|logiciel|tous] [--sans-cache] [--captures DOSSIER]
#                          [--scenario complet|court] [--taille plein-ecran|LxH] [--hors-ecran] [--sans-construire]
# Prérequis : ACTCRAFT_QT_DIR (dossier de Qt 6.12, par ex. ~/Qt/6.12.0/macos), CMake, Ninja, outils Xcode.
# Dernière ligne et code de sortie : verdict de « auto » (Metal) — BUDGET TENU (0), BUDGET DEPASSE (1),
# MESURE INVALIDE (2). Compatible bash 3.2 : ni tableaux associatifs ni ${x,,}.
set -u
cd "$(dirname "$0")/.." || exit 2

rendu=tous
sans_cache=0
captures=""
scenario=complet
taille=plein-ecran
hors_ecran=0
construire=1
while [ $# -gt 0 ]; do
  case "$1" in
    --rendu) rendu="$2"; shift 2 ;;
    --sans-cache) sans_cache=1; shift ;;
    --captures) captures="$2"; shift 2 ;;
    --scenario) scenario="$2"; shift 2 ;;
    --taille) taille="$2"; shift 2 ;;
    --hors-ecran) hors_ecran=1; shift ;;
    --sans-construire) construire=0; shift ;;
    *) echo "option inconnue : $1" >&2; echo "MESURE INVALIDE"; exit 2 ;;
  esac
done
case "$rendu" in
  auto|logiciel|tous) ;;
  *) echo "--rendu : auto, logiciel ou tous" >&2; echo "MESURE INVALIDE"; exit 2 ;;
esac

if [ "$construire" = 1 ]; then
  if [ -z "${ACTCRAFT_QT_DIR:-}" ]; then
    echo "ACTCRAFT_QT_DIR non défini (dossier de Qt 6.12)" >&2; echo "MESURE INVALIDE"; exit 2
  fi
  if ! cmake --preset release || ! cmake --build --preset release; then
    echo "MESURE INVALIDE (compilation)"; exit 2
  fi
fi

exe="build/release/actcraft-bench"
if [ ! -x "$exe" ]; then echo "MESURE INVALIDE ($exe introuvable)"; exit 2; fi
mkdir -p bench/results
if [ "$hors_ecran" = 1 ]; then
  export QT_QPA_PLATFORM=offscreen
elif [ "$taille" = plein-ecran ]; then
  echo "La fenêtre du banc couvre l'écran principal pendant 30 à 60 s par rendu."
fi

if [ "$rendu" = tous ]; then rendus="auto logiciel"; else rendus="$rendu"; fi
code_auto=""
dernier=2
for r in $rendus; do
  set -- --rendu "$r" --scenario "$scenario" --taille "$taille" --sortie "bench/results/"
  if [ "$sans_cache" = 1 ]; then set -- "$@" --sans-cache; fi
  if [ -n "$captures" ]; then mkdir -p "$captures"; set -- "$@" --captures "$captures"; fi
  essai=1
  while :; do
    "$exe" "$@"
    code=$?
    if [ "$code" != 2 ] || [ "$essai" = 2 ]; then break; fi
    echo "Passage $r invalide : nouvel essai."
    essai=2
  done
  dernier=$code
  if [ "$r" = auto ]; then code_auto=$code; fi
done

final=${code_auto:-$dernier}
case "$final" in
  0) echo "BUDGET TENU" ;;
  1) echo "BUDGET DEPASSE" ;;
  *) echo "MESURE INVALIDE"; final=2 ;;
esac
exit "$final"
