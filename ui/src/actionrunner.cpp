#include "isolatedtext.h"

#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/objectactions.h>
#include <actcraft/core/undo.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/ui/actionrunner.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/designmode.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/linklauncher.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineF>
#include <QRectF>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextLayout>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace actcraft::ui {

namespace {

// Px d'écran, comme le seuil de la sélection (tools/src/selecttool.cpp, kDragThresholdScreen) : au-delà,
// l'appui devient un glisser rendu à l'outil.
constexpr qreal kClickSlopScreen = 4;

const QString kHistoryBack = QStringLiteral("page.history-back");
const QString kHistoryForward = QStringLiteral("page.history-forward");

// Actions du catalogue sans table dans le moteur (core/objectactions.h) : commande existante du registre,
// jouée sur la cible sélectionnée (`target` vrai) ou telle quelle.
struct RegistryAction
{
    const char* action;
    const char* command;
    bool target;
};
constexpr RegistryAction kRegistryActions[] = {
    {"object.copy", "edit.copy", true},
    {"object.cut", "edit.cut", true},
    {"object.paste", "edit.paste", false},
    {"object.align-left", "align.left", true},
    {"object.align-center-x", "align.center-x", true},
    {"object.align-right", "align.right", true},
    {"object.align-top", "align.top", true},
    {"object.align-center-y", "align.center-y", true},
    {"object.align-bottom", "align.bottom", true},
    {"object.align-width", "align.width", true},
    {"object.align-height", "align.height", true},
    {"object.align-angle", "align.angle", true},
    {"object.align-all", "align.all", true},
    {"object.toggle-group", "object.group", true},
    {"object.select-all", "edit.select-all", false},
};

const RegistryAction* registryAction(const QString& id)
{
    for (const RegistryAction& entry : kRegistryActions) {
        if (id == QLatin1String(entry.action)) {
            return &entry;
        }
    }
    return nullptr;
}

// Actions qui écrivent ou créent un texte : son emprise est remesurée dans la même étape.
bool writesText(const QString& id)
{
    return id == QLatin1String("object.add-text") || id == QLatin1String("object.set-text") ||
           id == QLatin1String("object.text-value") || id == QLatin1String("object.new-text") ||
           id == QLatin1String("object.extract-text");
}

core::PageObject* findObject(core::Page& page, core::ObjectId id)
{
    for (core::PageObject& object : page.objects) {
        if (object.common.id == id) {
            return &object;
        }
    }
    return nullptr;
}

const core::PageObject* findObject(const core::Page& page, core::ObjectId id)
{
    for (const core::PageObject& object : page.objects) {
        if (object.common.id == id) {
            return &object;
        }
    }
    return nullptr;
}

// Texte que la mesure sait reprendre : HTML sans lignes lues du fichier (`runs` fait foi tant qu'il y en a ;
// une action de texte le vide, core/src/objectactions_edit.cpp), horizontal ou en colonne (revue L8-C3).
core::Text* measurableText(core::PageObject* object)
{
    auto* text = object ? std::get_if<core::Text>(&core::body(*object)) : nullptr;
    if (text == nullptr || !text->runs.empty() || text->html.empty()) {
        return nullptr;
    }
    return text;
}

// Largeur naturelle du HTML sur le document du peintre (ui/src/textediting.cpp, measuredBounds : marge 0,
// ceil(idealWidth) + 1).
qreal naturalWidth(QTextDocument& measure)
{
    measure.setTextWidth(-1);
    return std::ceil(measure.idealWidth()) + 1;
}

std::unique_ptr<QTextDocument> measureDocument(const std::string& html)
{
    std::unique_ptr<QTextDocument> measure = isolatedTextDocument({});
    measure->setHtml(safeTextHtml(QString::fromStdString(html), {}));
    return measure;
}

// Largeur fixe d'un texte : celle de son cadre s'il n'est pas à sa largeur naturelle (élargi ou replié par
// l'utilisateur, comme m_fixedWidth de l'éditeur) ; 0 sinon.
qreal fixedWidthOf(const core::Text& text)
{
    std::unique_ptr<QTextDocument> measure = measureDocument(text.html);
    return std::abs(naturalWidth(*measure) - text.bounds.width) > 0.5 ? text.bounds.width : 0;
}

// Largeur fixe du texte visé, lue avant l'action (revue L8-C3) : un texte lu avec ses lignes (`runs`) a la
// largeur de son cadre, que l'action, qui vide `runs`, ne lui retire pas (pas d'élargissement en une ligne) ;
// un texte écrit horizontal, celle de fixedWidthOf ; une colonne, aucune (sa mesure est sa colonne).
qreal fixedWidthBefore(core::PageObject* object)
{
    const auto* text = object ? std::get_if<core::Text>(&core::body(*object)) : nullptr;
    if (text == nullptr || text->vertical) {
        return 0;
    }
    if (!text->runs.empty()) {
        return text->bounds.width;
    }
    return text->html.empty() ? 0 : fixedWidthOf(*text);
}

// Règle de measuredBounds (ui/src/textediting.cpp:717) : le peintre découpe le texte à son cadre. Une colonne
// (F-301) se mesure par la mise en page du peintre (render::layOutVerticalColumn), comme columnSize de
// l'éditeur.
core::RectF measuredBounds(const core::Text& text, qreal fixedWidth)
{
    std::unique_ptr<QTextDocument> measure = measureDocument(text.html);
    if (text.vertical) {
        QTextCursor batch(measure.get());
        batch.beginEditBlock();
        render::layOutVerticalColumn(*measure);
        batch.endEditBlock();
        return core::RectF{text.bounds.x, text.bounds.y, std::max<qreal>(1.0, measure->textWidth()),
                           std::ceil(measure->size().height())};
    }
    const qreal width = fixedWidth > 0 ? fixedWidth : naturalWidth(*measure);
    measure->setTextWidth(width);
    return core::RectF{text.bounds.x, text.bounds.y, width, std::ceil(measure->size().height())};
}

bool sameRect(const core::RectF& a, const core::RectF& b)
{
    return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
}

// Patron de measureFirstBaselines (ui/src/textediting.cpp, firstLineBaseline) : origine de la ligne de base
// de la première ligne, coordonnées de l'objet, sur le document de mesure à la largeur de l'emprise ; aucune
// pour une colonne ni pour un document sans ligne (commit de l'éditeur).
std::optional<core::PointF> firstLineBaseline(const core::Text& text)
{
    if (text.vertical) {
        return std::nullopt;
    }
    std::unique_ptr<QTextDocument> measure = measureDocument(text.html);
    measure->setTextWidth(text.bounds.width);
    static_cast<void>(measure->size()); // mise en page complète
    const QTextLayout* layout = measure->firstBlock().layout();
    if (layout == nullptr || layout->lineCount() == 0) {
        return std::nullopt;
    }
    const QTextLine line = layout->lineAt(0);
    return core::PointF{text.bounds.x + layout->position().x() + line.naturalTextRect().x(),
                        text.bounds.y + layout->position().y() + line.y() + line.ascent()};
}

bool samePoint(const std::optional<core::PointF>& a, const std::optional<core::PointF>& b)
{
    return a.has_value() == b.has_value() && (!a || (a->x == b->x && a->y == b->y));
}

// Action de texte et remesure de l'emprise des textes qu'elle écrit ou crée : une seule étape d'annulation.
class RemeasuredTextCommand final : public core::UndoCommand
{
public:
    RemeasuredTextCommand(std::unique_ptr<core::UndoCommand> inner, std::size_t page, core::ObjectId target)
        : m_inner(std::move(inner)), m_page(page), m_target(target)
    {}

