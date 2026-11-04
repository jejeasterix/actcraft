#include <actcraft/core/document.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/recentdocuments.h>

#include <QFileInfo>

namespace actcraft::ui {

RecentDocuments::RecentDocuments(QObject* parent) : QObject(parent) {}

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
    }
    emit preferencesChanged();
    emit changed();
}

void RecentDocuments::reload()
{
    if (!m_preferences) {
        return;
    }
    m_urls = m_preferences->value(QString::fromLatin1(Preferences::kRecent)).toStringList();
    while (m_urls.size() > kMax) {
        m_urls.removeLast();
    }
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
                                {QStringLiteral("when"), when}});
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
    const QString text = url.toString();
    m_urls.removeAll(text);
    m_urls.prepend(text);
    while (m_urls.size() > kMax) {
        m_urls.removeLast();
    }
    persist();
    emit changed();
}

void RecentDocuments::clear()
{
    if (m_urls.isEmpty()) {
        return;
    }
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
