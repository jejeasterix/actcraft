# Graph Report - prototype-mesure  (2026-10-14)

## Corpus Check
- 135 files · ~94,036 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 1378 nodes · 2188 edges · 97 communities (67 shown, 30 thin omitted)
- Extraction: 92% EXTRACTED · 8% INFERRED · 0% AMBIGUOUS · INFERRED: 179 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `d293c538`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- [[_COMMUNITY_Community 0|Community 0]]
- [[_COMMUNITY_Community 1|Community 1]]
- [[_COMMUNITY_Community 2|Community 2]]
- [[_COMMUNITY_Community 3|Community 3]]
- [[_COMMUNITY_Community 4|Community 4]]
- [[_COMMUNITY_Community 5|Community 5]]
- [[_COMMUNITY_Community 6|Community 6]]
- [[_COMMUNITY_Community 7|Community 7]]
- [[_COMMUNITY_Community 8|Community 8]]
- [[_COMMUNITY_Community 9|Community 9]]
- [[_COMMUNITY_Community 10|Community 10]]
- [[_COMMUNITY_Community 19|Community 19]]
- [[_COMMUNITY_Community 20|Community 20]]
- [[_COMMUNITY_Community 21|Community 21]]
- [[_COMMUNITY_Community 22|Community 22]]
- [[_COMMUNITY_Community 23|Community 23]]
- [[_COMMUNITY_Community 24|Community 24]]
- [[_COMMUNITY_Community 25|Community 25]]
- [[_COMMUNITY_Community 26|Community 26]]
- [[_COMMUNITY_Community 27|Community 27]]
- [[_COMMUNITY_Community 28|Community 28]]
- [[_COMMUNITY_Community 29|Community 29]]
- [[_COMMUNITY_Community 30|Community 30]]
- [[_COMMUNITY_Community 31|Community 31]]
- [[_COMMUNITY_Community 32|Community 32]]
- [[_COMMUNITY_Community 33|Community 33]]
- [[_COMMUNITY_Community 34|Community 34]]
- [[_COMMUNITY_Community 35|Community 35]]
- [[_COMMUNITY_Community 36|Community 36]]
- [[_COMMUNITY_Community 37|Community 37]]
- [[_COMMUNITY_Community 38|Community 38]]
- [[_COMMUNITY_Community 39|Community 39]]
- [[_COMMUNITY_Community 40|Community 40]]
- [[_COMMUNITY_Community 41|Community 41]]
- [[_COMMUNITY_Community 42|Community 42]]
- [[_COMMUNITY_Community 43|Community 43]]
- [[_COMMUNITY_Community 44|Community 44]]
- [[_COMMUNITY_Community 45|Community 45]]
- [[_COMMUNITY_Community 46|Community 46]]
- [[_COMMUNITY_Community 47|Community 47]]
- [[_COMMUNITY_Community 48|Community 48]]
- [[_COMMUNITY_Community 49|Community 49]]
- [[_COMMUNITY_Community 50|Community 50]]
- [[_COMMUNITY_Community 51|Community 51]]
- [[_COMMUNITY_Community 52|Community 52]]
- [[_COMMUNITY_Community 53|Community 53]]
- [[_COMMUNITY_Community 54|Community 54]]
- [[_COMMUNITY_Community 55|Community 55]]
- [[_COMMUNITY_Community 56|Community 56]]
- [[_COMMUNITY_Community 57|Community 57]]
- [[_COMMUNITY_Community 58|Community 58]]
- [[_COMMUNITY_Community 59|Community 59]]
- [[_COMMUNITY_Community 60|Community 60]]
- [[_COMMUNITY_Community 61|Community 61]]
- [[_COMMUNITY_Community 62|Community 62]]
- [[_COMMUNITY_Community 63|Community 63]]
- [[_COMMUNITY_Community 64|Community 64]]
- [[_COMMUNITY_Community 65|Community 65]]
- [[_COMMUNITY_Community 66|Community 66]]
- [[_COMMUNITY_Community 67|Community 67]]
- [[_COMMUNITY_Community 68|Community 68]]
- [[_COMMUNITY_Community 69|Community 69]]
- [[_COMMUNITY_Community 70|Community 70]]
- [[_COMMUNITY_Community 71|Community 71]]
- [[_COMMUNITY_Community 72|Community 72]]
- [[_COMMUNITY_Community 73|Community 73]]
- [[_COMMUNITY_Community 74|Community 74]]
- [[_COMMUNITY_Community 75|Community 75]]
- [[_COMMUNITY_Community 76|Community 76]]
- [[_COMMUNITY_Community 77|Community 77]]
- [[_COMMUNITY_Community 78|Community 78]]
- [[_COMMUNITY_Community 79|Community 79]]
- [[_COMMUNITY_Community 80|Community 80]]
- [[_COMMUNITY_Community 81|Community 81]]
- [[_COMMUNITY_Community 86|Community 86]]
- [[_COMMUNITY_Community 87|Community 87]]

