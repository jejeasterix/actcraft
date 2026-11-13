#include "startup.h"

#include <actcraft/ui/commands.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/updatechecker.h>

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QScopeGuard>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include <utility>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using actcraft::ui::Preferences;
using actcraft::ui::UpdateChecker;

namespace {

constexpr const char* kVariable = "ACTCRAFT_UPDATES_URL";

bool isLocal(const QUrl& url)
{
    return url.isLocalFile() ||
           (url.scheme() == QLatin1String("http") &&
            (url.host() == QLatin1String("127.0.0.1") || url.host() == QLatin1String("localhost")));
}

// Gestionnaire espion : garde les requêtes et ne sort jamais de la machine. Une adresse qui n'est ni file: ni
// la boucle locale est détournée vers un fichier absent : même sous une mutation, aucun accès réseau réel.
class SpyManager : public QNetworkAccessManager
{
public:
    using QNetworkAccessManager::QNetworkAccessManager;
    int created = 0;
    QList<QNetworkRequest> requests;

protected:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest& request, QIODevice* data) override
    {
        ++created;
        requests.append(request);
        if (isLocal(request.url()))
            return QNetworkAccessManager::createRequest(op, request, data);
        QNetworkRequest guarded = request;
        guarded.setUrl(QUrl::fromLocalFile(QStringLiteral("/actcraft-essai/aucun-reseau.json")));
        return QNetworkAccessManager::createRequest(op, guarded, data);
    }
};

