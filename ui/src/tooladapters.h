#pragma once

#include "penadapters.h"

#include <actcraft/render/inkitem.h>
#include <actcraft/render/liftlayeritem.h>
#include <actcraft/tools/tool.h>
#include <actcraft/tools/toolcontext.h>
#include <actcraft/tools/userstate.h>

#include <QPointer>
#include <QRectF>
#include <QString>
#include <QVariantMap>

#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace actcraft::ui {

class Commands;

// Le cadre de sélection tel que QML le lit (visible, corners, handles, bar, rubberBand, handlesVisible).
// commands peut être nul : la barre n'a alors pas d'icônes.
QVariantMap selectionFrameMap(const tools::SelectionView& view, Commands* commands);
class DocumentController;
class PageView;

// Adaptateurs des interfaces d'outil (DA6, règle de modularité 2) : un outil ne voit que tools::ToolContext ;
// ui/ branche chaque interface sur DocumentController, LiftLayerItem, PageView, Commands et ToolHost. Aucun
// outil ne reçoit un pointeur vers un élément de rendu ni vers la vue.

class DocumentEditorAdapter final : public tools::DocumentEditor
{
public:
    explicit DocumentEditorAdapter(DocumentController& controller);
    const core::Page& page() const override;
    std::size_t pageIndex() const override;
    void execute(std::unique_ptr<core::UndoCommand> command) override;
    std::vector<core::PageObject> lift(const std::vector<core::ObjectId>& ids) override;
    void drop(std::unique_ptr<core::UndoCommand> command) override;
    bool isLifting() const override;

private:
    DocumentController& m_controller;
};

class LiftSurfaceAdapter final : public tools::LiftSurface
{
public:
    explicit LiftSurfaceAdapter(render::LiftLayerItem& layer);
    void show(std::vector<core::PageObject> objects) override;
    void setTransform(const core::Transform& extra) override;
    void clear() override;

private:
    render::LiftLayerItem& m_layer;
};

// Le cadre de la sélection va à ToolHost : selectionFrame (coordonnées de page, contrat du lot 3) et
// handlesVisible.
class SelectionSurfaceAdapter final : public tools::SelectionSurface
{
public:
    using Show = std::function<void(const QVariantMap& frame, bool handlesVisible)>;
    SelectionSurfaceAdapter(Commands* commands, Show show);
    void show(const tools::SelectionView& view) override;

private:
    QPointer<Commands> m_commands;
    Show m_show;
};

// Zoom de la vue ; corbeille : zone de ToolHost en coordonnées de scène, comparée au point de page passé par
// PageView::pageToScene ; barre d'objet : Commands::objectBar.
class ViewInfoAdapter final : public tools::ViewInfo
{
public:
    using TrashArea = std::function<QRectF()>;
    using TrashHover = std::function<void(bool)>;
    ViewInfoAdapter(PageView& view, Commands* commands, TrashArea area, TrashHover hover);
    double zoom() const override;
    bool overTrash(core::PointF pagePoint) const override;
    void setTrashHover(bool hover) override;
    QStringList objectBarCommands() const override;

private:
    PageView& m_view;
    QPointer<Commands> m_commands;
    TrashArea m_area;
    TrashHover m_hover;
};

class CommandSinkAdapter final : public tools::CommandSink
{
public:
    explicit CommandSinkAdapter(Commands* commands);
    bool trigger(const QString& id, const QVariant& argument) override;

private:
    QPointer<Commands> m_commands;
};

// Contexte des outils d'un utilisateur et ses outils (une instance par outil, créée à la première demande).
// Détruit dans l'ordre inverse de la déclaration : les outils d'abord, le contexte, puis les adaptateurs.
struct ToolAdapters
{
    ToolAdapters(DocumentController& controller, PageView& pageView, render::InkItem& inkItem,
                 render::LiftLayerItem& layer, Commands* commandList, tools::UserState& user,
                 SelectionSurfaceAdapter::Show showSelection, ViewInfoAdapter::TrashArea trashArea,
                 ViewInfoAdapter::TrashHover trashHover);
    // Vrai si ces adaptateurs portent exactement ce branchement (sinon ils sont reconstruits).
    bool matches(const DocumentController* controller, const PageView* pageView,
                 const render::InkItem* inkItem, const render::LiftLayerItem* layer,
                 const Commands* commandList) const noexcept;

    QPointer<DocumentController> controllerRef;
    QPointer<PageView> viewRef;
    QPointer<render::InkItem> inkRef;
    QPointer<render::LiftLayerItem> layerRef;
    QPointer<Commands> commandsRef;

    DocumentEditorAdapter document;
    InkSurfaceAdapter ink;
    ControllerSink strokes; // commitAnnotation redéfini : la translucidité du marqueur va au document
    LiftSurfaceAdapter lift;
    SelectionSurfaceAdapter selection;
    ViewInfoAdapter view;
    CommandSinkAdapter commands;
    tools::ToolContext context;
    std::map<QString, std::unique_ptr<tools::Tool>> tools;
};

} // namespace actcraft::ui
