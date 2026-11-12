# Lot 11 : restes pour le PC, joués sur `origin/main` (ordre OA12)

Joué du 2026-11-11 au 2026-11-12 sur le PC Windows 11 (Dell Latitude 5450, écran principal 1920 × 1080), branche
`verif/lot11-pc` partie d'`origin/main` 37c96c36 (PR #23 fusionnée). La file de CI du PC (runs d'O98 et d'OA27) n'a
presque pas désempli pendant l'ordre : les suites et l'APK ont tourné en priorité basse à côté des jobs ; rien n'a été
joué à l'écran pendant un job, sauf la fin des captures D3D11 (arrêtées dès qu'un job a pris le PC, voir plus bas).

## Verdict

<!-- VERDICT -->

## Windows

### Suite debug (`ctest --preset debug`, liste des noms fournie)

- 337 tests : 333 verts, 3 sautés attendus (`tst_formats_oracle`, `tst_formats_oracle_synth`,
  `tst_flipchart_miniatures_samples` : variables du poste absentes), 1 rouge hors périmètre (ci-dessous). 0 avertissement
  (`/W4 /WX`, MSVC 17.14). `format.ps1 -Check` : 0.
- Jouée en quatre tranches (`-I 130,337`, `-I 266,337`, `-I 320,337`) : la commande de fond de l'outil s'arrête à 2 h.
- `ACTCRAFT_REFERENCE_NAMES` = `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot11-noms.txt` (72 noms, hors
  dépôt) : `namesAreOurs` et `textsAreOurs` jouent, ne sautent plus.
