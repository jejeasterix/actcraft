#include <actcraft/ui/webpolicy.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using actcraft::ui::WebContentPolicy;
using V = WebContentPolicy::Verdict;

class TstUiWebPolicy : public QObject
{
    Q_OBJECT
private slots:
    void javascriptOffByDefault() { QVERIFY(!WebContentPolicy::javascriptEnabled()); }
    void fileOutsideResourcesRefused()
    {
        QTemporaryDir dir;
        WebContentPolicy p;
        p.setResourceFolder(dir.path());
        QCOMPARE(p.decide(QUrl::fromLocalFile(QStringLiteral("C:/Windows/win.ini")), QStringLiteral("d"), 0),
                 V::Refuse);
        QCOMPARE(
            p.decide(QUrl::fromLocalFile(dir.filePath(QStringLiteral("../x.png"))), QStringLiteral("d"), 0),
            V::Refuse);
    }
    void fileInsideResourcesAllowed()
    {
        QTemporaryDir dir;
        WebContentPolicy p;
        p.setResourceFolder(dir.path());
        QCOMPARE(p.decide(QUrl::fromLocalFile(dir.filePath(QStringLiteral("a.png"))), QStringLiteral("d"), 0),
                 V::Load);
    }
    void networkPathRefused()
    {
        WebContentPolicy p;
        QCOMPARE(p.decide(QUrl(QStringLiteral("file://serveur/partage/a.png")), QStringLiteral("d"), 0),
                 V::Refuse);
        QCOMPARE(
            p.decide(QUrl::fromLocalFile(QStringLiteral("//serveur/partage/a.png")), QStringLiteral("d"), 0),
            V::Refuse);
    }
    void remoteBlockedInAReceivedDocument()
    {
        WebContentPolicy p;
        p.setReceived(true);
        QCOMPARE(p.decide(QUrl(QStringLiteral("http://127.0.0.1:8765/")), QStringLiteral("d"), 2),
                 V::Blocked);
    }
    void unblockIsPerPageAndDocument()
    {
        WebContentPolicy p;
        p.setReceived(true);
        p.unblock(QStringLiteral("d"), 2);
        QCOMPARE(p.decide(QUrl(QStringLiteral("http://127.0.0.1:8765/")), QStringLiteral("d"), 2), V::Load);
        QCOMPARE(p.decide(QUrl(QStringLiteral("http://127.0.0.1:8765/")), QStringLiteral("d"), 3),
                 V::Blocked);
        QCOMPARE(p.decide(QUrl(QStringLiteral("http://127.0.0.1:8765/")), QStringLiteral("e"), 2),
                 V::Blocked);
    }
    void typedAddressIsNotReceived()
    {
        WebContentPolicy p;
        p.setReceived(true);
        p.typedByUser(QUrl(QStringLiteral("https://example.org/")));
        QCOMPARE(p.decide(QUrl(QStringLiteral("https://example.org/")), QStringLiteral("d"), 0), V::Load);
        QCOMPARE(p.decide(QUrl(QStringLiteral("https://autre.example/")), QStringLiteral("d"), 0),
                 V::Blocked);
    }
    void htmlIsNeverPassedToQTextDocumentWithoutTheGuard()
    {
        // m7 : balaye les fichiers qui manient le HTML d'un cadre (écrits par L8-T30) ; rouge dès qu'un
        // setHtml ou un Text.RichText y apparaît (mutation de L8-T30). ACTCRAFT_SOURCE_DIR : définition de
        // compilation posée par L8-T21.
        for (const char* rel :
             {"/ui/src/webframeitem.cpp", "/ui/qml/WebFrameLayer.qml", "/ui/qml/WebFrameDialog.qml"}) {
            QFile f(QStringLiteral(ACTCRAFT_SOURCE_DIR) + QString::fromLatin1(rel));
            QVERIFY2(f.open(QIODevice::ReadOnly), rel);
            const QString source = QString::fromUtf8(f.readAll());
            QVERIFY2(!source.contains(QStringLiteral("setHtml")), rel);
            QVERIFY2(!source.contains(QStringLiteral("RichText")), rel);
        }
    }

