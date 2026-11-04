#pragma once
#include <actcraft/core/document.h>
#include <actcraft/core/model.h>

#include <QFuture>
#include <QString>
#include <QUrl>

namespace actcraft::exporting {

bool pdfImportAvailable(); // construit avec ACTCRAFT_IMPORT_PDF
struct ImportResult
{
    bool ok = false;
    QString error;
    core::Document document; // une page par page du PDF (D16)
};
QFuture<ImportResult> importPdf(QUrl url, int dpi = 150);

} // namespace actcraft::exporting