    void redo(core::Document& document, core::Changes& changes) override
    {
        qreal targetFixed = 0;
        if (m_page < document.pageCount()) {
            targetFixed = fixedWidthBefore(findObject(document.page(m_page), m_target));
        }
        const std::size_t first = changes.size();
        m_inner->redo(document, changes);
        const std::size_t last = changes.size();
        m_saved.clear();
        for (std::size_t i = first; i < last; ++i) {
            const core::Change change = changes[i];
            const bool changed = change.kind == core::Change::Kind::ObjectChanged;
            if ((!changed && change.kind != core::Change::Kind::ObjectInserted) ||
                change.page >= document.pageCount()) {
                continue;
            }
            core::Page& page = document.page(change.page);
            core::PageObject* object = findObject(page, change.object);
            core::Text* text = measurableText(object);
            if (text == nullptr) {
                continue;
            }
            const qreal fixed = changed && change.object == m_target ? targetFixed : 0;
            core::Text measured = *text;
            measured.bounds = measuredBounds(*text, fixed);
            measured.firstBaseline = firstLineBaseline(measured);
            if (sameRect(measured.bounds, text->bounds) &&
                samePoint(measured.firstBaseline, text->firstBaseline)) {
                continue;
            }
            m_saved.push_back({change.page, change.object, text->bounds, text->firstBaseline});
            text->bounds = measured.bounds;
            text->firstBaseline = measured.firstBaseline;
            changes.push_back({core::Change::Kind::ObjectChanged, change.page, change.object,
                               core::boundsOnPage(page, *object)});
        }
    }

