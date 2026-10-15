#include <actcraft/core/generator.h>
#include <actcraft/ui/documentcontroller.h>

namespace actcraft::ui {

DocumentController::DocumentController(QObject* parent) : QObject(parent) {}

render::FixedPageItem* DocumentController::fixedPage() const noexcept
{
    return m_fixed.data();
}

void DocumentController::setFixedPage(render::FixedPageItem* page)
{
    if (page == m_fixed) {
        return;
    }
    syncFromView();
    m_fixed = page;
    if (m_fixed) {
        m_fixed->setPage(m_document.page(m_current));
    }
    emit fixedPageChanged();
}

int DocumentController::pageCount() const noexcept
{
    return static_cast<int>(m_document.pageCount());
}

int DocumentController::currentIndex() const noexcept
{
    return static_cast<int>(m_current);
}

QString DocumentController::pageLabel() const
{
    return QStringLiteral("%1 / %2").arg(m_current + 1).arg(m_document.pageCount());
}

const core::Document& DocumentController::document()
{
    syncFromView();
    return m_document;
}

void DocumentController::nextPage()
{
    emit pageAboutToChange(); // le trait en cours rejoint la page affichée avant la recopie
    // Avant l'ajout : la page vierge prend la taille de la dernière, qui peut être celle affichée.
    syncFromView();
    show(core::nextPageIndex(m_document, m_current));
}

void DocumentController::pageBack()
{
    const std::size_t target = core::formerPageIndex(m_current);
    if (target == m_current) {
        return;
    }
    emit pageAboutToChange();
    syncFromView();
    show(target);
}

void DocumentController::loadGeneratedFirstPage()
{
    syncFromView();
    m_document.page(0) = core::generatePage(core::kDefaultSeed);
    if (m_current == 0 && m_fixed) {
        m_fixed->setPage(m_document.page(0));
    }
}

void DocumentController::syncFromView()
{
    if (m_fixed) {
        m_document.page(m_current) = m_fixed->page();
    }
}

// Le document est déjà à jour de la page quittée (syncFromView par l'appelant).
void DocumentController::show(std::size_t index)
{
    m_current = index;
    if (m_fixed) {
        m_fixed->setPage(m_document.page(m_current));
    }
    emit pagesChanged();
}

} // namespace actcraft::ui
