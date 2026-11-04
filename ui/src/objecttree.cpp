#include <actcraft/actions/commandregistry.h>
#include <actcraft/core/arrange.h>
#include <actcraft/core/objectflags.h>
#include <actcraft/tools/userstate.h>
#include <actcraft/ui/objecttree.h>

#include <QCoreApplication>

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace actcraft::ui {

namespace {

// Ordre d'affichage : du dessus vers le fond.
constexpr std::array<core::Layer, 4> kShownLayers{core::Layer::Top, core::Layer::Middle, core::Layer::Bottom,
                                                  core::Layer::Background};

QString layerKey(core::Layer layer)
{
    switch (layer) {
    case core::Layer::Top:
        return QStringLiteral("top");
    case core::Layer::Middle:
        return QStringLiteral("middle");
    case core::Layer::Bottom:
        return QStringLiteral("bottom");
    case core::Layer::Background:
        break;
    }
    return QStringLiteral("background");
}

QString kindKey(core::ObjectKind kind)
{
    switch (kind) {
    case core::ObjectKind::Shape:
        return QStringLiteral("shape");
    case core::ObjectKind::Image:
        return QStringLiteral("image");
    case core::ObjectKind::Stroke:
        return QStringLiteral("stroke");
    case core::ObjectKind::Line:
        return QStringLiteral("line");
    case core::ObjectKind::Text:
        return QStringLiteral("text");
    case core::ObjectKind::Media:
        return QStringLiteral("media");
    case core::ObjectKind::Group:
        return QStringLiteral("group");
    case core::ObjectKind::Connector:
        return QStringLiteral("connector");
    case core::ObjectKind::Equation:
        return QStringLiteral("equation");
    case core::ObjectKind::Unknown:
        break;
    }
    return QStringLiteral("unknown");
}

// Nom affiché d'un objet sans nom : le genre, dans la langue de l'interface.
QString kindLabel(core::ObjectKind kind)
{
    const char* text = "Objet";
    switch (kind) {
    case core::ObjectKind::Shape:
        text = "Forme";
        break;
    case core::ObjectKind::Image:
        text = "Image";
        break;
    case core::ObjectKind::Stroke:
        text = "Trait";
        break;
    case core::ObjectKind::Line:
        text = "Ligne";
        break;
    case core::ObjectKind::Text:
        text = "Texte";
        break;
    case core::ObjectKind::Media:
        text = "Média";
        break;
    case core::ObjectKind::Group:
        text = "Groupe";
        break;
    case core::ObjectKind::Connector:
        text = "Connecteur";
        break;
    case core::ObjectKind::Equation:
        text = "Équation";
        break;
    case core::ObjectKind::Unknown:
        break;
    }
    return QCoreApplication::translate("actcraft::ui::ObjectTreeModel", text);
}

std::unordered_set<core::ObjectId> groupIds(const core::Page& page)
{
    std::unordered_set<core::ObjectId> groups;
    for (const core::PageObject& object : page.objects) {
        if (std::get_if<core::Group>(&object) != nullptr) {
            groups.insert(object.common.id);
        }
    }
    return groups;
}

// Objets dessinés d'une ligne : l'objet, ou les membres de son groupe, sous-groupes dépliés. Le dessin, la
// sélection et le déplacement lisent chaque membre, jamais l'objet Group (règle D9).
std::vector<core::ObjectId> drawnIds(const core::Page& page, core::ObjectId id,
                                     const std::unordered_set<core::ObjectId>& groups)
{
    if (groups.count(id) == 0) {
        return {id};
    }
    std::vector<core::ObjectId> ids = core::expandGroups(page, {id});
    std::erase_if(ids, [&groups](core::ObjectId candidate) { return groups.count(candidate) != 0; });
    if (ids.empty()) {
        ids.push_back(id); // groupe sans membre
    }
    return ids;
}

// Objet qui range la ligne de `id` dans la couche `layer` : le plus haut de ses objets dessinés qui y sont
// (un groupe peut être entrelacé, ou à cheval sur deux couches) ; l'objet lui-même s'il n'en a aucun.
core::ObjectId topDrawnInLayer(const core::Page& page, core::ObjectId id, core::Layer layer,
                               const std::unordered_set<core::ObjectId>& groups)
{
    const std::vector<core::ObjectId> drawn = drawnIds(page, id, groups);
    const std::unordered_set<core::ObjectId> wanted(drawn.begin(), drawn.end());
    for (auto it = page.objects.rbegin(); it != page.objects.rend(); ++it) {
        if (it->common.layer == layer && wanted.count(it->common.id) != 0) {
            return it->common.id;
        }
    }
    return id;
}

std::unordered_set<core::ObjectId> hiddenIds(const core::Page& page)
{
    std::unordered_set<core::ObjectId> hidden;
    for (const core::PageObject& object : page.objects) {
        if (!object.common.visible) {
            hidden.insert(object.common.id);
        }
    }
    return hidden;
}

// Ligne allumée (SelectedRole), lue par rebuild et par le Ctrl+clic : l'objet de la ligne est sélectionné ;
// pour un groupe, aussi quand tous ses membres visibles le sont. Une sélection faite au canevas porte les
// objets dessinés, jamais l'objet Group (core::objectAt, core::objectsTouching), et un masqué n'y entre pas :
// un groupe entièrement masqué ne s'allume que par l'objet Group lui-même.
bool rowSelected(const core::Page& page, core::ObjectId id, const std::unordered_set<core::ObjectId>& groups,
                 const std::unordered_set<core::ObjectId>& hidden,
                 const std::unordered_set<core::ObjectId>& selected)
{
    if (selected.count(id) != 0) {
        return true;
    }
    if (groups.count(id) == 0) {
        return false;
    }
    std::vector<core::ObjectId> shown = drawnIds(page, id, groups);
    std::erase_if(shown, [&hidden](core::ObjectId member) { return hidden.count(member) != 0; });
    return !shown.empty() && std::all_of(shown.begin(), shown.end(), [&selected](core::ObjectId member) {
        return selected.count(member) != 0;
    });
}

} // namespace

ObjectTreeModel::ObjectTreeModel(QObject* parent) : QAbstractListModel(parent)
{
    // Fin d'un contact : la remise à jour retenue part au tour suivant.
    connect(this, &ObjectTreeModel::contactActiveChanged, this, [this] {
        if (!m_contactActive && m_pending) {
            schedule();
        }
    });
}

ObjectTreeModel::~ObjectTreeModel()
{
    detachDocument();
}

int ObjectTreeModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant ObjectTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const Row& row = m_rows[static_cast<std::size_t>(index.row())];
    switch (role) {
    case DepthRole:
        return row.depth;
    case LayerRole:
        return layerKey(row.layer);
    case ObjectIdRole:
        return QVariant::fromValue<qulonglong>(row.id);
    case Qt::DisplayRole:
    case NameRole:
        return row.name;
    case KindRole:
        return row.kind;
    case HiddenRole:
        return row.hidden;
    case LockedRole:
        return row.locked;
    case SelectedRole:
        return row.selected;
    default:
        return {};
    }
}

