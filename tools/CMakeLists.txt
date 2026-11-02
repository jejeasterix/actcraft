# Outils en extensions autonomes : stylo, gomme, formes, texte, sélection, grille…
actcraft_add_module(tools
    SOURCES
        src/tools.cpp
        src/pentool.cpp
        include/actcraft/tools/tool.h
        include/actcraft/tools/toolcontext.h
        include/actcraft/tools/userstate.h
        src/userstate.cpp
        include/actcraft/tools/toolregistry.h
        src/toolregistry.cpp
        include/actcraft/tools/erasertool.h
        src/erasertool.cpp
        include/actcraft/tools/selecttool.h
        src/selecttool.cpp
        include/actcraft/tools/selectiongeometry.h
        src/selectiongeometry.cpp
        include/actcraft/tools/shapetool.h
        src/shapetool.cpp
        include/actcraft/tools/texttool.h
        src/texttool.cpp
    DEPENDS
        ActCraft::core ActCraft::actions ActCraft::input ActCraft::render)
