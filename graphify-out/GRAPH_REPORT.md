# Graph Report - .  (2026-10-13)

## Corpus Check
- Corpus is ~4,937 words - fits in a single context window. You may not need a graph.

## Summary
- 70 nodes · 57 edges · 23 communities (14 shown, 9 thin omitted)
- Extraction: 89% EXTRACTED · 11% INFERRED · 0% AMBIGUOUS · INFERRED: 6 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

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

## God Nodes (most connected - your core abstractions)
1. `parseRenderBackend()` - 9 edges
2. `TstRenderBackend` - 6 edges
3. `main()` - 3 edges
4. `RenderBackend` - 3 edges
5. `applyRenderBackend()` - 3 edges
6. `TstModules` - 3 edges
7. `TstUi` - 3 edges
8. `actcraft()` - 2 edges
9. `QStringList` - 2 edges
10. `QString` - 2 edges

## Surprising Connections (you probably didn't know these)
- `main()` --calls--> `applyRenderBackend()`  [INFERRED]
  app/main.cpp → app/renderbackend.cpp
- `main()` --calls--> `parseRenderBackend()`  [INFERRED]
  app/main.cpp → app/renderbackend.cpp

## Import Cycles
- None detected.

## Communities (23 total, 9 thin omitted)

### Community 0 - "Community 0"
Cohesion: 0.24
Nodes (6): main(), applyRenderBackend(), parseRenderBackend(), RenderBackend, Q_OBJECT, TstRenderBackend

### Community 1 - "Community 1"
Cohesion: 0.22
Nodes (5): QObject, Q_OBJECT, TstModules, Q_OBJECT, TstUi

## Knowledge Gaps
- **11 isolated node(s):** `namespace`, `namespace`, `namespace`, `namespace`, `namespace` (+6 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **9 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `parseRenderBackend()` connect `Community 0` to `Community 2`?**
  _High betweenness centrality (0.005) - this node is a cross-community bridge._
- **Why does `TstRenderBackend` connect `Community 0` to `Community 1`?**
  _High betweenness centrality (0.002) - this node is a cross-community bridge._
- **Are the 2 inferred relationships involving `main()` (e.g. with `applyRenderBackend()` and `parseRenderBackend()`) actually correct?**
  _`main()` has 2 INFERRED edges - model-reasoned connections that need verification._
- **What connects `namespace`, `namespace`, `namespace` to the rest of the system?**
  _11 weakly-connected nodes found - possible documentation gaps or missing edges._