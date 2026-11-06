#include "actcraft/media/displayrecorder.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QMediaFormat>
#include <QMediaPlayer>
#include <QPointer>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <QVideoFrame>
#include <QVideoSink>
#include <QtTest>

#include <functional>
#include <memory>

using actcraft::media::DisplayEngine;
using actcraft::media::DisplayRecorder;

namespace {

constexpr int screenWidth = 640;
constexpr int screenHeight = 480;

// Écran simulé où chaque pixel dit sa position (x et y sur 12 bits) : une zone recadrée se reconnaît d'une
// image entière mise à l'échelle, qui aurait la même taille mais d'autres coins.
QImage positionPattern()
{
    QImage image(screenWidth, screenHeight, QImage::Format_RGB32);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x)
            image.setPixel(x, y, qRgb(x & 0xff, y & 0xff, ((x >> 8) << 4) | (y >> 8)));
    }
    return image;
}

QPoint positionAt(const QImage& image, int x, int y)
{
    const QRgb pixel = image.pixel(x, y);
    return {qRed(pixel) | ((qBlue(pixel) >> 4) << 8), qGreen(pixel) | ((qBlue(pixel) & 0xf) << 8)};
}

// Ce que l'encodeur a reçu, relu dans le fichier : taille commune des images, coins (position sur l'écran,
// image simulée seulement), durée, plus grand écart entre deux images consécutives.
struct Recorded
{
    bool valid = false;
    QString why;
    qint64 milliseconds = 0;
    QSize size;
    int frames = 0;
    bool positions = false;
    QPoint topLeft;
    QPoint bottomRight;
    qint64 largestGapMs = 0;
};

// Fichier du moteur simulé : une ligne par image encodée, « largeur hauteur x0 y0 x1 y1 début fin » (µs).
Recorded readSimulated(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {false, QStringLiteral("fichier absent : %1").arg(path)};
    Recorded recorded;
    recorded.positions = true;
    qint64 previousStart = -1;
    const QStringList lines = QString::fromUtf8(file.readAll()).split(u'\n', Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const QStringList fields = line.split(u' ');
        if (fields.size() != 8)
            return {false, QStringLiteral("ligne illisible : %1").arg(line)};
        const QSize size(fields.at(0).toInt(), fields.at(1).toInt());
        const QPoint topLeft(fields.at(2).toInt(), fields.at(3).toInt());
        const QPoint bottomRight(fields.at(4).toInt(), fields.at(5).toInt());
        const qint64 start = fields.at(6).toLongLong();
        const qint64 end = fields.at(7).toLongLong();
        if (recorded.frames == 0) {
            recorded.size = size;
            recorded.topLeft = topLeft;
            recorded.bottomRight = bottomRight;
        } else if (size != recorded.size || topLeft != recorded.topLeft ||
                   bottomRight != recorded.bottomRight) {
            return {false, QStringLiteral("images différentes dans le même fichier : %1").arg(line)};
        }
        if (start < 0 || end <= start || start < previousStart)
            return {false, QStringLiteral("horodatage incohérent : %1").arg(line)};
        if (previousStart >= 0)
            recorded.largestGapMs = qMax(recorded.largestGapMs, (start - previousStart) / 1000);
        previousStart = start;
        recorded.milliseconds = end / 1000;
        ++recorded.frames;
    }
    if (recorded.frames == 0)
        return {false, QStringLiteral("aucune image encodée")};
    recorded.valid = true;
    return recorded;
}

