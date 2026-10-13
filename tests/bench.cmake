# Tests du module bench : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).
if(NOT ACTCRAFT_BUILD_BENCH)
    return()
endif()

actcraft_add_test(tst_bench_stats
    SOURCES tst_bench_stats.cpp
    LIBS ActCraft::bench)
