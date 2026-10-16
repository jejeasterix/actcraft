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
