# Tests du module input : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).

actcraft_add_test(tst_input
    SOURCES tst_input.cpp
    LIBS ActCraft::input Qt6::Gui)

actcraft_add_test(tst_input_routing
    SOURCES tst_input_routing.cpp
    LIBS ActCraft::input Qt6::Gui)

actcraft_add_test(tst_input_gesture
    SOURCES tst_input_gesture.cpp
    LIBS ActCraft::input Qt6::Gui)

# Lot 11 (L11-T1) : tests réservés à l'origine par des talons QSKIP, remplis par les tâches nommées (plus aucun talon).
actcraft_add_test(tst_input_touchmodes
    SOURCES tst_input_touchmodes.cpp
    LIBS ActCraft::input Qt6::Gui)

actcraft_add_test(tst_input_twousers
    SOURCES tst_input_twousers.cpp
    LIBS ActCraft::input ActCraft::ui Qt6::Gui)
