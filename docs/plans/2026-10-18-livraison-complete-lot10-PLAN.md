# Livraison complète — lot 10 « Cache incrémental » (H-mes-1) — plan d'exécution

> Exécution : étage 3 de nouvelle-feature — l'orchestrateur lit la table, une tâche = sa section.

**But.** Version **0.3.0** : plus aucun figement ni aucune reprise hors limite au banc, en Direct3D 11 plein écran,
sur tous les scénarios, dont un nouveau scénario à pauses longues (S13) ; traits lissés (anticrénelés) en
Direct3D 11 comme en rendu logiciel (constat L2-C11 ; recommandation de la porte de fin du lot 2 appliquée faute
d'autre réponse de Jérôme, pas un choix explicite : à lui présenter à la porte de fin du lot 10). Seule change
l'**implémentation** de l'interface de cache figée au lot 2 (`core::PageCache` : `insert`, `remove`, `invalidate`) :
aucun outil, aucun appelant de cette interface ne change (décision 13 et DA11 du cadrage).

**Architecture.** Aujourd'hui, chaque trait terminé fait redessiner tout le cache de la page (le
`ShaderEffectSource` de `PageArea.qml` photographie les 300 objets de `FixedPageItem`) dans l'image même où l'encre
se vide : c'est le figement mesuré (64,4 ms au prototype, 41,65 ms au lot 1). Au lot 10, un objet ajouté en fin de
page **attend** au-dessus du cache : il n'entre pas dans la scène que le cache photographie, une **couche
d'attente** (`render::PendingLayerItem`, nouvelle) le dessine par-dessus. Le cache est redessiné plus tard,
**hors de l'écriture** : après 500 ms sans entrée, au-delà de 128 objets en attente, à la fin d'un zoom, ou à tout
redessin complet ; ce redessin « verse » les objets en attente dans la scène dans le même tour de boucle que la
demande, de sorte que la même image montre le cache qui les contient et la couche d'attente qui ne les dessine
plus. Annuler un objet encore en attente ne redessine rien. **Limite déclarée de DA11** : seuls les objets insérés
en fin de page, sans image, sur une page sans grille dessinée au-dessus des objets, sont différés ; les autres (forme
posée sous la couche du dessus, image insérée, trait sur une page à grille au-dessus) gardent le redessin immédiat
du lot 2 (écart 5, H10-8). Le banc gagne le scénario `pauses`, dont chaque reprise
tombe dans la fenêtre du versement différé. Si une reprise y dépasse sa limite, une tâche conditionnelle passe à un
cache par tuiles (DA11). Anticrénelage : échantillonnage multiple (MSAA 4×) de la fenêtre et de la texture du cache
en Direct3D 11, mesuré au banc ; la grille reste nette (règle de rendu relevée au pixel sur la référence).

**Pile.** C++20, Qt 6.12 (Core, Gui, Quick, QuickControls2, Test), CMake + Ninja, MSVC 2022, PowerShell 7.

## Table des tâches

