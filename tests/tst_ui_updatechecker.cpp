#include <actcraft/ui/preferences.h>
#include <actcraft/ui/updatechecker.h>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

using actcraft::ui::Preferences;
using actcraft::ui::UpdateChecker;

namespace {

constexpr const char* kVariable = "ACTCRAFT_UPDATES_URL";

bool isLocal(const QUrl& url)
{
    return url.isLocalFile()
           || (url.scheme() == QLatin1String("http")
               && (url.host() == QLatin1String("127.0.0.1") || url.host() == QLatin1String("localhost")));
}

// Gestionnaire espion : compte les requêtes et ne sort jamais de la machine. Une adresse qui n'est ni file: ni
// la boucle locale est détournée vers un fichier absent : même sous une mutation, aucun accès réseau réel.
class SpyManager : public QNetworkAccessManager
{
public:
    using QNetworkAccessManager::QNetworkAccessManager;
    int created = 0;
    QList<QUrl> urls;

protected:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest& request, QIODevice* data) override
    {
        ++created;
        urls.append(request.url());
        if (isLocal(request.url()))
            return QNetworkAccessManager::createRequest(op, request, data);
        QNetworkRequest guarded = request;
        guarded.setUrl(QUrl::fromLocalFile(QStringLiteral("/actcraft-essai/aucun-reseau.json")));
        return QNetworkAccessManager::createRequest(op, guarded, data);
    }
};

// Serveur de la boucle locale qui répond 404, comme l'API publique d'un dépôt privé sans jeton.
class NotFoundServer : public QTcpServer
{
public:
    NotFoundServer()
    {
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket* socket = nextPendingConnection()) {
                connect(socket, &QTcpSocket::readyRead, socket, [socket] {
                    socket->readAll();
                    socket->write("HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n"
                                  "Content-Length: 24\r\nConnection: close\r\n\r\n"
                                  "{\"message\": \"Not Found\"}");
                    socket->disconnectFromHost();
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
    }
};

QByteArray fileUrl(const QString& path)
{
    return QUrl::fromLocalFile(path).toString().toUtf8();
}

} // namespace

class TstUiUpdatechecker : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void semverComparison();
    void noRequestAtStartByDefault();
    void requestAtStartWhenEnabled();
    void availableFromReply();
    void draftAndPrereleaseIgnored();
    void failureReported();
    void remoteOverrideRefused();

private:
    QString data(const char* name) const;

    QTemporaryDir m_settings;
};

QString TstUiUpdatechecker::data(const char* name) const
{
    return QFINDTESTDATA(QStringLiteral("data/updates/") + QString::fromLatin1(name));
}

void TstUiUpdatechecker::initTestCase()
{
    QVERIFY(m_settings.isValid());
    QStandardPaths::setTestModeEnabled(true);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
    QVERIFY(!data("newer.json").isEmpty());
    QVERIFY(!data("draft.json").isEmpty());
    QVERIFY(!data("prerelease.json").isEmpty());

    QSettings probe;
    QCOMPARE(probe.format(), QSettings::IniFormat);
    QVERIFY(probe.fileName().startsWith(m_settings.path()));
}

void TstUiUpdatechecker::init()
{
    qunsetenv(kVariable);
    Preferences preferences;
    preferences.remove(QStringLiteral("updates/check-at-start"));
}

void TstUiUpdatechecker::semverComparison()
{
    QVERIFY(UpdateChecker::isNewer(u"1.0.1", u"1.0.0"));
    QVERIFY(UpdateChecker::isNewer(u"1.10.0", u"1.9.0"));
    QVERIFY(UpdateChecker::isNewer(u"0.10.0", u"0.9.0"));
    QVERIFY(UpdateChecker::isNewer(u"v2.0.0", u"1.99.99"));
    QVERIFY(UpdateChecker::isNewer(u"1.0.0", u"v0.9.9"));
    QVERIFY(!UpdateChecker::isNewer(u"1.0.0", u"1.0.0"));
    QVERIFY(!UpdateChecker::isNewer(u"v1.0.0", u"1.0.0"));
    QVERIFY(!UpdateChecker::isNewer(u"1.0.0", u"1.0.1"));
    QVERIFY(!UpdateChecker::isNewer(u"zzz", u"1.0.0"));
    QVERIFY(!UpdateChecker::isNewer(u"1.0.0", u"zzz"));
    QVERIFY(!UpdateChecker::isNewer(u"", u"1.0.0"));
    QVERIFY(!UpdateChecker::isNewer(u"2.0", u"1.0.0"));
    QVERIFY(!UpdateChecker::isNewer(u"2.0.0-rc1", u"1.0.0"));
    QVERIFY(!UpdateChecker::isNewer(u"2..0", u"1.0.0"));
}

// Le test qui distingue : le patron de NetworkSettings (agir dès la construction, au démarrage) ferait une
// requête ici. Réglage absent du poste = aucune requête, ni par start() ni à la construction.
void TstUiUpdatechecker::noRequestAtStartByDefault()
{
    // Une requête fautive irait vers un fichier absent, jamais vers le réseau.
    qputenv(kVariable, fileUrl(m_settings.filePath(QStringLiteral("absent.json"))));
    Preferences preferences;
    SpyManager spy;
    UpdateChecker checker;
    checker.setNetworkAccessManager(&spy);
    checker.setPreferences(&preferences);
    QVERIFY(!checker.checkAtStart());

    checker.start();
    QCoreApplication::processEvents();
    QCOMPARE(spy.created, 0);
    QCOMPARE(checker.requestCount(), 0);
    QCOMPARE(checker.state(), UpdateChecker::State::Idle);

    // Sans préférences liées non plus.
    UpdateChecker bare;
    bare.setNetworkAccessManager(&spy);
    QVERIFY(!bare.checkAtStart());
    bare.start();
    QCoreApplication::processEvents();
    QCOMPARE(spy.created, 0);
    QCOMPARE(bare.requestCount(), 0);
    QCOMPARE(bare.state(), UpdateChecker::State::Idle);
}