QHash<int, QByteArray> ObjectTreeModel::roleNames() const
{
    return {
        {DepthRole, QByteArrayLiteral("depth")},       {LayerRole, QByteArrayLiteral("layer")},
        {ObjectIdRole, QByteArrayLiteral("objectId")}, {NameRole, QByteArrayLiteral("name")},
        {KindRole, QByteArrayLiteral("kind")},         {HiddenRole, QByteArrayLiteral("hidden")},
        {LockedRole, QByteArrayLiteral("locked")},     {SelectedRole, QByteArrayLiteral("selected")},
    };
}

void ObjectTreeModel::select(int row, int modifiers)
{
    const Row* target = objectRow(row);
    if (m_tools == nullptr || m_document == nullptr || target == nullptr) {
        return;
    }
    const core::ObjectId id = target->id;
    std::vector<core::ObjectId> ids{id};
    const auto held = Qt::KeyboardModifiers::fromInt(static_cast<Qt::KeyboardModifiers::Int>(modifiers));
    if (held.testFlag(Qt::ControlModifier)) {
        // Ctrl bascule ce que la ligne montre (rowSelected, comme rebuild), relu sur la sélection courante :
        // la ligne de la dernière remise à jour peut être en retard (même tour de boucle, contact). Éteindre
        // la ligne d'un groupe retire le groupe et ses membres.
        const core::Page& page =
            m_document->document().page(static_cast<std::size_t>(m_document->currentIndex()));
        ids = m_tools->user().selection();
        const std::unordered_set<core::ObjectId> selected(ids.begin(), ids.end());
        if (rowSelected(page, id, groupIds(page), hiddenIds(page), selected)) {
            std::unordered_set<core::ObjectId> dropped{id};
            for (const core::ObjectId member : core::expandGroups(page, {id})) {
                dropped.insert(member);
            }
            std::erase_if(ids,
                          [&dropped](core::ObjectId candidate) { return dropped.count(candidate) != 0; });
        } else {
            ids.push_back(id);
        }
    }
    if (ids.empty()) {
        m_tools->clearSelection();
    } else {
        m_tools->selectObjects(ids); // active l'outil Sélection ; le document n'est pas modifié
    }
}