// Serveur de la boucle locale : garde la première ligne reçue et rend une réponse écrite d'avance (404 de
// l'API publique d'un dépôt privé sans jeton, ou refus d'un faux mandataire).
class ScriptedServer : public QTcpServer
{
public:
    explicit ScriptedServer(QByteArray response) : m_response(std::move(response))
    {
        connect(this, &QTcpServer::newConnection, this, [this] {
            while (QTcpSocket* socket = nextPendingConnection()) {
                connect(socket, &QTcpSocket::readyRead, socket, [this, socket] {
                    const QByteArray received = socket->readAll();
                    if (firstLine.isEmpty())
                        firstLine = received.left(received.indexOf('\r'));
                    socket->write(m_response);
                    socket->disconnectFromHost();
                });
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
    }

    QByteArray firstLine;

private:
    QByteArray m_response;
};

QByteArray fileUrl(const QString& path)
{
    return QUrl::fromLocalFile(path).toString().toUtf8();
}

QByteArray release(const QByteArray& htmlUrl, const QByteArray& extra = {})
{
    return QByteArray(R"({"tag_name": "v9.9.9", "html_url": ")") + htmlUrl +
           QByteArray(R"(", "draft": false, "prerelease": false)") + extra + QByteArray("}");
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
    void startCalledOnceByTheShell();
    void bannerShownOnlyForANewVersionAtStart_data();
    void bannerShownOnlyForANewVersionAtStart();
    void updateBannerTextAfterStartupDiscovery();
    void updateBannerKeepsDiscoveryDuringManualCheck();
    void updateBannerCloseHides();
    void updateBannerDoesNotCoverMessageBanner();
    void messageStaysBelowRecoveryBanner();

private:
    QString data(const char* name) const;
    void enableCheckAtStart(const QByteArray& url) const;
    QString writeFile(const QString& name, const QByteArray& content) const;

    QTemporaryDir m_settings;
};

QString TstUiUpdatechecker::data(const char* name) const
{
    return QFINDTESTDATA(QStringLiteral("data/updates/") + QString::fromLatin1(name));
}

QString TstUiUpdatechecker::writeFile(const QString& name, const QByteArray& content) const
{
    const QString path = m_settings.filePath(name);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(content) != content.size())
        return {};
    return path;
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
    // Vide : QStringView() et non u"", dont la longueur calculée laisse du code inaccessible (C4702, MSVC).
    QVERIFY(!UpdateChecker::isNewer(QStringView(), u"1.0.0"));
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
    QCOMPARE(checker.requestCount(), 1);
    QTRY_COMPARE(checker.state(), UpdateChecker::State::Available);

    // Une lecture sans cookie ni identifiant.
    const QNetworkRequest sent = spy.requests.value(0);
    QCOMPARE(sent.url(), UpdateChecker::endpoint());
    QCOMPARE(sent.attribute(QNetworkRequest::CookieLoadControlAttribute).toInt(),
             int(QNetworkRequest::Manual));
    QCOMPARE(sent.attribute(QNetworkRequest::CookieSaveControlAttribute).toInt(),
             int(QNetworkRequest::Manual));
    QVERIFY(!sent.hasRawHeader("Authorization"));
    QVERIFY(!sent.hasRawHeader("Cookie"));

    // Gestionnaire interne : la requête suit le mandataire de l'application (posé par NetworkSettings). Un
    // faux mandataire de la boucle locale reçoit le tunnel vers l'API et le refuse ; rien ne sort de la
    // machine.
    ScriptedServer proxy(
        QByteArray("HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"));
    QVERIFY(proxy.listen(QHostAddress::LocalHost));
    QNetworkProxy::setApplicationProxy(
        QNetworkProxy(QNetworkProxy::HttpProxy, QStringLiteral("127.0.0.1"), proxy.serverPort()));
    const auto restore = qScopeGuard([] { QNetworkProxy::setApplicationProxy(QNetworkProxy()); });
    qunsetenv(kVariable);
    UpdateChecker internal;
    internal.setPreferences(&reread);
    internal.start();
    QTRY_COMPARE(internal.state(), UpdateChecker::State::Failed);
    QVERIFY2(proxy.firstLine.startsWith("CONNECT api.github.com:443 "), proxy.firstLine.constData());
}

void TstUiUpdatechecker::availableFromReply()
{
    qputenv(kVariable, fileUrl(data("newer.json")));
    SpyManager spy;
    UpdateChecker checker;
    checker.setNetworkAccessManager(&spy);
    QSignalSpy changed(&checker, &UpdateChecker::stateChanged);

    checker.check();
    QCOMPARE(checker.state(), UpdateChecker::State::Checking);
    QCOMPARE(checker.requestCount(), 1);
    checker.check(); // une recherche à la fois
    QCOMPARE(checker.requestCount(), 1);
    QCOMPARE(spy.created, 1);
    QTRY_COMPARE(checker.state(), UpdateChecker::State::Available);
    QCOMPARE(checker.latestVersion(), QStringLiteral("9.9.9"));
    QCOMPARE(checker.releasePage(),
             QUrl(QStringLiteral("https://github.com/jejeasterix/actcraft/releases/tag/v9.9.9")));
    QVERIFY(changed.count() >= 2);

    // Une page hors des releases du dépôt ne s'ouvre pas : la page des releases la remplace.
    const QUrl releases(QStringLiteral("https://github.com/jejeasterix/actcraft/releases/latest"));
    for (const char* foreign :
         {"https://exemple.invalid/piege", "http://github.com/jejeasterix/actcraft/releases/tag/v9.9.9",
          "https://github.com/jejeasterix/actcraft/releases/../../autre/releases/tag/v9.9.9",
          "https://github.com.exemple.invalid/jejeasterix/actcraft/releases/tag/v9.9.9",
          "javascript:alert(1)"}) {
        const QString path = writeFile(QStringLiteral("etrangere.json"), release(foreign));
        QVERIFY(!path.isEmpty());
        qputenv(kVariable, fileUrl(path));
        checker.check();
        QTRY_COMPARE(checker.state(), UpdateChecker::State::Available);
        QCOMPARE(checker.releasePage(), releases);
    }
    QCOMPARE(spy.created, 6);
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

// Fichier absent, 404 de la boucle locale (dépôt privé sans jeton), réponse illisible ou trop longue, version
// courante illisible : « impossible de vérifier », sans avertissement au journal, et une nouvelle
// vérification reste possible.
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

    ScriptedServer server(QByteArray("HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n"
                                     "Content-Length: 24\r\nConnection: close\r\n\r\n"
                                     "{\"message\": \"Not Found\"}"));
    QVERIFY(server.listen(QHostAddress::LocalHost));
    qputenv(kVariable, QByteArray("http://127.0.0.1:") + QByteArray::number(server.serverPort()) +
                           QByteArray("/repos/jejeasterix/actcraft/releases/latest"));
    checker.check();
    QCOMPARE(checker.state(), UpdateChecker::State::Checking);
    QTRY_COMPARE(checker.state(), UpdateChecker::State::Failed);
    QCOMPARE(checker.latestVersion(), QString());
    QVERIFY(server.firstLine.startsWith("GET /repos/jejeasterix/actcraft/releases/latest "));

    const QString unreadable = writeFile(QStringLiteral("illisible.json"), "<html>pas du JSON</html>");
    QVERIFY(!unreadable.isEmpty());
    qputenv(kVariable, fileUrl(unreadable));
    checker.check();
    QTRY_COMPARE(checker.state(), UpdateChecker::State::Failed);

    // JSON valide, mais au-delà de 1 Mio.
    const QByteArray padding = QByteArray(R"(, "body": ")") + QByteArray(1100 * 1024, 'x') + QByteArray("\"");
    const QString large =
        writeFile(QStringLiteral("trop-long.json"),
                  release("https://github.com/jejeasterix/actcraft/releases/tag/v9.9.9", padding));
    QVERIFY(!large.isEmpty());
    qputenv(kVariable, fileUrl(large));
    checker.check();
    QTRY_COMPARE(checker.state(), UpdateChecker::State::Failed);

