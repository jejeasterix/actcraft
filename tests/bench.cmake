# Tests du module bench : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).
if(NOT ACTCRAFT_BUILD_BENCH)
    return()
endif()

# Environnement hors écran des tests lancés sans actcraft_add_test (fumée du banc, captures) : mêmes polices que
# lui sous Windows (tests/CMakeLists.txt), sans quoi leurs textes et captures n'ont que des carrés vides.
set(_bench_env_offscreen "QT_QPA_PLATFORM=offscreen")
if(WIN32)
    list(APPEND _bench_env_offscreen "QT_QPA_FONTDIR=$ENV{WINDIR}/Fonts")
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
# Délai : 25-29 s en debug sur machine calme ; 50 s déjà sous charge modérée, plus de 60 s avec des compilations
# en parallèle. 300 s (≥ 10 × la durée calme) : le délai ne sert qu'à détecter un blocage, pas à mesurer.
set_tests_properties(tst_bench_smoke PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "${_bench_env_offscreen}"
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

# Joueur de scénarios d'interface d'actcraft-captures (DA8), sur une fenêtre d'essai à vraie PageView. QPA
# (GuiPrivate) : le test simule une livraison des touches sans carte des raccourcis, comme Qt sous macOS.
find_package(Qt6 6.12 REQUIRED COMPONENTS GuiPrivate)
actcraft_add_test(tst_bench_uiscenario
    SOURCES tst_bench_uiscenario.cpp
    LIBS ActCraft::benchui ActCraft::ui actcraft_uiplugin Qt6::Quick Qt6::GuiPrivate)
target_compile_definitions(tst_bench_uiscenario PRIVATE ACTCRAFT_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

# Scénario d'acceptation S2 (premiers pas), hors écran en rendu logiciel (DA8, décision 36).
add_test(NAME tst_captures_premiers_pas
    COMMAND $<TARGET_FILE:actcraft-captures>
        --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/premiers-pas.json
        --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-premiers-pas)
# Même règle : environ 14 s calme ; 300 s ne détecte qu'un blocage.
set_tests_properties(tst_captures_premiers_pas PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "SCENARIO OK"
    FAIL_REGULAR_EXPRESSION "ECART|SCENARIO EN ECHEC|SCENARIO INVALIDE")

# Scénario d'acceptation S5 (aller-retour natif), hors écran en rendu logiciel : document de 3 pages écrit par la
# fixture sous le dossier de sortie, enregistré hors du fil principal (sonde de boucle), fermé, rouvert.
add_test(NAME tst_captures_aller_retour_natif
    COMMAND $<TARGET_FILE:actcraft-captures>
        --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/aller-retour-natif.json
        --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-aller-retour-natif)
# Comme premiers-pas : le délai ne détecte qu'un blocage (machine chargée), il ne mesure rien.
set_tests_properties(tst_captures_aller_retour_natif PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "SCENARIO OK"
    FAIL_REGULAR_EXPRESSION "ECART|SCENARIO EN ECHEC|SCENARIO INVALIDE")

# S4 (L2-T21) : rendu de chaque page comparé à sa miniature .thm d'origine, réduits à 80 × 60. En CI, sur le
# paperboard de fctest::sampleFlipchart dont les miniatures sont rendues par notre peintre (preuve du chemin) ; en
# local, sur samples/ lu en place (fidélité), sauté sans ACTCRAFT_SAMPLES_DIR (code 77). Les deux variables sont lues
# à la configuration : les poser avant scripts/build.ps1 pour le test local.
set(_flipchart_synthetic ${CMAKE_BINARY_DIR}/flipchart-synthetique.flipchart)
set(_flipchart_altered ${CMAKE_BINARY_DIR}/flipchart-altere.flipchart)
add_test(NAME tst_flipchart_ecrire_synthetique
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --ecrire-synthetique ${_flipchart_synthetic})
set_tests_properties(tst_flipchart_ecrire_synthetique PROPERTIES
    FIXTURES_SETUP flipchart_synthetic ENVIRONMENT "${_bench_env_offscreen}" PASS_REGULAR_EXPRESSION "SYNTHETIQUE OK")
# Même paperboard, quart droit de chaque miniature peint en noir : la mesure doit le voir.
add_test(NAME tst_flipchart_ecrire_altere
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --ecrire-synthetique ${_flipchart_altered} --miniatures-alterees)
set_tests_properties(tst_flipchart_ecrire_altere PROPERTIES
    FIXTURES_SETUP flipchart_altered ENVIRONMENT "${_bench_env_offscreen}" PASS_REGULAR_EXPRESSION "SYNTHETIQUE OK")
add_test(NAME tst_flipchart_miniatures_synthetique
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --miniatures ${_flipchart_synthetic})
set_tests_properties(tst_flipchart_miniatures_synthetique PROPERTIES
    FIXTURES_REQUIRED flipchart_synthetic ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "MINIATURES OK : 2 pages")
# Tests qui distinguent une vraie mesure d'une mesure aveugle : l'écart tient dans le quart droit des images, que
# lirait de travers une mesure qui suppose le format RGB 8 bits conservé par la réduction (QImage::scaled lissé
# rend du RGB 32 bits). Code 1 attendu : le motif seul juge (le code de sortie est ignoré avec PASS_REGULAR_EXPRESSION).
add_test(NAME tst_flipchart_miniatures_alterees
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --miniatures ${_flipchart_altered})
set_tests_properties(tst_flipchart_miniatures_alterees PROPERTIES
    FIXTURES_REQUIRED flipchart_altered ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "MINIATURES EN ECHEC : 2 pages, ")
# La liste des pages connues est lue page par page : la page 2, au-dessus de 7 et hors de la liste, est nommée.
add_test(NAME tst_flipchart_miniatures_non_connue
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --miniatures ${_flipchart_altered}
        --connues "flipchart-altere.flipchart:1")
set_tests_properties(tst_flipchart_miniatures_non_connue PROPERTIES
    FIXTURES_REQUIRED flipchart_altered ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "MINIATURES EN ECHEC : page flipchart-altere.flipchart:2 au-dessus de 7 et non connue")
add_test(NAME tst_flipchart_miniatures_samples
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --samples "$ENV{ACTCRAFT_SAMPLES_DIR}"
        --connues "$ENV{ACTCRAFT_THM_CONNUES}")
set_tests_properties(tst_flipchart_miniatures_samples PROPERTIES
    SKIP_RETURN_CODE 77 TIMEOUT 600 ENVIRONMENT "${_bench_env_offscreen}" PASS_REGULAR_EXPRESSION "MINIATURES OK")

# Scénario d'acceptation S4 en CI, hors écran en rendu logiciel : le paperboard synthétique écrit par la fixture
# s'ouvre par le lecteur .flipchart (document sans fichier, avertissement affiché), fond et forme visibles.
add_test(NAME tst_captures_ouvrir_flipchart_synthetique
    COMMAND $<TARGET_FILE:actcraft-captures>
        --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/ouvrir-flipchart-synthetique.json
        --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-ouvrir-flipchart)
# Comme premiers-pas : le délai ne détecte qu'un blocage (machine chargée), il ne mesure rien.
set_tests_properties(tst_captures_ouvrir_flipchart_synthetique PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "SCENARIO OK"
    FAIL_REGULAR_EXPRESSION "ECART|SCENARIO EN ECHEC|SCENARIO INVALIDE")