// MP4 réel : durée lue par QMediaPlayer::duration, taille de la première image décodée.
Recorded readVideo(const QString& path)
{
    QMediaPlayer player;
    QVideoSink sink;
    player.setVideoSink(&sink);
    QSize size;
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, &sink, [&size](const QVideoFrame& frame) {
        if (frame.isValid() && size.isEmpty())
            size = frame.size();
    });
    player.setSource(QUrl::fromLocalFile(path));
    const auto loaded = [&player] {
        return player.mediaStatus() == QMediaPlayer::LoadedMedia ||
               player.mediaStatus() == QMediaPlayer::InvalidMedia;
    };
    if (!QTest::qWaitFor(loaded, 10000) || player.mediaStatus() == QMediaPlayer::InvalidMedia)
        return {false, QStringLiteral("MP4 illisible : %1").arg(path)};
    Recorded recorded;
    recorded.milliseconds = player.duration();
    player.play(); // aucune sortie audio, aucune fenêtre : les images vont au puits
    QTest::qWaitFor([&size] { return !size.isEmpty(); }, 5000);
    player.stop();
    if (size.isEmpty())
        return {false, QStringLiteral("aucune image décodée : %1").arg(path)};
    recorded.size = size;
    recorded.valid = true;
    return recorded;
}

Recorded readRecorded(const QString& path, bool screen)
{
    return screen ? readVideo(path) : readSimulated(path);
}

// Moteur simulé, au contrat de Qt Multimedia : start() rend la main, puis l'écran entier arrive toutes les
// 20 ms par frameCaptured() ; encode() note ce que l'encodeur reçoit ; le fichier n'existe qu'à la fermeture,
// 80 ms après stop(), ou à la destruction du moteur s'il n'est pas encore fermé ; sans chemin imposé, il
// écrit à son emplacement par défaut, comme QMediaRecorder dans le dossier Vidéos. failAfterMs > 0 : panne.
class SimulatedScreen : public DisplayEngine
{
public:
    SimulatedScreen(QString defaultFolder, int failAfterMs)
        : m_defaultFolder(std::move(defaultFolder)), m_failAfterMs(failAfterMs), m_pattern(positionPattern())
    {
        m_timer.setInterval(20);
        connect(&m_timer, &QTimer::timeout, this, [this] { emit frameCaptured(QVideoFrame(m_pattern)); });
    }
    ~SimulatedScreen() override
    {
        if (m_open)
            write();
    }

    void start(const QString& path) override
    {
        m_path = path;
        m_open = true;
        m_lines.clear();
        m_timer.start();
        if (m_failAfterMs > 0) {
            QTimer::singleShot(m_failAfterMs, this, [this] {
                emit failed(QStringLiteral("panne simulée de la capture"));
                stop(); // fermeture tardive : le fichier s'écrit encore après la panne
            });
        }
    }
    void encode(const QVideoFrame& frame) override
    {
        const QImage image = frame.toImage();
        if (image.isNull())
            return;
        const QPoint topLeft = positionAt(image, 0, 0);
        const QPoint bottomRight = positionAt(image, image.width() - 1, image.height() - 1);
        m_lines << QStringList{QString::number(image.width()),     QString::number(image.height()),
                               QString::number(topLeft.x()),       QString::number(topLeft.y()),
                               QString::number(bottomRight.x()),   QString::number(bottomRight.y()),
                               QString::number(frame.startTime()), QString::number(frame.endTime())}
                       .join(u' ');
    }
    void stop() override
    {
        m_timer.stop();
        QTimer::singleShot(80, this, [this] {
            if (!m_open)
                return;
            m_open = false;
            write();
            emit stopped(target());
        });
    }

private:
    QString target() const
    {
        return m_path.isEmpty() ? m_defaultFolder + QStringLiteral("/enregistrement.mp4") : m_path;
    }
    void write() const
    {
        QFile file(target());
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
            file.write((m_lines.join(u'\n') + u'\n').toUtf8());
    }

    QString m_defaultFolder;
    int m_failAfterMs = 0;
    QImage m_pattern;
    QTimer m_timer;
    QString m_path;
    QStringList m_lines;
    bool m_open = false;
};

// Capture réelle de l'écran principal : sous Windows seulement (QScreenCapture et MP4 du moteur Windows).
QString screenUnavailable()
{
#if defined(Q_OS_WIN)
    return {};
#else
    return QStringLiteral("Windows seulement");
#endif
}

QString canonicalFolder(const QString& path)
{
    return QFileInfo(path).canonicalPath();
}

