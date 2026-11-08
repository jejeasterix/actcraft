#include <actcraft/ui/helpviewer.h>

#include <QFile>

#include <utility>

namespace actcraft::ui {

namespace {
const QString kHelpDir = QStringLiteral("qrc:/actcraft/help/");
const QString kIndexName = QStringLiteral("index.html");

QString resourcePath(const QUrl& url)
{
    return QStringLiteral(":") + url.path();
}

bool pageExists(const QUrl& url)
{
    return url.scheme() == QLatin1String("qrc") && QFile::exists(resourcePath(url));
}

// Page d'aide d'une commande d'outil : « tool.x » -> « x » ; les commandes de maths ne gardent que leur nom ;
// les commandes voisines du rideau et du projecteur partagent une page.
QString pageName(QString id)
{
    if (id.startsWith(QLatin1String("tool.")))
        return id.mid(5);
    if (id.startsWith(QLatin1String("math.")))
        return id.mid(5);
    if (id.startsWith(QLatin1String("present.curtain")))
        return QStringLiteral("curtain");
    if (id.startsWith(QLatin1String("present.spotlight")))
        return QStringLiteral("spotlight");
    return id;
}

} // namespace

HelpViewer::HelpViewer(QObject* parent) : QObject(parent) {}

void HelpViewer::openFor(QString toolId)
{
    toolId = pageName(std::move(toolId));
    // Un identifiant d'outil ne contient ni séparateur ni point : pas d'échappement hors du dossier d'aide.
    const bool plain =
        !toolId.isEmpty() && !toolId.contains(QLatin1Char('/')) && !toolId.contains(QLatin1Char('.'));
    const QUrl page(kHelpDir + toolId + QStringLiteral(".html"));
    show(plain && pageExists(page) ? page : QUrl(kHelpDir + kIndexName));
}

void HelpViewer::follow(QUrl link)
{
    if (link.isRelative() && !m_source.isEmpty())
        link = m_source.resolved(link);
    if (pageExists(link) && link.path().startsWith(kHelpDir.mid(4))) {
        show(link);
        return;
    }
    if (link.scheme() == QLatin1String("qrc"))
        return;              // page embarquée absente : la page courante reste
    emit externalLink(link); // le QML l'ouvre par le navigateur du système
}

void HelpViewer::show(const QUrl& url)
{
    QFile file(resourcePath(url));
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QString text = QString::fromUtf8(file.readAll());
    if (url != m_source) {
        m_source = url;
        emit sourceChanged();
    }
    if (text != m_html) {
        m_html = text;
        emit htmlChanged();
    }
}

} // namespace actcraft::ui
