#include "actcraft/media/soundrecorder.h"

#include <QAudioDevice>
#include <QDataStream>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QMediaDevices>
#include <QMediaFormat>
#include <QPointer>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QtEndian>
#include <QtTest>

#include <functional>
#include <memory>

using actcraft::media::RecordingEngine;
using actcraft::media::SoundRecorder;

namespace {

// WAV relu : en-tête RIFF/WAVE fermé (taille du fichier écrite à la fermeture), durée tirée de « data ».
struct WavInfo
{
    bool valid = false;
    qint64 milliseconds = 0;
};

WavInfo readWav(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QByteArray bytes = file.readAll();
    if (bytes.size() < 12 || !bytes.startsWith("RIFF") || bytes.mid(8, 4) != "WAVE")
        return {};
    const auto u32 = [&bytes](qsizetype at) { return qFromLittleEndian<quint32>(bytes.constData() + at); };
    if (u32(4) != quint32(bytes.size() - 8))
        return {};
    quint32 byteRate = 0;
    qint64 dataSize = -1;
    for (qsizetype at = 12; at + 8 <= bytes.size();) {
        const QByteArray id = bytes.mid(at, 4);
        const quint32 size = u32(at + 4);
        if (id == "fmt " && size >= 16 && at + 8 + 16 <= bytes.size())
            byteRate = u32(at + 8 + 8);
        else if (id == "data")
            dataSize = size;
        at += 8 + qsizetype(size) + qsizetype(size & 1);
    }
    if (byteRate == 0 || dataSize < 0)
        return {};
    return {true, dataSize * 1000 / byteRate};
}

bool writeSilentWav(const QString& path, qint64 milliseconds)
{
    constexpr quint32 rate = 8000;
    constexpr quint16 channels = 1;
    constexpr quint16 bits = 16;
    const quint32 dataSize = quint32(rate * milliseconds / 1000) * channels * (bits / 8);
    QByteArray bytes;
    {
        QDataStream out(&bytes, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);
        out.writeRawData("RIFF", 4);
        out << quint32(36 + dataSize);
        out.writeRawData("WAVEfmt ", 8);
        out << quint32(16) << quint16(1) << channels << rate << quint32(rate * channels * (bits / 8))
            << quint16(channels * (bits / 8)) << bits;
        out.writeRawData("data", 4);
        out << dataSize;
    }
    bytes.append(QByteArray(qsizetype(dataSize), '\0'));
    QFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size();
}

// Source simulée, au contrat de Qt Multimedia : le temps ne compte que hors pause ; le fichier n'existe
// qu'à la fermeture, 80 ms après stop() (un vrai moteur écrit et ferme son conteneur sur ses fils), ou à la
// destruction du moteur s'il n'est pas encore fermé ; sans chemin imposé, il écrit à son emplacement par
// défaut, comme QMediaRecorder dans le dossier Musique. failAfterMs > 0 : panne du micro en cours de route.
class SimulatedEngine : public RecordingEngine
{
public:
    SimulatedEngine(QString defaultFolder, int failAfterMs)
        : m_defaultFolder(std::move(defaultFolder)), m_failAfterMs(failAfterMs)
    {}
    ~SimulatedEngine() override
    {
        if (m_open)
            writeSilentWav(target(), duration());
    }

    void start(const QString& path) override
    {
        m_path = path;
        m_total = 0;
        m_running = true;
        m_open = true;
        m_clock.start();
        if (m_failAfterMs > 0) {
            QTimer::singleShot(m_failAfterMs, this, [this] {
                emit failed(QStringLiteral("panne simulée du micro"));
                stop(); // fermeture tardive : le fichier s'écrit encore après la panne
            });
        }
    }
    void pause() override
    {
        if (m_running)
            m_total += m_clock.elapsed();
        m_running = false;
    }
    void resume() override
    {
        if (!m_running)
            m_clock.start();
        m_running = true;
    }
    void stop() override
    {
        pause();
        QTimer::singleShot(80, this, [this] {
            if (!m_open)
                return;
            m_open = false;
            writeSilentWav(target(), m_total);
            emit stopped(target());
        });
    }
    qint64 duration() const override { return m_total + (m_running ? m_clock.elapsed() : 0); }

private:
    QString target() const
    {
        return m_path.isEmpty() ? m_defaultFolder + QStringLiteral("/enregistrement.wav") : m_path;
    }

