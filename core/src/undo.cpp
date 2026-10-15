#include <actcraft/core/document.h>
#include <actcraft/core/undo.h>

#include <algorithm>

namespace actcraft::core {

UndoStack::UndoStack(std::size_t limit) : m_limit(std::max<std::size_t>(1, limit)) {}

Changes UndoStack::push(Document& document, std::unique_ptr<UndoCommand> command)
{
    Changes changes;
    if (!command) {
        return changes;
    }
    command->redo(document, changes);
    if (m_index < m_commands.size()) {
        if (m_clean != kNoClean && m_clean > m_index) {
            m_clean = kNoClean; // l'état enregistré était dans la branche effacée
        }
        m_commands.erase(m_commands.begin() + static_cast<std::ptrdiff_t>(m_index), m_commands.end());
    }
    if (!m_commands.empty()) {
        UndoCommand& top = *m_commands.back();
        if (top.mergeId() >= 0 && top.mergeId() == command->mergeId() && top.mergeWith(*command)) {
            if (m_clean == m_index) {
                m_clean = kNoClean; // la commande enregistrée a changé
            }
            return changes;
        }
    }
    m_commands.push_back(std::move(command));
    ++m_index;
    if (m_commands.size() > m_limit) {
        m_commands.erase(m_commands.begin());
        --m_index;
        if (m_clean != kNoClean) {
            m_clean = m_clean == 0 ? kNoClean : m_clean - 1;
        }
    }
    return changes;
}

Changes UndoStack::undo(Document& document)
{
    Changes changes;
    if (!canUndo()) {
        return changes;
    }
    --m_index;
    m_commands[m_index]->undo(document, changes);
    return changes;
}

Changes UndoStack::redo(Document& document)
{
    Changes changes;
    if (!canRedo()) {
        return changes;
    }
    m_commands[m_index]->redo(document, changes);
    ++m_index;
    return changes;
}

bool UndoStack::canUndo() const noexcept
{
    return m_index > 0;
}

bool UndoStack::canRedo() const noexcept
{
    return m_index < m_commands.size();
}

std::size_t UndoStack::count() const noexcept
{
    return m_commands.size();
}

std::size_t UndoStack::index() const noexcept
{
    return m_index;
}

std::size_t UndoStack::limit() const noexcept
{
    return m_limit;
}

bool UndoStack::isClean() const noexcept
{
    return m_clean == m_index;
}

void UndoStack::setClean() noexcept
{
    m_clean = m_index;
}

void UndoStack::clear() noexcept
{
    m_commands.clear();
    m_index = 0;
    m_clean = 0;
}

} // namespace actcraft::core
