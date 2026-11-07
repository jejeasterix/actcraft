#include <actcraft/actions/objectactions.h>
#include <actcraft/core/actionedits.h>
#include <actcraft/ui/actionbrowser.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QtDebug>

#include <algorithm>
#include <memory>
#include <unordered_set>
#include <utility>

namespace actcraft::ui {

namespace {

constexpr actions::ActionCategory kOrder[] = {actions::ActionCategory::Command, actions::ActionCategory::Page,
                                              actions::ActionCategory::Object,
                                              actions::ActionCategory::Document};
constexpr int kCategoryCount = 4; // plus « Toutes » (0)

const actions::ObjectActionCatalog& catalog()
{
    return actions::builtInObjectActionCatalog();
}

QString parameterLabel(const QString& type)
{
    if (type == QLatin1String("page")) {
        return QStringLiteral("Page");
    }
    if (type == QLatin1String("angle")) {
        return QStringLiteral("Angle");
    }
    if (type == QLatin1String("text")) {
        return QStringLiteral("Texte");
    }
    if (type == QLatin1String("url")) {
        return QStringLiteral("Adresse");
    }
    if (type == QLatin1String("file")) {
        return QStringLiteral("Fichier");
    }
    if (type == QLatin1String("translucency")) {
        return QStringLiteral("Translucidité");
    }
    return QStringLiteral("Valeur");
}

QVariant defaultValue(const actions::ActionParameter& parameter)
{
    if (parameter.type == QLatin1String("text") || parameter.type == QLatin1String("url") ||
        parameter.type == QLatin1String("file")) {
        return QString();
    }
    if (parameter.type == QLatin1String("page")) {
        return qMax(1, int(parameter.minimum));
    }
    if (parameter.hasDefault) {
        return parameter.defaultValue;
    }
    return parameter.minimum;
}

// Une borne stricte (facteur > 0) refuse la valeur égale au minimum ou en dessous, comme le moteur de core/.
bool parametersValid(const QVariantMap& values, const actions::ObjectActionInfo& info)
{
    for (const actions::ActionParameter& parameter : info.parameters) {
        if (!parameter.exclusiveMinimum) {
            continue;
        }
        const QVariant value = values.value(parameter.name);
        bool ok = false;
        const double number = value.toDouble(&ok);
        if (!ok || !(number > parameter.minimum)) {
            return false;
        }
    }
    return true;
}

// Paramètres du catalogue seulement, en JSON compact (core::Action::parameters) ; aucun : chaîne vide.
std::string encodeParameters(const QVariantMap& values, const actions::ObjectActionInfo& info)
{
    QJsonObject object;
    for (const actions::ActionParameter& parameter : info.parameters) {
        if (values.contains(parameter.name)) {
            object.insert(parameter.name, QJsonValue::fromVariant(values.value(parameter.name)));
        }
    }
    return object.isEmpty() ? std::string()
                            : QJsonDocument(object).toJson(QJsonDocument::Compact).toStdString();
}

// Les actions au clic remplacées par `clicks`, l'action de glisser-déposer de l'objet gardée en fin de liste
// : un objet en porte au plus une (L8-T13), et DragDropActions::dropOn (dragdropactions.cpp) garde de même
// les actions au clic et pose l'action déposée après elles (revue L8-C3).
std::vector<core::Action> keepingDragDrop(const std::vector<core::Action>& current,
                                          std::vector<core::Action> clicks)
{
    for (const core::Action& action : current) {
        if (action.dragDrop) {
            clicks.push_back(action);
        }
    }
    return clicks;
}

} // namespace

ActionBrowserModel::ActionBrowserModel(QObject* parent) : QAbstractListModel(parent)
{
    flush(); // lignes de « Toutes » dès la construction
}

ActionBrowserModel::~ActionBrowserModel()
{
    detachDocument();
}

int ActionBrowserModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

QVariant ActionBrowserModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const Row& row = m_rows[static_cast<std::size_t>(index.row())];
    switch (role) {
    case IdRole:
        return row.info->id;
    case LabelRole:
        return row.info->label;
    case IconRole:
        return row.info->icon;
    case ExistingRole:
        return row.existing;
    default:
        return {};
    }
}

