# Livraison complète — lot 4 « Outils II », sous-lot 4b (pages, navigateurs, texte, page allongée, zones) — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

## Amendement 5 (observations L4-O1 et L4-O2, découvertes des tâches, 2026-10-21)

Liste complète, avec la décision qui fonde chaque changement : plan 4a, section « Amendement 5 ». Les paragraphes
« Amendement 5 » des sections priment sur les amendements 1 à 4. Tâches mergées (L4-T15, L4-T16, L4-T22, et L4-T17,
L4-T18, L4-T21 : ccd826b, 0a54fab, 8a64872) : note « écart constaté » seulement. En cours : L4-T24 (lancée sur
e9e6e42), que cet amendement ne change pas. Ce qui change ici :
- **L4-T19** : miniatures suspendues quand le panneau des navigateurs est caché — propriété `active` ajoutée en fin de
  `PageThumbnails`, liée à `browserPanel.visible` dans `Main.qml` ; test `inactiveRendersNothing` (trouvé par L4-T17 :
  `appThumbnails` rendait panneau fermé, banc compris).
- **L4-T23** : Échap valide le texte par `Keys.onShortcutOverride` → `appText.commit()` : Échap n'atteint pas
  `TextTool::cancel()` (`ToolHost::cancelGesture` n'appelle `cancel()` que si l'outil est occupé, `TextTool` ne l'est
  jamais ; trouvé par L4-T21) ; test « Échap valide le texte » (segment final du scénario `texte`, capture `echap-valide`).
- **L4-T26** : attendu **1022,208** au lieu de 998,4 après trois clics depuis 768 (D10 cumulé, L4-O1 ; L4-T4 l'a
  appliqué).
- **L4-T27** : taille de cadre d'au moins 500 (H4-20) dans `clampZones` ; clé `zones` dans le gabarit `everyPageKey`
  du format natif (trouvé par L4-T6), test `zonesKeyIsKnown` ; séparations sans effet sur l'accrochage, grille partie
  de l'origine de la page (H4-21), test `separatorsDoNotAttract`.
- **L4-T28** : **n'est plus suspendue** (L4-O2 a observé D11) : séparations fines en pointillé aux limites des cadres,
  en surcouche, en édition et en plein écran, couleur tirée de notre thème ; elle dépend de L4-O2 au lieu de L4-O1.
- **L4-T29** : banc des deux cas, panneau fermé (`miniatures-ferme`) et ouvert (`miniatures`).
- **L4-C1** : taille de page = nombre de cadres × taille d'un cadre (observé, L4-O2) ; 2 × 3 cadres de 400 × 300
  donnent des cadres de 500 × 500 et une page de 1000 × 1500 (H4-20).
- **Correctif L4-C2 ajouté** (trouvé par L4-T21) : le contrôleur des pages valide l'édition en cours avant toute
  commande de pages ; V12.
- **Correctif L4-C4 ajouté** (trouvé par L4-T15) : un objet de type inconnu s'écrit avec son identifiant courant ;
  V9 bis, seule, après le merge de L4-T24 et avant L4-T27.
- Hypothèses H4-20 à H4-24 : plan 4a. 19 tâches dans ce sous-lot ; **N inchangé** (= 136 + n_B) : L4-C2 et L4-C4
  ajoutent des fonctions à `tst_ui_text` et `tst_formats_lot4`, déjà enregistrés.

## Amendement 4 (résultats de L4-T0, 2026-10-21)

Liste complète, avec la décision qui fonde chaque changement : plan 4a, section « Amendement 4 ». Ce qui change ici
(chaque section concernée porte un paragraphe « Amendement 4 », qui prime sur les amendements 1 à 3) :
- **L4-T17** : aucun état de contact n'est publié (V5) → `contactActive` ajouté en fin de `pageview.h`, signalé.
- **L4-T20** : boîte du fond à trois boutons, annuler, appliquer, valider (D15).
- **L4-T21** : validation sur `editAboutToApply` **et** `pageAboutToChange` (V6) ; texte existant vidé supprimé en une
  étape (D7) ; taille ±2 points, minimum 8 ; interligne ±2 points d'espace au-dessus du paragraphe, minimum 0 (D8) ;
  texte verrouillé non édité (P17).
- **L4-T23** : Échap valide le texte au lieu de l'annuler (D7) ; Ctrl+B, Ctrl+I, Ctrl+U pris par l'éditeur (D8).
- **L4-T24** : un texte de haut en bas d'un `.flipchart` se lit par son cadre et son image rendue (D9).
- **L4-T27** : cadres en largeur de 0 à 40, en hauteur de 0 à 53, taille d'un cadre stockée dans `PageZones`, dans
  l'empreinte et dans le format natif ; l'allongement ne la modifie pas (D11).
- **L4-T28** : **suspendue jusqu'à l'observation à l'écran (D11)**, tâche L4-O1 du plan 4a.
- **L4-C1** ajoutée (D16) : taille personnalisée d'un paperboard neuf en pixels d'un cadre et en nombre de cadres ;
  vague V12, après L4-T26, L4-T27 et L4-O1.
