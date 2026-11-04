# Tests du module print (lot 5) : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).

# Déclaré seulement si l'impression est construite (ACTCRAFT_PRINT : Windows par défaut).
if(ACTCRAFT_PRINT)
    # L5-T27
    actcraft_add_test(tst_print_printer
        SOURCES tst_print_printer.cpp
        LIBS ActCraft::print)
endif()
