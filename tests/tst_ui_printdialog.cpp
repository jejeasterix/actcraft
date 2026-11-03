#include <actcraft/core/printlayout.h>
#include <actcraft/print/printer.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/printcontroller.h>

#include <QDesktopServices>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

// A4 portrait : la feuille de l'aperçu et de la boîte (L5-T28).
constexpr core::SizeF kSheet{210.0, 297.0};

core::Stroke redStroke(double y)
{
    core::Stroke stroke;
    stroke.color = 0xFFFF0000u;
    stroke.baseWidth = 40;
    for (int i = 0; i <= 20; ++i) {
        stroke.points.push_back(core::StrokePoint{100.0 + i * 20.0, y, 0.5f});
    }
    return stroke;
}

// Document de `pages` pages de 800 × 600, un trait rouge sur chacune.
void fillDocument(ui::DocumentController& document, int pages)
{
    document.newDocument(800, 600);
    for (int i = 0; i < pages; ++i) {
        document.commitStroke(redStroke(100.0 + 60.0 * i));
        if (i + 1 < pages) {
            document.nextPage();
        }
    }
    document.showPage(0);
}

core::PrintSettings settingsOf(const ui::PrintController& p)
{
    core::PrintSettings s;
    s.rows = p.rows();
    s.columns = p.columns();
    s.frames = p.frames();
    s.pageNumbers = p.pageNumbers();
    s.range = p.range() == QLatin1String("current")   ? core::PrintRange::Current
              : p.range() == QLatin1String("from-to") ? core::PrintRange::FromTo
                                                      : core::PrintRange::All;
    s.from = p.from();
    s.to = p.to();
    s.copies = p.copies();
    s.marginMm = p.marginMm();
    s.gray = p.gray();
    s.notes = p.notes() == QLatin1String("only")         ? core::PrintNotes::NotesOnly
              : p.notes() == QLatin1String("with-pages") ? core::PrintNotes::WithPages
                                                         : core::PrintNotes::None;
    s.noteBreak = p.noteBreak();
    s.noteSeparator = p.noteSeparator();
    return s;
}

bool hasColor(const QImage& image)
{
    for (int y = 0; y < image.height(); y += 2) {
        for (int x = 0; x < image.width(); x += 2) {
            const QColor c = image.pixelColor(x, y);
            if (c.red() != c.green() || c.green() != c.blue()) {
                return true;
            }
        }
    }
    return false;
}

int pdfPages(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return -1;
    }
    const QByteArray data = file.readAll();
    int count = 0;
    const QRegularExpression page(QStringLiteral("/Type\\s*/Page(?![A-Za-z])"));
    auto it = page.globalMatch(QString::fromLatin1(data));
    while (it.hasNext()) {
        it.next();
        ++count;
    }
    return count;
}

// Boîte d'impression dans une fenêtre : le contrôleur et un faux échange (FileExchange) qui note ses appels.
const char* kShellQml = R"(
import QtQuick
import ActCraft.Ui
Item {
    width: 900; height: 700
    property alias printing: ctl
    property alias dialog: dlg
    property var calls: []
    PrintController { id: ctl; objectName: "ctl" }
    QtObject {
        id: exchange
        function exportPdf(url) { calls = calls.concat(["export:" + url]) }
        function printThroughPdf() { calls = calls.concat(["through-pdf"]) }
    }
    PrintSetupDialog { id: dlg; objectName: "dlg"; printing: ctl; exchange: exchange }
}
)";

struct Shell
{
    QQmlEngine engine;
    QQuickWindow window;
    QQuickItem* root = nullptr;
    ui::PrintController* printing = nullptr;
    QObject* dialog = nullptr;
    QTemporaryDir dir;
    ui::DocumentController document;

    bool start(int pages)
    {
        fillDocument(document, pages);
        QQmlComponent component(&engine);
        component.setData(kShellQml, QUrl());
        root = qobject_cast<QQuickItem*>(component.create());
        if (!root) {
            qWarning("%s", qPrintable(component.errorString()));
            return false;
        }
        root->setParentItem(window.contentItem());
        window.resize(900, 700);
        window.show();
        printing = root->property("printing").value<ui::PrintController*>();
        dialog = root->property("dialog").value<QObject*>();
        if (!printing || !dialog || !QTest::qWaitForWindowExposed(&window)) {
            return false;
        }
        printing->setSettingsFile(dir.filePath(QStringLiteral("reglages.ini")));
        printing->setDocument(&document);
        return true;
    }
    template <class T = QQuickItem> T* find(const char* name)
    {
        return dialog->findChild<T*>(QString::fromLatin1(name));
    }
};

