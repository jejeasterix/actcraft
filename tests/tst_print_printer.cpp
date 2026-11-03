#include <actcraft/core/printlayout.h>
#include <actcraft/print/printer.h>
#include <actcraft/render/pagepainter.h>

#include <QColor>
#include <QDeadlineTimer>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>

#include <cstdlib>

using namespace actcraft;

namespace {

constexpr core::SizeF kA4{210, 297};
const QString kWindowsVirtualPrinter = QStringLiteral("Microsoft Print to PDF");

// Document synthétique de S17 : 4 pages, la troisième allongée (1024 × 1536), notes sur les pages 2 et 4.
core::Document documentS17()
{
    std::vector<core::Page> pages(4);
    pages[2].height = 1536;
    pages[1].notes = "ligne a\nligne b";
    pages[3].notes = "ligne c";
    return core::Document(std::move(pages));
}

// Document coloré : fond bleu, rectangle rouge à contour vert, texte jaune (HTML, document de texte gardé du
// peintre) : une composition qui oublierait l'un d'eux se voit.
core::Document colouredDocument()
{
    std::vector<core::Page> pages(1);
    pages[0].background.fill = core::solidFill(0xFF3060C0u);
    core::Shape rect;
    rect.kind = core::ShapeKind::Rectangle;
    rect.bounds = {120, 120, 360, 220};
    rect.fill = 0xFFD02020u;
    rect.outline = 0xFF20A040u;
    rect.outlineWidth = 6;
    pages[0].objects.emplace_back(rect);
    core::Text text;
    text.bounds = {120, 420, 760, 200};
    text.html = "<p style=\"font-size:40pt; color:#f0c000\">Impression</p>";
    pages[0].objects.emplace_back(text);
    return core::Document(std::move(pages));
}

// Imprimante virtuelle qui écrit un fichier : « Microsoft Print to PDF » sous Windows ; ailleurs, le PDF de
// Qt (nom d'imprimante vide, fichier de sortie posé).
print::PrintJob virtualPrinterJob(const QString& file = {}, const core::PrintSettings& settings = {})
{
    print::PrintJob job;
    if (print::printerNames().contains(kWindowsVirtualPrinter))
        job.printer = kWindowsVirtualPrinter;
    job.outputFile = file;
    job.settings = settings;
    return job;
}

// Imprimante native capable d'écrire un fichier : « Microsoft Print to PDF » sous Windows ; sous macOS, toute
// file du système (le moteur natif écrit le PDF quand un fichier de sortie est posé). Vide : aucune.
QString nativeFilePrinter()
{
    const QStringList names = print::printerNames();
    if (names.contains(kWindowsVirtualPrinter))
        return kWindowsVirtualPrinter;
#ifdef Q_OS_MACOS
    if (!names.isEmpty())
        return names.front();
#endif
    return {};
}

QByteArray readFile(const QString& path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

int countMatches(const QByteArray& pdf, const QString& pattern)
{
    const QRegularExpression re(pattern);
    int n = 0;
    for (auto it = re.globalMatch(QString::fromLatin1(pdf)); it.hasNext(); it.next())
        ++n;
    return n;
}

// Pages d'un PDF : objets `/Type /Page` (pas `/Pages`).
int countPdfPages(const QString& path)
{
    const QByteArray pdf = readFile(path);
    return pdf.isEmpty() ? -1 : countMatches(pdf, QStringLiteral("/Type\\s*/Page(?![s\\w])"));
}

int countPdfImages(const QString& path)
{
    return countMatches(readFile(path), QStringLiteral("/Subtype\\s*/Image\\b"));
}

// Le moteur PDF de Qt signe `/Producer (Qt x.y.z)` en UTF-16 BE (et `<pdf:Producer>Qt` en XMP).
bool producedByQt(const QByteArray& pdf)
{
    static const QByteArray utf16("\xfe\xff\0Q\0t\0 ", 8);
    return pdf.contains(utf16) || pdf.contains("Producer>Qt ");
}

bool waitForFile(const QString& path)
{
    // Le pilote d'une imprimante native écrit le fichier après le spouleur : attente bornée.
    const QDeadlineTimer deadline(30000);
    while (QFileInfo(path).size() <= 0 && !deadline.hasExpired())
        QTest::qWait(100);
    return QFileInfo(path).size() > 0;
}

bool isGray(QRgb c)
{
    return qRed(c) == qGreen(c) && qGreen(c) == qBlue(c);
}

bool hasColour(const QImage& image)
{
    const QImage rgb = image.convertToFormat(QImage::Format_RGB32);
    for (int y = 0; y < rgb.height(); ++y)
        for (int x = 0; x < rgb.width(); ++x)
            if (!isGray(rgb.pixel(x, y)))
                return true;
    return false;
}

QRectF toPixels(const core::SheetCell& cell, int dpi)
{
    const double k = dpi / 25.4;
    return {cell.target.x * k, cell.target.y * k, cell.target.width * k, cell.target.height * k};
}

} // namespace

class TestPrinter : public QObject
{
    Q_OBJECT
private slots:
    void printingIsBuilt() { QVERIFY(print::printingAvailable()); }