## God Nodes (most connected - your core abstractions)
1. `TstBenchVerdict` - 24 edges
2. `PainterNode` - 21 edges
3. `classifyIntervals()` - 20 edges
4. `applyVerdict()` - 19 edges
5. `Vérifications de Qt 6.12 — prototype de mesure` - 19 edges
6. `fingerprint()` - 18 edges
7. `TstBenchStats` - 18 edges
8. `ms()` - 16 edges
9. `good()` - 16 edges
10. `generatePage()` - 15 edges

## Surprising Connections (you probably didn't know these)
- `start()` --calls--> `loadMainWindow()`  [INFERRED]
  tests/tst_bench_verdict.cpp → app/startup.cpp
- `start()` --calls--> `loadMainWindow()`  [INFERRED]
  tests/tst_ui_drawing.cpp → app/startup.cpp
- `sample()` --calls--> `classifyIntervals()`  [INFERRED]
  tests/tst_bench_json.cpp → bench/src/stats.cpp
- `sample()` --calls--> `computeLatency()`  [INFERRED]
  tests/tst_bench_json.cpp → bench/src/stats.cpp
- `main()` --calls--> `parseRenderBackend()`  [INFERRED]
  bench/app/main.cpp → app/renderbackend.cpp

## Import Cycles
- None detected.

## Communities (97 total, 30 thin omitted)

### Community 0 - "Community 0"
Cohesion: 0.11
Nodes (17): main(), printLine(), applyRenderBackend(), QString, QStringList, parseRenderBackend(), QQmlApplicationEngine, QQuickWindow (+9 more)

### Community 1 - "Community 1"
Cohesion: 0.05
Nodes (16): namespace, actcraft(), QObject, actcraft(), namespace, Q_OBJECT, QByteArray, TstBenchResult (+8 more)

### Community 2 - "Community 2"
Cohesion: 0.07
Nodes (45): BenchResult, ExitCode, int64_t, PlannedInput, QString, QStringList, ScenarioResult, SyncRecord (+37 more)

### Community 19 - "Community 19"
Cohesion: 0.06
Nodes (56): QColor, QImage, QPoint, QPointF, qreal, QString, QStringList, BenchOptions (+48 more)

### Community 20 - "Community 20"
Cohesion: 0.07
Nodes (41): atomic_bool, class, namespace, GpuMemoryReader(), MemoryTracker(), actcraft(), InkProbe, namespace (+33 more)

### Community 21 - "Community 21"
Cohesion: 0.05
Nodes (36): QQuickItem, checkStaticLayer(), ColorPainterItem, m_color, Q_PROPERTY, Q_OBJECT, QByteArray, QColor (+28 more)

### Community 22 - "Community 22"
Cohesion: 0.07
Nodes (37): namespace, actcraft(), QString, BenchResult, int64_t, optional, QString, ScenarioResult (+29 more)

### Community 23 - "Community 23"
Cohesion: 0.05
Nodes (29): ObjectRenderer, actcraft(), actcraft(), namespace, namespace, namespace, namespace, actcraft() (+21 more)

### Community 24 - "Community 24"
Cohesion: 0.06
Nodes (34): int64_t, PlannedInput, QEvent, QObject, QWindow, size_t, vector, int64_t (+26 more)

### Community 25 - "Community 25"
Cohesion: 0.08
Nodes (36): Argb, ImageObject, int32_t, Page, PointF, RectF, Shape, Stroke (+28 more)

### Community 26 - "Community 26"
Cohesion: 0.07
Nodes (27): addPoint(), color(), finish(), onSample(), PenTool(), setColor(), string, Argb (+19 more)

### Community 27 - "Community 27"
Cohesion: 0.05
Nodes (40): Banc de fluidité — mode d'emploi, Carte des fichiers, Ce que la latence mesurée ne prouve pas, Ce qui est joué, Clôture — passages au banc (orchestrateur), Commandes, Contrats partagés entre tâches (noms figés), Couverture des scénarios (+32 more)

