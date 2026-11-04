# L7-T41 : deux passes d'actcraft-captures à la suite. La première écrit l'effet de transition global, la seconde
# doit repartir des réglages par défaut. Discrimine : sous les réglages réels de l'utilisateur, sous un fichier de
# réglages commun à toute la suite, ou sous QStandardPaths::setTestModeEnabled (sans effet sur le format natif de
# QSettings : CFPreferences sous macOS, registre sous Windows), la seconde passe lit le fondu de la première.
#   cmake -DCAPTURES=<exe> -DSCENARIOS=<dossier> -DSORTIE=<dossier> -P captures_reglages_isoles.cmake
foreach(passe IN ITEMS reglages-ecrits reglages-vierges)
    execute_process(
        COMMAND "${CAPTURES}" --scenario "${SCENARIOS}/${passe}.json" --rendu logiciel --taille 1280x800
                --sortie "${SORTIE}/${passe}"
        OUTPUT_VARIABLE sortie ERROR_VARIABLE erreurs RESULT_VARIABLE code)
    message("== ${passe}\n${sortie}${erreurs}")
    if(NOT code EQUAL 0 OR NOT sortie MATCHES "SCENARIO OK")
        message(FATAL_ERROR "REGLAGES PARTAGES : la passe ${passe} finit en ${code}")
    endif()
endforeach()
message("REGLAGES ISOLES OK")