    // Version courante illisible : rien à comparer, pas de « à jour » de complaisance.
    qputenv(kVariable, fileUrl(data("newer.json")));
    checker.setCurrentVersion(QStringLiteral("1.0"));
    checker.check();
    QTRY_COMPARE(checker.state(), UpdateChecker::State::Failed);
    checker.setCurrentVersion(QStringLiteral("0.6.0"));
    checker.check();
    QTRY_COMPARE(checker.state(), UpdateChecker::State::Available);

    QCOMPARE(checker.requestCount(), 6);
    QCOMPARE(spy.created, 6);
}

void TstUiUpdatechecker::remoteOverrideRefused()
{
    const QUrl fallback(QStringLiteral("https://api.github.com/repos/jejeasterix/actcraft/releases/latest"));
    QCOMPARE(UpdateChecker::endpoint(), fallback);

    for (const char* refused : {"https://exemple.invalid/x", "http://exemple.invalid:8080/x",
                                "http://127.0.0.1.exemple.invalid:80/x", "https://127.0.0.1:8443/x",
                                "ftp://localhost:21/x", "file://hote/partage/x.json", "pas une adresse"}) {
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

// M15 : la coquille (Main.qml) appelle appUpdates.start() une seule fois, au chargement. Sans cet appel, la
// case « au démarrage » ne ferait rien. Réglage vrai : une requête, vers le fichier local ; faux : aucune.
void TstUiUpdatechecker::startCalledOnceByTheShell()
{
    qputenv(kVariable, fileUrl(data("newer.json")));
    for (const bool atStart : {true, false}) {
        {
            QSettings settings;
            settings.setValue(QStringLiteral("updates/check-at-start"), atStart);
            settings.sync();
        }
        QTemporaryDir profiles;
        QVERIFY(profiles.isValid());
        QQmlApplicationEngine engine;
        StartupOptions options;
        options.profileRoot = profiles.path();
        QQuickWindow* window = loadMainWindow(engine, options);
        QVERIFY(window);
        auto* checker = window->findChild<UpdateChecker*>(QStringLiteral("appUpdates"));
        QVERIFY(checker);
        QCOMPARE(checker->checkAtStart(), atStart);
        QCOMPARE(checker->requestCount(), atStart ? 1 : 0);
        if (atStart)
            QTRY_COMPARE(checker->state(), UpdateChecker::State::Available);
        else
            QCOMPARE(checker->state(), UpdateChecker::State::Idle);
        // check() ignore un appel pendant « Checking » : un second start() synchrone ne se voit pas d'ici
        // (sans toucher au code de production, aucun espion sur start()). Un second appel après la réponse,
        // lui, relancerait une requête : on laisse la boucle tourner et on recompte.
        QTest::qWait(150);
        QCOMPARE(checker->requestCount(), atStart ? 1 : 0);
    }
}

// Une ligne par fenêtre principale chargée : QTest arme sa limite de 300 s par ligne de données. Les quatre
// chargements dans une seule fonction en prenaient plus de 300 s sous Windows (run 43141999645, « Test
// function timed out » au quatrième).
void TstUiUpdatechecker::bannerShownOnlyForANewVersionAtStart_data()
{
    QTest::addColumn<QByteArray>("url");
    QTest::addColumn<bool>("atStart");
    QTest::addColumn<UpdateChecker::State>("expected");
    QTest::addColumn<bool>("bannerShown");
    QTest::newRow("nouvelle version")
        << fileUrl(data("newer.json")) << true << UpdateChecker::State::Available << true;
    QTest::newRow("à jour") << fileUrl(data("prerelease.json")) << true << UpdateChecker::State::UpToDate
                            << false;
    QTest::newRow("erreur") << fileUrl(m_settings.filePath(QStringLiteral("absent.json"))) << true
                            << UpdateChecker::State::Failed << false;
    QTest::newRow("réglage faux") << fileUrl(data("newer.json")) << false << UpdateChecker::State::Idle
                                  << false;
}

// L11-C17 (L11-T22) : la recherche du démarrage n'ouvre jamais la boîte modale ; le bandeau non modal
// (bannerShown) n'apparaît que pour une nouvelle version (ni à jour, ni en erreur, ni quand le réglage est
// faux).
void TstUiUpdatechecker::bannerShownOnlyForANewVersionAtStart()
{
    QFETCH(QByteArray, url);
    QFETCH(bool, atStart);
    QFETCH(UpdateChecker::State, expected);
    QFETCH(bool, bannerShown);
    qputenv(kVariable, url);
    {
        QSettings settings;
        settings.setValue(QStringLiteral("updates/check-at-start"), atStart);
        settings.sync();
    }
    QTemporaryDir profiles;
    QVERIFY(profiles.isValid());
    QQmlApplicationEngine engine;
    StartupOptions options;
    options.profileRoot = profiles.path();
    QQuickWindow* window = loadMainWindow(engine, options);
    QVERIFY(window);
    auto* checker = window->findChild<UpdateChecker*>(QStringLiteral("appUpdates"));
    auto* dialog = window->findChild<QObject*>(QStringLiteral("updateDialog"));
    auto* banner = window->findChild<QObject*>(QStringLiteral("updateBanner"));
    QVERIFY(checker && dialog && banner);
    if (atStart) {
        QTRY_COMPARE(checker->state(), expected);
    } else {
        // Réglage faux : aucune requête, donc aucune réponse qui ouvrirait le bandeau ou la boîte.
        QCOMPARE(checker->requestCount(), 0);
        QCOMPARE(checker->state(), expected);
    }
    QCoreApplication::processEvents();
    if (bannerShown)
        QTRY_VERIFY(banner->property("visible").toBool());
    else
        QVERIFY(!banner->property("visible").toBool());
    QTest::qWait(50);
    QVERIFY(!dialog->property("visible").toBool()); // jamais de boîte au démarrage
    QCOMPARE(checker->requestCount(), atStart ? 1 : 0);
}

void TstUiUpdatechecker::enableCheckAtStart(const QByteArray& url) const
{
    qputenv(kVariable, url);
    QSettings settings;
    settings.setValue(QStringLiteral("updates/check-at-start"), true);
    settings.sync();
}

namespace {
QString textOf(QQuickWindow* window, const char* name)
{
    auto* item = window->findChild<QObject*>(QString::fromLatin1(name));
    return item ? item->property("text").toString() : QStringLiteral("<absent>");
}

// Montage commun des essais du bandeau : profils temporaires, moteur, fenêtre principale. L'ordre des membres
// fait détruire le moteur avant le dossier des profils.
struct Shell
{
    QTemporaryDir profiles;
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;

    bool load()
    {
        if (!profiles.isValid())
            return false;
        StartupOptions options;
        options.profileRoot = profiles.path();
        window = loadMainWindow(engine, options);
        return window != nullptr;
    }
    QObject* child(const char* name) const { return window->findChild<QObject*>(QString::fromLatin1(name)); }
    QQuickItem* item(const char* name) const { return qobject_cast<QQuickItem*>(child(name)); }
};
// Rectangle d'un item dans le repère de la scène (repère commun à deux items de parents différents).
QRectF sceneRect(const QQuickItem* item)
{
    return item->mapRectToScene(QRectF(0, 0, item->width(), item->height()));
}
} // namespace

// L11-C20 : après la découverte du démarrage, le bandeau annonce la version trouvée.
void TstUiUpdatechecker::updateBannerTextAfterStartupDiscovery()
{
    enableCheckAtStart(fileUrl(data("newer.json")));
    Shell shell;
    QVERIFY(shell.load());
    QQuickWindow* window = shell.window;
    auto* banner = window->findChild<QObject*>(QStringLiteral("updateBanner"));
    QVERIFY(banner);
    QTRY_VERIFY(banner->property("visible").toBool());
    QVERIFY2(textOf(window, "updateBannerText").contains(QStringLiteral("9.9.9")),
             qPrintable(textOf(window, "updateBannerText")));
}

// L11-C20 : « Aide > Rechercher des mises à jour » après la découverte vide l'état du vérificateur ; le
// bandeau garde ce qui a été découvert, que la recherche soit en cours, en échec ou « à jour ».
void TstUiUpdatechecker::updateBannerKeepsDiscoveryDuringManualCheck()
{
    enableCheckAtStart(fileUrl(data("newer.json")));
    Shell shell;
    QVERIFY(shell.load());
    QQuickWindow* window = shell.window;
    auto* checker = window->findChild<UpdateChecker*>(QStringLiteral("appUpdates"));
    auto* commands = window->findChild<actcraft::ui::Commands*>(QStringLiteral("commands"));
    auto* banner = window->findChild<QObject*>(QStringLiteral("updateBanner"));
    QVERIFY(checker && commands && banner);
    QTRY_VERIFY(banner->property("visible").toBool());
    // La page reçue est celle de la version découverte, non vide.
    const QString discovered = banner->property("page").toUrl().toString();
    QVERIFY2(discovered.endsWith(QStringLiteral("/releases/tag/v9.9.9")), qPrintable(discovered));

    // 10.0.0 est sorti après la découverte de 9.9.9.
    const QString newer10 = writeFile(
        QStringLiteral("newer-10.json"),
        R"({"tag_name": "v10.0.0", "html_url": "https://github.com/jejeasterix/actcraft/releases/tag/v10.0.0", "draft": false, "prerelease": false})");
    QVERIFY(!newer10.isEmpty());

    // Checking et Failed : le bandeau garde la version découverte.
    qputenv(kVariable, fileUrl(m_settings.filePath(QStringLiteral("absent.json"))));
    QVERIFY(commands->trigger(QStringLiteral("help.check-updates")));
    QCOMPARE(checker->state(), UpdateChecker::State::Checking);
    QVERIFY(checker->latestVersion().isEmpty());
    QVERIFY2(textOf(window, "updateBannerText").contains(QStringLiteral("9.9.9")),
             qPrintable(textOf(window, "updateBannerText")));
    QVERIFY(banner->property("visible").toBool());
    QTRY_COMPARE(checker->state(), UpdateChecker::State::Failed);
    QVERIFY2(textOf(window, "updateBannerText").contains(QStringLiteral("9.9.9")),
             qPrintable(textOf(window, "updateBannerText")));
    QVERIFY(banner->property("visible").toBool());
    QCOMPARE(banner->property("page").toUrl().toString(), discovered);

    // Available de nouveau : version et page recopiées.
    qputenv(kVariable, fileUrl(newer10));
    QVERIFY(commands->trigger(QStringLiteral("help.check-updates")));
    QTRY_COMPARE(checker->state(), UpdateChecker::State::Available);
    QVERIFY2(textOf(window, "updateBannerText").contains(QStringLiteral("10.0.0")),
             qPrintable(textOf(window, "updateBannerText")));
    QVERIFY(banner->property("page").toUrl().toString().endsWith(QStringLiteral("/releases/tag/v10.0.0")));
    QVERIFY(banner->property("visible").toBool());

    // UpToDate : plus rien à annoncer, le bandeau disparaît.
    qputenv(kVariable, fileUrl(data("prerelease.json")));
    QVERIFY(commands->trigger(QStringLiteral("help.check-updates")));
    QTRY_COMPARE(checker->state(), UpdateChecker::State::UpToDate);
    QTRY_VERIFY(!banner->property("visible").toBool());
}

// L11-C20 : « Fermer » masque le bandeau seulement.
void TstUiUpdatechecker::updateBannerCloseHides()
{
    enableCheckAtStart(fileUrl(data("newer.json")));
    Shell shell;
    QVERIFY(shell.load());
    QQuickWindow* window = shell.window;
    auto* banner = window->findChild<QObject*>(QStringLiteral("updateBanner"));
    auto* close = window->findChild<QObject*>(QStringLiteral("updateBannerClose"));
    auto* view = window->findChild<QObject*>(QStringLiteral("updateBannerView"));
    QVERIFY(banner && close && view);
    QTRY_VERIFY(banner->property("visible").toBool());
    QVERIFY(QMetaObject::invokeMethod(close, "click"));
    QVERIFY(!banner->property("visible").toBool());
}

// L11-C20 : un message (avertissement de lecture, « Ouverture impossible ») ne passe pas sous le bandeau des
// mises à jour : le message commence sous le bas du bandeau, son « Fermer » reste atteignable.
void TstUiUpdatechecker::updateBannerDoesNotCoverMessageBanner()
{
    enableCheckAtStart(fileUrl(data("newer.json")));
    Shell shell;
    QVERIFY(shell.load());
    QQuickWindow* window = shell.window;
    auto* banner = shell.item("updateBanner");
    auto* message = shell.item("messageBanner");
    QVERIFY(banner && message);
    QTRY_VERIFY(banner->isVisible());
    QVERIFY(
        QMetaObject::invokeMethod(message, "show", Q_ARG(QVariant, QStringLiteral("Ouverture impossible"))));
    QTRY_VERIFY(message->isVisible());
    QTRY_VERIFY(banner->height() > 0 && message->height() > 0);
    const QRectF b = sceneRect(banner);
    const QRectF m = sceneRect(message);
    QTRY_VERIFY2(m.top() >= b.bottom() - 0.5, qPrintable(QStringLiteral("bandeau %1..%2, message %3..%4")
                                                             .arg(b.top())
                                                             .arg(b.bottom())
                                                             .arg(m.top())
                                                             .arg(m.bottom())));
    // Le bandeau fermé, le message remonte en haut de la page.
    auto* close = window->findChild<QObject*>(QStringLiteral("updateBannerClose"));
    QVERIFY(close);
    QVERIFY(QMetaObject::invokeMethod(close, "click"));
    QTRY_VERIFY(!banner->isVisible());
    // Même repère (la scène) des deux côtés : message->y() est en repère de la page, b en repère de la scène.
    QTRY_VERIFY2(
        sceneRect(message).top() < b.bottom() - 0.5,
        qPrintable(
            QStringLiteral("message %1, bas du bandeau %2").arg(sceneRect(message).top()).arg(b.bottom())));
}

// L11-C22 : sans bandeau de mise à jour, le message commence sous le bandeau de reprise (Main.qml, ancrage du
// message). Le bandeau de reprise est rendu visible directement : l'ancrage ne dépend que de sa visibilité.
void TstUiUpdatechecker::messageStaysBelowRecoveryBanner()
{
    Shell shell;
    QVERIFY(shell.load());
    auto* recovery = shell.item("recoveryBanner");
    auto* message = shell.item("messageBanner");
    QVERIFY(recovery && message);
    recovery->setVisible(true);
    QVERIFY(
        QMetaObject::invokeMethod(message, "show", Q_ARG(QVariant, QStringLiteral("Ouverture impossible"))));
    QTRY_VERIFY(message->isVisible());
    QTRY_VERIFY(recovery->height() > 0 && message->height() > 0);
    const QRectF r = sceneRect(recovery);
    QTRY_VERIFY2(sceneRect(message).top() >= r.bottom() - 0.5,
                 qPrintable(QStringLiteral("reprise %1..%2, message %3")
                                .arg(r.top())
                                .arg(r.bottom())
                                .arg(sceneRect(message).top())));
}

QTEST_MAIN(TstUiUpdatechecker)
#include "tst_ui_updatechecker.moc"
