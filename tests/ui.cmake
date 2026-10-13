# Tests du module ui : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).

actcraft_add_test(tst_viewtransform
    SOURCES tst_viewtransform.cpp
    LIBS ActCraft::ui)

actcraft_add_test(tst_pageview
    SOURCES tst_pageview.cpp
    LIBS ActCraft::ui Qt6::Quick)