- **Rouge hors périmètre** : `tst_media_soundrecorder`, lignes `micro` (vrai micro du PC) : `704 ms enregistrées pour
  3 s`, `1002 ms enregistrées pour 2 s hors pause` ; reproduit seul, deux fois. Les lignes sans micro passent ; la
  CI (compte de service, sans micro) ne joue pas ces lignes. Même cause pour le scénario de captures `enregistreurs`
  (`appRecorders.lastDurationOk=false`). Non corrigé ici : hors des restes listés, cause non élucidée (débit du
  périphérique d'entrée ?). À ouvrir en tâche.

### L11-T17-PC : `NOMS OK`, titres du corpus, `textsAreOurs`

- `NOMS OK : 1315 fichiers, 0 nom composé d'Inspire`, après deux correctifs : une aide de
  `tst_ui_secondpalette` (renommée `strokeColorOf`) et un nom d'objet cité dans l'état du chantier (reformulé).
- Contrôle des titres du corpus : 8 coïncidences avant correctif. Deux vraies : « Nouveau paperboard » et « Outil
  formes », libellés périmés du contexte `actions-objet` de `ui/src/commandlabels.cpp` (jamais affichés :
  `object-actions.json` dit « Créer un paperboard », « Paperboard à la taille de l'écran », « Formes »). Test rouge
  `objectActionLabelsMatchTheirContext` (`absents : Créer un paperboard | Formes | Paperboard à la taille de
  l'écran`), puis correctif. Restent deux faux positifs, dans des commentaires de code (« porteur d'action masqué »,
  `tests/tst_core_hittest.cpp:50`, `ui/src/actionrunner.cpp:283`) : sous-chaîne d'une phrase française, pas un titre.
- `textsAreOurs` avec la vraie liste : rouge sur « Origine XY » (nom de l'outil de l'original, repris par la commande
  `math.xy-origin` du lot 7). Libellé visible renommé « Repère XY » (commande, traduction, aide) ; identifiant
  inchangé. Vert ensuite.

### `recognitioncontroller.cpp:230` (point noté dans l'état)

- Confirmé : la conversion a posteriori (`recognition.convert-to-shape`, `convert-to-text`) lisait la sélection brute
  du premier utilisateur, objets tenus par l'autre compris. Test rouge `heldObjectIsNotRecognized`
  (`tst_ui_pageinput`) : `Actual (kindShown()) : 0` (forme), `Expected : 2` (trait). Correctif : `usableSelection()`,
  comme les panneaux de L11-C10 (G). Vert ensuite (36/36).

### L11-T23-PC : raccourcis F-950 à F-953

Relevé de l'ancien dépôt (`docs/spec/fonctionnalites.md:862-868`, nos mots) contre `actions/data/commands.json` :

- Ajoutés, pour des commandes qui existent : F4 mode à deux utilisateurs, F9 propriétés, F10 navigateur d'actions,
  Ctrl+J profils et réglages, Ctrl+Maj+B arrière-plan, Ctrl+Maj+D capture de l'écran entier, Ctrl+Maj+F premier plan,
  Ctrl+Maj+N notes de la page, Ctrl+Maj+V avancer d'un cran, Ctrl+Maj+W reculer d'un cran. Table de
  `tst_ui_shortcuts` étendue (rouge vu : 11 raccourcis absents), page `raccourcis.html` complétée.
- Non repris : F3 (vote, exclu du cadrage), F8 et Ctrl+U (aucune commande propre), Ctrl+Maj+Q (pas de question de
  page) ; Ctrl+Q et Ctrl+Maj+S gardent quitter et enregistrer sous (usage du système) ; F12 reste libre (le site du
  projet n'a pas de raccourci, `tst_actions_layout`, lot 6). F-953 : déjà couvert (Échap, PgPréc, PgSuiv, Suppr ;
  Maj+clic et Ctrl+clic sont des gestes de souris).

### L11-T25-PC : notices et licences

- `THIRD_PARTY_NOTICES.md` : les 35 lignes « à relever au PC » et l'entrée d'AGG relevées ; `grep -c "à relever"` = 0
  (la garde de `release.yml` les compte). Sources : SBOM SPDX de Qt 6.12.0 installé
  (`C:\Qt\6.12.0\msvc2022_64\sbom\*.spdx.json`) ; pour Pixman, DejaVu et le style Android, `qt_attribution.json` de
  `qt/qtbase` v6.12.0 ; pour Qt PDF, fichiers de licence de `qtwebengine-chromium` au commit déclaré par le SBOM de
  Qt PDF (le SBOM de Qt PDF ne déclare aucun composant tiers : la liste livrée reste « à confirmer » dans la notice).
- AGG 2.3 : « (C) 2002-2005 Maxim Shemanarev (McSeem) », licence propre à AGG (pas du MIT, sans identifiant SPDX) ;
  texte reproduit dans la notice, pas de fichier sous `licenses/` (`tst_app_notices` exige empreinte et ressource
  pour chaque fichier : à décider).
- Abseil : son `LICENSE` ne porte pas de copyright ; noté « The Abseil Authors » d'après les en-têtes, dit dans la ligne.
- FFmpeg (Windows) : 9.0.1 (`avcodec-63`, `avformat-63`, `avutil-61`, `swresample-7`, `swscale-10`), licence compilée
  « LGPL version 2.1 or later », ligne `configure` consignée dans la notice. Android : non relevé.

### À l'écran, Direct3D 11

- Captures (`captures.ps1 -Rendu d3d11 -SansConstruire`, `ACTCRAFT_UPDATES_URL` sur `tests/data/updates/newer.json`) :
  **66 scénarios sur 68 `CAPTURES OK`**, dont S8 (`edition-export`) et S17 (`imprimer`). La passe `tous` a été arrêtée
  après 56 scénarios, quand un job de CI a pris le PC ; les 12 restants et les écarts ont été joués ensuite, un par un,
  PC sans job. `marqueur` : écart au premier passage (`documentController.shownObjectCount=1`, lu 0), vert rejoué.
  En écart reproduit : `enregistreurs` (micro du PC, ci-dessus) et `exporter` (`ATTENTE appTabs.count=2 ECHEC (lu 1
  après 15000 ms)` après `file.import-pdf` du PDF exporté ; 2 fois sur 2 à l'écran, vert hors écran dans la suite :
  non élucidé).
- Scénarios `gestes` (agent-desktop, `tests/gestes/`), binaire debug du worktree (origin/main + correctifs de cette
  branche, sans changement de l'impression ni de l'édition) :
  - `actcraft-imprimer-contraste` : **réussi**, 5 vérifications vraies (document neuf, titre « rendu : Direct3D 11 »,
    boîte ouverte, libellé « Imprimante », fermée par Échap). Captures `boite-impression.png`, `avant-quitter.png`
    (`%LOCALAPPDATA%\claude-gestes\2026-11-11T21-32-39-actcraft-imprimer-contraste\`).
  - `actcraft-edition-export` : **réussi**, 4 vérifications vraies (menu d'édition : Copier, Coller). Captures
    `menu-edition.png`, `avant-quitter.png` (`…\2026-11-11T21-33-56-actcraft-edition-export\`).
  - Premier passage en `erreur` : le binaire de build ne trouve pas les DLL de Qt hors de `dev-env.ps1`
    (`app « actcraft.exe » introuvable`) ; rejoué depuis un shell où `dev-env.ps1` est chargé, sans autre changement.
- Contraste des libellés de `PrintSetupDialog`, mesuré sur `boite-impression.png` (luminance relative WCAG) :
  « Imprimante » 10,13:1, « Pages par feuille : lignes » 10,37:1, « De la page » (inactif) 10,42:1, « Notes » 10,09:1 ;
  texte ≈ (220, 226, 230) sur fond (43, 47, 54). Seuil AA : 4,5:1. **Tenu.**

## Android (émulateur, API 34, x86_64)

- **L11-T14-PC** : `paquet-android.ps1 -Abi x86_64 > .tmp/apk.txt 2>&1` → `LICENCES OK`, `APK OK`
  (`ActCraft-0.6.0-x86_64.apk`) ; la main revient 0 s après la dernière ligne, aucun `java` restant (A24 tenu). La
  compilation Android (JNI de `documentportal.cpp`) passe. `essai-android-portail.ps1` → **`ANDROID PORTAIL OK`**,
  après deux correctifs de l'essai :
  - l'effacement par MediaStore ne retire **pas** la permission gardée (question du plan tranchée) : l'entrée est
    cachée (`ACTCRAFT RECENT CACHE essai-20261112-081343.actcraft`), conforme à `RecentDocuments::prune` ; l'essai
    accepte cette voie et exige toujours que l'entrée ne soit plus affichée ;
  - l'accueil du lot 6 couvrait la page : le premier trait lisait la couleur de l'accueil (`lu 43 47 54`) ; fermé par
    `Close-ActCraftAccueil` (`android-commun.ps1`), le trait est lu noir (`lu 0 0 0`).
  Les éléments Qt Quick sont visibles par `uiautomator dump` (entrées récentes trouvées).
- **L11-T11-PC** : AVD `actcraft-tablette` créé (pixel_tablet, image déjà installée). `essai-android-tablette.ps1`
  **non conclu**. Quatre défauts de l'essai corrigés : accueil fermé ; rotation par `wm user-rotation lock` (le réglage
  seul ne tourne plus l'écran) ; mesure après l'animation ; arbre relu 60 s ; haut visible du clavier lu dans
  `mGivenContentInsets=[0,1676][0,0]`. Reste un écart réel : `cadre du texte sous le clavier (bas 1723 >= haut 1676)`.
  L'app relève bien la page (appui à y = 1792, cadre remonté à 1723) d'après `Qt.inputMethod.keyboardRectangle`
  (`PageArea.qml:68`, `KeyboardInset`), qui commence aux touches (≈ 1724 sur la capture) ; Android déclare 48 px de
  plus. Le curseur reste visible ; la marge à garder est un choix de conception, non tranché ici.
- **L11-C3-PC** : non joué. Aucun essai ne couvre l'import d'une image ou d'un PDF par le portail ni le rendu des
  permissions (`release`) : à écrire.
- **L11-T13-PC** : `essai-android-cycle.ps1` **ne peut pas conclure** avec l'APK de `paquet-android.ps1` : Qt ignore
  `applicationArguments` hors débogage (`Qt JAVA : Not in debug mode! It is not allowed to use extra arguments in
  non-debug mode.`), donc `--journal-empreinte` n'arrive jamais et `ACTCRAFT EMPREINTE` manque. Correction à
  concevoir (option lue par l'app dans un extra d'intent, ou APK d'essai débogable). Premier passage : processus
  remplacé 5 s après son lancement (second `START` qui vide la tâche, pendant un ANR de `com.android.phone`, machine
  chargée) ; non reproduit au second. Date de modification d'une URL `content://` : non relevée.
- **L11-T15-PC** : `paquet-banc-android.ps1 -Abi x86_64` → **`APK BANC OK`** (aucune permission, pas débogable),
  après deux correctifs : banc configuré dans `build/android-x86_64-banc` avec `-DACTCRAFT_BUILD_BENCH=ON` (les
  préréglages Android le coupent : `unknown target 'actcraft-bench_make_apk'`), APK cherché sous `bench/`
  (`android-build-release-unsigned.apk`). `banc-android.ps1 -Serie emulator-5592` → `MESURE INDICATIVE (émulateur)`
  puis **`MESURE INVALIDE`** : `edition.paste` et `edition.group` refusées dans le scénario `edition`,
  `invalid-refresh-period` et `gpu-memory-unavailable` (émulateur), `content-check-failed`. Non conclu ; les deux
  commandes refusées sont à examiner avant la mesure de Jérôme sur la tablette.

## Stylet et L11-C24

- L11-C24 (suite tablette puis souris d'un vrai stylet Windows Ink sur un bouton `math.*`) : non simulé. Windows permet
  l'injection d'un stylet (`InjectSyntheticPointerInput`, `PT_PEN`), mais aucun outil du dépôt ne le fait et en écrire
  un dépasse cet ordre : **la preuve attend un vrai stylet.** Preuve MSVC `/W4 /WX` des fichiers de L11-C23 et L11-C24 :
  faite (suite debug du PC compilée sans avertissement).

## L11-T0-PC

- Relevés hors dépôt écrits : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot11-noms.txt` (72 noms) et
  `…\lot11.md`. Points relevés : la « boîte à outils double » de l'original a ses propres couleurs (F-110 dit « non
  personnalisable ») ; F-303 : rien sur le relâchement d'un objet tenu ; F-930 : ordre d'application des réglages non
  dit ; F-760 : la « licence » citée par la spécification n'est confirmée nulle part.
- Non faits : REA statique d'ActivInspire (serveurs MCP coupés par défaut), A22 (événement d'état de l'application à
  `qApp`) sous Windows et Android. N Windows sur cette branche : 337 ; N11 (début du lot) n'est pas recomptable.
- `adb shell content delete --uri` : la voie marche (`Clear-FichiersEssai` de l'essai portail, état vérifié).

## Pour O98 (`release.yml`, non touché)

- Les notices n'ont plus aucune ligne « à relever » : la garde des notices de `release.yml` passe désormais.
- Le code source exact de FFmpeg n'est toujours pas publié avec les paquets (promis par la notice). Version Windows
  relevée : FFmpeg 9.0.1, LGPL 2.1 ou ultérieure, ligne `configure` dans la notice ; Android non relevé.
- La liste des composants tiers de Qt PDF reste « à confirmer » (SBOM de Qt PDF vide).

<!-- BANC -->