    // Le patron que la tâche écarte : une feuille composée par un second chemin de dessin (ou depuis le
    // cache, ou par les miniatures). La cellule de la feuille doit être, au pixel près, ce que le peintre
    // partagé dessine pour cette page dans ce rectangle.
    void sheetCellIsSharedPainterOutput()
    {
        const auto d = colouredDocument();
        const core::PrintSettings s; // une page par feuille, en couleur
        const auto sheets = core::layoutSheets(d, s, 0, kA4);
        QCOMPARE(sheets.size(), std::size_t(1));
        const int dpi = 150; // autre que 96 ppp : la page est mise à l'échelle de sa cellule
        const QImage sheet =
            print::renderSheet(d, sheets[0], s, kA4, dpi).convertToFormat(QImage::Format_RGB32);
        QVERIFY(!sheet.isNull());
        QCOMPARE(sheet.width(), qRound(kA4.width * dpi / 25.4));
        QCOMPARE(sheet.height(), qRound(kA4.height * dpi / 25.4));
        QVERIFY(hasColour(sheet));

        const auto& cell = sheets[0].cells[0];
        const core::Page& page = d.page(cell.page);
        const QRectF target = toPixels(cell, dpi);
        // Environnement de render::renderPage (render/src/pagepainter.cpp) : 96 ppp, fond blanc, lissage.
        QImage reference(sheet.size(), QImage::Format_ARGB32_Premultiplied);
        reference.setDotsPerMeterX(3780);
        reference.setDotsPerMeterY(3780);
        reference.fill(Qt::white);
        {
            QPainter p(&reference);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.setRenderHint(QPainter::TextAntialiasing, true);
            p.translate(target.topLeft());
            p.scale(target.width() / page.width, target.height() / page.height);
            const render::DecodedImages images = render::decodeImages(d);
            render::paintPage(p, page, images.lookup());
        }
        reference = reference.convertToFormat(QImage::Format_RGB32);
        // Intérieur de la cellule (le cadre est dessiné sur son bord) : écart au plus de 2 par canal.
        const QRect inner = target.toAlignedRect().adjusted(4, 4, -4, -4);
        int differing = 0;
        for (int y = inner.top(); y <= inner.bottom(); ++y) {
            for (int x = inner.left(); x <= inner.right(); ++x) {
                const QRgb a = sheet.pixel(x, y);
                const QRgb b = reference.pixel(x, y);
                if (std::abs(qRed(a) - qRed(b)) > 2 || std::abs(qGreen(a) - qGreen(b)) > 2 ||
                    std::abs(qBlue(a) - qBlue(b)) > 2)
                    ++differing;
            }
        }
        QCOMPARE(differing, 0);
    }

