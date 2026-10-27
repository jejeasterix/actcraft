#include <actcraft/core/pagesettings.h>
#include <actcraft/ui/pageoptions.h>

#include <QColor>
#include <QVariantList>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace actcraft::ui {

namespace {

constexpr double kMinSpacing = 1;
constexpr double kMaxSpacing = 4096;

// Réécrit `target` seulement si la valeur est un nombre fini (un NaN passe `std::clamp` sans être borné).
void assignFinite(double& target, const QVariant& value, double low, double high)
{
    const double number = value.toDouble();
    if (std::isfinite(number)) {
        target = std::clamp(number, low, high);
    }
}

constexpr double kNoLow = std::numeric_limits<double>::lowest();
constexpr double kNoHigh = std::numeric_limits<double>::max();

QString key(const char* name)
{
    return QString::fromLatin1(name);
}

QVariantMap levelToMap(const core::GridLevel& level)
{
    QVariantMap map;
    map.insert(key("visible"), level.visible);
    map.insert(key("snap"), level.snap);
    map.insert(key("startX"), level.start.x);
    map.insert(key("startY"), level.start.y);
    map.insert(key("spacingX"), level.spacingX);
    map.insert(key("spacingY"), level.spacingY);
    map.insert(key("color"), QColor::fromRgba(level.color));
    map.insert(key("pattern"), static_cast<int>(level.pattern));
    map.insert(key("angle"), level.angle);
    map.insert(key("line"), static_cast<int>(level.line));
    map.insert(key("alongX"), level.linesAlongX);
    map.insert(key("alongY"), level.linesAlongY);
    return map;
}

// Les clés absentes gardent la valeur de `base` (niveau de même rang, ou niveau neuf).
core::GridLevel levelFromMap(const QVariantMap& map, core::GridLevel base)
{
    const auto has = [&](const char* name) { return map.contains(key(name)); };
    const auto get = [&](const char* name) { return map.value(key(name)); };
    if (has("visible")) {
        base.visible = get("visible").toBool();
    }
    if (has("snap")) {
        base.snap = get("snap").toBool();
    }
    if (has("startX")) {
        assignFinite(base.start.x, get("startX"), kNoLow, kNoHigh);
    }
    if (has("startY")) {
        assignFinite(base.start.y, get("startY"), kNoLow, kNoHigh);
    }
    if (has("spacingX")) {
        assignFinite(base.spacingX, get("spacingX"), kMinSpacing, kMaxSpacing);
    }
    if (has("spacingY")) {
        assignFinite(base.spacingY, get("spacingY"), kMinSpacing, kMaxSpacing);
    }
    if (has("color")) {
        const QColor color = get("color").value<QColor>();
        if (color.isValid()) {
            base.color = color.rgba();
        }
    }
    if (has("pattern")) {
        base.pattern = static_cast<core::GridPattern>(std::clamp(get("pattern").toInt(), 0, 2));
    }
    if (has("angle")) {
        assignFinite(base.angle, get("angle"), kNoLow, kNoHigh);
    }
    if (has("line")) {
        base.line = static_cast<core::GridLine>(std::clamp(get("line").toInt(), 0, 5));
    }
    if (has("alongX")) {
        base.linesAlongX = get("alongX").toBool();
    }
    if (has("alongY")) {
        base.linesAlongY = get("alongY").toBool();
    }
    return base;
}

core::Grid gridFromMap(const QVariantMap& map, const core::Grid& base)
{
    core::Grid grid = base;
    const auto has = [&](const char* name) { return map.contains(key(name)); };
    const auto get = [&](const char* name) { return map.value(key(name)); };
    if (has("visible")) {
        grid.visible = get("visible").toBool();
    }
    if (has("snap")) {
        grid.snapEnabled = get("snap").toBool();
    }
    if (has("above")) {
        grid.drawnAbove = get("above").toBool();
    }
    if (has("previewZoom")) {
        assignFinite(grid.previewZoom, get("previewZoom"), kNoLow, kNoHigh);
    }
    if (has("baseZoom")) {
        assignFinite(grid.baseZoom, get("baseZoom"), kNoLow, kNoHigh);
    }
    if (has("zoomIncrement")) {
        assignFinite(grid.zoomIncrement, get("zoomIncrement"), kNoLow, kNoHigh);
    }
    if (has("levels")) {
        const QVariantList list = get("levels").toList();
        std::vector<core::GridLevel> levels;
        levels.reserve(static_cast<std::size_t>(list.size()));
        for (qsizetype i = 0; i < list.size(); ++i) {
            const auto index = static_cast<std::size_t>(i);
            // Un niveau de même rang garde ses clés inconnues et les valeurs que la carte ne nomme pas.
            const core::GridLevel from = index < base.levels.size() ? base.levels[index] : core::GridLevel{};
            levels.push_back(levelFromMap(list.at(i).toMap(), from));
        }
        grid.levels = std::move(levels);
    }
    return grid;
}

} // namespace

// Talon L4-T1 : fond (L4-T20), extension (L4-T26), zones (L4-T28) restent à remplir ; la grille est ici
// (L4-T13).
PageOptions::PageOptions(QObject* parent) : QObject(parent) {}