### Community 28 - "Community 28"
Cohesion: 0.07
Nodes (22): atomic_int, QSGRenderNode, namespace, actcraft(), Q_OBJECT, QPoint, QRectF, QSGNode (+14 more)

### Community 29 - "Community 29"
Cohesion: 0.13
Nodes (19): FrameSeries, int64_t, SyncRecord, vector, IntervalStats, LatencyStats, Percentiles, classifyIntervals() (+11 more)

### Community 30 - "Community 30"
Cohesion: 0.08
Nodes (23): InkProbe, Sample, SampleSink, uint64_t, accept(), nextSequence(), Pipeline(), push() (+15 more)

### Community 31 - "Community 31"
Cohesion: 0.07
Nodes (28): MemorySummary, optional, uint64_t, int64_t, MemorySummary, pair, QObject, QQuickWindow (+20 more)

### Community 32 - "Community 32"
Cohesion: 0.09
Nodes (15): int32_t, ObjectKind, Page, PageObject, PointF, RectF, uint32_t, uint64_t (+7 more)

### Community 33 - "Community 33"
Cohesion: 0.12
Nodes (29): PageObject, PointF, QPainter, QQuickWindow, QSGNode, RenderContext, Shape, size_t (+21 more)

### Community 34 - "Community 34"
Cohesion: 0.11
Nodes (22): MouseButton, MouseButtons, App, engine, fixed, ink, view, window (+14 more)

### Community 35 - "Community 35"
Cohesion: 0.09
Nodes (17): QRgb, Page, QImage, QObject, size_t, uint64_t, ImageObject, QImage (+9 more)

### Community 36 - "Community 36"
Cohesion: 0.12
Nodes (21): ItemChangeData, content(), fixedPage(), ink(), itemChange(), notifyCacheUpdated(), onStrokeFrozen(), PageView() (+13 more)

### Community 37 - "Community 37"
Cohesion: 0.15
Nodes (14): KeyboardModifiers, PageView, Q_OBJECT, QPoint, QPointF, QQuickItem, QQuickWindow, Fixture (+6 more)

### Community 38 - "Community 38"
Cohesion: 0.14
Nodes (21): Argb, InkBuffer, InkProbe, QQuickItem, QSGNode, RectF, span, uint64_t (+13 more)

### Community 39 - "Community 39"
Cohesion: 0.13
Nodes (14): cacheTextureSize(), clampZoom(), fitPage(), wheelScrollDelta(), wheelZoomFactor(), zoomAt(), Q_OBJECT, TstViewTransform (+6 more)

### Community 40 - "Community 40"
Cohesion: 0.13
Nodes (15): GpuMemoryReader, QQuickWindow, unique_ptr, optional, QQuickWindow, QString, uint64_t, unique_ptr (+7 more)

### Community 41 - "Community 41"
Cohesion: 0.16
Nodes (16): Argb, RectF, size_t, span, StrokePoint, add(), begin(), block() (+8 more)

### Community 42 - "Community 42"
Cohesion: 0.18
Nodes (8): BenchOptions, QString, QStringList, outputAccepted(), parseBenchOptions(), resolveOutputPath(), Q_OBJECT, TstBenchOptions

### Community 43 - "Community 43"
Cohesion: 0.22
Nodes (7): Fixture, argb(), Q_OBJECT, QColor, Stroke, TstRenderFixedPage, private

### Community 44 - "Community 44"
Cohesion: 0.14
Nodes (13): Page, QQuickItem, QRectF, QSGNode, Stroke, UpdatePaintNodeData, appendStroke(), clearChildren() (+5 more)

### Community 45 - "Community 45"
Cohesion: 0.14
Nodes (16): QColor, QPainter, QQuickWindow, QRectF, RenderingFlags, RenderState, span, StateFlags (+8 more)

### Community 46 - "Community 46"
Cohesion: 0.13
Nodes (13): QSGGeometry, QSGGeometryNode, vector, Vertex2D, namespace, clear(), InkBlockNode, kCapacity (+5 more)

### Community 47 - "Community 47"
Cohesion: 0.30
Nodes (15): size_t, apply(), fitPage(), geometryChange(), pageSize(), setContent(), setPan(), setZoom() (+7 more)

### Community 48 - "Community 48"
Cohesion: 0.16
Nodes (12): addPoint(), beginStroke(), commitStroke(), FixedPageSink(), InkSurfaceAdapter(), Argb, FixedPageItem, InkItem (+4 more)

