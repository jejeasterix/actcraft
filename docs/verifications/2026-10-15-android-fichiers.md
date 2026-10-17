# Android : Ouvrir et Enregistrer par content:// — vérification du 2026-10-16

Tâche L2-T22 du chantier `livraison-complete` (décision 17, scénario S5 du cadrage
`docs/specs/2026-10-14-livraison-complete-cadrage.md`). Voie principale prouvée : le repli (commandes de fichier
retirées de l'APK) n'est pas appliqué, `actions/data/commands.json` est inchangé.

## Résultat

VERIF s5-emulateur = AVD actcraft-x86_64, image system-images;android-34;google_apis;x86_64, API 34, écran 2560 × 1600, lancé par le script sans fenêtre (`-no-window -gpu host -no-snapshot`, port 5584, WHPX)
VERIF s5-apk = `pwsh -NoProfile -File ./scripts/paquet-android.ps1 -Abi x86_64` → APK OK (build/paquet/ActCraft-0.1.0-x86_64.apk, clé de débogage locale)
VERIF s5-commande = `pwsh -NoProfile -File ./scripts/essai-android-fichiers.ps1 -Apk build/paquet/ActCraft-0.1.0-x86_64.apk`
VERIF s5-enregistre = `10-16 22:09:19.684  6138  6165 I actcraft.document: ACTCRAFT ENREGISTRE ba4f750efe51eb50` (« Enregistrer sous » par Ctrl+Maj+S, bouton « SAVE » du sélecteur DocumentsUI, `android:id/button1`)
VERIF s5-ouvert = `10-16 22:09:30.626  6138  6165 I actcraft.document: ACTCRAFT OUVERT ba4f750efe51eb50` (Ctrl+W, Ctrl+O, « Sans titre.actcraft » touché dans le sélecteur ; URL du fournisseur `content://com.android.providers.downloads.documents/document/13`)
VERIF s5-verdict = ANDROID FICHIERS OK (empreinte enregistrée = empreinte relue ; trait au doigt contrôlé sur la capture avant l'enregistrement, pixel lu 0 0 0 ; aucun FATAL EXCEPTION dans logcat)
VERIF s5-mutation = journal OUVERT faussé d'un bit (ui/src/documentcontroller.cpp, `fingerprint ^ 1`), APK reconstruit : `ANDROID FICHIERS EN ECHEC : empreintes différentes : a6c1e66f17d9f0a3 enregistrée, a6c1e66f17d9f0a2 relue` ; fichier remis par git checkout --, APK reconstruit propre
VERIF s5-final = APK propre reconstruit après la mutation, script final (nettoyage par `find /sdcard/`) : `ACTCRAFT ENREGISTRE a3277271775faa91`, `ACTCRAFT OUVERT a3277271775faa91`, ANDROID FICHIERS OK
VERIF s5-trois-passages = 2026-10-17, APK reconstruit après L2-C6 (LICENCES OK sans ajout), script robuste (nom propre à l'essai, critères indépendants de la langue, indication « plein écran » écartée, serveur adb arrêté par PID) : passages consécutifs `essai-20261017-023343.actcraft` d168020cda87c217 = d168020cda87c217, `essai-20261017-023710.actcraft` 285cad712988f85e = 285cad712988f85e, `essai-20261017-024148.actcraft` 285cad712988f85e = 285cad712988f85e : trois ANDROID FICHIERS OK
VERIF s3-module = essai-android.ps1 passe par scripts/android-commun.ps1 ; sous un adb bouchon qui ne répond pas, dernière ligne identique avant et après l'extraction : `ANDROID EN ECHEC : adb get-state sans réponse (15 s)`
VERIF s5-patron-ecarte = sous les mêmes bouchons (adb et emulator), essai-android-fichiers.ps1 finit par `ANDROID FICHIERS EN ECHEC : adb get-state sans réponse (15 s)` et l'émulateur bouchon est arrêté ; avec l'`Echec` d'essai-android.ps1 recopié dans le module (patron écarté), la dernière ligne devient `ANDROID EN ECHEC : …` et l'émulateur bouchon reste vivant

## Écueils rencontrés (trois essais)

1. `adb install` refusé : la clé de débogage est propre à chaque dossier de construction
   (`build/android-debug.keystore`, scripts/paquet-android.ps1:53) ; un APK installé depuis un autre worktree a une
   autre signature. Le script désinstalle d'abord le paquet (émulateur sans donnée réelle).
2. Serveur adb démarré par le premier appel borné : il héritait de la sortie redirigée (`> fichier` resté ouvert
   après la fin du script) et du dossier courant (worktree verrouillé). Le script le démarre à part
   (`adb start-server`, dossier temporaire, sans handle hérité).
3. `uiautomator dump` sans fichier tant que la fenêtre de l'application s'anime : nouvel essai jusqu'au délai.
4. Rejeu de l'orchestrateur (2026-10-17) : `bouton d'enregistrement du sélecteur introuvable`. L'arbre et la capture
   montrent l'indication « plein écran » du système (`android:id/immersive_cling_title`, paquet `android`) au premier
   plein écran de l'application réinstallée : elle prend le raccourci, aucun sélecteur ne s'ouvre. Le script confirme
   l'indication d'avance (`settings put secure immersive_mode_confirmations confirmed`), l'écarte par son identifiant
   si elle paraît et renvoie le raccourci (une fois de plus si aucune fenêtre de DocumentsUI n'est venue en 20 s).
   Critères indépendants de la langue : paquet DocumentsUI et `android:id/button1` pour enregistrer,
   `android:id/title` et l'extension `.actcraft` pour ouvrir. En échec, l'arbre et la capture restent sous `.tmp/`
   (`fichiers-enregistrer.xml|png`, `fichiers-ouvrir.xml|png`), chemins dans la ligne d'échec. Le serveur adb démarré
   par l'essai (port 5037 libre avant) est arrêté par son PID.
5. Paperboards des essais précédents toujours listés par le fournisseur des téléchargements malgré `find -delete`
   (« Sans titre (1).actcraft »…) : un passage a fini sans ligne ENREGISTRE après « SAVE ». Le script saisit un nom
   propre à l'essai (`essai-<date>.actcraft`, Ctrl+A puis `input text`), retouche « SAVE » tant que le sélecteur reste
   ouvert (trois fois au plus) et rouvre ce nom exact. Glissement rejoué (trois fois au plus) s'il tombe pendant le
   passage en plein écran. À tout échec : journal et capture sous `.tmp/fichiers-echec-*`.

## Hors périmètre, constaté

- `packaging/qt-autorises.txt` ne contient pas `Qt6QuickControls2FluentWinUI3` (seulement sa `StyleImpl`) :
  `paquet-android.ps1` finit par `LICENCES EN ECHEC : Qt6QuickControls2FluentWinUI3` sur le code actuel. L'APK de
  cet essai a été construit avec la ligne ajoutée le temps de la construction, puis la liste remise
  (`git checkout --`) : la correction revient à une autre tâche.
- Gradle (JDK 21) : `Unable to establish loopback connection` (`SocketException: Invalid argument: connect` dans
  `sun.nio.ch.PipeImpl`, socket de domaine Unix) quand le dossier temporaire de Windows est encombré (88 066
  entrées le 2026-10-16) ; contourné par `JAVA_TOOL_OPTIONS=-Djdk.net.unixdomain.tmpdir=<dossier vide sous .tmp>`.