    void undo(core::Document& document, core::Changes& changes) override
    {
        for (auto it = m_saved.rbegin(); it != m_saved.rend(); ++it) {
            if (it->page >= document.pageCount()) {
                continue;
            }
            core::Page& page = document.page(it->page);
            core::PageObject* object = findObject(page, it->object);
            if (core::Text* text = measurableText(object)) {
                const core::RectF wide = core::boundsOnPage(page, *object);
                text->bounds = it->bounds;
                text->firstBaseline = it->firstBaseline;
                changes.push_back({core::Change::Kind::ObjectChanged, it->page, it->object, wide});
            }
        }
        m_inner->undo(document, changes);
    }

private:
    struct Saved
    {
        std::size_t page;
        core::ObjectId object;
        core::RectF bounds; // emprise rendue par la commande du moteur
        std::optional<core::PointF> firstBaseline;
    };
    std::unique_ptr<core::UndoCommand> m_inner;
    std::size_t m_page;
    core::ObjectId m_target;
    std::vector<Saved> m_saved;
};

// Touche d'un objet, règle de core::objectAt (core/src/hittest.cpp) : emprise agrandie de `tolerance`, et
// pour un trait sa ligne (strokeHit), jouée par objectAt lui-même sur une page sonde réduite à ce trait
// (rendu visible : un porteur d'action masqué reste cliquable, D5). Emprise lue sur la page (un groupe).
bool touches(const core::Page& page, const core::PageObject& object, core::PointF point, double tolerance)
{
    if (!core::hasExtent(page, object)) {
        return false;
    }
    const core::RectF b = core::boundsOnPage(page, object);
    const core::RectF grown{b.x - tolerance, b.y - tolerance, b.width + 2 * tolerance,
                            b.height + 2 * tolerance};
    if (!grown.contains(point)) {
        return false;
    }
    if (std::get_if<core::Stroke>(&core::body(object)) == nullptr) {
        return true;
    }
    core::Page probe;
    core::PageObject copy = object;
    copy.common.visible = true;
    probe.objects.push_back(std::move(copy));
    return core::objectAt(probe, point, tolerance).has_value();
}

QJsonObject parametersOf(const core::Action& action)
{
    return QJsonDocument::fromJson(QByteArray::fromStdString(action.parameters)).object();
}

} // namespace

ActionRunner::ActionRunner(QObject* parent) : QObject(parent)
{
    QString error;
    m_catalog.load(actions::defaultObjectActionsJson(), error);
}

