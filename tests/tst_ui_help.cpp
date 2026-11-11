#include <actcraft/ui/helpviewer.h>

#include <QFile>
#include <QSignalSpy>
#include <QtTest>

using actcraft::ui::HelpViewer;

namespace {
const QUrl kIndex(QStringLiteral("qrc:/actcraft/help/index.html"));

QString indexText()
{
    QFile file(QStringLiteral(":/actcraft/help/index.html"));
    return file.open(QIODevice::ReadOnly) ? QString::fromUtf8(file.readAll()) : QString();
}
} // namespace

class TstUiHelp : public QObject
{
    Q_OBJECT
private slots:
    void indexLoads()
    {
        HelpViewer help;
        help.openFor(QString());
        QCOMPARE(help.source(), kIndex);
        QVERIFY(help.html().contains(QStringLiteral("<h1>")));
        QVERIFY(help.html().contains(QStringLiteral("mot de passe de la classe"), Qt::CaseInsensitive));
    }

    void currentToolPageOrIndex()
    {
        HelpViewer help;
        help.openFor(QStringLiteral("tool.inconnu")); // aucune page pour cet outil : l'index
        QCOMPARE(help.source(), kIndex);
        help.openFor(QStringLiteral("inconnu"));
        QCOMPARE(help.source(), kIndex);
        QVERIFY(!help.html().isEmpty());
    }

    void noNetworkUrl()
    {
        HelpViewer help;
        help.openFor(QString());
        QSignalSpy external(&help, &HelpViewer::externalLink);
        QSignalSpy changed(&help, &HelpViewer::sourceChanged);
        help.follow(QUrl(QStringLiteral("https://example.org/page")));
        help.follow(QUrl(QStringLiteral("http://example.org/page")));
        QCOMPARE(help.source(), kIndex);
        QCOMPARE(changed.count(), 0);
        QCOMPARE(external.count(), 2);
        QCOMPARE(external.at(0).at(0).toUrl(), QUrl(QStringLiteral("https://example.org/page")));
    }

    void internalLinkFollows()
    {
        HelpViewer help;
        help.openFor(QString());
        QSignalSpy external(&help, &HelpViewer::externalLink);
        const QUrl anchor(QStringLiteral("qrc:/actcraft/help/index.html#mot-de-passe-oublie"));
        help.follow(anchor);
        QCOMPARE(help.source(), anchor);
        QVERIFY(!help.html().isEmpty());
        // lien relatif à la page courante
        help.follow(QUrl(QStringLiteral("index.html#sommaire")));
        QCOMPARE(help.source(), QUrl(QStringLiteral("qrc:/actcraft/help/index.html#sommaire")));
        // page absente : la page courante reste affichée
        const QUrl before = help.source();
        help.follow(QUrl(QStringLiteral("qrc:/actcraft/help/inexistante.html")));
        QCOMPARE(help.source(), before);
        QCOMPARE(external.count(), 0);
    }

    // L11-C14 : nom de la page montrée, sans dossier ni « .html » (convention de help.contents : « pen »).
    void currentPageName()
    {
        HelpViewer help;
        QVERIFY(help.currentPage().isEmpty());
        QSignalSpy changed(&help, &HelpViewer::sourceChanged);
        help.openFor(QStringLiteral("tool.pen"));
        QCOMPARE(help.currentPage(), QStringLiteral("pen"));
        QCOMPARE(changed.count(), 1);
        help.openFor(QStringLiteral("deux-utilisateurs"));
        QCOMPARE(help.currentPage(), QStringLiteral("deux-utilisateurs"));
        help.openFor(QStringLiteral("inconnue"));
        QCOMPARE(help.currentPage(), QStringLiteral("index"));
    }

    void textsAreOurs()
    {
        const QString text = indexText();
        QVERIFY(!text.isEmpty());
        for (const char* banned : {"ActivInspire", "Promethean", "ExpressPoll"})
            QVERIFY2(!text.contains(QLatin1String(banned), Qt::CaseInsensitive), banned);
    }
};

QTEST_MAIN(TstUiHelp)
#include "tst_ui_help.moc"
