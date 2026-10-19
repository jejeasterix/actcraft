#include "mathtoolscontroller.h"

#include <QQuickItem>
#include <QSettings>

#include <algorithm>
#include <cstddef>
#include <utility>

namespace actcraft::ui {

namespace {

constexpr double kDefaultLength = 600;
constexpr double kDefaultThickness = 80;
constexpr double kDefaultSquareSide = 300;
constexpr double kNextOffset = 24; // une règle de plus s'ouvre décalée, pas sur la précédente
constexpr double kMinLength = 120;
constexpr double kMaxLength = 4000;
const char* const kGuidedTool = "pen"; // tools::PenTool::id()
const char* const kRulerGroup = "mathtools/ruler";
const char* const kSquareGroup = "mathtools/setsquare";

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

} // namespace

features::StrokeGuide* MathToolsController::Entry::guide() const noexcept
{
    return ruler ? static_cast<features::StrokeGuide*>(ruler.get()) : square.get();
}

const char* MathToolsController::Entry::settingsGroup() const noexcept
{
    return ruler ? kRulerGroup : kSquareGroup;
}

MathToolsController::MathToolsController(QObject* parent) : QObject(parent) {}

MathToolsController::~MathToolsController()
{
    // Le filtre tient `this` : jamais laissé à une vue qui survit au contrôleur.
    if (m_view && m_filterInstalled) {
        m_view->setSampleFilter({});
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
        return; // l'équerre n'a pas d'unités
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

QVariantMap MathToolsController::toolAt(int index) const
{
    if (!valid(index)) {
        return {};
    }
    const Entry& entry = m_tools[static_cast<std::size_t>(index)];
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
    return guidesTool(sample.user) ? m_guides.filter(sample) : sample;
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
