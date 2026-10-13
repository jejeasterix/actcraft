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

# Fumée du banc (décision 33) : sans actcraft_add_test pour le lancement, qui construit son propre exécutable.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_bench_smoke_expected "2")
    set(_bench_smoke_valid 0)
else()
    set(_bench_smoke_expected "0,1")
    set(_bench_smoke_valid 1)
endif()
set(_bench_smoke_json ${CMAKE_BINARY_DIR}/bench-smoke.json)

# Efface le JSON avant la fumée : jamais de résultat périmé.
add_test(NAME bench_smoke_setup COMMAND ${CMAKE_COMMAND} -E rm -f ${_bench_smoke_json})
set_tests_properties(bench_smoke_setup PROPERTIES FIXTURES_SETUP bench_smoke_clean)

add_test(NAME tst_bench_smoke
    COMMAND ${CMAKE_COMMAND}
        -DBENCH=$<TARGET_FILE:actcraft-bench>
        -DOUTPUT=${_bench_smoke_json}
        -DEXPECTED=${_bench_smoke_expected}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_bench_smoke.cmake)
set_tests_properties(tst_bench_smoke PROPERTIES
    TIMEOUT 60
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    FIXTURES_REQUIRED bench_smoke_clean
    FIXTURES_SETUP bench_smoke_json)

actcraft_add_test(tst_bench_result
    SOURCES tst_bench_result.cpp
    LIBS Qt6::Core)
target_compile_definitions(tst_bench_result PRIVATE ACTCRAFT_EXPECT_VALID=${_bench_smoke_valid})
set_tests_properties(tst_bench_result PROPERTIES
    FIXTURES_REQUIRED bench_smoke_json
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software;ACTCRAFT_BENCH_SMOKE_JSON=${_bench_smoke_json}")

# Résumé Markdown du banc (S3), sur le JSON de la fumée.
find_program(ACTCRAFT_PWSH pwsh)
if(ACTCRAFT_PWSH)
    # Sortie dans un dossier absent (créé par le script) : on l'efface avant, pour que le test reste rouge sans le correctif.
    set(_bench_summary_dir ${CMAKE_BINARY_DIR}/bench-smoke-resume)
    add_test(NAME bench_summary_setup COMMAND ${CMAKE_COMMAND} -E remove_directory ${_bench_summary_dir})
    set_tests_properties(bench_summary_setup PROPERTIES FIXTURES_SETUP bench_summary_clean)
    add_test(NAME tst_bench_summary
        COMMAND ${ACTCRAFT_PWSH} -NoProfile -File ${PROJECT_SOURCE_DIR}/scripts/resume-banc.ps1
            -Entrees ${_bench_smoke_json} -Sortie ${_bench_summary_dir}/sous/dossier/resume.md)
    set_tests_properties(tst_bench_summary PROPERTIES
        FIXTURES_REQUIRED "bench_smoke_json;bench_summary_clean"
        PASS_REGULAR_EXPRESSION "RESUME OK")
endif()