// Gestionnaire des URL « file » : note les PDF que « Imprimer » ouvrirait dans un lecteur.
struct UrlCatcher : QObject
{
    Q_OBJECT
public:
    QList<QUrl> urls;
public slots:
    void got(const QUrl& url) { urls.push_back(url); }
};

} // namespace

class TstUiPrintDialog : public QObject
{
    Q_OBJECT
private slots:
    void sheetCountFollowsSettings();
    void settingsSurviveRestart();
    void previewMatchesSheets();
    void previewIsLazy();
    void printWritesTheSheets();
    void dialogEditsSettingsAndShowsTheSheet();
    void printButtonPrintsTheSheetsOfTheSettings();
};

void TstUiPrintDialog::sheetCountFollowsSettings()
{
    ui::DocumentController document;
    fillDocument(document, 4);
    ui::PrintController printing;
    QTemporaryDir dir;
    printing.setSettingsFile(dir.filePath(QStringLiteral("r.ini")));
    printing.setDocument(&document);
    QSignalSpy changed(&printing, &ui::PrintController::settingsChanged);

    QCOMPARE(printing.sheetCount(), 4);
    printing.setRows(2);
    printing.setColumns(2);
    QCOMPARE(printing.sheetCount(), 1);
    QVERIFY(changed.count() >= 2);
    printing.setColumns(1);
    QCOMPARE(printing.sheetCount(), 2);
    printing.setRows(1);
    printing.setRange(QStringLiteral("current"));
    QCOMPARE(printing.sheetCount(), 1);
    printing.setRange(QStringLiteral("from-to"));
    printing.setFrom(2);
    printing.setTo(3);
    QCOMPARE(printing.sheetCount(), 2);
    printing.setRange(QStringLiteral("all"));
    printing.setCopies(3);
    QCOMPARE(printing.sheetCount(), 12);
    // Le décompte est celui de la mise en page du cœur, pas un second calcul.
    QCOMPARE(printing.sheetCount(), core::sheetCount(document.document(), settingsOf(printing), 0, kSheet));
    // Hors bornes : ramené à des valeurs utilisables.
    printing.setCopies(0);
    QCOMPARE(printing.copies(), 1);
    printing.setRows(0);
    QCOMPARE(printing.rows(), 1);
}

void TstUiPrintDialog::settingsSurviveRestart()
{
    QTemporaryDir dir;
    const QString file = dir.filePath(QStringLiteral("r.ini"));
    {
        ui::PrintController first;
        first.setSettingsFile(file);
        first.setRows(2);
        first.setColumns(3);
        first.setFrames(true);
        first.setPageNumbers(true);
        first.setRange(QStringLiteral("from-to"));
        first.setFrom(2);
        first.setTo(4);
        first.setCopies(2);
        first.setMarginMm(12.0);
        first.setGray(true);
        first.setNotes(QStringLiteral("with-pages"));
        first.setNoteBreak(false);
        first.setNoteSeparator(true);
        first.saveSettings();
    }
    {
        // Réglages non enregistrés : ils ne survivent pas.
        ui::PrintController second;
        second.setSettingsFile(file);
        QCOMPARE(second.rows(), 2);
        second.setRows(5);
    }
    ui::PrintController third; // « relance de l'application »
    third.setSettingsFile(file);
    QCOMPARE(third.rows(), 2);
    QCOMPARE(third.columns(), 3);
    QVERIFY(third.frames());
    QVERIFY(third.pageNumbers());
    QCOMPARE(third.range(), QStringLiteral("from-to"));
    QCOMPARE(third.from(), 2);
    QCOMPARE(third.to(), 4);
    QCOMPARE(third.copies(), 2);
    QCOMPARE(third.marginMm(), 12.0);
    QVERIFY(third.gray());
    QCOMPARE(third.notes(), QStringLiteral("with-pages"));
    QVERIFY(!third.noteBreak());
    QVERIFY(third.noteSeparator());
    // Fichier sans réglages : les valeurs par défaut de core::PrintSettings.
    ui::PrintController fresh;
    fresh.setSettingsFile(dir.filePath(QStringLiteral("autre.ini")));
    QCOMPARE(fresh.rows(), 1);
    QCOMPARE(fresh.marginMm(), 5.0);
    QCOMPARE(fresh.range(), QStringLiteral("all"));
    QVERIFY(!fresh.gray());
    // Valeurs illisibles : bornées, jamais une mise en page absurde.
    {
        QFile out(dir.filePath(QStringLiteral("mauvais.ini")));
        QVERIFY(out.open(QIODevice::WriteOnly));
        out.write("[print]\nrows=0\ncolumns=-4\ncopies=0\nrange=nimporte\nnotes=zzz\nmarginMm=-3\n");
    }
    ui::PrintController bad;
    bad.setSettingsFile(dir.filePath(QStringLiteral("mauvais.ini")));
    QCOMPARE(bad.rows(), 1);
    QCOMPARE(bad.columns(), 1);
    QCOMPARE(bad.copies(), 1);
    QCOMPARE(bad.range(), QStringLiteral("all"));
    QCOMPARE(bad.notes(), QStringLiteral("none"));
    QVERIFY(bad.marginMm() >= 0.0);
}