    QString m_defaultFolder;
    int m_failAfterMs = 0;
    bool m_open = false;
    QString m_path;
    QElapsedTimer m_clock;
    qint64 m_total = 0;
    bool m_running = false;
};

// Micro réel : sous macOS, la première capture ouvrirait la demande d'autorisation du système. L'essai se
// fait au PC (ou sur le Mac avec ACTCRAFT_TEST_MICRO=1).
QString microphoneUnavailable()
{
#if defined(Q_OS_DARWIN)
    if (!qEnvironmentVariableIsSet("ACTCRAFT_TEST_MICRO"))
        return QStringLiteral("micro réel : essai au PC (aucune demande d'autorisation sur le Mac)");
#endif
    if (QMediaDevices::audioInputs().isEmpty())
        return QStringLiteral("aucune entrée audio");
    return {};
}

QString canonicalFolder(const QString& path)
{
    return QFileInfo(path).canonicalPath();
}

} // namespace

#define SKIP_WITHOUT_MICROPHONE(micro)                                                                       \
    do {                                                                                                     \
        if (micro) {                                                                                         \
            const QString why = microphoneUnavailable();                                                     \
            if (!why.isEmpty())                                                                              \
                QSKIP(qPrintable(why));                                                                      \
        }                                                                                                    \
    } while (false)

class TstMediaSoundrecorder : public QObject
{
    Q_OBJECT

    enum class Answer
    {
        Grant,
        Refuse
    };

    QTemporaryDir m_root;
    QString m_folder;    // dossier temporaire du document : seul endroit permis
    QString m_elsewhere; // emplacement par défaut du moteur : doit rester vide
    QPointer<RecordingEngine> m_lastEngine;
    int m_enginesMade = 0;
    int m_failAfterMs = 0;
    int m_asked = 0;
    int m_case = 0;

    std::unique_ptr<SoundRecorder> makeRecorder(bool micro, Answer answer = Answer::Grant)
    {
        SoundRecorder::EngineFactory engines;
        if (micro) {
            engines = [this]() -> std::unique_ptr<RecordingEngine> {
                ++m_enginesMade;
                auto engine = SoundRecorder::createCaptureEngine();
                m_lastEngine = engine.get();
                return engine;
            };
        } else {
            engines = [this]() -> std::unique_ptr<RecordingEngine> {
                ++m_enginesMade;
                auto engine = std::make_unique<SimulatedEngine>(m_elsewhere, m_failAfterMs);
                m_lastEngine = engine.get();
                return engine;
            };
        }
        // Permission simulée, répondue au tour suivant de la boucle comme une fenêtre du système.
        auto permission = [this, answer](std::function<void(bool)> reply) {
            ++m_asked;
            QTimer::singleShot(0, this, [reply, answer] { reply(answer == Answer::Grant); });
        };
        auto recorder = std::make_unique<SoundRecorder>(std::move(engines), std::move(permission));
        recorder->setFolder(m_folder);
        return recorder;
    }

    void addEngines()
    {
        QTest::addColumn<bool>("micro");
        QTest::newRow("simulee") << false;
        QTest::newRow("micro") << true;
    }

    QStringList filesIn(const QString& folder) const
    {
        return QDir(folder).entryList(QDir::Files | QDir::Hidden | QDir::System);
    }

private slots:
    void initTestCase() { QVERIFY(m_root.isValid()); }

    void init()
    {
        ++m_case;
        m_folder = m_root.filePath(QStringLiteral("document-%1").arg(m_case)); // créé par l'enregistreur
        m_elsewhere = m_root.filePath(QStringLiteral("ailleurs-%1").arg(m_case));
        QVERIFY(QDir().mkpath(m_elsewhere));
        m_lastEngine.clear();
        m_enginesMade = 0;
        m_failAfterMs = 0;
        m_asked = 0;
    }

    // Le vrai moteur, construit sans enregistrer (le micro n'est pas ouvert) : WAV, et encodable ici.
    void captureEngineWritesWav()
    {
        const std::unique_ptr<RecordingEngine> engine = SoundRecorder::createCaptureEngine();
        QVERIFY(engine);
        const QMediaFormat format = engine->mediaFormat();
        QCOMPARE(format.fileFormat(), QMediaFormat::Wave);
        QCOMPARE(format.audioCodec(), QMediaFormat::AudioCodec::Wave);
        QVERIFY2(format.isSupported(QMediaFormat::Encode),
                 "WAV non encodable par le moteur de Qt Multimedia");
    }

