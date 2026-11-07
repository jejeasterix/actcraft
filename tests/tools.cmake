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

# Lot 5 (L5-T1) : tests réservés (talons QSKIP), remplis par les tâches nommées.
# L5-T12
actcraft_add_test(tst_tools_fill
    SOURCES tst_tools_fill.cpp
    LIBS ActCraft::tools ActCraft::render)
# L5-T12
actcraft_add_test(tst_tools_penshape
    SOURCES tst_tools_penshape.cpp
    LIBS ActCraft::tools)
# L5-T13
actcraft_add_test(tst_tools_connector
    SOURCES tst_tools_connector.cpp
    LIBS ActCraft::tools)
# L5-T16
actcraft_add_test(tst_tools_selectcopy
    SOURCES tst_tools_selectcopy.cpp
    LIBS ActCraft::tools)

# Lot 8 (L8-T1) : talon.
actcraft_add_test(tst_tools_interactivity
    SOURCES tst_tools_interactivity.cpp
    LIBS ActCraft::tools ActCraft::core)

# Lot 11 (L11-T1) : talon QSKIP, rempli par L11-T8 (lie ActCraft::ui si le test crée un ToolHost).
actcraft_add_test(tst_tools_twousers
    SOURCES tst_tools_twousers.cpp
    LIBS ActCraft::tools)
