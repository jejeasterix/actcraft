# Tests intermittents sous Windows et sortie vide sous ctest (O149)

Machine : PC du runner `pc-actcraft` (Core Ultra 5 125U, 14 cœurs logiques), Qt 6.12.0, preset `debug`, 4 tests à la
fois comme la CI (`CTEST_PARALLEL_LEVEL=4`). Base : tête de la PR #22, commit `bb9027e1`.

## 1. Sortie vide sous ctest : cause établie

Qt Test envoie sa sortie standard à `OutputDebugString` quand le processus n'a pas de console
(`QPlainTestLogger::outputMessage`, `shouldLogToStderr`, qui vaut `GetConsoleWindow()` sauf variable contraire).
ctest crée ses processus sans console. Preuve de l'échec : `LastTest.log` du run 42528727502 montre `Output:` suivi de
`<end of output>` pour `tst_ui_layouteditor`.

Correctif : sous Windows, `actcraft_add_test` lance chaque test par `actcraft_test_launcher`
(`tests/lanceur_test_windows.cpp`). Le lanceur pose `QT_FORCE_STDERR_LOGGING=1`, fait écrire
`build/debug/journaux-tests/<test>.txt` et y ajoute `code de sortie : N`. Il garde le test dans un travail Windows, pour
que le délai de ctest tue aussi le test. `scripts/ci/journaux-tests.cmake` copie ces fichiers dans `par-test/`.

Preuve : un `QVERIFY2` rendu faux exprès dans `tst_core_hittest`, retiré ensuite.

- journal : `FAIL! : TstCoreHittest::topmostObjectWins() ... (ROUGE EXPRES O149)` et `code de sortie : 1 (0x00000001)` ;
- `LastTest.log` de ctest : le même message `FAIL!`.

## 2. Intermittences : ce qui est établi

La plupart ne viennent pas d'un dossier partagé entre tests. Aucune collision entre exécutables n'a été observée.
Aucun `RESOURCE_LOCK` n'est posé.

| Test | Message réel | Cause | Avant | Après |
|---|---|---|---|---|
| `tst_ui_resources` (`renameAndDelete`) | `'f.library.remove(b)' returned FALSE` | Défaut du produit : la tâche de vignette garde l'image ouverte, Windows refuse la suppression | 5 lancements sur 1 800, 6 copies en parallèle ; 1 passage sur 20 à `-j 4` | 0 sur 1 800 |
| `tst_ui_mediaitems` (`lastSoundRequestWins`) | `isPlaying(Player::First)` faux | Délai trop serré dans le test : son de 400 ms, vérifié après l'attente d'écriture puis 500 ms | 7 sur 240, 6 copies ; 0 sur 40 seul | 0 sur 480, 6 copies |
| `tst_bench_verdict` (`benchRunReadsItsTargetsAt1366x768`) | `command edition.paste refused`, `injection incomplete in zones-zoom` | Reproduit sous charge, condition exacte non isolée (§ 3) | 3 sur 36, 6 copies ; 0 sur 8 seul ; 1 passage complet sur 2 | Tests du banc en `RUN_SERIAL` : 16 sur 16 à `-j 4`, 1 030 s |

Preuve pour `tst_ui_resources` : `QFile::errorString()` relevé dans le code, retiré ensuite, 5 fois sur 5 :
« Le processus ne peut pas accéder au fichier car ce fichier est utilisé par un autre processus », avec une tâche de
vignette active. Le nouveau cas `fileEditsWaitForTheThumbnailBeingRead` est rouge sans le correctif, pour la suppression
et le renommage, puis vert.

Correctifs :

- `ResourceLibrary::rename` et `remove` rejouent l'opération après la fin de la vignette en vol, seulement si elle a échoué (`withFileReleased`) ;
- `lastSoundRequestWins` : son B de 8 s, aucune assertion retirée ;
- `tst_bench_verdict` et `tst_bench_smoke` en `RUN_SERIAL` ; délai du job `build` Windows de `ci.yml` porté à 120 min.

## 3. Ce qui reste une hypothèse

- `tst_bench_verdict` : la charge déclenche l'échec (3 sur 36), mais 0 sur 8 seul ne prouve pas son absence : à ce taux,
  8 lancements verts arrivent une fois sur deux. La condition exacte du geste refusé n'est pas isolée, car le diagnostic
  temporaire n'a pas compilé. Candidats dans `runEditionGesture` : attente de 15 s dépassée, nombre d'annotations.
- `tst_bench_smoke` : `RUN_SERIAL` posé par analogie. Le lien avec le rouge de `tst_bench_result` (PR #21) n'est pas
  reproduit : `tst_bench_result` reste à 0 sur 360, 6 copies, sur un JSON déjà écrit.
- `tst_ui_layouteditor` (PR #22) : non reproduit, 0 sur 372 sous charge, 0 sur 42 passages à `-j 4`. Cause inconnue. Le
  prochain rouge laissera son message dans `par-test/`.
- `tst_media_displayrecorder` (`pauseExcludesThePause(simulee)`) : rouge à la CI complète 42738454526, « trou de 228 ms
  dans le fichier » pour une limite de 200 ms ; relancé dans le même job, il passe. Non reproduit en local : 0 sur 72 avec
  6 copies, 0 sur 12 seul, 0 sur 100 avec le processeur chargé. Hypothèse : retard d'images sous la charge des
  `tst_captures_*` voisins. Aucun correctif posé sans preuve.
- La recherche par mots-clés lit aussi des fragments dans un autre fil. Le même refus de Windows y est possible, sans
  reproduction.

## 4. Passages

- 47 tests `tst_ui_*` (les moins de 30 s, plus `tst_ui_resources`, `tst_ui_layouteditor`, `tst_ui_mediaitems`) : 20 passages
  verts sur 20 à `-j 4` (68 à 95 s), 1 passage vert en série (254 s).
- Suite complète locale à `-j 4`, avant les derniers correctifs : 304 sur 305 à chaque passage. Rouges :
  `tst_ui_mediaitems` (5 519 s), puis `tst_bench_verdict` (4 160 s). Tous deux traités ci-dessus.

- CI ciblée 42736461140 : 7 sur 7, dont `tst_ui_layouteditor`, `tst_ui_resources`, `tst_ui_mediaitems`, `tst_bench_result`.
- CI complète 42738454526 : 304 sur 305 sous Windows, Mac, Android, paquet Windows et format verts. Seul rouge :
  `tst_media_displayrecorder` (§ 3). Son journal `par-test/` donne le message réel et le code de sortie : le correctif du
  § 1 est prouvé en CI.

Coût de `RUN_SERIAL` : 5 070,30 s contre 4 250,87 s (run 42528727502), soit 819 s de plus. Un seul run de chaque côté :
cet écart compte aussi la variation d'un run à l'autre.

## 5. Noté, non traité

Les `tst_ui_*` lents (`tst_ui_desktop` 1 409,92 s, `tst_ui_shell` 1 326,66 s, `tst_ui_mathtools` 1 250,82 s au run
42528727502). Rien ne relie cette lenteur aux causes ci-dessus.