ActionRunner::~ActionRunner()
{
    bindHistoryHandlers(m_commands.data(), false);
    if (m_controller && m_changeObserver != 0) {
        m_controller->removeChangeObserver(m_changeObserver);
    }
}

DocumentController* ActionRunner::controller() const
{
    return m_controller.data();
}

bool ActionRunner::actionsDisabled() const
{
    return m_design && m_design->actionsDisabled();
}

qreal ActionRunner::viewZoom() const
{
    const PageView* view = m_tools ? m_tools->view() : nullptr;
    return view != nullptr && view->zoom() > 0 ? view->zoom() : 1.0;
}

// Objet le plus haut sous le point, par la règle de core::objectAt (touches) ; pris seulement s'il porte des
// actions. Un objet masqué qui en porte reste cliquable (D5), un objet verrouillé aussi (D4) ; un objet
// masqué sans action ne cache rien ; un groupe sans action non plus : le clic va à ses membres, comme
// objectAt.
std::optional<core::ObjectId> ActionRunner::actionObjectAt(QPointF pagePoint) const
{
    const DocumentController* c = controller();
    if (c == nullptr) {
        return std::nullopt;
    }
    const int index = c->currentIndex();
    const core::Document& document = c->document();
    if (index < 0 || static_cast<std::size_t>(index) >= document.pageCount()) {
        return std::nullopt;
    }
    const core::Page& page = document.page(static_cast<std::size_t>(index));
    const core::PointF point{pagePoint.x(), pagePoint.y()};
    const double tolerance = core::kHitToleranceScreen / viewZoom();
    for (auto it = page.objects.rbegin(); it != page.objects.rend(); ++it) {
        const core::PageObject& object = *it;
        const bool carries = !object.common.actions.empty();
        if (!object.common.visible && !carries) {
            continue;
        }
        if (!carries && std::holds_alternative<core::Group>(core::body(object))) {
            continue;
        }
        if (!touches(page, object, point, tolerance)) {
            continue;
        }
        if (!carries) {
            return std::nullopt;
        }
        return object.common.id;
    }
    return std::nullopt;
}

bool ActionRunner::takes(QPointF pagePoint) const
{
    return !actionsDisabled() && controller() != nullptr && actionObjectAt(pagePoint).has_value();
}

bool ActionRunner::press(QPointF pagePoint, Qt::MouseButton button)
{
    m_pressed.reset();
    if (button != Qt::LeftButton || actionsDisabled() || controller() == nullptr) {
        return false;
    }
    const std::optional<core::ObjectId> id = actionObjectAt(pagePoint);
    if (!id) {
        return false;
    }
    m_pressed = Pending{static_cast<std::size_t>(controller()->currentIndex()), *id};
    m_pressPoint = pagePoint;
    return true; // rien d'exécuté : un glisser peut encore commencer
}

bool ActionRunner::move(QPointF pagePoint)
{
    if (!m_pressed) {
        return false;
    }
    if (QLineF(m_pressPoint, pagePoint).length() * viewZoom() > kClickSlopScreen) {
        m_pressed.reset(); // glisser : l'outil reçoit l'appui retenu et la suite
        return false;
    }
    return true;
}

bool ActionRunner::release(QPointF pagePoint)
{
    if (!m_pressed) {
        return false;
    }
    if (QLineF(m_pressPoint, pagePoint).length() * viewZoom() > kClickSlopScreen) {
        m_pressed.reset();
        return false;
    }
    m_pending = m_pressed;
    m_pressed.reset();
    if (!m_contactActive) {
        schedule();
    }
    return true;
}

void ActionRunner::cancel()
{
    m_pressed.reset();
    m_pending.reset();
}

void ActionRunner::schedule()
{
    if (m_scheduled) {
        return;
    }
    m_scheduled = true;
    QTimer::singleShot(0, this, [this] {
        m_scheduled = false;
        runPending();
    });
}

