# Tests au label « materiel »

Certains tests exigent une vraie machine : ils échouent sur les machines de GitHub pour une raison matérielle, pas
pour un défaut du code. Ils portent le label ctest `materiel`.

| Test | Exigence | Mesure sur macos-latest |
|---|---|---|
| `tst_media_displayrecorder` (cas `framesAreCappedAtThirtyPerSecond`) | au moins une image encodée toutes les 60 ms | 21 à 23 images pour 1 550 ms, rendu sans RHI (run 43303503126) |

Le label porte sur tout l'exécutable : ctest ne sait pas étiqueter un seul cas.

## Où ils tournent

- CI (`ci.yml`) : joués sous Windows ; exclus du seul job macOS (`ctest -LE materiel`).
- Mac réel : `scripts/tests-materiel.sh`, sur la tête à publier. Dernière ligne attendue : `TESTS MATERIEL OK : <n>
  test(s)`.

## Porte avant chaque release

Avant de pousser le tag `v<version>`, jouer `scripts/tests-materiel.sh` sur un Mac réel, sur le commit du tag, et
garder sa sortie (description de la PR ou rapport de la release). Rouge : pas de tag.

Les seuils de ces tests ne se baissent pas, et aucun n'est désactivé (règle de Jérôme). Un nouveau test n'entre dans
ce label que pour une exigence matérielle mesurée, écrite dans le tableau ci-dessus.
