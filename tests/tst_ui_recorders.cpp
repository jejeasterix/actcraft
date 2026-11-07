#include <actcraft/media/displayrecorder.h>
#include <actcraft/media/soundrecorder.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/recorders.h>
#include <actcraft/ui/settingsschema.h>

#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include <memory>

using namespace actcraft;

namespace {

// Moteur de son simulé : écrit un fichier non vide à la fermeture.
class FakeSound final : public media::RecordingEngine
{
public:
    void start(const QString& path) override { m_path = path; }
    void pause() override {}
    void resume() override {}
    void stop() override
    {
        QFile file(m_path);
        (void)file.open(QIODevice::WriteOnly);
        (void)file.write("RIFF");
        file.close();
        const QString path = m_path;
        QMetaObject::invokeMethod(this, [this, path] { emit stopped(path); }, Qt::QueuedConnection);
    }
    qint64 duration() const override { return 1000; }

private:
    QString m_path;
};

std::unique_ptr<media::RecordingEngine> makeSound()
{
    return std::make_unique<FakeSound>();
}

struct Rig
{
    QTemporaryDir dir;
    ui::Commands commands;
    ui::Preferences preferences{dir.filePath("preferences.ini"), nullptr};
    media::SoundRecorder sound{&makeSound, [](std::function<void(bool)> answer) { answer(true); }};
    media::DisplayRecorder display; // sans moteur d'écran hors Windows, avec son motif
    ui::Recorders recorders;
    Rig()
    {
        sound.setFolder(dir.filePath("son"));
        recorders.useRecorders(&sound, &display);
        recorders.setCommands(&commands);
        recorders.setPreferences(&preferences);
    }
};

} // namespace

