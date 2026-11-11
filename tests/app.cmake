# Tests de l'exécutable et des scripts de paquets (S1, décision 34).

# --version : écrit la version et sort avant toute fenêtre.
add_test(NAME tst_app_version COMMAND $<TARGET_FILE:actcraft> --version)
set_tests_properties(tst_app_version PROPERTIES PASS_REGULAR_EXPRESSION "^ActCraft ${PROJECT_VERSION}")

# Liste des modules Qt autorisés : un dossier conforme passe, un module sous GPL seule est refusé.
find_program(ACTCRAFT_PWSH pwsh)
if(ACTCRAFT_PWSH)
    # Fixtures HORS du dossier de construction : verifier-licences.ps1 -Dossier build/<preset> les trouverait sinon.
    set(_licences ${CMAKE_BINARY_DIR}/../tests-fixtures/licences)
    file(MAKE_DIRECTORY ${_licences}/autorise ${_licences}/refuse)
    file(WRITE ${_licences}/autorise/Qt6Core.dll "")
    file(WRITE ${_licences}/autorise/Qt6Quick.dll "")
    # Stubs de Qt Multimedia (APK) : nom de module à trait d'union.
    file(WRITE ${_licences}/autorise/libQt6FFmpegStub-crypto_arm64-v8a.so "")
    file(WRITE ${_licences}/autorise/libQt6FFmpegStub-ssl_arm64-v8a.so "")
    file(WRITE ${_licences}/refuse/Qt6Core.dll "")
    file(WRITE ${_licences}/refuse/Qt6Charts.dll "")
    add_test(NAME tst_licences_ok
        COMMAND ${ACTCRAFT_PWSH} -NoProfile -File ${PROJECT_SOURCE_DIR}/scripts/verifier-licences.ps1
            -Dossier ${_licences}/autorise)
    set_tests_properties(tst_licences_ok PROPERTIES PASS_REGULAR_EXPRESSION "LICENCES OK")
    add_test(NAME tst_licences_refus
        COMMAND ${ACTCRAFT_PWSH} -NoProfile -File ${PROJECT_SOURCE_DIR}/scripts/verifier-licences.ps1
            -Dossier ${_licences}/refuse)
    set_tests_properties(tst_licences_refus PROPERTIES PASS_REGULAR_EXPRESSION "LICENCES EN ECHEC : Qt6Charts")
    # Garde : le dossier de construction lui-même ne contient aucune fixture refusée.
    add_test(NAME tst_licences_sans_fixture
        COMMAND ${ACTCRAFT_PWSH} -NoProfile -File ${PROJECT_SOURCE_DIR}/scripts/verifier-licences.ps1
            -Dossier ${CMAKE_BINARY_DIR})
    # RUN_SERIAL : il parcourt tout le dossier de construction, où les captures lancées en parallèle par ctest
    # suppriment et recréent leurs dossiers d'essai (DirectoryNotFound sous $ErrorActionPreference = 'Stop').
    set_tests_properties(tst_licences_sans_fixture PROPERTIES PASS_REGULAR_EXPRESSION "LICENCES"
        FAIL_REGULAR_EXPRESSION "Qt6Charts" RUN_SERIAL TRUE)
endif()

# Fichier ouvert au lancement (S4, décision 26).
actcraft_add_test(tst_app_open
    SOURCES tst_app_open.cpp ${PROJECT_SOURCE_DIR}/app/startup.cpp
    LIBS Qt6::Quick ActCraft::ui ActCraft::formats actcraft_uiplugin)
target_include_directories(tst_app_open PRIVATE ${PROJECT_SOURCE_DIR}/app)

# Lot 11 (L11-T1) : tests réservés à l'origine par des talons QSKIP, remplis par L11-T25 et L11-T28 (plus aucun talon).
actcraft_add_test(tst_app_notices
    SOURCES tst_app_notices.cpp
    LIBS ActCraft::ui)
target_compile_definitions(tst_app_notices PRIVATE ACTCRAFT_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

# Règle du versionCode (cmake/ActCraftVersion.cmake) : la valeur du projet et trois exemples calculés par la même fonction.
include(ActCraftVersion)
actcraft_version_code(ACTCRAFT_TEST_CODE_PROJECT ${PROJECT_VERSION_MAJOR} ${PROJECT_VERSION_MINOR} ${PROJECT_VERSION_PATCH})
actcraft_version_code(ACTCRAFT_TEST_CODE_1_0_0 1 0 0)
actcraft_version_code(ACTCRAFT_TEST_CODE_0_10_0 0 10 0)
actcraft_version_code(ACTCRAFT_TEST_CODE_1_2_3 1 2 3)
actcraft_add_test(tst_app_versioninfo
    SOURCES tst_app_versioninfo.cpp
    LIBS ActCraft::ui)
target_compile_definitions(tst_app_versioninfo PRIVATE
    ACTCRAFT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    ACTCRAFT_PROJECT_VERSION="${PROJECT_VERSION}"
    ACTCRAFT_VERSION_CODE=${ACTCRAFT_TEST_CODE_PROJECT}
    ACTCRAFT_CODE_1_0_0=${ACTCRAFT_TEST_CODE_1_0_0}
    ACTCRAFT_CODE_0_10_0=${ACTCRAFT_TEST_CODE_0_10_0}
    ACTCRAFT_CODE_1_2_3=${ACTCRAFT_TEST_CODE_1_2_3})

