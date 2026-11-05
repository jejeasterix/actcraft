# Lot 7 — tour 2 de relecture (2026-11-05, Mac)

Diff des correctifs : plage A `0ba3637..8f8b2c7a` (L7-T35, L7-T36), plage B `aa73fdee..5cf1c47f` (suite de L7-T36b, L7-T41, L7-T37, L7-T38, L7-T40, L7-T39). 95 fichiers, 3 zones × 2 axes, tout lu (« Non couvert » vide partout). Carnets : scratchpad de la session, `relecture-lot7-t2-{conformite,qualite}-z{1,2,3}.md`.

Verdict : **0 bloquant, 2 majeurs**, mineurs et simplifications ci-dessous.

## Majeurs
- **Ma1 (M8 pendant l'annotation du bureau)** : `ui/src/desktopcontroller.cpp:370-375`, `ui/qml/DesktopToolsWheel.qml:58-61`. Annotation ouverte, `desktop.tools`, puis « Horloge » : `desktop.restore` fait `showNormal` (`back` vaut `Windowed` pendant l'annotation), la fenêtre principale revient à la taille de la palette (`keepMainInFront`), et `clockRoot` (posé dans `presentation.layer`, `Presentation.qml:55`) tombe hors de la fenêtre ou est rogné. Idem dés, calculatrice, message défilant. `wheelToolIsVisible` ne couvre que le cas sans annotation, et `root->isVisible()` ne verrait pas un panneau rogné. → L7-T42.
- **Ma2 (borne affaiblie)** : `tests/tst_ui_ticker.cpp:505` (`loopSwitchedOffDuringAPassEndsIt`) : la borne passe à « du tiers au triple » du passage ; une coupure de boucle qui laisserait un passage de plus passe. Les bornes des l. 354 et 540 ont aussi été élargies sans horloge injectée (seul `pauseHoldsTheMessage` l'utilise). → L7-T43 : horloge injectée ou borne haute à 7/4 comme `singlePassHidesTheBand`.

## Mineurs (défauts, à corriger)
Pour L7-T42 (fichiers du bureau et de la roue) :
- `ui/src/desktopcontroller.cpp:165-178` : `opensInTheMainWindow` recopie les 4 commandes de l'anneau « Autres outils » (`features/src/desktopwheel.cpp:39-42`) ; une 5e entrée rouvrirait M8 sans test → drapeau porté par `features::WheelEntry`.
- `captureDoesNotBlockTheMainThread` remplace la source de capture : un `msleep` remis dans `grabPrimaryScreen` (lieu du défaut M4) ne serait vu par aucun test.

Pour L7-T43 :
- `bench/src/runner.cpp:1209-1218`, `1328-1335` : le ticker démarre avant cinq sorties `break` et n'est jamais arrêté (réglages `loop`, `speed`, `text` restent) ; l'effet de page a sa garde `qScopeGuard`, pas le ticker.
- `bench/src/runner.cpp:82` : `TransitionWatch::valid()` ne vérifie pas la propriété `playing` lue par son nom.
- `bench/src/runner.cpp:652` : le rang du repos relevé côté runner n'a pas de test (reporté de L7-T38).
- `bench/src/runner.cpp:747-752` : un effet de page jamais fini reste en surcouche pour le scénario suivant, aucun `settle()` après l'attente de fin (reporté de L7-T38).
- `bench/captures/main.cpp:158` : `--retard-images` sans valeur ignoré sans erreur (reporté de L7-T40).
- Test `_lent` : ne prouve que `checkEdge` ; ni `capture`, ni `checkPixel`, ni le plafond de 10 s (reporté de L7-T40).
- `ui/src/mathtoolscontroller.cpp:5` : `pagecache.h` inutile ; `features/src/curtain.cpp:6-7`, `features/src/spotlight.cpp:6-7` : `<cmath>`, `<iterator>` inutiles (reportés de L7-T39).
- `ui/qml/Presentation.qml:6` : commentaire « ce fichier ne change plus » contredit (reporté de L7-T39).
- `ui/qml/CountdownDialog.qml:7` : commentaire mal recoupé (« répétition, » seul sur sa ligne).
- M7 prouvé pour un coin sur quatre : `tests/tst_ui_clock.cpp:396` ; branches `sx < 0`, `sy < 0` de `ui/qml/ClockPanel.qml:265-266` sans test ; poignées hautes sur la croix `floatingPanelClose` et la barre (`DragHandler`).
- `ui/qml/CalculatorPanel.qml:125` : garde `enabled: calculatorController.canWrite` sans test.
- `ui/src/recognitioncontroller.cpp:543`, `.h:169` : `m_arrivedAt` garde un trait gommé jusqu'à Annuler ou changement de mode ; commentaire « tant qu'il reste un trait » faux pour la gomme ; aucun test de la gomme.

## Consignés (porte de clôture, pas de correctif)
- `OriginMemory` « spéculative » (brief L7-T37) non jugée : `refreshOrigin` l'emploie pour une seule page (`mathtoolscontroller.cpp:1035-1049`).
- `designMode` gardé sans être posé (`transitions.h:34`, justifié `transitioncontroller.h:35`, merge f132b114).
- Relance « en direct » de `preuves-windows.yml` (forme 4f0ef916) jamais vue sur un run rouge.
- `docs/banc.md` non tenu depuis le lot 4b (hérité).
- `tests/tst_ui_mathtools.cpp:1605-1607` : `qWait(50)` avant des affirmations négatives (style du fichier).
- M10 : `zoom < during` après le relâché ne discrimine rien.

## Simplifications (au rapport de clôture ; L7-T39 a eu son unique essai)
Isolement des réglages copié 3 fois (`bench/app/main.cpp:40-50`, `bench/captures/main.cpp:104-110`, `tests/tst_bench_verdict.cpp:228-240`) ; ligne de 700 caractères de la relance CI ; préambule dupliqué `tst_bench_result.cpp:279-293`/`324-338` ; `findChild("transition")` deux fois (`runner.cpp:582-588`, `1223-1229`) ; switchs `Side` → énumération locale (`curtain.cpp:52-70`, `spotlight.cpp:73-89`) ; `classTokens` public sans besoin (`equation.h:65`) ; jonction des jetons deux fois (`equation.cpp:94-101`, `512-517`) ; `static_assert` + cases du rideau (`curtaincontroller.cpp:95-113`) ; ouverture des menus copiée dans 4 tests (`tst_ui_mathtools.cpp:1395-1500`) ; `pageMenuSceneRect` copié (`tst_ui_ticker.cpp:778-785`, `tst_ui_clock.cpp:352-361`) ; `syncEnabled()` (`RecognitionAlternatives.qml:89-90`, `111-112`) ; assertions redondantes (`tst_ui_clock.cpp:324`, `377-378`, `tst_ui_curtain.cpp:70`).