void TstUiPrintDialog::previewMatchesSheets()
{
    ui::DocumentController document;
    fillDocument(document, 5);
    ui::PrintController printing;
    QTemporaryDir dir;
    printing.setSettingsFile(dir.filePath(QStringLiteral("r.ini")));
    printing.setDocument(&document);
    printing.setRows(2);
    printing.setColumns(2);
    printing.setPageNumbers(true);
    printing.setFrames(true);
    QCOMPARE(printing.sheetCount(), 2);
    for (const bool gray : {false, true}) {
        printing.setGray(gray);
        const core::PrintSettings settings = settingsOf(printing);
        const std::vector<core::Sheet> sheets = core::layoutSheets(document.document(), settings, 0, kSheet);
        QCOMPARE(sheets.size(), std::size_t{2});
        for (int i = 0; i < 2; ++i) {
            const QImage shown = printing.previewSheet(i, 72);
            const QImage expected = print::renderSheet(
                document.document(), sheets[static_cast<std::size_t>(i)], settings, kSheet, 72);
            QVERIFY(!shown.isNull());
            QCOMPARE(shown.size(), expected.size());
            QCOMPARE(shown.format(), expected.format());
            QVERIFY2(shown == expected, "l'aperçu n'est pas la feuille imprimée");
            QCOMPARE(hasColor(shown), !gray);
        }
        // La seconde feuille (une seule page) diffère de la première (quatre pages).
        QVERIFY(printing.previewSheet(0, 72) != printing.previewSheet(1, 72));
    }
    // Hors bornes : image vide, jamais une feuille inventée.
    QVERIFY(printing.previewSheet(-1, 72).isNull());
    QVERIFY(printing.previewSheet(2, 72).isNull());
    QVERIFY(printing.previewSheet(0, 0).isNull());
    // Copies : la feuille 1 de la copie 2 est celle de la copie 1.
    printing.setCopies(2);
    QCOMPARE(printing.sheetCount(), 4);
    QVERIFY(printing.previewSheet(2, 72) == printing.previewSheet(0, 72));
}

void TstUiPrintDialog::previewIsLazy()
{
    ui::DocumentController document;
    fillDocument(document, 6);
    ui::PrintController printing;
    QTemporaryDir dir;
    printing.setSettingsFile(dir.filePath(QStringLiteral("r.ini")));
    printing.setDocument(&document);
    QCOMPARE(printing.sheetCount(), 6);
    // Compter les feuilles ne compose rien.
    QCOMPARE(printing.previewRenders(), 0);
    QElapsedTimer timer;
    timer.start();
    QVERIFY(!printing.previewSheet(3, 96).isNull());
    const qint64 oneSheetMs = timer.elapsed();
    QCOMPARE(printing.previewRenders(), 1); // une feuille, pas les six
    // Même feuille, mêmes réglages : gardée.
    QVERIFY(!printing.previewSheet(3, 96).isNull());
    QCOMPARE(printing.previewRenders(), 1);
    // Un réglage change : l'ancienne feuille est périmée, la suivante est recomposée une fois.
    printing.setGray(true);
    QCOMPARE(printing.sheetCount(), 6);
    QCOMPARE(printing.previewRenders(), 1);
    QVERIFY(!printing.previewSheet(3, 96).isNull());
    QCOMPARE(printing.previewRenders(), 2);
    // Le document change (un trait de plus) : recomposée aussi.
    document.showPage(3);
    document.commitStroke(redStroke(500));
    QVERIFY(!printing.previewSheet(3, 96).isNull());
    QCOMPARE(printing.previewRenders(), 3);
    qInfo("mesure : une feuille A4 à 96 ppp, document de 6 pages : %lld ms (Debug)",
          static_cast<long long>(oneSheetMs));
}