void TstUiUpdatechecker::requestAtStartWhenEnabled()
{
    qputenv(kVariable, fileUrl(data("newer.json")));
    Preferences preferences;
    SpyManager spy;
    UpdateChecker checker;
    checker.setNetworkAccessManager(&spy);
    checker.setPreferences(&preferences);
    QSignalSpy changed(&checker, &UpdateChecker::checkAtStartChanged);

    checker.setCheckAtStart(true);
    QCOMPARE(changed.count(), 1);
    QVERIFY(checker.checkAtStart());
    checker.setCheckAtStart(true);
    QCOMPARE(changed.count(), 1);

    // Réglage du poste : relu par d'autres préférences.
    Preferences reread;
    UpdateChecker other;
    other.setPreferences(&reread);
    QVERIFY(other.checkAtStart());

    checker.start();
    QCOMPARE(spy.created, 1);
    QCOMPARE(spy.urls.value(0), UpdateChecker::endpoint());
    QCOMPARE(checker.requestCount(), 1);
    QTRY_COMPARE(checker.state(), UpdateChecker::State::Available);
}

void TstUiUpdatechecker::availableFromReply()
{
    qputenv(kVariable, fileUrl(data("newer.json")));
    UpdateChecker checker; // gestionnaire interne
    QSignalSpy changed(&checker, &UpdateChecker::stateChanged);

    checker.check();
    QCOMPARE(checker.state(), UpdateChecker::State::Checking);
    QCOMPARE(checker.requestCount(), 1);
    QTRY_COMPARE(checker.state(), UpdateChecker::State::Available);
    QCOMPARE(checker.latestVersion(), QStringLiteral("9.9.9"));
    QCOMPARE(checker.releasePage(),
             QUrl(QStringLiteral("https://github.com/jejeasterix/actcraft/releases/tag/v9.9.9")));
    QVERIFY(changed.count() >= 2);
}

void TstUiUpdatechecker::draftAndPrereleaseIgnored()
{
    for (const char* name : {"draft.json", "prerelease.json"}) {
        qputenv(kVariable, fileUrl(data(name)));
        SpyManager spy;
        UpdateChecker checker;
        checker.setNetworkAccessManager(&spy);
        checker.check();
        QTRY_COMPARE(checker.state(), UpdateChecker::State::UpToDate);
        QCOMPARE(spy.created, 1);
        QCOMPARE(checker.latestVersion(), QString());
        QCOMPARE(checker.releasePage(), QUrl());
    }
}

// Fichier absent, 404 de la boucle locale (dépôt privé sans jeton), réponse illisible : « impossible de
// vérifier », sans avertissement au journal, et une nouvelle vérification reste possible.
void TstUiUpdatechecker::failureReported()
{
    QTest::failOnWarning(QRegularExpression(QStringLiteral(".*")));
    SpyManager spy;
    UpdateChecker checker;
    checker.setNetworkAccessManager(&spy);

    qputenv(kVariable, fileUrl(m_settings.filePath(QStringLiteral("absent.json"))));
    checker.check();
    QTRY_COMPARE(checker.state(), UpdateChecker::State::Failed);
    QCOMPARE(checker.latestVersion(), QString());
    QCOMPARE(checker.releasePage(), QUrl());

    NotFoundServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    qputenv(kVariable, QByteArray("http://127.0.0.1:") + QByteArray::number(server.serverPort()) +
                           QByteArray("/repos/jejeasterix/actcraft/releases/latest"));
    checker.check();
    QCOMPARE(checker.state(), UpdateChecker::State::Checking);
    QTRY_COMPARE(checker.state(), UpdateChecker::State::Failed);
    QCOMPARE(checker.latestVersion(), QString());

    QFile unreadable(m_settings.filePath(QStringLiteral("illisible.json")));
    QVERIFY(unreadable.open(QIODevice::WriteOnly));
    unreadable.write("<html>pas du JSON</html>");
    unreadable.close();
    qputenv(kVariable, fileUrl(unreadable.fileName()));
    checker.check();
    QTRY_COMPARE(checker.state(), UpdateChecker::State::Failed);

    QCOMPARE(checker.requestCount(), 3);
    QCOMPARE(spy.created, 3);
}

void TstUiUpdatechecker::remoteOverrideRefused()
{
    const QUrl fallback(QStringLiteral("https://api.github.com/repos/jejeasterix/actcraft/releases/latest"));
    QCOMPARE(UpdateChecker::endpoint(), fallback);

    for (const char* refused :
         {"https://exemple.invalid/x", "http://exemple.invalid:8080/x", "http://127.0.0.1.exemple.invalid:80/x",
          "https://127.0.0.1:8443/x", "ftp://localhost:21/x", "pas une adresse"}) {
        qputenv(kVariable, refused);
        QCOMPARE(UpdateChecker::endpoint(), fallback);
    }

    for (const char* accepted : {"http://127.0.0.1:8080/latest", "http://localhost:8080/latest"}) {
        qputenv(kVariable, accepted);
        QCOMPARE(UpdateChecker::endpoint(), QUrl(QString::fromLatin1(accepted)));
    }
    const QByteArray local = fileUrl(data("newer.json"));
    qputenv(kVariable, local);
    QCOMPARE(UpdateChecker::endpoint(), QUrl(QString::fromUtf8(local)));
}

QTEST_MAIN(TstUiUpdatechecker)
#include "tst_ui_updatechecker.moc"
