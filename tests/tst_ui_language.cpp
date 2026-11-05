#include <actcraft/ui/language.h>
#include <actcraft/ui/preferences.h>

#include <QCoreApplication>
#include <QLocale>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTranslator>
#include <QtTest>

#include <memory>

using namespace actcraft::ui;

// Langue de l'interface (L6-T36) : le réglage `workstation/language` (L6-T5), le choix entre les traductions
// livrées, l'installation du traducteur au démarrage et l'annonce du redémarrage. La langue du système est fixée
// par QLocale::setDefault, jamais lue du poste.
class TstUiLanguage : public QObject
{
    Q_OBJECT
    QTemporaryDir m_dir;
    std::unique_ptr<Preferences> m_prefs;

    static QLocale locale(const char* name) { return QLocale(QString::fromLatin1(name)); }

private slots:
    void initTestCase() { QVERIFY(m_dir.isValid()); }

    void init()
    {
        QFile::remove(m_dir.filePath(QStringLiteral("poste.ini")));
        m_prefs = std::make_unique<Preferences>(m_dir.filePath(QStringLiteral("poste.ini")), nullptr);
    }

    void cleanup() { QLocale::setDefault(QLocale::c()); }

    // Réglage vide : la langue du système, si une traduction la porte.
    void systemLanguageByDefault()
    {
        const QStringList available{QStringLiteral("fr"), QStringLiteral("en")};
        QCOMPARE(Language::resolve(QString(), available, locale("en_GB")), QStringLiteral("en"));
        QCOMPARE(Language::resolve(QString(), available, locale("fr_CA")), QStringLiteral("fr"));
        // Un code explicite l'emporte sur le système.
        QCOMPARE(Language::resolve(QStringLiteral("fr"), available, locale("en_GB")), QStringLiteral("fr"));
        // Les traductions livrées contiennent le français ; le système en français le choisit.
        QVERIFY(Language::available().contains(QStringLiteral("fr")));
        QLocale::setDefault(locale("fr_FR"));
        QCOMPARE(Language::effective(*m_prefs), QStringLiteral("fr"));
    }

    // Une langue sans traduction (système ou réglage) : le français.
    void unknownFallsBackToFrench()
    {
        const QStringList available{QStringLiteral("fr"), QStringLiteral("en")};
        QCOMPARE(Language::resolve(QString(), available, locale("de_DE")), QStringLiteral("fr"));
        QCOMPARE(Language::resolve(QStringLiteral("xx"), available, locale("en_GB")), QStringLiteral("fr"));
        QCOMPARE(Language::resolve(QStringLiteral("en"), {QStringLiteral("fr")}, locale("fr_FR")),
                 QStringLiteral("fr"));
        QLocale::setDefault(locale("de_DE"));
        QCOMPARE(Language::effective(*m_prefs), QStringLiteral("fr"));
        m_prefs->setLanguage(QStringLiteral("klingon"));
        QCOMPARE(Language::effective(*m_prefs), QStringLiteral("fr"));
    }

    // Le choix s'écrit sous `workstation/language`, et nulle part ailleurs.
    void settingWritten()
    {
        Language language;
        language.setPreferences(m_prefs.get());
        language.change(QStringLiteral("fr"));
        QCOMPARE(m_prefs->language(), QStringLiteral("fr"));
        QCOMPARE(m_prefs->value(QString::fromLatin1(Preferences::kLanguage)).toString(), QStringLiteral("fr"));
        QCOMPARE(m_prefs->allKeys(), QStringList{QString::fromLatin1(Preferences::kLanguage)});
        language.change(QString());
        QCOMPARE(m_prefs->language(), QString());
    }

    // Un changement annonce le redémarrage, une fois par valeur nouvelle ; le retour à la langue du départ le retire.
    void restartAnnounced()
    {
        Language language;
        language.setPreferences(m_prefs.get());
        QSignalSpy spy(&language, &Language::restartNeeded);
        QVERIFY(!language.restartPending());
        m_prefs->setLanguage(QStringLiteral("fr")); // écrit hors de Language (réglage de la page)
        QCOMPARE(spy.count(), 1);
        QVERIFY(language.restartPending());
        m_prefs->setLanguage(QStringLiteral("fr")); // même valeur : rien de plus
        QCOMPARE(spy.count(), 1);
        m_prefs->setLanguage(QString());
        QVERIFY(!language.restartPending());
        QCOMPARE(spy.count(), 1);
        language.change(QStringLiteral("fr"));
        QCOMPARE(spy.count(), 2);
    }

    // Au démarrage, le traducteur de la langue choisie est installé sur l'application.
    void translatorInstalledAtStart()
    {
        QLocale::setDefault(locale("fr_FR"));
        QCoreApplication& app = *QCoreApplication::instance();
        const int before = app.findChildren<QTranslator*>(Qt::FindDirectChildrenOnly).size();
        QVERIFY(Language::install(app, *m_prefs));
        const auto translators = app.findChildren<QTranslator*>(Qt::FindDirectChildrenOnly);
        QCOMPARE(translators.size(), before + 1);
        QVERIFY(!translators.constLast()->isEmpty());
        QCOMPARE(translators.constLast()->language(), QStringLiteral("fr"));
        // Réglage inconnu : le français, installé aussi.
        m_prefs->setLanguage(QStringLiteral("xx"));
        QVERIFY(Language::install(app, *m_prefs));
        qDeleteAll(app.findChildren<QTranslator*>(Qt::FindDirectChildrenOnly));
    }
};

QTEST_MAIN(TstUiLanguage)
#include "tst_ui_language.moc"