bool ObjectTreeModel::move(int row, int beforeRow)
{
    const Row* moved = objectRow(row);
    if (!canEdit() || moved == nullptr || beforeRow < 1 || beforeRow > rowCount()) {
        return false;
    }
    // La ligne déplacée arrive juste sous `anchor` : couche → sommet de cette couche ; objet → juste en
    // dessous.
    const Row& anchor = m_rows[static_cast<std::size_t>(beforeRow - 1)];
    if (anchor.layer == core::Layer::Background || anchor.id == moved->id) {
        return false;
    }
    if (beforeRow == row) {
        return false; // déjà juste sous l'ancre : dépôt sur place, aucune étape
    }
    const auto pageIndex = static_cast<std::size_t>(m_document->currentIndex());
    const core::Page& page = m_document->document().page(pageIndex);
    std::unordered_map<core::ObjectId, core::Layer> layers;
    std::vector<core::ObjectId> order;
    order.reserve(page.objects.size());
    for (const core::PageObject& object : page.objects) {
        layers.emplace(object.common.id, object.common.layer);
        order.push_back(object.common.id);
    }
    const auto found = layers.find(moved->id);
    if (found == layers.end()) {
        return false; // objet retiré depuis la dernière remise à jour
    }
    // Le groupe bouge avec ses membres (comme core::arrangedOrder).
    const std::vector<core::ObjectId> block = core::expandGroups(page, {moved->id});
    if (found->second != anchor.layer) {
        m_document->execute(std::make_unique<core::SetLayerCommand>(pageIndex, block, anchor.layer));
        return true;
    }
    // Même couche : seuls les objets du bloc qui y sont bougent, comme core::arrangedOrder (couche par
    // couche). Un membre d'une autre couche garde sa place : la page reste rangée par couche.
    const std::unordered_set<core::ObjectId> inBlock(block.begin(), block.end());
    std::vector<core::ObjectId> rest;
    std::vector<core::ObjectId> carried; // ordre relatif gardé
    for (const core::ObjectId id : order) {
        (inBlock.count(id) != 0 && layers[id] == anchor.layer ? carried : rest).push_back(id);
    }
    std::vector<core::ObjectId>::iterator at = rest.end();
    if (anchor.depth == 0) {
        // Sommet de la couche : après le dernier objet de la couche dans l'ordre de la page.
        const auto last = std::find_if(rest.rbegin(), rest.rend(),
                                       [&](core::ObjectId id) { return layers[id] == anchor.layer; });
        if (last == rest.rend()) {
            return false; // seul dans sa couche
        }
        at = last.base();
    } else if (const Row* below = objectRow(beforeRow); below != nullptr && below->layer == anchor.layer) {
        // Entre l'ancre et la ligne qui la suit (jamais la ligne déplacée : beforeRow != row) : juste
        // au-dessus de l'objet qui range cette ligne (rebuild). L'ancre est rangée par le plus haut de ses
        // membres ; l'objet passe aussi sous ceux de ses membres qui sont plus bas, sans couper un groupe
        // d'un seul tenant.
        at = std::find(rest.begin(), rest.end(),
                       topDrawnInLayer(page, below->id, anchor.layer, groupIds(page)));
        if (at == rest.end()) {
            return false;
        }
        ++at;
    } else {
        // L'ancre est la dernière ligne de sa couche : fond de la couche.
        at = std::find_if(rest.begin(), rest.end(),
                          [&](core::ObjectId id) { return layers[id] == anchor.layer; });
        if (at == rest.end()) {
            return false;
        }
    }
    rest.insert(at, carried.begin(), carried.end());
    if (rest == order) {
        return false;
    }
    m_document->execute(std::make_unique<core::ReorderObjectsCommand>(pageIndex, std::move(rest)));
    return true;
}

