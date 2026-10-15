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

// « #rrggbbaa » en minuscules, comme le f"{r:02x}" de l'oracle.
QString rgba(Argb c)
{
    return u"#%1%2%3%4"_s.arg((c >> 16) & 0xFFu, 2, 16, QLatin1Char('0'))
        .arg((c >> 8) & 0xFFu, 2, 16, QLatin1Char('0'))
        .arg(c & 0xFFu, 2, 16, QLatin1Char('0'))
        .arg((c >> 24) & 0xFFu, 2, 16, QLatin1Char('0'));
}

QString sha(const std::string& key)
{
    return QString::fromStdString(key); // les clés de ressources sont déjà le SHA-256 des octets
}

QJsonArray rect(const RectF& r)
{
    return QJsonArray{r.x, r.y, r.width, r.height};
}

// Règle de la table : style 0 sans largeur, quel que soit le style rangé dans le modèle.
QJsonObject pen(const Outline& o)
{
    return QJsonObject{{u"color"_s, rgba(o.color)},
                       {u"width"_s, o.width},
                       {u"style"_s, o.width <= 0 ? 0 : static_cast<int>(o.style)}};
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
                       {u"pattern"_s, f.kind == FillKind::Pattern ? static_cast<int>(f.pattern) : 0}};
}

template <typename Points> QJsonObject geometry(const Points& points)
{
    double sx = 0;
    double sy = 0;
    for (const auto& p : points) {
        sx += p.x;
        sy += p.y;
    }
    return QJsonObject{{u"points"_s, static_cast<int>(points.size())}, {u"sum"_s, QJsonArray{sx, sy}}};
}

QJsonObject object(const PageObject& o, const std::map<ObjectId, int>& indexOf)
{
    const auto index = [&indexOf](ObjectId id) {
        const auto it = indexOf.find(id);
        return it == indexOf.end() ? -1 : it->second;
    };
    const Transform& t = o.common.transform;
    // Couche du .flipchart (0 dessous, 1 milieu, 2 dessus) : le modèle numérote Bottom 1, Middle 2, Top 3.
    QJsonObject d{{u"layer"_s, std::max(0, static_cast<int>(o.common.layer) - 1)},
                  {u"name"_s, QString::fromStdString(o.common.name)},
                  {u"visible"_s, o.common.visible},
                  {u"locked"_s, o.common.locked},
                  {u"translucency"_s, static_cast<int>(o.common.translucency)},
                  {u"matrix"_s, QJsonArray{t.m11, t.m12, t.m21, t.m22, t.dx, t.dy}}};
    if (const auto* s = std::get_if<Stroke>(&o)) {
        d.insert(u"kind"_s, u"annotation"_s);
        d.insert(u"pen"_s, QJsonObject{{u"color"_s, rgba(s->color)}, {u"width"_s, s->baseWidth}});
        d.insert(u"strokeMode"_s, static_cast<int>(s->kind));
        d.insert(u"geometry"_s, geometry(s->points));
    } else if (const auto* l = std::get_if<Line>(&o)) {
        d.insert(u"kind"_s, u"line"_s);
        d.insert(u"pen"_s, pen(l->pen));
        d.insert(u"geometry"_s, geometry(l->points));
    } else if (const auto* sh = std::get_if<Shape>(&o)) {
        d.insert(u"kind"_s, u"shape"_s);
        d.insert(u"pen"_s, pen(sh->pen));
        d.insert(u"brush"_s, brush(sh->brush));
        std::vector<PointF> all;
        for (const auto& path : sh->paths) {
            all.insert(all.end(), path.begin(), path.end());
        }
        QJsonObject outlines = geometry(all);
        outlines.insert(u"polygons"_s, static_cast<int>(sh->paths.size()));
        d.insert(u"geometry"_s, outlines);
        d.insert(u"open"_s, sh->open);
        d.insert(u"bounds"_s, rect(sh->bounds));
    } else if (const auto* im = std::get_if<ImageObject>(&o)) {
        d.insert(u"kind"_s, u"image"_s);
        d.insert(u"bounds"_s, rect(im->bounds));
        d.insert(u"image"_s, sha(im->resource));
        d.insert(u"transparent"_s, im->hasKeyColor);
    } else if (const auto* tx = std::get_if<Text>(&o)) {
        d.insert(u"kind"_s, u"text"_s);
        d.insert(u"bounds"_s, rect(tx->bounds));
        d.insert(u"text"_s,
                 QJsonObject{{u"html"_s, tx->html.empty() ? QString()
                                                          : QString::fromStdString(sha256Hex(
                                                                QByteArray::fromStdString(tx->html)))},
                             {u"runs"_s, static_cast<int>(tx->runs.size())}});
    } else if (const auto* m = std::get_if<Media>(&o)) {
        d.insert(u"kind"_s, u"media"_s);
        d.insert(u"bounds"_s, rect(m->bounds));
        d.insert(u"image"_s, sha(m->placeholder));
    } else if (const auto* g = std::get_if<Group>(&o)) {
        d.insert(u"kind"_s, u"group"_s);
        // Indices dans la liste exportée, pas identifiants du modèle.
        std::vector<int> members;
        for (const ObjectId id : g->members) {
            if (const int i = index(id); i >= 0) {
                members.push_back(i);
            }
        }
        std::sort(members.begin(), members.end());
        QJsonArray list;
        for (const int i : members) {
            list.append(i);
        }
        d.insert(u"members"_s, list);
    } else if (const auto* c = std::get_if<Connector>(&o)) {
        d.insert(u"kind"_s, u"connector"_s);
        d.insert(u"pen"_s, pen(c->pen));
    }
    QJsonArray actions;
    for (const Action& a : o.common.actions) {
        actions.append(QJsonObject{{u"type"_s, static_cast<int>(a.sourceType)},
                                   {u"command"_s, static_cast<int>(a.command)},
                                   {u"target"_s, a.target == 0 ? -1 : index(a.target)}});
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
        std::map<ObjectId, int> indexOf;
        for (std::size_t i = 0; i < page.objects.size(); ++i) {
            indexOf[page.objects[i].common.id] = static_cast<int>(i);
        }
        QJsonArray objects;
        for (const PageObject& o : page.objects) {
            objects.append(object(o, indexOf));
        }
        const Fill& fill = page.background.fill;
        const bool gradient = fill.kind == FillKind::Gradient;
        pages.append(QJsonObject{
            {u"width"_s, page.width},
            {u"height"_s, page.height},
            {u"background"_s, QJsonObject{{u"color"_s, rgba(fill.color)},
                                          {u"gradient"_s, gradient ? static_cast<int>(fill.gradient) : 0},
                                          {u"color2"_s, gradient ? rgba(fill.color2) : QString()},
                                          {u"image"_s, sha(page.background.image)}}},
            {u"grid"_s, QJsonObject{{u"visible"_s, page.grid.visible},
                                    {u"levels"_s, static_cast<int>(page.grid.levels.size())}}},
            {u"objects"_s, objects}});
    }
    return QJsonObject{{u"format"_s, QString::fromLatin1(kDumpFormat)},
                       {u"title"_s, QString::fromStdString(document.title())},
                       {u"pages"_s, pages}};
}

QStringList compareCanonical(const QJsonValue& expected, const QJsonValue& actual, int maxDiffs)
{
    QStringList diffs;
    compare(expected, actual, QString(), diffs, maxDiffs);
    return diffs;
}

} // namespace actcraft::formats::flipchart
