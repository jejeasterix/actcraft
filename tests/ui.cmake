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