| id | titre | dépend de | fichiers touchés | worktree | subagent | preuve | scénario | section |
|----|-------|-----------|------------------|----------|----------|--------|----------|---------|
| L10-T0 | Ouverture : vidéos, REA et ancien dépôt confrontés au lot (rendu, lissage, réactivité, zoom, annulation) | — | `docs/verifications/2026-10-18-lot10-principes.md` (hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot10.md`) | isolé | concepteur-critique | rapport commité, relevé brut hors dépôt présent, `NOMS OK`, `git grep` des titres du corpus vide | S30 (adapté au lot 10) | #tache-l10-t0 |
| L10-T1 | Banc : scénario `pauses` (reprises dans la fenêtre du versement), redessins du cache relevés par scénario | — | `ui/include/actcraft/ui/viewtransform.h`, `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp`, `bench/include/actcraft/bench/result.h`, `bench/src/result.cpp`, `bench/src/runner.cpp`, `tests/tst_bench_scenarios.cpp`, `tests/tst_bench_result.cpp`, `docs/banc.md` | isolé | implementeur-tdd | `100% tests passed … out of N`, N ≥ 91, `TESTS OK`, `tst_bench_scenarios` et `tst_bench_result` Passed (dont `pausesResumeInsideTheFoldWindow`, `fiveScenariosWithFrames`) ; banc hors écran : scénario `pauses` présent, `cache.refreshes` relevé | S13 | #tache-l10-t1 |
| L10-T2 | `render` : fabrique de nœuds d'objet partagée (refonte sans changement de comportement) | — | `render/src/objectnodes.h`, `render/src/objectnodes.cpp`, `render/src/fixedpageitem.cpp`, `render/CMakeLists.txt` | isolé | implementeur-tdd | `TESTS OK`, N ≥ 91, `tst_render_fixedpage` et `tst_render_painted` Passed ; mutation de `wrapped` rouge puis remise ; banc hors écran `content_check.ok = True` | S13 | #tache-l10-t2 |
| L10-T5 | Anticrénelage en Direct3D 11 (MSAA 4× fenêtre et cache) ; étape `checkEdge` du joueur ; grille nette | L10-T0 | `app/renderbackend.h`, `app/renderbackend.cpp`, `tests/tst_renderbackend.cpp`, `ui/qml/PageArea.qml`, `bench/include/actcraft/bench/contentcheck.h`, `bench/src/contentcheck.cpp`, `tests/tst_bench_content.cpp`, `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`, `tests/tst_bench_uiscenario.cpp`, `bench/src/fixtures.cpp`, `bench/scenarios/anticrenelage.json`, `tests/bench.cmake` | isolé | concepteur-critique | `TESTS OK`, N ≥ 92, `tst_renderbackend`, `tst_bench_content`, `tst_bench_uiscenario`, `tst_captures_anticrenelage` Passed ; à l'écran D3D11 (jeton `ecran`) : `BORD trait-diagonal OK`, `BORD grille-nette OK`, `LU pageCache.samples=4 OK` ; banc A/B D3D11 avant / après | S33 | #tache-l10-t5 |
| L10-T3 | Page fixe : objets en attente hors de la scène du cache ; couche d'attente `PendingLayerItem` | L10-T0, L10-T2 | `render/include/actcraft/render/fixedpageitem.h`, `render/src/fixedpageitem.cpp`, `render/include/actcraft/render/pendinglayeritem.h`, `render/src/pendinglayeritem.cpp`, `render/CMakeLists.txt`, `tests/render.cmake`, `tests/tst_render_pendinglayer.cpp` | isolé | concepteur-critique | `TESTS OK`, N ≥ 92, `tst_render_pendinglayer` Passed (dont `foldShowsTheSameImage`, `undoingAPendingStrokeRebuildsNothing`, `pendingStrokeMovedToTheFrontFoldsEverything`), `tst_render_fixedpage` Passed ; banc hors écran `content_check.ok = True` | S13 | #tache-l10-t3 |
| L10-T4 | Vue : versement différé (inactivité, plafond, zoom), couche d'attente branchée, scénario `cache-differe` | L10-T1, L10-T3, L10-T5 | `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`, `ui/src/uiforeigntypes.h`, `ui/qml/PageArea.qml`, `tests/tst_pageview.cpp`, `tests/tst_ui_drawing.cpp`, `bench/include/actcraft/bench/result.h`, `bench/src/result.cpp`, `bench/src/runner.cpp`, `tests/tst_bench_verdict.cpp`, `tests/tst_bench_result.cpp`, `bench/scenarios/cache-differe.json`, `tests/bench.cmake` | isolé | concepteur-critique | `TESTS OK`, N ≥ 94, `tst_pageview` (dont `strokeWaitsAboveTheCacheUntilIdle`, `wheelPostponesTheFold`, `hoverDoesNotPostponeTheFold`), `tst_ui_drawing` (dont `deferredStrokeStaysVisibleThroughTheFold`), `tst_captures_cache_differe`, `tst_bench_verdict`, `tst_bench_result` (dont `deferredFoldShowsInTheResult`) Passed ; `captures.ps1 -Scenario cache-differe` hors écran `CAPTURES OK` ; banc hors écran : `stroke` → `cache.refreshes = 0`, `stroke-folded` OK | S13 | #tache-l10-t4 |
| L10-T6 | Mesure S13 : trois passages D3D11 plein écran encadrés par la référence, condition « machine calme », décision tuiles | L10-T4 | `docs/mesures/AAAA-MM-JJ-cache-incremental.md` (date du passage) | isolé | concepteur-critique | 3 passages D3D11 plein écran consécutifs du lot 10 : `freeze_over_limit = 0` et `resume_over_limit = 0` sur les 5 scénarios ; référence avant et après ; RAM libre relevée à chaque passage ; résumé commité ; décision écrite sur L10-T7 | S13 | #tache-l10-t6 |
| L10-T7 | (Conditionnelle) Cache par tuiles : seules les tuiles touchées par les objets versés sont redessinées | L10-T6 | `render/include/actcraft/render/fixedpageitem.h`, `render/src/fixedpageitem.cpp`, `tests/tst_render_pendinglayer.cpp`, `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`, `ui/qml/PageArea.qml`, `tests/tst_pageview.cpp`, `tests/tst_ui_drawing.cpp`, `docs/mesures/AAAA-MM-JJ-cache-incremental.md` | isolé | concepteur-critique | jouée seulement si L10-T6 l'exige ; `TESTS OK`, N ≥ 94, `tst_pageview` (dont `foldRefreshesOnlyTheTouchedTiles`) Passed ; mesure S13 de L10-T6 rejouée et tenue | S13 | #tache-l10-t7 |

8 tâches, dont une conditionnelle (L10-T7), un seul plan. Tests ctest ajoutés : L10-T5 +1
(`tst_captures_anticrenelage`), L10-T3 +1 (`tst_render_pendinglayer`), L10-T4 +1 (`tst_captures_cache_differe`) :
**94** à la fin du lot (91 à la fin du lot 2). Les minima de la table comptent les 91 du lot 2, plus les tests
ajoutés par la tâche et par celles dont elle dépend ; « ≥ » est la règle.

Worktrees : `.worktrees/livraison-complete--L10-T<n>` (profil), tous isolés : deux tâches d'une même vague ne
partagent aucun fichier (vérifié sur la colonne « fichiers touchés »).

**Écran.** L10-T5 et L10-T6 ouvrent des fenêtres à l'écran (D3D11) sous le jeton `ecran` : l'orchestrateur
l'annonce à Jérôme d'une ligne **avant** de lancer chacune. Aucune autre tâche n'ouvre de fenêtre visible.

## Vagues (parallélisme 2 au plus)

V1 : L10-T0 ‖ L10-T1 · V2 : L10-T2 ‖ L10-T5 · V3 : L10-T3 · V4 : L10-T4 · V5 : L10-T6 · V6 : L10-T7 (si exigée).

Fichiers partagés tenus par les dépendances : `ui/qml/PageArea.qml` et `tests/bench.cmake` (L10-T5 → L10-T4 →
L10-T7) ; `render/src/fixedpageitem.cpp` et `render/CMakeLists.txt` (L10-T2 → L10-T3 → L10-T7) ;
`bench/include/actcraft/bench/result.h`, `bench/src/result.cpp`, `bench/src/runner.cpp`, `tests/tst_bench_result.cpp`
(L10-T1 → L10-T4) ;
`ui/src/pageview.cpp`, `tests/tst_pageview.cpp`, `tests/tst_ui_drawing.cpp` (L10-T4 → L10-T7) ;
`tests/tst_render_pendinglayer.cpp` (L10-T3 → L10-T7) ; le résumé de mesure (L10-T6 → L10-T7).

**L10-T0 et les autres tâches.** La règle `Lx-T0` de l'amendement (§ 4) vaut pour les lots 3 à 8 et 11 ; pour le
lot 10, la tâche d'ouverture est demandée par la décision 0100 (vidéos et REA avant chaque lot). Les tâches dont le
comportement peut en dépendre (L10-T3, L10-T5, donc L10-T4 et la suite) l'attendent ; L10-T1 (outil de mesure) et
L10-T2 (refonte sans effet visible) ne l'attendent pas. Si L10-T0 rapporte un écart qui touche le cache ou le rendu
des traits, l'orchestrateur amende ce plan (tâche ajoutée, relecture du plan exigée) avant la vague V3.

## Ce que l'ancien dépôt dit déjà (relevé pour ce plan, reformulé)

Lu dans `C:\Users\JeromeDenis\REA - Activinspire` (référence de comportement, consigne de Jérôme du 2026-10-18) ;
L10-T0 le complète par le corpus et par le binaire.

1. **Traits lissés à l'écran.** L'application de référence dessine l'encre du stylo et du marqueur avec
   l'anticrénelage actif, comme notre rendu logiciel ; notre rendu Direct3D 11 pose des triangles de couleur
   unie, sans lissage (constat L2-C11). → L10-T5.
2. **Grille nette, jamais lissée.** Les lignes de grille font un pixel d'écran quel que soit le zoom, sans
   lissage (règle relevée au pixel sur l'original, `app/actcraft/canvas/grid.py`, en-tête). Notre grille passe
   par une texture peinte sans lissage : l'échantillonnage multiple ne doit pas la flouter. → contrôle négatif de
   L10-T5 (`BORD grille-nette`).
3. **Forme du trait.** La référence lisse aussi la **courbe** du trait (passage par les milieux des points
   successifs) et lui donne des bouts et des jonctions ronds ; marqueur à 50 % d'opacité. Notre trait est fait de
   segments droits à bouts carrés (`render/include/actcraft/render/strokegeometry.h`, `core::PenCap::Square` du
   prototype). C'est un comportement de l'**outil** stylo, pas du cache : **écarté du lot 10**, transmis au lot 3
   (sa tâche d'ouverture `L3-T0` le tranche sur la 3.5.18) — hypothèse H10-6.
4. **Cache de rendu.** L'original garde des caches de rendu attachés à sa vue de page (architecture relevée,
   `docs/re/architecture.md` § 4) ; leur politique exacte n'est pas relevée. L10-T0 cherche, en statique seulement,
   ce que le binaire laisse voir de leur invalidation (zoom, annulation) ; rien de ce qu'il trouve ne s'écrit dans
   le dépôt avec les noms de l'original.

## Écarts au cadrage (repris à la porte de fin du lot 10)

1. **S6 et S7 n'existent pas encore.** S13 demande de les rejouer « à l'identique sur la nouvelle
   implémentation » ; l'ordre des lots modifié par Jérôme (lot 10 juste après le lot 2) les place aux lots 3 et 4,
   qui n'ont pas commencé. Remplacement : S2 (`premiers-pas`), S4 (`ouvrir-flipchart-synthetique` et
   `local/ouvrir-flipchart`), S5 (`aller-retour-natif`) et le nouveau `cache-differe` rejoués à l'écran en D3D11 à
   la clôture. S6 et S7 seront, de fait, joués sur la nouvelle implémentation à la clôture des lots 3 et 4 (H10-1).
2. **Trois passages « consécutifs ».** Les trois passages du lot 10 se suivent sans passage du lot 10 écarté entre
   eux ; un passage de la référence les précède et un les suit, pour qualifier l'état de la machine (leçon de la
   mesure du lot 2 : `docs/mesures/2026-10-17-lot2-document.md`).
3. **Référence du lot 10.** Le commit de merge de L10-T1 (ancienne implémentation du cache, banc du lot 10) : la
   0.2.0 n'a pas le scénario `pauses`. La non-régression du lot 2 (0.1.0 contre 0.2.0) et celle du lot 10 (0.2.0
   contre 0.3.0) se jouent ensemble à la clôture, sur `stroke`, `zoom`, `scroll`.
4. **Scénario S33 ajouté par ce plan.** Il vient des recommandations de la porte de fin du lot 2 (« traits sans
   anticrénelage en D3D11 traités au lot 10 »), appliquées **faute d'autre réponse** de Jérôme (sa seule réponse
   écrite : « Pas maintenant » pour la release), pas d'un choix explicite de sa part. À verser au cadrage et à
   présenter comme tel à la porte de fin du lot 10.
5. **Limite de DA11 déclarée.** Seuls les objets insérés en fin de page, sans image, sur une page sans grille
   dessinée au-dessus, attendent au-dessus du cache. Trois cas gardent le redessin immédiat du lot 2 (un figement
   possible) : (a) une forme ou un objet posé sous la couche du dessus (outils du lot 4) ; (b) une image insérée
   (lot 5) ; (c) un trait sur une page dont la grille est dessinée au-dessus des objets (lot 4). S7 et S8 sont des
   scénarios de captures, pas des mesures : un scénario de banc est **à ajouter au banc du lot 4** (cas a et c) et
   **du lot 5** (cas b). Si leur mesure dépasse la limite : tuiles (L10-T7, si elle a été construite) ou tâche du
   lot concerné (H10-8).
6. **Volume.** Le lot passe de 4 tâches estimées (cadrage § 1, amendement § 6) à 8, dont une conditionnelle :
   tâche d'ouverture (décision 0100), anticrénelage (porte du lot 2), outil de mesure et refonte. À présenter à la
   porte de fin du lot 10.

## Scénario ajouté

- **S33 — Traits lissés en Direct3D 11 (lot 10).** À l'écran, `captures.ps1 -Scenario anticrenelage -Rendu d3d11` :
  un trait noir diagonal posé au stylet injecté a, sur une ligne horizontale qui le traverse, au moins 2 pixels
  intermédiaires (ni blanc ni noir) quand il vient d'être tracé, après son versement au cache et au zoom 300 % ;
  une ligne de grille horizontale n'en a aucun ; la texture du cache est multi-échantillonnée (`pageCache.samples`
  = 4). Hors écran en rendu logiciel, le même scénario passe (le peintre lisse déjà) : `tst_captures_anticrenelage`.

## Couverture des scénarios

| Scénario | Tâches | Joué à la clôture |
|---|---|---|
| S13 — plus de figement | L10-T1 (mesure), L10-T2, L10-T3, L10-T4 (implémentation), L10-T6 (preuve), L10-T7 (repli) | oui : les 3 passages de L10-T6 (rejoués si un correctif de fin de lot touche `render/ input/ tools/ ui/`), S2, S4, S5 et `cache-differe` à l'écran en D3D11 |
| S30 — tâche d'ouverture (adapté) | L10-T0 | contrôlé à la relecture : rapport, relevé brut, `NOMS OK` |
| S33 — traits lissés en D3D11 | L10-T5 (et L10-T4 : étape `checkEdge` sur l'objet en attente) | oui : `anticrenelage` et `cache-differe` à l'écran en D3D11 |

## Carte des fichiers

| Fichier | Responsabilité | Tâche |
|---|---|---|
| `docs/verifications/2026-10-18-lot10-principes.md` | constats du corpus, de l'ancien dépôt et du binaire, en nos mots ; principe → exigence → tâche → scénario, ou raison | L10-T0 |
| `ui/include/actcraft/ui/viewtransform.h` | `kCacheFoldIdleMs` (500), `kCacheFoldMaxPending` (128) | L10-T1 |
| `bench/include/actcraft/bench/scenarios.h`, `bench/src/scenarios.cpp` | `pausesScenario`, `kPausesStepNs` ; `allScenarios` à cinq scénarios | L10-T1 |
| `bench/include/actcraft/bench/result.h`, `bench/src/result.cpp`, `bench/src/runner.cpp` | `ScenarioResult::cacheRequestNs`, `cacheUpdatedNs`, `pendingMax` ; JSON `cache.refreshes`, `cache.pending_max`, `raw.cache_request_ns`, `raw.cache_updated_ns` ; contrôle `stroke-folded` | L10-T1, L10-T4 |
| `render/src/objectnodes.h`, `render/src/objectnodes.cpp` | `ObjectNode`, `createObjectNode`, `wrapObjectNode` (internes à `render/`) | L10-T2 |
| `render/include/actcraft/render/fixedpageitem.h` | page fixe : objets en attente (`setDeferredFolding`, `foldPending`, `pendingCount`, `pendingIds`, `pendingRevision`, `cachedObjectCount`, signal `pendingChanged`) | L10-T3 |
| `render/include/actcraft/render/pendinglayeritem.h` | `PendingLayerItem` : dessine les objets en attente au-dessus du cache | L10-T3 |
| `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp` | politique du versement : `pendingLayer`, minuterie d'inactivité, plafond, versement dans `requestCacheRefresh` | L10-T4 |
| `ui/qml/PageArea.qml` | `samples` du cache (L10-T5) ; `PendingLayer` entre le cache et l'encre (L10-T4) | L10-T5, L10-T4 |
| `app/renderbackend.*` | `kMsaaSamples` ; format de surface multi-échantillonné en RHI | L10-T5 |
| `bench/src/uiscenario.cpp`, `bench/src/contentcheck.cpp` | étape `checkEdge` (ligne `BORD`), `countIntermediatePixels` ; fixture `grille` | L10-T5 |
| `bench/scenarios/anticrenelage.json`, `bench/scenarios/cache-differe.json` | S33, preuve visuelle du versement différé | L10-T5, L10-T4 |
| `docs/mesures/AAAA-MM-JJ-cache-incremental.md` | résumé S13 (A/B, machine calme) | L10-T6 |

## Contrats partagés entre tâches (noms figés)

Tous les noms des lots 1 et 2 restent figés ; on ajoute, on ne renomme pas.

- `core::PageCache`, `core::CacheArea`, `core::applyToCache` : **inchangés** (DA11). `ui::DocumentController`,
  `ui::ControllerSink`, `ui::FixedPageSink`, `tools::PenTool` : **inchangés** (appelants et outils).
- `actcraft::ui` (L10-T1) : `inline constexpr int kCacheFoldIdleMs = 500;`, `inline constexpr int
  kCacheFoldMaxPending = 128;` dans `viewtransform.h`.
- `actcraft::bench` (L10-T1) : `ScenarioPlan pausesScenario(ScenarioSet)`, nom `"pauses"`, `inline constexpr
  std::int64_t kPausesStepNs = 8'000'000;` ; `allScenarios` rend `stroke, zoom, scroll, page, pauses` ;
  `ScenarioResult::cacheRequestNs`, `ScenarioResult::cacheUpdatedNs` ; JSON du scénario : objet `cache` avec
  `refreshes` ; `raw.cache_request_ns`, `raw.cache_updated_ns`.
- `actcraft::render` (L10-T2, interne) : `struct ObjectNode { QSGNode* root; QSGNode* leaf; bool viaPainter; }`,
  `ObjectNode createObjectNode(const core::PageObject&, std::size_t index, const RendererRegistry&, const
  PaintedRenderer&, const RenderContext& context, const RenderContext& painted)`.
- `actcraft::render` (L10-T3) : `FixedPageItem::setDeferredFolding(bool)`, `deferredFolding()`, `foldPending()`,
  `pendingCount()` (propriété QML `pendingCount`, signal `pendingChanged`), `pendingIds()`, `pendingRevision()`,
  `cachedObjectCount()`, `forceRasterized()` ; `PendingLayerItem` (propriété `fixedPage`, `builtCount()` crochet
  de test).
- `actcraft::ui` (L10-T4) : `PageView::pendingLayer` (propriété QML), `PageView::foldTimerActive()` (crochet de
  test) ; type QML `PendingLayer` ; `objectName` `pendingLayer` dans `PageArea.qml`. `cacheReady` garde son sens :
  ce qui est affiché (cache + couche d'attente) correspond à la page et au zoom courants.
- `bench` (L10-T4) : `ScenarioResult::pendingMax`, JSON `cache.pending_max`, contrôle de contenu `stroke-folded`
  (cinq contrôles au total).
- `app` (L10-T5) : `inline constexpr int kMsaaSamples = 4;` dans `renderbackend.h`.
- `bench` (L10-T5) : `UiStepKind::CheckEdge` (ajouté à la fin de l'énumération) ; étape
  `{"checkEdge": "nom", "from": [x, y], "to": [x, y], "minIntermediate": n, "maxIntermediate": m}` ; ligne
  `BORD <nom> OK|ECART (intermédiaires <k>, attendu …)` ; `int countIntermediatePixels(const QImage&, QPoint from,
  QPoint to, QColor background, QColor ink, int margin)` dans `contentcheck.h` ; fixture `grille`.

## Règles communes à toutes les tâches (rappelées dans chaque section)

Celles des plans 2a et 2b s'appliquent toutes, mot pour mot (`docs/plans/2026-10-15-livraison-complete-lot2a-PLAN.md`
et `…-lot2b-PLAN.md`, « Règles communes ») : processus enfant pour toute commande PowerShell, `git -C`, mutation
remise par `git checkout --`, `format.ps1` avant la preuve, pièges de Clang (`-Wshadow`, conversions explicites,
pas d'agrégat partiel par position), pas de `std::visit` direct sur `PageObject`, **aucune API privée de Qt** dans le
code livré, propriété intellectuelle, `samples/` en lecture seule, commits explicites en français terminés par
`Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`, arrêt par PID. S'y ajoutent, pour le lot 10 :

- **Machine partagée : toute construction, toute suite, tout banc et toute capture passent par le jeton**
  `C:\Users\JeromeDenis\.claude\scripts\jeton.ps1` : `-Nom tests` pour construire, tester, jouer le banc ou les
  captures hors écran ; `-Nom ecran` pour toute fenêtre visible (D3D11 à l'écran, banc plein écran). Une commande
  par prise ; aucun démon laissé vivant ; si le jeton est long à venir, attendre sa notification, ne pas le
  sonder en boucle.
- `<wt>` = `C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L10-T<n>` ; `<id>` = `L10-T<n>`.
- **Commande ciblée** (rouge, vert d'un seul test) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande '. ./scripts/dev-env.ps1; cmake --build --preset debug *> .tmp/build.txt; ctest --preset debug -R ''<motif>'' --output-on-failure *> .tmp/ctest.txt'; Select-String -Path .tmp/build.txt,.tmp/ctest.txt -Pattern 'error|tests passed|tests failed|No tests were found' | Select-Object -Last 4"
  ```
  Premier passage dans un worktree neuf : la même commande avec `-Commande 'pwsh -NoProfile -File ./scripts/build.ps1 *> .tmp/build.txt'`.
- **Preuve de tâche** (forme bornée du profil, sous le jeton), puis le nom du test :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/test.ps1 > .tmp/test.txt 2>&1'; Select-String -Path .tmp/test.txt -Pattern 'tests passed|TESTS (OK|EN ECHEC)' | Select-Object -Last 2; Select-String -Path .tmp/test.txt -Pattern '<nom_du_test> .*Passed' | Select-Object -Last 1"
  ```
  Attendu : `100% tests passed, 0 tests failed out of N` (N ≥ la valeur de la table), `TESTS OK`, puis la ligne
  `Passed` du test nommé. Un test a tourné : N ≥ 1 et le nom attendu est présent.
- **Banc sans écran** (toute tâche qui touche `render/`, `input/`, `tools/` ou `ui/` ; résultat, pas une porte) :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire <id> -Commande 'pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu logiciel -Scenario court -Taille 1280x800 -HorsEcran > .tmp/bench.txt 2>&1'; Select-String -Path .tmp/bench.txt -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE' | Select-Object -Last 1"
  pwsh -NoProfile -Command "Set-Location '<wt>'; `$j = Get-ChildItem bench/results/*.json | Sort-Object LastWriteTime | Select-Object -Last 1; `$o = Get-Content -Raw `$j | ConvertFrom-Json; Write-Output (`$j.Name + ' content_check.ok = ' + `$o.content_check.ok); `$o.scenarios | ForEach-Object { Write-Output (`$_.name + ' freeze>lim=' + `$_.frames.freeze_over_limit + ' resume>lim=' + `$_.frames.resume_over_limit + ' cache=' + (`$_.cache | ConvertTo-Json -Compress)) }"
  ```
  Attendu : `BUDGET TENU` ou `BUDGET DEPASSE` (jamais `MESURE INVALIDE`), `content_check.ok = True` ; le nom du
  JSON est joint au rapport. Le rendu logiciel hors écran a deux modes connus d'un passage à l'autre (fichier
  d'état, « Pièges ») : il ne juge pas la fluidité, il prouve que le banc tourne et lit ses contrôles.
- **Preuve visuelle hors écran** (forme bornée du profil, sous le jeton `tests`) : lectures `-Lire` d'abord, puis
  description des captures ; motif étendu au lot 10 : `'^(COMMANDE|TRAIT|TOUCHE|ATTENTE|PRET|FENETRE|LU|PIXEL|BORD|CAPTURE) |^SCENARIO |CAPTURES (OK|EN ECHEC)'`.
- **Mesure à l'écran** (L10-T5, L10-T6, clôture) : sous le jeton `ecran`, annoncée à Jérôme par l'orchestrateur.
  **Machine calme** = les trois conditions, relevées juste avant chaque passage et écrites dans le résumé :
  ```powershell
  pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Etat; Write-Output "jeton tests : code $LASTEXITCODE (0 = aucune place tenue)"
  Write-Output ("RAM libre : {0:N2} Go" -f ((Get-CimInstance Win32_OperatingSystem).FreePhysicalMemory / 1MB))
  Get-Process cl, link, ninja, cmake, java -ErrorAction SilentlyContinue | Measure-Object | ForEach-Object { "compilations en cours : $($_.Count)" }
  ```
  (1) jeton `tests` libre (code 0), (2) RAM libre ≥ 4 Go, (3) aucune compilation en cours. **Non réunie** : les
  passages sont joués quand même et **rapportés** (« machine chargée », valeurs relevées), la porte de fluidité
  est déclarée **non jugée**, la tâche **n'échoue pas** ; l'orchestrateur rejoue la mesure à la clôture si la
  machine est devenue calme, et sinon la présente comme non jugée à la porte de fin.
- **Fils de Qt Quick** : `updatePaintNode` tourne sur le fil de rendu pendant que le fil principal est bloqué ; tout
  état lu là (`m_pendingIds`, page) est écrit seulement sur le fil principal. Aucun verrou à ajouter.

---

## Tâches

<a id="tache-l10-t0"></a>
### Tâche L10-T0 — Ouverture : vidéos, REA et ancien dépôt confrontés au lot

**Objectif.** Décision 0100 (coffre `02_Decisions/0100-…`) : avant chaque lot, confronter le lot aux tutoriels
vidéo, au binaire d'origine et au travail de rétro-ingénierie déjà fait. Pour le lot 10, relever ce
qu'ActivInspire montre ou laisse voir de lié au cache et au rendu : lissage des traits (anticrénelage et forme de
la courbe), réactivité au tracé (rien qui fige en fin de trait, encre visible jusqu'au relâché), rendu pendant le
zoom et le défilement (flou puis net, ou net tout de suite), réaction à l'annulation (trait effacé tout de suite),
grille. Écrire le résultat **avec nos mots**, dans le dépôt, et verser les écarts au plan.

**Sources.**
- Corpus : `C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\README.md` (sections « Principes absents » et
  « Écarts avec la spec ») et les fiches (`*.md` du même dossier) qui parlent du stylo, du marqueur, du zoom, du
  défilement, de l'annulation, de la grille.
- Ancien dépôt : `docs/re/` (`architecture.md` § 4 « Rendu, scène et saisie », `interface.md`, `format-flipchart.md`),
  `app/actcraft/canvas/` (`items.py` : encre, `grid.py` : grille relevée au pixel, `tools.py`, `page.py`), et son
  graphe : `graphify query "…"` lancé **dans** `C:\Users\JeromeDenis\REA - Activinspire`.
- Spécification : `C:\Users\JeromeDenis\REA - Activinspire\docs\spec\fonctionnalites.md` (stylo, marqueur, zoom,
  annulation, grille, réglages d'affichage).
- Binaire : skill `reverse-engineer-anything` sur `C:\Program Files\Activ Software\Inspire\`, **en statique
  seulement** (ressources, chaînes, réglages par défaut, symboles du moteur de rendu). Aucune observation à l'écran
  dans ce lot (elle pilote la souris de l'écran principal) : un point qui l'exigerait est noté « non tranché » et
  devient une hypothèse.

**Fichiers.**
- Créer : `docs/verifications/2026-10-18-lot10-principes.md` (dans le dépôt, nos mots).
- Créer hors dépôt : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot10.md` (relevé brut : constats REA,
  noms de l'original, renvois aux fiches). Le dossier `docs\re\lots` est créé s'il manque.

**Test et commande bornée.** Pas de code. Preuves :
```powershell
Test-Path 'C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot10.md'   # True
python -I "C:/Users/JeromeDenis/REA - Activinspire/tools/controle_noms_actcraft.py" "C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L10-T0" | Select-Object -Last 1   # NOMS OK : <n> fichiers
pwsh -NoProfile -Command "Get-ChildItem 'C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\*.md' -Exclude README.md | ForEach-Object { `$t = (Get-Content -LiteralPath `$_.FullName -TotalCount 5 | Where-Object { `$_ -like '# *' } | Select-Object -First 1); if (`$t) { `$t = `$t.Substring(2).Trim(); git -C 'C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--L10-T0' grep -l -i -F -- `$t } }"   # aucune sortie
```

**Interdits.** Aucun titre, aucune transcription, aucune citation de vidéo dans le dépôt (identifiants `Vnn`
seulement, N2-A1). Aucun nom composé de l'original (classes, propriétés, fichiers extraits), aucun texte de l'aide,
aucune capture. Aucune ligne de code. Ne pas modifier `samples/` ni le corpus. Pas d'observation de l'original à
l'écran.

**Terminé quand.** Le rapport existe et a une ligne par constat (principe → exigence en nos mots → tâche du lot
10 → scénario, ou raison de l'écart et lot destinataire) ; le relevé brut existe hors dépôt et est cité par son
chemin ; `NOMS OK` ; la commande des titres ne sort rien ; le rapport de tâche liste, pour l'orchestrateur, les
écarts qui touchent le cache ou le rendu des traits (à verser au plan) et ceux qui vont à d'autres lots.

**Étapes.**

- [ ] **Étape 1 : relever le corpus.** Lire la synthèse, puis chercher les fiches concernées :
  ```powershell
  Select-String -Path 'C:\Users\JeromeDenis\REA - Activinspire\docs\videos-tanguay\*.md' -Pattern 'liss|crénel|net|flou|zoom|défil|annul|rétabl|fig|lent|saccad|rapide|fluid|grille|stylo|marqueur|encre' -List | Select-Object -ExpandProperty Path
  ```
  Pour chaque fiche retenue : noter son numéro `Vnn` (colonne « # » de la table du `README.md`) et le principe
  observé, reformulé en une phrase à nous.
- [ ] **Étape 2 : relever l'ancien dépôt.** `graphify query "rendu des traits lissage anticrénelage"`,
  `graphify query "zoom de la vue et redessin"`, `graphify query "annulation et redessin de la page"` dans l'ancien
  dépôt ; lire `app/actcraft/canvas/items.py` (encre : courbe lissée par les milieux, bouts et jonctions ronds,
  anticrénelage, marqueur à 50 %), `app/actcraft/canvas/grid.py` (grille d'un pixel, sans lissage),
  `docs/re/architecture.md` § 4. Les constats repris par ce plan (§ « Ce que l'ancien dépôt dit déjà ») sont à
  confirmer ou corriger.
- [ ] **Étape 3 : REA statique.** Avec le skill `reverse-engineer-anything` : réglages d'affichage et de qualité du
  rendu (lissage, qualité des traits, accélération graphique), valeurs par défaut des profils pour le stylo,
  traces d'un cache de rendu de la page et de ses invalidations (zoom, défilement, annulation). Écrire les
  constats bruts, avec les noms de l'original, dans le relevé hors dépôt.
- [ ] **Étape 4 : écrire le rapport.** `docs/verifications/2026-10-18-lot10-principes.md` :
  ```markdown
  # Lot 10 — principes confrontés (vidéos, ancien dépôt, binaire)

  Relevé brut (hors dépôt) : `C:\Users\JeromeDenis\REA - Activinspire\docs\re\lots\lot10.md`. Références `Vnn` :
  numéros des fiches du corpus (table hors dépôt). Constats écrits par nous.

  | # | Principe (nos mots) | Source | Exigence pour ActCraft | Tâche | Scénario | Écart, raison ou lot destinataire |
  |---|---|---|---|---|---|---|
  | P1 | Les traits sont lissés à l'écran, quel que soit le moteur | ancien dépôt (encre), Vnn… | trait anticrénelé en D3D11 | L10-T5 | S33 | — |
  | P2 | La grille reste nette, un pixel d'écran | ancien dépôt (grille) | aucune ligne de grille lissée | L10-T5 | S33 | — |
  | P3 | La courbe du trait est lissée, bouts ronds | ancien dépôt (encre) | — | — | — | outil stylo : lot 3 (L3-T0), H10-6 |
  | … | | | | | | |
  ```
  Remplir chaque ligne avec ce que les étapes 1 à 3 ont établi (les trois lignes ci-dessus sont celles du plan, à
  confirmer ou corriger) ; une ligne par principe du corpus lié au rendu, au tracé, au zoom, au défilement, à
  l'annulation ou à la grille.
- [ ] **Étape 5 : contrôles.** Lancer les trois commandes de « Test et commande bornée » ; corriger jusqu'à
  `NOMS OK` et sortie vide.
- [ ] **Étape 6 : commit.**
  ```powershell
  git -C <wt> add docs/verifications/2026-10-18-lot10-principes.md
  git -C <wt> commit -m "Lot 10 : principes du rendu et du cache confrontés aux vidéos, à l'ancien dépôt et au binaire (L10-T0)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l10-t1"></a>
### Tâche L10-T1 — Banc : scénario `pauses`, redessins du cache relevés par scénario

**Objectif.** DA11 : « le banc gagne un scénario à pauses longues pour que le redessin différé soit mesuré ». Le
scénario `stroke` laisse 300 ms entre deux traits : un redessin déclenché après une inactivité plus longue n'y
arriverait jamais, et la mesure tromperait. `pauses` trace 10 traits (4 en court) séparés par **le délai
d'inactivité plus 0 à 64 ms** (0 à 48 ms en court) : chaque reprise du stylet tombe dans la fenêtre où le cache est redessiné.
La reprise k = 0 tombe à l'instant même du versement : l'appui relance la minuterie et ne mesure rien de sûr ;
seules les reprises k ≥ 1 comptent pour juger le versement (L10-T6), à condition que le plus grand retard
d'injection et de réception du scénario (`max` des retards de l'objet `input` du JSON) reste sous 8 ms, le pas qui
sépare deux reprises. Le banc
relève aussi, par scénario, les instants des demandes de redessin du cache et de leurs fins (`cache.refreshes`,
`raw.cache_request_ns`, `raw.cache_updated_ns`), pour attribuer un figement ou une reprise à un redessin (L10-T6).
La tâche ne change pas le cache : sur l'implémentation du lot 2, `pauses` doit montrer un redessin par trait.

**Fichiers.**
- Modifier : `ui/include/actcraft/ui/viewtransform.h`, `bench/include/actcraft/bench/scenarios.h`,
  `bench/src/scenarios.cpp`, `bench/include/actcraft/bench/result.h`, `bench/src/result.cpp`,
  `bench/src/runner.cpp`, `tests/tst_bench_scenarios.cpp`, `tests/tst_bench_result.cpp`, `docs/banc.md`.

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_bench_scenarios` → `error` (`pausesScenario`
inconnu). Vert : preuve de tâche, N ≥ 91, `TESTS OK`, `tst_bench_scenarios .*Passed`, `tst_bench_result .*Passed`
(`tst_bench_result` dépend de `tst_bench_smoke`, qui joue le banc court en debug : il exécute `pauses`). Banc sans
écran : `pauses` dans la liste, `cache={"refreshes":4}` pour `pauses` en court (un redessin par trait, ancienne
implémentation).

**Interdits.** Ne pas changer `stroke`, `zoom`, `scroll` ni `page` (comparaison avec les lots 1 et 2). Ne toucher
ni `render/` ni `ui/src/`. Un test existant ne change que là où il compte les scénarios (quatre → cinq), raison
écrite dans le commit.

**Terminé quand.** Preuve verte, JSON du banc joint (nom, `pauses` présent, `cache.refreshes`), commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests du scénario.** Dans `tests/tst_bench_scenarios.cpp`, remplacer le corps de
  `allScenariosInOrder` et ajouter deux cas :

```cpp
    void allScenariosInOrder()
    {
        const auto plans = bench::allScenarios(bench::ScenarioSet::Court);
        QCOMPARE(plans.size(), std::size_t{5}); // L2-T23 : « page » ; L10-T1 : « pauses », en dernier
        QCOMPARE(plans[0].name, QStringLiteral("stroke"));
        QCOMPARE(plans[1].name, QStringLiteral("zoom"));
        QCOMPARE(plans[2].name, QStringLiteral("scroll"));
        QCOMPARE(plans[3].name, QStringLiteral("page"));
        QCOMPARE(plans[4].name, QStringLiteral("pauses"));
    }

    // L10-T1 (DA11) : chaque reprise tombe après le délai d'inactivité du cache, décalée de 0 à 64 ms (court : 0 à 48), pour
    // tomber dans la fenêtre du redessin différé ; page du banc rechargée (300 objets).
    void pausesResumeInsideTheFoldWindow()
    {
        for (const auto set : {bench::ScenarioSet::Complet, bench::ScenarioSet::Court}) {
            const bench::ScenarioPlan plan = bench::pausesScenario(set);
            QCOMPARE(plan.name, QStringLiteral("pauses"));
            QCOMPARE(plan.generatedPages, 1);
            QCOMPARE(plan.zoom, 1.0);
            std::vector<std::int64_t> pauses;
            std::int64_t lastRelease = -1;
            for (const auto& in : plan.inputs) {
                QVERIFY(!core::hitsWitness({in.pagePos.x(), in.pagePos.y()}, 16.0));
                QCOMPARE(in.measured, in.kind != InputKind::TabletRelease);
                if (in.kind == InputKind::TabletPress && lastRelease >= 0) {
                    pauses.push_back(in.offsetNs - lastRelease);
                }
                if (in.kind == InputKind::TabletRelease) {
                    lastRelease = in.offsetNs;
                }
            }
            const std::size_t strokes = set == bench::ScenarioSet::Court ? 4 : 10;
            QCOMPARE(pauses.size(), strokes - 1);
            const std::int64_t idleNs = std::int64_t{ui::kCacheFoldIdleMs} * 1'000'000;
            for (std::int64_t p : pauses) {
                QVERIFY(p >= idleNs);
                QVERIFY(p <= idleNs + 64'000'000);
            }
            // k = 0 : l'appui tombe à l'instant du versement et relance la minuterie ; il ne mesure rien de sûr.
            // Seules les reprises k >= 1 jugent le versement (L10-T6).
            QCOMPARE(pauses.front(), idleNs);
            QVERIFY(pauses.back() - pauses.front() >= 40'000'000); // la fenêtre du versement est balayée
        }
    }

    void pausesComesAfterTheOthersWithoutChangingThem()
    {
        const auto plans = bench::allScenarios(bench::ScenarioSet::Complet);
        QCOMPARE(plans[0].inputs.size(), bench::strokeScenario(bench::ScenarioSet::Complet).inputs.size());
        QCOMPARE(plans[4].inputs.size(), std::size_t{10 * 100});
    }
```

  Le fichier inclut déjà `<actcraft/ui/viewtransform.h>` ; sinon l'ajouter.

- [ ] **Étape 2 : écrire le test du résultat.** Dans `tests/tst_bench_result.cpp`, renommer `fourScenariosWithFrames`
  en `fiveScenariosWithFrames` (cinquième scénario ajouté par le plan) et en adapter le corps :

```cpp
    // L2-T23 : « page » ; L10-T1 : « pauses » (reprises après le délai d'inactivité du cache).
    void fiveScenariosWithFrames()
    {
        const QJsonArray scenarios = m_root.value("scenarios").toArray();
        QCOMPARE(scenarios.size(), 5);
        const QStringList names{QStringLiteral("stroke"), QStringLiteral("zoom"), QStringLiteral("scroll"),
                                QStringLiteral("page"), QStringLiteral("pauses")};
        for (qsizetype i = 0; i < 5; ++i) {
            const QJsonObject s = scenarios.at(i).toObject();
            QCOMPARE(s.value("name").toString(), names.at(i));
            QVERIFY(s.value("frames").toObject().value("count").toInt() > 0);
            QVERIFY(s.value("frames").toObject().value("active").toInt() > 0);
        }
    }

    // L10-T1 : instants des demandes de redessin du cache et de leurs fins, par scénario. Implémentation du
    // lot 2 : au moins un redessin par trait de « pauses » (4 en court).
    void cacheRefreshesAreRecorded()
    {
        const QJsonObject pauses = m_root.value("scenarios").toArray().at(4).toObject();
        const QJsonObject cache = pauses.value("cache").toObject();
        QVERIFY(cache.contains("refreshes"));
        const QJsonObject raw = pauses.value("raw").toObject();
        const QJsonArray requests = raw.value("cache_request_ns").toArray();
        const QJsonArray updates = raw.value("cache_updated_ns").toArray();
        // Lot 2 : un redessin par trait de « pauses », 4 en court ; au moins 3 exigés (la fin du dernier peut
        // tomber après la fenêtre). L10-T4 abaisse ce minimum à 2 (versement différé, reprise k = 0 exclue).
        QVERIFY2(cache.value("refreshes").toInt() >= 3, "un redessin par trait attendu (ancien cache)");
        QVERIFY(requests.size() >= 3);
        QVERIFY(updates.size() >= 3);
        for (qsizetype i = 1; i < requests.size(); ++i) {
            QVERIFY(requests.at(i).toDouble() >= requests.at(i - 1).toDouble());
        }
    }
```

- [ ] **Étape 3 : voir le rouge.** Commande ciblée, motif `tst_bench_scenarios` : `error` (`pausesScenario` et
  `kCacheFoldIdleMs` inconnus).

- [ ] **Étape 4 : constantes de la vue.** Dans `ui/include/actcraft/ui/viewtransform.h`, après
  `kCacheRefreshDelayMs` :

```cpp
// Lot 10 (DA11) : un objet ajouté en fin de page attend au-dessus du cache ; le cache le prend à son prochain
// redessin, au plus tard après kCacheFoldIdleMs sans entrée, ou dès kCacheFoldMaxPending objets en attente
// (H10-2 : valeurs de départ, réglées par le banc).
inline constexpr int kCacheFoldIdleMs = 500;
inline constexpr int kCacheFoldMaxPending = 128;
```

- [ ] **Étape 5 : le scénario.** Dans `bench/include/actcraft/bench/scenarios.h`, après `pageScenario` :

```cpp
// Lot 10 (DA11) : décalage de la reprise d'un trait à l'autre, après le délai d'inactivité du cache.
inline constexpr std::int64_t kPausesStepNs = 8'000'000;
// Traits séparés par ui::kCacheFoldIdleMs + k × kPausesStepNs (complet : 10 traits de 0,5 s, pause après le trait k : k = 0 à 8 ;
// court : 4 traits de 0,2 s, 3k pour k = 0 à 2) : chaque reprise tombe dans la fenêtre du redessin différé du
// cache. Page du banc rechargée avant (generatedPages = 1 : page de 300 objets, sans éléments peints).
ScenarioPlan pausesScenario(ScenarioSet set);
```

  et commenter `allScenarios` : `// stroke, zoom, scroll, page, pauses`. Dans `bench/src/scenarios.cpp`, ajouter
  `#include <actcraft/ui/viewtransform.h>` puis, avant `allScenarios` :

```cpp
ScenarioPlan pausesScenario(ScenarioSet set)
{
    const bool court = set == ScenarioSet::Court;
    const int strokes = court ? 4 : 10;
    const int samples = court ? 40 : 100; // 0,2 s ou 0,5 s à 200 points/s
    const std::int64_t idleNs = std::int64_t{ui::kCacheFoldIdleMs} * 1'000'000;
    ScenarioPlan plan;
    plan.name = QStringLiteral("pauses");
    plan.zoom = 1.0;
    plan.generatedPages = 1;
    std::int64_t start = 0;
    for (int k = 0; k < strokes; ++k) {
        for (int i = 0; i < samples; ++i) {
            const double u = static_cast<double>(i) / (samples - 1);
            PlannedInput in;
            in.offsetNs = start + i * kStylusPeriodNs;
            in.kind = i == 0             ? InputKind::TabletPress
                      : i == samples - 1 ? InputKind::TabletRelease
                                         : InputKind::TabletMove;
            // Même zone centrale que « stroke » (hors des témoins et de la bande de démonstration).
            in.pagePos =
                QPointF(220.0 + 584.0 * u, 170.0 + 44.0 * k + 20.0 * std::sin(2 * std::numbers::pi * 2 * u));
            in.pressure = in.kind == InputKind::TabletRelease
                              ? 0.0f
                              : static_cast<float>(0.5 + 0.4 * std::sin(2 * std::numbers::pi * u));
            in.measured = in.kind != InputKind::TabletRelease;
            plan.inputs.push_back(in);
        }
        const int step = court ? 3 * k : k;
        start += (samples - 1) * kStylusPeriodNs + idleNs + step * kPausesStepNs;
    }
    return plan;
}
```

  et `allScenarios` rend `{strokeScenario(set), zoomScenario(set), scrollScenario(set), pageScenario(set),
  pausesScenario(set)}`. La pause qui suit le trait k vaut le délai plus k × 8 ms (complet, k = 0 à 8 : 0 à 64 ms) ou 3k × 8 ms (court, k = 0 à 2 : 0 à 48 ms) ; le test de l'étape 1 le borne.

- [ ] **Étape 6 : relever les redessins.** Dans `bench/include/actcraft/bench/result.h`, à `ScenarioResult`
  (corriger aussi le commentaire du nom : `// stroke | zoom | scroll | page | pauses`) :

```cpp
    // L10-T1 : PageView::cacheRefreshRequested et cacheUpdated pendant le scénario (horloge
    // core::monotonicNowNs()), pour attribuer un figement ou une reprise à un redessin du cache.
    std::vector<std::int64_t> cacheRequestNs;
    std::vector<std::int64_t> cacheUpdatedNs;
```

  Dans `bench/src/runner.cpp`, `Runner::play`, juste après la connexion `frozen` :

```cpp
    std::vector<std::int64_t> cacheRequests;
    std::vector<std::int64_t> cacheUpdates;
    const auto requested =
        QObject::connect(m_view, &ui::PageView::cacheRefreshRequested, m_view,
                         [&cacheRequests] { cacheRequests.push_back(core::monotonicNowNs()); });
    const auto updated = QObject::connect(m_view, &ui::PageView::cacheUpdated, m_view,
                                          [&cacheUpdates] { cacheUpdates.push_back(core::monotonicNowNs()); });
```

  après `QObject::disconnect(frozen);` :

```cpp
    QObject::disconnect(requested);
    QObject::disconnect(updated);
    s.cacheRequestNs = std::move(cacheRequests);
    s.cacheUpdatedNs = std::move(cacheUpdates);
```

  Dans `Runner::run`, les deux blocs qui suivent `play` et ne valent que pour le scénario des changements de page
  (relevé `atZoom` et `checkPageFlash`) passent de `if (plan.generatedPages > 0)` à
  `if (plan.name == QLatin1String("page"))` ; le chargement des pages générées avant le scénario reste sur
  `plan.generatedPages > 0` (il sert aussi à `pauses`). Même condition pour la connexion `zoomWatch` (elle ne
  relève rien hors de « page », mais on garde l'intention lisible).

- [ ] **Étape 7 : écrire le JSON.** Dans `bench/src/result.cpp`, `scenarioJson` : ajouter au `raw`

```cpp
        {"cache_request_ns", relative(s.cacheRequestNs, s.startNs)},
        {"cache_updated_ns", relative(s.cacheUpdatedNs, s.startNs)},
```

  et à l'objet rendu, après `{"memory", memory}` : `{"cache", QJsonObject{{"refreshes",
  static_cast<double>(s.cacheRequestNs.size())}}}`.

- [ ] **Étape 8 : documenter.** Dans `docs/banc.md`, à la liste des scénarios, ajouter : « `pauses` (lot 10) : 10
  traits (4 en court) séparés par le délai d'inactivité du cache (500 ms) plus 0 à 64 ms (0 à 48 ms en court) ; chaque reprise tombe
  dans la fenêtre du redessin différé ; page du banc rechargée. Champs `cache.refreshes`, `raw.cache_request_ns`,
  `raw.cache_updated_ns` : redessins du cache pendant le scénario. »

- [ ] **Étape 9 : vert.** `pwsh -NoProfile -File ./scripts/format.ps1` dans le worktree, puis preuve de tâche
  (`tst_bench_scenarios`, puis `tst_bench_result`), puis banc sans écran : `pauses … cache={"refreshes":4}` attendu
  (au moins 4 ; un de plus si le versement de fin de scénario tombe dans la fenêtre).

- [ ] **Étape 10 : commit.**
  ```powershell
  git -C <wt> add ui/include/actcraft/ui/viewtransform.h bench/include/actcraft/bench/scenarios.h bench/src/scenarios.cpp bench/include/actcraft/bench/result.h bench/src/result.cpp bench/src/runner.cpp tests/tst_bench_scenarios.cpp tests/tst_bench_result.cpp docs/banc.md
  git -C <wt> commit -m "Banc : scénario « pauses » et redessins du cache relevés par scénario (L10-T1)" -m "Les tests qui comptaient quatre scénarios en comptent cinq : le scénario « pauses » est ajouté en dernier par le plan du lot 10." -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l10-t2"></a>
### Tâche L10-T2 — `render` : fabrique de nœuds d'objet partagée

**Objectif.** La couche d'attente (L10-T3) doit poser exactement les mêmes nœuds que la page fixe pour un objet
(même moteur, même repère pour une image illisible, mêmes nœuds de transformation et d'opacité) : sinon l'objet
changerait d'aspect au moment de son versement au cache. Sortir de `FixedPageItem::updatePaintNode` la création du
nœud d'un objet dans une fonction interne à `render/`, **sans aucun changement de comportement**.

**Fichiers.**
- Créer : `render/src/objectnodes.h`, `render/src/objectnodes.cpp`
- Modifier : `render/src/fixedpageitem.cpp`, `render/CMakeLists.txt`

**Test et commande bornée.** Refonte : pas de test rouge propre. Les tests existants couvrent le code déplacé
(`tst_render_painted` : `sceneMatchesThePainter`, transformation, translucidité, miroir ; `tst_render_fixedpage`).
Preuve de couverture par mutation : dans `objectnodes.cpp`, mettre `opacity->setOpacity(1.0)` → commande ciblée,
motif `tst_render_painted|tst_render_fixedpage` → au moins un échec ; remettre par `git -C <wt> checkout --
render/src/objectnodes.cpp`. Vert : preuve de tâche, N ≥ 91, `tst_render_fixedpage .*Passed`,
`tst_render_painted .*Passed`. Banc sans écran : `content_check.ok = True`.

**Interdits.** Aucun changement de comportement ni de test. Pas d'en-tête public (`render/src/`, pas
`render/include/`).

**Terminé quand.** Preuve verte, mutation vue rouge puis remise, JSON du banc joint, commit fait.

**Étapes.**

- [ ] **Étape 1 : l'en-tête.** Créer `render/src/objectnodes.h` :

```cpp
#pragma once

#include <actcraft/core/model.h>
#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/paintedrenderer.h>

#include <cstddef>

class QSGNode;

namespace actcraft::render {

// Nœud d'un objet de page, tel que la page fixe le pose (et la couche d'attente, lot 10) : moteur du
// prototype s'il sait faire, sinon moteur peint ; image illisible : repère du peintre, comme la miniature ;
// transformation et translucidité par nœuds parents. root == nullptr : objet invisible, groupe, ou rien à
// dessiner. leaf : nœud du moteur (avant enveloppe) ; viaPainter : il vient du moteur peint (en RHI, nœud
// rastérisé à refaire quand l'échelle change).
struct ObjectNode
{
    QSGNode* root = nullptr;
    QSGNode* leaf = nullptr;
    bool viaPainter = false;
};

ObjectNode createObjectNode(const core::PageObject& object, std::size_t index, const RendererRegistry& registry,
                            const PaintedRenderer& painter, const RenderContext& context,
                            const RenderContext& painted);

} // namespace actcraft::render
```

- [ ] **Étape 2 : la fonction.** Créer `render/src/objectnodes.cpp` en déplaçant `wrapped` de
  `fixedpageitem.cpp` et le corps de la boucle d'objets de `updatePaintNode` (lignes « Ordre du vecteur … » à
  `node = wrapped(node, object.common);`) :

```cpp
#include "objectnodes.h"

#include <QMatrix4x4>
#include <QSGOpacityNode>
#include <QSGTransformNode>
#include <QTransform>

namespace actcraft::render {

namespace {
// Transformation et translucidité de l'objet par des nœuds parents, quel que soit son moteur : aucun moteur
// ne les applique lui-même (sinon double transformation).
QSGNode* wrapped(QSGNode* node, const core::ObjectCommon& common)
{
    if (!common.transform.isIdentity()) {
        const core::Transform& t = common.transform;
        auto* transform = new QSGTransformNode;
        transform->setMatrix(QMatrix4x4(QTransform(t.m11, t.m12, t.m21, t.m22, t.dx, t.dy)));
        transform->appendChildNode(node);
        node = transform;
    }
    if (common.translucency != 0) {
        auto* opacity = new QSGOpacityNode;
        opacity->setOpacity(1.0 - common.translucency / 255.0);
        opacity->appendChildNode(node);
        node = opacity;
    }
    return node;
}
} // namespace

ObjectNode createObjectNode(const core::PageObject& object, std::size_t index, const RendererRegistry& registry,
                            const PaintedRenderer& painter, const RenderContext& context,
                            const RenderContext& painted)
{
    ObjectNode out;
    if (!object.common.visible || std::get_if<core::Group>(&object)) {
        return out;
    }
    QSGNode* node = nullptr;
    bool viaPainter = usesPaintedRenderer(object);
    if (viaPainter) {
        node = painter.createNode(object, painted);
    } else if (const ObjectRenderer* renderer = registry.find(core::kindOf(object))) {
        node = renderer->createNode(object, index, context);
        if (!node && std::get_if<core::ImageObject>(&object)) {
            node = painter.createNode(object, painted); // image illisible : repère du peintre
            viaPainter = true;
        }
    }
    if (!node) {
        return out;
    }
    out.leaf = node;
    out.viaPainter = viaPainter;
    out.root = wrapped(node, object.common);
    return out;
}

} // namespace actcraft::render
```

- [ ] **Étape 3 : la page fixe l'emploie.** Dans `render/src/fixedpageitem.cpp` : retirer `wrapped`, inclure
  `"objectnodes.h"`, et remplacer le corps de la boucle `for (std::size_t i = m_builtCount; …)` par :

```cpp
        for (std::size_t i = m_builtCount; i < shown.objects.size(); ++i) {
            const ObjectNode made =
                createObjectNode(shown.objects[i], i, m_registry, m_painted, context, painted);
            if (!made.root) {
                continue;
            }
            if (made.viaPainter && rasterized) {
                addRasterSlot(made.leaf, i); // feuille : les nœuds de transformation et d'opacité restent en place
            }
            if (m_gridDrawnAbove) {
                root->insertChildNodeBefore(made.root, m_gridDrawnAbove);
            } else {
                root->appendChildNode(made.root);
            }
        }
```

  Retirer les inclusions devenues inutiles (`QMatrix4x4`, `QSGOpacityNode`, `QTransform` si plus employées ;
  `QSGTransformNode` reste pour la racine logicielle).

- [ ] **Étape 4 : CMake.** Dans `render/CMakeLists.txt`, après `src/fixedpageitem.cpp` : `src/objectnodes.h` et
  `src/objectnodes.cpp`.

- [ ] **Étape 5 : couverture par mutation.** Mutation de l'objectif (`setOpacity(1.0)`), commande ciblée rouge,
  remise par `git -C <wt> checkout -- render/src/objectnodes.cpp`.

- [ ] **Étape 6 : vert.** `format.ps1`, preuve de tâche, banc sans écran.

- [ ] **Étape 7 : commit.**
  ```powershell
  git -C <wt> add render/src/objectnodes.h render/src/objectnodes.cpp render/src/fixedpageitem.cpp render/CMakeLists.txt
  git -C <wt> commit -m "render : fabrique de nœuds d'objet partagée, sans changement de comportement (L10-T2)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l10-t5"></a>
### Tâche L10-T5 — Anticrénelage en Direct3D 11 ; étape `checkEdge` ; grille nette

**Objectif.** Constat L2-C11, tranché à la porte de fin du lot 2 : en Direct3D 11, la page fixe dessine les traits
nets (0 ou 255 en bordure) alors que le rendu logiciel les lisse ; la référence les lisse (§ « Ce que l'ancien dépôt
dit déjà », point 1). Demander l'échantillonnage multiple (MSAA 4×) à la fenêtre (traits de l'encre et de la couche
d'attente, dessinés directement) **et** à la texture du cache (`ShaderEffectSource.samples`), en RHI seulement. La
grille, texture peinte sans lissage, doit rester nette (point 2).

**Patron à ne pas reprendre.** Le cache seul multi-échantillonné (le plus simple) : l'encre et la couche d'attente
resteraient crénelées, et un trait changerait d'aspect à son versement. L'échantillonnage de la fenêtre seule ne
suffit pas non plus : la texture du cache ne l'hérite pas. Et ne pas juger le lissage par `checkPixel` : sa moyenne
3 × 3 donne des gris même sur un bord crénelé (c'est ce qui a trompé L2-C11) ; la preuve compte des pixels un à un
sur une ligne qui traverse un bord **diagonal** (un bord crénelé n'y a aucun pixel intermédiaire).

**Fichiers.**
- Modifier : `app/renderbackend.h`, `app/renderbackend.cpp`, `tests/tst_renderbackend.cpp`, `ui/qml/PageArea.qml`,
  `bench/include/actcraft/bench/contentcheck.h`, `bench/src/contentcheck.cpp`, `tests/tst_bench_content.cpp`,
  `bench/include/actcraft/bench/uiscenario.h`, `bench/src/uiscenario.cpp`, `tests/tst_bench_uiscenario.cpp`,
  `bench/src/fixtures.cpp`, `tests/bench.cmake`
- Créer : `bench/scenarios/anticrenelage.json`

`app/renderbackend.*` n'est pas un appelant du cache : c'est la configuration du rendu, partagée par
`actcraft`, `actcraft-bench` et `actcraft-captures` (les trois appellent `applyRenderBackend`), ce qui garantit
que le banc mesure ce que l'utilisateur voit.

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_renderbackend|tst_bench_content|tst_bench_uiscenario`
→ `error` (`kMsaaSamples`, `countIntermediatePixels`, `CheckEdge` inconnus). Vert : preuve de tâche, N ≥ 92,
`tst_renderbackend`, `tst_bench_content`, `tst_bench_uiscenario`, `tst_captures_anticrenelage` Passed. Preuve
visuelle hors écran : `captures.ps1 -Scenario anticrenelage -Rendu logiciel -HorsEcran` → `LU pageCache.samples=4
OK`, `BORD trait-diagonal OK`, `BORD trait-zoom OK`, `BORD grille-nette OK`, `CAPTURES OK`. **À l'écran en D3D11**
(jeton `ecran`, annoncé par l'orchestrateur) : d'abord sous mutation (échantillonnage coupé, étape 10 : la base
n'a pas l'étape `checkEdge`) → `BORD trait-diagonal ECART (intermédiaires 0 …)` (le rouge du constat), puis sans
mutation → les lignes `BORD` OK ;
captures regardées et décrites (bord du trait, grille). Banc A/B D3D11 plein écran (voir étape 10).

**Interdits.** Pas d'anticrénelage dans `render/` par une autre voie (géométrie à bords fondus, shader) sans que
la mesure de l'étape 10 ait refusé le MSAA : c'est le repli, à présenter à l'orchestrateur, pas à écrire d'emblée.
Ne pas lisser la grille. Rien de changé en rendu logiciel.

**Terminé quand.** Preuve verte ; rouge D3D11 vu sur la base puis vert sur la tâche, à l'écran ; banc A/B rapporté
(avec la condition « machine calme ») ; JSON du banc sans écran joint ; commit fait.

**Étapes.**

- [ ] **Étape 1 : test du format de surface.** Dans `tests/tst_renderbackend.cpp`, ajouter
  `#include <QSurfaceFormat>` et les cas :

```cpp
    void rhiRequestsMultisampling()
    {
        QSurfaceFormat::setDefaultFormat(QSurfaceFormat());
        applyRenderBackend(RenderBackend::Direct3D11);
        QCOMPARE(QSurfaceFormat::defaultFormat().samples(), kMsaaSamples);
        QCOMPARE(kMsaaSamples, 4);
    }

    void softwareKeepsTheDefaultFormat()
    {
        QSurfaceFormat::setDefaultFormat(QSurfaceFormat());
        applyRenderBackend(RenderBackend::Software);
        QVERIFY(QSurfaceFormat::defaultFormat().samples() <= 1); // le peintre lisse déjà
    }
```

  Si le test n'a pas de `QGuiApplication` (`QTEST_GUILESS_MAIN`), passer à `QTEST_MAIN` : `QSurfaceFormat` et
  `QQuickWindow::setGraphicsApi` le demandent.

- [ ] **Étape 2 : test du compte de pixels.** Dans `tests/tst_bench_content.cpp` :

```cpp
    // L10-T5 : un bord crénelé n'a aucun pixel intermédiaire sur une ligne qui le traverse ; un bord lissé en a.
    void intermediatePixelsTellAliasedFromSmooth()
    {
        QImage aliased(40, 40, QImage::Format_RGB32);
        aliased.fill(Qt::white);
        QImage smooth = aliased;
        {
            QPainter p(&aliased);
            p.setRenderHint(QPainter::Antialiasing, false);
            p.setPen(QPen(Qt::black, 6));
            p.drawLine(QPointF(5, 5), QPointF(35, 35));
        }
        {
            QPainter p(&smooth);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setPen(QPen(Qt::black, 6));
            p.drawLine(QPointF(5, 5), QPointF(35, 35));
        }
        const QPoint from(0, 20);
        const QPoint to(39, 20);
        QCOMPARE(bench::countIntermediatePixels(aliased, from, to, Qt::white, Qt::black, 24), 0);
        QVERIFY(bench::countIntermediatePixels(smooth, from, to, Qt::white, Qt::black, 24) >= 2);
        QCOMPARE(bench::countIntermediatePixels(QImage(), from, to, Qt::white, Qt::black, 24), -1);
        QVERIFY(bench::countInkPixels(aliased, from, to, Qt::black, 24) >= 1); // la ligne croise le trait
        QCOMPARE(bench::countInkPixels(aliased, QPoint(0, 0), QPoint(0, 39), Qt::black, 24), 0);
    }
```

  (inclure `<QPainter>` si le fichier ne l'a pas).

- [ ] **Étape 3 : test de l'étape du joueur.** Dans `tests/tst_bench_uiscenario.cpp`, sur le modèle des cas
  d'analyse existants :

```cpp
    void checkEdgeStepIsParsed()
    {
        const QByteArray json = R"({"format": "actcraft-scenario/1", "name": "x", "steps": [
            {"checkEdge": "bord", "from": [380, 400], "to": [440, 400], "minIntermediate": 2}]})";
        bench::UiScenario s;
        QString error;
        QVERIFY2(bench::parseUiScenario(json, s, error), qPrintable(error));
        QCOMPARE(s.steps.size(), std::size_t{1});
        const bench::UiStep& step = s.steps[0];
        QCOMPARE(step.kind, bench::UiStepKind::CheckEdge);
        QCOMPARE(step.name, QStringLiteral("bord"));
        QCOMPARE(step.at, QPointF(380, 400));
        QCOMPARE(step.to, QPointF(440, 400));
        QCOMPARE(step.minIntermediate, 2);
        QCOMPARE(step.maxIntermediate, -1); // sans plafond
        QCOMPARE(step.minInk, 0);
        const QByteArray bad = R"({"format": "actcraft-scenario/1", "name": "x", "steps": [
            {"checkEdge": "bord", "from": [380, 400]}]})";
        QVERIFY(!bench::parseUiScenario(bad, s, error));
    }
```

  (attention moc et chaînes brutes : pas d'apostrophe dans un `R"(...)"`, piège du lot 2).

- [ ] **Étape 4 : voir le rouge.** Commande ciblée, motif `tst_renderbackend|tst_bench_content|tst_bench_uiscenario`.

- [ ] **Étape 5 : MSAA en RHI.** Dans `app/renderbackend.h`, après l'énumération : `inline constexpr int
  kMsaaSamples = 4; // L10-T5 : échantillonnage multiple en RHI (lissage des traits, constat L2-C11)`. Dans
  `app/renderbackend.cpp`, inclure `<QSurfaceFormat>` et, dans `applyRenderBackend`, avant le `switch` :

```cpp
    if (backend != RenderBackend::Software) {
        // Traits lissés en RHI comme en rendu logiciel (L10-T5) : la fenêtre (encre, couche d'attente) est
        // multi-échantillonnée ; la texture du cache l'est par ShaderEffectSource.samples (PageArea.qml).
        QSurfaceFormat format = QSurfaceFormat::defaultFormat();
        format.setSamples(kMsaaSamples);
        QSurfaceFormat::setDefaultFormat(format);
    }
```

  Si la capture D3D11 de l'étape 9 montre encore un bord crénelé sur le trait qui vient d'être tracé, le format
  par défaut n'a pas atteint la fenêtre : poser le même format sur la fenêtre avant son affichage dans
  `loadMainWindow` (`app/startup.cpp`) et l'écrire en écart au rapport.

- [ ] **Étape 6 : cache multi-échantillonné.** Dans `ui/qml/PageArea.qml`, au `ShaderEffectSource` `pageCache` :
  `samples: 4 // L10-T5 : texture du cache lissée en RHI (sans effet en rendu logiciel)`.
  Mémoire (H10-9) : la texture du cache va jusqu'à 4096 px de côté au zoom 800 % ; multi-échantillonnée 4×, elle
  coûte de + 192 à + 384 Mio selon le pilote, à ajouter au pic du lot 1 (473-698 Mo) sous le budget de 1 Gio. Le
  pic mémoire GPU du scénario `zoom` est relevé à l'étape 10. **Repli** s'il dépasse le budget ou gagne plus de
  200 Mio sur la base : échantillons selon la taille de la texture,
  `samples: view.cacheTextureSize.width * view.cacheTextureSize.height <= 2048 * 2048 ? 4 : 1` (lissé jusqu'à
  2048², net au-delà, aux forts zooms où le crénelage se voit le moins). **1 et non 0** : d'après la documentation
  de Qt, `samples: 0` suit l'échantillonnage de la fenêtre (4 ici) et n'économiserait rien. Puis A/B rejoué, avec
  le gain vérifié au pic mémoire GPU du scénario `zoom`.

- [ ] **Étape 7 : compte de pixels et étape `checkEdge`.** Dans `bench/include/actcraft/bench/contentcheck.h` :

```cpp
// L10-T5 : pixels de la ligne [from, to] (tracé de Bresenham, extrémités comprises) dont la couleur n'est ni à
// `margin` près du fond ni à `margin` près de l'encre, par canal : bords lissés. -1 si l'image est vide ou
// si un point sort de l'image.
int countIntermediatePixels(const QImage& image, QPoint from, QPoint to, QColor background, QColor ink,
                            int margin);
// Pixels de la même ligne à `margin` près de l'encre : contrôle positif (le segment croise bien un trait ou
// une ligne de grille). -1 dans les mêmes cas.
int countInkPixels(const QImage& image, QPoint from, QPoint to, QColor ink, int margin);
```

  Dans `bench/src/contentcheck.cpp` :

```cpp
int countIntermediatePixels(const QImage& image, QPoint from, QPoint to, QColor background, QColor ink,
                            int margin)
{
    if (image.isNull() || !image.rect().contains(from) || !image.rect().contains(to)) {
        return -1;
    }
    int x = from.x();
    int y = from.y();
    const int dx = std::abs(to.x() - x);
    const int dy = -std::abs(to.y() - y);
    const int sx = x < to.x() ? 1 : -1;
    const int sy = y < to.y() ? 1 : -1;
    int err = dx + dy;
    int count = 0;
    for (;;) {
        const QColor c = image.pixelColor(x, y);
        if (!colorNear(c, background, margin) && !colorNear(c, ink, margin)) {
            ++count;
        }
        if (x == to.x() && y == to.y()) {
            break;
        }
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y += sy;
        }
    }
    return count;
}
```

  `countInkPixels` : même parcours de Bresenham, compte les pixels `colorNear(c, ink, margin)`. Écrire le parcours
  une fois, dans une fonction interne qui reçoit un prédicat sur la couleur, et l'appeler depuis les deux
  fonctions publiques (mêmes cas -1).

  Dans `bench/include/actcraft/bench/uiscenario.h`, à la fin de `UiStepKind` :

```cpp
    // Ajoutée au lot 10 (L10-T5, S33), à la fin.
    CheckEdge, // {"checkEdge": "nom", "from": [x, y], "to": [x, y], "minIntermediate": 2, "maxIntermediate": 0, "minInk": 1}
```

  et à `UiStep` : `QPointF to;`, `int minIntermediate = 0;`, `int maxIntermediate = -1; // -1 : sans plafond`,
  `int minInk = 0; // pixels d'encre exigés sur le segment (contrôle positif d'un contrôle négatif)`.
  Déclarer `bool checkEdge(const UiStep& step, QStringList& report);` près de `checkPixel`. Dans
  `bench/src/uiscenario.cpp`, analyse (après le bloc `checkPixel`) :

```cpp
    if (o.contains(QStringLiteral("checkEdge"))) {
        step.kind = UiStepKind::CheckEdge;
        step.name = o.value(QStringLiteral("checkEdge")).toString();
        step.minIntermediate = o.value(QStringLiteral("minIntermediate")).toInt(0);
        step.maxIntermediate = o.value(QStringLiteral("maxIntermediate")).toInt(-1);
        step.minInk = o.value(QStringLiteral("minInk")).toInt(0);
        if (step.name.isEmpty() || !readPoint(o.value(QStringLiteral("from")), step.at) ||
            !readPoint(o.value(QStringLiteral("to")), step.to)) {
            why = QStringLiteral("contrôle de bord invalide (checkEdge, from, to)");
            return false;
        }
        return true;
    }
```

  exécution (près de `checkPixel`) :

```cpp
// L10-T5 (S33) : pixels intermédiaires, un à un, le long d'un segment de page (fond blanc, encre noire,
// marge 24 par canal) ; image du moment, comme checkPixel.
bool UiScenarioPlayer::checkEdge(const UiStep& step, QStringList& report)
{
    ui::PageView* v = view();
    const QImage image = m_window->grabWindow();
    int count = -1;
    int ink = -1;
    if (v && !image.isNull()) {
        const qreal dpr = m_window->devicePixelRatio();
        const QPoint from = sceneToImage(v->pageToScene(step.at), dpr);
        const QPoint to = sceneToImage(v->pageToScene(step.to), dpr);
        count = countIntermediatePixels(image, from, to, QColor(Qt::white), QColor(Qt::black), 24);
        ink = countInkPixels(image, from, to, QColor(Qt::black), 24);
    }
    const bool ok = count >= step.minIntermediate &&
                    (step.maxIntermediate < 0 || count <= step.maxIntermediate) && ink >= step.minInk;
    const QString expected = step.maxIntermediate < 0
                                 ? QStringLiteral("≥ %1").arg(step.minIntermediate)
                                 : QStringLiteral("%1 à %2").arg(step.minIntermediate).arg(step.maxIntermediate);
    report << QStringLiteral("BORD %1 %2 (intermédiaires %3, attendu %4 ; encre %5, au moins %6)")
                  .arg(step.name, ok ? QStringLiteral("OK") : QStringLiteral("ECART"))
                  .arg(count)
                  .arg(expected)
                  .arg(ink)
                  .arg(step.minInk);
    return ok;
}
```

  et dans `runStep` : `case UiStepKind::CheckEdge: return checkEdge(step, report);`. Ajouter `BORD` à la liste des
  lignes du commentaire de `UiScenarioPlayer`.

- [ ] **Étape 8 : fixture `grille` et scénario.** Dans `bench/src/fixtures.cpp`, à côté de `kThreePages` :
  `constexpr QLatin1StringView kGrid("grille");` et

```cpp
// L10-T5 : une page blanche à grille visible (un niveau, pas de 32, lignes noires pleines), pour vérifier que
// le lissage des traits ne touche pas la grille (lignes d'un pixel, nettes, comme la référence).
bool buildGrid(const QString& path, QString& error)
{
    std::vector<core::Page> pages(1);
    core::GridLevel level;
    level.spacingX = 32;
    level.spacingY = 32;
    level.color = 0xFF000000u;
    pages[0].grid.visible = true;
    pages[0].grid.levels.push_back(level);
    core::Document document(std::move(pages));
    document.setTitle("Grille");
    const QByteArray archive = formats::encodeActcraft(document, {}, &error);
    return !archive.isEmpty() && formats::writeUrl(QUrl::fromLocalFile(path), archive, &error);
}
```

  et `{kGrid, buildGrid}` dans `kFixtures`. Créer `bench/scenarios/anticrenelage.json` :

```json
{
  "format": "actcraft-scenario/1",
  "name": "anticrenelage",
  "description": "S33 : trait noir diagonal lissé quand il vient d'être tracé, après son versement au cache et au zoom 300 % ; grille nette ; texture du cache multi-échantillonnée.",
  "steps": [
    {"waitReady": true},
    {"expect": "pageCache.samples", "value": 4},
    {"fixture": "grille", "path": "${SORTIE}/grille.actcraft"},
    {"command": "file.open", "argument": "${SORTIE}/grille.actcraft"},
    {"waitFor": "pageIndicator.text", "value": "1 / 1", "timeoutMs": 10000},
    {"command": "view.zoom", "argument": 100},
    {"waitReady": true},
    {"checkEdge": "grille-nette", "from": [100, 40], "to": [100, 90], "minIntermediate": 0, "maxIntermediate": 0, "minInk": 1},
    {"capture": "grille"},
    {"command": "pen.color", "argument": "#000000"},
    {"command": "pen.width", "argument": 8},
    {"stroke": [[300, 300], [500, 500]], "pressure": 0.5},
    {"waitReady": true},
    {"checkEdge": "trait-diagonal", "from": [370, 400], "to": [430, 400], "minIntermediate": 2},
    {"capture": "trait"},
    {"wait": 800},
    {"waitReady": true},
    {"checkEdge": "trait-verse", "from": [370, 400], "to": [430, 400], "minIntermediate": 2},
    {"command": "view.zoom", "argument": 300},
    {"waitReady": true},
    {"checkEdge": "trait-zoom", "from": [385, 400], "to": [415, 400], "minIntermediate": 2},
    {"capture": "trait-zoom"}
  ]
}
```

  La grille est ouverte d'abord, sur le paperboard vierge du lancement (non modifié : `file.open` ne demande pas
  d'enregistrement) ; le trait est tracé ensuite sur la page à grille. La ligne de grille horizontale `y = 64`
  croise le segment vertical `x = 100` (de `y = 40` à `y = 90`, entre les colonnes de grille 96 et 128) : ses
  pixels sont noirs ou blancs, aucun intermédiaire. Les lignes de grille croisées par le segment du trait (colonnes
  384 et 416) sont noires : elles ne comptent pas comme intermédiaires. Avant de figer les nombres, jouer le
  scénario hors écran (étape 9). Le segment de la grille est **fixé par la géométrie de la fixture** (ligne `y = 64`,
  colonnes 96 et 128) et ne se déplace pas : s'il sort de la vue, changer le zoom ou le défilement, pas le segment.
  Son contrôle positif (`minInk: 1` : au moins un pixel noir, la ligne est bien croisée) rend le contrôle négatif
  probant ; un segment qui ne croise rien échouerait. Toute coordonnée changée pour un trait est rejouée sous la
  mutation de l'étape 10 (rouge exigé), jamais les seuils. Si `grille-nette` échoue **aussi sans MSAA** (mutation de l'étape 10), le défaut
  précède la tâche (grille rééchantillonnée à une position fractionnaire, par exemple) : le rapporter, garder
  l'étape et l'écrire en hypothèse pour l'orchestrateur, sans corriger la grille ici.

  Dans `tests/bench.cmake`, sur le modèle de `tst_captures_premiers_pas` :

```cmake
# Scénario S33 (traits lissés, grille nette), hors écran en rendu logiciel : le peintre lisse déjà ; prouve
# l'étape checkEdge et la lecture de pageCache.samples. La preuve D3D11 se joue à l'écran (L10-T5, clôture).
add_test(NAME tst_captures_anticrenelage
    COMMAND $<TARGET_FILE:actcraft-captures>
        --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/anticrenelage.json
        --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-anticrenelage)
set_tests_properties(tst_captures_anticrenelage PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "SCENARIO OK"
    FAIL_REGULAR_EXPRESSION "ECART|SCENARIO EN ECHEC|SCENARIO INVALIDE")
```

- [ ] **Étape 9 : vert hors écran.** `format.ps1`, preuve de tâche (`tst_captures_anticrenelage .*Passed`), puis
  preuve visuelle hors écran :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire L10-T5 -Commande 'pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario anticrenelage -Rendu logiciel -HorsEcran -Sortie .tmp/captures -Lire pageCache.samples=4 > .tmp/captures.txt 2>&1'; Select-String -Path .tmp/captures.txt -Pattern '^(COMMANDE|TRAIT|TOUCHE|ATTENTE|PRET|FENETRE|LU|PIXEL|BORD|CAPTURE) |^SCENARIO |CAPTURES (OK|EN ECHEC)' | Select-Object -Last 16"
  ```
  Attendu : `LU pageCache.samples=4 OK`, quatre `BORD … OK`, `SCENARIO OK`, `CAPTURES OK` ; décrire `trait-zoom`
  (bord du trait en dégradé) et `grille` (lignes d'un pixel).

- [ ] **Étape 10 : à l'écran en D3D11, rouge puis vert, et banc A/B.** L'orchestrateur a annoncé l'écran à Jérôme.
  **Rouge du constat** : le scénario a besoin de l'étape `checkEdge`, absente de la base ; il se joue donc avec
  l'outil de la tâche, échantillonnage coupé par mutation temporaire (`kMsaaSamples` à 1 dans
  `app/renderbackend.h`, `samples: 4` à `samples: 0` dans `PageArea.qml`, et l'attente `pageCache.samples` à 0
  dans le scénario), construction release, `captures.ps1 -Scenario anticrenelage -Rendu d3d11` ; attendu :
  `BORD trait-diagonal ECART (intermédiaires 0, attendu ≥ 2)`. Remise par `git -C <wt> checkout --
  app/renderbackend.h ui/qml/PageArea.qml bench/scenarios/anticrenelage.json`, reconstruction, même commande :
  quatre `BORD … OK`. Forme :
  ```powershell
  pwsh -NoProfile -Command "Set-Location '<wt>'; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom ecran -Proprietaire L10-T5 -Commande 'pwsh -NoProfile -File ./scripts/captures.ps1 -Scenario anticrenelage -Rendu d3d11 -Sortie .tmp/captures-ecran > .tmp/captures-ecran.txt 2>&1'; Select-String -Path .tmp/captures-ecran.txt -Pattern '^(LU|BORD|CAPTURE) |^SCENARIO |CAPTURES (OK|EN ECHEC)' | Select-Object -Last 12"
  ```
  Banc A/B : construire la base en release dans un worktree détaché (`git -C C:\Users\JeromeDenis\ActCraft worktree
  add --detach .worktrees/livraison-complete--L10-T5-base <commit de base du worktree de la tâche>`, puis
  `scripts/build.ps1 -Preset release` sous le jeton `tests`) ; relever la « machine calme » (règles communes),
  puis, sous le jeton `ecran`, en alternance base → tâche → base → tâche,
  `pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu d3d11 -SansConstruire` dans le worktree concerné. Rapporter pour chaque passage : RAM libre, images/s, p95 et part > 25 ms de `stroke`, `zoom`,
  `scroll`, latence p50/p95, pic mémoire, et **pic mémoire GPU du scénario `zoom`** (`memory.gpu_peak_bytes`, base
  contre tâche ; repli de l'étape 6 si besoin, H10-9). **Critère** : aucun de ces critères tenu par la base n'est perdu par la
  tâche sur les deux paires (machine calme) ; machine chargée : rapporté, non jugé. Si le MSAA fait perdre un
  critère sur machine calme, s'arrêter et le rapporter à l'orchestrateur (repli : bords fondus dans la géométrie,
  tâche nouvelle). Supprimer le worktree de base ensuite (`git worktree remove`, depuis un autre dossier courant).

- [ ] **Étape 11 : commit.**
  ```powershell
  git -C <wt> add app/renderbackend.h app/renderbackend.cpp tests/tst_renderbackend.cpp ui/qml/PageArea.qml bench/include/actcraft/bench/contentcheck.h bench/src/contentcheck.cpp tests/tst_bench_content.cpp bench/include/actcraft/bench/uiscenario.h bench/src/uiscenario.cpp tests/tst_bench_uiscenario.cpp bench/src/fixtures.cpp bench/scenarios/anticrenelage.json tests/bench.cmake
  git -C <wt> commit -m "Traits lissés en Direct3D 11 : MSAA 4× de la fenêtre et du cache, étape checkEdge, grille nette (L10-T5)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l10-t3"></a>
### Tâche L10-T3 — Page fixe : objets en attente ; couche d'attente `PendingLayerItem`

**Objectif.** DA11, cœur du lot : « les objets versés depuis le dernier redessin restent des nœuds au-dessus du
cache ». En mode **différé** (posé par la vue au L10-T4), un objet inséré en fin de page, sans image, sur une page
prête et sans grille dessinée au-dessus des objets, **n'entre pas** dans la scène de `FixedPageItem` (celle que le
cache photographie) : il **attend**. `PendingLayerItem` le dessine, avec les mêmes nœuds (L10-T2). `foldPending()`
le verse dans la scène. Annuler un objet en attente (`remove`) ou le modifier (`invalidate`) ne reconstruit rien :
seule la couche d'attente change. Tout autre changement fait la reconstruction complète du lot 2, qui verse aussi
tout ce qui attend. Hors mode différé, rien ne change (défaut : comportement du lot 2, tests existants inchangés).
Les cas non différés sont la limite déclarée de DA11 (écart 5, H10-8). Un objet en attente que l'on réordonne
(changement annoncé en `ObjectChanged` : `core/include/actcraft/core/undo.h` n'a pas de type « réordonné ») cesse
d'être en queue de page : `invalidate` le détecte et reconstruit tout.

**Invariant partagé (à tester, c'est le piège).** À chaque synchronisation, la scène de `FixedPageItem` contient
exactement les `cachedObjectCount()` premiers objets de la page, et la couche d'attente exactement les suivants
(`pendingIds()`, dans l'ordre de la page). Les deux lisent le même état, écrit seulement sur le fil principal :
ni trou (objet dessiné par personne) ni double (objet translucide dessiné deux fois, plus foncé).

**Patron à ne pas reprendre.** (1) Laisser `FixedPageItem` ajouter le nœud tout de suite et compter sur
`live: false` pour qu'il n'apparaisse pas dans le cache : tout objet inséré entre une demande de redessin et la
synchronisation suivante serait alors dans le cache **et** dans la couche d'attente. (2) Garder l'objet dans
l'encre (`InkItem`) jusqu'au versement : il faudrait changer l'outil stylo, interdit (décision 13).
(3) Mettre la couche d'attente sous `FixedPageItem` : `hideSource` du cache la cacherait avec lui.

**Fichiers.**
- Créer : `render/include/actcraft/render/pendinglayeritem.h`, `render/src/pendinglayeritem.cpp`,
  `tests/tst_render_pendinglayer.cpp`
- Modifier : `render/include/actcraft/render/fixedpageitem.h`, `render/src/fixedpageitem.cpp`,
  `render/CMakeLists.txt`, `tests/render.cmake`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_render_pendinglayer` → `error`. Vert : preuve de
tâche, N ≥ 92, `tst_render_pendinglayer .*Passed`, puis `tst_render_fixedpage .*Passed` (inchangé). Banc sans écran
`content_check.ok = True` (aucun différé hors de la vue : comportement du lot 2).

**Interdits.** Ne toucher ni `core/` ni `ui/` (la vue est L10-T4). Ne pas changer la signature de `core::PageCache`.
Aucun test existant modifié. Aucun verrou.

**Terminé quand.** Preuve verte, JSON du banc joint, commit fait.

**Étapes.**

- [ ] **Étape 1 : écrire les tests.** Créer `tests/tst_render_pendinglayer.cpp` :

```cpp
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/pendinglayeritem.h>

#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>

#include <algorithm>

using namespace actcraft;

namespace {

bool near(QColor a, QColor b, int tolerance)
{
    return std::abs(a.red() - b.red()) <= tolerance && std::abs(a.green() - b.green()) <= tolerance &&
           std::abs(a.blue() - b.blue()) <= tolerance;
}

core::Stroke lineAt(double y, core::Argb color = 0xFF000000u)
{
    core::Stroke s;
    s.baseWidth = 8;
    s.color = color;
    for (int i = 0; i <= 20; ++i) {
        s.points.push_back(core::StrokePoint{100.0 + 20.0 * i, y, 0.5f});
    }
    return s;
}

} // namespace

// Lot 10 (DA11) : objets en attente au-dessus du cache. Rendu logiciel hors écran (comme tous les tests).
class TstRenderPendingLayer : public QObject
{
    Q_OBJECT

private:
    struct Fixture
    {
        QQuickWindow window;
        render::FixedPageItem* page = nullptr;
        render::PendingLayerItem* layer = nullptr;
    };

    // `withLayer` faux : la page fixe seule, pour voir ce que SA scène dessine (ce que le cache photographie).
    static void open(Fixture& f, bool withLayer)
    {
        f.window.setColor(Qt::white);
        f.window.resize(640, 480);
        f.page = new render::FixedPageItem;
        f.page->setSize(QSizeF(640, 480));
        f.page->setParentItem(f.window.contentItem());
        if (withLayer) {
            f.layer = new render::PendingLayerItem;
            f.layer->setSize(QSizeF(640, 480));
            f.layer->setParentItem(f.window.contentItem());
            f.layer->setFixedPage(f.page);
        }
        f.page->setPage(core::Page{});
        f.window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&f.window));
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
    }

    static QColor at(Fixture& f, int x, int y) { return f.window.grabWindow().pixelColor(x, y); }

private slots:
    void withoutDeferralNothingWaits()
    {
        Fixture f;
        open(f, false);
        f.page->appendStroke(lineAt(100));
        QCOMPARE(f.page->pendingCount(), 0);
        QCOMPARE(f.page->cachedObjectCount(), std::size_t{1});
        QVERIFY(near(at(f, 300, 100), Qt::black, 60)); // lot 2 : dans la scène tout de suite
    }

    void deferredStrokeIsNotInTheCachedScene()
    {
        Fixture f;
        open(f, false);
        f.page->setDeferredFolding(true);
        QSignalSpy content(f.page, &render::FixedPageItem::contentChanged);
        QSignalSpy pending(f.page, &render::FixedPageItem::pendingChanged);
        f.page->appendStroke(lineAt(100));
        QCOMPARE(f.page->objectCount(), 1);
        QCOMPARE(f.page->pendingCount(), 1);
        QCOMPARE(f.page->cachedObjectCount(), std::size_t{0});
        QCOMPARE(content.count(), 0); // aucun redessin du cache demandé
        QCOMPARE(pending.count(), 1);
        QVERIFY(near(at(f, 300, 100), Qt::white, 8)); // la scène du cache ne l'a pas
        f.page->foldPending();
        QCOMPARE(f.page->pendingCount(), 0);
        QVERIFY(near(at(f, 300, 100), Qt::black, 60)); // versé
    }

    void layerDrawsWhatWaits()
    {
        Fixture f;
        open(f, true);
        f.page->setDeferredFolding(true);
        f.page->appendStroke(lineAt(100));
        f.page->appendStroke(lineAt(200));
        QVERIFY(near(at(f, 300, 100), Qt::black, 60));
        QVERIFY(near(at(f, 300, 200), Qt::black, 60));
        QCOMPARE(f.layer->builtCount(), 2);
    }

    // Même image avant et après le versement : la couche ne dessine plus ce que la scène dessine désormais.
    // Un trait translucide le prouve : dessiné deux fois, il serait plus foncé ; par personne, blanc.
    void foldShowsTheSameImage()
    {
        Fixture f;
        open(f, true);
        f.page->setDeferredFolding(true);
        f.page->appendStroke(lineAt(100, 0x80E53935u));
        const QColor before = at(f, 300, 100);
        QVERIFY(!near(before, Qt::white, 8));
        f.page->foldPending();
        const QColor after = at(f, 300, 100);
        QCOMPARE(f.layer->builtCount(), 0);
        QVERIFY2(near(after, before, 4), qPrintable(after.name() + QStringLiteral(" / ") + before.name()));
    }

    // Annulation d'un trait encore en attente, par le chemin de la pile : l'objet quitte la page du document,
    // puis le cache reçoit remove(id, zone). Ni reconstruction, ni redessin du cache demandé.
    void undoingAPendingStrokeRebuildsNothing()
    {
        Fixture f;
        open(f, true);
        core::Document d;
        f.page->setSource(&d, 0);
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        f.page->setDeferredFolding(true);
        core::PageObject stroke = lineAt(300);
        stroke.common.id = d.allocateId();
        stroke.common.layer = core::Layer::Top;
        const core::ObjectId id = stroke.common.id;
        core::insertAtTopOfLayer(d.page(0), std::move(stroke));
        f.page->insert(id);
        QCOMPARE(f.page->pendingCount(), 1);
        QVERIFY(near(at(f, 300, 300), Qt::black, 60));
        const int rebuilds = f.page->rebuildCount();
        QSignalSpy content(f.page, &render::FixedPageItem::contentChanged);
        d.page(0).objects.pop_back();
        f.page->remove(id, core::RectF{100, 296, 400, 8});
        QCOMPARE(f.page->pendingCount(), 0);
        QCOMPARE(content.count(), 0);
        QVERIFY(near(at(f, 300, 300), Qt::white, 8));
        QCOMPARE(f.page->rebuildCount(), rebuilds);
    }

    void anyOtherChangeFoldsEverything()
    {
        Fixture f;
        open(f, true);
        core::Document d;
        f.page->setSource(&d, 0);
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        f.page->setDeferredFolding(true);
        core::PageObject a = lineAt(100);
        a.common.id = d.allocateId();
        a.common.layer = core::Layer::Top;
        const core::ObjectId aId = a.common.id;
        core::insertAtTopOfLayer(d.page(0), std::move(a));
        f.page->insert(aId);
        QCOMPARE(f.page->pendingCount(), 1);
        QSignalSpy content(f.page, &render::FixedPageItem::contentChanged);
        // Objet inséré en tête de page (couche inférieure) : pas en fin de page, reconstruction complète.
        core::PageObject b = lineAt(200);
        b.common.id = d.allocateId();
        b.common.layer = core::Layer::Bottom;
        const core::ObjectId bId = b.common.id;
        core::insertAtTopOfLayer(d.page(0), std::move(b));
        f.page->insert(bId);
        QCOMPARE(f.page->pendingCount(), 0);
        QCOMPARE(content.count(), 1);
        QCOMPARE(f.page->cachedObjectCount(), std::size_t{2});
    }

    // Un trait en attente réordonné en tête de page (ObjectChanged) n'est plus en queue : tout est reconstruit.
    void pendingStrokeMovedToTheFrontFoldsEverything()
    {
        Fixture f;
        open(f, true);
        core::Document d;
        core::PageObject first = lineAt(100);
        first.common.id = d.allocateId();
        first.common.layer = core::Layer::Top;
        core::insertAtTopOfLayer(d.page(0), std::move(first));
        f.page->setSource(&d, 0);
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        f.page->setDeferredFolding(true);
        core::PageObject moved = lineAt(300);
        moved.common.id = d.allocateId();
        moved.common.layer = core::Layer::Top;
        const core::ObjectId id = moved.common.id;
        core::insertAtTopOfLayer(d.page(0), std::move(moved));
        f.page->insert(id);
        QCOMPARE(f.page->pendingCount(), 1);
        QVERIFY(near(at(f, 300, 300), Qt::black, 60));
        const int rebuilds = f.page->rebuildCount();
        auto& objects = d.page(0).objects;
        std::rotate(objects.begin(), objects.end() - 1, objects.end()); // en tête de page
        f.page->invalidate(id, core::RectF{100, 296, 400, 8});
        QCOMPARE(f.page->pendingCount(), 0);
        QVERIFY(near(at(f, 300, 300), Qt::black, 60)); // dans la scène, plus dans la couche
        QCOMPARE(f.layer->builtCount(), 0);
        QVERIFY(f.page->rebuildCount() > rebuilds);
    }

    void pendingAreAlwaysTheLastObjects()
    {
        Fixture f;
        open(f, true);
        f.page->setDeferredFolding(true);
        for (int i = 0; i < 5; ++i) {
            f.page->appendStroke(lineAt(50.0 + 40 * i));
        }
        const core::Page& page = f.page->page();
        const auto& ids = f.page->pendingIds();
        QCOMPARE(ids.size(), std::size_t{5});
        const std::size_t first = page.objects.size() - ids.size();
        QCOMPARE(first, f.page->cachedObjectCount());
        for (std::size_t k = 0; k < ids.size(); ++k) {
            QCOMPARE(page.objects[first + k].common.id, ids[k]);
        }
    }

    void turningDeferralOffFoldsThePending()
    {
        Fixture f;
        open(f, true);
        f.page->setDeferredFolding(true);
        f.page->appendStroke(lineAt(100));
        f.page->setDeferredFolding(false);
        QCOMPARE(f.page->pendingCount(), 0);
        QVERIFY(near(at(f, 300, 100), Qt::black, 60));
    }

    void gridDrawnAboveIsNeverDeferred()
    {
        Fixture f;
        open(f, true);
        core::Page page;
        page.grid.drawnAbove = true;
        f.page->setPage(std::move(page));
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        f.page->setDeferredFolding(true);
        f.page->appendStroke(lineAt(100));
        QCOMPARE(f.page->pendingCount(), 0); // il doit passer sous la grille : dans la scène
    }
};

QTEST_MAIN(TstRenderPendingLayer)
#include "tst_render_pendinglayer.moc"
```

  `undoingAPendingStrokeRebuildsNothing` et `anyOtherChangeFoldsEverything` passent par une page **source**
  (document), seule voie où un retrait ou une insertion hors fin de page arrive par l'interface de cache (la page
  interne n'accepte que `appendStroke`). Dans `tests/render.cmake` :

```cmake
actcraft_add_test(tst_render_pendinglayer
    SOURCES tst_render_pendinglayer.cpp
    LIBS ActCraft::render Qt6::Quick)
```

- [ ] **Étape 2 : voir le rouge.** Commande ciblée, motif `tst_render_pendinglayer` → `error`
  (`pendinglayeritem.h` absent).

- [ ] **Étape 3 : l'interface de la page fixe.** Dans `render/include/actcraft/render/fixedpageitem.h` :
  `Q_PROPERTY(int pendingCount READ pendingCount NOTIFY pendingChanged)` ; en public, après `invalidate` :

```cpp
    // Lot 10 (DA11, cache incrémental). Différé (posé par la vue quand une couche d'attente est branchée et le
    // cache actif) : un objet inséré en fin de page, sans image, page prête et sans grille dessinée au-dessus,
    // n'entre PAS dans la scène de l'élément (celle que le cache photographie) : il attend, dessiné par la
    // couche d'attente (PendingLayerItem), jusqu'à foldPending(). Retiré (remove) ou modifié (invalidate)
    // pendant qu'il attend : seule la couche d'attente change. Tout autre changement reconstruit tout et
    // verse ce qui attend. Sans différé (défaut) : comportement du lot 2. Repasser à faux verse tout.
    void setDeferredFolding(bool on);
    bool deferredFolding() const noexcept;
    // Verse les objets en attente dans la scène : à appeler dans le même tour de boucle que la demande de
    // redessin du cache (la vue le fait dans requestCacheRefresh) ; la même image montre alors le cache qui
    // les contient et la couche d'attente vide.
    void foldPending();
    // Objets en attente : toujours les derniers de la page, dans l'ordre de la page.
    int pendingCount() const noexcept;
    const std::vector<core::ObjectId>& pendingIds() const noexcept;
    // Augmente quand un objet en attente change sans changer de place (invalidate) : la couche refait tout.
    std::uint64_t pendingRevision() const noexcept;
    // Objets de la page que la scène de l'élément contient : les cachedObjectCount() premiers.
    std::size_t cachedObjectCount() const noexcept;
    bool forceRasterized() const noexcept; // crochet de test lu par la couche d'attente (même chemin RHI)
```

  signal `void pendingChanged();` ; membres privés :

```cpp
    bool m_deferred = false;
    std::vector<core::ObjectId> m_pendingIds; // fil principal ; lu par les synchronisations
    std::uint64_t m_pendingRevision = 0;
```

- [ ] **Étape 4 : l'implémentation.** Dans `render/src/fixedpageitem.cpp` (inclure `<algorithm>`) :

```cpp
void FixedPageItem::setDeferredFolding(bool on)
{
    if (!on) {
        foldPending();
    }
    m_deferred = on;
}

bool FixedPageItem::deferredFolding() const noexcept
{
    return m_deferred;
}

void FixedPageItem::foldPending()
{
    if (m_pendingIds.empty()) {
        return;
    }
    m_pendingIds.clear(); // la prochaine synchronisation ajoute leurs nœuds à la scène (m_builtCount suit)
    emit pendingChanged();
    update();
}

int FixedPageItem::pendingCount() const noexcept
{
    return static_cast<int>(m_pendingIds.size());
}

const std::vector<core::ObjectId>& FixedPageItem::pendingIds() const noexcept
{
    return m_pendingIds;
}

std::uint64_t FixedPageItem::pendingRevision() const noexcept
{
    return m_pendingRevision;
}

std::size_t FixedPageItem::cachedObjectCount() const noexcept
{
    const std::size_t count = page().objects.size();
    return count >= m_pendingIds.size() ? count - m_pendingIds.size() : 0;
}

bool FixedPageItem::forceRasterized() const noexcept
{
    return m_forceRasterized;
}
```

  `insert` devient :

```cpp
void FixedPageItem::insert(core::ObjectId id)
{
    const core::Page& shown = page();
    const std::size_t index = core::indexOf(shown, id);
    if (index == shown.objects.size()) {
        return;
    }
    const core::PageObject& object = shown.objects[index];
    const bool imageChanged = usesImages(object);
    const bool atEnd = index + 1 == shown.objects.size();
    if (atEnd && m_deferred && m_ready && !imageChanged && !shown.grid.drawnAbove) {
        // Lot 10 : il attend au-dessus du cache ; ni nœud dans la scène, ni redessin du cache. Limite de DA11
        // (écart 5, H10-8) : hors fin de page, image ou grille au-dessus → redessin immédiat ci-dessous.
        m_pendingIds.push_back(id);
        emit objectCountChanged();
        if (std::get_if<core::Stroke>(&object)) {
            emit strokeAppended();
        }
        emit pendingChanged();
        return;
    }
    if (atEnd && index >= m_builtCount && !imageChanged && m_pendingIds.empty()) {
        // Fin de page (le stylo, hors différé) : le prochain updatePaintNode n'ajoute que ce nœud.
        emit objectCountChanged();
        if (std::get_if<core::Stroke>(&object)) {
            emit strokeAppended();
        }
        emit contentChanged();
        update();
        return;
    }
    // image ajoutée pendant un décodage : il ne la couvre pas
    restart(imageChanged && !m_ready ? Decode::Force : Decode::IfUncovered, Scope::SamePage);
}
```

  `remove` et `invalidate` :

```cpp
void FixedPageItem::remove(core::ObjectId id, const core::RectF&)
{
    const auto it = std::find(m_pendingIds.begin(), m_pendingIds.end(), id);
    if (it != m_pendingIds.end()) {
        // Retiré avant d'être versé (annulation d'un trait récent) : le cache ne l'a jamais contenu.
        m_pendingIds.erase(it);
        emit objectCountChanged();
        emit pendingChanged();
        return;
    }
    restart(Decode::IfUncovered, Scope::SamePage);
}

void FixedPageItem::invalidate(core::ObjectId id, const core::CacheArea&)
{
    if (id != 0 && std::find(m_pendingIds.begin(), m_pendingIds.end(), id) != m_pendingIds.end()) {
        const core::Page& shown = page();
        const std::size_t at = core::indexOf(shown, id);
        const bool image = at < shown.objects.size() && usesImages(shown.objects[at]);
        // Image (la couche d'attente n'a pas de magasin d'images) ou objet qui n'est plus en queue : tout reconstruire.
        if (pendingAreTheLastObjects() && !image) {
            ++m_pendingRevision; // modifié en place pendant qu'il attend : la couche le refait
            emit pendingChanged();
            return;
        }
        // Réordonné pendant qu'il attend (annoncé en ObjectChanged, undo.h n'a pas de type « réordonné ») : ce
        // qui attend n'est plus la queue de la page ; reconstruction complète, qui verse tout.
        restart(Decode::IfUncovered, Scope::SamePage);
        return;
    }
    const core::Page& shown = page();
    const std::size_t index = core::indexOf(shown, id);
    const bool imageChanged = index < shown.objects.size() && usesImages(shown.objects[index]);
    restart(imageChanged && !m_ready ? Decode::Force : Decode::IfUncovered, Scope::SamePage);
}
```

  Fonction privée (déclarée dans l'en-tête, `bool pendingAreTheLastObjects() const noexcept;`) :

```cpp
// Invariant de la couche d'attente : les objets en attente sont, dans l'ordre, les derniers de la page.
bool FixedPageItem::pendingAreTheLastObjects() const noexcept
{
    const core::Page& shown = page();
    if (m_pendingIds.size() > shown.objects.size()) {
        return false;
    }
    const std::size_t first = shown.objects.size() - m_pendingIds.size();
    for (std::size_t k = 0; k < m_pendingIds.size(); ++k) {
        if (shown.objects[first + k].common.id != m_pendingIds[k]) {
            return false;
        }
    }
    return true;
}
```

  Dans `restart`, en tête : `const bool hadPending = !m_pendingIds.empty(); m_pendingIds.clear();` et, après
  `emit objectCountChanged();` : `if (hadPending) { emit pendingChanged(); }` (la reconstruction met tout dans la
  scène et `contentChanged` fait redessiner le cache). Dans `updatePaintNode`, la boucle d'objets s'arrête à
  `const std::size_t limit = cachedObjectCount();` au lieu de `shown.objects.size()`, et `m_builtCount = limit;`.

- [ ] **Étape 5 : la couche d'attente.** Créer `render/include/actcraft/render/pendinglayeritem.h` :

```cpp
#pragma once

#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/objectrenderer.h>
#include <actcraft/render/paintedrenderer.h>

#include <QPointer>
#include <QQuickItem>

#include <atomic>
#include <cstdint>
#include <utility>
#include <vector>

namespace actcraft::render {

// Lot 10 (DA11) : dessine, au-dessus du cache, les objets de la page fixe qui attendent leur versement
// (FixedPageItem::pendingIds), avec les nœuds mêmes de la page fixe (createObjectNode). Sœur du cache dans la
// scène (jamais enfant de la page fixe : hideSource du cache la cacherait). Aucune image n'attend
// (FixedPageItem::insert les fait reconstruire) : pas de magasin d'images ici.
class PendingLayerItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(actcraft::render::FixedPageItem* fixedPage READ fixedPage WRITE setFixedPage NOTIFY
                   fixedPageChanged)

public:
    explicit PendingLayerItem(QQuickItem* parent = nullptr);
    FixedPageItem* fixedPage() const noexcept;
    void setFixedPage(FixedPageItem* page);
    int builtCount() const noexcept; // crochet de test : nœuds posés à la dernière synchronisation

signals:
    void fixedPageChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* old, UpdatePaintNodeData* data) override;

private:
    QPointer<FixedPageItem> m_page;
    QMetaObject::Connection m_pendingConnection;
    QMetaObject::Connection m_contentConnection;
    RendererRegistry m_registry;
    PaintedRenderer m_painted;
    std::vector<std::pair<core::ObjectId, QSGNode*>> m_nodes; // fil de rendu ; nœud nul : rien à dessiner
    std::uint64_t m_builtRevision = 0;
    std::atomic<int> m_builtCount{0};
};

} // namespace actcraft::render
```

  Créer `render/src/pendinglayeritem.cpp` :

```cpp
#include "objectnodes.h"

#include <actcraft/render/pendinglayeritem.h>

#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSGTransformNode>

namespace actcraft::render {

PendingLayerItem::PendingLayerItem(QQuickItem* parent)
    : QQuickItem(parent), m_registry(RendererRegistry::withDefaults())
{
    setFlag(ItemHasContents);
}

FixedPageItem* PendingLayerItem::fixedPage() const noexcept
{
    return m_page.data();
}

void PendingLayerItem::setFixedPage(FixedPageItem* page)
{
    if (page == m_page) {
        return;
    }
    disconnect(m_pendingConnection);
    disconnect(m_contentConnection);
    m_page = page;
    if (m_page) {
        m_pendingConnection = connect(m_page, &FixedPageItem::pendingChanged, this, [this] { update(); });
        m_contentConnection = connect(m_page, &FixedPageItem::contentChanged, this, [this] { update(); });
    }
    update();
    emit fixedPageChanged();
}

int PendingLayerItem::builtCount() const noexcept
{
    return m_builtCount.load();
}

QSGNode* PendingLayerItem::updatePaintNode(QSGNode* old, UpdatePaintNodeData*)
{
    const bool software = window()->rendererInterface()->graphicsApi() == QSGRendererInterface::Software;
    // Logiciel : racine de transformation (identité), comme la page fixe (piège du moteur logiciel 6.12).
    QSGNode* root = old ? old : (software ? new QSGTransformNode : new QSGNode);
    static const std::vector<core::ObjectId> none;
    const std::vector<core::ObjectId>& wanted = m_page ? m_page->pendingIds() : none;
    const std::uint64_t revision = m_page ? m_page->pendingRevision() : 0;
    bool keep = revision == m_builtRevision && m_nodes.size() <= wanted.size();
    for (std::size_t i = 0; keep && i < m_nodes.size(); ++i) {
        keep = m_nodes[i].first == wanted[i];
    }
    if (!keep) {
        while (QSGNode* child = root->firstChild()) {
            root->removeChildNode(child);
            delete child;
        }
        m_nodes.clear();
        m_builtRevision = revision;
    }
    if (m_page && m_nodes.size() < wanted.size()) {
        const core::Page& page = m_page->page();
        RenderContext context;
        context.window = window();
        context.software = software;
        context.images = nullptr;
        context.rasterScale = m_page->rasterScale();
        RenderContext painted = context;
        painted.software = software && !m_page->forceRasterized();
        const std::size_t first = page.objects.size() - wanted.size();
        for (std::size_t k = m_nodes.size(); k < wanted.size(); ++k) {
            const std::size_t index = first + k;
            const ObjectNode made =
                createObjectNode(page.objects[index], index, m_registry, m_painted, context, painted);
            m_nodes.emplace_back(wanted[k], made.root);
            if (made.root) {
                root->appendChildNode(made.root);
            }
        }
    }
    int built = 0;
    for (const auto& entry : m_nodes) {
        built += entry.second ? 1 : 0;
    }
    m_builtCount.store(built);
    return root;
}

} // namespace actcraft::render
```

  Dans `render/CMakeLists.txt`, après `src/objectnodes.cpp` : `include/actcraft/render/pendinglayeritem.h` et
  `src/pendinglayeritem.cpp`.

- [ ] **Étape 6 : vert.** `format.ps1`, preuve de tâche (`tst_render_pendinglayer`, puis `tst_render_fixedpage`),
  banc sans écran. Si `foldShowsTheSameImage` échoue d'un ou deux niveaux à l'arrondi du rendu logiciel, élargir
  la tolérance à 6 au plus et l'écrire au rapport ; au-delà, c'est un défaut de l'invariant.

- [ ] **Étape 7 : commit.**
  ```powershell
  git -C <wt> add render/include/actcraft/render/fixedpageitem.h render/src/fixedpageitem.cpp render/include/actcraft/render/pendinglayeritem.h render/src/pendinglayeritem.cpp render/CMakeLists.txt tests/render.cmake tests/tst_render_pendinglayer.cpp
  git -C <wt> commit -m "render : objets en attente hors de la scène du cache, couche d'attente au-dessus (L10-T3)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l10-t4"></a>
### Tâche L10-T4 — Vue : versement différé ; couche d'attente branchée ; scénario `cache-differe`

**Objectif.** La politique du cache est dans la vue (`PageView`), qui fait partie de l'implémentation du cache
depuis le lot 2 (`ShaderEffectSource` de `PageArea.qml`, `requestCacheRefresh`, `notifyCacheUpdated`). Brancher la
couche d'attente ; mettre la page fixe en différé quand **le cache est actif et une couche d'attente branchée** ; ne
plus redessiner le cache à chaque trait : verser après `kCacheFoldIdleMs` sans entrée (minuterie précise, relancée
par chaque entrée de pointeur et chaque objet mis en attente, repoussée tant qu'un trait est en cours), dès
`kCacheFoldMaxPending` objets en attente, et à tout redessin demandé (zoom, page, reconstruction) : **tout
redessin verse** (`foldPending` dans `requestCacheRefresh`, juste avant `cacheRefreshRequested`). `cacheReady`
garde son sens (ce qui est affiché correspond à la page et au zoom) : il reste vrai pendant qu'un objet attend.
Le banc relève le plus grand nombre d'objets en attente (`cache.pending_max`) et vérifie que le trait de
démonstration est toujours là après son versement (`stroke-folded`).

**Appelants inchangés.** `DocumentController`, `ControllerSink`, `FixedPageSink`, `PenTool`, `applyToCache` ne
changent pas. `ui/src/uiforeigntypes.h` déclare seulement le type QML `PendingLayer` (enregistrement, pas un
appel du cache).

**Fichiers.**
- Créer : `bench/scenarios/cache-differe.json`
- Modifier : `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`, `ui/src/uiforeigntypes.h`,
  `ui/qml/PageArea.qml`, `tests/tst_pageview.cpp`, `tests/tst_ui_drawing.cpp`, `bench/include/actcraft/bench/result.h`,
  `bench/src/result.cpp`, `bench/src/runner.cpp`, `tests/tst_bench_verdict.cpp`, `tests/tst_bench_result.cpp`,
  `tests/bench.cmake`

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_pageview|tst_ui_drawing` → `error`
(`setPendingLayer` inconnu). Vert : preuve de tâche, N ≥ 94, `tst_pageview`, `tst_ui_drawing`,
`tst_captures_cache_differe`, `tst_bench_verdict` Passed. Preuve visuelle hors écran : `captures.ps1 -Scenario
cache-differe -Rendu logiciel -HorsEcran` → `LU fixedPage.pendingCount=3 OK` puis `ATTENTE fixedPage.pendingCount=0
OK`, `PIXEL … OK`, `BORD trait-en-attente OK`, `CAPTURES OK`. Banc sans écran : `stroke … cache={"refreshes":0,…}`
(aucun redessin pendant les traits du scénario `stroke`, dont les pauses font 60 ms en court), `pauses`
`pending_max ≥ 1`, `content_check.ok = True` avec `stroke-folded`.

**Interdits.** Aucun changement de `DocumentController`, `penadapters.*`, `tools/`, `core/`. `oneCacheRenderPerStroke`
et les autres tests du cache de `tst_pageview` restent tels quels (sans couche d'attente : comportement du lot 2).
Deux tests existants changent, avec leur raison : `benchRunReadsItsTargetsAt1366x768` (`tst_bench_verdict`) compte
cinq contrôles de contenu au lieu de quatre (`stroke-folded` ajouté par le plan) ; `cacheRefreshesAreRecorded`
(`tst_bench_result`, L10-T1) exige au moins 2 redessins dans `pauses` au lieu de 3 (versement différé : un par
reprise k ≥ 1, la reprise k = 0 relançant la minuterie), le commentaire du test le dit.

**Terminé quand.** Preuve verte, captures décrites, JSON du banc joint, commit fait.

**Étapes.**

- [ ] **Étape 1 : tests de la politique.** Dans `tests/tst_pageview.cpp`, ajouter `#include
  <actcraft/render/pendinglayeritem.h>` et `#include <actcraft/ui/viewtransform.h>`, un montage avec couche
  d'attente à côté de `openCached`, et les cas :

```cpp
struct PendingFixture
{
    CachedFixture cached;
    actcraft::render::PendingLayerItem* layer = nullptr;
};

bool openPending(PendingFixture& f, QPointingDevice* stylus)
{
    if (!openCached(f.cached, stylus)) {
        return false;
    }
    f.layer = new actcraft::render::PendingLayerItem;
    f.layer->setSize(f.cached.fixed->size());
    f.layer->setParentItem(f.cached.fixed->parentItem());
    f.layer->setFixedPage(f.cached.fixed);
    f.cached.view->setPendingLayer(f.layer);
    return f.cached.fixed->deferredFolding();
}
```

```cpp
    // Lot 10 (DA11) : avec une couche d'attente, un trait n'est versé au cache qu'après l'inactivité.
    void strokeWaitsAboveTheCacheUntilIdle()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        QSignalSpy requested(f.cached.view, &PageView::cacheRefreshRequested);
        cachedStroke(f.cached, QPointF(300, 200), QPointF(500, 260));
        QTRY_COMPARE(f.cached.fixed->pendingCount(), 1);
        QCOMPARE(requested.count(), 0);
        QVERIFY(f.cached.view->cacheReady()); // l'affichage (cache + attente) est juste
        QVERIFY(f.cached.view->foldTimerActive());
        QTRY_COMPARE_WITH_TIMEOUT(requested.count(), 1, kCacheFoldIdleMs + 2000);
        QCOMPARE(f.cached.fixed->pendingCount(), 0); // versé par la demande elle-même
    }

    void inputPostponesTheFold()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        QSignalSpy requested(f.cached.view, &PageView::cacheRefreshRequested);
        cachedStroke(f.cached, QPointF(300, 200), QPointF(500, 260));
        QTest::qWait(kCacheFoldIdleMs * 6 / 10);
        cachedStroke(f.cached, QPointF(300, 300), QPointF(500, 360));
        QTest::qWait(kCacheFoldIdleMs * 6 / 10);
        QCOMPARE(requested.count(), 0); // 1,2 délai depuis le premier trait, 0,6 depuis le second
        QTRY_COMPARE_WITH_TIMEOUT(requested.count(), 1, kCacheFoldIdleMs + 2000);
        QCOMPARE(f.cached.fixed->pendingCount(), 0);
    }

    // Molette et clavier repoussent aussi le versement (une entrée, quelle qu'elle soit).
    void wheelPostponesTheFold()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        QSignalSpy requested(f.cached.view, &PageView::cacheRefreshRequested);
        f.cached.fixed->appendStroke(line(100));
        QVERIFY(f.cached.view->foldTimerActive());
        QTest::qWait(kCacheFoldIdleMs * 6 / 10);
        QWheelEvent event(QPointF(500, 300), f.cached.window.mapToGlobal(QPointF(500, 300)), QPoint(),
                          QPoint(0, -120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(&f.cached.window, &event);
        QTest::qWait(kCacheFoldIdleMs * 6 / 10);
        QCOMPARE(requested.count(), 0); // 1,2 délai depuis le trait, 0,6 depuis la molette
        QTest::keyClick(&f.cached.window, Qt::Key_Shift);
        QTest::qWait(kCacheFoldIdleMs * 6 / 10);
        QCOMPARE(requested.count(), 0); // 0,6 délai depuis la touche
        QTRY_COMPARE_WITH_TIMEOUT(requested.count(), 1, kCacheFoldIdleMs + 2000);
        QCOMPARE(f.cached.fixed->pendingCount(), 0);
    }

    // Un survol (stylet tenu près du tableau, souris qui passe) ne repousse pas le versement.
    void hoverDoesNotPostponeTheFold()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        QSignalSpy requested(f.cached.view, &PageView::cacheRefreshRequested);
        f.cached.fixed->appendStroke(line(100));
        QElapsedTimer clock;
        clock.start();
        while (requested.count() == 0 && clock.elapsed() < kCacheFoldIdleMs + 2000) {
            const QPointF at(400.0 + static_cast<double>(clock.elapsed() % 50), 300.0);
            QTabletEvent hover(QEvent::TabletMove, f.cached.stylus, at, f.cached.window.mapToGlobal(at), 0.0, 0, 0,
                               0, 0, 0, Qt::NoModifier, Qt::NoButton, Qt::NoButton);
            QCoreApplication::sendEvent(&f.cached.window, &hover);
            QMouseEvent move(QEvent::MouseMove, at, at, f.cached.window.mapToGlobal(at), Qt::NoButton,
                             Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(&f.cached.window, &move);
            QTest::qWait(20);
        }
        QCOMPARE(requested.count(), 1);                    // versé malgré les survols
        QVERIFY(clock.elapsed() < kCacheFoldIdleMs + 500); // et à l'heure, pas repoussé
        QCOMPARE(f.cached.fixed->pendingCount(), 0);
    }

    void zoomFoldsThePending()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        cachedStroke(f.cached, QPointF(300, 200), QPointF(500, 260));
        QTRY_COMPARE(f.cached.fixed->pendingCount(), 1);
        QSignalSpy requested(f.cached.view, &PageView::cacheRefreshRequested);
        f.cached.view->setZoom(2.0);
        QCOMPARE(requested.count(), 1);
        QCOMPARE(f.cached.fixed->pendingCount(), 0);
        QVERIFY(!f.cached.view->foldTimerActive());
    }

    void tooManyPendingFoldAtOnce()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        QSignalSpy requested(f.cached.view, &PageView::cacheRefreshRequested);
        for (int i = 0; i < kCacheFoldMaxPending - 1; ++i) {
            f.cached.fixed->appendStroke(line(10.0 + i)); // page interne : insert, donc attente
        }
        QCOMPARE(requested.count(), 0);
        QCOMPARE(f.cached.fixed->pendingCount(), kCacheFoldMaxPending - 1);
        f.cached.fixed->appendStroke(line(400));
        QCOMPARE(requested.count(), 1);
        QCOMPARE(f.cached.fixed->pendingCount(), 0);
    }

    void withoutCacheNothingWaits()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        f.cached.view->setCacheEnabled(false);
        QVERIFY(!f.cached.fixed->deferredFolding());
        f.cached.fixed->appendStroke(line(100));
        QCOMPARE(f.cached.fixed->pendingCount(), 0);
        f.cached.view->setCacheEnabled(true);
        QVERIFY(f.cached.fixed->deferredFolding());
    }
```

  `line(y)` : trait horizontal de 21 points de (100, y) à (500, y), largeur 4 (fonction libre à ajouter dans
  l'espace anonyme du fichier si aucune n'existe). `cachedStroke` est l'aide existante du fichier.

- [ ] **Étape 2 : tests de bout en bout (Main.qml).** Dans `tests/tst_ui_drawing.cpp` (inclure
  `<actcraft/ui/documentcontroller.h>` et `<actcraft/ui/viewtransform.h>`) :

```cpp
    // Lot 10 : le trait reste visible à chaque image entre le relâché, son attente et son versement ; un seul
    // redessin du cache, après l'inactivité.
    void deferredStrokeStaysVisibleThroughTheFold()
    {
        App app;
        QVERIFY(start(app, true));
        const QPointF probe(350, 670);
        QSignalSpy requested(app.view, &ui::PageView::cacheRefreshRequested);
        post(app, QEvent::TabletPress, QPointF(300, 670), 0.7, Qt::LeftButton, Qt::LeftButton);
        for (int i = 1; i <= 100; ++i) {
            post(app, QEvent::TabletMove, QPointF(300 + 2.0 * i, 670), 0.7, Qt::NoButton, Qt::LeftButton);
        }
        post(app, QEvent::TabletRelease, QPointF(500, 670), 0.0, Qt::LeftButton, Qt::NoButton);
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->pendingCount(), 1, 3000);
        QCOMPARE(requested.count(), 0);
        QSignalSpy swapped(app.window, &QQuickWindow::frameSwapped);
        QElapsedTimer clock;
        clock.start();
        int checked = 0;
        int checkedAfterRequest = 0; // images contrôlées entre la demande de redessin et le cache prêt
        const auto checkFrame = [&] {
            swapped.clear();
            ++checked;
            if (requested.count() > 0) {
                ++checkedAfterRequest;
            }
            QVERIFY2(colorNear(pixelAtPage(app, probe), QColor(Qt::black), 60),
                     "trait absent entre le relâché et le versement achevé");
        };
        while (!(app.fixed->pendingCount() == 0 && app.view->cacheReady()) &&
               clock.elapsed() < ui::kCacheFoldIdleMs + 5000) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            if (!swapped.isEmpty()) {
                checkFrame();
            }
        }
        if (!swapped.isEmpty()) {
            checkFrame(); // image échangée dans le même tour que la fin du versement
        }
        QCOMPARE(app.fixed->pendingCount(), 0);
        QVERIFY(app.view->cacheReady());
        QVERIFY2(colorNear(pixelAtPage(app, probe), QColor(Qt::black), 60), "trait absent après le versement");
        QCOMPARE(requested.count(), 1);
        QVERIFY2(checked >= 2, qPrintable(QStringLiteral("images contrôlées : %1").arg(checked)));
        QVERIFY2(checkedAfterRequest >= 1, "aucune image contrôlée pendant le versement");
    }

    void undoOfAPendingStrokeLeavesTheCacheAlone()
    {
        App app;
        QVERIFY(start(app, true));
        auto* controller = app.window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        QVERIFY(controller);
        const QPointF probe(350, 670);
        post(app, QEvent::TabletPress, QPointF(300, 670), 0.7, Qt::LeftButton, Qt::LeftButton);
        for (int i = 1; i <= 100; ++i) {
            post(app, QEvent::TabletMove, QPointF(300 + 2.0 * i, 670), 0.7, Qt::NoButton, Qt::LeftButton);
        }
        post(app, QEvent::TabletRelease, QPointF(500, 670), 0.0, Qt::LeftButton, Qt::NoButton);
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->pendingCount(), 1, 3000);
        QSignalSpy requested(app.view, &ui::PageView::cacheRefreshRequested);
        const int rebuilds = app.fixed->rebuildCount();
        controller->undo();
        QCOMPARE(app.fixed->pendingCount(), 0);
        QCOMPARE(requested.count(), 0);
        QTest::qWait(50);
        QCOMPARE(app.fixed->rebuildCount(), rebuilds);
        QVERIFY2(!colorNear(pixelAtPage(app, probe), QColor(Qt::black), 60), "trait annulé encore visible");
    }
```

  La page du banc (300 objets) est sous le trait : le contrôle « annulé » vérifie seulement qu'on ne lit plus le
  noir du trait au point sondé (bande de démonstration y = 670, libre de témoins).

- [ ] **Étape 3 : voir le rouge.** Commande ciblée, motif `tst_pageview|tst_ui_drawing`.

- [ ] **Étape 4 : la vue.** Dans `ui/include/actcraft/ui/pageview.h` : inclure
  `<actcraft/render/pendinglayeritem.h>` ; propriété

```cpp
    // Lot 10 : couche d'attente au-dessus du cache. Branchée et cache actif : la page fixe est en différé.
    Q_PROPERTY(actcraft::render::PendingLayerItem* pendingLayer READ pendingLayer WRITE setPendingLayer NOTIFY
                   pendingLayerChanged)
```

  méthodes publiques `render::PendingLayerItem* pendingLayer() const noexcept;`, `void
  setPendingLayer(render::PendingLayerItem* layer);`, `bool foldTimerActive() const noexcept; // crochet de test` ;
  signal `void pendingLayerChanged();` ; privés : `void updateDeferral();`, `void onPendingChanged();`,
  `QPointer<render::PendingLayerItem> m_pendingLayer;` (ou pointeur nu comme `m_fixedPage`, même règle de durée de
  vie), `QTimer m_foldTimer;`, `QMetaObject::Connection m_pendingConnection;`.

  Dans `ui/src/pageview.cpp`, constructeur :

```cpp
    m_foldTimer.setSingleShot(true);
    m_foldTimer.setTimerType(Qt::PreciseTimer); // la reprise de « pauses » vise la fin du délai à 8 ms près
    m_foldTimer.setInterval(kCacheFoldIdleMs);
    connect(&m_foldTimer, &QTimer::timeout, this, [this] {
        if (m_pen && m_pen->isDrawing()) {
            m_foldTimer.start(); // jamais pendant un trait
            return;
        }
        if (m_fixedPage && m_fixedPage->pendingCount() > 0) {
            requestCacheRefresh(); // verse et redessine, hors de l'écriture
        }
    });
```

  puis :

```cpp
render::PendingLayerItem* PageView::pendingLayer() const noexcept
{
    return m_pendingLayer;
}

void PageView::setPendingLayer(render::PendingLayerItem* layer)
{
    if (layer == m_pendingLayer) {
        return;
    }
    m_pendingLayer = layer;
    updateDeferral();
    emit pendingLayerChanged();
}

bool PageView::foldTimerActive() const noexcept
{
    return m_foldTimer.isActive();
}

void PageView::updateDeferral()
{
    if (m_fixedPage) {
        const bool deferred = m_cacheEnabled && m_pendingLayer;
        if (!deferred && m_fixedPage->pendingCount() > 0) {
            requestCacheRefresh(); // ce qui attendait passe au cache avant que la couche ne serve plus
        }
        m_fixedPage->setDeferredFolding(deferred);
    }
    if (!m_cacheEnabled || !m_pendingLayer) {
        m_foldTimer.stop();
    }
}

void PageView::onPendingChanged()
{
    if (!m_fixedPage || m_fixedPage->pendingCount() == 0) {
        m_foldTimer.stop();
        return;
    }
    if (m_fixedPage->pendingCount() >= kCacheFoldMaxPending) {
        requestCacheRefresh(); // plafond : versé tout de suite (H10-3)
        return;
    }
    m_foldTimer.start();
}
```

  Dans `requestCacheRefresh`, en tête : `m_foldTimer.stop(); if (m_fixedPage) { m_fixedPage->foldPending(); }`
  (avant le branchement sur `m_cacheEnabled`, pour que tout redessin verse). Dans `setFixedPage`, après les deux
  connexions existantes : `m_pendingConnection = connect(m_fixedPage, &render::FixedPageItem::pendingChanged, this,
  &PageView::onPendingChanged);` (et `disconnect(m_pendingConnection);` avec les autres), puis `updateDeferral();`.
  Dans `setCacheEnabled`, après `m_cacheEnabled = enabled;` : `updateDeferral();`. **Toute entrée qui agit repousse le versement ; un survol, non** (stylet
  tenu près du tableau, souris sur une barre : s'il repoussait, le versement ne partirait qu'au plafond, pendant
  l'écriture). Une fonction privée `void postponeFold() { if (m_foldTimer.isActive()) { m_foldTimer.start(); } }`
  est appelée, quand `postponesFold(event)` est vrai, dans `handlePointer`, dans `wheelEvent` et dans
  `PageView::eventFilter` (le filtre déjà posé sur la fenêtre, `itemChange`), sans rien accepter ni filtrer de plus
  qu'aujourd'hui. Dans l'espace anonyme de `pageview.cpp` (inclure `<QMouseEvent>`, `<QTabletEvent>`) :

```cpp
// Lot 10 : une entrée qui agit (appui, relâché, mouvement en contact, molette, appui de touche) repousse le
// versement ; un survol non (stylet tenu près du tableau, souris sur une barre), sinon il ne partirait qu'au plafond.
bool postponesFold(const QEvent& event)
{
    switch (event.type()) {
    case QEvent::TabletPress:
    case QEvent::TabletRelease:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::Wheel:
    case QEvent::KeyPress:
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
        return true;
    case QEvent::TabletMove: {
        const auto& tablet = static_cast<const QTabletEvent&>(event);
        return tablet.buttons() != Qt::NoButton || tablet.pressure() > 0;
    }
    case QEvent::MouseMove:
        return static_cast<const QMouseEvent&>(event).buttons() != Qt::NoButton;
    default:
        return false;
    }
}
```

  Le redessin du lot 2 sur `contentChanged` reste : en différé, un trait n'émet plus `contentChanged` (il émet
  `pendingChanged`), une reconstruction l'émet toujours.

- [ ] **Étape 5 : QML.** Dans `ui/src/uiforeigntypes.h`, sur le modèle de `FixedPageForeign` :

```cpp
struct PendingLayerForeign
{
    Q_GADGET
    QML_FOREIGN(actcraft::render::PendingLayerItem)
    QML_NAMED_ELEMENT(PendingLayer)
};
```

  (inclure `<actcraft/render/pendinglayeritem.h>`). Dans `ui/qml/PageArea.qml` : `pendingLayer: pendingLayerItem`
  dans `PageView`, et entre le `ShaderEffectSource` et `InkLayer` :

```qml
            // Lot 10 : objets ajoutés depuis le dernier redessin du cache, dessinés au-dessus de lui.
            PendingLayer {
                id: pendingLayerItem
                objectName: "pendingLayer"
                anchors.fill: parent
                fixedPage: fixedPageItem
            }
```

- [ ] **Étape 6 : le banc.** Dans `bench/include/actcraft/bench/result.h`, `ScenarioResult` : `int pendingMax = 0;
  // L10-T4 : plus grand FixedPageItem::pendingCount pendant le scénario`. Dans `bench/src/runner.cpp`, `play` :
  connexion `render::FixedPageItem::pendingChanged` → `s.pendingMax = std::max(s.pendingMax,
  m_fixed->pendingCount());` (déconnectée avec les autres). Dans `bench/src/result.cpp` : l'objet `cache` du
  scénario gagne `{"pending_max", s.pendingMax}`. Contrôle `stroke-folded` dans la démonstration, juste **après** la capture
  `const QImage endStroke = m_window->grabWindow();` (qui garde ainsi une image de l'état en attente : le trait de
  démonstration y est dessiné par la couche d'attente) :

```cpp
    // Lot 10 : le trait de démonstration attend au-dessus du cache, puis y est versé ; il doit y être encore.
    must("stroke not folded", [this] { return m_fixed->pendingCount() == 0 && m_view->cacheReady(); },
         ui::kCacheFoldIdleMs + 5000);
    waitMs(100);
    const QImage folded = m_window->grabWindow();
```

  et, dans `result.contentChecks`, après `ink-mid-stroke` : `checkColor(QStringLiteral("stroke-folded"), folded,
  inkPixel, QColor(Qt::black), kInkTolerance),`. `must` prend une condition et un délai comme ses voisins (relire
  sa signature dans le fichier et s'y conformer). Dans `tests/tst_bench_verdict.cpp`,
  `benchRunReadsItsTargetsAt1366x768` : `std::size_t{5}` au lieu de `{4}` (commentaire : `// L10-T4 :
  stroke-folded`). Dans `tests/tst_bench_result.cpp`, `cacheRefreshesAreRecorded` : les trois minimums passent de 3
  à 2, commentaire « L10-T4 : versement différé, un redessin par reprise k ≥ 1 (4 traits en court) ». Et un
  contrôle sans minutage :

```cpp
    // L10-T4 : aucun redessin du cache pendant « stroke » (pauses de 60 ms en court, sous le délai
    // d'inactivité), et au moins un objet a attendu au-dessus du cache dans « pauses ».
    void deferredFoldShowsInTheResult()
    {
        const QJsonArray scenarios = m_root.value("scenarios").toArray();
        QCOMPARE(scenarios.at(0).toObject().value("cache").toObject().value("refreshes").toInt(), 0);
        QVERIFY(scenarios.at(4).toObject().value("cache").toObject().value("pending_max").toInt() >= 1);
    }
```

- [ ] **Étape 7 : scénario `cache-differe`.** Créer `bench/scenarios/cache-differe.json` :

```json
{
  "format": "actcraft-scenario/1",
  "name": "cache-differe",
  "description": "Lot 10 : trois traits attendent au-dessus du cache, restent visibles, sont versés après l'inactivité et y restent ; un zoom verse aussi.",
  "steps": [
    {"waitReady": true},
    {"command": "pen.color", "argument": "#000000"},
    {"command": "pen.width", "argument": 8},
    {"stroke": [[300, 300], [700, 300]], "pressure": 0.5},
    {"stroke": [[300, 400], [700, 400]], "pressure": 0.5},
    {"stroke": [[300, 450], [500, 650]], "pressure": 0.5},
    {"expect": "fixedPage.pendingCount", "value": 3},
    {"checkPixel": "attente-1", "at": [500, 300], "color": "#000000", "tolerance": 60},
    {"checkPixel": "attente-2", "at": [500, 400], "color": "#000000", "tolerance": 60},
    {"checkEdge": "trait-en-attente", "from": [370, 550], "to": [430, 550], "minIntermediate": 2},
    {"capture": "en-attente"},
    {"waitFor": "fixedPage.pendingCount", "value": 0, "timeoutMs": 5000},
    {"waitReady": true},
    {"checkPixel": "verse-1", "at": [500, 300], "color": "#000000", "tolerance": 60},
    {"checkPixel": "verse-2", "at": [500, 400], "color": "#000000", "tolerance": 60},
    {"checkEdge": "trait-verse", "from": [370, 550], "to": [430, 550], "minIntermediate": 2},
    {"capture": "verses"},
    {"stroke": [[300, 200], [700, 200]], "pressure": 0.5},
    {"expect": "fixedPage.pendingCount", "value": 1},
    {"command": "view.zoom", "argument": 150},
    {"waitReady": true},
    {"expect": "fixedPage.pendingCount", "value": 0},
    {"checkPixel": "verse-au-zoom", "at": [500, 200], "color": "#000000", "tolerance": 60},
    {"capture": "zoom"}
  ]
}
```

  Les trois traits se suivent plus vite que le délai d'inactivité (le joueur enchaîne ses étapes) ; si la machine
  chargée en laisse passer un, `LU fixedPage.pendingCount` le dira (`ECART`) : relancer, et si l'écart se répète,
  le rapporter (ne jamais allonger le délai pour le test). Dans `tests/bench.cmake`, `tst_captures_cache_differe`
  sur le modèle de `tst_captures_anticrenelage` (scénario `cache-differe.json`, sortie
  `${CMAKE_BINARY_DIR}/captures-cache-differe`, mêmes propriétés).

- [ ] **Étape 8 : vert.** `format.ps1`, preuve de tâche (`tst_pageview`, `tst_ui_drawing`, `tst_captures_cache_differe`,
  `tst_bench_verdict`), preuve visuelle hors écran (`-Scenario cache-differe`, `-Lire fixedPage.pendingCount=0`),
  captures `en-attente` et `verses` décrites (mêmes traits, mêmes bords), banc sans écran (lignes `cache=` de chaque
  scénario jointes).

- [ ] **Étape 9 : commit.**
  ```powershell
  git -C <wt> add ui/include/actcraft/ui/pageview.h ui/src/pageview.cpp ui/src/uiforeigntypes.h ui/qml/PageArea.qml tests/tst_pageview.cpp tests/tst_ui_drawing.cpp bench/include/actcraft/bench/result.h bench/src/result.cpp bench/src/runner.cpp tests/tst_bench_verdict.cpp tests/tst_bench_result.cpp bench/scenarios/cache-differe.json tests/bench.cmake
  git -C <wt> commit -m "Vue : versement différé au cache (inactivité, plafond, zoom), couche d'attente branchée (L10-T4)" -m "tst_bench_verdict compte cinq contrôles de contenu (stroke-folded) ; tst_bench_result attend au moins deux redessins dans pauses (versement différé)." -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

<a id="tache-l10-t6"></a>
### Tâche L10-T6 — Mesure S13 et décision sur les tuiles

**Objectif.** S13 : trois passages `scripts/bench.ps1` en D3D11, plein écran, cache actif, **consécutifs** (aucun
passage du lot 10 écarté entre eux) : `freeze_over_limit = 0` et `resume_over_limit = 0` sur les cinq scénarios,
dont `pauses`. Le poste est partagé et souvent saturé (mesure du lot 2 : la 0.1.0 s'y dégrade autant que le lot 2) :
les trois passages sont **encadrés par la référence** (un passage avant, un après), et la condition « machine
calme » est relevée avant chaque passage. Résumé commité ; décision écrite sur la tâche conditionnelle L10-T7.

**Référence.** Le commit de merge de L10-T1 sur `feat/livraison-complete` (ancien cache, banc à cinq scénarios),
lu dans le fichier d'état ; construit en release dans `.worktrees/livraison-complete--mesure-ref10` (détaché). Le
lot 10 : la tête de `feat/livraison-complete` après L10-T4, construite en release dans le worktree de la tâche.

**Fichiers.**
- Créer : `docs/mesures/AAAA-MM-JJ-cache-incremental.md` (date du jour des passages).

**Test et commande bornée.** Pas de test ctest. Préparation (jeton `tests`) :
```powershell
git -C C:\Users\JeromeDenis\ActCraft worktree add --detach .worktrees/livraison-complete--mesure-ref10 <commit de merge de L10-T1>
pwsh -NoProfile -Command "Set-Location 'C:\Users\JeromeDenis\ActCraft\.worktrees\livraison-complete--mesure-ref10'; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire L10-T6 -Commande 'pwsh -NoProfile -File ./scripts/build.ps1 -Preset release *> .tmp-build.txt'"
pwsh -NoProfile -Command "Set-Location '<wt>'; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom tests -Proprietaire L10-T6 -Commande 'pwsh -NoProfile -File ./scripts/build.ps1 -Preset release *> .tmp/build-release.txt'"
```
Un passage (jeton `ecran`, après le relevé « machine calme » des règles communes) :
```powershell
pwsh -NoProfile -Command "Set-Location '<dossier du passage>'; New-Item -ItemType Directory -Force .tmp | Out-Null; pwsh -NoProfile -File C:\Users\JeromeDenis\.claude\scripts\jeton.ps1 -Nom ecran -Proprietaire L10-T6 -Commande 'pwsh -NoProfile -File ./scripts/bench.ps1 -Rendu d3d11 -SansConstruire > .tmp/bench-d3d11.txt 2>&1'; Select-String -Path .tmp/bench-d3d11.txt -Pattern 'BUDGET (TENU|DEPASSE)|MESURE INVALIDE' | Select-Object -Last 1"
pwsh -NoProfile -Command "Set-Location '<dossier du passage>'; `$j = Get-ChildItem bench/results/*direct3d11.json | Sort-Object LastWriteTime | Select-Object -Last 1; `$o = Get-Content -Raw `$j | ConvertFrom-Json; Write-Output (`$j.Name + ' valide=' + `$o.valid_for_measurement + ' content=' + `$o.content_check.ok); `$o.scenarios | ForEach-Object { Write-Output ('{0} fps={1:N1} p95={2:N1} >25={3:P1} figements>lim={4} (max {5:N1}) reprises>lim={6} (max {7:N1}) cache={8}' -f `$_.name, `$_.frames.fps, `$_.frames.p95_ms, `$_.frames.over_25ms_ratio, `$_.frames.freeze_over_limit, `$_.frames.freeze_max_ms, `$_.frames.resume_over_limit, `$_.frames.resume_max_ms, (`$_.cache | ConvertTo-Json -Compress)) }"
```
Ordre : référence → lot 10 → lot 10 → lot 10 → référence. Un passage `MESURE INVALIDE` est rejoué une fois
(écrit dans le résumé) ; s'il l'est encore, la série recommence.

**Interdits.** Aucun changement de code. Aucune relance « jusqu'à ce que ça passe » : une série de trois est
jugée telle quelle ; une seconde série n'est permise que si la première a eu un passage invalide ou si la machine
n'était pas calme, et les deux séries sont rapportées. Fenêtres fermées par PID si un passage échoue.

**Terminé quand.** Le résumé est commité avec : configuration (machine, commits, date), condition « machine
calme » de chaque passage (jeton, RAM libre, compilations), la table par passage et par scénario (images/s, p95,
> 25 ms, figements et reprises hors limite avec leurs maximums, `cache.refreshes`, `cache.pending_max`, latence
p50/p95, pic mémoire), le verdict S13 (**tenu**, **non tenu** ou **non jugé** — machine chargée), l'attribution de
chaque figement ou reprise hors limite (instant comparé à `raw.cache_request_ns` : redessin du cache ou autre
cause), et la **décision sur L10-T7** ; worktree de référence supprimé.

**Étapes.**

- [ ] **Étape 1 : construire** la référence et le lot 10 en release (commandes ci-dessus).
- [ ] **Étape 2 : jouer la série** (cinq passages, relevé « machine calme » avant chacun, sous le jeton `ecran`).
- [ ] **Étape 3 : attribuer.** Fenêtres de redessin : pour chaque demande `raw.cache_request_ns[i]`, la fenêtre
  va jusqu'à la première fin `raw.cache_updated_ns` qui la suit (des demandes rapprochées peuvent partager une fin ;
  sans fin dans le scénario : jusqu'à la dernière image). Tout intervalle `F` ou `R` hors limite (`raw.interval_class`,
  bornes `raw.swap_ns[k-1]`, `raw.swap_ns[k]`) qui **recoupe** une fenêtre est classé « redessin », sinon « autre »
  (une fenêtre fixe de 50 ms raterait un redessin de 41,65 ms suivi d'une période). Dans `pauses`, ne juger le
  versement que sur les reprises k ≥ 1, et seulement si le `max` des retards d'injection et de réception du
  scénario (objet `input` du JSON) reste sous 8 ms ; sinon `pauses` est noté « hors fenêtre » pour ce passage
  (l'attribution par recouvrement reste faite). Le
  verdict S13 compte, lui, toutes les reprises.
- [ ] **Étape 4 : décider L10-T7.** Machine calme et S13 tenu → **L10-T7 non jouée** (écrite « non requise »). Machine
  calme, une reprise ou un figement hors limite attribué à un redessin du cache → **L10-T7 requise** (DA11). Hors
  limite attribué à autre chose → rapporté à l'orchestrateur (tâche de correction à écrire, hors tuiles). Machine
  chargée → **non jugé**, L10-T7 non jouée, mesure à refaire à la clôture.
- [ ] **Étape 5 : écrire le résumé** `docs/mesures/AAAA-MM-JJ-cache-incremental.md` sur le modèle de
  `docs/mesures/2026-10-17-lot2-document.md` (en-tête « Ce n'est pas une machine de référence », machine, régime du
  banc : résultat, pas une porte, décision 35), avec les éléments du critère « terminé ». Le tableau « Résumé du banc
  de fluidité » de chaque passage du lot 10 peut venir de `scripts/resume-banc.ps1`.
- [ ] **Étape 6 : nettoyer et commiter.**
  ```powershell
  git -C C:\Users\JeromeDenis\ActCraft worktree remove .worktrees/livraison-complete--mesure-ref10
  git -C <wt> add docs/mesures/AAAA-MM-JJ-cache-incremental.md
  git -C <wt> commit -m "Mesure S13 du cache incrémental : trois passages D3D11 encadrés par la référence (L10-T6)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```
  (le nom du fichier porte la vraie date).

---

<a id="tache-l10-t7"></a>
### Tâche L10-T7 — (Conditionnelle) Cache par tuiles

**Jouée seulement si L10-T6 l'a écrite « requise »** (machine calme, reprise ou figement hors limite attribué au
redessin du cache). Sinon l'orchestrateur la marque « non requise » dans l'état, avec le renvoi au résumé.

**Objectif.** DA11 : « passage à un cache par tuiles qui ne redessine que les tuiles touchées ». La texture unique
est remplacée par une grille de `ShaderEffectSource`, chacun avec son `sourceRect` sur la page fixe ; un versement
ne redessine que les tuiles qui recoupent l'emprise des objets versés ; un redessin complet (zoom, page,
reconstruction) les redessine toutes.

**Vérifier avant d'écrire (le piège).** Chaque tuile photographie toute la scène de la page fixe, rognée : si le
coût mesuré vient de la préparation des nœuds (fil de synchronisation) et non du dessin des pixels, des tuiles ne
le réduiront pas. Étape 1 obligatoire : mesurer, dans `tst_ui_drawing` en release sur la page du banc, le coût
d'un redessin d'une seule tuile de 512 × 512 contre celui du cache entier (instants `cacheRefreshRequested` →
`scheduledUpdateCompleted`). **Si une tuile coûte plus de la moitié du cache entier, abandonner** : rapporter à
l'orchestrateur (hypothèse pour Jérôme : mesure sur machine calme et seuil, ou autre remède), sans écrire le pavage.

**Fichiers.**
- Modifier : `render/include/actcraft/render/fixedpageitem.h`, `render/src/fixedpageitem.cpp`,
  `tests/tst_render_pendinglayer.cpp`, `ui/include/actcraft/ui/pageview.h`, `ui/src/pageview.cpp`,
  `ui/qml/PageArea.qml`, `tests/tst_pageview.cpp`, `tests/tst_ui_drawing.cpp`,
  `docs/mesures/AAAA-MM-JJ-cache-incremental.md` (même fichier que L10-T6, section « Tuiles » ajoutée)

**Test et commande bornée.** Rouge : commande ciblée, motif `tst_pageview|tst_render_pendinglayer` → `error`
(`pendingBounds`, `cacheTileRects` inconnus). Vert : preuve de tâche, N ≥ 94, `tst_pageview` (dont
`foldRefreshesOnlyTheTouchedTiles`, `fullRefreshRefreshesEveryTile`), `tst_render_pendinglayer` (dont
`pendingBoundsCoverThePendingStrokes`), `tst_ui_drawing`, `tst_captures_cache_differe` Passed ; série S13 de
L10-T6 rejouée (même procédure) et tenue.

**Interdits.** Ceux de L10-T4 (aucun appelant changé). Aucun recouvrement visible entre tuiles (une couture d'un
pixel est un défaut : test de pixels sur une frontière de tuile).

**Terminé quand.** Mesure de l'étape 1 écrite ; soit abandon rapporté, soit preuve verte et S13 tenu ; commit fait.

**Étapes.**

- [ ] **Étape 1 : mesurer une tuile contre le cache entier** (voir ci-dessus) ; écrire les deux durées médianes sur
  10 redessins dans le résumé de mesure ; décider (≤ 50 % : continuer ; sinon : abandon rapporté).
- [ ] **Étape 2 : tests.** Dans `tests/tst_render_pendinglayer.cpp` :

```cpp
    void pendingBoundsCoverThePendingStrokes()
    {
        Fixture f;
        open(f, true);
        f.page->setDeferredFolding(true);
        QVERIFY(f.page->pendingBounds().isEmpty());
        f.page->appendStroke(lineAt(100));
        f.page->appendStroke(lineAt(300));
        const QRectF b = f.page->pendingBounds();
        QVERIFY(b.contains(QPointF(100, 100)) && b.contains(QPointF(500, 300)));
        QVERIFY(b.top() <= 100 - 4 && b.bottom() >= 300 + 4); // demi-largeur comprise
    }
```

  Dans `tests/tst_pageview.cpp` (montage `PendingFixture` de L10-T4) :

```cpp
    void foldRefreshesOnlyTheTouchedTiles()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        const QVariantList tiles = f.cached.view->cacheTileRects();
        QVERIFY(tiles.size() > 1);
        QSignalSpy tile(f.cached.view, &PageView::cacheTileRefreshRequested);
        f.cached.fixed->appendStroke(line(20)); // en haut de la page : une rangée de tuiles
        QTRY_COMPARE_WITH_TIMEOUT(tile.count() > 0, true, kCacheFoldIdleMs + 2000);
        QVERIFY(tile.count() < tiles.size());
        for (const auto& call : tile) {
            QVERIFY(tiles.at(call.at(0).toInt()).toRectF().top() < 30);
        }
    }

    void fullRefreshRefreshesEveryTile()
    {
        PendingFixture f;
        QVERIFY(openPending(f, m_stylus.get()));
        QSignalSpy tile(f.cached.view, &PageView::cacheTileRefreshRequested);
        f.cached.view->setZoom(2.0);
        QCOMPARE(tile.count(), f.cached.view->cacheTileRects().size());
    }
```

- [ ] **Étape 3 : rouge** (commande ciblée).
- [ ] **Étape 4 : emprise en attente.** `FixedPageItem::pendingBounds()` (`QRectF`, coordonnées de page) : union des
  emprises des objets en attente (`strokeBounds(points, baseWidth)` pour un trait ; pour les autres, l'emprise de
  l'objet transformée), calculée sur le fil principal.
- [ ] **Étape 5 : tuiles dans la vue.** `PageView` : `Q_PROPERTY(QVariantList cacheTileRects READ cacheTileRects
  NOTIFY cacheTilesChanged)` (rectangles de page de 512 / zoom pixels de page de côté, recalculés avec
  `updateCacheTextureSize`), signal `cacheTileRefreshRequested(int index)` ; `requestCacheRefresh(QRectF area =
  {})` interne : aire vide = toutes les tuiles ; le versement par inactivité passe `pendingBounds()` **lu avant**
  `foldPending()` ; `notifyCacheUpdated` compte les tuiles demandées et ne rend `cacheReady` vrai qu'à la dernière.
  `cacheRefreshRequested` reste émis pour un redessin complet (les tests et le banc du lot 2 l'écoutent).
- [ ] **Étape 6 : tuiles en QML.** Dans `PageArea.qml`, le `ShaderEffectSource` unique devient un `Repeater` sur
  `view.cacheTileRects` : chaque délégué est un `ShaderEffectSource` (`sourceItem: fixedPageItem`, `sourceRect:
  modelData`, `x`/`y`/`width`/`height` = `modelData`, `live: false`, `samples: 4`, `hideSource` porté par un seul
  d'entre eux — ou par un `Item` parent `layer` — vérifier que la page fixe reste cachée), `textureSize` à la taille
  en pixels de sa tuile ; `Connections { target: view; function onCacheTileRefreshRequested(i) { if (i === index)
  scheduleUpdate() } }` ; `onScheduledUpdateCompleted: view.notifyCacheUpdated()`. `objectName` `pageCache` garde
  le premier délégué lisible par les scénarios (`pageCache.samples`).
- [ ] **Étape 7 : couture.** Dans `tst_ui_drawing`, un trait qui traverse la frontière de deux tuiles : après
  versement, les pixels de part et d'autre de la frontière sont noirs (aucune ligne blanche).
- [ ] **Étape 8 : vert** (preuve de tâche, preuves visuelles `cache-differe` et `anticrenelage` hors écran, banc sans écran avec son JSON), puis
  **série S13 de L10-T6 rejouée** (même procédure, même ordre) et ajoutée au résumé, section « Tuiles ».
- [ ] **Étape 9 : commit.**
  ```powershell
  git -C <wt> add render/include/actcraft/render/fixedpageitem.h render/src/fixedpageitem.cpp tests/tst_render_pendinglayer.cpp ui/include/actcraft/ui/pageview.h ui/src/pageview.cpp ui/qml/PageArea.qml tests/tst_pageview.cpp tests/tst_ui_drawing.cpp docs/mesures/AAAA-MM-JJ-cache-incremental.md
  git -C <wt> commit -m "Cache par tuiles : seules les tuiles touchées par le versement sont redessinées (L10-T7)" -m "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"
  ```

---

## Hypothèses à revoir (porte de fin du lot 10)

- **H10-1 — S6 et S7 remplacés** (écart 1) par S2, S4, S5 et `cache-differe` ; S6 et S7 joués sur la nouvelle
  implémentation aux lots 3 et 4. **Reporté explicitement dans S6 (lot 3)** : l'essai, à l'écran en D3D11, de
  l'annulation d'un trait encore en attente (le trait disparaît, aucun redessin du cache), quand l'annulation sera
  branchée à une commande.
- **H10-2 — Délai d'inactivité 500 ms, plafond 128 objets** : valeurs de départ. Un délai plus court verse plus
  souvent pendant l'écriture (risque de reprise au mauvais moment) ; plus long, la couche d'attente grandit. Réglées
  par la mesure de L10-T6 si elle le demande.
  Seules les entrées qui agissent repoussent le versement ; un survol ne le repousse pas (L10-T4,
  `hoverDoesNotPostponeTheFold`).
- **H10-3 — Plafond atteint pendant l'écriture** : 128 traits sans 500 ms de pause font verser pendant l'écriture (un
  figement possible, comme au lot 2) ; aucun scénario du banc ne l'atteint. Les survols ne comptant pas comme entrées, seule une écriture
  vraiment continue l'atteint.
- **H10-4 — Annuler un objet déjà versé** redessine tout le cache tout de suite (comportement du lot 2) ; annuler un
  objet encore en attente ne coûte rien. Mesuré au lot 3 (S6, annulation branchée).
- **H10-5 — MSAA 4×** de la fenêtre et du cache : coût mesuré sur la puce Intel intégrée de la machine de
  développement ; sur le Dell 3510, inconnu (mesure hors porte, décision 0087).
- **H10-6 — Forme du trait de la référence** (courbe lissée, bouts et jonctions ronds) : hors lot 10 ; transmise au
  lot 3 (`L3-T0`), qui la tranche sur la 3.5.18.
- **H10-7 — Grille nette** : préservée par contrôle négatif (S33) ; si une grille lue d'un `.flipchart` est dessinée
  en géométrie un jour, le MSAA la toucherait : le contrôle `grille-nette` le verra.
- **H10-8 — Limite de DA11** (écart 5) : restent au redessin immédiat, avec un figement possible, (a) l'objet posé
  sous la couche du dessus (lot 4), (b) l'image insérée (lot 5), (c) le trait sur une page à grille dessinée
  au-dessus (lot 4) ; scénarios de banc à ajouter au banc du lot 4 (a, c) et du lot 5 (b), S7 et S8 n'étant que des
  captures. Hors limite : tuiles (L10-T7) ou tâche du lot concerné.
- **H10-9 — Mémoire du MSAA** : texture du cache jusqu'à 4096 px de côté (zoom 800 %), + 192 à + 384 Mio selon le
  pilote, sur un pic du lot 1 de 473 à 698 Mo et un budget de 1 Gio ; relevée à L10-T5 (pic GPU de `zoom`) ; repli
  écrit à L10-T5, étape 6 (échantillons du cache selon la taille de la texture : 4 sous 2048², 1 au-dessus ; 0 suivrait la fenêtre).

**Écartés du lot 10, avec la raison :**
- H2b-5 (rastérisation des objets peints pendant la synchronisation, RHI) : ni un figement ni une reprise ; le
  scénario `page` le rapporte à la clôture ; s'il dépasse sur machine calme, il va au lot 4 (texte, objets peints).
- Copie du document à l'Enregistrer (3,5 ms depuis L2-C9) : hors de l'interface de cache.
- `FixedPageItem::setPage` qui redécode (banc et tests seulement) : hors de la fenêtre de mesure du banc.
- H2b-7 (deux redessins vers une page à images) : la reconstruction verse et redessine comme avant ; le scénario
  `page` le mesure ; aucun changement sans mesure qui le demande.

## Décisions tranchées par ce plan

1. **Version 0.3.0** : la décision 31 du cadrage (0.9.1) est **amendée** par l'amendement 1, § 6 (« lot 10 →
   0.3.0 », versions dans l'ordre de livraison ; `versionCode` 300), qui fait autorité sur ce qu'il change. Ce n'est
   pas une hypothèse.
2. **Couche d'attente sœur du cache**, état partagé écrit sur le fil principal seulement (patrons écartés : voir
   L10-T3).
3. **Différé seulement avec cache actif et couche branchée** : sans couche, comportement du lot 2 (les tests du lot
   2 restent vrais sans modification).
4. **MSAA plutôt que bords fondus dans la géométrie** : une ligne de configuration, mesurée au banc ; le repli reste
   possible derrière la même apparence (L10-T5, étape 10).
5. **L10-T0 n'attend pas L10-T1 ni L10-T2** (outil de mesure, refonte sans effet) ; les tâches de comportement
   l'attendent.

## Clôture du lot 10 (orchestrateur, hors tâches)

Jouée après la dernière vague, dans le worktree du chantier, sur `feat/livraison-complete` à jour, avant la PR.

1. **Double relecture** (conformité, qualité) du diff du lot, chaque tâche `concepteur-critique` relue après rejeu ;
   correctifs `L10-C<n>` (deux tours au plus).
2. **Simplification** : tâche `L10-S1` (comme L2-S1 : diff de `tests/` vide).
3. **Version 0.3.0** : `project(VERSION 0.3.0)` dans `CMakeLists.txt` ; `CHANGELOG.md` section `## 0.3.0 — <date>`
   (traits versés au cache sans figement, traits lissés en Direct3D 11, scénario de banc `pauses`) ;
   `scripts/notes-version.ps1` → `NOTES OK`.
4. **Suites** (jeton `tests`) : `scripts/test.ps1 -Preset release` puis `scripts/test.ps1`, formes bornées, N ≥ 94
   chacune, `TESTS OK` ; aucun test retiré ; seuls sautés, sans variables : les trois tests locaux du lot 2 (rejoués
   avec leurs variables, comme à la clôture du lot 2 : `ORACLE OK`, `ORACLE SYNTHETIQUE OK`, `MINIATURES OK`).
5. **Captures à l'écran en D3D11** (jeton `ecran`, annoncé) : `captures.ps1 -Scenario tous -Rendu d3d11 -Sortie
   .tmp/captures-ecran` (`premiers-pas`, `aller-retour-natif`, `ouvrir-flipchart-synthetique`, `anticrenelage`,
   `cache-differe`) → `SCENARIO OK` chacun, `BORD … OK`, `CAPTURES OK` ; `local/ouvrir-flipchart -Ouvrir
   "<samples>\<paperboard réel n° 16>"` → `CAPTURES OK` ; captures regardées et décrites (bords lissés, grille
   nette, traits en attente puis versés identiques).
6. **Banc** (jeton `ecran`, machine calme relevée) : (a) si un correctif de fin de lot a touché `render/ input/
   tools/ ui/` après L10-T6, ou si L10-T6 a conclu « non jugé », **S13 rejoué** (série de L10-T6) ; (b)
   **non-régression du lot 2 et du lot 10**, rejouée ici (celle du lot 2 n'a pas pu être jugée, machine saturée) :
   0.1.0 (`3de027a`) → 0.2.0 (`70c0493`) → 0.3.0 → 0.2.0 → 0.1.0, `bench.ps1 -Rendu d3d11 -SansConstruire`,
   chacun dans un worktree détaché construit en release ; comparé critère par critère à
   `docs/mesures/2026-10-15-lot1-socle.md` sur `stroke`, `zoom`, `scroll` (« figements » et « reprises » entrent
   désormais dans la porte, DA7) ; puis `bench.ps1 -Rendu tous -Captures .tmp/banc-lot10` sur la 0.3.0 ; résumé
   ajouté au fichier de mesure du lot. Machine chargée : rapporté, **non jugé**, présenté tel quel à la porte.
7. **Paquets** : `scripts/paquet.ps1` + `scripts/essai-installateur.ps1` (`PAQUET OK`, `INSTALLATEUR OK`,
   `--version : ActCraft 0.3.0`), `scripts/paquet-android.ps1` (`APK OK`, démon Gradle arrêté par PID).
8. **Dépôt public** : `NOMS OK` ; `git grep` des titres du corpus vide (commande de L10-T0) ;
   `git grep -n "Private\|_p\.h" -- core formats actions render ui app` vide.
9. **Graphe** : `graphify update .`, commit de `graphify-out/`.
10. **PR du lot** (étage 4) et **porte de fin du lot 10** : release 0.3.0 proposée (publiée seulement sur go écrit) ;
    S13 (tenu, non tenu ou non jugé, avec la condition machine), S33, écarts au cadrage 1 à 6 (dont le volume, 4 → 8 tâches), hypothèses H10-1 à
    H10-9, décision sur L10-T7, constats de L10-T0 transmis aux autres lots.
