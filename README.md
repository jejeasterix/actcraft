# ActCraft

Logiciel libre de tableau blanc interactif pour la classe, écrit en C++20 et Qt 6 Quick.

Objectifs, dans l'ordre :

1. **Fluidité** sur les portables d'école les plus modestes (puce graphique intégrée, 4 Go de mémoire) :
   60 images par seconde, latence d'encre sous 20 ms.
2. **Multiplateforme** : Windows et macOS d'abord, puis Linux, Android et iOS.
3. **Modularité** : chaque outil et chaque fonction est un module remplaçable.

ActCraft lit les paperboards `.flipchart` pour l'interopérabilité. Ce projet n'est ni affilié à Promethean ni
soutenu par Promethean.

> État : version 1.0.0 (novembre 2026). ActCraft dessine et corrige : sélection, stylo et marqueur,
> gomme, annuler et rétablir, effacer, corbeille et gestes à deux doigts, avec un tracé qui ne fige plus. Depuis la
> 0.5.0, il pose des formes et du texte, gère les pages (navigateur, menu, fond) et la grille, et allonge la page. Il ouvre et
> enregistre les paperboards (format natif `.actcraft`, lecture des `.flipchart`), depuis la 0.2.0. Depuis la
> 0.6.0, il édite les objets (couper, copier, coller, grouper, aligner), relie les objets par des connecteurs, ouvre
> plusieurs paperboards en onglets, exporte en image et en PDF, importe un PDF ou une présentation, et imprime. Les
> autres outils arrivent avec les versions suivantes. Détail : [`CHANGELOG.md`](CHANGELOG.md).

## Installer

- **Windows 10 ou 11** : `ActCraft-<version>-setup.exe`, sans droits d'administrateur.
- **Android 9 ou plus** : `ActCraft-<version>-arm64-v8a.apk`.
- **macOS 27 ou plus récent** (Mac à puce Apple) : `ActCraft-<version>-macos-arm64.dmg`, non signé.

Cadre web : sur macOS, il lit H.264 et AAC. Sous Windows, il ne les lit pas (le Qt officiel n'a pas ces codecs) : les
pages qui n'offrent que H.264, comme Vimeo et Lumni, ne jouent pas, ouvrez-les dans le navigateur du système ; YouTube
joue dans le cadre.

Les paquets seront publiés sur la page des [versions](https://github.com/jejeasterix/actcraft/releases) du dépôt.
Les paquets de la prochaine version ne sont pas encore publiés.

## Démarrer

### Installer sous Windows

Lancez `ActCraft-<version>-setup.exe`. L'installation se fait pour vous seul : aucun droit d'administrateur n'est
demandé. Comme le programme n'est pas signé par un éditeur connu, Windows (SmartScreen) peut afficher « Windows a
protégé votre ordinateur » : cliquez sur « Informations complémentaires », puis sur « Exécuter quand même ».

### Installer sous Android

Copiez `ActCraft-<version>-arm64-v8a.apk` sur la tablette et ouvrez-le. Android demande d'autoriser l'installation
depuis cette source (le navigateur ou le gestionnaire de fichiers qui a ouvert le fichier) : acceptez, puis
installez.

### Installer sous macOS

1. Vérifiez que votre Mac est à puce Apple et sous **macOS 27 ou plus récent** : le paquet déclare ce minimum, et
   macOS refuse de l'ouvrir sur un système plus ancien.
2. Ouvrez `ActCraft-<version>-macos-arm64.dmg`, puis glissez ActCraft dans le dossier Applications.
3. Le paquet n'est **pas signé** : ni certificat de développeur, ni notarisation par Apple. macOS le dit au premier
   lancement : ouvrez ActCraft une fois (macOS le refuse), puis Réglages Système > Confidentialité et sécurité >
   « Ouvrir quand même ».
4. Pourquoi : macOS marque tout fichier téléchargé (attribut `com.apple.quarantine`) et refuse d'ouvrir une
   application marquée qui n'a pas été notarisée par Apple. Cette marque se retire en une ligne, dans le Terminal :

   ```sh
   xattr -dr com.apple.quarantine /Applications/ActCraft.app
   ```

   À faire seulement pour un paquet téléchargé depuis la page des versions de ce dépôt.

### Premiers pas

- **Écrire** : prenez le stylo dans la palette d'outils et écrivez sur la page, au stylet, à la souris ou au doigt.
- **Page suivante** : le navigateur de pages (ou le menu Affichage) passe d'une page à l'autre.
- **Enregistrer** : commande « Enregistrer » du menu Fichier. Le paperboard garde le format `.actcraft` ; les `.flipchart` s'ouvrent aussi.
- **Aide** : la touche F1 ouvre l'aide, avec une page par outil et des procédures pas à pas.
- **Modes du toucher** : sur une tablette, choisissez ce que fait le doigt (Gestion complète par défaut, ignorer, écrire,
  sélectionner) par Affichage > Mode du toucher ; la paume posée sur l'écran n'écrit pas. En mode tactile, les
  cibles à toucher font au moins 48 dp.
- **Deux utilisateurs** : deux personnes travaillent ensemble sur la même page, chacune avec sa palette, son outil et
  son trait. Ses réglages sont propres à chacune.
- **Réglages** : Outils > Profils et réglages…, onglet Paramètres : la fenêtre regroupe la langue, les outils, le profil de commandes et les mises à jour.
- **Reprise après interruption** : si l'application est coupée (batterie, appel, changement d'application),
  elle reprend seule le travail là où il en était, sans perte, et un bandeau l'indique (vous pouvez supprimer la copie).
- **Mises à jour** : la commande « Rechercher des mises à jour… » du menu d'aide. L'application peut aussi le faire au démarrage, si vous le
  décidez dans les réglages ; rien n'est installé sans votre accord.

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

[GNU GPL v3](LICENSE) (SPDX : `GPL-3.0-only`). Copyright (C) 2026 Jérôme Denis.
ActCraft est un logiciel libre : vous pouvez le redistribuer et le modifier selon les termes de la GNU General Public
License version 3, publiée par la Free Software Foundation. Il est fourni sans aucune garantie.
Qt est utilisé sous licence LGPLv3 ; composants tiers : [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).