QHash<int, QByteArray> ActionBrowserModel::roleNames() const
{
    return {{IdRole, "id"}, {LabelRole, "label"}, {IconRole, "icon"}, {ExistingRole, "existing"}};
}

Commands* ActionBrowserModel::commands() const noexcept
{
    return m_commands.data();
}

void ActionBrowserModel::setCommands(Commands* commands)
{
    if (m_commands == commands) {
        return;
    }
    disconnect(m_commandsConnection);
    m_commands = commands;
    if (m_commands) {
        m_commandsConnection =
            connect(m_commands.data(), &Commands::stateChanged, this, &ActionBrowserModel::updateEnabled);
    }
    updateEnabled();
    emit sourcesChanged();
}

DocumentController* ActionBrowserModel::document() const noexcept
{
    return m_document.data();
}

void ActionBrowserModel::setDocument(DocumentController* document)
{
    if (m_document == document) {
        return;
    }
    detachDocument();
    m_document = document;
    attachDocument();
    requestRefresh();
    emit sourcesChanged();
}

ToolHost* ActionBrowserModel::toolHost() const noexcept
{
    return m_tools.data();
}

void ActionBrowserModel::setToolHost(ToolHost* tools)
{
    if (m_tools == tools) {
        return;
    }
    disconnect(m_selectionConnection);
    cancelPick();
    m_tools = tools;
    if (m_tools) {
        m_selectionConnection = connect(m_tools.data(), &ToolHost::selectionChanged, this,
                                        &ActionBrowserModel::onSelectionChanged);
    }
    requestRefresh();
    emit sourcesChanged();
}

bool ActionBrowserModel::contactActive() const noexcept
{
    return m_contactActive;
}

void ActionBrowserModel::setContactActive(bool active)
{
    if (m_contactActive == active) {
        return;
    }
    m_contactActive = active;
    emit contactActiveChanged();
    if (m_contactActive) {
        return;
    }
    if (m_restorePending) {
        QMetaObject::invokeMethod(this, &ActionBrowserModel::restoreAfterPick, Qt::QueuedConnection);
    }
    if (m_pending) {
        schedule(); // la remise à jour reportée pendant le contact
    }
}

int ActionBrowserModel::category() const noexcept
{
    return m_category;
}

void ActionBrowserModel::setCategory(int category)
{
    category = std::clamp(category, 0, kCategoryCount);
    if (m_category == category) {
        return;
    }
    m_category = category;
    emit categoryChanged();
    m_pending = true;
    flush(); // un clic sur l'onglet : les lignes suivent aussitôt, même pendant un contact sur la page
}

QString ActionBrowserModel::currentId() const
{
    return m_currentId;
}

void ActionBrowserModel::setCurrentId(const QString& id)
{
    if (m_currentId == id) {
        return;
    }
    cancelPick();
    m_currentId = id;
    QVariantMap defaults;
    if (const actions::ObjectActionInfo* info = currentInfo()) {
        for (const actions::ActionParameter& parameter : info->parameters) {
            defaults.insert(parameter.name, defaultValue(parameter));
        }
    }
    emit currentIdChanged();
    if (defaults != m_parameters) {
        m_parameters = std::move(defaults);
        emit parametersChanged();
    }
    setTarget(0);
}

QVariantMap ActionBrowserModel::parameters() const
{
    return m_parameters;
}

void ActionBrowserModel::setParameters(const QVariantMap& parameters)
{
    QVariantMap kept;
    if (const actions::ObjectActionInfo* info = currentInfo()) {
        for (const actions::ActionParameter& parameter : info->parameters) {
            kept.insert(parameter.name, parameters.value(parameter.name, m_parameters.value(parameter.name)));
        }
    }
    if (kept == m_parameters) {
        return;
    }
    m_parameters = std::move(kept);
    emit parametersChanged();
}

