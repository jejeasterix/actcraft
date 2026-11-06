#include <actcraft/ui/linklauncher.h>

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

using actcraft::ui::LinkLauncher;

// Reçoit toute adresse http, https ou file que QDesktopServices::openUrl ouvrirait : même si un test
// atteignait l'ouvreur du système, rien ne s'ouvrirait sur le poste qui lance la suite.
class UrlCatcher : public QObject
{
    Q_OBJECT
public:
    QList<QUrl> caught;
public slots:
    void got(const QUrl& url) { caught << url; }
};

class TstUiLinkLauncher : public QObject
{
    Q_OBJECT
    QList<QUrl> m_opened;
    UrlCatcher m_catcher;
    void fake(LinkLauncher& l)
    {
        m_opened.clear();
        l.setOpener([this](const QUrl& u) {
            m_opened << u;
            return true;
        });
    }
    static QString extractionDir()
    {
        return QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
               QStringLiteral("/actcraft-doc1");
    }
    static void touch(const QString& path)
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.close();
    }

private slots:
    void initTestCase()
    {
        for (const char* scheme : {"http", "https", "file"})
            QDesktopServices::setUrlHandler(QString::fromLatin1(scheme), &m_catcher, "got");
        QDir(extractionDir()).removeRecursively();
    }
    void cleanupTestCase()
    {
        for (const char* scheme : {"http", "https", "file"})
            QDesktopServices::unsetUrlHandler(QString::fromLatin1(scheme));
        QDir(extractionDir()).removeRecursively();
        QVERIFY(m_catcher.caught.isEmpty()); // aucun test n'a atteint l'ouvreur du système
    }

    void noOpenWithoutConfirmation()
    {
        LinkLauncher l;
        fake(l);
        QSignalSpy ask(&l, &LinkLauncher::confirmationNeeded);
        l.request(QStringLiteral("https://example.org/"), LinkLauncher::Website);
        QCOMPARE(ask.size(), 1);
        QVERIFY(m_opened.isEmpty());
        l.answer(ask[0][1].toInt(), true);
        QCOMPARE(m_opened.size(), 1);
        QCOMPARE(l.openedCount(), 1);
        l.answer(ask[0][1].toInt(), true); // un jeton ne sert qu'une fois
        QCOMPARE(m_opened.size(), 1);
        QCOMPARE(l.openedCount(), 1);
    }
    void refusalOpensNothing()
    {
        LinkLauncher l;
        fake(l);
        QSignalSpy ask(&l, &LinkLauncher::confirmationNeeded);
        l.request(QStringLiteral("https://example.org/"), LinkLauncher::Website);
        QCOMPARE(ask.size(), 1);
        l.answer(ask[0][1].toInt(), false);
        QVERIFY(m_opened.isEmpty());
        l.answer(ask[0][1].toInt(), true); // le jeton refusé est consommé : un « oui » tardif n'ouvre rien
        QVERIFY(m_opened.isEmpty());
        QCOMPARE(l.openedCount(), 0);
    }
    void onlyHttpAndHttps()
    {
        for (const char* u :
             {"ftp://example.org/", "javascript:alert(1)", "file:///C:/x.txt", "mailto:a@b.c"})
            QVERIFY2(!LinkLauncher::refusalFor(QUrl(QString::fromLatin1(u)), LinkLauncher::Website).isEmpty(),
                     u);
        QVERIFY(LinkLauncher::refusalFor(QUrl(QStringLiteral("http://example.org/")), LinkLauncher::Website)
                    .isEmpty());
    }
    void networkPathRefused()
    {
        QVERIFY(!LinkLauncher::refusalFor(QUrl::fromLocalFile(QStringLiteral("//serveur/partage/a.pdf")),
                                          LinkLauncher::File)
                     .isEmpty());
        QVERIFY(!LinkLauncher::refusalFor(QUrl(QStringLiteral("file://serveur/partage/a.pdf")),
                                          LinkLauncher::File)
                     .isEmpty());
    }
    void executableRefused()
    {
        QTemporaryDir dir;
        for (const char* ext : {"exe", "bat",       "cmd", "com", "msi", "ps1", "vbs",  "js",   "lnk",
                                "scr", "hta",       "wsf", "url", "jar", "msc", "chm",  "cpl",  "reg",
                                "scf", "appref-ms", "doc", "xls", "ppt", "rtf", "docm", "xlsm", "pptm"}) {
            const QString path = dir.filePath(QStringLiteral("a.") + QString::fromLatin1(ext));
            touch(path);
            QVERIFY2(!LinkLauncher::refusalFor(QUrl::fromLocalFile(path), LinkLauncher::File).isEmpty(), ext);
        }
        // Liste blanche, jamais liste noire : un type que personne n'a pensé à interdire est refusé aussi.
        // Une liste de types à refuser (celle des lanceurs, ui/src/launchers.cpp, ou les types énumérés
        // ci-dessus) les accepterait tous.
        for (const char* name : {"a.xyz", "a.html", "a.mht", "a.iso", "a.vhdx", "a.ps1xml", "a.command",
                                 "a.dmg", "a.pkg", "sansextension"}) {
            const QString path = dir.filePath(QString::fromLatin1(name));
            touch(path);
            QVERIFY2(!LinkLauncher::refusalFor(QUrl::fromLocalFile(path), LinkLauncher::File).isEmpty(),
                     name);
        }
    }
    void upperCaseExecutableRefused()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("A.EXE"));
        touch(path);
        QVERIFY(!LinkLauncher::refusalFor(QUrl::fromLocalFile(path), LinkLauncher::File).isEmpty());
    }
    void upperCasePdfAccepted()
    {
        // Témoin accepté : rouge si la comparaison des extensions devient sensible à la casse (mutation
        // demandée).
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("A.PDF"));
        touch(path);
        QCOMPARE(LinkLauncher::refusalFor(QUrl::fromLocalFile(path), LinkLauncher::File), QString());
    }
    void trailingDotRefused()
    {
        // Les fichiers existent : Windows résout « a.exe. » et « a.exe  » en « a.exe ». Seul le retrait des
        // points et espaces finaux avant la comparaison refuse ces noms ; la règle « fichier absent » ne les
        // attrape pas.
        QTemporaryDir dir;
        for (const char* real : {"a.exe", "a.hta"})
            touch(dir.filePath(QString::fromLatin1(real)));
        for (const char* asked : {"a.exe.", "a.exe ", "a.hta. ."}) {
            const QString path = dir.filePath(QString::fromLatin1(asked));
            QVERIFY2(!LinkLauncher::refusalFor(QUrl::fromLocalFile(path), LinkLauncher::File).isEmpty(),
                     asked);
        }
    }
    void trailingDotPdfAccepted()
    {
        // Témoin accepté : « cours.pdf. » désigne « cours.pdf », qui existe ; l'existence se teste sur le nom
        // après retrait des points et espaces finaux.
        QTemporaryDir dir;
        touch(dir.filePath(QStringLiteral("cours.pdf")));
        QCOMPARE(LinkLauncher::refusalFor(QUrl::fromLocalFile(dir.filePath(QStringLiteral("cours.pdf."))),
                                          LinkLauncher::File),
                 QString());
    }
    void svgRefused()
    {
        QTemporaryDir dir;
        for (const char* name : {"dessin.svg", "dessin.SVG"}) {
            const QString path = dir.filePath(QString::fromLatin1(name));
            touch(path);
            QVERIFY2(!LinkLauncher::refusalFor(QUrl::fromLocalFile(path), LinkLauncher::File).isEmpty(),
                     name);
        }
    }
    void htaRefused()
    {
        LinkLauncher l;
        fake(l);
        QSignalSpy refused(&l, &LinkLauncher::refused);
        QSignalSpy ask(&l, &LinkLauncher::confirmationNeeded);
        l.requestStored(QByteArray("<script>"), QStringLiteral("cours.hta"), QStringLiteral("doc1"));
        QCOMPARE(refused.size(), 1);
        QVERIFY(!refused[0][0].toString().isEmpty()); // le refus montre son motif
        QCOMPARE(ask.size(), 0);                      // refusé avant même la confirmation
        QVERIFY(m_opened.isEmpty());
        QVERIFY(!QFile::exists(extractionDir() + QStringLiteral("/cours.hta"))); // refusé avant l'écriture
    }
    void missingFileRefused()
    {
        QVERIFY(!LinkLauncher::refusalFor(QUrl::fromLocalFile(QStringLiteral("C:/nulle-part/a.pdf")),
                                          LinkLauncher::File)
                     .isEmpty());
        // Chemin absolu sur tout système (« C:/… » est relatif hors de Windows) : seule l'absence le refuse.
        QTemporaryDir dir;
        QVERIFY(!LinkLauncher::refusalFor(QUrl::fromLocalFile(dir.filePath(QStringLiteral("absent.pdf"))),
                                          LinkLauncher::File)
                     .isEmpty());
    }
    void storedFileExtractedUnderASafeName()
    {
        LinkLauncher l;
        fake(l);
        QSignalSpy ask(&l, &LinkLauncher::confirmationNeeded);
        l.requestStored(QByteArray("%PDF-1.4"), QStringLiteral("..\\..\\a<b>.pdf"), QStringLiteral("doc1"));
        QCOMPARE(ask.size(), 1);
        const QString shown = ask[0][0].toString();
        QVERIFY(shown.endsWith(QStringLiteral("ab.pdf")));
        QVERIFY(!shown.contains(QStringLiteral("..")));
        const QString extracted = QDir::cleanPath(extractionDir() + QStringLiteral("/ab.pdf"));
        QCOMPARE(QDir::fromNativeSeparators(shown), extracted);
        QFile f(extracted);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), QByteArray("%PDF-1.4"));
        QVERIFY(m_opened.isEmpty());
        l.answer(ask[0][1].toInt(), true);
        QCOMPARE(m_opened, QList<QUrl>{QUrl::fromLocalFile(extracted)});
    }
    void confirmationShowsTheFullTarget()
    {
        LinkLauncher l;
        fake(l);
        QSignalSpy ask(&l, &LinkLauncher::confirmationNeeded);
        l.request(QStringLiteral("https://example.org/chemin?q=1"), LinkLauncher::Website);
        QCOMPARE(ask.size(), 1);
        QCOMPARE(ask[0][0].toString(), QStringLiteral("https://example.org/chemin?q=1"));
    }
};

QTEST_MAIN(TstUiLinkLauncher)
#include "tst_ui_linklauncher.moc"
