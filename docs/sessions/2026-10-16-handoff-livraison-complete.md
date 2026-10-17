# Handoff — Reprise sur livraison-complete (lot 2b) en NOUVELLE session

## MODE DE TRAVAIL (source : `.claude/chantier.md` de la branche du chantier, qui fait autorité)

- **Premier geste : « reprends le chantier livraison-complete »**, dans une session ouverte sur
  `C:\Users\JeromeDenis\ActCraft`, branche `main` (skill `nouvelle-feature`, entrée « reprise » de
  `references/etage-3-execution.md`). Le fichier d'état fait autorité :
  `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete\docs\sessions\livraison-complete-etat.md`.
- **Autonomie totale jusqu'à la livraison finale** (Jérôme) : aucune question avant les portes de fin de lot,
  sauf blocage matériel. Mesures Dell 3510 / MacBook M1 : pas une porte (coffre `02_Decisions/0087`).
- **Orchestrateur** : ne code pas, délègue ; rejoue chaque preuve sous sa forme bornée (un vert rapporté n'est
  jamais cru) ; merge `--no-ff` dans `feat/livraison-complete` ; seul rédacteur du fichier d'état. Une tâche = un
  worktree `.worktrees/livraison-complete--L2-T<n>`. Parallélisme 2, **1 si mémoire libre < 2 Go**.
- **Tâches `concepteur-critique`** : relecture adversariale (`reviewer-adversarial`) du diff avant merge, lancée
  APRÈS mon rejeu, jamais pendant (le relecteur mute le code). Corrections renvoyées au même agent.
- **Fin de lot** (référence changée le 2026-10-16) : double relecture, puis une tâche « simplification du lot »
  (`implementeur-tdd`, diff des tests vide, un seul essai), puis exercer les scénarios à l'écran.
- **Commandes** : toujours `pwsh -NoProfile` (jamais `powershell.exe`) ; jamais `Set-Location` dans la session
  (`git -C`, `pwsh -NoProfile -Command "Set-Location '<wt>'; …"`) ; suites sous le jeton
  `~/.claude/scripts/jeton.ps1 -Nom tests` ; `ctest` / `tst_*.exe` seulement après `. ./scripts/dev-env.ps1` ;
  `tst_*.exe` à la main n'affichent rien → `-o fichier,txt` ; captures à la main → `QT_QPA_FONTDIR=$env:WINDIR\Fonts`.
- **Règles communes des tâches** à recréer dans le scratchpad de la nouvelle session : plan 2a l. 173-231,
  plan 2b l. 168-192, et la section « Pièges trouvés » de l'état. Contrôle des noms obligatoire avant chaque
  merge : `python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" <wt>` → `NOMS OK`
  (ne jamais citer un nom d'Inspire en toutes lettres, même dans l'état).
- **Dépôt public** : rien de Promethean ; `samples/` jamais modifié. **Sécurité** : aucun HTML de document rendu
  hors du `QTextDocument` neutralisé du peintre. **Tests** : rien créé dans le profil de l'utilisateur.
- **Banc** : le banc logiciel hors écran a deux modes d'un passage à l'autre, et la machine est partagée avec
  d'autres sessions de Jérôme (`chiffrement-catalog`, `ia-entreprise`, qui prennent aussi le jeton) → ne rien
  conclure d'un passage ; fluidité jugée en D3D11 à l'écran, machine calme, à la clôture du lot.
- **Releases** : tag `v…` et `gh release` = action finale, jamais sans go écrit de Jérôme.

**État au 2026-10-16, 17 h** : lot 2a terminé ; lot 2b : L2-T11 à L2-T18 mergées ; L2-T23 et le correctif L2-C5
interrompus à la fermeture de la session, leur travail commité en WIP non vérifié.

## ✅ Fait cette session
- Lot 2a : L2-T2, T5, T6, T9, T10 et correctifs L2-C1 à C4 ; point d'étape vert (release et debug 56/56, oracle
  réel et synthétique 0 écart). Détail : état, sections « Tâches » et « Point d'étape 2a ».
- Lot 2b : L2-T11 à L2-T18 mergées, chacune rejouée et relue (faille de sécurité HTML fermée en L2-T11 ;
  interface DA11 enrichie en L2-T14 ; perte de données silencieuse corrigée en L2-T17).
- Outils de l'ancien dépôt : `tools/controle_noms_actcraft.py`, `tools/oracle_flipchart.py`.

## ⏭️ Prochain
1. **L2-C5** (`.worktrees/livraison-complete--L2-C5`, WIP `7a7c323`) : finir le correctif de `tst_ui_files`
   (tâches du pool qui survivent à un test en échec). Le WIP n'est PAS à jeter : relancer un `implementeur-tdd`
   dessus avec la consigne de la ligne L2-C5 de l'état.
2. **L2-T23** (`.worktrees/livraison-complete--L2-T23`, `524a862` + WIP `0bbd233`) : finir les points de revue
   restants (comparer au diff `8d28187..HEAD`), preuves, rejeu, merge.
3. **V12** : L2-T19 (`implementeur-tdd`) ‖ L2-T20 (`concepteur-critique`), puis **V13** : L2-T21 ‖ L2-T22.
4. **Clôture du lot 2** : section « Clôture du lot 2 » du plan 2b et étage 3.8. La clôture lit
  `page_rasterizations_during_zoom`, et non `page_rebuilds_during_zoom`. Elle juge aussi la latence d'encre
  mesurée par L2-T23, à 42-82 ms sous saturation mémoire, en D3D11 sur machine calme. Viennent ensuite
  `graphify update .`, la PR du lot 2 et la porte de fin, où la release 0.2.0 sera proposée.
- Fini quand (lot 2) : clôture du plan 2b faite, PR mergée, CI verte, porte de fin passée.

## 🔧 Git
- `main` = `3de027a` (PR #3), à jour sur `origin`. `feat/livraison-complete` non poussée (≈ 115 commits d'avance).
- Worktrees vivants : `livraison-complete`, `--L2-C5`, `--L2-T23`.

## 🔑 Accès
Aucune clé dans le dépôt. Dossier vide à supprimer à la main (créé par un ancien essai de test, accès refusé à
l'agent) : `C:\Users\JeromeDenis\OneDrive - VIDEO SYNERGIE\Documents\ActCraft`.

## 🧩 Pièges
Section « Pièges trouvés » de l'état, notamment : mémoire saturée (0,3 Go libres sur 15,5) qui rend les tests
10 fois plus lents ; banc hors écran à deux modes ; preuve rejouée pendant une revue ; HTML qui charge des
fichiers ; `tst_*.exe` muets ; moc et chaînes brutes. Coffre : `C:\SecondBrain\06_MOC\actcraft.md`.

## ♻️ Reliquats hors chantier
- Étape 2 du BRIEF (Dell 3510, MacBook M1) : procédure `docs/banc.md`, pas une porte.

## 🛠️ Skills utiles
`nouvelle-feature` (reprise), `finalize-session-chaine` (PR du lot 2), `transfert-session`.
