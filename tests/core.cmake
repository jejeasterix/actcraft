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

actcraft_add_test(tst_core_pagecache
    SOURCES tst_core_pagecache.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_replace
    SOURCES tst_core_replace.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_hittest
    SOURCES tst_core_hittest.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_erase
    SOURCES tst_core_erase.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_objectedits
    SOURCES tst_core_objectedits.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_lift
    SOURCES tst_core_lift.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_shapes
    SOURCES tst_core_shapes.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_gridsnap
    SOURCES tst_core_gridsnap.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_pagesettings
    SOURCES tst_core_pagesettings.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_pageedits
    SOURCES tst_core_pageedits.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_textedits
    SOURCES tst_core_textedits.cpp
    LIBS ActCraft::core)

actcraft_add_test(tst_core_zones
    SOURCES tst_core_zones.cpp
    LIBS ActCraft::core)
