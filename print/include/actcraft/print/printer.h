#pragma once
#include <actcraft/core/printlayout.h>

#include <QImage>
#include <QString>
#include <QStringList>

#include <cstddef>

namespace actcraft::print {

bool printingAvailable(); // construit avec ACTCRAFT_PRINT
QStringList printerNames();
QString defaultPrinterName();
QImage renderSheet(const core::Document& document, const core::Sheet& sheet,
                   const core::PrintSettings& settings, core::SizeF sheetMm,
                   int dpi); // gris si settings.gray
// Couture de test : nombre d'images que le dernier renderSheet de ce fil a décodées (aucune pour une feuille
// de notes).
std::size_t lastSheetDecodedImageCount();
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
