#pragma once
#include <actcraft/core/printlayout.h>

#include <QImage>
#include <QString>
#include <QStringList>

namespace actcraft::print {

bool printingAvailable(); // construit avec ACTCRAFT_PRINT
QStringList printerNames();
QString defaultPrinterName();
QImage renderSheet(const core::Document& document, const core::Sheet& sheet,
                   const core::PrintSettings& settings, core::SizeF sheetMm,
                   int dpi); // gris si settings.gray
struct PrintJob
{
    QString printer;
    QString outputFile; // non vide : fichier écrit par l'imprimante
    core::PrintSettings settings;
};
struct PrintResult
{
    bool ok = false;
    QString error;
    int sheets = 0;
};
PrintResult printDocument(const core::Document& document, std::size_t current, const PrintJob& job);

} // namespace actcraft::print