bool ObjectTreeModel::setHidden(int row, bool hidden)
{
    return setFlag(row, static_cast<int>(core::ObjectFlag::Hidden), hidden);
}

bool ObjectTreeModel::setLocked(int row, bool locked)
{
    if (!setFlag(row, static_cast<int>(core::ObjectFlag::Locked), locked)) {
        return false;
    }
    requestReset(); // aucun Change : le contrôleur n'émet rien (objectflags.h)
    return true;
}

bool ObjectTreeModel::setFlag(int row, int flag, bool value)
{
    const Row* target = objectRow(row);
    if (!canEdit() || target == nullptr) {
        return false;
    }
    const auto which = static_cast<core::ObjectFlag>(flag);
    const auto pageIndex = static_cast<std::size_t>(m_document->currentIndex());
    const core::Page& page = m_document->document().page(pageIndex);
    const bool exists = std::any_of(page.objects.begin(), page.objects.end(),
                                    [&](const core::PageObject& o) { return o.common.id == target->id; });
    // « Déjà » se lit comme la ligne l'affiche : tous les objets dessinés de la ligne.
    if (!exists || core::flagOfAll(page, drawnIds(page, target->id, groupIds(page)), which) == value) {
        return false;
    }
    // Règle D9, comme object.lock et object.hide (ObjectEditing) : un groupe agit sur ses membres.
    m_document->execute(std::make_unique<core::SetObjectFlagCommand>(
        pageIndex, core::expandGroups(page, {target->id}), which, value));
    return true;
}

DocumentController* ObjectTreeModel::document() const noexcept
{
    return m_document.data();
}

void ObjectTreeModel::setDocument(DocumentController* document)
{
    if (m_document == document) {
        return;
    }
    detachDocument();
    m_document = document;
    attachDocument();
    emit sourcesChanged();
    requestReset();
}

ToolHost* ObjectTreeModel::tools() const noexcept
{
    return m_tools.data();
}

void ObjectTreeModel::setTools(ToolHost* tools)
{
    if (m_tools == tools) {
        return;
    }
    disconnect(m_selectionConnection);
    m_tools = tools;
    if (m_tools) {
        m_selectionConnection =
            connect(m_tools.data(), &ToolHost::selectionChanged, this, &ObjectTreeModel::requestReset);
    }
    emit sourcesChanged();
    requestReset();
}

Commands* ObjectTreeModel::commands() const noexcept
{
    return m_commands.data();
}

void ObjectTreeModel::setCommands(Commands* commands)
{
    if (m_commands == commands) {
        return;
    }
    disconnect(m_lockConnection);
    m_commands = commands;
    if (m_commands) {
        // Verrou posé ailleurs : la commande a déjà été exécutée (triggered suit le gestionnaire), sans
        // Change.
        m_lockConnection = connect(
            m_commands.data(), &Commands::triggered, this, [this](const QString& id, const QVariant&) {
                if (id == QLatin1String("object.lock") || id == QLatin1String("object.drag-copy")) {
                    requestReset();
                }
            });
    }
    emit sourcesChanged();
}

int ObjectTreeModel::resetCount() const noexcept
{
    return m_resets;
}

void ObjectTreeModel::requestReset()
{
    m_pending = true;
    if (!m_contactActive) {
        schedule();
    }
}

void ObjectTreeModel::schedule()
{
    if (m_scheduled) {
        return;
    }
    m_scheduled = true;
    QMetaObject::invokeMethod(this, &ObjectTreeModel::flush, Qt::QueuedConnection);
}

void ObjectTreeModel::flush()
{
    m_scheduled = false;
    if (!m_pending || m_contactActive) {
        return; // pendant un contact : repris à sa fin (contactActiveChanged)
    }
    m_pending = false;
    beginResetModel();
    rebuild();
    endResetModel();
    ++m_resets;
    emit reset();
}

