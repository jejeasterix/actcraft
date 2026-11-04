#pragma once
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <vector>

namespace actcraft::core {

enum class ObjectFlag : std::uint8_t
{
    Locked,   // F-585
    Hidden,   // F-586 (visible = false)
    DragCopy, // F-587
};
bool flagOfAll(const Page& page, const std::vector<ObjectId>& ids, ObjectFlag flag);

// Une étape. Changes : ObjectChanged (zone : emprise) pour Hidden ; aucun pour Locked et DragCopy.
class SetObjectFlagCommand final : public UndoCommand
{
public:
    SetObjectFlagCommand(std::size_t page, std::vector<ObjectId> ids, ObjectFlag flag, bool value);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    std::vector<ObjectId> m_ids;
    ObjectFlag m_flag;
    bool m_value;
    std::vector<bool> m_before;
};

} // namespace actcraft::core
