# Tests du module tools : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).

actcraft_add_test(tst_pentool
    SOURCES tst_pentool.cpp
    LIBS ActCraft::tools)

actcraft_add_test(tst_tools_registry
    SOURCES tst_tools_registry.cpp
    LIBS ActCraft::tools)

actcraft_add_test(tst_tools_eraser
    SOURCES tst_tools_eraser.cpp
    LIBS ActCraft::tools)

actcraft_add_test(tst_tools_select
    SOURCES tst_tools_select.cpp
    LIBS ActCraft::tools)

actcraft_add_test(tst_tools_shape
    SOURCES tst_tools_shape.cpp
    LIBS ActCraft::tools)

actcraft_add_test(tst_tools_text
    SOURCES tst_tools_text.cpp
    LIBS ActCraft::tools)
