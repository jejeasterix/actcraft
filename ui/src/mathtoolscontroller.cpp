#include "mathtoolscontroller.h"

#include "presentationdocument.h"

#include <QQuickItem>
#include <QSettings>
#include <QTimer>
#include <QtMath>

#include <algorithm>
#include <cmath>
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
// Compas : rayon de 5 cm et bras de 250 à l'ouverture (référence) ; le rayon reste sous 1,9 bras (la
// charnière existe) ; pointe sous le centre de la page (la charnière reste sur la page).
constexpr double kDefaultCompassRadius = 5 * features::kPixelsPerCentimetre;
constexpr double kDefaultLeg = 250;
constexpr double kMinLeg = 120;
constexpr double kMaxLeg = 600;
constexpr double kLegStep = 1.2;
constexpr double kRadiusPerLeg = 1.9;
constexpr double kCompassDrop = 80;
constexpr double kCrossHalf = 6;                    // croix du centre : 12 pixels de page
const char* const kPenTool = "pen";                 // tools::PenTool::id()
const char* const kHighlighterTool = "highlighter"; // tools/tool.h
const char* const kEraserTool = "eraser";           // tools::EraserTool::id()
const char* const kRulerGroup = "mathtools/ruler";
const char* const kSquareGroup = "mathtools/setsquare";
const char* const kProtractorGroup = "mathtools/protractor";
const char* const kCompassGroup = "mathtools/compass";

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
// insertAddition). Fermé : les deux rayons d'un seul objet (bout de départ de l'arc, centre, bout d'arrivée),
// de l'épaisseur du trait. Plein : le secteur rempli, sans contour (l'arc est le trait du stylo). `path` :
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

// Croix du centre du compas (« marquer le centre »), après le trait guidé : à sa couleur, de la moitié de son
// épaisseur (au moins 1, comme la référence). Un seul objet : core::Line est une polyligne, qui repasse par
// le centre entre ses deux diagonales de 12 pixels (haut-gauche, bas-droite, centre, haut-droite, bas-gauche)
// ; à quatre points, elle tracerait un troisième segment.
core::PageObject crossObject(QPointF centre, const core::Stroke& stroke)
{
    core::Line cross;
    cross.points = {pointOf(centre + QPointF(-kCrossHalf, -kCrossHalf)),
                    pointOf(centre + QPointF(kCrossHalf, kCrossHalf)), pointOf(centre),
                    pointOf(centre + QPointF(kCrossHalf, -kCrossHalf)),
                    pointOf(centre + QPointF(-kCrossHalf, kCrossHalf))};
    cross.pen.color = stroke.color;
    cross.pen.width = std::max(1.0, stroke.baseWidth / 2);
    core::PageObject marked = std::move(cross);
    return marked;
}

// Direction de la pointe vers la mine, degrés de page (y vers le bas).
double armDegrees(const features::Compass& compass)
{
    const QPointF arm = compass.pencil() - compass.pivot();
    return qRadiansToDegrees(std::atan2(arm.y(), arm.x()));
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
    if (compass) {
        return compass.get();
    }
    return protractor.get();
}

const char* MathToolsController::Entry::settingsGroup() const noexcept
{
    if (ruler) {
        return kRulerGroup;
    }
    if (compass) {
        return kCompassGroup;
    }
    return square ? kSquareGroup : kProtractorGroup;
}

std::optional<input::Sample> MathToolsController::CompassGuide::project(const input::Sample& sample,
                                                                        double tolerance) const
{
    return compass.project(sample, tolerance);
}

std::optional<input::Sample> MathToolsController::CompassGuide::capture(const input::Sample& sample,
                                                                        double tolerance)
{
    const auto onLead = compass.capture(sample, tolerance);
    seized = onLead.has_value();
    return onLead;
}

input::Sample MathToolsController::CompassGuide::follow(const input::Sample& sample)
{
    return compass.follow(sample);
}

MathToolsController::MathToolsController(QObject* parent) : QObject(parent)
{
    connect(this, &MathToolsController::originVisibleChanged, this, &MathToolsController::syncPivot);
    connect(this, &MathToolsController::originChanged, this, &MathToolsController::syncPivot);
}

