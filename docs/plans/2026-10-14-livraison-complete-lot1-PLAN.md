# Livraison complète — lot 1 « Socle ouvrable et paquets » — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**But.** Version 0.1.0 : une fenêtre ActCraft utilisable (ligne supérieure avec menus, indicateur « page n / N »,
échelle et plein écran ; palette d'outils à droite ; page au centre avec barres de défilement ; pas de barre
d'état), le stylo branché (16 couleurs, 4 épaisseurs et une réglette, indicateur de largeur ; stylet, souris et un
doigt), plusieurs pages (« suivante » crée une page à la fin), F5, `--version` et « À propos » avec notices ; le
banc et un outil de captures adaptés à cette interface ; un installateur Windows et des APK Android produits,
essayés et construits par la CI ; un workflow de release qui crée un brouillon sous go.

**Architecture.** `core/` gagne le squelette du document (`Document` → pages, navigation) ; `actions/` le registre
des commandes nommées et la disposition (menus, palette, raccourcis, couleurs) lus en JSON, profil d'abord ;
`input/` le doigt unique ; `ui/` le thème et les icônes à nous, le contrôleur de document, le pont QML des
commandes, `AppInfo`, et la coquille QML découpée en composants ; `bench/` l'exécutable `actcraft-captures` (DA8) ;
`scripts/` et `packaging/` les paquets (Inno Setup, chaîne Android) ; `.github/workflows/` la CI des paquets et
`release.yml`. Cadrage qui fait autorité : `docs/specs/2026-10-14-livraison-complete-cadrage.md` (commit
`663644e`), décisions 4, 20 à 23, 25 à 36, 38, DA1, DA2, DA6 à DA10, scénarios S1, S2, S3.

**Pile.** C++20, Qt 6.12 (Core, Gui, Qml, Quick, QuickControls2, Svg, Test), CMake + Ninja, MSVC 2022 ; Qt 6.12 pour
Android (JDK 17, SDK, NDK) ; Inno Setup 6 ; PowerShell 7, Python 3.12 + Pillow (lecture de pixels) ; GitHub
Actions (Windows, macOS, Ubuntu + KVM).

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L1-T1 | Profil du chantier : release due sous go, motifs d'action finale, durée du go, régime du banc, preuve visuelle | — | `.claude/chantier.md` | isolé | implementeur-tdd | `PROFIL OK` (53 cas : 35 écritures tenues, 18 lectures laissées ; rouge avant : 31 `ECART`) | S1, S2, S3 | #tache-l1-t1 |
| L1-T2 | Outillage des paquets : JDK 17, SDK/NDK, Qt 6.12 Android, Inno Setup, émulateur (WHPX sinon CI) | — | `scripts/dev-env-android.ps1`, `packaging/android/versions.env`, `docs/verifications/2026-10-14-outillage-paquets.md` | isolé | implementeur-tdd | 14 lignes `VERIF` sans `…` ; `dev-env-android.ps1` → `ENV ANDROID OK` ; `ISCC` répond | S1, S3 | #tache-l1-t2 |
| L1-T3 | `core` : squelette `Document` (pages, page suivante qui ajoute, empreinte) | L1-T1 | `core/CMakeLists.txt`, `core/include/actcraft/core/document.h`, `core/src/document.cpp`, `tests/core.cmake`, `tests/tst_core_document.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 30 tests, `tst_core_document` Passed | S2 | #tache-l1-t3 |
| L1-T4 | `actions` : registre des commandes nommées, déclarations JSON | L1-T1 | `actions/CMakeLists.txt`, `actions/include/actcraft/actions/commandregistry.h`, `actions/src/commandregistry.cpp`, `actions/data/commands.json`, `tests/CMakeLists.txt`, `tests/actions.cmake`, `tests/tst_actions_registry.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 30 tests, `tst_actions_registry` Passed | S2 | #tache-l1-t4 |
| L1-T5 | `actions` : disposition (menus, palette, raccourcis, couleurs, épaisseurs), profil d'abord | L1-T4 | `actions/CMakeLists.txt`, `actions/include/actcraft/actions/layout.h`, `actions/src/layout.cpp`, `actions/data/layout.json`, `tests/actions.cmake`, `tests/tst_actions_layout.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 31 tests, `tst_actions_layout` Passed | S2 | #tache-l1-t5 |
| L1-T6 | `input` : un doigt devient un échantillon | L1-T1 | `input/include/actcraft/input/sample.h`, `input/include/actcraft/input/qtevents.h`, `input/src/qtevents.cpp`, `tests/tst_input.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 29 tests, `tst_input` Passed ; `tst_input.exe` : 2 lignes `PASS` des fonctions nouvelles ; banc hors écran (résultat) | S3 | #tache-l1-t6 |
| L1-T7 | `ui` : direction visuelle, thème `Theme` et icônes SVG à nous | L1-T1 | `CMakeLists.txt`, `ui/CMakeLists.txt`, `ui/qml/Theme.qml`, `ui/icons/menu.svg`, `ui/icons/outils.svg`, `ui/icons/page-precedente.svg`, `ui/icons/page-suivante.svg`, `ui/icons/stylo.svg`, `ui/icons/plein-ecran.svg`, `ui/icons/ajuster.svg`, `ui/icons/quitter.svg`, `ui/icons/aide.svg`, `ui/icons/a-propos.svg`, `tests/ui.cmake`, `tests/tst_ui_theme.cpp`, `docs/design/2026-10-14-direction-visuelle.md` | isolé | implementeur-tdd | TESTS OK, ≥ 30 tests, `tst_ui_theme` Passed ; banc hors écran (résultat) | S2 | #tache-l1-t7 |
| L1-T8 | `ui` : `PageView` — stylo réglable, défilement, entrée réservée à la vue, doigt unique | L1-T6 | `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`, `ui/include/actcraft/ui/viewtransform.h`, `ui/src/viewtransform.cpp`, `tests/tst_pageview.cpp`, `tests/tst_viewtransform.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 29 tests, `tst_pageview` et `tst_viewtransform` Passed ; exécutables : 7 lignes `PASS` des fonctions nouvelles ; banc hors écran (résultat) | S2, S3 | #tache-l1-t8 |
| L1-T9 | `ui` : `DocumentController` (pages, page affichée, navigation) | L1-T3, L1-T7 | `ui/CMakeLists.txt`, `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`, `tests/ui.cmake`, `tests/tst_ui_document.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 32 tests, `tst_ui_document` Passed ; banc hors écran (résultat) | S2 | #tache-l1-t9 |
| L1-T10 | `ui` : pont QML `Commands` (menus, palette, raccourcis filtrés, déclenchement) | L1-T5, L1-T9 | `ui/CMakeLists.txt`, `ui/include/actcraft/ui/commands.h`, `ui/src/commands.cpp`, `tests/ui.cmake`, `tests/tst_ui_commands.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 35 tests, `tst_ui_commands` Passed ; banc hors écran (résultat) | S2 | #tache-l1-t10 |
| L1-T11 | Version, « À propos », notices, liste des modules Qt autorisés | L1-T10 | `ui/CMakeLists.txt`, `ui/include/actcraft/ui/appinfo.h`, `ui/src/appinfo.cpp`, `ui/qml/AboutDialog.qml`, `app/CMakeLists.txt`, `app/main.cpp`, `THIRD_PARTY_NOTICES.md`, `LICENSES/LGPL-3.0.txt`, `LICENSES/GPL-3.0.txt`, `packaging/qt-autorises.txt`, `scripts/verifier-licences.ps1`, `scripts/version-projet.ps1`, `tests/CMakeLists.txt`, `tests/app.cmake`, `tests/ui.cmake`, `tests/tst_ui_about.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 39 tests, `tst_ui_about`, `tst_app_version`, `tst_licences_ok`, `tst_licences_refus` Passed ; banc hors écran (résultat) | S1 | #tache-l1-t11 |
| L1-T12 | `ui` + `app` : coquille QML (ligne supérieure, palette, zone de page, F5, commandes) | L1-T8, L1-T11 | `ui/CMakeLists.txt`, `ui/qml/Main.qml`, `ui/qml/TopBar.qml`, `ui/qml/ToolPalette.qml`, `ui/qml/PageArea.qml`, `ui/qml/CommandButton.qml`, `ui/qml/CommandMenu.qml`, `ui/qml/ColorSwatches.qml`, `ui/qml/WidthPicker.qml`, `app/startup.h`, `app/startup.cpp`, `bench/app/main.cpp`, `tests/ui.cmake`, `tests/tst_ui.cpp`, `tests/tst_ui_drawing.cpp`, `tests/tst_bench_verdict.cpp`, `tests/tst_ui_shell.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 40 tests, `tst_ui_shell`, `tst_ui`, `tst_ui_drawing` Passed ; exécutables : 8 `PASS` `TstUiShell`, 1 `PASS` `generatedPageOptionLoadsThe300Objects` ; banc hors écran (résultat) | S2 | #tache-l1-t12 |
| L1-T13 | `bench` : `actcraft-captures`, joueur de scénarios d'interface (DA8) | L1-T9 | `bench/CMakeLists.txt`, `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`, `bench/captures/main.cpp`, `tests/bench.cmake`, `tests/tst_bench_uiscenario.cpp`, `tests/data/uiscenario/fenetre.qml` | isolé | concepteur-critique | TESTS OK, ≥ 33 tests, `tst_bench_uiscenario` Passed | S2 | #tache-l1-t13 |
| L1-T14 | Banc adapté à la nouvelle interface (DA7) | L1-T12 | `tests/tst_bench_verdict.cpp`, `bench/src/runner.cpp` | isolé | concepteur-critique | release : TESTS OK, ≥ 40 tests, `tst_bench_verdict` et `tst_bench_smoke` Passed, `PASS` de `benchTargetsAreVisibleInTheShell` ; banc hors écran : `content_check.ok = True` | S2 | #tache-l1-t14 |
| L1-T15 | Scénario `premiers-pas` et `scripts/captures.ps1` (S2) | L1-T12, L1-T13 | `bench/scenarios/premiers-pas.json`, `scripts/captures.ps1`, `tests/bench.cmake` | isolé | implementeur-tdd | TESTS OK, ≥ 42 tests, `tst_captures_premiers_pas` Passed ; `captures.ps1` → `LU pageIndicator.text=1 / 2 OK` puis `CAPTURES OK` | S2 | #tache-l1-t15 |
| L1-T16 | Installateur Windows : Inno Setup, `paquet.ps1`, `essai-installateur.ps1` (S1) | L1-T2, L1-T11 | `packaging/windows/actcraft.iss`, `scripts/paquet.ps1`, `scripts/essai-installateur.ps1`, `packaging/qt-autorises.txt` | isolé | implementeur-tdd | `PAQUET OK` puis `INSTALLATEUR OK` | S1 | #tache-l1-t16 |
| L1-T17 | Chaîne Android : presets, propriétés de l'APK, `paquet-android.ps1` | L1-T2, L1-T11, L1-T16 | `CMakePresets.json`, `app/CMakeLists.txt`, `scripts/paquet-android.ps1`, `scripts/dev-env-android.ps1`, `packaging/qt-autorises.txt`, `docs/verifications/2026-10-14-outillage-paquets.md`, `android/AndroidManifest.xml` (repli seulement) | isolé | concepteur-critique | `APK OK` (2 APK : identifiant, `versionCode` 100, aucune permission, signature, licences) | S3 | #tache-l1-t17 |
| L1-T18 | Essai de l'APK sur émulateur : `essai-android.ps1`, `lire-pixel.py` (S3) | L1-T12, L1-T17 | `scripts/essai-android.ps1`, `scripts/lire-pixel.py`, `docs/verifications/2026-10-14-outillage-paquets.md` | isolé | implementeur-tdd | `ANDROID OK` (émulateur local) ; sans WHPX : scripts analysés sans erreur, S3 prouvé par L1-T19 | S3 | #tache-l1-t18 |
| L1-T19 | CI : jobs `paquet-windows` (S1) et `android` (S3, KVM), branches `essai-ci/**` | L1-T14, L1-T15, L1-T16, L1-T18 | `.github/workflows/ci.yml`, `scripts/paquet.ps1`, `scripts/essai-installateur.ps1`, `scripts/paquet-android.ps1`, `scripts/essai-android.ps1`, `scripts/dev-env-android.ps1`, `packaging/android/versions.env` ; en repli de L1-T17 ou L1-T18 seulement : `CMakePresets.json`, `app/CMakeLists.txt`, `android/AndroidManifest.xml`, `packaging/qt-autorises.txt`, `scripts/lire-pixel.py` | isolé | concepteur-critique | contrôle de propriété intellectuelle avant push recopié ; run CI réel sur `essai-ci/L1-T19` `completed / success` (suivi par appels de 8 min) ; journaux : `INSTALLATEUR OK`, `APK OK`, `ANDROID OK` | S1, S3 | #tache-l1-t19 |
| L1-T20 | `release.yml` (brouillon, essai / réel, échec sans secrets), `CHANGELOG.md`, commandes réelles au profil | L1-T19 | `.github/workflows/release.yml`, `CHANGELOG.md`, `scripts/verifier-secrets-release.ps1`, `scripts/notes-version.ps1`, `.claude/chantier.md` | isolé | concepteur-critique | contrôle de propriété intellectuelle avant push recopié ; run réel de `release.yml` sur `essai-ci/release-L1-T20` `completed / success` : `CONTROLE OK`, `NOTES OK`, `INSTALLATEUR OK`, `APK OK` ; aucune release créée ; bloc `action-finale` inchangé | S1, S3 | #tache-l1-t20 |

20 tâches, un lot (lot 1 du cadrage). Le nombre minimal de tests de la colonne `preuve` compte les 29 tests de la
référence (`docs/sessions/livraison-complete-etat.md`, « Référence ») plus ceux des tâches dont la tâche dépend,
directement ou non, plus les siens ; une tâche lancée après d'autres en voit davantage : « ≥ » est la règle. Tests
ctest ajoutés par le lot : L1-T3 +1, L1-T4 +1, L1-T5 +1, L1-T7 +1, L1-T9 +1, L1-T10 +1, L1-T11 +4, L1-T12 +1,
L1-T13 +1, L1-T15 +1, soit **42** à la fin du lot (sous Windows, où `pwsh` existe).

Aucune tâche ne joue de fenêtre en plein écran ni le banc à l'écran : le passage D3D11 plein écran de S2, le banc
complet de non-régression (DA7) et la remise des paquets sont faits **à la clôture par l'orchestrateur**, sous le
jeton `ecran` (section « Clôture du lot 1 »). Deux tâches affichent une fenêtre ordinaire quelques secondes
(L1-T16 : lancement de 5 s de l'application installée ; L1-T18 : rien, l'émulateur tourne sans fenêtre) : les
annoncer d'une ligne.

## Écarts au cadrage (repris à la porte de fin du lot 1)

Choix de ce plan que le cadrage ne dit pas, ou dit autrement. Chacun est réversible et présenté à Jérôme à la
porte de fin.

1. **Menus Éditer et Insérer masqués tant qu'ils sont vides** (L1-T5, L1-T10, L1-T12). La décision 4 cite six menus ;
   ils sont tous déclarés dans `actions/data/layout.json`, mais un menu sans commande disponible n'est pas affiché :
   le lot 1 montre Fichier, Affichage, Outils, Aide. Les deux autres apparaissent quand un lot leur donne une
   commande.
2. **Page de démarrage vierge** (L1-T9, L1-T12). Le cadrage ne le dit pas (la décision 4 ne parle pas du
   démarrage) : défaut prudent, un tableau blanc s'ouvre sur une page vierge ; la page générée de 300 objets
   n'est chargée que pour le banc et les tests (`StartupOptions::generatedPage`, DA7). Trois tests du prototype
   changent pour cette raison (écrite en L1-T12).
3. **`release.yml` dès le lot 1** (L1-T20). Le cadrage place l'essai à blanc complet (S14) au lot 11 ; le workflow
   est avancé pour que la release 0.1.0 puisse être proposée à la porte du lot 1 (décision 33). Le lot 11 garde S14.
4. **Preuves de CI par des branches `essai-ci/…`** (L1-T19, L1-T20). `workflow_dispatch` n'est utilisable qu'une fois
   le workflow sur `main`, et la PR du lot est créée une seule fois, à la clôture : les tâches de CI poussent une
   branche jetable `essai-ci/<sujet>` (ou `essai-ci/release-<sujet>`), après un contrôle de propriété intellectuelle,
   puis la suppriment. Ce push publie le lot sur le dépôt public avant la relecture de fin de lot.
5. **Motifs d'action finale élargis** (L1-T1). Les deux motifs de la décision 38 b et les trois du profil sont
   remplacés par quatre motifs plus stricts : `git -C <dossier>`, `tag -a`, `--tags`, `--follow-tags`, `refs/tags/`,
   `gh release upload|delete`, `gh api` en écriture implicite (`-f`, `-F`, `--input`), guillemets, `.exe`,
   `--mirror`, `refs/tags/*`, création de tag par l'API sont tenus ; les lectures restent libres. Résidus acceptés
   (règle écrite seulement) : tag passé par variable, `curl` / `Invoke-RestMethod` vers l'API, mutation GraphQL.
6. **Association `.flipchart` réduite à « Ouvrir avec »** (L1-T16). Décision 26 : association proposée décochée ;
   cochée, elle ajoute ActCraft à « Ouvrir avec » sans prendre la place d'ActivInspire. La 0.1.0 n'ouvre encore
   aucun fichier (lecteur et format natif au lot 2) : les associations lancent l'application sur une page vierge.
7. **Épaisseur du stylo par défaut : 4 px au lieu de 3** (L1-T8 ; F-220). Les traits mesurés par le banc
   s'épaississent un peu ; la mesure de fin de lot 1 devient la référence (DA7).

## Couverture des scénarios

| Scénario | Tâches | Joué à la clôture |
|---|---|---|
| S1 — installateur | L1-T2 (Inno Setup), L1-T11 (`--version`, notices, licences), L1-T16 (paquet + essai local), L1-T19 (CI `windows-latest`), L1-T20 (release), L1-T1 (règles de release) | oui : `paquet.ps1` + `essai-installateur.ps1` en release, installateur remis dans `build/paquet/` |
| S2 — premiers pas | L1-T3, L1-T4, L1-T5, L1-T7, L1-T8, L1-T9, L1-T10, L1-T12 (interface), L1-T13, L1-T15 (captures, hors écran), L1-T14 (banc), L1-T1 (commande de preuve) | oui : `captures.ps1 -Scenario premiers-pas -Rendu d3d11` à l'écran (jeton `ecran`) |
| S3 — APK | L1-T2 (chaîne), L1-T6 + L1-T8 (doigt), L1-T17 (APK), L1-T18 (émulateur local), L1-T19 (CI KVM), L1-T20 (release) | oui : `paquet-android.ps1`, APK remis dans `build/paquet/` ; S3 rejoué par la CI de la PR |

## Vagues proposées (parallélisme 2 au plus)

V1 : L1-T1 ‖ L1-T2 · V2 : L1-T3 ‖ L1-T4 · V3 : L1-T5 ‖ L1-T6 · V4 : L1-T7 ‖ L1-T8 · V5 : L1-T9 ·
V6 : L1-T10 ‖ L1-T13 · V7 : L1-T11 · V8 : L1-T12 ‖ L1-T16 · V9 : L1-T15 ‖ L1-T17 · V10 : L1-T14 ‖ L1-T18 ·
V11 : L1-T19 · V12 : L1-T20.

Deux tâches d'une même vague ne partagent aucun fichier (vérifié sur la colonne `fichiers touchés`). Fichiers
partagés tenus par les dépendances : `CMakeLists.txt` racine (L1-T7 seule) ; `ui/CMakeLists.txt` (L1-T7 → L1-T9 →
L1-T10 → L1-T11 → L1-T12) ; `tests/ui.cmake` (même chaîne) ; `tests/CMakeLists.txt` (L1-T4 → L1-T11) ;
`tests/actions.cmake` (L1-T4 → L1-T5) ; `tests/bench.cmake` (L1-T13 → L1-T15) ; `app/CMakeLists.txt` (L1-T11 →
L1-T17) ; `packaging/qt-autorises.txt` (L1-T11 → L1-T16 → L1-T17) ; `ui/qml/Main.qml` (L1-T12 seule) ;
`tests/tst_bench_verdict.cpp` (L1-T12 → L1-T14) ; `.github/workflows/*.yml` (L1-T19 → L1-T20) ;
`.claude/chantier.md` (L1-T1 → … → L1-T20) ; `docs/verifications/2026-10-14-outillage-paquets.md` (L1-T2 → L1-T17 →
L1-T18).

## Carte des fichiers

| Fichier | Responsabilité | Tâche |
|---|---|---|
| `.claude/chantier.md` | profil : release due sous go, motifs d'action finale, banc, preuve visuelle, paquets | L1-T1, L1-T20 |
| `scripts/dev-env-android.ps1`, `packaging/android/versions.env` | environnement de la chaîne Android (poste et CI), versions relevées | L1-T2 (L1-T17, L1-T19 corrigent) |
| `docs/verifications/2026-10-14-outillage-paquets.md` | lignes `VERIF` de l'outillage, de l'APK et de l'émulateur | L1-T2, L1-T17, L1-T18 |
| `core/include/actcraft/core/document.h` | `Document`, `nextPageIndex`, `previousPageIndex`, `fingerprint(Document)` | L1-T3 |
| `actions/include/actcraft/actions/commandregistry.h`, `actions/data/commands.json` | `CommandRegistry`, `CommandInfo`, `Platform`, `ArgumentKind`, déclarations des 13 commandes | L1-T4 |
| `actions/include/actcraft/actions/layout.h`, `actions/data/layout.json` | `Layout`, `parseLayout`, `readLayeredFile`, `unknownCommands` ; disposition par défaut | L1-T5 |
| `input/include/actcraft/input/sample.h`, `input/src/qtevents.cpp` | `DeviceKind::Touch`, conversion d'un doigt | L1-T6 |
| `ui/qml/Theme.qml`, `ui/icons/*.svg` | jetons visuels, 10 icônes neuves 24 × 24 | L1-T7 |
| `ui/include/actcraft/ui/pageview.h`, `viewtransform.h` | stylo réglable, défilement, entrée réservée, doigt | L1-T8 |
| `ui/include/actcraft/ui/documentcontroller.h` | document ouvert, page affichée, « n / N » | L1-T9 |
| `ui/include/actcraft/ui/commands.h` | pont QML du registre et de la disposition | L1-T10 |
| `ui/include/actcraft/ui/appinfo.h`, `ui/qml/AboutDialog.qml` | version, licence, notices ; « À propos » | L1-T11 |
| `THIRD_PARTY_NOTICES.md`, `LICENSES/`, `packaging/qt-autorises.txt`, `scripts/verifier-licences.ps1` | licences livrées et contrôlées (décision 34) | L1-T11 (L1-T16, L1-T17 complètent la liste) |
| `scripts/version-projet.ps1` | `Get-ActCraftVersion`, `Get-ActCraftVersionCode` (décision 31) | L1-T11 |
| `ui/qml/*.qml` (coquille) | `Main`, `TopBar`, `ToolPalette`, `PageArea`, `CommandButton`, `CommandMenu`, `ColorSwatches`, `WidthPicker` | L1-T12 |
| `app/startup.*` | `StartupOptions::generatedPage` (banc et tests) | L1-T12 |
| `bench/include/actcraft/bench/uiscenario.h`, `bench/captures/main.cpp` | joueur de scénarios, exécutable `actcraft-captures` | L1-T13 |
| `bench/scenarios/premiers-pas.json`, `scripts/captures.ps1` | scénario S2, commande de preuve visuelle | L1-T15 |
| `packaging/windows/actcraft.iss`, `scripts/paquet.ps1`, `scripts/essai-installateur.ps1` | installateur, essai S1 | L1-T16 |
| `CMakePresets.json` (presets `android-*`), `scripts/paquet-android.ps1` | APK `arm64-v8a` et `x86_64` | L1-T17 |
| `scripts/essai-android.ps1`, `scripts/lire-pixel.py` | essai S3 sur émulateur | L1-T18 |
| `.github/workflows/ci.yml` | jobs `paquet-windows`, `android` | L1-T19 |
| `.github/workflows/release.yml`, `CHANGELOG.md`, `scripts/verifier-secrets-release.ps1`, `scripts/notes-version.ps1` | flux de release (décision 32) | L1-T20 |

## Contrats partagés entre tâches (noms figés)

Une tâche ne renomme aucun de ces noms. Les noms du prototype (plan `docs/plans/2026-10-13-prototype-mesure-PLAN.md`,
section « Contrats ») restent figés eux aussi.

- `actcraft::core` : `Document` (`pageCount`, `page`, `appendPage`), `nextPageIndex`, `previousPageIndex`,
  `fingerprint(const Document&)`.
- `actcraft::actions` : `Platform` (`Windows`, `Android`, `MacOS`, `Linux`), `Platforms`, `kAllPlatforms`,
  `platformBit`, `currentPlatform`, `platformName`, `ArgumentKind` (`None`, `Color`, `Number`, `Url`),
  `CommandInfo`, `argumentIsValid`, `CommandRegistry` (`loadDeclarations`, `declare`, `find`, `ids`,
  `isAvailable`, `setHandler`, `hasHandler`, `setEnabled`, `isEnabled`, `setChecked`, `isChecked`, `trigger`),
  `dataResourceDir`, `defaultCommandsJson` ; `MenuSpec`, `PaletteGroup`, `Layout`, `parseLayout`,
  `readLayeredFile`, `unknownCommands`. Formats JSON `actcraft-commands/1`, `actcraft-layout/1`.
- Identifiants des commandes du lot 1 : `file.quit`, `view.fullscreen`, `view.fit`, `view.zoom` (argument : pour
  cent), `page.previous`, `page.next`, `tool.pen`, `pen.color` (argument `#RRGGBB`), `pen.width` (argument :
  pixels de page), `menu.main`, `menu.tools`, `help.website`, `help.about`.
- Groupes de palette spéciaux : `couleurs`, `epaisseur` ; séparateur de menu : `"-"`.
- `actcraft::input` : `DeviceKind::Touch` (ajouté après `Other`).
- `actcraft::ui` : `ScrollAxis`, `scrollAxis`, `panForScrollPosition`, `kDefaultPenWidth`, `kMinPenWidth`,
  `kMaxPenWidth` ; `PageView` (propriétés `penColor`, `penWidth`, `inputBlocked`, `scrollSizeX`,
  `scrollPositionX`, `scrollSizeY`, `scrollPositionY` ; `scrollTo` ; signaux `penChanged`, `scrollChanged`,
  `inputBlockedChanged`) ; `DocumentController` (`fixedPage`, `pageCount`, `currentIndex`, `pageLabel`,
  `nextPage`, `previousPage`, `loadGeneratedFirstPage`, `document`) ; `Commands` (`menus`, `palette`,
  `shortcuts`, `colors`, `penWidths`, `platformName`, `errors`, `revision`, `trigger`, `setHandler`, `label`,
  `iconUrl`, `shortcut`, `isAvailable`, `isEnabled`, `setEnabled`, `isChecked`, `setChecked`, `setProfileDir`,
  `setPlatform`, `reload`, signaux `layoutChanged`, `stateChanged`, `triggered`) ; `AppInfo` (singleton :
  `version`, `licenseName`, `projectUrl`, `notices`).
- QML : singleton `Theme` (jetons de L1-T7) ; `objectName` `pageView`, `page` (1024 × 768), `fixedPage`,
  `pageCache`, `ink` (inchangés), et `topBar`, `toolPalette`, `pageArea`, `pageIndicator`, `zoomBox`,
  `colorSwatches`, `swatch-<i>`, `widthPicker`, `widthPreset-<i>`, `widthSlider`, `widthIndicator`,
  `menu-<id du menu>`, `button-<id de commande>`, `mainMenu`, `toolsMenu`, `aboutDialog`, `commands`,
  `documentController`, `horizontalScrollBar`, `verticalScrollBar` ; propriétés racine `cacheEnabled`,
  `generatedPage`, `fullScreen`, `openPopups` ; fonction racine `runCommand(id, argument)`.
- `app/startup.h` : `StartupOptions::generatedPage` (faux par défaut).
- `actcraft::bench` : `UiStepKind`, `UiStep`, `UiScenario`, `parseUiScenario`, `parseReadSpec`, `propertyText`,
  `UiScenarioPlayer` (`play`, `expect`). Format `actcraft-scenario/1`. Lignes de rapport `COMMANDE`, `TRAIT`,
  `TOUCHE`, `ATTENTE`, `PRET`, `CAPTURE`, `PIXEL`, `LU` ; dernière ligne `SCENARIO OK` / `SCENARIO EN ECHEC` /
  `SCENARIO INVALIDE`.
- Scripts : `version-projet.ps1` (`Get-ActCraftVersion`, `Get-ActCraftVersionCode`) ; dernières lignes
  `LICENCES OK`, `PAQUET OK : <chemin>`, `INSTALLATEUR OK`, `APK OK`, `ANDROID OK`, `CAPTURES OK`,
  `SECRETS OK` / `SECRETS ABSENTS`, `NOTES OK`, `ENV ANDROID OK` (et leurs formes `… EN ECHEC`).
- Noms des paquets : `ActCraft-<version>-setup.exe`, `ActCraft-<version>-arm64-v8a.apk`,
  `ActCraft-<version>-x86_64.apk` dans `build/paquet/` ; identifiant Android `io.github.jejeasterix.actcraft`.

## Règles communes à toutes les tâches (rappelées dans chaque section)

- Worktree de la tâche : `.worktrees/livraison-complete--L1-T<n>` (profil). Toute commande vise ce dossier :
  `Set-Location <worktree>;` en tête de chaque commande PowerShell, `git -C <worktree>` pour git. Toujours
  `pwsh -NoProfile`, jamais `powershell.exe`.
- Avant de relire des fichiers : `graphify query "…"` dans le worktree (carte du code, `graphify-out/`).
- Premier passage dans un worktree neuf : `pwsh -NoProfile -File ./scripts/build.ps1` (configure le preset
  `debug`), puis les commandes ciblées.
- **Commande ciblée** (test rouge, test vert d'un seul test) — jamais `ctest` ni `tst_*.exe` sans
  `dev-env.ps1` (sinon boîte « DLL introuvable » bloquante) :
  ```powershell
  Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -Command ". ./scripts/dev-env.ps1; cmake --build --preset debug *> .tmp/build.txt; ctest --preset debug -R '<motif>' *> .tmp/ctest.txt"; Select-String -Path .tmp/build.txt,.tmp/ctest.txt -Pattern 'error|tests passed|tests failed|No tests were found' | Select-Object -Last 4
  ```
- **Preuve de tâche (forme bornée du profil)**, puis le nom du test :
  ```powershell
  Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2
  Set-Location <worktree>; Select-String -Path .tmp/test.txt -Pattern '<nom_du_test> .*Passed' | Select-Object -Last 1
  ```
  Attendu : `100% tests passed, 0 tests failed out of N` avec N ≥ la valeur de la table, puis `TESTS OK`, puis la
  ligne `Passed` du test nommé.
- **Banc sans écran** (tâches qui touchent `render/`, `input/`, `tools/` ou `ui/`, profil « Banc de fluidité » et
  décision 35 : résultat, pas une porte) :
  ```powershell
  Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu logiciel -Scenario court -Taille 1280x800 -HorsEcran > .tmp/bench.txt 2>&1 ; Select-String -Path .tmp/bench.txt -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE' | Select-Object -Last 1
  Set-Location <worktree>; $j = Get-ChildItem bench/results/*.json | Sort-Object LastWriteTime | Select-Object -Last 1; Write-Output "$($j.Name) content_check.ok = $((Get-Content -Raw $j | ConvertFrom-Json).content_check.ok)"
  ```
  Attendu : `BUDGET TENU` ou `BUDGET DEPASSE` (jamais `MESURE INVALIDE`) ; le nom du JSON est joint au rapport.
- `pwsh -NoProfile -File ./scripts/format.ps1` avant la preuve (le contrôle de format fait partie de `TESTS OK`).
- Clang (CI macOS) active `-Wsign-conversion` et `-Wshadow` : indices de `std::vector` en `std::size_t`, de
  `QList` en `qsizetype`, conversions explicites (`static_cast`). MSVC `/W4 /WX` ne le voit pas : y penser.
- Textes de l'interface en français, écrits par nous (`qsTr`) ; rien de Promethean (icônes, libellés, textes,
  noms) ; termes propres à Promethean remplacés (N1-12). Captures, sorties d'essai et JSON du banc sous `.tmp/`,
  `build/` ou `bench/results/` (ignorés), jamais commités. Ne jamais toucher à `REA - Activinspire\samples\`.
- Commit en français, chemins explicites (`git -C <worktree> add <fichiers>`), jamais `git add -A`, terminé par
  `Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>`. Pas de push, sauf les branches
  `essai-ci/…` des tâches L1-T19 et L1-T20 (jamais `main`, jamais de tag `v*`).
- Hors périmètre : consigné dans le rapport, jamais corrigé. Un test existant ne se modifie que pour la raison
  écrite dans la section (comportement voulu qui change), jamais pour le faire passer.

---

## Tâches


<a id="tache-l1-t1"></a>
### Tâche L1-T1 — Profil du chantier : release due sous go, motifs d'action finale, durée du go, régime du banc, preuve visuelle

**Objectif.** Appliquer la décision 38 du cadrage avant toute tâche qui en dépend : (a) la release n'est plus
« due seulement quand Jérôme la demande » (il l'a demandée le 2026-10-14) mais proposée à chaque porte de fin de
lot, chaque publication restant sous son go écrit ; (b) le bloc `action-finale` tient aussi la publication du
brouillon (`gh release edit … --draft=false`) et les écritures de releases par `gh api` (POST, PATCH, DELETE) ;
(b′) le fichier `go-action-finale` reste en place après le tag jusqu'à la publication du brouillon (6 h au plus),
puis est supprimé ; (c) le régime du banc de la décision 35 (résultat par tâche, non-régression par lot, DA7) et
la commande de preuve visuelle de la décision 36 (annoncée ; sa forme bornée réelle est écrite par L1-T20).

**Fichiers.** Modifier : `.claude/chantier.md` (sections « Preuve visuelle », « Banc de fluidité », « Action finale
irréversible »). Fichier d'essai non commité : `.tmp/verifier-profil.js`.

**Test et commande bornée.** Le test est un script Node qui lit le bloc `action-finale` exactement comme
`~/.claude/hooks/interdits.js` (fonctions `bloc` et `regle`) et l'applique à 53 commandes (35 écritures à tenir,
18 lectures à laisser) :
```powershell
Set-Location <worktree>; node .tmp/verifier-profil.js > .tmp/profil.txt 2>&1 ; Select-String -Path .tmp/profil.txt -Pattern '^(ECART|PROFIL)' | Select-Object -Last 6
```
Rouge avant la modification : 31 lignes `ECART`, puis `PROFIL EN ECHEC`. Vert après : aucune ligne `ECART`
(`Select-String -Path .tmp/profil.txt -Pattern '^ECART' | Measure-Object` → 0), 53 lignes `OK`, dernière ligne
`PROFIL OK`.

**Interdits.** Ne toucher ni au bloc `interdits` ni aux autres sections. Ne pas affaiblir un motif existant. Ne
pas commiter `.tmp/verifier-profil.js`. Aucun push, aucun tag.

**Terminé quand.** `PROFIL OK` ; `Select-String -Path .claude/chantier.md -Pattern 'jusqu.à la publication du brouillon'`
trouve une ligne ; `Select-String -Path .claude/chantier.md -Pattern 'Due seulement quand'` n'en trouve aucune ;
commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire le test.** Créer `.tmp/verifier-profil.js` :

```js
// Vérifie le bloc action-finale de .claude/chantier.md comme ~/.claude/hooks/interdits.js le lit.
const fs = require("fs");
const texte = fs.readFileSync(".claude/chantier.md", "utf8");
function bloc(t, nom) {
  const m = t.match(new RegExp("```" + nom + "\\s*\\n([\\s\\S]*?)```"));
  return m ? m[1].split(/\r?\n/).map((l) => l.trim()).filter((l) => l && !l.startsWith("#")) : [];
}
const motifs = bloc(texte, "action-finale").map((l) => new RegExp(l.split(" :: ")[0]));
const tenue = (c) => motifs.some((r) => r.test(c));
// Toutes les formes d'écriture d'une release sont tenues, avec ou sans `git -C <dossier>` (règle des tâches) ;
// les lectures restent libres.
const bloquer = [
  "git tag v0.1.0", "git -C wt tag v0.1.0", "git tag -a v0.1.0 -m x", "git -C C:/a/b tag -a v0.1.0",
  "git push origin v0.1.0", "git -C wt push origin v0.1.0", "git push origin --tags", "git push -u origin v0.1.0",
  "git push origin HEAD:refs/tags/v0.1.0", "git push origin refs/tags/v0.1.0", "git push --follow-tags origin x",
  "git push origin tag v0.1.0",
  "gh release create v0.1.0 --draft", "gh release edit v0.1.0 --draft=false", "gh release edit v0.1.0 --draft false",
  "gh release edit v0.1.0 --draft=$false", "gh release upload v0.1.0 a.apk", "gh release delete v0.1.0",
  "gh api -X PATCH repos/jejeasterix/actcraft/releases/123 -F draft=false",
  "gh api --method=POST repos/jejeasterix/actcraft/releases",
  "gh api --method DELETE repos/jejeasterix/actcraft/releases/1",
  "gh api repos/jejeasterix/actcraft/releases -f tag_name=v0.1.0 -F draft=false",
  "gh api repos/jejeasterix/actcraft/releases --input corps.json",
  // Guillemets, `+`, `refs/tags/*`, `--mirror`, `.exe`, dossier à espaces, création de tag par l'API.
  "git tag \"v0.1.0\"", "git tag 'v0.1.0'", "git push origin \"v0.1.0\"", "git push origin 'refs/tags/v0.1.0'",
  "git push origin refs/tags/*", "git push origin +v0.1.0", "git push origin :v0.1.0", "git push --mirror origin",
  "git.exe tag v0.1.0", "gh.exe release create v0.1.0", "git -C \"C:/a b/c\" tag v0.1.0",
  "gh api repos/o/r/git/refs -f ref=refs/tags/v0.1.0 -f sha=abc",
];
const laisser = [
  "gh release view v0.1.0", "gh release list --limit 5", "gh api repos/jejeasterix/actcraft/releases",
  "gh api -X GET repos/jejeasterix/actcraft/releases", "gh api --method GET repos/x/y/releases -f per_page=5",
  "git tag --list", "git tag -l", "git -C wt log --oneline -1", "git push origin feat/livraison-complete",
  "git push origin HEAD:refs/heads/essai-ci/L1-T19", "git push origin --delete essai-ci/L1-T19",
  "gh api -X PATCH repos/jejeasterix/actcraft/pulls/3", "git -C wt push origin HEAD:refs/heads/essai-ci/release-L1-T20",
  "gh run list --branch essai-ci/release-L1-T20 --workflow release.yml",
  "git push origin HEAD:refs/heads/essai-ci/v2", "gh api repos/o/r/git/refs/tags",
  "git -C \"C:/a b/c\" log --oneline -1", "git push origin feat/x",
];
const cas = [...bloquer.map((c) => [c, true]), ...laisser.map((c) => [c, false])];
let ok = true;
for (const [commande, attendu] of cas) {
  const lu = tenue(commande);
  console.log((lu === attendu ? "OK    " : "ECART ") + commande + " (tenue : " + lu + ")");
  if (lu !== attendu) ok = false;
}
console.log(ok ? "PROFIL OK" : "PROFIL EN ECHEC");
process.exit(ok ? 0 : 1);
```

- [ ] **Étape 2 : le voir échouer.** Lancer la commande bornée (sortie entière :
`Get-Content .tmp/profil.txt`). Attendu avec le profil actuel : 31 lignes `ECART` (guillemets, `.exe`, `git -C`, `tag -a`, `--mirror`,
`--tags`, `-u`, `refs/tags`, `--follow-tags`, `push … tag`, `gh release edit/upload/delete`, les `gh api`
d'écriture), aucune lecture bloquée, puis `PROFIL EN ECHEC`.

- [ ] **Étape 3 : bloc `action-finale` et lignes qui le suivent.** Dans `.claude/chantier.md`, remplacer le bloc et
les deux premières puces qui le suivent (« Go requis », « Due seulement quand… ») par :

````markdown
```action-finale
\bgit(\.exe)?\b(\s+-[Cc]\s+("[^"]*"|'[^']*'|\S+))*\s+tag\b(?=.*[\s"']v\d) :: tag de release
\bgit(\.exe)?\b(\s+-[Cc]\s+("[^"]*"|'[^']*'|\S+))*\s+push\b(?=.*(refs/tags/|[\s"'+:]v\d|\s--tags\b|\s--follow-tags\b|\s--mirror\b|\stag\s)) :: publication du tag de release
\bgh(\.exe)?\s+release\s+(create|edit|upload|delete)\b :: écriture d'une release GitHub (création, publication du brouillon, fichiers, suppression)
\bgh(\.exe)?\s+api\b(?=.*(releases|refs/tags))(?!.*(-X|--method)[\s=]*(GET|get)\b)(?=.*(\s-X|\s--method|\s(-f|-F|--field|--raw-field|--input)[\s=])) :: écriture dans les releases ou les tags par l'API
```

- Go requis : oui — une release est publique et téléchargeable ; jamais sans accord explicite de Jérôme.
- Due : proposée à la porte de fin de chaque lot du chantier `livraison-complete` (Jérôme a demandé les releases
  `.exe` et `.apk` le 2026-10-14 ; décisions 32, 33 et 38 du cadrage
  `docs/specs/2026-10-14-livraison-complete-cadrage.md`). Chaque publication reste soumise à son go écrit. Le tag,
  sa poussée et la publication du brouillon (`gh release edit --draft=false` ou appel d'API) relèvent du **même**
  go. Aucune distribution hors de ce go : les paquets de chaque lot sont remis localement (`build/paquet/`), la CI
  les construit et les essaie sans les téléverser.
- Fichier go : `<git-common-dir>/chantiers/go-action-finale` est écrit au go, **reste en place après le tag jusqu'à
  la publication du brouillon** (le brouillon n'existe qu'à la fin du workflow `release.yml`), dans la limite de
  6 h que lit `interdits.js`, puis est supprimé aussitôt la publication faite. Passé 6 h sans publication, le go est
  redemandé à Jérôme.
````

Garder telles quelles les puces « Porte avant merge » et « CI couverte par la suite complète ». Les quatre motifs
remplacent les trois d'origine et les deux du cadrage (décision 38 b) : ils tiennent tout ce que ceux-ci tenaient,
plus les formes `git -C <dossier>`, `tag -a`, `--tags`, `--follow-tags`, `refs/tags/`, `gh release
upload|delete`, et `gh api` en écriture implicite (`-f`, `-F`, `--input` font un POST) ; les lectures restent
libres (vérifié sous node, 35 cas tenus, 18 lectures laissées, dont guillemets, `.exe`, `+v…`, `refs/tags/*`,
`--mirror`, dossier à espaces et création de tag par `gh api …/git/refs`). Écart au cadrage, plus strict, jamais
plus permissif (section « Écarts au cadrage »). **Résidus acceptés**, que des motifs ne peuvent pas tenir sans
bloquer aussi des lectures : tag passé par variable (`$t='v0.1.0'; git tag $t`), appel direct de l'API par `curl`
ou `Invoke-RestMethod`, mutation GraphQL (`gh api graphql`). Ils restent couverts par la règle écrite (go requis) et
sont rappelés à la porte de fin.

- [ ] **Étape 4 : section « Preuve visuelle ».** La remplacer entière par :

```markdown
## Preuve visuelle

Commande (décision 36 et DA8 du cadrage `docs/specs/2026-10-14-livraison-complete-cadrage.md`), livrée par la tâche
L1-T15 du chantier `livraison-complete` :
`pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario <nom>|tous [-Rendu d3d11|logiciel] [-HorsEcran] -Sortie <dossier> [-Lire "<objet>.<propriété>=<attendu>"]`.
Elle joue un scénario d'interface nommé (`bench/scenarios/<nom>.json` : commandes, stylet injecté, touches),
contrôle des pixels, lit des propriétés (lignes `LU …`, à lire avant les images) et écrit les captures PNG et un
`index.md`. Sorties sous `.tmp/` ou `build/`, jamais commitées. Hors écran en rendu logiciel pour chaque tâche
d'interface ; à l'écran en D3D11 en fin de lot, sous le jeton `ecran`. La forme bornée exacte est écrite ici par la
tâche L1-T20.

Tant que la commande n'existe pas : lancer `build/debug/actcraft.exe --rendu d3d11` puis `--rendu logiciel` ; les
captures et ce qu'il faut regarder vont à la porte de fin. Les fenêtres doivent s'ouvrir sur l'écran principal
(1920 × 1080, 4 moniteurs).
```

- [ ] **Étape 5 : section « Banc de fluidité ».** Remplacer le paragraphe qui commence par « **Pendant le chantier
`prototype-mesure`** » (jusqu'à « … de l'étape 2 du BRIEF. ») par :

```markdown
**Pendant le chantier `livraison-complete`** (décision 35 et DA7 du cadrage
`docs/specs/2026-10-14-livraison-complete-cadrage.md`, hypothèse N1-9) : le régime de la décision 38 du prototype
continue — le verdict du banc est un **résultat, pas une porte** — tant que l'étape 2 du BRIEF n'est pas faite.
Chaque tâche qui touche `render/`, `input/`, `tools/` ou `ui/` joint le JSON du banc sans écran (`-Rendu logiciel
-Scenario court -Taille 1280x800 -HorsEcran`). S'y ajoute une **porte de non-régression par lot** : en fin de lot,
banc complet D3D11 en plein écran sous le jeton `ecran` ; tout critère tenu par la référence doit l'être encore,
sinon une tâche de correction précède la clôture. Référence du lot 1 : `docs/mesures/2026-10-13-prototype-mesure.md` ;
la mesure de fin de lot 1, commitée dans `docs/mesures/`, devient la référence des lots suivants ; après le lot 10,
« figements » et « reprises » y entrent. La règle de refus ci-dessus s'applique après la décision go / no-go de
l'étape 2 du BRIEF.
```

- [ ] **Étape 6 : le voir passer.** Relancer la commande bornée. Attendu : 53 lignes `OK`, aucune `ECART`,
`PROFIL OK`. Puis :

```powershell
Set-Location <worktree>; Select-String -Path .claude/chantier.md -Pattern 'jusqu.à la publication du brouillon|Due seulement quand|résultat, pas une porte' | ForEach-Object { $_.Line.Trim() }
```
Attendu : la ligne du fichier go et celle du banc ; aucune ligne « Due seulement quand ».

- [ ] **Étape 7 : commit.**

```powershell
Set-Location <worktree>; git add .claude/chantier.md
git -C <worktree> commit -m "Profil : release proposée à chaque lot sous go, publication du brouillon tenue, banc et preuve visuelle du chantier" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

<a id="tache-l1-t2"></a>
### Tâche L1-T2 — Outillage des paquets : JDK 17, SDK/NDK, Qt 6.12 Android, Inno Setup, émulateur (WHPX sinon CI)

**Objectif.** Purger d'abord les risques d'outillage (décisions 26, 27, N2-8, N2-12) : installer sur le poste, sans
droits d'administrateur, JDK 17, le SDK Android (outils en ligne de commande, plateforme, build-tools, NDK à la
version exigée par Qt 6.12), Qt 6.12 pour Android (`android_arm64_v8a`, `android_x86_64`) par aqtinstall de
développement, Inno Setup 6, Pillow ; relever les versions (NDK, API minimale, build-tools, plateforme) dans
`packaging/android/versions.env` (lu par `dev-env-android.ps1` et par la CI en L1-T19) ; vérifier si l'émulateur
peut tourner localement (WHPX utilisable sans administrateur). Chaque résultat est une ligne `VERIF` d'un document
commité. Aucune ligne de code de l'application.

**Fichiers.** Créer : `scripts/dev-env-android.ps1`, `packaging/android/versions.env`,
`docs/verifications/2026-10-14-outillage-paquets.md`. Installations hors dépôt : `~/.tools/jdk-17`,
`~/.tools/android-sdk`, Inno Setup par utilisateur, `C:\Qt\6.12.0\android_*`.

**Test et commande bornée.** Pas de test unitaire (outillage). Preuves :
```powershell
Set-Location <worktree>; Select-String -Path docs/verifications/2026-10-14-outillage-paquets.md -Pattern '^VERIF ' | ForEach-Object { $_.Line }
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -Command ". ./scripts/dev-env-android.ps1; Write-Output 'ENV ANDROID OK'" > .tmp/env-android.txt 2>&1 ; Select-String -Path .tmp/env-android.txt -Pattern 'ENV ANDROID OK|introuvable' | Select-Object -Last 1
```
Attendu : 14 lignes `VERIF` (jdk17, sdk, build-tools, plateforme, ndk, api-min, qt-android-arm64,
qt-android-x86_64, qt-android-modules, inno, whpx, emulateur-local, avd, pillow), aucune avec `…`, puis
`ENV ANDROID OK`.

**Interdits.** Pas de droits d'administrateur, pas d'activation de fonctionnalité Windows (WHPX, Hyper-V) : si
WHPX n'est pas utilisable, le noter et passer (repli CI). Ne pas modifier `C:\Qt\6.12.0\msvc2022_64`. Ne rien
télécharger dans le dépôt. L'émulateur se ferme par `adb emu kill`, jamais par `taskkill`.

**Replis écrits (porte « installation »).**
- Qt pour Android ne s'installe pas sur le poste : `VERIF qt-android-* = ECHEC (<message>)` ; la CI (Ubuntu)
  reste la référence de construction (N2-8) et L1-T17 prouvera par la CI ; continuer.
- JDK, SDK ou NDK ne s'installent pas : même repli (CI), continuer.
- WHPX non utilisable : `VERIF whpx = non utilisable (…)`, `VERIF emulateur-local = non (WHPX)` ; l'émulateur
  tournera en CI (KVM sur `ubuntu-latest`, L1-T19). **Porte « installation »** seulement si l'émulateur de la CI
  échoue aussi (constaté en L1-T19) : rien à décider ici.
- Inno Setup ne s'installe pas par utilisateur : `VERIF inno = ECHEC (…)` ; L1-T16 s'arrêtera et le dira (porte
  « installation ») ; la CI Windows produit alors seule l'installateur.

**Terminé quand.** Le document a ses 14 lignes `VERIF`, `versions.env` ses cinq clés, `ENV ANDROID OK` (ou, en
repli, la ligne `introuvable` et sa raison au rapport) ; commit fait.

**Étapes.**

- [ ] **Étape 1 : relever les versions exigées par Qt 6.12.** Lire la page « Getting Started with Qt for Android »
(https://doc.qt.io/qt-6/android-getting-started.html, tableau des versions de JDK, NDK, build-tools et plateforme)
et le fichier de Qt Creator
https://code.qt.io/cgit/qt-creator/qt-creator.git/plain/share/qtcreator/android/sdk_definitions.json (entrée de la
version de Qt la plus proche de 6.12). Noter la version du NDK (forme `27.2.12479018`), celle des build-tools
(forme `35.0.0`) et la plateforme (forme `android-35`). Si les deux sources diffèrent : prendre la doc de Qt et
noter l'écart.

- [ ] **Étape 2 : JDK 17 (Temurin).**

```powershell
$outils = Join-Path $HOME '.tools'; New-Item -ItemType Directory -Force $outils | Out-Null
$zip = Join-Path ([IO.Path]::GetTempPath()) 'jdk17.zip'
Invoke-WebRequest 'https://api.adoptium.net/v3/binary/latest/17/ga/windows/x64/jdk/hotspot/normal/eclipse' -OutFile $zip
$tmp = Join-Path ([IO.Path]::GetTempPath()) 'jdk17-x'; Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
Expand-Archive $zip $tmp; Move-Item (Get-ChildItem $tmp -Directory | Select-Object -First 1).FullName (Join-Path $outils 'jdk-17')
& (Join-Path $outils 'jdk-17/bin/java.exe') -version
```
Attendu : `openjdk version "17.…"`. Ligne : `VERIF jdk17 = OK (17.x.y, ~/.tools/jdk-17)`.

- [ ] **Étape 3 : SDK Android.** Télécharger les outils en ligne de commande (lien « Command line tools only » de
https://developer.android.com/studio#command-line-tools-only, fichier `commandlinetools-win-<n>_latest.zip`), les
déplier pour que `sdkmanager.bat` soit dans `~/.tools/android-sdk/cmdline-tools/latest/bin/`, puis (valeurs entre
chevrons : étape 1) :

```powershell
$env:JAVA_HOME = Join-Path $HOME '.tools/jdk-17'; $sdk = Join-Path $HOME '.tools/android-sdk'
$sdkmanager = Join-Path $sdk 'cmdline-tools/latest/bin/sdkmanager.bat'
1..30 | ForEach-Object { 'y' } | & $sdkmanager --sdk_root=$sdk --licenses | Out-Null
& $sdkmanager --sdk_root=$sdk 'platform-tools' 'emulator' 'platforms;<plateforme>' 'build-tools;<build-tools>' 'ndk;<NDK>' 'system-images;android-34;google_apis;x86_64'
& $sdkmanager --sdk_root=$sdk --list_installed
```
Lignes : `VERIF sdk = OK (cmdline-tools <version>)`, `VERIF build-tools = <version>`, `VERIF plateforme =
<android-N>`, `VERIF ndk = <version> (source : doc Qt 6.12)`.

- [ ] **Étape 4 : Qt 6.12 pour Android.**

```powershell
python -m pip install --user --upgrade "git+https://github.com/miurahr/aqtinstall"
python -m aqt install-qt all_os android 6.12.0 android_arm64_v8a -m qtshadertools qtimageformats --outputdir C:\Qt
python -m aqt install-qt all_os android 6.12.0 android_x86_64 -m qtshadertools qtimageformats --outputdir C:\Qt
Test-Path C:\Qt\6.12.0\android_arm64_v8a\lib\cmake\Qt6\qt.toolchain.cmake, C:\Qt\6.12.0\android_x86_64\lib\cmake\Qt6\qt.toolchain.cmake
Get-ChildItem C:\Qt\6.12.0\android_arm64_v8a\lib -Filter 'libQt6*_arm64-v8a.so' | Where-Object Name -match 'Svg|QuickControls2_|QuickTemplates2' | ForEach-Object Name
```
Si `all_os` est refusé : `python -m aqt list-qt all_os android --arch 6.12.0`, à défaut
`python -m aqt list-qt windows android --arch 6.12.0`, et reprendre avec l'hôte qui répond (le noter). Lignes :
`VERIF qt-android-arm64 = OK`, `VERIF qt-android-x86_64 = OK`, `VERIF qt-android-modules = Svg, QuickControls2,
QuickTemplates2 présents` (ou la liste des absents). API minimale par défaut de Qt 6.12 :

```powershell
Select-String -Path C:\Qt\6.12.0\android_arm64_v8a\lib\cmake\Qt6Core\*.cmake, C:\Qt\6.12.0\android_arm64_v8a\src\android\templates\build.gradle -Pattern 'MIN_SDK|minSdk' | Select-Object -First 5
```
Ligne : `VERIF api-min = <N> (source : <fichier:ligne>)`.

- [ ] **Étape 5 : Inno Setup 6 et Pillow.**

```powershell
winget install --id JRSoftware.InnoSetup -e --scope user --silent --accept-package-agreements --accept-source-agreements
$iscc = @((Join-Path $env:LOCALAPPDATA 'Programs/Inno Setup 6/ISCC.exe'), (Join-Path $HOME '.tools/InnoSetup6/ISCC.exe')) | Where-Object { Test-Path $_ } | Select-Object -First 1
& $iscc /? | Select-Object -First 2
python -m pip install --user pillow; python -c "import PIL; print('Pillow', PIL.__version__)"
```
Si winget échoue : télécharger `https://jrsoftware.org/download.php/is.exe` dans le dossier temporaire et lancer
`is.exe /VERYSILENT /SUPPRESSMSGBOXES /CURRENTUSER /DIR="$HOME\.tools\InnoSetup6"`. Lignes :
`VERIF inno = OK (6.x, <chemin de ISCC.exe>)`, `VERIF pillow = OK (<version>)`.

- [ ] **Étape 6 : émulateur local.**

```powershell
$sdk = Join-Path $HOME '.tools/android-sdk'; & (Join-Path $sdk 'emulator/emulator.exe') -accel-check
```
Une sortie qui contient `WHPX` et `is installed and usable` : utilisable. Créer alors l'appareil virtuel et
l'essayer (sans fenêtre) :

```powershell
$env:JAVA_HOME = Join-Path $HOME '.tools/jdk-17'; $env:ANDROID_SDK_ROOT = $sdk
$avdmanager = Join-Path $sdk 'cmdline-tools/latest/bin/avdmanager.bat'; $adb = Join-Path $sdk 'platform-tools/adb.exe'
& $avdmanager list device | Select-String 'pixel_tablet'
'no' | & $avdmanager create avd -n actcraft-x86_64 -k 'system-images;android-34;google_apis;x86_64' -d pixel_tablet --force
$emu = Start-Process -FilePath (Join-Path $sdk 'emulator/emulator.exe') -ArgumentList '-avd', 'actcraft-x86_64', '-no-window', '-no-audio', '-no-snapshot', '-no-boot-anim', '-gpu', 'swiftshader_indirect' -PassThru
& $adb wait-for-device
$limite = (Get-Date).AddMinutes(6); do { Start-Sleep -Seconds 3; $b = (& $adb shell getprop sys.boot_completed) } until ("$b".Trim() -eq '1' -or (Get-Date) -gt $limite)
Write-Output "boot_completed = $b"
& $adb emu kill
```
Si `pixel_tablet` n'est pas listé, prendre la première tablette de la liste et la noter. Lignes :
`VERIF whpx = utilisable | non utilisable (<sortie de -accel-check>)`, `VERIF emulateur-local = démarre (boot en
<s> s) | non (<raison>)`, `VERIF avd = actcraft-x86_64, android-34 google_apis x86_64, appareil <id>`. Si WHPX n'est
pas utilisable : ne pas créer l'appareil, écrire `VERIF emulateur-local = non (WHPX)` et
`VERIF avd = non créé (émulateur en CI)`.

- [ ] **Étape 7 : `packaging/android/versions.env`.** Créer le fichier (une clé par ligne, sans espace autour de
`=`, lisible par `>> $GITHUB_ENV`) avec les valeurs des étapes 1 et 6 :

```text
ANDROID_NDK_VERSION=<NDK de l'étape 1, ex. 27.2.12479018>
ANDROID_BUILD_TOOLS=<build-tools de l'étape 1, ex. 35.0.0>
ANDROID_PLATFORM=<numéro seul de la plateforme, ex. 35>
ANDROID_EMULATOR_API=34
ANDROID_AVD_PROFILE=<appareil de l'étape 6, pixel_tablet par défaut>
```
Aucun chevron ne reste dans le fichier commité.

- [ ] **Étape 8 : `scripts/dev-env-android.ps1`.**

```powershell
# Prépare la chaîne Android dans la session courante (décision 27) : JDK 17, SDK, NDK, Qt hôte et Qt pour Android.
# Une variable déjà définie (CI) est gardée ; sinon, valeur du poste (~/.tools, C:/Qt). Échoue si un dossier manque.
# Usage : . ./scripts/dev-env-android.ps1
$ErrorActionPreference = 'Stop'
$outils = Join-Path $HOME '.tools'
$qtRacine = if ($IsWindows) { 'C:/Qt/6.12.0' } else { Join-Path $HOME 'Qt/6.12.0' }
$hote = if ($IsWindows) { 'msvc2022_64' } elseif ($IsMacOS) { 'macos' } else { 'gcc_64' }

function Set-ActCraftDefault([string]$Nom, [string]$Valeur) {
    if (-not [Environment]::GetEnvironmentVariable($Nom)) {
        [Environment]::SetEnvironmentVariable($Nom, $Valeur.Replace('\', '/'))
    }
}

Set-ActCraftDefault 'JAVA_HOME' (Join-Path $outils 'jdk-17')
Set-ActCraftDefault 'ANDROID_SDK_ROOT' (Join-Path $outils 'android-sdk')
if (-not $env:ANDROID_NDK_ROOT) {
    $versions = Join-Path $PSScriptRoot '../packaging/android/versions.env'
    $voulu = (Get-Content $versions | Where-Object { $_ -match '^ANDROID_NDK_VERSION=' }) -replace '^ANDROID_NDK_VERSION=', ''
    Set-ActCraftDefault 'ANDROID_NDK_ROOT' (Join-Path $env:ANDROID_SDK_ROOT "ndk/$voulu")
}
Set-ActCraftDefault 'ACTCRAFT_QT_HOST_DIR' (Join-Path $qtRacine $hote)
Set-ActCraftDefault 'ACTCRAFT_QT_ANDROID_ARM64_DIR' (Join-Path $qtRacine 'android_arm64_v8a')
Set-ActCraftDefault 'ACTCRAFT_QT_ANDROID_X86_64_DIR' (Join-Path $qtRacine 'android_x86_64')

foreach ($nom in 'JAVA_HOME', 'ANDROID_SDK_ROOT', 'ANDROID_NDK_ROOT', 'ACTCRAFT_QT_HOST_DIR',
    'ACTCRAFT_QT_ANDROID_ARM64_DIR', 'ACTCRAFT_QT_ANDROID_X86_64_DIR') {
    $valeur = [Environment]::GetEnvironmentVariable($nom)
    if (-not $valeur -or -not (Test-Path $valeur)) {
        throw "$nom introuvable ($valeur) : voir docs/verifications/2026-10-14-outillage-paquets.md"
    }
}

$sep = [IO.Path]::PathSeparator
$dossiers = @((Join-Path $env:JAVA_HOME 'bin'), (Join-Path $env:ANDROID_SDK_ROOT 'platform-tools'),
    (Join-Path $env:ANDROID_SDK_ROOT 'emulator'), (Join-Path $env:ANDROID_SDK_ROOT 'cmdline-tools/latest/bin'))
foreach ($d in $dossiers) {
    if ((Test-Path $d) -and -not (($env:PATH -split [regex]::Escape($sep)) -contains $d)) { $env:PATH = "$d$sep$env:PATH" }
}
# Ninja installé par pip, comme dans dev-env.ps1.
if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
    $python = if (Get-Command python3 -ErrorAction SilentlyContinue) { 'python3' } else { 'python' }
    $ninjaDir = & $python -c "import ninja, os; print(os.path.join(os.path.dirname(ninja.__file__), 'data', 'bin'))"
    $env:PATH = "$ninjaDir$sep$env:PATH"
}
```

- [ ] **Étape 9 : document de vérification.** Créer `docs/verifications/2026-10-14-outillage-paquets.md` :

```markdown
# Outillage des paquets — vérifications du 2026-10-14

Tâche L1-T2 du chantier `livraison-complete` (décisions 26, 27, 28 ; N2-8, N2-12 du cadrage
`docs/specs/2026-10-14-livraison-complete-cadrage.md`). Une ligne `VERIF` par point ; les commandes exactes sont
celles du plan `docs/plans/2026-10-14-livraison-complete-lot1-PLAN.md`, tâche L1-T2. Les tâches L1-T17 (APK) et
L1-T18 (émulateur) ajoutent leurs lignes plus bas.

## Résultats

VERIF jdk17 = …
VERIF sdk = …
VERIF build-tools = …
VERIF plateforme = …
VERIF ndk = …
VERIF api-min = …
VERIF qt-android-arm64 = …
VERIF qt-android-x86_64 = …
VERIF qt-android-modules = …
VERIF inno = …
VERIF whpx = …
VERIF emulateur-local = …
VERIF avd = …
VERIF pillow = …

## Conséquences

- Émulateur : local (WHPX) | en CI seulement (KVM sur `ubuntu-latest`, tâche L1-T19).
- Écarts entre la doc de Qt et `sdk_definitions.json` : …
```
Chaque `…` reçoit la valeur relevée aux étapes 1 à 6 ; aucun `…` ne reste dans le fichier commité.

- [ ] **Étape 10 : preuve.** Lancer les deux commandes bornées. Attendu : 14 lignes `VERIF` sans `…`, puis
`ENV ANDROID OK` (ou, en repli, la ligne `introuvable` et la raison au rapport).

- [ ] **Étape 11 : commit.**

```powershell
Set-Location <worktree>; git add scripts/dev-env-android.ps1 packaging/android/versions.env docs/verifications/2026-10-14-outillage-paquets.md
git -C <worktree> commit -m "Outillage des paquets : chaîne Android, Inno Setup et émulateur vérifiés" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

<a id="tache-l1-t3"></a>
### Tâche L1-T3 — `core` : squelette `Document` (pages, page suivante qui ajoute, empreinte)

**Objectif.** Squelette du modèle de document de DA2 et de la décision 15 : un `Document` est une suite de pages,
jamais vide ; « page suivante » sur la dernière page ajoute une page vierge de la même taille (F-125) ;
« page précédente » s'arrête à la première ; une empreinte du document (nombre de pages puis empreinte de chaque
page) servira aux allers-retours du lot 2. Sans QtGui (CMake le vérifie déjà pour `actcraft_core`).

**Fichiers.**
- Créer : `core/include/actcraft/core/document.h`, `core/src/document.cpp`, `tests/tst_core_document.cpp`
- Modifier : `core/CMakeLists.txt`, `tests/core.cmake`

**Test et commande bornée.** `tst_core_document` (6 fonctions). Rouge : commande ciblée, motif
`tst_core_document` → `error` (en-tête `document.h` introuvable). Vert : forme bornée, attendu `out of N` avec
N ≥ 30, `TESTS OK`, `tst_core_document .*Passed`.

**Interdits.** Ne pas toucher à `model.h` ni à `fingerprint(const Page&)` (empreinte figée du prototype). Pas de
QtGui dans `core/`.

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire le test.** Créer `tests/tst_core_document.cpp` :

```cpp
#include <actcraft/core/document.h>

#include <QTest>

using namespace actcraft::core;

namespace {

Stroke redStroke()
{
    Stroke s;
    s.points = {StrokePoint{10, 20, 0.5f}, StrokePoint{30, 40, 0.5f}};
    s.baseWidth = 4;
    s.color = 0xFFE53935u;
    return s;
}

} // namespace

// Squelette du document (DA2) : pages, navigation de F-125, empreinte.
class TstCoreDocument : public QObject
{
    Q_OBJECT

private slots:
    void newDocumentHasOneBlankPage()
    {
        Document d;
        QCOMPARE(d.pageCount(), std::size_t{1});
        QCOMPARE(d.page(0).width, 1024.0);
        QCOMPARE(d.page(0).height, 768.0);
        QVERIFY(d.page(0).objects.empty());
    }

    void nextOnLastPageAppendsBlankPage()
    {
        Document d;
        d.page(0).objects.emplace_back(redStroke());
        QCOMPARE(nextPageIndex(d, 0), std::size_t{1});
        QCOMPARE(d.pageCount(), std::size_t{2});
        QVERIFY(d.page(1).objects.empty());
        QCOMPARE(d.page(0).objects.size(), std::size_t{1});
    }

    void nextInsideDocumentDoesNotAppend()
    {
        Document d;
        d.appendPage();
        QCOMPARE(nextPageIndex(d, 0), std::size_t{1});
        QCOMPARE(d.pageCount(), std::size_t{2});
    }

    void appendedPageKeepsLastPageSize()
    {
        Document d;
        d.page(0).width = 800;
        d.page(0).height = 600;
        QCOMPARE(d.appendPage(), std::size_t{1});
        QCOMPARE(d.page(1).width, 800.0);
        QCOMPARE(d.page(1).height, 600.0);
    }

    void previousStopsAtFirstPage()
    {
        QCOMPARE(previousPageIndex(0), std::size_t{0});
        QCOMPARE(previousPageIndex(3), std::size_t{2});
    }

    void fingerprintFollowsPagesAndContent()
    {
        Document a;
        Document b;
        QCOMPARE(fingerprint(a), fingerprint(b));
        b.appendPage();
        QVERIFY(fingerprint(a) != fingerprint(b));
        Document c;
        c.page(0).objects.emplace_back(redStroke());
        QVERIFY(fingerprint(a) != fingerprint(c));
        Document d;
        d.page(0).objects.emplace_back(redStroke());
        QCOMPARE(fingerprint(c), fingerprint(d));
    }
};

QTEST_GUILESS_MAIN(TstCoreDocument)
#include "tst_core_document.moc"
```

Ajouter à la fin de `tests/core.cmake` :

```cmake
actcraft_add_test(tst_core_document
    SOURCES tst_core_document.cpp
    LIBS ActCraft::core)
```

- [ ] **Étape 2 : le voir échouer.** Commande ciblée, motif `tst_core_document`. Attendu : `error` (en-tête
`actcraft/core/document.h` introuvable).

- [ ] **Étape 3 : écrire `core/include/actcraft/core/document.h`.**

```cpp
#pragma once

#include <actcraft/core/model.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace actcraft::core {

// Squelette du document (DA2, décision 15 du cadrage livraison-complete) : une suite de pages, jamais vide.
// Couches, ressources et pile d'annulation arrivent au lot 2.
class Document
{
public:
    Document(); // une page vierge de 1024 × 768

    std::size_t pageCount() const noexcept;
    // Exige index < pageCount() (assert).
    const Page& page(std::size_t index) const;
    Page& page(std::size_t index);
    // Ajoute à la fin une page vierge de la taille de la dernière ; retourne son indice.
    std::size_t appendPage();

private:
    std::vector<Page> m_pages;
};

// « Page suivante » (F-125) : indice suivant ; sur la dernière page, ajoute d'abord une page vierge.
std::size_t nextPageIndex(Document& document, std::size_t current);
// « Page précédente » : indice précédent, borné à 0.
std::size_t previousPageIndex(std::size_t current) noexcept;

// Empreinte FNV-1a 64 bits : nombre de pages, puis fingerprint(page) de chaque page dans l'ordre.
std::uint64_t fingerprint(const Document& document) noexcept;

} // namespace actcraft::core
```

- [ ] **Étape 4 : écrire `core/src/document.cpp`.**

```cpp
#include <actcraft/core/document.h>

#include <cassert>
#include <utility>

namespace actcraft::core {

namespace {

// FNV-1a 64 bits sur les 8 octets de `value`, poids faible d'abord (même ordre que model.cpp).
std::uint64_t mix(std::uint64_t hash, std::uint64_t value) noexcept
{
    for (unsigned i = 0; i < 8; ++i) {
        hash ^= (value >> (8u * i)) & 0xFFu;
        hash *= 0x100000001B3ull;
    }
    return hash;
}

} // namespace

Document::Document() : m_pages(1) {}

std::size_t Document::pageCount() const noexcept
{
    return m_pages.size();
}

const Page& Document::page(std::size_t index) const
{
    assert(index < m_pages.size());
    return m_pages[index];
}

Page& Document::page(std::size_t index)
{
    assert(index < m_pages.size());
    return m_pages[index];
}

std::size_t Document::appendPage()
{
    Page blank;
    blank.width = m_pages.back().width;
    blank.height = m_pages.back().height;
    m_pages.push_back(std::move(blank));
    return m_pages.size() - 1;
}

std::size_t nextPageIndex(Document& document, std::size_t current)
{
    if (current + 1 < document.pageCount()) {
        return current + 1;
    }
    return document.appendPage();
}

std::size_t previousPageIndex(std::size_t current) noexcept
{
    return current == 0 ? 0 : current - 1;
}

std::uint64_t fingerprint(const Document& document) noexcept
{
    std::uint64_t hash = 0xCBF29CE484222325ull;
    hash = mix(hash, static_cast<std::uint64_t>(document.pageCount()));
    for (std::size_t i = 0; i < document.pageCount(); ++i) {
        hash = mix(hash, fingerprint(document.page(i)));
    }
    return hash;
}

} // namespace actcraft::core
```

Dans `core/CMakeLists.txt`, ajouter `src/document.cpp` après `src/generator.cpp` dans `SOURCES`.

- [ ] **Étape 5 : le voir passer.** Commande ciblée, motif `tst_core_document`. Attendu : `100% tests passed, 0
tests failed out of 1`.

- [ ] **Étape 6 : format et preuve.** `pwsh -NoProfile -File ./scripts/format.ps1`, puis la forme bornée et la ligne
`tst_core_document .*Passed`. Attendu : N ≥ 30, `TESTS OK`.

- [ ] **Étape 7 : commit.**

```powershell
Set-Location <worktree>; git add core/CMakeLists.txt core/include/actcraft/core/document.h core/src/document.cpp tests/core.cmake tests/tst_core_document.cpp
git -C <worktree> commit -m "core : squelette du document (pages, page suivante qui ajoute, empreinte)" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

<a id="tache-l1-t4"></a>
### Tâche L1-T4 — `actions` : registre des commandes nommées, déclarations JSON

**Objectif.** DA1 et décision 20 : un registre `CommandRegistry` (QtCore seulement) d'identifiants anglais à points,
avec libellé, icône, raccourci, cochable, plateformes, type d'argument (aucun, couleur, nombre, URL — chemin local
ou `content://`), état actif et coché, et un gestionnaire par commande ; `trigger` refuse, sans rien appeler, une
commande inconnue, absente de la plateforme, désactivée, sans gestionnaire ou dont l'argument ne convient pas. Les
13 commandes du lot 1 sont déclarées dans `actions/data/commands.json` (ressource intégrée
`:/actcraft/actions/commands.json`), libellés en français écrits par nous.

**Fichiers.**
- Créer : `actions/include/actcraft/actions/commandregistry.h`, `actions/src/commandregistry.cpp`,
  `actions/data/commands.json`, `tests/actions.cmake`, `tests/tst_actions_registry.cpp`
- Modifier : `actions/CMakeLists.txt`, `tests/CMakeLists.txt`

**Test et commande bornée.** `tst_actions_registry` (7 fonctions, une avec données). Rouge : commande ciblée,
motif `tst_actions_registry` → `error` (en-tête introuvable). Vert : forme bornée, N ≥ 30, `TESTS OK`,
`tst_actions_registry .*Passed`.

**Interdits.** Aucune dépendance à QtGui dans `actions/` (couleur validée par expression régulière, pas par
`QColor`). Aucun nom ni libellé repris de `docs/re/commands.json` de l'ancien dépôt (données extraites du
binaire) : identifiants et libellés sont les nôtres.

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire le test.** Créer `tests/tst_actions_registry.cpp` :

```cpp
#include <actcraft/actions/commandregistry.h>

#include <QSysInfo>
#include <QTest>
#include <QUrl>

#include <limits>

using namespace actcraft::actions;

namespace {

const QByteArray kTwoCommands = R"({
  "format": "actcraft-commands/1",
  "commands": [
    { "id": "page.next", "label": "Page suivante", "icon": "page-suivante", "shortcut": "PgDown" },
    { "id": "pen.color", "label": "Couleur du stylo", "argument": "color", "checkable": true,
      "platforms": ["windows", "android"] }
  ]
})";

Platforms bits(Platform a, Platform b)
{
    return static_cast<Platforms>(static_cast<Platforms>(a) | static_cast<Platforms>(b));
}

} // namespace

// Registre des commandes nommées (DA1, décision 20).
class TstActionsRegistry : public QObject
{
    Q_OBJECT

private slots:
    void loadsDeclarationsInOrder()
    {
        CommandRegistry r;
        QString error;
        QVERIFY2(r.loadDeclarations(kTwoCommands, error), qPrintable(error));
        QCOMPARE(r.ids(), (QStringList{QStringLiteral("page.next"), QStringLiteral("pen.color")}));
        const CommandInfo* next = r.find(QStringLiteral("page.next"));
        QVERIFY(next);
        QCOMPARE(next->label, QStringLiteral("Page suivante"));
        QCOMPARE(next->icon, QStringLiteral("page-suivante"));
        QCOMPARE(next->shortcut, QStringLiteral("PgDown"));
        QCOMPARE(next->platforms, kAllPlatforms);
        QVERIFY(next->argument == ArgumentKind::None);
        QVERIFY(!next->checkable);
        const CommandInfo* color = r.find(QStringLiteral("pen.color"));
        QVERIFY(color && color->checkable && color->argument == ArgumentKind::Color);
        QCOMPARE(color->platforms, bits(Platform::Windows, Platform::Android));
        QVERIFY(!r.find(QStringLiteral("absent")));
    }

    void rejectsWholeFileOnError_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<QString>("expected");
        QTest::newRow("json") << QByteArray("{") << QStringLiteral("JSON invalide");
        QTest::newRow("format") << QByteArray(R"({"format":"autre/1","commands":[]})")
                                << QStringLiteral("format");
        QTest::newRow("label") << QByteArray(R"({"format":"actcraft-commands/1","commands":[
            {"id":"a.b","label":"A"},{"id":"c.d"}]})")
                               << QStringLiteral("commande 2");
        QTest::newRow("doublon") << QByteArray(R"({"format":"actcraft-commands/1","commands":[
            {"id":"a.b","label":"A"},{"id":"a.b","label":"B"}]})")
                                 << QStringLiteral("déjà déclaré");
        QTest::newRow("plateforme") << QByteArray(R"({"format":"actcraft-commands/1","commands":[
            {"id":"a.b","label":"A","platforms":["amiga"]}]})")
                                    << QStringLiteral("plateforme inconnue");
        QTest::newRow("argument") << QByteArray(R"({"format":"actcraft-commands/1","commands":[
            {"id":"a.b","label":"A","argument":"image"}]})")
                                  << QStringLiteral("argument inconnu");
    }

    void rejectsWholeFileOnError()
    {
        QFETCH(QByteArray, json);
        QFETCH(QString, expected);
        CommandRegistry r;
        QString error;
        QVERIFY(!r.loadDeclarations(json, error));
        QVERIFY2(error.contains(expected), qPrintable(error));
        QVERIFY(r.ids().isEmpty());
    }

    void triggerCallsHandlerWithValidArgument()
    {
        CommandRegistry r;
        QString error;
        QVERIFY(r.loadDeclarations(kTwoCommands, error));
        QVariant received;
        int calls = 0;
        r.setHandler(QStringLiteral("pen.color"), [&](const QVariant& argument) {
            received = argument;
            ++calls;
        });
        QVERIFY(r.hasHandler(QStringLiteral("pen.color")));
        QVERIFY(r.trigger(QStringLiteral("pen.color"), QStringLiteral("#E53935"), Platform::Windows));
        QCOMPARE(calls, 1);
        QCOMPARE(received.toString(), QStringLiteral("#E53935"));
        QVERIFY(!r.trigger(QStringLiteral("pen.color"), QStringLiteral("rouge"), Platform::Windows));
        QCOMPARE(calls, 1);
    }

    void triggerRefusesWhatCannotRun()
    {
        CommandRegistry r;
        QString error;
        QVERIFY(r.loadDeclarations(kTwoCommands, error));
        int calls = 0;
        r.setHandler(QStringLiteral("pen.color"), [&](const QVariant&) { ++calls; });
        const QVariant red = QStringLiteral("#E53935");
        QVERIFY(!r.trigger(QStringLiteral("absent"), {}, Platform::Windows));
        QVERIFY(!r.trigger(QStringLiteral("page.next"), {}, Platform::Windows)); // sans gestionnaire
        QVERIFY(!r.trigger(QStringLiteral("pen.color"), red, Platform::MacOS));  // hors plateforme
        QVERIFY(!r.isAvailable(QStringLiteral("pen.color"), Platform::MacOS));
        QVERIFY(r.isAvailable(QStringLiteral("pen.color"), Platform::Android));
        r.setEnabled(QStringLiteral("pen.color"), false);
        QVERIFY(!r.trigger(QStringLiteral("pen.color"), red, Platform::Windows));
        r.setEnabled(QStringLiteral("pen.color"), true);
        QVERIFY(r.trigger(QStringLiteral("pen.color"), red, Platform::Windows));
        QCOMPARE(calls, 1);
    }

    void stateDefaultsAndChanges()
    {
        CommandRegistry r;
        QString error;
        QVERIFY(r.loadDeclarations(kTwoCommands, error));
        QVERIFY(r.isEnabled(QStringLiteral("page.next")));
        QVERIFY(!r.isChecked(QStringLiteral("pen.color")));
        r.setChecked(QStringLiteral("pen.color"), true);
        QVERIFY(r.isChecked(QStringLiteral("pen.color")));
        QVERIFY(!r.isEnabled(QStringLiteral("absent")));
        QVERIFY(!r.declare(CommandInfo{QStringLiteral("page.next"), QStringLiteral("Doublon")}));
        QVERIFY(!r.declare(CommandInfo{}));
    }

    void argumentValidation()
    {
        QVERIFY(argumentIsValid(ArgumentKind::Color, QStringLiteral("#e53935")));
        QVERIFY(argumentIsValid(ArgumentKind::Color, QStringLiteral("#80E53935")));
        QVERIFY(!argumentIsValid(ArgumentKind::Color, QStringLiteral("e53935")));
        QVERIFY(!argumentIsValid(ArgumentKind::Color, QVariant(12)));
        QVERIFY(argumentIsValid(ArgumentKind::Number, QVariant(8)));
        QVERIFY(argumentIsValid(ArgumentKind::Number, QVariant(2.5)));
        QVERIFY(!argumentIsValid(ArgumentKind::Number, QVariant()));
        QVERIFY(!argumentIsValid(ArgumentKind::Number, QVariant(true)));
        QVERIFY(!argumentIsValid(ArgumentKind::Number, QVariant(std::numeric_limits<double>::quiet_NaN())));
        QVERIFY(argumentIsValid(ArgumentKind::Url, QStringLiteral("content://com.android.providers/document/12")));
        QVERIFY(argumentIsValid(ArgumentKind::Url, QStringLiteral("C:/Documents/essai.actcraft")));
        QVERIFY(argumentIsValid(ArgumentKind::Url, QUrl::fromLocalFile(QStringLiteral("/tmp/a.actcraft"))));
        QVERIFY(!argumentIsValid(ArgumentKind::Url, QString()));
        QVERIFY(!argumentIsValid(ArgumentKind::Url, QUrl()));
        QVERIFY(argumentIsValid(ArgumentKind::None, QVariant(42)));
    }

    void platformsByName()
    {
        QCOMPARE(platformBit(QStringLiteral("windows")), static_cast<Platforms>(Platform::Windows));
        QCOMPARE(platformBit(QStringLiteral("android")), static_cast<Platforms>(Platform::Android));
        QCOMPARE(platformBit(QStringLiteral("macos")), static_cast<Platforms>(Platform::MacOS));
        QCOMPARE(platformBit(QStringLiteral("linux")), static_cast<Platforms>(Platform::Linux));
        QCOMPARE(platformBit(QStringLiteral("amiga")), Platforms{0});
        QCOMPARE(platformName(Platform::Android), QStringLiteral("android"));
        QVERIFY(!platformName(currentPlatform()).isEmpty());
        if (QSysInfo::productType() == QLatin1String("windows")) {
            QVERIFY(currentPlatform() == Platform::Windows);
        }
    }

    void defaultCommandsFileDeclaresLotOne()
    {
        const QByteArray json = defaultCommandsJson();
        QVERIFY2(!json.isEmpty(), "ressource :/actcraft/actions/commands.json absente");
        CommandRegistry r;
        QString error;
        QVERIFY2(r.loadDeclarations(json, error), qPrintable(error));
        const QStringList expected{
            QStringLiteral("file.quit"),     QStringLiteral("view.fullscreen"), QStringLiteral("view.fit"),
            QStringLiteral("view.zoom"),     QStringLiteral("page.previous"),   QStringLiteral("page.next"),
            QStringLiteral("tool.pen"),      QStringLiteral("pen.color"),       QStringLiteral("pen.width"),
            QStringLiteral("menu.main"),     QStringLiteral("menu.tools"),      QStringLiteral("help.website"),
            QStringLiteral("help.about")};
        QCOMPARE(r.ids(), expected);
        QCOMPARE(r.find(QStringLiteral("view.fullscreen"))->shortcut, QStringLiteral("F5"));
        QVERIFY(r.find(QStringLiteral("view.fullscreen"))->checkable);
        QVERIFY(r.find(QStringLiteral("pen.width"))->argument == ArgumentKind::Number);
        QVERIFY(r.find(QStringLiteral("pen.color"))->argument == ArgumentKind::Color);
        QVERIFY(r.find(QStringLiteral("view.zoom"))->argument == ArgumentKind::Number);
    }
};

QTEST_GUILESS_MAIN(TstActionsRegistry)
#include "tst_actions_registry.moc"
```

Créer `tests/actions.cmake` :

```cmake
# Tests du module actions : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).

actcraft_add_test(tst_actions_registry
    SOURCES tst_actions_registry.cpp
    LIBS ActCraft::actions)
```

Dans `tests/CMakeLists.txt`, après la ligne `include(${CMAKE_CURRENT_SOURCE_DIR}/core.cmake)`, ajouter :

```cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/actions.cmake)
```

- [ ] **Étape 2 : le voir échouer.** Commande ciblée, motif `tst_actions_registry`. Attendu : `error`
(`actcraft/actions/commandregistry.h` introuvable).

- [ ] **Étape 3 : écrire `actions/include/actcraft/actions/commandregistry.h`.**

```cpp
#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace actcraft::actions {

// Plateformes où une commande existe (DA10) : un bit chacune.
enum class Platform : std::uint8_t
{
    Windows = 1,
    Android = 2,
    MacOS = 4,
    Linux = 8,
};
using Platforms = std::uint8_t;
inline constexpr Platforms kAllPlatforms = 0x0F;

// « windows », « android », « macos », « linux » → bit ; 0 si le nom est inconnu.
Platforms platformBit(const QString& name) noexcept;
// Plateforme d'exécution, lue par QSysInfo::productType() (pas de #ifdef, DA10).
Platform currentPlatform();
QString platformName(Platform platform);

enum class ArgumentKind : std::uint8_t
{
    None,   // aucun argument (tout argument est ignoré)
    Color,  // « #RRGGBB » ou « #AARRGGBB »
    Number, // réel fini
    Url,    // texte non vide formant une URL valide : chemin local ou content:// (décisions 17 et 20)
};

struct CommandInfo
{
    QString id;       // anglais, à points : « page.next »
    QString label;    // français, écrit par nous
    QString icon;     // nom d'icône sans extension (ui/icons/<icon>.svg) ; vide = sans icône
    QString shortcut; // QKeySequence::PortableText ; vide = aucun
    bool checkable = false;
    Platforms platforms = kAllPlatforms;
    ArgumentKind argument = ArgumentKind::None;
};

bool argumentIsValid(ArgumentKind kind, const QVariant& argument);

// Registre central des commandes nommées (DA1, décision 20). Sans interface : QtCore seulement.
class CommandRegistry
{
public:
    using Handler = std::function<void(const QVariant& argument)>;

    // Fichier « actcraft-commands/1 ». Tout ou rien : en cas d'erreur, rien n'est déclaré et `error` dit
    // pourquoi (« commande 2 (c.d) : label manquant »).
    bool loadDeclarations(const QByteArray& json, QString& error);
    // Faux si l'identifiant est vide ou déjà déclaré.
    bool declare(const CommandInfo& info);
    const CommandInfo* find(const QString& id) const;
    QStringList ids() const; // ordre de déclaration
    bool isAvailable(const QString& id, Platform platform) const;

    void setHandler(const QString& id, Handler handler);
    bool hasHandler(const QString& id) const;
    void setEnabled(const QString& id, bool enabled);
    bool isEnabled(const QString& id) const; // vrai par défaut ; faux pour une commande inconnue
    void setChecked(const QString& id, bool checked);
    bool isChecked(const QString& id) const; // faux par défaut

    // Faux, sans rien appeler, si la commande est inconnue, absente de `platform`, désactivée, sans
    // gestionnaire, ou si l'argument ne convient pas. Le gestionnaire est copié avant l'appel : il peut
    // modifier le registre.
    bool trigger(const QString& id, const QVariant& argument, Platform platform) const;

private:
    struct Entry
    {
        CommandInfo info;
        Handler handler;
        bool enabled = true;
        bool checked = false;
    };
    Entry* entry(const QString& id);
    const Entry* entry(const QString& id) const;

    std::vector<Entry> m_entries;
    QHash<QString, std::size_t> m_index;
};

// Dossier des données intégrées du module (« :/actcraft/actions »), ressources initialisées.
QString dataResourceDir();
// Contenu de :/actcraft/actions/commands.json (vide si la ressource manque).
QByteArray defaultCommandsJson();

} // namespace actcraft::actions
```

- [ ] **Étape 4 : écrire `actions/src/commandregistry.cpp`.**

```cpp
#include <actcraft/actions/commandregistry.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QSysInfo>
#include <QUrl>

#include <cmath>
#include <utility>

// Ressources du module (qt_add_resources « actions_data ») : Q_INIT_RESOURCE hors de tout espace de noms.
static void initActionsResources()
{
    Q_INIT_RESOURCE(actions_data);
}

namespace actcraft::actions {

namespace {

bool parseArgumentKind(const QString& text, ArgumentKind& out)
{
    if (text.isEmpty() || text == QLatin1String("none")) {
        out = ArgumentKind::None;
    } else if (text == QLatin1String("color")) {
        out = ArgumentKind::Color;
    } else if (text == QLatin1String("number")) {
        out = ArgumentKind::Number;
    } else if (text == QLatin1String("url")) {
        out = ArgumentKind::Url;
    } else {
        return false;
    }
    return true;
}

} // namespace

Platforms platformBit(const QString& name) noexcept
{
    if (name == QLatin1String("windows")) {
        return static_cast<Platforms>(Platform::Windows);
    }
    if (name == QLatin1String("android")) {
        return static_cast<Platforms>(Platform::Android);
    }
    if (name == QLatin1String("macos")) {
        return static_cast<Platforms>(Platform::MacOS);
    }
    if (name == QLatin1String("linux")) {
        return static_cast<Platforms>(Platform::Linux);
    }
    return 0;
}

Platform currentPlatform()
{
    const QString type = QSysInfo::productType();
    if (type == QLatin1String("windows")) {
        return Platform::Windows;
    }
    if (type == QLatin1String("android")) {
        return Platform::Android;
    }
    if (type == QLatin1String("macos") || type == QLatin1String("osx")) {
        return Platform::MacOS;
    }
    return Platform::Linux;
}

QString platformName(Platform platform)
{
    switch (platform) {
    case Platform::Windows:
        return QStringLiteral("windows");
    case Platform::Android:
        return QStringLiteral("android");
    case Platform::MacOS:
        return QStringLiteral("macos");
    case Platform::Linux:
        return QStringLiteral("linux");
    }
    return QStringLiteral("linux");
}

bool argumentIsValid(ArgumentKind kind, const QVariant& argument)
{
    switch (kind) {
    case ArgumentKind::None:
        return true;
    case ArgumentKind::Color: {
        static const QRegularExpression pattern(QStringLiteral("^#([0-9A-Fa-f]{6}|[0-9A-Fa-f]{8})$"));
        return argument.typeId() == QMetaType::QString && pattern.match(argument.toString()).hasMatch();
    }
    case ArgumentKind::Number: {
        if (!argument.isValid() || argument.typeId() == QMetaType::Bool) {
            return false;
        }
        bool ok = false;
        const double value = argument.toDouble(&ok);
        return ok && std::isfinite(value);
    }
    case ArgumentKind::Url: {
        if (argument.typeId() == QMetaType::QUrl) {
            const QUrl url = argument.toUrl();
            return url.isValid() && !url.isEmpty();
        }
        if (argument.typeId() != QMetaType::QString) {
            return false;
        }
        const QString text = argument.toString();
        return !text.isEmpty() && QUrl(text).isValid();
    }
    }
    return false;
}

bool CommandRegistry::loadDeclarations(const QByteArray& json, QString& error)
{
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("JSON invalide : %1").arg(parseError.errorString());
        return false;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() != QLatin1String("actcraft-commands/1")) {
        error = QStringLiteral("format inconnu (attendu actcraft-commands/1)");
        return false;
    }
    const QJsonArray commands = root.value(QStringLiteral("commands")).toArray();
    std::vector<CommandInfo> parsed;
    QSet<QString> seen;
    for (qsizetype i = 0; i < commands.size(); ++i) {
        const QJsonObject o = commands.at(i).toObject();
        CommandInfo info;
        info.id = o.value(QStringLiteral("id")).toString();
        info.label = o.value(QStringLiteral("label")).toString();
        info.icon = o.value(QStringLiteral("icon")).toString();
        info.shortcut = o.value(QStringLiteral("shortcut")).toString();
        info.checkable = o.value(QStringLiteral("checkable")).toBool(false);
        const QString where = QStringLiteral("commande %1 (%2)").arg(i + 1).arg(info.id);
        if (info.id.isEmpty()) {
            error = QStringLiteral("commande %1 : id manquant").arg(i + 1);
            return false;
        }
        if (info.label.isEmpty()) {
            error = where + QStringLiteral(" : label manquant");
            return false;
        }
        if (seen.contains(info.id) || m_index.contains(info.id)) {
            error = where + QStringLiteral(" : identifiant déjà déclaré");
            return false;
        }
        if (o.contains(QStringLiteral("platforms"))) {
            info.platforms = 0;
            const QJsonArray names = o.value(QStringLiteral("platforms")).toArray();
            for (const QJsonValue& name : names) {
                const Platforms bit = platformBit(name.toString());
                if (bit == 0) {
                    error = where + QStringLiteral(" : plateforme inconnue « %1 »").arg(name.toString());
                    return false;
                }
                info.platforms = static_cast<Platforms>(info.platforms | bit);
            }
            if (info.platforms == 0) {
                error = where + QStringLiteral(" : aucune plateforme");
                return false;
            }
        }
        const QString argument = o.value(QStringLiteral("argument")).toString();
        if (!parseArgumentKind(argument, info.argument)) {
            error = where + QStringLiteral(" : argument inconnu « %1 »").arg(argument);
            return false;
        }
        seen.insert(info.id);
        parsed.push_back(std::move(info));
    }
    for (const CommandInfo& info : parsed) {
        declare(info);
    }
    return true;
}

bool CommandRegistry::declare(const CommandInfo& info)
{
    if (info.id.isEmpty() || m_index.contains(info.id)) {
        return false;
    }
    m_index.insert(info.id, m_entries.size());
    Entry e;
    e.info = info;
    m_entries.push_back(std::move(e));
    return true;
}

CommandRegistry::Entry* CommandRegistry::entry(const QString& id)
{
    const auto it = m_index.constFind(id);
    return it == m_index.cend() ? nullptr : &m_entries[it.value()];
}

const CommandRegistry::Entry* CommandRegistry::entry(const QString& id) const
{
    const auto it = m_index.constFind(id);
    return it == m_index.cend() ? nullptr : &m_entries[it.value()];
}

const CommandInfo* CommandRegistry::find(const QString& id) const
{
    const Entry* e = entry(id);
    return e ? &e->info : nullptr;
}

QStringList CommandRegistry::ids() const
{
    QStringList out;
    out.reserve(static_cast<qsizetype>(m_entries.size()));
    for (const Entry& e : m_entries) {
        out << e.info.id;
    }
    return out;
}

bool CommandRegistry::isAvailable(const QString& id, Platform platform) const
{
    const Entry* e = entry(id);
    return e && (e->info.platforms & static_cast<Platforms>(platform)) != 0;
}

void CommandRegistry::setHandler(const QString& id, Handler handler)
{
    if (Entry* e = entry(id)) {
        e->handler = std::move(handler);
    }
}

bool CommandRegistry::hasHandler(const QString& id) const
{
    const Entry* e = entry(id);
    return e && static_cast<bool>(e->handler);
}

void CommandRegistry::setEnabled(const QString& id, bool enabled)
{
    if (Entry* e = entry(id)) {
        e->enabled = enabled;
    }
}

bool CommandRegistry::isEnabled(const QString& id) const
{
    const Entry* e = entry(id);
    return e && e->enabled;
}

void CommandRegistry::setChecked(const QString& id, bool checked)
{
    if (Entry* e = entry(id)) {
        e->checked = checked;
    }
}

bool CommandRegistry::isChecked(const QString& id) const
{
    const Entry* e = entry(id);
    return e && e->checked;
}

bool CommandRegistry::trigger(const QString& id, const QVariant& argument, Platform platform) const
{
    const Entry* e = entry(id);
    if (!e || !isAvailable(id, platform) || !e->enabled || !e->handler ||
        !argumentIsValid(e->info.argument, argument)) {
        return false;
    }
    const Handler handler = e->handler; // copie : le gestionnaire peut modifier le registre
    handler(argument);
    return true;
}

QString dataResourceDir()
{
    static const bool initialised = [] {
        initActionsResources();
        return true;
    }();
    Q_UNUSED(initialised)
    return QStringLiteral(":/actcraft/actions");
}

QByteArray defaultCommandsJson()
{
    QFile file(dataResourceDir() + QStringLiteral("/commands.json"));
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

} // namespace actcraft::actions
```

- [ ] **Étape 5 : écrire `actions/data/commands.json`.**

```json
{
  "format": "actcraft-commands/1",
  "commands": [
    { "id": "file.quit", "label": "Quitter", "icon": "quitter", "shortcut": "Ctrl+Q" },
    { "id": "view.fullscreen", "label": "Plein écran", "icon": "plein-ecran", "shortcut": "F5", "checkable": true },
    { "id": "view.fit", "label": "Ajuster la page", "icon": "ajuster" },
    { "id": "view.zoom", "label": "Échelle", "argument": "number" },
    { "id": "page.previous", "label": "Page précédente", "icon": "page-precedente", "shortcut": "PgUp" },
    { "id": "page.next", "label": "Page suivante", "icon": "page-suivante", "shortcut": "PgDown" },
    { "id": "tool.pen", "label": "Stylo", "icon": "stylo", "shortcut": "Ctrl+Shift+P", "checkable": true },
    { "id": "pen.color", "label": "Couleur du stylo", "argument": "color" },
    { "id": "pen.width", "label": "Épaisseur du stylo", "argument": "number" },
    { "id": "menu.main", "label": "Menu principal", "icon": "menu" },
    { "id": "menu.tools", "label": "Outils", "icon": "outils" },
    { "id": "help.website", "label": "Site du projet", "icon": "aide", "shortcut": "F1" },
    { "id": "help.about", "label": "À propos d'ActCraft", "icon": "a-propos" }
  ]
}
```

Remplacer `actions/CMakeLists.txt` par :

```cmake
# Registre central des commandes nommées ; menus, raccourcis et barres décrits par fichiers de données.
actcraft_add_module(actions
    SOURCES
        src/actions.cpp
        src/commandregistry.cpp
    DEPENDS
        ActCraft::core)

# Déclarations des commandes et disposition par défaut (DA1), lues sous :/actcraft/actions.
qt_add_resources(actcraft_actions "actions_data"
    PREFIX "/actcraft/actions"
    BASE data
    FILES
        data/commands.json)
```

- [ ] **Étape 6 : le voir passer.** Commande ciblée, motif `tst_actions_registry`. Attendu : `0 tests failed out of
1`. Si `defaultCommandsFileDeclaresLotOne` échoue sur « ressource absente » alors que le fichier est bien listé :
vérifier dans `build/debug/actions/` qu'un objet `actions_data` est produit et que l'appel
`Q_INIT_RESOURCE(actions_data)` compile (nom de ressource identique au deuxième argument de `qt_add_resources`).

- [ ] **Étape 7 : format et preuve.** `pwsh -NoProfile -File ./scripts/format.ps1`, forme bornée, ligne
`tst_actions_registry .*Passed`. Attendu : N ≥ 30, `TESTS OK`.

- [ ] **Étape 8 : commit.**

```powershell
Set-Location <worktree>; git add actions/CMakeLists.txt actions/include/actcraft/actions/commandregistry.h actions/src/commandregistry.cpp actions/data/commands.json tests/CMakeLists.txt tests/actions.cmake tests/tst_actions_registry.cpp
git -C <worktree> commit -m "actions : registre des commandes nommées et déclarations du lot 1" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

<a id="tache-l1-t5"></a>
### Tâche L1-T5 — `actions` : disposition (menus, palette, raccourcis, couleurs, épaisseurs), profil d'abord

**Objectif.** Règle 3 du BRIEF, DA1 et N2-5 : menus, groupes de la palette, raccourcis remplacés, 16 couleurs de
pastilles (F-133) et 4 épaisseurs du stylo (F-137 : 2, 4, 6, 8) sont décrits dans `layout.json`, lu d'abord dans
le dossier du profil, sinon dans les ressources (`:/actcraft/actions/layout.json`) : réorganiser l'interface ne
demande pas de recompiler. Les six menus du cadrage (Fichier, Éditer, Affichage, Insérer, Outils, Aide) sont
déclarés ; ceux qui n'ont encore aucune commande livrée restent vides (le pont QML de L1-T10 ne les affiche pas).
Couleurs à nous, dans l'ordre de F-133 (rouge, orange, jaune, vert / cyan, bleu, violet, magenta / brun, chair,
lavande, bordeaux / noir, gris foncé, gris, blanc).

**Fichiers.**
- Créer : `actions/include/actcraft/actions/layout.h`, `actions/src/layout.cpp`, `actions/data/layout.json`,
  `tests/tst_actions_layout.cpp`
- Modifier : `actions/CMakeLists.txt`, `tests/actions.cmake`

**Test et commande bornée.** `tst_actions_layout` (6 fonctions, une avec données). Rouge : commande ciblée, motif
`tst_actions_layout` → `error` (`layout.h` introuvable). Vert : forme bornée, N ≥ 31, `TESTS OK`,
`tst_actions_layout .*Passed`.

**Interdits.** Pas de QtGui. Pas d'ordre ni de libellé de menu repris des données extraites d'ActivInspire
(`docs/re/`) : la disposition est la nôtre.

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire le test.** Créer `tests/tst_actions_layout.cpp` :

```cpp
#include <actcraft/actions/commandregistry.h>
#include <actcraft/actions/layout.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace actcraft::actions;

namespace {

const QByteArray kLayout = R"({
  "format": "actcraft-layout/1",
  "menus": [ { "id": "file", "label": "Fichier", "items": ["file.quit", "-", "help.about"] } ],
  "palette": [ { "group": "pages", "items": ["page.next"] }, { "group": "couleurs" } ],
  "shortcuts": { "page.next": "Space" },
  "colors": ["#E53935", "#000000"],
  "penWidths": [2, 4]
})";

bool writeFile(const QString& path, const QByteArray& data)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}

} // namespace

// Disposition décrite par des données (règle 3 du BRIEF, DA1, N2-5).
class TstActionsLayout : public QObject
{
    Q_OBJECT

private slots:
    void parsesEverySection()
    {
        Layout layout;
        QString error;
        QVERIFY2(parseLayout(kLayout, layout, error), qPrintable(error));
        QCOMPARE(layout.menus.size(), std::size_t{1});
        QCOMPARE(layout.menus[0].id, QStringLiteral("file"));
        QCOMPARE(layout.menus[0].label, QStringLiteral("Fichier"));
        QCOMPARE(layout.menus[0].items,
                 (QStringList{QStringLiteral("file.quit"), QStringLiteral("-"), QStringLiteral("help.about")}));
        QCOMPARE(layout.palette.size(), std::size_t{2});
        QCOMPARE(layout.palette[0].group, QStringLiteral("pages"));
        QVERIFY(layout.palette[1].items.isEmpty());
        QCOMPARE(layout.shortcuts.value(QStringLiteral("page.next")), QStringLiteral("Space"));
        QCOMPARE(layout.colors, (QStringList{QStringLiteral("#E53935"), QStringLiteral("#000000")}));
        QCOMPARE(layout.penWidths, (QList<double>{2, 4}));
    }

    void rejectsInvalidLayouts_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<QString>("expected");
        QTest::newRow("format") << QByteArray(R"({"format":"actcraft-layout/9"})") << QStringLiteral("format");
        QTest::newRow("menu") << QByteArray(R"({"format":"actcraft-layout/1","menus":[{"id":"file"}]})")
                              << QStringLiteral("menu 1");
        QTest::newRow("couleur") << QByteArray(R"({"format":"actcraft-layout/1","colors":["rouge"]})")
                                 << QStringLiteral("couleur invalide");
        QTest::newRow("epaisseur") << QByteArray(R"({"format":"actcraft-layout/1","penWidths":[0]})")
                                   << QStringLiteral("épaisseur invalide");
        QTest::newRow("groupe") << QByteArray(R"({"format":"actcraft-layout/1","palette":[{"items":[]}]})")
                                << QStringLiteral("groupe 1");
    }

    void rejectsInvalidLayouts()
    {
        QFETCH(QByteArray, json);
        QFETCH(QString, expected);
        Layout layout;
        layout.colors << QStringLiteral("#123456");
        QString error;
        QVERIFY(!parseLayout(json, layout, error));
        QVERIFY2(error.contains(expected), qPrintable(error));
        QCOMPARE(layout.colors, QStringList{QStringLiteral("#123456")}); // inchangée en cas d'erreur
    }

    void profileFileWinsOverResource()
    {
        QTemporaryDir profile;
        QVERIFY(profile.isValid());
        QVERIFY(writeFile(profile.filePath(QStringLiteral("layout.json")), kLayout));
        QString used;
        const QByteArray data =
            readLayeredFile(QStringLiteral("layout.json"), profile.path(), dataResourceDir(), &used);
        QCOMPARE(used, profile.path() + QStringLiteral("/layout.json"));
        QCOMPARE(data, kLayout);
    }

    void fallsBackToResourceThenToNothing()
    {
        const QString absent = QDir::temp().filePath(QStringLiteral("actcraft-profil-absent-4f2a"));
        QString used;
        QVERIFY(!readLayeredFile(QStringLiteral("layout.json"), absent, dataResourceDir(), &used).isEmpty());
        QCOMPARE(used, dataResourceDir() + QStringLiteral("/layout.json"));
        QVERIFY(readLayeredFile(QStringLiteral("absent.json"), absent, dataResourceDir(), &used).isEmpty());
        QVERIFY(used.isEmpty());
    }

    void defaultLayoutMatchesLotOne()
    {
        Layout layout;
        QString error;
        const QByteArray json = readLayeredFile(QStringLiteral("layout.json"), QString(), dataResourceDir());
        QVERIFY2(parseLayout(json, layout, error), qPrintable(error));
        QStringList labels;
        for (const MenuSpec& menu : layout.menus) {
            labels << menu.label;
        }
        QCOMPARE(labels, (QStringList{QStringLiteral("Fichier"), QStringLiteral("Éditer"),
                                      QStringLiteral("Affichage"), QStringLiteral("Insérer"),
                                      QStringLiteral("Outils"), QStringLiteral("Aide")}));
        QStringList groups;
        for (const PaletteGroup& group : layout.palette) {
            groups << group.group;
        }
        QCOMPARE(groups, (QStringList{QStringLiteral("commandes"), QStringLiteral("pages"),
                                      QStringLiteral("couleurs"), QStringLiteral("epaisseur"),
                                      QStringLiteral("outils")}));
        QCOMPARE(layout.colors.size(), qsizetype{16});
        QCOMPARE(layout.colors.first(), QStringLiteral("#E53935"));
        QCOMPARE(layout.penWidths, (QList<double>{2, 4, 6, 8}));
        CommandRegistry registry;
        QVERIFY2(registry.loadDeclarations(defaultCommandsJson(), error), qPrintable(error));
        QCOMPARE(unknownCommands(layout, registry), QStringList{});
    }

    void unknownCommandsAreListedOnce()
    {
        Layout layout;
        layout.menus.push_back(MenuSpec{QStringLiteral("m"), QStringLiteral("M"),
                                        {QStringLiteral("a.b"), QStringLiteral("-"), QStringLiteral("a.b")}});
        layout.palette.push_back(PaletteGroup{QStringLiteral("g"), {QStringLiteral("c.d")}});
        CommandRegistry registry;
        QCOMPARE(unknownCommands(layout, registry), (QStringList{QStringLiteral("a.b"), QStringLiteral("c.d")}));
    }
};

QTEST_GUILESS_MAIN(TstActionsLayout)
#include "tst_actions_layout.moc"
```

Ajouter à la fin de `tests/actions.cmake` :

```cmake
actcraft_add_test(tst_actions_layout
    SOURCES tst_actions_layout.cpp
    LIBS ActCraft::actions)
```

- [ ] **Étape 2 : le voir échouer.** Commande ciblée, motif `tst_actions_layout`. Attendu : `error` (`layout.h`
introuvable).

- [ ] **Étape 3 : écrire `actions/include/actcraft/actions/layout.h`.**

```cpp
#pragma once

#include <actcraft/actions/commandregistry.h>

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

#include <vector>

namespace actcraft::actions {

// Un menu de la ligne supérieure : identifiants de commandes dans l'ordre ; "-" = séparateur.
struct MenuSpec
{
    QString id;
    QString label;
    QStringList items;
};

// Un groupe de la palette. « couleurs » et « epaisseur » sont des groupes spéciaux, sans commandes : l'interface
// les dessine à partir de Layout::colors et Layout::penWidths.
struct PaletteGroup
{
    QString group;
    QStringList items;
};

// Disposition de l'interface (règle 3 du BRIEF, DA1), format « actcraft-layout/1 ».
struct Layout
{
    std::vector<MenuSpec> menus;
    std::vector<PaletteGroup> palette;
    QHash<QString, QString> shortcuts; // id → séquence qui remplace celle de la commande ("" la retire)
    QStringList colors;                // pastilles « #RRGGBB » (F-133)
    QList<double> penWidths;           // préréglages du stylo, pixels de page (F-137)
};

// Tout ou rien : `out` n'est modifiée qu'en cas de succès.
bool parseLayout(const QByteArray& json, Layout& out, QString& error);
// Contenu de `fileName` lu dans `profileDir` s'il y existe, sinon dans `resourceDir` (N2-5) ; vide si absent des
// deux. `usedPath` reçoit le chemin lu (vide si rien n'a été lu).
QByteArray readLayeredFile(const QString& fileName, const QString& profileDir, const QString& resourceDir,
                           QString* usedPath = nullptr);
// Identifiants de la disposition (menus, palette, raccourcis) que le registre ne déclare pas, sans doublon, dans
// l'ordre de rencontre.
QStringList unknownCommands(const Layout& layout, const CommandRegistry& registry);

} // namespace actcraft::actions
```

- [ ] **Étape 4 : écrire `actions/src/layout.cpp`.**

```cpp
#include <actcraft/actions/layout.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <utility>

namespace actcraft::actions {

namespace {

// Liste de textes ; une valeur absente donne une liste vide.
bool readStringList(const QJsonValue& value, QStringList& out)
{
    out.clear();
    if (value.isUndefined()) {
        return true;
    }
    if (!value.isArray()) {
        return false;
    }
    const QJsonArray array = value.toArray();
    for (const QJsonValue& item : array) {
        if (!item.isString()) {
            return false;
        }
        out << item.toString();
    }
    return true;
}

} // namespace

bool parseLayout(const QByteArray& json, Layout& out, QString& error)
{
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("JSON invalide : %1").arg(parseError.errorString());
        return false;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() != QLatin1String("actcraft-layout/1")) {
        error = QStringLiteral("format inconnu (attendu actcraft-layout/1)");
        return false;
    }
    Layout layout;

    const QJsonArray menus = root.value(QStringLiteral("menus")).toArray();
    for (qsizetype i = 0; i < menus.size(); ++i) {
        const QJsonObject o = menus.at(i).toObject();
        MenuSpec menu;
        menu.id = o.value(QStringLiteral("id")).toString();
        menu.label = o.value(QStringLiteral("label")).toString();
        if (menu.id.isEmpty() || menu.label.isEmpty()) {
            error = QStringLiteral("menu %1 : id ou label manquant").arg(i + 1);
            return false;
        }
        if (!readStringList(o.value(QStringLiteral("items")), menu.items)) {
            error = QStringLiteral("menu %1 (%2) : items invalides").arg(i + 1).arg(menu.id);
            return false;
        }
        layout.menus.push_back(std::move(menu));
    }

    const QJsonArray palette = root.value(QStringLiteral("palette")).toArray();
    for (qsizetype i = 0; i < palette.size(); ++i) {
        const QJsonObject o = palette.at(i).toObject();
        PaletteGroup group;
        group.group = o.value(QStringLiteral("group")).toString();
        if (group.group.isEmpty() || !readStringList(o.value(QStringLiteral("items")), group.items)) {
            error = QStringLiteral("groupe %1 de la palette : nom ou items invalides").arg(i + 1);
            return false;
        }
        layout.palette.push_back(std::move(group));
    }

    const QJsonObject shortcuts = root.value(QStringLiteral("shortcuts")).toObject();
    for (auto it = shortcuts.constBegin(); it != shortcuts.constEnd(); ++it) {
        if (!it.value().isString()) {
            error = QStringLiteral("raccourci de %1 : texte attendu").arg(it.key());
            return false;
        }
        layout.shortcuts.insert(it.key(), it.value().toString());
    }

    if (!readStringList(root.value(QStringLiteral("colors")), layout.colors)) {
        error = QStringLiteral("couleur invalide : liste de textes attendue");
        return false;
    }
    for (const QString& color : std::as_const(layout.colors)) {
        if (!argumentIsValid(ArgumentKind::Color, color)) {
            error = QStringLiteral("couleur invalide « %1 »").arg(color);
            return false;
        }
    }

    const QJsonArray widths = root.value(QStringLiteral("penWidths")).toArray();
    for (const QJsonValue& width : widths) {
        const double value = width.toDouble(-1);
        if (!width.isDouble() || value <= 0 || value > 100) {
            error = QStringLiteral("épaisseur invalide (attendu un nombre de 1 à 100)");
            return false;
        }
        layout.penWidths << value;
    }

    out = std::move(layout);
    return true;
}

QByteArray readLayeredFile(const QString& fileName, const QString& profileDir, const QString& resourceDir,
                           QString* usedPath)
{
    for (const QString& dir : {profileDir, resourceDir}) {
        if (dir.isEmpty()) {
            continue;
        }
        const QString path = dir + QLatin1Char('/') + fileName;
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            if (usedPath) {
                *usedPath = path;
            }
            return file.readAll();
        }
    }
    if (usedPath) {
        usedPath->clear();
    }
    return {};
}

QStringList unknownCommands(const Layout& layout, const CommandRegistry& registry)
{
    QStringList unknown;
    const auto check = [&](const QString& id) {
        if (id == QLatin1String("-") || registry.find(id) || unknown.contains(id)) {
            return;
        }
        unknown << id;
    };
    for (const MenuSpec& menu : layout.menus) {
        for (const QString& id : menu.items) {
            check(id);
        }
    }
    for (const PaletteGroup& group : layout.palette) {
        for (const QString& id : group.items) {
            check(id);
        }
    }
    for (auto it = layout.shortcuts.cbegin(); it != layout.shortcuts.cend(); ++it) {
        check(it.key());
    }
    return unknown;
}

} // namespace actcraft::actions
```

- [ ] **Étape 5 : écrire `actions/data/layout.json`.**

```json
{
  "format": "actcraft-layout/1",
  "menus": [
    { "id": "file", "label": "Fichier", "items": ["file.quit"] },
    { "id": "edit", "label": "Éditer", "items": [] },
    { "id": "view", "label": "Affichage", "items": ["view.fullscreen", "-", "page.previous", "page.next", "-", "view.fit"] },
    { "id": "insert", "label": "Insérer", "items": [] },
    { "id": "tools", "label": "Outils", "items": ["tool.pen"] },
    { "id": "help", "label": "Aide", "items": ["help.website", "-", "help.about"] }
  ],
  "palette": [
    { "group": "commandes", "items": ["menu.main", "menu.tools"] },
    { "group": "pages", "items": ["page.previous", "page.next"] },
    { "group": "couleurs" },
    { "group": "epaisseur" },
    { "group": "outils", "items": ["tool.pen"] }
  ],
  "shortcuts": {},
  "colors": [
    "#E53935", "#FB8C00", "#FDD835", "#43A047",
    "#00ACC1", "#1E88E5", "#8E24AA", "#D81B60",
    "#6D4C41", "#F5CBA7", "#B39DDB", "#880E4F",
    "#000000", "#424242", "#9E9E9E", "#FFFFFF"
  ],
  "penWidths": [2, 4, 6, 8]
}
```

Dans `actions/CMakeLists.txt` : ajouter `src/layout.cpp` après `src/commandregistry.cpp` dans `SOURCES`, et
`data/layout.json` sous `data/commands.json` dans `FILES` de `qt_add_resources`.

- [ ] **Étape 6 : le voir passer.** Commande ciblée, motif `tst_actions_`. Attendu : `0 tests failed out of 2`.

- [ ] **Étape 7 : format et preuve.** `pwsh -NoProfile -File ./scripts/format.ps1`, forme bornée, ligne
`tst_actions_layout .*Passed`. Attendu : N ≥ 31, `TESTS OK`.

- [ ] **Étape 8 : commit.**

```powershell
Set-Location <worktree>; git add actions/CMakeLists.txt actions/include/actcraft/actions/layout.h actions/src/layout.cpp actions/data/layout.json tests/actions.cmake tests/tst_actions_layout.cpp
git -C <worktree> commit -m "actions : disposition des menus, de la palette et des raccourcis, profil d'abord" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

<a id="tache-l1-t6"></a>
### Tâche L1-T6 — `input` : un doigt devient un échantillon

**Objectif.** Décision 25 : un doigt agit comme l'outil courant. Un événement tactile à **un seul point**
(`TouchBegin`, `TouchUpdate`, `TouchEnd`) devient un échantillon `DeviceKind::Touch` (appui, déplacement,
relâché) de pression nominale 0,5 ; deux doigts ou plus ne donnent jamais d'échantillon (gestes : lot 3). La
souris synthétisée depuis l'écran tactile reste refusée (déjà le cas).

**Fichiers.** Modifier : `input/include/actcraft/input/sample.h`, `input/include/actcraft/input/qtevents.h`
(commentaire), `input/src/qtevents.cpp`, `tests/tst_input.cpp`.

**Test et commande bornée.** Deux fonctions ajoutées à `tst_input` : `singleTouchPointBecomesTouchSamples`,
`twoFingersAndCancelGiveNothing`. Rouge : commande ciblée, motif `tst_input` → `error` (`DeviceKind::Touch`
inconnu). Vert : forme bornée, N ≥ 29, `TESTS OK`, `tst_input .*Passed` ; banc sans écran (résultat).

**Interdits.** Ne pas renuméroter `DeviceKind` : `Touch` s'ajoute après `Other`. Ne pas toucher au filtre ni à la
séquence (`pipeline.*`).

**Terminé quand.** Preuve verte, JSON du banc joint ; commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire le test.** Dans `tests/tst_input.cpp`, ajouter `#include <QTouchEvent>` aux inclusions et,
juste avant `void otherEventsIgnored()`, les deux fonctions :

```cpp
    void singleTouchPointBecomesTouchSamples()
    {
        QPointingDevice screen(QStringLiteral("tst-touch"), 7701, QInputDevice::DeviceType::TouchScreen,
                               QPointingDevice::PointerType::Finger, QInputDevice::Capability::Position, 10,
                               0);
        QTouchEvent begin(QEvent::TouchBegin, &screen, Qt::NoModifier,
                          {QEventPoint(1, QEventPoint::State::Pressed, QPointF(10, 20), QPointF(10, 20))});
        const auto press = input::sampleFromEvent(begin, QPointF(5, 6), 42);
        QVERIFY(press.has_value());
        QVERIFY(press->device == input::DeviceKind::Touch);
        QVERIFY(press->phase == Phase::Press);
        QCOMPARE(press->x, 5.0);
        QCOMPARE(press->y, 6.0);
        QCOMPARE(press->pressure, 0.5f);
        QCOMPARE(press->timestampNs, std::int64_t{42});

        QTouchEvent update(QEvent::TouchUpdate, &screen, Qt::NoModifier,
                           {QEventPoint(1, QEventPoint::State::Updated, QPointF(30, 20), QPointF(30, 20))});
        const auto move = input::sampleFromEvent(update, QPointF(7, 6), 43);
        QVERIFY(move.has_value() && move->phase == Phase::Move);

        QTouchEvent end(QEvent::TouchEnd, &screen, Qt::NoModifier,
                        {QEventPoint(1, QEventPoint::State::Released, QPointF(30, 20), QPointF(30, 20))});
        const auto release = input::sampleFromEvent(end, QPointF(7, 6), 44);
        QVERIFY(release.has_value() && release->phase == Phase::Release);
    }

    void twoFingersAndCancelGiveNothing()
    {
        QPointingDevice screen(QStringLiteral("tst-touch-2"), 7702, QInputDevice::DeviceType::TouchScreen,
                               QPointingDevice::PointerType::Finger, QInputDevice::Capability::Position, 10,
                               0);
        QTouchEvent two(QEvent::TouchUpdate, &screen, Qt::NoModifier,
                        {QEventPoint(1, QEventPoint::State::Updated, QPointF(10, 20), QPointF(10, 20)),
                         QEventPoint(2, QEventPoint::State::Pressed, QPointF(90, 20), QPointF(90, 20))});
        QVERIFY(!input::sampleFromEvent(two, QPointF(5, 6), 1).has_value());
        QTouchEvent cancel(QEvent::TouchCancel, &screen, Qt::NoModifier, {});
        QVERIFY(!input::sampleFromEvent(cancel, QPointF(5, 6), 2).has_value());
    }
```

- [ ] **Étape 2 : le voir échouer.** Commande ciblée, motif `tst_input`. Attendu : `error` (`Touch` n'est pas membre
de `DeviceKind`).

- [ ] **Étape 3 : `sample.h`.** Remplacer l'énumération par :

```cpp
enum class DeviceKind : std::uint8_t
{
    Stylus,
    Mouse,
    Other,
    Touch, // un doigt (décision 25 du cadrage livraison-complete) ; ajouté en dernier : valeurs inchangées
};
```

- [ ] **Étape 4 : `qtevents.cpp`.** Ajouter `#include <QTouchEvent>` après `#include <QTabletEvent>`, et dans le
`switch`, avant `default:` :

```cpp
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd: {
        const auto& touch = static_cast<const QTouchEvent&>(event);
        if (touch.pointCount() != 1) {
            return std::nullopt; // deux doigts et plus : gestes (lot 3), jamais un trait
        }
        sample.device = DeviceKind::Touch;
        sample.phase = event.type() == QEvent::TouchBegin    ? Phase::Press
                       : event.type() == QEvent::TouchUpdate ? Phase::Move
                                                             : Phase::Release;
        sample.pressure = 0.5f; // comme la souris : épaisseur nominale
        return sample;
    }
```

Dans `qtevents.h`, remplacer le commentaire de `sampleFromEvent` par :

```cpp
// Tablette (appui, déplacement en contact, relâché), souris bouton gauche (N2-12) ou un seul doigt
// (décision 25) ; nullopt pour le survol, les autres boutons, la souris synthétisée depuis un stylet ou un écran
// tactile, deux doigts ou plus, l'annulation tactile et tout autre événement.
// `pagePos` : position déjà ramenée en coordonnées de page par la vue.
```

- [ ] **Étape 5 : le voir passer.** Commande ciblée, motif `tst_input`. Attendu : `0 tests failed out of 1`.

- [ ] **Étape 6 : format, preuve, banc.** `pwsh -NoProfile -File ./scripts/format.ps1` ; forme bornée (N ≥ 29,
`TESTS OK`, `tst_input .*Passed`) ; puis la preuve que les fonctions ajoutées ont tourné (la ligne ctest ne le dit
pas) :

```powershell
Set-Location <worktree>; pwsh -NoProfile -Command ". ./scripts/dev-env.ps1; `$env:QT_QPA_PLATFORM='offscreen'; ./build/debug/tst_input.exe *> .tmp/fonctions.txt"; Select-String -Path .tmp/fonctions.txt -Pattern '^(PASS|FAIL!).*(singleTouchPointBecomesTouchSamples|twoFingersAndCancelGiveNothing)|^Totals' | ForEach-Object { $_.Line }
```
Attendu : deux lignes `PASS   : TstInput::…()` et `Totals: N passed, 0 failed`. Enfin le banc sans écran
(`BUDGET TENU` ou `BUDGET DEPASSE`, nom du JSON au rapport).

- [ ] **Étape 7 : commit.**

```powershell
Set-Location <worktree>; git add input/include/actcraft/input/sample.h input/include/actcraft/input/qtevents.h input/src/qtevents.cpp tests/tst_input.cpp
git -C <worktree> commit -m "input : un doigt devient un échantillon, deux doigts sont laissés aux gestes" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

<a id="tache-l1-t7"></a>
### Tâche L1-T7 — `ui` : direction visuelle, thème `Theme` et icônes SVG à nous

**Objectif.** Décision 23 et N1-13 : poser la direction visuelle par le skill `impeccable` (première tâche de
design), la traduire en jetons dans un singleton QML `Theme` (couleurs, espacements, tailles, typographie) et
dessiner un jeu d'icônes SVG **neuf**, sur une grille de 24 px, pour les commandes du lot 1. Disposition de la
référence, dessin à nous ; rien des icônes d'ActivInspire ni de celles du clone Python. Ombres éventuelles en
images (règle de fluidité 4) : aucune dans ce lot. Ajoute aussi `QuickControls2` et `Svg` aux composants Qt du
projet (utilisés par la coquille et par les icônes).

**Fichiers.**
- Créer : `ui/qml/Theme.qml`, `ui/icons/menu.svg`, `ui/icons/outils.svg`, `ui/icons/page-precedente.svg`,
  `ui/icons/page-suivante.svg`, `ui/icons/stylo.svg`, `ui/icons/plein-ecran.svg`, `ui/icons/ajuster.svg`,
  `ui/icons/quitter.svg`, `ui/icons/aide.svg`, `ui/icons/a-propos.svg`, `tests/tst_ui_theme.cpp`,
  `docs/design/2026-10-14-direction-visuelle.md`
- Modifier : `CMakeLists.txt` (racine), `ui/CMakeLists.txt`, `tests/ui.cmake`

**Test et commande bornée.** `tst_ui_theme` : le singleton expose ses jetons ; chacune des 10 icônes existe dans le
module QML, est un SVG valide et a une `viewBox` de 24 × 24. Rouge : commande ciblée, motif `tst_ui_theme` →
`error` puis, une fois le test compilable, `Theme` introuvable. Vert : forme bornée, N ≥ 30, `TESTS OK`,
`tst_ui_theme .*Passed` ; banc sans écran (résultat).

**Interdits.** Aucune icône, image, couleur de marque ou capture de Promethean, ni copiée de l'ancien dépôt
(`app/actcraft/…/icons`, `docs/re/shots/`) ; aucune police embarquée dans ce lot. Les noms des jetons sont figés
(contrat) : la direction peut changer les **valeurs**, pas les noms.

**Terminé quand.** Preuve verte, JSON du banc joint ; le document de direction est commité ; commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire le test.** Créer `tests/tst_ui_theme.cpp` :

```cpp
#include <QColor>
#include <QFile>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSvgRenderer>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

namespace {

const char* const kIcons[] = {"menu",      "outils",      "page-precedente", "page-suivante", "stylo",
                              "plein-ecran", "ajuster", "quitter",         "aide",          "a-propos"};

} // namespace

// Thème et icônes à nous (décision 23).
class TstUiTheme : public QObject
{
    Q_OBJECT

private slots:
    void themeSingletonExposesTokens()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\nQtObject {\n"
                          "  property color fond: Theme.fond\n"
                          "  property color palette: Theme.palette\n"
                          "  property color texte: Theme.texte\n"
                          "  property int bouton: Theme.tailleBouton\n"
                          "  property int largeurPalette: Theme.largeurPalette\n"
                          "  property int hauteurBarre: Theme.hauteurBarre\n"
                          "}\n",
                          QUrl());
        std::unique_ptr<QObject> tokens(component.create());
        QVERIFY2(tokens, qPrintable(component.errorString()));
        QVERIFY(tokens->property("fond").value<QColor>().isValid());
        QVERIFY(tokens->property("palette").value<QColor>().isValid());
        QVERIFY(tokens->property("texte").value<QColor>().isValid());
        QVERIFY(tokens->property("bouton").toInt() >= 44); // cible tactile
        QVERIFY(tokens->property("largeurPalette").toInt() >= 96);
        QVERIFY(tokens->property("hauteurBarre").toInt() >= 32);
    }

    void iconsAreOurSvgOnA24Grid_data()
    {
        QTest::addColumn<QString>("name");
        for (const char* name : kIcons) {
            QTest::newRow(name) << QString::fromLatin1(name);
        }
    }

    void iconsAreOurSvgOnA24Grid()
    {
        QFETCH(QString, name);
        const QString path = QStringLiteral(":/qt/qml/ActCraft/Ui/icons/%1.svg").arg(name);
        QVERIFY2(QFile::exists(path), qPrintable(path));
        QSvgRenderer renderer(path);
        QVERIFY(renderer.isValid());
        QCOMPARE(renderer.viewBoxF(), QRectF(0, 0, 24, 24));
    }
};

QTEST_MAIN(TstUiTheme)
#include "tst_ui_theme.moc"
```

Ajouter à la fin de `tests/ui.cmake` :

```cmake
actcraft_add_test(tst_ui_theme
    SOURCES tst_ui_theme.cpp
    LIBS Qt6::Quick Qt6::Svg ActCraft::ui actcraft_uiplugin)
```

Dans le `CMakeLists.txt` racine, remplacer la ligne `find_package(Qt6 6.12 REQUIRED COMPONENTS Core Gui Qml Quick)`
par :

```cmake
find_package(Qt6 6.12 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Svg)
```

- [ ] **Étape 2 : le voir échouer.** Commande ciblée, motif `tst_ui_theme`. Attendu : le test compile, puis
`themeSingletonExposesTokens` échoue (`Theme is not a type` ou `Theme is not defined`) et chaque ligne de
`iconsAreOurSvgOnA24Grid` échoue (fichier absent) ; `tests failed`.

- [ ] **Étape 3 : direction visuelle.** Invoquer le skill `impeccable` (outil Skill, nom `impeccable`) avec cette
consigne : « Direction visuelle d'ActCraft, tableau blanc interactif libre pour la classe (écran 1366 × 768 à
1920 × 1080 vu de loin, tableaux tactiles, tablettes) : barre supérieure et palette sombres et sobres, page
blanche au centre sur un fond bleu calme, contrastes AA, cibles tactiles ≥ 44 px, aucune ombre calculée ni
flou ; propose les valeurs des jetons nommés ci-dessous et la règle de dessin des icônes (grille 24, trait 2,
extrémités rondes, une seule couleur). Rien qui rappelle Promethean ActivInspire. » Reporter les valeurs retenues
dans `Theme.qml` (étape 4 ; noms inchangés) et la justification dans
`docs/design/2026-10-14-direction-visuelle.md` (étape 6). Sans proposition exploitable du skill, garder les
valeurs de l'étape 4 et l'écrire dans le document.

- [ ] **Étape 4 : écrire `ui/qml/Theme.qml`.**

```qml
pragma Singleton
import QtQuick

// Jetons de l'interface (décision 23 du cadrage livraison-complete) : couleurs, espacements, tailles,
// typographie. Valeurs posées par la direction visuelle (docs/design/2026-10-14-direction-visuelle.md) ; les
// noms sont figés : les composants ne lisent jamais une couleur ou une taille ailleurs qu'ici.
QtObject {
    // Couleurs
    readonly property color fond: "#4A6A8A"           // autour de la page (F-020 : fond bleu par défaut)
    readonly property color barre: "#20242B"          // ligne supérieure
    readonly property color palette: "#2B2F36"        // palette d'outils (repère de l'essai Android, S3)
    readonly property color surface: "#363B44"        // bouton au repos
    readonly property color surfaceSurvol: "#424955"  // bouton survolé
    readonly property color surfaceActive: "#4E7FD1"  // bouton coché, outil actif, préréglage choisi
    readonly property color texte: "#E8EAED"
    readonly property color texteDiscret: "#A9B0BA"
    readonly property color separateur: "#3F444D"
    readonly property color cadreSelection: "#FFFFFF" // pastille de la couleur courante
    readonly property color page: "#FFFFFF"

    // Espacements et tailles, pixels logiques
    readonly property int espace1: 4
    readonly property int espace2: 8
    readonly property int espace3: 12
    readonly property int espace4: 16
    readonly property int rayon: 6
    readonly property int hauteurBarre: 40
    readonly property int largeurPalette: 112
    readonly property int tailleBouton: 44      // cible tactile
    readonly property int tailleIcone: 24
    readonly property int taillePastille: 20

    // Typographie : police sans empattement du système (N2-3)
    readonly property int taillePolice: 14
    readonly property int taillePoliceTitre: 16
}
```

La couleur `palette` sert de repère à `scripts/essai-android.ps1` (L1-T18) : si la direction la change, noter la
nouvelle valeur au rapport (L1-T18 la lira dans ce fichier).

- [ ] **Étape 5 : icônes.** Créer les 10 fichiers (une seule couleur, `#E8EAED`, trait 2, extrémités rondes) :

`ui/icons/menu.svg`
```xml
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#E8EAED" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 6h16M4 12h16M4 18h16"/></svg>
```
`ui/icons/outils.svg`
```xml
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#E8EAED" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="4" y="4" width="6" height="6" rx="1"/><rect x="14" y="4" width="6" height="6" rx="1"/><rect x="4" y="14" width="6" height="6" rx="1"/><rect x="14" y="14" width="6" height="6" rx="1"/></svg>
```
`ui/icons/page-precedente.svg`
```xml
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#E8EAED" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M15 5l-7 7 7 7"/></svg>
```
`ui/icons/page-suivante.svg`
```xml
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#E8EAED" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M9 5l7 7-7 7"/></svg>
```
`ui/icons/stylo.svg`
```xml
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#E8EAED" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 20l1-4L16 5l3 3L8 19l-4 1z"/><path d="M14 7l3 3"/></svg>
```
`ui/icons/plein-ecran.svg`
```xml
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#E8EAED" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M4 9V4h5M20 9V4h-5M4 15v5h5M20 15v5h-5"/></svg>
```
`ui/icons/ajuster.svg`
```xml
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#E8EAED" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="6" y="7" width="12" height="10" rx="1"/><path d="M2 12h2M20 12h2M12 2v3M12 19v3"/></svg>
```
`ui/icons/quitter.svg`
```xml
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#E8EAED" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M10 4H5v16h5"/><path d="M14 8l4 4-4 4M18 12H9"/></svg>
```
`ui/icons/aide.svg`
```xml
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#E8EAED" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="9"/><path d="M9.5 9.5a2.5 2.5 0 1 1 3.5 2.3c-.6.3-1 .8-1 1.5V14"/><path d="M12 17.5v.01"/></svg>
```
`ui/icons/a-propos.svg`
```xml
<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="#E8EAED" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><circle cx="12" cy="12" r="9"/><path d="M12 11v6M12 7.5v.01"/></svg>
```
Si la direction de l'étape 3 change le dessin, garder la `viewBox` 24 × 24, le nom de fichier et l'unique couleur.

- [ ] **Étape 6 : `docs/design/2026-10-14-direction-visuelle.md`.**

```markdown
# Direction visuelle d'ActCraft — 2026-10-14

Tâche L1-T7 du chantier `livraison-complete` (décision 23, hypothèse N1-13 du cadrage
`docs/specs/2026-10-14-livraison-complete-cadrage.md`). Posée avec le skill `impeccable`. Disposition de la
référence (menus en haut, palette à droite, page au centre, pas de barre d'état) ; dessin entièrement à nous.

## Intention

Un tableau blanc de classe, vu de loin et touché du doigt : la page blanche domine, l'interface s'efface autour
d'elle. Barre et palette sombres et sobres, fond bleu calme autour de la page, une seule couleur d'accent pour
l'état actif. Aucune ombre calculée, aucun flou, aucune transparence empilée (règle de fluidité 4).

## Jetons (ui/qml/Theme.qml)

| Jeton | Valeur | Rôle |
|---|---|---|
| `fond` | `#4A6A8A` | autour de la page |
| `barre` | `#20242B` | ligne supérieure |
| `palette` | `#2B2F36` | palette d'outils |
| `surface` / `surfaceSurvol` / `surfaceActive` | `#363B44` / `#424955` / `#4E7FD1` | boutons |
| `texte` / `texteDiscret` | `#E8EAED` / `#A9B0BA` | libellés |
| `separateur` | `#3F444D` | séparation des groupes |
| `tailleBouton` | 44 | cible tactile minimale |
| `largeurPalette`, `hauteurBarre` | 112, 40 | gabarit |

## Icônes (ui/icons/)

Grille de 24 px, trait de 2, extrémités et jonctions rondes, une seule couleur (`texte`), formes géométriques
simples. Dessinées pour ActCraft ; aucune ne reprend celles d'ActivInspire ni celles du clone Python, qui les
imitaient.

## À signaler à Jérôme (porte de fin du lot 1)

Les enseignants habitués aux icônes d'origine devront apprendre celles-ci (N1-13). Captures : scénario
`premiers-pas` (`scripts/captures.ps1`).
```
Mettre dans le tableau les valeurs réellement retenues à l'étape 3.

- [ ] **Étape 7 : `ui/CMakeLists.txt`.** Avant `qt_add_qml_module`, ajouter :

```cmake
# Thème : singleton QML (décision 23).
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
```
Dans `qt_add_qml_module(actcraft_ui …)`, ajouter `qml/Theme.qml` sous `qml/Main.qml` dans `QML_FILES`, et, entre
`QML_FILES` et `SOURCES`, le bloc :

```cmake
    RESOURCES
        icons/menu.svg
        icons/outils.svg
        icons/page-precedente.svg
        icons/page-suivante.svg
        icons/stylo.svg
        icons/plein-ecran.svg
        icons/ajuster.svg
        icons/quitter.svg
        icons/aide.svg
        icons/a-propos.svg
```
Remplacer `target_link_libraries(actcraft_ui PUBLIC …)` par :

```cmake
target_link_libraries(actcraft_ui PUBLIC
    Qt6::Quick Qt6::QuickControls2 Qt6::Svg ActCraft::actions ActCraft::input ActCraft::render ActCraft::tools)
```

- [ ] **Étape 8 : le voir passer.** Commande ciblée, motif `tst_ui_theme`. Attendu : `0 tests failed out of 1`.

- [ ] **Étape 9 : format, preuve, banc.** `pwsh -NoProfile -File ./scripts/format.ps1` ; forme bornée (N ≥ 30,
`TESTS OK`, `tst_ui_theme .*Passed`) ; banc sans écran (résultat, nom du JSON au rapport).

- [ ] **Étape 10 : commit.**

```powershell
Set-Location <worktree>; git add CMakeLists.txt ui/CMakeLists.txt ui/qml/Theme.qml ui/icons/menu.svg ui/icons/outils.svg ui/icons/page-precedente.svg ui/icons/page-suivante.svg ui/icons/stylo.svg ui/icons/plein-ecran.svg ui/icons/ajuster.svg ui/icons/quitter.svg ui/icons/aide.svg ui/icons/a-propos.svg tests/ui.cmake tests/tst_ui_theme.cpp docs/design/2026-10-14-direction-visuelle.md
git -C <worktree> commit -m "ui : direction visuelle, thème et icônes à nous" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

<a id="tache-l1-t8"></a>
### Tâche L1-T8 — `ui` : `PageView` — stylo réglable, défilement, entrée réservée à la vue, doigt unique

**Objectif.** Préparer la vue à la coquille sans toucher à sa politique de cache :
1. **Stylo réglable** (F-133, F-134, F-137, F-220) : propriétés `penColor` (noir par défaut) et `penWidth` (4 par
   défaut, bornée à 1–100), appliquées au `PenTool` et gardées quand il est reconstruit.
2. **Défilement** (F-022) : propriétés `scrollSizeX/Y`, `scrollPositionX/Y` et `scrollTo(x, y)`, calculées par deux
   fonctions pures de `viewtransform` (`scrollAxis`, `panForScrollPosition`).
3. **Entrée réservée à la vue** : aujourd'hui le filtre de la fenêtre prend **tout** appui de stylet, même sur
   une barre ou une palette. Désormais un trait ne commence que si l'appui tombe sur la vue et qu'aucune fenêtre
   surgissante n'est ouverte (`inputBlocked`) ; un trait commencé suit le stylet partout jusqu'au relâché.
4. **Un doigt** (décision 25) : `touchEvent` transmet un toucher à un point à la chaîne d'entrée ; un deuxième
   doigt ou l'annulation termine proprement le trait du premier (gestes : lot 3).

Le patron évident — décider « sur la vue » au moment de chaque déplacement — est le mauvais : il couperait un
trait qui sort de la vue et laisserait passer un trait commencé sur la palette. La règle est : la décision se prend
**à l'appui**, et seuls les événements d'un trait en cours sont pris ensuite.

**Fichiers.** Modifier : `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`,
`ui/include/actcraft/ui/viewtransform.h`, `ui/src/viewtransform.cpp`, `tests/tst_pageview.cpp`,
`tests/tst_viewtransform.cpp`.

**Test et commande bornée.** `tst_viewtransform` (+2 fonctions), `tst_pageview` (+5 fonctions). Rouge : commande
ciblée, motif `tst_viewtransform|tst_pageview` → `error`. Vert : forme bornée, N ≥ 29, `TESTS OK`,
`tst_pageview .*Passed` et `tst_viewtransform .*Passed` ; banc sans écran (résultat).

**Interdits.** Ne pas changer la politique du cache (`requestCacheRefresh`, `notifyCacheUpdated`, minuteur), ni
`setExclusiveDevices` (banc), ni les noms figés du prototype. Ne pas toucher à `ui/CMakeLists.txt` ni à
`tests/ui.cmake` (les tests existent déjà).

**Terminé quand.** Preuve verte, JSON du banc joint ; commit fait.

**Étapes.**

- [ ] **Étape 1 : tests de `viewtransform`.** Dans `tests/tst_viewtransform.cpp`, avant `QTEST_GUILESS_MAIN`, dans
la section `private slots:`, ajouter :

```cpp
    void scrollAxisCoversPageAndMargins()
    {
        const ScrollAxis fits = scrollAxis(10, 1000, 500);
        QCOMPARE(fits.size, 1.0);
        QCOMPARE(fits.position, 0.0);
        const double extent = 2000 + 2 * kViewMargin;
        const ScrollAxis start = scrollAxis(kViewMargin, 1000, 2000);
        QCOMPARE(start.size, 1000 / extent);
        QCOMPARE(start.position, 0.0);
        const ScrollAxis end = scrollAxis(1000 - 2000 - kViewMargin, 1000, 2000);
        QVERIFY(qAbs(end.position - (1.0 - 1000 / extent)) < 1e-12);
    }

    void panForScrollPositionInvertsScrollAxis()
    {
        for (const double position : {0.0, 0.2, 0.4}) {
            const double pan = panForScrollPosition(position, 1000, 2000);
            QVERIFY(qAbs(scrollAxis(pan, 1000, 2000).position - position) < 1e-12);
        }
        QCOMPARE(panForScrollPosition(0.5, 1000, 500), 250.0); // page plus petite que la vue : centrée
        QCOMPARE(panForScrollPosition(-1.0, 1000, 2000), kViewMargin); // borné au début
    }
```

- [ ] **Étape 2 : tests de `PageView`.** Dans `tests/tst_pageview.cpp` : ajouter aux inclusions

```cpp
#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>

#include <QTabletEvent>
#include <QTouchEvent>

#include <memory>
```
ajouter dans l'espace de noms anonyme, après `wheel(...)` :

```cpp
// Vue de 600 × 600 posée en x = 200 dans une fenêtre de 1000 × 600 : à gauche et à droite, « hors de la vue »
// (barre, palette). Page vierge, sans cache.
struct DrawFixture
{
    QQuickWindow window;
    PageView* view = nullptr;
    actcraft::render::FixedPageItem* fixed = nullptr;
    QPointingDevice* stylus = nullptr;
};

bool openDrawing(DrawFixture& f, QPointingDevice* stylus)
{
    f.stylus = stylus;
    f.window.resize(1000, 600);
    f.view = new PageView;
    f.view->setParentItem(f.window.contentItem());
    f.view->setPosition(QPointF(200, 0));
    f.view->setSize(QSizeF(600, 600));
    auto* content = new QQuickItem;
    content->setSize(QSizeF(1024, 768));
    content->setParentItem(f.view);
    f.fixed = new actcraft::render::FixedPageItem;
    f.fixed->setSize(content->size());
    f.fixed->setParentItem(content);
    auto* ink = new actcraft::render::InkItem;
    ink->setSize(content->size());
    ink->setParentItem(content);
    f.view->setCacheEnabled(false);
    f.view->setContent(content);
    f.view->setFixedPage(f.fixed);
    f.view->setInk(ink);
    f.fixed->setPage(actcraft::core::Page{});
    f.window.show();
    return QTest::qWaitForWindowExposed(&f.window) && QTest::qWaitFor([&] { return f.fixed->isReady(); }, 5000);
}

void tablet(DrawFixture& f, QEvent::Type type, QPointF scene, Qt::MouseButtons buttons)
{
    QTabletEvent event(type, f.stylus, scene, f.window.mapToGlobal(scene), buttons ? 0.5 : 0.0, 0, 0, 0, 0, 0,
                       Qt::NoModifier, Qt::LeftButton, buttons);
    QCoreApplication::sendEvent(&f.window, &event);
}

void tabletStroke(DrawFixture& f, QPointF from, QPointF to)
{
    tablet(f, QEvent::TabletPress, from, Qt::LeftButton);
    for (int i = 1; i <= 4; ++i) {
        tablet(f, QEvent::TabletMove, from + (to - from) * i / 4.0, Qt::LeftButton);
    }
    tablet(f, QEvent::TabletRelease, to, Qt::NoButton);
}
```
dans la classe `TstPageView`, juste après `private slots:` ajouter le membre et les fonctions (le membre se
déclare dans une section `private:` placée avant `private slots:`) :

```cpp
private:
    std::unique_ptr<QPointingDevice> m_stylus;

private slots:
    void initTestCase()
    {
        m_stylus = std::make_unique<QPointingDevice>(
            QStringLiteral("tst-pageview-stylus"), 5160, QInputDevice::DeviceType::Stylus,
            QPointingDevice::PointerType::Pen,
            QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
    }

    void penColorAndWidthAreClampedAndSignalled()
    {
        Fixture f;
        QVERIFY(open(f));
        QCOMPARE(f.view->penColor(), QColor(Qt::black));
        QCOMPARE(f.view->penWidth(), kDefaultPenWidth);
        QSignalSpy changed(f.view, &PageView::penChanged);
        f.view->setPenColor(QColor(QStringLiteral("#E53935")));
        f.view->setPenWidth(0.0);
        QCOMPARE(f.view->penWidth(), kMinPenWidth);
        f.view->setPenWidth(500.0);
        QCOMPARE(f.view->penWidth(), kMaxPenWidth);
        f.view->setPenColor(QColor()); // invalide : ignorée
        QCOMPARE(f.view->penColor(), QColor(QStringLiteral("#E53935")));
        QCOMPARE(changed.count(), 3);
    }

    void scrollPropertiesFollowZoom()
    {
        Fixture f;
        QVERIFY(open(f));
        QCOMPARE(f.view->scrollSizeX(), 1.0);
        QCOMPARE(f.view->scrollSizeY(), 1.0);
        QSignalSpy scrolled(f.view, &PageView::scrollChanged);
        f.view->setZoom(4.0);
        QVERIFY(scrolled.count() >= 1);
        QVERIFY(f.view->scrollSizeX() < 1.0);
        QVERIFY(f.view->scrollSizeY() < 1.0);
        f.view->scrollTo(0.0, 0.0);
        QCOMPARE(f.view->pan(), QPointF(kViewMargin, kViewMargin));
        QCOMPARE(f.view->scrollPositionX(), 0.0);
        f.view->scrollTo(1.0, 1.0); // borné à 1 − taille
        QVERIFY(qAbs(f.view->scrollPositionX() + f.view->scrollSizeX() - 1.0) < 1e-9);
    }

    void strokeStartsOnlyOnTheViewThenFollowsTheStylus()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        tabletStroke(f, QPointF(100, 300), QPointF(500, 300)); // appui hors de la vue
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 0);
        tabletStroke(f, QPointF(300, 300), QPointF(900, 300)); // appui sur la vue, relâché hors de la vue
        QTRY_COMPARE(f.fixed->objectCount(), 1);
    }

    void blockedInputStartsNoStroke()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        QSignalSpy blocked(f.view, &PageView::inputBlockedChanged);
        f.view->setInputBlocked(true);
        QCOMPARE(blocked.count(), 1);
        tabletStroke(f, QPointF(300, 300), QPointF(500, 300));
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 0);
        f.view->setInputBlocked(false);
        tabletStroke(f, QPointF(300, 300), QPointF(500, 300));
        QTRY_COMPARE(f.fixed->objectCount(), 1);
    }

    void oneFingerDrawsTwoFingersDoNot()
    {
        DrawFixture f;
        QVERIFY(openDrawing(f, m_stylus.get()));
        QPointingDevice* screen = QTest::createTouchDevice();
        QTest::touchEvent(&f.window, screen).press(0, QPoint(300, 200));
        QTest::touchEvent(&f.window, screen).move(0, QPoint(400, 200));
        QTest::touchEvent(&f.window, screen).move(0, QPoint(500, 200));
        QTest::touchEvent(&f.window, screen).release(0, QPoint(500, 200));
        QTRY_COMPARE(f.fixed->objectCount(), 1);
        QTest::touchEvent(&f.window, screen).press(0, QPoint(300, 400)).press(1, QPoint(600, 400));
        QTest::touchEvent(&f.window, screen).move(0, QPoint(350, 400)).move(1, QPoint(650, 400));
        QTest::touchEvent(&f.window, screen).release(0, QPoint(350, 400)).release(1, QPoint(650, 400));
        QTest::qWait(50);
        QCOMPARE(f.fixed->objectCount(), 1);
    }
```
Si `TstPageView` a déjà une section `private:`, y placer `m_stylus` ; s'il a déjà un `initTestCase`, y ajouter la
création de `m_stylus`.

- [ ] **Étape 3 : les voir échouer.** Commande ciblée, motif `tst_viewtransform|tst_pageview`. Attendu : `error`
(`ScrollAxis`, `penColor`, `kDefaultPenWidth`… inconnus).

- [ ] **Étape 4 : `viewtransform`.** Dans `ui/include/actcraft/ui/viewtransform.h`, avant la fermeture de l'espace
de noms, ajouter :

```cpp
// Barre de défilement d'un axe (F-022) : taille et position en fraction de l'étendue (page zoomée + 2 marges),
// comme ScrollBar.size et ScrollBar.position de Qt Quick Controls. Page plus petite que la vue : {1, 0}.
struct ScrollAxis
{
    double size = 1.0;
    double position = 0.0;
};
ScrollAxis scrollAxis(double pan, double viewLength, double contentLength, double margin = kViewMargin);
// Inverse de scrollAxis : défilement pour une position (bornée à [0, 1 − taille]) ; page plus petite que la vue :
// centrée.
double panForScrollPosition(double position, double viewLength, double contentLength, double margin = kViewMargin);
```
Dans `ui/src/viewtransform.cpp`, à la fin de l'espace de noms :

```cpp
ScrollAxis scrollAxis(double pan, double viewLength, double contentLength, double margin)
{
    const double extent = contentLength + 2 * margin;
    if (extent <= viewLength || extent <= 0) {
        return {};
    }
    ScrollAxis axis;
    axis.size = viewLength / extent;
    axis.position = std::clamp((margin - pan) / extent, 0.0, 1.0 - axis.size);
    return axis;
}

double panForScrollPosition(double position, double viewLength, double contentLength, double margin)
{
    const double extent = contentLength + 2 * margin;
    if (extent <= viewLength) {
        return (viewLength - contentLength) / 2;
    }
    const double p = std::clamp(position, 0.0, 1.0 - viewLength / extent);
    return margin - p * extent;
}
```

- [ ] **Étape 5 : `pageview.h`.** Ajouter `#include <QColor>` aux inclusions ; avant `class PageView`, après les
déclarations anticipées :

```cpp
// Épaisseur du stylo, pixels de page (F-137 : réglette 1 à 100 ; F-220 : 4 par défaut).
inline constexpr qreal kDefaultPenWidth = 4.0;
inline constexpr qreal kMinPenWidth = 1.0;
inline constexpr qreal kMaxPenWidth = 100.0;
```
dans la classe, après `Q_PROPERTY(actcraft::render::InkItem* ink …)` :

```cpp
    Q_PROPERTY(QColor penColor READ penColor WRITE setPenColor NOTIFY penChanged)
    Q_PROPERTY(qreal penWidth READ penWidth WRITE setPenWidth NOTIFY penChanged)
    Q_PROPERTY(bool inputBlocked READ inputBlocked WRITE setInputBlocked NOTIFY inputBlockedChanged)
    Q_PROPERTY(qreal scrollSizeX READ scrollSizeX NOTIFY scrollChanged)
    Q_PROPERTY(qreal scrollPositionX READ scrollPositionX NOTIFY scrollChanged)
    Q_PROPERTY(qreal scrollSizeY READ scrollSizeY NOTIFY scrollChanged)
    Q_PROPERTY(qreal scrollPositionY READ scrollPositionY NOTIFY scrollChanged)
```
dans `public:`, après `const ViewTransform& viewTransform() const noexcept;` :

```cpp
    QColor penColor() const noexcept;
    // Couleur invalide ignorée.
    void setPenColor(const QColor& color);
    qreal penWidth() const noexcept;
    // Bornée à [kMinPenWidth, kMaxPenWidth].
    void setPenWidth(qreal width);
    bool inputBlocked() const noexcept;
    // Vrai tant qu'une fenêtre surgissante est ouverte (menu, dialogue) : aucun trait ne commence ; un trait
    // commencé se termine normalement.
    void setInputBlocked(bool blocked);
    qreal scrollSizeX() const;
    qreal scrollPositionX() const;
    qreal scrollSizeY() const;
    qreal scrollPositionY() const;
    // Barres de défilement (F-022) : positions en fraction de l'étendue ; quitte « Ajuster ».
    Q_INVOKABLE void scrollTo(qreal positionX, qreal positionY);
```
dans `signals:`, ajouter `void penChanged();`, `void inputBlockedChanged();`, `void scrollChanged();` ; dans
`protected:`, ajouter `void touchEvent(QTouchEvent* event) override;` ; dans `private:` (fonctions), ajouter

```cpp
    // Un trait peut commencer ici : vue visible, aucune fenêtre surgissante, point sur la vue.
    bool canStartStroke(QPointF viewPos) const;
    // Termine le trait d'un doigt (deuxième doigt, annulation) par un relâché au dernier point connu.
    void finishTouchStroke();
```
et dans les membres, après `QList<const QInputDevice*> m_exclusive;` :

```cpp
    QColor m_penColor = Qt::black;
    qreal m_penWidth = kDefaultPenWidth;
    bool m_inputBlocked = false;
    bool m_touchDrawing = false; // un trait d'un doigt est en cours
    QPointF m_lastTouchViewPos;
```

- [ ] **Étape 6 : `pageview.cpp`.** Ajouter `#include <QTouchEvent>` et `#include <algorithm>` aux inclusions.
Dans le constructeur, après `setAcceptedMouseButtons(Qt::LeftButton);`, ajouter `setAcceptTouchEvents(true);`.
Dans `rebuildPen()`, juste après `m_pen = std::make_unique<tools::PenTool>(*m_inkSurface, *m_documentSink);`,
ajouter :

```cpp
    m_pen->setColor(m_penColor.rgba());
    m_pen->setBaseWidth(m_penWidth);
```
Remplacer `mousePressEvent` et `eventFilter` par :

```cpp
void PageView::mousePressEvent(QMouseEvent* event)
{
    if (!canStartStroke(event->position())) {
        event->ignore();
        return;
    }
    event->setAccepted(handlePointer(*event, event->position()));
}

bool PageView::eventFilter(QObject* watched, QEvent* event)
{
    const QEvent::Type type = event->type();
    if (type == QEvent::TabletPress || type == QEvent::TabletMove || type == QEvent::TabletRelease) {
        auto* tablet = static_cast<QTabletEvent*>(event);
        const QPointF viewPos = mapFromScene(tablet->position());
        // La décision se prend à l'appui : un trait ne commence que sur la vue ; commencé, il suit le stylet
        // partout jusqu'au relâché. Le reste (palette, barre, survol) n'est pas pris.
        const bool drawing = m_pen && m_pen->isDrawing();
        const bool mine = drawing || (type == QEvent::TabletPress && canStartStroke(viewPos));
        if (mine && handlePointer(*tablet, viewPos)) {
            tablet->accept();
            return true; // consommé : pas de souris synthétisée en double
        }
    }
    return QQuickItem::eventFilter(watched, event);
}
```
Ajouter, après `eventFilter` :

```cpp
bool PageView::canStartStroke(QPointF viewPos) const
{
    return !m_inputBlocked && isVisible() && contains(viewPos);
}

void PageView::touchEvent(QTouchEvent* event)
{
    if (event->type() == QEvent::TouchCancel) {
        finishTouchStroke();
        event->accept();
        return;
    }
    if (event->pointCount() != 1) {
        finishTouchStroke(); // un deuxième doigt arrête le trait du premier (gestes : lot 3)
        event->ignore();
        return;
    }
    const QPointF viewPos = event->points().constFirst().position();
    const bool begin = event->type() == QEvent::TouchBegin;
    if ((begin && !canStartStroke(viewPos)) || (!begin && !m_touchDrawing)) {
        event->ignore();
        return;
    }
    m_lastTouchViewPos = viewPos;
    const bool handled = handlePointer(*event, viewPos);
    if (begin) {
        m_touchDrawing = handled;
    } else if (event->type() == QEvent::TouchEnd) {
        m_touchDrawing = false;
    }
    event->setAccepted(handled);
}

void PageView::finishTouchStroke()
{
    if (!m_touchDrawing) {
        return;
    }
    m_touchDrawing = false;
    if (!m_pipeline) {
        return;
    }
    const QPointF page = viewTransform().viewToPage(m_lastTouchViewPos);
    input::Sample release;
    release.x = page.x();
    release.y = page.y();
    release.timestampNs = core::monotonicNowNs();
    release.device = input::DeviceKind::Touch;
    release.phase = input::Phase::Release;
    m_pipeline->push(release);
}

QColor PageView::penColor() const noexcept
{
    return m_penColor;
}

void PageView::setPenColor(const QColor& color)
{
    if (!color.isValid() || color == m_penColor) {
        return;
    }
    m_penColor = color;
    if (m_pen) {
        m_pen->setColor(m_penColor.rgba());
    }
    emit penChanged();
}

qreal PageView::penWidth() const noexcept
{
    return m_penWidth;
}

void PageView::setPenWidth(qreal width)
{
    const qreal clamped = std::clamp(width, kMinPenWidth, kMaxPenWidth);
    if (qFuzzyCompare(clamped, m_penWidth)) {
        return;
    }
    m_penWidth = clamped;
    if (m_pen) {
        m_pen->setBaseWidth(m_penWidth);
    }
    emit penChanged();
}

bool PageView::inputBlocked() const noexcept
{
    return m_inputBlocked;
}

void PageView::setInputBlocked(bool blocked)
{
    if (blocked == m_inputBlocked) {
        return;
    }
    m_inputBlocked = blocked;
    emit inputBlockedChanged();
}

qreal PageView::scrollSizeX() const
{
    return scrollAxis(m_transform.pan.x(), width(), pageSize().width() * m_transform.zoom).size;
}

qreal PageView::scrollPositionX() const
{
    return scrollAxis(m_transform.pan.x(), width(), pageSize().width() * m_transform.zoom).position;
}

qreal PageView::scrollSizeY() const
{
    return scrollAxis(m_transform.pan.y(), height(), pageSize().height() * m_transform.zoom).size;
}

qreal PageView::scrollPositionY() const
{
    return scrollAxis(m_transform.pan.y(), height(), pageSize().height() * m_transform.zoom).position;
}

void PageView::scrollTo(qreal positionX, qreal positionY)
{
    m_fitMode = false;
    ViewTransform t = m_transform;
    const QSizeF content = pageSize() * t.zoom;
    t.pan = QPointF(panForScrollPosition(positionX, width(), content.width()),
                    panForScrollPosition(positionY, height(), content.height()));
    apply(clampPan(t, size(), pageSize()));
}
```
Dans `apply(...)`, juste avant l'accolade finale, ajouter :

```cpp
    if (zoomChangedNow || panChangedNow) {
        emit scrollChanged();
    }
```
Dans `geometryChange(...)`, à la fin, ajouter `emit scrollChanged();` (la taille de la vue change la taille des
barres).

- [ ] **Étape 7 : les voir passer.** Commande ciblée, motif `tst_viewtransform|tst_pageview|tst_ui`. Attendu :
`0 tests failed` (les tests `tst_ui*` du prototype restent verts : le stylo par défaut garde le noir ; seule
l'épaisseur par défaut passe de 3 à 4, ce qui ne change pas leurs pixels sondés).

- [ ] **Étape 8 : format, preuve, banc.** `pwsh -NoProfile -File ./scripts/format.ps1` ; forme bornée (N ≥ 29,
`TESTS OK`, lignes `tst_pageview .*Passed` et `tst_viewtransform .*Passed`) ; preuve que les 7 fonctions ajoutées
ont tourné :

```powershell
Set-Location <worktree>; pwsh -NoProfile -Command ". ./scripts/dev-env.ps1; `$env:QT_QPA_PLATFORM='offscreen'; `$env:QT_QUICK_BACKEND='software'; ./build/debug/tst_viewtransform.exe *> .tmp/fonctions-vt.txt; ./build/debug/tst_pageview.exe *> .tmp/fonctions-pv.txt"; Select-String -Path .tmp/fonctions-vt.txt, .tmp/fonctions-pv.txt -Pattern '^(PASS|FAIL!).*(scrollAxisCoversPageAndMargins|panForScrollPositionInvertsScrollAxis|penColorAndWidthAreClampedAndSignalled|scrollPropertiesFollowZoom|strokeStartsOnlyOnTheViewThenFollowsTheStylus|blockedInputStartsNoStroke|oneFingerDrawsTwoFingersDoNot)|^Totals' | ForEach-Object { $_.Line }
```
Attendu : 7 lignes `PASS`, deux lignes `Totals: … 0 failed` ; banc sans écran (résultat ; noter au
rapport que l'épaisseur par défaut du stylo passe de 3 à 4 px, F-220, ce qui touche aussi les traits du banc).

- [ ] **Étape 9 : commit.**

```powershell
Set-Location <worktree>; git add ui/include/actcraft/ui/pageview.h ui/src/pageview.cpp ui/include/actcraft/ui/viewtransform.h ui/src/viewtransform.cpp tests/tst_pageview.cpp tests/tst_viewtransform.cpp
git -C <worktree> commit -m "ui : stylo réglable, défilement, trait réservé à la vue, un doigt dessine" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

<a id="tache-l1-t9"></a>
### Tâche L1-T9 — `ui` : `DocumentController` (pages, page affichée, navigation)

**Objectif.** Relier le squelette `core::Document` (L1-T3) à l'élément « page fixe » de `render/` : le contrôleur
tient le document ouvert et l'indice de la page affichée, montre « n / N » (F-002), et passe à la page suivante
(qui ajoute une page vierge sur la dernière, F-125) ou précédente. Pour le banc et les tests, la page 1 peut
devenir la page générée de 300 objets.

**Invariant (concepteur-critique).** Une seule source de vérité à la fois. Le patron évident — garder les traits
dans deux copies (l'élément « page fixe » **et** le document) et les tenir à jour toutes les deux à chaque trait —
est le mauvais : deux chemins d'écriture divergent dès qu'un troisième écrivain apparaît. Ici, la page **affichée**
vit dans l'élément « page fixe » (le stylo y verse ses traits par `FixedPageSink`, sans changement) ; le document
garde les **autres** pages ; la page affichée est recopiée dans le document **avant** tout changement de page et
**à toute lecture** (`document()`). Rien d'autre que ce contrôleur n'appelle `FixedPageItem::setPage` ou
`loadGeneratedPage` dans l'interface (L1-T12 retire l'appel de `Main.qml`). Le lot 2 remplace ce relais par le vrai
modèle et sa pile d'annulation.

**Fichiers.**
- Créer : `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`, `tests/tst_ui_document.cpp`
- Modifier : `ui/CMakeLists.txt`, `tests/ui.cmake`

**Test et commande bornée.** `tst_ui_document` (7 fonctions). Rouge : commande ciblée, motif `tst_ui_document` →
`error`. Vert : forme bornée, N ≥ 32, `TESTS OK`, `tst_ui_document .*Passed` ; banc sans écran (résultat).

**Interdits.** Ne pas modifier `FixedPageItem` ni `penadapters.*`. Ne pas garder de seconde copie de la page
affichée dans le contrôleur.

**Terminé quand.** Preuve verte, JSON du banc joint ; commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire le test.** Créer `tests/tst_ui_document.cpp` :

```cpp
#include <actcraft/core/document.h>
#include <actcraft/core/generator.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/documentcontroller.h>

#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>
#include <variant>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

core::Stroke redStroke()
{
    core::Stroke s;
    s.points = {core::StrokePoint{100, 100, 0.5f}, core::StrokePoint{300, 100, 0.5f}};
    s.baseWidth = 8;
    s.color = 0xFFE53935u;
    return s;
}

} // namespace

// Document ouvert et page affichée (squelette du lot 1, DA2).
class TstUiDocument : public QObject
{
    Q_OBJECT

private slots:
    void startsWithOneBlankPageShown()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QCOMPARE(controller.pageCount(), 1);
        QCOMPARE(controller.currentIndex(), 0);
        QCOMPARE(controller.pageLabel(), QStringLiteral("1 / 1"));
        QCOMPARE(fixed.objectCount(), 0);
        QTRY_VERIFY(fixed.isReady());
    }

    void nextOnLastPageAddsABlankPage()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QSignalSpy pages(&controller, &ui::DocumentController::pagesChanged);
        controller.nextPage();
        QCOMPARE(pages.count(), 1);
        QCOMPARE(controller.pageCount(), 2);
        QCOMPARE(controller.currentIndex(), 1);
        QCOMPARE(controller.pageLabel(), QStringLiteral("2 / 2"));
        controller.previousPage();
        QCOMPARE(controller.pageLabel(), QStringLiteral("1 / 2"));
        controller.nextPage(); // la page 2 existe : pas de troisième page
        QCOMPARE(controller.pageCount(), 2);
    }

    void previousOnFirstPageDoesNothing()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QSignalSpy pages(&controller, &ui::DocumentController::pagesChanged);
        controller.previousPage();
        QCOMPARE(pages.count(), 0);
        QCOMPARE(controller.currentIndex(), 0);
    }

    void strokeSurvivesPageRoundTrip()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        fixed.appendStroke(redStroke()); // comme le stylo (FixedPageSink)
        controller.nextPage();
        QCOMPARE(fixed.objectCount(), 0);
        controller.previousPage();
        QCOMPARE(fixed.objectCount(), 1);
        const core::Document& document = controller.document();
        QCOMPARE(document.page(0).objects.size(), std::size_t{1});
        QVERIFY(std::get<core::Stroke>(document.page(0).objects[0]).color == 0xFFE53935u);
        QVERIFY(document.page(1).objects.empty());
    }

    void documentReadsTheShownPage()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        fixed.appendStroke(redStroke());
        QCOMPARE(controller.document().page(0).objects.size(), std::size_t{1});
    }

    void generatedFirstPageForTheBench()
    {
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.loadGeneratedFirstPage();
        QCOMPARE(fixed.objectCount(), 300);
        QCOMPARE(core::fingerprint(controller.document().page(0)),
                 core::fingerprint(core::generatePage(core::kDefaultSeed)));
        controller.nextPage();
        QCOMPARE(fixed.objectCount(), 0);
        QCOMPARE(controller.pageLabel(), QStringLiteral("2 / 2"));
    }

    void registeredInQml()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import ActCraft.Ui\nDocumentController {}\n", QUrl());
        std::unique_ptr<QObject> controller(component.create());
        QVERIFY2(controller, qPrintable(component.errorString()));
        QCOMPARE(controller->property("pageLabel").toString(), QStringLiteral("1 / 1"));
        QVERIFY(QMetaObject::invokeMethod(controller.get(), "nextPage"));
        QCOMPARE(controller->property("pageCount").toInt(), 2);
    }
};

QTEST_MAIN(TstUiDocument)
#include "tst_ui_document.moc"
```

Ajouter à la fin de `tests/ui.cmake` :

```cmake
actcraft_add_test(tst_ui_document
    SOURCES tst_ui_document.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
```

- [ ] **Étape 2 : le voir échouer.** Commande ciblée, motif `tst_ui_document`. Attendu : `error`
(`documentcontroller.h` introuvable).

- [ ] **Étape 3 : écrire `ui/include/actcraft/ui/documentcontroller.h`.**

```cpp
#pragma once

#include <actcraft/core/document.h>
#include <actcraft/render/fixedpageitem.h>

#include <QObject>
#include <QPointer>
#include <QString>
#include <QtQml/qqmlregistration.h>

#include <cstddef>

namespace actcraft::ui {

// Document ouvert et page affichée (squelette du lot 1, DA2). Une seule source de vérité à la fois : la page
// affichée vit dans l'élément « page fixe » (le stylo y verse ses traits) ; le document garde les autres pages.
// La page affichée est recopiée dans le document avant tout changement de page et à toute lecture (document()).
// Le lot 2 remplace ce relais par le vrai modèle et sa pile d'annulation.
class DocumentController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        actcraft::render::FixedPageItem* fixedPage READ fixedPage WRITE setFixedPage NOTIFY fixedPageChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY pagesChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY pagesChanged)
    Q_PROPERTY(QString pageLabel READ pageLabel NOTIFY pagesChanged)

public:
    explicit DocumentController(QObject* parent = nullptr);

    render::FixedPageItem* fixedPage() const noexcept;
    // L'élément reçoit aussitôt la page courante.
    void setFixedPage(render::FixedPageItem* page);
    int pageCount() const noexcept;
    int currentIndex() const noexcept;
    QString pageLabel() const; // « n / N » (F-002)
    // Document à jour de la page affichée.
    const core::Document& document();

    // F-125 : sur la dernière page, ajoute une page vierge et l'affiche.
    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void previousPage();
    // Banc et tests : la page 1 devient la page générée de 300 objets (core::generatePage).
    Q_INVOKABLE void loadGeneratedFirstPage();

signals:
    void fixedPageChanged();
    void pagesChanged();

private:
    void syncFromView();
    void show(std::size_t index);

    core::Document m_document;
    std::size_t m_current = 0;
    QPointer<render::FixedPageItem> m_fixed;
};

} // namespace actcraft::ui
```

- [ ] **Étape 4 : écrire `ui/src/documentcontroller.cpp`.**

```cpp
#include <actcraft/core/generator.h>
#include <actcraft/ui/documentcontroller.h>

namespace actcraft::ui {

DocumentController::DocumentController(QObject* parent) : QObject(parent) {}

render::FixedPageItem* DocumentController::fixedPage() const noexcept
{
    return m_fixed.data();
}

void DocumentController::setFixedPage(render::FixedPageItem* page)
{
    if (page == m_fixed) {
        return;
    }
    syncFromView();
    m_fixed = page;
    if (m_fixed) {
        m_fixed->setPage(m_document.page(m_current));
    }
    emit fixedPageChanged();
}

int DocumentController::pageCount() const noexcept
{
    return static_cast<int>(m_document.pageCount());
}

int DocumentController::currentIndex() const noexcept
{
    return static_cast<int>(m_current);
}

QString DocumentController::pageLabel() const
{
    return QStringLiteral("%1 / %2").arg(m_current + 1).arg(m_document.pageCount());
}

const core::Document& DocumentController::document()
{
    syncFromView();
    return m_document;
}

void DocumentController::nextPage()
{
    syncFromView();
    show(core::nextPageIndex(m_document, m_current));
}

void DocumentController::previousPage()
{
    const std::size_t target = core::previousPageIndex(m_current);
    if (target == m_current) {
        return;
    }
    show(target);
}

void DocumentController::loadGeneratedFirstPage()
{
    syncFromView();
    m_document.page(0) = core::generatePage(core::kDefaultSeed);
    if (m_current == 0 && m_fixed) {
        m_fixed->setPage(m_document.page(0));
    }
    emit pagesChanged();
}

void DocumentController::syncFromView()
{
    if (m_fixed) {
        m_document.page(m_current) = m_fixed->page();
    }
}

void DocumentController::show(std::size_t index)
{
    syncFromView();
    m_current = index;
    if (m_fixed) {
        m_fixed->setPage(m_document.page(m_current));
    }
    emit pagesChanged();
}

} // namespace actcraft::ui
```

Dans `ui/CMakeLists.txt`, dans `SOURCES` de `qt_add_qml_module`, ajouter `include/actcraft/ui/documentcontroller.h`
après `include/actcraft/ui/pageview.h`, et `src/documentcontroller.cpp` après `src/pageview.cpp`.

- [ ] **Étape 5 : le voir passer.** Commande ciblée, motif `tst_ui_document`. Attendu : `0 tests failed out of 1`.

- [ ] **Étape 6 : format, preuve, banc.** `pwsh -NoProfile -File ./scripts/format.ps1` ; forme bornée (N ≥ 32,
`TESTS OK`, `tst_ui_document .*Passed`) ; banc sans écran (résultat).

- [ ] **Étape 7 : commit.**

```powershell
Set-Location <worktree>; git add ui/CMakeLists.txt ui/include/actcraft/ui/documentcontroller.h ui/src/documentcontroller.cpp tests/ui.cmake tests/tst_ui_document.cpp
git -C <worktree> commit -m "ui : contrôleur du document (pages, page affichée, page suivante qui ajoute)" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

<a id="tache-l1-t10"></a>
### Tâche L1-T10 — `ui` : pont QML `Commands` (menus, palette, raccourcis filtrés, déclenchement)

**Objectif.** DA1 côté interface : un élément QML `Commands` charge les déclarations (`commands.json`) et la
disposition (`layout.json`, profil `<AppConfigLocation>/disposition` d'abord, sinon ressources ; repli sur les
ressources si celle du profil est illisible), et expose à QML : les menus (commandes disponibles sur la plateforme,
séparateurs jamais en tête, en fin ni doublés, **menus vides masqués**), les groupes de la palette (groupes
spéciaux `couleurs` et `epaisseur` gardés), les raccourcis (`{id, sequence}`, remplacements du profil compris), les
couleurs et épaisseurs, l'URL de l'icône de chaque commande, l'état actif / coché (avec `revision` pour les
liaisons QML) et `trigger(id, argument)`. Les gestionnaires sont des fonctions JavaScript posées par `setHandler`
(la coquille, L1-T12), gardées à travers `reload()`.

**Fichiers.**
- Créer : `ui/include/actcraft/ui/commands.h`, `ui/src/commands.cpp`, `tests/tst_ui_commands.cpp`
- Modifier : `ui/CMakeLists.txt`, `tests/ui.cmake`

**Test et commande bornée.** `tst_ui_commands` (7 fonctions). Rouge : commande ciblée, motif `tst_ui_commands` →
`error`. Vert : forme bornée, N ≥ 35, `TESTS OK`, `tst_ui_commands .*Passed` ; banc sans écran (résultat).

**Interdits.** Pas de logique de déclenchement en double : `Commands::trigger` passe toujours par
`CommandRegistry::trigger` (plateforme, état, argument). Pas de `#ifdef` de plateforme (DA10 : la plateforme vient de
`actions::currentPlatform()`).

**Terminé quand.** Preuve verte, JSON du banc joint ; commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire le test.** Créer `tests/tst_ui_commands.cpp` :

```cpp
#include <actcraft/ui/commands.h>

#include <QFile>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using actcraft::actions::Platform;
using actcraft::ui::Commands;

namespace {

QStringList menuLabels(const Commands& commands)
{
    QStringList out;
    for (const QVariant& menu : commands.menus()) {
        out << menu.toMap().value(QStringLiteral("label")).toString();
    }
    return out;
}

QStringList itemIds(const QVariant& menu)
{
    QStringList out;
    for (const QVariant& item : menu.toMap().value(QStringLiteral("items")).toList()) {
        const QVariantMap map = item.toMap();
        out << (map.value(QStringLiteral("separator")).toBool() ? QStringLiteral("-")
                                                                 : map.value(QStringLiteral("id")).toString());
    }
    return out;
}

bool writeFile(const QString& path, const QByteArray& data)
{
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}

} // namespace

// Pont QML du registre de commandes (DA1).
class TstUiCommands : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_emptyProfile; // profil sans disposition : celle des ressources

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY(m_emptyProfile.isValid());
    }

    void defaultMenusHideEmptyOnes()
    {
        Commands c;
        c.setProfileDir(m_emptyProfile.path());
        c.setPlatform(Platform::Windows);
        QVERIFY2(c.errors().isEmpty(), qPrintable(c.errors().join(QLatin1Char('\n'))));
        QCOMPARE(menuLabels(c), (QStringList{QStringLiteral("Fichier"), QStringLiteral("Affichage"),
                                             QStringLiteral("Outils"), QStringLiteral("Aide")}));
        QCOMPARE(itemIds(c.menus().at(1)),
                 (QStringList{QStringLiteral("view.fullscreen"), QStringLiteral("-"),
                              QStringLiteral("page.previous"), QStringLiteral("page.next"), QStringLiteral("-"),
                              QStringLiteral("view.fit")}));
        const QVariantMap fullscreen =
            c.menus().at(1).toMap().value(QStringLiteral("items")).toList().at(0).toMap();
        QCOMPARE(fullscreen.value(QStringLiteral("label")).toString(), QStringLiteral("Plein écran"));
        QCOMPARE(fullscreen.value(QStringLiteral("shortcut")).toString(), QStringLiteral("F5"));
        QVERIFY(fullscreen.value(QStringLiteral("checkable")).toBool());
        QCOMPARE(c.platformName(), QStringLiteral("windows"));
    }

    void paletteColorsAndWidths()
    {
        Commands c;
        c.setProfileDir(m_emptyProfile.path());
        QStringList groups;
        for (const QVariant& group : c.palette()) {
            groups << group.toMap().value(QStringLiteral("group")).toString();
        }
        QCOMPARE(groups, (QStringList{QStringLiteral("commandes"), QStringLiteral("pages"),
                                      QStringLiteral("couleurs"), QStringLiteral("epaisseur"),
                                      QStringLiteral("outils")}));
        const QVariantMap first = c.palette().at(0).toMap().value(QStringLiteral("items")).toList().at(0).toMap();
        QCOMPARE(first.value(QStringLiteral("id")).toString(), QStringLiteral("menu.main"));
        QCOMPARE(first.value(QStringLiteral("icon")).toString(),
                 QStringLiteral("qrc:/qt/qml/ActCraft/Ui/icons/menu.svg"));
        QCOMPARE(c.colors().size(), qsizetype{16});
        QCOMPARE(c.penWidths(), (QVariantList{2.0, 4.0, 6.0, 8.0}));
    }

    void everyDeclaredIconExists()
    {
        Commands c;
        c.setProfileDir(m_emptyProfile.path());
        for (const QString& id : c.registry().ids()) {
            const QString url = c.iconUrl(id);
            if (!url.isEmpty()) {
                QVERIFY2(QFile::exists(url.mid(3)), qPrintable(url)); // « qrc:/… » → « :/… »
            }
        }
    }

    void profileLayoutOverridesResource()
    {
        QTemporaryDir profile;
        QVERIFY(writeFile(profile.filePath(QStringLiteral("layout.json")),
                          R"({"format":"actcraft-layout/1",
                              "menus":[{"id":"help","label":"Aide",
                                        "items":["-","file.quit","-","-","help.about","-"]}],
                              "shortcuts":{"page.next":"Space"}})"));
        Commands c;
        c.setProfileDir(profile.path());
        QCOMPARE(menuLabels(c), QStringList{QStringLiteral("Aide")});
        QCOMPARE(itemIds(c.menus().at(0)),
                 (QStringList{QStringLiteral("file.quit"), QStringLiteral("-"), QStringLiteral("help.about")}));
        QCOMPARE(c.shortcut(QStringLiteral("page.next")), QStringLiteral("Space"));
        bool found = false;
        for (const QVariant& s : c.shortcuts()) {
            const QVariantMap map = s.toMap();
            if (map.value(QStringLiteral("id")).toString() == QLatin1String("page.next")) {
                found = map.value(QStringLiteral("sequence")).toString() == QLatin1String("Space");
            }
        }
        QVERIFY(found);
        QVERIFY(c.palette().isEmpty());
    }

    void unreadableProfileLayoutFallsBackToResource()
    {
        QTemporaryDir profile;
        QVERIFY(writeFile(profile.filePath(QStringLiteral("layout.json")), QByteArray("{")));
        Commands c;
        c.setProfileDir(profile.path());
        QVERIFY(!c.errors().isEmpty());
        QCOMPARE(menuLabels(c).size(), qsizetype{4});
    }

    void triggerReachesTheQmlHandler()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\nItem {\n"
                          "  property var recu: null\n"
                          "  property QtObject registre: commandes\n"
                          "  Commands { id: commandes }\n"
                          "  Component.onCompleted: commandes.setHandler(\"pen.width\", function (w) { recu = w })\n"
                          "}\n",
                          QUrl());
        std::unique_ptr<QObject> root(component.create());
        QVERIFY2(root, qPrintable(component.errorString()));
        auto* c = qobject_cast<Commands*>(root->property("registre").value<QObject*>());
        QVERIFY(c);
        QSignalSpy triggered(c, &Commands::triggered);
        QVERIFY(c->trigger(QStringLiteral("pen.width"), 8));
        QCOMPARE(root->property("recu").toDouble(), 8.0);
        QCOMPARE(triggered.count(), 1);
        QVERIFY(!c->trigger(QStringLiteral("pen.width"), QStringLiteral("épais")));
        QVERIFY(!c->trigger(QStringLiteral("page.next"))); // sans gestionnaire
        QVERIFY(!c->trigger(QStringLiteral("absent")));
        QCOMPARE(triggered.count(), 1);
        c->reload(); // les gestionnaires survivent au rechargement
        QVERIFY(c->trigger(QStringLiteral("pen.width"), 6));
        QCOMPARE(root->property("recu").toDouble(), 6.0);
    }

    void stateChangesBumpRevision()
    {
        Commands c;
        c.setProfileDir(m_emptyProfile.path());
        QSignalSpy state(&c, &Commands::stateChanged);
        const int before = c.revision();
        c.setChecked(QStringLiteral("view.fullscreen"), true);
        QVERIFY(c.isChecked(QStringLiteral("view.fullscreen")));
        QCOMPARE(c.revision(), before + 1);
        QCOMPARE(state.count(), 1);
        c.setEnabled(QStringLiteral("page.next"), false);
        QVERIFY(!c.isEnabled(QStringLiteral("page.next")));
        QCOMPARE(state.count(), 2);
        c.setChecked(QStringLiteral("view.fullscreen"), true); // inchangé : pas de signal
        QCOMPARE(state.count(), 2);
    }
};

QTEST_MAIN(TstUiCommands)
#include "tst_ui_commands.moc"
```

Ajouter à la fin de `tests/ui.cmake` :

```cmake
actcraft_add_test(tst_ui_commands
    SOURCES tst_ui_commands.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
```

- [ ] **Étape 2 : le voir échouer.** Commande ciblée, motif `tst_ui_commands`. Attendu : `error` (`commands.h`
introuvable).

- [ ] **Étape 3 : écrire `ui/include/actcraft/ui/commands.h`.**

```cpp
#pragma once

#include <actcraft/actions/commandregistry.h>
#include <actcraft/actions/layout.h>

#include <QHash>
#include <QJSValue>
#include <QObject>
#include <QStringList>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Pont QML du registre de commandes (DA1) : déclarations et disposition lues au démarrage ; menus, palette et
// raccourcis filtrés par plateforme ; déclenchement par nom. Les gestionnaires sont des fonctions JavaScript
// (setHandler), appelées avec l'argument de la commande.
class Commands : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList menus READ menus NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList palette READ palette NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList shortcuts READ shortcuts NOTIFY layoutChanged)
    Q_PROPERTY(QStringList colors READ colors NOTIFY layoutChanged)
    Q_PROPERTY(QVariantList penWidths READ penWidths NOTIFY layoutChanged)
    Q_PROPERTY(QString platformName READ platformName NOTIFY layoutChanged)
    Q_PROPERTY(QStringList errors READ errors NOTIFY layoutChanged)
    Q_PROPERTY(int revision READ revision NOTIFY stateChanged)

public:
    explicit Commands(QObject* parent = nullptr);

    // Tests : dossier du profil (défaut : <AppConfigLocation>/disposition) et plateforme ; rechargent.
    void setProfileDir(const QString& dir);
    void setPlatform(actions::Platform platform);
    // Relit déclarations et disposition ; garde les gestionnaires posés.
    Q_INVOKABLE void reload();

    // [{id, label, items: [{id, label, shortcut, checkable, separator: false} | {separator: true}]}] ; menus
    // sans commande disponible omis ; séparateurs jamais en tête, en fin ni doublés.
    QVariantList menus() const;
    // [{group, items: [{id, label, icon, checkable}]}] ; « couleurs » et « epaisseur » gardés sans items.
    QVariantList palette() const;
    // [{id, sequence}] des commandes disponibles qui ont un raccourci.
    QVariantList shortcuts() const;
    QStringList colors() const;
    QVariantList penWidths() const;
    QString platformName() const;
    QStringList errors() const;
    int revision() const noexcept;
    const actions::CommandRegistry& registry() const noexcept;

    Q_INVOKABLE bool trigger(const QString& id, const QVariant& argument = QVariant());
    Q_INVOKABLE void setHandler(const QString& id, const QJSValue& handler);
    Q_INVOKABLE QString label(const QString& id) const;
    // « qrc:/qt/qml/ActCraft/Ui/icons/<icône>.svg », ou vide.
    Q_INVOKABLE QString iconUrl(const QString& id) const;
    Q_INVOKABLE QString shortcut(const QString& id) const;
    Q_INVOKABLE bool isAvailable(const QString& id) const;
    Q_INVOKABLE bool isEnabled(const QString& id) const;
    Q_INVOKABLE void setEnabled(const QString& id, bool enabled);
    Q_INVOKABLE bool isChecked(const QString& id) const;
    Q_INVOKABLE void setChecked(const QString& id, bool checked);

signals:
    void layoutChanged();
    void stateChanged();
    void triggered(const QString& id, const QVariant& argument);

private:
    void bindHandler(const QString& id);
    void bumpState();

    actions::CommandRegistry m_registry;
    actions::Layout m_layout;
    actions::Platform m_platform;
    QString m_profileDir;
    QStringList m_errors;
    QHash<QString, QJSValue> m_handlers;
    int m_revision = 0;
};

} // namespace actcraft::ui
```

- [ ] **Étape 4 : écrire `ui/src/commands.cpp`.**

```cpp
#include <actcraft/ui/commands.h>

#include <QCoreApplication>
#include <QJSEngine>
#include <QStandardPaths>

namespace actcraft::ui {

namespace {

QString defaultProfileDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/disposition");
}

} // namespace

Commands::Commands(QObject* parent)
    : QObject(parent), m_platform(actions::currentPlatform()), m_profileDir(defaultProfileDir())
{
    reload();
}

void Commands::setProfileDir(const QString& dir)
{
    m_profileDir = dir;
    reload();
}

void Commands::setPlatform(actions::Platform platform)
{
    m_platform = platform;
    reload();
}

void Commands::reload()
{
    m_registry = actions::CommandRegistry{};
    m_layout = actions::Layout{};
    m_errors.clear();
    QString error;
    if (!m_registry.loadDeclarations(actions::defaultCommandsJson(), error)) {
        m_errors << QStringLiteral("commands.json : %1").arg(error);
    }
    const QString resourceDir = actions::dataResourceDir();
    QString used;
    const QByteArray json =
        actions::readLayeredFile(QStringLiteral("layout.json"), m_profileDir, resourceDir, &used);
    if (!actions::parseLayout(json, m_layout, error)) {
        m_errors << QStringLiteral("%1 : %2").arg(used.isEmpty() ? QStringLiteral("layout.json") : used, error);
        // Disposition du profil illisible : celle des ressources.
        QString ignored;
        const bool fallback = actions::parseLayout(
            actions::readLayeredFile(QStringLiteral("layout.json"), QString(), resourceDir), m_layout, ignored);
        Q_UNUSED(fallback)
    }
    const QStringList unknown = actions::unknownCommands(m_layout, m_registry);
    for (const QString& id : unknown) {
        m_errors << QStringLiteral("commande inconnue dans la disposition : %1").arg(id);
    }
    for (auto it = m_handlers.cbegin(); it != m_handlers.cend(); ++it) {
        bindHandler(it.key());
    }
    for (const QString& message : std::as_const(m_errors)) {
        qWarning("ActCraft : %s", qPrintable(message));
    }
    ++m_revision;
    emit layoutChanged();
    emit stateChanged();
}

void Commands::bindHandler(const QString& id)
{
    m_registry.setHandler(id, [this, id](const QVariant& argument) {
        const QJSValue handler = m_handlers.value(id);
        QJSEngine* engine = qjsEngine(this);
        if (!handler.isCallable() || !engine) {
            return;
        }
        const QJSValue result = handler.call({engine->toScriptValue(argument)});
        if (result.isError()) {
            qWarning("ActCraft : commande %s : %s", qPrintable(id), qPrintable(result.toString()));
        }
    });
}

void Commands::bumpState()
{
    ++m_revision;
    emit stateChanged();
}

QVariantList Commands::menus() const
{
    QVariantList out;
    for (const actions::MenuSpec& menu : m_layout.menus) {
        QVariantList items;
        bool pendingSeparator = false;
        for (const QString& id : menu.items) {
            if (id == QLatin1String("-")) {
                pendingSeparator = !items.isEmpty();
                continue;
            }
            if (!isAvailable(id)) {
                continue;
            }
            if (pendingSeparator) {
                items.append(QVariantMap{{QStringLiteral("separator"), true}});
                pendingSeparator = false;
            }
            const actions::CommandInfo* info = m_registry.find(id);
            items.append(QVariantMap{{QStringLiteral("id"), id},
                                     {QStringLiteral("label"), label(id)},
                                     {QStringLiteral("shortcut"), shortcut(id)},
                                     {QStringLiteral("checkable"), info->checkable},
                                     {QStringLiteral("separator"), false}});
        }
        if (items.isEmpty()) {
            continue; // menu vide (lot 1 : Éditer, Insérer) : pas affiché
        }
        out.append(QVariantMap{{QStringLiteral("id"), menu.id},
                               {QStringLiteral("label"), menu.label},
                               {QStringLiteral("items"), items}});
    }
    return out;
}

QVariantList Commands::palette() const
{
    QVariantList out;
    for (const actions::PaletteGroup& group : m_layout.palette) {
        const bool special =
            group.group == QLatin1String("couleurs") || group.group == QLatin1String("epaisseur");
        QVariantList items;
        for (const QString& id : group.items) {
            if (!isAvailable(id)) {
                continue;
            }
            items.append(QVariantMap{{QStringLiteral("id"), id},
                                     {QStringLiteral("label"), label(id)},
                                     {QStringLiteral("icon"), iconUrl(id)},
                                     {QStringLiteral("checkable"), m_registry.find(id)->checkable}});
        }
        if (!special && items.isEmpty()) {
            continue;
        }
        out.append(QVariantMap{{QStringLiteral("group"), group.group}, {QStringLiteral("items"), items}});
    }
    return out;
}

QVariantList Commands::shortcuts() const
{
    QVariantList out;
    for (const QString& id : m_registry.ids()) {
        const QString sequence = shortcut(id);
        if (!isAvailable(id) || sequence.isEmpty()) {
            continue;
        }
        out.append(QVariantMap{{QStringLiteral("id"), id}, {QStringLiteral("sequence"), sequence}});
    }
    return out;
}

QStringList Commands::colors() const
{
    return m_layout.colors;
}

QVariantList Commands::penWidths() const
{
    QVariantList out;
    for (const double width : m_layout.penWidths) {
        out << width;
    }
    return out;
}

QString Commands::platformName() const
{
    return actions::platformName(m_platform);
}

QStringList Commands::errors() const
{
    return m_errors;
}

int Commands::revision() const noexcept
{
    return m_revision;
}

const actions::CommandRegistry& Commands::registry() const noexcept
{
    return m_registry;
}

bool Commands::trigger(const QString& id, const QVariant& argument)
{
    if (!m_registry.trigger(id, argument, m_platform)) {
        return false;
    }
    emit triggered(id, argument);
    return true;
}

void Commands::setHandler(const QString& id, const QJSValue& handler)
{
    if (!m_registry.find(id)) {
        qWarning("ActCraft : gestionnaire pour une commande inconnue : %s", qPrintable(id));
        return;
    }
    m_handlers.insert(id, handler);
    bindHandler(id);
}

QString Commands::label(const QString& id) const
{
    const actions::CommandInfo* info = m_registry.find(id);
    return info ? QCoreApplication::translate("commandes", info->label.toUtf8().constData()) : QString();
}

QString Commands::iconUrl(const QString& id) const
{
    const actions::CommandInfo* info = m_registry.find(id);
    if (!info || info->icon.isEmpty()) {
        return {};
    }
    return QStringLiteral("qrc:/qt/qml/ActCraft/Ui/icons/%1.svg").arg(info->icon);
}

QString Commands::shortcut(const QString& id) const
{
    const auto it = m_layout.shortcuts.constFind(id);
    if (it != m_layout.shortcuts.cend()) {
        return it.value();
    }
    const actions::CommandInfo* info = m_registry.find(id);
    return info ? info->shortcut : QString();
}

bool Commands::isAvailable(const QString& id) const
{
    return m_registry.isAvailable(id, m_platform);
}

bool Commands::isEnabled(const QString& id) const
{
    return m_registry.isEnabled(id);
}

void Commands::setEnabled(const QString& id, bool enabled)
{
    if (!m_registry.find(id) || m_registry.isEnabled(id) == enabled) {
        return;
    }
    m_registry.setEnabled(id, enabled);
    bumpState();
}

bool Commands::isChecked(const QString& id) const
{
    return m_registry.isChecked(id);
}

void Commands::setChecked(const QString& id, bool checked)
{
    if (!m_registry.find(id) || m_registry.isChecked(id) == checked) {
        return;
    }
    m_registry.setChecked(id, checked);
    bumpState();
}

} // namespace actcraft::ui
```

Dans `ui/CMakeLists.txt`, dans `SOURCES`, ajouter `include/actcraft/ui/commands.h` après
`include/actcraft/ui/documentcontroller.h` et `src/commands.cpp` après `src/documentcontroller.cpp`.

- [ ] **Étape 5 : le voir passer.** Commande ciblée, motif `tst_ui_commands`. Attendu : `0 tests failed out of 1`.

- [ ] **Étape 6 : format, preuve, banc.** `pwsh -NoProfile -File ./scripts/format.ps1` ; forme bornée (N ≥ 35,
`TESTS OK`, `tst_ui_commands .*Passed`) ; banc sans écran (résultat).

- [ ] **Étape 7 : commit.**

```powershell
Set-Location <worktree>; git add ui/CMakeLists.txt ui/include/actcraft/ui/commands.h ui/src/commands.cpp tests/ui.cmake tests/tst_ui_commands.cpp
git -C <worktree> commit -m "ui : pont QML des commandes (menus, palette, raccourcis, déclenchement)" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

<a id="tache-l1-t11"></a>
### Tâche L1-T11 — Version, « À propos », notices, liste des modules Qt autorisés

**Objectif.** Décisions 31 et 34, F-760 :
1. `actcraft --version` écrit `ActCraft <version>` (version de `project(VERSION)`, 0.1.0) sur la sortie standard et
   sort à 0, **avant** toute fenêtre (S1) ;
2. singleton QML `AppInfo` (version, licence MIT, page du projet, notices) et dialogue `AboutDialog` (« À propos
   d'ActCraft », bouton « Fermer » écrit par nous, notices défilantes) ;
3. `THIRD_PARTY_NOTICES.md` (Qt en LGPLv3 lié dynamiquement, composants tiers de Qt) intégré aux ressources et livré
   dans les paquets, avec `LICENSES/LGPL-3.0.txt` et `LICENSES/GPL-3.0.txt` ;
4. `packaging/qt-autorises.txt` et `scripts/verifier-licences.ps1` : les bibliothèques Qt d'un dossier déployé ou
   d'un APK sont comparées à la liste autorisée (aucun module sous GPL seule) ;
5. `scripts/version-projet.ps1` : version et `versionCode` Android (majeur × 10000 + mineur × 100 + correctif) lus
   dans `CMakeLists.txt`, pour tous les scripts de paquets.

**Fichiers.**
- Créer : `ui/include/actcraft/ui/appinfo.h`, `ui/src/appinfo.cpp`, `ui/qml/AboutDialog.qml`,
  `THIRD_PARTY_NOTICES.md`, `LICENSES/LGPL-3.0.txt`, `LICENSES/GPL-3.0.txt`, `packaging/qt-autorises.txt`,
  `scripts/verifier-licences.ps1`, `scripts/version-projet.ps1`, `tests/app.cmake`, `tests/tst_ui_about.cpp`
- Modifier : `ui/CMakeLists.txt`, `app/CMakeLists.txt`, `app/main.cpp`, `tests/CMakeLists.txt`, `tests/ui.cmake`

**Test et commande bornée.** Quatre tests nouveaux : `tst_ui_about` (2 fonctions), `tst_app_version`
(`actcraft --version`, motif `^ActCraft 0.1.0`), `tst_licences_ok` (dossier factice autorisé → `LICENCES OK`),
`tst_licences_refus` (dossier factice avec `Qt6Charts.dll` → `LICENCES EN ECHEC : Qt6Charts`). Rouge : commande
ciblée, motif `tst_ui_about|tst_app_version|tst_licences` → `error` ou `tests failed`. Vert : forme bornée, N ≥ 39,
`TESTS OK`, les quatre lignes `Passed` ; banc sans écran (résultat).

**Interdits.** Pas de `QCommandLineParser::showVersion()` (boîte de message sous Windows, l'application n'ayant pas
de console). Aucun texte de l'aide d'ActivInspire, aucune mention de Promethean dans « À propos » (N1-12). Les
textes de licence de la FSF sont copiés tels quels (seule exception au « tout écrit par nous » : ce sont des textes
légaux à reproduire à l'identique).

**Terminé quand.** Preuve verte, JSON du banc joint ; commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests.** Créer `tests/tst_ui_about.cpp` :

```cpp
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

// « À propos » et identité de l'application (F-760, décision 34).
class TstUiAbout : public QObject
{
    Q_OBJECT

private slots:
    void appInfoGivesVersionLicenseAndNotices()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\nQtObject {\n"
                          "  property string version: AppInfo.version\n"
                          "  property string licence: AppInfo.licenseName\n"
                          "  property string url: AppInfo.projectUrl\n"
                          "  property string notices: AppInfo.notices\n"
                          "}\n",
                          QUrl());
        std::unique_ptr<QObject> info(component.create());
        QVERIFY2(info, qPrintable(component.errorString()));
        QCOMPARE(info->property("version").toString(), QStringLiteral(ACTCRAFT_TEST_VERSION));
        QCOMPARE(info->property("licence").toString(), QStringLiteral("MIT"));
        QCOMPARE(info->property("url").toString(), QStringLiteral("https://github.com/jejeasterix/actcraft"));
        const QString notices = info->property("notices").toString();
        QVERIFY2(notices.contains(QLatin1String("Qt")) && notices.contains(QLatin1String("LGPL")),
                 "THIRD_PARTY_NOTICES.md absent des ressources");
    }

    void aboutDialogLoadsWithOurTitle()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\nWindow {\n"
                          "  width: 800; height: 600\n"
                          "  property QtObject dialogue: about\n"
                          "  AboutDialog { id: about }\n"
                          "}\n",
                          QUrl());
        std::unique_ptr<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        QObject* dialog = window->property("dialogue").value<QObject*>();
        QVERIFY(dialog);
        QCOMPARE(dialog->objectName(), QStringLiteral("aboutDialog"));
        QCOMPARE(dialog->property("title").toString(), QStringLiteral("À propos d'ActCraft"));
    }
};

QTEST_MAIN(TstUiAbout)
#include "tst_ui_about.moc"
```

Ajouter à la fin de `tests/ui.cmake` :

```cmake
actcraft_add_test(tst_ui_about
    SOURCES tst_ui_about.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_compile_definitions(tst_ui_about PRIVATE ACTCRAFT_TEST_VERSION="${PROJECT_VERSION}")
```

Créer `tests/app.cmake` :

```cmake
# Tests de l'exécutable et des scripts de paquets (S1, décision 34).

# --version : écrit la version et sort avant toute fenêtre.
add_test(NAME tst_app_version COMMAND $<TARGET_FILE:actcraft> --version)
set_tests_properties(tst_app_version PROPERTIES PASS_REGULAR_EXPRESSION "^ActCraft ${PROJECT_VERSION}")

# Liste des modules Qt autorisés : un dossier conforme passe, un module sous GPL seule est refusé.
find_program(ACTCRAFT_PWSH pwsh)
if(ACTCRAFT_PWSH)
    set(_licences ${CMAKE_BINARY_DIR}/licences-essai)
    file(MAKE_DIRECTORY ${_licences}/autorise ${_licences}/refuse)
    file(WRITE ${_licences}/autorise/Qt6Core.dll "")
    file(WRITE ${_licences}/autorise/Qt6Quick.dll "")
    file(WRITE ${_licences}/refuse/Qt6Core.dll "")
    file(WRITE ${_licences}/refuse/Qt6Charts.dll "")
    add_test(NAME tst_licences_ok
        COMMAND ${ACTCRAFT_PWSH} -NoProfile -File ${PROJECT_SOURCE_DIR}/scripts/verifier-licences.ps1
            -Dossier ${_licences}/autorise)
    set_tests_properties(tst_licences_ok PROPERTIES PASS_REGULAR_EXPRESSION "LICENCES OK")
    add_test(NAME tst_licences_refus
        COMMAND ${ACTCRAFT_PWSH} -NoProfile -File ${PROJECT_SOURCE_DIR}/scripts/verifier-licences.ps1
            -Dossier ${_licences}/refuse)
    set_tests_properties(tst_licences_refus PROPERTIES PASS_REGULAR_EXPRESSION "LICENCES EN ECHEC : Qt6Charts")
endif()
```

Dans `tests/CMakeLists.txt`, après la ligne `include(${CMAKE_CURRENT_SOURCE_DIR}/bench.cmake)`, ajouter :

```cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/app.cmake)
```

- [ ] **Étape 2 : les voir échouer.** Commande ciblée, motif `tst_ui_about|tst_app_version|tst_licences`. Attendu :
`tst_ui_about` ne compile pas (`AppInfo` inconnu en QML → échec à l'exécution, ou `ACTCRAFT_TEST_VERSION` défini
mais type absent) ; `tst_app_version` échoue (l'exécutable ouvre sa fenêtre au lieu d'écrire la version : arrêté
par le délai de ctest — lancer cette commande ciblée avec `--timeout 20` ajouté après `-R '<motif>'`) ;
`tst_licences_*` échouent (script absent).

- [ ] **Étape 3 : `scripts/version-projet.ps1`.**

```powershell
# Version du projet lue dans « project(ActCraft VERSION x.y.z) » du CMakeLists.txt racine (décision 31).
# Usage : . ./scripts/version-projet.ps1 ; Get-ActCraftVersion ; Get-ActCraftVersionCode
function Get-ActCraftVersion {
    $texte = Get-Content -Raw (Join-Path $PSScriptRoot '../CMakeLists.txt')
    if ($texte -notmatch 'project\(\s*ActCraft\s+VERSION\s+(\d+)\.(\d+)\.(\d+)') {
        throw 'version introuvable dans CMakeLists.txt (project(ActCraft VERSION x.y.z))'
    }
    return "$($Matches[1]).$($Matches[2]).$($Matches[3])"
}

# versionCode Android : majeur × 10000 + mineur × 100 + correctif (0.1.0 → 100).
function Get-ActCraftVersionCode {
    $parties = (Get-ActCraftVersion).Split('.') | ForEach-Object { [int]$_ }
    return $parties[0] * 10000 + $parties[1] * 100 + $parties[2]
}
```

- [ ] **Étape 4 : `packaging/qt-autorises.txt`.**

```text
# Modules Qt autorisés dans les paquets d'ActCraft (décision 34 du cadrage livraison-complete, N1-8).
# Tous sous LGPLv3 et liés dynamiquement. Lu par scripts/verifier-licences.ps1 (un nom de module par ligne).
# Jamais : Qt6VirtualKeyboard, Qt6Charts, Qt6Graphs, Qt6DataVisualization, Qt6Quick3D*, Qt6Lottie*,
# Qt6QuickTimeline* (disponibles seulement sous GPL ou licence commerciale).
# Ajouter un module : vérifier d'abord sa licence sur https://doc.qt.io/qt-6/qtmodules.html, puis l'écrire ici.
Qt6Core
Qt6Gui
Qt6Network
Qt6OpenGL
Qt6Svg
Qt6Qml
Qt6QmlMeta
Qt6QmlModels
Qt6QmlWorkerScript
Qt6Quick
Qt6QuickLayouts
Qt6QuickShapes
Qt6QuickEffects
Qt6QuickTemplates2
Qt6QuickControls2
Qt6QuickControls2Impl
Qt6QuickControls2Basic
Qt6QuickControls2BasicStyleImpl
Qt6QuickControls2Fusion
Qt6QuickControls2FusionStyleImpl
Qt6QuickControls2Material
Qt6QuickControls2MaterialStyleImpl
Qt6QuickControls2Universal
Qt6QuickControls2UniversalStyleImpl
Qt6QuickControls2Imagine
Qt6QuickControls2ImagineStyleImpl
Qt6QuickControls2FluentWinUI3StyleImpl
Qt6QuickControls2WindowsStyleImpl
Qt6QuickDialogs2
Qt6QuickDialogs2QuickImpl
Qt6QuickDialogs2Utils
Qt6LabsFolderListModel
Qt6LabsQmlModels
Qt6LabsSettings
Qt6LabsAnimation
Qt6LabsPlatform
```

- [ ] **Étape 5 : `scripts/verifier-licences.ps1`.**

```powershell
# Compare les bibliothèques Qt d'un dossier déployé ou d'un APK à packaging/qt-autorises.txt (décision 34).
# Usage : pwsh -NoProfile -File ./scripts/verifier-licences.ps1 -Dossier <dossier> | -Apk <fichier.apk>
# Dernière ligne : « LICENCES OK » (0) ou « LICENCES EN ECHEC : <modules> » (1).
param([string]$Dossier, [string]$Apk)
$ErrorActionPreference = 'Stop'

$liste = Join-Path $PSScriptRoot '../packaging/qt-autorises.txt'
$autorises = @(Get-Content $liste | ForEach-Object { $_.Trim() } | Where-Object { $_ -and -not $_.StartsWith('#') })

if ($Apk) {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [System.IO.Compression.ZipFile]::OpenRead((Resolve-Path $Apk).Path)
    try { $noms = @($zip.Entries | ForEach-Object { $_.Name }) } finally { $zip.Dispose() }
} elseif ($Dossier) {
    $noms = @(Get-ChildItem -Recurse -File $Dossier | ForEach-Object { $_.Name })
} else {
    Write-Output 'LICENCES EN ECHEC : ni -Dossier ni -Apk'
    exit 1
}

# Qt6Core.dll (Windows), libQt6Core_arm64-v8a.so (Android) → Qt6Core. Les greffons (qwindows.dll…) suivent leur
# module et ne sont pas comptés.
$modules = @($noms | ForEach-Object {
        if ($_ -match '^(?:lib)?(Qt6[A-Za-z0-9]+?)(?:_[A-Za-z0-9_-]+)?\.(?:dll|so)$') { $Matches[1] }
    } | Sort-Object -Unique)
if ($modules.Count -eq 0) {
    Write-Output 'LICENCES EN ECHEC : aucune bibliothèque Qt trouvée'
    exit 1
}
Write-Output "Modules Qt : $($modules -join ', ')"
$refuses = @($modules | Where-Object { $autorises -notcontains $_ })
if ($refuses.Count -gt 0) {
    Write-Output "LICENCES EN ECHEC : $($refuses -join ', ')"
    exit 1
}
Write-Output 'LICENCES OK'
```

- [ ] **Étape 6 : textes de licence.**

```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force LICENSES | Out-Null
Invoke-WebRequest https://www.gnu.org/licenses/lgpl-3.0.txt -OutFile LICENSES/LGPL-3.0.txt
Invoke-WebRequest https://www.gnu.org/licenses/gpl-3.0.txt -OutFile LICENSES/GPL-3.0.txt
Select-String -Path LICENSES/LGPL-3.0.txt, LICENSES/GPL-3.0.txt -Pattern 'Version 3, 29 June 2007' | Select-Object -First 2
```
Attendu : deux lignes. Ne pas modifier le contenu de ces textes.

- [ ] **Étape 7 : `THIRD_PARTY_NOTICES.md`.**

```markdown
# Avis concernant les composants tiers

ActCraft est distribué sous licence MIT (fichier `LICENSE`). Ses paquets (installateur Windows, APK Android)
contiennent aussi les composants tiers ci-dessous, chacun sous sa propre licence.

## Qt 6.12

ActCraft utilise le framework Qt (The Qt Company Ltd. et autres contributeurs), version 6.12, sous la **GNU Lesser
General Public License version 3** (LGPL-3.0-only ; texte dans `LICENSES/LGPL-3.0.txt`, qui renvoie à la GNU
General Public License version 3, texte dans `LICENSES/GPL-3.0.txt`).

- Qt est lié **dynamiquement** : bibliothèques `Qt6*.dll` à côté de `actcraft.exe` sous Windows, `libQt6*.so` dans
  l'APK sous Android. Vous pouvez remplacer ces bibliothèques par une version de Qt modifiée et compatible.
- Code source de Qt 6.12 : https://download.qt.io/official_releases/qt/6.12/ et https://code.qt.io/.
- Code source d'ActCraft : https://github.com/jejeasterix/actcraft.
- Modules livrés : ceux de `packaging/qt-autorises.txt`, contrôlés à chaque paquet par
  `scripts/verifier-licences.ps1`. Aucun module disponible seulement sous GPL n'est livré.

### Composants tiers inclus dans Qt

D'après la page « Licenses Used in Qt » (https://doc.qt.io/qt-6/licenses-used-in-qt.html), pour les modules
livrés. Selon la plateforme, certains ne sont pas présents dans le paquet.

| Module | Composant | Licence |
|---|---|---|
| Qt Core | Apache Tika MimeType Definitions | Apache-2.0 |
| Qt Core | BLAKE2 (implémentation de référence) | CC0-1.0 ou Apache-2.0 |
| Qt Core | zlib | Zlib |
| Qt Core | Easing Equations (Robert Penner) | BSD-3-Clause |
| Qt Core | double-conversion | BSD-3-Clause |
| Qt Core | MD4, MD5, SHA-1 | domaine public |
| Qt Core | PCRE2 et son compilateur SLJIT | BSD-3-Clause (avec exception PCRE2), BSD-2-Clause |
| Qt Core | SHA-3 Keccak, brg_endian | CC0-1.0, BSD-2-Clause |
| Qt Core | SHA-384 et SHA-512 (RFC 6234) | BSD-3-Clause |
| Qt Core | SipHash, tl::expected | CC0-1.0 |
| Qt Core | TinyCBOR, forkfd | MIT |
| Qt Core | Unicode Character Database, Unicode CLDR | Unicode-DFS-2016, Unicode-3.0 |
| Qt Gui | FreeType 2 (et son rastériseur) | FTL ou GPL-2.0-only (FTL retenue) |
| Qt Gui | FreeType 2 : BDF, PCF, zlib | MIT, Zlib |
| Qt Gui | HarfBuzz-NG | MIT |
| Qt Gui | libjpeg-turbo | IJG et BSD-3-Clause |
| Qt Gui | libpng | Libpng-2.0 |
| Qt Gui | MD4C | MIT |
| Qt Gui | Pixman | MIT |
| Qt Gui | Emoji Segmenter | Apache-2.0 |
| Qt Gui | Adobe Glyph List For New Fonts | BSD-3-Clause |
| Qt Gui | DejaVu Fonts | Bitstream Vera |
| Qt Gui | D3D12 Memory Allocator, Vulkan Memory Allocator, générateur de mipmaps D3D12 | MIT |
| Qt Gui | Vulkan API Registry | Apache-2.0 ou MIT |
| Qt Gui | en-têtes OpenGL et OpenGL ES 2 | MIT |
| Qt Gui | Smooth Scaling Algorithm | BSD-2-Clause et Imlib2 |
| Qt Gui | WebGradients | MIT |
| Qt Gui | Wintab API (Windows) | licence LCS-Telegraphics |
| Qt Gui | Native Style for Android (Android) | Apache-2.0 |
| Qt Gui | profil de couleur sRGB (fichier ICC) | licence de l'International Color Consortium |
| Qt Qml | JavaScriptCore Macro Assembler | BSD-2-Clause |
| Qt Quick | Yoga | MIT |
| Qt Quick Controls | valeurs d'ombre d'Angular Material | MIT |
| Qt Network | Public Suffix List, libpsl | MPL-2.0, BSD-3-Clause |

Les textes complets de ces licences accompagnent le code source de Qt (fichiers `LICENSES/` et
`qt_attribution.json` de chaque module).

## Bibliothèques d'exécution de Microsoft Visual C++ (Windows)

`vcruntime140.dll`, `vcruntime140_1.dll` et `msvcp140.dll` sont redistribués à côté de l'exécutable selon les
conditions des « Distributable Code » de Microsoft Visual Studio 2022.
```

- [ ] **Étape 8 : `AppInfo`.** Créer `ui/include/actcraft/ui/appinfo.h` :

```cpp
#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Identité de l'application pour « À propos » (F-760, décision 34) : version, licence, page du projet, notices.
class AppInfo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString licenseName READ licenseName CONSTANT)
    Q_PROPERTY(QString projectUrl READ projectUrl CONSTANT)
    Q_PROPERTY(QString notices READ notices CONSTANT)

public:
    explicit AppInfo(QObject* parent = nullptr);

    QString version() const;     // project(VERSION) du CMakeLists.txt racine
    QString licenseName() const; // « MIT »
    QString projectUrl() const;  // page du projet (F-760 : l'aide est la page du projet)
    // THIRD_PARTY_NOTICES.md, intégré aux ressources (:/actcraft/THIRD_PARTY_NOTICES.md).
    QString notices() const;
};

} // namespace actcraft::ui
```

Créer `ui/src/appinfo.cpp` :

```cpp
#include <actcraft/ui/appinfo.h>

#include <QFile>

#ifndef ACTCRAFT_VERSION
#error "ACTCRAFT_VERSION doit être défini par ui/CMakeLists.txt"
#endif

// Ressource des notices (qt_add_resources « ui_notices ») : Q_INIT_RESOURCE hors de tout espace de noms.
static void initNoticesResource()
{
    Q_INIT_RESOURCE(ui_notices);
}

namespace actcraft::ui {

AppInfo::AppInfo(QObject* parent) : QObject(parent)
{
    static const bool initialised = [] {
        initNoticesResource();
        return true;
    }();
    Q_UNUSED(initialised)
}

QString AppInfo::version() const
{
    return QStringLiteral(ACTCRAFT_VERSION);
}

QString AppInfo::licenseName() const
{
    return QStringLiteral("MIT");
}

QString AppInfo::projectUrl() const
{
    return QStringLiteral("https://github.com/jejeasterix/actcraft");
}

QString AppInfo::notices() const
{
    QFile file(QStringLiteral(":/actcraft/THIRD_PARTY_NOTICES.md"));
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}

} // namespace actcraft::ui
```

- [ ] **Étape 9 : `ui/qml/AboutDialog.qml`.**

```qml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// « À propos » (F-760, décision 34) : version, licence, page du projet, notices des composants tiers.
Dialog {
    id: dialog
    objectName: "aboutDialog"
    title: qsTr("À propos d'ActCraft")
    modal: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(640, Overlay.overlay ? Overlay.overlay.width - 2 * Theme.espace4 : 640)
    height: Math.min(520, Overlay.overlay ? Overlay.overlay.height - 2 * Theme.espace4 : 520)

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.espace2

        Label {
            objectName: "aboutVersion"
            text: qsTr("ActCraft %1").arg(AppInfo.version)
            font.pixelSize: Theme.taillePoliceTitre
            font.bold: true
        }
        Label {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Logiciel libre de tableau blanc interactif, sous licence %1.").arg(AppInfo.licenseName)
        }
        Label {
            textFormat: Text.RichText
            text: "<a href=\"%1\">%1</a>".arg(AppInfo.projectUrl)
            onLinkActivated: link => Qt.openUrlExternally(link)
        }
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            TextArea {
                objectName: "aboutNotices"
                readOnly: true
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.MarkdownText
                text: AppInfo.notices
            }
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Fermer")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
    }
}
```

- [ ] **Étape 10 : CMake de `ui` et de `app`.** Dans `ui/CMakeLists.txt` : ajouter `qml/AboutDialog.qml` à
`QML_FILES` (après `qml/Theme.qml`), `include/actcraft/ui/appinfo.h` et `src/appinfo.cpp` à `SOURCES` (après les
fichiers de `commands`), puis, à la fin du fichier :

```cmake
# Version affichée par « À propos » (décision 31).
target_compile_definitions(actcraft_ui PRIVATE ACTCRAFT_VERSION="${PROJECT_VERSION}")

# Notices des composants tiers, affichées par « À propos » et livrées dans les paquets (décision 34).
set_source_files_properties(${PROJECT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md
    PROPERTIES QT_RESOURCE_ALIAS THIRD_PARTY_NOTICES.md)
qt_add_resources(actcraft_ui "ui_notices"
    PREFIX "/actcraft"
    FILES ${PROJECT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md)
```
Dans `app/CMakeLists.txt`, après `actcraft_set_warnings(actcraft)` :

```cmake
# `actcraft --version` (décision 31, S1).
target_compile_definitions(actcraft PRIVATE ACTCRAFT_VERSION="${PROJECT_VERSION}")
```

- [ ] **Étape 11 : `--version` dans `app/main.cpp`.** Ajouter `#include <cstring>` aux inclusions et, en tête de
`main`, avant `QGuiApplication app(argc, argv);` :

```cpp
    // `--version` (S1) : avant toute fenêtre, sur la sortie standard. Pas de QCommandLineParser::showVersion :
    // sous Windows, une application sans console l'afficherait dans une boîte de message.
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--version") == 0) {
            std::printf("ActCraft %s\n", ACTCRAFT_VERSION);
            std::fflush(stdout);
            return 0;
        }
    }
```

- [ ] **Étape 12 : les voir passer.** Commande ciblée, motif `tst_ui_about|tst_app_version|tst_licences`. Attendu :
`0 tests failed out of 4`. Puis, à la main :

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/verifier-licences.ps1 -Dossier build/debug | Select-Object -Last 2
```
Attendu : la liste des modules puis `LICENCES OK` ou `LICENCES EN ECHEC : Qt6Test` (normal : `build/debug` contient
les tests ; ne rien ajouter à la liste pour `Qt6Test`, jamais livré).

- [ ] **Étape 13 : format, preuve, banc.** `pwsh -NoProfile -File ./scripts/format.ps1` ; forme bornée (N ≥ 39,
`TESTS OK`, lignes `Passed` de `tst_ui_about`, `tst_app_version`, `tst_licences_ok`, `tst_licences_refus`) ; banc
sans écran (résultat).

- [ ] **Étape 14 : commit.**

```powershell
Set-Location <worktree>; git add ui/CMakeLists.txt ui/include/actcraft/ui/appinfo.h ui/src/appinfo.cpp ui/qml/AboutDialog.qml app/CMakeLists.txt app/main.cpp THIRD_PARTY_NOTICES.md LICENSES/LGPL-3.0.txt LICENSES/GPL-3.0.txt packaging/qt-autorises.txt scripts/verifier-licences.ps1 scripts/version-projet.ps1 tests/CMakeLists.txt tests/app.cmake tests/ui.cmake tests/tst_ui_about.cpp
git -C <worktree> commit -m "Version, « À propos », notices des composants tiers et contrôle des modules Qt livrés" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

<a id="tache-l1-t12"></a>
### Tâche L1-T12 — `ui` + `app` : coquille QML (ligne supérieure, palette, zone de page, F5, commandes)

**Objectif.** La fenêtre de la décision 4 (F-001, F-002, F-003, F-020, F-021, F-022, F-060, F-120, F-121, F-122,
F-125, F-128, F-133, F-134, F-137, F-138, F-200, F-220, F-630, F-720 en partie, F-760) :
- **ligne supérieure** : menus de la disposition (Fichier, Affichage, Outils, Aide en lot 1 ; Éditer et Insérer,
  vides, ne s'affichent pas), indicateur « n / N », échelle (« Ajuster », 25 % à 800 %), bouton plein écran ;
- **palette à droite** : « Menu principal » (mêmes menus en surgissant, F-122) et « Outils » (menu Outils en
  surgissant, F-128), page précédente / suivante, 16 pastilles (la courante encadrée), 4 préréglages d'épaisseur,
  réglette 1–100, échantillon de largeur, bouton « Stylo » coché (outil de départ, F-200) ;
- **zone de page** : vue, cache, encre (noms d'objet inchangés), barres de défilement quand la page dépasse,
  indicateur de largeur sous le pointeur (cercle de la taille et de la couleur du trait, F-138) ;
- **pas de barre d'état** ; F5 bascule le plein écran ; tous les boutons, menus et raccourcis passent par
  `Commands.trigger` ; `runCommand(id, argument)` sur la fenêtre pour piloter sans souris (DA8) ; tant qu'une
  fenêtre surgissante est ouverte, la page ne prend pas le stylet (`PageView.inputBlocked`) ;
- la page de 300 objets n'est plus chargée d'office : seulement avec `StartupOptions::generatedPage` (banc, tests).
  Ce n'est pas écrit au cadrage : c'est un défaut prudent de ce plan (un tableau blanc s'ouvre sur une page
  vierge ; la page générée est un outil de mesure, DA7), consigné dans la section « Écarts au cadrage ».

**Fichiers.**
- Créer : `ui/qml/TopBar.qml`, `ui/qml/ToolPalette.qml`, `ui/qml/PageArea.qml`, `ui/qml/CommandButton.qml`,
  `ui/qml/CommandMenu.qml`, `ui/qml/ColorSwatches.qml`, `ui/qml/WidthPicker.qml`, `tests/tst_ui_shell.cpp`
- Modifier : `ui/qml/Main.qml`, `ui/CMakeLists.txt`, `app/startup.h`, `app/startup.cpp`, `bench/app/main.cpp`,
  `tests/ui.cmake`, `tests/tst_ui.cpp`, `tests/tst_ui_drawing.cpp`, `tests/tst_bench_verdict.cpp`

**Changements voulus de tests existants (raison écrite).** `tst_ui` attendait 300 objets au démarrage : le
démarrage montre désormais une page vierge (défaut de ce plan, « Écarts au cadrage ») ; la page générée reste vérifiée sous l'option
`generatedPage`. `tst_ui_drawing` et `tst_bench_verdict` sondent des pixels de la page générée : ils passent
`generatedPage = true` (même page qu'avant).

**Test et commande bornée.** `tst_ui_shell` (8 fonctions) ; `tst_ui` (1 fonction ajoutée, 1 ajustée). Rouge :
commande ciblée, motif `tst_ui_shell` → `error` puis échecs (`topBar` introuvable). Vert : forme bornée, N ≥ 40,
`TESTS OK`, lignes `Passed` de `tst_ui_shell`, `tst_ui`, `tst_ui_drawing` ; banc sans écran (résultat) ;
démarrage sans écran sans avertissement QML (étape 13).

**Interdits.** Ne pas renommer les objets `pageView`, `page`, `fixedPage`, `pageCache`, `ink` (banc). Ne pas
appeler `FixedPage.loadGeneratedPage()` ni `setPage` en QML (seul `DocumentController` le fait, L1-T9). Aucun
libellé, icône ou terme de Promethean (N1-12). Aucune ombre ni flou (règle de fluidité 4). Pas de barre d'état.

**Terminé quand.** Preuve verte, JSON du banc joint, démarrage sans écran sans avertissement QML (étape 13) ;
commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire le test.** Créer `tests/tst_ui_shell.cpp` :

```cpp
#include "startup.h"

#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/pageview.h>

#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTabletEvent>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

struct Shell
{
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
    render::FixedPageItem* fixed = nullptr;
};

bool ready(Shell& s)
{
    return QTest::qWaitFor([&] { return s.fixed->isReady() && s.view->cacheReady(); }, 10000);
}

bool start(Shell& s)
{
    s.window = loadMainWindow(s.engine, StartupOptions{});
    if (!s.window || !QTest::qWaitForWindowExposed(s.window)) {
        return false;
    }
    s.view = s.window->findChild<ui::PageView*>(QStringLiteral("pageView"));
    s.fixed = s.window->findChild<render::FixedPageItem*>(QStringLiteral("fixedPage"));
    return s.view && s.fixed && ready(s);
}

// Les délégués d'un Repeater n'ont pas toujours la fenêtre pour parent QObject : on suit l'arbre des éléments.
QQuickItem* findItem(QQuickItem* root, const QString& name)
{
    if (!root) {
        return nullptr;
    }
    if (root->objectName() == name) {
        return root;
    }
    const QList<QQuickItem*> children = root->childItems();
    for (QQuickItem* child : children) {
        if (QQuickItem* found = findItem(child, name)) {
            return found;
        }
    }
    return nullptr;
}

QQuickItem* item(Shell& s, const char* name)
{
    return findItem(s.window->contentItem(), QString::fromLatin1(name));
}

bool run(Shell& s, const QString& id, const QVariant& argument = QVariant())
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(s.window, "runCommand", Q_RETURN_ARG(QVariant, result),
                                                   Q_ARG(QVariant, QVariant(id)), Q_ARG(QVariant, argument));
    return invoked && result.toBool();
}

QString indicator(Shell& s)
{
    QQuickItem* label = item(s, "pageIndicator");
    return label ? label->property("text").toString() : QString();
}

QColor pixelAtPage(Shell& s, QPointF page)
{
    const QImage image = s.window->grabWindow();
    const QPointF scene = s.view->pageToScene(page) * s.window->effectiveDevicePixelRatio();
    return image.pixelColor(scene.toPoint());
}

bool near(const QColor& a, const QColor& b, int tolerance)
{
    return qAbs(a.red() - b.red()) <= tolerance && qAbs(a.green() - b.green()) <= tolerance &&
           qAbs(a.blue() - b.blue()) <= tolerance;
}

} // namespace

// Coquille de l'interface (décision 4 du cadrage livraison-complete ; S2).
class TstUiShell : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QPointingDevice> m_stylus;

    void post(Shell& s, QEvent::Type type, QPointF scene, Qt::MouseButtons buttons)
    {
        QCoreApplication::postEvent(
            s.window, new QTabletEvent(type, m_stylus.get(), scene, s.window->mapToGlobal(scene),
                                       buttons ? 0.5 : 0.0, 0, 0, 0, 0, 0, Qt::NoModifier, Qt::LeftButton,
                                       buttons));
    }

private slots:
    void initTestCase()
    {
        m_stylus = std::make_unique<QPointingDevice>(
            QStringLiteral("tst-ui-shell-stylus"), 5170, QInputDevice::DeviceType::Stylus,
            QPointingDevice::PointerType::Pen,
            QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
    }

    void shellShowsTopBarPaletteAndPage()
    {
        Shell s;
        QVERIFY(start(s));
        for (const char* name : {"topBar", "toolPalette", "pageArea", "pageIndicator", "zoomBox", "colorSwatches",
                                 "widthPicker", "menu-file", "menu-view", "menu-tools", "menu-help",
                                 "button-page.next", "button-page.previous", "button-tool.pen",
                                 "button-menu.main", "button-menu.tools", "button-view.fullscreen"}) {
            QQuickItem* found = item(s, name);
            QVERIFY2(found && found->isVisible(), name);
        }
        QVERIFY(!item(s, "menu-edit"));   // menu vide en lot 1 : pas affiché
        QVERIFY(!item(s, "menu-insert")); // idem
        QCOMPARE(indicator(s), QStringLiteral("1 / 1"));
        QCOMPARE(s.fixed->objectCount(), 0);
        // Palette à droite de la page, ligne supérieure au-dessus (F-001, F-002) ; pas de barre d'état (F-003).
        QQuickItem* palette = item(s, "toolPalette");
        QQuickItem* bar = item(s, "topBar");
        QVERIFY(palette->mapToScene(QPointF(0, 0)).x() >= s.view->mapToScene(QPointF(s.view->width(), 0)).x() - 1);
        QVERIFY(bar->mapToScene(QPointF(0, bar->height())).y() <= s.view->mapToScene(QPointF(0, 0)).y() + 1);
        QVERIFY(s.view->mapToScene(QPointF(0, s.view->height())).y() >= s.window->height() - 1);
        QVERIFY(item(s, "button-tool.pen")->property("checked").toBool()); // F-200
    }

    void pageCommandsAddPageAndMoveTheIndicator()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(ready(s));
        QCOMPARE(indicator(s), QStringLiteral("2 / 2"));
        QVERIFY(run(s, QStringLiteral("page.previous")));
        QVERIFY(ready(s));
        QCOMPARE(indicator(s), QStringLiteral("1 / 2"));
    }

    void penCommandsSetColorAndWidth()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(run(s, QStringLiteral("pen.color"), QStringLiteral("#E53935")));
        QCOMPARE(s.view->penColor(), QColor(QStringLiteral("#E53935")));
        QVERIFY(run(s, QStringLiteral("pen.width"), 8));
        QCOMPARE(s.view->penWidth(), 8.0);
        QVERIFY(!run(s, QStringLiteral("pen.color"), QStringLiteral("rouge")));
        QCOMPARE(s.view->penColor(), QColor(QStringLiteral("#E53935")));
    }

    void redStrokeSurvivesPageRoundTrip()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(run(s, QStringLiteral("pen.color"), QStringLiteral("#E53935")));
        QVERIFY(run(s, QStringLiteral("pen.width"), 8));
        const QPoint a = s.view->pageToScene(QPointF(300, 400)).toPoint();
        const QPoint b = s.view->pageToScene(QPointF(700, 400)).toPoint();
        QTest::mousePress(s.window, Qt::LeftButton, Qt::NoModifier, a);
        for (int i = 1; i <= 20; ++i) {
            QTest::mouseMove(s.window, a + (b - a) * i / 20);
        }
        QTest::mouseRelease(s.window, Qt::LeftButton, Qt::NoModifier, b);
        QTRY_COMPARE_WITH_TIMEOUT(s.fixed->objectCount(), 1, 3000);
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(ready(s));
        QCOMPARE(s.fixed->objectCount(), 0);
        QVERIFY(run(s, QStringLiteral("page.previous")));
        QVERIFY(ready(s));
        QCOMPARE(s.fixed->objectCount(), 1);
        QVERIFY(near(pixelAtPage(s, QPointF(500, 400)), QColor(QStringLiteral("#E53935")), 60));
    }

    void f5TogglesFullScreen()
    {
        Shell s;
        QVERIFY(start(s));
        QTest::keyClick(s.window, Qt::Key_F5);
        QTRY_VERIFY(s.window->property("fullScreen").toBool());
        QTest::keyClick(s.window, Qt::Key_F5);
        QTRY_VERIFY(!s.window->property("fullScreen").toBool());
    }

    void swatchTapChoosesThePenColor()
    {
        Shell s;
        QVERIFY(start(s));
        QQuickItem* swatch = item(s, "swatch-5");
        QVERIFY(swatch);
        const QPointF center = swatch->mapToScene(QPointF(swatch->width() / 2, swatch->height() / 2));
        QTest::mouseClick(s.window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
        QTRY_COMPARE(s.view->penColor(), QColor(QStringLiteral("#1E88E5")));
    }

    void stylusOnThePaletteDoesNotDraw()
    {
        Shell s;
        QVERIFY(start(s));
        QQuickItem* palette = item(s, "toolPalette");
        const QPointF a = palette->mapToScene(QPointF(palette->width() / 2, palette->height() - 12));
        post(s, QEvent::TabletPress, a, Qt::LeftButton);
        post(s, QEvent::TabletMove, a - QPointF(300, 0), Qt::LeftButton);
        post(s, QEvent::TabletRelease, a - QPointF(300, 0), Qt::NoButton);
        QTest::qWait(150);
        QCOMPARE(s.fixed->objectCount(), 0);
    }

    void aboutCommandOpensTheDialogAndBlocksThePage()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(run(s, QStringLiteral("help.about")));
        QObject* dialog = s.window->findChild<QObject*>(QStringLiteral("aboutDialog"));
        QVERIFY(dialog);
        QTRY_VERIFY(dialog->property("opened").toBool());
        QVERIFY(s.view->inputBlocked());
        QVERIFY(QMetaObject::invokeMethod(dialog, "close"));
        QTRY_VERIFY(!s.view->inputBlocked());
    }
};

QTEST_MAIN(TstUiShell)
#include "tst_ui_shell.moc"
```

Ajouter à la fin de `tests/ui.cmake` :

```cmake
actcraft_add_test(tst_ui_shell
    SOURCES tst_ui_shell.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_shell PRIVATE ${PROJECT_SOURCE_DIR}/app)
```

- [ ] **Étape 2 : le voir échouer.** Commande ciblée, motif `tst_ui_shell`. Attendu : le test compile et échoue
(`topBar` introuvable, `runCommand` absent) ; `tests failed`.

- [ ] **Étape 3 : option `generatedPage`.** Dans `app/startup.h`, remplacer la structure par :

```cpp
struct StartupOptions
{
    bool cacheEnabled = true;
    // Banc et tests : la page 1 est la page générée de 300 objets (DA7) ; sinon, page vierge.
    bool generatedPage = false;
};
```
Dans `app/startup.cpp`, remplacer la ligne `engine.setInitialProperties(...)` par :

```cpp
    engine.setInitialProperties({{QStringLiteral("cacheEnabled"), options.cacheEnabled},
                                 {QStringLiteral("generatedPage"), options.generatedPage}});
```
Dans `bench/app/main.cpp`, après `startup.cacheEnabled = options.cacheEnabled;`, ajouter :

```cpp
    startup.generatedPage = true; // le banc mesure la page de 300 objets (DA7)
```
Dans `tests/tst_ui_drawing.cpp`, fonction `start`, après `options.cacheEnabled = cacheEnabled;`, ajouter
`options.generatedPage = true;`. Dans `tests/tst_bench_verdict.cpp`, fonction `start`, remplacer
`app.window = loadMainWindow(app.engine, StartupOptions{});` par :

```cpp
    StartupOptions options;
    options.generatedPage = true; // le banc mesure la page de 300 objets
    app.window = loadMainWindow(app.engine, options);
```

- [ ] **Étape 4 : `tests/tst_ui.cpp`.** Remplacer la classe par :

```cpp
// La fenêtre principale se charge et s'affiche avec le rendu logiciel de secours.
class TstUi : public QObject
{
    Q_OBJECT

private slots:
    void mainWindowLoadsWithSoftwareRenderer()
    {
        QQmlApplicationEngine engine;
        engine.loadFromModule("ActCraft.Ui", "Main");
        QCOMPARE(engine.rootObjects().size(), 1);

        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
        QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        QCOMPARE(window->rendererInterface()->graphicsApi(), QSGRendererInterface::Software);

        auto* page = window->findChild<QQuickItem*>(QStringLiteral("page"));
        QVERIFY(page);
        QCOMPARE(page->width(), 1024.0);
        QCOMPARE(page->height(), 768.0);

        auto* view = window->findChild<actcraft::ui::PageView*>(QStringLiteral("pageView"));
        auto* fixed = window->findChild<actcraft::render::FixedPageItem*>(QStringLiteral("fixedPage"));
        QVERIFY(view && fixed);
        QTRY_VERIFY_WITH_TIMEOUT(fixed->isReady() && view->cacheReady(), 20000);
        QCOMPARE(fixed->objectCount(), 0); // page vierge au démarrage (la page générée sert au banc)
        QVERIFY(window->title().contains(QStringLiteral("logiciel")));
    }

    void generatedPageOptionLoadsThe300Objects()
    {
        QQmlApplicationEngine engine;
        engine.setInitialProperties({{QStringLiteral("generatedPage"), true}});
        engine.loadFromModule("ActCraft.Ui", "Main");
        QCOMPARE(engine.rootObjects().size(), 1);
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
        QVERIFY(window && QTest::qWaitForWindowExposed(window));
        auto* view = window->findChild<actcraft::ui::PageView*>(QStringLiteral("pageView"));
        auto* fixed = window->findChild<actcraft::render::FixedPageItem*>(QStringLiteral("fixedPage"));
        QVERIFY(view && fixed);
        QTRY_VERIFY_WITH_TIMEOUT(fixed->isReady() && view->cacheReady(), 20000);
        QCOMPARE(fixed->objectCount(), 300);
    }
};
```

- [ ] **Étape 5 : `ui/qml/CommandButton.qml`.**

```qml
import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Bouton relié à une commande nommée (DA1) : icône, info-bulle avec raccourci, état actif et coché lus dans le
// registre ; un clic déclenche la commande par son nom.
ToolButton {
    id: button
    required property Commands commands
    required property string commandId
    property var argument: undefined

    objectName: "button-" + commandId
    implicitWidth: Theme.tailleBouton
    implicitHeight: Theme.tailleBouton
    enabled: commands.revision >= 0 && commands.isEnabled(commandId)
    checked: commands.revision >= 0 && commands.isChecked(commandId)
    display: AbstractButton.IconOnly
    icon.source: commands.iconUrl(commandId)
    icon.width: Theme.tailleIcone
    icon.height: Theme.tailleIcone
    icon.color: "transparent" // garder la couleur dessinée de l'icône
    ToolTip.visible: hovered
    ToolTip.delay: 600
    ToolTip.text: commands.shortcut(commandId) === ""
                  ? commands.label(commandId)
                  : qsTr("%1 (%2)").arg(commands.label(commandId)).arg(commands.shortcut(commandId))

    background: Rectangle {
        radius: Theme.rayon
        color: button.checked ? Theme.surfaceActive : button.hovered ? Theme.surfaceSurvol : Theme.surface
    }

    onClicked: {
        if (button.argument === undefined)
            commands.trigger(commandId)
        else
            commands.trigger(commandId, button.argument)
    }
}
```

- [ ] **Étape 6 : `ui/qml/CommandMenu.qml`.**

```qml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Menu construit depuis la disposition (actions/data/layout.json) : une entrée par commande disponible ; libellé,
// raccourci et coche lus dans le registre ; un clic déclenche la commande par son nom.
Menu {
    id: menu
    required property Commands commands
    property var spec: ({ id: "", label: "", items: [] })
    title: spec.label

    function rebuild() {
        while (menu.count > 0)
            menu.removeItem(menu.itemAt(0))
        const items = spec.items || []
        for (let i = 0; i < items.length; ++i) {
            if (items[i].separator)
                menu.addItem(separatorComponent.createObject(null))
            else
                menu.addItem(entryComponent.createObject(null, {
                    commandId: items[i].id,
                    commandCheckable: items[i].checkable === true
                }))
        }
    }

    onSpecChanged: rebuild()
    Component.onCompleted: rebuild()

    Component {
        id: separatorComponent
        MenuSeparator {}
    }

    Component {
        id: entryComponent
        MenuItem {
            id: entry
            property string commandId
            property bool commandCheckable: false
            objectName: "menuItem-" + commandId
            text: menu.commands.label(commandId)
            enabled: menu.commands.revision >= 0 && menu.commands.isEnabled(commandId)
            checkable: commandCheckable
            checked: menu.commands.revision >= 0 && menu.commands.isChecked(commandId)
            // Le clic bascule la coche localement : on rend la main au registre.
            onToggled: checked = Qt.binding(function () {
                return menu.commands.revision >= 0 && menu.commands.isChecked(entry.commandId)
            })
            onTriggered: menu.commands.trigger(commandId)
            contentItem: RowLayout {
                spacing: Theme.espace3
                Label {
                    text: entry.text
                    Layout.fillWidth: true
                    opacity: entry.enabled ? 1 : 0.5
                }
                Label {
                    text: menu.commands.shortcut(entry.commandId)
                    opacity: 0.6
                }
            }
        }
    }
}
```

- [ ] **Étape 7 : `ui/qml/TopBar.qml`.**

```qml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Ligne supérieure (F-002) : menus à gauche ; indicateur « n / N », échelle (F-021) et plein écran à droite. Pas
// de barre d'état ailleurs (F-003).
Rectangle {
    id: bar
    objectName: "topBar"
    required property Commands commands
    required property DocumentController documentController
    required property PageView view
    signal popupOpened()
    signal popupClosed()

    implicitHeight: Theme.hauteurBarre
    color: Theme.barre

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.espace2
        anchors.rightMargin: Theme.espace2
        spacing: Theme.espace1

        Repeater {
            model: bar.commands.menus
            delegate: ToolButton {
                id: menuButton
                required property var modelData
                objectName: "menu-" + modelData.id
                text: modelData.label
                font.pixelSize: Theme.taillePolice
                Layout.fillHeight: true
                contentItem: Label {
                    text: menuButton.text
                    font: menuButton.font
                    color: Theme.texte
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: Theme.rayon
                    color: menuButton.down || menu.opened ? Theme.surfaceActive
                         : menuButton.hovered ? Theme.surfaceSurvol : "transparent"
                }
                onClicked: menu.popup(menuButton, 0, menuButton.height)

                CommandMenu {
                    id: menu
                    commands: bar.commands
                    spec: menuButton.modelData
                    onOpened: bar.popupOpened()
                    onClosed: bar.popupClosed()
                }
            }
        }

        Item {
            Layout.fillWidth: true
        }

        Label {
            id: pageIndicator
            objectName: "pageIndicator"
            text: bar.documentController.pageLabel
            color: Theme.texte
            font.pixelSize: Theme.taillePolice
            Layout.rightMargin: Theme.espace3
        }

        ComboBox {
            id: zoomBox
            objectName: "zoomBox"
            Layout.preferredWidth: 112
            readonly property var percents: [0, 25, 50, 100, 150, 200, 400, 800]
            model: [qsTr("Ajuster"), "25 %", "50 %", "100 %", "150 %", "200 %", "400 %", "800 %"]
            displayText: Math.round(bar.view.zoom * 100) + " %"
            onActivated: index => {
                if (index === 0)
                    bar.commands.trigger("view.fit")
                else
                    bar.commands.trigger("view.zoom", zoomBox.percents[index])
            }
            Connections {
                target: zoomBox.popup
                function onOpened() { bar.popupOpened() }
                function onClosed() { bar.popupClosed() }
            }
        }

        CommandButton {
            commands: bar.commands
            commandId: "view.fullscreen"
        }
    }
}
```

- [ ] **Étape 8 : `ui/qml/ColorSwatches.qml` et `ui/qml/WidthPicker.qml`.**

```qml
import QtQuick
import ActCraft.Ui

// Pastilles de couleur (F-133, F-134) : 4 colonnes ; un toucher choisit la couleur du stylo par la commande
// « pen.color » ; la couleur courante est encadrée.
Grid {
    id: grid
    objectName: "colorSwatches"
    required property Commands commands
    required property PageView view
    columns: 4
    spacing: Theme.espace1

    Repeater {
        model: grid.commands.colors
        delegate: Rectangle {
            id: swatch
            required property string modelData
            required property int index
            readonly property bool current: Qt.colorEqual(grid.view.penColor, modelData)
            objectName: "swatch-" + index
            width: Theme.taillePastille
            height: Theme.taillePastille
            radius: 3
            color: modelData
            border.width: current ? 2 : 1
            border.color: current ? Theme.cadreSelection : Theme.separateur
            TapHandler {
                onTapped: grid.commands.trigger("pen.color", swatch.modelData)
            }
        }
    }
}
```

```qml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Épaisseur du stylo (F-137, F-138) : quatre préréglages ronds, réglette de 1 à 100, échantillon de la largeur
// et de la couleur courantes. Tout passe par la commande « pen.width ».
ColumnLayout {
    id: picker
    objectName: "widthPicker"
    required property Commands commands
    required property PageView view
    spacing: Theme.espace1

    Row {
        spacing: Theme.espace1
        Repeater {
            model: picker.commands.penWidths
            delegate: Rectangle {
                id: preset
                required property real modelData
                required property int index
                objectName: "widthPreset-" + index
                width: 21
                height: 21
                radius: 4
                color: picker.view.penWidth === modelData ? Theme.surfaceActive : Theme.surface
                Rectangle {
                    anchors.centerIn: parent
                    width: Math.min(17, 3 + preset.modelData * 1.5)
                    height: width
                    radius: width / 2
                    color: Theme.texte
                }
                TapHandler {
                    onTapped: picker.commands.trigger("pen.width", preset.modelData)
                }
            }
        }
    }

    Slider {
        id: slider
        objectName: "widthSlider"
        Layout.fillWidth: true
        from: 1
        to: 100
        stepSize: 1
        value: picker.view.penWidth
        onMoved: picker.commands.trigger("pen.width", value)
        // Le glissement casse la liaison : on la rend au stylo au relâché.
        onPressedChanged: if (!pressed) value = Qt.binding(() => picker.view.penWidth)
    }

    Item {
        Layout.fillWidth: true
        implicitHeight: 40
        Rectangle {
            objectName: "widthIndicatorSample"
            anchors.centerIn: parent
            width: Math.min(36, Math.max(2, picker.view.penWidth))
            height: width
            radius: width / 2
            color: picker.view.penColor
            border.width: 1
            border.color: Theme.separateur
        }
    }
}
```

- [ ] **Étape 9 : `ui/qml/ToolPalette.qml`.**

```qml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Palette d'outils principale, ancrée à droite (F-120, F-121) : groupes lus dans la disposition ; « couleurs » et
// « epaisseur » sont dessinés ici, les autres groupes sont des boutons de commande sur deux colonnes.
Rectangle {
    id: paletteRoot
    objectName: "toolPalette"
    required property Commands commands
    required property PageView view
    implicitWidth: Theme.largeurPalette
    color: Theme.palette

    Flickable {
        anchors.fill: parent
        contentHeight: column.implicitHeight + 2 * Theme.espace2
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: column
            x: Theme.espace2
            y: Theme.espace2
            width: paletteRoot.width - 2 * Theme.espace2
            spacing: Theme.espace2

            Repeater {
                model: paletteRoot.commands.palette
                delegate: ColumnLayout {
                    id: groupItem
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: Theme.espace2

                    Loader {
                        Layout.fillWidth: true
                        sourceComponent: groupItem.modelData.group === "couleurs" ? swatchesComponent
                                       : groupItem.modelData.group === "epaisseur" ? widthComponent
                                       : buttonsComponent
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: Theme.separateur
                    }

                    Component {
                        id: swatchesComponent
                        ColorSwatches {
                            commands: paletteRoot.commands
                            view: paletteRoot.view
                        }
                    }
                    Component {
                        id: widthComponent
                        WidthPicker {
                            commands: paletteRoot.commands
                            view: paletteRoot.view
                        }
                    }
                    Component {
                        id: buttonsComponent
                        Grid {
                            columns: 2
                            spacing: Theme.espace1
                            Repeater {
                                model: groupItem.modelData.items
                                delegate: CommandButton {
                                    required property var modelData
                                    commands: paletteRoot.commands
                                    commandId: modelData.id
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
```

- [ ] **Étape 10 : `ui/qml/PageArea.qml`.**

```qml
import QtQuick
import QtQuick.Controls.Basic
import ActCraft.Ui

// Zone de la page (F-020, F-022) : vue zoomable, cache du contenu fixe, encre ; barres de défilement quand la
// page dépasse ; indicateur de largeur du stylo sous le pointeur (F-138). Noms d'objet lus par le banc : inchangés.
Item {
    id: area
    objectName: "pageArea"
    property alias view: view
    property alias fixedPage: fixedPageItem
    property bool cacheEnabled: true
    property bool inputBlocked: false
    clip: true

    PageView {
        id: view
        objectName: "pageView"
        anchors.fill: parent
        cacheEnabled: area.cacheEnabled
        inputBlocked: area.inputBlocked
        content: pageContent
        fixedPage: fixedPageItem
        ink: inkLayer
        // Branché dès la création de la vue (un bloc Connections ne l'est qu'en fin de composant).
        onCacheRefreshRequested: cache.scheduleUpdate()

        // Page au format d'un paperboard (1024 × 768), transformée par la vue.
        Item {
            id: pageContent
            objectName: "page"
            width: 1024
            height: 768

            FixedPage {
                id: fixedPageItem
                objectName: "fixedPage"
                anchors.fill: parent
            }

            // Cache du contenu fixe : redessiné seulement sur demande de la vue.
            ShaderEffectSource {
                id: cache
                objectName: "pageCache"
                anchors.fill: parent
                visible: view.cacheEnabled
                sourceItem: view.cacheEnabled ? fixedPageItem : null
                hideSource: view.cacheEnabled
                live: false
                textureSize: view.cacheTextureSize
                onScheduledUpdateCompleted: view.notifyCacheUpdated()
            }

            InkLayer {
                id: inkLayer
                objectName: "ink"
                anchors.fill: parent
            }
        }
    }

    // Indicateur de largeur (F-138) : cercle de la taille et de la couleur du trait sous le pointeur.
    HoverHandler {
        id: hover
    }
    Rectangle {
        objectName: "widthIndicator"
        visible: hover.hovered && !area.inputBlocked
        width: Math.max(4, view.penWidth * view.zoom)
        height: width
        radius: width / 2
        x: hover.point.position.x - width / 2
        y: hover.point.position.y - height / 2
        color: "transparent"
        border.width: 1
        border.color: view.penColor
    }

    ScrollBar {
        id: horizontalBar
        objectName: "horizontalScrollBar"
        orientation: Qt.Horizontal
        anchors.left: parent.left
        anchors.right: verticalBar.left
        anchors.bottom: parent.bottom
        policy: ScrollBar.AlwaysOn
        visible: size < 1
        size: view.scrollSizeX
        position: view.scrollPositionX
        onPositionChanged: if (pressed) view.scrollTo(position, view.scrollPositionY)
        onPressedChanged: if (!pressed) position = Qt.binding(() => view.scrollPositionX)
    }

    ScrollBar {
        id: verticalBar
        objectName: "verticalScrollBar"
        orientation: Qt.Vertical
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: horizontalBar.top
        policy: ScrollBar.AlwaysOn
        visible: size < 1
        size: view.scrollSizeY
        position: view.scrollPositionY
        onPositionChanged: if (pressed) view.scrollTo(view.scrollPositionX, position)
        onPressedChanged: if (!pressed) position = Qt.binding(() => view.scrollPositionY)
    }
}
```

- [ ] **Étape 11 : `ui/qml/Main.qml`.** Remplacer le fichier entier par :

```qml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import ActCraft.Ui

// Fenêtre principale (décision 4 du cadrage livraison-complete ; F-001 à F-003) : ligne supérieure, page au
// centre, palette à droite, pas de barre d'état. Tout bouton, menu ou raccourci déclenche une commande nommée
// (DA1) ; les gestionnaires des commandes du lot 1 sont posés ici.
Window {
    id: root
    width: 1280
    height: 800
    visible: true
    // Rendu effectif dans le titre (décision 19 du prototype) : aucun texte dans la scène.
    title: qsTr("ActCraft — rendu : %1").arg(root.apiName(GraphicsInfo.api))
    color: Theme.fond

    // Fixées avant le chargement par l'application, le banc ou les tests (app/startup.cpp).
    property bool cacheEnabled: true
    property bool generatedPage: false
    // État de l'interface, lu par les tests et par actcraft-captures.
    property bool fullScreen: false
    property int openPopups: 0

    Commands {
        id: appCommands
        objectName: "commands"
    }

    DocumentController {
        id: appDocument
        objectName: "documentController"
        fixedPage: pageArea.fixedPage
    }

    // Pilotage sans souris (actcraft-captures, tests) : même chemin que les boutons.
    function runCommand(id, argument) {
        return appCommands.trigger(id, argument)
    }

    function setFullScreen(on) {
        root.fullScreen = on
        if (on)
            root.showFullScreen()
        else
            root.showNormal()
        appCommands.setChecked("view.fullscreen", on)
    }

    function popupOpened() {
        root.openPopups += 1
    }

    function popupClosed() {
        root.openPopups = Math.max(0, root.openPopups - 1)
    }

    function menuSpec(id) {
        const menus = appCommands.menus
        for (let i = 0; i < menus.length; ++i) {
            if (menus[i].id === id)
                return menus[i]
        }
        return { id: id, label: "", items: [] }
    }

    TopBar {
        id: topBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        commands: appCommands
        documentController: appDocument
        view: pageArea.view
        onPopupOpened: root.popupOpened()
        onPopupClosed: root.popupClosed()
    }

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: topBar.bottom
        anchors.bottom: parent.bottom
        spacing: 0

        PageArea {
            id: pageArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            cacheEnabled: root.cacheEnabled
            inputBlocked: root.openPopups > 0
        }

        ToolPalette {
            id: toolPalette
            Layout.fillHeight: true
            Layout.preferredWidth: Theme.largeurPalette
            commands: appCommands
            view: pageArea.view
        }
    }

    // « Menu principal » (F-122) : les mêmes menus que la ligne supérieure.
    Menu {
        id: mainMenu
        objectName: "mainMenu"
        onOpened: root.popupOpened()
        onClosed: root.popupClosed()

        function rebuild() {
            while (mainMenu.count > 0)
                mainMenu.removeItem(mainMenu.itemAt(0))
            const menus = appCommands.menus
            for (let i = 0; i < menus.length; ++i)
                mainMenu.addMenu(subMenuComponent.createObject(null, { commands: appCommands, spec: menus[i] }))
        }
    }

    Component {
        id: subMenuComponent
        CommandMenu {}
    }

    // « Outils » (F-128) : le menu Outils en menu surgissant.
    CommandMenu {
        id: toolsMenu
        objectName: "toolsMenu"
        commands: appCommands
        spec: root.menuSpec("tools")
        onOpened: root.popupOpened()
        onClosed: root.popupClosed()
    }

    AboutDialog {
        id: aboutDialog
        onOpened: root.popupOpened()
        onClosed: root.popupClosed()
    }

    // Raccourcis de la disposition (commande, ou remplacement du profil).
    Instantiator {
        model: appCommands.shortcuts
        delegate: Shortcut {
            required property var modelData
            sequence: modelData.sequence
            context: Qt.ApplicationShortcut
            onActivated: appCommands.trigger(modelData.id)
        }
    }

    Connections {
        target: appCommands
        function onLayoutChanged() {
            mainMenu.rebuild()
        }
    }

    Component.onCompleted: {
        appCommands.setHandler("file.quit", function () { Qt.quit() })
        appCommands.setHandler("view.fullscreen", function () { root.setFullScreen(!root.fullScreen) })
        appCommands.setHandler("view.fit", function () { pageArea.view.fitPage() })
        appCommands.setHandler("view.zoom", function (percent) { pageArea.view.zoom = percent / 100 })
        appCommands.setHandler("page.next", function () { appDocument.nextPage() })
        appCommands.setHandler("page.previous", function () { appDocument.previousPage() })
        appCommands.setHandler("tool.pen", function () { appCommands.setChecked("tool.pen", true) })
        appCommands.setHandler("pen.color", function (color) { pageArea.view.penColor = color })
        appCommands.setHandler("pen.width", function (width) { pageArea.view.penWidth = width })
        appCommands.setHandler("menu.main", function () { mainMenu.popup() })
        appCommands.setHandler("menu.tools", function () { toolsMenu.popup() })
        appCommands.setHandler("help.about", function () { aboutDialog.open() })
        appCommands.setHandler("help.website", function () { Qt.openUrlExternally(AppInfo.projectUrl) })
        appCommands.setChecked("tool.pen", true) // F-200 : le stylo est l'outil de départ
        mainMenu.rebuild()
        if (root.generatedPage)
            appDocument.loadGeneratedFirstPage()
    }

    function apiName(api) {
        switch (api) {
        case GraphicsInfo.Software: return "logiciel"
        case GraphicsInfo.Direct3D11: return "Direct3D 11"
        case GraphicsInfo.Direct3D12: return "Direct3D 12"
        case GraphicsInfo.Metal: return "Metal"
        case GraphicsInfo.Vulkan: return "Vulkan"
        case GraphicsInfo.OpenGL: return "OpenGL"
        default: return "inconnu"
        }
    }
}
```

Dans `ui/CMakeLists.txt`, `QML_FILES` devient :

```cmake
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/AboutDialog.qml
        qml/TopBar.qml
        qml/ToolPalette.qml
        qml/PageArea.qml
        qml/CommandButton.qml
        qml/CommandMenu.qml
        qml/ColorSwatches.qml
        qml/WidthPicker.qml
```

- [ ] **Étape 12 : le voir passer.** Commande ciblée, motif `tst_ui|tst_bench_verdict`. Attendu : `0 tests failed`
(`tst_ui`, `tst_ui_drawing`, `tst_ui_shell`, `tst_ui_theme`, `tst_ui_document`, `tst_ui_commands`, `tst_ui_about`,
`tst_bench_verdict`). Si un `QML … warning` signale une liaison en boucle ou une propriété requise non fixée, la
corriger avant de continuer (aucun avertissement QML nouveau dans la sortie de `tst_ui_shell`).

- [ ] **Étape 13 : démarrage de l'application, sans écran.** `actcraft-captures` (L1-T13, L1-T15) produira les
captures de la coquille ; ici, vérifier que l'exécutable démarre et se ferme seul sans avertissement QML :

```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -Command ". ./scripts/dev-env.ps1; `$env:QT_QPA_PLATFORM='offscreen'; ./build/debug/actcraft.exe --rendu logiciel --quitter-apres 3000 *> .tmp/demarrage.txt; Write-Output `"code `$LASTEXITCODE`""; Select-String -Path .tmp/demarrage.txt -Pattern 'qml|warning' | Select-Object -First 5
```
Attendu : `code 0` et aucune ligne `qml` ni `warning` (sinon la corriger et recommencer).

- [ ] **Étape 14 : format, preuve, banc.** `pwsh -NoProfile -File ./scripts/format.ps1` ; forme bornée (N ≥ 40,
`TESTS OK`, lignes `Passed` de `tst_ui_shell`, `tst_ui`, `tst_ui_drawing`) ; preuve que les fonctions nouvelles ont
tourné :

```powershell
Set-Location <worktree>; pwsh -NoProfile -Command ". ./scripts/dev-env.ps1; `$env:QT_QPA_PLATFORM='offscreen'; `$env:QT_QUICK_BACKEND='software'; ./build/debug/tst_ui_shell.exe *> .tmp/fonctions-shell.txt; ./build/debug/tst_ui.exe *> .tmp/fonctions-ui.txt"; Select-String -Path .tmp/fonctions-shell.txt, .tmp/fonctions-ui.txt -Pattern '^PASS .*(TstUiShell::(?!initTestCase|cleanupTestCase)|generatedPageOptionLoadsThe300Objects)|^FAIL!|^Totals' | ForEach-Object { $_.Line }
```
Attendu : 8 lignes `PASS   : TstUiShell::…`, une ligne `PASS   : TstUi::generatedPageOptionLoadsThe300Objects()`,
aucune `FAIL!`, deux lignes `Totals: … 0 failed` ; banc sans écran (résultat ; un
`MESURE INVALIDE` ici signale que le banc ne trouve plus la page : le dire au rapport, L1-T14 le traite).

- [ ] **Étape 15 : commit.**

```powershell
Set-Location <worktree>; git add ui/CMakeLists.txt ui/qml/Main.qml ui/qml/TopBar.qml ui/qml/ToolPalette.qml ui/qml/PageArea.qml ui/qml/CommandButton.qml ui/qml/CommandMenu.qml ui/qml/ColorSwatches.qml ui/qml/WidthPicker.qml app/startup.h app/startup.cpp bench/app/main.cpp tests/ui.cmake tests/tst_ui.cpp tests/tst_ui_drawing.cpp tests/tst_bench_verdict.cpp tests/tst_ui_shell.cpp
git -C <worktree> commit -m "ui : coquille de l'interface (ligne supérieure, palette, zone de page, plein écran)" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

<a id="tache-l1-t13"></a>
### Tâche L1-T13 — `bench` : `actcraft-captures`, joueur de scénarios d'interface (DA8)

**Objectif.** DA8 et décision 36 : un exécutable `actcraft-captures` (outil de preuve, jamais livré) charge la vraie
interface (`loadMainWindow`), joue un scénario JSON `actcraft-scenario/1` et écrit une ligne de rapport par étape :
commandes nommées appelées par la fonction QML `runCommand` (même chemin que les boutons ; jamais de dialogue
natif, coffre 0084), traits au stylet par événements tablette postés, touches par `QTest::keySequence`, attentes,
attente de page prête, captures PNG (`grabWindow`) avec `index.md`, contrôles de pixels (réutilise `sampleColor`,
`colorNear`, `sceneToImage` du banc), lectures de propriétés (`LU objet.propriété=valeur OK|ECART`). Options :
`--scenario <fichier.json>`, `--rendu`, `--sortie <dossier>`, `--taille LxH|plein-ecran`, `--lire
objet.propriété=valeur` (répétable, lu après le scénario). Dernière ligne et code : `SCENARIO OK` (0),
`SCENARIO EN ECHEC` (1), `SCENARIO INVALIDE` (2).

**Points délicats (concepteur-critique).** (1) Durée de vie : Qt Quick garde le périphérique synthétique du stylet
jusqu'à la destruction de la fenêtre ; le joueur (qui le possède) doit être déclaré **avant** le moteur QML, donc
détruit après la fenêtre — comme le `Runner` du banc (`bench/include/actcraft/bench/runner.h`). (2) Un contrôle de
pixel lit l'écran **après** que la page et le cache sont prêts : le scénario place `waitReady` avant ; le joueur ne
le devine pas. (3) Une étape en échec arrête le scénario (rapport lisible, pas d'échec en cascade) ; les lectures
`--lire` sont toutes faites ensuite.

**Fichiers.**
- Créer : `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`, `bench/captures/main.cpp`,
  `tests/tst_bench_uiscenario.cpp`, `tests/data/uiscenario/fenetre.qml`
- Modifier : `bench/CMakeLists.txt`, `tests/bench.cmake`

**Test et commande bornée.** `tst_bench_uiscenario` (5 fonctions, une avec données) ; le jeu complet est essayé sur
une fenêtre d'essai qui contient une vraie `PageView`. Rouge : commande ciblée, motif `tst_bench_uiscenario` →
`error`. Vert : forme bornée, N ≥ 33, `TESTS OK`, `tst_bench_uiscenario .*Passed`.

**Interdits.** Pas de souris réelle ni d'injection native (souris de Jérôme jamais pilotée). Ne pas modifier
`runner.*`, `injector.*` (le banc mesuré). Sorties sous `.tmp/` ou `build/` seulement. L'exécutable n'entre dans
aucun paquet.

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : fenêtre d'essai.** Créer `tests/data/uiscenario/fenetre.qml` :

```qml
import QtQuick
import ActCraft.Ui

// Fenêtre d'essai du joueur de scénarios (tst_bench_uiscenario) : runCommand, raccourci F5, page vierge réelle.
Window {
    id: root
    width: 800
    height: 600
    visible: true
    property string dernier: ""
    property int compteur: 0

    function runCommand(id, argument) {
        root.dernier = argument === undefined ? id : id + ":" + argument
        root.compteur += 1
        return true
    }

    Shortcut {
        sequence: "F5"
        context: Qt.ApplicationShortcut
        onActivated: root.compteur += 10
    }

    DocumentController {
        fixedPage: fixed
    }

    PageView {
        id: view
        objectName: "pageView"
        anchors.fill: parent
        cacheEnabled: false
        content: page
        fixedPage: fixed
        ink: ink

        Item {
            id: page
            width: 1024
            height: 768

            FixedPage {
                id: fixed
                objectName: "fixedPage"
                anchors.fill: parent
            }

            InkLayer {
                id: ink
                anchors.fill: parent
            }
        }
    }
}
```

- [ ] **Étape 2 : écrire le test.** Créer `tests/tst_bench_uiscenario.cpp` :

```cpp
#include <actcraft/bench/uiscenario.h>

#include <QFile>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft::bench;

namespace {

const QByteArray kAllSteps = R"({
  "format": "actcraft-scenario/1",
  "name": "essai",
  "description": "Toutes les sortes d'étapes.",
  "steps": [
    {"command": "essai.commande", "argument": 3},
    {"command": "sans.argument"},
    {"stroke": [[100, 100], [300, 100]], "pressure": 1.0},
    {"key": "F5"},
    {"wait": 10},
    {"waitReady": true},
    {"capture": "fin"},
    {"checkPixel": "trait", "at": [200, 100], "color": "#000000", "tolerance": 60},
    {"expect": "window.compteur", "value": 12}
  ]
})";

} // namespace

// Joueur de scénarios d'interface (DA8).
class TstBenchUiScenario : public QObject
{
    Q_OBJECT

private slots:
    void parsesEveryStepKind()
    {
        UiScenario s;
        QString error;
        QVERIFY2(parseUiScenario(kAllSteps, s, error), qPrintable(error));
        QCOMPARE(s.name, QStringLiteral("essai"));
        QCOMPARE(s.steps.size(), std::size_t{9});
        QVERIFY(s.steps[0].kind == UiStepKind::Command);
        QCOMPARE(s.steps[0].argument.toDouble(), 3.0);
        QVERIFY(!s.steps[1].argument.isValid());
        QVERIFY(s.steps[2].kind == UiStepKind::Stroke);
        QCOMPARE(s.steps[2].points.size(), std::size_t{2});
        QCOMPARE(s.steps[2].pressure, 1.0);
        QVERIFY(s.steps[3].kind == UiStepKind::Key);
        QCOMPARE(s.steps[4].ms, 10);
        QVERIFY(s.steps[5].kind == UiStepKind::WaitReady);
        QCOMPARE(s.steps[6].name, QStringLiteral("fin"));
        QCOMPARE(s.steps[7].color, QColor(Qt::black));
        QCOMPARE(s.steps[7].tolerance, 60);
        QCOMPARE(s.steps[8].target, QStringLiteral("window"));
        QCOMPARE(s.steps[8].property, QStringLiteral("compteur"));
        QCOMPARE(s.steps[8].value, QStringLiteral("12"));
    }

    void rejectsBadScenarios_data()
    {
        QTest::addColumn<QByteArray>("json");
        QTest::addColumn<QString>("expected");
        QTest::newRow("format") << QByteArray(R"({"format":"autre/1","name":"x","steps":[{"wait":1}]})")
                                << QStringLiteral("format");
        QTest::newRow("vide") << QByteArray(R"({"format":"actcraft-scenario/1","name":"x","steps":[]})")
                              << QStringLiteral("aucune étape");
        QTest::newRow("inconnue")
            << QByteArray(R"({"format":"actcraft-scenario/1","name":"x","steps":[{"danse":1}]})")
            << QStringLiteral("étape 1");
        QTest::newRow("trait")
            << QByteArray(R"({"format":"actcraft-scenario/1","name":"x","steps":[{"stroke":[[1,2]]}]})")
            << QStringLiteral("moins de 2 points");
        QTest::newRow("lecture")
            << QByteArray(R"({"format":"actcraft-scenario/1","name":"x","steps":[{"expect":"titre","value":1}]})")
            << QStringLiteral("objet.propriété");
    }

    void rejectsBadScenarios()
    {
        QFETCH(QByteArray, json);
        QFETCH(QString, expected);
        UiScenario s;
        QString error;
        QVERIFY(!parseUiScenario(json, s, error));
        QVERIFY2(error.contains(expected), qPrintable(error));
    }

    void readSpecSplitsAtFirstDotAndEquals()
    {
        UiStep s;
        QVERIFY(parseReadSpec(QStringLiteral("pageIndicator.text=2 / 2"), s));
        QCOMPARE(s.target, QStringLiteral("pageIndicator"));
        QCOMPARE(s.property, QStringLiteral("text"));
        QCOMPARE(s.value, QStringLiteral("2 / 2"));
        QVERIFY(parseReadSpec(QStringLiteral("a.b=x=y"), s));
        QCOMPARE(s.value, QStringLiteral("x=y"));
        QVERIFY(!parseReadSpec(QStringLiteral("sanspoint=1"), s));
        QVERIFY(!parseReadSpec(QStringLiteral("a.b"), s));
    }

    void propertyTextIsStable()
    {
        QCOMPARE(propertyText(true), QStringLiteral("true"));
        QCOMPARE(propertyText(8.0), QStringLiteral("8"));
        QCOMPARE(propertyText(0.5), QStringLiteral("0.5"));
        QCOMPARE(propertyText(QColor(QStringLiteral("#E53935"))), QStringLiteral("#e53935"));
        QCOMPARE(propertyText(QStringLiteral("2 / 2")), QStringLiteral("2 / 2"));
    }

    void playsOnARealPageView()
    {
        UiScenario s;
        QString error;
        QVERIFY(parseUiScenario(kAllSteps, s, error));
        // Joueur déclaré avant le moteur : détruit après la fenêtre (périphérique du stylet).
        std::unique_ptr<UiScenarioPlayer> player;
        QQmlApplicationEngine engine;
        engine.load(QUrl::fromLocalFile(QStringLiteral(ACTCRAFT_TESTS_DIR "/data/uiscenario/fenetre.qml")));
        QCOMPARE(engine.rootObjects().size(), 1);
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
        QVERIFY(window && QTest::qWaitForWindowExposed(window));
        player = std::make_unique<UiScenarioPlayer>(window);
        QTemporaryDir out;
        QStringList report;
        QVERIFY2(player->play(s, out.path(), report), qPrintable(report.join(QLatin1Char('\n'))));
        QCOMPARE(window->property("dernier").toString(), QStringLiteral("sans.argument"));
        QVERIFY(QFile::exists(out.filePath(QStringLiteral("essai-01-fin.png"))));
        QVERIFY(QFile::exists(out.filePath(QStringLiteral("index.md"))));
        const QString all = report.join(QLatin1Char('\n'));
        QVERIFY2(all.contains(QLatin1String("PIXEL trait OK")), qPrintable(all));
        QVERIFY2(all.contains(QLatin1String("LU window.compteur=12 OK")), qPrintable(all));
        UiStep wrong;
        QVERIFY(parseReadSpec(QStringLiteral("window.compteur=99"), wrong));
        QVERIFY(!player->expect(wrong, report));
        QVERIFY(report.last().contains(QLatin1String("ECART")));
        UiStep missing;
        QVERIFY(parseReadSpec(QStringLiteral("absent.text=x"), missing));
        QVERIFY(!player->expect(missing, report));
    }
};

QTEST_MAIN(TstBenchUiScenario)
#include "tst_bench_uiscenario.moc"
```

Ajouter à la fin de `tests/bench.cmake` :

```cmake
# Joueur de scénarios d'interface d'actcraft-captures (DA8), sur une fenêtre d'essai à vraie PageView.
actcraft_add_test(tst_bench_uiscenario
    SOURCES tst_bench_uiscenario.cpp
    LIBS ActCraft::bench ActCraft::ui actcraft_uiplugin Qt6::Quick)
target_compile_definitions(tst_bench_uiscenario PRIVATE ACTCRAFT_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
```

- [ ] **Étape 3 : le voir échouer.** Commande ciblée, motif `tst_bench_uiscenario`. Attendu : `error`
(`uiscenario.h` introuvable).

- [ ] **Étape 4 : écrire `bench/include/actcraft/bench/uiscenario.h`.**

```cpp
#pragma once

#include <QColor>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QVariant>

#include <cstdint>
#include <memory>
#include <vector>

class QObject;
class QPointingDevice;
class QQuickWindow;

namespace actcraft::bench {

enum class UiStepKind : std::uint8_t
{
    Command,    // {"command": "page.next", "argument": …}
    Stroke,     // {"stroke": [[x, y], …], "pressure": 0.5} — coordonnées de page
    Key,        // {"key": "F5"} — texte de QKeySequence
    Wait,       // {"wait": 300} — millisecondes
    WaitReady,  // {"waitReady": true} — page fixe prête et cache à jour (10 s au plus)
    Capture,    // {"capture": "nom"}
    CheckPixel, // {"checkPixel": "nom", "at": [x, y], "color": "#rrggbb", "tolerance": 48}
    Expect,     // {"expect": "objet.propriété", "value": …}
};

struct UiStep
{
    UiStepKind kind = UiStepKind::Wait;
    QString command;
    QVariant argument; // invalide : commande sans argument
    std::vector<QPointF> points;
    double pressure = 0.5;
    QString key;
    int ms = 0;
    QString name;
    QPointF at;
    QColor color;
    int tolerance = 48; // par canal
    QString target;     // objectName, ou « window » pour la fenêtre racine
    QString property;
    QString value; // attendue, en texte (propertyText)
};

struct UiScenario
{
    QString name;
    QString description;
    std::vector<UiStep> steps;
};

// Fichier « actcraft-scenario/1 » ; tout ou rien, `error` nomme l'étape fautive (« étape 3 : … »).
bool parseUiScenario(const QByteArray& json, UiScenario& out, QString& error);
// « objet.propriété=valeur » (premier point, premier signe =) → étape Expect ; faux si une partie manque.
bool parseReadSpec(const QString& text, UiStep& out);
// Valeur d'une propriété en texte : booléen « true »/« false », réel sans zéros inutiles, couleur « #rrggbb ».
QString propertyText(const QVariant& value);

// Joue un scénario sur une fenêtre Qt Quick (DA8). Une ligne de rapport par étape : COMMANDE, TRAIT, TOUCHE,
// ATTENTE, PRET, CAPTURE, PIXEL, LU. DURÉE DE VIE : le joueur possède le stylet synthétique ; il doit être détruit
// APRÈS la fenêtre (le déclarer avant le moteur QML).
class UiScenarioPlayer
{
public:
    explicit UiScenarioPlayer(QQuickWindow* window);
    ~UiScenarioPlayer();
    UiScenarioPlayer(const UiScenarioPlayer&) = delete;
    UiScenarioPlayer& operator=(const UiScenarioPlayer&) = delete;

    // S'arrête à la première étape en échec. Captures et index.md dans `outputDir` (créé).
    bool play(const UiScenario& scenario, const QString& outputDir, QStringList& report);
    // Lecture d'une propriété (étape Expect) : « LU cible.propriété=valeur OK » ou « … ECART (attendu …) ».
    bool expect(const UiStep& step, QStringList& report);

private:
    bool runStep(const UiStep& step, QStringList& report);
    QObject* find(const QString& target) const;
    QPointF pageToScene(QPointF page) const;
    bool isReady() const;
    void postTablet(int type, QPointF page, double pressure, bool pressed);

    QQuickWindow* m_window = nullptr;
    std::unique_ptr<QPointingDevice> m_stylus;
    QString m_scenarioName;
    QString m_outputDir;
    QStringList m_index;
    int m_captureIndex = 0;
};

} // namespace actcraft::bench
```

- [ ] **Étape 5 : écrire `bench/src/uiscenario.cpp`.**

```cpp
#include <actcraft/bench/contentcheck.h>
#include <actcraft/bench/uiscenario.h>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QPointingDevice>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTabletEvent>
#include <QTest>
#include <QThread>

#include <functional>
#include <utility>

namespace actcraft::bench {

namespace {

constexpr int kStrokeSubdivisions = 16; // déplacements postés entre deux points d'un trait
constexpr int kReadyTimeoutMs = 10000;

bool readPoint(const QJsonValue& value, QPointF& out)
{
    const QJsonArray array = value.toArray();
    if (array.size() != 2 || !array.at(0).isDouble() || !array.at(1).isDouble()) {
        return false;
    }
    out = QPointF(array.at(0).toDouble(), array.at(1).toDouble());
    return true;
}

QString jsonText(const QJsonValue& value)
{
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    if (value.isDouble()) {
        return propertyText(QVariant(value.toDouble()));
    }
    return value.toString();
}

bool splitTarget(const QString& text, QString& target, QString& property)
{
    const qsizetype dot = text.indexOf(QLatin1Char('.'));
    if (dot <= 0 || dot == text.size() - 1) {
        return false;
    }
    target = text.left(dot);
    property = text.mid(dot + 1);
    return true;
}

bool parseStep(const QJsonObject& o, UiStep& step, QString& why)
{
    if (o.contains(QStringLiteral("command"))) {
        step.kind = UiStepKind::Command;
        step.command = o.value(QStringLiteral("command")).toString();
        if (o.contains(QStringLiteral("argument"))) {
            step.argument = o.value(QStringLiteral("argument")).toVariant();
        }
        if (step.command.isEmpty()) {
            why = QStringLiteral("commande vide");
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("stroke"))) {
        step.kind = UiStepKind::Stroke;
        const QJsonArray points = o.value(QStringLiteral("stroke")).toArray();
        for (const QJsonValue& value : points) {
            QPointF point;
            if (!readPoint(value, point)) {
                why = QStringLiteral("point de trait invalide");
                return false;
            }
            step.points.push_back(point);
        }
        if (step.points.size() < 2) {
            why = QStringLiteral("trait de moins de 2 points");
            return false;
        }
        step.pressure = o.value(QStringLiteral("pressure")).toDouble(0.5);
        return true;
    }
    if (o.contains(QStringLiteral("key"))) {
        step.kind = UiStepKind::Key;
        step.key = o.value(QStringLiteral("key")).toString();
        if (QKeySequence(step.key).isEmpty()) {
            why = QStringLiteral("touche inconnue « %1 »").arg(step.key);
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("wait"))) {
        step.kind = UiStepKind::Wait;
        step.ms = o.value(QStringLiteral("wait")).toInt(-1);
        if (step.ms < 0) {
            why = QStringLiteral("attente invalide");
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("waitReady"))) {
        step.kind = UiStepKind::WaitReady;
        return true;
    }
    if (o.contains(QStringLiteral("capture"))) {
        step.kind = UiStepKind::Capture;
        step.name = o.value(QStringLiteral("capture")).toString();
        if (step.name.isEmpty()) {
            why = QStringLiteral("capture sans nom");
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("checkPixel"))) {
        step.kind = UiStepKind::CheckPixel;
        step.name = o.value(QStringLiteral("checkPixel")).toString();
        step.color = QColor(o.value(QStringLiteral("color")).toString());
        step.tolerance = o.value(QStringLiteral("tolerance")).toInt(48);
        if (step.name.isEmpty() || !readPoint(o.value(QStringLiteral("at")), step.at) || !step.color.isValid()) {
            why = QStringLiteral("contrôle de pixel invalide (checkPixel, at, color)");
            return false;
        }
        return true;
    }
    if (o.contains(QStringLiteral("expect"))) {
        step.kind = UiStepKind::Expect;
        if (!splitTarget(o.value(QStringLiteral("expect")).toString(), step.target, step.property) ||
            !o.contains(QStringLiteral("value"))) {
            why = QStringLiteral("lecture invalide (expect « objet.propriété », value)");
            return false;
        }
        step.value = jsonText(o.value(QStringLiteral("value")));
        return true;
    }
    why = QStringLiteral("étape inconnue");
    return false;
}

bool waitUntil(const std::function<bool()>& condition, int timeoutMs)
{
    QElapsedTimer clock;
    clock.start();
    while (!condition()) {
        if (clock.elapsed() > timeoutMs) {
            return false;
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
        QThread::msleep(2);
    }
    return true;
}

// Les délégués d'un Repeater n'ont pas toujours la fenêtre pour parent QObject : on suit aussi l'arbre des éléments.
QQuickItem* findItem(QQuickItem* root, const QString& name)
{
    if (!root) {
        return nullptr;
    }
    if (root->objectName() == name) {
        return root;
    }
    const QList<QQuickItem*> children = root->childItems();
    for (QQuickItem* child : children) {
        if (QQuickItem* found = findItem(child, name)) {
            return found;
        }
    }
    return nullptr;
}

} // namespace

bool parseUiScenario(const QByteArray& json, UiScenario& out, QString& error)
{
    QJsonParseError parseError{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("JSON invalide : %1").arg(parseError.errorString());
        return false;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("format")).toString() != QLatin1String("actcraft-scenario/1")) {
        error = QStringLiteral("format inconnu (attendu actcraft-scenario/1)");
        return false;
    }
    UiScenario scenario;
    scenario.name = root.value(QStringLiteral("name")).toString();
    scenario.description = root.value(QStringLiteral("description")).toString();
    if (scenario.name.isEmpty()) {
        error = QStringLiteral("nom manquant");
        return false;
    }
    const QJsonArray steps = root.value(QStringLiteral("steps")).toArray();
    if (steps.isEmpty()) {
        error = QStringLiteral("aucune étape");
        return false;
    }
    for (qsizetype i = 0; i < steps.size(); ++i) {
        UiStep step;
        QString why;
        if (!parseStep(steps.at(i).toObject(), step, why)) {
            error = QStringLiteral("étape %1 : %2").arg(i + 1).arg(why);
            return false;
        }
        scenario.steps.push_back(std::move(step));
    }
    out = std::move(scenario);
    return true;
}

bool parseReadSpec(const QString& text, UiStep& out)
{
    const qsizetype equals = text.indexOf(QLatin1Char('='));
    if (equals <= 0) {
        return false;
    }
    UiStep step;
    step.kind = UiStepKind::Expect;
    if (!splitTarget(text.left(equals), step.target, step.property)) {
        return false;
    }
    step.value = text.mid(equals + 1);
    out = std::move(step);
    return true;
}

QString propertyText(const QVariant& value)
{
    switch (value.typeId()) {
    case QMetaType::Bool:
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    case QMetaType::Double:
    case QMetaType::Float:
        return QString::number(value.toDouble(), 'g', 12);
    case QMetaType::QColor:
        return value.value<QColor>().name(QColor::HexRgb);
    default:
        return value.toString();
    }
}

UiScenarioPlayer::UiScenarioPlayer(QQuickWindow* window)
    : m_window(window),
      m_stylus(std::make_unique<QPointingDevice>(
          QStringLiteral("actcraft-captures-stylet"), 5180, QInputDevice::DeviceType::Stylus,
          QPointingDevice::PointerType::Pen,
          QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3))
{
}

UiScenarioPlayer::~UiScenarioPlayer() = default;

QObject* UiScenarioPlayer::find(const QString& target) const
{
    if (!m_window) {
        return nullptr;
    }
    if (target == QLatin1String("window")) {
        return m_window;
    }
    if (QObject* found = m_window->findChild<QObject*>(target)) {
        return found;
    }
    return findItem(m_window->contentItem(), target);
}

QPointF UiScenarioPlayer::pageToScene(QPointF page) const
{
    QPointF scene = page;
    if (QObject* view = find(QStringLiteral("pageView"))) {
        QMetaObject::invokeMethod(view, "pageToScene", Q_RETURN_ARG(QPointF, scene), Q_ARG(QPointF, page));
    }
    return scene;
}

bool UiScenarioPlayer::isReady() const
{
    QObject* fixed = find(QStringLiteral("fixedPage"));
    QObject* view = find(QStringLiteral("pageView"));
    return fixed && view && fixed->property("ready").toBool() && view->property("cacheReady").toBool();
}

void UiScenarioPlayer::postTablet(int type, QPointF page, double pressure, bool pressed)
{
    const QPointF scene = pageToScene(page);
    QCoreApplication::postEvent(
        m_window, new QTabletEvent(static_cast<QEvent::Type>(type), m_stylus.get(), scene,
                                   m_window->mapToGlobal(scene), pressure, 0, 0, 0, 0, 0, Qt::NoModifier,
                                   Qt::LeftButton, pressed ? Qt::LeftButton : Qt::NoButton));
    QCoreApplication::processEvents();
}

bool UiScenarioPlayer::expect(const UiStep& step, QStringList& report)
{
    QObject* object = find(step.target);
    const QString actual = object ? propertyText(object->property(step.property.toUtf8().constData()))
                                  : QStringLiteral("<introuvable>");
    const bool ok = object && actual == step.value;
    report << (ok ? QStringLiteral("LU %1.%2=%3 OK").arg(step.target, step.property, actual)
                  : QStringLiteral("LU %1.%2=%3 ECART (attendu %4)")
                        .arg(step.target, step.property, actual, step.value));
    return ok;
}

bool UiScenarioPlayer::runStep(const UiStep& step, QStringList& report)
{
    switch (step.kind) {
    case UiStepKind::Command: {
        QVariant result;
        const bool invoked = QMetaObject::invokeMethod(m_window, "runCommand", Q_RETURN_ARG(QVariant, result),
                                                       Q_ARG(QVariant, QVariant(step.command)),
                                                       Q_ARG(QVariant, step.argument));
        QCoreApplication::processEvents();
        const bool ok = invoked && result.toBool();
        const QString argument =
            step.argument.isValid() ? QStringLiteral(" ") + propertyText(step.argument) : QString();
        report << QStringLiteral("COMMANDE %1%2 %3")
                      .arg(step.command, argument, ok ? QStringLiteral("OK") : QStringLiteral("ECHEC"));
        return ok;
    }
    case UiStepKind::Stroke: {
        postTablet(QEvent::TabletPress, step.points.front(), step.pressure, true);
        for (std::size_t i = 1; i < step.points.size(); ++i) {
            const QPointF from = step.points[i - 1];
            const QPointF to = step.points[i];
            for (int k = 1; k <= kStrokeSubdivisions; ++k) {
                postTablet(QEvent::TabletMove, from + (to - from) * (k / static_cast<double>(kStrokeSubdivisions)),
                           step.pressure, true);
            }
        }
        postTablet(QEvent::TabletRelease, step.points.back(), 0.0, false);
        report << QStringLiteral("TRAIT %1 points OK").arg(step.points.size());
        return true;
    }
    case UiStepKind::Key: {
        // Fenêtre active si possible ; un raccourci d'application (Qt.ApplicationShortcut) agit de toute façon.
        m_window->requestActivate();
        const bool active = QTest::qWaitForWindowActive(m_window, 2000); // résultat exigé par [[nodiscard]]
        Q_UNUSED(active)
        QTest::keySequence(m_window, QKeySequence(step.key));
        QCoreApplication::processEvents();
        report << QStringLiteral("TOUCHE %1 OK").arg(step.key);
        return true;
    }
    case UiStepKind::Wait: {
        QElapsedTimer clock;
        clock.start();
        waitUntil([&clock, &step] { return clock.elapsed() >= step.ms; }, step.ms + 1000);
        report << QStringLiteral("ATTENTE %1 ms").arg(step.ms);
        return true;
    }
    case UiStepKind::WaitReady: {
        const bool ok = waitUntil([this] { return isReady(); }, kReadyTimeoutMs);
        report << (ok ? QStringLiteral("PRET OK") : QStringLiteral("PRET ECHEC (page ou cache pas prêts après 10 s)"));
        return ok;
    }
    case UiStepKind::Capture: {
        const QImage image = m_window->grabWindow();
        const QString file = QStringLiteral("%1-%2-%3.png")
                                 .arg(m_scenarioName)
                                 .arg(++m_captureIndex, 2, 10, QLatin1Char('0'))
                                 .arg(step.name);
        const QString path = QDir(m_outputDir).filePath(file);
        const bool ok = !image.isNull() && image.save(path);
        m_index << QStringLiteral("- `%1` : %2").arg(file, step.name);
        report << (ok ? QStringLiteral("CAPTURE %1").arg(path) : QStringLiteral("CAPTURE %1 ECHEC").arg(path));
        return ok;
    }
    case UiStepKind::CheckPixel: {
        const QImage image = m_window->grabWindow();
        const QPoint pixel = sceneToImage(pageToScene(step.at), m_window->devicePixelRatio());
        const QColor actual = sampleColor(image, pixel);
        const bool ok = actual.isValid() && colorNear(actual, step.color, step.tolerance);
        report << QStringLiteral("PIXEL %1 %2 (lu %3, attendu %4, tolérance %5)")
                      .arg(step.name, ok ? QStringLiteral("OK") : QStringLiteral("ECART"),
                           actual.isValid() ? actual.name() : QStringLiteral("hors image"), step.color.name())
                      .arg(step.tolerance);
        return ok;
    }
    case UiStepKind::Expect:
        return expect(step, report);
    }
    return false;
}

bool UiScenarioPlayer::play(const UiScenario& scenario, const QString& outputDir, QStringList& report)
{
    if (!m_window) {
        report << QStringLiteral("FENETRE absente");
        return false;
    }
    m_scenarioName = scenario.name;
    m_outputDir = outputDir;
    m_captureIndex = 0;
    m_index.clear();
    if (!QDir().mkpath(outputDir)) {
        report << QStringLiteral("CAPTURE dossier impossible : %1").arg(outputDir);
        return false;
    }
    bool ok = true;
    for (const UiStep& step : scenario.steps) {
        if (!runStep(step, report)) {
            ok = false;
            break;
        }
    }
    QFile index(QDir(outputDir).filePath(QStringLiteral("index.md")));
    if (index.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QString text = QStringLiteral("# Captures — %1\n\n%2\n\n").arg(scenario.name, scenario.description);
        text += m_index.join(QLatin1Char('\n'));
        text += QStringLiteral("\n\nRésultat : %1\n").arg(ok ? QStringLiteral("réussi") : QStringLiteral("en échec"));
        index.write(text.toUtf8());
    }
    return ok;
}

} // namespace actcraft::bench
```

- [ ] **Étape 6 : écrire `bench/captures/main.cpp`.**

```cpp
#include "renderbackend.h"
#include "startup.h"

#include <actcraft/bench/uiscenario.h>

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QScreen>
#include <QTest>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>

#include <cstdio>
#include <memory>
#include <vector>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft::bench;

namespace {

void printLine(const QString& line)
{
    std::printf("%s\n", line.toUtf8().constData());
    std::fflush(stdout);
}

int invalid(const QString& why)
{
    std::fprintf(stderr, "%s\n", qPrintable(why));
    printLine(QStringLiteral("SCENARIO INVALIDE"));
    return 2;
}

QString valueAfter(const QStringList& args, const QString& name)
{
    const qsizetype index = args.indexOf(name);
    return index >= 0 && index + 1 < args.size() ? args.at(index + 1) : QString();
}

} // namespace

// Captures et scénarios d'interface (DA8). Dernière ligne et code : SCENARIO OK (0), SCENARIO EN ECHEC (1),
// SCENARIO INVALIDE (2 : arguments, fichier, interface).
//   actcraft-captures --scenario <fichier.json> [--rendu auto|d3d11|logiciel] [--sortie <dossier>]
//                     [--taille LxH|plein-ecran] [--lire objet.propriété=valeur]…
int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("ActCraft-Captures"));
    const QStringList args = QGuiApplication::arguments();

    RenderBackend backend = RenderBackend::Auto;
    QString error;
    if (!parseRenderBackend(args, backend, error)) {
        return invalid(error);
    }
    applyRenderBackend(backend);

    const QString scenarioPath = valueAfter(args, QStringLiteral("--scenario"));
    QString output = valueAfter(args, QStringLiteral("--sortie"));
    if (output.isEmpty()) {
        output = QDir::current().filePath(QStringLiteral("captures"));
    }
    QFile file(scenarioPath);
    if (scenarioPath.isEmpty() || !file.open(QIODevice::ReadOnly)) {
        return invalid(QStringLiteral("scénario introuvable : %1").arg(scenarioPath));
    }
    UiScenario scenario;
    if (!parseUiScenario(file.readAll(), scenario, error)) {
        return invalid(QStringLiteral("%1 : %2").arg(scenarioPath, error));
    }
    std::vector<UiStep> reads;
    for (qsizetype i = 0; i + 1 < args.size(); ++i) {
        if (args.at(i) == QLatin1String("--lire")) {
            UiStep read;
            if (!parseReadSpec(args.at(i + 1), read)) {
                return invalid(QStringLiteral("--lire attend « objet.propriété=valeur » : %1").arg(args.at(i + 1)));
            }
            reads.push_back(read);
        }
    }
    const QString size = valueAfter(args, QStringLiteral("--taille"));
    const bool fullScreen = size == QLatin1String("plein-ecran");
    QSize windowSize(1280, 800);
    if (!size.isEmpty() && !fullScreen) {
        const QStringList parts = size.split(QLatin1Char('x'));
        bool okWidth = false;
        bool okHeight = false;
        windowSize = QSize(parts.value(0).toInt(&okWidth), parts.value(1).toInt(&okHeight));
        if (parts.size() != 2 || !okWidth || !okHeight || windowSize.isEmpty()) {
            return invalid(QStringLiteral("--taille attend LxH ou plein-ecran"));
        }
    }

    // Déclaré AVANT le moteur QML, donc détruit APRÈS la fenêtre : Qt Quick garde le stylet synthétique.
    std::unique_ptr<UiScenarioPlayer> player;
    QQmlApplicationEngine engine;
    QQuickWindow* window = loadMainWindow(engine, StartupOptions{});
    if (!window) {
        return invalid(QStringLiteral("interface non chargée"));
    }
    QScreen* screen = QGuiApplication::primaryScreen();
    window->setScreen(screen); // écran principal (4 moniteurs sur le poste de Jérôme)
    if (fullScreen) {
        window->setGeometry(screen->geometry());
        window->showFullScreen();
    } else {
        window->resize(windowSize);
        window->setPosition(screen->availableGeometry().center() -
                            QPoint(windowSize.width() / 2, windowSize.height() / 2));
        window->showNormal();
    }
    player = std::make_unique<UiScenarioPlayer>(window);

    int code = 2;
    QTimer::singleShot(0, &app, [&] {
        QStringList report;
        bool ok = QTest::qWaitForWindowExposed(window, 10000);
        if (!ok) {
            report << QStringLiteral("FENETRE non exposée");
        }
        ok = ok && player->play(scenario, output, report);
        for (const UiStep& read : reads) {
            ok = player->expect(read, report) && ok;
        }
        for (const QString& line : std::as_const(report)) {
            printLine(line);
        }
        printLine(ok ? QStringLiteral("SCENARIO OK") : QStringLiteral("SCENARIO EN ECHEC"));
        code = ok ? 0 : 1;
        QCoreApplication::exit(code);
    });
    QGuiApplication::exec();
    return code;
}
```

- [ ] **Étape 7 : `bench/CMakeLists.txt`.** En tête du fichier, après le commentaire, ajouter :

```cmake
# QTest : touches synthétiques d'actcraft-captures (raccourcis QML compris).
find_package(Qt6 6.12 REQUIRED COMPONENTS Test)
```
Dans `actcraft_add_module(bench SOURCES …)`, ajouter `include/actcraft/bench/uiscenario.h` et `src/uiscenario.cpp`
après `src/runner.cpp`. Après `actcraft_add_module(...)`, ajouter `target_link_libraries(actcraft_bench PRIVATE
Qt6::Test)`. À la fin du fichier :

```cmake
# Captures et scénarios d'interface (DA8) : outil de preuve, jamais livré.
qt_add_executable(actcraft-captures
    captures/main.cpp
    ${PROJECT_SOURCE_DIR}/app/renderbackend.cpp
    ${PROJECT_SOURCE_DIR}/app/startup.cpp)
target_include_directories(actcraft-captures PRIVATE ${PROJECT_SOURCE_DIR}/app)
target_link_libraries(actcraft-captures PRIVATE ActCraft::bench ActCraft::ui actcraft_uiplugin Qt6::Quick Qt6::Test)
actcraft_set_warnings(actcraft-captures)
set_target_properties(actcraft-captures PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
```

- [ ] **Étape 8 : le voir passer.** Commande ciblée, motif `tst_bench_uiscenario`. Attendu : `0 tests failed out of
1`. Si `LU window.compteur=12` lit 2 (la touche F5 n'a pas agi) : vérifier que le raccourci de `fenetre.qml` est en
`Qt.ApplicationShortcut` et que `QTest::keySequence` reçoit bien la `QQuickWindow` ; ne pas remplacer la touche par
un appel direct.

- [ ] **Étape 9 : essai manuel de l'exécutable (sans écran).**

```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; '{"format":"actcraft-scenario/1","name":"fumee","steps":[{"waitReady":true},{"capture":"depart"}]}' | Set-Content -Encoding utf8NoBOM .tmp/fumee.json; pwsh -NoProfile -Command ". ./scripts/dev-env.ps1; `$env:QT_QPA_PLATFORM='offscreen'; ./build/debug/actcraft-captures.exe --scenario .tmp/fumee.json --rendu logiciel --sortie .tmp/captures-fumee --lire 'pageIndicator.text=1 / 1'" > .tmp/fumee.txt 2>&1 ; Select-String -Path .tmp/fumee.txt -Pattern '^(LU|CAPTURE|PRET|SCENARIO)' | ForEach-Object { $_.Line }
```
Attendu (la coquille de L1-T12 n'étant pas forcément fusionnée dans ce worktree, `pageIndicator` peut manquer) :
`PRET OK`, `CAPTURE …/fumee-01-depart.png`, puis `LU pageIndicator.text=1 / 1 OK` (coquille présente) ou
`… ECART` (coquille absente), et la dernière ligne en conséquence. Rapporter les lignes telles quelles.

- [ ] **Étape 10 : format et preuve.** `pwsh -NoProfile -File ./scripts/format.ps1` ; forme bornée (N ≥ 33,
`TESTS OK`, `tst_bench_uiscenario .*Passed`).

- [ ] **Étape 11 : commit.**

```powershell
Set-Location <worktree>; git add bench/CMakeLists.txt bench/include/actcraft/bench/uiscenario.h bench/src/uiscenario.cpp bench/captures/main.cpp tests/bench.cmake tests/tst_bench_uiscenario.cpp tests/data/uiscenario/fenetre.qml
git -C <worktree> commit -m "bench : actcraft-captures, joueur de scénarios d'interface et captures" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

<a id="tache-l1-t14"></a>
### Tâche L1-T14 — Banc adapté à la nouvelle interface (DA7)

**Objectif.** DA7 : le banc continue de charger la **vraie** interface (coffre 0079) avec la page générée ; la
coquille de L1-T12 réduit la zone de la page (barre en haut, palette à droite, barres de défilement). Garantir que
les points contrôlés par la démonstration du banc (trois témoins, point du trait de démonstration) restent dans la
vue à 100 %, hors barre, palette et barres de défilement, à 1280 × 800 (fumée ctest, preuves sans écran) ; que la
fumée du banc passe en `release` (code 0 ou 1, contrôles de contenu valides) ; et que le banc sans écran donne
`content_check.ok = True`. La mesure plein écran D3D11 (porte de non-régression du lot) est jouée à la clôture.

**Patron à éviter (concepteur-critique).** Ne pas « réparer » le banc en lui faisant charger une autre fenêtre que
l'interface livrée, ni en masquant palette ou barre pendant la mesure : on mesurerait autre chose que ce que les
enseignants auront. Si un point sort de la vue, on déplace la **vue** (défilement), jamais l'interface.

**Fichiers.** Modifier : `tests/tst_bench_verdict.cpp` ; `bench/src/runner.cpp` seulement si le test rouge le
montre nécessaire (étape 4).

**Test et commande bornée.** Fonction ajoutée à `tst_bench_verdict` : `benchTargetsAreVisibleInTheShell`. Preuves :
suite `release`, puis suite `debug`, puis banc sans écran :
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 -Preset release > .tmp/test-release.txt 2>&1 ; Select-String -Path .tmp/test-release.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2
Set-Location <worktree>; Select-String -Path .tmp/test-release.txt -Pattern '(tst_bench_verdict|tst_bench_smoke) .*Passed' | ForEach-Object { $_.Line.Trim() }
```
Attendu : N ≥ 40, `TESTS OK`, deux lignes `Passed` ; puis la forme bornée `debug` (N ≥ 40, `TESTS OK`) ; puis le banc
sans écran (`BUDGET TENU` ou `BUDGET DEPASSE`) et `content_check.ok = True`.

**Interdits.** Ne pas changer les scénarios mesurés (`scenarios.cpp`), les seuils (`verdict.cpp`), ni le JSON
`actcraft-bench/1`. Ne pas toucher à `ui/qml/`.

**Terminé quand.** Les trois preuves sont vertes ; JSON du banc joint ; commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire le test.** Dans `tests/tst_bench_verdict.cpp`, ajouter aux inclusions
`#include <actcraft/bench/scenarios.h>` et `#include <actcraft/core/generator.h>` (si absentes), puis dans la
section `private slots:` :

```cpp
    // DA7 : dans la nouvelle interface, les points contrôlés par la démonstration du banc tombent dans la vue à
    // 100 %, hors barre, palette et barres de défilement, à 1280 × 800 (taille de la fumée et des preuves).
    void benchTargetsAreVisibleInTheShell()
    {
        App app;
        QVERIFY(start(app));
        app.window->resize(1280, 800);
        QTRY_COMPARE(app.window->width(), 1280);
        app.view->setZoom(1.0);
        QTRY_VERIFY(app.view->cacheReady());
        const qreal scrollBarRoom = 16; // barres de défilement, à droite et en bas de la vue
        const QRectF visible = app.view->mapRectToScene(
            QRectF(0, 0, app.view->width() - scrollBarRoom, app.view->height() - scrollBarRoom));
        const core::Witnesses& w = core::witnesses();
        const QList<QPointF> targets{
            QPointF(w.shapeRect.center().x, w.shapeRect.center().y),
            QPointF(w.imageRect.center().x, w.imageRect.center().y),
            QPointF(w.strokeProbe.x, w.strokeProbe.y),
            bench::demoInkProbePoint(),
        };
        for (const QPointF& page : targets) {
            const QPointF scene = app.view->pageToScene(page);
            QVERIFY2(visible.contains(scene),
                     qPrintable(QStringLiteral("point de page (%1, %2) → scène (%3, %4) hors de %5 × %6")
                                    .arg(page.x())
                                    .arg(page.y())
                                    .arg(scene.x())
                                    .arg(scene.y())
                                    .arg(visible.width())
                                    .arg(visible.height())));
        }
    }
```
(`App`, `start` et les espaces de noms `core`, `bench`, `ui` sont ceux du fichier ; `start` passe déjà
`generatedPage = true` depuis L1-T12.)

- [ ] **Étape 2 : le lancer.** Commande ciblée, motif `tst_bench_verdict`. Deux issues :
  - **vert** : la coquille laisse les points visibles ; passer à l'étape 5 (aucun changement du banc) ;
  - **rouge** : le message nomme le point hors de la vue ; passer aux étapes 3 et 4.

- [ ] **Étape 3 (seulement si rouge) : comprendre.** Lire dans `bench/src/runner.cpp` les appels `setZoom(1.0)` de
`runDemo` et `setLevel` ; relever la taille de la vue (`app.view->width()`, `height()`) et le défilement après
`setZoom(1.0)` (`app.view->pan()`), et les noter au rapport.

- [ ] **Étape 4 (seulement si rouge) : recentrer la vue sur les témoins.** Dans `bench/src/runner.cpp`, ajouter dans
l'espace de noms anonyme :

```cpp
// Enveloppe des trois témoins de la page générée (core/src/generator.cpp : kShapeWitness, kImageWitness,
// kStrokeWitness) : centrée dans la vue à 100 %, elle reste visible malgré la barre et la palette (DA7).
QPointF witnessesCenter()
{
    const core::Witnesses& w = core::witnesses();
    const double left = std::min({w.shapeRect.x, w.imageRect.x, w.strokeRect.x});
    const double top = std::min({w.shapeRect.y, w.imageRect.y, w.strokeRect.y});
    const double right = std::max({w.shapeRect.x + w.shapeRect.width, w.imageRect.x + w.imageRect.width,
                                   w.strokeRect.x + w.strokeRect.width});
    const double bottom = std::max({w.shapeRect.y + w.shapeRect.height, w.imageRect.y + w.imageRect.height,
                                    w.strokeRect.y + w.strokeRect.height});
    return QPointF((left + right) / 2, (top + bottom) / 2);
}
```
et, dans `Runner::runDemo`, après **chacun** des deux `m_view->setZoom(1.0);` (début de la démonstration et
« retour à 100 % »), ajouter :

```cpp
    m_view->setPan(QPointF(m_view->width() / 2, m_view->height() / 2) - witnessesCenter());
```
(`setPan` borne déjà le défilement ; à 100 %, page et vue ont la même échelle.) Relancer la commande ciblée,
motif `tst_bench_verdict`, jusqu'au vert. Le test de l'étape 1 n'est pas modifié ; s'il reste rouge, le point
visé est hors d'atteinte à 1280 × 800 : arrêter et le dire au rapport (décision de taille pour l'orchestrateur).

- [ ] **Étape 5 : preuves.** `pwsh -NoProfile -File ./scripts/format.ps1` ; les deux commandes bornées `release`
ci-dessus ; la preuve que la fonction ajoutée a tourné en release :

```powershell
Set-Location <worktree>; pwsh -NoProfile -Command ". ./scripts/dev-env.ps1; `$env:QT_QPA_PLATFORM='offscreen'; `$env:QT_QUICK_BACKEND='software'; ./build/release/tst_bench_verdict.exe *> .tmp/fonctions-verdict.txt"; Select-String -Path .tmp/fonctions-verdict.txt -Pattern '^(PASS|FAIL!).*benchTargetsAreVisibleInTheShell|^Totals' | ForEach-Object { $_.Line }
```
Attendu : `PASS   : TstBenchVerdict::benchTargetsAreVisibleInTheShell()` et `Totals: … 0 failed` ; la forme
bornée `debug` ; le banc sans écran et sa ligne `content_check.ok`. Attendu : N ≥ 40 en release
et en debug, `TESTS OK` deux fois, `tst_bench_verdict` et `tst_bench_smoke` `Passed`, `BUDGET TENU` ou
`BUDGET DEPASSE`, `content_check.ok = True`. Joindre le nom du JSON et les quatre contrôles (`checks`) au rapport.

- [ ] **Étape 6 : commit.**

```powershell
Set-Location <worktree>; git add tests/tst_bench_verdict.cpp
git -C <worktree> commit -m "bench : points contrôlés visibles dans la nouvelle interface" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```
Si l'étape 4 a été faite, ajouter `bench/src/runner.cpp` à `git add` et au message (« vue recentrée sur les
témoins »).

---

<a id="tache-l1-t15"></a>
### Tâche L1-T15 — Scénario `premiers-pas` et `scripts/captures.ps1` (S2)

**Objectif.** Le scénario d'acceptation S2 joué par l'agent, sans souris réelle : la fenêtre montre menus,
palette et page ; le rouge et l'épaisseur 8 sont choisis par leurs commandes ; un trait est tracé au stylet
injecté ; « page suivante » affiche « 2 / 2 » ; retour : le trait rouge est toujours là (contrôle de pixel) ; F5
bascule le plein écran (et le rétablit). Le même fichier sert hors écran (ctest, chaque tâche) et à l'écran en
D3D11 (clôture, jeton `ecran`). `scripts/captures.ps1` est la commande de preuve visuelle du profil.

**Fichiers.**
- Créer : `bench/scenarios/premiers-pas.json`, `scripts/captures.ps1`
- Modifier : `tests/bench.cmake`

**Test et commande bornée.** Test ctest `tst_captures_premiers_pas` : lance `actcraft-captures` sur le scénario,
hors écran, en rendu logiciel, à 1280 × 800 ; réussit sur `SCENARIO OK`, échoue sur `ECART`, `SCENARIO EN ECHEC` ou
`SCENARIO INVALIDE`. Rouge : commande ciblée, motif `tst_captures` → `tests failed` (fichier de scénario absent :
`SCENARIO INVALIDE`). Vert : forme bornée, N ≥ 42, `TESTS OK`, `tst_captures_premiers_pas .*Passed` ; preuve
visuelle :
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario premiers-pas -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "pageIndicator.text=1 / 2" > .tmp/captures.txt 2>&1 ; Select-String -Path .tmp/captures.txt -Pattern '^(LU|PIXEL|CAPTURE) |CAPTURES (OK|EN ECHEC)' | Select-Object -Last 16
```
Attendu : les lignes `LU … OK` (dont `LU pageIndicator.text=1 / 2 OK`), trois lignes `PIXEL … OK`, cinq lignes
`CAPTURE …`, puis `CAPTURES OK`.

**Interdits.** Pas de souris réelle, pas d'écran (hors écran ici ; l'écran est pour la clôture). Captures sous
`.tmp/` ou `build/`, jamais commitées.

**Terminé quand.** Preuves vertes ; les cinq captures sont regardées (outil Read) et décrites en une ligne chacune
au rapport ; commit fait.

**Étapes.**

- [ ] **Étape 1 : déclarer le test.** Ajouter à la fin de `tests/bench.cmake` :

```cmake
# Scénario d'acceptation S2 (premiers pas), hors écran en rendu logiciel (DA8, décision 36).
add_test(NAME tst_captures_premiers_pas
    COMMAND $<TARGET_FILE:actcraft-captures>
        --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/premiers-pas.json
        --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-premiers-pas)
set_tests_properties(tst_captures_premiers_pas PROPERTIES
    TIMEOUT 120
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    PASS_REGULAR_EXPRESSION "SCENARIO OK"
    FAIL_REGULAR_EXPRESSION "ECART|SCENARIO EN ECHEC|SCENARIO INVALIDE")
```

- [ ] **Étape 2 : le voir échouer.** Commande ciblée, motif `tst_captures`. Attendu : `tests failed`
(`scénario introuvable`, `SCENARIO INVALIDE`).

- [ ] **Étape 3 : écrire `bench/scenarios/premiers-pas.json`.** Le rouge est la première pastille de
`actions/data/layout.json` (`#E53935`).

```json
{
  "format": "actcraft-scenario/1",
  "name": "premiers-pas",
  "description": "S2 : menus, palette et page visibles ; stylo rouge d'épaisseur 8 ; trait ; page suivante (2 / 2) et retour ; le trait est toujours là ; F5 bascule le plein écran.",
  "steps": [
    {"waitReady": true},
    {"expect": "topBar.visible", "value": true},
    {"expect": "toolPalette.visible", "value": true},
    {"expect": "pageView.visible", "value": true},
    {"expect": "pageIndicator.text", "value": "1 / 1"},
    {"capture": "interface"},
    {"command": "pen.color", "argument": "#E53935"},
    {"command": "pen.width", "argument": 8},
    {"expect": "pageView.penColor", "value": "#e53935"},
    {"expect": "pageView.penWidth", "value": 8},
    {"stroke": [[300, 400], [700, 400]], "pressure": 0.5},
    {"waitReady": true},
    {"checkPixel": "trait-rouge", "at": [500, 400], "color": "#E53935", "tolerance": 60},
    {"capture": "trait"},
    {"command": "page.next"},
    {"waitReady": true},
    {"expect": "pageIndicator.text", "value": "2 / 2"},
    {"checkPixel": "page-2-vierge", "at": [500, 400], "color": "#FFFFFF", "tolerance": 8},
    {"capture": "page-2"},
    {"command": "page.previous"},
    {"waitReady": true},
    {"expect": "pageIndicator.text", "value": "1 / 2"},
    {"checkPixel": "trait-conserve", "at": [500, 400], "color": "#E53935", "tolerance": 60},
    {"capture": "retour-page-1"},
    {"key": "F5"},
    {"wait": 300},
    {"expect": "window.fullScreen", "value": true},
    {"capture": "plein-ecran"},
    {"key": "F5"},
    {"wait": 300},
    {"expect": "window.fullScreen", "value": false}
  ]
}
```

- [ ] **Étape 4 : le voir passer.** Commande ciblée, motif `tst_captures`. Attendu : `0 tests failed out of 1`.
En cas d'échec, lire la sortie (`ctest --preset debug -R tst_captures --output-on-failure` dans une session où
`dev-env.ps1` est chargé) : la ligne `ECART` ou `ECHEC` nomme l'étape ; corriger la cause (jamais le scénario pour
le faire passer, sauf erreur de saisie du scénario lui-même).

- [ ] **Étape 5 : écrire `scripts/captures.ps1`.**

```powershell
# Preuve visuelle (DA8, décision 36) : joue des scénarios d'interface et écrit captures PNG et index.md.
# Usage : pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario <nom>|tous [-Rendu d3d11|logiciel] [-HorsEcran]
#         -Sortie <dossier> [-Lire "<objet>.<propriété>=<attendu>[;…]"] [-Taille LxH|plein-ecran]
#         [-Preset debug|release] [-SansConstruire]
# Scénarios : bench/scenarios/<nom>.json. Sorties sous .tmp/ ou build/, jamais commitées.
# Dernière ligne et code : « CAPTURES OK » (0) ou « CAPTURES EN ECHEC » (1).
param(
    [string]$Scenario = 'tous',
    [ValidateSet('d3d11', 'logiciel')][string]$Rendu = 'logiciel',
    [switch]$HorsEcran,
    [Parameter(Mandatory)][string]$Sortie,
    [string]$Lire = '',
    [string]$Taille = '1280x800',
    [ValidateSet('debug', 'release')][string]$Preset = 'debug',
    [switch]$SansConstruire
)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

if (-not $SansConstruire) {
    & "$PSScriptRoot/build.ps1" -Preset $Preset
    if ($LASTEXITCODE -ne 0) { Write-Output 'CAPTURES EN ECHEC (compilation)'; exit 1 }
}
. "$PSScriptRoot/dev-env.ps1"

$exe = Join-Path (Get-Location) "build/$Preset/actcraft-captures.exe"
if (-not (Test-Path $exe)) { Write-Output "CAPTURES EN ECHEC ($exe introuvable)"; exit 1 }
$fichiers = @(if ($Scenario -eq 'tous') { Get-ChildItem bench/scenarios/*.json } else { Get-Item "bench/scenarios/$Scenario.json" })

if ($HorsEcran) {
    $env:QT_QPA_PLATFORM = 'offscreen'
} else {
    Write-Output 'Les scénarios s''affichent sur l''écran principal (souris non pilotée).'
}
New-Item -ItemType Directory -Force $Sortie | Out-Null
$racine = (Resolve-Path $Sortie).Path
$lectures = @($Lire.Split(';', [System.StringSplitOptions]::RemoveEmptyEntries) | ForEach-Object { $_.Trim() })

$echecs = 0
foreach ($f in $fichiers) {
    $dossier = Join-Path $racine "$($f.BaseName)-$Rendu"
    $arguments = @('--scenario', $f.FullName, '--rendu', $Rendu, '--taille', $Taille, '--sortie', $dossier)
    foreach ($l in $lectures) { $arguments += @('--lire', $l) }
    & $exe @arguments
    if ($LASTEXITCODE -ne 0) { $echecs++ }
}
if ($echecs -gt 0) { Write-Output 'CAPTURES EN ECHEC'; exit 1 }
Write-Output 'CAPTURES OK'
```

- [ ] **Étape 6 : preuve visuelle.** Lancer la commande de preuve visuelle ci-dessus. Attendu : `LU … OK`, trois
`PIXEL … OK`, cinq `CAPTURE`, `CAPTURES OK`. Puis ouvrir chacune des cinq images de
`.tmp/captures/premiers-pas-logiciel/` avec l'outil Read et décrire au rapport, une ligne par image : barre en haut
avec les menus « Fichier », « Affichage », « Outils », « Aide », indicateur et échelle ; palette à droite avec les
16 pastilles et les épaisseurs ; page blanche sur fond bleu ; trait rouge horizontal ; « 2 / 2 » et page vierge ;
retour avec le trait ; plein écran.

- [ ] **Étape 7 : format et preuve.** `pwsh -NoProfile -File ./scripts/format.ps1` ; forme bornée (N ≥ 42,
`TESTS OK`, `tst_captures_premiers_pas .*Passed`).

- [ ] **Étape 8 : commit.**

```powershell
Set-Location <worktree>; git add bench/scenarios/premiers-pas.json scripts/captures.ps1 tests/bench.cmake
git -C <worktree> commit -m "Scénario premiers-pas (S2) et commande de preuve visuelle" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

<a id="tache-l1-t16"></a>
### Tâche L1-T16 — Installateur Windows : Inno Setup, `paquet.ps1`, `essai-installateur.ps1` (S1)

**Objectif.** Décision 26, DA9, S1 : `scripts/paquet.ps1` construit le preset `release` (la construction mesurée),
prépare `build/paquet/windows/` (`actcraft.exe`, `windeployqt --release --no-compiler-runtime --no-translations
--qmldir ui/qml`, runtime MSVC copié à côté de l'exécutable, `LICENSE.txt`, `THIRD_PARTY_NOTICES.md`, `LICENSES/`,
aucun PDB), contrôle les modules Qt (`verifier-licences.ps1`) et compile `packaging/windows/actcraft.iss` en
`build/paquet/ActCraft-<version>-setup.exe` : installation **par utilisateur sans droits d'administrateur** par
défaut (`%LOCALAPPDATA%\Programs\ActCraft`, choix « pour tous » proposé), menu Démarrer, désinstallation,
installation silencieuse, association `.actcraft` cochée, `.flipchart` proposée **décochée** (ajout à « Ouvrir
avec », sans prendre la place d'ActivInspire), non signé (N1-4). `scripts/essai-installateur.ps1` joue S1 :
installation silencieuse dans un dossier temporaire, `--version` = `ActCraft <version>`, lancement de 5 s dont les
modules `vcruntime140.dll`, `vcruntime140_1.dll`, `msvcp140.dll` sont chargés **depuis ce dossier** (processus
suivi par son PID), sortie à 0, désinstallation silencieuse qui vide le dossier.

**Fichiers.**
- Créer : `packaging/windows/actcraft.iss`, `scripts/paquet.ps1`, `scripts/essai-installateur.ps1`
- Modifier (seulement si `verifier-licences.ps1` refuse un module LGPL légitime) : `packaging/qt-autorises.txt`

**Test et commande bornée.** Le test est le script d'essai S1 (rouge tant que le paquet n'existe pas) :
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/paquet.ps1 > .tmp/paquet.txt 2>&1 ; Select-String -Path .tmp/paquet.txt -Pattern 'LICENCES (OK|EN ECHEC)|PAQUET OK|Exception|échec' | Select-Object -Last 3
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/essai-installateur.ps1 > .tmp/essai-installateur.txt 2>&1 ; Select-String -Path .tmp/essai-installateur.txt -Pattern '^(--version|vcruntime|msvcp|INSTALLATEUR)' | ForEach-Object { $_.Line }
```
Attendu : `LICENCES OK`, `PAQUET OK : …\build\paquet\ActCraft-0.1.0-setup.exe` ; puis `--version : ActCraft
0.1.0`, trois lignes de DLL chargées depuis le dossier temporaire, `INSTALLATEUR OK`. Le lancement de 5 s ouvre une
fenêtre ordinaire sur l'écran principal : l'annoncer d'une ligne avant de lancer l'essai.

**Interdits.** Pas de `vc_redist` (exige l'administrateur, décision 36 du prototype). Pas de signature (N1-4). Pas
d'installation « pour tous » dans l'essai (`/CURRENTUSER`). Ne jamais publier ni téléverser l'installateur. Ne
fermer l'application lancée que par son PID (`Stop-Process -Id`). Si `ISCC.exe` est introuvable (`VERIF inno =
ECHEC` en L1-T2) : s'arrêter et le dire (porte « installation »).

**Terminé quand.** `PAQUET OK` et `INSTALLATEUR OK` ; commit fait.

**Étapes.**

- [ ] **Étape 1 : le test d'abord.** Créer `scripts/essai-installateur.ps1` :

```powershell
# Essai de l'installateur (scénario S1) : installation silencieuse par utilisateur, --version, lancement de 5 s avec
# contrôle du runtime MSVC chargé depuis le dossier d'installation, désinstallation silencieuse.
# Usage : pwsh -NoProfile -File ./scripts/essai-installateur.ps1 [-Setup <chemin>] [-Rendu d3d11|logiciel]
# Dernière ligne et code : « INSTALLATEUR OK » (0) ou « INSTALLATEUR EN ECHEC : <raison> » (1).
param([string]$Setup, [ValidateSet('d3d11', 'logiciel')][string]$Rendu = 'd3d11')
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
. "$PSScriptRoot/version-projet.ps1"
$version = Get-ActCraftVersion
if (-not $Setup) { $Setup = "build/paquet/ActCraft-$version-setup.exe" }

function Echec([string]$Raison) {
    Write-Output "INSTALLATEUR EN ECHEC : $Raison"
    exit 1
}

if (-not (Test-Path $Setup)) { Echec "$Setup introuvable" }
$dossier = Join-Path ([IO.Path]::GetTempPath()) ('actcraft-essai-' + [guid]::NewGuid().ToString('N').Substring(0, 8))
$journal = "$dossier.log"

$p = Start-Process -FilePath (Resolve-Path $Setup).Path -Wait -PassThru -ArgumentList @(
    '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/CURRENTUSER', "/DIR=`"$dossier`"", "/LOG=`"$journal`"")
if ($p.ExitCode -ne 0) { Echec "installation : code $($p.ExitCode) (journal $journal)" }
$exe = Join-Path $dossier 'actcraft.exe'
if (-not (Test-Path $exe)) { Echec 'actcraft.exe absent après installation' }
Write-Output "Installé dans $dossier"

# --version : application graphique, sortie standard redirigée vers un fichier.
$sortie = "$dossier-version.txt"
$v = Start-Process -FilePath $exe -ArgumentList '--version' -RedirectStandardOutput $sortie -Wait -PassThru
$texte = (Get-Content -Raw $sortie).Trim()
Write-Output "--version : $texte"
if ($v.ExitCode -ne 0 -or $texte -ne "ActCraft $version") { Echec "--version a donné « $texte » (attendu ActCraft $version)" }

# Lancement de 5 s : le processus lancé ici est suivi par son PID ; runtime MSVC chargé depuis $dossier.
$run = Start-Process -FilePath $exe -ArgumentList '--rendu', $Rendu, '--quitter-apres', '5000' -PassThru
$null = $run.Handle # garde le descripteur : ExitCode lisible après la fin
Start-Sleep -Seconds 2
$modules = @((Get-Process -Id $run.Id).Modules | ForEach-Object { $_.FileName })
foreach ($dll in 'vcruntime140.dll', 'vcruntime140_1.dll', 'msvcp140.dll') {
    $chemin = $modules | Where-Object { (Split-Path $_ -Leaf) -ieq $dll } | Select-Object -First 1
    Write-Output "$dll : $chemin"
    if (-not $chemin -or -not $chemin.StartsWith($dossier, [StringComparison]::OrdinalIgnoreCase)) {
        Stop-Process -Id $run.Id -Force
        Echec "$dll non chargé depuis $dossier"
    }
}
if (-not $run.WaitForExit(30000)) { Stop-Process -Id $run.Id -Force; Echec 'pas de fermeture après --quitter-apres 5000' }
if ($run.ExitCode -ne 0) { Echec "code de sortie $($run.ExitCode)" }

# Désinstallation silencieuse : le dossier est vidé (le désinstallateur finit après son retour).
$u = Start-Process -FilePath (Join-Path $dossier 'unins000.exe') -ArgumentList '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART' -Wait -PassThru
if ($u.ExitCode -ne 0) { Echec "désinstallation : code $($u.ExitCode)" }
$limite = (Get-Date).AddSeconds(30)
while ((Get-ChildItem -Recurse -File $dossier -ErrorAction SilentlyContinue) -and (Get-Date) -lt $limite) {
    Start-Sleep -Milliseconds 500
}
$restes = @(Get-ChildItem -Recurse -File $dossier -ErrorAction SilentlyContinue)
if ($restes.Count -gt 0) { Echec "$($restes.Count) fichier(s) restant(s) après désinstallation" }
Write-Output 'INSTALLATEUR OK'
```

- [ ] **Étape 2 : le voir échouer.** Lancer la deuxième commande bornée. Attendu :
`INSTALLATEUR EN ECHEC : build/paquet/ActCraft-0.1.0-setup.exe introuvable`.

- [ ] **Étape 3 : `packaging/windows/actcraft.iss`.**

```iss
; Installateur Windows d'ActCraft (décision 26, DA9 du cadrage livraison-complete). Compilé par scripts/paquet.ps1 :
;   ISCC.exe /DAppVersion=<x.y.z> /DSourceDir=<dossier préparé> /DOutputDir=<build/paquet> actcraft.iss
#ifndef AppVersion
  #error AppVersion manquant (/DAppVersion=x.y.z)
#endif
#ifndef SourceDir
  #error SourceDir manquant (/DSourceDir=<dossier préparé>)
#endif
#ifndef OutputDir
  #define OutputDir "."
#endif

[Setup]
AppId={{5F0C2E7A-3B4D-4C8E-9A61-ACC0C4AF7A10}
AppName=ActCraft
AppVersion={#AppVersion}
AppVerName=ActCraft {#AppVersion}
AppPublisher=ActCraft
AppPublisherURL=https://github.com/jejeasterix/actcraft
AppSupportURL=https://github.com/jejeasterix/actcraft/issues
DefaultDirName={autopf}\ActCraft
DefaultGroupName=ActCraft
DisableProgramGroupPage=yes
; Par utilisateur, sans droits d'administrateur ; « pour tous » proposé (boîte ou /ALLUSERS).
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog commandline
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#OutputDir}
OutputBaseFilename=ActCraft-{#AppVersion}-setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ChangesAssociations=yes
UninstallDisplayIcon={app}\actcraft.exe
LicenseFile={#SourceDir}\LICENSE.txt
SetupLogging=yes

[Languages]
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"

[Tasks]
Name: "assoc_actcraft"; Description: "Ouvrir les documents .actcraft avec ActCraft"; GroupDescription: "Associations de fichiers :"
Name: "assoc_flipchart"; Description: "Proposer ActCraft pour ouvrir les paperboards .flipchart"; GroupDescription: "Associations de fichiers :"; Flags: unchecked
Name: "desktopicon"; Description: "Créer un raccourci sur le bureau"; GroupDescription: "Raccourcis :"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\ActCraft"; Filename: "{app}\actcraft.exe"
Name: "{autodesktop}\ActCraft"; Filename: "{app}\actcraft.exe"; Tasks: desktopicon

[Registry]
Root: HKA; Subkey: "Software\Classes\.actcraft"; ValueType: string; ValueName: ""; ValueData: "ActCraft.Document"; Flags: uninsdeletevalue; Tasks: assoc_actcraft
; Le ProgID sert aux deux associations : créé dès que l'une des deux est cochée.
Root: HKA; Subkey: "Software\Classes\ActCraft.Document"; ValueType: string; ValueName: ""; ValueData: "Document ActCraft"; Flags: uninsdeletekey; Tasks: assoc_actcraft or assoc_flipchart
Root: HKA; Subkey: "Software\Classes\ActCraft.Document\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\actcraft.exe,0"; Tasks: assoc_actcraft or assoc_flipchart
Root: HKA; Subkey: "Software\Classes\ActCraft.Document\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\actcraft.exe"" ""%1"""; Tasks: assoc_actcraft or assoc_flipchart
; .flipchart : seulement « Ouvrir avec » ; l'application par défaut (ActivInspire installé) reste inchangée.
Root: HKA; Subkey: "Software\Classes\.flipchart\OpenWithProgids"; ValueType: string; ValueName: "ActCraft.Document"; ValueData: ""; Flags: uninsdeletevalue; Tasks: assoc_flipchart

[Run]
Filename: "{app}\actcraft.exe"; Description: "Lancer ActCraft"; Flags: nowait postinstall skipifsilent
```

- [ ] **Étape 4 : `scripts/paquet.ps1`.**

```powershell
# Installateur Windows (décision 26, DA9) : construction release, déploiement Qt, runtime MSVC local, licences,
# compilation Inno Setup. Usage : pwsh -NoProfile -File ./scripts/paquet.ps1 [-SansConstruire]
# Produit build/paquet/ActCraft-<version>-setup.exe : local, jamais publié (publication = go écrit de Jérôme).
# Dernière ligne : « PAQUET OK : <chemin> » ; toute erreur arrête le script (exception, code 1).
param([switch]$SansConstruire)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
. "$PSScriptRoot/version-projet.ps1"
$version = Get-ActCraftVersion

if (-not $SansConstruire) {
    & "$PSScriptRoot/build.ps1" -Preset release
    if ($LASTEXITCODE -ne 0) { throw 'compilation release en échec' }
}
. "$PSScriptRoot/dev-env.ps1"
if (-not $env:VCToolsRedistDir) { throw 'VCToolsRedistDir absent : environnement Visual Studio requis (dev-env.ps1).' }

$candidats = @($env:ACTCRAFT_ISCC,
    (Join-Path $env:LOCALAPPDATA 'Programs/Inno Setup 6/ISCC.exe'),
    (Join-Path $HOME '.tools/InnoSetup6/ISCC.exe'),
    (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6/ISCC.exe'),
    (Join-Path $env:ProgramFiles 'Inno Setup 6/ISCC.exe'))
$iscc = $candidats | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
if (-not $iscc) { $iscc = (Get-Command iscc -ErrorAction SilentlyContinue).Source }
if (-not $iscc) { throw 'ISCC.exe introuvable : Inno Setup 6 (docs/verifications/2026-10-14-outillage-paquets.md) ou ACTCRAFT_ISCC.' }

$racine = Join-Path (Get-Location) 'build/paquet'
$stage = Join-Path $racine 'windows'
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item build/release/actcraft.exe $stage

$windeployqt = Join-Path $env:ACTCRAFT_QT_DIR 'bin/windeployqt.exe'
& $windeployqt --release --no-compiler-runtime --no-translations --qmldir ui/qml (Join-Path $stage 'actcraft.exe')
if ($LASTEXITCODE -ne 0) { throw 'windeployqt en échec' }
Get-ChildItem -Recurse -Filter *.pdb $stage | Remove-Item -Force # N2-7 : pas de PDB dans l'installateur

# Runtime MSVC à côté de l'exécutable : vc_redist exigerait l'administrateur.
$crt = Join-Path $env:VCToolsRedistDir 'x64\Microsoft.VC143.CRT'
foreach ($dll in 'vcruntime140.dll', 'vcruntime140_1.dll', 'msvcp140.dll') { Copy-Item (Join-Path $crt $dll) $stage }
Copy-Item LICENSE (Join-Path $stage 'LICENSE.txt')
Copy-Item THIRD_PARTY_NOTICES.md $stage
Copy-Item -Recurse LICENSES (Join-Path $stage 'LICENSES')

& "$PSScriptRoot/verifier-licences.ps1" -Dossier $stage
if ($LASTEXITCODE -ne 0) { throw 'licences : bibliothèque Qt hors de packaging/qt-autorises.txt' }

& $iscc /Qp "/DAppVersion=$version" "/DSourceDir=$stage" "/DOutputDir=$racine" packaging/windows/actcraft.iss
if ($LASTEXITCODE -ne 0) { throw 'ISCC en échec' }
$setup = Join-Path $racine "ActCraft-$version-setup.exe"
if (-not (Test-Path $setup)) { throw "$setup absent" }
Write-Output "PAQUET OK : $setup"
```

- [ ] **Étape 5 : construire le paquet.** Lancer la première commande bornée. Si la sortie dit
`LICENCES EN ECHEC : <modules>` : pour chaque module, vérifier sa licence sur https://doc.qt.io/qt-6/qtmodules.html ;
un module sous LGPL s'ajoute à `packaging/qt-autorises.txt` (une ligne), un module sous GPL seule ne s'ajoute
**jamais** : arrêter et le dire au rapport (il faut alors retirer son import QML ou sa dépendance). Relancer
jusqu'à `PAQUET OK`.

- [ ] **Étape 6 : le voir passer.** Annoncer d'une ligne la fenêtre de 5 s, puis lancer la deuxième commande bornée.
Attendu : `--version : ActCraft 0.1.0`, les trois DLL sous le dossier `actcraft-essai-…`, `INSTALLATEUR OK`.

- [ ] **Étape 7 : commit.**

```powershell
Set-Location <worktree>; git add packaging/windows/actcraft.iss scripts/paquet.ps1 scripts/essai-installateur.ps1
git -C <worktree> commit -m "Installateur Windows par utilisateur (Inno Setup) et essai S1" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```
Si l'étape 5 a complété `packaging/qt-autorises.txt`, l'ajouter à `git add` et nommer les modules dans le message.

---

<a id="tache-l1-t17"></a>
### Tâche L1-T17 — Chaîne Android : presets, propriétés de l'APK, `paquet-android.ps1`

**Objectif.** Décisions 27, 28, 30, 31, DA9 : presets `android-arm64` et `android-x86_64` (Release, banc et tests
exclus) ; APK d'identifiant `io.github.jejeasterix.actcraft`, `versionName` 0.1.0, `versionCode` 100, **aucune
permission** ; `scripts/paquet-android.ps1` construit les deux APK, les aligne, les signe par une clé de débogage
générée localement (jamais de clé de release ici), contrôle identifiant, version, permissions, signature et
licences (`verifier-licences.ps1 -Apk`), et range `build/paquet/ActCraft-<version>-<abi>.apk`. Le rendu Android
reste celui de Qt par défaut ; aucune option `--rendu` n'y est utile (décision 29).

**Inconnues à lever (concepteur-critique), non vérifiées au cadrage.** (1) Les propriétés de cible de Qt 6.12
(`QT_ANDROID_PACKAGE_NAME`, `QT_ANDROID_APP_NAME`, `QT_ANDROID_VERSION_NAME`, `QT_ANDROID_VERSION_CODE`) suffisent-
elles à poser l'identifiant et la version ? (2) Qt ajoute-t-il des permissions (QtNetwork, tiré par QtQml, ajoute
souvent `INTERNET`) ? Le patron évident — accepter les permissions par défaut — contredit la décision 28. Le repli,
pour les deux, est un manifeste à nous dans `android/` (étape 6), dérivé du gabarit de Qt.

**Fichiers.**
- Modifier : `CMakePresets.json`, `app/CMakeLists.txt`, `docs/verifications/2026-10-14-outillage-paquets.md`
- Créer : `scripts/paquet-android.ps1`
- Modifier si nécessaire : `scripts/dev-env-android.ps1` (chemin manquant), `packaging/qt-autorises.txt` (module
  LGPL propre à Android)
- Créer au repli seulement : `android/AndroidManifest.xml`

**Test et commande bornée.** Le test est le script de paquet (rouge tant que les presets n'existent pas) :
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/paquet-android.ps1 > .tmp/paquet-android.txt 2>&1 ; Select-String -Path .tmp/paquet-android.txt -Pattern 'permissions =|Signer #1 certificate DN|LICENCES (OK|EN ECHEC)|^APK|Exception' | ForEach-Object { $_.Line }
```
Attendu, pour chaque ABI : `… : permissions = ` (vide), `Signer #1 certificate DN: CN=Android Debug, …`,
`LICENCES OK`, `APK : …ActCraft-0.1.0-<abi>.apk` ; dernière ligne `APK OK`.

**Interdits.** Pas de `.aab`, pas de Play Store, pas de clé de release, pas de secret (décision 30). Pas de
`#ifdef` hors de `app/` et des fichiers `*_android` (DA10). Ne pas activer le banc ni les tests dans les presets
Android. Ne rien publier. Si la chaîne n'est pas installée sur le poste (`VERIF qt-android-* = ECHEC` en L1-T2) :
écrire le script et les presets, prouver par l'analyse du script
(`[System.Management.Automation.Language.Parser]::ParseFile`, 0 erreur, comme en L1-T18) et la validité JSON de
`CMakePresets.json` (`Get-Content -Raw CMakePresets.json | ConvertFrom-Json`), et laisser la construction à la CI
(L1-T19) ; le dire au rapport.

**Terminé quand.** `APK OK` (ou, en repli CI, scripts analysés sans erreur et raison au rapport) ; lignes `VERIF
apk-*` ajoutées au document ; commit fait.

**Étapes.**

- [ ] **Étape 1 : le test d'abord.** Créer `scripts/paquet-android.ps1` :

```powershell
# APK Android (décisions 27, 28, 30, 31 ; DA9) : presets android-arm64 et android-x86_64, alignement, signature
# (clé de débogage locale par défaut, clé de release seulement en paramètres), contrôles.
# Usage : pwsh -NoProfile -File ./scripts/paquet-android.ps1 [-Abi arm64-v8a|x86_64|tous]
#         [-Keystore <fichier> -KeystorePass <p> -KeyAlias <alias> -KeyPass <p>]
# Produit build/paquet/ActCraft-<version>-<abi>.apk ; local, jamais publié. Dernière ligne : « APK OK ».
param(
    [ValidateSet('arm64-v8a', 'x86_64', 'tous')][string]$Abi = 'tous',
    [string]$Keystore,
    [string]$KeystorePass,
    [string]$KeyAlias,
    [string]$KeyPass
)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
. "$PSScriptRoot/version-projet.ps1"
. "$PSScriptRoot/dev-env-android.ps1"
$version = Get-ActCraftVersion
$versionCode = Get-ActCraftVersionCode
$identifiant = 'io.github.jejeasterix.actcraft'
$sortie = Join-Path (Get-Location) 'build/paquet'
New-Item -ItemType Directory -Force $sortie | Out-Null

$outils = Get-ChildItem (Join-Path $env:ANDROID_SDK_ROOT 'build-tools') -Directory |
    Sort-Object { [version]($_.Name -replace '-.*$', '') } | Select-Object -Last 1
if (-not $outils) { throw 'build-tools Android introuvables' }
$bat = if ($IsWindows) { '.bat' } else { '' }
$exe = if ($IsWindows) { '.exe' } else { '' }
$apksigner = Join-Path $outils.FullName "apksigner$bat"
$zipalign = Join-Path $outils.FullName "zipalign$exe"
$aapt2 = Join-Path $outils.FullName "aapt2$exe"

if (-not $Keystore) {
    # Clé de débogage locale (décision 30) : jamais versionnée, jamais utilisée pour une release.
    $Keystore = Join-Path (Get-Location) 'build/android-debug.keystore'
    $KeystorePass = 'android'; $KeyAlias = 'androiddebugkey'; $KeyPass = 'android'
    if (-not (Test-Path $Keystore)) {
        & (Join-Path $env:JAVA_HOME "bin/keytool$exe") -genkeypair -keystore $Keystore -storepass android `
            -alias androiddebugkey -keypass android -keyalg RSA -keysize 2048 -validity 10000 `
            -dname 'CN=Android Debug,O=Android,C=US'
        if ($LASTEXITCODE -ne 0) { throw 'keytool en échec' }
    }
}

$abis = if ($Abi -eq 'tous') { @('arm64-v8a', 'x86_64') } else { @($Abi) }
foreach ($a in $abis) {
    $preset = if ($a -eq 'arm64-v8a') { 'android-arm64' } else { 'android-x86_64' }
    cmake --preset $preset
    if ($LASTEXITCODE -ne 0) { throw "configuration $preset en échec" }
    cmake --build --preset $preset --target apk
    if ($LASTEXITCODE -ne 0) { throw "construction $preset en échec" }
    $brut = Get-ChildItem -Recurse -Filter '*.apk' "build/$preset" |
        Where-Object { $_.FullName -match 'outputs[\\/]apk' } | Sort-Object LastWriteTime | Select-Object -Last 1
    if (-not $brut) { throw "aucun APK produit sous build/$preset" }

    $aligne = Join-Path $sortie "ActCraft-$version-$a-aligne.apk"
    $final = Join-Path $sortie "ActCraft-$version-$a.apk"
    & $zipalign -p -f 4 $brut.FullName $aligne
    if ($LASTEXITCODE -ne 0) { throw 'zipalign en échec' }
    & $apksigner sign --ks $Keystore --ks-pass "pass:$KeystorePass" --ks-key-alias $KeyAlias `
        --key-pass "pass:$KeyPass" --out $final $aligne
    if ($LASTEXITCODE -ne 0) { throw 'apksigner en échec' }
    Remove-Item -Force $aligne

    # Contrôles : identifiant, version, aucune permission (décision 28), signature, licences (décision 34).
    $badging = @(& $aapt2 dump badging $final)
    $tete = ($badging | Select-String "^package: name='([^']+)' versionCode='(\d+)' versionName='([^']+)'").Matches
    if (-not $tete -or $tete[0].Groups[1].Value -ne $identifiant) { throw "identifiant inattendu : $($badging[0])" }
    if ($tete[0].Groups[2].Value -ne "$versionCode" -or $tete[0].Groups[3].Value -ne $version) {
        throw "version inattendue : $($badging[0]) (attendu $versionCode / $version)"
    }
    $permissions = @($badging | Select-String "^uses-permission: name='([^']+)'" |
        ForEach-Object { $_.Matches[0].Groups[1].Value })
    Write-Output "$a : permissions = $($permissions -join ', ')"
    if ($permissions.Count -gt 0) { throw "permissions demandées : $($permissions -join ', ') (décision 28 : aucune)" }
    & $apksigner verify --print-certs $final
    if ($LASTEXITCODE -ne 0) { throw 'signature invalide' }
    & "$PSScriptRoot/verifier-licences.ps1" -Apk $final
    if ($LASTEXITCODE -ne 0) { throw 'licences en échec' }
    Write-Output "APK : $final"
}
Write-Output 'APK OK'
```

- [ ] **Étape 2 : le voir échouer.** Lancer la commande bornée. Attendu : `Exception` / `configuration android-arm64
en échec` (preset inconnu).

- [ ] **Étape 3 : presets.** Dans `CMakePresets.json`, ajouter à `configurePresets` (après `release`) :

```json
    {
      "name": "android-base",
      "hidden": true,
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "QT_HOST_PATH": "$env{ACTCRAFT_QT_HOST_DIR}",
        "ANDROID_SDK_ROOT": "$env{ANDROID_SDK_ROOT}",
        "ANDROID_NDK_ROOT": "$env{ANDROID_NDK_ROOT}",
        "ACTCRAFT_BUILD_TESTS": "OFF",
        "ACTCRAFT_BUILD_BENCH": "OFF"
      }
    },
    {
      "name": "android-arm64",
      "displayName": "Android arm64-v8a (APK de release)",
      "inherits": "android-base",
      "toolchainFile": "$env{ACTCRAFT_QT_ANDROID_ARM64_DIR}/lib/cmake/Qt6/qt.toolchain.cmake"
    },
    {
      "name": "android-x86_64",
      "displayName": "Android x86_64 (émulateur)",
      "inherits": "android-base",
      "toolchainFile": "$env{ACTCRAFT_QT_ANDROID_X86_64_DIR}/lib/cmake/Qt6/qt.toolchain.cmake"
    }
```
et à `buildPresets` :

```json
    { "name": "android-arm64", "configurePreset": "android-arm64" },
    { "name": "android-x86_64", "configurePreset": "android-x86_64" }
```
(Pas de preset de test : tests exclus de ces constructions.)

- [ ] **Étape 4 : propriétés de l'APK.** Dans `app/CMakeLists.txt`, à la fin :

```cmake
# APK Android (décisions 28 et 31) : identifiant, nom affiché, version. versionCode = majeur × 10000 + mineur × 100
# + correctif.
if(ANDROID)
    math(EXPR ACTCRAFT_VERSION_CODE
        "${PROJECT_VERSION_MAJOR} * 10000 + ${PROJECT_VERSION_MINOR} * 100 + ${PROJECT_VERSION_PATCH}")
    set_target_properties(actcraft PROPERTIES
        QT_ANDROID_PACKAGE_NAME io.github.jejeasterix.actcraft
        QT_ANDROID_APP_NAME ActCraft
        QT_ANDROID_VERSION_NAME ${PROJECT_VERSION}
        QT_ANDROID_VERSION_CODE ${ACTCRAFT_VERSION_CODE})
endif()
```

- [ ] **Étape 5 : construire.** Lancer la commande bornée. Trois issues :
  - `APK OK` : aller à l'étape 7 ;
  - `identifiant inattendu` ou `permissions demandées` : étape 6 (repli du manifeste) ;
  - `licences en échec` : pour chaque module refusé, vérifier sa licence (https://doc.qt.io/qt-6/qtmodules.html),
    l'ajouter à `packaging/qt-autorises.txt` s'il est sous LGPL, sinon arrêter et le dire.

- [ ] **Étape 6 (repli seulement) : manifeste à nous.** Copier le gabarit de Qt
`$env:ACTCRAFT_QT_ANDROID_ARM64_DIR/src/android/templates/AndroidManifest.xml` vers `android/AndroidManifest.xml`
(fichier de Qt sous licence BSD, pas de Promethean), puis : fixer l'attribut `package` de `<manifest>` à
`io.github.jejeasterix.actcraft` s'il existe ; supprimer la ligne du marqueur `<!-- %%INSERT_PERMISSIONS -->`
(androiddeployqt n'y insère alors aucune permission) ; ne rien changer d'autre. Dans `app/CMakeLists.txt`, ajouter à
`set_target_properties(actcraft PROPERTIES …)` la ligne
`QT_ANDROID_PACKAGE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/android`. Relancer l'étape 5 jusqu'à `APK OK`. Noter au
rapport ce qui a imposé le repli (sortie d'`aapt2 dump badging`).

- [ ] **Étape 7 : vérifications.** Ajouter au document `docs/verifications/2026-10-14-outillage-paquets.md`, section
« Résultats », les lignes (valeurs relevées dans `.tmp/paquet-android.txt`) :

```text
VERIF apk-arm64 = OK (ActCraft-0.1.0-arm64-v8a.apk, <taille> Mo)
VERIF apk-x86_64 = OK (ActCraft-0.1.0-x86_64.apk, <taille> Mo)
VERIF apk-identifiant = io.github.jejeasterix.actcraft, versionCode 100, versionName 0.1.0 (propriétés de cible | manifeste à nous)
VERIF apk-permissions = aucune
VERIF apk-signature = clé de débogage locale (CN=Android Debug)
VERIF apk-licences = LICENCES OK
```
Taille : `[math]::Round((Get-Item build/paquet/ActCraft-0.1.0-arm64-v8a.apk).Length / 1MB, 1)`.

- [ ] **Étape 8 : commit.**

```powershell
Set-Location <worktree>; git add CMakePresets.json app/CMakeLists.txt scripts/paquet-android.ps1 docs/verifications/2026-10-14-outillage-paquets.md
git -C <worktree> commit -m "Chaîne Android : presets, propriétés de l'APK, paquet signé par une clé de débogage" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```
Ajouter à `git add` les fichiers modifiés aux étapes 5 et 6 s'il y en a (`android/AndroidManifest.xml`,
`packaging/qt-autorises.txt`, `scripts/dev-env-android.ps1`).

---

<a id="tache-l1-t18"></a>
### Tâche L1-T18 — Essai de l'APK sur émulateur : `essai-android.ps1`, `lire-pixel.py` (S3)

**Objectif.** S3 : sur un émulateur déjà démarré (local si WHPX est utilisable, sinon en CI par L1-T19), installer
l'APK `x86_64`, le lancer, vérifier que le processus vit encore après 10 s, que la capture d'écran montre la palette
(contrôle de pixels sur la couleur `palette` de `ui/qml/Theme.qml`, lue dans le fichier), puis qu'un glissement d'un
doigt (`adb shell input swipe`) laisse un trait visible sur la capture suivante. `scripts/lire-pixel.py` lit la
couleur moyenne d'un carré de pixels d'un PNG (Pillow).

**Fichiers.**
- Créer : `scripts/essai-android.ps1`, `scripts/lire-pixel.py`
- Modifier : `docs/verifications/2026-10-14-outillage-paquets.md`

**Test et commande bornée.** Toujours : l'essai de `lire-pixel.py` sur une image fabriquée, et l'analyse de
`essai-android.ps1` :
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; python -c "from PIL import Image; im = Image.new('RGB', (40, 30), (255, 255, 255)); im.paste((43, 47, 54), (30, 0, 40, 30)); im.save('.tmp/essai-pixels.png')"; python scripts/lire-pixel.py .tmp/essai-pixels.png -3 15; python scripts/lire-pixel.py .tmp/essai-pixels.png 10 15; python scripts/lire-pixel.py .tmp/essai-pixels.png --taille
Set-Location <worktree>; $jetons = $null; $erreurs = $null; [void][System.Management.Automation.Language.Parser]::ParseFile((Resolve-Path scripts/essai-android.ps1).Path, [ref]$jetons, [ref]$erreurs); Write-Output "erreurs d'analyse : $($erreurs.Count)"
```
Attendu : `43 47 54`, `255 255 255`, `40 30`, puis `erreurs d'analyse : 0`. Avec WHPX utilisable (`VERIF whpx =
utilisable`) : en plus, l'essai sur l'émulateur local (étape 5), dernière ligne `ANDROID OK`.

**Interdits.** Pas de `taskkill` : l'émulateur se ferme par `adb emu kill`. Aucune capture de l'émulateur commitée
(`.tmp/` seulement). Ne pas modifier l'APK ni `Theme.qml`.

**Terminé quand.** Preuves vertes (et `ANDROID OK` en local si WHPX) ; ligne `VERIF s3-local` ajoutée ; commit
fait.

**Étapes.**

- [ ] **Étape 1 : écrire `scripts/lire-pixel.py`.**

```python
"""Couleur moyenne d'un carré de pixels d'une capture PNG (essais Android, scénario S3).

Usage : python scripts/lire-pixel.py <capture.png> <x> <y> [rayon]   → « R G B »
        python scripts/lire-pixel.py <capture.png> --taille          → « largeur hauteur »
Moyenne du carré de côté 2 × rayon + 1 (rayon 2 par défaut) ; coordonnées en pixels de l'appareil, négatives
comptées depuis le bord droit ou le bord bas.
"""
import sys

from PIL import Image


def main() -> int:
    if len(sys.argv) < 3:
        print(__doc__, file=sys.stderr)
        return 2
    image = Image.open(sys.argv[1]).convert("RGB")
    width, height = image.size
    if sys.argv[2] == "--taille":
        print(f"{width} {height}")
        return 0
    if len(sys.argv) < 4:
        print(__doc__, file=sys.stderr)
        return 2
    x, y = int(sys.argv[2]), int(sys.argv[3])
    radius = int(sys.argv[4]) if len(sys.argv) > 4 else 2
    x = x + width if x < 0 else x
    y = y + height if y < 0 else y
    total = [0, 0, 0]
    count = 0
    for yy in range(max(0, y - radius), min(height, y + radius + 1)):
        for xx in range(max(0, x - radius), min(width, x + radius + 1)):
            pixel = image.getpixel((xx, yy))
            for channel in range(3):
                total[channel] += pixel[channel]
            count += 1
    if count == 0:
        print("hors image", file=sys.stderr)
        return 1
    print(" ".join(str(round(value / count)) for value in total))
    return 0


if __name__ == "__main__":
    sys.exit(main())
```

- [ ] **Étape 2 : écrire `scripts/essai-android.ps1`.**

```powershell
# Essai de l'APK sur un émulateur déjà démarré (scénario S3) : installation, lancement, processus vivant après 10 s,
# palette visible sur la capture, trait d'un doigt visible sur la capture suivante.
# Usage : pwsh -NoProfile -File ./scripts/essai-android.ps1 -Apk <fichier.apk> [-Sortie <dossier>]
# adb doit être dans le PATH (dev-env-android.ps1, ou l'émulateur de la CI). Captures sous -Sortie (.tmp/android).
# Dernière ligne et code : « ANDROID OK » (0) ou « ANDROID EN ECHEC : <raison> » (1).
param([Parameter(Mandatory)][string]$Apk, [string]$Sortie = '.tmp/android')
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
$paquet = 'io.github.jejeasterix.actcraft'
$python = if (Get-Command python3 -ErrorAction SilentlyContinue) { 'python3' } else { 'python' }

function Echec([string]$Raison) {
    Write-Output "ANDROID EN ECHEC : $Raison"
    exit 1
}
function Pixel([string]$Capture, [int]$X, [int]$Y) {
    $texte = & $python scripts/lire-pixel.py $Capture $X $Y
    if ($LASTEXITCODE -ne 0) { Echec "lecture du pixel ($X, $Y)" }
    return @("$texte".Trim().Split(' ') | ForEach-Object { [int]$_ })
}
function Proche($A, $B, [int]$Tolerance) {
    for ($i = 0; $i -lt 3; $i++) { if ([math]::Abs($A[$i] - $B[$i]) -gt $Tolerance) { return $false } }
    return $true
}
function Capture([string]$Nom) {
    $fichier = Join-Path $Sortie "$Nom.png"
    & adb shell screencap -p "/sdcard/actcraft-$Nom.png" | Out-Null
    & adb pull "/sdcard/actcraft-$Nom.png" $fichier | Out-Null
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path $fichier)) { Echec "capture $Nom" }
    return $fichier
}

# Repère : couleur de la palette, lue dans le thème (décision 23).
$theme = Get-Content -Raw ui/qml/Theme.qml
if ($theme -notmatch 'property color palette:\s*"#([0-9A-Fa-f]{6})"') { Echec 'couleur « palette » introuvable dans ui/qml/Theme.qml' }
$hex = $Matches[1]
$fondPalette = @([Convert]::ToInt32($hex.Substring(0, 2), 16), [Convert]::ToInt32($hex.Substring(2, 2), 16),
    [Convert]::ToInt32($hex.Substring(4, 2), 16))

New-Item -ItemType Directory -Force $Sortie | Out-Null
& adb wait-for-device
$limite = (Get-Date).AddMinutes(6)
while ("$(& adb shell getprop sys.boot_completed)".Trim() -ne '1') {
    if ((Get-Date) -gt $limite) { Echec 'émulateur non démarré après 6 min' }
    Start-Sleep -Seconds 2
}
& adb shell input keyevent 82 | Out-Null # déverrouille l'écran
& adb install -r (Resolve-Path $Apk).Path
if ($LASTEXITCODE -ne 0) { Echec 'adb install' }
& adb shell monkey -p $paquet -c android.intent.category.LAUNCHER 1 | Out-Null
Start-Sleep -Seconds 10
$processus = "$(& adb shell pidof $paquet)".Trim()
if (-not $processus) { Echec 'processus absent 10 s après le lancement' }
Write-Output "processus : $processus"

$avant = Capture 'avant'
$taille = ("$(& $python scripts/lire-pixel.py $avant --taille)".Trim()).Split(' ')
$largeur = [int]$taille[0]; $hauteur = [int]$taille[1]
Write-Output "écran : $largeur × $hauteur"
foreach ($f in 0.25, 0.5, 0.75) {
    $y = [int]($hauteur * $f)
    $lu = Pixel $avant ($largeur - 3) $y
    if (-not (Proche $lu $fondPalette 16)) { Echec "palette absente au bord droit, y = $y (lu $($lu -join ' '), attendu $($fondPalette -join ' '))" }
}
Write-Output 'palette : visible au bord droit'

# Trait d'un doigt au milieu de la page (blanche avant le trait).
$y = [int]($hauteur * 0.5)
$x0 = [int]($largeur * 0.30); $x1 = [int]($largeur * 0.55); $xm = [int](($x0 + $x1) / 2)
$blanc = Pixel $avant $xm $y
if (-not (Proche $blanc @(255, 255, 255) 12)) { Echec "page non blanche avant le trait (lu $($blanc -join ' '))" }
& adb shell input swipe $x0 $y $x1 $y 600 | Out-Null
Start-Sleep -Seconds 2
$apres = Capture 'apres'
$trait = Pixel $apres $xm $y
Write-Output "trait : lu $($trait -join ' ')"
if (-not ($trait[0] -lt 100 -and $trait[1] -lt 100 -and $trait[2] -lt 100)) { Echec 'trait absent après le glissement' }
if ("$(& adb shell pidof $paquet)".Trim() -ne $processus) { Echec 'le processus a redémarré ou s''est arrêté' }
Write-Output 'ANDROID OK'
```

- [ ] **Étape 3 : le voir passer (sans émulateur).** Lancer les deux commandes bornées. Attendu : `43 47 54`,
`255 255 255`, `40 30`, `erreurs d'analyse : 0`.

- [ ] **Étape 4 : décider de l'essai local.** Lire `VERIF whpx` dans
`docs/verifications/2026-10-14-outillage-paquets.md`. `non utilisable` : sauter l'étape 5, écrire à l'étape 6
`VERIF s3-local = non joué (WHPX) — S3 prouvé par la CI (L1-T19)`.

- [ ] **Étape 5 (WHPX utilisable) : essai local.** Construire l'APK si besoin, démarrer l'émulateur sans fenêtre,
jouer l'essai, fermer l'émulateur :

```powershell
Set-Location <worktree>; pwsh -NoProfile -Command ". ./scripts/dev-env-android.ps1; if (-not (Test-Path build/paquet/ActCraft-0.1.0-x86_64.apk)) { ./scripts/paquet-android.ps1 -Abi x86_64 | Select-Object -Last 1 }; `$emu = Start-Process -FilePath emulator -ArgumentList '-avd','actcraft-x86_64','-no-window','-no-audio','-no-snapshot','-no-boot-anim','-gpu','swiftshader_indirect' -PassThru; ./scripts/essai-android.ps1 -Apk build/paquet/ActCraft-0.1.0-x86_64.apk -Sortie .tmp/android; adb emu kill" > .tmp/essai-android.txt 2>&1 ; Select-String -Path .tmp/essai-android.txt -Pattern '^(processus|écran|palette|trait|ANDROID)' | ForEach-Object { $_.Line }
```
Attendu : `processus : <pid>`, `écran : …`, `palette : visible au bord droit`, `trait : lu …` (sombre),
`ANDROID OK`. Regarder `.tmp/android/avant.png` et `apres.png` (outil Read) et les décrire au rapport.

- [ ] **Étape 6 : vérification.** Ajouter au document, section « Résultats » :
`VERIF s3-local = ANDROID OK (émulateur actcraft-x86_64, <écran>)` ou
`VERIF s3-local = non joué (WHPX) — S3 prouvé par la CI (L1-T19)`.

- [ ] **Étape 7 : commit.**

```powershell
Set-Location <worktree>; git add scripts/essai-android.ps1 scripts/lire-pixel.py docs/verifications/2026-10-14-outillage-paquets.md
git -C <worktree> commit -m "Essai S3 de l'APK sur émulateur (palette visible, trait d'un doigt)" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

<a id="tache-l1-t19"></a>
### Tâche L1-T19 — CI : jobs `paquet-windows` (S1) et `android` (S3, KVM), branches `essai-ci/**`

**Objectif.** DA9, décisions 27 et 33 : à chaque PR (et sur `main`), la CI **construit et essaie** les paquets
**sans téléverser d'artefact** : job `paquet-windows` (`windows-latest` : Qt, MSVC, Inno Setup, `paquet.ps1`, essai S1
`essai-installateur.ps1 -Rendu logiciel`) ; job `android` (`ubuntu-latest` : JDK 17, Qt hôte et Qt Android, NDK des
versions de `packaging/android/versions.env`, `paquet-android.ps1` pour les deux ABI, KVM, émulateur
`reactivecircus/android-emulator-runner`, essai S3 `essai-android.ps1` sur l'APK `x86_64`). Un push sur une branche
`essai-ci/<sujet>` lance aussi la CI, pour prouver une tâche de CI sans ouvrir de PR (la PR du lot est créée une
seule fois, à la clôture) ; ces branches se suppriment après le run.

**Porte « installation ».** Si l'émulateur échoue en CI (démarrage, KVM) **et** que `VERIF whpx = non utilisable` :
ni l'émulateur local ni celui de la CI ne fonctionnent ; s'arrêter, rapporter le journal du run et la ligne `VERIF`,
et demander la porte « installation » à l'orchestrateur (activer WHPX demande l'administrateur et un redémarrage,
que seul Jérôme peut faire).

**Fichiers.** Modifier : `.github/workflows/ci.yml`. Corrections propres à la CI permises (et seulement elles) :
`scripts/paquet.ps1`, `scripts/essai-installateur.ps1`, `scripts/paquet-android.ps1`, `scripts/essai-android.ps1`,
`scripts/dev-env-android.ps1`, `packaging/android/versions.env`. Si L1-T17 a prouvé par analyse (chaîne absente du
poste) ou si L1-T18 n'a pas joué S3 en local (pas de WHPX), la première vraie construction Android et le premier
vrai S3 ont lieu ici : sont alors permis aussi `CMakePresets.json`, `app/CMakeLists.txt`,
`android/AndroidManifest.xml` (repli de L1-T17, étape 6), `packaging/qt-autorises.txt` (module LGPL seulement, même
règle que L1-T16) et `scripts/lire-pixel.py`. Chaque correction est nommée au rapport avec la ligne du journal qui
l'a imposée.

**Publication sur le dépôt public.** Le push d'une branche `essai-ci/…` publie tout le lot sur un dépôt **public**
avant la relecture de fin de lot ; supprimer la branche n'efface ni les commits ni les journaux. Le contrôle de
l'étape 2 (propriété intellectuelle) précède donc **tout** push, et son résultat entre dans la preuve.

**Test et commande bornée.** Run réel de la CI sur la branche `essai-ci/L1-T19` (jamais de tag). Un run dure plus
que le plafond de 10 min d'un appel d'outil : on attend que le run apparaisse, puis on le suit par appels courts.
```powershell
Set-Location <worktree>; git push origin HEAD:refs/heads/essai-ci/L1-T19
Set-Location <worktree>; $sha = git rev-parse HEAD; $id = $null; $fin = (Get-Date).AddMinutes(3); while (-not $id -and (Get-Date) -lt $fin) { Start-Sleep -Seconds 10; $id = (gh run list --branch essai-ci/L1-T19 --workflow ci.yml --commit $sha --limit 1 --json databaseId | ConvertFrom-Json | Select-Object -First 1).databaseId }; Write-Output "run : $id"
Set-Location <worktree>; $fin = (Get-Date).AddMinutes(8); do { Start-Sleep -Seconds 30; $r = gh run view <id> --json status,conclusion | ConvertFrom-Json } until ($r.status -eq 'completed' -or (Get-Date) -gt $fin); Write-Output "état : $($r.status) / $($r.conclusion)"
Set-Location <worktree>; gh run view <id> --json jobs --jq '.jobs[] | "\(.name) : \(.conclusion)"'; gh run view <id> --log | Select-String 'INSTALLATEUR OK|APK OK|ANDROID OK' | Select-Object -First 3 | ForEach-Object { $_.Line.Substring([Math]::Max(0, $_.Line.Length - 60)) }
```
La deuxième commande donne l'identifiant (`run : <id>` ; vide après 3 min : relire `gh run list --branch
essai-ci/L1-T19` et le rapporter). La troisième se **relance** tant que l'état n'est pas `completed` (chaque appel
dure 8 min au plus), **8 fois au plus** (environ 64 min) ; au-delà, s'arrêter et rapporter
`gh run view <id> --json jobs`. On peut aussi lancer `gh run watch <id> --exit-status --interval 60 >
.tmp/ci-run.txt 2>&1` en arrière-plan (`run_in_background`), avec la même limite, et attendre sa notification. Attendu : `état : completed / success` ; tous
les jobs `success` (`format`, `build (windows-latest…)`, `build (macos-latest…)`, `paquet-windows`, `android`) ; trois
lignes se terminant par `INSTALLATEUR OK`, `APK OK`, `ANDROID OK`. Puis suppression de la branche :
`git -C <worktree> push origin --delete essai-ci/L1-T19`.

**Interdits.** Aucun `actions/upload-artifact` dans `ci.yml` (pas de distribution avant le go). Jamais de tag `v*`,
jamais de push sur `main`. Pas de secret. Ne pas désactiver un job ni marquer `continue-on-error` un essai S1 ou S3
pour obtenir le vert.

**Terminé quand.** Run vert avec les trois lignes ; branche `essai-ci/L1-T19` supprimée ; commit fait (le commit
précède le push : c'est lui que la CI essaie).

**Étapes.**

- [ ] **Étape 1 : écrire `.github/workflows/ci.yml`.** Remplacer le fichier entier par :

```yaml
name: CI

on:
  push:
    branches:
      - main
      - "essai-ci/**"
      - "!essai-ci/release-**"
  pull_request:

concurrency:
  group: ci-${{ github.ref }}
  cancel-in-progress: true

env:
  QT_VERSION: "6.12.0"

jobs:
  format:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Vérifier le format C++
        run: |
          pip install clang-format==23.1.3
          git ls-files '*.cpp' '*.h' '*.hpp' | xargs --no-run-if-empty clang-format --dry-run --Werror

  build:
    strategy:
      fail-fast: false
      matrix:
        include:
          - os: windows-latest
            qt_arch: win64_msvc2022_64
          - os: macos-latest
            qt_arch: clang_64
    runs-on: ${{ matrix.os }}
    steps:
      - uses: actions/checkout@v4

      - uses: actions/setup-python@v5
        with:
          python-version: "3.12"

      # aqtinstall 3.3.0 ne sait pas installer Qt 6.12 (nouveau découpage du dépôt Qt) :
      # on prend la version de développement, vérifiée le 2026-10-13.
      - name: Installer Ninja et aqtinstall
        run: python -m pip install ninja "git+https://github.com/miurahr/aqtinstall"

      - name: Installer Qt
        shell: bash
        run: |
          host=$([ "${{ runner.os }}" = "Windows" ] && echo windows || echo mac)
          python -m aqt install-qt "$host" desktop "$QT_VERSION" ${{ matrix.qt_arch }} -m qtshadertools qtimageformats --outputdir "$RUNNER_TEMP/Qt"
          dir=$(ls -d "$RUNNER_TEMP"/Qt/"$QT_VERSION"/*/ | head -1)
          echo "ACTCRAFT_QT_DIR=${dir%/}" >> "$GITHUB_ENV"
          echo "${dir%/}/bin" >> "$GITHUB_PATH"

      - uses: ilammy/msvc-dev-cmd@v1
        if: runner.os == 'Windows'

      - name: Configurer
        run: cmake --preset debug

      - name: Compiler
        run: cmake --build --preset debug

      - name: Tester
        run: ctest --preset debug

      # N1-8 : premier contact avec Metal ; informatif, ne bloque jamais la CI.
      - name: Banc sur la plateforme native (informatif)
        if: runner.os == 'macOS'
        continue-on-error: true
        run: |
          set +e
          if [ ! -x ./build/debug/actcraft-bench ]; then
            echo "actcraft-bench introuvable dans build/debug :"; ls build/debug; exit 1
          fi
          ./build/debug/actcraft-bench --scenario court --rendu auto --taille 1280x800 --sortie "$RUNNER_TEMP/bench-macos.json"
          echo "code de sortie du banc : $?"
          python3 -c "import json,sys; d=json.load(open(sys.argv[1])); print('graphics_api =', d['graphics_api'])" "$RUNNER_TEMP/bench-macos.json"

  # S1 : installateur construit et essayé, jamais téléversé (DA9).
  paquet-windows:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4

      - uses: actions/setup-python@v5
        with:
          python-version: "3.12"

      - name: Installer Ninja et aqtinstall
        run: python -m pip install ninja "git+https://github.com/miurahr/aqtinstall"

      - name: Installer Qt
        shell: bash
        run: |
          python -m aqt install-qt windows desktop "$QT_VERSION" win64_msvc2022_64 -m qtshadertools qtimageformats --outputdir "$RUNNER_TEMP/Qt"
          dir=$(ls -d "$RUNNER_TEMP"/Qt/"$QT_VERSION"/*/ | head -1)
          echo "ACTCRAFT_QT_DIR=${dir%/}" >> "$GITHUB_ENV"
          echo "${dir%/}/bin" >> "$GITHUB_PATH"

      - uses: ilammy/msvc-dev-cmd@v1

      - name: Installer Inno Setup
        shell: pwsh
        run: |
          $iscc = Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'
          if (-not (Test-Path $iscc)) { choco install innosetup -y --no-progress }
          if (-not (Test-Path $iscc)) { throw "ISCC.exe introuvable après installation" }
          "ACTCRAFT_ISCC=$iscc" >> $env:GITHUB_ENV

      - name: Construire l'installateur
        shell: pwsh
        run: pwsh -NoProfile -File ./scripts/paquet.ps1

      - name: Essai de l'installateur (S1)
        shell: pwsh
        run: pwsh -NoProfile -File ./scripts/essai-installateur.ps1 -Rendu logiciel

  # S3 : APK arm64-v8a et x86_64 construits, x86_64 essayé sur l'émulateur (KVM), jamais téléversés (DA9).
  android:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - uses: actions/setup-python@v5
        with:
          python-version: "3.12"

      - uses: actions/setup-java@v4
        with:
          distribution: temurin
          java-version: "17"

      - name: Versions de la chaîne Android
        run: |
          cat packaging/android/versions.env >> "$GITHUB_ENV"
          echo "ACTCRAFT_VERSION=$(pwsh -NoProfile -Command '. ./scripts/version-projet.ps1; Get-ActCraftVersion')" >> "$GITHUB_ENV"

      - name: Installer Ninja, aqtinstall et Pillow
        run: python -m pip install ninja pillow "git+https://github.com/miurahr/aqtinstall"

      - name: Installer Qt (hôte et Android)
        run: |
          python -m aqt install-qt linux desktop "$QT_VERSION" linux_gcc_64 -m qtshadertools qtimageformats --outputdir "$RUNNER_TEMP/Qt"
          python -m aqt install-qt all_os android "$QT_VERSION" android_arm64_v8a -m qtshadertools qtimageformats --outputdir "$RUNNER_TEMP/Qt"
          python -m aqt install-qt all_os android "$QT_VERSION" android_x86_64 -m qtshadertools qtimageformats --outputdir "$RUNNER_TEMP/Qt"
          echo "ACTCRAFT_QT_HOST_DIR=$RUNNER_TEMP/Qt/$QT_VERSION/gcc_64" >> "$GITHUB_ENV"
          echo "ACTCRAFT_QT_ANDROID_ARM64_DIR=$RUNNER_TEMP/Qt/$QT_VERSION/android_arm64_v8a" >> "$GITHUB_ENV"
          echo "ACTCRAFT_QT_ANDROID_X86_64_DIR=$RUNNER_TEMP/Qt/$QT_VERSION/android_x86_64" >> "$GITHUB_ENV"

      - name: Installer le NDK, la plateforme et les build-tools
        run: |
          sdkmanager="$ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager"
          yes | "$sdkmanager" --licenses > /dev/null
          "$sdkmanager" "ndk;$ANDROID_NDK_VERSION" "build-tools;$ANDROID_BUILD_TOOLS" "platforms;android-$ANDROID_PLATFORM"
          echo "ANDROID_NDK_ROOT=$ANDROID_SDK_ROOT/ndk/$ANDROID_NDK_VERSION" >> "$GITHUB_ENV"

      - name: Construire les APK
        run: pwsh -NoProfile -File ./scripts/paquet-android.ps1

      - name: Activer KVM
        run: |
          echo 'KERNEL=="kvm", GROUP="kvm", MODE="0666", OPTIONS+="static_node=kvm"' | sudo tee /etc/udev/rules.d/99-kvm4all.rules
          sudo udevadm control --reload-rules
          sudo udevadm trigger --name-match=kvm

      - name: Essai sur l'émulateur (S3)
        uses: reactivecircus/android-emulator-runner@v2
        with:
          api-level: ${{ env.ANDROID_EMULATOR_API }}
          arch: x86_64
          target: google_apis
          profile: ${{ env.ANDROID_AVD_PROFILE }}
          disable-animations: true
          emulator-options: -no-window -gpu swiftshader_indirect -noaudio -no-boot-anim -no-snapshot
          script: pwsh -NoProfile -File ./scripts/essai-android.ps1 -Apk build/paquet/ActCraft-${{ env.ACTCRAFT_VERSION }}-x86_64.apk -Sortie build/android-essai
```

- [ ] **Étape 2 : commit (avant le push : c'est ce commit que la CI essaie).**

```powershell
Set-Location <worktree>; git add .github/workflows/ci.yml
git -C <worktree> commit -m "CI : installateur (S1) et APK sur émulateur (S3) construits et essayés, sans téléversement" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

- [ ] **Étape 2 bis : contrôle avant publication (propriété intellectuelle).** Avant **tout** push (et avant
chaque nouveau push après une correction) :

```powershell
Set-Location <worktree>; git fetch origin main; git diff --name-only --diff-filter=AM origin/main..HEAD | Where-Object { Test-Path $_ } | ForEach-Object { Select-String -Path $_ -Pattern 'Promethean|ActivInspire|\bActiv\b|ExpressPoll|Planet' } | ForEach-Object { "$($_.Path | Resolve-Path -Relative):$($_.LineNumber): $($_.Line.Trim())" }
Set-Location <worktree>; git diff --name-only --diff-filter=A origin/main..HEAD | Where-Object { $_ -match '\.(png|jpe?g|gif|bmp|ico|svg|flipchart|flp|as4|wav|mp3|mp4|ttf|otf)$' -or $_ -match '^(build|\.tmp|bench/results)/' -or $_ -match 'samples' }
```
Attendu : la première commande ne liste que des **mentions écrites par nous** (le nom d'ActivInspire cité pour
l'interopérabilité ou pour dire ce qu'on ne reprend pas : cadrage, plan, direction visuelle, commentaire du `.iss`)
— aucune phrase, donnée ou libellé copiés de l'aide, des binaires ou de `docs/re/` de l'ancien dépôt ; la deuxième ne
liste que les 10 icônes `ui/icons/*.svg` de L1-T7, chacune relue (outil Read) : chemins géométriques simples à une
couleur, `viewBox` 24 × 24, aucun commentaire ni métadonnée d'origine. Recopier les deux sorties et le verdict
(une ligne par fichier) dans la preuve. Tout contenu de Promethean, toute capture, tout fichier de `samples/`,
`build/`, `.tmp/` ou `bench/results/` **bloque le push** : s'arrêter et le rapporter.

- [ ] **Étape 3 : run réel.** Lancer les trois commandes bornées (push de `essai-ci/L1-T19`, identifiant du run,
suivi par appels de 8 min au plus, puis lecture). Rouge : un job en `failure`. Lire son journal :
`gh run view <id> --log-failed | Select-Object -Last 60`. Corriger **la cause** dans l'un des fichiers permis
(chemins Linux, variable d'environnement, version d'outil), commiter (message : « CI : <correction> »), repousser
(`git push origin HEAD:refs/heads/essai-ci/L1-T19`), relire. Trois essais au plus ; au-delà, rapport avec le
journal (et porte « installation » si c'est l'émulateur et que WHPX n'est pas utilisable).

- [ ] **Étape 4 : vert.** Attendu : `code 0`, jobs `success`, lignes `INSTALLATEUR OK`, `APK OK`, `ANDROID OK`.
Ajouter au rapport l'URL du run (`gh run view $id --json url --jq .url`).

- [ ] **Étape 5 : nettoyer.** `git -C <worktree> push origin --delete essai-ci/L1-T19`. Vérifier :
`git -C <worktree> ls-remote --heads origin "essai-ci/*"` ne liste plus cette branche.

---

<a id="tache-l1-t20"></a>
### Tâche L1-T20 — `release.yml` (brouillon, essai / réel, échec sans secrets), `CHANGELOG.md`, commandes réelles au profil

**Objectif.** Décisions 30, 31, 32, 38 (fin), DA9 :
- `.github/workflows/release.yml` : un tag `v<version>` lance le mode **réel** (refus si le tag ne correspond pas à
  la version du projet ; secrets de signature obligatoires ; installateur + APK `arm64-v8a` signé par la clé de
  release ; `SHA256SUMS.txt` ; notes tirées de `CHANGELOG.md` ; **brouillon** de release GitHub, jamais publié par
  le workflow). `workflow_dispatch` a deux modes : `essai` (tout est produit, APK signé par une clé de débogage et
  annoncé « non publiable », aucune release, aucun téléversement) et `reel` (exige les secrets ; sans eux, échoue
  avec le même message, avant toute construction, sans rien créer). Un push sur une branche
  `essai-ci/release-<sujet>` joue le mode `essai` (preuve sans tag, `workflow_dispatch` n'étant utilisable qu'une
  fois le workflow sur `main`) ; en mode `essai`, un job prouve que le mode réel, privé de secrets, échoue avec son
  message.
- `CHANGELOG.md` en français (section 0.1.0), `scripts/notes-version.ps1`, `scripts/verifier-secrets-release.ps1`.
- Profil : forme bornée de la preuve visuelle et commandes réelles des paquets et de la release (décision 38 :
  « une tâche de fin de lot 1 y ajoute les commandes réelles »).

La clé de release n'existe pas encore (créée au go de la première release, décision 30) : ce lot ne peut prouver que
le mode `essai` et l'échec du mode réel sans secrets.

**Fichiers.**
- Créer : `.github/workflows/release.yml`, `CHANGELOG.md`, `scripts/verifier-secrets-release.ps1`,
  `scripts/notes-version.ps1`
- Modifier : `.claude/chantier.md`

**Test et commande bornée.** Rouge puis vert, localement : les deux scripts (sans secrets → `SECRETS ABSENTS`, code
1 ; avec des valeurs factices → `SECRETS OK` ; notes de 0.1.0 → `NOTES OK`). Puis run réel de `release.yml` en mode
`essai` sur `essai-ci/release-L1-T20`, précédé du contrôle avant publication (étape 7 bis). Un run dure plus que le
plafond de 10 min d'un appel d'outil : attendre qu'il apparaisse, puis le suivre par appels courts.
```powershell
Set-Location <worktree>; git push origin HEAD:refs/heads/essai-ci/release-L1-T20
Set-Location <worktree>; $sha = git rev-parse HEAD; $id = $null; $fin = (Get-Date).AddMinutes(3); while (-not $id -and (Get-Date) -lt $fin) { Start-Sleep -Seconds 10; $id = (gh run list --branch essai-ci/release-L1-T20 --commit $sha --limit 1 --json databaseId | ConvertFrom-Json | Select-Object -First 1).databaseId }; Write-Output "run : $id"
Set-Location <worktree>; $fin = (Get-Date).AddMinutes(8); do { Start-Sleep -Seconds 30; $r = gh run view <id> --json status,conclusion | ConvertFrom-Json } until ($r.status -eq 'completed' -or (Get-Date) -gt $fin); Write-Output "état : $($r.status) / $($r.conclusion)"
Set-Location <worktree>; gh run view <id> --json jobs --jq '.jobs[] | "\(.name) : \(.conclusion)"'; gh run view <id> --log | Select-String 'CONTROLE OK|NOTES OK|INSTALLATEUR OK|APK OK|NON PUBLIABLE' | ForEach-Object { $_.Line.Substring([Math]::Max(0, $_.Line.Length - 70)) }; gh release list --limit 5
```
La troisième commande se relance tant que l'état n'est pas `completed`, **8 fois au plus** (environ 64 min) ;
au-delà, s'arrêter et rapporter `gh run view <id> --json jobs` (ou `gh run watch <id> --exit-status --interval 60`,
lancé en arrière-plan, avec la même limite). Attendu : `état : completed / success` ; jobs `mode`, `controle-mode-reel`, `windows`, `android` en `success`, `secrets`, `release` et `nettoyer`
en `skipped` ; lignes `CONTROLE OK`, `NOTES OK`, `INSTALLATEUR OK`, `APK OK`, `APK NON PUBLIABLE` ; `gh release
list` ne montre **aucune** release nouvelle. Puis `git -C <worktree> push origin --delete essai-ci/release-L1-T20`.

**Interdits.** Jamais de tag `v*`, jamais `gh release create` / `edit` à la main, jamais de secret créé ou écrit
(la clé de release est créée au go, décision 30). Pas de téléversement d'artefact en mode `essai`. Le workflow ne
publie jamais le brouillon (la publication est l'action finale, go écrit de Jérôme).

**Terminé quand.** Scripts verts localement ; run `essai` vert, aucune release créée, branche supprimée ; profil à
jour, bloc `action-finale` inchangé (étape 9) ; contrôle avant publication recopié dans la preuve ; commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests (commandes) et les voir échouer.**

```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/verifier-secrets-release.ps1; Write-Output "code $LASTEXITCODE"
Set-Location <worktree>; pwsh -NoProfile -Command "`$env:ACTCRAFT_KEYSTORE_BASE64='eA=='; `$env:ACTCRAFT_KEYSTORE_PASSWORD='x'; `$env:ACTCRAFT_KEY_ALIAS='x'; `$env:ACTCRAFT_KEY_PASSWORD='x'; ./scripts/verifier-secrets-release.ps1"; Write-Output "code $LASTEXITCODE"
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/notes-version.ps1 -Version 0.1.0 -Sortie .tmp/notes.md; Write-Output "code $LASTEXITCODE"
```
Attendu avant l'écriture des scripts : trois erreurs « fichier introuvable ». Attendu après (étape 4) :
`SECRETS ABSENTS : …` puis `code 1` ; `SECRETS OK` puis `code 0` ; `NOTES OK : .tmp/notes.md` puis `code 0`.

- [ ] **Étape 2 : `scripts/verifier-secrets-release.ps1`.**

```powershell
# Mode « reel » de release.yml (décision 32) : échoue, avant toute construction et sans rien créer, si un secret de
# signature manque. Jamais d'APK de débogage dans une release.
# Usage : pwsh -NoProfile -File ./scripts/verifier-secrets-release.ps1
# Dernière ligne et code : « SECRETS OK » (0) ou « SECRETS ABSENTS : <noms> … » (1).
$noms = 'ACTCRAFT_KEYSTORE_BASE64', 'ACTCRAFT_KEYSTORE_PASSWORD', 'ACTCRAFT_KEY_ALIAS', 'ACTCRAFT_KEY_PASSWORD'
$manquants = @($noms | Where-Object { -not [Environment]::GetEnvironmentVariable($_) })
if ($manquants.Count -gt 0) {
    Write-Output ("SECRETS ABSENTS : $($manquants -join ', ') — release impossible : un APK de release exige la clé " +
        'de release (créée au go de Jérôme, décision 30) ; aucune release créée.')
    exit 1
}
Write-Output 'SECRETS OK'
```

- [ ] **Étape 3 : `scripts/notes-version.ps1` et `CHANGELOG.md`.**

```powershell
# Notes d'une version tirées de CHANGELOG.md (section « ## <version> … », jusqu'à la section suivante).
# Usage : pwsh -NoProfile -File ./scripts/notes-version.ps1 -Version <x.y.z> -Sortie <fichier>
# Dernière ligne et code : « NOTES OK : <fichier> » (0) ou « NOTES ABSENTES : <raison> » (1).
param([Parameter(Mandatory)][string]$Version, [Parameter(Mandatory)][string]$Sortie)
$ErrorActionPreference = 'Stop'
$lignes = @(Get-Content (Join-Path $PSScriptRoot '../CHANGELOG.md'))
$debut = -1
for ($i = 0; $i -lt $lignes.Count; $i++) {
    if ($lignes[$i] -match "^## $([regex]::Escape($Version))(\s|$)") { $debut = $i; break }
}
if ($debut -lt 0) { Write-Output "NOTES ABSENTES : pas de section « ## $Version » dans CHANGELOG.md"; exit 1 }
$fin = $lignes.Count
for ($j = $debut + 1; $j -lt $lignes.Count; $j++) {
    if ($lignes[$j] -match '^## ') { $fin = $j; break }
}
if ($fin -le $debut + 1) { Write-Output "NOTES ABSENTES : section « ## $Version » vide"; exit 1 }
$texte = ($lignes[($debut + 1)..($fin - 1)] -join "`n").Trim()
if (-not $texte) { Write-Output "NOTES ABSENTES : section « ## $Version » vide"; exit 1 }
[IO.File]::WriteAllText($Sortie, "$texte`n")
Write-Output "NOTES OK : $Sortie"
```

```markdown
# Journal des versions

Les versions d'ActCraft, de la plus récente à la plus ancienne. Une section par version, rédigée à la clôture de
chaque lot (décision 31 du cadrage `docs/specs/2026-10-14-livraison-complete-cadrage.md`) ; la section sert de notes
à la release correspondante.

## 0.1.0 — à paraître

Premier socle utilisable : une interface, le stylo en couleurs, plusieurs pages, et les deux paquets.

- Fenêtre principale : menus Fichier, Affichage, Outils et Aide ; indicateur « page n / N », échelle et plein
  écran (F5) dans la ligne supérieure ; palette d'outils à droite ; pas de barre d'état.
- Stylo : 16 couleurs, 4 épaisseurs et une réglette, indicateur de largeur ; tracé au stylet, à la souris ou d'un
  doigt.
- Pages : « Page suivante » ajoute une page vierge après la dernière ; « Page précédente ».
- « À propos » : version, licence MIT, notices des composants tiers ; `actcraft --version`.
- Installateur Windows sans droits d'administrateur ; APK Android (arm64-v8a).
```
(La clôture du lot remplace « à paraître » par la date et complète la section.)

- [ ] **Étape 4 : les voir passer.** Relancer les trois commandes de l'étape 1. Attendu : `SECRETS ABSENTS`,
`code 1` ; `SECRETS OK`, `code 0` ; `NOTES OK`, `code 0`, et `.tmp/notes.md` commence par « Premier socle ».

- [ ] **Étape 5 : `.github/workflows/release.yml`.**

```yaml
name: Release

# Décision 32 du cadrage livraison-complete. Tag v<version> : mode « reel » (brouillon de release, clé de release
# obligatoire). workflow_dispatch : « essai » (tout produit, APK signé par une clé de débogage, non publiable,
# aucune release) ou « reel » (secrets obligatoires). Branche essai-ci/release-* : « essai », pour prouver le flux
# sans tag. Le tag, sa poussée et la publication du brouillon relèvent du go écrit de Jérôme (action finale).
on:
  push:
    tags: ["v*"]
    branches: ["essai-ci/release-**"]
  workflow_dispatch:
    inputs:
      mode:
        description: "essai (non publiable, aucune release) ou reel (secrets de signature requis)"
        type: choice
        options: [essai, reel]
        default: essai

# Lecture seule partout ; seuls « release » (brouillon) et « nettoyer » (artefacts du run) écrivent.
permissions:
  contents: read

env:
  QT_VERSION: "6.12.0"

jobs:
  mode:
    runs-on: ubuntu-latest
    outputs:
      mode: ${{ steps.mode.outputs.mode }}
      version: ${{ steps.mode.outputs.version }}
    steps:
      - uses: actions/checkout@v4
      - id: mode
        shell: pwsh
        run: |
          . ./scripts/version-projet.ps1
          $version = Get-ActCraftVersion
          $mode = if ('${{ github.ref_type }}' -eq 'tag') { 'reel' }
                  elseif ('${{ github.event_name }}' -eq 'workflow_dispatch') { '${{ inputs.mode }}' }
                  else { 'essai' }
          if ('${{ github.ref_type }}' -eq 'tag' -and '${{ github.ref_name }}' -ne "v$version") {
            Write-Output "Tag ${{ github.ref_name }} différent de la version du projet (v$version) : arrêt."
            exit 1
          }
          "mode=$mode" >> $env:GITHUB_OUTPUT
          "version=$version" >> $env:GITHUB_OUTPUT
          Write-Output "Mode $mode, version $version"

  # Mode réel : refus, avant toute construction, si un secret de signature manque.
  secrets:
    needs: mode
    if: needs.mode.outputs.mode == 'reel'
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Secrets de signature
        shell: pwsh
        env:
          ACTCRAFT_KEYSTORE_BASE64: ${{ secrets.ACTCRAFT_KEYSTORE_BASE64 }}
          ACTCRAFT_KEYSTORE_PASSWORD: ${{ secrets.ACTCRAFT_KEYSTORE_PASSWORD }}
          ACTCRAFT_KEY_ALIAS: ${{ secrets.ACTCRAFT_KEY_ALIAS }}
          ACTCRAFT_KEY_PASSWORD: ${{ secrets.ACTCRAFT_KEY_PASSWORD }}
        run: pwsh -NoProfile -File ./scripts/verifier-secrets-release.ps1

  # Mode essai : prouve que le mode réel, privé de secrets, échoue avec son message ; et que les notes existent.
  controle-mode-reel:
    needs: mode
    if: needs.mode.outputs.mode == 'essai'
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Le mode réel échoue sans secrets
        shell: pwsh
        run: |
          $sortie = pwsh -NoProfile -File ./scripts/verifier-secrets-release.ps1
          $code = $LASTEXITCODE
          Write-Output $sortie
          if ($code -eq 0 -or -not ("$sortie" -match 'SECRETS ABSENTS')) {
            Write-Output 'Le mode réel aurait dû échouer sans secrets.'
            exit 1
          }
          Write-Output 'CONTROLE OK : sans secrets, le mode réel échoue avant toute construction.'
      - name: Notes de la version
        shell: pwsh
        run: |
          pwsh -NoProfile -File ./scripts/notes-version.ps1 -Version '${{ needs.mode.outputs.version }}' -Sortie notes.md
          if ($LASTEXITCODE -ne 0) { exit 1 }
          Get-Content notes.md

  windows:
    needs: [mode, secrets]
    if: ${{ !cancelled() && needs.mode.result == 'success' && (needs.secrets.result == 'success' || needs.secrets.result == 'skipped') }}
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@v5
        with:
          python-version: "3.12"
      - name: Installer Ninja et aqtinstall
        run: python -m pip install ninja "git+https://github.com/miurahr/aqtinstall"
      - name: Installer Qt
        shell: bash
        run: |
          python -m aqt install-qt windows desktop "$QT_VERSION" win64_msvc2022_64 -m qtshadertools qtimageformats --outputdir "$RUNNER_TEMP/Qt"
          dir=$(ls -d "$RUNNER_TEMP"/Qt/"$QT_VERSION"/*/ | head -1)
          echo "ACTCRAFT_QT_DIR=${dir%/}" >> "$GITHUB_ENV"
          echo "${dir%/}/bin" >> "$GITHUB_PATH"
      - uses: ilammy/msvc-dev-cmd@v1
      - name: Installer Inno Setup
        shell: pwsh
        run: |
          $iscc = Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'
          if (-not (Test-Path $iscc)) { choco install innosetup -y --no-progress }
          if (-not (Test-Path $iscc)) { throw "ISCC.exe introuvable après installation" }
          "ACTCRAFT_ISCC=$iscc" >> $env:GITHUB_ENV
      - name: Construire et essayer l'installateur
        shell: pwsh
        run: |
          pwsh -NoProfile -File ./scripts/paquet.ps1
          if ($LASTEXITCODE -ne 0) { exit 1 }
          pwsh -NoProfile -File ./scripts/essai-installateur.ps1 -Rendu logiciel
          if ($LASTEXITCODE -ne 0) { exit 1 }
          Get-FileHash build/paquet/ActCraft-*-setup.exe -Algorithm SHA256 | ForEach-Object { "$($_.Hash.ToLower())  $(Split-Path $_.Path -Leaf)" }
      - name: Conserver l'installateur pour le brouillon (mode réel seulement)
        if: needs.mode.outputs.mode == 'reel'
        uses: actions/upload-artifact@v4
        with:
          name: paquet-windows
          path: build/paquet/ActCraft-*-setup.exe
          retention-days: 1

  android:
    needs: [mode, secrets]
    if: ${{ !cancelled() && needs.mode.result == 'success' && (needs.secrets.result == 'success' || needs.secrets.result == 'skipped') }}
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@v5
        with:
          python-version: "3.12"
      - uses: actions/setup-java@v4
        with:
          distribution: temurin
          java-version: "17"
      - name: Versions de la chaîne Android
        run: cat packaging/android/versions.env >> "$GITHUB_ENV"
      - name: Installer Ninja et aqtinstall
        run: python -m pip install ninja "git+https://github.com/miurahr/aqtinstall"
      - name: Installer Qt (hôte et Android arm64-v8a)
        run: |
          python -m aqt install-qt linux desktop "$QT_VERSION" linux_gcc_64 -m qtshadertools qtimageformats --outputdir "$RUNNER_TEMP/Qt"
          python -m aqt install-qt all_os android "$QT_VERSION" android_arm64_v8a -m qtshadertools qtimageformats --outputdir "$RUNNER_TEMP/Qt"
          echo "ACTCRAFT_QT_HOST_DIR=$RUNNER_TEMP/Qt/$QT_VERSION/gcc_64" >> "$GITHUB_ENV"
          echo "ACTCRAFT_QT_ANDROID_ARM64_DIR=$RUNNER_TEMP/Qt/$QT_VERSION/android_arm64_v8a" >> "$GITHUB_ENV"
          echo "ACTCRAFT_QT_ANDROID_X86_64_DIR=$RUNNER_TEMP/Qt/$QT_VERSION/android_arm64_v8a" >> "$GITHUB_ENV"
      - name: Installer le NDK, la plateforme et les build-tools
        run: |
          sdkmanager="$ANDROID_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager"
          yes | "$sdkmanager" --licenses > /dev/null
          "$sdkmanager" "ndk;$ANDROID_NDK_VERSION" "build-tools;$ANDROID_BUILD_TOOLS" "platforms;android-$ANDROID_PLATFORM"
          echo "ANDROID_NDK_ROOT=$ANDROID_SDK_ROOT/ndk/$ANDROID_NDK_VERSION" >> "$GITHUB_ENV"
      - name: Construire l'APK arm64-v8a
        shell: pwsh
        env:
          ACTCRAFT_KEYSTORE_BASE64: ${{ needs.mode.outputs.mode == 'reel' && secrets.ACTCRAFT_KEYSTORE_BASE64 || '' }}
          ACTCRAFT_KEYSTORE_PASSWORD: ${{ needs.mode.outputs.mode == 'reel' && secrets.ACTCRAFT_KEYSTORE_PASSWORD || '' }}
          ACTCRAFT_KEY_ALIAS: ${{ needs.mode.outputs.mode == 'reel' && secrets.ACTCRAFT_KEY_ALIAS || '' }}
          ACTCRAFT_KEY_PASSWORD: ${{ needs.mode.outputs.mode == 'reel' && secrets.ACTCRAFT_KEY_PASSWORD || '' }}
        run: |
          if ('${{ needs.mode.outputs.mode }}' -eq 'reel') {
            $ks = Join-Path $env:RUNNER_TEMP 'release.keystore'
            [IO.File]::WriteAllBytes($ks, [Convert]::FromBase64String($env:ACTCRAFT_KEYSTORE_BASE64))
            pwsh -NoProfile -File ./scripts/paquet-android.ps1 -Abi arm64-v8a -Keystore $ks `
              -KeystorePass $env:ACTCRAFT_KEYSTORE_PASSWORD -KeyAlias $env:ACTCRAFT_KEY_ALIAS -KeyPass $env:ACTCRAFT_KEY_PASSWORD
            if ($LASTEXITCODE -ne 0) { exit 1 }
          } else {
            pwsh -NoProfile -File ./scripts/paquet-android.ps1 -Abi arm64-v8a
            if ($LASTEXITCODE -ne 0) { exit 1 }
            Write-Output 'APK NON PUBLIABLE : signé par une clé de débogage (mode essai).'
          }
          Get-FileHash build/paquet/ActCraft-*-arm64-v8a.apk -Algorithm SHA256 | ForEach-Object { "$($_.Hash.ToLower())  $(Split-Path $_.Path -Leaf)" }
      - name: Conserver l'APK pour le brouillon (mode réel seulement)
        if: needs.mode.outputs.mode == 'reel'
        uses: actions/upload-artifact@v4
        with:
          name: paquet-android
          path: build/paquet/ActCraft-*-arm64-v8a.apk
          retention-days: 1

  # Mode réel seulement : brouillon de release, jamais publié ici (publication = go écrit, action finale).
  release:
    needs: [mode, windows, android]
    if: needs.mode.outputs.mode == 'reel'
    runs-on: ubuntu-latest
    permissions:
      contents: write
    steps:
      - uses: actions/checkout@v4
      - uses: actions/download-artifact@v4
        with:
          path: paquets
          merge-multiple: true
      - name: Sommes SHA-256 et notes
        shell: pwsh
        run: |
          Get-ChildItem paquets -File | Get-FileHash -Algorithm SHA256 |
            ForEach-Object { "$($_.Hash.ToLower())  $(Split-Path $_.Path -Leaf)" } |
            Set-Content -Encoding utf8NoBOM paquets/SHA256SUMS.txt
          pwsh -NoProfile -File ./scripts/notes-version.ps1 -Version '${{ needs.mode.outputs.version }}' -Sortie notes.md
          if ($LASTEXITCODE -ne 0) { exit 1 }
      - name: Brouillon de release
        env:
          GH_TOKEN: ${{ secrets.GITHUB_TOKEN }}
        run: |
          gh release create "v${{ needs.mode.outputs.version }}" --draft --verify-tag \
            --title "ActCraft ${{ needs.mode.outputs.version }}" --notes-file notes.md paquets/*

  # Mode réel : les artefacts du run (installateur, APK signé) ne servent qu'à assembler le brouillon. Supprimés à
  # la fin quoi qu'il arrive (échec de « windows », d'« android » ou de « release » compris), pour qu'ils ne restent
  # pas téléchargeables avant la publication (go écrit).
  nettoyer:
    needs: [mode, windows, android, release]
    if: always() && needs.mode.outputs.mode == 'reel'
    runs-on: ubuntu-latest
    permissions:
      actions: write
    steps:
      - name: Supprimer les artefacts du run
        env:
          GH_TOKEN: ${{ secrets.GITHUB_TOKEN }}
        run: |
          gh api "repos/${{ github.repository }}/actions/runs/${{ github.run_id }}/artifacts" --jq '.artifacts[].id' |
            while read -r artefact; do
              gh api -X DELETE "repos/${{ github.repository }}/actions/artifacts/$artefact"
            done
```
(Le job `android` de `release.yml` n'installe que Qt `android_arm64_v8a` : la release ne livre que cette ABI,
décision 28 ; `ACTCRAFT_QT_ANDROID_X86_64_DIR` y pointe vers le même dossier pour que `dev-env-android.ps1` ne
s'arrête pas, le preset `android-x86_64` n'étant pas construit.)

- [ ] **Étape 6 : profil — commandes réelles.** Dans `.claude/chantier.md` :
  1. section « Preuve visuelle » : remplacer la phrase « La forme bornée exacte est écrite ici par la tâche L1-T20. »
     par :

````markdown
- Forme bornée :
  `New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario premiers-pas -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "pageIndicator.text=1 / 2" > .tmp/captures.txt 2>&1 ; Select-String -Path .tmp/captures.txt -Pattern '^(LU|PIXEL|CAPTURE) |CAPTURES (OK|EN ECHEC)' | Select-Object -Last 16`
- Motifs d'échec : `CAPTURES EN ECHEC`, toute ligne `LU … ECART`, `PIXEL … ECART` ou `SCENARIO INVALIDE`.
- À l'écran (fin de lot, jeton `ecran`) : même commande avec `-Rendu d3d11` et sans `-HorsEcran`.
- Le scénario `premiers-pas` est aussi un test ctest (`tst_captures_premiers_pas`, hors écran).
````
  2. ajouter, juste avant la section « Sources du cadrage (après le coffre) », la section :

```markdown
## Paquets (installateur Windows, APK Android) et release

Locaux, jamais publiés hors du go de la release (décisions 26 à 33 du cadrage
`docs/specs/2026-10-14-livraison-complete-cadrage.md`).

- Installateur : `pwsh -NoProfile -File ./scripts/paquet.ps1` → `build/paquet/ActCraft-<version>-setup.exe`
  (`PAQUET OK`) ; essai S1 : `pwsh -NoProfile -File ./scripts/essai-installateur.ps1` (`INSTALLATEUR OK` ; ouvre une
  fenêtre 5 s : l'annoncer).
- APK : `pwsh -NoProfile -File ./scripts/paquet-android.ps1` (chaîne : `scripts/dev-env-android.ps1`) →
  `build/paquet/ActCraft-<version>-arm64-v8a.apk` et `…-x86_64.apk`, clé de débogage locale (`APK OK`) ; essai S3 sur
  un émulateur démarré : `pwsh -NoProfile -File ./scripts/essai-android.ps1 -Apk <…-x86_64.apk>` (`ANDROID OK`).
  Émulateur local ou CI : `docs/verifications/2026-10-14-outillage-paquets.md`.
- Licences : `scripts/verifier-licences.ps1` (appelé par les deux scripts de paquet ; liste
  `packaging/qt-autorises.txt`).
- CI : jobs `paquet-windows` (S1) et `android` (S3, émulateur KVM) à chaque PR, sans téléversement. Essai de CI sans
  PR : push d'une branche `essai-ci/<sujet>`, puis suppression de la branche.
- Release : `.github/workflows/release.yml`. Tag `v<version>` (action finale, go écrit) → brouillon avec
  installateur, APK `arm64-v8a` signé par la clé de release, `SHA256SUMS.txt`, notes de `CHANGELOG.md`. Essai sans
  tag : `workflow_dispatch` `mode=essai` (workflow sur `main`) ou push d'une branche `essai-ci/release-<sujet>`.
  Publication du brouillon : `gh release edit v<version> --draft=false`, sous le même go.
```

- [ ] **Étape 7 : commit (avant le push : c'est ce commit que le workflow essaie).**

```powershell
Set-Location <worktree>; git add .github/workflows/release.yml CHANGELOG.md scripts/verifier-secrets-release.ps1 scripts/notes-version.ps1 .claude/chantier.md
git -C <worktree> commit -m "Release : brouillon sur tag, mode essai sans release, échec sans secrets ; profil des paquets" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

- [ ] **Étape 7 bis : contrôle avant publication (propriété intellectuelle).** Avant **tout** push (le dépôt est
public ; supprimer la branche n'efface ni les commits ni les journaux) :

```powershell
Set-Location <worktree>; git fetch origin main; git diff --name-only --diff-filter=AM origin/main..HEAD | Where-Object { Test-Path $_ } | ForEach-Object { Select-String -Path $_ -Pattern 'Promethean|ActivInspire|\bActiv\b|ExpressPoll|Planet' } | ForEach-Object { "$($_.Path | Resolve-Path -Relative):$($_.LineNumber): $($_.Line.Trim())" }
Set-Location <worktree>; git diff --name-only --diff-filter=A origin/main..HEAD | Where-Object { $_ -match '\.(png|jpe?g|gif|bmp|ico|svg|flipchart|flp|as4|wav|mp3|mp4|ttf|otf)$' -or $_ -match '^(build|\.tmp|bench/results)/' -or $_ -match 'samples' }
```
Attendu : seulement des mentions écrites par nous (nom cité pour l'interopérabilité ou pour dire ce qu'on ne
reprend pas), et seulement les 10 icônes `ui/icons/*.svg` déjà relues en L1-T19. Recopier les sorties et le
verdict dans la preuve ; tout contenu de Promethean ou fichier de `samples/`, `build/`, `.tmp/`, `bench/results/`
bloque le push.

- [ ] **Étape 8 : run réel du mode essai.** Lancer les trois commandes bornées. Rouge : un job en `failure` →
`gh run view <id> --log-failed | Select-Object -Last 60`, corriger la cause dans `release.yml` ou les deux scripts
de cette tâche, commiter, refaire l'étape 7 bis, repousser ; trois essais au plus. Vert : voir « Attendu ».
Vérifier que `gh release list` n'a rien de nouveau.

- [ ] **Étape 9 : profil toujours tenu.** Le bloc `action-finale` posé par L1-T1 ne doit pas avoir changé :

```powershell
Set-Location <worktree>; $t = Get-Content -Raw .claude/chantier.md; foreach ($m in '\bgit(\.exe)?\b(\s+-[Cc]\s+(', '\s--follow-tags\b|\s--mirror\b|\stag\s))', '\bgh(\.exe)?\s+release\s+(create|edit|upload|delete)\b', '\bgh(\.exe)?\s+api\b(?=.*(releases|refs/tags))') { Write-Output "$($t.Contains($m)) : $m" }; git -C <worktree> log --format=%h -- .claude/chantier.md | Select-Object -First 1 | ForEach-Object { git -C <worktree> show $_ -- .claude/chantier.md } | Select-String '^[-+].*action-finale|^[-+]\\b(git|gh)'
```
Attendu : quatre lignes `True`, et aucune ligne de diff sur le bloc `action-finale` (cette tâche n'ajoute que les
sections « Preuve visuelle » et « Paquets »).

- [ ] **Étape 10 : nettoyer.** `git -C <worktree> push origin --delete essai-ci/release-L1-T20` ; vérifier avec
`git -C <worktree> ls-remote --heads origin "essai-ci/*"`.

---

## Clôture du lot 1 (orchestrateur, hors tâches)

Joué par l'orchestrateur après la dernière vague, avant la PR du lot (étage 4), dans le worktree du chantier, sur
la branche `feat/livraison-complete` à jour :

1. **Suite complète** : `scripts/test.ps1 -Preset release` puis `scripts/test.ps1` (formes bornées du profil),
   N ≥ 42 chacune, `TESTS OK`.
2. **S2 à l'écran** (jeton `ecran`, annoncé d'une ligne à Jérôme) :
   `pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario premiers-pas -Rendu d3d11 -Taille plein-ecran -Sortie .tmp/captures-ecran`
   → `CAPTURES OK` ; images regardées et décrites à la porte de fin.
3. **Banc de non-régression (DA7, décision 35)** sous le jeton `ecran` :
   `pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu tous -Captures .tmp/banc-lot1`, comparé critère par critère à
   `docs/mesures/2026-10-13-prototype-mesure.md` ; tout critère tenu par la référence doit l'être encore, sinon une
   tâche de correction précède la clôture. Résumé commité `docs/mesures/<date>-lot1-socle.md` (par
   `scripts/resume-banc.ps1`) : il devient la référence des lots suivants.
4. **Paquets remis à Jérôme** (S1, S3) : `scripts/paquet.ps1` + `scripts/essai-installateur.ps1`,
   `scripts/paquet-android.ps1` ; fichiers dans `build/paquet/` (chemins donnés à la porte). La CI de la PR rejoue
   S1 et S3.
5. **CHANGELOG** : date de la section 0.1.0 ; **graphe** : `graphify update .` puis commit de `graphify-out/`.
6. **Porte de fin du lot 1** : release 0.1.0 proposée (publiée seulement sur go écrit, décision 38 et profil) ; deux
   questions avec défaut appliqué sans réponse : fonctions à revoir complètement (défaut : aucune, N1-1) ; avancer le
   lot « cache incrémental » (défaut : non, N1-10). Signaler aussi N1-13 (aspect et icônes neufs, captures à
   l'appui) et les sept points de la section « Écarts au cadrage » (dont : la 0.1.0 n'ouvre encore aucun fichier,
   et le lot a été publié sur le dépôt public par les branches `essai-ci/…`).
