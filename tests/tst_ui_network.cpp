#include "actcraft/ui/networksettings.h"
#include "actcraft/ui/preferences.h"
#include "actcraft/ui/secretstore.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QNetworkProxy>
#include <QOperatingSystemVersion>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

#include <memory>

using actcraft::ui::NetworkSettings;
using actcraft::ui::Preferences;
using actcraft::ui::SecretStore;

namespace {

const QByteArray kPassword = QByteArrayLiteral("s3cret");

QStringList g_messages; // journal du test : tout message émis pendant que le mot de passe circule

void collectMessage(QtMsgType, const QMessageLogContext&, const QString& message)
{
    g_messages.append(message);
}

// Plateforme lue à l'exécution : aucun #ifdef hors des fichiers *_win / *_other (DA10).
bool onWindows()
{
    return QOperatingSystemVersion::currentType() == QOperatingSystemVersion::Windows;
}

QString freshSecretName()
{
    return QStringLiteral("essai-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

} // namespace

class TstUiNetwork : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void secretRoundTrip();
    void secretAbsentIsEmpty();
    void proxyApplied();
    void passwordNeverInSettings();
    void noProxyByDefault();
    void unavailableOffWindows();

private:
    QString iniPath() const { return m_dir->filePath(QStringLiteral("poste.ini")); }
    QString secretName()
    {
        // Nom propre au test, effacé par cleanup() même en échec : le secret du poste n'est jamais touché.
        const QString name = freshSecretName();
        m_secrets.append(name);
        return name;
    }

    std::unique_ptr<QTemporaryDir> m_dir;
    QStringList m_secrets;
};

void TstUiNetwork::initTestCase()
{
    QCoreApplication::setOrganizationName(QStringLiteral("ActCraftEssais"));
    QCoreApplication::setApplicationName(QStringLiteral("tst_ui_network"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
}

void TstUiNetwork::init()
{
    // Chaque test a son dossier : les préférences du test ET tout QSettings par défaut y écrivent.
    m_dir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_dir->isValid());
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir->path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, m_dir->path());
    QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    g_messages.clear();
}

void TstUiNetwork::cleanup()
{
    qInstallMessageHandler(nullptr);
    for (const QString& name : std::as_const(m_secrets))
        SecretStore::remove(name);
    m_secrets.clear();
    QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    m_dir.reset();
}

void TstUiNetwork::secretRoundTrip()
{
    const QString name = secretName();
    // Octet nul et UTF-8 : la longueur est en octets, pas en caractères.
    const QByteArray secret = QByteArrayLiteral("mot\0de passe \xc3\xa9t\xc3\xa9");
    QCOMPARE(SecretStore::write(name, secret), SecretStore::available());
    const std::optional<QByteArray> back = SecretStore::read(name);
    if (SecretStore::available()) {
        QVERIFY(back.has_value());
        QCOMPARE(*back, secret);
        QVERIFY(SecretStore::write(name, QByteArrayLiteral("autre")));
        QCOMPARE(SecretStore::read(name).value_or(QByteArray()), QByteArrayLiteral("autre"));
        QVERIFY(SecretStore::remove(name));
        QVERIFY(!SecretStore::read(name).has_value());
    } else {
        // Hors Windows, le magasin indisponible ne garde rien.
        QVERIFY(!back.has_value());
        QVERIFY(!SecretStore::remove(name));
    }
}

void TstUiNetwork::secretAbsentIsEmpty()
{
    const QString name = secretName();
    QVERIFY(!SecretStore::read(name).has_value());
    QVERIFY(!SecretStore::remove(name));
    QVERIFY(!SecretStore::read(QString()).has_value());
}

void TstUiNetwork::proxyApplied()
{
    const QString secret = secretName();
    Preferences prefs(iniPath(), nullptr);
    {
        NetworkSettings settings(&prefs, secret, nullptr);
        QSignalSpy changed(&settings, &NetworkSettings::changed);
        settings.setHost(QStringLiteral("proxy.test"));
        settings.setPort(3128);
        settings.setUser(QStringLiteral("eleve"));
        QCOMPARE(changed.count(), 3);

        // Chaque changement est appliqué, sans appel explicite à apply().
        const QNetworkProxy proxy = QNetworkProxy::applicationProxy();
        QCOMPARE(proxy.type(), QNetworkProxy::HttpProxy);
        QCOMPARE(proxy.hostName(), QStringLiteral("proxy.test"));
        QCOMPARE(proxy.port(), quint16(3128));
        QCOMPARE(proxy.user(), QStringLiteral("eleve"));
    }
    // Gardés au poste, sous les clés du plan.
    QCOMPARE(prefs.value(QString::fromLatin1(NetworkSettings::kHost)).toString(),
             QStringLiteral("proxy.test"));
    QCOMPARE(prefs.value(QString::fromLatin1(NetworkSettings::kPort)).toInt(), 3128);
    QCOMPARE(prefs.value(QString::fromLatin1(NetworkSettings::kUser)).toString(), QStringLiteral("eleve"));

    // Au démarrage : un nouvel objet sur les mêmes préférences applique le mandataire gardé.
    QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::NoProxy));
    NetworkSettings restarted(&prefs, secret, nullptr);
    QCOMPARE(restarted.host(), QStringLiteral("proxy.test"));
    QCOMPARE(restarted.port(), 3128);
    QCOMPARE(restarted.user(), QStringLiteral("eleve"));
    const QNetworkProxy proxy = QNetworkProxy::applicationProxy();
    QCOMPARE(proxy.type(), QNetworkProxy::HttpProxy);
    QCOMPARE(proxy.hostName(), QStringLiteral("proxy.test"));
    QCOMPARE(proxy.port(), quint16(3128));
}

