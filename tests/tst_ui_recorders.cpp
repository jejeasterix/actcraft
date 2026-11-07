#include <actcraft/media/displayrecorder.h>
#include <actcraft/media/soundrecorder.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/mediainsert.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/recorders.h>
#include <actcraft/ui/settingsschema.h>

#include <QDir>
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

// Écran simulé : écrit un fichier non vide à la fermeture.
class FakeScreen final : public media::DisplayEngine
{
public:
    void start(const QString& path) override { m_path = path; }
    void encode(const QVideoFrame&) override {}
    void stop() override
    {
        QFile file(m_path);
        (void)file.open(QIODevice::WriteOnly);
        (void)file.write("mp4");
        file.close();
        const QString path = m_path;
        QMetaObject::invokeMethod(this, [this, path] { emit stopped(path); }, Qt::QueuedConnection);
    }

private:
    QString m_path;
};

media::DisplayRecorder makeDisplay(bool withScreen)
{
    if (!withScreen)
        return media::DisplayRecorder(); // sans moteur d'écran hors Windows, avec son motif
    return media::DisplayRecorder(media::DisplayRecorder::EngineFactory(
        [] { return std::unique_ptr<media::DisplayEngine>(new FakeScreen); }));
}

struct Rig
{
    QTemporaryDir dir;
    ui::Commands commands;
    ui::Preferences preferences{dir.filePath("preferences.ini"), nullptr};
    media::SoundRecorder sound;
    media::DisplayRecorder display;
    ui::Recorders recorders;
    // microphoneGranted : réponse de la permission simulée ; withScreen : un écran simulé (sinon
    // l'enregistreur d'écran de la plateforme, absent hors Windows).
    explicit Rig(bool microphoneGranted = true, bool withScreen = false)
        : sound(&makeSound,
                [microphoneGranted](std::function<void(bool)> answer) { answer(microphoneGranted); }),
          display(makeDisplay(withScreen))
    {
        sound.setFolder(dir.filePath("son"));
        display.setFolder(dir.filePath("ecran"));
        recorders.useRecorders(&sound, &display);
        recorders.setCommands(&commands);
        recorders.setPreferences(&preferences);
        if (withScreen)
            commands.setPlatform(
                actions::Platform::Windows); // les commandes d'écran sont déclarées pour Windows
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

    // L8-C10, constat 1 (F-292 « plein écran ou zone »). Distingue le bon chemin (la commande demande un
    // tracé, la zone de l'enregistrement est celle que l'utilisateur a tracée, convertie par pixelArea) de
    // l'ancien (la commande démarre sur-le-champ sur la moitié centrale de l'écran principal, sans
    // sélecteur).
    void screenAreaCommandRecordsTheDrawnArea()
    {
        Rig rig(true, true);
        QSignalSpy asked(&rig.recorders, &ui::Recorders::areaSelectionRequested);
        QVERIFY(rig.commands.trigger(QStringLiteral("record.screen-area")));
        QCOMPARE(asked.count(), 1);
        QVERIFY(rig.recorders.isSelectingArea());
        QVERIFY(!rig.recorders.isRecording()); // rien ne part avant que l'utilisateur ait tracé la zone
        QVERIFY(!rig.recorders.separateIndicator());

        rig.recorders.finishAreaSelection(100, 50, 300, 200, 2.0);
        QVERIFY(!rig.recorders.isSelectingArea());
        QVERIFY(rig.recorders.isRecording());
        // Rapport de pixels 2 : la zone tracée en pixels logiques, pas la moitié centrale de l'écran.
        QCOMPARE(rig.display.area(), ui::Recorders::pixelArea(QRectF(100, 50, 300, 200), 2.0));
        QCOMPARE(rig.display.area(), QRect(QPoint(200, 100), QSize(600, 400)));
        // L'indicateur d'un enregistrement d'écran vit dans une fenêtre à part (L8-T28), pas celui du son.
        QVERIFY(rig.recorders.separateIndicator());
        rig.recorders.cancel();
        QTRY_VERIFY(!rig.recorders.isRecording());
        QVERIFY(!rig.recorders.separateIndicator());
    }

    void escapingTheAreaSelectionStartsNothing()
    {
        Rig rig(true, true);
        QVERIFY(rig.commands.trigger(QStringLiteral("record.screen-area")));
        QVERIFY(rig.recorders.isSelectingArea());
        rig.recorders.cancelAreaSelection(); // Échap
        QVERIFY(!rig.recorders.isSelectingArea());
        QVERIFY(!rig.recorders.isRecording());
        QVERIFY(rig.display.area().isEmpty());
    }

    void soundIndicatorStaysInTheMainWindow()
    {
        Rig rig;
        rig.recorders.toggleSound();
        QVERIFY(rig.recorders.indicatorVisible());
        QVERIFY(
            !rig.recorders.separateIndicator()); // seul l'écran capture la fenêtre : le son n'en a pas besoin
        rig.recorders.cancel();
        QTRY_VERIFY(!rig.recorders.isRecording());
    }

    // L8-C10, constat 3 (S26 « la commande se désactive »). Distingue l'état propagé au registre de l'état
    // seulement porté par l'enregistreur : sous l'ancien patron, isAvailable() est faux mais la commande
    // reste active et déclenchable.
    void refusedMicrophoneDisablesTheCommand()
    {
        Rig rig(false);
        QVERIFY(rig.commands.registry().isEnabled(QStringLiteral("record.sound")));
        QVERIFY(
            rig.commands.trigger(QStringLiteral("record.sound"))); // l'utilisateur demande, le système refuse
        QTRY_VERIFY(!rig.sound.isAvailable());
        QVERIFY(!rig.recorders.isRecording());
        QVERIFY(!rig.recorders.message().isEmpty());
        QVERIFY(!rig.commands.registry().isEnabled(QStringLiteral("record.sound")));
        QVERIFY(!rig.commands.trigger(QStringLiteral("record.sound")));
    }

    // L8-C10, constat 2 : la fenêtre de l'indicateur d'un enregistrement d'écran se pose hors de la zone
    // enregistrée (Qt n'offre pas d'exclusion de la capture). Sous l'ancien patron (toujours en haut à
    // droite), la fenêtre recouvre ici la zone et entre dans l'image.
    void indicatorWindowAvoidsTheDrawnArea()
    {
        Rig rig(true, true);
        const QRectF screen(0, 0, 1920, 1080);
        const QSizeF size(240, 36);
        QVERIFY(rig.commands.trigger(QStringLiteral("record.screen-area")));
        rig.recorders.finishAreaSelection(1000, 0, 920, 400, 1.0); // zone en haut à droite
        const QPointF spot = rig.recorders.indicatorSpot(screen, size);
        QVERIFY(!QRectF(spot, size).intersects(QRectF(1000, 0, 920, 400)));
        QVERIFY(screen.contains(QRectF(spot, size)));
        rig.recorders.cancel();
        QTRY_VERIFY(!rig.recorders.isRecording());

        QVERIFY(rig.commands.trigger(QStringLiteral("record.screen-area")));
        rig.recorders.finishAreaSelection(0, 0, 1920, 540, 1.0); // moitié haute : la fenêtre passe en bas
        const QPointF below = rig.recorders.indicatorSpot(screen, size);
        QVERIFY(!QRectF(below, size).intersects(QRectF(0, 0, 1920, 540)));
        rig.recorders.cancel();
        QTRY_VERIFY(!rig.recorders.isRecording());
    }

    void tooSmallAreaStartsNothing()
    {
        Rig rig(true, true);
        QVERIFY(rig.commands.trigger(QStringLiteral("record.screen-area")));
        rig.recorders.finishAreaSelection(100, 100, 0, 0, 1.0); // un clic sans glisser
        QVERIFY(!rig.recorders.isSelectingArea());
        QVERIFY(!rig.recorders.isRecording()); // une zone vide voudrait dire « plein écran »
        QVERIFY(!rig.recorders.message().isEmpty());
    }

    // L8-C10, constat 5 : le code livré ne regarde pas la plateforme ; le banc et les tests injectent leurs
    // moteurs simulés par Recorders::setDefaultEngines.
    void injectedEnginesFeedANewRecorders()
    {
        struct Reset
        {
            ~Reset() { ui::Recorders::resetDefaultEngines(); }
        } reset;
        ui::Recorders::setDefaultEngines(
            {&makeSound, [](std::function<void(bool)> answer) { answer(true); },
             [] { return std::unique_ptr<media::DisplayEngine>(new FakeScreen); }});
        ui::Commands commands;
        ui::Recorders recorders;
        recorders.setCommands(&commands);
        QVERIFY(recorders.screenAvailable());
        QVERIFY(commands.trigger(QStringLiteral("record.sound")));
        QVERIFY(recorders.isRecording());
        recorders.cancel();
        QTRY_VERIFY(!recorders.isRecording());
    }

    // L8-C10, constat 4 : vider le réglage du dossier rend le dossier par défaut, pas l'ancien dossier
    // choisi.
    void clearingTheFolderSettingRestoresTheDefaultFolder()
    {
        Rig rig;
        const QString defaultFolder = rig.sound.folder();
        const QString chosen = rig.dir.filePath("choisi");
        rig.preferences.setValue(QString::fromLatin1(ui::Recorders::kFolderKey), chosen);
        rig.recorders.toggleSound();
        QCOMPARE(rig.sound.folder(), chosen);
        rig.recorders.cancel();
        QTRY_VERIFY(!rig.recorders.isRecording());
        rig.preferences.setValue(QString::fromLatin1(ui::Recorders::kFolderKey), QString());
        rig.recorders.toggleSound();
        QCOMPARE(rig.sound.folder(), defaultFolder);
        rig.recorders.cancel();
        QTRY_VERIFY(!rig.recorders.isRecording());
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
    // L8-C10, essai 2, constat 4 : le dossier par défaut est propre à la session ; le fichier, copié dans le
    // document par l'insertion, y est effacé une fois l'insertion réussie.
    void defaultFolderIsPerSessionAndCleanedAfterInsertion()
    {
        struct Reset
        {
            ~Reset() { ui::Recorders::resetDefaultEngines(); }
        } reset;
        ui::Recorders::setDefaultEngines(
            {&makeSound, [](std::function<void(bool)> answer) { answer(true); }, {}});
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::MediaInsert insert;
        insert.setDocument(&controller);
        ui::Recorders recorders;
        recorders.setMediaInsert(&insert);
        QSignalSpy inserted(&insert, &ui::MediaInsert::inserted);
        recorders.toggleSound();
        const QString session = recorders.sessionFolder();
        QVERIFY(!session.isEmpty());
        QVERIFY(session.contains(QStringLiteral("actcraft-recordings")));
        QVERIFY(session != QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                               QStringLiteral("/actcraft-recordings"));
        recorders.stop();
        QTRY_COMPARE(inserted.count(), 1);
        QTRY_VERIFY(QDir(session).isEmpty());
    }

    // L8-C10, essai 2, constat 6 : insertion refusée après l'arrêt (verrou) ; le message dit où est le
    // fichier.
    void refusedInsertionTellsWhereTheFileIs()
    {
        struct Reset
        {
            ~Reset() { ui::Recorders::resetDefaultEngines(); }
        } reset;
        ui::Recorders::setDefaultEngines(
            {&makeSound, [](std::function<void(bool)> answer) { answer(true); }, {}});
        ui::Commands commands;
        ui::DocumentController controller;
        controller.newDocument(1024, 768);
        ui::MediaInsert insert;
        insert.setDocument(&controller);
        insert.setCommands(&commands);
        ui::Recorders recorders;
        recorders.setCommands(&commands);
        recorders.setMediaInsert(&insert);
        QVERIFY(commands.trigger(QStringLiteral("record.sound")));
        commands.setLocked(true);
        QVERIFY(commands.trigger(QStringLiteral("record.stop")));
        QTRY_VERIFY(!recorders.isRecording());
        const QStringList files = QDir(recorders.sessionFolder()).entryList(QDir::Files);
        QCOMPARE(files.size(), 1); // le fichier reste
        QVERIFY2(recorders.message().contains(QDir(recorders.sessionFolder()).filePath(files.first())),
                 qPrintable(recorders.message()));
    }
};

QTEST_MAIN(TstUiRecorders)
#include "tst_ui_recorders.moc"
