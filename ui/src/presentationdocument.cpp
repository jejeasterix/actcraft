#include "presentationdocument.h"

// Talon L7-S8 : rempli par L7-T2.

namespace actcraft::ui {

DocumentInsertions::DocumentInsertions(DocumentController& controller) : m_controller(controller) {}

core::ObjectId DocumentInsertions::insertObject([[maybe_unused]] core::PageObject object)
{
    return 0;
}

core::ObjectId DocumentInsertions::insertObjectOnNewPage([[maybe_unused]] core::PageObject object)
{
    return 0;
}

core::ObjectId DocumentInsertions::insertText([[maybe_unused]] const QString& plainText,
                                              [[maybe_unused]] const QRectF& bounds)
{
    return 0;
}

core::ObjectId DocumentInsertions::insertImage([[maybe_unused]] QByteArray data,
                                               [[maybe_unused]] std::string extension,
                                               [[maybe_unused]] core::ImageObject image,
                                               [[maybe_unused]] bool onNewPage)
{
    return 0;
}

std::vector<core::ObjectId>
DocumentInsertions::replaceObjects([[maybe_unused]] std::vector<core::ObjectId> removed,
                                   [[maybe_unused]] std::vector<core::PageObject> added)
{
    return {};
}

void DocumentInsertions::setPagePresentation([[maybe_unused]] core::PagePresentation value) {}

const core::Page* DocumentInsertions::shownPage() const noexcept
{
    return nullptr;
}

std::size_t DocumentInsertions::shownIndex() const noexcept
{
    return 0;
}

} // namespace actcraft::ui
