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
        include/actcraft/render/fixedpageitem.h
        src/fixedpageitem.cpp
        include/actcraft/render/inkbuffer.h
        src/inkbuffer.cpp
        include/actcraft/render/inkitem.h
        src/inkitem.cpp
        include/actcraft/render/pagepainter.h
        src/pagepainter.cpp
    DEPENDS
        ActCraft::core Qt6::Quick)

# Les types de render/ sont exposés en QML par ui/ (QML_FOREIGN) : qmltyperegistrar a besoin de leurs métatypes.
qt_extract_metatypes(actcraft_render)