// Aucun fichier hors du dossier .tmp/ du dépôt.
QString repositoryTmp()
{
    return QFileInfo(QString::fromUtf8(__FILE__)).absoluteDir().absoluteFilePath(QStringLiteral("../.tmp"));
}

} // namespace

#define SKIP_WITHOUT_SCREEN(screen)                                                                          \
    do {                                                                                                     \
        if (screen) {                                                                                        \
            const QString why = screenUnavailable();                                                         \
            if (!why.isEmpty())                                                                              \
                QSKIP(qPrintable(why));                                                                      \
        }                                                                                                    \
    } while (false)

class TstMediaDisplayrecorder : public QObject
{
    Q_OBJECT

    std::unique_ptr<QTemporaryDir> m_root;
    QString m_folder;    // dossier temporaire du document : seul endroit permis
    QString m_elsewhere; // emplacement par défaut du moteur : doit rester vide
    QPointer<DisplayEngine> m_lastEngine;
    int m_enginesMade = 0;
    int m_failAfterMs = 0;
    int m_case = 0;

    std::unique_ptr<DisplayRecorder> makeRecorder(bool screen)
    {
        DisplayRecorder::EngineFactory engines;
        if (screen) {
            engines = [this]() -> std::unique_ptr<DisplayEngine> {
                ++m_enginesMade;
                auto engine = DisplayRecorder::createScreenEngine();
                m_lastEngine = engine.get();
                return engine;
            };
        } else {
            engines = [this]() -> std::unique_ptr<DisplayEngine> {
                ++m_enginesMade;
                auto engine = std::make_unique<SimulatedScreen>(m_elsewhere, m_failAfterMs);
                m_lastEngine = engine.get();
                return engine;
            };
        }
        auto recorder = std::make_unique<DisplayRecorder>(std::move(engines));
        recorder->setFolder(m_folder);
        return recorder;
    }

    void addEngines()
    {
        QTest::addColumn<bool>("screen");
        QTest::newRow("simulee") << false;
        QTest::newRow("ecran") << true;
    }

    QStringList filesIn(const QString& folder) const
    {
        return QDir(folder).entryList(QDir::Files | QDir::Hidden | QDir::System);
    }

    // Enregistre (pauses comprises dans waitMs) puis arrête ; rend le seul fichier confié.
    QString recordAndStop(DisplayRecorder& recorder, int waitMs)
    {
        QStringList handed;
        recorder.setInsertHandler([&handed](const QString& path) { handed << path; });
        recorder.toggle();
        if (!recorder.isRecording())
            return {};
        QTest::qWait(waitMs);
        recorder.stop();
        QTest::qWaitFor([&handed] { return !handed.isEmpty(); }, 15000);
        recorder.setInsertHandler({});
        return handed.size() == 1 ? handed.first() : QString();
    }

private slots:
    void initTestCase()
    {
        QVERIFY(QDir().mkpath(repositoryTmp()));
        m_root = std::make_unique<QTemporaryDir>(repositoryTmp() +
                                                 QStringLiteral("/tst_media_displayrecorder-XXXXXX"));
        QVERIFY(m_root->isValid());
    }

    void init()
    {
        ++m_case;
        m_folder = m_root->filePath(QStringLiteral("document-%1").arg(m_case)); // créé par l'enregistreur
        m_elsewhere = m_root->filePath(QStringLiteral("ailleurs-%1").arg(m_case));
        QVERIFY(QDir().mkpath(m_elsewhere));
        m_lastEngine.clear();
        m_enginesMade = 0;
        m_failAfterMs = 0;
    }

