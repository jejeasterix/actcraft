# Tests du module render : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).

actcraft_add_test(tst_render_engines
    SOURCES tst_render_engines.cpp
    LIBS ActCraft::render Qt6::Quick)

actcraft_add_test(tst_render_images
    SOURCES tst_render_images.cpp
    LIBS ActCraft::render Qt6::Quick)

actcraft_add_test(tst_render_fixedpage
    SOURCES tst_render_fixedpage.cpp
    LIBS ActCraft::render Qt6::Quick)
