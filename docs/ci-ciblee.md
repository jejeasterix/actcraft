# CI ciblée et journaux des tests

La CI complète joue toute la suite (81 min de tests sur le PC). Pour savoir si un test rouge est réparé, on
rejoue seulement ce test, sur le même runner, avec `.github/workflows/ci-ciblee.yml`.

## Lancer

```sh
gh workflow run ci-ciblee.yml --repo jejeasterix/actcraft --ref main \
  -f tests='^(tst_bench_verdict|tst_bench_result)$' -f ref=<branche ou SHA> [-f plateforme=macos]
gh run list --repo jejeasterix/actcraft --workflow ci-ciblee.yml --limit 1        # numéro du run
gh run watch <run> --repo jejeasterix/actcraft --exit-status
gh run download <run> --repo jejeasterix/actcraft -n journaux-tests -D journaux    # si rouge
```

- `--ref main` choisit la version du workflow ; `-f ref=` choisit le code testé. Sans `-f ref`, le code de `--ref`.
- Pour une PR, `-f ref=refs/pull/<n>/merge` teste le même code que sa CI (la PR fusionnée avec `main`) ; une
  branche seule ne voit pas les changements de `main`.
- Le motif est obligatoire et non vide : vide, il lancerait toute la suite.
- Le runner est celui de la CI : le PC (`windows`, par défaut) ou le Mac mini (`macos`). Aucun exécuteur GitHub.
- Le runner fait un job à la fois : un run ciblé attend la fin d'une CI en cours sur le même runner.
- Compilation incrémentale : l'arbre `build/` du PC et le ccache du Mac sont ceux de la CI.

## Contrat (outillage `ci-ciblee.cjs`)

| Élément | Valeur |
|---|---|
| Fichier | `.github/workflows/ci-ciblee.yml` (`workflow_dispatch`) |
| Entrée `tests` | motif `ctest -R`, obligatoire |
| Entrée `plateforme` | `windows` (défaut) ou `macos` |
| Entrée `ref` | branche ou SHA ; vide = la ref du lancement |
| Artefact | `journaux-tests`, toujours téléversé si le job échoue ou est annulé, gardé 14 jours |

Un motif qui ne trouve aucun test fait échouer le run (`noTestsAction: error` du preset).

## Contenu de l'artefact

Écrit par le workflow, `ctest` et `scripts/ci/journaux-tests.cmake` (pris dans le commit du workflow) :

- `contexte.txt` : motif, ref, runner, run ; seul fichier si l'échec précède les tests (garde, compilation) ;
- `ctest-sortie.txt`, `ctest-junit.xml` : sortie de ctest et résultat par test ;
- `LastTest.log`, `LastTestsFailed.log` : copiés de `build/debug/Testing/Temporary` ;
- `<test>.txt` : sortie QtTest `-v2` du test rouge relancé en direct ;
- `<test>-console.txt` : sa console (messages de Qt, plantage), code de sortie en dernière ligne.

Relances en direct : 5 tests et 4 min au plus (2 min dans la CI complète), partagées entre les tests rouges.
L'étape des tests est coupée à 25 min ; ses journaux sont quand même téléversés.

## CI complète

Le job `build` de `ci.yml` téléverse le même contenu en cas d'échec, sous `journaux-tests-windows` et
`journaux-tests-macos` (deux entrées de matrice dans un même run). Les deux d'un coup :
`gh run download <run> -p 'journaux-tests*'`.