    void writesAWav_data() { addEngines(); }
    void writesAWav()
    {
        QFETCH(bool, micro);
        SKIP_WITHOUT_MICROPHONE(micro);
        const auto recorder = makeRecorder(micro);
        QStringList handed;
        recorder->setInsertHandler([&handed](const QString& path) { handed << path; });
        recorder->toggle();
        QTRY_VERIFY(recorder->isRecording());
        QTest::qWait(3000);
        recorder->stop();
        QTRY_COMPARE(handed.size(), 1);
        const WavInfo wav = readWav(handed.first());
        QVERIFY2(wav.valid, qPrintable(handed.first()));
        QVERIFY2(wav.milliseconds >= 2500 && wav.milliseconds <= 3600,
                 qPrintable(QStringLiteral("%1 ms enregistrées pour 3 s").arg(wav.milliseconds)));
        QCOMPARE(QFileInfo(handed.first()).suffix(), QStringLiteral("wav"));
    }

    void pauseAndResume_data() { addEngines(); }
    void pauseAndResume()
    {
        QFETCH(bool, micro);
        SKIP_WITHOUT_MICROPHONE(micro);
        const auto recorder = makeRecorder(micro);
        QStringList handed;
        recorder->setInsertHandler([&handed](const QString& path) { handed << path; });
        recorder->toggle();
        QTRY_VERIFY(recorder->isRecording());
        QTest::qWait(1000);
        recorder->toggle(); // second appui : pause
        QVERIFY(recorder->isPaused());
        QVERIFY(recorder->isRecording());
        QTest::qWait(150);
        const qint64 held = recorder->duration();
        QTest::qWait(1350);
        QVERIFY2(recorder->duration() - held < 100, "la durée avance pendant la pause");
        recorder->toggle(); // reprise
        QVERIFY(!recorder->isPaused());
        QTest::qWait(1000);
        recorder->stop();
        QTRY_COMPARE(handed.size(), 1);
        const WavInfo wav = readWav(handed.first());
        QVERIFY(wav.valid);
        // 1 s + 1 s enregistrées ; avec la pause de 1,5 s, ce serait 3,5 s.
        QVERIFY2(wav.milliseconds >= 1600 && wav.milliseconds <= 2700,
                 qPrintable(QStringLiteral("%1 ms enregistrées pour 2 s hors pause").arg(wav.milliseconds)));
    }

    // Discrimine le patron interdit : un rappel appelé dans stop() verserait un fichier que le moteur n'a pas
    // encore fermé ; une boucle locale qui attendrait la fermeture tiendrait le fil principal ; l'emplacement
    // par défaut du moteur écrirait hors du dossier du document.
    void stopPlacesAPlayIconOnThePage_data() { addEngines(); }
    void stopPlacesAPlayIconOnThePage()
    {
        QFETCH(bool, micro);
        SKIP_WITHOUT_MICROPHONE(micro);
        const auto recorder = makeRecorder(micro);
        QStringList handed;
        bool insideStop = false;
        bool handedInsideStop = false;
        bool completeWhenHanded = false;
        recorder->setInsertHandler([&](const QString& path) {
            handed << path;
            handedInsideStop = handedInsideStop || insideStop;
            completeWhenHanded = readWav(path).valid;
        });
        QSignalSpy finished(recorder.get(), &SoundRecorder::finished);
        recorder->toggle();
        QTRY_VERIFY(recorder->isRecording());
        QTest::qWait(600);

        bool loopRanInsideStop = false;
        QTimer::singleShot(0, this, [&] { loopRanInsideStop = insideStop; });
        QElapsedTimer clock;
        clock.start();
        insideStop = true;
        recorder->stop();
        insideStop = false;
        const qint64 spent = clock.elapsed();
        QVERIFY2(!handedInsideStop, "rappel appelé avant la fermeture du fichier par le moteur");
        QVERIFY2(!loopRanInsideStop, "stop() a fait tourner la boucle d'événements");
        QVERIFY2(spent < 50, qPrintable(QStringLiteral("stop() a tenu la main %1 ms").arg(spent)));

        QTRY_COMPARE(handed.size(), 1);
        QVERIFY2(completeWhenHanded, "fichier confié avant d'être complet");
        QCOMPARE(finished.count(), 1);
        QCOMPARE(finished.first().at(0).toString(), handed.first());
        QCOMPARE(canonicalFolder(handed.first()), QDir(m_folder).canonicalPath());
        QCOMPARE(filesIn(m_folder), QStringList{QFileInfo(handed.first()).fileName()});
        QVERIFY2(filesIn(m_elsewhere).isEmpty(), "écrit à l'emplacement par défaut du moteur");
        QTest::qWait(300);
        QCOMPARE(handed.size(), 1); // une seule fois
        QVERIFY(!recorder->isRecording());
    }

