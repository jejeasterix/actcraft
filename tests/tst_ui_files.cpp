#include "support/flipchartwriter.h"
#include "support/peakmemory.h"

#include <actcraft/core/document.h>
#include <actcraft/formats/nativefile.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documentio.h>

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QSemaphore>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QThreadPool>

#include <cstdint>
#include <string>
#include <vector>

using namespace actcraft;

namespace {

core::Stroke stroke(core::Argb color, double y)
{
    core::Stroke s;
    s.points = {core::StrokePoint{100, y, 0.5f}, core::StrokePoint{300, y, 0.5f}};
    s.baseWidth = 8;
    s.color = color;
    return s;
}

core::Stroke redStroke()
{
    return stroke(0xFFE53935u, 100);
}

bool writeFile(const QString& path, const QByteArray& bytes)
{
    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size();
}

QString hex(std::uint64_t value)
{
    return QStringLiteral("%1").arg(value, 16, 16, QLatin1Char('0'));
}

// Empreinte du document écrit dans le fichier (relu par le lecteur du format natif).
QString savedFingerprint(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const formats::DecodeResult r = formats::decodeActcraft(file.readAll());
    return r.ok ? hex(core::fingerprint(r.document)) : QString();
}

} // namespace

// Ouverture et enregistrement hors du fil principal (décision 17, règle de fluidité 1).
class TstUiFiles : public QObject
{
    Q_OBJECT

private slots:
    // En tête : le pic de mémoire du processus ne descend jamais, aucun test avant celui-ci ne doit l'avoir
    // monté. Huit photos de 12 Mpx distinctes : décodées en grand pour les miniatures, elles tiendraient
    // ensemble 8 × 48 Mo ; décodées réduites (une miniature fait 320 px de large), quelques Mo.
    void thumbnailImagesAreDecodedReduced()
    {
        QByteArray jpeg;
        {
            QImage photo(4000, 3000, QImage::Format_RGB32);
            photo.fill(qRgb(30, 120, 200));
            QBuffer buffer(&jpeg);
            QVERIFY(buffer.open(QIODevice::WriteOnly));
            QVERIFY(photo.save(&buffer, "JPG"));
        }
        core::Document document;
        for (int i = 0; i < 8; ++i) {
            QByteArray bytes = jpeg;
            bytes.append(static_cast<char>(i)); // après la fin de l'image : contenu distinct, même image
            core::ImageObject image;
            image.bounds = core::RectF{10.0 + 100.0 * i, 10, 96, 72};
            image.sourceWidth = 4000;
            image.sourceHeight = 3000;
            image.resource = document.addResource(bytes, "jpg");
            core::PageObject object = image;
            object.common.id = document.allocateId();
            document.page(0).objects.push_back(object);
        }
        QCOMPARE(document.resources().size(), std::size_t{8});
        QTemporaryDir dir;
        const QUrl url = QUrl::fromLocalFile(dir.filePath(QStringLiteral("photos.actcraft")));
        const qint64 before = peakMemoryBytes();
        QVERIFY(ui::saveDocument(document, url).ok);
        const qint64 grown = peakMemoryBytes() - before;
        QVERIFY2(grown < 150ll * 1024 * 1024,
                 qPrintable(QStringLiteral("pic monté de %1 Mo").arg(grown >> 20)));
    }

    void saveThenOpenKeepsTheFingerprint()
    {
        QTemporaryDir dir;
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        controller.nextPage();
        const QString saved = controller.property("fingerprint").toString();
        const QUrl url = QUrl::fromLocalFile(dir.filePath(QStringLiteral("essai.actcraft")));
        QTest::ignoreMessage(QtInfoMsg, qPrintable(QStringLiteral("ACTCRAFT ENREGISTRE ") + saved));
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        controller.saveToUrl(url);
        QVERIFY(controller.property("busy").toBool());
        QVERIFY(savedSpy.wait(10000));
        QCOMPARE(savedSpy.first().at(0).toBool(), true);
        QVERIFY(!controller.property("busy").toBool());
        QVERIFY(!controller.isModified());
        QCOMPARE(controller.fileUrl(), url);
        QCOMPARE(controller.title(), QStringLiteral("essai"));
        controller.newDocument(1024, 768);
        QTest::ignoreMessage(QtInfoMsg, qPrintable(QStringLiteral("ACTCRAFT OUVERT ") + saved));
        QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
        controller.openUrl(url);
        QVERIFY(loaded.wait(10000));
        QCOMPARE(loaded.first().at(0).toBool(), true);
        QCOMPARE(controller.property("fingerprint").toString(), saved);
        QCOMPARE(controller.pageLabel(), QStringLiteral("1 / 2"));
        QCOMPARE(controller.fileUrl(), url);
        QVERIFY(!controller.isModified());
    }

