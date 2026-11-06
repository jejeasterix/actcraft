#include "actcraft/ui/webpolicy.h"

#include "actcraft/ui/linklauncher.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include <utility>

namespace actcraft::ui {
namespace {

#ifdef Q_OS_WIN
constexpr Qt::CaseSensitivity kPathCase = Qt::CaseInsensitive;
#else
constexpr Qt::CaseSensitivity kPathCase = Qt::CaseSensitive;
#endif

// Même règle que l'ouvreur des liens (linklauncher.cpp, isNetworkPath, dans son espace de noms anonyme) :
// « \\hôte\… », « //hôte/… », « file://hôte/… », « file:////hôte/… ». Aucun accès au disque : sous Windows,
// toucher un chemin réseau envoie l'authentification du compte à l'hôte.
bool isNetworkPath(const QUrl& url)
{
    QString path = url.toLocalFile();
    path.replace(u'\\', u'/');
    return !url.host().isEmpty() || path.startsWith(QStringLiteral("//"));
}

bool isNetworkFolder(QString folder)
{
    folder.replace(u'\\', u'/');
    return folder.startsWith(QStringLiteral("//"));
}

// Chemin réel d'un chemin absolu nettoyé : liens résolus (canonicalFilePath) jusqu'au plus profond parent
// existant, la suite inexistante ajoutée telle quelle (le dossier temporaire de macOS, /var, est un lien vers
// /private/var). Vide : non résolu, donc refusé. Un lien pendant (cible absente) n'est jamais suivi.
QString realPath(const QString& cleaned)
{
    QString existing = cleaned;
    QStringList rest;
    while (!QFileInfo::exists(existing)) {
        const QFileInfo info(existing);
        if (info.isSymLink())
            return {};
        const QString parent = info.path();
        if (parent == existing)
            return {};
        rest.prepend(info.fileName());
        existing = parent;
    }
    const QString canonical = QFileInfo(existing).canonicalFilePath();
    if (canonical.isEmpty())
        return {};
    for (const QString& part : std::as_const(rest))
        if (part.isEmpty() || part == QLatin1String(".") || part == QLatin1String(".."))
            return {};
    return rest.isEmpty() ? canonical : QDir(canonical).filePath(rest.join(u'/'));
}

// Strictement dans le dossier : le dossier lui-même (sa liste) et un voisin au nom prolongé sont dehors.
bool isInside(const QString& path, const QString& folder)
{
    if (path.isEmpty() || folder.isEmpty() || path.compare(folder, kPathCase) == 0)
        return false;
    const QString prefix = folder.endsWith(u'/') ? folder : folder + u'/';
    return path.startsWith(prefix, kPathCase);
}

// Adresse distante de forme acceptable : la règle des sites de l'ouvreur des liens (http ou https, nom
// d'hôte, aucun identifiant), réemployée telle quelle.
bool isAcceptableRemote(const QUrl& url)
{
    return LinkLauncher::refusalFor(url, LinkLauncher::Website).isEmpty();
}

// Origine d'une adresse distante : schéma, hôte (forme ASCII), port effectif.
QString originOf(const QUrl& url)
{
    const int defaultPort = url.scheme() == QLatin1String("https") ? 443 : 80;
    return url.scheme() + QStringLiteral("://") + url.host(QUrl::FullyEncoded) + u':' +
           QString::number(url.port(defaultPort));
}

// « data: » d'une image matricielle seulement (liste fermée ; SVG exclu : il porte des scripts, D8-4).
bool isImageData(const QUrl& url)
{
    const QString path = url.path(QUrl::FullyDecoded);
    const QString mediaType = path.left(path.indexOf(u',')).section(u';', 0, 0).trimmed().toLower();
    static const QStringList kImages =
        QStringLiteral("image/png image/jpeg image/gif image/bmp image/webp").split(QLatin1Char(' '));
    return kImages.contains(mediaType);
}

QString pageKey(const QString& documentKey, int page)
{
    return documentKey + u'#' + QString::number(page);
}

} // namespace

void WebContentPolicy::setResourceFolder(const QString& folder)
{
    m_folder = folder;
}

void WebContentPolicy::setReceived(bool received)
{
    m_received = received;
}

void WebContentPolicy::typedByUser(const QUrl& address)
{
    if (isAcceptableRemote(address))
        m_typed.insert(originOf(address));
}

void WebContentPolicy::unblock(const QString& documentKey, int page)
{
    m_unblocked.insert(pageKey(documentKey, page));
}

WebContentPolicy::Verdict WebContentPolicy::decide(const QUrl& url, const QString& documentKey,
                                                   int page) const
{
    if (!url.isValid())
        return Verdict::Refuse;
    const QString scheme = url.scheme(); // QUrl rend le schéma en minuscules
    if (scheme == QLatin1String("about"))
        return url.path() == QLatin1String("blank") ? Verdict::Load : Verdict::Refuse;
    if (scheme == QLatin1String("data"))
        return isImageData(url) ? Verdict::Load : Verdict::Refuse;
    if (scheme == QLatin1String("file")) {
        // Le chemin réseau est refusé avant tout accès au disque.
        if (isNetworkPath(url) || m_folder.isEmpty() || isNetworkFolder(m_folder))
            return Verdict::Refuse;
        const QString path = QDir::cleanPath(QDir::fromNativeSeparators(url.toLocalFile()));
        const QString folder = QDir::cleanPath(QDir::fromNativeSeparators(m_folder));
        if (!QDir::isAbsolutePath(path) || !QDir::isAbsolutePath(folder))
            return Verdict::Refuse;
        // Un « : » hors de la lettre de lecteur : flux de données d'un fichier sous Windows (« a.png:b »).
        const qsizetype from = path.size() > 1 && path.at(1) == u':' ? 2 : 0;
        if (path.indexOf(u':', from) >= 0)
            return Verdict::Refuse;
        return isInside(realPath(path), QFileInfo(folder).canonicalFilePath()) ? Verdict::Load
                                                                               : Verdict::Refuse;
    }
    if (!isAcceptableRemote(url)) // tout autre schéma, hôte absent, identifiants
        return Verdict::Refuse;
    if (m_typed.contains(originOf(url)))
        return Verdict::Load;
    if (!m_received) // hors document reçu, l'adresse saisie est la seule voie distante
        return Verdict::Refuse;
    return m_unblocked.contains(pageKey(documentKey, page)) ? Verdict::Load : Verdict::Blocked;
}

} // namespace actcraft::ui
