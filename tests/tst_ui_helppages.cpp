#include <actcraft/actions/commandregistry.h>
#include <actcraft/ui/helpviewer.h>

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QtTest>

namespace {
// Liste H1 : outils des lots 1 à 5 qui ont un gestionnaire à la tête. Le fichier d'aide porte l'identifiant
// sans le préfixe « tool. » (c'est ce que cherche HelpViewer::openFor).
const QStringList kLots1To5Tools = {
    QStringLiteral("tool.pen"),           QStringLiteral("tool.select"), QStringLiteral("tool.highlighter"),
    QStringLiteral("tool.eraser"),        QStringLiteral("tool.shape"),  QStringLiteral("tool.text"),
    QStringLiteral("tool.text-vertical"), QStringLiteral("tool.fill"),   QStringLiteral("tool.connector")};

// Liste H2 : outils des lots 6 à 11 qui ont un gestionnaire (les autres commandes tool.* des lots 6 à 11 n'en
// ont pas encore : lot 7 absent de cette branche, caméra, reconnaissance, clavier).
const QStringList kLots6To11Tools = {QStringLiteral("tool.reveal-ink")};
const QStringList kAllTools = kLots1To5Tools + kLots6To11Tools;

// Commandes tool.* déclarées mais sans gestionnaire à la tête : pas de page tant qu'elles n'agissent pas.
// Une commande tool.* nouvelle, absente de cette liste et de kAllTools, fait échouer everyToolHasAPage.
const QStringList kToolsWithoutHandler = {
    QStringLiteral("tool.dice"),           QStringLiteral("tool.calculator"),
    QStringLiteral("tool.clock"),          QStringLiteral("tool.ticker"),
    QStringLiteral("tool.web-browser"),    QStringLiteral("tool.shape-recognition"),
    QStringLiteral("tool.handwriting"),    QStringLiteral("tool.camera-area"),
    QStringLiteral("tool.camera-polygon"), QStringLiteral("tool.camera-freehand"),
    QStringLiteral("tool.camera-window"),  QStringLiteral("tool.camera-screen"),
    QStringLiteral("tool.keyboard")};

const QStringList kProcedures = {QStringLiteral("raccourcis"),         QStringLiteral("deux-utilisateurs"),
                                 QStringLiteral("modes-toucher"),      QStringLiteral("reprise"),
                                 QStringLiteral("mises-a-jour"),       QStringLiteral("profils"),
                                 QStringLiteral("mot-de-passe-oublie")};

QString pageName(const QString& id)
{
    return id.startsWith(QLatin1String("tool.")) ? id.mid(5) : id;
}
// Toutes les pages du paquet d'aide (outils, procédures, index) : nom sans « .html ».
QStringList allPages()
{
    QStringList names;
    for (const QString& file : QDir(QStringLiteral(":/actcraft/help")).entryList({QStringLiteral("*.html")}))
        names << file.chopped(5);
    return names;
}
QString page(const QString& id)
{
    QFile f(QStringLiteral(":/actcraft/help/%1.html").arg(pageName(id)));
    return f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString();
}
} // namespace

