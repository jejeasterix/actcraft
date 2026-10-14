# Tests du module actions : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).

actcraft_add_test(tst_actions_registry
    SOURCES tst_actions_registry.cpp
    LIBS ActCraft::actions)

actcraft_add_test(tst_actions_layout
    SOURCES tst_actions_layout.cpp
    LIBS ActCraft::actions)
