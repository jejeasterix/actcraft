#include <actcraft/ui/linklauncher.h>

#include <QDesktopServices>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSemaphore>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include <atomic>
#include <memory>
#include <thread>

#ifdef Q_OS_MACOS
#include <sys/xattr.h>
#endif

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

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
    // Dossier temporaire propre à ce processus : les tests lancés en parallèle (ctest -j) créent leurs
    // propres dossiers actcraft-* dans le dossier temporaire commun, que extractionDirs() compterait (reste
    // d'OA92).
    std::unique_ptr<QTemporaryDir> m_tempRoot;
    void fake(LinkLauncher& l)
    {
        m_opened.clear();
        l.setOpener([this](const QUrl& u) {
            m_opened << u;
            return true;
        });
    }
    static void touch(const QString& path)
    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.close();
    }
    static QString tempRoot() { return QStandardPaths::writableLocation(QStandardPaths::TempLocation); }
    static QStringList extractionDirs()
    {
        return QDir(tempRoot()).entryList({QStringLiteral("actcraft-*")}, QDir::Dirs | QDir::NoDotAndDotDot);
    }
    static QString parentDirOf(const QString& shown)
    {
        return QFileInfo(QDir::fromNativeSeparators(shown)).absolutePath();
    }

private slots:
    void initTestCase()
    {
        m_tempRoot = std::make_unique<QTemporaryDir>();
        QVERIFY(m_tempRoot->isValid());
        // TMPDIR hors de Windows, TMP et TEMP sous Windows (GetTempPath2W) : lus par QDir::tempPath à chaque
        // appel.
        for (const char* name : {"TMPDIR", "TMP", "TEMP"})
            qputenv(name, QFile::encodeName(m_tempRoot->path()));
        QCOMPARE(QDir::cleanPath(tempRoot()), QDir::cleanPath(m_tempRoot->path()));
        for (const char* scheme : {"http", "https", "file"})
            QDesktopServices::setUrlHandler(QString::fromLatin1(scheme), &m_catcher, "got");
    }
    void cleanupTestCase()
    {
        for (const char* scheme : {"http", "https", "file"})
            QDesktopServices::unsetUrlHandler(QString::fromLatin1(scheme));
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
        // Identifiants dans l'adresse : « https://ecole.fr@evil.example/ » mène à evil.example.
        for (const char* u : {"ftp://example.org/", "javascript:alert(1)", "file:///C:/x.txt", "mailto:a@b.c",
                              "https://ecole.fr@evil.example/", "http://eleve:secret@example.org/"})
            QVERIFY2(!LinkLauncher::refusalFor(QUrl(QString::fromLatin1(u)), LinkLauncher::Website).isEmpty(),
                     u);
        QVERIFY(LinkLauncher::refusalFor(QUrl(QStringLiteral("http://example.org/")), LinkLauncher::Website)
                    .isEmpty());
    }
    void networkPathRefused()
    {
        // Le motif est celui du chemin réseau, et aucun accès au disque n'a lieu avant ce refus : sous
        // Windows, le moindre test d'existence d'un « \\hôte\… » envoie l'authentification du compte à
        // l'hôte.
        QTemporaryDir dir;
        const QString local = dir.filePath(QStringLiteral("cours.pdf"));
        touch(local);
        QString tail = QDir::fromNativeSeparators(local);
        while (tail.startsWith(u'/'))
            tail.remove(0, 1);
        // « file:////… » : hôte vide, chemin en « // ». Hors de Windows, il désigne un fichier qui existe :
        // seul le contrôle réseau le refuse.
        const QUrl doubleSlash(QStringLiteral("file:////") + tail);
        QVERIFY(doubleSlash.host().isEmpty());
        QVERIFY(doubleSlash.toLocalFile().startsWith(QStringLiteral("//")));
#ifndef Q_OS_WIN
        QVERIFY(QFileInfo::exists(doubleSlash.toLocalFile()));
#endif
        const int before = LinkLauncher::diskQueries();
        for (const QUrl& url : {QUrl::fromLocalFile(QStringLiteral("//serveur/partage/a.pdf")),
                                QUrl::fromLocalFile(QStringLiteral("\\\\serveur\\partage\\a.pdf")),
                                QUrl(QStringLiteral("file://serveur/partage/a.pdf")), doubleSlash}) {
            const QString reason = LinkLauncher::refusalFor(url, LinkLauncher::File);
            QVERIFY2(reason.contains(QStringLiteral("chemin réseau")),
                     qPrintable(url.toString() + u' ' + reason));
        }
        LinkLauncher l;
        fake(l);
        QSignalSpy refused(&l, &LinkLauncher::refused);
        l.request(QStringLiteral("\\\\serveur\\partage\\a.pdf"), LinkLauncher::File);
        QCOMPARE(refused.size(), 1);
        QVERIFY(refused[0][0].toString().contains(QStringLiteral("chemin réseau")));
        QCOMPARE(LinkLauncher::diskQueries(), before); // aucun accès au disque
        // Témoin : un fichier local, lui, se cherche sur le disque.
        QCOMPARE(LinkLauncher::refusalFor(QUrl::fromLocalFile(local), LinkLauncher::File), QString());
        QVERIFY(LinkLauncher::diskQueries() > before);
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
        // Garde le refus : Windows résout « a.exe. », « a.exe  » et « a.hta. . » en « a.exe » et « a.hta »,
        // qui existent. La liste blanche les refuse avec ou sans le retrait des points et espaces finaux
        // (suffixe vide ou « exe  ») : le témoin qui prouve le retrait est trailingDotPdfAccepted.
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
        const QStringList dirsBefore = extractionDirs();
        LinkLauncher l;
        fake(l);
        QSignalSpy refused(&l, &LinkLauncher::refused);
        QSignalSpy ask(&l, &LinkLauncher::confirmationNeeded);
        l.requestStored(QByteArray("<script>"), QStringLiteral("cours.hta"), QStringLiteral("doc1"));
        QCOMPARE(refused.size(), 1);
        QVERIFY(!refused[0][0].toString().isEmpty()); // le refus montre son motif
        QCOMPARE(ask.size(), 0);                      // refusé avant même la confirmation
        QVERIFY(m_opened.isEmpty());
        QCOMPARE(extractionDirs(), dirsBefore); // refusé avant toute écriture : aucun dossier d'extraction
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
        QString extractionDir;
        {
            LinkLauncher l;
            fake(l);
            QSignalSpy ask(&l, &LinkLauncher::confirmationNeeded);
            l.requestStored(QByteArray("%PDF-1.4"), QStringLiteral("..\\..\\a<b>.pdf"),
                            QStringLiteral("doc1"));
            QTRY_COMPARE_WITH_TIMEOUT(ask.size(), 1, 10000); // écriture hors du fil (L8-C8)
            const QString shown = ask[0][0].toString();
            QVERIFY(shown.endsWith(QStringLiteral("ab.pdf")));
            QVERIFY(!shown.contains(QStringLiteral("..")));
            const QString extracted = QDir::fromNativeSeparators(shown);
            QFile f(extracted);
            QVERIFY(f.open(QIODevice::ReadOnly));
            QCOMPARE(f.readAll(), QByteArray("%PDF-1.4"));
            f.close();

            // Dossier temporaire propre au document, au nom imprévisible, fermé aux autres comptes.
            extractionDir = parentDirOf(shown);
            QCOMPARE(QFileInfo(extractionDir).absolutePath(), QDir::cleanPath(tempRoot()));
            const QString dirName = QFileInfo(extractionDir).fileName();
            QVERIFY2(dirName.startsWith(QStringLiteral("actcraft-")), qPrintable(dirName));
            QVERIFY2(!dirName.contains(QStringLiteral("doc1")), qPrintable(dirName)); // clé hachée
#ifndef Q_OS_WIN
            const QFileDevice::Permissions others = QFileDevice::ReadGroup | QFileDevice::WriteGroup |
                                                    QFileDevice::ExeGroup | QFileDevice::ReadOther |
                                                    QFileDevice::WriteOther | QFileDevice::ExeOther;
            QVERIFY(!(QFileInfo(extractionDir).permissions() & others)); // 0700
#endif

            // Marque « venu d'ailleurs » du système sur le fichier extrait.
#ifdef Q_OS_MACOS
            char mark[256] = {};
            const ssize_t size = getxattr(QFile::encodeName(extracted).constData(), "com.apple.quarantine",
                                          mark, sizeof mark - 1, 0, 0);
            QVERIFY2(size > 0, "attribut com.apple.quarantine absent");
            QVERIFY(QByteArray(mark, size).contains(";ActCraft;"));
#elif defined(Q_OS_WIN)
            QFile zone(extracted + QStringLiteral(":Zone.Identifier"));
            QVERIFY(zone.open(QIODevice::ReadOnly));
            const QByteArray zoneText = zone.readAll();
            QVERIFY(zoneText.contains("[ZoneTransfer]"));
            QVERIFY(zoneText.contains("ZoneId=3"));
#endif

            // Même document : même dossier. Clés que l'ancien filtrage confondait (« doc.1 », « doc/1 ») :
            // dossiers distincts.
            l.requestStored(QByteArray("%PDF-1.4"), QStringLiteral("b.pdf"), QStringLiteral("doc1"));
            l.requestStored(QByteArray("%PDF-1.4"), QStringLiteral("b.pdf"), QStringLiteral("doc.1"));
            l.requestStored(QByteArray("%PDF-1.4"), QStringLiteral("b.pdf"), QStringLiteral("doc/1"));
            QTRY_COMPARE_WITH_TIMEOUT(ask.size(), 4, 10000);
            QCOMPARE(parentDirOf(ask[1][0].toString()), extractionDir);
            QVERIFY(parentDirOf(ask[2][0].toString()) != extractionDir);
            QVERIFY(parentDirOf(ask[3][0].toString()) != extractionDir);
            QVERIFY(parentDirOf(ask[3][0].toString()) != parentDirOf(ask[2][0].toString()));

            // Noms de périphérique de Windows, COM0 et LPT0 compris : « _ » en tête.
            l.requestStored(QByteArray("%PDF-1.4"), QStringLiteral("COM0.pdf"), QStringLiteral("doc1"));
            l.requestStored(QByteArray("texte"), QStringLiteral("lpt0.txt"), QStringLiteral("doc1"));
            l.requestStored(QByteArray("texte"), QStringLiteral("nul.txt"), QStringLiteral("doc1"));
            QTRY_COMPARE_WITH_TIMEOUT(ask.size(), 7, 10000);
            QVERIFY2(ask[4][0].toString().endsWith(QStringLiteral("_COM0.pdf")),
                     qPrintable(ask[4][0].toString()));
            QVERIFY2(ask[5][0].toString().endsWith(QStringLiteral("_lpt0.txt")),
                     qPrintable(ask[5][0].toString()));
            QVERIFY2(ask[6][0].toString().endsWith(QStringLiteral("_nul.txt")),
                     qPrintable(ask[6][0].toString()));

            QVERIFY(m_opened.isEmpty());
            l.answer(ask[0][1].toInt(), true);
            QCOMPARE(m_opened, QList<QUrl>{QUrl::fromLocalFile(extracted)});
        }
        QVERIFY(!QFileInfo::exists(extractionDir)); // le dossier part avec le lanceur
    }
    // Le patron écarté écrit le fichier extrait dans l'appel, sur le fil de l'interface. Ici la sonde bloque
    // le fil d'écriture : l'appel rend la main pendant ce blocage, rien n'est encore soumis à la
    // confirmation, puis elle arrive, écrite ailleurs que sur le fil de l'interface.
    void storedFileIsWrittenOffTheUiThread()
    {
        QSemaphore release;
        std::atomic<QThread*> writeThread{nullptr};
        LinkLauncher::setWriteProbe([&release, &writeThread] {
            writeThread = QThread::currentThread();
            release.tryAcquire(1, 2000); // borné : un fil bloqué à tort ne gèle pas la suite
        });
        LinkLauncher l;
        fake(l);
        QSignalSpy ask(&l, &LinkLauncher::confirmationNeeded);
        l.requestStored(QByteArray("%PDF-1.4"), QStringLiteral("cours.pdf"), QStringLiteral("doc1"));
        QCOMPARE(ask.size(), 0);
        QVERIFY(m_opened.isEmpty());
        release.release();
        QTRY_COMPARE_WITH_TIMEOUT(ask.size(), 1, 10000);
        LinkLauncher::setWriteProbe({});
        QVERIFY(writeThread.load() != nullptr);
        QVERIFY(writeThread.load() != QThread::currentThread());
        QFile written(QDir::fromNativeSeparators(ask[0][0].toString()));
        QVERIFY(written.open(QIODevice::ReadOnly)); // complet quand la confirmation arrive
        QCOMPARE(written.readAll(), QByteArray("%PDF-1.4"));
        QVERIFY(m_opened.isEmpty());
    }
    // Lanceur détruit pendant l'écriture : aucune confirmation, aucun refus, aucun fil laissé derrière.
    // L8-C15 : l'écriture est bien en cours (sonde entrée, fil bloqué) quand le lanceur est détruit, et ne
    // se libère que pendant la destruction : le destructeur doit l'attendre (aucun fil laissé derrière) sans
    // que rien ne soit livré à l'objet détruit.
    void launcherGoneDuringTheWriteDeliversNothing()
    {
        QSemaphore release;
        std::atomic<bool> entered{false};
        LinkLauncher::setWriteProbe([&release, &entered] {
            entered = true;
            release.tryAcquire(1, 5000);
        });
        auto l = std::make_unique<LinkLauncher>();
        fake(*l);
        QSignalSpy ask(l.get(), &LinkLauncher::confirmationNeeded);
        QSignalSpy refused(l.get(), &LinkLauncher::refused);
        l->requestStored(QByteArray("%PDF-1.4"), QStringLiteral("cours.pdf"), QStringLiteral("doc1"));
        QTRY_VERIFY_WITH_TIMEOUT(entered.load(), 5000); // l'écriture est en cours, pas finie
        std::thread late([&release] {
            QThread::msleep(300);
            release.release(); // libérée pendant la destruction
        });
        QElapsedTimer timer;
        timer.start();
        l.reset();
        const qint64 waited = timer.elapsed();
        late.join();
        QTest::qWait(200);
        LinkLauncher::setWriteProbe({});
        QVERIFY2(waited >= 200, qPrintable(QString::number(waited))); // le destructeur a attendu le fil
        QCOMPARE(ask.size(), 0);
        QCOMPARE(refused.size(), 0);
        QVERIFY(m_opened.isEmpty());
    }
    // L8-C15 : une confirmation donnée pendant qu'une écriture remplace le même fichier (à la demande
    // suivante) est refusée : le fichier remplacé n'a pas encore sa marque « venu d'ailleurs ». La nouvelle
    // demande se confirme ensuite, une fois le fichier écrit et marqué.
    void answerRefusedWhileTheSameFileIsBeingWritten()
    {
        LinkLauncher l;
        fake(l);
        QSignalSpy ask(&l, &LinkLauncher::confirmationNeeded);
        QSignalSpy refused(&l, &LinkLauncher::refused);
        l.requestStored(QByteArray("premier"), QStringLiteral("cours.pdf"), QStringLiteral("doc1"));
        QTRY_COMPARE_WITH_TIMEOUT(ask.size(), 1, 10000);
        QSemaphore release;
        std::atomic<bool> entered{false};
        LinkLauncher::setWriteProbe([&release, &entered] {
            entered = true;
            release.tryAcquire(1, 5000);
        });
        l.requestStored(QByteArray("second"), QStringLiteral("cours.pdf"), QStringLiteral("doc1"));
        QTRY_VERIFY_WITH_TIMEOUT(entered.load(), 5000);
        l.answer(ask[0][1].toInt(), true); // le fichier est en train d'être remplacé
        QVERIFY(m_opened.isEmpty());
        QCOMPARE(refused.size(), 1);
        release.release();
        QTRY_COMPARE_WITH_TIMEOUT(ask.size(), 2, 10000);
        LinkLauncher::setWriteProbe({});
        l.answer(ask[1][1].toInt(), true);
        QCOMPARE(m_opened.size(), 1);
        QFile written(m_opened[0].toLocalFile());
        QVERIFY(written.open(QIODevice::ReadOnly));
        QCOMPARE(written.readAll(), QByteArray("second"));
    }
    void confirmationShowsTheFullTarget()
    {
        LinkLauncher l;
        fake(l);
        QSignalSpy ask(&l, &LinkLauncher::confirmationNeeded);
        QSignalSpy refused(&l, &LinkLauncher::refused);
        l.request(QStringLiteral("https://example.org/chemin?q=1"), LinkLauncher::Website);
        QCOMPARE(ask.size(), 1);
        QCOMPARE(ask[0][0].toString(), QStringLiteral("https://example.org/chemin?q=1"));

        // Caractères de mise en forme (Cf, dont U+202E qui retourne l'affichage) et de contrôle (Cc) :
        // échappés dans la cible montrée comme dans le motif d'un refus.
        QTemporaryDir dir;
        const QString reversed =
            dir.filePath(QStringLiteral("rapport") + QChar(0x202E) + QStringLiteral("fdp.pdf"));
        touch(reversed);
        l.request(reversed, LinkLauncher::File);
        QCOMPARE(ask.size(), 2);
        const QString shown = ask[1][0].toString();
        QVERIFY2(!shown.contains(QChar(0x202E)), qPrintable(shown));
        QVERIFY2(shown.contains(QStringLiteral("<U+202E>")), qPrintable(shown));
        l.request(dir.filePath(QStringLiteral("a\tb") + QChar(0x200B) + QStringLiteral(".pdf")),
                  LinkLauncher::File);
        QCOMPARE(refused.size(), 1);
        const QString reason = refused[0][0].toString();
        QVERIFY2(!reason.contains(u'\t') && !reason.contains(QChar(0x200B)), qPrintable(reason));
        QVERIFY2(reason.contains(QStringLiteral("<U+0009>")) && reason.contains(QStringLiteral("<U+200B>")),
                 qPrintable(reason));

        // La boîte montre la cible complète ; un refus tardif (fichier retiré entre la demande et « Ouvrir »)
        // laisse son motif à l'écran.
        const QString pdf = dir.filePath(QStringLiteral("cours.pdf"));
        touch(pdf);
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\nWindow {\n"
                          "  width: 800; height: 600\n"
                          "  property QtObject dialogue: confirm\n"
                          "  LinkConfirmDialog { id: confirm }\n"
                          "}\n",
                          QUrl());
        std::unique_ptr<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        QObject* dialog = window->property("dialogue").value<QObject*>();
        QVERIFY(dialog);
        QVERIFY(dialog->setProperty("launcher", QVariant::fromValue(&l)));
        l.request(pdf, LinkLauncher::File);
        QVERIFY(dialog->property("visible").toBool());
        QCOMPARE(dialog->property("shownTarget").toString(), QDir::toNativeSeparators(QDir::cleanPath(pdf)));
        QVERIFY(QFile::remove(pdf));
        QVERIFY(QMetaObject::invokeMethod(dialog, "openTarget"));
        QVERIFY(m_opened.isEmpty());
        QVERIFY(dialog->property("visible").toBool()); // le motif du refus tardif reste visible
        QVERIFY(!dialog->property("asking").toBool());
        QVERIFY2(dialog->property("reason").toString().contains(QStringLiteral("introuvable")),
                 qPrintable(dialog->property("reason").toString()));
        window.reset(); // la boîte part avant le lanceur
    }
    void escapeRefusesWithoutChangingTool()
    {
        LinkLauncher l;
        fake(l);
        QQmlEngine engine;
        QQmlComponent component(&engine);
        // Échap est aussi le raccourci de l'outil de sélection : il passe avant la boîte s'il n'est pas
        // retenu.
        component.setData("import QtQuick\nimport QtQuick.Window\nimport ActCraft.Ui\nWindow {\n"
                          "  width: 800; height: 600; visible: true\n"
                          "  property QtObject dialogue: confirm\n"
                          "  property bool selectChosen: false\n"
                          "  Shortcut { sequence: \"Esc\"; context: Qt.ApplicationShortcut; onActivated: "
                          "selectChosen = true }\n"
                          "  LinkConfirmDialog { id: confirm }\n"
                          "}\n",
                          QUrl());
        std::unique_ptr<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        auto* quick = qobject_cast<QWindow*>(window.get());
        QVERIFY(quick);
        quick->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(quick));
        QObject* dialog = window->property("dialogue").value<QObject*>();
        QVERIFY(dialog->setProperty("launcher", QVariant::fromValue(&l)));
        l.request(QStringLiteral("https://example.org/"), LinkLauncher::Website);
        QTRY_VERIFY(dialog->property("visible").toBool());
        QTest::keyClick(quick, Qt::Key_Escape);
        QTRY_VERIFY(!dialog->property("visible").toBool());
        QVERIFY(!window->property("selectChosen").toBool());
        QVERIFY(m_opened.isEmpty());
        window.reset();
    }
};

QTEST_MAIN(TstUiLinkLauncher)
#include "tst_ui_linklauncher.moc"
