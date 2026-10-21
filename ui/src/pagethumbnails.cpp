#include <actcraft/ui/pagethumbnails.h>

namespace actcraft::ui {

// Talon L4-T1 : rempli par L4-T17.
PageThumbnails::PageThumbnails(QObject* parent) : QAbstractListModel(parent) {}

PageThumbnails::~PageThumbnails() = default;

int PageThumbnails::rowCount(const QModelIndex&) const
{
    return 0;
}

QVariant PageThumbnails::data(const QModelIndex&, int) const
{
    return {};
}

QHash<int, QByteArray> PageThumbnails::roleNames() const
{
    return {};
}

DocumentController* PageThumbnails::documentController() const noexcept
{
    return m_controller.data();
}

void PageThumbnails::setDocumentController(DocumentController* controller)
{
    if (m_controller == controller) {
        return;
    }
    m_controller = controller;
    emit wiringChanged();
}

PageView* PageThumbnails::view() const noexcept
{
    return m_view.data();
}

void PageThumbnails::setView(PageView* view)
{
    if (m_view == view) {
        return;
    }
    m_view = view;
    emit wiringChanged();
}

int PageThumbnails::thumbnailWidth() const noexcept
{
    return m_width;
}

void PageThumbnails::setThumbnailWidth(int width)
{
    if (m_width == width) {
        return;
    }
    m_width = width;
    emit thumbnailWidthChanged();
}

int PageThumbnails::pendingCount() const noexcept
{
    return m_pending;
}

int PageThumbnails::renderCount() const noexcept
{
    return m_renders;
}

double PageThumbnails::lastCopyMs() const noexcept
{
    return m_lastCopyMs;
}

int PageThumbnails::launchCount() const noexcept
{
    return 0;
}

QImage PageThumbnails::image(int page) const
{
    static_cast<void>(m_revisions);
    return page >= 0 && static_cast<std::size_t>(page) < m_images.size()
               ? m_images[static_cast<std::size_t>(page)]
               : QImage();
}

void PageThumbnails::refreshAll() {}

ThumbnailItem::ThumbnailItem(QQuickItem* parent) : QQuickPaintedItem(parent) {}

PageThumbnails* ThumbnailItem::source() const noexcept
{
    return m_source.data();
}

void ThumbnailItem::setSource(PageThumbnails* source)
{
    if (m_source == source) {
        return;
    }
    m_source = source;
    emit sourceChanged();
}

int ThumbnailItem::page() const noexcept
{
    return m_page;
}

void ThumbnailItem::setPage(int page)
{
    if (m_page == page) {
        return;
    }
    m_page = page;
    emit pageChanged();
}

int ThumbnailItem::revision() const noexcept
{
    return m_revision;
}

void ThumbnailItem::setRevision(int revision)
{
    if (m_revision == revision) {
        return;
    }
    m_revision = revision;
    emit revisionChanged();
}

void ThumbnailItem::paint(QPainter*) {}

} // namespace actcraft::ui
