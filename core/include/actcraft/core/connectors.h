#pragma once
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <memory>
#include <optional>
#include <vector>

namespace actcraft::core {

inline constexpr double kConnectorSnap = 20.0; // D11, unités de page

std::vector<PointF> attachPoints(const PageObject& object); // sommets d'une forme, coins de l'emprise sinon
struct Attachment
{
    ObjectId object = 0;
    PointF point;
};
std::optional<Attachment> snapConnectorEnd(const Page& page, PointF point); // connecteurs exclus
PageObject makeConnector(PointF a, ObjectId objectA, PointF b, ObjectId objectB,
                         std::uint8_t style); // couche du dessus

// Enveloppe une commande qui déplace, transforme ou retire : après son redo, chaque connecteur lié suit (même
// indice de point d'attache) ou se détache ; une étape ; undo : connecteurs remis, puis commande annulée.
// Changes : ceux de la commande plus un ObjectChanged par connecteur modifié.
class FollowingConnectorsCommand final : public UndoCommand
{
public:
    FollowingConnectorsCommand(std::size_t page, std::unique_ptr<UndoCommand> inner);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;

private:
    std::size_t m_page;
    std::unique_ptr<UndoCommand> m_inner;
    std::vector<PageObject> m_connectorsBefore;
};
std::unique_ptr<UndoCommand> withFollowingConnectors(const Page& page, std::size_t pageIndex,
                                                     std::unique_ptr<UndoCommand> inner);

} // namespace actcraft::core
