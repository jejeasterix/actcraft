# Tests du module export (lot 5) : actcraft_add_test(...) (fonction définie dans tests/CMakeLists.txt).

# L5-T23
actcraft_add_test(tst_export_image
    SOURCES tst_export_image.cpp
    LIBS ActCraft::export)

# L5-T23
actcraft_add_test(tst_export_pdf
    SOURCES tst_export_pdf.cpp
    LIBS ActCraft::export)

# L5-T24
actcraft_add_test(tst_export_pdfimport
    SOURCES tst_export_pdfimport.cpp
    LIBS ActCraft::export)

# L5-T29
actcraft_add_test(tst_export_officeconvert
    SOURCES tst_export_officeconvert.cpp
    LIBS ActCraft::export)

