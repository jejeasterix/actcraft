# Livraison complète — lot 11 « Tablette, toucher, finitions, 1.0 », sous-lot 11b — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

Sous-lot **11b** : pages d'aide, contrôle des traductions, recherche de mises à jour, raccourcis restants,
accessibilité, licences tierces, S19 et S32 rejoués sur le produit complet, version 1.0.0, paquets, release prête et
porte de fin. Il suit le sous-lot 11a (`docs/plans/2026-10-18-livraison-complete-lot11-PLAN.md`), dont il reprend
**sans les recopier** : la liste de réservation R1 à R4 (les 32 tests sont déjà enregistrés par L11-T1), la table A des
API supposées, les décisions D11-1 à D11-12, les commandes de preuve communes (commande ciblée, fonctions d'un test,
preuve de tâche avec **N = N11 + 32**, preuve visuelle, banc sans écran, réglages d'essai en INI par lancement,
émulateur, mutations, `NOMS OK`, contrôle des titres du corpus, commit) et la règle de propriété intellectuelle.
L'exécutant de chaque tâche lit ces sections du plan de 11a avant la sienne.

**La release 1.0.0 n'a jamais lieu sans le go écrit de Jérôme.** Ce plan s'arrête à « release prête » (L11-T29) et à la
porte de fin (L11-T30). Aucune tâche ne crée de tag, ne pousse de tag, n'écrit dans une release GitHub ni ne crée le
fichier go (`<git-common-dir>/chantiers/go-action-finale`).

## Amendement 1 (relecture tour 1)

La table complète des constats est dans l'amendement 1 du plan de 11a ; elle fait autorité. Constats appliqués ici,
corrigés en place : **B1** (`tst_app_versioninfo`, L11-T28), **B2** (point 7 de L11-T29 supprimé, interdit du
`workflow_dispatch` réel), **M14** (`commands.lockedCount`, L11-T26), **M15** (`appUpdates.start()` et
`startCalledOnceByTheShell`, L11-T22), **M16** (`bench/scenarios/controle/pseudo-langue.json`, `ACTCRAFT_UPDATES_URL`
avant `tous`, L11-T19, L11-T20, L11-T30), **M8** (réglages d'essai des tests de 11b : règle commune de 11a),
**M10** (comptes de talons, L11-T30), et les mineurs :
- m7 : `tst_ui_accessibility` pose `ACTCRAFT_UPDATES_URL` dans `initTestCase` (L11-T24) ;
- m12 : `#include <actcraft/ui/preferences.h>` dans `updatechecker.h` (L11-T21) ;
- m13 : fixture `same.json` retirée (L11-T21) ;
- m14 : `ENVIRONMENT` qui garde `_bench_env_offscreen` (L11-T22) ;
- m15 : aucun nom de l'original en clair dans le test des pages d'aide ; la liste est lue hors dépôt (L11-T17) ;
- m16 : `app/CMakeLists.txt:26-35`, `ACTCRAFT_SOURCE_DIR`, `versionCodeFollowsTheVersion` (L11-T28) ;
- m17 : chemin du rapport des épinglages du lot 7 résolu par L11-T0 (L11-T27) ;
- m18 : L11-T25 s'arrête et rend la main si H6-6b n'est pas tranchée.

## Amendement 3 (exécution sur le Mac, 2026-11-07)

**Ordre d'autorité : amendement 3, puis 2, puis 1, puis le texte d'origine.** Corrections faites en place dans les
sections nommées ; le volet 2 (palette coupée à 1 000 px, L11-T10) est dans l'amendement 3 du plan de 11a.

**Volet 1 — Paquet `.dmg` non signé du Mac** (demande du pilote, 2026-11-06).
- **Origine (D11-15 et N2-13 du cadrage rouverts).** Le `.dmg` vient de la demande du pilote du 2026-11-06, relayée sous
  la délégation de Jérôme du 2026-11-02 (« Je te laisse manager les sessions ActCraft »). C'est un **artefact** : sa
  publication reste soumise au go écrit de la release ; hypothèse **H11-19** « à revoir par Jérôme : `.dmg` dans la
  release 1.0.0 ».
- **D11-15 remplacée** : la 1.0.0 livre aussi `ActCraft-<version>-macos-arm64.dmg` (Mac Apple Silicon), **non signé** :
  signature ad hoc (`codesign -s -`), ni certificat Developer ID ni notarisation. Le paragraphe « Paquet macOS » du plan
  de 11a et la ligne « macOS sans paquet » de la relecture ci-dessous sont caducs.
- **Tâche neuve L11-T29m** (ligne de table, section `#tache-l11-t29m`) : `scripts/paquet-mac.sh` (bash, sans `pwsh`),
  `scripts/verifier-licences-mac.sh`, `scripts/essai-dmg.sh`, nom du bundle `ActCraft` (`app/CMakeLists.txt`),
  bibliothèques non Qt du bundle déclarées dans `packaging/autres-autorises.txt` avec leurs notices, job `macos` de
  `release.yml`. Dépend de L11-T28 ; **V15 : L11-T29m seule ; V15b : L11-T29**, qui dépend de L11-T29m (son essai à
  blanc de `release.yml` doit contenir le job `macos`, relecture tour 1, M1) ; L11-T30 dépend des deux.
- **Aucune entrée ctest neuve** : N reste **N11 + 32** (L11-T1 en cours avec 32) ; la preuve est par script
  (`DMG OK`, `ESSAI DMG OK`, `LICENCES OK`), plus `tst_app_notices` (L11-T25) toujours vert.
- **`release.yml`** : job `macos`, sans secret ; **exécuteur choisi selon la visibilité du dépôt** (relecture M4),
  comme `runner_macos` de `ci.yml` : privé → runner auto-hébergé du Mac mini ; public → `macos-latest` (gratuit pour un
  dépôt public), par `runs-on: ${{ github.event.repository.private && fromJSON('[…]') || 'macos-latest' }}`. Aucun
  `exit 1` sur « public » : la garde de visibilité de `ci.yml:74-78` n'est pas reprise, seule la garde de mémoire
  (`ci.yml:79-91`). Le dépôt est privé depuis le 2026-11-06 (décision de Jérôme ; `CLAUDE.md`, qui dit « public », est
  en retard). Le lancement graphique y est informatif (`continue-on-error`) ; les contrôles `otool` et `dyld` sont
  bloquants. Le brouillon de release attend trois paquets.
- **Qt WebEngine** jamais dans le bundle : `paquet-mac.sh` retire le greffon `libqtwebview_webengine.dylib` (Qt WebView
  passe par le greffon natif `darwin`) puis tout framework que plus rien ne référence ; un `QtWebEngine*` restant =
  `LICENCES EN ECHEC` (arrêt).
- **L11-T25** : format des lignes de `packaging/autres-autorises.txt` fixé : `<motif glob> <expression SPDX>` (premier
  blanc = séparateur, `#` = commentaire), lu par `verifier-licences.ps1` et par `verifier-licences-mac.sh`.
- **L11-T28** : `README.md` (installer sous macOS, Gatekeeper, `xattr -dr com.apple.quarantine`) et section 1.0.0 de
  `CHANGELOG.md` (donc les notes de version) disent « non signé : clic droit > Ouvrir au premier lancement ».
- **L11-T29** : dépend de L11-T29m (V15b) ; son essai à blanc relève les motifs du job `macos` (`DMG OK` dans sa
  preuve). **L11-T30** : DMG dans le rapport de porte, la procédure du go et les mesures du MacBook Air M1.
- Interdits inchangés et répétés dans L11-T29m : aucun tag, aucune release, aucun `gh workflow run`, aucun push.

**Volet 3 — Lecture Mac des preuves.**
- L'exécution se fait sur le Mac selon `~/pilotage-mac/regles-tache-lot11-mac.md` (hors dépôt, non recopié ici) : il
  remplace sur le Mac les commandes PowerShell, `jeton.ps1` et les chemins `C:\…` ; le reste du plan vaut tel quel.
- Branche du lot : `feat/livraison-complete-l11`. Toute commande qui cite `feat/livraison-complete...HEAD` se lit
  `feat/livraison-complete-l11...HEAD`.
- Mutations : **la remise prescrite par la section l'emporte sur la règle Mac** (`git checkout -- <fichier>`) quand le
  fichier porte des ajouts non commités de la tâche (L11-T10, L11-T29m : aller-retour par `sed` ou copie, prouvé par
  `grep` ou `cmp`).
- Restes pour le PC : preuves Android (`APK OK`, `ANDROID … OK`), REA et ancien dépôt, corpus vidéo (contrôle des
  titres), `NOMS OK`, captures et banc D3D11 (`captures.ps1`, `bench.ps1`), installateur Windows, preuve MSVC
  `/W4 /WX`. Chaque tâche les liste dans son rapport ; l'orchestrateur les groupe dans la section « Restes pour le PC
  (lot 11) » du fichier d'état. Ils ne bloquent pas une tâche du Mac.
- Lot 7 absent de la tête (H8-7) : **L11-T27** se fait sans les épinglages du lot 7 (`lot7And8PinsPresent` vérifie ceux
  du lot 8 ; ceux du lot 7 sont « à raccorder à la fusion du lot 7 », au rapport) ; L11-T8 (11a) sans outils de maths.

## Amendement 2 (relecture tour 2)

La table des constats N1 à N4 et n1 à n6 est dans l'amendement 2 du plan de 11a ; elle fait autorité. Pour 11b :
- **N1** : la règle des réglages d'essai de 11a (assertion `QSettings probe` à la fin d'`initTestCase`, mutation qui
  retire `setDefaultFormat`) s'applique à `tst_ui_updatechecker`, `tst_ui_shortcuts`, `tst_ui_accessibility`,
  `tst_ui_lockall`, `tst_ui_profilesall` ; le contrôle `reg query` n'existe plus.
- **N3** : formes d'enregistrement de R1 (11a) : `tst_ui_updatechecker`, `tst_ui_shortcuts`, `tst_ui_accessibility`,
  `tst_ui_lockall` et `tst_ui_profilesall` en **F1** (`Main.qml`, forme de `tests/ui.cmake:33-36`) ;
  `tst_ui_helppages` et `tst_ui_pseudolocale` en F2 ; `tst_app_notices` et `tst_app_versioninfo` en F6.
- **N4** : le contrôle des API privées (lignes ajoutées depuis la base de la tâche) vaut pour L11-T19 (joueur).

