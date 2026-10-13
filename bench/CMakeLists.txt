# Banc de fluidité : temps par image, latence d'encre, mémoire.
actcraft_add_module(bench
    SOURCES
        src/bench.cpp
    DEPENDS
        ActCraft::core ActCraft::render Qt6::Quick)