void ActionRunner::runPending()
{
    if (m_contactActive || !m_pending) {
        return; // la retombée du contact relance
    }
    const Pending pending = *m_pending;
    m_pending.reset();
    run(pending);
}

void ActionRunner::run(const Pending& pending)
{
    DocumentController* c = controller();
    if (c == nullptr || c->currentIndex() < 0 ||
        static_cast<std::size_t>(c->currentIndex()) != pending.page) {
        return; // page changée entre le relâché et l'exécution
    }
    const core::PageObject* object = findObject(c->document().page(pending.page), pending.object);
    if (object == nullptr) {
        return;
    }
    // Copie : chaque action peut modifier le document (la suivante voit l'effet de la précédente).
    const std::vector<core::Action> list = object->common.actions;
    for (const core::Action& action : list) {
        if (controller() == nullptr ||
            static_cast<std::size_t>(controller()->currentIndex()) != pending.page) {
            return; // une action a changé de page : les suivantes visaient la page de départ
        }
        runOne(action, pending);
    }
}

void ActionRunner::runOne(const core::Action& action, const Pending& pending)
{
    if (!m_commands) {
        refuse(QStringLiteral("Action refusée : commandes indisponibles"));
        return;
    }
    const QString id = QString::fromStdString(action.catalogId);
    const actions::ObjectActionInfo* info = id.isEmpty() ? nullptr : m_catalog.find(id);
    if (info == nullptr) {
        refuse(QStringLiteral("Action non prise en charge"));
        return;
    }
    const core::ObjectId target = action.target != 0 ? action.target : pending.object;
    switch (info->category) {
    case actions::ActionCategory::Command:
    case actions::ActionCategory::Page: {
        QVariant argument;
        if (!info->parameters.isEmpty()) {
            argument = parametersOf(action).value(info->parameters.front().name).toVariant();
        }
        runCommand(info->command, argument);
        return;
    }
    case actions::ActionCategory::Document:
        runDocument(*info, action);
        return;
    case actions::ActionCategory::Object:
        break;
    }
    if (const RegistryAction* entry = registryAction(id)) {
        runOnTarget(QString::fromLatin1(entry->command), entry->target ? target : 0);
        return;
    }
    DocumentController* c = controller();
    const core::Document& document = c->document();
    const core::Page& page = document.page(pending.page);
    core::ActionContext context;
    context.page = pending.page;
    context.source = pending.object;
    context.target = action.target;
    context.pageWidth = page.width;
    context.pageHeight = page.height;
    std::unique_ptr<core::UndoCommand> command =
        core::objectActionCommand(document, action.catalogId, action.parameters, context);
    if (!command) {
        refuse(QStringLiteral("Action impossible sur cette cible"));
        return;
    }
    if (writesText(id)) {
        command = std::make_unique<RemeasuredTextCommand>(std::move(command), pending.page, target);
    }
    c->execute(std::move(command));
}

// Seul chemin d'une commande : Commands::trigger, qui passe par le registre (verrou et diffusion protégée).
// Une commande absente ou sans gestionnaire sur cette tête (H8-7 : lot 7 absent) est non prise en charge.
void ActionRunner::runCommand(const QString& id, const QVariant& argument)
{
    const actions::CommandRegistry& registry = m_commands->registry();
    if (id.isEmpty() || registry.find(id) == nullptr || !registry.hasHandler(id)) {
        refuse(QStringLiteral("Action non prise en charge : %1").arg(id));
        return;
    }
    if (!registry.isEnabled(id)) {
        refuse(QStringLiteral("Action refusée : %1 est verrouillée ou désactivée").arg(id));
        return;
    }
    if (!m_commands->trigger(id, argument)) {
        refuse(QStringLiteral("Action refusée : %1").arg(id));
    }
}

