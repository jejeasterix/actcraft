# Un moteur de rendu par type d'objet (nœuds du graphe de scène Qt Quick), choisi dans un registre.
actcraft_add_module(render
    SOURCES
        src/render.cpp
        src/strokegeometry.cpp
        src/painternode.cpp
        src/nodes.cpp
        src/registry.cpp
        src/shaperenderer.cpp
        src/strokerenderer.cpp
    DEPENDS
        ActCraft::core Qt6::Quick)
