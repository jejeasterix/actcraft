#include <actcraft/core/connectors.h>

namespace actcraft::core {

// Talon L5-T1 : rempli par L5-T10.
std::vector<PointF> attachPoints(const PageObject& object)
{
    static_cast<void>(object);
    return {};
}

// Talon L5-T1 : rempli par L5-T10.
std::optional<Attachment> snapConnectorEnd(const Page& page, PointF point)
{
    static_cast<void>(page);
    static_cast<void>(point);
    return std::nullopt;
}

// Talon L5-T1 : rempli par L5-T10.
PageObject makeConnector(PointF a, ObjectId objectA, PointF b, ObjectId objectB, std::uint8_t style)
{
    static_cast<void>(a);
    static_cast<void>(objectA);
    static_cast<void>(b);
    static_cast<void>(objectB);
    static_cast<void>(style);
    return PageObject{Connector{}};
}

// Talon L5-T1 : rempli par L5-T10.
FollowingConnectorsCommand::FollowingConnectorsCommand(std::size_t page, std::unique_ptr<UndoCommand> inner)
    : m_page(page), m_inner(std::move(inner))
{}

// Talon L5-T1 : rempli par L5-T10.
void FollowingConnectorsCommand::redo(Document& document, Changes& changes)
{
    static_cast<void>(document);
    static_cast<void>(changes);
    static_cast<void>(m_page);
    static_cast<void>(m_inner);
    static_cast<void>(m_connectorsBefore);
}

// Talon L5-T1 : rempli par L5-T10.
void FollowingConnectorsCommand::undo(Document& document, Changes& changes)
{
    static_cast<void>(document);
    static_cast<void>(changes);
}

// Talon L5-T1 : rempli par L5-T10.
std::unique_ptr<UndoCommand> withFollowingConnectors(const Page& page, std::size_t pageIndex,
                                                     std::unique_ptr<UndoCommand> inner)
{
    static_cast<void>(page);
    static_cast<void>(pageIndex);
    return inner;
}

} // namespace actcraft::core