void ActionRunner::runOnTarget(const QString& command, core::ObjectId target)
{
    const actions::CommandRegistry& registry = m_commands->registry();
    if (registry.find(command) == nullptr || !registry.hasHandler(command) || (target != 0 && !m_tools)) {
        refuse(QStringLiteral("Action non prise en charge : %1").arg(command));
        return;
    }
    if (!registry.isEnabled(command)) { // avant la sélection : sous le verrou, rien ne change
        refuse(QStringLiteral("Action refusée : %1 est verrouillée ou désactivée").arg(command));
        return;
    }
    if (target != 0) {
        m_tools->selectObjects({target});
    }
    runCommand(command, {});
}

// Liens et fichiers : LinkLauncher::request, qui valide et demande confirmation ; jamais d'ouverture ici.
void ActionRunner::runDocument(const actions::ObjectActionInfo& info, const core::Action& action)
{
    if (!m_links) {
        refuse(QStringLiteral("Action non prise en charge : ouverture indisponible"));
        return;
    }
    const QJsonObject parameters = parametersOf(action);
    if (info.id == QLatin1String("document.open-website")) {
        m_links->request(parameters.value(QStringLiteral("url")).toString(), LinkLauncher::Website);
        return;
    }
    const int kind =
        info.id == QLatin1String("document.play-sound") ? LinkLauncher::SystemMedia : LinkLauncher::File;
    const QString file = parameters.value(QStringLiteral("file")).toString();
    if (!action.resource.empty() && controller() != nullptr) {
        if (const core::Resource* stored = controller()->document().resource(action.resource)) {
            QString name = QFileInfo(file).fileName();
            if (name.isEmpty()) {
                name = QString::fromStdString(action.resource + "." + stored->extension);
            }
            m_links->requestStored(stored->data, name, controller()->fileUrl().toString());
            return;
        }
    }
    if (file.isEmpty()) {
        refuse(QStringLiteral("Action non prise en charge : fichier absent"));
        return;
    }
    m_links->request(file, kind);
}

void ActionRunner::refuse(const QString& reason)
{
    emit actionRefused(reason);
}

void ActionRunner::hover(QPointF pagePoint)
{
    setHovering(!actionsDisabled() && actionObjectAt(pagePoint).has_value());
}

void ActionRunner::setHovering(bool hovering)
{
    if (hovering == m_hovering) {
        return;
    }
    m_hovering = hovering;
    emit hoveringChanged();
}

void ActionRunner::historyBack()
{
    DocumentController* c = controller();
    const int page = c ? m_history.back(c->pageCount()) : -1; // une page disparue est sautée
    if (page >= 0 && page < c->pageCount()) {
        c->showPage(page); // la page atteinte est l'entrée courante : pas réinscrite
    }
}

void ActionRunner::historyForward()
{
    DocumentController* c = controller();
    const int page = c ? m_history.forward(c->pageCount()) : -1;
    if (page >= 0 && page < c->pageCount()) {
        c->showPage(page);
    }
}

void ActionRunner::pageShown()
{
    if (DocumentController* c = controller()) {
        if (c->generation() != m_historyGeneration) {
            resetHistory(); // un autre document (ouverture, onglet) : son historique part de sa page
        } else {
            m_history.visited(c->currentIndex());
        }
    }
    m_pressed.reset();
    emit actionFramesChanged();
}

void ActionRunner::resetHistory()
{
    m_history.clear();
    if (DocumentController* c = controller()) {
        m_historyGeneration = c->generation();
        m_history.visited(c->currentIndex());
    }
}

Commands* ActionRunner::commands() const noexcept
{
    return m_commands.data();
}

void ActionRunner::bindHistoryHandlers(Commands* commands, bool bind)
{
    if (commands == nullptr) {
        return;
    }
    for (const QString& id : {kHistoryBack, kHistoryForward}) {
        if (commands->registry().find(id) == nullptr) {
            continue;
        }
        actions::CommandRegistry::Handler handler;
        if (bind) {
            const bool back = id == kHistoryBack;
            handler = [this, back](const QVariant&) { back ? historyBack() : historyForward(); };
        }
        commands->setNativeHandler(id, std::move(handler));
    }
}

