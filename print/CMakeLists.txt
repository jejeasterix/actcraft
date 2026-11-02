# Impression par QPrinter (DA-A2). Stubs du lot 5 (L5-T1) ; rempli par L5-T27 (Qt PrintSupport ajouté alors).
actcraft_add_module(print
    SOURCES
        src/printer.cpp
    DEPENDS
        ActCraft::render ActCraft::core Qt6::Gui)