    // Discrimine le même patron côté abandon : effacer dans cancel() précède la fermeture par le moteur, qui
    // écrit le fichier après coup. Une permission répondue après l'abandon ne lance rien.
    void cancelLeavesNothing_data() { addEngines(); }
    void cancelLeavesNothing()
    {
        QFETCH(bool, micro);
        SKIP_WITHOUT_MICROPHONE(micro);
        const auto recorder = makeRecorder(micro);
        int handed = 0;
        recorder->setInsertHandler([&handed](const QString&) { ++handed; });
        QSignalSpy finished(recorder.get(), &SoundRecorder::finished);

        recorder->toggle(); // permission en attente
        recorder->cancel(); // abandon avant la réponse
        QTest::qWait(200);
        QVERIFY2(!recorder->isRecording(), "enregistrement lancé par une réponse arrivée après l'abandon");
        QCOMPARE(m_enginesMade, 0);

        recorder->toggle();
        QTRY_VERIFY(recorder->isRecording());
        QTest::qWait(500);
        recorder->cancel();
        QTRY_VERIFY(!recorder->isRecording());
        QTest::qWait(300); // la fermeture du moteur est passée
        QCOMPARE(handed, 0);
        QCOMPARE(finished.count(), 0);
        QVERIFY2(filesIn(m_folder).isEmpty(), qPrintable(filesIn(m_folder).join(u' ')));
        QVERIFY(filesIn(m_elsewhere).isEmpty());
    }

    void refusedPermissionDisablesWithAMessage()
    {
        const auto recorder = makeRecorder(false, Answer::Refuse);
        int handed = 0;
        recorder->setInsertHandler([&handed](const QString&) { ++handed; });
        QSignalSpy availability(recorder.get(), &SoundRecorder::availabilityChanged);
        QSignalSpy failed(recorder.get(), &SoundRecorder::failed);
        QVERIFY(recorder->isAvailable());
        QVERIFY(recorder->message().isEmpty());

        recorder->toggle();
        QTRY_VERIFY(!recorder->isAvailable());
        QVERIFY(!recorder->isRecording());
        QVERIFY(!recorder->message().isEmpty());
        QCOMPARE(availability.count(), 1);
        QCOMPARE(failed.count(), 1);
        QCOMPARE(failed.first().at(0).toString(), recorder->message());
        QCOMPARE(m_enginesMade, 0); // le micro n'est jamais ouvert

        recorder->toggle(); // commande désactivée : ni nouvelle demande, ni enregistrement
        QTest::qWait(200);
        QCOMPARE(m_asked, 1);
        QCOMPARE(m_enginesMade, 0);
        QVERIFY(!recorder->isRecording());
        QCOMPARE(handed, 0);
        QVERIFY(filesIn(m_folder).isEmpty());
    }

    // Panne en cours d'enregistrement : message, aucun rappel ; le fichier, réécrit par la fermeture tardive
    // ou par la destruction du moteur, est effacé après elles.
    void engineFailureLeavesNothing()
    {
        m_failAfterMs = 300;
        const auto recorder = makeRecorder(false);
        int handed = 0;
        recorder->setInsertHandler([&handed](const QString&) { ++handed; });
        QSignalSpy finished(recorder.get(), &SoundRecorder::finished);
        QSignalSpy failed(recorder.get(), &SoundRecorder::failed);
        recorder->toggle();
        QTRY_VERIFY(recorder->isRecording());
        QTRY_COMPARE(failed.count(), 1);
        QCOMPARE(failed.first().at(0).toString(), QStringLiteral("panne simulée du micro"));
        QVERIFY(!recorder->isRecording());
        QVERIFY(!recorder->isPaused());
        QTRY_VERIFY(m_lastEngine.isNull());
        QTest::qWait(300);
        QCOMPARE(handed, 0);
        QCOMPARE(finished.count(), 0);
        QVERIFY2(filesIn(m_folder).isEmpty(), qPrintable(filesIn(m_folder).join(u' ')));
        QVERIFY(recorder->isAvailable()); // une panne ne désactive pas la commande
    }

