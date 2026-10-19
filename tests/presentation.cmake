# Tests du lot 7 (présentation et outils) : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).
# Inclus en dernier par tests/CMakeLists.txt (OPTIONAL). Phase 1 (L7-P0) : logique pure de features/, sans écran ;
# la phase 2 ajoute ici les tests d'interface et de captures du lot.
actcraft_add_test(tst_features_curtain SOURCES tst_features_curtain.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_spotlight SOURCES tst_features_spotlight.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_pagetools SOURCES tst_features_pagetools.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_pagezoom SOURCES tst_features_pagezoom.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_transitions SOURCES tst_features_transitions.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_mathtools SOURCES tst_features_mathtools.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_dice SOURCES tst_features_dice.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_calculator SOURCES tst_features_calculator.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_countdown SOURCES tst_features_countdown.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_ticker SOURCES tst_features_ticker.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_shapefinder SOURCES tst_features_shapefinder.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_recognitionsession SOURCES tst_features_recognitionsession.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_handwriting SOURCES tst_features_handwriting.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_camera SOURCES tst_features_camera.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_keyboard SOURCES tst_features_keyboard.cpp LIBS ActCraft::features)
actcraft_add_test(tst_features_desktopwheel SOURCES tst_features_desktopwheel.cpp LIBS ActCraft::features)

# Phase 2 (L7-S8) : tests d'interface, de logique de l'équation et de captures du lot 7, déclarés ici seulement ;
# aucune autre tâche n'en ajoute (compte figé : N = N_F + 29). Les chemins d'inclusion privés dont les tâches ont
# besoin sont posés ici une fois pour toutes (ui/src, render/src).
set(_lot7_ui_keys presentation curtain spotlight pagezoom pagetools transitions mathtools dice calculator clock
                  othertools ticker recognition equation camera keyboard desktop)
set(_lot7_ui_sources tst_ui_lot7_main.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp)
foreach(_key IN LISTS _lot7_ui_keys)
    list(APPEND _lot7_ui_sources tst_ui_${_key}.cpp)
endforeach()
# Un seul exécutable pour les 17 classes (un lien, un PDB) ; 17 entrées ctest à leur nom.
qt_add_executable(tst_ui_lot7 ${_lot7_ui_sources})
target_link_libraries(tst_ui_lot7 PRIVATE Qt6::Test Qt6::Quick ActCraft::ui actcraft_uiplugin)
target_include_directories(tst_ui_lot7 PRIVATE ${PROJECT_SOURCE_DIR}/app ${PROJECT_SOURCE_DIR}/ui/src
                                               ${CMAKE_CURRENT_SOURCE_DIR})
actcraft_set_warnings(tst_ui_lot7)
# Même environnement que les tests d'interface du socle, lu et non recopié (tests/ui.cmake inclus avant).
get_test_property(tst_ui_shell ENVIRONMENT _lot7_env)
foreach(_key IN LISTS _lot7_ui_keys)
    add_test(NAME tst_ui_${_key} COMMAND tst_ui_lot7 ${_key})
    set_tests_properties(tst_ui_${_key} PROPERTIES ENVIRONMENT "${_lot7_env}")
endforeach()

actcraft_add_test(tst_core_equation SOURCES tst_core_equation.cpp LIBS ActCraft::core)
actcraft_add_test(tst_render_equation SOURCES tst_render_equation.cpp LIBS ActCraft::render Qt6::Gui)
target_include_directories(tst_render_equation PRIVATE ${PROJECT_SOURCE_DIR}/render/src)
actcraft_add_test(tst_features_equationeditor SOURCES tst_features_equationeditor.cpp LIBS ActCraft::features)

foreach(_lot7_scenario IN ITEMS rideau spot effets regle compte-a-rebours message reconnaissance-forme equation
                                presentation)
    string(REPLACE "-" "_" _lot7_test "${_lot7_scenario}")
    add_test(NAME tst_captures_${_lot7_test}
        COMMAND $<TARGET_FILE:actcraft-captures>
            --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/${_lot7_scenario}.json
            --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-${_lot7_scenario})
    set_tests_properties(tst_captures_${_lot7_test} PROPERTIES
        TIMEOUT 300
        ENVIRONMENT "${_bench_env_offscreen}"
        PASS_REGULAR_EXPRESSION "SCENARIO OK"
        FAIL_REGULAR_EXPRESSION "ECART|SCENARIO EN ECHEC|SCENARIO INVALIDE")
endforeach()
