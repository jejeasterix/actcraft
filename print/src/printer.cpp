#include <actcraft/print/printer.h>
#include <actcraft/render/pagepainter.h>

#include <QColor>
#include <QPainter>
#include <QPen>
#include <QRectF>

#if defined(ACTCRAFT_PRINT)
#include <QPageLayout>
#include <QPrinter>
#include <QPrinterInfo>
#endif

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace actcraft::print {

namespace {

// Réglages de render::renderPage (render/src/pagepainter.cpp) : image à 96 ppp, fond blanc, lissage ; une
// feuille plus fine change l'échelle du peintre, pas ces réglages.
constexpr int kPainterDotsPerMeter = 3780;
constexpr double kPainterDpi = 96.0;
constexpr double kMmPerInch = 25.4;
thread_local std::size_t g_decodedImages = 0;
#if defined(ACTCRAFT_PRINT)
// Plafond de la feuille en image : A4 à 300 ppp ≈ 8,7 Mpx (35 Mo en couleur) ; au-delà, la mémoire croît sans
// gain visible pour une page de tableau blanc.
constexpr int kMaxSheetDpi = 300;
#endif

QRectF toPixels(const core::RectF& mm, double pxPerMm)
{
    return {mm.x * pxPerMm, mm.y * pxPerMm, mm.width * pxPerMm, mm.height * pxPerMm};
}

// Marge bornée comme dans core::layoutSheets : jamais négative, toujours sous la demi-feuille.
double clampedMargin(double marginMm, core::SizeF sheetMm)
{
    const double limit = 0.45 * std::min(sheetMm.width, sheetMm.height);
    return marginMm >= 0 ? std::min(marginMm, limit) : 0.0;
}

// Texte brut posé par le peintre partagé : son HTML passe par le QTextDocument à loadResource neutralisé
// (L2-T11), comme tout texte de document.
void paintPlainText(QPainter& p, const QString& text, const QRectF& bounds, double pixelSize, bool centered)
{
    core::Text t;
    t.bounds = {bounds.x(), bounds.y(), bounds.width(), bounds.height()};
    t.html = QStringLiteral("<div align=\"%1\" style=\"white-space:pre; font-size:%2px\">%3</div>")
                 .arg(centered ? QStringLiteral("center") : QStringLiteral("left"),
                      QString::number(std::max(1.0, pixelSize), 'f', 1), text.toHtmlEscaped())
                 .toStdString();
    const render::ImageLookup noImages = [](const std::string&) { return QImage(); };
    render::paintObject(p, core::PageObject(std::move(t)), noImages);
}

QStringList splitNotes(const core::Page& page)
{
    // Même compte que core::layoutSheets : lignes = sauts de ligne + 1, aucune si les notes sont vides.
    return page.notes.empty() ? QStringList() : QString::fromStdString(page.notes).split(QLatin1Char('\n'));
}

struct NoteLine
{
    std::size_t page = 0;
    QString text;
    bool labelled = false; // première ligne de la page sur cette feuille : son numéro est écrit devant
};

// Lignes de notes d'une feuille de notes, selon core/src/printlayout.cpp (notesSheets).
std::vector<NoteLine> notesOf(const core::Document& document, const core::Sheet& sheet,
                              const core::PrintSettings& settings)
{
    std::vector<NoteLine> out;
    const std::size_t count = document.pageCount();
    if (sheet.noteOf.empty() || sheet.noteOf.front() >= count)
        return out;
    if (settings.noteBreak) {
        for (const std::size_t page : sheet.noteOf) {
            if (page >= count)
                continue;
            const QStringList lines = splitNotes(document.page(page));
            for (qsizetype i = 0; i < lines.size(); ++i)
                out.push_back({page, lines[i], i == 0});
        }
        return out;
    }
    // Notes à la suite : kNoteLinesPerSheet lignes par feuille, comptées depuis le début de la plage.
    const std::size_t first = sheet.noteOf.front();
    std::size_t spanFirst = first; // Current : la plage est la page elle-même
    if (settings.range == core::PrintRange::All) {
        spanFirst = 0;
    } else if (settings.range == core::PrintRange::FromTo) {
        const int low = std::clamp(std::min(settings.from, settings.to), 1, static_cast<int>(count));
        spanFirst = std::min(static_cast<std::size_t>(low - 1), first);
    }
    std::size_t used = 0;
    for (std::size_t p = spanFirst; p < first; ++p)
        used += static_cast<std::size_t>(splitNotes(document.page(p)).size());
    const auto begin = static_cast<std::size_t>(std::max(0, sheet.firstNoteLine));
    const std::size_t end = begin + static_cast<std::size_t>(core::kNoteLinesPerSheet);
    std::size_t line = used;
    for (const std::size_t page : sheet.noteOf) {
        if (page >= count)
            continue;
        const QStringList lines = splitNotes(document.page(page));
        bool labelled = false;
        for (qsizetype i = 0; i < lines.size(); ++i, ++line) {
            if (line < begin || line >= end)
                continue;
            out.push_back({page, lines[i], !labelled});
            labelled = true;
        }
    }
    return out;
}

void paintNotesSheet(QPainter& p, const core::Document& document, const core::Sheet& sheet,
                     const core::PrintSettings& settings, core::SizeF sheetMm, double pxPerMm)
{
    const std::vector<NoteLine> lines = notesOf(document, sheet, settings);
    // Unités du peintre (96 ppp), mises à l'échelle de la feuille.
    const double unitPerMm = kPainterDpi / kMmPerInch;
    p.save();
    p.scale(pxPerMm / unitPerMm, pxPerMm / unitPerMm);
    const double margin = clampedMargin(settings.marginMm, sheetMm) * unitPerMm;
    const QRectF area(margin, margin, sheetMm.width * unitPerMm - 2 * margin,
                      sheetMm.height * unitPerMm - 2 * margin);
    const double slotCount =
        std::max(static_cast<double>(core::kNoteLinesPerSheet), static_cast<double>(lines.size()));
    const double lineHeight = area.height() / slotCount;
    const double gutter = std::min(area.width() / 4, 22 * unitPerMm);
    const double fontPx = lineHeight * 0.68;
    QPen rule(QColor(0x80, 0x80, 0x80));
    rule.setWidthF(0.6);
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const NoteLine& line = lines[i];
        const double y = area.top() + static_cast<double>(i) * lineHeight;
        if (line.labelled) {
            if (settings.noteSeparator && i > 0) {
                p.setPen(rule);
                p.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
            }
            paintPlainText(p, QStringLiteral("Page %1").arg(line.page + 1),
                           QRectF(area.left(), y, gutter, lineHeight), fontPx, false);
        }
        paintPlainText(p, line.text, QRectF(area.left() + gutter, y, area.width() - gutter, lineHeight),
                       fontPx, false);
    }
    p.restore();
}

void paintPagesSheet(QPainter& p, const core::Document& document, const core::Sheet& sheet,
                     const core::PrintSettings& settings, double pxPerMm)
{
    // Images du document en pleine résolution (jamais celles, réduites, des miniatures).
    const render::DecodedImages images = render::decodeImages(document);
    g_decodedImages = images.size();
    const render::ImageLookup lookup = images.lookup();
    const double unitPerPx = kPainterDpi / kMmPerInch / pxPerMm;
    for (const core::SheetCell& cell : sheet.cells) {
        if (cell.page >= document.pageCount())
            continue;
        const core::Page& page = document.page(cell.page);
        const QRectF target = toPixels(cell.target, pxPerMm);
        if (!(page.width > 0) || !(page.height > 0) || target.isEmpty())
            continue;
        // La page par le peintre partagé, comme render::renderPage, dans le rectangle de sa cellule.
        p.save();
        p.translate(target.topLeft());
        p.scale(target.width() / page.width, target.height() / page.height);
        p.setClipRect(QRectF(0, 0, page.width, page.height));
        render::paintPage(p, page, lookup);
        p.restore();
        if (settings.frames) {
            QPen pen(QColor(0x40, 0x40, 0x40));
            pen.setWidthF(std::max(1.0, 0.25 * pxPerMm));
            pen.setJoinStyle(Qt::MiterJoin);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawRect(target);
        }
        if (settings.pageNumbers) {
            p.save();
            p.scale(1 / unitPerPx, 1 / unitPerPx);
            const double height = 5 * kPainterDpi / kMmPerInch;
            const QRectF label(target.left() * unitPerPx, target.bottom() * unitPerPx,
                               target.width() * unitPerPx, height);
            paintPlainText(p, QString::number(cell.page + 1), label, height * 0.6, true);
            p.restore();
        }
    }
}

} // namespace

