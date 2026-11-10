# Journaux des tests rouges, pour l'artefact « journaux-tests » de la CI (ci.yml, ci-ciblee.yml).
# cmake -DBUILD_DIR=build/debug -DOUT_DIR=<dossier> [-DMAX_RELANCES=5] [-DMAX_SECONDES=240] -P scripts/ci/journaux-tests.cmake
# Le même script sur Windows et sur macOS : CMake est le seul interpréteur présent sur les deux runners (pas de pwsh
# sur le Mac mini, pas de bash garanti sur le PC).
# Contenu de OUT_DIR :
#   ctest-sortie.txt, ctest-junit.xml  écrits par ctest lui-même (--output-log, --output-junit) ;
#   LastTest.log, LastTestsFailed.log  copiés de <BUILD_DIR>/Testing/Temporary ;
#   par-test/<test>.txt                Windows : sortie QtTest et code de sortie du passage sous ctest, pour chaque test
#                                      (tests/lanceur_test_windows.cpp) ;
#   <test>.txt                         sortie QtTest -v2 du test rouge relancé en direct ;
#   <test>-console.txt                 sa sortie console (qDebug, plantage), code de sortie en dernière ligne.
# Relance en direct (même logique que preuves-windows.yml) : ctest n'affiche rien quand le test meurt avant d'écrire.
# Plafonnée en nombre et en durée : la CI complète a peu de marge sous son délai de job.
cmake_minimum_required(VERSION 3.21)

foreach(var BUILD_DIR OUT_DIR)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "journaux-tests : -D${var}=... manquant")
    endif()
    file(TO_CMAKE_PATH "${${var}}" ${var})
endforeach()
if(NOT DEFINED MAX_RELANCES)
    set(MAX_RELANCES 5)
endif()
if(NOT DEFINED MAX_SECONDES)
    set(MAX_SECONDES 240)
endif()

file(MAKE_DIRECTORY "${OUT_DIR}")
file(GLOB journaux "${BUILD_DIR}/Testing/Temporary/*.log")
if(journaux)
    file(COPY ${journaux} DESTINATION "${OUT_DIR}")
else()
    message(STATUS "journaux-tests : aucun journal dans ${BUILD_DIR}/Testing/Temporary")
endif()
file(GLOB par_test "${BUILD_DIR}/journaux-tests/*.txt")
if(par_test)
    file(COPY ${par_test} DESTINATION "${OUT_DIR}/par-test")
endif()
if(NOT EXISTS "${BUILD_DIR}/Testing/Temporary/LastTestsFailed.log")
    message(STATUS "journaux-tests : aucun test rouge à relancer")
    return()
endif()

execute_process(
    COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${BUILD_DIR}" --rerun-failed --show-only=json-v1
    OUTPUT_VARIABLE json
    RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
    message(STATUS "journaux-tests : ctest --show-only=json-v1 a échoué (${rc}), pas de relance")
    return()
endif()

# Environnement de actcraft_add_test (tests/CMakeLists.txt), plus la journalisation de Qt sur stderr.
set(ENV{QT_FORCE_STDERR_LOGGING} 1)
set(ENV{QT_LOGGING_RULES} "*.debug=false")

string(TIMESTAMP debut "%s" UTC)
set(relances 0)
string(JSON nb_tests LENGTH "${json}" tests)
math(EXPR dernier "${nb_tests} - 1")
foreach(i RANGE 0 ${dernier})
    if(nb_tests EQUAL 0)
        break()
    endif()
    string(JSON nom GET "${json}" tests ${i} name)
    string(JSON nb_args ERROR_VARIABLE sans_commande LENGTH "${json}" tests ${i} command)
    if(sans_commande OR nb_args EQUAL 0)
        message(STATUS "== ${nom} : pas de commande, non relancé")
        continue()
    endif()
    set(commande "")
    math(EXPR dernier_arg "${nb_args} - 1")
    foreach(j RANGE 0 ${dernier_arg})
        string(JSON arg GET "${json}" tests ${i} command ${j})
        list(APPEND commande "${arg}")
    endforeach()
    list(GET commande 0 exe)
    # Windows : actcraft_test_launcher <journal> <test> ... ; la relance vise le test lui-même.
    get_filename_component(lanceur "${exe}" NAME_WE)
    if(lanceur STREQUAL "actcraft_test_launcher")
        list(REMOVE_AT commande 0 1)
        list(GET commande 0 exe)
    endif()
    get_filename_component(exe_nom "${exe}" NAME)
    if(NOT exe_nom MATCHES "^tst_")
        message(STATUS "== ${nom} : ${exe_nom} n'est pas un exécutable QtTest, non relancé")
        continue()
    endif()

    string(TIMESTAMP maintenant "%s" UTC)
    math(EXPR reste "${MAX_SECONDES} - (${maintenant} - ${debut})")
    if(relances GREATER_EQUAL MAX_RELANCES OR reste LESS_EQUAL 0)
        message(STATUS "== ${nom} non relancé (plafond : ${MAX_RELANCES} relances, ${MAX_SECONDES} s)")
        continue()
    endif()
    # Part égale du reste entre les relances encore possibles : un test lent (tst_bench_verdict, 259 s) ne prive pas
    # les suivants, souvent ceux qui plantent tôt.
    math(EXPR reste "${reste} / (${MAX_RELANCES} - ${relances})")
    if(reste LESS 1)
        set(reste 1)
    endif()
    math(EXPR relances "${relances} + 1")

    # Dossier de travail et environnement du test, tels que ctest les donne.
    set(dossier "${BUILD_DIR}")
    string(JSON nb_props ERROR_VARIABLE sans_props LENGTH "${json}" tests ${i} properties)
    if(NOT sans_props AND nb_props GREATER 0)
        math(EXPR dernier_prop "${nb_props} - 1")
        foreach(k RANGE 0 ${dernier_prop})
            string(JSON prop GET "${json}" tests ${i} properties ${k} name)
            if(prop STREQUAL "WORKING_DIRECTORY")
                string(JSON dossier GET "${json}" tests ${i} properties ${k} value)
            elseif(prop STREQUAL "ENVIRONMENT")
                string(JSON nb_env LENGTH "${json}" tests ${i} properties ${k} value)
                math(EXPR dernier_env "${nb_env} - 1")
                foreach(l RANGE 0 ${dernier_env})
                    string(JSON affectation GET "${json}" tests ${i} properties ${k} value ${l})
                    string(FIND "${affectation}" "=" egal)
                    string(SUBSTRING "${affectation}" 0 ${egal} cle)
                    math(EXPR debut_valeur "${egal} + 1")
                    string(SUBSTRING "${affectation}" ${debut_valeur} -1 valeur)
                    set(ENV{${cle}} "${valeur}")
                endforeach()
            endif()
        endforeach()
    endif()

    message(STATUS "== ${nom} en direct (${reste} s au plus)")
    execute_process(
        COMMAND ${commande} -v2 -o "${OUT_DIR}/${nom}.txt,txt"
        WORKING_DIRECTORY "${dossier}"
        TIMEOUT ${reste}
        RESULT_VARIABLE code
        OUTPUT_FILE "${OUT_DIR}/${nom}-console.txt"
        ERROR_FILE "${OUT_DIR}/${nom}-console.txt")
    file(APPEND "${OUT_DIR}/${nom}-console.txt" "\ncode de sortie : ${code}\n")
    message(STATUS "   code de sortie : ${code}")
endforeach()
