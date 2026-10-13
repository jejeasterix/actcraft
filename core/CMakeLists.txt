# Modèle de document : pages, objets, pile d'annulation, géométrie, accrochage. Sans QtGui ni QtQuick.
actcraft_add_module(core
    SOURCES
        src/core.cpp
        src/model.cpp
        src/generator.cpp
    DEPENDS
        Qt6::Core)
