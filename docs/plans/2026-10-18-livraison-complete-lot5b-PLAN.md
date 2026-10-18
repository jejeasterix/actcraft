# Livraison complète — lot 5 « Édition et fichiers », sous-lot 5b (onglets, export, impression, imports) — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**But.** Suite du sous-lot 5a (`docs/plans/2026-10-18-livraison-complete-lot5-PLAN.md`, qui fait autorité pour les
contraintes, les API supposées A1 à A21, les décisions D1 à D20, les commandes communes des preuves, les scénarios et
l'annexe A des en-têtes figés). Livre : onglets de documents (F-010 à F-012), module d'export (DA12 : page en image,
PDF, import PDF), impression complète (F-670 amendée, DA-A2, S17), PowerPoint en images (F-661 en images), bancs, S8
et la clôture du lot. Une seule PR pour le lot, à la fin de 5b ; version attribuée à la fusion.

**Rappels.** L'export lit `core::Document` (copie prise sur le fil principal), jamais le cache ; rendu par
`render::paintPage` / `renderPage` (A11, vérifiés à 794cf84) sur un fil secondaire. Une seule `PageView` pour tous les
onglets (contrainte 3 du plan 5a). Espace de noms `actcraft::exporting` (contrainte 4). Tout HTML de document passe
par le `QTextDocument` à `loadResource` neutralisé du lot 2 (L2-T11) : le peintre partagé le fait déjà ; aucune tâche
d'export ne rend du HTML autrement. Mutation remise par `git -C <wt> checkout -- <fichier>`, jamais par `Copy-Item`.

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L5-T20 | `ui` : onglets — sessions garées, une seule vue, bascule mesurée | L5-T19 | `ui/src/documenttabs.cpp`, `ui/src/documentcontroller.cpp` (corps de `parkSession` / `resumeSession`), `tests/tst_ui_tabs.cpp` | isolé | concepteur-critique | `tst_ui_tabs` Passed (dont `parkResumeKeepsUndoHistory`, `parkDropsALiftFirst`, `switchRedrawsOnce`, `closingModifiedAsksToSave`, `copyPagesGoBeforeFirstPage`, `oneViewForAllTabs`) ; `tst_ui_document` vert ; mutation rouge puis remise ; banc joint | S8, S8.5 | #tache-l5-t20 |
| L5-T22 | Qt PDF : installation sur le poste et en CI, licences, option `ACTCRAFT_IMPORT_PDF` automatique | L5-T19 | `CMakeLists.txt` (bloc des options seulement), `export/CMakeLists.txt`, `.github/workflows/ci.yml`, `packaging/qt-autorises.txt`, `CLAUDE.md` (ligne d'installation), `docs/BRIEF.md` (§ 4, module `export/`) | isolé | implementeur-tdd | `cmake` affiche `ACTCRAFT_IMPORT_PDF=ON` ; build 0 avertissement ; `scripts/verifier-licences.ps1` → `LICENCES OK` | S8 | #tache-l5-t22 |
| L5-T26 | `core` : mise en page d'impression (feuilles, plage, copies, pages par feuille, marge, notes) | L5-T19 | `core/src/printlayout.cpp`, `tests/tst_core_printlayout.cpp` | isolé | implementeur-tdd | `tst_core_printlayout` Passed (dont `twoPerSheetRange2to4`, `notesAddSheets`, `longPageFitsWhole`, `copiesMultiply`, `marginRespected`, `currentOnly`) | S17 | #tache-l5-t26 |
| L5-T21 | QML : barre d'onglets, fermer, glisser des pages vers un onglet ; scénario `onglets` | L5-T20 | `ui/qml/DocumentTabBar.qml`, `ui/qml/PageNavigator.qml` (dépôt sur un onglet seulement), `bench/scenarios/onglets.json`, `tests/tst_ui_shell.cpp` | isolé | implementeur-tdd | `tst_ui_shell` Passed dont `tabBarFollowsPosition` ; `onglets` hors écran `CAPTURES OK` (lectures de la section) | S8.5 | #tache-l5-t21 |
| L5-T23 | `export` : page en image et PDF, hors du fil principal | L5-T22 | `export/src/pageexport.cpp`, `export/src/pdfexport.cpp`, `tests/tst_export_image.cpp`, `tests/tst_export_pdf.cpp` | isolé | implementeur-tdd | `tst_export_image` (dont `png1024x768`, `jpegBmpTiffByExtension`, `runsOffMainThread`) et `tst_export_pdf` (dont `threePagesGiveThreePages`, `pageSizeInPoints`, `textIsGuarded`) Passed | S8 | #tache-l5-t23 |
| L5-T27 | `print/` : impression par `QPrinter` (sans `QApplication`), feuilles par le peintre partagé, gris ; démarrage mesuré | L5-T26 | `print/CMakeLists.txt`, `print/src/printer.cpp`, `tests/tst_print_printer.cpp`, `packaging/qt-autorises.txt` | isolé | concepteur-critique | `tst_print_printer` Passed (dont `printsToFileThroughVirtualPrinter`, `grayHasNoColor`, `sheetCountMatchesLayout`, `worksWithoutQApplication`) ; temps de démarrage avant / après relevé ; `LICENCES OK` | S17 | #tache-l5-t27 |
| L5-T24 | `export` : import PDF (une page par page, image verrouillée sur la couche de fond) | L5-T22, L5-T23 | `export/src/pdfimport.cpp`, `tests/tst_export_pdfimport.cpp` | isolé | implementeur-tdd | `tst_export_pdfimport` Passed (dont `roundTripThreePages`, `pageSizeFromPdf`, `imageOnBackgroundLocked`, `offMainThread`, `absentWithoutQtPdf`) | S8 | #tache-l5-t24 |
| L5-T25 | `ui` + QML : commandes d'export et d'import PDF, menu de page « Exporter la page », impression par PDF sous Android | L5-T21, L5-T23, L5-T24 | `ui/src/fileexchange.cpp`, `ui/qml/FileExchangeActions.qml`, `ui/qml/PageContextMenu.qml` (entrée d'export seulement), `tests/tst_ui_fileexchange.cpp`, `bench/scenarios/exporter.json` | isolé | implementeur-tdd | `tst_ui_fileexchange` Passed (dont `exportPdfCommandWritesFile`, `exportPageImageCommand`, `importPdfOpensATab`, `printThroughPdfOnAndroid`, `busyWhileExporting`) ; `exporter` hors écran `CAPTURES OK` | S8 | #tache-l5-t25 |
| L5-T28 | QML : boîte d'impression, aperçu, réglages gardés ; scénario S17 | L5-T25, L5-T27 | `ui/src/printcontroller.cpp`, `ui/qml/PrintDialog.qml`, `ui/qml/PrintPreview.qml`, `tests/tst_ui_printdialog.cpp`, `bench/scenarios/imprimer.json` | isolé | implementeur-tdd | `tst_ui_printdialog` Passed (dont `settingsSurviveRestart`, `previewMatchesSheets`, `sheetCountFollowsSettings`) ; `imprimer` hors écran `CAPTURES OK` (S17) | S17 | #tache-l5-t28 |
| L5-T29 | `export` : PowerPoint en images (convertisseur trouvé, conversion en PDF, import PDF) | L5-T24 | `export/src/officeconvert.cpp`, `tests/tst_export_officeconvert.cpp` | isolé | implementeur-tdd | `tst_export_officeconvert` Passed (dont `envConverterWins`, `argumentsSubstituted`, `timeoutReported`, `missingConverterReported`, `fakeConverterChainGivesPages`) | S8 | #tache-l5-t29 |
| L5-T31 | Banc : scénarios `onglets` (mémoire et bascule) et `export-fond` (export PDF pendant l'écriture) | L5-T21, L5-T25 | `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp`, `bench/src/runner.cpp`, `tests/tst_bench_scenarios.cpp` | isolé | implementeur-tdd | `tst_bench_scenarios` Passed dont `tabsAndExportScenariosListed` ; banc hors écran : sections `onglets` (`memory_delta_mb`) et `export-fond` (latence d'encre) dans le JSON | S8.5, S8 | #tache-l5-t31 |
| L5-T30 | `ui` + QML : import PowerPoint en images, menu Importer (PDF et présentation seulement, N1-A8) ; scénario `importer` | L5-T25, L5-T29 | `ui/src/fileexchange.cpp`, `ui/qml/FileExchangeActions.qml`, `tests/tst_ui_import.cpp`, `bench/scenarios/importer.json` | isolé | implementeur-tdd | `tst_ui_import` Passed (dont `powerpointThroughFakeConverter`, `menuHidesUnavailableImports`, `failureShowsBanner`) ; `importer` hors écran `CAPTURES OK` | S8 | #tache-l5-t30 |
| L5-T32 | Scénario S8 `edition-export`, rejoué à l'écran en D3D11 ; S17 à l'écran | L5-T18, L5-T28, L5-T30, L5-T31 | `bench/scenarios/edition-export.json` | isolé | implementeur-tdd | hors écran `CAPTURES OK` (lectures de la section) ; à l'écran en D3D11 sous le jeton `ecran` `CAPTURES OK` | S8, S17 | #tache-l5-t32 |
| L5-T33 | Contrôle de fin du lot : suites debug et release, talons, graphe, noms | L5-T0 à L5-T32 | `graphify-out/` | partagé | implementeur-tdd | debug et release `TESTS OK`, N = N_départ + 26 ; aucun talon `L5-T1` restant (`git grep -n "Talon L5-T1" -- . ':(exclude)docs/'` vide) ; aucun `QSKIP("réservé : L5-` ; captures `-Scenario tous` `CAPTURES OK` ; banc hors écran joint ; `NOMS OK` ; titres du corpus sans sortie ; `graphify update .` commité | S8, S17, S8.1 à S8.6 | #tache-l5-t33 |

14 tâches (L5-T20 à L5-T33). Lot entier : 34 tâches, deux plans, un cadrage.

## Vagues (largeur 3 au plus, 2 places de tests)

V9 : L5-T20 ‖ L5-T22 ‖ L5-T26 · V10 : L5-T21 ‖ L5-T23 ‖ L5-T27 · V11 : L5-T24 ‖ L5-T28 · V12 : L5-T25 ‖ L5-T29 ·
V13 : L5-T30 ‖ L5-T31 · V14 : L5-T32 · V15 : L5-T33.

Recoupements vérifiés :
- V9 : T20 (`documenttabs.cpp`, `documentcontroller.cpp`, test) / T22 (CMake racine bloc des options, `export/CMakeLists.txt`,
  CI, licences, `CLAUDE.md`, `BRIEF.md`) / T26 (`printlayout.cpp`, test) — disjoints.
- V10 : T21 (QML des onglets, `PageNavigator.qml`, `onglets.json`, `tst_ui_shell.cpp`) / T23 (`pageexport.cpp`,
  `pdfexport.cpp`, deux tests) / T27 (`print/`, test, `packaging/qt-autorises.txt`) — disjoints (T22, qui touche aussi
  `qt-autorises.txt`, est en V9).
- V11 : T24 (`pdfimport.cpp`) / T28 (`printcontroller.cpp`, QML d'impression, `imprimer.json`). T25 est en V12 parce
  qu'il dépend de T24.
- V12 : T25 (`fileexchange.cpp`, `FileExchangeActions.qml`, `PageContextMenu.qml`, `exporter.json`) / T29
  (`officeconvert.cpp`) — disjoints.
- V13 : T30 (`fileexchange.cpp`, `FileExchangeActions.qml`, `importer.json`) / T31 (`bench/`) — disjoints ; T25 et
  T30 partagent `fileexchange.cpp` et sont dans deux vagues.

---

### Tâche L5-T20 — `ui` : onglets, sessions garées, une seule vue

<a id="tache-l5-t20"></a>

**Objectif.** Remplir `ui/src/documenttabs.cpp` et les corps de `DocumentController::parkSession` /
`resumeSession` (annexe A du plan 5a). Décision d'architecture (contrainte 3) : **un seul `DocumentController` affiché
et une seule `PageView`** ; un onglet inactif est une `ParkedSession` (document, pile d'annulation, URL, état modifié,
avertissements, index de page affichée, page de référence de « Rétablir la page »). Changer d'onglet = `parkSession()`
de l'actif puis `resumeSession()` de l'autre : un redessin du cache, une fois. Point délicat (`concepteur-critique`) :
l'invariant partagé « tout ce qui appartient au document part avec lui » — un oubli (pile d'annulation, page de
référence, soulèvement en cours, observateurs, miniatures du lot 4) se voit tard. Le patron évident (une
`DocumentController` + une `PageView` par onglet) est écarté : ≈ 74,6 Mo de cache par vue.

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée, motif `^(tst_ui_tabs|tst_ui_document)$` :
```cpp
void TestTabs::parkResumeKeepsUndoHistory()
{
    Fixture f; // DocumentController + DocumentTabs, un trait versé
    f.tabs.newTab();
    QCOMPARE(f.tabs.rowCount(), 2);
    QVERIFY(!f.document.canUndo());
    f.tabs.activate(0);
    QVERIFY(f.document.canUndo());
    f.document.undo();
    QCOMPARE(f.document.shownObjectCount(), 0);
}
void TestTabs::parkDropsALiftFirst()
{
    Fixture f;
    f.document.liftObjects({f.firstId()});
    f.tabs.newTab();
    f.tabs.activate(0);
    QVERIFY(!f.document.isLifting());
    QCOMPARE(f.document.shownObjectCount(), 1); // l'objet soulevé est revenu, sans étape
}
void TestTabs::switchRedrawsOnce()
{
    Fixture f(/*withView=*/true);
    f.tabs.newTab();
    const int before = f.view.cacheRefreshCount();
    f.tabs.activate(0);
    QTRY_COMPARE(f.view.cacheRefreshCount(), before + 1);
}
void TestTabs::closingModifiedAsksToSave()
{
    Fixture f;
    QSignalSpy asked(&f.tabs, &ui::DocumentTabs::saveRequested);
    QVERIFY(!f.tabs.requestClose(0));
    QCOMPARE(asked.count(), 1);
    f.tabs.closeWithoutSaving(0);
    QCOMPARE(f.tabs.rowCount(), 1); // un document vide reste toujours ouvert
}
void TestTabs::copyPagesGoBeforeFirstPage()
{
    Fixture f; // onglet 0 : 2 pages
    f.tabs.newTab();          // onglet 1 : 1 page, actif
    f.tabs.activate(0);
    f.tabs.copyPagesToTab({1}, 1);
    f.tabs.activate(1);
    QCOMPARE(f.document.pageCount(), 2);
    QCOMPARE(f.document.currentIndex(), 0);
    QVERIFY(f.pageHasObjects(0)); // la page copiée est la première
    f.document.undo();        // une étape dans l'onglet d'arrivée
    QCOMPARE(f.document.pageCount(), 1);
}
void TestTabs::oneViewForAllTabs()
{
    Fixture f(/*withView=*/true);
    for (int i = 0; i < 3; ++i)
        f.tabs.newTab();
    QCOMPARE(f.window.findChildren<ui::PageView*>().size(), qsizetype(1));
}
```
Mutation : ne pas garer la pile d'annulation (`parkSession` la laisse au contrôleur) → `parkResumeKeepsUndoHistory`
rouge ; remise par `git -C <wt> checkout -- ui/src/documentcontroller.cpp`. Banc sans écran joint.

**Interdits.** Aucune seconde `PageView` ; aucune signature publique du contrôleur changée hors des deux membres
réservés ; copier des pages passe par `core::pageedits` (A15), une étape dans le document d'arrivée.

**Terminé quand.** Tests `Passed` ; mutation rouge puis remise ; banc joint ; commit « Lot 5 : onglets de documents,
une seule vue ».

**Étapes.** Tests rouges → `ParkedSession` (membres déplacés par `std::move`) → `parkSession` (dépôt sans étape d'un
soulèvement, `editAboutToApply` émis pour finir un trait en cours) → `resumeSession` (une reconstruction de la page
affichée, signaux du contrôleur émis) → modèle d'onglets (titre, astérisque, « Sans titre ») → copie de pages vers un
onglet garé (exécutée sur sa pile garée) → vert → mutation → banc → format → commit.

### Tâche L5-T22 — Qt PDF : installation, CI, licences, option automatique

<a id="tache-l5-t22"></a>

**Objectif.** Qt PDF n'est pas installé (relevé à 794cf84 : `C:\Qt\6.12.0\msvc2022_64\lib` n'a pas `Qt6Pdf`).
L'installer sur le poste (`python -m aqt install-qt windows desktop 6.12.0 win64_msvc2022_64 -m qtpdf --outputdir
C:\Qt`, et pour Android `-m qtpdf` sur les deux cibles de `scripts/dev-env-android.ps1`), ajouter `qtpdf` aux
installations de `.github/workflows/ci.yml` (Windows, macOS, Android), rendre `ACTCRAFT_IMPORT_PDF` automatique
(`find_package(Qt6 COMPONENTS Pdf QUIET)` ; `ON` si trouvé), lier `Qt6::Pdf` à `actcraft_export` sous cette option et
définir `ACTCRAFT_IMPORT_PDF=1` ; compléter `packaging/qt-autorises.txt` (modules tirés par Qt PDF, licence LGPLv3,
relevés par `verifier-licences.ps1`) ; mettre à jour la ligne d'installation de `CLAUDE.md` ; amender `docs/BRIEF.md`
§ 4 (module `export/` après `render/`, DA12, et `print/`, DA-A2). Si l'installation échoue (réseau, aqt) : porte
« installation » à Jérôme (mode autonome, point 1) ; l'import PDF et PowerPoint restent déclarés absents, le reste du
lot continue.

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.**
```powershell
pwsh -NoProfile -Command "Set-Location '<wt>'; . ./scripts/dev-env.ps1; cmake --preset debug > .tmp/configure.txt 2>&1"
Select-String -Path '<wt>/.tmp/configure.txt' -Pattern 'ACTCRAFT_IMPORT_PDF' | Select-Object -Last 1   # ACTCRAFT_IMPORT_PDF=ON
pwsh -NoProfile -Command "Set-Location '<wt>'; pwsh -NoProfile -File ./scripts/verifier-licences.ps1" | Select-Object -Last 1   # LICENCES OK
```
plus la commande ciblée, motif `^(tst_export_pdfimport)$` (talon toujours `Passed`).

**Interdits.** Aucun module de Qt Web Engine installé (Qt PDF seul) ; aucune modification des fichiers d'export ;
aucune poussée de branche `essai-ci/…` sans la supprimer ensuite.

**Terminé quand.** Option `ON` ; build 0 avertissement ; `LICENCES OK` ; CI modifiée (essai par une branche
`essai-ci/qtpdf`, supprimée après) ; commit « Lot 5 : Qt PDF installé, option d'import automatique ».

**Étapes.** Installation → CMake → CI → licences → `CLAUDE.md`, `BRIEF.md` → preuves → commit.

### Tâche L5-T26 — `core` : mise en page d'impression

<a id="tache-l5-t26"></a>

**Objectif.** Remplir `core/src/printlayout.cpp` (annexe A du plan 5a) selon D17. Règle du nombre de feuilles (écrite
ici, revérifiée par L5-T0) : feuilles de pages = ⌈pages de la plage / (lignes × colonnes)⌉ ; feuilles de notes (si
`notes ≠ None`) = nombre de pages de la plage qui ont des notes quand `noteBreak`, sinon ⌈lignes de notes /
kNoteLinesPerSheet⌉ ; `NotesOnly` : feuilles de notes seules ; total × copies. Chaque page tient entière dans sa
cellule (rapport gardé : une page allongée est réduite, jamais coupée), cellules dans la zone utile (feuille moins la
marge).

**Fichiers.** `core/src/printlayout.cpp`, `tests/tst_core_printlayout.cpp`.

**Test et commande bornée.** Commande ciblée, motif `^(tst_core_printlayout)$` :
```cpp
void TestPrintLayout::twoPerSheetRange2to4()
{
    const auto d = documentS17(); // 4 pages, page 3 allongée (1024 × 1536), notes sur les pages 2 et 4
    core::PrintSettings s;
    s.rows = 2;
    s.range = core::PrintRange::FromTo;
    s.from = 2;
    s.to = 4;
    const auto sheets = core::layoutSheets(d, s, 0, {210, 297});
    QCOMPARE(sheets.size(), std::size_t(2)); // 3 pages, 2 par feuille
    QCOMPARE(sheets[0].cells.size(), std::size_t(2));
    QCOMPARE(sheets[1].cells.size(), std::size_t(1));
}
void TestPrintLayout::notesAddSheets()
{
    const auto d = documentS17();
    core::PrintSettings s;
    s.rows = 2;
    s.range = core::PrintRange::FromTo;
    s.from = 2;
    s.to = 4;
    s.notes = core::PrintNotes::WithPages;
    QCOMPARE(core::sheetCount(d, s, 0, {210, 297}), 4); // 2 de pages + 2 de notes (pages 2 et 4)
}
void TestPrintLayout::longPageFitsWhole()
{
    const auto d = documentS17();
    core::PrintSettings s;
    s.range = core::PrintRange::FromTo;
    s.from = 3;
    s.to = 3;
    const auto cell = core::layoutSheets(d, s, 0, {210, 297})[0].cells[0];
    QCOMPARE(cell.target.height() / cell.target.width(), 1536.0 / 1024.0);
    QVERIFY(cell.target.bottom() <= 297 - 5 + 1e-9);
}
void TestPrintLayout::copiesMultiply()
{
    core::PrintSettings s;
    s.copies = 3;
    QCOMPARE(core::sheetCount(documentS17(), s, 0, {210, 297}), 12);
}
void TestPrintLayout::marginRespected()
{
    core::PrintSettings s;
    s.marginMm = 10;
    const auto cell = core::layoutSheets(documentS17(), s, 0, {210, 297})[0].cells[0];
    QVERIFY(cell.target.left() >= 10 - 1e-9 && cell.target.top() >= 10 - 1e-9);
}
void TestPrintLayout::currentOnly()
{
    core::PrintSettings s;
    s.range = core::PrintRange::Current;
    const auto sheets = core::layoutSheets(documentS17(), s, 2, {210, 297});
    QCOMPARE(sheets.size(), std::size_t(1));
    QCOMPARE(sheets[0].cells[0].page, std::size_t(2));
}
```
`documentS17` : document synthétique écrit dans le test (aucun paperboard réel).

**Interdits.** Fonctions pures, sans QtGui. **Terminé quand.** 6 fonctions en `PASS` ; commit « Lot 5 : mise en page
d'impression (core) ». **Étapes.** Tests rouges → plage → grille de cellules → notes → copies → vert → format → commit.

### Tâche L5-T21 — QML : barre d'onglets ; scénario `onglets`

<a id="tache-l5-t21"></a>

**Objectif.** Remplir `DocumentTabBar.qml` (modèle `appTabs` ; nom en gras pour l'actif, astérisque si modifié, croix,
« Sans titre » ; en haut, en bas ou masquée selon `appTabs.position`, D14 ; visible dès deux documents ouverts ; menu
Fichier ▸ Nouveau et Ouvrir ouvrent un onglet ; fermer un onglet modifié ouvre la boîte d'enregistrement du lot 2) ;
dans `PageNavigator.qml` (lot 4), le dépôt de miniatures sur un onglet appelle `appTabs.copyPagesToTab` (F-011) ;
scénario `bench/scenarios/onglets.json` (S8.5).

**Test et commande bornée.** `tst_ui_shell` : `tabBarFollowsPosition`. Commande ciblée `^(tst_ui_shell)$`.

**Preuve visuelle.** `captures.ps1 -Scenario onglets -Rendu logiciel -HorsEcran -Sortie .tmp/captures-onglets -Lire
'appTabs.count=3' -Lire 'appTabs.currentIndex=1' -Lire 'appDocument.pageCount=2'` (forme bornée du profil) :
`CAPTURES OK` ; capture à décrire : trois onglets, le deuxième en gras, sa première page copiée depuis le premier.

**Interdits.** Aucune seconde `PageView` ; aucun changement du navigateur de pages hors du dépôt sur un onglet.

**Terminé quand.** Test `Passed` ; captures `CAPTURES OK` ; banc joint ; commit « Lot 5 : barre d'onglets ».

**Étapes.** Test rouge → barre → fermeture → dépôt de pages → scénario → captures → banc → commit.

### Tâche L5-T23 — `export` : page en image et PDF

<a id="tache-l5-t23"></a>

**Objectif.** Remplir `export/src/pageexport.cpp` et `export/src/pdfexport.cpp` (annexe A du plan 5a) selon D15 :
image à la taille de la page en pixels (`render::renderPage(page, lookup, 1.0)`), format par l'extension (PNG, JPEG,
BMP, TIFF) ; PDF par `QPdfWriter` (résolution 96, taille de page `QPageSize(QSizeF(w, h) * 72.0 / 96.0,
QPageSize::Point)`, marges nulles, `newPage()` entre les pages, `render::paintPage`) ; images décodées par
`render::decodeImages` ; tout sur un fil secondaire (`QtConcurrent::run`) à partir de la copie reçue par valeur ;
écriture dans un temporaire voisin puis renommage (fichier local) ou en une passe (`content://`), comme le format natif.

**Test et commande bornée.** Commande ciblée, motif `^(tst_export_image|tst_export_pdf)$` :
```cpp
void TestExportImage::png1024x768()
{
    QTemporaryDir dir;
    const QUrl url = QUrl::fromLocalFile(dir.filePath("page.png"));
    const auto result = exporting::exportPageImage(documentWithARedRect(), 0, url).result();
    QVERIFY2(result.ok, qPrintable(result.error));
    const QImage image(url.toLocalFile());
    QCOMPARE(image.size(), QSize(1024, 768));
    QCOMPARE(image.pixelColor(150, 150), QColor(Qt::red));
}
void TestExportPdf::threePagesGiveThreePages()
{
    QTemporaryDir dir;
    const QUrl url = QUrl::fromLocalFile(dir.filePath("doc.pdf"));
    QVERIFY(exporting::exportPdf(documentOfPages(3), url).result().ok);
    QFile file(url.toLocalFile());
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(exporting::countPdfPages(file.readAll()), 3);
}
```
plus `jpegBmpTiffByExtension`, `runsOffMainThread` (fil relevé dans le rendu par un crochet de test),
`pageSizeInPoints` (768 × 576 points pour 1024 × 768), `textIsGuarded` (un texte HTML citant `<img
src="file:///…">` exporté sans lecture du fichier cité : compteur de `loadResource` du lot 2 à 0).

**Interdits.** Aucune lecture du cache ni de la vue ; aucun rendu sur le fil principal ; aucune ressource de
Promethean. **Terminé quand.** 6 fonctions en `PASS` ; commit « Lot 5 : export de page en image et en PDF ».

**Étapes.** Tests rouges → image → PDF → `countPdfPages` (expression `/Type\s*/Page(?!s)`) → vert → format → commit.

### Tâche L5-T27 — `print/` : impression par `QPrinter`

<a id="tache-l5-t27"></a>

**Objectif.** Remplir `print/src/printer.cpp` (annexe A) et `print/CMakeLists.txt` (lien `Qt6::PrintSupport` sous
`ACTCRAFT_PRINT`, `ACTCRAFT_PRINT=1` défini). DA-A2 : vérifier que `QPrinter` marche dans une application Qt Quick
**sans `QApplication`** (non vérifié ; si Qt PrintSupport l'exige, le relever, passer l'application à `QApplication`
seulement si `ACTCRAFT_PRINT` est actif et le noter en hypothèse N1 pour Jérôme), et **mesurer le démarrage** avant et
après (`actcraft.exe --quitter-apres 1`, médiane de 5 lancements, preset `release`). Feuilles : `layoutSheets`
(L5-T26), chaque feuille rendue par `renderSheet` (peintre partagé, `QImage` à la résolution de l'imprimante, passée en
gris par `convertToFormat(QImage::Format_Grayscale8)` si `gray`), puis dessinée sur le `QPrinter` ; numéros de page et
cadres ; feuilles de notes : texte des notes par le `QTextDocument` gardé du lot 2. Imprimante virtuelle qui écrit un
fichier : « Microsoft Print to PDF » avec `QPrinter::setOutputFileName` (à vérifier ; sinon
`QPrinter::PdfFormat`, et le scénario le dit). Licences : modules de Qt PrintSupport (et Widgets s'il est tiré) ajoutés
à `packaging/qt-autorises.txt`.

Point délicat (`concepteur-critique`) : parité entre l'aperçu (L5-T28) et l'impression — **une seule** fonction
compose une feuille (`renderSheet`) ; l'aperçu et l'impression l'appellent tous deux.

**Test et commande bornée.** Commande ciblée, motif `^(tst_print_printer)$` : `printsToFileThroughVirtualPrinter`
(document S17, 2 par feuille, plage 2-4, notes → fichier écrit, `countPdfPages` = 4), `grayHasNoColor` (chaque pixel
de `renderSheet` a r = g = b), `sheetCountMatchesLayout`, `worksWithoutQApplication` (test lancé sous
`QGuiApplication`). Test sauté avec `QSKIP("imprimante virtuelle absente")` seulement si `printerNames()` ne la
contient pas — et alors le rapport le dit.

**Interdits.** Aucune boîte d'impression du système (la boîte est à nous, L5-T28) ; aucune dépendance de `core/` à
QtWidgets. **Terminé quand.** 4 fonctions en `PASS` ; mesure de démarrage notée ; `LICENCES OK` ; commit « Lot 5 :
impression par QPrinter ».

**Étapes.** Tests rouges → CMake → `renderSheet` → `printDocument` → vérification sans `QApplication` → mesure →
licences → vert → commit.

### Tâche L5-T24 — `export` : import PDF

<a id="tache-l5-t24"></a>

**Objectif.** Remplir `export/src/pdfimport.cpp` selon D16 : `QPdfDocument` (sous `ACTCRAFT_IMPORT_PDF`), une page du
document par page du PDF, taille de page = taille du PDF en points × 96 / 72, rendu à `dpi`, image PNG en ressource
(clé SHA-256), `ImageObject` verrouillé sur la couche `Background`, couvrant la page ; tout sur un fil secondaire.
Sans Qt PDF : `pdfImportAvailable()` faux, `importPdf` rend `ok = false` et un message à nous.

**Test et commande bornée.** Commande ciblée, motif `^(tst_export_pdfimport)$` : `roundTripThreePages` (PDF écrit par
`exportPdf` de L5-T23 → 3 pages importées), `pageSizeFromPdf` (1024 × 768), `imageOnBackgroundLocked`,
`offMainThread`, `absentWithoutQtPdf` (compilé seulement sans l'option ; sinon `QSKIP("Qt PDF présent")` nommé).

**Interdits.** Aucun objet texte tiré du PDF (images seulement) ; aucun fichier PDF tiers commité (PDF d'essai produit
par le test). **Terminé quand.** Fonctions en `PASS` ; commit « Lot 5 : import PDF ».

**Étapes.** Tests rouges → lecture et rendu → document → vert → format → commit.

### Tâche L5-T25 — `ui` + QML : commandes d'export et d'import PDF

<a id="tache-l5-t25"></a>

**Objectif.** Remplir les méthodes d'export et d'import PDF de `ui/src/fileexchange.cpp` (`exportPdf`,
`exportPageImage`, `importPdf` → nouvel onglet par `appTabs`, `printThroughPdf` sous Android : PDF dans le dossier
temporaire puis `QDesktopServices::openUrl`, N1-15), `busy` pendant le travail, bannière du lot 2 en cas d'erreur ;
dialogues de `FileExchangeActions.qml` (`QtQuick.Dialogs`, filtres « PDF », « Images » ; les scénarios passent l'URL en
argument, sans dialogue) ; entrée « Exporter la page » du menu de page (`PageContextMenu.qml`) ; scénario
`bench/scenarios/exporter.json`.

**Test et commande bornée.** Commande ciblée, motif `^(tst_ui_fileexchange)$` : `exportPdfCommandWritesFile`,
`exportPageImageCommand`, `importPdfOpensATab`, `printThroughPdfOnAndroid` (chemin forcé par une propriété de test),
`busyWhileExporting`.

**Preuve visuelle.** `captures.ps1 -Scenario exporter … -Lire 'appExchange.busy=false' -Lire 'appTabs.count=2'` :
`CAPTURES OK` ; le scénario lit ensuite le PDF écrit (`expect` sur le nombre de pages, A13).

**Interdits.** Aucun rendu sur le fil principal ; aucun export du cache. **Terminé quand.** Test `Passed` ; captures
`CAPTURES OK` ; banc joint ; commit « Lot 5 : commandes d'export et d'import PDF ».

**Étapes.** Tests rouges → méthodes → dialogues → menu de page → scénario → captures → banc → commit.

### Tâche L5-T28 — QML : boîte d'impression, aperçu ; scénario S17

<a id="tache-l5-t28"></a>

**Objectif.** Remplir `ui/src/printcontroller.cpp` (propriétés de F-670 reliées à `core::PrintSettings` ;
`previewSheet` et `print` appellent `print::renderSheet` / `printDocument` ; réglages gardés par `QSettings`, groupe
`print`, à « Enregistrer les réglages et quitter ») et les QML `PrintDialog.qml` (imprimante, couleur ou gris, lignes,
colonnes, cadres, numéros, plage, copies, marge, notes, aperçu, « Exporter en PDF » qui appelle `appExchange.exportPdf`)
et `PrintPreview.qml` (feuilles de `previewSheet`, suivante / précédente) ; scénario `bench/scenarios/imprimer.json`
(S17 : document synthétique de 4 pages, réglages de S17, sortie vers l'imprimante virtuelle).

**Test et commande bornée.** Commande ciblée, motif `^(tst_ui_printdialog)$` : `settingsSurviveRestart`,
`previewMatchesSheets` (image de `previewSheet(i)` = `renderSheet` de la feuille i), `sheetCountFollowsSettings`.

**Preuve visuelle.** Lectures d'abord : `-Lire 'appPrint.sheetCount=4' -Lire 'appPrint.gray=true'` ; puis le scénario
vérifie le fichier écrit (4 feuilles) et un contrôle de pixels sans couleur ; relance de l'application, boîte rouverte :
`-Lire 'appPrint.rows=2'`. `CAPTURES OK`.

**Interdits.** Aucune boîte d'impression du système ; libellés à nous (aucun texte de l'aide). **Terminé quand.** Test
`Passed` ; S17 `CAPTURES OK` ; banc joint ; commit « Lot 5 : boîte d'impression et aperçu ».

**Étapes.** Tests rouges → contrôleur → boîte → aperçu → scénario → captures → banc → commit.

### Tâche L5-T29 — `export` : PowerPoint en images

<a id="tache-l5-t29"></a>

**Objectif.** Remplir `export/src/officeconvert.cpp` (H5-3) : `findConverter` (variable `ACTCRAFT_OFFICE_CONVERTER`
d'abord, puis `soffice` — `--headless --convert-to pdf --outdir {outdir} {in}` —, puis PowerPoint par
`powershell.exe -NoProfile -NonInteractive -Command` et un script à nous qui ouvre la présentation par COM et
l'enregistre en PDF) ; `convertToPdf` par `QProcess` dans un `QTemporaryDir`, délai de 120 s, erreur à nous si le PDF
n'est pas produit ; l'appelant (L5-T30) enchaîne `importPdf` (L5-T24). Le poste a PowerPoint
(`C:\Program Files\Microsoft Office\root\Office16\POWERPNT.EXE`, relevé à 794cf84), pas LibreOffice.

**Test et commande bornée.** Commande ciblée, motif `^(tst_export_officeconvert)$` : `envConverterWins`,
`argumentsSubstituted`, `timeoutReported` (convertisseur factice qui attend), `missingConverterReported`,
`fakeConverterChainGivesPages` (convertisseur factice `cmake -E copy <pdf de 2 pages produit par exportPdf> {out}` →
`importPdf` → 2 pages). Un essai réel avec PowerPoint, sur une présentation **écrite par le test** (si l'automation
sait en créer une ; sinon essai sauté et noté), est rapporté sans être une preuve de tâche.

**Interdits.** Aucun convertisseur livré ni téléchargé ; aucune présentation tierce commitée ; Windows seulement.
**Terminé quand.** 5 fonctions en `PASS` ; commit « Lot 5 : conversion de présentation en PDF ».

**Étapes.** Tests rouges → recherche → conversion → vert → format → commit.

### Tâche L5-T31 — Banc : scénarios `onglets` et `export-fond`

<a id="tache-l5-t31"></a>

**Objectif.** Deux scénarios de banc : `onglets` (ouvrir 4 documents de 20 pages, basculer 10 fois ; relever
`memory_delta_mb` = mémoire du processus après moins avant, et la durée de chaque bascule) et `export-fond` (exporter
un PDF de 20 pages pendant qu'un trait est écrit : latence d'encre et figements comme le scénario `stroke`). Seuil de
S8.5 : `memory_delta_mb` < 4 × (taille des documents en mémoire) + 40 Mo, consigné comme résultat.

**Test et commande bornée.** Commande ciblée, motif `^(tst_bench_scenarios)$` : `tabsAndExportScenariosListed` ;
banc sans écran : sections `onglets` et `export-fond` dans le JSON. **Interdits.** Mesures `debug` jamais présentées
comme preuve. **Terminé quand.** Test `Passed` ; JSON joint ; commit « Lot 5 : bancs des onglets et de l'export ».

**Étapes.** Test rouge → scénarios → branchement → vert → banc → commit.

### Tâche L5-T30 — `ui` + QML : import PowerPoint en images ; scénario `importer`

<a id="tache-l5-t30"></a>

**Objectif.** Méthode `importPowerpointImages` de `ui/src/fileexchange.cpp` (convertisseur → PDF → `importPdf` → nouvel
onglet, `busy`, bannière d'erreur), `powerpointAvailable` ; menu Fichier ▸ Importer : PDF et « Présentation en
images » seulement (N1-A8), entrées absentes si indisponibles ; scénario `bench/scenarios/importer.json` (convertisseur
factice par la variable d'environnement).

**Test et commande bornée.** Commande ciblée, motif `^(tst_ui_import)$` : `powerpointThroughFakeConverter`,
`menuHidesUnavailableImports`, `failureShowsBanner`. **Preuve visuelle.** `captures.ps1 -Scenario importer … -Lire
'appTabs.count=2' -Lire 'appDocument.pageCount=2'` : `CAPTURES OK`.

**Interdits.** Aucun autre import offert (SMART, questions, `.iwb`, `.flp` : non retenus). **Terminé quand.** Test
`Passed` ; captures `CAPTURES OK` ; banc joint ; commit « Lot 5 : import de présentations en images ».

**Étapes.** Tests rouges → méthode → menu → scénario → captures → banc → commit.

### Tâche L5-T32 — Scénario S8 `edition-export`, à l'écran

<a id="tache-l5-t32"></a>

**Objectif.** Scénario `bench/scenarios/edition-export.json` = S8 : trois formes groupées dans l'onglet 1, copier,
`tab.next` (onglet 2 créé), coller, `align.left`, `edit.undo` ; `export.pdf .tmp/s8.pdf` sur un document de 3 pages ;
`export.page-image .tmp/s8.png`. Lectures : `-Lire 'appTabs.count=2' -Lire 'appDocument.canUndo=true'` ; `expect` sur
le PDF (3 pages) et sur l'image (1024 × 768). Hors écran en rendu logiciel, puis à l'écran en D3D11 sous le jeton
`ecran` (annoncé d'une ligne à Jérôme), avec S17 (`imprimer`) rejoué à l'écran.

**Interdits.** Aucune sortie commitée (sous `.tmp/`). **Terminé quand.** Les deux passages `CAPTURES OK` ; commit
« Lot 5 : scénario S8 édition et export ».

**Étapes.** Scénario → hors écran → à l'écran → commit.

### Tâche L5-T33 — Contrôle de fin du lot

<a id="tache-l5-t33"></a>

**Objectif.** Suites complètes debug et release (`scripts/test.ps1`, forme bornée du profil → `TESTS OK`, N =
N_départ + 26) ; aucun talon du lot (`git -C <wt> grep -n "Talon L5-T1" -- . ':(exclude)docs/'` vide ;
`git -C <wt> grep -n 'QSKIP("réservé : L5-' -- tests` vide) ; captures `-Scenario tous` hors écran `CAPTURES OK` ;
banc sans écran joint ; `NOMS OK` ; titres du corpus sans sortie ; `graphify update .` puis commit de
`graphify-out/` ; liste des hypothèses H5-1 à H5-6 et des écarts du lot remise à l'orchestrateur pour la porte de fin
(banc D3D11 plein écran sous le jeton `ecran`, porte de non-régression du profil).

**Interdits.** Aucun correctif de code ici : un échec ouvre une tâche corrective `L5-C<n>`. **Terminé quand.** Toutes
les preuves de la ligne ; commit « Lot 5 : contrôle de fin, graphe à jour ».

**Étapes.** Suites → talons → captures → banc → noms et titres → graphe → commit.