void TstUiPrintDialog::printWritesTheSheets()
{
    ui::DocumentController document;
    fillDocument(document, 4);
    ui::PrintController printing;
    QTemporaryDir dir;
    printing.setSettingsFile(dir.filePath(QStringLiteral("r.ini")));
    printing.setDocument(&document);
    QSignalSpy finished(&printing, &ui::PrintController::finished);
    QSignalSpy failed(&printing, &ui::PrintController::failed);
    // Imprimante virtuelle : un fichier PDF, une page par feuille (avec ou sans QPrinter construit).
    const QString out = dir.filePath(QStringLiteral("sortie.pdf"));
    QVERIFY(printing.print(out));
    QCOMPARE(pdfPages(out), 4);
    QCOMPARE(finished.count(), 1);
    QCOMPARE(failed.count(), 0);
    // Les réglages comptent : 2 × 2 sur une feuille, 3 copies.
    printing.setRows(2);
    printing.setColumns(2);
    printing.setCopies(3);
    const QString second = dir.filePath(QStringLiteral("deux.pdf"));
    QVERIFY(printing.print(second));
    QCOMPARE(pdfPages(second), 3);
    // Sans QPrinter ni fichier (L5-C15) : le PDF temporaire des feuilles s'ouvre dans le lecteur du système.
    if (!print::printingAvailable()) {
        UrlCatcher catcher;
        QDesktopServices::setUrlHandler(QStringLiteral("file"), &catcher, "got");
        printing.setPrinter(QString());
        QVERIFY(printing.print());
        QDesktopServices::unsetUrlHandler(QStringLiteral("file"));
        QCOMPARE(catcher.urls.size(), 1);
        QCOMPARE(pdfPages(catcher.urls.first().toLocalFile()), 3);
        QFile::remove(catcher.urls.first().toLocalFile());
        QCOMPARE(failed.count(), 0);
    }
    const qsizetype refused = failed.count();
    // Fichier impossible à écrire : refus annoncé, aucun fichier fantôme.
    const QString impossible = dir.filePath(QStringLiteral("absent/sortie.pdf"));
    QVERIFY(!printing.print(impossible));
    QCOMPARE(failed.count(), refused + 1);
    QVERIFY(!QFile::exists(impossible));
}

