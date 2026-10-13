# Outils en extensions autonomes : stylo, gomme, formes, texte, sélection, grille…
actcraft_add_module(tools
    SOURCES
        src/tools.cpp
        src/pentool.cpp
    DEPENDS
        ActCraft::core ActCraft::actions ActCraft::input ActCraft::render)
