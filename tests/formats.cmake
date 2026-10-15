# Tests du format natif (DA3) : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).

actcraft_add_test(tst_third_party_miniz
    SOURCES tst_third_party_miniz.cpp
    LIBS ActCraft::miniz)
