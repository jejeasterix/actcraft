# Banc de fluidité sur une tablette Android — procédure pas à pas

Objectif : mesurer ActCraft sur la vraie tablette (budget du BRIEF § 3 : 60 images/s, latence d'encre < 20 ms).
L'APK de banc est **séparé** de l'application (identifiant `io.github.jejeasterix.actcraft.banc`), construit en
release, sans aucune permission. Il n'entre ni dans `build/paquet/`, ni dans la release. Mode d'emploi du banc de
bureau : `docs/banc.md`.

Une mesure faite sur l'émulateur est **indicative** (le script l'écrit : `MESURE INDICATIVE (émulateur)`) : elle ne
vaut jamais mesure de tablette.

## Avant tout passage

- Tablette chargée (au moins 50 %), écran allumé, mise en veille désactivée (Paramètres ▸ Affichage).
- Fermer les autres applications de la tablette.
- Ne pas toucher à la tablette pendant la mesure (≈ 3 min en `complet`) : un contact rend le passage invalide.

## Procédure

1. Sur la tablette : Paramètres ▸ À propos ▸ appuyer 7 fois sur « Numéro de build » ; puis Options pour les
   développeurs ▸ activer « Débogage USB ».
2. Brancher la tablette au poste par USB. Sur la tablette, accepter l'empreinte du poste (« Toujours autoriser »).
3. Dans `pwsh -NoProfile`, à la racine du dépôt :
   ```powershell
   . ./scripts/dev-env-android.ps1
   adb devices
   ```
   Vous devez voir une ligne qui finit par `device` (pas `unauthorized`, pas `offline`). Notez son numéro de série
   (première colonne).
4. Construire l'APK de banc (plusieurs minutes la première fois) :
   ```powershell
   pwsh -NoProfile -File ./scripts/paquet-banc-android.ps1 -Abi arm64-v8a
   ```
   Dernière ligne attendue : `APK BANC OK`. Le fichier est `build/paquet-banc/ActCraft-banc-<version>-arm64-v8a.apk`.
5. Lancer la mesure (remplacer `<version>` et `<numéro>`) :
   ```powershell
   pwsh -NoProfile -File ./scripts/banc-android.ps1 -Apk build/paquet-banc/ActCraft-banc-<version>-arm64-v8a.apk -Serie <numéro> -Scenario complet -Sortie .tmp/banc-tablette
   ```
   Le script installe le banc, le lance, attend sa fin (≈ 3 min), rapatrie le JSON, puis désinstalle le banc.
   Laisser la tablette posée, sans la toucher.
6. Ce que vous devez voir à la fin : une ligne `JSON : …` par fichier rapatrié, puis une dernière ligne
   `BUDGET TENU` ou `BUDGET DEPASSE`. Les deux sont des résultats valables. `MESURE INVALIDE` ou
   `BANC ANDROID EN ECHEC : <raison>` : refaire le passage une fois ; si cela se répète, envoyer la ligne.
7. Envoyer à l'orchestrateur : la dernière ligne affichée, le fichier `.tmp/banc-tablette/*/*.json` (dossier
   `bench-results`), `.tmp/banc-tablette/logcat.txt` et le modèle de la tablette (Paramètres ▸ À propos).

Pour une tablette x86_64 ou un émulateur, remplacer `arm64-v8a` par `x86_64`.

## Si ça ne marche pas

| Constat | Cause probable |
|---|---|
| `adb devices` vide ou `unauthorized` | câble de charge seul, ou empreinte non acceptée sur la tablette |
| `adb introuvable dans le PATH` | oubli de `. ./scripts/dev-env-android.ps1` dans cette fenêtre |
| `aucun JSON rapatrié` | le banc s'est arrêté avant d'écrire : joindre `logcat.txt` |
| `banc absent 10 s après le lancement` | le banc a planté au démarrage : joindre `logcat.txt` |