QVariantList ActionBrowserModel::propertyRows() const
{
    QVariantList rows;
    const actions::ObjectActionInfo* info = currentInfo();
    if (!info) {
        return rows;
    }
    for (const actions::ActionParameter& parameter : info->parameters) {
        rows.append(QVariantMap{{QStringLiteral("name"), parameter.name},
                                {QStringLiteral("type"), parameter.type},
                                {QStringLiteral("label"), parameterLabel(parameter.type)},
                                {QStringLiteral("minimum"), parameter.minimum},
                                {QStringLiteral("maximum"), parameter.maximum},
                                {QStringLiteral("exclusiveMinimum"), parameter.exclusiveMinimum},
                                {QStringLiteral("decimals"), parameter.decimals},
                                {QStringLiteral("step"), parameter.step}});
    }
    if (info->target) {
        rows.append(QVariantMap{{QStringLiteral("name"), QStringLiteral("target")},
                                {QStringLiteral("type"), QStringLiteral("object")},
                                {QStringLiteral("label"), QStringLiteral("Objet visé")}});
    }
    return rows;
}

quint64 ActionBrowserModel::target() const noexcept
{
    return m_target;
}

void ActionBrowserModel::setTarget(quint64 target)
{
    if (m_target == target) {
        return;
    }
    m_target = target;
    emit targetChanged();
}

bool ActionBrowserModel::picking() const noexcept
{
    return m_picking;
}

bool ActionBrowserModel::enabled() const noexcept
{
    return m_enabled;
}

QStringList ActionBrowserModel::categoryLabels() const
{
    return {QStringLiteral("Toutes"), QStringLiteral("Commandes"), QStringLiteral("Pages"),
            QStringLiteral("Objets"), QStringLiteral("Document")};
}

bool ActionBrowserModel::apply()
{
    if (!allowed()) {
        return false;
    }
    const actions::ObjectActionInfo* info = currentInfo();
    const std::vector<core::ObjectId> ids = selectedOnPage();
    if (!info || ids.empty()) {
        return false; // liste grisée : rien à faire
    }
    if (!parametersValid(m_parameters, *info)) {
        emit refused(QStringLiteral("Valeur hors limites : le facteur doit être strictement positif."));
        return false;
    }
    dropVanishedTarget(); // objet supprimé, page changée : aucune action vers un objet absent
    core::Action action;  // créée par ActCraft : kind, sourceType et command restent nuls
    action.catalogId = info->id.toStdString();
    action.parameters = encodeParameters(m_parameters, *info);
    action.target = info->target ? static_cast<core::ObjectId>(m_target) : 0; // 0 : l'objet cliqué
    std::vector<core::SetObjectActionsCommand::ObjectActions> changing;
    const core::Page& page =
        m_document->document().page(static_cast<std::size_t>(m_document->currentIndex()));
    for (const core::ObjectId id : ids) {
        const auto it = std::find_if(page.objects.begin(), page.objects.end(),
                                     [id](const core::PageObject& object) { return object.common.id == id; });
        if (it == page.objects.end()) {
            continue;
        }
        std::vector<core::Action> list = keepingDragDrop(it->common.actions, {action});
        if (!core::sameActions(it->common.actions, list)) {
            changing.emplace_back(id, std::move(list));
        }
    }
    if (changing.empty()) {
        return false; // tous portent déjà cette liste : aucune étape vide
    }
    m_document->execute(std::make_unique<core::SetObjectActionsCommand>(
        static_cast<std::size_t>(m_document->currentIndex()), std::move(changing)));
    return true;
}

