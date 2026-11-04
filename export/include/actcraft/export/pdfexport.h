#pragma once
#include <actcraft/export/pageexport.h>

#include <QByteArray>

#include <vector>

namespace actcraft::exporting {

// Une page PDF par page (toutes si `pages` est vide), taille de la page (points = pixels × 72 / 96), marges
// nulles, render::paintPage sur un fil secondaire.
QFuture<ExportResult> exportPdf(core::Document document, QUrl url, std::vector<std::size_t> pages = {});
int countPdfPages(const QByteArray& pdf); // dictionnaires « /Type /Page », sans Qt PDF

} // namespace actcraft::exporting
