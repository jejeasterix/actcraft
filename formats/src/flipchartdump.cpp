#include <actcraft/formats/flipchartdump.h>

#include <QJsonArray>
#include <QJsonDocument>

#include <algorithm>
#include <cmath>
#include <map>
#include <vector>

namespace actcraft::formats::flipchart {

using namespace Qt::StringLiterals;
using namespace actcraft::core;

namespace {

using IndexOf = std::map<ObjectId, int>;

// « #rrggbbaa » en minuscules, comme le f"{r:02x}" de l'oracle.
QString rgba(Argb c)
{
    return u"#%1%2%3%4"_s.arg((c >> 16) & 0xFFu, 2, 16, QLatin1Char('0'))
        .arg((c >> 8) & 0xFFu, 2, 16, QLatin1Char('0'))
        .arg(c & 0xFFu, 2, 16, QLatin1Char('0'))
        .arg((c >> 24) & 0xFFu, 2, 16, QLatin1Char('0'));
}

QString utf8(const std::string& s)
{
    return QString::fromStdString(s); // les clés de ressources sont déjà le SHA-256 des octets
}

QJsonArray rect(const RectF& r)
{
    return QJsonArray{r.x, r.y, r.width, r.height};
}

QJsonArray xy(double x, double y)
{
    return QJsonArray{x, y};
}

template <typename Points> QJsonArray points(const Points& list)
{
    QJsonArray out;
    for (const auto& p : list) {
        out.append(xy(p.x, p.y));
    }
    return out;
}

// Indice dans la liste exportée, −1 sans référence ou si l'objet n'est pas sur la page.
int indexIn(const IndexOf& indexOf, ObjectId id)
{
    if (id == 0) {
        return -1;
    }
    const auto it = indexOf.find(id);
    return it == indexOf.end() ? -1 : it->second;
}

// Style 0 sans largeur, quel que soit le style rangé dans le modèle.
QJsonObject pen(const Outline& o)
{
    return QJsonObject{{u"color"_s, rgba(o.color)},
                       {u"width"_s, o.width},
                       {u"style"_s, o.width <= 0 ? 0 : static_cast<int>(o.style)},
                       {u"cap"_s, static_cast<int>(o.cap)},
                       {u"join"_s, static_cast<int>(o.join)}};
}

QJsonObject brush(const Fill& f)
{
    QString kind = u"solid"_s;
    switch (f.kind) {
    case FillKind::None:
        kind = u"none"_s;
        break;
    case FillKind::Gradient:
        kind = u"gradient"_s;
        break;
    case FillKind::Pattern:
        kind = u"pattern"_s;
        break;
    case FillKind::Solid:
        break;
    }
    const bool gradient = f.kind == FillKind::Gradient;
    return QJsonObject{{u"kind"_s, kind},
                       {u"color"_s, rgba(f.color)},
                       {u"color2"_s, gradient ? rgba(f.color2) : QString()},
                       {u"gradient"_s, gradient ? static_cast<int>(f.gradient) : 0},
                       {u"pattern"_s, f.kind == FillKind::Pattern ? static_cast<int>(f.pattern) : 0},
                       {u"opaque"_s, f.opaqueBackground},
                       {u"hatch"_s, rgba(f.backgroundColor)}};
}

QJsonObject span(const TextSpan& s)
{
    return QJsonObject{{u"text"_s, utf8(s.text)},
                       {u"font"_s, utf8(s.fontFamily)},
                       {u"size"_s, s.pointSize},
                       {u"weight"_s, static_cast<int>(s.weight)},
                       {u"italic"_s, s.italic},
                       {u"underline"_s, s.underline},
                       {u"strikeOut"_s, s.strikeOut},
                       {u"color"_s, rgba(s.color)},
                       {u"baseline"_s, xy(s.baseline.x, s.baseline.y)},
                       {u"bounds"_s, rect(s.bounds)}};
}

QJsonObject level(const GridLevel& l)
{
    return QJsonObject{{u"visible"_s, l.visible},
                       {u"snap"_s, l.snap},
                       {u"start"_s, xy(l.start.x, l.start.y)},
                       {u"spacingX"_s, l.spacingX},
                       {u"spacingY"_s, l.spacingY},
                       {u"color"_s, rgba(l.color)},
                       {u"pattern"_s, static_cast<int>(l.pattern)},
                       {u"angle"_s, l.angle},
                       {u"line"_s, static_cast<int>(l.line)},
                       {u"linesAlongX"_s, l.linesAlongX},
                       {u"linesAlongY"_s, l.linesAlongY}};
}

QJsonObject object(const PageObject& o, const IndexOf& indexOf)
{
    const ObjectCommon& c = o.common;
    const Transform& t = c.transform;
    // Couche du .flipchart (0 dessous, 1 milieu, 2 dessus) : le modèle numérote Bottom 1, Middle 2, Top 3.
    QJsonObject d{{u"layer"_s, std::max(0, static_cast<int>(c.layer) - 1)},
                  {u"name"_s, utf8(c.name)},
                  {u"visible"_s, c.visible},
                  {u"locked"_s, c.locked},
                  {u"translucency"_s, static_cast<int>(c.translucency)},
                  {u"movable"_s, c.restrictions.movable},
                  {u"resizable"_s, c.restrictions.resizable},
                  {u"rotatable"_s, c.restrictions.rotatable},
                  {u"label"_s, utf8(c.label)},
                  {u"matrix"_s, QJsonArray{t.m11, t.m12, t.m21, t.m22, t.dx, t.dy}}};
    if (const auto* s = std::get_if<Stroke>(&o)) {
        d.insert(u"kind"_s, u"annotation"_s);
        d.insert(u"pen"_s, QJsonObject{{u"color"_s, rgba(s->color)},
                                       {u"width"_s, s->baseWidth},
                                       {u"cap"_s, static_cast<int>(s->cap)}});
        d.insert(u"strokeMode"_s, static_cast<int>(s->kind));
        d.insert(u"points"_s, points(s->points));
    } else if (const auto* l = std::get_if<Line>(&o)) {
        d.insert(u"kind"_s, u"line"_s);
        d.insert(u"pen"_s, pen(l->pen));
        d.insert(u"points"_s, points(l->points));
    } else if (const auto* sh = std::get_if<Shape>(&o)) {
        d.insert(u"kind"_s, u"shape"_s);
        d.insert(u"pen"_s, pen(sh->pen));
        d.insert(u"brush"_s, brush(sh->brush));
        QJsonArray paths;
        for (const auto& path : sh->paths) {
            paths.append(points(path));
        }
        d.insert(u"paths"_s, paths);
        d.insert(u"open"_s, sh->open);
        d.insert(u"winding"_s, sh->windingFill);
        d.insert(u"bounds"_s, rect(sh->bounds));
    } else if (const auto* im = std::get_if<ImageObject>(&o)) {
        d.insert(u"kind"_s, u"image"_s);
        d.insert(u"bounds"_s, rect(im->bounds));
        d.insert(u"image"_s, utf8(im->resource));
        d.insert(u"transparent"_s, im->hasKeyColor);
        d.insert(u"keyColor"_s, im->hasKeyColor ? rgba(im->keyColor) : QString());
        d.insert(u"pixels"_s, im->resource.empty() ? QJsonArray{0, 0}
                                                   : QJsonArray{static_cast<int>(im->sourceWidth),
                                                                static_cast<int>(im->sourceHeight)});
    } else if (const auto* tx = std::get_if<Text>(&o)) {
        d.insert(u"kind"_s, u"text"_s);
        d.insert(u"bounds"_s, rect(tx->bounds));
        d.insert(u"html"_s, tx->html.empty()
                                ? QString()
                                : QString::fromStdString(sha256Hex(QByteArray::fromStdString(tx->html))));
        QJsonArray runs;
        for (const TextSpan& run : tx->runs) {
            runs.append(span(run));
        }
        d.insert(u"runs"_s, runs);
    } else if (const auto* m = std::get_if<Media>(&o)) {
        d.insert(u"kind"_s, u"media"_s);
        d.insert(u"bounds"_s, rect(m->bounds));
        d.insert(u"image"_s, utf8(m->placeholder));
        d.insert(u"media"_s, utf8(m->media));
        d.insert(u"file"_s, utf8(m->fileName));
        d.insert(u"playOnOpen"_s, m->playOnOpen);
        d.insert(u"loop"_s, m->loop);
    } else if (const auto* g = std::get_if<Group>(&o)) {
        d.insert(u"kind"_s, u"group"_s);
        std::vector<int> members;
        for (const ObjectId id : g->members) {
            if (const int i = indexIn(indexOf, id); i >= 0) {
                members.push_back(i);
            }
        }
        std::sort(members.begin(), members.end());
        QJsonArray list;
        for (const int i : members) {
            list.append(i);
        }
        d.insert(u"members"_s, list);
    } else if (const auto* k = std::get_if<Connector>(&o)) {
        d.insert(u"kind"_s, u"connector"_s);
        d.insert(u"pen"_s, pen(k->pen));
        d.insert(u"firstEnd"_s, xy(k->firstEnd.x, k->firstEnd.y));
        d.insert(u"secondEnd"_s, xy(k->secondEnd.x, k->secondEnd.y));
        d.insert(u"objectA"_s, indexIn(indexOf, k->objectA));
        d.insert(u"objectB"_s, indexIn(indexOf, k->objectB));
    }
    QJsonArray actions;
    for (const Action& a : c.actions) {
        actions.append(QJsonObject{{u"type"_s, static_cast<int>(a.sourceType)},
                                   {u"command"_s, static_cast<int>(a.command)},
                                   {u"target"_s, indexIn(indexOf, a.target)},
                                   {u"resource"_s, utf8(a.resource)}});
    }
    d.insert(u"actions"_s, actions);
    return d;
}

QString text(const QJsonValue& v)
{
    if (v.isUndefined()) {
        return u"<absent>"_s;
    }
    QJsonArray wrapper{v};
    const QByteArray json = QJsonDocument(wrapper).toJson(QJsonDocument::Compact);
    return QString::fromUtf8(json.mid(1, json.size() - 2));
}

void compare(const QJsonValue& e, const QJsonValue& a, const QString& path, QStringList& diffs, int maxDiffs)
{
    if (diffs.size() >= maxDiffs) {
        return;
    }
    if (e.isDouble() && a.isDouble()) {
        const double x = e.toDouble();
        const double y = a.toDouble();
        if (std::abs(x - y) > 1e-3 + 1e-9 * std::max(std::abs(x), std::abs(y))) {
            diffs << u"%1 : %2 → %3"_s.arg(path, text(e), text(a));
        }
        return;
    }
    if (e.type() != a.type()) {
        diffs << u"%1 : %2 → %3"_s.arg(path, text(e), text(a));
        return;
    }
    if (e.isObject()) {
        const QJsonObject eo = e.toObject();
        const QJsonObject ao = a.toObject();
        QStringList keys = eo.keys();
        for (const QString& k : ao.keys()) {
            if (!eo.contains(k)) {
                keys << k;
            }
        }
        for (const QString& k : keys) {
            compare(eo.value(k), ao.value(k), path + u'/' + k, diffs, maxDiffs);
        }
        return;
    }
    if (e.isArray()) {
        const QJsonArray ea = e.toArray();
        const QJsonArray aa = a.toArray();
        if (ea.size() != aa.size()) {
            diffs << u"%1 : %2 éléments → %3"_s.arg(path).arg(ea.size()).arg(aa.size());
        }
        for (qsizetype i = 0; i < std::min(ea.size(), aa.size()); ++i) {
            compare(ea.at(i), aa.at(i), u"%1/%2"_s.arg(path).arg(i), diffs, maxDiffs);
        }
        return;
    }
    if (e != a) {
        diffs << u"%1 : %2 → %3"_s.arg(path, text(e), text(a));
    }
}

} // namespace

QJsonObject canonicalDump(const Document& document)
{
    QJsonArray pages;
    for (std::size_t p = 0; p < document.pageCount(); ++p) {
        const Page& page = document.page(p);
        IndexOf indexOf;
        for (std::size_t i = 0; i < page.objects.size(); ++i) {
            indexOf[page.objects[i].common.id] = static_cast<int>(i);
        }
        QJsonArray objects;
        for (const PageObject& o : page.objects) {
            objects.append(object(o, indexOf));
        }
        QJsonArray levels;
        for (const GridLevel& l : page.grid.levels) {
            levels.append(level(l));
        }
        const Fill& fill = page.background.fill;
        const bool gradient = fill.kind == FillKind::Gradient;
        pages.append(QJsonObject{
            {u"width"_s, page.width},
            {u"height"_s, page.height},
            {u"name"_s, utf8(page.name)},
            {u"notes"_s, utf8(page.notes)},
            {u"background"_s, QJsonObject{{u"color"_s, rgba(fill.color)},
                                          {u"gradient"_s, gradient ? static_cast<int>(fill.gradient) : 0},
                                          {u"color2"_s, gradient ? rgba(fill.color2) : QString()},
                                          {u"image"_s, utf8(page.background.image)}}},
            {u"grid"_s, QJsonObject{{u"visible"_s, page.grid.visible},
                                    {u"snapEnabled"_s, page.grid.snapEnabled},
                                    {u"drawnAbove"_s, page.grid.drawnAbove},
                                    {u"previewZoom"_s, page.grid.previewZoom},
                                    {u"baseZoom"_s, page.grid.baseZoom},
                                    {u"zoomIncrement"_s, page.grid.zoomIncrement},
                                    {u"levels"_s, levels}}},
            {u"objects"_s, objects}});
    }
    return QJsonObject{{u"format"_s, QString::fromLatin1(kDumpFormat)},
                       {u"title"_s, utf8(document.title())},
                       {u"pages"_s, pages}};
}

QJsonObject canonicalDump(const FlipchartReadResult& result)
{
    QJsonObject d = canonicalDump(result.document);
    d.insert(u"warnings"_s, static_cast<int>(result.warnings.size()));
    return d;
}

QStringList compareCanonical(const QJsonValue& expected, const QJsonValue& actual, int maxDiffs)
{
    QStringList diffs;
    compare(expected, actual, QString(), diffs, maxDiffs);
    return diffs;
}

} // namespace actcraft::formats::flipchart
