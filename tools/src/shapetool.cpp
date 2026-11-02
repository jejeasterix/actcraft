#include <actcraft/core/edits.h>
#include <actcraft/core/gridsnap.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/tools/shapetool.h>
#include <actcraft/tools/userstate.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace actcraft::tools {

ShapeTool::ShapeTool(ToolContext& context) : m_context(context) {}

QString ShapeTool::id() const
{
    return QStringLiteral("shape");
}

// Les formes sans boîte (ligne et courbe brisées, main levée) ont leur geste dans onChainSample.
bool ShapeTool::boxGesture() const
{
    const std::string preset = m_context.user.shapeSettings().preset.toStdString();
    return core::shapeGesture(preset) == core::ShapeGesture::Box;
}

// Écart en pixels d'écran (donc brut, avant accrochage) : en deçà du seuil, le tracé est un clic (F-241).
bool ShapeTool::dragged() const
{
    if (!m_press) {
        return false;
    }
    const double zoom = m_context.view.zoom();
    const double distance = std::hypot(m_current.x - m_press->x, m_current.y - m_press->y);
    return distance >= core::kShapeMinDragScreen / (zoom > 0 ? zoom : 1.0);
}

core::PointF ShapeTool::snapped(core::PointF point) const
{
    const core::Page& page = m_context.document.page();
    return core::nearestGridPoint(page.grid, point, page.width, page.height).value_or(point);
}

core::ShapeStyle ShapeTool::style() const
{
    const ShapeSettings settings = m_context.user.shapeSettings();
    const InkSettings outline = m_context.user.settings(QStringLiteral("shape"));
    core::ShapeStyle style;
    style.outline = outline.color;
    style.width = outline.width;
    style.filled = settings.filled;
    style.fill = settings.fill;
    return style;
}

std::optional<core::PageObject> ShapeTool::currentObject() const
{
    if (!m_press) {
        return std::nullopt;
    }
    const ShapeSettings settings = m_context.user.shapeSettings();
    const core::ShapeStyle style = this->style();

    const std::string id = settings.preset.toStdString();
    const core::ShapePreset* preset = core::findShapePreset(id);
    const core::PointF from = snapped(*m_press);
    const core::PointF to = snapped(m_current);
    if (preset && preset->family == core::ShapeFamily::Line) {
        const auto [a, b] = core::dragLine(from, to, *preset, settings.fromCenter, false);
        return core::makeLineObject(id, a, b, style);
    }
    const bool square = preset && preset->square;
    return core::makeShapeObject(id, core::dragBox(from, to, settings.fromCenter, square), style);
}

void ShapeTool::showPreview()
{
    if (!dragged()) {
        m_context.lift.clear();
        return;
    }
    if (std::optional<core::PageObject> object = currentObject()) {
        std::vector<core::PageObject> shown;
        shown.push_back(std::move(*object));
        m_context.lift.show(std::move(shown));
    }
}

void ShapeTool::showChain(const std::vector<core::PointF>& points)
{
    std::optional<core::PageObject> object =
        core::makeChainObject(m_context.user.shapeSettings().preset.toStdString(), points, style());
    if (!object) {
        m_context.lift.clear();
        return;
    }
    std::vector<core::PageObject> shown;
    shown.push_back(std::move(*object));
    m_context.lift.show(std::move(shown));
}

void ShapeTool::resetChain()
{
    m_chain.clear();
    m_press.reset();
    m_context.lift.clear();
}

// Main levée : l'emprise du geste doit dépasser le seuil de clic ; ligne posée au clic : toujours vrai (deux
// points distincts exigés par makeChainObject).
bool ShapeTool::chainExtentReached() const
{
    if (m_chain.empty()) {
        return false;
    }
    if (core::shapeGesture(m_context.user.shapeSettings().preset.toStdString()) !=
        core::ShapeGesture::Freehand) {
        return true;
    }
    double minX = m_chain.front().x;
    double maxX = minX;
    double minY = m_chain.front().y;
    double maxY = minY;
    for (const core::PointF& p : m_chain) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }
    const double zoom = m_context.view.zoom();
    return std::max(maxX - minX, maxY - minY) >= core::kShapeMinDragScreen / (zoom > 0 ? zoom : 1.0);
}