DocumentController* PageOptions::documentController() const noexcept
{
    return m_controller.data();
}

void PageOptions::setDocumentController(DocumentController* controller)
{
    if (m_controller == controller) {
        return;
    }
    if (m_controller) {
        disconnect(m_controller, nullptr, this, nullptr);
    }
    m_controller = controller;
    if (m_controller) {
        // La page affichée change ou est touchée : les réglages se relisent.
        connect(m_controller, &DocumentController::pagesChanged, this, &PageOptions::changed);
        connect(m_controller, &DocumentController::pageContentChanged, this, [this](int page) {
            if (m_controller && page == m_controller->currentIndex()) {
                emit changed();
            }
        });
    }
    emit wiringChanged();
    emit changed();
}

core::Grid PageOptions::currentGrid() const
{
    if (!m_controller || m_controller->pageCount() == 0) {
        return {};
    }
    return m_controller->document().page(static_cast<std::size_t>(m_controller->currentIndex())).grid;
}

void PageOptions::applyGrid(core::Grid grid)
{
    if (!m_controller || m_controller->pageCount() == 0) {
        return;
    }
    m_controller->execute(std::make_unique<core::SetGridCommand>(
        static_cast<std::size_t>(m_controller->currentIndex()), std::move(grid), m_designer));
}

bool PageOptions::gridVisible() const
{
    return currentGrid().visible;
}

bool PageOptions::gridSnap() const
{
    return currentGrid().snapEnabled;
}

bool PageOptions::gridAbove() const
{
    return currentGrid().drawnAbove;
}

QVariantMap PageOptions::grid() const
{
    const core::Grid grid = currentGrid();
    QVariantMap map;
    map.insert(key("visible"), grid.visible);
    map.insert(key("snap"), grid.snapEnabled);
    map.insert(key("above"), grid.drawnAbove);
    map.insert(key("previewZoom"), grid.previewZoom);
    map.insert(key("baseZoom"), grid.baseZoom);
    map.insert(key("zoomIncrement"), grid.zoomIncrement);
    QVariantList levels;
    for (const core::GridLevel& level : grid.levels) {
        levels.append(levelToMap(level));
    }
    map.insert(key("levels"), levels);
    return map;
}

QVariantMap PageOptions::background() const
{
    return {};
}

qreal PageOptions::pageHeight() const
{
    return 0;
}

QString PageOptions::zones() const
{
    return QStringLiteral("0x0");
}

void PageOptions::toggleGridHidden()
{
    core::Grid grid = currentGrid();
    grid.visible = !grid.visible;
    applyGrid(std::move(grid));
}

void PageOptions::toggleSnap()
{
    core::Grid grid = currentGrid();
    grid.snapEnabled = !grid.snapEnabled;
    applyGrid(std::move(grid));
}

void PageOptions::beginDesigner()
{
    // Un réglage fait juste avant l'ouverture ne fusionne pas avec la séance.
    if (m_controller) {
        m_controller->closeMerge();
    }
    m_designer = true;
}

void PageOptions::setGrid(const QVariantMap& grid)
{
    applyGrid(gridFromMap(grid, currentGrid()));
}

void PageOptions::addLevel()
{
    core::Grid grid = currentGrid();
    grid.levels.push_back(core::GridLevel{});
    applyGrid(std::move(grid));
}

void PageOptions::removeLevel(int level)
{
    core::Grid grid = currentGrid();
    // Au moins un niveau reste.
    if (level < 0 || static_cast<std::size_t>(level) >= grid.levels.size() || grid.levels.size() <= 1) {
        return;
    }
    grid.levels.erase(grid.levels.begin() + level);
    applyGrid(std::move(grid));
}

void PageOptions::resetGrid()
{
    const core::Grid old = currentGrid();
    core::Grid grid;
    grid.visible = old.visible;
    grid.snapEnabled = false;
    core::GridLevel level;
    level.snap = false;
    grid.levels.push_back(level);
    applyGrid(std::move(grid));
}

void PageOptions::stepSpacing(int sign)
{
    core::Grid grid = currentGrid();
    const double factor = sign >= 0 ? 1 + grid.zoomIncrement : 1 - grid.zoomIncrement;
    if (!std::isfinite(factor)) {
        return; // incrément illisible dans le fichier : pas de pas NaN
    }
    for (core::GridLevel& level : grid.levels) {
        level.spacingX = std::clamp(level.spacingX * factor, kMinSpacing, kMaxSpacing);
        level.spacingY = std::clamp(level.spacingY * factor, kMinSpacing, kMaxSpacing);
    }
    applyGrid(std::move(grid));
}

void PageOptions::endDesigner()
{
    if (!m_designer) {
        return;
    }
    m_designer = false;
    if (m_controller) {
        m_controller->closeMerge();
    }
}

void PageOptions::setBackground(const QVariantMap&) {}

void PageOptions::extendPage() {}

void PageOptions::setZones(const QString&) {}

} // namespace actcraft::ui