bool printingAvailable()
{
#if defined(ACTCRAFT_PRINT)
    return true;
#else
    return false;
#endif
}

QStringList printerNames()
{
#if defined(ACTCRAFT_PRINT)
    return QPrinterInfo::availablePrinterNames();
#else
    return {};
#endif
}

QString defaultPrinterName()
{
#if defined(ACTCRAFT_PRINT)
    return QPrinterInfo::defaultPrinterName();
#else
    return {};
#endif
}

std::size_t lastSheetDecodedImageCount()
{
    return g_decodedImages;
}

// L'unique composition d'une feuille : l'aperçu (L5-T28) et l'impression l'appellent tous deux.
QImage renderSheet(const core::Document& document, const core::Sheet& sheet,
                   const core::PrintSettings& settings, core::SizeF sheetMm, int dpi)
{
    if (dpi <= 0 || !(sheetMm.width > 0) || !(sheetMm.height > 0))
        return {};
    const double pxPerMm = dpi / kMmPerInch;
    const int width = std::max(1, static_cast<int>(std::lround(sheetMm.width * pxPerMm)));
    const int height = std::max(1, static_cast<int>(std::lround(sheetMm.height * pxPerMm)));
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    if (image.isNull())
        return {};
    image.setDotsPerMeterX(kPainterDotsPerMeter);
    image.setDotsPerMeterY(kPainterDotsPerMeter);
    image.fill(Qt::white);
    g_decodedImages = 0;
    {
        QPainter p(&image);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        if (sheet.notes)
            paintNotesSheet(p, document, sheet, settings, sheetMm, pxPerMm);
        else
            paintPagesSheet(p, document, sheet, settings, pxPerMm);
    }
    return image.convertToFormat(settings.gray ? QImage::Format_Grayscale8 : QImage::Format_RGB32);
}

