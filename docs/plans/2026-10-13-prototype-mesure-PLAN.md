# Prototype de mesure (étape 1 du BRIEF) — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**But.** Une fenêtre QML montrant une page de 1024 × 768 à 300 objets (formes, images, traits), avec tracé au
stylo incrémental, zoom, défilement et contenu fixe en cache, plus le banc `actcraft-bench` qui rejoue tracé,
zoom et défilement en Direct3D 11 et en rendu logiciel et écrit temps par image, latence et mémoire dans un JSON.

**Architecture.** `core/` porte le modèle, le générateur déterministe et la sonde de latence ; `render/` un
registre de moteurs (forme, image, trait) à deux chemins (nœuds de géométrie pour le RHI, `QSGRenderNode` peint
au `QPainter` pour le logiciel), l'élément « page fixe » et l'élément d'encre par blocs de 256 points ;
`input/` la chaîne d'échantillons ; `tools/` le `PenTool` ; `ui/` la vue `PageView` et `Main.qml` (cache par
`ShaderEffectSource`) ; `bench/` l'injecteur, les collecteurs, les statistiques, le JSON, le verdict et
l'exécutable. Cadrage qui fait autorité : `docs/specs/2026-10-13-prototype-mesure-cadrage.md` (commit 3a217ec).

**Pile.** C++20, Qt 6.12 (Core, Gui, Qml, Quick, Test), CMake + Ninja, MSVC 2022 ; CI Windows + macOS (Clang).

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| T1 | Profil (banc = résultat), tests par module, `/Ob2`, Objective-C++ | — | `.claude/chantier.md`, `CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/verifications.cmake`, `tests/core.cmake`, `tests/render.cmake`, `tests/input.cmake`, `tests/tools.cmake`, `tests/ui.cmake`, `tests/bench.cmake` | partagé | implementeur-tdd | TESTS OK, ≥ 3 tests ; `/Ob2` dans `build/release/build.ninja` | S1 | #tache-t1 |
| T2 | Vérifications Qt 6.12 (nœud logiciel, couche logicielle, tablette) | T1 | `tests/verifications.cmake`, `tests/tst_qt_verifications.cpp`, `docs/verifications/2026-10-13-qt612.md` | isolé | concepteur-critique | TESTS OK, ≥ 4 tests, `tst_qt_verifications` Passed ; 5 lignes `VERIF` | S2 | #tache-t2 |
| T3 | `core` : modèle, empreinte, générateur 300 objets, sonde, horloge | T1 | `core/CMakeLists.txt`, `core/include/actcraft/core/model.h`, `core/include/actcraft/core/generator.h`, `core/include/actcraft/core/clock.h`, `core/include/actcraft/core/inkprobe.h`, `core/src/model.cpp`, `core/src/generator.cpp`, `tests/core.cmake`, `tests/tst_core_model.cpp`, `tests/tst_core_generator.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 5 tests, `tst_core_model` et `tst_core_generator` Passed | S4 | #tache-t3 |
| T4 | `render` : registre, moteurs forme et trait (RHI + logiciel) | T2, T3 | `render/CMakeLists.txt`, `render/include/actcraft/render/strokegeometry.h`, `render/include/actcraft/render/painternode.h`, `render/include/actcraft/render/objectrenderer.h`, `render/include/actcraft/render/shaperenderer.h`, `render/include/actcraft/render/strokerenderer.h`, `render/src/strokegeometry.cpp`, `render/src/painternode.cpp`, `render/src/nodes.h`, `render/src/nodes.cpp`, `render/src/registry.cpp`, `render/src/shaperenderer.cpp`, `render/src/strokerenderer.cpp`, `tests/render.cmake`, `tests/support/renderhost.h`, `tests/tst_render_engines.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 7 tests, `tst_render_engines` Passed | S2 | #tache-t4 |
| T5 | `render` : images procédurales sur fil secondaire, moteur image | T4 | `render/CMakeLists.txt`, `render/include/actcraft/render/imagesynth.h`, `render/include/actcraft/render/imagestore.h`, `render/include/actcraft/render/imagerenderer.h`, `render/src/imagesynth.cpp`, `render/src/imagestore.cpp`, `render/src/imagerenderer.cpp`, `render/src/registry.cpp`, `tests/render.cmake`, `tests/tst_render_images.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 8 tests, `tst_render_images` Passed | S2 | #tache-t5 |
| T6 | `render` : élément « page fixe » | T5 | `render/CMakeLists.txt`, `render/include/actcraft/render/fixedpageitem.h`, `render/src/fixedpageitem.cpp`, `tests/render.cmake`, `tests/tst_render_fixedpage.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 9 tests, `tst_render_fixedpage` Passed | S2 | #tache-t6 |
| T7 | `input` : échantillons, séquence, filtre, prédiction identité | T3 | `input/CMakeLists.txt`, `input/include/actcraft/input/sample.h`, `input/include/actcraft/input/pipeline.h`, `input/include/actcraft/input/qtevents.h`, `input/src/pipeline.cpp`, `input/src/qtevents.cpp`, `tests/input.cmake`, `tests/tst_input.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 6 tests, `tst_input` Passed | S5 | #tache-t7 |
| T8 | `tools` : `PenTool` | T7 | `tools/CMakeLists.txt`, `tools/include/actcraft/tools/pentool.h`, `tools/src/pentool.cpp`, `tests/tools.cmake`, `tests/tst_pentool.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 7 tests, `tst_pentool` Passed | S5 | #tache-t8 |
| T9 | `render` : tampon d'encre par blocs, élément d'encre (chemin RHI, sonde) | T6 | `render/CMakeLists.txt`, `render/include/actcraft/render/inkbuffer.h`, `render/include/actcraft/render/inkitem.h`, `render/src/inkbuffer.cpp`, `render/src/inkitem.cpp`, `tests/render.cmake`, `tests/tst_render_inkbuffer.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 10 tests, `tst_render_inkbuffer` Passed | S1 | #tache-t9 |
| T10 | `render` : encre en rendu logiciel (un `QSGRenderNode` par bloc) | T9 | `render/include/actcraft/render/inkitem.h`, `render/src/inkitem.cpp`, `tests/render.cmake`, `tests/tst_render_ink_software.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 11 tests, `tst_render_ink_software` Passed | S2 | #tache-t10 |
| T11 | `ui` : `PageView` (zoom, défilement, politique du cache) | T1 | `ui/CMakeLists.txt`, `ui/include/actcraft/ui/viewtransform.h`, `ui/include/actcraft/ui/pageview.h`, `ui/src/viewtransform.cpp`, `ui/src/pageview.cpp`, `tests/ui.cmake`, `tests/tst_viewtransform.cpp`, `tests/tst_pageview.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 5 tests, `tst_viewtransform` et `tst_pageview` Passed | S5 | #tache-t11 |
| T12 | `ui` + `app` : `Main.qml`, câblage stylo / encre / cache, `--sans-cache` | T6, T8, T10, T11 | `ui/CMakeLists.txt`, `ui/qml/Main.qml`, `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`, `ui/src/uiforeigntypes.h`, `ui/src/penadapters.h`, `ui/src/penadapters.cpp`, `app/startup.h`, `app/startup.cpp`, `app/main.cpp`, `app/CMakeLists.txt`, `tests/ui.cmake`, `tests/tst_ui.cpp`, `tests/tst_ui_drawing.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 16 tests, `tst_ui` et `tst_ui_drawing` Passed | S5 | #tache-t12 |
| T13 | `bench` : statistiques (intervalles actifs, figements, latence) | T1 | `bench/CMakeLists.txt`, `bench/include/actcraft/bench/stats.h`, `bench/src/stats.cpp`, `tests/bench.cmake`, `tests/tst_bench_stats.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 4 tests, `tst_bench_stats` Passed | S1 | #tache-t13 |
| T14 | `bench` : mémoire, machine, fichier JSON | T13 | `bench/CMakeLists.txt`, `bench/include/actcraft/bench/memory.h`, `bench/include/actcraft/bench/machineinfo.h`, `bench/include/actcraft/bench/result.h`, `bench/src/memory.cpp`, `bench/src/gpumemory_win.cpp`, `bench/src/gpumemory_mac.mm`, `bench/src/gpumemory_other.cpp`, `bench/src/machineinfo.cpp`, `bench/src/result.cpp`, `bench/src/buildinfo.h.in`, `tests/bench.cmake`, `tests/tst_bench_json.cpp`, `tests/tst_bench_memory.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 6 tests, `tst_bench_json` et `tst_bench_memory` Passed | S1 | #tache-t14 |
| T15 | `bench` : scénarios et injecteur avec rattrapage | T3, T11, T14 | `bench/CMakeLists.txt`, `bench/include/actcraft/bench/scenarios.h`, `bench/include/actcraft/bench/injector.h`, `bench/src/scenarios.cpp`, `bench/src/injector.cpp`, `tests/bench.cmake`, `tests/tst_bench_scenarios.cpp`, `tests/tst_bench_injector.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 12 tests, `tst_bench_scenarios` et `tst_bench_injector` Passed | S5 | #tache-t15 |
| T16 | `bench` : options, collecteur, déroulé, verdict, exécutable `actcraft-bench` | T12, T15 | `bench/CMakeLists.txt`, `bench/include/actcraft/bench/options.h`, `bench/include/actcraft/bench/verdict.h`, `bench/include/actcraft/bench/recorder.h`, `bench/include/actcraft/bench/runner.h`, `bench/src/options.cpp`, `bench/src/verdict.cpp`, `bench/src/recorder.cpp`, `bench/src/runner.cpp`, `bench/app/main.cpp`, `tests/bench.cmake`, `tests/tst_bench_options.cpp`, `tests/tst_bench_verdict.cpp` | isolé | concepteur-critique | TESTS OK, ≥ 23 tests, `tst_bench_options` et `tst_bench_verdict` Passed ; passage hors écran : JSON à 3 scénarios | S1 | #tache-t16 |
| T17 | `bench` : démonstration, captures, contrôle de contenu | T16 | `bench/CMakeLists.txt`, `bench/include/actcraft/bench/contentcheck.h`, `bench/src/contentcheck.cpp`, `bench/include/actcraft/bench/runner.h`, `bench/src/runner.cpp`, `tests/bench.cmake`, `tests/tst_bench_content.cpp` | isolé | implementeur-tdd | TESTS OK, ≥ 24 tests, `tst_bench_content` Passed ; passage hors écran : `content_check.ok = true`, 4 PNG | S2 | #tache-t17 |
| T18 | Fumée ctest du banc, `tst_bench_result`, pas CI macOS | T17 | `tests/bench.cmake`, `tests/run_bench_smoke.cmake`, `tests/tst_bench_result.cpp`, `.github/workflows/ci.yml` | isolé | implementeur-tdd | TESTS OK, ≥ 27 tests, `tst_bench_smoke` et `tst_bench_result` Passed | S4 | #tache-t18 |
| T19 | Scripts `bench.ps1`, `bench.sh`, `paquet-banc.ps1` ; commandes dans le profil | T17 | `scripts/bench.ps1`, `scripts/bench.sh`, `scripts/paquet-banc.ps1`, `.claude/chantier.md` | isolé | implementeur-tdd | TESTS OK, ≥ 24 tests ; `bench.ps1 -HorsEcran` → `BUDGET TENU` ou `BUDGET DEPASSE` ; `bash -n` OK | S1 | #tache-t19 |
| T20 | Résumé `resume-banc.ps1` et procédure `docs/banc.md` | T18, T19 | `scripts/resume-banc.ps1`, `docs/banc.md`, `tests/bench.cmake` | isolé | implementeur-tdd | TESTS OK, ≥ 28 tests, `tst_bench_summary` Passed | S3 | #tache-t20 |

20 tâches, un seul lot. Les passages du banc en plein écran (S1, S2, S3, S5) et le résumé chiffré commité
dans `docs/mesures/` sont joués **à la clôture par l'orchestrateur**, sous le jeton `ecran` : voir la section
« Clôture — passages au banc (orchestrateur) ». Aucune tâche ne lance de fenêtre en plein écran.

Le nombre minimal de tests de la colonne `preuve` compte les tests de la référence (3 : `tst_modules`,
`tst_renderbackend`, `tst_ui`) plus ceux des tâches dont la tâche dépend, directement ou non, plus les siens.
Une tâche lancée après d'autres tâches indépendantes en voit davantage : « ≥ » est la règle.

## Couverture des scénarios

| Scénario | Tâches | Joué à la clôture |
|---|---|---|
| S1 — banc D3D11 en release | T1 (`/Ob2`, profil), T9 (encre RHI), T13, T14, T16, T19 | oui (`bench.ps1 -Captures .tmp/s1`) |
| S2 — banc logiciel et captures | T2, T4, T5, T6, T10, T17 | oui (même passage) |
| S3 — cache chiffré | T20 (résumé) ; `--sans-cache` livré par T12 (application) et T16 (banc) | oui (`bench.ps1 -SansCache`, résumé commité) |
| S4 — fumée en CI | T3 (empreinte Windows = macOS), T18 | CI de la PR |
| S5 — prototype rejoué sans souris | T7, T8, T11, T12, T15 | oui (`--taille 1280x800 --captures .tmp/s5`) |

## Vagues proposées (parallélisme 2 au plus)

V1 : T1 · V2 : T2 ‖ T3 · V3 : T4 ‖ T7 · V4 : T5 ‖ T8 · V5 : T6 ‖ T11 · V6 : T9 ‖ T13 · V7 : T10 ‖ T14 ·
V8 : T12 ‖ T15 · V9 : T16 · V10 : T17 · V11 : T18 ‖ T19 · V12 : T20.
Deux tâches d'une même vague ne partagent aucun fichier (vérifié sur la colonne `fichiers touchés`).

## Carte des fichiers

| Fichier | Responsabilité | Tâche |
|---|---|---|
| `tests/<module>.cmake` | déclarations `actcraft_add_test` d'un module (inclus par `tests/CMakeLists.txt`) | T1, puis la tâche du module |
| `core/include/actcraft/core/model.h` | `Page`, `Shape`, `ImageObject`, `Stroke`, `Argb`, `fingerprint` | T3 |
| `core/include/actcraft/core/generator.h` | `generatePage`, témoins, `hitsWitness` | T3 |
| `core/include/actcraft/core/clock.h` | `monotonicNowNs()` : horloge unique | T3 |
| `core/include/actcraft/core/inkprobe.h` | interface `InkProbe` (DA4) | T3 |
| `render/include/actcraft/render/strokegeometry.h` | triangulation des traits, contours, remplissages | T4 |
| `render/include/actcraft/render/painternode.h` | base `PainterNode` : `QSGRenderNode` peint au `QPainter` | T4 |
| `render/include/actcraft/render/objectrenderer.h` | `ObjectRenderer`, `RendererRegistry`, `RenderContext` | T4 (T5 ajoute l'image) |
| `render/include/actcraft/render/{shape,stroke,image}renderer.h` | un moteur par type | T4, T5 |
| `render/include/actcraft/render/imagesynth.h`, `imagestore.h` | images procédurales, génération sur `QThreadPool` | T5 |
| `render/include/actcraft/render/fixedpageitem.h` | élément « page fixe » (modèle → nœuds) | T6 |
| `render/include/actcraft/render/inkbuffer.h`, `inkitem.h` | encre incrémentale par blocs, deux chemins | T9, T10 |
| `input/include/actcraft/input/*.h` | `Sample`, `Pipeline`, `DuplicateFilter`, `IdentityPredictor`, conversion des événements Qt | T7 |
| `tools/include/actcraft/tools/pentool.h` | `PenTool`, `InkSurface`, `DocumentSink` | T8 |
| `ui/include/actcraft/ui/viewtransform.h` | calculs purs de zoom, défilement, taille du cache | T11 |
| `ui/include/actcraft/ui/pageview.h` | élément `PageView` (QML) | T11, T12 |
| `ui/src/uiforeigntypes.h`, `ui/src/penadapters.*` | `FixedPage` / `InkLayer` en QML ; adaptateurs du stylo | T12 |
| `app/startup.*` | chargement commun de `Main` (application et banc) | T12 |
| `bench/include/actcraft/bench/stats.h` | intervalles actifs, figements, percentiles, latence | T13 |
| `bench/include/actcraft/bench/{memory,machineinfo,result}.h` | mémoire, métadonnées, JSON `actcraft-bench/1` | T14 |
| `bench/include/actcraft/bench/{scenarios,injector}.h` | scénarios rejoués, injecteur à rattrapage | T15 |
| `bench/include/actcraft/bench/{options,verdict,recorder,runner}.h`, `bench/app/main.cpp` | exécutable `actcraft-bench` | T16 |
| `bench/include/actcraft/bench/contentcheck.h` | captures et 4 contrôles de contenu | T17 |
| `tests/run_bench_smoke.cmake`, `tests/tst_bench_result.cpp` | fumée ctest | T18 |
| `scripts/bench.ps1`, `scripts/bench.sh`, `scripts/paquet-banc.ps1` | lancement du banc, paquet Dell | T19 |
| `scripts/resume-banc.ps1`, `docs/banc.md` | résumé Markdown, procédure de l'étape 2 | T20 |

**Pourquoi un fichier de tests par module.** `tests/CMakeLists.txt` serait touché par presque toutes les
tâches : deux tâches parallèles s'y croiseraient au merge. T1 le découpe en `tests/<module>.cmake` (inclus par
`include()` ; la fonction `actcraft_add_test` reste dans `tests/CMakeLists.txt`, et chaque test y est donc
toujours déclaré par `actcraft_add_test`, comme l'exige le profil). Les tâches d'un même module se suivent par
leurs dépendances ; deux modules différents avancent en parallèle.

## Contrats partagés entre tâches (noms figés)

Une tâche ne renomme aucun de ces noms.

- `actcraft::core` : `Argb`, `PointF`, `RectF`, `ObjectKind`, `ShapeKind`, `Shape`, `ImagePattern`,
  `ImageObject`, `StrokePoint`, `Stroke`, `PageObject`, `Page`, `kindOf`, `fingerprint`, `strokeWidthAt`,
  `kDefaultSeed`, `Witnesses`, `witnesses()`, `hitsWitness`, `generatePage`, `monotonicNowNs`, `InkProbe`
  (`sampleReceived(seq, nowNs)`, `samplesSynced(lastSeq, nowNs)`).
- `actcraft::render` : `Vertex2D`, `appendStrokeTriangles`, `appendOutlineTriangles`, `appendConvexFill`,
  `ellipsePolygon`, `strokeBounds`, `kVerticesPerSegment`, `toQColor`, `PainterNode`, `RenderContext`,
  `ObjectRenderer`, `RendererRegistry`, `ImageStore`, `synthesizeImage`, `FixedPageItem` (`loadGeneratedPage`,
  `setPage`, `appendStroke`, `isReady`, `objectCount`, `readyChanged`, `strokeAppended`), `InkBuffer`,
  `InkItem` (`beginStroke`, `addPoint`, `clear`, `setProbe`, `pointCount`, `blockCount`).
- `actcraft::input` : `Sample`, `DeviceKind`, `Phase`, `SampleSink`, `DuplicateFilter`, `IdentityPredictor`,
  `Pipeline` (`push`, `resetSequence`, `nextSequence`, `setProbe`), `sampleFromEvent`.
- `actcraft::tools` : `InkSurface`, `DocumentSink`, `PenTool`.
- `actcraft::ui` : `ViewTransform`, `clampZoom`, `wheelZoomFactor`, `zoomAt`, `clampPan`, `fitPage`,
  `wheelScrollDelta`, `cacheTextureSize`, `kViewMargin`, `kMinZoom`, `kMaxZoom`, `kCacheRefreshDelayMs`,
  `PageView` (`setZoom`, `setPan`, `fitPage`, `requestCacheRefresh`, `notifyCacheUpdated`, `pageToScene`,
  `setExclusiveDevices`, `setInkProbe`, `resetInputSequence`, `cacheReady`, `cacheEnabled`, signaux
  `cacheRefreshRequested`, `cacheUpdated`, `strokeFrozen`).
- QML : `objectName` `pageView`, `page` (1024 × 768), `fixedPage`, `pageCache`, `ink` ; types `PageView`,
  `FixedPage`, `InkLayer` ; propriété racine `cacheEnabled`.
- `actcraft::bench` : `Percentiles`, `FrameSeries`, `IntervalClass`, `IntervalStats`, `SyncRecord`,
  `LatencyStats`, `classifyIntervals`, `computeLatency`, `ProcessMemory`, `MemorySummary`, `MemoryTracker`,
  `GpuMemoryReader`, `MachineInfo`, `BenchResult`, `ScenarioResult`, `ContentCheck`, `Criterion`, `toJson`,
  `resultFileName`, `writeResult`, `apiName`, `PlannedInput`, `InputKind`, `ScenarioPlan`, `ScenarioSet`,
  `Injector`, `BenchOptions`, `Budget`, `applyVerdict`, `exitCodeFor`, `verdictLine`, `FrameRecorder`,
  `Runner`.
- JSON `actcraft-bench/1` : schéma figé en T14.

## Règles communes à toutes les tâches (rappelées dans chaque section)

- Toute commande vise le worktree de la tâche : `Set-Location <worktree>;` en tête de chaque commande
  PowerShell, `git -C <worktree>` pour git. `pwsh -NoProfile` toujours.
- Avant la preuve : `New-Item -ItemType Directory -Force .tmp | Out-Null` (le dossier est ignoré par git).
- Clang (CI macOS) active `-Wsign-conversion` par `-Wconversion` : indices de `std::vector` en `std::size_t`,
  de `QList` en `qsizetype`, conversions explicites (`static_cast`). MSVC `/W4 /WX` ne le voit pas : y penser.
- `pwsh -NoProfile -File ./scripts/format.ps1` avant la preuve (le contrôle de format fait partie de
  `TESTS OK`).
- Commit en français, chemins explicites (`git -C <wt> add <fichiers>`), jamais `git add -A`, terminé par
  `Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>`. Pas de push.
- Hors périmètre : consigné dans le rapport, jamais corrigé.

---

## Tâches

### Tâche T1 — Profil (banc = résultat), tests par module, `/Ob2`, Objective-C++

**Objectif.** Avant toute tâche de `render/`, `input/`, `tools/` ou `ui/` : écrire dans le profil que, pendant
ce chantier, le verdict du banc est un résultat et non une porte (décision 38, hypothèse N1-10) ; découper la
déclaration des tests par module pour que deux tâches parallèles ne modifient pas le même fichier ; passer
`/Ob2` en RelWithDebInfo sous MSVC (décision 28) ; activer Objective-C++ sous macOS (mémoire Metal, T14).

**Fichiers.**
- Modifier : `.claude/chantier.md` (section « Banc de fluidité »), `CMakeLists.txt`, `tests/CMakeLists.txt`
- Créer : `tests/verifications.cmake`, `tests/core.cmake`, `tests/render.cmake`, `tests/input.cmake`,
  `tests/tools.cmake`, `tests/ui.cmake`, `tests/bench.cmake`

**Test et commande bornée.** Aucun test nouveau (configuration). Preuve :
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2
Set-Location <worktree>; pwsh -NoProfile -Command ". ./scripts/dev-env.ps1; cmake --preset release *> .tmp/cfg-release.txt"; Select-String -Path build/release/build.ninja -Pattern '/Ob2' -SimpleMatch | Select-Object -First 1
```
Attendu : `100% tests passed, 0 tests failed out of 3` puis `TESTS OK` ; une ligne de `build.ninja` contenant
`/Ob2`.

**Interdits.** Ne toucher à aucune autre section du profil (le bloc `interdits` est lu par le hook
`interdits.js`). Ne pas déplacer les trois tests existants de `tests/CMakeLists.txt`.

**Terminé quand.** Les deux preuves ci-dessus sont vertes ; `Select-String -Path .claude/chantier.md -Pattern
'résultat, pas une porte'` trouve une ligne ; commit fait.

**Étapes.**

- [ ] **Étape 1 : profil.** Dans `.claude/chantier.md`, remplacer la section « Banc de fluidité » entière par :

```markdown
## Banc de fluidité

À partir de l'étape 1 du BRIEF : toute tâche qui touche `render/`, `input/`, `tools/` ou `ui/` passe au banc
(preset `release`) ; un dépassement du budget du BRIEF § 3 (60 images/s, latence d'encre < 20 ms) refuse la
tâche. Commande à écrire avec le banc.

**Pendant le chantier `prototype-mesure`** (décision 38 du cadrage
`docs/specs/2026-10-13-prototype-mesure-cadrage.md`, hypothèse N1-10) : le verdict du banc est un **résultat,
pas une porte**. Les tâches antérieures au banc sont validées par leurs tests seuls ; les tâches postérieures
passent au banc et joignent le JSON, sans refus sur dépassement. Un `BUDGET DEPASSE` produit un rapport chiffré
(critère, valeur, seuil, scénario) et une hypothèse N1 pour Jérôme, sans bloquer le merge. La règle de refus
ci-dessus s'applique après la décision go / no-go de l'étape 2 du BRIEF.
```

- [ ] **Étape 2 : `CMakeLists.txt` racine.** Juste après `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)`, ajouter :

```cmake
# Mesures du banc en RelWithDebInfo : MSVC y met /Ob1 (inlining limité) ; /Ob2 comme en Release
# (décision 28 du cadrage du prototype de mesure). Clang garde ses options.
if(MSVC)
    string(REPLACE "/Ob1" "/Ob2" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
endif()

# Mémoire graphique de Metal (bench/) : Objective-C++.
if(APPLE)
    enable_language(OBJCXX)
endif()
```

- [ ] **Étape 3 : fichiers de tests par module.** À la fin de `tests/CMakeLists.txt`, ajouter :

```cmake
# Un fichier par module : deux tâches parallèles ne modifient pas le même fichier.
# Chaque test y est déclaré par actcraft_add_test (fonction ci-dessus).
include(${CMAKE_CURRENT_SOURCE_DIR}/verifications.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/core.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/render.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/input.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/tools.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/ui.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/bench.cmake)
```

Créer chacun des sept fichiers avec une seule ligne de commentaire, par exemple `tests/core.cmake` :

```cmake
# Tests du module core : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).
```

(`verifications.cmake` : « Vérifications de comportements de Qt 6.12 » ; `render`, `input`, `tools`, `ui` :
même phrase avec le nom du module.) `tests/bench.cmake` contient en plus, après le commentaire :

```cmake
if(NOT ACTCRAFT_BUILD_BENCH)
    return()
endif()
```

- [ ] **Étape 4 : preuve.** Lancer les deux commandes bornées ci-dessus. Attendu : `out of 3`, `TESTS OK`,
une ligne `/Ob2`.

- [ ] **Étape 5 : commit.**

```powershell
Set-Location <worktree>; git add .claude/chantier.md CMakeLists.txt tests/CMakeLists.txt tests/verifications.cmake tests/core.cmake tests/render.cmake tests/input.cmake tests/tools.cmake tests/ui.cmake tests/bench.cmake
git -C <worktree> commit -m "Profil : banc sans refus pendant le prototype ; tests par module ; /Ob2" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

### Tâche T2 — Vérifications Qt 6.12 (nœud logiciel, couche logicielle, tablette)

**Objectif.** Trancher par des tests hors écran trois comportements de Qt 6.12 non vérifiés, avec un repli
écrit pour chacun (décisions 6, 7, 13 ; DA2, DA3) :
1. un `QSGRenderNode` peint au `QPainter` dessine en rendu logiciel (et, à titre d'information, un
   `QSGGeometryNode` n'y dessine pas) ;
2. un `ShaderEffectSource` `live: false` en rendu logiciel garde son image, se redessine sur
   `scheduleUpdate()` et émet `scheduledUpdateCompleted`, avec pour source un `Rectangle` puis un élément C++ à
   `QSGRenderNode` peint au `QPainter` (le cas réel de la page fixe) ;
3. un `QTabletEvent` posté à la fenêtre depuis un périphérique synthétique atteint `QQuickItem::event()` d'un
   élément C++ avec sa pression ; sinon, un filtre d'événements sur la fenêtre le voit (repli).

**Le piège.** Un test qui « passe » sans établir la vérité ne sert à rien. Les points 2 et 3 ne sont **pas**
des assertions : le test imprime le comportement observé (`VERIF <clé>: oui|non`) et n'échoue que si le repli
lui-même ne marche pas. Le point 1 est une assertion : le cadrage s'appuie dessus (DA2).

**Fichiers.**
- Créer : `tests/tst_qt_verifications.cpp`, `docs/verifications/2026-10-13-qt612.md`
- Modifier : `tests/verifications.cmake`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_qt_verifications .*(Passed|Failed)' | Select-Object -Last 3
Set-Location <worktree>; pwsh -NoProfile -Command ". ./scripts/dev-env.ps1; `$env:QT_QPA_PLATFORM='offscreen'; `$env:QT_QUICK_BACKEND='software'; ./build/debug/tst_qt_verifications.exe *> .tmp/verif.txt"; Select-String -Path .tmp/verif.txt -Pattern 'VERIF '
```
Attendu : `out of N` avec N ≥ 4, `tst_qt_verifications ... Passed`, `TESTS OK` ; cinq lignes `VERIF`.

**Interdits.** Ne pas affaiblir l'assertion du point 1 pour la faire passer. Ne pas écrire de code de
production (`render/`, `ui/`) : cette tâche ne fait que constater.

**Terminé quand.** Preuves vertes ; `docs/verifications/2026-10-13-qt612.md` contient les cinq lignes
`clé: oui|non` recopiées de la sortie réelle. **Si `render-node-logiciel: non`** (assertion en échec), ne pas
contourner : le rapport commence par `REPLI RENDERNODE` (repli écrit dans le document : chaque moteur peint son
objet dans une `QImage` affichée par un `QSGImageNode`) et l'orchestrateur ouvre une porte immédiate, T4 et T10
étant à replanifier.

**Étapes.**

- [ ] **Étape 1 : écrire le test.** Créer `tests/tst_qt_verifications.cpp` :

```cpp
#include <QPainter>
#include <QPointingDevice>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGRenderNode>
#include <QSGRendererInterface>
#include <QSignalSpy>
#include <QTabletEvent>
#include <QTest>
#include <QtQml/qqml.h>

#include <memory>

// Vérifications de comportements de Qt 6.12 dont dépend le prototype de mesure (cadrage, décisions 6, 7, 13).
// Les lignes « VERIF <clé>: oui|non » sont recopiées dans docs/verifications/2026-10-13-qt612.md.

namespace {

class PainterNode : public QSGRenderNode
{
public:
    PainterNode(QQuickWindow* window, QRectF rect, QColor color)
        : m_window(window), m_rect(rect), m_color(color)
    {
    }
    void render(const RenderState* state) override
    {
        auto* painter = static_cast<QPainter*>(
            m_window->rendererInterface()->getResource(m_window, QSGRendererInterface::PainterResource));
        if (!painter) {
            return;
        }
        // La région de découpe est en coordonnées du périphérique : la poser avant la transformation.
        if (const QRegion* clip = state->clipRegion(); clip && !clip->isEmpty()) {
            painter->setClipRegion(*clip, Qt::ReplaceClip);
        }
        painter->setTransform(matrix()->toTransform());
        painter->setOpacity(inheritedOpacity());
        painter->fillRect(m_rect, m_color);
    }
    StateFlags changedStates() const override { return {}; }
    RenderingFlags flags() const override { return BoundedRectRendering; }
    QRectF rect() const override { return m_rect; }
    void setColor(QColor color) { m_color = color; }

private:
    QQuickWindow* m_window;
    QRectF m_rect;
    QColor m_color;
};

class PainterItem : public QQuickItem
{
public:
    PainterItem() { setFlag(ItemHasContents); }

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData*) override
    {
        if (old) {
            return old;
        }
        return new PainterNode(window(), QRectF(0, 0, width(), height()), QColor(Qt::red));
    }
};

class GeometryItem : public QQuickItem
{
public:
    GeometryItem() { setFlag(ItemHasContents); }

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData*) override
    {
        if (old) {
            return old;
        }
        auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
        geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);
        auto* v = geometry->vertexDataAsPoint2D();
        const auto w = static_cast<float>(width());
        const auto h = static_cast<float>(height());
        v[0].set(0, 0);
        v[1].set(w, 0);
        v[2].set(0, h);
        v[3].set(w, h);
        auto* material = new QSGFlatColorMaterial;
        material->setColor(QColor(Qt::green));
        auto* node = new QSGGeometryNode;
        node->setGeometry(geometry);
        node->setMaterial(material);
        node->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
        return node;
    }
};

class TabletProbeItem : public QQuickItem
{
public:
    TabletProbeItem() { setAcceptedMouseButtons(Qt::LeftButton); }
    int tabletEvents = 0;
    qreal firstPressure = -1;

protected:
    bool event(QEvent* e) override
    {
        switch (e->type()) {
        case QEvent::TabletPress:
        case QEvent::TabletMove:
        case QEvent::TabletRelease: {
            auto* tablet = static_cast<QTabletEvent*>(e);
            if (tabletEvents == 0) {
                firstPressure = tablet->pressure();
            }
            ++tabletEvents;
            tablet->accept();
            return true;
        }
        default:
            break;
        }
        return QQuickItem::event(e);
    }
    void mousePressEvent(QMouseEvent* e) override { e->accept(); }
};

class TabletFilter : public QObject
{
public:
    int tabletEvents = 0;
    qreal firstPressure = -1;

protected:
    bool eventFilter(QObject* watched, QEvent* e) override
    {
        if (e->type() == QEvent::TabletPress || e->type() == QEvent::TabletMove ||
            e->type() == QEvent::TabletRelease) {
            if (tabletEvents == 0) {
                firstPressure = static_cast<QTabletEvent*>(e)->pressure();
            }
            ++tabletEvents;
        }
        return QObject::eventFilter(watched, e);
    }
};

bool sameColor(QColor a, QColor b)
{
    return qAbs(a.red() - b.red()) <= 3 && qAbs(a.green() - b.green()) <= 3 && qAbs(a.blue() - b.blue()) <= 3;
}

} // namespace

// Source C++ à QSGRenderNode, comme la page fixe en rendu logiciel (T6), pour vérifier la couche sur ce cas.
class ColorPainterItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor)

public:
    ColorPainterItem() { setFlag(ItemHasContents); }
    QColor color() const { return m_color; }
    void setColor(QColor color)
    {
        m_color = color;
        update();
    }

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData*) override
    {
        auto* node = old ? static_cast<PainterNode*>(old)
                         : new PainterNode(window(), QRectF(0, 0, width(), height()), m_color);
        node->setColor(m_color);
        node->markDirty(QSGNode::DirtyMaterial);
        return node;
    }

private:
    QColor m_color = Qt::red;
};

// Vrai si un ShaderEffectSource `live: false` garde son image, se redessine sur scheduleUpdate() et émet
// scheduledUpdateCompleted, avec pour source l'élément décrit par `source` (id src, propriété color).
static bool staticLayerWorks(const QByteArray& source, QString& details)
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nimport Verif\nItem {\n    width: 200; height: 200\n    " + source +
                          "\n    ShaderEffectSource {\n        objectName: \"cache\"; width: 100; height: 100\n"
                          "        sourceItem: src; live: false; hideSource: true; textureSize: Qt.size(200, 200)\n"
                          "    }\n}\n",
                      QUrl());
    std::unique_ptr<QObject> root(component.create());
    if (!root) {
        details = component.errorString();
        return false;
    }
    QQuickWindow window;
    window.setColor(Qt::white);
    window.resize(200, 200);
    auto* rootItem = qobject_cast<QQuickItem*>(root.get());
    rootItem->setParentItem(window.contentItem());
    window.show();
    if (!QTest::qWaitForWindowExposed(&window)) {
        details = QStringLiteral("fenêtre non exposée");
        return false;
    }
    const bool firstRed = sameColor(window.grabWindow().pixelColor(50, 50), QColor(Qt::red));
    auto* src = rootItem->findChild<QObject*>(QStringLiteral("src"));
    auto* cache = rootItem->findChild<QObject*>(QStringLiteral("cache"));
    if (!src || !cache) {
        details = QStringLiteral("src ou cache introuvable");
        return false;
    }
    src->setProperty("color", QColor(Qt::green));
    QTest::qWait(100);
    const bool stillRed = sameColor(window.grabWindow().pixelColor(50, 50), QColor(Qt::red));
    QSignalSpy completed(cache, SIGNAL(scheduledUpdateCompleted()));
    QMetaObject::invokeMethod(cache, "scheduleUpdate");
    const bool signalled = completed.isValid() && completed.wait(2000);
    const bool nowGreen = sameColor(window.grabWindow().pixelColor(50, 50), QColor(Qt::green));
    details = QStringLiteral("premier=%1 fige=%2 signal=%3 redessin=%4")
                  .arg(firstRed)
                  .arg(stillRed)
                  .arg(signalled)
                  .arg(nowGreen);
    return firstRed && stillRed && signalled && nowGreen;
}

class TstQtVerifications : public QObject
{
    Q_OBJECT

private slots:
    void renderNodePaintsInSoftware()
    {
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(200, 200);
        auto* item = new PainterItem;
        item->setSize(QSizeF(100, 100));
        item->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QCOMPARE(window.rendererInterface()->graphicsApi(), QSGRendererInterface::Software);
        const QImage image = window.grabWindow();
        const bool painted = sameColor(image.pixelColor(50, 50), QColor(Qt::red));
        qInfo("VERIF render-node-logiciel: %s", painted ? "oui" : "non");
        QVERIFY(painted);
        QVERIFY(sameColor(image.pixelColor(150, 150), QColor(Qt::white)));
    }

    void geometryNodeInSoftware()
    {
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(200, 200);
        auto* item = new GeometryItem;
        item->setSize(QSizeF(100, 100));
        item->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const QImage image = window.grabWindow();
        const bool drawn = sameColor(image.pixelColor(50, 50), QColor(Qt::green));
        qInfo("VERIF geometrie-logiciel: %s", drawn ? "oui" : "non");
    }

    void initTestCase() { qmlRegisterType<ColorPainterItem>("Verif", 1, 0, "ColorPainter"); }

    void staticLayerInSoftware()
    {
        QString details;
        const bool works = staticLayerWorks(
            "Rectangle { id: src; objectName: \"src\"; width: 100; height: 100; color: \"red\" }", details);
        qInfo("VERIF couche-logicielle: %s (%s)", works ? "oui" : "non", qPrintable(details));
    }

    void staticLayerOverRenderNodeInSoftware()
    {
        QString details;
        const bool works = staticLayerWorks(
            "ColorPainter { id: src; objectName: \"src\"; width: 100; height: 100; color: \"red\" }", details);
        qInfo("VERIF couche-rendernode-logiciel: %s (%s)", works ? "oui" : "non", qPrintable(details));
    }

    void tabletEventReachesQuickItem()
    {
        QQuickWindow window;
        window.resize(200, 200);
        auto* item = new TabletProbeItem;
        item->setSize(QSizeF(200, 200));
        item->setParentItem(window.contentItem());
        TabletFilter filter;
        window.installEventFilter(&filter);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        QPointingDevice stylus(QStringLiteral("verif-stylus"), 4242, QInputDevice::DeviceType::Stylus,
                               QPointingDevice::PointerType::Pen,
                               QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
        const auto post = [&](QEvent::Type type, QPointF pos, qreal pressure, Qt::MouseButton button,
                              Qt::MouseButtons buttons) {
            QCoreApplication::postEvent(&window, new QTabletEvent(type, &stylus, pos, window.mapToGlobal(pos),
                                                                  pressure, 0, 0, 0, 0, 0, Qt::NoModifier,
                                                                  button, buttons));
        };
        post(QEvent::TabletPress, QPointF(50, 50), 0.7, Qt::LeftButton, Qt::LeftButton);
        post(QEvent::TabletMove, QPointF(60, 60), 0.4, Qt::NoButton, Qt::LeftButton);
        post(QEvent::TabletRelease, QPointF(60, 60), 0.0, Qt::LeftButton, Qt::NoButton);
        QTRY_COMPARE_WITH_TIMEOUT(filter.tabletEvents, 3, 2000);
        QVERIFY(qAbs(filter.firstPressure - 0.7) < 0.01);

        const bool delivered = item->tabletEvents == 3 && qAbs(item->firstPressure - 0.7) < 0.01;
        qInfo("VERIF tablette-element: %s (recus=%d pression=%.2f)", delivered ? "oui" : "non",
              item->tabletEvents, item->firstPressure);
    }
};

QTEST_MAIN(TstQtVerifications)
#include "tst_qt_verifications.moc"
```

- [ ] **Étape 2 : déclarer le test.** Dans `tests/verifications.cmake`, sous le commentaire :

```cmake
actcraft_add_test(tst_qt_verifications
    SOURCES tst_qt_verifications.cpp
    LIBS Qt6::Quick Qt6::Qml)
```

- [ ] **Étape 3 : lancer.** Commande bornée. Attendu : `tst_qt_verifications ... Passed`, `TESTS OK`. Si la
compilation échoue sur une signature de Qt 6.12 (constructeur de `QTabletEvent`, de `QPointingDevice`), lire
l'en-tête installé (`C:\Qt\6.12.0\msvc2022_64\include\QtGui\qevent.h`) et corriger l'appel sans changer ce qui
est vérifié. Puis la seconde commande : relever les cinq lignes `VERIF`.

- [ ] **Étape 4 : document.** Créer `docs/verifications/2026-10-13-qt612.md` ; remplacer chaque `<oui|non>` par
la valeur lue à l'étape 3 :

~~~markdown
# Vérifications de Qt 6.12 — prototype de mesure

Faites le 2026-10-13 par `tests/tst_qt_verifications.cpp` (hors écran, `QT_QUICK_BACKEND=software`). Lignes lues
par les tâches suivantes du plan `docs/plans/2026-10-13-prototype-mesure-PLAN.md` (T12 surtout).

render-node-logiciel: <oui|non>
geometrie-logiciel: <oui|non>
couche-logicielle: <oui|non>
couche-rendernode-logiciel: <oui|non>
tablette-element: <oui|non>

| Clé | Ce qui est vérifié | Si « oui » | Repli si « non » |
|---|---|---|---|
| `render-node-logiciel` | un `QSGRenderNode` peint au `QPainter` dessine en logiciel | chemin logiciel des moteurs et de l'encre (DA2) | chaque moteur peint son objet dans une `QImage` affichée par un `QSGImageNode` ; T4 et T10 à replanifier (porte immédiate) |
| `geometrie-logiciel` | un `QSGGeometryNode` dessine en logiciel | information : le chemin `QPainter` resterait utile pour l'antialiasing | attendu : le chemin `QPainter` est obligatoire en logiciel |
| `couche-logicielle` | `ShaderEffectSource` `live: false` : image figée, `scheduleUpdate()` redessine, `scheduledUpdateCompleted` émis | cache actif dans les deux rendus | cache désactivé en rendu logiciel (`Main.qml`, variante B de T12) ; `cache` vaut `false` dans le JSON logiciel |
| `couche-rendernode-logiciel` | même vérification, avec pour source un élément C++ à `QSGRenderNode` peint au `QPainter` (cas de la page fixe en logiciel) | cache actif en logiciel | même repli que `couche-logicielle` (variante B de T12) |
| `tablette-element` | un `QTabletEvent` posté atteint `QQuickItem::event()` avec sa pression | `PageView::event()` reçoit le stylet (variante A de T12) | filtre d'événements sur la fenêtre (variante B de T12), vérifié par le même test |

Sortie brute :

```text
<coller ici les cinq lignes VERIF de .tmp/verif.txt>
```
~~~

- [ ] **Étape 5 : format, preuve, commit.**

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add tests/verifications.cmake tests/tst_qt_verifications.cpp docs/verifications/2026-10-13-qt612.md
git -C <worktree> commit -m "Vérifications de Qt 6.12 : nœud logiciel, couche logicielle, tablette" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```
Le rapport donne les cinq valeurs `VERIF` dans ses premières lignes.

---

### Tâche T3 — `core` : modèle, empreinte, générateur 300 objets, sonde, horloge

**Objectif.** Modèle minimal de page dans `core/` (décision 2), générateur déterministe des 300 objets avec trois
témoins (décision 3, N1-5), empreinte FNV-1a 64 bits identique sous MSVC et Clang, interface `InkProbe` (DA4) et
horloge monotone unique.

**Le piège.** Le patron évident — `std::uniform_int_distribution`, `std::uniform_real_distribution`, `sin`/`cos`,
`std::hash` — donne des résultats différents entre MSVC et la bibliothèque de Clang, ou entre x64 et arm64
(contraction FMA). Ici : `std::mt19937` (sa suite est fixée par la norme), tirage par rejet écrit par nous,
**entiers seulement** (coordonnées en 1/16 de pixel), conversion en réels à la fin par division exacte, empreinte
sur des octets dans un ordre fixé. Aucun `long`, aucun `std::hash`.

**Fichiers.**
- Créer : `core/include/actcraft/core/model.h`, `core/include/actcraft/core/generator.h`,
  `core/include/actcraft/core/clock.h`, `core/include/actcraft/core/inkprobe.h`, `core/src/model.cpp`,
  `core/src/generator.cpp`, `tests/tst_core_model.cpp`, `tests/tst_core_generator.cpp`
- Modifier : `core/CMakeLists.txt`, `tests/core.cmake`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_core_(model|generator) .*(Passed|Failed)|fingerprint = 0x' | Select-Object -Last 5
```
Attendu : `out of N` avec N ≥ 5, `tst_core_model ... Passed`, `tst_core_generator ... Passed`, `TESTS OK`.

**Interdits.** Ni QtGui ni QtQuick dans `core/` (CMake échoue sinon). Aucune distribution de la bibliothèque
standard, aucun `sin`/`cos`/`sqrt` dans le générateur. Ne pas « corriger » l'empreinte attendue après l'avoir
figée : si elle change, c'est le générateur qui a changé.

**Terminé quand.** Preuve verte ; `kExpectedFingerprint` vaut la valeur imprimée par le premier passage (étape 7) ;
commit fait. La CI macOS de la PR vérifiera la même valeur (S4).

**Étapes.**

- [ ] **Étape 1 : tests du modèle (rouges).** Créer `tests/tst_core_model.cpp` :

```cpp
#include <actcraft/core/clock.h>
#include <actcraft/core/model.h>

#include <QTest>

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

private slots:
    void kindOfFollowsVariantOrder()
    {
        const Page page = samplePage();
        QCOMPARE(kindOf(page.objects[0]), ObjectKind::Shape);
        QCOMPARE(kindOf(page.objects[1]), ObjectKind::Image);
        QCOMPARE(kindOf(page.objects[2]), ObjectKind::Stroke);
    }

    void defaultPageIs1024By768()
    {
        const Page page;
        QCOMPARE(page.width, 1024.0);
        QCOMPARE(page.height, 768.0);
        QVERIFY(page.objects.empty());
    }

    void fingerprintIsStable()
    {
        QCOMPARE(fingerprint(samplePage()), fingerprint(samplePage()));
    }

    void fingerprintSeesEveryField()
    {
        const std::uint64_t reference = fingerprint(samplePage());
        Page a = samplePage();
        std::get<Stroke>(a.objects[2]).points[1].pressure = 0.5001f;
        QVERIFY(fingerprint(a) != reference);
        Page b = samplePage();
        std::get<Shape>(b.objects[0]).polygon[2].y = 61;
        QVERIFY(fingerprint(b) != reference);
        Page c = samplePage();
        std::get<ImageObject>(c.objects[1]).pattern = ImagePattern::Rings;
        QVERIFY(fingerprint(c) != reference);
        Page d = samplePage();
        std::swap(d.objects[0], d.objects[2]);
        QVERIFY(fingerprint(d) != reference);
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

- [ ] **Étape 2 : tests du générateur (rouges).** Créer `tests/tst_core_generator.cpp` :

```cpp
#include <actcraft/core/generator.h>

#include <QTest>

using namespace actcraft::core;

namespace {
// Valeur relevée au premier passage sous Windows (étape 7), vérifiée sous macOS par la CI.
constexpr std::uint64_t kExpectedFingerprint = 0x0ull;

bool nearBlack(Argb c)
{
    const auto r = (c >> 16) & 0xFFu;
    const auto g = (c >> 8) & 0xFFu;
    const auto b = c & 0xFFu;
    return r < 0x40u && g < 0x40u && b < 0x40u;
}
} // namespace

class TstCoreGenerator : public QObject
{
    Q_OBJECT

private slots:
    void countsPerKind()
    {
        const Page page = generatePage();
        QCOMPARE(page.objects.size(), std::size_t{300});
        int shapes = 0;
        int images = 0;
        int strokes = 0;
        for (const PageObject& o : page.objects) {
            switch (kindOf(o)) {
            case ObjectKind::Shape:
                ++shapes;
                break;
            case ObjectKind::Image:
                ++images;
                break;
            case ObjectKind::Stroke:
                ++strokes;
                break;
            }
        }
        QCOMPARE(shapes, kShapeCount);
        QCOMPARE(images, kImageCount);
        QCOMPARE(strokes, kStrokeCount);
    }

    void deterministic()
    {
        QCOMPARE(fingerprint(generatePage()), fingerprint(generatePage()));
        QVERIFY(fingerprint(generatePage(kDefaultSeed)) != fingerprint(generatePage(kDefaultSeed + 1)));
    }

    void fingerprintIsPinned()
    {
        const std::uint64_t actual = fingerprint(generatePage());
        if (actual != kExpectedFingerprint) {
            qWarning("fingerprint = 0x%016llx", static_cast<unsigned long long>(actual));
        }
        QCOMPARE(actual, kExpectedFingerprint);
    }

    void witnessesAtTop()
    {
        const Page page = generatePage();
        const Witnesses& w = witnesses();
        const std::size_t n = page.objects.size();
        const auto& shape = std::get<Shape>(page.objects[n - 3]);
        QCOMPARE(shape.bounds.x, w.shapeRect.x);
        QCOMPARE(shape.fill, w.shapeColor);
        QCOMPARE(shape.opacity, 1.0);
        const auto& image = std::get<ImageObject>(page.objects[n - 2]);
        QCOMPARE(image.pattern, ImagePattern::SolidCenter);
        QCOMPARE(image.colorA, w.imageCenterColor);
        QCOMPARE(image.bounds.x, w.imageRect.x);
        const auto& stroke = std::get<Stroke>(page.objects[n - 1]);
        QCOMPARE(stroke.color, w.strokeColor);
        QVERIFY(w.strokeRect.contains(w.strokeProbe));
        QVERIFY(stroke.baseWidth >= 12.0);
    }

    void semiTransparentShare()
    {
        const Page page = generatePage();
        int translucent = 0;
        for (const PageObject& o : page.objects) {
            if (const auto* s = std::get_if<Shape>(&o); s && s->opacity < 1.0) {
                ++translucent;
            }
        }
        QCOMPARE(translucent, 30); // 20 % de 150
    }

    void generatedStrokesAvoidWitnesses()
    {
        const Page page = generatePage();
        for (std::size_t i = 0; i + 3 < page.objects.size(); ++i) {
            if (const auto* s = std::get_if<Stroke>(&page.objects[i])) {
                for (const StrokePoint& p : s->points) {
                    QVERIFY2(!hitsWitness({p.x, p.y}, 8.0), qPrintable(QString::number(i)));
                }
            }
        }
    }

    void rangesRespected()
    {
        const Page page = generatePage();
        for (std::size_t i = 0; i + 3 < page.objects.size(); ++i) {
            const PageObject& o = page.objects[i];
            if (const auto* s = std::get_if<Stroke>(&o)) {
                QVERIFY(s->points.size() >= 50 && s->points.size() <= 300);
                QVERIFY(!nearBlack(s->color));
                for (const StrokePoint& p : s->points) {
                    QVERIFY(p.x >= 0 && p.x <= 1024 && p.y >= 0 && p.y <= 768);
                    QVERIFY(p.pressure >= 0.25f && p.pressure < 1.0f);
                }
            } else if (const auto* im = std::get_if<ImageObject>(&o)) {
                QVERIFY(im->sourceWidth >= 256 && im->sourceWidth <= 1024);
                QVERIFY(im->sourceHeight >= 256 && im->sourceHeight <= 1024);
                QVERIFY(im->pattern != ImagePattern::SolidCenter);
                QVERIFY(im->bounds.x >= 0 && im->bounds.x + im->bounds.width <= 1024);
                QVERIFY(im->bounds.y >= 0 && im->bounds.y + im->bounds.height <= 768);
            } else if (const auto* sh = std::get_if<Shape>(&o)) {
                QVERIFY(!nearBlack(sh->fill));
                QVERIFY(sh->bounds.x >= 0 && sh->bounds.x + sh->bounds.width <= 1024);
                QVERIFY(sh->bounds.y >= 0 && sh->bounds.y + sh->bounds.height <= 768);
                if (sh->kind == ShapeKind::Polygon) {
                    QVERIFY(sh->polygon.size() >= 3 && sh->polygon.size() <= 8);
                }
            }
        }
    }
};

QTEST_GUILESS_MAIN(TstCoreGenerator)
#include "tst_core_generator.moc"
```

- [ ] **Étape 3 : déclarer les tests, constater le rouge.** Dans `tests/core.cmake` :

```cmake
actcraft_add_test(tst_core_model
    SOURCES tst_core_model.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_generator
    SOURCES tst_core_generator.cpp
    LIBS ActCraft::core)
```
Commande bornée. Attendu : `TESTS EN ECHEC (compilation)` (en-têtes `actcraft/core/model.h` absents).

- [ ] **Étape 4 : en-têtes.** Créer `core/include/actcraft/core/model.h` :

```cpp
#pragma once

#include <cstdint>
#include <variant>
#include <vector>

// Modèle minimal d'une page (décision 2 du cadrage du prototype de mesure). Sans QtGui : couleur en entier.

namespace actcraft::core {

// Couleur ARGB 32 bits (0xAARRGGBB), même codage que QRgb.
using Argb = std::uint32_t;

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

// L'ordre suit celui de PageObject : kindOf() en dépend.
enum class ObjectKind : std::uint8_t
{
    Shape = 0,
    Image = 1,
    Stroke = 2,
};

enum class ShapeKind : std::uint8_t
{
    Rectangle = 0,
    Ellipse = 1,
    Polygon = 2,
};

struct Shape
{
    ShapeKind kind = ShapeKind::Rectangle;
    RectF bounds;
    std::vector<PointF> polygon; // sommets en coordonnées de page (Polygon seulement)
    Argb fill = 0xFFFFFFFFu;
    Argb outline = 0xFF000000u;
    double outlineWidth = 1; // 0 : pas de contour
    double opacity = 1;
};

enum class ImagePattern : std::uint8_t
{
    LinearGradient = 0,
    Checker = 1,
    Rings = 2,
    SolidCenter = 3, // moitié centrale unie (colorA), bord colorB : objet témoin
};

// Image procédurale : seuls ses paramètres sont dans le modèle (décision 4).
struct ImageObject
{
    RectF bounds;
    std::int32_t sourceWidth = 256;
    std::int32_t sourceHeight = 256;
    ImagePattern pattern = ImagePattern::LinearGradient;
    Argb colorA = 0xFF000000u;
    Argb colorB = 0xFFFFFFFFu;
    std::uint32_t seed = 0;
};

struct StrokePoint
{
    double x = 0;
    double y = 0;
    float pressure = 0.5f;
};

struct Stroke
{
    std::vector<StrokePoint> points;
    double baseWidth = 3;
    Argb color = 0xFF000000u;
};

// Largeur rendue d'un point : base × (0,5 + pression) (hypothèse N2-2).
inline double strokeWidthAt(double baseWidth, float pressure) noexcept
{
    return baseWidth * (0.5 + static_cast<double>(pressure));
}

using PageObject = std::variant<Shape, ImageObject, Stroke>;

ObjectKind kindOf(const PageObject& object) noexcept;

// Objets dans l'ordre d'empilement : le dernier est au premier plan.
struct Page
{
    double width = 1024;
    double height = 768;
    std::vector<PageObject> objects;
};

// Empreinte FNV-1a 64 bits du contenu ; identique sur toutes les plateformes
// (types à taille fixe, octets dans un ordre fixé, réels par leur motif binaire IEEE 754).
std::uint64_t fingerprint(const Page& page) noexcept;

} // namespace actcraft::core
```

Créer `core/include/actcraft/core/clock.h` :

```cpp
#pragma once

#include <chrono>
#include <cstdint>

namespace actcraft::core {

// Horloge monotone unique du prototype (entrée, encre, banc), en nanosecondes.
inline std::int64_t monotonicNowNs() noexcept
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

} // namespace actcraft::core
```

Créer `core/include/actcraft/core/inkprobe.h` :

```cpp
#pragma once

#include <cstdint>

namespace actcraft::core {

// Sonde de latence d'encre (DA4) : implémentée par bench/, appelée par input/ et render/.
// Sans sonde (pointeur nul), coût nul. L'implémentation protège ses données : deux fils l'appellent.
class InkProbe
{
public:
    virtual ~InkProbe() = default;
    // Fil principal : l'échantillon numéro `seq` vient d'entrer dans la chaîne d'entrée.
    virtual void sampleReceived(std::uint64_t seq, std::int64_t nowNs) = 0;
    // Fil de rendu, pendant la synchronisation : les échantillons jusqu'à `lastSeq` sont dans la géométrie
    // de l'image en préparation.
    virtual void samplesSynced(std::uint64_t lastSeq, std::int64_t nowNs) = 0;
};

} // namespace actcraft::core
```

Créer `core/include/actcraft/core/generator.h` :

```cpp
#pragma once

#include <actcraft/core/model.h>

#include <cstdint>

namespace actcraft::core {

inline constexpr std::uint32_t kDefaultSeed = 20261013u;
inline constexpr int kShapeCount = 150;
inline constexpr int kImageCount = 30;
inline constexpr int kStrokeCount = 120;

// Trois objets témoins, de couleurs connues, au premier plan, à des positions connues (décision 3).
// Le banc contrôle leur couleur sur la capture « retour à 100 % » (décision 31).
struct Witnesses
{
    RectF shapeRect;
    Argb shapeColor;
    RectF imageRect;
    Argb imageCenterColor;
    RectF strokeRect;
    PointF strokeProbe;
    Argb strokeColor;
};

const Witnesses& witnesses() noexcept;

// Vrai si `p` tombe dans l'un des rectangles témoins agrandi de `margin` pixels de page.
bool hitsWitness(PointF p, double margin) noexcept;

// 300 objets : 150 formes (30 semi-transparentes), 30 images, 120 traits de 50 à 300 points (N1-5),
// dont les trois témoins, en dernier. Arithmétique entière : même résultat sous MSVC et Clang.
Page generatePage(std::uint32_t seed = kDefaultSeed);

} // namespace actcraft::core
```

- [ ] **Étape 5 : `core/src/model.cpp`.**

```cpp
#include <actcraft/core/model.h>

#include <bit>

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
    void point(const PointF& p) noexcept
    {
        f64(p.x);
        f64(p.y);
    }
    void rect(const RectF& r) noexcept
    {
        f64(r.x);
        f64(r.y);
        f64(r.width);
        f64(r.height);
    }
    std::uint64_t value() const noexcept { return m_hash; }

private:
    std::uint64_t m_hash = 0xCBF29CE484222325ull;
};

} // namespace

ObjectKind kindOf(const PageObject& object) noexcept
{
    return static_cast<ObjectKind>(object.index());
}

std::uint64_t fingerprint(const Page& page) noexcept
{
    Fnv1a h;
    h.f64(page.width);
    h.f64(page.height);
    h.u64(static_cast<std::uint64_t>(page.objects.size()));
    for (const PageObject& object : page.objects) {
        h.byte(static_cast<std::uint8_t>(kindOf(object)));
        if (const auto* s = std::get_if<Shape>(&object)) {
            h.byte(static_cast<std::uint8_t>(s->kind));
            h.rect(s->bounds);
            h.u64(static_cast<std::uint64_t>(s->polygon.size()));
            for (const PointF& p : s->polygon) {
                h.point(p);
            }
            h.u32(s->fill);
            h.u32(s->outline);
            h.f64(s->outlineWidth);
            h.f64(s->opacity);
        } else if (const auto* im = std::get_if<ImageObject>(&object)) {
            h.rect(im->bounds);
            h.i32(im->sourceWidth);
            h.i32(im->sourceHeight);
            h.byte(static_cast<std::uint8_t>(im->pattern));
            h.u32(im->colorA);
            h.u32(im->colorB);
            h.u32(im->seed);
        } else if (const auto* st = std::get_if<Stroke>(&object)) {
            h.u64(static_cast<std::uint64_t>(st->points.size()));
            for (const StrokePoint& p : st->points) {
                h.f64(p.x);
                h.f64(p.y);
                h.f32(p.pressure);
            }
            h.f64(st->baseWidth);
            h.u32(st->color);
        }
    }
    return h.value();
}

} // namespace actcraft::core
```

- [ ] **Étape 6 : `core/src/generator.cpp`.**

```cpp
#include <actcraft/core/generator.h>

#include <array>
#include <random>

namespace actcraft::core {

namespace {

// Coordonnées internes en 1/16 de pixel (décision 3).
constexpr std::int32_t kSub = 16;
constexpr std::int32_t kPageW = 1024 * kSub;
constexpr std::int32_t kPageH = 768 * kSub;
constexpr std::int32_t kEdge = 8 * kSub;          // les traits restent à 8 px du bord
constexpr std::int32_t kWitnessMargin = 16 * kSub; // et à 16 px des témoins

struct IntRect
{
    std::int32_t x;
    std::int32_t y;
    std::int32_t w;
    std::int32_t h;
};

// Témoins, en pixels de page entiers.
constexpr IntRect kShapeWitness{16, 16, 64, 64};
constexpr IntRect kImageWitness{944, 16, 64, 64};
constexpr IntRect kStrokeWitness{16, 708, 64, 24};
constexpr std::array<IntRect, 3> kWitnessRects{kShapeWitness, kImageWitness, kStrokeWitness};

// 16 directions, vecteur unitaire × 16, arrondi (aucun sin/cos à l'exécution).
constexpr std::array<std::array<std::int32_t, 2>, 16> kDirections{{
    {16, 0}, {15, 6}, {11, 11}, {6, 15}, {0, 16}, {-6, 15}, {-11, 11}, {-15, 6},
    {-16, 0}, {-15, -6}, {-11, -11}, {-6, -15}, {0, -16}, {6, -15}, {11, -11}, {15, -6},
}};

class Rng
{
public:
    explicit Rng(std::uint32_t seed) : m_engine(seed) {}

    std::uint32_t next() { return static_cast<std::uint32_t>(m_engine() & 0xFFFFFFFFu); }

    // Entier uniforme dans [0, n) par rejet : std::uniform_int_distribution n'est pas portable.
    std::uint32_t below(std::uint32_t n)
    {
        const std::uint64_t range = 1ull << 32;
        const std::uint64_t limit = range - range % n;
        std::uint64_t x = 0;
        do {
            x = next();
        } while (x >= limit);
        return static_cast<std::uint32_t>(x % n);
    }

    // Entier uniforme dans [lo, hi] (bornes incluses).
    std::int32_t between(std::int32_t lo, std::int32_t hi)
    {
        return lo + static_cast<std::int32_t>(below(static_cast<std::uint32_t>(hi - lo + 1)));
    }

private:
    std::mt19937 m_engine;
};

double toPage(std::int32_t sub)
{
    return static_cast<double>(sub) / kSub; // division exacte : puissance de deux
}

bool hitsWitnessSub(std::int32_t x, std::int32_t y)
{
    for (const IntRect& r : kWitnessRects) {
        const std::int32_t left = r.x * kSub - kWitnessMargin;
        const std::int32_t top = r.y * kSub - kWitnessMargin;
        const std::int32_t right = (r.x + r.w) * kSub + kWitnessMargin;
        const std::int32_t bottom = (r.y + r.h) * kSub + kWitnessMargin;
        if (x >= left && x <= right && y >= top && y <= bottom) {
            return true;
        }
    }
    return false;
}

// Couleur opaque, canaux de 0x40 à 0xEF : jamais proche du noir de l'encre (contrôle de contenu).
Argb randomColor(Rng& rng)
{
    const auto r = static_cast<Argb>(rng.between(0x40, 0xEF));
    const auto g = static_cast<Argb>(rng.between(0x40, 0xEF));
    const auto b = static_cast<Argb>(rng.between(0x40, 0xEF));
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

Shape makeShape(Rng& rng, int index)
{
    Shape shape;
    shape.kind = static_cast<ShapeKind>(rng.below(3));
    const std::int32_t w = rng.between(16, 160) * kSub;
    const std::int32_t h = rng.between(16, 160) * kSub;
    const std::int32_t x = rng.between(0, kPageW - w);
    const std::int32_t y = rng.between(0, kPageH - h);
    shape.bounds = {toPage(x), toPage(y), toPage(w), toPage(h)};
    shape.fill = randomColor(rng);
    shape.outline = randomColor(rng);
    shape.outlineWidth = static_cast<double>(rng.between(1, 4));
    shape.opacity = index % 5 == 0 ? 0.5 : 1.0; // 30 formes sur 150 (N1-5)
    if (shape.kind == ShapeKind::Polygon) {
        const std::int32_t k = rng.between(3, 8);
        const std::int32_t cx = x + w / 2;
        const std::int32_t cy = y + h / 2;
        for (std::int32_t j = 0; j < k; ++j) {
            const auto dir = kDirections[static_cast<std::size_t>((j * 8 / k) * 2)];
            const std::int32_t vx = cx + dir[0] * (w / 2) / 16;
            const std::int32_t vy = cy + dir[1] * (h / 2) / 16;
            shape.polygon.push_back({toPage(vx), toPage(vy)});
        }
    }
    return shape;
}

ImageObject makeImage(Rng& rng)
{
    ImageObject image;
    image.sourceWidth = rng.between(16, 64) * 16;
    image.sourceHeight = rng.between(16, 64) * 16;
    const std::int32_t dw = rng.between(48, 240);
    std::int32_t dh = dw * image.sourceHeight / image.sourceWidth;
    // Proportions gardées, hauteur bornée : une image haute (256 × 1024) donnerait jusqu'à 960 px > 768,
    // et between(0, (768 − dh) × 16) recevrait une borne négative.
    if (dh < 16) {
        dh = 16;
    } else if (dh > 768 - 16) {
        dh = 768 - 16;
    }
    const std::int32_t x = rng.between(0, (1024 - dw) * kSub);
    const std::int32_t y = rng.between(0, (768 - dh) * kSub);
    image.bounds = {toPage(x), toPage(y), static_cast<double>(dw), static_cast<double>(dh)};
    image.pattern = static_cast<ImagePattern>(rng.below(3));
    image.colorA = randomColor(rng);
    image.colorB = randomColor(rng);
    image.seed = rng.next();
    return image;
}

bool validStrokePoint(std::int32_t x, std::int32_t y)
{
    return x >= kEdge && x <= kPageW - kEdge && y >= kEdge && y <= kPageH - kEdge && !hitsWitnessSub(x, y);
}

Stroke makeStroke(Rng& rng)
{
    Stroke stroke;
    const std::int32_t count = rng.between(50, 300);
    stroke.baseWidth = static_cast<double>(rng.between(2, 6));
    stroke.color = randomColor(rng);
    std::int32_t x = 0;
    std::int32_t y = 0;
    do {
        x = rng.between(kEdge, kPageW - kEdge);
        y = rng.between(kEdge, kPageH - kEdge);
    } while (!validStrokePoint(x, y));
    std::int32_t dir = static_cast<std::int32_t>(rng.below(16));
    std::int32_t pressure = rng.between(256, 1023); // en 1/1024
    stroke.points.reserve(static_cast<std::size_t>(count));
    for (std::int32_t i = 0; i < count; ++i) {
        stroke.points.push_back({toPage(x), toPage(y), static_cast<float>(pressure) / 1024.0f});
        dir = (dir + 16 + rng.between(-1, 1)) % 16;
        const std::int32_t length = rng.between(3, 8);
        for (std::int32_t attempt = 0; attempt < 16; ++attempt) {
            const auto d = kDirections[static_cast<std::size_t>((dir + attempt) % 16)];
            const std::int32_t nx = x + d[0] * length;
            const std::int32_t ny = y + d[1] * length;
            if (validStrokePoint(nx, ny)) {
                x = nx;
                y = ny;
                dir = (dir + attempt) % 16;
                break;
            }
        }
        pressure += rng.between(-32, 32);
        if (pressure < 256) {
            pressure = 256;
        } else if (pressure > 1023) {
            pressure = 1023;
        }
    }
    return stroke;
}

RectF toRect(const IntRect& r)
{
    return {static_cast<double>(r.x), static_cast<double>(r.y), static_cast<double>(r.w),
            static_cast<double>(r.h)};
}

void appendWitnesses(Page& page)
{
    const Witnesses& w = witnesses();
    Shape shape;
    shape.kind = ShapeKind::Rectangle;
    shape.bounds = w.shapeRect;
    shape.fill = w.shapeColor;
    shape.outline = w.shapeColor;
    shape.outlineWidth = 0;
    shape.opacity = 1;
    page.objects.emplace_back(shape);

    ImageObject image;
    image.bounds = w.imageRect;
    image.pattern = ImagePattern::SolidCenter;
    image.colorA = w.imageCenterColor;
    image.colorB = 0xFF7E57C2u;
    page.objects.emplace_back(image);

    Stroke stroke;
    stroke.baseWidth = 12;
    stroke.color = w.strokeColor;
    for (int i = 0; i < 5; ++i) {
        stroke.points.push_back({24.0 + 12.0 * i, 720.0, 0.5f});
    }
    page.objects.emplace_back(stroke);
}

} // namespace

const Witnesses& witnesses() noexcept
{
    static const Witnesses value{
        toRect(kShapeWitness), 0xFF1E88E5u, toRect(kImageWitness), 0xFF43A047u,
        toRect(kStrokeWitness), {48.0, 720.0}, 0xFFFB8C00u,
    };
    return value;
}

bool hitsWitness(PointF p, double margin) noexcept
{
    for (const IntRect& r : kWitnessRects) {
        if (p.x >= r.x - margin && p.x <= r.x + r.w + margin && p.y >= r.y - margin &&
            p.y <= r.y + r.h + margin) {
            return true;
        }
    }
    return false;
}

Page generatePage(std::uint32_t seed)
{
    Rng rng(seed);
    Page page;
    page.objects.reserve(300);
    int shapes = kShapeCount - 1;
    int images = kImageCount - 1;
    int strokes = kStrokeCount - 1;
    int shapeIndex = 0;
    while (shapes + images + strokes > 0) {
        const auto pick = static_cast<int>(rng.below(static_cast<std::uint32_t>(shapes + images + strokes)));
        if (pick < shapes) {
            page.objects.emplace_back(makeShape(rng, shapeIndex++));
            --shapes;
        } else if (pick < shapes + images) {
            page.objects.emplace_back(makeImage(rng));
            --images;
        } else {
            page.objects.emplace_back(makeStroke(rng));
            --strokes;
        }
    }
    appendWitnesses(page);
    return page;
}

} // namespace actcraft::core
```

Dans `core/CMakeLists.txt`, la liste des sources devient :

```cmake
actcraft_add_module(core
    SOURCES
        src/core.cpp
        src/model.cpp
        src/generator.cpp
    DEPENDS
        Qt6::Core)
```

- [ ] **Étape 7 : figer l'empreinte.** Commande bornée. Attendu : tout vert sauf `fingerprintIsPinned`, avec une
ligne `fingerprint = 0x…` dans `.tmp/test.txt`. Recopier cette valeur dans `kExpectedFingerprint`
(`0x…ull`), relancer : `TESTS OK`. Relancer une seconde fois sans recompiler le générateur : même valeur
(déterminisme).

- [ ] **Étape 8 : format, preuve, commit.**

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add core/CMakeLists.txt core/include/actcraft/core/model.h core/include/actcraft/core/generator.h core/include/actcraft/core/clock.h core/include/actcraft/core/inkprobe.h core/src/model.cpp core/src/generator.cpp tests/core.cmake tests/tst_core_model.cpp tests/tst_core_generator.cpp
git -C <worktree> commit -m "core : modèle de page, générateur déterministe des 300 objets, sonde d'encre" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```
Le rapport donne la valeur de l'empreinte.

---

### Tâche T4 — `render` : registre, moteurs forme et trait (RHI + logiciel)

**Objectif.** Un moteur de rendu par type d'objet, choisi dans un registre (décision 5) ; chaque moteur a deux
chemins (décision 6, DA2) : nœuds de géométrie pour le RHI, `QSGRenderNode` peint au `QPainter` pour le rendu
logiciel, avec rectangle borné. Cette tâche livre la triangulation, la base `PainterNode`, le registre et les
moteurs « forme » et « trait ». L'image arrive en T5.

**Préalable.** Lire `docs/verifications/2026-10-13-qt612.md` : `render-node-logiciel: oui` est attendu. Si la
ligne dit `non`, ne pas commencer : le rapport dit `BLOQUÉ : render-node-logiciel non` (porte immédiate).

**Fichiers.**
- Créer : `render/include/actcraft/render/strokegeometry.h`, `render/include/actcraft/render/painternode.h`,
  `render/include/actcraft/render/objectrenderer.h`, `render/include/actcraft/render/shaperenderer.h`,
  `render/include/actcraft/render/strokerenderer.h`, `render/src/strokegeometry.cpp`,
  `render/src/painternode.cpp`, `render/src/nodes.h`, `render/src/nodes.cpp`, `render/src/registry.cpp`,
  `render/src/shaperenderer.cpp`, `render/src/strokerenderer.cpp`, `tests/support/renderhost.h`,
  `tests/tst_render_engines.cpp`
- Modifier : `render/CMakeLists.txt`, `tests/render.cmake`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_render_engines .*(Passed|Failed)' | Select-Object -Last 3
```
Attendu : `out of N` avec N ≥ 7, `tst_render_engines ... Passed`, `TESTS OK`.

**Interdits.** Pas d'élément QML par objet. Pas de chemin unique « géométrie partout » : le rendu logiciel ne la
dessine pas. Ne pas modifier `core/`.

**Terminé quand.** Preuve verte ; les tests de pixels (forme opaque, forme semi-transparente, ellipse, polygone,
trait) passent en rendu logiciel hors écran ; commit fait.

**Étapes.**

- [ ] **Étape 1 : hôte de test.** Créer `tests/support/renderhost.h` (réutilisé par T5) :

```cpp
#pragma once

#include <actcraft/render/objectrenderer.h>

#include <QQuickItem>
#include <QQuickWindow>
#include <QSGNode>
#include <QSGRendererInterface>

#include <vector>

// Élément de test : dessine une liste d'objets par le registre de moteurs, comme la page fixe.
class RenderHostItem : public QQuickItem
{
public:
    RenderHostItem() { setFlag(ItemHasContents); }

    std::vector<actcraft::core::PageObject> objects;
    const actcraft::render::ImageStore* images = nullptr;

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData*) override
    {
        QSGNode* root = old ? old : new QSGNode;
        while (QSGNode* child = root->firstChild()) {
            root->removeChildNode(child);
            delete child;
        }
        actcraft::render::RenderContext context;
        context.window = window();
        context.software = window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
        context.images = images;
        for (std::size_t i = 0; i < objects.size(); ++i) {
            if (const auto* renderer = m_registry.find(actcraft::core::kindOf(objects[i]))) {
                if (QSGNode* node = renderer->createNode(objects[i], i, context)) {
                    root->appendChildNode(node);
                }
            }
        }
        return root;
    }

private:
    actcraft::render::RendererRegistry m_registry = actcraft::render::RendererRegistry::withDefaults();
};

inline bool colorNear(QColor actual, QColor expected, int tolerance)
{
    return qAbs(actual.red() - expected.red()) <= tolerance &&
           qAbs(actual.green() - expected.green()) <= tolerance &&
           qAbs(actual.blue() - expected.blue()) <= tolerance;
}
```

- [ ] **Étape 2 : tests (rouges).** Créer `tests/tst_render_engines.cpp` :

```cpp
#include "support/renderhost.h"

#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/strokegeometry.h>

#include <QTest>

#include <algorithm>

using namespace actcraft;

namespace {

QImage renderObjects(std::vector<core::PageObject> objects)
{
    QQuickWindow window;
    window.setColor(Qt::white);
    window.resize(300, 300);
    auto* host = new RenderHostItem;
    host->setSize(QSizeF(300, 300));
    host->objects = std::move(objects);
    host->setParentItem(window.contentItem());
    window.show();
    if (!QTest::qWaitForWindowExposed(&window)) {
        return {};
    }
    return window.grabWindow();
}

core::Shape shape(core::ShapeKind kind, core::RectF bounds, core::Argb fill, double opacity = 1.0)
{
    core::Shape s;
    s.kind = kind;
    s.bounds = bounds;
    s.fill = fill;
    s.outline = fill;
    s.outlineWidth = 0;
    s.opacity = opacity;
    return s;
}

} // namespace

class TstRenderEngines : public QObject
{
    Q_OBJECT

private slots:
    void registryHasShapeAndStroke()
    {
        const auto registry = render::RendererRegistry::withDefaults();
        QVERIFY(registry.find(core::ObjectKind::Shape));
        QVERIFY(registry.find(core::ObjectKind::Stroke));
        QCOMPARE(registry.find(core::ObjectKind::Shape)->kind(), core::ObjectKind::Shape);
        QCOMPARE(registry.find(core::ObjectKind::Stroke)->kind(), core::ObjectKind::Stroke);
    }

    void horizontalSegmentIsAQuad()
    {
        const std::vector<core::StrokePoint> points{{0, 0, 0.5f}, {10, 0, 0.5f}};
        std::vector<render::Vertex2D> out;
        QCOMPARE(render::appendStrokeTriangles(points, 4.0, out), render::kVerticesPerSegment);
        QCOMPARE(out.size(), render::kVerticesPerSegment);
        float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
        for (const auto& v : out) {
            minX = std::min(minX, v.x);
            maxX = std::max(maxX, v.x);
            minY = std::min(minY, v.y);
            maxY = std::max(maxY, v.y);
        }
        QCOMPARE(minX, -2.0f); // bout carré : prolongé d'une demi-largeur
        QCOMPARE(maxX, 12.0f);
        QCOMPARE(minY, -2.0f); // largeur 4 × (0,5 + 0,5)
        QCOMPARE(maxY, 2.0f);
    }

    void degenerateSegmentKeepsVertexCount()
    {
        const std::vector<core::StrokePoint> points{{5, 5, 0.5f}, {5, 5, 0.5f}, {6, 5, 0.5f}};
        std::vector<render::Vertex2D> out;
        QCOMPARE(render::appendStrokeTriangles(points, 3.0, out), 2 * render::kVerticesPerSegment);
    }

    void fillAndOutlineCounts()
    {
        const auto square = render::rectanglePolygon({0, 0, 10, 10});
        QCOMPARE(square.size(), std::size_t{4});
        std::vector<render::Vertex2D> fill;
        QCOMPARE(render::appendConvexFill(square, fill), std::size_t{6});
        std::vector<render::Vertex2D> outline;
        QCOMPARE(render::appendOutlineTriangles(square, 2.0, outline), 4 * render::kVerticesPerSegment);
    }

    void ellipseInsideBounds()
    {
        const auto polygon = render::ellipsePolygon({10, 20, 100, 50}, 48);
        QCOMPARE(polygon.size(), std::size_t{48});
        for (const auto& p : polygon) {
            QVERIFY(p.x >= 10 - 1e-9 && p.x <= 110 + 1e-9);
            QVERIFY(p.y >= 20 - 1e-9 && p.y <= 70 + 1e-9);
        }
    }

    void strokeBoundsInflated()
    {
        const std::vector<core::StrokePoint> points{{10, 10, 1.0f}, {20, 30, 0.0f}};
        const core::RectF r = render::strokeBounds(points, 4.0); // demi-largeur max 3
        QCOMPARE(r.x, 7.0);
        QCOMPARE(r.y, 7.0);
        QCOMPARE(r.width, 16.0);
        QCOMPARE(r.height, 26.0);
    }

    void shapesRenderInSoftware()
    {
        core::Shape triangle = shape(core::ShapeKind::Polygon, {20, 120, 80, 80}, 0xFFE53935u);
        triangle.polygon = {{20, 200}, {100, 200}, {60, 120}};
        const QImage image = renderObjects({
            shape(core::ShapeKind::Rectangle, {20, 20, 80, 60}, 0xFF1E88E5u),
            shape(core::ShapeKind::Ellipse, {120, 20, 80, 60}, 0xFF43A047u),
            triangle,
            shape(core::ShapeKind::Rectangle, {120, 120, 80, 80}, 0xFF000000u, 0.5),
        });
        QVERIFY(!image.isNull());
        QVERIFY(colorNear(image.pixelColor(60, 50), QColor(0x1E, 0x88, 0xE5), 3));
        QVERIFY(colorNear(image.pixelColor(160, 50), QColor(0x43, 0xA0, 0x47), 3));
        QVERIFY(colorNear(image.pixelColor(60, 180), QColor(0xE5, 0x39, 0x35), 3));
        QVERIFY(colorNear(image.pixelColor(160, 160), QColor(127, 127, 127), 6)); // 50 % de noir sur blanc
        QVERIFY(colorNear(image.pixelColor(250, 250), QColor(Qt::white), 1));
    }

    void strokeRendersInSoftware()
    {
        core::Stroke stroke;
        stroke.baseWidth = 8;
        stroke.color = 0xFF000000u;
        stroke.points = {{20, 250, 0.5f}, {150, 250, 0.5f}, {280, 250, 0.5f}};
        const QImage image = renderObjects({stroke});
        QVERIFY(colorNear(image.pixelColor(100, 250), QColor(Qt::black), 10));
        QVERIFY(colorNear(image.pixelColor(200, 250), QColor(Qt::black), 10));
        QVERIFY(colorNear(image.pixelColor(100, 270), QColor(Qt::white), 1));
    }
};

QTEST_MAIN(TstRenderEngines)
#include "tst_render_engines.moc"
```

Dans `tests/render.cmake` :

```cmake
actcraft_add_test(tst_render_engines
    SOURCES tst_render_engines.cpp
    LIBS ActCraft::render Qt6::Quick)
```
Commande bornée. Attendu : `TESTS EN ECHEC (compilation)`.

- [ ] **Étape 3 : triangulation.** Créer `render/include/actcraft/render/strokegeometry.h` :

```cpp
#pragma once

#include <actcraft/core/model.h>

#include <cstddef>
#include <span>
#include <vector>

namespace actcraft::render {

// Même disposition que QSGGeometry::Point2D.
struct Vertex2D
{
    float x;
    float y;
};

// Chaque segment donne toujours 6 sommets (2 triangles), même nul : les blocs d'encre ont une taille fixe.
inline constexpr std::size_t kVerticesPerSegment = 6;

// Quadrilatère par segment, largeur strokeWidthAt(baseWidth, pression du premier point), bouts carrés.
// Retourne le nombre de sommets ajoutés.
std::size_t appendStrokeTriangles(std::span<const core::StrokePoint> points, double baseWidth,
                                  std::vector<Vertex2D>& out);
// Contour fermé d'un polygone, largeur fixe.
std::size_t appendOutlineTriangles(std::span<const core::PointF> polygon, double width,
                                   std::vector<Vertex2D>& out);
// Remplissage d'un polygone convexe (éventail).
std::size_t appendConvexFill(std::span<const core::PointF> polygon, std::vector<Vertex2D>& out);

std::vector<core::PointF> rectanglePolygon(const core::RectF& bounds);
std::vector<core::PointF> ellipsePolygon(const core::RectF& bounds, int segments);
// Rectangle englobant, agrandi de la demi-largeur maximale.
core::RectF strokeBounds(std::span<const core::StrokePoint> points, double baseWidth);

} // namespace actcraft::render
```

Créer `render/src/strokegeometry.cpp` :

```cpp
#include <actcraft/render/strokegeometry.h>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace actcraft::render {

namespace {

float f(double v)
{
    return static_cast<float>(v);
}

void appendQuad(core::PointF a, core::PointF b, double width, std::vector<Vertex2D>& out)
{
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double length = std::sqrt(dx * dx + dy * dy);
    if (length < 1e-9) {
        for (std::size_t i = 0; i < kVerticesPerSegment; ++i) {
            out.push_back({f(a.x), f(a.y)});
        }
        return;
    }
    const double half = width / 2;
    const double ux = dx / length;
    const double uy = dy / length;
    const double nx = -uy * half;
    const double ny = ux * half;
    const double ax = a.x - ux * half;
    const double ay = a.y - uy * half;
    const double bx = b.x + ux * half;
    const double by = b.y + uy * half;
    const Vertex2D p0{f(ax + nx), f(ay + ny)};
    const Vertex2D p1{f(ax - nx), f(ay - ny)};
    const Vertex2D p2{f(bx + nx), f(by + ny)};
    const Vertex2D p3{f(bx - nx), f(by - ny)};
    out.insert(out.end(), {p0, p1, p2, p2, p1, p3});
}

} // namespace

std::size_t appendStrokeTriangles(std::span<const core::StrokePoint> points, double baseWidth,
                                  std::vector<Vertex2D>& out)
{
    const std::size_t before = out.size();
    for (std::size_t i = 0; i + 1 < points.size(); ++i) {
        appendQuad({points[i].x, points[i].y}, {points[i + 1].x, points[i + 1].y},
                   core::strokeWidthAt(baseWidth, points[i].pressure), out);
    }
    return out.size() - before;
}

std::size_t appendOutlineTriangles(std::span<const core::PointF> polygon, double width,
                                   std::vector<Vertex2D>& out)
{
    const std::size_t before = out.size();
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        appendQuad(polygon[i], polygon[(i + 1) % polygon.size()], width, out);
    }
    return out.size() - before;
}

std::size_t appendConvexFill(std::span<const core::PointF> polygon, std::vector<Vertex2D>& out)
{
    const std::size_t before = out.size();
    for (std::size_t i = 1; i + 1 < polygon.size(); ++i) {
        out.push_back({f(polygon[0].x), f(polygon[0].y)});
        out.push_back({f(polygon[i].x), f(polygon[i].y)});
        out.push_back({f(polygon[i + 1].x), f(polygon[i + 1].y)});
    }
    return out.size() - before;
}

std::vector<core::PointF> rectanglePolygon(const core::RectF& r)
{
    return {{r.x, r.y}, {r.x + r.width, r.y}, {r.x + r.width, r.y + r.height}, {r.x, r.y + r.height}};
}

std::vector<core::PointF> ellipsePolygon(const core::RectF& r, int segments)
{
    std::vector<core::PointF> out;
    out.reserve(static_cast<std::size_t>(segments));
    const double cx = r.x + r.width / 2;
    const double cy = r.y + r.height / 2;
    for (int i = 0; i < segments; ++i) {
        const double angle = 2 * std::numbers::pi * i / segments;
        out.push_back({cx + r.width / 2 * std::cos(angle), cy + r.height / 2 * std::sin(angle)});
    }
    return out;
}

core::RectF strokeBounds(std::span<const core::StrokePoint> points, double baseWidth)
{
    if (points.empty()) {
        return {};
    }
    double minX = points[0].x;
    double maxX = points[0].x;
    double minY = points[0].y;
    double maxY = points[0].y;
    double half = 0;
    for (const auto& p : points) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
        half = std::max(half, core::strokeWidthAt(baseWidth, p.pressure) / 2);
    }
    return {minX - half, minY - half, maxX - minX + 2 * half, maxY - minY + 2 * half};
}

} // namespace actcraft::render
```

- [ ] **Étape 4 : `PainterNode`.** Créer `render/include/actcraft/render/painternode.h` :

```cpp
#pragma once

#include <actcraft/core/model.h>

#include <QColor>
#include <QRectF>
#include <QSGRenderNode>

#include <span>

class QPainter;
class QQuickWindow;

namespace actcraft::render {

// Nœud du rendu logiciel : un QSGRenderNode peint au QPainter (décision 6), à rectangle borné pour garder les
// mises à jour partielles. Les sous-classes ne font que peindre en coordonnées de l'élément.
class PainterNode : public QSGRenderNode
{
public:
    explicit PainterNode(QQuickWindow* window);

    void render(const RenderState* state) override;
    StateFlags changedStates() const override;
    RenderingFlags flags() const override;
    QRectF rect() const override;
    void setRect(const QRectF& rect);

protected:
    virtual void paint(QPainter& painter) = 0;

private:
    QQuickWindow* m_window;
    QRectF m_rect;
};

// Segments d'un trait au QPainter : même géométrie que appendStrokeTriangles (bouts carrés).
void paintStrokeSegments(QPainter& painter, std::span<const core::StrokePoint> points, double baseWidth,
                         QColor color);

} // namespace actcraft::render
```

Créer `render/src/painternode.cpp` :

```cpp
#include <actcraft/render/painternode.h>

#include <QPainter>
#include <QPen>
#include <QQuickWindow>
#include <QSGRendererInterface>

namespace actcraft::render {

PainterNode::PainterNode(QQuickWindow* window) : m_window(window) {}

void PainterNode::render(const RenderState* state)
{
    auto* painter = static_cast<QPainter*>(
        m_window->rendererInterface()->getResource(m_window, QSGRendererInterface::PainterResource));
    if (!painter) {
        return;
    }
    painter->save();
    // La région de découpe est en coordonnées du périphérique : la poser avant la transformation.
    if (const QRegion* clip = state->clipRegion(); clip && !clip->isEmpty()) {
        painter->setClipRegion(*clip, Qt::ReplaceClip);
    }
    painter->setTransform(matrix()->toTransform());
    painter->setOpacity(inheritedOpacity());
    painter->setRenderHint(QPainter::Antialiasing, true);
    paint(*painter);
    painter->restore();
}

QSGRenderNode::StateFlags PainterNode::changedStates() const
{
    return {};
}

QSGRenderNode::RenderingFlags PainterNode::flags() const
{
    return BoundedRectRendering;
}

QRectF PainterNode::rect() const
{
    return m_rect;
}

void PainterNode::setRect(const QRectF& rect)
{
    m_rect = rect;
}

void paintStrokeSegments(QPainter& painter, std::span<const core::StrokePoint> points, double baseWidth,
                         QColor color)
{
    QPen pen(color);
    pen.setCapStyle(Qt::SquareCap);
    for (std::size_t i = 0; i + 1 < points.size(); ++i) {
        pen.setWidthF(core::strokeWidthAt(baseWidth, points[i].pressure));
        painter.setPen(pen);
        painter.drawLine(QPointF(points[i].x, points[i].y), QPointF(points[i + 1].x, points[i + 1].y));
    }
}

} // namespace actcraft::render
```

- [ ] **Étape 5 : registre et aides de nœuds.** Créer `render/include/actcraft/render/objectrenderer.h` :

```cpp
#pragma once

#include <actcraft/core/model.h>

#include <QColor>

#include <array>
#include <cstddef>
#include <memory>

class QQuickWindow;
class QSGNode;

namespace actcraft::render {

class ImageStore; // T5

struct RenderContext
{
    QQuickWindow* window = nullptr;
    bool software = false; // rendererInterface()->graphicsApi() == Software
    const ImageStore* images = nullptr;
};

QColor toQColor(core::Argb argb, double opacity = 1.0);

// Un moteur de rendu par type d'objet (décision 5).
class ObjectRenderer
{
public:
    virtual ~ObjectRenderer() = default;
    virtual core::ObjectKind kind() const noexcept = 0;
    // Fil de rendu, pendant updatePaintNode. Le nœud appartient à l'appelant ; nullptr si rien à dessiner.
    // `objectIndex` : indice de l'objet dans Page::objects (sert aux images).
    virtual QSGNode* createNode(const core::PageObject& object, std::size_t objectIndex,
                                const RenderContext& context) const = 0;
};

class RendererRegistry
{
public:
    // Remplace le moteur du même type.
    void add(std::unique_ptr<ObjectRenderer> renderer);
    const ObjectRenderer* find(core::ObjectKind kind) const noexcept;
    std::size_t size() const noexcept;
    static RendererRegistry withDefaults();

private:
    std::array<std::unique_ptr<ObjectRenderer>, 3> m_renderers;
};

} // namespace actcraft::render
```

Créer `render/src/nodes.h` (interne au module) :

```cpp
#pragma once

#include <actcraft/render/strokegeometry.h>

#include <QColor>
#include <QSGGeometryNode>

#include <vector>

namespace actcraft::render {

// Nœud de triangles à couleur unie (chemin RHI). Possède sa géométrie et son matériau.
QSGGeometryNode* makeTrianglesNode(const std::vector<Vertex2D>& vertices, QColor color);

} // namespace actcraft::render
```

Créer `render/src/nodes.cpp` :

```cpp
#include "nodes.h"

#include <actcraft/render/objectrenderer.h>

#include <QSGFlatColorMaterial>

#include <cstring>

namespace actcraft::render {

static_assert(sizeof(Vertex2D) == sizeof(QSGGeometry::Point2D));

QColor toQColor(core::Argb argb, double opacity)
{
    QColor color = QColor::fromRgba(argb);
    color.setAlphaF(static_cast<float>(color.alphaF() * opacity));
    return color;
}

QSGGeometryNode* makeTrianglesNode(const std::vector<Vertex2D>& vertices, QColor color)
{
    auto* geometry = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), static_cast<int>(vertices.size()));
    geometry->setDrawingMode(QSGGeometry::DrawTriangles);
    if (!vertices.empty()) {
        std::memcpy(geometry->vertexData(), vertices.data(), vertices.size() * sizeof(Vertex2D));
    }
    auto* material = new QSGFlatColorMaterial;
    material->setColor(color);
    auto* node = new QSGGeometryNode;
    node->setGeometry(geometry);
    node->setMaterial(material);
    node->setFlags(QSGNode::OwnsGeometry | QSGNode::OwnsMaterial);
    return node;
}

} // namespace actcraft::render
```

Créer `render/src/registry.cpp` :

```cpp
#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/shaperenderer.h>
#include <actcraft/render/strokerenderer.h>

namespace actcraft::render {

void RendererRegistry::add(std::unique_ptr<ObjectRenderer> renderer)
{
    const auto index = static_cast<std::size_t>(renderer->kind());
    m_renderers[index] = std::move(renderer);
}

const ObjectRenderer* RendererRegistry::find(core::ObjectKind kind) const noexcept
{
    return m_renderers[static_cast<std::size_t>(kind)].get();
}

std::size_t RendererRegistry::size() const noexcept
{
    std::size_t count = 0;
    for (const auto& r : m_renderers) {
        count += r ? 1u : 0u;
    }
    return count;
}

RendererRegistry RendererRegistry::withDefaults()
{
    RendererRegistry registry;
    registry.add(std::make_unique<ShapeRenderer>());
    registry.add(std::make_unique<StrokeRenderer>());
    return registry;
}

} // namespace actcraft::render
```

- [ ] **Étape 6 : moteurs.** Créer `render/include/actcraft/render/shaperenderer.h` :

```cpp
#pragma once

#include <actcraft/render/objectrenderer.h>

namespace actcraft::render {

class ShapeRenderer final : public ObjectRenderer
{
public:
    core::ObjectKind kind() const noexcept override { return core::ObjectKind::Shape; }
    QSGNode* createNode(const core::PageObject& object, std::size_t objectIndex,
                        const RenderContext& context) const override;
};

} // namespace actcraft::render
```

Créer `render/include/actcraft/render/strokerenderer.h` (même forme, classe `StrokeRenderer`, type
`core::ObjectKind::Stroke`) :

```cpp
#pragma once

#include <actcraft/render/objectrenderer.h>

namespace actcraft::render {

class StrokeRenderer final : public ObjectRenderer
{
public:
    core::ObjectKind kind() const noexcept override { return core::ObjectKind::Stroke; }
    QSGNode* createNode(const core::PageObject& object, std::size_t objectIndex,
                        const RenderContext& context) const override;
};

} // namespace actcraft::render
```

Créer `render/src/shaperenderer.cpp` :

```cpp
#include "nodes.h"

#include <actcraft/render/painternode.h>
#include <actcraft/render/shaperenderer.h>
#include <actcraft/render/strokegeometry.h>

#include <QPainter>
#include <QPen>
#include <QPolygonF>

namespace actcraft::render {

namespace {

std::vector<core::PointF> outlineOf(const core::Shape& shape)
{
    switch (shape.kind) {
    case core::ShapeKind::Rectangle:
        return rectanglePolygon(shape.bounds);
    case core::ShapeKind::Ellipse:
        return ellipsePolygon(shape.bounds, 48);
    case core::ShapeKind::Polygon:
        return shape.polygon;
    }
    return {};
}

class ShapePainterNode final : public PainterNode
{
public:
    ShapePainterNode(QQuickWindow* window, const core::Shape& shape)
        : PainterNode(window), m_shape(shape), m_polygon(outlineOf(shape))
    {
        const double m = shape.outlineWidth;
        setRect(QRectF(shape.bounds.x - m, shape.bounds.y - m, shape.bounds.width + 2 * m,
                       shape.bounds.height + 2 * m));
    }

protected:
    void paint(QPainter& painter) override
    {
        QPolygonF polygon;
        for (const auto& p : m_polygon) {
            polygon << QPointF(p.x, p.y);
        }
        painter.setOpacity(painter.opacity() * m_shape.opacity);
        painter.setBrush(toQColor(m_shape.fill));
        if (m_shape.outlineWidth > 0) {
            QPen pen(toQColor(m_shape.outline));
            pen.setWidthF(m_shape.outlineWidth);
            pen.setJoinStyle(Qt::MiterJoin);
            painter.setPen(pen);
        } else {
            painter.setPen(Qt::NoPen);
        }
        painter.drawPolygon(polygon);
    }

private:
    core::Shape m_shape;
    std::vector<core::PointF> m_polygon;
};

} // namespace

QSGNode* ShapeRenderer::createNode(const core::PageObject& object, std::size_t,
                                   const RenderContext& context) const
{
    const auto* shape = std::get_if<core::Shape>(&object);
    if (!shape) {
        return nullptr;
    }
    if (context.software) {
        return new ShapePainterNode(context.window, *shape);
    }
    const auto polygon = outlineOf(*shape);
    auto* root = new QSGNode;
    std::vector<Vertex2D> vertices;
    appendConvexFill(polygon, vertices);
    root->appendChildNode(makeTrianglesNode(vertices, toQColor(shape->fill, shape->opacity)));
    if (shape->outlineWidth > 0) {
        vertices.clear();
        appendOutlineTriangles(polygon, shape->outlineWidth, vertices);
        root->appendChildNode(makeTrianglesNode(vertices, toQColor(shape->outline, shape->opacity)));
    }
    return root;
}

} // namespace actcraft::render
```

Créer `render/src/strokerenderer.cpp` :

```cpp
#include "nodes.h"

#include <actcraft/render/painternode.h>
#include <actcraft/render/strokegeometry.h>
#include <actcraft/render/strokerenderer.h>

namespace actcraft::render {

namespace {

class StrokePainterNode final : public PainterNode
{
public:
    StrokePainterNode(QQuickWindow* window, const core::Stroke& stroke) : PainterNode(window), m_stroke(stroke)
    {
        const core::RectF r = strokeBounds(stroke.points, stroke.baseWidth);
        setRect(QRectF(r.x, r.y, r.width, r.height));
    }

protected:
    void paint(QPainter& painter) override
    {
        paintStrokeSegments(painter, m_stroke.points, m_stroke.baseWidth, toQColor(m_stroke.color));
    }

private:
    core::Stroke m_stroke;
};

} // namespace

QSGNode* StrokeRenderer::createNode(const core::PageObject& object, std::size_t,
                                    const RenderContext& context) const
{
    const auto* stroke = std::get_if<core::Stroke>(&object);
    if (!stroke || stroke->points.size() < 2) {
        return nullptr;
    }
    if (context.software) {
        return new StrokePainterNode(context.window, *stroke);
    }
    std::vector<Vertex2D> vertices;
    appendStrokeTriangles(stroke->points, stroke->baseWidth, vertices);
    return makeTrianglesNode(vertices, toQColor(stroke->color));
}

} // namespace actcraft::render
```

Dans `render/CMakeLists.txt` :

```cmake
# Un moteur de rendu par type d'objet (nœuds du graphe de scène Qt Quick), choisi dans un registre.
actcraft_add_module(render
    SOURCES
        src/render.cpp
        src/strokegeometry.cpp
        src/painternode.cpp
        src/nodes.cpp
        src/registry.cpp
        src/shaperenderer.cpp
        src/strokerenderer.cpp
    DEPENDS
        ActCraft::core Qt6::Quick)
```

- [ ] **Étape 7 : vert.** Commande bornée. Attendu : `tst_render_engines ... Passed`, `TESTS OK`. Un pixel
d'antialiasing hors tolérance se règle en déplaçant le point lu vers l'intérieur de l'objet, jamais en
élargissant la tolérance au-delà de 10.

- [ ] **Étape 8 : format, preuve, commit.**

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add render/CMakeLists.txt render/include/actcraft/render/strokegeometry.h render/include/actcraft/render/painternode.h render/include/actcraft/render/objectrenderer.h render/include/actcraft/render/shaperenderer.h render/include/actcraft/render/strokerenderer.h render/src/strokegeometry.cpp render/src/painternode.cpp render/src/nodes.h render/src/nodes.cpp render/src/registry.cpp render/src/shaperenderer.cpp render/src/strokerenderer.cpp tests/render.cmake tests/support/renderhost.h tests/tst_render_engines.cpp
git -C <worktree> commit -m "render : registre et moteurs forme et trait, chemins RHI et logiciel" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

### Tâche T5 — `render` : images procédurales sur fil secondaire, moteur image

**Objectif.** Les 30 images sont générées par notre code (dégradé, damier, anneaux, centre uni), jamais lues sur
disque, sur un fil secondaire (`QThreadPool`, règle de fluidité 1, décision 4) ; le moteur « image » les affiche
par un `QSGImageNode` (fourni par Qt dans les deux rendus : `QSGSoftwareImageNode` en logiciel, d'où un seul
chemin pour ce moteur — **écart (e) au cadrage** : décision 6 et DA2 prévoient deux chemins par moteur ; consigné
en fin de plan).

**Fichiers.**
- Créer : `render/include/actcraft/render/imagesynth.h`, `render/include/actcraft/render/imagestore.h`,
  `render/include/actcraft/render/imagerenderer.h`, `render/src/imagesynth.cpp`, `render/src/imagestore.cpp`,
  `render/src/imagerenderer.cpp`, `tests/tst_render_images.cpp`
- Modifier : `render/src/registry.cpp`, `render/CMakeLists.txt`, `tests/render.cmake`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_render_images .*(Passed|Failed)' | Select-Object -Last 3
```
Attendu : `out of N` avec N ≥ 8, `tst_render_images ... Passed`, `TESTS OK`.

**Interdits.** Aucun fichier image dans le dépôt, aucune lecture d'image sur disque. Aucune génération d'image sur
le fil principal ni sur le fil de rendu.

**Terminé quand.** Preuve verte ; le témoin « image » montre sa couleur centrale à l'écran (test de pixels) ;
commit fait.

**Étapes.**

- [ ] **Étape 1 : tests (rouges).** Créer `tests/tst_render_images.cpp` :

```cpp
#include "support/renderhost.h"

#include <actcraft/core/generator.h>
#include <actcraft/render/imagestore.h>
#include <actcraft/render/imagesynth.h>

#include <QSignalSpy>
#include <QTest>
#include <QThread>

using namespace actcraft;

class TstRenderImages : public QObject
{
    Q_OBJECT

private slots:
    void solidCenterIsUniform()
    {
        core::ImageObject object;
        object.sourceWidth = 256;
        object.sourceHeight = 256;
        object.pattern = core::ImagePattern::SolidCenter;
        object.colorA = 0xFF43A047u;
        object.colorB = 0xFF7E57C2u;
        const QImage image = render::synthesizeImage(object);
        QCOMPARE(image.size(), QSize(256, 256));
        QCOMPARE(image.pixel(128, 128), QRgb(0xFF43A047u));
        QCOMPARE(image.pixel(70, 70), QRgb(0xFF43A047u));
        QCOMPARE(image.pixel(2, 2), QRgb(0xFF7E57C2u));
    }

    void everyPatternHasSourceSizeAndBothColors()
    {
        for (auto pattern : {core::ImagePattern::LinearGradient, core::ImagePattern::Checker,
                             core::ImagePattern::Rings}) {
            core::ImageObject object;
            object.sourceWidth = 320;
            object.sourceHeight = 512;
            object.pattern = pattern;
            object.colorA = 0xFFFF0000u;
            object.colorB = 0xFF0000FFu;
            const QImage image = render::synthesizeImage(object);
            QCOMPARE(image.size(), QSize(320, 512));
            bool hasA = false;
            bool hasB = false;
            for (int y = 0; y < image.height(); ++y) {
                for (int x = 0; x < image.width(); ++x) {
                    hasA = hasA || image.pixel(x, y) == 0xFFFF0000u;
                    hasB = hasB || image.pixel(x, y) == 0xFF0000FFu;
                }
            }
            QVERIFY(hasA && hasB);
        }
    }

    void storeGeneratesAllImagesOffThread()
    {
        const core::Page page = core::generatePage();
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(page);
        QVERIFY(!store.isReady()); // generate() rend la main avant la fin de la génération
        QVERIFY(ready.wait(20000));
        QVERIFY(store.isReady());
        QVERIFY(store.generatedOffOwnerThread()); // chaque image a été synthétisée sur un fil secondaire
        QCOMPARE(store.imageCount(), std::size_t{30});
        for (std::size_t i = 0; i < page.objects.size(); ++i) {
            if (const auto* im = std::get_if<core::ImageObject>(&page.objects[i])) {
                const QImage image = store.image(i);
                QCOMPARE(image.size(), QSize(im->sourceWidth, im->sourceHeight));
            } else {
                QVERIFY(store.image(i).isNull());
            }
        }
    }

    void storeWithoutImagesBecomesReady()
    {
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(core::Page{});
        QVERIFY(ready.wait(1000));
        QVERIFY(store.isReady());
    }

    void imageRendersInSoftware()
    {
        core::ImageObject witness;
        witness.bounds = {20, 20, 64, 64};
        witness.pattern = core::ImagePattern::SolidCenter;
        witness.colorA = 0xFF43A047u;
        witness.colorB = 0xFF7E57C2u;
        core::Page page;
        page.objects.emplace_back(witness);
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(page);
        QVERIFY(ready.wait(5000));

        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(200, 200);
        auto* host = new RenderHostItem;
        host->setSize(QSizeF(200, 200));
        host->objects = page.objects;
        host->images = &store;
        host->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const QImage image = window.grabWindow();
        QVERIFY(colorNear(image.pixelColor(52, 52), QColor(0x43, 0xA0, 0x47), 3));
        QVERIFY(colorNear(image.pixelColor(23, 23), QColor(0x7E, 0x57, 0xC2), 3));
        QVERIFY(colorNear(image.pixelColor(150, 150), QColor(Qt::white), 1));
    }
};

QTEST_MAIN(TstRenderImages)
#include "tst_render_images.moc"
```

Dans `tests/render.cmake`, ajouter :

```cmake
actcraft_add_test(tst_render_images
    SOURCES tst_render_images.cpp
    LIBS ActCraft::render Qt6::Quick)
```
Commande bornée. Attendu : `TESTS EN ECHEC (compilation)`.

- [ ] **Étape 2 : synthèse.** Créer `render/include/actcraft/render/imagesynth.h` :

```cpp
#pragma once

#include <actcraft/core/model.h>

#include <QImage>

namespace actcraft::render {

// Image procédurale d'un objet (décision 4) : aucune image lue sur disque. ARGB32 prémultiplié, taille source.
// Appelée sur un fil secondaire.
QImage synthesizeImage(const core::ImageObject& object);

} // namespace actcraft::render
```

Créer `render/src/imagesynth.cpp` :

```cpp
#include <actcraft/render/imagesynth.h>

#include <algorithm>
#include <cmath>

namespace actcraft::render {

namespace {

QRgb mix(QRgb a, QRgb b, int t) // t de 0 à 255
{
    const auto channel = [t](int ca, int cb) { return (ca * (255 - t) + cb * t) / 255; };
    return qRgb(channel(qRed(a), qRed(b)), channel(qGreen(a), qGreen(b)), channel(qBlue(a), qBlue(b)));
}

} // namespace

QImage synthesizeImage(const core::ImageObject& object)
{
    const int w = std::max(1, static_cast<int>(object.sourceWidth));
    const int h = std::max(1, static_cast<int>(object.sourceHeight));
    QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
    const QRgb a = object.colorA | 0xFF000000u;
    const QRgb b = object.colorB | 0xFF000000u;
    const int phase = static_cast<int>(object.seed % 32u);
    for (int y = 0; y < h; ++y) {
        auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < w; ++x) {
            QRgb c = a;
            switch (object.pattern) {
            case core::ImagePattern::LinearGradient:
                c = mix(a, b, (x + y) * 255 / std::max(1, w + h - 2));
                break;
            case core::ImagePattern::Checker:
                c = (((x + phase) / 32 + (y + phase) / 32) % 2 == 0) ? a : b;
                break;
            case core::ImagePattern::Rings: {
                const int dx = x - w / 2;
                const int dy = y - h / 2;
                const int ring = static_cast<int>(std::sqrt(static_cast<double>(dx * dx + dy * dy))) / 24;
                c = ring % 2 == 0 ? a : b;
                break;
            }
            case core::ImagePattern::SolidCenter:
                c = (x >= w / 4 && x < w - w / 4 && y >= h / 4 && y < h - h / 4) ? a : b;
                break;
            }
            line[x] = c;
        }
    }
    return image;
}

} // namespace actcraft::render
```

- [ ] **Étape 3 : magasin d'images.** Créer `render/include/actcraft/render/imagestore.h` :

```cpp
#pragma once

#include <actcraft/core/model.h>

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QThreadPool>

#include <cstdint>
#include <unordered_map>

namespace actcraft::render {

// Génère les images d'une page sur des fils secondaires (règle de fluidité 1) ; ready() quand toutes sont prêtes.
// image() est sûre depuis le fil de rendu.
class ImageStore : public QObject
{
    Q_OBJECT

public:
    explicit ImageStore(QObject* parent = nullptr);
    ~ImageStore() override;

    void generate(const core::Page& page);
    bool isReady() const noexcept;
    QImage image(std::size_t objectIndex) const;
    std::size_t imageCount() const;
    // Crochet de test : vrai si toutes les images de la dernière génération ont été synthétisées hors du fil de
    // l'objet.
    bool generatedOffOwnerThread() const noexcept;

signals:
    void ready();

private:
    void onImageReady(std::uint64_t generation, std::size_t index, const QImage& image, bool offOwnerThread);

    QThreadPool m_pool;
    mutable QMutex m_mutex;
    std::unordered_map<std::size_t, QImage> m_images;
    std::size_t m_pending = 0;
    std::uint64_t m_generation = 0;
    bool m_ready = false;
    bool m_offOwnerThread = true;
};

} // namespace actcraft::render
```

Créer `render/src/imagestore.cpp` :

```cpp
#include <actcraft/render/imagestore.h>
#include <actcraft/render/imagesynth.h>

#include <QMutexLocker>
#include <QThread>

#include <algorithm>

namespace actcraft::render {

ImageStore::ImageStore(QObject* parent) : QObject(parent)
{
    m_pool.setMaxThreadCount(std::max(1, QThread::idealThreadCount() - 1));
}

ImageStore::~ImageStore()
{
    // Les tâches postent vers `this` : on les attend avant que l'objet disparaisse.
    m_pool.clear();
    m_pool.waitForDone();
}

void ImageStore::generate(const core::Page& page)
{
    const std::uint64_t generation = ++m_generation;
    {
        QMutexLocker lock(&m_mutex);
        m_images.clear();
    }
    m_ready = false;
    m_pending = 0;
    m_offOwnerThread = true;
    QThread* const owner = thread();
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        const auto* object = std::get_if<core::ImageObject>(&page.objects[i]);
        if (!object) {
            continue;
        }
        ++m_pending;
        const core::ImageObject copy = *object;
        m_pool.start([this, generation, i, copy, owner] {
            const QImage image = synthesizeImage(copy);
            const bool off = QThread::currentThread() != owner;
            QMetaObject::invokeMethod(
                this, [this, generation, i, image, off] { onImageReady(generation, i, image, off); },
                Qt::QueuedConnection);
        });
    }
    if (m_pending == 0) {
        QMetaObject::invokeMethod(
            this,
            [this, generation] {
                if (generation == m_generation) {
                    m_ready = true;
                    emit ready();
                }
            },
            Qt::QueuedConnection);
    }
}

void ImageStore::onImageReady(std::uint64_t generation, std::size_t index, const QImage& image, bool offOwnerThread)
{
    if (generation != m_generation) {
        return;
    }
    m_offOwnerThread = m_offOwnerThread && offOwnerThread;
    {
        QMutexLocker lock(&m_mutex);
        m_images[index] = image;
    }
    if (--m_pending == 0) {
        m_ready = true;
        emit ready();
    }
}

bool ImageStore::isReady() const noexcept
{
    return m_ready;
}

bool ImageStore::generatedOffOwnerThread() const noexcept
{
    return m_offOwnerThread;
}

QImage ImageStore::image(std::size_t objectIndex) const
{
    QMutexLocker lock(&m_mutex);
    const auto it = m_images.find(objectIndex);
    return it == m_images.end() ? QImage() : it->second;
}

std::size_t ImageStore::imageCount() const
{
    QMutexLocker lock(&m_mutex);
    return m_images.size();
}

} // namespace actcraft::render
```

- [ ] **Étape 4 : moteur image et registre.** Créer `render/include/actcraft/render/imagerenderer.h` :

```cpp
#pragma once

#include <actcraft/render/objectrenderer.h>

namespace actcraft::render {

// QSGImageNode : Qt le fournit pour le RHI et pour le rendu logiciel ; un seul chemin suffit.
class ImageRenderer final : public ObjectRenderer
{
public:
    core::ObjectKind kind() const noexcept override { return core::ObjectKind::Image; }
    QSGNode* createNode(const core::PageObject& object, std::size_t objectIndex,
                        const RenderContext& context) const override;
};

} // namespace actcraft::render
```

Créer `render/src/imagerenderer.cpp` :

```cpp
#include <actcraft/render/imagerenderer.h>
#include <actcraft/render/imagestore.h>

#include <QQuickWindow>
#include <QSGImageNode>
#include <QSGTexture>

namespace actcraft::render {

QSGNode* ImageRenderer::createNode(const core::PageObject& object, std::size_t objectIndex,
                                   const RenderContext& context) const
{
    const auto* image = std::get_if<core::ImageObject>(&object);
    if (!image || !context.images || !context.window) {
        return nullptr;
    }
    const QImage pixels = context.images->image(objectIndex);
    if (pixels.isNull()) {
        return nullptr;
    }
    QSGImageNode* node = context.window->createImageNode();
    node->setTexture(context.window->createTextureFromImage(pixels));
    node->setOwnsTexture(true);
    node->setFiltering(QSGTexture::Linear);
    node->setRect(QRectF(image->bounds.x, image->bounds.y, image->bounds.width, image->bounds.height));
    return node;
}

} // namespace actcraft::render
```

Dans `render/src/registry.cpp`, ajouter `#include <actcraft/render/imagerenderer.h>` et, dans
`withDefaults()`, entre la forme et le trait :

```cpp
    registry.add(std::make_unique<ImageRenderer>());
```

Dans `render/CMakeLists.txt`, ajouter aux sources (l'en-tête de `ImageStore` est listé : AUTOMOC ne trouve
pas seul un en-tête `Q_OBJECT` rangé dans `include/`) :

```cmake
        include/actcraft/render/imagestore.h
        src/imagesynth.cpp
        src/imagestore.cpp
        src/imagerenderer.cpp
```

- [ ] **Étape 5 : vert, format, commit.** Commande bornée : `tst_render_images ... Passed`, `TESTS OK`.

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add render/CMakeLists.txt render/include/actcraft/render/imagesynth.h render/include/actcraft/render/imagestore.h render/include/actcraft/render/imagerenderer.h render/src/imagesynth.cpp render/src/imagestore.cpp render/src/imagerenderer.cpp render/src/registry.cpp tests/render.cmake tests/tst_render_images.cpp
git -C <worktree> commit -m "render : images procédurales générées hors du fil principal, moteur image" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

### Tâche T6 — `render` : élément « page fixe »

**Objectif.** L'élément `FixedPageItem` (exposé en QML sous le nom `FixedPage` par T12) construit les nœuds de
la page par le registre : fond blanc, puis les 300 objets dans l'ordre d'empilement, une fois les images prêtes ;
un trait figé s'y ajoute par `appendStroke` sans reconstruire le reste (décision 10). Il ne gère pas le cache :
`ShaderEffectSource` (T12) le prend pour source.

**Fichiers.**
- Créer : `render/include/actcraft/render/fixedpageitem.h`, `render/src/fixedpageitem.cpp`,
  `tests/tst_render_fixedpage.cpp`
- Modifier : `render/CMakeLists.txt`, `tests/render.cmake`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_render_fixedpage .*(Passed|Failed)' | Select-Object -Last 3
```
Attendu : `out of N` avec N ≥ 9, `tst_render_fixedpage ... Passed`, `TESTS OK`.

**Interdits.** Ne pas reconstruire tous les nœuds à chaque trait figé (seuls les objets nouveaux sont ajoutés).
Ne pas lire `m_page` depuis le fil de rendu ailleurs que dans `updatePaintNode` (fil principal bloqué).

**Terminé quand.** Preuve verte : page générée prête, 300 objets, les trois témoins à leur couleur, un trait
ajouté visible ; commit fait.

**Étapes.**

- [ ] **Étape 1 : tests (rouges).** Créer `tests/tst_render_fixedpage.cpp` :

```cpp
#include "support/renderhost.h"

#include <actcraft/core/generator.h>
#include <actcraft/render/fixedpageitem.h>

#include <QSignalSpy>
#include <QTest>

using namespace actcraft;

namespace {
QColor argb(core::Argb c)
{
    return QColor::fromRgba(c);
}
} // namespace

class TstRenderFixedPage : public QObject
{
    Q_OBJECT

private:
    struct Fixture
    {
        QQuickWindow window;
        render::FixedPageItem* page = nullptr;
    };

    static void open(Fixture& f)
    {
        f.window.setColor(QColor(0x3c, 0x3f, 0x45));
        f.window.resize(1024, 768);
        f.page = new render::FixedPageItem;
        f.page->setSize(QSizeF(1024, 768));
        f.page->setParentItem(f.window.contentItem());
        f.window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&f.window));
    }

private slots:
    void generatedPageBecomesReadyWithWitnesses()
    {
        Fixture f;
        open(f);
        f.page->loadGeneratedPage();
        QTRY_VERIFY_WITH_TIMEOUT(f.page->isReady(), 20000);
        QCOMPARE(f.page->objectCount(), 300);
        const QImage image = f.window.grabWindow();
        const core::Witnesses& w = core::witnesses();
        const auto at = [&](core::PointF p) {
            return image.pixelColor(static_cast<int>(p.x), static_cast<int>(p.y));
        };
        QVERIFY(colorNear(at(w.shapeRect.center()), argb(w.shapeColor), 3));
        QVERIFY(colorNear(at(w.imageRect.center()), argb(w.imageCenterColor), 3));
        QVERIFY(colorNear(at(w.strokeProbe), argb(w.strokeColor), 10));
    }

    void appendedStrokeIsDrawnAtTop()
    {
        Fixture f;
        open(f);
        f.page->loadGeneratedPage();
        QTRY_VERIFY_WITH_TIMEOUT(f.page->isReady(), 20000);
        QSignalSpy appended(f.page, &render::FixedPageItem::strokeAppended);
        core::Stroke stroke;
        stroke.baseWidth = 6;
        stroke.color = 0xFF000000u;
        stroke.points = {{300, 690, 0.5f}, {400, 690, 0.5f}, {500, 690, 0.5f}};
        f.page->appendStroke(stroke);
        QCOMPARE(appended.count(), 1);
        QCOMPARE(f.page->objectCount(), 301);
        const QImage image = f.window.grabWindow();
        QVERIFY(colorNear(image.pixelColor(400, 690), QColor(Qt::black), 40));
    }

    void emptyPageIsWhite()
    {
        Fixture f;
        open(f);
        f.page->setPage(core::Page{});
        QTRY_VERIFY_WITH_TIMEOUT(f.page->isReady(), 2000);
        QCOMPARE(f.page->objectCount(), 0);
        const QImage image = f.window.grabWindow();
        QVERIFY(colorNear(image.pixelColor(512, 384), QColor(Qt::white), 1));
    }
};

QTEST_MAIN(TstRenderFixedPage)
#include "tst_render_fixedpage.moc"
```

Dans `tests/render.cmake`, ajouter :

```cmake
actcraft_add_test(tst_render_fixedpage
    SOURCES tst_render_fixedpage.cpp
    LIBS ActCraft::render Qt6::Quick)
```
Commande bornée : `TESTS EN ECHEC (compilation)`.

- [ ] **Étape 2 : en-tête.** Créer `render/include/actcraft/render/fixedpageitem.h` :

```cpp
#pragma once

#include <actcraft/core/model.h>
#include <actcraft/render/imagestore.h>
#include <actcraft/render/objectrenderer.h>

#include <QQuickItem>

namespace actcraft::render {

// Contenu fixe de la page : modèle → nœuds par le registre de moteurs. Les traits figés s'y ajoutent
// (décision 10). Le cache (ShaderEffectSource, Main.qml) prend cet élément pour source.
class FixedPageItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ isReady NOTIFY readyChanged)
    Q_PROPERTY(int objectCount READ objectCount NOTIFY objectCountChanged)

public:
    explicit FixedPageItem(QQuickItem* parent = nullptr);

    // Page des 300 objets (core::generatePage(core::kDefaultSeed)).
    Q_INVOKABLE void loadGeneratedPage();
    void setPage(core::Page page);
    const core::Page& page() const noexcept;
    // Trait figé : ajouté au modèle et à la scène à la prochaine synchronisation, sans reconstruire le reste.
    void appendStroke(core::Stroke stroke);
    // Vrai quand les images sont prêtes et les nœuds construits à la prochaine image.
    bool isReady() const noexcept;
    int objectCount() const noexcept;

signals:
    void readyChanged();
    void objectCountChanged();
    void strokeAppended();

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData* data) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private:
    void onImagesReady();

    core::Page m_page;
    RendererRegistry m_registry;
    ImageStore m_images;
    bool m_ready = false;
    bool m_rebuild = true;
    std::size_t m_builtCount = 0;
};

} // namespace actcraft::render
```

- [ ] **Étape 3 : implémentation.** Créer `render/src/fixedpageitem.cpp` :

```cpp
#include <actcraft/core/generator.h>
#include <actcraft/render/fixedpageitem.h>

#include <QQuickWindow>
#include <QSGRectangleNode>
#include <QSGRendererInterface>

namespace actcraft::render {

namespace {
void clearChildren(QSGNode* root)
{
    while (QSGNode* child = root->firstChild()) {
        root->removeChildNode(child);
        delete child;
    }
}
} // namespace

FixedPageItem::FixedPageItem(QQuickItem* parent)
    : QQuickItem(parent), m_registry(RendererRegistry::withDefaults())
{
    setFlag(ItemHasContents);
    connect(&m_images, &ImageStore::ready, this, &FixedPageItem::onImagesReady);
}

void FixedPageItem::loadGeneratedPage()
{
    setPage(core::generatePage(core::kDefaultSeed));
}

void FixedPageItem::setPage(core::Page page)
{
    m_page = std::move(page);
    m_rebuild = true;
    if (m_ready) {
        m_ready = false;
        emit readyChanged();
    }
    m_images.generate(m_page);
    emit objectCountChanged();
    update();
}

const core::Page& FixedPageItem::page() const noexcept
{
    return m_page;
}

void FixedPageItem::appendStroke(core::Stroke stroke)
{
    m_page.objects.emplace_back(std::move(stroke));
    emit objectCountChanged();
    emit strokeAppended();
    update();
}

bool FixedPageItem::isReady() const noexcept
{
    return m_ready;
}

int FixedPageItem::objectCount() const noexcept
{
    return static_cast<int>(m_page.objects.size());
}

void FixedPageItem::onImagesReady()
{
    m_rebuild = true;
    m_ready = true;
    emit readyChanged();
    update();
}

void FixedPageItem::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    m_rebuild = true;
    update();
}

QSGNode* FixedPageItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
    QSGNode* root = old ? old : new QSGNode;
    if (m_rebuild || !old) {
        clearChildren(root);
        QSGRectangleNode* background = window()->createRectangleNode();
        background->setRect(boundingRect());
        background->setColor(Qt::white);
        root->appendChildNode(background);
        m_builtCount = 0;
        m_rebuild = !m_ready; // tant que les images manquent, on ne dessine que le fond
    }
    if (m_ready) {
        RenderContext context;
        context.window = window();
        context.software = window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
        context.images = &m_images;
        for (std::size_t i = m_builtCount; i < m_page.objects.size(); ++i) {
            if (const ObjectRenderer* renderer = m_registry.find(core::kindOf(m_page.objects[i]))) {
                if (QSGNode* node = renderer->createNode(m_page.objects[i], i, context)) {
                    root->appendChildNode(node);
                }
            }
        }
        m_builtCount = m_page.objects.size();
    }
    return root;
}

} // namespace actcraft::render
```

Dans `render/CMakeLists.txt`, ajouter aux sources `include/actcraft/render/fixedpageitem.h` et
`src/fixedpageitem.cpp`, et après l'appel `actcraft_add_module(render …)` :

```cmake
# Les types de render/ sont exposés en QML par ui/ (QML_FOREIGN) : qmltyperegistrar a besoin de leurs métatypes.
qt_extract_metatypes(actcraft_render)
```

- [ ] **Étape 4 : vert, format, commit.** Commande bornée : `tst_render_fixedpage ... Passed`, `TESTS OK`.

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add render/CMakeLists.txt render/include/actcraft/render/fixedpageitem.h render/src/fixedpageitem.cpp tests/render.cmake tests/tst_render_fixedpage.cpp
git -C <worktree> commit -m "render : élément page fixe (300 objets, traits figés ajoutés sans reconstruction)" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

### Tâche T7 — `input` : échantillons, séquence, filtre, prédiction identité

**Objectif.** La chaîne d'entrée de la décision 12 : événement Qt → échantillon (position de page, pression,
horodatage monotone, type de périphérique, phase) → numéro de séquence → sonde → filtre des points identiques →
prédiction (identité, N1-9) → outil. Le numéro est attribué **avant** le filtre : le banc relie ainsi chaque
événement injecté à son échantillon (DA4).

**Fichiers.**
- Créer : `input/include/actcraft/input/sample.h`, `input/include/actcraft/input/pipeline.h`,
  `input/include/actcraft/input/qtevents.h`, `input/src/pipeline.cpp`, `input/src/qtevents.cpp`,
  `tests/tst_input.cpp`
- Modifier : `input/CMakeLists.txt`, `tests/input.cmake`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_input .*(Passed|Failed)' | Select-Object -Last 3
```
Attendu : `out of N` avec N ≥ 6, `tst_input ... Passed`, `TESTS OK`.

**Interdits.** Ne jamais activer `AA_CompressHighFrequencyEvents` avec `AA_CompressTabletEvents`. Pas de
dépendance vers `render/`, `tools/` ni `ui/`.

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : tests (rouges).** Créer `tests/tst_input.cpp` :

```cpp
#include <actcraft/core/inkprobe.h>
#include <actcraft/input/pipeline.h>
#include <actcraft/input/qtevents.h>

#include <QMouseEvent>
#include <QPointingDevice>
#include <QTabletEvent>
#include <QTest>

#include <utility>
#include <vector>

using namespace actcraft;
using input::Phase;

namespace {

class RecordingSink : public input::SampleSink
{
public:
    std::vector<input::Sample> samples;
    void onSample(const input::Sample& sample) override { samples.push_back(sample); }
};

class RecordingProbe : public core::InkProbe
{
public:
    std::vector<std::pair<std::uint64_t, std::int64_t>> received;
    void sampleReceived(std::uint64_t seq, std::int64_t nowNs) override { received.emplace_back(seq, nowNs); }
    void samplesSynced(std::uint64_t, std::int64_t) override {}
};

input::Sample make(Phase phase, double x, double y, float pressure = 0.5f, std::int64_t t = 0)
{
    input::Sample s;
    s.phase = phase;
    s.x = x;
    s.y = y;
    s.pressure = pressure;
    s.timestampNs = t;
    s.device = input::DeviceKind::Stylus;
    return s;
}

} // namespace

class TstInput : public QObject
{
    Q_OBJECT

private slots:
    void sequenceNumbersIncreaseAndReset()
    {
        RecordingSink sink;
        input::Pipeline pipeline(sink);
        QCOMPARE(pipeline.push(make(Phase::Press, 1, 1)), std::uint64_t{0});
        QCOMPARE(pipeline.push(make(Phase::Move, 2, 2)), std::uint64_t{1});
        QCOMPARE(pipeline.nextSequence(), std::uint64_t{2});
        pipeline.resetSequence(10);
        QCOMPARE(pipeline.push(make(Phase::Move, 3, 3)), std::uint64_t{10});
        QCOMPARE(sink.samples.back().seq, std::uint64_t{10});
    }

    void duplicateMoveFilteredButSequenceConsumed()
    {
        RecordingSink sink;
        input::Pipeline pipeline(sink);
        pipeline.push(make(Phase::Press, 1, 1));
        pipeline.push(make(Phase::Move, 1, 1));
        pipeline.push(make(Phase::Move, 2, 2));
        QCOMPARE(sink.samples.size(), std::size_t{2});
        QCOMPARE(sink.samples[0].seq, std::uint64_t{0});
        QCOMPARE(sink.samples[1].seq, std::uint64_t{2});
    }

    void releaseIsNeverFiltered()
    {
        RecordingSink sink;
        input::Pipeline pipeline(sink);
        pipeline.push(make(Phase::Press, 5, 5));
        pipeline.push(make(Phase::Move, 6, 6));
        pipeline.push(make(Phase::Release, 6, 6));
        QCOMPARE(sink.samples.size(), std::size_t{3});
        QCOMPARE(sink.samples.back().phase, Phase::Release);
    }

    void probeSeesEveryReceivedSample()
    {
        RecordingSink sink;
        RecordingProbe probe;
        input::Pipeline pipeline(sink);
        pipeline.setProbe(&probe);
        pipeline.push(make(Phase::Press, 1, 1, 0.5f, 100));
        pipeline.push(make(Phase::Move, 1, 1, 0.5f, 200)); // filtré, mais reçu
        pipeline.push(make(Phase::Move, 2, 2, 0.5f, 300));
        QCOMPARE(probe.received.size(), std::size_t{3});
        QCOMPARE(probe.received[1].first, std::uint64_t{1});
        QCOMPARE(probe.received[1].second, std::int64_t{200});
    }

    void predictorIsIdentity()
    {
        const input::IdentityPredictor predictor;
        const input::Sample s = make(Phase::Move, 3.5, 4.25, 0.7f, 42);
        const input::Sample p = predictor.predict(s);
        QCOMPARE(p.x, s.x);
        QCOMPARE(p.y, s.y);
        QCOMPARE(p.pressure, s.pressure);
        QCOMPARE(p.timestampNs, s.timestampNs);
    }

    void tabletEventsConvert()
    {
        QPointingDevice stylus(QStringLiteral("test-stylus"), 77, QInputDevice::DeviceType::Stylus,
                               QPointingDevice::PointerType::Pen,
                               QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
        const QTabletEvent press(QEvent::TabletPress, &stylus, QPointF(5, 5), QPointF(5, 5), 0.7, 0, 0, 0, 0, 0,
                                 Qt::NoModifier, Qt::LeftButton, Qt::LeftButton);
        const auto s = input::sampleFromEvent(press, QPointF(100, 200), 123);
        QVERIFY(s.has_value());
        QCOMPARE(s->phase, Phase::Press);
        QCOMPARE(s->device, input::DeviceKind::Stylus);
        QVERIFY(qAbs(s->pressure - 0.7f) < 0.001f);
        QCOMPARE(s->x, 100.0);
        QCOMPARE(s->y, 200.0);
        QCOMPARE(s->timestampNs, std::int64_t{123});

        const QTabletEvent move(QEvent::TabletMove, &stylus, QPointF(6, 6), QPointF(6, 6), 0.4, 0, 0, 0, 0, 0,
                                Qt::NoModifier, Qt::NoButton, Qt::LeftButton);
        QCOMPARE(input::sampleFromEvent(move, QPointF(1, 1), 0)->phase, Phase::Move);

        const QTabletEvent hover(QEvent::TabletMove, &stylus, QPointF(6, 6), QPointF(6, 6), 0.0, 0, 0, 0, 0, 0,
                                 Qt::NoModifier, Qt::NoButton, Qt::NoButton);
        QVERIFY(!input::sampleFromEvent(hover, QPointF(1, 1), 0).has_value());

        const QTabletEvent release(QEvent::TabletRelease, &stylus, QPointF(6, 6), QPointF(6, 6), 0.0, 0, 0, 0,
                                   0, 0, Qt::NoModifier, Qt::LeftButton, Qt::NoButton);
        QCOMPARE(input::sampleFromEvent(release, QPointF(1, 1), 0)->phase, Phase::Release);
    }

    void mouseLeftButtonConverts()
    {
        const QMouseEvent press(QEvent::MouseButtonPress, QPointF(5, 5), QPointF(5, 5), QPointF(5, 5),
                                Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        const auto s = input::sampleFromEvent(press, QPointF(10, 20), 0);
        QVERIFY(s.has_value());
        QCOMPARE(s->device, input::DeviceKind::Mouse);
        QCOMPARE(s->pressure, 0.5f);
        const QMouseEvent right(QEvent::MouseButtonPress, QPointF(5, 5), QPointF(5, 5), QPointF(5, 5),
                                Qt::RightButton, Qt::RightButton, Qt::NoModifier);
        QVERIFY(!input::sampleFromEvent(right, QPointF(10, 20), 0).has_value());
        const QMouseEvent hover(QEvent::MouseMove, QPointF(5, 5), QPointF(5, 5), QPointF(5, 5), Qt::NoButton,
                                Qt::NoButton, Qt::NoModifier);
        QVERIFY(!input::sampleFromEvent(hover, QPointF(10, 20), 0).has_value());
    }

    void otherEventsIgnored()
    {
        const QEvent focus(QEvent::FocusIn);
        QVERIFY(!input::sampleFromEvent(focus, QPointF(0, 0), 0).has_value());
    }
};

QTEST_MAIN(TstInput)
#include "tst_input.moc"
```

Dans `tests/input.cmake` :

```cmake
actcraft_add_test(tst_input
    SOURCES tst_input.cpp
    LIBS ActCraft::input Qt6::Gui)
```
Commande bornée : `TESTS EN ECHEC (compilation)`.

- [ ] **Étape 2 : en-têtes.** Créer `input/include/actcraft/input/sample.h` :

```cpp
#pragma once

#include <cstdint>

namespace actcraft::input {

enum class DeviceKind : std::uint8_t
{
    Stylus,
    Mouse,
    Other,
};

enum class Phase : std::uint8_t
{
    Press,
    Move,
    Release,
};

// Échantillon d'entrée, en coordonnées de page (décision 12).
struct Sample
{
    double x = 0;
    double y = 0;
    float pressure = 0.5f;
    std::int64_t timestampNs = 0; // core::monotonicNowNs() à la réception
    DeviceKind device = DeviceKind::Other;
    Phase phase = Phase::Move;
    std::uint64_t seq = 0; // attribué par Pipeline, avant le filtre
};

class SampleSink
{
public:
    virtual ~SampleSink() = default;
    virtual void onSample(const Sample& sample) = 0;
};

} // namespace actcraft::input
```

Créer `input/include/actcraft/input/pipeline.h` :

```cpp
#pragma once

#include <actcraft/core/inkprobe.h>
#include <actcraft/input/sample.h>

#include <cstdint>

namespace actcraft::input {

// Écarte un déplacement identique (position et pression) au dernier échantillon accepté.
class DuplicateFilter
{
public:
    bool accept(const Sample& sample) noexcept;
    void reset() noexcept;

private:
    bool m_hasLast = false;
    Sample m_last;
};

// Étage de prédiction du stylet : identité à l'étape 1 (hypothèse N1-9), gardé pour la suite.
class IdentityPredictor
{
public:
    Sample predict(const Sample& sample) const noexcept { return sample; }
};

// Numérote → signale à la sonde → filtre → prédit → transmet à l'outil.
class Pipeline
{
public:
    explicit Pipeline(SampleSink& sink);

    void setProbe(core::InkProbe* probe) noexcept;
    void resetSequence(std::uint64_t next = 0) noexcept;
    std::uint64_t nextSequence() const noexcept;
    // Retourne le numéro attribué (même si l'échantillon est filtré).
    std::uint64_t push(Sample sample);

private:
    SampleSink& m_sink;
    core::InkProbe* m_probe = nullptr;
    DuplicateFilter m_filter;
    IdentityPredictor m_predictor;
    std::uint64_t m_next = 0;
};

} // namespace actcraft::input
```

Créer `input/include/actcraft/input/qtevents.h` :

```cpp
#pragma once

#include <actcraft/input/sample.h>

#include <QPointF>

#include <cstdint>
#include <optional>

class QEvent;

namespace actcraft::input {

// Tablette (appui, déplacement en contact, relâché) ou souris bouton gauche (N2-12) ; nullopt pour le survol,
// les autres boutons, la souris synthétisée depuis un stylet et tout autre événement.
// `pagePos` : position déjà ramenée en coordonnées de page par la vue.
std::optional<Sample> sampleFromEvent(const QEvent& event, QPointF pagePos, std::int64_t nowNs);

} // namespace actcraft::input
```

- [ ] **Étape 3 : implémentation.** Créer `input/src/pipeline.cpp` :

```cpp
#include <actcraft/input/pipeline.h>

namespace actcraft::input {

bool DuplicateFilter::accept(const Sample& sample) noexcept
{
    if (sample.phase == Phase::Move && m_hasLast && sample.x == m_last.x && sample.y == m_last.y &&
        sample.pressure == m_last.pressure) {
        return false;
    }
    m_last = sample;
    m_hasLast = true;
    return true;
}

void DuplicateFilter::reset() noexcept
{
    m_hasLast = false;
}

Pipeline::Pipeline(SampleSink& sink) : m_sink(sink) {}

void Pipeline::setProbe(core::InkProbe* probe) noexcept
{
    m_probe = probe;
}

void Pipeline::resetSequence(std::uint64_t next) noexcept
{
    m_next = next;
}

std::uint64_t Pipeline::nextSequence() const noexcept
{
    return m_next;
}

std::uint64_t Pipeline::push(Sample sample)
{
    sample.seq = m_next++;
    if (m_probe) {
        m_probe->sampleReceived(sample.seq, sample.timestampNs);
    }
    if (m_filter.accept(sample)) {
        m_sink.onSample(m_predictor.predict(sample));
    }
    return sample.seq;
}

} // namespace actcraft::input
```

Créer `input/src/qtevents.cpp` :

```cpp
#include <actcraft/input/qtevents.h>

#include <QMouseEvent>
#include <QPointingDevice>
#include <QTabletEvent>

namespace actcraft::input {

std::optional<Sample> sampleFromEvent(const QEvent& event, QPointF pagePos, std::int64_t nowNs)
{
    Sample sample;
    sample.x = pagePos.x();
    sample.y = pagePos.y();
    sample.timestampNs = nowNs;
    switch (event.type()) {
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease: {
        const auto& tablet = static_cast<const QTabletEvent&>(event);
        if (event.type() == QEvent::TabletMove && !(tablet.buttons() & Qt::LeftButton)) {
            return std::nullopt; // survol
        }
        sample.device = DeviceKind::Stylus;
        sample.phase = event.type() == QEvent::TabletPress  ? Phase::Press
                       : event.type() == QEvent::TabletMove ? Phase::Move
                                                            : Phase::Release;
        sample.pressure = static_cast<float>(tablet.pressure());
        return sample;
    }
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease: {
        const auto& mouse = static_cast<const QMouseEvent&>(event);
        if (const QPointingDevice* device = mouse.pointingDevice();
            device && device->type() != QInputDevice::DeviceType::Mouse &&
            device->type() != QInputDevice::DeviceType::TouchPad) {
            return std::nullopt; // souris synthétisée depuis un stylet ou un écran tactile
        }
        if (event.type() == QEvent::MouseMove) {
            if (!(mouse.buttons() & Qt::LeftButton)) {
                return std::nullopt;
            }
            sample.phase = Phase::Move;
        } else {
            if (mouse.button() != Qt::LeftButton) {
                return std::nullopt;
            }
            sample.phase = event.type() == QEvent::MouseButtonPress ? Phase::Press : Phase::Release;
        }
        sample.device = DeviceKind::Mouse;
        sample.pressure = 0.5f;
        return sample;
    }
    default:
        return std::nullopt;
    }
}

} // namespace actcraft::input
```

Dans `input/CMakeLists.txt` :

```cmake
# Chaîne stylet : événements bruts, filtrage, prédiction, puis outil. Un module natif par système.
actcraft_add_module(input
    SOURCES
        src/input.cpp
        src/pipeline.cpp
        src/qtevents.cpp
    DEPENDS
        ActCraft::core Qt6::Gui)
```

- [ ] **Étape 4 : vert, format, commit.** Commande bornée : `tst_input ... Passed`, `TESTS OK`.

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add input/CMakeLists.txt input/include/actcraft/input/sample.h input/include/actcraft/input/pipeline.h input/include/actcraft/input/qtevents.h input/src/pipeline.cpp input/src/qtevents.cpp tests/input.cmake tests/tst_input.cpp
git -C <worktree> commit -m "input : échantillons numérotés, filtre des doublons, prédiction identité" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

### Tâche T8 — `tools` : `PenTool`

**Objectif.** Le stylo ne connaît que des interfaces (règle de modularité 2, décision 12) : `InkSurface` (le
trait en cours) et `DocumentSink` (le document). Appui → trait en cours ; déplacements → points (avec leur
numéro de séquence, pour la sonde) ; relâché → trait versé au document **puis** encre vidée, dans le même tour de
boucle (décision 10). `ui/` fournira les adaptateurs (T12).

**Fichiers.**
- Créer : `tools/include/actcraft/tools/pentool.h`, `tools/src/pentool.cpp`, `tests/tst_pentool.cpp`
- Modifier : `tools/CMakeLists.txt`, `tests/tools.cmake`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_pentool .*(Passed|Failed)' | Select-Object -Last 3
```
Attendu : `out of N` avec N ≥ 7, `tst_pentool ... Passed`, `TESTS OK`.

**Interdits.** `PenTool` n'inclut aucun en-tête de `render/` ni de `ui/`, ni de Qt Quick.

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : tests (rouges).** Créer `tests/tst_pentool.cpp` :

```cpp
#include <actcraft/tools/pentool.h>

#include <QTest>

#include <string>
#include <vector>

using namespace actcraft;
using input::Phase;

namespace {

struct Log
{
    std::vector<std::string> calls;
};

class FakeInk : public tools::InkSurface
{
public:
    explicit FakeInk(Log& log) : m_log(log) {}
    std::vector<std::uint64_t> seqs;
    void beginStroke(core::Argb, double) override { m_log.calls.emplace_back("begin"); }
    void addPoint(const core::StrokePoint&, std::uint64_t seq) override
    {
        m_log.calls.emplace_back("add");
        seqs.push_back(seq);
    }
    void clear() override { m_log.calls.emplace_back("clear"); }

private:
    Log& m_log;
};

class FakeDocument : public tools::DocumentSink
{
public:
    explicit FakeDocument(Log& log) : m_log(log) {}
    std::vector<core::Stroke> strokes;
    void commitStroke(core::Stroke stroke) override
    {
        m_log.calls.emplace_back("commit");
        strokes.push_back(std::move(stroke));
    }

private:
    Log& m_log;
};

input::Sample make(Phase phase, double x, double y, std::uint64_t seq, float pressure = 0.5f)
{
    input::Sample s;
    s.phase = phase;
    s.x = x;
    s.y = y;
    s.seq = seq;
    s.pressure = pressure;
    return s;
}

} // namespace

class TstPenTool : public QObject
{
    Q_OBJECT

private slots:
    void pressMoveReleaseCommitsThenClears()
    {
        Log log;
        FakeInk ink(log);
        FakeDocument document(log);
        tools::PenTool pen(ink, document);
        pen.setColor(0xFF112233u);
        pen.setBaseWidth(4);
        pen.onSample(make(Phase::Press, 0, 0, 0, 0.3f));
        QVERIFY(pen.isDrawing());
        pen.onSample(make(Phase::Move, 1, 0, 1));
        pen.onSample(make(Phase::Release, 2, 0, 2, 0.0f));
        QVERIFY(!pen.isDrawing());
        const std::vector<std::string> expected{"begin", "add", "add", "add", "commit", "clear"};
        QCOMPARE(log.calls, expected);
        QCOMPARE(document.strokes.size(), std::size_t{1});
        QCOMPARE(document.strokes[0].points.size(), std::size_t{3});
        QCOMPARE(document.strokes[0].color, core::Argb{0xFF112233u});
        QCOMPARE(document.strokes[0].baseWidth, 4.0);
        QCOMPARE(document.strokes[0].points[0].pressure, 0.3f);
    }

    void sequenceNumbersReachTheInk()
    {
        Log log;
        FakeInk ink(log);
        FakeDocument document(log);
        tools::PenTool pen(ink, document);
        pen.onSample(make(Phase::Press, 0, 0, 7));
        pen.onSample(make(Phase::Move, 1, 1, 8));
        const std::vector<std::uint64_t> expected{7, 8};
        QCOMPARE(ink.seqs, expected);
    }

    void moveWithoutPressIsIgnored()
    {
        Log log;
        FakeInk ink(log);
        FakeDocument document(log);
        tools::PenTool pen(ink, document);
        pen.onSample(make(Phase::Move, 1, 1, 0));
        pen.onSample(make(Phase::Release, 1, 1, 1));
        QVERIFY(log.calls.empty());
    }

    void singleTapDoesNotCommit()
    {
        Log log;
        FakeInk ink(log);
        FakeDocument document(log);
        tools::PenTool pen(ink, document);
        pen.onSample(make(Phase::Press, 5, 5, 0));
        pen.onSample(make(Phase::Release, 5, 5, 1));
        QVERIFY(document.strokes.empty());
        QCOMPARE(log.calls.back(), std::string("clear"));
    }

    void pressWhileDrawingFinishesPrevious()
    {
        Log log;
        FakeInk ink(log);
        FakeDocument document(log);
        tools::PenTool pen(ink, document);
        pen.onSample(make(Phase::Press, 0, 0, 0));
        pen.onSample(make(Phase::Move, 3, 0, 1));
        pen.onSample(make(Phase::Press, 10, 10, 2)); // relâché perdu
        QCOMPARE(document.strokes.size(), std::size_t{1});
        QVERIFY(pen.isDrawing());
    }
};

QTEST_GUILESS_MAIN(TstPenTool)
#include "tst_pentool.moc"
```

Dans `tests/tools.cmake` :

```cmake
actcraft_add_test(tst_pentool
    SOURCES tst_pentool.cpp
    LIBS ActCraft::tools)
```
Commande bornée : `TESTS EN ECHEC (compilation)`.

- [ ] **Étape 2 : en-tête.** Créer `tools/include/actcraft/tools/pentool.h` :

```cpp
#pragma once

#include <actcraft/core/model.h>
#include <actcraft/input/sample.h>

#include <cstdint>

namespace actcraft::tools {

// Ce que le stylo voit du trait en cours (implémenté par ui/ sur render::InkItem).
class InkSurface
{
public:
    virtual ~InkSurface() = default;
    virtual void beginStroke(core::Argb color, double baseWidth) = 0;
    virtual void addPoint(const core::StrokePoint& point, std::uint64_t seq) = 0;
    virtual void clear() = 0;
};

// Ce que le stylo voit du document (implémenté par ui/ sur render::FixedPageItem).
class DocumentSink
{
public:
    virtual ~DocumentSink() = default;
    virtual void commitStroke(core::Stroke stroke) = 0;
};

// Stylo : appui → trait en cours ; déplacements → points ; relâché → trait versé au document puis encre vidée,
// dans le même tour de boucle (décision 10). Un trait de moins de 2 points n'est pas versé.
class PenTool final : public input::SampleSink
{
public:
    PenTool(InkSurface& ink, DocumentSink& document);

    void setColor(core::Argb color) noexcept;
    void setBaseWidth(double width) noexcept;
    core::Argb color() const noexcept;
    double baseWidth() const noexcept;
    bool isDrawing() const noexcept;

    void onSample(const input::Sample& sample) override;

private:
    void addPoint(const input::Sample& sample);
    void finish();

    InkSurface& m_ink;
    DocumentSink& m_document;
    core::Argb m_color = 0xFF000000u;
    double m_baseWidth = 3.0; // N2-2
    core::Stroke m_current;
    bool m_drawing = false;
};

} // namespace actcraft::tools
```

- [ ] **Étape 3 : implémentation.** Créer `tools/src/pentool.cpp` :

```cpp
#include <actcraft/tools/pentool.h>

namespace actcraft::tools {

PenTool::PenTool(InkSurface& ink, DocumentSink& document) : m_ink(ink), m_document(document) {}

void PenTool::setColor(core::Argb color) noexcept
{
    m_color = color;
}

void PenTool::setBaseWidth(double width) noexcept
{
    m_baseWidth = width;
}

core::Argb PenTool::color() const noexcept
{
    return m_color;
}

double PenTool::baseWidth() const noexcept
{
    return m_baseWidth;
}

bool PenTool::isDrawing() const noexcept
{
    return m_drawing;
}

void PenTool::onSample(const input::Sample& sample)
{
    switch (sample.phase) {
    case input::Phase::Press:
        if (m_drawing) {
            finish();
        }
        m_current = core::Stroke{};
        m_current.baseWidth = m_baseWidth;
        m_current.color = m_color;
        m_drawing = true;
        m_ink.beginStroke(m_color, m_baseWidth);
        addPoint(sample);
        break;
    case input::Phase::Move:
        if (m_drawing) {
            addPoint(sample);
        }
        break;
    case input::Phase::Release:
        if (!m_drawing) {
            break;
        }
        if (const core::StrokePoint& last = m_current.points.back(); last.x != sample.x || last.y != sample.y) {
            addPoint(sample);
        }
        finish();
        break;
    }
}

void PenTool::addPoint(const input::Sample& sample)
{
    const core::StrokePoint point{sample.x, sample.y, sample.pressure};
    m_current.points.push_back(point);
    m_ink.addPoint(point, sample.seq);
}

void PenTool::finish()
{
    m_drawing = false;
    if (m_current.points.size() >= 2) {
        m_document.commitStroke(std::move(m_current));
    }
    m_current = core::Stroke{};
    m_ink.clear();
}

} // namespace actcraft::tools
```

Dans `tools/CMakeLists.txt`, ajouter `src/pentool.cpp` aux sources (dépendances inchangées).

- [ ] **Étape 4 : vert, format, commit.** Commande bornée : `tst_pentool ... Passed`, `TESTS OK`.

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add tools/CMakeLists.txt tools/include/actcraft/tools/pentool.h tools/src/pentool.cpp tests/tools.cmake tests/tst_pentool.cpp
git -C <worktree> commit -m "tools : stylo qui ne connaît que l'encre et le document" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

### Tâche T9 — `render` : tampon d'encre par blocs, élément d'encre (chemin RHI, sonde)

**Objectif.** Encre incrémentale (règle de fluidité 3, décision 9, DA5) : le trait en cours est rangé en blocs de
256 points (N2-3) ; le premier point d'un bloc est le dernier du précédent (jonction partagée) ; seuls les blocs
modifiés depuis la dernière synchronisation sont retouchés, en pratique le dernier. Chemin RHI : un
`QSGGeometryNode` par bloc, géométrie allouée **une fois** à sa capacité (255 segments × 6 sommets), motif
`DynamicPattern`, et seuls les **nouveaux** segments sont écrits. La sonde reçoit, pendant la synchronisation, le
numéro du dernier échantillon entré dans la géométrie (DA4).

**Le piège.** Le patron évident — reconstruire une géométrie qui grandit à chaque point, ou un seul nœud pour tout
le trait — recopie tout le trait à chaque image : le coût croît avec la longueur (leçon d'OpenBoard). Le test
`tst_render_inkbuffer` vérifie que seul le dernier bloc est sale après un ajout.

**Fichiers.**
- Créer : `render/include/actcraft/render/inkbuffer.h`, `render/include/actcraft/render/inkitem.h`,
  `render/src/inkbuffer.cpp`, `render/src/inkitem.cpp`, `tests/tst_render_inkbuffer.cpp`
- Modifier : `render/CMakeLists.txt`, `tests/render.cmake`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_render_inkbuffer .*(Passed|Failed)' | Select-Object -Last 3
```
Attendu : `out of N` avec N ≥ 10, `tst_render_inkbuffer ... Passed`, `TESTS OK`.

**Interdits.** Pas de réallocation de géométrie par point ; pas d'écriture des segments déjà écrits. Le chemin
logiciel est livré par T10 : ici `updateSoftware` ne dessine rien (aucun test de pixels de l'encre dans cette
tâche). Ne pas modifier `FixedPageItem`.

**Terminé quand.** Preuve verte (blocs, jonctions, saleté, sonde) ; commit fait. Le chemin RHI ne peut pas être
dessiné hors écran (Qt y charge toujours le rendu logiciel) : il est contrôlé par le banc en D3D11 (contrôle
« encre mi-trait », T17, joué à la clôture).

**Étapes.**

- [ ] **Étape 1 : tests (rouges).** Créer `tests/tst_render_inkbuffer.cpp` :

```cpp
#include <actcraft/core/inkprobe.h>
#include <actcraft/render/inkbuffer.h>
#include <actcraft/render/inkitem.h>

#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>

#include <mutex>
#include <vector>

using namespace actcraft;

namespace {

core::StrokePoint pt(int i)
{
    return {static_cast<double>(i), static_cast<double>(i % 7), 0.5f};
}

class SyncProbe : public core::InkProbe
{
public:
    void sampleReceived(std::uint64_t, std::int64_t) override {}
    void samplesSynced(std::uint64_t lastSeq, std::int64_t) override
    {
        std::lock_guard lock(m_mutex);
        synced.push_back(lastSeq);
    }
    std::vector<std::uint64_t> snapshot()
    {
        std::lock_guard lock(m_mutex);
        return synced;
    }

private:
    std::mutex m_mutex;
    std::vector<std::uint64_t> synced;
};

} // namespace

class TstRenderInkBuffer : public QObject
{
    Q_OBJECT

private slots:
    void thousandPointsMakeFourBlocks()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 3.0);
        for (int i = 0; i < 1000; ++i) {
            buffer.add(pt(i));
        }
        QCOMPARE(buffer.pointCount(), std::size_t{1000});
        QCOMPARE(buffer.blockCount(), std::size_t{4}); // 256 + 255 + 255 + 234
        QCOMPARE(buffer.block(0).size(), render::InkBuffer::kBlockPoints);
        QCOMPARE(buffer.block(3).size(), std::size_t{235}); // jonction + 234
    }

    void blocksShareTheirJoint()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 3.0);
        for (int i = 0; i < 600; ++i) {
            buffer.add(pt(i));
        }
        const auto b0 = buffer.block(0);
        const auto b1 = buffer.block(1);
        QCOMPARE(b1.front().x, b0.back().x);
        QCOMPARE(b1.front().y, b0.back().y);
        QCOMPARE(b1[1].x, 256.0);
    }

    void onlyTheLastBlockIsDirtyAfterAnAdd()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 3.0);
        for (int i = 0; i < 700; ++i) {
            buffer.add(pt(i));
        }
        QCOMPARE(buffer.firstDirtyBlock(), std::size_t{0});
        buffer.markClean();
        QCOMPARE(buffer.firstDirtyBlock(), buffer.blockCount());
        buffer.add(pt(700));
        QCOMPARE(buffer.firstDirtyBlock(), buffer.blockCount() - 1);
    }

    void newBlockIsDirtyFromItsIndex()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 3.0);
        for (int i = 0; i < 256; ++i) {
            buffer.add(pt(i));
        }
        buffer.markClean();
        buffer.add(pt(256)); // ouvre le bloc 1
        QCOMPARE(buffer.blockCount(), std::size_t{2});
        QCOMPARE(buffer.firstDirtyBlock(), std::size_t{1});
    }

    void clearEmptiesAndFlags()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 3.0);
        buffer.add(pt(0));
        buffer.add(pt(1));
        buffer.markClean();
        buffer.clear();
        QVERIFY(buffer.empty());
        QVERIFY(buffer.clearedSinceClean());
        buffer.markClean();
        QVERIFY(!buffer.clearedSinceClean());
    }

    void blockBoundsCoverTheBlock()
    {
        render::InkBuffer buffer;
        buffer.begin(0xFF000000u, 4.0);
        buffer.add({10, 10, 1.0f});
        buffer.add({30, 20, 0.5f});
        const core::RectF r = buffer.blockBounds(0);
        QVERIFY(r.x <= 7.0 && r.y <= 7.0); // demi-largeur max 3
        QVERIFY(r.x + r.width >= 33.0 && r.y + r.height >= 23.0);
    }

    void probeReportsLastSyncedSequence()
    {
        QQuickWindow window;
        window.resize(200, 200);
        auto* ink = new render::InkItem;
        ink->setSize(QSizeF(200, 200));
        ink->setParentItem(window.contentItem());
        SyncProbe probe;
        ink->setProbe(&probe);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ink->beginStroke(0xFF000000u, 3.0);
        ink->addPoint({10, 10, 0.5f}, 7);
        ink->addPoint({20, 10, 0.5f}, 8);
        QSignalSpy swapped(&window, &QQuickWindow::frameSwapped);
        QVERIFY(swapped.wait(2000));
        QTRY_VERIFY_WITH_TIMEOUT(!probe.snapshot().empty(), 2000);
        QCOMPARE(probe.snapshot().back(), std::uint64_t{8});
        QCOMPARE(ink->pointCount(), std::size_t{2});
        const auto before = probe.snapshot().size();
        window.update(); // image sans nouveau point : pas de nouveau signalement
        QTest::qWait(100);
        QCOMPARE(probe.snapshot().size(), before);
    }
};

QTEST_MAIN(TstRenderInkBuffer)
#include "tst_render_inkbuffer.moc"
```

Dans `tests/render.cmake`, ajouter :

```cmake
actcraft_add_test(tst_render_inkbuffer
    SOURCES tst_render_inkbuffer.cpp
    LIBS ActCraft::render Qt6::Quick)
```
Commande bornée : `TESTS EN ECHEC (compilation)`.

- [ ] **Étape 2 : tampon.** Créer `render/include/actcraft/render/inkbuffer.h` :

```cpp
#pragma once

#include <actcraft/core/model.h>

#include <cstddef>
#include <span>
#include <vector>

namespace actcraft::render {

// Trait en cours rangé en blocs de taille fixe (DA5). Le premier point du bloc b > 0 est le dernier du bloc
// b − 1 (jonction partagée). Commun aux chemins RHI et logiciel.
class InkBuffer
{
public:
    static constexpr std::size_t kBlockPoints = 256; // N2-3

    void begin(core::Argb color, double baseWidth);
    void add(const core::StrokePoint& point);
    void clear() noexcept;

    bool empty() const noexcept;
    std::size_t pointCount() const noexcept;
    std::size_t blockCount() const noexcept;
    std::span<const core::StrokePoint> block(std::size_t index) const;
    // Rectangle du bloc, agrandi de la demi-largeur maximale de ses points.
    core::RectF blockBounds(std::size_t index) const;
    // Plus petit indice de bloc modifié depuis markClean() ; blockCount() si aucun.
    std::size_t firstDirtyBlock() const noexcept;
    bool clearedSinceClean() const noexcept;
    void markClean() noexcept;
    core::Argb color() const noexcept;
    double baseWidth() const noexcept;

private:
    // Boîte des points d'un bloc et demi-largeur maximale : un segment prend la largeur de son premier point
    // et déborde de son second point d'autant (bout carré).
    struct Extent
    {
        double minX;
        double minY;
        double maxX;
        double maxY;
        double maxHalf;
    };

    std::vector<std::vector<core::StrokePoint>> m_blocks;
    std::vector<Extent> m_extents;
    std::size_t m_points = 0;
    std::size_t m_firstDirty = 0;
    bool m_cleared = false;
    core::Argb m_color = 0xFF000000u;
    double m_baseWidth = 3.0;
};

} // namespace actcraft::render
```

Créer `render/src/inkbuffer.cpp` :

```cpp
#include <actcraft/render/inkbuffer.h>

#include <algorithm>

namespace actcraft::render {

namespace {
void extend(double& minX, double& minY, double& maxX, double& maxY, const core::StrokePoint& p)
{
    minX = std::min(minX, p.x);
    minY = std::min(minY, p.y);
    maxX = std::max(maxX, p.x);
    maxY = std::max(maxY, p.y);
}
} // namespace

void InkBuffer::begin(core::Argb color, double baseWidth)
{
    clear();
    m_color = color;
    m_baseWidth = baseWidth;
}

void InkBuffer::add(const core::StrokePoint& point)
{
    if (m_blocks.empty() || m_blocks.back().size() == kBlockPoints) {
        std::vector<core::StrokePoint> block;
        block.reserve(kBlockPoints);
        Extent extent{point.x, point.y, point.x, point.y, 0.0};
        if (!m_blocks.empty()) {
            const core::StrokePoint joint = m_blocks.back().back(); // jonction partagée
            block.push_back(joint);
            extent = {joint.x, joint.y, joint.x, joint.y,
                      core::strokeWidthAt(m_baseWidth, joint.pressure) / 2};
        }
        m_blocks.push_back(std::move(block));
        m_extents.push_back(extent);
    }
    m_blocks.back().push_back(point);
    Extent& e = m_extents.back();
    extend(e.minX, e.minY, e.maxX, e.maxY, point);
    e.maxHalf = std::max(e.maxHalf, core::strokeWidthAt(m_baseWidth, point.pressure) / 2);
    ++m_points;
    m_firstDirty = std::min(m_firstDirty, m_blocks.size() - 1);
}

void InkBuffer::clear() noexcept
{
    if (!m_blocks.empty()) {
        m_cleared = true;
    }
    m_blocks.clear();
    m_extents.clear();
    m_points = 0;
    m_firstDirty = 0;
}

bool InkBuffer::empty() const noexcept
{
    return m_points == 0;
}

std::size_t InkBuffer::pointCount() const noexcept
{
    return m_points;
}

std::size_t InkBuffer::blockCount() const noexcept
{
    return m_blocks.size();
}

std::span<const core::StrokePoint> InkBuffer::block(std::size_t index) const
{
    return m_blocks[index];
}

core::RectF InkBuffer::blockBounds(std::size_t index) const
{
    const Extent& e = m_extents[index];
    return {e.minX - e.maxHalf, e.minY - e.maxHalf, e.maxX - e.minX + 2 * e.maxHalf,
            e.maxY - e.minY + 2 * e.maxHalf};
}

std::size_t InkBuffer::firstDirtyBlock() const noexcept
{
    return std::min(m_firstDirty, m_blocks.size());
}

bool InkBuffer::clearedSinceClean() const noexcept
{
    return m_cleared;
}

void InkBuffer::markClean() noexcept
{
    m_firstDirty = m_blocks.size();
    m_cleared = false;
}

core::Argb InkBuffer::color() const noexcept
{
    return m_color;
}

double InkBuffer::baseWidth() const noexcept
{
    return m_baseWidth;
}

} // namespace actcraft::render
```

Note : après `markClean()`, `m_firstDirty` vaut `blockCount()` ; un ajout dans le dernier bloc le ramène à
`blockCount() − 1`, l'ouverture d'un bloc à son indice.

- [ ] **Étape 3 : élément d'encre.** Créer `render/include/actcraft/render/inkitem.h` :

```cpp
#pragma once

#include <actcraft/core/inkprobe.h>
#include <actcraft/render/inkbuffer.h>

#include <QQuickItem>

#include <cstdint>

namespace actcraft::render {

// Trait en cours, au-dessus du cache (décision 9). Exposé en QML sous le nom InkLayer (ui/, T12).
class InkItem : public QQuickItem
{
    Q_OBJECT

public:
    explicit InkItem(QQuickItem* parent = nullptr);

    void beginStroke(core::Argb color, double baseWidth);
    void addPoint(const core::StrokePoint& point, std::uint64_t seq);
    void clear();
    void setProbe(core::InkProbe* probe) noexcept;
    std::size_t pointCount() const noexcept;
    std::size_t blockCount() const noexcept;

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData* data) override;

private:
    void updateRhi(QSGNode* root);
    void updateSoftware(QSGNode* root);

    InkBuffer m_buffer;
    core::InkProbe* m_probe = nullptr;
    std::uint64_t m_lastSeq = 0;
    bool m_seqPending = false;
};

} // namespace actcraft::render
```

Créer `render/src/inkitem.cpp` :

```cpp
#include <actcraft/core/clock.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/strokegeometry.h>

#include <QQuickWindow>
#include <QSGFlatColorMaterial>
#include <QSGGeometryNode>
#include <QSGRendererInterface>

#include <cstring>
#include <vector>

namespace actcraft::render {

namespace {

void clearChildren(QSGNode* root)
{
    while (QSGNode* child = root->firstChild()) {
        root->removeChildNode(child);
        delete child;
    }
}

// Bloc RHI : géométrie allouée une fois à sa capacité ; seuls les nouveaux segments sont écrits.
class InkBlockNode final : public QSGGeometryNode
{
public:
    static constexpr int kCapacity = static_cast<int>((InkBuffer::kBlockPoints - 1) * kVerticesPerSegment);

    explicit InkBlockNode(QColor color) : m_geometry(QSGGeometry::defaultAttributes_Point2D(), kCapacity)
    {
        m_geometry.setDrawingMode(QSGGeometry::DrawTriangles);
        m_geometry.setVertexDataPattern(QSGGeometry::DynamicPattern);
        std::memset(m_geometry.vertexData(), 0, static_cast<std::size_t>(kCapacity) * sizeof(Vertex2D));
        setGeometry(&m_geometry);
        m_material.setColor(color);
        setMaterial(&m_material);
    }

    void appendFrom(std::span<const core::StrokePoint> points, double baseWidth)
    {
        if (points.size() < 2 || points.size() - 1 <= m_segments) {
            return;
        }
        m_scratch.clear();
        // Segments [m_segments, points.size() − 1) : chacun ne dépend que de ses deux points.
        appendStrokeTriangles(points.subspan(m_segments), baseWidth, m_scratch);
        auto* vertices = static_cast<Vertex2D*>(m_geometry.vertexData());
        std::memcpy(vertices + m_segments * kVerticesPerSegment, m_scratch.data(),
                    m_scratch.size() * sizeof(Vertex2D));
        m_segments = points.size() - 1;
        m_geometry.markVertexDataDirty();
        markDirty(QSGNode::DirtyGeometry);
    }

private:
    QSGGeometry m_geometry;
    QSGFlatColorMaterial m_material;
    std::size_t m_segments = 0;
    std::vector<Vertex2D> m_scratch;
};

} // namespace

InkItem::InkItem(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents);
}

void InkItem::beginStroke(core::Argb color, double baseWidth)
{
    m_buffer.begin(color, baseWidth);
    update();
}

void InkItem::addPoint(const core::StrokePoint& point, std::uint64_t seq)
{
    m_buffer.add(point);
    m_lastSeq = seq;
    m_seqPending = true;
    update();
}

void InkItem::clear()
{
    m_buffer.clear();
    update();
}

void InkItem::setProbe(core::InkProbe* probe) noexcept
{
    m_probe = probe;
}

std::size_t InkItem::pointCount() const noexcept
{
    return m_buffer.pointCount();
}

std::size_t InkItem::blockCount() const noexcept
{
    return m_buffer.blockCount();
}

QSGNode* InkItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
    QSGNode* root = old ? old : new QSGNode;
    if (m_buffer.clearedSinceClean()) {
        clearChildren(root);
    }
    if (window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software) {
        updateSoftware(root);
    } else {
        updateRhi(root);
    }
    m_buffer.markClean();
    if (m_probe && m_seqPending) {
        m_probe->samplesSynced(m_lastSeq, core::monotonicNowNs());
    }
    m_seqPending = false;
    return root;
}

void InkItem::updateRhi(QSGNode* root)
{
    const std::size_t count = m_buffer.blockCount();
    std::size_t index = 0;
    QSGNode* child = root->firstChild();
    // Avancer jusqu'au premier bloc sale ; créer les nœuds manquants.
    for (; index < count; ++index) {
        if (!child) {
            child = new InkBlockNode(toQColor(m_buffer.color()));
            root->appendChildNode(child);
        }
        if (index >= m_buffer.firstDirtyBlock()) {
            static_cast<InkBlockNode*>(child)->appendFrom(m_buffer.block(index), m_buffer.baseWidth());
        }
        child = child->nextSibling();
    }
}

void InkItem::updateSoftware(QSGNode*)
{
    // Chemin logiciel : tâche T10 (un QSGRenderNode par bloc). Rien n'est dessiné en logiciel avant elle.
}

} // namespace actcraft::render
```

Dans `render/CMakeLists.txt`, ajouter aux sources `include/actcraft/render/inkitem.h`, `src/inkbuffer.cpp`,
`src/inkitem.cpp`.

- [ ] **Étape 4 : vert, format, commit.** Commande bornée : `tst_render_inkbuffer ... Passed`, `TESTS OK`.

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add render/CMakeLists.txt render/include/actcraft/render/inkbuffer.h render/include/actcraft/render/inkitem.h render/src/inkbuffer.cpp render/src/inkitem.cpp tests/render.cmake tests/tst_render_inkbuffer.cpp
git -C <worktree> commit -m "render : encre incrémentale par blocs de 256 points, chemin RHI et sonde" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

### Tâche T10 — `render` : encre en rendu logiciel (un `QSGRenderNode` par bloc)

**Objectif.** Chemin logiciel de l'élément d'encre (décisions 6 et 9, DA5) : mêmes blocs de 256 points que le
RHI ; un `PainterNode` par bloc, qui garde une copie des points de son bloc et dont le `rect()` est celui de son
bloc ; seul le bloc modifié est marqué sale (`markDirty(DirtyMaterial)`). Un nœud unique réexposerait toute sa
zone à chaque point : coût croissant avec la longueur du trait.

**Le piège.** Parité de deux chemins : la géométrie dessinée en logiciel doit être celle du RHI
(`paintStrokeSegments` reprend `appendStrokeTriangles` : largeur au premier point, bouts carrés) et les deux
chemins partagent `InkBuffer`, les jonctions comprises. Le test exige que le **premier segment** soit toujours
dessiné après 1000 points, soit 4 blocs.

**Fichiers.**
- Créer : `tests/tst_render_ink_software.cpp`
- Modifier : `render/src/inkitem.cpp`, `render/include/actcraft/render/inkitem.h` (si un membre est nécessaire),
  `tests/render.cmake`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_render_ink_software .*(Passed|Failed)' | Select-Object -Last 3
```
Attendu : `out of N` avec N ≥ 11, `tst_render_ink_software ... Passed`, `TESTS OK`.

**Interdits.** Ne pas modifier `InkBuffer` ni le chemin RHI (sinon : hors périmètre, rapporté). Ne pas regrouper
les blocs en un seul nœud logiciel.

**Terminé quand.** Preuve verte : encre visible au premier segment, aux jonctions et au dernier segment après
1000 points ajoutés par paquets avec une image entre chaque ; encre vidée après `clear()` ; commit fait.

**Étapes.**

- [ ] **Étape 1 : test (rouge).** Créer `tests/tst_render_ink_software.cpp` :

```cpp
#include "support/renderhost.h"

#include <actcraft/render/inkitem.h>

#include <QSignalSpy>
#include <QTest>

using namespace actcraft;

namespace {

// 10 rangées de 100 points : x de 50 à 941, y = 100 + 50 × rangée.
core::StrokePoint pointAt(int i)
{
    return {50.0 + 9.0 * (i % 100), 100.0 + 50.0 * (i / 100), 0.5f};
}

QPoint midSegment(int i)
{
    const core::StrokePoint a = pointAt(i);
    const core::StrokePoint b = pointAt(i + 1);
    return QPoint(static_cast<int>((a.x + b.x) / 2), static_cast<int>((a.y + b.y) / 2));
}

} // namespace

class TstRenderInkSoftware : public QObject
{
    Q_OBJECT

private slots:
    void firstSegmentSurvivesFourBlocks()
    {
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(1024, 768);
        auto* ink = new render::InkItem;
        ink->setSize(QSizeF(1024, 768));
        ink->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        ink->beginStroke(0xFF000000u, 4.0);
        QSignalSpy swapped(&window, &QQuickWindow::frameSwapped);
        for (int i = 0; i < 1000; ++i) {
            ink->addPoint(pointAt(i), static_cast<std::uint64_t>(i));
            if (i % 100 == 99) {
                swapped.clear();
                QVERIFY(swapped.wait(2000)); // une image entre deux paquets : mise à jour incrémentale
            }
        }
        QCOMPARE(ink->blockCount(), std::size_t{4});
        const QImage image = window.grabWindow();
        QVERIFY2(colorNear(image.pixelColor(midSegment(0)), QColor(Qt::black), 60), "premier segment");
        QVERIFY2(colorNear(image.pixelColor(midSegment(255)), QColor(Qt::black), 60), "jonction 0-1");
        QVERIFY2(colorNear(image.pixelColor(midSegment(510)), QColor(Qt::black), 60), "jonction 1-2");
        QVERIFY2(colorNear(image.pixelColor(midSegment(997)), QColor(Qt::black), 60), "dernier segment");
        QVERIFY(colorNear(image.pixelColor(QPoint(1000, 700)), QColor(Qt::white), 1));
    }

    void clearRemovesTheInk()
    {
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(400, 300);
        auto* ink = new render::InkItem;
        ink->setSize(QSizeF(400, 300));
        ink->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        ink->beginStroke(0xFF000000u, 6.0);
        ink->addPoint({50, 100, 0.5f}, 0);
        ink->addPoint({350, 100, 0.5f}, 1);
        QVERIFY(colorNear(window.grabWindow().pixelColor(200, 100), QColor(Qt::black), 60));
        ink->clear();
        QCOMPARE(ink->pointCount(), std::size_t{0});
        QVERIFY(colorNear(window.grabWindow().pixelColor(200, 100), QColor(Qt::white), 1));
    }
};

QTEST_MAIN(TstRenderInkSoftware)
#include "tst_render_ink_software.moc"
```

Dans `tests/render.cmake`, ajouter :

```cmake
actcraft_add_test(tst_render_ink_software
    SOURCES tst_render_ink_software.cpp
    LIBS ActCraft::render Qt6::Quick)
```
Commande bornée. Attendu : `tst_render_ink_software ... Failed` (rien n'est dessiné en logiciel).

- [ ] **Étape 2 : chemin logiciel.** Dans `render/src/inkitem.cpp`, ajouter `#include
<actcraft/render/painternode.h>` et, dans l'espace de noms anonyme, après `InkBlockNode` :

```cpp
// Bloc logiciel : copie des points de son bloc, rect() = rectangle du bloc ; seul le bloc modifié est sali.
class InkBlockPainterNode final : public PainterNode
{
public:
    InkBlockPainterNode(QQuickWindow* window, QColor color) : PainterNode(window), m_color(color) {}

    void setBlock(std::span<const core::StrokePoint> points, const core::RectF& bounds, double baseWidth)
    {
        m_points.assign(points.begin(), points.end());
        m_baseWidth = baseWidth;
        setRect(QRectF(bounds.x, bounds.y, bounds.width, bounds.height));
        markDirty(QSGNode::DirtyMaterial);
    }

protected:
    void paint(QPainter& painter) override { paintStrokeSegments(painter, m_points, m_baseWidth, m_color); }

private:
    QColor m_color;
    std::vector<core::StrokePoint> m_points;
    double m_baseWidth = 3.0;
};
```

Remplacer `InkItem::updateSoftware` par :

```cpp
void InkItem::updateSoftware(QSGNode* root)
{
    const std::size_t count = m_buffer.blockCount();
    QSGNode* child = root->firstChild();
    for (std::size_t index = 0; index < count; ++index) {
        if (!child) {
            child = new InkBlockPainterNode(window(), toQColor(m_buffer.color()));
            root->appendChildNode(child);
        }
        if (index >= m_buffer.firstDirtyBlock()) {
            static_cast<InkBlockPainterNode*>(child)->setBlock(m_buffer.block(index), m_buffer.blockBounds(index),
                                                               m_buffer.baseWidth());
        }
        child = child->nextSibling();
    }
}
```

- [ ] **Étape 3 : vert.** Commande bornée : `tst_render_ink_software ... Passed`, `TESTS OK`. Si le premier
segment manque après plusieurs paquets, vérifier d'abord que `firstDirtyBlock()` ne vaut pas 0 à chaque image
(blocs gelés repeints : correct mais coûteux), puis que les nœuds gelés ne sont pas recréés.

- [ ] **Étape 4 : format, preuve, commit.**

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add render/src/inkitem.cpp render/include/actcraft/render/inkitem.h tests/render.cmake tests/tst_render_ink_software.cpp
git -C <worktree> commit -m "render : encre en rendu logiciel, un nœud peint par bloc" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```
(Si `inkitem.h` n'a pas changé, le retirer de `git add`.)

---

### Tâche T11 — `ui` : `PageView` (zoom, défilement, politique du cache)

**Objectif.** Élément C++ `PageView` (exposé au QML, décision 14) qui transforme un élément de contenu (la page) :
zoom Ctrl + molette de 25 % à 800 %, 1,15^(delta/120) ancré au curseur (décision 15, N2-11) ; défilement à la
molette, horizontal par la composante x ou Maj + molette si x est nul (décision 16) ; page bornée dans la vue avec
une marge de 32 px ; mode « Ajuster » à l'ouverture (décision 17). Il porte aussi la politique du cache
(décision 7) : taille de texture = page × zoom × rapport de pixels plafonnée à 4096 px, redessin demandé 150 ms
après la fin d'un geste de zoom (N2-4), tout de suite pour un zoom imposé ; sans cache, « prêt » tout de suite.
Pas de `Flickable` : le glisser appartient au stylo.

**Fichiers.**
- Créer : `ui/include/actcraft/ui/viewtransform.h`, `ui/include/actcraft/ui/pageview.h`,
  `ui/src/viewtransform.cpp`, `ui/src/pageview.cpp`, `tests/tst_viewtransform.cpp`, `tests/tst_pageview.cpp`
- Modifier : `ui/CMakeLists.txt`, `tests/ui.cmake`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_(viewtransform|pageview) .*(Passed|Failed)' | Select-Object -Last 4
```
Attendu : `out of N` avec N ≥ 5, `tst_viewtransform ... Passed`, `tst_pageview ... Passed`, `TESTS OK`.

**Interdits.** Ne pas toucher `ui/qml/Main.qml` (T12). Pas de barres de défilement, pas de défilement cinétique
(N2-9). Ne jamais inverser une composante x déjà présente.

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : tests des calculs (rouges).** Créer `tests/tst_viewtransform.cpp` :

```cpp
#include <actcraft/ui/viewtransform.h>

#include <QTest>

#include <cmath>

using namespace actcraft::ui;

class TstViewTransform : public QObject
{
    Q_OBJECT

private slots:
    void zoomIsClamped()
    {
        QCOMPARE(clampZoom(0.1), 0.25);
        QCOMPARE(clampZoom(20.0), 8.0);
        QCOMPARE(clampZoom(1.5), 1.5);
    }

    void wheelFactorFollowsDelta()
    {
        QVERIFY(qFuzzyCompare(wheelZoomFactor(120), 1.15));
        QVERIFY(qFuzzyCompare(wheelZoomFactor(-120), 1.0 / 1.15));
        QVERIFY(qFuzzyCompare(wheelZoomFactor(60), std::sqrt(1.15)));
    }

    void zoomKeepsAnchor()
    {
        ViewTransform t;
        t.zoom = 2.0;
        t.pan = QPointF(-100, -50);
        const QPointF anchor(300, 200);
        const QPointF before = t.viewToPage(anchor);
        const ViewTransform z = zoomAt(t, anchor, 3.0);
        QCOMPARE(z.zoom, 3.0);
        const QPointF after = z.viewToPage(anchor);
        QVERIFY(qAbs(after.x() - before.x()) < 1e-9 && qAbs(after.y() - before.y()) < 1e-9);
    }

    void fitShowsWholePageCentered()
    {
        const ViewTransform t = fitPage(QSizeF(1280, 800), QSizeF(1024, 768));
        QVERIFY(qFuzzyCompare(t.zoom, (800.0 - 64.0) / 768.0));
        QVERIFY(qFuzzyCompare(t.pan.x(), (1280.0 - 1024.0 * t.zoom) / 2));
        QVERIFY(qFuzzyCompare(t.pan.y(), 32.0));
    }

    void clampCentersSmallPageAndBoundsLargePage()
    {
        ViewTransform small;
        small.zoom = 0.5;
        small.pan = QPointF(-999, 999);
        const ViewTransform s = clampPan(small, QSizeF(1280, 800), QSizeF(1024, 768));
        QCOMPARE(s.pan, QPointF((1280 - 512) / 2.0, (800 - 384) / 2.0));

        ViewTransform big;
        big.zoom = 4.0;
        big.pan = QPointF(500, -99999);
        const ViewTransform b = clampPan(big, QSizeF(1280, 800), QSizeF(1024, 768));
        QCOMPARE(b.pan.x(), 32.0);
        QCOMPARE(b.pan.y(), 800.0 - 3072.0 - 32.0);
    }

    void wheelScrollRules()
    {
        QCOMPARE(wheelScrollDelta(QPoint(0, 120), QPoint(), Qt::NoModifier), QPointF(0, 60));
        QCOMPARE(wheelScrollDelta(QPoint(0, -120), QPoint(), Qt::ShiftModifier), QPointF(-60, 0));
        QCOMPARE(wheelScrollDelta(QPoint(120, 0), QPoint(), Qt::ShiftModifier), QPointF(60, 0));
        QCOMPARE(wheelScrollDelta(QPoint(0, 120), QPoint(0, 7), Qt::NoModifier), QPointF(0, 7));
    }

    void cacheTextureSizeIsCapped()
    {
        QCOMPARE(cacheTextureSize(QSizeF(1024, 768), 1.0, 1.0), QSize(1024, 768));
        QCOMPARE(cacheTextureSize(QSizeF(1024, 768), 4.0, 1.0), QSize(4096, 3072));
        QCOMPARE(cacheTextureSize(QSizeF(1024, 768), 8.0, 1.0), QSize(4096, 3072));
        QCOMPARE(cacheTextureSize(QSizeF(1024, 768), 0.25, 1.0), QSize(256, 192));
        QCOMPARE(cacheTextureSize(QSizeF(1024, 768), 1.0, 2.0), QSize(2048, 1536));
        QCOMPARE(cacheTextureSize(QSizeF(1024, 768), 3.0, 2.0), QSize(4096, 3072));
    }
};

QTEST_GUILESS_MAIN(TstViewTransform)
#include "tst_viewtransform.moc"
```

- [ ] **Étape 2 : tests de l'élément (rouges).** Créer `tests/tst_pageview.cpp` :

```cpp
#include <actcraft/ui/pageview.h>

#include <QPointingDevice>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>
#include <QWheelEvent>

using namespace actcraft::ui;

namespace {

struct Fixture
{
    QQuickWindow window;
    PageView* view = nullptr;
    QQuickItem* content = nullptr;
};

bool open(Fixture& f)
{
    f.window.resize(1280, 800);
    f.view = new PageView;
    f.view->setParentItem(f.window.contentItem());
    f.view->setSize(QSizeF(1280, 800));
    f.content = new QQuickItem;
    f.content->setSize(QSizeF(1024, 768));
    f.content->setParentItem(f.view);
    f.view->setContent(f.content);
    f.window.show();
    if (!QTest::qWaitForWindowExposed(&f.window)) {
        return false;
    }
    // Pas de QML ici : le test termine lui-même les redessins demandés à l'ouverture (chaque fin peut en
    // relancer un, d'où la boucle bornée).
    for (int i = 0; i < 5 && !f.view->cacheReady(); ++i) {
        f.view->notifyCacheUpdated();
    }
    return f.view->cacheReady();
}

void wheel(Fixture& f, QPointF pos, QPoint angle, Qt::KeyboardModifiers modifiers,
           const QPointingDevice* device = QPointingDevice::primaryPointingDevice())
{
    QWheelEvent event(pos, f.window.mapToGlobal(pos), QPoint(), angle, Qt::NoButton, modifiers, Qt::NoScrollPhase,
                      false, Qt::MouseEventNotSynthesized, device);
    QCoreApplication::sendEvent(&f.window, &event);
}

} // namespace

class TstPageView : public QObject
{
    Q_OBJECT

private slots:
    void startsInFitMode()
    {
        Fixture f;
        QVERIFY(open(f));
        QVERIFY(qFuzzyCompare(f.view->zoom(), (800.0 - 64.0) / 768.0));
        QVERIFY(qFuzzyCompare(f.content->scale(), f.view->zoom()));
        QVERIFY(qFuzzyCompare(f.content->x(), (1280.0 - 1024.0 * f.view->zoom()) / 2));
    }

    void ctrlWheelZoomsAtCursor()
    {
        Fixture f;
        QVERIFY(open(f));
        f.view->setZoom(2.0);
        const QPointF anchor(300, 200);
        const QPointF before = f.view->viewTransform().viewToPage(anchor);
        wheel(f, anchor, QPoint(0, 120), Qt::ControlModifier);
        QVERIFY(qFuzzyCompare(f.view->zoom(), 2.0 * 1.15));
        const QPointF after = f.view->viewTransform().viewToPage(anchor);
        QVERIFY(qAbs(after.x() - before.x()) < 0.01 && qAbs(after.y() - before.y()) < 0.01);
    }

    void zoomStaysWithinLimits()
    {
        Fixture f;
        QVERIFY(open(f));
        for (int i = 0; i < 40; ++i) {
            wheel(f, QPointF(640, 400), QPoint(0, 120), Qt::ControlModifier);
        }
        QCOMPARE(f.view->zoom(), 8.0);
        for (int i = 0; i < 80; ++i) {
            wheel(f, QPointF(640, 400), QPoint(0, -120), Qt::ControlModifier);
        }
        QCOMPARE(f.view->zoom(), 0.25);
    }

    void wheelScrollsVerticallyThenHorizontallyWithShift()
    {
        Fixture f;
        QVERIFY(open(f));
        f.view->setZoom(4.0);
        f.view->setPan(QPointF(32, 32));
        QCOMPARE(f.view->pan(), QPointF(32, 32));
        wheel(f, QPointF(640, 400), QPoint(0, -120), Qt::NoModifier);
        QCOMPARE(f.view->pan(), QPointF(32, -28));
        wheel(f, QPointF(640, 400), QPoint(0, -120), Qt::ShiftModifier);
        QCOMPARE(f.view->pan(), QPointF(-28, -28));
    }

    void zoomGestureRefreshesCacheAfterDelay()
    {
        Fixture f;
        QVERIFY(open(f));
        QSignalSpy refresh(f.view, &PageView::cacheRefreshRequested);
        f.view->setZoom(2.0); // zoom imposé : tout de suite
        QCOMPARE(refresh.count(), 1);
        f.view->notifyCacheUpdated();
        QVERIFY(f.view->cacheReady());
        refresh.clear();
        wheel(f, QPointF(640, 400), QPoint(0, 120), Qt::ControlModifier);
        QCOMPARE(refresh.count(), 0);
        QVERIFY(!f.view->cacheReady());
        QTRY_COMPARE_WITH_TIMEOUT(refresh.count(), 1, 1000);
        QCOMPARE(f.view->cacheTextureSize(), cacheTextureSize(QSizeF(1024, 768), f.view->zoom(),
                                                              f.window.effectiveDevicePixelRatio()));
    }

    void overlappingRequestsWaitForTheLastOne()
    {
        Fixture f;
        QVERIFY(open(f));
        QSignalSpy refresh(f.view, &PageView::cacheRefreshRequested);
        f.view->requestCacheRefresh();
        f.view->requestCacheRefresh(); // pendant le premier redessin : émise tout de suite quand même
        QCOMPARE(refresh.count(), 2);
        f.view->notifyCacheUpdated(); // fin d'un redessin peut-être antérieur à la seconde demande
        QVERIFY(!f.view->cacheReady());
        QCOMPARE(refresh.count(), 3);  // un redessin de plus
        f.view->notifyCacheUpdated();
        QVERIFY(f.view->cacheReady());
        QCOMPARE(refresh.count(), 3);
    }

    void cacheUpdatedSignalMakesReady()
    {
        Fixture f;
        QVERIFY(open(f));
        QSignalSpy updated(f.view, &PageView::cacheUpdated);
        f.view->requestCacheRefresh();
        QVERIFY(!f.view->cacheReady());
        f.view->notifyCacheUpdated();
        QVERIFY(f.view->cacheReady());
        QCOMPARE(updated.count(), 1);
    }

    void withoutCacheReadyAtOnce()
    {
        Fixture f;
        QVERIFY(open(f));
        f.view->setCacheEnabled(false);
        QSignalSpy refresh(f.view, &PageView::cacheRefreshRequested);
        QSignalSpy updated(f.view, &PageView::cacheUpdated);
        f.view->setZoom(3.0);
        QCOMPARE(refresh.count(), 0);
        QCOMPARE(updated.count(), 1);
        QVERIFY(f.view->cacheReady());
    }

    void exclusiveDevicesFilterTheWheel()
    {
        Fixture f;
        QVERIFY(open(f));
        QPointingDevice synthetic(QStringLiteral("test-wheel"), 991, QInputDevice::DeviceType::Mouse,
                                  QPointingDevice::PointerType::Generic, QInputDevice::Capability::Position, 1, 3);
        f.view->setExclusiveDevices({&synthetic});
        const qreal before = f.view->zoom();
        wheel(f, QPointF(640, 400), QPoint(0, 120), Qt::ControlModifier);
        QCOMPARE(f.view->zoom(), before);
        wheel(f, QPointF(640, 400), QPoint(0, 120), Qt::ControlModifier, &synthetic);
        QVERIFY(f.view->zoom() > before);
        f.view->setExclusiveDevices({});
    }
};

QTEST_MAIN(TstPageView)
#include "tst_pageview.moc"
```

Dans `tests/ui.cmake` :

```cmake
actcraft_add_test(tst_viewtransform
    SOURCES tst_viewtransform.cpp
    LIBS ActCraft::ui)

actcraft_add_test(tst_pageview
    SOURCES tst_pageview.cpp
    LIBS ActCraft::ui Qt6::Quick)
```
Commande bornée : `TESTS EN ECHEC (compilation)`.

- [ ] **Étape 3 : calculs.** Créer `ui/include/actcraft/ui/viewtransform.h` :

```cpp
#pragma once

#include <QPoint>
#include <QPointF>
#include <QSize>
#include <QSizeF>
#include <Qt>

namespace actcraft::ui {

inline constexpr double kMinZoom = 0.25;
inline constexpr double kMaxZoom = 8.0;
inline constexpr double kZoomStep = 1.15;             // par cran de molette (décision 15)
inline constexpr double kViewMargin = 32.0;           // marge de la page bornée dans la vue (décision 16)
inline constexpr double kScrollPixelsPerNotch = 60.0; // défilement par cran
inline constexpr int kCacheRefreshDelayMs = 150;      // N2-4
inline constexpr int kMaxCacheSide = 4096;            // N1-6

// Vue → page : page = (vue − pan) / zoom.
struct ViewTransform
{
    double zoom = 1.0;
    QPointF pan; // position de l'origine de la page dans la vue

    QPointF pageToView(QPointF p) const { return pan + p * zoom; }
    QPointF viewToPage(QPointF v) const { return (v - pan) / zoom; }
};

double clampZoom(double zoom) noexcept;
// 1,15^(delta/120) : tient compte des molettes fines (N2-11).
double wheelZoomFactor(int angleDeltaY) noexcept;
// Le point `anchor` de la vue reste sur le même point de page.
ViewTransform zoomAt(const ViewTransform& t, QPointF anchor, double newZoom);
// Page plus petite que la vue (marges comprises) : centrée sur cet axe ; sinon bornée à la marge.
ViewTransform clampPan(const ViewTransform& t, QSizeF view, QSizeF page, double margin = kViewMargin);
// « Ajuster » : page entière visible, centrée.
ViewTransform fitPage(QSizeF view, QSizeF page, double margin = kViewMargin);
// Déplacement de la page (pixels de vue) pour une molette sans Ctrl (décision 16).
QPointF wheelScrollDelta(QPoint angleDelta, QPoint pixelDelta, Qt::KeyboardModifiers modifiers);
// Taille de la texture du cache : page × zoom × rapport de pixels, plus grand côté plafonné (décision 7).
QSize cacheTextureSize(QSizeF page, double zoom, double devicePixelRatio, int maxSide = kMaxCacheSide);

} // namespace actcraft::ui
```

Créer `ui/src/viewtransform.cpp` :

```cpp
#include <actcraft/ui/viewtransform.h>

#include <algorithm>
#include <cmath>

namespace actcraft::ui {

double clampZoom(double zoom) noexcept
{
    return std::clamp(zoom, kMinZoom, kMaxZoom);
}

double wheelZoomFactor(int angleDeltaY) noexcept
{
    return std::pow(kZoomStep, angleDeltaY / 120.0);
}

ViewTransform zoomAt(const ViewTransform& t, QPointF anchor, double newZoom)
{
    const QPointF page = t.viewToPage(anchor);
    ViewTransform result;
    result.zoom = clampZoom(newZoom);
    result.pan = anchor - page * result.zoom;
    return result;
}

ViewTransform clampPan(const ViewTransform& t, QSizeF view, QSizeF page, double margin)
{
    const auto axis = [margin](double pan, double viewLength, double pageLength) {
        if (pageLength + 2 * margin <= viewLength) {
            return (viewLength - pageLength) / 2;
        }
        return std::clamp(pan, viewLength - pageLength - margin, margin);
    };
    ViewTransform result = t;
    result.pan.setX(axis(t.pan.x(), view.width(), page.width() * t.zoom));
    result.pan.setY(axis(t.pan.y(), view.height(), page.height() * t.zoom));
    return result;
}

ViewTransform fitPage(QSizeF view, QSizeF page, double margin)
{
    ViewTransform result;
    result.zoom = clampZoom(
        std::min((view.width() - 2 * margin) / page.width(), (view.height() - 2 * margin) / page.height()));
    result.pan = QPointF((view.width() - page.width() * result.zoom) / 2,
                         (view.height() - page.height() * result.zoom) / 2);
    return result;
}

QPointF wheelScrollDelta(QPoint angleDelta, QPoint pixelDelta, Qt::KeyboardModifiers modifiers)
{
    QPointF delta = !pixelDelta.isNull() ? QPointF(pixelDelta) : QPointF(angleDelta) * (kScrollPixelsPerNotch / 120.0);
    // Maj + molette = horizontal, seulement si la molette n'a pas déjà une composante x (macOS la donne).
    if (delta.x() == 0 && (modifiers & Qt::ShiftModifier)) {
        delta = QPointF(delta.y(), 0);
    }
    return delta;
}

QSize cacheTextureSize(QSizeF page, double zoom, double devicePixelRatio, int maxSide)
{
    double w = std::ceil(page.width() * zoom * devicePixelRatio);
    double h = std::ceil(page.height() * zoom * devicePixelRatio);
    const double longest = std::max(w, h);
    if (longest > maxSide) {
        const double scale = maxSide / longest;
        w = std::min<double>(maxSide, std::round(w * scale));
        h = std::min<double>(maxSide, std::round(h * scale));
    }
    return QSize(std::max(1, static_cast<int>(w)), std::max(1, static_cast<int>(h)));
}

} // namespace actcraft::ui
```

- [ ] **Étape 4 : élément.** Créer `ui/include/actcraft/ui/pageview.h` :

```cpp
#pragma once

#include <actcraft/ui/viewtransform.h>

#include <QList>
#include <QQuickItem>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class QInputDevice;

namespace actcraft::ui {

// Vue de la page (décision 14) : zoom, défilement, politique du cache. Transforme l'élément `content`.
class PageView : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQuickItem* content READ content WRITE setContent NOTIFY contentChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(QPointF pan READ pan WRITE setPan NOTIFY panChanged)
    Q_PROPERTY(bool cacheEnabled READ cacheEnabled WRITE setCacheEnabled NOTIFY cacheEnabledChanged)
    Q_PROPERTY(QSize cacheTextureSize READ cacheTextureSize NOTIFY cacheTextureSizeChanged)
    Q_PROPERTY(bool cacheReady READ cacheReady NOTIFY cacheReadyChanged)

public:
    explicit PageView(QQuickItem* parent = nullptr);

    QQuickItem* content() const noexcept;
    void setContent(QQuickItem* content);
    qreal zoom() const noexcept;
    // Zoom imposé (banc, programme) : ancré au centre de la vue, quitte « Ajuster », cache redessiné tout de suite.
    void setZoom(qreal zoom);
    QPointF pan() const noexcept;
    void setPan(QPointF pan);
    bool cacheEnabled() const noexcept;
    void setCacheEnabled(bool enabled);
    QSize cacheTextureSize() const noexcept;
    // Vrai quand la texture du cache correspond à la page et au zoom courants (toujours vrai sans cache).
    bool cacheReady() const noexcept;
    const ViewTransform& viewTransform() const noexcept;

    Q_INVOKABLE void fitPage();
    // Demande un redessin du cache (QML : cache.scheduleUpdate()) ; sans cache, « prêt » tout de suite.
    // La demande part toujours tout de suite (un trait figé n'attend pas, décision 10). Si elle arrive pendant
    // un redessin, la fin de celui-ci en relance un autre : cacheReady ne redevient vrai qu'après un redessin
    // commencé après la dernière demande. Une demande perdue (QML pas encore branché) est rattrapée par la
    // suivante.
    Q_INVOKABLE void requestCacheRefresh();
    // QML : appelé sur ShaderEffectSource.scheduledUpdateCompleted.
    Q_INVOKABLE void notifyCacheUpdated();
    Q_INVOKABLE QPointF pageToScene(QPointF pagePoint) const;
    // Pendant un scénario du banc, seuls ces périphériques sont écoutés ; liste vide = tous.
    void setExclusiveDevices(QList<const QInputDevice*> devices);

signals:
    void contentChanged();
    void zoomChanged();
    void panChanged();
    void cacheEnabledChanged();
    void cacheTextureSizeChanged();
    void cacheReadyChanged();
    void cacheRefreshRequested();
    void cacheUpdated();

protected:
    void wheelEvent(QWheelEvent* event) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    bool acceptsDevice(const QInputDevice* device) const;

private:
    QSizeF pageSize() const;
    void apply(const ViewTransform& transform);
    void updateCacheTextureSize();
    void setCacheReady(bool ready);

    QQuickItem* m_content = nullptr;
    ViewTransform m_transform;
    bool m_fitMode = true;
    bool m_cacheEnabled = true;
    bool m_cacheReady = false;
    bool m_refreshInFlight = false; // un redessin du cache est demandé et pas encore terminé
    bool m_refreshAgain = false;    // une demande est arrivée pendant ce redessin : en refaire un à sa fin
    QSize m_cacheTextureSize;
    QTimer m_cacheTimer;
    QList<const QInputDevice*> m_exclusive;
};

} // namespace actcraft::ui
```

Créer `ui/src/pageview.cpp` :

```cpp
#include <actcraft/ui/pageview.h>

#include <QQuickWindow>
#include <QWheelEvent>

namespace actcraft::ui {

PageView::PageView(QQuickItem* parent) : QQuickItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    m_cacheTimer.setSingleShot(true);
    m_cacheTimer.setInterval(kCacheRefreshDelayMs);
    connect(&m_cacheTimer, &QTimer::timeout, this, [this] {
        updateCacheTextureSize();
        requestCacheRefresh();
    });
}

QQuickItem* PageView::content() const noexcept
{
    return m_content;
}

void PageView::setContent(QQuickItem* content)
{
    if (content == m_content) {
        return;
    }
    m_content = content;
    if (m_content) {
        m_content->setTransformOrigin(QQuickItem::TopLeft);
    }
    if (m_fitMode) {
        fitPage();
    } else {
        apply(clampPan(m_transform, size(), pageSize()));
    }
    emit contentChanged();
}

qreal PageView::zoom() const noexcept
{
    return m_transform.zoom;
}

void PageView::setZoom(qreal zoom)
{
    m_fitMode = false;
    m_cacheTimer.stop();
    const ViewTransform t = clampPan(zoomAt(m_transform, QPointF(width() / 2, height() / 2), zoom), size(), pageSize());
    const bool changed = !qFuzzyCompare(t.zoom, m_transform.zoom);
    apply(t);
    if (changed) {
        updateCacheTextureSize();
        requestCacheRefresh();
    }
}

QPointF PageView::pan() const noexcept
{
    return m_transform.pan;
}

void PageView::setPan(QPointF pan)
{
    m_fitMode = false;
    ViewTransform t = m_transform;
    t.pan = pan;
    apply(clampPan(t, size(), pageSize()));
}

bool PageView::cacheEnabled() const noexcept
{
    return m_cacheEnabled;
}

void PageView::setCacheEnabled(bool enabled)
{
    if (enabled == m_cacheEnabled) {
        return;
    }
    m_cacheEnabled = enabled;
    emit cacheEnabledChanged();
    requestCacheRefresh();
}

QSize PageView::cacheTextureSize() const noexcept
{
    return m_cacheTextureSize;
}

bool PageView::cacheReady() const noexcept
{
    return m_cacheReady;
}

const ViewTransform& PageView::viewTransform() const noexcept
{
    return m_transform;
}

void PageView::fitPage()
{
    m_fitMode = true;
    m_cacheTimer.stop();
    const ViewTransform t = ui::fitPage(size(), pageSize());
    const bool changed = !qFuzzyCompare(t.zoom, m_transform.zoom) || m_cacheTextureSize.isEmpty();
    apply(t);
    if (changed) {
        updateCacheTextureSize();
        requestCacheRefresh();
    }
}

void PageView::requestCacheRefresh()
{
    if (!m_cacheEnabled) {
        m_refreshInFlight = false;
        m_refreshAgain = false;
        setCacheReady(true);
        emit cacheUpdated();
        return;
    }
    setCacheReady(false);
    if (m_refreshInFlight) {
        m_refreshAgain = true; // la fin du redessin en cours ne suffira pas
    }
    m_refreshInFlight = true;
    emit cacheRefreshRequested(); // toujours tout de suite (décision 10)
}

void PageView::notifyCacheUpdated()
{
    if (m_refreshAgain) {
        // Ce redessin a pu commencer avant la dernière demande : un de plus garantit l'état final.
        m_refreshAgain = false;
        emit cacheRefreshRequested();
        return;
    }
    m_refreshInFlight = false;
    setCacheReady(true);
    emit cacheUpdated();
}

QPointF PageView::pageToScene(QPointF pagePoint) const
{
    return mapToScene(m_transform.pageToView(pagePoint));
}

void PageView::setExclusiveDevices(QList<const QInputDevice*> devices)
{
    m_exclusive = std::move(devices);
}

bool PageView::acceptsDevice(const QInputDevice* device) const
{
    return m_exclusive.isEmpty() || m_exclusive.contains(device);
}

void PageView::wheelEvent(QWheelEvent* event)
{
    if (!acceptsDevice(event->pointingDevice())) {
        event->ignore();
        return;
    }
    m_fitMode = false;
    if (event->modifiers() & Qt::ControlModifier) {
        const int delta = event->angleDelta().y();
        if (delta != 0) {
            const ViewTransform t = clampPan(
                zoomAt(m_transform, event->position(), m_transform.zoom * wheelZoomFactor(delta)), size(), pageSize());
            const bool changed = !qFuzzyCompare(t.zoom, m_transform.zoom);
            apply(t);
            if (changed) {
                setCacheReady(false);
                m_cacheTimer.start(); // redessin 150 ms après le dernier cran
            }
        }
    } else {
        ViewTransform t = m_transform;
        t.pan += wheelScrollDelta(event->angleDelta(), event->pixelDelta(), event->modifiers());
        apply(clampPan(t, size(), pageSize()));
    }
    event->accept();
}

void PageView::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (m_fitMode) {
        fitPage();
    } else {
        apply(clampPan(m_transform, size(), pageSize()));
    }
}

QSizeF PageView::pageSize() const
{
    return m_content ? QSizeF(m_content->width(), m_content->height()) : QSizeF(1024, 768);
}

void PageView::apply(const ViewTransform& transform)
{
    const bool zoomChangedNow = !qFuzzyCompare(transform.zoom, m_transform.zoom);
    const bool panChangedNow = transform.pan != m_transform.pan;
    m_transform = transform;
    if (m_content) {
        m_content->setScale(transform.zoom);
        m_content->setPosition(transform.pan);
    }
    if (zoomChangedNow) {
        emit zoomChanged();
    }
    if (panChangedNow) {
        emit panChanged();
    }
}

void PageView::updateCacheTextureSize()
{
    const qreal dpr = window() ? window()->effectiveDevicePixelRatio() : 1.0;
    const QSize s = ui::cacheTextureSize(pageSize(), m_transform.zoom, dpr);
    if (s != m_cacheTextureSize) {
        m_cacheTextureSize = s;
        emit cacheTextureSizeChanged();
    }
}

void PageView::setCacheReady(bool ready)
{
    if (ready != m_cacheReady) {
        m_cacheReady = ready;
        emit cacheReadyChanged();
    }
}

} // namespace actcraft::ui
```

Remplacer `ui/CMakeLists.txt` par :

```cmake
# Interface QML : boîte à outils, navigateurs, menus, thèmes.
qt_add_library(actcraft_ui STATIC)
add_library(ActCraft::ui ALIAS actcraft_ui)

qt_add_qml_module(actcraft_ui
    URI ActCraft.Ui
    VERSION 1.0
    QML_FILES
        qml/Main.qml
    SOURCES
        include/actcraft/ui/viewtransform.h
        include/actcraft/ui/pageview.h
        src/viewtransform.cpp
        src/pageview.cpp)

# Le code d'enregistrement QML généré inclut les en-têtes par leur seul nom : d'où le dossier privé.
target_include_directories(actcraft_ui
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/actcraft/ui)
target_link_libraries(actcraft_ui PUBLIC Qt6::Quick ActCraft::actions)
actcraft_set_warnings(actcraft_ui)
```

- [ ] **Étape 5 : vert, format, commit.** Commande bornée : `tst_viewtransform ... Passed`,
`tst_pageview ... Passed`, `TESTS OK` (et `tst_ui` toujours vert).

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add ui/CMakeLists.txt ui/include/actcraft/ui/viewtransform.h ui/include/actcraft/ui/pageview.h ui/src/viewtransform.cpp ui/src/pageview.cpp tests/ui.cmake tests/tst_viewtransform.cpp tests/tst_pageview.cpp
git -C <worktree> commit -m "ui : vue de page (zoom ancré, défilement borné, politique du cache)" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

### Tâche T12 — `ui` + `app` : `Main.qml`, câblage stylo / encre / cache, `--sans-cache`

**Objectif.** Assembler le prototype : `Main.qml` montre la page (élément `page`, 1024 × 768) dans un `PageView`,
avec `FixedPage` (page fixe), le cache `ShaderEffectSource` (`live: false`, `hideSource`, `textureSize` venu de
la vue, `scheduleUpdate()` sur demande, `scheduledUpdateCompleted` → `notifyCacheUpdated()`, décision 7) et
`InkLayer` (trait en cours) au-dessus. `PageView` reçoit le stylet et la souris (bouton gauche), les convertit
par la chaîne d'entrée (T7) et les donne au `PenTool` (T8) par deux adaptateurs (`ui/src/penadapters.*`). Un
trait figé est versé à la page fixe, l'encre se vide et le cache est redessiné, dans le même tour de boucle
(décision 10). Option `--sans-cache` de l'application (décision 8) ; chargement commun dans `app/startup.*`
(DA1, repris par le banc en T16). Nom du rendu dans le titre, aucun texte dans la scène (décision 19).

**Préalable : lire `docs/verifications/2026-10-13-qt612.md`.**
- `tablette-element: oui` → **variante A** (le stylet arrive à `PageView::event()`).
- `tablette-element: non` → **variante B** (filtre d'événements sur la fenêtre).
- `couche-logicielle: oui` **et** `couche-rendernode-logiciel: oui` → **variante A du cache** (cache dans les
  deux rendus).
- l'une des deux à `non` → **variante B du cache** (cache coupé en rendu logiciel).
Le rapport nomme les variantes appliquées.

**Fichiers.**
- Créer : `ui/src/uiforeigntypes.h`, `ui/src/penadapters.h`, `ui/src/penadapters.cpp`, `app/startup.h`,
  `app/startup.cpp`, `tests/tst_ui_drawing.cpp`
- Modifier : `ui/CMakeLists.txt`, `ui/qml/Main.qml`, `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`,
  `app/main.cpp`, `app/CMakeLists.txt`, `tests/ui.cmake`, `tests/tst_ui.cpp`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_ui(_drawing)? .*(Passed|Failed)' | Select-Object -Last 4
```
Attendu : `out of N` avec N ≥ 16, `tst_ui ... Passed`, `tst_ui_drawing ... Passed`, `TESTS OK`.

**Preuve visuelle (facultative, fenêtre de 1280 × 800, pas de plein écran).** Le banc (T16-T17) produira les
captures ; ici, seulement si la tâche en a besoin pour comprendre un échec :
`build/debug/actcraft.exe --rendu logiciel --quitter-apres 4000` doit afficher la page aux 300 objets, sans
texte, titre « ActCraft — rendu : logiciel ». Ne pas piloter la souris.

**Interdits.** Aucun élément QML par objet ; pas de `Flickable` ; aucune commande nommée, bouton ni menu
(décision 18) ; aucun texte dans la scène ; aucun compteur d'images à l'écran. `tools/` ne doit pas connaître
`render/` : les adaptateurs vivent dans `ui/`. Ne jamais activer les deux compressions d'événements.

**Terminé quand.** Preuve verte : `tst_ui` (fenêtre, `page` 1024 × 768, page prête, cache prêt, rendu dans le
titre) et `tst_ui_drawing` (trait au stylet visible pendant le geste, toujours visible une fois figé, avec et sans
cache, trait à la souris, option `--sans-cache`) ; commit fait.

**Étapes.**

- [ ] **Étape 1 : tests (rouges).** Remplacer le corps de `TstUi::mainWindowLoadsWithSoftwareRenderer` dans
`tests/tst_ui.cpp` par (et ajouter les `#include <actcraft/render/fixedpageitem.h>` et
`#include <actcraft/ui/pageview.h>`) :

```cpp
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
        QCOMPARE(fixed->objectCount(), 300);
        QVERIFY(window->title().contains(QStringLiteral("logiciel")));
```

Créer `tests/tst_ui_drawing.cpp` :

```cpp
#include "startup.h"
#include "support/renderhost.h"

#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/ui/pageview.h>

#include <QElapsedTimer>
#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTabletEvent>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

struct App
{
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
    render::FixedPageItem* fixed = nullptr;
    render::InkItem* ink = nullptr;
};

bool start(App& app, bool cacheEnabled)
{
    StartupOptions options;
    options.cacheEnabled = cacheEnabled;
    app.window = loadMainWindow(app.engine, options);
    if (!app.window || !QTest::qWaitForWindowExposed(app.window)) {
        return false;
    }
    app.view = app.window->findChild<ui::PageView*>(QStringLiteral("pageView"));
    app.fixed = app.window->findChild<render::FixedPageItem*>(QStringLiteral("fixedPage"));
    app.ink = app.window->findChild<render::InkItem*>(QStringLiteral("ink"));
    if (!app.view || !app.fixed || !app.ink) {
        return false;
    }
    return QTest::qWaitFor([&] { return app.fixed->isReady() && app.view->cacheReady(); }, 20000);
}

QColor pixelAtPage(App& app, QPointF pagePoint)
{
    const QImage image = app.window->grabWindow();
    const QPointF scene = app.view->pageToScene(pagePoint) * app.window->effectiveDevicePixelRatio();
    return image.pixelColor(scene.toPoint());
}

} // namespace

class TstUiDrawing : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QPointingDevice> m_stylus;

    void post(App& app, QEvent::Type type, QPointF page, qreal pressure, Qt::MouseButton button,
              Qt::MouseButtons buttons)
    {
        const QPointF scene = app.view->pageToScene(page);
        QCoreApplication::postEvent(app.window,
                                    new QTabletEvent(type, m_stylus.get(), scene, app.window->mapToGlobal(scene),
                                                     pressure, 0, 0, 0, 0, 0, Qt::NoModifier, button, buttons));
    }

    void strokeIsDrawnThenFrozen(bool cacheEnabled)
    {
        App app;
        QVERIFY(start(app, cacheEnabled));
        QCOMPARE(app.view->cacheEnabled(), cacheEnabled);
        QSignalSpy frozen(app.view, &ui::PageView::strokeFrozen);
        post(app, QEvent::TabletPress, QPointF(300, 670), 0.7, Qt::LeftButton, Qt::LeftButton);
        for (int i = 1; i <= 100; ++i) {
            post(app, QEvent::TabletMove, QPointF(300 + 2.0 * i, 670), 0.7, Qt::NoButton, Qt::LeftButton);
        }
        QTRY_VERIFY_WITH_TIMEOUT(app.ink->pointCount() >= 100, 3000);
        QVERIFY2(colorNear(pixelAtPage(app, QPointF(350, 670)), QColor(Qt::black), 60), "encre en cours");

        // Décision 10 : entre le relâché et le cache redessiné, aucune image ne doit perdre le trait
        // (encre vidée et trait versé à la page fixe dans le même tour de boucle).
        // Limite : grabWindow() synchronise et dessine sa propre image ; on vérifie l'état de la scène après
        // chaque lot d'événements traité, pas l'image réellement présentée à l'écran.
        QSignalSpy swapped(app.window, &QQuickWindow::frameSwapped);
        post(app, QEvent::TabletRelease, QPointF(500, 670), 0.0, Qt::LeftButton, Qt::NoButton);
        QElapsedTimer clock;
        clock.start();
        int checkedFrames = 0;
        while (!(frozen.count() == 1 && app.view->cacheReady()) && clock.elapsed() < 3000) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            if (frozen.count() == 1 && !swapped.isEmpty()) {
                swapped.clear();
                ++checkedFrames;
                QVERIFY2(colorNear(pixelAtPage(app, QPointF(350, 670)), QColor(Qt::black), 60),
                         "trait absent entre le relâché et le cache prêt");
            }
        }
        QCOMPARE(frozen.count(), 1);
        QVERIFY(app.view->cacheReady());
        qInfo("images contrôlées entre relâché et cache prêt : %d", checkedFrames);
        QCOMPARE(app.ink->pointCount(), std::size_t{0});
        QCOMPARE(app.fixed->objectCount(), 301);
        QVERIFY2(colorNear(pixelAtPage(app, QPointF(350, 670)), QColor(Qt::black), 60), "trait figé");
    }

private slots:
    void initTestCase()
    {
        m_stylus = std::make_unique<QPointingDevice>(
            QStringLiteral("tst-ui-stylus"), 5150, QInputDevice::DeviceType::Stylus,
            QPointingDevice::PointerType::Pen, QInputDevice::Capability::Position | QInputDevice::Capability::Pressure,
            1, 3);
    }

    void sansCacheOptionIsParsed()
    {
        QVERIFY(parseStartupOptions({QStringLiteral("actcraft")}).cacheEnabled);
        QVERIFY(!parseStartupOptions({QStringLiteral("actcraft"), QStringLiteral("--sans-cache")}).cacheEnabled);
    }

    void stylusStrokeWithCache() { strokeIsDrawnThenFrozen(true); }

    void stylusStrokeWithoutCache() { strokeIsDrawnThenFrozen(false); }

    void mouseLeftButtonDraws()
    {
        App app;
        QVERIFY(start(app, true));
        const QPoint a = app.view->pageToScene(QPointF(300, 640)).toPoint();
        const QPoint b = app.view->pageToScene(QPointF(500, 640)).toPoint();
        QTest::mousePress(app.window, Qt::LeftButton, Qt::NoModifier, a);
        for (int i = 1; i <= 10; ++i) {
            QTest::mouseMove(app.window, a + (b - a) * i / 10);
        }
        QTest::mouseRelease(app.window, Qt::LeftButton, Qt::NoModifier, b);
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->objectCount(), 301, 3000);
    }
};

QTEST_MAIN(TstUiDrawing)
#include "tst_ui_drawing.moc"
```

Dans `tests/ui.cmake`, ajouter :

```cmake
actcraft_add_test(tst_ui_drawing
    SOURCES tst_ui_drawing.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_drawing PRIVATE ${PROJECT_SOURCE_DIR}/app)
```
Commande bornée : `TESTS EN ECHEC (compilation)`.

- [ ] **Étape 2 : démarrage commun.** Créer `app/startup.h` :

```cpp
#pragma once

#include <QStringList>

class QQmlApplicationEngine;
class QQuickWindow;

// Chargement commun de l'interface, pour l'application et le banc (DA1).
struct StartupOptions
{
    bool cacheEnabled = true;
};

// Lit `--sans-cache` (décision 8).
StartupOptions parseStartupOptions(const QStringList& args);

// Charge ActCraft.Ui/Main avec ces options ; retourne la fenêtre principale, nullptr en cas d'échec.
QQuickWindow* loadMainWindow(QQmlApplicationEngine& engine, const StartupOptions& options);
```

Créer `app/startup.cpp` :

```cpp
#include "startup.h"

#include <QQmlApplicationEngine>
#include <QQuickWindow>

StartupOptions parseStartupOptions(const QStringList& args)
{
    StartupOptions options;
    options.cacheEnabled = !args.contains(QStringLiteral("--sans-cache"));
    return options;
}

QQuickWindow* loadMainWindow(QQmlApplicationEngine& engine, const StartupOptions& options)
{
    engine.setInitialProperties({{QStringLiteral("cacheEnabled"), options.cacheEnabled}});
    engine.loadFromModule("ActCraft.Ui", "Main");
    if (engine.rootObjects().isEmpty()) {
        return nullptr;
    }
    return qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
}
```

Dans `app/main.cpp`, ajouter `#include "startup.h"` et remplacer la ligne
`engine.loadFromModule("ActCraft.Ui", "Main");` par :

```cpp
    if (!loadMainWindow(engine, parseStartupOptions(args))) {
        return 1;
    }
```
Dans `app/CMakeLists.txt`, ajouter `startup.cpp` et `startup.h` à `qt_add_executable(actcraft …)`.

- [ ] **Étape 3 : types QML de `render/` et adaptateurs.** Créer `ui/src/uiforeigntypes.h` :

```cpp
#pragma once

#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>

#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// render/ n'est pas un module QML : ses éléments y sont exposés ici.
struct FixedPageForeign
{
    Q_GADGET
    QML_FOREIGN(actcraft::render::FixedPageItem)
    QML_NAMED_ELEMENT(FixedPage)
};

struct InkLayerForeign
{
    Q_GADGET
    QML_FOREIGN(actcraft::render::InkItem)
    QML_NAMED_ELEMENT(InkLayer)
};

} // namespace actcraft::ui
```

Créer `ui/src/penadapters.h` :

```cpp
#pragma once

#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/tools/pentool.h>

namespace actcraft::ui {

class PageView;

// Le stylo (tools/) ne connaît que des interfaces ; ui/ les branche sur render/ (règle de modularité 2).
class InkSurfaceAdapter final : public tools::InkSurface
{
public:
    explicit InkSurfaceAdapter(render::InkItem& ink);
    void beginStroke(core::Argb color, double baseWidth) override;
    void addPoint(const core::StrokePoint& point, std::uint64_t seq) override;
    void clear() override;

private:
    render::InkItem& m_ink;
};

class FixedPageSink final : public tools::DocumentSink
{
public:
    FixedPageSink(render::FixedPageItem& page, PageView& view);
    void commitStroke(core::Stroke stroke) override;

private:
    render::FixedPageItem& m_page;
    PageView& m_view;
};

} // namespace actcraft::ui
```

Créer `ui/src/penadapters.cpp` :

```cpp
#include "penadapters.h"

#include <actcraft/ui/pageview.h>

namespace actcraft::ui {

InkSurfaceAdapter::InkSurfaceAdapter(render::InkItem& ink) : m_ink(ink) {}

void InkSurfaceAdapter::beginStroke(core::Argb color, double baseWidth)
{
    m_ink.beginStroke(color, baseWidth);
}

void InkSurfaceAdapter::addPoint(const core::StrokePoint& point, std::uint64_t seq)
{
    m_ink.addPoint(point, seq);
}

void InkSurfaceAdapter::clear()
{
    m_ink.clear();
}

FixedPageSink::FixedPageSink(render::FixedPageItem& page, PageView& view) : m_page(page), m_view(view) {}

void FixedPageSink::commitStroke(core::Stroke stroke)
{
    m_page.appendStroke(std::move(stroke));
    m_view.onStrokeFrozen(); // cache redessiné dans le même tour de boucle que le vidage de l'encre
}

} // namespace actcraft::ui
```

- [ ] **Étape 4 : `PageView` reçoit le stylo.** Dans `ui/include/actcraft/ui/pageview.h` :
ajouter les inclusions

```cpp
#include <actcraft/core/inkprobe.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>

#include <cstdint>
#include <memory>

namespace actcraft::input {
class Pipeline;
}
namespace actcraft::tools {
class PenTool;
}
namespace actcraft::ui {
class InkSurfaceAdapter;
class FixedPageSink;
}
```
puis, dans la classe, sous les `Q_PROPERTY` existants :

```cpp
    Q_PROPERTY(actcraft::render::FixedPageItem* fixedPage READ fixedPage WRITE setFixedPage NOTIFY fixedPageChanged)
    Q_PROPERTY(actcraft::render::InkItem* ink READ ink WRITE setInk NOTIFY inkChanged)
```
dans `public:` :

```cpp
    ~PageView() override;

    render::FixedPageItem* fixedPage() const noexcept;
    void setFixedPage(render::FixedPageItem* page);
    render::InkItem* ink() const noexcept;
    void setInk(render::InkItem* ink);
    // Banc : sonde de latence (DA4) et numérotation des échantillons depuis `next`.
    void setInkProbe(core::InkProbe* probe);
    void resetInputSequence(std::uint64_t next = 0);
    // Appelé par l'adaptateur du document quand un trait est figé.
    void onStrokeFrozen();
```
dans `signals:` : `void fixedPageChanged();`, `void inkChanged();`, `void strokeFrozen();` ; dans
`protected:` :

```cpp
    bool event(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
```
dans `private:` :

```cpp
    void rebuildPen();
    bool handlePointer(const QEvent& event, QPointF viewPos);

    render::FixedPageItem* m_fixedPage = nullptr;
    render::InkItem* m_ink = nullptr;
    core::InkProbe* m_probe = nullptr;
    std::unique_ptr<InkSurfaceAdapter> m_inkSurface;
    std::unique_ptr<FixedPageSink> m_documentSink;
    std::unique_ptr<tools::PenTool> m_pen;
    std::unique_ptr<input::Pipeline> m_pipeline;
    QMetaObject::Connection m_readyConnection;
```
(Les identifiants QML des éléments — `fixedPageItem`, `inkLayer` — diffèrent des noms de propriétés de `PageView`
pour qu'aucune liaison `fixedPage: …` ne se résolve sur elle-même.)

Dans `ui/src/pageview.cpp`, ajouter les inclusions :

```cpp
#include "penadapters.h"

#include <actcraft/core/clock.h>
#include <actcraft/input/pipeline.h>
#include <actcraft/input/qtevents.h>
#include <actcraft/tools/pentool.h>

#include <QMouseEvent>
#include <QTabletEvent>
```
et les définitions :

```cpp
PageView::~PageView() = default;

render::FixedPageItem* PageView::fixedPage() const noexcept
{
    return m_fixedPage;
}

void PageView::setFixedPage(render::FixedPageItem* page)
{
    if (page == m_fixedPage) {
        return;
    }
    disconnect(m_readyConnection);
    m_fixedPage = page;
    if (m_fixedPage) {
        // Page prête : premier dessin du cache.
        m_readyConnection = connect(m_fixedPage, &render::FixedPageItem::readyChanged, this, [this] {
            if (m_fixedPage && m_fixedPage->isReady()) {
                requestCacheRefresh();
            }
        });
    }
    rebuildPen();
    emit fixedPageChanged();
}

render::InkItem* PageView::ink() const noexcept
{
    return m_ink;
}

void PageView::setInk(render::InkItem* ink)
{
    if (ink == m_ink) {
        return;
    }
    m_ink = ink;
    rebuildPen();
    emit inkChanged();
}

void PageView::setInkProbe(core::InkProbe* probe)
{
    m_probe = probe;
    if (m_pipeline) {
        m_pipeline->setProbe(probe);
    }
    if (m_ink) {
        m_ink->setProbe(probe);
    }
}

void PageView::resetInputSequence(std::uint64_t next)
{
    if (m_pipeline) {
        m_pipeline->resetSequence(next);
    }
}

void PageView::onStrokeFrozen()
{
    emit strokeFrozen();
    requestCacheRefresh();
}

void PageView::rebuildPen()
{
    m_pipeline.reset();
    m_pen.reset();
    m_documentSink.reset();
    m_inkSurface.reset();
    if (!m_fixedPage || !m_ink) {
        return;
    }
    m_inkSurface = std::make_unique<InkSurfaceAdapter>(*m_ink);
    m_documentSink = std::make_unique<FixedPageSink>(*m_fixedPage, *this);
    m_pen = std::make_unique<tools::PenTool>(*m_inkSurface, *m_documentSink);
    m_pipeline = std::make_unique<input::Pipeline>(*m_pen);
    m_pipeline->setProbe(m_probe);
    m_ink->setProbe(m_probe);
}

bool PageView::handlePointer(const QEvent& event, QPointF viewPos)
{
    if (!m_pipeline) {
        return false;
    }
    const auto& pointer = static_cast<const QPointerEvent&>(event);
    if (!acceptsDevice(pointer.pointingDevice())) {
        return false;
    }
    const auto sample =
        input::sampleFromEvent(event, viewTransform().viewToPage(viewPos), core::monotonicNowNs());
    if (!sample) {
        return false;
    }
    m_pipeline->push(*sample);
    return true;
}

void PageView::mousePressEvent(QMouseEvent* event)
{
    event->setAccepted(handlePointer(*event, event->position()));
}

void PageView::mouseMoveEvent(QMouseEvent* event)
{
    event->setAccepted(handlePointer(*event, event->position()));
}

void PageView::mouseReleaseEvent(QMouseEvent* event)
{
    event->setAccepted(handlePointer(*event, event->position()));
}
```

**Variante A (`tablette-element: oui`)** — ajouter :

```cpp
bool PageView::event(QEvent* event)
{
    switch (event->type()) {
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease: {
        auto* tablet = static_cast<QTabletEvent*>(event);
        if (handlePointer(*tablet, tablet->position())) {
            tablet->accept(); // accepté : Qt ne synthétise pas de souris en double
            return true;
        }
        break;
    }
    default:
        break;
    }
    return QQuickItem::event(event);
}
```

**Variante B (`tablette-element: non`)** — même `event()` qui renvoie simplement `QQuickItem::event(event)`, et en
plus un filtre sur la fenêtre. Déclarer dans la classe (`protected:`)
`bool eventFilter(QObject* watched, QEvent* event) override;` et
`void itemChange(ItemChange change, const ItemChangeData& value) override;`, et en `private:`
`QPointer<QQuickWindow> m_filteredWindow;` (`#include <QPointer>`), puis :

```cpp
bool PageView::event(QEvent* event)
{
    return QQuickItem::event(event);
}

void PageView::itemChange(ItemChange change, const ItemChangeData& value)
{
    QQuickItem::itemChange(change, value);
    if (change == ItemSceneChange) {
        if (m_filteredWindow) {
            m_filteredWindow->removeEventFilter(this);
        }
        m_filteredWindow = value.window;
        if (m_filteredWindow) {
            m_filteredWindow->installEventFilter(this);
        }
    }
}

bool PageView::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::TabletPress || event->type() == QEvent::TabletMove ||
        event->type() == QEvent::TabletRelease) {
        auto* tablet = static_cast<QTabletEvent*>(event);
        if (handlePointer(*tablet, mapFromScene(tablet->position()))) {
            tablet->accept();
            return true; // consommé : pas de souris synthétisée en double
        }
    }
    return QQuickItem::eventFilter(watched, event);
}
```

- [ ] **Étape 5 : `Main.qml`.** Remplacer `ui/qml/Main.qml` par :

```qml
import QtQuick
import ActCraft.Ui

Window {
    id: root
    width: 1280
    height: 800
    visible: true
    // Rendu effectif dans le titre (décision 19) : aucun texte dans la scène.
    title: qsTr("ActCraft — rendu : %1").arg(root.apiName(GraphicsInfo.api))
    color: "#3c3f45"

    // Fixée avant le chargement par l'application ou le banc (--sans-cache, décision 8).
    property bool cacheEnabled: true

    PageView {
        id: view
        objectName: "pageView"
        anchors.fill: parent
        cacheEnabled: root.cacheEnabled
        content: pageContent
        fixedPage: fixedPageItem
        ink: inkLayer
        // Branché dès la création de la vue (un bloc Connections déclaré plus bas ne l'est qu'en fin de
        // composant et perdrait les premières demandes).
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
                Component.onCompleted: loadGeneratedPage()
            }

            // Cache du contenu fixe (décision 7) : redessiné seulement sur demande de la vue.
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

**Variante B du cache (`couche-logicielle: non` ou `couche-rendernode-logiciel: non`)** : remplacer la ligne `cacheEnabled: root.cacheEnabled` de
`PageView` par
`cacheEnabled: root.cacheEnabled && GraphicsInfo.api !== GraphicsInfo.Software` ; le JSON du banc lit
`PageView::cacheEnabled()` et écrira donc `cache: false` en logiciel.

- [ ] **Étape 6 : CMake de `ui/`.** Dans `ui/CMakeLists.txt`, compléter `SOURCES` :

```cmake
        src/uiforeigntypes.h
        src/penadapters.h
        src/penadapters.cpp
```
remplacer l'appel `target_include_directories` par (le code d'enregistrement généré inclut
`<uiforeigntypes.h>` sous `#if __has_include(...)` : hors du chemin d'inclusion, l'inclusion est sautée sans
erreur et `qmlRegisterTypesAndRevisions<actcraft::ui::FixedPageForeign>` ne compile plus) :

```cmake
target_include_directories(actcraft_ui
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/actcraft/ui ${CMAKE_CURRENT_SOURCE_DIR}/src)
```
et remplacer la ligne `target_link_libraries` par :

```cmake
target_link_libraries(actcraft_ui PUBLIC
    Qt6::Quick ActCraft::actions ActCraft::input ActCraft::render ActCraft::tools)
```
Si `actcraft_ui_qmltyperegistrations.cpp` ne compile pas sur `FixedPageForeign` ou `InkLayerForeign` : vérifier
d'abord que `src` figure dans les inclusions privées ci-dessus. Si `qmltyperegistrar` signale « type not found »
pour `actcraft::render::FixedPageItem` : vérifier `qt_extract_metatypes(actcraft_render)` dans
`render/CMakeLists.txt` (T6) et le lien `PUBLIC` vers `ActCraft::render`.

- [ ] **Étape 7 : vert.** Commande bornée : `tst_ui ... Passed`, `tst_ui_drawing ... Passed`, `TESTS OK`.
Si l'encre en cours n'apparaît pas dans `stylusStrokeWithCache` alors que `pointCount()` grandit : vérifier
l'ordre des enfants de `pageContent` (`InkLayer` en dernier) et que le cache ne masque pas l'encre.

- [ ] **Étape 8 : format, preuve, commit.**

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add ui/CMakeLists.txt ui/qml/Main.qml ui/include/actcraft/ui/pageview.h ui/src/pageview.cpp ui/src/uiforeigntypes.h ui/src/penadapters.h ui/src/penadapters.cpp app/startup.h app/startup.cpp app/main.cpp app/CMakeLists.txt tests/ui.cmake tests/tst_ui.cpp tests/tst_ui_drawing.cpp
git -C <worktree> commit -m "ui : page fixe en cache, encre et stylo câblés ; option --sans-cache" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

### Tâche T13 — `bench` : statistiques (intervalles actifs, figements, latence)

**Objectif.** Fonctions pures du banc (décisions 24 et 25, N1-1) :
- **intervalle actif** entre deux `frameSwapped` : au moins une entrée dans l'intervalle, **et** la première
  de ces entrées arrive au plus 2 périodes d'écran après l'image de départ ; sinon l'intervalle est « au repos »
  (pause du stylo levé, attente du redessin). Les instants d'entrée sont les instants **prévus** (T16 les
  fournit) : un fil principal bloqué qui poste l'arriéré d'un coup ne fait pas passer l'intervalle « au repos » ;
- un intervalle qui contient un **figement** de trait est compté à part ;
- statistiques sur les intervalles actifs hors figements : p50, p95, p99, max (rang le plus proche), part des
  intervalles > 25 ms, **images/s = 1000 / intervalle médian** ; figements : maximum et nombre de figements
  ≥ 2,5 périodes d'écran ;
- **latence** : chaque échantillon mesuré est rattaché à la première image dont la synchronisation contient son
  numéro ; total = `frameSwapped` − instant prévu, en deux composantes (prévu → synchronisation,
  synchronisation → `frameSwapped`).

**Le piège.** Le patron évident compte tous les intervalles (ou tous ceux qui suivent une entrée) : le premier
intervalle après une pause de 300 ms vaudrait 300 ms et ruinerait le p99 ; et une latence calculée sur le relâché
(qui n'entre jamais dans l'encre) se rattacherait au trait suivant. Les tests ci-dessous fixent ces deux cas.

**Fichiers.**
- Créer : `bench/include/actcraft/bench/stats.h`, `bench/src/stats.cpp`, `tests/tst_bench_stats.cpp`
- Modifier : `bench/CMakeLists.txt`, `tests/bench.cmake`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_bench_stats .*(Passed|Failed)' | Select-Object -Last 3
```
Attendu : `out of N` avec N ≥ 4, `tst_bench_stats ... Passed`, `TESTS OK`.

**Interdits.** Aucune dépendance à Qt Quick dans `stats.*` (fonctions pures). Ne pas changer la définition d'un
intervalle actif pour faire passer un test.

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : tests (rouges).** Créer `tests/tst_bench_stats.cpp` :

```cpp
#include <actcraft/bench/stats.h>

#include <QTest>

using namespace actcraft::bench;

namespace {
constexpr std::int64_t ms(double v)
{
    return static_cast<std::int64_t>(v * 1'000'000.0);
}
} // namespace

class TstBenchStats : public QObject
{
    Q_OBJECT

private slots:
    void percentileIsNearestRank()
    {
        std::vector<double> values;
        for (int i = 100; i >= 1; --i) {
            values.push_back(i);
        }
        const Percentiles p = summarize(values);
        QCOMPARE(p.p50, 50.0);
        QCOMPARE(p.p95, 95.0);
        QCOMPARE(p.p99, 99.0);
        QCOMPARE(p.max, 100.0);
        QCOMPARE(p.count, std::size_t{100});
        QCOMPARE(percentile({7.0}, 95), 7.0);
        QCOMPARE(percentile({}, 50), 0.0);
    }

    void intervalNeedsAnInput()
    {
        FrameSeries s;
        s.swapNs = {0, ms(16), ms(33), ms(50)};
        s.inputNs = {ms(5), ms(20)};
        const IntervalStats r = classifyIntervals(s);
        QCOMPARE(r.classes.size(), std::size_t{3});
        QCOMPARE(r.classes[0], IntervalClass::Active);
        QCOMPARE(r.classes[1], IntervalClass::Active);
        QCOMPARE(r.classes[2], IntervalClass::Idle);
        QCOMPARE(r.activeMs.size(), std::size_t{2});
    }

    void firstIntervalAfterAPauseIsIdle()
    {
        FrameSeries s;
        s.swapNs = {0, ms(16), ms(316), ms(333)};
        s.inputNs = {ms(5), ms(310), ms(320)};
        const IntervalStats r = classifyIntervals(s);
        QCOMPARE(r.classes[0], IntervalClass::Active);
        QCOMPARE(r.classes[1], IntervalClass::Idle); // 294 ms sans entrée depuis l'image de départ
        QCOMPARE(r.classes[2], IntervalClass::Active);
        QVERIFY(r.active.max < 20.0);
    }

    void freezeIsCountedApart()
    {
        FrameSeries s;
        s.swapNs = {0, ms(16), ms(33), ms(50)};
        s.inputNs = {ms(5), ms(18), ms(40)};
        s.freezeNs = {ms(20)};
        const IntervalStats r = classifyIntervals(s);
        QCOMPARE(r.classes[1], IntervalClass::Freeze);
        QCOMPARE(r.freezeMs.size(), std::size_t{1});
        QCOMPARE(r.freezeOverLimit, std::size_t{0});
        QCOMPARE(r.activeMs.size(), std::size_t{2});

        FrameSeries slow;
        slow.swapNs = {0, ms(16), ms(66)};
        slow.inputNs = {ms(5), ms(18)};
        slow.freezeNs = {ms(20)};
        const IntervalStats l = classifyIntervals(slow);
        QCOMPARE(l.freezeOverLimit, std::size_t{1}); // 50 ms ≥ 2,5 × 16,7 ms
        QVERIFY(qAbs(l.freezeMaxMs - 50.0) < 1e-6);
    }

    void plannedInstantsKeepABlockedIntervalActive()
    {
        // Fil principal bloqué 90 ms : entrées prévues toutes les 5 ms, postées en bloc à la fin.
        FrameSeries planned;
        planned.swapNs = {0, ms(90)};
        for (int i = 1; i <= 17; ++i) {
            planned.inputNs.push_back(ms(5.0 * i));
        }
        const IntervalStats r = classifyIntervals(planned);
        QCOMPARE(r.classes[0], IntervalClass::Active);
        QVERIFY(qAbs(r.active.max - 90.0) < 1e-6); // le blocage compte dans p95, p99 et max
        FrameSeries posted = planned;
        posted.inputNs.assign(17, ms(89));
        QCOMPARE(classifyIntervals(posted).classes[0], IntervalClass::Idle); // d'où les instants prévus
    }

    void fpsAndShareOver25()
    {
        FrameSeries s;
        for (int i = 0; i <= 100; ++i) {
            s.swapNs.push_back(ms(16.6667 * i + (i > 98 ? 20 : 0)));
            s.inputNs.push_back(ms(16.6667 * i + 1));
        }
        const IntervalStats r = classifyIntervals(s);
        QVERIFY(qAbs(r.fps - 60.0) < 0.1);
        QVERIFY(r.over25Ratio > 0.0 && r.over25Ratio <= 0.011);
    }

    void latencyUsesFirstFrameContainingTheSample()
    {
        const std::vector<std::int64_t> planned{0, ms(5), ms(10)};
        const std::vector<SyncRecord> records{{0, ms(8), ms(20)}, {2, ms(24), ms(36)}};
        const LatencyStats l = computeLatency(planned, records);
        QCOMPARE(l.seq.size(), std::size_t{3});
        QVERIFY(qAbs(l.totalMs[0] - 20.0) < 1e-9);
        QVERIFY(qAbs(l.plannedToSyncMs[0] - 8.0) < 1e-9);
        QVERIFY(qAbs(l.syncToSwapMs[0] - 12.0) < 1e-9);
        QVERIFY(qAbs(l.totalMs[1] - 31.0) < 1e-9);
        QVERIFY(qAbs(l.totalMs[2] - 26.0) < 1e-9);
        QCOMPARE(l.unmatched, std::size_t{0});
    }

    void unmeasuredAndUnmatchedSamples()
    {
        const std::vector<std::int64_t> planned{0, -1, ms(10), ms(15)};
        const std::vector<SyncRecord> records{{2, ms(12), ms(20)}};
        const LatencyStats l = computeLatency(planned, records);
        QCOMPARE(l.seq.size(), std::size_t{2}); // 0 et 2 ; 1 non mesuré
        QCOMPARE(l.unmatched, std::size_t{1});  // 3 n'est dans aucune image
    }
};

QTEST_GUILESS_MAIN(TstBenchStats)
#include "tst_bench_stats.moc"
```

Dans `tests/bench.cmake`, après le bloc `if(NOT ACTCRAFT_BUILD_BENCH) … endif()` :

```cmake
actcraft_add_test(tst_bench_stats
    SOURCES tst_bench_stats.cpp
    LIBS ActCraft::bench)
```
Commande bornée : `TESTS EN ECHEC (compilation)`.

- [ ] **Étape 2 : en-tête.** Créer `bench/include/actcraft/bench/stats.h` :

```cpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace actcraft::bench {

inline constexpr double kOver25Ms = 25.0;
inline constexpr double kFreezeFactor = 2.5; // figement : au plus une image perdue (N1-1)

struct Percentiles
{
    double p50 = 0;
    double p95 = 0;
    double p99 = 0;
    double max = 0;
    std::size_t count = 0;
};

// Rang le plus proche : valeur triée d'indice ceil(p/100 × n) − 1. 0 si vide.
double percentile(std::vector<double> values, double p);
Percentiles summarize(const std::vector<double>& values);

// Séries d'un scénario, horloge core::monotonicNowNs().
struct FrameSeries
{
    std::vector<std::int64_t> swapNs;   // frameSwapped, croissants
    std::vector<std::int64_t> inputNs;  // instants PRÉVUS des entrées postées, croissants (décision 23)
    std::vector<std::int64_t> freezeNs; // figements de trait
    std::int64_t refreshPeriodNs = 16'666'667;
};

enum class IntervalClass : std::uint8_t
{
    Idle,
    Active,
    Freeze,
};

struct IntervalStats
{
    std::vector<IntervalClass> classes; // une par intervalle (swapNs.size() − 1)
    std::vector<double> activeMs;       // intervalles actifs hors figements
    std::vector<double> freezeMs;
    Percentiles active;
    double fps = 0;         // 1000 / médiane des intervalles actifs
    double over25Ratio = 0; // part des intervalles actifs > 25 ms
    double freezeMaxMs = 0;
    std::size_t freezeOverLimit = 0; // figements ≥ kFreezeFactor périodes d'écran
};

IntervalStats classifyIntervals(const FrameSeries& series);

// Une synchronisation de l'encre et le frameSwapped de l'image qui la suit.
struct SyncRecord
{
    std::uint64_t lastSeq = 0;
    std::int64_t syncNs = 0;
    std::int64_t swapNs = 0;
};

struct LatencyStats
{
    std::vector<std::uint64_t> seq;
    std::vector<double> totalMs;
    std::vector<double> plannedToSyncMs;
    std::vector<double> syncToSwapMs;
    Percentiles total;
    Percentiles plannedToSync;
    Percentiles syncToSwap;
    std::size_t unmatched = 0; // échantillons mesurés jamais vus dans une image
};

// plannedNs[seq] : instant prévu de l'échantillon `seq`, ou −1 s'il n'est pas mesuré (relâché, molette).
// records : dans l'ordre des images, lastSeq croissant.
LatencyStats computeLatency(const std::vector<std::int64_t>& plannedNs, const std::vector<SyncRecord>& records);

} // namespace actcraft::bench
```

- [ ] **Étape 3 : implémentation.** Créer `bench/src/stats.cpp` :

```cpp
#include <actcraft/bench/stats.h>

#include <algorithm>
#include <cmath>

namespace actcraft::bench {

namespace {
double toMs(std::int64_t ns)
{
    return static_cast<double>(ns) / 1'000'000.0;
}
} // namespace

double percentile(std::vector<double> values, double p)
{
    if (values.empty()) {
        return 0;
    }
    std::sort(values.begin(), values.end());
    const double rank = std::ceil(p / 100.0 * static_cast<double>(values.size()));
    const std::size_t index = rank < 1 ? 0 : std::min(values.size() - 1, static_cast<std::size_t>(rank) - 1);
    return values[index];
}

Percentiles summarize(const std::vector<double>& values)
{
    Percentiles r;
    r.count = values.size();
    if (values.empty()) {
        return r;
    }
    r.p50 = percentile(values, 50);
    r.p95 = percentile(values, 95);
    r.p99 = percentile(values, 99);
    r.max = *std::max_element(values.begin(), values.end());
    return r;
}

IntervalStats classifyIntervals(const FrameSeries& s)
{
    IntervalStats r;
    for (std::size_t i = 1; i < s.swapNs.size(); ++i) {
        const std::int64_t start = s.swapNs[i - 1];
        const std::int64_t end = s.swapNs[i];
        const double length = toMs(end - start);
        const auto freeze = std::upper_bound(s.freezeNs.begin(), s.freezeNs.end(), start);
        if (freeze != s.freezeNs.end() && *freeze <= end) {
            r.classes.push_back(IntervalClass::Freeze);
            r.freezeMs.push_back(length);
            continue;
        }
        const auto input = std::upper_bound(s.inputNs.begin(), s.inputNs.end(), start);
        if (input == s.inputNs.end() || *input > end || *input - start > 2 * s.refreshPeriodNs) {
            r.classes.push_back(IntervalClass::Idle);
            continue;
        }
        r.classes.push_back(IntervalClass::Active);
        r.activeMs.push_back(length);
    }
    r.active = summarize(r.activeMs);
    if (!r.activeMs.empty()) {
        r.fps = r.active.p50 > 0 ? 1000.0 / r.active.p50 : 0;
        const auto over = std::count_if(r.activeMs.begin(), r.activeMs.end(), [](double v) { return v > kOver25Ms; });
        r.over25Ratio = static_cast<double>(over) / static_cast<double>(r.activeMs.size());
    }
    const double limitMs = kFreezeFactor * toMs(s.refreshPeriodNs);
    for (double v : r.freezeMs) {
        r.freezeMaxMs = std::max(r.freezeMaxMs, v);
        if (v >= limitMs) {
            ++r.freezeOverLimit;
        }
    }
    return r;
}

LatencyStats computeLatency(const std::vector<std::int64_t>& plannedNs, const std::vector<SyncRecord>& records)
{
    LatencyStats r;
    std::size_t k = 0;
    for (std::size_t seq = 0; seq < plannedNs.size(); ++seq) {
        if (plannedNs[seq] < 0) {
            continue;
        }
        while (k < records.size() && records[k].lastSeq < seq) {
            ++k;
        }
        if (k == records.size()) {
            ++r.unmatched;
            continue;
        }
        const SyncRecord& rec = records[k];
        r.seq.push_back(static_cast<std::uint64_t>(seq));
        r.totalMs.push_back(toMs(rec.swapNs - plannedNs[seq]));
        r.plannedToSyncMs.push_back(toMs(rec.syncNs - plannedNs[seq]));
        r.syncToSwapMs.push_back(toMs(rec.swapNs - rec.syncNs));
    }
    r.total = summarize(r.totalMs);
    r.plannedToSync = summarize(r.plannedToSyncMs);
    r.syncToSwap = summarize(r.syncToSwapMs);
    return r;
}

} // namespace actcraft::bench
```

Dans `bench/CMakeLists.txt`, ajouter `src/stats.cpp` aux sources (dépendances inchangées).

- [ ] **Étape 4 : vert, format, commit.** Commande bornée : `tst_bench_stats ... Passed`, `TESTS OK`.

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add bench/CMakeLists.txt bench/include/actcraft/bench/stats.h bench/src/stats.cpp tests/bench.cmake tests/tst_bench_stats.cpp
git -C <worktree> commit -m "bench : intervalles actifs, figements à part, latence par numéro de séquence" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

### Tâche T14 — `bench` : mémoire, machine, fichier JSON

**Objectif.** Ce que le banc écrit (décisions 26, 27, 28) :
- mémoire du processus : Windows `GetProcessMemoryInfo` (privée, ensemble de travail, pic), macOS `task_info`
  (empreinte physique) ; mémoire graphique : D3D11 par `IDXGIAdapter3::QueryVideoMemoryInfo` (périphérique obtenu
  par `QSGRendererInterface::getResource`), Metal par `currentAllocatedSize` du `MTLDevice` ; absente en
  logiciel ; un suivi début / fin / pic alimenté à chaque image ;
- métadonnées : modèle de machine (jamais le nom d'hôte ni le nom d'utilisateur), processeur, système, version de
  Qt, commit (et modifications locales), type de construction, options de compilation (`/Ob2` visible en
  release MSVC) ;
- le JSON `actcraft-bench/1` (schéma ci-dessous), son nom de fichier et son écriture.

**Fichiers.**
- Créer : `bench/include/actcraft/bench/memory.h`, `bench/include/actcraft/bench/machineinfo.h`,
  `bench/include/actcraft/bench/result.h`, `bench/src/memory.cpp`, `bench/src/gpumemory_win.cpp`,
  `bench/src/gpumemory_mac.mm`, `bench/src/gpumemory_other.cpp`, `bench/src/machineinfo.cpp`,
  `bench/src/result.cpp`, `bench/src/buildinfo.h.in`, `tests/tst_bench_json.cpp`, `tests/tst_bench_memory.cpp`
- Modifier : `bench/CMakeLists.txt`, `tests/bench.cmake`

**Schéma JSON `actcraft-bench/1` (figé ; T16-T20 s'y fient).** Clés en anglais (N2-7) ; durées en ms, séries
brutes en ns relatives au début du scénario, octets en nombres.

```text
{
  "format": "actcraft-bench/1",
  "started_at": "2026-10-13T14:03:22",
  "graphics_api": "direct3d11" | "software" | "metal" | ...,
  "cache": true,
  "valid_for_measurement": true,
  "within_budget": true,
  "informative": false,
  "meta": { "commit", "commit_dirty", "qt_version", "os", "kernel", "cpu", "machine_model", "gpu",
            "requested_backend", "render_loop", "build_type", "compiler_flags", "scenario_set", "activated",
            "screen": { "name", "width", "height", "refresh_hz", "device_pixel_ratio" },
            "window": { "mode", "width", "height" },
            "limits": "<texte : ce que la latence interne ne mesure pas>" },
  "scenarios": [ {
      "name": "stroke" | "zoom" | "scroll", "exposure_lost": false,
      "frames": { "count", "intervals", "active", "freeze", "idle", "fps", "p50_ms", "p95_ms", "p99_ms",
                  "max_ms", "over_25ms_ratio", "freeze_max_ms", "freeze_over_limit", "refresh_period_ms" },
      "latency": null | { "samples", "unmatched", "total_ms": P, "planned_to_sync_ms": P, "sync_to_swap_ms": P },
      "input": { "count", "injection_delay_ms": P, "reception_delay_ms": P },
      "memory": { "process_private_start_bytes", "process_private_end_bytes", "process_private_peak_bytes",
                  "working_set_peak_bytes", "gpu_start_bytes", "gpu_end_bytes", "gpu_peak_bytes" },
      "raw": { "swap_ns", "interval_class", "input_ns", "freeze_ns", "latency_seq", "latency_total_ms",
               "latency_planned_to_sync_ms", "latency_sync_to_swap_ms" } } ],
  "content_check": { "ok": bool, "checks": [ { "name", "ok", "expected", "actual" } ] },
  "verdict": { "valid_for_measurement", "invalid_reasons": [..], "within_budget", "informative",
               "criteria": [ { "scenario", "criterion", "value", "limit", "ok" } ] }
}
P = { "p50", "p95", "p99", "max", "count" } ; "gpu_*_bytes" = null si indisponible ;
"interval_class" = une lettre par intervalle : A (actif), I (repos), F (figement) ;
"input_ns" = instants prévus des entrées postées.
```

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_bench_(json|memory) .*(Passed|Failed)' | Select-Object -Last 4
```
Attendu : `out of N` avec N ≥ 6, `tst_bench_json ... Passed`, `tst_bench_memory ... Passed`, `TESTS OK`.

**Interdits.** Jamais `QSysInfo::machineHostName()`, `COMPUTERNAME`, `USERNAME`, `USER` ni `QDir::homePath()`
dans le JSON ou le nom du fichier. Pas de `QRhi::driverInfo` (exige `Qt6::GuiPrivate`). Le fichier `.mm` n'est
compilé qu'en CI macOS : le garder minimal, tel que ci-dessous.

**Terminé quand.** Preuve verte sous Windows ; commit fait. La compilation de `gpumemory_mac.mm` est prouvée par
la CI macOS de la PR (rapporter « non vérifié localement »).

**Étapes.**

- [ ] **Étape 1 : tests (rouges).** Créer `tests/tst_bench_json.cpp` :

```cpp
#include <actcraft/bench/machineinfo.h>
#include <actcraft/bench/result.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>
#include <QTemporaryDir>
#include <QTest>

using namespace actcraft::bench;

namespace {

constexpr std::int64_t ms(double v)
{
    return static_cast<std::int64_t>(v * 1'000'000.0);
}

BenchResult sample()
{
    BenchResult r;
    r.startedAt = QDateTime(QDate(2026, 10, 13), QTime(14, 3, 22));
    r.machine = readMachineInfo();
    r.machine.model = QStringLiteral("Dell Inc. Inspiron 15 3510");
    r.graphicsApi = QStringLiteral("direct3d11");
    r.cache = true;
    r.scenarioSet = QStringLiteral("court");
    ScenarioResult stroke;
    stroke.name = QStringLiteral("stroke");
    stroke.startNs = ms(1);
    stroke.frames.swapNs = {ms(1), ms(17), ms(34)};
    stroke.frames.inputNs = {ms(5), ms(20)};
    stroke.intervals = classifyIntervals(stroke.frames);
    stroke.latency = computeLatency({ms(2), ms(7)}, {{1, ms(10), ms(17)}});
    stroke.memory.privatePeak = 1000;
    r.scenarios.push_back(stroke);
    ScenarioResult zoom;
    zoom.name = QStringLiteral("zoom");
    r.scenarios.push_back(zoom);
    return r;
}

} // namespace

class TstBenchJson : public QObject
{
    Q_OBJECT

private slots:
    void fileNameHasDateModelAndApi()
    {
        QCOMPARE(resultFileName(sample()), QStringLiteral("2026-10-13_140322-Dell-Inc-Inspiron-15-3510-direct3d11.json"));
    }

    void sanitizeKeepsSafeCharacters()
    {
        QCOMPARE(sanitizeForFileName(QStringLiteral("Mac Book/Air (M1)")), QStringLiteral("Mac-Book-Air-M1"));
        QCOMPARE(sanitizeForFileName(QString()), QStringLiteral("inconnu"));
    }

    void topLevelKeys()
    {
        const QJsonObject o = toJson(sample());
        QCOMPARE(o.value("format").toString(), QStringLiteral("actcraft-bench/1"));
        QCOMPARE(o.value("graphics_api").toString(), QStringLiteral("direct3d11"));
        QCOMPARE(o.value("cache").toBool(), true);
        QVERIFY(o.contains("valid_for_measurement"));
        QVERIFY(o.contains("within_budget"));
        QVERIFY(o.value("meta").toObject().contains("compiler_flags"));
        QVERIFY(o.value("meta").toObject().contains("limits"));
        QCOMPARE(o.value("content_check").toObject().value("ok").toBool(), false); // aucun contrôle
        QVERIFY(o.value("verdict").isObject());
    }

    void scenariosAndRawSeries()
    {
        const QJsonArray scenarios = toJson(sample()).value("scenarios").toArray();
        QCOMPARE(scenarios.size(), 2);
        const QJsonObject stroke = scenarios.at(0).toObject();
        QCOMPARE(stroke.value("name").toString(), QStringLiteral("stroke"));
        QCOMPARE(stroke.value("frames").toObject().value("count").toInt(), 3);
        QCOMPARE(stroke.value("raw").toObject().value("swap_ns").toArray().at(0).toDouble(), 0.0);
        QCOMPARE(stroke.value("raw").toObject().value("interval_class").toString(), QStringLiteral("AA"));
        QCOMPARE(stroke.value("latency").toObject().value("samples").toInt(), 2);
        QVERIFY(stroke.value("memory").toObject().value("gpu_peak_bytes").isNull());
        QVERIFY(scenarios.at(1).toObject().value("latency").isNull());
    }

    void noHostOrUserName()
    {
        const QString text = QString::fromUtf8(QJsonDocument(toJson(sample())).toJson());
        const QStringList secrets{QSysInfo::machineHostName(), qEnvironmentVariable("COMPUTERNAME"),
                                  qEnvironmentVariable("USERNAME"), qEnvironmentVariable("USER")};
        for (const QString& secret : secrets) {
            if (secret.size() >= 3) {
                QVERIFY2(!text.contains(secret, Qt::CaseInsensitive), qPrintable(secret));
            }
        }
    }

    void writeCreatesFolders()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("a/b/resultat.json"));
        QString error;
        QVERIFY2(writeResult(sample(), path, error), qPrintable(error));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(QJsonDocument::fromJson(file.readAll()).object().value("format").toString(),
                 QStringLiteral("actcraft-bench/1"));
    }

    void apiNames()
    {
        QCOMPARE(apiName(QSGRendererInterface::Software), QStringLiteral("software"));
        QCOMPARE(apiName(QSGRendererInterface::Direct3D11), QStringLiteral("direct3d11"));
        QCOMPARE(apiName(QSGRendererInterface::Metal), QStringLiteral("metal"));
    }
};

QTEST_GUILESS_MAIN(TstBenchJson)
#include "tst_bench_json.moc"
```

Créer `tests/tst_bench_memory.cpp` :

```cpp
#include <actcraft/bench/machineinfo.h>
#include <actcraft/bench/memory.h>

#include <QQuickWindow>
#include <QTest>

using namespace actcraft::bench;

class TstBenchMemory : public QObject
{
    Q_OBJECT

private slots:
    void processMemoryIsReported()
    {
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
        const ProcessMemory m = readProcessMemory();
        QVERIFY(m.privateBytes > 0);
        QVERIFY(m.workingSetBytes > 0);
#else
        QSKIP("mémoire du processus non relevée sur cette plateforme");
#endif
    }

    void trackerKeepsStartEndAndPeak()
    {
        MemoryTracker tracker;
        tracker.sample({100, 50, 50}, 10);
        tracker.sample({300, 70, 70}, 30);
        tracker.sample({200, 60, 70}, 20);
        const MemorySummary s = tracker.summary();
        QCOMPARE(s.privateStart, std::uint64_t{100});
        QCOMPARE(s.privateEnd, std::uint64_t{200});
        QCOMPARE(s.privatePeak, std::uint64_t{300});
        QCOMPARE(s.workingSetPeak, std::uint64_t{70});
        QCOMPARE(s.gpuStart.value(), std::uint64_t{10});
        QCOMPARE(s.gpuEnd.value(), std::uint64_t{20});
        QCOMPARE(s.gpuPeak.value(), std::uint64_t{30});
        tracker.reset();
        tracker.sample({5, 5, 5}, std::nullopt);
        QCOMPARE(tracker.summary().privateStart, std::uint64_t{5});
        QVERIFY(!tracker.summary().gpuPeak.has_value());
    }

    void machineInfoHasBuildData()
    {
        const MachineInfo info = readMachineInfo();
        QVERIFY(!info.buildType.isEmpty());
        QVERIFY(!info.commit.isEmpty());
        QVERIFY(!info.compilerFlags.isEmpty());
        QVERIFY(info.qtVersion.startsWith(QStringLiteral("6.12")));
    }

    void gpuReaderIsNullInSoftware()
    {
        QQuickWindow window;
        window.resize(100, 100);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QCOMPARE(window.rendererInterface()->graphicsApi(), QSGRendererInterface::Software);
        QVERIFY(GpuMemoryReader::create(&window) == nullptr);
    }
};

QTEST_MAIN(TstBenchMemory)
#include "tst_bench_memory.moc"
```

Dans `tests/bench.cmake`, ajouter :

```cmake
actcraft_add_test(tst_bench_json
    SOURCES tst_bench_json.cpp
    LIBS ActCraft::bench)

actcraft_add_test(tst_bench_memory
    SOURCES tst_bench_memory.cpp
    LIBS ActCraft::bench Qt6::Quick)
```
Commande bornée : `TESTS EN ECHEC (compilation)`.

- [ ] **Étape 2 : mémoire.** Créer `bench/include/actcraft/bench/memory.h` :

```cpp
#pragma once

#include <QString>

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>

class QQuickWindow;

namespace actcraft::bench {

struct ProcessMemory
{
    std::uint64_t privateBytes = 0;        // Windows : PrivateUsage ; macOS : phys_footprint
    std::uint64_t workingSetBytes = 0;     // ensemble de travail / résident
    std::uint64_t peakWorkingSetBytes = 0; // pic tenu par le système
};

// Valeurs instantanées ; zéros sur une plateforme non prise en charge.
ProcessMemory readProcessMemory() noexcept;

// Mémoire graphique déclarée par le pilote (décision 26).
class GpuMemoryReader
{
public:
    virtual ~GpuMemoryReader() = default;
    // Fil de rendu. nullptr si le rendu n'expose pas sa mémoire (logiciel, plateforme non prise en charge).
    static std::unique_ptr<GpuMemoryReader> create(QQuickWindow* window);
    virtual std::optional<std::uint64_t> currentBytes() = 0;
    virtual QString adapterName() const = 0;
};

struct MemorySummary
{
    std::uint64_t privateStart = 0;
    std::uint64_t privateEnd = 0;
    std::uint64_t privatePeak = 0;
    std::uint64_t workingSetPeak = 0;
    std::optional<std::uint64_t> gpuStart;
    std::optional<std::uint64_t> gpuEnd;
    std::optional<std::uint64_t> gpuPeak;
};

// Début, fin et pic d'un scénario ; sample() est appelé à chaque image depuis le fil de rendu.
class MemoryTracker
{
public:
    void reset();
    void sample(const ProcessMemory& process, std::optional<std::uint64_t> gpu);
    MemorySummary summary() const;

private:
    mutable std::mutex m_mutex;
    bool m_started = false;
    MemorySummary m_summary;
};

} // namespace actcraft::bench
```

Créer `bench/src/memory.cpp` :

```cpp
#include <actcraft/bench/memory.h>

#include <QtGlobal>

#include <algorithm>

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#elif defined(Q_OS_MACOS)
#include <mach/mach.h>
#endif

namespace actcraft::bench {

ProcessMemory readProcessMemory() noexcept
{
    ProcessMemory m;
#if defined(Q_OS_WIN)
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
                             sizeof(counters))) {
        m.privateBytes = counters.PrivateUsage;
        m.workingSetBytes = counters.WorkingSetSize;
        m.peakWorkingSetBytes = counters.PeakWorkingSetSize;
    }
#elif defined(Q_OS_MACOS)
    task_vm_info_data_t info{};
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_VM_INFO, reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS) {
        m.privateBytes = static_cast<std::uint64_t>(info.phys_footprint);
        m.workingSetBytes = static_cast<std::uint64_t>(info.resident_size);
        m.peakWorkingSetBytes = static_cast<std::uint64_t>(info.resident_size_peak);
    }
#endif
    return m;
}

void MemoryTracker::reset()
{
    std::lock_guard lock(m_mutex);
    m_started = false;
    m_summary = MemorySummary{};
}

void MemoryTracker::sample(const ProcessMemory& process, std::optional<std::uint64_t> gpu)
{
    std::lock_guard lock(m_mutex);
    if (!m_started) {
        m_started = true;
        m_summary.privateStart = process.privateBytes;
        m_summary.gpuStart = gpu;
    }
    m_summary.privateEnd = process.privateBytes;
    m_summary.privatePeak = std::max(m_summary.privatePeak, process.privateBytes);
    m_summary.workingSetPeak =
        std::max({m_summary.workingSetPeak, process.workingSetBytes, process.peakWorkingSetBytes});
    m_summary.gpuEnd = gpu;
    if (gpu) {
        m_summary.gpuPeak = std::max(m_summary.gpuPeak.value_or(0), *gpu);
    }
}

MemorySummary MemoryTracker::summary() const
{
    std::lock_guard lock(m_mutex);
    return m_summary;
}

} // namespace actcraft::bench
```
(Le test attend `workingSetPeak == 70` : avec les échantillons `{100, 50, 50}`, `{300, 70, 70}`,
`{200, 60, 70}`, le maximum de l'ensemble de travail et de son pic vaut bien 70.)

Créer `bench/src/gpumemory_win.cpp` :

```cpp
#include <actcraft/bench/memory.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d11.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include <QQuickWindow>
#include <QSGRendererInterface>

namespace actcraft::bench {

namespace {

using Microsoft::WRL::ComPtr;

class D3D11GpuMemory final : public GpuMemoryReader
{
public:
    D3D11GpuMemory(ComPtr<IDXGIAdapter3> adapter, QString name) : m_adapter(std::move(adapter)), m_name(std::move(name))
    {
    }

    std::optional<std::uint64_t> currentBytes() override
    {
        DXGI_QUERY_VIDEO_MEMORY_INFO local{};
        DXGI_QUERY_VIDEO_MEMORY_INFO nonLocal{};
        if (FAILED(m_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &local))) {
            return std::nullopt;
        }
        if (FAILED(m_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonLocal))) {
            nonLocal.CurrentUsage = 0;
        }
        return static_cast<std::uint64_t>(local.CurrentUsage + nonLocal.CurrentUsage);
    }

    QString adapterName() const override { return m_name; }

private:
    ComPtr<IDXGIAdapter3> m_adapter;
    QString m_name;
};

} // namespace

std::unique_ptr<GpuMemoryReader> GpuMemoryReader::create(QQuickWindow* window)
{
    if (!window || window->rendererInterface()->graphicsApi() != QSGRendererInterface::Direct3D11) {
        return nullptr;
    }
    auto* device = static_cast<ID3D11Device*>(
        window->rendererInterface()->getResource(window, QSGRendererInterface::DeviceResource));
    if (!device) {
        return nullptr;
    }
    ComPtr<IDXGIDevice> dxgiDevice;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)))) {
        return nullptr;
    }
    ComPtr<IDXGIAdapter> adapter;
    if (FAILED(dxgiDevice->GetAdapter(&adapter))) {
        return nullptr;
    }
    ComPtr<IDXGIAdapter3> adapter3;
    if (FAILED(adapter.As(&adapter3))) {
        return nullptr;
    }
    DXGI_ADAPTER_DESC desc{};
    adapter3->GetDesc(&desc);
    return std::make_unique<D3D11GpuMemory>(adapter3, QString::fromWCharArray(desc.Description));
}

} // namespace actcraft::bench
```

Créer `bench/src/gpumemory_mac.mm` :

```objc
#include <actcraft/bench/memory.h>

#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QString>

#import <Metal/Metal.h>

namespace actcraft::bench {

namespace {

class MetalGpuMemory final : public GpuMemoryReader
{
public:
    explicit MetalGpuMemory(id<MTLDevice> device) : m_device(device) {}

    std::optional<std::uint64_t> currentBytes() override
    {
        return static_cast<std::uint64_t>([m_device currentAllocatedSize]);
    }

    QString adapterName() const override { return QString::fromNSString([m_device name]); }

private:
    id<MTLDevice> m_device; // appartient à la fenêtre, qui vit plus longtemps que le banc
};

} // namespace

std::unique_ptr<GpuMemoryReader> GpuMemoryReader::create(QQuickWindow* window)
{
    if (!window || window->rendererInterface()->graphicsApi() != QSGRendererInterface::Metal) {
        return nullptr;
    }
    void* resource = window->rendererInterface()->getResource(window, QSGRendererInterface::DeviceResource);
    if (!resource) {
        return nullptr;
    }
    return std::make_unique<MetalGpuMemory>((__bridge id<MTLDevice>)resource);
}

} // namespace actcraft::bench
```

Créer `bench/src/gpumemory_other.cpp` :

```cpp
#include <actcraft/bench/memory.h>

namespace actcraft::bench {

std::unique_ptr<GpuMemoryReader> GpuMemoryReader::create(QQuickWindow*)
{
    return nullptr;
}

} // namespace actcraft::bench
```

- [ ] **Étape 3 : machine et construction.** Créer `bench/src/buildinfo.h.in` :

```c
#pragma once
// Généré par CMake (bench/CMakeLists.txt) à chaque configuration.
#define ACTCRAFT_GIT_COMMIT "@ACTCRAFT_GIT_COMMIT@"
#define ACTCRAFT_GIT_DIRTY @ACTCRAFT_GIT_DIRTY@
#define ACTCRAFT_BUILD_TYPE "@CMAKE_BUILD_TYPE@"
#define ACTCRAFT_CXX_FLAGS "@ACTCRAFT_CXX_FLAGS@"
```

Créer `bench/include/actcraft/bench/machineinfo.h` :

```cpp
#pragma once

#include <QString>

namespace actcraft::bench {

// Métadonnées du passage (décision 27). Jamais le nom d'hôte ni le nom d'utilisateur.
struct MachineInfo
{
    QString model; // modèle matériel (Windows : fabricant + produit du BIOS ; macOS : hw.model)
    QString cpu;
    QString os;
    QString kernel;
    QString qtVersion;
    QString commit;
    bool commitDirty = false;
    QString buildType;
    QString compilerFlags;
};

MachineInfo readMachineInfo();

// Lettres ASCII et chiffres ; toute autre suite de caractères devient « - » ; vide → « inconnu ».
QString sanitizeForFileName(const QString& text);

} // namespace actcraft::bench
```

Créer `bench/src/machineinfo.cpp` :

```cpp
#include "buildinfo.h"

#include <actcraft/bench/machineinfo.h>

#include <QSettings>
#include <QSysInfo>
#include <QtGlobal>

#ifdef Q_OS_MACOS
#include <string>
#include <sys/sysctl.h>
#endif

namespace actcraft::bench {

namespace {
#ifdef Q_OS_MACOS
QString sysctlString(const char* name)
{
    size_t size = 0;
    if (sysctlbyname(name, nullptr, &size, nullptr, 0) != 0 || size == 0) {
        return {};
    }
    std::string buffer(size, '\0');
    if (sysctlbyname(name, buffer.data(), &size, nullptr, 0) != 0) {
        return {};
    }
    return QString::fromUtf8(buffer.c_str()).trimmed();
}
#endif
} // namespace

MachineInfo readMachineInfo()
{
    MachineInfo info;
#if defined(Q_OS_WIN)
    const QSettings bios(QStringLiteral("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\BIOS"),
                         QSettings::NativeFormat);
    info.model = (bios.value(QStringLiteral("SystemManufacturer")).toString() + QLatin1Char(' ') +
                  bios.value(QStringLiteral("SystemProductName")).toString())
                     .trimmed();
    const QSettings cpu(QStringLiteral("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"),
                        QSettings::NativeFormat);
    info.cpu = cpu.value(QStringLiteral("ProcessorNameString")).toString().trimmed();
#elif defined(Q_OS_MACOS)
    info.model = sysctlString("hw.model");
    info.cpu = sysctlString("machdep.cpu.brand_string");
#endif
    info.os = QSysInfo::prettyProductName();
    info.kernel = QSysInfo::kernelVersion();
    info.qtVersion = QString::fromLatin1(qVersion());
    info.commit = QStringLiteral(ACTCRAFT_GIT_COMMIT);
    info.commitDirty = ACTCRAFT_GIT_DIRTY != 0;
    info.buildType = QStringLiteral(ACTCRAFT_BUILD_TYPE);
    info.compilerFlags = QStringLiteral(ACTCRAFT_CXX_FLAGS);
    return info;
}

QString sanitizeForFileName(const QString& text)
{
    QString out;
    bool dash = false;
    for (const QChar c : text) {
        if ((c >= QLatin1Char('a') && c <= QLatin1Char('z')) || (c >= QLatin1Char('A') && c <= QLatin1Char('Z')) ||
            (c >= QLatin1Char('0') && c <= QLatin1Char('9'))) {
            if (dash && !out.isEmpty()) {
                out += QLatin1Char('-');
            }
            out += c;
            dash = false;
        } else {
            dash = true;
        }
    }
    return out.isEmpty() ? QStringLiteral("inconnu") : out;
}

} // namespace actcraft::bench
```

- [ ] **Étape 4 : résultat et JSON.** Créer `bench/include/actcraft/bench/result.h` :

```cpp
#pragma once

#include <actcraft/bench/machineinfo.h>
#include <actcraft/bench/memory.h>
#include <actcraft/bench/stats.h>

#include <QDateTime>
#include <QJsonObject>
#include <QSGRendererInterface>
#include <QSize>
#include <QString>
#include <QStringList>

#include <optional>
#include <vector>

namespace actcraft::bench {

inline constexpr char kResultFormat[] = "actcraft-bench/1";

struct ContentCheck
{
    QString name;
    bool ok = false;
    QString expected;
    QString actual;
};

struct Criterion
{
    QString scenario;
    QString name;
    double value = 0;
    double limit = 0;
    bool ok = false;
};

struct ScenarioResult
{
    QString name; // stroke | zoom | scroll
    std::int64_t startNs = 0; // origine des séries brutes
    FrameSeries frames;
    IntervalStats intervals;
    std::optional<LatencyStats> latency; // tracé seulement
    std::vector<double> injectionDelayMs;
    std::vector<double> receptionDelayMs;
    MemorySummary memory;
    bool exposureLost = false;
};

struct BenchResult
{
    QDateTime startedAt;
    MachineInfo machine;
    QString graphicsApi = QStringLiteral("unknown");
    QString requestedBackend;
    QString renderLoop;
    QString gpuName;
    bool cache = true;
    QString scenarioSet;
    QString windowMode;
    QSize windowSize;
    QString screenName;
    QSize screenSize;
    double refreshHz = 0;
    double devicePixelRatio = 1;
    bool activated = false;
    std::vector<ScenarioResult> scenarios;
    std::vector<ContentCheck> contentChecks;
    std::vector<Criterion> criteria;
    QStringList invalidReasons;
    bool valid = false;
    bool withinBudget = false;
    bool informative = false;
};

QString apiName(QSGRendererInterface::GraphicsApi api);
// Au moins un contrôle, et tous réussis.
bool contentCheckOk(const BenchResult& result);
QJsonObject toJson(const BenchResult& result);
// <AAAA-MM-JJ_HHMMSS>-<modèle>-<rendu>.json
QString resultFileName(const BenchResult& result);
// Crée les dossiers manquants ; écriture atomique.
bool writeResult(const BenchResult& result, const QString& path, QString& error);

} // namespace actcraft::bench
```

Créer `bench/src/result.cpp` :

```cpp
#include <actcraft/bench/result.h>

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

namespace actcraft::bench {

namespace {

QJsonObject percentiles(const Percentiles& p)
{
    return {{"p50", p.p50}, {"p95", p.p95}, {"p99", p.p99}, {"max", p.max}, {"count", static_cast<double>(p.count)}};
}

QJsonArray relative(const std::vector<std::int64_t>& values, std::int64_t origin)
{
    QJsonArray out;
    for (std::int64_t v : values) {
        out.append(static_cast<double>(v - origin));
    }
    return out;
}

QJsonArray doubles(const std::vector<double>& values)
{
    QJsonArray out;
    for (double v : values) {
        out.append(v);
    }
    return out;
}

QJsonValue bytes(const std::optional<std::uint64_t>& v)
{
    return v ? QJsonValue(static_cast<double>(*v)) : QJsonValue(QJsonValue::Null);
}

QString classes(const std::vector<IntervalClass>& values)
{
    QString out;
    for (IntervalClass c : values) {
        out += c == IntervalClass::Active ? QLatin1Char('A') : c == IntervalClass::Freeze ? QLatin1Char('F') : QLatin1Char('I');
    }
    return out;
}

QJsonObject scenarioJson(const ScenarioResult& s)
{
    const IntervalStats& iv = s.intervals;
    const std::size_t intervals = iv.classes.size();
    QJsonObject frames{
        {"count", static_cast<double>(s.frames.swapNs.size())},
        {"intervals", static_cast<double>(intervals)},
        {"active", static_cast<double>(iv.activeMs.size())},
        {"freeze", static_cast<double>(iv.freezeMs.size())},
        {"idle", static_cast<double>(intervals - iv.activeMs.size() - iv.freezeMs.size())},
        {"fps", iv.fps},
        {"p50_ms", iv.active.p50},
        {"p95_ms", iv.active.p95},
        {"p99_ms", iv.active.p99},
        {"max_ms", iv.active.max},
        {"over_25ms_ratio", iv.over25Ratio},
        {"freeze_max_ms", iv.freezeMaxMs},
        {"freeze_over_limit", static_cast<double>(iv.freezeOverLimit)},
        {"refresh_period_ms", static_cast<double>(s.frames.refreshPeriodNs) / 1'000'000.0},
    };
    QJsonValue latency(QJsonValue::Null);
    QJsonArray latencySeq;
    if (s.latency) {
        latency = QJsonObject{
            {"samples", static_cast<double>(s.latency->seq.size())},
            {"unmatched", static_cast<double>(s.latency->unmatched)},
            {"total_ms", percentiles(s.latency->total)},
            {"planned_to_sync_ms", percentiles(s.latency->plannedToSync)},
            {"sync_to_swap_ms", percentiles(s.latency->syncToSwap)},
        };
        for (std::uint64_t seq : s.latency->seq) {
            latencySeq.append(static_cast<double>(seq));
        }
    }
    const QJsonObject input{
        {"count", static_cast<double>(s.frames.inputNs.size())},
        {"injection_delay_ms", percentiles(summarize(s.injectionDelayMs))},
        {"reception_delay_ms", percentiles(summarize(s.receptionDelayMs))},
    };
    const QJsonObject memory{
        {"process_private_start_bytes", static_cast<double>(s.memory.privateStart)},
        {"process_private_end_bytes", static_cast<double>(s.memory.privateEnd)},
        {"process_private_peak_bytes", static_cast<double>(s.memory.privatePeak)},
        {"working_set_peak_bytes", static_cast<double>(s.memory.workingSetPeak)},
        {"gpu_start_bytes", bytes(s.memory.gpuStart)},
        {"gpu_end_bytes", bytes(s.memory.gpuEnd)},
        {"gpu_peak_bytes", bytes(s.memory.gpuPeak)},
    };
    const QJsonObject raw{
        {"swap_ns", relative(s.frames.swapNs, s.startNs)},
        {"interval_class", classes(iv.classes)},
        {"input_ns", relative(s.frames.inputNs, s.startNs)},
        {"freeze_ns", relative(s.frames.freezeNs, s.startNs)},
        {"latency_seq", latencySeq},
        {"latency_total_ms", s.latency ? doubles(s.latency->totalMs) : QJsonArray()},
        {"latency_planned_to_sync_ms", s.latency ? doubles(s.latency->plannedToSyncMs) : QJsonArray()},
        {"latency_sync_to_swap_ms", s.latency ? doubles(s.latency->syncToSwapMs) : QJsonArray()},
    };
    return {{"name", s.name},     {"exposure_lost", s.exposureLost}, {"frames", frames}, {"latency", latency},
            {"input", input},     {"memory", memory},                {"raw", raw}};
}

} // namespace

QString apiName(QSGRendererInterface::GraphicsApi api)
{
    switch (api) {
    case QSGRendererInterface::Software:
        return QStringLiteral("software");
    case QSGRendererInterface::Direct3D11:
        return QStringLiteral("direct3d11");
    case QSGRendererInterface::Direct3D12:
        return QStringLiteral("direct3d12");
    case QSGRendererInterface::Metal:
        return QStringLiteral("metal");
    case QSGRendererInterface::Vulkan:
        return QStringLiteral("vulkan");
    case QSGRendererInterface::OpenGL:
        return QStringLiteral("opengl");
    case QSGRendererInterface::Null:
        return QStringLiteral("null");
    default:
        return QStringLiteral("unknown");
    }
}

bool contentCheckOk(const BenchResult& result)
{
    if (result.contentChecks.empty()) {
        return false;
    }
    for (const ContentCheck& c : result.contentChecks) {
        if (!c.ok) {
            return false;
        }
    }
    return true;
}

QJsonObject toJson(const BenchResult& r)
{
    const QJsonObject meta{
        {"commit", r.machine.commit},
        {"commit_dirty", r.machine.commitDirty},
        {"qt_version", r.machine.qtVersion},
        {"os", r.machine.os},
        {"kernel", r.machine.kernel},
        {"cpu", r.machine.cpu},
        {"machine_model", r.machine.model},
        {"gpu", r.gpuName},
        {"requested_backend", r.requestedBackend},
        {"render_loop", r.renderLoop},
        {"build_type", r.machine.buildType},
        {"compiler_flags", r.machine.compilerFlags},
        {"scenario_set", r.scenarioSet},
        {"activated", r.activated},
        {"screen",
         QJsonObject{{"name", r.screenName},
                     {"width", r.screenSize.width()},
                     {"height", r.screenSize.height()},
                     {"refresh_hz", r.refreshHz},
                     {"device_pixel_ratio", r.devicePixelRatio}}},
        {"window",
         QJsonObject{{"mode", r.windowMode}, {"width", r.windowSize.width()}, {"height", r.windowSize.height()}}},
        {"limits", QStringLiteral("Latence interne : de l'instant prévu de l'échantillon au frameSwapped de la première "
                                  "image qui le contient. Ni le numériseur, ni la pile d'entrée du système, ni la "
                                  "composition (DWM), ni le balayage de l'écran. Entrées synthétiques postées à la "
                                  "fenêtre (QTabletEvent, QWheelEvent).")},
    };
    QJsonArray scenarios;
    for (const ScenarioResult& s : r.scenarios) {
        scenarios.append(scenarioJson(s));
    }
    QJsonArray checks;
    for (const ContentCheck& c : r.contentChecks) {
        checks.append(QJsonObject{{"name", c.name}, {"ok", c.ok}, {"expected", c.expected}, {"actual", c.actual}});
    }
    QJsonArray criteria;
    for (const Criterion& c : r.criteria) {
        criteria.append(QJsonObject{
            {"scenario", c.scenario}, {"criterion", c.name}, {"value", c.value}, {"limit", c.limit}, {"ok", c.ok}});
    }
    const QJsonObject verdict{
        {"valid_for_measurement", r.valid},
        {"invalid_reasons", QJsonArray::fromStringList(r.invalidReasons)},
        {"within_budget", r.withinBudget},
        {"informative", r.informative},
        {"criteria", criteria},
    };
    return {
        {"format", QString::fromLatin1(kResultFormat)},
        {"started_at", r.startedAt.toString(Qt::ISODate)},
        {"graphics_api", r.graphicsApi},
        {"cache", r.cache},
        {"valid_for_measurement", r.valid},
        {"within_budget", r.withinBudget},
        {"informative", r.informative},
        {"meta", meta},
        {"scenarios", scenarios},
        {"content_check", QJsonObject{{"ok", contentCheckOk(r)}, {"checks", checks}}},
        {"verdict", verdict},
    };
}

QString resultFileName(const BenchResult& r)
{
    return r.startedAt.toString(QStringLiteral("yyyy-MM-dd_HHmmss")) + QLatin1Char('-') +
           sanitizeForFileName(r.machine.model) + QLatin1Char('-') + sanitizeForFileName(r.graphicsApi) +
           QStringLiteral(".json");
}

bool writeResult(const BenchResult& result, const QString& path, QString& error)
{
    const QFileInfo info(path);
    if (!QDir().mkpath(info.absolutePath())) {
        error = QStringLiteral("dossier impossible à créer : %1").arg(info.absolutePath());
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        error = file.errorString();
        return false;
    }
    file.write(QJsonDocument(toJson(result)).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        error = file.errorString();
        return false;
    }
    return true;
}

} // namespace actcraft::bench
```

- [ ] **Étape 5 : CMake.** Remplacer `bench/CMakeLists.txt` par :

```cmake
# Banc de fluidité : temps par image, latence d'encre, mémoire.
if(WIN32)
    set(_bench_gpu_source src/gpumemory_win.cpp)
elseif(APPLE)
    set(_bench_gpu_source src/gpumemory_mac.mm)
else()
    set(_bench_gpu_source src/gpumemory_other.cpp)
endif()

actcraft_add_module(bench
    SOURCES
        src/bench.cpp
        src/stats.cpp
        src/memory.cpp
        src/machineinfo.cpp
        src/result.cpp
        ${_bench_gpu_source}
    DEPENDS
        ActCraft::core ActCraft::render Qt6::Quick)

if(WIN32)
    target_link_libraries(actcraft_bench PRIVATE dxgi psapi)
elseif(APPLE)
    target_link_libraries(actcraft_bench PRIVATE "-framework Metal" "-framework Foundation")
endif()

# Métadonnées de construction écrites dans le JSON (décisions 27 et 28), relues à chaque configuration.
execute_process(COMMAND git rev-parse --short=12 HEAD
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE ACTCRAFT_GIT_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT ACTCRAFT_GIT_COMMIT)
    set(ACTCRAFT_GIT_COMMIT "inconnu")
endif()
execute_process(COMMAND git status --porcelain --untracked-files=no
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE _bench_git_status OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(_bench_git_status)
    set(ACTCRAFT_GIT_DIRTY 1)
else()
    set(ACTCRAFT_GIT_DIRTY 0)
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" _bench_build_type)
string(STRIP "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${_bench_build_type}}" ACTCRAFT_CXX_FLAGS)
configure_file(src/buildinfo.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/buildinfo.h @ONLY)
target_include_directories(actcraft_bench PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
```
Si Clang refuse le `.mm` à cause de `-Wpedantic` (constaté en CI seulement), ajouter
`set_source_files_properties(src/gpumemory_mac.mm PROPERTIES COMPILE_OPTIONS "-Wno-pedantic")` et le signaler.

- [ ] **Étape 6 : vert, contrôle `/Ob2`, format, commit.** Commande bornée : `tst_bench_json ... Passed`,
`tst_bench_memory ... Passed`, `TESTS OK`. Puis, sans compiler la release :

```powershell
Set-Location <worktree>; pwsh -NoProfile -Command ". ./scripts/dev-env.ps1; cmake --preset release *> .tmp/cfg-release.txt"; Select-String -Path build/release/bench/generated/buildinfo.h -Pattern 'ACTCRAFT_CXX_FLAGS'
```
Attendu : la ligne contient `/Ob2`.

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add bench/CMakeLists.txt bench/include/actcraft/bench/memory.h bench/include/actcraft/bench/machineinfo.h bench/include/actcraft/bench/result.h bench/src/memory.cpp bench/src/gpumemory_win.cpp bench/src/gpumemory_mac.mm bench/src/gpumemory_other.cpp bench/src/machineinfo.cpp bench/src/result.cpp bench/src/buildinfo.h.in tests/bench.cmake tests/tst_bench_json.cpp tests/tst_bench_memory.cpp
git -C <worktree> commit -m "bench : mémoire du processus et du GPU, métadonnées, fichier JSON actcraft-bench/1" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

### Tâche T15 — `bench` : scénarios et injecteur avec rattrapage

**Objectif.** Les scénarios rejoués (décision 22, N2-1) et l'injecteur (décision 23, DA6) :
- **tracé** : 10 traits de 1,5 s à 200 points/s (300 échantillons, pression variable), stylo levé 300 ms entre
  deux traits, dans la zone centrale de la page (x 220–804, y 150–618 : hors des témoins et de la bande de
  démonstration y = 670) ; appui et déplacements mesurés, relâché non mesuré ;
- **zoom** : 100 → 800 → 25 → 100 % en 6 s, molette fine Ctrl à 120 Hz, deltas entiers dont la somme est exacte ;
- **défilement** : à 400 %, page calée en haut à gauche, aller-retour vertical puis horizontal en 6 s (4 × 1,5 s
  à 120 Hz, 10 px par événement) ;
- `court` : durées et nombres d'événements divisés par 5 ;
- trait de **démonstration** non mesuré (bande y = 670) et son point de contrôle d'encre ;
- **injecteur** : `QTabletEvent` et `QWheelEvent` postés à la fenêtre depuis deux périphériques synthétiques,
  instant prévu t0 + décalage ; à chaque déclenchement, il poste **tous** les événements dont l'instant prévu
  est passé (rattrapage), note l'instant réel de chaque envoi ; minuteur précis réarmé sur l'échéance suivante
  (arrondie au-dessus : jamais d'attente active). Pas de `SendInput` : la souris de Jérôme n'est jamais pilotée.

**Le piège.** Le patron évident — un `QTimer` de 5 ms qui poste un événement par déclenchement — prend du retard
dès que le fil principal est occupé et ne le rattrape jamais : la latence mesurée serait sous-estimée. Le test
`blockedMainThreadBacklogIsPostedAtOnce` bloque le fil principal 50 ms et exige que l'arriéré parte d'un coup.

**Fichiers.**
- Créer : `bench/include/actcraft/bench/scenarios.h`, `bench/include/actcraft/bench/injector.h`,
  `bench/src/scenarios.cpp`, `bench/src/injector.cpp`, `tests/tst_bench_scenarios.cpp`,
  `tests/tst_bench_injector.cpp`
- Modifier : `bench/CMakeLists.txt`, `tests/bench.cmake`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_bench_(scenarios|injector) .*(Passed|Failed)' | Select-Object -Last 4
```
Attendu : `out of N` avec N ≥ 12, `tst_bench_scenarios ... Passed`, `tst_bench_injector ... Passed`, `TESTS OK`.

**Interdits.** Pas de `SendInput`, `QTest::mouse*` ni `QCursor::setPos` dans `bench/`. Pas de
`QWindowSystemInterface` (API privée, N2-14). Le relâché n'est jamais « mesuré ».

**Terminé quand.** Preuve verte ; commit fait.

**Étapes.**

- [ ] **Étape 1 : tests des scénarios (rouges).** Créer `tests/tst_bench_scenarios.cpp` :

```cpp
#include <actcraft/bench/scenarios.h>
#include <actcraft/core/generator.h>
#include <actcraft/ui/viewtransform.h>

#include <QTest>

#include <cmath>
#include <numeric>

using namespace actcraft;
using bench::InputKind;

class TstBenchScenarios : public QObject
{
    Q_OBJECT

private slots:
    void strokeScenarioShape()
    {
        const bench::ScenarioPlan plan = bench::strokeScenario(bench::ScenarioSet::Complet);
        QCOMPARE(plan.name, QStringLiteral("stroke"));
        QCOMPARE(plan.zoom, 1.0);
        QCOMPARE(plan.inputs.size(), std::size_t{3000});
        int presses = 0;
        int releases = 0;
        for (std::size_t i = 0; i < plan.inputs.size(); ++i) {
            const auto& in = plan.inputs[i];
            presses += in.kind == InputKind::TabletPress ? 1 : 0;
            releases += in.kind == InputKind::TabletRelease ? 1 : 0;
            QCOMPARE(in.measured, in.kind != InputKind::TabletRelease);
            if (i > 0) {
                QVERIFY(in.offsetNs > plan.inputs[i - 1].offsetNs);
                const std::int64_t gap = in.offsetNs - plan.inputs[i - 1].offsetNs;
                QVERIFY(gap == bench::kStylusPeriodNs || (in.kind == InputKind::TabletPress && gap >= 300'000'000));
            }
        }
        QCOMPARE(presses, 10);
        QCOMPARE(releases, 10);
    }

    void strokesStayInTheCentralZone()
    {
        for (const auto& in : bench::strokeScenario(bench::ScenarioSet::Complet).inputs) {
            QVERIFY(!core::hitsWitness({in.pagePos.x(), in.pagePos.y()}, 16.0));
            QVERIFY(in.pagePos.y() >= 149.0 && in.pagePos.y() <= 619.0);
            QVERIFY(in.pagePos.x() >= 200.0 && in.pagePos.x() <= 824.0);
        }
    }

    void courtDividesByFive()
    {
        const auto complet = bench::strokeScenario(bench::ScenarioSet::Complet);
        const auto court = bench::strokeScenario(bench::ScenarioSet::Court);
        QCOMPARE(court.inputs.size(), std::size_t{600});
        const double ratio = static_cast<double>(bench::planDurationNs(complet)) /
                             static_cast<double>(bench::planDurationNs(court));
        QVERIFY(ratio > 4.8 && ratio < 5.2);
        QCOMPARE(bench::zoomScenario(bench::ScenarioSet::Court).inputs.size(), std::size_t{144});
        QCOMPARE(bench::scrollScenario(bench::ScenarioSet::Court).inputs.size(), std::size_t{144});
    }

    void zoomScenarioReachesItsTargets()
    {
        const auto plan = bench::zoomScenario(bench::ScenarioSet::Complet);
        QCOMPARE(plan.inputs.size(), std::size_t{720});
        double zoom = 1.0;
        double highest = 1.0;
        double lowest = 1.0;
        for (const auto& in : plan.inputs) {
            QCOMPARE(in.kind, InputKind::Wheel);
            QVERIFY(in.modifiers & Qt::ControlModifier);
            zoom = ui::clampZoom(zoom * ui::wheelZoomFactor(in.angleDelta.y()));
            highest = std::max(highest, zoom);
            lowest = std::min(lowest, zoom);
        }
        QVERIFY(std::abs(highest - 8.0) < 0.08);
        QVERIFY(std::abs(lowest - 0.25) < 0.0025);
        QVERIFY(std::abs(zoom - 1.0) < 0.01);
        const double seconds = static_cast<double>(bench::planDurationNs(plan)) / 1e9;
        QVERIFY(seconds > 5.9 && seconds < 6.1);
    }

    void scrollScenarioLegs()
    {
        const auto plan = bench::scrollScenario(bench::ScenarioSet::Complet);
        QCOMPARE(plan.zoom, 4.0);
        QVERIFY(plan.topLeft);
        QCOMPARE(plan.inputs.size(), std::size_t{720});
        QCOMPARE(plan.inputs[0].angleDelta, QPoint(0, -20));
        QCOMPARE(plan.inputs[180].angleDelta, QPoint(0, 20));
        QCOMPARE(plan.inputs[360].angleDelta, QPoint(-20, 0));
        QCOMPARE(plan.inputs[540].angleDelta, QPoint(20, 0));
        QCOMPARE(plan.inputs[0].modifiers, Qt::KeyboardModifiers(Qt::NoModifier));
    }

    void cumulativeSplitIsExact()
    {
        const std::vector<int> parts = bench::cumulativeSplit(1785, 216);
        QCOMPARE(parts.size(), std::size_t{216});
        QCOMPARE(std::accumulate(parts.begin(), parts.end(), 0), 1785);
        const std::vector<int> negative = bench::cumulativeSplit(-2976, 360);
        QCOMPARE(std::accumulate(negative.begin(), negative.end(), 0), -2976);
    }

    void demoStrokeIsOutsideTheMeasuredZone()
    {
        const auto demo = bench::demoStroke();
        QVERIFY(demo.size() > bench::kDemoMidIndex + 10);
        QCOMPARE(demo.front().kind, InputKind::TabletPress);
        QCOMPARE(demo.back().kind, InputKind::TabletRelease);
        for (const auto& in : demo) {
            QVERIFY(!in.measured);
            QVERIFY(in.pagePos.y() > 640.0 && in.pagePos.y() < 700.0);
            QVERIFY(!core::hitsWitness({in.pagePos.x(), in.pagePos.y()}, 16.0));
        }
        const QPointF probe = bench::demoInkProbePoint();
        QVERIFY(probe.x() < demo[bench::kDemoMidIndex - 1].pagePos.x()); // déjà tracé à la capture mi-trait
    }
};

QTEST_GUILESS_MAIN(TstBenchScenarios)
#include "tst_bench_scenarios.moc"
```

- [ ] **Étape 2 : tests de l'injecteur (rouges).** Créer `tests/tst_bench_injector.cpp` :

```cpp
#include <actcraft/bench/injector.h>
#include <actcraft/core/clock.h>

#include <QSignalSpy>
#include <QTabletEvent>
#include <QTest>
#include <QTimer>
#include <QWheelEvent>
#include <QWindow>

#include <chrono>
#include <thread>

using namespace actcraft;
using bench::InputKind;

namespace {

class RecordingWindow : public QWindow
{
public:
    struct Received
    {
        QEvent::Type type;
        qreal pressure = 0;
        QPointF position;
        const QInputDevice* device = nullptr;
        QPoint angleDelta;
        Qt::KeyboardModifiers modifiers;
    };
    std::vector<Received> events;

protected:
    bool event(QEvent* e) override
    {
        switch (e->type()) {
        case QEvent::TabletPress:
        case QEvent::TabletMove:
        case QEvent::TabletRelease: {
            auto* t = static_cast<QTabletEvent*>(e);
            events.push_back({e->type(), t->pressure(), t->position(), t->pointingDevice(), {}, t->modifiers()});
            return true;
        }
        case QEvent::Wheel: {
            auto* w = static_cast<QWheelEvent*>(e);
            events.push_back({e->type(), 0, w->position(), w->pointingDevice(), w->angleDelta(), w->modifiers()});
            return true;
        }
        default:
            return QWindow::event(e);
        }
    }
};

std::vector<bench::PlannedInput> strokePlan(int count)
{
    std::vector<bench::PlannedInput> plan;
    for (int i = 0; i < count; ++i) {
        bench::PlannedInput in;
        in.offsetNs = i * bench::kStylusPeriodNs;
        in.kind = i == 0 ? InputKind::TabletPress : i == count - 1 ? InputKind::TabletRelease : InputKind::TabletMove;
        in.pagePos = QPointF(10 + i, 20);
        in.pressure = 0.5f;
        plan.push_back(in);
    }
    return plan;
}

} // namespace

class TstBenchInjector : public QObject
{
    Q_OBJECT

private slots:
    void catchUpPostsEveryDueEvent()
    {
        RecordingWindow window;
        bench::Injector injector(&window, [](QPointF p) { return p; });
        QSignalSpy finished(&injector, &bench::Injector::planFinished);
        const std::int64_t t0 = 1'000'000'000;
        injector.start(strokePlan(21), t0, false);
        QCOMPARE(injector.tick(t0), std::size_t{1});
        QCOMPARE(injector.tick(t0 + 50'000'000), std::size_t{10}); // 5 … 50 ms : tout l'arriéré d'un coup
        QCoreApplication::processEvents();
        QCOMPARE(window.events.size(), std::size_t{11});
        QCOMPARE(window.events[0].type, QEvent::TabletPress);
        QCOMPARE(injector.tick(t0 + 1'000'000'000), std::size_t{10});
        QVERIFY(injector.finished());
        QCOMPARE(finished.count(), 1);
        QCoreApplication::processEvents();
        QCOMPARE(window.events.back().type, QEvent::TabletRelease);
    }

    void blockedMainThreadBacklogIsPostedAtOnce()
    {
        RecordingWindow window;
        bench::Injector injector(&window, [](QPointF p) { return p; });
        QSignalSpy finished(&injector, &bench::Injector::planFinished);
        injector.start(strokePlan(40), core::monotonicNowNs() + 10'000'000);
        QTimer::singleShot(20, [] { std::this_thread::sleep_for(std::chrono::milliseconds(50)); });
        QVERIFY(finished.wait(3000));
        const auto& planned = injector.plannedNs();
        const auto& posted = injector.postedNs();
        std::size_t longestBurst = 1;
        std::size_t burst = 1;
        for (std::size_t i = 0; i < posted.size(); ++i) {
            QVERIFY(posted[i] >= planned[i]);
            if (i > 0) {
                burst = posted[i] - posted[i - 1] < 1'000'000 ? burst + 1 : 1;
                longestBurst = std::max(longestBurst, burst);
            }
        }
        QVERIFY2(longestBurst >= 9, qPrintable(QString::number(longestBurst)));
        QCoreApplication::processEvents();
        QCOMPARE(window.events.size(), std::size_t{40});
    }

    void wheelEventsCarryDeltaAndModifiers()
    {
        RecordingWindow window;
        window.resize(200, 100);
        bench::Injector injector(&window, [](QPointF p) { return p; });
        bench::PlannedInput wheel;
        wheel.kind = InputKind::Wheel;
        wheel.angleDelta = QPoint(0, 8);
        wheel.modifiers = Qt::ControlModifier;
        injector.start({wheel}, 0, false);
        injector.tick(0);
        QCoreApplication::processEvents();
        QCOMPARE(window.events.size(), std::size_t{1});
        QCOMPARE(window.events[0].angleDelta, QPoint(0, 8));
        QVERIFY(window.events[0].modifiers & Qt::ControlModifier);
        QCOMPARE(window.events[0].device, injector.wheel());
        QCOMPARE(window.events[0].position, QPointF(100, 50)); // centre de la fenêtre
    }

    void tabletEventsUseTheSyntheticStylusAndPageToScene()
    {
        RecordingWindow window;
        bench::Injector injector(&window, [](QPointF p) { return p * 2; });
        bench::PlannedInput press;
        press.kind = InputKind::TabletPress;
        press.pagePos = QPointF(10, 20);
        press.pressure = 0.7f;
        injector.start({press}, 0, false);
        injector.tick(0);
        QCoreApplication::processEvents();
        QCOMPARE(window.events[0].device, injector.stylus());
        QCOMPARE(window.events[0].position, QPointF(20, 40));
        QVERIFY(qAbs(window.events[0].pressure - 0.7) < 0.001);
    }
};

QTEST_MAIN(TstBenchInjector)
#include "tst_bench_injector.moc"
```

Dans `tests/bench.cmake`, ajouter :

```cmake
actcraft_add_test(tst_bench_scenarios
    SOURCES tst_bench_scenarios.cpp
    LIBS ActCraft::bench ActCraft::ui)

actcraft_add_test(tst_bench_injector
    SOURCES tst_bench_injector.cpp
    LIBS ActCraft::bench Qt6::Gui)
```
Commande bornée : `TESTS EN ECHEC (compilation)`.

- [ ] **Étape 3 : scénarios.** Créer `bench/include/actcraft/bench/scenarios.h` :

```cpp
#pragma once

#include <QPoint>
#include <QPointF>
#include <QString>
#include <Qt>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace actcraft::bench {

enum class InputKind : std::uint8_t
{
    TabletPress,
    TabletMove,
    TabletRelease,
    Wheel,
};

struct PlannedInput
{
    std::int64_t offsetNs = 0; // depuis le début du scénario
    InputKind kind = InputKind::TabletMove;
    QPointF pagePos;           // tablette : coordonnées de page, converties en scène au moment de poster
    float pressure = 0.5f;
    QPoint angleDelta;         // molette
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    bool measured = false;     // compte dans la latence d'encre (appui et déplacements du tracé)
};

enum class ScenarioSet
{
    Complet,
    Court,
};

struct ScenarioPlan
{
    QString name;         // stroke | zoom | scroll
    double zoom = 1.0;    // niveau imposé avant la mesure (décision 17)
    bool topLeft = false; // page calée en haut à gauche (défilement) ; sinon position par défaut
    std::vector<PlannedInput> inputs;
};

inline constexpr std::int64_t kStylusPeriodNs = 5'000'000; // 200 points/s
inline constexpr std::int64_t kWheelPeriodNs = 8'333'333;  // 120 Hz
inline constexpr std::size_t kDemoMidIndex = 60;           // capture « mi-trait » après 60 événements

int durationDivisor(ScenarioSet set) noexcept; // 1 (complet) ou 5 (court)
ScenarioPlan strokeScenario(ScenarioSet set);
ScenarioPlan zoomScenario(ScenarioSet set);
ScenarioPlan scrollScenario(ScenarioSet set);
std::vector<ScenarioPlan> allScenarios(ScenarioSet set); // stroke, zoom, scroll
// Trait de démonstration non mesuré, bande y = 670 (décision 31).
std::vector<PlannedInput> demoStroke();
// Point du trait de démonstration déjà tracé à la capture « mi-trait ».
QPointF demoInkProbePoint();
// `count` deltas entiers dont la somme vaut exactement `total`.
std::vector<int> cumulativeSplit(int total, int count);
std::int64_t planDurationNs(const ScenarioPlan& plan);

} // namespace actcraft::bench
```

Créer `bench/src/scenarios.cpp` :

```cpp
#include <actcraft/bench/scenarios.h>

#include <cmath>
#include <numbers>

namespace actcraft::bench {

namespace {
constexpr int kDemoEvents = 120;
constexpr double kDemoY = 670.0;
constexpr double kDemoX0 = 260.0;
constexpr double kDemoLength = 500.0;
} // namespace

int durationDivisor(ScenarioSet set) noexcept
{
    return set == ScenarioSet::Court ? 5 : 1;
}

ScenarioPlan strokeScenario(ScenarioSet set)
{
    const int d = durationDivisor(set);
    const int samples = 300 / d;                        // 1,5 s à 200 points/s
    const std::int64_t pauseNs = 300'000'000 / d;       // stylo levé entre deux traits
    ScenarioPlan plan;
    plan.name = QStringLiteral("stroke");
    plan.zoom = 1.0;
    std::int64_t start = 0;
    for (int k = 0; k < 10; ++k) {
        for (int i = 0; i < samples; ++i) {
            const double u = static_cast<double>(i) / (samples - 1);
            PlannedInput in;
            in.offsetNs = start + i * kStylusPeriodNs;
            in.kind = i == 0 ? InputKind::TabletPress
                      : i == samples - 1 ? InputKind::TabletRelease
                                         : InputKind::TabletMove;
            in.pagePos = QPointF(220.0 + 584.0 * u, 170.0 + 44.0 * k + 20.0 * std::sin(2 * std::numbers::pi * 3 * u));
            in.pressure = in.kind == InputKind::TabletRelease
                              ? 0.0f
                              : static_cast<float>(0.5 + 0.4 * std::sin(2 * std::numbers::pi * u));
            in.measured = in.kind != InputKind::TabletRelease;
            plan.inputs.push_back(in);
        }
        start += (samples - 1) * kStylusPeriodNs + pauseNs;
    }
    return plan;
}

std::vector<int> cumulativeSplit(int total, int count)
{
    std::vector<int> out;
    if (count <= 0) {
        return out;
    }
    out.reserve(static_cast<std::size_t>(count));
    long long previous = 0;
    for (int i = 1; i <= count; ++i) {
        const long long current = std::llround(static_cast<double>(total) * i / count);
        out.push_back(static_cast<int>(current - previous));
        previous = current;
    }
    return out;
}

ScenarioPlan zoomScenario(ScenarioSet set)
{
    const int totalEvents = 720 / durationDivisor(set); // 6 s à 120 Hz
    const double ratios[] = {8.0, 1.0 / 32.0, 4.0};      // 100 → 800 → 25 → 100 %
    const double totalLog = std::log(1024.0);
    ScenarioPlan plan;
    plan.name = QStringLiteral("zoom");
    plan.zoom = 1.0;
    std::int64_t index = 0;
    for (double ratio : ratios) {
        const int events = static_cast<int>(std::lround(totalEvents * std::abs(std::log(ratio)) / totalLog));
        const int totalDelta = static_cast<int>(std::lround(120.0 * std::log(ratio) / std::log(1.15)));
        for (int delta : cumulativeSplit(totalDelta, events)) {
            PlannedInput in;
            in.offsetNs = index * kWheelPeriodNs;
            in.kind = InputKind::Wheel;
            in.angleDelta = QPoint(0, delta);
            in.modifiers = Qt::ControlModifier;
            plan.inputs.push_back(in);
            ++index;
        }
    }
    return plan;
}

ScenarioPlan scrollScenario(ScenarioSet set)
{
    const int perLeg = 180 / durationDivisor(set); // 1,5 s à 120 Hz par aller ou retour
    const QPoint legs[] = {QPoint(0, -20), QPoint(0, 20), QPoint(-20, 0), QPoint(20, 0)};
    ScenarioPlan plan;
    plan.name = QStringLiteral("scroll");
    plan.zoom = 4.0; // à 200 % en 1920 px, l'horizontal ne parcourrait qu'environ 128 px
    plan.topLeft = true;
    std::int64_t index = 0;
    for (const QPoint& leg : legs) {
        for (int i = 0; i < perLeg; ++i) {
            PlannedInput in;
            in.offsetNs = index * kWheelPeriodNs;
            in.kind = InputKind::Wheel;
            in.angleDelta = leg;
            plan.inputs.push_back(in);
            ++index;
        }
    }
    return plan;
}

std::vector<ScenarioPlan> allScenarios(ScenarioSet set)
{
    return {strokeScenario(set), zoomScenario(set), scrollScenario(set)};
}

std::vector<PlannedInput> demoStroke()
{
    std::vector<PlannedInput> out;
    for (int i = 0; i < kDemoEvents; ++i) {
        PlannedInput in;
        in.offsetNs = i * kStylusPeriodNs;
        in.kind = i == 0 ? InputKind::TabletPress
                  : i == kDemoEvents - 1 ? InputKind::TabletRelease
                                         : InputKind::TabletMove;
        in.pagePos = QPointF(kDemoX0 + kDemoLength * i / (kDemoEvents - 1), kDemoY);
        in.pressure = in.kind == InputKind::TabletRelease ? 0.0f : 0.6f;
        in.measured = false;
        out.push_back(in);
    }
    return out;
}

QPointF demoInkProbePoint()
{
    return QPointF(kDemoX0 + kDemoLength * 30 / (kDemoEvents - 1), kDemoY);
}

std::int64_t planDurationNs(const ScenarioPlan& plan)
{
    return plan.inputs.empty() ? 0 : plan.inputs.back().offsetNs;
}

} // namespace actcraft::bench
```

- [ ] **Étape 4 : injecteur.** Créer `bench/include/actcraft/bench/injector.h` :

```cpp
#pragma once

#include <actcraft/bench/scenarios.h>

#include <QObject>
#include <QPointingDevice>
#include <QTimer>

#include <functional>
#include <memory>
#include <vector>

class QWindow;
class QEvent;

namespace actcraft::bench {

// Entrées synthétiques postées à la fenêtre (DA6), avec rattrapage. Les événements postés pointent vers les
// périphériques de l'injecteur : il doit vivre jusqu'à leur traitement.
class Injector : public QObject
{
    Q_OBJECT

public:
    using PageToScene = std::function<QPointF(QPointF)>;

    Injector(QWindow* target, PageToScene pageToScene, QObject* parent = nullptr);
    ~Injector() override;

    const QPointingDevice* stylus() const noexcept;
    const QPointingDevice* wheel() const noexcept;

    // Instants prévus : t0Ns + offsetNs. autoTick : minuteur précis ; sinon tick() à la main (tests).
    void start(std::vector<PlannedInput> inputs, std::int64_t t0Ns, bool autoTick = true);
    // Poste tous les événements dont l'instant prévu est passé ; retourne leur nombre.
    std::size_t tick(std::int64_t nowNs);
    bool finished() const noexcept;

    const std::vector<PlannedInput>& inputs() const noexcept;
    const std::vector<std::int64_t>& plannedNs() const noexcept; // absolus
    const std::vector<std::int64_t>& postedNs() const noexcept;  // absolus ; −1 si pas encore posté

signals:
    void planFinished();

private:
    void scheduleNext();
    QEvent* makeEvent(const PlannedInput& input) const;

    QWindow* m_target;
    PageToScene m_pageToScene;
    std::unique_ptr<QPointingDevice> m_stylus;
    std::unique_ptr<QPointingDevice> m_wheel;
    QTimer m_timer;
    std::vector<PlannedInput> m_inputs;
    std::vector<std::int64_t> m_planned;
    std::vector<std::int64_t> m_posted;
    std::size_t m_next = 0;
    bool m_autoTick = true;
    bool m_finishedSignalled = false;
};

} // namespace actcraft::bench
```

Créer `bench/src/injector.cpp` :

```cpp
#include <actcraft/bench/injector.h>
#include <actcraft/core/clock.h>

#include <QCoreApplication>
#include <QTabletEvent>
#include <QWheelEvent>
#include <QWindow>

namespace actcraft::bench {

Injector::Injector(QWindow* target, PageToScene pageToScene, QObject* parent)
    : QObject(parent), m_target(target), m_pageToScene(std::move(pageToScene))
{
    m_stylus = std::make_unique<QPointingDevice>(
        QStringLiteral("actcraft-bench-stylus"), 0x41430001, QInputDevice::DeviceType::Stylus,
        QPointingDevice::PointerType::Pen, QInputDevice::Capability::Position | QInputDevice::Capability::Pressure,
        1, 3);
    m_wheel = std::make_unique<QPointingDevice>(
        QStringLiteral("actcraft-bench-wheel"), 0x41430002, QInputDevice::DeviceType::Mouse,
        QPointingDevice::PointerType::Generic, QInputDevice::Capability::Position | QInputDevice::Capability::Scroll,
        1, 3);
    m_timer.setSingleShot(true);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, [this] { tick(core::monotonicNowNs()); });
}

Injector::~Injector()
{
    m_timer.stop();
}

const QPointingDevice* Injector::stylus() const noexcept
{
    return m_stylus.get();
}

const QPointingDevice* Injector::wheel() const noexcept
{
    return m_wheel.get();
}

void Injector::start(std::vector<PlannedInput> inputs, std::int64_t t0Ns, bool autoTick)
{
    m_timer.stop();
    m_inputs = std::move(inputs);
    m_planned.resize(m_inputs.size());
    for (std::size_t i = 0; i < m_inputs.size(); ++i) {
        m_planned[i] = t0Ns + m_inputs[i].offsetNs;
    }
    m_posted.assign(m_inputs.size(), -1);
    m_next = 0;
    m_autoTick = autoTick;
    m_finishedSignalled = false;
    if (m_autoTick) {
        tick(core::monotonicNowNs());
    }
}

std::size_t Injector::tick(std::int64_t nowNs)
{
    std::size_t count = 0;
    while (m_next < m_inputs.size() && m_planned[m_next] <= nowNs) {
        QCoreApplication::postEvent(m_target, makeEvent(m_inputs[m_next]));
        m_posted[m_next] = core::monotonicNowNs();
        ++m_next;
        ++count;
    }
    if (finished()) {
        m_timer.stop();
        if (!m_finishedSignalled) {
            m_finishedSignalled = true;
            emit planFinished();
        }
    } else if (m_autoTick) {
        scheduleNext();
    }
    return count;
}

bool Injector::finished() const noexcept
{
    return m_next >= m_inputs.size();
}

const std::vector<PlannedInput>& Injector::inputs() const noexcept
{
    return m_inputs;
}

const std::vector<std::int64_t>& Injector::plannedNs() const noexcept
{
    return m_planned;
}

const std::vector<std::int64_t>& Injector::postedNs() const noexcept
{
    return m_posted;
}

void Injector::scheduleNext()
{
    const std::int64_t waitNs = m_planned[m_next] - core::monotonicNowNs();
    // Arrondi au-dessus : un réveil un peu tardif est mesuré (retard d'injection) ; une attente active ne l'est pas.
    const int waitMs = waitNs <= 0 ? 0 : static_cast<int>((waitNs + 999'999) / 1'000'000);
    m_timer.start(waitMs);
}

QEvent* Injector::makeEvent(const PlannedInput& input) const
{
    if (input.kind == InputKind::Wheel) {
        const QPointF center(m_target->width() / 2.0, m_target->height() / 2.0);
        return new QWheelEvent(center, m_target->mapToGlobal(center), QPoint(), input.angleDelta, Qt::NoButton,
                               input.modifiers, Qt::NoScrollPhase, false, Qt::MouseEventNotSynthesized,
                               m_wheel.get());
    }
    const QPointF scene = m_pageToScene(input.pagePos);
    QEvent::Type type = QEvent::TabletMove;
    Qt::MouseButton button = Qt::NoButton;
    Qt::MouseButtons buttons = Qt::LeftButton;
    if (input.kind == InputKind::TabletPress) {
        type = QEvent::TabletPress;
        button = Qt::LeftButton;
    } else if (input.kind == InputKind::TabletRelease) {
        type = QEvent::TabletRelease;
        button = Qt::LeftButton;
        buttons = Qt::NoButton;
    }
    return new QTabletEvent(type, m_stylus.get(), scene, m_target->mapToGlobal(scene), input.pressure, 0, 0, 0, 0, 0,
                            input.modifiers, button, buttons);
}

} // namespace actcraft::bench
```

Dans `bench/CMakeLists.txt`, ajouter aux sources de `actcraft_add_module(bench …)` :

```cmake
        include/actcraft/bench/injector.h
        src/scenarios.cpp
        src/injector.cpp
```

- [ ] **Étape 5 : vert, format, commit.** Commande bornée : `tst_bench_scenarios ... Passed`,
`tst_bench_injector ... Passed`, `TESTS OK`.

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add bench/CMakeLists.txt bench/include/actcraft/bench/scenarios.h bench/include/actcraft/bench/injector.h bench/src/scenarios.cpp bench/src/injector.cpp tests/bench.cmake tests/tst_bench_scenarios.cpp tests/tst_bench_injector.cpp
git -C <worktree> commit -m "bench : scénarios tracé, zoom, défilement ; injecteur à rattrapage sans pilotage de la souris" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

### Tâche T16 — `bench` : options, collecteur, déroulé, verdict, exécutable `actcraft-bench`

**Objectif.** L'exécutable console `actcraft-bench` (DA1, décisions 20, 21, 29, 30) : il charge la vraie
interface par `app/startup.*` et `app/renderbackend.cpp`, place la fenêtre (plein écran sur l'écran principal par
défaut, ou `--taille LxH`), attend « page prête » (images chargées, premier cache dessiné), chauffe 2 s, puis pour
chaque scénario impose le niveau de zoom, attend `scheduledUpdateCompleted` (via `PageView::cacheReady`), rejoue
les entrées et collecte : horodatage de chaque `frameSwapped` (fil de rendu), synchronisations de l'encre (sonde),
réceptions, figements, mémoire à chaque image, exposition. Il calcule statistiques et verdict, écrit le JSON, et
sa dernière ligne et son code de sortie sont `BUDGET TENU` (0), `BUDGET DEPASSE` (1) ou `MESURE INVALIDE` (2).

**Le piège.** Relier un échantillon à « son » image : le numéro de séquence est l'indice de l'événement dans le
plan **parce que** la vue numérote à partir de 0 (`resetInputSequence(0)`) et que chaque événement de tablette
donne exactement un échantillon ; les synchronisations arrivent sur le fil de rendu et ne sont rattachées qu'au
`frameSwapped` **suivant** ; la mémoire est lue à chaque image sur le fil de rendu. Un `waitFor` qui tourne à
vide (`processEvents` sans attente) fausserait la mesure : il attend des événements, réveillé toutes les 10 ms.

**Seuils (N1-1, N1-7), figés ici.** Par scénario, sur les intervalles actifs hors figements : images/s ≥ 58 ;
p95 ≤ 18,5 ms ; part > 25 ms ≤ 1 % ; aucun figement ≥ 2,5 périodes d'écran. Tracé : p50 de la latence interne
< 20 ms. Mémoire : max sur les scénarios de (pic privé + pic graphique) ≤ 1 Gio (2^30 octets). Invalide si la
construction n'est ni `RelWithDebInfo` ni `Release`, si l'exposition est perdue, si le contrôle de contenu échoue
(T17 l'ajoute : sans lui, tout passage est invalide), ou sur erreur. Rendu logiciel : verdict calculé, marqué
`informative` (N1-3).

**Fichiers.**
- Créer : `bench/include/actcraft/bench/options.h`, `bench/include/actcraft/bench/verdict.h`,
  `bench/include/actcraft/bench/recorder.h`, `bench/include/actcraft/bench/runner.h`, `bench/src/options.cpp`,
  `bench/src/verdict.cpp`, `bench/src/recorder.cpp`, `bench/src/runner.cpp`, `bench/app/main.cpp`,
  `tests/tst_bench_options.cpp`, `tests/tst_bench_verdict.cpp`
- Modifier : `bench/CMakeLists.txt`, `tests/bench.cmake`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_bench_(options|verdict) .*(Passed|Failed)' | Select-Object -Last 4
```
Attendu : `out of N` avec N ≥ 23, `tst_bench_options ... Passed`, `tst_bench_verdict ... Passed`, `TESTS OK`.

Passage hors écran (aucune fenêtre visible, aucune souris pilotée) :
```powershell
Set-Location <worktree>; pwsh -NoProfile -Command ". ./scripts/dev-env.ps1; `$env:QT_QPA_PLATFORM='offscreen'; ./build/debug/actcraft-bench.exe --scenario court --rendu logiciel --taille 1280x800 --sortie .tmp/t16.json *> .tmp/t16.txt; exit `$LASTEXITCODE"; "code=$LASTEXITCODE"; Get-Content .tmp/t16.txt -Tail 1; (Get-Content .tmp/t16.json -Raw | ConvertFrom-Json).scenarios | ForEach-Object { "$($_.name) images=$($_.frames.count) actifs=$($_.frames.active) latence=$($_.latency.samples)" }
```
Attendu : `code=2`, dernière ligne `MESURE INVALIDE` (debug, et pas encore de contrôle de contenu), trois lignes
`stroke`, `zoom`, `scroll` avec `images` > 0 ; `stroke` a `latence` > 0.

**Interdits.** Aucun passage en plein écran sur le poste (réservé à la clôture, jeton `ecran`). Pas de
`SendInput`. Ne jamais tuer un `actcraft.exe` ou `actcraft-bench.exe` sans son PID. L'application `actcraft` ne
dépend pas de `bench/`.

**Terminé quand.** Les deux preuves sont conformes ; commit fait.

**Étapes.**

- [ ] **Étape 1 : tests (rouges).** Créer `tests/tst_bench_options.cpp` :

```cpp
#include <actcraft/bench/options.h>

#include <QDir>
#include <QTest>

using namespace actcraft::bench;

class TstBenchOptions : public QObject
{
    Q_OBJECT

private slots:
    void defaults()
    {
        BenchOptions o;
        QString error;
        QVERIFY(parseBenchOptions({QStringLiteral("actcraft-bench")}, o, error));
        QCOMPARE(o.scenarioSet, ScenarioSet::Complet);
        QVERIFY(o.cacheEnabled);
        QVERIFY(o.fullScreen);
        QVERIFY(o.output.isEmpty());
        QVERIFY(o.capturesDir.isEmpty());
#ifdef Q_OS_WIN
        QCOMPARE(o.requestedBackend, QStringLiteral("d3d11"));
#else
        QCOMPARE(o.requestedBackend, QStringLiteral("auto"));
#endif
    }

    void everyOption()
    {
        BenchOptions o;
        QString error;
        QVERIFY(parseBenchOptions({"actcraft-bench", "--rendu", "logiciel", "--scenario", "court", "--sortie",
                                   "x.json", "--sans-cache", "--taille", "1280x800", "--captures", "cap"},
                                  o, error));
        QCOMPARE(o.requestedBackend, QStringLiteral("logiciel"));
        QCOMPARE(o.scenarioSet, ScenarioSet::Court);
        QCOMPARE(o.output, QStringLiteral("x.json"));
        QVERIFY(!o.cacheEnabled);
        QVERIFY(!o.fullScreen);
        QCOMPARE(o.windowSize, QSize(1280, 800));
        QCOMPARE(o.capturesDir, QStringLiteral("cap"));
    }

    void badValuesAreRejected()
    {
        const QList<QStringList> cases{
            {"b", "--taille", "100x100"}, {"b", "--taille", "grand"}, {"b", "--scenario", "long"},
            {"b", "--sortie"},            {"b", "--inconnue"},
        };
        for (const QStringList& args : cases) {
            BenchOptions o;
            QString error;
            QVERIFY2(!parseBenchOptions(args, o, error), qPrintable(args.join(' ')));
            QVERIFY(!error.isEmpty());
        }
    }

    void outputPath()
    {
        BenchOptions o;
        QCOMPARE(resolveOutputPath(o, QStringLiteral("D:/r"), QStringLiteral("f.json")),
                 QDir(QStringLiteral("D:/r")).filePath(QStringLiteral("f.json")));
        o.output = QStringLiteral("bench/results/");
        QCOMPARE(resolveOutputPath(o, QStringLiteral("D:/r"), QStringLiteral("f.json")),
                 QStringLiteral("bench/results/f.json"));
        o.output = QStringLiteral("sortie.json");
        QCOMPARE(resolveOutputPath(o, QStringLiteral("D:/r"), QStringLiteral("f.json")), QStringLiteral("sortie.json"));
    }
};

QTEST_GUILESS_MAIN(TstBenchOptions)
#include "tst_bench_options.moc"
```

Créer `tests/tst_bench_verdict.cpp` :

```cpp
#include <actcraft/bench/verdict.h>

#include <QTest>

using namespace actcraft::bench;

namespace {

constexpr std::uint64_t kMo = 1024ull * 1024ull;

BenchResult good()
{
    BenchResult r;
    r.machine.buildType = QStringLiteral("RelWithDebInfo");
    r.graphicsApi = QStringLiteral("direct3d11");
    r.contentChecks = {{QStringLiteral("witness-shape"), true, QStringLiteral("#ff1e88e5"), QStringLiteral("#ff1e88e5")}};
    ScenarioResult s;
    s.name = QStringLiteral("stroke");
    s.intervals.fps = 60;
    s.intervals.active.p95 = 17;
    s.intervals.active.count = 100;
    s.intervals.over25Ratio = 0;
    s.intervals.freezeOverLimit = 0;
    LatencyStats l;
    l.total.p50 = 15;
    l.total.count = 100;
    s.latency = l;
    s.memory.privatePeak = 300 * kMo;
    s.memory.gpuPeak = 200 * kMo;
    r.scenarios.push_back(s);
    return r;
}

ExitCode after(BenchResult r)
{
    applyVerdict(r);
    return exitCodeFor(r);
}

} // namespace

class TstBenchVerdict : public QObject
{
    Q_OBJECT

private slots:
    void goodRunIsWithinBudget()
    {
        BenchResult r = good();
        applyVerdict(r);
        QVERIFY(r.valid);
        QVERIFY(r.withinBudget);
        QVERIFY(!r.informative);
        QCOMPARE(exitCodeFor(r), ExitCode::WithinBudget);
        QCOMPARE(verdictLine(r), QStringLiteral("BUDGET TENU"));
        QVERIFY(!r.criteria.empty());
    }

    void thresholdsAreInclusiveOrStrictAsSpecified()
    {
        BenchResult r = good();
        r.scenarios[0].intervals.fps = 58.0;
        r.scenarios[0].intervals.active.p95 = 18.5;
        r.scenarios[0].intervals.over25Ratio = 0.01;
        r.scenarios[0].latency->total.p50 = 19.99;
        QCOMPARE(after(r), ExitCode::WithinBudget);

        BenchResult fps = good();
        fps.scenarios[0].intervals.fps = 57.9;
        QCOMPARE(after(fps), ExitCode::OverBudget);
        BenchResult p95 = good();
        p95.scenarios[0].intervals.active.p95 = 18.6;
        QCOMPARE(after(p95), ExitCode::OverBudget);
        BenchResult over = good();
        over.scenarios[0].intervals.over25Ratio = 0.011;
        QCOMPARE(after(over), ExitCode::OverBudget);
        BenchResult latency = good();
        latency.scenarios[0].latency->total.p50 = 20.0; // strict : < 20 ms
        QCOMPARE(after(latency), ExitCode::OverBudget);
        BenchResult freeze = good();
        freeze.scenarios[0].intervals.freezeOverLimit = 1;
        QCOMPARE(after(freeze), ExitCode::OverBudget);
    }

    void memoryAddsProcessAndGpu()
    {
        BenchResult ok = good();
        ok.scenarios[0].memory.privatePeak = 600 * kMo;
        ok.scenarios[0].memory.gpuPeak = 400 * kMo;
        QCOMPARE(after(ok), ExitCode::WithinBudget);
        BenchResult over = good();
        over.scenarios[0].memory.privatePeak = 700 * kMo;
        over.scenarios[0].memory.gpuPeak = 400 * kMo;
        QCOMPARE(after(over), ExitCode::OverBudget);
    }

    void invalidReasons()
    {
        BenchResult debug = good();
        debug.machine.buildType = QStringLiteral("Debug");
        applyVerdict(debug);
        QVERIFY(debug.invalidReasons.contains(QStringLiteral("build-not-release")));
        QCOMPARE(exitCodeFor(debug), ExitCode::Invalid);
        QCOMPARE(verdictLine(debug), QStringLiteral("MESURE INVALIDE"));

        BenchResult content = good();
        content.contentChecks[0].ok = false;
        applyVerdict(content);
        QVERIFY(content.invalidReasons.contains(QStringLiteral("content-check-failed")));

        BenchResult noCheck = good();
        noCheck.contentChecks.clear();
        QCOMPARE(after(noCheck), ExitCode::Invalid);

        BenchResult exposure = good();
        exposure.scenarios[0].exposureLost = true;
        applyVerdict(exposure);
        QVERIFY(exposure.invalidReasons.contains(QStringLiteral("exposure-lost")));
    }

    void reasonsAreNotDuplicated()
    {
        BenchResult r = good();
        r.machine.buildType = QStringLiteral("Debug");
        r.invalidReasons << QStringLiteral("error: essai");
        applyVerdict(r);
        applyVerdict(r);
        QCOMPARE(r.invalidReasons.count(QStringLiteral("build-not-release")), 1);
        QVERIFY(r.invalidReasons.contains(QStringLiteral("error: essai")));
    }

    void softwareIsInformative()
    {
        BenchResult r = good();
        r.graphicsApi = QStringLiteral("software");
        applyVerdict(r);
        QVERIFY(r.informative);
    }

    void summaryEndsWithTheVerdict()
    {
        BenchResult r = good();
        r.scenarios[0].intervals.fps = 40;
        applyVerdict(r);
        const QStringList lines = summaryLines(r, QStringLiteral("x.json"));
        QCOMPARE(lines.last(), QStringLiteral("BUDGET DEPASSE"));
        QVERIFY(lines.join('\n').contains(QStringLiteral("fps")));
    }
};

QTEST_GUILESS_MAIN(TstBenchVerdict)
#include "tst_bench_verdict.moc"
```

Dans `tests/bench.cmake`, ajouter :

```cmake
actcraft_add_test(tst_bench_options
    SOURCES tst_bench_options.cpp
    LIBS ActCraft::bench)

actcraft_add_test(tst_bench_verdict
    SOURCES tst_bench_verdict.cpp
    LIBS ActCraft::bench)
```
Commande bornée : `TESTS EN ECHEC (compilation)`.

- [ ] **Étape 2 : options.** Créer `bench/include/actcraft/bench/options.h` :

```cpp
#pragma once

#include <actcraft/bench/scenarios.h>

#include <QSize>
#include <QString>
#include <QStringList>

namespace actcraft::bench {

// Options du banc (décision 21). --rendu est validé par parseRenderBackend (app/renderbackend.cpp).
struct BenchOptions
{
    QString requestedBackend; // valeur de --rendu ; défaut : d3d11 sous Windows, auto ailleurs
    ScenarioSet scenarioSet = ScenarioSet::Complet;
    QString output;           // --sortie : fichier « .json », sinon dossier ; vide = dossier par défaut
    bool cacheEnabled = true; // --sans-cache
    bool fullScreen = true;   // --taille plein-ecran (défaut) | LxH
    QSize windowSize;
    QString capturesDir;      // --captures : PNG écrits seulement si non vide
};

bool parseBenchOptions(const QStringList& args, BenchOptions& out, QString& error);
// `output` se terminant par .json : ce fichier ; sinon `output` (ou `defaultDir`) + fileName.
QString resolveOutputPath(const BenchOptions& options, const QString& defaultDir, const QString& fileName);

} // namespace actcraft::bench
```

Créer `bench/src/options.cpp` :

```cpp
#include <actcraft/bench/options.h>

#include <QDir>
#include <QRegularExpression>

namespace actcraft::bench {

bool parseBenchOptions(const QStringList& args, BenchOptions& out, QString& error)
{
    out = BenchOptions{};
#ifdef Q_OS_WIN
    out.requestedBackend = QStringLiteral("d3d11");
#else
    out.requestedBackend = QStringLiteral("auto");
#endif
    for (qsizetype i = 1; i < args.size(); ++i) {
        const QString option = args.at(i);
        const auto value = [&](QString& target) {
            if (i + 1 >= args.size()) {
                error = QStringLiteral("%1 attend une valeur").arg(option);
                return false;
            }
            target = args.at(++i);
            return true;
        };
        if (option == QLatin1String("--rendu")) {
            if (!value(out.requestedBackend)) {
                return false;
            }
        } else if (option == QLatin1String("--scenario")) {
            QString v;
            if (!value(v)) {
                return false;
            }
            if (v == QLatin1String("complet")) {
                out.scenarioSet = ScenarioSet::Complet;
            } else if (v == QLatin1String("court")) {
                out.scenarioSet = ScenarioSet::Court;
            } else {
                error = QStringLiteral("--scenario : complet ou court");
                return false;
            }
        } else if (option == QLatin1String("--sortie")) {
            if (!value(out.output)) {
                return false;
            }
        } else if (option == QLatin1String("--sans-cache")) {
            out.cacheEnabled = false;
        } else if (option == QLatin1String("--taille")) {
            QString v;
            if (!value(v)) {
                return false;
            }
            if (v == QLatin1String("plein-ecran")) {
                out.fullScreen = true;
                continue;
            }
            static const QRegularExpression size(QStringLiteral("^(\\d+)x(\\d+)$"));
            const auto match = size.match(v);
            const int w = match.hasMatch() ? match.captured(1).toInt() : 0;
            const int h = match.hasMatch() ? match.captured(2).toInt() : 0;
            if (w < 320 || h < 240) {
                error = QStringLiteral("--taille : plein-ecran ou LxH (au moins 320x240)");
                return false;
            }
            out.fullScreen = false;
            out.windowSize = QSize(w, h);
        } else if (option == QLatin1String("--captures")) {
            if (!value(out.capturesDir)) {
                return false;
            }
        } else {
            error = QStringLiteral("option inconnue : %1").arg(option);
            return false;
        }
    }
    return true;
}

QString resolveOutputPath(const BenchOptions& options, const QString& defaultDir, const QString& fileName)
{
    if (options.output.endsWith(QLatin1String(".json"), Qt::CaseInsensitive)) {
        return options.output;
    }
    return QDir(options.output.isEmpty() ? defaultDir : options.output).filePath(fileName);
}

} // namespace actcraft::bench
```

- [ ] **Étape 3 : verdict.** Créer `bench/include/actcraft/bench/verdict.h` :

```cpp
#pragma once

#include <actcraft/bench/result.h>

#include <QStringList>

#include <cstdint>

namespace actcraft::bench {

// Seuils N1-1 et N1-7 (le figement, kFreezeFactor, est appliqué par classifyIntervals).
struct Budget
{
    double minFps = 58.0;
    double maxP95Ms = 18.5;
    double maxOver25Ratio = 0.01;
    double maxLatencyP50Ms = 20.0; // strict
    std::uint64_t maxMemoryBytes = 1ull << 30;
};

// Remplit criteria, withinBudget, informative, valid et invalidReasons (sans doublon ; les raisons déjà
// présentes, comme « error: … », sont gardées).
void applyVerdict(BenchResult& result, const Budget& budget = {});

enum class ExitCode : int
{
    WithinBudget = 0,
    OverBudget = 1,
    Invalid = 2,
};

ExitCode exitCodeFor(const BenchResult& result) noexcept;
// « BUDGET TENU » | « BUDGET DEPASSE » | « MESURE INVALIDE »
QString verdictLine(const BenchResult& result);
// Résumé console ; la dernière ligne est verdictLine().
QStringList summaryLines(const BenchResult& result, const QString& path);

} // namespace actcraft::bench
```

Créer `bench/src/verdict.cpp` :

```cpp
#include <actcraft/bench/verdict.h>

#include <algorithm>

namespace actcraft::bench {

namespace {
void addReason(BenchResult& r, const QString& reason)
{
    if (!r.invalidReasons.contains(reason)) {
        r.invalidReasons << reason;
    }
}
} // namespace

void applyVerdict(BenchResult& r, const Budget& b)
{
    if (r.machine.buildType != QLatin1String("RelWithDebInfo") && r.machine.buildType != QLatin1String("Release")) {
        addReason(r, QStringLiteral("build-not-release"));
    }
    for (const ScenarioResult& s : r.scenarios) {
        if (s.exposureLost) {
            addReason(r, QStringLiteral("exposure-lost"));
        }
    }
    if (!contentCheckOk(r)) {
        addReason(r, QStringLiteral("content-check-failed"));
    }
    r.valid = r.invalidReasons.isEmpty();

    r.criteria.clear();
    const auto criterion = [&r](const QString& scenario, const char* name, double value, double limit, bool ok) {
        r.criteria.push_back({scenario, QString::fromLatin1(name), value, limit, ok});
    };
    std::uint64_t memoryPeak = 0;
    for (const ScenarioResult& s : r.scenarios) {
        const IntervalStats& iv = s.intervals;
        criterion(s.name, "fps", iv.fps, b.minFps, iv.fps >= b.minFps);
        criterion(s.name, "p95_ms", iv.active.p95, b.maxP95Ms, iv.active.count > 0 && iv.active.p95 <= b.maxP95Ms);
        criterion(s.name, "over_25ms_ratio", iv.over25Ratio, b.maxOver25Ratio, iv.over25Ratio <= b.maxOver25Ratio);
        criterion(s.name, "freeze_over_limit", static_cast<double>(iv.freezeOverLimit), 0, iv.freezeOverLimit == 0);
        if (s.latency) {
            criterion(s.name, "latency_p50_ms", s.latency->total.p50, b.maxLatencyP50Ms,
                      s.latency->total.count > 0 && s.latency->total.p50 < b.maxLatencyP50Ms);
        }
        memoryPeak = std::max(memoryPeak, s.memory.privatePeak + s.memory.gpuPeak.value_or(0));
    }
    criterion(QStringLiteral("all"), "memory_peak_bytes", static_cast<double>(memoryPeak),
              static_cast<double>(b.maxMemoryBytes), memoryPeak <= b.maxMemoryBytes);
    r.withinBudget = !r.scenarios.empty() &&
                     std::all_of(r.criteria.begin(), r.criteria.end(), [](const Criterion& c) { return c.ok; });
    r.informative = r.graphicsApi == QLatin1String("software");
}

ExitCode exitCodeFor(const BenchResult& r) noexcept
{
    if (!r.valid) {
        return ExitCode::Invalid;
    }
    return r.withinBudget ? ExitCode::WithinBudget : ExitCode::OverBudget;
}

QString verdictLine(const BenchResult& r)
{
    switch (exitCodeFor(r)) {
    case ExitCode::WithinBudget:
        return QStringLiteral("BUDGET TENU");
    case ExitCode::OverBudget:
        return QStringLiteral("BUDGET DEPASSE");
    case ExitCode::Invalid:
        break;
    }
    return QStringLiteral("MESURE INVALIDE");
}

QStringList summaryLines(const BenchResult& r, const QString& path)
{
    QStringList lines;
    lines << QStringLiteral("Résultat : %1").arg(path);
    lines << QStringLiteral("Rendu : %1 (demandé : %2) · cache : %3 · boucle : %4%5")
                 .arg(r.graphicsApi, r.requestedBackend, r.cache ? QStringLiteral("oui") : QStringLiteral("non"),
                      r.renderLoop, r.informative ? QStringLiteral(" · informatif") : QString());
    for (const ScenarioResult& s : r.scenarios) {
        QString line = QStringLiteral("%1 : %2 fps · p95 %3 ms · > 25 ms %4 %")
                           .arg(s.name)
                           .arg(s.intervals.fps, 0, 'f', 1)
                           .arg(s.intervals.active.p95, 0, 'f', 1)
                           .arg(100.0 * s.intervals.over25Ratio, 0, 'f', 2);
        if (s.latency) {
            line += QStringLiteral(" · latence p50 %1 ms, p95 %2 ms")
                        .arg(s.latency->total.p50, 0, 'f', 1)
                        .arg(s.latency->total.p95, 0, 'f', 1);
        }
        lines << line;
    }
    for (const Criterion& c : r.criteria) {
        if (!c.ok) {
            lines << QStringLiteral("Dépassé : %1 %2 = %3 (seuil %4)").arg(c.scenario, c.name).arg(c.value).arg(c.limit);
        }
    }
    if (!r.invalidReasons.isEmpty()) {
        lines << QStringLiteral("Invalide : %1").arg(r.invalidReasons.join(QStringLiteral(", ")));
    }
    lines << verdictLine(r);
    return lines;
}

} // namespace actcraft::bench
```

- [ ] **Étape 4 : collecteur.** Créer `bench/include/actcraft/bench/recorder.h` :

```cpp
#pragma once

#include <actcraft/bench/memory.h>
#include <actcraft/bench/stats.h>
#include <actcraft/core/inkprobe.h>

#include <QObject>
#include <QString>

#include <memory>
#include <mutex>
#include <utility>
#include <vector>

class QQuickWindow;

namespace actcraft::bench {

// Collecte sur le fil de rendu (frameSwapped, mémoire) et sert de sonde d'encre (DA4).
class FrameRecorder : public QObject, public core::InkProbe
{
    Q_OBJECT

public:
    explicit FrameRecorder(QQuickWindow* window, QObject* parent = nullptr);

    void begin(); // vide les séries et active l'enregistrement
    void end();

    void sampleReceived(std::uint64_t seq, std::int64_t nowNs) override;
    void samplesSynced(std::uint64_t lastSeq, std::int64_t nowNs) override;

    std::vector<std::int64_t> swapNs() const;
    std::vector<SyncRecord> syncRecords() const;
    std::vector<std::pair<std::uint64_t, std::int64_t>> received() const;
    MemorySummary memory() const;
    QString renderLoop() const; // basic | threaded | unknown
    QString gpuName() const;

private:
    void onBeforeRendering(); // fil de rendu
    void onFrameSwapped();    // fil de rendu

    QQuickWindow* m_window;
    mutable std::mutex m_mutex;
    bool m_recording = false;
    std::vector<std::int64_t> m_swaps;
    std::vector<std::pair<std::uint64_t, std::int64_t>> m_pendingSyncs;
    std::vector<SyncRecord> m_syncs;
    std::vector<std::pair<std::uint64_t, std::int64_t>> m_received;
    MemoryTracker m_memory;
    std::unique_ptr<GpuMemoryReader> m_gpu; // créé et lu sur le fil de rendu seulement
    bool m_gpuTried = false;
    QString m_gpuName;
    QString m_renderLoop = QStringLiteral("unknown");
};

} // namespace actcraft::bench
```

Créer `bench/src/recorder.cpp` :

```cpp
#include <actcraft/bench/recorder.h>
#include <actcraft/core/clock.h>

#include <QCoreApplication>
#include <QQuickWindow>
#include <QThread>

namespace actcraft::bench {

FrameRecorder::FrameRecorder(QQuickWindow* window, QObject* parent) : QObject(parent), m_window(window)
{
    connect(window, &QQuickWindow::beforeRendering, this, &FrameRecorder::onBeforeRendering, Qt::DirectConnection);
    connect(window, &QQuickWindow::frameSwapped, this, &FrameRecorder::onFrameSwapped, Qt::DirectConnection);
}

void FrameRecorder::begin()
{
    std::lock_guard lock(m_mutex);
    m_swaps.clear();
    m_pendingSyncs.clear();
    m_syncs.clear();
    m_received.clear();
    m_memory.reset();
    m_recording = true;
}

void FrameRecorder::end()
{
    std::lock_guard lock(m_mutex);
    m_recording = false;
}

void FrameRecorder::sampleReceived(std::uint64_t seq, std::int64_t nowNs)
{
    std::lock_guard lock(m_mutex);
    if (m_recording) {
        m_received.emplace_back(seq, nowNs);
    }
}

void FrameRecorder::samplesSynced(std::uint64_t lastSeq, std::int64_t nowNs)
{
    std::lock_guard lock(m_mutex);
    if (m_recording) {
        m_pendingSyncs.emplace_back(lastSeq, nowNs);
    }
}

void FrameRecorder::onBeforeRendering()
{
    if (m_gpuTried) {
        return;
    }
    m_gpuTried = true;
    auto gpu = GpuMemoryReader::create(m_window);
    std::lock_guard lock(m_mutex);
    if (gpu) {
        m_gpuName = gpu->adapterName();
    }
    m_gpu = std::move(gpu);
}

void FrameRecorder::onFrameSwapped()
{
    const std::int64_t now = core::monotonicNowNs();
    const bool onMainThread = QThread::currentThread() == QCoreApplication::instance()->thread();
    bool recording = false;
    {
        std::lock_guard lock(m_mutex);
        m_renderLoop = onMainThread ? QStringLiteral("basic") : QStringLiteral("threaded");
        recording = m_recording;
        if (recording) {
            m_swaps.push_back(now);
            // Les synchronisations en attente appartiennent à l'image qui vient d'être présentée.
            for (const auto& [lastSeq, syncNs] : m_pendingSyncs) {
                m_syncs.push_back({lastSeq, syncNs, now});
            }
            m_pendingSyncs.clear();
        }
    }
    if (recording) {
        // Valeurs instantanées : relevées à chaque image pour saisir le pic (décision 26).
        m_memory.sample(readProcessMemory(), m_gpu ? m_gpu->currentBytes() : std::nullopt);
    }
}

std::vector<std::int64_t> FrameRecorder::swapNs() const
{
    std::lock_guard lock(m_mutex);
    return m_swaps;
}

std::vector<SyncRecord> FrameRecorder::syncRecords() const
{
    std::lock_guard lock(m_mutex);
    return m_syncs;
}

std::vector<std::pair<std::uint64_t, std::int64_t>> FrameRecorder::received() const
{
    std::lock_guard lock(m_mutex);
    return m_received;
}

MemorySummary FrameRecorder::memory() const
{
    return m_memory.summary();
}

QString FrameRecorder::renderLoop() const
{
    std::lock_guard lock(m_mutex);
    return m_renderLoop;
}

QString FrameRecorder::gpuName() const
{
    std::lock_guard lock(m_mutex);
    return m_gpuName;
}

} // namespace actcraft::bench
```

- [ ] **Étape 5 : déroulé.** Créer `bench/include/actcraft/bench/runner.h` :

```cpp
#pragma once

#include <actcraft/bench/injector.h>
#include <actcraft/bench/options.h>
#include <actcraft/bench/recorder.h>
#include <actcraft/bench/result.h>

#include <functional>
#include <memory>

class QQuickWindow;

namespace actcraft::render {
class FixedPageItem;
}
namespace actcraft::ui {
class PageView;
}

namespace actcraft::bench {

// Joue un passage complet du banc sur la fenêtre déjà chargée ; ne quitte pas l'application.
class Runner
{
public:
    Runner(QQuickWindow* window, BenchOptions options);
    ~Runner();

    BenchResult run();

private:
    bool waitFor(const std::function<bool()>& condition, int timeoutMs);
    void waitMs(int ms);
    bool placeWindow(BenchResult& result);
    bool setLevel(const ScenarioPlan& plan);
    ScenarioResult play(const ScenarioPlan& plan);

    QQuickWindow* m_window;
    BenchOptions m_options;
    ui::PageView* m_view = nullptr;
    render::FixedPageItem* m_fixed = nullptr;
    std::unique_ptr<FrameRecorder> m_recorder;
    std::unique_ptr<Injector> m_injector;
};

} // namespace actcraft::bench
```

Créer `bench/src/runner.cpp` :

```cpp
#include <actcraft/bench/runner.h>
#include <actcraft/bench/verdict.h>
#include <actcraft/core/clock.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/viewtransform.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>

#include <algorithm>

namespace actcraft::bench {

Runner::Runner(QQuickWindow* window, BenchOptions options) : m_window(window), m_options(std::move(options)) {}

Runner::~Runner() = default;

bool Runner::waitFor(const std::function<bool()>& condition, int timeoutMs)
{
    QElapsedTimer clock;
    clock.start();
    QTimer wake; // réveille la boucle sans la faire tourner à vide
    wake.start(10);
    while (!condition()) {
        if (clock.elapsed() > timeoutMs) {
            return false;
        }
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }
    return true;
}

void Runner::waitMs(int ms)
{
    QElapsedTimer clock;
    clock.start();
    waitFor([&clock, ms] { return clock.elapsed() >= ms; }, ms + 1000);
}

bool Runner::placeWindow(BenchResult& result)
{
    QScreen* screen = QGuiApplication::primaryScreen();
    m_window->setScreen(screen);
    if (m_options.fullScreen) {
        m_window->setGeometry(screen->geometry());
        m_window->showFullScreen();
        result.windowMode = QStringLiteral("fullscreen");
    } else {
        m_window->resize(m_options.windowSize);
        const QRect available = screen->availableGeometry();
        m_window->setPosition(available.center() -
                              QPoint(m_options.windowSize.width() / 2, m_options.windowSize.height() / 2));
        m_window->showNormal();
        result.windowMode = QStringLiteral("window");
    }
    m_window->requestActivate();
    if (!waitFor([this] { return m_window->isExposed(); }, 10000)) {
        return false;
    }
    // Relevé, sans invalider : les entrées sont postées à la fenêtre (décision 28).
    result.activated = waitFor([this] { return m_window->isActive(); }, 2000);
    waitMs(200);
    result.windowSize = m_window->size();
    result.screenName = screen->name();
    result.screenSize = screen->size();
    result.refreshHz = screen->refreshRate();
    result.devicePixelRatio = m_window->devicePixelRatio();
    return true;
}

bool Runner::setLevel(const ScenarioPlan& plan)
{
    m_view->setZoom(plan.zoom);
    if (plan.topLeft) {
        m_view->setPan(QPointF(ui::kViewMargin, ui::kViewMargin));
    }
    // Attendre la fin du redessin du cache (scheduledUpdateCompleted) avant la fenêtre de mesure (décision 17).
    const bool ready = waitFor([this] { return m_view->cacheReady(); }, 10000);
    waitMs(100);
    return ready;
}

ScenarioResult Runner::play(const ScenarioPlan& plan)
{
    ScenarioResult s;
    s.name = plan.name;
    // Pendant un scénario, la vue n'écoute que les périphériques synthétiques (décision 23).
    m_view->setExclusiveDevices({m_injector->stylus(), m_injector->wheel()});
    m_view->resetInputSequence(0); // numéro de séquence = indice de l'événement dans le plan
    m_view->setInkProbe(m_recorder.get());

    std::vector<std::int64_t> freezes;
    const auto frozen = QObject::connect(m_view, &ui::PageView::strokeFrozen,
                                         [&freezes] { freezes.push_back(core::monotonicNowNs()); });
    bool exposureLost = false;
    QTimer exposure;
    exposure.setInterval(50);
    QObject::connect(&exposure, &QTimer::timeout, [this, &exposureLost] {
        const auto v = m_window->visibility();
        if (!m_window->isExposed() || v == QWindow::Minimized || v == QWindow::Hidden) {
            exposureLost = true;
        }
    });
    exposure.start();

    m_recorder->begin();
    s.startNs = core::monotonicNowNs() + 20'000'000;
    m_injector->start(plan.inputs, s.startNs);
    waitFor([this] { return m_injector->finished(); }, static_cast<int>(planDurationNs(plan) / 1'000'000) + 10000);
    waitMs(150); // dernières images : figement, redessin du cache
    m_recorder->end();

    exposure.stop();
    QObject::disconnect(frozen);
    m_view->setInkProbe(nullptr);
    m_view->setExclusiveDevices({});

    s.exposureLost = exposureLost;
    const auto& planned = m_injector->plannedNs();
    const auto& posted = m_injector->postedNs();
    const auto& inputs = m_injector->inputs();
    s.frames.swapNs = m_recorder->swapNs();
    for (std::size_t i = 0; i < posted.size(); ++i) {
        if (posted[i] >= 0) {
            // Instants prévus (décision 23) : un fil principal bloqué qui poste en retard ne rend pas l'intervalle
            // « au repos » ; l'instant d'envoi ne sert qu'au retard d'injection.
            s.frames.inputNs.push_back(planned[i]);
            s.injectionDelayMs.push_back(static_cast<double>(posted[i] - planned[i]) / 1e6);
        }
    }
    std::sort(s.frames.inputNs.begin(), s.frames.inputNs.end());
    s.frames.freezeNs = freezes;
    const double hz = m_window->screen() ? m_window->screen()->refreshRate() : 60.0;
    s.frames.refreshPeriodNs = static_cast<std::int64_t>(1e9 / (hz > 1.0 ? hz : 60.0));
    s.intervals = classifyIntervals(s.frames);

    if (plan.name == QLatin1String("stroke")) {
        std::vector<std::int64_t> measured(planned.size(), -1);
        for (std::size_t i = 0; i < inputs.size(); ++i) {
            if (inputs[i].measured) {
                measured[i] = planned[i];
            }
        }
        s.latency = computeLatency(measured, m_recorder->syncRecords());
        for (const auto& [seq, at] : m_recorder->received()) {
            if (seq < planned.size()) {
                s.receptionDelayMs.push_back(static_cast<double>(at - planned[seq]) / 1e6);
            }
        }
    }
    s.memory = m_recorder->memory();
    return s;
}

BenchResult Runner::run()
{
    BenchResult result;
    result.startedAt = QDateTime::currentDateTime();
    result.machine = readMachineInfo();
    result.requestedBackend = m_options.requestedBackend;
    result.scenarioSet =
        m_options.scenarioSet == ScenarioSet::Court ? QStringLiteral("court") : QStringLiteral("complet");

    m_view = m_window->findChild<ui::PageView*>(QStringLiteral("pageView"));
    m_fixed = m_window->findChild<render::FixedPageItem*>(QStringLiteral("fixedPage"));
    if (!m_view || !m_fixed) {
        result.invalidReasons << QStringLiteral("error: interface incomplete");
        applyVerdict(result);
        return result;
    }
    m_recorder = std::make_unique<FrameRecorder>(m_window);
    m_injector = std::make_unique<Injector>(m_window, [this](QPointF p) { return m_view->pageToScene(p); });

    if (!placeWindow(result)) {
        result.invalidReasons << QStringLiteral("error: window not exposed");
        applyVerdict(result);
        return result;
    }
    // Page prête : images chargées et premier cache dessiné.
    if (!waitFor([this] { return m_fixed->isReady() && m_view->cacheReady(); }, 30000)) {
        result.invalidReasons << QStringLiteral("error: page not ready");
        applyVerdict(result);
        return result;
    }
    result.graphicsApi = apiName(m_window->rendererInterface()->graphicsApi());
    result.cache = m_view->cacheEnabled();
    waitMs(2000 / durationDivisor(m_options.scenarioSet)); // chauffe

    for (const ScenarioPlan& plan : allScenarios(m_options.scenarioSet)) {
        if (!setLevel(plan)) {
            result.invalidReasons << QStringLiteral("error: cache not ready before %1").arg(plan.name);
            break;
        }
        result.scenarios.push_back(play(plan));
        if (!m_injector->finished()) {
            result.invalidReasons << QStringLiteral("error: injection incomplete in %1").arg(plan.name);
            break;
        }
    }
    result.renderLoop = m_recorder->renderLoop();
    result.gpuName = m_recorder->gpuName().isEmpty() ? result.graphicsApi : m_recorder->gpuName();
    applyVerdict(result);
    return result;
}

} // namespace actcraft::bench
```

- [ ] **Étape 6 : exécutable.** Créer `bench/app/main.cpp` :

```cpp
#include "renderbackend.h"
#include "startup.h"

#include <actcraft/bench/options.h>
#include <actcraft/bench/result.h>
#include <actcraft/bench/runner.h>
#include <actcraft/bench/verdict.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>

#include <cstdio>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

namespace {
void printLine(const QString& line)
{
    std::printf("%s\n", line.toUtf8().constData());
    std::fflush(stdout);
}
} // namespace

// Banc de fluidité (DA1) : charge la vraie interface, rejoue tracé, zoom et défilement, écrit le JSON.
// Dernière ligne et code de sortie : BUDGET TENU (0), BUDGET DEPASSE (1), MESURE INVALIDE (2).
int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("ActCraft-Banc"));
    const QStringList args = QGuiApplication::arguments();

    RenderBackend backend = RenderBackend::Auto;
    QString error;
    actcraft::bench::BenchOptions options;
    if (!parseRenderBackend(args, backend, error) || !actcraft::bench::parseBenchOptions(args, options, error)) {
        std::fprintf(stderr, "%s\n", qPrintable(error));
        printLine(QStringLiteral("MESURE INVALIDE"));
        return 2;
    }
    applyRenderBackend(backend);

    QQmlApplicationEngine engine;
    StartupOptions startup;
    startup.cacheEnabled = options.cacheEnabled;
    QQuickWindow* window = loadMainWindow(engine, startup);
    if (!window) {
        printLine(QStringLiteral("MESURE INVALIDE"));
        return 2;
    }

    int code = 2;
    actcraft::bench::Runner runner(window, options);
    QTimer::singleShot(0, &app, [&] {
        actcraft::bench::BenchResult result = runner.run();
        const QString defaultDir = QCoreApplication::applicationDirPath() + QStringLiteral("/resultats-banc");
        const QString path =
            actcraft::bench::resolveOutputPath(options, defaultDir, actcraft::bench::resultFileName(result));
        QString writeError;
        if (!actcraft::bench::writeResult(result, path, writeError)) {
            result.invalidReasons << QStringLiteral("error: %1").arg(writeError);
            actcraft::bench::applyVerdict(result);
        }
        for (const QString& line : actcraft::bench::summaryLines(result, path)) {
            printLine(line);
        }
        code = static_cast<int>(actcraft::bench::exitCodeFor(result));
        QCoreApplication::exit(code);
    });
    QGuiApplication::exec();
    return code;
}
```

Dans `bench/CMakeLists.txt` : ajouter aux sources de `actcraft_add_module(bench …)`
`include/actcraft/bench/recorder.h`, `src/options.cpp`, `src/verdict.cpp`, `src/recorder.cpp`,
`src/runner.cpp` ; ajouter `ActCraft::ui` aux `DEPENDS` (avant `Qt6::Quick`) ; puis à la fin du fichier :

```cmake
# Exécutable du banc (DA1) : charge la vraie interface ; console, sans paquet macOS.
qt_add_executable(actcraft-bench
    app/main.cpp
    ${PROJECT_SOURCE_DIR}/app/renderbackend.cpp
    ${PROJECT_SOURCE_DIR}/app/startup.cpp)
target_include_directories(actcraft-bench PRIVATE ${PROJECT_SOURCE_DIR}/app)
target_link_libraries(actcraft-bench PRIVATE ActCraft::bench ActCraft::ui actcraft_uiplugin Qt6::Quick)
actcraft_set_warnings(actcraft-bench)
# qt_standard_project_setup ne regroupe les exécutables à la racine du dossier de construction que sous Windows
# (Qt6CoreMacros.cmake) : on l'impose partout, pour que scripts/bench.sh et la CI macOS trouvent
# build/<preset>/actcraft-bench.
set_target_properties(actcraft-bench PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
```

- [ ] **Étape 7 : vert et passage hors écran.** Commande bornée : `tst_bench_options ... Passed`,
`tst_bench_verdict ... Passed`, `TESTS OK`. Puis le passage hors écran : `code=2`, `MESURE INVALIDE`, trois
scénarios avec des images, latence du tracé non vide. Si `stroke` a `latence=0` : vérifier que la vue reçoit le
stylet synthétique (variante de T12) et que `InkItem` signale bien `samplesSynced`.

- [ ] **Étape 8 : format, commit.**

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add bench/CMakeLists.txt bench/include/actcraft/bench/options.h bench/include/actcraft/bench/verdict.h bench/include/actcraft/bench/recorder.h bench/include/actcraft/bench/runner.h bench/src/options.cpp bench/src/verdict.cpp bench/src/recorder.cpp bench/src/runner.cpp bench/app/main.cpp tests/bench.cmake tests/tst_bench_options.cpp tests/tst_bench_verdict.cpp
git -C <worktree> commit -m "bench : exécutable actcraft-bench, collecte par image, verdict et code de sortie" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```
Le rapport colle la dernière ligne et les trois lignes de scénarios du passage hors écran.

---

### Tâche T17 — `bench` : démonstration, captures, contrôle de contenu

**Objectif.** À **chaque** passage, après les scénarios, une séquence de démonstration **non mesurée**
(décision 31) : trait de démonstration (bande y = 670) avec capture en mémoire au milieu du trait, à la fin du
trait, à 800 %, puis de retour à 100 %. Quatre contrôles écrits dans `content_check` : couleur des trois objets
témoins sur la capture « retour à 100 % », couleur de l'encre (noir) au point de contrôle sur la capture
« mi-trait ». Un échec rend le passage invalide (déjà appliqué par `applyVerdict`). Les PNG ne sont écrits
qu'avec `--captures`, sous les noms `<rendu>-<étape>.png` (`software-mi-trait.png`,
`direct3d11-zoom-800.png`…). C'est ce qui prouve qu'un chemin D3D11 dessine réellement, encre comprise.

**Fichiers.**
- Créer : `bench/include/actcraft/bench/contentcheck.h`, `bench/src/contentcheck.cpp`,
  `tests/tst_bench_content.cpp`
- Modifier : `bench/include/actcraft/bench/runner.h`, `bench/src/runner.cpp`, `bench/CMakeLists.txt`,
  `tests/bench.cmake`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_bench_content .*(Passed|Failed)' | Select-Object -Last 3
```
Attendu : `out of N` avec N ≥ 24, `tst_bench_content ... Passed`, `TESTS OK`.

Passage hors écran avec captures :
```powershell
Set-Location <worktree>; pwsh -NoProfile -Command ". ./scripts/dev-env.ps1; `$env:QT_QPA_PLATFORM='offscreen'; ./build/debug/actcraft-bench.exe --scenario court --rendu logiciel --taille 1280x800 --sortie .tmp/t17.json --captures .tmp/t17 *> .tmp/t17.txt; exit `$LASTEXITCODE"; "code=$LASTEXITCODE"; $j = Get-Content .tmp/t17.json -Raw | ConvertFrom-Json; "content_check=$($j.content_check.ok) raisons=$($j.verdict.invalid_reasons -join ',')"; $j.content_check.checks | ForEach-Object { "$($_.name) $($_.ok) $($_.expected) $($_.actual)" }; Get-ChildItem .tmp/t17 -Filter *.png | Select-Object -ExpandProperty Name
```
Attendu : `code=2`, `content_check=True raisons=build-not-release` (debug seulement), quatre contrôles `True`,
quatre fichiers `software-mi-trait.png`, `software-fin-trait.png`, `software-zoom-800.png`,
`software-retour-100.png`.

**Preuve visuelle.** Ouvrir les quatre PNG (outil `Read` sur l'image) et les décrire en 3 lignes dans le
rapport : encre noire en cours dans la bande basse (mi-trait) ; trait noir complet et 300 objets (fin-trait) ;
page agrandie au centre (zoom-800) ; page entière nette avec le trait toujours présent (retour-100).

**Interdits.** Aucun contrôle ne s'appuie sur une capture écrite sur disque (tout est en mémoire). Pas de
tolérance de couleur au-delà de 24 (témoins) et 48 (encre).

**Terminé quand.** Les deux preuves et la preuve visuelle sont conformes ; commit fait.

**Étapes.**

- [ ] **Étape 1 : tests (rouges).** Créer `tests/tst_bench_content.cpp` :

```cpp
#include <actcraft/bench/contentcheck.h>

#include <QImage>
#include <QTest>

using namespace actcraft::bench;

class TstBenchContent : public QObject
{
    Q_OBJECT

private slots:
    void colorNearUsesEveryChannel()
    {
        QVERIFY(colorNear(QColor(100, 100, 100), QColor(110, 90, 100), 10));
        QVERIFY(!colorNear(QColor(100, 100, 100), QColor(100, 100, 111), 10));
    }

    void sampleColorAveragesNine()
    {
        QImage image(10, 10, QImage::Format_ARGB32);
        image.fill(QColor(0, 0, 0));
        image.setPixelColor(5, 5, QColor(90, 90, 90));
        const QColor c = sampleColor(image, QPoint(5, 5));
        QCOMPARE(c.red(), 10);
        QCOMPARE(sampleColor(image, QPoint(0, 0)).red(), 0); // bord : voisins hors image ignorés
    }

    void sceneToImageScalesByRatio()
    {
        QCOMPARE(sceneToImage(QPointF(10.4, 20.6), 1.0), QPoint(10, 21));
        QCOMPARE(sceneToImage(QPointF(10, 20), 1.5), QPoint(15, 30));
    }

    void checkColorReportsExpectedAndActual()
    {
        QImage image(4, 4, QImage::Format_ARGB32);
        image.fill(QColor(0x1E, 0x88, 0xE5));
        const ContentCheck ok = checkColor(QStringLiteral("witness-shape"), image, QPoint(2, 2),
                                           QColor(0x1E, 0x88, 0xE5), kWitnessTolerance);
        QVERIFY(ok.ok);
        QCOMPARE(ok.expected, QStringLiteral("#ff1e88e5"));
        QCOMPARE(ok.actual, QStringLiteral("#ff1e88e5"));
        const ContentCheck ko =
            checkColor(QStringLiteral("ink"), image, QPoint(2, 2), QColor(Qt::black), kInkTolerance);
        QVERIFY(!ko.ok);
        const ContentCheck outside =
            checkColor(QStringLiteral("x"), image, QPoint(50, 50), QColor(0x1E, 0x88, 0xE5), kWitnessTolerance);
        QVERIFY(!outside.ok);
    }

    void captureNames()
    {
        QCOMPARE(captureSteps(), QStringList({QStringLiteral("mi-trait"), QStringLiteral("fin-trait"),
                                              QStringLiteral("zoom-800"), QStringLiteral("retour-100")}));
        QCOMPARE(captureFileName(QStringLiteral("direct3d11"), QStringLiteral("mi-trait")),
                 QStringLiteral("direct3d11-mi-trait.png"));
    }
};

QTEST_GUILESS_MAIN(TstBenchContent)
#include "tst_bench_content.moc"
```

Dans `tests/bench.cmake`, ajouter :

```cmake
actcraft_add_test(tst_bench_content
    SOURCES tst_bench_content.cpp
    LIBS ActCraft::bench)
```
Commande bornée : `TESTS EN ECHEC (compilation)`.

- [ ] **Étape 2 : contrôles.** Créer `bench/include/actcraft/bench/contentcheck.h` :

```cpp
#pragma once

#include <actcraft/bench/result.h>

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QPointF>
#include <QStringList>

namespace actcraft::bench {

inline constexpr int kWitnessTolerance = 24; // par canal
inline constexpr int kInkTolerance = 48;

bool colorNear(QColor actual, QColor expected, int tolerance) noexcept;
// Moyenne 3 × 3 autour du pixel ; voisins hors de l'image ignorés ; invalide si le pixel est hors de l'image.
QColor sampleColor(const QImage& image, QPoint pixel);
// Point de scène (pixels logiques) → pixel de capture (× rapport de pixels, arrondi).
QPoint sceneToImage(QPointF scenePoint, qreal devicePixelRatio);
ContentCheck checkColor(const QString& name, const QImage& image, QPoint pixel, QColor expected, int tolerance);
// mi-trait, fin-trait, zoom-800, retour-100
QStringList captureSteps();
QString captureFileName(const QString& graphicsApi, const QString& step);

} // namespace actcraft::bench
```

Créer `bench/src/contentcheck.cpp` :

```cpp
#include <actcraft/bench/contentcheck.h>

#include <cmath>

namespace actcraft::bench {

bool colorNear(QColor actual, QColor expected, int tolerance) noexcept
{
    return std::abs(actual.red() - expected.red()) <= tolerance &&
           std::abs(actual.green() - expected.green()) <= tolerance &&
           std::abs(actual.blue() - expected.blue()) <= tolerance;
}

QColor sampleColor(const QImage& image, QPoint pixel)
{
    if (!image.rect().contains(pixel)) {
        return {};
    }
    int r = 0;
    int g = 0;
    int b = 0;
    int n = 0;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const QPoint p(pixel.x() + dx, pixel.y() + dy);
            if (image.rect().contains(p)) {
                const QColor c = image.pixelColor(p);
                r += c.red();
                g += c.green();
                b += c.blue();
                ++n;
            }
        }
    }
    return QColor(r / n, g / n, b / n);
}

QPoint sceneToImage(QPointF scenePoint, qreal devicePixelRatio)
{
    return QPoint(static_cast<int>(std::lround(scenePoint.x() * devicePixelRatio)),
                  static_cast<int>(std::lround(scenePoint.y() * devicePixelRatio)));
}

ContentCheck checkColor(const QString& name, const QImage& image, QPoint pixel, QColor expected, int tolerance)
{
    const QColor actual = sampleColor(image, pixel);
    ContentCheck check;
    check.name = name;
    check.expected = expected.name(QColor::HexArgb);
    check.actual = actual.isValid() ? actual.name(QColor::HexArgb) : QStringLiteral("hors-image");
    check.ok = actual.isValid() && colorNear(actual, expected, tolerance);
    return check;
}

QStringList captureSteps()
{
    return {QStringLiteral("mi-trait"), QStringLiteral("fin-trait"), QStringLiteral("zoom-800"),
            QStringLiteral("retour-100")};
}

QString captureFileName(const QString& graphicsApi, const QString& step)
{
    return graphicsApi + QLatin1Char('-') + step + QStringLiteral(".png");
}

} // namespace actcraft::bench
```

Dans `bench/CMakeLists.txt`, ajouter `src/contentcheck.cpp` aux sources.

- [ ] **Étape 3 : séquence de démonstration.** Dans `bench/include/actcraft/bench/runner.h`, ajouter en
`private:` la déclaration `void runDemo(BenchResult& result);`. Dans `bench/src/runner.cpp`, ajouter les
inclusions `#include <actcraft/bench/contentcheck.h>`, `#include <actcraft/core/generator.h>` et
`#include <QDir>`, puis la définition :

```cpp
void Runner::runDemo(BenchResult& result)
{
    // Séquence non mesurée (décision 31) : captures en mémoire, 4 contrôles, PNG seulement avec --captures.
    m_view->setZoom(1.0);
    waitFor([this] { return m_view->cacheReady(); }, 10000);
    m_view->setExclusiveDevices({m_injector->stylus(), m_injector->wheel()});

    const std::vector<PlannedInput> demo = demoStroke();
    const std::vector<PlannedInput> firstHalf(demo.begin(), demo.begin() + static_cast<std::ptrdiff_t>(kDemoMidIndex));
    std::vector<PlannedInput> secondHalf(demo.begin() + static_cast<std::ptrdiff_t>(kDemoMidIndex), demo.end());
    const std::int64_t shift = secondHalf.front().offsetNs;
    for (PlannedInput& in : secondHalf) {
        in.offsetNs -= shift;
    }

    m_injector->start(firstHalf, core::monotonicNowNs());
    waitFor([this] { return m_injector->finished(); }, 5000);
    waitMs(100);
    const QPoint inkPixel = sceneToImage(m_view->pageToScene(demoInkProbePoint()), m_window->devicePixelRatio());
    const QImage midStroke = m_window->grabWindow();

    m_injector->start(secondHalf, core::monotonicNowNs());
    waitFor([this] { return m_injector->finished(); }, 5000);
    waitMs(50);
    waitFor([this] { return m_view->cacheReady(); }, 10000); // trait figé dans le cache
    waitMs(100);
    const QImage endStroke = m_window->grabWindow();

    m_view->setZoom(8.0);
    waitFor([this] { return m_view->cacheReady(); }, 10000);
    waitMs(100);
    const QImage zoom800 = m_window->grabWindow();

    m_view->setZoom(1.0);
    waitFor([this] { return m_view->cacheReady(); }, 10000);
    waitMs(100);
    const QImage back100 = m_window->grabWindow();
    m_view->setExclusiveDevices({});

    const core::Witnesses& w = core::witnesses();
    const qreal dpr = m_window->devicePixelRatio();
    const auto at = [&](core::PointF p) { return sceneToImage(m_view->pageToScene(QPointF(p.x, p.y)), dpr); };
    result.contentChecks = {
        checkColor(QStringLiteral("witness-shape"), back100, at(w.shapeRect.center()),
                   QColor::fromRgba(w.shapeColor), kWitnessTolerance),
        checkColor(QStringLiteral("witness-image"), back100, at(w.imageRect.center()),
                   QColor::fromRgba(w.imageCenterColor), kWitnessTolerance),
        checkColor(QStringLiteral("witness-stroke"), back100, at(w.strokeProbe), QColor::fromRgba(w.strokeColor),
                   kWitnessTolerance),
        checkColor(QStringLiteral("ink-mid-stroke"), midStroke, inkPixel, QColor(Qt::black), kInkTolerance),
    };

    if (!m_options.capturesDir.isEmpty()) {
        QDir dir(m_options.capturesDir);
        dir.mkpath(QStringLiteral("."));
        const QImage images[] = {midStroke, endStroke, zoom800, back100};
        const QStringList steps = captureSteps();
        for (qsizetype i = 0; i < steps.size(); ++i) {
            images[i].save(dir.filePath(captureFileName(result.graphicsApi, steps.at(i))));
        }
    }
}
```

Dans `Runner::run()`, juste après la boucle des scénarios et avant `result.renderLoop = …`, ajouter :

```cpp
    if (result.invalidReasons.isEmpty()) { // à ce stade, seules des erreurs y figurent
        runDemo(result);
    }
```

- [ ] **Étape 4 : vert, passage hors écran, preuve visuelle, commit.** Commande bornée :
`tst_bench_content ... Passed`, `TESTS OK`. Puis le passage hors écran avec captures et la description des
quatre PNG. Si `ink-mid-stroke` échoue : comparer `expected`/`actual` ; un gris clair signale que l'encre n'est
pas dans la capture (ordre `InkLayer`, ou capture prise avant la synchronisation), un noir décalé un mauvais
rapport de pixels.

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add bench/CMakeLists.txt bench/include/actcraft/bench/contentcheck.h bench/src/contentcheck.cpp bench/include/actcraft/bench/runner.h bench/src/runner.cpp tests/bench.cmake tests/tst_bench_content.cpp
git -C <worktree> commit -m "bench : séquence de démonstration, captures et contrôle de contenu (témoins, encre)" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

### Tâche T18 — Fumée ctest du banc, `tst_bench_result`, pas CI macOS

**Objectif.** Le banc tourne à chaque passage des tests (décision 33, S4) : une fixture ctest efface
`<build>/bench-smoke.json`, puis `tst_bench_smoke` lance
`actcraft-bench --scenario court --rendu logiciel --taille 1280x800 --sortie <build>/bench-smoke.json` hors écran
(`TIMEOUT 60`) et vérifie son code de sortie (2 en debug : `MESURE INVALIDE` ; 0 ou 1 en release) ; enfin
`tst_bench_result` (test Qt dépendant) juge le contenu du JSON : format, `graphics_api = software`,
`valid_for_measurement` (faux en debug, vrai en release), trois scénarios avec des images, latence du tracé,
`content_check` ok sur 4 contrôles, critères du verdict, aucun nom d'hôte. La CI macOS ajoute un pas
**informatif** (`continue-on-error`) qui lance le banc en `auto` sur la plateforme `cocoa` et affiche
`graphics_api` (N1-8). Aucun seuil de performance ni test D3D11 en CI.

**Fichiers.**
- Créer : `tests/run_bench_smoke.cmake`, `tests/tst_bench_result.cpp`
- Modifier : `tests/bench.cmake`, `.github/workflows/ci.yml`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|(bench_smoke_setup|tst_bench_smoke|tst_bench_result) .*(Passed|Failed)' | Select-Object -Last 5
```
Attendu : `out of N` avec N ≥ 27, `bench_smoke_setup`, `tst_bench_smoke` et `tst_bench_result` `Passed`,
`TESTS OK`.

**Interdits.** Pas de `WILL_FAIL` (il accepterait n'importe quel échec). Pas de seuil de performance en CI. Pas
d'artefact publié par la CI (N1-4).

**Terminé quand.** Preuve verte en debug ; `yamllint` n'est pas requis, mais le YAML est relu (indentation de
2 espaces, même style que le fichier) ; commit fait. Le pas macOS est vérifié par la CI de la PR (rapporter
« non vérifié localement »).

**Étapes.**

- [ ] **Étape 1 : test du JSON (rouge).** Créer `tests/tst_bench_result.cpp` :

```cpp
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>
#include <QTest>

// Juge le JSON produit par tst_bench_smoke (fixture ctest bench_smoke_json).
class TstBenchResult : public QObject
{
    Q_OBJECT

private:
    QJsonObject m_root;
    QByteArray m_text;

private slots:
    void initTestCase()
    {
        const QString path = qEnvironmentVariable("ACTCRAFT_BENCH_SMOKE_JSON");
        QVERIFY2(!path.isEmpty(), "ACTCRAFT_BENCH_SMOKE_JSON absent");
        QFile file(path);
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(path));
        m_text = file.readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(m_text);
        QVERIFY(doc.isObject());
        m_root = doc.object();
    }

    void formatIsVersioned()
    {
        QCOMPARE(m_root.value("format").toString(), QStringLiteral("actcraft-bench/1"));
    }

    void softwareRenderer()
    {
        QCOMPARE(m_root.value("graphics_api").toString(), QStringLiteral("software"));
        QVERIFY(m_root.value("informative").toBool());
    }

    void validityFollowsBuildType()
    {
        QCOMPARE(m_root.value("valid_for_measurement").toBool(), ACTCRAFT_EXPECT_VALID != 0);
    }

    void threeScenariosWithFrames()
    {
        const QJsonArray scenarios = m_root.value("scenarios").toArray();
        QCOMPARE(scenarios.size(), 3);
        const QStringList names{QStringLiteral("stroke"), QStringLiteral("zoom"), QStringLiteral("scroll")};
        for (qsizetype i = 0; i < 3; ++i) {
            const QJsonObject s = scenarios.at(i).toObject();
            QCOMPARE(s.value("name").toString(), names.at(i));
            QVERIFY(s.value("frames").toObject().value("count").toInt() > 0);
            QVERIFY(s.value("frames").toObject().value("active").toInt() > 0);
        }
    }

    void strokeHasLatency()
    {
        const QJsonObject latency =
            m_root.value("scenarios").toArray().at(0).toObject().value("latency").toObject();
        QVERIFY(latency.value("samples").toInt() > 0);
        QVERIFY(latency.value("total_ms").toObject().value("p50").toDouble() > 0.0);
    }

    void contentCheckPasses()
    {
        const QJsonObject check = m_root.value("content_check").toObject();
        QVERIFY(check.value("ok").toBool());
        const QJsonArray checks = check.value("checks").toArray();
        QCOMPARE(checks.size(), 4);
        for (const QJsonValue& c : checks) {
            QVERIFY2(c.toObject().value("ok").toBool(), qPrintable(c.toObject().value("name").toString()));
        }
    }

    void verdictHasCriteria()
    {
        QVERIFY(!m_root.value("verdict").toObject().value("criteria").toArray().isEmpty());
    }

    void noHostName()
    {
        const QString host = QSysInfo::machineHostName();
        if (host.size() >= 3) {
            QVERIFY(!QString::fromUtf8(m_text).contains(host, Qt::CaseInsensitive));
        }
    }
};

QTEST_GUILESS_MAIN(TstBenchResult)
#include "tst_bench_result.moc"
```

- [ ] **Étape 2 : script de fumée.** Créer `tests/run_bench_smoke.cmake` :

```cmake
# Lance le banc en fumée et vérifie son code de sortie.
#   cmake -DBENCH=<exe> -DOUTPUT=<json> -DEXPECTED=<codes séparés par des virgules> -P run_bench_smoke.cmake
# Debug : 2 (MESURE INVALIDE, construction non release) ; release : 0 ou 1 (le verdict n'est pas une porte ici).
execute_process(
    COMMAND "${BENCH}" --scenario court --rendu logiciel --taille 1280x800 --sortie "${OUTPUT}"
    RESULT_VARIABLE code
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err)
message("${out}")
string(REPLACE "," "|" expected_regex "${EXPECTED}")
if(NOT code MATCHES "^(${expected_regex})$")
    message(FATAL_ERROR "actcraft-bench : code ${code}, attendu ${EXPECTED}\n${err}")
endif()
if(NOT EXISTS "${OUTPUT}")
    message(FATAL_ERROR "actcraft-bench n'a pas écrit ${OUTPUT}")
endif()
```

- [ ] **Étape 3 : déclarer la fumée.** Dans `tests/bench.cmake`, ajouter :

```cmake
# Fumée du banc (décision 33) : sans actcraft_add_test, qui construit son propre exécutable.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_bench_smoke_expected "2")
    set(_bench_smoke_valid 0)
else()
    set(_bench_smoke_expected "0,1")
    set(_bench_smoke_valid 1)
endif()
set(_bench_smoke_json ${CMAKE_BINARY_DIR}/bench-smoke.json)

add_test(NAME bench_smoke_setup COMMAND ${CMAKE_COMMAND} -E rm -f ${_bench_smoke_json})
set_tests_properties(bench_smoke_setup PROPERTIES FIXTURES_SETUP bench_smoke_clean)

add_test(NAME tst_bench_smoke
    COMMAND ${CMAKE_COMMAND}
        -DBENCH=$<TARGET_FILE:actcraft-bench>
        -DOUTPUT=${_bench_smoke_json}
        -DEXPECTED=${_bench_smoke_expected}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_bench_smoke.cmake)
set_tests_properties(tst_bench_smoke PROPERTIES
    TIMEOUT 60
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    FIXTURES_REQUIRED bench_smoke_clean
    FIXTURES_SETUP bench_smoke_json)

actcraft_add_test(tst_bench_result
    SOURCES tst_bench_result.cpp
    LIBS Qt6::Core)
target_compile_definitions(tst_bench_result PRIVATE ACTCRAFT_EXPECT_VALID=${_bench_smoke_valid})
set_tests_properties(tst_bench_result PROPERTIES
    FIXTURES_REQUIRED bench_smoke_json
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;ACTCRAFT_BENCH_SMOKE_JSON=${_bench_smoke_json}")
```
Commande bornée. Attendu d'abord `tst_bench_result` rouge si le JSON manque ; une fois la fumée déclarée, vert.

- [ ] **Étape 4 : CI macOS.** Dans `.github/workflows/ci.yml`, job `build`, après le pas `Tester`, ajouter :

```yaml
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
```

- [ ] **Étape 5 : vert, format, commit.** Commande bornée : les trois entrées `Passed`, `TESTS OK`.

```powershell
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/format.ps1
Set-Location <worktree>; git add tests/bench.cmake tests/run_bench_smoke.cmake tests/tst_bench_result.cpp .github/workflows/ci.yml
git -C <worktree> commit -m "Banc en fumée par ctest, contrôle du JSON, pas Metal informatif en CI macOS" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

### Tâche T19 — Scripts `bench.ps1`, `bench.sh`, `paquet-banc.ps1` ; commandes dans le profil

**Objectif.** Les commandes du banc (décisions 29, 34, 35, 36) :
- `scripts/bench.ps1 [-Rendu d3d11|logiciel|tous] [-SansCache] [-Captures <dossier>] [-Scenario complet|court]
  [-Taille plein-ecran|LxH] [-HorsEcran] [-SansConstruire]` : construit en release, joue d3d11 puis logiciel
  (défaut `tous`), écrit dans `bench/results/`, relance **une fois** un passage invalide, et sort sur le verdict
  **du rendu par défaut** (d3d11) ; `-HorsEcran` (ajout du plan, pour les preuves de tâche) fixe
  `QT_QPA_PLATFORM=offscreen` et ne montre aucune fenêtre ;
- `scripts/bench.sh` : même chose sous macOS, compatible bash 3.2 (ni tableaux associatifs ni `${x,,}`), défaut
  `--rendu tous` = `auto` puis `logiciel`, sortie sur le verdict de `auto` ;
- `scripts/paquet-banc.ps1` : zip local pour le Dell 3510 (release, `windeployqt --release
  --no-compiler-runtime`, les trois DLL d'exécution de MSVC copiées à côté de l'exécutable) ; jamais publié ;
- `.claude/chantier.md` § « Banc de fluidité » : les commandes et leur forme bornée.

**Fichiers.**
- Créer : `scripts/bench.ps1`, `scripts/bench.sh`, `scripts/paquet-banc.ps1`
- Modifier : `.claude/chantier.md`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu logiciel -Scenario court -Taille 1280x800 -HorsEcran > .tmp/bench.txt 2>&1 ; "code=$LASTEXITCODE"; Select-String -Path .tmp/bench.txt -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE' | Select-Object -Last 1; Get-ChildItem bench/results -Filter *software.json | Select-Object -Last 1 -ExpandProperty Name
Set-Location <worktree>; bash -n scripts/bench.sh; "bash-n=$LASTEXITCODE"
Set-Location <worktree>; pwsh -NoProfile -File ./scripts/paquet-banc.ps1 > .tmp/paquet.txt 2>&1 ; "code=$LASTEXITCODE"; Select-String -Path .tmp/paquet.txt -Pattern 'Paquet :' | Select-Object -Last 1
```
Attendu : `out of N` avec N ≥ 24 et `TESTS OK` ; `code=0` ou `code=1` avec `BUDGET TENU` ou `BUDGET DEPASSE`
(construction release, hors écran : passage valide ; le verdict n'est pas une porte, décision 38) et un fichier
`…-software.json` dans `bench/results/` ; `bash-n=0` ; `code=0` et une ligne `Paquet : …\actcraft-bench-<commit>.zip`.

**Interdits.** Aucun lancement en plein écran (pas de passage sans `-HorsEcran` ni `-Taille`). Ne pas modifier
d'autre section du profil que « Banc de fluidité » (le bloc `interdits` est lu par un hook). Le zip reste dans
`build/` : jamais commité, jamais publié.

**Terminé quand.** Les quatre preuves sont conformes ; `scripts/bench.sh` est exécutable dans git
(`git ls-files -s scripts/bench.sh` commence par `100755`) ; commit fait.

**Étapes.**

- [ ] **Étape 1 : `scripts/bench.ps1`.**

```powershell
# Banc de fluidité : construit en release, puis joue tracé, zoom et défilement (D3D11 puis logiciel).
# Usage : pwsh -NoProfile -File ./scripts/bench.ps1 [-Rendu d3d11|logiciel|tous] [-SansCache] [-Captures <dossier>]
#         [-Scenario complet|court] [-Taille plein-ecran|LxH] [-HorsEcran] [-SansConstruire]
# Résultats : bench/results/<horodatage>-<modèle>-<rendu>.json (ignoré par git).
# Dernière ligne et code de sortie : verdict du rendu par défaut (d3d11) — BUDGET TENU (0), BUDGET DEPASSE (1),
# MESURE INVALIDE (2). Le rendu logiciel est informatif (N1-3).
param(
    [ValidateSet('d3d11', 'logiciel', 'tous')][string]$Rendu = 'tous',
    [switch]$SansCache,
    [string]$Captures,
    [ValidateSet('complet', 'court')][string]$Scenario = 'complet',
    [string]$Taille = 'plein-ecran',
    [switch]$HorsEcran,
    [switch]$SansConstruire
)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

if (-not $SansConstruire) {
    & "$PSScriptRoot/build.ps1" -Preset release
    if ($LASTEXITCODE -ne 0) { Write-Output 'MESURE INVALIDE (compilation)'; exit 2 }
}
. "$PSScriptRoot/dev-env.ps1"

$exe = Join-Path (Get-Location) 'build/release/actcraft-bench.exe'
if (-not (Test-Path $exe)) { Write-Output "MESURE INVALIDE ($exe introuvable)"; exit 2 }
$resultats = Join-Path (Get-Location) 'bench/results'
New-Item -ItemType Directory -Force $resultats | Out-Null

if ($HorsEcran) {
    $env:QT_QPA_PLATFORM = 'offscreen'
} elseif ($Taille -eq 'plein-ecran') {
    Write-Output 'La fenêtre du banc couvre l''écran principal pendant 30 à 60 s par rendu.'
}

$rendus = if ($Rendu -eq 'tous') { @('d3d11', 'logiciel') } else { @($Rendu) }
$codes = @{}
foreach ($r in $rendus) {
    $arguments = @('--rendu', $r, '--scenario', $Scenario, '--taille', $Taille, '--sortie', "$resultats/")
    if ($SansCache) { $arguments += '--sans-cache' }
    if ($Captures) {
        New-Item -ItemType Directory -Force $Captures | Out-Null
        $arguments += @('--captures', (Resolve-Path $Captures).Path)
    }
    $code = 2
    for ($essai = 1; $essai -le 2; $essai++) {
        & $exe @arguments
        $code = $LASTEXITCODE
        if ($code -ne 2) { break }
        if ($essai -eq 1) { Write-Output "Passage $r invalide : nouvel essai." }
    }
    $codes[$r] = $code
}

$final = if ($codes.ContainsKey('d3d11')) { $codes['d3d11'] } else { $codes[$rendus[0]] }
$verdict = switch ($final) { 0 { 'BUDGET TENU' } 1 { 'BUDGET DEPASSE' } default { 'MESURE INVALIDE' } }
Write-Output $verdict
exit $final
```

- [ ] **Étape 2 : `scripts/bench.sh`.**

```bash
#!/usr/bin/env bash
# Banc de fluidité sous macOS : construit en release, puis joue tracé, zoom et défilement.
# Usage : scripts/bench.sh [--rendu auto|logiciel|tous] [--sans-cache] [--captures DOSSIER]
#                          [--scenario complet|court] [--taille plein-ecran|LxH] [--hors-ecran] [--sans-construire]
# Prérequis : ACTCRAFT_QT_DIR (dossier de Qt 6.12, par ex. ~/Qt/6.12.0/macos), CMake, Ninja, outils Xcode.
# Dernière ligne et code de sortie : verdict de « auto » (Metal) — BUDGET TENU (0), BUDGET DEPASSE (1),
# MESURE INVALIDE (2). Compatible bash 3.2 : ni tableaux associatifs ni ${x,,}.
set -u
cd "$(dirname "$0")/.." || exit 2

rendu=tous
sans_cache=0
captures=""
scenario=complet
taille=plein-ecran
hors_ecran=0
construire=1
while [ $# -gt 0 ]; do
  case "$1" in
    --rendu) rendu="$2"; shift 2 ;;
    --sans-cache) sans_cache=1; shift ;;
    --captures) captures="$2"; shift 2 ;;
    --scenario) scenario="$2"; shift 2 ;;
    --taille) taille="$2"; shift 2 ;;
    --hors-ecran) hors_ecran=1; shift ;;
    --sans-construire) construire=0; shift ;;
    *) echo "option inconnue : $1" >&2; echo "MESURE INVALIDE"; exit 2 ;;
  esac
done
case "$rendu" in
  auto|logiciel|tous) ;;
  *) echo "--rendu : auto, logiciel ou tous" >&2; echo "MESURE INVALIDE"; exit 2 ;;
esac

if [ "$construire" = 1 ]; then
  if [ -z "${ACTCRAFT_QT_DIR:-}" ]; then
    echo "ACTCRAFT_QT_DIR non défini (dossier de Qt 6.12)" >&2; echo "MESURE INVALIDE"; exit 2
  fi
  if ! cmake --preset release || ! cmake --build --preset release; then
    echo "MESURE INVALIDE (compilation)"; exit 2
  fi
fi

exe="build/release/actcraft-bench"
if [ ! -x "$exe" ]; then echo "MESURE INVALIDE ($exe introuvable)"; exit 2; fi
mkdir -p bench/results
if [ "$hors_ecran" = 1 ]; then
  export QT_QPA_PLATFORM=offscreen
elif [ "$taille" = plein-ecran ]; then
  echo "La fenêtre du banc couvre l'écran principal pendant 30 à 60 s par rendu."
fi

if [ "$rendu" = tous ]; then rendus="auto logiciel"; else rendus="$rendu"; fi
code_auto=""
dernier=2
for r in $rendus; do
  set -- --rendu "$r" --scenario "$scenario" --taille "$taille" --sortie "bench/results/"
  if [ "$sans_cache" = 1 ]; then set -- "$@" --sans-cache; fi
  if [ -n "$captures" ]; then mkdir -p "$captures"; set -- "$@" --captures "$captures"; fi
  essai=1
  while :; do
    "$exe" "$@"
    code=$?
    if [ "$code" != 2 ] || [ "$essai" = 2 ]; then break; fi
    echo "Passage $r invalide : nouvel essai."
    essai=2
  done
  dernier=$code
  if [ "$r" = auto ]; then code_auto=$code; fi
done

final=${code_auto:-$dernier}
case "$final" in
  0) echo "BUDGET TENU" ;;
  1) echo "BUDGET DEPASSE" ;;
  *) echo "MESURE INVALIDE"; final=2 ;;
esac
exit "$final"
```
Puis : `git -C <worktree> add scripts/bench.sh` et `git -C <worktree> update-index --chmod=+x scripts/bench.sh`.

- [ ] **Étape 3 : `scripts/paquet-banc.ps1`.**

```powershell
# Paquet du banc pour une machine sans outils de construction (Dell 3510, étape 2 du BRIEF).
# Usage : pwsh -NoProfile -File ./scripts/paquet-banc.ps1
# Produit build/paquet-banc/actcraft-bench-<commit>.zip : local, jamais publié (pas de distribution avant le go).
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')

& "$PSScriptRoot/build.ps1" -Preset release
if ($LASTEXITCODE -ne 0) { throw 'compilation release en échec' }
. "$PSScriptRoot/dev-env.ps1"
if (-not $env:VCToolsRedistDir) { throw 'VCToolsRedistDir absent : ouvrir un environnement Visual Studio (dev-env.ps1).' }

$commit = (git rev-parse --short=12 HEAD).Trim()
$racine = Join-Path (Get-Location) 'build/paquet-banc'
$dossier = Join-Path $racine 'actcraft-bench'
if (Test-Path $dossier) { Remove-Item -Recurse -Force $dossier }
New-Item -ItemType Directory -Force $dossier | Out-Null
Copy-Item build/release/actcraft-bench.exe $dossier

$windeployqt = Join-Path $env:ACTCRAFT_QT_DIR 'bin/windeployqt.exe'
& $windeployqt --release --no-compiler-runtime --qmldir ui/qml (Join-Path $dossier 'actcraft-bench.exe')
if ($LASTEXITCODE -ne 0) { throw 'windeployqt en échec' }

# Bibliothèques d'exécution de MSVC à côté de l'exécutable : --compiler-runtime livrerait l'installeur vc_redist,
# qui demande les droits d'administrateur.
$crt = Join-Path $env:VCToolsRedistDir 'x64\Microsoft.VC143.CRT'
foreach ($dll in 'vcruntime140.dll', 'vcruntime140_1.dll', 'msvcp140.dll') {
    Copy-Item (Join-Path $crt $dll) $dossier
}
if (Test-Path docs/banc.md) { Copy-Item docs/banc.md (Join-Path $dossier 'LISEZMOI-banc.md') }

$zip = Join-Path $racine "actcraft-bench-$commit.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path $dossier -DestinationPath $zip
Write-Output "Paquet : $zip"
```

- [ ] **Étape 4 : profil.** Dans `.claude/chantier.md`, section « Banc de fluidité », remplacer la phrase
« Commande à écrire avec le banc. » par :

```markdown
- Commande : `pwsh -NoProfile -File ./scripts/bench.ps1` (release ; D3D11 puis logiciel ; plein écran sur
  l'écran principal ; JSON dans `bench/results/`). Dernière ligne : `BUDGET TENU` (0), `BUDGET DEPASSE` (1),
  `MESURE INVALIDE` (2) ; code de sortie = verdict du rendu par défaut (d3d11). Options : `-Rendu
  d3d11|logiciel|tous`, `-SansCache`, `-Captures <dossier>`, `-Scenario complet|court`, `-Taille
  plein-ecran|LxH`, `-HorsEcran`, `-SansConstruire`.
- Forme bornée : `pwsh -NoProfile -File ./scripts/bench.ps1 > .tmp/bench.txt 2>&1 ; Select-String -Path
  .tmp/bench.txt -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE' | Select-Object -Last 1`
- Preuve de tâche sans écran (aucune fenêtre visible) : ajouter `-Rendu logiciel -Scenario court -Taille
  1280x800 -HorsEcran`. Un passage en plein écran se joue sous le jeton `ecran`, annoncé d'une ligne à Jérôme.
- macOS : `scripts/bench.sh` (`--rendu auto|logiciel|tous`, `--sans-cache`, `--captures`, `--hors-ecran`).
- Paquet pour le Dell 3510 : `pwsh -NoProfile -File ./scripts/paquet-banc.ps1` (zip local dans
  `build/paquet-banc/`, jamais publié). Procédure : `docs/banc.md`.
```

- [ ] **Étape 5 : preuves, commit.** Lancer les quatre commandes de preuve.

```powershell
Set-Location <worktree>; git add scripts/bench.ps1 scripts/bench.sh scripts/paquet-banc.ps1 .claude/chantier.md
Set-Location <worktree>; git update-index --chmod=+x scripts/bench.sh
git -C <worktree> commit -m "Scripts du banc (Windows, macOS), paquet Dell, commandes dans le profil" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

### Tâche T20 — Résumé `resume-banc.ps1` et procédure `docs/banc.md`

**Objectif.** Rendre chiffrable la comparaison avec et sans cache (S3) et possible la mesure de l'étape 2
(décision 36) :
- `scripts/resume-banc.ps1 -Entrees <fichiers ou dossiers JSON> [-Sortie <fichier.md>]` : tableau Markdown, une
  ligne par (rendu, cache, scénario) : images/s, p95 d'intervalle, part > 25 ms, latence p50 / p95, pic mémoire
  (privée + graphique), verdict, validité ; une liste des fichiers avec leurs métadonnées utiles (modèle,
  processeur, puce graphique, écran, commit) — **jamais de nom d'hôte** ; dernière ligne de console
  `RESUME OK (<n> fichiers)` ;
- un test ctest `tst_bench_summary` qui passe le JSON de fumée au script (si `pwsh` est présent) ;
- `docs/banc.md` : commandes, options, format du fichier, verdict et seuils, ce que la latence interne ne prouve
  pas, mesure bout à bout par caméra rapide (non outillée), procédures exactes Dell 3510 et MacBook M1,
  identification du matériel.

**Fichiers.**
- Créer : `scripts/resume-banc.ps1`, `docs/banc.md`
- Modifier : `tests/bench.cmake`

**Test et commande bornée.**
```powershell
Set-Location <worktree>; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1 ; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)|tst_bench_summary .*(Passed|Failed)' | Select-Object -Last 3
Set-Location <worktree>; Get-Content build/debug/bench-smoke-resume.md -TotalCount 12
```
Attendu : `out of N` avec N ≥ 28, `tst_bench_summary ... Passed`, `TESTS OK` ; le résumé montre l'en-tête du
tableau et trois lignes `software | oui | stroke|zoom|scroll`.

**Interdits.** Aucun nom d'hôte, nom d'utilisateur ou chemin personnel dans le résumé. Aucune ressource
Promethean dans `docs/banc.md` (textes et noms écrits par nous).

**Terminé quand.** Preuve verte ; `docs/banc.md` couvre les sections listées ; commit fait.

**Étapes.**

- [ ] **Étape 1 : test (rouge).** Dans `tests/bench.cmake`, ajouter :

```cmake
# Résumé Markdown du banc (S3), sur le JSON de la fumée.
find_program(ACTCRAFT_PWSH pwsh)
if(ACTCRAFT_PWSH)
    add_test(NAME tst_bench_summary
        COMMAND ${ACTCRAFT_PWSH} -NoProfile -File ${PROJECT_SOURCE_DIR}/scripts/resume-banc.ps1
            -Entrees ${_bench_smoke_json} -Sortie ${CMAKE_BINARY_DIR}/bench-smoke-resume.md)
    set_tests_properties(tst_bench_summary PROPERTIES
        FIXTURES_REQUIRED bench_smoke_json
        PASS_REGULAR_EXPRESSION "RESUME OK")
endif()
```
Commande bornée : `tst_bench_summary ... Failed` (script absent).

- [ ] **Étape 2 : `scripts/resume-banc.ps1`.**

```powershell
# Résumé chiffré de passages du banc, en Markdown (S3 ; docs/mesures/).
# Usage : pwsh -NoProfile -File ./scripts/resume-banc.ps1 -Entrees <fichier.json|dossier>[,...] [-Sortie <fichier.md>]
# Dernière ligne de console : « RESUME OK (<n> fichiers) ». Aucun nom d'hôte : le JSON n'en contient pas.
param(
    [Parameter(Mandatory)][string[]]$Entrees,
    [string]$Sortie
)
$ErrorActionPreference = 'Stop'

$fichiers = @(foreach ($e in $Entrees) {
        if (Test-Path $e -PathType Container) { Get-ChildItem $e -Filter *.json | Sort-Object Name }
        else { Get-Item $e }
    })
if ($fichiers.Count -eq 0) { throw 'aucun fichier JSON' }

$inv = [cultureinfo]::InvariantCulture
function Nombre($valeur, [int]$decimales = 1) {
    if ($null -eq $valeur) { return '—' }
    return ([double]$valeur).ToString("F$decimales", $inv)
}

$lignes = [System.Collections.Generic.List[string]]::new()
$lignes.Add('# Résumé du banc de fluidité')
$lignes.Add('')
$lignes.Add('| Rendu | Cache | Scénario | Images/s | p95 intervalle (ms) | > 25 ms | Latence p50 / p95 (ms) | Pic mémoire (Mo) | Verdict | Valide |')
$lignes.Add('|---|---|---|---|---|---|---|---|---|---|')
$sources = [System.Collections.Generic.List[string]]::new()
foreach ($f in $fichiers) {
    $j = Get-Content $f.FullName -Raw | ConvertFrom-Json
    if ($j.format -ne 'actcraft-bench/1') { throw "$($f.Name) : format inconnu ($($j.format))" }
    $verdict = if (-not $j.valid_for_measurement) { 'invalide' } elseif ($j.within_budget) { 'tenu' } else { 'dépassé' }
    if ($j.informative) { $verdict += ' (informatif)' }
    $cache = if ($j.cache) { 'oui' } else { 'non' }
    $valide = if ($j.valid_for_measurement) { 'oui' } else { 'non' }
    foreach ($s in $j.scenarios) {
        $latence = if ($s.latency) { "$(Nombre $s.latency.total_ms.p50) / $(Nombre $s.latency.total_ms.p95)" } else { '—' }
        $gpu = if ($null -ne $s.memory.gpu_peak_bytes) { [double]$s.memory.gpu_peak_bytes } else { 0.0 }
        $memoire = ([double]$s.memory.process_private_peak_bytes + $gpu) / 1MB
        $part = 100.0 * [double]$s.frames.over_25ms_ratio
        $lignes.Add("| $($j.graphics_api) | $cache | $($s.name) | $(Nombre $s.frames.fps) | $(Nombre $s.frames.p95_ms) | $(Nombre $part 2) % | $latence | $(Nombre $memoire 0) | $verdict | $valide |")
    }
    $m = $j.meta
    $sources.Add("- ``$($f.Name)`` : $($j.graphics_api), cache $cache, $($m.machine_model), $($m.cpu), puce $($m.gpu), écran $($m.screen.width) × $($m.screen.height) à $(Nombre $m.screen.refresh_hz 0) Hz, fenêtre $($m.window.mode), boucle $($m.render_loop), construction $($m.build_type), commit $($m.commit)")
}
$lignes.Add('')
$lignes.Add('Fichiers :')
$lignes.Add('')
foreach ($s in $sources) { $lignes.Add($s) }

$texte = ($lignes -join "`n") + "`n"
if ($Sortie) {
    [System.IO.File]::WriteAllText($Sortie, $texte, [System.Text.UTF8Encoding]::new($false))
} else {
    Write-Output $texte
}
Write-Output "RESUME OK ($($fichiers.Count) fichiers)"
```

- [ ] **Étape 3 : `docs/banc.md`.** Créer le fichier avec ce contenu :

~~~markdown
# Banc de fluidité — mode d'emploi

Le banc `actcraft-bench` charge la vraie interface d'ActCraft, rejoue un tracé, un zoom et un défilement par des
entrées synthétiques (aucune souris pilotée), et écrit temps par image, latence d'encre et mémoire dans un JSON.
Il sert l'étape 2 du BRIEF : décider go / no-go sur un Dell Inspiron 15 3510 et un MacBook Air M1.

## Commandes

| Besoin | Commande |
|---|---|
| Passage complet sous Windows (D3D11 puis logiciel, plein écran) | `pwsh -NoProfile -File ./scripts/bench.ps1` |
| Sans cache de la page fixe | `pwsh -NoProfile -File ./scripts/bench.ps1 -SansCache` |
| Avec captures | `pwsh -NoProfile -File ./scripts/bench.ps1 -Captures .tmp/captures` |
| Essai rapide sans fenêtre | `pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu logiciel -Scenario court -Taille 1280x800 -HorsEcran` |
| macOS (Metal puis logiciel) | `scripts/bench.sh` |
| Paquet pour une machine sans outils | `pwsh -NoProfile -File ./scripts/paquet-banc.ps1` |
| Résumé Markdown | `pwsh -NoProfile -File ./scripts/resume-banc.ps1 -Entrees bench/results -Sortie resume.md` |
| Exécutable seul | `actcraft-bench [--rendu d3d11\|logiciel\|auto] [--scenario complet\|court] [--sortie <fichier.json\|dossier>] [--sans-cache] [--taille plein-ecran\|LxH] [--captures <dossier>]` |

Sans `--sortie`, le JSON va dans `resultats-banc/` à côté de l'exécutable ; les scripts écrivent dans
`bench/results/` (ignoré par git). Un passage complet dure environ 70 à 90 s pour les deux rendus ; la fenêtre
couvre l'écran principal pendant ce temps.

## Ce qui est joué

Après « page prête » (images générées, premier cache dessiné) et 2 s de chauffe :

| Scénario | Contenu | Niveau imposé |
|---|---|---|
| `stroke` | 10 traits de 1,5 s à 200 points/s, pression variable, stylo levé 300 ms entre deux traits | 100 % |
| `zoom` | 100 → 800 → 25 → 100 % en 6 s, molette fine Ctrl à 120 Hz | 100 % |
| `scroll` | aller-retour vertical puis horizontal en 6 s | 400 %, page calée en haut à gauche |

Puis une séquence de démonstration non mesurée : un trait capturé à mi-parcours, à la fin, à 800 % et de retour à
100 %. Quatre contrôles de couleur (trois objets témoins, l'encre en cours) prouvent que le rendu dessine
vraiment ; `--captures` écrit les quatre images `<rendu>-<étape>.png`. `--scenario court` divise les durées par 5.

## Fichier de résultats

Un JSON par passage, `"format": "actcraft-bench/1"`, nommé `<AAAA-MM-JJ_HHMMSS>-<modèle>-<rendu>.json`. Il ne
contient ni nom d'hôte ni nom d'utilisateur. Clés principales : `graphics_api`, `cache`,
`valid_for_measurement`, `within_budget`, `informative`, `meta` (commit, Qt, système, processeur, puce graphique,
écran et fréquence, rapport de pixels, boucle de rendu, type de construction, options de compilation, fenêtre),
`scenarios` (statistiques, latence, entrées, mémoire, séries brutes), `content_check`, `verdict`.

## Verdict

Dernière ligne et code de sortie : `BUDGET TENU` (0), `BUDGET DEPASSE` (1), `MESURE INVALIDE` (2). Les scripts
sortent sur le verdict du rendu par défaut (Direct3D 11 sous Windows, Metal sous macOS) ; le rendu logiciel est
mesuré mais informatif.

| Critère | Seuil |
|---|---|
| Images/s (1000 / intervalle médian des images actives) | ≥ 58 |
| p95 de l'intervalle entre images | ≤ 18,5 ms |
| Part des intervalles > 25 ms | ≤ 1 % |
| Figement d'un trait (comptés à part) | < 2,5 périodes d'écran |
| Latence interne d'encre, p50 | < 20 ms |
| Pic de mémoire privée + pic de mémoire graphique | ≤ 1 Gio |

Un intervalle n'est « actif » que si une entrée y était prévue depuis l'image précédente, au plus 2 périodes
d'écran après elle (instant prévu, pas instant d'envoi : un fil principal bloqué ne masque pas son retard) : les pauses du stylo et l'attente des redessins ne comptent pas. Un passage est invalide si la
construction n'est pas `release`, si la fenêtre perd son exposition, si un contrôle de contenu échoue, ou sur
erreur ; les scripts relancent une fois un passage invalide.

## Ce que la latence mesurée ne prouve pas

La latence est **interne** : de l'instant prévu de l'échantillon à la présentation (`frameSwapped`) de la
première image qui le contient, en deux composantes (prévu → synchronisation, synchronisation → présentation).
Elle ne contient ni le numériseur du stylet, ni la pile d'entrée du système, ni la composition (DWM sous
Windows), ni le balayage de l'écran, qui ajoutent en général une à deux images (17 à 33 ms à 60 Hz). Une latence
interne < 20 ms ne prouve donc pas une latence perçue < 20 ms. Les entrées sont des événements Qt postés à la
fenêtre : la compression des événements par Qt et le pilote du stylet ne sont pas mesurés.

**Mesure bout à bout (non outillée).** Filmer à 240 images/s ou plus la pointe du stylet et l'encre, avec un
trait rapide, puis compter les images entre le contact et l'apparition de l'encre. À faire à la main si le
verdict interne est proche du seuil.

## Procédure Dell Inspiron 15 3510 (Windows 11)

Sur le poste de développement :
1. `pwsh -NoProfile -File ./scripts/paquet-banc.ps1` → `build/paquet-banc/actcraft-bench-<commit>.zip`.
2. Copier le zip sur une clé USB.

Sur le Dell (secteur branché, mode d'alimentation « Performances optimales », navigateur ouvert avec un onglet
ordinaire, comme en classe ; écran externe 1080p si c'est l'usage visé) :
1. Extraire le zip dans `C:\banc`.
2. Ouvrir PowerShell : `cd C:\banc\actcraft-bench`.
3. Si SmartScreen bloque l'exécutable (non signé) : « Informations complémentaires » → « Exécuter quand même ».
4. Lancer, sans toucher à la souris ni au clavier pendant chaque passage (la fenêtre couvre l'écran) :
   ```powershell
   .\actcraft-bench.exe
   .\actcraft-bench.exe --rendu logiciel
   .\actcraft-bench.exe --sans-cache
   .\actcraft-bench.exe --captures captures
   ```
5. Relever le matériel :
   ```powershell
   Get-CimInstance Win32_ComputerSystem | Select-Object Manufacturer, Model, TotalPhysicalMemory
   Get-CimInstance Win32_Processor | Select-Object Name
   Get-CimInstance Win32_VideoController | Select-Object Name, DriverVersion, CurrentRefreshRate
   ```
6. Rapporter le dossier `resultats-banc\` (et `captures\`) et la sortie de l'étape 5.

## Procédure MacBook Air M1 (macOS 14.4 ou plus)

1. Installer les outils : `xcode-select --install`, puis CMake et Ninja (`brew install cmake ninja`, ou les
   paquets officiels), et Python 3.12.
2. Installer Qt 6.12 :
   ```bash
   python3 -m pip install --user "git+https://github.com/miurahr/aqtinstall"
   python3 -m aqt install-qt mac desktop 6.12.0 clang_64 -m qtshadertools qtimageformats --outputdir ~/Qt
   export ACTCRAFT_QT_DIR=~/Qt/6.12.0/macos
   ```
3. `git clone https://github.com/jejeasterix/actcraft.git && cd actcraft`
4. Secteur branché, puis : `scripts/bench.sh` (Metal puis logiciel), `scripts/bench.sh --sans-cache`,
   `scripts/bench.sh --captures captures`. Ne pas toucher au trackpad pendant les passages.
5. Relever le matériel : `sysctl -n hw.model machdep.cpu.brand_string hw.memsize` et
   `system_profiler SPDisplaysDataType`.
6. Rapporter `bench/results/` (et `captures/`) et la sortie de l'étape 5.

## Résumé

`pwsh -NoProfile -File ./scripts/resume-banc.ps1 -Entrees <JSON ou dossiers> -Sortie docs/mesures/<date>-<sujet>.md`
produit le tableau comparatif (rendu × cache × scénario). Seuls les résumés commités dans `docs/mesures/` font foi ;
les JSON restent hors du dépôt.
~~~

- [ ] **Étape 4 : vert, commit.** Commande bornée : `tst_bench_summary ... Passed`, `TESTS OK` ; lire les
12 premières lignes du résumé de fumée.

```powershell
Set-Location <worktree>; git add scripts/resume-banc.ps1 docs/banc.md tests/bench.cmake
git -C <worktree> commit -m "Résumé Markdown des passages du banc et procédure de mesure Dell 3510 / MacBook M1" -m "Co-Authored-By: Claude Opus 5.5 (1M context) <noreply@anthropic.com>"
```

---

## Clôture — passages au banc (orchestrateur)

Pas une tâche de la table : l'orchestrateur joue ces passages à la clôture, après la double relecture, sous le
jeton `ecran`, dans le worktree du chantier, en annonçant d'une ligne à Jérôme que la fenêtre couvre l'écran
principal 30 à 60 s par rendu (décision 30, sans attendre de réponse). Le verdict est un résultat, pas une porte
(décision 38).

1. **S1 + S2** (plein écran, D3D11 puis logiciel, captures) :
   `pwsh -NoProfile -File ./scripts/bench.ps1 -Captures .tmp/s1 > .tmp/s1.txt 2>&1 ; Select-String -Path .tmp/s1.txt -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE' | Select-Object -Last 1`
   Contrôler dans `bench/results/` : JSON `…-direct3d11.json` (`graphics_api = direct3d11`,
   `valid_for_measurement = true`, `meta.compiler_flags` contient `/Ob2`, trois scénarios à plus de 100
   intervalles actifs chacun, latence en deux composantes, mémoire processus et graphique, `content_check.ok`) et
   `…-software.json` (`graphics_api = software`, `informative = true`, `content_check.ok`) ; huit PNG dans
   `.tmp/s1` (`direct3d11-…`, `software-…`), décrits en 3 lignes dans l'état.
2. **S3** (sans cache) : `pwsh -NoProfile -File ./scripts/bench.ps1 -SansCache -SansConstruire > .tmp/s3.txt 2>&1`
   → `cache = false` dans les deux nouveaux JSON.
3. **Résumé commité** : `pwsh -NoProfile -File ./scripts/resume-banc.ps1 -Entrees <les 4 JSON des étapes 1 et 2> -Sortie docs/mesures/2026-10-13-prototype-mesure.md`,
   puis ajouter en tête du fichier un paragraphe : machine de développement (pas une machine de référence),
   date, commit, verdicts, et pour tout `BUDGET DEPASSE` le rapport chiffré (critère, valeur, seuil, scénario)
   et l'hypothèse N1 proposée à Jérôme. Vérifier l'absence de nom d'hôte
   (`Select-String -Path docs/mesures/2026-10-13-prototype-mesure.md -Pattern $env:COMPUTERNAME`). Commit.
4. **S5** (fenêtre, sans souris) :
   `build/release/actcraft-bench.exe --scenario court --rendu d3d11 --taille 1280x800 --sortie .tmp/s5/ --captures .tmp/s5`
   puis la même commande avec `--rendu logiciel` : `content_check.ok` dans les deux JSON ; captures décrites.
   Proposer à Jérôme l'essai à la souris réelle (`build/release/actcraft.exe`) avec les captures, sans bloquer
   le merge.
5. **S4** : `pwsh -NoProfile -File ./scripts/test.ps1 -Preset release` puis `pwsh -NoProfile -File ./scripts/test.ps1`
   (suite complète, fumée incluse) ; CI de la PR verte, et lire dans le journal du pas macOS informatif la ligne
   `graphics_api = …` (si ce n'est pas `metal` : N1-8 s'applique, à écrire dans le rapport).
6. `graphify update .` puis commit de `graphify-out/` (règle du dépôt).

Point d'attention CI : `bench/src/gpumemory_mac.mm`, l'empreinte du générateur sous Clang arm64 et le pas macOS ne
sont vérifiés qu'en CI. Une PR en brouillon ouverte tôt (après T18) les éprouve avant la clôture ; c'est à
l'orchestrateur d'en décider.

## Relecture du plan par son auteur

**Couverture du cadrage.** Décisions 1-38 : 1 (périmètre ; étape 2 hors chantier, procédure en T20) ; 2-3 (T3) ;
4 (T5) ; 5-6 (T4, T10, DA2 vérifiée en T2) ; 7 (T11 politique, T12 `ShaderEffectSource`, T2 vérifie la couche
logicielle) ; 8 (T12 application, T16 banc) ; 9 (T9, T10) ; 10 (T6, T8, T12) ; 11 (T7 `IdentityPredictor`) ;
12 (T7, T8) ; 13 (T2, variantes de T12) ; 14-17 (T11 ; banc : T16 `setLevel`) ; 18-19 (T12, interdits) ; 20-21
(T16) ; 22 (T15) ; 23 (T15 injecteur, T16 exclusivité) ; 24-25 (T13, T16) ; 26 (T14, T16) ; 27 (T14) ; 28 (T1
`/Ob2`, T14 options, T16 validité) ; 29 (T16, T19) ; 30 (T16 placement, clôture) ; 31 (T17) ; 32 (tests de chaque
tâche) ; 33 (T18) ; 34 (T18 CI, T19 `bench.sh`) ; 35 (T1, T19) ; 36 (T19 paquet, T20 procédure) ; 37 (clôture) ;
38 (T1 profil, clôture). DA1-DA6 : T16/T12, T4/T10, T12, T3/T9/T16, T9/T10, T15. N1-1 et N1-7 : seuils figés en
T16 ; N2-1 à N2-14 : valeurs reprises telles quelles (N2-3 en T9, N2-4 en T11, N2-11 en T11, N2-13 en T1).

**Écarts au cadrage (jugés acceptables à la relecture du plan, tour 1).**
- (a) Découpage de `tests/CMakeLists.txt` en `tests/<module>.cmake` (T1) pour le parallélisme ; chaque test
  reste déclaré par `actcraft_add_test`.
- (b) `--sortie` accepte un dossier (valeur sans `.json`), pour que les scripts écrivent dans `bench/results/`
  sous le nom calculé par le banc (T16).
- (c) `-HorsEcran` (`--hors-ecran`) et `-SansConstruire` (`--sans-construire`) dans les scripts (T19), pour les
  preuves de tâche sans fenêtre et pour enchaîner les passages.
- (d) `scripts/resume-banc.ps1` produit le résumé de S3 (T20).
- (e) **Écart à la décision 6 et à DA2** (« deux chemins par moteur ») : le moteur image n'a qu'un chemin,
  `QSGImageNode`, que Qt fournit pour le RHI et pour le rendu logiciel (`QSGSoftwareImageNode`) ; les moteurs
  forme et trait, et l'encre, gardent leurs deux chemins (T4, T5, T9, T10).

**Noms.** Vérifiés d'une tâche à l'autre : `PageView` (`setZoom`, `setPan`, `cacheReady`, `requestCacheRefresh`,
`notifyCacheUpdated`, `pageToScene`, `setExclusiveDevices`, `setInkProbe`, `resetInputSequence`,
`strokeFrozen`), `FixedPageItem` (`loadGeneratedPage`, `appendStroke`, `isReady`, `objectCount`), `InkItem`
(`beginStroke`, `addPoint`, `clear`, `setProbe`, `pointCount`, `blockCount`), `InkProbe`
(`sampleReceived`, `samplesSynced`), `Injector` (`stylus`, `wheel`, `start`, `tick`, `finished`, `plannedNs`,
`postedNs`, `inputs`, `planFinished`), `classifyIntervals`, `computeLatency`, `applyVerdict`, `exitCodeFor`,
`verdictLine`, `summaryLines`, `resultFileName`, `writeResult`, `contentCheckOk`, `kDemoMidIndex`,
`demoStroke`, `demoInkProbePoint`, objets QML `pageView`, `page`, `fixedPage`, `pageCache`, `ink`.