    // Le même patron vu de l'imprimante : chaque feuille imprimée est l'image de `renderSheet` (une image par
    // feuille dans le PDF), jamais un dessin vectoriel de `paintPage` sur le QPrinter, qui perdrait le gris
    // et la parité avec l'aperçu.
    void printedSheetsAreRenderedSheets()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        core::PrintSettings s;
        s.rows = 2;
        s.range = core::PrintRange::FromTo;
        s.from = 2;
        s.to = 4;
        s.notes = core::PrintNotes::WithPages;
        print::PrintJob job; // PDF de Qt sur toutes les plateformes
        job.outputFile = dir.filePath(QStringLiteral("images.pdf"));
        job.settings = s;
        const auto r = print::printDocument(documentS17(), 0, job);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.sheets, 4);
        QCOMPARE(countPdfPages(job.outputFile), 4);
        QCOMPARE(countPdfImages(job.outputFile), 4);
    }

    void printsToFileThroughVirtualPrinter()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        core::PrintSettings s;
        s.rows = 2;
        s.range = core::PrintRange::FromTo;
        s.from = 2;
        s.to = 4;
        s.notes = core::PrintNotes::WithPages;
        const auto job = virtualPrinterJob(dir.filePath(QStringLiteral("s17.pdf")), s);
        const auto r = print::printDocument(documentS17(), 0, job);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.sheets, 4);
        QVERIFY(waitForFile(job.outputFile));
        QCOMPARE(countPdfPages(job.outputFile), 4);
    }

    void grayHasNoColor()
    {
        const auto d = colouredDocument();
        core::PrintSettings s;
        const auto sheets = core::layoutSheets(d, s, 0, kA4);
        QVERIFY(!sheets.empty());
        // Le contenu est coloré : sans cela, le contrôle du gris passerait à vide.
        QVERIFY(hasColour(print::renderSheet(d, sheets[0], s, kA4, 50)));
        s.gray = true;
        const QImage gray =
            print::renderSheet(d, sheets[0], s, kA4, 50).convertToFormat(QImage::Format_RGB32);
        QVERIFY(!gray.isNull());
        bool allWhite = true;
        for (int y = 0; y < gray.height(); ++y) {
            for (int x = 0; x < gray.width(); ++x) {
                const QRgb c = gray.pixel(x, y);
                QVERIFY2(isGray(c), qPrintable(QStringLiteral("pixel (%1, %2) en couleur").arg(x).arg(y)));
                allWhite = allWhite && c == qRgb(255, 255, 255);
            }
        }
        QVERIFY(!allWhite);
    }

    // L5-T26 : `layoutSheets` rend les feuilles d'une copie ; l'impression les répète `copies` fois.
    void sheetCountMatchesLayout()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const auto d = documentS17();
        core::PrintSettings s;
        s.rows = 2;
        s.range = core::PrintRange::FromTo;
        s.from = 2;
        s.to = 4;
        s.notes = core::PrintNotes::WithPages;
        s.copies = 2;
        const auto layout = core::layoutSheets(d, s, 0, kA4);
        const int expected = core::sheetCount(d, s, 0, kA4);
        QCOMPARE(expected, static_cast<int>(layout.size()) * 2);
        print::PrintJob job;
        job.outputFile = dir.filePath(QStringLiteral("copies.pdf"));
        job.settings = s;
        const auto r = print::printDocument(d, 0, job);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.sheets, expected);
        QCOMPARE(countPdfPages(job.outputFile), expected);
    }

    void worksWithoutQApplication()
    {
        QVERIFY(!qApp->inherits("QApplication"));
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(
            print::printDocument(documentS17(), 0, virtualPrinterJob(dir.filePath(QStringLiteral("a.pdf"))))
                .ok);
    }

    // Sortie nommée `….pdf` vers une imprimante native : Qt passerait en PdfFormat d'après l'extension et
    // écrirait le fichier lui-même ; le format natif est reposé après le nom, le fichier vient donc du
    // pilote.
    void nativeFormatKeptForPdfName()
    {
        const QString printer = nativeFilePrinter();
        if (printer.isEmpty())
            QSKIP("imprimante virtuelle absente");
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        print::PrintJob job;
        job.printer = printer;
        job.outputFile = dir.filePath(QStringLiteral("natif.pdf"));
        core::PrintSettings s;
        s.range = core::PrintRange::Current;
        job.settings = s;
        const auto r = print::printDocument(documentS17(), 0, job);
        QVERIFY2(r.ok, qPrintable(r.error));
        QVERIFY(waitForFile(job.outputFile));
        const QByteArray pdf = readFile(job.outputFile);
        QVERIFY(pdf.startsWith("%PDF"));
        QVERIFY2(!producedByQt(pdf), "fichier écrit par le moteur PDF de Qt : format natif perdu");
    }
};

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv); // jamais QApplication
    TestPrinter test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_print_printer.moc"
