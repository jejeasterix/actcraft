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
