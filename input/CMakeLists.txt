# Chaîne stylet : événements bruts, filtrage, prédiction, puis outil. Un module natif par système.
actcraft_add_module(input
    SOURCES
        src/input.cpp
        src/pipeline.cpp
        src/qtevents.cpp
        include/actcraft/input/routing.h
        src/routing.cpp
        include/actcraft/input/gesture.h
        src/gesture.cpp
    DEPENDS
        ActCraft::core Qt6::Gui)
