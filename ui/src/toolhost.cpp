#include "tooladapters.h"

#include <actcraft/core/connectors.h>
#include <actcraft/core/model.h>
#include <actcraft/core/penmodifiers.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/core/textedits.h>
#include <actcraft/render/imagefill.h>
#include <actcraft/tools/connectortool.h>
#include <actcraft/tools/filltool.h>
#include <actcraft/tools/penshapetool.h>
#include <actcraft/tools/pentool.h>
#include <actcraft/tools/selecttool.h>
#include <actcraft/tools/toolregistry.h>
#include <actcraft/ui/toolhost.h>

#include <QByteArray>
#include <QCoreApplication>
#include <QImage>
#include <QInputEvent>
#include <QKeyEvent>
#include <QPoint>
#include <QPointF>
#include <QQuickWindow>
#include <QSettings>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QUrl>
#include <QVariantList>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace actcraft::ui {

namespace {

constexpr int kRelayedModifiers = Qt::ShiftModifier | Qt::ControlModifier;

const tools::ToolRegistry& registry()
{
    static const tools::ToolRegistry instance = [] {
        tools::ToolRegistry built;
        tools::registerBuiltinTools(built);
        // Outils du lot 5 (L5-T17) : inscrits ici, le registre de tools/ n'est pas dans cette tâche.
        built.add(QStringLiteral("fill"),
                  [](tools::ToolContext& context) { return std::make_unique<tools::FillTool>(context); });
        built.add(QStringLiteral("pen-shape"),
                  [](tools::ToolContext& context) { return std::make_unique<tools::PenShapeTool>(context); });
        built.add(QStringLiteral("connector"), [](tools::ToolContext& context) {
            return std::make_unique<tools::ConnectorTool>(context);
        });
        return built;
    }();
    return instance;
}

bool isInkTool(const QString& id)
{
    return id == QStringLiteral("pen") || id == QStringLiteral("highlighter");
}

constexpr int kConnectorStyleCount = core::kConnectorStyleCount; // panneau des connecteurs (D11)
constexpr int kConnectorCapCount = 3;                            // extrémités : aucune, rayon, flèche (D11)

// Outils dont la palette principale règle la couleur : l'encre, le pot de peinture (L5-T17), et l'outil des
// formes pour son contour (P6, réglage propre ; L4-C7). Le dernier outil d'encre (m_lastInk) reste un outil
// d'encre.
bool ownsColor(const QString& id)
{
    return isInkTool(id) || id == QStringLiteral("shape") || id == QStringLiteral("fill");
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

// HTML d'un texte venu du fichier ouvert : rien n'en est chargé (bloquant 5, textediting.cpp).
class RecolorDocument final : public QTextDocument
{
protected:
    QVariant loadResource(int /*type*/, const QUrl& /*name*/) override { return QVariant(QByteArray()); }
};

// L6-P1 : tout le texte prend la couleur, comme text.color dans l'éditeur (textediting.cpp : setForeground
// puis fusion) ; les lignes lues (runs) aussi, pour le peintre. Texte qui l'a déjà : rien (pas d'étape vide).
// Comme TextEditing::commit (unchanged = live == m_initialHtml), le HTML est comparé après le même
// aller-retour par QTextDocument, jamais à celui du fichier, que toHtml réécrit toujours.
std::optional<core::Text> recolored(const core::Text& text, core::Argb color)
{
    RecolorDocument doc;
    doc.setHtml(QString::fromStdString(text.html));
    const QString before = doc.toHtml();
    QTextCursor cursor(&doc);
    cursor.select(QTextCursor::Document);
    QTextCharFormat format;
    format.setForeground(QColor::fromRgba(color));
    cursor.mergeCharFormat(format);
    const QString after = doc.toHtml();
    const bool runsTaken = std::all_of(text.runs.begin(), text.runs.end(),
                                       [color](const core::TextSpan& run) { return run.color == color; });
    if (after == before && runsTaken) {
        return std::nullopt;
    }
    core::Text out = text;
    out.html = after.toStdString();
    for (core::TextSpan& run : out.runs) {
        run.color = color;
    }
    return out;
}

void writeShapeSettings(const QString& file, const tools::ShapeSettings& settings)
{
    const std::unique_ptr<QSettings> store = openSettings(file);
    if (!store) {
        return;
    }
    store->setValue(settingsKey(QStringLiteral("shape"), QStringLiteral("preset")), settings.preset);
    store->setValue(settingsKey(QStringLiteral("shape"), QStringLiteral("filled")), settings.filled);
    store->setValue(settingsKey(QStringLiteral("shape"), QStringLiteral("fill")),
                    QColor::fromRgba(settings.fill).name(QColor::HexRgb));
    store->setValue(settingsKey(QStringLiteral("shape"), QStringLiteral("fromCenter")), settings.fromCenter);
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

void ToolHost::activate(const QString& toolId, const QVariant& argument)
{
    if (toolId != QStringLiteral("pen-shape") || !argument.isValid() || argument.toString().isEmpty()) {
        activate(toolId); // sans argument : le tracé gardé
        return;
    }
    const QString current = currentTool();
    if (!isInkTool(current) && current != QStringLiteral("pen-shape")) {
        return; // F-221 : la modification de stylo ne vaut que sous les outils d'encre
    }
    const std::optional<core::PenShape> shape = core::penShapeFromId(argument.toString().toStdString());
    if (!shape) {
        return;
    }
    ensureSettings();
    if (*shape == core::PenShape::Free) { // recliquer le tracé revient au trait libre
        if (current == QStringLiteral("pen-shape")) {
            cancelGesture();
            activate(m_lastInk);
        }
        return;
    }
    cancelGesture();
    m_penShape = argument.toString();
    writeValue(settingsKey(QStringLiteral("pen-shape"), QStringLiteral("trace")), m_penShape);
    if (current == QStringLiteral("pen-shape")) {
        configureTool(existingTool(current));
        emit settingsChanged();
        return;
    }
    activate(QStringLiteral("pen-shape"));
}

void ToolHost::activate(const QString& toolId)
{
    const QString shown = currentTool();
    if (toolId == QStringLiteral("pen-shape") && !isInkTool(shown) && shown != QStringLiteral("pen-shape")) {
        return; // F-221
    }
    if (toolId ==
        QStringLiteral("select-points")) { // object.shape-points : Sélection en mode points de forme
        activate(QStringLiteral("select"));
        if (tools::SelectTool* select = selectTool()) {
            select->setShapePointsMode(true);
        }
        return;
    }
    if (toolId == QStringLiteral("select") && shown == QStringLiteral("select")) {
        // Échap au repos (tool.select, M9) : quitte le mode points de forme.
        tools::SelectTool* select = selectTool();
        if (select && select->shapePointsMode() && !select->isBusy()) {
            select->setShapePointsMode(false);
        }
        return;
    }
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
        configureTool(
            next); // couleur du pot, style du connecteur, tracé et encre de la modification de stylo
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
    if (recolorSelectedTexts(color)) {
        return; // L6-P1 : textes sélectionnés, outil et sélection gardés
    }
    ensureSettings();
    const bool penModifier = currentTool() == QStringLiteral("pen-shape"); // règle l'encre dont elle vient
    if (!penModifier && !ownsColor(currentTool())) {
        activate(m_lastInk); // sous Sélectionner ou la gomme : au dernier outil d'encre (L3-T0)
    }
    const QString id = penModifier ? m_lastInk : currentTool();
    tools::InkSettings settings = user().settings(id);
    settings.color = opaque(color);
    if (auto* pen = dynamic_cast<tools::PenTool*>(existingTool(id))) {
        pen->setColor(settings.color);
    }
    storeSettings(id, settings);
    configureTool(existingTool(currentTool())); // pot de peinture, modification de stylo
}

bool ToolHost::recolorSelectedTexts(const QColor& color)
{
    if (!m_controller || currentTool() != QStringLiteral("select") || user().selection().empty()) {
        return false;
    }
    cancelGesture(); // ce qui est soulevé revient sur la page avant la commande (execute, L3-T12)
    const std::vector<core::ObjectId>& ids = user().selection();
    const auto pageIndex = static_cast<std::size_t>(m_controller->currentIndex());
    const core::Argb argb = opaque(color);
    bool anyText = false;
    std::vector<std::pair<core::ObjectId, core::Text>> texts; // seulement ceux que la couleur change
    for (const core::PageObject& object : m_controller->document().page(pageIndex).objects) {
        if (std::find(ids.begin(), ids.end(), object.common.id) == ids.end()) {
            continue;
        }
        // Un groupe sélectionné n'est pas parcouru (supposé, L6-P1).
        if (const auto* text = std::get_if<core::Text>(&core::body(object))) {
            anyText = true;
            if (std::optional<core::Text> value = recolored(*text, argb)) {
                texts.emplace_back(object.common.id, std::move(*value));
            }
        }
    }
    if (!texts.empty()) {
        m_controller->execute(std::make_unique<core::SetTextsCommand>(pageIndex, std::move(texts)));
    }
    // Textes qui ont tous déjà la couleur : ni étape, ni couleur passée à l'encre (relecture L6-P1).
    return anyText;
}

void ToolHost::setWidth(qreal width)
{
    if (!(width > 0)) {
        return;
    }
    ensureSettings();
    if (const QString shown = currentTool(); shown == QStringLiteral("select") ||
                                             shown == QStringLiteral("fill") ||
                                             shown == QStringLiteral("connector")) {
        activate(m_lastInk); // Sélectionner n'a pas d'épaisseur ; la gomme garde la sienne
    }
    const QString id = currentTool() == QStringLiteral("pen-shape") ? m_lastInk : currentTool();
    tools::InkSettings settings = user().settings(id);
    settings.width = std::clamp(static_cast<double>(width), static_cast<double>(kMinInkWidth),
                                static_cast<double>(kMaxInkWidth));
    if (auto* pen = dynamic_cast<tools::PenTool*>(existingTool(id))) {
        pen->setBaseWidth(settings.width);
    }
    storeSettings(id, settings);
    configureTool(existingTool(currentTool()));
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

void ToolHost::selectObjects(const std::vector<core::ObjectId>& ids)
{
    cancelGesture();
    activate(QStringLiteral("select"));
    user().setSelection(ids);
    if (tools::SelectTool* select = selectTool()) {
        select->refresh(); // filtre sur la page, recalcule le cadre
    } else {
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

// Style du connecteur (rang du panneau, 14 styles) : gardé comme les réglages par outil (F-136), rendu à
// l'outil Connecteur.
void ToolHost::setConnectorStyle(int style)
{
    ensureSettings();
    m_connectorStyle = std::clamp(style, 0, kConnectorStyleCount - 1);
    writeValue(settingsKey(QStringLiteral("connector"), QStringLiteral("style")), m_connectorStyle);
    configureTool(existingTool(QStringLiteral("connector")));
    emit settingsChanged();
}

// Extrémités A et B du connecteur (D11, M3 de L5-C18) : gardées et rendues à l'outil comme le style.
void ToolHost::setConnectorCaps(int capA, int capB)
{
    ensureSettings();
    m_connectorCapA = std::clamp(capA, 0, kConnectorCapCount - 1);
    m_connectorCapB = std::clamp(capB, 0, kConnectorCapCount - 1);
    writeValue(settingsKey(QStringLiteral("connector"), QStringLiteral("capA")), m_connectorCapA);
    writeValue(settingsKey(QStringLiteral("connector"), QStringLiteral("capB")), m_connectorCapB);
    configureTool(existingTool(QStringLiteral("connector")));
    emit settingsChanged();
}

// H5-33 (L5-C3) : choisir un style au panneau règle aussi les extrémités de l'outil selon ses flèches (départ
// → A, arrivée → B : flèche, sinon aucune) ; elles restent réglables ensuite. setConnectorStyle, qui ne fait
// que garder le rang, ne les touche pas : relire ou poser un rang n'écrase jamais des extrémités choisies.
void ToolHost::chooseConnectorStyle(int style)
{
    setConnectorStyle(style);
    const core::ConnectorStyle chosen = core::connectorStyle(m_connectorStyle);
    const int arrow = static_cast<int>(core::ConnectorCap::Arrow);
    const int none = static_cast<int>(core::ConnectorCap::None);
    setConnectorCaps(chosen.arrowStart ? arrow : none, chosen.arrowEnd ? arrow : none);
}

// Vignette d'un style (L5-C3) : le tracé de core (connectorPath) de (4, 4) à (width − 4, height − 4), avec
// les flèches et les pointillés du style ; la page et le panneau dessinent la même géométrie.
QVariantMap ToolHost::connectorStyleSketch(int style, qreal width, qreal height) const
{
    constexpr double kInset = 4;
    core::Connector sketch;
    sketch.firstEnd = {kInset, kInset};
    sketch.secondEnd = {width - kInset, height - kInset};
    sketch.style = static_cast<std::uint8_t>(std::clamp(style, 0, kConnectorStyleCount - 1));
    QVariantList points;
    for (const core::PointF& p : core::connectorPath(sketch)) {
        points.append(QPointF(p.x, p.y));
    }
    const core::ConnectorTips tips = core::connectorTips(sketch);
    const core::ConnectorStyle look = core::connectorStyle(sketch.style);
    return {
        {QStringLiteral("points"), points},
        {QStringLiteral("outwardA"), QPointF(tips.a.outward.x, tips.a.outward.y)},
        {QStringLiteral("outwardB"), QPointF(tips.b.outward.x, tips.b.outward.y)},
        {QStringLiteral("arrowStart"), look.arrowStart},
        {QStringLiteral("arrowEnd"), look.arrowEnd},
        {QStringLiteral("dashed"), look.dashed},
    };
}

int ToolHost::connectorStyle() const noexcept
{
    return m_connectorStyle;
}

int ToolHost::connectorCapA() const noexcept
{
    return m_connectorCapA;
}

int ToolHost::connectorCapB() const noexcept
{
    return m_connectorCapB;
}

void ToolHost::writeValue(const QString& key, const QVariant& value) const
{
    if (const std::unique_ptr<QSettings> store = openSettings(m_settingsFile)) {
        store->setValue(key, value);
    }
}

// Ce que l'outil tient de l'hôte : il ne voit que son contexte (règle de modularité 2), ui/ lui donne le
// reste.
void ToolHost::configureTool(tools::Tool* tool) const
{
    if (auto* fill = dynamic_cast<tools::FillTool*>(tool)) {
        fill->setColor(settingsOf(QStringLiteral("fill")).color);
        fill->setImageFiller([](const QImage& image, QPoint seed, core::Argb color, int tolerance) {
            return render::floodFill(image, seed, color, tolerance);
        });
    } else if (auto* connector = dynamic_cast<tools::ConnectorTool*>(tool)) {
        connector->setStyle(static_cast<std::uint8_t>(m_connectorStyle));
        connector->setCaps(static_cast<core::ConnectorCap>(m_connectorCapA),
                           static_cast<core::ConnectorCap>(m_connectorCapB));
    } else if (auto* penShape = dynamic_cast<tools::PenShapeTool*>(tool)) {
        // Le trait suit l'encre dont la modification vient ; seule l'alpha de la couleur rend le marqueur
        // translucide (stroke.color).
        const tools::InkSettings ink = settingsOf(m_lastInk);
        core::Outline pen;
        pen.color = (ink.color & 0x00FFFFFFu) | (static_cast<core::Argb>(255 - ink.translucency) << 24);
        pen.width = ink.width;
        penShape->setShape(core::penShapeFromId(m_penShape.toStdString()).value_or(core::PenShape::Free));
        penShape->setPen(pen, m_lastInk == QStringLiteral("highlighter"));
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

bool ToolHost::lockIgnored() const noexcept
{
    return m_lockIgnored;
}

void ToolHost::setLockIgnored(bool ignored)
{
    if (m_lockIgnored == ignored) {
        return;
    }
    m_lockIgnored = ignored;
    if (m_adapters) {
        m_adapters->context.lockIgnored = ignored;
    }
    refreshSelection();
    emit designFlagsChanged();
}

bool ToolHost::snapDisabled() const noexcept
{
    return m_snapDisabled;
}

void ToolHost::setSnapDisabled(bool disabled)
{
    if (m_snapDisabled == disabled) {
        return;
    }
    m_snapDisabled = disabled;
    if (m_adapters) {
        m_adapters->context.snapDisabled = disabled;
    }
    emit designFlagsChanged();
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
    m_adapters->context.lockIgnored = m_lockIgnored;
    m_adapters->context.snapDisabled = m_snapDisabled;
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
    tools::Tool* added = wired->tools.emplace(id, std::move(created)).first->second.get();
    configureTool(added);
    return added;
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
    tools::ShapeSettings shape; // défauts du lot 4 : relus à chaque changement de fichier
    if (store) {
        const QString preset =
            store->value(settingsKey(QStringLiteral("shape"), QStringLiteral("preset"))).toString();
        if (core::findShapePreset(preset.toStdString()) != nullptr) {
            shape.preset = preset;
        }
        shape.filled =
            store->value(settingsKey(QStringLiteral("shape"), QStringLiteral("filled")), shape.filled)
                .toBool();
        const QColor fill(
            store->value(settingsKey(QStringLiteral("shape"), QStringLiteral("fill"))).toString());
        if (fill.isValid()) {
            shape.fill = opaque(fill);
        }
        shape.fromCenter =
            store->value(settingsKey(QStringLiteral("shape"), QStringLiteral("fromCenter")), shape.fromCenter)
                .toBool();
    }
    user().setShapeSettings(shape);
    const int styleBefore =
        m_connectorStyle; // L5-C23 : un panneau déjà lié relit la mémoire à la première lecture
    const int capABefore = m_connectorCapA;
    const int capBBefore = m_connectorCapB;
    m_connectorStyle = 0;
    m_connectorCapA = 0;
    m_connectorCapB = 0;
    m_penShape.clear();
    if (store) {
        m_connectorStyle = std::clamp(
            store->value(settingsKey(QStringLiteral("connector"), QStringLiteral("style")), 0).toInt(), 0,
            kConnectorStyleCount - 1);
        m_connectorCapA = std::clamp(
            store->value(settingsKey(QStringLiteral("connector"), QStringLiteral("capA")), 0).toInt(), 0,
            kConnectorCapCount - 1);
        m_connectorCapB = std::clamp(
            store->value(settingsKey(QStringLiteral("connector"), QStringLiteral("capB")), 0).toInt(), 0,
            kConnectorCapCount - 1);
        const QString trace =
            store->value(settingsKey(QStringLiteral("pen-shape"), QStringLiteral("trace"))).toString();
        if (core::penShapeFromId(trace.toStdString())) {
            m_penShape = trace;
        }
    }
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
    if (m_adapters) { // outils déjà créés : les réglages relus du nouveau fichier (L5-T17)
        for (const auto& entry : m_adapters->tools) {
            configureTool(entry.second.get());
        }
    }
    if (styleBefore != m_connectorStyle || capABefore != m_connectorCapA || capBBefore != m_connectorCapB) {
        emit settingsChanged();
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

// La couleur montrée suit l'outil qui la porte : sous l'outil des formes, son contour (comme l'épaisseur).
QString ToolHost::shownInkTool() const
{
    const QString current = currentTool();
    return ownsColor(current) ? current : m_lastInk;
}

QString ToolHost::shownWidthTool() const
{
    const QString current = currentTool();
    const bool borrowsInk = current == QStringLiteral("select") || current == QStringLiteral("fill") ||
                            current == QStringLiteral("connector") || current == QStringLiteral("pen-shape");
    return borrowsInk ? m_lastInk : current;
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

// Formes (lot 4) : le préréglage, le remplissage et le mode de tracé sont gardés chez l'utilisateur
// (ShapeSettings) et dans le même fichier que les réglages par outil ; le contour de la forme (couleur,
// épaisseur) est celui de l'outil « shape » (réglages par outil).
QString ToolHost::shapePreset() const
{
    return m_user ? m_user->shapeSettings().preset : tools::ShapeSettings{}.preset;
}

bool ToolHost::shapeFilled() const
{
    return m_user ? m_user->shapeSettings().filled : tools::ShapeSettings{}.filled;
}

bool ToolHost::shapeFromCenter() const
{
    return m_user ? m_user->shapeSettings().fromCenter : tools::ShapeSettings{}.fromCenter;
}

QStringList ToolHost::shapePresets() const
{
    QStringList ids;
    for (const core::ShapePreset& preset : core::shapeCatalog()) {
        ids.push_back(QString::fromStdString(preset.id));
    }
    return ids;
}

QColor ToolHost::shapeFillColor() const
{
    return QColor::fromRgba(m_user ? m_user->shapeSettings().fill : tools::ShapeSettings{}.fill);
}

QColor ToolHost::shapeDefaultFillColor() const
{
    return QColor::fromRgba(tools::ShapeSettings{}.fill);
}

void ToolHost::chooseShape(const QString& preset)
{
    if (core::findShapePreset(preset.toStdString()) == nullptr) {
        return;
    }
    ensureSettings();
    cancelGesture(); // un autre préréglage ne reprend pas le tracé commencé
    tools::ShapeSettings settings = user().shapeSettings();
    settings.preset = preset;
    user().setShapeSettings(settings);
    writeShapeSettings(m_settingsFile, settings);
    emit settingsChanged();
    activate(QStringLiteral("shape"));
}

void ToolHost::setShapeFilled(bool filled)
{
    ensureSettings();
    tools::ShapeSettings settings = user().shapeSettings();
    settings.filled = filled;
    user().setShapeSettings(settings);
    writeShapeSettings(m_settingsFile, settings);
    emit settingsChanged();
}

void ToolHost::setShapeFillColor(const QColor& color)
{
    if (!color.isValid()) {
        return;
    }
    ensureSettings();
    tools::ShapeSettings settings = user().shapeSettings();
    settings.fill = opaque(color);
    user().setShapeSettings(settings);
    writeShapeSettings(m_settingsFile, settings);
    emit settingsChanged();
}

void ToolHost::setShapeFromCenter(bool fromCenter)
{
    ensureSettings();
    tools::ShapeSettings settings = user().shapeSettings();
    settings.fromCenter = fromCenter;
    user().setShapeSettings(settings);
    writeShapeSettings(m_settingsFile, settings);
    emit settingsChanged();
}

// Contour du préréglage dans un carré 1 × 1, tel que le construit l'outil (même géométrie) ; fermé pour une
// forme pleine. Une ligne rend son segment : les trois formes sans boîte aussi (L4-T3).
QVariantList ToolHost::shapeOutline(const QString& preset) const
{
    const std::string id = preset.toStdString();
    const core::ShapePreset* found = core::findShapePreset(id);
    if (found == nullptr) {
        return {};
    }
    const core::ShapeStyle style;
    std::vector<core::PointF> points;
    if (found->family == core::ShapeFamily::Line) {
        const auto [from, to] = core::dragLine(core::PointF{0, 1}, core::PointF{1, 0}, *found, false, false);
        const core::PageObject object = core::makeLineObject(id, from, to, style);
        if (const auto* line = std::get_if<core::Line>(&object)) {
            points = line->points;
        }
    } else {
        const core::PageObject object = core::makeShapeObject(id, core::RectF{0, 0, 1, 1}, style);
        if (const auto* shape = std::get_if<core::Shape>(&object)) {
            points = shape->polygon;
        }
        if (points.size() > 1 &&
            (points.front().x != points.back().x || points.front().y != points.back().y)) {
            points.push_back(points.front());
        }
    }
    // Une forme peut dépasser sa boîte (la pointe d'une bulle) : le contour est ramené dans [0, 1] par sa
    // propre étendue, pour que la miniature de la palette ne déborde jamais.
    double minX = 0.0;
    double maxX = 1.0;
    double minY = 0.0;
    double maxY = 1.0;
    for (const core::PointF& point : points) {
        minX = std::min(minX, point.x);
        maxX = std::max(maxX, point.x);
        minY = std::min(minY, point.y);
        maxY = std::max(maxY, point.y);
    }
    QVariantList list;
    for (const core::PointF& point : points) {
        list.push_back(QVariantMap{{QStringLiteral("x"), (point.x - minX) / (maxX - minX)},
                                   {QStringLiteral("y"), (point.y - minY) / (maxY - minY)}});
    }
    return list;
}

} // namespace actcraft::ui