    // Le test qui distingue le bon patron du mauvais : une page saisie ne donne aucun droit à ce qu'elle
    // demande ensuite (redirection, sous-ressource). Chaque requête est jugée sur sa propre adresse. Une
    // règle posée une fois dans le QML de la vue (« ce cadre est autorisé ») chargerait les quatre premières.
    void redirectFromATypedPageIsDecidedAgain()
    {
        QTemporaryDir dir;
        QTemporaryDir outside;
        WebContentPolicy p;
        p.setResourceFolder(dir.path());
        p.typedByUser(QUrl(QStringLiteral("https://example.org/")));
        QCOMPARE(p.decide(QUrl::fromLocalFile(outside.filePath(QStringLiteral("secret.txt"))),
                          QStringLiteral("d"), 0),
                 V::Refuse);
        QCOMPARE(p.decide(QUrl(QStringLiteral("file://serveur/partage/a.png")), QStringLiteral("d"), 0),
                 V::Refuse);
        QCOMPARE(p.decide(QUrl(QStringLiteral("https://autre.example/")), QStringLiteral("d"), 0), V::Refuse);
        QCOMPARE(p.decide(QUrl(QStringLiteral("http://example.org/")), QStringLiteral("d"), 0), V::Refuse);
        // Même origine que l'adresse saisie (schéma, hôte, port) : la navigation dans le site saisi.
        QCOMPARE(
            p.decide(QUrl(QStringLiteral("https://example.org/cours/page2.html")), QStringLiteral("d"), 0),
            V::Load);
    }
    // Hors document reçu, l'adresse saisie est la seule voie distante : rien à débloquer.
    void remoteNotTypedRefusedInASessionDocument()
    {
        WebContentPolicy p;
        QCOMPARE(p.decide(QUrl(QStringLiteral("http://127.0.0.1:8765/")), QStringLiteral("d"), 0), V::Refuse);
        p.unblock(QStringLiteral("d"), 0);
        QCOMPARE(p.decide(QUrl(QStringLiteral("http://127.0.0.1:8765/")), QStringLiteral("d"), 0), V::Refuse);
    }
    void onlyAllowedSchemes()
    {
        WebContentPolicy p;
        p.setReceived(true);
        p.unblock(QStringLiteral("d"), 0);
        p.typedByUser(QUrl(QStringLiteral("https://ecole.example@evil.example/")));
        const auto decide = [&p](const char* url) {
            return p.decide(QUrl(QString::fromLatin1(url)), QStringLiteral("d"), 0);
        };
        QCOMPARE(decide("about:blank"), V::Load);
        QCOMPARE(decide("data:image/png;base64,iVBORw0KGgo="), V::Load);
        QCOMPARE(decide("about:config"), V::Refuse);
        QCOMPARE(decide("data:text/html,<p>x</p>"), V::Refuse);
        QCOMPARE(decide("data:image/svg+xml,<svg/>"), V::Refuse);
        QCOMPARE(decide("javascript:alert(1)"), V::Refuse);
        QCOMPARE(decide("ftp://example.org/a.png"), V::Refuse);
        QCOMPARE(decide("smb://serveur/partage"), V::Refuse);
        QCOMPARE(decide("qrc:/qt/qml/ActCraft/Main.qml"), V::Refuse);
        QCOMPARE(decide("https://ecole.example@evil.example/"),
                 V::Refuse); // identifiants : refusée même saisie
        QCOMPARE(decide("http:///sans-hote"), V::Refuse);
        QCOMPARE(p.decide(QUrl(), QStringLiteral("d"), 0), V::Refuse);
    }
    void noFileUrlWithoutResourceFolder()
    {
        QTemporaryDir dir;
        WebContentPolicy p;
        QCOMPARE(p.decide(QUrl::fromLocalFile(dir.filePath(QStringLiteral("a.png"))), QStringLiteral("d"), 0),
                 V::Refuse);
        p.setResourceFolder(dir.path());
        QCOMPARE(p.decide(QUrl::fromLocalFile(dir.path()), QStringLiteral("d"), 0),
                 V::Refuse); // le dossier même
        QCOMPARE(p.decide(QUrl::fromLocalFile(dir.path() + QStringLiteral("-voisin/a.png")),
                          QStringLiteral("d"), 0),
                 V::Refuse);
    }
    void symlinkLeavingResourcesRefused()
    {
#ifdef Q_OS_WIN
        QVERIFY(true); // QFile::link crée un raccourci .lnk sous Windows, jamais suivi par le moteur
#else
        QTemporaryDir dir;
        QTemporaryDir outside;
        const QString target = outside.filePath(QStringLiteral("secret.png"));
        QFile secret(target);
        QVERIFY(secret.open(QIODevice::WriteOnly));
        secret.close();
        const QString link = dir.filePath(QStringLiteral("lien.png"));
        QVERIFY(QFile::link(target, link));
        WebContentPolicy p;
        p.setResourceFolder(dir.path());
        QCOMPARE(p.decide(QUrl::fromLocalFile(link), QStringLiteral("d"), 0), V::Refuse);
        QFile inside(dir.filePath(QStringLiteral("vrai.png")));
        QVERIFY(inside.open(QIODevice::WriteOnly));
        inside.close();
        QCOMPARE(p.decide(QUrl::fromLocalFile(inside.fileName()), QStringLiteral("d"), 0), V::Load);
#endif
    }
};

QTEST_MAIN(TstUiWebPolicy)
#include "tst_ui_webpolicy.moc"