### Community 49 - "Community 49"
Cohesion: 0.15
Nodes (12): Architecture et règles, Base de connaissance — l'ancien dépôt et la rétro-ingénierie, CLAUDE.md — ActCraft (C++ / Qt 6 Quick), Démarrer une fonctionnalité — chaîne `nouvelle-feature`, Git, Knowledge graph (graphify), Langue, Mode autonome (+4 more)

### Community 50 - "Community 50"
Cohesion: 0.15
Nodes (12): Avant tout passage, Banc de fluidité — mode d'emploi, Ce qu'il faut renvoyer, Ce qui est joué, Commandes, Fichier de résultats, Limites connues, Lire le verdict (+4 more)

### Community 51 - "Community 51"
Cohesion: 0.18
Nodes (8): QSGMaterial, QSGMaterialType, QColor, QQuickWindow, size_t, InkBlockMaterial, m_block, pointCount()

### Community 52 - "Community 52"
Cohesion: 0.17
Nodes (10): PageObject, QPainter, QQuickWindow, QSGNode, RenderContext, size_t, Stroke, createNode() (+2 more)

### Community 53 - "Community 53"
Cohesion: 0.17
Nodes (11): Compteurs, Demande, Hors périmètre, Hypothèses à revoir (niveau 1 d'abord), Pièges trouvés, Portes, Référence, Scénarios et captures (+3 more)

### Community 54 - "Community 54"
Cohesion: 0.21
Nodes (12): event(), eventFilter(), handlePointer(), mouseMoveEvent(), mousePressEvent(), mouseReleaseEvent(), pageToScene(), pan() (+4 more)

### Community 55 - "Community 55"
Cohesion: 0.27
Nodes (8): QPainter, QRectF, StrokePoint, InkBlockPainterNode, m_baseWidth, m_color, m_maxHalf, m_points

### Community 56 - "Community 56"
Cohesion: 0.18
Nodes (10): 1. Demande et gabarit, 2. Décisions, 3. Hypothèses à revoir, 4. Décisions d'architecture, 5. Scénarios d'acceptation, 6. Hors périmètre, 7. Portes immédiates, Cadrage — prototype de mesure (étape 1 du BRIEF) (+2 more)

### Community 57 - "Community 57"
Cohesion: 0.25
Nodes (7): 1. Objectif, 2. Décisions, 3. Matériel cible et budget, 4. Architecture, 5. Plan, 6. Points ouverts, BRIEF — ActCraft (réécriture C++ / Qt 6 Quick)

### Community 58 - "Community 58"
Cohesion: 0.33
Nodes (6): QSGFlatColorMaterial, QColor, QSGGeometryNode, vector, Vertex2D, makeTrianglesNode()

### Community 59 - "Community 59"
Cohesion: 0.33
Nodes (5): PageObject, QSGNode, RenderContext, size_t, createNode()

### Community 60 - "Community 60"
Cohesion: 0.40
Nodes (4): ActCraft, Construire (Windows), Licence, Organisation

### Community 61 - "Community 61"
Cohesion: 0.50
Nodes (4): QInputDevice, acceptsDevice(), setExclusiveDevices(), QList

## Knowledge Gaps
- **399 isolated node(s):** `namespace`, `QStringList`, `QString`, `QStringList`, `QString` (+394 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **30 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `TstBenchVerdict` connect `Community 2` to `Community 1`?**
  _High betweenness centrality (0.075) - this node is a cross-community bridge._
- **Why does `PainterNode` connect `Community 21` to `Community 33`, `Community 38`, `Community 45`, `Community 52`, `Community 55`, `Community 28`?**
  _High betweenness centrality (0.073) - this node is a cross-community bridge._
- **Why does `TstBenchMemory` connect `Community 22` to `Community 1`, `Community 31`?**
  _High betweenness centrality (0.059) - this node is a cross-community bridge._
- **Are the 15 inferred relationships involving `classifyIntervals()` (e.g. with `play()` and `sample()`) actually correct?**
  _`classifyIntervals()` has 15 INFERRED edges - model-reasoned connections that need verification._
- **Are the 14 inferred relationships involving `applyVerdict()` (e.g. with `run()` and `contentCheckOk()`) actually correct?**
  _`applyVerdict()` has 14 INFERRED edges - model-reasoned connections that need verification._
- **What connects `namespace`, `QStringList`, `QString` to the rest of the system?**
  _399 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Community 0` be split into smaller, more focused modules?**
  _Cohesion score 0.10591133004926108 - nodes in this community are weakly interconnected._