void ObjectTreeModel::rebuild()
{
    m_rows.clear();
    if (!m_document) {
        return;
    }
    const core::Page& page =
        m_document->document().page(static_cast<std::size_t>(m_document->currentIndex()));
    const std::unordered_set<core::ObjectId> groups = groupIds(page);
    const std::unordered_set<core::ObjectId> hidden = hiddenIds(page);
    std::unordered_set<core::ObjectId> members;
    std::unordered_map<core::ObjectId, std::size_t>
        drawRank; // place dans la page : dessiné après = plus haut
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        const core::PageObject& object = page.objects[i];
        drawRank.emplace(object.common.id, i);
        if (const auto* group = std::get_if<core::Group>(&object)) {
            members.insert(group->members.begin(), group->members.end());
        }
    }
    std::unordered_set<core::ObjectId> selected;
    if (m_tools) {
        const std::vector<core::ObjectId>& selection = m_tools->user().selection();
        selected.insert(selection.begin(), selection.end());
    }
    // Lignes d'objet par couche, avec leur rang de dessin.
    std::array<std::vector<std::pair<std::size_t, Row>>, kShownLayers.size()> byLayer;
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        const core::PageObject& object = page.objects[i];
        const core::ObjectCommon& common = object.common;
        const auto shown = std::find(kShownLayers.begin(), kShownLayers.end(), common.layer);
        if (shown == kShownLayers.end() || members.count(common.id) != 0) {
            continue;
        }
        Row row;
        row.depth = 1;
        row.layer = common.layer;
        row.id = common.id;
        const core::ObjectKind kind = core::kindOf(object);
        row.kind = kindKey(kind);
        row.name = common.name.empty() ? kindLabel(kind) : QString::fromStdString(common.name);
        row.selected = rowSelected(page, common.id, groups, hidden, selected);
        std::size_t rank = i;
        if (groups.count(common.id) != 0) {
            // Règle D9 : le groupe se range au rang du plus haut de ses membres dans sa couche (l'objet Group
            // n'est pas dessiné ; un membre d'une autre couche n'a pas de rang dans celle-ci) ; masqué ou
            // verrouillé quand tous ses membres le sont (sélectionné : rowSelected).
            const std::vector<core::ObjectId> drawn = drawnIds(page, common.id, groups);
            rank = drawRank.at(topDrawnInLayer(page, common.id, common.layer, groups));
            row.hidden = core::flagOfAll(page, drawn, core::ObjectFlag::Hidden);
            row.locked = core::flagOfAll(page, drawn, core::ObjectFlag::Locked);
        } else {
            row.hidden = !common.visible;
            row.locked = common.locked;
        }
        byLayer[static_cast<std::size_t>(shown - kShownLayers.begin())].emplace_back(rank, std::move(row));
    }
    m_rows.reserve(page.objects.size() + kShownLayers.size());
    for (std::size_t index = 0; index < kShownLayers.size(); ++index) {
        const core::Layer layer = kShownLayers[index];
        Row node;
        node.layer = layer;
        switch (layer) {
        case core::Layer::Top:
            node.name = tr("Couche du dessus");
            break;
        case core::Layer::Middle:
            node.name = tr("Couche intermédiaire");
            break;
        case core::Layer::Bottom:
            node.name = tr("Couche inférieure");
            break;
        case core::Layer::Background:
            node.name = tr("Arrière-plan");
            break;
        }
        m_rows.push_back(std::move(node));
        // Du plus haut au plus bas dans l'ordre de dessin (rangs distincts : un membre n'a pas de ligne).
        std::vector<std::pair<std::size_t, Row>>& rows = byLayer[index];
        std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
        for (std::pair<std::size_t, Row>& entry : rows) {
            m_rows.push_back(std::move(entry.second));
        }
    }
}

void ObjectTreeModel::attachDocument()
{
    if (!m_document) {
        return;
    }
    DocumentController* controller = m_document.data();
    m_observer = controller->addChangeObserver(
        [this](const core::Changes&, DocumentController::ChangeOrigin) { requestReset(); });
    m_documentConnections = {
        connect(controller, &DocumentController::documentChanged, this, &ObjectTreeModel::requestReset),
        connect(controller, &DocumentController::pagesChanged, this, &ObjectTreeModel::requestReset),
    };
}

void ObjectTreeModel::detachDocument()
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

bool ObjectTreeModel::canEdit() const
{
    return m_document != nullptr && m_commands != nullptr && !m_commands->registry().isLocked();
}

const ObjectTreeModel::Row* ObjectTreeModel::objectRow(int row) const
{
    if (row < 0 || row >= rowCount()) {
        return nullptr;
    }
    const Row& candidate = m_rows[static_cast<std::size_t>(row)];
    return candidate.depth == 1 ? &candidate : nullptr;
}

} // namespace actcraft::ui
