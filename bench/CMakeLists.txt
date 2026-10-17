# Banc de fluidité : temps par image, latence d'encre, mémoire.
# Joueur de scénarios d'actcraft-captures : QTest (fenêtre active) et QPA (touches avec leur « acceptée »).
find_package(Qt6 6.12 REQUIRED COMPONENTS Test GuiPrivate)

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
        include/actcraft/bench/injector.h
        src/scenarios.cpp
        src/injector.cpp
        include/actcraft/bench/recorder.h
        src/options.cpp
        src/verdict.cpp
        src/contentcheck.cpp
        src/recorder.cpp
        include/actcraft/bench/runner.h
        src/runner.cpp
        ${_bench_gpu_source}
    DEPENDS
        ActCraft::core ActCraft::render ActCraft::ui Qt6::Quick)

if(WIN32)
    target_link_libraries(actcraft_bench PRIVATE dxgi psapi dwmapi)
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

# Exécutable du banc (DA1) : charge la vraie interface ; console, sans paquet macOS.
qt_add_executable(actcraft-bench
    app/main.cpp
    ${PROJECT_SOURCE_DIR}/app/renderbackend.cpp
    ${PROJECT_SOURCE_DIR}/app/startup.cpp)
target_include_directories(actcraft-bench PRIVATE ${PROJECT_SOURCE_DIR}/app)
target_link_libraries(actcraft-bench PRIVATE ActCraft::bench ActCraft::ui actcraft_uiplugin Qt6::Quick)
actcraft_set_warnings(actcraft-bench)
# qt_standard_project_setup ne regroupe les exécutables à la racine du dossier de construction que sous Windows
# (Qt6CoreMacros.cmake) : on l'impose partout, pour que scripts/bench.sh et la CI macOS trouvent
# build/<preset>/actcraft-bench.
set_target_properties(actcraft-bench PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Joueur de scénarios d'interface (DA8) : bibliothèque à part, pour que le banc livré (actcraft_bench,
# actcraft-bench) ne lie ni QtTest ni les en-têtes QPA, sous aucun système.
actcraft_add_module(benchui
    SOURCES
        include/actcraft/bench/uiscenario.h
        src/uiscenario.cpp
        include/actcraft/bench/fixtures.h
        src/fixtures.cpp
    DEPENDS
        ActCraft::bench ActCraft::formats)
target_link_libraries(actcraft_benchui PRIVATE Qt6::Test Qt6::GuiPrivate)
# Fixture « flipchart-synthetique » (L2-T21) : paperboard de notre écrivain de tests, jamais un fichier réel.
target_include_directories(actcraft_benchui PRIVATE ${PROJECT_SOURCE_DIR}/tests/support)

# Captures et scénarios d'interface (DA8) : outil de preuve, jamais livré.
qt_add_executable(actcraft-captures
    captures/main.cpp
    ${PROJECT_SOURCE_DIR}/app/renderbackend.cpp
    ${PROJECT_SOURCE_DIR}/app/startup.cpp)
target_include_directories(actcraft-captures PRIVATE ${PROJECT_SOURCE_DIR}/app)
target_link_libraries(actcraft-captures PRIVATE ActCraft::benchui ActCraft::ui actcraft_uiplugin Qt6::Quick Qt6::Test)
actcraft_set_warnings(actcraft-captures)
set_target_properties(actcraft-captures PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Export canonique d'un .flipchart (oracle du lecteur, décision 19) et miniatures comparées aux .thm (S4,
# L2-T21) : outil de preuve, jamais livré. Peintre partagé (render) ; écrivain synthétique des tests.
qt_add_executable(actcraft-flipchart-dump flipchart/main.cpp)
target_link_libraries(actcraft-flipchart-dump PRIVATE ActCraft::formats ActCraft::render Qt6::Gui)
target_include_directories(actcraft-flipchart-dump PRIVATE ${PROJECT_SOURCE_DIR}/tests/support)
# Racine du dépôt : --comparaisons n'écrit que sous .tmp/ ou build/ (PNG des vraies miniatures).
target_compile_definitions(actcraft-flipchart-dump PRIVATE ACTCRAFT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
actcraft_set_warnings(actcraft-flipchart-dump)
set_target_properties(actcraft-flipchart-dump PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