    // Le fil d'exécution est relevé PAR l'écriture et la lecture elles-mêmes (point d'injection :
    // offMainThread des résultats) : une écriture synchrone émise ensuite en file serait vue (relecture du
    // plan, tour 1).
    void ioRunsOffTheMainThread()
    {
        QTemporaryDir dir;
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        const QUrl url = QUrl::fromLocalFile(dir.filePath(QStringLiteral("fil.actcraft")));
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        QSignalSpy ioSpy(&controller, &ui::DocumentController::lastIoChanged);
        controller.saveToUrl(url);
        QVERIFY(savedSpy.wait(10000));
        QVERIFY(controller.property("lastIoOffMainThread").toBool());
        QCOMPARE(ioSpy.count(), 1);
        QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
        controller.openUrl(url);
        QVERIFY(loaded.wait(10000));
        QVERIFY(controller.property("lastIoOffMainThread").toBool());
        QCOMPARE(ioSpy.count(), 2);
        // Appel direct sur le fil principal : le résultat le dit (le relevé n'est pas une constante).
        QVERIFY(!ui::saveDocument(controller.document(), url).offMainThread);
        QVERIFY(!ui::loadDocument(url).offMainThread);
    }

    void thumbnailsAreWrittenForEveryPage()
    {
        QTemporaryDir dir;
        core::Document document(std::vector<core::Page>(3));
        const QUrl url = QUrl::fromLocalFile(dir.filePath(QStringLiteral("m.actcraft")));
        QVERIFY(ui::saveDocument(document, url).ok);
        QFile file(url.toLocalFile());
        QVERIFY(file.open(QIODevice::ReadOnly));
        const formats::DecodeResult r = formats::decodeActcraft(file.readAll());
        QVERIFY(r.ok);
        QCOMPARE(r.thumbnails.size(), std::size_t{3});
        QImage thumb;
        QVERIFY(thumb.loadFromData(r.thumbnails[2], "PNG"));
        QCOMPARE(thumb.size(), QSize(320, 240));
    }

