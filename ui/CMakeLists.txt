# Interface QML : boîte à outils, navigateurs, menus, thèmes.
qt_add_library(actcraft_ui STATIC)
add_library(ActCraft::ui ALIAS actcraft_ui)

qt_add_qml_module(actcraft_ui
    URI ActCraft.Ui
    VERSION 1.0
    QML_FILES
        qml/Main.qml)

target_link_libraries(actcraft_ui PUBLIC Qt6::Quick ActCraft::actions)
actcraft_set_warnings(actcraft_ui)
