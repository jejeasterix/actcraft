# Banc de fluidité : temps par image, latence d'encre, mémoire.
if(WIN32)
    set(_bench_gpu_source src/gpumemory_win.cpp)
elseif(APPLE)
    set(_bench_gpu_source src/gpumemory_mac.mm)
else()
    set(_bench_gpu_source src/gpumemory_other.cpp)
endif()

actcraft_add_module(bench
    SOURCES
        src/bench.cpp
        src/stats.cpp
        src/memory.cpp
        src/machineinfo.cpp
        src/result.cpp
        ${_bench_gpu_source}
    DEPENDS
        ActCraft::core ActCraft::render Qt6::Quick)

if(WIN32)
    target_link_libraries(actcraft_bench PRIVATE dxgi psapi)
elseif(APPLE)
    target_link_libraries(actcraft_bench PRIVATE "-framework Metal" "-framework Foundation")
endif()

# Métadonnées de construction écrites dans le JSON (décisions 27 et 28), relues à chaque configuration.
execute_process(COMMAND git rev-parse --short=12 HEAD
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE ACTCRAFT_GIT_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(NOT ACTCRAFT_GIT_COMMIT)
    set(ACTCRAFT_GIT_COMMIT "inconnu")
endif()
execute_process(COMMAND git status --porcelain --untracked-files=no
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE _bench_git_status OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
if(_bench_git_status)
    set(ACTCRAFT_GIT_DIRTY 1)
else()
    set(ACTCRAFT_GIT_DIRTY 0)
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" _bench_build_type)
string(STRIP "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_${_bench_build_type}}" ACTCRAFT_CXX_FLAGS)
configure_file(src/buildinfo.h.in ${CMAKE_CURRENT_BINARY_DIR}/generated/buildinfo.h @ONLY)
target_include_directories(actcraft_bench PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
