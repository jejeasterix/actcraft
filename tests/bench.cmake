# Tests du module bench : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).
if(NOT ACTCRAFT_BUILD_BENCH)
    return()
endif()

actcraft_add_test(tst_bench_stats
    SOURCES tst_bench_stats.cpp
    LIBS ActCraft::bench)

actcraft_add_test(tst_bench_json
    SOURCES tst_bench_json.cpp
    LIBS ActCraft::bench)

actcraft_add_test(tst_bench_memory
    SOURCES tst_bench_memory.cpp
    LIBS ActCraft::bench Qt6::Quick)

actcraft_add_test(tst_bench_scenarios
    SOURCES tst_bench_scenarios.cpp
    LIBS ActCraft::bench ActCraft::ui)

actcraft_add_test(tst_bench_injector
    SOURCES tst_bench_injector.cpp
    LIBS ActCraft::bench Qt6::Gui)

actcraft_add_test(tst_bench_options
    SOURCES tst_bench_options.cpp
    LIBS ActCraft::bench)

# Verdict et validité ; les contrôles de la chaîne réelle chargent la vraie interface (startup.cpp).
actcraft_add_test(tst_bench_verdict
    SOURCES tst_bench_verdict.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS ActCraft::bench ActCraft::ui actcraft_uiplugin Qt6::Quick)
target_include_directories(tst_bench_verdict PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_bench_content
    SOURCES tst_bench_content.cpp
    LIBS ActCraft::bench)