void ShapeTool::finishChain()
{
    std::optional<core::PageObject> object;
    if (chainExtentReached()) {
        object = core::makeChainObject(m_context.user.shapeSettings().preset.toStdString(), m_chain, style());
    }
    resetChain();
    if (object) {
        m_context.document.execute(
            std::make_unique<core::AddObjectCommand>(m_context.document.pageIndex(), std::move(*object)));
    }
}

void ShapeTool::onChainSample(const input::Sample& sample)
{
    const core::PointF point{sample.x, sample.y};
    const double zoom = m_context.view.zoom() > 0 ? m_context.view.zoom() : 1.0;
    const bool clicks =
        core::shapeGesture(m_context.user.shapeSettings().preset.toStdString()) == core::ShapeGesture::Clicks;
    switch (sample.phase) {
    case input::Phase::Press: {
        if (clicks) {
            const core::PointF vertex = snapped(point);
            const bool doubleClick = !m_chain.empty() &&
                                     std::hypot(vertex.x - m_chain.back().x, vertex.y - m_chain.back().y) <
                                         core::kChainDoubleClickScreen / zoom &&
                                     sample.timestampNs - m_lastPressNs < core::kChainDoubleClickNs;
            m_lastPressNs = sample.timestampNs;
            if (doubleClick) {
                finishChain();
                return;
            }
            m_press = point;
            m_current = point;
            m_chain.push_back(vertex);
            showChain(m_chain);
            return;
        }
        m_press = point; // main levée : points bruts, sans accrochage
        m_current = point;
        m_chain = {point};
        m_context.lift.clear();
        return;
    }
    case input::Phase::Move: {
        if (!m_press) {
            return;
        }
        m_current = point;
        if (clicks) {
            if (dragged()) { // glissé bouton tenu : le prochain sommet suit le pointeur
                std::vector<core::PointF> shown = m_chain;
                shown.push_back(snapped(point));
                showChain(shown);
            }
            return;
        }
        if (std::hypot(point.x - m_chain.back().x, point.y - m_chain.back().y) >= 1.0 / zoom) {
            m_chain.push_back(point);
            showChain(m_chain);
        }
        return;
    }
    case input::Phase::Release: {
        if (!m_press) {
            return;
        }
        m_current = point;
        if (clicks) {
            if (dragged()) { // un glissé trace un segment : le relâché pose un sommet
                m_chain.push_back(snapped(point));
            }
            m_press.reset();
            showChain(m_chain);
            return;
        }
        if (std::hypot(point.x - m_chain.back().x, point.y - m_chain.back().y) >= 1.0 / zoom) {
            m_chain.push_back(point);
        }
        finishChain();
        return;
    }
    }
}

void ShapeTool::onSample(const input::Sample& sample)
{
    if (!boxGesture()) {
        onChainSample(sample);
        return;
    }
    const core::PointF point{sample.x, sample.y};
    switch (sample.phase) {
    case input::Phase::Press:
        m_press = point;
        m_current = point;
        return;
    case input::Phase::Move:
        if (!m_press) {
            return;
        }
        m_current = point;
        showPreview();
        return;
    case input::Phase::Release: {
        if (!m_press) {
            return;
        }
        m_current = point;
        const bool place = dragged();
        std::optional<core::PageObject> object;
        if (place) {
            object = currentObject();
        }
        m_press.reset();
        m_context.lift.clear();
        if (object) {
            m_context.document.execute(
                std::make_unique<core::AddObjectCommand>(m_context.document.pageIndex(), std::move(*object)));
        }
        return;
    }
    }
}

bool ShapeTool::isBusy() const noexcept
{
    return m_press.has_value() || !m_chain.empty();
}

// Changement d'outil ou de page, Annuler, Échap (par tool.select) : la ligne commencée est versée si elle a
// deux points distincts, comme le trait du stylo ; sinon rien.
void ShapeTool::cancel()
{
    if (!m_chain.empty()) {
        finishChain();
        return;
    }
    m_press.reset();
    m_context.lift.clear();
}

void ShapeTool::abandon()
{
    resetChain();
}

bool ShapeTool::onKey(int, int)
{
    return false;
}

} // namespace actcraft::tools
