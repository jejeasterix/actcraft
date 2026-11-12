# Mesures — cadre-web-webengine (relevées le 2026-11-10, Mac mini Apple Silicon, 8 Go)

Machine très chargée par d'autres ouvriers pendant les relevés (charge 16 à 70) : les temps du banc sont pessimistes.
Code mesuré : tête de la branche `feat/cadre-web-webengine` (1c15aa4c).

## Provenance de Qt
Homebrew `qtwebengine` 6.11.2, révision 1 (installé : 6.11.2_1) ; Qt WebEngine fondé sur Chromium 140 (chaîne
`Chrome/140.0.7339.225` relevée dans `QtWebEngineCore.framework` du bundle). Voie `aqt` non essayée (H2).

## Poids (S4, E5)
| | Avant (76315dbc) | Après |
|---|---|---|
| `.dmg` | 64 435 520 octets (61,5 Mo), reconstruit ici par `scripts/paquet-mac.sh`, pas de repli H12 | 189 358 318 octets (180,6 Mo) |
| `ActCraft.app` | 146 Mo (`du -sm`) | 490 Mo (`du -sm`) |

Écart : +124,9 Mo pour le `.dmg`, +344 Mo pour le bundle.

## H.264 (H2)
`ACTCRAFT H264 w=320 h=240 t=2.362454` (second relevé : `t=2.336582`) : vidéo H.264 (x264, yuv420p, synthétique) décodée
dans le cadre, depuis l'image `.dmg` montée en lecture seule. Redistribution : décision R3, non prise.

## Sans cadre / avec un cadre (DA2, n° 36)
- Sans cadre web : 0 processus du moteur, 115 Mo résidents (somme, bundle monté ; premier relevé : 114 Mo).
- Avec un cadre : 258 Mo résidents (somme de l'application, 168 Mo, et d'un processus `QtWebEngineProcess`, 91 Mo ;
  1 processus du moteur compté au relevé). Le premier relevé donnait 112 Mo : pris trop tôt, avant le démarrage du
  moteur (aucune ligne `QtWebEngineProcess` n'était listée), il n'est pas retenu. Le second relevé laisse 14 s au
  cadre avant la mesure.

## Coût du premier cadre (n° 36, H15), `release`, rendu logiciel, hors écran
Plus grand trou de la boucle d'événements : `BOUCLE max=237 ms HORS SEUIL (seuil 120 ms, informatif)`, machine
saturée (charge 17). `SCENARIO OK`. Valeur sur Dell 3510 : Reste PC 8.

## Banc court hors écran
Verdict : DEPASSE (`BUDGET DEPASSE`), résultat et non porte. Dépassements relevés : page, gommer, soulever,
forme-page-annotee, miniatures-pauses, edition, effets-page ; la machine est partagée avec d'autres constructions,
aucun relevé comparatif avant/après n'est fait ici (non mesuré : banc de la branche sans le chantier).
JSON : `bench/results/2026-11-10_131902-Macmini9-1-software.json` (non versionné).

## Essai de l'image
`DMG OK`, `ESSAI DMG OK` (rendu par le script borné `scripts/essai-dmg.sh`, délai de lancement `ACTCRAFT_ESSAI_DELAI`,
90 s ici), `CADRE WEB OK`, `LANCEMENT OK`, aucune bibliothèque hors du bundle.

## Suite complète (`build-mac`, `ctest --timeout 300`)
334 tests, 99 % réussis, 2 en échec : `tst_ui_linklauncher` (rouge préexistant instable, `htaRefused`) et
`tst_ui_webimagefetch` (`oversizeIsRefused` : « Connection closed » au lieu du message « 25 Mo », test réseau local
sensible à la charge). Ce dernier rejoué seul : `Passed`, 100 % (0,87 s). 4 tests ignorés (Skipped) : `tst_formats_oracle`,
`tst_formats_oracle_synth`, `tst_flipchart_miniatures_samples`, `tst_captures_raccourcis_programmes`. Les 17 tests
de la carte d'impact sont tous `Passed`. Durée : 720 s.

## À ajouter à `release.yml` par O98
`qtwebengine qtwebchannel qtpositioning` lignes 194 (Windows), 289 (`aqt` Mac), 314 (`modules=` du repli Mac mini). Commentaires à relire : lignes 273, 294, 312, 339. Lignes 415-416 : rien. Voie `aqt` du paquet Mac (lignes 289, 291, 343) : frameworks pris dans `$ACTCRAFT_QT_DIR/lib`, `LICENSE.Chromium` livré depuis la copie exacte `LICENSES/LICENSE.Chromium`.
