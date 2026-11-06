#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/mediainsert.h>

#include <QByteArray>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

#include <string>
#include <variant>

using namespace actcraft;

namespace {

void putLe(QByteArray& out, quint32 value, int bytes)
{
    for (int i = 0; i < bytes; ++i)
        out.append(static_cast<char>((value >> (8 * i)) & 0xFF));
}

// WAV de 0,5 s (8 kHz, mono, 16 bits) : en-tête RIFF écrit à la main.
QUrl writeWav(const QTemporaryDir& dir, const QString& name)
{
    const quint32 samples = 4000;
    QByteArray wav("RIFF");
    putLe(wav, 36 + samples * 2, 4);
    wav += "WAVEfmt ";
    putLe(wav, 16, 4);
    putLe(wav, 1, 2);
    putLe(wav, 1, 2);
    putLe(wav, 8000, 4);
    putLe(wav, 16000, 4);
    putLe(wav, 2, 2);
    putLe(wav, 16, 2);
    wav += "data";
    putLe(wav, samples * 2, 4);
    wav += QByteArray(static_cast<int>(samples * 2), '\0');
    const QString path = dir.filePath(name);
    QFile file(path);
    (void)file.open(QIODevice::WriteOnly);
    (void)file.write(wav);
    return QUrl::fromLocalFile(path);
}

QUrl writeBytes(const QTemporaryDir& dir, const QString& name)
{
    const QString path = dir.filePath(name);
    QFile file(path);
    (void)file.open(QIODevice::WriteOnly);
    (void)file.write("not really a movie");
    return QUrl::fromLocalFile(path);
}

const core::Page& firstPage(const ui::DocumentController& controller)
{
    return controller.document().page(0);
}

const core::Media* mediaAt(const ui::DocumentController& controller, std::size_t index)
{
    const auto& objects = firstPage(controller).objects;
    if (index >= objects.size())
        return nullptr;
    return std::get_if<core::Media>(&core::body(objects[index]));
}

} // namespace

