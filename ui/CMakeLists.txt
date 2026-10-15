# Interface QML : boîte à outils, navigateurs, menus, thèmes.
qt_add_library(actcraft_ui STATIC)
add_library(ActCraft::ui ALIAS actcraft_ui)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
# Le code C++ généré pour ce dialogue déclenche C4702 (code inatteignable, dans les en-têtes de Qt) sous /W4 /WX :
# il est chargé depuis sa source QML, sans précompilation (écran rarement ouvert).
set_source_files_properties(qml/AboutDialog.qml PROPERTIES QT_QML_SKIP_CACHEGEN TRUE)

# Même défaut (C4702) pour trois composants de la coquille qui appellent Commands.trigger : chargés depuis leur
# source QML. Les composants du chemin de dessin (page, vue, encre) restent précompilés.
set_source_files_properties(qml/TopBar.qml qml/CommandButton.qml qml/WidthPicker.qml
    PROPERTIES QT_QML_SKIP_CACHEGEN TRUE)

qt_add_qml_module(actcraft_ui
    URI ActCraft.Ui
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/AboutDialog.qml
        qml/TopBar.qml
        qml/ToolPalette.qml
        qml/PageArea.qml
        qml/CommandButton.qml
        qml/CommandMenu.qml
        qml/ColorSwatches.qml
        qml/WidthPicker.qml
    RESOURCES
        icons/menu.svg
        icons/outils.svg
        icons/page-precedente.svg
        icons/page-suivante.svg
        icons/stylo.svg
        icons/plein-ecran.svg
        icons/ajuster.svg
        icons/quitter.svg
        icons/aide.svg
        icons/a-propos.svg
    SOURCES
        include/actcraft/ui/viewtransform.h
        include/actcraft/ui/pageview.h
        include/actcraft/ui/documentcontroller.h
        include/actcraft/ui/commands.h
        src/uiforeigntypes.h
        src/penadapters.h
        src/penadapters.cpp
        src/viewtransform.cpp
        src/pageview.cpp
        src/documentcontroller.cpp
        src/commands.cpp
        include/actcraft/ui/appinfo.h
        src/appinfo.cpp)

# Le code d'enregistrement QML généré inclut les en-têtes par leur seul nom : d'où le dossier privé.
target_include_directories(actcraft_ui
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/actcraft/ui ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(actcraft_ui PUBLIC
    Qt6::Quick ActCraft::actions ActCraft::input ActCraft::render ActCraft::tools)
# PageView distingue les boutons propres d'un élément de ceux qu'ajoutent ses gestionnaires de pointeur : l'API
# publique les confond (pageview.cpp, takesPress).
find_package(Qt6 6.12 REQUIRED COMPONENTS QuickPrivate)
target_link_libraries(actcraft_ui PRIVATE Qt6::QuickPrivate)
actcraft_set_warnings(actcraft_ui)

# Version affichée par « À propos » (décision 31).
target_compile_definitions(actcraft_ui PRIVATE ACTCRAFT_VERSION="${PROJECT_VERSION}")

# Notices des composants tiers, affichées par « À propos » et livrées dans les paquets (décision 34).
set_source_files_properties(${PROJECT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md
    PROPERTIES QT_RESOURCE_ALIAS THIRD_PARTY_NOTICES.md)
qt_add_resources(actcraft_ui "ui_notices"
    PREFIX "/actcraft"
    FILES ${PROJECT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md)
