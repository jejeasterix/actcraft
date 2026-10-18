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

# Même défaut (C4702) pour les commandes de fichier et leurs dialogues (L2-T18) : chargés depuis leur source.
set_source_files_properties(qml/FileActions.qml qml/SaveChangesDialog.qml qml/PageSizeDialog.qml
    PROPERTIES QT_QML_SKIP_CACHEGEN TRUE)

# Même défaut (C4702) pour les composants de la coquille du socle 3-7 (L0-S0) : chargés depuis leur source.
set_source_files_properties(qml/SelectionOverlay.qml qml/ObjectBar.qml qml/TrashCan.qml qml/PageContextMenu.qml
    qml/TranslucencyPopup.qml qml/Presentation.qml
    PROPERTIES QT_QML_SKIP_CACHEGEN TRUE)

# Lot 7 (présentation et outils) : ses sources QML et C++ sont listées dans ui/presentation.cmake (fichier de la
# session B) ; ce fichier-ci n'est plus modifié pour elles. Presentation.qml est déclaré ici (socle) : ne pas le
# relister dans presentation.cmake.
set(ACTCRAFT_UI_LOT7_QML "")
set(ACTCRAFT_UI_LOT7_SOURCES "")
include(${CMAKE_CURRENT_SOURCE_DIR}/presentation.cmake OPTIONAL)

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
        qml/ItemHighlight.qml
        qml/ScrollThumb.qml
        qml/FileActions.qml
        qml/SaveChangesDialog.qml
        qml/PageSizeDialog.qml
        qml/MessageBanner.qml
        qml/SelectionOverlay.qml
        qml/ObjectBar.qml
        qml/TranslucencyPopup.qml
        qml/TrashCan.qml
        qml/PageContextMenu.qml
        qml/ToolCursor.qml
        qml/Presentation.qml
        ${ACTCRAFT_UI_LOT7_QML}
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
        icons/selection.svg
        icons/marqueur.svg
        icons/gomme.svg
        icons/annuler.svg
        icons/retablir.svg
        icons/supprimer.svg
        icons/corbeille.svg
        icons/effacer.svg
        icons/deplacer.svg
        icons/rotation.svg
        icons/translucidite.svg
        icons/agrandir.svg
        icons/reduire.svg
        icons/rideau.svg
        icons/spot.svg
        icons/zoom-page.svg
        icons/effets.svg
        icons/regle.svg
        icons/equerre.svg
        icons/rapporteur.svg
        icons/compas.svg
        icons/origine.svg
        icons/des.svg
        icons/calculatrice.svg
        icons/horloge.svg
        icons/message.svg
        icons/forme-reconnue.svg
        icons/ecriture.svg
        icons/camera.svg
        icons/clavier.svg
        icons/bureau.svg
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
        include/actcraft/ui/documentio.h
        src/documentio.cpp
        src/commands.cpp
        include/actcraft/ui/appinfo.h
        src/appinfo.cpp
        include/actcraft/ui/toolhost.h
        src/toolhost.cpp
        src/tooladapters.h
        src/tooladapters.cpp
        ${ACTCRAFT_UI_LOT7_SOURCES})

# Le code d'enregistrement QML généré inclut les en-têtes par leur seul nom : d'où le dossier privé.
target_include_directories(actcraft_ui
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/actcraft/ui ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(actcraft_ui PUBLIC
    Qt6::Quick ActCraft::actions ActCraft::formats ActCraft::input ActCraft::render ActCraft::tools
    ActCraft::features)
actcraft_set_warnings(actcraft_ui)

# Version affichée par « À propos » (décision 31).
target_compile_definitions(actcraft_ui PRIVATE ACTCRAFT_VERSION="${PROJECT_VERSION}")

# Notices des composants tiers, affichées par « À propos » et livrées dans les paquets (décision 34).
set_source_files_properties(${PROJECT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md
    PROPERTIES QT_RESOURCE_ALIAS THIRD_PARTY_NOTICES.md)
qt_add_resources(actcraft_ui "ui_notices"
    PREFIX "/actcraft"
    FILES ${PROJECT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md)