bool ActionBrowserModel::removeExisting()
{
    if (!allowed()) {
        return false;
    }
    std::vector<core::SetObjectActionsCommand::ObjectActions> withActions;
    const std::vector<core::ObjectId> ids = selectedOnPage();
    if (!ids.empty()) {
        const core::Page& page =
            m_document->document().page(static_cast<std::size_t>(m_document->currentIndex()));
        const std::unordered_set<core::ObjectId> selected(ids.begin(), ids.end());
        for (const core::PageObject& object : page.objects) {
            const std::vector<core::Action>& actions = object.common.actions;
            const bool clicks = std::any_of(actions.begin(), actions.end(),
                                            [](const core::Action& action) { return !action.dragDrop; });
            if (selected.count(object.common.id) != 0 && clicks) {
                withActions.emplace_back(object.common.id, keepingDragDrop(actions, {}));
            }
        }
    }
    if (withActions.empty()) {
        return false; // aucune action au clic : aucune étape vide
    }
    m_document->execute(std::make_unique<core::SetObjectActionsCommand>(
        static_cast<std::size_t>(m_document->currentIndex()), std::move(withActions)));
    return true;
}

void ActionBrowserModel::pickTarget()
{
    if (!allowed()) {
        return; // fermé par défaut, refusé sous le verrou : l'outil courant ne change pas
    }
    if (m_tools->currentTool() != QLatin1String("select")) {
        m_tools->activate(
            QStringLiteral("select")); // avant d'écouter : son changement de sélection ne compte pas
    }
    m_beforePick = m_tools->user().selection();
    if (!m_picking) {
        m_picking = true;
        emit pickingChanged();
    }
}

void ActionBrowserModel::cancelPick()
{
    if (!m_picking) {
        return;
    }
    m_picking = false;
    emit pickingChanged();
}

void ActionBrowserModel::attachDocument()
{
    if (!m_document) {
        return;
    }
    DocumentController* controller = m_document.data();
    // Toute modification, d'où qu'elle vienne (annulation comprise) : la colonne « existing » la suit.
    m_observer = controller->addChangeObserver(
        [this](const core::Changes&, DocumentController::ChangeOrigin) { requestRefresh(); });
    m_documentConnections = {
        connect(controller, &DocumentController::documentChanged, this, &ActionBrowserModel::requestRefresh),
        connect(controller, &DocumentController::pagesChanged, this, &ActionBrowserModel::requestRefresh),
    };
}

void ActionBrowserModel::detachDocument()
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

void ActionBrowserModel::requestRefresh()
{
    m_pending = true;
    if (!m_contactActive) {
        schedule();
    }
}

void ActionBrowserModel::schedule()
{
    if (m_scheduled) {
        return;
    }
    m_scheduled = true;
    QMetaObject::invokeMethod(
        this,
        [this] {
            m_scheduled = false;
            if (m_pending && !m_contactActive) {
                flush();
            } // pendant un contact : repris à sa fin (setContactActive)
        },
        Qt::QueuedConnection);
}

void ActionBrowserModel::flush()
{
    m_pending = false;
    const std::vector<core::ObjectId> ids = selectedOnPage();
    std::vector<const core::PageObject*> objects;
    if (!ids.empty()) {
        const core::Page& page =
            m_document->document().page(static_cast<std::size_t>(m_document->currentIndex()));
        const std::unordered_set<core::ObjectId> selected(ids.begin(), ids.end());
        for (const core::PageObject& object : page.objects) {
            if (selected.count(object.common.id) != 0) {
                objects.push_back(&object);
            }
        }
    }
    const auto carriedByAll = [&objects](const QString& id) {
        const std::string catalogId = id.toStdString();
        return !objects.empty() &&
               std::all_of(objects.begin(), objects.end(), [&catalogId](const core::PageObject* o) {
                   return std::any_of(
                       o->common.actions.begin(), o->common.actions.end(),
                       [&catalogId](const core::Action& a) { return a.catalogId == catalogId; });
               });
    };
    std::vector<Row> rows;
    for (int i = 0; i < kCategoryCount; ++i) {
        if (m_category != 0 && m_category != i + 1) {
            continue;
        }
        for (const actions::ObjectActionInfo* info : catalog().inCategory(kOrder[i])) {
            rows.push_back(Row{info, carriedByAll(info->id)});
        }
    }
    if (rows != m_rows) { // un document qui change sans toucher aux lignes ne remet rien à zéro
        beginResetModel();
        m_rows = std::move(rows);
        endResetModel();
    }
    m_hasSelection = !ids.empty();
    dropVanishedTarget();
    updateEnabled();
}

