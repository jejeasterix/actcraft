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
    set_tests_properties(tst_licences_sans_fixture PROPERTIES PASS_REGULAR_EXPRESSION "LICENCES"
        FAIL_REGULAR_EXPRESSION "Qt6Charts")
endif()
