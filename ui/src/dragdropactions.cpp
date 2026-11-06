#include <actcraft/actions/objectactions.h>
#include <actcraft/core/actionedits.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/ui/dragdropactions.h>

#include <QtDebug>

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace actcraft::ui {

namespace {

const actions::ObjectActionCatalog& catalog()
{
    static const actions::ObjectActionCatalog loaded = [] {
        actions::ObjectActionCatalog c;
        QString error;
        if (!c.load(actions::defaultObjectActionsJson(), error)) {
            qWarning() << "DragDropActions : catalogue des actions illisible :" << error;
        }
        return c;
    }();
    return loaded;
}

const core::Page* currentPage(const DocumentController* controller)
{
    if (controller == nullptr || controller->pageCount() <= 0 || controller->currentIndex() < 0 ||
        static_cast<std::size_t>(controller->currentIndex()) >= controller->document().pageCount()) {
        return nullptr;
    }
    return &controller->document().page(static_cast<std::size_t>(controller->currentIndex()));
}

} // namespace

DragDropActions::DragDropActions(QObject* parent) : QAbstractListModel(parent)
{
    for (const actions::ObjectActionInfo* info : catalog().dragDropList()) {
        m_rows.append(info);
    }
}

DragDropActions::~DragDropActions()
{
    detachDocument();
}

int DragDropActions::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant DragDropActions::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const actions::ObjectActionInfo* info = m_rows[index.row()];
    switch (role) {
    case IdRole:
        return info->id;
    case LabelRole:
        return info->label;
    case IconRole:
        return info->icon;
    default:
        return {};
    }
}

QHash<int, QByteArray> DragDropActions::roleNames() const
{
    return {{IdRole, "id"}, {LabelRole, "label"}, {IconRole, "icon"}};
}

Commands* DragDropActions::commands() const noexcept
{
    return m_commands.data();
}

void DragDropActions::setCommands(Commands* commands)
{
    if (m_commands == commands) {
        return;
    }
    m_commands = commands;
    emit sourcesChanged();
}

DocumentController* DragDropActions::document() const noexcept
{
    return m_document.data();
}

void DragDropActions::setDocument(DocumentController* document)
{
    if (m_document == document) {
        return;
    }
    detachDocument();
    m_document = document;
    attachDocument();
    refresh();
    emit sourcesChanged();
}

QVariantList DragDropActions::overlays() const
{
    return m_overlays;
}

const core::PageObject* DragDropActions::find(core::ObjectId id) const
{
    const core::Page* page = currentPage(m_document.data());
    if (page == nullptr) {
        return nullptr;
    }
    const auto it = std::find_if(page->objects.begin(), page->objects.end(),
                                 [id](const core::PageObject& object) { return object.common.id == id; });
    return it == page->objects.end() ? nullptr : &*it;
}

bool DragDropActions::allowed()
{
    if (!m_commands) {
        emit refused(QStringLiteral("Glisser-déposer sans commandes : rien n'est modifié."));
        return false;
    }
    if (m_commands->registry().isLocked()) {
        emit refused(QStringLiteral("Verrou de classe : les actions ne se modifient pas."));
        return false;
    }
    return m_document != nullptr;
}

bool DragDropActions::dropOn(quint64 objectId, const QString& actionId)
{
    if (!allowed()) {
        return false;
    }
    const actions::ObjectActionInfo* info = catalog().find(actionId);
    if (info == nullptr || !info->dragDrop) {
        return false; // hors de la liste de l'onglet
    }
    const core::PageObject* object = find(static_cast<core::ObjectId>(objectId));
    if (object == nullptr) {
        return false;
    }
    // Les actions au clic restent ; une action de glisser-déposer précédente est remplacée.
    std::vector<core::Action> list;
    for (const core::Action& action : object->common.actions) {
        if (!action.dragDrop) {
            list.push_back(action);
        }
    }
    core::Action dropped; // créée par ActCraft : kind, sourceType et command restent nuls
    dropped.catalogId = info->id.toStdString();
    dropped.dragDrop = true;
    list.push_back(std::move(dropped));
    if (core::sameActions(object->common.actions, list)) {
        return false; // déjà posée : aucune étape vide
    }
    m_document->execute(std::make_unique<core::SetObjectActionsCommand>(
        static_cast<std::size_t>(m_document->currentIndex()), std::vector<core::ObjectId>{object->common.id},
        std::move(list)));
    return true;
}

