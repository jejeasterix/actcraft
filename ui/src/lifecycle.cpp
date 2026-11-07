// Talon L11 : rempli par L11-T13.
#include <actcraft/ui/lifecycle.h>

namespace actcraft::ui {

LifecycleKeeper::LifecycleKeeper(QObject* parent) : QObject(parent) {}

DocumentController* LifecycleKeeper::document() const noexcept
{
    return m_document;
}

void LifecycleKeeper::setDocument(DocumentController* document)
{
    if (m_document == document) {
        return;
    }
    m_document = document;
    emit documentChanged();
}

PageView* LifecycleKeeper::view() const noexcept
{
    return m_view;
}

void LifecycleKeeper::setView(PageView* view)
{
    if (m_view == view) {
        return;
    }
    m_view = view;
    emit viewChanged();
}

} // namespace actcraft::ui
