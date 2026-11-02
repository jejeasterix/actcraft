# Export des pages (image, PDF), import PDF et conversion de présentations (DA12). Stubs du lot 5 (L5-T1) ;
# remplis par L5-T23, L5-T24 et L5-T29.
actcraft_add_module(export
    SOURCES
        src/pageexport.cpp
        src/pdfexport.cpp
        src/pdfimport.cpp
        src/officeconvert.cpp
    DEPENDS
        ActCraft::render ActCraft::formats ActCraft::core Qt6::Gui Qt6::Concurrent)

# Import PDF par Qt PDF (liaison dynamique, LGPLv3) ; sans l'option, pdfImportAvailable() reste faux.
if(ACTCRAFT_IMPORT_PDF)
    find_package(Qt6 6.12 REQUIRED COMPONENTS Pdf)
    target_link_libraries(actcraft_export PUBLIC Qt6::Pdf)
    target_compile_definitions(actcraft_export PUBLIC ACTCRAFT_IMPORT_PDF=1)
endif()