    void unreachableFolderFailsWithAMessage()
    {
        const QString blocker = m_root.filePath(QStringLiteral("fichier-%1").arg(m_case));
        {
            QFile file(blocker);
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        const auto recorder = makeRecorder(false);
        recorder->setFolder(blocker + QStringLiteral("/sous-dossier")); // sous un fichier : mkpath échoue
        int handed = 0;
        recorder->setInsertHandler([&handed](const QString&) { ++handed; });
        QSignalSpy failed(recorder.get(), &SoundRecorder::failed);
        recorder->toggle();
        QTRY_COMPARE(failed.count(), 1);
        QVERIFY(failed.first().at(0).toString().contains(QStringLiteral("sous-dossier")));
        QVERIFY(!recorder->isRecording());
        QCOMPARE(m_enginesMade, 0);
        QTest::qWait(200);
        QCOMPARE(handed, 0);
        QVERIFY(filesIn(m_elsewhere).isEmpty());
    }

    void engineReleasedBetweenRecordings()
    {
        const auto recorder = makeRecorder(false);
        QStringList handed;
        recorder->setInsertHandler([&handed](const QString& path) { handed << path; });
        for (int round = 1; round <= 2; ++round) {
            recorder->toggle();
            QTRY_VERIFY(recorder->isRecording());
            QCOMPARE(m_enginesMade, round);
            QVERIFY(!m_lastEngine.isNull());
            QTest::qWait(200);
            recorder->stop();
            QTRY_COMPARE(handed.size(), round);
            QTRY_VERIFY2(m_lastEngine.isNull(), "moteur gardé après l'arrêt : micro non relâché");
        }
        QCOMPARE(filesIn(m_folder).size(), 2);
        QVERIFY(handed.at(0) != handed.at(1));
    }

    void destroyedWhileRecordingLeavesNothing()
    {
        auto recorder = makeRecorder(false);
        int handed = 0;
        recorder->setInsertHandler([&handed](const QString&) { ++handed; });
        recorder->toggle();
        QTRY_VERIFY(recorder->isRecording());
        QTest::qWait(300);
        recorder.reset(); // document fermé : le moteur écrit son fichier partiel en se détruisant
        QVERIFY(m_lastEngine.isNull());
        QTest::qWait(200);
        QCOMPARE(handed, 0);
        QVERIFY2(filesIn(m_folder).isEmpty(), qPrintable(filesIn(m_folder).join(u' ')));
    }

    void indicatorStateFollows_data() { addEngines(); }
    void indicatorStateFollows()
    {
        QFETCH(bool, micro);
        SKIP_WITHOUT_MICROPHONE(micro);
        const auto recorder = makeRecorder(micro);
        QSignalSpy recording(recorder.get(), &SoundRecorder::recordingChanged);
        QSignalSpy paused(recorder.get(), &SoundRecorder::pausedChanged);
        QVERIFY(!recorder->isRecording());

        recorder->toggle();
        QTRY_VERIFY(recorder->isRecording());
        QCOMPARE(recording.count(), 1);
        recorder->toggle();
        QVERIFY(recorder->isPaused());
        QVERIFY(recorder->isRecording()); // l'indicateur reste visible pendant la pause
        recorder->toggle();
        QVERIFY(!recorder->isPaused());
        QCOMPARE(paused.count(), 2);
        QTest::qWait(300);
        recorder->stop();
        if (!micro)
            QVERIFY2(recorder->isRecording(), "indicateur éteint avant la fermeture du fichier");
        QTRY_VERIFY(!recorder->isRecording());
        QCOMPARE(recording.count(), 2);
        QVERIFY(!recorder->isPaused());

        recorder->toggle(); // permission déjà accordée : pas de nouvelle demande
        QVERIFY(recorder->isRecording());
        QCOMPARE(m_asked, 1);
        recorder->cancel();
        QTRY_VERIFY(!recorder->isRecording());
        QCOMPARE(recording.count(), 4);
    }
};

// Sans interface et, sous macOS, sur le répartiteur Core Foundation (voir tst_media_player.cpp) : le moteur
// darwin de Qt Multimedia ne livre rien sinon. Réglages d'essai (M7) : QStandardPaths en mode test.
int main(int argc, char* argv[])
{
#if defined(Q_OS_DARWIN)
    qputenv("QT_EVENT_DISPATCHER_CORE_FOUNDATION", "1");
#endif
    QCoreApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    TstMediaSoundrecorder test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}
#include "tst_media_soundrecorder.moc"
