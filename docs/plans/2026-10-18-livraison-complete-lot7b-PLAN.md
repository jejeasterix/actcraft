# Livraison complète — lot 7 « Présentation et outils » — sous-lot 7b — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**Suite du sous-lot 7a** (`docs/plans/2026-10-18-livraison-complete-lot7-PLAN.md`) : même session B (chantier
`livraison-complete-b`, branche `feat/livraison-complete-b`), mêmes contrats partagés, mêmes **règles communes**
(section du plan 7a, applicables mot pour mot ici, `<wt>` = `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete-b--L7-T<n>`),
même N0. 7b commence quand 7a est terminé (« Fin du sous-lot 7a »). **Il s'exécute sans connaître le lot 3** jusqu'à
`L7-T33` (fusion), qui attend la livraison du lot 3 dans `feat/livraison-complete` ; `L7-T34` regroupe ce qui a
besoin du lot 3.

**But.** Reconnaissance de forme (algorithme à nous) et d'écriture (Windows), équation (modèle, MathML lu et écrit
sans perte, rendu, éditeur à modèles), caméra, clavier à l'écran, annotation de bureau et outils de bureau (bureau
seulement) ; contrôle du lot ; fusion avec le lot 3 ; fonctions qui en dépendent.

## Table des tâches (sous-lot 7b)

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L7-T20 | Reconnaissance de forme I (F-296) : algorithme à nous (ligne, rectangle, carré, ellipse, cercle, triangle, rectangle incliné ; sinon rien) | L7-T19 | `features/include/actcraft/features/shaperecognizer.h`, `features/src/shaperecognizer.cpp`, `features/CMakeLists.txt`, `tests/tst_features_shaperecognizer.cpp`, `tests/presentation.cmake` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 19, `tst_features_shaperecognizer` Passed (dont `wobblyCircleIsACircle`, `tiltedRectangleKeepsItsCorners`, `openScribbleIsNothing`, `shortStrokeIsNothing`) | S10 | #tache-l7-t20 |
| L7-T21 | Reconnaissance de forme II : mode, délai, trait remplacé par la forme en une étape, remplissage facultatif, conversion des derniers traits ; scénario de banc `reconnaissance` | L7-T20 | `features/include/actcraft/features/recognitionsession.h`, `features/src/recognitionsession.cpp`, `features/CMakeLists.txt`, `ui/src/recognitioncontroller.h`, `ui/src/recognitioncontroller.cpp`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `ui/icons/forme-reconnue.svg`, `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp`, `tests/tst_bench_scenarios.cpp`, `bench/scenarios/reconnaissance-forme.json`, `tests/tst_features_shaperecognizer.cpp`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 20, `tst_features_shaperecognizer` (dont `sessionFiresAfterTheDelay`, `newStrokeRestartsTheDelay`), `tst_ui_presentation` (dont `recognizedStrokeBecomesAShapeInOneStep`, `fillOptionFillsTheShape`), `tst_bench_scenarios` (dont `recognitionScenarioListed`), `tst_captures_reconnaissance_forme` Passed ; banc hors écran : scénario `reconnaissance` présent, `cache.refreshes` relevé | S10 | #tache-l7-t21 |
| L7-T22 | Reconnaissance d'écriture I (F-295, Windows) : moteur d'encre du système par COM, reconnaisseur français préféré, alternatives ; indisponible = explication | L7-T21 | `features/include/actcraft/features/handwriting.h`, `features/src/handwriting.cpp`, `features/src/handwriting_win.cpp`, `features/src/handwriting_other.cpp`, `features/CMakeLists.txt`, `tests/tst_features_handwriting.cpp`, `tests/presentation.cmake` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 21, `tst_features_handwriting` Passed (dont `unavailableOffWindows` ou `recognizesSyntheticInk`, sauté et signalé `QSKIP` si le reconnaisseur manque) | S10 | #tache-l7-t22 |
| L7-T23 | Reconnaissance d'écriture II : mode Ctrl+Shift+H, délai 1 à 5 s, mot → texte (couche intermédiaire) en une étape, alternatives, commande grisée si indisponible, absente d'Android | L7-T22 | `ui/src/recognitioncontroller.h`, `ui/src/recognitioncontroller.cpp`, `ui/qml/RecognitionAlternatives.qml`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `ui/icons/ecriture.svg`, `tests/tst_ui_presentation.cpp` | isolé | implementeur-tdd | `TESTS OK`, N ≥ N0 + 21, `tst_ui_presentation` (dont `handwritingBecomesTextInOneStep` (sauté et signalé sans reconnaisseur), `handwritingDisabledWithExplanation`, `handwritingAbsentOnAndroid`) | S10 | #tache-l7-t23 |
| L7-T24 | Équation I (F-301) : modèle d'arbre et MathML (lecture, écriture canonique, éléments inconnus gardés) | L7-T23 | `core/include/actcraft/core/equation.h`, `core/src/equation.cpp`, `tests/tst_core_equation.cpp`, `tests/presentation.cmake` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 22, `tst_core_equation` Passed (dont `templatesRoundTrip`, `unknownElementKeptVerbatim`, `entitiesDecoded`, `malformedIsAnErrorNotACrash`, `untouchedEquationKeepsItsBytes`) | S24 | #tache-l7-t24 |
| L7-T25 | Équation II : mise en page et dessin (fraction, puissance, indice, racines, parenthèses, grands opérateurs, matrice) par le peintre partagé ; objet versé au cache par l'interface figée | L7-T24 | `render/src/equationlayout.h`, `render/src/equationlayout.cpp`, `tests/tst_render_equation.cpp`, `tests/presentation.cmake` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 23, `tst_render_equation` Passed (dont `fractionIsTallerThanItsParts`, `superscriptIsRaised`, `rootCoversItsContent`, `malformedDrawsTheNeutralFrame`, `paintedMatchesSoftware`) ; banc hors écran joint | S24 | #tache-l7-t25 |
| L7-T26 | Équation III : éditeur à modèles (curseur dans l'arbre, modèles, saisie, effacement, flèches), aperçu, insérer | L7-T25 | `features/include/actcraft/features/equationeditor.h`, `features/src/equationeditor.cpp`, `features/CMakeLists.txt`, `ui/src/equationcontroller.h`, `ui/src/equationcontroller.cpp`, `ui/src/equationpreviewitem.h`, `ui/src/equationpreviewitem.cpp`, `ui/qml/EquationEditor.qml`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `tests/tst_features_equationeditor.cpp`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 24, `tst_features_equationeditor` Passed (dont `fractionTemplateThenTwoDigits`, `backspaceLeavesAPlaceholder`, `arrowsWalkTheTree`), `tst_ui_presentation` (dont `insertEquationIsOneUndoStep`) | S24 | #tache-l7-t26 |
| L7-T27 | Équation IV : rééditer par `equation.edit <id>` (même identifiant), aller-retour `.actcraft`, `.flipchart` synthétique affiché, scénario S24 | L7-T26 | `ui/src/equationcontroller.h`, `ui/src/equationcontroller.cpp`, `ui/qml/EquationEditor.qml`, `bench/scenarios/equation.json`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake` | isolé | implementeur-tdd | `TESTS OK`, N ≥ N0 + 25, `tst_ui_presentation` (dont `editKeepsTheId`, `equationRoundTripIdentical`, `flipchartEquationIsDrawn`), `tst_captures_equation` Passed ; `captures.ps1 -Scenario equation` hors écran `LU documentController.fingerprint=${avant}` après aller-retour | S24 | #tache-l7-t27 |
| L7-T28 | Caméra (F-275, bureau) : zone, point par point, main levée, fenêtre, écran entier ; vers la page, une nouvelle page, le presse-papiers | L7-T27 | `features/include/actcraft/features/camera.h`, `features/src/camera.cpp`, `features/src/camera_win.cpp`, `features/src/camera_other.cpp`, `features/CMakeLists.txt`, `ui/src/cameracontroller.h`, `ui/src/cameracontroller.cpp`, `ui/qml/CameraOverlay.qml`, `ui/qml/CameraPalette.qml`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `ui/icons/camera.svg`, `tests/tst_features_camera.cpp`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 26, `tst_features_camera` Passed (dont `clickNearFirstPointClosesThePolygon`, `freehandClosesOnRelease`, `cropOutsideIsTransparent`), `tst_ui_presentation` (dont `captureToPageAddsOneImage`, `captureToNewPageAddsAPage`, `captureToClipboard`, `cameraAbsentOnAndroid`) ; banc hors écran joint | S10 | #tache-l7-t28 |
| L7-T29 | Clavier à l'écran (F-294, bureau) : le nôtre, déplaçable, standard ou étendu, nombre de touches ; vers le champ de texte actif ou une autre application (Windows) | L7-T28 | `features/include/actcraft/features/screenkeyboard.h`, `features/src/screenkeyboard.cpp`, `features/src/keyinjector_win.cpp`, `features/src/keyinjector_other.cpp`, `features/CMakeLists.txt`, `ui/src/keyboardcontroller.h`, `ui/src/keyboardcontroller.cpp`, `ui/qml/ScreenKeyboard.qml`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `ui/icons/clavier.svg`, `tests/tst_features_keyboard.cpp`, `tests/tst_ui_presentation.cpp`, `tests/presentation.cmake` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 27, `tst_features_keyboard` Passed (dont `shiftTypesCapitals`, `compactLayoutHasFewerKeys`, `accentKeysTypeFrench`), `tst_ui_presentation` (dont `keyboardTypesIntoTheFocusedField`, `keyboardWindowNeverTakesFocus`) | S10 | #tache-l7-t29 |
| L7-T30 | Annotation de bureau (F-070, F-071, bureau) : paperboard transparent au-dessus du bureau, stylo de la palette, clics qui traversent sur demande ; fond « capture du bureau » et « superposition du bureau » | L7-T29 | `ui/src/desktopcontroller.h`, `ui/src/desktopcontroller.cpp`, `ui/qml/DesktopAnnotation.qml`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `ui/icons/bureau.svg`, `tests/tst_ui_desktop.cpp`, `tests/presentation.cmake` | isolé | concepteur-critique | `TESTS OK`, N ≥ N0 + 28, `tst_ui_desktop` Passed (dont `annotationWindowIsTransparentAndOnTop`, `strokeGoesToTheDesktopPaperboard`, `passThroughMakesTheWindowTransparentForInput`, `desktopCaptureBecomesTheBackground`, `overlayBackgroundIsNotSaved`) ; banc hors écran joint | S10 | #tache-l7-t30 |
| L7-T31 | Outils de bureau (F-080, bureau) : fenêtre réduite, icône flottante toujours devant, roue d'outils (sens horaire depuis le haut) et anneaux secondaires | L7-T30 | `features/include/actcraft/features/desktopwheel.h`, `features/src/desktopwheel.cpp`, `features/CMakeLists.txt`, `ui/src/desktopcontroller.h`, `ui/src/desktopcontroller.cpp`, `ui/qml/DesktopToolsWheel.qml`, `ui/qml/Presentation.qml`, `ui/presentation.cmake`, `tests/tst_ui_desktop.cpp` | isolé | implementeur-tdd | `TESTS OK`, N ≥ N0 + 28, `tst_ui_desktop` (dont `wheelIsClockwiseFromTop`, `restoreClosesTheWheel`, `wheelClosesWhenTheWindowIsRestored`, `wheelHasNoVoteNorVendorEntry`) | S10 | #tache-l7-t31 |
| L7-T32 | Contrôle du lot : chaque commande a son gestionnaire, menus Android sans les commandes de bureau, scénario de bout en bout `presentation` (S10), liste des épinglages pour les profils du lot 6 | L7-T31 | `tests/tst_ui_presentation.cpp`, `bench/scenarios/presentation.json`, `tests/presentation.cmake`, `docs/verifications/AAAA-MM-JJ-lot7-epinglages.md` | isolé | implementeur-tdd | `TESTS OK`, N ≥ N0 + 29, `tst_ui_presentation` (dont `everyLot7CommandHasAHandler`, `androidMenusHaveNoDesktopCommand`), `tst_captures_presentation` Passed ; `captures.ps1 -Scenario presentation` hors écran `CAPTURES OK` | S10 | #tache-l7-t32 |
| L7-T33 | Fusion : `feat/livraison-complete` (lot 3 livré) fusionnée dans `feat/livraison-complete-b`, conflits des points réservés résolus, suites rejouées | L7-T32 et **livraison du lot 3 dans `feat/livraison-complete`** | fichiers en conflit (liste attendue : section de la tâche) ; `graphify-out/` | partagé | concepteur-critique | merge sans marqueur restant (`git -C <wt> grep -n '^<<<<<<<\|^>>>>>>>'` vide) ; suite debug et release `TESTS OK`, N ≥ (tests du lot 3 à sa livraison) + 29 ; captures hors écran de tous les scénarios du dépôt `CAPTURES OK` ; `NOMS OK` ; `graphify update .` commité | S10, S22, S23, S24 (rejoués) | #tache-l7-t33 |
| L7-T34 | Après fusion du lot 3 : origine XY = pivot de rotation, convertir en forme ou en texte la sélection, double-clic sur une équation, « rétablir la page » en fin de compte à rebours, guides pour le marqueur et la gomme, clics qui traversent avec l'outil de sélection | L7-T33 | `ui/src/mathtoolscontroller.h`, `ui/src/mathtoolscontroller.cpp`, `ui/qml/MathToolsLayer.qml`, `ui/src/recognitioncontroller.h`, `ui/src/recognitioncontroller.cpp`, `ui/src/equationcontroller.h`, `ui/src/equationcontroller.cpp`, `ui/src/clockcontroller.h`, `ui/src/clockcontroller.cpp`, `ui/src/desktopcontroller.h`, `ui/src/desktopcontroller.cpp`, `ui/qml/Presentation.qml`, `tests/tst_ui_presentation.cpp`, `tests/tst_ui_desktop.cpp` (et, si la fusion l'exige, les seuls points d'accroche du lot 3 que la section nomme) | isolé | concepteur-critique | `TESTS OK`, N ≥ celui de L7-T33, `tst_ui_presentation` (dont `rotationPivotsOnTheOrigin`, `convertSelectionToShape`, `convertSelectionToText` (sauté sans reconnaisseur), `doubleClickEditsTheEquation`, `countdownCanRevertThePage`, `markerFollowsTheRuler`, `eraserFollowsTheCompass`), `tst_ui_desktop` (dont `selectToolPassesClicksThrough`) ; banc hors écran joint | S10, S24 | #tache-l7-t34 |

15 tâches dans ce sous-lot. Tests ajoutés en 7b : `tst_features_shaperecognizer`, `tst_captures_reconnaissance_forme`,
`tst_features_handwriting`, `tst_core_equation`, `tst_render_equation`, `tst_features_equationeditor`,
`tst_captures_equation`, `tst_features_camera`, `tst_features_keyboard`, `tst_ui_desktop`,
`tst_captures_presentation` : **N0 + 29** à la fin de L7-T32 (N0 + 18 en fin de 7a).

**Écran.** Aucune tâche de 7b n'ouvre de fenêtre visible, à deux exceptions annoncées par l'orchestrateur à Jérôme
d'une ligne **avant** leur lancement, sous le jeton `ecran` : L7-T28 (une capture d'écran réelle, contrôle de fin de
tâche : la fenêtre de sélection s'ouvre une seconde puis se ferme seule) et L7-T30 (une fenêtre transparente au-dessus
du bureau, 3 s). Aucune n'utilise la souris de Jérôme : événements injectés seulement.

## Vagues (session B : une seule place de tests)

V20 : L7-T20 · V21 : L7-T21 · V22 : L7-T22 · V23 : L7-T23 · V24 : L7-T24 · V25 : L7-T25 · V26 : L7-T26 ·
V27 : L7-T27 · V28 : L7-T28 · V29 : L7-T29 · V30 : L7-T30 · V31 : L7-T31 · V32 : L7-T32 · V33 : L7-T33 (attend
le lot 3) · V34 : L7-T34.

Les tâches de 7b sont en chaîne : chacune touche `ui/qml/Presentation.qml`, `ui/presentation.cmake`,
`tests/presentation.cmake` ou `features/CMakeLists.txt`, et une seule construit à la fois. Une tâche sans
construction peut accompagner chaque vague si l'orchestrateur en a une (relecture d'une tâche précédente, rapport).

## Dépendances déclarées

- **Lot 10** (dans la base de `feat/livraison-complete-b`) : interface de cache figée et versement différé
  (`PendingLayerItem`) ; `pageOverlays` est posé **au-dessus** de la couche d'attente (L7-T2) ; reconnaissance et
  équation insèrent ou remplacent des objets par la pile, donc par l'interface figée ; le remplacement d'un trait déjà
  versé garde le redessin immédiat (limite déclarée de DA11, écart 5 du lot 10) : L7-T21 le **mesure** par le
  scénario de banc `reconnaissance` demandé par le lot 10.
- **Lot 3** : L7-T33 attend sa livraison dans `feat/livraison-complete` ; L7-T34 utilise sa sélection, sa rotation,
  son marqueur, sa gomme, sa commande de rétablissement de page. Aucune autre tâche n'en dépend.
- **Lots 4, 5, 6, 8** (après le lot 7) : texte (le lot 4 rendra modifiables les textes posés par le lot 7) ; menu
  d'édition d'objet (lot 5 : entrées « Convertir ») ; profils, ressources, navigateur de propriétés (lot 6 : épinglages
  de L7-T32, destinations « ressources » de la caméra, propriétés de page de présentation dans le navigateur) ; sons
  (lot 8 : son de fin du compte à rebours, enregistreurs dans la roue de bureau).

## Couverture des scénarios (7b)

| Scénario | Tâches | Joué à la clôture |
|---|---|---|
| S10 — présentation | L7-T20 à L7-T23, L7-T28 à L7-T32, L7-T34 | `presentation` hors écran et à l'écran D3D11 ; reconnaissance d'écriture sous Windows (sautée et signalée sans reconnaisseur) ; menus de l'APK sans les commandes de bureau ni d'écriture (capture de l'émulateur) |
| S24 — équation | L7-T24 à L7-T27, L7-T34 (double-clic) | `equation` hors écran et à l'écran |
| S22, S23 | (7a) | rejoués à la clôture du lot, voir « Clôture » |

## Carte des fichiers (7b)

| Fichier | Responsabilité | Tâche |
|---|---|---|
| `features/…/shaperecognizer.h`, `features/src/shaperecognizer.cpp` | `recognizeShape(points)` à nous | L7-T20 |
| `features/…/recognitionsession.h`, `features/src/recognitionsession.cpp` | traits regroupés, délai, déclenchement | L7-T21 |
| `ui/src/recognitioncontroller.*` | modes forme et écriture, remplacement par `replaceObjects`, alternatives | L7-T21, L7-T23, L7-T34 |
| `bench/…/scenarios.h`, `bench/src/scenarios.cpp` | scénario de banc `reconnaissance` | L7-T21 |
| `features/…/handwriting.h`, `features/src/handwriting.cpp`, `features/src/handwriting_win.cpp` | interface du reconnaisseur ; moteur COM de Windows | L7-T22 |
| `core/…/equation.h`, `core/src/equation.cpp` | arbre d'équation, MathML (fichiers réservés vides par L7-T1) | L7-T24 |
| `render/src/equationlayout.h`, `render/src/equationlayout.cpp` | mise en page et dessin (fichiers réservés par L7-T1 avec le repère neutre) | L7-T25 |
| `features/…/equationeditor.h`, `features/src/equationeditor.cpp` | modèle d'édition | L7-T26 |
| `ui/src/equationcontroller.*`, `ui/src/equationpreviewitem.*`, `ui/qml/EquationEditor.qml` | éditeur, aperçu, insertion, réédition | L7-T26, L7-T27, L7-T34 |
| `features/…/camera.h`, `features/src/camera.cpp`, `features/src/camera_win.cpp` | sélection, découpe, liste des fenêtres | L7-T28 |
| `features/…/screenkeyboard.h`, `features/src/screenkeyboard.cpp`, `features/src/keyinjector_win.cpp` | dispositions, état des majuscules, envoi aux autres applications | L7-T29 |
| `ui/src/desktopcontroller.*`, `ui/qml/DesktopAnnotation.qml`, `ui/qml/DesktopToolsWheel.qml`, `features/…/desktopwheel.h` | annotation et outils de bureau | L7-T30, L7-T31 |

---

## Tâches

<a id="tache-l7-t20"></a>
### Tâche L7-T20 — Reconnaissance de forme I : algorithme à nous (F-296)

**Objectif.** Une fonction pure qui dit si un trait à main levée est une ligne, un rectangle (ou un carré, droit ou
incliné), une ellipse (ou un cercle, droits ou inclinés) ou un triangle, et sinon rien. Algorithme **conçu par nous**,
différent de celui de la référence Python : ré-échantillonnage à pas constant, détection des coins par l'angle de
virage cumulé sur une fenêtre glissante, contrôle de rectitude de chaque côté, ajustement d'ellipse par les axes
principaux (analyse en composantes principales des points) et l'écart radial normalisé.

**Fichiers.** Voir la table (ligne L7-T20).

**Test et commande bornée.** Commande ciblée `-R tst_features_shaperecognizer`, preuve de tâche (N ≥ N0 + 19).

**Interdits.** Aucun code, constante ni seuil recopiés d'un logiciel tiers ; les seuils sont les nôtres, choisis sur
nos tracés synthétiques et écrits avec leur raison dans le code. Pas de QtGui (QPointF, QRectF de QtCore seulement).

**Terminé quand.** Les cas du test passent, y compris les tracés bruités (graine fixe) ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.** `tests/tst_features_shaperecognizer.cpp` (tracés synthétiques générés dans le test,
  bruit de ± 3 pixels par `QRandomGenerator(1234)`) :
  ```cpp
  #include <actcraft/features/shaperecognizer.h>

  #include <QRandomGenerator>
  #include <QtTest>

  #include <cmath>

  using namespace actcraft::features;

  namespace {
  std::vector<QPointF> noisy(std::vector<QPointF> pts, double amplitude, quint32 seed)
  {
      QRandomGenerator g(seed);
      for (auto& p : pts)
          p += QPointF((g.generateDouble() * 2 - 1) * amplitude, (g.generateDouble() * 2 - 1) * amplitude);
      return pts;
  }
  std::vector<QPointF> ellipse(QPointF c, double rx, double ry, double tiltDeg, int n, double turns = 1.0)
  {
      std::vector<QPointF> out;
      const double t0 = qDegreesToRadians(tiltDeg);
      for (int i = 0; i <= n; ++i) {
          const double a = 2 * M_PI * turns * i / n;
          const double x = rx * std::cos(a), y = ry * std::sin(a);
          out.emplace_back(c.x() + x * std::cos(t0) - y * std::sin(t0), c.y() + x * std::sin(t0) + y * std::cos(t0));
      }
      return out;
  }
  std::vector<QPointF> polyline(const std::vector<QPointF>& corners, int perSide)
  {
      std::vector<QPointF> out;
      for (std::size_t i = 0; i < corners.size(); ++i) {
          const QPointF a = corners[i], b = corners[(i + 1) % corners.size()];
          for (int k = 0; k < perSide; ++k)
              out.push_back(a + (b - a) * (double(k) / perSide));
      }
      out.push_back(corners.front());
      return out;
  }
  } // namespace

  class TstFeaturesShapeRecognizer : public QObject
  {
      Q_OBJECT
  private slots:
      void straightLineIsALine()
      {
          const auto r = recognizeShape(noisy({{100, 100}, {200, 150}, {300, 200}, {400, 250}}, 2, 1));
          QVERIFY(r.has_value());
          QCOMPARE(r->kind, RecognizedShape::Kind::Line);
      }
      void wobblyCircleIsACircle()
      {
          const auto r = recognizeShape(noisy(ellipse({300, 300}, 100, 100, 0, 80), 3, 2));
          QVERIFY(r.has_value());
          QCOMPARE(r->kind, RecognizedShape::Kind::Circle);
          QVERIFY(std::abs(r->bounds.width() - 200) < 12);
      }
      void tiltedEllipseKeepsItsAngle()
      {
          const auto r = recognizeShape(noisy(ellipse({300, 300}, 150, 70, 30, 90), 2, 3));
          QVERIFY(r.has_value());
          QCOMPARE(r->kind, RecognizedShape::Kind::Ellipse);
          QVERIFY(std::abs(r->angleDegrees - 30) < 6);
      }
      void squareIsASquare()
      {
          const auto r = recognizeShape(noisy(polyline({{100, 100}, {300, 100}, {300, 300}, {100, 300}}, 25), 3, 4));
          QVERIFY(r.has_value());
          QCOMPARE(r->kind, RecognizedShape::Kind::Square);
      }
      void tiltedRectangleKeepsItsCorners()
      {
          const auto r = recognizeShape(noisy(polyline({{200, 100}, {400, 200}, {350, 300}, {150, 200}}, 25), 2, 5));
          QVERIFY(r.has_value());
          QCOMPARE(r->kind, RecognizedShape::Kind::Rectangle);
          QCOMPARE(r->corners.size(), std::size_t{4});
      }
      void triangleIsATriangle()
      {
          const auto r = recognizeShape(noisy(polyline({{300, 100}, {450, 350}, {150, 350}}, 30), 2, 6));
          QVERIFY(r.has_value());
          QCOMPARE(r->kind, RecognizedShape::Kind::Triangle);
      }
      void openScribbleIsNothing()
      {
          QVERIFY(!recognizeShape(noisy({{100, 100}, {180, 260}, {120, 90}, {300, 310}, {90, 200}, {260, 80}}, 1, 7)).has_value());
      }
      void spiralIsNothing()
      {
          auto pts = ellipse({300, 300}, 100, 100, 0, 120, 2.2);
          for (std::size_t i = 0; i < pts.size(); ++i) // rayon qui croît : pas un cercle
              pts[i] = QPointF(300, 300) + (pts[i] - QPointF(300, 300)) * (0.4 + 0.6 * double(i) / pts.size());
          QVERIFY(!recognizeShape(pts).has_value());
      }
      void shortStrokeIsNothing()
      {
          QVERIFY(!recognizeShape({{10, 10}, {14, 12}}).has_value());
      }
  };

  QTEST_GUILESS_MAIN(TstFeaturesShapeRecognizer)
  #include "tst_features_shaperecognizer.moc"
  ```
  Déclarer `tst_features_shaperecognizer` (`LIBS ActCraft::features`).
- [ ] **Étape 2 : en-tête.** `features/include/actcraft/features/shaperecognizer.h` :
  ```cpp
  #pragma once

  #include <QPointF>
  #include <QRectF>

  #include <optional>
  #include <vector>

  namespace actcraft::features {

  struct RecognizedShape
  {
      enum class Kind { Line, Rectangle, Square, Ellipse, Circle, Triangle };
      Kind kind = Kind::Line;
      QRectF bounds;               // non tourné, dans le repère de la forme (centre = centre de la forme)
      double angleDegrees = 0;     // rotation autour du centre de `bounds`
      std::vector<QPointF> corners; // sommets en coordonnées de page (triangle, rectangle) ; extrémités (ligne)
  };

  // Reconnaissance de forme (F-296), algorithme à nous ; rien si le trait ne ressemble à aucune forme.
  std::optional<RecognizedShape> recognizeShape(const std::vector<QPointF>& points);

  } // namespace actcraft::features
  ```
- [ ] **Étape 3 : algorithme.** `features/src/shaperecognizer.cpp`, en fonctions internes nommées : `resample`
  (64 points à pas constant le long du trait), `extent` (diagonale de la boîte ; < 16 pixels → rien), `isLine`
  (distance maximale à la corde ≤ 4 % de la corde et corde ≥ 85 % de la longueur), `closure` (distance entre le premier
  et le dernier point ≤ 20 % de la longueur ; sinon rien, la ligne mise à part), `corners` (angle de virage cumulé sur
  une fenêtre de 5 points ≥ 55° → coin ; coins voisins fusionnés), `sidesStraight` (chaque côté : écart maximal à son
  segment ≤ 8 % de sa longueur), `principalAxes` (moyenne, covariance, angle de l'axe principal, demi-axes aux écarts
  extrêmes projetés), `radialError` (moyenne de |r/r_attendu − 1| sur l'ellipse ajustée ≤ 0,12). Ordre : ligne ; puis
  3 coins et côtés droits → triangle ; 4 coins, côtés droits, angles 90° ± 20° → rectangle ou carré (rapport des côtés
  ≥ 0,85) ; puis ellipse ajustée → cercle si rapport des axes ≥ 0,85 ; sinon rien. Chaque seuil est une constante
  nommée avec sa raison en commentaire. `features/CMakeLists.txt`.
- [ ] **Étape 4 : preuves et commit.** Preuve de tâche, `NOMS OK` ; commit « Lot 7 : reconnaissance de forme,
  algorithme à nous (F-296) (L7-T20) ».

<a id="tache-l7-t21"></a>
### Tâche L7-T21 — Reconnaissance de forme II : mode, délai, remplacement, banc

**Objectif.** `tool.shape-recognition` (cochable) : chaque trait versé par le stylo (observateur de L7-T2) est confié
à une session ; à la fin du délai (réglage commun de reconnaissance, 1 à 5 s, défaut 5 s ; L7-T0 dit si l'original
convertit les formes au relâché, auquel cas le délai des formes est 0), un trait reconnu est **remplacé** par la
forme, couche intermédiaire, en **une** étape (`replaceObjects`) ; option « remplir les formes reconnues » (défaut
non ; remplissage de la couleur du contour, ou d'une autre couleur choisie). `recognition.convert-to-shape` convertit
les derniers traits non reconnus de la session (la conversion de la sélection vient en L7-T34). Scénario de banc
`reconnaissance` (demandé par le lot 10, écart 5).

**Fichiers.** Voir la table (ligne L7-T21).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_shaperecognizer|tst_ui_presentation|tst_bench_scenarios|tst_captures_reconnaissance_forme'`,
preuve de tâche (N ≥ N0 + 20), banc sans écran (scénario `reconnaissance` présent dans le JSON, `cache.refreshes`
relevé), preuve visuelle `-Scenario reconnaissance-forme -Lire "recognition.lastKind=circle"`.

**Preuve visuelle.** `bench/scenarios/reconnaissance-forme.json` : `tool.shape-recognition` ; `stroke` d'un cercle à
main levée (40 points) ; `wait` délai + 300 ms ; `waitReady` ; `expect` `recognition.lastKind` `circle` ;
`checkPixel` encre sur le cercle idéal et page au centre ; capture `cercle-reconnu`. Décrire : cercle net à la place
du tracé.

**Interdits.** Ne pas toucher au stylo ni à la vue. `bench/src/scenarios.cpp` et `bench/include/actcraft/bench/scenarios.h`
sont **partagés avec le lot 3** : y ajouter seulement la fonction du scénario et sa ligne dans `allScenarios`, en fin
de liste, et le consigner pour L7-T33 (point de fusion).

**Terminé quand.** Tests nommés verts ; JSON du banc avec `reconnaissance` ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge de la session.** Dans `tests/tst_features_shaperecognizer.cpp` :
  ```cpp
  void sessionFiresAfterTheDelay()
  {
      RecognitionSession s(5000);                // délai en ms
      s.addStroke(11, 0);                        // identifiant, instant (ns)
      QVERIFY(s.due(4'999'000'000).empty());
      const auto due = s.due(5'000'000'000);
      QCOMPARE(due.size(), std::size_t{1});
      QCOMPARE(due.front(), actcraft::core::ObjectId{11});
      QVERIFY(s.due(6'000'000'000).empty());     // une seule fois
  }
  void newStrokeRestartsTheDelay()
  {
      RecognitionSession s(5000);
      s.addStroke(1, 0);
      s.addStroke(2, 3'000'000'000);
      QVERIFY(s.due(5'500'000'000).empty());
      QCOMPARE(s.due(8'000'000'000).size(), std::size_t{2}); // les deux traits ensemble (écriture : un mot)
  }
  void zeroDelayIsImmediate()
  {
      RecognitionSession s(0);
      s.addStroke(5, 100);
      QCOMPARE(s.due(100).size(), std::size_t{1});
  }
  ```
- [ ] **Étape 2 : session.** `features/include/actcraft/features/recognitionsession.h`, `features/src/recognitionsession.cpp` :
  `class RecognitionSession` (`explicit RecognitionSession(int delayMs)`, `setDelayMs` borné 0 à 5000, `addStroke(id,
  nowNs)`, `due(nowNs)` rend et oublie les traits mûrs, `pending()`, `clear()`). `features/CMakeLists.txt`.
- [ ] **Étape 3 : contrôleur.** `ui/src/recognitioncontroller.h/.cpp` (`RecognitionController` : `shapeMode`,
  `fillShapes`, `fillColor` (vide = couleur du contour), `delayMs`, `lastKind` (texte), propriété `documentController` ;
  pose `setStrokeObserver` ; une `QTimer` à 100 ms tant qu'une session attend ; à l'échéance, pour chaque trait :
  lire ses points dans `shownPage()`, `recognizeShape`, et s'il y a une forme, `replaceObjects({id}, {forme})` : ligne
  → `core::Line` (couleur et épaisseur du trait) ; rectangle, carré → `core::Shape` `Rectangle` (rotation par
  `common.transform` si inclinée) ; ellipse, cercle → `Ellipse` ; triangle → `Polygon` ; contour = couleur et épaisseur
  du trait, remplissage selon l'option, sinon transparent) ; gestionnaires `tool.shape-recognition`
  et `recognition.convert-to-shape` dans `Presentation.qml` ; `ui/presentation.cmake` ;
  `ui/icons/forme-reconnue.svg`.
- [ ] **Étape 4 : tests d'interface.** `recognizedStrokeBecomesAShapeInOneStep` (cercle injecté, délai 0 par la
  propriété : un `Shape` `Ellipse`, plus de trait ; un Annuler rend le trait), `fillOptionFillsTheShape`,
  `unrecognizedStrokeStays`, `modeOffLeavesStrokes` dans `tst_ui_presentation.cpp`.
- [ ] **Étape 5 : scénario de banc.** Test rouge `recognitionScenarioListed` dans `tests/tst_bench_scenarios.cpp`
  (`allScenarios` contient `reconnaissance` ; ses étapes tracent un cercle puis attendent le délai + 300 ms) ;
  `ScenarioPlan recognitionScenario(ScenarioSet)` dans `bench/…/scenarios.h` et `bench/src/scenarios.cpp`, sur le
  modèle du scénario `pauses` du lot 10 (lire `pausesScenario` et reprendre sa forme), qui active la reconnaissance par
  la commande `tool.shape-recognition` avant le premier trait ; ajout en **fin** de `allScenarios`. Les tests existants
  qui comptent les scénarios (`fiveScenariosWithFrames` du lot 10) sont mis à jour en comprenant pourquoi ils
  changent (un scénario de plus), jamais affaiblis.
- [ ] **Étape 6 : captures et commit.** `bench/scenarios/reconnaissance-forme.json`,
  `tst_captures_reconnaissance_forme` dans `tests/presentation.cmake` ; preuve de tâche, banc sans écran, preuve
  visuelle, `NOMS OK` ; commit « Lot 7 : reconnaissance de forme, mode, remplacement annulable et scénario de banc
  (F-296) (L7-T21) ».

<a id="tache-l7-t22"></a>
### Tâche L7-T22 — Reconnaissance d'écriture I : moteur de Windows (F-295)

**Objectif.** Une interface `HandwritingRecognizer` et deux réalisations : celle de Windows (API d'encre du système
par COM : énumérer les reconnaisseurs, préférer le français, sinon le premier qui a une langue ; créer l'encre en
unités HIMETRIC (1 pixel de page = 26,4583 HIMETRIC à 96 ppp) ; reconnaître ; meilleur texte puis jusqu'à 10
alternatives sans doublon) et une réalisation « indisponible » partout ailleurs ou quand le moteur manque, avec son
explication en français.

**Fichiers.** Voir la table (ligne L7-T22).

**Test et commande bornée.** Commande ciblée `-R tst_features_handwriting`, preuve de tâche (N ≥ N0 + 21), avec la
ligne du test : `Passed` (moteur présent) ou la ligne `SKIP` signalée dans le rapport (moteur absent).

**Interdits.** `#include <windows.h>`, `<msinkaut.h>` et tout appel COM seulement dans `features/src/handwriting_win.cpp`,
construit sous `WIN32` seulement (`if(WIN32) target_sources(… src/handwriting_win.cpp) target_link_libraries(… ole32
oleaut32) else() target_sources(… src/handwriting_other.cpp) endif()` dans `features/CMakeLists.txt`). `CoInitializeEx` en appartement unique sur le fil qui reconnaît, et
`CoUninitialize` apparié. Aucune bibliothèque tierce.

**Terminé quand.** Test vert ou sauté et signalé ; construction macOS et Android sans le fichier Windows (vérifié par
la CI de la PR finale) ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.**
  ```cpp
  void factoryNeverReturnsNull()
  {
      auto r = makeHandwritingRecognizer();
      QVERIFY(r != nullptr);
      if (!r->available())
          QVERIFY(!r->status().isEmpty());
  }
  void unavailableOffWindows()
  {
  #ifndef Q_OS_WIN
      QVERIFY(!makeHandwritingRecognizer()->available());
  #else
      QSKIP("Windows : voir recognizesSyntheticInk");
  #endif
  }
  void recognizesSyntheticInk()
  {
      auto r = makeHandwritingRecognizer();
      if (!r->available())
          QSKIP(qPrintable(QStringLiteral("reconnaisseur absent : ") + r->status()));
      // « l » : un trait vertical ; le contenu n'est pas comparé (S10), seulement la forme du résultat
      const std::vector<std::vector<QPointF>> ink{{{100, 100}, {100, 140}, {100, 180}}};
      const QStringList out = r->recognize(ink);
      QVERIFY(!out.isEmpty());
      QCOMPARE(out.removeDuplicates(), qsizetype{0});
  }
  ```
  (`Q_OS_WIN` dans un test est permis : les tests ne sont pas du code livré ; le code livré n'a pas de `#ifdef`.)
- [ ] **Étape 2 : interface et réalisations.** `features/include/actcraft/features/handwriting.h` :
  ```cpp
  class HandwritingRecognizer
  {
  public:
      virtual ~HandwritingRecognizer() = default;
      virtual bool available() const = 0;
      virtual QString status() const = 0;  // explication si indisponible ; nom du reconnaisseur sinon
      virtual QStringList recognize(const std::vector<std::vector<QPointF>>& strokes) = 0;
  };
  std::unique_ptr<HandwritingRecognizer> makeHandwritingRecognizer();
  ```
  `features/src/handwriting.cpp` : réalisation « indisponible » (« La reconnaissance d'écriture utilise le moteur
  d'encre de Windows ; il n'est pas disponible sur ce système. ») et `makeHandwritingRecognizer`, qui appelle
  `makePlatformHandwritingRecognizer()` et retombe sur la réalisation « indisponible » si elle rend `nullptr`. Cette
  fonction est définie dans `features/src/handwriting_win.cpp` (Windows) ou `features/src/handwriting_other.cpp`
  (ailleurs : rend `nullptr`), CMake choisissant le fichier (`if(WIN32) … else() … endif()`), sur le modèle de
  `bench/src/gpumemory_win.cpp` / `gpumemory_other.cpp` : **aucun `#ifdef`** dans le code livré (DA10).
  `features/src/handwriting_win.cpp` : COM
  (`CLSID_InkRecognizers`, `IInkRecognizers`, `IInkRecognizer::get_Languages`, `CLSID_InkDisp`,
  `IInkDisp::CreateStroke` avec un `SAFEARRAY` de `LONG`, `IInkRecognizerContext::Recognize`,
  `IInkRecognitionResult::get_TopString`, `AlternatesFromSelection`), toutes les interfaces libérées
  (`Microsoft::WRL::ComPtr`).
- [ ] **Étape 3 : preuves et commit.** Preuve de tâche, `NOMS OK` ; commit « Lot 7 : reconnaissance d'écriture par le
  moteur d'encre de Windows (F-295) (L7-T22) ».

<a id="tache-l7-t23"></a>
### Tâche L7-T23 — Reconnaissance d'écriture II : mode et texte (F-295)

**Objectif.** `tool.handwriting` (Ctrl+Shift+H, Windows) : les traits écrits sont regroupés par la session
(L7-T21) ; à la fin du délai (1 à 5 s, défaut 5), le mot reconnu **remplace** les traits par un objet texte (couche
intermédiaire, police par défaut N2-3, taille déduite de la hauteur des traits) en une étape ; un petit bouton
« alternatives » à côté du texte propose les autres lectures (choisir = remplacer le texte, une étape). Moteur
indisponible : commande grisée (`setEnabled(false)`), info-bulle = `status()`. Absente d'Android (plateformes de L7-T1).
La reconnaissance tourne **hors du fil principal** (`QtConcurrent::run` ou fil dédié ; COM initialisé sur ce fil).

**Fichiers.** Voir la table (ligne L7-T23).

**Test et commande bornée.** Commande ciblée `-R tst_ui_presentation`, preuve de tâche (N ≥ N0 + 21).

**Interdits.** Aucun appel COM hors de `handwriting_win.cpp`. Le fil principal n'attend jamais le reconnaisseur.

**Terminé quand.** `handwritingBecomesTextInOneStep` (vert, ou sauté et signalé), `handwritingDisabledWithExplanation`
(reconnaisseur factice indisponible injecté), `handwritingAbsentOnAndroid`, `alternativeReplacesTheText`
(reconnaisseur factice) verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.** Les quatre tests dans `tst_ui_presentation.cpp`, avec un reconnaisseur factice
  injecté par une propriété de test du contrôleur (`setRecognizerForTest`) qui rend `{"bonjour", "bonjours"}`.
- [ ] **Étape 2 : code.** Mode écriture dans `RecognitionController` (exclusif avec le mode forme : activer l'un
  désactive l'autre), file vers un fil secondaire, retour par signal sur le fil principal, `replaceObjects(ids,
  {texte})` ; `ui/qml/RecognitionAlternatives.qml` (liste flottante près du dernier texte reconnu) ; gestionnaires
  `tool.handwriting` dans `Presentation.qml` ; `ui/presentation.cmake` ; `ui/icons/ecriture.svg`.
- [ ] **Étape 3 : preuves et commit.** Preuve de tâche, banc sans écran, `NOMS OK` ; commit « Lot 7 : mode de
  reconnaissance d'écriture, texte et alternatives (F-295) (L7-T23) ».

<a id="tache-l7-t24"></a>
### Tâche L7-T24 — Équation I : modèle d'arbre et MathML (F-301)

**Objectif.** Dans `core/` (QtCore seulement : `QXmlStreamReader`, `QXmlStreamWriter`) : un arbre d'équation
(`EquationNode`) et sa conversion depuis et vers un **sous-ensemble de MathML présentation** (norme ouverte, choix de
réalisation moderne consigné, décision 0100). Garantie « sans perte » : `core::Equation::mathml` est le texte gardé ;
une équation qu'on ne modifie pas est réécrite **octet pour octet** ; une équation modifiée est réécrite sous la forme
canonique ; un élément MathML inconnu de cette version est gardé tel quel (nœud `Raw`, texte XML d'origine) et réécrit
identique.

**Fichiers.** Voir la table (ligne L7-T24). `core/include/actcraft/core/equation.h` et `core/src/equation.cpp` ont été
**créés vides et déclarés** par L7-T1 (fichiers réservés) : cette tâche les remplit, sans toucher `core/CMakeLists.txt`.

**Test et commande bornée.** Commande ciblée `-R tst_core_equation`, preuve de tâche (N ≥ N0 + 22).

**Interdits.** Aucune résolution d'entité externe, aucun DTD (`QXmlStreamReader` sans résolveur ; une entité inconnue
est une erreur de lecture, pas un accès réseau) ; profondeur d'imbrication bornée à 64, taille à 256 Kio (fichiers
reçus). Pas de QtGui.

**Terminé quand.** Tests nommés verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.** `tests/tst_core_equation.cpp` :
  ```cpp
  #include <actcraft/core/equation.h>

  #include <QtTest>

  using namespace actcraft::core;

  class TstCoreEquation : public QObject
  {
      Q_OBJECT
  private slots:
      void templatesRoundTrip_data()
      {
          QTest::addColumn<QString>("mathml");
          QTest::newRow("fraction") << "<math><mfrac><mn>1</mn><mn>2</mn></mfrac></math>";
          QTest::newRow("puissance") << "<math><msup><mi>x</mi><mn>2</mn></msup></math>";
          QTest::newRow("indice") << "<math><msub><mi>a</mi><mi>n</mi></msub></math>";
          QTest::newRow("racine") << "<math><msqrt><mi>x</mi></msqrt></math>";
          QTest::newRow("racine-n") << "<math><mroot><mi>x</mi><mn>3</mn></mroot></math>";
          QTest::newRow("parentheses") << "<math><mrow><mo>(</mo><mi>a</mi><mo>+</mo><mi>b</mi><mo>)</mo></mrow></math>";
          QTest::newRow("somme") << "<math><munderover><mo>&#x2211;</mo><mrow><mi>i</mi><mo>=</mo><mn>1</mn></mrow><mi>n</mi></munderover></math>";
          QTest::newRow("matrice") << "<math><mtable><mtr><mtd><mn>1</mn></mtd><mtd><mn>0</mn></mtd></mtr><mtr><mtd><mn>0</mn></mtd><mtd><mn>1</mn></mtd></mtr></mtable></math>";
      }
      void templatesRoundTrip()
      {
          QFETCH(QString, mathml);
          QString error;
          const auto tree = parseMathml(mathml.toStdString(), error);
          QVERIFY2(tree.has_value(), qPrintable(error));
          const std::string again = toMathml(*tree);
          const auto tree2 = parseMathml(again, error);
          QVERIFY(tree2.has_value());
          QCOMPARE(toMathml(*tree2), again); // forme canonique stable
      }
      void unknownElementKeptVerbatim()
      {
          const std::string in = "<math><mrow><mi>a</mi><menclose notation=\"box\"><mi>b</mi></menclose></mrow></math>";
          QString error;
          const auto tree = parseMathml(in, error);
          QVERIFY(tree.has_value());
          QVERIFY(toMathml(*tree).find("<menclose notation=\"box\"><mi>b</mi></menclose>") != std::string::npos);
      }
      void entitiesDecoded()
      {
          QString error;
          const auto tree = parseMathml("<math><mi>&#x3B1;</mi></math>", error);
          QVERIFY(tree.has_value());
          QCOMPARE(QString::fromStdString(tree->children.front().text), QStringLiteral("α"));
      }
      void malformedIsAnErrorNotACrash()
      {
          QString error;
          QVERIFY(!parseMathml("<math><mfrac><mn>1</mn></math>", error).has_value());
          QVERIFY(!error.isEmpty());
          QVERIFY(!parseMathml(std::string(300 * 1024, 'a'), error).has_value());
      }
      void tooDeepIsAnError()
      {
          std::string s = "<math>";
          for (int i = 0; i < 80; ++i) s += "<mrow>";
          for (int i = 0; i < 80; ++i) s += "</mrow>";
          s += "</math>";
          QString error;
          QVERIFY(!parseMathml(s, error).has_value());
      }
      void untouchedEquationKeepsItsBytes()
      {
          Equation e;
          e.mathml = "<math xmlns=\"http://www.w3.org/1998/Math/MathML\">  <mn>1</mn></math>";
          const std::string before = e.mathml;
          QString error;
          QVERIFY(parseMathml(e.mathml, error).has_value()); // lire pour dessiner ne réécrit rien
          QCOMPARE(e.mathml, before);
      }
  };

  QTEST_GUILESS_MAIN(TstCoreEquation)
  #include "tst_core_equation.moc"
  ```
  Déclarer `tst_core_equation` (`LIBS ActCraft::core`).
- [ ] **Étape 2 : modèle.** `core/include/actcraft/core/equation.h` :
  ```cpp
  #pragma once

  #include <QString>

  #include <optional>
  #include <string>
  #include <vector>

  namespace actcraft::core {

  // Arbre d'équation (F-301) ; sous-ensemble de MathML présentation. Placeholder : case vide de l'éditeur.
  struct EquationNode
  {
      enum class Kind { Math, Row, Identifier, Number, Operator, Text, Fraction, Sup, Sub, SubSup, Sqrt, Root,
                        UnderOver, Under, Over, Table, TableRow, TableCell, Placeholder, Raw };
      Kind kind = Kind::Row;
      std::string text;                  // Identifier, Number, Operator, Text : contenu UTF-8 ; Raw : XML d'origine
      std::vector<EquationNode> children;
  };

  inline constexpr std::size_t kMaxMathmlBytes = 256 * 1024;
  inline constexpr int kMaxEquationDepth = 64;

  std::optional<EquationNode> parseMathml(const std::string& mathml, QString& error);
  std::string toMathml(const EquationNode& root); // forme canonique : sans espaces, entités en caractères UTF-8
                                                   // sauf < > & échappés ; Raw recopié tel quel

  } // namespace actcraft::core
  ```
  `core/src/equation.cpp` : lecteur récursif sur `QXmlStreamReader` (éléments reconnus : `math`, `mrow`, `mi`, `mn`,
  `mo`, `mtext`, `mfrac`, `msup`, `msub`, `msubsup`, `msqrt`, `mroot`, `munderover`, `munder`, `mover`, `mtable`,
  `mtr`, `mtd`, et `mspace`/`mstyle` lus comme `Raw`), arité contrôlée (`mfrac` : 2 enfants, etc. ; sinon erreur),
  tout autre élément → `Raw` dont le texte est la sous-chaîne d'origine (positions `characterOffset` de début et de
  fin) ; écrivain canonique sur `QXmlStreamWriter` sans indentation.
- [ ] **Étape 3 : preuves et commit.** Preuve de tâche, `NOMS OK` ; commit « Lot 7 : équation, arbre et MathML lus et
  écrits sans perte (F-301) (L7-T24) ».

<a id="tache-l7-t25"></a>
### Tâche L7-T25 — Équation II : mise en page et dessin

**Objectif.** Mettre en page l'arbre (boîtes : largeur, hauteur au-dessus et au-dessous de la ligne de base) avec
`QFontMetricsF` à la taille `Equation::pointSize`, et le dessiner avec `QPainter` (peintre partagé, DA5) : fraction
(barre, numérateur et dénominateur centrés, réduits à 80 %), puissance et indice (réduits à 70 %, montés ou
descendus), racine (signe tracé, barre au-dessus du contenu, indice de la racine n-ième), parenthèses étirées à la
hauteur du contenu, grands opérateurs avec bornes au-dessus et au-dessous, matrice (colonnes alignées), case vide
(rectangle pointillé, seulement dans l'éditeur). Une équation illisible garde le repère neutre. Les deux chemins de
dessin (`pagepainter.cpp` et `paintedrenderer.cpp`) appellent déjà `paintEquation` depuis L7-T1 : seul
`equationlayout.cpp` change.

**Fichiers.** Voir la table (ligne L7-T25). `render/src/equationlayout.h` et `render/src/equationlayout.cpp` ont été
créés et déclarés par L7-T1 (`paintEquation` qui dessine le repère neutre) : cette tâche les remplit.

**Test et commande bornée.** Commande ciblée `-R tst_render_equation`, preuve de tâche (N ≥ N0 + 23), banc sans écran.

**Interdits.** Aucune police ni glyphe de l'original ; police du système (N2-3) ; le MathML est relu à chaque dessin
seulement si l'arbre mis en cache (clé : `std::hash` du texte) a changé — pas de relecture par image.

**Terminé quand.** Tests nommés verts ; `paintedMatchesSoftware` (même image à 2 niveaux près par les deux chemins) ;
`NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.** `tests/tst_render_equation.cpp` (sur le modèle de `tests/tst_render_painter.cpp` :
  `QImage` 400 × 200 blanche, `QPainter`, `paintEquation`) : `fractionIsTallerThanItsParts` (`layoutEquation`
  d'une fraction : hauteur > 1,8 × hauteur d'un chiffre), `superscriptIsRaised` (ligne de base de l'exposant au-dessus
  de celle de la base), `rootCoversItsContent` (largeur de la racine > largeur du contenu + 0,5 em),
  `fractionBarIsDrawn` (une ligne de pixels sombres à mi-hauteur sur la largeur de la fraction),
  `malformedDrawsTheNeutralFrame` (même image que `paintUnknown` sur la même boîte), `paintedMatchesSoftware`.
  Déclarer `tst_render_equation` (`LIBS ActCraft::render`).
- [ ] **Étape 2 : code.** `render/src/equationlayout.h` : `struct EquationBox { double width, ascent, descent; …
  enfants placés }`, `EquationBox layoutEquation(const core::EquationNode&, const QFont&)`, `void paintEquation(QPainter&,
  const core::Equation&)` (lecture `parseMathml`, cache local par texte, mise à l'échelle dans `bounds`) ;
  `render/src/equationlayout.cpp`.
- [ ] **Étape 3 : preuves et commit.** Preuve de tâche, banc sans écran, `NOMS OK` ; commit « Lot 7 : mise en page et
  dessin des équations (F-301) (L7-T25) ».

<a id="tache-l7-t26"></a>
### Tâche L7-T26 — Équation III : éditeur à modèles

**Objectif.** `insert.equation` ouvre un éditeur : palettes de modèles (liste de L7-T0 ; à défaut : fraction,
puissance, indice, racine carrée, racine n-ième, parenthèses, valeur absolue, somme, intégrale, matrice 2 × 2,
lettres grecques, opérateurs ±, ×, ÷, ≤, ≥, ≠, ∞), saisie au clavier dans la case courante, effacement, flèches pour
passer d'une case à l'autre, aperçu dessiné par `paintEquation`, « OK » insère l'équation (couche intermédiaire, au
centre de la vue, une étape), « Annuler » ferme.

**Fichiers.** Voir la table (ligne L7-T26).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_equationeditor|tst_ui_presentation'`, preuve de tâche
(N ≥ N0 + 24).

**Interdits.** Aucun nom de modèle, icône ni libellé de l'original ; boutons de modèles dessinés par l'aperçu lui-même
(une petite équation par bouton).

**Terminé quand.** Tests nommés verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.** `tests/tst_features_equationeditor.cpp` :
  ```cpp
  void fractionTemplateThenTwoDigits()
  {
      EquationEditor e;
      e.insertTemplate(QStringLiteral("fraction"));
      e.type(QStringLiteral("1"));
      e.moveRight();                       // case suivante : dénominateur
      e.type(QStringLiteral("2"));
      QCOMPARE(QString::fromStdString(e.mathml()), QStringLiteral("<math><mfrac><mn>1</mn><mn>2</mn></mfrac></math>"));
  }
  void backspaceLeavesAPlaceholder()
  {
      EquationEditor e;
      e.insertTemplate(QStringLiteral("power"));
      e.type(QStringLiteral("x"));
      e.backspace();
      QVERIFY(e.hasPlaceholder());
  }
  void arrowsWalkTheTree()
  {
      EquationEditor e;
      e.insertTemplate(QStringLiteral("fraction"));
      QCOMPARE(e.cursorPath(), (std::vector<int>{0, 0}));
      e.moveRight();
      QCOMPARE(e.cursorPath(), (std::vector<int>{0, 1}));
      e.moveLeft();
      QCOMPARE(e.cursorPath(), (std::vector<int>{0, 0}));
  }
  void digitsGroupIntoOneNumber()
  {
      EquationEditor e;
      e.type(QStringLiteral("1"));
      e.type(QStringLiteral("2"));
      QCOMPARE(QString::fromStdString(e.mathml()), QStringLiteral("<math><mn>12</mn></math>"));
  }
  void loadThenEditKeepsTheTree()
  {
      EquationEditor e;
      QString error;
      QVERIFY(e.load("<math><msqrt><mi>x</mi></msqrt></math>", error));
      e.moveRight();
      e.type(QStringLiteral("+"));
      e.type(QStringLiteral("1"));
      QVERIFY(e.mathml().find("<msqrt>") != std::string::npos);
  }
  ```
  Déclarer `tst_features_equationeditor`.
- [ ] **Étape 2 : modèle d'édition.** `features/include/actcraft/features/equationeditor.h`, `features/src/equationeditor.cpp` :
  `class EquationEditor` sur `core::EquationNode` (curseur = chemin d'indices ; `templates()` : liste `{id, label,
  mathml}` ; `insertTemplate(id)`, `type(text)` (chiffres groupés en `mn`, lettres en `mi`, autres en `mo`),
  `backspace`, `moveLeft/Right/Up/Down`, `load(mathml, error)`, `mathml()`, `hasPlaceholder()`, `cursorPath()`).
  `features/CMakeLists.txt`.
- [ ] **Étape 3 : interface.** `ui/src/equationpreviewitem.h/.cpp` (`EquationPreview`, `QQuickPaintedItem`, propriété
  `mathml`, `cursorPath`, dessine par `paintEquation` avec les cases vides et le curseur) ;
  `ui/src/equationcontroller.h/.cpp` (`EquationController` : éditeur exposé à QML, `open()`, `accept()` →
  `documentController->insertObject(core::Equation{…})`, `cancel()`) ; `ui/qml/EquationEditor.qml`
  (`FloatingPanel` : palettes, aperçu, OK, Annuler ; frappe au clavier vers `type`) ; gestionnaire `insert.equation`
  dans `Presentation.qml` ; `ui/presentation.cmake`.
- [ ] **Étape 4 : test d'interface.** `insertEquationIsOneUndoStep` dans `tst_ui_presentation.cpp`.
- [ ] **Étape 5 : preuves et commit.** Preuve de tâche, banc sans écran, `NOMS OK` ; commit « Lot 7 : éditeur
  d'équations à modèles (F-301) (L7-T26) ».

<a id="tache-l7-t27"></a>
### Tâche L7-T27 — Équation IV : réédition, aller-retour, `.flipchart`, S24

**Objectif.** `equation.edit <id>` rouvre l'éditeur sur l'équation d'identifiant `id` de la page affichée ; « OK »
la remplace **sous le même identifiant** (`replaceObjects({id}, {équation avec common.id = id})`), une étape ;
aller-retour `.actcraft` identique ; une équation lue d'un `.flipchart` synthétique s'affiche ; scénario S24 (le
double-clic vient en L7-T34).

**Fichiers.** Voir la table (ligne L7-T27).

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_presentation|tst_captures_equation'`, preuve de tâche (N ≥ N0 +
25), preuve visuelle `-Scenario equation`.

**Preuve visuelle.** `bench/scenarios/equation.json` : `insert.equation` ; touches injectées (modèle « fraction » par
son raccourci de l'éditeur, `1`, flèche droite, `2`, Entrée) ; `waitReady` ; `checkPixel` encre sur la barre de
fraction ; `remember` `documentController.fingerprint` comme `avant` ; `fixture` vide puis `command` `file.save-as`
`${SORTIE}/eq.actcraft`, `file.open` du même fichier ; `expect` `documentController.fingerprint` `${avant}` ; capture
`equation`. Décrire : fraction un demi au centre de la page.

**Interdits.** Comme L7-T26.

**Terminé quand.** `editKeepsTheId`, `equationRoundTripIdentical`, `flipchartEquationIsDrawn`, `tst_captures_equation`
verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : tests rouges.** Les trois tests dans `tst_ui_presentation.cpp` (le `.flipchart` synthétique est
  écrit par `tests/support/flipchartwriter.h` avec l'objet MathML de L7-T1, ouvert par `documentController.openUrl` ;
  un pixel sombre dans la boîte de l'équation).
- [ ] **Étape 2 : code.** `EquationController::edit(quint64 id)`, `accept()` en mode réédition ; gestionnaire
  `equation.edit` (posé par `EquationEditor.qml`, qui reçoit `commands`) ; `bench/scenarios/equation.json`,
  `tst_captures_equation` dans `tests/presentation.cmake`.
- [ ] **Étape 3 : preuves et commit.** Preuve de tâche, preuve visuelle, `NOMS OK` ; commit « Lot 7 : réédition des
  équations et scénario S24 (F-301) (L7-T27) ».

<a id="tache-l7-t28"></a>
### Tâche L7-T28 — Caméra (F-275, bureau)

**Objectif.** Cinq commandes de capture (zone, point par point, main levée, fenêtre, écran entier). Au déclenchement :
l'écran qui porte la fenêtre principale est photographié (`QScreen::grabWindow(0)`), puis une fenêtre de sélection
plein écran, sans cadre, montre l'image figée ; une petite palette à nous offre les destinations « page actuelle »,
« nouvelle page », « presse-papiers » (les destinations « ressources » arrivent au lot 6, H7-3) et, pour la zone,
gauche, haut, largeur, hauteur. Zone : rectangle pointillé déplaçable et redimensionnable ; point par point : chaque
clic ajoute un sommet, un clic près du premier ferme ; main levée : le relâché ferme par une droite ; fenêtre : survol
qui encadre la fenêtre sous le pointeur (liste des fenêtres de Windows ; ailleurs, écran entier) ; écran entier :
capture immédiate. Image PNG rangée comme ressource du document (`addResource`) et posée par `insertObject` (ou
`insertObjectOnNewPage`), une étape.

**Fichiers.** Voir la table (ligne L7-T28).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_camera|tst_ui_presentation'`, preuve de tâche (N ≥ N0 +
26), banc sans écran. Contrôle à l'écran (jeton `ecran`, annoncé) : `actcraft.exe` lancé, `tool.camera-screen` par la
ligne de commande de scénario, image posée sur la page (lecture `LU`), fermeture par PID.

**Interdits.** Code propre à Windows (énumération des fenêtres : `EnumWindows`, `IsWindowVisible`,
`DwmGetWindowAttribute`) dans `features/src/camera_win.cpp` seulement, construit sous `WIN32`. Aucune capture réelle
dans les tests automatiques (image injectée). La fenêtre de sélection ne reste jamais ouverte sans moyen de sortie :
Échap la ferme.

**Terminé quand.** Tests nommés verts ; `cameraAbsentOnAndroid` (menus filtrés sous `Platform::Android` sans les cinq
commandes) ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.** `tests/tst_features_camera.cpp` :
  ```cpp
  void clickNearFirstPointClosesThePolygon()
  {
      PolygonPicker p;
      p.click(QPointF(10, 10));
      p.click(QPointF(100, 10));
      p.click(QPointF(100, 100));
      QVERIFY(!p.closed());
      p.click(QPointF(14, 12));            // à moins de kClosePixels du premier
      QVERIFY(p.closed());
      QCOMPARE(p.polygon().size(), 3);
  }
  void freehandClosesOnRelease()
  {
      FreehandPicker f;
      f.press(QPointF(0, 0));
      f.move(QPointF(50, 0));
      f.move(QPointF(50, 50));
      f.release();
      QVERIFY(f.closed());
      QCOMPARE(f.polygon().first(), f.polygon().last());
  }
  void cropOutsideIsTransparent()
  {
      QImage screen(100, 100, QImage::Format_ARGB32_Premultiplied);
      screen.fill(Qt::red);
      const QImage out = cropToPolygon(screen, QPolygonF({{10, 10}, {90, 10}, {50, 90}}));
      QCOMPARE(out.size(), QSize(80, 80));
      QCOMPARE(qAlpha(out.pixel(1, 79)), 0);     // coin bas gauche : hors du triangle
      QCOMPARE(QColor(out.pixel(40, 20)), QColor(Qt::red));
  }
  void rectangleNormalized()
  {
      QCOMPARE(normalizedRect(QPointF(50, 50), QPointF(10, 20)), QRectF(10, 20, 40, 30));
  }
  ```
  Déclarer `tst_features_camera`.
- [ ] **Étape 2 : logique.** `features/include/actcraft/features/camera.h`, `features/src/camera.cpp`
  (`PolygonPicker`, `FreehandPicker`, `cropToPolygon`, `normalizedRect`, `kClosePixels = 10`, `std::vector<QRect>
  topLevelWindows()`) ; `features/src/camera_win.cpp` (réalisation Windows) et `features/src/camera_other.cpp` (liste
  vide), choisis par CMake comme en L7-T22 ; `features/CMakeLists.txt` (`dwmapi` sous `WIN32`).
- [ ] **Étape 3 : interface.** `ui/src/cameracontroller.h/.cpp` (`CameraController` : `mode`, `frozen` (image),
  `setFrozenImageForTest`, `deliver(int destination)` → ressource PNG + objet image (`core::ImageObject` avec
  `resource`, `bounds` = taille de l'image en pixels de page, centrée dans la vue) ou presse-papiers) ;
  `ui/qml/CameraOverlay.qml` (fenêtre `Window` plein écran, `flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint`,
  image figée, sélection dessinée) ; `ui/qml/CameraPalette.qml` ; cinq gestionnaires dans `Presentation.qml` ;
  `ui/presentation.cmake` ; `ui/icons/camera.svg`.
- [ ] **Étape 4 : tests d'interface.** `captureToPageAddsOneImage`, `captureToNewPageAddsAPage`, `captureToClipboard`,
  `cameraAbsentOnAndroid`, `escapeClosesTheSelection`.
- [ ] **Étape 5 : preuves et commit.** Preuve de tâche, banc sans écran, contrôle à l'écran annoncé, `NOMS OK` ;
  commit « Lot 7 : caméra, cinq captures et trois destinations (F-275) (L7-T28) ».

<a id="tache-l7-t29"></a>
### Tâche L7-T29 — Clavier à l'écran (F-294, bureau)

**Objectif.** `tool.keyboard` (Ctrl+K) ouvre **notre** clavier flottant (fenêtre toujours devant, déplaçable par sa
barre de titre, qui **ne prend jamais le focus** : `Qt::WindowDoesNotAcceptFocus | Qt::WindowStaysOnTopHint |
Qt::Tool`) : disposition française AZERTY, mode standard ou étendu (chiffres, accents, symboles), nombre de touches
réglable (compact ou complet), majuscule et verrouillage. Chaque touche envoie son caractère au champ de texte qui a
le focus dans ActCraft (`QGuiApplication::focusObject`, `QKeyEvent` envoyé) ou, si une autre application est au
premier plan, à celle-ci (Windows : `SendInput` en `KEYEVENTF_UNICODE`). Fermeture par la croix. Pas de Qt Virtual
Keyboard (GPL, décision 34).

**Fichiers.** Voir la table (ligne L7-T29).

**Test et commande bornée.** Commande ciblée `-R 'tst_features_keyboard|tst_ui_presentation'`, preuve de tâche (N ≥ N0 +
27).

**Interdits.** `SendInput` dans `features/src/keyinjector_win.cpp` seulement ; jamais appelé par les tests
automatiques (injecteur remplaçable). Aucune frappe n'est journalisée.

**Terminé quand.** Tests nommés verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.** `tests/tst_features_keyboard.cpp` :
  ```cpp
  void shiftTypesCapitals()
  {
      ScreenKeyboard k(ScreenKeyboard::Layout::Standard);
      QCOMPARE(k.press(QStringLiteral("a")).text, QStringLiteral("a"));
      k.press(QStringLiteral("shift"));
      QCOMPARE(k.press(QStringLiteral("a")).text, QStringLiteral("A"));
      QCOMPARE(k.press(QStringLiteral("a")).text, QStringLiteral("a")); // majuscule pour une touche
  }
  void capsLockStays()
  {
      ScreenKeyboard k(ScreenKeyboard::Layout::Standard);
      k.press(QStringLiteral("caps"));
      QCOMPARE(k.press(QStringLiteral("z")).text, QStringLiteral("Z"));
      QCOMPARE(k.press(QStringLiteral("z")).text, QStringLiteral("Z"));
  }
  void compactLayoutHasFewerKeys()
  {
      QVERIFY(ScreenKeyboard(ScreenKeyboard::Layout::Compact).keys().size()
              < ScreenKeyboard(ScreenKeyboard::Layout::Extended).keys().size());
  }
  void accentKeysTypeFrench()
  {
      ScreenKeyboard k(ScreenKeyboard::Layout::Extended);
      QCOMPARE(k.press(QStringLiteral("é")).text, QStringLiteral("é"));
      QCOMPARE(k.press(QStringLiteral("backspace")).key, Qt::Key_Backspace);
  }
  ```
  Déclarer `tst_features_keyboard`.
- [ ] **Étape 2 : logique.** `features/include/actcraft/features/screenkeyboard.h`, `features/src/screenkeyboard.cpp`
  (`ScreenKeyboard` : `Layout { Compact, Standard, Extended }`, `keys()` (lignes de touches `{id, label, width}`),
  `press(id)` → `KeyOutput { QString text; Qt::Key key; }`, état majuscule) ; `features/src/keyinjector_win.cpp`
  (`sendUnicodeToForeground(QString)`) et `features/src/keyinjector_other.cpp` (rend faux), choisis par CMake comme en
  L7-T22 ; `features/CMakeLists.txt`.
- [ ] **Étape 3 : interface.** `ui/src/keyboardcontroller.h/.cpp` (`KeyboardController` : `layout`, `press(id)` →
  envoi au focus d'ActCraft si la fenêtre principale est active, sinon à l'injecteur ; injecteur remplaçable pour les
  tests) ; `ui/qml/ScreenKeyboard.qml` (`Window` aux trois drapeaux, touches dessinées par nous) ; gestionnaire
  `tool.keyboard` ; `ui/presentation.cmake` ; `ui/icons/clavier.svg`.
- [ ] **Étape 4 : tests d'interface.** `keyboardTypesIntoTheFocusedField` (un `TextField` de test a le focus : trois
  touches → son texte), `keyboardWindowNeverTakesFocus` (drapeau présent, `QGuiApplication::focusWindow()` inchangée
  après un appui), `keyboardAbsentOnAndroid`.
- [ ] **Étape 5 : preuves et commit.** Preuve de tâche, banc sans écran, `NOMS OK` ; commit « Lot 7 : clavier à
  l'écran écrit par nous (F-294) (L7-T29) ».

<a id="tache-l7-t30"></a>
### Tâche L7-T30 — Annotation de bureau (F-070, F-071, bureau)

**Objectif.** `desktop.annotate` (Ctrl+Shift+A, cochable) ouvre, au-dessus du bureau de l'écran principal, une
fenêtre sans cadre, transparente, toujours devant, qui porte son propre paperboard (« Annotation du bureau », un
`DocumentController` à part, fond transparent, jamais enregistré sans demande) ; la palette principale reste
accessible (elle est dans la fenêtre principale, gardée devant l'annotation) ; le stylo y écrit avec la couleur et
l'épaisseur de la palette (liaisons de propriétés vers `pageArea.view`, sans modifier les gestionnaires de
`Main.qml`). Bouton « laisser passer les clics » (à nous) : la fenêtre devient transparente aux entrées
(`Qt::WindowTransparentForInput`) ; avec l'outil de sélection du lot 3, ce passage sera automatique (L7-T34). Désactivé
pendant l'annotation : effets de page (bloqueur de L7-T7). Fonds de page (F-071) : « capture du bureau » (image figée
du bureau en fond de la page affichée, ressource du document, une étape) et « superposition du bureau » (fond
transparent sur le bureau vivant, non enregistré) — commandes `page.background-desktop-capture` et
`page.background-desktop-overlay` déclarées par L7-T1.

**Fichiers.** Voir la table (ligne L7-T30).

**Test et commande bornée.** Commande ciblée `-R tst_ui_desktop`, preuve de tâche (N ≥ N0 + 28), banc sans écran.
Contrôle à l'écran (jeton `ecran`, annoncé) : fenêtre ouverte 3 s, capture de l'écran principal montrant le bureau à
travers, fermeture par PID.

**Interdits.** Aucune modification de `Main.qml`, `PageArea.qml` (réutilisé tel quel dans la fenêtre d'annotation).
Le paperboard du bureau n'est pas le document ouvert.

**Terminé quand.** Tests nommés verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.** `tests/tst_ui_desktop.cpp` (montage de `tst_ui_presentation`) :
  `annotationWindowIsTransparentAndOnTop` (drapeaux, `color` transparente), `strokeGoesToTheDesktopPaperboard` (un trait
  injecté dans la fenêtre d'annotation : un objet de plus dans **son** document, aucun dans le document ouvert),
  `passThroughMakesTheWindowTransparentForInput`, `desktopCaptureBecomesTheBackground` (image injectée : fond de la
  page = ressource PNG, une étape), `overlayBackgroundIsNotSaved` (enregistrer : fond d'origine dans le fichier),
  `transitionsBlockedWhileAnnotating`. Déclarer `tst_ui_desktop` dans `tests/presentation.cmake`.
- [ ] **Étape 2 : code.** `ui/src/desktopcontroller.h/.cpp` (`DesktopController` : `annotating`, `passThrough`,
  `desktopDocument` (`DocumentController*` possédé), `captureBackground()`, `overlayBackground` (bool), image
  injectable pour les tests) ; `ui/qml/DesktopAnnotation.qml` (`Window` sans cadre, transparente, `PageArea` à
  l'intérieur, `documentController: desktop.desktopDocument`, `view.inkColor`/`inkWidth` liés à ceux de la vue
  principale, bouton de passage, bouton de fermeture) ; gestionnaires `desktop.annotate`,
  `page.background-desktop-capture`, `page.background-desktop-overlay` ; `ui/presentation.cmake` ;
  `ui/icons/bureau.svg`.
- [ ] **Étape 3 : preuves et commit.** Preuve de tâche, banc sans écran, contrôle à l'écran annoncé, `NOMS OK` ;
  commit « Lot 7 : annotation du bureau et fonds de bureau (F-070, F-071) (L7-T30) ».

<a id="tache-l7-t31"></a>
### Tâche L7-T31 — Outils de bureau (F-080, bureau)

**Objectif.** `desktop.tools` (F6) réduit la fenêtre principale et affiche une icône flottante toujours devant,
déplaçable ; un clic déploie une roue d'outils dans le sens horaire depuis le haut : revenir à ActCraft
(`desktop.restore`), caméra (anneau secondaire : les cinq captures), clavier à l'écran, autres outils (anneau
secondaire : message défilant, horloge, calculatrice, dés). Les entrées de vote et de services de l'éditeur
d'origine sont absentes ; enregistreurs d'écran et de son ajoutés au lot 8. Revenir à ActCraft ferme la roue ; la roue
se ferme aussi si la fenêtre est restaurée autrement.

**Fichiers.** Voir la table (ligne L7-T31).

**Test et commande bornée.** Commande ciblée `-R tst_ui_desktop`, preuve de tâche (N ≥ N0 + 28).

**Interdits.** Chaque entrée de la roue déclenche une commande nommée existante (aucune action codée dans la roue).

**Terminé quand.** Tests nommés verts ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : test rouge.** Dans `tests/tst_ui_desktop.cpp` : `wheelIsClockwiseFromTop` (`wheelPositions(4, 100)`
  : premier point à (0, −100), deuxième à (100, 0) ± 0,01), `restoreClosesTheWheel`, `wheelClosesWhenTheWindowIsRestored`
  (fenêtre principale `showNormal()` : roue fermée), `wheelHasNoVoteNorVendorEntry` (les identifiants de la roue sont
  tous des commandes déclarées, aucune hors des préfixes `desktop.`, `tool.`).
- [ ] **Étape 2 : code.** `features/include/actcraft/features/desktopwheel.h`, `features/src/desktopwheel.cpp`
  (`std::vector<QPointF> wheelPositions(int count, double radius)`, sens horaire depuis le haut ; `struct WheelEntry
  { QString command; std::vector<WheelEntry> ring; }`, `defaultWheel()`) ; `DesktopController` : `toolsShown`,
  `showTools()`, `restore()` ; `ui/qml/DesktopToolsWheel.qml` (`Window` toujours devant, sans cadre, icône et roue
  dessinées par nous) ; gestionnaires `desktop.tools`, `desktop.restore` ; `ui/presentation.cmake` ;
  `features/CMakeLists.txt`.
- [ ] **Étape 3 : preuves et commit.** Preuve de tâche, banc sans écran, `NOMS OK` ; commit « Lot 7 : outils de bureau
  et leur roue (F-080) (L7-T31) ».

<a id="tache-l7-t32"></a>
### Tâche L7-T32 — Contrôle du lot (S10 de bout en bout)

**Objectif.** Prouver que le lot est complet et cohérent : chaque commande déclarée par L7-T1 a un gestionnaire
(hors celles de L7-T34, listées) ; les menus Android ne contiennent aucune commande de bureau ni d'écriture ;
scénario `presentation` de bout en bout (S10) ; liste des épinglages des profils pour le lot 6.

**Fichiers.** Voir la table (ligne L7-T32).

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_presentation|tst_captures_presentation'`, preuve de tâche
(N ≥ N0 + 29), preuve visuelle `-Scenario presentation -Lire "pageIndicator.text=2 / 2"`.

**Preuve visuelle.** `bench/scenarios/presentation.json` (S10) : `present.curtain` ; `checkPixel` noir au centre ;
`pointer` depuis le haut jusqu'au milieu ; `checkPixel` page en haut, noir en bas ; `present.curtain` (fermé) ;
`math.ruler` ; `stroke` le long du bord ; `checkPixel` encre sur le bord ; réglage d'un compte à rebours de 3 s
(touches dans le panneau) avec « page suivante » ; `waitFor` `pageIndicator.text` `2 / 2` (6 s au plus) ; captures
`rideau`, `regle`, `apres-compte-a-rebours`. La partie « écriture » de S10 est dans `handwritingBecomesTextInOneStep`
(sautée et signalée sans reconnaisseur). Décrire chaque capture.

**Interdits.** Ne rien ajouter aux fonctions : une commande sans gestionnaire est un rapport à l'orchestrateur, pas un
gestionnaire bouche-trou.

**Terminé quand.** `everyLot7CommandHasAHandler` (les 43 commandes de L7-T1, sauf `recognition.convert-to-text`
quand le reconnaisseur manque — gestionnaire présent mais commande grisée), `androidMenusHaveNoDesktopCommand`,
`tst_captures_presentation` verts ; fichier des épinglages écrit ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : tests.** `everyLot7CommandHasAHandler` (liste des 43 identifiants recopiée de L7-T1 ;
  `appCommands.registry().hasHandler(id)`), `androidMenusHaveNoDesktopCommand` (`setPlatform(Android)`, `reload()`,
  parcourir `menus()` et `palette()` : aucun identifiant `tool.camera-*`, `tool.keyboard`, `desktop.*`,
  `tool.handwriting`, `recognition.convert-to-text`).
- [ ] **Étape 2 : scénario.** `bench/scenarios/presentation.json`, `tst_captures_presentation` dans
  `tests/presentation.cmake`.
- [ ] **Étape 3 : épinglages pour le lot 6.** `docs/verifications/AAAA-MM-JJ-lot7-epinglages.md` : pour chaque
  profil fourni décrit par L7-T0 ou la spécification (`fonctionnalites.md`, F-880), les commandes du lot 7 qu'il
  épingle (identifiants stables) et la raison ; à défaut d'information, la liste des commandes du lot 7 proposées à
  l'épinglage (palette du profil « mathématiques » : `math.*`, `tool.dice`, `tool.calculator` ; profil « au tableau » :
  `present.*`, `view.page-zoom`, `tool.clock`).
- [ ] **Étape 4 : preuves et commit.** Preuve de tâche, preuve visuelle, `NOMS OK` ; commit « Lot 7 : contrôle du lot
  et scénario de présentation de bout en bout (S10) (L7-T32) ».

<a id="tache-l7-t33"></a>
### Tâche L7-T33 — Fusion avec le lot 3

**Objectif.** Quand le lot 3 est livré dans `feat/livraison-complete` (l'orchestrateur B le lit dans l'état du
chantier A), fusionner `feat/livraison-complete` dans `feat/livraison-complete-b`, résoudre les conflits, rejouer les
suites ; le lot 7 est alors prêt à être fusionné dans `feat/livraison-complete`.

**Fichiers.** Les fichiers en conflit. Conflits attendus, tous sur des points réservés (le résoudre = garder les deux
ajouts, dans l'ordre indiqué) :
- `actions/data/commands.json` : commandes du lot 3, puis celles du lot 7 ; aucun raccourci en double
  (`lot7CommandsDeclared` le vérifie ; un double est tranché par F-750 et F-950 à F-953 de la spécification et rapporté).
- `actions/data/layout.json` : menu `tools` remis dans l'ordre de F-750 (stylo, marqueur, gomme… du lot 3 à leur place
  ; entrées du lot 7 comme posées par L7-T2) ; si le lot 3 a aussi ajouté des sous-menus (« Effacer ▸ »), une seule
  forme est gardée — celle de L7-T2 si le lot 3 n'en a pas défini, sinon la plus ancienne mergée, et l'autre lot est
  adapté (H7-11).
- `actions/…/commandregistry.h`, `actions/src/commandregistry.cpp` : énumérations ajoutées en fin par les deux lots.
- `core/include/actcraft/core/model.h`, `core/src/model.cpp`, `core/src/edits.cpp`, `core/CMakeLists.txt` ;
  `formats/src/nativejson.cpp` (si le lot 3 y a touché) ; `render/src/pagepainter.cpp`, `render/src/paintedrenderer.cpp`.
- `ui/…/pageview.h`, `ui/src/pageview.cpp` (filtre d'échantillons du lot 7 **avant** l'outil courant du lot 3 : si le
  lot 3 a remplacé l'appel direct au stylo par un routage d'outil, le filtre s'applique à l'entrée du routage, pour le
  stylo et le marqueur — L7-T34 le prouve) ; `ui/…/documentcontroller.h`, `ui/src/documentcontroller.cpp` (bloc
  « lot 7 » gardé entier) ; `ui/qml/PageArea.qml` (`pageOverlays` au-dessus de toute surcouche du lot 3, sauf les
  poignées de sélection si le lot 3 les veut au-dessus : à trancher par la règle « la sélection reste saisissable
  quand un outil de maths ne la recouvre pas ») ; `ui/qml/Main.qml` ; `ui/CMakeLists.txt` (ligne `include` gardée) ;
  `ui/src/commands.cpp`, `ui/qml/CommandMenu.qml`.
- `bench/…/uiscenario.h`, `bench/src/uiscenario.cpp`, `tests/tst_bench_uiscenario.cpp` : étapes ajoutées en fin par les
  deux lots (ordre : lot 3 puis lot 7 ; aucun rang existant ne change, les fichiers JSON ne dépendent pas des rangs).
- `bench/…/scenarios.h`, `bench/src/scenarios.cpp`, `tests/tst_bench_scenarios.cpp` (L7-T21 et scénario « gommer » du
  lot 3, s'il existe : les deux gardés, comptes mis à jour).
- `tests/CMakeLists.txt` (ligne `include(presentation.cmake)` gardée), `tests/tst_core_model.cpp`,
  `tests/tst_actions_registry.cpp`, `tests/tst_actions_layout.cpp`, `tests/tst_ui_commands.cpp`.
- `graphify-out/` : régénéré (`graphify update .`), jamais résolu à la main.

**Test et commande bornée.**
```powershell
git -C <wt> fetch origin
git -C <wt> merge --no-ff origin/feat/livraison-complete -m "Fusion du lot 3 dans le lot 7 (L7-T33)"
git -C <wt> diff --name-only --diff-filter=U
git -C <wt> grep -n -e '^<<<<<<< ' -e '^>>>>>>> ' -- . ':!graphify-out'   # aucune sortie après résolution
```
puis preuve de tâche en debug **et** en release (`-Preset release` dans la commande), N ≥ (tests du lot 3 à sa
livraison) + 29, captures hors écran `-Scenario tous` (`CAPTURES OK`), banc sans écran, `NOMS OK`.

**Interdits.** Jamais `git checkout --theirs` ou `--ours` sur un fichier entier ; jamais un test modifié pour passer
sans comprendre ; jamais `git push --force` ; si un conflit touche un comportement du lot 3 hors des points réservés,
s'arrêter et le rapporter (l'orchestrateur B consulte l'état du chantier A).

**Terminé quand.** Merge commité, aucun marqueur, suites debug et release `TESTS OK`, `CAPTURES OK` sur tous les
scénarios, banc joint, `NOMS OK`, `graphify update .` commité.

**Étapes.**

- [ ] **Étape 1 : préalable.** Vérifier dans `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete\docs\sessions\livraison-complete-etat.md`
  (lecture seule) que le lot 3 est déclaré livré dans `feat/livraison-complete` (tâches mergées, clôture faite) ; sinon
  la tâche attend.
- [ ] **Étape 2 : merge.** Les deux premières commandes de « Test et commande bornée ».
- [ ] **Étape 3 : résolution.** Un fichier à la fois, dans l'ordre de la liste ; `git -C <wt> add <fichier>` après
  chaque résolution ; `graphify-out/` régénéré en dernier.
- [ ] **Étape 4 : suites.** Debug, release, captures `tous`, banc sans écran, `NOMS OK`.
- [ ] **Étape 5 : commit.** `git -C <wt> commit` (message du merge déjà posé, terminé par la ligne `Co-Authored-By`
  ajoutée par `-m`) ; `graphify update .` puis `git -C <wt> add graphify-out` et commit « Graphe du code après la
  fusion du lot 3 (L7-T33) ».

<a id="tache-l7-t34"></a>
### Tâche L7-T34 — Après fusion du lot 3

**Objectif.** Brancher sur la sélection, la rotation, le marqueur, la gomme et la commande de rétablissement de page
du lot 3 les comportements du lot 7 qui en dépendent :
1. **Origine XY (F-284)** : tant qu'elle est affichée, la poignée de rotation du lot 3 fait tourner l'objet ou le groupe
   sélectionné **autour de l'origine** (et non de son centre).
2. **Convertir en forme / en texte (F-295, F-296 a posteriori)** : `recognition.convert-to-shape` et
   `recognition.convert-to-text` agissent sur les annotations sélectionnées (une étape) ; sans sélection, sur les
   derniers traits de la session (comportement de L7-T21 et L7-T23).
3. **Double-clic sur une équation (F-301)** avec l'outil de sélection : `equation.edit <id>`.
4. **Compte à rebours (F-290)** : l'action de fin « rétablir la page » appelle la commande du lot 3.
5. **Guides (F-280 à F-283)** : le marqueur suit les bords comme le stylo ; la gomme suit la mine du compas.
6. **Annotation de bureau (F-070)** : avec l'outil de sélection, les clics traversent vers le bureau (passage
   automatique ; le bouton de L7-T30 reste).

**Fichiers.** Voir la table (ligne L7-T34). Les noms des API du lot 3 ne sont pas connus à l'écriture de ce plan : les
localiser **d'abord** par le graphe (`graphify query "rotation de la sélection poignée pivot"`, `graphify query
"outil courant sélection changement d'outil"`, `graphify query "rétablir la page commande"`, `graphify query "routage
des échantillons marqueur gomme"`) et les citer dans le rapport ; si l'un d'eux exige de modifier un fichier du lot 3
au-delà d'un point d'accroche d'une ligne (paramètre de pivot, signal de double-clic), s'arrêter et le rapporter : la
modification revient au lot 3 ou à une tâche ajoutée par l'orchestrateur.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_presentation|tst_ui_desktop'`, preuve de tâche, banc sans
écran.

**Interdits.** Aucun changement de comportement des outils du lot 3 quand aucun outil du lot 7 n'est actif
(`tst_*` du lot 3 inchangés et verts). Pas de nouvelle commande hors de celles de L7-T1.

**Terminé quand.** Les tests nommés de la table verts (`convertSelectionToText` sauté et signalé sans reconnaisseur) ;
suites du lot 3 vertes sans modification ; `NOMS OK`.

**Étapes.**

- [ ] **Étape 1 : localiser.** Les quatre requêtes du graphe ; noter fichier:ligne des API trouvées.
- [ ] **Étape 2 : tests rouges.** Les huit tests nommés de la table, chacun en termes de comportement : par exemple
  `rotationPivotsOnTheOrigin` : un rectangle sélectionné, origine affichée en (100, 100), rotation de 90° par la
  poignée injectée : le centre du rectangle tourne autour de (100, 100) (position calculée à ± 1 pixel) ;
  `markerFollowsTheRuler` : outil marqueur du lot 3, règle ouverte, trait presque le long du bord : tous les points à
  y = 400 ± 0,5.
- [ ] **Étape 3 : brancher.** Un comportement à la fois, test vert après chacun.
- [ ] **Étape 4 : preuves et commit.** Preuve de tâche, banc sans écran, `NOMS OK` ; commit « Lot 7 : fonctions qui
  dépendent du lot 3 (pivot sur l'origine, conversions, double-clic, rétablissement, guides) (L7-T34) ».

## Hypothèses à revoir (porte de fin du lot 7)

- **H7-1** Propriété du texte MathML dans les `.flipchart` : relevée par L7-T0 ; à défaut, première chaîne de
  l'objet (L7-T1). Aucun exemplaire dans `samples/` : lecture non validée sur un vrai paperboard (N1-14).
- **H7-2** Outils de maths par fenêtre, pas par page : ils restent en changeant de page (comme la référence Python ;
  à confirmer par L7-T0). Pas de second utilisateur avant le lot 11 (DA-A1 : un seul utilisateur porte les outils).
- **H7-3** Caméra vers « mes ressources » et « ressources partagées » : au lot 6 (navigateur de ressources).
- **H7-4** Son de fin du compte à rebours : au lot 8 (Qt Multimedia).
- **H7-5** Délai de reconnaissance commun à la forme et à l'écriture (1 à 5 s, défaut 5), sauf si L7-T0 établit que
  les formes se convertissent au relâché.
- **H7-6** Clavier à l'écran écrit par nous (décision 34) plutôt que le clavier du système (choix de la référence
  Python) : licence et cohérence sous Windows ; Android a le sien (commande absente).
- **H7-7** Annotation de bureau : passage des clics par un bouton jusqu'au lot 3, puis automatique avec la sélection.
- **H7-8** Catalogue d'effets : liste de L7-T0 ; à défaut, la liste par défaut de L7-T7 (dix effets à nous).
- **H7-9** Vitesse du zoom continu : ×2 par seconde (à nous, sauf relevé de L7-T0).
- **H7-10** Profils : la liste d'épinglages de L7-T32 remplace la règle « chaque lot complète ses profils » (lot 6
  après le lot 7).
- **H7-11** Sous-menus de la disposition introduits par le lot 7 (L7-T2) ; si le lot 3 en a introduit d'autres
  (« Effacer ▸ »), une seule forme gardée à la fusion (L7-T33).
- **H7-12** Équation en MathML (norme ouverte) : choix de réalisation moderne (décision 0100), signalé au rapport de
  lot ; le jeu de modèles est celui de L7-T0.
- **H7-13** Zoom de page : un redessin du cache au relâché (écart 7 du plan 7a), pas zéro.

## Décisions tranchées par ce plan

1. Outils de maths, rideau, spot, zoom de page, message défilant et effets = surcouches autonomes, sans sélection ni
   gestes du lot 3 (ainsi les réalise la référence Python ; F-284 pivot et conversions a posteriori → L7-T34).
2. Encre guidée par un filtre d'échantillons de la vue, posé une fois (L7-T2) : stylo inchangé.
3. Réservation en deux tâches (données, interface) et fichiers propres au lot (`ui/presentation.cmake`,
   `tests/presentation.cmake`, `ui/qml/Presentation.qml`) : la fusion avec le lot 3 ne porte que sur les points
   réservés, listés en L7-T33.
4. Équation gardée en MathML texte (sans perte par construction ; réécrite seulement si elle est modifiée).
5. Remplacement annulable (`ReplaceObjectsCommand`) dans un fichier neuf de `core/` (pas dans `edits.h`, partagé).
6. Code de plateforme dans `*_win.cpp` de `features/` seulement (écriture, caméra, clavier).

## Clôture du lot 7 (orchestrateur, hors tâches)

Après L7-T34, sur la tête de `feat/livraison-complete-b` :
1. **Double relecture** (`reviewer-adversarial`, deux relecteurs, diff du lot entier contre `feat/livraison-complete`
   après L7-T33).
2. **Suites** debug et release `TESTS OK` ; `NOMS OK` ; `graphify update .` commité.
3. **Banc (DA7)**, jeton `ecran`, annoncé à Jérôme : banc complet D3D11 plein écran, **A/B** contre la tête de
   `feat/livraison-complete` avant la fusion du lot 7 (passages alternés, référence avant et après, RAM relevée) ;
   tout critère tenu par la référence doit l'être encore. **S22** : un passage par effet du catalogue
   (`view.page-effect <id>` puis `page.next` en boucle pendant le scénario), aucune image au-delà de la limite.
   **S23** : banc avec un message défilant en boucle, aucune reprise hors limite. **Reconnaissance** : scénario du banc
   `reconnaissance` (L7-T21), résultat rapporté (limite de DA11 : redessin immédiat). Résumé commité dans
   `docs/mesures/AAAA-MM-JJ-lot7-presentation.md`.
4. **Captures (DA8)** à l'écran en D3D11 : `presentation`, `rideau`, `spot`, `regle`, `effets`, `compte-a-rebours`,
   `message`, `reconnaissance-forme`, `equation`, et ceux des lots précédents.
5. **Paquets** : installateur (`PAQUET OK`, `INSTALLATEUR OK`) et APK (`APK OK`, `ANDROID OK` sur l'émulateur, capture
   des menus : aucune commande de bureau ni d'écriture) ; croissance des tailles rapportée (N1-A10).
6. **`CHANGELOG.md`** : section du lot rédigée **sans numéro** (« À venir — présentation et outils ») ; le numéro est
   posé à la fusion dans `feat/livraison-complete` selon l'ordre de fusion (le lot 3 prend 0.4.0).
7. **Porte de fin** : rapport de lot (écarts des deux plans, hypothèses H7-1 à H7-13, réalisations modernes
   choisies), release proposée, publiée seulement sur go écrit.
8. Fusion de `feat/livraison-complete-b` dans `feat/livraison-complete` (PR du chantier A ou merge local selon la
   règle de l'orchestrateur A).
