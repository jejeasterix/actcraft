#include <actcraft/core/pagesettings.h>
#include <actcraft/core/pagezones.h>
#include <actcraft/formats/nativejson.h>
#include <actcraft/ui/pageoptions.h>

#include <QColor>
#include <QStringList>
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

// Deux fonds identiques : ni étape d'annulation ni redessin.
bool sameBackground(const core::Background& a, const core::Background& b)
{
    return a.image == b.image && a.fill.kind == b.fill.kind && a.fill.color == b.fill.color &&
           a.fill.color2 == b.fill.color2 && a.fill.gradient == b.fill.gradient &&
           a.fill.pattern == b.fill.pattern && a.fill.opaqueBackground == b.fill.opaqueBackground &&
           a.fill.backgroundColor == b.fill.backgroundColor;
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
    if (!m_controller || m_controller->pageCount() == 0) {
        return {};
    }
    const core::Background& background =
        m_controller->document().page(static_cast<std::size_t>(m_controller->currentIndex())).background;
    const core::Fill& fill = background.fill;
    QVariantMap map;
    // « other » : un fond que la boîte n'offre pas (aucun, motif) ; l'image reste dans « image ».
    map.insert(key("kind"), fill.kind == core::FillKind::Gradient ? key("gradient")
                            : fill.kind == core::FillKind::Solid  ? key("color")
                                                                  : key("other"));
    map.insert(key("color"), QColor::fromRgba(fill.color));
    map.insert(key("color2"), QColor::fromRgba(fill.color2));
    map.insert(key("gradient"), formats::gradientKindName(fill.gradient));
    map.insert(key("image"), QString::fromStdString(background.image));
    return map;
}

qreal PageOptions::pageHeight() const
{
    if (!m_controller || m_controller->pageCount() == 0) {
        return 0;
    }
    return m_controller->document().page(static_cast<std::size_t>(m_controller->currentIndex())).height;
}

QString PageOptions::zones() const
{
    if (!m_controller || m_controller->pageCount() == 0) {
        return QStringLiteral("0x0");
    }
    const core::PageZones& zones =
        m_controller->document().page(static_cast<std::size_t>(m_controller->currentIndex())).zones;
    return QStringLiteral("%1x%2").arg(zones.horizontal).arg(zones.vertical);
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

void PageOptions::setBackground(const QVariantMap& background)
{
    if (!m_controller || m_controller->pageCount() == 0) {
        return;
    }
    const std::size_t page = static_cast<std::size_t>(m_controller->currentIndex());
    const core::Background current = m_controller->document().page(page).background;
    core::Background next = current;
    const auto color = [&](const char* name, core::Argb fallback) {
        const QColor value = background.value(key(name)).value<QColor>();
        return value.isValid() ? static_cast<core::Argb>(value.rgba()) : fallback;
    };
    const QString kind = background.value(key("kind")).toString();
    if (kind == QLatin1String("gradient")) {
        core::Fill fill = current.fill.kind == core::FillKind::Gradient ? current.fill : core::Fill{};
        fill.kind = core::FillKind::Gradient;
        fill.color = color("color", fill.color);
        fill.color2 = color("color2", fill.color2);
        fill.gradient = formats::gradientKindFromName(background.value(key("gradient")).toString())
                            .value_or(fill.gradient);
        next.fill = fill;
        next.image.clear();
    } else if (kind == QLatin1String("color")) {
        next.fill = core::solidFill(color("color", current.fill.color));
        next.image.clear();
    } else {
        return; // l'image viendra avec les ressources du lot 7 (H4-18) ; un genre inconnu ne change rien
    }
    if (sameBackground(current, next)) {
        return;
    }
    m_controller->execute(std::make_unique<core::SetBackgroundCommand>(page, std::move(next)));
}

void PageOptions::extendPage()
{
    if (!m_controller || m_controller->pageCount() == 0) {
        return;
    }
    // Une étape par clic (F-301 a) : le pas est cumulé par la commande du noyau (D10, L4-O1).
    m_controller->execute(
        std::make_unique<core::ExtendPageCommand>(static_cast<std::size_t>(m_controller->currentIndex())));
}

void PageOptions::setZones(const QString& zones)
{
    if (!m_controller || m_controller->pageCount() == 0) {
        return;
    }
    // « <cadres en largeur>x<cadres en hauteur> » : deux entiers positifs ou nuls, sinon rien ne change.
    const QStringList parts = zones.split(QLatin1Char('x'));
    bool okAcross = false;
    bool okDown = false;
    const int across = parts.size() == 2 ? parts.at(0).trimmed().toInt(&okAcross) : 0;
    const int down = parts.size() == 2 ? parts.at(1).trimmed().toInt(&okDown) : 0;
    if (!okAcross || !okDown || across < 0 || down < 0) {
        return;
    }
    const auto page = static_cast<std::size_t>(m_controller->currentIndex());
    // La taille d'un cadre déjà posée est gardée : ce réglage ne donne que des nombres de cadres.
    core::PageZones next = m_controller->document().page(page).zones;
    next.horizontal = across;
    next.vertical = down;
    next = core::clampZones(next);
    const core::PageZones& current = m_controller->document().page(page).zones;
    if (next.horizontal == current.horizontal && next.vertical == current.vertical) {
        return; // rien à changer : ni étape ni redessin
    }
    m_controller->execute(std::make_unique<core::SetPageZonesCommand>(page, next));
}

} // namespace actcraft::ui
