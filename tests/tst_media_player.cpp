#include "actcraft/media/player.h"
#include "actcraft/ui/linklauncher.h"

#include <QAudioDevice>
#include <QDataStream>
#include <QDesktopServices>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QUrl>
#include <QtTest>

using actcraft::media::Player;
using actcraft::ui::LinkLauncher;

namespace {

// Son muet (PCM 16 bits mono) : la lecture avance sans rien faire entendre.
QString writeSilentWav(const QString& dir, const QString& name, int milliseconds)
{
    constexpr quint32 rate = 22050;
    constexpr quint16 channels = 1;
    constexpr quint16 bits = 16;
    const quint32 dataSize = rate * static_cast<quint32>(milliseconds) / 1000 * channels * (bits / 8);
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
    bytes.append(QByteArray(static_cast<qsizetype>(dataSize), '\0'));
    const QString path = dir + u'/' + name;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size())
        return {};
    return path;
}

// Fichier texte sous un nom de son : Qt Multimedia ne sait pas le lire.
QString writeUnreadable(const QString& dir, const QString& name)
{
    const QString path = dir + u'/' + name;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return {};
    for (int i = 0; i < 200; ++i)
        file.write("Ceci est un texte, pas un son.\n");
    return path;
}

bool hasAudioOutput()
{
    return !QMediaDevices::audioOutputs().isEmpty();
}

} // namespace

// Piège : toute ouverture par QDesktopServices::openUrl d'un fichier local passe ici au lieu d'ouvrir quoi
// que ce soit. Seul LinkLauncher, après « oui », a le droit d'ouvrir (et ses tests lui posent un faux
// ouvreur).
class OpenTrap : public QObject
{
    Q_OBJECT
public:
    OpenTrap() { QDesktopServices::setUrlHandler(QStringLiteral("file"), this, "open"); }
    ~OpenTrap() override { QDesktopServices::unsetUrlHandler(QStringLiteral("file")); }
    QList<QUrl> urls;
public slots:
    void open(const QUrl& url) { urls << url; }
};