class TstUiMediainsert : public QObject
{
    Q_OBJECT
private slots:
    void soundBecomesAPlaceholderWithAPlayAction()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::MediaInsert insert;
        insert.setDocument(&controller);
        QSignalSpy inserted(&insert, &ui::MediaInsert::inserted);
        insert.insertFile(writeWav(dir, "cloche.wav"));
        QTRY_COMPARE(inserted.count(), 1);
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{1});
        const core::Media* media = mediaAt(controller, 0);
        QVERIFY(media != nullptr);
        QVERIFY(controller.document().resource(media->media) != nullptr);
        QVERIFY(controller.document().resource(media->placeholder) != nullptr);
        QCOMPARE(media->fileName, std::string("cloche.wav"));
        const auto& actions = firstPage(controller).objects[0].common.actions;
        QCOMPARE(actions.size(), std::size_t{1});
        QCOMPARE(actions[0].catalogId, std::string("document.play-sound"));
        QCOMPARE(actions[0].resource, media->media);
        const QJsonObject parameters =
            QJsonDocument::fromJson(QByteArray::fromStdString(actions[0].parameters)).object();
        QCOMPARE(parameters.value("file").toString(), QString("cloche.wav"));
        QVERIFY(controller.canUndo());
        controller.undo();
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
        QVERIFY(!controller.canUndo()); // une seule étape
    }

    void videoKeepsItsAspect()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::MediaInsert insert;
        insert.setDocument(&controller);
        // Le décodage réel n'a pas lieu hors écran sous macOS : la première image est fournie par la sonde.
        QThread* mainThread = QThread::currentThread();
        std::atomic<bool> offMain{false};
        insert.setFrameProbe([&](const QString&) {
            offMain = QThread::currentThread() != mainThread;
            QImage frame(1920, 1080, QImage::Format_RGB32);
            frame.fill(Qt::darkGreen);
            return frame;
        });
        QSignalSpy inserted(&insert, &ui::MediaInsert::inserted);
        insert.insertFile(writeBytes(dir, "film.mp4"));
        QTRY_COMPARE(inserted.count(), 1);
        QVERIFY(offMain);
        const core::Media* media = mediaAt(controller, 0);
        QVERIFY(media != nullptr);
        QVERIFY(media->bounds.width > 0 && media->bounds.height > 0);
        QVERIFY(qAbs(media->bounds.width / media->bounds.height - 16.0 / 9.0) < 0.01);
        QVERIFY(media->aspectLocked);
        QVERIFY(media->bounds.width <= 0.8 * 1024 && media->bounds.height <= 0.8 * 768);
        QCOMPARE(firstPage(controller).objects[0].common.actions.size(), std::size_t{1});
    }

    void secondMediaGoesBesideTheFirst()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::MediaInsert insert;
        insert.setDocument(&controller);
        QSignalSpy inserted(&insert, &ui::MediaInsert::inserted);
        insert.insertFile(writeWav(dir, "un.wav"));
        QTRY_COMPARE(inserted.count(), 1);
        insert.insertFile(writeWav(dir, "deux.wav"));
        QTRY_COMPARE(inserted.count(), 2);
        const core::Media* first = mediaAt(controller, 0);
        const core::Media* second = mediaAt(controller, 1);
        QVERIFY(first != nullptr && second != nullptr);
        QVERIFY(second->bounds.x >= first->bounds.x + first->bounds.width);
        QCOMPARE(second->bounds.y, first->bounds.y);
        // Le premier est au centre de la page.
        QVERIFY(qAbs((first->bounds.x + first->bounds.width / 2) - 512) < 1);
        QVERIFY(second->bounds.x + second->bounds.width <= 1024);
    }

    void resourceDropInsertsInOneStep()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::MediaInsert insert;
        insert.setDocument(&controller);
        QSignalSpy inserted(&insert, &ui::MediaInsert::inserted);
        insert.insertFile(writeWav(dir, "un.wav"));
        QTRY_COMPARE(inserted.count(), 1);
        const std::string key = mediaAt(controller, 0)->media;
        insert.insertResource(QString::fromStdString(key));
        QTRY_COMPARE(inserted.count(), 2);
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{2});
        const core::Media* dropped = mediaAt(controller, 1);
        QVERIFY(dropped != nullptr);
        QCOMPARE(dropped->media, key);
        const auto& actions = firstPage(controller).objects[1].common.actions;
        QCOMPARE(actions.size(), std::size_t{1});
        QCOMPARE(actions[0].resource, key);
        controller.undo(); // une seule étape pour la ressource glissée
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{1});
        // Clé inconnue : refus, rien posé.
        QSignalSpy failed(&insert, &ui::MediaInsert::failed);
        insert.insertResource(QStringLiteral("absente"));
        QCOMPARE(failed.count(), 1);
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{1});
    }

    void flashShowsOnlyItsPlaceholder()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::MediaInsert insert;
        insert.setDocument(&controller);
        QSignalSpy inserted(&insert, &ui::MediaInsert::inserted);
        insert.insertFile(writeBytes(dir, "animation.swf"));
        QTRY_COMPARE(inserted.count(), 1);
        const core::Media* media = mediaAt(controller, 0);
        QVERIFY(media != nullptr);
        QVERIFY(controller.document().resource(media->placeholder) != nullptr);
        QVERIFY(firstPage(controller).objects[0].common.actions.empty());
        QVERIFY(!media->playOnOpen);
    }

    void refusedWhileLocked()
    {
        QTemporaryDir dir;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::Commands commands;
        ui::MediaInsert insert;
        insert.setDocument(&controller);
        insert.setCommands(&commands);
        QSignalSpy failed(&insert, &ui::MediaInsert::failed);
        commands.setLocked(true);
        insert.insertFile(writeWav(dir, "verrou.wav"));
        QCOMPARE(failed.count(), 1);
        QVERIFY(!insert.isBusy());
        QVERIFY(!controller.canUndo());
        insert.insertResource(QStringLiteral("x"));
        QCOMPARE(failed.count(), 2);
        QTest::qWait(100);
        QCOMPARE(firstPage(controller).objects.size(), std::size_t{0});
        commands.setLocked(false);
        QSignalSpy inserted(&insert, &ui::MediaInsert::inserted);
        insert.insertFile(writeWav(dir, "libre.wav"));
        QTRY_COMPARE(inserted.count(), 1);
    }
};

QTEST_MAIN(TstUiMediainsert)
#include "tst_ui_mediainsert.moc"
