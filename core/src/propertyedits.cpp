#include <actcraft/core/document.h>
#include <actcraft/core/propertyedits.h>

#include <utility>

// Talon creux : compile, ne fait rien (test rouge de L6-T11).
namespace actcraft::core {

bool propertyTouchesTheCache(ObjectProperty) noexcept
{
    return false;
}

bool isDelegated(ObjectProperty) noexcept
{
    return false;
}

SetObjectPropertiesCommand::SetObjectPropertiesCommand(int page, std::vector<ObjectId> objects,
                                                       ObjectProperty property, PropertyValue value)
    : m_page(page), m_objects(std::move(objects)), m_property(property), m_value(std::move(value))
{}

void SetObjectPropertiesCommand::redo(Document&, Changes&) {}

void SetObjectPropertiesCommand::undo(Document&, Changes&) {}

int SetObjectPropertiesCommand::mergeId() const noexcept
{
    return m_page < -1 && m_property == ObjectProperty::Name ? 0 : -1;
}

bool SetObjectPropertiesCommand::mergeWith(const UndoCommand&)
{
    return false;
}

SetPagePropertiesCommand::SetPagePropertiesCommand(int page, PageProperty property, PropertyValue value)
    : m_page(page), m_property(property), m_value(std::move(value))
{}

void SetPagePropertiesCommand::redo(Document&, Changes&) {}

void SetPagePropertiesCommand::undo(Document&, Changes&) {}

int SetPagePropertiesCommand::mergeId() const noexcept
{
    return m_page < -1 && m_property == PageProperty::Name ? 0 : -1;
}

bool SetPagePropertiesCommand::mergeWith(const UndoCommand&)
{
    return false;
}

std::optional<PropertyValue> readProperty(const PageObject&, ObjectProperty)
{
    return std::nullopt;
}

PropertyValue readPageProperty(const Page&, PageProperty)
{
    return {};
}

} // namespace actcraft::core
