#!/usr/bin/env bash
# Lit la version du projet (x.y.z) dans CMakeLists.txt ; à charger par « . scripts/version-projet.sh » depuis la racine.
# Pose `version` (vide si introuvable) ; l'appelant décide du message d'échec.
version="$(perl -0777 -ne 'print "$1.$2.$3" if /project\(\s*ActCraft\s+VERSION\s+(\d+)\.(\d+)\.(\d+)/' CMakeLists.txt)"
