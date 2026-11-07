#include <actcraft/core/document.h>
#include <actcraft/formats/nativefile.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/recentdocuments.h>

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QThread>
#include <QtConcurrent>

#include <algorithm>

namespace actcraft::ui {

namespace {

struct ThumbnailJob
{
    QString url;
    qint64 stamp = 0;
    QString dataUrl; // vide : pas de vignette
    bool offMainThread = false;
};

// Lit `thumbnails/page-1.png` de l'archive locale ; tout échec donne une vignette vide.
ThumbnailJob readThumbnail(const QString& url, qint64 stamp)
{
    ThumbnailJob job;
    job.url = url;
    job.stamp = stamp;
    job.offMainThread = QThread::currentThread() != QCoreApplication::instance()->thread();
    QFile file(QUrl(url).toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) {
        return job;
    }
    const formats::DecodeResult decoded = formats::decodeActcraft(file.readAll());
    if (decoded.ok && !decoded.thumbnails.empty() && !decoded.thumbnails.front().isEmpty()) {
        job.dataUrl = QStringLiteral("data:image/png;base64,") +
                      QString::fromLatin1(decoded.thumbnails.front().toBase64());
    }
    return job;
}

} // namespace

RecentDocuments::RecentDocuments(QObject* parent)
    : QObject(parent), m_platformPortal(new DocumentPortal(this))
{
    m_pool.setMaxThreadCount(1);
}

RecentDocuments::~RecentDocuments()
{
    m_pool.waitForDone();
}

qint64 RecentDocuments::stampOf(const QUrl& url)
{
    if (!url.isLocalFile()) {
        return 0;
    }
    const QFileInfo info(url.toLocalFile());
    return info.exists() ? (info.lastModified().toMSecsSinceEpoch() ^ (info.size() << 1)) | 1 : 0;
}

void RecentDocuments::setContactActive(bool active)
{
    if (m_contactActive == active) {
        return;
    }
    m_contactActive = active;
    emit contactActiveChanged();
    pumpThumbnails(); // relâché : les demandes gardées partent
}

// Met en file les entrées locales sans vignette à jour ; la lecture part hors contact (pumpThumbnails).
void RecentDocuments::requestThumbnails()
{
    for (const QString& text : std::as_const(m_urls)) {
        const qint64 stamp = stampOf(QUrl(text));
        if (stamp == 0) {
            continue;
        }
        const auto known = m_thumbnails.constFind(text);
        if ((known != m_thumbnails.cend() && known->stamp == stamp) || text == m_thumbnailLoading ||
            std::find(m_thumbnailQueue.begin(), m_thumbnailQueue.end(), text) != m_thumbnailQueue.end()) {
            continue;
        }
        m_thumbnailQueue.push_back(text);
    }
    pumpThumbnails();
}

// Patron de PageThumbnails::pump : une lecture en vol au plus, la suivante part à son retour, et seulement
// hors contact.
void RecentDocuments::pumpThumbnails()
{
    if (m_thumbnailInFlight || m_contactActive || m_thumbnailQueue.empty()) {
        return;
    }
    const QString text = m_thumbnailQueue.front();
    m_thumbnailQueue.pop_front();
    const qint64 stamp = stampOf(QUrl(text));
    if (stamp == 0) {
        pumpThumbnails();
        return;
    }
    m_thumbnailInFlight = true;
    m_thumbnailLoading = text;
    auto* watcher = new QFutureWatcher<ThumbnailJob>(this);
    connect(watcher, &QFutureWatcher<ThumbnailJob>::finished, this, [this, watcher] {
        const ThumbnailJob job = watcher->result();
        watcher->deleteLater();
        m_thumbnailInFlight = false;
        m_thumbnailLoading.clear();
        m_thumbnails.insert(job.url, Thumbnail{job.stamp, job.dataUrl});
        ++m_thumbnailsRead;
        m_lastReadOffMainThread = job.offMainThread;
        emit changed();
        pumpThumbnails();
    });
    watcher->setFuture(QtConcurrent::run(&m_pool, [text, stamp] { return readThumbnail(text, stamp); }));
}

void RecentDocuments::setPreferences(Preferences* preferences)
{
    if (m_preferences == preferences) {
        return;
    }
    if (m_preferences) {
        disconnect(m_preferences, nullptr, this, nullptr);
    }
    m_preferences = preferences;
    if (m_preferences) {
        // Une autre instance (ou la commande file.recent-clear) change la liste : elle est relue.
        connect(m_preferences, &Preferences::changed, this, [this] {
            reload();
            emit changed();
        });
        reload();
        prune(); // lecture de la liste au lancement : les permissions perdues sont retirées
    }
    emit preferencesChanged();
    emit changed();
}

void RecentDocuments::setPortal(DocumentPortal* portal)
{
    if (m_portal == portal) {
        return;
    }
    m_portal = portal;
    emit portalChanged();
}

DocumentPortal& RecentDocuments::activePortal() const
{
    return m_portal ? *m_portal : *m_platformPortal;
}

// Rend la permission des URL content:// que la liste ne montre plus.
void RecentDocuments::forgetDropped(const QStringList& dropped)
{
    for (const QString& text : dropped) {
        activePortal().forget(QUrl(text));
    }
}

// Retire les URL content:// dont la permission est perdue : jamais par un chemin tiré de l'URL, seulement par
// le portail. Pas à chaque changement de réglage (une requête au fournisseur par entrée sous Android) : au
// lancement et sur `refresh`.
void RecentDocuments::prune()
{
    QStringList kept;
    QStringList stale;
    for (const QString& text : std::as_const(m_urls)) {
        const QUrl url(text);
        (DocumentPortal::isContent(url) && !activePortal().canOpen(url) ? stale : kept) << text;
    }
    if (stale.isEmpty()) {
        return;
    }
    m_urls = kept;
    forgetDropped(stale);
    persist();
    for (const QString& text : std::as_const(stale)) {
        const QString name = DocumentPortal::displayName(QUrl(text));
        qInfo("ACTCRAFT RECENT RETIRE %s", qUtf8Printable(name));
        emit staleRemoved(name);
    }
    emit changed();
}

void RecentDocuments::refresh()
{
    reload();
    prune();
    emit changed();
}

void RecentDocuments::reload()
{
    if (!m_preferences) {
        return;
    }
    m_urls = m_preferences->value(QString::fromLatin1(Preferences::kRecent)).toStringList();
    QStringList dropped;
    while (m_urls.size() > kMax) {
        dropped << m_urls.takeLast();
    }
    forgetDropped(dropped);
    requestThumbnails();
}

void RecentDocuments::persist()
{
    if (!m_preferences) {
        return;
    }
    if (m_urls.isEmpty()) {
        m_preferences->remove(QString::fromLatin1(Preferences::kRecent));
    } else {
        m_preferences->setValue(QString::fromLatin1(Preferences::kRecent), m_urls);
    }
}

QVariantList RecentDocuments::entries() const
{
    QVariantList list;
    for (const QString& text : m_urls) {
        const QUrl url(text);
        QVariant when;
        if (url.isLocalFile()) {
            const QFileInfo info(url.toLocalFile());
            if (!info.exists()) {
                continue;
            }
            when = info.lastModified();
        }
        list.append(QVariantMap{{QStringLiteral("url"), url},
                                {QStringLiteral("name"), url.fileName()},
                                {QStringLiteral("when"), when},
                                {QStringLiteral("thumbnail"), m_thumbnails.value(text).dataUrl}});
    }
    return list;
}

int RecentDocuments::count() const
{
    return static_cast<int>(entries().size());
}

void RecentDocuments::note(const QUrl& url)
{
    if (url.isEmpty()) {
        return;
    }
    if (DocumentPortal::isContent(url)) {
        activePortal().keep(url); // rouvert sans sélecteur après la relance (D11-9)
    }
    const QString text = url.toString();
    m_urls.removeAll(text);
    m_urls.prepend(text);
    QStringList dropped;
    while (m_urls.size() > kMax) {
        dropped << m_urls.takeLast();
    }
    forgetDropped(dropped);
    persist();
    emit changed();
    requestThumbnails();
}

void RecentDocuments::clear()
{
    if (m_urls.isEmpty()) {
        return;
    }
    forgetDropped(m_urls);
    m_urls.clear();
    persist();
    emit changed();
}

QVariantMap RecentDocuments::summary(DocumentController* controller, const QUrl& url) const
{
    int objects = 0;
    int resources = 0;
    int pages = 0;
    if (controller) {
        const core::Document& document = controller->document();
        pages = static_cast<int>(document.pageCount());
        for (std::size_t i = 0; i < document.pageCount(); ++i) {
            objects += static_cast<int>(document.page(i).objects.size());
        }
        resources = static_cast<int>(document.resources().size());
    }
    qint64 bytes = 0;
    if (url.isLocalFile()) {
        bytes = QFileInfo(url.toLocalFile()).size();
    }
    return QVariantMap{{QStringLiteral("name"), url.fileName()},
                       {QStringLiteral("pages"), pages},
                       {QStringLiteral("objects"), objects},
                       {QStringLiteral("resources"), resources},
                       {QStringLiteral("bytes"), bytes}};
}

} // namespace actcraft::ui
