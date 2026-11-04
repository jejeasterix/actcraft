# Impression par QPrinter (DA-A2), remplie par L5-T27. renderSheet (feuille composée par le peintre partagé) est
# construite partout : l'aperçu s'en sert. Qt PrintSupport n'est lié que sous ACTCRAFT_PRINT, en PRIVATE : aucun
# en-tête public de print/ n'inclut un en-tête de Qt PrintSupport. Il tire QtWidgets ; l'application reste une
# QGuiApplication (worksWithoutQApplication).
actcraft_add_module(print
    SOURCES
        src/printer.cpp
    DEPENDS
        ActCraft::render ActCraft::core Qt6::Gui)

if(ACTCRAFT_PRINT)
    find_package(Qt6 REQUIRED COMPONENTS PrintSupport)
    target_link_libraries(actcraft_print PRIVATE Qt6::PrintSupport)
    target_compile_definitions(actcraft_print PRIVATE ACTCRAFT_PRINT=1)
endif()
