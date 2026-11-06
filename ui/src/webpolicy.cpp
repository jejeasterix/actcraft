#include "actcraft/ui/webpolicy.h"

namespace actcraft::ui {

// Étape intermédiaire du TDD (L8-T29) : tout est chargé ; le test doit rougir.
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
    m_typed.insert(address.toString());
}

void WebContentPolicy::unblock(const QString& documentKey, int page)
{
    m_unblocked.insert(documentKey + u'#' + QString::number(page));
}

WebContentPolicy::Verdict WebContentPolicy::decide(const QUrl& url, const QString& documentKey,
                                                   int page) const
{
    Q_UNUSED(url);
    Q_UNUSED(documentKey);
    Q_UNUSED(page);
    return Verdict::Load;
}

} // namespace actcraft::ui
