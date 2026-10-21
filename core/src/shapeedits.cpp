#include <actcraft/core/shapeedits.h>

#include <utility>

namespace actcraft::core {

namespace {

// Talon : lit les membres que le contrat réserve (évite -Wunused-private-field).
template <typename... T> void touch(const T&...) noexcept {}

} // namespace

// Talon L4-T1 : rempli par L4-T3.
SetShapeParametersCommand::SetShapeParametersCommand(std::size_t page, ObjectId id,
                                                     std::vector<double> parameters)
    : m_page(page), m_id(id), m_after(std::move(parameters))
{}

void SetShapeParametersCommand::redo(Document&, Changes&)
{
    touch(m_page, m_id, m_after, m_before);
}

void SetShapeParametersCommand::undo(Document&, Changes&) {}

} // namespace actcraft::core