void TstUiPrintDialog::dialogEditsSettingsAndShowsTheSheet()
{
    Shell shell;
    QVERIFY(shell.start(4));
    QMetaObject::invokeMethod(shell.dialog, "open");
    QTRY_VERIFY(shell.dialog->property("opened").toBool());

    auto* rows = shell.find<QQuickItem>("rowsBox");
    auto* columns = shell.find<QQuickItem>("columnsBox");
    auto* preview = shell.find<QQuickItem>("previewImage");
    auto* label = shell.find<QQuickItem>("sheetLabel");
    auto* next = shell.find<QQuickItem>("nextSheetButton");
    auto* prev = shell.find<QQuickItem>("previousSheetButton");
    auto* gray = shell.find<QQuickItem>("grayRadio");
    QVERIFY(rows && columns && preview && label && next && prev && gray);
    QCOMPARE(label->property("text").toString(), QStringLiteral("Feuille 1 sur 4"));
    QTRY_COMPARE(preview->property("status").toInt(), 1); // Image.Ready
    // Le rendu paresseux : seule la feuille montrée est composée.
    QCOMPARE(shell.printing->previewRenders(), 1);

    // Suivante / précédente.
    QVERIFY(!prev->property("enabled").toBool());
    QTest::mouseClick(&shell.window, Qt::LeftButton, {},
                      next->mapToScene(QPointF(next->width() / 2, next->height() / 2)).toPoint());
    QCOMPARE(label->property("text").toString(), QStringLiteral("Feuille 2 sur 4"));
    QTRY_COMPARE(shell.printing->previewRenders(), 2);

    // Les contrôles écrivent dans le contrôleur, et inversement.
    rows->setProperty("value", 2);
    columns->setProperty("value", 2);
    QCOMPARE(shell.printing->rows(), 2);
    QCOMPARE(shell.printing->columns(), 2);
    QCOMPARE(shell.printing->sheetCount(), 1);
    QCOMPARE(label->property("text").toString(), QStringLiteral("Feuille 1 sur 1"));
    shell.printing->setRows(1);
    QCOMPARE(rows->property("value").toInt(), 1);
    QTest::mouseClick(&shell.window, Qt::LeftButton, {},
                      gray->mapToScene(QPointF(gray->width() / 2, gray->height() / 2)).toPoint());
    QVERIFY(shell.printing->gray());

    // L'aperçu affiché est l'image de previewSheet.
    QTRY_COMPARE(preview->property("status").toInt(), 1);
    QVERIFY(preview->property("sourceSize").toSize().width() > 0);

    // Exporter en PDF : appel de l'échange avec l'URL choisie.
    QVERIFY(QMetaObject::invokeMethod(shell.dialog, "exportTo",
                                      Q_ARG(QVariant, QUrl::fromLocalFile("/tmp/x.pdf"))));
    QCOMPARE(shell.root->property("calls").toStringList(),
             QStringList{QStringLiteral("export:file:///tmp/x.pdf")});

    // « Enregistrer les réglages et quitter » : réglages gardés, boîte fermée.
    auto* save = shell.find<QQuickItem>("saveAndQuitButton");
    QVERIFY(save);
    QTest::mouseClick(&shell.window, Qt::LeftButton, {},
                      save->mapToScene(QPointF(save->width() / 2, save->height() / 2)).toPoint());
    QTRY_VERIFY(!shell.dialog->property("opened").toBool());
    ui::PrintController again;
    again.setSettingsFile(shell.printing->settingsFile());
    QVERIFY(again.gray());

    // Pour la relecture à l'œil : ACTCRAFT_GRAB_DIR (jamais posée en preuve).
    const QString grab = qEnvironmentVariable("ACTCRAFT_GRAB_DIR");
    if (!grab.isEmpty()) {
        QMetaObject::invokeMethod(shell.dialog, "open");
        QTRY_VERIFY(shell.dialog->property("opened").toBool());
        QTest::qWait(300);
        shell.window.grabWindow().save(QDir(grab).filePath(QStringLiteral("boite-impression.png")));
    }
}

// L5-C15 : sans QPrinter, « Imprimer » ouvre un PDF des feuilles des réglages (2 × 2 : une feuille pour
// quatre pages), pas celui du document entier (quatre pages) que donnerait FileExchange::printThroughPdf.
void TstUiPrintDialog::printButtonPrintsTheSheetsOfTheSettings()
{
    if (print::printingAvailable()) {
        QSKIP("QPrinter construit : « Imprimer » part vers l'imprimante, pas vers un PDF");
    }
    Shell shell;
    QVERIFY(shell.start(4));
    shell.printing->setRows(2);
    shell.printing->setColumns(2);
    QCOMPARE(shell.printing->sheetCount(), 1);
    QMetaObject::invokeMethod(shell.dialog, "open");
    QTRY_VERIFY(shell.dialog->property("opened").toBool());
    UrlCatcher catcher;
    QDesktopServices::setUrlHandler(QStringLiteral("file"), &catcher, "got");
    QSignalSpy finished(shell.printing, &ui::PrintController::finished);
    auto* button = shell.find<QQuickItem>("printButton");
    QVERIFY(button);
    QTest::mouseClick(&shell.window, Qt::LeftButton, {},
                      button->mapToScene(QPointF(button->width() / 2, button->height() / 2)).toPoint());
    QTRY_COMPARE(catcher.urls.size(), 1);
    QDesktopServices::unsetUrlHandler(QStringLiteral("file"));
    const QString path = catcher.urls.first().toLocalFile();
    QCOMPARE(pdfPages(path), 1);
    QCOMPARE(finished.count(), 1);
    QVERIFY(shell.root->property("calls").toStringList().isEmpty()); // jamais printThroughPdf
    QTRY_VERIFY(!shell.dialog->property("opened").toBool());
    QFile::remove(path);
}

QTEST_MAIN(TstUiPrintDialog)
#include "tst_ui_printdialog.moc"