PrintResult printDocument(const core::Document& document, std::size_t current, const PrintJob& job)
{
#if defined(ACTCRAFT_PRINT)
    PrintResult result;
    QPrinter printer(QPrinter::HighResolution);
    if (!job.printer.isEmpty()) {
        if (!printerNames().contains(job.printer)) {
            result.error = QStringLiteral("Imprimante introuvable : %1").arg(job.printer);
            return result;
        }
        if (!job.outputFile.isEmpty()) {
            // Qt passe en PdfFormat d'après l'extension « .pdf » et écrirait le fichier lui-même : le format
            // natif est reposé après le nom, puis l'imprimante choisie (le nom de fichier suit le changement
            // de moteur).
            printer.setOutputFileName(job.outputFile);
            printer.setOutputFormat(QPrinter::NativeFormat);
        }
        printer.setPrinterName(job.printer);
        if (printer.outputFormat() != QPrinter::NativeFormat || printer.printerName() != job.printer ||
            printer.outputFileName() != job.outputFile) {
            result.error = QStringLiteral("Imprimante inutilisable : %1").arg(job.printer);
            return result;
        }
    } else if (!job.outputFile.isEmpty()) {
        // Imprimante virtuelle hors Windows : le PDF de Qt.
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(job.outputFile);
    } else {
        result.error = QStringLiteral("Aucune imprimante ni fichier de sortie");
        return result;
    }
    if (!printer.isValid()) {
        result.error = QStringLiteral("Imprimante inutilisable : %1").arg(job.printer);
        return result;
    }
    printer.setFullPage(true); // la marge vient des réglages (core::layoutSheets), pas de l'imprimante
    printer.setCopyCount(1);   // les copies sont répétées ici (L5-T26)
    printer.setColorMode(job.settings.gray ? QPrinter::GrayScale : QPrinter::Color);
    printer.setCreator(QStringLiteral("ActCraft"));

    const QPageLayout layout = printer.pageLayout();
    const QSizeF paperMm = layout.fullRect(QPageLayout::Millimeter).size();
    const core::SizeF sheetMm{paperMm.width(), paperMm.height()};
    const std::vector<core::Sheet> sheets = core::layoutSheets(document, job.settings, current, sheetMm);
    if (sheets.empty()) {
        result.error = QStringLiteral("Rien à imprimer");
        return result;
    }
    const int resolution = printer.resolution();
    const int dpi = std::clamp(resolution, 1, kMaxSheetDpi);
    const QRectF paper(QPointF(0, 0), QSizeF(layout.fullRectPixels(resolution).size()));

    QPainter painter;
    if (!painter.begin(&printer)) {
        result.error = QStringLiteral("Impression refusée par l'imprimante");
        return result;
    }
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const int copies = std::max(1, job.settings.copies);
    bool first = true;
    for (int copy = 0; copy < copies; ++copy) {
        for (const core::Sheet& sheet : sheets) {
            if (!first && !printer.newPage()) {
                painter.end();
                result.error = QStringLiteral("Feuille suivante refusée par l'imprimante");
                return result;
            }
            first = false;
            // Chaque feuille est l'image de renderSheet, la même que l'aperçu : jamais un second dessin.
            painter.drawImage(paper, renderSheet(document, sheet, job.settings, sheetMm, dpi));
            ++result.sheets;
        }
    }
    if (!painter.end()) {
        result.error = QStringLiteral("Impression interrompue");
        return result;
    }
    result.ok = true;
    return result;
#else
    static_cast<void>(document);
    static_cast<void>(current);
    static_cast<void>(job);
    return {false, QStringLiteral("Impression absente de cette version (ACTCRAFT_PRINT)"), 0};
#endif
}

} // namespace actcraft::print
