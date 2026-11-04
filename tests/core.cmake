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

# Lot 5 (L5-T1) : tests réservés (talons QSKIP), remplis par les tâches nommées.
# L5-T4
actcraft_add_test(tst_core_clipboardedits
    SOURCES tst_core_clipboardedits.cpp
    LIBS ActCraft::core)
# L5-T5
actcraft_add_test(tst_core_arrange
    SOURCES tst_core_arrange.cpp
    LIBS ActCraft::core)
# L5-T6
actcraft_add_test(tst_core_alignment
    SOURCES tst_core_alignment.cpp
    LIBS ActCraft::core)
# L5-T7
actcraft_add_test(tst_core_objectflags
    SOURCES tst_core_objectflags.cpp
    LIBS ActCraft::core)
# L5-T8
actcraft_add_test(tst_core_fill
    SOURCES tst_core_fill.cpp
    LIBS ActCraft::core ActCraft::render)
# L5-T9
actcraft_add_test(tst_core_shapepoints
    SOURCES tst_core_shapepoints.cpp
    LIBS ActCraft::core)
# L5-T10
actcraft_add_test(tst_core_connectors
    SOURCES tst_core_connectors.cpp
    LIBS ActCraft::core)
# L5-T11
actcraft_add_test(tst_core_penmodifiers
    SOURCES tst_core_penmodifiers.cpp
    LIBS ActCraft::core)
# L5-T26
actcraft_add_test(tst_core_printlayout
    SOURCES tst_core_printlayout.cpp
    LIBS ActCraft::core)
