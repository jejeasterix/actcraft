#pragma once
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>

#include <memory>
#include <optional>
#include <vector>

namespace actcraft::core {

inline constexpr double kConnectorSnap = 20.0; // D11, unités de page

// Extrémités (F-245, D11) : disque de ce diamètre, flèche de cette longueur et de cette largeur, soit
// max(3 × épaisseur, 8). Le dessin (render/) et l'emprise (boundsOnPage, M8) partent de la même taille.
constexpr double connectorCapSize(double width) noexcept
{
    return 3.0 * width > 8.0 ? 3.0 * width : 8.0;
}

// Panneau des connecteurs (L5-C3, D11) : 14 rangs, chacun (tracé, flèche au départ, flèche à l'arrivée,
// pointillés), d'après notre ancêtre Python (CONNECTOR_TYPES). Le rang porte le tracé et les pointillés ; les
// flèches dessinées restent celles de capA / capB (H5-33) : celles du rang ne règlent l'outil qu'au choix
// d'un style dans le panneau.
enum class ConnectorRoute : std::uint8_t
{
    Straight, // droite a→b
    CurveA,   // quadratique de contrôle (a.x, b.y) : part vertical, arrive horizontal
    CurveB,   // quadratique de contrôle (b.x, a.y) : part horizontal, arrive vertical
    Step,     // horizontal, vertical à mi-largeur, horizontal
    Elbow,    // horizontal puis vertical
    Angle,    // oblique jusqu'à mi-largeur à la hauteur de b, puis horizontal
};
struct ConnectorStyle
{
    ConnectorRoute route = ConnectorRoute::Straight;
    bool arrowStart = false;
    bool arrowEnd = false;
    bool dashed = false;
};
inline constexpr int kConnectorStyleCount = 14;
inline constexpr int kConnectorCurveSegments =
    32;                                           // échantillonnage d'une courbe, segments égaux en paramètre
ConnectorStyle connectorStyle(int rank) noexcept; // hors de 0..13 : le rang 0, la ligne simple

// Géométrie du tracé, seule source du dessin (render/), de la vignette du panneau (ui/) et de l'emprise :
// tous les points restent dans le rectangle a–b, que boundsOnPage englobe déjà (la touche, objectAt, la
// suit).
std::vector<PointF> connectorControlPoints(const Connector& c); // sommets ; [a, contrôle, b] pour une courbe
std::vector<PointF> connectorPath(const Connector& c);          // polyligne, la courbe échantillonnée
struct ConnectorTip
{
    PointF point;   // a ou b
    PointF outward; // direction unitaire de la flèche, hors du tracé : sa tangente au bout ; (0, 0) si a == b
    double reach =
        0; // longueur du segment de contrôle au bout : une flèche plus longue sortirait du rectangle
};
struct ConnectorTips
{
    ConnectorTip a;
    ConnectorTip b;
};
ConnectorTips connectorTips(const Connector& c); // premier segment de contrôle non nul depuis chaque bout

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
// Un bout ne suit que si son objet a bougé ou disparu, ou si le connecteur a bougé ; une commande qui insère,
// retire ou remplace une page n'est pas suivie. Changes : ceux de la commande plus un ObjectChanged par
// connecteur modifié. La fusion (réglettes, lot 3) est celle de la commande enveloppée.
class FollowingConnectorsCommand final : public UndoCommand
{
public:
    FollowingConnectorsCommand(std::size_t page, std::unique_ptr<UndoCommand> inner);
    void redo(Document& document, Changes& changes) override;
    void undo(Document& document, Changes& changes) override;
    int mergeId() const noexcept override;
    bool mergeWith(const UndoCommand& other) override;

private:
    std::size_t m_page;
    std::unique_ptr<UndoCommand> m_inner;
    std::vector<PageObject> m_connectorsBefore;
};
std::unique_ptr<UndoCommand> withFollowingConnectors(const Page& page, std::size_t pageIndex,
                                                     std::unique_ptr<UndoCommand> inner);

} // namespace actcraft::core
