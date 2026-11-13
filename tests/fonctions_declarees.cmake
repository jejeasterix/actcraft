# Vérifie que les entrées ctest d'un exécutable Qt Test jouent toutes ses fonctions, ni plus ni moins : une fonction
# ajoutée au test mais oubliée dans la liste de CMake ne serait jouée par aucune entrée.
#   cmake -DEXE=<exécutable> -DFUNCTIONS=<noms séparés par des virgules> -P fonctions_declarees.cmake
execute_process(
    COMMAND "${EXE}" -functions
    RESULT_VARIABLE code
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err)
if(NOT code EQUAL 0)
    message(FATAL_ERROR "${EXE} -functions : code ${code}\n${err}")
endif()
string(REPLACE "()" "" out "${out}")
string(STRIP "${out}" out)
string(REPLACE "\n" ";" listed "${out}")
string(REPLACE "," ";" declared "${FUNCTIONS}")
list(SORT listed)
list(SORT declared)
if(NOT listed STREQUAL declared)
    message(FATAL_ERROR "fonctions de ${EXE} : ${listed}\nfonctions déclarées dans CMake : ${declared}")
endif()
message("FONCTIONS DECLAREES OK : ${EXE}")
