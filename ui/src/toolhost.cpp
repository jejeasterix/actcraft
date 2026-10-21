#include "tooladapters.h"

#include <actcraft/tools/pentool.h>
#include <actcraft/tools/selecttool.h>
#include <actcraft/tools/toolregistry.h>
#include <actcraft/ui/toolhost.h>

#include <QCoreApplication>
#include <QInputEvent>
#include <QKeyEvent>
#include <QQuickWindow>
#include <QSettings>
#include <QVariantList>

#include <algorithm>
#include <memory>
#include <utility>

namespace actcraft::ui {

namespace {

constexpr int kRelayedModifiers = Qt::ShiftModifier | Qt::ControlModifier;

const tools::ToolRegistry& registry()
{
    static const tools::ToolRegistry instance = [] {
        tools::ToolRegistry built;
        tools::registerBuiltinTools(built);
        return built;
    }();
    return instance;
}

bool isInkTool(const QString& id)
{
    return id == QStringLiteral("pen") || id == QStringLiteral("highlighter");
}

QString settingsKey(const QString& toolId, const QString& leaf)
{
    return QStringLiteral("outils/%1/%2").arg(toolId, leaf);
}

// Réglages persistants (F-136) : le fichier `settingsFile` s'il est posé ; sinon ceux de l'application, mais
// seulement si elle a un nom d'organisation (app/main.cpp) : le banc, les captures et les tests, qui n'en ont
// pas, n'écrivent jamais dans un profil réel.
std::unique_ptr<QSettings> openSettings(const QString& file)
{
    if (!file.isEmpty()) {
        return std::make_unique<QSettings>(file, QSettings::IniFormat);
    }
    if (QCoreApplication::organizationName().isEmpty()) {
        return nullptr;
    }
    return std::make_unique<QSettings>();
}

QVariantMap hiddenFrame()
{
    return selectionFrameMap(tools::SelectionView{}, nullptr);
}

core::Argb opaque(const QColor& color)
{
    return static_cast<core::Argb>(color.rgba()) | 0xFF000000u; // la translucidité va à part (F-225)
}

} // namespace

ToolHost::ToolHost(QObject* parent) : QObject(parent), m_selectionFrame(hiddenFrame()) {}

ToolHost::~ToolHost()
{
    if (m_controller && m_observer != 0) {
        m_controller->removeChangeObserver(m_observer); // l'observateur tient `this`
    }
    m_adapters.reset(); // les outils avant les adaptateurs qu'ils regardent
}

Commands* ToolHost::commands() const noexcept
{
    return m_commands.data();
}

void ToolHost::setCommands(Commands* commands)
{
    if (commands == m_commands) {
        return;
    }
    cancelGesture();
    m_commands = commands;
    m_adapters.reset();
    syncToolCommands();
    emit wiringChanged();
}

DocumentController* ToolHost::documentController() const noexcept
{
    return m_controller.data();
}

void ToolHost::setDocumentController(DocumentController* controller)
{
    if (controller == m_controller) {
        return;
    }
    cancelGesture();
    if (m_controller) {
        disconnect(m_controller, nullptr, this, nullptr);
        if (m_observer != 0) {
            m_controller->removeChangeObserver(m_observer);
        }
    }
    m_observer = 0;
    m_controller = controller;
    m_adapters.reset();
    if (m_controller) {
        // Annuler, Rétablir, Effacer, Ouvrir… et changement de page : l'outil abandonne d'abord son geste ;
        // la sélection et la gomme déposent sans étape (M2). Le soulèvement, lui, n'annonce rien (B1).
        connect(m_controller, &DocumentController::editAboutToApply, this, &ToolHost::cancelGesture);
        connect(m_controller, &DocumentController::pageAboutToChange, this, &ToolHost::cancelGesture);
        connect(m_controller, &DocumentController::pagesChanged, this, &ToolHost::refreshSelection);
        connect(m_controller, &DocumentController::documentChanged, this, &ToolHost::refreshSelection);
        // Annuler et Rétablir vident la sélection (décision de L3-T0).
        m_observer = m_controller->addChangeObserver(
            [this](const core::Changes&, DocumentController::ChangeOrigin origin) {
                if ((origin == DocumentController::ChangeOrigin::Undo ||
                     origin == DocumentController::ChangeOrigin::Redo) &&
                    m_user) {
                    m_user->setSelection({});
                }
            });
    }
    emit wiringChanged();
}

PageView* ToolHost::view() const noexcept
{
    return m_view.data();
}

void ToolHost::setView(PageView* view)
{
    if (view == m_view) {
        return;
    }
    cancelGesture();
    if (m_view) {
        disconnect(m_view, nullptr, this, nullptr);
    }
    m_view = view;
    m_adapters.reset();
    if (m_view) {
        // L'encre remplacée : les adaptateurs sont reconstruits à la demande suivante (adapters()), après
        // l'abandon du geste en cours (m1, L3-C8 : discardAdapters, pas cancelGesture, voir plus bas).
        connect(m_view, &PageView::inkChanged, this, [this] { discardAdapters(); });
        if (!m_filtering && QCoreApplication::instance()) {
            // Filtre de l'application : il passe avant celui que la vue pose sur la fenêtre pour le stylet.
            QCoreApplication::instance()->installEventFilter(this);
            m_filtering = true;
        }
        ensureSettings();
        applyToView();
    }
    emit wiringChanged();
}

render::LiftLayerItem* ToolHost::liftLayer() const noexcept
{
    return m_liftLayer.data();
}

void ToolHost::setLiftLayer(render::LiftLayerItem* layer)
{
    if (layer == m_liftLayer) {
        return;
    }
    cancelGesture();
    m_liftLayer = layer;
    m_adapters.reset();
    emit wiringChanged();
}

QString ToolHost::currentTool() const
{
    return m_user ? m_user->currentTool() : QStringLiteral("pen");
}

QColor ToolHost::color() const
{
    return QColor::fromRgba(settingsOf(shownInkTool()).color);
}

qreal ToolHost::width() const
{
    return settingsOf(shownWidthTool()).width;
}

int ToolHost::selectionCount() const
{
    return m_user ? static_cast<int>(m_user->selection().size()) : 0;
}

bool ToolHost::handlesVisible() const
{
    return m_handlesVisible;
}

QVariantMap ToolHost::selectionFrame() const
{
    return m_selectionFrame;
}

QRectF ToolHost::trashArea() const noexcept
{
    return m_trashArea;
}

void ToolHost::setTrashArea(const QRectF& area)
{
    if (area == m_trashArea) {
        return;
    }
    m_trashArea = area;
    emit trashAreaChanged();
}

bool ToolHost::trashHover() const noexcept
{
    return m_trashHover;
}

QString ToolHost::settingsFile() const
{
    return m_settingsFile;
}

void ToolHost::setSettingsFile(const QString& path)
{
    if (path == m_settingsFile) {
        return;
    }
    m_settingsFile = path;
    m_settingsLoaded = false;
    ensureSettings(); // relus, et rendus aux outils déjà créés
    applyToView();
    emit settingsFileChanged();
    emit settingsChanged();
}

void ToolHost::activate(const QString& toolId)
{
    if (!registry().contains(toolId)) {
        return;
    }
    ensureSettings();
    tools::UserState& state = user();
    if (state.currentTool() == toolId) {
        return; // activer l'outil courant ne fait rien
    }
    if (tools::Tool* previous = existingTool(state.currentTool())) {
        previous->deactivate(); // abandon du geste en cours (M9) ; la sélection masque son cadre
    }
    state.setCurrentTool(toolId);
    if (isInkTool(toolId)) {
        m_lastInk = toolId;
    }
    if (tools::Tool* next = activeTool()) {
        next->activate();
    }
    syncToolCommands();
    applyToView();
    emit currentToolChanged();
    emit settingsChanged();
}

void ToolHost::setColor(const QColor& color)
{
    if (!color.isValid()) {
        return;
    }
    ensureSettings();
    if (!isInkTool(currentTool())) {
        activate(m_lastInk); // sous Sélectionner ou la gomme : au dernier outil d'encre (L3-T0)
    }
    const QString id = currentTool();
    tools::InkSettings settings = user().settings(id);
    settings.color = opaque(color);
    if (auto* pen = dynamic_cast<tools::PenTool*>(existingTool(id))) {
        pen->setColor(settings.color);
    }
    storeSettings(id, settings);
}

void ToolHost::setWidth(qreal width)
{
    if (!(width > 0)) {
        return;
    }
    ensureSettings();
    if (currentTool() == QStringLiteral("select")) {
        activate(m_lastInk); // Sélectionner n'a pas d'épaisseur ; la gomme garde la sienne
    }
    const QString id = currentTool();
    tools::InkSettings settings = user().settings(id);
    settings.width = std::clamp(static_cast<double>(width), static_cast<double>(kMinInkWidth),
                                static_cast<double>(kMaxInkWidth));
    if (auto* pen = dynamic_cast<tools::PenTool*>(existingTool(id))) {
        pen->setBaseWidth(settings.width);
    }
    storeSettings(id, settings);
}

// Les opérations de la sélection peuvent arriver pendant un geste (raccourci pendant un glissement) : le
// contrôleur ne dépose pas d'abord ce qui est soulevé (execute, L3-T12), l'outil abandonne donc son geste
// avant.
void ToolHost::selectAll()
{
    cancelGesture();
    activate(QStringLiteral("select"));
    if (tools::SelectTool* select = selectTool()) {
        select->selectAll();
    }
}

void ToolHost::clearSelection()
{
    cancelGesture();
    if (tools::SelectTool* select = selectTool()) {
        select->clearSelection();
    } else if (m_user) {
        m_user->setSelection({});
        emit selectionChanged();
    }
}

void ToolHost::deleteSelection()
{
    cancelGesture();
    if (tools::SelectTool* select = selectTool()) {
        select->deleteSelection();
    }
}

void ToolHost::enlargeSelection()
{
    cancelGesture();
    if (tools::SelectTool* select = selectTool()) {
        select->enlarge();
    }
}

void ToolHost::reduceSelection()
{
    cancelGesture();
    if (tools::SelectTool* select = selectTool()) {
        select->reduce();
    }
}

void ToolHost::setSelectionTranslucency(int value)
{
    cancelGesture();
    if (tools::SelectTool* select = selectTool()) {
        select->setTranslucency(static_cast<std::uint8_t>(std::clamp(value, 0, 255)));
    }
}

void ToolHost::cancelGesture()
{
    if (!m_user) {
        return;
    }
    tools::Tool* tool = existingTool(m_user->currentTool());
    if (tool && tool->isBusy()) {
        tool->cancel();
    }
}

tools::Tool* ToolHost::activeTool(input::UserId) const
{
    return toolFor(currentTool());
}

tools::UserState& ToolHost::user(input::UserId)
{
    if (!m_user) {
        m_user = std::make_unique<tools::UserState>(input::kFirstUser);
    }
    return *m_user;
}

ToolAdapters* ToolHost::adapters() const
{
    render::InkItem* ink = m_view ? m_view->ink() : nullptr;
    if (!m_controller || !m_view || !m_liftLayer || !ink) {
        discardAdapters();
        return nullptr;
    }
    if (m_adapters &&
        m_adapters->matches(m_controller.data(), m_view.data(), ink, m_liftLayer.data(), m_commands.data())) {
        return m_adapters.get();
    }
    discardAdapters();
    // L'hôte n'est jamais constant pour ceux qui le branchent : activeTool() const crée à la demande.
    auto* self = const_cast<ToolHost*>(this);
    self->ensureSettings();
    m_adapters = std::make_unique<ToolAdapters>(
        *m_controller, *m_view, *ink, *m_liftLayer, m_commands.data(), self->user(),
        [self](const QVariantMap& frame, bool handles) { self->showSelection(frame, handles); },
        [self] { return self->m_trashArea; }, [self](bool hover) { self->setTrashHoverState(hover); });
    return m_adapters.get();
}

// m1 (L3-C8) : des outils jetés abandonnent d'abord leur geste (dépôt sans étape, couche soulevée vidée),
// comme avant setView ou setLiftLayer. cancelGesture() ne convient pas : il passe par adapters(), qui ne
// retrouve plus les outils d'une encre déjà remplacée (et s'y rappellerait). Seulement si tout ce qu'ils
// regardent vit encore : l'abandon toucherait sinon un objet détruit. Sortis de m_adapters d'abord : un
// signal émis pendant l'abandon (documentChanged, refreshSelection) ne les retrouve pas et ne les annule pas
// deux fois.
void ToolHost::discardAdapters() const
{
    const std::unique_ptr<ToolAdapters> discarded = std::move(m_adapters);
    if (!discarded || !discarded->controllerRef || !discarded->viewRef || !discarded->inkRef ||
        !discarded->layerRef) {
        return;
    }
    for (const auto& entry : discarded->tools) {
        if (tools::Tool* tool = entry.second.get(); tool && tool->isBusy()) {
            tool->cancel();
        }
    }
}

tools::SelectTool* ToolHost::selectTool() const
{
    return dynamic_cast<tools::SelectTool*>(toolFor(QStringLiteral("select")));
}

tools::InkSettings ToolHost::settingsOf(const QString& id) const
{
    return m_user ? m_user->settings(id) : tools::defaultSettings(id);
}

void ToolHost::storeSettings(const QString& id, const tools::InkSettings& settings)
{
    user().setSettings(id, settings);
    writeSettings(id);
    applyToView();
    emit settingsChanged();
}

tools::Tool* ToolHost::toolFor(const QString& id) const
{
    ToolAdapters* wired = adapters();
    if (!wired) {
        return nullptr;
    }
    if (auto found = wired->tools.find(id); found != wired->tools.end()) {
        return found->second.get();
    }
    std::unique_ptr<tools::Tool> created = registry().create(id, wired->context); // réglages de UserState
    if (!created) {
        return nullptr;
    }
    return wired->tools.emplace(id, std::move(created)).first->second.get();
}

tools::Tool* ToolHost::existingTool(const QString& id) const
{
    ToolAdapters* wired = adapters();
    if (!wired) {
        return nullptr;
    }
    const auto found = wired->tools.find(id);
    return found == wired->tools.end() ? nullptr : found->second.get();
}

void ToolHost::ensureSettings()
{
    if (m_settingsLoaded) {
        return;
    }
    m_settingsLoaded = true;
    const std::unique_ptr<QSettings> store = openSettings(m_settingsFile);
    for (const QString& id : registry().ids()) {
        if (id == QStringLiteral("select")) {
            continue;
        }
        tools::InkSettings settings = tools::defaultSettings(id);
        if (store) {
            const QColor stored(store->value(settingsKey(id, QStringLiteral("couleur"))).toString());
            if (stored.isValid()) {
                settings.color = opaque(stored);
            }
            bool ok = false;
            const double width = store->value(settingsKey(id, QStringLiteral("epaisseur"))).toDouble(&ok);
            if (ok && width > 0) {
                settings.width = width;
            }
        }
        user().setSettings(id, settings);
        if (m_adapters) {
            if (auto found = m_adapters->tools.find(id); found != m_adapters->tools.end()) {
                if (auto* pen = dynamic_cast<tools::PenTool*>(found->second.get())) {
                    pen->setColor(settings.color);
                    pen->setBaseWidth(settings.width);
                }
            }
        }
    }
}

void ToolHost::writeSettings(const QString& toolId) const
{
    const std::unique_ptr<QSettings> store = openSettings(m_settingsFile);
    if (!store || !m_user) {
        return;
    }
    const tools::InkSettings settings = m_user->settings(toolId);
    store->setValue(settingsKey(toolId, QStringLiteral("couleur")),
                    QColor::fromRgba(settings.color).name(QColor::HexRgb));
    store->setValue(settingsKey(toolId, QStringLiteral("epaisseur")), settings.width);
}

void ToolHost::applyToView()
{
    if (!m_view) {
        return;
    }
    // La vue n'a plus de couleur propre : elle montre celle de l'outil d'encre courant (ou du dernier), avec
    // l'alpha du marqueur, et l'épaisseur de l'outil courant (la gomme : la sienne).
    const tools::InkSettings ink = user().settings(shownInkTool());
    QColor shown = QColor::fromRgba(ink.color);
    shown.setAlpha(255 - ink.translucency);
    m_view->setInkColor(shown);
    m_view->setInkWidth(user().settings(shownWidthTool()).width);
}

void ToolHost::syncToolCommands()
{
    if (!m_commands) {
        return;
    }
    const QString current = currentTool();
    for (const QString& id : registry().ids()) {
        m_commands->setChecked(QStringLiteral("tool.") + id, id == current);
    }
}

void ToolHost::showSelection(const QVariantMap& frame, bool handlesVisible)
{
    // Un autre outil que Sélectionner peut faire agir la sélection (Supprimer au stylo) : le cadre reste
    // masqué.
    const bool shown = currentTool() == QStringLiteral("select");
    m_selectionFrame = shown ? frame : hiddenFrame();
    m_handlesVisible = shown && handlesVisible;
    emit selectionChanged();
}

void ToolHost::setTrashHoverState(bool hover)
{
    if (hover == m_trashHover) {
        return;
    }
    m_trashHover = hover;
    emit trashHoverChanged();
}

void ToolHost::refreshSelection()
{
    // Jamais pendant un geste ni un soulèvement : la page affichée n'a plus les objets soulevés, la sélection
    // les perdrait (B1).
    if (m_controller && m_controller->isLifting()) {
        return;
    }
    auto* select = dynamic_cast<tools::SelectTool*>(existingTool(QStringLiteral("select")));
    if (select && !select->isBusy()) {
        select->refresh();
    } else if (!select) {
        emit selectionChanged();
    }
}

void ToolHost::relayModifiers(int key, int modifiers)
{
    if (tools::Tool* tool = activeTool()) {
        tool->onKey(key, modifiers & kRelayedModifiers);
    }
}

QString ToolHost::shownInkTool() const
{
    const QString current = currentTool();
    return isInkTool(current) ? current : m_lastInk;
}

QString ToolHost::shownWidthTool() const
{
    const QString current = currentTool();
    return current == QStringLiteral("select") ? m_lastInk : current;
}

// Maj et Ctrl (L3-T8) : un échantillon ne les porte pas. Relayés à l'outil actif à l'appui et au relâché de
// la touche, et avant chaque appui de pointeur (état resté collé après une perte de focus corrigé). Échap et
// Suppr n'y vont pas : ce sont des raccourcis de commande (M9).
bool ToolHost::eventFilter(QObject* watched, QEvent* event)
{
    if (!m_view || watched != m_view->window()) {
        return false;
    }
    switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        const auto* key = static_cast<const QKeyEvent*>(event);
        const int code = key->key();
        if (code != Qt::Key_Shift && code != Qt::Key_Control) {
            break;
        }
        const int own = code == Qt::Key_Shift ? Qt::ShiftModifier : Qt::ControlModifier;
        const int held = static_cast<int>(key->modifiers());
        relayModifiers(code, event->type() == QEvent::KeyPress ? (held | own) : (held & ~own));
        break;
    }
    case QEvent::MouseButtonPress:
    case QEvent::TabletPress:
    case QEvent::TouchBegin:
        relayModifiers(Qt::Key_unknown,
                       static_cast<int>(static_cast<const QInputEvent*>(event)->modifiers()));
        break;
    default:
        break;
    }
    return false;
}

// Talon L4-T1 : rempli par L4-T11.
QString ToolHost::shapePreset() const
{
    return QStringLiteral("line");
}

bool ToolHost::shapeFilled() const
{
    return true;
}

bool ToolHost::shapeFromCenter() const
{
    return false;
}

QStringList ToolHost::shapePresets() const
{
    return {};
}

void ToolHost::chooseShape(const QString&) {}

void ToolHost::setShapeFilled(bool) {}

void ToolHost::setShapeFillColor(const QColor&) {}

void ToolHost::setShapeFromCenter(bool) {}

QVariantList ToolHost::shapeOutline(const QString&) const
{
    return {};
}

} // namespace actcraft::ui
