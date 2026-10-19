# Livraison complète — lot 7 « Présentation et outils » — amendement « phase 2 » (après le socle) — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

Brouillon relu trois fois (tours 1 à 3, rapports revue-phase2-t1..t3), puis confronté au socle réel le 2026-10-19 ; relecture exigée de cette confrontation avant exécution.

**Statut : confronté au socle réel.** Le socle est mergé et fusionné dans `feat/livraison-complete-b` (131f790,
depuis `feat/livraison-complete` @ e7c6713). Les 17 points « [SOCLE?] » du brouillon sont levés par lecture du code
fusionné : chacun porte « vérifié : … (fichier:ligne) » ou la correction faite (section « Vérifié contre le socle
réel », en fin de plan). Les quatre points de A (état du chantier, « Socle fusionné ») sont intégrés (décision 13).
Relecture exigée (hook `relire-plan.js`) avant toute exécution.

**Ce que c'est.** Un amendement aux plans du lot 7 : `docs/plans/2026-10-18-livraison-complete-lot7-PLAN.md` (« 7a »)
et `docs/plans/2026-10-18-livraison-complete-lot7b-PLAN.md` (« 7b »). Il **ne les réécrit pas** : pour chaque tâche
`L7-T*`, il dit ce qu'elle garde, ce qui disparaît (fait par le socle ou par la phase 1) et ce qui s'ajoute (rapport
L7-T0, amendements du socle). À l'exécution, la section d'une tâche = **sa section de 7a ou 7b, corrigée par sa
section ici** ; en cas de contradiction, ce plan l'emporte. Il ajoute une tâche, **L7-S8**, qui reprend l'ancienne
L0-S8 du socle (squelette du lot 7).

**Autorités, dans l'ordre.**
1. Le plan du socle, version de `feat/livraison-complete` (794cf84), sections « Contrats figés par le socle (noms) »,
   « Amendements à porter une fois le socle mergé », partie « Plans du lot 7 », et « Ce que la session B doit
   savoir » (copie : `C:\Users\JEROME~1\AppData\Local\Temp\claude\C--Users-JeromeDenis-pilotage\67662d88-b0d4-48e1-9ce6-e529d6829fab\scratchpad\socle-PLAN-794cf84.md`,
   citée « socle:<ligne> ») ; **à la fusion, le code mergé fait foi sur le plan**.
2. L'addendum 1 de A (`…\fd5ef06a-7b9b-46ba-a8c3-b06809fefd99\scratchpad\brief-b-addendum-1.md`) et tout addendum
   ultérieur du même dossier.
3. Le brief de B, section « Phase 2 » (`docs/sessions/livraison-complete-b-brief.md`).
4. Ce plan.
5. Le plan de la phase 1 (`docs/plans/2026-10-18-livraison-complete-lot7-phase1-PLAN.md`), section « Ce que la
   phase 2 reprend » ; L7-P0 à L7-P9 sont tenues pour **faites** telles que décrites par leur section.
6. Le rapport L7-T0 (`docs/verifications/2026-10-18-lot7-principes.md`), section « Écarts pour l'orchestrateur ».
7. Les sections `L7-T*` de 7a et 7b, pour tout ce que ce plan ne redit pas.

**Ancienne L0-S8** (texte de référence, retirée du socle par 1880a6f) : `git -C <wt> show aa621f1:docs/plans/2026-10-18-livraison-complete-socle-3-7-PLAN.md`,
section « Tâche L0-S8 — Squelette du lot 7 » (citée « S8a:<ligne> »). L7-S8 en reprend le contrat de
`DocumentInsertions` (signatures identiques, `#include <cstddef>` ajouté) et la liste des QML ; le reste est adapté (phase 1 faite, noms figés du socle).

