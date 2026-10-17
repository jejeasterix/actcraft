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
VERIF s5-mutation-2 = même mutation sur le script robuste (2026-10-17) : `ANDROID FICHIERS EN ECHEC : empreintes différentes : 285cad712988f85e enregistrée, 285cad712988f85f relue` ; fichier remis par git checkout --, APK reconstruit propre
VERIF s5-mutation-3 = même mutation sur le script final (2be9c25, 2026-10-17) : `ANDROID FICHIERS EN ECHEC : empreintes différentes : 380221149fbb1b81 enregistrée, 380221149fbb1b80 relue` ; fichier remis par git checkout --, APK propre reconstruit avant les cinq passages
VERIF s5-cinq-passages = 2026-10-17, script final 2be9c25, même APK propre (SHA-256 BA76C9277916AE12…, identique avant et après la mutation), chaque passage sous le jeton `tests`, machine partagée : 285cad712988f85e = 285cad712988f85e, 285cad712988f85e = 285cad712988f85e, 69b71dec81c2416f = 69b71dec81c2416f, 8c1d213facef8e5f = 8c1d213facef8e5f, 28710ede3f19e88c = 28710ede3f19e88c : cinq ANDROID FICHIERS OK consécutifs (nom saisi une fois, relu deux fois)
VERIF s5-revue-a-sec = 2026-10-17, script 0d0cb3a (constats de la revue), adb et emulator remplacés par des bouchons hors dépôt, chaque cas joué aussi sur la version d'avant (2f99d02) : Ctrl+C simulé (arrêt du pipeline) → le bloc finally arrête l'émulateur, son qemu et le serveur adb lancé par l'essai (avant : les trois restent vivants) ; noms piégés (`essai-x';rm -rf /sdcard/Download;'.actcraft`, `notes-cours.actcraft`, `e`, `essai`, sous-dossier, Android/data, chiffres non ASCII) → seuls `essai-AAAAMMJJ-HHMMSS.actcraft` de Download et Documents effacés, aucun `rm` du nom piégé (avant : 11 `rm` dont le piégé, 5 `content delete`) ; serveur adb créé par une autre session pendant le `start-server` → laissé vivant (avant : arrêté) ; serveur déjà là → laissé ; port 5584 écouté ou emulator-5584 connu d'adb → refus net ; qemu orphelin d'un émulateur arrêté au démarrage → arrêté (avant : vivant 600 s) ; client `start-server` muet → arrêté (avant : vivant) ; deux empreintes illisibles → `empreintes différentes : (vide) enregistrée, (vide) relue` (avant : OK) ; marqueur en minuscules → aucune ligne (avant : ligne prise)
VERIF s5-revue-ci = essai-android.ps1 sous adb bouchon muet : `ANDROID EN ECHEC : adb get-state sans réponse (15 s)`, code 1 (inchangé) ; `logcat -c` refusé : `ANDROID EN ECHEC : adb logcat -c (code 1)`
VERIF s5-revue-arret-reel = sur l'émulateur réel, descendants arrêtés par l'essai : `qemu-system-x86_64-headless`, `netsimd`, `crashpad_handler` ; serveur adb identifié comme lancé par l'essai et arrêté ; aucun processus Android créé pendant un passage n'est resté vivant
VERIF s5-revue-trois-passages = 2026-10-17, script 0d0cb3a, APK existant non reconstruit (SHA-256 D8C5678C0B788F69…), lancé depuis la racine du worktree, chaque passage sous le jeton `tests`, machine partagée (0,9 à 4,5 Go libres) : 24523d26c6dc58ef = 24523d26c6dc58ef, 18c072cf06239114 = 18c072cf06239114, 9a6e8411c2857e6e = 9a6e8411c2857e6e : trois ANDROID FICHIERS OK consécutifs
VERIF s5-revue-chemins = sorties absolues sous le `.tmp/` de la racine du script (calculée depuis `$PSScriptRoot`), APK relatif résolu depuis le dossier d'appel : `adb pull` écrivait les chemins relatifs dans le dossier courant du processus, que `Set-Location` ne change pas (premier rejeu de la revue lancé hors de la racine : captures écrites dans le `.tmp` du dépôt principal, `trait absent après trois glissements`). À sec, lancé depuis un dossier vide qui a son `.tmp` : aucun fichier hors du `.tmp` du worktree, trait lu, essai poursuivi jusqu'au sélecteur ; version 0d0cb3a : `fichiers-trait.png` et `fichiers-echec.png` écrits dans le dossier d'appel, `trait absent après trois glissements`. Sortie d'erreur d'adb recueillie par l'appel borné : `ANDROID EN ECHEC : adb install : adb: failed to install stub.apk: Failure [STUB_REFUS]` (bouchon) ; dernière ligne sous adb muet inchangée. Passages réels rejoués par l'orchestrateur
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