    // Hors Windows, le module est construit sans moteur d'écran : enregistreur indisponible, message.
    // Sous Windows, le vrai moteur (QScreenCapture, QMediaRecorder) écrit du MP4 encodable ici.
    void notBuiltOutsideWindows()
    {
        DisplayRecorder recorder; // moteur de la plateforme
        recorder.setFolder(m_folder);
#if defined(Q_OS_WIN)
        QVERIFY(recorder.isAvailable());
        QVERIFY(recorder.message().isEmpty());
        const std::unique_ptr<DisplayEngine> engine = DisplayRecorder::createScreenEngine();
        QVERIFY(engine);
        const QMediaFormat format = engine->mediaFormat();
        QCOMPARE(format.fileFormat(), QMediaFormat::MPEG4);
        QCOMPARE(format.videoCodec(), QMediaFormat::VideoCodec::H264);
        QVERIFY2(format.isSupported(QMediaFormat::Encode), "MP4 H.264 non encodable par Qt Multimedia");
#else
        QVERIFY(!recorder.isAvailable());
        QVERIFY2(recorder.message().contains(QStringLiteral("Windows seulement")),
                 qPrintable(recorder.message()));
        QVERIFY(!DisplayRecorder::createScreenEngine());
        int handed = 0;
        recorder.setInsertHandler([&handed](const QString&) { ++handed; });
        recorder.toggle(); // commande absente ; même appelée, rien ne démarre
        QVERIFY(!recorder.isRecording());
        QTest::qWait(100);
        QCOMPARE(handed, 0);
        QVERIFY(filesIn(m_folder).isEmpty());
#endif
    }

    void fullScreenFiveSeconds_data() { addEngines(); }
    void fullScreenFiveSeconds()
    {
        QFETCH(bool, screen);
        SKIP_WITHOUT_SCREEN(screen);
        const auto recorder = makeRecorder(screen);
        const QString path = recordAndStop(*recorder, 5000);
        QVERIFY2(!path.isEmpty(), "aucun fichier confié, ou plus d'une fois");
        QVERIFY(QFileInfo::exists(path));
        QCOMPARE(QFileInfo(path).suffix(), QStringLiteral("mp4"));
        QCOMPARE(canonicalFolder(path), QDir(m_folder).canonicalPath());
        const Recorded file = readRecorded(path, screen);
        QVERIFY2(file.valid, qPrintable(file.why));
        QVERIFY2(qAbs(file.milliseconds - 5000) <= 500,
                 qPrintable(QStringLiteral("%1 ms enregistrées pour 5 s").arg(file.milliseconds)));
        if (file.positions) {
            QCOMPARE(file.size, QSize(screenWidth, screenHeight));
            QCOMPARE(file.topLeft, QPoint(0, 0));
            QCOMPARE(file.bottomRight, QPoint(screenWidth - 1, screenHeight - 1));
        }
    }

    // Discrimine le patron écarté : un encodeur réglé à 320 × 240 (ou l'image entière mise à l'échelle)
    // donne des images de la bonne taille, mais leurs coins seraient (0, 0) et (638, 478) de l'écran.
    void areaIsCropped_data() { addEngines(); }
    void areaIsCropped()
    {
        QFETCH(bool, screen);
        SKIP_WITHOUT_SCREEN(screen);
        const QRect area(100, 60, 320, 240);
        const auto recorder = makeRecorder(screen);
        recorder->setArea(area);
        QCOMPARE(recorder->area(), area);
        const QString path = recordAndStop(*recorder, screen ? 2000 : 1000);
        QVERIFY2(!path.isEmpty(), "aucun fichier confié, ou plus d'une fois");
        const Recorded file = readRecorded(path, screen);
        QVERIFY2(file.valid, qPrintable(file.why));
        QCOMPARE(file.size, area.size());
        if (file.positions) {
            QCOMPARE(file.topLeft, area.topLeft());
            QCOMPARE(file.bottomRight, area.bottomRight());
        }
    }

    // Zone qui déborde de l'écran et côtés impairs : coupée au bord, côtés ramenés au pair (H.264 4:2:0).
    void areaIsClippedToEvenSides()
    {
        const auto recorder = makeRecorder(false);
        recorder->setArea(QRect(500, 401, 201, 101));
        const QString path = recordAndStop(*recorder, 500);
        QVERIFY(!path.isEmpty());
        const Recorded file = readSimulated(path);
        QVERIFY2(file.valid, qPrintable(file.why));
        QCOMPARE(file.size, QSize(140, 78));
        QCOMPARE(file.topLeft, QPoint(500, 401));
        QCOMPARE(file.bottomRight, QPoint(639, 478));
    }