// Écart au nom prescrit : la fonction s'appelle pagesOfAllToolsExist (et non pagesOfLots1To5Exist) parce
// qu'elle couvre kAllTools = lots 1 à 5 + lots 6 à 11 (tool.reveal-ink). Les 13 commandes tool.* sans
// gestionnaire à la tête, donc sans page d'aide, sont exclues : tool.dice, tool.calculator, tool.clock,
// tool.ticker, tool.web-browser, tool.shape-recognition, tool.handwriting, tool.camera-area,
// tool.camera-polygon, tool.camera-freehand, tool.camera-window, tool.camera-screen, tool.keyboard
// (kToolsWithoutHandler : une page décrirait un outil qui n'agit pas encore). everyToolHasAPage fait
// échouer toute commande tool.* nouvelle absente des deux listes.
class TstUiHelpPages : public QObject
{
    Q_OBJECT
private slots:
    void pagesOfAllToolsExist()
    {
        for (const QString& id : kAllTools)
            QVERIFY2(!page(id).isEmpty(), qPrintable(id));
    }
    void allPagesAreCollected()
    {
        // Plancher : outils, procédures et index sont tous parcourus par les contrôles qui suivent.
        const QStringList pages = allPages();
        QVERIFY2(pages.size() >= kAllTools.size() + kProcedures.size() + 1, qPrintable(pages.join(',')));
        QVERIFY(pages.contains(QStringLiteral("index")));
        for (const QString& procedure : kProcedures)
            QVERIFY2(pages.contains(procedure), qPrintable(procedure));
    }
    void everyPageHasATitleAndABackLink()
    {
        for (const QString& id : allPages()) {
            const QString html = page(id);
            QVERIFY2(QRegularExpression(QStringLiteral("<title>[^<]{3,}</title>")).match(html).hasMatch(),
                     qPrintable(id));
            QVERIFY2(html.contains(QStringLiteral("href=\"index.html\"")), qPrintable(id));
        }
    }
    void internalLinksResolve()
    {
        const QRegularExpression link(QStringLiteral("href=\"([^\"#:]+\\.html)"));
        for (const QString& id : allPages()) {
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
        for (const QString& id : allPages()) {
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
        for (const QString& id : allPages()) {
            const QString html = page(id);
            QVERIFY2(!html.isEmpty(), qPrintable(id));
            for (const QString& word : forbidden)
                QVERIFY2(!html.contains(word, Qt::CaseInsensitive), qPrintable(id + " : " + word));
        }
    }
    void everyToolHasAPage()
    {
        actcraft::actions::CommandRegistry registry;
        QString error;
        QVERIFY2(registry.loadDeclarations(actcraft::actions::defaultCommandsJson(), error),
                 qPrintable(error));
        int seen = 0;
        for (const QString& id : registry.ids()) {
            if (!id.startsWith(QLatin1String("tool.")) ||
                !registry.isAvailable(id, actcraft::actions::currentPlatform()) ||
                kToolsWithoutHandler.contains(id))
                continue;
            ++seen;
            QVERIFY2(kAllTools.contains(id), qPrintable(id + " : commande sans page d'aide"));
            QVERIFY2(!page(id).isEmpty(), qPrintable(id));
        }
        QVERIFY(seen >= kAllTools.size());
    }
    void procedurePagesExist()
    {
        for (const QString& name : kProcedures)
            QVERIFY2(!page(name).isEmpty(), qPrintable(name));
        // Chaque raccourci de la page des raccourcis est celui d'une commande déclarée.
        actcraft::actions::CommandRegistry registry;
        QString error;
        QVERIFY2(registry.loadDeclarations(actcraft::actions::defaultCommandsJson(), error),
                 qPrintable(error));
        QStringList declared;
        for (const QString& id : registry.ids())
            declared << registry.find(id)->shortcut;
        const QRegularExpression kbd(QStringLiteral("<kbd>([^<]+)</kbd>"));
        auto it = kbd.globalMatch(page(QStringLiteral("raccourcis")));
        int count = 0;
        while (it.hasNext()) {
            QString key = it.next().captured(1);
            key.replace(QStringLiteral("Maj"), QStringLiteral("Shift"))
                .replace(QStringLiteral("Suppr"), QStringLiteral("Del"))
                .replace(QStringLiteral("Échap"), QStringLiteral("Esc"));
            QVERIFY2(declared.contains(key), qPrintable(key));
            ++count;
        }
        QVERIFY(count >= 20);
    }
    void indexLinksEveryPage()
    {
        const QString index = page(QStringLiteral("index"));
        QVERIFY(!index.isEmpty());
        const QStringList files =
            QDir(QStringLiteral(":/actcraft/help")).entryList(QStringList{QStringLiteral("*.html")});
        QVERIFY(files.size() >= 2 + kAllTools.size());
        for (const QString& file : files)
            if (file != QLatin1String("index.html"))
                QVERIFY2(index.contains(QStringLiteral("href=\"%1\"").arg(file)), qPrintable(file));
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
