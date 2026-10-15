# Livraison complète — lot 2, sous-lot 2a « Modèle, format natif et lecteur `.flipchart` » — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**But.** Première moitié du lot 2 du cadrage (`docs/specs/2026-10-14-livraison-complete-cadrage.md`, décision 5) :
tout ce qui se fait **sans interface**. Le modèle de document complet dans `core/` (DA2, décision 15), la pile
d'annulation (décision 16), le format natif `.actcraft` (DA3, décision 17), le lecteur `.flipchart` (DA4,
décision 18) et son oracle local (décision 19), les commandes de fichier déclarées (décision 20). Le sous-lot 2b
(`docs/plans/2026-10-15-livraison-complete-lot2b-PLAN.md`) branche ensuite le rendu, l'interface, les scénarios S4
et S5 et le banc.

**Pourquoi deux sous-lots.** Le lot 2 compte 23 tâches (plus de 20) : découpe en 2a (10 tâches : `core/`,
`actions/`, `formats/`, `third_party/`, outil d'oracle) puis 2b (13 tâches : `render/`, `ui/`, `app/`, `bench/`,
Android). 2b dépend de 2a entier pour ses contrats (modèle, pile, format, lecteur). **Une seule PR pour le lot 2**,
à la fin de 2b : 2a se termine par un point d'étape interne (section « Point d'étape 2a »), sans PR ni release,
car rien de 2a n'est visible dans l'application avant 2b.

**Architecture.** `core/` : `model.h` gagne les propriétés communes (`ObjectCommon` : identifiant stable, couche,
transformation affine, verrou, translucidité, restrictions, légende, actions), cinq types (`Line`, `Text`, `Media`,
`Group`, `Connector`), le fond et la grille de page ; `document.h` les ressources par SHA-256 et les identifiants ;
`undo.h` la pile ; `edits.h` les commandes du lot (ajout d'objet au sommet de sa couche, ajout de page) et la
liste des changements que 2b enverra au cache. `formats/` (QtCore seulement, vérifié par CMake) : `document.json`,
l'archive zip (miniz, `third_party/`), les E/S par URL, puis la chaîne `.flipchart` : conteneur Bamboo et index,
flux objet, correspondance vers le modèle, export canonique pour l'oracle. `bench/` reçoit l'outil
`actcraft-flipchart-dump` (outil de preuve, jamais livré).

**Pile.** C++20, Qt 6.12 (Core ; Gui et Test pour les tests), CMake + Ninja, MSVC 2022, miniz 3.0.2 (MIT, C),
Python 3.12 (oracle, dans l'ancien dépôt seulement).

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L2-T1 | `core` : modèle complet (propriétés communes, cinq types, fond, grille, ressources, identifiants) et empreintes recalculées | — | `core/include/actcraft/core/model.h`, `core/src/model.cpp`, `core/include/actcraft/core/document.h`, `core/src/document.cpp`, `core/src/generator.cpp`, `render/include/actcraft/render/objectrenderer.h`, `tests/tst_core_model.cpp`, `tests/tst_core_document.cpp`, `tests/tst_core_generator.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 44 tests, `tst_core_model`, `tst_core_document`, `tst_core_generator` Passed ; empreintes de `Page{}` et de `samplePage()` recoupées par le script Python de l'étape 7 ; banc hors écran (résultat) | S4, S5 | #tache-l2-t1 |
| L2-T2 | `core` : pile d'annulation, commandes du lot (objet au sommet de sa couche, page), changements | L2-T1 | `core/include/actcraft/core/undo.h`, `core/src/undo.cpp`, `core/include/actcraft/core/edits.h`, `core/src/edits.cpp`, `core/CMakeLists.txt`, `tests/core.cmake`, `tests/tst_core_undo.cpp`, `tests/tst_core_edits.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 46 tests, `tst_core_undo` et `tst_core_edits` Passed | S5 | #tache-l2-t2 |
| L2-T3 | `actions` : argument facultatif, commandes de fichier déclarées | — | `actions/include/actcraft/actions/commandregistry.h`, `actions/src/commandregistry.cpp`, `actions/data/commands.json`, `tests/tst_actions_registry.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 44 tests, `tst_actions_registry` Passed (dont `optionalArgumentMayBeAbsent`) | S5 | #tache-l2-t3 |
| L2-T4 | miniz vendu, `formats/` interdit de QtGui, fichiers de tests des formats | — | `third_party/miniz/miniz.c`, `third_party/miniz/miniz.h`, `third_party/miniz/LICENSE`, `third_party/miniz/README.md`, `third_party/miniz/CMakeLists.txt`, `third_party/.clang-format`, `CMakeLists.txt`, `formats/CMakeLists.txt`, `formats/native.cmake`, `formats/flipchart.cmake`, `scripts/format.ps1`, `.graphifyignore`, `THIRD_PARTY_NOTICES.md`, `tests/CMakeLists.txt`, `tests/formats.cmake`, `tests/flipchart.cmake`, `tests/tst_third_party_miniz.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 45 tests, `tst_third_party_miniz` Passed ; configuration en échec si `actcraft_formats` lie `Qt6::Gui` (rouge vu puis remis par `git checkout --`) | S5 | #tache-l2-t4 |
| L2-T5 | `formats` : `document.json` (écriture, lecture tolérante, version majeure refusée) | L2-T1, L2-T4 | `formats/include/actcraft/formats/nativejson.h`, `formats/src/nativejson.cpp`, `formats/native.cmake`, `tests/formats.cmake`, `tests/support/richdocument.h`, `tests/tst_formats_nativejson.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 46 tests, `tst_formats_nativejson` Passed (dont `roundTripKeepsTheFingerprint`) | S5 | #tache-l2-t5 |
| L2-T6 | `formats` : archive `.actcraft` (miniz) et E/S par URL (fichier local atomique, `content://`) | L2-T5 | `formats/include/actcraft/formats/nativefile.h`, `formats/src/nativefile.cpp`, `formats/include/actcraft/formats/urlio.h`, `formats/src/urlio.cpp`, `formats/native.cmake`, `tests/formats.cmake`, `tests/tst_formats_nativefile.cpp`, `tests/tst_formats_urlio.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 48 tests, `tst_formats_nativefile` et `tst_formats_urlio` Passed | S5 | #tache-l2-t6 |
| L2-T7 | `formats` : conteneur Bamboo et index « culm » ; écrivain de paperboards synthétiques | L2-T4 | `formats/include/actcraft/formats/bamboo.h`, `formats/src/bamboo.cpp`, `formats/flipchart.cmake`, `tests/flipchart.cmake`, `tests/support/flipchartwriter.h`, `tests/support/peakmemory.h`, `tests/tst_formats_bamboo.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 46 tests, `tst_formats_bamboo` Passed | S4 | #tache-l2-t7 |
| L2-T8 | `formats` : flux objet (variantes, sous-types, saut des propriétés illisibles) | L2-T7 | `formats/include/actcraft/formats/objectstream.h`, `formats/src/objectstream.cpp`, `formats/flipchart.cmake`, `tests/flipchart.cmake`, `tests/support/flipchartwriter.h`, `tests/tst_formats_objectstream.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 47 tests, `tst_formats_objectstream` Passed | S4 | #tache-l2-t8 |
| L2-T9 | `formats` : lecteur `.flipchart` vers le modèle (tables écrites à la main) | L2-T1, L2-T8 | `formats/include/actcraft/formats/flipchart.h`, `formats/src/flipchart.cpp`, `formats/src/flipchartids.h`, `formats/flipchart.cmake`, `tests/flipchart.cmake`, `tests/support/flipchartwriter.h`, `tests/tst_formats_flipchart.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 48 tests, `tst_formats_flipchart` Passed | S4 | #tache-l2-t9 |
| L2-T10 | Oracle : export canonique, `actcraft-flipchart-dump`, script Python de l'ancien dépôt, test local | L2-T9 | `formats/include/actcraft/formats/flipchartdump.h`, `formats/src/flipchartdump.cpp`, `formats/flipchart.cmake`, `bench/flipchart/main.cpp`, `bench/CMakeLists.txt`, `tests/flipchart.cmake`, `tests/tst_formats_dump.cpp`, `tests/tst_formats_oracle.cpp` ; hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\tools\oracle_flipchart.py` | isolé | concepteur-critique | TESTS OK, ≥ 50 tests, `tst_formats_dump` Passed ; localement `ORACLE OK : 17 paperboards, 124 pages, 0 écart` (`tst_formats_oracle`, `ACTCRAFT_SAMPLES_DIR` posé) | S4 | #tache-l2-t10 |

10 tâches. Le nombre minimal de tests de la colonne `preuve` compte les **44** tests de la fin du lot 1
(`docs/sessions/livraison-complete-etat.md`, « Clôture du lot 1 ») plus ceux des tâches dont la tâche dépend,
directement ou non, plus les siens ; une tâche lancée après d'autres en voit davantage : « ≥ » est la règle. Tests
ctest ajoutés : L2-T2 +2, L2-T4 +1, L2-T5 +1, L2-T6 +2, L2-T7 +1, L2-T8 +1, L2-T9 +1, L2-T10 +2, soit **55** à la
fin de 2a. `tst_formats_oracle` est sauté (`QSKIP`, signalé) sans `ACTCRAFT_SAMPLES_DIR` : c'est le cas en CI
(décision 19, DA4).

Aucune tâche de 2a n'ouvre de fenêtre. Seule L2-T1 (en-tête de `render/`) joint le JSON du banc hors écran.

## Écarts au cadrage (repris à la porte de fin du lot 2)

1. **Lot 2 découpé en 2a et 2b, une seule PR** (ce plan). Le cadrage annonce « ≈ 16 tâches » ; le relais
   `DocumentController` à défaire (revue L1-T9), le scénario de banc « changement de page » et la taille de page
   variable (constat L1-T8) en ajoutent.
2. **Deux familles de formes** (L2-T1). `Shape` garde ses champs du prototype (`fill`, `outline`, `outlineWidth`,
   `opacity`, `polygon`) pour Rectangle, Ellipse et Polygone : la page du banc et ses moteurs ne changent pas. Les
   formes lues (`ShapeKind::Path`) portent une plume (`Outline`) et un remplissage (`Fill`) complets. Le lot 4
   (outils de formes) décidera s'il unifie.
3. **Page du banc inchangée** (L2-T1). Ses 300 objets reçoivent les identifiants 1 à 300 et restent sur la couche
   intermédiaire, dans le même ordre ; seules les empreintes figées sont recalculées (DA2 : « empreinte
   recalculée »). L'empreinte du générateur n'est pas recoupée hors C++ cette fois (le générateur ne change pas,
   seuls des champs à valeur par défaut s'ajoutent ; la CI macOS la recoupe entre compilateurs) ; celles de
   `Page{}` et de la page d'exemple le sont.
4. **Argument d'URL facultatif** (L2-T3). `file.open` et `file.save-as` prennent une URL (S5 : `file.save-as
   <chemin>`, `file.open <chemin>`) ; déclenchées sans argument (menu, raccourci), elles ouvrent le dialogue.
   Le registre gagne la clé `"optional": true`.
5. **« Nouveau ▸ » à plat** (L2-T3). La disposition n'a pas de sous-menus : six commandes, `file.new` (Ctrl+N,
   1024 × 768) et `file.new-screen`, `file.new-1024x768`, `file.new-1152x864`, `file.new-1280x1024`,
   `file.new-custom` (les cinq tailles de F-700). Raccourci ajouté par nous : Ctrl+Maj+S pour `file.save-as`.
6. **Export canonique de l'oracle** (L2-T10). Clés et règles fixées par ce plan : les objets de type non lu
   (MathML, texte vertical, inconnus) sont exclus des deux côtés ; les actions sont identifiées par le numéro de
   type du `.flipchart` ; les rectangles englobants des annotations et des lignes ne sont pas comparés (le modèle
   ne les garde pas).
7. **Translucidité sur 0-255** (L2-T1) comme le `.flipchart` (opacité = 1 − t / 255) ; le navigateur de propriétés
   (lot 6) l'affichera en pour cent.
8. **Conteneur `.flipchart` décompressé par miniz, pas par `qUncompress`** (L2-T7 ; la décision 18 dit
   « `qUncompress` pour le conteneur »). L'en-tête de taille de `qCompress` est écrit par le fichier et `qUncompress`
   le dépasse (bombe zlib mesurée en relecture : en-tête de 16 octets, 597 Kio → 600 Mio) ; miniz décompresse dans un
   tampon de la taille de l'entrée, bornée à 512 Mio, et `formats/` le lie déjà (L2-T4).
9. **Extensions de ressources contraintes** (L2-T1) : `[a-z0-9]{1,8}`, sinon `bin` ; clés lues dans `document.json`
   vérifiées (`[0-9a-f]{64}`) avant toute lecture de l'archive (L2-T5).

## Couverture des scénarios

| Scénario | Tâches de 2a | Joué |
|---|---|---|
| S4 — ouvrir un `.flipchart` | L2-T1 (modèle), L2-T7, L2-T8, L2-T9 (lecteur), L2-T10 (oracle sur les 17 paperboards) | oracle local au point d'étape 2a ; ouverture et miniatures en 2b (L2-T21) |
| S5 — aller-retour natif | L2-T1, L2-T2 (modèle, pile), L2-T3 (commandes à URL), L2-T4, L2-T5, L2-T6 (format, E/S) | aller-retour par les tests de 2a ; scénario d'interface et Android en 2b (L2-T20, L2-T22) |

## Vagues proposées (parallélisme 2 au plus)

V1 : L2-T1 ‖ L2-T4 · V2 : L2-T2 ‖ L2-T7 · V3 : L2-T5 ‖ L2-T8 · V4 : L2-T6 ‖ L2-T9 · V5 : L2-T10 ‖ L2-T3.

Deux tâches d'une même vague ne partagent aucun fichier (vérifié sur la colonne `fichiers touchés`). Fichiers
partagés tenus par les dépendances : `formats/native.cmake` et `tests/formats.cmake` (L2-T4 → L2-T5 → L2-T6) ;
`formats/flipchart.cmake`, `tests/flipchart.cmake` et `tests/support/flipchartwriter.h` (L2-T4 → L2-T7 → L2-T8 →
L2-T9 → L2-T10) ; `core/CMakeLists.txt` et `tests/core.cmake` (L2-T2 seule dans 2a). La séparation des fichiers
CMake des formats (`native.cmake`, `flipchart.cmake`), posée par L2-T4, est ce qui permet de mener le format natif
et le lecteur `.flipchart` en parallèle.

## Carte des fichiers

| Fichier | Responsabilité | Tâche |
|---|---|---|
| `core/include/actcraft/core/model.h` | types d'objets, `ObjectCommon`, `Transform`, `Outline`, `Fill`, `Page` (fond, grille), empreinte de page | L2-T1 |
| `core/include/actcraft/core/document.h` | `Document` (pages, ressources SHA-256, identifiants, titre), empreinte du document | L2-T1 |
| `core/include/actcraft/core/undo.h` | `Change`, `Changes`, `UndoCommand`, `UndoStack` (500 étapes, état propre, fusion) | L2-T2 |
| `core/include/actcraft/core/edits.h` | `insertAtTopOfLayer`, `indexOf`, `depthInLayer`, `isLayerOrdered`, `boundsOnPage`, `AddObjectCommand`, `AddPageCommand` | L2-T2 |
| `actions/data/commands.json` | commandes `file.*` du lot 2 | L2-T3 |
| `third_party/miniz/` | miniz 3.0.2 amalgamé, licence, provenance | L2-T4 |
| `formats/native.cmake`, `formats/flipchart.cmake` | listes de sources des deux familles de formats | L2-T4 (puis T5-T10) |
| `formats/include/actcraft/formats/nativejson.h` | `documentToJson`, `documentFromJson`, `kNativeFormat` | L2-T5 |
| `formats/include/actcraft/formats/nativefile.h` | `encodeActcraft`, `decodeActcraft`, `archiveEntryNames`, `looksLikeActcraft` | L2-T6 |
| `formats/include/actcraft/formats/urlio.h` | `readUrl`, `writeUrl`, `urlFromArgument` | L2-T6 |
| `formats/include/actcraft/formats/bamboo.h` | `BambooContainer`, `IndexNode`, `parseIndex` | L2-T7 |
| `formats/include/actcraft/formats/objectstream.h` | `Value`, `StreamObject`, `parseObjectStream`, accesseurs `as*` | L2-T8 |
| `formats/src/flipchartids.h` | identifiants de types et de propriétés lus, écrits à la main | L2-T9 |
| `formats/include/actcraft/formats/flipchart.h` | `readFlipchart`, `looksLikeFlipchart`, `FlipchartReadResult` | L2-T9 |
| `formats/include/actcraft/formats/flipchartdump.h` | `canonicalDump`, `compareCanonical` (`actcraft-flipchart-dump/1`) | L2-T10 |
| `bench/flipchart/main.cpp` | exécutable `actcraft-flipchart-dump` | L2-T10 |
| `tests/support/richdocument.h` | document de test qui remplit tous les champs du modèle | L2-T5 |
| `tests/support/flipchartwriter.h` | écrivain de paperboards synthétiques (`fctest::`) | L2-T7, L2-T8, L2-T9 |
| `REA - Activinspire\tools\oracle_flipchart.py` | oracle Python (hors dépôt public) | L2-T10 |

## Contrats partagés entre tâches (noms figés)

Une tâche ne renomme aucun de ces noms. Ceux du lot 1 (`docs/plans/2026-10-14-livraison-complete-lot1-PLAN.md`,
« Contrats partagés ») et du prototype restent figés ; on ajoute, on ne renomme pas.

- `actcraft::core` (L2-T1) : `ObjectId`, `Transform` (`m11`, `m12`, `m21`, `m22`, `dx`, `dy`, `map`,
  `isIdentity`), `compose`, `translation`, `scaling`, `rotation`, `Layer` (`Background`, `Bottom`, `Middle`, `Top`),
  `PenStyle`, `PenCap`, `PenJoin`, `Outline`, `FillKind`, `GradientKind`, `Fill`, `solidFill`, `Restrictions`,
  `ActionKind`, `Action`, `ObjectCommon`, `ObjectKind` (8 valeurs, ordre du variant), `kObjectKindCount`,
  `ShapeKind::Path`, `StrokeKind`, `Line`, `TextSpan`, `Text`, `Media`, `Group`, `Connector`, `ObjectVariant`,
  `PageObject` (dérive du variant, membre `common`), `body`, `GridPattern`, `GridLine`, `GridLevel`, `Grid`,
  `Background`, `Page` (`width`, `height`, `objects`, `background`, `grid`, `name`, `notes`), `Resource`,
  `Document` (`pageCount`, `page`, `appendPage`, `insertPage`, `takePage`, `allocateId`, `nextId`,
  `refreshNextId`, `reserveIdsBelow`, `addResource`, `resource`, `resources`, `title`, `setTitle`),
  `sha256Hex`, `hasUniqueIds`, `fingerprint(const Page&)`, `fingerprint(const Document&)`.
- `actcraft::core` (L2-T2) : `Change` (`Kind` : `ObjectInserted`, `ObjectRemoved`, `ObjectChanged`,
  `PageInserted`, `PageRemoved`, `PageChanged` ; champs `kind`, `page`, `object`, `area`), `Changes`,
  `UndoCommand` (`redo`, `undo`, `mergeId`, `mergeWith`), `UndoStack` (`kDefaultLimit` = 500, `push`, `undo`,
  `redo`, `canUndo`, `canRedo`, `count`, `index`, `limit`, `isClean`, `setClean`, `clear`), `insertAtTopOfLayer`,
  `indexOf`, `depthInLayer`, `isLayerOrdered`, `boundsOnPage`, `AddObjectCommand` (`id`), `AddPageCommand`.
- `actcraft::actions` (L2-T3) : `CommandInfo::argumentOptional` ; clé JSON `"optional"`. Identifiants :
  `file.new`, `file.new-screen`, `file.new-1024x768`, `file.new-1152x864`, `file.new-1280x1024`,
  `file.new-custom`, `file.open` (URL facultative), `file.save`, `file.save-as` (URL facultative), `file.close`.
- `actcraft::formats` (L2-T5, L2-T6) : `kNativeFormat` (`"actcraft-document/1"`), `kNativeMajorVersion`,
  `documentToJson`, `ResourceLoader`, `JsonReadResult`, `documentFromJson`, `encodeActcraft`, `DecodeResult`,
  `decodeActcraft`, `archiveEntryNames`, `looksLikeActcraft`, `kMaxArchiveEntry`, `kMaxArchiveTotal`,
  `UrlReadResult`, `readUrl`, `readBounded`,
  `writeUrl`, `urlFromArgument`. Noms d'entrées : `document.json`, `resources/<sha256>.<ext>`,
  `thumbnails/page-<n>.png` (n à partir de 1).
- `actcraft::formats::flipchart` (L2-T7 à L2-T10) : `BambooEntry`, `BambooContainer` (`parse`, `names`,
  `contains`, `read`, `version`), `kMaxEntrySize`, `IndexNode` (`storageName`), `parseIndex`, `SizeF`, `Color`,
  `Pointer`, `ResourceItem`, `RawValue`, `StreamObject` (`find`), `ObjectPtr`, `Value`, `ValueList`, `Polyline`,
  `parseObjectStream`, `asNumber`, `asBool`, `asString`, `asPoint`, `asSize`, `asRect`, `asColor`,
  `asPointer`, `asObject`, `asObjects`, `asPolyline`, `asPolylines`, `asResourceItems` ; `FlipchartReadResult`,
  `readFlipchart`, `looksLikeFlipchart` ; `kDumpFormat` (`"actcraft-flipchart-dump/1"`), `canonicalDump`,
  `compareCanonical`.
- Outils : `actcraft-flipchart-dump <fichier> [--sortie <json>]`, dernière ligne `DUMP OK : <n> pages` ou
  `DUMP EN ECHEC : <raison>` (code 1). Script `oracle_flipchart.py` : dernière ligne `ORACLE OK : <n> paperboards,
  <p> pages`. Test `tst_formats_oracle` : ligne `ORACLE OK : <n> paperboards, <p> pages, 0 écart`.
- Écrivain de test (`tests/support/flipchartwriter.h`, espace `fctest`) : `Bytes`, `Entry`, `container`, `Node`,
  `index`, `Obj`, `Res`, `pngBytes`, `sampleFlipchart`.
- Variables d'environnement : `ACTCRAFT_SAMPLES_DIR` (dossier `samples/` de l'ancien dépôt, lecture seule),
  `ACTCRAFT_ORACLE_DIR` (JSON de l'oracle ; défaut `<samples>/../build/oracle-flipchart`).

## Règles communes à toutes les tâches (rappelées dans chaque section)

- Worktree de la tâche : `.worktrees/livraison-complete--L2-T<n>` (profil). **Toute commande PowerShell se lance
  en processus enfant** : `pwsh -NoProfile -Command "Set-Location '<worktree>'; …"` — jamais de `Set-Location`
  dans la session (son dossier courant resterait verrouillé et `git worktree remove` échouerait : piège du lot 1).
  `git -C <worktree>` pour git. Toujours `pwsh -NoProfile`, jamais `powershell.exe`.
- Avant de relire des fichiers : `graphify query "…"` dans le worktree (carte du code, `graphify-out/`) ; pour la
  référence de comportement, `graphify query "…"` lancé **dans** `C:\Users\JeromeDenis\REA - Activinspire`.
- Premier passage dans un worktree neuf :
  `pwsh -NoProfile -Command "Set-Location '<worktree>'; pwsh -NoProfile -File ./scripts/build.ps1"`.
- **Commande ciblée** (test rouge, test vert d'un seul test) — jamais `ctest` ni `tst_*.exe` sans `dev-env.ps1`
  (boîte « DLL introuvable » bloquante) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<worktree>'; New-Item -ItemType Directory -Force .tmp | Out-Null; . ./scripts/dev-env.ps1; cmake --build --preset debug *> .tmp/build.txt; ctest --preset debug -R '<motif>' --output-on-failure *> .tmp/ctest.txt; Select-String -Path .tmp/build.txt,.tmp/ctest.txt -Pattern 'error|tests passed|tests failed|No tests were found' | Select-Object -Last 4"
  ```
- **Preuve de tâche** (forme bornée du profil), puis le nom du test :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<worktree>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2; Select-String -Path .tmp/test.txt -Pattern '<nom_du_test> .*Passed' | Select-Object -Last 1"
  ```
  Attendu : `100% tests passed, 0 tests failed out of N` avec N ≥ la valeur de la table, puis `TESTS OK`, puis la
  ligne `Passed` du test nommé.
- **Banc sans écran** (tâches qui touchent `render/`, `input/`, `tools/` ou `ui/` ; profil « Banc de fluidité »,
  décision 35 : résultat, pas une porte) — toujours `-Taille 1280x800` hors écran (piège du lot 1) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<worktree>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu logiciel -Scenario court -Taille 1280x800 -HorsEcran > .tmp/bench.txt 2>&1; Select-String -Path .tmp/bench.txt -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE' | Select-Object -Last 1"
  pwsh -NoProfile -Command "Set-Location '<worktree>'; `$j = Get-ChildItem bench/results/*.json | Sort-Object LastWriteTime | Select-Object -Last 1; Write-Output (`$j.Name + ' content_check.ok = ' + (Get-Content -Raw `$j | ConvertFrom-Json).content_check.ok)"
  ```
  Attendu : `BUDGET TENU` ou `BUDGET DEPASSE` (jamais `MESURE INVALIDE`), puis `content_check.ok = True` ; le nom du
  JSON est joint au rapport.
- **Rouge prouvé par mutation** : quand le rouge n'a pas pu être vu avant le code, muter le code, constater le
  rouge, puis **remettre le fichier par `git -C <worktree> checkout -- <fichier>`** (jamais par `Copy-Item` d'une
  copie : une copie périmée a déjà écrasé un correctif au lot 1).
- `pwsh -NoProfile -File ./scripts/format.ps1` avant la preuve (le contrôle de format fait partie de `TESTS OK`).
- Clang (CI macOS) active `-Wshadow`, `-Wconversion`, `-Wsign-conversion` et `-Wmissing-field-initializers` : indices
  de `std::vector` en `std::size_t`, de `QList`/`QByteArray` en `qsizetype`, conversions explicites
  (`static_cast`), **jamais d'initialisation d'agrégat partielle par position** (`Fill{FillKind::Solid, c}`
  échoue sous Clang) : affecter les champs un à un ou passer par une fonction (`solidFill`). MSVC `/W4 /WX` ne voit
  pas tout cela : y penser.
- `std::visit` sur `core::PageObject` : ne pas l'employer directement (classe dérivée du variant) ; passer par
  `core::body(object)` ou par `std::get_if<T>(&object)`, qui marchent sur les deux compilateurs.
- **Aucune API privée de Qt** dans le code livré (`core/`, `formats/`, `actions/`, `render/`, `ui/`, `app/`) : ni
  `QZipReader`, ni en-tête `*_p.h`, ni cible `*Private`
  (`git -C <worktree> grep -n "Private\|_p\.h" -- core formats actions render ui app` vide). Les outils de preuve
  (`bench/`) en ont le droit, comme au lot 1.
- **Propriété intellectuelle (dépôt public)** : rien de Promethean n'entre ici. Les identifiants du `.flipchart`
  sont écrits à la main, pour les seuls types et propriétés lus, d'après `docs/re/format-flipchart.md` et la
  consultation de `app/actcraft/io/_schema.py` de l'ancien dépôt (consulté, jamais copié : aucun nom de classe
  ou de propriété d'Inspire dans le code, seulement nos noms et les numéros). **`samples/` n'est jamais modifié,
  jamais copié dans le dépôt, jamais commité** : un test qui le lit le lit en place, par `ACTCRAFT_SAMPLES_DIR` ;
  ses sorties vont sous `.tmp/` ou `build/`. Les paperboards des tests sont écrits par notre écrivain
  (`tests/support/flipchartwriter.h`).
- Textes en français, écrits par nous ; rien de Promethean (icônes, libellés, noms) ; termes propres à Promethean
  remplacés (N1-12). Sorties d'essai sous `.tmp/` ou `build/`, jamais commitées.
- Commit en français, chemins explicites (`git -C <worktree> add <fichiers>`), jamais `git add -A` ni
  `git -C <worktree> add .` (le hook ne voit pas cette forme : règle écrite), terminé par
  `Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>`. Pas de push.
- Ce qu'on lance, on l'arrête **par son PID** (`Stop-Process -Id`), jamais par nom (hook `interdits`).
- Hors périmètre : consigné dans le rapport, jamais corrigé. Un test existant ne se modifie que pour la raison
  écrite dans la section (comportement voulu qui change), jamais pour le faire passer.

---

## Tâches

<a id="tache-l2-t1"></a>
### Tâche L2-T1 — `core` : modèle complet (propriétés communes, cinq types, fond, grille, ressources, identifiants) et empreintes recalculées

**Objectif.** Le modèle de DA2 et de la décision 15, sans QtGui : chaque objet porte des propriétés communes
(`ObjectCommon` : identifiant stable, nom, couche parmi quatre, visible, verrouillé, translucidité 0-255,
restrictions, transformation affine objet → page, légende, actions) ; cinq types s'ajoutent (`Line`, `Text` en HTML
avec ses runs, `Media`, `Group` par références, `Connector`) ; `Shape` gagne la forme à chemins (`ShapeKind::Path`,
plume et remplissage complets) ; `Stroke` son genre (stylo, surligneur, encre révélatrice) et ses bouts ;
`ImageObject` sa ressource ; la page son fond (couleur, dégradé, image), sa grille à niveaux, son nom et ses notes ;
le document ses ressources par SHA-256, ses identifiants et son titre. Les empreintes couvrent tout. La page du banc
ne change pas d'aspect : ses 300 objets gardent leur ordre, sur la couche intermédiaire, avec les identifiants 1 à
300.

Patron à **ne pas** reprendre : un `struct PageObject { ObjectCommon common; ObjectVariant body; }` obligerait à
réécrire chaque `std::get_if<core::Shape>(&page.objects[i])` des moteurs, du banc et des tests ; et des propriétés
communes copiées dans chaque type obligeraient à un `std::visit` pour lire la couche. Ici `PageObject` **dérive** du
variant : `std::get_if`, `std::get`, `index()` et `kindOf` marchent tels quels, et `object.common.layer` se lit
directement.

**Fichiers.**
- Modifier : `core/include/actcraft/core/model.h`, `core/src/model.cpp`, `core/include/actcraft/core/document.h`,
  `core/src/document.cpp`, `core/src/generator.cpp` (identifiants 1 à 300),
  `render/include/actcraft/render/objectrenderer.h` (tableau du registre à `core::kObjectKindCount` cases)
- Tests : `tests/tst_core_model.cpp`, `tests/tst_core_document.cpp`, `tests/tst_core_generator.cpp`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_core_(model|document|generator)` → `error`
(`ObjectCommon`, `Layer`, `Document::addResource` inconnus). Vert : forme bornée, `out of N` avec N ≥ 44, `TESTS OK`,
puis `tst_core_model .*Passed`, `tst_core_document .*Passed`, `tst_core_generator .*Passed`. Banc hors écran
(l'en-tête `objectrenderer.h` est dans `render/`) : `BUDGET TENU` ou `BUDGET DEPASSE`, `content_check.ok = True`.

**Interdits.** Pas de QtGui dans `core/` (CMake le vérifie). Ne pas changer l'ordre ni le contenu des objets du
générateur (seulement leurs identifiants). Ne renommer aucun champ existant de `Shape`, `ImageObject`, `Stroke`,
`Page`. Pas de `std::visit` direct sur `PageObject`. Les trois valeurs figées changent pour une seule raison (DA2,
champs ajoutés) : la nouvelle valeur de `Page{}` et de `samplePage()` est recoupée par le script Python de
l'étape 7 avant d'être écrite.

**Terminé quand.** Preuve verte, valeurs figées recoupées (sortie du script dans le rapport), JSON du banc joint,
commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests du modèle.** Remplacer `tests/tst_core_model.cpp` par :

```cpp
#include <actcraft/core/clock.h>
#include <actcraft/core/model.h>

#include <QTest>

#include <cstdint>
#include <variant>

using namespace actcraft::core;

class TstCoreModel : public QObject
{
    Q_OBJECT

private:
    static Page samplePage()
    {
        Page page;
        Shape shape;
        shape.kind = ShapeKind::Polygon;
        shape.bounds = {10, 20, 30, 40};
        shape.polygon = {{10, 20}, {40, 20}, {25, 60}};
        shape.fill = 0xFF102030u;
        page.objects.emplace_back(shape);
        ImageObject image;
        image.bounds = {100, 100, 64, 48};
        image.pattern = ImagePattern::Checker;
        page.objects.emplace_back(image);
        Stroke stroke;
        stroke.points = {{1, 2, 0.25f}, {3, 4, 0.5f}};
        page.objects.emplace_back(stroke);
        return page;
    }

    // Un objet de chaque type ajouté par le lot 2, champs hors défaut.
    static Page richPage()
    {
        Page page = samplePage();
        Line line;
        line.points = {{0, 0}, {10, 5}};
        line.pen.width = 3;
        page.objects.emplace_back(line);
        Text text;
        text.bounds = {0, 0, 90, 36};
        text.html = "<p>Bonjour</p>";
        TextSpan run;
        run.text = "Bonjour";
        run.fontFamily = "Arial";
        run.pointSize = 24;
        text.runs.push_back(run);
        page.objects.emplace_back(text);
        Media media;
        media.bounds = {0, 0, 64, 48};
        media.media = "abc";
        page.objects.emplace_back(media);
        Group group;
        group.members = {1, 2};
        page.objects.emplace_back(group);
        Connector connector;
        connector.secondEnd = {100, 100};
        page.objects.emplace_back(connector);
        page.grid.levels.push_back(GridLevel{});
        return page;
    }

private slots:
    void kindOfFollowsVariantOrder()
    {
        const Page page = richPage();
        QCOMPARE(kindOf(page.objects[0]), ObjectKind::Shape);
        QCOMPARE(kindOf(page.objects[1]), ObjectKind::Image);
        QCOMPARE(kindOf(page.objects[2]), ObjectKind::Stroke);
        QCOMPARE(kindOf(page.objects[3]), ObjectKind::Line);
        QCOMPARE(kindOf(page.objects[4]), ObjectKind::Text);
        QCOMPARE(kindOf(page.objects[5]), ObjectKind::Media);
        QCOMPARE(kindOf(page.objects[6]), ObjectKind::Group);
        QCOMPARE(kindOf(page.objects[7]), ObjectKind::Connector);
        QCOMPARE(std::variant_size_v<ObjectVariant>, kObjectKindCount);
    }

    void pageObjectKeepsVariantAccessAndCommonProperties()
    {
        Page page = samplePage();
        PageObject& object = page.objects[2];
        QVERIFY(std::get_if<Stroke>(&object) != nullptr);
        QVERIFY(std::get_if<Shape>(&object) == nullptr);
        QCOMPARE(object.common.layer, Layer::Middle);
        QCOMPARE(object.common.id, ObjectId{0});
        QVERIFY(object.common.transform.isIdentity());
        object.common.layer = Layer::Top;
        const PageObject copy = object;
        QCOMPARE(copy.common.layer, Layer::Top);
        QCOMPARE(std::get<Stroke>(copy).points.size(), std::size_t{2});
        QCOMPARE(body(copy).index(), std::size_t{2});
    }

    void defaultPageIs1024By768OnWhite()
    {
        const Page page;
        QCOMPARE(page.width, 1024.0);
        QCOMPARE(page.height, 768.0);
        QVERIFY(page.objects.empty());
        QCOMPARE(page.background.fill.kind, FillKind::Solid);
        QCOMPARE(page.background.fill.color, Argb{0xFFFFFFFFu});
        QVERIFY(page.background.image.empty());
        QVERIFY(!page.grid.visible);
        QVERIFY(page.grid.levels.empty());
    }

    void transformsComposeLikeQTransform()
    {
        // Mise à l'échelle puis rotation de 90° (sens horaire, y vers le bas) puis translation.
        const Transform t = compose(compose(scaling(2, 1), rotation(90)), translation(100, 100));
        const PointF p = t.map({100, 0});
        QCOMPARE(p.x, 100.0);
        QCOMPARE(p.y, 300.0);
        const Transform r = rotation(90);
        QCOMPARE(r.m11, 0.0);
        QCOMPARE(r.m12, 1.0);
        QCOMPARE(r.m21, -1.0);
        QCOMPARE(r.m22, 0.0);
        QVERIFY(rotation(360).isIdentity());
        QCOMPARE(rotation(-90).m12, -1.0);
        const PointF q = rotation(30).map({1, 0});
        QVERIFY(qAbs(q.x - 0.8660254037844386) < 1e-12);
        QVERIFY(qAbs(q.y - 0.5) < 1e-12);
    }

    void solidFillSetsKindAndColor()
    {
        const Fill f = solidFill(0xFF00FF00u);
        QCOMPARE(f.kind, FillKind::Solid);
        QCOMPARE(f.color, Argb{0xFF00FF00u});
    }

    void fingerprintIsStable() { QCOMPARE(fingerprint(richPage()), fingerprint(richPage())); }

    void fingerprintSeesEveryField()
    {
        const std::uint64_t reference = fingerprint(richPage());
        const auto differs = [reference](auto mutate) {
            Page p = richPage();
            mutate(p);
            return fingerprint(p) != reference;
        };
        QVERIFY(differs([](Page& p) { std::get<Stroke>(p.objects[2]).points[1].pressure = 0.5001f; }));
        QVERIFY(differs([](Page& p) { std::get<Shape>(p.objects[0]).polygon[2].y = 61; }));
        QVERIFY(differs([](Page& p) { std::get<ImageObject>(p.objects[1]).pattern = ImagePattern::Rings; }));
        QVERIFY(differs([](Page& p) { std::swap(p.objects[0], p.objects[2]); }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.id = 7; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.name = "Forme1"; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.layer = Layer::Bottom; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.visible = false; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.locked = true; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.translucency = 1; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.restrictions.resizable = false; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.transform.dx = 0.5; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.label = "Légende"; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.actions.push_back(Action{}); }));
        QVERIFY(differs([](Page& p) { std::get<Shape>(p.objects[0]).paths.push_back({{1, 1}}); }));
        QVERIFY(differs([](Page& p) { std::get<Shape>(p.objects[0]).brush.gradient = GradientKind::Radial; }));
        QVERIFY(differs([](Page& p) { std::get<Shape>(p.objects[0]).pen.join = PenJoin::Bevel; }));
        QVERIFY(differs([](Page& p) { std::get<ImageObject>(p.objects[1]).resource = "x"; }));
        QVERIFY(differs([](Page& p) { std::get<Stroke>(p.objects[2]).kind = StrokeKind::Highlighter; }));
        QVERIFY(differs([](Page& p) { std::get<Line>(p.objects[3]).pen.style = PenStyle::Dash; }));
        QVERIFY(differs([](Page& p) { std::get<Text>(p.objects[4]).html = "<p>Bonsoir</p>"; }));
        QVERIFY(differs([](Page& p) { std::get<Text>(p.objects[4]).runs[0].weight = 700; }));
        QVERIFY(differs([](Page& p) { std::get<Media>(p.objects[5]).loop = true; }));
        QVERIFY(differs([](Page& p) { std::get<Group>(p.objects[6]).members.push_back(3); }));
        QVERIFY(differs([](Page& p) { std::get<Connector>(p.objects[7]).objectA = 9; }));
        QVERIFY(differs([](Page& p) { p.background.fill.color2 = 0xFF000000u; }));
        QVERIFY(differs([](Page& p) { p.background.image = "cle"; }));
        QVERIFY(differs([](Page& p) { p.grid.visible = true; }));
        QVERIFY(differs([](Page& p) { p.grid.levels[0].spacingY = 31; }));
        QVERIFY(differs([](Page& p) { p.name = "Page un"; }));
        QVERIFY(differs([](Page& p) { p.notes = "notes"; }));
    }

    // Valeurs recoupées hors C++ par le script Python de L2-T1 (étape 7), qui hache les octets petit-boutistes
    // des champs dans l'ordre de model.cpp : FNV-1a 64 bits, base 0xCBF29CE484222325, multiplicateur
    // 0x100000001B3. Valeurs du lot 1 avant l'ajout des champs du lot 2 : 0xD93C4EE35CD928AD et
    // 0x21FF926A276797BC.
    void fingerprintMatchesIndependentReference()
    {
        QCOMPARE(fingerprint(Page{}), std::uint64_t{0xFFFFFFFFFFFFFFFFull}); // remplacée à l'étape 7
        QCOMPARE(fingerprint(samplePage()), std::uint64_t{0xFFFFFFFFFFFFFFFFull}); // remplacée à l'étape 7
    }

    void strokeWidthFollowsPressure()
    {
        QCOMPARE(strokeWidthAt(3.0, 0.5f), 3.0);
        QCOMPARE(strokeWidthAt(3.0, 0.0f), 1.5);
        QCOMPARE(strokeWidthAt(3.0, 1.0f), 4.5);
    }

    void rectContainsAndCenter()
    {
        const RectF r{10, 20, 30, 40};
        QVERIFY(r.contains({10, 20}));
        QVERIFY(r.contains({40, 60}));
        QVERIFY(!r.contains({41, 60}));
        QCOMPARE(r.center().x, 25.0);
        QCOMPARE(r.center().y, 40.0);
    }

    void clockIsMonotonic()
    {
        const std::int64_t a = monotonicNowNs();
        const std::int64_t b = monotonicNowNs();
        QVERIFY(b >= a);
        QVERIFY(a > 0);
    }
};

QTEST_GUILESS_MAIN(TstCoreModel)
#include "tst_core_model.moc"
```

Les trois dernières fonctions (`strokeWidthFollowsPressure`, `rectContainsAndCenter`, `clockIsMonotonic`) sont
celles du lot 1, inchangées. Les deux valeurs `0xFFFF…` sont des sentinelles volontairement fausses : le test reste
rouge jusqu'à l'étape 7.

- [ ] **Étape 2 : compléter les tests du document.** Ajouter à la classe de `tests/tst_core_document.cpp` (après
  `fingerprintFollowsPagesAndContent`), et l'include `<QByteArray>` en tête :

```cpp
    void resourcesAreKeyedBySha256AndDeduplicated()
    {
        Document d;
        const std::string key = d.addResource(QByteArray("abc"), "PNG");
        QCOMPARE(QString::fromStdString(key),
                 QStringLiteral("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
        QCOMPARE(d.addResource(QByteArray("abc"), "png"), key);
        QCOMPARE(d.resources().size(), std::size_t{1});
        QVERIFY(d.resource(key) != nullptr);
        QCOMPARE(d.resource(key)->extension, std::string("png"));
        QCOMPARE(d.resource(key)->data, QByteArray("abc"));
        QVERIFY(d.resource("absente") == nullptr);
        // Extension sûre seulement : jamais de chemin dans un nom d'entrée d'archive.
        QCOMPARE(d.resource(d.addResource(QByteArray("y"), "../../evil"))->extension, std::string("bin"));
        QCOMPARE(d.resource(d.addResource(QByteArray("z"), ""))->extension, std::string("bin"));
        QCOMPARE(d.resource(d.addResource(QByteArray("w"), "tropLongueExtension"))->extension, std::string("bin"));
    }

    void pagesCanBeInsertedAndTaken()
    {
        Document d;
        Page wide;
        wide.width = 800;
        d.insertPage(0, wide);
        QCOMPARE(d.pageCount(), std::size_t{2});
        QCOMPARE(d.page(0).width, 800.0);
        const Page taken = d.takePage(0);
        QCOMPARE(taken.width, 800.0);
        QCOMPARE(d.pageCount(), std::size_t{1});
        QCOMPARE(d.page(0).width, 1024.0);
    }

    void documentFromPagesNeverEmpty()
    {
        const Document empty(std::vector<Page>{});
        QCOMPARE(empty.pageCount(), std::size_t{1});
        Page a;
        a.width = 640;
        const Document two(std::vector<Page>{a, Page{}});
        QCOMPARE(two.pageCount(), std::size_t{2});
        QCOMPARE(two.page(0).width, 640.0);
        const Document sized(1280, 1024);
        QCOMPARE(sized.page(0).width, 1280.0);
        QCOMPARE(sized.page(0).height, 1024.0);
    }

    void identifiersAreAllocatedAfterTheLargestInUse()
    {
        Document d;
        QCOMPARE(d.allocateId(), ObjectId{1});
        QCOMPARE(d.allocateId(), ObjectId{2});
        PageObject o = redStroke();
        o.common.id = 40;
        d.page(0).objects.push_back(o);
        d.refreshNextId();
        QCOMPARE(d.nextId(), ObjectId{41});
        d.reserveIdsBelow(100);
        QCOMPARE(d.allocateId(), ObjectId{100});
        d.reserveIdsBelow(50); // jamais en arrière
        QCOMPARE(d.nextId(), ObjectId{101});
    }

    void uniqueIdsAreChecked()
    {
        Document d;
        PageObject a = redStroke();
        a.common.id = 1;
        PageObject b = redStroke();
        b.common.id = 2;
        d.page(0).objects = {a, b};
        QVERIFY(hasUniqueIds(d));
        d.appendPage();
        d.page(1).objects.push_back(a);
        QVERIFY(!hasUniqueIds(d)); // l'identifiant 1 sur deux pages
        d.page(1).objects[0].common.id = 0;
        QVERIFY(!hasUniqueIds(d)); // 0 n'est pas un identifiant
    }

    void fingerprintFollowsResourcesAndTitleButNotNextId()
    {
        Document a;
        Document b;
        b.setTitle("Leçon");
        QVERIFY(fingerprint(a) != fingerprint(b));
        Document c;
        c.addResource(QByteArray("x"), "png");
        QVERIFY(fingerprint(a) != fingerprint(c));
        Document d;
        d.allocateId();
        QCOMPARE(fingerprint(a), fingerprint(d));
    }
```

  La fonction `redStroke()` du fichier rend un `Stroke` : `PageObject o = redStroke();` s'appuie sur le constructeur
  hérité du variant. `QString` vient de `<QTest>`.

- [ ] **Étape 3 : compléter les tests du générateur.** Dans `tests/tst_core_generator.cpp`, remplacer la valeur
  de `kExpectedFingerprint` par la sentinelle `0xFFFFFFFFFFFFFFFFull` et son commentaire par :

```cpp
// Valeur relevée au premier passage sous Windows après l'ajout des propriétés communes du lot 2 (DA2 : empreinte
// recalculée), vérifiée sous macOS par la CI. Le contenu des 300 objets n'a pas changé ; seuls des champs à valeur
// par défaut et les identifiants 1 à 300 entrent dans l'empreinte. Valeur du lot 1 : 0xEC92781F3ED73B1E.
// Ne jamais la « corriger » : si elle change, c'est le générateur qui a changé.
```

  puis ajouter à la classe :

```cpp
    void objectsHaveIdsOneToThreeHundredOnTheMiddleLayer()
    {
        const Page page = generatePage();
        QCOMPARE(page.objects.size(), std::size_t{300});
        for (std::size_t i = 0; i < page.objects.size(); ++i) {
            QCOMPARE(page.objects[i].common.id, static_cast<ObjectId>(i + 1));
            QCOMPARE(page.objects[i].common.layer, Layer::Middle);
            QVERIFY(page.objects[i].common.transform.isIdentity());
        }
    }
```

- [ ] **Étape 4 : constater le rouge.** Commande ciblée, motif `tst_core_(model|document|generator)`. Attendu :
  `error` à la compilation (`ObjectCommon`, `Line`, `Document::addResource` inconnus).

- [ ] **Étape 5 : écrire le modèle.** Remplacer `core/include/actcraft/core/model.h` par :

```cpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

// Modèle d'une page (DA2, décision 15 du cadrage livraison-complete). Sans QtGui : couleurs en entiers,
// géométrie en doubles, textes en UTF-8. Les trois types du prototype (Shape, ImageObject, Stroke) gardent leurs
// champs et leur rang dans le variant : générateur du banc et moteurs de rendu inchangés.

namespace actcraft::core {

// Couleur ARGB 32 bits (0xAARRGGBB), même codage que QRgb.
using Argb = std::uint32_t;
// Identifiant stable d'un objet dans son document (décision 15) ; 0 = aucun.
using ObjectId = std::uint64_t;

struct PointF
{
    double x = 0;
    double y = 0;
};

struct RectF
{
    double x = 0;
    double y = 0;
    double width = 0;
    double height = 0;

    bool contains(PointF p) const noexcept
    {
        return p.x >= x && p.x <= x + width && p.y >= y && p.y <= y + height;
    }
    PointF center() const noexcept { return {x + width / 2, y + height / 2}; }
};

// Transformation affine 2D, convention de QTransform : x' = m11·x + m21·y + dx ; y' = m12·x + m22·y + dy.
struct Transform
{
    double m11 = 1;
    double m12 = 0;
    double m21 = 0;
    double m22 = 1;
    double dx = 0;
    double dy = 0;

    PointF map(PointF p) const noexcept { return {m11 * p.x + m21 * p.y + dx, m12 * p.x + m22 * p.y + dy}; }
    bool isIdentity() const noexcept
    {
        return m11 == 1 && m12 == 0 && m21 == 0 && m22 == 1 && dx == 0 && dy == 0;
    }
};

// `first` appliquée d'abord, puis `second` (QTransform : first * second).
Transform compose(const Transform& first, const Transform& second) noexcept;
Transform translation(double dx, double dy) noexcept;
Transform scaling(double sx, double sy) noexcept;
// Degrés, sens horaire à l'écran (y vers le bas), comme QTransform::rotate ; angles droits exacts.
Transform rotation(double degrees) noexcept;

// Quatre couches (F-510), de bas en haut. Page::objects est rangé par couche (isLayerOrdered, edits.h).
enum class Layer : std::uint8_t
{
    Background = 0,
    Bottom = 1,
    Middle = 2,
    Top = 3,
};

// Valeurs de Qt::PenStyle.
enum class PenStyle : std::uint8_t
{
    None = 0,
    Solid = 1,
    Dash = 2,
    Dot = 3,
    DashDot = 4,
    DashDotDot = 5,
};

enum class PenCap : std::uint8_t
{
    Flat = 0,
    Square = 1,
    Round = 2,
};

enum class PenJoin : std::uint8_t
{
    Miter = 0,
    Bevel = 1,
    Round = 2,
};

struct Outline
{
    PenStyle style = PenStyle::Solid;
    Argb color = 0xFF000000u;
    double width = 1;
    PenCap cap = PenCap::Round;
    PenJoin join = PenJoin::Round;
};

enum class FillKind : std::uint8_t
{
    None = 0,
    Solid = 1,
    Pattern = 2,
    Gradient = 3,
};

enum class GradientKind : std::uint8_t
{
    Vertical = 1,
    Horizontal = 2,
    DiagonalDown = 3, // coin haut gauche → bas droit
    DiagonalUp = 4,   // coin haut droit → bas gauche
    Radial = 5,
};

struct Fill
{
    FillKind kind = FillKind::None;
    Argb color = 0xFFFFFFFFu;
    Argb color2 = 0xFFFFFFFFu; // dégradé
    GradientKind gradient = GradientKind::Vertical;
    std::uint8_t pattern = 0;      // Qt::BrushStyle 2 à 14 (FillKind::Pattern)
    bool opaqueBackground = false; // F-574 : fond des motifs
    Argb backgroundColor = 0xFF000000u;
};

inline Fill solidFill(Argb color) noexcept
{
    Fill f;
    f.kind = FillKind::Solid;
    f.color = color;
    return f;
}

struct Restrictions
{
    bool movable = true;
    bool resizable = true;
    bool rotatable = true;
};

// Actions : données seulement au lot 2 (F-583) ; leur comportement arrive au lot 8.
enum class ActionKind : std::uint8_t
{
    Other = 0,
    Navigate = 1,
    OpenExternal = 2,
    OnObject = 3,
};

struct Action
{
    ActionKind kind = ActionKind::Other;
    std::uint16_t sourceType = 0; // numéro de type .flipchart d'origine ; 0 : action créée par ActCraft
    std::int32_t command = 0;
    ObjectId target = 0;
    std::string resource; // clé de ressource (fichier ouvert) ; vide sinon
};

// Propriétés communes à tout objet (décision 15).
struct ObjectCommon
{
    ObjectId id = 0;
    std::string name;
    Layer layer = Layer::Middle;
    bool visible = true;
    bool locked = false;
    std::uint8_t translucency = 0; // 0 opaque … 255 invisible : opacité = 1 − t / 255
    Restrictions restrictions;
    Transform transform; // coordonnées de l'objet → coordonnées de page
    std::string label;   // légende (F-576) ; vide : aucune
    std::vector<Action> actions;
};

// L'ordre suit celui d'ObjectVariant : kindOf() en dépend.
enum class ObjectKind : std::uint8_t
{
    Shape = 0,
    Image = 1,
    Stroke = 2,
    Line = 3,
    Text = 4,
    Media = 5,
    Group = 6,
    Connector = 7,
};
inline constexpr std::size_t kObjectKindCount = 8;

enum class ShapeKind : std::uint8_t
{
    Rectangle = 0,
    Ellipse = 1,
    Polygon = 2,
    Path = 3, // formes lues : sous-chemins, plume et remplissage complets
};

struct Shape
{
    ShapeKind kind = ShapeKind::Rectangle;
    RectF bounds;
    std::vector<PointF> polygon; // sommets (Polygon seulement)
    Argb fill = 0xFFFFFFFFu;
    Argb outline = 0xFF000000u;
    double outlineWidth = 1; // 0 : pas de contour
    double opacity = 1;
    // ShapeKind::Path : sous-chemins en coordonnées de l'objet, dessinés avec `pen` et `brush` (les quatre champs
    // de couleur ci-dessus ne servent pas). `bounds` : rectangle englobant lu (épaisseur de plume comprise).
    std::vector<std::vector<PointF>> paths;
    bool open = false;
    bool windingFill = false;
    Outline pen;
    Fill brush;
};

enum class ImagePattern : std::uint8_t
{
    LinearGradient = 0,
    Checker = 1,
    Rings = 2,
    SolidCenter = 3, // moitié centrale unie (colorA), bord colorB : objet témoin
};

// Image : procédurale (banc, `resource` vide) ou ressource du document (`resource` = clé SHA-256).
struct ImageObject
{
    RectF bounds;
    std::int32_t sourceWidth = 256;
    std::int32_t sourceHeight = 256;
    ImagePattern pattern = ImagePattern::LinearGradient;
    Argb colorA = 0xFF000000u;
    Argb colorB = 0xFFFFFFFFu;
    std::uint32_t seed = 0;
    std::string resource;
    bool hasKeyColor = false;
    Argb keyColor = 0;
};

struct StrokePoint
{
    double x = 0;
    double y = 0;
    float pressure = 0.5f;
};

enum class StrokeKind : std::uint8_t
{
    Pen = 0,
    Highlighter = 1,
    Revealer = 2, // encre révélatrice (N1-12)
};

struct Stroke
{
    std::vector<StrokePoint> points;
    double baseWidth = 3;
    Argb color = 0xFF000000u;
    StrokeKind kind = StrokeKind::Pen;
    PenCap cap = PenCap::Square; // bouts carrés : moteur du prototype ; les annotations lues sont rondes
};

// Largeur rendue d'un point : base × (0,5 + pression) (hypothèse N2-2).
inline double strokeWidthAt(double baseWidth, float pressure) noexcept
{
    return baseWidth * (0.5 + static_cast<double>(pressure));
}

struct Line
{
    std::vector<PointF> points;
    Outline pen;
};

// Ligne d'un texte mise en page à l'avance (.flipchart) ; vide pour un texte créé par ActCraft.
struct TextSpan
{
    std::string text;
    std::string fontFamily;
    double pointSize = 12;
    std::int32_t weight = 400; // échelle de Qt 6 (100 à 900)
    bool italic = false;
    bool underline = false;
    bool strikeOut = false;
    Argb color = 0xFF000000u;
    PointF baseline; // origine de la ligne de base, coordonnées de l'objet
    RectF bounds;
};

// Texte riche (décision 37) : HTML pour QTextDocument ; `runs`, s'il y en a, fait foi pour le dessin.
struct Text
{
    RectF bounds;
    std::string html;
    std::vector<TextSpan> runs;
};

// Son ou vidéo (N2-9 : ouvert par l'application du système) ; dessiné par son image d'espace réservé.
struct Media
{
    RectF bounds;
    std::string placeholder; // clé de ressource image ; vide : cadre par défaut
    std::string media;       // clé de ressource du son ou de la vidéo
    std::string fileName;
    bool playOnOpen = false;
    bool loop = false;
};

// Groupe par références (F-500) : les membres restent des objets de la page, à transformation absolue.
struct Group
{
    std::vector<ObjectId> members;
};

struct Connector
{
    PointF firstEnd;
    PointF secondEnd;
    ObjectId objectA = 0;
    ObjectId objectB = 0;
    Outline pen;
};

using ObjectVariant = std::variant<Shape, ImageObject, Stroke, Line, Text, Media, Group, Connector>;

// Objet de page : le variant de son type (std::get_if, std::get, index() marchent tels quels) et ses propriétés
// communes. Ne pas appeler std::visit dessus : passer par body().
struct PageObject : ObjectVariant
{
    using ObjectVariant::ObjectVariant;
    using ObjectVariant::operator=;
    ObjectCommon common;
};

inline const ObjectVariant& body(const PageObject& object) noexcept
{
    return object;
}
inline ObjectVariant& body(PageObject& object) noexcept
{
    return object;
}

ObjectKind kindOf(const PageObject& object) noexcept;

// Grille à niveaux (F-620, F-621) ; dessin d'après REA - Activinspire/app/actcraft/canvas/grid.py.
enum class GridPattern : std::uint8_t
{
    Normal = 0,
    Diagonal45 = 1,
    Angle = 2,
};

enum class GridLine : std::uint8_t
{
    Points = 0,
    Cross = 1,
    Solid = 2,
    Dotted = 3,
    Dashed = 4,
    BigDots = 5,
};

struct GridLevel
{
    bool visible = true;
    bool snap = true;
    PointF start;
    double spacingX = 32;
    double spacingY = 32;
    Argb color = 0xFF000000u;
    GridPattern pattern = GridPattern::Normal;
    double angle = 30;
    GridLine line = GridLine::Solid;
    bool linesAlongX = true;
    bool linesAlongY = true;
};

struct Grid
{
    bool visible = false;
    bool snapEnabled = false;
    bool drawnAbove = false;
    double previewZoom = 1;
    double baseZoom = 1;
    double zoomIncrement = 0.5;
    std::vector<GridLevel> levels;
};

// Fond de page (F-610) : couleur ou dégradé, image éventuelle par-dessus (clé de ressource).
struct Background
{
    Fill fill = solidFill(0xFFFFFFFFu);
    std::string image;
};

// Objets dans l'ordre de dessin, rangés par couche : le dernier est au premier plan.
struct Page
{
    double width = 1024;
    double height = 768;
    std::vector<PageObject> objects;
    Background background;
    Grid grid;
    std::string name;
    std::string notes;
};

// Empreinte FNV-1a 64 bits du contenu ; identique sur toutes les plateformes (types à taille fixe, octets dans un
// ordre fixé, réels par leur motif binaire IEEE 754, textes par leurs octets UTF-8 précédés de leur longueur).
std::uint64_t fingerprint(const Page& page) noexcept;

} // namespace actcraft::core
```

- [ ] **Étape 6 : écrire `model.cpp`, le document et le générateur.** Remplacer `core/src/model.cpp` par :

```cpp
#include <actcraft/core/model.h>

#include <bit>
#include <cmath>
#include <numbers>

namespace actcraft::core {

namespace {

class Fnv1a
{
public:
    void byte(std::uint8_t b) noexcept
    {
        m_hash ^= b;
        m_hash *= 0x100000001B3ull;
    }
    void u32(std::uint32_t v) noexcept
    {
        for (unsigned i = 0; i < 4; ++i) {
            byte(static_cast<std::uint8_t>((v >> (8u * i)) & 0xFFu));
        }
    }
    void u64(std::uint64_t v) noexcept
    {
        for (unsigned i = 0; i < 8; ++i) {
            byte(static_cast<std::uint8_t>((v >> (8u * i)) & 0xFFu));
        }
    }
    void i32(std::int32_t v) noexcept { u32(static_cast<std::uint32_t>(v)); }
    void f64(double v) noexcept { u64(std::bit_cast<std::uint64_t>(v)); }
    void f32(float v) noexcept { u32(std::bit_cast<std::uint32_t>(v)); }
    void boolean(bool v) noexcept { byte(v ? 1u : 0u); }
    template <typename E> void tag(E v) noexcept { byte(static_cast<std::uint8_t>(v)); }
    void text(const std::string& s) noexcept
    {
        u64(static_cast<std::uint64_t>(s.size()));
        for (const char c : s) {
            byte(static_cast<std::uint8_t>(c));
        }
    }
    void point(const PointF& p) noexcept
    {
        const auto& [x, y] = p;
        f64(x);
        f64(y);
    }
    void points(const std::vector<PointF>& ps) noexcept
    {
        u64(static_cast<std::uint64_t>(ps.size()));
        for (const PointF& p : ps) {
            point(p);
        }
    }
    void rect(const RectF& r) noexcept
    {
        const auto& [x, y, width, height] = r;
        f64(x);
        f64(y);
        f64(width);
        f64(height);
    }
    void transform(const Transform& t) noexcept
    {
        const auto& [m11, m12, m21, m22, dx, dy] = t;
        f64(m11);
        f64(m12);
        f64(m21);
        f64(m22);
        f64(dx);
        f64(dy);
    }
    void outline(const Outline& o) noexcept
    {
        const auto& [style, color, width, cap, join] = o;
        tag(style);
        u32(color);
        f64(width);
        tag(cap);
        tag(join);
    }
    void fill(const Fill& f) noexcept
    {
        const auto& [kind, color, color2, gradient, pattern, opaqueBackground, backgroundColor] = f;
        tag(kind);
        u32(color);
        u32(color2);
        tag(gradient);
        byte(pattern);
        boolean(opaqueBackground);
        u32(backgroundColor);
    }
    std::uint64_t value() const noexcept { return m_hash; }

private:
    std::uint64_t m_hash = 0xCBF29CE484222325ull;
};

void hashCommon(Fnv1a& h, const ObjectCommon& c) noexcept
{
    const auto& [id, name, layer, visible, locked, translucency, restrictions, transform, label, actions] = c;
    h.u64(id);
    h.text(name);
    h.tag(layer);
    h.boolean(visible);
    h.boolean(locked);
    h.byte(translucency);
    const auto& [movable, resizable, rotatable] = restrictions;
    h.boolean(movable);
    h.boolean(resizable);
    h.boolean(rotatable);
    h.transform(transform);
    h.text(label);
    h.u64(static_cast<std::uint64_t>(actions.size()));
    for (const Action& a : actions) {
        const auto& [kind, sourceType, command, target, resource] = a;
        h.tag(kind);
        h.u32(sourceType);
        h.i32(command);
        h.u64(target);
        h.text(resource);
    }
}

void hashBody(Fnv1a& h, const PageObject& object) noexcept
{
    if (const auto* s = std::get_if<Shape>(&object)) {
        const auto& [kind, bounds, polygon, fill, outline, outlineWidth, opacity, paths, open, windingFill, pen,
                     brush] = *s;
        h.tag(kind);
        h.rect(bounds);
        h.points(polygon);
        h.u32(fill);
        h.u32(outline);
        h.f64(outlineWidth);
        h.f64(opacity);
        h.u64(static_cast<std::uint64_t>(paths.size()));
        for (const auto& path : paths) {
            h.points(path);
        }
        h.boolean(open);
        h.boolean(windingFill);
        h.outline(pen);
        h.fill(brush);
    } else if (const auto* im = std::get_if<ImageObject>(&object)) {
        const auto& [bounds, sourceWidth, sourceHeight, pattern, colorA, colorB, seed, resource, hasKeyColor,
                     keyColor] = *im;
        h.rect(bounds);
        h.i32(sourceWidth);
        h.i32(sourceHeight);
        h.tag(pattern);
        h.u32(colorA);
        h.u32(colorB);
        h.u32(seed);
        h.text(resource);
        h.boolean(hasKeyColor);
        h.u32(keyColor);
    } else if (const auto* st = std::get_if<Stroke>(&object)) {
        const auto& [points, baseWidth, color, kind, cap] = *st;
        h.u64(static_cast<std::uint64_t>(points.size()));
        for (const StrokePoint& p : points) {
            const auto& [x, y, pressure] = p;
            h.f64(x);
            h.f64(y);
            h.f32(pressure);
        }
        h.f64(baseWidth);
        h.u32(color);
        h.tag(kind);
        h.tag(cap);
    } else if (const auto* l = std::get_if<Line>(&object)) {
        const auto& [points, pen] = *l;
        h.points(points);
        h.outline(pen);
    } else if (const auto* t = std::get_if<Text>(&object)) {
        const auto& [bounds, html, runs] = *t;
        h.rect(bounds);
        h.text(html);
        h.u64(static_cast<std::uint64_t>(runs.size()));
        for (const TextSpan& r : runs) {
            const auto& [text, fontFamily, pointSize, weight, italic, underline, strikeOut, color, baseline,
                         runBounds] = r;
            h.text(text);
            h.text(fontFamily);
            h.f64(pointSize);
            h.i32(weight);
            h.boolean(italic);
            h.boolean(underline);
            h.boolean(strikeOut);
            h.u32(color);
            h.point(baseline);
            h.rect(runBounds);
        }
    } else if (const auto* m = std::get_if<Media>(&object)) {
        const auto& [bounds, placeholder, media, fileName, playOnOpen, loop] = *m;
        h.rect(bounds);
        h.text(placeholder);
        h.text(media);
        h.text(fileName);
        h.boolean(playOnOpen);
        h.boolean(loop);
    } else if (const auto* g = std::get_if<Group>(&object)) {
        const auto& [members] = *g;
        h.u64(static_cast<std::uint64_t>(members.size()));
        for (const ObjectId id : members) {
            h.u64(id);
        }
    } else if (const auto* c = std::get_if<Connector>(&object)) {
        const auto& [firstEnd, secondEnd, objectA, objectB, pen] = *c;
        h.point(firstEnd);
        h.point(secondEnd);
        h.u64(objectA);
        h.u64(objectB);
        h.outline(pen);
    }
}

} // namespace

Transform compose(const Transform& a, const Transform& b) noexcept
{
    Transform t;
    t.m11 = a.m11 * b.m11 + a.m12 * b.m21;
    t.m12 = a.m11 * b.m12 + a.m12 * b.m22;
    t.m21 = a.m21 * b.m11 + a.m22 * b.m21;
    t.m22 = a.m21 * b.m12 + a.m22 * b.m22;
    t.dx = a.dx * b.m11 + a.dy * b.m21 + b.dx;
    t.dy = a.dx * b.m12 + a.dy * b.m22 + b.dy;
    return t;
}

Transform translation(double dx, double dy) noexcept
{
    Transform t;
    t.dx = dx;
    t.dy = dy;
    return t;
}

Transform scaling(double sx, double sy) noexcept
{
    Transform t;
    t.m11 = sx;
    t.m22 = sy;
    return t;
}

Transform rotation(double degrees) noexcept
{
    double turn = std::fmod(degrees, 360.0);
    if (turn < 0) {
        turn += 360.0;
    }
    double c = 1;
    double s = 0;
    if (turn == 90) {
        c = 0;
        s = 1;
    } else if (turn == 180) {
        c = -1;
        s = 0;
    } else if (turn == 270) {
        c = 0;
        s = -1;
    } else if (turn != 0) {
        const double r = turn * std::numbers::pi / 180.0;
        c = std::cos(r);
        s = std::sin(r);
    }
    Transform t;
    t.m11 = c;
    t.m12 = s;
    t.m21 = -s;
    t.m22 = c;
    return t;
}

ObjectKind kindOf(const PageObject& object) noexcept
{
    return static_cast<ObjectKind>(object.index());
}

// Chaque structure est lue par une liaison structurée qui nomme tous ses champs : un champ ajouté casse la
// compilation au lieu d'échapper à l'empreinte. Champs hachés dans l'ordre de déclaration ; pour chaque objet :
// son type (un octet), ses propriétés communes, puis les champs de son type. Le script Python de L2-T1 (étape 7)
// suit ce même ordre : le modifier change les valeurs figées des tests.
static_assert(std::variant_size_v<ObjectVariant> == kObjectKindCount,
              "type d'objet ajouté : l'ajouter à hashBody() et à kObjectKindCount");

std::uint64_t fingerprint(const Page& page) noexcept
{
    const auto& [width, height, objects, background, grid, name, notes] = page;
    Fnv1a h;
    h.f64(width);
    h.f64(height);
    h.u64(static_cast<std::uint64_t>(objects.size()));
    for (const PageObject& object : objects) {
        h.tag(kindOf(object));
        hashCommon(h, object.common);
        hashBody(h, object);
    }
    const auto& [fill, image] = background;
    h.fill(fill);
    h.text(image);
    const auto& [visible, snapEnabled, drawnAbove, previewZoom, baseZoom, zoomIncrement, levels] = grid;
    h.boolean(visible);
    h.boolean(snapEnabled);
    h.boolean(drawnAbove);
    h.f64(previewZoom);
    h.f64(baseZoom);
    h.f64(zoomIncrement);
    h.u64(static_cast<std::uint64_t>(levels.size()));
    for (const GridLevel& l : levels) {
        const auto& [lvVisible, snap, start, spacingX, spacingY, color, pattern, angle, line, linesAlongX, linesAlongY] = l;
        h.boolean(lvVisible);
        h.boolean(snap);
        h.point(start);
        h.f64(spacingX);
        h.f64(spacingY);
        h.u32(color);
        h.tag(pattern);
        h.f64(angle);
        h.tag(line);
        h.boolean(linesAlongX);
        h.boolean(linesAlongY);
    }
    h.text(name);
    h.text(notes);
    return h.value();
}

} // namespace actcraft::core
```

  Remplacer `core/include/actcraft/core/document.h` par :

```cpp
#pragma once

#include <actcraft/core/model.h>

#include <QByteArray>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace actcraft::core {

// Ressource d'un document (image, son, vidéo) : octets d'origine, jamais recompressés (DA3).
struct Resource
{
    QByteArray data;
    std::string extension; // [a-z0-9]{1,8}, sans point : « png », « jpg », « mp3 »… ; sinon « bin »
};

// Document (DA2, décision 15) : une suite de pages, jamais vide ; ressources par empreinte SHA-256 ;
// identifiants d'objets uniques dans tout le document. Les modifications passent par la pile d'annulation
// (undo.h, edits.h) ; ces méthodes sont ses briques.
class Document
{
public:
    Document(); // une page vierge de 1024 × 768
    Document(double pageWidth, double pageHeight);
    // Liste vide : une page vierge (un document n'est jamais vide).
    explicit Document(std::vector<Page> pages);

    std::size_t pageCount() const noexcept;
    // Exige index < pageCount() (assert).
    const Page& page(std::size_t index) const;
    Page& page(std::size_t index);
    // Ajoute à la fin une page vierge de la taille de la dernière ; retourne son indice.
    std::size_t appendPage();
    // Exige index <= pageCount().
    void insertPage(std::size_t index, Page page);
    // Retire et rend la page `index` ; exige pageCount() > 1.
    Page takePage(std::size_t index);

    // Identifiant neuf, jamais 0.
    ObjectId allocateId() noexcept;
    ObjectId nextId() const noexcept;
    // Le prochain identifiant dépasse tous ceux des pages (après une lecture ou une page posée telle quelle).
    void refreshNextId() noexcept;
    // Le prochain identifiant vaut au moins `next` ; jamais en arrière.
    void reserveIdsBelow(ObjectId next) noexcept;

    // Clé = SHA-256 des octets en hexadécimal minuscule (64 caractères) ; même contenu, même clé, une seule copie.
    std::string addResource(QByteArray data, std::string extension);
    // nullptr si la clé est inconnue.
    const Resource* resource(const std::string& key) const noexcept;
    const std::map<std::string, Resource>& resources() const noexcept;

    const std::string& title() const noexcept;
    void setTitle(std::string title);

private:
    std::vector<Page> m_pages;
    std::map<std::string, Resource> m_resources;
    ObjectId m_nextId = 1;
    std::string m_title;
};

// « Page suivante » (F-125) : indice suivant ; sur la dernière page, ajoute d'abord une page vierge.
std::size_t nextPageIndex(Document& document, std::size_t current);
// « Page précédente » : indice précédent, borné à 0.
std::size_t formerPageIndex(std::size_t current) noexcept;

// SHA-256 en hexadécimal minuscule.
std::string sha256Hex(const QByteArray& data);
// Vrai si chaque objet a un identifiant non nul, unique dans tout le document.
bool hasUniqueIds(const Document& document);

// Empreinte FNV-1a 64 bits : nombre de pages, empreinte de chaque page, titre, ressources (clé et extension ; la
// clé résume les octets). Le prochain identifiant n'y entre pas : un document relu a la même empreinte.
std::uint64_t fingerprint(const Document& document) noexcept;

} // namespace actcraft::core
```

  Remplacer `core/src/document.cpp` par :

```cpp
#include <actcraft/core/document.h>

#include <QCryptographicHash>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <set>
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

std::uint64_t mixText(std::uint64_t hash, const std::string& text) noexcept
{
    hash = mix(hash, static_cast<std::uint64_t>(text.size()));
    for (const char c : text) {
        hash ^= static_cast<std::uint8_t>(c);
        hash *= 0x100000001B3ull;
    }
    return hash;
}

Page blankPage(double width, double height)
{
    Page page;
    page.width = width;
    page.height = height;
    return page;
}

} // namespace

Document::Document() : m_pages(1) {}

Document::Document(double pageWidth, double pageHeight) : m_pages{blankPage(pageWidth, pageHeight)} {}

Document::Document(std::vector<Page> pages) : m_pages(std::move(pages))
{
    if (m_pages.empty()) {
        m_pages.emplace_back();
    }
    refreshNextId();
}

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
    m_pages.push_back(blankPage(m_pages.back().width, m_pages.back().height));
    return m_pages.size() - 1;
}

void Document::insertPage(std::size_t index, Page page)
{
    assert(index <= m_pages.size());
    m_pages.insert(m_pages.begin() + static_cast<std::ptrdiff_t>(index), std::move(page));
}

Page Document::takePage(std::size_t index)
{
    assert(index < m_pages.size() && m_pages.size() > 1);
    Page taken = std::move(m_pages[index]);
    m_pages.erase(m_pages.begin() + static_cast<std::ptrdiff_t>(index));
    return taken;
}

ObjectId Document::allocateId() noexcept
{
    return m_nextId++;
}

ObjectId Document::nextId() const noexcept
{
    return m_nextId;
}

void Document::refreshNextId() noexcept
{
    ObjectId largest = 0;
    for (const Page& p : m_pages) {
        for (const PageObject& o : p.objects) {
            largest = std::max(largest, o.common.id);
        }
    }
    reserveIdsBelow(largest + 1);
}

void Document::reserveIdsBelow(ObjectId next) noexcept
{
    m_nextId = std::max(m_nextId, next);
}

std::string Document::addResource(QByteArray data, std::string extension)
{
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
    // Extension sûre : elle forme un nom d'entrée d'archive (« resources/<clé>.<ext> ») et, au lot 5, un nom de
    // fichier exporté. [a-z0-9]{1,8}, sinon « bin » (relecture du plan, tour 1 : glissement de chemin).
    const bool safe = !extension.empty() && extension.size() <= 8 &&
                      std::all_of(extension.begin(), extension.end(),
                                  [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'); });
    if (!safe) {
        extension = "bin";
    }
    std::string key = sha256Hex(data);
    m_resources.try_emplace(key, Resource{std::move(data), std::move(extension)});
    return key;
}

const Resource* Document::resource(const std::string& key) const noexcept
{
    const auto it = m_resources.find(key);
    return it == m_resources.end() ? nullptr : &it->second;
}

const std::map<std::string, Resource>& Document::resources() const noexcept
{
    return m_resources;
}

const std::string& Document::title() const noexcept
{
    return m_title;
}

void Document::setTitle(std::string title)
{
    m_title = std::move(title);
}

std::size_t nextPageIndex(Document& document, std::size_t current)
{
    if (current + 1 < document.pageCount()) {
        return current + 1;
    }
    return document.appendPage();
}

std::size_t formerPageIndex(std::size_t current) noexcept
{
    return current == 0 ? 0 : current - 1;
}

std::string sha256Hex(const QByteArray& data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex().toStdString();
}

bool hasUniqueIds(const Document& document)
{
    std::set<ObjectId> seen;
    for (std::size_t i = 0; i < document.pageCount(); ++i) {
        for (const PageObject& o : document.page(i).objects) {
            if (o.common.id == 0 || !seen.insert(o.common.id).second) {
                return false;
            }
        }
    }
    return true;
}

std::uint64_t fingerprint(const Document& document) noexcept
{
    std::uint64_t hash = 0xCBF29CE484222325ull;
    hash = mix(hash, static_cast<std::uint64_t>(document.pageCount()));
    for (std::size_t i = 0; i < document.pageCount(); ++i) {
        hash = mix(hash, fingerprint(document.page(i)));
    }
    hash = mixText(hash, document.title());
    hash = mix(hash, static_cast<std::uint64_t>(document.resources().size()));
    for (const auto& [key, resource] : document.resources()) {
        hash = mixText(hash, key);
        hash = mixText(hash, resource.extension);
    }
    return hash;
}

} // namespace actcraft::core
```

  `Resource{std::move(data), std::move(extension)}` initialise **tous** les champs de `Resource` : pas
  d'avertissement de Clang. Si `fingerprint(const Document&)` lève un avertissement `noexcept` (allocation dans
  `mixText` : aucune), le garder `noexcept`.

  Dans `core/src/generator.cpp`, juste avant `return page;` de `generatePage` (ligne 285 au lot 1), ajouter :

```cpp
    // Identifiants stables (DA2) : 1 à 300 dans l'ordre de dessin ; tous sur la couche intermédiaire (défaut).
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        page.objects[i].common.id = static_cast<ObjectId>(i + 1);
    }
```

  Dans `render/include/actcraft/render/objectrenderer.h`, remplacer
  `std::array<std::unique_ptr<ObjectRenderer>, 3> m_renderers;` par
  `std::array<std::unique_ptr<ObjectRenderer>, core::kObjectKindCount> m_renderers;` (les types sans moteur rendent
  `nullptr` à `find`, que `FixedPageItem` saute déjà).

- [ ] **Étape 7 : recouper les valeurs figées hors C++.** Lancer la commande ciblée (motif `tst_core_`) : les trois
  tests compilent ; `fingerprintMatchesIndependentReference` et `fingerprintIsPinned` sont rouges et affichent les
  valeurs calculées. Écrire dans le dossier de travail de l'agent (hors dépôt) `fnv_lot2.py` :

```python
"""Recoupe hors C++ les empreintes figées de tst_core_model (L2-T1) : FNV-1a 64 bits sur les octets
petit-boutistes des champs, dans l'ordre de core/src/model.cpp."""
import struct

class F:
    def __init__(self):
        self.h = 0xCBF29CE484222325
    def b(self, v):
        self.h ^= v & 0xFF
        self.h = (self.h * 0x100000001B3) & 0xFFFFFFFFFFFFFFFF
    def raw(self, bs):
        for x in bs:
            self.b(x)
    def u32(self, v): self.raw(struct.pack('<I', v))
    def u64(self, v): self.raw(struct.pack('<Q', v))
    def i32(self, v): self.raw(struct.pack('<i', v))
    def f64(self, v): self.raw(struct.pack('<d', v))
    def f32(self, v): self.raw(struct.pack('<f', v))
    def boolean(self, v): self.b(1 if v else 0)
    def text(self, s):
        bs = s.encode('utf-8'); self.u64(len(bs)); self.raw(bs)

def outline_default(h):          # Outline{} : Solid, noir, 1, Round, Round
    h.b(1); h.u32(0xFF000000); h.f64(1.0); h.b(2); h.b(2)

def fill(h, kind):               # Fill{} (kind 0) ou solidFill(blanc) (kind 1)
    h.b(kind); h.u32(0xFFFFFFFF); h.u32(0xFFFFFFFF); h.b(1); h.b(0); h.boolean(False); h.u32(0xFF000000)

def common_default(h):           # ObjectCommon{}
    h.u64(0); h.text(''); h.b(2); h.boolean(True); h.boolean(False); h.b(0)
    h.boolean(True); h.boolean(True); h.boolean(True)
    for v in (1.0, 0.0, 0.0, 1.0, 0.0, 0.0): h.f64(v)
    h.text(''); h.u64(0)

def page_tail(h):                # fond blanc, grille par défaut, nom et notes vides
    fill(h, 1); h.text('')
    h.boolean(False); h.boolean(False); h.boolean(False); h.f64(1.0); h.f64(1.0); h.f64(0.5); h.u64(0)
    h.text(''); h.text('')

def empty_page():
    h = F(); h.f64(1024.0); h.f64(768.0); h.u64(0); page_tail(h); return h.h

def sample_page():
    h = F(); h.f64(1024.0); h.f64(768.0); h.u64(3)
    # Shape Polygon
    h.b(0); common_default(h)
    h.b(2); [h.f64(v) for v in (10.0, 20.0, 30.0, 40.0)]
    h.u64(3); [h.f64(v) for v in (10.0, 20.0, 40.0, 20.0, 25.0, 60.0)]
    h.u32(0xFF102030); h.u32(0xFF000000); h.f64(1.0); h.f64(1.0)
    h.u64(0); h.boolean(False); h.boolean(False); outline_default(h); fill(h, 0)
    # ImageObject Checker
    h.b(1); common_default(h)
    [h.f64(v) for v in (100.0, 100.0, 64.0, 48.0)]
    h.i32(256); h.i32(256); h.b(1); h.u32(0xFF000000); h.u32(0xFFFFFFFF); h.u32(0)
    h.text(''); h.boolean(False); h.u32(0)
    # Stroke
    h.b(2); common_default(h)
    h.u64(2); h.f64(1.0); h.f64(2.0); h.f32(0.25); h.f64(3.0); h.f64(4.0); h.f32(0.5)
    h.f64(3.0); h.u32(0xFF000000); h.b(0); h.b(1)
    page_tail(h)
    return h.h

print(f"Page{{}}       = 0x{empty_page():016X}")
print(f"samplePage()  = 0x{sample_page():016X}")
```

  Lancer `python -E <dossier de travail>\fnv_lot2.py`. Les deux valeurs doivent **égaler** celles qu'affiche
  `tst_core_model` (lire `.tmp/ctest.txt`, `QCOMPARE` donne la valeur obtenue en décimal : la convertir en
  hexadécimal). Égales : remplacer les deux sentinelles par ces valeurs. Différentes : c'est l'ordre des champs de
  `model.cpp` ou le script qui se trompe ; trouver lequel avant d'écrire quoi que ce soit (jamais copier la valeur du
  C++ sans recoupement). Remplacer ensuite `kExpectedFingerprint` par la valeur affichée par `fingerprintIsPinned`
  (`qWarning("fingerprint = 0x…")`). Coller la sortie du script dans le rapport.

- [ ] **Étape 8 : vert ciblé, puis preuve et banc.** Commande ciblée (motif `tst_core_`) : `100% tests passed`.
  Puis `format.ps1`, la preuve de tâche (N ≥ 44 ; `tst_core_model`, `tst_core_document`, `tst_core_generator`
  Passed) et le banc sans écran (`content_check.ok = True` prouve que la page du banc est restée la même).

- [ ] **Étape 9 : commit.**

```powershell
git -C <worktree> add core/include/actcraft/core/model.h core/src/model.cpp core/include/actcraft/core/document.h core/src/document.cpp core/src/generator.cpp render/include/actcraft/render/objectrenderer.h tests/tst_core_model.cpp tests/tst_core_document.cpp tests/tst_core_generator.cpp
git -C <worktree> commit -m "core : modèle complet du lot 2 (propriétés communes, cinq types, fond, grille, ressources) et empreintes recalculées" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t2"></a>
### Tâche L2-T2 — `core` : pile d'annulation, commandes du lot (objet au sommet de sa couche, page), changements

**Objectif.** Décision 16 et DA2 : une pile d'annulation propre à `core/` (`QUndoStack` est dans QtGui, interdit
ici), par document, 500 étapes au plus (N2-1), avec état propre (pour « enregistrer les modifications ? ») et
fusion des commandes consécutives de même famille. Chaque commande rend la liste des **changements** qu'elle a
faits (`Change`) : 2b les enverra au cache de la page affichée (DA11). Deux commandes au lot 2 : ajouter un objet
**au sommet de sa couche** (F-512 ; identifiant alloué une fois, gardé au rétablissement) et ajouter une page. Les
briques d'édition (`insertAtTopOfLayer`, `indexOf`, `depthInLayer`, `isLayerOrdered`, `boundsOnPage`) sont
publiques : les outils des lots 3 à 8 les réutilisent.

**Fichiers.**
- Créer : `core/include/actcraft/core/undo.h`, `core/src/undo.cpp`, `core/include/actcraft/core/edits.h`,
  `core/src/edits.cpp`, `tests/tst_core_undo.cpp`, `tests/tst_core_edits.cpp`
- Modifier : `core/CMakeLists.txt`, `tests/core.cmake`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_core_(undo|edits)` → `error` (`undo.h`
introuvable). Vert : forme bornée, `out of N` avec N ≥ 46, `TESTS OK`, `tst_core_undo .*Passed`,
`tst_core_edits .*Passed`.

**Interdits.** Pas de QtGui ni de `QObject` dans `undo.h`/`edits.h`. Une commande ne garde jamais de pointeur ou
de référence vers un objet de page (les vecteurs se réallouent) : elle garde un indice de page et un identifiant.
Pas de comportement d'interface (Annuler/Rétablir arrivent au lot 3).

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests de la pile.** Créer `tests/tst_core_undo.cpp` :

```cpp
#include <actcraft/core/document.h>
#include <actcraft/core/undo.h>

#include <QTest>

#include <memory>

using namespace actcraft::core;

namespace {

// Commande de test : ajoute `step` au titre du document (longueur = nombre d'étapes).
class Append final : public UndoCommand
{
public:
    explicit Append(char step, int mergeKey = -1) : m_steps(1, step), m_mergeKey(mergeKey) {}
    void redo(Document& d, Changes& changes) override
    {
        d.setTitle(d.title() + m_steps);
        changes.push_back(Change{Change::Kind::PageChanged, 0, 0, RectF{}});
    }
    void undo(Document& d, Changes& changes) override
    {
        d.setTitle(d.title().substr(0, d.title().size() - m_steps.size()));
        changes.push_back(Change{Change::Kind::PageChanged, 0, 0, RectF{}});
    }
    int mergeId() const noexcept override { return m_mergeKey; }
    bool mergeWith(const UndoCommand& other) override
    {
        m_steps += static_cast<const Append&>(other).m_steps;
        return true;
    }

private:
    std::string m_steps;
    int m_mergeKey;
};

} // namespace

class TstCoreUndo : public QObject
{
    Q_OBJECT

private slots:
    void pushRunsTheCommandAndReturnsItsChanges()
    {
        Document d;
        UndoStack stack;
        const Changes changes = stack.push(d, std::make_unique<Append>('a'));
        QCOMPARE(d.title(), std::string("a"));
        QCOMPARE(changes.size(), std::size_t{1});
        QVERIFY(stack.canUndo());
        QVERIFY(!stack.canRedo());
        QCOMPARE(stack.count(), std::size_t{1});
        QCOMPARE(stack.index(), std::size_t{1});
    }

    void undoAndRedoWalkTheStack()
    {
        Document d;
        UndoStack stack;
        stack.push(d, std::make_unique<Append>('a'));
        stack.push(d, std::make_unique<Append>('b'));
        QCOMPARE(stack.undo(d).size(), std::size_t{1});
        QCOMPARE(d.title(), std::string("a"));
        QVERIFY(stack.canRedo());
        stack.redo(d);
        QCOMPARE(d.title(), std::string("ab"));
        stack.undo(d);
        stack.undo(d);
        QCOMPARE(d.title(), std::string());
        QVERIFY(!stack.canUndo());
        QVERIFY(stack.undo(d).empty()); // rien à annuler : aucun changement
    }

    void pushAfterUndoDropsTheRedoBranch()
    {
        Document d;
        UndoStack stack;
        stack.push(d, std::make_unique<Append>('a'));
        stack.push(d, std::make_unique<Append>('b'));
        stack.undo(d);
        stack.push(d, std::make_unique<Append>('c'));
        QCOMPARE(d.title(), std::string("ac"));
        QVERIFY(!stack.canRedo());
        QCOMPARE(stack.count(), std::size_t{2});
    }

    void limitDropsTheOldestCommands()
    {
        Document d;
        UndoStack stack(3);
        QCOMPARE(stack.limit(), std::size_t{3});
        for (const char c : std::string("abcde")) {
            stack.push(d, std::make_unique<Append>(c));
        }
        QCOMPARE(stack.count(), std::size_t{3});
        stack.undo(d);
        stack.undo(d);
        stack.undo(d);
        QCOMPARE(d.title(), std::string("ab")); // « a » et « b » ne s'annulent plus
        QVERIFY(!stack.canUndo());
    }

    void defaultLimitIsFiveHundred() { QCOMPARE(UndoStack().limit(), std::size_t{500}); }

    void cleanStateFollowsTheSavedIndex()
    {
        Document d;
        UndoStack stack;
        QVERIFY(stack.isClean());
        stack.push(d, std::make_unique<Append>('a'));
        QVERIFY(!stack.isClean());
        stack.setClean();
        QVERIFY(stack.isClean());
        stack.push(d, std::make_unique<Append>('b'));
        QVERIFY(!stack.isClean());
        stack.undo(d);
        QVERIFY(stack.isClean()); // revenu à l'état enregistré
        stack.undo(d);
        QVERIFY(!stack.isClean());
    }

    void cleanStateLostWhenItsBranchIsDropped()
    {
        Document d;
        UndoStack stack;
        stack.push(d, std::make_unique<Append>('a'));
        stack.setClean();
        stack.undo(d);
        stack.push(d, std::make_unique<Append>('b')); // l'état enregistré (« a ») n'est plus atteignable
        stack.undo(d);
        QVERIFY(!stack.isClean());
        stack.redo(d);
        QVERIFY(!stack.isClean());
    }

    void cleanStateLostWhenTheLimitDropsIt()
    {
        Document d;
        UndoStack stack(2);
        stack.setClean(); // document vide enregistré
        stack.push(d, std::make_unique<Append>('a'));
        stack.push(d, std::make_unique<Append>('b'));
        stack.push(d, std::make_unique<Append>('c'));
        stack.undo(d);
        stack.undo(d);
        QVERIFY(!stack.isClean()); // « a » reste : ce n'est pas l'état enregistré
    }

    void consecutiveCommandsOfTheSameFamilyMerge()
    {
        Document d;
        UndoStack stack;
        stack.push(d, std::make_unique<Append>('a', 7));
        stack.push(d, std::make_unique<Append>('b', 7));
        stack.push(d, std::make_unique<Append>('c', 8));
        QCOMPARE(stack.count(), std::size_t{2});
        stack.undo(d);
        QCOMPARE(d.title(), std::string("ab"));
        stack.undo(d);
        QCOMPARE(d.title(), std::string()); // « a » et « b » fusionnés : une seule étape
    }

    void mergingIntoTheCleanCommandLosesTheCleanState()
    {
        Document d;
        UndoStack stack;
        stack.push(d, std::make_unique<Append>('a', 7));
        stack.setClean();
        stack.push(d, std::make_unique<Append>('b', 7));
        QVERIFY(!stack.isClean());
    }

    void clearEmptiesAndMarksClean()
    {
        Document d;
        UndoStack stack;
        stack.push(d, std::make_unique<Append>('a'));
        stack.clear();
        QCOMPARE(stack.count(), std::size_t{0});
        QVERIFY(stack.isClean());
        QVERIFY(!stack.canUndo());
    }
};

QTEST_GUILESS_MAIN(TstCoreUndo)
#include "tst_core_undo.moc"
```

- [ ] **Étape 2 : écrire les tests des commandes.** Créer `tests/tst_core_edits.cpp` :

```cpp
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/undo.h>

#include <QTest>

#include <memory>

using namespace actcraft::core;

namespace {

PageObject onLayer(Layer layer, ObjectId id)
{
    Stroke s;
    s.points = {{10, 10, 0.5f}, {30, 10, 0.5f}};
    s.baseWidth = 4;
    PageObject o = s;
    o.common.layer = layer;
    o.common.id = id;
    return o;
}

} // namespace

class TstCoreEdits : public QObject
{
    Q_OBJECT

private slots:
    void newObjectGoesAtTopOfItsLayer()
    {
        Page p;
        insertAtTopOfLayer(p, onLayer(Layer::Middle, 1));
        insertAtTopOfLayer(p, onLayer(Layer::Top, 2));
        QCOMPARE(insertAtTopOfLayer(p, onLayer(Layer::Middle, 3)), std::size_t{1});
        QCOMPARE(insertAtTopOfLayer(p, onLayer(Layer::Bottom, 4)), std::size_t{0});
        QCOMPARE(insertAtTopOfLayer(p, onLayer(Layer::Background, 5)), std::size_t{0});
        QCOMPARE(p.objects[0].common.id, ObjectId{5});
        QCOMPARE(p.objects[1].common.id, ObjectId{4});
        QCOMPARE(p.objects[2].common.id, ObjectId{1});
        QCOMPARE(p.objects[3].common.id, ObjectId{3});
        QCOMPARE(p.objects[4].common.id, ObjectId{2});
        QVERIFY(isLayerOrdered(p));
    }

    void depthCountsWithinTheLayer()
    {
        Page p;
        insertAtTopOfLayer(p, onLayer(Layer::Bottom, 1));
        insertAtTopOfLayer(p, onLayer(Layer::Middle, 2));
        insertAtTopOfLayer(p, onLayer(Layer::Middle, 3));
        QCOMPARE(depthInLayer(p, 1), std::size_t{0});
        QCOMPARE(depthInLayer(p, 2), std::size_t{0});
        QCOMPARE(depthInLayer(p, 3), std::size_t{1});
        QCOMPARE(indexOf(p, 3), std::size_t{2});
        QCOMPARE(indexOf(p, 99), p.objects.size());
    }

    void layerOrderIsChecked()
    {
        Page p;
        p.objects = {onLayer(Layer::Top, 1), onLayer(Layer::Middle, 2)};
        QVERIFY(!isLayerOrdered(p));
    }

    void boundsOnPageFollowsTheTransformAndTheWidth()
    {
        PageObject o = onLayer(Layer::Top, 1); // points (10,10)-(30,10), largeur 4 à pression 0,5
        RectF r = boundsOnPage(o);
        QCOMPARE(r.x, 8.0);
        QCOMPARE(r.y, 8.0);
        QCOMPARE(r.width, 24.0);
        QCOMPARE(r.height, 4.0);
        o.common.transform = translation(100, 0);
        r = boundsOnPage(o);
        QCOMPARE(r.x, 108.0);
        Text t;
        t.bounds = {0, 0, 50, 20};
        PageObject text = t;
        text.common.transform = rotation(90);
        r = boundsOnPage(text);
        QCOMPARE(r.x, -20.0);
        QCOMPARE(r.y, 0.0);
        QCOMPARE(r.width, 20.0);
        QCOMPARE(r.height, 50.0);
    }

    void addObjectAllocatesAnIdOnceAndUndoes()
    {
        Document d;
        UndoStack stack;
        Stroke s;
        s.points = {{1, 1, 0.5f}};
        PageObject o = s;
        o.common.layer = Layer::Top;
        auto command = std::make_unique<AddObjectCommand>(0, o);
        AddObjectCommand* raw = command.get();
        Changes changes = stack.push(d, std::move(command));
        const ObjectId id = raw->id();
        QVERIFY(id != 0);
        QCOMPARE(d.page(0).objects.size(), std::size_t{1});
        QCOMPARE(d.page(0).objects[0].common.id, id);
        QCOMPARE(changes.size(), std::size_t{1});
        QCOMPARE(changes[0].kind, Change::Kind::ObjectInserted);
        QCOMPARE(changes[0].page, std::size_t{0});
        QCOMPARE(changes[0].object, id);
        changes = stack.undo(d);
        QVERIFY(d.page(0).objects.empty());
        QCOMPARE(changes[0].kind, Change::Kind::ObjectRemoved);
        QCOMPARE(changes[0].object, id);
        stack.redo(d);
        QCOMPARE(d.page(0).objects[0].common.id, id); // même identifiant au rétablissement
        QCOMPARE(d.nextId(), id + 1);
    }

    void addObjectLandsAtTopOfItsLayerNotAtTheEnd()
    {
        Document d;
        UndoStack stack;
        stack.push(d, std::make_unique<AddObjectCommand>(0, onLayer(Layer::Top, 0)));
        stack.push(d, std::make_unique<AddObjectCommand>(0, onLayer(Layer::Middle, 0)));
        QCOMPARE(d.page(0).objects[0].common.layer, Layer::Middle);
        QCOMPARE(d.page(0).objects[1].common.layer, Layer::Top);
    }

    void addPageInsertsABlankPageOfTheGivenSize()
    {
        Document d;
        UndoStack stack;
        Changes changes = stack.push(d, std::make_unique<AddPageCommand>(1, 800, 600));
        QCOMPARE(d.pageCount(), std::size_t{2});
        QCOMPARE(d.page(1).width, 800.0);
        QCOMPARE(changes[0].kind, Change::Kind::PageInserted);
        QCOMPARE(changes[0].page, std::size_t{1});
        changes = stack.undo(d);
        QCOMPARE(d.pageCount(), std::size_t{1});
        QCOMPARE(changes[0].kind, Change::Kind::PageRemoved);
    }
};

QTEST_GUILESS_MAIN(TstCoreEdits)
#include "tst_core_edits.moc"
```

  Déclarer les deux tests à la fin de `tests/core.cmake` :

```cmake
actcraft_add_test(tst_core_undo
    SOURCES tst_core_undo.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_edits
    SOURCES tst_core_edits.cpp
    LIBS ActCraft::core)
```

- [ ] **Étape 3 : constater le rouge.** Commande ciblée, motif `tst_core_(undo|edits)` → `error` (`undo.h`
  introuvable).

- [ ] **Étape 4 : écrire la pile.** Créer `core/include/actcraft/core/undo.h` :

```cpp
#pragma once

#include <actcraft/core/model.h>

#include <cstddef>
#include <limits>
#include <memory>
#include <vector>

namespace actcraft::core {

class Document;

// Ce qu'une commande a changé : le cache de la page affichée (DA11) et l'interface s'en servent.
struct Change
{
    enum class Kind : std::uint8_t
    {
        ObjectInserted,
        ObjectRemoved,
        ObjectChanged,
        PageInserted,
        PageRemoved,
        PageChanged,
    };
    Kind kind;
    std::size_t page;
    ObjectId object; // 0 pour un changement de page
    RectF area;      // zone touchée en coordonnées de page ; vide = toute la page
};

using Changes = std::vector<Change>;

// Une modification du document (décision 16). Ne garde ni pointeur ni référence vers les objets d'une page :
// un indice de page et un identifiant d'objet.
class UndoCommand
{
public:
    virtual ~UndoCommand() = default;
    virtual void redo(Document& document, Changes& changes) = 0;
    virtual void undo(Document& document, Changes& changes) = 0;
    // Deux commandes consécutives de même famille (≥ 0) peuvent fusionner : glissements continus (lot 3).
    virtual int mergeId() const noexcept { return -1; }
    // Absorbe `other`, déjà exécutée ; vrai si la fusion a eu lieu.
    virtual bool mergeWith(const UndoCommand& other)
    {
        static_cast<void>(other);
        return false;
    }
};

// Pile d'annulation d'un document (décision 16, N2-1 : 500 étapes). Toute modification du document passe par
// push() ; l'état propre suit le dernier enregistrement.
class UndoStack
{
public:
    static constexpr std::size_t kDefaultLimit = 500;

    explicit UndoStack(std::size_t limit = kDefaultLimit); // 0 traité comme 1

    // Exécute la commande, efface les commandes annulées, fusionne si possible, borne à limit().
    Changes push(Document& document, std::unique_ptr<UndoCommand> command);
    Changes undo(Document& document); // rien si !canUndo()
    Changes redo(Document& document); // rien si !canRedo()

    bool canUndo() const noexcept;
    bool canRedo() const noexcept;
    std::size_t count() const noexcept;
    std::size_t index() const noexcept; // commandes exécutées (0 … count())
    std::size_t limit() const noexcept;

    bool isClean() const noexcept;
    void setClean() noexcept; // l'état courant est celui du fichier
    void clear() noexcept;    // pile vide, état propre

private:
    static constexpr std::size_t kNoClean = std::numeric_limits<std::size_t>::max();

    std::vector<std::unique_ptr<UndoCommand>> m_commands;
    std::size_t m_index = 0;
    std::size_t m_clean = 0;
    std::size_t m_limit;
};

} // namespace actcraft::core
```

  Créer `core/src/undo.cpp` :

```cpp
#include <actcraft/core/document.h>
#include <actcraft/core/undo.h>

#include <algorithm>

namespace actcraft::core {

UndoStack::UndoStack(std::size_t limit) : m_limit(std::max<std::size_t>(1, limit)) {}

Changes UndoStack::push(Document& document, std::unique_ptr<UndoCommand> command)
{
    Changes changes;
    if (!command) {
        return changes;
    }
    command->redo(document, changes);
    if (m_index < m_commands.size()) {
        if (m_clean != kNoClean && m_clean > m_index) {
            m_clean = kNoClean; // l'état enregistré était dans la branche effacée
        }
        m_commands.erase(m_commands.begin() + static_cast<std::ptrdiff_t>(m_index), m_commands.end());
    }
    if (!m_commands.empty()) {
        UndoCommand& top = *m_commands.back();
        if (top.mergeId() >= 0 && top.mergeId() == command->mergeId() && top.mergeWith(*command)) {
            if (m_clean == m_index) {
                m_clean = kNoClean; // la commande enregistrée a changé
            }
            return changes;
        }
    }
    m_commands.push_back(std::move(command));
    ++m_index;
    if (m_commands.size() > m_limit) {
        m_commands.erase(m_commands.begin());
        --m_index;
        if (m_clean != kNoClean) {
            m_clean = m_clean == 0 ? kNoClean : m_clean - 1;
        }
    }
    return changes;
}

Changes UndoStack::undo(Document& document)
{
    Changes changes;
    if (!canUndo()) {
        return changes;
    }
    --m_index;
    m_commands[m_index]->undo(document, changes);
    return changes;
}

Changes UndoStack::redo(Document& document)
{
    Changes changes;
    if (!canRedo()) {
        return changes;
    }
    m_commands[m_index]->redo(document, changes);
    ++m_index;
    return changes;
}

bool UndoStack::canUndo() const noexcept
{
    return m_index > 0;
}

bool UndoStack::canRedo() const noexcept
{
    return m_index < m_commands.size();
}

std::size_t UndoStack::count() const noexcept
{
    return m_commands.size();
}

std::size_t UndoStack::index() const noexcept
{
    return m_index;
}

std::size_t UndoStack::limit() const noexcept
{
    return m_limit;
}

bool UndoStack::isClean() const noexcept
{
    return m_clean == m_index;
}

void UndoStack::setClean() noexcept
{
    m_clean = m_index;
}

void UndoStack::clear() noexcept
{
    m_commands.clear();
    m_index = 0;
    m_clean = 0;
}

} // namespace actcraft::core
```

- [ ] **Étape 5 : écrire les commandes du lot.** Créer `core/include/actcraft/core/edits.h` :

```cpp
#pragma once

#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <cstddef>

namespace actcraft::core {

// Briques d'édition d'une page ; Page::objects reste rangé par couche (F-510, F-512).

// Insère `object` au sommet de sa couche ; retourne son indice dans page.objects.
std::size_t insertAtTopOfLayer(Page& page, PageObject object);
// Indice de l'objet `id`, ou page.objects.size() s'il n'y est pas.
std::size_t indexOf(const Page& page, ObjectId id) noexcept;
// Profondeur (F-512, lecture seule) : rang dans sa couche, 0 = base ; page.objects.size() si absent.
std::size_t depthInLayer(const Page& page, ObjectId id) noexcept;
// Vrai si les objets sont rangés par couche croissante.
bool isLayerOrdered(const Page& page) noexcept;
// Rectangle englobant en coordonnées de page, épaisseur comprise (transformation appliquée) ; vide pour un groupe.
RectF boundsOnPage(const PageObject& object) noexcept;

// Ajoute un objet au sommet de sa couche sur la page `page`. L'identifiant est alloué au premier redo() si
// l'objet n'en a pas, puis gardé.
class AddObjectCommand final : public UndoCommand
{
public:
    AddObjectCommand(std::size_t page, PageObject object);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;
    ObjectId id() const noexcept; // 0 avant le premier redo()

private:
    std::size_t m_page;
    PageObject m_object;
};

// Ajoute une page vierge `width` × `height` à l'indice `index` (≤ pageCount()).
class AddPageCommand final : public UndoCommand
{
public:
    AddPageCommand(std::size_t index, double width, double height);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_index;
    double m_width;
    double m_height;
};

} // namespace actcraft::core
```

  Créer `core/src/edits.cpp` :

```cpp
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>

#include <algorithm>
#include <cassert>
#include <limits>
#include <utility>

namespace actcraft::core {

namespace {

struct Box
{
    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();

    void add(PointF p) noexcept
    {
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
    }
    void addRect(const RectF& r) noexcept
    {
        add({r.x, r.y});
        add({r.x + r.width, r.y + r.height});
    }
    bool empty() const noexcept { return minX > maxX; }
    RectF grown(double margin) const noexcept
    {
        return {minX - margin, minY - margin, maxX - minX + 2 * margin, maxY - minY + 2 * margin};
    }
};

// Rectangle local (coordonnées de l'objet) ; marge = demi-épaisseur du trait.
RectF localBounds(const PageObject& object) noexcept
{
    Box box;
    double margin = 0;
    if (const auto* s = std::get_if<Shape>(&object)) {
        if (s->kind == ShapeKind::Path) {
            for (const auto& path : s->paths) {
                for (const PointF& p : path) {
                    box.add(p);
                }
            }
            if (box.empty()) {
                box.addRect(s->bounds);
            }
            margin = s->pen.style == PenStyle::None ? 0 : s->pen.width / 2;
        } else if (s->kind == ShapeKind::Polygon) {
            for (const PointF& p : s->polygon) {
                box.add(p);
            }
            margin = s->outlineWidth / 2;
        } else {
            box.addRect(s->bounds);
            margin = s->outlineWidth / 2;
        }
    } else if (const auto* im = std::get_if<ImageObject>(&object)) {
        box.addRect(im->bounds);
    } else if (const auto* st = std::get_if<Stroke>(&object)) {
        for (const StrokePoint& p : st->points) {
            box.add({p.x, p.y});
            margin = std::max(margin, strokeWidthAt(st->baseWidth, p.pressure) / 2);
        }
    } else if (const auto* l = std::get_if<Line>(&object)) {
        for (const PointF& p : l->points) {
            box.add(p);
        }
        margin = l->pen.width / 2;
    } else if (const auto* t = std::get_if<Text>(&object)) {
        box.addRect(t->bounds);
    } else if (const auto* m = std::get_if<Media>(&object)) {
        box.addRect(m->bounds);
    } else if (const auto* c = std::get_if<Connector>(&object)) {
        box.add(c->firstEnd);
        box.add(c->secondEnd);
        margin = c->pen.width / 2;
    }
    return box.empty() ? RectF{} : box.grown(margin);
}

} // namespace

std::size_t insertAtTopOfLayer(Page& page, PageObject object)
{
    const Layer layer = object.common.layer;
    const auto it = std::find_if(page.objects.begin(), page.objects.end(),
                                 [layer](const PageObject& o) { return o.common.layer > layer; });
    const auto index = static_cast<std::size_t>(it - page.objects.begin());
    page.objects.insert(it, std::move(object));
    return index;
}

std::size_t indexOf(const Page& page, ObjectId id) noexcept
{
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        if (page.objects[i].common.id == id) {
            return i;
        }
    }
    return page.objects.size();
}

std::size_t depthInLayer(const Page& page, ObjectId id) noexcept
{
    const std::size_t index = indexOf(page, id);
    if (index == page.objects.size()) {
        return index;
    }
    std::size_t depth = 0;
    for (std::size_t i = 0; i < index; ++i) {
        if (page.objects[i].common.layer == page.objects[index].common.layer) {
            ++depth;
        }
    }
    return depth;
}

bool isLayerOrdered(const Page& page) noexcept
{
    for (std::size_t i = 1; i < page.objects.size(); ++i) {
        if (page.objects[i].common.layer < page.objects[i - 1].common.layer) {
            return false;
        }
    }
    return true;
}

RectF boundsOnPage(const PageObject& object) noexcept
{
    const RectF local = localBounds(object);
    if (local.width == 0 && local.height == 0 && local.x == 0 && local.y == 0) {
        return local;
    }
    const Transform& t = object.common.transform;
    Box box;
    box.add(t.map({local.x, local.y}));
    box.add(t.map({local.x + local.width, local.y}));
    box.add(t.map({local.x, local.y + local.height}));
    box.add(t.map({local.x + local.width, local.y + local.height}));
    return box.grown(0);
}

AddObjectCommand::AddObjectCommand(std::size_t page, PageObject object) : m_page(page), m_object(std::move(object)) {}

void AddObjectCommand::redo(Document& document, Changes& changes)
{
    if (m_object.common.id == 0) {
        m_object.common.id = document.allocateId();
    }
    insertAtTopOfLayer(document.page(m_page), m_object);
    changes.push_back(Change{Change::Kind::ObjectInserted, m_page, m_object.common.id, boundsOnPage(m_object)});
}

void AddObjectCommand::undo(Document& document, Changes& changes)
{
    Page& page = document.page(m_page);
    const std::size_t index = indexOf(page, m_object.common.id);
    assert(index < page.objects.size());
    page.objects.erase(page.objects.begin() + static_cast<std::ptrdiff_t>(index));
    changes.push_back(Change{Change::Kind::ObjectRemoved, m_page, m_object.common.id, boundsOnPage(m_object)});
}

ObjectId AddObjectCommand::id() const noexcept
{
    return m_object.common.id;
}

AddPageCommand::AddPageCommand(std::size_t index, double width, double height)
    : m_index(index), m_width(width), m_height(height)
{
}

void AddPageCommand::redo(Document& document, Changes& changes)
{
    Page page;
    page.width = m_width;
    page.height = m_height;
    document.insertPage(m_index, std::move(page));
    changes.push_back(Change{Change::Kind::PageInserted, m_index, 0, RectF{}});
}

void AddPageCommand::undo(Document& document, Changes& changes)
{
    document.takePage(m_index);
    changes.push_back(Change{Change::Kind::PageRemoved, m_index, 0, RectF{}});
}

} // namespace actcraft::core
```

  Dans `boundsOnPage`, l'objet vide (groupe) donne `RectF{}` ; `RectF{0, 0, 0, 0}` d'un objet réel en (0, 0) sans
  taille est un cas limite sans conséquence (le cache redessine toute la page sur une zone vide). Si MSVC signale
  `ObjectId` → `std::uint64_t` dans `document.allocateId()` + 1 du test : c'est le même type.

  Ajouter `src/undo.cpp` et `src/edits.cpp` à la liste `SOURCES` de `core/CMakeLists.txt` (après
  `src/document.cpp`).

- [ ] **Étape 6 : vert ciblé.** Commande ciblée, motif `tst_core_(undo|edits)` : `100% tests passed`.

- [ ] **Étape 7 : preuve.** `format.ps1`, puis la preuve de tâche (N ≥ 46 ; `tst_core_undo` et `tst_core_edits`
  Passed).

- [ ] **Étape 8 : commit.**

```powershell
git -C <worktree> add core/include/actcraft/core/undo.h core/src/undo.cpp core/include/actcraft/core/edits.h core/src/edits.cpp core/CMakeLists.txt tests/core.cmake tests/tst_core_undo.cpp tests/tst_core_edits.cpp
git -C <worktree> commit -m "core : pile d'annulation (500 étapes, état propre, fusion) et commandes d'ajout d'objet et de page" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t3"></a>
### Tâche L2-T3 — `actions` : argument facultatif, commandes de fichier déclarées

**Objectif.** Décisions 17, 20 et 22 : les commandes de fichier prennent une URL (chemin local ou `content://`) et
s'appellent sans dialogue par les scénarios (S5) ; déclenchées sans argument (menu, raccourci), elles ouvrent le
dialogue. Le registre accepte donc un argument **facultatif** : absent (`QVariant` invalide), il passe ; présent
mais invalide, il est refusé. Déclarer les dix commandes `file.*` du lot 2 (F-700, partie). Leur place dans les
menus et leurs gestionnaires arrivent en 2b (L2-T18) : déclarées sans gestionnaire, elles restent invisibles
(le pont `Commands` du lot 1 n'affiche que les commandes disponibles).

**Fichiers.**
- Modifier : `actions/include/actcraft/actions/commandregistry.h`, `actions/src/commandregistry.cpp`,
  `actions/data/commands.json`, `tests/tst_actions_registry.cpp`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_actions_registry` → `error`
(`argumentOptional` inconnu). Vert : forme bornée, N ≥ 44, `TESTS OK`, `tst_actions_registry .*Passed`.

**Interdits.** Ne renommer aucune commande du lot 1. Ne pas toucher à `actions/data/layout.json` (L2-T18). Pas
d'icône nouvelle. Libellés écrits par nous (« paperboard » est un terme courant, N1-12).

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests.** Dans `tests/tst_actions_registry.cpp`, ajouter à la classe :

```cpp
    void optionalArgumentMayBeAbsent()
    {
        CommandRegistry r;
        QString error;
        QVERIFY2(r.loadDeclarations(R"({"format":"actcraft-commands/1","commands":[
            {"id":"file.open","label":"Ouvrir…","argument":"url","optional":true},
            {"id":"file.export","label":"Exporter","argument":"url"}]})",
                                    error),
                 qPrintable(error));
        QVERIFY(r.find(QStringLiteral("file.open"))->argumentOptional);
        QVERIFY(!r.find(QStringLiteral("file.export"))->argumentOptional);
        QVariantList seen;
        r.setHandler(QStringLiteral("file.open"), [&seen](const QVariant& a) { seen << a; });
        r.setHandler(QStringLiteral("file.export"), [&seen](const QVariant& a) { seen << a; });
        const Platform here = currentPlatform();
        QVERIFY(r.trigger(QStringLiteral("file.open"), QVariant(), here));           // absent : dialogue
        QVERIFY(r.trigger(QStringLiteral("file.open"), QStringLiteral("C:/a.actcraft"), here));
        QVERIFY(!r.trigger(QStringLiteral("file.open"), QStringLiteral(""), here));  // présent mais vide
        QVERIFY(!r.trigger(QStringLiteral("file.open"), 42, here));                  // présent mais pas une URL
        QVERIFY(!r.trigger(QStringLiteral("file.export"), QVariant(), here));        // obligatoire
        QCOMPARE(seen.size(), 2);
        QVERIFY(!seen.at(0).isValid());
    }

    void optionalWithoutArgumentIsRefused()
    {
        CommandRegistry r;
        QString error;
        QVERIFY(!r.loadDeclarations(
            R"({"format":"actcraft-commands/1","commands":[{"id":"a.b","label":"A","optional":true}]})", error));
        QVERIFY2(error.contains(QStringLiteral("facultatif")), qPrintable(error));
    }
```

  Dans `defaultCommandsFileDeclaresLotOne` (même nom, contenu étendu : les commandes du lot 2 s'ajoutent),
  remplacer la liste `expected` par :

```cpp
        const QStringList expected{
            QStringLiteral("file.new"),         QStringLiteral("file.new-screen"),
            QStringLiteral("file.new-1024x768"), QStringLiteral("file.new-1152x864"),
            QStringLiteral("file.new-1280x1024"), QStringLiteral("file.new-custom"),
            QStringLiteral("file.open"),        QStringLiteral("file.save"),
            QStringLiteral("file.save-as"),     QStringLiteral("file.close"),
            QStringLiteral("file.quit"),        QStringLiteral("view.fullscreen"),
            QStringLiteral("view.fit"),         QStringLiteral("view.zoom"),
            QStringLiteral("page.previous"),    QStringLiteral("page.next"),
            QStringLiteral("tool.pen"),         QStringLiteral("pen.color"),
            QStringLiteral("pen.width"),        QStringLiteral("menu.main"),
            QStringLiteral("menu.tools"),       QStringLiteral("help.website"),
            QStringLiteral("help.about")};
```

  et ajouter à la fin de cette fonction :

```cpp
        QVERIFY(r.find(QStringLiteral("file.open"))->argument == ArgumentKind::Url);
        QVERIFY(r.find(QStringLiteral("file.open"))->argumentOptional);
        QVERIFY(r.find(QStringLiteral("file.save-as"))->argumentOptional);
        QCOMPARE(r.find(QStringLiteral("file.new"))->shortcut, QStringLiteral("Ctrl+N"));
        QCOMPARE(r.find(QStringLiteral("file.open"))->shortcut, QStringLiteral("Ctrl+O"));
        QCOMPARE(r.find(QStringLiteral("file.save"))->shortcut, QStringLiteral("Ctrl+S"));
        QCOMPARE(r.find(QStringLiteral("file.save-as"))->shortcut, QStringLiteral("Ctrl+Shift+S"));
        QCOMPARE(r.find(QStringLiteral("file.close"))->shortcut, QStringLiteral("Ctrl+W"));
```

- [ ] **Étape 2 : constater le rouge.** Commande ciblée, motif `tst_actions_registry` → `error`
  (`argumentOptional` n'est pas membre de `CommandInfo`).

- [ ] **Étape 3 : écrire le code.** Dans `commandregistry.h`, ajouter à `CommandInfo`, après
  `ArgumentKind argument = ArgumentKind::None;` :

```cpp
    // Vrai : la commande se déclenche aussi sans argument (QVariant invalide) — dialogue au lieu d'une URL
    // (décision 22). Un argument présent doit rester valide.
    bool argumentOptional = false;
```

  et compléter le commentaire de `trigger` : « …ou si l'argument ne convient pas (un argument absent convient à une
  commande dont l'argument est facultatif) ».

  Dans `commandregistry.cpp`, `loadDeclarations`, juste après le bloc qui lit `"argument"` (après
  `parseArgumentKind`, avant `seen.insert`) :

```cpp
        info.argumentOptional = o.value(QStringLiteral("optional")).toBool(false);
        if (info.argumentOptional && info.argument == ArgumentKind::None) {
            error = where + QStringLiteral(" : argument facultatif sans argument déclaré");
            return false;
        }
```

  et dans `trigger`, remplacer la condition `!argumentIsValid(e->info.argument, argument)` par :

```cpp
        !(argumentIsValid(e->info.argument, argument) || (e->info.argumentOptional && !argument.isValid()))
```

- [ ] **Étape 4 : déclarer les commandes.** Dans `actions/data/commands.json`, insérer **avant** la ligne de
  `file.quit` :

```json
    { "id": "file.new", "label": "Nouveau paperboard", "shortcut": "Ctrl+N" },
    { "id": "file.new-screen", "label": "Paperboard à la taille de l'écran" },
    { "id": "file.new-1024x768", "label": "Paperboard 1024 × 768" },
    { "id": "file.new-1152x864", "label": "Paperboard 1152 × 864" },
    { "id": "file.new-1280x1024", "label": "Paperboard 1280 × 1024" },
    { "id": "file.new-custom", "label": "Taille de paperboard personnalisée…" },
    { "id": "file.open", "label": "Ouvrir…", "shortcut": "Ctrl+O", "argument": "url", "optional": true },
    { "id": "file.save", "label": "Enregistrer", "shortcut": "Ctrl+S" },
    { "id": "file.save-as", "label": "Enregistrer sous…", "shortcut": "Ctrl+Shift+S", "argument": "url", "optional": true },
    { "id": "file.close", "label": "Fermer le paperboard", "shortcut": "Ctrl+W" },
```

- [ ] **Étape 5 : vert ciblé, puis preuve.** Commande ciblée (`tst_actions_registry`) : `100% tests passed`.
  `format.ps1`, puis la preuve de tâche (N ≥ 44 ; `tst_actions_registry` Passed). Les tests `tst_ui_commands` et
  `tst_ui_shell` doivent rester verts sans changement (commandes sans gestionnaire : absentes des menus) ; s'ils
  échouent, c'est que le pont affiche une commande sans gestionnaire : le consigner et s'arrêter (hors
  périmètre de la tâche, décision de l'orchestrateur).

- [ ] **Étape 6 : commit.**

```powershell
git -C <worktree> add actions/include/actcraft/actions/commandregistry.h actions/src/commandregistry.cpp actions/data/commands.json tests/tst_actions_registry.cpp
git -C <worktree> commit -m "actions : argument facultatif et commandes de fichier du lot 2 (nouveau, ouvrir, enregistrer, fermer)" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t4"></a>
### Tâche L2-T4 — miniz vendu, `formats/` interdit de QtGui, fichiers de tests des formats

**Objectif.** DA3 : le zip du format natif passe par **miniz** (MIT), pas par `QZipReader`/`QZipWriter` (API
privée de Qt, `Qt6::CorePrivate`). Vendre miniz 3.0.2 amalgamé dans `third_party/miniz/` (provenance et SHA-256
écrits, notice dans `THIRD_PARTY_NOTICES.md`), compilé sans nos avertissements ni notre format, en archives
reproductibles (`MINIZ_NO_TIME`) et sans noms zlib (Qt embarque zlib). DA4 : `actcraft_formats` ne lie jamais
QtGui (CMake échoue sinon). Préparer les fichiers CMake qui laissent avancer en parallèle le format natif et le
lecteur `.flipchart` : `formats/native.cmake`, `formats/flipchart.cmake`, `tests/formats.cmake`,
`tests/flipchart.cmake`.

**Fichiers.**
- Créer : `third_party/miniz/miniz.c`, `third_party/miniz/miniz.h`, `third_party/miniz/LICENSE`,
  `third_party/miniz/README.md`, `third_party/miniz/CMakeLists.txt`, `third_party/.clang-format`, `formats/native.cmake`,
  `formats/flipchart.cmake`, `tests/formats.cmake`, `tests/flipchart.cmake`, `tests/tst_third_party_miniz.cpp`
- Modifier : `CMakeLists.txt`, `formats/CMakeLists.txt`, `scripts/format.ps1`, `.graphifyignore`,
  `THIRD_PARTY_NOTICES.md`, `tests/CMakeLists.txt`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_third_party_miniz` → `error` (`miniz.h`
introuvable). Vert : forme bornée, N ≥ 45, `TESTS OK`, `tst_third_party_miniz .*Passed`. Garde-fou QtGui, après
le commit (étape 9) : `Qt6::Gui` ajouté aux `DEPENDS` de `formats/CMakeLists.txt` → configuration en échec
(`actcraft_formats ne doit pas dépendre de Qt6::Gui`), puis fichier remis par
`git -C <worktree> checkout -- formats/CMakeLists.txt`.

**Interdits.** Aucune modification de `miniz.c`/`miniz.h` (code tiers tel que publié). miniz n'est ni formaté ni
compilé avec `/W4 /WX`. Pas de `QZipReader`. Le fichier téléchargé est une donnée non fiable : dans un dossier neuf
du dossier de travail de l'agent, jamais exécuté.

**Terminé quand.** Preuve verte, garde-fou vu rouge, SHA-256 dans `README.md`, commit fait.

**Étapes.**

- [ ] **Étape 1 : télécharger miniz.** Dans un dossier neuf du dossier de travail de l'agent (hors dépôt) :

```powershell
pwsh -NoProfile -Command "`$d = Join-Path `$env:TEMP ('miniz-' + [guid]::NewGuid()); New-Item -ItemType Directory `$d | Out-Null; Invoke-WebRequest https://github.com/richgel999/miniz/releases/download/3.0.2/miniz-3.0.2.zip -OutFile (Join-Path `$d 'miniz-3.0.2.zip'); (Get-FileHash (Join-Path `$d 'miniz-3.0.2.zip') -Algorithm SHA256).Hash; Expand-Archive (Join-Path `$d 'miniz-3.0.2.zip') (Join-Path `$d 'x'); Get-ChildItem -Recurse (Join-Path `$d 'x') | Select-Object -ExpandProperty FullName; Write-Output `$d"
```

  Attendu : l'archive contient `miniz.c`, `miniz.h` et `LICENSE` à sa racine (version amalgamée). Si ce n'est pas le
  cas (pas de `miniz.c` amalgamé), **s'arrêter** et le rapporter (ne pas assembler soi-même le code tiers).
  Copier `miniz.c`, `miniz.h`, `LICENSE` dans `third_party/miniz/` du worktree, puis relever le SHA-256 des trois
  fichiers copiés (`Get-FileHash`).

- [ ] **Étape 2 : écrire le test.** Créer `tests/tst_third_party_miniz.cpp` :

```cpp
#include <miniz.h>

#include <QByteArray>
#include <QTest>

namespace {

QByteArray zipOf(const QByteArray& text, const QByteArray& stored)
{
    mz_zip_archive zip{};
    if (!mz_zip_writer_init_heap(&zip, 0, 1024)) {
        return {};
    }
    mz_zip_writer_add_mem(&zip, "a.txt", text.constData(), static_cast<size_t>(text.size()),
                          static_cast<mz_uint>(MZ_BEST_COMPRESSION));
    mz_zip_writer_add_mem(&zip, "b.bin", stored.constData(), static_cast<size_t>(stored.size()),
                          static_cast<mz_uint>(MZ_NO_COMPRESSION));
    void* buffer = nullptr;
    size_t size = 0;
    QByteArray out;
    if (mz_zip_writer_finalize_heap_archive(&zip, &buffer, &size)) {
        out = QByteArray(static_cast<const char*>(buffer), static_cast<qsizetype>(size));
        mz_free(buffer);
    }
    mz_zip_writer_end(&zip);
    return out;
}

QByteArray extract(const QByteArray& archive, const char* name)
{
    mz_zip_archive zip{};
    if (!mz_zip_reader_init_mem(&zip, archive.constData(), static_cast<size_t>(archive.size()), 0)) {
        return {};
    }
    QByteArray out;
    const int index = mz_zip_reader_locate_file(&zip, name, nullptr, 0);
    mz_zip_archive_file_stat stat{};
    if (index >= 0 && mz_zip_reader_file_stat(&zip, static_cast<mz_uint>(index), &stat)) {
        out.resize(static_cast<qsizetype>(stat.m_uncomp_size));
        if (!mz_zip_reader_extract_to_mem(&zip, static_cast<mz_uint>(index), out.data(),
                                          static_cast<size_t>(out.size()), 0)) {
            out.clear();
        }
    }
    mz_zip_reader_end(&zip);
    return out;
}

} // namespace

// miniz vendu (DA3) : écriture en mémoire, relecture, archives reproductibles (MINIZ_NO_TIME).
class TstThirdPartyMiniz : public QObject
{
    Q_OBJECT

private slots:
    void roundTripInMemory()
    {
        const QByteArray text(5000, 'x');
        const QByteArray stored("\x89PNG\r\n\x1a\n", 8);
        const QByteArray archive = zipOf(text, stored);
        QVERIFY(archive.startsWith("PK\x03\x04"));
        QVERIFY(archive.size() < text.size()); // compressé
        QCOMPARE(extract(archive, "a.txt"), text);
        QCOMPARE(extract(archive, "b.bin"), stored);
        QVERIFY(extract(archive, "absent").isEmpty());
    }

    void sameInputGivesSameBytes()
    {
        const QByteArray a = zipOf("bonjour", "abc");
        QTest::qWait(1100); // une seconde de plus : sans MINIZ_NO_TIME, la date DOS changerait
        QCOMPARE(zipOf("bonjour", "abc"), a);
    }
};

QTEST_GUILESS_MAIN(TstThirdPartyMiniz)
#include "tst_third_party_miniz.moc"
```

  Créer `tests/formats.cmake` :

```cmake
# Tests du format natif (DA3) : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).

actcraft_add_test(tst_third_party_miniz
    SOURCES tst_third_party_miniz.cpp
    LIBS ActCraft::miniz)
```

  Créer `tests/flipchart.cmake` :

```cmake
# Tests du lecteur .flipchart (DA4) : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).
# Les paperboards des tests sont écrits par tests/support/flipchartwriter.h ; samples/ n'est lu que par
# tst_formats_oracle, en place, et seulement si ACTCRAFT_SAMPLES_DIR est posé (décision 19).
```

  Dans `tests/CMakeLists.txt`, après la ligne `include(${CMAKE_CURRENT_SOURCE_DIR}/actions.cmake)`, ajouter :

```cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/formats.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/flipchart.cmake)
```

- [ ] **Étape 3 : constater le rouge.** Commande ciblée, motif `tst_third_party_miniz` → erreur de configuration
  ou de compilation (`ActCraft::miniz` / `miniz.h` inconnus).

- [ ] **Étape 4 : cible CMake de miniz.** Créer `third_party/miniz/CMakeLists.txt` :

```cmake
# miniz 3.0.2 (MIT) : archives zip du format natif .actcraft (DA3). Code tiers tel que publié : ni nos
# avertissements, ni clang-format (scripts/format.ps1 exclut third_party/).
add_library(actcraft_miniz STATIC miniz.c miniz.h)
add_library(ActCraft::miniz ALIAS actcraft_miniz)
target_include_directories(actcraft_miniz SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Sans E/S de fichiers (Qt s'en charge), sans horodatage (archives reproductibles), sans noms zlib (Qt embarque
# zlib : aucun conflit de symboles).
target_compile_definitions(actcraft_miniz PUBLIC MINIZ_NO_STDIO MINIZ_NO_TIME MINIZ_NO_ZLIB_COMPATIBLE_NAMES)
set_target_properties(actcraft_miniz PROPERTIES POSITION_INDEPENDENT_CODE ON)
if(MSVC)
    target_compile_options(actcraft_miniz PRIVATE /W0 /utf-8)
else()
    target_compile_options(actcraft_miniz PRIVATE -w)
endif()
```

  Créer `third_party/miniz/README.md` (valeurs relevées à l'étape 1) :

```markdown
# miniz 3.0.2

- Source : https://github.com/richgel999/miniz/releases/download/3.0.2/miniz-3.0.2.zip (version amalgamée).
- SHA-256 de l'archive : <valeur de l'étape 1>
- SHA-256 de `miniz.c` : <valeur> ; de `miniz.h` : <valeur> ; de `LICENSE` : <valeur>
- Licence : MIT (`LICENSE`), reprise dans `THIRD_PARTY_NOTICES.md`.
- Modifications locales : aucune. Options de compilation : `MINIZ_NO_STDIO`, `MINIZ_NO_TIME`,
  `MINIZ_NO_ZLIB_COMPATIBLE_NAMES` (`CMakeLists.txt`).
- Usage : format natif `.actcraft` (décision 17, DA3 du cadrage `docs/specs/2026-10-14-livraison-complete-cadrage.md`).
```

  Les trois `<valeur>` sont les empreintes réelles relevées, pas des exemples.

- [ ] **Étape 5 : brancher miniz et les formats.** Dans `CMakeLists.txt` racine : `LANGUAGES CXX` devient
  `LANGUAGES C CXX` ; ajouter `add_subdirectory(third_party/miniz)` juste **avant** `add_subdirectory(core)` (avec
  le commentaire `# Code tiers (DA3) : avant les modules qui l'utilisent.`) ; et, après la ligne
  `actcraft_forbid_links(actcraft_core …)`, ajouter :

```cmake
# DA4 : formats/ lit et écrit sans interface graphique (QtCore seulement) ; images et PDF vont dans export/ (DA12).
actcraft_forbid_links(actcraft_formats Qt6::Gui Qt6::Quick Qt6::Qml Qt6::Widgets)
```

  Remplacer `formats/CMakeLists.txt` par :

```cmake
# Lecture et écriture des formats : .flipchart (lecture, DA4) et format natif ouvert .actcraft (DA3). QtCore
# seulement (CMakeLists.txt racine : actcraft_forbid_links). Deux listes de sources, une par famille, pour que
# les deux chantiers avancent sans toucher le même fichier.
include(${CMAKE_CURRENT_SOURCE_DIR}/native.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/flipchart.cmake)
actcraft_add_module(formats
    SOURCES
        src/formats.cpp
        ${ACTCRAFT_FORMATS_NATIVE_SOURCES}
        ${ACTCRAFT_FORMATS_FLIPCHART_SOURCES}
    DEPENDS
        ActCraft::core)
target_link_libraries(actcraft_formats PRIVATE ActCraft::miniz)
```

  Créer `formats/native.cmake` :

```cmake
# Sources du format natif .actcraft (DA3), relatives à formats/. Complété par L2-T5 et L2-T6.
set(ACTCRAFT_FORMATS_NATIVE_SOURCES)
```

  Créer `formats/flipchart.cmake` :

```cmake
# Sources du lecteur .flipchart (DA4), relatives à formats/. Complété par L2-T7 à L2-T10.
set(ACTCRAFT_FORMATS_FLIPCHART_SOURCES)
```

  Dans `scripts/format.ps1`, remplacer la ligne `$files = git ls-files …` par :

```powershell
# Code tiers (third_party/) : tel que publié, jamais reformaté.
$files = git ls-files --cached --others --exclude-standard -- '*.cpp' '*.h' '*.hpp' ':!third_party/**'
```

  Dans `.graphifyignore`, ajouter la ligne `third_party/` (avec le commentaire `# Code tiers vendu.`).

  Le job `format` de la CI (`.github/workflows/ci.yml:30`) passe **tous** les `*.h` suivis par git à
  `clang-format --dry-run --Werror`, donc `third_party/miniz/miniz.h` : `format.ps1` ne suffit pas. Créer
  `third_party/.clang-format` (clang-format prend le fichier de configuration le plus proche) :

```yaml
# Code tiers tel que publié : jamais reformaté. Le job « format » de la CI passe tous les *.h suivis par git.
DisableFormat: true
SortIncludes: Never
```

  Dans `THIRD_PARTY_NOTICES.md`, ajouter à la fin une section :

```markdown
## miniz 3.0.2

Bibliothèque de compression et d'archives zip, liée statiquement à ActCraft pour le format `.actcraft`.
Source : https://github.com/richgel999/miniz (version 3.0.2). Licence MIT :

<texte intégral du fichier third_party/miniz/LICENSE, recopié tel quel dans un bloc de code>
```

  Le texte de la licence est celui du fichier téléchargé, recopié à l'identique (obligation de la licence MIT).

- [ ] **Étape 6 : vert ciblé.** Commande ciblée (motif `tst_third_party_miniz`) : `100% tests passed`. Rejouer
  ensuite la commande du job `format` de la CI, avec le clang-format que trouve `format.ps1`, sur les deux fichiers
  tiers : `clang-format --dry-run --Werror third_party/miniz/miniz.h third_party/miniz/miniz.c` → code 0 (sans
  `third_party/.clang-format`, des centaines d'écarts : le constater une fois, rouge vu). Vérifier
  aussi `pwsh -NoProfile -Command "Set-Location '<worktree>'; pwsh -NoProfile -File ./scripts/format.ps1 -Check; Write-Output `$LASTEXITCODE"` → `0`
  (miniz ignoré).

- [ ] **Étape 7 : preuve.** Preuve de tâche (N ≥ 45 ; `tst_third_party_miniz` Passed). `tst_ui_about` doit rester
  vert (il lit `THIRD_PARTY_NOTICES.md`).

- [ ] **Étape 8 : commit.**

```powershell
git -C <worktree> add third_party/miniz/miniz.c third_party/miniz/miniz.h third_party/miniz/LICENSE third_party/miniz/README.md third_party/miniz/CMakeLists.txt third_party/.clang-format CMakeLists.txt formats/CMakeLists.txt formats/native.cmake formats/flipchart.cmake scripts/format.ps1 .graphifyignore THIRD_PARTY_NOTICES.md tests/CMakeLists.txt tests/formats.cmake tests/flipchart.cmake tests/tst_third_party_miniz.cpp
git -C <worktree> commit -m "miniz 3.0.2 vendu pour le format natif ; formats/ interdit de QtGui ; fichiers de tests des formats" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

- [ ] **Étape 9 : garde-fou QtGui vu rouge.** Ajouter `Qt6::Gui` après `ActCraft::core` dans les `DEPENDS` de
  `formats/CMakeLists.txt`, lancer `pwsh -NoProfile -Command "Set-Location '<worktree>'; pwsh -NoProfile -File ./scripts/build.ps1 *> .tmp/garde.txt; Select-String -Path .tmp/garde.txt -Pattern 'ne doit pas dépendre' | Select-Object -First 1"`
  → la ligne `actcraft_formats ne doit pas dépendre de Qt6::Gui` ; puis
  `git -C <worktree> checkout -- formats/CMakeLists.txt` et reconstruire (`build.ps1` vert). Rapporter la ligne vue.


<a id="tache-l2-t5"></a>
### Tâche L2-T5 — `formats` : `document.json` (écriture, lecture tolérante, version majeure refusée)

**Objectif.** DA3, décision 17 : le cœur du format natif est `document.json`, `"format": "actcraft-document/1"`,
clés en anglais. **Parité écrivain/lecteur** : tout champ du modèle écrit est relu à l'identique (même empreinte
`core::fingerprint(const Document&)`, même prochain identifiant). Lecture **tolérante** : clés inconnues ignorées,
valeurs absentes = défauts du modèle, objet de type inconnu sauté avec un avertissement, valeur mal formée
remplacée par le défaut avec un avertissement ; **refus** d'un format absent, étranger ou d'une version majeure
autre que 1, sans toucher au document de destination. Les octets des ressources ne sont pas dans le JSON : un
chargeur (`ResourceLoader`) les fournit (l'archive, L2-T6) ; une ressource dont les octets ne redonnent pas la clé
SHA-256 est écartée.

Patron à **ne pas** reprendre : sérialiser par `std::visit` + `QVariantMap` génériques, ou écrire les énumérations
en nombres. Les clés et les noms d'énumérations sont écrits en clair, un par un, et figés par
`keysAreFrozen` ; un nombre d'énumération rendrait le fichier illisible et fragile au moindre réordonnancement.

**Fichiers.**
- Créer : `formats/include/actcraft/formats/nativejson.h`, `formats/src/nativejson.cpp`,
  `tests/support/richdocument.h`, `tests/tst_formats_nativejson.cpp`
- Modifier : `formats/native.cmake`, `tests/formats.cmake`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_formats_nativejson` → `error` (`nativejson.h`
introuvable). Vert : forme bornée, N ≥ 46, `TESTS OK`, `tst_formats_nativejson .*Passed`.

**Interdits.** QtCore seulement (pas de `QColor`, pas de `QImage`). Ne pas écrire les ressources en base64 dans le
JSON. Ne pas changer le modèle (L2-T1). Pas de clé française dans le JSON.

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire le document de test.** Créer `tests/support/richdocument.h` :

```cpp
#pragma once

// Document de test qui remplit chaque champ du modèle d'une valeur hors défaut (DA2) : allers-retours du format
// natif (L2-T5, L2-T6) et des fichiers (L2-T17). Écrit par nous ; les « images » sont de faux octets (le format ne
// les décode pas).

#include <actcraft/core/document.h>

#include <QByteArray>

#include <string>
#include <utility>
#include <vector>

namespace richdoc {

inline const QByteArray& pngBytes()
{
    static const QByteArray bytes("\x89PNG\r\n\x1a\nfaux", 12);
    return bytes;
}

inline const QByteArray& mp3Bytes()
{
    static const QByteArray bytes("ID3\x04\0\0\0\0\0\0son", 13);
    return bytes;
}

inline actcraft::core::Document richDocument()
{
    using namespace actcraft::core;
    const std::string png = sha256Hex(pngBytes());
    const std::string mp3 = sha256Hex(mp3Bytes());
    std::vector<Page> pages(3);

    Page& p1 = pages[0];
    p1.name = "Page un";
    p1.notes = "Notes : « é è à »";

    Shape rect;
    rect.kind = ShapeKind::Rectangle;
    rect.bounds = {10, 20, 100, 50};
    rect.fill = 0xFF336699u;
    rect.outline = 0xFF112233u;
    rect.outlineWidth = 2.5;
    rect.opacity = 0.75;
    PageObject o1 = rect;
    o1.common.id = 1;
    o1.common.name = "Rectangle";
    o1.common.layer = Layer::Bottom;
    o1.common.visible = false;
    o1.common.locked = true;
    o1.common.translucency = 40;
    o1.common.restrictions.movable = false;
    o1.common.restrictions.resizable = false;
    o1.common.restrictions.rotatable = false;
    o1.common.transform = compose(rotation(30), translation(5.5, -2.25));
    o1.common.label = "Légende";
    Action action;
    action.kind = ActionKind::OnObject;
    action.sourceType = 28;
    action.command = 184;
    action.target = 6;
    action.resource = mp3;
    o1.common.actions.push_back(action);
    p1.objects.push_back(o1);

    ImageObject generated;
    generated.bounds = {200, 10, 64, 48};
    generated.sourceWidth = 128;
    generated.sourceHeight = 96;
    generated.pattern = ImagePattern::Rings;
    generated.colorA = 0xFF00FF00u;
    generated.colorB = 0xFF0000FFu;
    generated.seed = 7;
    PageObject o2 = generated;
    o2.common.id = 2;
    p1.objects.push_back(o2);

    ImageObject picture;
    picture.bounds = {0, 0, 2, 2};
    picture.sourceWidth = 2;
    picture.sourceHeight = 2;
    picture.resource = png;
    picture.pattern = ImagePattern::Rings;
    picture.colorA = 0xFF00FF00u;
    picture.colorB = 0xFF0000FFu;
    picture.seed = 3;
    picture.hasKeyColor = true;
    picture.keyColor = 0xFFFFFFFFu;
    PageObject o3 = picture;
    o3.common.id = 3;
    o3.common.transform = scaling(50, 25);
    p1.objects.push_back(o3);

    Shape path;
    path.kind = ShapeKind::Path;
    path.bounds = {-3, -3, 86, 66};
    path.paths = {{{0, 0}, {80, 0}, {80, 60}}, {{20, 20}, {60, 20}, {40, 50}}};
    path.windingFill = true;
    path.open = true;
    path.polygon = {{1, 1}, {9, 1}, {5, 8}};
    path.fill = 0xFF010101u;
    path.outline = 0xFF020202u;
    path.outlineWidth = 4;
    path.opacity = 0.5;
    path.brush.pattern = 5;
    path.pen.style = PenStyle::Dash;
    path.pen.color = 0xFF010203u;
    path.pen.width = 6;
    path.pen.cap = PenCap::Flat;
    path.pen.join = PenJoin::Bevel;
    path.brush.kind = FillKind::Gradient;
    path.brush.color = 0xFF00AA00u;
    path.brush.color2 = 0x80FFFFFFu;
    path.brush.gradient = GradientKind::Radial;
    path.brush.opaqueBackground = true;
    path.brush.backgroundColor = 0xFF444444u;
    PageObject o4 = path;
    o4.common.id = 4;
    p1.objects.push_back(o4);

    Line line;
    line.points = {{0, 0}, {50, 25}, {100, 0}};
    line.pen.style = PenStyle::DashDotDot;
    line.pen.width = 3;
    PageObject o5 = line;
    o5.common.id = 5;
    p1.objects.push_back(o5);

    Text text;
    text.bounds = {0, 0, 180, 40};
    text.html = "<p>Bonjour <b>tout</b> le monde</p>";
    TextSpan first;
    first.text = "Bonjour";
    first.fontFamily = "Arial";
    first.pointSize = 24;
    first.weight = 700;
    first.italic = true;
    first.color = 0xFFFF0000u;
    first.underline = true;
    first.strikeOut = true;
    first.baseline = {0, 30};
    first.bounds = {0, 0, 90, 36};
    TextSpan second = first;
    second.text = "tout";
    second.underline = true;
    second.strikeOut = true;
    second.weight = 400;
    text.runs = {first, second};
    PageObject o6 = text;
    o6.common.id = 6;
    p1.objects.push_back(o6);

    Media media;
    media.bounds = {0, 0, 64, 48};
    media.placeholder = png;
    media.media = mp3;
    media.fileName = "chanson.mp3";
    media.playOnOpen = true;
    media.loop = true;
    PageObject o7 = media;
    o7.common.id = 7;
    p1.objects.push_back(o7);

    Group group;
    group.members = {4, 5};
    PageObject o8 = group;
    o8.common.id = 8;
    p1.objects.push_back(o8);

    Connector connector;
    connector.firstEnd = {1, 2};
    connector.secondEnd = {300, 400};
    connector.objectA = 4;
    connector.objectB = 6;
    connector.pen.color = 0xFF990000u;
    PageObject o9 = connector;
    o9.common.id = 9;
    o9.common.layer = Layer::Top;
    p1.objects.push_back(o9);

    Stroke stroke;
    stroke.points = {{10, 10, 0.25f}, {20, 15, 0.5f}, {30, 10, 0.75f}};
    stroke.baseWidth = 6;
    stroke.color = 0x80FFFF00u;
    stroke.kind = StrokeKind::Highlighter;
    stroke.cap = PenCap::Round;
    PageObject o10 = stroke;
    o10.common.id = 10;
    o10.common.layer = Layer::Top;
    p1.objects.push_back(o10);

    Page& p2 = pages[1];
    p2.name = "Page deux";
    p2.notes = "Notes de la page deux";
    p2.width = 800;
    p2.height = 600;
    p2.background.fill.kind = FillKind::Gradient;
    p2.background.fill.color = 0xFFC8DCFFu;
    p2.background.fill.color2 = 0xFFFFFFFFu;
    p2.background.fill.gradient = GradientKind::DiagonalUp;
    p2.background.image = png;
    p2.grid.visible = true;
    p2.grid.snapEnabled = true;
    p2.grid.drawnAbove = true;
    p2.grid.previewZoom = 0.5;
    p2.grid.baseZoom = 2;
    p2.grid.zoomIncrement = 0.25;
    GridLevel coarse;
    coarse.spacingX = 50;
    coarse.spacingY = 25;
    coarse.color = 0xFF0000FFu;
    GridLevel fine;
    fine.visible = false;
    fine.snap = false;
    fine.start = {3, 4};
    fine.pattern = GridPattern::Angle;
    fine.angle = 45;
    fine.line = GridLine::Dotted;
    fine.linesAlongY = false;
    fine.linesAlongX = false;
    fine.spacingX = 64;
    fine.spacingY = 48;
    fine.color = 0xFF00FF00u;
    p2.grid.levels = {coarse, fine};

    Document document(std::move(pages));
    document.setTitle("Document riche");
    document.addResource(pngBytes(), "png");
    document.addResource(mp3Bytes(), "MP3");
    document.reserveIdsBelow(20);
    return document;
}

} // namespace richdoc
```

- [ ] **Étape 2 : écrire les tests.** Créer `tests/tst_formats_nativejson.cpp` :

```cpp
#include "support/richdocument.h"

#include <actcraft/formats/nativejson.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

using namespace actcraft;
using namespace Qt::StringLiterals;

namespace {

formats::ResourceLoader loaderFrom(const core::Document& source)
{
    return [&source](const std::string& key, const std::string&) -> std::optional<QByteArray> {
        const core::Resource* r = source.resource(key);
        if (!r) {
            return std::nullopt;
        }
        return r->data;
    };
}

QJsonObject object(const QJsonObject& root, int page, int index)
{
    return root.value(u"pages"_s).toArray().at(page).toObject().value(u"objects"_s).toArray().at(index).toObject();
}

QJsonObject withObject(QJsonObject root, int page, int index, const QJsonObject& value)
{
    QJsonArray pages = root.value(u"pages"_s).toArray();
    QJsonObject p = pages.at(page).toObject();
    QJsonArray objects = p.value(u"objects"_s).toArray();
    objects.replace(index, value);
    p.insert(u"objects"_s, objects);
    pages.replace(page, p);
    root.insert(u"pages"_s, pages);
    return root;
}

} // namespace

// document.json (DA3) : parité écrivain/lecteur, clés figées, lecture tolérante, version refusée.
class TstFormatsNativeJson : public QObject
{
    Q_OBJECT

private slots:
    void roundTripKeepsTheFingerprint()
    {
        const core::Document rich = richdoc::richDocument();
        QVERIFY(core::hasUniqueIds(rich));
        core::Document back;
        const formats::JsonReadResult r = formats::documentFromJson(formats::documentToJson(rich), loaderFrom(rich), back);
        QVERIFY2(r.ok, qPrintable(r.error));
        QVERIFY2(r.warnings.isEmpty(), qPrintable(r.warnings.join(u'\n')));
        QCOMPARE(core::fingerprint(back), core::fingerprint(rich));
        QCOMPARE(back.nextId(), core::ObjectId{20});
        QCOMPARE(back.resources().size(), std::size_t{2});
        QCOMPARE(back.pageCount(), std::size_t{3});
    }

    void roundTripThroughTextKeepsTheFingerprint()
    {
        const core::Document rich = richdoc::richDocument();
        const QByteArray text = QJsonDocument(formats::documentToJson(rich)).toJson(QJsonDocument::Indented);
        core::Document back;
        const auto r = formats::documentFromJson(QJsonDocument::fromJson(text).object(), loaderFrom(rich), back);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(core::fingerprint(back), core::fingerprint(rich));
    }

    void keysAreFrozen()
    {
        const QJsonObject root = formats::documentToJson(richdoc::richDocument());
        QCOMPARE(root.value(u"format"_s).toString(), u"actcraft-document/1"_s);
        QCOMPARE(root.value(u"title"_s).toString(), u"Document riche"_s);
        QCOMPARE(root.value(u"nextId"_s).toDouble(), 20.0);
        QCOMPARE(root.value(u"resources"_s).toArray().at(0).toObject().value(u"extension"_s).toString().size(), 3);
        const QJsonObject rect = object(root, 0, 0);
        QCOMPARE(rect.value(u"type"_s).toString(), u"shape"_s);
        QCOMPARE(rect.value(u"shape"_s).toString(), u"rectangle"_s);
        QCOMPARE(rect.value(u"layer"_s).toString(), u"bottom"_s);
        QCOMPARE(rect.value(u"fill"_s).toString(), u"#ff336699"_s);
        QCOMPARE(rect.value(u"transform"_s).toArray().size(), 6);
        QCOMPARE(rect.value(u"restrictions"_s).toObject().value(u"move"_s).toBool(true), false);
        QCOMPARE(rect.value(u"actions"_s).toArray().at(0).toObject().value(u"kind"_s).toString(), u"onObject"_s);
        const QJsonObject path = object(root, 0, 3);
        QCOMPARE(path.value(u"shape"_s).toString(), u"path"_s);
        QCOMPARE(path.value(u"pen"_s).toObject().value(u"style"_s).toString(), u"dash"_s);
        QCOMPARE(path.value(u"brush"_s).toObject().value(u"gradient"_s).toString(), u"radial"_s);
        QCOMPARE(object(root, 0, 5).value(u"type"_s).toString(), u"text"_s);
        QCOMPARE(object(root, 0, 7).value(u"members"_s).toArray().size(), 2);
        const QJsonObject stroke = object(root, 0, 9);
        QCOMPARE(stroke.value(u"type"_s).toString(), u"stroke"_s);
        QCOMPARE(stroke.value(u"kind"_s).toString(), u"highlighter"_s);
        QCOMPARE(stroke.value(u"cap"_s).toString(), u"round"_s);
        QCOMPARE(stroke.value(u"points"_s).toArray().size(), 9); // x, y, pression × 3
        const QJsonObject page2 = root.value(u"pages"_s).toArray().at(1).toObject();
        QCOMPARE(page2.value(u"background"_s).toObject().value(u"fill"_s).toObject().value(u"gradient"_s).toString(),
                 u"diagonalUp"_s);
        QCOMPARE(page2.value(u"grid"_s).toObject().value(u"levels"_s).toArray().at(1).toObject()
                     .value(u"line"_s).toString(),
                 u"dotted"_s);
    }

    // Garde-fou de parité (relecture du plan, tour 1) : chaque champ du modèle a, dans richDocument, une valeur hors
    // défaut. Un champ ajouté au modèle casse ces liaisons structurées à la compilation : il faut alors le remplir
    // ici, et roundTripKeepsTheFingerprint prouve qu'il traverse document.json (l'empreinte couvre tous les champs).
    void richDocumentFillsEveryField()
    {
        using namespace actcraft::core;
        const Document d = richdoc::richDocument();
        const Page& p1 = d.page(0);
        {
            const auto& [id, name, layer, visible, locked, translucency, restrictions, transform, label, actions] =
                p1.objects[0].common;
            QVERIFY(id != 0 && !name.empty() && layer != Layer::Middle && !visible && locked && translucency != 0);
            const auto& [movable, resizable, rotatable] = restrictions;
            QVERIFY(!movable && !resizable && !rotatable);
            const auto& [m11, m12, m21, m22, dx, dy] = transform;
            QVERIFY(m11 != 1 && m12 != 0 && m21 != 0 && m22 != 1 && dx != 0 && dy != 0);
            QVERIFY(!label.empty() && actions.size() == 1);
            const auto& [kind, sourceType, command, target, resource] = actions[0];
            QVERIFY(kind != ActionKind::Other && sourceType != 0 && command != 0 && target != 0 && !resource.empty());
        }
        {
            const auto& [kind, bounds, polygon, fill, outline, outlineWidth, opacity, paths, open, windingFill, pen,
                         brush] = std::get<Shape>(p1.objects[3]);
            QVERIFY(kind != ShapeKind::Rectangle && bounds.width != 0 && !polygon.empty() && fill != 0xFFFFFFFFu &&
                    outline != 0xFF000000u && outlineWidth != 1 && opacity != 1 && !paths.empty() && open &&
                    windingFill);
            const auto& [style, color, width, cap, join] = pen;
            QVERIFY(style != PenStyle::Solid && color != 0xFF000000u && width != 1 && cap != PenCap::Round &&
                    join != PenJoin::Round);
            const auto& [fillKind, color1, color2, gradient, pattern, opaque, background] = brush;
            QVERIFY(fillKind != FillKind::None && color1 != 0xFFFFFFFFu && color2 != 0xFFFFFFFFu &&
                    gradient != GradientKind::Vertical && pattern != 0 && opaque && background != 0xFF000000u);
        }
        {
            const auto& [bounds, sw, sh, pattern, a, b, seed, resource, hasTransparent, transparent] =
                std::get<ImageObject>(p1.objects[2]);
            QVERIFY(bounds.width != 0 && sw != 256 && sh != 256 && pattern != ImagePattern::LinearGradient &&
                    a != 0xFF000000u && b != 0xFFFFFFFFu && seed != 0 && !resource.empty() && hasTransparent &&
                    transparent != 0);
        }
        {
            const auto& [points, baseWidth, color, kind, cap] = std::get<Stroke>(p1.objects[9]);
            QVERIFY(points.size() == 3 && points[0].pressure != 0.5f && baseWidth != 3 && color != 0xFF000000u &&
                    kind != StrokeKind::Pen && cap != PenCap::Square);
        }
        {
            const auto& [points, pen] = std::get<Line>(p1.objects[4]);
            QVERIFY(!points.empty() && pen.style != PenStyle::Solid);
        }
        {
            const auto& [bounds, html, runs] = std::get<Text>(p1.objects[5]);
            QVERIFY(bounds.width != 0 && !html.empty() && !runs.empty());
            const auto& [text, family, size, weight, italic, underline, strikeOut, color, baseline, runBounds] =
                runs[0];
            QVERIFY(!text.empty() && !family.empty() && size != 12 && weight != 400 && italic && underline &&
                    strikeOut && color != 0xFF000000u && baseline.y != 0 && runBounds.width != 0);
        }
        {
            const auto& [bounds, placeholder, media, fileName, playOnOpen, loop] = std::get<Media>(p1.objects[6]);
            QVERIFY(bounds.width != 0 && !placeholder.empty() && !media.empty() && !fileName.empty() && playOnOpen &&
                    loop);
        }
        {
            const auto& [members] = std::get<Group>(p1.objects[7]);
            QVERIFY(!members.empty());
        }
        {
            const auto& [firstEnd, secondEnd, objectA, objectB, pen] = std::get<Connector>(p1.objects[8]);
            QVERIFY(firstEnd.x != 0 && secondEnd.x != 0 && objectA != 0 && objectB != 0 && pen.color != 0xFF000000u);
        }
        {
            const auto& [width, height, objects, background, grid, name, notes] = d.page(1);
            QVERIFY(width != 1024 && height != 768 && objects.empty() && !name.empty() && !notes.empty());
            const auto& [fill, image] = background;
            QVERIFY(fill.kind != FillKind::Solid && !image.empty());
            const auto& [visible, snapEnabled, drawnAbove, previewZoom, baseZoom, zoomIncrement, levels] = grid;
            QVERIFY(visible && snapEnabled && drawnAbove && previewZoom != 1 && baseZoom != 1 && zoomIncrement != 0.5 &&
                    levels.size() == 2);
            const auto& [lvVisible, snap, start, spacingX, spacingY, color, pattern, angle, line, linesAlongX, linesAlongY] = levels[1];
            QVERIFY(!lvVisible && !snap && start.x != 0 && spacingX != 32 && spacingY != 32 && color != 0xFF000000u &&
                    pattern != GridPattern::Normal && angle != 30 && line != GridLine::Solid && !linesAlongX && !linesAlongY);
        }
        QVERIFY(!d.title().empty() && d.resources().size() == 2 && d.nextId() == 20);
    }

    void hugeOrNonFiniteNumbersAreRefused()
    {
        const core::Document rich = richdoc::richDocument();
        QJsonObject o = object(formats::documentToJson(rich), 0, 1); // image procédurale
        o.insert(u"sourceWidth"_s, 1e30);
        o.insert(u"seed"_s, -5);
        core::Document back;
        const auto r = formats::documentFromJson(withObject(formats::documentToJson(rich), 0, 1, o), loaderFrom(rich), back);
        QVERIFY(r.ok);
        QCOMPARE(r.warnings.size(), 2);
        const auto* image = std::get_if<core::ImageObject>(&back.page(0).objects[1]);
        QVERIFY(image);
        QCOMPARE(image->sourceWidth, std::int32_t{256}); // hors limites : défaut du modèle, pas 1e30 tronqué
        QCOMPARE(image->seed, std::uint32_t{0});          // négatif pour un non signé : défaut du modèle
    }

    void unsafeResourceNamesAreRefused()
    {
        const core::Document rich = richdoc::richDocument();
        QJsonObject root = formats::documentToJson(rich);
        QJsonArray resources = root.value(u"resources"_s).toArray();
        QJsonObject first = resources.at(0).toObject();
        first.insert(u"extension"_s, u"../../x"_s);
        resources.replace(0, first);
        QJsonObject second = resources.at(1).toObject();
        second.insert(u"key"_s, u"../cle"_s);
        resources.replace(1, second);
        root.insert(u"resources"_s, resources);
        core::Document back;
        bool loaderCalled = false;
        const auto r = formats::documentFromJson(
            root, [&loaderCalled](const std::string&, const std::string&) -> std::optional<QByteArray> {
                loaderCalled = true;
                return std::nullopt;
            },
            back);
        QVERIFY(r.ok);
        QVERIFY(!loaderCalled); // un nom invalide n'atteint jamais l'archive
        QVERIFY(r.warnings.join(u' ').contains(u"nom invalide"_s));
        QCOMPARE(back.resources().size(), std::size_t{0});
    }

    void unknownKeysAreIgnoredSilently()
    {
        const core::Document rich = richdoc::richDocument();
        QJsonObject root = formats::documentToJson(rich);
        root.insert(u"future"_s, u"plus tard"_s);
        QJsonObject o = object(root, 0, 0);
        o.insert(u"future"_s, 3);
        root = withObject(root, 0, 0, o);
        core::Document back;
        const auto r = formats::documentFromJson(root, loaderFrom(rich), back);
        QVERIFY(r.ok);
        QVERIFY(r.warnings.isEmpty());
        QCOMPARE(core::fingerprint(back), core::fingerprint(rich));
    }

    void unknownObjectTypeIsSkippedWithAWarning()
    {
        const core::Document rich = richdoc::richDocument();
        QJsonObject o = object(formats::documentToJson(rich), 0, 1);
        o.insert(u"type"_s, u"hologram"_s);
        core::Document back;
        const auto r = formats::documentFromJson(withObject(formats::documentToJson(rich), 0, 1, o), loaderFrom(rich), back);
        QVERIFY(r.ok);
        QVERIFY(r.warnings.join(u' ').contains(u"hologram"_s));
        QCOMPARE(back.page(0).objects.size(), std::size_t{9});
    }

    void otherMajorVersionIsRefusedAndTargetUntouched()
    {
        const core::Document rich = richdoc::richDocument();
        QJsonObject root = formats::documentToJson(rich);
        root.insert(u"format"_s, u"actcraft-document/2"_s);
        core::Document target(640, 480);
        const std::uint64_t before = core::fingerprint(target);
        const auto r = formats::documentFromJson(root, loaderFrom(rich), target);
        QVERIFY(!r.ok);
        QVERIFY2(r.error.contains(u"version"_s), qPrintable(r.error));
        QCOMPARE(core::fingerprint(target), before);
        root.remove(u"format"_s);
        QVERIFY(!formats::documentFromJson(root, loaderFrom(rich), target).ok);
        root.insert(u"format"_s, u"autre-chose/1"_s);
        QVERIFY(!formats::documentFromJson(root, loaderFrom(rich), target).ok);
    }

    void missingOrTamperedResourceIsAWarning()
    {
        const core::Document rich = richdoc::richDocument();
        core::Document back;
        auto r = formats::documentFromJson(
            formats::documentToJson(rich), [](const std::string&, const std::string&) { return std::nullopt; }, back);
        QVERIFY(r.ok);
        QCOMPARE(r.warnings.size(), 2);
        QCOMPARE(back.resources().size(), std::size_t{0});
        r = formats::documentFromJson(
            formats::documentToJson(rich),
            [](const std::string&, const std::string&) -> std::optional<QByteArray> { return QByteArray("autre"); },
            back);
        QVERIFY(r.ok);
        QVERIFY(r.warnings.join(u' ').contains(u"empreinte"_s));
        QCOMPARE(back.resources().size(), std::size_t{0});
    }

    void malformedValuesFallBackWithWarnings()
    {
        const core::Document rich = richdoc::richDocument();
        QJsonObject o = object(formats::documentToJson(rich), 0, 4); // ligne
        o.insert(u"points"_s, QJsonArray{1, 2, 3});
        QJsonObject pen = o.value(u"pen"_s).toObject();
        pen.insert(u"color"_s, u"rouge"_s);
        o.insert(u"pen"_s, pen);
        core::Document back;
        const auto r = formats::documentFromJson(withObject(formats::documentToJson(rich), 0, 4, o), loaderFrom(rich), back);
        QVERIFY(r.ok);
        QCOMPARE(r.warnings.size(), 2);
        const auto* line = std::get_if<core::Line>(&back.page(0).objects[4]);
        QVERIFY(line);
        QCOMPARE(line->points.size(), std::size_t{1});
        QCOMPARE(line->pen.color, core::Outline{}.color);
    }

    void noPageGivesOneBlankPage()
    {
        QJsonObject root{{u"format"_s, u"actcraft-document/1"_s}, {u"pages"_s, QJsonArray{}}};
        core::Document back;
        const auto r = formats::documentFromJson(root, [](const std::string&, const std::string&) { return std::nullopt; }, back);
        QVERIFY(r.ok);
        QCOMPARE(back.pageCount(), std::size_t{1});
        QVERIFY(!r.warnings.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TstFormatsNativeJson)
#include "tst_formats_nativejson.moc"
```

  Les lambdas qui rendent `std::nullopt` sans type de retour explicite sont converties en `ResourceLoader` ; si MSVC
  refuse la déduction, écrire `-> std::optional<QByteArray>` comme dans la deuxième.

  Ajouter à `tests/formats.cmake` :

```cmake
actcraft_add_test(tst_formats_nativejson
    SOURCES tst_formats_nativejson.cpp
    LIBS ActCraft::formats)
```

- [ ] **Étape 3 : constater le rouge.** Commande ciblée, motif `tst_formats_nativejson` → `error`
  (`actcraft/formats/nativejson.h` introuvable).

- [ ] **Étape 4 : écrire l'en-tête.** Créer `formats/include/actcraft/formats/nativejson.h` :

```cpp
#pragma once

#include <actcraft/core/document.h>

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <functional>
#include <optional>
#include <string>

namespace actcraft::formats {

// Format natif (DA3, décision 17) : « document.json » de l'archive .actcraft. Clés en anglais, figées par ce
// fichier et par tst_formats_nativejson. Tout changement incompatible passe par « actcraft-document/2 » et un
// lecteur des deux versions.
inline constexpr char kNativeFormat[] = "actcraft-document/1";
inline constexpr int kNativeMajorVersion = 1;

QJsonObject documentToJson(const core::Document& document);

// Octets d'une ressource déclarée (clé SHA-256, extension) ; nullopt si l'archive ne les a pas.
using ResourceLoader =
    std::function<std::optional<QByteArray>(const std::string& key, const std::string& extension)>;

struct JsonReadResult
{
    bool ok = false;
    QString error;        // si !ok
    QStringList warnings; // valeurs remplacées par défaut, objets sautés, ressources écartées
};

// Remplace `out` par le document lu. Tolérant (clés inconnues ignorées, valeurs absentes = défauts, objet de type
// inconnu sauté, valeur mal formée remplacée avec un avertissement) ; refuse un « format » absent, étranger ou
// d'une autre version majeure, sans toucher à `out`. Une ressource dont les octets ne redonnent pas la clé est
// écartée (avertissement).
JsonReadResult documentFromJson(const QJsonObject& json, const ResourceLoader& loadResource, core::Document& out);

} // namespace actcraft::formats
```

- [ ] **Étape 5 : écrire l'implémentation.** Créer `formats/src/nativejson.cpp` :

```cpp
#include <actcraft/formats/nativejson.h>

#include <QJsonArray>
#include <QJsonValue>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <regex>
#include <utility>

namespace actcraft::formats {

using namespace Qt::StringLiterals;
using namespace actcraft::core;

namespace {

template <typename E>
struct Named
{
    E value;
    const char* name;
};

constexpr std::array<Named<Layer>, 4> kLayers{
    {{Layer::Background, "background"}, {Layer::Bottom, "bottom"}, {Layer::Middle, "middle"}, {Layer::Top, "top"}}};
constexpr std::array<Named<PenStyle>, 6> kPenStyles{{{PenStyle::None, "none"},
                                                     {PenStyle::Solid, "solid"},
                                                     {PenStyle::Dash, "dash"},
                                                     {PenStyle::Dot, "dot"},
                                                     {PenStyle::DashDot, "dashDot"},
                                                     {PenStyle::DashDotDot, "dashDotDot"}}};
constexpr std::array<Named<PenCap>, 3> kPenCaps{
    {{PenCap::Flat, "flat"}, {PenCap::Square, "square"}, {PenCap::Round, "round"}}};
constexpr std::array<Named<PenJoin>, 3> kPenJoins{
    {{PenJoin::Miter, "miter"}, {PenJoin::Bevel, "bevel"}, {PenJoin::Round, "round"}}};
constexpr std::array<Named<FillKind>, 4> kFillKinds{{{FillKind::None, "none"},
                                                     {FillKind::Solid, "solid"},
                                                     {FillKind::Pattern, "pattern"},
                                                     {FillKind::Gradient, "gradient"}}};
constexpr std::array<Named<GradientKind>, 5> kGradients{{{GradientKind::Vertical, "vertical"},
                                                         {GradientKind::Horizontal, "horizontal"},
                                                         {GradientKind::DiagonalDown, "diagonalDown"},
                                                         {GradientKind::DiagonalUp, "diagonalUp"},
                                                         {GradientKind::Radial, "radial"}}};
constexpr std::array<Named<ActionKind>, 4> kActionKinds{{{ActionKind::Other, "other"},
                                                         {ActionKind::Navigate, "navigate"},
                                                         {ActionKind::OpenExternal, "openExternal"},
                                                         {ActionKind::OnObject, "onObject"}}};
constexpr std::array<Named<ObjectKind>, 8> kObjectKinds{{{ObjectKind::Shape, "shape"},
                                                         {ObjectKind::Image, "image"},
                                                         {ObjectKind::Stroke, "stroke"},
                                                         {ObjectKind::Line, "line"},
                                                         {ObjectKind::Text, "text"},
                                                         {ObjectKind::Media, "media"},
                                                         {ObjectKind::Group, "group"},
                                                         {ObjectKind::Connector, "connector"}}};
constexpr std::array<Named<ShapeKind>, 4> kShapeKinds{{{ShapeKind::Rectangle, "rectangle"},
                                                       {ShapeKind::Ellipse, "ellipse"},
                                                       {ShapeKind::Polygon, "polygon"},
                                                       {ShapeKind::Path, "path"}}};
constexpr std::array<Named<ImagePattern>, 4> kPatterns{{{ImagePattern::LinearGradient, "linearGradient"},
                                                        {ImagePattern::Checker, "checker"},
                                                        {ImagePattern::Rings, "rings"},
                                                        {ImagePattern::SolidCenter, "solidCenter"}}};
constexpr std::array<Named<StrokeKind>, 3> kStrokeKinds{
    {{StrokeKind::Pen, "pen"}, {StrokeKind::Highlighter, "highlighter"}, {StrokeKind::Revealer, "revealer"}}};
constexpr std::array<Named<GridPattern>, 3> kGridPatterns{
    {{GridPattern::Normal, "normal"}, {GridPattern::Diagonal45, "diagonal45"}, {GridPattern::Angle, "angle"}}};
constexpr std::array<Named<GridLine>, 6> kGridLines{{{GridLine::Points, "points"},
                                                     {GridLine::Cross, "cross"},
                                                     {GridLine::Solid, "solid"},
                                                     {GridLine::Dotted, "dotted"},
                                                     {GridLine::Dashed, "dashed"},
                                                     {GridLine::BigDots, "bigDots"}}};

template <typename E, std::size_t N>
QString nameOf(const std::array<Named<E>, N>& table, E value)
{
    for (const auto& entry : table) {
        if (entry.value == value) {
            return QString::fromLatin1(entry.name);
        }
    }
    return QString::fromLatin1(table[0].name);
}

QString str(const std::string& s)
{
    return QString::fromStdString(s);
}

QString colorText(Argb c)
{
    return u"#%1"_s.arg(c, 8, 16, QLatin1Char('0'));
}

double idValue(ObjectId id)
{
    return static_cast<double>(id);
}

QJsonArray pairs(const std::vector<PointF>& points)
{
    QJsonArray a;
    for (const PointF& p : points) {
        a.append(p.x);
        a.append(p.y);
    }
    return a;
}

QJsonArray rectArray(const RectF& r)
{
    return QJsonArray{r.x, r.y, r.width, r.height};
}

QJsonArray pointArray(const PointF& p)
{
    return QJsonArray{p.x, p.y};
}

QJsonObject outlineJson(const Outline& o)
{
    QJsonObject j;
    j.insert(u"style"_s, nameOf(kPenStyles, o.style));
    j.insert(u"color"_s, colorText(o.color));
    j.insert(u"width"_s, o.width);
    j.insert(u"cap"_s, nameOf(kPenCaps, o.cap));
    j.insert(u"join"_s, nameOf(kPenJoins, o.join));
    return j;
}

QJsonObject fillJson(const Fill& f)
{
    QJsonObject j;
    j.insert(u"kind"_s, nameOf(kFillKinds, f.kind));
    j.insert(u"color"_s, colorText(f.color));
    j.insert(u"color2"_s, colorText(f.color2));
    j.insert(u"gradient"_s, nameOf(kGradients, f.gradient));
    j.insert(u"pattern"_s, static_cast<int>(f.pattern));
    j.insert(u"opaqueBackground"_s, f.opaqueBackground);
    j.insert(u"backgroundColor"_s, colorText(f.backgroundColor));
    return j;
}

void writeCommon(QJsonObject& j, const ObjectCommon& c)
{
    j.insert(u"id"_s, idValue(c.id));
    j.insert(u"name"_s, str(c.name));
    j.insert(u"layer"_s, nameOf(kLayers, c.layer));
    j.insert(u"visible"_s, c.visible);
    j.insert(u"locked"_s, c.locked);
    j.insert(u"translucency"_s, static_cast<int>(c.translucency));
    j.insert(u"restrictions"_s, QJsonObject{{u"move"_s, c.restrictions.movable},
                                            {u"size"_s, c.restrictions.resizable},
                                            {u"rotate"_s, c.restrictions.rotatable}});
    const Transform& t = c.transform;
    j.insert(u"transform"_s, QJsonArray{t.m11, t.m12, t.m21, t.m22, t.dx, t.dy});
    j.insert(u"label"_s, str(c.label));
    QJsonArray actions;
    for (const Action& a : c.actions) {
        actions.append(QJsonObject{{u"kind"_s, nameOf(kActionKinds, a.kind)},
                                   {u"sourceType"_s, static_cast<int>(a.sourceType)},
                                   {u"command"_s, static_cast<int>(a.command)},
                                   {u"target"_s, idValue(a.target)},
                                   {u"resource"_s, str(a.resource)}});
    }
    j.insert(u"actions"_s, actions);
}

QJsonObject objectJson(const PageObject& object)
{
    QJsonObject j;
    j.insert(u"type"_s, nameOf(kObjectKinds, kindOf(object)));
    writeCommon(j, object.common);
    if (const auto* s = std::get_if<Shape>(&object)) {
        j.insert(u"shape"_s, nameOf(kShapeKinds, s->kind));
        j.insert(u"bounds"_s, rectArray(s->bounds));
        j.insert(u"polygon"_s, pairs(s->polygon));
        j.insert(u"fill"_s, colorText(s->fill));
        j.insert(u"outline"_s, colorText(s->outline));
        j.insert(u"outlineWidth"_s, s->outlineWidth);
        j.insert(u"opacity"_s, s->opacity);
        QJsonArray paths;
        for (const auto& path : s->paths) {
            paths.append(pairs(path));
        }
        j.insert(u"paths"_s, paths);
        j.insert(u"open"_s, s->open);
        j.insert(u"windingFill"_s, s->windingFill);
        j.insert(u"pen"_s, outlineJson(s->pen));
        j.insert(u"brush"_s, fillJson(s->brush));
    } else if (const auto* im = std::get_if<ImageObject>(&object)) {
        j.insert(u"bounds"_s, rectArray(im->bounds));
        j.insert(u"sourceWidth"_s, im->sourceWidth);
        j.insert(u"sourceHeight"_s, im->sourceHeight);
        j.insert(u"pattern"_s, nameOf(kPatterns, im->pattern));
        j.insert(u"colorA"_s, colorText(im->colorA));
        j.insert(u"colorB"_s, colorText(im->colorB));
        j.insert(u"seed"_s, static_cast<double>(im->seed));
        j.insert(u"resource"_s, str(im->resource));
        j.insert(u"keyColor"_s, im->hasKeyColor ? QJsonValue(colorText(im->keyColor))
                                                                 : QJsonValue(QJsonValue::Null));
    } else if (const auto* st = std::get_if<Stroke>(&object)) {
        QJsonArray points;
        for (const StrokePoint& p : st->points) {
            points.append(p.x);
            points.append(p.y);
            points.append(static_cast<double>(p.pressure));
        }
        j.insert(u"points"_s, points);
        j.insert(u"baseWidth"_s, st->baseWidth);
        j.insert(u"color"_s, colorText(st->color));
        j.insert(u"kind"_s, nameOf(kStrokeKinds, st->kind));
        j.insert(u"cap"_s, nameOf(kPenCaps, st->cap));
    } else if (const auto* l = std::get_if<Line>(&object)) {
        j.insert(u"points"_s, pairs(l->points));
        j.insert(u"pen"_s, outlineJson(l->pen));
    } else if (const auto* t = std::get_if<Text>(&object)) {
        j.insert(u"bounds"_s, rectArray(t->bounds));
        j.insert(u"html"_s, str(t->html));
        QJsonArray runs;
        for (const TextSpan& r : t->runs) {
            runs.append(QJsonObject{{u"text"_s, str(r.text)},
                                    {u"fontFamily"_s, str(r.fontFamily)},
                                    {u"pointSize"_s, r.pointSize},
                                    {u"weight"_s, r.weight},
                                    {u"italic"_s, r.italic},
                                    {u"underline"_s, r.underline},
                                    {u"strikeOut"_s, r.strikeOut},
                                    {u"color"_s, colorText(r.color)},
                                    {u"baseline"_s, pointArray(r.baseline)},
                                    {u"bounds"_s, rectArray(r.bounds)}});
        }
        j.insert(u"runs"_s, runs);
    } else if (const auto* m = std::get_if<Media>(&object)) {
        j.insert(u"bounds"_s, rectArray(m->bounds));
        j.insert(u"placeholder"_s, str(m->placeholder));
        j.insert(u"media"_s, str(m->media));
        j.insert(u"fileName"_s, str(m->fileName));
        j.insert(u"playOnOpen"_s, m->playOnOpen);
        j.insert(u"loop"_s, m->loop);
    } else if (const auto* g = std::get_if<Group>(&object)) {
        QJsonArray members;
        for (const ObjectId id : g->members) {
            members.append(idValue(id));
        }
        j.insert(u"members"_s, members);
    } else if (const auto* c = std::get_if<Connector>(&object)) {
        j.insert(u"firstEnd"_s, pointArray(c->firstEnd));
        j.insert(u"secondEnd"_s, pointArray(c->secondEnd));
        j.insert(u"objectA"_s, idValue(c->objectA));
        j.insert(u"objectB"_s, idValue(c->objectB));
        j.insert(u"pen"_s, outlineJson(c->pen));
    }
    return j;
}

QJsonObject pageJson(const Page& p)
{
    QJsonObject j;
    j.insert(u"width"_s, p.width);
    j.insert(u"height"_s, p.height);
    j.insert(u"name"_s, str(p.name));
    j.insert(u"notes"_s, str(p.notes));
    j.insert(u"background"_s,
             QJsonObject{{u"fill"_s, fillJson(p.background.fill)}, {u"image"_s, str(p.background.image)}});
    QJsonArray levels;
    for (const GridLevel& l : p.grid.levels) {
        levels.append(QJsonObject{{u"visible"_s, l.visible},
                                  {u"snap"_s, l.snap},
                                  {u"start"_s, pointArray(l.start)},
                                  {u"spacingX"_s, l.spacingX},
                                  {u"spacingY"_s, l.spacingY},
                                  {u"color"_s, colorText(l.color)},
                                  {u"pattern"_s, nameOf(kGridPatterns, l.pattern)},
                                  {u"angle"_s, l.angle},
                                  {u"line"_s, nameOf(kGridLines, l.line)},
                                  {u"linesAlongX"_s, l.linesAlongX},
                                  {u"linesAlongY"_s, l.linesAlongY}});
    }
    j.insert(u"grid"_s, QJsonObject{{u"visible"_s, p.grid.visible},
                                    {u"snapEnabled"_s, p.grid.snapEnabled},
                                    {u"drawnAbove"_s, p.grid.drawnAbove},
                                    {u"previewZoom"_s, p.grid.previewZoom},
                                    {u"baseZoom"_s, p.grid.baseZoom},
                                    {u"zoomIncrement"_s, p.grid.zoomIncrement},
                                    {u"levels"_s, levels}});
    QJsonArray objects;
    for (const PageObject& o : p.objects) {
        objects.append(objectJson(o));
    }
    j.insert(u"objects"_s, objects);
    return j;
}

// Lecture : chaque valeur absente garde le défaut du modèle ; une valeur présente mais mal formée aussi, avec un
// avertissement qui nomme où.
class Reader
{
public:
    QStringList warnings;

    template <typename E, std::size_t N>
    E pick(const std::array<Named<E>, N>& table, const QJsonValue& v, E fallback, const QString& where)
    {
        if (v.isUndefined()) {
            return fallback;
        }
        const QString text = v.toString();
        for (const auto& entry : table) {
            if (text == QLatin1String(entry.name)) {
                return entry.value;
            }
        }
        warn(where, u"valeur inconnue « %1 »"_s.arg(text));
        return fallback;
    }

    double number(const QJsonObject& o, QStringView key, double fallback, const QString& where)
    {
        const QJsonValue v = o.value(key);
        if (v.isUndefined()) {
            return fallback;
        }
        if (!v.isDouble() || !std::isfinite(v.toDouble())) {
            warn(where, u"« %1 » n'est pas un nombre"_s.arg(key));
            return fallback;
        }
        return v.toDouble();
    }

    bool flag(const QJsonObject& o, QStringView key, bool fallback, const QString& where)
    {
        const QJsonValue v = o.value(key);
        if (v.isUndefined()) {
            return fallback;
        }
        if (!v.isBool()) {
            warn(where, u"« %1 » n'est pas un booléen"_s.arg(key));
            return fallback;
        }
        return v.toBool();
    }

    // Entier lu d'un réel : hors des bornes du type (1e30, négatif pour un non signé), défaut et avertissement.
    template <typename T> T integer(const QJsonObject& o, QStringView key, T fallback, const QString& where)
    {
        const double v = number(o, key, static_cast<double>(fallback), where);
        if (v < static_cast<double>(std::numeric_limits<T>::lowest()) ||
            v > static_cast<double>(std::numeric_limits<T>::max())) {
            warn(where, u"« %1 » hors limites"_s.arg(key));
            return fallback;
        }
        return static_cast<T>(v);
    }

    std::string text(const QJsonObject& o, QStringView key)
    {
        return o.value(key).toString().toStdString();
    }

    Argb color(const QJsonObject& o, QStringView key, Argb fallback, const QString& where)
    {
        const QJsonValue v = o.value(key);
        if (v.isUndefined()) {
            return fallback;
        }
        const QString s = v.toString();
        bool ok = false;
        const uint value = s.size() == 9 && s.startsWith(u'#') ? s.mid(1).toUInt(&ok, 16) : 0u;
        if (!ok) {
            warn(where, u"couleur « %1 » invalide"_s.arg(s));
            return fallback;
        }
        return static_cast<Argb>(value);
    }

    ObjectId id(const QJsonValue& v)
    {
        const double d = v.toDouble(0);
        if (d < 0 || d != std::floor(d) || d > 9007199254740992.0) {
            return 0;
        }
        return static_cast<ObjectId>(d);
    }

    std::vector<PointF> pairsOf(const QJsonValue& v, const QString& where)
    {
        std::vector<PointF> out;
        const QJsonArray a = v.toArray();
        if (a.size() % 2 != 0) {
            warn(where, u"nombre impair de coordonnées, la dernière est ignorée"_s);
        }
        for (qsizetype i = 0; i + 1 < a.size(); i += 2) {
            out.push_back(PointF{a.at(i).toDouble(), a.at(i + 1).toDouble()});
        }
        return out;
    }

    RectF rect(const QJsonValue& v, RectF fallback, const QString& where)
    {
        if (v.isUndefined()) {
            return fallback;
        }
        const QJsonArray a = v.toArray();
        if (a.size() != 4) {
            warn(where, u"rectangle mal formé"_s);
            return fallback;
        }
        return RectF{a.at(0).toDouble(), a.at(1).toDouble(), a.at(2).toDouble(), a.at(3).toDouble()};
    }

    PointF point(const QJsonValue& v, PointF fallback, const QString& where)
    {
        if (v.isUndefined()) {
            return fallback;
        }
        const QJsonArray a = v.toArray();
        if (a.size() != 2) {
            warn(where, u"point mal formé"_s);
            return fallback;
        }
        return PointF{a.at(0).toDouble(), a.at(1).toDouble()};
    }

    Outline outline(const QJsonValue& v, const QString& where)
    {
        Outline o;
        const QJsonObject j = v.toObject();
        o.style = pick(kPenStyles, j.value(u"style"_s), o.style, where);
        o.color = color(j, u"color", o.color, where);
        o.width = number(j, u"width", o.width, where);
        o.cap = pick(kPenCaps, j.value(u"cap"_s), o.cap, where);
        o.join = pick(kPenJoins, j.value(u"join"_s), o.join, where);
        return o;
    }

    Fill fill(const QJsonValue& v, Fill f, const QString& where)
    {
        if (!v.isObject()) {
            return f;
        }
        const QJsonObject j = v.toObject();
        f.kind = pick(kFillKinds, j.value(u"kind"_s), f.kind, where);
        f.color = color(j, u"color", f.color, where);
        f.color2 = color(j, u"color2", f.color2, where);
        f.gradient = pick(kGradients, j.value(u"gradient"_s), f.gradient, where);
        f.pattern = static_cast<std::uint8_t>(std::clamp(number(j, u"pattern", f.pattern, where), 0.0, 255.0));
        f.opaqueBackground = flag(j, u"opaqueBackground", f.opaqueBackground, where);
        f.backgroundColor = color(j, u"backgroundColor", f.backgroundColor, where);
        return f;
    }

    void readCommon(const QJsonObject& j, ObjectCommon& c, const QString& where)
    {
        c.id = id(j.value(u"id"_s));
        c.name = text(j, u"name");
        c.layer = pick(kLayers, j.value(u"layer"_s), c.layer, where);
        c.visible = flag(j, u"visible", c.visible, where);
        c.locked = flag(j, u"locked", c.locked, where);
        c.translucency =
            static_cast<std::uint8_t>(std::clamp(number(j, u"translucency", c.translucency, where), 0.0, 255.0));
        const QJsonObject r = j.value(u"restrictions"_s).toObject();
        c.restrictions.movable = flag(r, u"move", true, where);
        c.restrictions.resizable = flag(r, u"size", true, where);
        c.restrictions.rotatable = flag(r, u"rotate", true, where);
        const QJsonArray t = j.value(u"transform"_s).toArray();
        if (t.size() == 6) {
            c.transform = Transform{t.at(0).toDouble(), t.at(1).toDouble(), t.at(2).toDouble(),
                                    t.at(3).toDouble(), t.at(4).toDouble(), t.at(5).toDouble()};
        } else if (!j.value(u"transform"_s).isUndefined()) {
            warn(where, u"transformation mal formée"_s);
        }
        c.label = text(j, u"label");
        for (const QJsonValue& av : j.value(u"actions"_s).toArray()) {
            const QJsonObject a = av.toObject();
            Action action;
            action.kind = pick(kActionKinds, a.value(u"kind"_s), action.kind, where);
            action.sourceType =
                static_cast<std::uint16_t>(std::clamp(number(a, u"sourceType", 0, where), 0.0, 65535.0));
            action.command = integer<std::int32_t>(a, u"command", 0, where);
            action.target = id(a.value(u"target"_s));
            action.resource = text(a, u"resource");
            c.actions.push_back(std::move(action));
        }
    }

    std::optional<PageObject> object(const QJsonObject& j, const QString& where)
    {
        const QString type = j.value(u"type"_s).toString();
        PageObject out;
        if (type == u"shape"_s) {
            Shape s;
            s.kind = pick(kShapeKinds, j.value(u"shape"_s), s.kind, where);
            s.bounds = rect(j.value(u"bounds"_s), s.bounds, where);
            s.polygon = pairsOf(j.value(u"polygon"_s), where);
            s.fill = color(j, u"fill", s.fill, where);
            s.outline = color(j, u"outline", s.outline, where);
            s.outlineWidth = number(j, u"outlineWidth", s.outlineWidth, where);
            s.opacity = number(j, u"opacity", s.opacity, where);
            for (const QJsonValue& path : j.value(u"paths"_s).toArray()) {
                s.paths.push_back(pairsOf(path, where));
            }
            s.open = flag(j, u"open", s.open, where);
            s.windingFill = flag(j, u"windingFill", s.windingFill, where);
            s.pen = outline(j.value(u"pen"_s), where);
            s.brush = fill(j.value(u"brush"_s), s.brush, where);
            out = std::move(s);
        } else if (type == u"image"_s) {
            ImageObject im;
            im.bounds = rect(j.value(u"bounds"_s), im.bounds, where);
            im.sourceWidth = integer<std::int32_t>(j, u"sourceWidth", im.sourceWidth, where);
            im.sourceHeight = integer<std::int32_t>(j, u"sourceHeight", im.sourceHeight, where);
            im.pattern = pick(kPatterns, j.value(u"pattern"_s), im.pattern, where);
            im.colorA = color(j, u"colorA", im.colorA, where);
            im.colorB = color(j, u"colorB", im.colorB, where);
            im.seed = integer<std::uint32_t>(j, u"seed", im.seed, where);
            im.resource = text(j, u"resource");
            if (j.value(u"keyColor"_s).isString()) {
                im.hasKeyColor = true;
                im.keyColor = color(j, u"keyColor", 0, where);
            }
            out = std::move(im);
        } else if (type == u"stroke"_s) {
            Stroke st;
            const QJsonArray p = j.value(u"points"_s).toArray();
            if (p.size() % 3 != 0) {
                warn(where, u"points du trait incomplets, le dernier est ignoré"_s);
            }
            for (qsizetype i = 0; i + 2 < p.size(); i += 3) {
                st.points.push_back(StrokePoint{p.at(i).toDouble(), p.at(i + 1).toDouble(),
                                                static_cast<float>(p.at(i + 2).toDouble())});
            }
            st.baseWidth = number(j, u"baseWidth", st.baseWidth, where);
            st.color = color(j, u"color", st.color, where);
            st.kind = pick(kStrokeKinds, j.value(u"kind"_s), st.kind, where);
            st.cap = pick(kPenCaps, j.value(u"cap"_s), st.cap, where);
            out = std::move(st);
        } else if (type == u"line"_s) {
            Line l;
            l.points = pairsOf(j.value(u"points"_s), where);
            l.pen = outline(j.value(u"pen"_s), where);
            out = std::move(l);
        } else if (type == u"text"_s) {
            Text t;
            t.bounds = rect(j.value(u"bounds"_s), t.bounds, where);
            t.html = text(j, u"html");
            for (const QJsonValue& rv : j.value(u"runs"_s).toArray()) {
                const QJsonObject r = rv.toObject();
                TextSpan run;
                run.text = text(r, u"text");
                run.fontFamily = text(r, u"fontFamily");
                run.pointSize = number(r, u"pointSize", run.pointSize, where);
                run.weight = integer<std::int32_t>(r, u"weight", run.weight, where);
                run.italic = flag(r, u"italic", run.italic, where);
                run.underline = flag(r, u"underline", run.underline, where);
                run.strikeOut = flag(r, u"strikeOut", run.strikeOut, where);
                run.color = color(r, u"color", run.color, where);
                run.baseline = point(r.value(u"baseline"_s), run.baseline, where);
                run.bounds = rect(r.value(u"bounds"_s), run.bounds, where);
                t.runs.push_back(std::move(run));
            }
            out = std::move(t);
        } else if (type == u"media"_s) {
            Media m;
            m.bounds = rect(j.value(u"bounds"_s), m.bounds, where);
            m.placeholder = text(j, u"placeholder");
            m.media = text(j, u"media");
            m.fileName = text(j, u"fileName");
            m.playOnOpen = flag(j, u"playOnOpen", m.playOnOpen, where);
            m.loop = flag(j, u"loop", m.loop, where);
            out = std::move(m);
        } else if (type == u"group"_s) {
            Group g;
            for (const QJsonValue& v : j.value(u"members"_s).toArray()) {
                g.members.push_back(id(v));
            }
            out = std::move(g);
        } else if (type == u"connector"_s) {
            Connector c;
            c.firstEnd = point(j.value(u"firstEnd"_s), c.firstEnd, where);
            c.secondEnd = point(j.value(u"secondEnd"_s), c.secondEnd, where);
            c.objectA = id(j.value(u"objectA"_s));
            c.objectB = id(j.value(u"objectB"_s));
            c.pen = outline(j.value(u"pen"_s), where);
            out = std::move(c);
        } else {
            warn(where, u"type d'objet inconnu « %1 », objet sauté"_s.arg(type));
            return std::nullopt;
        }
        readCommon(j, out.common, where);
        return out;
    }

    Page page(const QJsonObject& j, const QString& where)
    {
        Page p;
        p.width = number(j, u"width", p.width, where);
        p.height = number(j, u"height", p.height, where);
        p.name = text(j, u"name");
        p.notes = text(j, u"notes");
        const QJsonObject bg = j.value(u"background"_s).toObject();
        p.background.fill = fill(bg.value(u"fill"_s), p.background.fill, where);
        p.background.image = text(bg, u"image");
        const QJsonObject g = j.value(u"grid"_s).toObject();
        p.grid.visible = flag(g, u"visible", p.grid.visible, where);
        p.grid.snapEnabled = flag(g, u"snapEnabled", p.grid.snapEnabled, where);
        p.grid.drawnAbove = flag(g, u"drawnAbove", p.grid.drawnAbove, where);
        p.grid.previewZoom = number(g, u"previewZoom", p.grid.previewZoom, where);
        p.grid.baseZoom = number(g, u"baseZoom", p.grid.baseZoom, where);
        p.grid.zoomIncrement = number(g, u"zoomIncrement", p.grid.zoomIncrement, where);
        for (const QJsonValue& lv : g.value(u"levels"_s).toArray()) {
            const QJsonObject l = lv.toObject();
            GridLevel level;
            level.visible = flag(l, u"visible", level.visible, where);
            level.snap = flag(l, u"snap", level.snap, where);
            level.start = point(l.value(u"start"_s), level.start, where);
            level.spacingX = number(l, u"spacingX", level.spacingX, where);
            level.spacingY = number(l, u"spacingY", level.spacingY, where);
            level.color = color(l, u"color", level.color, where);
            level.pattern = pick(kGridPatterns, l.value(u"pattern"_s), level.pattern, where);
            level.angle = number(l, u"angle", level.angle, where);
            level.line = pick(kGridLines, l.value(u"line"_s), level.line, where);
            level.linesAlongX = flag(l, u"linesAlongX", level.linesAlongX, where);
            level.linesAlongY = flag(l, u"linesAlongY", level.linesAlongY, where);
            p.grid.levels.push_back(level);
        }
        const QJsonArray objects = j.value(u"objects"_s).toArray();
        for (qsizetype i = 0; i < objects.size(); ++i) {
            if (auto o = object(objects.at(i).toObject(), u"%1, objet %2"_s.arg(where).arg(i + 1))) {
                p.objects.push_back(std::move(*o));
            }
        }
        return p;
    }

private:
    void warn(const QString& where, const QString& what) { warnings << u"%1 : %2"_s.arg(where, what); }
};

} // namespace

QJsonObject documentToJson(const Document& document)
{
    QJsonObject root;
    root.insert(u"format"_s, QString::fromLatin1(kNativeFormat));
    root.insert(u"title"_s, str(document.title()));
    root.insert(u"nextId"_s, idValue(document.nextId()));
    QJsonArray resources;
    for (const auto& [key, resource] : document.resources()) {
        resources.append(QJsonObject{{u"key"_s, str(key)}, {u"extension"_s, str(resource.extension)}});
    }
    root.insert(u"resources"_s, resources);
    QJsonArray pages;
    for (std::size_t i = 0; i < document.pageCount(); ++i) {
        pages.append(pageJson(document.page(i)));
    }
    root.insert(u"pages"_s, pages);
    return root;
}

JsonReadResult documentFromJson(const QJsonObject& json, const ResourceLoader& loadResource, Document& out)
{
    JsonReadResult result;
    const QString format = json.value(u"format"_s).toString();
    static const QString prefix = u"actcraft-document/"_s;
    if (!format.startsWith(prefix)) {
        result.error = u"format inconnu « %1 » (attendu %2)"_s.arg(format, QString::fromLatin1(kNativeFormat));
        return result;
    }
    bool ok = false;
    const int major = format.mid(prefix.size()).section(u'.', 0, 0).toInt(&ok);
    if (!ok || major != kNativeMajorVersion) {
        result.error = u"version %1 du format non prise en charge (attendu %2)"_s.arg(format.mid(prefix.size()))
                           .arg(kNativeMajorVersion);
        return result;
    }
    Reader reader;
    std::vector<Page> pages;
    const QJsonArray jsonPages = json.value(u"pages"_s).toArray();
    for (qsizetype i = 0; i < jsonPages.size(); ++i) {
        pages.push_back(reader.page(jsonPages.at(i).toObject(), u"page %1"_s.arg(i + 1)));
    }
    if (pages.empty()) {
        reader.warnings << u"aucune page : une page vierge est créée"_s;
    }
    Document document(std::move(pages));
    document.setTitle(json.value(u"title"_s).toString().toStdString());
    for (const QJsonValue& rv : json.value(u"resources"_s).toArray()) {
        const QJsonObject r = rv.toObject();
        const std::string key = r.value(u"key"_s).toString().toStdString();
        const std::string extension = r.value(u"extension"_s).toString().toStdString();
        // Clé = SHA-256 hexadécimal, extension sûre : sinon le nom d'entrée d'archive pourrait viser ailleurs.
        static const std::regex kKey("^[0-9a-f]{64}$");
        static const std::regex kExtension("^[a-z0-9]{1,8}$");
        if (!std::regex_match(key, kKey) || !std::regex_match(extension, kExtension)) {
            reader.warnings << u"ressource « %1.%2 » : nom invalide, écartée"_s.arg(str(key), str(extension));
            continue;
        }
        const std::optional<QByteArray> data = loadResource ? loadResource(key, extension) : std::nullopt;
        if (!data) {
            reader.warnings << u"ressource %1 absente de l'archive"_s.arg(str(key));
            continue;
        }
        if (sha256Hex(*data) != key) {
            reader.warnings << u"ressource %1 : empreinte différente, écartée"_s.arg(str(key));
            continue;
        }
        document.addResource(*data, extension);
    }
    document.reserveIdsBelow(reader.id(json.value(u"nextId"_s)));
    if (!hasUniqueIds(document)) {
        reader.warnings << u"identifiants d'objets absents ou en double"_s;
    }
    out = std::move(document);
    result.ok = true;
    result.warnings = std::move(reader.warnings);
    return result;
}

} // namespace actcraft::formats
```

  Points d'attention : `Transform{…}` à six valeurs initialise **tous** ses champs (pas d'avertissement de Clang) ;
  `QString::arg(uint, int, int, QChar)` écrit l'hexadécimal en minuscules ; `QJsonValue` numérique :
  `QJsonDocument::toJson` écrit le plus court qui se relit à l'identique, d'où `roundTripThroughText`. Le test
  `noPageGivesOneBlankPage` attend un avertissement et une page ; `hasUniqueIds` ne signale rien sur une page vide.

  Ajouter à `formats/native.cmake` (dans le `set`) :

```cmake
set(ACTCRAFT_FORMATS_NATIVE_SOURCES
    include/actcraft/formats/nativejson.h
    src/nativejson.cpp)
```

- [ ] **Étape 6 : vert ciblé.** Commande ciblée, motif `tst_formats_nativejson` : `100% tests passed`.

- [ ] **Étape 7 : preuve.** `format.ps1`, preuve de tâche (N ≥ 46 ; `tst_formats_nativejson` Passed).

- [ ] **Étape 8 : commit.**

```powershell
git -C <worktree> add formats/include/actcraft/formats/nativejson.h formats/src/nativejson.cpp formats/native.cmake tests/formats.cmake tests/support/richdocument.h tests/tst_formats_nativejson.cpp
git -C <worktree> commit -m "formats : document.json du format natif (actcraft-document/1), lecture tolérante, version majeure refusée" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t6"></a>
### Tâche L2-T6 — `formats` : archive `.actcraft` (miniz) et E/S par URL (fichier local atomique, `content://`)

**Objectif.** DA3, décision 17 : l'archive `.actcraft` est un zip contenant `document.json` (compressé), puis
`resources/<sha256>.<ext>` (octets d'origine, **stockés sans compression** : images déjà compressées, et lisibles
telles quelles par d'autres outils), puis `thumbnails/page-<n>.png` (octets PNG fournis par l'appelant : `render/`
les produit en 2b ; `formats/` ne fait que les ranger). Encodage et décodage **en mémoire** (`QByteArray`), pour
que l'appelant (2b, fil secondaire) choisisse où écrire. Écriture par URL : fichier local → temporaire voisin puis
renommage (`QSaveFile`, atomique) ; `content://` (Android, aucun dossier voisin inscriptible) → écriture complète en
une passe. Lecture par URL bornée en taille. Garde-fous : entrée de plus de 512 Mio refusée avant allocation ;
archive illisible → erreur sans exception.

**Fichiers.**
- Créer : `formats/include/actcraft/formats/nativefile.h`, `formats/src/nativefile.cpp`,
  `formats/include/actcraft/formats/urlio.h`, `formats/src/urlio.cpp`, `tests/tst_formats_nativefile.cpp`,
  `tests/tst_formats_urlio.cpp`
- Modifier : `formats/native.cmake`, `tests/formats.cmake`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_formats_(nativefile|urlio)` → `error`
(en-têtes introuvables). Vert : forme bornée, N ≥ 48, `TESTS OK`, `tst_formats_nativefile .*Passed`,
`tst_formats_urlio .*Passed`. Le chemin `content://` n'est pas exécutable sous Windows : il est prouvé sur
l'émulateur par L2-T22 ; ici, seul son aiguillage est testé (`writeUrl` sur un schéma inconnu échoue avec son
message, `urlFromArgument` garde `content://`).

**Interdits.** Pas de `QZipReader`/`QZipWriter` (API privée). Pas de QtGui. Ne jamais écrire directement par-dessus
un fichier local existant (toujours `QSaveFile`). Ne pas lire une archive entière d'une taille annoncée
déraisonnable.

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests de l'archive.** Créer `tests/tst_formats_nativefile.cpp` :

```cpp
#include "support/richdocument.h"

#include <actcraft/formats/nativefile.h>
#include <actcraft/formats/nativejson.h>

#include <QJsonDocument>
#include <QTest>

using namespace actcraft;
using namespace Qt::StringLiterals;

// Archive .actcraft (DA3) : document.json + ressources + miniatures ; aller-retour à empreinte égale.
class TstFormatsNativeFile : public QObject
{
    Q_OBJECT

private slots:
    void roundTripKeepsTheFingerprintAndThumbnails()
    {
        const core::Document rich = richdoc::richDocument();
        const std::vector<QByteArray> thumbs{QByteArray("png-1"), QByteArray("png-2"), QByteArray("png-3")};
        QString error;
        const QByteArray archive = formats::encodeActcraft(rich, thumbs, &error);
        QVERIFY2(!archive.isEmpty(), qPrintable(error));
        QVERIFY(formats::looksLikeActcraft(archive));
        const formats::DecodeResult r = formats::decodeActcraft(archive);
        QVERIFY2(r.ok, qPrintable(r.error));
        QVERIFY2(r.warnings.isEmpty(), qPrintable(r.warnings.join(u'\n')));
        QCOMPARE(core::fingerprint(r.document), core::fingerprint(rich));
        QCOMPARE(r.thumbnails.size(), std::size_t{3});
        QCOMPARE(r.thumbnails[1], QByteArray("png-2"));
    }

    void entriesAreNamedAsDocumented()
    {
        const core::Document rich = richdoc::richDocument();
        const QByteArray archive = formats::encodeActcraft(rich, {QByteArray("a"), QByteArray("b"), QByteArray("c")});
        const QStringList names = formats::archiveEntryNames(archive);
        QCOMPARE(names.first(), u"document.json"_s);
        const QString png = QString::fromStdString(core::sha256Hex(richdoc::pngBytes()));
        const QString mp3 = QString::fromStdString(core::sha256Hex(richdoc::mp3Bytes()));
        QVERIFY(names.contains(u"resources/"_s + png + u".png"_s));
        QVERIFY(names.contains(u"resources/"_s + mp3 + u".mp3"_s));
        QVERIFY(names.contains(u"thumbnails/page-1.png"_s));
        QVERIFY(names.contains(u"thumbnails/page-3.png"_s));
        QCOMPARE(names.size(), 6);
    }

    void resourcesAreStoredVerbatim()
    {
        const core::Document rich = richdoc::richDocument();
        const QByteArray archive = formats::encodeActcraft(rich, {});
        QVERIFY(archive.contains(richdoc::pngBytes())); // stockées sans compression : octets d'origine lisibles
        QVERIFY(archive.contains(richdoc::mp3Bytes()));
    }

    void encodingIsReproducible()
    {
        const core::Document rich = richdoc::richDocument();
        QCOMPARE(formats::encodeActcraft(rich, {QByteArray("x")}), formats::encodeActcraft(rich, {QByteArray("x")}));
    }

    void missingThumbnailsAreAllowed()
    {
        const core::Document rich = richdoc::richDocument();
        const formats::DecodeResult r = formats::decodeActcraft(formats::encodeActcraft(rich, {}));
        QVERIFY(r.ok);
        QCOMPARE(r.thumbnails.size(), std::size_t{3}); // une case par page, vides
        QVERIFY(r.thumbnails[0].isEmpty());
    }

    void totalDecompressionBudgetIsEnforced()
    {
        // Budget = document.json + 1 octet : chaque entrée tient seule sous le budget, seul le CUMUL le dépasse dès la
        // première ressource (sans `m_total += …`, les deux ressources seraient lues et le test passerait au rouge).
        const core::Document rich = richdoc::richDocument();
        const QByteArray archive = formats::encodeActcraft(rich, {});
        QVERIFY(formats::decodeActcraft(archive).ok);
        const qint64 json = QJsonDocument(formats::documentToJson(rich)).toJson(QJsonDocument::Indented).size();
        const formats::DecodeResult r = formats::decodeActcraft(archive, json + 1);
        QVERIFY2(r.ok, qPrintable(r.error)); // document.json lu ; les ressources refusées deviennent des avertissements
        QCOMPARE(r.document.resources().size(), std::size_t{0});
        QVERIFY2(r.warnings.join(u' ').contains(u"absente"_s), qPrintable(r.warnings.join(u'
')));
    }

    void garbageIsRefusedWithAMessage()
    {
        QVERIFY(!formats::looksLikeActcraft("Bamboo"));
        const formats::DecodeResult r = formats::decodeActcraft(QByteArray("PK\x03\x04 pas un zip", 16));
        QVERIFY(!r.ok);
        QVERIFY(!r.error.isEmpty());
        QVERIFY(!formats::decodeActcraft(QByteArray()).ok);
    }

    void archiveWithoutDocumentJsonIsRefused()
    {
        // Zip valide dont l'entrée « document.json » porte un autre nom de même longueur.
        core::Document empty;
        QByteArray archive = formats::encodeActcraft(empty, {QByteArray("t")});
        archive.replace("document.json", "documentXjson"); // nom local et nom central modifiés ensemble
        const formats::DecodeResult r = formats::decodeActcraft(archive);
        QVERIFY(!r.ok);
        QVERIFY2(r.error.contains(u"document.json"_s), qPrintable(r.error));
    }
};

QTEST_GUILESS_MAIN(TstFormatsNativeFile)
#include "tst_formats_nativefile.moc"
```

  Remplacer le nom dans les en-têtes locaux et centraux d'un zip garde des tailles identiques (13 octets) : miniz
  relit l'archive, ne trouve pas `document.json`. Si la somme de contrôle du nom n'existe pas dans le format zip
  (c'est le cas : seules les données ont un CRC), le test reste valable.

- [ ] **Étape 2 : écrire les tests des E/S par URL.** Créer `tests/tst_formats_urlio.cpp` :

```cpp
#include <actcraft/formats/urlio.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>
#include <cstring>

using namespace actcraft;
using namespace Qt::StringLiterals;

// E/S par URL (décision 17) : fichier local par temporaire voisin puis renommage ; content:// aiguillé.
class TstFormatsUrlIo : public QObject
{
    Q_OBJECT

private slots:
    void writeThenReadLocalFile()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QUrl url = QUrl::fromLocalFile(dir.filePath(u"essai.actcraft"_s));
        QString error;
        QVERIFY2(formats::writeUrl(url, QByteArray("bonjour"), &error), qPrintable(error));
        const formats::UrlReadResult r = formats::readUrl(url);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.data, QByteArray("bonjour"));
    }

    void rewriteReplacesTheWholeFile()
    {
        QTemporaryDir dir;
        const QUrl url = QUrl::fromLocalFile(dir.filePath(u"essai.actcraft"_s));
        QVERIFY(formats::writeUrl(url, QByteArray(1000, 'a'), nullptr));
        QVERIFY(formats::writeUrl(url, QByteArray("court"), nullptr));
        QCOMPARE(formats::readUrl(url).data, QByteArray("court"));
        // Aucun temporaire laissé à côté.
        QCOMPARE(QDir(dir.path()).entryList(QDir::Files).size(), 1);
    }

    void failedWriteKeepsThePreviousFile()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(u"essai.actcraft"_s);
        QVERIFY(formats::writeUrl(QUrl::fromLocalFile(path), QByteArray("ancien"), nullptr));
        QString error;
        // Dossier absent : rien n'est écrit, l'erreur dit pourquoi.
        QVERIFY(!formats::writeUrl(QUrl::fromLocalFile(dir.filePath(u"absent/x.actcraft"_s)), "y", &error));
        QVERIFY(!error.isEmpty());
        QCOMPARE(formats::readUrl(QUrl::fromLocalFile(path)).data, QByteArray("ancien"));
    }

    void readRefusesMissingAndOversizedFiles()
    {
        QTemporaryDir dir;
        const formats::UrlReadResult missing = formats::readUrl(QUrl::fromLocalFile(dir.filePath(u"absent"_s)));
        QVERIFY(!missing.ok);
        QVERIFY(!missing.error.isEmpty());
        const QUrl big = QUrl::fromLocalFile(dir.filePath(u"gros"_s));
        QVERIFY(formats::writeUrl(big, QByteArray(100, 'x'), nullptr));
        const formats::UrlReadResult tooBig = formats::readUrl(big, 10);
        QVERIFY(!tooBig.ok);
        QVERIFY2(tooBig.error.contains(u"volumineux"_s), qPrintable(tooBig.error));
    }

    void sequentialDeviceOfUnknownSizeIsReadToTheEnd()
    {
        // Comme un content:// : séquentiel, taille 0 annoncée, atEnd() vrai avant toute lecture.
        class Sequential final : public QIODevice
        {
        public:
            explicit Sequential(QByteArray data) : m_data(std::move(data)) {}
            bool isSequential() const override { return true; }

        protected:
            qint64 readData(char* out, qint64 max) override
            {
                const qint64 n = std::min(max, static_cast<qint64>(m_data.size()) - m_pos);
                std::memcpy(out, m_data.constData() + m_pos, static_cast<std::size_t>(n));
                m_pos += n;
                return n;
            }
            qint64 writeData(const char*, qint64) override { return -1; }

        private:
            QByteArray m_data;
            qint64 m_pos = 0;
        };
        const QByteArray payload(5 * (1 << 20) / 2, 'q'); // 2,5 Mio : trois blocs
        Sequential device(payload);
        QVERIFY(device.open(QIODevice::ReadOnly));
        QString error;
        const std::optional<QByteArray> all = formats::readBounded(device, 1ll << 30, &error);
        QVERIFY2(all, qPrintable(error));
        QCOMPARE(all->size(), payload.size());
        Sequential bounded(payload);
        QVERIFY(bounded.open(QIODevice::ReadOnly));
        QVERIFY(!formats::readBounded(bounded, 1 << 20, &error));
        QVERIFY(error.contains(u"volumineux"_s));
    }

    void smallFileDoesNotReserveTheMaximum()
    {
        // Borne énorme : l'ancienne lecture (file.read(maxBytes + 1)) tentait de réserver 1 Tio et échouait.
        QTemporaryDir dir;
        const QUrl url = QUrl::fromLocalFile(dir.filePath(u"petit"_s));
        QVERIFY(formats::writeUrl(url, QByteArray("7 octets"), nullptr));
        const formats::UrlReadResult r = formats::readUrl(url, 1ll << 40);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.data, QByteArray("7 octets"));
    }

    void unsupportedSchemeIsRefused()
    {
        QString error;
        QVERIFY(!formats::writeUrl(QUrl(u"https://exemple.org/x.actcraft"_s), "x", &error));
        QVERIFY2(error.contains(u"non prise en charge"_s), qPrintable(error));
        QVERIFY(!formats::readUrl(QUrl(u"https://exemple.org/x.actcraft"_s)).ok);
    }

    void argumentsBecomeUrls()
    {
        const QUrl local = formats::urlFromArgument(u"C:/Users/x/doc.actcraft"_s);
        QVERIFY(local.isLocalFile());
        QCOMPARE(QDir::fromNativeSeparators(local.toLocalFile()), u"C:/Users/x/doc.actcraft"_s);
        const QUrl content = formats::urlFromArgument(u"content://com.android.externalstorage.documents/document/x"_s);
        QCOMPARE(content.scheme(), u"content"_s);
        QCOMPARE(formats::urlFromArgument(QUrl(u"file:///C:/a.actcraft"_s)).toLocalFile(), u"C:/a.actcraft"_s);
        QVERIFY(formats::urlFromArgument(QVariant()).isEmpty());
        QVERIFY(formats::urlFromArgument(u""_s).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TstFormatsUrlIo)
#include "tst_formats_urlio.moc"
```

  `argumentsBecomeUrls` suppose des chemins Windows : sous macOS (CI), `C:/Users/x/doc.actcraft` n'est pas absolu et
  `file:///C:/a.actcraft` donne `/C:/a.actcraft`. Encadrer les lignes `local` et `file:///C:` par `#ifdef Q_OS_WIN` /
  `#else` / `#endif`, avec dans la branche `#else` les mêmes vérifications sur `/tmp/doc.actcraft` et
  `file:///tmp/a.actcraft` (→ `/tmp/a.actcraft`).

  Ajouter à `tests/formats.cmake` :

```cmake
actcraft_add_test(tst_formats_nativefile
    SOURCES tst_formats_nativefile.cpp
    LIBS ActCraft::formats)

actcraft_add_test(tst_formats_urlio
    SOURCES tst_formats_urlio.cpp
    LIBS ActCraft::formats)
```

- [ ] **Étape 3 : constater le rouge.** Commande ciblée, motif `tst_formats_(nativefile|urlio)` → `error`.

- [ ] **Étape 4 : écrire l'archive.** Créer `formats/include/actcraft/formats/nativefile.h` :

```cpp
#pragma once

#include <actcraft/core/document.h>

#include <QByteArray>
#include <QString>
#include <QStringList>

#include <vector>

namespace actcraft::formats {

// Une entrée décompressée de plus de 512 Mio est refusée avant toute allocation (archive non fiable), et une archive
// dont les entrées lues dépasseraient 2 Gio au total aussi (budget de décompression).
inline constexpr qint64 kMaxArchiveEntry = 512ll * 1024 * 1024;
inline constexpr qint64 kMaxArchiveTotal = 2048ll * 1024 * 1024;

// Archive .actcraft (DA3) : « document.json » (compressé), « resources/<sha256>.<ext> » (stockés tels quels),
// « thumbnails/page-<n>.png » (n à partir de 1 ; octets fournis par l'appelant, rangés tels quels). Reproductible :
// mêmes entrées, mêmes octets. Vide en cas d'échec (`error` dit pourquoi).
QByteArray encodeActcraft(const core::Document& document, const std::vector<QByteArray>& thumbnailsPng,
                          QString* error = nullptr);

struct DecodeResult
{
    bool ok = false;
    QString error;
    QStringList warnings;
    core::Document document;
    std::vector<QByteArray> thumbnails; // une case par page, vide si absente
};

// `maxTotal` : budget de décompression (petit dans les tests, pour le prouver sans archive de 2 Gio).
DecodeResult decodeActcraft(const QByteArray& archive, qint64 maxTotal = kMaxArchiveTotal);
// Noms des entrées dans l'ordre de l'archive ; vide si illisible.
QStringList archiveEntryNames(const QByteArray& archive);
// Signature zip (« PK\3\4 ») : de quoi choisir le lecteur sans lire tout le fichier.
bool looksLikeActcraft(const QByteArray& head) noexcept;

} // namespace actcraft::formats
```

  Créer `formats/src/nativefile.cpp` :

```cpp
#include <actcraft/formats/nativefile.h>
#include <actcraft/formats/nativejson.h>

#include <QJsonDocument>
#include <QJsonObject>

#include <miniz.h>

#include <optional>
#include <string>

namespace actcraft::formats {

using namespace Qt::StringLiterals;

namespace {

std::string resourceEntry(const std::string& key, const std::string& extension)
{
    return "resources/" + key + (extension.empty() ? std::string() : "." + extension);
}

std::string thumbnailEntry(std::size_t page)
{
    return "thumbnails/page-" + std::to_string(page + 1) + ".png";
}

// Lecteur zip en mémoire, fermé à la destruction.
class ZipReader
{
public:
    explicit ZipReader(const QByteArray& archive, qint64 maxTotal = kMaxArchiveTotal) : m_maxTotal(maxTotal)
    {
        m_open = !archive.isEmpty() &&
                 mz_zip_reader_init_mem(&m_zip, archive.constData(), static_cast<size_t>(archive.size()), 0);
    }
    ~ZipReader()
    {
        if (m_open) {
            mz_zip_reader_end(&m_zip);
        }
    }
    ZipReader(const ZipReader&) = delete;
    ZipReader& operator=(const ZipReader&) = delete;

    bool isOpen() const noexcept { return m_open; }

    QStringList names()
    {
        QStringList out;
        const mz_uint count = mz_zip_reader_get_num_files(&m_zip);
        for (mz_uint i = 0; i < count; ++i) {
            char name[1024] = {};
            mz_zip_reader_get_filename(&m_zip, i, name, static_cast<mz_uint>(sizeof(name)));
            out << QString::fromUtf8(name);
        }
        return out;
    }

    // nullopt si absente ; `tooBig` vrai si refusée par sa taille.
    std::optional<QByteArray> extract(const std::string& name, bool* tooBig = nullptr)
    {
        const int index = mz_zip_reader_locate_file(&m_zip, name.c_str(), nullptr, 0);
        if (index < 0) {
            return std::nullopt;
        }
        mz_zip_archive_file_stat stat{};
        if (!mz_zip_reader_file_stat(&m_zip, static_cast<mz_uint>(index), &stat)) {
            return std::nullopt;
        }
        if (stat.m_uncomp_size > static_cast<mz_uint64>(kMaxArchiveEntry) ||
            m_total + static_cast<qint64>(stat.m_uncomp_size) > m_maxTotal) {
            if (tooBig) {
                *tooBig = true;
            }
            return std::nullopt;
        }
        QByteArray out(static_cast<qsizetype>(stat.m_uncomp_size), Qt::Uninitialized);
        if (!mz_zip_reader_extract_to_mem(&m_zip, static_cast<mz_uint>(index), out.data(),
                                          static_cast<size_t>(out.size()), 0)) {
            return std::nullopt;
        }
        m_total += out.size();
        return out;
    }

private:
    mz_zip_archive m_zip{};
    bool m_open = false;
    qint64 m_total = 0; // octets décompressés jusqu'ici
    qint64 m_maxTotal;  // budget (kMaxArchiveTotal hors des tests)
};

} // namespace

QByteArray encodeActcraft(const core::Document& document, const std::vector<QByteArray>& thumbnailsPng,
                          QString* error)
{
    mz_zip_archive zip{};
    if (!mz_zip_writer_init_heap(&zip, 0, 64 * 1024)) {
        if (error) {
            *error = u"archive : initialisation impossible"_s;
        }
        return {};
    }
    const auto fail = [&zip, error](const QString& why) {
        mz_zip_writer_end(&zip);
        if (error) {
            *error = why;
        }
        return QByteArray();
    };
    const QByteArray json = QJsonDocument(documentToJson(document)).toJson(QJsonDocument::Indented);
    if (!mz_zip_writer_add_mem(&zip, "document.json", json.constData(), static_cast<size_t>(json.size()),
                               static_cast<mz_uint>(MZ_BEST_COMPRESSION))) {
        return fail(u"archive : document.json non écrit"_s);
    }
    for (const auto& [key, resource] : document.resources()) {
        const std::string name = resourceEntry(key, resource.extension);
        if (!mz_zip_writer_add_mem(&zip, name.c_str(), resource.data.constData(),
                                   static_cast<size_t>(resource.data.size()), static_cast<mz_uint>(MZ_NO_COMPRESSION))) {
            return fail(u"archive : ressource %1 non écrite"_s.arg(QString::fromStdString(key)));
        }
    }
    for (std::size_t i = 0; i < thumbnailsPng.size(); ++i) {
        if (thumbnailsPng[i].isEmpty()) {
            continue;
        }
        const std::string name = thumbnailEntry(i);
        if (!mz_zip_writer_add_mem(&zip, name.c_str(), thumbnailsPng[i].constData(),
                                   static_cast<size_t>(thumbnailsPng[i].size()),
                                   static_cast<mz_uint>(MZ_NO_COMPRESSION))) {
            return fail(u"archive : miniature %1 non écrite"_s.arg(i + 1));
        }
    }
    void* buffer = nullptr;
    size_t size = 0;
    if (!mz_zip_writer_finalize_heap_archive(&zip, &buffer, &size)) {
        return fail(u"archive : finalisation impossible"_s);
    }
    QByteArray out(static_cast<const char*>(buffer), static_cast<qsizetype>(size));
    mz_free(buffer);
    mz_zip_writer_end(&zip);
    return out;
}

DecodeResult decodeActcraft(const QByteArray& archive, qint64 maxTotal)
{
    DecodeResult result;
    ZipReader zip(archive, maxTotal);
    if (!zip.isOpen()) {
        result.error = u"archive .actcraft illisible"_s;
        return result;
    }
    bool tooBig = false;
    const std::optional<QByteArray> json = zip.extract("document.json", &tooBig);
    if (!json) {
        result.error = tooBig ? u"document.json trop volumineux"_s : u"document.json absent de l'archive"_s;
        return result;
    }
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(*json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        result.error = u"document.json illisible : %1"_s.arg(parseError.errorString());
        return result;
    }
    const JsonReadResult read = documentFromJson(
        doc.object(),
        [&zip](const std::string& key, const std::string& extension) {
            return zip.extract(resourceEntry(key, extension));
        },
        result.document);
    if (!read.ok) {
        result.error = read.error;
        return result;
    }
    result.warnings = read.warnings;
    result.thumbnails.resize(result.document.pageCount());
    for (std::size_t i = 0; i < result.thumbnails.size(); ++i) {
        if (std::optional<QByteArray> png = zip.extract(thumbnailEntry(i))) {
            result.thumbnails[i] = std::move(*png);
        }
    }
    result.ok = true;
    return result;
}

QStringList archiveEntryNames(const QByteArray& archive)
{
    ZipReader zip(archive);
    return zip.isOpen() ? zip.names() : QStringList();
}

bool looksLikeActcraft(const QByteArray& head) noexcept
{
    return head.startsWith("PK\x03\x04");
}

} // namespace actcraft::formats
```

- [ ] **Étape 5 : écrire les E/S par URL.** Créer `formats/include/actcraft/formats/urlio.h` :

```cpp
#pragma once

#include <QByteArray>
#include <QIODevice>
#include <QString>
#include <QUrl>
#include <QVariant>

#include <optional>

namespace actcraft::formats {

// Lecture et écriture d'octets par une URL (décision 17) : fichier local, ou content:// d'Android (QFile sait
// ouvrir ces URL sous Android). Aucun autre schéma.
struct UrlReadResult
{
    bool ok = false;
    QString error;
    QByteArray data;
};

UrlReadResult readUrl(const QUrl& url, qint64 maxBytes = 1024ll * 1024 * 1024);
// Lit `device` par blocs de 1 Mio jusqu'à un bloc vide (jamais atEnd : taille inconnue d'un content://) ; nullopt et
// `error` au-delà de `maxBytes`.
std::optional<QByteArray> readBounded(QIODevice& device, qint64 maxBytes, QString* error);
// Fichier local : temporaire voisin puis renommage (QSaveFile) — l'ancien fichier reste intact en cas d'échec.
// content:// : écriture complète en une passe (pas de dossier voisin inscriptible).
bool writeUrl(const QUrl& url, const QByteArray& data, QString* error);
// Argument d'une commande de fichier (décision 20) : QUrl telle quelle ; texte « content:… » ou « file:… » lu
// comme URL ; autre texte = chemin local (relatif au dossier courant). Vide si l'argument est absent ou vide.
QUrl urlFromArgument(const QVariant& argument);

} // namespace actcraft::formats
```

  Créer `formats/src/urlio.cpp` :

```cpp
#include <actcraft/formats/urlio.h>

#include <QDir>
#include <QFile>
#include <QSaveFile>

namespace actcraft::formats {

using namespace Qt::StringLiterals;

namespace {

bool isContent(const QUrl& url)
{
    return url.scheme() == u"content"_s;
}

} // namespace

std::optional<QByteArray> readBounded(QIODevice& device, qint64 maxBytes, QString* error)
{
    // Jamais atEnd() : un appareil séquentiel de taille inconnue (content://) dit « fini » avant la première lecture.
    QByteArray data;
    for (;;) {
        const QByteArray chunk = device.read(1 << 20);
        if (chunk.isEmpty()) {
            break;
        }
        data += chunk;
        if (data.size() > maxBytes) {
            if (error) {
                *error = u"fichier trop volumineux"_s;
            }
            return std::nullopt;
        }
    }
    return data;
}

UrlReadResult readUrl(const QUrl& url, qint64 maxBytes)
{
    UrlReadResult r;
    if (!url.isLocalFile() && !isContent(url)) {
        r.error = u"URL non prise en charge : %1"_s.arg(url.toString());
        return r;
    }
    QFile file(url.isLocalFile() ? url.toLocalFile() : url.toString());
    if (!file.open(QIODevice::ReadOnly)) {
        r.error = u"ouverture impossible : %1 (%2)"_s.arg(url.toDisplayString(), file.errorString());
        return r;
    }
    if (file.size() > maxBytes) {
        r.error = u"fichier trop volumineux : %1"_s.arg(url.toDisplayString());
        return r;
    }
    // Jamais file.read(maxBytes + 1) : QIODevice réserve la taille demandée avant de lire (relecture du plan,
    // tour 1 : 2 Gio réservés pour un fichier de 7 octets). Fichier local : taille connue, contrôlée, lecture
    // entière. content:// : taille souvent inconnue (0) : blocs de 1 Mio, total borné.
    QByteArray data;
    if (url.isLocalFile()) {
        data = file.readAll();
    } else {
        std::optional<QByteArray> read = readBounded(file, maxBytes, &r.error);
        if (!read) {
            r.error = u"%1 : %2"_s.arg(url.toDisplayString(), r.error);
            return r;
        }
        data = std::move(*read);
    }
    if (file.error() != QFileDevice::NoError) {
        r.error = u"lecture impossible : %1 (%2)"_s.arg(url.toDisplayString(), file.errorString());
        return r;
    }
    r.ok = true;
    r.data = std::move(data);
    return r;
}

bool writeUrl(const QUrl& url, const QByteArray& data, QString* error)
{
    const auto fail = [error](const QString& why) {
        if (error) {
            *error = why;
        }
        return false;
    };
    if (url.isLocalFile()) {
        QSaveFile file(url.toLocalFile());
        if (!file.open(QIODevice::WriteOnly)) {
            return fail(u"écriture impossible : %1 (%2)"_s.arg(url.toDisplayString(), file.errorString()));
        }
        if (file.write(data) != data.size()) {
            const QString why = file.errorString();
            file.cancelWriting();
            return fail(u"écriture incomplète : %1 (%2)"_s.arg(url.toDisplayString(), why));
        }
        if (!file.commit()) {
            return fail(u"enregistrement impossible : %1 (%2)"_s.arg(url.toDisplayString(), file.errorString()));
        }
        return true;
    }
    if (isContent(url)) {
        QFile file(url.toString());
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return fail(u"écriture impossible : %1 (%2)"_s.arg(url.toDisplayString(), file.errorString()));
        }
        if (file.write(data) != data.size() || !file.flush()) {
            return fail(u"écriture incomplète : %1 (%2)"_s.arg(url.toDisplayString(), file.errorString()));
        }
        file.close(); // erreur possible à la fermeture (fournisseur de documents) : la lire
        if (file.error() != QFileDevice::NoError) {
            return fail(u"enregistrement impossible : %1 (%2)"_s.arg(url.toDisplayString(), file.errorString()));
        }
        return true;
    }
    return fail(u"URL non prise en charge : %1"_s.arg(url.toString()));
}

QUrl urlFromArgument(const QVariant& argument)
{
    if (argument.typeId() == QMetaType::QUrl) {
        return argument.toUrl();
    }
    const QString text = argument.toString().trimmed();
    if (text.isEmpty()) {
        return {};
    }
    if (text.startsWith(u"content:"_s) || text.startsWith(u"file:"_s)) {
        return QUrl(text);
    }
    return QUrl::fromLocalFile(QDir::cleanPath(QDir::current().absoluteFilePath(text)));
}

} // namespace actcraft::formats
```

  Ajouter les quatre fichiers à `formats/native.cmake` :

```cmake
set(ACTCRAFT_FORMATS_NATIVE_SOURCES
    include/actcraft/formats/nativejson.h
    src/nativejson.cpp
    include/actcraft/formats/nativefile.h
    src/nativefile.cpp
    include/actcraft/formats/urlio.h
    src/urlio.cpp)
```

- [ ] **Étape 6 : vert ciblé.** Commande ciblée, motif `tst_formats_(nativefile|urlio)` : `100% tests passed`.
  `failedWriteKeepsThePreviousFile` et `rewriteReplacesTheWholeFile` prouvent l'atomicité locale ; vérifier aussi à
  la main, une fois, que `QSaveFile` n'est pas en écriture directe (`directWriteFallback()` faux par défaut) : le
  noter au rapport.

- [ ] **Étape 7 : preuve.** `format.ps1`, preuve de tâche (N ≥ 48 ; `tst_formats_nativefile` et
  `tst_formats_urlio` Passed).

- [ ] **Étape 8 : commit.**

```powershell
git -C <worktree> add formats/include/actcraft/formats/nativefile.h formats/src/nativefile.cpp formats/include/actcraft/formats/urlio.h formats/src/urlio.cpp formats/native.cmake tests/formats.cmake tests/tst_formats_nativefile.cpp tests/tst_formats_urlio.cpp
git -C <worktree> commit -m "formats : archive .actcraft (miniz, reproductible) et E/S par URL (fichier local atomique, content://)" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t7"></a>
### Tâche L2-T7 — `formats` : conteneur Bamboo et index « culm » ; écrivain de paperboards synthétiques

**Objectif.** DA4, décision 18 : première couche du lecteur `.flipchart`, en QtCore seulement, d'après notre
documentation (`C:\Users\JeromeDenis\REA - Activinspire\docs\re\format-flipchart.md` § 2 et § 3 ; comportement de
référence : `app/actcraft/io/bamboo.py` et `read_index` de `app/actcraft/io/objstream.py`). Le conteneur : signature
`" Bamboo "`, version u16, QString de format, 8 octets réservés, puis des entrées (nom, trois dates, taille
décompressée u64, drapeau de compression u8, taille stockée u64, données ; données compressées = en-tête `qCompress`
4 octets gros-boutiste + flux zlib, que `qUncompress` lit tel quel). L'index : `"# culm #" u16 1 u16 7`, puis des
nœuds `N id u16-type id-stockage … n` et des références `S id s`. Fichier non fiable : toute lecture est bornée,
aucune exception, une erreur qui dit où. Écrire aussi le premier morceau de **l'écrivain de test**
(`tests/support/flipchartwriter.h`) qui produit ces octets d'après la même documentation : aucun paperboard réel
n'entre dans les tests.

**Fichiers.**
- Créer : `formats/include/actcraft/formats/bamboo.h`, `formats/src/bamboo.cpp`,
  `tests/support/flipchartwriter.h`, `tests/support/peakmemory.h`, `tests/tst_formats_bamboo.cpp`
- Modifier : `formats/flipchart.cmake`, `tests/flipchart.cmake`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_formats_bamboo` → `error` (`bamboo.h`
introuvable). Vert : forme bornée, N ≥ 46, `TESTS OK`, `tst_formats_bamboo .*Passed`. Bombe zlib, deux preuves :
(a) structurelle, `git -C <worktree> grep -n "qUncompress" -- formats/src` → vide ; (b) le test
`zlibBombStaysWithinTheAnnouncedSize` : 256 Mio de zéros compressés par `mz_deflate` (≈ 256 Kio), annoncés à
10 octets ; hausse du pic de mémoire du processus pendant `read()` < 32 Mio (aide `tests/support/peakmemory.h`). Le
chemin fautif (`qUncompress`) **écrit** les 256 Mio décompressés dans son tampon avant de constater l'écart de taille :
les pages sont touchées, le pic monte d'environ 256 Mio. Rouge vu par mutation **après le commit** : remplacer
l'appel `mz_uncompress` par `const QByteArray out = qUncompress(e.raw);` suivi du contrôle de taille, constater le
rouge (hausse ≈ 256 Mio), remettre par `git -C <worktree> checkout -- formats/src/bamboo.cpp`. **Écart à la décision 18** (« `qUncompress` pour le conteneur ») : décompression par miniz
(déjà lié par `formats/`, L2-T4), bornée par la taille de l'entrée.

**Interdits.** QtCore seulement. Rien de copié depuis `samples/` (ni octets, ni noms de fichiers réels). Pas de
lecture hors limites (`Cursor` vérifie chaque taille). Aucun nom de classe d'Inspire dans le code.

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire l'écrivain de test (conteneur et index).** Créer `tests/support/flipchartwriter.h` :

```cpp
#pragma once

// Écrivain de paperboards .flipchart synthétiques pour les tests (DA4, décision 19). Écrit d'après notre
// documentation du format (REA - Activinspire/docs/re/format-flipchart.md), jamais à partir de fichiers réels :
// aucun octet de samples/ n'est copié ici. Complété par L2-T8 (flux objet) et L2-T9 (paperboard complet).

#include <QByteArray>
#include <QString>
#include <QtEndian>

#include <cstring>
#include <utility>
#include <vector>

namespace fctest {

// Octets petit-boutistes, à la suite.
class Bytes
{
public:
    Bytes& u8(quint8 v)
    {
        m_data.append(static_cast<char>(v));
        return *this;
    }
    Bytes& u16(quint16 v) { return put(qToLittleEndian(v)); }
    Bytes& u32(quint32 v) { return put(qToLittleEndian(v)); }
    Bytes& u64(quint64 v) { return put(qToLittleEndian(v)); }
    Bytes& i32(qint32 v) { return put(qToLittleEndian(v)); }
    Bytes& f64(double v)
    {
        quint64 bits = 0;
        std::memcpy(&bits, &v, sizeof bits);
        return u64(bits);
    }
    Bytes& tag(char c)
    {
        m_data.append(c);
        return *this;
    }
    Bytes& raw(const QByteArray& bytes)
    {
        m_data.append(bytes);
        return *this;
    }
    // QString : u32 = longueur en octets, puis UTF-16LE.
    Bytes& qstr(const QString& s)
    {
        u32(static_cast<quint32>(s.size() * 2));
        for (const QChar c : s) {
            u16(c.unicode());
        }
        return *this;
    }
    Bytes& nullQstr() { return u32(0xFFFFFFFFu); }
    // Chaîne d'une variante : u16 = 1 puis QString.
    Bytes& str16(const QString& s)
    {
        u16(1);
        return qstr(s);
    }
    const QByteArray& data() const noexcept { return m_data; }

private:
    template <typename T> Bytes& put(T v)
    {
        m_data.append(reinterpret_cast<const char*>(&v), static_cast<qsizetype>(sizeof v));
        return *this;
    }
    QByteArray m_data;
};

struct Entry
{
    Entry(QString n, QByteArray d, bool c = true) : name(std::move(n)), data(std::move(d)), compress(c) {}
    QString name;
    QByteArray data;
    bool compress;
};

// Conteneur « Bamboo » (§ 2) ; données compressées par qCompress (u32 gros-boutiste + zlib), comme le format.
inline QByteArray container(const std::vector<Entry>& entries)
{
    Bytes b;
    b.raw(QByteArray(" Bamboo ", 8)).u16(7).qstr(QStringLiteral("1.0")).raw(QByteArray("\0\0\0\0\0\0\x01\0", 8));
    for (const Entry& e : entries) {
        b.qstr(e.name);
        for (int i = 0; i < 3; ++i) {
            b.qstr(QStringLiteral("lun. 1. janv. 00:00:00 2024"));
        }
        const QByteArray stored = e.compress ? qCompress(e.data) : e.data;
        b.u64(static_cast<quint64>(e.data.size())).u8(e.compress ? 1 : 0).u64(static_cast<quint64>(stored.size()));
        b.raw(stored);
    }
    return b.data();
}

struct Node
{
    Node(quint32 i, quint16 t, quint32 s, std::vector<Node> c = {}, std::vector<quint32> r = {})
        : id(i), type(t), storage(s), children(std::move(c)), refs(std::move(r))
    {
    }
    quint32 id;
    quint16 type;
    quint32 storage;
    std::vector<Node> children;
    std::vector<quint32> refs;
};

inline void writeNode(Bytes& b, const Node& n)
{
    b.tag('N').u32(n.id).u16(n.type).u32(n.storage);
    for (const quint32 ref : n.refs) {
        b.tag('S').u32(ref).tag('s');
    }
    for (const Node& child : n.children) {
        writeNode(b, child);
    }
    b.tag('n');
}

// Index « culm » (§ 3).
inline QByteArray index(const Node& root)
{
    Bytes b;
    b.raw(QByteArray("# culm #", 8)).u16(1).u16(7);
    writeNode(b, root);
    return b.data();
}

} // namespace fctest
```

- [ ] **Étape 2 : écrire les tests.** Créer `tests/tst_formats_bamboo.cpp` :

```cpp
#include "support/flipchartwriter.h"
#include "support/peakmemory.h"

#include <actcraft/formats/bamboo.h>

#include <QTest>

#include <miniz.h>

using namespace actcraft::formats::flipchart;
using namespace Qt::StringLiterals;

namespace {

// Flux zlib de `total` octets nuls, compressé par blocs de 1 Mio : jamais `total` octets en mémoire.
QByteArray zlibOfZeros(qint64 total)
{
    mz_stream s{};
    mz_deflateInit(&s, MZ_BEST_COMPRESSION);
    const QByteArray zeros(1 << 20, '\0');
    QByteArray buffer(1 << 16, Qt::Uninitialized);
    QByteArray out;
    for (qint64 done = 0; done < total; done += zeros.size()) {
        const bool last = done + zeros.size() >= total;
        s.next_in = reinterpret_cast<const unsigned char*>(zeros.constData());
        s.avail_in = static_cast<unsigned int>(zeros.size());
        int status = MZ_OK;
        do {
            s.next_out = reinterpret_cast<unsigned char*>(buffer.data());
            s.avail_out = static_cast<unsigned int>(buffer.size());
            status = mz_deflate(&s, last ? MZ_FINISH : MZ_NO_FLUSH);
            out.append(buffer.constData(), buffer.size() - static_cast<qsizetype>(s.avail_out));
        } while (s.avail_in > 0 || (last && status != MZ_STREAM_END));
    }
    mz_deflateEnd(&s);
    return out;
}

} // namespace

// Conteneur Bamboo et index « culm » (format-flipchart.md § 2 et § 3), sur des octets écrits par fctest.
class TstFormatsBamboo : public QObject
{
    Q_OBJECT

private slots:
    void readsCompressedAndRawEntries()
    {
        const QByteArray data = fctest::container(
            {fctest::Entry(u"100.io"_s, QByteArray(3000, 'a')), fctest::Entry(u"115.png"_s, "brut", false)});
        QString error;
        const auto c = BambooContainer::parse(data, error);
        QVERIFY2(c, qPrintable(error));
        QCOMPARE(c->version(), quint16{7});
        QCOMPARE(c->names(), (QStringList{u"100.io"_s, u"115.png"_s}));
        QVERIFY(c->contains(u"115.png"_s));
        QVERIFY(!c->contains(u"absent"_s));
        QCOMPARE(c->read(u"100.io"_s, error).value_or(QByteArray()), QByteArray(3000, 'a'));
        QCOMPARE(c->read(u"115.png"_s, error).value_or(QByteArray()), QByteArray("brut"));
        QVERIFY(!c->read(u"absent"_s, error));
        QVERIFY(error.contains(u"absent"_s));
    }

    void nestedContainerIsAnEntry()
    {
        const QByteArray page = fctest::container({fctest::Entry(u"103.in"_s, "index")});
        const QByteArray data = fctest::container({fctest::Entry(u"103.fpg"_s, page)});
        QString error;
        const auto outer = BambooContainer::parse(data, error);
        QVERIFY(outer);
        const auto inner = BambooContainer::parse(outer->read(u"103.fpg"_s, error).value_or(QByteArray()), error);
        QVERIFY2(inner, qPrintable(error));
        QVERIFY(inner->contains(u"103.in"_s));
    }

    void namesAreUtf16()
    {
        const QByteArray data = fctest::container({fctest::Entry(u"élève.io"_s, "x")});
        QString error;
        QCOMPARE(BambooContainer::parse(data, error)->names(), QStringList{u"élève.io"_s});
    }

    void refusesBadOrTruncatedData()
    {
        QString error;
        QVERIFY(!BambooContainer::parse("pas un conteneur", error));
        QVERIFY(error.contains(u"signature"_s));
        const QByteArray data = fctest::container({fctest::Entry(u"100.io"_s, QByteArray(100, 'z'))});
        for (const qsizetype cut : {qsizetype{10}, qsizetype{40}, data.size() - 3}) {
            error.clear();
            QVERIFY2(!BambooContainer::parse(data.left(cut), error), qPrintable(QString::number(cut)));
            QVERIFY(!error.isEmpty());
        }
    }

    void sizeMismatchIsAnError()
    {
        // Entrée écrite à la main : taille annoncée 99, contenu de 3 octets.
        fctest::Bytes b;
        b.raw(QByteArray(" Bamboo ", 8)).u16(7).qstr(u"1.0"_s).raw(QByteArray(8, '\0'));
        const QByteArray stored = qCompress(QByteArray("abc"));
        b.qstr(u"x.io"_s).qstr(u""_s).qstr(u""_s).qstr(u""_s).u64(99).u8(1).u64(static_cast<quint64>(stored.size()));
        b.raw(stored);
        QString error;
        const auto c = BambooContainer::parse(b.data(), error);
        QVERIFY(c);
        QVERIFY(!c->read(u"x.io"_s, error));
        QVERIFY2(error.contains(u"annoncé"_s), qPrintable(error));
    }

    void oversizedEntryIsRefusedWithoutAllocating()
    {
        fctest::Bytes b;
        b.raw(QByteArray(" Bamboo ", 8)).u16(7).qstr(u"1.0"_s).raw(QByteArray(8, '\0'));
        b.qstr(u"x.io"_s).qstr(u""_s).qstr(u""_s).qstr(u""_s).u64(600ull * 1024 * 1024).u8(1).u64(8);
        b.u32(qToBigEndian(quint32{600u * 1024u * 1024u})).raw("zzzz");
        QString error;
        const auto c = BambooContainer::parse(b.data(), error);
        QVERIFY(c);
        QVERIFY(!c->read(u"x.io"_s, error));
        QVERIFY2(error.contains(u"volumineuse"_s), qPrintable(error));
    }

    void zlibBombStaysWithinTheAnnouncedSize()
    {
        const QByteArray stream = zlibOfZeros(256ll << 20); // ≈ 256 Kio compressés
        fctest::Bytes b;
        b.raw(QByteArray(" Bamboo ", 8)).u16(7).qstr(u"1.0"_s).raw(QByteArray(8, '\0'));
        b.qstr(u"bombe.io"_s).qstr(u""_s).qstr(u""_s).qstr(u""_s).u64(10).u8(1);
        b.u64(static_cast<quint64>(4 + stream.size()));
        b.u32(qToBigEndian(quint32{10})).raw(stream); // en-tête qCompress menteur : 10 octets
        QString error;
        const auto c = BambooContainer::parse(b.data(), error);
        QVERIFY(c);
        const qint64 before = peakMemoryBytes();
        QVERIFY(!c->read(u"bombe.io"_s, error));
        const qint64 rise = peakMemoryBytes() - before;
        QVERIFY2(rise < (32ll << 20), qPrintable(QStringLiteral("hausse du pic : %1 Mio").arg(rise >> 20)));
    }

    void decompressionStopsAtTheAnnouncedSize()
    {
        // Entrée piégée : 10 octets annoncés, flux qui en donnerait 4 Mio (bombe zlib en petit).
        fctest::Bytes b;
        b.raw(QByteArray(" Bamboo ", 8)).u16(7).qstr(u"1.0"_s).raw(QByteArray(8, '\0'));
        const QByteArray stored = qCompress(QByteArray(4 * 1024 * 1024, 'a'));
        b.qstr(u"bombe.io"_s).qstr(u""_s).qstr(u""_s).qstr(u""_s).u64(10).u8(1).u64(static_cast<quint64>(stored.size()));
        b.raw(stored);
        QString error;
        const auto c = BambooContainer::parse(b.data(), error);
        QVERIFY(c);
        QVERIFY(!c->read(u"bombe.io"_s, error));
        QVERIFY2(error.contains(u"annoncés"_s), qPrintable(error));
    }

    void indexKeepsTreeOrderAndReferences()
    {
        const fctest::Node root(101, 1, 100,
                                {fctest::Node(103, 2, 102,
                                              {fctest::Node(104, 29, 105),
                                               fctest::Node(118, 18, 119, {}, {110, 112})}),
                                 fctest::Node(203, 2, 202)});
        QString error;
        const auto tree = parseIndex(fctest::index(root), error);
        QVERIFY2(tree, qPrintable(error));
        QCOMPARE(tree->nodeId, quint32{101});
        QCOMPARE(tree->type, quint16{1});
        QCOMPARE(tree->storageName(), u"100.io"_s);
        QCOMPARE(tree->children.size(), std::size_t{2});
        const IndexNode& page = tree->children[0];
        QCOMPARE(page.nodeId, quint32{103});
        QCOMPARE(page.children.size(), std::size_t{2});
        QCOMPARE(page.children[1].type, quint16{18});
        QCOMPARE(page.children[1].refs, (std::vector<quint32>{110, 112}));
        QCOMPARE(tree->children[1].storageId, quint32{202});
    }

    void indexErrorsAreReported_data()
    {
        QTest::addColumn<QByteArray>("tail");
        QTest::addColumn<QString>("expected");
        QTest::newRow("vide") << QByteArray() << u"vide"_s;
        QTest::newRow("balise") << QByteArray("X") << u"balise"_s;
        QTest::newRow("fermeture") << QByteArray("n") << u"sans nœud"_s;
        fctest::Bytes two;
        two.tag('N').u32(1).u16(1).u32(1).tag('n').tag('N').u32(2).u16(1).u32(2).tag('n');
        QTest::newRow("deux racines") << two.data() << u"racines"_s;
        fctest::Bytes open;
        open.tag('N').u32(1).u16(1).u32(1);
        QTest::newRow("non fermé") << open.data() << u"fermé"_s;
    }

    void indexErrorsAreReported()
    {
        QFETCH(QByteArray, tail);
        QFETCH(QString, expected);
        fctest::Bytes b;
        b.raw(QByteArray("# culm #", 8)).u16(1).u16(7).raw(tail);
        QString error;
        QVERIFY(!parseIndex(b.data(), error));
        QVERIFY2(error.contains(expected), qPrintable(error));
        QVERIFY(!parseIndex("pas un index", error));
    }
};

QTEST_GUILESS_MAIN(TstFormatsBamboo)
#include "tst_formats_bamboo.moc"
```

  Ajouter à `tests/flipchart.cmake` :

```cmake
actcraft_add_test(tst_formats_bamboo
    SOURCES tst_formats_bamboo.cpp
    LIBS ActCraft::formats ActCraft::miniz)
if(WIN32)
    target_link_libraries(tst_formats_bamboo PRIVATE psapi) # GetProcessMemoryInfo (peakmemory.h)
endif()
```

  Créer `tests/support/peakmemory.h` (aide de test, jamais livrée) :

```cpp
#pragma once

// Pic de mémoire résidente du processus, en octets (tests de bornes mémoire).
#include <QtGlobal>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// windows.h doit précéder psapi.h : clang-format (SortIncludes) ne doit pas les inverser (même bloc que
// bench/src/memory.cpp).
// clang-format off
#include <windows.h>
#include <psapi.h>
// clang-format on
#else
#include <sys/resource.h>
#endif

inline qint64 peakMemoryBytes()
{
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof counters;
    return GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof counters)
               ? static_cast<qint64>(counters.PeakWorkingSetSize)
               : 0;
#else
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
#ifdef Q_OS_MACOS
    return static_cast<qint64>(usage.ru_maxrss); // octets sous macOS
#else
    return static_cast<qint64>(usage.ru_maxrss) * 1024; // Kio ailleurs
#endif
#endif
}
```

- [ ] **Étape 3 : constater le rouge.** Commande ciblée, motif `tst_formats_bamboo` → `error`.

- [ ] **Étape 4 : écrire l'en-tête.** Créer `formats/include/actcraft/formats/bamboo.h` :

```cpp
#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>

#include <cstddef>
#include <optional>
#include <vector>

namespace actcraft::formats::flipchart {

// Une entrée décompressée de plus de 512 Mio est refusée (fichier non fiable).
inline constexpr qint64 kMaxEntrySize = 512ll * 1024 * 1024;

struct BambooEntry
{
    QString name;
    quint64 size = 0; // taille décompressée annoncée
    bool compressed = false;
    QByteArray raw; // octets stockés
};

// Conteneur « Bamboo » d'un .flipchart (format-flipchart.md § 2) : entrées nommées, compressées par zlib avec
// l'en-tête de qCompress. Les pages (*.fpg) sont des conteneurs imbriqués : parse() sur leurs octets.
class BambooContainer
{
public:
    static std::optional<BambooContainer> parse(const QByteArray& data, QString& error);
    QStringList names() const; // ordre du fichier
    bool contains(const QString& name) const;
    // Contenu décompressé ; nullopt et `error` si absente, trop grosse, illisible ou d'une autre taille que
    // l'annoncée.
    std::optional<QByteArray> read(const QString& name, QString& error) const;
    quint16 version() const noexcept;

private:
    std::vector<BambooEntry> m_entries;
    QHash<QString, std::size_t> m_byName; // un nom en double : la dernière entrée l'emporte
    quint16 m_version = 0;
};

// Nœud de l'index « culm » (§ 3) : la hiérarchie fait foi, pas la liste des entrées.
struct IndexNode
{
    quint32 nodeId = 0;
    quint16 type = 0;
    quint32 storageId = 0;
    std::vector<IndexNode> children;
    std::vector<quint32> refs; // balises 'S' : membres d'un groupe, rangés ailleurs dans l'arbre
    QString storageName() const; // « <storageId>.io »
};

std::optional<IndexNode> parseIndex(const QByteArray& data, QString& error);

} // namespace actcraft::formats::flipchart
```

- [ ] **Étape 5 : écrire l'implémentation.** Créer `formats/src/bamboo.cpp` :

```cpp
#include <actcraft/formats/bamboo.h>

#include <QStringDecoder>
#include <QtEndian>

#include <miniz.h>

namespace actcraft::formats::flipchart {

using namespace Qt::StringLiterals;

namespace {

// Lecture bornée : chaque méthode rend faux au lieu de lire hors des données.
class Cursor
{
public:
    Cursor(const QByteArray& data, qsizetype pos) : m_data(data), m_pos(pos) {}

    bool atEnd() const noexcept { return m_pos >= m_data.size(); }
    qsizetype pos() const noexcept { return m_pos; }
    bool has(qsizetype n) const noexcept { return n >= 0 && m_pos <= m_data.size() - n; }

    template <typename T> bool le(T& out)
    {
        constexpr auto n = static_cast<qsizetype>(sizeof(T));
        if (!has(n)) {
            return false;
        }
        out = qFromLittleEndian<T>(m_data.constData() + m_pos);
        m_pos += n;
        return true;
    }
    bool bytes(qsizetype n, QByteArray& out)
    {
        if (!has(n)) {
            return false;
        }
        out = m_data.mid(m_pos, n);
        m_pos += n;
        return true;
    }
    bool tag(char& out)
    {
        if (!has(1)) {
            return false;
        }
        out = m_data.at(m_pos);
        ++m_pos;
        return true;
    }
    // u32 longueur en octets (0xFFFFFFFF = chaîne nulle), puis UTF-16LE.
    bool qstring(QString& out)
    {
        quint32 n = 0;
        if (!le(n)) {
            return false;
        }
        if (n == 0xFFFFFFFFu) {
            out.clear();
            return true;
        }
        QByteArray b;
        if (n % 2 != 0 || !bytes(static_cast<qsizetype>(n), b)) {
            return false;
        }
        out = QStringDecoder(QStringDecoder::Utf16LE)(b);
        return true;
    }

private:
    const QByteArray& m_data;
    qsizetype m_pos;
};

} // namespace

std::optional<BambooContainer> BambooContainer::parse(const QByteArray& data, QString& error)
{
    if (!data.startsWith(" Bamboo ")) {
        error = u"signature Bamboo absente"_s;
        return std::nullopt;
    }
    Cursor c(data, 8);
    BambooContainer out;
    QString format;
    QByteArray reserved;
    if (!c.le(out.m_version) || !c.qstring(format) || !c.bytes(8, reserved)) {
        error = u"en-tête du conteneur tronqué"_s;
        return std::nullopt;
    }
    while (!c.atEnd()) {
        const qsizetype at = c.pos();
        BambooEntry e;
        QString created;
        QString modified;
        QString accessed;
        quint8 compressed = 0;
        quint64 stored = 0;
        if (!c.qstring(e.name) || !c.qstring(created) || !c.qstring(modified) || !c.qstring(accessed) ||
            !c.le(e.size) || !c.le(compressed) || !c.le(stored)) {
            error = u"entrée tronquée à l'octet %1"_s.arg(at);
            return std::nullopt;
        }
        if (stored > static_cast<quint64>(data.size()) || !c.bytes(static_cast<qsizetype>(stored), e.raw)) {
            error = u"données de l'entrée « %1 » tronquées"_s.arg(e.name);
            return std::nullopt;
        }
        e.compressed = compressed != 0;
        out.m_byName.insert(e.name, out.m_entries.size());
        out.m_entries.push_back(std::move(e));
    }
    return out;
}

QStringList BambooContainer::names() const
{
    QStringList out;
    for (const BambooEntry& e : m_entries) {
        out << e.name;
    }
    return out;
}

bool BambooContainer::contains(const QString& name) const
{
    return m_byName.contains(name);
}

std::optional<QByteArray> BambooContainer::read(const QString& name, QString& error) const
{
    const auto it = m_byName.constFind(name);
    if (it == m_byName.cend()) {
        error = u"entrée « %1 » absente"_s.arg(name);
        return std::nullopt;
    }
    const BambooEntry& e = m_entries[it.value()];
    if (e.size > static_cast<quint64>(kMaxEntrySize)) {
        error = u"entrée « %1 » trop volumineuse"_s.arg(name);
        return std::nullopt;
    }
    if (!e.compressed) {
        if (static_cast<quint64>(e.raw.size()) != e.size) {
            error = u"entrée « %1 » : %2 octets stockés, %3 annoncés"_s.arg(name).arg(e.raw.size()).arg(e.size);
            return std::nullopt;
        }
        return e.raw;
    }
    if (e.raw.size() < 4) {
        error = u"entrée « %1 » : bloc compressé trop court"_s.arg(name);
        return std::nullopt;
    }
    if (e.size == 0) {
        return QByteArray();
    }
    // Décompression dans un tampon de la taille annoncée par l'entrée (≤ kMaxEntrySize), jamais plus : l'en-tête de
    // 4 octets de qCompress est fixé par le fichier et qUncompress le dépasse (relecture du plan, tour 1 : en-tête
    // de 16 octets, entrée de 597 Kio → 600 Mio). mz_uncompress s'arrête au bout du tampon (MZ_BUF_ERROR).
    QByteArray out(static_cast<qsizetype>(e.size), Qt::Uninitialized);
    mz_ulong length = static_cast<mz_ulong>(e.size);
    const int status = mz_uncompress(reinterpret_cast<unsigned char*>(out.data()), &length,
                                     reinterpret_cast<const unsigned char*>(e.raw.constData()) + 4,
                                     static_cast<mz_ulong>(e.raw.size() - 4));
    if (status != MZ_OK || length != static_cast<mz_ulong>(e.size)) {
        error = u"entrée « %1 » : flux zlib illisible ou d'une autre taille que les %2 octets annoncés"_s.arg(name).arg(
            e.size);
        return std::nullopt;
    }
    return out;
}

quint16 BambooContainer::version() const noexcept
{
    return m_version;
}

QString IndexNode::storageName() const
{
    return u"%1.io"_s.arg(storageId);
}

std::optional<IndexNode> parseIndex(const QByteArray& data, QString& error)
{
    if (!data.startsWith("# culm #")) {
        error = u"signature d'index absente"_s;
        return std::nullopt;
    }
    Cursor c(data, 12);
    std::optional<IndexNode> root;
    std::vector<IndexNode*> stack; // chemin de la racine au nœud ouvert ; seul le dernier enfant est ouvert
    while (!c.atEnd()) {
        const qsizetype at = c.pos();
        char tag = 0;
        c.tag(tag);
        if (tag == 'N') {
            IndexNode node;
            if (!c.le(node.nodeId) || !c.le(node.type) || !c.le(node.storageId)) {
                error = u"nœud tronqué à l'octet %1"_s.arg(at);
                return std::nullopt;
            }
            if (!stack.empty()) {
                stack.back()->children.push_back(std::move(node));
                stack.push_back(&stack.back()->children.back());
            } else if (!root) {
                root = std::move(node);
                stack.push_back(&*root);
            } else {
                error = u"plusieurs racines dans l'index"_s;
                return std::nullopt;
            }
            if (stack.size() > 256) {
                error = u"arbre trop profond"_s;
                return std::nullopt;
            }
        } else if (tag == 'S') {
            quint32 ref = 0;
            char close = 0;
            if (!c.le(ref) || !c.tag(close) || close != 's') {
                error = u"référence mal fermée à l'octet %1"_s.arg(at);
                return std::nullopt;
            }
            if (stack.empty()) {
                error = u"référence hors nœud à l'octet %1"_s.arg(at);
                return std::nullopt;
            }
            stack.back()->refs.push_back(ref);
        } else if (tag == 'n') {
            if (stack.empty()) {
                error = u"'n' sans nœud ouvert à l'octet %1"_s.arg(at);
                return std::nullopt;
            }
            stack.pop_back();
        } else {
            error = u"balise d'index inconnue à l'octet %1"_s.arg(at);
            return std::nullopt;
        }
    }
    if (!root) {
        error = u"index vide"_s;
        return std::nullopt;
    }
    if (!stack.empty()) {
        error = u"nœud non fermé en fin d'index"_s;
        return std::nullopt;
    }
    return root;
}

} // namespace actcraft::formats::flipchart
```

  Le test `fermeture` attend « sans nœud » : le message `'n' sans nœud ouvert` le contient. Le test `non fermé`
  attend « fermé » : `nœud non fermé en fin d'index`. (La référence Python accepte un nœud non fermé ; ici on le
  refuse : un index tronqué ne doit pas passer pour complet. Écart écrit au rapport ; si l'oracle (L2-T10) montre
  un échantillon réel concerné, le consigner sans changer la règle.)

  Remplir `formats/flipchart.cmake` :

```cmake
set(ACTCRAFT_FORMATS_FLIPCHART_SOURCES
    include/actcraft/formats/bamboo.h
    src/bamboo.cpp)
```

- [ ] **Étape 6 : vert ciblé.** Commande ciblée, motif `tst_formats_bamboo` : `100% tests passed`.

- [ ] **Étape 7 : preuve.** `format.ps1`, preuve de tâche (N ≥ 46 ; `tst_formats_bamboo` Passed).

- [ ] **Étape 8 : commit.**

```powershell
git -C <worktree> add formats/include/actcraft/formats/bamboo.h formats/src/bamboo.cpp formats/flipchart.cmake tests/flipchart.cmake tests/support/flipchartwriter.h tests/support/peakmemory.h tests/tst_formats_bamboo.cpp
git -C <worktree> commit -m "formats : conteneur Bamboo et index culm du .flipchart (lecture bornée) ; écrivain de paperboards synthétiques" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t8"></a>
### Tâche L2-T8 — `formats` : flux objet (variantes, sous-types, saut des propriétés illisibles)

**Objectif.** DA4 : deuxième couche du lecteur, d'après `format-flipchart.md` § 4 (grammaire, identifiants, types
de variante, sous-types) ; comportement de référence : `_read_object`, la lecture d'une valeur, `_read_user` de
`app/actcraft/io/objstream.py`. Un flux `.io`/`.ir` donne un `StreamObject` générique : type, identifiant de
stockage, nœud propriétaire éventuel, propriétés **par numéro** (`std::map<quint16, Value>`). Aucune table de noms
ici : les numéros sont interprétés par L2-T9. Une propriété qu'on ne sait pas décoder devient `RawValue` grâce à la
table des tailles (`M`), sans faire échouer l'objet. Bornes : profondeur 32, compteurs 5 000 000, 10 000 propriétés.

**Fichiers.**
- Créer : `formats/include/actcraft/formats/objectstream.h`, `formats/src/objectstream.cpp`,
  `tests/tst_formats_objectstream.cpp`
- Modifier : `formats/flipchart.cmake`, `tests/flipchart.cmake`, `tests/support/flipchartwriter.h`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_formats_objectstream` → `error`. Vert : forme
bornée, N ≥ 47, `TESTS OK`, `tst_formats_objectstream .*Passed`.

**Interdits.** QtCore seulement ; pas de noms de propriétés d'Inspire. Pas d'exception, pas de lecture hors
limites.

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : compléter l'écrivain de test.** Dans `tests/support/flipchartwriter.h`, ajouter les includes
  `<QPointF>` et `<optional>`, et avant la fermeture de l'espace `fctest` :

```cpp
// Entrée du registre de ressources (sous-type 3).
struct Res
{
    Res(quint32 i, QString e, QString f, QString d = QStringLiteral("unknown"), quint32 r = 1)
        : id(i), entry(std::move(e)), dataFile(std::move(f)), digest(std::move(d)), refCount(r)
    {
    }
    quint32 id;
    QString entry;
    QString dataFile;
    QString digest;
    quint32 refCount;
};

// Objet du flux (§ 4.1) : 'O' type stockage 'M' n tailles enregistrements 'p' ['R' nœud 'r'] 'o'. Les
// propriétés sont des numéros : l'écrivain ne connaît pas leurs noms.
class Obj
{
public:
    Obj(quint16 type, quint32 storage) : m_type(type), m_storage(storage) {}

    Obj& boolean(quint16 id, bool v) { return add(id, Bytes().u16(1).u8(v ? 1 : 0)); }
    Obj& integer(quint16 id, qint32 v) { return add(id, Bytes().u16(2).i32(v)); }
    Obj& uinteger(quint16 id, quint32 v) { return add(id, Bytes().u16(3).u32(v)); }
    Obj& real(quint16 id, double v) { return add(id, Bytes().u16(6).f64(v)); }
    Obj& string(quint16 id, const QString& v) { return add(id, Bytes().u16(10).str16(v)); }
    Obj& stringList(quint16 id, const QStringList& v)
    {
        Bytes b;
        b.u16(11).u16(1).i32(static_cast<qint32>(v.size()));
        for (const QString& s : v) {
            b.qstr(s);
        }
        return add(id, b);
    }
    Obj& bytes(quint16 id, const QByteArray& v) { return add(id, Bytes().u16(12).u32(static_cast<quint32>(v.size())).raw(v)); }
    Obj& floatRect(quint16 id, double x, double y, double w, double h)
    {
        return add(id, Bytes().u16(20).f64(x).f64(y).f64(w).f64(h));
    }
    Obj& size(quint16 id, qint32 w, qint32 h) { return add(id, Bytes().u16(21).i32(w).i32(h)); }
    Obj& sizeF(quint16 id, double w, double h) { return add(id, Bytes().u16(22).f64(w).f64(h)); }
    Obj& pointF(quint16 id, double x, double y) { return add(id, Bytes().u16(26).f64(x).f64(y)); }
    Obj& color(quint16 id, quint8 r, quint8 g, quint8 b, quint8 a = 255)
    {
        return add(id, Bytes().u16(67).u8(r).u8(g).u8(b).u8(a));
    }
    Obj& points(quint16 id, const std::vector<QPointF>& v)
    {
        Bytes b;
        b.u16(127).u16(1).i32(static_cast<qint32>(v.size()));
        appendPoints(b, v);
        return add(id, b);
    }
    Obj& polygons(quint16 id, const std::vector<std::vector<QPointF>>& v)
    {
        Bytes b;
        b.u16(127).u16(2).i32(static_cast<qint32>(v.size()));
        for (const auto& poly : v) {
            b.i32(static_cast<qint32>(poly.size()));
            appendPoints(b, poly);
        }
        return add(id, b);
    }
    Obj& resources(quint16 id, const std::vector<Res>& v)
    {
        Bytes b;
        b.u16(127).u16(3).i32(static_cast<qint32>(v.size()));
        for (const Res& r : v) {
            b.u32(r.id).str16(r.entry).str16(r.dataFile).str16(r.digest).u32(r.refCount);
        }
        return add(id, b);
    }
    // Objet embarqué (sous-types 4, 6, 7, 8, 12, 14, 18).
    Obj& object(quint16 id, quint16 subtype, const Obj& inner)
    {
        return add(id, Bytes().u16(127).u16(subtype).raw(inner.body()));
    }
    Obj& userRect(quint16 id, double x, double y, double w, double h)
    {
        return add(id, Bytes().u16(127).u16(5).f64(x).f64(y).f64(w).f64(h));
    }
    Obj& pointer(quint16 id, quint32 node, quint32 storage, quint16 type)
    {
        return add(id, Bytes().u16(127).u16(9).u32(node).u32(storage).u16(type));
    }
    Obj& runs(quint16 id, const std::vector<Obj>& v)
    {
        Bytes b;
        b.u16(127).u16(10).i32(static_cast<qint32>(v.size()));
        for (const Obj& o : v) {
            b.raw(o.body());
        }
        return add(id, b);
    }
    // Variante quelconque (type et octets donnés) : types inconnus, valeurs illisibles.
    Obj& raw(quint16 id, const QByteArray& variant) { return add(id, Bytes().raw(variant)); }
    Obj& owner(quint32 node)
    {
        m_owner = node;
        return *this;
    }

    // L'objet sans en-tête de flux (objet embarqué).
    QByteArray body() const
    {
        std::vector<QByteArray> records;
        for (std::size_t i = 0; i < m_props.size(); ++i) {
            Bytes r;
            r.tag(i == 0 ? 'm' : 'p').tag('P').u16(m_props[i].first).raw(m_props[i].second);
            records.push_back(r.data());
        }
        Bytes b;
        b.tag('O').u16(m_type).u32(m_storage).tag('M').u32(static_cast<quint32>(records.size()));
        for (const QByteArray& r : records) {
            b.u32(static_cast<quint32>(r.size()));
        }
        for (const QByteArray& r : records) {
            b.raw(r);
        }
        b.tag('p');
        if (m_owner) {
            b.tag('R').u32(*m_owner).tag('r');
        }
        b.tag('o');
        return b.data();
    }
    // Fichier .io / .ir : en-tête de flux " Bamboo " u16 1 u16 7, puis l'objet.
    QByteArray stream() const
    {
        Bytes b;
        b.raw(QByteArray(" Bamboo ", 8)).u16(1).u16(7).raw(body());
        return b.data();
    }

private:
    static void appendPoints(Bytes& b, const std::vector<QPointF>& v)
    {
        Bytes doubles;
        for (const QPointF& p : v) {
            doubles.f64(p.x()).f64(p.y());
        }
        b.u32(static_cast<quint32>(doubles.data().size())).raw(doubles.data());
    }
    Obj& add(quint16 id, const Bytes& variant)
    {
        m_props.emplace_back(id, variant.data());
        return *this;
    }

    quint16 m_type;
    quint32 m_storage;
    std::optional<quint32> m_owner;
    std::vector<std::pair<quint16, QByteArray>> m_props;
};
```

  `Bytes().u16(…)` sur un temporaire : les méthodes de `Bytes` rendent `Bytes&`, et `add(quint16, const Bytes&)` lit
  l'objet temporaire avant la fin de l'expression : correct.

- [ ] **Étape 2 : écrire les tests.** Créer `tests/tst_formats_objectstream.cpp` :

```cpp
#include "support/flipchartwriter.h"

#include <actcraft/formats/objectstream.h>

#include <QTest>

using namespace actcraft::formats::flipchart;
using namespace Qt::StringLiterals;

// Flux objet (format-flipchart.md § 4) : chaque type de variante et de sous-type, saut des propriétés illisibles.
class TstFormatsObjectStream : public QObject
{
    Q_OBJECT

private slots:
    void readsEveryScalarType()
    {
        const QByteArray data = fctest::Obj(7, 335)
                                    .boolean(2, true)
                                    .integer(3, -5)
                                    .uinteger(4, 4000000000u)
                                    .real(5, 2.5)
                                    .string(6, u"Forme24"_s)
                                    .stringList(7, {u"a"_s, u"b"_s})
                                    .bytes(8, "xyz")
                                    .floatRect(9, 1, 2, 3, 4)
                                    .size(10, 320, 240)
                                    .sizeF(11, 1024, 768)
                                    .pointF(12, 5, 6)
                                    .color(13, 10, 20, 30, 128)
                                    .owner(0x67)
                                    .stream();
        QString error;
        const auto o = parseObjectStream(data, error);
        QVERIFY2(o, qPrintable(error));
        QCOMPARE(o->type, quint16{7});
        QCOMPARE(o->storageId, quint32{335});
        QCOMPARE(o->nodeId.value_or(0), quint32{0x67});
        QCOMPARE(asBool(o->find(2)).value_or(false), true);
        QCOMPARE(asNumber(o->find(3)).value_or(0), -5.0);
        QCOMPARE(asNumber(o->find(4)).value_or(0), 4000000000.0);
        QCOMPARE(asNumber(o->find(5)).value_or(0), 2.5);
        QCOMPARE(asString(o->find(6)), u"Forme24"_s);
        QCOMPARE(std::get<QStringList>(o->find(7)->data), (QStringList{u"a"_s, u"b"_s}));
        QCOMPARE(std::get<QByteArray>(o->find(8)->data), QByteArray("xyz"));
        QCOMPARE(asRect(o->find(9))->width, 3.0);
        QCOMPARE(asSize(o->find(10))->width, 320.0);
        QCOMPARE(asSize(o->find(11))->height, 768.0);
        QCOMPARE(asPoint(o->find(12))->y, 6.0);
        const Color c = asColor(o->find(13)).value_or(Color{});
        QCOMPARE(c.r, quint8{10});
        QCOMPARE(c.a, quint8{128});
        QVERIFY(o->find(99) == nullptr);
        QVERIFY(!asNumber(o->find(6))); // une chaîne n'est pas un nombre
    }

    void readsUserSubtypes()
    {
        const fctest::Obj ink = fctest::Obj(20, 0).real(3, 50).color(7, 255, 255, 0, 128);
        const fctest::Obj run = fctest::Obj(31, 0).string(6, u"Bonjour"_s);
        const QByteArray data =
            fctest::Obj(3, 111)
                .points(2, {QPointF(0, 0), QPointF(100, 0), QPointF(100, 50)})
                .polygons(3, {{QPointF(0, 0), QPointF(1, 1)}, {QPointF(2, 2)}})
                .resources(4, {fctest::Res(165, u"165.ir"_s, u"115.png"_s, u"7370"_s, 1)})
                .object(5, 6, ink)
                .userRect(6, -3, -3, 107.5, 107.5)
                .pointer(7, 116, 117, 11)
                .runs(8, {run, run})
                .stream();
        QString error;
        const auto o = parseObjectStream(data, error);
        QVERIFY2(o, qPrintable(error));
        QCOMPARE(asPolyline(o->find(2)).size(), std::size_t{3});
        QCOMPARE(asPolyline(o->find(2))[2].y, 50.0);
        QCOMPARE(asPolylines(o->find(3)).size(), std::size_t{2});
        QCOMPARE(asPolylines(o->find(3))[1].size(), std::size_t{1});
        const auto items = asResourceItems(o->find(4));
        QCOMPARE(items.size(), std::size_t{1});
        QCOMPARE(items[0].id, quint32{165});
        QCOMPARE(items[0].dataFile, u"115.png"_s);
        const StreamObject* embedded = asObject(o->find(5));
        QVERIFY(embedded);
        QCOMPARE(embedded->type, quint16{20});
        QCOMPARE(asNumber(embedded->find(3)).value_or(0), 50.0);
        QCOMPARE(asRect(o->find(6))->width, 107.5);
        const Pointer p = asPointer(o->find(7)).value_or(Pointer{});
        QCOMPARE(p.nodeId, quint32{116});
        QCOMPARE(p.type, quint16{11});
        QVERIFY(!p.isNull());
        QCOMPARE(asObjects(o->find(8)).size(), std::size_t{2});
        QCOMPARE(asString(asObjects(o->find(8))[0]->find(6)), u"Bonjour"_s);
    }

    void unreadablePropertyBecomesRawAndTheRestIsRead()
    {
        // Variante de type 70 (QImage, inconnue) puis une propriété lisible : la table M permet de sauter.
        fctest::Bytes unknown;
        unknown.u16(70).raw("????");
        // Chaîne qui annonce plus d'octets qu'elle n'en a : illisible, gardée brute.
        fctest::Bytes broken;
        broken.u16(10).u16(1).u32(1000).raw("ab");
        const QByteArray data =
            fctest::Obj(9, 1).raw(2, unknown.data()).raw(3, broken.data()).integer(4, 42).stream();
        QString error;
        const auto o = parseObjectStream(data, error);
        QVERIFY2(o, qPrintable(error));
        QVERIFY(std::holds_alternative<RawValue>(o->find(2)->data));
        QCOMPARE(std::get<RawValue>(o->find(2)->data).qtype, quint16{70});
        QVERIFY(std::holds_alternative<RawValue>(o->find(3)->data));
        QCOMPARE(asNumber(o->find(4)).value_or(0), 42.0);
    }

    void nullStringAndNullPointer()
    {
        fctest::Bytes nullString;
        nullString.u16(10).u16(1).nullQstr();
        const QByteArray data = fctest::Obj(2, 1).raw(2, nullString.data()).pointer(3, 0, 0, 0).stream();
        QString error;
        const auto o = parseObjectStream(data, error);
        QVERIFY(o);
        QVERIFY(asString(o->find(2)).isNull());
        QVERIFY(asPointer(o->find(3))->isNull());
    }

    void badStreamsAreRefused()
    {
        QString error;
        QVERIFY(!parseObjectStream("pas un flux", error));
        QVERIFY(error.contains(u"signature"_s));
        const QByteArray good = fctest::Obj(7, 1).integer(2, 1).stream();
        QVERIFY(!parseObjectStream(good.left(15), error));
        fctest::Bytes wrongTag;
        wrongTag.raw(QByteArray(" Bamboo ", 8)).u16(1).u16(7).tag('X');
        QVERIFY(!parseObjectStream(wrongTag.data(), error));
        QVERIFY(error.contains(u"'O'"_s));
    }

    void nestingDepthIsBounded()
    {
        fctest::Obj inner(20, 0);
        for (int depth = 0; depth < 40; ++depth) {
            inner = fctest::Obj(20, 0).object(2, 6, inner);
        }
        QString error;
        const auto o = parseObjectStream(inner.stream(), error);
        // Trop profond : la propriété fautive devient brute, l'objet racine reste lisible.
        QVERIFY2(o, qPrintable(error));
    }
};

QTEST_GUILESS_MAIN(TstFormatsObjectStream)
#include "tst_formats_objectstream.moc"
```

  Ajouter à `tests/flipchart.cmake` :

```cmake
actcraft_add_test(tst_formats_objectstream
    SOURCES tst_formats_objectstream.cpp
    LIBS ActCraft::formats)
```

- [ ] **Étape 3 : constater le rouge.** Commande ciblée, motif `tst_formats_objectstream` → `error`.

- [ ] **Étape 4 : écrire l'en-tête.** Créer `formats/include/actcraft/formats/objectstream.h` :

```cpp
#pragma once

#include <actcraft/core/model.h>

#include <QByteArray>
#include <QString>
#include <QStringList>

#include <map>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace actcraft::formats::flipchart {

struct SizeF
{
    double width = 0;
    double height = 0;
};

struct Color
{
    quint8 r = 0;
    quint8 g = 0;
    quint8 b = 0;
    quint8 a = 255;
};

// Référence vers un nœud de l'arbre (sous-type 9) ; (0, 0, 0) = nulle.
struct Pointer
{
    quint32 nodeId = 0;
    quint32 storageId = 0;
    quint16 type = 0;
    bool isNull() const noexcept { return nodeId == 0 && storageId == 0; }
};

// Entrée du registre de ressources (sous-type 3).
struct ResourceItem
{
    quint32 id = 0;
    QString entry;    // entrée qui porte l'objet ressource, ex. « 165.ir »
    QString dataFile; // fichier de données brutes, ex. « 115.png » ; vide si aucun
    QString digest;
    quint32 refCount = 0;
};

// Valeur non décodée : octets bruts conservés (type Qt 4, sous-type éventuel).
struct RawValue
{
    quint16 qtype = 0;
    quint16 subtype = 0;
    QByteArray data;
};

struct StreamObject;
using ObjectPtr = std::shared_ptr<const StreamObject>;
using Polyline = std::vector<core::PointF>;
struct Value;
using ValueList = std::vector<Value>;

// Valeur d'une propriété (format-flipchart.md § 4.3). QRect et QSize sont rendus en doubles (RectF, SizeF) ;
// QTime et QDateTime en texte ISO.
struct Value
{
    std::variant<std::monostate, bool, qint32, quint32, double, QString, QStringList, QByteArray, core::PointF, SizeF,
                 core::RectF, Color, Polyline, std::vector<Polyline>, std::vector<ResourceItem>, Pointer, ObjectPtr,
                 std::vector<ObjectPtr>, ValueList, RawValue>
        data;
};

// Objet générique d'un flux .io / .ir (§ 4.1) : propriétés par numéro (les noms sont l'affaire de L2-T9).
struct StreamObject
{
    quint16 type = 0;
    quint32 storageId = 0;
    std::optional<quint32> nodeId; // 'R' : nœud propriétaire (objets de premier niveau)
    std::map<quint16, Value> properties;

    const Value* find(quint16 id) const;
};

// Décode un fichier .io / .ir (en-tête « Bamboo » u16 1 u16 7, puis un objet). nullopt et `error` si
// l'en-tête ou l'objet racine est illisible ; une propriété illisible devient RawValue sans faire échouer l'objet.
std::optional<StreamObject> parseObjectStream(const QByteArray& data, QString& error);

// Accesseurs tolérants : nullopt (ou vide) si la valeur est absente ou d'un autre type.
std::optional<double> asNumber(const Value* v); // booléen, entier ou réel
std::optional<bool> asBool(const Value* v);     // booléen ou entier
QString asString(const Value* v);               // chaîne nulle si absente
std::optional<core::PointF> asPoint(const Value* v);
std::optional<SizeF> asSize(const Value* v);
std::optional<core::RectF> asRect(const Value* v); // QRectF, QRect ou rectangle réel propre au format
std::optional<Color> asColor(const Value* v);
std::optional<Pointer> asPointer(const Value* v);
const StreamObject* asObject(const Value* v);
std::vector<const StreamObject*> asObjects(const Value* v);
Polyline asPolyline(const Value* v);
std::vector<Polyline> asPolylines(const Value* v);
std::vector<ResourceItem> asResourceItems(const Value* v);

} // namespace actcraft::formats::flipchart
```

- [ ] **Étape 5 : écrire l'implémentation.** Créer `formats/src/objectstream.cpp` :

```cpp
#include <actcraft/formats/objectstream.h>

#include <QStringDecoder>
#include <QtEndian>

#include <cstring>

namespace actcraft::formats::flipchart {

using namespace Qt::StringLiterals;

namespace {

constexpr int kMaxDepth = 32;
constexpr qint32 kMaxCount = 5'000'000;
constexpr quint32 kMaxProperties = 10'000;

// Lecteur borné d'un intervalle [pos, end) ; `ok` passe à faux à la première lecture impossible.
class Reader
{
public:
    Reader(const QByteArray& data, qsizetype pos, qsizetype end) : m_data(data), m_pos(pos), m_end(end) {}

    bool ok() const noexcept { return m_ok; }
    const QByteArray& data() const noexcept { return m_data; }
    qsizetype pos() const noexcept { return m_pos; }
    void seek(qsizetype pos) noexcept { m_pos = pos; }
    qsizetype end() const noexcept { return m_end; }
    bool has(qsizetype n) const noexcept { return n >= 0 && m_pos <= m_end - n; }
    char peek() const noexcept { return m_pos < m_end ? m_data.at(m_pos) : '\0'; }

    template <typename T> T le()
    {
        constexpr auto n = static_cast<qsizetype>(sizeof(T));
        if (!m_ok || !has(n)) {
            m_ok = false;
            return T{};
        }
        const T v = qFromLittleEndian<T>(m_data.constData() + m_pos);
        m_pos += n;
        return v;
    }
    double f64()
    {
        const quint64 bits = le<quint64>();
        double v = 0;
        std::memcpy(&v, &bits, sizeof v);
        return v;
    }
    char tag() { return static_cast<char>(le<quint8>()); }
    QByteArray take(qsizetype n)
    {
        if (!m_ok || !has(n)) {
            m_ok = false;
            return {};
        }
        QByteArray out = m_data.mid(m_pos, n);
        m_pos += n;
        return out;
    }
    qint32 count()
    {
        const qint32 n = le<qint32>();
        if (n < 0 || n > kMaxCount) {
            m_ok = false;
            return 0;
        }
        return n;
    }
    QString qstring()
    {
        const quint32 n = le<quint32>();
        if (!m_ok || n == 0xFFFFFFFFu) {
            return QString();
        }
        if (n % 2 != 0) {
            m_ok = false;
            return QString();
        }
        const QByteArray b = take(static_cast<qsizetype>(n));
        return m_ok ? QString(QStringDecoder(QStringDecoder::Utf16LE)(b)) : QString();
    }
    QByteArray qbytearray()
    {
        const quint32 n = le<quint32>();
        if (!m_ok || n == 0xFFFFFFFFu) {
            return QByteArray();
        }
        return take(static_cast<qsizetype>(n));
    }
    QString str16()
    {
        le<quint16>(); // préfixe constant 1 (§ 8 : rôle inconnu)
        return qstring();
    }

private:
    const QByteArray& m_data;
    qsizetype m_pos;
    qsizetype m_end;
    bool m_ok = true;
};

Polyline pointsOf(const QByteArray& blob)
{
    Polyline out;
    const qsizetype n = blob.size() / 16;
    out.reserve(static_cast<std::size_t>(n));
    for (qsizetype i = 0; i < n; ++i) {
        double xy[2] = {0, 0};
        std::memcpy(xy, blob.constData() + i * 16, 16);
        out.push_back(core::PointF{qFromLittleEndian(xy[0]), qFromLittleEndian(xy[1])});
    }
    return out;
}

std::optional<StreamObject> readObject(Reader& r, int depth, QString& error);

Value readUser(Reader& r, quint16 sub, int depth, QString& error)
{
    Value v;
    switch (sub) {
    case 1: {
        r.le<qint32>();
        v.data = pointsOf(r.qbytearray());
        break;
    }
    case 2: {
        std::vector<Polyline> polys;
        const qint32 n = r.count();
        for (qint32 i = 0; i < n && r.ok(); ++i) {
            r.le<qint32>();
            polys.push_back(pointsOf(r.qbytearray()));
        }
        v.data = std::move(polys);
        break;
    }
    case 3: {
        std::vector<ResourceItem> items;
        const qint32 n = r.count();
        for (qint32 i = 0; i < n && r.ok(); ++i) {
            ResourceItem item;
            item.id = r.le<quint32>();
            item.entry = r.str16();
            item.dataFile = r.str16();
            item.digest = r.str16();
            item.refCount = r.le<quint32>();
            items.push_back(std::move(item));
        }
        v.data = std::move(items);
        break;
    }
    case 4:
    case 6:
    case 7:
    case 8:
    case 12:
    case 14:
    case 18: {
        std::optional<StreamObject> o = readObject(r, depth + 1, error);
        if (!o) {
            r.seek(r.end() + 1); // marque l'échec : la propriété deviendra brute
            r.le<quint8>();
            break;
        }
        v.data = ObjectPtr(std::make_shared<const StreamObject>(std::move(*o)));
        break;
    }
    case 5:
        v.data = core::RectF{r.f64(), r.f64(), r.f64(), r.f64()};
        break;
    case 9: {
        Pointer p;
        p.nodeId = r.le<quint32>();
        p.storageId = r.le<quint32>();
        p.type = r.le<quint16>();
        v.data = p;
        break;
    }
    case 10: {
        std::vector<ObjectPtr> objects;
        const qint32 n = r.count();
        for (qint32 i = 0; i < n && r.ok(); ++i) {
            std::optional<StreamObject> o = readObject(r, depth + 1, error);
            if (!o) {
                r.seek(r.end() + 1);
                r.le<quint8>();
                break;
            }
            objects.push_back(std::make_shared<const StreamObject>(std::move(*o)));
        }
        v.data = std::move(objects);
        break;
    }
    default:
        v.data = RawValue{127, sub, r.take(r.end() - r.pos())};
        break;
    }
    return v;
}

Value readStreamValue(Reader& r, int depth, QString& error)
{
    Value v;
    if (depth > kMaxDepth) {
        r.seek(r.end() + 1);
        r.le<quint8>();
        return v;
    }
    const quint16 t = r.le<quint16>();
    switch (t) {
    case 1:
        v.data = r.le<quint8>() != 0;
        break;
    case 2:
        v.data = r.le<qint32>();
        break;
    case 3:
        v.data = r.le<quint32>();
        break;
    case 6:
        v.data = r.f64();
        break;
    case 9: {
        ValueList list;
        const qint32 n = r.count();
        for (qint32 i = 0; i < n && r.ok(); ++i) {
            list.push_back(readStreamValue(r, depth + 1, error));
        }
        v.data = std::move(list);
        break;
    }
    case 10:
        v.data = r.str16();
        break;
    case 11: {
        r.le<quint16>();
        QStringList list;
        const qint32 n = r.count();
        for (qint32 i = 0; i < n && r.ok(); ++i) {
            list << r.qstring();
        }
        v.data = std::move(list);
        break;
    }
    case 12:
        v.data = r.qbytearray();
        break;
    case 15: {
        const int h = r.le<quint8>();
        const int m = r.le<quint8>();
        const int s = r.le<quint8>();
        v.data = u"%1:%2:%3"_s.arg(h, 2, 10, QLatin1Char('0')).arg(m, 2, 10, QLatin1Char('0')).arg(s, 2, 10, QLatin1Char('0'));
        break;
    }
    case 16: {
        const int year = r.le<quint16>();
        int parts[5] = {};
        for (int& p : parts) {
            p = r.le<quint8>();
        }
        v.data = u"%1-%2-%3T%4:%5:%6"_s.arg(year, 4, 10, QLatin1Char('0'))
                     .arg(parts[0], 2, 10, QLatin1Char('0'))
                     .arg(parts[1], 2, 10, QLatin1Char('0'))
                     .arg(parts[2], 2, 10, QLatin1Char('0'))
                     .arg(parts[3], 2, 10, QLatin1Char('0'))
                     .arg(parts[4], 2, 10, QLatin1Char('0'));
        break;
    }
    case 19: {
        const qint32 x = r.le<qint32>();
        const qint32 y = r.le<qint32>();
        const qint32 w = r.le<qint32>();
        const qint32 h = r.le<qint32>();
        v.data = core::RectF{double(x), double(y), double(w), double(h)};
        break;
    }
    case 20:
        v.data = core::RectF{r.f64(), r.f64(), r.f64(), r.f64()};
        break;
    case 21: {
        const qint32 w = r.le<qint32>();
        const qint32 h = r.le<qint32>();
        v.data = SizeF{double(w), double(h)};
        break;
    }
    case 22:
        v.data = SizeF{r.f64(), r.f64()};
        break;
    case 26:
        v.data = core::PointF{r.f64(), r.f64()};
        break;
    case 67: {
        Color c;
        c.r = r.le<quint8>();
        c.g = r.le<quint8>();
        c.b = r.le<quint8>();
        c.a = r.le<quint8>();
        v.data = c;
        break;
    }
    case 127:
        return readUser(r, r.le<quint16>(), depth, error);
    default:
        v.data = RawValue{t, 0, r.take(r.end() - r.pos())};
        break;
    }
    return v;
}

std::optional<StreamObject> readObject(Reader& r, int depth, QString& error)
{
    if (depth > kMaxDepth) {
        error = u"imbrication trop profonde"_s;
        return std::nullopt;
    }
    const qsizetype at = r.pos();
    if (r.tag() != 'O') {
        error = u"balise 'O' attendue à l'octet %1"_s.arg(at);
        return std::nullopt;
    }
    StreamObject o;
    o.type = r.le<quint16>();
    o.storageId = r.le<quint32>();
    if (r.tag() != 'M') {
        error = u"balise 'M' attendue après l'octet %1"_s.arg(at);
        return std::nullopt;
    }
    const quint32 n = r.le<quint32>();
    if (!r.ok() || n > kMaxProperties) {
        error = u"table des tailles illisible après l'octet %1"_s.arg(at);
        return std::nullopt;
    }
    std::vector<quint32> sizes(n);
    for (quint32& s : sizes) {
        s = r.le<quint32>();
    }
    if (!r.ok()) {
        error = u"table des tailles tronquée après l'octet %1"_s.arg(at);
        return std::nullopt;
    }
    for (const quint32 size : sizes) {
        const qsizetype start = r.pos();
        if (size < 6 || !r.has(static_cast<qsizetype>(size))) {
            error = u"enregistrement de propriété invalide à l'octet %1"_s.arg(start);
            return std::nullopt;
        }
        const qsizetype end = start + static_cast<qsizetype>(size);
        Reader header(r.data(), start, end);
        header.tag();
        if (header.tag() != 'P') {
            error = u"balise 'P' attendue à l'octet %1"_s.arg(start + 1);
            return std::nullopt;
        }
        const quint16 id = header.le<quint16>();
        Reader sub(r.data(), start + 4, end);
        QString ignored;
        Value value = readStreamValue(sub, depth, ignored);
        if (!sub.ok()) {
            Reader rawType(r.data(), start + 4, end);
            const quint16 qtype = rawType.le<quint16>();
            value.data = RawValue{qtype, 0, r.data().mid(start + 6, static_cast<qsizetype>(size) - 6)};
        }
        o.properties[id] = std::move(value);
        r.seek(end);
    }
    // Fin d'objet : 'p' ['R' u32 ['r']] ['o'].
    if (r.peek() == 'p') {
        r.tag();
        if (r.peek() == 'R') {
            r.tag();
            o.nodeId = r.le<quint32>();
            if (r.peek() == 'r') {
                r.tag();
            }
        }
        if (r.peek() == 'o') {
            r.tag();
        }
    }
    if (!r.ok()) {
        error = u"fin d'objet tronquée après l'octet %1"_s.arg(at);
        return std::nullopt;
    }
    return o;
}

} // namespace

const Value* StreamObject::find(quint16 id) const
{
    const auto it = properties.find(id);
    return it == properties.end() ? nullptr : &it->second;
}

std::optional<StreamObject> parseObjectStream(const QByteArray& data, QString& error)
{
    if (!data.startsWith(" Bamboo ") || data.size() < 12) {
        error = u"signature de flux absente"_s;
        return std::nullopt;
    }
    Reader r(data, 12, data.size());
    return readObject(r, 0, error);
}

std::optional<double> asNumber(const Value* v)
{
    if (!v) {
        return std::nullopt;
    }
    if (const auto* b = std::get_if<bool>(&v->data)) {
        return *b ? 1.0 : 0.0;
    }
    if (const auto* i = std::get_if<qint32>(&v->data)) {
        return double(*i);
    }
    if (const auto* u = std::get_if<quint32>(&v->data)) {
        return double(*u);
    }
    if (const auto* d = std::get_if<double>(&v->data)) {
        return *d;
    }
    return std::nullopt;
}

std::optional<bool> asBool(const Value* v)
{
    if (v && std::holds_alternative<bool>(v->data)) {
        return std::get<bool>(v->data);
    }
    if (const std::optional<double> n = asNumber(v)) {
        return *n != 0;
    }
    return std::nullopt;
}

QString asString(const Value* v)
{
    if (const auto* s = v ? std::get_if<QString>(&v->data) : nullptr) {
        return *s;
    }
    return QString();
}

std::optional<core::PointF> asPoint(const Value* v)
{
    if (const auto* p = v ? std::get_if<core::PointF>(&v->data) : nullptr) {
        return *p;
    }
    return std::nullopt;
}

std::optional<SizeF> asSize(const Value* v)
{
    if (const auto* s = v ? std::get_if<SizeF>(&v->data) : nullptr) {
        return *s;
    }
    return std::nullopt;
}

std::optional<core::RectF> asRect(const Value* v)
{
    if (const auto* r = v ? std::get_if<core::RectF>(&v->data) : nullptr) {
        return *r;
    }
    return std::nullopt;
}

std::optional<Color> asColor(const Value* v)
{
    if (const auto* c = v ? std::get_if<Color>(&v->data) : nullptr) {
        return *c;
    }
    return std::nullopt;
}

std::optional<Pointer> asPointer(const Value* v)
{
    if (const auto* p = v ? std::get_if<Pointer>(&v->data) : nullptr) {
        return *p;
    }
    return std::nullopt;
}

const StreamObject* asObject(const Value* v)
{
    if (const auto* o = v ? std::get_if<ObjectPtr>(&v->data) : nullptr) {
        return o->get();
    }
    return nullptr;
}

std::vector<const StreamObject*> asObjects(const Value* v)
{
    std::vector<const StreamObject*> out;
    if (const auto* list = v ? std::get_if<std::vector<ObjectPtr>>(&v->data) : nullptr) {
        for (const ObjectPtr& o : *list) {
            out.push_back(o.get());
        }
    }
    return out;
}

Polyline asPolyline(const Value* v)
{
    if (const auto* p = v ? std::get_if<Polyline>(&v->data) : nullptr) {
        return *p;
    }
    return {};
}

std::vector<Polyline> asPolylines(const Value* v)
{
    if (const auto* p = v ? std::get_if<std::vector<Polyline>>(&v->data) : nullptr) {
        return *p;
    }
    return {};
}

std::vector<ResourceItem> asResourceItems(const Value* v)
{
    if (const auto* p = v ? std::get_if<std::vector<ResourceItem>>(&v->data) : nullptr) {
        return *p;
    }
    return {};
}

} // namespace actcraft::formats::flipchart
```

  Les deux lignes `r.seek(r.end() + 1); r.le<quint8>();` forcent l'état
  « échec » du sous-lecteur (lecture hors borne) : elles transforment une propriété dont l'objet embarqué est
  illisible en `RawValue`, sans faire échouer l'objet parent. `RawValue{qtype, 0, …}` et `RawValue{127, sub, …}`
  nomment les trois champs : pas d'avertissement de Clang. `u"…"_s.arg(h, 2, 10, QLatin1Char('0'))` avec `h` en
  `int` : `QString::arg(int, int, int, QChar)`. Les listes entre accolades (`core::RectF{r.f64(), …}`) sont évaluées
  de gauche à droite (garanti par la norme), contrairement aux arguments d'un appel de fonction.

  Compléter `formats/flipchart.cmake` :

```cmake
set(ACTCRAFT_FORMATS_FLIPCHART_SOURCES
    include/actcraft/formats/bamboo.h
    src/bamboo.cpp
    include/actcraft/formats/objectstream.h
    src/objectstream.cpp)
```

- [ ] **Étape 6 : vert ciblé.** Commande ciblée, motif `tst_formats_objectstream` : `100% tests passed`.

- [ ] **Étape 7 : preuve.** `format.ps1`, preuve de tâche (N ≥ 47 ; `tst_formats_objectstream` Passed).

- [ ] **Étape 8 : commit.**

```powershell
git -C <worktree> add formats/include/actcraft/formats/objectstream.h formats/src/objectstream.cpp formats/flipchart.cmake tests/flipchart.cmake tests/support/flipchartwriter.h tests/tst_formats_objectstream.cpp
git -C <worktree> commit -m "formats : flux objet du .flipchart (variantes, sous-types, propriétés illisibles gardées brutes)" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t9"></a>
### Tâche L2-T9 — `formats` : lecteur `.flipchart` vers le modèle (tables écrites à la main)

**Objectif.** DA4, décision 18 : lire un `.flipchart` en **lecture seule** vers `core::Document`, d'après notre
documentation (`format-flipchart.md` § 5 à § 7) ; comportement de référence : `app/actcraft/io/flipchart_reader.py`
(`_Reader.read`, `read_page`, `_to_page_object`, `_apply_background`, `_grid`, `_action`, `_run`, `Transform.matrix`)
et `app/actcraft/canvas/grid.py` (`GridLevel.from_flipchart`). Correspondances fixées ici :

- couche `0/1/2` → `Bottom/Middle/Top` (absente : 1) ; ordre de dessin = couche, puis profondeur (`z`), puis ordre
  de l'arbre ; les enfants d'un objet suivent leur parent, transformations composées (enfant puis parent) ;
- annotation → `Stroke` (points locaux à pression 0,5, `baseWidth` = épaisseur de plume, couleur de plume, bouts
  plat/carré/rond = 0/16/32, genre 0 stylo, 1 surligneur, 2 encre révélatrice) ; ligne → `Line` ; forme →
  `Shape` `Path` (polygones de la ressource, sinon points locaux ; remplissage « winding » si mode 2) ; image →
  `ImageObject` à ressource ; texte → `Text` (HTML + runs, graisse Qt 4 → Qt 6) ; groupe → `Group` (références
  `S`) ; média → `Media` ; connecteur → `Connector` ; autres types (MathML, texte vertical, inconnus) sautés avec un
  avertissement, et la prochaine action n'a alors pas de déclencheur (comme la référence, qui la rattache à un objet
  non lu) ; étiquettes (type 53) sautées sans avertissement ;
- une action suit le dernier objet non-groupe qui la précède (déclencheur déduit, § 7.6) ; enfant d'un objet :
  rattachée à cet objet ; sans déclencheur : avertissement, action perdue ;
- transformation : `translate(t + o) ; rotate(angle) ; scale(sx, sy) ; translate(-o)`, puis miroir éventuel (§ 7.4) ;
- fond : couleur de la brosse si son style n'est pas 0, dégradé si son type n'est pas 0, image par pointeur pour les
  types 1 et 2 ; grille : niveaux d'après les objets « niveau de grille » enfants ;
- ressources : registre du paperboard, sinon `<id>.ir` ; données : fichier du registre, nom d'origine, puis
  `<id>.png|jpg|ird|gif|bmp|tif` ; clé SHA-256 des octets, extension devinée par la signature ;
- miniatures : `<id de page>.thm` (JPEG d'origine) rendues telles quelles pour S4 (L2-T21).

Patron à **ne pas** reprendre : copier `_schema.py` ou ses noms. Les tables (`flipchartids.h`) ne contiennent que
les numéros des types et propriétés lus, sous nos noms, écrits à la main (relevés dans la documentation et, pour
les numéros qu'elle ne donne pas, par consultation de `_schema.py` : jamais de copie de fichier, jamais de nom
`as…` d'Inspire dans le code).

**Fichiers.**
- Créer : `formats/include/actcraft/formats/flipchart.h`, `formats/src/flipchart.cpp`, `formats/src/flipchartids.h`,
  `tests/tst_formats_flipchart.cpp`
- Modifier : `formats/flipchart.cmake`, `tests/flipchart.cmake`, `tests/support/flipchartwriter.h`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_formats_flipchart` → `error`. Vert : forme bornée,
N ≥ 48, `TESTS OK`, `tst_formats_flipchart .*Passed`.

**Interdits.** QtCore seulement dans `formats/` (le test, lui, lie QtGui pour fabriquer un PNG). Aucun nom de
classe ou de propriété d'Inspire. Ne jamais lire `samples/` ici (c'est L2-T10). Pas d'écriture `.flipchart`.

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : compléter l'écrivain avec le paperboard synthétique.** Dans `tests/support/flipchartwriter.h`,
  ajouter les includes `<QBuffer>`, `<QImage>`, `<QColor>`, puis avant la fermeture de `fctest` :

```cpp
// PNG w × h d'une couleur, produit par Qt (tests et outils de preuve seulement).
inline QByteArray pngBytes(int w, int h, QRgb color)
{
    QImage image(w, h, QImage::Format_ARGB32);
    image.fill(color);
    QByteArray out;
    QBuffer buffer(&out);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return out;
}

inline const QByteArray& sampleMp3()
{
    static const QByteArray bytes("ID3\x04\0\0\0\0\0\0son", 13);
    return bytes;
}

// Paperboard synthétique de deux pages qui passe par chaque branche du lecteur (L2-T9). Numéros de types : 1
// paperboard, 2 page, 3 annotation, 4 ressource d'annotation, 5 ligne, 6 ressource de ligne, 7 forme, 8 ressource
// de forme, 9 image, 10 ressource d'image, 11 texte, 12 ressource de texte, 13 grille, 14 niveau de grille,
// 15 transformation, 16 registre, 18 groupe, 20 encre, 28 action sur objet, 29 fond, 30 ressource de média,
// 31 ligne de texte, 52 connecteur, 53 étiquette, 253 média, 460 formule (non lue). `thumbnails` : octets des
// miniatures « 103.thm » et « 203.thm » (aucune si vide).
inline QByteArray sampleFlipchart(const std::vector<QByteArray>& thumbnails = {})
{
    const QByteArray png = pngBytes(2, 2, qRgb(255, 0, 0));
    const auto translate = [](double x, double y) { return Obj(15, 0).pointF(2, x, y); };

    const QByteArray page1 = container({
        Entry(QStringLiteral("103.in"),
              index(Node(103, 2, 102,
                         {Node(104, 29, 105), Node(106, 13, 107, {Node(108, 14, 109)}), Node(110, 3, 111),
                          Node(112, 7, 113), Node(114, 28, 115), Node(116, 11, 117),
                          Node(118, 18, 119, {}, {110, 112})}))),
        Entry(QStringLiteral("102.io"), Obj(2, 102)
                                             .string(2, QStringLiteral("Page un"))
                                             .string(5, QStringLiteral("Notes 1"))
                                             .sizeF(6, 1024, 768)
                                             .boolean(17, true)
                                             .owner(103)
                                             .stream()),
        Entry(QStringLiteral("105.io"),
              Obj(29, 105)
                  .integer(2, 0)
                  .object(3, 6, Obj(20, 0).integer(8, 1).color(9, 200, 220, 255).integer(14, 1).color(13, 255, 255, 255))
                  .stream()),
        Entry(QStringLiteral("107.io"), Obj(13, 107).boolean(16, true).real(14, 1).stream()),
        Entry(QStringLiteral("109.io"),
              Obj(14, 109).real(7, 50).real(8, 25).color(4, 0, 0, 255).integer(2, 3).integer(3, 1).stream()),
        Entry(QStringLiteral("111.io"),
              Obj(3, 111)
                  .string(4, QStringLiteral("Stylo1"))
                  .integer(8, 2)
                  .real(9, 1)
                  .object(2, 7, Obj(15, 0).pointF(2, 100, 100).real(3, 90).pointF(4, 2, 1))
                  .object(10, 6, Obj(20, 0).integer(2, 1).real(3, 20).color(7, 255, 255, 0, 128).integer(5, 32))
                  .integer(39, 310)
                  .owner(110)
                  .stream()),
        Entry(QStringLiteral("113.io"),
              Obj(7, 113)
                  .string(4, QStringLiteral("Forme1"))
                  .integer(8, 1)
                  .real(9, 2)
                  .object(2, 7, translate(300, 200))
                  .userRect(7, -3, -3, 86, 66)
                  .object(10, 6, Obj(20, 0).integer(4, 2).real(3, 6).color(7, 0, 0, 0).integer(8, 1).color(9, 0, 128, 0))
                  .integer(38, 320)
                  .integer(39, 2)
                  .object(34, 12, Obj(53, 0).string(2, QStringLiteral("Étiquette")))
                  .owner(112)
                  .stream()),
        Entry(QStringLiteral("115.io"), Obj(28, 115).pointer(2, 116, 117, 11).integer(3, 42).integer(5, 184).stream()),
        Entry(QStringLiteral("117.io"), Obj(11, 117)
                                             .integer(8, 1)
                                             .real(9, 3)
                                             .object(2, 7, translate(500, 500))
                                             .userRect(7, 0, 0, 90, 36)
                                             .boolean(18, true)
                                             .integer(38, 330)
                                             .owner(116)
                                             .stream()),
        Entry(QStringLiteral("119.io"), Obj(18, 119).owner(118).stream()),
    });

    const QByteArray page2 = container({
        Entry(QStringLiteral("203.in"),
              index(Node(203, 2, 202,
                         {Node(204, 29, 205), Node(208, 9, 209), Node(210, 5, 211), Node(212, 253, 213),
                          Node(214, 52, 215), Node(216, 460, 217)}))),
        Entry(QStringLiteral("202.io"), Obj(2, 202).sizeF(6, 800, 600).stream()),
        Entry(QStringLiteral("205.io"), Obj(29, 205).integer(2, 1).pointer(4, 0, 207, 9).stream()),
        Entry(QStringLiteral("207.io"), Obj(9, 207).integer(38, 300).stream()),
        Entry(QStringLiteral("209.io"), Obj(9, 209)
                                             .integer(8, 1)
                                             .userRect(7, 0, 0, 2, 2)
                                             .object(2, 7, Obj(15, 0).pointF(2, 10, 10).pointF(4, 50, 50))
                                             .integer(38, 300)
                                             .boolean(39, true)
                                             .color(40, 255, 255, 255)
                                             .stream()),
        Entry(QStringLiteral("211.io"),
              Obj(5, 211)
                  .integer(8, 1)
                  .object(10, 6, Obj(20, 0).real(3, 3).color(7, 0, 0, 255).integer(4, 1))
                  .integer(38, 340)
                  .stream()),
        Entry(QStringLiteral("213.io"), Obj(253, 213)
                                             .integer(8, 1)
                                             .userRect(7, 0, 0, 64, 48)
                                             .integer(38, 300)
                                             .integer(45, 350)
                                             .boolean(41, true)
                                             .stream()),
        Entry(QStringLiteral("215.io"), Obj(52, 215)
                                             .pointF(39, 0, 0)
                                             .pointF(40, 100, 100)
                                             .pointer(34, 208, 209, 9)
                                             .pointer(35, 210, 211, 5)
                                             .stream()),
        Entry(QStringLiteral("217.io"), Obj(460, 217).stream()),
    });

    const Obj registry = Obj(16, 0)
                             .resources(5, {Res(310, QStringLiteral("310.ir"), QString())})
                             .resources(6, {Res(300, QStringLiteral("300.ir"), QStringLiteral("301.png"))})
                             .resources(7, {Res(340, QStringLiteral("340.ir"), QString())})
                             .resources(8, {Res(320, QStringLiteral("320.ir"), QString())})
                             .resources(9, {Res(330, QStringLiteral("330.ir"), QString())})
                             .resources(11, {Res(350, QStringLiteral("350.ir"), QStringLiteral("351.mp3"))});
    const Obj run = Obj(31, 0)
                        .floatRect(2, 0, 0, 90, 36)
                        .pointF(3, 0, 30)
                        .color(5, 255, 0, 0)
                        .string(6, QStringLiteral("Bonjour"))
                        .string(7, QStringLiteral("Arial"))
                        .real(8, 24)
                        .integer(9, 75);
    std::vector<Entry> root{
        Entry(QStringLiteral("101.in"), index(Node(101, 1, 100, {Node(103, 2, 102), Node(203, 2, 202)}))),
        Entry(QStringLiteral("100.io"), Obj(1, 100)
                                             .string(12, QStringLiteral("Essai synthétique"))
                                             .sizeF(18, 1024, 768)
                                             .object(7, 4, registry)
                                             .owner(101)
                                             .stream()),
        Entry(QStringLiteral("103.fpg"), page1),
        Entry(QStringLiteral("203.fpg"), page2),
        Entry(QStringLiteral("300.ir"), Obj(10, 300).size(5, 2, 2).stream()),
        Entry(QStringLiteral("301.png"), png, false),
        Entry(QStringLiteral("310.ir"), Obj(4, 310).points(4, {QPointF(0, 0), QPointF(100, 0), QPointF(100, 50)}).stream()),
        Entry(QStringLiteral("320.ir"),
              Obj(8, 320)
                  .polygons(4, {{QPointF(0, 0), QPointF(80, 0), QPointF(80, 60), QPointF(0, 60)},
                                {QPointF(20, 20), QPointF(60, 20), QPointF(40, 50)}})
                  .stream()),
        Entry(QStringLiteral("330.ir"),
              Obj(12, 330).runs(4, {run}).string(5, QStringLiteral("<p>Bonjour</p>")).integer(6, 2).stream()),
        Entry(QStringLiteral("340.ir"), Obj(6, 340).points(4, {QPointF(0, 0), QPointF(200, 100)}).stream()),
        Entry(QStringLiteral("350.ir"), Obj(30, 350).string(3, QStringLiteral("351.mp3")).stream()),
        Entry(QStringLiteral("351.mp3"), sampleMp3(), false),
    };
    if (thumbnails.size() >= 1) {
        root.emplace_back(QStringLiteral("103.thm"), thumbnails[0], false);
    }
    if (thumbnails.size() >= 2) {
        root.emplace_back(QStringLiteral("203.thm"), thumbnails[1], false);
    }
    return container(root);
}
```

- [ ] **Étape 2 : écrire les tests.** Créer `tests/tst_formats_flipchart.cpp` :

```cpp
#include "support/flipchartwriter.h"

#include <actcraft/core/document.h>
#include <actcraft/formats/flipchart.h>

#include <QTest>

#include <limits>

using namespace actcraft;
using namespace Qt::StringLiterals;
using fctest::Entry;
using fctest::Node;
using fctest::Obj;

namespace {

template <typename T> const T& as(const core::PageObject& o)
{
    const T* p = std::get_if<T>(&o);
    if (!p) {
        qFatal("type d'objet inattendu");
    }
    return *p;
}

} // namespace

// Lecteur .flipchart (DA4) sur le paperboard synthétique de fctest::sampleFlipchart.
class TstFormatsFlipchart : public QObject
{
    Q_OBJECT

private slots:
    void readsTheSyntheticFlipchart()
    {
        const auto r = formats::flipchart::readFlipchart(
            fctest::sampleFlipchart({QByteArray("thm-1"), QByteArray("thm-2")}));
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.warnings.size(), 1);
        QVERIFY2(r.warnings.first().contains(u"460"_s), qPrintable(r.warnings.first()));
        const core::Document& d = r.document;
        QCOMPARE(d.pageCount(), std::size_t{2});
        QCOMPARE(d.title(), std::string("Essai synthétique"));
        QCOMPARE(d.resources().size(), std::size_t{2});
        QVERIFY(core::hasUniqueIds(d));
        QCOMPARE(r.thumbnails.size(), std::size_t{2});
        QCOMPARE(r.thumbnails[1], QByteArray("thm-2"));
    }

    void pageOneOrderLayersAndGroup()
    {
        const auto r = formats::flipchart::readFlipchart(fctest::sampleFlipchart());
        const core::Page& p = r.document.page(0);
        QCOMPARE(p.name, std::string("Page un"));
        QCOMPARE(p.notes, std::string("Notes 1"));
        QCOMPARE(p.objects.size(), std::size_t{4});
        // Couche, puis profondeur, puis arbre : groupe (z 0), forme (z 2), texte (z 3), puis l'annotation (couche 2).
        QCOMPARE(core::kindOf(p.objects[0]), core::ObjectKind::Group);
        QCOMPARE(core::kindOf(p.objects[1]), core::ObjectKind::Shape);
        QCOMPARE(core::kindOf(p.objects[2]), core::ObjectKind::Text);
        QCOMPARE(core::kindOf(p.objects[3]), core::ObjectKind::Stroke);
        QCOMPARE(p.objects[3].common.layer, core::Layer::Top);
        QCOMPARE(p.objects[1].common.layer, core::Layer::Middle);
        const auto& group = as<core::Group>(p.objects[0]);
        QCOMPARE(group.members,
                 (std::vector<core::ObjectId>{p.objects[3].common.id, p.objects[1].common.id}));
    }

    void annotationBecomesAHighlighterStroke()
    {
        const auto r = formats::flipchart::readFlipchart(fctest::sampleFlipchart());
        const core::PageObject& o = r.document.page(0).objects[3];
        const auto& s = as<core::Stroke>(o);
        QCOMPARE(o.common.name, std::string("Stylo1"));
        QCOMPARE(s.kind, core::StrokeKind::Highlighter);
        QCOMPARE(s.cap, core::PenCap::Round);
        QCOMPARE(s.baseWidth, 20.0);
        QCOMPARE(s.color, core::Argb{0x80FFFF00u});
        QCOMPARE(s.points.size(), std::size_t{3});
        QCOMPARE(s.points[2].pressure, 0.5f);
        const core::PointF mapped = o.common.transform.map({100, 0});
        QCOMPARE(mapped.x, 100.0);
        QCOMPARE(mapped.y, 300.0);
    }

    void shapeTextAndAction()
    {
        const auto r = formats::flipchart::readFlipchart(fctest::sampleFlipchart());
        const core::Page& p = r.document.page(0);
        const core::PageObject& shapeObject = p.objects[1];
        const auto& shape = as<core::Shape>(shapeObject);
        QCOMPARE(shape.kind, core::ShapeKind::Path);
        QCOMPARE(shape.paths.size(), std::size_t{2});
        QCOMPARE(shape.bounds.width, 86.0);
        QVERIFY(shape.windingFill);
        QCOMPARE(shape.pen.style, core::PenStyle::Dash);
        QCOMPARE(shape.pen.width, 6.0);
        QCOMPARE(shape.brush.kind, core::FillKind::Solid);
        QCOMPARE(shape.brush.color, core::Argb{0xFF008000u});
        QCOMPARE(shapeObject.common.label, std::string("Étiquette"));
        QCOMPARE(shapeObject.common.transform.dx, 300.0);
        QCOMPARE(shapeObject.common.actions.size(), std::size_t{1});
        const core::Action& a = shapeObject.common.actions[0];
        QCOMPARE(a.kind, core::ActionKind::OnObject);
        QCOMPARE(a.sourceType, std::uint16_t{28});
        QCOMPARE(a.command, std::int32_t{184});
        QCOMPARE(a.target, p.objects[2].common.id);
        const core::PageObject& textObject = p.objects[2];
        const auto& text = as<core::Text>(textObject);
        QVERIFY(textObject.common.locked);
        QCOMPARE(text.html, std::string("<p>Bonjour</p>"));
        QCOMPARE(text.runs.size(), std::size_t{1});
        QCOMPARE(text.runs[0].weight, std::int32_t{700});
        QCOMPARE(text.runs[0].pointSize, 24.0);
        QCOMPARE(text.runs[0].color, core::Argb{0xFFFF0000u});
        QCOMPARE(text.runs[0].baseline.y, 30.0);
    }

    void backgroundAndGrid()
    {
        const auto r = formats::flipchart::readFlipchart(fctest::sampleFlipchart());
        const core::Page& p1 = r.document.page(0);
        QCOMPARE(p1.background.fill.kind, core::FillKind::Gradient);
        QCOMPARE(p1.background.fill.color, core::Argb{0xFFC8DCFFu});
        QCOMPARE(p1.background.fill.color2, core::Argb{0xFFFFFFFFu});
        QCOMPARE(p1.background.fill.gradient, core::GradientKind::Vertical);
        QVERIFY(p1.grid.visible);
        QVERIFY(p1.grid.snapEnabled);
        QCOMPARE(p1.grid.levels.size(), std::size_t{1});
        QCOMPARE(p1.grid.levels[0].spacingX, 50.0);
        QCOMPARE(p1.grid.levels[0].spacingY, 25.0);
        QCOMPARE(p1.grid.levels[0].color, core::Argb{0xFF0000FFu});
        QCOMPARE(p1.grid.levels[0].line, core::GridLine::Solid);
        const core::Page& p2 = r.document.page(1);
        QCOMPARE(p2.width, 800.0);
        QCOMPARE(p2.height, 600.0);
        QCOMPARE(p2.background.fill.kind, core::FillKind::Solid);
        QCOMPARE(p2.background.fill.color, core::Argb{0xFFFFFFFFu});
        const core::Resource* image = r.document.resource(p2.background.image);
        QVERIFY(image);
        QCOMPARE(image->extension, std::string("png"));
    }

    void pageTwoImageMediaLineConnector()
    {
        const auto r = formats::flipchart::readFlipchart(fctest::sampleFlipchart());
        const core::Page& p = r.document.page(1);
        QCOMPARE(p.objects.size(), std::size_t{4}); // la formule (460) est sautée
        const auto& image = as<core::ImageObject>(p.objects[0]);
        QCOMPARE(image.resource, p.background.image);
        QVERIFY(image.hasKeyColor);
        QCOMPARE(image.keyColor, core::Argb{0xFFFFFFFFu});
        QCOMPARE(image.sourceWidth, std::int32_t{2});
        const core::PointF corner = p.objects[0].common.transform.map({1, 1});
        QCOMPARE(corner.x, 60.0);
        QCOMPARE(corner.y, 60.0);
        const auto& line = as<core::Line>(p.objects[1]);
        QCOMPARE(line.points.size(), std::size_t{2});
        QCOMPARE(line.pen.color, core::Argb{0xFF0000FFu});
        QCOMPARE(line.pen.width, 3.0);
        const auto& media = as<core::Media>(p.objects[2]);
        QCOMPARE(media.placeholder, image.resource);
        QCOMPARE(media.fileName, std::string("351.mp3"));
        QVERIFY(media.playOnOpen);
        QCOMPARE(r.document.resource(media.media)->extension, std::string("mp3"));
        const auto& connector = as<core::Connector>(p.objects[3]);
        QCOMPARE(connector.objectA, p.objects[0].common.id);
        QCOMPARE(connector.objectB, p.objects[1].common.id);
        QCOMPARE(connector.secondEnd.x, 100.0);
    }

    void unreadablePagesAreSkippedWithAWarning()
    {
        // Deux pages dans l'index, une seule dans le conteneur.
        const QByteArray page = fctest::container(
            {Entry(u"103.in"_s, fctest::index(Node(103, 2, 102))), Entry(u"102.io"_s, Obj(2, 102).stream())});
        const QByteArray data = fctest::container(
            {Entry(u"101.in"_s, fctest::index(Node(101, 1, 100, {Node(103, 2, 102), Node(203, 2, 202)}))),
             Entry(u"100.io"_s, Obj(1, 100).stream()), Entry(u"103.fpg"_s, page)});
        const auto r = formats::flipchart::readFlipchart(data);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.document.pageCount(), std::size_t{1});
        QVERIFY(r.warnings.join(u' ').contains(u"203"_s));
    }

    void absurdNumbersFallBackToDefaults()
    {
        // Couche NaN, profondeur 1e30, taille d'image hors bornes : défauts, aucune conversion indéfinie.
        const QByteArray page = fctest::container(
            {Entry(u"103.in"_s, fctest::index(Node(103, 2, 102, {Node(110, 7, 111), Node(112, 9, 113)}))),
             Entry(u"102.io"_s, Obj(2, 102).stream()),
             Entry(u"111.io"_s, Obj(7, 111).real(8, std::numeric_limits<double>::quiet_NaN()).real(9, 1e30).stream()),
             Entry(u"113.io"_s, Obj(9, 113).integer(38, 300).stream())});
        const QByteArray page2 = fctest::container(
            {Entry(u"203.in"_s, fctest::index(Node(203, 2, 202, {Node(210, 9, 211), Node(212, 9, 213)}))),
             Entry(u"202.io"_s, Obj(2, 202).stream()), Entry(u"211.io"_s, Obj(9, 211).integer(38, 999).stream()),
             Entry(u"213.io"_s, Obj(9, 213).integer(38, 999).stream())});
        const QByteArray data = fctest::container(
            {Entry(u"101.in"_s, fctest::index(Node(101, 1, 100, {Node(103, 2, 102), Node(203, 2, 202)}))),
             Entry(u"100.io"_s, Obj(1, 100).stream()), Entry(u"103.fpg"_s, page), Entry(u"203.fpg"_s, page2),
             Entry(u"300.ir"_s, Obj(10, 300).sizeF(5, 1e30, 1e30).stream()),
             Entry(u"300.png"_s, fctest::pngBytes(2, 2, qRgb(0, 0, 255)), false)});
        const auto r = formats::flipchart::readFlipchart(data);
        QVERIFY2(r.ok, qPrintable(r.error));
        const auto& objects = r.document.page(0).objects;
        QCOMPARE(objects.size(), std::size_t{2});
        // Avec un cast direct, NaN donne INT_MIN sous MSVC : « couche -2147483648 inconnue ». Ici, défaut sans bruit.
        QVERIFY2(!r.warnings.join(u' ').contains(u"couche"_s), qPrintable(r.warnings.join(u'\n')));
        for (const core::PageObject& o : objects) {
            if (const auto* im = std::get_if<core::ImageObject>(&o)) {
                QCOMPARE(im->sourceWidth, std::int32_t{256}); // taille 1e30 × 1e30 : défaut du modèle
            } else {
                QCOMPARE(o.common.layer, core::Layer::Middle);
            }
        }
        // Mémorisation par ressource : deux images sur la même ressource absente → un seul avertissement.
        QCOMPARE(r.warnings.filter(u"999"_s).size(), 1);
    }

    void refusesWhatIsNotAFlipchart()
    {
        QVERIFY(!formats::flipchart::looksLikeFlipchart("PK\x03\x04"));
        QVERIFY(formats::flipchart::looksLikeFlipchart(fctest::sampleFlipchart().left(8)));
        auto r = formats::flipchart::readFlipchart("pas un paperboard");
        QVERIFY(!r.ok);
        QVERIFY(!r.error.isEmpty());
        r = formats::flipchart::readFlipchart(fctest::container({Entry(u"100.io"_s, Obj(1, 100).stream())}));
        QVERIFY(!r.ok);
        QVERIFY2(r.error.contains(u"index"_s), qPrintable(r.error));
        r = formats::flipchart::readFlipchart(fctest::container(
            {Entry(u"101.in"_s, fctest::index(Node(101, 1, 100, {Node(103, 2, 102)}))),
             Entry(u"100.io"_s, Obj(1, 100).stream())}));
        QVERIFY(!r.ok);
        QVERIFY2(r.error.contains(u"aucune page"_s), qPrintable(r.error));
    }
};

QTEST_GUILESS_MAIN(TstFormatsFlipchart)
#include "tst_formats_flipchart.moc"
```

  `QTEST_GUILESS_MAIN` suffit à `QImage` (pas d'écran : `QImage::save` en PNG n'a pas besoin de `QGuiApplication`).
  Ajouter à `tests/flipchart.cmake` :

```cmake
actcraft_add_test(tst_formats_flipchart
    SOURCES tst_formats_flipchart.cpp
    LIBS ActCraft::formats Qt6::Gui)
```

  et ajouter `Qt6::Gui` aux `LIBS` de `tst_formats_bamboo` et `tst_formats_objectstream` (l'écrivain inclut
  désormais `<QImage>`).

- [ ] **Étape 3 : constater le rouge.** Commande ciblée, motif `tst_formats_flipchart` → `error`.

- [ ] **Étape 4 : écrire les tables.** Créer `formats/src/flipchartids.h` :

> Bloc réécrit (L2-C2) : c'est le fichier livré par L2-T9, à nos noms. Les extraits de code de cette tâche
> citent les clés du format par leur numéro seul.

```cpp
#pragma once

#include <QtGlobal>

// Numéros du format .flipchart lus par ActCraft, écrits à la main d'après notre documentation
// (REA - Activinspire/docs/re/format-flipchart.md § 4 à § 7). Seuls les numéros des genres d'objets et des
// propriétés lus y sont, sous des noms à nous : aucun nom de classe ni de propriété d'Inspire. Les numéros de
// propriétés sont stables entre versions du format (§ 4.2), mais propres à chaque genre au-delà de 36 : 38
// est la ressource d'une forme et la translucidité d'une annotation.

namespace actcraft::formats::flipchart::ids {

// Genres d'objets : numéro u16 après la balise 'O' d'un flux et dans chaque nœud de l'index.
namespace kind {
inline constexpr quint16 kDocumentObject = 1;    // le paperboard lui-même
inline constexpr quint16 kPageObject = 2;        // une page
inline constexpr quint16 kStrokeObject = 3;      // trait d'encre (annotation)
inline constexpr quint16 kLineObject = 5;        // ligne brisée
inline constexpr quint16 kShapeObject = 7;       // forme à polygones
inline constexpr quint16 kImageObject = 9;       // image
inline constexpr quint16 kTextObject = 11;       // texte riche
inline constexpr quint16 kGridObject = 13;       // grille de la page
inline constexpr quint16 kGridLevelObject = 14;  // un niveau de la grille
inline constexpr quint16 kGroupObject = 18;      // groupe par références
inline constexpr quint16 kToolAction = 22;       // action : choisir un outil
inline constexpr quint16 kGoToPageAction = 23;   // action : aller à une page
inline constexpr quint16 kShiftAction = 24;      // action : déplacer
inline constexpr quint16 kProgramAction = 25;    // action : commande de l'application
inline constexpr quint16 kOpenFileAction = 26;   // action : ouvrir un fichier
inline constexpr quint16 kObjectAction = 28;     // action sur un objet de la page
inline constexpr quint16 kBackgroundObject = 29; // fond de la page
inline constexpr quint16 kTextAction = 50;       // action sur un texte
inline constexpr quint16 kConnectorObject = 52;  // connecteur entre deux objets
inline constexpr quint16 kTagObject = 53;        // étiquette (légende) d'un objet
inline constexpr quint16 kMediaObject = 253;     // son ou vidéo
inline constexpr quint16 kLaunchAction = 463;    // action : lancer une mini-application
} // namespace kind

inline constexpr bool isActionKind(quint16 k) noexcept
{
    return k == kind::kToolAction || k == kind::kGoToPageAction || k == kind::kShiftAction ||
           k == kind::kProgramAction || k == kind::kOpenFileAction || k == kind::kObjectAction ||
           k == kind::kTextAction || k == kind::kLaunchAction;
}

// Paperboard (§ 5).
namespace docKeys {
inline constexpr quint16 kFallbackTitle = 2;  // titre de secours, si le titre est vide
inline constexpr quint16 kResourceTable = 7;  // table des ressources (objet embarqué)
inline constexpr quint16 kTitleText = 12;     // titre
inline constexpr quint16 kStartPageSize = 18; // taille des pages qui n'en donnent pas
} // namespace docKeys

// Page (§ 6).
namespace pageKeys {
inline constexpr quint16 kPageTitle = 2;
inline constexpr quint16 kPresenterNotes = 5;
inline constexpr quint16 kSheetSize = 6;
inline constexpr quint16 kShowGuides = 17; // grille affichée
} // namespace pageKeys

// Fond de page (§ 6).
namespace fillKeys {
inline constexpr quint16 kFillSource = 2; // 0 encre, 1 image, 2 ressource
inline constexpr quint16 kFillStyle = 3;  // encre embarquée : couleur ou dégradé
inline constexpr quint16 kPictureRef = 4; // pointeur vers l'objet image
} // namespace fillKeys

// Grille de la page.
namespace guideKeys {
inline constexpr quint16 kPreviewZoom = 13;   // échelle dans les miniatures
inline constexpr quint16 kBaseZoom = 14;      // échelle de départ
inline constexpr quint16 kZoomIncrement = 15; // pas d'échelle
inline constexpr quint16 kSnapEnabled = 16;   // aimantation permise
inline constexpr quint16 kDrawnAbove = 17;    // dessinée par-dessus les objets
} // namespace guideKeys

// Niveau de grille.
namespace guideLevelKeys {
inline constexpr quint16 kMarkStyle = 2;   // tracé : indice + 1 dans core::GridLine
inline constexpr quint16 kLayoutStyle = 3; // disposition : indice + 1 dans core::GridPattern
inline constexpr quint16 kTint = 4;        // couleur
inline constexpr quint16 kShown = 5;
inline constexpr quint16 kAnchor = 6; // point de départ
inline constexpr quint16 kSpacingX = 7;
inline constexpr quint16 kSpacingY = 8;
inline constexpr quint16 kSlant = 9; // angle de la disposition oblique
inline constexpr quint16 kLinesAlongX = 11;
inline constexpr quint16 kLinesAlongY = 12;
inline constexpr quint16 kMagnetic = 13; // aimante les objets
} // namespace guideLevelKeys

// Propriétés communes aux objets affichables (§ 7.1).
namespace itemKeys {
inline constexpr quint16 kPlacement = 2;   // transformation (objet embarqué)
inline constexpr quint16 kItemTitle = 4;   // nom de l'objet
inline constexpr quint16 kShown = 6;       // visible
inline constexpr quint16 kLocalBox = 7;    // rectangle englobant local
inline constexpr quint16 kPlane = 8;       // couche : 0 inférieure, 1 intermédiaire, 2 supérieure
inline constexpr quint16 kStackOrder = 9;  // profondeur dans la couche
inline constexpr quint16 kPenAndFill = 10; // encre (objet embarqué)
inline constexpr quint16 kPivot = 12;      // origine de rotation (objet embarqué)
inline constexpr quint16 kMovable = 13;
inline constexpr quint16 kResizable = 14;
inline constexpr quint16 kRotatable = 15;
inline constexpr quint16 kFrozen = 18;       // verrouillé
inline constexpr quint16 kConnectorTag = 33; // étiquette d'un connecteur (sa 34 est une extrémité)
inline constexpr quint16 kTag = 34;          // étiquette des autres objets
} // namespace itemKeys

namespace strokeKeys {
inline constexpr quint16 kSeeThrough = 38; // translucidité, 0 opaque à 255 invisible
inline constexpr quint16 kPointsRef = 39;  // ressource des points
} // namespace strokeKeys

namespace shapeKeys {
inline constexpr quint16 kOutlineRef = 38;   // ressource des polygones
inline constexpr quint16 kWindingRule = 39;  // 2 : remplissage « winding »
inline constexpr quint16 kUnclosed = 42;     // contour ouvert
inline constexpr quint16 kInlinePoints = 49; // points portés par l'objet, sans ressource
} // namespace shapeKeys

namespace lineKeys {
inline constexpr quint16 kPointsRef = 38;
}

namespace imageKeys {
inline constexpr quint16 kPixelsRef = 38; // ressource de l'image
inline constexpr quint16 kKeyed = 39;     // une couleur est rendue transparente
inline constexpr quint16 kKeyColor = 40;  // cette couleur
} // namespace imageKeys

namespace textKeys {
inline constexpr quint16 kContentRef = 38;
}

namespace mediaKeys {
inline constexpr quint16 kPosterRef = 38; // image affichée à la place du média
inline constexpr quint16 kPlayOnOpen = 41;
inline constexpr quint16 kRepeat = 42;
inline constexpr quint16 kClipRef = 45; // ressource du son ou de la vidéo
} // namespace mediaKeys

namespace connectorKeys {
inline constexpr quint16 kFromItem = 34; // pointeur vers l'objet de départ
inline constexpr quint16 kToItem = 35;   // pointeur vers l'objet d'arrivée
inline constexpr quint16 kFromPoint = 39;
inline constexpr quint16 kToPoint = 40;
} // namespace connectorKeys

// Transformation embarquée (§ 7.4).
namespace placementKeys {
inline constexpr quint16 kOffset = 2;      // translation
inline constexpr quint16 kTurnDegrees = 3; // rotation, degrés, sens horaire
inline constexpr quint16 kAxisFactors = 4; // facteurs d'échelle
inline constexpr quint16 kMirrorX = 5;     // miroir horizontal
inline constexpr quint16 kMirrorY = 6;     // miroir vertical
} // namespace placementKeys

namespace pivotKeys {
inline constexpr quint16 kPivotPoint = 2;
}

// Encre embarquée (§ 7.2) : plume et remplissage.
namespace penKeys {
inline constexpr quint16 kStrokeMode = 2; // 0 stylo, 1 surligneur, 2 encre révélatrice
inline constexpr quint16 kOutlineWidth = 3;
inline constexpr quint16 kDashPattern = 4; // valeurs de Qt::PenStyle
inline constexpr quint16 kEndShape = 5;    // bouts : 0 plat, 16 carré, 32 rond
inline constexpr quint16 kCornerShape = 6; // jointures : 0 angle, 64 biseau, 128 rond
inline constexpr quint16 kOutlineColor = 7;
inline constexpr quint16 kFillPattern = 8;     // 0 aucun, 1 plein, 2 à 14 motifs, 25 dégradé
inline constexpr quint16 kFillTint = 9;        // couleur du remplissage
inline constexpr quint16 kOpaqueHatch = 10;    // fond des motifs opaque
inline constexpr quint16 kHatchBaseColor = 11; // couleur de ce fond
inline constexpr quint16 kSecondaryColor = 13; // seconde couleur du dégradé
inline constexpr quint16 kBlendDirection = 14; // 0 aucun dégradé, 1 à 5 : core::GradientKind
} // namespace penKeys

namespace tagKeys {
inline constexpr quint16 kTagText = 2;
}

// Objets ressources (fichiers .ir, § 5).
namespace resourceKeys {
inline constexpr quint16 kPointList = 4;      // trait d'encre, ligne
inline constexpr quint16 kPathOutlines = 4;   // forme
inline constexpr quint16 kSourceFileName = 3; // image, média : nom du fichier d'origine
inline constexpr quint16 kPixelSize = 5;      // image : taille en pixels
inline constexpr quint16 kLayoutRuns = 4;     // texte : lignes mises en page
inline constexpr quint16 kMarkup = 5;         // texte : HTML
} // namespace resourceKeys

// Ligne de texte mise en page (§ 7.5).
namespace runKeys {
inline constexpr quint16 kBox = 2;
inline constexpr quint16 kBaselinePoint = 3;
inline constexpr quint16 kGlyphColor = 5;
inline constexpr quint16 kCharacters = 6;
inline constexpr quint16 kTypeface = 7;
inline constexpr quint16 kPointHeight = 8;
inline constexpr quint16 kBoldness = 9; // échelle Qt 4 (50 normal, 75 gras)
inline constexpr quint16 kSlanted = 10;
inline constexpr quint16 kUnderlined = 11;
inline constexpr quint16 kStruckThrough = 12;
} // namespace runKeys

// Actions (§ 7.6) : commande et cible selon le genre.
namespace actionKeys {
inline constexpr quint16 kObjectActionTarget = 2;
inline constexpr quint16 kObjectActionCommand = 5;
inline constexpr quint16 kOpenFileResource = 2;
inline constexpr quint16 kOpenFileTarget = 4;
inline constexpr quint16 kOpenFileCommand = 10;
inline constexpr quint16 kGoToPageCommand = 4;
inline constexpr quint16 kToolCommand = 3;
inline constexpr quint16 kShiftCommand = 3;
inline constexpr quint16 kProgramCommand = 4;
inline constexpr quint16 kTextActionTarget = 3;
inline constexpr quint16 kTextActionCommand = 6;
inline constexpr quint16 kLaunchCommand = 4;
} // namespace actionKeys

} // namespace actcraft::formats::flipchart::ids
```

- [ ] **Étape 5 : écrire l'en-tête du lecteur.** Créer `formats/include/actcraft/formats/flipchart.h` :

```cpp
#pragma once

#include <actcraft/core/document.h>

#include <QByteArray>
#include <QString>
#include <QStringList>

#include <vector>

namespace actcraft::formats::flipchart {

struct FlipchartReadResult
{
    bool ok = false;
    QString error;        // si !ok
    QStringList warnings; // pages, objets ou ressources illisibles, types non lus ; à afficher sans bloquer
    core::Document document;
    std::vector<QByteArray> thumbnails; // miniature JPEG d'origine de chaque page lue, vide si absente
};

// Lit un paperboard .flipchart en lecture seule (DA4, décision 18). Ordre de dessin : couche, puis profondeur,
// puis ordre de l'arbre (format-flipchart.md § 7.3). Échoue seulement si le conteneur, l'index racine ou toutes
// les pages sont illisibles.
FlipchartReadResult readFlipchart(const QByteArray& data);
// Signature du conteneur (« Bamboo ») : de quoi choisir le lecteur sans lire tout le fichier.
bool looksLikeFlipchart(const QByteArray& head) noexcept;

} // namespace actcraft::formats::flipchart
```

- [ ] **Étape 6 : écrire le lecteur.** Créer `formats/src/flipchart.cpp` :

```cpp
#include "flipchartids.h"

#include <actcraft/formats/bamboo.h>
#include <actcraft/formats/flipchart.h>
#include <actcraft/formats/objectstream.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <utility>

namespace actcraft::formats::flipchart {

using namespace Qt::StringLiterals;

namespace {

core::Argb argb(const Color& c) noexcept
{
    return (core::Argb{c.a} << 24) | (core::Argb{c.r} << 16) | (core::Argb{c.g} << 8) | core::Argb{c.b};
}

double numberOr(const StreamObject* o, quint16 id, double fallback)
{
    return o ? asNumber(o->find(id)).value_or(fallback) : fallback;
}

// Entier lu du fichier (non fiable) : NaN, infini ou hors des bornes d'un int → valeur par défaut.
int intOr(const StreamObject* o, quint16 id, int fallback)
{
    const double v = numberOr(o, id, fallback);
    if (!std::isfinite(v) || v < static_cast<double>(std::numeric_limits<int>::min()) ||
        v > static_cast<double>(std::numeric_limits<int>::max())) {
        return fallback;
    }
    return static_cast<int>(v);
}

bool flagOr(const StreamObject* o, quint16 id, bool fallback)
{
    return o ? asBool(o->find(id)).value_or(fallback) : fallback;
}

core::Argb colorOr(const StreamObject* o, quint16 id, core::Argb fallback)
{
    const std::optional<Color> c = o ? asColor(o->find(id)) : std::nullopt;
    return c ? argb(*c) : fallback;
}

std::string text(const StreamObject* o, quint16 id)
{
    return o ? asString(o->find(id)).toStdString() : std::string();
}

// Extension devinée par la signature, sinon par le nom (comme _sniff de la référence).
std::string extensionOf(const QByteArray& data, const QString& name)
{
    if (data.startsWith("\x89PNG\r\n\x1a\n")) {
        return "png";
    }
    if (data.startsWith("\xff\xd8\xff")) {
        return "jpg";
    }
    if (data.startsWith("GIF87a") || data.startsWith("GIF89a")) {
        return "gif";
    }
    if (data.startsWith("BM")) {
        return "bmp";
    }
    if (data.startsWith(QByteArray("II*\0", 4)) || data.startsWith(QByteArray("MM\0*", 4))) {
        return "tif";
    }
    if (data.startsWith("RIFF") && data.mid(8, 4) == "WAVE") {
        return "wav";
    }
    if (data.startsWith("ID3") || data.startsWith("\xff\xfb") || data.startsWith("\xff\xf3")) {
        return "mp3";
    }
    if (data.mid(4, 4) == "ftyp") {
        return "mp4";
    }
    const qsizetype dot = name.lastIndexOf(u'.');
    return dot >= 0 ? name.mid(dot + 1).toLower().toStdString() : std::string("bin");
}

// Graisse Qt 4 (0 à 99, 50 normal, 75 gras) → Qt 6 (100 à 900) : la valeur de la table la plus proche.
std::int32_t qt6Weight(int w)
{
    if (w >= 100) {
        return std::min(900, w);
    }
    static constexpr int table[][2] = {{0, 100}, {12, 200}, {25, 300}, {50, 400}, {57, 500},
                                       {63, 600}, {75, 700}, {81, 800}, {87, 900}};
    int best = 400;
    int distance = 1000;
    for (const auto& [qt4, qt6] : table) {
        if (std::abs(qt4 - w) < distance) {
            distance = std::abs(qt4 - w);
            best = qt6;
        }
    }
    return best;
}

core::Outline outlineFrom(const StreamObject* ink)
{
    core::Outline o;
    if (!ink) {
        return o;
    }
    const int style = intOr(ink, 4, 1);
    o.style = style >= 0 && style <= 5 ? static_cast<core::PenStyle>(style) : core::PenStyle::Solid;
    o.color = colorOr(ink, 7, 0xFF000000u);
    o.width = numberOr(ink, 3, 1);
    if (o.width <= 0) {
        o.style = core::PenStyle::None;
    }
    const int cap = intOr(ink, 5, 32);
    o.cap = cap == 0 ? core::PenCap::Flat : cap == 16 ? core::PenCap::Square : core::PenCap::Round;
    const int join = intOr(ink, 6, 128);
    o.join = join == 0 ? core::PenJoin::Miter : join == 64 ? core::PenJoin::Bevel : core::PenJoin::Round;
    return o;
}

core::GradientKind gradientOf(int value)
{
    return value >= 2 && value <= 5 ? static_cast<core::GradientKind>(value) : core::GradientKind::Vertical;
}

core::Fill fillFrom(const StreamObject* ink)
{
    core::Fill f;
    f.color = colorOr(ink, 9, 0xFFFFFFFFu);
    f.color2 = colorOr(ink, 13, 0xFF000000u);
    f.opaqueBackground = intOr(ink, 10, 0) != 0;
    f.backgroundColor = colorOr(ink, 11, 0xFF000000u);
    const int style = intOr(ink, 8, 0);
    const int gradient = intOr(ink, 14, 0);
    if (style == 0) {
        f.kind = core::FillKind::None;
    } else if (gradient != 0 || style == 25) {
        f.kind = core::FillKind::Gradient;
        f.gradient = gradientOf(gradient);
    } else if (style >= 2 && style <= 14) {
        f.kind = core::FillKind::Pattern;
        f.pattern = static_cast<std::uint8_t>(style);
    } else {
        f.kind = core::FillKind::Solid;
    }
    return f;
}

// § 7.4 : translate(t + o) ; rotate(angle) ; scale(sx, sy) ; translate(-o) ; puis miroir éventuel.
core::Transform localTransform(const StreamObject& o, const core::RectF& bounds)
{
    const StreamObject* t = asObject(o.find(2));
    if (!t) {
        return {};
    }
    const core::PointF translate = asPoint(t->find(2)).value_or(core::PointF{});
    const core::PointF scale = asPoint(t->find(4)).value_or(core::PointF{1, 1});
    const double angle = numberOr(t, 3, 0);
    const bool mirrored = flagOr(t, 5, false);
    const bool flipped = flagOr(t, 6, false);
    core::PointF origin;
    if (const StreamObject* rot = asObject(o.find(12))) {
        origin = asPoint(rot->find(2)).value_or(core::PointF{});
    }
    core::Transform m = core::compose(
        core::compose(core::compose(core::translation(-origin.x, -origin.y), core::scaling(scale.x, scale.y)),
                      core::rotation(angle)),
        core::translation(translate.x + origin.x, translate.y + origin.y));
    if (mirrored || flipped) {
        const core::Transform flip = core::compose(core::scaling(mirrored ? -1 : 1, flipped ? -1 : 1),
                                                   core::translation(mirrored ? bounds.width : 0,
                                                                     flipped ? bounds.height : 0));
        m = core::compose(flip, m);
    }
    return m;
}

struct PendingAction
{
    core::Action action;
    quint32 targetNode = 0;
};

// Objet lu, avant le tri et la résolution des références.
struct Pending
{
    int layer = 1;
    double depth = 0;
    int order = 0;
    quint32 nodeId = 0;
    core::PageObject object;
    std::vector<quint32> members; // groupe
    quint32 connectorA = 0;
    quint32 connectorB = 0;
    std::vector<PendingAction> actions;
    std::vector<Pending> children;
};

bool drawsBefore(const Pending& a, const Pending& b)
{
    if (a.layer != b.layer) {
        return a.layer < b.layer;
    }
    if (a.depth != b.depth) {
        return a.depth < b.depth;
    }
    return a.order < b.order;
}

class Reader
{
public:
    explicit Reader(const QByteArray& data) : m_data(data) {}

    FlipchartReadResult read()
    {
        FlipchartReadResult result;
        QString error;
        m_root = BambooContainer::parse(m_data, error);
        if (!m_root) {
            result.error = u"paperboard illisible : %1"_s.arg(error);
            return result;
        }
        QString indexName;
        for (const QString& name : m_root->names()) {
            if (name.endsWith(u".in"_s)) {
                indexName = name;
                break;
            }
        }
        std::optional<IndexNode> tree;
        if (!indexName.isEmpty()) {
            if (const std::optional<QByteArray> bytes = m_root->read(indexName, error)) {
                tree = parseIndex(*bytes, error);
            }
        }
        if (!tree) {
            result.error = u"index racine absent ou illisible %1"_s.arg(error);
            return result;
        }
        const std::optional<StreamObject> fc = load(*m_root, tree->storageName(), error);
        if (!fc) {
            result.error = u"objet paperboard illisible : %1"_s.arg(error);
            return result;
        }
        if (const StreamObject* registry = asObject(fc->find(7))) {
            for (const auto& [id, value] : registry->properties) {
                for (const ResourceItem& item : asResourceItems(&value)) {
                    m_register[item.id] = item;
                }
            }
        }
        std::string title = text(&*fc, 12);
        if (title.empty()) {
            title = text(&*fc, 2);
        }
        const SizeF defaultSize =
            asSize(fc->find(18)).value_or(SizeF{1024, 768});
        std::vector<core::Page> pages;
        for (const IndexNode& child : tree->children) {
            if (child.type != 2) {
                continue;
            }
            QByteArray thumbnail;
            if (std::optional<core::Page> page = readPage(child, defaultSize, thumbnail)) {
                pages.push_back(std::move(*page));
                result.thumbnails.push_back(std::move(thumbnail));
            }
        }
        if (pages.empty()) {
            result.error = u"aucune page lisible"_s;
            result.warnings = m_warnings;
            return result;
        }
        core::Document document(std::move(pages));
        document.setTitle(std::move(title));
        for (const auto& [key, resource] : m_resources.resources()) {
            document.addResource(resource.data, resource.extension);
        }
        result.document = std::move(document);
        result.warnings = m_warnings;
        result.ok = true;
        return result;
    }

private:
    std::optional<StreamObject> load(const BambooContainer& c, const QString& name, QString& error)
    {
        const std::optional<QByteArray> bytes = c.read(name, error);
        return bytes ? parseObjectStream(*bytes, error) : std::nullopt;
    }

    void warn(const QString& message) { m_warnings << message; }

    const StreamObject* resourceObject(quint32 rid)
    {
        if (const auto it = m_resourceObjects.find(rid); it != m_resourceObjects.end()) {
            return it->second.get();
        }
        const auto item = m_register.find(rid);
        const QString entry = item != m_register.end() && !item->second.entry.isEmpty() ? item->second.entry
                                                                                       : u"%1.ir"_s.arg(rid);
        std::shared_ptr<const StreamObject> object;
        if (m_root->contains(entry)) {
            QString error;
            if (std::optional<StreamObject> o = load(*m_root, entry, error)) {
                object = std::make_shared<const StreamObject>(std::move(*o));
            } else {
                warn(u"ressource %1 illisible : %2"_s.arg(entry, error));
            }
        }
        m_resourceObjects[rid] = object;
        return object.get();
    }

    std::optional<std::pair<QString, QByteArray>> resourceFile(quint32 rid, const StreamObject* res)
    {
        QStringList names;
        if (const auto item = m_register.find(rid); item != m_register.end() && !item->second.dataFile.isEmpty()) {
            names << item->second.dataFile;
        }
        if (res) {
            if (const QString original = asString(res->find(3)); !original.isEmpty()) {
                names << original;
            }
        }
        for (const char* ext : {"png", "jpg", "ird", "gif", "bmp", "tif"}) {
            names << u"%1.%2"_s.arg(rid).arg(QLatin1String(ext));
        }
        for (const QString& name : names) {
            if (!m_root->contains(name)) {
                continue;
            }
            QString error;
            if (std::optional<QByteArray> data = m_root->read(name, error)) {
                return std::make_pair(name, std::move(*data));
            }
            warn(u"données %1 illisibles : %2"_s.arg(name, error));
        }
        return std::nullopt;
    }

    // Clé de la ressource image `rid` (vide, avec un avertissement, si ses données manquent). Mémorisée par
    // identifiant : une image partagée par cent objets n'est lue, décompressée et hachée qu'une fois.
    std::string imageKey(quint32 rid)
    {
        if (const auto it = m_fileKeys.find(rid); it != m_fileKeys.end()) {
            return it->second.first;
        }
        const auto file = resourceFile(rid, resourceObject(rid));
        if (!file) {
            warn(u"données image introuvables pour la ressource %1"_s.arg(rid));
            m_fileKeys[rid] = {};
            return {};
        }
        const std::string key = m_resources.addResource(file->second, extensionOf(file->second, file->first));
        m_fileKeys[rid] = {key, file->first.toStdString()};
        return key;
    }

    // Clé d'un fichier quelconque (son, vidéo, document ouvert par une action) ; `fileName` reçoit son nom.
    std::string fileKey(quint32 rid, std::string* fileName)
    {
        if (const auto it = m_fileKeys.find(rid); it != m_fileKeys.end()) {
            if (fileName) {
                *fileName = it->second.second;
            }
            return it->second.first;
        }
        const auto file = resourceFile(rid, resourceObject(rid));
        if (!file) {
            warn(u"données introuvables pour la ressource %1"_s.arg(rid));
            m_fileKeys[rid] = {};
            return {};
        }
        const std::string key = m_resources.addResource(file->second, extensionOf(file->second, file->first));
        m_fileKeys[rid] = {key, file->first.toStdString()};
        if (fileName) {
            *fileName = file->first.toStdString();
        }
        return key;
    }

    std::optional<core::Page> readPage(const IndexNode& pageNode, const SizeF& defaultSize, QByteArray& thumbnail)
    {
        const QString where = u"page %1"_s.arg(pageNode.nodeId);
        QString error;
        const std::optional<QByteArray> bytes = m_root->read(u"%1.fpg"_s.arg(pageNode.nodeId), error);
        std::optional<BambooContainer> container = bytes ? BambooContainer::parse(*bytes, error) : std::nullopt;
        if (!container) {
            warn(u"%1 illisible : %2"_s.arg(where, error));
            return std::nullopt;
        }
        IndexNode tree = pageNode;
        const QString indexName = u"%1.in"_s.arg(pageNode.nodeId);
        if (container->contains(indexName)) {
            const std::optional<QByteArray> indexBytes = container->read(indexName, error);
            std::optional<IndexNode> parsed = indexBytes ? parseIndex(*indexBytes, error) : std::nullopt;
            if (!parsed) {
                warn(u"%1 : index illisible : %2"_s.arg(where, error));
                return std::nullopt;
            }
            tree = std::move(*parsed);
        }
        const std::optional<StreamObject> pageObject = load(*container, tree.storageName(), error);
        if (!pageObject) {
            warn(u"%1 illisible : %2"_s.arg(where, error));
            return std::nullopt;
        }
        core::Page page;
        const SizeF size = asSize(pageObject->find(6)).value_or(defaultSize);
        page.width = size.width;
        page.height = size.height;
        page.name = text(&*pageObject, 2);
        page.notes = text(&*pageObject, 5);
        page.grid.visible = flagOr(&*pageObject, 17, false);
        const QString thumbName = u"%1.thm"_s.arg(pageNode.nodeId);
        if (m_root->contains(thumbName)) {
            thumbnail = m_root->read(thumbName, error).value_or(QByteArray());
        }

        std::vector<Pending> pending;
        int order = 0;
        std::optional<std::size_t> last; // dernier objet non-groupe : déclencheur des actions qui suivent
        for (const IndexNode& child : tree.children) {
            const QString name = child.storageName();
            if (!container->contains(name)) {
                warn(u"%1 : %2 absent (nœud %3)"_s.arg(where, name).arg(child.nodeId));
                continue;
            }
            const std::optional<StreamObject> o = load(*container, name, error);
            if (!o) {
                warn(u"%1 : %2 illisible : %3"_s.arg(where, name, error));
                continue;
            }
            if (ids::isAction(child.type)) {
                PendingAction action = actionFrom(child, *o);
                if (last) {
                    pending[*last].actions.push_back(std::move(action));
                } else {
                    warn(u"%1 : action sans objet déclencheur (nœud %2)"_s.arg(where).arg(child.nodeId));
                }
                continue;
            }
            if (child.type == 29) {
                background(*container, *o, page);
                continue;
            }
            if (child.type == 13) {
                grid(*container, child, *o, page);
                continue;
            }
            if (child.type == 53) {
                continue;
            }
            std::optional<Pending> p = toPending(*container, child, *o, where);
            if (!p) {
                last.reset(); // comme la référence : l'action suivante viserait cet objet non lu
                continue;
            }
            p->order = order++;
            const bool isGroup = std::get_if<core::Group>(&p->object) != nullptr;
            pending.push_back(std::move(*p));
            if (!isGroup) {
                last = pending.size() - 1;
            }
        }

        std::stable_sort(pending.begin(), pending.end(), drawsBefore);
        std::vector<Pending*> flat;
        std::map<quint32, core::ObjectId> idOf;
        flatten(pending, core::Transform{}, flat, idOf);
        const auto resolve = [&idOf](quint32 node) {
            const auto it = idOf.find(node);
            return it == idOf.end() ? core::ObjectId{0} : it->second;
        };
        for (Pending* p : flat) {
            if (auto* group = std::get_if<core::Group>(&p->object)) {
                for (const quint32 member : p->members) {
                    if (const core::ObjectId id = resolve(member)) {
                        group->members.push_back(id);
                    }
                }
            }
            if (auto* c = std::get_if<core::Connector>(&p->object)) {
                c->objectA = resolve(p->connectorA);
                c->objectB = resolve(p->connectorB);
            }
            for (PendingAction& a : p->actions) {
                if (a.targetNode != 0) {
                    a.action.target = resolve(a.targetNode);
                    if (a.action.target == 0) {
                        warn(u"%1 : cible d'action introuvable (nœud %2)"_s.arg(where).arg(a.targetNode));
                    }
                }
                p->object.common.actions.push_back(std::move(a.action));
            }
            page.objects.push_back(std::move(p->object));
        }
        if (!std::is_sorted(page.objects.begin(), page.objects.end(),
                            [](const core::PageObject& a, const core::PageObject& b) {
                                return a.common.layer < b.common.layer;
                            })) {
            std::stable_sort(page.objects.begin(), page.objects.end(),
                             [](const core::PageObject& a, const core::PageObject& b) {
                                 return a.common.layer < b.common.layer;
                             });
            warn(u"%1 : enfants d'objets rangés par couche (ordre de l'arbre modifié)"_s.arg(where));
        }
        return page;
    }

    // Parent puis enfants (triés de même), transformations composées, identifiants dans l'ordre de dessin.
    void flatten(std::vector<Pending>& list, const core::Transform& parent, std::vector<Pending*>& flat,
                 std::map<quint32, core::ObjectId>& idOf)
    {
        for (Pending& p : list) {
            p.object.common.transform = core::compose(p.object.common.transform, parent);
            p.object.common.id = m_nextId++;
            idOf[p.nodeId] = p.object.common.id;
            flat.push_back(&p);
            std::stable_sort(p.children.begin(), p.children.end(), drawsBefore);
            flatten(p.children, p.object.common.transform, flat, idOf);
        }
    }

    PendingAction actionFrom(const IndexNode& node, const StreamObject& o)
    {
        PendingAction p;
        p.action.sourceType = node.type;
        quint16 command = 0;
        quint16 target = 0;
        switch (node.type) {
        case 28:
            p.action.kind = core::ActionKind::OnObject;
            command = 5;
            target = 2;
            break;
        case 26: {
            p.action.kind = core::ActionKind::OpenExternal;
            command = 10;
            target = 4;
            const int rid = intOr(&o, 2, 0);
            if (rid > 0) {
                p.action.resource = fileKey(static_cast<quint32>(rid), nullptr);
            }
            break;
        }
        case 23:
            p.action.kind = core::ActionKind::Navigate;
            command = 4;
            break;
        case 22:
            command = 3;
            break;
        case 24:
            command = 3;
            break;
        case 25:
            command = 4;
            break;
        case 50:
            command = 6;
            target = 3;
            break;
        case 463:
            command = 4;
            break;
        default:
            break;
        }
        p.action.command = command ? intOr(&o, command, 0) : 0;
        if (target) {
            if (const std::optional<Pointer> ptr = asPointer(o.find(target)); ptr && !ptr->isNull()) {
                p.targetNode = ptr->nodeId;
            }
        }
        return p;
    }

    void background(const BambooContainer& container, const StreamObject& bg, core::Page& page)
    {
        const StreamObject* ink = asObject(bg.find(3));
        if (intOr(ink, 8, 0) != 0) {
            page.background.fill = core::solidFill(colorOr(ink, 9, 0xFFFFFFFFu));
            if (const int gradient = intOr(ink, 14, 0); gradient != 0) {
                page.background.fill.kind = core::FillKind::Gradient;
                page.background.fill.gradient = gradientOf(gradient);
                page.background.fill.color2 = colorOr(ink, 13, 0xFF000000u);
            }
        }
        const int kind = intOr(&bg, 2, 0);
        const std::optional<Pointer> ptr = asPointer(bg.find(4));
        if ((kind == 1 || kind == 2) && ptr && !ptr->isNull()) {
            const QString name = u"%1.io"_s.arg(ptr->storageId);
            const BambooContainer& source = container.contains(name) ? container : *m_root;
            QString error;
            const std::optional<StreamObject> image = load(source, name, error);
            if (!image) {
                warn(u"image de fond illisible : %1"_s.arg(error));
                return;
            }
            const int rid = intOr(&*image, 38, 0);
            if (rid > 0) {
                page.background.image = imageKey(static_cast<quint32>(rid));
            }
        }
    }

    void grid(const BambooContainer& container, const IndexNode& node, const StreamObject& g, core::Page& page)
    {
        page.grid.snapEnabled = flagOr(&g, 16, false);
        page.grid.drawnAbove = flagOr(&g, 17, false);
        page.grid.previewZoom = numberOr(&g, 13, page.grid.previewZoom);
        page.grid.baseZoom = numberOr(&g, 14, page.grid.baseZoom);
        page.grid.zoomIncrement = numberOr(&g, 15, page.grid.zoomIncrement);
        for (const IndexNode& child : node.children) {
            if (child.type != 14 || !container.contains(child.storageName())) {
                continue;
            }
            QString error;
            const std::optional<StreamObject> s = load(container, child.storageName(), error);
            if (!s) {
                warn(u"niveau de grille illisible : %1"_s.arg(error));
                continue;
            }
            const StreamObject* l = &*s;
            core::GridLevel level;
            const auto orDefault = [](double v, double fallback) { return v != 0 ? v : fallback; };
            level.spacingX = orDefault(numberOr(l, 7, 32), 32);
            level.spacingY = orDefault(numberOr(l, 8, 32), 32);
            level.color = colorOr(l, 4, 0xFF000000u) | 0xFF000000u;
            level.visible = flagOr(l, 5, true);
            level.snap = flagOr(l, 13, true);
            level.start = asPoint(l->find(6)).value_or(core::PointF{});
            level.angle = orDefault(numberOr(l, 9, 30), 30);
            level.linesAlongX = flagOr(l, 11, true);
            level.linesAlongY = flagOr(l, 12, true);
            const int pattern = intOr(l, 3, 1);
            level.pattern = pattern >= 1 && pattern <= 3 ? static_cast<core::GridPattern>(pattern - 1)
                                                         : core::GridPattern::Normal;
            const int line = intOr(l, 2, 3);
            level.line = line >= 1 && line <= 6 ? static_cast<core::GridLine>(line - 1) : core::GridLine::Solid;
            page.grid.levels.push_back(level);
        }
    }

    std::optional<Pending> toPending(const BambooContainer& container, const IndexNode& node,
                                     const StreamObject& o, const QString& where)
    {
        const quint16 type = node.type;
        Pending p;
        p.nodeId = node.nodeId;
        const core::RectF bounds = asRect(o.find(7)).value_or(core::RectF{});
        const StreamObject* ink = asObject(o.find(10));
        switch (type) {
        case 3: {
            core::Stroke s;
            const core::Outline pen = outlineFrom(ink);
            s.baseWidth = numberOr(ink, 3, 1);
            s.color = pen.color;
            s.cap = pen.cap;
            const int kind = intOr(ink, 2, 0);
            s.kind = kind == 1 ? core::StrokeKind::Highlighter
                               : kind == 2 ? core::StrokeKind::Revealer : core::StrokeKind::Pen;
            const int rid = intOr(&o, 39, 0);
            if (const StreamObject* res = rid > 0 ? resourceObject(static_cast<quint32>(rid)) : nullptr) {
                for (const core::PointF& q : asPolyline(res->find(4))) {
                    s.points.push_back(core::StrokePoint{q.x, q.y, 0.5f});
                }
            }
            p.object = std::move(s);
            p.object.common.translucency =
                static_cast<std::uint8_t>(std::clamp(intOr(&o, 38, 0), 0, 255));
            break;
        }
        case 5: {
            core::Line l;
            l.pen = outlineFrom(ink);
            const int rid = intOr(&o, 38, 0);
            if (const StreamObject* res = rid > 0 ? resourceObject(static_cast<quint32>(rid)) : nullptr) {
                l.points = asPolyline(res->find(4));
            }
            p.object = std::move(l);
            break;
        }
        case 7: {
            core::Shape s;
            s.kind = core::ShapeKind::Path;
            s.bounds = bounds;
            const int rid = intOr(&o, 38, 0);
            if (const StreamObject* res = rid > 0 ? resourceObject(static_cast<quint32>(rid)) : nullptr) {
                s.paths = asPolylines(res->find(4));
            }
            if (s.paths.empty()) {
                if (Polyline local = asPolyline(o.find(49)); !local.empty()) {
                    s.paths.push_back(std::move(local));
                }
            }
            s.open = flagOr(&o, 42, false);
            s.windingFill = intOr(&o, 39, 0) == 2;
            s.pen = outlineFrom(ink);
            s.brush = fillFrom(ink);
            p.object = std::move(s);
            break;
        }
        case 9: {
            core::ImageObject im;
            im.bounds = bounds;
            const int rid = intOr(&o, 38, 0);
            if (rid > 0) {
                im.resource = imageKey(static_cast<quint32>(rid));
                if (const StreamObject* res = resourceObject(static_cast<quint32>(rid))) {
                    if (const std::optional<SizeF> size = asSize(res->find(5));
                        size && std::isfinite(size->width) && std::isfinite(size->height) && size->width >= 0 &&
                        size->height >= 0 && size->width < 1e6 && size->height < 1e6) {
                        im.sourceWidth = static_cast<std::int32_t>(size->width);
                        im.sourceHeight = static_cast<std::int32_t>(size->height);
                    }
                }
            }
            if (flagOr(&o, 39, false)) {
                im.hasKeyColor = true;
                im.keyColor = colorOr(&o, 40, 0xFF000000u);
            }
            p.object = std::move(im);
            break;
        }
        case 11: {
            core::Text t;
            t.bounds = bounds;
            const int rid = intOr(&o, 38, 0);
            if (const StreamObject* res = rid > 0 ? resourceObject(static_cast<quint32>(rid)) : nullptr) {
                t.html = text(res, 5);
                for (const StreamObject* r : asObjects(res->find(4))) {
                    core::TextSpan run;
                    run.text = text(r, 6);
                    run.fontFamily = text(r, 7);
                    run.pointSize = numberOr(r, 8, 12);
                    run.weight = qt6Weight(intOr(r, 9, 50));
                    run.italic = flagOr(r, 10, false);
                    run.underline = flagOr(r, 11, false);
                    run.strikeOut = flagOr(r, 12, false);
                    run.color = colorOr(r, 5, 0xFF000000u);
                    run.baseline = asPoint(r->find(3)).value_or(core::PointF{});
                    run.bounds = asRect(r->find(2)).value_or(core::RectF{});
                    t.runs.push_back(std::move(run));
                }
            }
            p.object = std::move(t);
            break;
        }
        case 18:
            p.object = core::Group{};
            p.members = node.refs;
            break;
        case 253: {
            core::Media m;
            m.bounds = bounds;
            if (const int placeholder = intOr(&o, 38, 0); placeholder > 0) {
                m.placeholder = imageKey(static_cast<quint32>(placeholder));
            }
            if (const int file = intOr(&o, 45, 0); file > 0) {
                m.media = fileKey(static_cast<quint32>(file), &m.fileName);
            }
            m.playOnOpen = flagOr(&o, 41, false);
            m.loop = flagOr(&o, 42, false);
            p.object = std::move(m);
            break;
        }
        case 52: {
            core::Connector c;
            c.firstEnd = asPoint(o.find(39)).value_or(core::PointF{});
            c.secondEnd = asPoint(o.find(40)).value_or(core::PointF{});
            c.pen = outlineFrom(ink);
            if (const std::optional<Pointer> a = asPointer(o.find(34)); a && !a->isNull()) {
                p.connectorA = a->nodeId;
            }
            if (const std::optional<Pointer> b = asPointer(o.find(35)); b && !b->isNull()) {
                p.connectorB = b->nodeId;
            }
            p.object = std::move(c);
            break;
        }
        default:
            warn(u"%1 : objet de type %2 non lu (nœud %3)"_s.arg(where).arg(type).arg(node.nodeId));
            return std::nullopt;
        }

        core::ObjectCommon& c = p.object.common;
        c.name = text(&o, 4);
        p.layer = intOr(&o, 8, 1);
        c.layer = p.layer == 0 ? core::Layer::Bottom : p.layer == 2 ? core::Layer::Top : core::Layer::Middle;
        if (p.layer < 0 || p.layer > 2) {
            warn(u"%1 : couche %2 inconnue (nœud %3), couche intermédiaire"_s.arg(where).arg(p.layer).arg(node.nodeId));
        }
        p.depth = numberOr(&o, 9, 0);
        c.visible = flagOr(&o, 6, true);
        c.locked = flagOr(&o, 18, false);
        c.restrictions.movable = flagOr(&o, 13, true);
        c.restrictions.resizable = flagOr(&o, 14, true);
        c.restrictions.rotatable = flagOr(&o, 15, true);
        c.transform = localTransform(o, bounds);
        if (type != 52) {
            if (const StreamObject* label = asObject(o.find(34))) {
                c.label = text(label, 2);
            }
        }

        int order = 0;
        for (const IndexNode& child : node.children) {
            QString error;
            const std::optional<StreamObject> co = load(container, child.storageName(), error);
            if (!co) {
                warn(u"%1 : %2 illisible : %3"_s.arg(where, child.storageName(), error));
                continue;
            }
            if (ids::isAction(child.type)) {
                p.actions.push_back(actionFrom(child, *co));
                continue;
            }
            if (child.type == 53) {
                continue;
            }
            if (std::optional<Pending> cp = toPending(container, child, *co, where)) {
                cp->order = order++;
                p.children.push_back(std::move(*cp));
            }
        }
        return p;
    }

    const QByteArray& m_data;
    std::optional<BambooContainer> m_root;
    std::map<quint32, ResourceItem> m_register;
    std::map<quint32, std::shared_ptr<const StreamObject>> m_resourceObjects;
    std::map<quint32, std::pair<std::string, std::string>> m_fileKeys; // ressource → (clé, nom de fichier)
    core::Document m_resources; // ne sert qu'à ranger les ressources par empreinte
    QStringList m_warnings;
    core::ObjectId m_nextId = 1;
};

} // namespace

FlipchartReadResult readFlipchart(const QByteArray& data)
{
    return Reader(data).read();
}

bool looksLikeFlipchart(const QByteArray& head) noexcept
{
    return head.startsWith(" Bamboo ");
}

} // namespace actcraft::formats::flipchart
```

  Points d'attention : `core::Argb{c.a} << 24` élargit l'octet avant le décalage ; `p.object = core::Group{}` passe
  par l'affectation du variant (le `using ObjectVariant::operator=` de `PageObject`) ; `std::abs(int)` vient de
  `<cstdlib>` ; `flatten` garde des pointeurs vers les éléments de `pending` et de leurs
  `children` : aucun vecteur n'est modifié après le tri, ils restent valides. Une action sur un objet parent passe
  par `p.actions` (même chemin que la page). Les numéros `ids::*` sont ceux de `flipchartids.h`, aucun autre.

  Compléter `formats/flipchart.cmake` :

```cmake
set(ACTCRAFT_FORMATS_FLIPCHART_SOURCES
    include/actcraft/formats/bamboo.h
    src/bamboo.cpp
    include/actcraft/formats/objectstream.h
    src/objectstream.cpp
    include/actcraft/formats/flipchart.h
    src/flipchartids.h
    src/flipchart.cpp)
```

- [ ] **Étape 7 : vert ciblé.** Commande ciblée, motif `tst_formats_flipchart` : `100% tests passed`. Si un cas
  échoue sur une valeur, relire la référence Python à l'endroit indiqué dans l'objectif avant de changer le code
  ou le test ; ne jamais « ajuster » le paperboard synthétique pour faire passer le lecteur.

- [ ] **Étape 8 : preuve.** `format.ps1`, preuve de tâche (N ≥ 48 ; `tst_formats_flipchart` Passed).
  `python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" <worktree>` finit par `NOMS OK` (aucun nom d'Inspire,
  préfixé ou non, dans les fichiers suivis, classes d'action comprises : nos constantes
  s'appellent `ActionTargetObject`, `ActionGoToPage`, `ActionTool`, `ActionTargetText`, `ActionShift`,
  `ActionProgram`).

- [ ] **Étape 9 : commit.**

```powershell
git -C <worktree> add formats/include/actcraft/formats/flipchart.h formats/src/flipchart.cpp formats/src/flipchartids.h formats/flipchart.cmake tests/flipchart.cmake tests/support/flipchartwriter.h tests/tst_formats_flipchart.cpp
git -C <worktree> commit -m "formats : lecteur .flipchart vers le modèle (tables écrites à la main, ordre couche-profondeur-arbre, ressources par empreinte)" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


<a id="tache-l2-t10"></a>
### Tâche L2-T10 — Oracle : export canonique, `actcraft-flipchart-dump`, script Python de l'ancien dépôt, test local

**Objectif.** Décision 19, DA4 : vérifier le lecteur C++ contre le lecteur Python sur les **17 paperboards réels**
de `samples/`, localement seulement. Deux chemins produisent le **même** JSON canonique (`actcraft-flipchart-dump/1`) :
le script Python de l'ancien dépôt (`tools/oracle_flipchart.py`, sur `read_flipchart`) et `canonicalDump` en C++
(sur `core::Document`). Le test C++ `tst_formats_oracle` relit chaque paperboard en place, compare son export à celui
de l'oracle (nombres à 10⁻³ près) et échoue au premier écart, en le nommant. Sans `ACTCRAFT_SAMPLES_DIR`, il est
sauté et le dit (CI). L'outil `actcraft-flipchart-dump` écrit l'export d'un fichier (débogage, S4 en 2b).

**Patron à ne pas reprendre** (parité de deux chemins) : comparer des textes JSON (Python écrit `1.0`, Qt `1`, l'ordre
des clés diffère), ou laisser chaque côté décider seul de ses règles. Les règles sont écrites **une fois**, ici, et
les deux côtés les appliquent :

| Clé | Règle (identique en Python et en C++) |
|---|---|
| racine | `format` = `"actcraft-flipchart-dump/1"`, `title`, `pages` |
| page | `width`, `height` (réels) ; `background` : `color` (`#rrggbbaa`), `gradient` (0 sans dégradé ; sinon 2 à 5, toute autre valeur → 1), `color2` (`#rrggbbaa` si dégradé, sinon `""`), `image` (SHA-256 des octets, `""` sans image) ; `grid` : `visible`, `levels` (nombre de niveaux) ; `objects` |
| objets | ordre de `Page.walk()` (parent puis enfants) ; seuls les genres `annotation`, `line`, `shape`, `image`, `text`, `group`, `connector`, `media` ; indices de cette liste pour `members` et `target` |
| tout objet | `kind`, `layer` (0, 1, 2), `name`, `visible`, `locked`, `translucency` (entier), `matrix` (6 réels, objet → page, convention QTransform), `actions` : liste de `{type` (numéro de type de l'action), `command` (0 si absent), `target` (indice, −1 si aucun)`}` |
| `annotation` | `pen` : `{color, width}` ; `strokeMode` (0, 1, 2 ; autre → 0) ; `geometry` : `{points, sum: [Σx, Σy]}` (coordonnées locales) |
| `line` | `pen` : `{color, width, style}` (style 0 si largeur ≤ 0, sinon 0 à 5, autre → 1) ; `geometry` |
| `shape` | `pen` ; `brush` : `{kind` (`none` si style 0 ; `gradient` si type de dégradé ≠ 0 ou style 25 ; `pattern` si style 2 à 14 ; `solid` sinon), `color`, `color2` (`""` hors dégradé), `gradient` (règle de page, 0 hors dégradé), `pattern` (style, 0 hors motif)`}` ; `geometry` : `{polygons, points, sum}` ; `open` ; `bounds` (rectangle local) |
| `image` | `bounds` ; `image` (SHA-256 des données) ; `transparent` (booléen) |
| `text` | `bounds` ; `text` : `{html` (SHA-256 de l'UTF-8, `""` si vide), `runs` (nombre)`}` |
| `media` | `bounds` ; `image` (SHA-256 de l'image d'espace réservé, `""` sans) |
| `group` | `members` : indices triés des membres présents dans la liste |
| `connector` | `pen` |

**Fichiers.**
- Créer : `formats/include/actcraft/formats/flipchartdump.h`, `formats/src/flipchartdump.cpp`,
  `bench/flipchart/main.cpp`, `tests/tst_formats_dump.cpp`, `tests/tst_formats_oracle.cpp` ; hors dépôt :
  `C:\Users\JeromeDenis\REA - Activinspire\tools\oracle_flipchart.py`
- Modifier : `formats/flipchart.cmake`, `bench/CMakeLists.txt`, `tests/flipchart.cmake`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_formats_(dump|oracle)` → `error`. Vert : forme
bornée, N ≥ 50, `TESTS OK`, `tst_formats_dump .*Passed` ; `tst_formats_oracle` Passed **sauté** sans la variable.
Oracle local (le cœur de la preuve) :

```powershell
pwsh -NoProfile -Command "Set-Location 'C:\Users\JeromeDenis\REA - Activinspire'; python -E tools/oracle_flipchart.py --samples samples --out build/oracle-flipchart"
pwsh -NoProfile -Command "Set-Location '<worktree>'; New-Item -ItemType Directory -Force .tmp | Out-Null; . ./scripts/dev-env.ps1; `$env:ACTCRAFT_SAMPLES_DIR = 'C:\Users\JeromeDenis\REA - Activinspire\samples'; ctest --preset debug -R '^tst_formats_oracle$' -V *> .tmp/oracle.txt; Select-String -Path .tmp/oracle.txt -Pattern 'ORACLE OK|ECART|FAIL!|tests passed|tests failed' | Select-Object -Last 4"
```

Attendu : `ORACLE OK : 17 paperboards, 124 pages` (script), puis `ORACLE OK : 17 paperboards, 124 pages, 0 écart` et
`100% tests passed` (test).

**Interdits.** Rien de `samples/` n'est écrit, copié ou commité (lecture en place) ; les JSON de l'oracle restent dans
`REA - Activinspire\build\oracle-flipchart\` (dossier ignoré de l'ancien dépôt), jamais dans le worktree. Ne pas
« corriger » un écart en assouplissant une règle de la table. `formats/src/flipchart.cpp` (L2-T9) n'est pas dans les
fichiers de cette tâche : **un écart de lecture est rapporté** (paperboard, page, chemin JSON, deux valeurs, cause
supposée) et l'orchestrateur ouvre un correctif. Seul cas réglé ici : une erreur de l'export canonique lui-même (C++ ou
Python) contre la table ci-dessus.

**Terminé quand.** Preuve verte, oracle local `0 écart` (ou écarts rapportés, voir interdits), script commité dans
l'ancien dépôt, commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire l'oracle Python (hors dépôt public).** Créer
  `C:\Users\JeromeDenis\REA - Activinspire\tools\oracle_flipchart.py` :

```python
# Les <…> désignent des attributs de l'ancien lecteur (app/actcraft/io/flipchart_reader.py), à reprendre
# de ce fichier : leurs noms viennent d'Inspire et n'entrent pas dans le dépôt public (L2-C2).
"""Oracle du lecteur .flipchart d'ActCraft (DA4, décision 19 du cadrage livraison-complete).

Exporte en JSON canonique (« actcraft-flipchart-dump/1 ») ce que lit le lecteur Python de ce dépôt pour chaque
paperboard d'un dossier. Le test C++ tst_formats_oracle d'ActCraft compare son propre export à celui-ci. Règles :
docs/plans/2026-10-15-livraison-complete-lot2a-PLAN.md du dépôt ActCraft, tâche L2-T10.

Usage : python -E tools/oracle_flipchart.py --samples samples --out build/oracle-flipchart
Les JSON restent ici (build/ est ignoré) ; rien n'entre dans le dépôt public d'ActCraft.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "app"))
from actcraft.io import _schema  # noqa: E402  (nom de classe d'action -> numéro de type)
from actcraft.io.flipchart_reader import read_flipchart  # noqa: E402

KINDS = {"annotation", "line", "shape", "image", "text", "group", "connector", "media"}
TYPE_BY_CLASS = {name: number for number, name in _schema.OBJECT_TYPES.items()}


def rgba(c) -> str:
    r, g, b, a = c
    return f"#{r:02x}{g:02x}{b:02x}{a:02x}"


def sha(data) -> str:
    return hashlib.sha256(data).hexdigest() if data else ""


def gradient_rule(value: int) -> int:
    return value if 2 <= value <= 5 else 1


def pen(st) -> dict:
    style = 0 if st.<largeur_plume> <= 0 else (st.<style_plume> if 0 <= st.<style_plume> <= 5 else 1)
    return {"color": rgba(st.<couleur_plume>), "width": float(st.<largeur_plume>), "style": style}


def brush(st) -> dict:
    s = st.<motif_remplissage>
    if s == 0:
        kind, gradient, pattern = "none", 0, 0
    elif st.gradient or s == 25:
        kind, gradient, pattern = "gradient", gradient_rule(st.gradient), 0
    elif 2 <= s <= 14:
        kind, gradient, pattern = "pattern", 0, s
    else:
        kind, gradient, pattern = "solid", 0, 0
    return {"kind": kind, "color": rgba(st.<couleur_remplissage>),
            "color2": rgba(st.<couleur_remplissage_2>) if kind == "gradient" else "",
            "gradient": gradient, "pattern": pattern}


def geometry(points) -> dict:
    return {"points": len(points), "sum": [float(sum(p[0] for p in points)), float(sum(p[1] for p in points))]}


def canonical_object(o, index_of) -> dict:
    st = o.style
    d = {"kind": o.kind, "layer": o.layer_index, "name": o.name, "visible": bool(o.visible),
         "locked": bool(o.locked), "translucency": int(o.translucency), "matrix": list(o.page_matrix())}
    if o.kind == "annotation":
        d["pen"] = {"color": rgba(st.<couleur_plume>), "width": float(st.<largeur_plume>)}
        d["strokeMode"] = st.<genre_trait> if st.<genre_trait> in (0, 1, 2) else 0
        d["geometry"] = geometry(o.data.get("points") or [])
    elif o.kind == "line":
        d["pen"] = pen(st)
        d["geometry"] = geometry(o.data.get("points") or [])
    elif o.kind == "shape":
        polys = o.data.get("polygons") or []
        d["pen"] = pen(st)
        d["brush"] = brush(st)
        d["geometry"] = {"polygons": len(polys), **geometry([p for poly in polys for p in poly])}
        d["open"] = bool(o.data.get("<ouvert>"))
        d["bounds"] = [float(v) for v in o.<rectangle_englobant>]
    elif o.kind == "image":
        img = o.data.get("image")
        d["bounds"] = [float(v) for v in o.<rectangle_englobant>]
        d["image"] = sha(img.data) if img else ""
        d["transparent"] = o.data.get("<couleur_cle>") is not None
    elif o.kind == "text":
        html = o.data.get("html") or ""
        d["bounds"] = [float(v) for v in o.<rectangle_englobant>]
        d["text"] = {"html": hashlib.sha256(html.encode("utf-8")).hexdigest() if html else "",
                     "runs": len(o.data.get("runs") or [])}
    elif o.kind == "media":
        ph = o.data.get("placeholder")
        d["bounds"] = [float(v) for v in o.<rectangle_englobant>]
        d["image"] = sha(ph.data) if ph else ""
    elif o.kind == "group":
        d["members"] = sorted(index_of[m] for m in o.data.get("member_ids", []) if m in index_of)
    elif o.kind == "connector":
        d["pen"] = pen(st)
    d["actions"] = [{"type": TYPE_BY_CLASS.get(a.kind, 0), "command": int(a.<commande> or 0),
                     "target": index_of.get(a.target_node_id, -1)} for a in o.actions]
    return d


def canonical_page(page) -> dict:
    objects = [o for o in page.walk() if o.kind in KINDS]
    index_of = {o.node_id: i for i, o in enumerate(objects)}
    color = page.background_color or (255, 255, 255, 255)
    grad = page.background_gradient
    return {
        "width": float(page.width), "height": float(page.height),
        "background": {"color": rgba(color),
                       "gradient": gradient_rule(grad[0]) if grad else 0,
                       "color2": rgba(grad[1]) if grad else "",
                       "image": sha(page.<image_de_fond>.data) if page.<image_de_fond> else ""},
        "grid": {"visible": bool(page.<grille_visible>), "levels": len(page.grid["styles"]) if page.grid else 0},
        "objects": [canonical_object(o, index_of) for o in objects],
    }


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description="Oracle du lecteur .flipchart d'ActCraft (JSON canonique)")
    ap.add_argument("--samples", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args(argv)
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    files = sorted(Path(a.samples).glob("*.flipchart"))
    pages = 0
    for f in files:
        fc = read_flipchart(f)
        doc = {"format": "actcraft-flipchart-dump/1", "title": fc.title,
               "pages": [canonical_page(p) for p in fc.pages]}
        pages += len(fc.pages)
        (out / (f.stem + ".json")).write_bytes(
            json.dumps(doc, ensure_ascii=False, indent=1, sort_keys=True).encode("utf-8"))
    print(f"ORACLE OK : {len(files)} paperboards, {pages} pages")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
```

  Lancer la première commande de la preuve : `ORACLE OK : 17 paperboards, 124 pages`. Commiter **dans l'ancien
  dépôt**, ce seul fichier (il a d'autres modifications en cours qui ne sont pas les nôtres) :
  `git -C "C:\Users\JeromeDenis\REA - Activinspire" add tools/oracle_flipchart.py` puis
  `git -C "C:\Users\JeromeDenis\REA - Activinspire" commit -m "Oracle du lecteur .flipchart d'ActCraft (JSON canonique, L2-T10)" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>" -- tools/oracle_flipchart.py`
  — le `-- tools/oracle_flipchart.py` final restreint le commit à ce fichier, même si un autre agent a indexé autre
  chose dans ce dépôt entre-temps.

- [ ] **Étape 2 : écrire les tests C++.** Créer `tests/tst_formats_dump.cpp` :

```cpp
#include "support/flipchartwriter.h"

#include <actcraft/formats/flipchart.h>
#include <actcraft/formats/flipchartdump.h>

#include <QJsonArray>
#include <QTest>

using namespace actcraft::formats::flipchart;
using namespace Qt::StringLiterals;

// Export canonique (L2-T10) sur le paperboard synthétique, et comparaison tolérante.
class TstFormatsDump : public QObject
{
    Q_OBJECT

private slots:
    void dumpFollowsTheRules()
    {
        const FlipchartReadResult r = readFlipchart(fctest::sampleFlipchart());
        QVERIFY(r.ok);
        const QJsonObject d = canonicalDump(r.document);
        QCOMPARE(d.value(u"format"_s).toString(), u"actcraft-flipchart-dump/1"_s);
        QCOMPARE(d.value(u"title"_s).toString(), u"Essai synthétique"_s);
        const QJsonArray pages = d.value(u"pages"_s).toArray();
        QCOMPARE(pages.size(), 2);
        const QJsonObject p1 = pages.at(0).toObject();
        QCOMPARE(p1.value(u"background"_s).toObject().value(u"gradient"_s).toInt(), 1);
        QCOMPARE(p1.value(u"background"_s).toObject().value(u"color"_s).toString(), u"#c8dcffff"_s);
        QCOMPARE(p1.value(u"grid"_s).toObject().value(u"levels"_s).toInt(), 1);
        const QJsonArray objects = p1.value(u"objects"_s).toArray();
        QCOMPARE(objects.size(), 4);
        const QJsonObject group = objects.at(0).toObject();
        QCOMPARE(group.value(u"kind"_s).toString(), u"group"_s);
        QCOMPARE(group.value(u"members"_s).toArray(), (QJsonArray{1, 3}));
        const QJsonObject shape = objects.at(1).toObject();
        QCOMPARE(shape.value(u"brush"_s).toObject().value(u"kind"_s).toString(), u"solid"_s);
        QCOMPARE(shape.value(u"brush"_s).toObject().value(u"color2"_s).toString(), QString());
        QCOMPARE(shape.value(u"pen"_s).toObject().value(u"style"_s).toInt(), 2);
        QCOMPARE(shape.value(u"geometry"_s).toObject().value(u"polygons"_s).toInt(), 2);
        QCOMPARE(shape.value(u"geometry"_s).toObject().value(u"points"_s).toInt(), 7);
        const QJsonObject action = shape.value(u"actions"_s).toArray().at(0).toObject();
        QCOMPARE(action.value(u"type"_s).toInt(), 28);
        QCOMPARE(action.value(u"command"_s).toInt(), 184);
        QCOMPARE(action.value(u"target"_s).toInt(), 2);
        const QJsonObject annotation = objects.at(3).toObject();
        QCOMPARE(annotation.value(u"kind"_s).toString(), u"annotation"_s);
        QCOMPARE(annotation.value(u"layer"_s).toInt(), 2);
        QCOMPARE(annotation.value(u"strokeMode"_s).toInt(), 1);
        QCOMPARE(annotation.value(u"pen"_s).toObject().value(u"color"_s).toString(), u"#ffff0080"_s);
        QCOMPARE(annotation.value(u"geometry"_s).toObject().value(u"sum"_s).toArray(), (QJsonArray{200.0, 50.0}));
        QCOMPARE(annotation.value(u"matrix"_s).toArray().size(), 6);
        const QJsonObject p2 = pages.at(1).toObject();
        QVERIFY(!p2.value(u"background"_s).toObject().value(u"image"_s).toString().isEmpty());
        QCOMPARE(p2.value(u"objects"_s).toArray().at(0).toObject().value(u"transparent"_s).toBool(), true);
    }

    void comparisonToleratesTinyNumericDifferences()
    {
        const QJsonObject a{{u"x"_s, 1.0}, {u"s"_s, u"abc"_s}, {u"l"_s, QJsonArray{1, 2}}};
        QVERIFY(compareCanonical(a, QJsonObject{{u"x"_s, 1.0005}, {u"s"_s, u"abc"_s}, {u"l"_s, QJsonArray{1, 2}}})
                    .isEmpty());
        QCOMPARE(compareCanonical(a, QJsonObject{{u"x"_s, 1.01}, {u"s"_s, u"abc"_s}, {u"l"_s, QJsonArray{1, 2}}}).size(),
                 1);
        const QStringList diffs =
            compareCanonical(a, QJsonObject{{u"x"_s, 1.0}, {u"s"_s, u"abd"_s}, {u"l"_s, QJsonArray{1}}});
        QCOMPARE(diffs.size(), 2);
        QVERIFY2(diffs.join(u' ').contains(u"/s"_s), qPrintable(diffs.join(u'\n')));
        QVERIFY(!compareCanonical(a, QJsonObject{{u"x"_s, 1.0}, {u"l"_s, QJsonArray{1, 2}}}).isEmpty());
    }

    void comparisonStopsAtTheLimit()
    {
        QJsonArray expected;
        QJsonArray actual;
        for (int i = 0; i < 50; ++i) {
            expected.append(i);
            actual.append(i + 10);
        }
        QCOMPARE(compareCanonical(expected, actual, 20).size(), 20);
    }
};

QTEST_GUILESS_MAIN(TstFormatsDump)
#include "tst_formats_dump.moc"
```

  Créer `tests/tst_formats_oracle.cpp` :

```cpp
#include <actcraft/formats/flipchart.h>
#include <actcraft/formats/flipchartdump.h>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QTest>

using namespace actcraft::formats::flipchart;
using namespace Qt::StringLiterals;

// Oracle local (décision 19) : chaque paperboard de samples/, lu en place, a le même export canonique que le
// lecteur Python. Sauté sans ACTCRAFT_SAMPLES_DIR (CI, poste sans l'ancien dépôt).
class TstFormatsOracle : public QObject
{
    Q_OBJECT

private slots:
    void everySampleMatchesThePythonReader()
    {
        const QString samples = qEnvironmentVariable("ACTCRAFT_SAMPLES_DIR");
        if (samples.isEmpty()) {
            QSKIP("ACTCRAFT_SAMPLES_DIR absent : oracle local seulement (décision 19, DA4)");
        }
        QString oracle = qEnvironmentVariable("ACTCRAFT_ORACLE_DIR");
        if (oracle.isEmpty()) {
            oracle = QDir(samples).absoluteFilePath(u"../build/oracle-flipchart"_s);
        }
        QVERIFY2(QDir(oracle).exists(),
                 qPrintable(u"JSON de l'oracle absents : %1 (lancer tools/oracle_flipchart.py)"_s.arg(oracle)));
        const QFileInfoList files = QDir(samples).entryInfoList({u"*.flipchart"_s}, QDir::Files, QDir::Name);
        QVERIFY(!files.isEmpty());
        int pages = 0;
        QStringList failures;
        for (const QFileInfo& info : files) {
            QFile file(info.absoluteFilePath());
            QVERIFY(file.open(QIODevice::ReadOnly)); // lecture seule : samples/ n'est jamais modifié
            const FlipchartReadResult r = readFlipchart(file.readAll());
            if (!r.ok) {
                failures << u"%1 : %2"_s.arg(info.fileName(), r.error);
                continue;
            }
            pages += static_cast<int>(r.document.pageCount());
            QFile expectedFile(QDir(oracle).filePath(info.completeBaseName() + u".json"_s));
            if (!expectedFile.open(QIODevice::ReadOnly)) {
                failures << u"%1 : JSON de l'oracle absent"_s.arg(info.fileName());
                continue;
            }
            const QJsonObject expected = QJsonDocument::fromJson(expectedFile.readAll()).object();
            for (const QString& diff : compareCanonical(expected, canonicalDump(r.document))) {
                failures << u"%1 : %2"_s.arg(info.fileName(), diff);
            }
        }
        for (const QString& f : failures.mid(0, 40)) {
            qWarning("ECART %s", qPrintable(f));
        }
        QVERIFY2(failures.isEmpty(), qPrintable(u"%1 écart(s)"_s.arg(failures.size())));
        qInfo("ORACLE OK : %d paperboards, %d pages, 0 écart", static_cast<int>(files.size()), pages);
    }
};

QTEST_GUILESS_MAIN(TstFormatsOracle)
#include "tst_formats_oracle.moc"
```

  Ajouter à `tests/flipchart.cmake` :

```cmake
actcraft_add_test(tst_formats_dump
    SOURCES tst_formats_dump.cpp
    LIBS ActCraft::formats Qt6::Gui)

# Oracle local (décision 19) : sauté sans ACTCRAFT_SAMPLES_DIR ; lit samples/ en place, n'écrit rien.
actcraft_add_test(tst_formats_oracle
    SOURCES tst_formats_oracle.cpp
    LIBS ActCraft::formats)
```

- [ ] **Étape 3 : constater le rouge.** Commande ciblée, motif `tst_formats_(dump|oracle)` → `error`.

- [ ] **Étape 4 : écrire l'export canonique.** Créer `formats/include/actcraft/formats/flipchartdump.h` :

```cpp
#pragma once

#include <actcraft/core/document.h>

#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>

namespace actcraft::formats::flipchart {

inline constexpr char kDumpFormat[] = "actcraft-flipchart-dump/1";

// Export canonique d'un document lu d'un .flipchart (décision 19), comparable à celui de l'oracle Python
// (REA - Activinspire/tools/oracle_flipchart.py). Règles : plan du lot 2a, tâche L2-T10.
QJsonObject canonicalDump(const core::Document& document);

// Écarts entre deux exports, au plus `maxDiffs` : « /pages/0/objects/3/name : "a" → "b" ». Nombres égaux à
// 10⁻³ près (plus 10⁻⁹ relatif) ; clés absentes d'un côté, tailles de listes et types différents comptent.
QStringList compareCanonical(const QJsonValue& expected, const QJsonValue& actual, int maxDiffs = 20);

} // namespace actcraft::formats::flipchart
```

  Créer `formats/src/flipchartdump.cpp` :

```cpp
#include <actcraft/formats/flipchartdump.h>

#include <QJsonArray>
#include <QJsonDocument>

#include <algorithm>
#include <cmath>
#include <map>

namespace actcraft::formats::flipchart {

using namespace Qt::StringLiterals;
using namespace actcraft::core;

namespace {

QString rgba(Argb c)
{
    return u"#%1%2%3%4"_s.arg((c >> 16) & 0xFFu, 2, 16, QLatin1Char('0'))
        .arg((c >> 8) & 0xFFu, 2, 16, QLatin1Char('0'))
        .arg(c & 0xFFu, 2, 16, QLatin1Char('0'))
        .arg((c >> 24) & 0xFFu, 2, 16, QLatin1Char('0'));
}

QString sha(const std::string& key)
{
    return QString::fromStdString(key); // les clés de ressources sont déjà le SHA-256 des octets
}

QJsonArray rect(const RectF& r)
{
    return QJsonArray{r.x, r.y, r.width, r.height};
}

QJsonObject pen(const Outline& o)
{
    return QJsonObject{{u"color"_s, rgba(o.color)},
                       {u"width"_s, o.width},
                       {u"style"_s, o.width <= 0 ? 0 : static_cast<int>(o.style)}};
}

QJsonObject brush(const Fill& f)
{
    QString kind = u"solid"_s;
    switch (f.kind) {
    case FillKind::None:
        kind = u"none"_s;
        break;
    case FillKind::Gradient:
        kind = u"gradient"_s;
        break;
    case FillKind::Pattern:
        kind = u"pattern"_s;
        break;
    case FillKind::Solid:
        break;
    }
    const bool gradient = f.kind == FillKind::Gradient;
    return QJsonObject{{u"kind"_s, kind},
                       {u"color"_s, rgba(f.color)},
                       {u"color2"_s, gradient ? rgba(f.color2) : QString()},
                       {u"gradient"_s, gradient ? static_cast<int>(f.gradient) : 0},
                       {u"pattern"_s, f.kind == FillKind::Pattern ? static_cast<int>(f.pattern) : 0}};
}

template <typename Points> QJsonObject geometry(const Points& points)
{
    double sx = 0;
    double sy = 0;
    for (const auto& p : points) {
        sx += p.x;
        sy += p.y;
    }
    return QJsonObject{{u"points"_s, static_cast<int>(points.size())}, {u"sum"_s, QJsonArray{sx, sy}}};
}

QJsonObject object(const PageObject& o, const std::map<ObjectId, int>& indexOf)
{
    const auto index = [&indexOf](ObjectId id) {
        const auto it = indexOf.find(id);
        return it == indexOf.end() ? -1 : it->second;
    };
    const Transform& t = o.common.transform;
    QJsonObject d{{u"layer"_s, std::max(0, static_cast<int>(o.common.layer) - 1)},
                  {u"name"_s, QString::fromStdString(o.common.name)},
                  {u"visible"_s, o.common.visible},
                  {u"locked"_s, o.common.locked},
                  {u"translucency"_s, static_cast<int>(o.common.translucency)},
                  {u"matrix"_s, QJsonArray{t.m11, t.m12, t.m21, t.m22, t.dx, t.dy}}};
    if (const auto* s = std::get_if<Stroke>(&o)) {
        d.insert(u"kind"_s, u"annotation"_s);
        d.insert(u"pen"_s, QJsonObject{{u"color"_s, rgba(s->color)}, {u"width"_s, s->baseWidth}});
        d.insert(u"strokeMode"_s, static_cast<int>(s->kind));
        d.insert(u"geometry"_s, geometry(s->points));
    } else if (const auto* l = std::get_if<Line>(&o)) {
        d.insert(u"kind"_s, u"line"_s);
        d.insert(u"pen"_s, pen(l->pen));
        d.insert(u"geometry"_s, geometry(l->points));
    } else if (const auto* sh = std::get_if<Shape>(&o)) {
        d.insert(u"kind"_s, u"shape"_s);
        d.insert(u"pen"_s, pen(sh->pen));
        d.insert(u"brush"_s, brush(sh->brush));
        std::vector<PointF> all;
        for (const auto& path : sh->paths) {
            all.insert(all.end(), path.begin(), path.end());
        }
        QJsonObject g = geometry(all);
        g.insert(u"polygons"_s, static_cast<int>(sh->paths.size()));
        d.insert(u"geometry"_s, g);
        d.insert(u"open"_s, sh->open);
        d.insert(u"bounds"_s, rect(sh->bounds));
    } else if (const auto* im = std::get_if<ImageObject>(&o)) {
        d.insert(u"kind"_s, u"image"_s);
        d.insert(u"bounds"_s, rect(im->bounds));
        d.insert(u"image"_s, sha(im->resource));
        d.insert(u"transparent"_s, im->hasKeyColor);
    } else if (const auto* tx = std::get_if<Text>(&o)) {
        d.insert(u"kind"_s, u"text"_s);
        d.insert(u"bounds"_s, rect(tx->bounds));
        d.insert(u"text"_s,
                 QJsonObject{{u"html"_s, tx->html.empty() ? QString()
                                                          : QString::fromStdString(sha256Hex(
                                                                QByteArray::fromStdString(tx->html)))},
                             {u"runs"_s, static_cast<int>(tx->runs.size())}});
    } else if (const auto* m = std::get_if<Media>(&o)) {
        d.insert(u"kind"_s, u"media"_s);
        d.insert(u"bounds"_s, rect(m->bounds));
        d.insert(u"image"_s, sha(m->placeholder));
    } else if (const auto* g = std::get_if<Group>(&o)) {
        d.insert(u"kind"_s, u"group"_s);
        std::vector<int> members;
        for (const ObjectId id : g->members) {
            if (const int i = index(id); i >= 0) {
                members.push_back(i);
            }
        }
        std::sort(members.begin(), members.end());
        QJsonArray list;
        for (const int i : members) {
            list.append(i);
        }
        d.insert(u"members"_s, list);
    } else if (const auto* c = std::get_if<Connector>(&o)) {
        d.insert(u"kind"_s, u"connector"_s);
        d.insert(u"pen"_s, pen(c->pen));
    }
    QJsonArray actions;
    for (const Action& a : o.common.actions) {
        actions.append(QJsonObject{{u"type"_s, static_cast<int>(a.sourceType)},
                                   {u"command"_s, static_cast<int>(a.command)},
                                   {u"target"_s, a.target == 0 ? -1 : index(a.target)}});
    }
    d.insert(u"actions"_s, actions);
    return d;
}

QString text(const QJsonValue& v)
{
    if (v.isUndefined()) {
        return u"<absent>"_s;
    }
    QJsonArray wrapper{v};
    const QByteArray json = QJsonDocument(wrapper).toJson(QJsonDocument::Compact);
    return QString::fromUtf8(json.mid(1, json.size() - 2));
}

void compare(const QJsonValue& e, const QJsonValue& a, const QString& path, QStringList& diffs, int maxDiffs)
{
    if (diffs.size() >= maxDiffs) {
        return;
    }
    if (e.isDouble() && a.isDouble()) {
        const double x = e.toDouble();
        const double y = a.toDouble();
        if (std::abs(x - y) > 1e-3 + 1e-9 * std::max(std::abs(x), std::abs(y))) {
            diffs << u"%1 : %2 → %3"_s.arg(path, text(e), text(a));
        }
        return;
    }
    if (e.type() != a.type()) {
        diffs << u"%1 : %2 → %3"_s.arg(path, text(e), text(a));
        return;
    }
    if (e.isObject()) {
        const QJsonObject eo = e.toObject();
        const QJsonObject ao = a.toObject();
        QStringList keys = eo.keys();
        for (const QString& k : ao.keys()) {
            if (!eo.contains(k)) {
                keys << k;
            }
        }
        for (const QString& k : keys) {
            compare(eo.value(k), ao.value(k), path + u'/' + k, diffs, maxDiffs);
        }
        return;
    }
    if (e.isArray()) {
        const QJsonArray ea = e.toArray();
        const QJsonArray aa = a.toArray();
        if (ea.size() != aa.size()) {
            diffs << u"%1 : %2 éléments → %3"_s.arg(path).arg(ea.size()).arg(aa.size());
        }
        for (qsizetype i = 0; i < std::min(ea.size(), aa.size()); ++i) {
            compare(ea.at(i), aa.at(i), u"%1/%2"_s.arg(path).arg(i), diffs, maxDiffs);
        }
        return;
    }
    if (e != a) {
        diffs << u"%1 : %2 → %3"_s.arg(path, text(e), text(a));
    }
}

} // namespace

QJsonObject canonicalDump(const Document& document)
{
    QJsonArray pages;
    for (std::size_t p = 0; p < document.pageCount(); ++p) {
        const Page& page = document.page(p);
        std::map<ObjectId, int> indexOf;
        for (std::size_t i = 0; i < page.objects.size(); ++i) {
            indexOf[page.objects[i].common.id] = static_cast<int>(i);
        }
        QJsonArray objects;
        for (const PageObject& o : page.objects) {
            objects.append(object(o, indexOf));
        }
        const Fill& fill = page.background.fill;
        const bool gradient = fill.kind == FillKind::Gradient;
        pages.append(QJsonObject{
            {u"width"_s, page.width},
            {u"height"_s, page.height},
            {u"background"_s, QJsonObject{{u"color"_s, rgba(fill.color)},
                                          {u"gradient"_s, gradient ? static_cast<int>(fill.gradient) : 0},
                                          {u"color2"_s, gradient ? rgba(fill.color2) : QString()},
                                          {u"image"_s, sha(page.background.image)}}},
            {u"grid"_s, QJsonObject{{u"visible"_s, page.grid.visible},
                                    {u"levels"_s, static_cast<int>(page.grid.levels.size())}}},
            {u"objects"_s, objects}});
    }
    return QJsonObject{{u"format"_s, QString::fromLatin1(kDumpFormat)},
                       {u"title"_s, QString::fromStdString(document.title())},
                       {u"pages"_s, pages}};
}

QStringList compareCanonical(const QJsonValue& expected, const QJsonValue& actual, int maxDiffs)
{
    QStringList diffs;
    compare(expected, actual, QString(), diffs, maxDiffs);
    return diffs;
}

} // namespace actcraft::formats::flipchart
```

  `u"#%1…"_s.arg(uint, 2, 16, QChar)` écrit l'hexadécimal en minuscules, comme le `f"{r:02x}"` du script. Le premier
  niveau d'un chemin d'écart commence par `/` (`path + u'/' + k`), d'où `/s` dans le test.

  Compléter `formats/flipchart.cmake` (ajouter les deux fichiers à la liste) :

```cmake
    include/actcraft/formats/flipchartdump.h
    src/flipchartdump.cpp
```

- [ ] **Étape 5 : l'outil.** Créer `bench/flipchart/main.cpp` :

```cpp
#include <actcraft/formats/flipchart.h>
#include <actcraft/formats/flipchartdump.h>

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>

#include <cstdio>

// actcraft-flipchart-dump <fichier.flipchart> [--sortie <fichier.json>] : export canonique du lecteur (oracle,
// décision 19). Outil de preuve, jamais livré. Dernière ligne : « DUMP OK : <n> pages » (0) ou
// « DUMP EN ECHEC : <raison> » (1).
int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    const QStringList args = QCoreApplication::arguments();
    const auto fail = [](const QString& why) {
        std::printf("DUMP EN ECHEC : %s\n", qPrintable(why));
        return 1;
    };
    if (args.size() < 2 || args.at(1).startsWith(QLatin1String("--"))) {
        return fail(QStringLiteral("usage : actcraft-flipchart-dump <fichier.flipchart> [--sortie <fichier.json>]"));
    }
    QFile in(args.at(1));
    if (!in.open(QIODevice::ReadOnly)) {
        return fail(QStringLiteral("ouverture impossible : %1").arg(args.at(1)));
    }
    const auto r = actcraft::formats::flipchart::readFlipchart(in.readAll());
    if (!r.ok) {
        return fail(r.error);
    }
    for (const QString& w : r.warnings) {
        std::printf("AVERTISSEMENT %s\n", qPrintable(w));
    }
    const QByteArray json =
        QJsonDocument(actcraft::formats::flipchart::canonicalDump(r.document)).toJson(QJsonDocument::Indented);
    const qsizetype at = args.indexOf(QStringLiteral("--sortie"));
    if (at > 0 && at + 1 < args.size()) {
        QFile out(args.at(at + 1));
        if (!out.open(QIODevice::WriteOnly) || out.write(json) != json.size()) {
            return fail(QStringLiteral("écriture impossible : %1").arg(args.at(at + 1)));
        }
    } else {
        std::fwrite(json.constData(), 1, static_cast<std::size_t>(json.size()), stdout);
    }
    std::printf("DUMP OK : %d pages\n", static_cast<int>(r.document.pageCount()));
    return 0;
}
```

  Ajouter à la fin de `bench/CMakeLists.txt` :

```cmake
# Export canonique d'un .flipchart (oracle du lecteur, décision 19) : outil de preuve, jamais livré.
qt_add_executable(actcraft-flipchart-dump flipchart/main.cpp)
target_link_libraries(actcraft-flipchart-dump PRIVATE ActCraft::formats Qt6::Core)
actcraft_set_warnings(actcraft-flipchart-dump)
set_target_properties(actcraft-flipchart-dump PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
```

- [ ] **Étape 6 : vert ciblé.** Commande ciblée, motif `tst_formats_(dump|oracle)` : `100% tests passed`
  (`tst_formats_oracle` sauté : la ligne `SKIP` est dans `.tmp/ctest.txt` avec `-V`).

- [ ] **Étape 7 : oracle local.** Les deux commandes de la section « Test et commande bornée ». Attendu :
  `ORACLE OK : 17 paperboards, 124 pages` puis `ORACLE OK : 17 paperboards, 124 pages, 0 écart`. Si des lignes
  `ECART` sortent : vérifier d'abord que les deux exports suivent la table (erreur d'export → corriger l'export du côté
  fautif, relancer) ; sinon c'est une différence de lecture : la rapporter telle quelle (paperboard, chemin JSON, deux
  valeurs, cause supposée) sans toucher à `formats/src/flipchart.cpp`, et terminer la tâche en signalant le correctif
  à ouvrir. Lancer aussi l'outil sur un paperboard, sortie sous `.tmp/` :
  `pwsh -NoProfile -Command "Set-Location '<worktree>'; . ./scripts/dev-env.ps1; ./build/debug/actcraft-flipchart-dump.exe 'C:\Users\JeromeDenis\REA - Activinspire\samples\tri des formes.flipchart' --sortie .tmp/tri.json | Select-Object -Last 1"`
  → `DUMP OK : <n> pages` ; noter `<n>` au rapport (L2-T21 s'en sert).

- [ ] **Étape 8 : preuve.** `format.ps1`, preuve de tâche (N ≥ 50 ; `tst_formats_dump` Passed).

- [ ] **Étape 9 : commit.**

```powershell
git -C <worktree> add formats/include/actcraft/formats/flipchartdump.h formats/src/flipchartdump.cpp formats/flipchart.cmake bench/flipchart/main.cpp bench/CMakeLists.txt tests/flipchart.cmake tests/tst_formats_dump.cpp tests/tst_formats_oracle.cpp
git -C <worktree> commit -m "Oracle du lecteur .flipchart : export canonique, actcraft-flipchart-dump, test local sur les 17 paperboards" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```


---

## Hypothèses nouvelles de 2a (à revoir, reprises à la porte de fin du lot 2)

- **H2a-1 — Index « culm » non fermé refusé** (L2-T7). La référence Python l'accepte ; ActCraft le refuse pour ne
  pas prendre un fichier tronqué pour complet. Si l'oracle (L2-T10) trouve un échantillon réel concerné, le consigner.
- **H2a-2 — Bouts des traits** (L2-T1). Les traits du stylo d'ActCraft gardent leurs bouts carrés (moteur du
  prototype) ; les annotations lues ont les leurs (ronds le plus souvent). Le lot 3 (stylo, marqueur) tranchera
  pour les nouveaux traits.
- **H2a-3 — Paperboard sans nom de titre** : titre = nom du paperboard (`Name`), comme la référence ; sinon vide (le
  nom du fichier s'affichera, 2b).

## Point d'étape 2a (orchestrateur, hors tâches)

Joué par l'orchestrateur après la vague V5, dans le worktree du chantier, avant de lancer 2b. **Pas de PR** : la PR
du lot 2 est créée à la clôture de 2b.

1. **Suite complète** : `scripts/test.ps1 -Preset release` puis `scripts/test.ps1` (formes bornées du profil),
   N ≥ 55 chacune, `TESTS OK`, aucun test retiré, aucun `QSKIP` ajouté hors `tst_formats_oracle`.
2. **Oracle local (S4, partie lecteur)** : les deux commandes de L2-T10 → `ORACLE OK : 17 paperboards, 124 pages`
   puis `ORACLE OK : 17 paperboards, 124 pages, 0 écart`. Un écart restant = tâche de correction avant 2b.
3. **Contrôles du dépôt public** (sorties dans l'état) :
   - `git -C <worktree> grep -n "Private\|_p\.h" -- core formats actions render ui app` → vide ;
   - `git -C <worktree> ls-files -- '*.flipchart' '*.thm' 'third_party/*'` → seulement les cinq fichiers de
     `third_party/miniz/` ;
   - `git -C <worktree> grep -n -i "Promethean" -- formats core tests bench` → vide (le mot n'apparaît que
     dans la documentation et les commentaires de propriété intellectuelle) ;
   - `git -C <worktree> grep -n "qUncompress" -- formats/src` → vide (décompression bornée par miniz, L2-T7) ;
   - `python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" <worktree>` → `NOMS OK` (aucun nom de
     classe ou de propriété d'Inspire, préfixé ou non, dans les fichiers suivis ; nos classes d'action :
     `ActionTargetObject`, `ActionGoToPage`, `ActionTool`, `ActionTargetText`, `ActionShift`, `ActionProgram`).
4. **État** : tâches L2-T1 à L2-T10 terminées, compteurs, pièges rencontrés ; puis lancement de
   `docs/plans/2026-10-15-livraison-complete-lot2b-PLAN.md` (sa table) sur la même branche.
