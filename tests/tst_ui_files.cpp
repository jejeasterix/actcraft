#include "support/flipchartwriter.h"
#include "support/peakmemory.h"

#include <actcraft/core/document.h>
#include <actcraft/formats/nativefile.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documentio.h>

#include <QBuffer>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QSemaphore>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QThreadPool>

#include <cstdint>
#include <memory>
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

QByteArray readAll(const QString& path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

// Paperboard natif de deux pages écrit sur le fil du test (fichier à ouvrir).
QUrl writeTwoPages(const QTemporaryDir& dir, const QString& name)
{
    const QUrl url = QUrl::fromLocalFile(dir.filePath(name));
    const core::Document document(std::vector<core::Page>(2));
    return ui::saveDocument(document, url).ok ? url : QUrl();
}

// Tient tous les fils du pool global occupés jusqu'à release() : une opération lancée entre-temps attend.
class PoolBlocker
{
public:
    // Les sémaphores sont tenus par un shared_ptr capturé par valeur : une tâche qui survivrait au bloqueur
    // (délai dépassé) ne touche jamais un objet détruit. Le destructeur relâche la porte quel que soit le
    // chemin de sortie du test (échec d'un QVERIFY compris), puis attend le pool de façon bornée.
    PoolBlocker() : m_threads(QThreadPool::globalInstance()->maxThreadCount()), m_state(std::make_shared<State>())
    {
        for (int i = 0; i < m_threads; ++i) {
            QThreadPool::globalInstance()->start([state = m_state] {
                state->started.release();
                state->gate.acquire();
            });
        }
    }
    ~PoolBlocker()
    {
        release();
        QThreadPool::globalInstance()->waitForDone(60000);
    }
    PoolBlocker(const PoolBlocker&) = delete;
    PoolBlocker& operator=(const PoolBlocker&) = delete;

    bool allBlocked() { return m_state->started.tryAcquire(m_threads, 60000); }
    void release()
    {
        if (!m_released) {
            m_released = true;
            m_state->gate.release(m_threads);
        }
    }

private:
    struct State
    {
        QSemaphore started;
        QSemaphore gate;
    };
    int m_threads;
    std::shared_ptr<State> m_state;
    bool m_released = false;
};

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
        QVERIFY(savedSpy.wait(60000));
        QCOMPARE(savedSpy.first().at(0).toBool(), true);
        QVERIFY(!controller.property("busy").toBool());
        QVERIFY(!controller.isModified());
        QCOMPARE(controller.fileUrl(), url);
        QCOMPARE(controller.title(), QStringLiteral("essai"));
        controller.newDocument(1024, 768);
        QTest::ignoreMessage(QtInfoMsg, qPrintable(QStringLiteral("ACTCRAFT OUVERT ") + saved));
        QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
        controller.openUrl(url);
        QVERIFY(loaded.wait(60000));
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
        QVERIFY(savedSpy.wait(60000));
        QVERIFY(controller.property("lastIoOffMainThread").toBool());
        QCOMPARE(ioSpy.count(), 1);
        QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
        controller.openUrl(url);
        QVERIFY(loaded.wait(60000));
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
        QVERIFY(loaded.wait(60000));
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
        QVERIFY(loaded.wait(60000));
        QCOMPARE(loaded.first().at(0).toBool(), false);
        QVERIFY(!controller.property("lastError").toString().isEmpty());
        QCOMPARE(controller.property("fingerprint").toString(), before);
        QVERIFY(controller.isModified());
        // Une ouverture réussie efface la dernière erreur.
        const QUrl good = writeTwoPages(dir, QStringLiteral("bon.actcraft"));
        QVERIFY(!good.isEmpty());
        controller.openUrl(good);
        QVERIFY(loaded.wait(60000));
        QCOMPARE(loaded.last().at(0).toBool(), true);
        QVERIFY(controller.property("lastError").toString().isEmpty());
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
        QTRY_COMPARE_WITH_TIMEOUT(savedSpy.count(), 2, 60000);
        // Le refus, mis en file aussitôt, arrive AVANT la fin de l'écriture de « a ».
        QCOMPARE(savedSpy.at(0).at(0).toBool(), false);
        QCOMPARE(savedSpy.at(1).at(0).toBool(), true);
        QVERIFY(QFile::exists(dir.filePath(QStringLiteral("a.actcraft"))));
        QVERIFY(!QFile::exists(dir.filePath(QStringLiteral("b.actcraft"))));
    }

    // Le dossier par défaut est créé sous le dossier de test, jamais sous le vrai Documents de l'utilisateur.
    void argumentsBecomeUrlsAndTheDefaultFolderExists()
    {
        const QString real =
            QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QStringLiteral("/ActCraft");
        const bool realExisted = QFileInfo::exists(real);
        QTemporaryDir documents;
        ui::DocumentController controller;
        controller.setDocumentsRoot(documents.path());
        QVERIFY(controller.urlFromArgument(QStringLiteral("content://fournisseur/doc")).scheme() ==
                QStringLiteral("content"));
        QVERIFY(controller.urlFromArgument(QVariant()).isEmpty());
        const QUrl folder = controller.defaultFolderUrl();
        QVERIFY(folder.isLocalFile());
        QCOMPARE(folder.toLocalFile(), documents.filePath(QStringLiteral("ActCraft")));
        QVERIFY(!QFileInfo::exists(folder.toLocalFile()));   // aucun accès disque sans demande
        QCOMPARE(controller.defaultFolderUrl(true), folder); // le dialogue le demande en s'ouvrant
        QVERIFY(QFileInfo(folder.toLocalFile()).isDir());
        QCOMPARE(QFileInfo::exists(real), realExisted);
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
        QSignalSpy documentSpy(&controller, &ui::DocumentController::documentChanged);
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        controller.saveToUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("banc.actcraft"))));
        QVERIFY(savedSpy.wait(60000));
        QCOMPARE(savedSpy.first().at(0).toBool(), true);
        QVERIFY(!controller.isModified());
        QCOMPARE(modifiedSpy.count(), 1);
        QCOMPARE(documentSpy.count(), 0); // le document n'a pas changé : rien à redessiner ni à relire
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
        PoolBlocker blocker; // détruit avant le contrôleur : porte relâchée et pool vidé même si un QVERIFY sort
        QVERIFY(blocker.allBlocked());
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        controller.saveToUrl(QUrl::fromLocalFile(path));
        controller.commitStroke(stroke(0xFF1E88E5u, 200)); // pendant l'enregistrement
        const QString after = controller.property("fingerprint").toString();
        QVERIFY(after != before);
        blocker.release();
        QVERIFY(savedSpy.wait(60000));
        QVERIFY(QThreadPool::globalInstance()->waitForDone(60000));
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
        QVERIFY(savedSpy.wait(60000));
        QCOMPARE(savedSpy.first().at(0).toBool(), true);
        QVERIFY(controller.fileUrl().isEmpty());
        QCOMPARE(controller.title(), QStringLiteral("Sans titre"));
        QVERIFY(!controller.property("busy").toBool());
    }

    // Revue, bloquant 1 : « modifié » faux ⇔ l'écran est le contenu du fichier lié. Trait A enregistré
    // (repère propre à l'indice 1), trait B, Enregistrer, Annuler pendant l'écriture : le fichier a B,
    // l'écran non.
    void undoDuringSaveLeavesTheDocumentModified()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        const QUrl url = QUrl::fromLocalFile(dir.filePath(QStringLiteral("a.actcraft")));
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        controller.commitStroke(redStroke());
        controller.saveToUrl(url);
        QVERIFY(savedSpy.wait(60000));
        QVERIFY(!controller.isModified());
        controller.commitStroke(stroke(0xFF1E88E5u, 200));
        controller.saveToUrl(url);
        controller.undo(); // pendant l'écriture : retour à l'indice qui portait l'ancien repère propre
        QVERIFY(savedSpy.wait(60000));
        QCOMPARE(savedSpy.last().at(0).toBool(), true);
        QCOMPARE(controller.document().page(0).objects.size(), std::size_t{1});
        QVERIFY(controller.isModified());
        QVERIFY(controller.property("modified").toBool());
    }

    // Même chose par « Enregistrer sous » : le nouveau fichier a B, l'écran non.
    void undoDuringSaveAsLeavesTheDocumentModified()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        controller.commitStroke(redStroke());
        controller.saveToUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("a.actcraft"))));
        QVERIFY(savedSpy.wait(60000));
        controller.commitStroke(stroke(0xFF1E88E5u, 200));
        const QUrl other = QUrl::fromLocalFile(dir.filePath(QStringLiteral("b.actcraft")));
        controller.saveToUrl(other);
        controller.undo();
        QVERIFY(savedSpy.wait(60000));
        QCOMPARE(controller.fileUrl(), other);
        QVERIFY(controller.isModified());
    }

    // Revue, important 2 : un trait tiré pendant une ouverture n'est jamais perdu en silence.
    void strokeDuringOpenRefusesTheOpening()
    {
        QTemporaryDir dir;
        const QUrl url = writeTwoPages(dir, QStringLiteral("deux.actcraft"));
        QVERIFY(!url.isEmpty());
        ui::DocumentController controller;
        QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
        controller.openUrl(url);
        controller.commitStroke(redStroke()); // pendant l'ouverture
        QVERIFY(loaded.wait(60000));
        QCOMPARE(loaded.first().at(0).toBool(), false);
        QVERIFY(!controller.property("lastError").toString().isEmpty());
        QCOMPARE(controller.pageCount(), 1);
        QCOMPARE(controller.document().page(0).objects.size(), std::size_t{1});
        QVERIFY(controller.isModified());
        QVERIFY(!controller.property("busy").toBool());
    }

    // « Nouveau » (et « Fermer », qui laisse un paperboard vierge) pendant une ouverture : pas écrasé, même
    // d'empreinte égale.
    void newDocumentDuringOpenRefusesTheOpening()
    {
        QTemporaryDir dir;
        const QUrl url = writeTwoPages(dir, QStringLiteral("deux.actcraft"));
        QVERIFY(!url.isEmpty());
        ui::DocumentController controller;
        QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
        controller.openUrl(url);
        controller.newDocument(1024, 768); // même empreinte que le paperboard vierge de départ
        QVERIFY(loaded.wait(60000));
        QCOMPARE(loaded.first().at(0).toBool(), false);
        QCOMPARE(controller.pageCount(), 1);
        QVERIFY(controller.fileUrl().isEmpty());
    }

    // Revue, important 3 (décision 18) : une cible .flipchart est refusée, quelle que soit la casse ; le
    // fichier reste intact octet pour octet.
    void flipchartIsNeverOverwritten()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("cours.flipchart"));
        const QByteArray original = fctest::sampleFlipchart();
        QVERIFY(writeFile(path, original));
        ui::DocumentController controller;
        controller.commitStroke(redStroke());
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        controller.saveToUrl(QUrl::fromLocalFile(path));
        QCOMPARE(savedSpy.count(), 0); // refus en file, jamais pendant l'appel
        QVERIFY(savedSpy.wait(60000));
        QCOMPARE(savedSpy.first().at(0).toBool(), false);
        QVERIFY(!controller.property("lastError").toString().isEmpty());
        QVERIFY(controller.fileUrl().isEmpty());
        QVERIFY(controller.isModified());
        const QString upper = dir.filePath(QStringLiteral("COURS.FLIPCHART"));
        QVERIFY(writeFile(upper, original));
        QVERIFY(!ui::saveDocument(controller.document(), QUrl::fromLocalFile(upper)).ok);
        QCOMPARE(readAll(path), original);
        QCOMPARE(readAll(upper), original);
    }

    // Une cible locale sans l'extension .actcraft la reçoit : jamais un fichier d'un autre format écrasé.
    void localTargetWithoutExtensionGetsActcraft()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        controller.saveToUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("cours"))));
        QVERIFY(savedSpy.wait(60000));
        QCOMPARE(savedSpy.first().at(0).toBool(), true);
        QCOMPARE(controller.fileUrl(), QUrl::fromLocalFile(dir.filePath(QStringLiteral("cours.actcraft"))));
        QVERIFY(QFile::exists(dir.filePath(QStringLiteral("cours.actcraft"))));
        QVERIFY(!QFile::exists(dir.filePath(QStringLiteral("cours"))));
        QCOMPARE(controller.title(), QStringLiteral("cours"));
    }

    // Revue, important 4 : un enregistrement qui échoue (dossier absent) laisse le document modifié et sans
    // fichier ; le suivant, réussi, efface l'erreur.
    void failedSaveKeepsTheDocumentModified()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.commitStroke(redStroke());
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        controller.saveToUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("absent/x.actcraft"))));
        QVERIFY(savedSpy.wait(60000));
        QCOMPARE(savedSpy.first().at(0).toBool(), false);
        QVERIFY(!controller.property("lastError").toString().isEmpty());
        QVERIFY(controller.isModified());
        QVERIFY(controller.fileUrl().isEmpty());
        QCOMPARE(controller.title(), QStringLiteral("Sans titre"));
        controller.saveToUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("x.actcraft"))));
        QVERIFY(savedSpy.wait(60000));
        QCOMPARE(savedSpy.last().at(0).toBool(), true);
        QVERIFY(controller.property("lastError").toString().isEmpty());
    }

    void openIsRefusedWhileBusy()
    {
        QTemporaryDir dir;
        const QUrl url = writeTwoPages(dir, QStringLiteral("deux.actcraft"));
        QVERIFY(!url.isEmpty());
        ui::DocumentController controller;
        controller.commitStroke(redStroke());
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        QSignalSpy loaded(&controller, &ui::DocumentController::loadFinished);
        controller.saveToUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("a.actcraft"))));
        controller.openUrl(url);
        QCOMPARE(loaded.count(), 0);
        QVERIFY(savedSpy.wait(60000));
        QCOMPARE(loaded.count(), 1);
        QCOMPARE(loaded.first().at(0).toBool(), false);
        QCOMPARE(savedSpy.first().at(0).toBool(), true);
        QCOMPARE(controller.pageCount(), 1);
        QCOMPARE(controller.document().page(0).objects.size(), std::size_t{1});
    }

    // Contrôleur détruit pendant une écriture puis pendant une lecture : le fil finit seul, rien n'est rendu
    // à un objet détruit ; le fichier écrit est complet (relu).
    void controllerDestroyedDuringIoReceivesNothing()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("orphelin.actcraft"));
        {
            auto* controller = new ui::DocumentController;
            controller->commitStroke(redStroke());
            PoolBlocker blocker;
            QVERIFY(blocker.allBlocked());
            controller->saveToUrl(QUrl::fromLocalFile(path));
            delete controller; // l'écriture n'a pas commencé
            blocker.release();
            QVERIFY(QThreadPool::globalInstance()->waitForDone(60000));
            QCoreApplication::sendPostedEvents();
        }
        QVERIFY(!savedFingerprint(path).isEmpty());
        {
            auto* controller = new ui::DocumentController;
            PoolBlocker blocker;
            QVERIFY(blocker.allBlocked());
            controller->openUrl(QUrl::fromLocalFile(path));
            delete controller;
            blocker.release();
            QVERIFY(QThreadPool::globalInstance()->waitForDone(60000));
            QCoreApplication::sendPostedEvents();
        }
    }

    // Revue, mineur 5 : le chemin d'une URL content:// n'est décodé qu'une fois.
    void contentTitlesAreDecodedOnce()
    {
        QCOMPARE(ui::titleFromUrl(QUrl(QStringLiteral(
                     "content://fournisseur/document/primary%3A100%25%20r%C3%A9ussite.actcraft"))),
                 QString::fromUtf8("100% r\xC3\xA9ussite"));
        QCOMPARE(ui::titleFromUrl(QUrl(QStringLiteral("content://fournisseur/document/a%2541b.actcraft"))),
                 QStringLiteral("a%41b"));
    }

    // Revue, mineur 7 : coût, sur le fil principal, de la copie prise à l'Enregistrer (5 000 traits de 50
    // points). Garde large ; le chiffre est écrit dans le journal du test.
    void snapshotCopyCostIsMeasured()
    {
        core::Document document;
        for (int i = 0; i < 5000; ++i) {
            core::Stroke s;
            for (int k = 0; k < 50; ++k) {
                s.points.push_back(core::StrokePoint{10.0 + k * 4, 10.0 + (i % 700), 0.5f});
            }
            core::PageObject object = std::move(s);
            object.common.id = document.allocateId();
            document.page(0).objects.push_back(std::move(object));
        }
        ui::DocumentController controller;
        controller.replaceDocument(std::move(document), QUrl(), QStringLiteral("mesure"));
        QTemporaryDir dir;
        QSignalSpy savedSpy(&controller, &ui::DocumentController::saveFinished);
        QElapsedTimer timer;
        timer.start();
        controller.saveToUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("mesure.actcraft"))));
        const qint64 micros = timer.nsecsElapsed() / 1000;
        qInfo("COPIE A L'ENREGISTRER : %lld us (5000 traits de 50 points)", static_cast<long long>(micros));
        QVERIFY2(micros < 1000000, qPrintable(QStringLiteral("%1 us").arg(micros)));
        QVERIFY(savedSpy.wait(60000));
        QCOMPARE(savedSpy.first().at(0).toBool(), true);
    }
};

QTEST_MAIN(TstUiFiles)
#include "tst_ui_files.moc"
