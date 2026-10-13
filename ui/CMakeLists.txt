# Interface QML : boîte à outils, navigateurs, menus, thèmes.
qt_add_library(actcraft_ui STATIC)
add_library(ActCraft::ui ALIAS actcraft_ui)

qt_add_qml_module(actcraft_ui
    URI ActCraft.Ui
    VERSION 1.0
    QML_FILES
        qml/Main.qml
    SOURCES
        include/actcraft/ui/viewtransform.h
        include/actcraft/ui/pageview.h
        src/viewtransform.cpp
        src/pageview.cpp)

# Le code d'enregistrement QML généré inclut les en-têtes par leur seul nom : d'où le dossier privé.
target_include_directories(actcraft_ui
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/actcraft/ui)
target_link_libraries(actcraft_ui PUBLIC Qt6::Quick ActCraft::actions)
actcraft_set_warnings(actcraft_ui)