- **L4-T26** attend L4-O1 (règle de l'allongement, D10) : L4-O1 a pour échéance le début de V9.
- Hypothèses : H4-11 à H4-17 au plan 4a ; l'hypothèse de ce plan numérotée H4-11 (fond en image) devient **H4-18**.
- Citation corrigée : garde `loadResource` en `render/src/pagepainter.cpp:73` (et non `:71`).
- 17 tâches dans ce sous-lot ; N inchangé (= 136 + n_B) : L4-C1 ajoute une fonction à `tst_ui_filemenu`, déjà enregistré.

## Amendement 3 (relecture tour 3, 2026-10-19)

Liste complète (M1, m1 à m6) : plan 4a, section « Amendement 3 ». Ce qui change ici :
- **M1** et **m1** (L4-T21) : attendus de `editingLoadsNoExternalResource` réécrits (le DOCTYPE de `toHtml()` cite
  `http://www.w3.org/…` ; les images de fond passent aussi par `loadResource`) ; forme CSS `background-image` ajoutée.
- **m4** (L4-T17, L4-T29) : `launchCount` (posé par L4-T1) compte les rendus lancés ; `during_contact` se compte sur
  lui, plus sur `renderCount`.
- **m6** : place du groupe `formes-texte` = D19 de L4-T0 ; décisions D1 à D19.

## Amendement 2 (relecture tour 2, 2026-10-18)

Liste complète des défauts N1 à N13 : plan 4a, section « Amendement 2 ». Ce qui change ici :
- **N4** : `git add` de `ui/src/documentcontroller.cpp` (L4-T21, si modifié) et de `tests/tst_ui_theme.cpp` (L4-T19).
- **N5** : `safeTextHtml` retire aussi `QTextFormat::BackgroundImageUrl` des formats de bloc, de cadre et de cellule ;
  cas ajouté à `editingLoadsNoExternalResource`.
- **N7** : phrase de N réécrite. **N9** : règle de sécurité du fichier d'état citée par son texte.
- **N10** : source de `thumbnails.during_contact` (L4-T29). **N11** : L4-T26 dépend de L4-T21. **N12** : n_B relevé de
  nouveau si le lot 7 fusionne en cours de lot (clôture).

## Amendement 1 (relecture tour 1, 2026-10-18)

La liste complète des défauts et des changements est dans le plan 4a, section « Amendement 1 ». Ce qui change ici :
- **N = 136 + n_B** partout (défaut 13), à la place de l'ancienne règle « N au moins égal à N3 + 23 ».
- **L4-T17** : `contactActive` (en-tête `pageview.h` figé par L0-S7 : dériver de l'état du routeur s'il est publié,
  sinon ajout en fin d'en-tête signalé) ; remise à zéro sur tout `PageInserted` ou `PageRemoved`, test
  `moveRerendersShiftedRows` (défaut 11).
- **L4-T19** : les 5 icônes des navigateurs entrent dans `kIcons` (`tests/tst_ui_theme.cpp`).
- **L4-T20** : menu de page = `commands.contextMenu` (socle, étendu aux sous-menus par L4-T1) ; fond en image selon
  l'API d'insertion du lot 7 (D-S8), H4-18 (numérotée H4-11 avant l'amendement 4).
- **L4-T21** : HTML assaini (bloquant 5, `editingLoadsNoExternalResource`) ; validation avant tout dépôt forcé
  (défaut 12, trois tests) ; espion par `addChangeObserver` (défaut 6) ; texte du lot 7 posé par son API d'insertion
  (D-S8).
- **L4-T22** : argument de `text.edit-at` en texte `"x;y;v"`, absent pour valider (bloquant 2).
- **L4-T23** : D8 étendue (raccourcis pendant la frappe) ; jamais de texte riche sans HTML assaini.
- **L4-T26** : S15 « la poignée suit » reçoit une lecture `expect`.
- **L4-T27** : `SetPageZonesCommand` émet `PageSettingsChanged` (L0-S3) ; `pagezones.*` et `core/CMakeLists.txt`
  réservés par L4-T1.
- **L4-T29** : attendus chiffrés du banc (défaut 16).
- **Clôture** : règle 4 des talons du socle (défaut 14) ; attendus chiffrés ; décisions D1 à D19.

**But.** Fin du lot 4, version **0.5.0** : panneau des navigateurs et navigateur de pages (miniatures à jour,
sélection multiple, réordonner, insérer, couper / copier / coller / dupliquer / supprimer des pages), menu de page,
fond de page, texte et barre de mise en forme, texte de haut en bas, page allongée, zones de page, scénario S7
complet, scénario de banc `miniatures`, clôture du lot. Le sous-lot 4a
(`docs/plans/2026-10-18-livraison-complete-lot4-PLAN.md`) fait autorité sur tout ce qui est commun : architecture,
API supposées A1 à A14 (à revérifier à l'ouverture, L4-T0), contrats et noms figés, en-têtes posés par L4-T1
(sa section, étape 6), carte des fichiers, **règles communes** (jointes à chaque section par l'orchestrateur),
hypothèses H4-1 à H4-17 (amendement 4). Rien n'y est répété ici.

**Point de départ.** Le point d'étape de 4a est passé : V1 à V6 fusionnées dans `feat/livraison-complete`, talons
restants ceux de 4b seulement, N = 136 + n_B. Les décisions D1 à D19 de L4-T0 ont amendé les deux plans.

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L4-T15 | `core` : commandes de pages (déplacer, retirer, insérer des copies à identifiants neufs) | L4-T0, L4-T1 | `core/include/actcraft/core/pageedits.h`, `core/src/pageedits.cpp`, `tests/tst_core_pageedits.cpp` | isolé | concepteur-critique | `TESTS OK`, N = 136 + n_B, `tst_core_pageedits` Passed (dont `moveTwoPagesToFront`, `removeRefusesAllPages`, `insertGivesFreshIds`, `groupMembersFollowFreshIds`, `connectorEndsFollow`, `redoKeepsTheSameIds`, `cacheFollowsTheShownPage`) ; mutation de `withFreshIds` (identifiants gardés) rouge puis remise | S7, S7.4 | #tache-l4-t15 |
| L4-T16 | `core` : commande de texte et ligne de base | L4-T0, L4-T1 | `core/include/actcraft/core/textedits.h`, `core/src/textedits.cpp`, `tests/tst_core_textedits.cpp` | isolé | implementeur-tdd | `TESTS OK`, `tst_core_textedits` Passed (dont `setTextIsOneStep`, `undoRestoresHtmlAndBounds`, `notATextIsIgnored`, `baselineFromFirstRun`, `noRunsNoBaseline`) | S7.3 | #tache-l4-t16 |
| L4-T17 | `ui` : miniatures des pages rendues hors du fil principal, regroupées, jamais pendant un contact | L4-T1 | `ui/include/actcraft/ui/pagethumbnails.h`, `ui/src/pagethumbnails.cpp`, `ui/include/actcraft/ui/pageview.h` (`contactActive` ajouté en fin, signalé, V5), `ui/src/pageview.cpp`, `tests/tst_ui_pagethumbnails.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_ui_pagethumbnails` Passed (dont `rendersAfterSettling`, `oneRenderForManyEdits`, `postponedDuringContact`, `renderRunsOffTheMainThread`, `rowsFollowPages`, `moveRerendersShiftedRows`, `staleRenderIsDropped`), `tst_pageview` Passed ; banc hors écran joint | S7, S7.4 | #tache-l4-t17 |
| L4-T18 | `ui` : contrôleur des pages (aller, sélection multiple, presse-papiers, insérer, déplacer, dupliquer, supprimer) | L4-T15 | `ui/include/actcraft/ui/pagescontroller.h`, `ui/src/pagescontroller.cpp`, `tests/tst_ui_pages.cpp` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_pages` Passed (dont `clickSelectsAndShows`, `shiftSelectsRange`, `ctrlToggles`, `moveKeepsSelection`, `pasteInsertsBeforeActive`, `cutThenPasteMoves`, `duplicateAfterSelection`, `deleteRefusesLastPage`, `oneUndoPerCommand`) | S7, S7.4 | #tache-l4-t18 |
| L4-T21 | `ui` : édition du texte (soulèvement, validation, texte vide, barre de mise en forme) | L4-T16 | `ui/include/actcraft/ui/textediting.h`, `ui/src/textediting.cpp`, `tests/tst_ui_text.cpp`, `ui/src/documentcontroller.cpp` (attendu inchangé, V6 : seulement si un dépôt forcé n'émet ni `editAboutToApply` ni `pageAboutToChange`) | isolé | concepteur-critique | `TESTS OK`, `tst_ui_text` Passed (dont `editingLoadsNoExternalResource`, `saveDuringEditCommitsFirst`, `undoDuringEditCommitsFirst`, `pageChangeDuringEditCommitsFirst`, `newTextStartsWithPlaceholderSelected`, `typingThenCommitAddsOneObject`, `emptyTextIsNotKept` (texte neuf et texte existant vidé), `editingExistingLiftsIt`, `commitMakesASingleStep`, `boldAppliesToSelection`, `sizeUpStepsByTwo`, `sizeDownStopsAtEight`, `spacingAddsTwoPointsAbove`, `lockedTextIsNotEdited`, `textOfLot7IsEditable`, `cancelRestoresLifted`) ; banc hors écran joint | S7.3 | #tache-l4-t21 |
| L4-T22 | `tools` : outils Texte et Texte de haut en bas | L4-T1 | `tools/include/actcraft/tools/texttool.h`, `tools/src/texttool.cpp`, `tests/tst_tools_text.cpp` | isolé | implementeur-tdd | `TESTS OK`, `tst_tools_text` Passed (dont `pressAsksToEditAt`, `verticalToolSaysVertical`, `deactivateCommits`, `neverBusy`) | S7.3, S16 | #tache-l4-t22 |
| L4-T19 | QML : panneau des navigateurs (icônes, ancrage, Ctrl+B, largeur) et navigateur de pages ; miniatures suspendues panneau caché (amendement 5) ; scénario `navigateur-pages` | L4-T17, L4-T18 | `ui/qml/BrowserPanel.qml`, `ui/qml/PageNavigator.qml`, `ui/icons/{nav-ressources,nav-objets,nav-notes,nav-proprietes,nav-actions}.svg`, `ui/CMakeLists.txt`, `tests/tst_ui_theme.cpp`, `bench/scenarios/navigateur-pages.json`, `ui/include/actcraft/ui/pagethumbnails.h` (ajout en fin, signalé), `ui/src/pagethumbnails.cpp`, `tests/tst_ui_pagethumbnails.cpp`, `ui/qml/Main.qml` (une ligne) | isolé | implementeur-tdd | `TESTS OK`, `tst_captures_navigateur_pages` Passed, `tst_ui_theme` (`iconsAreOurSvgOnA24Grid` : 5 lignes neuves en `PASS`), `tst_ui_pagethumbnails` (dont `inactiveRendersNothing`, les fonctions de L4-T17 telles qu'après L4-C10 inchangées) Passed ; `captures.ps1 -Scenario navigateur-pages` hors écran : `LU appThumbnails.active=false OK` avant `view.browsers`, `LU browserPanel.visible=true OK`, `LU appThumbnails.active=true OK`, `LU appPages.selection=[0,1] OK`, `LU appDocument.pageCount=5 OK`, `CAPTURES OK` ; banc hors écran joint | S7.4 | #tache-l4-t19 |
| L4-T24 | Texte de haut en bas I : rendu (colonne d'un caractère) et lecture `.flipchart` par le cadre et l'image (D9) | L4-T0, L4-T6 | `render/src/pagepainter.cpp`, `render/src/paintedrenderer.cpp`, `tests/tst_render_verticaltext.cpp`, `formats/src/flipchart.cpp`, `formats/src/flipchartids.h`, `tests/tst_formats_lot4.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_render_verticaltext` (dont `lettersStackOneUnderAnother`, `boldIsKept`, `sameInPainterAndTexture`), `tst_formats_lot4` (dont `flipchartVerticalTextRead` : image au cadre, ou objet non lu déclaré) Passed ; `tst_formats_oracle_synth` Passed ; banc hors écran joint | S16 | #tache-l4-t24 |
| L4-T26 | Page allongée : poignée, commande, une étape par clic ; scénario S15 (partie 1) | L4-T4, L4-T13, L4-T17, L4-T21, L4-O1 (plan 4a, D10) | `ui/qml/PageExtensionHandle.qml`, `ui/src/pageoptions.cpp`, `tests/tst_ui_pagesettings.cpp`, `ui/src/documentcontroller.cpp`, `ui/src/pageview.cpp`, `bench/scenarios/page-allongee-zones.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_pagesettings` (dont `extendPageThreeTimes`, `extendKeepsTheCurrentTool`, `viewFollowsTheNewHeight`), `tst_captures_page_allongee_zones` Passed ; captures hors écran : `LU appPageSettings.pageHeight=1022.208 OK` (amendement 5, D10 cumulé), `LU pageExtensionHandle.atPageBottom=true OK`, `LU appTools.currentTool=pen OK`, `LU appPageSettings.pageHeight=768 OK`, `CAPTURES OK` ; banc hors écran joint | S15 | #tache-l4-t26 |
| L4-T20 | Menu de page appliqué aux pages sélectionnées ; boîte « Fond de la page » (E2) | L4-T19, L4-T26 | `ui/qml/PageContextMenu.qml`, `ui/qml/BackgroundDialog.qml`, `ui/src/pageoptions.cpp`, `tests/tst_ui_pagesettings.cpp`, `bench/scenarios/navigateur-pages.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_pagesettings` (dont `backgroundColorIsOneStep`, `backgroundGradient`, `backgroundRefreshesOnce`, `eachApplyIsOneStep`), `tst_captures_navigateur_pages` Passed ; captures hors écran (D15, trois boutons) : `LU backgroundDialog.visible=true OK` après « appliquer », `PIXEL fond-jaune OK`, `LU backgroundDialog.visible=false OK` après « annuler », fond toujours jaune, `CAPTURES OK` | S7.4 | #tache-l4-t20 |
| L4-T23 | QML : éditeur de texte et barre de mise en forme ; scénario `texte` | L4-T21, L4-T22 | `ui/qml/TextEditor.qml`, `ui/qml/TextFormatBar.qml`, `bench/scenarios/texte.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_captures_texte` Passed ; captures hors écran : `LU appText.active=true OK`, `TEXTE Bonjour OK`, `LU appText.bold=true OK`, `LU browserPanel.visible=false OK` (Ctrl+B pendant la frappe, D8), `LU appDocument.shownObjectCount=1 OK`, `LU appText.active=false OK` après Échap (validé, D7), `CAPTURES OK` ; banc hors écran joint | S7.3 | #tache-l4-t23 |
| L4-T27 | Zones de page I : modèle, commande, natif (et `.flipchart` si D11 l'a relevé) | L4-T0, L4-T24, L4-C4 (amendement 5 : mêmes fichiers du format natif) | `core/include/actcraft/core/model.h`, `core/src/model.cpp`, `core/include/actcraft/core/pagezones.h`, `core/src/pagezones.cpp`, `tests/tst_core_zones.cpp`, `formats/src/nativejson.cpp`, `formats/src/flipchart.cpp`, `formats/src/flipchartids.h`, `tests/tst_formats_lot4.cpp` | isolé | concepteur-critique | `TESTS OK`, `tst_core_zones` (dont `zonesKeepOldFingerprints`, `setZonesIsOneStep`, `zonesChangeIsIgnoredByTheCache`, `zonesAreBounded` (0 à 40 en largeur, 0 à 53 en hauteur, D11 ; tailles négatives ou non finies ramenées à 0 ; taille non nulle d'au moins 500, amendement 5, H4-20), `extendKeepsTheFrameSize`, `separatorsDoNotAttract` (amendement 5, H4-21)), `tst_formats_lot4` (dont `zonesRoundTrip`, taille d'un cadre comprise, et `zonesKeyIsKnown`, amendement 5) Passed | S15 | #tache-l4-t27 |
| L4-T25 | Texte de haut en bas II : outil, barre, scénario `texte-vertical` (S16) | L4-T23, L4-T24 | `ui/qml/VerticalTextBar.qml`, `ui/src/textediting.cpp`, `tests/tst_ui_text.cpp`, `bench/scenarios/texte-vertical.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_text` (dont `verticalFrameIsOneColumn`, `verticalBarOffersFiveCommands`), `tst_captures_texte_vertical` Passed ; captures hors écran : `LU appText.vertical=true OK`, `PIXEL lettre-a OK`, `PIXEL lettre-b OK`, `PIXEL lettre-c OK`, `LU appDocument.fingerprint=${avant} OK`, `CAPTURES OK` | S16 | #tache-l4-t25 |
| L4-T28 | Zones de page II : séparations pointillées des cadres en surcouche (édition et plein écran), réglage ; scénario S15 (partie 2) — amendement 5 : plus suspendue | L4-T20, L4-T26, L4-T27, L4-O2 (plan 4a) | `ui/qml/PageZonesOverlay.qml`, `ui/qml/PageArea.qml`, `ui/qml/Main.qml` (une ligne), `ui/qml/Theme.qml` (un jeton ajouté), `ui/src/pageoptions.cpp`, `tests/tst_ui_pagesettings.cpp`, `bench/scenarios/page-allongee-zones.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_ui_pagesettings` (dont `zonesRefreshNoCache`, `nextPageIgnoresFrames`), `tst_captures_page_allongee_zones` Passed ; captures hors écran : `LU appPageSettings.zones=2x0 OK`, `BORD separation-absente OK` avant le réglage, `LU pageZones.separatorCount=1 OK`, `BORD separation-zone OK` (`checkEdge`, au moins un pixel de pointillé), `LU pageZones.separatorCount=3 OK` (2 × 3 cadres), `LU pageZones.visible=true OK` en plein écran, `LU appDocument.fingerprint=${avant} OK`, `CAPTURES OK` ; banc hors écran joint | S15 | #tache-l4-t28 |
| L4-T29 | Banc : scénarios `miniatures` (écrire, panneau des navigateurs ouvert) et `miniatures-ferme` (panneau fermé, amendement 5) | L4-T19 | `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp`, `bench/src/runner.cpp`, `bench/include/actcraft/bench/result.h`, `bench/src/result.cpp`, `tests/tst_bench_scenarios.cpp`, `tests/tst_bench_result.cpp`, `docs/banc.md` | isolé | implementeur-tdd | `TESTS OK`, `tst_bench_scenarios` (dont `thumbnailsScenarioOpensTheBrowsers`, `closedPanelVariantKeepsItClosed`), `tst_bench_result` (dont `thumbnailsShowInTheResult`) Passed ; banc hors écran : scénario `miniatures` présent, `thumbnails.renders ≥ 1` **dans la fenêtre mesurée**, `thumbnails.during_contact = 0`, `thumbnails.max_copy_ms` relevé ; scénario `miniatures-ferme` présent, `thumbnails.renders = 0` dans la fenêtre mesurée ; variante sur `pauses` jouée | S7.5 | #tache-l4-t29 |
| L4-T30 | Scénario S7 `formes-texte-pages-grille`, rejoué à l'écran en D3D11 | L4-T12, L4-T13, L4-T20, L4-T23 | `bench/scenarios/formes-texte-pages-grille.json` | isolé | implementeur-tdd | `TESTS OK`, `tst_captures_formes_texte_pages_grille` Passed ; hors écran `CAPTURES OK` (`LU appThumbnails.renderCount=…` en hausse après le texte, `LU appDocument.currentIndex=0 OK` après le glisser, `PIXEL coin-accroche OK`) ; à l'écran D3D11 (jeton `ecran`) `CAPTURES OK` | S7 | #tache-l4-t30 |
| L4-C1 | Correctif : taille personnalisée d'un paperboard neuf en pixels d'un cadre et en nombre de cadres (D16, E10) ; taille de page = cadres × cadre, cadre de 500 au moins (amendement 5) | L4-T26, L4-T27, L4-O1, L4-O2 (plan 4a) | `ui/qml/PageSizeDialog.qml`, `ui/qml/FileActions.qml`, `ui/include/actcraft/ui/documentcontroller.h` (ajout en fin, signalé), `ui/src/documentcontroller.cpp`, `tests/tst_ui_filemenu.cpp` | isolé | implementeur-tdd | `TESTS OK`, N inchangé (= 136 + n_B), `tst_ui_filemenu` Passed (dont `customSizeTakesFrames` par `acceptFramedSize` : page de 1000 × 1500, cadres de 500 × 500, `newSizesComeFromTheirCommands` inchangé) ; `premiers-pas` hors écran `CAPTURES OK` | S15 | #tache-l4-c1 |
| L4-C2 | Correctif de L4-T18 : le contrôleur des pages valide l'édition du texte avant toute commande de pages (amendement 5) | L4-T18, L4-T21, L4-T25 (fichier `tests/tst_ui_text.cpp`) | `ui/src/pagescontroller.cpp`, `tests/tst_ui_text.cpp` | isolé | concepteur-critique | rouge d'abord (`duplicateDuringEditKeepsTheText`, `cutDuringEditKeepsTheText`, `deleteDuringEditCommitsFirst` en `FAIL!` sur le code d'avant), puis `TESTS OK`, N = 136 + n_B inchangé (aucun test enregistré), `tst_ui_text` Passed (dont `duplicateDuringEditKeepsTheText`, `cutDuringEditKeepsTheText`, `deleteDuringEditCommitsFirst`, et `pageChangeDuringEditCommitsFirst` de L4-T21 inchangé), `tst_ui_pages` Passed (fonctions de L4-T18 inchangées) ; banc hors écran joint | S7, S7.3, S7.4 | #tache-l4-c2 |
| L4-C4 | Correctif de L4-T6 et L4-T15 : un objet de type inconnu s'écrit avec son identifiant courant (amendement 5) | L4-T6, L4-T15, L4-T24 (fichier `tests/tst_formats_lot4.cpp`) | `formats/src/nativejson.cpp`, `tests/tst_formats_lot4.cpp` | isolé | concepteur-critique | rouge d'abord (`unknownObjectWritesItsCurrentId` en `FAIL!` sur le code d'avant), puis `TESTS OK`, N = 136 + n_B inchangé (aucun test enregistré), `tst_formats_lot4` Passed (dont `unknownObjectWritesItsCurrentId`, fonctions de L4-T6 inchangées), `tst_formats_nativejson` et `tst_formats_nativefile` Passed (objets inconnus du lot 2 gardés tels quels) | S7, S7.4 | #tache-l4-c4 |

17 tâches (T15 à T30, et L4-C1 par l'amendement 4). Aucune n'enregistre de test (L4-T1 l'a fait ; L4-C1 ajoute une
fonction à `tst_ui_filemenu`, déjà enregistré) : N = 136 + n_B partout, `TESTS OK`. **Amendement 5 :** 19 tâches
(L4-C2 et L4-C4 ajoutées) ; elles n'enregistrent aucun test non plus : N inchangé.

**Écran.** Seules L4-T30 et la clôture ouvrent des fenêtres visibles (D3D11), sous le jeton `ecran`, annoncées à
Jérôme d'une ligne avant.

## Vagues (largeur 3 au plus ; 2 places de tests)

V7 : L4-T15 ‖ L4-T16 ‖ L4-T17 · V8 : L4-T18 ‖ L4-T21 ‖ L4-T22 · V9 : L4-T19 ‖ L4-T24 ‖ L4-T26 ·
V10 : L4-T20 ‖ L4-T23 ‖ L4-T27 · V11 : L4-T25 ‖ L4-T28 ‖ L4-T29 · V12 : L4-T30 ‖ L4-C1 · puis la clôture.

**Amendement 5.** V7 et V8 sont mergées (L4-T15, L4-T16, L4-T17, L4-T18, L4-T21, L4-T22). L4-T24 est en cours
(lancée sur e9e6e42). **L4-C4 joue en V9 bis, seule**, après le merge de L4-T24 et avant L4-T27 (V10) : ses deux
fichiers (`formats/src/nativejson.cpp`, `tests/tst_formats_lot4.cpp`) sont aussi ceux de L4-T24
(`tst_formats_lot4.cpp`) et de L4-T27 (les deux) ; elle dépend donc de L4-T24, et L4-T27 dépend d'elle. Elle ne
recoupe aucune autre tâche de V9 (L4-T19 : QML, icônes, `pagethumbnails.*`, `Main.qml` ; L4-T26 : `pageoptions.cpp`,
`tst_ui_pagesettings.cpp`, `documentcontroller.cpp`, `pageview.cpp`, scénario) : si L4-T24 est mergée avant que V9 ne
finisse, L4-C4 peut partir aussitôt, en parallèle de L4-T19 et L4-T26.
V12 devient **L4-T30 ‖ L4-C1 ‖ L4-C2** : L4-C2 dépend de L4-T18 et L4-T21 (V8) et suit L4-T25 (V11) sur
`tests/tst_ui_text.cpp` ; V9, V10 et V11 sont pleines (trois tâches). Ses fichiers (`pagescontroller.cpp`,
`tst_ui_text.cpp`) ne recoupent ni `formes-texte-pages-grille.json` (L4-T30) ni ceux de L4-C1 (`PageSizeDialog.qml`,
`FileActions.qml`, `documentcontroller.*`, `tst_ui_filemenu.cpp`). L4-T28 attend L4-O2 (mergée) et non plus L4-O1.
V9 attend aussi la fin de R3 de 4a (L4-T26 dépend de L4-T13).

**Amendement 4.** L4-T26, L4-T28 et L4-C1 attendent L4-O1 (plan 4a, séance d'observation à l'écran) : son échéance
est le début de V9, et l'orchestrateur ne lance pas V9 avant son rapport ; les verdicts « contredit » amendent les
sections touchées avant leur lancement (correctif `L4-C<n>` nommé par tâche pour ce qui est déjà fait).
L4-C1 en V12 : ses fichiers (`PageSizeDialog.qml`, `FileActions.qml`, `documentcontroller.*`, `tst_ui_filemenu.cpp`)
ne recoupent pas `formes-texte-pages-grille.json` (L4-T30) ;
`ui/src/documentcontroller.cpp` est tenu par les dépendances (L4-T21 → L4-T26 → L4-C1).

Fichiers partagés tenus par les dépendances : `ui/src/pageoptions.cpp`, `tests/tst_ui_pagesettings.cpp`
(L4-T13 → L4-T26 → L4-T20 → L4-T28) ; `bench/scenarios/navigateur-pages.json` (L4-T19 → L4-T20) ;
`bench/scenarios/page-allongee-zones.json` (L4-T26 → L4-T28) ; `ui/src/textediting.cpp`, `tests/tst_ui_text.cpp`
(L4-T21 → L4-T25) ; `tests/tst_formats_lot4.cpp`, `formats/src/flipchart.cpp`, `flipchartids.h` (L4-T6 → L4-T24 →
L4-T27) ; `formats/src/nativejson.cpp` (L4-T6 → L4-T27) ; `ui/src/pageview.cpp` (L4-T17 → L4-T26) ; `ui/src/documentcontroller.cpp` (L4-T21 → L4-T26, dépendance déclarée par l'amendement 2) ;
`tests/tst_ui_theme.cpp` (L4-T1 → L4-T19) ; `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp`
(L4-T1 → L4-T21 → L4-T26 → L4-C1) ;
`bench/src/scenarios.cpp`, `runner.cpp`, `tst_bench_*` (L4-T14 → L4-T29) ; `ui/CMakeLists.txt` (L4-T2 → L4-T19).
**Amendement 5 :** `formats/src/nativejson.cpp`, `tests/tst_formats_lot4.cpp` (L4-T6 → L4-T24 → L4-C4 →
L4-T27) ;
`ui/src/pagescontroller.cpp` (L4-T18 → L4-C2) ; `tests/tst_ui_text.cpp` (L4-T21 → L4-T25 → L4-C2) ;
`ui/include/actcraft/ui/pagethumbnails.h`, `ui/src/pagethumbnails.cpp`, `tests/tst_ui_pagethumbnails.cpp` (L4-T17 →
L4-T19) ; `ui/qml/Main.qml` (L4-T2 → L4-T19 → L4-T28) ; `ui/qml/PageArea.qml` (L4-T2 → L4-C3 → L4-T7 de 4a →
L4-T28) ; `ui/qml/Theme.qml` (L4-T28 seule dans ce lot).
Vérifié vague par vague : aucun fichier commun à deux tâches d'une même vague.

**D11 et les zones.** L4-T27 et L4-T28 ne commencent que si D11 (L4-T0) a tranché le comportement des zones. Sinon
elles passent en hypothèse de niveau 1 (H4-4), S15 est joué sans sa partie « zones », et l'écart est porté à la
porte de fin du lot. **Amendement 4 :** D11 a tranché la donnée (un cadre : nombre de cadres en largeur de 0 à 40, en
hauteur de 0 à 53, taille d'un cadre en pixels), pas le dessin ni l'effet sur les objets, l'accrochage et la
présentation. L4-T27 commence ; L4-T28 attend L4-O1, et l'orchestrateur amende sa section d'après le rapport avant de
la lancer ; si L4-O1 rend « non observé », L4-T28 ne se joue pas (H4-4, S15 sans sa partie 2, écart à la porte).
**Amendement 5 :** L4-O2 a observé le dessin, la présentation, l'effet sur un objet déplacé et sur l'accrochage, et la
taille de page de la boîte de taille personnalisée. L4-T28 se joue (section amendée), S15 complet.

---

## Tâches

<a id="tache-l4-t15"></a>
### Tâche L4-T15 — `core` : commandes de pages

**Écart constaté (amendement 5).** Mergée. Son rapport a relevé qu'une page collée qui contient un objet de type
inconnu reçoit un identifiant neuf en mémoire (`withFreshIds`) mais garde l'ancien dans le fichier, le format natif
réécrivant cet objet depuis son JSON d'origine : correctif **L4-C4** (format natif), rien à reprendre ici.

**Objectif.** Remplir `pageedits` (en-tête de L4-T1). **Le patron évident est le mauvais** : coller une copie de page
telle quelle dupliquerait les identifiants d'objets (règle du document : uniques dans tout le document,
`hasUniqueIds`) ; renuméroter sans suivre les références casserait groupes (`Group::members`), connecteurs
(`objectA`, `objectB`) et actions qui visent un objet de la page (`Action::target`). `withFreshIds` construit la
table ancien → neuf sur toute la page, puis réécrit chaque référence interne ; une référence vers un objet hors de la
page reste telle quelle. `InsertPagesCommand` renumérote **au premier redo seulement** : un rétablir après annuler
rend les mêmes identifiants (le cache et la sélection s'y fient).

**Fichiers.** `core/include/actcraft/core/pageedits.h`, `core/src/pageedits.cpp`, `tests/tst_core_pageedits.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_core_pageedits` ; fonctions de la table en `PASS` ; mutation ;
preuve de tâche.

**Interdits.** Pas de QtGui. Pas de changement de `AddPageCommand` ni de `Document::insertPage` / `takePage`.

**Terminé quand.** Fonctions vertes ; mutation : `withFreshIds` qui rend la page sans renuméroter → rouge sur
`insertGivesFreshIds` ; remise (règle « Mutation » : date du fichier remise, ou `git checkout --`) → vert ;
`TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge.** `tests/tst_core_pageedits.cpp` :
  ```cpp
  #include <actcraft/core/document.h>
  #include <actcraft/core/pagecache.h>
  #include <actcraft/core/pageedits.h>
  #include <actcraft/core/undo.h>

  #include <QtTest>

  using namespace actcraft::core;

  namespace {
  Document pages(int n)
  {
      Document d;
      for (int i = 1; i < n; ++i)
          d.appendPage();
      for (std::size_t i = 0; i < d.pageCount(); ++i)
          d.page(i).name = std::to_string(i + 1);
      return d;
  }
  std::string names(const Document& d)
  {
      std::string s;
      for (std::size_t i = 0; i < d.pageCount(); ++i)
          s += d.page(i).name;
      return s;
  }
  PageObject shape(ObjectId id)
  {
      PageObject o{Shape{}};
      o.common.id = id;
      return o;
  }
  struct Spy final : PageCache
  {
      int calls = 0;
      void insert(ObjectId) override { ++calls; }
      void remove(ObjectId, const RectF&) override { ++calls; }
      void invalidate(ObjectId, const CacheArea&) override { ++calls; }
  };
  } // namespace

  class TstCorePageEdits : public QObject
  {
      Q_OBJECT

  private slots:
      void moveTwoPagesToFront()
      {
          Document d = pages(4);
          UndoStack stack;
          stack.push(d, std::make_unique<MovePagesCommand>(std::vector<std::size_t>{2, 3}, 0));
          QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("3412"));
          QCOMPARE(positionsAfterMove(4, {2, 3}, 0), (std::vector<std::size_t>{0, 1}));
          stack.undo(d);
          QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("1234"));
      }
      void moveWithoutEffectLeavesTheOrder()
      {
          Document d = pages(3);
          UndoStack stack;
          const Changes c = stack.push(d, std::make_unique<MovePagesCommand>(std::vector<std::size_t>{1}, 1));
          QVERIFY(c.empty());
          QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("123"));
      }
      void removeRefusesAllPages()
      {
          Document d = pages(2);
          UndoStack stack;
          stack.push(d, std::make_unique<RemovePagesCommand>(std::vector<std::size_t>{0, 1}));
          QCOMPARE(d.pageCount(), std::size_t{2});
          stack.push(d, std::make_unique<RemovePagesCommand>(std::vector<std::size_t>{0}));
          QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("2"));
          stack.undo(d);
          QCOMPARE(QString::fromStdString(names(d)), QStringLiteral("12"));
      }
      void insertGivesFreshIds()
      {
          Document d = pages(1);
          d.page(0).objects = {shape(1), shape(2)};
          d.refreshNextId();
          UndoStack stack;
          stack.push(d, std::make_unique<InsertPagesCommand>(1, std::vector<Page>{d.page(0)}));
          QCOMPARE(d.pageCount(), std::size_t{2});
          QVERIFY(hasUniqueIds(d));
      }
      void groupMembersFollowFreshIds()
      {
          Document d = pages(1);
          PageObject group{Group{{1, 2}}};
          group.common.id = 3;
          d.page(0).objects = {shape(1), shape(2), group};
          d.refreshNextId();
          const Page copy = withFreshIds(d, d.page(0));
          const auto& members = std::get<Group>(body(copy.objects[2])).members;
          QCOMPARE(members[0], copy.objects[0].common.id);
          QCOMPARE(members[1], copy.objects[1].common.id);
      }
      void connectorEndsFollow()
      {
          Document d = pages(1);
          Connector link;
          link.objectA = 1;
          link.objectB = 99; // hors de la page : gardé
          PageObject c{link};
          c.common.id = 2;
          d.page(0).objects = {shape(1), c};
          d.refreshNextId();
          const Page copy = withFreshIds(d, d.page(0));
          QCOMPARE(std::get<Connector>(body(copy.objects[1])).objectA, copy.objects[0].common.id);
          QCOMPARE(std::get<Connector>(body(copy.objects[1])).objectB, ObjectId{99});
      }
      void actionTargetsFollow()
      {
          Document d = pages(1);
          PageObject a = shape(2);
          Action action;
          action.target = 1;
          a.common.actions.push_back(action);
          d.page(0).objects = {shape(1), a};
          d.refreshNextId();
          const Page copy = withFreshIds(d, d.page(0));
          QCOMPARE(copy.objects[1].common.actions[0].target, copy.objects[0].common.id);
      }
      void redoKeepsTheSameIds()
      {
          Document d = pages(1);
          d.page(0).objects = {shape(1)};
          d.refreshNextId();
          UndoStack stack;
          stack.push(d, std::make_unique<InsertPagesCommand>(1, std::vector<Page>{d.page(0)}));
          const ObjectId first = d.page(1).objects[0].common.id;
          stack.undo(d);
          stack.redo(d);
          QCOMPARE(d.page(1).objects[0].common.id, first);
      }
      void cacheFollowsTheShownPage()
      {
          Document d = pages(3);
          UndoStack stack;
          const Changes c = stack.push(d, std::make_unique<MovePagesCommand>(std::vector<std::size_t>{2}, 0));
          Spy spy;
          const auto shown = applyToCache(c, 1, spy); // la page « 2 » affichée passe à l'indice 2
          QVERIFY(shown.has_value());
          QCOMPARE(*shown, std::size_t{2});
      }
  };

  QTEST_GUILESS_MAIN(TstCorePageEdits)
  #include "tst_core_pageedits.moc"
  ```
  (Si `Group` n'est pas constructible ainsi, poser `members` après coup.)
- [ ] **Étape 2 : implémenter.** `withFreshIds` : première passe, table par `document.allocateId()` ; seconde passe,
  identifiant propre, membres, extrémités, cibles. `MovePagesCommand::redo` : pages retirées de la plus haute à la
  plus basse (`takePage`), insérées dans l'ordre à l'indice corrigé (`before` moins le nombre de pages retirées
  avant lui) ; `Changes` : un `PageRemoved` par page à son indice du moment, puis un `PageInserted` par page à son
  indice d'arrivée (ce que `applyToCache` suit) ; `undo` : inverse exact. `RemovePagesCommand` : garde les pages
  retirées et leurs indices ; refus si `pages.size() >= pageCount()`. `InsertPagesCommand` : `withFreshIds` au premier
  redo (`m_fresh`), puis `insertPage` ; `undo` : `takePage` en ordre inverse.
- [ ] **Étape 3 : vert**, mutation, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add core/include/actcraft/core/pageedits.h core/src/pageedits.cpp tests/tst_core_pageedits.cpp
  git -C <wt> commit -m "core : déplacer, retirer et coller des pages, identifiants neufs et références suivies (L4-T15)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t16"></a>
### Tâche L4-T16 — `core` : commande de texte et ligne de base

**Objectif.** Remplir `textedits` : `SetTextCommand` (texte entier remplacé, une étape, `ObjectChanged` de zone
emprise avant ∪ après) et `textBaseline` (ligne de base de la première ligne lue, `runs[0].baseline`).

**Fichiers.** `core/include/actcraft/core/textedits.h`, `core/src/textedits.cpp`, `tests/tst_core_textedits.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_core_textedits` ; fonctions de la table en `PASS` ; preuve.

**Interdits.** Pas de QtGui (aucune mise en page du HTML dans `core/`).

**Terminé quand.** Fonctions vertes ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge.**
  ```cpp
  #include <actcraft/core/document.h>
  #include <actcraft/core/edits.h>
  #include <actcraft/core/textedits.h>
  #include <actcraft/core/undo.h>

  #include <QtTest>

  using namespace actcraft::core;

  class TstCoreTextEdits : public QObject
  {
      Q_OBJECT

      static Document withText()
      {
          Document d;
          PageObject t{Text{{10, 10, 100, 30}, "<p>un</p>", {}}};
          t.common.id = 5;
          d.page(0).objects.push_back(t);
          d.refreshNextId();
          return d;
      }

  private slots:
      void setTextIsOneStep()
      {
          Document d = withText();
          UndoStack stack;
          Text after;
          after.bounds = {10, 10, 200, 30};
          after.html = "<p>deux</p>";
          const Changes c = stack.push(d, std::make_unique<SetTextCommand>(0, 5, after));
          QCOMPARE(c.size(), std::size_t{1});
          QCOMPARE(c[0].kind, Change::Kind::ObjectChanged);
          QCOMPARE(c[0].area.width, 200.0);
          QCOMPARE(QString::fromStdString(std::get<Text>(body(d.page(0).objects[0])).html), QStringLiteral("<p>deux</p>"));
          QCOMPARE(stack.count(), std::size_t{1});
      }
      void undoRestoresHtmlAndBounds()
      {
          Document d = withText();
          UndoStack stack;
          Text after;
          after.html = "<p>deux</p>";
          stack.push(d, std::make_unique<SetTextCommand>(0, 5, after));
          stack.undo(d);
          const auto& t = std::get<Text>(body(d.page(0).objects[0]));
          QCOMPARE(QString::fromStdString(t.html), QStringLiteral("<p>un</p>"));
          QCOMPARE(t.bounds.width, 100.0);
      }
      void notATextIsIgnored()
      {
          Document d = withText();
          PageObject s{Shape{}};
          s.common.id = 6;
          d.page(0).objects.push_back(s);
          UndoStack stack;
          QVERIFY(stack.push(d, std::make_unique<SetTextCommand>(0, 6, Text{})).empty());
          QVERIFY(stack.push(d, std::make_unique<SetTextCommand>(0, 42, Text{})).empty());
      }
      void baselineFromFirstRun()
      {
          Text t;
          TextSpan a;
          a.baseline = {0, 22};
          t.runs = {a};
          QCOMPARE(textBaseline(t)->y, 22.0);
      }
      void noRunsNoBaseline() { QVERIFY(!textBaseline(Text{}).has_value()); }
  };

  QTEST_GUILESS_MAIN(TstCoreTextEdits)
  #include "tst_core_textedits.moc"
  ```
  (Initialisation de `Text` par champs nommés si l'agrégat par position est refusé par la règle Clang.)
- [ ] **Étape 2 : implémenter** (emprise par la fonction d'emprise de `edits`).
- [ ] **Étape 3 : vert**, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add core/include/actcraft/core/textedits.h core/src/textedits.cpp tests/tst_core_textedits.cpp
  git -C <wt> commit -m "core : texte remplacé en une étape, ligne de base (L4-T16)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t17"></a>
### Tâche L4-T17 — `ui` : miniatures des pages

**Écart constaté (amendement 5).** Mergée (ccd826b) ; contrat non réécrit. `Main.qml` crée `appThumbnails` en
permanence : les miniatures sont rendues panneau des navigateurs fermé, banc compris. La suspension est ajoutée par
**L4-T19** (propriété `active`, défaut vrai : les fonctions de cette tâche ne changent pas) et mesurée par **L4-T29**.

**Amendement 4 (V5).** L4-T0 a cherché l'état d'un contact publié : il n'y en a pas. `input::PointerRouter` garde son
état privé (`input/include/actcraft/input/routing.h:76-79`) et `PageView::gestureActive` ne couvre que les gestes
à deux doigts. Le choix (1) de l'amendement 1 tombe : `contactActive` est **ajouté en fin de `pageview.h`** (choix
(2)), avec le commentaire « amendement 1 du lot 4 : ajout en fin, en-tête figé par L0-S7 », signalé au rapport ;
`PageView` le met à jour dans son propre chemin d'entrée (`pageview.cpp`), à l'appui et au relâché de tout contact
routé, sans toucher au routeur.

**Amendement 1 (défaut 11, amendement 7).** `pageview.h` est figé par L0-S7 (chemin d'entrée de la vue). Ordre de
préférence : (1) état d'un contact déjà publié (`input::PointerRouter` ou `PageView`, constat A7 de L4-T0) lu par
`PageThumbnails` sans toucher l'en-tête ; (2) sinon, `contactActive` **ajouté en fin** de `pageview.h`, avec le
commentaire « amendement 1 du lot 4 : ajout en fin, en-tête figé par L0-S7 », signalé au rapport. Lignes : remise à
zéro (`beginResetModel`, toutes les pages marquées) sur **tout** `PageInserted` ou `PageRemoved` reçu par
l'observateur (`DocumentController::addChangeObserver`, D-S7), même si le nombre de pages ne change pas (un
déplacement retire puis insère). Test de plus : `moveRerendersShiftedRows` (trois pages rendues, `MovePagesCommand`
de la page 2 en tête : après `2 × kSettleMs`, `image(0)` est la miniature de l'ancienne page 2 et `renderCount` a
monté de 3). **Amendement 3 (m4)** : `launchCount` (en-tête de L4-T1) monte d'un à chaque rendu lancé, sur le fil
principal, au moment du lancement ; `postponedDuringContact` vérifie en plus que `launchCount` ne bouge pas entre
l'appui et le relâché.

**Objectif.** Remplir `PageThumbnails` et `ThumbnailItem` (en-tête de L4-T1). **Le patron évident est le mauvais** :
appeler `render::thumbnailPng` sur le fil principal à chaque `pageContentChanged` figerait l'écriture (rendu du
peintre complet de la page à chaque trait). Contrat : `pageContentChanged(page)` marque la page ; un minuteur unique
relance `kSettleMs` après la dernière marque ; à son échéance, si un contact est en cours sur la vue
(`PageView::contactActive`, ajouté ici), on attend la fin du contact ; sinon, pour **une** page marquée à la fois :
copie de `core::Page` sur le fil principal (temps mesuré : `lastCopyMs`), rendu par `render::paintPage` sur une
`QImage` de `thumbnailWidth` px de large dans `QThreadPool::globalInstance()`, retour par
`QMetaObject::invokeMethod(…, Qt::QueuedConnection)` gardé par un `QPointer` et un numéro de génération (document
remplacé, page retirée : résultat jeté, `staleRenderIsDropped`). Images décodées : par la même voie que les
miniatures de l'enregistrement (`ui/src/documentio.cpp`, peintre sur fil secondaire : la lire d'abord et réutiliser
sa recherche d'images). Lignes : `beginResetModel` à chaque `PageInserted` ou `PageRemoved` (amendement 1), toutes
marquées. `ThumbnailItem::paint` dessine l'image (ou un fond blanc avant le premier rendu).

**Ajout à `PageView`** (`pageview.h`, `pageview.cpp`) : `Q_PROPERTY(bool contactActive READ contactActive NOTIFY
contactActiveChanged)` — vrai entre l'appui et le relâché de tout contact routé (stylet, souris, doigt), signal émis
aux deux bascules ; si L0-S ou le lot 3 a déjà posé une propriété équivalente (A7), la réutiliser et ne pas toucher
`PageView`.

**Fichiers.** `ui/include/actcraft/ui/pagethumbnails.h`, `ui/src/pagethumbnails.cpp`,
`ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`, `tests/tst_ui_pagethumbnails.cpp`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_pagethumbnails|tst_pageview'` ; fonctions de la table en
`PASS` ; banc sans écran ; preuve de tâche.

**Interdits.** Aucun accès au document hors du fil principal (la copie seule voyage). Aucun appel au cache. Pas de
fournisseur d'images enregistré dans le moteur (décision 8 du plan 4a).

**Terminé quand.** Fonctions vertes ; `tst_pageview` vert ; banc joint ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_ui_pagethumbnails.cpp` (`QTEST_MAIN`, `DocumentController` réel ; pour
  `contactActive`, une vue réelle dans une fenêtre hors écran, contacts injectés comme `tst_ui_drawing`) :
  ```cpp
  void rendersAfterSettling()
  {
      ui::DocumentController controller;
      controller.newDocument(1024, 768);
      ui::PageThumbnails thumbs;
      thumbs.setDocumentController(&controller);
      QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 1, 5000); // première image de la page vierge
      core::Stroke stroke;
      stroke.points = {{100, 100, 0.5f}, {900, 700, 0.5f}};
      stroke.baseWidth = 30;
      controller.commitStroke(stroke);
      QTest::qWait(ui::PageThumbnails::kSettleMs / 2);
      QCOMPARE(thumbs.renderCount(), 1); // pas encore
      QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 2, 5000);
      const QImage image = thumbs.image(0);
      QCOMPARE(image.width(), ui::PageThumbnails::kDefaultWidth);
      QVERIFY(qGray(image.pixel(image.width() / 2, image.height() / 2)) < 128);
  }
  void oneRenderForManyEdits()
  {
      // dix traits à 20 ms d'écart après le premier rendu : un seul rendu de plus
  }
  void postponedDuringContact()
  {
      // appui injecté sur la vue (contactActive vrai), trait commis par le contrôleur, attente de 2 × kSettleMs :
      // renderCount inchangé ; relâché : un rendu
  }
  void renderRunsOffTheMainThread()
  {
      // aide de test : PageThumbnails::lastRenderThread() (membre public « tests ») != QThread::currentThread()
  }
  void rowsFollowPages()
  {
      // nextPage() deux fois : rowCount() == 3, chaque ligne a PageNumberRole 1, 2, 3
  }
  void staleRenderIsDropped()
  {
      // rendu lancé puis newDocument() avant son retour : aucune image de l'ancien document dans image(0)
  }
  ```
  Écrire en entier les corps esquissés par leurs commentaires, sur le modèle de `rendersAfterSettling` ; ajouter à
  l'en-tête, en public, `QThread* lastRenderThread() const noexcept; // tests` (ajout permis).
- [ ] **Étape 2 : `PageView::contactActive`** (ou réutilisation, A7) et son test dans `tests/tst_pageview.cpp`
  n'est pas requis : `postponedDuringContact` le couvre.
- [ ] **Étape 3 : implémenter** le modèle, le minuteur, la file d'une page à la fois, le retour sur le fil principal,
  `ThumbnailItem`.
- [ ] **Étape 4 : vert**, banc sans écran (le panneau n'est pas ouvert par le banc : aucun effet attendu), preuve.
- [ ] **Étape 5 : commit.**
  ```powershell
  git -C <wt> add ui/include/actcraft/ui/pagethumbnails.h ui/src/pagethumbnails.cpp ui/include/actcraft/ui/pageview.h ui/src/pageview.cpp tests/tst_ui_pagethumbnails.cpp
  git -C <wt> commit -m "ui : miniatures des pages rendues hors du fil principal, regroupées, suspendues pendant un contact (L4-T17)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t18"></a>
### Tâche L4-T18 — `ui` : contrôleur des pages

**Écart constaté (amendement 5).** Mergée (0a54fab) ; contrat non réécrit. Ses commandes passent par
`DocumentController::execute()`, qui n'annonce rien : pendant l'édition d'un texte, supprimer, couper, dupliquer ou
déplacer sans changer de page ne valide pas le texte d'abord. Correctif **L4-C2** (même fichier
`ui/src/pagescontroller.cpp`, après le merge de cette tâche et de L4-T21).

**Objectif.** Remplir `PagesController` : sélection (jamais vide ; suit la page affichée quand elle change par une
autre voie), `click` (F-801), `goTo` (`DocumentController::showPage`), `moveSelection` (`MovePagesCommand`, puis
sélection = `positionsAfterMove`, page affichée = la première déplacée, comme la référence), `insertBlank`
(`AddPageCommand` avant / après la page affichée, de sa taille), `copySelection` (copies des pages),
`cutSelection` (copie puis `RemovePagesCommand`), `paste` (`InsertPagesCommand` à l'indice de la page active,
F-601 : avant elle), `duplicateSelection` (`InsertPagesCommand` après la dernière sélectionnée), `deleteSelection`
(`RemovePagesCommand`, refus de la dernière page). Chaque action = une étape, par `DocumentController::execute`.

**Fichiers.** `ui/include/actcraft/ui/pagescontroller.h`, `ui/src/pagescontroller.cpp`, `tests/tst_ui_pages.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_pages` ; fonctions de la table en `PASS` ; preuve de tâche.

**Interdits.** Aucun accès direct aux pages du document hors commandes (lecture permise).

**Terminé quand.** Fonctions vertes ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_ui_pages.cpp` (`DocumentController` réel, pages nommées « 1 » à « 4 »
  par une aide qui pose `page(i).name` à travers une fixture du test) : `clickSelectsAndShows` (click(2, 0) :
  sélection [2], `currentIndex` 2) ; `shiftSelectsRange` (click(1,0), click(3, Qt::ShiftModifier) : [1,2,3]) ;
  `ctrlToggles` (click(0,0), click(2, Ctrl) : [0,2] ; click(0, Ctrl) : [2] ; click(2, Ctrl) : [2]) ;
  `moveKeepsSelection` (sélection [2,3], moveSelection(0) : ordre « 3412 », sélection [0,1]) ;
  `pasteInsertsBeforeActive` (copie de [0], goTo(2), paste() : la copie à l'indice 2, 5 pages) ;
  `cutThenPasteMoves` (cut [0], goTo(2), paste : « 2314 ») ; `duplicateAfterSelection` (sélection [1],
  duplicate : 5 pages, la copie en 2) ; `deleteRefusesLastPage` (document d'une page : `deleteSelection` ne change
  rien) ; `oneUndoPerCommand` (chaque action puis `undo()` rend l'ordre d'avant).
- [ ] **Étape 2 : implémenter.**
- [ ] **Étape 3 : vert**, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add ui/include/actcraft/ui/pagescontroller.h ui/src/pagescontroller.cpp tests/tst_ui_pages.cpp
  git -C <wt> commit -m "ui : pages sélectionnées, réordonnées, coupées, copiées, collées, dupliquées (L4-T18)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t21"></a>
### Tâche L4-T21 — `ui` : édition du texte

**Écart constaté (amendement 5).** Mergée (8a64872) ; contrat non réécrit. Deux points relevés par son rapport :
(1) Échap n'atteint pas `TextTool::cancel()` (`ToolHost::cancelGesture` n'appelle `cancel()` que si l'outil est occupé,
`ui/src/toolhost.cpp:370-378`, et `TextTool` ne l'est jamais) → **L4-T23** valide par `Keys.onShortcutOverride` ;
(2) une commande de pages pendant l'édition ne valide pas le texte → correctif **L4-C2**.

**Amendement 4 (V6, D7, D8, P17).** Prime sur l'amendement 1 et sur le reste de la section.
- **Dépôts forcés (V6).** Enregistrer, annuler, rétablir, ouvrir, supprimer et effacer émettent `editAboutToApply`
  puis déposent ; changer de page émet `pageAboutToChange` puis dépose (`ui/src/documentcontroller.cpp:322-332`) ;
  `replaceDocument` (`:503`) émet `pageAboutToChange` sans dépôt. `TextEditing` valide donc le texte sur **les deux
  signaux**, comme `ToolHost` les écoute (`ui/src/toolhost.cpp:121-122`) ; `documentcontroller.cpp` reste inchangé pour
  ce motif (fichier gardé dans la ligne de la table pour un chemin qui n'émettrait ni l'un ni l'autre, signalé alors).
  `pageChangeDuringEditCommitsFirst` passe par `pageAboutToChange`.
- **Texte vide (D7).** Un texte neuf laissé vide disparaît sans étape ; un texte **existant** vidé est supprimé en
  **une** étape (`dropLifted(RemoveObjectsCommand)`). `emptyTextIsNotKept` couvre les deux cas (code ci-dessous).
- **Barre (D8).** Taille ±2 points, **minimum 8** (et non 4) : test `sizeDownStopsAtEight`. Interligne : `text.spacing-up`
  ajoute 2 points d'espace **au-dessus du paragraphe** (`QTextBlockFormat::topMargin`), `text.spacing-down` en retire 2,
  minimum 0 (et non un pourcentage) : test `spacingAddsTwoPointsAbove`. Retrait ±1 niveau, minimum 0 ; une sorte de
  puces au lot 4. Texte par défaut : police de l'application, 14 points, noir, sans fond ni contour ; texte de départ
  « Votre texte » (D7).
- **Texte verrouillé (P17).** `beginAt` sur un texte verrouillé (`common.locked`) ne l'édite pas : un texte neuf se pose
  au point cliqué, par-dessus. Test `lockedTextIsNotEdited`.
- Citation : garde `loadResource` en `render/src/pagepainter.cpp:73`.

**Amendement 1 (bloquant 5, défauts 6 et 12).**
- **HTML assaini.** Le HTML d'un texte vient du fichier ouvert : `beginAt` ne le donne jamais tel quel au `TextEdit`.
  Fonction libre de `textediting.cpp`, `QString safeTextHtml(const QString& html)` : parse dans un `QTextDocument`
  dérivé local dont `loadResource` rend `QVariant()` (même garde que L2-T11, `render/src/pagepainter.cpp:73`), retire
  tout fragment d'image (`QTextImageFormat`) dont le nom n'est pas une URL `data:`, retire les ancres, puis rend
  `toHtml()`. **Amendement 2 (N5)** : retire aussi `QTextFormat::BackgroundImageUrl` de chaque format de bloc
  (`QTextBlockFormat`), de cadre et de tableau (`QTextFrameFormat`, `QTextTableFormat`) et de cellule
  (`QTextTableCellFormat`), par `QTextCursor::setBlockFormat`, `QTextFrame::setFrameFormat` et
  `QTextTableCell::setFormat`. Seul ce HTML entre dans `textDocument`. Test `editingLoadsNoExternalResource` : texte dont le HTML a
  `<img src="file://hote/partage/x.png">`, `<img src="http://exemple.invalid/y.png">`, un lien, et (amendement 2)
  `<table background="file://hote/z.png"><tr><td background="file://hote/w.png">a</td></tr></table>`, et
  (amendement 3) `<p style="background-image:url(file://hote/v.png)">b</p>` ; après `beginAt`, le HTML du
  `TextEdit` ne contient ni `file:`, ni `exemple.invalid`, ni `hote/` (amendement 3 : pas « ni `http` », le DOCTYPE
  de `toHtml()` cite `http://www.w3.org/…`) ; le compteur de `loadResource` du document d'analyse n'a été appelé
  que pour des URL de l'entrée et a toujours rendu `QVariant()` ; le texte visible est gardé.
- **Dépôt forcé.** Tout chemin de `DocumentController` qui dépose un soulèvement sans geste de l'outil
  (enregistrer, annuler, refaire, changer de page, remplacer le document ; liste relevée par L4-T0, A6) doit émettre
  `editAboutToApply` **avant** le dépôt ; s'il ne le fait pas, cette tâche l'ajoute dans `ui/src/documentcontroller.cpp`
  (signal existant, aucun ajout à l'en-tête). `TextEditing` valide le texte sur ce signal. Tests :
  `saveDuringEditCommitsFirst` (édition en cours, enregistrement : le fichier relu contient le texte frappé),
  `undoDuringEditCommitsFirst` (frappe puis Annuler : la frappe est validée puis annulée, une étape),
  `pageChangeDuringEditCommitsFirst` (frappe puis `showPage(1)` : la page 0 garde le texte, aucun soulèvement).
- **Espion** : `DocumentController::addChangeObserver` puis `core::applyToCache(changes, currentIndex, spy)`.
- **Texte du lot 7** (`textOfLot7IsEditable`) : posé par l'API d'insertion du lot 7 (hors de `documentcontroller.*`,
  D-S8) ; si elle n'a pas fusionné, `execute(AddObjectCommand)` d'un `core::Text` de même forme.
- `beginAt` reçoit le point et le sens depuis le gestionnaire de `text.edit-at` (argument `"x;y;v"`, L4-T2).

**Objectif.** Remplir `TextEditing` (en-tête de L4-T1), décision 37 : édition dans le `TextEdit` QML
(`textDocument` = son `QQuickTextDocument`), dessin final par le peintre partagé. **Le patron évident est le
mauvais** : écrire dans le document à chaque frappe (une commande par touche, un redessin du cache par touche).
Contrat : `beginAt` sur un texte existant (`core::objectAt`, A8, texte seulement) le **soulève**
(`DocumentController::liftObjects`, un redessin) et charge son HTML ; ailleurs, texte neuf (« Votre texte » tout
sélectionné, police et taille de `UserState::textSettings`, D7), rien dans le document ; `commit` : texte vide
(`QTextDocument::toPlainText().trimmed().isEmpty()`) → texte neuf oublié, texte existant supprimé par
`dropLifted(RemoveObjectsCommand)` ; sinon texte neuf → `execute(AddObjectCommand)` (couche intermédiaire, F-253),
texte existant → `dropLifted(SetTextCommand)`. Une étape dans tous les cas. `cancel` : `dropLifted(nullptr)`.
`editAboutToApply` du contrôleur (Annuler, changement de page) valide d'abord. `apply` traite les commandes
`text.*` de la barre (F-100) sur la sélection du `TextEdit` par `QTextCursor` : police, taille (pas de 2 points,
D8), gras, italique, souligné, exposant, indice, couleurs, alignements, interligne, retrait, puces, tout
sélectionner, symbole inséré. Le cadre (`frame`) suit la taille du document (largeur réglable par la poignée
d'extension, hauteur = hauteur du texte). Mémoire : la police et la taille choisies deviennent celles des textes
suivants (`setTextSettings`).

**Fichiers.** `ui/include/actcraft/ui/textediting.h`, `ui/src/textediting.cpp`, `tests/tst_ui_text.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_text` ; fonctions de la table en `PASS` ; banc sans écran ;
preuve de tâche.

**Interdits.** Pas d'API privée de Qt (`QQuickTextDocument::textDocument()` est publique). Aucun appel au cache hors
`DocumentController`.

**Terminé quand.** Fonctions vertes ; banc joint ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_ui_text.cpp` (`QTEST_MAIN` ; un `QQuickTextDocument` obtenu d'un
  `TextEdit` créé par `QQmlComponent` sur `import QtQuick; TextEdit {}` ; espion de cache par `addChangeObserver`
  et `core::applyToCache`, D-S7) :
  ```cpp
  void newTextStartsWithPlaceholderSelected()
  {
      Harness h; // contrôleur, TextEdit, TextEditing branchés
      QVERIFY(h.editing.beginAt({200, 200}, false));
      QVERIFY(h.editing.isActive());
      QCOMPARE(h.editing.plainText(), QStringLiteral("Votre texte"));
      QCOMPARE(h.editing.selectionStart(), 0);
      QCOMPARE(h.editing.selectionEnd(), h.editing.plainText().size());
      QCOMPARE(h.controller.shownObjectCount(), 0);
  }
  void typingThenCommitAddsOneObject()
  {
      Harness h;
      h.editing.beginAt({200, 200}, false);
      h.replaceAllText(QStringLiteral("Bonjour")); // QTextCursor sur le document du TextEdit
      h.editing.commit();
      QCOMPARE(h.controller.shownObjectCount(), 1);
      QVERIFY(!h.editing.isActive());
  }
  void emptyTextIsNotKept()
  {
      Harness h;
      h.editing.beginAt({200, 200}, false);
      h.replaceAllText(QString());
      h.editing.commit();
      QCOMPARE(h.controller.shownObjectCount(), 0);
      QVERIFY(!h.controller.canUndo());
      // Amendement 4 (D7) : un texte existant vidé est supprimé en une étape.
      h.addText(QStringLiteral("un"), {200, 200, 120, 30});
      QVERIFY(h.editing.beginAt({210, 210}, false));
      h.replaceAllText(QString());
      h.editing.commit();
      QCOMPARE(h.controller.shownObjectCount(), 0);
      h.controller.undo();
      QCOMPARE(h.controller.shownObjectCount(), 1);
      QCOMPARE(h.htmlPlainOfFirst(), QStringLiteral("un"));
  }
  void editingExistingLiftsIt()
  {
      Harness h;
      h.addText(QStringLiteral("un"), {200, 200, 120, 30});
      h.cacheSpy.reset();
      QVERIFY(h.editing.beginAt({210, 210}, false));
      QCOMPARE(h.cacheSpy.removes, 1);
      for (int i = 0; i < 20; ++i)
          h.typeChar(QLatin1Char('x'));
      QCOMPARE(h.cacheSpy.total(), 1); // rien pendant la frappe
      h.editing.commit();
      QCOMPARE(h.cacheSpy.inserts, 1);
  }
  void commitMakesASingleStep()
  {
      Harness h;
      h.addText(QStringLiteral("un"), {200, 200, 120, 30});
      h.editing.beginAt({210, 210}, false);
      h.replaceAllText(QStringLiteral("deux"));
      h.editing.commit();
      h.controller.undo();
      QCOMPARE(h.htmlPlainOfFirst(), QStringLiteral("un"));
  }
  void boldAppliesToSelection()
  {
      Harness h;
      h.editing.beginAt({200, 200}, false); // tout sélectionné
      h.editing.apply(QStringLiteral("text.bold"), {});
      QVERIFY(h.editing.bold());
  }
  void sizeUpStepsByTwo()
  {
      Harness h;
      h.editing.beginAt({200, 200}, false);
      const qreal before = h.editing.fontSize();
      h.editing.apply(QStringLiteral("text.size-up"), {});
      QCOMPARE(h.editing.fontSize(), before + 2);
  }
  void sizeDownStopsAtEight() // amendement 4 (D8)
  {
      Harness h;
      h.editing.beginAt({200, 200}, false); // 14 points (D7)
      for (int i = 0; i < 5; ++i)
          h.editing.apply(QStringLiteral("text.size-down"), {});
      QCOMPARE(h.editing.fontSize(), 8.0);
  }
  void spacingAddsTwoPointsAbove() // amendement 4 (D8)
  {
      Harness h;
      h.editing.beginAt({200, 200}, false);
      const qreal before = h.firstBlockTopMargin(); // QTextBlockFormat::topMargin du premier paragraphe
      h.editing.apply(QStringLiteral("text.spacing-up"), {});
      QCOMPARE(h.firstBlockTopMargin(), before + 2);
      for (int i = 0; i < 3; ++i)
          h.editing.apply(QStringLiteral("text.spacing-down"), {});
      QCOMPARE(h.firstBlockTopMargin(), 0.0);
  }
  void lockedTextIsNotEdited() // amendement 4 (P17)
  {
      Harness h;
      h.addText(QStringLiteral("un"), {200, 200, 120, 30}, /*locked=*/true);
      QVERIFY(h.editing.beginAt({210, 210}, false));
      QCOMPARE(h.editing.plainText(), QStringLiteral("Votre texte")); // texte neuf, par-dessus
      QVERIFY(!h.controller.isLifting());
  }
  void textOfLot7IsEditable()
  {
      Harness h;
      h.addText(QStringLiteral("12"), {300, 300, 80, 40}); // même objet que l'insertion de texte du lot 7 (hors documentcontroller.*, D-S8)
      QVERIFY(h.editing.beginAt({310, 310}, false));
      QCOMPARE(h.editing.plainText(), QStringLiteral("12"));
  }
  void cancelRestoresLifted()
  {
      Harness h;
      h.addText(QStringLiteral("un"), {200, 200, 120, 30});
      h.editing.beginAt({210, 210}, false);
      h.editing.cancel();
      QCOMPARE(h.controller.shownObjectCount(), 1);
      QVERIFY(!h.controller.isLifting());
  }
  ```
  Le `Harness` (fenêtre hors écran, contrôleur, `TextEdit`, espion de cache avec `reset`, `total`, `addText` par
  `execute(AddObjectCommand)` avec un drapeau `locked` facultatif, `replaceAllText`, `typeChar` par
  `QTextCursor::insertText`, `htmlPlainOfFirst` par `QTextDocument::setHtml` puis `toPlainText`,
  `firstBlockTopMargin` par le premier bloc du document du `TextEdit`) est écrit en entier dans le fichier.
- [ ] **Étape 2 : implémenter.**
- [ ] **Étape 3 : vert**, banc sans écran, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add ui/include/actcraft/ui/textediting.h ui/src/textediting.cpp tests/tst_ui_text.cpp
  git -C <wt> add ui/src/documentcontroller.cpp   # seulement si modifié (dépôt forcé, amendement 2)
  git -C <wt> commit -m "ui : édition du texte soulevé, une étape à la validation, barre de mise en forme (L4-T21)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t22"></a>
### Tâche L4-T22 — `tools` : outils Texte et Texte de haut en bas

**Objectif.** Remplir `TextTool` : à l'appui (`Phase::Press`), `commands.trigger("text.edit-at",
QStringLiteral("%1;%2;%3").arg(x, 0, 'f', 2).arg(y, 0, 'f', 2).arg(m_vertical ? 1 : 0))` (amendement 1 : argument
texte, `ArgumentKind::Text` n'accepte qu'une chaîne non vide) ; `deactivate` : `commands.trigger("text.edit-at",
QVariant())` (valide, F-252 ; accepté parce que l'argument est `"optional": true`) ; jamais occupé (l'édition vit dans l'interface) ; `id()` : « text » ou « text-vertical ».

**Fichiers.** `tools/include/actcraft/tools/texttool.h`, `tools/src/texttool.cpp`, `tests/tst_tools_text.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_tools_text` ; fonctions `pressAsksToEditAt`,
`verticalToolSaysVertical`, `deactivateCommits`, `neverBusy` en `PASS` ; preuve de tâche.

**Interdits.** Aucun en-tête de `ui/` ni de `render/`.

**Terminé quand.** Fonctions vertes ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge** (faux du contexte comme `tst_tools_shape.cpp`, `FakeCommands` qui garde
  `(id, argument)`) :
  ```cpp
  void pressAsksToEditAt()
  {
      Harness h;
      tools::TextTool tool(h.context, false);
      tool.onSample(at(120, 80, input::Phase::Press));
      QCOMPARE(h.commands.last.first, QStringLiteral("text.edit-at"));
      QCOMPARE(h.commands.last.second.toString(), QStringLiteral("120.00;80.00;0")); // amendement 1
      QCOMPARE(tool.id(), QStringLiteral("text"));
  }
  void verticalToolSaysVertical()
  {
      Harness h;
      tools::TextTool tool(h.context, true);
      tool.onSample(at(10, 10, input::Phase::Press));
      QVERIFY(h.commands.last.second.toString().endsWith(QStringLiteral(";1")));
      QCOMPARE(tool.id(), QStringLiteral("text-vertical"));
  }
  void deactivateCommits()
  {
      Harness h;
      tools::TextTool tool(h.context, false);
      tool.deactivate();
      QCOMPARE(h.commands.last.first, QStringLiteral("text.edit-at"));
      QVERIFY(!h.commands.last.second.isValid());
  }
  void neverBusy()
  {
      Harness h;
      tools::TextTool tool(h.context, false);
      tool.onSample(at(10, 10, input::Phase::Press));
      QVERIFY(!tool.isBusy());
  }
  ```
- [ ] **Étape 2 : implémenter.** **Étape 3 : vert**, preuve. **Étape 4 : commit.**
  ```powershell
  git -C <wt> add tools/include/actcraft/tools/texttool.h tools/src/texttool.cpp tests/tst_tools_text.cpp
  git -C <wt> commit -m "tools : outils Texte et Texte de haut en bas (L4-T22)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t19"></a>
### Tâche L4-T19 — QML : panneau des navigateurs et navigateur de pages ; scénario `navigateur-pages`

**Amendement 5 (trouvé par L4-T17).** Le rendu des miniatures est **suspendu quand le panneau des navigateurs est
caché**. `Main.qml` crée `appThumbnails` en permanence ; sans suspension, chaque modification lance un rendu,
panneau fermé, banc compris.
- **En-tête** `ui/include/actcraft/ui/pagethumbnails.h`, ajout en fin, signalé (en-tête de L4-T1, rempli par L4-T17) :
  en fin de la liste des propriétés,
  ```cpp
      // Amendement 5 du lot 4 (L4-T19) : faux quand le panneau des navigateurs est caché ; aucun rendu n'est alors
      // lancé. Revenu à vrai : les pages modifiées entre-temps, ou jamais rendues, le sont kSettleMs plus tard,
      // comme après une modification (et jamais pendant un contact). Vrai par défaut.
      Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
  ```
  en fin des membres publics `bool active() const noexcept;` et `void setActive(bool active);`, en fin des signaux
  `void activeChanged();`, en fin des membres privés `bool m_active = true;`.
- **`pagethumbnails.cpp`** : le lancement d'un rendu (là où `launchCount` monte, L4-T17) ne se fait que si
  `m_active` ; une page à rendre pendant la suspension reste marquée ; `setActive(true)` relance le regroupement de
  `kSettleMs` pour les pages marquées. `setActive(false)` n'interrompt pas un rendu déjà lancé.
- **`Main.qml`**, dans `PageThumbnails { id: appThumbnails … }` : `active: browserPanel.visible` (une ligne).
- **Test** dans `tests/tst_ui_pagethumbnails.cpp`, sur le montage de `oneRenderForManyEdits` (L4-T17) :
  ```cpp
      // Amendement 5 (trouvé par L4-T17) : panneau des navigateurs caché, aucun rendu lancé ; rouvert, la page
      // modifiée pendant la suspension est rendue.
      void inactiveRendersNothing()
      {
          ui::DocumentController controller;
          controller.newDocument(1024, 768);
          PageThumbnails thumbs;
          thumbs.setActive(false);
          thumbs.setDocumentController(&controller);
          controller.commitStroke(smallStroke(0));
          QTest::qWait(2 * PageThumbnails::kSettleMs);
          QCOMPARE(thumbs.launchCount(), 0);
          QCOMPARE(thumbs.renderCount(), 0);
          thumbs.setActive(true);
          QTRY_COMPARE_WITH_TIMEOUT(thumbs.renderCount(), 1, 5000);
          QCOMPARE(thumbs.launchCount(), 1);
      }
  ```
  Les fonctions de L4-T17 telles qu'après L4-C10 restent telles quelles (défaut vrai) : dix, dont
  `moveRendersOnlyArrivedRow` (ex-`moveRerendersShiftedRows`), `liftedObjectStaysInThumbnail`,
  `insertRendersOnlyNewPage` et `shownPageRendersFirst`. Commande ciblée
  `-R 'tst_ui_pagethumbnails|tst_captures_navigateur_pages|tst_ui_theme'` ; `inactiveRendersNothing` rouge d'abord.
- **Scénario** `navigateur-pages` : `expect appThumbnails.active false` juste après `waitReady` (avant `view.browsers`),
  `expect appThumbnails.active true` juste après `expect browserPanel.visible true`.
- **Commit** : ajouter `ui/include/actcraft/ui/pagethumbnails.h ui/src/pagethumbnails.cpp tests/tst_ui_pagethumbnails.cpp
  ui/qml/Main.qml` au `git add` de l'étape 4 ; l'ajout à l'en-tête est signalé au rapport.

**Amendement 1.** Les 5 icônes `nav-*` sont ajoutées en fin de `kIcons` (`tests/tst_ui_theme.cpp:17`) ;
`iconsAreOurSvgOnA24Grid` les teste.

**Objectif.** Remplir `BrowserPanel.qml` (F-040 à F-042 : rangée d'icônes — pages active, ressources, objets, notes,
propriétés, actions grisées jusqu'au lot 6, pas de vote ; côté `left` / `right` ; visible par `view.browsers` ;
bord glissé pour la largeur, de 160 à 480 px de thème ; menu propre au navigateur par une icône de menu) et
`PageNavigator.qml` (F-800 à F-803 : colonne de `ThumbnailImage` sur `appThumbnails`, page active encadrée avec son
icône de menu, réglette de taille liée à `appThumbnails.thumbnailWidth`, clic → `appPages.click(index,
modifiers)`, glisser une sélection avec un fantôme d'insertion → `appPages.moveSelection(before)`, clic droit →
`pageContextMenu.popupAt`, touches Ctrl+X/C/V/D et Suppr traduites en commandes de page quand le navigateur a le
focus ; délégués nommés `pageThumb<n>`, n à partir de 1). Cinq icônes de navigateur dessinées par nous (24 × 24).
Le panneau est enregistré comme couverture d'entrée de la vue (`pageView.addInputCover`, A7) s'il la recouvre.

**Fichiers.** Ceux de la ligne L4-T19.

**Test et commande bornée.** Commande ciblée `-R tst_captures_navigateur_pages` ; preuve visuelle ; banc sans
écran ; preuve de tâche.

**Preuve visuelle.** `captures.ps1 -Scenario navigateur-pages -Rendu logiciel -HorsEcran -Sortie .tmp/captures
-Lire "browserPanel.visible=true" -Lire "appDocument.pageCount=5"` : `LU … OK`, `LU appPages.selection=[0,1] OK`,
`CAPTURES OK` ; décrire `panneau.png`, `plage.png`, `deplace.png`, `colle.png`.

**Scénario** `bench/scenarios/navigateur-pages.json` : `waitReady` ; `view.browsers` ; `expect browserPanel.visible
true` ; trois `page.next` (4 pages) ; `waitReady` ; `capture panneau` ; `pointer` sur `pageThumb2` puis sur
`pageThumb3` avec Maj (A11 : cible par nom d'objet ; si le joueur de L0-S ne vise que des coordonnées, la cible par
nom d'objet s'ajoute à l'étape `pointer` dans `bench/src/uiscenario.cpp` et `tests/tst_bench_uiscenario.cpp`, fichiers
alors ajoutés à la tâche par l'amendement de L4-T0) ; `expect appPages.selection [1,2]` ; `capture plage` ; glisser
`pageThumb2` au-dessus de `pageThumb1` (`pointer` appui, mouvements, relâché) ; `expect appPages.selection [0,1]` ;
`capture deplace` ; `page.copy` ; `page.goto 4` ; `page.paste` ; `waitReady` ; `expect appDocument.pageCount 6` ;
`edit.undo` ; `expect appDocument.pageCount 4` ; `page.insert-after` ; `expect appDocument.pageCount 5` ;
`capture colle` ; `page.goto 1` ; `pointer` sur `pageThumb2` avec Ctrl ; `expect appPages.selection [0,1]`.

**Interdits.** Aucune icône de l'original. Le navigateur ne modifie le document que par `appPages`.

**Terminé quand.** Scénario vert, preuve visuelle conforme, banc joint, `TESTS OK`, `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : scénario rouge** (le JSON ci-dessus, écrit en entier). **Étape 2 : QML et icônes** (ajoutées à
  `ui/CMakeLists.txt`). **Étape 3 : vert**, preuve visuelle, banc sans écran, preuve de tâche.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add ui/qml/BrowserPanel.qml ui/qml/PageNavigator.qml ui/icons/nav-ressources.svg ui/icons/nav-objets.svg ui/icons/nav-notes.svg ui/icons/nav-proprietes.svg ui/icons/nav-actions.svg ui/CMakeLists.txt tests/tst_ui_theme.cpp bench/scenarios/navigateur-pages.json
  git -C <wt> commit -m "ui : panneau des navigateurs et navigateur de pages ; scénario navigateur-pages (L4-T19)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t24"></a>
### Tâche L4-T24 — Texte de haut en bas I : rendu et lecture `.flipchart`

**Amendement 5.** Contrat inchangé ; en cours (lancée sur e9e6e42). L4-C4 la suit sur `tests/tst_formats_lot4.cpp`
(V9 bis) : rien à faire ici.

**Amendement 4 (D9).** Rendu : inchangé (lettres empilées en colonne, supposé : écriture verticale asiatique).
Lecture `.flipchart` : dans l'original, l'objet garde **une image rendue et une source opaque** ; le texte n'y est pas
en clair. Il ne devient donc pas un `core::Text` à `vertical = true` : `flipchartVerticalTextRead` vérifie qu'un objet
de ce genre avec son image devient une image (`core::ImageObject`) posée sur son cadre (position et taille lues), avec
un avertissement de lecture « texte de haut en bas lu comme une image » ; sans image, l'objet n'est pas lu et un
avertissement le déclare. La source opaque n'est pas gardée. Les identifiants du genre et de ses champs sont écrits à
la main dans `flipchartids.h` d'après notre documentation du format (ancien dépôt, `docs/re/format-flipchart.md`) ;
s'ils n'y sont pas, le repli « objet non lu déclaré » s'applique et le rapport le dit. Couleurs et orientation du texte
de haut en bas : hors du lot (H4-13).

**Objectif.** H4-3 (D9) : un texte `vertical` se met en page dans une colonne d'un caractère (largeur = plus grande
avance des caractères du texte à sa police), retour à la ligne partout (`QTextOption::WrapAnywhere`), centré :
chaque lettre sous la précédente, mise en forme gardée. Même dessin dans le peintre partagé
(`render/src/pagepainter.cpp`) et dans la texture du rendu (`render/src/paintedrenderer.cpp`), qui l'appelle.
Lecture des `.flipchart` : le genre « texte de haut en bas », aujourd'hui sauté (`formats/src/flipchart.cpp:258`),
devient une image posée sur son cadre, ou un objet non lu déclaré (amendement 4, D9), d'après des identifiants écrits
à la main dans `flipchartids.h` (aucune copie de `_schema.py`).

**Fichiers.** Ceux de la ligne L4-T24.

**Test et commande bornée.** Commande ciblée
`-R 'tst_render_verticaltext|tst_formats_lot4|tst_formats_flipchart|tst_formats_oracle_synth'` ; fonctions de la
table en `PASS` ; banc sans écran ; preuve de tâche.

**Interdits.** Pas de QtGui dans `formats/`. Pas d'oracle modifié pour le faire passer.

**Terminé quand.** Fonctions vertes ; oracle synthétique vert ; banc joint ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** `tests/tst_render_verticaltext.cpp` (`QTEST_MAIN`) : `lettersStackOneUnderAnother`
  (texte « ABC », police de 40 points, `vertical` vrai, peint par `render::paintPage` sur une image : trois bandes
  horizontales disjointes contiennent chacune de l'encre, la colonne d'encre fait moins de 1,5 fois la largeur d'un
  caractère) ; `boldIsKept` (HTML `<b>ABC</b>` : plus de pixels d'encre qu'en maigre) ; `sameInPainterAndTexture`
  (image du peintre et image de la texture du rendu logiciel identiques à 2 niveaux près). Dans
  `tests/tst_formats_lot4.cpp` : `flipchartVerticalTextRead` (paperboard synthétique écrit par l'écrivain de test du
  lot 2 avec un texte de haut en bas : avec son image, objet `ImageObject` au cadre et un avertissement ; sans image,
  aucun objet et un avertissement — amendement 4, D9).
- [ ] **Étape 2 : implémenter** (mise en page commune dans une fonction du peintre ; lecteur).
- [ ] **Étape 3 : vert**, banc sans écran, preuve. **Étape 4 : commit.**
  ```powershell
  git -C <wt> add render/src/pagepainter.cpp render/src/paintedrenderer.cpp tests/tst_render_verticaltext.cpp formats/src/flipchart.cpp formats/src/flipchartids.h tests/tst_formats_lot4.cpp
  git -C <wt> commit -m "render, formats : texte de haut en bas dessiné en colonne et lu des paperboards (L4-T24)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t26"></a>
### Tâche L4-T26 — Page allongée : poignée, commande ; scénario S15 (partie 1)

**Amendement 5 (D10, L4-O1).** Le pas est **cumulé** : × 1,1 sur la hauteur courante à chaque clic, déjà appliqué par
L4-T4 (`nextExtendedHeight`, test `extendStepsByTenPercentOfCurrent`). Après trois clics depuis 768 : **1022,208**
(768 × 1,1³), et non 998,4. Les attendus ci-dessous sont corrigés en conséquence ; trois Annuler rendent 768.

**Amendement 1 (mineur S15).** « La poignée suit » se lit : `PageExtensionHandle.qml` expose
`readonly property bool atPageBottom` (vrai si son centre est à moins d'un pixel du bas de la page en coordonnées de
scène, par `pageView.pageToScene`) ; le scénario `page-allongee-zones` ajoute `expect pageExtensionHandle.atPageBottom
true` après chaque allongement et après un zoom.

**Objectif.** F-301 a, D10, E12 : `PageOptions::extendPage` exécute `core::ExtendPageCommand` sur la page
affichée ; `pageHeight` suit. `PageExtensionHandle.qml` : poignée à nous (barre arrondie et flèche), centrée au bas
de la page, visible par `view.page-extension`, qui suit le bas de la page (zoom, défilement) ; un clic déclenche
`appCommands.trigger("page.extend")` **sans changer d'outil** (la poignée est une couverture d'entrée de la vue,
A7). La vue suit la nouvelle hauteur (zone de défilement, page fixe, taille de la texture du cache) : vérifier que
`pagesChanged` (ou `pageSize`) est émis après un `PageChanged` qui change la taille ; sinon l'ajouter dans
`DocumentController::settle` et faire relire la taille à `PageView`.

**Fichiers.** Ceux de la ligne L4-T26 (`documentcontroller.cpp` et `pageview.cpp` seulement si la vue ne suit pas).

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_pagesettings|tst_captures_page_allongee_zones'` ;
fonctions `extendPageThreeTimes`, `extendKeepsTheCurrentTool`, `viewFollowsTheNewHeight` en `PASS` ; preuve
visuelle ; banc sans écran ; preuve.

**Preuve visuelle.** `captures.ps1 -Scenario page-allongee-zones -Rendu logiciel -HorsEcran -Sortie .tmp/captures
-Lire "appPageSettings.pageHeight=768"` : `LU appPageSettings.pageHeight=1022.208 OK` après trois clics (amendement 5),
`LU appTools.currentTool=pen OK`, `LU appPageSettings.pageHeight=768 OK` après trois Annuler, `CAPTURES OK` ; décrire
`allongee.png` (poignée au bas de la page allongée).

**Scénario** (partie 1) : `waitReady` ; `view.page-extension` ; `expect pageExtensionHandle.visible true` ; trois
`pointer` (clic) sur `pageExtensionHandle` avec `waitReady` ; `expect appPageSettings.pageHeight 1022.208` (tolérance
0,01 ; pas cumulé, D10, amendement 5) ; `expect appTools.currentTool "pen"` ; `capture allongee` ; trois `edit.undo` ;
`expect appPageSettings.pageHeight 768`.

**Interdits.** La poignée ne passe pas par l'outil courant.

**Terminé quand.** Fonctions vertes, scénario vert, banc joint, `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : tests rouges** dans `tests/tst_ui_pagesettings.cpp` : `extendPageThreeTimes` (trois `extendPage`,
  `pageHeight` ≈ 1022,208 à 1e-3 près, amendement 5 ; trois `undo` → 768) ; `extendKeepsTheCurrentTool` (`ToolHost` sur « pen », commande
  `page.extend` déclenchée par `Commands` : outil inchangé) ; `viewFollowsTheNewHeight` (vue réelle :
  `scrollSizeY` change après `extendPage`). Scénario rouge.
- [ ] **Étape 2 : implémenter** (contrôleur, poignée, suivi de la vue si nécessaire).
- [ ] **Étape 3 : vert**, preuve visuelle, banc sans écran, preuve. **Étape 4 : commit.**
  ```powershell
  git -C <wt> add ui/qml/PageExtensionHandle.qml ui/src/pageoptions.cpp tests/tst_ui_pagesettings.cpp bench/scenarios/page-allongee-zones.json
  git -C <wt> add ui/src/documentcontroller.cpp ui/src/pageview.cpp   # seulement si modifiés
  git -C <wt> commit -m "ui : page allongée par sa poignée, une étape par clic (L4-T26)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t20"></a>
### Tâche L4-T20 — Menu de page ; boîte « Fond de la page »

**Amendement 4 (D15, D14).** La boîte du fond a **trois boutons** — annuler, appliquer, valider —, pas de bouton
unique : « appliquer » exécute `PageOptions::setBackground` (une étape) et laisse la boîte ouverte ; « valider »
applique ce qui ne l'est pas encore, puis ferme ; « annuler » ferme sans rien changer de plus (ce qui a été appliqué
reste, annulable par Annuler). Boutons nommés `backgroundCancel`, `backgroundApply`, `backgroundOk`. Au lot 4 :
couleur, deux couleurs en dégradé, image (H4-18) ; capture et superposition du bureau : lot 7 ; ressources : lot 6 ;
case « toutes les pages » et dégradé radial : hors du lot (H4-16). Le fond vaut pour la page active. Test de plus
`eachApplyIsOneStep` (deux `setBackground` de suite, jaune puis vert : deux `undo()` rendent le blanc, un seul rend
le jaune). Scénario : jaune choisi, `pointer` sur `backgroundApply`, `expect backgroundDialog.visible true`,
`checkPixel fond-jaune`, `pointer` sur `backgroundCancel`, `expect backgroundDialog.visible false`, `checkPixel
fond-jaune` (le fond appliqué reste), `capture fond`. D14 : l'ordre du menu de page posé par L4-T1 est conforme.

**Objectif.** F-771, D14 : le menu de page (`PageContextMenu.qml`, entrées de `commands.contextMenu`, amendement 1) ouvert sur une
zone vide de la page agit sur la page affichée (`appPages.goTo` d'abord, sélection = elle) ; ouvert depuis le
navigateur, sur les pages sélectionnées ; les bascules `grid.hide` et `grid.snap` montrent l'état de la page
(`appPageSettings`). F-610, E2, D15 : `BackgroundDialog.qml` (couleur unie, dégradé à deux couleurs et sens, image
si `DocumentController` offre l'ajout d'une ressource — l'API d'insertion du lot 7, hors de `documentcontroller.*` (D-S8, A6) —
sinon couleur et dégradé seulement, H4-18) ; validation selon E2 (D15, amendement 4 : trois boutons) ; `PageOptions::setBackground` exécute
`core::SetBackgroundCommand` (une étape). Le scénario `navigateur-pages` gagne la fin : menu de page ▸ Fond, jaune,
validé, `PIXEL fond-jaune`.

**Fichiers.** Ceux de la ligne L4-T20.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_pagesettings|tst_captures_navigateur_pages'` ; fonctions
`backgroundColorIsOneStep`, `backgroundGradient`, `backgroundRefreshesOnce`, `eachApplyIsOneStep` en `PASS` ; preuve
visuelle ; preuve.

**Preuve visuelle.** `captures.ps1 -Scenario navigateur-pages … -Lire "backgroundDialog.visible=false"` : `LU
backgroundDialog.visible=true OK` après « appliquer », `PIXEL fond-jaune OK`, `LU backgroundDialog.visible=false OK`
après « annuler » (amendement 4), `PIXEL fond-jaune OK` de nouveau, `CAPTURES OK` ; décrire `fond.png` (fond jaune,
boîte fermée).

**Interdits.** Capture de bureau et superposition de bureau absentes (lot 7).

**Terminé quand.** Fonctions vertes, scénario vert, `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : tests rouges** : `backgroundColorIsOneStep` (`setBackground({kind: "color", color: "#ffff00"})` :
  fond jaune, un `undo` le rend blanc) ; `backgroundGradient` (`{kind: "gradient", color, color2, gradient:
  "vertical"}` → `FillKind` dégradé) ; `backgroundRefreshesOnce` (espion de cache : un `invalidate` de toute la
  page). Étapes du scénario ajoutées : `pointer` clic droit sur une zone vide (A11), `expect pageContextMenu.visible
  true`, entrée `page.background` (commande directe si le clic dans le menu n'est pas jouable), choix du jaune par
  `pointer`, validation, `waitReady`, `checkPixel fond-jaune [50, 50] #FFFF00 8`, `capture fond`.
- [ ] **Étape 2 : implémenter.** **Étape 3 : vert**, preuve visuelle, preuve. **Étape 4 : commit.**
  ```powershell
  git -C <wt> add ui/qml/PageContextMenu.qml ui/qml/BackgroundDialog.qml ui/src/pageoptions.cpp tests/tst_ui_pagesettings.cpp bench/scenarios/navigateur-pages.json
  git -C <wt> commit -m "ui : menu de page sur les pages choisies, boîte du fond de la page (L4-T20)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t23"></a>
### Tâche L4-T23 — QML : éditeur de texte et barre de mise en forme ; scénario `texte`

**Amendement 5 (trouvé par L4-T21).** Échap n'atteint pas `TextTool::cancel()` : son raccourci est celui de
`tool.select`, dont le gestionnaire appelle `ToolHost::cancelGesture`, qui n'appelle `cancel()` que si l'outil est
occupé (`ui/src/toolhost.cpp:370-378`) ; `TextTool` ne l'est jamais (`neverBusy`, L4-T22). Le contrat de l'amendement 4
est donc **le seul chemin** : `Keys.onShortcutOverride` du `TextEdit`, pendant l'édition, accepte Échap
(`event.accepted = true`) et `Keys.onPressed` appelle `appText.commit()` ; aucune autre voie (ni `appText.cancel()`, ni
le raccourci de `tool.select`). **Test « Échap valide le texte »** : les étapes Échap de l'amendement 4 sont
remplacées par ce segment, placé **à la fin** du scénario `texte` (après `expect appDocument.shownObjectCount 0`) et
repéré par sa capture finale `echap-valide` : `tool.text` ; appui en (300, 450) ; `expect appText.active
true` ; `typeText "Salut"` ; `{"key": "Escape"}` ; `waitReady` ; `expect appText.active false` ; `expect
appDocument.shownObjectCount 1` (le texte neuf est gardé : une annulation l'aurait perdu) ; `edit.undo` ; `expect
appDocument.shownObjectCount 0` (une seule étape) ; `capture echap-valide`. Preuve visuelle : `LU appText.active=false OK` puis
`LU appDocument.shownObjectCount=1 OK` après Échap.

**Amendement 4 (D7, D8).** D8 confirme la recommandation : pendant l'édition, `Keys.onShortcutOverride` du
`TextEdit` accepte Ctrl+B, Ctrl+I, Ctrl+U (gras, italique, souligné, traduits en `appText.apply`) ; hors édition, les
raccourcis de commande restent. **Échap valide** le texte (D7 : validation au clic ailleurs, au changement d'outil, de
page, et à Échap) : `Keys.onShortcutOverride` accepte aussi Échap pendant l'édition et appelle `appText.commit()`, et
non `appText.cancel()` comme le disait l'objectif. Barre en haut, contenu et ordre de D8 : police, taille, plus grand,
plus petit, gras, italique, souligné, exposant, indice, couleur, fond, trois alignements, interligne plus et moins,
retrait plus et moins, puces, tout sélectionner, symboles (le clavier à l'écran n'a pas de commande au lot 4). Étapes
du scénario `texte` en plus, après la mise en gras : `typeText " !"` puis `{"key": "Escape"}`, `expect appText.active
false`, `expect appDocument.shownObjectCount 1` (le texte frappé est validé, pas abandonné).

**Amendement 1 (bloquant 5, défaut 15).** Le `TextEdit` ne reçoit de HTML que par `appText` (HTML assaini de
L4-T21) : jamais de `text:` lié à un HTML de document, jamais `Text.RichText` sur un HTML non assaini. Raccourcis
pendant la frappe selon **D8** (L4-T0) ; recommandé : `Keys.onShortcutOverride` du `TextEdit` accepte Ctrl+B, Ctrl+I
et Ctrl+U, `Keys.onPressed` les traduit en `appText.apply("text.bold" | "text.italic" | "text.underline")`. Étapes
du scénario `texte` en plus, pendant l'édition : `{"key": "Ctrl+B"}` puis `expect browserPanel.visible false` et
`expect appText.bold true` ; `{"key": "Ctrl+I"}` puis `expect appDocument.pageCount 1`. Si D8 tranche autrement, les
attendus suivent D8 (amendement de L4-T0).

**Objectif.** `TextEditor.qml` : `TextEdit` placé sur `appText.frame` (par `pageView.pageToScene`, échelle du zoom),
`textDocument` donné à `appText`, sélection liée, cadre pointillé à nous, poignée de déplacement et poignée
d'extension de largeur (F-250), focus pris sur `appText.focusRequested`, clic hors de la zone → `appText.commit()`
(F-252), Échap → `appText.commit()` (amendement 4, D7) ; couverture d'entrée de la vue (A7) pendant l'édition. `TextFormatBar.qml`
(F-100, D8) : visible quand `appText.active` et non `vertical`, groupes dans l'ordre de D8, liste des polices
(`Qt.fontFamilies()`), taille, boutons des commandes `text.*` (icônes de L4-T1), couleurs par le nuancier existant,
liste de symboles à nous (Ω, π, ±, ×, ÷, ≤, ≥, ≠, √, ∞, °, €) ; chaque bouton déclenche sa commande par
`appCommands`. Scénario `texte` (S7.3).

**Fichiers.** `ui/qml/TextEditor.qml`, `ui/qml/TextFormatBar.qml`, `bench/scenarios/texte.json`.

**Test et commande bornée.** Commande ciblée `-R tst_captures_texte` ; preuve visuelle ; banc sans écran ; preuve.

**Preuve visuelle.** `captures.ps1 -Scenario texte … -Lire "appDocument.shownObjectCount=1"` : `LU appText.active=true
OK`, `TEXTE Bonjour OK`, `LU appText.bold=true OK`, `LU appDocument.shownObjectCount=1 OK` après validation,
`PIXEL texte-encre OK`, `CAPTURES OK` ; décrire `edition.png`, `barre.png`, `valide.png`.

**Scénario** `bench/scenarios/texte.json` : `waitReady` ; `tool.text` ; `stroke [[300, 300], [300, 300]]` (appui)
; `expect appText.active true` ; `typeText "Bonjour"` (L4-T14) ; `text.select-all` ; `text.bold` ; `expect
appText.bold true` ; `capture edition` ; `capture barre` ; `tool.pen` (valide) ; `waitReady` ; `expect
appDocument.shownObjectCount 1` ; `checkPixel texte-encre` sur la première lettre ; `capture valide` ; `tool.text` ;
appui ailleurs ; `text.select-all` ; `key Delete` ; `tool.pen` ; `expect appDocument.shownObjectCount 1` (texte vide
oublié) ; `edit.undo` ; `expect appDocument.shownObjectCount 0`.

**Interdits.** Aucun texte de l'aide ; libellés et infobulles à nous.

**Terminé quand.** Scénario vert, preuve visuelle conforme, banc joint, `TESTS OK`, `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : scénario rouge.** **Étape 2 : QML.** **Étape 3 : vert**, preuve visuelle, banc, preuve.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add ui/qml/TextEditor.qml ui/qml/TextFormatBar.qml bench/scenarios/texte.json
  git -C <wt> commit -m "ui : éditeur de texte et barre de mise en forme ; scénario texte (L4-T23)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t27"></a>
### Tâche L4-T27 — Zones de page I : modèle, commande, natif

**Amendement 5 (L4-O2, trouvé par L4-T6).** Prime sur l'amendement 4 là où ils diffèrent.
- **Bornes** : 0 à 40 cadres en largeur, 0 à 53 en hauteur (inchangé) ; taille d'un cadre négative ou non finie
  ramenée à 0 (inchangé, 0 = « non donnée ») ; **taille non nulle d'au moins 500** par côté (`clampZones` ramène
  400 à 500), d'après la boîte de taille personnalisée de l'original (400 × 300 devenus 500 × 500) — minimum vu, pas
  cherché plus loin : **hypothèse H4-20** (plan 4a), écrite en commentaire au-dessus de la constante
  `inline constexpr double kMinFrameSide = 500; // H4-20 : minimum vu dans l'original (L4-O2), ni maximum ni pas cherchés`
  ajoutée dans `pagezones.h`. Le natif lit `zones` par `clampZones`.
- **Gabarit du format natif** (trouvé par L4-T6) : la clé `zones`, omise quand ses quatre valeurs sont nulles, entre
  dans le gabarit des clés connues de la page avec ses quatre sous-clés hors défaut : dans `formats/src/nativejson.cpp`
  (`:1692-1694`), après `everyPageKey.baseHeight = 1;`, la ligne
  `everyPageKey.zones = PageZones{.horizontal = 1, .vertical = 1, .frameWidth = 1, .frameHeight = 1}; // « zones » n'est écrite que hors défaut (L4-T27)`.
  Sans elle, `zones` est lue comme clé inconnue, gardée dans `unknownKeys` et réécrite : l'empreinte change. Test
  `zonesKeyIsKnown` (code ci-dessous).
- **Accrochage** : les séparations n'attirent pas les objets et n'ajoutent aucun point d'accrochage (L4-O2) ; aucune
  modification de `core/…/gridsnap.*`. « La grille d'accrochage repart-elle de l'origine de chaque cadre ? » n'est pas
  tranché (L4-O2) : on garde l'origine de la page, supposé, **hypothèse H4-21** ; test `separatorsDoNotAttract`.
- **Tests** : `zonesAreBounded` étendu, `extendKeepsTheFrameSize` sur des tailles valides (500 × 600),
  `separatorsDoNotAttract` ajouté (`tst_core_zones`) ; `zonesRoundTrip` sur 2 × 1 cadres de 500 × 600 et
  `zonesKeyIsKnown` ajouté (`tst_formats_lot4`). Code ci-dessous, à l'étape 1. Fichiers inchangés (ceux de la ligne).

**Amendement 4 (D11).** D11 tranche la donnée : une « zone » est un **cadre** ; la page compte un nombre de cadres en
largeur (**0 à 40**) et en hauteur (**0 à 53**), chaque cadre ayant une taille en pixels. Dans `PageZones`,
`horizontal` est le nombre de cadres en largeur et `vertical` le nombre de cadres en hauteur ; `clampZones` borne à 0-40
et 0-53 (et non 0-10). **La taille d'un cadre est stockée**, comme D11 l'a tranché : `PageZones` reçoit, après les
deux nombres, `double frameWidth = 0;` et `double frameHeight = 0; // taille d'un cadre en pixels (D11) ; 0 : non
donnée` ; `clampZones` borne les nombres et ramène une taille négative ou non finie à 0 ; l'empreinte (queue `0xA6`)
porte les deux nombres puis les deux tailles (motifs IEEE 754), seulement si l'un des quatre n'est pas nul ; le
natif écrit `"zones": {"horizontal": n, "vertical": m, "frameWidth": w, "frameHeight": h}`, omis si les quatre sont
nuls. `page.zones` (`"<h>x<v>"`) ne change que les nombres ; L4-C1 écrit la taille ; l'allongement
(`ExtendPageCommand`, L4-T4) ne la modifie pas (test `extendKeepsTheFrameSize`). Les tests initialisent `PageZones`
par champs nommés (`PageZones{.horizontal = 2}`), jamais par position (règle Clang des règles communes). Le dessin des
séparations et l'effet sur les objets restent à L4-T28, après
L4-O1. `.flipchart` : propriété de page lue si notre documentation du format en donne l'identifiant (ancien dépôt,
`docs/re/format-flipchart.md`), sinon non lue, déclarée au rapport, et `zonesRoundTrip` ne couvre que le natif.

**Amendement 1 (défaut 7, mineur réservations).** `SetPageZonesCommand` émet `Change::Kind::PageSettingsChanged`
(L0-S3), ignoré par le cache : `zonesChangeIsIgnoredByTheCache` l'attend. `pagezones.h`, `pagezones.cpp` existent
(talons vides de L4-T1) et `src/pagezones.cpp` est déjà dans `core/CMakeLists.txt` : ce fichier n'est pas touché.
Tout `switch` sur `Change::Kind` traite déjà `PageSettingsChanged` (`/we4062`).

**Objectif.** Selon D11 (si D11 n'est pas tranché, la tâche ne commence pas). Contrat par défaut, à amender par
D11 : `struct PageZones { std::int32_t horizontal = 0; std::int32_t vertical = 0; double frameWidth = 0; double
frameHeight = 0; };` (amendement 4) ajouté à la fin de `Page`
(`model.h`), bornes 0 à 40 (`horizontal`, cadres en largeur) et 0 à 53 (`vertical`, cadres en hauteur), amendement
4 ; empreinte : queue `0xA6` + les deux nombres puis les deux tailles, seulement si l'un des quatre n'est pas nul
(comme L4-T1) ; `core/include/actcraft/core/pagezones.h` : `class SetPageZonesCommand final : public UndoCommand`
(`SetPageZonesCommand(std::size_t page, PageZones zones)`, une étape, changement de page **ignoré par le cache**
(contrat A13 de L4-T4 : les séparations sont dessinées en surcouche) et `PageZones clampZones(PageZones) noexcept`
; natif : clé `"zones": {"horizontal": n, "vertical": m, "frameWidth": w, "frameHeight": h}` omise si les quatre
sont nuls (amendement 4) ; `.flipchart` : propriété de page relevée par
D11, écrite à la main dans `flipchartids.h`.

**Fichiers.** Ceux de la ligne L4-T27.

**Test et commande bornée.** Commande ciblée `-R 'tst_core_zones|tst_formats_lot4|tst_core_model'` ; fonctions
`zonesKeepOldFingerprints`, `setZonesIsOneStep`, `zonesChangeIsIgnoredByTheCache`, `zonesAreBounded`,
`extendKeepsTheFrameSize`, `separatorsDoNotAttract`, `zonesRoundTrip`, `zonesKeyIsKnown` (amendement 5)
en `PASS` ; preuve de tâche.

**Interdits.** Pas de QtGui. Un fichier sans zones garde ses octets.

**Terminé quand.** Fonctions vertes ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** `tests/tst_core_zones.cpp` :
  ```cpp
  #include <actcraft/core/document.h>
  #include <actcraft/core/gridsnap.h>
  #include <actcraft/core/pagecache.h>
  #include <actcraft/core/pagesettings.h>
  #include <actcraft/core/pagezones.h>
  #include <actcraft/core/undo.h>

  #include <QtTest>

  #include <limits>

  using namespace actcraft::core;

  class TstCoreZones : public QObject
  {
      Q_OBJECT

  private slots:
      void zonesKeepOldFingerprints()
      {
          Page page;
          const auto before = fingerprint(page);
          page.zones.horizontal = 2;
          QVERIFY(fingerprint(page) != before);
          page.zones.horizontal = 0;
          QCOMPARE(fingerprint(page), before);
          page.zones.frameWidth = 400; // amendement 4 : la taille d'un cadre entre dans l'empreinte
          QVERIFY(fingerprint(page) != before);
          page.zones.frameWidth = 0;
          QCOMPARE(fingerprint(page), before);
      }
      void extendKeepsTheFrameSize() // amendement 4 (D11) : l'allongement ne touche pas les cadres
      {
          Document d;
          d.page(0).zones = PageZones{.horizontal = 2, .vertical = 3, .frameWidth = 500, .frameHeight = 600};
          UndoStack stack;
          stack.push(d, std::make_unique<ExtendPageCommand>(0));
          QCOMPARE(d.page(0).zones.frameWidth, 500.0);
          QCOMPARE(d.page(0).zones.frameHeight, 600.0);
          QCOMPARE(d.page(0).zones.vertical, 3);
      }
      void setZonesIsOneStep()
      {
          Document d;
          UndoStack stack;
          stack.push(d, std::make_unique<SetPageZonesCommand>(0, PageZones{.horizontal = 2}));
          QCOMPARE(d.page(0).zones.horizontal, 2);
          stack.undo(d);
          QCOMPARE(d.page(0).zones.horizontal, 0);
      }
      void zonesChangeIsIgnoredByTheCache()
      {
          struct Spy final : PageCache
          {
              int calls = 0;
              void insert(ObjectId) override { ++calls; }
              void remove(ObjectId, const RectF&) override { ++calls; }
              void invalidate(ObjectId, const CacheArea&) override { ++calls; }
          } spy;
          Document d;
          UndoStack stack;
          const Changes c = stack.push(d, std::make_unique<SetPageZonesCommand>(0, PageZones{.vertical = 3}));
          QCOMPARE(c.size(), std::size_t{1});
          applyToCache(c, 0, spy);
          QCOMPARE(spy.calls, 0);
      }
      void zonesAreBounded() // amendement 4 (D11) : 0 à 40 en largeur, 0 à 53 en hauteur
      {
          const PageZones z = clampZones({.horizontal = -1, .vertical = 60});
          QCOMPARE(z.horizontal, 0);
          QCOMPARE(z.vertical, 53);
          const PageZones w = clampZones({.horizontal = 50, .vertical = 2});
          QCOMPARE(w.horizontal, 40);
          QCOMPARE(w.vertical, 2);
          const PageZones s = clampZones({.frameWidth = -5, .frameHeight = std::numeric_limits<double>::quiet_NaN()});
          QCOMPARE(s.frameWidth, 0.0); // amendement 4 : taille négative ou non finie ramenée à 0
          QCOMPARE(s.frameHeight, 0.0);
          const PageZones m = clampZones({.frameWidth = 400, .frameHeight = 300});
          QCOMPARE(m.frameWidth, 500.0); // amendement 5 (H4-20) : un cadre fait au moins 500 de côté
          QCOMPARE(m.frameHeight, 500.0);
          QCOMPARE(clampZones({.frameWidth = 0, .frameHeight = 700}).frameWidth, 0.0); // 0 : non donnée, gardée
          QCOMPARE(clampZones({.frameWidth = 0, .frameHeight = 700}).frameHeight, 700.0);
      }
      // Amendement 5 (L4-O2, H4-21) : les séparations n'attirent pas un objet déplacé et n'ajoutent aucun point
      // d'accrochage ; la grille part de l'origine de la page, pas de celle de chaque cadre (supposé). Passe par
      // snapDelta, qui reçoit la page entière (zones comprises), comme l'outil de sélection (L4-T9). Cadres de 500 :
      // séparations en x = 500 et y = 500, qui ne sont pas des multiples de 32.
      void separatorsDoNotAttract()
      {
          Page page;
          page.width = 1000;
          page.height = 1500;
          page.zones = PageZones{.horizontal = 2, .vertical = 3, .frameWidth = 500, .frameHeight = 500};
          page.grid.snapEnabled = true;
          page.grid.levels.push_back(GridLevel{}); // pas de 32, départ en (0, 0)
          PageObject shape{Shape{}};
          shape.common.id = 1;
          std::get<Shape>(body(shape)).bounds = RectF{497, 463, 40, 40}; // coin bas gauche en (497, 503)
          page.objects.push_back(shape);
          const PointF anchor = snapAnchor(page.objects.front());
          QCOMPARE(anchor.x, 497.0);
          QCOMPARE(anchor.y, 503.0);
          // Ancrage ramené sur (512, 512) : +15, +9. Vers la séparation (500, 500), ou une grille repartie du
          // cadre, ce serait +3, -3.
          const PointF delta = snapDelta(page, {ObjectId{1}}, PointF{0, 0});
          QCOMPARE(delta.x, 15.0);
          QCOMPARE(delta.y, 9.0);
      }
  };

  QTEST_GUILESS_MAIN(TstCoreZones)
  #include "tst_core_zones.moc"
  ```
  Dans `tests/tst_formats_lot4.cpp` : `zonesRoundTrip` (2 × 1 cadres de 500 × 600 relus, taille comprise ; document
  sans zones : pas de clé `"zones"` ; amendement 5 : tailles valides, au moins 500) et, amendement 5 :
  ```cpp
      // Amendement 5 (trouvé par L4-T6) : « zones », omise à zéro, est dans le gabarit everyPageKey avec ses quatre
      // sous-clés hors défaut ; sinon elle serait lue comme inconnue (unknownKeys) et l'empreinte changerait.
      void zonesKeyIsKnown()
      {
          core::Document d = plain();
          d.page(0).zones = core::PageZones{.horizontal = 2, .vertical = 3, .frameWidth = 500, .frameHeight = 600};
          ReadBack back = readBack(formats::documentToJson(d));
          QVERIFY2(back.result.ok && back.result.warnings.isEmpty(), qPrintable(report(back.result)));
          core::Page& page = back.document.page(0);
          QVERIFY2(page.unknownKeys.empty(), page.unknownKeys.c_str());
          QCOMPARE(core::fingerprint(back.document), core::fingerprint(d));
          page.zones = core::PageZones{}; // remise au défaut : plus de clé « zones » dans le fichier réécrit
          QVERIFY(!bytes(back.document).contains("\"zones\":"));
      }
  ```
  (Aides `plain`, `readBack`, `report`, `bytes` : celles du fichier, posées par L4-T6.) `zonesKeyIsKnown` doit être
  rouge si la ligne du gabarit manque (le vérifier en la retirant un instant, règle « Mutation »).
- [ ] **Étape 2 : implémenter** (modèle, empreinte, commande, `core/CMakeLists.txt`, natif, `.flipchart`).
- [ ] **Étape 3 : vert**, preuve. **Étape 4 : commit.**
  ```powershell
  git -C <wt> add core/include/actcraft/core/model.h core/src/model.cpp core/include/actcraft/core/pagezones.h core/src/pagezones.cpp tests/tst_core_zones.cpp formats/src/nativejson.cpp formats/src/flipchart.cpp formats/src/flipchartids.h tests/tst_formats_lot4.cpp
  git -C <wt> commit -m "core, formats : zones de page, une étape, lues et écrites (L4-T27)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t25"></a>
### Tâche L4-T25 — Texte de haut en bas II : outil, barre ; scénario S16

**Objectif.** `TextEditing` en mode `vertical` (`beginAt(…, true)`) : cadre d'une colonne (largeur de L4-T24), poignée
d'extension masquée, objet créé avec `vertical = true`. `VerticalTextBar.qml` : visible quand `appText.active` et
`vertical` ; police, taille, gras, italique, souligné (F-301 b). Scénario `texte-vertical` (S16).

**Fichiers.** `ui/qml/VerticalTextBar.qml`, `ui/src/textediting.cpp`, `tests/tst_ui_text.cpp`,
`bench/scenarios/texte-vertical.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_text|tst_captures_texte_vertical'` ; fonctions
`verticalFrameIsOneColumn`, `verticalBarOffersFiveCommands` en `PASS` ; preuve visuelle ; preuve.

**Preuve visuelle.** `captures.ps1 -Scenario texte-vertical … -Lire "appText.vertical=true"` : `LU … OK`,
`PIXEL lettre-a OK`, `PIXEL lettre-b OK`, `PIXEL lettre-c OK` (trois bandes horizontales), `LU
appDocument.fingerprint=${avant} OK` après réouverture, `CAPTURES OK` ; décrire `abc.png` et `rouvert.png`.

**Scénario** : `waitReady` ; `tool.text-vertical` ; appui en (400, 200) ; `expect appText.vertical true` ;
`text.select-all` ; `typeText "ABC"` ; `text.select-all` ; `text.bold` ; `tool.pen` ; `waitReady` ; trois
`checkPixel` (bandes de A, B, C, coordonnées calculées pour la police par défaut à 14 points et écrites dans le
JSON après un premier passage regardé) ; `capture abc` ; `remember appDocument.fingerprint as avant` ;
`command file.save-as` vers `${SORTIE}/v.actcraft` ; `command file.open` du même ; `waitReady` ; `expect
appDocument.fingerprint ${avant}` ; `capture rouvert`.

**Interdits.** Comme L4-T23.

**Terminé quand.** Fonctions vertes, scénario vert, `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : tests rouges** (`verticalFrameIsOneColumn` : `beginAt({200,200}, true)`, `frame().width()` < 1,5 ×
  la plus grande avance de « W » à la police par défaut ; `verticalBarOffersFiveCommands` : le QML de
  `VerticalTextBar` chargé expose cinq boutons nommés `vbar-font`, `vbar-size`, `vbar-bold`, `vbar-italic`,
  `vbar-underline`). Scénario rouge.
- [ ] **Étape 2 : implémenter.** **Étape 3 : vert**, preuve visuelle, preuve. **Étape 4 : commit.**
  ```powershell
  git -C <wt> add ui/qml/VerticalTextBar.qml ui/src/textediting.cpp tests/tst_ui_text.cpp bench/scenarios/texte-vertical.json
  git -C <wt> commit -m "ui : texte de haut en bas et sa barre ; scénario texte-vertical (L4-T25)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t28"></a>
### Tâche L4-T28 — Zones de page II : surcouche, réglage ; scénario S15 (partie 2)

**Amendement 5 — plus suspendue (D11 observé par L4-O2).** Prime sur l'amendement 4 et sur l'objectif. L4-O2 a
observé : séparations fines, en pointillé (un pixel sur trois environ), claires, aux limites des cadres (page de
1024 × 768 à 2 × 3 cadres : x = 512 ; y = 256 et 512), en édition comme en présentation ; page suivante change de page
sans parcourir les cadres ; les séparations n'attirent pas un objet déplacé et n'ajoutent aucun point d'accrochage. La
dépendance à L4-O1 tombe : cette tâche dépend de **L4-O2** (mergée). Contrat :
- **Dessin.** `PageZonesOverlay.qml`, dans `pageOverlays` (posé par L4-T2) : n cadres en largeur donnent n − 1
  séparations verticales en `x = k × largeur / n`, m cadres en hauteur m − 1 séparations horizontales en
  `y = k × hauteur / m` (page courante, H4-23) ; trait d'**un pixel d'écran** à tout zoom, **pointillé d'un pixel sur
  trois** ; couleur **`Theme.separationCadre`**, jeton à nous ajouté à `ui/qml/Theme.qml` (couleur exacte de l'original
  non mesurée). Aucun redessin du cache (surcouche hors de la scène que le cache photographie, `zonesRefreshNoCache`).
- **Édition et présentation.** Notre présentation est le plein écran (`view.fullscreen`) : la surcouche ne dépend pas
  de lui, elle reste dessinée (lecture `pageZones.visible=true` en plein écran).
- **Aucun effet** sur les objets ni sur l'accrochage (rien à coder ; `separatorsDoNotAttract`, L4-T27) ; **page
  suivante** ne parcourt pas les cadres (rien à coder ; test `nextPageIgnoresFrames`).
- **Branchement.** `PageArea.qml` déclare `property string zones: ""` et passe `zones: area.zones` et
  `zoom: view.zoom` à `PageZonesOverlay` (objet `pageZones`, déjà là) ; `Main.qml` passe `zones: appPageSettings.zones`
  à son `PageArea` (une ligne, à côté de `documentController: appDocument`). `PageOptions::setZones("<h>x<v>")` exécute
  `SetPageZonesCommand` (nombres seulement ; la taille d'un cadre n'est écrite que par L4-C1), `zones` suit la page.
- **Fichiers** (ligne de la table) : `ui/qml/PageZonesOverlay.qml`, `ui/qml/PageArea.qml`, `ui/qml/Main.qml`,
  `ui/qml/Theme.qml`, `ui/src/pageoptions.cpp`, `tests/tst_ui_pagesettings.cpp`, `bench/scenarios/page-allongee-zones.json`.
  `PageArea.qml` suit L4-C3 et L4-T7 (plan 4a, mergées avant V9) ; `Main.qml` suit L4-T19 (V9).

**Jeton du thème** (`ui/qml/Theme.qml`, après `separateur`) :
```qml
    readonly property color separationCadre: "#B9C0C8" // séparations des cadres d'une page (décor, sans seuil)
```

**`ui/qml/PageZonesOverlay.qml`** (remplace le talon) :
```qml
import QtQuick
import ActCraft.Ui

// Séparations des cadres d'une page (D11 ; observation de l'original L4-O2) : fines, en pointillé (un pixel d'écran
// sur trois), aux limites des cadres, en édition comme en plein écran. Dans `pageOverlays`, hors de la scène que le
// cache photographie : les montrer ou les changer ne redessine pas le cache. Elles n'attirent rien (L4-O2).
Item {
    id: overlay
    property string zones: "" // « <cadres en largeur>x<cadres en hauteur> » (appPageSettings.zones)
    property real zoom: 1     // view.zoom : un trait d'un pixel d'écran à tout zoom
    readonly property int across: overlay.zonePart(0)
    readonly property int down: overlay.zonePart(1)
    // n cadres : n − 1 séparations ; lu par le scénario page-allongee-zones.
    readonly property int separatorCount: Math.max(0, across - 1) + Math.max(0, down - 1)
    visible: separatorCount > 0

    function zonePart(i) {
        const parts = overlay.zones.split("x")
        return parts.length === 2 ? Math.max(0, parseInt(parts[i], 10) || 0) : 0
    }

    // Une séparation : un Canvas d'un pixel d'écran d'épaisseur, peint à l'échelle de l'écran (scale 1 / zoom) ;
    // il ne se repeint que si sa longueur change (zoom, taille de la page).
    component Separation: Canvas {
        property bool vertical: true
        property real at: 0 // position en coordonnées de page
        x: vertical ? at : 0
        y: vertical ? 0 : at
        width: vertical ? 1 : Math.max(1, Math.ceil(overlay.width * overlay.zoom))
        height: vertical ? Math.max(1, Math.ceil(overlay.height * overlay.zoom)) : 1
        scale: 1 / Math.max(0.01, overlay.zoom)
        transformOrigin: Item.TopLeft
        smooth: false // pas d'échantillonnage linéaire : chaque point garde sa couleur, même à une position fractionnaire
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d")
            ctx.clearRect(0, 0, width, height)
            ctx.fillStyle = Theme.separationCadre
            const length = vertical ? height : width
            for (let i = 0; i < length; i += 3) {
                if (vertical)
                    ctx.fillRect(0, i, 1, 1)
                else
                    ctx.fillRect(i, 0, 1, 1)
            }
        }
    }

    Repeater {
        model: Math.max(0, overlay.across - 1)
        delegate: Separation {
            required property int index
            vertical: true
            at: overlay.width * (index + 1) / overlay.across
        }
    }
    Repeater {
        model: Math.max(0, overlay.down - 1)
        delegate: Separation {
            required property int index
            vertical: false
            at: overlay.height * (index + 1) / overlay.down
        }
    }
}
```
Interdits de plus : un élément par point du pointillé ; une image de la taille de la page au zoom ; tout dessin des
cadres dans la page fixe ou dans le cache ; toute modification de l'accrochage.

**Test de plus** dans `tests/tst_ui_pagesettings.cpp` (montage de L4-T13) :
```cpp
    // Amendement 5 (D11, L4-O2) : page suivante change de page et ne parcourt pas les cadres.
    void nextPageIgnoresFrames()
    {
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::PageOptions options;
        options.setDocumentController(&controller);
        options.setZones(QStringLiteral("2x3"));
        QCOMPARE(options.zones(), QStringLiteral("2x3"));
        controller.nextPage();
        QCOMPARE(controller.currentIndex(), 1);
        QCOMPARE(controller.pageCount(), 2); // page ajoutée en fin, comme sans cadres
    }
```
**Scénario** (partie 2) : celui de l'objectif, amendé — avant `page.zones "2x0"`, le même segment sans séparation :
`{"checkEdge": "separation-absente", "from": [508, 300], "to": [516, 336], "maxIntermediate": 0, "minInk": 0}`
(le contrôle de présence a ainsi un état rouge) ; après `expect appPageSettings.zones "2x0"`, `expect
pageZones.separatorCount 1` ; `{"checkEdge": "separation-zone", "from": [508, 300], "to": [516, 336],
"minIntermediate": 1, "minInk": 0}` à la place du `checkPixel` de l'objectif. Pourquoi : `checkPixel` fait la
moyenne d'un carré de 3 × 3 (`bench/src/contentcheck.cpp:54-75`), et un pointillé d'un pixel y donne un gris presque
blanc (environ `#F7F8F9`) ; `checkEdge` compte les pixels un à un le long du segment (`bench/src/uiscenario.cpp:1057-1085`).
Son « encre » est le noir à 24 près : notre `#B9C0C8` y compte comme pixel **intermédiaire** (ni blanc ni noir à 24
près), d'où `minIntermediate` 1 et `minInk` 0 — un `minInk ≥ 1` échouerait. Le segment, presque vertical, passe par
les colonnes 508 à 516 sur 36 lignes : il croise la séparation (x = 512) sur plusieurs lignes, donc au moins un point
du pointillé, même décalé d'un pixel (`smooth: false` sur `Separation` : un point n'est pas dilué sur deux pixels) ;
coordonnées confirmées au premier passage regardé ; un `ECART` se diagnostique en lisant les pixels autour de x = 512,
jamais en changeant les seuils ; après `capture zones` : `page.zones "2x3"` ; `waitReady` ; `expect
pageZones.separatorCount 3` ; `view.fullscreen` ; `waitReady` ; `expect pageZones.visible true` ; `expect
pageZones.separatorCount 3` ; `capture zones-plein-ecran` ; `view.fullscreen` ; `waitReady` ; puis la fin de
l'objectif (empreinte, enregistrer, rouvrir). Commande ciblée et preuve : celles de la section, fonctions
`zonesRefreshNoCache` et `nextPageIgnoresFrames` en `PASS`. Commit : ajouter `ui/qml/PageArea.qml ui/qml/Main.qml
ui/qml/Theme.qml` au `git add`.

**Objectif.** Selon D11 (contrat de l'amendement 5 ci-dessus, qui prime : pointillé d'un pixel sur trois, jeton
`Theme.separationCadre`). Par défaut : `PageZonesOverlay.qml` (dans `pageOverlays`) dessine les séparations des zones
en traits tiretés à nous (n zones horizontales = n − 1 lignes, de même pour les verticales) ; aucun redessin du
cache ; `PageOptions::setZones("<h>x<v>")` exécute `SetPageZonesCommand` ; `zones` suit la page ;
commande `page.zones` (et l'entrée de réglage retenue par D11). Le scénario `page-allongee-zones` gagne la partie 2.

**Fichiers.** `ui/qml/PageZonesOverlay.qml`, `ui/qml/PageArea.qml`, `ui/qml/Main.qml`, `ui/qml/Theme.qml`
(amendement 5), `ui/src/pageoptions.cpp`, `tests/tst_ui_pagesettings.cpp`, `bench/scenarios/page-allongee-zones.json`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_pagesettings|tst_captures_page_allongee_zones'` ; fonctions
`zonesRefreshNoCache` et `nextPageIgnoresFrames` (amendement 5) en `PASS` ; preuve visuelle ; banc sans écran ; preuve.

**Preuve visuelle.** `… -Lire "appPageSettings.zones=2x0" -Lire "pageZones.separatorCount=1"` : `LU … OK`, `BORD
separation-zone OK` (le segment qui croise x = 512 porte au moins un pixel du pointillé, amendement 5), `LU pageZones.separatorCount=3 OK` après `"2x3"`,
`LU pageZones.visible=true OK` en plein écran, `LU appDocument.fingerprint=${avant} OK` après aller-retour `.actcraft`,
`CAPTURES OK` ; décrire `zones.png` (une séparation verticale pointillée au milieu) et `zones-plein-ecran.png` (une
verticale, deux horizontales, en plein écran) — amendement 5.

**Scénario** (partie 2, après la partie 1) : `page.zones "2x0"` ; `waitReady` ; `expect appPageSettings.zones
"2x0"` ; `remember pageView.cacheRefreshCount as r` avant la commande et `expect … ${r}` après ; `checkEdge
separation-zone` (amendement 5 : pas de `checkPixel`) ; `capture zones` ; `remember appDocument.fingerprint as avant` ;
`file.save-as` `${SORTIE}/z.actcraft` ; `file.open` ; `waitReady` ; `expect appDocument.fingerprint ${avant}`.

**Interdits.** Aucun dessin des zones dans la page fixe.

**Terminé quand.** Fonctions vertes (`zonesRefreshNoCache`, `nextPageIgnoresFrames`), scénario vert, captures
décrites, banc joint, `TESTS OK`, `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge** `zonesRefreshNoCache` (espion de cache : `setZones("2x0")` → aucun appel ;
  `zones == "2x0"`). Scénario rouge.
- [ ] **Étape 2 : implémenter.** **Étape 3 : vert**, preuve visuelle, banc, preuve. **Étape 4 : commit.**
  ```powershell
  git -C <wt> add ui/qml/PageZonesOverlay.qml ui/qml/PageArea.qml ui/qml/Main.qml ui/qml/Theme.qml ui/src/pageoptions.cpp tests/tst_ui_pagesettings.cpp bench/scenarios/page-allongee-zones.json
  git -C <wt> commit -m "ui : séparations des cadres d'une page en surcouche ; scénario page-allongee-zones complet (L4-T28)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t29"></a>
### Tâche L4-T29 — Banc : scénario `miniatures`

**Amendement 1 (défaut 16).** Les rendus de miniatures sont comptés **dans la fenêtre mesurée** (pas après) :
`thumbnails.renders` ≥ 1 entre le premier et le dernier échantillon de la mesure, `thumbnails.during_contact = 0`
(aucun rendu lancé pendant un contact). **Amendement 2 (N10)** : source de `during_contact` — le banc lit
`appThumbnails.launchCount` (amendement 3 : rendus **lancés** ; `renderCount` compte les terminés, revenus d'un fil
secondaire) à chaque appui et à chaque relâché injectés ; `during_contact` = somme des hausses de `launchCount` entre
un appui et son relâché ; écrit dans le JSON (`thumbnails.during_contact`) par
`bench/src/runner.cpp` et lu par `bench/src/result.cpp`. Variante sur `pauses` : le scénario rejoue les traits de `pauses`, panneau
ouvert, pour que les rendus tombent dans les pauses ; ses figements sont comparés à `pauses` sans panneau, dans le
même JSON. Ces attendus sont écrits dans `docs/banc.md`.

**Amendement 5 (trouvé par L4-T17 ; L4-T19 suspend les miniatures panneau caché).** Le banc mesure **les deux cas** :
`miniatures` (panneau ouvert, ci-dessous) et **`miniatures-ferme`** — même scénario, même page générée et trois pages
de plus, **sans** `view.browsers` : `thumbnails.renders = 0` dans la fenêtre mesurée (aucun rendu panneau fermé,
L4-T19), figements et latence comparés à `stroke`. Test de plus dans `tst_bench_scenarios` :
`closedPanelVariantKeepsItClosed` (le scénario `miniatures-ferme` existe et ne déclenche jamais `view.browsers`).
`docs/banc.md` décrit les deux scénarios et leurs attendus. Fichiers inchangés.

**Objectif.** Mesurer que les miniatures ne figent pas l'écriture : scénario `miniatures` = scénario `stroke` joué
**panneau des navigateurs ouvert** (`view.browsers` avant la mesure), page générée et trois pages de plus pour que
le navigateur ait des miniatures à tenir à jour. Le résultat gagne `thumbnails` : `{"renders": n,
"max_copy_ms": x}` lus sur `appThumbnails` (`renderCount`, et le maximum de `lastCopyMs` relevé à chaque
`renderingChanged`). Comparaison avec `stroke` à la clôture : mêmes critères de figement et de latence (DA7).

**Fichiers.** Ceux de la ligne L4-T29.

**Test et commande bornée.** Commande ciblée `-R 'tst_bench_scenarios|tst_bench_result'` ; fonctions
`thumbnailsScenarioOpensTheBrowsers`, `closedPanelVariantKeepsItClosed`, `thumbnailsShowInTheResult` en `PASS` ; banc
sans écran : `miniatures` dans le JSON, `thumbnails.renders ≥ 1` ; `miniatures-ferme` dans le JSON,
`thumbnails.renders = 0` (amendement 5) ; preuve de tâche.

**Interdits.** Ne pas changer `stroke` ni ses critères.

**Terminé quand.** Fonctions vertes ; JSON joint ; `docs/banc.md` décrit `miniatures` ; `TESTS OK`.

**Étapes.**
- [ ] **Étape 1 : tests rouges** (`thumbnailsScenarioOpensTheBrowsers` : le scénario existe et déclenche
  `view.browsers` avant la mesure ; `thumbnailsShowInTheResult` : un résultat avec `thumbnails` sérialise et relit
  `renders` et `max_copy_ms`). **Étape 2 : implémenter.** **Étape 3 : vert**, banc sans écran, preuve.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add bench/include/actcraft/bench/scenarios.h bench/src/scenarios.cpp bench/src/runner.cpp bench/include/actcraft/bench/result.h bench/src/result.cpp tests/tst_bench_scenarios.cpp tests/tst_bench_result.cpp docs/banc.md
  git -C <wt> commit -m "banc : scénario miniatures, écriture mesurée panneau des navigateurs ouvert (L4-T29)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-t30"></a>
### Tâche L4-T30 — Scénario S7 `formes-texte-pages-grille`

**Objectif.** S7 du cadrage, joué hors écran puis à l'écran en D3D11 : tracer un rectangle, poser « Bonjour » par
touches injectées, la miniature de la page se met à jour ; glisser la page 3 avant la page 1 dans le navigateur ;
grille aimantée : le rectangle déplacé pose son coin bas gauche sur une intersection.

**Fichiers.** `bench/scenarios/formes-texte-pages-grille.json`.

**Test et commande bornée.** Commande ciblée `-R tst_captures_formes_texte_pages_grille` ; preuve visuelle hors
écran ; à l'écran sous le jeton `ecran` (annoncé) : même commande avec `-Rendu d3d11` sans `-HorsEcran` ; preuve.

**Scénario** : `waitReady` ; `view.browsers` ; deux `page.next` puis `page.goto 1` (3 pages) ; `tool.shape` ;
`shape.choose rectangle` ; `stroke [[212, 212], [312, 292]]` ; `waitReady` ; `remember appThumbnails.renderCount as
r0` ; `tool.text` ; appui en (500, 200) ; `text.select-all` ; `typeText "Bonjour"` ; `tool.pen` ; `waitFor
appThumbnails.renderCount` supérieur à `${r0}` (valeur `${r0}` + 1, `timeoutMs` 3000) ; `capture miniature` ;
`grid.designer` ; pas de 50, accrochage coché (`pointer` sur `gridSpacingX`, `typeText "50"`, `gridSnapCheck`) ;
fermer ; `tool.select` ; glisser le rectangle de (260, 250) à (290, 250) (`pointer`) ; `waitReady` ;
`checkPixel coin-accroche [250, 300] #000000 60` (coin bas gauche sur l'intersection (250, 300)) ; `capture accroche`
; glisser `pageThumb3` au-dessus de `pageThumb1` ; `expect appDocument.currentIndex 0` ; `expect
appDocument.pageCount 3` ; `capture pages`.

**Interdits.** Ne pas modifier les QML pour faire passer le scénario : un écart est rapporté et corrigé par la tâche
du fichier (correctif `L4-C<n>`).

**Terminé quand.** `tst_captures_formes_texte_pages_grille` vert ; hors écran et à l'écran `CAPTURES OK` ; captures
regardées et décrites.

**Étapes.**
- [ ] **Étape 1 : scénario** (JSON écrit en entier). **Étape 2 : hors écran** jusqu'au vert. **Étape 3 : à l'écran
  D3D11** (jeton `ecran`). **Étape 4 : commit.**
  ```powershell
  git -C <wt> add bench/scenarios/formes-texte-pages-grille.json
  git -C <wt> commit -m "Scénario S7 : formes, texte, pages et grille (L4-T30)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-c1"></a>
### Tâche L4-C1 — Correctif : taille personnalisée d'un paperboard neuf en pixels d'un cadre et en nombre de cadres

**Amendement 5 (D11, L4-O2).** Prime sur l'origine et l'objectif là où ils diffèrent. La règle supposée est
**observée** : taille de page = nombre de cadres × taille d'un cadre (2 × 3 cadres → page de 1000 × 1500). La boîte de
l'original ramène un cadre de 400 × 300 à **500 × 500** : un cadre fait au moins 500 par côté (**H4-20**, minimum vu,
pas cherché plus loin). Chez nous :
- `PageSizeDialog.qml` : `readonly property bool framed: framesAcrossBox.value > 0 || framesDownBox.value > 0`
  (`framesAcrossBox`, `framesDownBox` : identifiants des champs `pageFramesAcrossBox` et `pageFramesDownBox`) ; les deux
  champs de taille ont `from: dialog.framed ? 500 : 200` (200 : minimum du lot 1, sans cadre) ; `acceptFramedSize`
  remplit **d'abord** les deux nombres de cadres, puis les deux tailles (le minimum de 500 s'applique), puis accepte ;
  `acceptSize` remet d'abord les deux nombres à 0, puis remplit les tailles.
- `newDocumentWithFrames` : `core::clampZones` (L4-T27) applique aussi le minimum de 500 à une taille non nulle ;
  la règle « cadre × nombre de cadres, chaque côté borné à 8000 » est inchangée.
- `customSizeTakesFrames` : attendus corrigés (code de l'étape 1).
Dépend de L4-O2 en plus de L4-O1.

**Origine.** Amendement 4 : D16 (écart E10). Dans la 3.5.18, la taille personnalisée d'un paperboard neuf se donne en
pixels pour **un cadre** (préréglages et autre) et en **nombre de cadres** en largeur et en hauteur ; F-700 et notre
boîte (`ui/qml/PageSizeDialog.qml`, lot 1) ne donnent que des pixels. Même donnée que les zones de page (D11) : après
L4-T27 (`core::PageZones`, `core::clampZones`, taille d'un cadre comprise) et après L4-T26 (`documentcontroller.cpp`).
Après L4-O1 aussi : la règle de la taille de page ci-dessous (cadre × nombre de cadres) est supposée ; si le verdict
de L4-O1 sur D11 la contredit, l'orchestrateur amende cette section avant de la lancer. Si L4-O1 rend « non observé »,
L4-C1 se joue quand même avec cette règle : la donnée (taille d'un cadre et nombres) est tranchée par D11 et D16.

**Objectif.**
- `PageSizeDialog.qml` : « Largeur d'un cadre (pixels) » et « Hauteur d'un cadre (pixels) » (les deux champs
  existants, mêmes noms d'objet `pageWidthBox`, `pageHeightBox`), plus « Cadres en largeur » (`pageFramesAcrossBox`,
  0 à 40, 0 par défaut) et « Cadres en hauteur » (`pageFramesDownBox`, 0 à 53, 0 par défaut). Libellés à nous.
  `acceptSize(w, h)` garde sa signature et son effet (il remet en plus les deux nombres de cadres à 0) : `newSizesComeFromTheirCommands`
  (`tests/tst_ui_filemenu.cpp:450-451`) ne change pas. Fonction ajoutée `acceptFramedSize(w, h, across, down)`, qui
  remplit les quatre champs puis accepte. Signal `sizeAccepted(real width, real height, int across, int down)`.
- `FileActions.qml` : `onSizeAccepted: (w, h, across, down) => actions.newDocumentWithFrames(w, h, across, down)` et
  `function newDocumentWithFrames(w, h, across, down) { guard(function () {
  actions.document.newDocumentWithFrames(w, h, across, down) }) }` (même garde que `newDocument`).
- `DocumentController` : à la fin des membres publics, signalé, `// L4-C1 (D16) : ajout en fin.` puis
  `Q_INVOKABLE void newDocumentWithFrames(double frameWidth, double frameHeight, int across, int down);`. Corps :
  `zones = core::clampZones({.horizontal = across, .vertical = down, .frameWidth = frameWidth, .frameHeight =
  frameHeight})` ; page de `frameWidth × max(1, zones.horizontal)` sur `frameHeight × max(1, zones.vertical)` (règle
  observée par L4-O2, amendement 5), chaque côté borné à 8000 px en ramenant le nombre de cadres au plus grand qui
  tient ; `page(0).zones = zones` (taille d'un cadre comprise) **seulement si** `across > 0 || down > 0`, sinon
  `PageZones{}` : sans cadre, le paperboard est celui du lot 1, sans zones, même empreinte et même fichier que
  `newDocument` ; puis comme `newDocument` (`replaceDocument` vers un
  paperboard « Sans titre », `forgetStaleLift`) : aucune étape d'annulation, document non modifié.

**Fichiers.** `ui/qml/PageSizeDialog.qml`, `ui/qml/FileActions.qml`, `ui/include/actcraft/ui/documentcontroller.h`,
`ui/src/documentcontroller.cpp`, `tests/tst_ui_filemenu.cpp`.

**Test et commande bornée.** Commande ciblée `-R tst_ui_filemenu` (`100% tests passed, 0 tests failed out of 1`) ;
fonctions `customSizeTakesFrames` et `newSizesComeFromTheirCommands` (inchangée) en `PASS` ; preuve de tâche :
`100% tests passed, 0 tests failed out of N`, N = 136 + n_B **inchangé** (aucun test enregistré), `TESTS OK`, ligne
`tst_ui_filemenu … Passed` ; `premiers-pas` hors écran `-Lire "pageIndicator.text=1 / 1"` → `CAPTURES OK`.

**Interdits.** Aucun libellé de l'original. Ne pas changer la signature de `newDocument` ni les commandes `file.new-*`.
`documentcontroller.h` : ajout en fin seulement (fichier chaud commun avec le lot 7). Aucune étape d'annulation pour un
paperboard neuf.

**Terminé quand.** `customSizeTakesFrames` et `newSizesComeFromTheirCommands` passent ; `TESTS OK` ; N inchangé ;
`premiers-pas` `CAPTURES OK` ; ajout à `documentcontroller.h` signalé au rapport ; `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge.** Ajouter à `tests/tst_ui_filemenu.cpp`, après `newSizesComeFromTheirCommands`, sur le
  même montage (`Shell`, `start`, `controller`, `run`, `namedObject`) :
  ```cpp
      void customSizeTakesFrames() // L4-C1 (D16)
      {
          Shell s;
          QVERIFY(start(s));
          auto* doc = controller(s);
          QVERIFY(run(s, QStringLiteral("file.new-custom")));
          QObject* sizeDialog = namedObject(s, "pageSizeDialog");
          QVERIFY(sizeDialog);
          QTRY_VERIFY(sizeDialog->property("visible").toBool());
          QVERIFY(QMetaObject::invokeMethod(sizeDialog, "acceptFramedSize", Q_ARG(QVariant, 400),
                                            Q_ARG(QVariant, 300), Q_ARG(QVariant, 2), Q_ARG(QVariant, 3)));
          // Amendement 5 (L4-O2) : un cadre de 400 × 300 devient 500 × 500 (H4-20) ; page = 2 × 3 cadres.
          QCOMPARE(namedObject(s, "pageWidthBox")->property("value").toInt(), 500);
          QCOMPARE(namedObject(s, "pageHeightBox")->property("value").toInt(), 500);
          QTRY_COMPARE(doc->pageSize(), QSizeF(1000, 1500));
          QCOMPARE(doc->document().page(0).zones.horizontal, 2);
          QCOMPARE(doc->document().page(0).zones.vertical, 3);
          QCOMPARE(doc->document().page(0).zones.frameWidth, 500.0); // D11 : taille d'un cadre stockée
          QCOMPARE(doc->document().page(0).zones.frameHeight, 500.0);
          QVERIFY(!doc->canUndo());
          QVERIFY(!doc->property("modified").toBool());
          QVERIFY(QMetaObject::invokeMethod(sizeDialog, "acceptSize", Q_ARG(QVariant, 640), Q_ARG(QVariant, 480)));
          QTRY_COMPARE(doc->pageSize(), QSizeF(640, 480)); // sans cadre : aucune zone, minimum du lot 1 (200)
          QCOMPARE(doc->document().page(0).zones.frameWidth, 0.0);
          QCOMPARE(doc->document().page(0).zones.horizontal, 0);
          doc->newDocumentWithFrames(4000, 300, 3, 0); // cadre de 4000 × 500 (H4-20) ; 3 × 4000 dépasse 8000 : 2 cadres
          QCOMPARE(doc->pageSize(), QSizeF(8000, 500));
          QCOMPARE(doc->document().page(0).zones.horizontal, 2);
      }
  ```
  (Accesseurs réels de `DocumentController` à lire d'abord : `document()`, `canUndo()` et la propriété `modified`
  existent au lot 3 ; adapter les noms sans changer ce qui est vérifié.) Commande ciblée → rouge.
- [ ] **Étape 2 : implémenter** `newDocumentWithFrames`, puis la boîte et `FileActions.qml`. Commande ciblée → vert.
- [ ] **Étape 3 : preuves.** `format.ps1`, preuve de tâche, `premiers-pas` hors écran, `git add`, `NOMS OK`.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add ui/qml/PageSizeDialog.qml ui/qml/FileActions.qml ui/include/actcraft/ui/documentcontroller.h ui/src/documentcontroller.cpp tests/tst_ui_filemenu.cpp
  git -C <wt> commit -m "ui : taille personnalisée d'un paperboard neuf en pixels d'un cadre et en nombre de cadres (L4-C1)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-c2"></a>
### Tâche L4-C2 — Correctif de L4-T18 : valider l'édition du texte avant une commande de pages

**Origine.** Amendement 5, point 7 (trouvé par L4-T21). `DocumentController::execute()` n'annonce rien
(`ui/src/documentcontroller.cpp`, B1 et L3-T12 : une commande de la sélection lancée pendant un geste ne doit pas
faire abandonner son geste à l'outil qui soulève). `PagesController` (L4-T18) passe par lui pour déplacer, insérer,
coller, dupliquer et supprimer des pages, et lit les pages pour copier. Pendant l'édition d'un texte (soulevé, L4-T21),
ces commandes ne valident donc pas le texte d'abord : dupliquer la page affichée donne une copie **sans** le texte en
cours ; supprimer la page affichée laisse l'édition ouverte sur une page retirée. Seuls les changements de page
(`showPage`, `nextPage`) valident déjà, par `pageAboutToChange` (V6).

**Contrat.** Le contrôleur des pages valide l'édition en cours **avant toute commande de pages et avant de lire les
pages pour les copier**, par le même chemin qu'`announceEdit` (`ui/src/documentcontroller.cpp:322-326`) :
`editAboutToApply` (le texte se valide, en une étape, comme pour Annuler ; les outils abandonnent leur geste,
`ToolHost::cancelGesture`), puis dépôt forcé de ce qui resterait soulevé (`isLifting()` → `dropLifted(nullptr)`).
`goTo` et `click` ne changent pas : ils passent par `showPage`, qui émet `pageAboutToChange` (V6 : pas
d'`editAboutToApply` au changement de page, `pageChangeDuringEditCommitsFirst` reste vert). Une commande refusée
(supprimer ou couper toutes les pages) ne valide rien.

**Le patron évident est le mauvais.** Faire annoncer `DocumentController::execute()` : tous les outils l'appellent
pendant leurs gestes ; l'annonce ferait abandonner son geste à l'outil qui exécute (B1, L3-T12) et toucherait un fichier
chaud commun avec le lot 7. **Chemin le plus étroit retenu** : `ui/src/pagescontroller.cpp` seul. Il fait, avec l'API
publique du contrôleur de document (`editAboutToApply` est un signal, donc public en Qt 6 ; `isLifting` et `dropLifted`
sont publics, `documentcontroller.h:145-146`), exactement ce que fait `announceEdit` ; `documentcontroller.h` et
`documentcontroller.cpp` (L4-T26, L4-C1, lot 7) ne sont pas touchés. Référence : `DocumentController::announceEdit`
et `dropRemaining`, `ui/src/documentcontroller.cpp:322-340`.

**Fichiers.** `ui/src/pagescontroller.cpp`, `tests/tst_ui_text.cpp` (montage `Harness` de L4-T21 : contrôleur, `TextEdit`
QML, `TextEditing` ; le test y ajoute un `PagesController`).

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_text|tst_ui_pages'` (`100% tests passed, 0 tests failed out
of 2`) ; fonctions `duplicateDuringEditKeepsTheText`, `cutDuringEditKeepsTheText`, `deleteDuringEditCommitsFirst` en `PASS`, et toutes celles de
L4-T21, L4-T25 et L4-T18 inchangées et vertes ; preuve de tâche (N = 136 + n_B **inchangé**, `TESTS OK`, lignes
`tst_ui_text … Passed` et `tst_ui_pages … Passed`) ; banc sans écran.

**Interdits.** Ne pas toucher `DocumentController` (en-tête ni source). Ne pas émettre `pageAboutToChange` depuis le
contrôleur des pages. Ne pas modifier un test de L4-T18 ou de L4-T21 pour faire passer celui-ci.

**Terminé quand.** Les trois fonctions rouges sur le code d'avant (constaté), vertes après ; `TESTS OK` ; N inchangé ;
banc joint ; `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : tests rouges.** Dans `tests/tst_ui_text.cpp` : `#include <actcraft/ui/pagescontroller.h>` au bloc des
  inclusions, puis dans la classe de test :
  ```cpp
      // L4-C2 : une commande de pages lancée pendant l'édition valide d'abord le texte (comme Annuler). Dupliquer
      // la page affichée : la copie porte le texte frappé ; sans validation, le texte soulevé manquait à la copie.
      void duplicateDuringEditKeepsTheText()
      {
          Harness h;
          ui::PagesController pages;
          pages.setDocumentController(&h.controller);
          h.addText(QStringLiteral("un"), {200, 200, 120, 30});
          QVERIFY(h.editing.beginAt({210, 210}, false));
          h.replaceAllText(QStringLiteral("deux"));
          QVERIFY(h.controller.isLifting());
          pages.duplicateSelection(); // sélection = {0}, la page affichée
          QVERIFY(!h.editing.isActive());
          QVERIFY(!h.controller.isLifting());
          QCOMPARE(h.controller.pageCount(), 2);
          QCOMPARE(h.plainOf(0, 0), QStringLiteral("deux"));
          QCOMPARE(h.plainOf(1, 0), QStringLiteral("deux"));
      }

      // L4-C2 : couper la page affichée pendant l'édition : le presse-papiers reçoit la page avec le texte frappé ;
      // collée, elle le porte. cutSelection lit les pages lui-même (il ne passe pas par copySelection).
      void cutDuringEditKeepsTheText()
      {
          Harness h;
          h.controller.nextPage();
          h.controller.showPage(0);
          ui::PagesController pages;
          pages.setDocumentController(&h.controller);
          h.addText(QStringLiteral("un"), {200, 200, 120, 30});
          QVERIFY(h.editing.beginAt({210, 210}, false));
          h.replaceAllText(QStringLiteral("deux"));
          pages.cutSelection(); // sélection = {0}
          QVERIFY(!h.editing.isActive());
          QVERIFY(!h.controller.isLifting());
          QCOMPARE(h.controller.pageCount(), 1);
          QVERIFY(pages.canPaste());
          pages.paste(); // avant la page active : la page collée est la page 0
          QCOMPARE(h.controller.pageCount(), 2);
          QCOMPARE(h.plainOf(0, 0), QStringLiteral("deux"));
      }

      // L4-C2 : supprimer la page affichée pendant l'édition valide le texte (une étape), puis retire la page (une
      // étape) : deux Annuler rendent la page, puis son texte d'avant.
      void deleteDuringEditCommitsFirst()
      {
          Harness h;
          h.controller.nextPage();
          h.controller.showPage(0);
          ui::PagesController pages;
          pages.setDocumentController(&h.controller);
          h.addText(QStringLiteral("un"), {200, 200, 120, 30});
          QVERIFY(h.editing.beginAt({210, 210}, false));
          h.replaceAllText(QStringLiteral("deux"));
          pages.deleteSelection(); // sélection = {0}
          QVERIFY(!h.editing.isActive());
          QVERIFY(!h.controller.isLifting());
          QCOMPARE(h.controller.pageCount(), 1);
          h.controller.undo(); // la page revient, avec le texte validé
          QCOMPARE(h.controller.pageCount(), 2);
          QCOMPARE(h.plainOf(0, 0), QStringLiteral("deux"));
          h.controller.undo(); // l'édition est sa propre étape
          QCOMPARE(h.plainOf(0, 0), QStringLiteral("un"));
      }
  ```
  (Noms du montage : ceux de `tests/tst_ui_text.cpp` après L4-T21 et L4-T25 — `Harness`, `addText`, `replaceAllText`,
  `plainOf` ; les lire d'abord, adapter les noms sans changer ce qui est vérifié.) Commande ciblée → les trois en
  `FAIL!` ; noter la ligne qui échoue de chacune au rapport.
- [ ] **Étape 2 : implémenter** dans `ui/src/pagescontroller.cpp`, dans l'espace de noms anonyme du fichier (en créer
  un s'il manque) :
  ```cpp
  // L4-C2 : avant une commande de pages, ou une copie qui lit les pages, l'édition en cours est validée par le chemin
  // d'announceEdit (documentcontroller.cpp) : editAboutToApply (le texte se valide, les outils abandonnent leur
  // geste), puis dépôt forcé de ce qui resterait soulevé. execute() n'annonce rien (B1, L3-T12).
  void settleEditing(DocumentController& controller)
  {
      emit controller.editAboutToApply();
      if (controller.isLifting()) {
          controller.dropLifted(nullptr);
      }
  }
  ```
  puis `settleEditing(*m_controller);` en tête de `moveSelection`, `insertBlank`, `copySelection`, `cutSelection`,
  `paste`, `duplicateSelection` et `deleteSelection`, juste après leurs gardes (`!m_controller`, et refus « toutes les
  pages » pour `cutSelection` et `deleteSelection`). `cutSelection` (`ui/src/pagescontroller.cpp:230-238`) ne passe pas
  par `copySelection` : il appelle `selectedPages()` puis `deleteSelection()` ; l'appel se place après sa garde et
  **avant** `selectedPages()`, sinon le presse-papiers reçoit la page sans le texte en cours (le second appel, dans
  `deleteSelection`, ne trouve plus rien à valider). Rien dans `click`, `goTo`.
- [ ] **Étape 3 : vert**, `format.ps1`, commande ciblée, preuve de tâche, banc sans écran, `git add`, `NOMS OK`.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add ui/src/pagescontroller.cpp tests/tst_ui_text.cpp
  git -C <wt> commit -m "ui : le texte en cours d'édition est validé avant une commande de pages (L4-C2)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l4-c4"></a>
### Tâche L4-C4 — Correctif : un objet de type inconnu s'écrit avec son identifiant courant

**Origine.** Amendement 5, point 9 (trouvé par L4-T15). Le format natif écrit un `UnknownObject` (type d'une version plus
récente, L2-C8) tel qu'il l'a lu : son JSON d'origine, identifiant compris (`formats/src/nativejson.cpp:710-718`,
`objectJson`). Coller ou dupliquer une page (`InsertPagesCommand`, `withFreshIds`, L4-T15) donne à l'objet inconnu de la
copie un identifiant neuf **en mémoire** ; le fichier garde l'ancien. Après enregistrement puis relecture, deux objets
ont le même identifiant.

**Dépendances** (amendement 5) : L4-T6 et L4-T15 (mergées), L4-T24 (même fichier de test, en cours) ; L4-T27 la
suit. Partir de la base où L4-T24 est mergée.

**Contrat.** `objectJson` écrit l'identifiant **courant** de l'objet (`object.common.id`, par `idValue`) à la place de
celui du JSON d'origine ; toutes les autres clés de ce JSON restent telles quelles. Un objet inconnu dont l'identifiant
n'a pas changé s'écrit comme avant (mêmes octets : les clés d'un `QJsonObject` sont rangées, la valeur est la même).
Les références que le JSON d'un objet inconnu ferait à d'autres objets ne sont pas réécrites (on ne les connaît pas) :
limite écrite en commentaire et au rapport.

**Le patron évident est le mauvais.** Régénérer l'objet depuis le modèle (`writeCommon`) : il ajouterait les clés que le
fichier n'a pas et perdrait celles qu'on ne connaît pas (commentaire de `:710-711`). Seule la clé `id` change.

**Fichiers.** `formats/src/nativejson.cpp`, `tests/tst_formats_lot4.cpp`.

**Test et commande bornée.** Commande ciblée `-R 'tst_formats_lot4|tst_formats_nativejson|tst_formats_nativefile'`
(`100% tests passed, 0 tests failed out of 3`) ; fonction `unknownObjectWritesItsCurrentId` en `PASS`, celles de L4-T6
inchangées ; preuve de tâche (N = 136 + n_B **inchangé**, `TESTS OK`, ligne `tst_formats_lot4 … Passed`).

**Interdits.** Pas de QtGui dans `formats/`. Ne pas toucher `core/` : si `withFreshIds` ne renouvelait pas l'identifiant
d'un objet inconnu (assertion `fresh != 9` en échec), s'arrêter et le dire au rapport (amendement requis). Ne pas modifier
un test du lot 2 sur les objets inconnus.

**Terminé quand.** `unknownObjectWritesItsCurrentId` rouge sur le code d'avant (sur l'assertion de l'identifiant écrit),
vert après ; tests du format natif verts ; `TESTS OK` ; N inchangé ; `NOMS OK`.

**Étapes.**
- [ ] **Étape 1 : test rouge.** Dans `tests/tst_formats_lot4.cpp` : `#include <actcraft/core/pageedits.h>`,
  `#include <actcraft/core/undo.h>`, `#include <QJsonArray>` (s'ils manquent), puis :
  ```cpp
      // L4-C4 : un objet de type inconnu est réécrit depuis son JSON d'origine ; son identifiant courant y remplace
      // celui du JSON. Sans cela, la page collée (identifiants neufs, withFreshIds) garde dans le fichier
      // l'identifiant de sa source : doublon à la relecture.
      void unknownObjectWritesItsCurrentId()
      {
          QJsonObject root = formats::documentToJson(plain());
          QJsonArray pages = root.value(u"pages"_s).toArray();
          QJsonObject page = pages.at(0).toObject();
          QJsonArray objects = page.value(u"objects"_s).toArray();
          QJsonObject future = objects.at(0).toObject(); // clés communes écrites par cette version
          future.insert(u"type"_s, u"objet-a-venir"_s);
          future.insert(u"id"_s, 9);
          objects.append(future);
          page.insert(u"objects"_s, objects);
          pages.replace(0, page);
          root.insert(u"pages"_s, pages);
          ReadBack back = readBack(root);
          QVERIFY2(back.result.ok, qPrintable(report(back.result)));
          core::Document& d = back.document;
          QCOMPARE(d.page(0).objects.size(), std::size_t{3});
          QCOMPARE(d.page(0).objects[2].common.id, core::ObjectId{9});
          core::UndoStack stack;
          stack.push(d, std::make_unique<core::InsertPagesCommand>(1, std::vector<core::Page>{d.page(0)}));
          QCOMPARE(d.pageCount(), std::size_t{2});
          const core::ObjectId fresh = d.page(1).objects[2].common.id;
          QVERIFY(fresh != 0 && fresh != core::ObjectId{9});
          const QJsonObject written = formats::documentToJson(d);
          const QJsonObject copied = written.value(u"pages"_s).toArray().at(1).toObject()
                                         .value(u"objects"_s).toArray().at(2).toObject();
          QCOMPARE(copied.value(u"id"_s).toDouble(), static_cast<double>(fresh)); // rouge avant le correctif
          QCOMPARE(copied.value(u"type"_s).toString(), u"objet-a-venir"_s);      // le reste du JSON gardé
          ReadBack again = readBack(written);
          QVERIFY2(again.result.ok, qPrintable(report(again.result)));
          QCOMPARE(again.document.page(1).objects[2].common.id, fresh);
          QCOMPARE(again.document.page(0).objects[2].common.id, core::ObjectId{9});
      }
  ```
  (Aides `plain`, `readBack`, `report` : celles du fichier, L4-T6 ; noms réels de `pageCount` et `ObjectId` à vérifier
  dans `core/`.) Commande ciblée → `FAIL!` sur la comparaison de l'identifiant écrit.
- [ ] **Étape 2 : implémenter** dans `objectJson` (`formats/src/nativejson.cpp:712-718`), avant `return kept;` :
  ```cpp
          // L4-C4 : l'identifiant courant (neuf après un collage, withFreshIds), jamais celui du JSON d'origine. Les
          // références que ce JSON ferait à d'autres objets ne sont pas réécrites : on ne les connaît pas.
          kept.insert(u"id"_s, idValue(object.common.id));
  ```
- [ ] **Étape 3 : vert**, `format.ps1`, commande ciblée, preuve de tâche, `git add`, `NOMS OK`.
- [ ] **Étape 4 : commit.**
  ```powershell
  git -C <wt> add formats/src/nativejson.cpp tests/tst_formats_lot4.cpp
  git -C <wt> commit -m "formats : un objet de type inconnu s'écrit avec son identifiant courant (L4-C4)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

## Hypothèse ajoutée par 4b

- **H4-18** (niveau 2 ; numérotée H4-11 avant l'amendement 4, numéro repris par L4-T0) — Fond en image : offert seulement si le contrôleur sait ajouter une ressource (lot 7 fusionné
  par son API d'insertion, D-S8) ; sinon au lot 5 (insérer une image).

## Clôture du lot 4 (orchestrateur, hors tâches)

Jouée après V12 (L4-C2 comprise, amendement 5), dans le worktree du chantier, sur `feat/livraison-complete` à jour, avant la PR.

1. **Double relecture** (conformité, qualité) du diff du lot (4a et 4b), chaque tâche `concepteur-critique` relue
   après rejeu ; correctifs `L4-C<n>` (deux tours au plus).
2. **Simplification** : tâche `L4-S1` (diff de `tests/` vide).
3. **Version 0.5.0** : `project(VERSION 0.5.0)` dans `CMakeLists.txt` ; `CHANGELOG.md` section `## 0.5.0 — <date>`
   (formes, texte, navigateur de pages, menu de page, fond, grille, page allongée, zones) ; `scripts/notes-version.ps1`
   → `NOTES OK`. Si le lot 7 a fusionné avant, l'orchestrateur renumérote à la fusion.
4. **Suites** (jeton `tests`) : `scripts/test.ps1 -Preset release` puis `scripts/test.ps1`, formes bornées, N = 136 +
   n_B chacune, `TESTS OK` ; aucun test sauté hors les tests locaux du lot 2 rejoués avec leurs variables ; **aucun
   talon restant** : `git -C <wt> grep -n "Talon L4-T1" -- . ':(exclude)docs/'` → sortie vide ; **règle 4 des talons**
   du socle (amendement 1, défaut 14) : chaque test du lot lancé avec `-functions` ne liste pas `reserved` ; chaque
   scénario rempli sort au moins une ligne `TRAIT`, `POINTEUR` ou `TEXTE` ; `ctest --preset debug -V` ne contient
   aucun `SKIP` dont le message commence par « Talon ». Si le lot 7 a fusionné depuis L4-T0, n_B est relevé de
   nouveau (`ctest --preset debug -N`) et le nouveau N, écrit dans le fichier d'état, sert aux suites (amendement 2,
   N12).
5. **Captures à l'écran en D3D11** (jeton `ecran`, annoncé) : `captures.ps1 -Scenario tous -Rendu d3d11 -Sortie
   .tmp/captures-ecran` → `SCENARIO OK` chacun (dont `formes`, `grille`, `texte`, `navigateur-pages`,
   `page-allongee-zones`, `texte-vertical`, `formes-texte-pages-grille`, et ceux des lots 1 à 3 et 10), `CAPTURES OK` ;
   captures regardées et décrites (DA8).
6. **Banc** (DA7, jeton `ecran`, machine relevée) : non-régression **A/B** 0.4.0 → 0.5.0 → 0.4.0,
   `bench.ps1 -Rendu d3d11 -SansConstruire` dans des worktrees détachés construits en release, critère par critère
   sur `stroke`, `zoom`, `scroll`, `pauses`, `gommer`, `soulever` (figements et reprises compris) ; puis
   `bench.ps1 -Rendu tous` sur la 0.5.0 avec `forme-page-annotee` (`cache.refreshes = k`), `grille-dessus`
   (`cache.refreshes` = nombre de versements), `miniatures` (rendus dans la fenêtre mesurée, aucun pendant un contact,
   variante `pauses`) (S7.5) ; résumé
   commité dans `docs/mesures/<date>-lot4-outils2.md`. Hors limite : H4-7, H4-8, décision tuiles ou tâche corrective.
7. **Paquets** : `scripts/paquet.ps1` + `scripts/essai-installateur.ps1` (`PAQUET OK`, `INSTALLATEUR OK`,
   `--version : ActCraft 0.5.0`), `scripts/paquet-android.ps1` (`APK OK`, démon Gradle arrêté par PID) ;
   `essai-android.ps1` sur l'émulateur si disponible (`ANDROID OK`) : un doigt trace une forme, le navigateur de
   pages s'ouvre.
8. **Dépôt public** : `git add` puis `NOMS OK`, relancé après le commit ; `git grep -n "Private\|_p\.h" -- core
   formats actions render ui app tools input` vide ; commande des titres du corpus (section de L4-T0) lancée sur le
   worktree du chantier → sortie vide.
9. **Graphe** : `graphify update .`, commit de `graphify-out/`.
10. **PR du lot** (étage 4) et **porte de fin du lot 4** : release 0.5.0 proposée (publiée seulement sur go écrit) ;
    S7, S7.1 à S7.5, S15, S16 ; écarts 1 à 6 du plan 4a ; hypothèses H4-1 à H4-24 (amendement 5) ; décisions D1 à D19
    de L4-T0 et verdicts de L4-O1 et L4-O2 (dont H4-22, trait en cours sous la grille du dessus, et H4-24, Maj+clic) ;
    réalisation libre signalée (miniatures peintes par un élément C++) ; reports aux lots 5, 6, 7 et 11.
