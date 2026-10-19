# Livraison complète — lot 11 « Tablette, toucher, finitions, 1.0 », sous-lot 11b — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

Sous-lot **11b** : pages d'aide, contrôle des traductions, recherche de mises à jour, raccourcis restants,
accessibilité, licences tierces, S19 et S32 rejoués sur le produit complet, version 1.0.0, paquets, release prête et
porte de fin. Il suit le sous-lot 11a (`docs/plans/2026-10-18-livraison-complete-lot11-PLAN.md`), dont il reprend
**sans les recopier** : la liste de réservation R1 à R4 (les 32 tests sont déjà enregistrés par L11-T1), la table A des
API supposées, les décisions D11-1 à D11-12, les commandes de preuve communes (commande ciblée, fonctions d'un test,
preuve de tâche avec **N = N11 + 32**, preuve visuelle, banc sans écran, réglages d'essai en INI par lancement,
émulateur, mutations, `NOMS OK`, contrôle des titres du corpus, commit) et la règle de propriété intellectuelle.
L'exécutant de chaque tâche lit ces sections du plan de 11a avant la sienne.

**La release 1.0.0 n'a jamais lieu sans le go écrit de Jérôme.** Ce plan s'arrête à « release prête » (L11-T29) et à la
porte de fin (L11-T30). Aucune tâche ne crée de tag, ne pousse de tag, n'écrit dans une release GitHub ni ne crée le
fichier go (`<git-common-dir>/chantiers/go-action-finale`).

## Amendement 1 (relecture tour 1)

La table complète des constats est dans l'amendement 1 du plan de 11a ; elle fait autorité. Constats appliqués ici,
corrigés en place : **B1** (`tst_app_versioninfo`, L11-T28), **B2** (point 7 de L11-T29 supprimé, interdit du
`workflow_dispatch` réel), **M14** (`commands.lockedCount`, L11-T26), **M15** (`appUpdates.start()` et
`startCalledOnceByTheShell`, L11-T22), **M16** (`bench/scenarios/controle/pseudo-langue.json`, `ACTCRAFT_UPDATES_URL`
avant `tous`, L11-T19, L11-T20, L11-T30), **M8** (réglages d'essai des tests de 11b : règle commune de 11a),
**M10** (comptes de talons, L11-T30), et les mineurs :
- m7 : `tst_ui_accessibility` pose `ACTCRAFT_UPDATES_URL` dans `initTestCase` (L11-T24) ;
- m12 : `#include <actcraft/ui/preferences.h>` dans `updatechecker.h` (L11-T21) ;
- m13 : fixture `same.json` retirée (L11-T21) ;
- m14 : `ENVIRONMENT` qui garde `_bench_env_offscreen` (L11-T22) ;
- m15 : aucun nom de l'original en clair dans le test des pages d'aide ; la liste est lue hors dépôt (L11-T17) ;
- m16 : `app/CMakeLists.txt:26-35`, `ACTCRAFT_SOURCE_DIR`, `versionCodeFollowsTheVersion` (L11-T28) ;
- m17 : chemin du rapport des épinglages du lot 7 résolu par L11-T0 (L11-T27) ;
- m18 : L11-T25 s'arrête et rend la main si H6-6b n'est pas tranchée.

## Amendement 2 (relecture tour 2)

La table des constats N1 à N4 et n1 à n6 est dans l'amendement 2 du plan de 11a ; elle fait autorité. Pour 11b :
- **N1** : la règle des réglages d'essai de 11a (assertion `QSettings probe` à la fin d'`initTestCase`, mutation qui
  retire `setDefaultFormat`) s'applique à `tst_ui_updatechecker`, `tst_ui_shortcuts`, `tst_ui_accessibility`,
  `tst_ui_lockall`, `tst_ui_profilesall` ; le contrôle `reg query` n'existe plus.
- **N3** : formes d'enregistrement de R1 (11a) : `tst_ui_updatechecker`, `tst_ui_shortcuts`, `tst_ui_accessibility`,
  `tst_ui_lockall` et `tst_ui_profilesall` en **F1** (`Main.qml`, forme de `tests/ui.cmake:33-36`) ;
  `tst_ui_helppages` et `tst_ui_pseudolocale` en F2 ; `tst_app_notices` et `tst_app_versioninfo` en F6.
- **N4** : le contrôle des API privées (lignes ajoutées depuis la base de la tâche) vaut pour L11-T19 (joueur).

