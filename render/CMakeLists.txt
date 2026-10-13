# Un moteur de rendu par type d'objet (nœuds du graphe de scène Qt Quick), choisi dans un registre.
actcraft_add_module(render
    SOURCES
        src/render.cpp
    DEPENDS
        ActCraft::core Qt6::Quick)