class TstUiRecorders : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }

    void indicatorVisibleWhileRecording()
    {
        Rig rig;
        QVERIFY(!rig.recorders.indicatorVisible());
        QVERIFY(rig.commands.trigger(QStringLiteral("record.sound")));
        QVERIFY(rig.recorders.indicatorVisible());
        QCOMPARE(rig.recorders.kind(), QStringLiteral("sound"));
        QVERIFY(rig.commands.trigger(QStringLiteral("record.pause")));
        QVERIFY(rig.recorders.indicatorVisible()); // la pause reste un enregistrement
        QVERIFY(rig.recorders.isPaused());
        QVERIFY(rig.commands.trigger(QStringLiteral("record.pause")));
        QVERIFY(!rig.recorders.isPaused());
        QVERIFY(rig.commands.trigger(QStringLiteral("record.stop")));
        QVERIFY(rig.recorders.indicatorVisible()); // jusqu'à la fermeture du fichier
        QTRY_VERIFY(!rig.recorders.indicatorVisible());
        QCOMPARE(rig.recorders.kind(), QString());
    }

    void firstUseShowsTheNotice()
    {
        Rig rig;
        QSignalSpy shown(&rig.recorders, &ui::Recorders::noticeChanged);
        QVERIFY(!rig.recorders.noticeVisible());
        rig.recorders.toggleSound(); // une action de l'utilisateur
        QVERIFY(rig.recorders.noticeVisible());
        QVERIFY(rig.recorders.notice().contains(QStringLiteral("Rien n'est envoyé")));
        rig.recorders.dismissNotice();
        rig.recorders.cancel();
        QTRY_VERIFY(!rig.recorders.isRecording());
        shown.clear();
        rig.recorders.toggleSound(); // deuxième usage : plus de mention
        QVERIFY(!rig.recorders.noticeVisible());
        QCOMPARE(shown.count(), 0);
        QVERIFY(rig.preferences.value(QString::fromLatin1(ui::Recorders::kNoticeShownKey)).toBool());
        rig.recorders.cancel();
        QTRY_VERIFY(!rig.recorders.isRecording());
    }

    void noRecordingWithoutAUserAction()
    {
        Rig rig;
        QVERIFY(!rig.recorders.isRecording());
        QVERIFY(!rig.sound.isRecording());
    }

    void hiddenPaletteComesBackOnRestart()
    {
        Rig rig;
        rig.recorders.toggleSound();
        rig.recorders.hidePalette();
        QVERIFY(!rig.recorders.paletteVisible());
        QVERIFY(rig.recorders.indicatorVisible()); // l'indicateur ne se masque pas
        rig.recorders.cancel();
        QTRY_VERIFY(!rig.recorders.isRecording());
        QVERIFY(!rig.recorders.paletteVisible());
        rig.recorders.toggleSound();
        QVERIFY(rig.recorders.paletteVisible());
        rig.recorders.cancel();
        QTRY_VERIFY(!rig.recorders.isRecording());
    }

    void settingsSectionRead()
    {
        Rig rig;
        ui::SettingsSchema schema;
        schema.setPreferences(&rig.preferences);
        const QString folder = rig.dir.filePath("mes-enregistrements");
        schema.setValue(QStringLiteral("recordings"), QStringLiteral("recordings/folder"), folder);
        QCOMPARE(rig.recorders.folder(), folder);
        QCOMPARE(rig.recorders.soundFormat(), QStringLiteral("wav"));
        QCOMPARE(rig.recorders.screenFormat(), QStringLiteral("mp4"));
        rig.recorders.toggleSound();
        QCOMPARE(rig.sound.folder(), folder);
        rig.recorders.cancel();
        QTRY_VERIFY(!rig.recorders.isRecording());
    }

    void stopAlwaysAvailableUnderTheLock()
    {
        Rig rig;
        rig.commands.setLocked(true);
        QVERIFY(!rig.commands.trigger(QStringLiteral("record.sound"))); // démarrer reste verrouillé
        QVERIFY(!rig.recorders.isRecording());
        rig.commands.setLocked(false);
        QVERIFY(rig.commands.trigger(QStringLiteral("record.sound")));
        rig.commands.setLocked(true);
        QVERIFY(!rig.commands.trigger(QStringLiteral("record.pause")));
        QVERIFY(rig.commands.trigger(QStringLiteral("record.stop")));
        QTRY_VERIFY(!rig.recorders.isRecording());
        rig.commands.setLocked(false);
        QVERIFY(rig.commands.trigger(QStringLiteral("record.sound")));
        rig.commands.setLocked(true);
        QVERIFY(rig.commands.trigger(QStringLiteral("record.cancel")));
        QTRY_VERIFY(!rig.recorders.isRecording());
    }

    void screenAreaIsConvertedToPixels()
    {
        // Rapport de pixels 2 : 100,5 px logiques deviennent 201 px, arrondis vers l'extérieur.
        const QRect area = ui::Recorders::pixelArea(QRectF(10.25, 20, 100.5, 50), 2.0, QPointF(0, 0));
        QCOMPARE(area, QRect(QPoint(20, 40), QPoint(221, 139)));
        // Origine de l'écran soustraite (écran principal décalé).
        QCOMPARE(ui::Recorders::pixelArea(QRectF(-1900, 10, 100, 100), 1.0, QPointF(-1920, 0)),
                 QRect(QPoint(20, 10), QSize(100, 100)));
        QVERIFY(ui::Recorders::pixelArea(QRectF(0, 0, 10, 10), 0.0).isEmpty());
        // Au-delà de l'image, la zone est coupée par l'enregistreur d'écran lui-même.
        QCOMPARE(media::recordedArea(area, QSize(100, 100)), QRect(QPoint(20, 40), QSize(80, 60)));
    }

    void screenRecorderAbsentOnThisPlatform()
    {
#ifdef Q_OS_WIN
        QSKIP("Windows : l'enregistreur d'écran est présent, sa preuve réelle est opt-in (PC).");
#else
        Rig rig;
        QVERIFY(!rig.recorders.screenAvailable());
        QVERIFY(!rig.commands.registry().hasHandler(QStringLiteral("record.screen-full")) ||
                !rig.commands.isAvailable(QStringLiteral("record.screen-full")));
        rig.recorders.toggleScreen(false);
        QVERIFY(!rig.recorders.isRecording());
        QVERIFY(!rig.recorders.message().isEmpty());
#endif
    }

    void finishedSoundIsHandedToTheInsertion()
    {
        Rig rig;
        QSignalSpy duration(&rig.recorders, &ui::Recorders::lastDurationChanged);
        rig.recorders.toggleSound();
        rig.recorders.stop();
        QTRY_COMPARE(duration.count(), 1);
        QVERIFY(!rig.recorders.isRecording());
    }
};

QTEST_MAIN(TstUiRecorders)
#include "tst_ui_recorders.moc"
