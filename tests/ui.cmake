# Tests du module ui : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).

actcraft_add_test(tst_viewtransform
    SOURCES tst_viewtransform.cpp
    LIBS ActCraft::ui)

actcraft_add_test(tst_pageview
    SOURCES tst_pageview.cpp
    LIBS ActCraft::ui Qt6::Quick)

actcraft_add_test(tst_ui_drawing
    SOURCES tst_ui_drawing.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_drawing PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_ui_theme
    SOURCES tst_ui_theme.cpp
    LIBS Qt6::Quick Qt6::Svg ActCraft::ui actcraft_uiplugin)

actcraft_add_test(tst_ui_document
    SOURCES tst_ui_document.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)

actcraft_add_test(tst_ui_commands
    SOURCES tst_ui_commands.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)

actcraft_add_test(tst_ui_about
    SOURCES tst_ui_about.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_compile_definitions(tst_ui_about PRIVATE ACTCRAFT_TEST_VERSION="${PROJECT_VERSION}")

actcraft_add_test(tst_ui_shell
    SOURCES tst_ui_shell.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_shell PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_ui_police
    SOURCES tst_ui_police.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)

# Ouvrir et enregistrer hors du fil principal (L2-T17).
actcraft_add_test(tst_ui_files
    SOURCES tst_ui_files.cpp
    LIBS Qt6::Quick ActCraft::ui ActCraft::formats)
if(WIN32)
    target_link_libraries(tst_ui_files PRIVATE psapi) # GetProcessMemoryInfo (peakmemory.h)
endif()

# Menu Fichier, dialogues, demande d'enregistrement, bandeau (L2-T18).
actcraft_add_test(tst_ui_filemenu
    SOURCES tst_ui_filemenu.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick Qt6::Gui ActCraft::ui ActCraft::formats actcraft_uiplugin)
target_include_directories(tst_ui_filemenu PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_ui_lift
    SOURCES tst_ui_lift.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)

actcraft_add_test(tst_ui_tools
    SOURCES tst_ui_tools.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)

actcraft_add_test(tst_ui_inputpath
    SOURCES tst_ui_inputpath.cpp
    LIBS ActCraft::ui Qt6::Quick actcraft_uiplugin)

actcraft_add_test(tst_ui_shapes
    SOURCES tst_ui_shapes.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)

actcraft_add_test(tst_ui_pagesettings
    SOURCES tst_ui_pagesettings.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)

actcraft_add_test(tst_ui_pagethumbnails
    SOURCES tst_ui_pagethumbnails.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)

actcraft_add_test(tst_ui_pages
    SOURCES tst_ui_pages.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)

actcraft_add_test(tst_ui_text
    SOURCES tst_ui_text.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)

# Les fenêtres de choix de la barre de texte se referment par un second appui sur leur bouton (L4-C20).
actcraft_add_test(tst_ui_textformatpopup
    SOURCES tst_ui_textformatpopup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)

# Échap ferme une boîte modale au lieu de passer à l'outil Sélectionner (L4-C6).
actcraft_add_test(tst_ui_escape
    SOURCES tst_ui_escape.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)

# Lot 5 (L5-T1) : tests des tâches nommées.
# L5-T14
actcraft_add_test(tst_ui_objectediting
    SOURCES tst_ui_objectediting.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L5-T15
actcraft_add_test(tst_ui_insertion
    SOURCES tst_ui_insertion.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L5-T17
actcraft_add_test(tst_ui_lot5tools
    SOURCES tst_ui_lot5tools.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L5-T20
actcraft_add_test(tst_ui_tabs
    SOURCES tst_ui_tabs.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L5-T25
actcraft_add_test(tst_ui_fileexchange
    SOURCES tst_ui_fileexchange.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_fileexchange PRIVATE ${PROJECT_SOURCE_DIR}/app)
# L5-T28
actcraft_add_test(tst_ui_printdialog
    SOURCES tst_ui_printdialog.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L5-T30
actcraft_add_test(tst_ui_import
    SOURCES tst_ui_import.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_import PRIVATE ${PROJECT_SOURCE_DIR}/app)
target_compile_definitions(tst_ui_import PRIVATE
    ACTCRAFT_CMAKE="${CMAKE_COMMAND}"
    ACTCRAFT_IMPORT_DATA="${PROJECT_SOURCE_DIR}/tests/data/import")

# Lot 6 (L6-T1) : tests des tâches nommées.
# L6-T5
actcraft_add_test(tst_ui_preferences
    SOURCES tst_ui_preferences.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T6
actcraft_add_test(tst_ui_profiles
    SOURCES tst_ui_profiles.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T7
actcraft_add_test(tst_ui_palette
    SOURCES tst_ui_palette.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_palette PRIVATE ${PROJECT_SOURCE_DIR}/app)
# L6-T8
actcraft_add_test(tst_ui_colors
    SOURCES tst_ui_colors.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T9
actcraft_add_test(tst_ui_shortcutbar
    SOURCES tst_ui_shortcutbar.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T12
actcraft_add_test(tst_ui_propertymodel
    SOURCES tst_ui_propertymodel.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T14
actcraft_add_test(tst_ui_objecttree
    SOURCES tst_ui_objecttree.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T16
actcraft_add_test(tst_ui_notes
    SOURCES tst_ui_notes.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T17
actcraft_add_test(tst_ui_labels
    SOURCES tst_ui_labels.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T20
actcraft_add_test(tst_ui_resources
    SOURCES tst_ui_resources.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T22
actcraft_add_test(tst_ui_designmode
    SOURCES tst_ui_designmode.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T24
actcraft_add_test(tst_ui_recents
    SOURCES tst_ui_recents.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T25
actcraft_add_test(tst_ui_profileeditor
    SOURCES tst_ui_profileeditor.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T26
actcraft_add_test(tst_ui_layouteditor
    SOURCES tst_ui_layouteditor.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T27
actcraft_add_test(tst_ui_settingsschema
    SOURCES tst_ui_settingsschema.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T28
actcraft_add_test(tst_ui_network
    SOURCES tst_ui_network.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T29
actcraft_add_test(tst_ui_launchers
    SOURCES tst_ui_launchers.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T30
actcraft_add_test(tst_ui_launcherspage
    SOURCES tst_ui_launcherspage.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
add_dependencies(tst_ui_launcherspage launcher_probe)
# L6-T31
actcraft_add_test(tst_ui_classroomlock
    SOURCES tst_ui_classroomlock.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T34
actcraft_add_test(tst_ui_spelling
    SOURCES tst_ui_spelling.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T35
actcraft_add_test(tst_ui_help
    SOURCES tst_ui_help.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
# L6-T36
actcraft_add_test(tst_ui_language
    SOURCES tst_ui_language.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_compile_definitions(tst_ui_language PRIVATE ACTCRAFT_LABELS_SRC="${PROJECT_SOURCE_DIR}")

# Pseudo-langue de contrôle (L11-T19, D11-14) : `build/<preset>/i18n/actcraft_xx.qm`, produite au build des tests
# seulement (scripts/pseudo-langue.py puis lrelease) ; jamais dans les ressources ni dans les paquets.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(ACTCRAFT_PSEUDO_TS ${CMAKE_BINARY_DIR}/i18n/actcraft_xx.ts)
set(ACTCRAFT_PSEUDO_QM ${CMAKE_BINARY_DIR}/i18n/actcraft_xx.qm)
set(ACTCRAFT_FR_TS ${PROJECT_SOURCE_DIR}/ui/i18n/actcraft_fr.ts)
add_custom_command(
    OUTPUT ${ACTCRAFT_PSEUDO_QM}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/i18n
    COMMAND ${Python3_EXECUTABLE} -I ${PROJECT_SOURCE_DIR}/scripts/pseudo-langue.py
            ${ACTCRAFT_FR_TS} ${ACTCRAFT_PSEUDO_TS}
    COMMAND $<TARGET_FILE:Qt6::lrelease> ${ACTCRAFT_PSEUDO_TS} -qm ${ACTCRAFT_PSEUDO_QM}
    DEPENDS ${ACTCRAFT_FR_TS} ${PROJECT_SOURCE_DIR}/scripts/pseudo-langue.py
    VERBATIM)
add_custom_target(actcraft_pseudolocale DEPENDS ${ACTCRAFT_PSEUDO_QM})

# Programme d'essai des raccourcis vers des programmes (L6-T29).
add_executable(launcher_probe launcher_probe.cpp)
actcraft_set_warnings(launcher_probe)

# Lot 8 (L8-T1) : talons.
actcraft_add_test(tst_ui_actionrunner
    SOURCES tst_ui_actionrunner.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
actcraft_add_test(tst_ui_linklauncher
    SOURCES tst_ui_linklauncher.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
actcraft_add_test(tst_ui_actionbrowser
    SOURCES tst_ui_actionbrowser.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
actcraft_add_test(tst_ui_dragdropactions
    SOURCES tst_ui_dragdropactions.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
actcraft_add_test(tst_ui_interactivityprops
    SOURCES tst_ui_interactivityprops.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
actcraft_add_test(tst_formats_samples_interactivity
    SOURCES tst_formats_samples_interactivity.cpp
    LIBS ActCraft::formats ActCraft::tools ActCraft::ui)
# Paperboards réels : dossier transmis par l'environnement quand il existe (sinon le test se saute).
if(DEFINED ENV{ACTCRAFT_SAMPLES_DIR})
    set_property(TEST tst_formats_samples_interactivity APPEND PROPERTY ENVIRONMENT "ACTCRAFT_SAMPLES_DIR=$ENV{ACTCRAFT_SAMPLES_DIR}")
endif()

# Lot 8 (L8-T21) : talons de 8b.
actcraft_add_test(tst_ui_linkdialogs
    SOURCES tst_ui_linkdialogs.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
actcraft_add_test(tst_ui_mediainsert
    SOURCES tst_ui_mediainsert.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
actcraft_add_test(tst_ui_mediaitems
    SOURCES tst_ui_mediaitems.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
actcraft_add_test(tst_ui_recorders
    SOURCES tst_ui_recorders.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
actcraft_add_test(tst_ui_webpolicy
    SOURCES tst_ui_webpolicy.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
actcraft_add_test(tst_ui_webframe
    SOURCES tst_ui_webframe.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
actcraft_add_test(tst_ui_broadcast
    SOURCES tst_ui_broadcast.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_compile_definitions(tst_ui_webpolicy PRIVATE ACTCRAFT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

# Lot 11 (L11-T1) : talons QSKIP, remplis par les tâches nommées dans leur message.
actcraft_add_test(tst_ui_pageinput
    SOURCES tst_ui_pageinput.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_pageinput PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_ui_touchmode
    SOURCES tst_ui_touchmode.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_touchmode PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_ui_secondpalette
    SOURCES tst_ui_secondpalette.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_secondpalette PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_ui_narrowlayout
    SOURCES tst_ui_narrowlayout.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_narrowlayout PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_ui_orientation
    SOURCES tst_ui_orientation.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_orientation PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_ui_fingermenus
    SOURCES tst_ui_fingermenus.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_fingermenus PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_ui_lifecycle
    SOURCES tst_ui_lifecycle.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_lifecycle PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_ui_updatechecker
    SOURCES tst_ui_updatechecker.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_updatechecker PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_ui_shortcuts
    SOURCES tst_ui_shortcuts.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_shortcuts PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_ui_accessibility
    SOURCES tst_ui_accessibility.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_accessibility PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_ui_lockall
    SOURCES tst_ui_lockall.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_lockall PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_ui_profilesall
    SOURCES tst_ui_profilesall.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_profilesall PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_ui_documentportal
    SOURCES tst_ui_documentportal.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)

actcraft_add_test(tst_ui_pseudolocale
    SOURCES tst_ui_pseudolocale.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)
add_dependencies(tst_ui_pseudolocale actcraft_pseudolocale)
target_compile_definitions(tst_ui_pseudolocale PRIVATE
    ACTCRAFT_PSEUDO_TS="${ACTCRAFT_PSEUDO_TS}"
    ACTCRAFT_PSEUDO_QM="${ACTCRAFT_PSEUDO_QM}"
    ACTCRAFT_FR_TS="${ACTCRAFT_FR_TS}")

actcraft_add_test(tst_ui_helppages
    SOURCES tst_ui_helppages.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)