    void openingAFlipchartGivesADocumentWithoutFile()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("cours.flipchart"));
        QVERIFY(writeFile(path, fctest::sampleFlipchart()));
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
        controller.openUrl(QUrl::fromLocalFile(path));
        QVERIFY(loaded.wait(10000));
        QCOMPARE(loaded.first().at(0).toBool(), true);
        QCOMPARE(controller.pageCount(), 2);
        QVERIFY(controller.fileUrl().isEmpty());
        QCOMPARE(controller.title(), QStringLiteral("cours"));
        QCOMPARE(controller.property("suggestedFileName").toString(), QStringLiteral("cours.actcraft"));
        QCOMPARE(controller.property("warnings").toStringList().size(), 1); // la formule non lue
    }

    void unreadableFileKeepsTheDocumentAndReportsWhy()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("faux.actcraft"));
        QVERIFY(writeFile(path, "ni zip ni paperboard"));
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        const QString before = controller.property("fingerprint").toString();
        QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
        controller.openUrl(QUrl::fromLocalFile(path));
        QVERIFY(loaded.wait(10000));
        QCOMPARE(loaded.first().at(0).toBool(), false);
        QVERIFY(!controller.property("lastError").toString().isEmpty());
        QCOMPARE(controller.property("fingerprint").toString(), before);
        QVERIFY(controller.isModified());
    }

    void secondOperationIsRefusedWhileBusy()
    {
        QTemporaryDir dir;
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        controller.saveToUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("a.actcraft"))));
        controller.saveToUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("b.actcraft"))));
        QCOMPARE(savedSpy.count(), 0); // le refus aussi arrive en file, jamais pendant l'appel
        QTRY_COMPARE_WITH_TIMEOUT(savedSpy.count(), 2, 10000);
        // Le refus, mis en file aussitôt, arrive AVANT la fin de l'écriture de « a ».
        QCOMPARE(savedSpy.at(0).at(0).toBool(), false);
        QCOMPARE(savedSpy.at(1).at(0).toBool(), true);
        QVERIFY(QFile::exists(dir.filePath(QStringLiteral("a.actcraft"))));
        QVERIFY(!QFile::exists(dir.filePath(QStringLiteral("b.actcraft"))));
    }

    void argumentsBecomeUrlsAndTheDefaultFolderExists()
    {
        ui::DocumentController controller;
        QVERIFY(controller.urlFromArgument(QStringLiteral("content://fournisseur/doc")).scheme() ==
                QStringLiteral("content"));
        QVERIFY(controller.urlFromArgument(QVariant()).isEmpty());
        const QUrl folder = controller.defaultFolderUrl();
        QVERIFY(folder.isLocalFile());
        QVERIFY(QFileInfo(folder.toLocalFile()).isDir());
    }

    // La page du banc garde « modifié » hors de la pile (L2-T15) : un enregistrement réussi rend l'état
    // propre en entier, pile ET drapeau.
    void successfulSaveAlsoClearsTheChangeOutsideTheStack()
    {
        QTemporaryDir dir;
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        controller.loadGeneratedFirstPage(); // pile vidée, « modifié » gardé hors de la pile
        QVERIFY(controller.isModified());
        QVERIFY(!controller.canUndo());
        QSignalSpy modifiedSpy(&controller, &ui::DocumentController::modifiedChanged);
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        controller.saveToUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("banc.actcraft"))));
        QVERIFY(savedSpy.wait(10000));
        QCOMPARE(savedSpy.first().at(0).toBool(), true);
        QVERIFY(!controller.isModified());
        QCOMPARE(modifiedSpy.count(), 1);
    }

    // Le fichier reçoit la copie prise à l'appel, pas le document tel qu'il est quand le fil démarre : le fil
    // est tenu occupé jusqu'après un trait. Le trait n'est pas enregistré, le document reste modifié.
    void strokeDuringSaveIsNotSavedAndKeepsTheDocumentModified()
    {
        QTemporaryDir dir;
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        controller.commitStroke(redStroke());
        const QString before = controller.property("fingerprint").toString();
        const QString path = dir.filePath(QStringLiteral("copie.actcraft"));
        QThreadPool* pool = QThreadPool::globalInstance();
        const int threads = pool->maxThreadCount();
        QSemaphore started;
        QSemaphore gate;
        for (int i = 0; i < threads; ++i) {
            pool->start([&started, &gate] {
                started.release();
                gate.acquire();
            });
        }
        QVERIFY(started.tryAcquire(threads, 10000));
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        controller.saveToUrl(QUrl::fromLocalFile(path));
        controller.commitStroke(stroke(0xFF1E88E5u, 200)); // pendant l'enregistrement
        const QString after = controller.property("fingerprint").toString();
        QVERIFY(after != before);
        gate.release(threads);
        QVERIFY(savedSpy.wait(10000));
        QVERIFY(pool->waitForDone(10000));
        QCOMPARE(savedSpy.first().at(0).toBool(), true);
        QCOMPARE(savedFingerprint(path), before);
        QVERIFY(controller.isModified());
        QCOMPARE(controller.property("fingerprint").toString(), after);
    }

    // « Nouveau » pendant un enregistrement : le document neuf n'est pas rattaché au fichier, même s'il a
    // l'empreinte du document enregistré (deux paperboards vierges).
    void newDocumentDuringSaveIsNotTiedToTheFile()
    {
        QTemporaryDir dir;
        render::FixedPageItem fixed;
        ui::DocumentController controller;
        controller.setFixedPage(&fixed);
        const QString blank = controller.property("fingerprint").toString();
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        controller.saveToUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("vierge.actcraft"))));
        controller.newDocument(1024, 768);
        QCOMPARE(controller.property("fingerprint").toString(), blank);
        QVERIFY(savedSpy.wait(10000));
        QCOMPARE(savedSpy.first().at(0).toBool(), true);
        QVERIFY(controller.fileUrl().isEmpty());
        QCOMPARE(controller.title(), QStringLiteral("Sans titre"));
        QVERIFY(!controller.property("busy").toBool());
    }
};

QTEST_MAIN(TstUiFiles)
#include "tst_ui_files.moc"