bool DragDropActions::dropAt(QPointF pagePoint, const QString& actionId, qreal zoom)
{
    const core::Page* page = currentPage(m_document.data());
    if (page == nullptr) {
        return false;
    }
    const std::optional<core::ObjectId> hit =
        core::objectAt(*page, core::PointF{pagePoint.x(), pagePoint.y()},
                       core::kHitToleranceScreen / std::max<qreal>(zoom, 0.01));
    return hit.has_value() && dropOn(static_cast<quint64>(*hit), actionId);
}

bool DragDropActions::dropOnTrash(quint64 objectId)
{
    if (!allowed()) {
        return false;
    }
    const core::PageObject* object = find(static_cast<core::ObjectId>(objectId));
    if (object == nullptr) {
        return false;
    }
    std::vector<core::Action> list;
    for (const core::Action& action : object->common.actions) {
        if (!action.dragDrop) {
            list.push_back(action);
        }
    }
    if (list.size() == object->common.actions.size()) {
        return false; // aucune action de glisser-déposer : aucune étape vide
    }
    m_document->execute(std::make_unique<core::SetObjectActionsCommand>(
        static_cast<std::size_t>(m_document->currentIndex()), std::vector<core::ObjectId>{object->common.id},
        std::move(list)));
    return true;
}

QVariantList DragDropActions::overlayIcons() const
{
    QVariantList icons;
    const core::Page* page = currentPage(m_document.data());
    if (page == nullptr) {
        return icons;
    }
    for (const core::PageObject& object : page->objects) {
        const auto it = std::find_if(object.common.actions.begin(), object.common.actions.end(),
                                     [](const core::Action& action) { return action.dragDrop; });
        if (it == object.common.actions.end()) {
            continue;
        }
        const actions::ObjectActionInfo* info = catalog().find(QString::fromStdString(it->catalogId));
        if (info == nullptr) {
            continue; // action inconnue de cette version : rien n'est dessiné
        }
        const core::RectF b = core::boundsOnPage(*page, object);
        icons.append(QVariantMap{{QStringLiteral("objectId"), static_cast<quint64>(object.common.id)},
                                 {QStringLiteral("actionId"), info->id},
                                 {QStringLiteral("icon"), info->icon},
                                 {QStringLiteral("label"), info->label},
                                 {QStringLiteral("x"), b.x},
                                 {QStringLiteral("y"), b.y},
                                 {QStringLiteral("width"), b.width},
                                 {QStringLiteral("height"), b.height}});
    }
    return icons;
}

void DragDropActions::attachDocument()
{
    if (!m_document) {
        return;
    }
    DocumentController* controller = m_document.data();
    m_observer = controller->addChangeObserver(
        [this](const core::Changes&, DocumentController::ChangeOrigin) { requestRefresh(); });
    m_documentConnections = {
        connect(controller, &DocumentController::documentChanged, this, &DragDropActions::requestRefresh),
        connect(controller, &DocumentController::pagesChanged, this, &DragDropActions::requestRefresh),
    };
}

void DragDropActions::detachDocument()
{
    for (const QMetaObject::Connection& connection : std::as_const(m_documentConnections)) {
        disconnect(connection);
    }
    m_documentConnections.clear();
    if (m_document && m_observer > 0) {
        m_document->removeChangeObserver(m_observer);
    }
    m_observer = 0;
}

// Regroupée : un tour de boucle pour tous les changements d'un geste ; le signal ne part que si la liste change.
void DragDropActions::requestRefresh()
{
    if (m_scheduled) {
        return;
    }
    m_scheduled = true;
    QMetaObject::invokeMethod(
        this,
        [this] {
            m_scheduled = false;
            refresh();
        },
        Qt::QueuedConnection);
}

void DragDropActions::refresh()
{
    QVariantList next = overlayIcons();
    if (next == m_overlays) {
        return;
    }
    m_overlays = std::move(next);
    emit overlaysChanged();
}

} // namespace actcraft::ui