6. Rejeu de l'orchestrateur (2026-10-17, 3 passages, 1 OK) : `ouvrir : nœud ^android:id/title$ introuvable en 120 s`.
   L'arbre montre deux fichiers « ess » et « essai-20261017-033111.actcr » : noms réellement tronqués (la description
   de l'aperçu reprend le même nom), pas une ellipse de la vue grille (les passages OK étaient aussi en grille, titres
   en `android:id/title`). « SAVE » était touché avant que la saisie (`input text`, qui passe par le clavier du système)
   ait rempli le champ. Ces noms, sans extension, échappaient au nettoyage `find -name '*.actcraft'` et restaient
   listés. Le script : (a) ressaisit le nom (champ touché, Ctrl+A, `input text`) tant que l'arbre ne le relit pas
   exact deux fois de suite, attend s'il se remplit encore (180 s au plus) ; (b) vérifie après l'enregistrement que
   MediaStore contient ce seul fichier d'essai, sous ce nom ; (c) ouvre par le texte exact, quel que soit
   l'identifiant du nœud (vue liste ou grille) ; (d) efface au départ les fichiers d'essai par MediaStore
   (`content delete --uri content://media/external/file/<id>`, base d'où le fournisseur des téléchargements tire sa
   liste), puis du stockage s'il en reste, et vérifie les deux, sinon échec. Fichier d'essai : extension `.actcraft`,
   nom `essai-…` ou début de ce nom. MediaStore plutôt qu'un AVD effacé (`-wipe-data`) : la voie est vérifiable à
   chaque passage, et un premier démarrage à neuf est plus long et plus chargé.
7. Mise au point (2026-10-17, 0,4 Go libres) : boîte « System UI isn't responding » sur le sélecteur pendant la
   relecture du nom ; chaque lecture de l'arbre l'écarte par `android:id/aerr_wait`. Passage de la mutation : émulateur
   figé plus d'une minute (ANR de gms et du lanceur), `adb shell screencap sans réponse (60 s)` avant le trait ; dans
   les attentes bornées (arbre, trait, journal, MediaStore), un adb sans réponse compte comme un tour manqué.

## Hors périmètre, constaté

- Rejeu de la revue (2026-10-17, machine partagée, 2 Go libres) : un passage a fini par `adb install` (binder en
  échec dans logcat, `Performing Streamed Install` sans suite), sans détail : la sortie d'erreur d'adb n'était pas
  recueillie (corrigé ensuite, VERIF s5-revue-chemins).

- `packaging/qt-autorises.txt` ne contient pas `Qt6QuickControls2FluentWinUI3` (seulement sa `StyleImpl`) :
  `paquet-android.ps1` finit par `LICENCES EN ECHEC : Qt6QuickControls2FluentWinUI3` sur le code actuel. L'APK de
  cet essai a été construit avec la ligne ajoutée le temps de la construction, puis la liste remise
  (`git checkout --`) : la correction revient à une autre tâche.
- Gradle (JDK 21) : `Unable to establish loopback connection` (`SocketException: Invalid argument: connect` dans
  `sun.nio.ch.PipeImpl`, socket de domaine Unix) quand le dossier temporaire de Windows est encombré (88 066
  entrées le 2026-10-16) ; contourné par `JAVA_TOOL_OPTIONS=-Djdk.net.unixdomain.tmpdir=<dossier vide sous .tmp>`.
