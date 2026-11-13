# Tests au label « materiel »

Certains tests exigent une vraie machine : ils échouent sur les machines de GitHub pour une raison matérielle, pas
pour un défaut du code. Ils portent le label ctest `materiel`.

| Test | Exigence | Mesure sur macos-latest |
|---|---|---|
| `tst_media_player` | une sortie audio qui consomme le son | position restée à 0 dans 8 cas, sortie virtuelle BlackHole comprise (runs 43303503126, 43313320173) |
| `tst_media_displayrecorder` (cas `framesAreCappedAtThirtyPerSecond`) | au moins une image encodée toutes les 60 ms | 21 à 23 images pour 1 550 ms, rendu sans RHI (run 43303503126) |
| `tst_ui_keyboard` (cas `keyboardWindowTakesEveryPointer(doigt)`, entrée ctest de l'exécutable `tst_ui_lot7`) | le toucher simulé arrive à la fenêtre du clavier | texte vide au lieu de « z », 2 fois sur 2 (runs 43313320173, 43326876982) ; vert 3 fois sur 3 sur le Mac mini (macOS 27.2, Qt de Homebrew) |

Cause du rouge de `tst_ui_keyboard`, à l'état d'**hypothèse** (non établie) : sur le macOS des machines de GitHub,
avec le Qt officiel installé par aqt, le toucher simulé n'est pas livré à une seconde fenêtre. Reste à trouver la cause
et à dire si elle touche le Qt officiel sur un vrai Mac. Les cas « souris » et « stylet » du même test sont verts.

Le label porte sur une entrée ctest entière : ctest ne sait pas étiqueter un seul cas. Pour `tst_media_player` et
`tst_media_displayrecorder`, l'entrée est tout l'exécutable ; pour `tst_ui_keyboard`, seule la classe du clavier
de `tst_ui_lot7` (les 16 autres entrées de cet exécutable restent dans le job macOS). Le Mac qui joue le script doit avoir
une sortie audio active (haut-parleur intégré du Mac mini, par exemple).

## Où ils tournent

- CI (`ci.yml`) : joués sous Windows ; exclus du seul job macOS (`ctest -LE materiel`).
- Mac réel : `scripts/tests-materiel.sh`, sur la tête à publier. Dernière ligne attendue : `TESTS MATERIEL OK : <n>
  test(s)`.

## Porte avant chaque release

Avant de pousser le tag `v<version>`, jouer `scripts/tests-materiel.sh` sur un Mac réel, sur le commit du tag, et
garder sa sortie (description de la PR ou rapport de la release). Rouge : pas de tag.

Les seuils de ces tests ne se baissent pas, et aucun n'est désactivé (règle de Jérôme). Un nouveau test n'entre dans
ce label que pour une exigence matérielle mesurée, écrite dans le tableau ci-dessus.