// Le test qui distingue : le mot de passe ne suit pas le chemin de l'hôte, du port et du compte (QSettings),
// ni quand le trousseau existe, ni quand il manque (hors Windows, où un repli vers QSettings serait la
// tentation).
void TstUiNetwork::passwordNeverInSettings()
{
    const QString secret = secretName();
    qInstallMessageHandler(collectMessage);
    {
        Preferences prefs(iniPath(), nullptr);
        NetworkSettings settings(&prefs, secret, nullptr);
        settings.setHost(QStringLiteral("proxy.test"));
        settings.setPort(3128);
        settings.setUser(QStringLiteral("eleve"));
        settings.setPassword(QString::fromLatin1(kPassword));
        settings.apply();

        // Le magasin fouillé est rempli (hôte, port, compte) : une recherche vide ne prouverait rien.
        const QStringList keys = prefs.allKeys();
        QVERIFY(keys.size() >= 3);
        for (const QString& key : keys) {
            QVERIFY2(!key.contains(QLatin1String(kPassword)), qPrintable(key));
            const QVariant value = prefs.value(key);
            QVERIFY2(!value.toString().contains(QLatin1String(kPassword)), qPrintable(key));
            QVERIFY2(!value.toStringList().join(QLatin1Char(' ')).contains(QLatin1String(kPassword)),
                     qPrintable(key));
        }

        // Appliqué au mandataire seulement depuis le trousseau ; sans trousseau, aucun mot de passe.
        const QString expected = SecretStore::available() ? QString::fromLatin1(kPassword) : QString();
        QCOMPARE(QNetworkProxy::applicationProxy().password(), expected);
    }
    qInstallMessageHandler(nullptr);

    // Aucun fichier de réglages du dossier du test (préférences du test et QSettings par défaut) ne le
    // contient.
    QDirIterator files(m_dir->path(), QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    int count = 0;
    while (files.hasNext()) {
        QFile file(files.next());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY2(!file.readAll().contains(kPassword), qPrintable(file.fileName()));
        ++count;
    }
    QVERIFY(count >= 1);

    // Ni journal ni message.
    for (const QString& message : std::as_const(g_messages))
        QVERIFY2(!message.contains(QLatin1String(kPassword)), qPrintable(message));

    // Le trousseau le garde quand il existe.
    const QByteArray stored = SecretStore::read(secret).value_or(QByteArray());
    QCOMPARE(stored, SecretStore::available() ? kPassword : QByteArray());
}

// « Aucun mandataire par défaut » se lit « aucun mandataire posé par ActCraft » : sans hôte, la main revient
// à la configuration du système (mandataire imposé par GPO ou WPAD), que setApplicationProxy(NoProxy)
// couperait. Le test qui distingue est usesSystemConfiguration() : vrai sous setUseSystemConfiguration(true),
// faux sous NoProxy comme sous un constructeur qui ne ferait rien (le reste posé ci-dessous l'a mis à faux).
// applicationProxy().type() ne distingue pas : sur un poste sans mandataire, le système répond NoProxy aussi.
void TstUiNetwork::noProxyByDefault()
{
    // Reste d'un réglage antérieur : un mandataire applicatif traîne, la configuration du système est coupée.
    QNetworkProxy::setApplicationProxy(
        QNetworkProxy(QNetworkProxy::HttpProxy, QStringLiteral("reste.test"), 8081));
    QVERIFY(!QNetworkProxyFactory::usesSystemConfiguration());
    Preferences prefs(iniPath(), nullptr);
    NetworkSettings settings(&prefs, secretName(), nullptr);
    QVERIFY(settings.host().isEmpty());
    QVERIFY(settings.user().isEmpty());
    QVERIFY(QNetworkProxyFactory::usesSystemConfiguration());
    QVERIFY(QNetworkProxy::applicationProxy().hostName() != QStringLiteral("reste.test"));

    // Avec un hôte : le mandataire saisi, qui prime sur le système.
    settings.setHost(QStringLiteral("proxy.test"));
    settings.setPort(3128);
    settings.setUser(QStringLiteral("eleve"));
    settings.setPassword(QString::fromLatin1(kPassword));
    QCOMPARE(QNetworkProxy::applicationProxy().type(), QNetworkProxy::HttpProxy);
    QCOMPARE(QNetworkProxy::applicationProxy().hostName(), QStringLiteral("proxy.test"));
    QVERIFY(!QNetworkProxyFactory::usesSystemConfiguration());

    // clear() rend la main au système et efface ce qui était gardé.
    settings.clear();
    QVERIFY(QNetworkProxyFactory::usesSystemConfiguration());
    QVERIFY(QNetworkProxy::applicationProxy().hostName() != QStringLiteral("proxy.test"));
    QVERIFY(settings.host().isEmpty());
    QVERIFY(settings.user().isEmpty());
    QVERIFY(!prefs.value(QString::fromLatin1(NetworkSettings::kHost)).isValid());
    QVERIFY(!prefs.value(QString::fromLatin1(NetworkSettings::kUser)).isValid());
    QVERIFY(!SecretStore::read(m_secrets.constLast()).has_value());
}

void TstUiNetwork::unavailableOffWindows()
{
    QCOMPARE(SecretStore::available(), onWindows());
    Preferences prefs(iniPath(), nullptr);
    NetworkSettings settings(&prefs, secretName(), nullptr);
    QCOMPARE(settings.available(), onWindows());
    if (!onWindows()) {
        QVERIFY(!SecretStore::write(m_secrets.constLast(), kPassword));
        QVERIFY(!SecretStore::read(m_secrets.constLast()).has_value());
    }
}

QTEST_MAIN(TstUiNetwork)
#include "tst_ui_network.moc"
