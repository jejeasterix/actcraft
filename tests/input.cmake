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
