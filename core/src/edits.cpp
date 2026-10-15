#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>

#include <algorithm>
#include <cassert>
#include <limits>
#include <utility>

namespace actcraft::core {

namespace {

struct Box
{
    double minX = std::numeric_limits<double>::max();
    double minY = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double maxY = std::numeric_limits<double>::lowest();

    void add(PointF p) noexcept
    {
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
    }
    void addRect(const RectF& r) noexcept
    {
        add({r.x, r.y});
        add({r.x + r.width, r.y + r.height});
    }
    bool empty() const noexcept { return minX > maxX; }
    RectF grown(double margin) const noexcept
    {
        return {minX - margin, minY - margin, maxX - minX + 2 * margin, maxY - minY + 2 * margin};
    }
};

// Rectangle local (coordonnées de l'objet) ; marge = demi-épaisseur du trait.
RectF localBounds(const PageObject& object) noexcept
{
    Box box;
    double margin = 0;
    if (const auto* s = std::get_if<Shape>(&object)) {
        if (s->kind == ShapeKind::Path) {
            for (const auto& path : s->paths) {
                for (const PointF& p : path) {
                    box.add(p);
                }
            }
            if (box.empty()) {
                box.addRect(s->bounds);
            }
            margin = s->pen.style == PenStyle::None ? 0 : s->pen.width / 2;
        } else if (s->kind == ShapeKind::Polygon) {
            for (const PointF& p : s->polygon) {
                box.add(p);
            }
            margin = s->outlineWidth / 2;
        } else {
            box.addRect(s->bounds);
            margin = s->outlineWidth / 2;
        }
    } else if (const auto* im = std::get_if<ImageObject>(&object)) {
        box.addRect(im->bounds);
    } else if (const auto* st = std::get_if<Stroke>(&object)) {
        for (const StrokePoint& p : st->points) {
            box.add({p.x, p.y});
            margin = std::max(margin, strokeWidthAt(st->baseWidth, p.pressure) / 2);
        }
    } else if (const auto* l = std::get_if<Line>(&object)) {
        for (const PointF& p : l->points) {
            box.add(p);
        }
        margin = l->pen.width / 2;
    } else if (const auto* t = std::get_if<Text>(&object)) {
        box.addRect(t->bounds);
    } else if (const auto* m = std::get_if<Media>(&object)) {
        box.addRect(m->bounds);
    } else if (const auto* c = std::get_if<Connector>(&object)) {
        box.add(c->firstEnd);
        box.add(c->secondEnd);
        margin = c->pen.width / 2;
    }
    return box.empty() ? RectF{} : box.grown(margin);
}

} // namespace

std::size_t insertOnTopOfLayer(Page& page, PageObject object)
{
    const Layer layer = object.common.layer;
    const auto it = std::find_if(page.objects.begin(), page.objects.end(),
                                 [layer](const PageObject& o) { return o.common.layer > layer; });
    const auto index = static_cast<std::size_t>(it - page.objects.begin());
    page.objects.insert(it, std::move(object));
    return index;
}

std::size_t indexOf(const Page& page, ObjectId id) noexcept
{
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        if (page.objects[i].common.id == id) {
            return i;
        }
    }
    return page.objects.size();
}

std::size_t depthInLayer(const Page& page, ObjectId id) noexcept
{
    const std::size_t index = indexOf(page, id);
    if (index == page.objects.size()) {
        return index;
    }
    std::size_t depth = 0;
    for (std::size_t i = 0; i < index; ++i) {
        if (page.objects[i].common.layer == page.objects[index].common.layer) {
            ++depth;
        }
    }
    return depth;
}

bool isLayerOrdered(const Page& page) noexcept
{
    for (std::size_t i = 1; i < page.objects.size(); ++i) {
        if (page.objects[i].common.layer < page.objects[i - 1].common.layer) {
            return false;
        }
    }
    return true;
}

RectF boundsOnPage(const PageObject& object) noexcept
{
    const RectF local = localBounds(object);
    if (local.width == 0 && local.height == 0 && local.x == 0 && local.y == 0) {
        return local;
    }
    const Transform& t = object.common.transform;
    Box box;
    box.add(t.map({local.x, local.y}));
    box.add(t.map({local.x + local.width, local.y}));
    box.add(t.map({local.x, local.y + local.height}));
    box.add(t.map({local.x + local.width, local.y + local.height}));
    return box.grown(0);
}

AddObjectCommand::AddObjectCommand(std::size_t page, PageObject object)
    : m_page(page), m_object(std::move(object))
{}

void AddObjectCommand::redo(Document& document, Changes& changes)
{
    if (m_object.common.id == 0) {
        m_object.common.id = document.allocateId();
        if (m_object.common.id == 0) {
            return; // identifiants épuisés : jamais d'objet d'identifiant 0
        }
    }
    insertOnTopOfLayer(document.page(m_page), m_object);
    m_inserted = true;
    changes.push_back(
        Change{Change::Kind::ObjectInserted, m_page, m_object.common.id, boundsOnPage(m_object)});
}

void AddObjectCommand::undo(Document& document, Changes& changes)
{
    if (!m_inserted) {
        return;
    }
    Page& page = document.page(m_page);
    const std::size_t index = indexOf(page, m_object.common.id);
    assert(index < page.objects.size());
    page.objects.erase(page.objects.begin() + static_cast<std::ptrdiff_t>(index));
    m_inserted = false;
    changes.push_back(
        Change{Change::Kind::ObjectRemoved, m_page, m_object.common.id, boundsOnPage(m_object)});
}

ObjectId AddObjectCommand::id() const noexcept
{
    return m_object.common.id;
}

AddPageCommand::AddPageCommand(std::size_t index, double width, double height)
    : m_index(index), m_width(width), m_height(height)
{}

void AddPageCommand::redo(Document& document, Changes& changes)
{
    Page page;
    page.width = m_width;
    page.height = m_height;
    document.insertPage(m_index, std::move(page));
    changes.push_back(Change{Change::Kind::PageInserted, m_index, 0, RectF{}});
}

void AddPageCommand::undo(Document& document, Changes& changes)
{
    document.takePage(m_index);
    changes.push_back(Change{Change::Kind::PageRemoved, m_index, 0, RectF{}});
}

} // namespace actcraft::core
