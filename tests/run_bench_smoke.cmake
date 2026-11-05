# Lance le banc en fumée et vérifie son code de sortie.
#   cmake -DBENCH=<exe> -DOUTPUT=<json> -DEXPECTED=<codes séparés par des virgules> -P run_bench_smoke.cmake
# Debug : 2 (MESURE INVALIDE, construction non release) ; release : 0 ou 1 (le verdict n'est pas une porte ici).
# --retenir-decodage (L6-C11) : la sonde de flash est exercée même si le décodage finit avant l'image.
execute_process(
    COMMAND "${BENCH}" --scenario court --rendu logiciel --taille 1280x800 --retenir-decodage --sortie "${OUTPUT}"
    RESULT_VARIABLE code
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err)
message("${out}")
string(REPLACE "," "|" expected_regex "${EXPECTED}")
if(NOT code MATCHES "^(${expected_regex})$")
    message(FATAL_ERROR "actcraft-bench : code ${code}, attendu ${EXPECTED}\n${err}")
endif()
if(NOT EXISTS "${OUTPUT}")
    message(FATAL_ERROR "actcraft-bench n'a pas écrit ${OUTPUT}")
endif()
