# Tests du module actions : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).

actcraft_add_test(tst_actions_registry
    SOURCES tst_actions_registry.cpp
    LIBS ActCraft::actions)

actcraft_add_test(tst_actions_layout
    SOURCES tst_actions_layout.cpp
    LIBS ActCraft::actions)

# Lot 6 (L6-T1) : tests réservés (talons QSKIP), remplis par les tâches nommées.
# L6-T4
actcraft_add_test(tst_actions_profile
    SOURCES tst_actions_profile.cpp
    LIBS ActCraft::actions)
# L6-T3
actcraft_add_test(tst_actions_lockable
    SOURCES tst_actions_lockable.cpp
    LIBS ActCraft::actions)
