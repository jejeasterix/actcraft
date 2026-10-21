#include <actcraft/ui/pageoptions.h>

namespace actcraft::ui {

// Talon L4-T1 : rempli par L4-T13 (grille), L4-T20 (fond), L4-T26 (extension), L4-T28 (zones).
PageOptions::PageOptions(QObject* parent) : QObject(parent) {}

DocumentController* PageOptions::documentController() const noexcept
{
    return m_controller.data();
}

void PageOptions::setDocumentController(DocumentController* controller)
{
    if (m_controller == controller) {
        return;
    }
    m_controller = controller;
    emit wiringChanged();
}

bool PageOptions::gridVisible() const
{
    return false;
}

bool PageOptions::gridSnap() const
{
    return false;
}

bool PageOptions::gridAbove() const
{
    return false;
}

QVariantMap PageOptions::grid() const
{
    return {};
}

QVariantMap PageOptions::background() const
{
    return {};
}

qreal PageOptions::pageHeight() const
{
    return 0;
}

QString PageOptions::zones() const
{
    return QStringLiteral("0x0");
}

void PageOptions::toggleGridHidden() {}

void PageOptions::toggleSnap() {}

void PageOptions::beginDesigner()
{
    m_designer = true;
}

void PageOptions::setGrid(const QVariantMap&) {}

void PageOptions::addLevel() {}

void PageOptions::removeLevel(int) {}

void PageOptions::resetGrid() {}

void PageOptions::stepSpacing(int) {}

void PageOptions::endDesigner()
{
    m_designer = false;
}

void PageOptions::setBackground(const QVariantMap&) {}

void PageOptions::extendPage() {}

void PageOptions::setZones(const QString&) {}

} // namespace actcraft::ui