## Table des tâches (11b)

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L11-T17 | Pages d'aide I : une page par outil des lots 1 à 5 (liste H1), écrites par nous | L11-T16 | `ui/help/<id>.html` pour chaque identifiant de la liste H1 (écrite dans cette ligne par l'orchestrateur après L11-T0), `ui/CMakeLists.txt`, `tests/tst_ui_helppages.cpp` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_helppages` : 6 fonctions `PASS` (dont `pagesOfLots1To5Exist`, `textsAreOurs`) ; `NOMS OK` ; contrôle des titres du corpus vide | S29 | #tache-l11-t17 |
| L11-T18 | Pages d'aide II : outils des lots 6 à 11 (liste H2), pages de procédure, index complet ; scénario `aide-complete` | L11-T17 | `ui/help/<id>.html` pour chaque identifiant de la liste H2, `ui/help/{index,raccourcis,deux-utilisateurs,modes-toucher,reprise,mises-a-jour,profils,mot-de-passe-oublie}.html`, `ui/CMakeLists.txt`, `tests/tst_ui_helppages.cpp`, `bench/scenarios/aide-complete.json` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_helppages` 9 fonctions `PASS` (dont `everyToolHasAPage`) ; `tst_captures_aide_complete` Passed ; `LU helpViewer.visible=true OK`, `CAPTURES OK` | S29 | #tache-l11-t18 |
| L11-T19 | Pseudo-langue de contrôle : génération au build des tests, chargement par le joueur, scénario `pseudo-langue` (liste des chaînes non marquées) | L11-T16 | `scripts/pseudo-langue.py`, `tests/ui.cmake`, `tests/bench.cmake`, `ui/include/actcraft/ui/language.h`, `ui/src/language.cpp`, `bench/captures/main.cpp`, `scripts/captures.ps1`, `tests/tst_ui_pseudolocale.cpp`, `bench/scenarios/controle/pseudo-langue.json` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_pseudolocale` 5 fonctions `PASS` ; scénario joué : `TEXTES OK <n>` ou liste `TEXTE ECART` écrite au rapport | S29 | #tache-l11-t19 |
| L11-T20 | Chaînes non traduisibles corrigées (liste de L11-T19), `actcraft_fr.ts` mis à jour, test de captures activé | L11-T19 | fichiers QML et C++ listés par le rapport de L11-T19 (`TEXTE ECART`, écrits dans cette ligne par l'orchestrateur), `ui/i18n/actcraft_fr.ts`, `tests/bench.cmake` | partagé | implementeur-tdd | `TESTS OK` ; `tst_captures_pseudo_langue` Passed ; `TEXTES OK <n>` à chaque étape `visibleTexts` ; `tst_ui_language` toujours `Passed` | S29 | #tache-l11-t20 |
| L11-T21 | Recherche de mises à jour I (F-760, F-930) : `UpdateChecker`, comparaison SemVer, aucune requête au démarrage par défaut, mandataire du lot 6 | L11-T16 | `ui/include/actcraft/ui/updatechecker.h`, `ui/src/updatechecker.cpp`, `tests/tst_ui_updatechecker.cpp`, `tests/data/updates/{newer,draft,prerelease}.json` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_updatechecker` 7 fonctions `PASS` (dont `noRequestAtStartByDefault`, `semverComparison`) ; mutation rouge puis remise | S29 | #tache-l11-t21 |
| L11-T22 | Recherche de mises à jour II : boîte, menu Aide, réglage « au démarrage » ; scénario `mises-a-jour` contre une réponse simulée | L11-T21 | `ui/qml/UpdateDialog.qml`, `ui/qml/SettingsPage.qml`, `bench/scenarios/mises-a-jour.json`, `tests/bench.cmake`, `tests/tst_ui_updatechecker.cpp` | isolé | implementeur-tdd | `TESTS OK` ; `tst_captures_mises_a_jour` Passed ; `tst_ui_updatechecker` : `startCalledOnceByTheShell` `PASS` (8 fonctions) ; `LU appUpdates.requestCount=0 OK` avant la commande, `LU appUpdates.latestVersion=9.9.9 OK`, `CAPTURES OK` | S29 | #tache-l11-t22 |
| L11-T23 | Raccourcis clavier restants (F-950 à F-953) : table complète, aucun doublon, aucun raccourci pendant la saisie | L11-T18, L11-T20 | `actions/data/commands.json`, `tests/tst_ui_shortcuts.cpp` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_shortcuts` 4 fonctions `PASS` ; `tst_actions_registry` (`noShortcutTwice`) toujours `Passed` | S2.1 | #tache-l11-t23 |
| L11-T24 | Accessibilité : contrastes WCAG AA et focus visible dans toutes les boîtes et en mise en page étroite, noms accessibles des boutons | L11-T20 | `tests/tst_ui_accessibility.cpp`, `tests/support/wcag.h`, fichiers QML corrigés listés par le rapport de la tâche (écrits dans cette ligne par l'orchestrateur à la fin de l'étape 2) | partagé | implementeur-tdd | `TESTS OK` ; `tst_ui_accessibility` 4 fonctions `PASS` ; `tst_ui_shell` (`renderedColorsMeetWcagAa`, `keyboardFocusIsVisible`) toujours `Passed` | S2.2 | #tache-l11-t24 |
| L11-T25 | Licences tierces : `THIRD_PARTY_NOTICES.md` complet (Qt et ses modules, Qt PDF et PDFium, Qt Multimedia et FFmpeg, Hunspell selon H6-6b, dictionnaire, miniz, runtime MSVC), textes des licences, contrôle des bibliothèques non Qt | L11-T16, **décision de Jérôme H6-6b** (sinon arrêt) | `THIRD_PARTY_NOTICES.md`, `LICENSES/*.txt` (textes SPDX cités), `scripts/verifier-licences.ps1`, `packaging/qt-autorises.txt`, `packaging/autres-autorises.txt`, `tests/tst_app_notices.cpp` | isolé | concepteur-critique | `TESTS OK` ; `tst_app_notices` 4 fonctions `PASS` ; `LICENCES OK` sur le dossier déployé et sur l'APK | S1.1 | #tache-l11-t25 |
| L11-T26 | S19 rejoué sur toutes les commandes des lots 1 à 11 : attributs déclarés partout, verrou exact, scénario `classe-protegee-complete` | L11-T9, L11-T23 | `actions/data/commands.json`, `tests/tst_ui_lockall.cpp`, `bench/scenarios/classe-protegee-complete.json` | isolé | concepteur-critique | `TESTS OK` ; `tst_ui_lockall` 5 fonctions `PASS` ; `tst_captures_classe_protegee_complete` Passed ; `LU commands.lockedCount=<K> OK` (K exact, calculé des données) ; mot de passe absent de l'INI | S19 | #tache-l11-t26 |
| L11-T27 | S32 : les cinq profils de bout en bout (épinglages des lots 7 et 8, navigateurs et réglages, filtre d'Android) ; scénario `profils-complets` | L11-T9, L11-T20 | `actions/data/profiles/{en-classe,preparation,lecture-ecriture,calcul-geometrie,son-image}.json`, `actions/include/actcraft/actions/profile.h`, `actions/src/profile.cpp`, `tests/tst_ui_profilesall.cpp`, `bench/scenarios/profils-complets.json` | isolé | implementeur-tdd | `TESTS OK` ; `tst_ui_profilesall` 5 fonctions `PASS` ; `tst_captures_profils_complets` Passed ; pour chaque profil `LU appProfile.currentId=<id> OK` ; `tst_ui_profiles` (`namesAreOurs` en `PASS`) toujours `Passed` | S32 | #tache-l11-t27 |
| L11-T28 | Version 1.0.0 (`versionCode` 10000), section 1.0.0 de `CHANGELOG.md`, guide de démarrage dans `README.md`, notes de version | L11-T17 à L11-T27 | `CMakeLists.txt`, `CHANGELOG.md`, `README.md`, `tests/app.cmake`, `tests/tst_app_versioninfo.cpp` | isolé | implementeur-tdd | `TESTS OK` ; `tst_app_versioninfo` 4 fonctions `PASS`, `tst_app_version` (`--version`) toujours `Passed` ; `Get-ActCraftVersionCode` = `10000` ; `NOTES OK` ; `NOMS OK` ; contrôle des titres du corpus vide | S14 | #tache-l11-t28 |
| L11-T29 | Paquets 1.0.0 et S14 : installateur et APK locaux, essais, flux de release en essai à blanc sur la CI, mode réel sans tag refusé — release prête | L11-T28 | `scripts/paquet.ps1`, `docs/verifications/2026-10-22-lot11-release-prete.md` | partagé | implementeur-tdd | `PAQUET OK : …ActCraft-1.0.0-setup.exe`, `INSTALLATEUR OK`, `APK OK`, `LICENCES OK` ×2, `ANDROID OK`, `ANDROID FICHIERS OK` ; run d'essai : `APK NON PUBLIABLE`, `NOTES OK`, `SHA256SUMS.txt`, `SECRETS ABSENTS` et `CONTROLE OK` du job `controle-mode-reel`, aucune release ; branche d'essai supprimée ; aucun `gh workflow run release.yml -f mode=reel` | S14 | #tache-l11-t29 |
| L11-T30 | Clôture du lot et porte de fin : suites, captures et banc à l'écran (jeton `ecran`), S13 rejoué, talons nuls, graphe, mesures et procédures pour Jérôme, go demandé | L11-T29 | `tests/tst_ui_shell.cpp`, `docs/mesures/2026-10-22-lot11-fin.md`, `docs/verifications/2026-10-22-lot11-porte.md`, `graphify-out/` | partagé | implementeur-tdd | debug et release `TESTS OK`, N = N11 + 32 ; `git -C <wt> grep -l "Talon L11" -- . ':!docs/' ':!graphify-out/' ':!tests/tst_ui_shell.cpp'` vide ; `captures.ps1 -Scenario tous -Rendu d3d11` `CAPTURES OK` ; banc D3D11 plein écran × 3 : verdict et 0 figement ; rapport de porte commité | S14, S13, S2, S3 | #tache-l11-t30 |

## Vagues de 11b (largeur 3 au plus ; aucun fichier partagé dans une vague)

V8 : L11-T17 ‖ L11-T19 ‖ L11-T21 · V9 : L11-T18 ‖ L11-T22 ‖ L11-T25 · V10 : L11-T20 (seule) · V11 : L11-T24 (seule) ·
V12 : L11-T23 ‖ L11-T27 · V13 : L11-T26 · V14 : L11-T28 · V15 : L11-T29 · V16 : L11-T30.

Vérification des recoupements :
- V8 : L11-T17 (`ui/help/` H1, `ui/CMakeLists.txt`, `tst_ui_helppages.cpp`) / L11-T19 (`pseudo-langue.py`,
  `tests/ui.cmake`, `tests/bench.cmake`, `language.*`, `bench/captures/main.cpp`, `captures.ps1`, un test, un scénario)
  / L11-T21 (`updatechecker.*`, un test, `tests/data/updates/`) — disjoints.
- V9 : L11-T18 (`ui/help/` H2 et procédures, `ui/CMakeLists.txt`, `tst_ui_helppages.cpp`, un scénario) / L11-T22
  (`UpdateDialog.qml`, `SettingsPage.qml`, un scénario, `tests/bench.cmake`, `tst_ui_updatechecker.cpp` repris de L11-T21) / L11-T25 (`THIRD_PARTY_NOTICES.md`,
  `LICENSES/`, `verifier-licences.ps1`, `packaging/`, un test) — disjoints. `tests/bench.cmake` : L11-T19 (V8) puis
  L11-T22 (V9), tenu par la vague.
- V10 et V11 : tâches à fichiers larges (QML), seules dans leur vague, worktree `partagé`.
- V12 : L11-T23 (`commands.json`, un test) / L11-T27 (profils, `profile.*`, un test, un scénario) — disjoints.
- V13 : L11-T26 seule (`commands.json` après L11-T23).
- Fichiers repris d'une vague à l'autre, tenus par « dépend de » : `ui/CMakeLists.txt` (L11-T17 → L11-T18) ;
  `tst_ui_helppages.cpp` (L11-T17 → L11-T18) ; `tests/bench.cmake` (L11-T19 → L11-T22 → L11-T20) ;
  `commands.json` (L11-T9 → L11-T23 → L11-T26).

**Écran.** Seule L11-T30 ouvre des fenêtres visibles (captures et banc en D3D11, plein écran sur l'écran principal),
sous le jeton `ecran`, annoncés à Jérôme d'une ligne avant. L11-T29 ouvre la fenêtre de l'essai de l'installateur
(5 s) : l'annoncer d'une ligne.

## Décisions appliquées (suite de D11-1 à D11-12 ; L11-T0 peut les amender)

- **D11-13 — Mises à jour.** Source : `https://api.github.com/repos/jejeasterix/actcraft/releases/latest` ; brouillons
  et préversions ignorés ; Windows seul (`help.check-updates` déclaré `"platforms": ["windows"]`) ; recherche au
  démarrage désactivée par défaut (N1-A12), réglage du poste `updates/check-at-start` ; mandataire du lot 6 appliqué
  (A16) ; aucune télémétrie, aucun identifiant envoyé ; une version plus récente ouvre la page de la release dans le
  navigateur du système, rien n'est téléchargé. Point d'entrée remplaçable pour les essais par la variable
  `ACTCRAFT_UPDATES_URL`, acceptée seulement en `file:` ou en `http://127.0.0.1:<port>` / `http://localhost:<port>`.
- **D11-14 — Pseudo-langue.** Code `xx` (usage privé), chaque traduction = `⟦` + source + `⟧`, marqueurs `%1`, `%n` et
  balises gardés ; générée au build des tests seulement (`build/<preset>/i18n/actcraft_xx.qm`), jamais dans les
  ressources ni dans les paquets ; chargée par l'option `--traduction <fichier .qm>` du joueur de captures seulement.
- **D11-15 — Pas de paquet macOS** (cadrage § 6, amendement 1 § 1) ; macOS construit et testé par la CI.
- **D11-16 — Version.** 1.0.0 ; `versionCode` = 1 × 10000 + 0 × 100 + 0 = **10000**, plus grand que celui de 0.10.0
  (1000) : la mise à jour de l'APK par-dessus la version du lot 9 passe (amendement 1, § 6).

## Listes de 11b

- **H1 et H2 — pages d'aide.** L11-T0 écrit dans son rapport la liste des identifiants de commandes `tool.*` (et des
  commandes d'outils de présentation et de maths) **disponibles** à la tête du chantier, partagée en H1 (outils des
  lots 1 à 5) et H2 (lots 6 à 11) ; l'orchestrateur la recopie dans les lignes de L11-T17 et L11-T18 avant leur
  lancement. Un nom de fichier = l'identifiant de la commande (`ui/help/tool.pen.html`), comme le lit
  `HelpViewer::openFor` (A8).
- **Chaînes à corriger (L11-T20)** : sortie `TEXTE ECART` de L11-T19, regroupée par fichier dans son rapport.
- **QML à corriger (L11-T24)** : sortie de l'étape 2 de L11-T24 (fonctions en `FAIL!`), regroupée par fichier.

---

<a id="tache-l11-t17"></a>
### Tâche L11-T17 — Pages d'aide I (outils des lots 1 à 5)

**Objectif.** F-760 (aide), N2-A4 : une page HTML par identifiant de la liste H1, écrite par nous en français simple :
à quoi sert l'outil, comment le choisir (palette, menu, raccourci), comment s'en servir au stylet, à la souris et au
doigt, ses réglages, un lien vers l'index. Pages servies par `qrc:/actcraft/help/` (A8), ajoutées à la ressource de
`ui/CMakeLists.txt` à côté de `ui/help/index.html` (L6-T35).

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R tst_ui_helppages` ; « fonctions d'un test ».

**Interdits.** Aucun texte, aucune image, aucune capture de l'aide d'ActivInspire ni de sa documentation
(`Documentation Activinspire/` sert à comprendre, jamais à copier) ; aucune image dans les pages ; aucun `<script>` ;
aucune ressource distante ; aucun nom de l'original (termes de N1-12) ; aucun titre de vidéo.

**Terminé quand** les 6 fonctions passent, `NOMS OK`, contrôle des titres du corpus vide.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tests/tst_ui_helppages.cpp` (remplace le talon) :
  ```cpp
  #include <actcraft/ui/helpviewer.h>

  #include <QFile>
  #include <QRegularExpression>
  #include <QtTest>

  namespace {
  // Liste H1, recopiée de L11-T0 par l'orchestrateur ; l'exemple ci-dessous est à remplacer par la liste réelle.
  const QStringList kLots1To5 = {QStringLiteral("tool.pen"), QStringLiteral("tool.highlighter"),
                                 QStringLiteral("tool.eraser"), QStringLiteral("tool.select")};
  QString page(const QString& id)
  {
      QFile f(QStringLiteral(":/actcraft/help/%1.html").arg(id));
      return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
  }
  } // namespace

  class TstUiHelpPages : public QObject
  {
      Q_OBJECT
  private slots:
      void pagesOfLots1To5Exist()
      {
          for (const QString& id : kLots1To5)
              QVERIFY2(!page(id).isEmpty(), qPrintable(id));
      }
      void everyPageHasATitleAndABackLink()
      {
          for (const QString& id : kLots1To5) {
              const QString html = page(id);
              QVERIFY2(QRegularExpression(QStringLiteral("<title>[^<]{3,}</title>")).match(html).hasMatch(), qPrintable(id));
              QVERIFY2(html.contains(QStringLiteral("href=\"index.html\"")), qPrintable(id));
          }
      }
      void internalLinksResolve()
      {
          const QRegularExpression link(QStringLiteral("href=\"([^\"#:]+\\.html)"));
          for (const QString& id : kLots1To5) {
              auto it = link.globalMatch(page(id));
              while (it.hasNext()) {
                  const QString target = it.next().captured(1);
                  QVERIFY2(QFile::exists(QStringLiteral(":/actcraft/help/") + target), qPrintable(id + " → " + target));
              }
          }
      }
      void noScriptImageOrRemoteResource()
      {
          for (const QString& id : kLots1To5) {
              const QString html = page(id);
              QVERIFY2(!html.contains(QStringLiteral("<script"), Qt::CaseInsensitive), qPrintable(id));
              QVERIFY2(!html.contains(QStringLiteral("<img"), Qt::CaseInsensitive), qPrintable(id));
              QVERIFY2(!html.contains(QStringLiteral("src=\"http"), Qt::CaseInsensitive), qPrintable(id));
          }
      }
      void textsAreOurs()
      {
          QStringList forbidden = {QStringLiteral("ActivInspire"), QStringLiteral("Promethean")};
          const QString names = qEnvironmentVariable("ACTCRAFT_REFERENCE_NAMES");
          if (!names.isEmpty()) {
              QFile f(names);
              QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
              for (const QByteArray& line : f.readAll().split('\n'))
                  if (!line.trimmed().isEmpty())
                      forbidden << QString::fromUtf8(line.trimmed());
          }
          for (const QString& id : kLots1To5)
              for (const QString& word : forbidden)
                  QVERIFY2(!page(id).contains(word, Qt::CaseInsensitive), qPrintable(id + " : " + word));
      }
      void f1OpensTheToolPage()
      {
          actcraft::ui::HelpViewer viewer;
          viewer.openFor(QStringLiteral("tool.pen"));
          QVERIFY(viewer.source().toString().endsWith(QStringLiteral("tool.pen.html")));
      }
  };

  QTEST_MAIN(TstUiHelpPages)
  #include "tst_ui_helppages.moc"
  ```
  La preuve exige `textsAreOurs` en `PASS` **avec** `ACTCRAFT_REFERENCE_NAMES` posée par l'orchestrateur vers le
  fichier hors dépôt `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot11-noms.txt` (écrit par L11-T0 : les noms
  de `lot6-noms-profils.txt` et ceux des outils et des produits de l'original, un par ligne). Aucun de ces noms n'est
  écrit dans le dépôt : le test n'en garde en clair que les deux noms de la marque.
- [ ] **Étape 2 : rouge vu.**
- [ ] **Étape 3 : pages.** Une page par identifiant H1, gabarit commun à nous :
  ```html
  <!doctype html>
  <html lang="fr">
  <head><meta charset="utf-8"><title>Stylo</title></head>
  <body>
  <h1>Stylo</h1>
  <p>Le stylo écrit à main levée sur la page, dans la couleur et l'épaisseur choisies.</p>
  <h2>Le choisir</h2>
  <p>Bouton du stylo dans la palette, ou menu Outils ▸ Stylo.</p>
  <h2>S'en servir</h2>
  <p>Posez la pointe du stylet, ou le doigt quand le mode du toucher le permet, et tracez. Relevez pour finir le trait.</p>
  <h2>Réglages</h2>
  <p>Couleur : une pastille de la palette. Épaisseur : les quatre tailles, ou la réglette.</p>
  <p><a href="index.html">Retour au sommaire</a></p>
  </body>
  </html>
  ```
  (exemple pour `tool.pen` ; chaque page décrit le comportement réel d'ActCraft, relu dans son code et ses tests).
  Ressource ajoutée dans `ui/CMakeLists.txt`.
- [ ] **Étape 4 : vert** (avec `ACTCRAFT_REFERENCE_NAMES`), preuve de tâche, `NOMS OK`, contrôle des titres du corpus
  vide ; commit « Lot 11 : pages d'aide des outils des lots 1 à 5 (L11-T17) ».

<a id="tache-l11-t18"></a>
### Tâche L11-T18 — Pages d'aide II, procédures et index ; scénario `aide-complete`

**Objectif.** Pages des identifiants H2 (même gabarit), pages de procédure (`raccourcis.html` : table des raccourcis
lue dans `commands.json` au moment de l'écriture ; `deux-utilisateurs.html` ; `modes-toucher.html` ; `reprise.html` ;
`mises-a-jour.html` ; `profils.html` ; `mot-de-passe-oublie.html` repris de l'index de L6-T35 en page à part) ; index
qui lie chaque page. Trois fonctions ajoutées : `everyToolHasAPage` (chaque commande `tool.*` disponible sur la
plateforme du test, lue dans le registre, a sa page), `procedurePagesExist`, `indexLinksEveryPage` (chaque fichier de
`:/actcraft/help/` sauf l'index est lié depuis l'index). Les listes `kLots1To5` et H2 sont réunies en `kAllTools`.

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_helppages|tst_captures_aide_complete|tst_ui_help'`.

**Preuve visuelle.** `captures.ps1 -Scenario aide-complete -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "helpViewer.visible=true"` :
`LU … OK`, `CAPTURES OK` ; décrire `aide-stylo.png` et `aide-deux-utilisateurs.png`.

**Interdits.** Ceux de L11-T17 ; aucun chargement réseau dans le lecteur (L6-T35, `noNetworkUrl`).

**Terminé quand** les 9 fonctions et le scénario passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** (trois fonctions ajoutées ; `kAllTools`).
- [ ] **Étape 2 : pages et index**, ressource.
- [ ] **Étape 3 : scénario** `aide-complete.json` : `waitReady` ; `tool.pen` ; `{"key": "F1"}` ;
  `waitFor helpViewer.visible = true` ; `expect helpViewer.currentPage = tool.pen` (propriété ajoutée si absente : nom de la
  page courante, sans extension) ; capture `aide-stylo` ; Échap ; `users.dual` ; `{"key": "F1"}` ;
  `expect helpViewer.currentPage = deux-utilisateurs` (F1 en mode à deux utilisateurs ouvre sa page) ; capture
  `aide-deux-utilisateurs` ; Échap ; `users.dual`. Si F1 en mode à deux utilisateurs ne doit pas ouvrir cette page
  (L11-T0), l'étape passe par l'index (`helpViewer.follow`, commande `help.page deux-utilisateurs` si elle existe).
- [ ] **Étape 4 : vert**, preuve visuelle, preuve de tâche, `NOMS OK`, contrôle des titres du corpus vide ; commit
  « Lot 11 : pages d'aide complètes et procédures (L11-T18) ».

<a id="tache-l11-t19"></a>
### Tâche L11-T19 — Pseudo-langue de contrôle ; scénario `pseudo-langue`

**Objectif.** D11-14, S29 (langue). `scripts/pseudo-langue.py <entrée.ts> <sortie.ts>` (Python 3, lancé par
`python -I`) : copie le `.ts` produit par `lupdate` (`ui/i18n/actcraft_fr.ts`, L6-T36) en remplissant chaque
`<translation>` par `⟦<source>⟧` (formes plurielles comprises, `%1` et `%n` gardés, attribut `type="unfinished"`
retiré), écrit en UTF-8 sans BOM, fins de ligne LF (`write_bytes`). `tests/ui.cmake` : commande de build qui produit
`build/<preset>/i18n/actcraft_xx.qm` (`find_package(Python3 COMPONENTS Interpreter)`, puis `Qt6::lrelease`), dépendance
de `tst_ui_pseudolocale` et de `tst_captures_pseudo_langue`. `Language::installFile(QCoreApplication&, QString qmPath)`
(nouvelle fonction) ; le joueur de captures accepte `--traduction <qm>` et l'installe **avant** `loadMainWindow` ;
`scripts/captures.ps1` gagne `-Traduction <fichier>` qu'il passe au joueur. Étapes `visibleTexts` (L11-T2) sur la
fenêtre principale, le menu Fichier ouvert, et trois boîtes (À propos, taille de page, réglages).

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R tst_ui_pseudolocale` ; puis le scénario par `captures.ps1` (le test
`tst_captures_pseudo_langue` reste désactivé jusqu'à L11-T20).

**Preuve visuelle.** `captures.ps1 -Scenario controle/pseudo-langue -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Traduction build/debug/i18n/actcraft_xx.qm -Lire "appPreferences.language=fr"` :
lignes `TEXTES OK <n>` ou `TEXTES EN ECART <k>` et leurs `TEXTE ECART` ; la liste entière est recopiée au rapport,
regroupée par fichier source (recherche du texte dans `ui/`, `features/`, `app/` par `git grep -n -F`).

**Interdits.** La pseudo-langue n'entre dans aucune ressource ni aucun paquet ; aucune traduction automatique ;
aucune correction de chaîne ici (c'est L11-T20).

**Terminé quand** les 5 fonctions passent et la liste des écarts est au rapport (vide ou non).

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_pseudolocale.cpp` : `everySourceMarked` (le `.ts` généré : chaque
  `<translation>` = `⟦` + source + `⟧`) ; `placeholdersKept` (`%1`, `%n` présents à l'identique) ; `qmLoads`
  (`QTranslator::load` vrai ; `QCoreApplication::translate("Main", "<une source relevée dans actcraft_fr.ts>")` rend la
  forme marquée) ; `installFileInstallsTheTranslator` ; `notShippedInResources` (`QFile::exists(":/i18n/actcraft_xx.qm")`
  faux, et aucune entrée `xx` dans `Language::available()`).
- [ ] **Étape 2 : script, CMake, `language.*`, joueur, `captures.ps1`.**
- [ ] **Étape 3 : scénario** `bench/scenarios/controle/pseudo-langue.json` (sous-dossier que `-Scenario tous` ne prend
  pas ; déclaration ctest de L11-T1 à ce chemin) : `waitReady` ; `{"visibleTexts": "pseudo", "ignore": ["pageIndicator", "zoomLabel"]}` ;
  capture `principale` ; menu Fichier ouvert (`pointer` sur son titre, coordonnées de fenêtre relevées sur la capture
  `principale`) ; `visibleTexts` ; capture `menu-fichier` ; Échap ; `help.about` ; `visibleTexts` ; capture `a-propos` ;
  Échap ; boîte de taille de page (commande relevée dans `commands.json`) ; `visibleTexts` ; Échap ; boîte des réglages ;
  `visibleTexts` ; capture `reglages`.
- [ ] **Étape 4 : vert** (unitaire), scénario joué, rapport ; preuve de tâche ; commit « Lot 11 : pseudo-langue de
  contrôle des textes (L11-T19) ».

<a id="tache-l11-t20"></a>
### Tâche L11-T20 — Chaînes non traduisibles corrigées

**Objectif.** Chaque `TEXTE ECART` de L11-T19 : la chaîne passe par `qsTr()` (QML) ou `tr()` / `QCoreApplication::translate`
(C++) ; une chaîne qui vient des données (`commands.json`, profils) passe par `QT_TRANSLATE_NOOP` à la lecture ou par un
contexte de traduction des données (forme retenue écrite au rapport) ; `ui/i18n/actcraft_fr.ts` régénéré par la cible
`update_translations` (L6-T36) ; `DISABLED` retiré de `tst_captures_pseudo_langue` dans `tests/bench.cmake` (avec
l'argument `--traduction` vers le `.qm` de build et le scénario `bench/scenarios/controle/pseudo-langue.json`).

**Fichiers.** Ceux de la ligne de la table (liste exacte écrite par l'orchestrateur depuis le rapport de L11-T19).

**Test et commande bornée.** Commande ciblée `-R 'tst_captures_pseudo_langue|tst_ui_language|tst_ui_shell'`.

**Preuve visuelle.** Commande de L11-T19 : `TEXTES OK <n>` à chaque étape `visibleTexts`, `CAPTURES OK` ; décrire
`principale.png` (tous les libellés entre `⟦ ⟧`).

**Interdits.** Aucun texte changé (seule l'enveloppe de traduction change) ; aucun contenu de document marqué comme
écart corrigé par une exclusion (`ignore`) sans le dire au rapport.

**Terminé quand** le test de captures passe et la suite reste verte.

**Étapes.**
- [ ] **Étape 1 :** reproduire la liste (rouge : `TEXTES EN ECART`).
- [ ] **Étape 2 :** corriger fichier par fichier ; régénérer `actcraft_fr.ts` ; rejouer le scénario.
- [ ] **Étape 3 : vert**, preuve de tâche ; `NOMS OK` ; commit « Lot 11 : toutes les chaînes visibles traduisibles
  (L11-T20) ».

<a id="tache-l11-t21"></a>
### Tâche L11-T21 — Recherche de mises à jour I

**Objectif.** D11-13, N1-A12.

**Contrat.**
```cpp
// ui/include/actcraft/ui/updatechecker.h
#pragma once

#include <actcraft/ui/preferences.h>

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

class UpdateChecker : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::Preferences* preferences READ preferences WRITE setPreferences NOTIFY preferencesChanged)
    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY stateChanged)
    Q_PROPERTY(QUrl releasePage READ releasePage NOTIFY stateChanged)
    Q_PROPERTY(bool checkAtStart READ checkAtStart WRITE setCheckAtStart NOTIFY checkAtStartChanged)
    Q_PROPERTY(int requestCount READ requestCount NOTIFY stateChanged)

public:
    enum class State { Idle, Checking, UpToDate, Available, Failed };
    Q_ENUM(State)

    explicit UpdateChecker(QObject* parent = nullptr);
    // Versions « x.y.z », préfixe « v » toléré ; faux si l'une ne se lit pas.
    static bool isNewer(QStringView candidate, QStringView current);
    // Point d'entrée : ACTCRAFT_UPDATES_URL si file: ou http://127.0.0.1 / localhost, sinon l'API des releases du dépôt.
    static QUrl endpoint();
    void setNetworkAccessManager(QNetworkAccessManager* manager); // tests ; sinon un gestionnaire interne
    Q_INVOKABLE void check();
    Q_INVOKABLE void start(); // appelé une fois au démarrage : check() seulement si checkAtStart

    Preferences* preferences() const;
    void setPreferences(Preferences* preferences);
    State state() const;
    QString latestVersion() const;
    QUrl releasePage() const;
    bool checkAtStart() const; // réglage updates/check-at-start, faux par défaut
    void setCheckAtStart(bool on);
    int requestCount() const;

signals:
    void preferencesChanged();
    void stateChanged();
    void checkAtStartChanged();
};

} // namespace actcraft::ui
```
Réponse lue : `tag_name`, `html_url`, `draft`, `prerelease` ; un brouillon ou une préversion vaut `UpToDate` ; erreur
réseau ou JSON illisible → `Failed`. Le mandataire de `NetworkSettings` (A16) est posé sur le gestionnaire interne.

**Fichiers.** Ceux de la ligne de la table ; `tests/data/updates/newer.json` :
```json
{"tag_name": "v9.9.9", "html_url": "https://github.com/jejeasterix/actcraft/releases/tag/v9.9.9", "draft": false, "prerelease": false}
```
`draft.json` (`v9.9.9`, `"draft": true`), `prerelease.json` (`v9.9.9`, `"prerelease": true`).

**Test et commande bornée.** Commande ciblée `-R tst_ui_updatechecker` ; « fonctions d'un test ».

**Interdits.** Aucune requête réseau réelle dans les tests (réponses en `file:`) ; aucune requête au démarrage quand le
réglage est faux ; aucun téléchargement ; aucun envoi d'identifiant.

**Terminé quand** les 7 fonctions passent et la mutation est vue rouge.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_updatechecker.cpp` : `semverComparison` (`isNewer("1.0.1","1.0.0")`,
  `isNewer("1.10.0","1.9.0")`, `isNewer("0.10.0","0.9.0")`, `!isNewer("1.0.0","1.0.0")`, `!isNewer("zzz","1.0.0")`) ;
  `noRequestAtStartByDefault` (gestionnaire espion qui compte `createRequest` ; `start()` → 0) ;
  `requestAtStartWhenEnabled` ; `availableFromReply` (`ACTCRAFT_UPDATES_URL` = `file:` de `newer.json` →
  `Available`, `latestVersion` `9.9.9`, `releasePage` lue) ; `draftAndPrereleaseIgnored` ; `failureReported`
  (fichier absent → `Failed`) ; `remoteOverrideRefused` (`ACTCRAFT_UPDATES_URL=https://exemple.invalid/x` → point
  d'entrée par défaut).
- [ ] **Étape 2 : implémentation.**
- [ ] **Étape 3 : mutation** : faire appeler `check()` sans condition dans `start()` → `noRequestAtStartByDefault`
  rouge ; `git -C <wt> checkout -- ui/src/updatechecker.cpp` ; vert.
- [ ] **Étape 4 : preuve de tâche** ; commit « Lot 11 : recherche de mises à jour (L11-T21) ».

<a id="tache-l11-t22"></a>
### Tâche L11-T22 — Recherche de mises à jour II ; scénario `mises-a-jour`

**Objectif.** `UpdateDialog.qml` : gestionnaire de `help.check-updates` (remplace le talon de L11-T3) : ouvre la boîte,
appelle `appUpdates.check()`, montre l'état (recherche, à jour, version plus récente avec son numéro, échec), bouton
« Voir la version » (`Qt.openUrlExternally(appUpdates.releasePage)`), bouton « Fermer ». `SettingsPage.qml` : case
« Rechercher les mises à jour au démarrage » liée à `appUpdates.checkAtStart` (Windows seulement). `tests/bench.cmake` :
`ENVIRONMENT "${_bench_env_offscreen};ACTCRAFT_UPDATES_URL=file:///${PROJECT_SOURCE_DIR}/tests/data/updates/newer.json"`
sur `tst_captures_mises_a_jour` (la liste `_bench_env_offscreen`, `tests/bench.cmake:8-10`, est gardée : sans elle la
fenêtre s'ouvrirait à l'écran). Démarrage : `UpdateDialog.qml` porte `Component.onCompleted: appUpdates.start()`
(un seul appel, au chargement de la coquille) ; sans lui, la case « au démarrage » ne ferait rien. Fonction ajoutée à
`tests/tst_ui_updatechecker.cpp` : `startCalledOnceByTheShell` (`Main.qml` chargé, réglages d'essai en INI temporaire
avec `updates/check-at-start=true` et `ACTCRAFT_UPDATES_URL` vers `newer.json` → `requestCount == 1` ; même montage
avec `false` → 0).

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R tst_captures_mises_a_jour`.

**Preuve visuelle.** `captures.ps1 -Scenario mises-a-jour -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "appUpdates.checkAtStart=false"`
avec `$env:ACTCRAFT_UPDATES_URL` posé comme dans `tests/bench.cmake` : `LU appUpdates.requestCount=0 OK`,
`LU appUpdates.latestVersion=9.9.9 OK`, `CAPTURES OK` ; décrire `version-plus-recente.png`.

**Interdits.** Aucun clic sur « Voir la version » dans le scénario (il ouvrirait le navigateur) ; aucune boîte au
démarrage.

**Terminé quand** le scénario et `startCalledOnceByTheShell` passent.

**Étapes.**
- [ ] **Étape 1 : scénario** `mises-a-jour.json` : `waitReady` ; `expect appUpdates.requestCount = 0` (S29 : aucune
  requête au démarrage, réglage désactivé) ; `command help.check-updates` ; `waitFor appUpdates.state = Available`
  (forme de l'énumération lue par le joueur : valeur entière ou nom, relevée dans `uiscenario.cpp`) ;
  `expect updateDialog.visible = true` ; `expect appUpdates.latestVersion = 9.9.9` ; capture `version-plus-recente`.
- [ ] **Étape 2 : rouge vu** (talon), **QML**, réglage, `tests/bench.cmake`.
- [ ] **Étape 3 : vert**, preuve visuelle, preuve de tâche ; `NOMS OK` ; commit « Lot 11 : boîte de recherche de mises
  à jour (L11-T22) ».

<a id="tache-l11-t23"></a>
### Tâche L11-T23 — Raccourcis clavier restants (F-950 à F-953)

**Objectif.** Décision 14 du cadrage. Table complète des raccourcis des fonctions incluses (liste établie par L11-T0
depuis F-950 à F-953 de la spécification, en nos termes ; touches génériques du système gardées : Ctrl+N, Ctrl+O,
Ctrl+S, Ctrl+Z, Ctrl+Y, Ctrl+C, Ctrl+X, Ctrl+V, Suppr, F1, F5…) ; chaque raccourci déclaré dans `commands.json` ; aucun
doublon (`noShortcutTwice`, `tst_actions_registry`) ; aucun raccourci d'une seule touche pendant la saisie d'un texte
(Suppr efface une lettre, pas l'objet) ; la page `raccourcis.html` (L11-T18) les liste tous.

**Fichiers.** `actions/data/commands.json`, `tests/tst_ui_shortcuts.cpp`.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_shortcuts|tst_actions_registry'`.

**Interdits.** Aucun raccourci qui reprend une combinaison propre à l'original sans équivalent générique (relevé par
L11-T0) ; aucune touche réservée au système (Alt+F4, Win+…).

**Terminé quand** les 4 fonctions passent.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_shortcuts.cpp` (montage `Main.qml`, réglages d'essai) :
  `everyListedShortcutDeclared` (table du test = liste de L11-T0 : identifiant → touches) ;
  `shortcutTriggersItsCommand` (`QTest::keySequence` sur la fenêtre pour chaque entrée → compteur de déclenchement de la
  commande par `Commands::triggered`) ; `noSingleKeyShortcutWhileTyping` (texte en cours d'édition, Suppr → l'objet
  existe toujours) ; `helpPageListsEveryShortcut` (`:/actcraft/help/raccourcis.html` contient chaque séquence de
  `commands.json`, en texte natif de `QKeySequence::toString(QKeySequence::NativeText)`).
- [ ] **Étape 2 : données** ; si `helpPageListsEveryShortcut` échoue faute d'entrée dans la page, la page est corrigée
  **ici** (`ui/help/raccourcis.html` ajouté à la ligne de la table par l'orchestrateur ; aucune autre tâche de V12 ne
  la touche).
- [ ] **Étape 3 : vert**, preuve de tâche ; commit « Lot 11 : raccourcis clavier complets (L11-T23) ».

<a id="tache-l11-t24"></a>
### Tâche L11-T24 — Accessibilité

**Objectif.** Étendre aux boîtes de tous les lots et à la mise en page étroite ce que `tst_ui_shell` prouve pour la
fenêtre principale (`renderedColorsMeetWcagAa`, `keyboardFocusIsVisible`) : contraste ≥ 4,5:1 (texte normal) et ≥ 3:1
(texte de 18,66 px gras ou 24 px, bordures de contrôles), focus visible au clavier, nom accessible pour chaque bouton
(`Accessible.name`, lu par `QAccessible`).

**Fichiers.** `tests/tst_ui_accessibility.cpp`, `tests/support/wcag.h` (calcul de luminance relative et de contraste,
extrait du test de `tst_ui_shell` sans le modifier : copie de la fonction, `tst_ui_shell` garde la sienne), et les QML
corrigés (liste au rapport de l'étape 2).

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_accessibility|tst_ui_shell'`.

**Interdits.** Aucun test existant affaibli ; aucune couleur hors des jetons de `Theme.qml` ; aucune correction qui
change la disposition Windows en largeur ≥ 960.

**Terminé quand** les 4 fonctions passent et `tst_ui_shell` reste `Passed`.

**Étapes.**
- [ ] **Étape 1 : test** `tst_ui_accessibility.cpp` (dans `initTestCase` : réglages d'essai en INI temporaire, et
  `qputenv("ACTCRAFT_UPDATES_URL", <URL file: de tests/data/updates/newer.json>)` : la boîte des mises à jour ne fait
  aucune requête réseau) : `everyDialogMeetsAa` (pour chaque commande `opensDialog`
  disponible sous Windows (L11-T9) : déclencher, attendre la boîte, rendre la fenêtre par `grabWindow`, pour chaque
  `Text` visible comparer la couleur du texte au pixel de fond relevé à côté de son rectangle) ;
  `focusVisibleInEveryDialog` (Tab parcourt chaque contrôle ; l'élément focalisé montre l'anneau de `Theme`, forme de
  `keyboardFocusIsVisible`) ; `everyButtonHasAnAccessibleName` (`QAccessible::queryAccessibleInterface(item)->text(QAccessible::Name)`
  non vide pour chaque `CommandButton`, `Button`, `ToolButton` visible de la fenêtre, des boîtes et du tiroir) ;
  `narrowLayoutMeetsAa` (800 × 1280, `touch-ui` vrai).
- [ ] **Étape 2 : rouge relevé** : la liste des `FAIL!` regroupée par fichier QML, au rapport ; l'orchestrateur la
  recopie dans la ligne de la table.
- [ ] **Étape 3 : corrections** (jetons de `Theme.qml`, `Accessible.name: qsTr(…)`, anneau de focus) ; vert ; banc sans
  écran ; preuve de tâche ; `NOMS OK` ; commit « Lot 11 : accessibilité des boîtes et de la mise en page étroite
  (L11-T24) ».

<a id="tache-l11-t25"></a>
### Tâche L11-T25 — Licences tierces

**Objectif.** Décision 34 du cadrage, N1-A10, H6-6b. `THIRD_PARTY_NOTICES.md` couvre **tout** ce qui est livré dans
l'installateur et l'APK 1.0.0 :
- Qt 6.12 (LGPLv3, liaison dynamique) : un paragraphe par module de `packaging/qt-autorises.txt` ;
- Qt PDF et PDFium (BSD-3-Clause) avec les composants que Qt déclare pour lui (liste lue dans les fichiers
  `qt_attribution.json` de l'installation `C:\Qt\6.12.0\msvc2022_64`, recopiée en nos mots : nom, version, licence
  SPDX, URL du projet) ;
- Qt Multimedia et FFmpeg (LGPL-2.1-or-later), si les bibliothèques FFmpeg sont déployées (relevé dans le dossier
  déployé) ;
- Hunspell selon la décision de Jérôme H6-6b (MPL-1.1 recommandée au lot 6, ou LGPL en liaison dynamique) et le
  dictionnaire français (licence du dictionnaire retenu en H6-6) ;
- miniz (MIT), runtime MSVC (licence de redistribution de Microsoft) ;
- chaque licence SPDX citée a son texte sous `LICENSES/<SPDX>.txt`.
`scripts/verifier-licences.ps1` contrôle aussi les bibliothèques **non Qt** déployées (`*.dll` hors `Qt6*`, `*.so`
hors `libQt6*` de l'APK) contre `packaging/autres-autorises.txt` (nouveau ; une ligne par bibliothèque : motif de nom,
licence SPDX) : toute bibliothèque absente → `LICENCES EN ECHEC : <noms>`.

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R tst_app_notices` ; puis
`pwsh -NoProfile -File ./scripts/paquet.ps1` (qui appelle `verifier-licences.ps1`) et
`pwsh -NoProfile -File ./scripts/verifier-licences.ps1 -Apk build/paquet/ActCraft-<version>-arm64-v8a.apk`
(`LICENCES OK` les deux fois).

**Préalable (H6-6b, amendement 1, m18).** Si la décision de Jérôme H6-6b (licence de Hunspell) n'est pas écrite dans
le fichier d'état du chantier, la tâche s'arrête **avant toute écriture**, le signale à l'orchestrateur et rend la
main ; aucune licence supposée.

**Interdits.** Aucun texte de licence inventé (textes SPDX officiels) ; aucun module Qt sous GPL seule ; aucune
affirmation juridique (N1-8 : non validé par un juriste, dit dans le fichier).

**Terminé quand** les 4 fonctions passent et `LICENCES OK` deux fois.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_app_notices.cpp` (lit les fichiers du dépôt par `ACTCRAFT_SOURCE_DIR`, définition de
  compilation déjà posée par `tests/app.cmake` ou ajoutée par L11-T1) : `everyAllowedQtModuleHasANotice` ;
  `everyOtherLibraryHasANotice` (chaque ligne de `autres-autorises.txt` et chaque dossier de `third_party/`) ;
  `everyCitedLicenseHasItsText` (identifiants SPDX du fichier → `LICENSES/<id>.txt` existe) ;
  `aboutShowsTheSameNotices` (`AppInfo::notices()` égale le contenu du fichier : la ressource est à jour).
- [ ] **Étape 2 : relevés** (dossier déployé par `paquet.ps1 -SansConstruire` après une construction release, APK) ;
  notices, textes, listes.
- [ ] **Étape 3 : script** `verifier-licences.ps1` ; essai rouge : retirer une ligne de `autres-autorises.txt` →
  `LICENCES EN ECHEC` ; `git -C <wt> checkout -- packaging/autres-autorises.txt` ; `LICENCES OK`.
- [ ] **Étape 4 : vert**, preuve de tâche ; `NOMS OK` ; commit « Lot 11 : licences tierces complètes et contrôlées
  (L11-T25) ».

<a id="tache-l11-t26"></a>
### Tâche L11-T26 — S19 rejoué sur toutes les commandes

**Objet lu.** L'instance `Commands` de `Main.qml` a `objectName: "commands"` (`ui/qml/Main.qml:56-58`) : le
joueur la lit sous ce nom (`commands.lockedCount`), jamais sous son `id` `appCommands`.

**Objectif.** S19 (amendement 1, § 7), rejoué sur les commandes des lots 1 à 11 : chaque entrée de `commands.json`
déclare **explicitement** `lockable` et `restrictable` (aucune valeur par défaut tacite) ; sous verrou, exactement les
commandes `lockable` disponibles sur la plateforme sont grisées ; le compte est exact ; le verrou survit au
relancement ; le mot de passe n'est jamais en clair dans les réglages.

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_lockall|tst_captures_classe_protegee_complete|tst_ui_classroomlock'`.

**Preuve visuelle.** `captures.ps1 -Scenario classe-protegee-complete -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "commands.lockedCount=<K>"` :
`LU commands.lockedCount=<K> OK`, `RELANCE`, `LU appClassroom.locked=true OK`, `CAPTURES OK` ; décrire
`verrouille.png` ; puis, mot de passe absent de l'INI du lancement (sortie vide attendue) :
```powershell
Get-ChildItem -Recurse .tmp/captures/classe-protegee-complete-logiciel/reglages -Filter *.ini | Select-String -SimpleMatch -Pattern 'essai-1234','ZXNzYWktMTIzNA=='
```

**Interdits.** Aucun attribut changé sans raison écrite au rapport (une commande rendue non verrouillable doit l'être
dans l'original ou par une règle du lot 6) ; aucun mot de passe d'essai autre que celui du scénario.

**Terminé quand** les 5 fonctions et le scénario passent, et K est écrit au rapport avec son calcul.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_lockall.cpp` : `everyCommandDeclaresBothAttributes` (lecture brute du JSON :
  chaque objet a les clés `lockable` et `restrictable`, booléennes) ; `lockGreysExactlyTheLockables` (verrou posé :
  pour chaque commande disponible sous Windows, `isEnabled` faux ⇔ `lockable`) ; `lockedCountMatchesTheData`
  (`Commands::lockedCount()` = nombre calculé depuis le JSON pour la plateforme) ; `unlockRestoresOwnState` (une
  commande désactivée avant le verrou reste désactivée après, `isOwnEnabled`) ; `androidCountMatchesTheData` (registre
  filtré pour Android).
- [ ] **Étape 2 : données** (attributs manquants des lots 7 à 11 renseignés, liste au rapport) ; **K** = sortie de
  `lockedCountMatchesTheData` (affichée par `qInfo`), écrit dans le scénario.
- [ ] **Étape 3 : scénario** `classe-protegee-complete.json` (forme de `classe-protegee.json`, L6-T31, avec ses
  `waitFor`) : `classroom.lock` → création « essai-1234 » deux fois → `waitFor appClassroom.hasPassword = true` →
  `expect appClassroom.locked = true` → `expect commands.lockedCount = <K>` → capture `verrouille` → mauvais mot de
  passe → `waitFor appClassroom.busy = false` → `expect appClassroom.locked = true` → `{"restart": true}` →
  `waitReady` → `expect appClassroom.locked = true` → `expect commands.lockedCount = <K>` → bon mot de passe →
  `waitFor appClassroom.locked = false`.
- [ ] **Étape 4 : vert**, preuve visuelle et contrôle de l'INI, preuve de tâche ; commit « Lot 11 : mode classe protégé
  rejoué sur toutes les commandes (L11-T26) ».

<a id="tache-l11-t27"></a>
### Tâche L11-T27 — S32 : les cinq profils de bout en bout

**Objectif.** S32 (amendement 1, § 7 ; N2-6 amendée). Pour chacun des profils `en-classe`, `preparation`,
`lecture-ecriture`, `calcul-geometrie`, `son-image` (L6-T6) : activé depuis un profil vierge, chaque outil qu'il
épingle (ceux des lots 7 et 8 compris) est présent dans la palette et la barre de raccourcis (modèle de disposition) ;
les navigateurs et réglages qu'il fixe sont appliqués ; sous Android, aucune commande absente de la plateforme n'y
figure. Fonction pure `actions::layoutForPlatform(const Profile&, const CommandRegistry&, Platform)` (dans
`profile.h`) si le lot 6 n'en a pas (A7) : la disposition effective d'un profil pour une plateforme.

**Fichiers.** Ceux de la ligne de la table.

**Test et commande bornée.** Commande ciblée `-R 'tst_ui_profilesall|tst_captures_profils_complets|tst_ui_profiles'`
(avec `ACTCRAFT_REFERENCE_NAMES`, comme L6-T6).

**Preuve visuelle.** `captures.ps1 -Scenario profils-complets -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire "appProfile.currentId=son-image"` :
pour chaque profil `LU appProfile.currentId=<id> OK` et `LU toolPalette.pinnedCount=<n> OK` (n exact par profil),
`CAPTURES OK` ; décrire `calcul-geometrie.png` et `son-image.png`.

**Interdits.** Aucun nom de profil de l'original ; aucun épinglage inventé : une commande absente du registre est
retirée du profil et listée au rapport ; aucune commande Windows seule dans la disposition Android.

**Terminé quand** les 5 fonctions et le scénario passent, `namesAreOurs` en `PASS`.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_ui_profilesall.cpp` : `everyPinnedCommandExists` ; `activatedFromBlankShowsEveryPin`
  (profil vierge, puis `profile.activate <id>` : chaque épinglage dans le modèle de la palette ou de la barre) ;
  `browsersAndSettingsApplied` (valeurs du profil relues dans `Preferences` et l'état du panneau) ;
  `noAndroidAbsentCommandOnAndroid` (`layoutForPlatform(…, Platform::Android)` ne contient aucune commande dont
  `platforms` exclut Android) ; `lot7And8PinsPresent` (liste des épinglages des lots 7 et 8 relevée dans
  le rapport des épinglages du lot 7, dont L11-T0 écrit le chemin réel — ou « absent : liste lue dans les profils du
  lot 7 » —, et son équivalent du lot 8, recopiée dans le test).
- [ ] **Étape 2 : implémentation** et corrections des profils.
- [ ] **Étape 3 : scénario** `profils-complets.json` : `waitReady` ; pour chaque profil : `command profile.activate
  <id>` → `expect appProfile.currentId = <id>` → `expect toolPalette.pinnedCount = <n>` (n calculé par le test, écrit
  ici ; propriété ajoutée à `ToolPalette.qml` si absente → fichier ajouté à la ligne par l'orchestrateur) → capture
  `<id>`.
- [ ] **Étape 4 : vert**, preuve visuelle, preuve de tâche ; `NOMS OK` ; contrôle des titres du corpus vide ; commit
  « Lot 11 : les cinq profils contrôlés de bout en bout (L11-T27) ».

<a id="tache-l11-t28"></a>
### Tâche L11-T28 — Version 1.0.0, `CHANGELOG.md`, `README.md`

**Objectif.** D11-16 ; décision 31 du cadrage ; décision 14 (guide de démarrage).
- `CMakeLists.txt` : `VERSION 1.0.0` (le `versionCode` en découle, règle de `app/CMakeLists.txt:26-35`) ;
- `CHANGELOG.md` : section `## 1.0.0 — <date de la tâche>` en tête, en français, pour un enseignant : ce que le lot 11
  apporte (toucher, deux utilisateurs, tablette, reprise, aide, mises à jour, accessibilité), puis un paragraphe
  « Depuis la 0.1.0 » qui résume les sections précédentes sans les recopier ;
- `README.md` : section « Démarrer » : installer sous Windows (installateur par utilisateur, avertissement SmartScreen
  N1-4), sous Android (APK, autoriser l'installation depuis cette source), premiers pas (écrire, page suivante,
  enregistrer), F1, modes du toucher, deux utilisateurs, où sont les réglages, reprise après interruption, rechercher
  les mises à jour ; aucune capture de l'original ;
- `tests/tst_app_versioninfo.cpp` et `tests/app.cmake` (définitions `ACTCRAFT_VERSION_CODE` et `ACTCRAFT_SOURCE_DIR`
  passées au test) ; le test `tst_app_version` existant (`tests/app.cmake:4-5`, `actcraft --version`) est gardé.

**Test et commande bornée.** Commande ciblée `-R tst_app_version` (attrape `tst_app_version` et `tst_app_versioninfo`) ; puis
```powershell
pwsh -NoProfile -Command "Set-Location '<wt>'; . ./scripts/version-projet.ps1; Get-ActCraftVersion; Get-ActCraftVersionCode"
pwsh -NoProfile -File ./scripts/notes-version.ps1 -Version 1.0.0 -Sortie .tmp/notes-1.0.0.md
```
Attendu : `1.0.0`, `10000`, `NOTES OK : .tmp/notes-1.0.0.md`.

**Interdits.** Aucun tag ; aucune mention d'une release publiée ; aucun nom de l'original hors du nom du format
`.flipchart` ; aucun nom de paperboard réel ; aucun titre de vidéo.

**Terminé quand** les 4 fonctions de `tst_app_versioninfo` passent, `tst_app_version` aussi, `10000`, `NOTES OK`, `NOMS OK`, contrôle des titres du corpus vide.

**Étapes.**
- [ ] **Étape 1 : test rouge** `tst_app_versioninfo.cpp` :
  ```cpp
  #include <actcraft/ui/appinfo.h>

  #include <QFile>
  #include <QtTest>

  class TstAppVersionInfo : public QObject
  {
      Q_OBJECT
  private slots:
      void versionIs100() { QCOMPARE(actcraft::ui::AppInfo().version(), QStringLiteral("1.0.0")); }
      void versionCodeIs10000() { QCOMPARE(ACTCRAFT_VERSION_CODE, 10000); }
      void versionCodeFollowsTheVersion()
      {
          const QStringList parts = actcraft::ui::AppInfo().version().split(QLatin1Char('.'));
          QCOMPARE(parts.size(), 3);
          const int expected = parts[0].toInt() * 10000 + parts[1].toInt() * 100 + parts[2].toInt();
          QCOMPARE(ACTCRAFT_VERSION_CODE, expected);
          QVERIFY(ACTCRAFT_VERSION_CODE > 1000); // 0.10.0 → 1000 : la mise à jour par-dessus passe
      }
      void changelogStartsWithTheVersion()
      {
          QFile f(QStringLiteral(ACTCRAFT_SOURCE_DIR "/CHANGELOG.md"));
          QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
          const QString text = QString::fromUtf8(f.readAll());
          const qsizetype first = text.indexOf(QStringLiteral("\n## "));
          QVERIFY(first >= 0);
          QVERIFY(text.mid(first + 1).startsWith(QStringLiteral("## 1.0.0 — ")));
      }
  };

  QTEST_GUILESS_MAIN(TstAppVersionInfo)
  #include "tst_app_versioninfo.moc"
  ```
  `AppInfo` : `ui/include/actcraft/ui/appinfo.h` (vérifié à `5373f04`, `version()` rend un `QString`) ; si son
  constructeur demande des arguments, reprendre la forme réelle.
- [ ] **Étape 2 : rouge vu**, puis version, journal, guide ; vert ; commandes ci-dessus.
- [ ] **Étape 3 : preuve de tâche** ; `NOMS OK` ; contrôle des titres du corpus vide ; commit « Lot 11 : version 1.0.0,
  journal des versions et guide de démarrage (L11-T28) ».

<a id="tache-l11-t29"></a>
### Tâche L11-T29 — Paquets 1.0.0 et S14 : release prête

**Objectif.** S14 (cadrage § 5), S1, S3, S5 sur la 1.0.0 ; contrôle du déploiement du greffon TLS de Windows
(`tls/qschannelbackend.dll`, nécessaire à la recherche de mises à jour) ajouté à `scripts/paquet.ps1` (échec
`PAQUET EN ECHEC : greffon TLS absent`).

**Fichiers.** `scripts/paquet.ps1`, `docs/verifications/2026-10-22-lot11-release-prete.md`.

**Commandes (dans l'ordre, chacune bornée, sorties sous `.tmp/`).**
1. `pwsh -NoProfile -File ./scripts/paquet.ps1 > .tmp/paquet.txt 2>&1` → `PAQUET OK : …\build\paquet\ActCraft-1.0.0-setup.exe`
   (taille rapportée, N1-A10) ; `LICENCES OK` dans la sortie.
2. `pwsh -NoProfile -File ./scripts/essai-installateur.ps1 > .tmp/installateur.txt 2>&1` (annoncé : fenêtre 5 s) →
   `INSTALLATEUR OK` ; `--version` → `ActCraft 1.0.0`.
3. `pwsh -NoProfile -File ./scripts/paquet-android.ps1 > .tmp/apk.txt 2>&1` → `APK OK`, `versionCode = 10000,
   versionName = 1.0.0` ; `verifier-licences.ps1 -Apk …` → `LICENCES OK` ; tailles rapportées.
4. Local : `apksigner verify --print-certs build/paquet/ActCraft-1.0.0-arm64-v8a.apk` → certificat de débogage
   (`CN=Android Debug`).
5. Émulateur : `essai-android.ps1 -Apk build/paquet/ActCraft-1.0.0-x86_64.apk` → `ANDROID OK` ;
   `essai-android-fichiers.ps1 -Apk …` → `ANDROID FICHIERS OK`.
6. Essai à blanc de `release.yml` (sans tag) : depuis le worktree du chantier,
   `git -C <wt> push origin HEAD:refs/heads/essai-ci/release-lot11` ; puis
   `gh run list --branch essai-ci/release-lot11 --workflow release.yml --limit 1` ; `gh run watch <id>` ;
   `gh run view <id> --log > .tmp/release-essai.txt` ;
   `Select-String -Path .tmp/release-essai.txt -Pattern 'APK NON PUBLIABLE|NOTES OK|SHA256SUMS|SECRETS ABSENTS|CONTROLE OK|ActCraft-1\.0\.0-(setup\.exe|arm64-v8a\.apk)' | Select-Object -Last 14`
   → tous les motifs présents ; `SECRETS ABSENTS` et `CONTROLE OK` viennent du job `controle-mode-reel`
   (`release.yml:110-147`) : c'est la preuve de S14 que le mode réel, sans secrets, échoue avec son message.
7. Aucune release créée : `gh release list --limit 5` (lecture) → aucune entrée `v1.0.0`.
8. `git -C <wt> push origin --delete essai-ci/release-lot11`.

**Interdits.** **Aucun tag** (`git tag`, `git push … v1.0.0`, `--tags`, `--follow-tags`) ; aucune commande
`gh release create|edit|upload|delete` ; aucun appel d'API en écriture sur les releases ou les tags ; aucun
`gh workflow run release.yml -f mode=reel` (le job `secrets` référence l'environnement `release`, que GitHub créerait
alors sans la règle des tags `v*`) ; aucune création du
fichier go ; aucun secret lu, posé ni affiché ; aucun téléversement de paquet ; aucun paquet distribué hors du poste.

**Terminé quand** les huit points sont verts et le rapport `…-release-prete.md` est commité :
noms, tailles et SHA-256 des deux paquets locaux, numéros des runs, lignes relevées.

**Étapes.**
- [ ] **Étape 1 :** contrôle du greffon TLS dans `scripts/paquet.ps1` ; essai rouge (renommer temporairement le dossier
  `tls` du déploiement de `build/` → `PAQUET EN ECHEC : greffon TLS absent`), remise en place, `PAQUET OK`.
- [ ] **Étape 2 :** points 1 à 5.
- [ ] **Étape 3 :** points 6 à 8.
- [ ] **Étape 4 :** rapport ; `git add scripts/paquet.ps1 docs/verifications/2026-10-22-lot11-release-prete.md` ;
  `NOMS OK` ; commit « Lot 11 : paquets 1.0.0 et flux de release essayé à blanc (L11-T29) ».

<a id="tache-l11-t30"></a>
### Tâche L11-T30 — Clôture du lot et porte de fin

**Objectif.** Contrôle du lot 11 sur le worktree du chantier, à jour de `main`, avant la PR ; préparer la porte de fin :
release prête, go demandé. Passe de fluidité finale (décision 14).

**Fichiers.** `tests/tst_ui_shell.cpp` (`lot11StubsGone` : aucun marqueur dans les sources, les tests et les
scénarios, liste R2 complète ; le marqueur est construit dans le test, `QStringLiteral("Talon ") + QStringLiteral("L11")`,
pour que le test ne se compte pas lui-même), `docs/mesures/2026-10-22-lot11-fin.md`, `docs/verifications/2026-10-22-lot11-porte.md`, `graphify-out/`.

**Commandes.**
1. Suites : `pwsh -NoProfile -File ./scripts/test.ps1 -Preset release` puis `pwsh -NoProfile -File ./scripts/test.ps1`
   (formes bornées) → `TESTS OK` deux fois, `out of <N11 + 32>`, aucun `SKIP` de talon ;
   `git -C <wt> grep -l "Talon L11" -- . ':!docs/' ':!graphify-out/' ':!tests/tst_ui_shell.cpp'` vide.
2. Jeton `ecran` (annoncé à Jérôme d'une ligne : « captures et banc à l'écran principal pendant ≈ 20 min ») :
   `$env:ACTCRAFT_UPDATES_URL = 'file:///<wt avec des barres obliques>/tests/data/updates/newer.json'` **avant**
   `pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario tous -Rendu d3d11 -Sortie .tmp/captures-ecran > .tmp/captures-ecran.txt 2>&1`
   → `CAPTURES OK` (`tous` ne prend pas `bench/scenarios/controle/` ; aucune requête vers l'API réelle) ; puis
   `pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario controle/pseudo-langue -Rendu d3d11 -Traduction build/debug/i18n/actcraft_xx.qm -Sortie .tmp/captures-ecran-pseudo > .tmp/captures-ecran-pseudo.txt 2>&1`
   → `TEXTES OK`, `CAPTURES OK`.
3. Banc complet D3D11 plein écran, trois passages (S13 rejoué) :
   `pwsh -NoProfile -File ./scripts/bench.ps1 > .tmp/bench-<i>.txt 2>&1` ; verdicts, figements et reprises relevés ;
   comparaison à la référence de fin du lot précédent (`docs/mesures/`) : tout critère tenu par la référence l'est
   encore, sinon une tâche de correction `L11-C<n>` précède la PR (porte de non-régression du profil).
4. `graphify update .` ; commit de `graphify-out/`.
5. Rapport de mesure `docs/mesures/2026-10-22-lot11-fin.md` (JSON cités, jamais commités).
6. Rapport de porte `docs/verifications/2026-10-22-lot11-porte.md` : ce qui est livré ; hypothèses H11-1 à H11-11 ;
   tailles des paquets (N1-A10) ; manières modernes choisies (coffre 0100) ; **mesures qui reviennent à Jérôme** et
   leurs procédures ; **procédure du go** (ci-dessous), écrite pour être suivie telle quelle **après** le go.

**Mesures pour Jérôme (procédures exactes, mode autonome point 4).**
- Dell Inspiron 15 3510 : `pwsh -NoProfile -File ./scripts/paquet-banc.ps1` sur le poste de développement → zip de
  `build/paquet-banc/` copié sur le Dell ; procédure de `docs/banc.md` (dézipper, lancer, renvoyer le JSON).
- MacBook Air M1 : dans un clone du dépôt à jour, `scripts/bench.sh --rendu tous` ; renvoyer `bench/results/*.json`.
- Tablette Android : `docs/banc-tablette.md` (L11-T15).
Ces mesures fondent la décision go / no-go de l'étape 2 du BRIEF ; elles ne bloquent pas la PR.

**Procédure du go (exécutée seulement après le go écrit de Jérôme, jamais par cette tâche).**
1. Clé de signature (N1-6) : `keytool -genkeypair -v -keystore actcraft-release.jks -alias actcraft -keyalg RSA -keysize 4096 -validity 10000`
   sur le poste de Jérôme ; copie hors ligne gardée par Jérôme ; empreinte SHA-256 notée
   (`keytool -list -v -keystore actcraft-release.jks -alias actcraft`).
2. Environnement GitHub `release` (Settings → Environments), règle « Selected branches and tags », motif de tag `v*` ;
   secrets d'environnement `ACTCRAFT_KEYSTORE_BASE64`, `ACTCRAFT_KEYSTORE_PASSWORD`, `ACTCRAFT_KEY_ALIAS`,
   `ACTCRAFT_KEY_PASSWORD` par `gh secret set <nom> --env release` (saisis par Jérôme).
3. Fichier go `<git-common-dir>/chantiers/go-action-finale` écrit par l'orchestrateur au go (limite de 6 h).
4. `git tag -a v1.0.0 -m "ActCraft 1.0.0"` sur le commit mergé dans `main`, puis `git push origin v1.0.0`.
5. Fin du workflow `release.yml` : brouillon avec installateur, APK signé, `SHA256SUMS.txt`, notes ; l'empreinte
   SHA-256 du certificat de l'APK (`apksigner verify --print-certs`) égale celle du point 1 (S14).
6. `gh release edit v1.0.0 --draft=false` sous le même go ; suppression du fichier go aussitôt après.

**Interdits.** Aucun tag, aucune commande `gh release` en écriture, aucun fichier go, aucun merge (le merge passe par la
PR et attend la CI verte, profil) ; aucune fenêtre visible hors du jeton `ecran`.

**Terminé quand** les points 1 à 6 sont faits et commités ; la porte de fin est prête à être présentée par
l'orchestrateur : « release 1.0.0 prête, go écrit demandé ».

**Étapes.**
- [ ] **Étape 1 :** `lot11StubsGone` rouge si un talon reste, puis vert ; suites.
- [ ] **Étape 2 :** captures et banc à l'écran (jeton `ecran`, annoncé).
- [ ] **Étape 3 :** rapports, graphe ; `git add` des chemins nommés ; `NOMS OK` ; contrôle des titres du corpus vide ;
  commit « Lot 11 : clôture du lot, mesures et porte de fin (L11-T30) ».

## Scénarios et tâches (11b)

| Scénario | Tâches |
|---|---|
| S29 — Aide, langue et mises à jour | L11-T17, L11-T18 (aide, F1 sans réseau) ; L11-T19, L11-T20 (pseudo-langue : fenêtre, menus, trois boîtes, aucune chaîne sans marque) ; L11-T21, L11-T22 (réponse simulée, aucune requête au démarrage) |
| S19 — Mode classe protégé, rejoué au lot 11 sur toutes les commandes | L11-T26 |
| S32 — Profils fournis | L11-T27 |
| S14 — Release prête | L11-T28, L11-T29, L11-T30 (porte ; le tag attend le go) |
| S2.1 — Raccourcis (posé par ce plan) : chaque raccourci déclenche sa commande, aucun pendant la saisie | L11-T23 |
| S2.2 — Accessibilité (posé par ce plan) : AA et focus visible dans chaque boîte et en mise en page étroite | L11-T24 |
| S1.1 — Notices (posé par ce plan) : toute bibliothèque livrée a sa notice ; « À propos » montre le même texte | L11-T25 |
| S13, S2, S3 (rejoués) | L11-T30 |

## Hypothèses pour Jérôme (suite de H11-1 à H11-6 ; porte de fin)

### Niveau 1
- **H11-7 — Mises à jour.** La recherche contacte `api.github.com` quand on la demande (ou au démarrage si le réglage
  est coché) ; rien d'autre n'est envoyé.
- **H11-8 — Licences.** Notices complètes, non validées par un juriste (N1-8).
- **H11-9 — Mesures sur les machines de référence** (Dell 3510, M1, tablette) : procédures de L11-T30 ; décision go /
  no-go de l'étape 2 du BRIEF.
- **H11-10 — Taille des paquets 1.0.0** (N1-A10), rapportée par L11-T29.

### Niveau 2
- **H11-11** Variables et options d'essai (`ACTCRAFT_UPDATES_URL` limitée à `file:` et à l'hôte local ;
  `--traduction` du joueur de captures seulement).

## Relecture par l'auteur (writing-plans)

- Couverture : S29 (aide, langue, mises à jour), S19 rejoué, S32, S14 ont leurs tâches ; finitions demandées (contrastes
  et focus, traductions, notes de version et `CHANGELOG.md`, paquets Windows et Android, licences tierces) aussi ;
  macOS sans paquet (D11-15).
- La release n'a lieu que sur go : L11-T29 et L11-T30 l'interdisent explicitement ; la procédure du go est écrite pour
  l'après.
- Noms cohérents avec 11a : `appUpdates`, `UpdateDialog`, `appLayout`, `opensDialog`, `lockedCount`, `appProfile`,
  N = N11 + 32.
