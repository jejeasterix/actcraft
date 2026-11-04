#include "startup.h"

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/export/pdfexport.h>
#include <actcraft/export/pdfimport.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documenttabs.h>
#include <actcraft/ui/fileexchange.h>

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

core::Stroke redStroke()
{
    core::Stroke stroke;
    stroke.color = 0xFFFF0000u;
    stroke.baseWidth = 40;
    for (int i = 0; i <= 20; ++i) {
        stroke.points.push_back(core::StrokePoint{100.0 + i * 20.0, 300.0, 0.5f});
    }
    return stroke;
}

// L'interface réelle (Main.qml) : les commandes passent par leurs gestionnaires, comme au menu et aux
// scénarios.
struct Shell
{
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::FileExchange* exchange = nullptr;
    ui::DocumentTabs* tabs = nullptr;
    ui::DocumentController* document = nullptr;

    bool start()
    {
        window = loadMainWindow(engine, StartupOptions{});
        if (!window || !QTest::qWaitForWindowExposed(window)) {
            return false;
        }
        exchange = window->findChild<ui::FileExchange*>(QStringLiteral("appExchange"));
        tabs = window->findChild<ui::DocumentTabs*>(QStringLiteral("appTabs"));
        document = window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        return exchange && tabs && document;
    }
    bool run(const QString& id, const QVariant& argument = QVariant())
    {
        QVariant result;
        return QMetaObject::invokeMethod(window, "runCommand", Q_RETURN_ARG(QVariant, result),
                                         Q_ARG(QVariant, QVariant(id)), Q_ARG(QVariant, argument)) &&
               result.toBool();
    }
    // Trois pages de 800 × 600, un trait rouge sur la première.
    void threePages()
    {
        document->newDocument(800, 600);
        document->commitStroke(redStroke());
        document->nextPage();
        document->nextPage();
    }
};

bool hasRed(const QImage& image)
{
    for (int y = 0; y < image.height(); y += 2) {
        for (int x = 0; x < image.width(); x += 2) {
            const QColor c = image.pixelColor(x, y);
            if (c.red() > 200 && c.green() < 80 && c.blue() < 80) {
                return true;
            }
        }
    }
    return false;
}

// Reçoit l'ouverture du PDF d'impression à la place du système.
struct UrlCatcher : QObject
{
    Q_OBJECT
public:
    QList<QUrl> urls;
public slots:
    void got(const QUrl& url) { urls.push_back(url); }
};

} // namespace

