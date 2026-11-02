#include <actcraft/ui/objectediting.h>

namespace actcraft::ui {

// Talon L5-T1 : rempli par L5-T14.
ObjectEditing::ObjectEditing(QObject* parent) : QObject(parent) {}

// Talon L5-T1 : rempli par L5-T14.
DocumentController* ObjectEditing::document() const
{
    return m_document;
}

// Talon L5-T1 : rempli par L5-T14.
void ObjectEditing::setDocument(DocumentController* document)
{
    m_document = document;
}

// Talon L5-T1 : rempli par L5-T14.
ToolHost* ObjectEditing::tools() const
{
    return m_tools;
}

// Talon L5-T1 : rempli par L5-T14.
void ObjectEditing::setTools(ToolHost* tools)
{
    m_tools = tools;
}

// Talon L5-T1 : rempli par L5-T14.
bool ObjectEditing::hasSelection() const
{
    return false;
}

// Talon L5-T1 : rempli par L5-T14.
bool ObjectEditing::canPaste() const
{
    return false;
}

// Talon L5-T1 : rempli par L5-T14.
bool ObjectEditing::selectionLocked() const
{
    return false;
}

// Talon L5-T1 : rempli par L5-T14.
bool ObjectEditing::selectionHidden() const
{
    return false;
}

// Talon L5-T1 : rempli par L5-T14.
bool ObjectEditing::selectionDragCopy() const
{
    return false;
}

// Talon L5-T1 : rempli par L5-T14.
bool ObjectEditing::selectionIsGroup() const
{
    return false;
}

// Talon L5-T1 : rempli par L5-T14.
bool ObjectEditing::selectionIsAnnotation() const
{
    return false;
}

// Talon L5-T1 : rempli par L5-T14.
void ObjectEditing::cut() {}

// Talon L5-T1 : rempli par L5-T14.
void ObjectEditing::copy() {}

// Talon L5-T1 : rempli par L5-T14.
void ObjectEditing::paste() {}

// Talon L5-T1 : rempli par L5-T14.
void ObjectEditing::pasteOnto() {}

// Talon L5-T1 : rempli par L5-T14.
void ObjectEditing::duplicate() {}

// Talon L5-T1 : rempli par L5-T14.
void ObjectEditing::toggleGroup() {}

// Talon L5-T1 : rempli par L5-T14.
void ObjectEditing::pasteAt(QPointF) {}

// Talon L5-T1 : rempli par L5-T14.
void ObjectEditing::arrange(const QString&) {}

// Talon L5-T1 : rempli par L5-T14.
void ObjectEditing::align(const QString&) {}

// Talon L5-T1 : rempli par L5-T14.
void ObjectEditing::transform(const QString&) {}

// Talon L5-T1 : rempli par L5-T14.
void ObjectEditing::toggleFlag(const QString&) {}

} // namespace actcraft::ui
