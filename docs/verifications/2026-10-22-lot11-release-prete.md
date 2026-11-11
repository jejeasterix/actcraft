# Lot 11, L11-T29 : release prête (version Mac)

Relevé du 2026-11-08. La version du projet reste **0.6.0** (H11-O10, d'après H8-19) : partout où le plan attend « 1.0.0 », lire
0.6.0 ; le numéro 1.0.0 se pose à la release, avec le go. Aucune release, aucun tag, aucun push.

## Prouvé sur le Mac (paquet macOS, L11-T29m)

Source : `docs/verifications/2026-11-07-lot11-paquet-macos.md`.

- Fichier : `build/paquet/ActCraft-0.6.0-macos-arm64.dmg` (local, non commité, jamais publié), signature ad hoc, ni Developer ID
  ni notarisation.
- Taille : 61 Mo (64 328 601 octets). SHA-256 : `0d5594a82114a1cc91dfc4eb0f923856e804d88e09bd76099324c5a588d76cfb`.
- `LICENCES OK`, `DMG OK`, `VERSION OK : ActCraft 0.6.0`, `LANCEMENT OK` (fenêtre 3 s), `ESSAI DMG OK`.
- `tst_app_notices` vert ; suite complète du lot : `100% tests passed out of 278` en debug et en release
  (`docs/mesures/2026-10-22-lot11-fin.md`).
- Contrôle des notices à l'entrée du `.dmg` : toute bibliothèque absente de `packaging/*-autorises.txt` fait échouer
  `LICENCES EN ECHEC` (contre-épreuve jouée en L11-T29m).

## Restes pour le PC (points 1 à 5 du plan)

1. `scripts/paquet.ps1` : `PAQUET OK : …\build\paquet\ActCraft-<version>-setup.exe`, taille (N1-A10), `LICENCES OK` Windows ;
   contrôle du greffon TLS de Windows (`tls/qschannelbackend.dll`, `PAQUET EN ECHEC : greffon TLS absent`) : non fait ici
   (`scripts/paquet.ps1` non modifié par cette tâche).
2. `scripts/essai-installateur.ps1` : `INSTALLATEUR OK`, `--version`.
3. `scripts/paquet-android.ps1` : `APK OK`, `versionCode`/`versionName`, `verifier-licences.ps1 -Apk …` : `LICENCES OK`, tailles.
4. `apksigner verify --print-certs` sur l'APK : certificat de débogage.
5. Émulateur : `essai-android.ps1` (`ANDROID OK`) et `essai-android-fichiers.ps1` (`ANDROID FICHIERS OK`).

Aucun SDK Android ni Windows sur le Mac.

## Essai à blanc de `release.yml`

Non joué à la clôture du lot (reprise du 2026-11-08, minutes de CI comptées, aucune release demandée) : **à jouer avant tout tag**, sur l'exécuteur que prendra le tag (H11-18).
Motifs attendus : `APK NON PUBLIABLE|NOTES OK|SHA256SUMS|SECRETS ABSENTS|CONTROLE OK|DMG OK|ESSAI DMG OK` et les noms des paquets.
Points 7 et 8 du plan : aucune entrée de release `v…` (`gh release list`), branche d'essai supprimée.

## Interdits tenus

Aucun tag, aucune commande `gh release`, aucun `gh workflow run`, aucun secret lu, aucun fichier go, aucun paquet distribué.
