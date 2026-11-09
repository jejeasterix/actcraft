// Téléchargement borné d'une image (cadrage n° 23 à 26), sur un serveur local seulement.
#include "support/testserver.h"

#include <actcraft/ui/webimagefetch.h>

#include <QSignalSpy>
#include <QtTest>

using actcraft::ui::WebContentPolicy;
using actcraft::ui::WebImageFetch;

namespace {

struct Run
{
    WebImageFetch fetch;
    QSignalSpy finished{&fetch, &WebImageFetch::finished};
    QByteArray data() const { return finished.at(0).at(0).toByteArray(); }
    QString error() const { return finished.at(0).at(1).toString(); }
};

} // namespace

class TstUiWebImageFetch : public QObject
{
    Q_OBJECT
private slots:
    void fetchesTheBytes()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/i.png"), "ABC");
        Run run;
        run.fetch.start(server.url(QStringLiteral("/i.png")));
        QTRY_COMPARE(run.finished.count(), 1);
        QCOMPARE(run.error(), QString());
        QCOMPARE(run.data(), QByteArray("ABC"));
    }

    void missingPageFails()
    {
        TestServer server;
        QVERIFY(server.listening());
        Run run;
        run.fetch.start(server.url(QStringLiteral("/absent.png")));
        QTRY_COMPARE(run.finished.count(), 1);
        QVERIFY(run.error().contains(QStringLiteral("404")));
        QVERIFY(run.data().isEmpty());
    }

    void oversizeIsRefused()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/gros"), QByteArray(int(WebImageFetch::kMaxBytes) + 1024, 'x'));
        Run run;
        run.fetch.start(server.url(QStringLiteral("/gros")));
        QTRY_COMPARE_WITH_TIMEOUT(run.finished.count(), 1, 30000);
        QVERIFY2(run.error().contains(QStringLiteral("25 Mo")), qPrintable(run.error()));
    }

    // Review Focus 3 (n° 25) : aucun Content-Length ; la borne tient sur ce qui arrive.
    void oversizeWithoutLengthIsRefused()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.rawResponse(QStringLiteral("/flux"),
                           "HTTP/1.1 200 OK\r\nContent-Type: image/png\r\nConnection: close\r\n\r\n" +
                               QByteArray(int(WebImageFetch::kMaxBytes) + 1024, 'x'));
        Run run;
        run.fetch.start(server.url(QStringLiteral("/flux")));
        QTRY_COMPARE_WITH_TIMEOUT(run.finished.count(), 1, 30000);
        QVERIFY2(run.error().contains(QStringLiteral("25 Mo")), qPrintable(run.error()));
    }

    void redirectIsFollowed()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/fin"), "OK");
        server.redirect(QStringLiteral("/depart"), server.url(QStringLiteral("/fin")));
        Run run;
        run.fetch.start(server.url(QStringLiteral("/depart")));
        QTRY_COMPARE(run.finished.count(), 1);
        QCOMPARE(run.data(), QByteArray("OK"));
    }

    void redirectLoopFails()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.redirect(QStringLiteral("/a"), server.url(QStringLiteral("/a")));
        Run run;
        run.fetch.start(server.url(QStringLiteral("/a")));
        QTRY_COMPARE_WITH_TIMEOUT(run.finished.count(), 1, 10000);
        QVERIFY(!run.error().isEmpty());
    }

    void policyRefusesTheAddress()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/i"), "x");
        Run run;
        run.fetch.start(server.url(QStringLiteral("/i")),
                        [](const QUrl&) { return WebContentPolicy::Verdict::Refuse; });
        QTRY_COMPARE(run.finished.count(), 1);
        QVERIFY(!run.error().isEmpty());
        QCOMPARE(server.requests(), 0);
    }

    void policyRefusesARedirect()
    {
        TestServer server;
        QVERIFY(server.listening());
        server.page(QStringLiteral("/autre"), "x");
        server.redirect(QStringLiteral("/depart"), server.url(QStringLiteral("/autre")));
        Run run;
        run.fetch.start(server.url(QStringLiteral("/depart")), [](const QUrl& url) {
            return url.path() == QLatin1String("/depart") ? WebContentPolicy::Verdict::Load
                                                          : WebContentPolicy::Verdict::Refuse;
        });
        QTRY_COMPARE(run.finished.count(), 1);
        QVERIFY(!run.error().isEmpty());
        QVERIFY(!server.paths().contains(QStringLiteral("/autre")));
    }
};

QTEST_MAIN(TstUiWebImageFetch)
#include "tst_ui_webimagefetch.moc"
