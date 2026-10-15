# Tests du format natif (DA3) : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).

actcraft_add_test(tst_third_party_miniz
    SOURCES tst_third_party_miniz.cpp
    LIBS ActCraft::miniz)
actcraft_add_test(tst_formats_nativejson
    SOURCES tst_formats_nativejson.cpp
    LIBS ActCraft::formats)
# document.json de référence du document riche : tests/data/nativejson/richdocument.json.
target_compile_definitions(tst_formats_nativejson PRIVATE ACTCRAFT_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
