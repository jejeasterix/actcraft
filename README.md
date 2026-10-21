# ActCraft

Logiciel libre de tableau blanc interactif pour la classe, écrit en C++20 et Qt 6 Quick.

Objectifs, dans l'ordre :

1. **Fluidité** sur les portables d'école les plus modestes (puce graphique intégrée, 4 Go de mémoire) :
   60 images par seconde, latence d'encre sous 20 ms.
2. **Multiplateforme** : Windows et macOS d'abord, puis Linux, Android et iOS.
3. **Modularité** : chaque outil et chaque fonction est un module remplaçable.

ActCraft lit les paperboards `.flipchart` pour l'interopérabilité. Ce projet n'est ni affilié à Promethean ni
soutenu par Promethean.

> État : version 0.4.0 en préparation (octobre 2026). ActCraft dessine et corrige : sélection, stylo et marqueur,
> gomme, annuler et rétablir, effacer, corbeille et gestes à deux doigts, avec un tracé qui ne fige plus. Il ouvre et
> enregistre les paperboards (format natif `.actcraft`, lecture des `.flipchart`), depuis la 0.2.0. Les autres
> outils arrivent avec les versions suivantes. Détail : [`CHANGELOG.md`](CHANGELOG.md).

## Installer

- **Windows 10 ou 11** : `ActCraft-<version>-setup.exe`, sans droits d'administrateur.
- **Android 9 ou plus** : `ActCraft-<version>-arm64-v8a.apk`.

Les paquets seront publiés sur la page des [versions](https://github.com/jejeasterix/actcraft/releases) du dépôt.

## Construire (Windows)

Prérequis : Visual Studio Build Tools 2022 (C++), CMake 3.25 ou plus, Python 3.12.

```powershell
python -m pip install --user ninja clang-format "git+https://github.com/miurahr/aqtinstall"
python -m aqt install-qt windows desktop 6.12.0 win64_msvc2022_64 -m qtshadertools qtimageformats --outputdir C:\Qt
pwsh -NoProfile -File ./scripts/test.ps1      # compile, lance les tests, vérifie le format
./build/debug/actcraft.exe --rendu d3d11       # ou --rendu logiciel
```

Qt installé ailleurs : définir `ACTCRAFT_QT_DIR` (par exemple `C:\Qt\6.12.0\msvc2022_64`).

## Organisation

| Dossier | Rôle |
|---|---|
| `core/` | modèle de document, sans interface graphique |
| `formats/` | lecture et écriture des fichiers |
| `actions/` | registre des commandes nommées |
| `input/` | chaîne d'entrée du stylet |
| `render/` | rendu par type d'objet |
| `tools/`, `features/` | outils et fonctions, chacun en module |
| `ui/` | interface QML |
| `app/` | exécutable |
| `bench/` | banc de fluidité |
| `tests/` | tests |

Détails : [`docs/BRIEF.md`](docs/BRIEF.md).

## Licence

[MIT](LICENSE). Qt est utilisé sous licence LGPLv3.
