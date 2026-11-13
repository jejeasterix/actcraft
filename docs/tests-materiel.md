# Tests au label « materiel »

Certains tests exigent une vraie machine : ils échouent sur les machines de GitHub pour une raison matérielle, pas
pour un défaut du code. Ils portent le label ctest `materiel`.

| Test | Exigence | Mesure sur macos-latest |
|---|---|---|
| `tst_media_player` | une sortie audio qui consomme le son | position restée à 0 dans 8 cas, sortie virtuelle BlackHole comprise (runs 43303503126, 43313320173) |
| `tst_media_displayrecorder_framesAreCappedAtThirtyPerSecond` (seul cas de `tst_media_displayrecorder` déclaré à part) | au moins une image encodée toutes les 60 ms | 21 à 23 images pour 1 550 ms, rendu sans RHI (run 43303503126) |

Le label porte sur une entrée ctest entière : ctest ne sait pas étiqueter un seul cas. Pour `tst_media_player`,
l'entrée est tout l'exécutable. Pour `tst_media_displayrecorder`, le cas de la cadence a son entrée à lui ; les 13
autres cas restent dans l'entrée `tst_media_displayrecorder`, jouée par le job macOS. L'entrée
`tst_media_displayrecorder_functions` rougit si une fonction ajoutée au test n'est jouée par aucune des deux. Le Mac qui
joue le script doit avoir une sortie audio active (haut-parleur intégré du Mac mini, par exemple).

## Sortis du label (OA90, 2026-11-13)

Deux tests y sont entrés sur une hypothèse (« toucher simulé non livré sur le macOS de GitHub avec le Qt d'aqt »).
L'hypothèse était fausse : les deux rouges venaient d'attentes fixes trop courtes ou trop longues pour une machine
lente. Causes reproduites sur le Mac mini, corrigées, et les deux tests rendus au job macOS.

| Test | Cause prouvée | Correctif |
|---|---|---|
| `tst_ui_inputpath` (`syntheticReleaseIsFiltered`, partie « doigt ») | Qt Quick retient un `TouchUpdate` jusqu'à l'image suivante. Le `TouchCancel` part droit à la vue, sans rien livrer avant lui. Si aucune image ne passe pendant l'attente fixe de 50 ms, la vue n'a jamais le déplacement : 1 objet au lieu de 2. Reproduit avec une attente de 1 ms. | attente de l'état : `pointerPosition` de la vue égal au point envoyé |
| `tst_ui_keyboard` (`keyboardWindowTakesEveryPointer(doigt)`) | Le `TapHandler` d'une touche ne tape pas un appui tenu plus de 0,8 s (`longPressThreshold`). Le trait au doigt de `strokeAt` attend 50 ms 9 fois avant le relâché (8 étapes) : au moins 0,45 s, plus de 0,8 s sur une machine lente. Reproduit avec 85 ms par étape : texte vide au lieu de « z ». | `presentationharness::tapAt` : appui puis relâché, sans attente entre les deux |

## Où ils tournent

- CI (`ci.yml`) : joués sous Windows ; exclus du seul job macOS (`ctest -LE materiel`).
- Mac réel : `scripts/tests-materiel.sh`, sur la tête à publier. Dernière ligne attendue : `TESTS MATERIEL OK : <n>
  test(s)`.

## Porte avant chaque release

Avant de pousser le tag `v<version>`, jouer `scripts/tests-materiel.sh` sur un Mac réel, sur le commit du tag, et
garder sa sortie (description de la PR ou rapport de la release). Rouge : pas de tag.

Les seuils de ces tests ne se baissent pas, et aucun n'est désactivé (règle de Jérôme). Un nouveau test n'entre dans
ce label que pour une exigence matérielle mesurée, écrite dans le tableau ci-dessus.