**Branche et worktrees.** Base : `feat/livraison-complete-b` **après** la fusion de `feat/livraison-complete` (socle
mergé, conflit unique attendu de `tests/CMakeLists.txt` résolu : commentaire du socle gardé, une seule ligne
d'inclusion), suites debug `TESTS OK`, `NOMS OK` (brief, phase 2, point 1 ; fait par l'orchestrateur, hors tâches).
Une tâche = un worktree `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b--L7-<id>`, branche
`feat/livraison-complete-b--L7-<id>`, partie de `feat/livraison-complete-b` à jour des tâches mergées. Merge
`--no-ff` par l'orchestrateur, puis `graphify update .` et commit de `graphify-out/`. **Après le merge de la
deuxième tâche d'une vague**, l'orchestrateur lance sur `feat/livraison-complete-b` une construction et la suite
complète (preuve de tâche, sous le jeton `tests`, N = N_F + 29) **avant** la vague suivante : deux tâches vertes
chacune dans leur worktree peuvent ne plus lier ensemble (exécutable d'interface unique, décision 1). Rouge : la
vague suivante ne part pas ; l'écart est rendu à la tâche fautive.

**Pile.** C++20, Qt 6.12 (Core, Gui, Quick, QuickControls2, Svg, Test), CMake + Ninja, MSVC 2022, PowerShell 7.

## Décisions de cet amendement

1. **Fichiers propres à chaque fonction, pour tenir deux tâches de front.** Le profil permet deux tâches à la fois
   (`.claude/chantier.md`, « Parallélisme ») ; 7a et 7b les enchaînaient toutes parce qu'elles partageaient
   `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `tests/presentation.cmake` et `tests/tst_ui_presentation.cpp`.
   Après L7-S8, **aucune tâche ne touche plus ces quatre fichiers**, sauf L7-T2 (`tst_ui_presentation.cpp`, ses
   fonctions), L7-T17 (`Presentation.qml`, au besoin) et L7-T32 (`tst_ui_presentation.cpp`, en bout de chaîne,
   seule). Pour cela :
   - **Une racine QML par fonction**, instanciée une fois par L7-S8 dans `Presentation.qml`, qui porte le contrôleur
     de la fonction et **ses gestionnaires de commandes** (`commands.setHandler` dans son `Component.onCompleted`).
     Les sections de 7a/7b qui disent « gestionnaire dans `Presentation.qml` » se lisent « dans la racine de la
     fonction » (table de la décision 3).
   - **Un fichier de test d'interface par fonction, un seul exécutable** (décision de l'orchestrateur, revue M5 : la
     machine manque de mémoire). `tests/tst_ui_<fonction>.cpp` porte la classe de la fonction, inscrite dans un
     registre statique (`tests/support/lot7registry.h`) ; un seul exécutable `tst_ui_lot7` (son `main`,
     `tests/tst_ui_lot7_main.cpp`, écrit par L7-S8 et plus jamais touché) lie les 17 fichiers ; 17 entrées ctest
     gardent leur nom (`add_test(NAME tst_ui_curtain COMMAND tst_ui_lot7 curtain)`), le compte N ne change pas. Deux
     tâches de front n'écrivent jamais dans le même fichier ; un seul lien et un seul PDB d'interface pour le lot, au
     lieu de 17 (et de 34 avec le preset release du banc). **Règle d'isolation** (revue, tour 2, I1) : dans un
     `tst_ui_<fonction>.cpp`, tout (classe de test, aides, `struct`, constantes) est dans un `namespace { … }`
     anonyme ou `static` ; aucun nom au niveau global hors de la ligne `ACTCRAFT_LOT7_TEST` ; aucun objet Qt de portée
     fichier (son constructeur tournerait avant `QGuiApplication`). Sans cette règle, deux aides du même nom donnent
     `LNK2005`, ou pire une violation de l'ODR sans diagnostic sous MSVC. `tst_ui_presentation` ne garde que les fonctions de L7-S8,
     de L7-T2 et de L7-T32.
   - **Sources C++ du lot listées par motifs nommés** dans `ui/presentation.cmake` (`file(GLOB … CONFIGURE_DEPENDS)`
     sur les noms exacts `src/<nom>controller.h`, `src/<nom>controller.cpp`, …) : la tâche crée son contrôleur, CMake le
     voit à la construction suivante, et aucun `.cpp` talon vide n'est nécessaire (un `.cpp` sans symbole ferait dire
     `LNK4221` à `lib.exe`, constat de la phase 1, décision 5). Les QML, eux, sont listés en entier (les racines
     doivent exister dès L7-S8 pour que `Presentation.qml` se charge). **Écart au brief** (phase 2, point 2 :
     « talons des contrôleurs de `ui/src/` ») : les motifs nommés remplacent ces talons, pour la raison `LNK4221`.
   - **Tous les tests du lot déclarés par L7-S8** dans `tests/presentation.cmake`, chacun talon `reserved()` sauté :
     le compte est figé dès L7-S8 (« = », pas « ≥ »). Les chemins d'inclusion privés dont les tâches auront
     besoin (`ui/src` pour les contrôleurs et `DocumentInsertions` ; `render/src` pour le seul test du module `render`,
     `tst_render_equation`) sont posés
     dans ces déclarations, une fois pour toutes (revue B1).
2. **Talons du lot 7.** Marqueur unique `Talon L7-S8 : rempli par L7-T<n>.` (commentaire C++ ou QML, champ
   `description` d'un scénario, texte du `QSKIP` de `reserved()`), sur le modèle de la règle d'honnêteté du socle
   (socle:131-153). La tâche propriétaire supprime le marqueur et `reserved()`. Contrôle de clôture du lot :
   `git -C <wt> grep -n "Talon L7-S8 : rempli par" -- . ':(exclude)docs/'` vide, et
   `git -C <wt> grep -n "Talon L0-S : rempli par L7-" -- . ':(exclude)docs/'` vide (talons du lot 7 posés par le
   socle). Vérifié : trois lignes sur la branche fusionnée — `core/include/actcraft/core/equation.h:2` et
   `core/src/equation.cpp:2` (`… par L7-T24.`, retirées par L7-T24), `render/src/equationlayout.cpp:1`
   (`… par L7-T25.`, retirée par L7-T25) ; chaque tâche le prouve par son `git grep` vide.
3. **Racines et emplacements** (posés par L7-S8). `Presentation.qml` ne pose **que** `parent` (aucun `anchors`) :
   la racine décide de sa géométrie (revue M2). La racine de `Presentation.qml` est un **`QtObject`** (socle L0-S9,
   `ui/qml/Presentation.qml:7-12` : `layer` est une propriété FINAL de `Item`) avec `default property list<QtObject>
   data` : les racines du lot y sont déclarées et posées par `parent: presentation.layer` ou
   `parent: presentation.pageOverlays`, jamais comme enfants visuels de `presentation` (point 2 de A). Les racines
   sans parent (`PerPageTools`, `OtherToolsHost`, les trois racines à fenêtre propre) sont des `Item` non visuels de
   `data`. Dans `pageOverlays` (coordonnées de page : la surcouche suit le zoom
   et le défilement), la racine écrit elle-même `anchors.fill: parent` ; dans `presentation.layer`, elle garde les
   liaisons de 7a (alignement sur `pageArea` par `mapToItem`) ou la taille propre du panneau. **Empilement** : chaque
   racine reçoit un `z` explicite dans `Presentation.qml` (revue, tour 2, mineur 9), sans dépendre de l'ordre de
   réévaluation des `parent:` — dans `pageOverlays` : rideau 10, spot 20, outils de maths 30 (les outils restent
   saisissables au-dessus du rideau) ; dans `presentation.layer` : effets 10, message défilant 20, dés 30,
   calculatrice 31, horloge 32, alternatives d'écriture 40, éditeur d'équation 41, navigateur d'effets 50.

   | Racine (`ui/qml/`) | Tâche qui la remplit | Parent (seule propriété de placement posée) | Contrôleur(s) qu'elle porte |
   |---|---|---|---|
   | `CurtainOverlay.qml` | L7-T3 | `presentation.pageOverlays` (décision de l'orchestrateur, revue M2 ; 7a:1062 le posait dans `presentation.layer`, aligné par `mapToItem`) | `CurtainController` |
   | `SpotlightOverlay.qml` | L7-T4 | `presentation.pageOverlays` (même règle) | `SpotlightController` |
   | `PageZoomOverlay.qml` | L7-T5 | `presentation.pageArea.presentationSlot` (coordonnées de la zone, socle L0-S9) | `PageZoomController` |
   | `PerPageTools.qml` | L7-T6 | aucun (`Item` non visuel de `data`) | `PageToolsController` |
   | `TransitionLayer.qml` | L7-T7 | `presentation.layer` (7a:1477) | `TransitionController` ; enveloppe `page.next` / `page.previous` |
   | `PageEffectsPicker.qml` | L7-T8 | `presentation.layer` | — (lit `transitionRoot.controller`) |
   | `MathToolsLayer.qml` | L7-T9 (puis T10 à T13) | `presentation.pageOverlays` (7a:1708) | `MathToolsController` |
   | `DicePanel.qml` | L7-T14 | `presentation.layer` | `DiceController` |
   | `CalculatorPanel.qml` | L7-T15 | `presentation.layer` | `CalculatorController` |
   | `ClockPanel.qml` | L7-T16 | `presentation.layer` | `ClockController` |
   | `TickerOverlay.qml` | L7-T18 (puis T19) | `presentation.layer` (7a:2245) | `TickerController` |
   | `RecognitionAlternatives.qml` | L7-T21 (puis T23) | `presentation.layer` | `RecognitionController` |
   | `EquationEditor.qml` | L7-T26 (puis T27) | `presentation.layer` | `EquationController` |
   | `CameraOverlay.qml` | L7-T28 | aucun : fenêtre créée à la demande (décision 4 bis) | `CameraController` |
   | `ScreenKeyboard.qml` | L7-T29 | aucun : fenêtre créée à la demande (7b, L7-T29 : ne prend jamais le focus) | `KeyboardController` |
   | `DesktopAnnotation.qml` | L7-T30 (puis T31) | aucun : fenêtre créée à la demande par le contrôleur (7b, L7-T30) | `DesktopController` |
   | `OtherToolsHost.qml` | L7-T17 | aucun (`Item` non visuel de `data`) | `PresentationCommands` |

   QML non racines (créés en talon par L7-S8, instanciés par leur racine) : `FloatingPanel.qml` (T2),
   `RulerTool.qml` (T9), `SetSquareTool.qml` (T10), `ProtractorTool.qml` (T11), `CompassTool.qml` (T12),
   `OriginMarker.qml` (T13), `CountdownDialog.qml` (T16), `TickerDialog.qml` (T19), `CameraPalette.qml` (T28),
   `DesktopToolsWheel.qml` (T31). `PerPageTools.qml` et `OtherToolsHost.qml` sont deux noms neufs (fonctions sans
   QML dans 7a) : contrôle des noms dans L7-S8. Avec `OtherToolsHost.qml`, L7-T17 ne touche plus `Presentation.qml`
   (la décision 1 garde l'exception par prudence, si le contrôleur doit y être nommé). Le nom de travail du
   premier brouillon (« hôte des outils de page ») est devenu `PerPageTools.qml` (revue, tour 1, mineur 9 : trop
   proche de `ui::ToolHost` du socle).
4. **Contrat d'une racine talon** (L7-S8) — chaque racine déclare, et seulement cela :
   ```qml
   import QtQuick
   import ActCraft.Ui

   // Talon L7-S8 : rempli par L7-T<n>.
   Item {
       id: root
       property Commands commands: null
       property DocumentController documentController: null
       property Tools tools: null
       property Item pageArea: null
       property QtObject controller: null // la tâche y lie son contrôleur : controller: curtainController
   }
   ```
   plus, pour les racines qui lisent une autre fonction : `property QtObject curtain: null` et
   `property QtObject spotlight: null` (`PerPageTools.qml`), `property QtObject curtain: null`
   (`TransitionLayer.qml`), `property QtObject transitions: null` (`PageEffectsPicker.qml`). La tâche **ne retire
   aucune** de ces propriétés (`Presentation.qml` les pose) ; elle en ajoute à sa guise.

   **4 bis. Fonctions à fenêtre propre** (caméra, clavier à l'écran, annotation de bureau ; revue M1). Une `Window`
   n'a ni `parent` ni `anchors` : leur racine reste un `Item` **non visuel** (sans `parent` ni `anchors` dans
   `Presentation.qml`, `visible: false`, taille nulle) qui porte le contrôleur et un `Component { Window { … } }`
   créé à la demande (`createObject(null)`, la fenêtre gardée dans une propriété de la racine, `property Window
   window: null`, sinon le ramasse-miettes de JavaScript peut la détruire ouverte ; détruite à la fermeture ; pour le bureau, le contrôleur crée la fenêtre par `beginCreate`, pose
   son format transparent sur la `QQuickWindow`, puis `completeCreate`, 7b L7-T30). Les étapes de 7b qui disent
   « `CameraOverlay.qml` est une fenêtre » se lisent « la fenêtre est le `Component` de la racine
   `CameraOverlay.qml` » (de même pour `ScreenKeyboard.qml` et `DesktopAnnotation.qml`).
5. **Couvertures (B2, socle « Changent » 5).** Toute surcouche ou tout panneau qui prend l'entrée appelle
   `pageArea.view.addInputCover(<item>)` à son ouverture et `removeInputCover` à sa fermeture (vérifié : `Q_INVOKABLE void
   addInputCover(QQuickItem* cover)` et `removeInputCover`, `ui/include/actcraft/ui/pageview.h:181-182` ; `view`
   alias de `PageArea`, `ui/qml/PageArea.qml:10` ; `coveredAt` respecte visibilité, activité et `contains`,
   `ui/src/pageview.cpp:724-727`). `FloatingPanel.qml` le fait pour tous les
   panneaux (L7-T2). Chaque tâche a son test `<fonction>BlocksTheStylus` **à données**, trois lignes `stylet`
   (`QTabletEvent`), `doigt` (`QTouchEvent`), `souris` (`QMouseEvent`), injectées sous la surcouche par les aides de
   `tests/support/presentationharness.h` (L7-S8) ; attendu : nombre d'objets de la page inchangé, encre vide,
   **et** l'effet du gestionnaire de la surcouche (bord tiré, outil déplacé, panneau déplacé, bouton pressé), lu par
   `objectName`. Un appui couvert du stylet laisse les doigts libres (socle D-S1) : vérifié une fois, dans
   `coveredStylusLeavesFingersFree` (L7-T2). Chaque test de couverture a son pendant « surcouche fermée ou hors de
   sa forme : le même trait est versé » (sinon la moitié « aucun trait » ne peut pas échouer).
   **Fenêtres propres** (décision 4 bis) : la couverture de la vue n'a pas de sens ; le test est
   `<fonction>WindowTakesEveryPointer` à données (`stylet`, `doigt`, `souris`) : l'événement est envoyé à la
   fenêtre de la fonction, **son** effet est lu (rectangle de capture agrandi, touche reçue par le champ, trait versé
   dans le paperboard du bureau) et la page de la fenêtre principale reste inchangée. Il échoue si la fenêtre ignore
   un des trois pointeurs (mutation : retirer le gestionnaire du stylet → rouge sur la ligne `stylet`).
6. **Ce qui sort de toutes les colonnes « fichiers touchés »** : `ui/CMakeLists.txt`, `ui/icons/*` (dessinées par
   L0-S1 ; une tâche qui redessine **son** icône l'ajoute à sa ligne avant lancement, socle « Changent » 4),
   `features/CMakeLists.txt` (sources par motif, bibliothèques posées : phase 1, décision 5), `ui/qml/Presentation.qml`
   (sauf L7-S8, L7-T17 au besoin), `ui/presentation.cmake`, `tests/presentation.cmake`,
   `tests/tst_ui_lot7_main.cpp`, `tests/support/lot7registry.h`, `tests/support/presentationharness.h` (L7-S8
   seulement), `tests/tst_ui_presentation.cpp` (L7-S8, L7-T2, L7-T32), `features/…` et `tests/tst_features_*` déjà
   faits en phase 1 (sauf ajout nommé dans une section).
7. **Interdits de B posés par le socle** (socle:1478-1482) : `core/`, `formats/`, `render/`, `actions/data/*`,
   `ui/icons/*`, `ui/CMakeLists.txt`, `tests/{core,input,tools,render,ui,bench}.cmake`, `Main.qml`, `PageArea.qml`,
   `ToolPalette.qml`, `pageview.*`, `documentcontroller.*` ; ne jamais recréer `PagePresentation`, `CurtainMode`,
   `SpotlightShape`, `PageToolsMode`, `Equation`, `ReplaceObjectsCommand`, `SetPagePresentationCommand`.
   **Exceptions nommées**, et seulement elles : `formats/src/flipchart.cpp`, `formats/src/flipchartids.h`,
   `tests/support/flipchartwriter.h`, `tests/tst_formats_flipchart.cpp` (L7-T1 réduite, socle « Disparaissent » 2 ;
   L7-T6) ; `core/include/actcraft/core/equation.h` et `core/src/equation.cpp` pour l'**arbre** d'équation seulement
   (L7-T24 ; `struct Equation` du modèle inchangée) ; `render/src/equationlayout.h`, `render/src/equationlayout.cpp`,
   le nouvel en-tête public `render/include/actcraft/render/equationpaint.h`, la seule ligne de cet en-tête dans
   `render/CMakeLists.txt`, et `tests/tst_render_painter.cpp` (L7-T25, décision de A : un module ne voit d'un autre
   que `<module>/include/actcraft/<module>/`). Exceptions d'**ajout seulement** pour L7-T2 (décision de
   l'orchestrateur, revue contre le socle, B1 et M2 ; signalées à A avant L7-T2) : `ui/include/actcraft/ui/documentcontroller.h`,
   `ui/src/documentcontroller.cpp` (signal `documentReplaced`), `core/include/actcraft/core/document.h`,
   `core/src/document.cpp` (`removeResource`) ; aucune ligne existante modifiée. L0-S3 est mergée (be2e5de) : `render/src/equationlayout.h` y déclare
   `paintEquation(QPainter&, const core::Equation&)` (ligne 10, privée) ; le reste est à relire à la fusion.
8. **Commandes.** Les commandes du lot sont dans `actions/data/commands.json` (L0-S4) ; B n'en ajoute aucune. Les
   actions neuves de L7-T0 sans commande au socle (mémoriser le rideau pour la page, P9 ; capturer la zone du spot,
   P8 ; poser le calcul sur la page, P3) sont des entrées du **menu propre** de la surcouche ou des boutons du
   panneau ; leur effet passe par une commande d'annulation nommée de `core` (`SetPagePresentationCommand`,
   `AddObjectCommand`) via `DocumentInsertions`, une étape. Vérifié : `actions/data/commands.json` (85 commandes)
   n'a aucune commande pour P8, P9 ni P3 ; les identifiants du lot cités par ce plan y sont tous (dont
   `view.page-effects`, `page.tools-mode`, `page.curtain-mode`, `page.curtain-distance`, `page.spotlight-mode`,
   `present.curtain-color`, `desktop.restore`).
9. **Valeurs de L7-T0 déjà dans la logique** (phase 1, décision 9) : rien à refaire côté `features/` ; les tâches
   d'interface les **lisent** par leurs constantes nommées (`Spotlight::kDefaultOutline`, `kDefaultToolOpacity`,
   `RecognitionSession::kDefaultDelayMs`, catalogue de `transitions.h`, `Dice` à 1 dé) et ne les recopient jamais.
10. **L7-T20 et L7-T22 sont faites** (L7-P7, L7-P8) : elles n'ont plus de ligne. Leurs dépendants (L7-T21, L7-T23)
    dépendent de L7-S8.
11. **Hypothèses ajoutées** (rapport L7-T0, E9 et E11) : **H7-14** — actions de fin du compte à rebours : celles de la
    spécification (aucune, page d'après, page d'avant ; « remettre la page dans son état enregistré » en L7-T34) ;
    **H7-15** — validation du message défilant : un bouton « Essayer » et un bouton « Appliquer » qui garde le message
    comme défaut et ferme la boîte. Elles s'ajoutent à « Hypothèses à revoir » de 7b (porte de fin du lot 7).
12. **Écarts aux autorités, écrits** (revue, mineur 10) : `overlaySlotsExist` (socle « Disparaissent » 3, gardé
    pour L7-T2) est remplacé par `lot7RootsExist` (L7-S8) — les emplacements sont prouvés par `socleSlotsExist`
    (L0-S9), il reste à prouver les racines ; `presentationLoadsWithoutQmlWarning` est retiré (revue, mineur 2 :
    `shellLoadsWithoutQmlWarning` du socle charge le même `Main.qml`, racines comprises) ; talons des contrôleurs
    remplacés par des motifs (décision 1) ; le glisser du calcul vers la page (P3) va à L7-T32 (revue M6).
13. **Points de A après la fusion du socle** (état du chantier, « Socle fusionné », 2026-10-19) :
    - **Observateur et chargements** : `replaceDocument`, `loadGeneratedFirstPage`, `loadGeneratedPages`
      (`ui/include/actcraft/ui/documentcontroller.h:86-103`) ne passent pas par `applyToCache` et n'appellent pas
      l'observateur. **Jamais** déduire un remplacement de `documentChanged` : il part après **chaque**
      modification, trait compris (`ui/src/documentcontroller.cpp:311-316`, `notifyState` ; `:202-206`,
      `commitStroke` → `executeAs`) — revue contre le socle, tour 1, B1. Décision de l'orchestrateur : **L7-T2 ajoute à
      `DocumentController` un signal dédié `documentReplaced()`**, émis seulement par `replaceDocument`,
      `loadGeneratedFirstPage` et `loadGeneratedPages`. Les contrôleurs qui relisent leur état au chargement
      (outils de page L7-T6, origine L7-T13, session de reconnaissance L7-T21) écoutent `documentReplaced`, et
      chacun a son test « un trait ne remet rien à zéro ». **Fichier du socle** : `documentcontroller.*` (interdit
      à B, décision 7) reçoit cet **ajout seulement** — une déclaration de signal et trois `emit`, aucune ligne
      existante modifiée ; l'orchestrateur le signale à A avant L7-T2 (le lot 3 peut toucher `documentcontroller.*` :
      conflit d'ajout possible à la fusion L7-T33).
    - **Racine `QtObject`** de `Presentation.qml` : décision 3.
    - **Démontage** : tout QML neuf protège ses accès à `commands` (et à `documentController`, `tools`, `pageArea`),
      nuls pendant le démontage (`if (!root.commands) return` dans les gestionnaires, `root.commands ? … : …` dans
      les liaisons) ; `tst_ui_shell::shellLoadsWithoutQmlWarning` (`tests/tst_ui_shell.cpp:845`,
      `QTest::failOnWarning`) échoue sur tout avertissement QML : il fait partie de la preuve de chaque tâche qui
      touche un QML (suite complète).
    - **Injection** : étapes de scénario `touch`, `pinch`, `pointer`, `doubleClick` jouées par
      `QTest::createTouchDevice` et `QTest::touchEvent` (un `QTouchEvent` posté n'atteint pas Qt Quick) ; `expect`
      accepte `tolerance` (`bench/src/uiscenario.cpp:214`, `:243-244`) ; les aides de test du lot font de même
      (`tests/support/presentationharness.h`, L7-S8, étape 6).
14. **Deux lots** (plus de 20 tâches) : **P2-A** = L7-S8, L7-T1 à L7-T19 (20 tâches, ancien 7a) ; **P2-B** = L7-T21,
    L7-T23 à L7-T34 (13 tâches, ancien 7b). Un seul fichier, à la demande de l'orchestrateur ; les lots s'exécutent
    l'un après l'autre.

## Compte de tests

- **N_F** = nombre de tests ctest de `feat/livraison-complete-b` juste après la fusion du socle : **N_F = 129** =
  113 (suite de A sur `feat/livraison-complete`, debug et release 113/113) + 16 (phase 1) ; à confirmer par la suite
  de l'orchestrateur en cours sur 131f790 (inscrite dans l'état ; une autre valeur remplace 129 partout).
- **L7-S8 déclare 29 tests** : **N = N_F + 29 = 158** pour L7-S8 et **toutes** les tâches suivantes jusqu'à L7-T32
  (« = »). n_B = 16 + 29 = **45**.
- Les 29 : 17 entrées ctest d'interface, toutes servies par l'exécutable unique `tst_ui_lot7` (décision 1) —
  `tst_ui_presentation` (L7-S8, L7-T2, L7-T32), `tst_ui_curtain` (T3),
  `tst_ui_spotlight` (T4), `tst_ui_pagezoom` (T5), `tst_ui_pagetools` (T6), `tst_ui_transitions` (T7, T8),
  `tst_ui_mathtools` (T9 à T13), `tst_ui_dice` (T14), `tst_ui_calculator` (T15), `tst_ui_clock` (T16),
  `tst_ui_othertools` (T17), `tst_ui_ticker` (T18, T19), `tst_ui_recognition` (T21, T23), `tst_ui_equation` (T26,
  T27), `tst_ui_camera` (T28), `tst_ui_keyboard` (T29), `tst_ui_desktop` (T30, T31) ; 3 de logique —
  `tst_core_equation` (T24), `tst_render_equation` (T25), `tst_features_equationeditor` (T26) ; 9 de captures —
  `tst_captures_rideau` (T3), `tst_captures_spot` (T4), `tst_captures_effets` (T8), `tst_captures_regle` (T9),
  `tst_captures_compte_a_rebours` (T16), `tst_captures_message` (T19), `tst_captures_reconnaissance_forme` (T21),
  `tst_captures_equation` (T27), `tst_captures_presentation` (T32).
- `tst_core_presentation` (7a, L7-T1) disparaît (socle « Disparaissent » 2) : il n'est pas déclaré.
- Après L7-T33 : **N = N_A + 45** (N_A relevé par l'orchestrateur avant L7-T33), où N_A = compte de `feat/livraison-complete` à la livraison du lot 3 (socle
  compris ; le lot 3 n'enregistre aucun test, socle:49, mais A peut en avoir ajouté).

## Table des tâches — lot P2-A (L7-S8, L7-T1 à L7-T19)

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L7-S8 | Squelette du lot 7 (ancienne L0-S8) : `Presentation.qml` et 17 racines, 10 QML talons, `ui/presentation.cmake` aux noms figés, `DocumentInsertions` (en-tête figé, corps talon), aides de test, 29 tests et 9 scénarios talons | fusion du socle dans `feat/livraison-complete-b` ; L7-P0 à L7-P9 toutes mergées | `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `tests/presentation.cmake`, `ui/src/presentationdocument.h`, `ui/src/presentationdocument.cpp`, `ui/qml/{CurtainOverlay,SpotlightOverlay,PageZoomOverlay,PerPageTools,TransitionLayer,PageEffectsPicker,MathToolsLayer,DicePanel,CalculatorPanel,ClockPanel,TickerOverlay,RecognitionAlternatives,EquationEditor,CameraOverlay,ScreenKeyboard,DesktopAnnotation,OtherToolsHost}.qml`, `ui/qml/{FloatingPanel,RulerTool,SetSquareTool,ProtractorTool,CompassTool,OriginMarker,CountdownDialog,TickerDialog,CameraPalette,DesktopToolsWheel}.qml`, `tests/support/presentationharness.h`, `tests/support/lot7registry.h`, `tests/tst_ui_lot7_main.cpp`, `tests/tst_ui_{presentation,curtain,spotlight,pagezoom,pagetools,transitions,mathtools,dice,calculator,clock,othertools,ticker,recognition,equation,camera,keyboard,desktop}.cpp`, `tests/tst_core_equation.cpp`, `tests/tst_render_equation.cpp`, `tests/tst_features_equationeditor.cpp`, `bench/scenarios/{rideau,spot,effets,regle,compte-a-rebours,message,reconnaissance-forme,equation,presentation}.json` | isolé | concepteur-critique | `TESTS OK`, **N = N_F + 29**, les 29 tests neufs Passed ; 20 lignes `SKIP   : …reserved() Talon L7-S8 : rempli par L7-T<n>.` (16 classes de `tst_ui_lot7` hors `presentation`, 3 de logique, et le `reserved()` de `tst_ui_presentation`, pour L7-T2) ; `tst_ui_presentation` : PASS de `lot7RootsExist` ; 9 `tst_captures_*` Passed (`waitReady`) ; `premiers-pas` hors écran `CAPTURES OK` (`LU pageIndicator.text=1 / 2 OK`) ; `NOMS OK` après `git add` et après le commit | S10, S22, S23, S24 (support) | #tache-l7-s8 |
| L7-T1 | Objet MathML des `.flipchart` lu en équation (reste de L7-T1) | L7-S8 | `formats/src/flipchart.cpp`, `formats/src/flipchartids.h`, `tests/support/flipchartwriter.h`, `tests/tst_formats_flipchart.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_formats_flipchart` Passed, PASS de `mathmlObjectBecomesEquation` | S24 | #tache-l7-t1 |
| L7-T2 | Insertions du lot (`DocumentInsertions`), panneau flottant couverture, observateur des traits, signal `documentReplaced`, retrait de ressource à l'annulation | L7-S8 | `ui/src/presentationdocument.cpp`, `ui/qml/FloatingPanel.qml`, `tests/tst_ui_presentation.cpp`, `ui/include/actcraft/ui/documentcontroller.h`, `ui/src/documentcontroller.cpp` (ajout seulement : signal `documentReplaced`), `core/include/actcraft/core/document.h`, `core/src/document.cpp` (ajout seulement : `removeResource`) | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_presentation` 0 skipped, PASS de `insertTextIsUndoneAtOnce`, `insertObjectOnNewPageIsUndoneAtOnce`, `replaceObjectsIsUndoneAtOnce`, `setPagePresentationIsUndoneAtOnce`, `insertImageIsUndoneAtOnce`, `changeObserverSeesEveryStroke`, `loadEmitsDocumentReplaced`, `strokeDoesNotEmitDocumentReplaced`, `insertReadsIdsOnlyWhenApplied`, `floatingPanelIsACover(stylet)`, `(doigt)`, `(souris)`, `coveredStylusLeavesFingersFree` ; banc hors écran joint | S10 | #tache-l7-t2 |
| L7-T3 | Rideau (F-265) : contrôleur, surcouche couverture, scénario | L7-S8 | `ui/src/curtaincontroller.h`, `ui/src/curtaincontroller.cpp`, `ui/qml/CurtainOverlay.qml`, `tests/tst_ui_curtain.cpp`, `bench/scenarios/rideau.json` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_curtain` 0 skipped, PASS de `curtainNeverRefreshesTheCache`, `curtainBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; `tst_captures_rideau` Passed ; captures `rideau` hors écran `LU curtain.coverage=…` puis `PIXEL` couvert et découvert OK ; banc hors écran joint | S10 | #tache-l7-t3 |
| L7-T4 | Spot (F-270) : contrôleur, surcouche (contour 10, curseurs par zone), capture de la zone éclairée | L7-T2 | `ui/src/spotlightcontroller.h`, `ui/src/spotlightcontroller.cpp`, `ui/qml/SpotlightOverlay.qml`, `tests/tst_ui_spotlight.cpp`, `bench/scenarios/spot.json` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_spotlight` 0 skipped, PASS de `spotlightNeverRefreshesTheCache`, `spotlightBlocksTheStylus(stylet)`, `(doigt)`, `(souris)`, `spotlightOutlineIsTheDefault`, `spotlightCursorFollowsTheZone`, `spotlightCaptureIsUndoneAtOnce` ; `tst_captures_spot` Passed ; banc hors écran joint | S10 | #tache-l7-t4 |
| L7-T5 | Zoom de page (F-260) : contrôleur sur `zoomAround`, surcouche couverture (appui long droit compris) | L7-S8 | `ui/src/pagezoomcontroller.h`, `ui/src/pagezoomcontroller.cpp`, `ui/qml/PageZoomOverlay.qml`, `tests/tst_ui_pagezoom.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_pagezoom` 0 skipped, PASS de `pageZoomRefreshesTheCacheOnceAtRelease` (exactement 1), `pageZoomLeavesTheDocumentUnchanged`, `pageZoomBlocksTheStylus(stylet)`, `(doigt)`, `(souris)`, `rightHoldIsTakenByTheCover` ; banc hors écran joint | S10 | #tache-l7-t5 |
| L7-T6 | Outils de page par page (F-591 à F-593) : conversion depuis `PagePresentation`, contrôleur, lecture `.flipchart` (propriétés codées 12, 16, 27, 30 seulement pour les valeurs établies par une source indépendante, sinon H7-16), mémoriser le rideau pour la page | L7-T1, L7-T2, L7-T3, L7-T4 | `features/include/actcraft/features/pagetools.h`, `features/src/pagetools.cpp`, `tests/tst_features_pagetools.cpp`, `ui/src/pagetoolscontroller.h`, `ui/src/pagetoolscontroller.cpp`, `ui/qml/PerPageTools.qml`, `ui/qml/CurtainOverlay.qml` (entrée « Mémoriser pour cette page » du menu du rideau), `tests/tst_ui_pagetools.cpp`, `formats/src/flipchart.cpp`, `formats/src/flipchartids.h`, `tests/support/flipchartwriter.h`, `tests/tst_formats_flipchart.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_features_pagetools` 0 skipped dont PASS de `curtainModeOffMeansFull`, `conversionKeepsEveryValue` ; `tst_formats_flipchart` PASS de `pagePresentationRead` (propriétés 13, 18, 29 et valeurs établies des propriétés codées seulement), et de `pageToolsPropertyRead` si et seulement si au moins une valeur de la propriété 27 est établie (sinon fonction absente, H7-16 au rapport) ; `tst_ui_pagetools` PASS de `pageChangeAppliesPageTools`, `openedDocumentAppliesPageTools`, `strokeKeepsTheLiveCurtain`, `rememberCurtainIsUndoneAtOnce` ; banc hors écran joint | S10 | #tache-l7-t6 |
| L7-T7 | Effets de changement de page I (F-640) : lecteur en surcouche, effet de la page quittée, enveloppe de `page.next` et `page.previous` | L7-T3 | `ui/src/transitioncontroller.h`, `ui/src/transitioncontroller.cpp`, `ui/qml/TransitionLayer.qml`, `tests/tst_ui_transitions.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_transitions` 0 skipped, PASS de `transitionDoesNotRefreshTheCacheBeforeTheEnd`, `snapshotShowsAPendingStroke`, `fastRepeatLosesNoPage`, `noEffectWhenNone`, `pageEffectReadFromThePresentation`, `noEffectWhileCurtainActive` ; banc hors écran joint | S22 | #tache-l7-t7 |
| L7-T8 | Effets II : navigateur `PageEffectsPicker.qml` (18 entrées), scénario `effets` | L7-T7, L7-T2 | `ui/qml/PageEffectsPicker.qml`, `tests/tst_ui_transitions.cpp`, `bench/scenarios/effets.json` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_transitions` PASS de `browserListsEveryEffect` (18 délégués), `browserChoiceSetsTheGlobalEffect`, `pageEffectsPickerBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; `tst_captures_effets` Passed ; captures `effets` hors écran `LU transition.played=dissolve OK` | S22 | #tache-l7-t8 |
| L7-T9 | Règle (F-280) : contrôleur des outils de maths sur `setSampleFilter`, couche, curseurs par zone, translucidité 100 | L7-S8 | `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/MathToolsLayer.qml`, `ui/qml/RulerTool.qml`, `tests/tst_ui_mathtools.cpp`, `bench/scenarios/regle.json` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_mathtools` 0 skipped, PASS de `rulerGuidesThePenWithoutChangingIt`, `syntheticReleaseStaysOnTheGuide`, `mathToolIsNotSaved`, `closingTheRulerKeepsTheStroke`, `openToolsKeptInTheUserState`, `mathToolOpacityIsTheDefault`, `mathToolCursorFollowsTheZone`, `rulerBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; `tst_captures_regle` Passed ; banc hors écran joint | S10 | #tache-l7-t9 |
| L7-T10 | Équerre (F-283) : délégué et guidage | L7-T9 | `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/MathToolsLayer.qml`, `ui/qml/SetSquareTool.qml`, `tests/tst_ui_mathtools.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_mathtools` PASS de `setSquareGuidesThePen`, `setSquareOpacityIsTheDefault`, `setSquareCursorFollowsTheZone`, `setSquareBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; banc hors écran joint | S10 | #tache-l7-t10 |
| L7-T11 | Rapporteur (F-281) : délégué, rotation, arc fermé ou plein inséré | L7-T10, L7-T2 | `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/MathToolsLayer.qml`, `ui/qml/ProtractorTool.qml`, `tests/tst_ui_mathtools.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_mathtools` PASS de `filledArcBecomesOneShape`, `closedArcIsUndoneAtOnce`, `protractorTurnsByItsHandle`, `protractorOpacityIsTheDefault`, `protractorCursorFollowsTheZone`, `protractorBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; banc hors écran joint | S10 | #tache-l7-t11 |
| L7-T12 | Compas (F-282) : délégué, mine à la couleur du stylo, croix du centre | L7-T11 | `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/MathToolsLayer.qml`, `ui/qml/CompassTool.qml`, `tests/tst_ui_mathtools.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_mathtools` PASS de `markCentreAddsACross`, `compassStrokeIsACircle`, `compassLeadTakesThePenColour`, `compassOpacityIsTheDefault`, `compassCursorFollowsTheZone`, `compassBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; banc hors écran joint | S10 | #tache-l7-t12 |
| L7-T13 | Origine XY (F-284) : marqueur, mémoire par page dans `PagePresentation` | L7-T12 | `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/MathToolsLayer.qml`, `ui/qml/OriginMarker.qml`, `tests/tst_ui_mathtools.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_mathtools` PASS de `originSavedWithThePage`, `originMoveIsUndoneAtOnce`, `originReadFromAnOpenedDocument`, `strokeKeepsTheOrigin`, `originMarkerOpacityIsTheDefault`, `originMarkerBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` | S10 | #tache-l7-t13 |
| L7-T14 | Dés (F-285, E5) : panneau, total écrit, nombre et vitesse mémorisés | L7-T2 | `ui/src/dicecontroller.h`, `ui/src/dicecontroller.cpp`, `ui/qml/DicePanel.qml`, `tests/tst_ui_dice.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_dice` 0 skipped, PASS de `diceTotalWrittenOnThePage`, `oneDieTheFirstTime`, `diceCountAndSpeedRemembered`, `dicePanelClosesByItsCross`, `diceBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` | S10 | #tache-l7-t14 |
| L7-T15 | Calculatrice (F-286) : panneau à deux lignes, deux effacements, calcul posé sur la page | L7-T2 | `ui/src/calculatorcontroller.h`, `ui/src/calculatorcontroller.cpp`, `ui/qml/CalculatorPanel.qml`, `tests/tst_ui_calculator.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_calculator` 0 skipped, PASS de `systemCalculatorLaunchedOnWindows`, `panelShowsTheCalculationAboveTheResult`, `clearEntryAndClearAllAreTwoKeys`, `calculationToPageIsUndoneAtOnce`, `calculatorBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` | S10 | #tache-l7-t15 |
| L7-T16 | Horloge et compte à rebours (F-290, H7-14) : panneau, boîte, actions de fin, scénario | L7-T2 | `ui/src/clockcontroller.h`, `ui/src/clockcontroller.cpp`, `ui/qml/ClockPanel.qml`, `ui/qml/CountdownDialog.qml`, `tests/tst_ui_clock.cpp`, `bench/scenarios/compte-a-rebours.json` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_clock` 0 skipped, PASS de `countdownEndGoesToNextPage`, `endActionsAreTheThreeOfThisLot`, `clockBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; `tst_captures_compte_a_rebours` Passed (`LU pageIndicator.text=2 / 2 OK` après 3 s) | S10 | #tache-l7-t16 |
| L7-T17 | Date et heure (F-300), navigateur web (F-299) | L7-T2 | `ui/src/presentationcommands.h`, `ui/src/presentationcommands.cpp`, `ui/qml/OtherToolsHost.qml`, `tests/tst_ui_othertools.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_othertools` 0 skipped, PASS de `dateTimeInsertsOneText`, `webBrowserOpensTheHomeUrl`, `webBrowserRefusesOtherSchemes` | S10 | #tache-l7-t17 |
| L7-T18 | Message défilant I (F-291) : contrôleur, surcouche animée, banc | L7-S8 | `ui/src/tickercontroller.h`, `ui/src/tickercontroller.cpp`, `ui/qml/TickerOverlay.qml`, `tests/tst_ui_ticker.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_ticker` 0 skipped, PASS de `tickerNeverRefreshesTheCache`, `tickerBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; banc hors écran avec message en boucle joint | S23 | #tache-l7-t18 |
| L7-T19 | Message défilant II (F-291, H7-15) : boîte d'édition, « Essayer » et « Appliquer », fichiers de messages, scénario | L7-T18, L7-T2 | `ui/src/tickercontroller.h`, `ui/src/tickercontroller.cpp`, `ui/qml/TickerOverlay.qml`, `ui/qml/TickerDialog.qml`, `tests/tst_ui_ticker.cpp`, `bench/scenarios/message.json` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_ticker` PASS de `applyKeepsTheDefault`, `tryDoesNotKeepTheDefault`, `messageFileLoadedFromTheDialog`, `tickerDialogBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` ; `tst_captures_message` Passed | S23 | #tache-l7-t19 |

20 tâches dans le lot P2-A.

## Table des tâches — lot P2-B (L7-T21, L7-T23 à L7-T34)

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L7-T21 | Reconnaissance de forme II : contrôleur (délai 1 s, 0 à 5), trait remplacé par la forme en une étape, polygone, scénario | lot P2-A | `ui/src/recognitioncontroller.h`, `ui/src/recognitioncontroller.cpp`, `ui/qml/RecognitionAlternatives.qml`, `tests/tst_ui_recognition.cpp`, `bench/scenarios/reconnaissance-forme.json` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_recognition` PASS de `recognizedStrokeMakesOneStep`, `fillOptionFillsTheShape`, `roughQuadrilateralBecomesAPolygonShape`, `controllerDelayIsTheSessionDefault`, `sessionResetsOnAnOpenedDocument`, `strokeKeepsThePendingRecognition` ; `tst_captures_reconnaissance_forme` Passed ; banc hors écran joint | S10 | #tache-l7-t21 |
| L7-T23 | Reconnaissance d'écriture II : mode écriture qui possède un `HandwritingThread`, mot → texte en une étape, alternatives couverture, commande grisée, absente d'Android | L7-T21 | `ui/src/recognitioncontroller.h`, `ui/src/recognitioncontroller.cpp`, `ui/qml/RecognitionAlternatives.qml`, `tests/tst_ui_recognition.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_recognition` PASS de `handwritingMakesOneStep` (ou SKIP « reconnaisseur absent », signalé), `handwritingDisabledWithExplanation`, `handwritingAbsentOnAndroid`, `controllerOwnsOneHandwritingThread`, `clickOutsideValidatesTheText`, `eachWordBecomesItsOwnText`, `alternativesBlockTheStylus(stylet)`, `(doigt)`, `(souris)` ; banc hors écran joint | S10 | #tache-l7-t23 |
| L7-T24 | Équation I (F-301) : arbre et MathML (17 modèles) | lot P2-A | `core/include/actcraft/core/equation.h`, `core/src/equation.cpp`, `tests/tst_core_equation.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_core_equation` 0 skipped, PASS de `templatesRoundTrip` (17 lignes de données), `unknownElementKeptVerbatim`, `entitiesDecoded`, `malformedIsAnErrorNotACrash`, `nonAsciiRawKeptVerbatim` ; `git grep "Talon L0-S : rempli par L7-T24"` vide | S24 | #tache-l7-t24 |
| L7-T25 | Équation II : mise en page et dessin, en-tête public `equationpaint.h` (modes `Page` et `Editor`) | L7-T24 | `render/include/actcraft/render/equationpaint.h`, `render/CMakeLists.txt` (la ligne de l'en-tête public dans la liste, et elle seule), `render/src/equationlayout.h`, `render/src/equationlayout.cpp`, `tests/tst_render_equation.cpp`, `tests/tst_render_painter.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_render_equation` 0 skipped, PASS de `fractionIsTallerThanItsParts`, `superscriptIsRaised`, `rootCoversItsContent`, `malformedDrawsTheNeutralFrame`, `paintedMatchesSoftware`, `paintFromTwoThreads`, `fixedPageDrawsTheEquation`, `editorModeDrawsPlaceholderAndCursor`, `pageDrawsNoPlaceholder` ; `tst_render_painter` PASS de ses fonctions existantes et de `pageModeIsUnchanged`, `editorModeShowsAnEmptyBox` ; `git grep "Talon L0-S : rempli par L7-T25"` vide ; banc hors écran joint | S24 | #tache-l7-t25 |
| L7-T26 | Équation III : éditeur à modèles, palettes du rapport, aperçu, insertion | L7-T25 | `features/include/actcraft/features/equationeditor.h`, `features/src/equationeditor.cpp`, `tests/tst_features_equationeditor.cpp`, `ui/src/equationcontroller.h`, `ui/src/equationcontroller.cpp`, `ui/src/equationpreviewitem.h`, `ui/src/equationpreviewitem.cpp`, `ui/qml/EquationEditor.qml`, `tests/tst_ui_equation.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_features_equationeditor` 0 skipped, PASS de `fractionTemplateThenTwoDigits`, `backspaceLeavesAPlaceholder`, `arrowsWalkTheTree`, `everyTemplateHasAPaletteEntry` ; `tst_ui_equation` PASS de `insertEquationIsUndoneAtOnce`, `insertedEquationHasNoEditorMark`, `incompleteEquationIsRefused`, `editorHasItsOwnKeys`, `previewMatchesThePageWithoutMarks`, `equationEditorBlocksTheStylus(stylet)`, `(doigt)`, `(souris)` | S24 | #tache-l7-t26 |
| L7-T27 | Équation IV : réédition, aller-retour, `.flipchart`, scénario S24 | L7-T26, L7-T1 | `ui/src/equationcontroller.h`, `ui/src/equationcontroller.cpp`, `ui/qml/EquationEditor.qml`, `tests/tst_ui_equation.cpp`, `bench/scenarios/equation.json` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_equation` PASS de `editKeepsTheId`, `equationRoundTripIdentical`, `untouchedEquationKeepsItsBytes`, `flipchartEquationIsDrawn` ; `tst_captures_equation` Passed ; captures `equation` hors écran `LU documentController.fingerprint=${avant} OK` | S24 | #tache-l7-t27 |
| L7-T28 | Caméra (F-275, bureau) : liste des fenêtres (`camera_win.cpp`, `camera_other.cpp`), contrôleur, surcouche et palette couvertures | lot P2-A | `features/include/actcraft/features/camera.h`, `features/src/camera_win.cpp`, `features/src/camera_other.cpp`, `ui/src/cameracontroller.h`, `ui/src/cameracontroller.cpp`, `ui/qml/CameraOverlay.qml`, `ui/qml/CameraPalette.qml`, `tests/tst_ui_camera.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_camera` 0 skipped, PASS de `captureToPageAddsOneImage`, `captureToNewPageAddsAPage`, `captureToClipboard`, `cameraAbsentOnAndroid`, `emptyGrabShowsAMessage`, `captureAreaKeptForTheNextCapture`, `topLevelWindowsListsOurWindow`, `cameraWindowTakesEveryPointer(stylet)`, `(doigt)`, `(souris)` ; banc hors écran joint ; contrôle à l'écran annoncé (jeton `ecran`) | S10 | #tache-l7-t28 |
| L7-T29 | Clavier à l'écran (F-294, bureau) : injecteur (`keyinjector_win.cpp`, `keyinjector_other.cpp`), contrôleur, fenêtre sans focus | lot P2-A | `features/include/actcraft/features/screenkeyboard.h`, `features/src/keyinjector_win.cpp`, `features/src/keyinjector_other.cpp`, `ui/src/keyboardcontroller.h`, `ui/src/keyboardcontroller.cpp`, `ui/qml/ScreenKeyboard.qml`, `tests/tst_ui_keyboard.cpp` | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_keyboard` 0 skipped, PASS de `keyboardTypesIntoTheFocusedField`, `keyboardWindowNeverTakesFocus`, `keyboardWindowTakesEveryPointer(stylet)`, `(doigt)`, `(souris)` ; banc hors écran joint | S10 | #tache-l7-t29 |
| L7-T30 | Annotation de bureau (F-070, F-071, bureau) | lot P2-A | `ui/src/desktopcontroller.h`, `ui/src/desktopcontroller.cpp`, `ui/qml/DesktopAnnotation.qml`, `tests/tst_ui_desktop.cpp`, `app/main.cpp` (repli de la transparence seulement) | isolé | concepteur-critique | `TESTS OK`, N = N_F + 29, `tst_ui_desktop` PASS de `annotationWindowIsTransparentAndAbove`, `strokeGoesToTheDesktopPaperboard`, `passThroughMakesTheWindowTransparentForInput`, `desktopCaptureBecomesTheBackground`, `liveDesktopBackgroundIsNotSaved`, `nextPageIsANewTransparentPage`, `desktopPaperboardKeptBetweenActivations`, `onlyItsPageSeesTheDesktop`, `desktopWindowTakesEveryPointer(stylet)`, `(doigt)`, `(souris)` ; banc hors écran joint ; contrôle à l'écran annoncé | S10 | #tache-l7-t30 |
| L7-T31 | Outils de bureau (F-080, bureau) : roue et anneaux | L7-T30 | `ui/src/desktopcontroller.h`, `ui/src/desktopcontroller.cpp`, `ui/qml/DesktopAnnotation.qml`, `ui/qml/DesktopToolsWheel.qml`, `tests/tst_ui_desktop.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_desktop` 0 skipped, PASS de `wheelHasNoVoteNorVendorEntry`, `restoreClosesTheWheel`, `wheelClosesWhenTheWindowIsRestored` | S10 | #tache-l7-t31 |
| L7-T32 | Contrôle du lot : gestionnaires, menus Android, couvertures, scénario `presentation`, épinglages, talons comptés ; glisser du calcul vers la page (P3) | L7-T19, L7-T23, L7-T27, L7-T28, L7-T29, L7-T31 | `tests/tst_ui_presentation.cpp`, `bench/scenarios/presentation.json`, `docs/verifications/AAAA-MM-JJ-lot7-epinglages.md`, `ui/src/calculatorcontroller.h`, `ui/src/calculatorcontroller.cpp`, `ui/qml/CalculatorPanel.qml`, `tests/tst_ui_calculator.cpp` | isolé | implementeur-tdd | `TESTS OK`, N = N_F + 29, `tst_ui_presentation` PASS de `everyLot7CommandHasAHandler`, `androidMenusHaveNoDesktopCommand`, `everyInputPanelIsACover` ; `tst_ui_calculator` PASS de `calculationDraggedToThePageIsUndoneAtOnce` ; `tst_captures_presentation` Passed ; captures `presentation` hors écran `CAPTURES OK` ; `git grep "Talon L7-S8 : rempli par"` vide | S10 | #tache-l7-t32 |
| L7-T33 | Fusion avec le lot 3 | L7-T32 et **livraison du lot 3 dans `feat/livraison-complete`** | fichiers en conflit (aucun attendu hors `graphify-out/`) ; `graphify-out/` | partagé | concepteur-critique | aucun marqueur restant ; debug et release `TESTS OK`, **N = N_A + 45** (N_A relevé) ; captures hors écran `-Scenario tous` `CAPTURES OK` ; `NOMS OK` ; `graphify update .` commité | S10, S22, S23, S24 (rejoués) | #tache-l7-t33 |
| L7-T34 | Après fusion du lot 3 : pivot sur l'origine, conversions de la sélection, double-clic sur une équation, « rétablir la page », guides du marqueur et de la gomme, clics qui traversent | L7-T33 | `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/MathToolsLayer.qml`, `ui/src/recognitioncontroller.h`, `ui/src/recognitioncontroller.cpp`, `ui/src/equationcontroller.h`, `ui/src/equationcontroller.cpp`, `ui/src/clockcontroller.h`, `ui/src/clockcontroller.cpp`, `ui/src/desktopcontroller.h`, `ui/src/desktopcontroller.cpp`, `ui/qml/RecognitionAlternatives.qml`, `ui/qml/EquationEditor.qml`, `ui/qml/ClockPanel.qml`, `ui/qml/CountdownDialog.qml`, `ui/qml/DesktopAnnotation.qml`, `tests/tst_ui_mathtools.cpp`, `tests/tst_ui_recognition.cpp`, `tests/tst_ui_equation.cpp`, `tests/tst_ui_clock.cpp`, `tests/tst_ui_desktop.cpp`, `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp`, `tests/tst_bench_scenarios.cpp`, `tests/tst_bench_result.cpp` | isolé | concepteur-critique | `TESTS OK`, N = celui de L7-T33 (aucun test ajouté) ; PASS de `rotationPivotsOnTheOrigin`, `markerFollowsTheRuler`, `eraserFollowsTheCompass` (`tst_ui_mathtools`), `convertSelectionToShape`, `convertSelectionToText` (sauté sans reconnaisseur) (`tst_ui_recognition`), `doubleClickEditsTheEquation` (`tst_ui_equation`), `countdownCanRevertThePage`, `endActionsAreTheFourOfTheSpecification` (`tst_ui_clock`, qui remplace `endActionsAreTheThreeOfThisLot`), `selectToolPassesClicksThrough` (`tst_ui_desktop`), `recognitionScenarioListed` (`tst_bench_scenarios`) ; banc hors écran : scénario `reconnaissance` présent, `cache.refreshes` relevé | S10, S24 | #tache-l7-t34 |

13 tâches dans le lot P2-B ; **33 tâches** en tout (L7-T0, L7-T20 et L7-T22 faites). « lot P2-A » en dépendance =
toutes les tâches du lot P2-A mergées (les lots s'exécutent l'un après l'autre). Deux noms de 7b changent pour suivre
le suffixe imposé (`…MakesOneStep`) : `recognizedStrokeBecomesAShapeInOneStep` → `recognizedStrokeMakesOneStep`,
`handwritingBecomesTextInOneStep` → `handwritingMakesOneStep`.

## Vagues (deux tâches à la fois au plus ; le code s'écrit en parallèle, les constructions passent une à une)

Profil `.claude/chantier.md`, « Parallélisme » : deux tâches de front au plus ; toute construction, suite, banc ou
capture passe par le jeton `tests` (une place pour B sauf accord de A par la session Pilote ; un agent qui attend la
notification ne sonde pas).

**Préalable** (revue contre le socle, M1) : la phase 2 ne démarre qu'une fois **L7-P0 à L7-P9 toutes mergées** dans
`feat/livraison-complete-b` (à 131f790 : P6 et P9 en cours de merge ; L7-T4 appelle `cropToPolygon` de L7-P9, L7-T18
et L7-T19 le message défilant de L7-P6, L7-T28, L7-T29 et L7-T31 la caméra, le clavier et la roue de L7-P9). L7-S8,
étape 1, le vérifie.

**Lot P2-A.** V1 : L7-S8 (seule) · V2 : L7-T2 ‖ L7-T1 · V3 : L7-T3 ‖ L7-T9 · V4 : L7-T4 ‖ L7-T10 ·
V5 : L7-T5 ‖ L7-T11 · V6 : L7-T6 ‖ L7-T12 · V7 : L7-T7 ‖ L7-T13 · V8 : L7-T8 ‖ L7-T14 · V9 : L7-T15 ‖ L7-T16 ·
V10 : L7-T17 ‖ L7-T18 · V11 : L7-T19.

**Lot P2-B.** V12 : L7-T21 ‖ L7-T24 · V13 : L7-T23 ‖ L7-T25 · V14 : L7-T26 ‖ L7-T30 · V15 : L7-T27 ‖ L7-T31 ·
V16 : L7-T28 ‖ L7-T29 · V17 : L7-T32 · V18 : L7-T33 (attend le lot 3) · V19 : L7-T34. L7-T28 et L7-T30 (jeton
`ecran`) ne sont jamais dans la même vague.

Recoupements vérifiés sur la colonne « fichiers touchés » : dans chaque vague, les deux tâches n'ont aucun fichier
commun. Chaînes tenues par « dépend de » : `mathtoolscontroller.*`, `MathToolsLayer.qml`, `tst_ui_mathtools.cpp`
(T9 → T10 → T11 → T12 → T13 → T34) ; fichiers de `formats/` (T1 → T6) ; `CurtainOverlay.qml` (T3 → T6 ; L7-T12, voisine de L7-T6 en V6, n'y
touche pas) ; `tst_ui_transitions.cpp` (T7 → T8) ;
`tickercontroller.*`, `TickerOverlay.qml`, `tst_ui_ticker.cpp` (T18 → T19) ; `recognitioncontroller.*`,
`RecognitionAlternatives.qml`, `tst_ui_recognition.cpp` (T21 → T23 → T34) ; `equationcontroller.*`,
`EquationEditor.qml`, `tst_ui_equation.cpp` (T26 → T27 → T34) ; `desktopcontroller.*`, `DesktopAnnotation.qml`,
`tst_ui_desktop.cpp` (T30 → T31 → T34) ; `tst_ui_presentation.cpp` (S8 → T2 → T32) ; `clockcontroller.*`,
`tst_ui_clock.cpp`, `ClockPanel.qml`, `CountdownDialog.qml` (T16 → T34) ; `calculatorcontroller.*`,
`CalculatorPanel.qml`, `tst_ui_calculator.cpp` (T15 → T32) ; `RecognitionAlternatives.qml`, `EquationEditor.qml`,
`DesktopAnnotation.qml` (→ T34) ; fichiers créés en talon par L7-S8 → leur tâche. `render/CMakeLists.txt`, `render/include/actcraft/render/equationpaint.h`,
`tests/tst_render_painter.cpp` : L7-T25 seule (aucune autre tâche ne les touche ; sa voisine de V13, L7-T23, touche
`recognitioncontroller.*`, `RecognitionAlternatives.qml`, `tst_ui_recognition.cpp`). Dans une vague, la tâche
`concepteur-critique` part la première (sa revue adversariale chevauche la vague suivante). Une tâche peut avancer
d'une vague si ses dépendances sont mergées et qu'elle ne recoupe pas sa voisine.

**Écran.** Seules L7-T28 et L7-T30 ouvrent une fenêtre visible (jeton `ecran`, annoncé à Jérôme d'une ligne avant le
lancement, 7b « Écran ») ; tout le reste se joue hors écran.

## Couverture des scénarios du cadrage

| Scénario | Tâches |
|---|---|
| S10 — présentation | L7-S8 (support), L7-T2 à L7-T6, L7-T9 à L7-T17, L7-T21, L7-T23, L7-T28 à L7-T34 |
| S22 — effets de page | L7-S8 (support), L7-T7, L7-T8, L7-T33 (rejoué) |
| S23 — message défilant | L7-S8 (support), L7-T18, L7-T19, L7-T33 (rejoué) |
| S24 — équation | L7-S8 (support), L7-T1, L7-T24 à L7-T27, L7-T34 |
| S30 — tâche d'ouverture | L7-T0 (faite, f0772b6) |

## Règles communes à toutes les tâches (à joindre à chaque section)

Celles de 7a (« Règles communes », 7a:263-327) et celles du socle (socle:204-266 : processus enfant, `git -C`, jamais
de `cd`, `format.ps1` avant la preuve, pièges de Clang, pas de `std::visit` direct sur `PageObject`, aucune API
privée de Qt, propriété intellectuelle, `samples/` en lecture seule, arrêt par PID, fil de Qt Quick, mutation remise
par `git checkout --`, pas d'apostrophe dans un `R"(...)"` d'un test moc) s'appliquent pour tout ce qui suit et
qu'elles ne contredisent pas. S'y ajoutent ou s'y substituent :

- `<wt>` = `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b--<id>` ; `<id>` = `L7-S8` ou `L7-T<n>`.
  Toute commande PowerShell commence par `Set-Location '<wt>';` ou passe par `git -C '<wt>'`. Toujours
  `pwsh -NoProfile`.
- **Fichiers** : ceux de la ligne de la table, et aucun autre (décisions 6 et 7). Un besoin hors de la liste :
  s'arrêter et le rapporter à l'orchestrateur.
- **Section** = section de la tâche dans 7a ou 7b **corrigée par sa section ici**. Les étapes « test rouge de la
  logique » et « logique » des tâches reprises par la phase 1 sont faites : ne pas les rejouer, ne pas réécrire
  `features/` sauf ajout nommé ici.
- **Talon rempli** : la tâche réécrit chaque fichier talon de sa ligne ; marqueur `Talon L7-S8 : rempli par <id>.`
  et `reserved()` disparaissent. Contrôle : `git -C '<wt>' grep -n "Talon L7-S8 : rempli par <id>[ .]" -- . ':(exclude)docs/'` vide (le motif voit
  aussi la description d'un scénario talon, « … par L7-T<n> (S<k>). »).
- **Racine** : la tâche remplit sa racine (décisions 3 et 4) : contrôleur instancié dans la racine, `controller:`
  lié à lui, gestionnaires de commandes dans `Component.onCompleted` de la racine ; elle ne touche ni
  `Presentation.qml` ni la racine d'une autre fonction.
- **Couverture** (décision 5) : `<fonction>BlocksTheStylus` à données (`stylet`, `doigt`, `souris`), aides de
  `tests/support/presentationharness.h` ; aucun trait versé **et** effet du gestionnaire lu.
- **Signaux du document** (décision 13) : un contrôleur qui relit son état quand le document est remplacé écoute
  `documentReplaced` (L7-T2), **jamais** `documentChanged` (émis après chaque trait).
- **Démontage** (décision 13, point 3 de A) : chaque QML neuf protège ses accès à `commands`, `documentController`,
  `tools`, `pageArea` (nuls au démontage) ; `shellLoadsWithoutQmlWarning` reste vert dans la suite.
- **Exécutable d'interface unique** (décision 1) : dans le `tst_ui_<fonction>.cpp` de la tâche, tout est dans un
  `namespace { … }` anonyme (ou `static`) ; seule la ligne `ACTCRAFT_LOT7_TEST` est hors de lui ; aucun objet Qt de
  portée fichier. Une erreur de lien qui nomme le fichier d'une autre tâche : s'arrêter et la rapporter.
- **Noms imposés** : `shapefinder`, `spotlightFrame`, `OriginMarker.qml`, `PageEffectsPicker.qml` (et non
  `EffectPicker.qml` de 7a, L7-T8), tests d'annulation suffixés `…MakesOneStep` ou `…IsUndoneAtOnce`. Un nom refusé par
  le contrôle se renomme et le renommage est rapporté (amendement de ce plan).
- **Fonctions d'un test d'interface** (exécutable unique `tst_ui_lot7`, première option = clé de la classe :
  `presentation`, `curtain`, `spotlight`, `pagezoom`, `pagetools`, `transitions`, `mathtools`, `dice`, `calculator`,
  `clock`, `othertools`, `ticker`, `recognition`, `equation`, `camera`, `keyboard`, `desktop`) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; . ./scripts/dev-env.ps1; `$env:QT_QPA_PLATFORM='offscreen'; `$env:QT_QUICK_BACKEND='software'; & build/debug/tst_ui_lot7.exe <clé> -o .tmp/tst_ui_<clé>.txt,txt; Write-Output ('code=' + `$LASTEXITCODE); Select-String -Path .tmp/tst_ui_<clé>.txt -Pattern '^PASS   :|^FAIL!  :|^SKIP   :|^Totals' -ErrorAction SilentlyContinue | Select-Object -Last 40"
  ```
  Liste des fonctions (règle 4 du socle, socle:144, adaptée) : `& build/debug/tst_ui_lot7.exe <clé> -functions`.
  (chemin : celui que `ctest --preset debug -N -V -R tst_ui_<clé>` imprime s'il diffère). Pour les autres tests, la
  forme du socle (socle:231-235).
- **Premier passage dans un worktree neuf** (construction complète) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire livraison-complete-b/<id> -Commande 'pwsh -NoProfile -File ./scripts/build.ps1 *> .tmp/build.txt'; Select-String -Path .tmp/build.txt -Pattern 'error|FAILED' | Select-Object -Last 4"
  ```
- **Commande ciblée** (rouge puis vert, `<motif>` donné par la section ; sortie de Qt Test lue par `-V`) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire livraison-complete-b/<id> -Commande '. ./scripts/dev-env.ps1; cmake --build --preset debug *> .tmp/build.txt; ctest --preset debug -R ''<motif>'' -V *> .tmp/ctest.txt'; Select-String -Path .tmp/build.txt,.tmp/ctest.txt -Pattern 'error C|error:|FAIL!  :|SKIP   :|Totals:|tests passed|tests failed|No tests were found' | Select-Object -Last 12"
  ```
  Rouge attendu : `error C…` ou `error:` sur le nom testé, ou `FAIL!  :`. Vert attendu : une ligne `Totals: <k>
  passed, 0 failed, 0 skipped` par test du motif (sauf SKIP nommé par la table), `100% tests passed`. Une fonction se
  lit par `Select-String -Path .tmp/ctest.txt -Pattern 'PASS   : .*<fonction>\('`. Jamais de `|` sur la sortie du
  jeton : on lit les fichiers après.
- **Preuve de tâche** (forme bornée du profil) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire livraison-complete-b/<id> -Commande 'pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1'; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2; Select-String -Path .tmp/test.txt -Pattern '<nom_du_test> .*Passed' | Select-Object -Last 4"
  ```
  Attendu : `100% tests passed, 0 tests failed out of N` avec **N = N_F + 29** exactement (N_F lu dans l'état),
  `TESTS OK`, la ligne `Passed` de chaque test de la tâche. Un test a tourné : N ≥ 1, nom présent, et la commande
  ciblée verte a montré les fonctions nommées en `PASS` (un SKIP ne prouve rien).
- **Banc sans écran** et **preuve visuelle hors écran** : formes de 7a (7a:303-322), `-Proprietaire
  livraison-complete-b/<id>`, motif de lecture du socle (lignes `DOIGT`, `PINCEMENT` comprises, socle:247). Lectures
  `-Lire "Id=attendu"` d'abord, puis une phrase par capture nommée.
- **Contrôle des noms**, après `git -C '<wt>' add <fichiers>` puis après le commit :
  `python -I "C:\Users\JeromeDenis\REA - Activinspire\tools\controle_noms_actcraft.py" '<wt>'` → dernière ligne
  `NOMS OK` (l'outil ne lit que les fichiers suivis ; il dure plus de deux minutes : le lancer en arrière-plan).
- **Commit** : `git -C '<wt>' add` de chaque fichier de la ligne, nommé (jamais `-A`, `--all`, `.`), relire
  `git -C '<wt>' diff --cached --stat`, puis `git -C '<wt>' commit -m "<message de la section>" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"`.
- **Rapport** (avant la limite de tours) : sha, sorties bornées (ciblée verte, preuve, `NOMS OK` ×2), écarts.

---

## Tâches

<a id="tache-l7-s8"></a>
### Tâche L7-S8 — Squelette du lot 7 (ancienne L0-S8)

**Objectif.** Créer et enregistrer d'un coup tous les fichiers neufs d'interface du lot 7 que la phase 1 n'a pas
posés : `Presentation.qml` complété de ses 17 racines, 27 QML talons, `ui/presentation.cmake` (noms figés
`ACTCRAFT_UI_LOT7_QML`, `ACTCRAFT_UI_LOT7_SOURCES`), `DocumentInsertions` (en-tête figé, corps talon), aides de test
au stylet, au doigt et à la souris, exécutable d'interface unique `tst_ui_lot7` (registre et `main`), 29 entrées
ctest et 9 scénarios talons, avec tous les chemins d'inclusion privés dont les tâches auront besoin. Après elle,
aucune tâche ne touche un fichier CMake, et `Presentation.qml` n'est plus touché qu'en L7-T17 (au besoin). Aucun
comportement visible ne change. `concepteur-critique` : l'invariant partagé est le chargement du module QML (un doublon de `Presentation.qml`
ou une racine absente casse **toute** l'interface, pas seulement le lot 7).

**Fichiers.** Voir la table. Créer tous les fichiers de la ligne sauf `ui/qml/Presentation.qml` (existe, socle L0-S9)
et `tests/presentation.cmake` (existe, phase 1) : ces deux-là sont complétés.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_|tst_core_equation|tst_render_equation|tst_features_equationeditor|tst_captures_'`
(toutes les suites `tst_ui_*` du dépôt, celles du socle comprises : aucune ne doit régresser) ; preuve de tâche
(N = N_F + 29) ; preuve visuelle hors écran `premiers-pas` avec `-Lire "pageIndicator.text=1 / 2"` (la fenêtre n'a pas
changé) ; « Fonctions d'un test d'interface » (règles communes) sur la clé `curtain` → une ligne
`SKIP   : TstUiCurtain::reserved() Talon L7-S8 : rempli par L7-T3.` ; la même commande sans clé → `code=2` et la liste
des 17 clés sur la sortie d'erreur.

**Preuve visuelle.** `premiers-pas` hors écran : `LU pageIndicator.text=1 / 2 OK`, `CAPTURES OK` ; capture
`interface` décrite en une phrase (menus, palette et page comme avant ; aucune surcouche visible).

**Interdits.** Aucun comportement dans un talon (racines : propriétés de la décision 4 seulement) ; aucun fichier de la
décision 7 ; ne pas relister `qml/Presentation.qml` dans `ACTCRAFT_UI_LOT7_QML` (doublon de module, socle:1474-1476) ;
ni `qt_target_qml_sources` ni `qt_add_qml_module` dans `ui/presentation.cmake` (socle:1471-1473) ; aucune des 16
lignes de la phase 1 de `tests/presentation.cmake` modifiée ; aucun nom de classe dans un talon C++ ; libellés à nous.

**Terminé quand.** Tous les fichiers existent, compilent et sont enregistrés ; N = N_F + 29 ; chaque talon porte le
marqueur de sa tâche ; `lot7RootsExist` en `PASS` ; `shellLoadsWithoutQmlWarning` (socle) toujours en `PASS` ; `premiers-pas`
`CAPTURES OK` ; `TESTS OK` ; `NOMS OK` deux fois.

**Étapes.**

- [ ] **Étape 1 : base.** Vérifier que la base contient le socle : `git -C '<wt>' grep -n "ACTCRAFT_UI_LOT7_QML" -- ui/CMakeLists.txt`
  (au moins 2 lignes), `git -C '<wt>' grep -n "property Item layer" -- ui/qml/Presentation.qml` (1 ligne),
  `git -C '<wt>' grep -n "addChangeObserver" -- ui/include/actcraft/ui/documentcontroller.h` (≥ 1),
  `git -C '<wt>' grep -n "addInputCover" -- ui/include/actcraft/ui/pageview.h` (≥ 1). Phase 1 complète :
  `git -C '<wt>' grep -ln "cropToPolygon\|tickerToJson\|wheelPositions\|class ScreenKeyboard" -- features/include`
  (au moins `camera.h`, `ticker.h`, `desktopwheel.h`, `screenkeyboard.h`) et
  `git -C '<wt>' grep -n "Talon L7-P0" -- tests/` vide. Une sortie vide (ou non vide pour le dernier) : s'arrêter et
  le rapporter (socle ou phase 1 non mergés). Premier passage (construction complète) ; preuve de tâche : `out of N_F` (la valeur
  de l'état), `TESTS OK`.
- [ ] **Étape 2 : test rouge.** `tests/tst_ui_presentation.cpp` (montage de `tests/tst_ui_shell.cpp` : le lire et
  reprendre son chargement de `Main.qml` hors écran, par `tests/support/presentationharness.h` de l'étape 6 ; classe
  inscrite au registre de l'étape 7, aucun `QTEST_MAIN`) :
  ```cpp
  #include "support/lot7registry.h"
  #include "support/presentationharness.h"

  #include <QtTest>

  #include <utility>

  namespace {

  class TstUiPresentation : public QObject
  {
      Q_OBJECT

  private slots:
      void lot7RootsExist()
      {
          presentationharness::MainWindow main;
          QVERIFY(main.window() != nullptr);
          // QtObject, pas un Item (socle L0-S9).
          QObject* presentation = main.window()->findChild<QObject*>(QStringLiteral("presentation"));
          QVERIFY(presentation != nullptr);
          const char* roots[] = {"curtainRoot",      "spotlightRoot", "pageZoomRoot",    "pageToolsRoot",
                                 "transitionRoot",   "effectsRoot",   "mathToolsRoot",   "diceRoot",
                                 "calculatorRoot",   "clockRoot",     "tickerRoot",      "recognitionRoot",
                                 "equationRoot",     "cameraRoot",    "keyboardRoot",    "desktopRoot",
                                 "otherToolsRoot"};
          for (const char* name : roots) {
              QObject* root = main.window()->findChild<QObject*>(QString::fromLatin1(name));
              QVERIFY2(root != nullptr, name);
              QVERIFY2(root->property("commands").value<QObject*>() != nullptr, name);
          }
          auto* overlays = main.window()->findChild<QQuickItem*>(QStringLiteral("pageOverlays"));
          QVERIFY(overlays != nullptr);
          for (const char* name : {"curtainRoot", "spotlightRoot", "mathToolsRoot"}) {
              auto* root = main.window()->findChild<QQuickItem*>(QString::fromLatin1(name));
              QVERIFY2(root != nullptr && root->parentItem() == overlays, name);
          }
          auto* layer = main.window()->findChild<QQuickItem*>(QStringLiteral("presentationLayer"));
          auto* ticker = main.window()->findChild<QQuickItem*>(QStringLiteral("tickerRoot"));
          QVERIFY(layer != nullptr && ticker != nullptr);
          QCOMPARE(ticker->parentItem(), layer);
          // Empilement fixé par z (décision 3) : chaque valeur, pas seulement un ordre.
          const std::pair<const char*, qreal> stacking[] = {
              {"curtainRoot", 10},    {"spotlightRoot", 20},  {"mathToolsRoot", 30},  {"transitionRoot", 10},
              {"tickerRoot", 20},     {"diceRoot", 30},       {"calculatorRoot", 31}, {"clockRoot", 32},
              {"recognitionRoot", 40}, {"equationRoot", 41}, {"effectsRoot", 50}};
          for (const auto& [name, z] : stacking) {
              auto* root = main.window()->findChild<QQuickItem*>(QString::fromLatin1(name));
              QVERIFY2(root != nullptr, name);
              QVERIFY2(qFuzzyCompare(root->z(), z), name);
          }
          for (const char* name : {"cameraRoot", "keyboardRoot", "desktopRoot"}) {
              auto* root = main.window()->findChild<QQuickItem*>(QString::fromLatin1(name));
              QVERIFY2(root != nullptr && !root->isVisible(), name); // racine non visuelle (décision 4 bis)
          }
          // Parent visuel de chacune des 17 racines (décision 3) : 12 placées, 5 sans parent.
          auto* slot = main.window()->findChild<QQuickItem*>(QStringLiteral("presentationSlot"));
          QVERIFY(slot != nullptr);
          const std::pair<const char*, QQuickItem*> parents[] = {
              {"curtainRoot", overlays},    {"spotlightRoot", overlays},  {"mathToolsRoot", overlays},
              {"pageZoomRoot", slot},       {"transitionRoot", layer},    {"effectsRoot", layer},
              {"diceRoot", layer},          {"calculatorRoot", layer},    {"clockRoot", layer},
              {"tickerRoot", layer},        {"recognitionRoot", layer},   {"equationRoot", layer},
              {"pageToolsRoot", nullptr},   {"otherToolsRoot", nullptr},  {"cameraRoot", nullptr},
              {"keyboardRoot", nullptr},    {"desktopRoot", nullptr}};
          for (const auto& [name, expected] : parents) {
              auto* root = main.window()->findChild<QQuickItem*>(QString::fromLatin1(name));
              QVERIFY2(root != nullptr && root->parentItem() == expected, name);
          }
      }

      void reserved() { QSKIP("Talon L7-S8 : rempli par L7-T2."); }
  };

  } // namespace

  ACTCRAFT_LOT7_TEST("presentation", TstUiPresentation)
  #include "tst_ui_presentation.moc"
  ```
  Commande ciblée `-R 'tst_ui_presentation'` → rouge (cible `tst_ui_lot7` inconnue, ou fichier manquant).
- [ ] **Étape 3 : `DocumentInsertions`.** `ui/src/presentationdocument.h` : signatures de S8a (D-S8 du socle), deux
  écarts : `#include <cstddef>` pour `std::size_t`, et `addResource` **remplacé** par `insertImage` — vérifié :
  `DocumentController::document()` est `const` (`documentcontroller.h:76`) et le contrôleur n'a aucune méthode de
  ressource ; seule une commande (`redo(core::Document&, …)`) peut appeler `core::Document::addResource`
  (`core/include/actcraft/core/document.h:66`) :
  ```cpp
  #pragma once

  #include <actcraft/core/model.h>
  #include <actcraft/ui/documentcontroller.h>

  #include <QByteArray>
  #include <QRectF>
  #include <QString>

  #include <cstddef>
  #include <string>
  #include <vector>

  namespace actcraft::ui {

  // Insertions du lot 7 (F-285, F-295, F-296, F-300, F-301, F-275) : par la pile du contrôleur (execute), donc par
  // l'interface de cache figée. Hors de documentcontroller.* (socle, D-S8). Fil principal.
  class DocumentInsertions
  {
  public:
      explicit DocumentInsertions(DocumentController& controller);
      core::ObjectId insertObject(core::PageObject object);          // page affichée, une étape ; 0 si refusé
      core::ObjectId insertObjectOnNewPage(core::PageObject object); // page ajoutée en fin, affichée ; une étape
      core::ObjectId insertText(const QString& plainText, const QRectF& bounds); // couche intermédiaire, une étape
      // Image d'une ressource neuve (caméra, capture du spot) : ressource ajoutée et objet inséré en une étape (une
      // commande locale dont redo() appelle core::Document::addResource, puis insère l'objet).
      core::ObjectId insertImage(QByteArray data, std::string extension, core::ImageObject image, bool onNewPage);
      // core::ReplaceObjectsCommand, un groupe ; identifiants des ajouts.
      std::vector<core::ObjectId> replaceObjects(std::vector<core::ObjectId> removed,
                                                 std::vector<core::PageObject> added);
      void setPagePresentation(core::PagePresentation value); // core::SetPagePresentationCommand, page affichée
      const core::Page* shownPage() const noexcept;
      std::size_t shownIndex() const noexcept;

  private:
      DocumentController& m_controller;
  };

  } // namespace actcraft::ui
  ```
  `ui/src/presentationdocument.cpp` : `#include "presentationdocument.h"`, `// Talon L7-S8 : rempli par L7-T2.`, puis les
  corps les plus simples : constructeur qui pose `m_controller`, `return 0;` (identifiants), `return {};` (chaîne,
  vecteur), corps vide pour `setPagePresentation` (paramètre marqué `[[maybe_unused]]`), `return nullptr;`,
  `return 0;` pour `shownIndex` (sans conversion : aucun avertissement `-Wconversion`). Vérifié : `core::ObjectId`,
  `core::PageObject`, `core::PagePresentation` (`model.h:462-474`), `core::ImageObject` (`model.h:236`), `core::Page`
  dans `core/include/actcraft/core/model.h` ; `shownIndex()` rendra `static_cast<std::size_t>(m_controller.currentIndex())`
  (`documentcontroller.h:66`, `int`) en L7-T2.
- [ ] **Étape 4 : QML talons.** Les 17 racines (décision 3), contenu de la décision 4 mot pour mot, marqueur de leur
  tâche ; `PerPageTools.qml`, `TransitionLayer.qml`, `PageEffectsPicker.qml` avec leurs propriétés de lecture en
  plus ; `CameraOverlay.qml`, `ScreenKeyboard.qml`, `DesktopAnnotation.qml` (décision 4 bis) avec `visible: false`
  en plus (racine non visuelle ; la tâche y met son `Component { Window { … } }`). Les 10 non-racines :
  ```qml
  import QtQuick

  // Talon L7-S8 : rempli par L7-T<n>.
  Item {
  }
  ```
  sauf `FloatingPanel.qml` (talon de L7-T2) :
  ```qml
  import QtQuick

  // Talon L7-S8 : rempli par L7-T2.
  Item {
      id: panel
      property string title
      property Item pageArea: null
      default property alias content: body.data
      signal closed()
      Item {
          id: body
          anchors.fill: parent
      }
  }
  ```
- [ ] **Étape 5 : `Presentation.qml`.** Remplacer le contenu posé par L0-S9 en gardant **mot pour mot** sa racine
  `QtObject`, sa propriété `default property list<QtObject> data` et ses six propriétés (vérifié :
  `ui/qml/Presentation.qml:12-21` sur 131f790 ; son commentaire, l. 4-11, se résume dans celui ci-dessous).
  Seul `parent` est posé (aucun `anchors`, décision 3) ; les trois racines à fenêtre propre n'ont pas de `parent`
  (décision 4 bis) :
  ```qml
  import QtQuick
  import ActCraft.Ui

  // Racine des fonctions du lot 7 : une racine par fonction (contrôleur, surcouche ou panneau, gestionnaires de
  // commandes). Une surcouche qui prend l'entrée se déclare couverture : pageArea.view.addInputCover(<item>).
  // Seul le parent est posé ici : chaque racine décide de sa géométrie. Après L7-S8, ce fichier ne change plus.
  // Racine QtObject et non Item : la propriété `layer` figée par le socle est une propriété FINAL de Item. Les racines
  // sont déclarées dans `data` et posées par `parent:` ; jamais enfants visuels de cette racine.
  QtObject {
      id: presentation
      default property list<QtObject> data
      property Commands commands: null
      property DocumentController documentController: null
      property Tools tools: null
      property Item pageArea: null
      property Item pageOverlays: null
      property Item layer: null

      CurtainOverlay { id: curtainRoot; objectName: "curtainRoot"; z: 10; parent: presentation.pageOverlays
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      SpotlightOverlay { id: spotlightRoot; objectName: "spotlightRoot"; z: 20; parent: presentation.pageOverlays
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      PageZoomOverlay { id: pageZoomRoot; objectName: "pageZoomRoot"
          parent: presentation.pageArea ? presentation.pageArea.presentationSlot : null
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      PerPageTools { id: pageToolsRoot; objectName: "pageToolsRoot"
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea
          curtain: curtainRoot.controller; spotlight: spotlightRoot.controller }
      TransitionLayer { id: transitionRoot; objectName: "transitionRoot"; z: 10; parent: presentation.layer
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea; curtain: curtainRoot.controller }
      PageEffectsPicker { id: effectsRoot; objectName: "effectsRoot"; z: 50; parent: presentation.layer
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea; transitions: transitionRoot.controller }
      MathToolsLayer { id: mathToolsRoot; objectName: "mathToolsRoot"; z: 30; parent: presentation.pageOverlays
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      DicePanel { id: diceRoot; objectName: "diceRoot"; z: 30; parent: presentation.layer
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      CalculatorPanel { id: calculatorRoot; objectName: "calculatorRoot"; z: 31; parent: presentation.layer
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      ClockPanel { id: clockRoot; objectName: "clockRoot"; z: 32; parent: presentation.layer
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      TickerOverlay { id: tickerRoot; objectName: "tickerRoot"; z: 20; parent: presentation.layer
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      RecognitionAlternatives { id: recognitionRoot; objectName: "recognitionRoot"; z: 40; parent: presentation.layer
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      EquationEditor { id: equationRoot; objectName: "equationRoot"; z: 41; parent: presentation.layer
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      CameraOverlay { id: cameraRoot; objectName: "cameraRoot"
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      ScreenKeyboard { id: keyboardRoot; objectName: "keyboardRoot"
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      DesktopAnnotation { id: desktopRoot; objectName: "desktopRoot"
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
      OtherToolsHost { id: otherToolsRoot; objectName: "otherToolsRoot"
          commands: presentation.commands; documentController: presentation.documentController
          tools: presentation.tools; pageArea: presentation.pageArea }
  }
  ```
  `clang-format` ne met pas en forme le QML : garder ce texte tel quel ; si `qmlformat` fait partie de `format.ps1`,
  accepter sa sortie.
- [ ] **Étape 6 : aides de test.** `tests/support/presentationharness.h` (en-tête seul, `inline`, pour tous les
  `tst_ui_*` du lot) : `class MainWindow` (charge `Main.qml` hors écran comme `tests/tst_ui_shell.cpp` — reprendre son
  aide de chargement et `app/startup.cpp` à l'identique, lire le fichier ; son constructeur pose
  `QTest::failOnWarning(QRegularExpression(QStringLiteral("qrc:.*\\.qml")))` **avant** le chargement (revue, m6 :
  `shellLoadsWithoutQmlWarning` n'ouvre aucun panneau ; ainsi tout test du lot, qui ouvre et ferme ses panneaux et
  détruit la fenêtre à la fin, échoue sur un avertissement QML d'ouverture, de fermeture ou de démontage) ; `window()`, `item(objectName)`,
  `pageView()`, `pageObjectCount()` lu sur `fixedPage` comme les tests du socle), `enum class Pointer { Stylus,
  Finger, Mouse }`, `void addPointerRows()` (colonne `Pointer pointer`, lignes `stylet`, `doigt`, `souris`) et
  `void strokeAt(QQuickWindow* window, Pointer pointer, QPointF from, QPointF to, int steps = 8)` :
  ```cpp
  #pragma once

  // Aides des tests d'interface du lot 7 : fenêtre principale hors écran, trait injecté au stylet, au doigt ou à la
  // souris. Stylet : QTabletEvent envoyé à la fenêtre (le filtre de la vue le prend avant les éléments QML, socle
  // H-S1, tests/tst_pageview.cpp) ; doigt : QTest::touchEvent sur un périphérique tactile de test ; souris :
  // QTest::mousePress / mouseMove / mouseRelease. API publique de Qt seulement.
  #include <QPointF>
  #include <QPointingDevice>
  #include <QQuickItem>
  #include <QQuickWindow>
  #include <QTabletEvent>
  #include <QtTest>

  namespace presentationharness {

  enum class Pointer { Stylus, Finger, Mouse };

  inline void addPointerRows()
  {
      QTest::addColumn<Pointer>("pointer");
      QTest::newRow("stylet") << Pointer::Stylus;
      QTest::newRow("doigt") << Pointer::Finger;
      QTest::newRow("souris") << Pointer::Mouse;
  }

  // Périphériques construits comme ceux de tests/tst_ui_inputpath.cpp:220-224 (socle L0-S7) ; jamais
  // d'enregistrement par QWindowSystemInterface (API privée).
  inline const QPointingDevice* stylusDevice()
  {
      // Volontairement jamais détruit : un objet statique serait détruit après QGuiApplication (revue contre le socle).
      static const QPointingDevice* device = new QPointingDevice(
          QStringLiteral("tst-ui-lot7-stylus"), 5171, QInputDevice::DeviceType::Stylus, QPointingDevice::PointerType::Pen,
          QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
      return device;
  }

  inline QPointingDevice* touchDevice()
  {
      static QPointingDevice* device = QTest::createTouchDevice();
      return device;
  }

  // Comme tst_ui_inputpath.cpp:141-150 : bouton changé à l'appui et au relâché, aucun au déplacement.
  inline void sendTablet(QQuickWindow* window, QEvent::Type type, QPointF point)
  {
      const Qt::MouseButtons buttons = type == QEvent::TabletRelease ? Qt::NoButton : Qt::LeftButton;
      const Qt::MouseButton changed = type == QEvent::TabletMove ? Qt::NoButton : Qt::LeftButton;
      QTabletEvent event(type, stylusDevice(), point, window->mapToGlobal(point), buttons ? 0.5 : 0.0, 0, 0, 0, 0, 0,
                         Qt::NoModifier, changed, buttons);
      QCoreApplication::sendEvent(window, &event);
  }

  // Qt Quick retarde et compresse les TouchUpdate jusqu'à l'image suivante (tst_ui_inputpath.cpp:171-176) : une image
  // entre deux étapes.
  inline void settleTouch()
  {
      QTest::qWait(50);
  }

  inline void strokeAt(QQuickWindow* window, Pointer pointer, QPointF from, QPointF to, int steps = 8)
  {
      const auto at = [&](int i) { return from + (to - from) * (static_cast<qreal>(i) / steps); };
      switch (pointer) {
      case Pointer::Stylus:
          sendTablet(window, QEvent::TabletPress, from);
          for (int i = 1; i <= steps; ++i) {
              sendTablet(window, QEvent::TabletMove, at(i));
          }
          sendTablet(window, QEvent::TabletRelease, to);
          break;
      case Pointer::Finger:
          QTest::touchEvent(window, touchDevice()).press(0, from.toPoint());
          settleTouch();
          for (int i = 1; i <= steps; ++i) {
              QTest::touchEvent(window, touchDevice()).move(0, at(i).toPoint());
              settleTouch();
          }
          QTest::touchEvent(window, touchDevice()).release(0, to.toPoint());
          settleTouch();
          break;
      case Pointer::Mouse:
          QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, from.toPoint());
          for (int i = 1; i <= steps; ++i) {
              QTest::mouseMove(window, at(i).toPoint());
          }
          QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, to.toPoint());
          break;
      }
      QCoreApplication::processEvents();
  }

  // class MainWindow : chargement de Main.qml repris de tests/tst_ui_shell.cpp (lire le fichier, même montage) ;
  // window(), item(name), pageView(), pageObjectCount().

  } // namespace presentationharness

  Q_DECLARE_METATYPE(presentationharness::Pointer)
  ```
  Vérifié : la forme est celle de `tests/tst_ui_inputpath.cpp` (stylet l. 141-160 et 220-223, doigt l. 171-187 et
  224, souris l. 162-169), copiée et non incluse (les aides de ce test sont locales à son fichier).
- [ ] **Étape 7 : exécutable d'interface unique et tests talons** (décision 1, revue M5).
  `tests/support/lot7registry.h` :
  ```cpp
  #pragma once

  // Registre des classes de test d'interface du lot 7 : un fichier par fonction, un seul exécutable (tst_ui_lot7).
  // Chaque fichier inscrit sa classe sous une clé ; le main choisit la classe par sa première option.
  #include <QObject>

  #include <functional>
  #include <map>
  #include <memory>
  #include <string>

  namespace lot7 {

  using Factory = std::function<std::unique_ptr<QObject>()>;

  inline std::map<std::string, Factory>& registry()
  {
      static std::map<std::string, Factory> entries;
      return entries;
  }

  struct Registration
  {
      Registration(const char* key, Factory factory) { registry().emplace(key, std::move(factory)); }
  };

  } // namespace lot7

  #define ACTCRAFT_LOT7_TEST(key, Class) \
      static const lot7::Registration lot7Registration##Class(key, [] { return std::make_unique<Class>(); });
  ```
  `tests/tst_ui_lot7_main.cpp` (écrit une fois ; plus aucune tâche n'y touche) :
  ```cpp
  #include "support/lot7registry.h"

  #include <QGuiApplication>
  #include <QtQml/QQmlExtensionPlugin>
  #include <QtTest>

  #include <cstdio>
  #include <vector>

  // Module QML statique, comme tests/tst_ui_shell.cpp:30 : une fois pour tout l'exécutable.
  Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

  // Exécutable unique des tests d'interface du lot 7 : tst_ui_lot7 <clé> [options de Qt Test].
  int main(int argc, char** argv)
  {
      if (argc < 2 || lot7::registry().count(argv[1]) == 0) {
          std::fprintf(stderr, "tst_ui_lot7 : clé attendue parmi :");
          for (const auto& entry : lot7::registry()) {
              std::fprintf(stderr, " %s", entry.first.c_str());
          }
          std::fprintf(stderr, "\n");
          return 2;
      }
      const lot7::Factory& factory = lot7::registry().at(argv[1]);
      std::vector<char*> arguments{argv[0]};
      for (int i = 2; i < argc; ++i) {
          arguments.push_back(argv[i]);
      }
      int count = static_cast<int>(arguments.size());
      QGuiApplication app(count, arguments.data());
      std::unique_ptr<QObject> test = factory();
      return QTest::qExec(test.get(), count, arguments.data());
  }
  ```
  (`QTEST_MAIN` pose aussi des attributs d'application : les relire dans `qtest.h` de Qt 6.12 et reprendre ceux qui
  s'appliquent à `QGuiApplication` ; point de Qt, à vérifier à la construction, section « Vérifié contre le socle réel », point 15.)
  Les 16 `tests/tst_ui_<fonction>.cpp` hors `tst_ui_presentation`, sur ce modèle (classe, clé et
  `.moc` adaptés) :
  ```cpp
  #include "support/lot7registry.h"

  #include <QtTest>

  namespace {

  // Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
  class TstUiCurtain : public QObject
  {
      Q_OBJECT

  private slots:
      void reserved() { QSKIP("Talon L7-S8 : rempli par L7-T3."); }
  };

  } // namespace

  ACTCRAFT_LOT7_TEST("curtain", TstUiCurtain)
  #include "tst_ui_curtain.moc"
  ```
  Clés et tâches : `curtain` T3, `spotlight` T4, `pagezoom` T5, `pagetools` T6, `transitions` T7, `mathtools` T9,
  `dice` T14, `calculator` T15, `clock` T16, `othertools` T17, `ticker` T18, `recognition` T21, `equation` T26,
  `camera` T28, `keyboard` T29, `desktop` T30. `tst_core_equation.cpp` (T24), `tst_render_equation.cpp` (T25),
  `tst_features_equationeditor.cpp` (T26) : exécutables à eux, modèle du socle (`QTEST_GUILESS_MAIN` pour `core` et
  `features`, `QTEST_MAIN` pour `render`). Vérifié : ni `tests/tst_core_equation.cpp` ni `tests/tst_render_equation.cpp`
  n'existent sur 131f790 : L7-S8 les crée (k = 0).
- [ ] **Étape 8 : scénarios talons.** Les 9 `bench/scenarios/<nom>.json`, sur le modèle exact d'un scénario talon du
  socle (`bench/scenarios/selection.json`, L0-S0 : le lire et reprendre ses clés), avec `"name": "<nom>"` et
  `"description": "Talon L7-S8 : rempli par L7-T<n> (S<k>)."` — `rideau` T3 (S10), `spot` T4 (S10), `effets` T8
  (S22), `regle` T9 (S10), `compte-a-rebours` T16 (S10), `message` T19 (S23), `reconnaissance-forme` T21 (S10),
  `equation` T27 (S24), `presentation` T32 (S10) ; une seule étape `{"waitReady": true}`.
- [ ] **Étape 9 : `ui/presentation.cmake`.**
  ```cmake
  # Lot 7 (présentation et outils) : sources QML et C++ du lot, lues par ui/CMakeLists.txt (crochet du socle L0-S0,
  # avant qt_add_qml_module). Ce fichier ne fait que remplir les deux listes. Presentation.qml est déclaré par le
  # socle : ne pas le relister ici.
  set(ACTCRAFT_UI_LOT7_QML
      qml/FloatingPanel.qml
      qml/CurtainOverlay.qml
      qml/SpotlightOverlay.qml
      qml/PageZoomOverlay.qml
      qml/PerPageTools.qml
      qml/TransitionLayer.qml
      qml/PageEffectsPicker.qml
      qml/MathToolsLayer.qml
      qml/RulerTool.qml
      qml/SetSquareTool.qml
      qml/ProtractorTool.qml
      qml/CompassTool.qml
      qml/OriginMarker.qml
      qml/DicePanel.qml
      qml/CalculatorPanel.qml
      qml/ClockPanel.qml
      qml/CountdownDialog.qml
      qml/OtherToolsHost.qml
      qml/TickerOverlay.qml
      qml/TickerDialog.qml
      qml/RecognitionAlternatives.qml
      qml/EquationEditor.qml
      qml/CameraOverlay.qml
      qml/CameraPalette.qml
      qml/ScreenKeyboard.qml
      qml/DesktopAnnotation.qml
      qml/DesktopToolsWheel.qml)
  # Pas de QT_QML_SKIP_CACHEGEN : le socle tait C4702 dans le C++ de qmlcachegen de tous les QML du module, ceux du
  # lot 7 compris (ui/CMakeLists.txt, bloc « MSVC, C4702 »).

  # Sources C++ du lot par noms exacts : chaque tâche crée son contrôleur, CMake le voit à la construction suivante
  # (CONFIGURE_DEPENDS) ; aucun .cpp talon vide (LNK4221). Seuls ces noms sont pris, jamais un fichier du socle.
  file(GLOB ACTCRAFT_UI_LOT7_SOURCES CONFIGURE_DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/src/presentationdocument.h
      ${CMAKE_CURRENT_SOURCE_DIR}/src/presentationdocument.cpp
      ${CMAKE_CURRENT_SOURCE_DIR}/src/curtaincontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/spotlightcontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/pagezoomcontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/pagetoolscontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/transitioncontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/mathtoolscontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/dicecontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/calculatorcontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/clockcontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/presentationcommands.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/tickercontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/recognitioncontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/equationcontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/equationpreviewitem.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/cameracontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/keyboardcontroller.*
      ${CMAKE_CURRENT_SOURCE_DIR}/src/desktopcontroller.*)
  ```
  Vérifié : `ui/CMakeLists.txt:27-29` (`set(… "")` puis `include(… OPTIONAL)` avant `qt_add_qml_module`, l. 31),
  `${ACTCRAFT_UI_LOT7_QML}` en fin de `QML_FILES` (l. 60), `${ACTCRAFT_UI_LOT7_SOURCES}` en fin de `SOURCES`
  (l. 123), `ActCraft::features` lié (l. 147), `/wd4702` sur le C++ de qmlcachegen de tous les QML du module
  (l. 132-140, après `qt_add_qml_module` : il voit les QML du lot) ; `file(GLOB …)` remplace la valeur vide, c'est
  voulu. Reste à la construction de L7-S8 (point de CMake, pas du socle) : chemins absolus dans `SOURCES` acceptés
  (sinon `list(TRANSFORM … REPLACE "^${CMAKE_CURRENT_SOURCE_DIR}/" "")`).
- [ ] **Étape 10 : `tests/presentation.cmake`.** À la suite des 16 lignes de la phase 1 (inchangées) :
  ```cmake
  # Phase 2 (L7-S8) : tests d'interface, de logique de l'équation et de captures du lot 7, déclarés ici seulement ;
  # aucune autre tâche n'en ajoute (compte figé : N = N_F + 29). Les chemins d'inclusion privés dont les tâches ont
  # besoin sont posés ici une fois pour toutes (ui/src, render/src).
  set(_lot7_ui_keys presentation curtain spotlight pagezoom pagetools transitions mathtools dice calculator clock
                    othertools ticker recognition equation camera keyboard desktop)
  set(_lot7_ui_sources tst_ui_lot7_main.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp)
  foreach(_key IN LISTS _lot7_ui_keys)
      list(APPEND _lot7_ui_sources tst_ui_${_key}.cpp)
  endforeach()
  # Un seul exécutable pour les 17 classes (un lien, un PDB) ; 17 entrées ctest à leur nom.
  qt_add_executable(tst_ui_lot7 ${_lot7_ui_sources})
  target_link_libraries(tst_ui_lot7 PRIVATE Qt6::Test Qt6::Quick ActCraft::ui actcraft_uiplugin)
  target_include_directories(tst_ui_lot7 PRIVATE ${PROJECT_SOURCE_DIR}/app ${PROJECT_SOURCE_DIR}/ui/src
                                                 ${CMAKE_CURRENT_SOURCE_DIR})
  actcraft_set_warnings(tst_ui_lot7)
  # Même environnement que les tests d'interface du socle, lu et non recopié (tests/ui.cmake inclus avant).
  get_test_property(tst_ui_shell ENVIRONMENT _lot7_env)
  foreach(_key IN LISTS _lot7_ui_keys)
      add_test(NAME tst_ui_${_key} COMMAND tst_ui_lot7 ${_key})
      set_tests_properties(tst_ui_${_key} PROPERTIES ENVIRONMENT "${_lot7_env}")
  endforeach()

  actcraft_add_test(tst_core_equation SOURCES tst_core_equation.cpp LIBS ActCraft::core)
  actcraft_add_test(tst_render_equation SOURCES tst_render_equation.cpp LIBS ActCraft::render Qt6::Gui)
  target_include_directories(tst_render_equation PRIVATE ${PROJECT_SOURCE_DIR}/render/src)
  actcraft_add_test(tst_features_equationeditor SOURCES tst_features_equationeditor.cpp LIBS ActCraft::features)

  foreach(_lot7_scenario IN ITEMS rideau spot effets regle compte-a-rebours message reconnaissance-forme equation
                                  presentation)
      string(REPLACE "-" "_" _lot7_test "${_lot7_scenario}")
      add_test(NAME tst_captures_${_lot7_test}
          COMMAND $<TARGET_FILE:actcraft-captures>
              --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/${_lot7_scenario}.json
              --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-${_lot7_scenario})
      set_tests_properties(tst_captures_${_lot7_test} PROPERTIES
          TIMEOUT 300
          ENVIRONMENT "${_bench_env_offscreen}"
          PASS_REGULAR_EXPRESSION "SCENARIO OK"
          FAIL_REGULAR_EXPRESSION "ECART|SCENARIO EN ECHEC|SCENARIO INVALIDE")
  endforeach()
  ```
  L'environnement des 17 entrées est lu sur `tst_ui_shell` (`get_test_property`, même portée de répertoire) : il suit
  le socle sans copie ; si la propriété revient vide (`NOTFOUND`), s'arrêter et le rapporter. `LIBS` de `tst_ui_lot7` : alignés sur `tst_ui_shell` dans `tests/ui.cmake` (relu à la
  fusion) ; `tst_render_equation` : aligné sur `tst_render_painter` dans `tests/render.cmake` (le lire).
  `_bench_env_offscreen` est posé par `tests/bench.cmake` (l. 8), inclus avant ce fichier — vérifié :
  `tests/CMakeLists.txt:42-47` (`ui.cmake`, `bench.cmake`, `app.cmake`, puis `presentation.cmake` en dernière
  ligne) ; `tst_ui_shell` : `tests/ui.cmake:33-36` (`app/startup.cpp`, `Qt6::Quick ActCraft::ui actcraft_uiplugin`,
  inclusion de `app`). `ui/src` en inclusion du test : `DocumentInsertions`
  (L7-T2), contrôleurs construits par les tests (L7-T14, L7-T15, L7-T23, …) ; `render/src` : `layoutEquation`
  (L7-T25). Côté `ui/` (bibliothèque), aucune inclusion de `render/src` n'est nécessaire : voir L7-T26.
- [ ] **Étape 11 : vert et preuves.** Commande ciblée → vert (16 entrées `tst_ui_*` du lot en `SKIP` nommé,
  `tst_ui_presentation` `Totals: 3 passed, 0 failed, 1 skipped`, suites du socle `tst_ui_shell` — dont
  `shellLoadsWithoutQmlWarning` — et `tst_ui_inputpath` sans régression, 9 `tst_captures_*` Passed) ; `format.ps1` ;
  preuve de tâche (N = N_F + 29) ; « Fonctions d'un test d'interface » sur la clé `curtain`, puis sans clé (`code=2`,
  liste des 17 clés) ; preuve visuelle `premiers-pas` ; `git -C '<wt>' add` des fichiers de la ligne, nommés ;
  `NOMS OK`.
- [ ] **Étape 12 : commit.**
  ```powershell
  git -C '<wt>' diff --cached --stat
  git -C '<wt>' commit -m "Lot 7 : squelette de la phase 2 (racines des fonctions, QML talons, sources du lot, insertions, aides de test, exécutable d'interface unique, tests et scénarios réservés) (L7-S8)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```
  puis `NOMS OK` relancé après le commit.

### Lecture des sections amendées (lot P2-A et P2-B)

Chaque section ci-dessous a la même forme : **Garde** (étapes de 7a/7b qui restent, à exécuter telles quelles sauf
correction écrite ici), **Disparaît** (fait par le socle ou la phase 1 : ne pas le refaire), **S'ajoute** (rapport
L7-T0, socle, décisions de ce plan), puis objectif, fichiers, test, interdits, terminé quand, étapes. Partout :
`tst_ui_presentation` → `tst_ui_<fonction>` de la table ; « dans `Presentation.qml` » → « dans la racine » ;
`…BlocksTheStylus` → test à données (décision 5) ; fichiers de la décision 6 retirés ; N = N_F + 29.

<a id="tache-l7-t1"></a>
### Tâche L7-T1 — Objet MathML des `.flipchart` lu en équation (7a:437-737, réduite)

**Garde.** 7a, étape 8 (« objet MathML des `.flipchart` », test rouge `mathmlObjectBecomesEquation`) et son code.
**Disparaît.** Étapes 1 à 7, 9 (commandes, `ArgumentKind::Text`, modèle, `replace.*`, `SetPagePresentationCommand`,
format natif, repère neutre, fichiers réservés d'équation : L0-S3, L0-S4) ; `tst_core_presentation` ; étape 10
réduite à la preuve ci-dessous. **S'ajoute.** Rien du rapport L7-T0 (il confirme le MathML brut gardé, objet 460,
propriété 52 : utiliser ces identifiants, et `docs/re/format-flipchart.md:254`).

**Objectif.** Un objet équation d'un `.flipchart` devient un `core::Equation` dont `mathml` garde le texte brut.
**Fichiers.** Ligne de la table. **Test.** Commande ciblée `-R 'tst_formats_flipchart'` (rouge puis vert, PASS de
`mathmlObjectBecomesEquation`), preuve de tâche. **Interdits.** Tout `formats/` hors des quatre fichiers ; ne pas
toucher `nativejson.cpp` (L0-S3). **Terminé quand.** PASS nommé, N = N_F + 29, `TESTS OK`, `NOMS OK`.
**Étapes.** (1) Base et premier passage. (2) 7a, étape 8, test rouge. (3) Code de 7a, étape 8 ; vérifié : `core::Equation`
= `bounds`, `mathml`, `pointSize`, `color` (`core/include/actcraft/core/model.h:337-343`). (4) Vert, `format.ps1`, preuve, `NOMS OK`, commit
« Lot 7 : objet MathML des paperboards lu en équation (L7-T1) ».

<a id="tache-l7-t2"></a>
### Tâche L7-T2 — Insertions du lot, panneau flottant couverture (7a:739-906, réduite)

**Garde.** 7a, étape 5, seulement les règles d'`insertText` (`core::Text` dont le HTML est le texte échappé par
`QString::toHtmlEscaped`, police par défaut N2-3, couche intermédiaire) et d'`insertObjectOnNewPage` (page ajoutée en
fin et objet en **une** étape, commande composée locale au fichier, page affichée) ; 7a, étape 6, le texte de
`FloatingPanel.qml` (titre, fermeture, déplaçable par sa barre avec un `DragHandler`, borné à la fenêtre, jetons de
`Theme`). **Disparaît.** Étapes 1 à 3 (sous-menus, disposition : L0-S4), 4 et 5 pour `PageView` (filtre, zoom ancré,
`sceneToPage` : L0-S7, prouvés par `tst_ui_inputpath`), 6 pour `PageArea.qml` et `Main.qml` (L0-S9), 7 (CMake : L7-S8),
8 (`pointer`, `doubleClick` : L0-S2) ; `sampleFilterReachesThePen`, `zoomAroundKeepsThePointStill`,
`overlaySlotsExist` (remplacé par `lot7RootsExist`, L7-S8) ; `setStrokeObserver` (remplacé par
`DocumentController::addChangeObserver`, origine `Stroke`, D-S7). **S'ajoute.** Corps de `DocumentInsertions`
(`ui/src/presentationdocument.cpp`) par `m_controller.execute(…)` ; `FloatingPanel` couverture (décision 5) ;
`coveredStylusLeavesFingersFree`.

**Objectif.** Les insertions du lot passent par la pile du contrôleur, une étape chacune ; tout panneau flottant est
une couverture. `concepteur-critique` : le patron évident (appeler `DocumentController` puis bricoler l'étape) casse
l'invariant « une action = une étape d'annulation » et l'ordre des `Change` vus par le cache.
**Fichiers.** Ligne de la table. **Test.** Commande ciblée `-R 'tst_ui_presentation|tst_ui_document|tst_ui_inputpath'`.
**Interdits.** `documentcontroller.*` (socle) : on n'y ajoute rien ; si `execute()` ou `document()` ne sont pas
publics, s'arrêter et le rapporter (vérifié publics : `documentcontroller.h:76`, `:79`). **Terminé quand.** PASS des fonctions de la table, 0 skipped.
**Étapes.**
- [ ] **Étape 1 : tests rouges** dans `tests/tst_ui_presentation.cpp` (retirer `reserved()`), avec
  `presentationharness::MainWindow` et un `DocumentInsertions insertions(*controller)` sur le contrôleur de la fenêtre :
  `insertTextIsUndoneAtOnce` (`insertText("12", {100,100,80,40})` : id non nul, `pageObjectCount()` + 1, objet
  `Text` ; `undo` : compte de départ) ; `insertObjectOnNewPageIsUndoneAtOnce` (document de **3 pages**
  (`loadGeneratedPages(3)`), page **2 sur 3** affichée par `nextPage()` ; une forme : pages = 4, page affichée = 4,
  un objet ; un seul `undo` : 3 pages, puis `QTRY_COMPARE(currentIndex(), 1)` — page de départ) ; `replaceObjectsIsUndoneAtOnce` (un trait
  injecté au stylet → `replaceObjects({id}, {forme})` : un id rendu, le trait absent ; `undo` : le trait revient au même
  rang) ; `insertImageIsUndoneAtOnce` (une image PNG de 8 × 8 : un `ImageObject` de plus dont `resource` est non vide
  et présente dans `document().resources()` ; `undo` : objet retiré, clé absente de `document().resources()`,
  `documentController.fingerprint` égal à sa valeur avant l'insertion, et `formats::documentToJson(document())` sans
  entrée de cette clé dans `resources` ; `redo` : objet et ressource revenus ; une image déjà présente dans le
  document, insérée puis annulée, garde sa ressource) ; `setPagePresentationIsUndoneAtOnce` (`curtain = Top`, `curtainDistance = 120` : relu sur `shownPage()` ;
  `undo` : valeur par défaut ; aucun `cacheRefreshRequested`, I3) ; `changeObserverSeesEveryStroke` (observateur
  ajouté par `addChangeObserver` : deux traits au stylet → deux appels d'origine `ChangeOrigin::Stroke` dont les
  `core::Changes` portent chacun un `Change` de genre `ObjectInserted` avec l'identifiant du trait — vérifié :
  `ChangeObserver = std::function<void(const core::Changes& applied, ChangeOrigin origin)>`,
  `documentcontroller.h:145-161` ; `Change::Kind`, `Change::object`, `core/include/actcraft/core/undo.h:16-30`) ;
  `loadEmitsDocumentReplaced` (`QSignalSpy` sur `documentReplaced` : `loadGeneratedFirstPage`, `loadGeneratedPages(3)`
  et `replaceDocument` l'émettent une fois chacun, et l'observateur n'est pas appelé) ;
  `strokeDoesNotEmitDocumentReplaced` (un trait au stylet, `undo`, `redo`, `insertText` : `documentReplaced` jamais
  émis, alors que `documentChanged` l'est) ; `insertReadsIdsOnlyWhenApplied` (`replaceObjects` sur des identifiants
  absents et aucun ajout : vecteur vide rendu, aucun plantage, pile inchangée) ; `floatingPanelIsACover` (à données : un
  `FloatingPanel` de 200 × 120 ouvert sur la page, trait sous lui → compte inchangé et `DragHandler` de la barre
  `active` pendant le trait souris et doigt, panneau déplacé au stylet ; panneau fermé → le même trait est versé) ;
  `coveredStylusLeavesFingersFree` (stylet appuyé sur le panneau, un doigt trace hors du panneau → trait versé).
  Commande ciblée → rouge.
- [ ] **Étape 2 : code.** `documentcontroller.h` : `void documentReplaced();` ajouté à la fin des `signals:`, avec son
  commentaire ; `documentcontroller.cpp` : `emit documentReplaced();` à la fin de `replaceDocument`,
  `loadGeneratedFirstPage` et `loadGeneratedPages` (après leur `notifyState`) — trois lignes ajoutées, aucune modifiée
  (`git -C '<wt>' diff --stat` et `diff` relus : seulement des `+`). `presentationdocument.cpp` : marqueur retiré ;
  **identifiants lus seulement si la commande est entrée dans la pile** (revue, m4) : `execute()` détruit une commande
  sans changement (`documentcontroller.cpp:161-164`) ; garder un pointeur brut avant le `std::move`, comparer
  `core::fingerprint(document())` avant et après, et ne lire `id()` ou `addedIds()` que s'il a changé (sinon rendre 0
  ou vide sans toucher au pointeur) ; `insertObject` et `insertText` par
  `core::AddObjectCommand` sur `shownIndex()` ; `insertObjectOnNewPage` par la commande composée, puis affichage de
  la page ajoutée par la seule **voie publique** : `nextPage()` répété jusqu'à `currentIndex() == pageCount() - 1`
  (`show()` est privée, `documentcontroller.h:210` ; `nextPage` n'ajoute une page que sur la dernière,
  `core/include/actcraft/core/document.h`, `nextPageIndex` : la boucle s'arrête avant) ; `DocumentInsertions`
  retient l'indice de départ et, par un observateur (`addChangeObserver`, origine `Undo`, `Change` `PageRemoved` de
  cette page), revient à la page de départ par `pageBack()` répété, **différé** à la boucle d'événements
  (`QTimer::singleShot(0, &controller, …)` : jamais de navigation pendant l'appel de l'observateur) ; `replaceObjects` par
  `core::ReplaceObjectsCommand` (un `core::ObjectReplacement` qui retire `removed` et ajoute `added` en un groupe,
  `addedIds()` après `redo()`) — vérifié : `ObjectReplacement { removed; added; }` et le constructeur « un groupe »
  `ReplaceObjectsCommand(page, removed, added)`, `core/include/actcraft/core/replace.h:14-42` ; `setPagePresentation`
  par `core::SetPagePresentationCommand(shownIndex(), value)` (`replace.h:59-64`) ; `insertImage` par une commande
  locale au fichier (`redo` : `document.addResource(data, extension)`, clé dans `image.resource`, puis insertion de
  l'objet, sur une page neuve si `onNewPage` ; `undo` : retrait de l'objet, de la page s'il l'a ajoutée, et de la
  ressource **si sa clé était absente avant `redo`**, par `core::Document::removeResource(key)`). **Voie retenue**
  (revue contre le socle, M2), la plus petite après lecture : l'empreinte inclut toutes les ressources
  (`core/src/document.cpp:240-246`) et l'écriture native toutes aussi (`formats/src/nativejson.cpp:1585-1588`,
  `formats/src/nativefile.cpp:207-209`) ; élaguer à l'enregistrement toucherait trois endroits de `core/` et
  `formats/`, alors que `removeResource` est **un ajout** de quatre lignes dans `core/` (déclaration dans
  `document.h` à côté d'`addResource`, l. 66 ; `m_resources.erase(key)` dans `document.cpp`), aucune ligne
  existante modifiée — fichier du socle, signalé à A avec `documentReplaced`. Ainsi, après l'annulation, l'empreinte
  revient à sa valeur de départ et le fichier enregistré ne contient pas l'image (contenu possiblement privé d'une
  capture du bureau). `FloatingPanel.qml` : texte de 7a, plus `Component.onCompleted` et `onVisibleChanged` qui appellent
  `pageArea.view.addInputCover(panel)` / `removeInputCover(panel)` quand `pageArea` est posé (un panneau visible
  dès sa création s'enregistre aussi ; `floatingPanelIsACover` ouvre un panneau déjà visible à sa création).
- [ ] **Étape 3 : vert et preuves.** Commande ciblée verte, `format.ps1`, preuve, banc sans écran, `NOMS OK`, commit
  « Lot 7 : insertions du lot et panneau flottant couverture (L7-T2) ».

<a id="tache-l7-t3"></a>
### Tâche L7-T3 — Rideau (7a:908-1103)

**Placement** (décision 3, revue M2). `CurtainOverlay.qml` est dans `pageOverlays` (coordonnées de page, suit zoom et
défilement) ; la racine écrit `anchors.fill: parent`. **Disparaît aussi** : la règle de 7a (7a:928-930) selon
laquelle le rideau « ne passe pas par `pageOverlays` », et l'alignement par `mapToItem` de 7a, étape 4 (le store se
calcule en coordonnées de page, celles du scénario `rideau`).
**Garde.** Étapes 3 (contrôleur), 4 (surcouche ; `CurtainOverlay.qml` **est** la racine : son contenu de 7a y va, le
contrôleur `CurtainController { id: curtainController; … }` instancié dedans, `controller: curtainController`,
gestionnaire `present.curtain` (bascule, Ctrl+R) dans son `Component.onCompleted`), 5 (`curtainNeverRefreshesTheCache`),
6 (scénario `rideau`, talon remplacé), « stylet sous la surcouche », 7. **Disparaît.** Étapes 1 et 2 (L7-P1,
`tst_features_curtain`) ; `features/…` ; icône. **S'ajoute.** Couverture : `pageArea.view.addInputCover(<poignée du
rideau>)` à l'activation (le store opaque prend l'entrée **sur sa partie couvrante** par son `containmentMask`, socle
D-S1) ; `curtainBlocksTheStylus` à données ; la mémorisation pour la page (P9) va à L7-T6.

**Objectif.** Le rideau couvre la page sans redessin du cache et sans laisser tracer dessous. **Fichiers.** Ligne.
**Test.** Commande ciblée `-R 'tst_ui_curtain|tst_captures_rideau'`. **Preuve visuelle.** `rideau` hors écran,
`-Lire "curtain.coverage=…"` (valeur de 7a, étape 6) puis `PIXEL` couvert et découvert ; capture décrite.
**Interdits.** `CurtainOverlay.qml` ne lit pas `Curtain` hors de son contrôleur. **Terminé quand.** Preuve de la table.
**Étapes.** (1) Test rouge : `curtainNeverRefreshesTheCache` et `curtainBlocksTheStylus` dans `tst_ui_curtain`
(reserved retiré). (2) 7a étapes 3 et 4 dans la racine. (3) 7a étape 6. (4) Preuves, commit de 7a étape 7.

<a id="tache-l7-t4"></a>
### Tâche L7-T4 — Spot (7a:1105-1204)

**Placement.** Comme le rideau : racine dans `pageOverlays`, `anchors.fill: parent` dans la racine, masque en
coordonnées de page.
**Garde.** Étapes 3, 4, « stylet », 5 de 7a. **Disparaît.** Étapes 1 et 2 (L7-P1). **S'ajoute** (L7-T0, P7, P8 ;
phase 1) : `SpotlightController` convertit `Spotlight::Shape` ↔ `core::SpotlightShape` (mêmes valeurs,
`static_assert` par valeur) ; contour par défaut **`Spotlight::kDefaultOutline` (10)**, jamais 15 en dur ; curseur selon
la zone (`HoverHandler.cursorShape` : `Qt::SizeAllCursor` à l'intérieur, curseur de redimensionnement sur le bord) ;
**capture de la zone éclairée** depuis le menu propre du spot : `QQuickItem::grabToImage` de la zone de page sous le
spot, découpe par `cropToPolygon` (L7-P9), image codée en PNG et insérée par `DocumentInsertions::insertImage`
(une étape ; vérifié : `ImageObject::resource`, `model.h:236-247`).

**Objectif.** Spot à quatre formes, couverture, contour 10, curseurs par zone, capture vers la page. **Fichiers.**
Ligne. **Test.** `-R 'tst_ui_spotlight|tst_captures_spot'`. **Interdits.** Aucun `Spotlight` recréé.
**Terminé quand.** Preuve de la table. **Étapes.** (1) Tests rouges : `spotlightNeverRefreshesTheCache`,
`spotlightBlocksTheStylus` (à données), `spotlightOutlineIsTheDefault` (`outline` lu sur la surcouche = 10),
`spotlightCursorFollowsTheZone` (survol au centre puis au bord : deux `cursorShape` différents, lus par `objectName`
`spotlightHover`), `spotlightCaptureIsUndoneAtOnce` (spot carré sur un trait, capture : un `ImageObject` de plus ;
`undo` : retiré). (2) 7a étape 3 dans la racine, puis l'ajout. (3) 7a étape 4. (4) Preuves, commit de 7a étape 5.

<a id="tache-l7-t5"></a>
### Tâche L7-T5 — Zoom de page (7a:1206-1283)

**Garde.** Étapes 3, 4, 5. **Disparaît.** Étapes 1 et 2 (L7-P2) ; `zoomAround` et `sceneToPage` existent (L0-S7).
**S'ajoute** (socle « Changent » 6) : `static_assert(PageZoom::kMinZoom == ui::kMinZoom)` et `kMaxZoom` dans
`pagezoomcontroller.cpp` (vérifié : `ui::kMinZoom = 0.25`, `ui::kMaxZoom = 8.0`,
`ui/include/actcraft/ui/viewtransform.h:12-13`) ; l'appui long droit est pris par la couverture du zoom
(test de couverture direct au clic droit, socle D-S1) : `rightHoldIsTakenByTheCover` (zoom actif, appui droit tenu
600 ms par `QTest::mousePress` bouton droit : zoom diminué, `contextMenuRequested` non émis) ;
`pageZoomRefreshesTheCacheOnceAtRelease` : **exactement** 1 émission.

**Objectif.** Zoom continu ancré, panoramique, ajuster au double-clic, un redessin au relâché. **Test.**
`-R 'tst_ui_pagezoom'`. **Terminé quand.** Preuve de la table. **Étapes.** (1) Tests rouges (7a étape 4 et l'ajout,
`pageZoomBlocksTheStylus` à données). (2) 7a étape 3 dans la racine (gestionnaire `view.page-zoom`). (3) Preuves,
commit de 7a étape 5.

<a id="tache-l7-t6"></a>
### Tâche L7-T6 — Outils de page par page (7a:1285-1387)

**Garde.** Étapes 3 (application au changement de page, dans `PerPageTools.qml`, qui lit `curtain` et `spotlight`),
4 (lecture `.flipchart`, `pagePresentationRead`), 5. **Disparaît.** Étapes 1 et 2 (L7-P1). **S'ajoute** (phase 1,
L7-T0 P9 et constat 5 ; L7-T7 étape 5 déplacée ici) :
- `PageToolsSettings toPageToolsSettings(const core::PagePresentation&)` dans `features/…/pagetools.h/.cpp` :
  `PageToolsMode` → `PageToolChoice` par valeur vérifiée (`static_assert`) ; `CurtainMode::Off` et `Full` →
  `Curtain::Start::Full`, `Top/Bottom/Left/Right` → même côté ; `spotlightFrame` recopié. Tests dans
  `tst_features_pagetools` : `curtainModeOffMeansFull`, `conversionKeepsEveryValue` (chaque valeur des cinq champs).
- **Propriétés non codées 13, 18, 29** (distance du rideau, cadre du spot, origine : un nombre, un rectangle, un
  point) : lues telles quelles (7a, étape 4, `pagePresentationRead`).
- **Propriétés codées 12, 16, 27, 30** (mode du rideau, mode du spot, outils de page F-591, effet propre à la page ;
  décisions de l'orchestrateur, revues tours 2 et 3). Leurs codes ne sont établis nulle part : le relevé REA de L7-T0
  (`REA - Activinspire/docs/re/lots/lot7.md:108-110`) et le rapport L7-T0 (constat 5) ne donnent que les numéros et
  les noms, l'ancien dépôt (`app/actcraft/io/_schema.py`) que le nom des types, `docs/re/format-flipchart.md:157`
  que les noms. **Étape 0 de la tâche, avant tout test** : chercher, pour chacune, une correspondance valeur → mode.
  - **Ce qui établit une valeur** : seulement une source **indépendante du fichier lu** — un relevé de L7-T0 dans le
    binaire, une documentation, ou l'ordre d'une énumération lisible en statique. **Jamais** une déduction tirée
    d'autres propriétés du même fichier (par exemple « 27 = 2 sur les pages où 16 n'est pas nul, donc 2 = spot »).
    Pour la propriété 27, la correspondance doit **distinguer `Keep` de `Off`** (une page qui ne montre rien ne
    sépare pas les deux) ; à défaut, aucune valeur de 27 n'est établie.
  - Les `.flipchart` réels de `C:\Users\JeromeDenis\REA - Activinspire\samples\` peuvent être lus (lecture seule,
    programme d'essai hors dépôt dans `.tmp/`, lecteur existant) pour **voir quelles valeurs brutes apparaissent**,
    jamais pour leur attribuer un sens. Jamais d'écriture dans `samples\` ; aucun nom de paperboard ni chemin
    d'échantillon dans le dépôt, **ni dans le rapport de la tâche, ni dans la PR** ; aucune donnée copiée.
  - Valeur établie : constante nommée à nous dans `formats/src/flipchartids.h`, commentée par sa source
    indépendante ; `pagePresentationRead` (12, 16, 30) et `pageToolsPropertyRead` (27) ne testent **que** ces
    valeurs (`.flipchart` synthétique par `tests/support/flipchartwriter.h`). Valeur non établie : lue comme la
    valeur par défaut du champ (`CurtainMode::Off`, `SpotlightShape::Off`, `PageToolsMode::Keep`, effet vide), avec
    un avertissement de lecture (7a:1298-1300), gardée sans perte (rien n'est réécrit dans le `.flipchart`) ; aucun
    test ne la relit.
  - Aucune valeur établie pour une propriété : sa lecture codée **sort** de L7-T6 (aucune constante, aucun test), et
    elle entre dans l'hypothèse **H7-16**, présentée à la porte de fin du lot. Jamais un test qui relit ses propres
    suppositions.
  - Le rapport dit, par propriété, les valeurs établies et leur source, ou « non établie » ; jamais un nom
    d'échantillon.
- **Mémoriser le rideau pour la page** (P9) : entrée « Mémoriser pour cette page » du menu propre du rideau (décision
  8), qui pose `curtain` et `curtainDistance` courants par `DocumentInsertions::setPagePresentation` (une étape) ;
  `rememberCurtainIsUndoneAtOnce` (rideau tiré de 120 px depuis le haut, mémoriser : relu `Top`, 120 ; `undo` : défaut).
- Sous-clés inconnues de `"presentation"` : rien à faire (gardées par `unknownKeys`, `model.h:473`).
- Décision 13 : `PerPageTools` applique les outils de page au changement de page (7a, étape 3) et sur
  `documentReplaced` (L7-T2), jamais sur `documentChanged` ; `openedDocumentAppliesPageTools` (document de deux pages
  chargé par `replaceDocument`, première page en mode rideau : le rideau est actif sans aucun appel de
  l'observateur) ; `strokeKeepsTheLiveCurtain` (rideau tiré à 300 px sans le mémoriser, puis un trait au stylet hors
  du rideau : la distance vivante reste 300 px).

**Objectif.** Les outils de page suivent la page, lus et mémorisés. `concepteur-critique` : parité de deux chemins
(lecture `.flipchart` et natif) sur la même structure. **Test.** `-R 'tst_features_pagetools|tst_formats_flipchart|tst_ui_pagetools'`.
**Interdits.** `nativejson.cpp` ; aucune nouvelle sous-clé native. **Terminé quand.** Preuve de la table.
**Étapes.** (0) Correspondances des propriétés codées 12, 16, 27, 30 par source indépendante (ci-dessus). (1) Tests rouges (logique,
`.flipchart`, interface). (2) Conversion. (3) 7a étape 3 dans la racine ; entrée « Mémoriser pour cette page » dans
`CurtainOverlay.qml`. (4) 7a étape 4 : propriétés 13, 18, 29, et valeurs établies des propriétés codées seulement. (5) Preuves, commit de 7a étape 5.

<a id="tache-l7-t7"></a>
### Tâche L7-T7 — Effets de changement de page I (7a:1389-1500)

**Garde.** Étapes 3 (lecteur ; `TransitionLayer.qml` est la racine ; les gestionnaires `page.next` et `page.previous`
sont **enveloppés dans la racine**, plus dans `Presentation.qml`), 4, 6. **Disparaît.** Étapes 1 et 2 (L7-P2) ;
étape 5 (lecture `.flipchart` de l'effet : L7-T6). **S'ajoute.** Effet de la page quittée lu dans
`PagePresentation::transition`, puis `pickEffect` pour `random` ; identifiants du catalogue de 18 (`dissolve`,
`slide-right`…, plus `fade` ni `slide-left`) ; un mouvement QML par identifiant (déduit du nom, rapport L7-T0 :
« Mouvement » à préciser à l'écran à la clôture) ; « aucun effet pendant le rideau » lit `curtain.active`.

**Test.** `-R 'tst_ui_transitions'`, PASS de la table (dont `pageEffectReadFromThePresentation` : page 0 avec
`transition = "dissolve"`, `page.next` → `transition.played == "dissolve"` ; `noEffectWhileCurtainActive`).
**Interdits.** L'enveloppe ne change pas le comportement de `page.next` hors effet (`tst_ui_shell` vert).
**Terminé quand.** Preuve de la table. **Étapes.** (1) Tests rouges. (2) 7a étape 3 dans la racine. (3) Preuves,
commit de 7a étape 6.

<a id="tache-l7-t8"></a>
### Tâche L7-T8 — Effets II : navigateur, scénario (7a:1502-1541)

**Garde.** Étapes 1, 3, 4, 5 ; identifiants du scénario `effets` déjà amendés dans 7a (`view.page-effect dissolve`,
`page.effect slide-right`, `LU transition.played=dissolve`). **Change.** `EffectPicker.qml` → **`PageEffectsPicker.qml`**
(nom imposé ; racine posée par L7-S8, `parent: presentation.layer`, `transitions` lu) ; un délégué par entrée du
catalogue (18) ; le panneau est un `FloatingPanel` (couverture). **Test.** `-R 'tst_ui_transitions|tst_captures_effets'`.
**Preuve visuelle.** `effets` hors écran, `-Lire "transition.played=dissolve"`, puis capture décrite.
**Terminé quand.** Preuve de la table. **Étapes.** (1) 7a étape 1 dans `tst_ui_transitions` + `pageEffectsPickerBlocksTheStylus`.
(2) 7a étape 2 dans `PageEffectsPicker.qml`. (3) 7a étape 3 (talon remplacé). (4) Preuves, commit de 7a étape 5.

<a id="tache-l7-t9"></a>
### Tâche L7-T9 — Règle et guides de tracé (7a:1543-1728)

**Garde.** Étapes 3 (contrôleur), 4 (`MathToolsLayer.qml` racine dans `pageOverlays`, gestionnaires `math.*` dans la
racine), 5, 6. **Disparaît.** Étapes 1 et 2 (L7-P3 : `StrokeGuide`, `GuideSet`, `Ruler`, `projectOnSegments`).
**S'ajoute.** `MathToolsController` branche `GuideSet::filter` sur `PageView::setSampleFilter` (tolérance = 16 px
d'écran ÷ zoom ; relâchés synthétisés filtrés depuis le dernier point brut, socle « Changent » 8) et range les outils
ouverts par `ToolHost::user().setOpenMathTools` (`ToolHost*` reçu de la racine, `tools`) — vérifié :
`tools::UserState& user(input::UserId = kFirstUser)`, `ui/include/actcraft/ui/toolhost.h:92` ;
`void setOpenMathTools(std::vector<QString> ids)`, `tools/include/actcraft/tools/userstate.h:39` (identifiants des
commandes `math.*`) ; translucidité par défaut `kDefaultToolOpacity` (100 sur 255, L7-T0) ;
curseurs par zone (P21) : déplacer au centre, tourner à l'extrémité, allonger au bout ; **fermer la règle garde les
traits** (P6) ; chaque outil est une couverture **hors de son bord guide** (`containmentMask` : le stylo se pose sur le
bord extérieur, 7a étape 4).

**Test.** `-R 'tst_ui_mathtools|tst_captures_regle|tst_ui_inputpath'`. **Preuve visuelle.** `regle` hors écran,
lecture de 7a étape 5 ; capture décrite. **Interdits.** Le stylo et `pageview.*` ne changent pas. **Terminé quand.**
Preuve de la table. **Étapes.** (1) Tests rouges (7a étape 5 et ajouts : `closingTheRulerKeepsTheStroke`,
`openToolsKeptInTheUserState`, `mathToolOpacityIsTheDefault`, `mathToolCursorFollowsTheZone`, `rulerBlocksTheStylus`
à données — au stylet **hors** du bord guide). (2) 7a étapes 3 et 4. (3) Preuves, commit de 7a étape 6.

<a id="tache-l7-t10"></a>
### Tâche L7-T10 — Équerre (7a:1730-1772)

**Garde.** Étapes 3, 4, 5. **Disparaît.** Étapes 1 et 2 (L7-P3, `SetSquare`). **S'ajoute.** Translucidité, curseurs
par zone et couverture comme L7-T9 ; `setSquareBlocksTheStylus` à données. **Test.** `-R 'tst_ui_mathtools'`.
**Terminé quand.** Preuve de la table. **Étapes.** (1) Tests rouges. (2) 7a étape 3. (3) Preuves, commit de 7a.

<a id="tache-l7-t11"></a>
### Tâche L7-T11 — Rapporteur (7a:1774-1832)

**Garde.** Étapes 3, 4, 5. **Disparaît.** Étapes 1 et 2 (L7-P4 : `Protractor`, `arcPath`, `ArcStyle`). **S'ajoute.**
Arc fermé ou plein inséré par `DocumentInsertions::insertObject` (une étape : `closedArcIsUndoneAtOnce`) ; rotation
par une poignée (P21 : `protractorTurnsByItsHandle`, glisser la poignée de 90° → `rotation` lue 90 ± 1) ; curseurs,
translucidité, couverture. **Test.** `-R 'tst_ui_mathtools'`. **Étapes.** (1) Tests rouges. (2) 7a étape 3.
(3) Preuves, commit de 7a.

<a id="tache-l7-t12"></a>
### Tâche L7-T12 — Compas (7a:1834-1893)

**Garde.** Étapes 3, 4, 5. **Disparaît.** Étapes 1 et 2 (L7-P4, `Compass`). **S'ajoute** (P5) : quand la pointe du
stylo approche du porte-mine (rayon de capture du guide), une mine apparaît **à la couleur du stylo** (`tools` lu :
couleur de l'outil courant, vérifié : `Q_PROPERTY(QColor color …)` de `Tools`, `toolhost.h:42`) : `compassLeadTakesThePenColour`
(stylo rouge, survol au stylet du porte-mine : élément `compassLead` visible, `color` = rouge) ; croix du centre par
`insertObject` ; couverture. **Test.** `-R 'tst_ui_mathtools'`. **Étapes.** (1) Tests rouges. (2) 7a étape 3 et la
mine. (3) Preuves, commit de 7a.

<a id="tache-l7-t13"></a>
### Tâche L7-T13 — Origine XY (7a:1895-1924)

**Garde.** Étapes 3, 4. **Disparaît.** Étapes 1 et 2 (L7-P4, `OriginMemory`). **S'ajoute.** `OriginMemory` alimentée
depuis `PagePresentation::hasOrigin` / `origin` au changement de page ; au relâché du marqueur,
`DocumentInsertions::setPagePresentation` (une étape : `originMoveIsUndoneAtOnce`) ; mémoire relue sur
`documentReplaced` (décision 13) : `originReadFromAnOpenedDocument` ; `strokeKeepsTheOrigin` (origine déplacée puis un
trait : l'origine affichée ne bouge pas) ; `OriginMarker.qml` (nom imposé)
couverture. Les symétries autour de l'origine (P20) vont au lot 5 : rien ici ni en L7-T34. **Test.**
`-R 'tst_ui_mathtools'`. **Étapes.** (1) Tests rouges. (2) 7a étape 3. (3) Preuves, commit de 7a étape 4.

<a id="tache-l7-t14"></a>
### Tâche L7-T14 — Dés (7a:1926-1995)

**Garde.** Étapes 3 (dans la racine `DicePanel.qml`), 4, « stylet », 5. **Disparaît.** Étapes 1 et 2 (L7-P5). **S'ajoute**
(E5, P4) : 1 dé au premier lancement, puis nombre et vitesse **mémorisés** dans `QSettings` (`tools/dice/count`,
`tools/dice/speed`, réécrits à chaque changement) : `oneDieTheFirstTime` (réglages vides → `count == 1`),
`diceCountAndSpeedRemembered` (régler 4 et 9, recréer le contrôleur → 4 et 9) ; croix de fermeture
(`dicePanelClosesByItsCross`) ; total écrit par `DocumentInsertions::insertText`. Les tests isolent `QSettings`
(`QStandardPaths::setTestModeEnabled(true)` et organisation de test). **Test.** `-R 'tst_ui_dice'`.
**Étapes.** (1) Tests rouges. (2) 7a étape 3 et réglages. (3) Preuves, commit de 7a.

<a id="tache-l7-t15"></a>
### Tâche L7-T15 — Calculatrice (7a:1997-2067)

**Garde.** Étapes 3, 4, « stylet », 5. **Disparaît.** Étapes 1 et 2 (L7-P5). **S'ajoute** (P1, P2, P3) : deux lignes
(`expression()` au-dessus de `display()`), touches « C » (tout le calcul) et « CE » (dernière saisie), bouton
« Poser sur la page » qui insère `pageText()` par `DocumentInsertions::insertText` (une étape) ; glisser vers la page :
L7-T32 (plan de phase 1, « Ce que la phase 2 reprend » : « 7b, P3 » ; revue M6). **Test.** `-R 'tst_ui_calculator'`, PASS de la table
(`panelShowsTheCalculationAboveTheResult` : saisir 12 + 3 → ligne haute « 12 + 3 », ligne basse « 3 » puis « 15 »
après « = » ; `clearEntryAndClearAllAreTwoKeys` ; `calculationToPageIsUndoneAtOnce`). **Étapes.** (1) Tests rouges.
(2) 7a étape 3 et ajouts. (3) Preuves, commit de 7a.

<a id="tache-l7-t16"></a>
### Tâche L7-T16 — Horloge et compte à rebours (7a:2069-2159)

**Garde.** Étapes 3, 4, « stylet », 5. **Disparaît.** Étapes 1 et 2 (L7-P5). **S'ajoute.** **H7-14** (décision 11) :
`endActionsAreTheThreeOfThisLot` (la liste des actions de fin vaut exactement « aucune », « page suivante »,
« page précédente », et « rétablir la page » n'y est **pas** : elle n'existe qu'après le lot 3 ; la spécification en
compte quatre, L7-T34 ajoute la quatrième et remplace ce test par `endActionsAreTheFourOfTheSpecification`) ; panneau et boîte en `FloatingPanel` (couverture). Le son de fin
reste au lot 8 (H7-4). **Test.** `-R 'tst_ui_clock|tst_captures_compte_a_rebours'`. **Preuve visuelle.**
`compte-a-rebours` hors écran, `-Lire "pageIndicator.text=2 / 2"` après 3 s. **Étapes.** (1) Tests rouges. (2) 7a
étape 3. (3) Scénario (talon remplacé). (4) Preuves, commit de 7a.

<a id="tache-l7-t17"></a>
### Tâche L7-T17 — Date et heure, navigateur web (7a:2161-2187)

**Garde.** Étapes 1, 2, 3 ; gestionnaires `insert.date-time` et `tool.web-browser` dans **`OtherToolsHost.qml`**
(racine), `PresentationCommands` instancié dedans. **Change.** `insertText` par `DocumentInsertions`. Si
`PresentationCommands` doit être joint ailleurs que dans sa racine, `Presentation.qml` peut recevoir une ligne : la
tâche l'ajoute alors à sa ligne de table avant lancement (décision 1). **Test.** `-R 'tst_ui_othertools'`.
**Étapes.** Celles de 7a, tests dans `tst_ui_othertools`.

<a id="tache-l7-t18"></a>
### Tâche L7-T18 — Message défilant I (7a:2189-2257)

**Garde.** Étapes 3 (racine `TickerOverlay.qml`, bande pleine largeur dans `presentation.layer`, `XAnimator` sur le
fil de rendu), 4, « stylet », 5. **Disparaît.** Étapes 1 et 2 (L7-P6). **S'ajoute.** Réglages `QSettings`
`tools/ticker` ; couverture limitée à la bande (`containmentMask`). **Test.** `-R 'tst_ui_ticker'`. **Étapes.**
(1) Tests rouges. (2) 7a étape 3. (3) Preuves (banc avec message en boucle), commit de 7a.

<a id="tache-l7-t19"></a>
### Tâche L7-T19 — Message défilant II (7a:2259-2293)

**Garde.** Étapes 2 (côté contrôleur et boîte ; `toJson` / `fromJson` faits par L7-P6, sous les noms
`tickerToJson` / `tickerFromJson`), 3, 4. **Disparaît.** Étape 1 (`messageFileRoundTrip` : L7-P6). **S'ajoute.**
**H7-15** : bouton « Essayer » (montre sans garder : `tryDoesNotKeepTheDefault`) et « Appliquer » (garde comme défaut
et ferme : `applyKeepsTheDefault`) ; charger un fichier de messages depuis la boîte (`messageFileLoadedFromTheDialog`) ;
boîte en `FloatingPanel`. **Test.** `-R 'tst_ui_ticker|tst_captures_message'`. **Étapes.** (1) Tests rouges.
(2) 7a étape 2. (3) Scénario `message` (talon remplacé). (4) Preuves, commit de 7a.

<a id="tache-l7-t21"></a>
### Tâche L7-T21 — Reconnaissance de forme II (7b:269-342)

**Garde.** Étapes 3 (contrôleur, instancié dans la racine `RecognitionAlternatives.qml`, gestionnaires
`tool.shape-recognition` et `recognition.convert-to-shape` dans la racine), 4, 5. **Disparaît.** Étapes 1 et 2
(L7-P7 : `RecognitionSession`, ses tests dans **`tst_features_recognitionsession`**, plus dans
`tst_features_shapefinder` : preuve corrigée) ; dépendance à L7-T20 (faite). **S'ajoute.** Délai commun
`RecognitionSession::kDefaultDelayMs` (1 s), réglable de 0 à 5 s (L7-T0 ; 7b disait 5 s, 1 à 5) : 0 = au relâché,
**supposé** (non déterminable en statique, à revoir à la porte) ; l'original ne convertit pas au relâché par défaut ;
`RecognizedShape::Kind::Polygon` rangé en `core::Shape` de genre `ShapeKind::Polygon`, ses sommets dans
`Shape::polygon` (vérifié : `model.h:200-212`) ; session remise à zéro sur `documentReplaced` seulement (décision 13 :
`sessionResetsOnAnOpenedDocument`) ; `strokeKeepsThePendingRecognition` (un cercle tracé, puis un second trait avant
la fin du délai : les deux traits restent dans la session, la reconnaissance part au délai) ; remplacement par `DocumentInsertions::replaceObjects` ; traits suivis par `addChangeObserver`
(origine `Stroke`). Noms : `recognizedStrokeMakesOneStep` (au lieu de `…BecomesAShapeInOneStep`).
**Test.** `-R 'tst_ui_recognition|tst_captures_reconnaissance_forme'`. **Étapes.** (1) Tests rouges (7b étape 4 et
ajouts). (2) 7b étape 3 dans la racine. (3) 7b étape 5 (talon remplacé), preuves, commit.

<a id="tache-l7-t23"></a>
### Tâche L7-T23 — Reconnaissance d'écriture II (7b:425-457)

**Garde.** Étapes 1 à 3. **Disparaît.** Le fil dédié STA (`HandwritingThread`, L7-P8) et
`recognitionRunsOnOneDedicatedThread` (dans `tst_features_handwriting`) : retirés de la preuve. **S'ajoute.** Le
contrôleur en mode écriture **possède** un `HandwritingThread` (aucun autre fil, aucun `moveToThread` à écrire) :
`controllerOwnsOneHandwritingThread` ; commande grisée par `probe()` ; clic hors du texte = validation (P19,
`clickOutsideValidatesTheText`) ; un objet texte par mot coupé à la pause (P17, `eachWordBecomesItsOwnText` : deux mots factices séparés d'une
pause → deux objets `Text`) ; alternatives en `FloatingPanel`
(couverture, `alternativesBlockTheStylus`). Nom : `handwritingMakesOneStep`. **Test.** `-R 'tst_ui_recognition'`.
**Étapes.** Celles de 7b, tests dans `tst_ui_recognition`.

<a id="tache-l7-t24"></a>
### Tâche L7-T24 — Équation I : arbre et MathML (7b:459-607)

**Garde.** Étapes 1 à 3. **Change.** Fichiers créés et enregistrés par le socle (vérifié : `core/equation.*` de
L0-S3 ne portent que le marqueur, l. 2 ; y écrire l'arbre sans toucher à `struct Equation`, `model.h:337-343`) ; `tst_core_equation` déclaré par
L7-S8. **S'ajoute** (L7-T0, « Modèles d'équation retenus ») : `templatesRoundTrip` à 17 lignes de données, les 10
formes du rapport (`fraction`, `power`, `subscript`, `square-root`, `nth-root`, `parentheses`, `sum`, `integral`,
`matrix-2x2`, `greek`) plus les 7 proposées (valeur absolue, matrices 1 × 2, 2 × 1 et 3 × 3, intégrales double et
triple, fonction trigonométrique), MathML écrit et relu à l'identique. **Talon du socle** : retirer le marqueur
`// Talon L0-S : rempli par L7-T24.` de `core/include/actcraft/core/equation.h` et `core/src/equation.cpp` (posé par
L0-S3) ; preuve : `git -C '<wt>' grep -n "Talon L0-S : rempli par L7-T24" -- . ':(exclude)docs/'` vide.
**Test.** `-R 'tst_core_equation|tst_core_model'`.

<a id="tache-l7-t25"></a>
### Tâche L7-T25 — Équation II : mise en page et dessin (7b:609-650)

**Garde.** Étapes 1 à 3. **Change.** `render/src/equationlayout.*` créés par L0-S3 avec le repère neutre (vérifié :
déclaration privée `paintEquation(QPainter&, const core::Equation&)`, `render/src/equationlayout.h:10` ; marqueur
`render/src/equationlayout.cpp:1`) :
remplacer le repère par la mise en page en gardant `malformedDrawsTheNeutralFrame` ; `tst_render_equation` déclaré par
L7-S8, avec `render/src` en inclusion privée (L7-S8, étape 10 : `layoutEquation` s'y lit) ; les formes ajoutées
par L7-T24 se dessinent (une ligne de données chacune dans `paintedMatchesSoftware`). **S'ajoute** (décision de
l'orchestrateur, revue tour 3, I3 ; forme décidée par A) : un **en-tête public neuf**
`render/include/actcraft/render/equationpaint.h` :
```cpp
#pragma once

#include <actcraft/core/model.h>

class QPainter;

namespace actcraft::render {

// Dessin d'une équation (F-301). Page : le rendu de la page et de l'impression. Editor : en plus, les cases vides et
// le curseur de l'éditeur (marques actcraft-placeholder et actcraft-cursor du MathML).
enum class EquationPaintMode { Page, Editor };

void paintEquation(QPainter& painter, const core::Equation& equation, EquationPaintMode mode = EquationPaintMode::Page);

} // namespace actcraft::render
```
La déclaration privée de `render/src/equationlayout.h` (ligne 10 à be2e5de) est **retirée** ; `equationlayout.h` ne
garde que la mise en page (`EquationBox`, `layoutEquation`) et **inclut** `<actcraft/render/equationpaint.h>`.
Vérifié : seul `render/src/pagepainter.cpp:1` inclut `equationlayout.h` ; `paintedrenderer.cpp` ne l'inclut pas. Par
cette inclusion, `pagepainter.cpp` voit la déclaration publique et appelle `paintEquation` en mode `Page` (argument
par défaut) **sans être modifié** : ni `pagepainter.cpp` ni `paintedrenderer.cpp` n'entrent dans les fichiers de la
tâche. `render/CMakeLists.txt` : la seule ligne
`include/actcraft/render/equationpaint.h` ajoutée à la liste, juste avant `src/equationlayout.h` (vérifié :
`render/CMakeLists.txt:27-32`). En mode `Editor` seulement, la case marquée `class="actcraft-placeholder"` se dessine en cadre pointillé
et la case marquée `class="actcraft-cursor"` se dessine surlignée. En mode `Page`, un `<mrow/>` vide, marqué ou non,
ne dessine rien, et les marques sont ignorées. Tests : `editorModeDrawsPlaceholderAndCursor` ;
`pageDrawsNoPlaceholder` (rendu de page de `<mfrac><mrow/><mrow/></mfrac>`, avec et sans marques : aucun pixel non
blanc hors de la barre de fraction). Dans `tests/tst_render_painter.cpp` (décision de A) : `pageModeIsUnchanged`
(pour les équations du test existant `equationDrawsTheNeutralFrame` et une fraction complète, l'image de
`paintObjectLocal` est identique pixel à pixel à celle de `paintEquation(…, EquationPaintMode::Page)` et à celle
d'avant la tâche, relevée à l'étape rouge) ; `editorModeShowsAnEmptyBox` (`<mfrac><mrow class="actcraft-placeholder"/>
<mrow/></mfrac>` en mode `Editor` : au moins un pixel du pointillé dans la case vide ; le même en mode `Page` : aucun).
**Talon du socle** :
retirer `// Talon L0-S : rempli par L7-T25.` de `render/src/equationlayout.cpp` (et de `equationlayout.h` s'il y est) ;
preuve : `git -C '<wt>' grep -n "Talon L0-S : rempli par L7-T25" -- . ':(exclude)docs/'` vide. **Test.**
`-R 'tst_render_equation|tst_render_painter'`.

<a id="tache-l7-t26"></a>
### Tâche L7-T26 — Équation III : éditeur à modèles (7b:652-738)

**Garde.** Étapes 1 à 5. **Change.** `equationeditor.*` créés par la tâche (pris par le motif de `features/`) ;
`tst_features_equationeditor` déclaré par L7-S8 ; `EquationEditor.qml` racine (gestionnaire `insert.equation`) ;
tests d'interface dans `tst_ui_equation`. **S'ajoute** (L7-T0) : palettes de l'éditeur par catégorie, en nos mots
(fractions, racines, indices et exposants, au-dessus et au-dessous, accents, délimiteurs, grands opérateurs,
intégrales, matrices, fonctions, relations, lettres grecques) ; **aperçu en mode éditeur** (revue tour 3, I3) :
`EquationPreview` n'inclut que `<actcraft/render/equationpaint.h>` (L7-T25) et appelle
`paintEquation(…, EquationPaintMode::Editor)` ; aucun en-tête de `render/src` dans `ui/` (`git grep -n
"equationlayout.h" -- ui/` vide). `previewMatchesThePageWithoutMarks` (l'image de l'aperçu d'une fraction complète,
sans marque, est identique pixel à pixel à celle de `paintEquation(…, EquationPaintMode::Page)` sur le même
`core::Equation`, et diffère de celle du repère neutre d'un MathML mal formé) ; **aucune marque ne fuit dans le document** (revues tours
2 et 3) : l'éditeur retire `actcraft-cursor` et `actcraft-placeholder` à l'insertion,
`insertedEquationHasNoEditorMark` (fraction saisie, curseur dans le dénominateur, « OK » : le `mathml` de l'objet
inséré ne contient aucune des deux marques) ; **une équation incomplète est refusée** (H7-19) :
`incompleteEquationIsRefused` (fraction vide, « OK » : aucun objet ajouté, `hasPlaceholder()` vrai, message de
l'éditeur affiché, le bouton reste actif) ; `everyTemplateHasAPaletteEntry` (les 17 modèles de
L7-T24 ont une entrée) ; éditeur en `FloatingPanel` (`equationEditorBlocksTheStylus`). **Test.**
`-R 'tst_features_equationeditor|tst_ui_equation'`.

<a id="tache-l7-t27"></a>
### Tâche L7-T27 — Équation IV (7b:740-777)

**Garde.** Étapes 1 à 3. **Change.** Tests dans `tst_ui_equation` ; gestionnaire `equation.edit` dans la racine ;
scénario `equation` (talon remplacé) ; `flipchartEquationIsDrawn` s'appuie sur la lecture de L7-T1 (dépendance) ;
`editKeepsTheId` vérifie aussi qu'une équation rééditée puis acceptée ne contient ni `actcraft-cursor` ni
`actcraft-placeholder`.
**Test.** `-R 'tst_ui_equation|tst_captures_equation'`. **Preuve visuelle.** `equation` hors écran,
`-Lire "documentController.fingerprint=${avant}"` après aller-retour ; capture décrite.

<a id="tache-l7-t28"></a>
### Tâche L7-T28 — Caméra (7b:779-863)

**Garde.** Étapes 3 à 5. **Disparaît.** Étapes 1 et 2 pour `PolygonPicker`, `FreehandPicker`, `cropToPolygon`,
`normalizedRect`, `toLogicalPixels`, `windowRectsInLogicalPixels` (L7-P9). **S'ajoute.** `topLevelWindows()` déclarée
dans `camera.h`, écrite dans `camera_win.cpp` (`EnumWindows`, `DwmGetWindowAttribute`, `dwmapi` déjà lié) **et**
`camera_other.cpp` (liste vide), créés ensemble (règle des paires) ; `topLevelWindowsListsOurWindow` (la fenêtre de
test y figure, en pixels logiques) ; zone de capture partielle gardée d'une capture à l'autre (P11,
`captureAreaKeptForTheNextCapture`) ; palette en `FloatingPanel` **dans la fenêtre de capture**, avec `pageArea: null` (elle n'est pas une couverture
de la vue principale : un élément d'une autre fenêtre). **Fenêtre propre**
(décision 4 bis) : la fenêtre plein écran de 7b, étape 3, est le `Component` de la racine `CameraOverlay.qml`, créé
à la demande ; `cameraWindowTakesEveryPointer` (à données) remplace `cameraPaletteBlocksTheStylus` : appui et glisser
envoyés à la fenêtre de capture → rectangle de sélection agrandi pour chaque pointeur, page principale inchangée.
Destinations « ressources » : lot 6 (H7-3). **Test.**
`-R 'tst_ui_camera|tst_features_camera'`. **Écran.** Contrôle de 7b étape 5, annoncé à Jérôme.

<a id="tache-l7-t29"></a>
### Tâche L7-T29 — Clavier à l'écran (7b:865-936)

**Garde.** Étapes 3 à 5. **Disparaît.** Étapes 1 et 2 pour `ScreenKeyboard` (L7-P9). **S'ajoute.** Injecteur
`keyinjector_win.cpp` (`SendInput`, jamais appelé par un test) **et** `keyinjector_other.cpp`, créés ensemble ; sa
déclaration va dans `screenkeyboard.h` (ou un en-tête neuf de `features/include`, alors ajouté à la ligne) ; tests dans
`tst_ui_keyboard`. **Fenêtre propre** (décision 4 bis) : la `Window` de 7b est le `Component` de la racine
`ScreenKeyboard.qml` ; `keyboardWindowTakesEveryPointer` (à données) remplace `keyboardBlocksTheStylus` : une touche
pressée au stylet, au doigt et à la souris sur la fenêtre du clavier tape son caractère dans le champ de test, la
page principale reste inchangée. **Test.** `-R 'tst_ui_keyboard|tst_features_keyboard'`.

<a id="tache-l7-t30"></a>
### Tâche L7-T30 — Annotation de bureau (7b:938-988)

**Garde.** Étapes 1 à 3 ; `DesktopAnnotation.qml` réutilise `PageArea` avec `tools: null` (prouvé par
`pageAreaWithoutToolsDraws`, L0-S9). **S'ajoute** (L7-T0) : page suivante = nouvelle page transparente vierge (P13,
`nextPageIsANewTransparentPage`) ; paperboard du bureau gardé d'une activation à l'autre pendant la session (P14,
`desktopPaperboardKeptBetweenActivations`) ; fond « superposition » limité à sa page (P22,
`onlyItsPageSeesTheDesktop`). L'onglet propre du bureau (P15) dépend du lot 2 : hors lot. **Fenêtre propre**
(décision 4 bis) : `DesktopController` crée la fenêtre depuis le `Component` de la racine `DesktopAnnotation.qml`
(`beginCreate`, format transparent posé sur la `QQuickWindow`, `completeCreate`) ; `desktopWindowTakesEveryPointer`
(à données) : un trait au stylet, au doigt et à la souris sur la fenêtre de bureau est versé dans le paperboard du
bureau, la page principale reste inchangée. **Test.**
`-R 'tst_ui_desktop'`. **Écran.** Contrôle de 7b étape 3, annoncé.

<a id="tache-l7-t31"></a>
### Tâche L7-T31 — Outils de bureau (7b:990-1021)

**Garde.** Étapes 2 et 3 côté contrôleur et QML. **Disparaît.** `wheelPositions`, `WheelEntry`, `defaultWheel` et
`wheelIsClockwiseFromTop` (L7-P9, `tst_features_desktopwheel`). **S'ajoute.** `wheelHasNoVoteNorVendorEntry`
(identifiants de `defaultWheel()` contre le registre des commandes chargé de `commands.json`). **Test.**
`-R 'tst_ui_desktop|tst_features_desktopwheel'`.

<a id="tache-l7-t32"></a>
### Tâche L7-T32 — Contrôle du lot (7b:1023-1063)

**Garde.** Étapes 1 à 4. **Change.** `everyLot7CommandHasAHandler` lit la liste des commandes du lot dans
`actions/data/commands.json` (bloc du lot 7, L0-S4) au lieu de la recopier ; tests dans `tst_ui_presentation`.
**S'ajoute.** Glisser le calcul vers la page (P3, rendu par la revue M6) : le résultat de `CalculatorPanel` se
glisse hors du panneau, un `DragHandler` sur la ligne du résultat ; au relâché sur la page, `pageText()` est inséré à
cet endroit par `DocumentInsertions::insertText` (une étape) : `calculationDraggedToThePageIsUndoneAtOnce` (glisser à
la souris jusqu'à (300, 200) de page : un `Text` de plus dont les bornes contiennent ce point ; `undo` : retiré),
dans `tst_ui_calculator`. `everyInputPanelIsACover` : chaque racine visible qui prend l'entrée (liste de la décision 3, sauf
`PerPageTools`, `OtherToolsHost`, fenêtres propres) est une couverture déclarée quand elle est ouverte
— vérifié : `PageView` n'expose pas la liste (`m_covers` privé, `pageview.cpp:725`) ; le test injecte donc un trait
au stylet sous chacune, ouverte : compte inchangé ; contrôle des talons de la décision 2 (deux `git grep` vides) et règle 4 du socle
(`tst_ui_lot7.exe <clé> -functions` pour chacune des 17 clés et `<test>.exe -functions` pour les autres : au
moins une fonction, aucune `reserved` ; une ligne `TRAIT`, `DOIGT` ou `POINTEUR` par scénario rempli). **Test.**
`-R 'tst_ui_presentation|tst_captures_presentation'`.

<a id="tache-l7-t33"></a>
### Tâche L7-T33 — Fusion avec le lot 3 (7b:1065-1126)

**Change** (socle « Changent » 10) : la liste de conflits attendus de 7b (commandes, disposition, modèle, vue,
contrôleur, `PageArea.qml`, `Main.qml`, `ui/CMakeLists.txt`, joueur de scénarios) **disparaît** : ces fichiers sont du
socle, le lot 7 n'y a rien écrit. La ligne de `tests/CMakeLists.txt` a déjà fusionné en phase 2. **Aucun conflit
attendu hors `graphify-out/`** ; tout autre conflit est un écart, rapporté avant résolution. Preuve : N = N_A + 45.

<a id="tache-l7-t34"></a>
### Tâche L7-T34 — Après fusion du lot 3 (7b:1128-1179)

**Garde.** Toutes ses étapes. **Change.** Tests répartis dans `tst_ui_mathtools`, `tst_ui_recognition`,
`tst_ui_equation`, `tst_ui_clock`, `tst_ui_desktop` (table) ; « Convertir en texte » du menu contextuel est déjà en
données (socle « Changent » 11) : la tâche branche seulement ses gestionnaires dans les racines concernées
(`RecognitionAlternatives.qml`, `EquationEditor.qml`, `ClockPanel.qml`, `CountdownDialog.qml`,
`DesktopAnnotation.qml`, `MathToolsLayer.qml`) et ne touche pas `Presentation.qml` (revue M3) ; « rétablir la page »
devient la quatrième action de fin : `endActionsAreTheThreeOfThisLot` est **remplacé** par
`endActionsAreTheFourOfTheSpecification` (revue M4), écrit dans le rapport ; les points
d'accroche du lot 3 sont connus par leurs noms (contrats figés du socle) : `ToolHost::activeTool()` (`toolhost.h:91`), outils
d'identifiants `highlighter` (le marqueur) et `eraser` (vérifié : `tools/include/actcraft/tools/tool.h:14`,
`erasertool.h:14`) ; guides du marqueur et de la gomme par le même filtre d'échantillons (le filtre
est en amont de l'outil actif, socle D-S1 : rien à changer dans `pageview.*`). Les symétries autour de l'origine
(P20) restent au lot 5.

---

## Hypothèses ajoutées (porte de fin du lot 7)

- **H7-14** (E9) : actions de fin du compte à rebours = spécification ; vérifiée à l'écran si une observation le
  permet, sinon gardée.
- **H7-15** (E11) : « Essayer » et « Appliquer » du message défilant = spécification.
- **H7-16** (décisions de l'orchestrateur, revues tours 2 et 3) : pour chaque propriété de page codée (12 mode du
  rideau, 16 mode du spot, 27 outils de page F-591, 30 effet propre à la page) dont L7-T6 n'établit aucune valeur par
  une source indépendante, la lecture codée n'est pas livrée (valeur par défaut, avertissement de lecture) ; la liste
  exacte vient du rapport de L7-T6.
- **H7-17** (ce plan) : délai de reconnaissance 0 = conversion au relâché (supposé, L7-T0).
- **H7-18** (ce plan) : mouvements des 16 effets déduits de leur nom (L7-T0) ; à préciser à l'écran en S22.
- **H7-19** (ce plan, revues tours 2 et 3) : une case vide d'une équation se dessine en cadre pointillé dans
  l'éditeur seulement (mode `Editor` de `paintEquation`, L7-T25) ; sur la page et à l'impression, rien ne se
  dessine pour un `<mrow/>` vide (`pageDrawsNoPlaceholder`) ; l'éditeur refuse d'insérer une équation incomplète
  (`incompleteEquationIsRefused`). Question de spécification, à trancher à la porte.

## Vérifié contre le socle réel (feat/livraison-complete-b @ 131f790)

Lu dans le code fusionné, sans construction. Chaque point du brouillon : « vérifié : … (fichier:ligne) » ou la
correction faite. Ce qui relève de Qt ou de CMake et non du socle reste à prouver par la construction de la tâche
nommée.

1. **N_F** — vérifié par A : 113 tests sur `feat/livraison-complete` (debug et release 113/113, état du chantier,
   « Socle fusionné ») ; N_F = 113 + 16 = **129**, à confirmer par la suite de l'orchestrateur ; N = 158 après L7-S8.
2. **Crochet de `ui/`** — vérifié : `ui/CMakeLists.txt:27-29` (deux `set(… "")`, `include(… OPTIONAL)` avant
   `qt_add_qml_module`, l. 31), listes en fin de `QML_FILES` (l. 60) et de `SOURCES` (l. 123), `ActCraft::features`
   lié (l. 147). **Correction** : `QT_QML_SKIP_CACHEGEN` retiré de `ui/presentation.cmake` (le socle tait C4702 pour
   tout le C++ de qmlcachegen du module, l. 126-140). Chemins absolus de `file(GLOB)` : à la construction de L7-S8.
3. **`Presentation.qml`** — vérifié avec correction : racine **`QtObject`** (`ui/qml/Presentation.qml:12`, `layer`
   FINAL de `Item`), `default property list<QtObject> data` (l. 14), six propriétés (l. 15-20) ; instance dans
   `ui/qml/Main.qml:254-262` avec `tools: appTools`, `pageOverlays: pageArea.pageOverlays`,
   `layer: presentationLayer` ; `presentationLayer` `z: 50` (l. 248-253). Décision 3, étape 5 de L7-S8 et
   `lot7RootsExist` (`findChild<QObject*>`) corrigés.
4. **`PageArea`** — vérifié : alias `view`, `pageOverlays`, `presentationSlot`, propriétés `tools`, `commands`
   (`ui/qml/PageArea.qml:10-19`) ; `addInputCover`, `removeInputCover` `Q_INVOKABLE` (`pageview.h:181-182`).
5. **`PageView`** — vérifié : `SampleFilter = std::function<input::Sample(const input::Sample&)>` (`pageview.h:84`),
   `setSampleFilter` (l. 185), `zoomAround` et `sceneToPage` `Q_INVOKABLE` (l. 188-189), `cacheRefreshRequested`
   (l. 199), `cacheRefreshCount` (l. 80, 175), `contextMenuRequested` (l. 211) ; bornes `ui::kMinZoom = 0.25`,
   `kMaxZoom = 8.0` (`viewtransform.h:12-13`).
6. **`DocumentController`** — vérifié : `document()` (l. 76) et `execute()` (l. 79) publics ; `ChangeOrigin::Stroke`
   (l. 145-153), `ChangeObserver = std::function<void(const core::Changes&, ChangeOrigin)>`, `addChangeObserver`
   (l. 160-162) ; pas de `shownIndex` : `currentIndex()` (l. 66, `int`) ; `show()` privée (l. 210), navigation publique `nextPage`, `pageBack`
   (l. 92-93) ; `documentChanged` émis après chaque modification (`documentcontroller.cpp:311-316`).
   **Corrections** : test de L7-T2 sur `Change::object` ; signal dédié `documentReplaced` ajouté par L7-T2
   (décision 13) ; page de départ rendue par `pageBack()` différé (L7-T2) ; `addResource`
   remplacé par `insertImage` (pas d'accès en écriture au document, l. 76).
7. **`core`** — vérifié : `Equation` (`model.h:337-343`), `PageToolsMode`, `CurtainMode`, `SpotlightShape`
   (l. 438-461, valeurs du contrat), `PagePresentation` avec `spotlightFrame`, `hasOrigin`, `origin`, `transition`,
   `unknownKeys` (l. 462-474), `ObjectReplacement`, `ReplaceObjectsCommand`, `addedIds`, `SetPagePresentationCommand`
   (`replace.h:14-64`), polygone = `ShapeKind::Polygon` et `Shape::polygon` (`model.h:200-212`, L7-T21 corrigée),
   `ImageObject::resource` (l. 236-247), `Document::addResource` (`document.h:66`) ; aucune méthode de retrait :
   `removeResource` ajouté par L7-T2 (ajout seulement, signalé à A).
8. **Fichiers d'équation** — vérifié : `core/equation.*` et `render/src/equationlayout.*` enregistrés
   (`render/CMakeLists.txt:31-32`) ; marqueurs : trois lignes (décision 2) ; `paintEquation` privé
   (`render/src/equationlayout.h:10`) ; seul `render/src/pagepainter.cpp:1` inclut `equationlayout.h`
   (`paintedrenderer.cpp` non) : L7-T25 corrigée (inclusion de l'en-tête public dans `equationlayout.h`, peintres
   inchangés) ; `tst_core_equation.cpp`, `tst_render_equation.cpp` absents (k = 0).
9. **`ToolHost`** — vérifié : `user()` (`toolhost.h:92`), `activeTool()` (l. 91), couleur `Q_PROPERTY(QColor color)`
   (l. 42), type QML `Tools` (l. 34) ; `UserState::setOpenMathTools(std::vector<QString>)` (`userstate.h:39`) ;
   outils `highlighter`, `eraser` (`tool.h:14`, `erasertool.h:14`).
10. **Commandes** — vérifié : 85 commandes dans `actions/data/commands.json`, tous les identifiants du lot cités ici
    présents ; aucune pour P8, P9, P3 (décision 8 maintenue).
11. **Talons du lot 3 et couvertures** — vérifié : `SelectionOverlay.qml` n'a aucun gestionnaire d'appui (talon du
    lot 3) : il ne prend pas l'entrée au-dessus de `presentationSlot` ; `coveredAt` : visible, actif,
    `contains(mapFromScene)` (`pageview.cpp:724-727`). Le piège de L3-T15 (gestionnaire sur la racine) reste à
    surveiller à la fusion du lot 3 (L7-T33).
12. **Ordre des `include`** — vérifié : `tests/CMakeLists.txt:42-47` (`ui`, `bench`, `app`, puis `presentation` en
    dernier) ; `_bench_env_offscreen` (`tests/bench.cmake:8`) ; `tst_ui_shell` (`tests/ui.cmake:33-36`) ;
    `tst_ui_inputpath` (`tests/ui.cmake:64-66`).
13. **Aides d'injection** — vérifié avec correction : forme de `tests/tst_ui_inputpath.cpp` (stylet l. 141-160,
    220-223 ; doigt par `QTest::createTouchDevice` et `QTest::touchEvent` avec une image entre deux étapes, l. 171-187
    et 224 ; souris l. 162-169) reprise dans `presentationharness.h` (L7-S8, étape 6) ; scénarios : `expect` avec
    `tolerance` (`bench/src/uiscenario.cpp:214`, `:243-244`, point 4 de A).
14. **Peintre de l'équation** — vérifié : `pagepainter.cpp` appelle `paintEquation` par `equationlayout.h`
    (l. 1) ; ligne de l'en-tête public à poser avant `src/equationlayout.h` (`render/CMakeLists.txt:27-32`) ;
    décision de A appliquée (L7-T25, L7-T26).
15. **Exécutable unique** — point de Qt et de l'éditeur de liens, non du socle : `Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)`
    à reprendre une fois dans `tst_ui_lot7_main.cpp` (comme `tests/tst_ui_shell.cpp:30`) ; attributs de
    `QTEST_MAIN` : à la construction de L7-S8.
16. **Fenêtres créées à la demande** — point de Qt, non du socle : à prouver par L7-T28, L7-T29, L7-T30 (hors écran,
    puis contrôle à l'écran annoncé de L7-T28 et L7-T30).
17. **Noms** — `NOMS OK` sur la branche fusionnée (contrôle de 97c3185 : l'outil affiche « NOMS OK : 487 fichiers », son propre compte des fichiers qu'il lit, pas
    le nombre de fichiers suivis par git) ; noms neufs des
    tâches repassés au contrôle après `git add` par chaque tâche.

## Risques

1. **Exécutable d'interface unique** (décision 1, revue M5) : un seul lien et un seul PDB au lieu de 17 (34 avec
   le preset release du banc) ; en échange, toute modification d'un `tst_ui_<fonction>.cpp` relie l'exécutable
   entier, et un plantage d'une classe n'arrête que son entrée ctest (chaque entrée est un processus). Deux tâches
   d'une vague peuvent ne plus lier ensemble : règle d'isolation (décision 1) et construction de la branche après la
   deuxième tâche de chaque vague (« Branche et worktrees »). ctest tourne en
   série : la mémoire à l'exécution n'est pas multipliée.
2. **`file(GLOB … CONFIGURE_DEPENDS)` dans `ui/`** : écart à la convention des listes écrites (déjà pris pour
   `features/`, phase 1) ; un contrôleur créé sous un autre nom n'est pas compilé — la commande ciblée rouge le montre.
3. **Racines instanciées avant leur contenu** : une racine talon qui casse (propriété retirée) casse le chargement de
   toute l'interface ; `lot7RootsExist` (dans `tst_ui_presentation`) et `shellLoadsWithoutQmlWarning` (socle), joués
   par la suite complète à chaque preuve, le gardent.
4. **Couverture et `containmentMask`** : une surcouche qui couvre toute la page (rideau, spot) et ne restreint pas sa forme
   couvre toute la page ; chaque `…BlocksTheStylus` doit avoir son pendant « hors de la surcouche, le trait est
   versé » (piège de L3-T15, socle « Amendements », 12).
5. **Points de Qt non prouvés** (points 15 et 16 ci-dessus) : ils ne dépendent plus du socle mais de la
   construction ; L7-S8, L7-T28, L7-T29 et L7-T30 les prouvent ou rapportent l'écart.
