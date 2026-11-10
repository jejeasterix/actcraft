# Windows : la sortie de Qt Test et le code de sortie de chaque test vont dans <build>/journaux-tests/<test>.txt
# (tests/lanceur_test_windows.cpp) ; repris par l'artefact journaux-tests (scripts/ci/journaux-tests.cmake).
if(WIN32)
    add_executable(actcraft_test_launcher lanceur_test_windows.cpp)
    actcraft_set_warnings(actcraft_test_launcher)
endif()

# Entrée ctest d'un exécutable Qt Test ; sous Windows, par le lanceur. Tout `add_test` d'un exécutable Qt Test passe ici.
function(actcraft_register_test name target)
    if(WIN32)
        add_test(NAME ${name} COMMAND actcraft_test_launcher "${CMAKE_BINARY_DIR}/journaux-tests/${name}.txt"
            $<TARGET_FILE:${target}> ${ARGN})
        add_dependencies(${target} actcraft_test_launcher)
    else()
        add_test(NAME ${name} COMMAND ${target} ${ARGN})
    endif()
endfunction()

# Ajoute un test Qt Test : actcraft_add_test(<nom> SOURCES ... LIBS ... [ARGS ...]) ; ARGS : arguments de Qt Test.
function(actcraft_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS;ARGS" ${ARGN})
    qt_add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE Qt6::Test ${ARG_LIBS})
    actcraft_set_warnings(${name})
    actcraft_register_test(${name} ${name} ${ARG_ARGS})
    # Sans écran : fenêtres hors écran, rendu logiciel (le secours doit toujours marcher).
    set(env_test "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
    if(WIN32)
        # La plateforme hors écran de Qt n'a pas de polices sous Windows (elle cherche <Qt>/lib/fonts) : tous les
        # textes deviennent des carrés vides. On lui montre le dossier des polices du système (L1-C2).
        list(APPEND env_test "QT_QPA_FONTDIR=$ENV{WINDIR}/Fonts")
    endif()
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "${env_test}")
endfunction()

actcraft_add_test(tst_modules
    SOURCES tst_modules.cpp
    LIBS ActCraft::core ActCraft::formats ActCraft::actions ActCraft::input
         ActCraft::render ActCraft::tools ActCraft::features ActCraft::bench)
# Lot 8 (L8-T21) : le module media se lie avec les autres quand il est construit.
if(ACTCRAFT_MEDIA)
    target_link_libraries(tst_modules PRIVATE ActCraft::media)
endif()

actcraft_add_test(tst_renderbackend
    SOURCES tst_renderbackend.cpp ${PROJECT_SOURCE_DIR}/app/renderbackend.cpp
    LIBS Qt6::Quick)
target_include_directories(tst_renderbackend PRIVATE ${PROJECT_SOURCE_DIR}/app)

actcraft_add_test(tst_ui
    SOURCES tst_ui.cpp
    LIBS Qt6::Quick ActCraft::ui actcraft_uiplugin)

# Un fichier par module : deux tâches parallèles ne modifient pas le même fichier.
# Chaque test y est déclaré par actcraft_add_test (fonction ci-dessus).
include(${CMAKE_CURRENT_SOURCE_DIR}/verifications.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/core.cmake)
if(ACTCRAFT_MEDIA)
    include(${CMAKE_CURRENT_SOURCE_DIR}/media.cmake)
endif()
include(${CMAKE_CURRENT_SOURCE_DIR}/actions.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/formats.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/flipchart.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/render.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/export.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/print.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/input.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/tools.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/ui.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/bench.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/app.cmake)

# Lot 7 : tests déclarés par la session B dans tests/presentation.cmake (absent tant que B n'est pas fusionnée).
include(${CMAKE_CURRENT_SOURCE_DIR}/presentation.cmake OPTIONAL)