    // Discrimine le patron écarté : des images horodatées à l'horloge murale garderaient la pause dans le
    // fichier (5 s, et un trou de 2 s entre deux images) ; des images encodées pendant la pause aussi.
    void pauseExcludesThePause_data() { addEngines(); }
    void pauseExcludesThePause()
    {
        QFETCH(bool, screen);
        SKIP_WITHOUT_SCREEN(screen);
        const auto recorder = makeRecorder(screen);
        QStringList handed;
        recorder->setInsertHandler([&handed](const QString& path) { handed << path; });
        QSignalSpy paused(recorder.get(), &DisplayRecorder::pausedChanged);
        recorder->toggle();
        QVERIFY(recorder->isRecording());
        QTest::qWait(1500);
        recorder->toggle(); // pause
        QVERIFY(recorder->isPaused());
        QVERIFY(recorder->isRecording()); // l'indicateur reste visible pendant la pause
        QTest::qWait(150);
        const qint64 held = recorder->duration();
        QTest::qWait(1850);
        QVERIFY2(recorder->duration() - held < 100, "la durée avance pendant la pause");
        recorder->toggle(); // reprise
        QVERIFY(!recorder->isPaused());
        QCOMPARE(paused.count(), 2);
        QTest::qWait(1500);
        recorder->stop();
        QTRY_COMPARE_WITH_TIMEOUT(handed.size(), 1, 15000);
        const Recorded file = readRecorded(handed.first(), screen);
        QVERIFY2(file.valid, qPrintable(file.why));
        QVERIFY2(qAbs(file.milliseconds - 3000) <= 500,
                 qPrintable(QStringLiteral("%1 ms enregistrées pour 3 s hors pause").arg(file.milliseconds)));
        if (file.positions) {
            QVERIFY2(file.largestGapMs < 200,
                     qPrintable(QStringLiteral("trou de %1 ms dans le fichier").arg(file.largestGapMs)));
            QVERIFY2(file.frames <= 3300 / 20,
                     qPrintable(QStringLiteral("%1 images encodées pour 3 s").arg(file.frames)));
        }
    }

    // Abandon : le fichier s'efface quand le moteur l'a fermé (effacer dans cancel() précéderait l'écriture).
    void cancelDeletesTheFile_data() { addEngines(); }
    void cancelDeletesTheFile()
    {
        QFETCH(bool, screen);
        SKIP_WITHOUT_SCREEN(screen);
        const auto recorder = makeRecorder(screen);
        int handed = 0;
        recorder->setInsertHandler([&handed](const QString&) { ++handed; });
        QSignalSpy finished(recorder.get(), &DisplayRecorder::finished);
        recorder->toggle();
        QVERIFY(recorder->isRecording());
        QTest::qWait(1000);
        recorder->cancel();
        QTRY_VERIFY_WITH_TIMEOUT(!recorder->isRecording(), 15000);
        QTRY_VERIFY(m_lastEngine.isNull());
        QTest::qWait(300); // la fermeture du moteur est passée
        QCOMPARE(handed, 0);
        QCOMPARE(finished.count(), 0);
        QVERIFY2(filesIn(m_folder).isEmpty(), qPrintable(filesIn(m_folder).join(u' ')));
        QVERIFY(filesIn(m_elsewhere).isEmpty());
    }