MathToolsController::~MathToolsController()
{
    // Le filtre et l'observateur tiennent `this` : jamais laissés à une vue ou un document qui survit.
    if (m_view && m_filterInstalled) {
        m_view->setSampleFilter({});
    }
    if (m_view) {
        m_view->setProperty(kRotationPivotProperty, QVariant());
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
    if (m_view) {
        m_view->setProperty(kRotationPivotProperty, QVariant());
    }
    m_view = view;
    if (m_view) {
        m_zoomConnection = connect(m_view, &PageView::zoomChanged, this, &MathToolsController::syncTolerance);
    }
    syncTolerance();
    updateFilter();
    refreshOrigin();
    syncPivot();
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
    m_pending.reset();
    for (QMetaObject::Connection& connection : m_originConnections) {
        disconnect(connection);
    }
    m_document = controller;
    watchStrokes();
    if (controller) {
        // Changement de page affichée seulement (comme PageToolsController) : un document remplacé qui garde
        // son indice est relu par documentReplaced.
        m_originConnections[0] = connect(controller, &DocumentController::pagesChanged, this, [this] {
            if (m_document && m_document->currentIndex() != m_originIndex) {
                refreshOrigin();
            }
        });
        m_originConnections[1] = connect(controller, &DocumentController::documentReplaced, this,
                                         &MathToolsController::refreshOrigin);
        m_originConnections[2] = connect(controller, &DocumentController::documentChanged, this,
                                         &MathToolsController::refreshOrigin);
    }
    refreshOrigin();
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
    if (entry.compass) {
        m_compassGuides.add(entry.guide());
    }
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

int MathToolsController::openCompass()
{
    const QSizeF page = pageSize();
    const double offset = kNextOffset * static_cast<double>(m_tools.size());
    Entry entry;
    entry.command = QStringLiteral("math.compass");
    entry.compass = std::make_unique<CompassGuide>();
    entry.side = kDefaultLeg;
    features::Compass& compass = entry.compass->compass;
    // Pointe et mine de part et d'autre du centre de la page, la charnière au-dessus.
    compass.setPivot(QPointF(page.width() / 2 - kDefaultCompassRadius / 2 + offset,
                             page.height() / 2 + kCompassDrop + offset));
    compass.setRadius(kDefaultCompassRadius);
    compass.setAngle(0);
    QSettings settings;
    settings.beginGroup(QLatin1String(kCompassGroup));
    compass.setMirrored(settings.value(QStringLiteral("mirrored"), false).toBool());
    entry.showAngle = settings.value(QStringLiteral("showAngle"), true).toBool();
    entry.showRadius = settings.value(QStringLiteral("showRadius"), true).toBool();
    entry.markCentre = settings.value(QStringLiteral("markCentre"), false).toBool();
    settings.endGroup();
    return add(std::move(entry), QStringLiteral("compass"));
}

void MathToolsController::close(int index)
{
    if (!valid(index)) {
        return;
    }
    // Retiré des guides avant d'être détruit : un trait guidé en cours finit libre, rien n'est effacé.
    auto it = m_tools.begin() + index;
    m_guides.remove(it->guide());
    if (it->compass) {
        m_compassGuides.remove(it->guide());
    }
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
    if (entry.compass) {
        features::Compass& compass = entry.compass->compass;
        compass.setPivot(center);
        compass.setRadius(std::min<double>(length, kRadiusPerLeg * entry.side)); // puis 20 … 2000 (features)
        compass.setAngle(angle);
        touch();
        return;
    }
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

void MathToolsController::setMirrored(int index, bool on)
{
    Entry* const entry = compassEntry(index);
    if (!entry) {
        return;
    }
    entry->compass->compass.setMirrored(on);
    storeCompassChoice(*entry);
    touch();
}

void MathToolsController::setShowRadius(int index, bool on)
{
    Entry* const entry = compassEntry(index);
    if (!entry) {
        return;
    }
    entry->showRadius = on;
    storeCompassChoice(*entry);
    touch();
}

void MathToolsController::setMarkCentre(int index, bool on)
{
    Entry* const entry = compassEntry(index);
    if (!entry) {
        return;
    }
    entry->markCentre = on;
    storeCompassChoice(*entry);
    touch();
}

void MathToolsController::stepCompassLength(int index, int step)
{
    Entry* const entry = compassEntry(index);
    if (!entry) {
        return;
    }
    const double wanted = step > 0 ? entry->side * kLegStep : entry->side / kLegStep;
    entry->side = step == 0 ? kDefaultLeg : std::clamp(wanted, kMinLeg, kMaxLeg);
    features::Compass& compass = entry->compass->compass;
    compass.setRadius(std::min(compass.radius(), kRadiusPerLeg * entry->side));
    touch();
}

QVariantMap MathToolsController::toolAt(int index) const
{
    if (!valid(index)) {
        return {};
    }
    const Entry& entry = m_tools[static_cast<std::size_t>(index)];
    if (entry.compass) {
        const features::Compass& compass = entry.compass->compass;
        return QVariantMap{{QStringLiteral("kind"), QStringLiteral("compass")},
                           {QStringLiteral("command"), entry.command},
                           {QStringLiteral("centerX"), compass.pivot().x()},
                           {QStringLiteral("centerY"), compass.pivot().y()},
                           {QStringLiteral("length"), compass.radius()},
                           {QStringLiteral("thickness"), entry.side},
                           {QStringLiteral("angle"), armDegrees(compass)},
                           {QStringLiteral("displayAngle"), compass.displayAngle()},
                           {QStringLiteral("topUnit"), QStringLiteral("none")},
                           {QStringLiteral("bottomUnit"), QStringLiteral("none")},
                           {QStringLiteral("showAngle"), entry.showAngle},
                           {QStringLiteral("showRadius"), entry.showRadius},
                           {QStringLiteral("mirrored"), compass.mirrored()},
                           {QStringLiteral("markCentre"), entry.markCentre},
                           {QStringLiteral("radiusText"), compass.radiusText()},
                           {QStringLiteral("leadX"), compass.pencil().x()},
                           {QStringLiteral("leadY"), compass.pencil().y()}};
    }
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
        return QVariantMap{{QStringLiteral("snapped"), false}, {QStringLiteral("tool"), -1}};
    }
    // L'outil dont le guide saisirait le stylo : celui dont la projection est le point d'accroche (la règle
    // du plus proche reste celle de GuideSet).
    int tool = -1;
    const double band = guideBand();
    for (std::size_t i = 0; i < m_tools.size() && tool < 0; ++i) {
        const auto projected = m_tools[i].guide()->project(hover, band);
        if (projected && qAbs(projected->x - snapped->x) < 1e-6 && qAbs(projected->y - snapped->y) < 1e-6) {
            tool = static_cast<int>(i);
        }
    }
    return QVariantMap{{QStringLiteral("snapped"), true},
                       {QStringLiteral("x"), snapped->x},
                       {QStringLiteral("y"), snapped->y},
                       {QStringLiteral("tool"), tool}};
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

MathToolsController::GuidedBy MathToolsController::guidedBy(input::UserId user) const
{
    // Sans hôte d'outils (annotation de bureau), la vue n'a que son stylo.
    if (!m_toolHost) {
        return GuidedBy::All;
    }
    const QString tool = m_toolHost->user(user).currentTool();
    if (tool == QLatin1String(kPenTool) || tool == QLatin1String(kHighlighterTool)) {
        return GuidedBy::All;
    }
    return tool == QLatin1String(kEraserTool) ? GuidedBy::Compass : GuidedBy::None;
}

input::Sample MathToolsController::guide(const input::Sample& sample)
{
    const GuidedBy scope = guidedBy(sample.user);
    if (scope == GuidedBy::None) {
        return sample;
    }
    if (sample.phase == input::Phase::Press) {
        m_pending.reset();
        for (Entry& entry : m_tools) {
            if (entry.protractor) {
                entry.protractor->seized = false;
            }
            if (entry.compass) {
                entry.compass->seized = false;
            }
        }
    }
    const input::Sample out =
        scope == GuidedBy::Compass ? m_compassGuides.filter(sample) : m_guides.filter(sample);
    if (Entry* const held = seizedCompass()) {
        followCompass(*held, sample.phase, scope == GuidedBy::All);
        return out;
    }
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
            PendingAddition pending;
            pending.center = arc->vertex;
            pending.radius = arc->side;
            pending.start = protractor.startDegrees();
            pending.sweep = protractor.signedSweptDegrees();
            pending.style = arc->arcStyle;
            m_pending = pending;
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

MathToolsController::Entry* MathToolsController::seizedCompass() noexcept
{
    for (Entry& entry : m_tools) {
        if (entry.compass && entry.compass->seized) {
            return &entry;
        }
    }
    return nullptr;
}

MathToolsController::Entry* MathToolsController::compassEntry(int index) noexcept
{
    return valid(index) && m_tools[static_cast<std::size_t>(index)].compass
               ? &m_tools[static_cast<std::size_t>(index)]
               : nullptr;
}

void MathToolsController::followCompass(Entry& entry, input::Phase phase, bool poursInk)
{
    const features::Compass& compass = entry.compass->compass;
    if (phase == input::Phase::Release) {
        entry.compass->seized = false;
        // Rien n'est écrit ici : la croix suit le trait que le stylo verse juste après ce relâché
        // (watchStrokes).
        if (poursInk && entry.markCentre && compass.sweptDegrees() >= kMinSweep) {
            PendingAddition pending;
            pending.kind = PendingAddition::Kind::Cross;
            pending.center = compass.pivot();
            m_pending = pending;
        }
    }
    // La mine suit le stylo : le bras est publié à chaque degré entier.
    const int arm = qRound(armDegrees(compass));
    if (arm != entry.sweptShown) {
        entry.sweptShown = arm;
        touch();
    }
}

void MathToolsController::storeCompassChoice(const Entry& entry)
{
    QSettings settings;
    settings.beginGroup(QLatin1String(kCompassGroup));
    settings.setValue(QStringLiteral("mirrored"), entry.compass->compass.mirrored());
    settings.setValue(QStringLiteral("showRadius"), entry.showRadius);
    settings.setValue(QStringLiteral("markCentre"), entry.markCentre);
    settings.endGroup();
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
        if (origin != DocumentController::ChangeOrigin::Stroke || !m_pending) {
            return;
        }
        const auto inserted = std::find_if(applied.begin(), applied.end(), [](const core::Change& change) {
            return change.kind == core::Change::Kind::ObjectInserted;
        });
        if (inserted == applied.end()) {
            return;
        }
        PendingAddition pending = *m_pending;
        m_pending.reset();
        pending.stroke = inserted->object;
        pending.page = inserted->page;
        QTimer::singleShot(0, this, [this, pending] { insertAddition(pending); });
    });
}

void MathToolsController::insertAddition(const PendingAddition& pending)
{
    // Page quittée entre-temps (trait coupé par un changement de page) : rien n'est ajouté ailleurs.
    if (!m_document || static_cast<std::size_t>(m_document->currentIndex()) != pending.page) {
        return;
    }
    const auto& objects = m_document->document().page(pending.page).objects;
    const auto found =
        std::find_if(objects.begin(), objects.end(), [&pending](const core::PageObject& object) {
            return object.common.id == pending.stroke;
        });
    const core::Stroke* stroke =
        found != objects.end() ? std::get_if<core::Stroke>(&core::body(*found)) : nullptr;
    if (!stroke) {
        return; // trait déjà retiré
    }
    core::PageObject added = pending.kind == PendingAddition::Kind::Cross
                                 ? crossObject(pending.center, *stroke)
                                 : arcObject(features::arcPath(pending.center, pending.radius, pending.start,
                                                               pending.sweep, pending.style),
                                             pending.style, *stroke);
    // La couche du trait, lue sur lui (F-510 : commitStroke la pose).
    added.common.layer = found->common.layer;
    DocumentInsertions(*m_document).insertObject(std::move(added));
}

void MathToolsController::syncTolerance()
{
    m_guides.setTolerance(guideBand());
    m_compassGuides.setTolerance(guideBand());
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

bool MathToolsController::originVisible() const noexcept
{
    return m_originVisible;
}

void MathToolsController::setOriginVisible(bool visible)
{
    if (visible == m_originVisible) {
        return;
    }
    m_originVisible = visible;
    emit originVisibleChanged();
}

void MathToolsController::syncPivot()
{
    if (m_view) {
        m_view->setProperty(kRotationPivotProperty, m_originVisible ? QVariant(m_origin) : QVariant());
    }
}

QPointF MathToolsController::origin() const noexcept
{
    return m_origin;
}

bool MathToolsController::moveOrigin(QPointF point)
{
    if (!m_document || !std::isfinite(point.x()) || !std::isfinite(point.y())) {
        return false;
    }
    DocumentInsertions insertions(*m_document);
    const core::Page* page = insertions.shownPage();
    if (!page) {
        return false;
    }
    // Présentation courante recopiée, seule l'origine change : les réglages des outils de page restent.
    core::PagePresentation value = page->presentation;
    if (value.hasOrigin && value.origin.x == point.x() && value.origin.y == point.y()) {
        return false;
    }
    value.hasOrigin = true;
    value.origin = pointOf(point);
    insertions.setPagePresentation(std::move(value));
    return true;
}

void MathToolsController::refreshOrigin()
{
    // Mémoire de la seule page affichée, relue du modèle : jamais gardée d'un indice à l'autre, donc jamais
    // périmée par l'ajout ou le retrait d'une page.
    features::OriginMemory memory(pageSize());
    std::size_t shown = 0;
    m_originIndex = -1;
    if (m_document) {
        const int index = m_document->currentIndex();
        m_originIndex = index;
        if (index >= 0 && static_cast<std::size_t>(index) < m_document->document().pageCount()) {
            shown = static_cast<std::size_t>(index);
            const core::PagePresentation& stored = m_document->document().page(shown).presentation;
            if (stored.hasOrigin) {
                memory.set(shown, QPointF(stored.origin.x, stored.origin.y));
            }
        }
    }
    const QPointF next = memory.at(shown);
    if (next != m_origin) {
        m_origin = next;
        emit originChanged();
    }
}

void MathToolsController::touch()
{
    ++m_revision;
    emit revisionChanged();
}

} // namespace actcraft::ui