## Table des tâches (11b)

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L11-T17 | Pages d'aide I : une page par outil des lots 1 à 5 (liste H1), écrites par nous | L11-T16 | `ui/help/<id>.html` pour chaque identifiant de la liste H1 (écrite dans cette ligne par l'orchestrateur après L11-T0), `ui/CMakeLists.txt`, `tests/tst_ui_helppages.cpp` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_helppages` : 6 fonctions `PASS` (dont `pagesOfLots1To5Exist`, `textsAreOurs`) ; `NOMS OK` ; contrôle des titres du corpus vide | S29 | #tache-l11-t17 |
| L11-T18 | Pages d'aide II : outils des lots 6 à 11 (liste H2), pages de procédure, index complet ; scénario `aide-complete` | L11-T17 | `ui/help/<id>.html` pour chaque identifiant de la liste H2, `ui/help/{index,raccourcis,deux-utilisateurs,modes-toucher,reprise,mises-a-jour,profils,mot-de-passe-oublie}.html`, `ui/CMakeLists.txt`, `tests/tst_ui_helppages.cpp`, `bench/scenarios/aide-complete.json` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_helppages` 9 fonctions `PASS` (dont `everyToolHasAPage`) ; `tst_captures_aide_complete` Passed ; `LU helpViewer.visible=true OK`, `CAPTURES OK` | S29 | #tache-l11-t18 |
| L11-T19 | Pseudo-langue de contrôle : génération au build des tests, chargement par le joueur, scénario `pseudo-langue` (liste des chaînes non marquées) | L11-T16 | `scripts/pseudo-langue.py`, `tests/ui.cmake`, `tests/bench.cmake`, `ui/include/actcraft/ui/language.h`, `ui/src/language.cpp`, `bench/captures/main.cpp`, `scripts/captures.ps1`, `tests/tst_ui_pseudolocale.cpp`, `bench/scenarios/controle/pseudo-langue.json` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_pseudolocale` 5 fonctions `PASS` ; scénario joué : `TEXTES OK <n>` ou liste `TEXTE ECART` écrite au rapport | S29 | #tache-l11-t19 |
| L11-T20 | Chaînes non traduisibles corrigées (liste de L11-T19), `actcraft_fr.ts` mis à jour, test de captures activé | L11-T19 | fichiers QML et C++ listés par le rapport de L11-T19 (`TEXTE ECART`, écrits dans cette ligne par l'orchestrateur), `ui/i18n/actcraft_fr.ts`, `tests/bench.cmake` | partagé | implementeur-tdd | `TESTS OK` ; `tst_captures_pseudo_langue` Passed ; `TEXTES OK <n>` à chaque étape `visibleTexts` ; `tst_ui_language` toujours `Passed` | S29 | #tache-l11-t20 |
| L11-T21 | Recherche de mises à jour I (F-760, F-930) : `UpdateChecker`, comparaison SemVer, aucune requête au démarrage par défaut, mandataire du lot 6 | L11-T16 | `ui/include/actcraft/ui/updatechecker.h`, `ui/src/updatechecker.cpp`, `tests/tst_ui_updatechecker.cpp`, `tests/data/updates/{newer,draft,prerelease}.json` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_updatechecker` 7 fonctions `PASS` (dont `noRequestAtStartByDefault`, `semverComparison`) ; mutation rouge puis remise | S29 | #tache-l11-t21 |
| L11-T22 | Recherche de mises à jour II : boîte, menu Aide, réglage « au démarrage » ; scénario `mises-a-jour` contre une réponse simulée | L11-T21 | `ui/qml/UpdateDialog.qml`, `ui/qml/SettingsPage.qml`, `bench/scenarios/mises-a-jour.json`, `tests/bench.cmake`, `tests/tst_ui_updatechecker.cpp` | isolé | implementeur-tdd | `TESTS OK` ; `tst_captures_mises_a_jour` Passed ; `tst_ui_updatechecker` : `startCalledOnceByTheShell` `PASS` (8 fonctions) ; `LU appUpdates.requestCount=0 OK` avant la commande, `LU appUpdates.latestVersion=9.9.9 OK`, `CAPTURES OK` | S29 | #tache-l11-t22 |
| L11-T23 | Raccourcis clavier restants (F-950 à F-953) : table complète, aucun doublon, aucun raccourci pendant la saisie | L11-T18, L11-T20 | `actions/data/commands.json`, `tests/tst_ui_shortcuts.cpp` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_shortcuts` 4 fonctions `PASS` ; `tst_actions_registry` (`noShortcutTwice`) toujours `Passed` | S2.1 | #tache-l11-t23 |
| L11-T24 | Accessibilité : contrastes WCAG AA et focus visible dans toutes les boîtes et en mise en page étroite, noms accessibles des boutons | L11-T20 | `tests/tst_ui_accessibility.cpp`, `tests/support/wcag.h`, fichiers QML corrigés listés par le rapport de la tâche (écrits dans cette ligne par l'orchestrateur à la fin de l'étape 2) | partagé | implementeur-tdd | `TESTS OK` ; `tst_ui_accessibility` 4 fonctions `PASS` ; `tst_ui_shell` (`renderedColorsMeetWcagAa`, `keyboardFocusIsVisible`) toujours `Passed` | S2.2 | #tache-l11-t24 |
| L11-T25 | Licences tierces : `THIRD_PARTY_NOTICES.md` complet (Qt et ses modules, Qt PDF et PDFium, Qt Multimedia et FFmpeg, Hunspell selon H6-6b, dictionnaire, miniz, runtime MSVC), textes des licences, contrôle des bibliothèques non Qt | L11-T16, **décision de Jérôme H6-6b** (sinon arrêt) | `THIRD_PARTY_NOTICES.md`, `LICENSES/*.txt` (textes SPDX cités), `scripts/verifier-licences.ps1`, `packaging/qt-autorises.txt`, `packaging/autres-autorises.txt`, `tests/tst_app_notices.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_app_notices` 4 fonctions `PASS` ; `LICENCES OK` sur le dossier déployé et sur l'APK | S1.1 | #tache-l11-t25 |
| L11-T26 | S19 rejoué sur toutes les commandes des lots 1 à 11 : attributs déclarés partout, verrou exact, scénario `classe-protegee-complete` | L11-T9, L11-T23 | `actions/data/commands.json`, `tests/tst_ui_lockall.cpp`, `bench/scenarios/classe-protegee-complete.json` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_lockall` 5 fonctions `PASS` ; `tst_captures_classe_protegee_complete` Passed ; `LU commands.lockedCount=<K> OK` (K exact, calculé des données) ; mot de passe absent de l'INI | S19 | #tache-l11-t26 |
| L11-T27 | S32 : les cinq profils de bout en bout (épinglages des lots 7 et 8, navigateurs et réglages, filtre d'Android) ; scénario `profils-complets` | L11-T9, L11-T20 | `actions/data/profiles/{en-classe,preparation,lecture-ecriture,calcul-geometrie,son-image}.json`, `actions/include/actcraft/actions/profile.h`, `actions/src/profile.cpp`, `tests/tst_ui_profilesall.cpp`, `bench/scenarios/profils-complets.json` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_profilesall` 5 fonctions `PASS` ; `tst_captures_profils_complets` Passed ; pour chaque profil `LU appProfile.currentId=<id> OK` ; `tst_ui_profiles` (`namesAreOurs` en `PASS`) toujours `Passed` | S32 | #tache-l11-t27 |
| L11-T28 | Version 1.0.0 (`versionCode` 10000), section 1.0.0 de `CHANGELOG.md`, guide de démarrage dans `README.md`, notes de version | L11-T17 à L11-T27 | `CMakeLists.txt`, `CHANGELOG.md`, `README.md`, `tests/app.cmake`, `tests/tst_app_versioninfo.cpp` | isolé | implementeur-tdd | `TESTS OK` ; `tst_app_versioninfo` 4 fonctions `PASS`, `tst_app_version` (`--version`) toujours `Passed` ; `Get-ActCraftVersionCode` = `10000` ; `NOTES OK` ; `NOMS OK` ; contrôle des titres du corpus vide | S14 | #tache-l11-t28 |
| L11-T29 | Paquets 1.0.0 et S14 : installateur et APK locaux, essais, flux de release en essai à blanc sur la CI, mode réel sans tag refusé — release prête | L11-T28, L11-T29m | `scripts/paquet.ps1`, `docs/verifications/2026-10-22-lot11-release-prete.md` | partagé | implementeur-tdd | `PAQUET OK : …ActCraft-1.0.0-setup.exe`, `INSTALLATEUR OK`, `APK OK`, `LICENCES OK` ×2, `ANDROID OK`, `ANDROID FICHIERS OK` ; run d'essai : `APK NON PUBLIABLE`, `DMG OK` et `ESSAI DMG OK` (job `macos`), `NOTES OK`, `SHA256SUMS.txt`, `SECRETS ABSENTS` et `CONTROLE OK` du job `controle-mode-reel`, aucune release ; branche d'essai supprimée ; aucun `gh workflow run release.yml -f mode=reel` | S14 | #tache-l11-t29 |
| L11-T29m | Paquet macOS : `.app` déployé par `macdeployqt` et `.dmg` non signé, 1.0.0 | L11-T28 | `scripts/paquet-mac.sh`, `scripts/essai-dmg.sh`, `scripts/verifier-licences-mac.sh`, `app/CMakeLists.txt`, `packaging/qt-autorises.txt`, `packaging/autres-autorises.txt`, `THIRD_PARTY_NOTICES.md`, `LICENSES/*.txt` (textes SPDX des bibliothèques du bundle), `.github/workflows/release.yml`, `docs/verifications/2026-11-07-lot11-paquet-macos.md` | isolé | concepteur-critique | `LICENCES OK` sur le bundle ; `DMG OK : …/build/paquet/ActCraft-1.0.0-macos-arm64.dmg` ; `ESSAI DMG OK` (dont `VERSION OK : ActCraft 1.0.0`, `LANCEMENT OK`, `qt.conf`, aucune bibliothèque chargée hors du bundle à `--version` ni au lancement, `libqcocoa.dylib` du bundle) ; contre-épreuves `otool` et `dyld` ≥ 1 sur le `.app` non déployé ; aucun `QtWebEngine*` ; mutation de `autres-autorises.txt` → `LICENCES EN ECHEC`, remise ; `tst_app_notices` toujours `Passed` ; `YAML OK` (job `macos`) ; suite `100% tests passed … out of N`, N = N11 + 32 (aucun test neuf) | S14 | #tache-l11-t29m |
| L11-T30 | Clôture du lot et porte de fin : suites, captures et banc à l'écran (jeton `ecran`), S13 rejoué, talons nuls, graphe, mesures et procédures pour Jérôme, go demandé | L11-T29, L11-T29m | `tests/tst_ui_shell.cpp`, `docs/mesures/2026-10-22-lot11-fin.md`, `docs/verifications/2026-10-22-lot11-porte.md`, `graphify-out/` | partagé | implementeur-tdd | debug et release `TESTS OK`, N = N11 + 32 ; `git -C <wt> grep -l "Talon L11" -- . ':!docs/' ':!graphify-out/' ':!tests/tst_ui_shell.cpp'` vide ; `captures.ps1 -Scenario tous -Rendu d3d11` `CAPTURES OK` ; banc D3D11 plein écran × 3 : verdict et 0 figement ; rapport de porte commité | S14, S13, S2, S3 | #tache-l11-t30 |

## Vagues de 11b (largeur 3 au plus ; aucun fichier partagé dans une vague)

V8 : L11-T17 ‖ L11-T19 ‖ L11-T21 · V9 : L11-T18 ‖ L11-T22 ‖ L11-T25 · V10 : L11-T20 (seule) · V11 : L11-T24 (seule) ·
V12 : L11-T23 ‖ L11-T27 · V13 : L11-T26 · V14 : L11-T28 · V15 : L11-T29m (seule) · V15b : L11-T29 · V16 : L11-T30.

Vérification des recoupements :
- V8 : L11-T17 (`ui/help/` H1, `ui/CMakeLists.txt`, `tst_ui_helppages.cpp`) / L11-T19 (`pseudo-langue.py`,
  `tests/ui.cmake`, `tests/bench.cmake`, `language.*`, `bench/captures/main.cpp`, `captures.ps1`, un test, un scénario)
  / L11-T21 (`updatechecker.*`, un test, `tests/data/updates/`) — disjoints.
- V9 : L11-T18 (`ui/help/` H2 et procédures, `ui/CMakeLists.txt`, `tst_ui_helppages.cpp`, un scénario) / L11-T22
  (`UpdateDialog.qml`, `SettingsPage.qml`, un scénario, `tests/bench.cmake`, `tst_ui_updatechecker.cpp` repris de L11-T21) / L11-T25 (`THIRD_PARTY_NOTICES.md`,
  `LICENSES/`, `verifier-licences.ps1`, `packaging/`, un test) — disjoints. `tests/bench.cmake` : L11-T19 (V8) puis
  L11-T22 (V9), tenu par la vague.
- V10 et V11 : tâches à fichiers larges (QML), seules dans leur vague, worktree `partagé`.
- V12 : L11-T23 (`commands.json`, un test) / L11-T27 (profils, `profile.*`, un test, un scénario) — disjoints.
- V13 : L11-T26 seule (`commands.json` après L11-T23).
- V15 et V15b (amendement 3) : L11-T29m seule (trois scripts bash, `app/CMakeLists.txt`, `packaging/`, notices,
  `LICENSES/`, `release.yml`, un rapport), puis L11-T29 (`scripts/paquet.ps1`, un rapport), qui en dépend : son essai à
  blanc doit voir le job `macos`. `packaging/`, `THIRD_PARTY_NOTICES.md` et `LICENSES/` : L11-T25 (V9) puis L11-T29m
  (V15), tenus par « dépend de ».
- Fichiers repris d'une vague à l'autre, tenus par « dépend de » : `ui/CMakeLists.txt` (L11-T17 → L11-T18) ;
  `tst_ui_helppages.cpp` (L11-T17 → L11-T18) ; `tests/bench.cmake` (L11-T19 → L11-T22 → L11-T20) ;
  `commands.json` (L11-T9 → L11-T23 → L11-T26).

**Écran.** Seule L11-T30 ouvre des fenêtres visibles (captures et banc en D3D11, plein écran sur l'écran principal),
sous le jeton `ecran`, annoncés à Jérôme d'une ligne avant. L11-T29 ouvre la fenêtre de l'essai de l'installateur
(5 s) : l'annoncer d'une ligne. L11-T29m (amendement 3) ouvre la fenêtre d'ActCraft 3 s sur l'écran du Mac
(`essai-dmg.sh --lancer`) : l'annoncer d'une ligne.

## Décisions appliquées (suite de D11-1 à D11-12 ; L11-T0 peut les amender)

- **D11-13 — Mises à jour.** Source : `https://api.github.com/repos/jejeasterix/actcraft/releases/latest` ; brouillons
  et préversions ignorés ; Windows seul (`help.check-updates` déclaré `"platforms": ["windows"]`) ; recherche au
  démarrage désactivée par défaut (N1-A12), réglage du poste `updates/check-at-start` ; mandataire du lot 6 appliqué
  (A16) ; aucune télémétrie, aucun identifiant envoyé ; une version plus récente ouvre la page de la release dans le
  navigateur du système, rien n'est téléchargé. Point d'entrée remplaçable pour les essais par la variable
  `ACTCRAFT_UPDATES_URL`, acceptée seulement en `file:` ou en `http://127.0.0.1:<port>` / `http://localhost:<port>`.
- **D11-14 — Pseudo-langue.** Code `xx` (usage privé), chaque traduction = `⟦` + source + `⟧`, marqueurs `%1`, `%n` et
  balises gardés ; générée au build des tests seulement (`build/<preset>/i18n/actcraft_xx.qm`), jamais dans les
  ressources ni dans les paquets ; chargée par l'option `--traduction <fichier .qm>` du joueur de captures seulement.
- **D11-15 — Paquet macOS non signé** (remplacée par l'amendement 3, demande du pilote du 2026-11-06 ; texte
  d'origine : « pas de paquet macOS », cadrage § 6, amendement 1 § 1). `ActCraft-<version>-macos-arm64.dmg`, signature
  ad hoc, ni Developer ID ni notarisation ; construit par L11-T29m ; macOS reste construit et testé par la CI.
- **D11-16 — Version.** 1.0.0 ; `versionCode` = 1 × 10000 + 0 × 100 + 0 = **10000**, plus grand que celui de 0.10.0
  (1000) : la mise à jour de l'APK par-dessus la version du lot 9 passe (amendement 1, § 6).

## Listes de 11b

- **H1 et H2 — pages d'aide.** L11-T0 écrit dans son rapport la liste des identifiants de commandes `tool.*` (et des
  commandes d'outils de présentation et de maths) **disponibles** à la tête du chantier, partagée en H1 (outils des
  lots 1 à 5) et H2 (lots 6 à 11) ; l'orchestrateur la recopie dans les lignes de L11-T17 et L11-T18 avant leur
  lancement. Un nom de fichier = l'identifiant de la commande (`ui/help/tool.pen.html`), comme le lit
  `HelpViewer::openFor` (A8).
- **Chaînes à corriger (L11-T20)** : sortie `TEXTE ECART` de L11-T19, regroupée par fichier dans son rapport.
- **QML à corriger (L11-T24)** : sortie de l'étape 2 de L11-T24 (fonctions en `FAIL!`), regroupée par fichier.

---

<a id="tache-l11-t17"></a>
### Tâche L11-T17 — Pages d'aide I (outils des lots 1 à 5)

**Objectif.** F-760 (aide), N2-A4 : une page HTML par identifiant de la liste H1, écrite par nous en français simple :
à quoi sert l'outil, comment le choisir (palette, menu, raccourci), comment s'en servir au stylet, à la souris et au
doigt, ses réglages, un lien vers l'index. Pages servies par `qrc:/actcraft/help/` (A8), ajoutées à la ressource de
`ui/CMakeLists.txt` à côté de `ui/help/index.html` (L6-T35).

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R tst_ui_helppages` ; « fonctions d'un test ».

**Interdits.** Aucun texte, aucune image, aucune capture de l'aide d'ActivInspire ni de sa documentation
(`Documentation Activinspire/` sert à comprendre, jamais à copier) ; aucune image dans les pages ; aucun `<script>` ;
aucune ressource distante ; aucun nom de l'original (termes de N1-12) ; aucun titre de vidéo.

**Terminé quand** les 6 fonctions passent, `NOMS OK`, contrôle des titres du corpus vide.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_ui_helppages.cpp` (remplace le talon) :
  ```cpp
  #include <actcraft/ui/helpviewer.h>

  #include <QFile>
  #include <QRegularExpression>
  #include <QtTest>

  namespace {
  // Liste H1, recopiée de L11-T0 par l'orchestrateur ; l'exemple ci-dessous est à remplacer par la liste réelle.
  const QStringList kLots1To5 = {QStringLiteral("tool.pen"), QStringLiteral("tool.highlighter"),
                                 QStringLiteral("tool.eraser"), QStringLiteral("tool.select")};
  QString page(const QString& id)
  {
      QFile f(QStringLiteral(":/actcraft/help/%1.html").arg(id));
      return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
  }
  } // namespace

  class TstUiHelpPages : public QObject
  {
      Q_OBJECT
  private slots:
      void pagesOfLots1To5Exist()
      {
          for (const QString& id : kLots1To5)
              QVERIFY2(!page(id).isEmpty(), qPrintable(id));
      }
      void everyPageHasATitleAndABackLink()
      {
          for (const QString& id : kLots1To5) {
              const QString html = page(id);
              QVERIFY2(QRegularExpression(QStringLiteral("<title>[^<]{3,}</title>")).match(html).hasMatch(), qPrintable(id));
              QVERIFY2(html.contains(QStringLiteral("href=\"index.html\"")), qPrintable(id));
          }
      }
      void internalLinksResolve()
      {
          const QRegularExpression link(QStringLiteral("href=\"([^\"#:]+\\.html)"));
          for (const QString& id : kLots1To5) {
              auto it = link.globalMatch(page(id));
              while (it.hasNext()) {
                  const QString target = it.next().captured(1);
                  QVERIFY2(QFile::exists(QStringLiteral(":/actcraft/help/") + target), qPrintable(id + " → " + target));
              }
          }
      }
      void noScriptImageOrRemoteResource()
      {
          for (const QString& id : kLots1To5) {
              const QString html = page(id);
              QVERIFY2(!html.contains(QStringLiteral("<script"), Qt::CaseInsensitive), qPrintable(id));
              QVERIFY2(!html.contains(QStringLiteral("<img"), Qt::CaseInsensitive), qPrintable(id));
              QVERIFY2(!html.contains(QStringLiteral("src=\"http"), Qt::CaseInsensitive), qPrintable(id));
          }
      }
      void textsAreOurs()
      {
          QStringList forbidden = {QStringLiteral("ActivInspire"), QStringLiteral("Promethean")};
          const QString names = qEnvironmentVariable("ACTCRAFT_REFERENCE_NAMES");
          if (!names.isEmpty()) {
              QFile f(names);
              QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
              for (const QByteArray& line : f.readAll().split('\n'))
                  if (!line.trimmed().isEmpty())
                      forbidden << QString::fromUtf8(line.trimmed());
          }
          for (const QString& id : kLots1To5)
              for (const QString& word : forbidden)
                  QVERIFY2(!page(id).contains(word, Qt::CaseInsensitive), qPrintable(id + " : " + word));
      }
      void f1OpensTheToolPage()
      {
          actcraft::ui::HelpViewer viewer;
          viewer.openFor(QStringLiteral("tool.pen"));
          QVERIFY(viewer.source().toString().endsWith(QStringLiteral("tool.pen.html")));
      }
  };

  QTEST_MAIN(TstUiHelpPages)
  #include "tst_ui_helppages.moc"
  ```
  La preuve exige `textsAreOurs` en `PASS` **avec** `ACTCRAFT_REFERENCE_NAMES` posée par l'orchestrateur vers le
  fichier hors dépôt `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot11-noms.txt` (écrit par L11-T0 : les noms
  de `lot6-noms-profils.txt` et ceux des outils et des produits de l'original, un par ligne). Aucun de ces noms n'est
  écrit dans le dépôt : le test n'en garde en clair que les deux noms de la marque.
- [ ] **Étape 2 : rouge vu.**
- [ ] **Étape 3 : pages.** Une page par identifiant H1, gabarit commun à nous :
  ```html
  <!doctype html>
  <html lang="fr">
  <head><meta charset="utf-8"><title>Stylo</title></head>
  <body>
  <h1>Stylo</h1>
  <p>Le stylo écrit à main levée sur la page, dans la couleur et l'épaisseur choisies.</p>
  <h2>Le choisir</h2>
  <p>Bouton du stylo dans la palette, ou menu Outils ▸ Stylo.</p>
  <h2>S'en servir</h2>
  <p>Posez la pointe du stylet, ou le doigt quand le mode du toucher le permet, et tracez. Relevez pour finir le trait.</p>
  <h2>Réglages</h2>
  <p>Couleur : une pastille de la palette. Épaisseur : les quatre tailles, ou la réglette.</p>
  <p><a href="index.html">Retour au sommaire</a></p>
  </body>
  </html>
  ```
  (exemple pour `tool.pen` ; chaque page décrit le comportement réel d'ActCraft, relu dans son code et ses tests).
  Ressource ajoutée dans `ui/CMakeLists.txt`.
- [ ] **Étape 4 : vert** (avec `ACTCRAFT_REFERENCE_NAMES`), preuve de tâche, `NOMS OK`, contrôle des titres du corpus
  vide ; commit « Lot 11 : pages d'aide des outils des lots 1 à 5 (L11-T17) ».

<a id="tache-l11-t18"></a>
### Tâche L11-T18 — Pages d'aide II, procédures et index ; scénario `aide-complete`

**Objectif.** Pages des identifiants H2 (même gabarit), pages de procédure (`raccourcis.html` : table des raccourcis
lue dans `commands.json` au moment de l'écriture ; `deux-utilisateurs.html` ; `modes-toucher.html` ; `reprise.html` ;
`mises-a-jour.html` ; `profils.html` ; `mot-de-passe-oublie.html` repris de l'index de L6-T35 en page à part) ; index
qui lie chaque page. Trois fonctions ajoutées : `everyToolHasAPage` (chaque commande `tool.*` disponible sur la
plateforme du test, lue dans le registre, a sa page), `procedurePagesExist`, `indexLinksEveryPage` (chaque fichier de
`:/actcraft/help/` sauf l'index est lié depuis l'index). Les listes `kLots1To5` et H2 sont réunies en `kAllTools`.

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_helppages|tst_captures_aide_complete|tst_ui_help'`.

**Preuve visuelle.** `captures.ps1 -Scenario aide-complete -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "helpViewer.visible=true"` :
`LU … OK`, `CAPTURES OK` ; décrire `aide-stylo.png` et `aide-deux-utilisateurs.png`.

**Interdits.** Ceux de L11-T17 ; aucun chargement réseau dans le lecteur (L6-T35, `noNetworkUrl`).

**Terminé quand** les 9 fonctions et le scénario passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** (trois fonctions ajoutées ; `kAllTools`).
- [ ] **Étape 2 : pages et index**, ressource.
- [ ] **Étape 3 : scénario** `aide-complete.json` : `waitReady` ; `tool.pen` ; `{"key": "F1"}` ;
  `waitFor helpViewer.visible = true` ; `expect helpViewer.currentPage = tool.pen` (propriété ajoutée si absente : nom de la
  page courante, sans extension) ; capture `aide-stylo` ; Échap ; `users.dual` ; `{"key": "F1"}` ;
  `expect helpViewer.currentPage = deux-utilisateurs` (F1 en mode à deux utilisateurs ouvre sa page) ; capture
  `aide-deux-utilisateurs` ; Échap ; `users.dual`. Si F1 en mode à deux utilisateurs ne doit pas ouvrir cette page
  (L11-T0), l'étape passe par l'index (`helpViewer.follow`, commande `help.page deux-utilisateurs` si elle existe).
- [ ] **Étape 4 : vert**, preuve visuelle, preuve de tâche, `NOMS OK`, contrôle des titres du corpus vide ; commit
  « Lot 11 : pages d'aide complètes et procédures (L11-T18) ».

<a id="tache-l11-t19"></a>
### Tâche L11-T19 — Pseudo-langue de contrôle ; scénario `pseudo-langue`

**Objectif.** D11-14, S29 (langue). `scripts/pseudo-langue.py <entrée.ts> <sortie.ts>` (Python 3, lancé par
`python -I`) : copie le `.ts` produit par `lupdate` (`ui/i18n/actcraft_fr.ts`, L6-T36) en remplissant chaque
`<translation>` par `⟦<source>⟧` (formes plurielles comprises, `%1` et `%n` gardés, attribut `type="unfinished"`
retiré), écrit en UTF-8 sans BOM, fins de ligne LF (`write_bytes`). `tests/ui.cmake` : commande de build qui produit
`build/<preset>/i18n/actcraft_xx.qm` (`find_package(Python3 COMPONENTS Interpreter)`, puis `Qt6::lrelease`), dépendance
de `tst_ui_pseudolocale` et de `tst_captures_pseudo_langue`. `Language::installFile(QCoreApplication&, QString qmPath)`
(nouvelle fonction) ; le joueur de captures accepte `--traduction <qm>` et l'installe **avant** `loadMainWindow` ;
`scripts/captures.ps1` gagne `-Traduction <fichier>` qu'il passe au joueur. Étapes `visibleTexts` (L11-T2) sur la
fenêtre principale, le menu Fichier ouvert, et trois boîtes (À propos, taille de page, réglages).

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R tst_ui_pseudolocale` ; puis le scénario par `captures.ps1` (le test
`tst_captures_pseudo_langue` reste désactivé jusqu'à L11-T20).

**Preuve visuelle.** `captures.ps1 -Scenario controle/pseudo-langue -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Traduction build/debug/i18n/actcraft_xx.qm -Lire "appPreferences.language=fr"` :
lignes `TEXTES OK <n>` ou `TEXTES EN ECART <k>` et leurs `TEXTE ECART` ; la liste entière est recopiée au rapport,
regroupée par fichier source (recherche du texte dans `ui/`, `features/`, `app/` par `git grep -n -F`).

**Interdits.** La pseudo-langue n'entre dans aucune ressource ni aucun paquet ; aucune traduction automatique ;
aucune correction de chaîne ici (c'est L11-T20).

**Terminé quand** les 5 fonctions passent et la liste des écarts est au rapport (vide ou non).

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_pseudolocale.cpp` : `everySourceMarked` (le `.ts` généré : chaque
  `<translation>` = `⟦` + source + `⟧`) ; `placeholdersKept` (`%1`, `%n` présents à l'identique) ; `qmLoads`
  (`QTranslator::load` vrai ; `QCoreApplication::translate("Main", "<une source relevée dans actcraft_fr.ts>")` rend la
  forme marquée) ; `installFileInstallsTheTranslator` ; `notShippedInResources` (`QFile::exists(":/i18n/actcraft_xx.qm")`
  faux, et aucune entrée `xx` dans `Language::available()`).
- [ ] **Étape 2 : script, CMake, `language.*`, joueur, `captures.ps1`.**
- [ ] **Étape 3 : scénario** `bench/scenarios/controle/pseudo-langue.json` (sous-dossier que `-Scenario tous` ne prend
  pas ; déclaration ctest de L11-T1 à ce chemin) : `waitReady` ; `{"visibleTexts": "pseudo", "ignore": ["pageIndicator", "zoomLabel"]}` ;
  capture `principale` ; menu Fichier ouvert (`pointer` sur son titre, coordonnées de fenêtre relevées sur la capture
  `principale`) ; `visibleTexts` ; capture `menu-fichier` ; Échap ; `help.about` ; `visibleTexts` ; capture `a-propos` ;
  Échap ; boîte de taille de page (commande relevée dans `commands.json`) ; `visibleTexts` ; Échap ; boîte des réglages ;
  `visibleTexts` ; capture `reglages`.
- [ ] **Étape 4 : vert** (unitaire), scénario joué, rapport ; preuve de tâche ; commit « Lot 11 : pseudo-langue de
  contrôle des textes (L11-T19) ».

<a id="tache-l11-t20"></a>
### Tâche L11-T20 — Chaînes non traduisibles corrigées

**Objectif.** Chaque `TEXTE ECART` de L11-T19 : la chaîne passe par `qsTr()` (QML) ou `tr()` / `QCoreApplication::translate`
(C++) ; une chaîne qui vient des données (`commands.json`, profils) passe par `QT_TRANSLATE_NOOP` à la lecture ou par un
contexte de traduction des données (forme retenue écrite au rapport) ; `ui/i18n/actcraft_fr.ts` régénéré par la cible
`update_translations` (L6-T36) ; `DISABLED` retiré de `tst_captures_pseudo_langue` dans `tests/bench.cmake` (avec
l'argument `--traduction` vers le `.qm` de build et le scénario `bench/scenarios/controle/pseudo-langue.json`).

**Fichiers.** Ceux de la ligne de la table (liste exacte écrite par l'orchestrateur depuis le rapport de L11-T19).

**Test et commande bornée.** Commande ciblée `-R 'tst_captures_pseudo_langue|tst_ui_language|tst_ui_shell'`.

**Preuve visuelle.** Commande de L11-T19 : `TEXTES OK <n>` à chaque étape `visibleTexts`, `CAPTURES OK` ; décrire
`principale.png` (tous les libellés entre `⟦ ⟧`).

**Interdits.** Aucun texte changé (seule l'enveloppe de traduction change) ; aucun contenu de document marqué comme
écart corrigé par une exclusion (`ignore`) sans le dire au rapport.

**Terminé quand** le test de captures passe et la suite reste verte.

**Étapes.**
- [ ] **Étape 1 :** reproduire la liste (rouge : `TEXTES EN ECART`).
- [ ] **Étape 2 :** corriger fichier par fichier ; régénérer `actcraft_fr.ts` ; rejouer le scénario.
- [ ] **Étape 3 : vert**, preuve de tâche ; `NOMS OK` ; commit « Lot 11 : toutes les chaînes visibles traduisibles
  (L11-T20) ».

<a id="tache-l11-t21"></a>
### Tâche L11-T21 — Recherche de mises à jour I

**Objectif.** D11-13, N1-A12.

**Contrat.**
```cpp
// ui/include/actcraft/ui/updatechecker.h
#pragma once

#include <actcraft/ui/preferences.h>

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

class UpdateChecker : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Preferences* preferences READ preferences WRITE setPreferences NOTIFY preferencesChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY stateChanged)
    Q_PROPERTY(QUrl releasePage READ releasePage NOTIFY stateChanged)
    Q_PROPERTY(bool checkAtStart READ checkAtStart WRITE setCheckAtStart NOTIFY checkAtStartChanged)
    Q_PROPERTY(int requestCount READ requestCount NOTIFY stateChanged)

public:
    enum class State { Idle, Checking, UpToDate, Available, Failed };
    Q_ENUM(State)

    explicit UpdateChecker(QObject* parent = nullptr);
    // Versions « x.y.z », préfixe « v » toléré ; faux si l'une ne se lit pas.
    static bool isNewer(QStringView candidate, QStringView current);
    // Point d'entrée : ACTCRAFT_UPDATES_URL si file: ou http://127.0.0.1 / localhost, sinon l'API des releases du dépôt.
    static QUrl endpoint();
    void setNetworkAccessManager(QNetworkAccessManager* manager); // tests ; sinon un gestionnaire interne
    Q_INVOKABLE void check();
    Q_INVOKABLE void start(); // appelé une fois au démarrage : check() seulement si checkAtStart

    Preferences* preferences() const;
    void setPreferences(Preferences* preferences);
    State state() const;
    QString latestVersion() const;
    QUrl releasePage() const;
    bool checkAtStart() const; // réglage updates/check-at-start, faux par défaut
    void setCheckAtStart(bool on);
    int requestCount() const;

signals:
    void preferencesChanged();
    void stateChanged();
    void checkAtStartChanged();
};

} // namespace actcraft::ui
```
Réponse lue : `tag_name`, `html_url`, `draft`, `prerelease` ; un brouillon ou une préversion vaut `UpToDate` ; erreur
réseau ou JSON illisible → `Failed`. Le mandataire de `NetworkSettings` (A16) est posé sur le gestionnaire interne.

**Fichiers.** Ceux de la ligne de la table ; `tests/data/updates/newer.json` :
```json
{"tag_name": "v9.9.9", "html_url": "https://github.com/jejeasterix/actcraft/releases/tag/v9.9.9", "draft": false, "prerelease": false}
```
`draft.json` (`v9.9.9`, `"draft": true`), `prerelease.json` (`v9.9.9`, `"prerelease": true`).

**Test et commande bornée.** Commande ciblée `-R tst_ui_updatechecker` ; « fonctions d'un test ».

**Interdits.** Aucune requête réseau réelle dans les tests (réponses en `file:`) ; aucune requête au démarrage quand le
réglage est faux ; aucun téléchargement ; aucun envoi d'identifiant.

**Terminé quand** les 7 fonctions passent et la mutation est vue rouge.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_updatechecker.cpp` : `semverComparison` (`isNewer("1.0.1","1.0.0")`,
  `isNewer("1.10.0","1.9.0")`, `isNewer("0.10.0","0.9.0")`, `!isNewer("1.0.0","1.0.0")`, `!isNewer("zzz","1.0.0")`) ;
  `noRequestAtStartByDefault` (gestionnaire espion qui compte `createRequest` ; `start()` → 0) ;
  `requestAtStartWhenEnabled` ; `availableFromReply` (`ACTCRAFT_UPDATES_URL` = `file:` de `newer.json` →
  `Available`, `latestVersion` `9.9.9`, `releasePage` lue) ; `draftAndPrereleaseIgnored` ; `failureReported`
  (fichier absent → `Failed`) ; `remoteOverrideRefused` (`ACTCRAFT_UPDATES_URL=https://exemple.invalid/x` → point
  d'entrée par défaut).
- [ ] **Étape 2 : implémentation.**
- [ ] **Étape 3 : mutation** : faire appeler `check()` sans condition dans `start()` → `noRequestAtStartByDefault`
  rouge ; `git -C <wt> checkout -- ui/src/updatechecker.cpp` ; vert.
- [ ] **Étape 4 : preuve de tâche** ; commit « Lot 11 : recherche de mises à jour (L11-T21) ».

<a id="tache-l11-t22"></a>
### Tâche L11-T22 — Recherche de mises à jour II ; scénario `mises-a-jour`

**Objectif.** `UpdateDialog.qml` : gestionnaire de `help.check-updates` (remplace le talon de L11-T3) : ouvre la boîte,
appelle `appUpdates.check()`, montre l'état (recherche, à jour, version plus récente avec son numéro, échec), bouton
« Voir la version » (`Qt.openUrlExternally(appUpdates.releasePage)`), bouton « Fermer ». `SettingsPage.qml` : case
« Rechercher les mises à jour au démarrage » liée à `appUpdates.checkAtStart` (Windows seulement). `tests/bench.cmake` :
`ENVIRONMENT "${_bench_env_offscreen};ACTCRAFT_UPDATES_URL=file:///${PROJECT_SOURCE_DIR}/tests/data/updates/newer.json"`
sur `tst_captures_mises_a_jour` (la liste `_bench_env_offscreen`, `tests/bench.cmake:8-10`, est gardée : sans elle la
fenêtre s'ouvrirait à l'écran). Démarrage : `UpdateDialog.qml` porte `Component.onCompleted: appUpdates.start()`
(un seul appel, au chargement de la coquille) ; sans lui, la case « au démarrage » ne ferait rien. Fonction ajoutée à
`tests/tst_ui_updatechecker.cpp` : `startCalledOnceByTheShell` (`Main.qml` chargé, réglages d'essai en INI temporaire
avec `updates/check-at-start=true` et `ACTCRAFT_UPDATES_URL` vers `newer.json` → `requestCount == 1` ; même montage
avec `false` → 0).

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R tst_captures_mises_a_jour`.

**Preuve visuelle.** `captures.ps1 -Scenario mises-a-jour -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "appUpdates.checkAtStart=false"`
avec `$env:ACTCRAFT_UPDATES_URL` posé comme dans `tests/bench.cmake` : `LU appUpdates.requestCount=0 OK`,
`LU appUpdates.latestVersion=9.9.9 OK`, `CAPTURES OK` ; décrire `version-plus-recente.png`.

**Interdits.** Aucun clic sur « Voir la version » dans le scénario (il ouvrirait le navigateur) ; aucune boîte au
démarrage.

**Terminé quand** le scénario et `startCalledOnceByTheShell` passent.

**Étapes.**
- [ ] **Étape 1 : scénario** `mises-a-jour.json` : `waitReady` ; `expect appUpdates.requestCount = 0` (S29 : aucune
  requête au démarrage, réglage désactivé) ; `command help.check-updates` ; `waitFor appUpdates.state = Available`
  (forme de l'énumération lue par le joueur : valeur entière ou nom, relevée dans `uiscenario.cpp`) ;
  `expect updateDialog.visible = true` ; `expect appUpdates.latestVersion = 9.9.9` ; capture `version-plus-recente`.
- [ ] **Étape 2 : rouge vu** (talon), **QML**, réglage, `tests/bench.cmake`.
- [ ] **Étape 3 : vert**, preuve visuelle, preuve de tâche ; `NOMS OK` ; commit « Lot 11 : boîte de recherche de mises
  à jour (L11-T22) ».

<a id="tache-l11-t23"></a>
### Tâche L11-T23 — Raccourcis clavier restants (F-950 à F-953)

**Objectif.** Décision 14 du cadrage. Table complète des raccourcis des fonctions incluses (liste établie par L11-T0
depuis F-950 à F-953 de la spécification, en nos termes ; touches génériques du système gardées : Ctrl+N, Ctrl+O,
Ctrl+S, Ctrl+Z, Ctrl+Y, Ctrl+C, Ctrl+X, Ctrl+V, Suppr, F1, F5…) ; chaque raccourci déclaré dans `commands.json` ; aucun
doublon (`noShortcutTwice`, `tst_actions_registry`) ; aucun raccourci d'une seule touche pendant la saisie d'un texte
(Suppr efface une lettre, pas l'objet) ; la page `raccourcis.html` (L11-T18) les liste tous.

**Fichiers.** `actions/data/commands.json`, `tests/tst_ui_shortcuts.cpp`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_shortcuts|tst_actions_registry'`.

**Interdits.** Aucun raccourci qui reprend une combinaison propre à l'original sans équivalent générique (relevé par
L11-T0) ; aucune touche réservée au système (Alt+F4, Win+…).

**Terminé quand** les 4 fonctions passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_shortcuts.cpp` (montage `Main.qml`, réglages d'essai) :
  `everyListedShortcutDeclared` (table du test = liste de L11-T0 : identifiant → touches) ;
  `shortcutTriggersItsCommand` (`QTest::keySequence` sur la fenêtre pour chaque entrée → compteur de déclenchement de la
  commande par `Commands::triggered`) ; `noSingleKeyShortcutWhileTyping` (texte en cours d'édition, Suppr → l'objet
  existe toujours) ; `helpPageListsEveryShortcut` (`:/actcraft/help/raccourcis.html` contient chaque séquence de
  `commands.json`, en texte natif de `QKeySequence::toString(QKeySequence::NativeText)`).
- [ ] **Étape 2 : données** ; si `helpPageListsEveryShortcut` échoue faute d'entrée dans la page, la page est corrigée
  **ici** (`ui/help/raccourcis.html` ajouté à la ligne de la table par l'orchestrateur ; aucune autre tâche de V12 ne
  la touche).
- [ ] **Étape 3 : vert**, preuve de tâche ; commit « Lot 11 : raccourcis clavier complets (L11-T23) ».

<a id="tache-l11-t24"></a>
### Tâche L11-T24 — Accessibilité

**Objectif.** Étendre aux boîtes de tous les lots et à la mise en page étroite ce que `tst_ui_shell` prouve pour la
fenêtre principale (`renderedColorsMeetWcagAa`, `keyboardFocusIsVisible`) : contraste ≥ 4,5:1 (texte normal) et ≥ 3:1
(texte de 18,66 px gras ou 24 px, bordures de contrôles), focus visible au clavier, nom accessible pour chaque bouton
(`Accessible.name`, lu par `QAccessible`).

**Fichiers.** `tests/tst_ui_accessibility.cpp`, `tests/support/wcag.h` (calcul de luminance relative et de contraste,
extrait du test de `tst_ui_shell` sans le modifier : copie de la fonction, `tst_ui_shell` garde la sienne), et les QML
corrigés (liste au rapport de l'étape 2).

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_accessibility|tst_ui_shell'`.

**Interdits.** Aucun test existant affaibli ; aucune couleur hors des jetons de `Theme.qml` ; aucune correction qui
change la disposition Windows en largeur ≥ 960.

**Terminé quand** les 4 fonctions passent et `tst_ui_shell` reste `Passed`.

**Étapes.**
- [ ] **Étape 1 : test** `tst_ui_accessibility.cpp` (dans `initTestCase` : réglages d'essai en INI temporaire, et
  `qputenv("ACTCRAFT_UPDATES_URL", <URL file: de tests/data/updates/newer.json>)` : la boîte des mises à jour ne fait
  aucune requête réseau) : `everyDialogMeetsAa` (pour chaque commande `opensDialog`
  disponible sous Windows (L11-T9) : déclencher, attendre la boîte, rendre la fenêtre par `grabWindow`, pour chaque
  `Text` visible comparer la couleur du texte au pixel de fond relevé à côté de son rectangle) ;
  `focusVisibleInEveryDialog` (Tab parcourt chaque contrôle ; l'élément focalisé montre l'anneau de `Theme`, forme de
  `keyboardFocusIsVisible`) ; `everyButtonHasAnAccessibleName` (`QAccessible::queryAccessibleInterface(item)->text(QAccessible::Name)`
  non vide pour chaque `CommandButton`, `Button`, `ToolButton` visible de la fenêtre, des boîtes et du tiroir) ;
  `narrowLayoutMeetsAa` (800 × 1280, `touch-ui` vrai).
- [ ] **Étape 2 : rouge relevé** : la liste des `FAIL!` regroupée par fichier QML, au rapport ; l'orchestrateur la
  recopie dans la ligne de la table.
- [ ] **Étape 3 : corrections** (jetons de `Theme.qml`, `Accessible.name: qsTr(…)`, anneau de focus) ; vert ; banc sans
  écran ; preuve de tâche ; `NOMS OK` ; commit « Lot 11 : accessibilité des boîtes et de la mise en page étroite
  (L11-T24) ».

<a id="tache-l11-t25"></a>
### Tâche L11-T25 — Licences tierces

**Objectif.** Décision 34 du cadrage, N1-A10, H6-6b. `THIRD_PARTY_NOTICES.md` couvre **tout** ce qui est livré dans
l'installateur et l'APK 1.0.0 :
- Qt 6.12 (LGPLv3, liaison dynamique) : un paragraphe par module de `packaging/qt-autorises.txt` ;
- Qt PDF et PDFium (BSD-3-Clause) avec les composants que Qt déclare pour lui (liste lue dans les fichiers
  `qt_attribution.json` de l'installation `C:\Qt\6.12.0\msvc2022_64`, recopiée en nos mots : nom, version, licence
  SPDX, URL du projet) ;
- Qt Multimedia et FFmpeg (LGPL-2.1-or-later), si les bibliothèques FFmpeg sont déployées (relevé dans le dossier
  déployé) ;
- Hunspell selon la décision de Jérôme H6-6b (MPL-1.1 recommandée au lot 6, ou LGPL en liaison dynamique) et le
  dictionnaire français (licence du dictionnaire retenu en H6-6) ;
- miniz (MIT), runtime MSVC (licence de redistribution de Microsoft) ;
- chaque licence SPDX citée a son texte sous `LICENSES/<SPDX>.txt`.
`scripts/verifier-licences.ps1` contrôle aussi les bibliothèques **non Qt** déployées (`*.dll` hors `Qt6*`, `*.so`
hors `libQt6*` de l'APK) contre `packaging/autres-autorises.txt` (nouveau ; une ligne par bibliothèque : motif de nom,
licence SPDX) : toute bibliothèque absente → `LICENCES EN ECHEC : <noms>`.
**Format des lignes (amendement 3)** : `<motif glob> <expression SPDX>`, le premier blanc sépare les deux champs
(`vcruntime140*.dll MSVC-Redist`, `libfreetype.*.dylib FTL OR GPL-2.0-or-later`) ; ligne vide ou commençant par `#`
ignorée. `scripts/verifier-licences-mac.sh` (L11-T29m) lit le même fichier ; L11-T29m y ajoute les bibliothèques du
bundle macOS.

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R tst_app_notices` ; puis
`pwsh -NoProfile -File ./scripts/paquet.ps1` (qui appelle `verifier-licences.ps1`) et
`pwsh -NoProfile -File ./scripts/verifier-licences.ps1 -Apk build/paquet/ActCraft-<version>-arm64-v8a.apk`
(`LICENCES OK` les deux fois).

**Préalable (H6-6b, amendement 1, m18).** Si la décision de Jérôme H6-6b (licence de Hunspell) n'est pas écrite dans
le fichier d'état du chantier, la tâche s'arrête **avant toute écriture**, le signale à l'orchestrateur et rend la
main ; aucune licence supposée.

**Interdits.** Aucun texte de licence inventé (textes SPDX officiels) ; aucun module Qt sous GPL seule ; aucune
affirmation juridique (N1-8 : non validé par un juriste, dit dans le fichier).

**Terminé quand** les 4 fonctions passent et `LICENCES OK` deux fois.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_app_notices.cpp` (lit les fichiers du dépôt par `ACTCRAFT_SOURCE_DIR`, définition de
  compilation déjà posée par `tests/app.cmake` ou ajoutée par L11-T1) : `everyAllowedQtModuleHasANotice` ;
  `everyOtherLibraryHasANotice` (chaque ligne de `autres-autorises.txt` et chaque dossier de `third_party/`) ;
  `everyCitedLicenseHasItsText` (identifiants SPDX du fichier → `LICENSES/<id>.txt` existe) ;
  `aboutShowsTheSameNotices` (`AppInfo::notices()` égale le contenu du fichier : la ressource est à jour).
- [ ] **Étape 2 : relevés** (dossier déployé par `paquet.ps1 -SansConstruire` après une construction release, APK) ;
  notices, textes, listes.
- [ ] **Étape 3 : script** `verifier-licences.ps1` ; essai rouge : retirer une ligne de `autres-autorises.txt` →
  `LICENCES EN ECHEC` ; `git -C <wt> checkout -- packaging/autres-autorises.txt` ; `LICENCES OK`.
- [ ] **Étape 4 : vert**, preuve de tâche ; `NOMS OK` ; commit « Lot 11 : licences tierces complètes et contrôlées
  (L11-T25) ».

<a id="tache-l11-t26"></a>
### Tâche L11-T26 — S19 rejoué sur toutes les commandes

**Objet lu.** L'instance `Commands` de `Main.qml` a `objectName: "commands"` (`ui/qml/Main.qml:56-58`) : le
joueur la lit sous ce nom (`commands.lockedCount`), jamais sous son `id` `appCommands`.

**Objectif.** S19 (amendement 1, § 7), rejoué sur les commandes des lots 1 à 11 : chaque entrée de `commands.json`
déclare **explicitement** `lockable` et `restrictable` (aucune valeur par défaut tacite) ; sous verrou, exactement les
commandes `lockable` disponibles sur la plateforme sont grisées ; le compte est exact ; le verrou survit au
relancement ; le mot de passe n'est jamais en clair dans les réglages.

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_lockall|tst_captures_classe_protegee_complete|tst_ui_classroomlock'`.

**Preuve visuelle.** `captures.ps1 -Scenario classe-protegee-complete -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "commands.lockedCount=<K>"` :
`LU commands.lockedCount=<K> OK`, `RELANCE`, `LU appClassroom.locked=true OK`, `CAPTURES OK` ; décrire
`verrouille.png` ; puis, mot de passe absent de l'INI du lancement (sortie vide attendue) :
```powershell
Get-ChildItem -Recurse .tmp/captures/classe-protegee-complete-logiciel/reglages -Filter *.ini | Select-String -SimpleMatch -Pattern 'essai-1234','ZXNzYWktMTIzNA=='
```

**Interdits.** Aucun attribut changé sans raison écrite au rapport (une commande rendue non verrouillable doit l'être
dans l'original ou par une règle du lot 6) ; aucun mot de passe d'essai autre que celui du scénario.

**Terminé quand** les 5 fonctions et le scénario passent, et K est écrit au rapport avec son calcul.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_lockall.cpp` : `everyCommandDeclaresBothAttributes` (lecture brute du JSON :
  chaque objet a les clés `lockable` et `restrictable`, booléennes) ; `lockGreysExactlyTheLockables` (verrou posé :
  pour chaque commande disponible sous Windows, `isEnabled` faux ⇔ `lockable`) ; `lockedCountMatchesTheData`
  (`Commands::lockedCount()` = nombre calculé depuis le JSON pour la plateforme) ; `unlockRestoresOwnState` (une
  commande désactivée avant le verrou reste désactivée après, `isOwnEnabled`) ; `androidCountMatchesTheData` (registre
  filtré pour Android).
- [ ] **Étape 2 : données** (attributs manquants des lots 7 à 11 renseignés, liste au rapport) ; **K** = sortie de
  `lockedCountMatchesTheData` (affichée par `qInfo`), écrit dans le scénario.
- [ ] **Étape 3 : scénario** `classe-protegee-complete.json` (forme de `classe-protegee.json`, L6-T31, avec ses
  `waitFor`) : `classroom.lock` → création « essai-1234 » deux fois → `waitFor appClassroom.hasPassword = true` →
  `expect appClassroom.locked = true` → `expect commands.lockedCount = <K>` → capture `verrouille` → mauvais mot de
  passe → `waitFor appClassroom.busy = false` → `expect appClassroom.locked = true` → `{"restart": true}` →
  `waitReady` → `expect appClassroom.locked = true` → `expect commands.lockedCount = <K>` → bon mot de passe →
  `waitFor appClassroom.locked = false`.
- [ ] **Étape 4 : vert**, preuve visuelle et contrôle de l'INI, preuve de tâche ; commit « Lot 11 : mode classe protégé
  rejoué sur toutes les commandes (L11-T26) ».

<a id="tache-l11-t27"></a>
### Tâche L11-T27 — S32 : les cinq profils de bout en bout

**Lot 7 absent (amendement 3, H8-7).** Sur la tête du lot (`feat/livraison-complete-l11`), le lot 7 n'est pas fusionné :
aucun épinglage du lot 7 n'est créé ni inventé ; `lot7And8PinsPresent` vérifie les épinglages du lot 8 seuls, et la
liste du lot 7 est écrite au rapport « à raccorder à la fusion du lot 7 ». Exécution sur le Mac : règles
`~/pilotage-mac/regles-tache-lot11-mac.md` ; `NOMS OK` et contrôle des titres du corpus : restes pour le PC.

**Objectif.** S32 (amendement 1, § 7 ; N2-6 amendée). Pour chacun des profils `en-classe`, `preparation`,
`lecture-ecriture`, `calcul-geometrie`, `son-image` (L6-T6) : activé depuis un profil vierge, chaque outil qu'il
épingle (ceux des lots 7 et 8 compris) est présent dans la palette et la barre de raccourcis (modèle de disposition) ;
les navigateurs et réglages qu'il fixe sont appliqués ; sous Android, aucune commande absente de la plateforme n'y
figure. Fonction pure `actions::layoutForPlatform(const Profile&, const CommandRegistry&, Platform)` (dans
`profile.h`) si le lot 6 n'en a pas (A7) : la disposition effective d'un profil pour une plateforme.

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_profilesall|tst_captures_profils_complets|tst_ui_profiles'`
(avec `ACTCRAFT_REFERENCE_NAMES`, comme L6-T6).

**Preuve visuelle.** `captures.ps1 -Scenario profils-complets -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "appProfile.currentId=son-image"` :
pour chaque profil `LU appProfile.currentId=<id> OK` et `LU toolPalette.pinnedCount=<n> OK` (n exact par profil),
`CAPTURES OK` ; décrire `calcul-geometrie.png` et `son-image.png`.

**Interdits.** Aucun nom de profil de l'original ; aucun épinglage inventé : une commande absente du registre est
retirée du profil et listée au rapport ; aucune commande Windows seule dans la disposition Android.

**Terminé quand** les 5 fonctions et le scénario passent, `namesAreOurs` en `PASS`.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_profilesall.cpp` : `everyPinnedCommandExists` ; `activatedFromBlankShowsEveryPin`
  (profil vierge, puis `profile.activate <id>` : chaque épinglage dans le modèle de la palette ou de la barre) ;
  `browsersAndSettingsApplied` (valeurs du profil relues dans `Preferences` et l'état du panneau) ;
  `noAndroidAbsentCommandOnAndroid` (`layoutForPlatform(…, Platform::Android)` ne contient aucune commande dont
  `platforms` exclut Android) ; `lot7And8PinsPresent` (liste des épinglages des lots 7 et 8 relevée dans
  le rapport des épinglages du lot 7, dont L11-T0 écrit le chemin réel — ou « absent : liste lue dans les profils du
  lot 7 » —, et son équivalent du lot 8, recopiée dans le test).
- [ ] **Étape 2 : implémentation** et corrections des profils.
- [ ] **Étape 3 : scénario** `profils-complets.json` : `waitReady` ; pour chaque profil : `command profile.activate
  <id>` → `expect appProfile.currentId = <id>` → `expect toolPalette.pinnedCount = <n>` (n calculé par le test, écrit
  ici ; propriété ajoutée à `ToolPalette.qml` si absente → fichier ajouté à la ligne par l'orchestrateur) → capture
  `<id>`.
- [ ] **Étape 4 : vert**, preuve visuelle, preuve de tâche ; `NOMS OK` ; contrôle des titres du corpus vide ; commit
  « Lot 11 : les cinq profils contrôlés de bout en bout (L11-T27) ».

<a id="tache-l11-t28"></a>
### Tâche L11-T28 — Version 1.0.0, `CHANGELOG.md`, `README.md`

**Objectif.** D11-16 ; décision 31 du cadrage ; décision 14 (guide de démarrage).
- `CMakeLists.txt` : `VERSION 1.0.0` (le `versionCode` en découle, règle de `app/CMakeLists.txt:26-35`) ;
- `CHANGELOG.md` : section `## 1.0.0 — <date de la tâche>` en tête, en français, pour un enseignant : ce que le lot 11
  apporte (toucher, deux utilisateurs, tablette, reprise, aide, mises à jour, accessibilité), puis un paragraphe
  « Depuis la 0.1.0 » qui résume les sections précédentes sans les recopier ;
- `README.md` : section « Démarrer » : installer sous Windows (installateur par utilisateur, avertissement SmartScreen
  N1-4), sous Android (APK, autoriser l'installation depuis cette source), premiers pas (écrire, page suivante,
  enregistrer), F1, modes du toucher, deux utilisateurs, où sont les réglages, reprise après interruption, rechercher
  les mises à jour ; aucune capture de l'original ;
  **macOS (amendement 3)** : sous-section « Installer sous macOS » (Mac Apple Silicon) : ouvrir
  `ActCraft-<version>-macos-arm64.dmg`, glisser ActCraft dans Applications ; « non signé : clic droit > Ouvrir au premier
  lancement » (macOS 14 et avant ; dès macOS 15, ouvrir une fois puis Réglages Système > Confidentialité et sécurité >
  « Ouvrir quand même ») ; effet de Gatekeeper, en nos mots : macOS marque tout fichier téléchargé (attribut
  `com.apple.quarantine`) et refuse d'ouvrir une application marquée qui n'est pas notarisée par Apple ;
  `xattr -dr com.apple.quarantine /Applications/ActCraft.app` retire la marque (à faire seulement pour un paquet
  téléchargé depuis la page des releases du dépôt) ;
- `CHANGELOG.md`, section 1.0.0 (amendement 3) : une ligne « Paquet macOS (Mac Apple Silicon), non signé : clic droit >
  Ouvrir au premier lancement » ; les notes de version, tirées de cette section par `notes-version.ps1`, la reprennent ;
- `tests/tst_app_versioninfo.cpp` et `tests/app.cmake` (définitions `ACTCRAFT_VERSION_CODE` et `ACTCRAFT_SOURCE_DIR`
  passées au test) ; le test `tst_app_version` existant (`tests/app.cmake:4-5`, `actcraft --version`) est gardé.

**Test et commande bornée.** Commande ciblée `-R tst_app_version` (attrape `tst_app_version` et `tst_app_versioninfo`) ; puis
```powershell
pwsh -NoProfile -Command "Set-Location '<wt>'; . ./scripts/version-projet.ps1; Get-ActCraftVersion; Get-ActCraftVersionCode"
pwsh -NoProfile -File ./scripts/notes-version.ps1 -Version 1.0.0 -Sortie .tmp/notes-1.0.0.md
```
Attendu : `1.0.0`, `10000`, `NOTES OK : .tmp/notes-1.0.0.md`.

**Interdits.** Aucun tag ; aucune mention d'une release publiée ; aucun nom de l'original hors du nom du format
`.flipchart` ; aucun nom de paperboard réel ; aucun titre de vidéo.

**Terminé quand** les 4 fonctions de `tst_app_versioninfo` passent, `tst_app_version` aussi, `10000`, `NOTES OK`, `NOMS OK`, contrôle des titres du corpus vide.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_app_versioninfo.cpp` :
  ```cpp
  #include <actcraft/ui/appinfo.h>

  #include <QFile>
  #include <QtTest>

  class TstAppVersionInfo : public QObject
  {
      Q_OBJECT
  private slots:
      void versionIs100() { QCOMPARE(actcraft::ui::AppInfo().version(), QStringLiteral("1.0.0")); }
      void versionCodeIs10000() { QCOMPARE(ACTCRAFT_VERSION_CODE, 10000); }
      void versionCodeFollowsTheVersion()
      {
          const QStringList parts = actcraft::ui::AppInfo().version().split(QLatin1Char('.'));
          QCOMPARE(parts.size(), 3);
          const int expected = parts[0].toInt() * 10000 + parts[1].toInt() * 100 + parts[2].toInt();
          QCOMPARE(ACTCRAFT_VERSION_CODE, expected);
          QVERIFY(ACTCRAFT_VERSION_CODE > 1000); // 0.10.0 → 1000 : la mise à jour par-dessus passe
      }
      void changelogStartsWithTheVersion()
      {
          QFile f(QStringLiteral(ACTCRAFT_SOURCE_DIR "/CHANGELOG.md"));
          QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
          const QString text = QString::fromUtf8(f.readAll());
          const qsizetype first = text.indexOf(QStringLiteral("\n## "));
          QVERIFY(first >= 0);
          QVERIFY(text.mid(first + 1).startsWith(QStringLiteral("## 1.0.0 — ")));
      }
  };

  QTEST_GUILESS_MAIN(TstAppVersionInfo)
  #include "tst_app_versioninfo.moc"
  ```
  `AppInfo` : `ui/include/actcraft/ui/appinfo.h` (vérifié à `5373f04`, `version()` rend un `QString`) ; si son
  constructeur demande des arguments, reprendre la forme réelle.
- [ ] **Étape 2 : rouge vu**, puis version, journal, guide ; vert ; commandes ci-dessus.
- [ ] **Étape 3 : preuve de tâche** ; `NOMS OK` ; contrôle des titres du corpus vide ; commit « Lot 11 : version 1.0.0,
  journal des versions et guide de démarrage (L11-T28) ».

<a id="tache-l11-t29"></a>
### Tâche L11-T29 — Paquets 1.0.0 et S14 : release prête

**Objectif.** S14 (cadrage § 5), S1, S3, S5 sur la 1.0.0 ; contrôle du déploiement du greffon TLS de Windows
(`tls/qschannelbackend.dll`, nécessaire à la recherche de mises à jour) ajouté à `scripts/paquet.ps1` (échec
`PAQUET EN ECHEC : greffon TLS absent`).

**Fichiers.** `scripts/paquet.ps1`, `docs/verifications/2026-10-22-lot11-release-prete.md`.

**Commandes (dans l'ordre, chacune bornée, sorties sous `.tmp/`).**
1. `pwsh -NoProfile -File ./scripts/paquet.ps1 > .tmp/paquet.txt 2>&1` → `PAQUET OK : …\build\paquet\ActCraft-1.0.0-setup.exe`
   (taille rapportée, N1-A10) ; `LICENCES OK` dans la sortie.
2. `pwsh -NoProfile -File ./scripts/essai-installateur.ps1 > .tmp/installateur.txt 2>&1` (annoncé : fenêtre 5 s) →
   `INSTALLATEUR OK` ; `--version` → `ActCraft 1.0.0`.
3. `pwsh -NoProfile -File ./scripts/paquet-android.ps1 > .tmp/apk.txt 2>&1` → `APK OK`, `versionCode = 10000,
   versionName = 1.0.0` ; `verifier-licences.ps1 -Apk …` → `LICENCES OK` ; tailles rapportées.
4. Local : `apksigner verify --print-certs build/paquet/ActCraft-1.0.0-arm64-v8a.apk` → certificat de débogage
   (`CN=Android Debug`).
5. Émulateur : `essai-android.ps1 -Apk build/paquet/ActCraft-1.0.0-x86_64.apk` → `ANDROID OK` ;
   `essai-android-fichiers.ps1 -Apk …` → `ANDROID FICHIERS OK`.
6. Essai à blanc de `release.yml` (sans tag) : depuis le worktree du chantier,
   `git -C <wt> push origin HEAD:refs/heads/essai-ci/release-lot11` ; puis
   `gh run list --branch essai-ci/release-lot11 --workflow release.yml --limit 1` ; `gh run watch <id>` ;
   `gh run view <id> --log > .tmp/release-essai.txt` ;
   `Select-String -Path .tmp/release-essai.txt -Pattern 'APK NON PUBLIABLE|NOTES OK|SHA256SUMS|SECRETS ABSENTS|CONTROLE OK|DMG OK|ESSAI DMG OK|ActCraft-1\.0\.0-(setup\.exe|arm64-v8a\.apk|macos-arm64\.dmg)' | Select-Object -Last 20`
   → tous les motifs présents (`DMG OK`, `ESSAI DMG OK` et le nom du `.dmg` viennent du job `macos`, amendement 3) ; `SECRETS ABSENTS` et `CONTROLE OK` viennent du job `controle-mode-reel`
   (`release.yml:110-147`) : c'est la preuve de S14 que le mode réel, sans secrets, échoue avec son message.
7. Aucune release créée : `gh release list --limit 5` (lecture) → aucune entrée `v1.0.0`.
8. `git -C <wt> push origin --delete essai-ci/release-lot11`.

**Interdits.** **Aucun tag** (`git tag`, `git push … v1.0.0`, `--tags`, `--follow-tags`) ; aucune commande
`gh release create|edit|upload|delete` ; aucun appel d'API en écriture sur les releases ou les tags ; aucun
`gh workflow run release.yml -f mode=reel` (le job `secrets` référence l'environnement `release`, que GitHub créerait
alors sans la règle des tags `v*`) ; aucune création du
fichier go ; aucun secret lu, posé ni affiché ; aucun téléversement de paquet ; aucun paquet distribué hors du poste.

**L11-T29m (amendement 3).** L11-T29 dépend de L11-T29m (V15b après V15) : le run d'essai contient le job `macos`
(`DMG OK`, `ESSAI DMG OK`, le nom du `.dmg` ; son lancement de 3 s est informatif). Sur le Mac, aucun push : les points 6
à 8 sont des restes pour le PC (ou de l'orchestrateur).

**Terminé quand** les huit points sont verts et le rapport `…-release-prete.md` est commité :
noms, tailles et SHA-256 des deux paquets locaux, numéros des runs, lignes relevées.

**Étapes.**
- [ ] **Étape 1 :** contrôle du greffon TLS dans `scripts/paquet.ps1` ; essai rouge (renommer temporairement le dossier
  `tls` du déploiement de `build/` → `PAQUET EN ECHEC : greffon TLS absent`), remise en place, `PAQUET OK`.
- [ ] **Étape 2 :** points 1 à 5.
- [ ] **Étape 3 :** points 6 à 8.
- [ ] **Étape 4 :** rapport ; `git add scripts/paquet.ps1 docs/verifications/2026-10-22-lot11-release-prete.md` ;
  `NOMS OK` ; commit « Lot 11 : paquets 1.0.0 et flux de release essayé à blanc (L11-T29) ».

<a id="tache-l11-t29m"></a>
### Tâche L11-T29m — Paquet macOS : `.app` déployé par `macdeployqt` et `.dmg` non signé, 1.0.0

**Objectif.** Amendement 3, volet 1 (demande du pilote, 2026-11-06, sous la délégation de Jérôme du 2026-11-02) ;
D11-15 et N2-13 rouverts (H11-19 : à revoir par Jérôme ; le `.dmg` est un artefact, sa publication attend le go de
release) ; S14. Sur le Mac Apple Silicon, une commande bash produit `build/paquet/ActCraft-<version>-macos-arm64.dmg` : construction Release de la cible
`actcraft` (bundle `MACOSX_BUNDLE`, `app/CMakeLists.txt:1`), copie en `ActCraft.app`, déploiement de Qt par
`macdeployqt` (Qt Homebrew : `$(brew --prefix qt)/bin/macdeployqt`, ou `$ACTCRAFT_QT_DIR/bin/macdeployqt`), retrait
de Qt WebEngine (greffon `libqtwebview_webengine.dylib` et frameworks que plus rien ne référence : Qt WebView passe par le
greffon natif `darwin`), `qt.conf` contrôlé, notices dans le bundle, contrôle des licences (frameworks Qt **et** bibliothèques non Qt), signature **ad hoc** (obligatoire sur
arm64 : `macdeployqt` réécrit les chemins des binaires, ce qui casse leur signature, et macOS tue un binaire arm64 non
signé), image disque par `hdiutil create`. Construction refusée si `ACTCRAFT_MEDIA` ou `ACTCRAFT_WEBFRAME` vaut `OFF`.
Un essai monte l'image, vérifie le bundle, `qt.conf` et `--version`, lance l'application 3 s sous
`DYLD_PRINT_LIBRARIES=1` (greffons compris), démonte. Le job `macos` de `release.yml` fait la même chose sur le runner du
Mac mini si le dépôt est privé, sur `macos-latest` s'il est public ; le lancement y est informatif.

**Fichiers.** Ceux de la ligne de la table :
- neufs : `scripts/paquet-mac.sh`, `scripts/essai-dmg.sh`, `scripts/verifier-licences-mac.sh` (exécutables, mode
  `100755` dans git comme `scripts/bench.sh`), `docs/verifications/2026-11-07-lot11-paquet-macos.md` ;
- modifiés : `app/CMakeLists.txt` (nom du bundle), `packaging/autres-autorises.txt` (bibliothèques du bundle),
  `packaging/qt-autorises.txt` (frameworks propres à macOS, licence vérifiée), `THIRD_PARTY_NOTICES.md` et
  `LICENSES/<SPDX>.txt` (notices et textes des bibliothèques du bundle), `.github/workflows/release.yml` (job `macos`).

**Interfaces.**
- Consomme : `project(ActCraft VERSION x.y.z)` de `CMakeLists.txt` (1.0.0, L11-T28) ; `packaging/qt-autorises.txt`
  (un nom `Qt6<Module>` par ligne, `#` = commentaire) ; `packaging/autres-autorises.txt` (L11-T25, format de
  l'amendement 3 : `<motif glob> <expression SPDX>`) ; `THIRD_PARTY_NOTICES.md` et `LICENSES/` (L11-T25) ;
  `actcraft --version` → `ActCraft <version>` sur la sortie standard, avant toute fenêtre (`app/main.cpp:28-36`) ;
  `--quitter-apres <ms>` (`app/main.cpp:79-83`).
- Produit :
  - `scripts/verifier-licences-mac.sh <ActCraft.app>` → dernière ligne `LICENCES OK` (0) ou `LICENCES EN ECHEC : <noms>` (1) ;
  - `scripts/paquet-mac.sh [--sans-construire]` → `DMG OK : <chemin absolu du .dmg>` (0) ou `DMG EN ECHEC : <raison>` (1) ;
  - `scripts/essai-dmg.sh [<image.dmg>] [--lancer]` → `ESSAI DMG OK` (0) ou `ESSAI DMG EN ECHEC : <raison>` (1) ;
  - job `macos` de `release.yml`, artefact `paquet-macos` (mode réel seulement), attendu par le job `release`.

**Patron à ne pas reprendre (concepteur-critique).**
1. Transposer `scripts/verifier-licences.ps1:21-26` : son motif `^(?:lib)?(Qt6…)\.(?:dll|so)$` ne voit **aucun** module
   sur macOS (les modules y sont des `QtCore.framework`, sans « 6 ») et ne regarde pas les autres bibliothèques. Ici :
   `Qt<Nom>.framework` → `Qt6<Nom>` contre `qt-autorises.txt`, et chaque `*.dylib` de `Contents/Frameworks` contre
   `autres-autorises.txt`, sans GPL seule.
2. Prouver le paquet par un lancement sur le Mac qui l'a construit (forme de `scripts/paquet.ps1:28-58` suivie d'un
   essai) : Homebrew est installé ici, donc une dépendance que `macdeployqt` n'a pas copiée (chemin absolu
   `/opt/homebrew/…` resté dans un binaire) se charge quand même, et `--version` passe sur un bundle incomplet. La preuve
   est double : aucun chemin `/opt/homebrew` ni `/usr/local` dans `otool -L` de tout binaire du bundle, et aucune
   bibliothèque chargée hors du bundle monté et du système (`DYLD_PRINT_LIBRARIES=1`), à `--version` **et au
   lancement** : `--version` sort avant `QGuiApplication` (`app/main.cpp:30-36`), les greffons (cocoa, images, webview,
   QML) ne se chargent qu'au lancement. Contre-épreuves : `otool` voit les chemins Homebrew du `.app` non déployé, `dyld`
   les voit au lancement du `.app` non déployé (étape 6) ; l'essai exige `libqcocoa.dylib` chargé depuis le bundle.
Chemins de référence : `scripts/paquet.ps1:28-58` (ordre : déploiement, notices `:46-48`, licences `:50-51`, paquet,
verdict `:58`), `scripts/verifier-licences.ps1:21-37`, `.github/workflows/ci.yml:69-91` (gardes du runner du Mac) et
`:138-174` (Qt, Ninja et ccache du Mac), `.github/workflows/release.yml:159-224` (job `windows`), `:331-387` (job
`release`, liste `$attendus` à la ligne 352), `:389-391` (job `nettoyer`) ; règle d'exécuteur de `ci.yml:10-14,61`
(`runner_macos`) ; installation hébergée de Qt `ci.yml:117-136`.

**Test et commande bornée.** Aucune entrée ctest neuve (N reste N11 + 32). Preuve par script, chaque commande sous le
jeton `tests` (forme unique des règles du Mac, `<id>` = `L11-T29m`), au premier plan, `timeout: 600000` :
- construction, en trois passages de moins de 10 min chacun (trois appels du jeton) :
  `<commande>` = `cmake --build build/macos-paquet --parallel 2 --target actcraft_core actcraft_formats > .tmp/paquet-mac-build1.txt 2>&1`,
  puis `<commande>` = `cmake --build build/macos-paquet --parallel 2 --target actcraft_render actcraft_tools actcraft_ui > .tmp/paquet-mac-build2.txt 2>&1`,
  puis `<commande>` = `cmake --build build/macos-paquet --parallel 2 --target actcraft > .tmp/paquet-mac-build3.txt 2>&1` ;
  lire `tail -5` de chaque fichier (échec : `error:` ou `FAILED:`) ;
- paquet : `<commande>` = `scripts/paquet-mac.sh --sans-construire > .tmp/paquet-mac.txt 2>&1`, puis `tail -4 .tmp/paquet-mac.txt`
  → `Taille : …`, `SHA-256 : …`, `DMG OK : <wt>/build/paquet/ActCraft-1.0.0-macos-arm64.dmg` ;
- essai (sans jeton : ni construction ni ctest) : `cd <wt> && scripts/essai-dmg.sh --lancer > .tmp/essai-dmg.txt 2>&1; tail -4 .tmp/essai-dmg.txt`
  → `VERSION OK : ActCraft 1.0.0`, `LANCEMENT OK`, `ESSAI DMG OK` ;
- notices : `<commande>` = `cmake --build build-mac -j2 > .tmp/build.txt 2>&1 && QT_QPA_PLATFORM=offscreen ctest --test-dir build-mac -R '^tst_app_notices$' --output-on-failure --timeout 120 > .tmp/ctest.txt 2>&1`,
  puis `tail -5 .tmp/ctest.txt` → `100% tests passed, 0 tests failed out of 1` ;
- YAML (job `macos`, `release` et `nettoyer` qui l'attendent, `runs-on` selon la visibilité, téléversement sous un
  `if` du mode `reel`, `.dmg` attendu par `release`) :
  `cd <wt> && ruby -ryaml -e 'j = YAML.load_file(".github/workflows/release.yml")["jobs"]; m = j["macos"] or abort "macos absent"; %w[release nettoyer].each { |n| abort "#{n} sans macos" unless j[n]["needs"].include?("macos") }; r = m["runs-on"].to_s; abort "runs-on hors règle (visibilité)" unless r.include?("github.event.repository.private") && r.include?("self-hosted") && r.include?("macos-latest"); u = m["steps"].find { |s| s["uses"].to_s.start_with?("actions/upload-artifact@") } or abort "upload-artifact absent"; abort "upload-artifact hors mode reel" unless u["if"].to_s.include?("reel"); abort "release n'attend pas le .dmg" unless j["release"]["steps"].any? { |s| s["run"].to_s.include?("macos-arm64.dmg") }; puts "YAML OK"'`
  → `YAML OK`.

**Preuve visuelle.** Aucune capture : la fenêtre de `--lancer` (3 s, annoncée d'une ligne à l'orchestrateur avant) sert
au code de sortie 0 et au journal `.tmp/essai-dmg-dyld-lancement.txt`. L'apparence du premier lancement sous Gatekeeper est une mesure de Jérôme (L11-T30,
MacBook Air M1).

**Contraintes (recopiées de l'en-tête et des amendements).**
- Exécution sur le Mac selon `~/pilotage-mac/regles-tache-lot11-mac.md` (dossier `<wt>`, chaque commande commence par
  `cd <wt> &&`, jeton `tests`, une construction à la fois, `-j2`, jamais de `sleep`) ; branche du lot
  `feat/livraison-complete-l11`.
- N = N11 + 32 exactement : aucun test ajouté à ctest (L11-T1 a réservé les 32).
- Dossier de construction du paquet : `build/macos-paquet` (Release), distinct de `build-mac` (Debug des tests).
- Propriété intellectuelle : aucune ressource de Promethean ; `LISEZ-MOI.txt`, notices et textes du README en nos mots ;
  textes de licence = textes SPDX officiels (`https://github.com/spdx/license-list-data`, dossier `text/`), jamais
  réécrits ; aucune affirmation juridique (N1-8 : « non validé par un juriste », déjà dit dans `THIRD_PARTY_NOTICES.md`).
- Commit : fichiers nommés (`git add <chemins>`, jamais `-A`, `.`, `commit -a`), message en français terminé par
  `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>` ; pas de push.
- Restes pour le PC (au rapport) : `NOMS OK`, contrôle des titres du corpus, essai à blanc de `release.yml` (points 6 à 8
  de L11-T29, qui dépend de cette tâche).
- Mutation : la remise prescrite ici (copie prouvée par `cmp`) l'emporte sur la règle Mac (`git checkout`), qui
  effacerait les ajouts non commités de l'étape 7.

**Interdits.** **Aucun tag** (`git tag`, `--tags`, `--follow-tags`) ; aucune commande `gh release` ni `gh workflow run` ;
aucun push (pas même d'une branche `essai-ci/…`) ; aucun fichier go ; aucun secret lu, posé ni affiché ; aucun
téléversement ni distribution du `.dmg` hors du poste. Aucun certificat Developer ID, aucune notarisation
(`notarytool`, `altool`), aucun `spctl --master-disable`. Aucune bibliothèque sous GPL seule dans le bundle : si
`verifier-licences-mac.sh` en trouve une, arrêt, rapport et main rendue (décision de Jérôme, comme H6-6b) ; de même pour
un `QtWebEngine*` resté dans le bundle. `runs-on` du job `macos` : la règle de visibilité seule (privé → Mac mini, public
→ `macos-latest`), jamais `macos-latest` sur un dépôt privé (minutes payantes), jamais d'arrêt sur « public ». Aucune
modification de `scripts/paquet.ps1` (L11-T29, vague suivante).

**Terminé quand** `LICENCES OK`, `DMG OK`, `ESSAI DMG OK` (avec `VERSION OK` et `LANCEMENT OK`, journaux `dyld` sans
chemin hors du bundle), les deux contre-épreuves de l'étape 6, la mutation rouge puis remise, `tst_app_notices` vert, `YAML OK`, la suite complète `100% tests passed … out of N` (N = N11 + 32) et le
rapport `docs/verifications/2026-11-07-lot11-paquet-macos.md` commité : nom, taille et SHA-256 du `.dmg`, liste des
frameworks Qt et des bibliothèques non Qt du bundle (nom, formule Homebrew, version, licence SPDX), lignes relevées.

**Étapes.**
- [ ] **Étape 1 : nom du bundle.** Dans `app/CMakeLists.txt`, `set_target_properties(actcraft PROPERTIES` (ligne 21),
  ajouter après `MACOSX_BUNDLE_GUI_IDENTIFIER org.actcraft.ActCraft` :
  ```cmake
      # Nom affiché par macOS (barre des menus, Finder) ; le modèle Info.plist.in le lit (CFBundleName).
      MACOSX_BUNDLE_BUNDLE_NAME ActCraft
  ```
- [ ] **Étape 2 : contrôle des licences**, `scripts/verifier-licences-mac.sh` :
  ```bash
  #!/usr/bin/env bash
  # Licences d'un ActCraft.app déployé (décision 34 ; amendement 3 du plan du lot 11). Chaque framework Qt
  # (QtCore.framework → Qt6Core) est dans packaging/qt-autorises.txt ; chaque autre bibliothèque de Contents/Frameworks a
  # sa ligne « <motif glob> <expression SPDX> » dans packaging/autres-autorises.txt ; aucune sous GPL seule, aucun
  # Qt WebEngine. Les greffons
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
      QtWebEngine*)
        # Jamais Qt WebEngine (qt-autorises.txt, lot 8) : arrêt, quelle que soit la liste.
        refuses="$refuses $nom.framework(WebEngine)"
        ;;
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
    case "$nom" in *WebEngine*) refuses="$refuses $nom(WebEngine)"; continue ;; esac
    licence="$(licence_de "$nom")" || { refuses="$refuses $nom"; continue; }
    sans_gpl_seule "$licence" || refuses="$refuses $nom(GPL)"
    echo "  $nom : $licence"
  done
  if [ -n "$refuses" ]; then
    echo "LICENCES EN ECHEC :$refuses"
    exit 1
  fi
  echo "LICENCES OK"
  ```
  `/bin/bash` du Mac est la 3.2 : ni tableau vide sous `set -u`, ni `mapfile` ; ce script n'en emploie pas.
- [ ] **Étape 3 : paquet**, `scripts/paquet-mac.sh` :
  ```bash
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

  version="$(perl -0777 -ne 'print "$1.$2.$3" if /project\(\s*ActCraft\s+VERSION\s+(\d+)\.(\d+)\.(\d+)/' CMakeLists.txt)"
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
      echec "$option_cmake n'est pas ON dans $build/CMakeCache.txt (Qt Multimedia ou Qt WebView manquant)"
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
  erreur="$(grep '^ERROR' "$journal" | grep -v 'ActCraft\.Ui' | head -1)"
  [ -z "$erreur" ] || echec "macdeployqt : $erreur"

  # Qt WebView passe par le moteur natif de macOS (greffon darwin), jamais par Qt WebEngine (qt-autorises.txt, lot 8) :
  # macdeployqt copie pourtant les deux greffons et, avec celui de WebEngine, ses frameworks. Le greffon est retiré, puis
  # tout framework et toute bibliothèque que plus rien ne référence, jusqu'à point fixe. Il ne doit rester aucun
  # QtWebEngine* : sinon verifier-licences-mac.sh refuse le bundle (arrêt).
  rm -f "$app/Contents/PlugIns/webview/libqtwebview_webengine.dylib"
  [ -f "$app/Contents/PlugIns/webview/libqtwebview_darwin.dylib" ] || echec "greffon Qt WebView natif (darwin) absent"
  liens="$paquet/liens.txt"
  while :; do
    find "$app" -type f \( -perm -u+x -o -name '*.dylib' \) -exec otool -L {} + 2>/dev/null |
      awk '/^[^[:space:]].*:$/ { f = substr($0, 1, length($0) - 1); next } /^[[:space:]]/ { print f "\t" $1 }' > "$liens"
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

  restes="$(find "$app" -type f \( -perm -u+x -o -name '*.dylib' \) -exec otool -L {} + 2>/dev/null |
    grep -E '^[[:space:]]+(/opt/homebrew|/usr/local)/' | sort -u)"
  [ -z "$restes" ] || echec "dépendances hors du bundle : $(echo "$restes" | head -3 | tr -s '[:space:]' ' ')"
  [ "$(lipo -archs "$app/Contents/MacOS/actcraft")" = arm64 ] || echec "binaire autre que arm64"

  grep -q 'PlugIns' "$app/Contents/Resources/qt.conf" 2>/dev/null || echec "qt.conf absent ou sans chemin des greffons"
  cp LICENSE "$app/Contents/Resources/LICENSE.txt"
  cp THIRD_PARTY_NOTICES.md "$app/Contents/Resources/"
  cp -R LICENSES "$app/Contents/Resources/LICENSES"

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
  ```
  Dans le fichier, le texte entre `<<'TEXTE'` et `TEXTE` commence en colonne 1 (sans l'indentation de ce plan), et
  `TEXTE` final aussi.
- [ ] **Étape 4 : essai**, `scripts/essai-dmg.sh` :
  ```bash
  #!/usr/bin/env bash
  # Essai de l'image disque macOS (amendement 3 du plan du lot 11) : montée sans Finder, bundle complet, signature ad hoc
  # valide, qt.conf, aucune dépendance hors du bundle (otool), `actcraft --version`, lancement de 3 s si --lancer, démontée.
  # Bibliothèques réellement chargées (DYLD_PRINT_LIBRARIES) : toutes dans le bundle ou le système, à --version (cadres
  # liés) et au lancement (greffons cocoa, images, webview, QML, chargés après QGuiApplication). Journaux sous .tmp/.
  # Usage : scripts/essai-dmg.sh [<image.dmg>] [--lancer]   (--lancer : fenêtre ouverte 3 s sur l'écran du Mac)
  # Dernière ligne et code : « ESSAI DMG OK » (0) ou « ESSAI DMG EN ECHEC : <raison> » (1).
  set -u
  cd "$(dirname "$0")/.." || exit 1
  echec() {
    echo "ESSAI DMG EN ECHEC : $1"
    exit 1
  }
  version="$(perl -0777 -ne 'print "$1.$2.$3" if /project\(\s*ActCraft\s+VERSION\s+(\d+)\.(\d+)\.(\d+)/' CMakeLists.txt)"
  [ -n "$version" ] || echec "version introuvable dans CMakeLists.txt"
  dmg=""
  lancer=0
  for option in "$@"; do
    case "$option" in
      --lancer) lancer=1 ;;
      -*) echec "option inconnue : $option" ;;
      *) dmg="$option" ;;
    esac
  done
  [ -n "$dmg" ] || dmg="build/paquet/ActCraft-$version-macos-arm64.dmg"
  [ -f "$dmg" ] || echec "image absente : $dmg"

  mkdir -p .tmp
  # Chemin réel (/private/tmp) : c'est celui qu'écrit dyld.
  point="$(cd "$(mktemp -d /tmp/actcraft-dmg.XXXXXX)" && pwd -P)"
  monte=0
  fin() {
    if [ "$monte" = 1 ]; then
      hdiutil detach "$point" -quiet || hdiutil detach "$point" -force -quiet
    fi
    rmdir "$point" 2>/dev/null
  }
  trap fin EXIT

  # Chemins chargés (journal de DYLD_PRINT_LIBRARIES) hors du bundle monté et du système ; trois au plus.
  hors_systeme() {
    awk -v b="$point/" '/^dyld\[[0-9]+\]: / { p = $NF; if (index(p, b) != 1 && index(p, "/System/") != 1 && index(p, "/usr/lib/") != 1) print p }' "$1" | head -3
  }
  hdiutil attach -nobrowse -readonly -noautoopen -mountpoint "$point" "$dmg" > /dev/null || echec "hdiutil attach"
  monte=1

  app="$point/ActCraft.app"
  exe="$app/Contents/MacOS/actcraft"
  [ -x "$exe" ] || echec "ActCraft.app/Contents/MacOS/actcraft absent"
  [ -L "$point/Applications" ] || echec "lien Applications absent"
  [ -f "$app/Contents/Resources/THIRD_PARTY_NOTICES.md" ] || echec "notices absentes du bundle"
  nom="$(plutil -extract CFBundleName raw "$app/Contents/Info.plist" 2>/dev/null)"
  [ "$nom" = ActCraft ] || echec "CFBundleName = « $nom », attendu « ActCraft »"
  grep -q 'PlugIns' "$app/Contents/Resources/qt.conf" 2>/dev/null || echec "qt.conf absent ou sans chemin des greffons"
  codesign --verify --deep --strict "$app" 2> /dev/null || echec "signature ad hoc invalide"

  restes="$(find "$app" -type f \( -perm -u+x -o -name '*.dylib' \) -exec otool -L {} + 2>/dev/null |
    grep -E '^[[:space:]]+(/opt/homebrew|/usr/local)/' | sort -u)"
  [ -z "$restes" ] || echec "dépendances hors du bundle : $(echo "$restes" | head -3 | tr -s '[:space:]' ' ')"

  sortie="$("$exe" --version)" || echec "--version : code $?"
  [ "$sortie" = "ActCraft $version" ] || echec "--version a rendu « $sortie », attendu « ActCraft $version »"
  DYLD_PRINT_LIBRARIES=1 "$exe" --version > /dev/null 2> .tmp/essai-dmg-dyld-version.txt
  grep -qF "$app/Contents/Frameworks/QtCore.framework/" .tmp/essai-dmg-dyld-version.txt ||
    echec "journal de dyld muet ou QtCore chargé hors du bundle (.tmp/essai-dmg-dyld-version.txt)"
  dehors="$(hors_systeme .tmp/essai-dmg-dyld-version.txt)"
  [ -z "$dehors" ] || echec "bibliothèques chargées hors du bundle : $(echo "$dehors" | tr -s '[:space:]' ' ')"
  echo "VERSION OK : $sortie"

  if [ "$lancer" = 1 ]; then
    echo "Fenêtre d'ActCraft ouverte 3 s sur l'écran du Mac."
    DYLD_PRINT_LIBRARIES=1 "$exe" --quitter-apres 3000 > /dev/null 2> .tmp/essai-dmg-dyld-lancement.txt ||
      echec "lancement : code $? (.tmp/essai-dmg-dyld-lancement.txt)"
    grep -qF "$app/Contents/PlugIns/platforms/libqcocoa.dylib" .tmp/essai-dmg-dyld-lancement.txt ||
      echec "greffon cocoa non chargé depuis le bundle (.tmp/essai-dmg-dyld-lancement.txt)"
    dehors="$(hors_systeme .tmp/essai-dmg-dyld-lancement.txt)"
    [ -z "$dehors" ] || echec "greffons ou bibliothèques chargés hors du bundle : $(echo "$dehors" | tr -s '[:space:]' ' ')"
    echo "LANCEMENT OK"
  fi
  echo "ESSAI DMG OK"
  ```
- [ ] **Étape 5 : rouge vu.** `chmod +x scripts/paquet-mac.sh scripts/essai-dmg.sh scripts/verifier-licences-mac.sh` ;
  `cd <wt> && scripts/essai-dmg.sh > .tmp/essai-dmg.txt 2>&1; tail -1 .tmp/essai-dmg.txt` →
  `ESSAI DMG EN ECHEC : image absente : build/paquet/ActCraft-<version>-macos-arm64.dmg`.
- [ ] **Étape 6 : construction Release.** Sous le jeton : `<commande>` =
  `cmake -S . -B build/macos-paquet -G Ninja -DCMAKE_PREFIX_PATH=$(brew --prefix qt) -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache > .tmp/paquet-mac-cfg.txt 2>&1`,
  puis les trois passages de construction de « Test et commande bornée ». **Contre-épreuves** (les contrôles ne sont
  pas aveugles), sur le `.app` non déployé :
  ```bash
  cd <wt> && find build/macos-paquet/actcraft.app -type f \( -perm -u+x -o -name '*.dylib' \) -exec otool -L {} + | grep -cE '^[[:space:]]+(/opt/homebrew|/usr/local)/'
  cd <wt> && DYLD_PRINT_LIBRARIES=1 build/macos-paquet/actcraft.app/Contents/MacOS/actcraft --version 2>&1 >/dev/null | awk '/^dyld\[[0-9]+\]: / { print $NF }' | grep -c '^/opt/homebrew/'
  ```
  → deux nombres ≥ 1, écrits au rapport.
- [ ] **Étape 7 : relevé des bibliothèques.** `scripts/paquet-mac.sh --sans-construire` (sous le jeton) : le premier
  passage finit attendu par `LICENCES EN ECHEC : <noms>` (bibliothèques Homebrew de Qt : glib, gettext, ICU, OpenSSL,
  pcre2, freetype, harfbuzz, libpng, brotli, zstd, md4c, libb2, double-conversion, et celles des greffons d'images ;
  frameworks Qt propres à macOS absents de `qt-autorises.txt`). Pour chaque bibliothèque : formule Homebrew
  (`otool -D` du fichier source sous `/opt/homebrew/opt/<formule>/lib`), version (`brew list --versions <formule>`),
  licence SPDX (`brew info --json=v2 --formula <formule>`, champ `license`, recoupé avec le fichier `COPYING` ou
  `LICENSE` de la formule) ; puis :
  - une ligne `<motif glob> <expression SPDX>` dans `packaging/autres-autorises.txt` (motif sans numéro de version
    majeure inutile, par exemple `libglib-2.0.*.dylib LGPL-2.1-or-later`), regroupées sous un commentaire
    `# Paquet macOS (L11-T29m) : bibliothèques Homebrew copiées par macdeployqt` ;
  - pour une licence double qui comprend la GPL (freetype `FTL OR GPL-2.0-or-later`, zstd `BSD-3-Clause OR
    GPL-2.0-only`), l'expression entière ; la notice dit la licence retenue (celle qui n'est pas la GPL) ;
  - un paragraphe par bibliothèque dans une section `## Bibliothèques du paquet macOS` de `THIRD_PARTY_NOTICES.md` (nom,
    version, licence SPDX, URL du projet, en nos mots) ; `LICENSES/<SPDX>.txt` pour chaque identifiant neuf ;
  - un framework Qt absent de `qt-autorises.txt` (styles macOS et iOS de Qt Quick Controls, par exemple) : licence lue
    dans la ligne SPDX d'un de ses en-têtes sous `$(brew --prefix qt)/lib/<Framework>.framework/Headers/`, ligne
    ajoutée avec un commentaire de la forme des commentaires existants ; un module sous GPL seule → arrêt (interdits) ;
  - lignes `Retiré, plus référencé : …` de la sortie (frameworks de Qt WebEngine et leurs dépendances seules) : recopiées
    au rapport ; un `QtWebEngine*` refusé (`(WebEngine)`) → arrêt (interdits).
  Rejouer jusqu'à `LICENCES OK` puis `DMG OK : …`.
- [ ] **Étape 8 : mutation.** Le fichier porte les ajouts non commités de l'étape 7 : `git checkout` les effacerait, la
  remise se fait donc par une copie, prouvée identique :
  ```bash
  cd <wt> && cp packaging/autres-autorises.txt .tmp/autres-autorises.sauve && sed -i '' '/^libglib-2\.0/d' packaging/autres-autorises.txt && scripts/verifier-licences-mac.sh build/paquet/macos/ActCraft.app | tail -1
  ```
  → `LICENCES EN ECHEC : libglib-2.0.0.dylib` ; puis
  `cd <wt> && cp .tmp/autres-autorises.sauve packaging/autres-autorises.txt && cmp packaging/autres-autorises.txt .tmp/autres-autorises.sauve && scripts/verifier-licences-mac.sh build/paquet/macos/ActCraft.app | tail -1`
  → aucune sortie de `cmp`, puis `LICENCES OK`.
- [ ] **Étape 9 : essai vert.** Annoncer d'une ligne la fenêtre de 3 s ; `scripts/essai-dmg.sh --lancer` (commande de
  « Test et commande bornée ») → `VERSION OK : ActCraft 1.0.0`, `LANCEMENT OK`, `ESSAI DMG OK` ; nombre de lignes de
  `.tmp/essai-dmg-dyld-lancement.txt` chargées depuis le bundle (`grep -c '/ActCraft.app/'`) au rapport. Puis `tst_app_notices`
  (commande ciblée) → `Passed` (les lignes ajoutées ont leur notice et leur texte).
- [ ] **Étape 10 : job `macos` de `release.yml`**, inséré entre les jobs `windows` et `android` :
  ```yaml
    # Paquet macOS non signé (amendement 3 du plan du lot 11), aucun secret. Exécuteur choisi selon la visibilité du dépôt,
    # comme runner_macos de ci.yml : privé → runner auto-hébergé du Mac mini (macos-latest y coûterait des minutes
    # payantes) ; public → macos-latest (gratuit ; un runner auto-hébergé est exclu d'un dépôt public). Aucun arrêt sur
    # « public ». Image téléversée en mode réel seulement.
    macos:
      needs: [mode, secrets]
      if: ${{ !cancelled() && needs.mode.result == 'success' && (needs.secrets.result == 'success' || (needs.secrets.result == 'skipped' && needs.mode.outputs.mode == 'essai')) }}
      runs-on: ${{ github.event.repository.private && fromJSON('["self-hosted", "macOS", "ARM64", "actcraft"]') || 'macos-latest' }}
      timeout-minutes: 60
      steps:
        # Mac mini : garde de mémoire de ci.yml:79-91 (sa garde de visibilité n'a plus d'objet ici : runs-on la tient).
        - name: Garde du runner auto-hébergé (mémoire)
          if: runner.environment != 'github-hosted'
          run: |
            dispo_go() {
              # Part libre selon macOS (mémoire compressible comprise), en Go entiers.
              memory_pressure | awk -v t="$(sysctl -n hw.memsize)" '/free percentage/ {sub("%","",$NF); printf "%d", $NF*t/100/1073741824}'
            }
            for i in $(seq 1 30); do
              go=$(dispo_go)
              if ! [ "$go" -ge 0 ] 2>/dev/null; then echo "::error::Mémoire disponible illisible (memory_pressure)"; exit 1; fi
              if [ "$go" -ge 3 ]; then echo "Mémoire disponible : $go Go"; exit 0; fi
              echo "Mémoire disponible : $go Go (< 3 Go), nouvel essai dans 30 s ($i/30)"
              sleep 30
            done
            echo "::error::Moins de 3 Go de mémoire disponible sur le Mac depuis 15 min : build macOS abandonné, à relancer."
            exit 1

        - uses: actions/checkout@11d5960a326750d5838078e36cf38b85af677262 # v4.4.0

        # macos-latest : Qt par aqtinstall, comme ci.yml:117-136, avec Qt Multimedia et Qt WebView (requis).
        - uses: actions/setup-python@a26af69be951a213d495a4c3e4e4022e16d87065 # v5.6.0
          if: runner.environment == 'github-hosted'
          with:
            python-version: "3.12"

        # aqtinstall 3.3.0 ne sait pas installer Qt 6.12 (nouveau découpage du dépôt Qt) : version de développement,
        # épinglée sur le commit vérifié par les runs du 2026-10-15 (même commit dans les trois jobs).
        - name: Installer Ninja et aqtinstall
          if: runner.environment == 'github-hosted'
          run: python -m pip install ninja "git+https://github.com/miurahr/aqtinstall@076e1659807d0b362a3ed684d54c2e9c775eb9c7"

        - name: Installer Qt
          if: runner.environment == 'github-hosted'
          shell: bash
          run: |
            python -m aqt install-qt mac desktop "$QT_VERSION" clang_64 -m qtshadertools qtimageformats qtpdf qtmultimedia qtwebview --outputdir "$RUNNER_TEMP/Qt"
            dir=$(ls -d "$RUNNER_TEMP"/Qt/"$QT_VERSION"/*/ | head -1)
            echo "ACTCRAFT_QT_DIR=${dir%/}" >> "$GITHUB_ENV"
            echo "${dir%/}/bin" >> "$GITHUB_PATH"

        # Mac mini : copie de ci.yml:138-174, sans « matrix » ; le repli aqt prend aussi qtmultimedia et qtwebview.
        # Mac mini : Qt Homebrew en lecture seule si sa version concorde, sinon Qt installé une fois dans le cache du
        # runner (hors des worktrees des sessions). ccache garde la compilation chaude d'un run à l'autre.
        - name: Qt, Ninja et ccache du Mac
          if: runner.environment != 'github-hosted' && runner.os == 'macOS'
          shell: bash
          run: |
            cache="$HOME/actions-runner-actcraft/_cache"
            mkdir -p "$cache"
            brew_qt=/opt/homebrew/opt/qt
            version=$(sed -n 's/^set(PACKAGE_VERSION "\([0-9.]*\)")/\1/p' "$brew_qt/lib/cmake/Qt6/Qt6ConfigVersionImpl.cmake" 2>/dev/null | head -1)
            if [ "$version" = "$QT_VERSION" ]; then
              qt_dir="$brew_qt"
            else
              echo "Qt Homebrew $version ≠ $QT_VERSION : Qt $QT_VERSION dans $cache/Qt"
              qt_dir="$cache/Qt/$QT_VERSION/macos"
              # Témoin écrit en dernier : une installation interrompue est refaite au run suivant.
              if [ ! -f "$cache/Qt/$QT_VERSION/.complet" ]; then
                rm -rf "$cache/Qt/$QT_VERSION"
                python3 -m venv "$cache/venv-aqt"
                "$cache/venv-aqt/bin/pip" install -q "git+https://github.com/miurahr/aqtinstall@076e1659807d0b362a3ed684d54c2e9c775eb9c7"
                "$cache/venv-aqt/bin/python" -m aqt install-qt mac desktop "$QT_VERSION" clang_64 -m qtshadertools qtimageformats qtpdf qtmultimedia qtwebview --outputdir "$cache/Qt"
                touch "$cache/Qt/$QT_VERSION/.complet"
              fi
            fi
            command -v ninja >/dev/null || { echo "::error::ninja absent du Mac (brew install ninja)"; exit 1; }
            echo "Qt : $qt_dir ($version)"
            echo "ACTCRAFT_QT_DIR=$qt_dir" >> "$GITHUB_ENV"
            echo "$qt_dir/bin" >> "$GITHUB_PATH"
            # 4 compilations à la fois : le Mac (8 Go) garde de la mémoire pour les sessions pendant le build.
            echo "CMAKE_BUILD_PARALLEL_LEVEL=4" >> "$GITHUB_ENV"
            if command -v ccache >/dev/null; then
              echo "CMAKE_C_COMPILER_LAUNCHER=ccache" >> "$GITHUB_ENV"
              echo "CMAKE_CXX_COMPILER_LAUNCHER=ccache" >> "$GITHUB_ENV"
              # Pas de CCACHE_BASEDIR : il rend __FILE__ relatif et casse QFINDTESTDATA (tst_modules, tst_ui_preferences).
              # Le dossier de travail du runner est fixe, les chemins absolus suffisent au cache.
              echo "CCACHE_DIR=$cache/ccache" >> "$GITHUB_ENV"
            fi

        # paquet-mac.sh refuse une construction où ACTCRAFT_MEDIA ou ACTCRAFT_WEBFRAME vaut OFF (module Qt manquant).
        - name: Construire l'image disque
          shell: bash
          run: |
            if ./scripts/paquet-mac.sh > "$RUNNER_TEMP/paquet-mac.txt" 2>&1; then code=0; else code=$?; fi
            cat "$RUNNER_TEMP/paquet-mac.txt"
            if [ "$code" != 0 ] || ! tail -1 "$RUNNER_TEMP/paquet-mac.txt" | grep -q '^DMG OK : '; then exit 1; fi

        # Bloquant : bundle, signature, qt.conf, otool, bibliothèques chargées à --version.
        - name: Essai de l'image disque
          shell: bash
          run: |
            if ./scripts/essai-dmg.sh > "$RUNNER_TEMP/essai-dmg.txt" 2>&1; then code=0; else code=$?; fi
            cat "$RUNNER_TEMP/essai-dmg.txt"
            if [ "$code" != 0 ] || [ "$(tail -1 "$RUNNER_TEMP/essai-dmg.txt")" != "ESSAI DMG OK" ]; then exit 1; fi

        # Informatif : une fenêtre ne s'ouvre pas sûrement sur l'exécuteur ; le lancement est prouvé par L11-T29m sur le Mac.
        - name: Lancement de 3 s (informatif)
          continue-on-error: true
          shell: bash
          run: |
            ./scripts/essai-dmg.sh --lancer 2>&1 | tail -4
            test "${PIPESTATUS[0]}" = 0

        - name: Somme SHA-256
          shell: bash
          env:
            VERSION: ${{ needs.mode.outputs.version }}
          run: |
            cd build/paquet
            shasum -a 256 "ActCraft-$VERSION-macos-arm64.dmg"

        - name: Conserver l'image disque pour le brouillon (mode réel seulement)
          if: needs.mode.outputs.mode == 'reel'
          uses: actions/upload-artifact@ea165f8d65b6e75b540449e92b4886f43607fa02 # v4.6.2
          with:
            name: paquet-macos
            path: build/paquet/ActCraft-${{ needs.mode.outputs.version }}-macos-arm64.dmg
            if-no-files-found: error
            retention-days: 1
  ```
  Le bloc est à l'indentation du fichier (jobs à deux espaces sous `jobs:`, étapes à six) ; dans le fichier, il perd les
  deux espaces de ce plan. Puis, dans le même fichier :
  - job `release` : `needs: [mode, secrets, windows, android, macos]` ; dans « Sommes SHA-256 et notes »,
    `$attendus = @("ActCraft-$env:VERSION-arm64-v8a.apk", "ActCraft-$env:VERSION-macos-arm64.dmg", "ActCraft-$env:VERSION-setup.exe")`
    (ordre alphabétique : il est comparé à la liste triée des fichiers lus) ;
  - job `nettoyer` : `needs: [mode, windows, android, macos, release]` ;
  - commentaire d'en-tête (lignes 4-6) : « installateur + APK arm64-v8a signé par la clé de release + image disque macOS
    non signée ».
  Exécuteur : `runs-on` selon la visibilité (`github.event.repository.private`, présent dans les événements `push` et
  `workflow_dispatch`) ; sur le Mac mini, seule la garde de mémoire ; le lancement de 3 s est une étape
  `continue-on-error: true` (une fenêtre ne s'ouvre pas sûrement sur l'exécuteur ; L11-T29m le prouve sur le Mac).
  Contrôle : commande YAML de « Test et commande bornée » → `YAML OK`.
- [ ] **Étape 11 : rapport et commit.** `docs/verifications/2026-11-07-lot11-paquet-macos.md` (contenu de « Terminé
  quand », restes pour le PC) ; preuve de tâche (suite complète sous le jeton, règles du Mac) → `100% tests passed, 0
  tests failed out of N`, N = N11 + 32 ; `git -C <wt> add scripts/paquet-mac.sh scripts/essai-dmg.sh scripts/verifier-licences-mac.sh app/CMakeLists.txt packaging/qt-autorises.txt packaging/autres-autorises.txt THIRD_PARTY_NOTICES.md .github/workflows/release.yml docs/verifications/2026-11-07-lot11-paquet-macos.md`
  et chaque `LICENSES/<SPDX>.txt` neuf nommé ; `git -C <wt> diff --cached --stat` relu (modes `100755` des trois
  scripts : `git -C <wt> ls-files -s scripts/*.sh`) ; commit « Lot 11 : paquet macOS non signé, image disque et son
  essai (L11-T29m) ».

<a id="tache-l11-t30"></a>
### Tâche L11-T30 — Clôture du lot et porte de fin

**Objectif.** Contrôle du lot 11 sur le worktree du chantier, à jour de `main`, avant la PR ; préparer la porte de fin :
release prête, go demandé. Passe de fluidité finale (décision 14).

**Fichiers.** `tests/tst_ui_shell.cpp` (`lot11StubsGone` : aucun marqueur dans les sources, les tests et les
scénarios, liste R2 complète ; le marqueur est construit dans le test, `QStringLiteral("Talon ") + QStringLiteral("L11")`,
pour que le test ne se compte pas lui-même), `docs/mesures/2026-10-22-lot11-fin.md`, `docs/verifications/2026-10-22-lot11-porte.md`, `graphify-out/`.

**Commandes.**
1. Suites : `pwsh -NoProfile -File ./scripts/test.ps1 -Preset release` puis `pwsh -NoProfile -File ./scripts/test.ps1`
   (formes bornées) → `TESTS OK` deux fois, `out of <N11 + 32>`, aucun `SKIP` de talon ;
   `git -C <wt> grep -l "Talon L11" -- . ':!docs/' ':!graphify-out/' ':!tests/tst_ui_shell.cpp'` vide.
2. Jeton `ecran` (annoncé à Jérôme d'une ligne : « captures et banc à l'écran principal pendant ≈ 20 min ») :
   `$env:ACTCRAFT_UPDATES_URL = 'file:///<wt avec des barres obliques>/tests/data/updates/newer.json'` **avant**
   `pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario tous -Rendu d3d11 -Sortie .tmp/captures-ecran > .tmp/captures-ecran.txt 2>&1`
   → `CAPTURES OK` (`tous` ne prend pas `bench/scenarios/controle/` ; aucune requête vers l'API réelle) ; puis
   `pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario controle/pseudo-langue -Rendu d3d11 -Traduction build/debug/i18n/actcraft_xx.qm -Sortie .tmp/captures-ecran-pseudo > .tmp/captures-ecran-pseudo.txt 2>&1`
   → `TEXTES OK`, `CAPTURES OK`.
3. Banc complet D3D11 plein écran, trois passages (S13 rejoué) :
   `pwsh -NoProfile -File ./scripts/bench.ps1 > .tmp/bench-<i>.txt 2>&1` ; verdicts, figements et reprises relevés ;
   comparaison à la référence de fin du lot précédent (`docs/mesures/`) : tout critère tenu par la référence l'est
   encore, sinon une tâche de correction `L11-C<n>` précède la PR (porte de non-régression du profil).
4. `graphify update .` ; commit de `graphify-out/`.
5. Rapport de mesure `docs/mesures/2026-10-22-lot11-fin.md` (JSON cités, jamais commités).
6. Rapport de porte `docs/verifications/2026-10-22-lot11-porte.md` : ce qui est livré ; hypothèses H11-1 à H11-18 et H11-19 (amendement 3 ; H11-12 à H11-17 : rapport de L11-T0) ;
   tailles des paquets (N1-A10 ; le `.dmg` de L11-T29m compris) ; les deux `SKIP` de `tst_tools_twousers` attendus tant
   que le lot 7 est absent (amendement 3 du plan de 11a ; ce ne sont pas des talons) ; manières modernes choisies (coffre 0100) ; **mesures qui reviennent à Jérôme** et
   leurs procédures ; **procédure du go** (ci-dessous), écrite pour être suivie telle quelle **après** le go.

**Mesures pour Jérôme (procédures exactes, mode autonome point 4).**
- Dell Inspiron 15 3510 : `pwsh -NoProfile -File ./scripts/paquet-banc.ps1` sur le poste de développement → zip de
  `build/paquet-banc/` copié sur le Dell ; procédure de `docs/banc.md` (dézipper, lancer, renvoyer le JSON).
- MacBook Air M1 : dans un clone du dépôt à jour, `scripts/bench.sh --rendu tous` ; renvoyer `bench/results/*.json`.
  Paquet macOS (amendement 3) : copier `ActCraft-1.0.0-macos-arm64.dmg` sur le M1 **par un téléchargement** (navigateur
  ou AirDrop, pour que macOS pose la marque de quarantaine), l'ouvrir, glisser ActCraft dans Applications, premier
  lancement par clic droit > Ouvrir (ou « Ouvrir quand même » dès macOS 15) ; renvoyer la version de macOS, le message
  de Gatekeeper vu et « ActCraft s'ouvre : oui / non ».
- Tablette Android : `docs/banc-tablette.md` (L11-T15).
Ces mesures fondent la décision go / no-go de l'étape 2 du BRIEF ; elles ne bloquent pas la PR.

**Procédure du go (exécutée seulement après le go écrit de Jérôme, jamais par cette tâche).**
1. Clé de signature (N1-6) : `keytool -genkeypair -v -keystore actcraft-release.jks -alias actcraft -keyalg RSA -keysize 4096 -validity 10000`
   sur le poste de Jérôme ; copie hors ligne gardée par Jérôme ; empreinte SHA-256 notée
   (`keytool -list -v -keystore actcraft-release.jks -alias actcraft`).
2. Environnement GitHub `release` (Settings → Environments), règle « Selected branches and tags », motif de tag `v*` ;
   secrets d'environnement `ACTCRAFT_KEYSTORE_BASE64`, `ACTCRAFT_KEYSTORE_PASSWORD`, `ACTCRAFT_KEY_ALIAS`,
   `ACTCRAFT_KEY_PASSWORD` par `gh secret set <nom> --env release` (saisis par Jérôme).
3. Fichier go `<git-common-dir>/chantiers/go-action-finale` écrit par l'orchestrateur au go (limite de 6 h).
4. `git tag -a v1.0.0 -m "ActCraft 1.0.0"` sur le commit mergé dans `main`, puis `git push origin v1.0.0`.
5. Fin du workflow `release.yml` : brouillon avec installateur, APK signé, `.dmg` non signé (job `macos`, amendement 3),
   `SHA256SUMS.txt`, notes ; l'empreinte
   SHA-256 du certificat de l'APK (`apksigner verify --print-certs`) égale celle du point 1 (S14).
6. `gh release edit v1.0.0 --draft=false` sous le même go ; suppression du fichier go aussitôt après.

**Interdits.** Aucun tag, aucune commande `gh release` en écriture, aucun fichier go, aucun merge (le merge passe par la
PR et attend la CI verte, profil) ; aucune fenêtre visible hors du jeton `ecran`.

**Terminé quand** les points 1 à 6 sont faits et commités ; la porte de fin est prête à être présentée par
l'orchestrateur : « release 1.0.0 prête, go écrit demandé ».

**Étapes.**
- [ ] **Étape 1 :** `lot11StubsGone` rouge si un talon reste, puis vert ; suites.
- [ ] **Étape 2 :** captures et banc à l'écran (jeton `ecran`, annoncé).
- [ ] **Étape 3 :** rapports, graphe ; `git add` des chemins nommés ; `NOMS OK` ; contrôle des titres du corpus vide ;
  commit « Lot 11 : clôture du lot, mesures et porte de fin (L11-T30) ».

## Scénarios et tâches (11b)

| Scénario | Tâches |
|---|---|
| S29 — Aide, langue et mises à jour | L11-T17, L11-T18 (aide, F1 sans réseau) ; L11-T19, L11-T20 (pseudo-langue : fenêtre, menus, trois boîtes, aucune chaîne sans marque) ; L11-T21, L11-T22 (réponse simulée, aucune requête au démarrage) |
| S19 — Mode classe protégé, rejoué au lot 11 sur toutes les commandes | L11-T26 |
| S32 — Profils fournis | L11-T27 |
| S14 — Release prête | L11-T28, L11-T29, L11-T29m (paquet macOS, amendement 3), L11-T30 (porte ; le tag attend le go) |
| S2.1 — Raccourcis (posé par ce plan) : chaque raccourci déclenche sa commande, aucun pendant la saisie | L11-T23 |
| S2.2 — Accessibilité (posé par ce plan) : AA et focus visible dans chaque boîte et en mise en page étroite | L11-T24 |
| S1.1 — Notices (posé par ce plan) : toute bibliothèque livrée a sa notice ; « À propos » montre le même texte | L11-T25 |
| S13, S2, S3 (rejoués) | L11-T30 |

## Hypothèses pour Jérôme (suite de H11-1 à H11-6 ; porte de fin)

### Niveau 1
- **H11-7 — Mises à jour.** La recherche contacte `api.github.com` quand on la demande (ou au démarrage si le réglage
  est coché) ; rien d'autre n'est envoyé.
- **H11-8 — Licences.** Notices complètes, non validées par un juriste (N1-8).
- **H11-9 — Mesures sur les machines de référence** (Dell 3510, M1, tablette) : procédures de L11-T30 ; décision go /
  no-go de l'étape 2 du BRIEF.
- **H11-10 — Taille des paquets 1.0.0** (N1-A10), rapportée par L11-T29 (et L11-T29m pour le `.dmg`).
- **H11-18 — Paquet macOS non signé** (amendement 3 ; H11-12 à H11-17 sont prises par le rapport de L11-T0). Signature ad hoc seulement : Gatekeeper bloque le premier
  lancement d'un `.dmg` téléchargé (clic droit > Ouvrir, ou « Ouvrir quand même » dès macOS 15). Signer et notariser
  demande un compte Apple Developer payant (décision de Jérôme). Le job `macos` de `release.yml` tourne sur le runner
  du Mac mini tant que le dépôt est privé : une release attend que le Mac soit allumé et libre ; dépôt public →
  `macos-latest`.
  Bibliothèques Homebrew embarquées (glib et gettext sous LGPL, OpenSSL, ICU…) : notices sans avis de juriste (N1-8).
- **H11-19 — À revoir par Jérôme : `.dmg` dans la release 1.0.0** (amendement 3). Demandé par le pilote le 2026-11-06,
  sous la délégation du 2026-11-02 ; D11-15 et N2-13 (« pas de paquet macOS ») rouverts. Artefact construit et essayé ;
  sa présence dans la release publiée se tranche au go.

### Niveau 2
- **H11-11** Variables et options d'essai (`ACTCRAFT_UPDATES_URL` limitée à `file:` et à l'hôte local ;
  `--traduction` du joueur de captures seulement).

## Relecture par l'auteur (writing-plans)

- Couverture : S29 (aide, langue, mises à jour), S19 rejoué, S32, S14 ont leurs tâches ; finitions demandées (contrastes
  et focus, traductions, notes de version et `CHANGELOG.md`, paquets Windows et Android, licences tierces) aussi ;
  macOS : paquet `.dmg` non signé (D11-15 remplacée par l'amendement 3, L11-T29m).
- La release n'a lieu que sur go : L11-T29 et L11-T30 l'interdisent explicitement ; la procédure du go est écrite pour
  l'après.
- Noms cohérents avec 11a : `appUpdates`, `UpdateDialog`, `appLayout`, `opensDialog`, `lockedCount`, `appProfile`,
  N = N11 + 32.
