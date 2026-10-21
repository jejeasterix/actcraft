#pragma once

#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>
#include <actcraft/tools/pentool.h>

#include <QString>
#include <QStringList>
#include <QVariant>

#include <memory>
#include <optional>
#include <vector>

namespace actcraft::tools {

class UserState;

// Le document vu par un outil : lecture de la page affichée, écriture par la pile seulement.
class DocumentEditor
{
public:
    virtual ~DocumentEditor() = default;
    virtual const core::Page& page() const = 0; // page affichée, sans les objets soulevés
    virtual std::size_t pageIndex() const = 0;
    virtual void execute(std::unique_ptr<core::UndoCommand> command) = 0;
    // Soulever (DA11) : retire les objets de la page affichée hors pile (le cache en reçoit remove) et rend
    // leurs copies, dans l'ordre de la page. Un second appel pendant le même geste ajoute aux objets
    // soulevés.
    virtual std::vector<core::PageObject> lift(const std::vector<core::ObjectId>& ids) = 0;
    // Remet les objets soulevés puis exécute `command` par la pile (une étape) ; nullptr : remise sans étape.
    virtual void drop(std::unique_ptr<core::UndoCommand> command) = 0;
    virtual bool isLifting() const = 0;
};

// Couche soulevée : dessine les copies au-dessus de la page pendant le geste.
class LiftSurface
{
public:
    virtual ~LiftSurface() = default;
    virtual void
    show(std::vector<core::PageObject> objects) = 0; // remplace ; transformation remise à l'identité
    virtual void setTransform(const core::Transform& extra) = 0; // appliquée après celle de chaque objet
    virtual void clear() = 0;
};

enum class HandleKind : std::uint8_t
{
    TopLeft,
    Top,
    TopRight,
    Right,
    BottomRight, // proportionnelle (F-520)
    Bottom,
    BottomLeft,
    Left,
};

struct BarSpot
{
    QString command; // « object.enlarge »…
    core::PointF center;
};

// Ce que la sélection montre ; l'interface le dessine (coordonnées de page).
struct SelectionView
{
    bool visible = false;
    std::vector<core::PointF> corners; // 4 coins : haut gauche, haut droit, bas droit, bas gauche
    std::vector<core::PointF> handles; // 8 poignées, dans l'ordre de HandleKind
    std::vector<BarSpot> bar;          // barre d'objet (F-521)
    bool handlesVisible = true;        // F-528 : faux pendant un glissement
    std::optional<core::RectF> rubberBand;
    std::vector<core::PointF>
        smartPoints; // F-242 : points intelligents de la forme sélectionnée seule (page)
};

class SelectionSurface
{
public:
    virtual ~SelectionSurface() = default;
    virtual void show(const SelectionView& view) = 0;
};

class ViewInfo
{
public:
    virtual ~ViewInfo() = default;
    virtual double zoom() const = 0;                          // pixels d'écran par pixel de page
    virtual bool overTrash(core::PointF pagePoint) const = 0; // F-050
    virtual void setTrashHover(bool hover) = 0;               // flèche de la corbeille
    virtual QStringList objectBarCommands() const = 0;        // disposition (`objectBar`)
};

class CommandSink
{
public:
    virtual ~CommandSink() = default;
    virtual bool trigger(const QString& id, const QVariant& argument) = 0;
};

struct ToolContext
{
    DocumentEditor& document;
    InkSurface& ink;
    DocumentSink& strokes;
    LiftSurface& lift;
    SelectionSurface& selection;
    ViewInfo& view;
    CommandSink& commands;
    UserState& user;
};

} // namespace actcraft::tools