class TstMediaPlayer : public QObject
{
    Q_OBJECT
private:
    QTemporaryDir m_dir;
    QString m_short;      // 0,5 s
    QString m_second;     // 1 s
    QString m_long;       // 4 s
    QString m_unreadable; // texte nommé .mp3

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        m_short = writeSilentWav(m_dir.path(), QStringLiteral("court.wav"), 500);
        m_second = writeSilentWav(m_dir.path(), QStringLiteral("seconde.wav"), 1000);
        m_long = writeSilentWav(m_dir.path(), QStringLiteral("long.wav"), 4000);
        m_unreadable = writeUnreadable(m_dir.path(), QStringLiteral("illisible.mp3"));
        QVERIFY(!m_short.isEmpty() && !m_second.isEmpty() && !m_long.isEmpty() && !m_unreadable.isEmpty());
    }

    void wavPlaysAndPositionAdvances()
    {
        if (!hasAudioOutput())
            QSKIP("aucune sortie audio");
        Player player;
        QSignalSpy positions(&player, &Player::positionChanged);
        player.play(m_second, Player::First, false);
        QTRY_VERIFY(player.position(Player::First) > 0);
        QVERIFY(player.isPlaying(Player::First));
        QVERIFY(!player.isPlaying(Player::Second));
        QTRY_VERIFY(!positions.isEmpty()); // signal périodique : il suit la position, sans lui être synchrone
        QCOMPARE(positions.last().at(0).toInt(), int(Player::First));
    }

    void twoTracksAtOnce()
    {
        if (!hasAudioOutput())
            QSKIP("aucune sortie audio");
        Player player;
        player.play(m_long, Player::First, false);
        player.play(m_second, Player::Second, false);
        QTRY_VERIFY(player.position(Player::First) > 0 && player.position(Player::Second) > 0);
        QVERIFY(player.isPlaying(Player::First));
        QVERIFY(player.isPlaying(Player::Second));
    }

    void secondSoundOnTrackOneReplacesTheFirst()
    {
        if (!hasAudioOutput())
            QSKIP("aucune sortie audio");
        Player player;
        QSignalSpy finished(&player, &Player::finished);
        player.play(m_long, Player::First, false);
        QTRY_VERIFY(player.position(Player::First) > 0);

        // Le son court remplace le long : il finit bien avant les 4 s du premier, et rien ne joue après lui.
        QElapsedTimer clock;
        clock.start();
        player.play(m_short, Player::First, false);
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 3000);
        QVERIFY2(clock.elapsed() < 3000, "le premier son n'a pas été remplacé");
        QCOMPARE(finished.first().at(0).toInt(), int(Player::First));
        QSignalSpy positions(&player, &Player::positionChanged);
        QTest::qWait(600);
        for (const QList<QVariant>& emitted : positions)
            QVERIFY2(emitted.at(1).toLongLong() == 0, "le premier son joue encore sous le second");
        QCOMPARE(finished.count(), 1);
        QVERIFY(!player.isPlaying(Player::First));
        QVERIFY(!player.isPlaying(Player::Second));
    }

    void unreadableFallsBackToTheSystem()
    {
        OpenTrap trap;
        Player player;
        QStringList handed;
        player.setFallback([&handed](const QString& path) { handed << path; });
        QSignalSpy fellBack(&player, &Player::fellBack);
        player.play(m_unreadable, Player::First, false);
        QTRY_COMPARE(fellBack.count(), 1);
        QCOMPARE(fellBack.first().at(0).toString(), m_unreadable);
        QTest::qWait(300); // une erreur et un état « illisible » arrivent tous deux : un seul repli
        QCOMPARE(fellBack.count(), 1);
        QCOMPARE(handed, QStringList{m_unreadable});
        QVERIFY(!player.isPlaying(Player::First));
        QVERIFY2(trap.urls.isEmpty(), "media/ a ouvert le fichier lui-même, sans confirmation");
    }

    // Discrimine le patron interdit : media/ qui ouvrirait par QDesktopServices contournerait la
    // confirmation.
    void fallbackNeedsConfirmation()
    {
        OpenTrap trap;
        LinkLauncher launcher;
        QList<QUrl> opened;
        launcher.setOpener([&opened](const QUrl& url) {
            opened << url;
            return true;
        });
        QSignalSpy asked(&launcher, &LinkLauncher::confirmationNeeded);
        QSignalSpy refused(&launcher, &LinkLauncher::refused);
        Player player;
        player.setFallback(
            [&launcher](const QString& path) { launcher.request(path, LinkLauncher::SystemMedia); });
        QSignalSpy fellBack(&player, &Player::fellBack);

        player.play(m_unreadable, Player::First, false);
        QTRY_COMPARE(fellBack.count(), 1);
        QCOMPARE(refused.count(), 0);
        QCOMPARE(asked.count(), 1);
        QVERIFY(asked.first().at(0).toString().contains(QStringLiteral("illisible.mp3")));
        QTest::qWait(300);
        QVERIFY2(opened.isEmpty(), "ouvert sans réponse");
        QVERIFY2(trap.urls.isEmpty(), "ouvert par QDesktopServices, hors de LinkLauncher");

        launcher.answer(asked.first().at(1).toInt(), true);
        QCOMPARE(opened.size(), 1);
        QCOMPARE(opened.first(), QUrl::fromLocalFile(m_unreadable));
        QVERIFY(trap.urls.isEmpty());
    }

    // Discrimine l'autre patron interdit : play() qui attendrait le chargement (boucle locale) traiterait
    // les événements en attente avant de rendre la main.
    void playNeverWaitsForTheMedia()
    {
        Player player;
        QSignalSpy fellBack(&player, &Player::fellBack);
        bool ranInsidePlay = false;
        QTimer::singleShot(0, this, [&ranInsidePlay] { ranInsidePlay = true; });
        QElapsedTimer clock;
        clock.start();
        player.play(m_unreadable, Player::First, false);
        player.play(m_second, Player::Second, false);
        const qint64 spent = clock.elapsed();
        QVERIFY2(!ranInsidePlay, "play() a fait tourner la boucle d'événements");
        QVERIFY2(spent < 50, qPrintable(QStringLiteral("play() a tenu la main %1 ms").arg(spent)));
        QTRY_COMPARE(fellBack.count(), 1);
        QVERIFY(ranInsidePlay);
    }

    void neverOnTheRenderThread()
    {
        QThread* const testThread = QThread::currentThread();
        QList<QThread*> seen;
        Player player;
        player.setFallback([&seen](const QString&) { seen << QThread::currentThread(); });
        connect(&player, &Player::fellBack, this, [&seen] { seen << QThread::currentThread(); });
        connect(&player, &Player::positionChanged, this, [&seen] { seen << QThread::currentThread(); });
        connect(&player, &Player::finished, this, [&seen] { seen << QThread::currentThread(); });
        QSignalSpy fellBack(&player, &Player::fellBack);
        QSignalSpy finished(&player, &Player::finished);
        player.play(m_unreadable, Player::First, false);
        if (hasAudioOutput())
            player.play(m_short, Player::Second, false);
        QTRY_COMPARE(fellBack.count(), 1);
        if (hasAudioOutput())
            QTRY_COMPARE(finished.count(), 1);
        QVERIFY(seen.size() >= 2);
        for (QThread* thread : seen)
            QCOMPARE(thread, testThread);
    }

    void stopAllOnDocumentClose()
    {
        Player player;
        int handed = 0;
        player.setFallback([&handed](const QString&) { ++handed; });
        QSignalSpy fellBack(&player, &Player::fellBack);
        if (hasAudioOutput()) {
            player.play(m_long, Player::First, true);
            player.play(m_long, Player::Second, true);
            QTRY_VERIFY(player.position(Player::First) > 0 && player.position(Player::Second) > 0);
        }
        player.play(m_unreadable, Player::First, false); // repli encore en attente à la fermeture
        player.stopAll();
        const QList<QMediaPlayer*> engines = player.findChildren<QMediaPlayer*>();
        QCOMPARE(engines.size(), 2);
        for (QMediaPlayer* engine : engines)
            QCOMPARE(engine->playbackState(), QMediaPlayer::StoppedState); // pas seulement oubliés
        QVERIFY(!player.isPlaying(Player::First));
        QVERIFY(!player.isPlaying(Player::Second));
        QCOMPARE(player.position(Player::First), 0);
        QCOMPARE(player.position(Player::Second), 0);

        // Témoin : un autre lecteur reçoit le même fichier ; quand il a basculé, le lecteur fermé l'aurait
        // fait.
        Player witness;
        QSignalSpy witnessFell(&witness, &Player::fellBack);
        witness.play(m_unreadable, Player::First, false);
        QTRY_COMPARE(witnessFell.count(), 1);
        QSignalSpy positions(&player, &Player::positionChanged);
        QTest::qWait(300);
        QCOMPARE(fellBack.count(), 0);
        QCOMPARE(handed, 0);
        QVERIFY(positions.isEmpty());
        QVERIFY(!player.isPlaying(Player::Second));
    }

    void missingFileNeitherPlaysNorFallsBack()
    {
        OpenTrap trap;
        Player player;
        int handed = 0;
        player.setFallback([&handed](const QString&) { ++handed; });
        QSignalSpy fellBack(&player, &Player::fellBack);
        QSignalSpy failed(&player, &Player::failed);
        player.play(m_dir.path() + QStringLiteral("/absent.wav"), Player::First, false);
        QCOMPARE(failed.count(), 0); // jamais avant le retour de play()
        Player witness;
        QSignalSpy witnessFell(&witness, &Player::fellBack);
        witness.play(m_unreadable, Player::First, false);
        QTRY_COMPARE(witnessFell.count(), 1);
        QCOMPARE(fellBack.count(), 0);
        QCOMPARE(handed, 0);
        QCOMPARE(failed.count(), 1);
        QCOMPARE(failed.first().at(0).toInt(), int(Player::First));
        QVERIFY(!player.isPlaying(Player::First));
        QVERIFY(trap.urls.isEmpty());
    }

    // Revue, point 1 : l'arrêt du son lisible ne doit pas marquer « chargé » le fichier illisible qui le
    // suit.
    void readableThenUnreadableOnTheSameTrackFallsBack()
    {
        Player player;
        QSignalSpy fellBack(&player, &Player::fellBack);
        player.play(m_second, Player::First, false);
        if (hasAudioOutput())
            QTRY_VERIFY(player.position(Player::First) > 0);
        else
            QTest::qWait(500);
        player.play(m_unreadable, Player::First, false);
        QTRY_COMPARE(fellBack.count(), 1);
        QCOMPARE(fellBack.first().at(0).toString(), m_unreadable);
    }

    // Revue, point 2 : rejouer le même fichier illisible le recharge et le confie de nouveau.
    void sameUnreadableTwiceFallsBackTwice()
    {
        Player player;
        QSignalSpy fellBack(&player, &Player::fellBack);
        player.play(m_unreadable, Player::First, false);
        QTRY_COMPARE(fellBack.count(), 1);
        player.play(m_unreadable, Player::First, false);
        QTRY_COMPARE(fellBack.count(), 2);
    }

    // Revue, point 4 : seul un format illisible part au système ; un accès refusé est une erreur signalée.
    void deniedFileIsReportedWithoutFallback()
    {
        const QString denied = writeSilentWav(m_dir.path(), QStringLiteral("interdit.wav"), 500);
        QVERIFY(!denied.isEmpty());
        // Sous Windows, Qt ne retire pas la lecture (droits NTFS) : setPermissions rend faux, le test est
        // sauté.
        if (!QFile::setPermissions(denied, QFileDevice::Permissions{}) || QFileInfo(denied).isReadable())
            QSKIP("droits de lecture non retirables sur ce système de fichiers");
        OpenTrap trap;
        Player player;
        int handed = 0;
        player.setFallback([&handed](const QString&) { ++handed; });
        QSignalSpy fellBack(&player, &Player::fellBack);
        QSignalSpy failed(&player, &Player::failed);
        player.play(denied, Player::First, false);
        QTRY_COMPARE(failed.count(), 1);
        QTest::qWait(300);
        QCOMPARE(failed.count(), 1);
        QCOMPARE(fellBack.count(), 0);
        QCOMPARE(handed, 0);
        QVERIFY(trap.urls.isEmpty());
        QFile::setPermissions(denied, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }

    // Revue, point 5 : un chemin réseau est refusé avant tout accès au disque, sans repli.
    void networkPathIsRefusedWithoutFallback()
    {
        Player player;
        int handed = 0;
        player.setFallback([&handed](const QString&) { ++handed; });
        QSignalSpy fellBack(&player, &Player::fellBack);
        QSignalSpy failed(&player, &Player::failed);
        QElapsedTimer clock;
        clock.start();
        player.play(QStringLiteral("//serveur.invalid/partage/son.wav"), Player::First, false);
        player.play(QStringLiteral("\\\\serveur.invalid\\partage\\son.wav"), Player::Second, false);
        QVERIFY(clock.elapsed() < 50);
        QTRY_COMPARE(failed.count(), 2);
        QTest::qWait(200);
        QCOMPARE(fellBack.count(), 0);
        QCOMPARE(handed, 0);
    }

    void pauseHoldsThePosition()
    {
        if (!hasAudioOutput())
            QSKIP("aucune sortie audio");
        Player player;
        player.play(m_long, Player::First, false);
        QTRY_VERIFY(player.position(Player::First) > 0);
        player.pause(Player::First);
        QTRY_VERIFY(!player.isPlaying(Player::First));
        // L'état passe à « en pause » avant que le moteur ait fini de ralentir : la position lue sur-le-champ
        // peut encore gagner quelques millisecondes (14 ms vus sous AVFoundation). Référence prise après.
        QTest::qWait(150);
        const qint64 held = player.position(Player::First);
        QTest::qWait(400);
        QCOMPARE(player.position(Player::First), held);
    }

    void loopKeepsPlaying()
    {
        if (!hasAudioOutput())
            QSKIP("aucune sortie audio");
        Player player;
        QSignalSpy finished(&player, &Player::finished);
        player.play(m_short, Player::First, true);
        QTRY_VERIFY(player.position(Player::First) > 0);
        QTest::qWait(1500); // trois fois la durée du son
        QCOMPARE(finished.count(), 0);
        QVERIFY(player.isPlaying(Player::First));
    }
};

// Sans interface et, sous macOS, sur le répartiteur Core Foundation : le moteur darwin de Qt Multimedia livre
// ses chargements par la file principale de GCD, que ni le répartiteur de la plateforme « offscreen » ni
// celui de QCoreApplication par défaut (QEventDispatcherUNIX) ne vident : rien n'arriverait jamais. Le
// lecteur n'a besoin d'aucun écran ; l'application, sous la plateforme cocoa, vide cette file d'elle-même.
int main(int argc, char* argv[])
{
#if defined(Q_OS_DARWIN)
    qputenv("QT_EVENT_DISPATCHER_CORE_FOUNDATION", "1");
#endif
    QCoreApplication app(argc, argv);
    TstMediaPlayer test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}
#include "tst_media_player.moc"
