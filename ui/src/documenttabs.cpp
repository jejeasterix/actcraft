#include <actcraft/ui/documenttabs.h>

namespace actcraft::ui {

// Talon L5-T1 : rempli par L5-T20.
DocumentTabs::DocumentTabs(QObject* parent) : QAbstractListModel(parent) {}

// Talon L5-T1 : rempli par L5-T20.
int DocumentTabs::rowCount(const QModelIndex&) const
{
    return 0;
}

// Talon L5-T1 : rempli par L5-T20.
QVariant DocumentTabs::data(const QModelIndex&, int) const
{
    return {};
}

// Talon L5-T1 : rempli par L5-T20.
QHash<int, QByteArray> DocumentTabs::roleNames() const
{
    return {{TitleRole, "title"}, {ModifiedRole, "modified"}, {CurrentRole, "current"}, {UrlRole, "url"}};
}

// Talon L5-T1 : rempli par L5-T20.
DocumentController* DocumentTabs::document() const
{
    return m_document;
}

// Talon L5-T1 : rempli par L5-T20.
void DocumentTabs::setDocument(DocumentController* document)
{
    m_document = document;
}

// Talon L5-T1 : rempli par L5-T20.
PageView* DocumentTabs::view() const
{
    return m_view;
}

// Talon L5-T1 : rempli par L5-T20.
void DocumentTabs::setView(PageView* view)
{
    m_view = view;
}

// Talon L5-T1 : rempli par L5-T20.
int DocumentTabs::currentIndex() const
{
    return 0;
}

// Talon L5-T1 : rempli par L5-T20.
QString DocumentTabs::position() const
{
    return m_position;
}

// Talon L5-T1 : rempli par L5-T20.
void DocumentTabs::setPosition(const QString& position)
{
    m_position = position;
}

// Talon L5-T1 : rempli par L5-T20.
void DocumentTabs::newTab() {}

// Talon L5-T1 : rempli par L5-T20.
void DocumentTabs::openInNewTab(const QUrl&) {}

// Talon L5-T1 : rempli par L5-T20.
void DocumentTabs::activate(int) {}

// Talon L5-T1 : rempli par L5-T20.
bool DocumentTabs::requestClose(int)
{
    return false;
}

// Talon L5-T1 : rempli par L5-T20.
void DocumentTabs::closeWithoutSaving(int) {}

// Talon L5-T1 : rempli par L5-T20.
void DocumentTabs::copyPagesToTab(const QVariantList&, int) {}

// Talon L5-T1 : rempli par L5-T20.
void DocumentTabs::next() {}

// Talon L5-T1 : rempli par L5-T20.
void DocumentTabs::previous() {}

} // namespace actcraft::ui
