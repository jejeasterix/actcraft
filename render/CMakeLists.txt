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
        include/actcraft/render/imagestore.h
        src/imagesynth.cpp
        src/imagestore.cpp
        src/imagerenderer.cpp
    DEPENDS
        ActCraft::core Qt6::Quick)