class TestUiFileExchange : public QObject
{
    Q_OBJECT
private slots:
    void exportPdfCommandWritesFile()
    {
        Shell s;
        QVERIFY(s.start());
        s.threePages();
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("sortie.pdf"));
        QSignalSpy done(s.exchange, &ui::FileExchange::finished);
        QVERIFY(s.run(QStringLiteral("export.pdf"), path));
        QTRY_VERIFY_WITH_TIMEOUT(done.count() == 1 && !s.exchange->isBusy(), 15000);
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(exporting::countPdfPages(file.readAll()), 3);
        QCOMPARE(s.document->pageCount(), 3); // le paperboard n'est pas touché
    }

    void exportPageImageCommand()
    {
        Shell s;
        QVERIFY(s.start());
        s.threePages();
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        // Page affichée : la troisième, vierge.
        QSignalSpy done(s.exchange, &ui::FileExchange::finished);
        const QString blank = dir.filePath(QStringLiteral("page3.png"));
        QVERIFY(s.run(QStringLiteral("export.page-image"), blank));
        QTRY_VERIFY_WITH_TIMEOUT(done.count() == 1, 15000);
        QImage third(blank);
        QCOMPARE(third.size(), QSize(800, 600));
        QVERIFY(!hasRed(third));
        // Première page : le trait rouge y est.
        s.document->showPage(0);
        const QString inked = dir.filePath(QStringLiteral("page1.png"));
        QVERIFY(s.run(QStringLiteral("export.page-image"), inked));
        QTRY_VERIFY_WITH_TIMEOUT(done.count() == 2, 15000);
        QVERIFY(hasRed(QImage(inked)));
    }

    // Un chemin natif (barres obliques inverses, lettre de lecteur) donné en texte par une commande doit
    // aboutir au même fichier que le chemin à barres obliques.
    void exportPageImageWithNativeSeparators()
    {
        Shell s;
        QVERIFY(s.start());
        s.threePages();
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("natif.png"));
        QSignalSpy done(s.exchange, &ui::FileExchange::finished);
        QSignalSpy failed(s.exchange, &ui::FileExchange::failed);
        QVERIFY(s.run(QStringLiteral("export.page-image"), QDir::toNativeSeparators(path)));
        QTRY_VERIFY_WITH_TIMEOUT(done.count() + failed.count() == 1, 15000);
        QCOMPARE(failed.count(), 0);
        QVERIFY(QFileInfo::exists(path));
    }

    // Un chemin sans lettre de lecteur (relatif ici ; absolu « /… » sous macOS et Linux) n'a pas de schéma :
    // il se résout tel quel depuis le dossier courant.
    void exportPageImageWithRelativePath()
    {
        Shell s;
        QVERIFY(s.start());
        s.threePages();
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString previous = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));
        QSignalSpy done(s.exchange, &ui::FileExchange::finished);
        QSignalSpy failed(s.exchange, &ui::FileExchange::failed);
        const bool ran = s.run(QStringLiteral("export.page-image"), QStringLiteral("relatif.png"));
        const bool settled = QTest::qWaitFor([&] { return done.count() + failed.count() == 1; }, 15000);
        QDir::setCurrent(previous);
        QVERIFY(ran);
        QVERIFY(settled);
        QCOMPARE(failed.count(), 0);
        QVERIFY(QFileInfo::exists(dir.filePath(QStringLiteral("relatif.png"))));
    }

    // « # » dans un nom de dossier ou de fichier fait partie du chemin, pas d'un fragment d'URL, avec des
    // barres obliques comme avec des barres obliques inverses.
    void exportPageImageWithHashInPath()
    {
        Shell s;
        QVERIFY(s.start());
        s.threePages();
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("Projets C#")));
        const QStringList paths = {
            dir.filePath(QStringLiteral("Projets C#/a#1.png")),
            QDir::toNativeSeparators(dir.filePath(QStringLiteral("Projets C#/b#2.png")))};
        for (const QString& path : paths) {
            QSignalSpy done(s.exchange, &ui::FileExchange::finished);
            QSignalSpy failed(s.exchange, &ui::FileExchange::failed);
            QVERIFY(s.run(QStringLiteral("export.page-image"), path));
            QTRY_VERIFY_WITH_TIMEOUT(done.count() + failed.count() == 1, 15000);
            QCOMPARE(failed.count(), 0);
            QVERIFY2(QFileInfo::exists(QDir::fromNativeSeparators(path)), qPrintable(path));
        }
    }

    void importPdfOpensATab()
    {
        if (!exporting::pdfImportAvailable()) {
            QSKIP("Qt PDF absent : import PDF indisponible dans cette construction");
        }
        Shell s;
        QVERIFY(s.start());
        s.threePages();
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("trois.pdf"));
        QSignalSpy exported(s.exchange, &ui::FileExchange::finished);
        QVERIFY(s.run(QStringLiteral("export.pdf"), path));
        QTRY_VERIFY_WITH_TIMEOUT(exported.count() == 1 && !s.exchange->isBusy(), 15000);
        QCOMPARE(s.tabs->rowCount(), 1);
        QVERIFY(s.run(QStringLiteral("file.import-pdf"), path));
        QTRY_COMPARE_WITH_TIMEOUT(s.tabs->rowCount(), 2, 15000);
        QCOMPARE(s.tabs->currentIndex(), 1);
        QCOMPARE(s.document->pageCount(), 3);
        QCOMPARE(s.document->title(), QStringLiteral("trois"));
        // Retour au premier onglet : le paperboard d'origine est intact.
        s.tabs->activate(0);
        QCOMPARE(s.document->pageCount(), 3);
        QVERIFY(s.document->isModified());
    }

    void importPdfFailureKeepsTheTabs()
    {
        if (!exporting::pdfImportAvailable()) {
            QSKIP("Qt PDF absent : import PDF indisponible dans cette construction");
        }
        Shell s;
        QVERIFY(s.start());
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("faux.pdf"));
        QFile bad(path);
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("ceci n'est pas un PDF");
        bad.close();
        QSignalSpy failed(s.exchange, &ui::FileExchange::failed);
        QVERIFY(s.run(QStringLiteral("file.import-pdf"), path));
        QTRY_VERIFY_WITH_TIMEOUT(failed.count() == 1 && !s.exchange->isBusy(), 15000);
        QVERIFY(failed.first().first().toString().contains(QStringLiteral("Import PDF impossible")));
        QCOMPARE(s.tabs->rowCount(), 1);
    }

    void printThroughPdfOpensThePdfOfTheDocument()
    {
        Shell s;
        QVERIFY(s.start());
        s.threePages();
        UrlCatcher catcher;
        QDesktopServices::setUrlHandler(QStringLiteral("file"), &catcher, "got");
        QSignalSpy done(s.exchange, &ui::FileExchange::finished);
        // « Imprimer » ouvre la boîte (L5-C15) ; son repli sans QPrinter appelle printThroughPdf : on
        // appelle cette voie directement, la boîte est couverte par tst_ui_filemenu.
        QMetaObject::invokeMethod(s.exchange, "printThroughPdf");
        QTRY_VERIFY_WITH_TIMEOUT(done.count() == 1 && !s.exchange->isBusy(), 15000);
        QDesktopServices::unsetUrlHandler(QStringLiteral("file"));
        QCOMPARE(catcher.urls.size(), 1);
        const QString path = catcher.urls.first().toLocalFile();
        QVERIFY(path.startsWith(QDir::tempPath()));
        QVERIFY(path.endsWith(QStringLiteral(".pdf")));
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(exporting::countPdfPages(file.readAll()), 3);
        file.close();
        QFile::remove(path);
    }

    void busyWhileExporting()
    {
        Shell s;
        QVERIFY(s.start());
        s.document->loadGeneratedPages(6, true);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QSignalSpy busy(s.exchange, &ui::FileExchange::busyChanged);
        QSignalSpy done(s.exchange, &ui::FileExchange::finished);
        QSignalSpy failed(s.exchange, &ui::FileExchange::failed);
        s.exchange->exportPdf(QUrl::fromLocalFile(dir.filePath(QStringLiteral("a.pdf"))));
        // Rendu sur un autre fil : l'appel est déjà revenu que le travail dure encore.
        QVERIFY(s.exchange->isBusy());
        QCOMPARE(busy.count(), 1);
        // Un second export pendant le premier est refusé, visiblement.
        s.exchange->exportPdf(QUrl::fromLocalFile(dir.filePath(QStringLiteral("b.pdf"))));
        QCOMPARE(failed.count(), 1);
        QVERIFY(s.exchange->isBusy());
        QTRY_VERIFY_WITH_TIMEOUT(!s.exchange->isBusy(), 30000);
        QCOMPARE(done.count(), 1);
        QCOMPARE(busy.count(), 2);
        QVERIFY(QFile::exists(dir.filePath(QStringLiteral("a.pdf"))));
        QVERIFY(!QFile::exists(dir.filePath(QStringLiteral("b.pdf"))));
    }

    // L5-C13 : un export lancé pendant un soulèvement contient ce que l'utilisateur voit (les objets
    // soulevés), sans perdre ni dupliquer l'objet, et sans ajouter plus d'une étape d'annulation.
    void exportDuringLiftContainsLiftedObjects()
    {
        Shell s;
        QVERIFY(s.start());
        s.document->newDocument(800, 600);
        s.document->commitStroke(redStroke());
        const core::ObjectId id = s.document->document().page(0).objects.front().common.id;
        const auto lifted = s.document->liftObjects({id});
        QCOMPARE(lifted.size(), std::size_t(1));
        QVERIFY(s.document->isLifting());
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QSignalSpy done(s.exchange, &ui::FileExchange::finished);
        const QString path = dir.filePath(QStringLiteral("soulevee.png"));
        QVERIFY(s.run(QStringLiteral("export.page-image"), path));
        QTRY_VERIFY_WITH_TIMEOUT(done.count() == 1 && !s.exchange->isBusy(), 15000);
        QVERIFY(hasRed(QImage(path)));
        QVERIFY(!s.document->isLifting());
        QCOMPARE(s.document->document().page(0).objects.size(), std::size_t(1));
        // Une seule étape d'annulation : le trait seul (le dépôt n'en ajoute pas).
        s.document->undo();
        QCOMPARE(s.document->document().page(0).objects.size(), std::size_t(0));
        QVERIFY(!s.document->canUndo());
    }

    // L5-C15 : Fichier > Imprimer règle l'édition en cours avant d'ouvrir la boîte : l'objet soulevé est
    // déposé, donc l'aperçu et le PDF imprimé le contiennent.
    void printSettlesTheLiftBeforeOpeningTheDialog()
    {
        Shell s;
        QVERIFY(s.start());
        s.document->newDocument(800, 600);
        s.document->commitStroke(redStroke());
        const core::ObjectId id = s.document->document().page(0).objects.front().common.id;
        QCOMPARE(s.document->liftObjects({id}).size(), std::size_t(1));
        QVERIFY(s.document->isLifting());
        QVERIFY(s.run(QStringLiteral("file.print")));
        QVERIFY(!s.document->isLifting());
        QCOMPARE(s.document->document().page(0).objects.size(), std::size_t(1));
        QObject* dialog = s.window->findChild<QObject*>(QStringLiteral("printSetupDialog"));
        QVERIFY(dialog);
        QTRY_VERIFY(dialog->property("visible").toBool());
    }

    void failedExportReportsAndReleasesBusy()
    {
        Shell s;
        QVERIFY(s.start());
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QFile blocker(dir.filePath(QStringLiteral("fichier")));
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.close();
        QSignalSpy failed(s.exchange, &ui::FileExchange::failed);
        // Le « dossier » de destination est un fichier : l'écriture échoue.
        s.exchange->exportPdf(QUrl::fromLocalFile(dir.filePath(QStringLiteral("fichier/sortie.pdf"))));
        QTRY_VERIFY_WITH_TIMEOUT(failed.count() == 1 && !s.exchange->isBusy(), 15000);
        QVERIFY(failed.first().first().toString().contains(QStringLiteral("Export PDF impossible")));
    }
};

QTEST_MAIN(TestUiFileExchange)
#include "tst_ui_fileexchange.moc"
