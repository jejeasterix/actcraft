# Fonctions activables ou retirables : règle, rapporteur, dés, horloge, rideau, spot…
actcraft_add_module(features
    SOURCES
        src/features.cpp
    DEPENDS
        ActCraft::core ActCraft::actions ActCraft::render)
