# Ajoute un test Qt Test : actcraft_add_test(<nom> SOURCES ... LIBS ...)
function(actcraft_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})
    qt_add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE Qt6::Test ${ARG_LIBS})
    actcraft_set_warnings(${name})
    add_test(NAME ${name} COMMAND ${name})
    # Sans écran : fenêtres hors écran, rendu logiciel (le secours doit toujours marcher).
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
endfunction()

actcraft_add_test(tst_modules
    SOURCES tst_modules.cpp
    LIBS ActCraft::core ActCraft::formats ActCraft::actions ActCraft::input
         ActCraft::render ActCraft::tools ActCraft::features ActCraft::bench)

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
include(${CMAKE_CURRENT_SOURCE_DIR}/render.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/input.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/tools.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/ui.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/bench.cmake)
