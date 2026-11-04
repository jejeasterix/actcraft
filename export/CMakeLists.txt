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
    # Homebrew sépare Qt PDF (paquet qtwebengine) : le Qt6::Pdf du paquet qt annonce un dossier d'en-têtes
    # inexistant (include/QtPdf) ; les en-têtes viennent du framework. On écarte les dossiers absents.
    get_target_property(_pdf_inc Qt6::Pdf INTERFACE_INCLUDE_DIRECTORIES)
    set(_pdf_inc_ok "")
    foreach(_dir IN LISTS _pdf_inc)
        if(NOT _dir MATCHES "^\\$<" AND NOT EXISTS "${_dir}")
            continue()
        endif()
        list(APPEND _pdf_inc_ok "${_dir}")
    endforeach()
    set_property(TARGET Qt6::Pdf PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${_pdf_inc_ok}")
    target_link_libraries(actcraft_export PUBLIC Qt6::Pdf)
    target_compile_definitions(actcraft_export PUBLIC ACTCRAFT_IMPORT_PDF=1)
endif()

# Script PowerShell de conversion PowerPoint → PDF (L5-T29) : ressource Qt copiée dans un dossier temporaire
# avant l'appel (officeconvert.cpp).
qt_add_resources(actcraft_export "export_resources"
    PREFIX "/actcraft/export"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/resources"
    FILES resources/pptx-to-pdf.ps1)