void ActionBrowserModel::dropVanishedTarget()
{
    if (m_target == 0) {
        return;
    }
    bool present = false;
    if (m_document && m_document->pageCount() > 0) {
        const core::Page& page =
            m_document->document().page(static_cast<std::size_t>(m_document->currentIndex()));
        const auto id = static_cast<core::ObjectId>(m_target);
        present = std::any_of(page.objects.begin(), page.objects.end(),
                              [id](const core::PageObject& object) { return object.common.id == id; });
    }
    if (!present) {
        setTarget(0);
    }
}

void ActionBrowserModel::onSelectionChanged()
{
    if (m_picking && m_tools) {
        const std::vector<core::ObjectId>& selection = m_tools->user().selection();
        // Un clic sur un objet : il devient la cible. Une sélection vide (clic hors objet) ou republiée à
        // l'identique ne choisit rien.
        // Ctrl ou Maj+clic ajoute en fin : la cible est le premier identifiant absent de la sélection d'avant
        // ; un clic sur un objet déjà sélectionné le laisse seul et le choisit.
        core::ObjectId picked = 0;
        for (const core::ObjectId id : selection) {
            if (std::find(m_beforePick.begin(), m_beforePick.end(), id) == m_beforePick.end()) {
                picked = id;
                break;
            }
        }
        if (picked == 0 && selection.size() == 1 && selection != m_beforePick) {
            picked = selection.front();
        }
        if (picked != 0) {
            m_picking = false;
            emit pickingChanged();
            setTarget(static_cast<quint64>(picked));
            m_restorePending = true;
            if (!m_contactActive) {
                QMetaObject::invokeMethod(this, &ActionBrowserModel::restoreAfterPick, Qt::QueuedConnection);
            }
        }
    }
    requestRefresh();
}

void ActionBrowserModel::restoreAfterPick()
{
    if (!m_restorePending || m_contactActive) {
        return; // pendant un contact : repris à sa fin
    }
    m_restorePending = false;
    if (!m_tools || !m_commands || m_commands->registry().isLocked()) {
        return; // sous le verrou (ou fermé) : rien n'est réactivé
    }
    if (m_beforePick.empty()) {
        m_tools->clearSelection();
    } else {
        m_tools->selectObjects(m_beforePick);
    }
}

void ActionBrowserModel::updateEnabled()
{
    if (!m_commands || m_commands->registry().isLocked()) {
        cancelPick(); // inactif : aucun choix de cible ouvert, aucune sélection rétablie
        m_restorePending = false;
    }
    const bool enabled = m_commands && !m_commands->registry().isLocked() && m_hasSelection;
    if (m_enabled == enabled) {
        return;
    }
    m_enabled = enabled;
    emit enabledChanged();
}

bool ActionBrowserModel::allowed()
{
    if (!m_commands) {
        emit refused(QStringLiteral("Navigateur d'actions sans commandes : rien n'est modifié."));
        return false;
    }
    if (m_commands->registry().isLocked()) {
        emit refused(QStringLiteral("Verrou de classe : les actions ne se modifient pas."));
        return false;
    }
    return m_document && m_tools;
}

std::vector<core::ObjectId> ActionBrowserModel::selectedOnPage() const
{
    std::vector<core::ObjectId> ids;
    if (!m_document || !m_tools || m_document->pageCount() <= 0) {
        return ids;
    }
    const core::Page& page =
        m_document->document().page(static_cast<std::size_t>(m_document->currentIndex()));
    for (const core::ObjectId id : m_tools->user().selection()) {
        const bool onPage =
            std::any_of(page.objects.begin(), page.objects.end(),
                        [id](const core::PageObject& object) { return object.common.id == id; });
        if (onPage) {
            ids.push_back(id);
        }
    }
    return ids;
}

const actions::ObjectActionInfo* ActionBrowserModel::currentInfo() const
{
    return m_currentId.isEmpty() ? nullptr : catalog().find(m_currentId);
}

} // namespace actcraft::ui
