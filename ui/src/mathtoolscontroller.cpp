#include "mathtoolscontroller.h"

#include "presentationdocument.h"

#include <QQuickItem>
#include <QSettings>
#include <QTimer>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <variant>

namespace actcraft::ui {

namespace {

constexpr double kDefaultLength = 600;
constexpr double kDefaultThickness = 80;
constexpr double kDefaultSquareSide = 300;
constexpr double kDefaultRadius = 200;
constexpr double kNextOffset = 24; // une règle de plus s'ouvre décalée, pas sur la précédente
constexpr double kMinLength = 120;
constexpr double kMaxLength = 4000;
constexpr double kMinRadius = 60;
constexpr double kMaxRadius = 2000;
// En deçà (degrés balayés), un appui sur le pourtour sans arc : ni rayons ni secteur.
constexpr double kMinSweep = 1;
const char* const kGuidedTool = "pen"; // tools::PenTool::id()
const char* const kRulerGroup = "mathtools/ruler";
const char* const kSquareGroup = "mathtools/setsquare";
const char* const kProtractorGroup = "mathtools/protractor";

QString unitName(features::RulerUnit unit)
{
    switch (unit) {
    case features::RulerUnit::Cm:
        return QStringLiteral("cm");
    case features::RulerUnit::Mm:
        return QStringLiteral("mm");
    case features::RulerUnit::Inch:
        return QStringLiteral("inch");
    case features::RulerUnit::None:
        return QStringLiteral("none");
    }
    return QStringLiteral("none");
}

features::RulerUnit unitFrom(const QString& name, features::RulerUnit fallback)
{
    if (name == QLatin1String("cm")) {
        return features::RulerUnit::Cm;
    }
    if (name == QLatin1String("mm")) {
        return features::RulerUnit::Mm;
    }
    if (name == QLatin1String("inch")) {
        return features::RulerUnit::Inch;
    }
    if (name == QLatin1String("none")) {
        return features::RulerUnit::None;
    }
    return fallback;
}

QString shapeName(features::Protractor::Kind kind)
{
    return kind == features::Protractor::Kind::Half ? QStringLiteral("half") : QStringLiteral("full");
}

features::Protractor::Kind shapeFrom(const QString& name, features::Protractor::Kind fallback)
{
    if (name == QLatin1String("full")) {
        return features::Protractor::Kind::Full;
    }
    if (name == QLatin1String("half")) {
        return features::Protractor::Kind::Half;
    }
    return fallback;
}

QString styleName(features::ArcStyle style)
{
    switch (style) {
    case features::ArcStyle::Arc:
        return QStringLiteral("arc");
    case features::ArcStyle::Closed:
        return QStringLiteral("closed");
    case features::ArcStyle::Filled:
        return QStringLiteral("filled");
    }
    return QStringLiteral("arc");
}

features::ArcStyle styleFrom(const QString& name, features::ArcStyle fallback)
{
    if (name == QLatin1String("arc")) {
        return features::ArcStyle::Arc;
    }
    if (name == QLatin1String("closed")) {
        return features::ArcStyle::Closed;
    }
    if (name == QLatin1String("filled")) {
        return features::ArcStyle::Filled;
    }
    return fallback;
}

core::PointF pointOf(QPointF point)
{
    return core::PointF{point.x(), point.y()};
}

// Ce que le rapporteur ajoute après le trait guidé, à sa couleur (la couche, celle du trait, est posée par
// insertArc). Fermé : les deux rayons d'un seul objet (bout de départ de l'arc, centre, bout d'arrivée), de
// l'épaisseur du trait. Plein : le secteur rempli, sans contour (l'arc est le trait du stylo). `path` :
// features::arcPath, le centre en premier et en dernier.
core::PageObject arcObject(const std::vector<QPointF>& path, features::ArcStyle style,
                           const core::Stroke& stroke)
{
    if (style == features::ArcStyle::Filled) {
        core::Shape sector;
        sector.kind = core::ShapeKind::Path;
        std::vector<core::PointF> points;
        points.reserve(path.size());
        double minX = path.front().x();
        double minY = path.front().y();
        double maxX = minX;
        double maxY = minY;
        for (const QPointF& point : path) {
            points.push_back(pointOf(point));
            minX = std::min(minX, point.x());
            minY = std::min(minY, point.y());
            maxX = std::max(maxX, point.x());
            maxY = std::max(maxY, point.y());
        }
        sector.paths.push_back(std::move(points));
        sector.bounds = core::RectF{minX, minY, maxX - minX, maxY - minY};
        sector.pen.style = core::PenStyle::None;
        sector.pen.width = 0;
        sector.brush = core::solidFill(stroke.color);
        core::PageObject filled = std::move(sector);
        return filled;
    }
    core::Line radii;
    radii.points = {pointOf(path[1]), pointOf(path.front()), pointOf(path[path.size() - 2])};
    radii.pen.color = stroke.color;
    radii.pen.width = stroke.baseWidth;
    core::PageObject closed = std::move(radii);
    return closed;
}

} // namespace

std::optional<input::Sample> MathToolsController::ArcGuide::project(const input::Sample& sample,
                                                                    double tolerance) const
{
    return protractor.project(sample, tolerance);
}

std::optional<input::Sample> MathToolsController::ArcGuide::capture(const input::Sample& sample,
                                                                    double tolerance)
{
    const auto onRim = protractor.capture(sample, tolerance);
    seized = onRim.has_value();
    return onRim;
}

input::Sample MathToolsController::ArcGuide::follow(const input::Sample& sample)
{
    return protractor.follow(sample);
}

features::StrokeGuide* MathToolsController::Entry::guide() const noexcept
{
    if (ruler) {
        return ruler.get();
    }
    if (square) {
        return square.get();
    }
    return protractor.get();
}

const char* MathToolsController::Entry::settingsGroup() const noexcept
{
    if (ruler) {
        return kRulerGroup;
    }
    return square ? kSquareGroup : kProtractorGroup;
}

MathToolsController::MathToolsController(QObject* parent) : QObject(parent) {}

MathToolsController::~MathToolsController()
{
    // Le filtre et l'observateur tiennent `this` : jamais laissés à une vue ou un document qui survit.
    if (m_view && m_filterInstalled) {
        m_view->setSampleFilter({});
    }
    if (m_document && m_strokeObserver != 0) {
        m_document->removeChangeObserver(m_strokeObserver);
    }
}

PageView* MathToolsController::view() const noexcept
{
    return m_view;
}

void MathToolsController::setView(PageView* view)
{
    if (view == m_view) {
        return;
    }
    if (m_view && m_filterInstalled) {
        m_view->setSampleFilter({});
    }
    m_filterInstalled = false;
    disconnect(m_zoomConnection);
    m_view = view;
    if (m_view) {
        m_zoomConnection = connect(m_view, &PageView::zoomChanged, this, &MathToolsController::syncTolerance);
    }
    syncTolerance();
    updateFilter();
    emit viewChanged();
}

ToolHost* MathToolsController::toolHost() const noexcept
{
    return m_toolHost;
}

void MathToolsController::setToolHost(ToolHost* host)
{
    if (host == m_toolHost) {
        return;
    }
    m_toolHost = host;
    storeOpenTools();
    emit toolHostChanged();
}

DocumentController* MathToolsController::documentController() const noexcept
{
    return m_document;
}

void MathToolsController::setDocumentController(DocumentController* controller)
{
    if (controller == m_document) {
        return;
    }
    if (m_document && m_strokeObserver != 0) {
        m_document->removeChangeObserver(m_strokeObserver);
    }
    m_strokeObserver = 0;
    m_pendingArc.reset();
    m_document = controller;
    watchStrokes();
    emit documentControllerChanged();
}

int MathToolsController::count() const noexcept
{
    return static_cast<int>(m_tools.size());
}

int MathToolsController::revision() const noexcept
{
    return m_revision;
}

int MathToolsController::defaultOpacity() noexcept
{
    return features::kDefaultToolOpacity;
}

qreal MathToolsController::minimumLength() noexcept
{
    return kMinLength;
}

qreal MathToolsController::maximumLength() noexcept
{
    return kMaxLength;
}

qreal MathToolsController::minimumRadius() noexcept
{
    return kMinRadius;
}

qreal MathToolsController::maximumRadius() noexcept
{
    return kMaxRadius;
}

qreal MathToolsController::guideBand() const noexcept
{
    const qreal zoom = m_view ? m_view->zoom() : 1.0;
    return kSnapScreenPixels / std::max<qreal>(zoom, 0.01);
}

QSizeF MathToolsController::pageSize() const
{
    return m_view && m_view->content() ? QSizeF(m_view->content()->size()) : QSizeF(1024, 768);
}

int MathToolsController::add(Entry entry, const QString& kind)
{
    m_guides.add(entry.guide());
    m_tools.push_back(std::move(entry));
    updateFilter();
    storeOpenTools();
    emit countChanged();
    emit toolOpened(count() - 1, kind);
    touch();
    return count() - 1;
}

int MathToolsController::openRuler()
{
    const QSizeF page = pageSize();
    const double offset = kNextOffset * static_cast<double>(m_tools.size());
    Entry entry;
    entry.command = QStringLiteral("math.ruler");
    entry.ruler = std::make_unique<features::Ruler>();
    entry.ruler->setFrame(QPointF(page.width() / 2 + offset, page.height() / 2 + offset), kDefaultLength,
                          kDefaultThickness, 0);
    QSettings settings;
    settings.beginGroup(QLatin1String(kRulerGroup));
    entry.ruler->setUnits(
        unitFrom(settings.value(QStringLiteral("topUnit")).toString(), features::RulerUnit::Cm),
        unitFrom(settings.value(QStringLiteral("bottomUnit")).toString(), features::RulerUnit::Inch));
    entry.showAngle = settings.value(QStringLiteral("showAngle"), true).toBool();
    settings.endGroup();
    return add(std::move(entry), QStringLiteral("ruler"));
}

int MathToolsController::openSetSquare()
{
    const QSizeF page = pageSize();
    const double offset = kNextOffset * static_cast<double>(m_tools.size());
    Entry entry;
    entry.command = QStringLiteral("math.set-square");
    entry.square = std::make_unique<features::SetSquare>();
    // Le triangle occupe la boîte [sommet, sommet + côté] : centrée sur la page.
    entry.vertex = QPointF((page.width() - kDefaultSquareSide) / 2 + offset,
                           (page.height() - kDefaultSquareSide) / 2 + offset);
    entry.side = kDefaultSquareSide;
    entry.square->setFrame(entry.vertex, entry.side, entry.angle);
    QSettings settings;
    settings.beginGroup(QLatin1String(kSquareGroup));
    entry.showAngle = settings.value(QStringLiteral("showAngle"), true).toBool();
    settings.endGroup();
    return add(std::move(entry), QStringLiteral("setSquare"));
}

int MathToolsController::openProtractor()
{
    const QSizeF page = pageSize();
    const double offset = kNextOffset * static_cast<double>(m_tools.size());
    Entry entry;
    entry.command = QStringLiteral("math.protractor");
    entry.protractor = std::make_unique<ArcGuide>();
    entry.vertex = QPointF(page.width() / 2 + offset, page.height() / 2 + offset);
    entry.side = kDefaultRadius;
    QSettings settings;
    settings.beginGroup(QLatin1String(kProtractorGroup));
    entry.shape =
        shapeFrom(settings.value(QStringLiteral("shape")).toString(), features::Protractor::Kind::Full);
    entry.arcStyle =
        styleFrom(settings.value(QStringLiteral("arcStyle")).toString(), features::ArcStyle::Arc);
    entry.showAngle = settings.value(QStringLiteral("showAngle"), true).toBool();
    settings.endGroup();
    applyProtractorFrame(entry);
    return add(std::move(entry), QStringLiteral("protractor"));
}

void MathToolsController::close(int index)
{
    if (!valid(index)) {
        return;
    }
    // Retiré des guides avant d'être détruit : un trait guidé en cours finit libre, rien n'est effacé.
    auto it = m_tools.begin() + index;
    m_guides.remove(it->guide());
    m_tools.erase(it);
    updateFilter();
    storeOpenTools();
    emit countChanged();
    emit toolClosed(index);
    touch();
}

void MathToolsController::setFrame(int index, QPointF center, qreal length, qreal thickness, qreal angle)
{
    if (!valid(index)) {
        return;
    }
    Entry& entry = m_tools[static_cast<std::size_t>(index)];
    if (entry.protractor) {
        entry.vertex = center;
        entry.side = std::clamp<double>(length, kMinRadius, kMaxRadius);
        entry.angle = angle;
        applyProtractorFrame(entry);
        touch();
        return;
    }
    const double clamped = std::clamp<double>(length, kMinLength, kMaxLength);
    if (entry.square) {
        entry.vertex = center;
        entry.side = clamped;
        entry.angle = angle;
        entry.square->setFrame(center, clamped, angle);
    } else {
        entry.ruler->setFrame(center, clamped, std::max<double>(thickness, 1), angle);
    }
    touch();
}

void MathToolsController::setUnits(int index, const QString& top, const QString& bottom)
{
    if (!valid(index) || !m_tools[static_cast<std::size_t>(index)].ruler) {
        return; // l'équerre et le rapporteur n'ont pas d'unités
    }
    features::Ruler& ruler = *m_tools[static_cast<std::size_t>(index)].ruler;
    ruler.setUnits(unitFrom(top, ruler.topUnit()), unitFrom(bottom, ruler.bottomUnit()));
    QSettings settings;
    settings.beginGroup(QLatin1String(kRulerGroup));
    settings.setValue(QStringLiteral("topUnit"), unitName(ruler.topUnit()));
    settings.setValue(QStringLiteral("bottomUnit"), unitName(ruler.bottomUnit()));
    settings.endGroup();
    touch();
}

void MathToolsController::setShowAngle(int index, bool on)
{
    if (!valid(index)) {
        return;
    }
    Entry& entry = m_tools[static_cast<std::size_t>(index)];
    entry.showAngle = on;
    QSettings settings;
    settings.beginGroup(QLatin1String(entry.settingsGroup()));
    settings.setValue(QStringLiteral("showAngle"), on);
    settings.endGroup();
    touch();
}

void MathToolsController::setProtractorShape(int index, const QString& shape)
{
    if (!valid(index) || !m_tools[static_cast<std::size_t>(index)].protractor) {
        return;
    }
    Entry& entry = m_tools[static_cast<std::size_t>(index)];
    entry.shape = shapeFrom(shape, entry.shape);
    applyProtractorFrame(entry);
    storeProtractorChoice(entry);
    touch();
}

void MathToolsController::setArcStyle(int index, const QString& style)
{
    if (!valid(index) || !m_tools[static_cast<std::size_t>(index)].protractor) {
        return;
    }
    Entry& entry = m_tools[static_cast<std::size_t>(index)];
    entry.arcStyle = styleFrom(style, entry.arcStyle);
    storeProtractorChoice(entry);
    touch();
}

QVariantMap MathToolsController::toolAt(int index) const
{
    if (!valid(index)) {
        return {};
    }
    const Entry& entry = m_tools[static_cast<std::size_t>(index)];
    if (entry.protractor) {
        return QVariantMap{{QStringLiteral("kind"), QStringLiteral("protractor")},
                           {QStringLiteral("command"), entry.command},
                           {QStringLiteral("centerX"), entry.vertex.x()},
                           {QStringLiteral("centerY"), entry.vertex.y()},
                           {QStringLiteral("length"), entry.side},
                           {QStringLiteral("thickness"), entry.side},
                           {QStringLiteral("angle"), entry.angle},
                           {QStringLiteral("displayAngle"), entry.protractor->protractor.displayAngle()},
                           {QStringLiteral("topUnit"), QStringLiteral("none")},
                           {QStringLiteral("bottomUnit"), QStringLiteral("none")},
                           {QStringLiteral("showAngle"), entry.showAngle},
                           {QStringLiteral("shape"), shapeName(entry.shape)},
                           {QStringLiteral("arcStyle"), styleName(entry.arcStyle)},
                           {QStringLiteral("swept"), entry.sweptShown}};
    }
    if (entry.square) {
        return QVariantMap{{QStringLiteral("kind"), QStringLiteral("setSquare")},
                           {QStringLiteral("command"), entry.command},
                           {QStringLiteral("centerX"), entry.vertex.x()},
                           {QStringLiteral("centerY"), entry.vertex.y()},
                           {QStringLiteral("length"), entry.side},
                           {QStringLiteral("thickness"), entry.side},
                           {QStringLiteral("angle"), entry.angle},
                           {QStringLiteral("displayAngle"), entry.square->displayAngle()},
                           {QStringLiteral("topUnit"), QStringLiteral("none")},
                           {QStringLiteral("bottomUnit"), QStringLiteral("none")},
                           {QStringLiteral("showAngle"), entry.showAngle}};
    }
    const features::Ruler& ruler = *entry.ruler;
    return QVariantMap{{QStringLiteral("kind"), QStringLiteral("ruler")},
                       {QStringLiteral("command"), entry.command},
                       {QStringLiteral("centerX"), ruler.center().x()},
                       {QStringLiteral("centerY"), ruler.center().y()},
                       {QStringLiteral("length"), ruler.length()},
                       {QStringLiteral("thickness"), ruler.width()},
                       {QStringLiteral("angle"), ruler.angleDegrees()},
                       {QStringLiteral("displayAngle"), ruler.displayAngle()},
                       {QStringLiteral("topUnit"), unitName(ruler.topUnit())},
                       {QStringLiteral("bottomUnit"), unitName(ruler.bottomUnit())},
                       {QStringLiteral("showAngle"), entry.showAngle}};
}

QVariantMap MathToolsController::snapAt(QPointF pagePoint) const
{
    input::Sample hover;
    hover.x = pagePoint.x();
    hover.y = pagePoint.y();
    hover.phase = input::Phase::Move;
    const auto snapped = m_guides.snapPreview(hover);
    if (!snapped) {
        return QVariantMap{{QStringLiteral("snapped"), false}};
    }
    return QVariantMap{{QStringLiteral("snapped"), true},
                       {QStringLiteral("x"), snapped->x},
                       {QStringLiteral("y"), snapped->y}};
}

bool MathToolsController::valid(int index) const noexcept
{
    return index >= 0 && static_cast<std::size_t>(index) < m_tools.size();
}

void MathToolsController::updateFilter()
{
    if (!m_view) {
        return;
    }
    const bool wanted = !m_tools.empty();
    if (wanted == m_filterInstalled) {
        return;
    }
    if (wanted) {
        // Chaque échantillon de la voie Tool, avant le Pipeline : appui, déplacements et relâché (naturel ou
        // synthétisé par la vue au dernier point brut). Le guide saisi à l'appui garde le trait jusqu'au
        // relâché.
        m_view->setSampleFilter([this](const input::Sample& sample) { return guide(sample); });
    } else {
        m_view->setSampleFilter({});
    }
    m_filterInstalled = wanted;
}

bool MathToolsController::guidesTool(input::UserId user) const
{
    // Sans hôte d'outils (annotation de bureau), la vue n'a que son stylo.
    return !m_toolHost || m_toolHost->user(user).currentTool() == QLatin1String(kGuidedTool);
}

input::Sample MathToolsController::guide(const input::Sample& sample)
{
    if (!guidesTool(sample.user)) {
        return sample;
    }
    if (sample.phase == input::Phase::Press) {
        m_pendingArc.reset();
        for (Entry& entry : m_tools) {
            if (entry.protractor) {
                entry.protractor->seized = false;
            }
        }
    }
    const input::Sample out = m_guides.filter(sample);
    Entry* const arc = seizedProtractor();
    if (!arc) {
        return out;
    }
    const features::Protractor& protractor = arc->protractor->protractor;
    if (sample.phase == input::Phase::Release) {
        arc->protractor->seized = false;
        // Rien n'est écrit ici : le stylo verse son trait juste après ce relâché ; l'observateur du document
        // le voit et l'insertion suit (watchStrokes).
        if (arc->arcStyle != features::ArcStyle::Arc && protractor.sweptDegrees() >= kMinSweep) {
            PendingArc pending;
            pending.center = arc->vertex;
            pending.radius = arc->side;
            pending.start = protractor.startDegrees();
            pending.sweep = protractor.signedSweptDegrees();
            pending.style = arc->arcStyle;
            m_pendingArc = pending;
        }
    }
    // L'angle balayé se lit sur l'échelle : publié à chaque degré entier.
    const int swept = qRound(protractor.sweptDegrees());
    if (swept != arc->sweptShown) {
        arc->sweptShown = swept;
        touch();
    }
    return out;
}

MathToolsController::Entry* MathToolsController::seizedProtractor() noexcept
{
    for (Entry& entry : m_tools) {
        if (entry.protractor && entry.protractor->seized) {
            return &entry;
        }
    }
    return nullptr;
}

void MathToolsController::applyProtractorFrame(Entry& entry)
{
    entry.protractor->protractor.setFrame(entry.vertex, entry.side, entry.angle, entry.shape);
}

void MathToolsController::storeProtractorChoice(const Entry& entry)
{
    QSettings settings;
    settings.beginGroup(QLatin1String(kProtractorGroup));
    settings.setValue(QStringLiteral("shape"), shapeName(entry.shape));
    settings.setValue(QStringLiteral("arcStyle"), styleName(entry.arcStyle));
    settings.endGroup();
}

void MathToolsController::watchStrokes()
{
    if (!m_document) {
        return;
    }
    // Le Pipeline est synchrone : le relâché filtré, puis commitStroke du stylo, dans la même pile d'appels.
    // Le trait versé complète l'arc armé ; l'insertion est différée (aucune commande pendant la diffusion des
    // changements, contrat de addChangeObserver), donc elle suit le trait dans la pile d'annulation.
    m_strokeObserver = m_document->addChangeObserver([this](const core::Changes& applied,
                                                            DocumentController::ChangeOrigin origin) {
        if (origin != DocumentController::ChangeOrigin::Stroke || !m_pendingArc) {
            return;
        }
        const auto inserted = std::find_if(applied.begin(), applied.end(), [](const core::Change& change) {
            return change.kind == core::Change::Kind::ObjectInserted;
        });
        if (inserted == applied.end()) {
            return;
        }
        PendingArc arc = *m_pendingArc;
        m_pendingArc.reset();
        arc.stroke = inserted->object;
        arc.page = inserted->page;
        QTimer::singleShot(0, this, [this, arc] { insertArc(arc); });
    });
}

void MathToolsController::insertArc(const PendingArc& arc)
{
    // Page quittée entre-temps (trait coupé par un changement de page) : rien n'est ajouté ailleurs.
    if (!m_document || static_cast<std::size_t>(m_document->currentIndex()) != arc.page) {
        return;
    }
    const auto& objects = m_document->document().page(arc.page).objects;
    const auto found = std::find_if(objects.begin(), objects.end(), [&arc](const core::PageObject& object) {
        return object.common.id == arc.stroke;
    });
    const core::Stroke* stroke =
        found != objects.end() ? std::get_if<core::Stroke>(&core::body(*found)) : nullptr;
    if (!stroke) {
        return; // trait déjà retiré
    }
    const std::vector<QPointF> path =
        features::arcPath(arc.center, arc.radius, arc.start, arc.sweep, arc.style);
    core::PageObject added = arcObject(path, arc.style, *stroke);
    // La couche du trait, lue sur lui (F-510 : commitStroke la pose).
    added.common.layer = found->common.layer;
    DocumentInsertions(*m_document).insertObject(std::move(added));
}

void MathToolsController::syncTolerance()
{
    m_guides.setTolerance(guideBand());
    emit guideBandChanged();
}

void MathToolsController::storeOpenTools()
{
    if (!m_toolHost) {
        return;
    }
    std::vector<QString> ids;
    ids.reserve(m_tools.size());
    for (const Entry& entry : m_tools) {
        ids.push_back(entry.command);
    }
    m_toolHost->user().setOpenMathTools(std::move(ids));
}

void MathToolsController::touch()
{
    ++m_revision;
    emit revisionChanged();
}

} // namespace actcraft::ui
