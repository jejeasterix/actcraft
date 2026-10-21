#include <actcraft/ui/pagescontroller.h>

namespace actcraft::ui {

// Talon L4-T1 : rempli par L4-T18.
PagesController::PagesController(QObject* parent) : QObject(parent) {}

DocumentController* PagesController::documentController() const noexcept
{
    return m_controller.data();
}

void PagesController::setDocumentController(DocumentController* controller)
{
    if (m_controller == controller) {
        return;
    }
    m_controller = controller;
    emit wiringChanged();
}

QList<int> PagesController::selection() const
{
    return m_selection;
}

bool PagesController::canPaste() const noexcept
{
    return !m_clipboard.empty();
}

void PagesController::click(int index, int)
{
    m_anchor = index;
}

void PagesController::goTo(int) {}

void PagesController::moveSelection(int) {}

void PagesController::insertBlank(bool) {}

void PagesController::copySelection() {}

void PagesController::cutSelection() {}

void PagesController::paste() {}

void PagesController::duplicateSelection() {}

void PagesController::deleteSelection() {}

} // namespace actcraft::ui
