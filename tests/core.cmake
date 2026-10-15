# Tests du module core : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).

actcraft_add_test(tst_core_model
    SOURCES tst_core_model.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_generator
    SOURCES tst_core_generator.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_document
    SOURCES tst_core_document.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_undo
    SOURCES tst_core_undo.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_edits
    SOURCES tst_core_edits.cpp
    LIBS ActCraft::core)
