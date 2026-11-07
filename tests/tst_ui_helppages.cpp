#include <actcraft/ui/helpviewer.h>

#include <QFile>
#include <QRegularExpression>
#include <QtTest>

namespace {
// Liste H1 : outils des lots 1 à 5 qui ont un gestionnaire à la tête. Le fichier d'aide porte l'identifiant
// sans le préfixe « tool. » (c'est ce que cherche HelpViewer::openFor).
const QStringList kLots1To5 = {
    QStringLiteral("tool.pen"),           QStringLiteral("tool.select"), QStringLiteral("tool.highlighter"),
    QStringLiteral("tool.eraser"),        QStringLiteral("tool.shape"),  QStringLiteral("tool.text"),
    QStringLiteral("tool.text-vertical"), QStringLiteral("tool.fill"),   QStringLiteral("tool.connector")};

QString pageName(const QString& id)
{
    return id.startsWith(QLatin1String("tool.")) ? id.mid(5) : id;
}
QString page(const QString& id)
{
    QFile f(QStringLiteral(":/actcraft/help/%1.html").arg(pageName(id)));
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}
} // namespace

class TstUiHelpPages : public QObject
{
    Q_OBJECT
private slots:
    void pagesOfLots1To5Exist()
    {
        for (const QString& id : kLots1To5)
            QVERIFY2(!page(id).isEmpty(), qPrintable(id));
    }
    void everyPageHasATitleAndABackLink()
    {
        for (const QString& id : kLots1To5) {
            const QString html = page(id);
            QVERIFY2(QRegularExpression(QStringLiteral("<title>[^<]{3,}</title>")).match(html).hasMatch(),
                     qPrintable(id));
            QVERIFY2(html.contains(QStringLiteral("href=\"index.html\"")), qPrintable(id));
        }
    }
    void internalLinksResolve()
    {
        const QRegularExpression link(QStringLiteral("href=\"([^\"#:]+\\.html)"));
        for (const QString& id : kLots1To5) {
            auto it = link.globalMatch(page(id));
            while (it.hasNext()) {
                const QString target = it.next().captured(1);
                QVERIFY2(QFile::exists(QStringLiteral(":/actcraft/help/") + target),
                         qPrintable(id + " -> " + target));
            }
        }
    }
    void noScriptImageOrRemoteResource()
    {
        for (const QString& id : kLots1To5) {
            const QString html = page(id);
            QVERIFY2(!html.contains(QStringLiteral("<script"), Qt::CaseInsensitive), qPrintable(id));
            QVERIFY2(!html.contains(QStringLiteral("<img"), Qt::CaseInsensitive), qPrintable(id));
            QVERIFY2(!html.contains(QStringLiteral("src=\"http"), Qt::CaseInsensitive), qPrintable(id));
            QVERIFY2(!html.contains(QStringLiteral("href=\"http"), Qt::CaseInsensitive), qPrintable(id));
        }
    }
    void textsAreOurs()
    {
        QStringList forbidden = {QStringLiteral("ActivInspire"), QStringLiteral("Promethean")};
        const QString names = qEnvironmentVariable("ACTCRAFT_REFERENCE_NAMES");
        if (!names.isEmpty()) {
            QFile f(names);
            QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
            for (const QByteArray& line : f.readAll().split('\n'))
                if (!line.trimmed().isEmpty())
                    forbidden << QString::fromUtf8(line.trimmed());
        }
        for (const QString& id : kLots1To5) {
            const QString html = page(id);
            QVERIFY2(!html.isEmpty(), qPrintable(id));
            for (const QString& word : forbidden)
                QVERIFY2(!html.contains(word, Qt::CaseInsensitive), qPrintable(id + " : " + word));
        }
    }
    void f1OpensTheToolPage()
    {
        actcraft::ui::HelpViewer viewer;
        viewer.openFor(QStringLiteral("tool.pen"));
        QVERIFY(viewer.source().toString().endsWith(QStringLiteral("/help/pen.html")));
    }
};

QTEST_MAIN(TstUiHelpPages)
#include "tst_ui_helppages.moc"