void ActionRunner::setCommands(Commands* commands)
{
    if (commands == m_commands) {
        return;
    }
    bindHistoryHandlers(m_commands.data(), false);
    m_commands = commands;
    bindHistoryHandlers(commands, true);
    emit commandsChanged();
}

bool ActionRunner::contactActive() const noexcept
{
    return m_contactActive;
}

void ActionRunner::setContactActive(bool active)
{
    if (active == m_contactActive) {
        return;
    }
    m_contactActive = active;
    emit contactActiveChanged();
    if (!active && m_pending) {
        schedule(); // file d'attente d'un tour : jamais dans le gestionnaire de la garde
    }
}

QObject* ActionRunner::document() const noexcept
{
    return m_controller.data();
}

void ActionRunner::setDocument(QObject* document)
{
    auto* c = qobject_cast<DocumentController*>(document);
    if (c == m_controller) {
        return;
    }
    if (m_controller) {
        if (m_changeObserver != 0) {
            m_controller->removeChangeObserver(m_changeObserver);
        }
        disconnect(m_controller, nullptr, this, nullptr);
    }
    m_changeObserver = 0;
    cancel();
    m_controller = c;
    if (c != nullptr) {
        // Avant pagesChanged (DocumentController::settle) : les numéros sont recalés quand la page affichée
        // est réinscrite, chaque changement lu dans l'ordre avec l'indice du moment (core::applyToCache).
        m_changeObserver =
            c->addChangeObserver([this](const core::Changes& changes, DocumentController::ChangeOrigin) {
                for (const core::Change& change : changes) {
                    if (change.kind == core::Change::Kind::PageInserted) {
                        m_history.pageInserted(static_cast<int>(change.page));
                    } else if (change.kind == core::Change::Kind::PageRemoved) {
                        m_history.pageRemoved(static_cast<int>(change.page));
                    }
                }
            });
        connect(c, &DocumentController::pagesChanged, this, &ActionRunner::pageShown);
        // « Enregistrer sous » change fileUrl sans changer de document : l'historique reste (revue L8-C3).
        connect(c, &DocumentController::documentChanged, this, &ActionRunner::actionFramesChanged);
    }
    resetHistory();
    emit documentChanged();
    emit actionFramesChanged();
}

QObject* ActionRunner::designMode() const noexcept
{
    return m_design.data();
}

void ActionRunner::setDesignMode(QObject* designMode)
{
    auto* design = qobject_cast<DesignMode*>(designMode);
    if (design == m_design) {
        return;
    }
    m_design = design;
    emit designModeChanged();
}

LinkLauncher* ActionRunner::links() const noexcept
{
    return m_links.data();
}

void ActionRunner::setLinks(LinkLauncher* links)
{
    if (links == m_links) {
        return;
    }
    m_links = links;
    emit linksChanged();
}

void ActionRunner::setTools(ToolHost* tools)
{
    m_tools = tools;
}

bool ActionRunner::hovering() const noexcept
{
    return m_hovering;
}

QVariantList ActionRunner::actionFrames() const
{
    QVariantList frames;
    const DocumentController* c = controller();
    if (c == nullptr || c->currentIndex() < 0 ||
        static_cast<std::size_t>(c->currentIndex()) >= c->document().pageCount()) {
        return frames;
    }
    const core::Page& page = c->document().page(static_cast<std::size_t>(c->currentIndex()));
    for (const core::PageObject& object : page.objects) {
        if (object.common.actions.empty()) {
            continue;
        }
        const core::RectF b = core::boundsOnPage(page, object);
        frames.append(QRectF(b.x, b.y, b.width, b.height));
    }
    return frames;
}

} // namespace actcraft::ui