    // Patron de l'enregistreur de son : un rappel appelé dans stop() verserait un fichier que le moteur n'a
    // pas encore fermé ; une boucle locale qui attendrait la fermeture tiendrait le fil principal ;
    // l'emplacement par défaut du moteur écrirait hors du dossier du document.
    void stopHandsTheFileOnceClosed()
    {
        const auto recorder = makeRecorder(false);
        QStringList handed;
        bool insideStop = false;
        bool handedInsideStop = false;
        bool completeWhenHanded = false;
        recorder->setInsertHandler([&](const QString& path) {
            handed << path;
            handedInsideStop = handedInsideStop || insideStop;
            completeWhenHanded = readSimulated(path).valid;
        });
        QSignalSpy finished(recorder.get(), &DisplayRecorder::finished);
        recorder->toggle();
        QVERIFY(recorder->isRecording());
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
        QVERIFY2(recorder->isRecording(), "indicateur éteint avant la fermeture du fichier");

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

    // Panne en cours d'enregistrement : message, aucun rappel ; le fichier, réécrit par la fermeture tardive
    // ou par la destruction du moteur, est effacé après elles.
    void engineFailureLeavesNothing()
    {
        m_failAfterMs = 300;
        const auto recorder = makeRecorder(false);
        int handed = 0;
        recorder->setInsertHandler([&handed](const QString&) { ++handed; });
        QSignalSpy finished(recorder.get(), &DisplayRecorder::finished);
        QSignalSpy failed(recorder.get(), &DisplayRecorder::failed);
        recorder->toggle();
        QVERIFY(recorder->isRecording());
        QTRY_COMPARE(failed.count(), 1);
        QCOMPARE(failed.first().at(0).toString(), QStringLiteral("panne simulée de la capture"));
        QVERIFY(!recorder->isRecording());
        QVERIFY(!recorder->isPaused());
        QTRY_VERIFY(m_lastEngine.isNull());
        QTest::qWait(300);
        QCOMPARE(handed, 0);
        QCOMPARE(finished.count(), 0);
        QVERIFY2(filesIn(m_folder).isEmpty(), qPrintable(filesIn(m_folder).join(u' ')));
        QVERIFY(recorder->isAvailable()); // une panne ne désactive pas la commande
    }

    // Un moteur par enregistrement : l'écran est relâché entre deux enregistrements, noms distincts.
    void engineReleasedBetweenRecordings()
    {
        const auto recorder = makeRecorder(false);
        QStringList paths;
        for (int round = 1; round <= 2; ++round) {
            paths << recordAndStop(*recorder, 200);
            QCOMPARE(m_enginesMade, round);
            QVERIFY(!paths.last().isEmpty());
            QTRY_VERIFY2(m_lastEngine.isNull(), "moteur gardé après l'arrêt : capture non relâchée");
        }
        QCOMPARE(filesIn(m_folder).size(), 2);
        QVERIFY(paths.at(0) != paths.at(1));
    }

    void destroyedWhileRecordingLeavesNothing()
    {
        auto recorder = makeRecorder(false);
        int handed = 0;
        recorder->setInsertHandler([&handed](const QString&) { ++handed; });
        recorder->toggle();
        QVERIFY(recorder->isRecording());
        QTest::qWait(300);
        recorder.reset(); // document fermé : le moteur écrit son fichier partiel en se détruisant
        QVERIFY(m_lastEngine.isNull());
        QTest::qWait(200);
        QCOMPARE(handed, 0);
        QVERIFY2(filesIn(m_folder).isEmpty(), qPrintable(filesIn(m_folder).join(u' ')));
    }
};

// Sous macOS, sur le répartiteur Core Foundation (voir tst_media_player.cpp) : le moteur darwin de Qt
// Multimedia ne livre rien sinon. Sous Windows, la capture vise l'écran principal réel : greffon de
// plateforme Windows (le greffon hors écran n'a pas d'écran à capturer) ; aucune fenêtre n'est ouverte.
// Réglages d'essai (M7) : QStandardPaths en mode test.
int main(int argc, char* argv[])
{
#if defined(Q_OS_DARWIN)
    qputenv("QT_EVENT_DISPATCHER_CORE_FOUNDATION", "1");
#endif
#if defined(Q_OS_WIN)
    qputenv("QT_QPA_PLATFORM", "windows");
#endif
    QGuiApplication app(argc, argv);
    QStandardPaths::setTestModeEnabled(true);
    TstMediaDisplayrecorder test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}
#include "tst_media_displayrecorder.moc"
