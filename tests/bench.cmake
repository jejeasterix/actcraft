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
# Chien de garde de Qt Test : 300 s par fonction par défaut (QTEST_FUNCTION_TIMEOUT). Les fonctions qui jouent le
# vrai banc (runBench) le dépassaient sous charge : abandon par qFatal (« Test function timed out »), rien dans la
# sortie de ctest, échec 397 s après le départ (L3-C3 ; même signature reproduite avec un chien de 15 s). 259 s
# seul, 355 s sous charge pour tout l'exécutable : 1200 s par fonction et 1800 s en tout ne détectent qu'un blocage.
set_property(TEST tst_bench_verdict APPEND PROPERTY ENVIRONMENT "QTEST_FUNCTION_TIMEOUT=1200000")
set_tests_properties(tst_bench_verdict PROPERTIES TIMEOUT 1800)

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
# L3-T19 : le smoke joue aussi gommer et soulever, dont chaque redessin du cache coûte des centaines de ms en debug
# logiciel. Seul : environ 60 s ; sous la charge de la suite complète en parallèle : plus de 300 s (relevé le
# 2026-10-19, Timeout à 300,39 s). Le fichier JSON qu'il écrit alimente tst_bench_result et tst_bench_summary.
set_tests_properties(tst_bench_smoke PROPERTIES
    TIMEOUT 1800
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

# Scénario S33 (traits lissés, grille nette), hors écran en rendu logiciel : le peintre lisse déjà ; prouve
# l'étape checkEdge et la lecture de pageCache.samples. La preuve D3D11 se joue à l'écran (L10-T5, clôture).
add_test(NAME tst_captures_anticrenelage
    COMMAND $<TARGET_FILE:actcraft-captures>
        --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/anticrenelage.json
        --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-anticrenelage)
set_tests_properties(tst_captures_anticrenelage PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "SCENARIO OK"
    FAIL_REGULAR_EXPRESSION "ECART|SCENARIO EN ECHEC|SCENARIO INVALIDE")

# Lot 10 (S13, S33) : versement différé au cache. Trois traits attendent au-dessus du cache, sont versés après
# l'inactivité, puis un zoom verse ; hors écran en rendu logiciel. À l'écran en D3D11, trait-en-attente prouve
# l'échantillonnage multiple de la fenêtre (L10-T4).
add_test(NAME tst_captures_cache_differe
    COMMAND $<TARGET_FILE:actcraft-captures>
        --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/cache-differe.json
        --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-cache-differe)
set_tests_properties(tst_captures_cache_differe PROPERTIES
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
# local, sur samples/ lu en place (fidélité), sauté sans ACTCRAFT_SAMPLES_DIR (code 77). Les trois variables
# (ACTCRAFT_SAMPLES_DIR, ACTCRAFT_THM_CONNUES, ACTCRAFT_THM_PAGES) sont lues à la configuration : les poser avant
# scripts/build.ps1 pour le test local.
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
# Miniature JPEG (L2-C7) : le rendu passe par le même codec, à la qualité lue dans les tables de la miniature, avant
# la mesure. Miniatures de qualité 50 (celle des .thm) rendues par notre peintre : écart quasi nul avec la
# recompression ; sans elle (--sans-recompression), l'écart de la compression seule, nettement plus grand.
set(_flipchart_q50 ${CMAKE_BINARY_DIR}/flipchart-q50.flipchart)
add_test(NAME tst_flipchart_ecrire_q50
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --ecrire-synthetique ${_flipchart_q50} --miniatures-q50)
set_tests_properties(tst_flipchart_ecrire_q50 PROPERTIES
    FIXTURES_SETUP flipchart_q50 ENVIRONMENT "${_bench_env_offscreen}" PASS_REGULAR_EXPRESSION "SYNTHETIQUE OK")
add_test(NAME tst_flipchart_miniatures_q50
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --miniatures ${_flipchart_q50})
set_tests_properties(tst_flipchart_miniatures_q50 PROPERTIES
    FIXTURES_REQUIRED flipchart_q50 ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "MINIATURES OK : 2 pages, médiane 0[.]0[0-9]")
add_test(NAME tst_flipchart_miniatures_q50_sans_recompression
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --miniatures ${_flipchart_q50} --sans-recompression)
set_tests_properties(tst_flipchart_miniatures_q50_sans_recompression PROPERTIES
    FIXTURES_REQUIRED flipchart_q50 ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "MINIATURES EN ECHEC : 2 pages, médiane ([2-9]|[1-9][0-9])[.]")
# La qualité estimée de chaque page est imprimée (L2-C7).
add_test(NAME tst_flipchart_miniatures_q50_qualite
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --miniatures ${_flipchart_q50})
set_tests_properties(tst_flipchart_miniatures_q50_qualite PROPERTIES
    FIXTURES_REQUIRED flipchart_q50 ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "PAGE flipchart-q50[.]flipchart:1 écart [0-9.]+ qualité 50\n")
# Une page de la liste des pages connues redescendue sous 7 est signalée, sans faire échouer.
add_test(NAME tst_flipchart_miniatures_connue_sous_7
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --miniatures ${_flipchart_synthetic}
        --connues "flipchart-synthetique.flipchart:1;flipchart-synthetique.flipchart:*")
set_tests_properties(tst_flipchart_miniatures_connue_sous_7 PROPERTIES
    FIXTURES_REQUIRED flipchart_synthetic ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "CONNUE SOUS 7 : flipchart-synthetique[.]flipchart:1 [(][0-9.]+[)].*CONNUE SOUS 7 : flipchart-synthetique[.]flipchart:[*] .*MINIATURES OK")
# Un défaut qui survit à la recompression q50 est vu : quart droit noir ET miniatures JPEG de qualité 50.
set(_flipchart_altered_q50 ${CMAKE_BINARY_DIR}/flipchart-altere-q50.flipchart)
add_test(NAME tst_flipchart_ecrire_altere_q50
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --ecrire-synthetique ${_flipchart_altered_q50}
        --miniatures-alterees --miniatures-q50)
set_tests_properties(tst_flipchart_ecrire_altere_q50 PROPERTIES
    FIXTURES_SETUP flipchart_altered_q50 ENVIRONMENT "${_bench_env_offscreen}" PASS_REGULAR_EXPRESSION "SYNTHETIQUE OK")
add_test(NAME tst_flipchart_miniatures_alterees_q50
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --miniatures ${_flipchart_altered_q50})
set_tests_properties(tst_flipchart_miniatures_alterees_q50 PROPERTIES
    FIXTURES_REQUIRED flipchart_altered_q50 ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "MINIATURES EN ECHEC : 2 pages, ")
# Tests qui distinguent une vraie mesure d'une mesure aveugle (rendu comparé à lui-même, lignes lues en partie) :
# l'écart tient dans le quart droit des miniatures. Code 1 attendu : le motif seul juge (le code de sortie est
# ignoré avec PASS_REGULAR_EXPRESSION).
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
# « fichier:* » excuse UNE page quelconque du fichier (plan) : la seconde page au-dessus de 7 est nommée.
add_test(NAME tst_flipchart_miniatures_etoile_une_page
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --miniatures ${_flipchart_altered}
        --connues "flipchart-altere.flipchart:*")
set_tests_properties(tst_flipchart_miniatures_etoile_une_page PROPERTIES
    FIXTURES_REQUIRED flipchart_altered ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "MINIATURES EN ECHEC : page flipchart-altere.flipchart:2 au-dessus de 7 et non connue")
# Aucune page ne sort de la mesure sans faire échouer : page 1 à miniature vide, page 2 à miniature indécodable ;
# pages écartées par le lecteur : nombre de pages lues comparé au nombre attendu.
set(_flipchart_holes ${CMAKE_BINARY_DIR}/flipchart-trous.flipchart)
add_test(NAME tst_flipchart_ecrire_trous
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --ecrire-synthetique ${_flipchart_holes} --miniatures-trouees)
set_tests_properties(tst_flipchart_ecrire_trous PROPERTIES
    FIXTURES_SETUP flipchart_holes ENVIRONMENT "${_bench_env_offscreen}" PASS_REGULAR_EXPRESSION "SYNTHETIQUE OK")
add_test(NAME tst_flipchart_miniatures_trous
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --miniatures ${_flipchart_holes})
set_tests_properties(tst_flipchart_miniatures_trous PROPERTIES
    FIXTURES_REQUIRED flipchart_holes ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "MINIATURES EN ECHEC : 2 pages hors de la mesure [(]1 sans miniature, 1 miniature illisible[)]")
add_test(NAME tst_flipchart_miniatures_pages_attendues
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --miniatures ${_flipchart_synthetic} --pages-attendues 3)
set_tests_properties(tst_flipchart_miniatures_pages_attendues PROPERTIES
    FIXTURES_REQUIRED flipchart_synthetic ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "MINIATURES EN ECHEC : 2 pages lues, 3 attendues")
# Critère « au plus 3 pages au-dessus de 7 » : 3 paperboards sains et 2 altérés, 10 pages, médiane basse, 4 pages
# au-dessus de 7.
set(_flipchart_folder ${CMAKE_BINARY_DIR}/flipchart-dossier)
foreach(_flipchart_name sain-1 sain-2 sain-3 altere-1 altere-2)
    set(_flipchart_variant "")
    if(_flipchart_name MATCHES "^altere")
        set(_flipchart_variant --miniatures-alterees)
    endif()
    add_test(NAME tst_flipchart_ecrire_dossier_${_flipchart_name}
        COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --ecrire-synthetique
            ${_flipchart_folder}/${_flipchart_name}.flipchart ${_flipchart_variant})
    set_tests_properties(tst_flipchart_ecrire_dossier_${_flipchart_name} PROPERTIES
        FIXTURES_SETUP flipchart_folder ENVIRONMENT "${_bench_env_offscreen}" PASS_REGULAR_EXPRESSION "SYNTHETIQUE OK")
endforeach()
add_test(NAME tst_flipchart_miniatures_quatre_au_dessus
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --samples ${_flipchart_folder} --pages-attendues 10)
set_tests_properties(tst_flipchart_miniatures_quatre_au_dessus PROPERTIES
    FIXTURES_REQUIRED flipchart_folder ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "MINIATURES EN ECHEC : 10 pages, médiane 0[.][0-9]+, 4 au-dessus de 7")
# Rendu borné comme les miniatures du peintre : une miniature de 2000 px de large est refusée, jamais rendue.
set(_flipchart_huge ${CMAKE_BINARY_DIR}/flipchart-geant.flipchart)
add_test(NAME tst_flipchart_ecrire_geant
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --ecrire-synthetique ${_flipchart_huge} --miniatures-geantes)
set_tests_properties(tst_flipchart_ecrire_geant PROPERTIES
    FIXTURES_SETUP flipchart_huge ENVIRONMENT "${_bench_env_offscreen}" PASS_REGULAR_EXPRESSION "SYNTHETIQUE OK")
add_test(NAME tst_flipchart_miniatures_geantes
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --miniatures ${_flipchart_huge})
set_tests_properties(tst_flipchart_miniatures_geantes PROPERTIES
    FIXTURES_REQUIRED flipchart_huge ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "MINIATURES EN ECHEC : flipchart-geant.flipchart page 1 : rendu de 2000 × 1500 refusé")
# Les comparaisons contiennent les vraies miniatures : jamais écrites hors de .tmp/ ou build/ du dépôt.
add_test(NAME tst_flipchart_comparaisons_hors_depot
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --miniatures ${_flipchart_synthetic}
        --comparaisons ${PROJECT_SOURCE_DIR}/bench/comparaisons)
set_tests_properties(tst_flipchart_comparaisons_hors_depot PROPERTIES
    FIXTURES_REQUIRED flipchart_synthetic ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "MINIATURES EN ECHEC : dossier de comparaisons hors de .tmp/ et de build/")
# Local : ACTCRAFT_THM_PAGES = nombre de pages attendu (124), sans quoi le test échoue (jamais de nom réel ici).
add_test(NAME tst_flipchart_miniatures_samples
    COMMAND $<TARGET_FILE:actcraft-flipchart-dump> --samples "$ENV{ACTCRAFT_SAMPLES_DIR}"
        --connues "$ENV{ACTCRAFT_THM_CONNUES}" --pages-attendues "$ENV{ACTCRAFT_THM_PAGES}")
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

# Scénarios du lot 3 (S34, S35, S37, S6), talons du socle : remplis par L3-T15, L3-T16, L3-T18.

add_test(NAME tst_captures_selection
    COMMAND $<TARGET_FILE:actcraft-captures>
        --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/selection.json
        --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-selection)
set_tests_properties(tst_captures_selection PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "SCENARIO OK"
    FAIL_REGULAR_EXPRESSION "ECART|SCENARIO EN ECHEC|SCENARIO INVALIDE")

add_test(NAME tst_captures_corbeille
    COMMAND $<TARGET_FILE:actcraft-captures>
        --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/corbeille.json
        --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-corbeille)
set_tests_properties(tst_captures_corbeille PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "SCENARIO OK"
    FAIL_REGULAR_EXPRESSION "ECART|SCENARIO EN ECHEC|SCENARIO INVALIDE")

add_test(NAME tst_captures_marqueur
    COMMAND $<TARGET_FILE:actcraft-captures>
        --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/marqueur.json
        --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-marqueur)
set_tests_properties(tst_captures_marqueur PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "SCENARIO OK"
    FAIL_REGULAR_EXPRESSION "ECART|SCENARIO EN ECHEC|SCENARIO INVALIDE")

add_test(NAME tst_captures_ecrire_gommer_annuler
    COMMAND $<TARGET_FILE:actcraft-captures>
        --scenario ${PROJECT_SOURCE_DIR}/bench/scenarios/ecrire-gommer-annuler.json
        --rendu logiciel --taille 1280x800 --sortie ${CMAKE_BINARY_DIR}/captures-ecrire-gommer-annuler)
set_tests_properties(tst_captures_ecrire_gommer_annuler PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "${_bench_env_offscreen}"
    PASS_REGULAR_EXPRESSION "SCENARIO OK"
    FAIL_REGULAR_EXPRESSION "ECART|SCENARIO EN ECHEC|SCENARIO INVALIDE")
