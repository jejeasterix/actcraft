#include "support/richdocument.h"

#include <actcraft/formats/nativejson.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>
#include <QVariant>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <vector>

using namespace actcraft;
using namespace Qt::StringLiterals;

namespace {

formats::ResourceLoader loaderFrom(const core::Document& source)
{
    return [&source](const std::string& key, const std::string&) -> std::optional<QByteArray> {
        const core::Resource* r = source.resource(key);
        if (!r) {
            return std::nullopt;
        }
        return r->data;
    };
}

QJsonObject object(const QJsonObject& root, int page, int index)
{
    return root.value(u"pages"_s)
        .toArray()
        .at(page)
        .toObject()
        .value(u"objects"_s)
        .toArray()
        .at(index)
        .toObject();
}

QJsonObject withObject(QJsonObject root, int page, int index, const QJsonObject& value)
{
    QJsonArray pages = root.value(u"pages"_s).toArray();
    QJsonObject p = pages.at(page).toObject();
    QJsonArray objects = p.value(u"objects"_s).toArray();
    objects.replace(index, value);
    p.insert(u"objects"_s, objects);
    pages.replace(page, p);
    root.insert(u"pages"_s, pages);
    return root;
}

// document.json du document riche, figé : toutes ses clés, à tous les niveaux.
QString referencePath()
{
    return QStringLiteral(ACTCRAFT_TESTS_DIR "/data/nativejson/richdocument.json");
}

// Premier écart entre deux valeurs JSON, vide s'il n'y en a pas : mêmes clés à chaque niveau, mêmes types,
// mêmes textes et booléens ; nombres à 1e-12 près en relatif (cos et sin peuvent différer d'une unité du
// dernier rang d'une bibliothèque C à l'autre).
QString firstDifference(const QJsonValue& expected, const QJsonValue& actual, const QString& path)
{
    if (expected.type() != actual.type()) {
        return u"%1 : type différent"_s.arg(path);
    }
    if (expected.isObject()) {
        const QJsonObject e = expected.toObject();
        const QJsonObject a = actual.toObject();
        if (e.keys() != a.keys()) {
            return u"%1 : clés [%2] au lieu de [%3]"_s.arg(path, a.keys().join(u','), e.keys().join(u','));
        }
        for (const QString& key : e.keys()) {
            const QString d = firstDifference(e.value(key), a.value(key), path + u'.' + key);
            if (!d.isEmpty()) {
                return d;
            }
        }
        return {};
    }
    if (expected.isArray()) {
        const QJsonArray e = expected.toArray();
        const QJsonArray a = actual.toArray();
        if (e.size() != a.size()) {
            return u"%1 : %2 éléments au lieu de %3"_s.arg(path, QString::number(a.size()),
                                                           QString::number(e.size()));
        }
        for (qsizetype i = 0; i < e.size(); ++i) {
            const QString d = firstDifference(e.at(i), a.at(i), u"%1[%2]"_s.arg(path, QString::number(i)));
            if (!d.isEmpty()) {
                return d;
            }
        }
        return {};
    }
    if (expected.isDouble()) {
        const double x = expected.toDouble();
        const double y = actual.toDouble();
        if (std::abs(x - y) > 1e-12 * std::max(1.0, std::abs(x))) {
            return u"%1 : %2 au lieu de %3"_s.arg(path, QString::number(y, 'g', 17),
                                                  QString::number(x, 'g', 17));
        }
        return {};
    }
    if (expected != actual) {
        return u"%1 : « %2 » au lieu de « %3 »"_s.arg(path, actual.toVariant().toString(),
                                                      expected.toVariant().toString());
    }
    return {};
}

// Vrai si chaque paire de champs (i, k) prend des valeurs différentes dans une instance au moins : échanger
// deux champs frères dans l'écrivain ou le lecteur change alors l'empreinte de l'aller-retour.
template <typename T, std::size_t N> bool separated(const std::vector<std::array<T, N>>& instances)
{
    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t k = i + 1; k < N; ++k) {
            const bool found =
                std::any_of(instances.begin(), instances.end(),
                            [i, k](const std::array<T, N>& values) { return values[i] != values[k]; });
            if (!found) {
                return false;
            }
        }
    }
    return true;
}

} // namespace

// document.json (DA3) : parité écrivain/lecteur, clés figées, lecture tolérante, version refusée.
class TstFormatsNativeJson : public QObject
{
    Q_OBJECT

private slots:
    void roundTripKeepsTheFingerprint()
    {
        const core::Document rich = richdoc::richDocument();
        QVERIFY(core::hasUniqueIds(rich));
        core::Document back;
        const formats::JsonReadResult r =
            formats::documentFromJson(formats::documentToJson(rich), loaderFrom(rich), back);
        QVERIFY2(r.ok, qPrintable(r.error));
        QVERIFY2(r.warnings.isEmpty(), qPrintable(r.warnings.join(u'\n')));
        QCOMPARE(core::fingerprint(back), core::fingerprint(rich));
        QCOMPARE(back.nextId(), core::ObjectId{20});
        QCOMPARE(back.resources().size(), std::size_t{2});
        QCOMPARE(back.pageCount(), std::size_t{3});
        // L'empreinte résume les ressources par leur clé : les octets, eux, viennent du chargeur.
        for (const auto& [key, resource] : rich.resources()) {
            QVERIFY(back.resource(key));
            QCOMPARE(back.resource(key)->data, resource.data);
        }
    }

    // −0 revient +0 de QJsonValue ; l'empreinte hache −0 comme +0, donc l'aller-retour la conserve.
    void roundTripKeepsTheFingerprintWithNegativeZeros()
    {
        core::Page page;
        core::Shape shape;
        shape.kind = core::ShapeKind::Polygon;
        shape.bounds = {10, 20, 30, 40};
        shape.polygon = {{-0.0, 20}, {40, -0.0}, {25, 60}};
        page.objects.emplace_back(shape);
        page.objects.back().common.id = 1;
        page.objects.back().common.transform.m12 = -0.0;
        core::Stroke stroke;
        stroke.points = {{1, 2, -0.0f}, {3, 4, 0.5f}};
        page.objects.emplace_back(stroke);
        page.objects.back().common.id = 2;
        const core::Document source(std::vector<core::Page>{page});
        QVERIFY(std::signbit(std::get<core::Shape>(source.page(0).objects[0]).polygon[0].x));
        core::Document back;
        const formats::JsonReadResult r =
            formats::documentFromJson(formats::documentToJson(source), loaderFrom(source), back);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(core::fingerprint(back), core::fingerprint(source));
    }

    void roundTripThroughTextKeepsTheFingerprint()
    {
        const core::Document rich = richdoc::richDocument();
        const QByteArray text = QJsonDocument(formats::documentToJson(rich)).toJson(QJsonDocument::Indented);
        core::Document back;
        const auto r =
            formats::documentFromJson(QJsonDocument::fromJson(text).object(), loaderFrom(rich), back);
        QVERIFY2(r.ok, qPrintable(r.error));
        QVERIFY2(r.warnings.isEmpty(), qPrintable(r.warnings.join(u'\n')));
        QCOMPARE(core::fingerprint(back), core::fingerprint(rich));
    }

    // Toutes les clés figées : le document.json du document riche est comparé, clé par clé, à la référence
    // commitée. Renommer ou retirer une clé (même dans l'écrivain ET le lecteur) le fait échouer. Une
    // évolution voulue (clé ajoutée, version mineure) régénère la référence :
    // ACTCRAFT_REGENERATE_REFERENCE=1, puis relecture du diff.
    void writerMatchesTheReference()
    {
        const QJsonObject written = formats::documentToJson(richdoc::richDocument());
        QFile file(referencePath());
        if (qEnvironmentVariableIsSet("ACTCRAFT_REGENERATE_REFERENCE")) {
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
            file.write(QJsonDocument(written).toJson(QJsonDocument::Indented));
            QSKIP("référence réécrite : relire le diff avant de la committer");
        }
        QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(referencePath()));
        QJsonParseError error;
        const QJsonDocument reference = QJsonDocument::fromJson(file.readAll(), &error);
        QVERIFY2(reference.isObject(), qPrintable(error.errorString()));
        const QString written2reference = firstDifference(reference.object(), written, u"document"_s);
        QVERIFY2(written2reference.isEmpty(), qPrintable(written2reference));
        // Le lecteur lit ces mêmes clés : la référence relue puis réécrite redonne la référence.
        const core::Document rich = richdoc::richDocument();
        core::Document back;
        const auto r = formats::documentFromJson(reference.object(), loaderFrom(rich), back);
        QVERIFY2(r.ok && r.warnings.isEmpty(), qPrintable(r.error + r.warnings.join(u'\n')));
        const QString reread =
            firstDifference(reference.object(), formats::documentToJson(back), u"document"_s);
        QVERIFY2(reread.isEmpty(), qPrintable(reread));
    }

    // Discrimine le patron écarté (énumérations en nombres, sérialisation générique) : un aller-retour
    // resterait vert avec des nombres, ce test non. Une énumération de chaque table est lue ici par son nom.
    void keysAreFrozen()
    {
        const QJsonObject root = formats::documentToJson(richdoc::richDocument());
        QCOMPARE(root.value(u"format"_s).toString(), u"actcraft-document/1"_s);
        QCOMPARE(root.value(u"title"_s).toString(), u"Document riche"_s);
        QCOMPARE(root.value(u"nextId"_s).toDouble(), 20.0);
        const QJsonArray resources = root.value(u"resources"_s).toArray();
        QCOMPARE(resources.size(), 2);
        QCOMPARE(resources.at(0).toObject().value(u"extension"_s).toString().size(), 3);
        // Les octets ne sont pas dans le JSON (pas de base64) : clé et extension seulement.
        QCOMPARE(resources.at(0).toObject().keys(), (QStringList{u"extension"_s, u"key"_s}));
        const QJsonObject rect = object(root, 0, 0);
        QCOMPARE(rect.value(u"type"_s).toString(), u"shape"_s);
        QCOMPARE(rect.value(u"shape"_s).toString(), u"rectangle"_s);
        QCOMPARE(rect.value(u"layer"_s).toString(), u"bottom"_s);
        QCOMPARE(rect.value(u"fill"_s).toString(), u"#ff336699"_s);
        QCOMPARE(rect.value(u"transform"_s).toArray().size(), 6);
        QCOMPARE(rect.value(u"restrictions"_s).toObject().value(u"move"_s).toBool(true), false);
        QCOMPARE(rect.value(u"actions"_s).toArray().at(0).toObject().value(u"kind"_s).toString(),
                 u"onObject"_s);
        const QJsonObject generated = object(root, 0, 1);
        QCOMPARE(generated.value(u"type"_s).toString(), u"image"_s);
        QCOMPARE(generated.value(u"pattern"_s).toString(), u"rings"_s);
        QCOMPARE(generated.value(u"hasTransparentColor"_s).toBool(true), false);
        const QJsonObject picture = object(root, 0, 2);
        QCOMPARE(picture.value(u"hasTransparentColor"_s).toBool(false), true);
        QCOMPARE(picture.value(u"transparentColor"_s).toString(), u"#ffffffff"_s);
        const QJsonObject path = object(root, 0, 3);
        QCOMPARE(path.value(u"shape"_s).toString(), u"path"_s);
        QCOMPARE(path.value(u"pen"_s).toObject().value(u"style"_s).toString(), u"dash"_s);
        QCOMPARE(path.value(u"pen"_s).toObject().value(u"cap"_s).toString(), u"flat"_s);
        QCOMPARE(path.value(u"pen"_s).toObject().value(u"join"_s).toString(), u"bevel"_s);
        QCOMPARE(path.value(u"brush"_s).toObject().value(u"kind"_s).toString(), u"gradient"_s);
        QCOMPARE(path.value(u"brush"_s).toObject().value(u"gradient"_s).toString(), u"radial"_s);
        QCOMPARE(object(root, 0, 4).value(u"type"_s).toString(), u"line"_s);
        QCOMPARE(object(root, 0, 5).value(u"type"_s).toString(), u"text"_s);
        QCOMPARE(object(root, 0, 6).value(u"type"_s).toString(), u"media"_s);
        QCOMPARE(object(root, 0, 7).value(u"type"_s).toString(), u"group"_s);
        QCOMPARE(object(root, 0, 7).value(u"members"_s).toArray().size(), 2);
        QCOMPARE(object(root, 0, 8).value(u"type"_s).toString(), u"connector"_s);
        const QJsonObject stroke = object(root, 0, 9);
        QCOMPARE(stroke.value(u"type"_s).toString(), u"stroke"_s);
        QCOMPARE(stroke.value(u"kind"_s).toString(), u"highlighter"_s);
        QCOMPARE(stroke.value(u"cap"_s).toString(), u"round"_s);
        QCOMPARE(stroke.value(u"points"_s).toArray().size(), 9); // x, y, pression × 3
        const QJsonObject page2 = root.value(u"pages"_s).toArray().at(1).toObject();
        QCOMPARE(page2.value(u"background"_s)
                     .toObject()
                     .value(u"fill"_s)
                     .toObject()
                     .value(u"gradient"_s)
                     .toString(),
                 u"diagonalUp"_s);
        const QJsonObject fine =
            page2.value(u"grid"_s).toObject().value(u"levels"_s).toArray().at(1).toObject();
        QCOMPARE(fine.value(u"line"_s).toString(), u"dotted"_s);
        QCOMPARE(fine.value(u"pattern"_s).toString(), u"angle"_s);
    }

    // Garde-fou de parité (relecture du plan, tour 1) : chaque champ du modèle a, dans richDocument, une
    // valeur hors défaut. Un champ ajouté au modèle casse ces liaisons structurées à la compilation : il faut
    // alors le remplir ici, et roundTripKeepsTheFingerprint prouve qu'il traverse document.json (l'empreinte
    // couvre tous les champs).
    void richDocumentFillsEveryField()
    {
        using namespace actcraft::core;
        const Document d = richdoc::richDocument();
        const Page& p1 = d.page(0);
        {
            const auto& [id, name, layer, visible, locked, translucency, restrictions, transform, label,
                         actions] = p1.objects[0].common;
            QVERIFY(id != 0 && !name.empty() && layer != Layer::Middle && !visible && locked &&
                    translucency != 0);
            const auto& [canMove, canSize, canRotate] = restrictions;
            QVERIFY(!canMove && !canSize && canRotate && !p1.objects[1].common.restrictions.canRotate);
            const auto& [m11, m12, m21, m22, dx, dy] = transform;
            QVERIFY(m11 != 1 && m12 != 0 && m21 != 0 && m22 != 1 && dx != 0 && dy != 0);
            QVERIFY(!label.empty() && actions.size() == 1);
            const auto& [kind, sourceType, command, target, resource] = actions[0];
            QVERIFY(kind != ActionKind::Other && sourceType != 0 && command != 0 && target != 0 &&
                    !resource.empty());
        }
        {
            const auto& [x, y, width, height] = std::get<Shape>(p1.objects[0]).bounds;
            QVERIFY(x != 0 && y != 0 && width != 0 && height != 0);
            const auto& [px, py] = std::get<Connector>(p1.objects[8]).endA;
            QVERIFY(px != 0 && py != 0);
        }
        {
            const auto& [kind, bounds, polygon, fill, outline, outlineWidth, opacity, paths, open,
                         windingFill, pen, brush] = std::get<Shape>(p1.objects[3]);
            QVERIFY(kind != ShapeKind::Rectangle && bounds.width != 0 && !polygon.empty() &&
                    fill != 0xFFFFFFFFu && outline != 0xFF000000u && outlineWidth != 1 && opacity != 1 &&
                    !paths.empty() && open && !windingFill && std::get<Shape>(p1.objects[0]).windingFill);
            const auto& [style, color, width, cap, join] = pen;
            QVERIFY(style != PenStyle::Solid && color != 0xFF000000u && width != 1 && cap != PenCap::Round &&
                    join != PenJoin::Round);
            const auto& [fillKind, color1, color2, gradient, pattern, opaque, background] = brush;
            QVERIFY(fillKind != FillKind::None && color1 != 0xFFFFFFFFu && color2 != 0xFFFFFFFFu &&
                    gradient != GradientKind::Vertical && pattern != 0 && opaque &&
                    background != 0xFF000000u);
        }
        {
            const auto& [bounds, sw, sh, pattern, a, b, seed, resource, hasTransparent, transparent] =
                std::get<ImageObject>(p1.objects[2]);
            QVERIFY(bounds.width != 0 && sw != 256 && sh != 256 && pattern != ImagePattern::LinearGradient &&
                    a != 0xFF000000u && b != 0xFFFFFFFFu && seed != 0 && !resource.empty() &&
                    hasTransparent && transparent != 0);
        }
        {
            const auto& [points, baseWidth, color, kind, cap] = std::get<Stroke>(p1.objects[9]);
            QVERIFY(points.size() == 3 && baseWidth != 3 && color != 0xFF000000u && kind != StrokeKind::Pen &&
                    cap != PenCap::Square);
            const auto& [x, y, pressure] = points[0];
            QVERIFY(x != 0 && y != 0 && pressure != 0.5f);
        }
        {
            const auto& [points, pen] = std::get<Line>(p1.objects[4]);
            QVERIFY(!points.empty() && pen.style != PenStyle::Solid);
        }
        {
            const auto& [bounds, html, runs] = std::get<Text>(p1.objects[5]);
            QVERIFY(bounds.width != 0 && !html.empty() && runs.size() == 2);
            const auto& [text, family, size, weight, italic, underline, strikeOut, color, baseline,
                         runBounds] = runs[0];
            QVERIFY(!text.empty() && !family.empty() && size != 12 && weight != 400 && italic && underline &&
                    !strikeOut && runs[1].strikeOut && color != 0xFF000000u && baseline.y != 0 &&
                    runBounds.width != 0);
        }
        {
            const auto& [bounds, placeholder, media, fileName, autoPlay, loop] =
                std::get<Media>(p1.objects[6]);
            QVERIFY(bounds.width != 0 && !placeholder.empty() && !media.empty() && !fileName.empty() &&
                    autoPlay && !loop && std::get<Media>(d.page(2).objects[0]).loop);
        }
        {
            const auto& [members] = std::get<Group>(p1.objects[7]);
            QVERIFY(!members.empty());
        }
        {
            const auto& [endA, endB, objectA, objectB, pen] = std::get<Connector>(p1.objects[8]);
            QVERIFY(endA.x != 0 && endB.x != 0 && objectA != 0 && objectB != 0 && pen.color != 0xFF000000u);
        }
        {
            const auto& [width, height, objects, background, grid, name, notes] = d.page(1);
            QVERIFY(width != 1024 && height != 768 && objects.empty() && !name.empty() && !notes.empty());
            const auto& [fill, image] = background;
            QVERIFY(fill.kind != FillKind::Solid && !image.empty());
            const auto& [visible, allowSnap, onTop, thumbnailScale, defaultScale, scaleStep, levels] = grid;
            QVERIFY(visible && !allowSnap && d.page(2).grid.allowSnap && onTop && thumbnailScale != 1 &&
                    defaultScale != 1 && scaleStep != 0.5 && levels.size() == 3);
            const auto& [lvVisible, snap, start, stepX, stepY, color, pattern, angle, line, drawX, drawY] =
                levels[1];
            QVERIFY(!lvVisible && snap && start.x != 0 && stepX != 32 && stepY != 32 &&
                    color != 0xFF000000u && pattern != GridPattern::Normal && angle != 30 &&
                    line != GridLine::Solid && drawX && !drawY);
            QVERIFY(!levels[0].snap && !levels[2].drawX);
        }
        {
            const auto& [data, extension] = d.resources().begin()->second;
            QVERIFY(!data.isEmpty() && !extension.empty());
        }
        QVERIFY(!d.title().empty() && d.resources().size() == 2 && d.nextId() == 20);
    }

    // Revue de L2-T5 : l'empreinte ne voit pas l'échange de deux champs frères de même valeur. Chaque groupe
    // de champs frères de même type a, dans richDocument, des valeurs distinctes pour chaque paire, dans une
    // instance au moins.
    void richDocumentSeparatesSiblings()
    {
        using namespace actcraft::core;
        const Document d = richdoc::richDocument();
        std::vector<std::array<bool, 2>> commonFlags, shapeFlags, mediaFlags;
        std::vector<std::array<bool, 3>> restrictionFlags, runFlags, gridFlags;
        std::vector<std::array<bool, 4>> levelFlags;
        std::vector<std::array<double, 6>> transforms;
        std::vector<std::array<double, 4>> rects;
        std::vector<std::array<double, 2>> points, shapeNumbers, pageSizes;
        std::vector<std::array<double, 3>> gridScales, levelNumbers;
        std::vector<std::array<Argb, 2>> shapeColors;
        std::vector<std::array<Argb, 3>> fillColors, imageColors;
        std::vector<std::array<std::int32_t, 2>> imageSizes;
        std::vector<std::array<ObjectId, 2>> connectorIds;
        std::vector<std::array<std::string, 2>> commonTexts, pageTexts, runTexts;
        std::vector<std::array<std::string, 3>> mediaTexts;
        const auto addPoint = [&points](const PointF& p) { points.push_back({p.x, p.y}); };
        const auto addRect = [&rects](const RectF& r) { rects.push_back({r.x, r.y, r.width, r.height}); };
        const auto addFill = [&fillColors](const Fill& f) {
            fillColors.push_back({f.color, f.color2, f.backgroundColor});
        };
        for (std::size_t i = 0; i < d.pageCount(); ++i) {
            const Page& page = d.page(i);
            pageSizes.push_back({page.width, page.height});
            pageTexts.push_back({page.name, page.notes});
            addFill(page.background.fill);
            gridFlags.push_back({page.grid.visible, page.grid.allowSnap, page.grid.onTop});
            gridScales.push_back({page.grid.thumbnailScale, page.grid.defaultScale, page.grid.scaleStep});
            for (const GridLevel& level : page.grid.levels) {
                levelFlags.push_back({level.visible, level.snap, level.drawX, level.drawY});
                levelNumbers.push_back({level.stepX, level.stepY, level.angle});
                addPoint(level.start);
            }
            for (const PageObject& o : page.objects) {
                const ObjectCommon& c = o.common;
                commonFlags.push_back({c.visible, c.locked});
                restrictionFlags.push_back(
                    {c.restrictions.canMove, c.restrictions.canSize, c.restrictions.canRotate});
                transforms.push_back({c.transform.m11, c.transform.m12, c.transform.m21, c.transform.m22,
                                      c.transform.dx, c.transform.dy});
                commonTexts.push_back({c.name, c.label});
                if (const auto* s = std::get_if<Shape>(&o)) {
                    shapeFlags.push_back({s->open, s->windingFill});
                    shapeNumbers.push_back({s->outlineWidth, s->opacity});
                    shapeColors.push_back({s->fill, s->outline});
                    addRect(s->bounds);
                    addFill(s->brush);
                } else if (const auto* im = std::get_if<ImageObject>(&o)) {
                    imageSizes.push_back({im->sourceWidth, im->sourceHeight});
                    imageColors.push_back({im->colorA, im->colorB, im->transparentColor});
                    addRect(im->bounds);
                } else if (const auto* st = std::get_if<Stroke>(&o)) {
                    for (const StrokePoint& sp : st->points) {
                        points.push_back({sp.x, sp.y});
                    }
                } else if (const auto* tx = std::get_if<Text>(&o)) {
                    addRect(tx->bounds);
                    for (const TextRun& run : tx->runs) {
                        runFlags.push_back({run.italic, run.underline, run.strikeOut});
                        runTexts.push_back({run.text, run.fontFamily});
                        addPoint(run.baseline);
                        addRect(run.bounds);
                    }
                } else if (const auto* m = std::get_if<Media>(&o)) {
                    mediaFlags.push_back({m->autoPlay, m->loop});
                    mediaTexts.push_back({m->placeholder, m->media, m->fileName});
                    addRect(m->bounds);
                } else if (const auto* cn = std::get_if<Connector>(&o)) {
                    connectorIds.push_back({cn->objectA, cn->objectB});
                    addPoint(cn->endA);
                    addPoint(cn->endB);
                }
            }
        }
        QVERIFY(separated(commonFlags));
        QVERIFY(separated(restrictionFlags));
        QVERIFY(separated(transforms));
        QVERIFY(separated(rects));
        QVERIFY(separated(points));
        QVERIFY(separated(shapeFlags));
        QVERIFY(separated(shapeNumbers));
        QVERIFY(separated(shapeColors));
        QVERIFY(separated(fillColors));
        QVERIFY(separated(imageSizes));
        QVERIFY(separated(imageColors));
        QVERIFY(separated(runFlags));
        QVERIFY(separated(runTexts));
        QVERIFY(separated(mediaFlags));
        QVERIFY(separated(mediaTexts));
        QVERIFY(separated(connectorIds));
        QVERIFY(separated(gridFlags));
        QVERIFY(separated(gridScales));
        QVERIFY(separated(levelFlags));
        QVERIFY(separated(levelNumbers));
        QVERIFY(separated(pageSizes));
        QVERIFY(separated(pageTexts));
        QVERIFY(separated(commonTexts));
    }

    // Un champ = une clé : un champ dont la valeur ne sert pas (couleur transparente sans transparence)
    // traverse quand même. Une clé qui fusionnerait les deux (null = pas de transparence) perdrait la
    // couleur, et l'empreinte, qui hache les deux champs, changerait ; richDocument seul ne le verrait pas.
    void unusedFieldsStillTravel()
    {
        core::ImageObject image;
        image.bounds = {0, 0, 10, 10};
        image.hasTransparentColor = false;
        image.transparentColor = 0xFF123456u;
        core::PageObject o = image;
        o.common.id = 1;
        std::vector<core::Page> pages(1);
        pages[0].objects.push_back(o);
        const core::Document source(std::move(pages));
        core::Document back;
        const QByteArray text = QJsonDocument(formats::documentToJson(source)).toJson(QJsonDocument::Compact);
        const auto r =
            formats::documentFromJson(QJsonDocument::fromJson(text).object(), loaderFrom(source), back);
        QVERIFY2(r.ok, qPrintable(r.error));
        QVERIFY2(r.warnings.isEmpty(), qPrintable(r.warnings.join(u'\n')));
        const auto* read = std::get_if<core::ImageObject>(&back.page(0).objects[0]);
        QVERIFY(read);
        QVERIFY(!read->hasTransparentColor);
        QCOMPARE(read->transparentColor, core::Argb{0xFF123456u});
        QCOMPARE(core::fingerprint(back), core::fingerprint(source));
    }

    void hugeOrNonFiniteNumbersAreRefused()
    {
        const core::Document rich = richdoc::richDocument();
        QJsonObject root = formats::documentToJson(rich);
        QJsonObject image = object(root, 0, 1); // image procédurale
        image.insert(u"sourceWidth"_s, 1e30);   // hors limites
        image.insert(u"sourceHeight"_s, 2.5);   // fraction pour un entier
        image.insert(u"seed"_s, -5);            // négatif pour un non signé
        root = withObject(root, 0, 1, image);
        QJsonObject stroke = object(root, 0, 9);
        stroke.insert(u"baseWidth"_s, std::numeric_limits<double>::quiet_NaN());
        stroke.insert(u"translucency"_s, std::numeric_limits<double>::quiet_NaN()); // entier NaN
        root = withObject(root, 0, 9, stroke);
        QJsonObject line = object(root, 0, 4);
        QJsonObject pen = line.value(u"pen"_s).toObject();
        pen.insert(u"width"_s, -std::numeric_limits<double>::infinity());
        line.insert(u"pen"_s, pen);
        root = withObject(root, 0, 4, line);
        core::Document back;
        const auto r = formats::documentFromJson(root, loaderFrom(rich), back);
        QVERIFY(r.ok);
        QCOMPARE(r.warnings.size(), 6);
        const auto* readImage = std::get_if<core::ImageObject>(&back.page(0).objects[1]);
        QVERIFY(readImage);
        QCOMPARE(readImage->sourceWidth, std::int32_t{256});  // défaut du modèle, pas 1e30 tronqué
        QCOMPARE(readImage->sourceHeight, std::int32_t{256}); // défaut du modèle, pas 2
        QCOMPARE(readImage->seed, std::uint32_t{0});
        const auto* readStroke = std::get_if<core::Stroke>(&back.page(0).objects[9]);
        QVERIFY(readStroke);
        QCOMPARE(readStroke->baseWidth, core::Stroke{}.baseWidth);
        QCOMPARE(back.page(0).objects[9].common.translucency, std::uint8_t{0});
        const auto* readLine = std::get_if<core::Line>(&back.page(0).objects[4]);
        QVERIFY(readLine);
        QCOMPARE(readLine->pen.width, core::Outline{}.width);
    }

    // 2^53 + 1 s'écrit 2^53 dans un réel JSON : 2^53 est ambigu, refusé ; 2^53 − 1 est le plus grand porté.
    void idsFromTwoToThe53AreRefused()
    {
        const core::Document rich = richdoc::richDocument();
        QJsonObject root = formats::documentToJson(rich);
        QJsonObject first = object(root, 0, 0);
        first.insert(u"id"_s, 9007199254740992.0);
        root = withObject(root, 0, 0, first);
        QJsonObject second = object(root, 0, 1);
        second.insert(u"id"_s, 9007199254740991.0);
        root = withObject(root, 0, 1, second);
        core::Document back;
        const auto r = formats::documentFromJson(root, loaderFrom(rich), back);
        QVERIFY(r.ok);
        QVERIFY2(r.warnings.join(u' ').contains(u"identifiant invalide"_s),
                 qPrintable(r.warnings.join(u'\n')));
        QCOMPARE(back.page(0).objects[0].common.id, core::ObjectId{0});
        QCOMPARE(back.page(0).objects[1].common.id, core::ObjectId{9007199254740991ull});
    }

    // Page::objects est rangé par couche : un fichier désordonné est remis en ordre, stable dans chaque
    // couche, avec un avertissement (tolérance plutôt que refus : rien n'est perdu).
    void layersOutOfOrderAreReorderedWithAWarning()
    {
        const core::Document rich = richdoc::richDocument();
        QJsonObject root = formats::documentToJson(rich);
        QJsonArray pages = root.value(u"pages"_s).toArray();
        QJsonObject p1 = pages.at(0).toObject();
        QJsonArray objects = p1.value(u"objects"_s).toArray();
        const QJsonValue bottom = objects.takeAt(0); // le rectangle, couche du bas…
        objects.append(bottom);                      // …posé après ceux du haut
        p1.insert(u"objects"_s, objects);
        pages.replace(0, p1);
        root.insert(u"pages"_s, pages);
        core::Document back;
        const auto r = formats::documentFromJson(root, loaderFrom(rich), back);
        QVERIFY(r.ok);
        QCOMPARE(r.warnings.size(), 1);
        QVERIFY2(r.warnings.first().contains(u"couches"_s), qPrintable(r.warnings.first()));
        QCOMPARE(back.page(0).objects[0].common.id, core::ObjectId{1});
        QCOMPARE(core::fingerprint(back), core::fingerprint(rich)); // ordre d'origine dans chaque couche
    }

    void unsafeResourceNamesAreRefused()
    {
        const core::Document rich = richdoc::richDocument();
        QJsonObject root = formats::documentToJson(rich);
        QJsonArray resources = root.value(u"resources"_s).toArray();
        QJsonObject first = resources.at(0).toObject();
        first.insert(u"extension"_s, u"../../x"_s);
        resources.replace(0, first);
        QJsonObject second = resources.at(1).toObject();
        second.insert(u"key"_s, u"../cle"_s);
        resources.replace(1, second);
        root.insert(u"resources"_s, resources);
        core::Document back;
        bool loaderCalled = false;
        const auto r = formats::documentFromJson(
            root,
            [&loaderCalled](const std::string&, const std::string&) -> std::optional<QByteArray> {
                loaderCalled = true;
                return std::nullopt;
            },
            back);
        QVERIFY(r.ok);
        QVERIFY(!loaderCalled); // un nom invalide n'atteint jamais l'archive
        QVERIFY(r.warnings.join(u' ').contains(u"nom invalide"_s));
        QCOMPARE(back.resources().size(), std::size_t{0});
    }

    void unknownKeysAreIgnoredSilently()
    {
        const core::Document rich = richdoc::richDocument();
        QJsonObject root = formats::documentToJson(rich);
        root.insert(u"future"_s, u"plus tard"_s);
        QJsonObject o = object(root, 0, 0);
        o.insert(u"future"_s, 3);
        root = withObject(root, 0, 0, o);
        core::Document back;
        const auto r = formats::documentFromJson(root, loaderFrom(rich), back);
        QVERIFY(r.ok);
        QVERIFY(r.warnings.isEmpty());
        QCOMPARE(core::fingerprint(back), core::fingerprint(rich));
    }

    void unknownObjectTypeIsSkippedWithAWarning()
    {
        const core::Document rich = richdoc::richDocument();
        QJsonObject o = object(formats::documentToJson(rich), 0, 1);
        o.insert(u"type"_s, u"hologram"_s);
        core::Document back;
        const auto r = formats::documentFromJson(withObject(formats::documentToJson(rich), 0, 1, o),
                                                 loaderFrom(rich), back);
        QVERIFY(r.ok);
        QVERIFY(r.warnings.join(u' ').contains(u"hologram"_s));
        QCOMPARE(back.page(0).objects.size(), std::size_t{9});
    }

    void otherMajorVersionIsRefusedAndTargetUntouched()
    {
        const core::Document rich = richdoc::richDocument();
        QJsonObject root = formats::documentToJson(rich);
        root.insert(u"format"_s, u"actcraft-document/2"_s);
        core::Document target(640, 480);
        const std::uint64_t before = core::fingerprint(target);
        const auto r = formats::documentFromJson(root, loaderFrom(rich), target);
        QVERIFY(!r.ok);
        QVERIFY2(r.error.contains(u"version"_s), qPrintable(r.error));
        QCOMPARE(core::fingerprint(target), before);
        root.remove(u"format"_s);
        QVERIFY(!formats::documentFromJson(root, loaderFrom(rich), target).ok);
        root.insert(u"format"_s, u"autre-chose/1"_s);
        QVERIFY(!formats::documentFromJson(root, loaderFrom(rich), target).ok);
        QCOMPARE(core::fingerprint(target), before);
        // Une version mineure plus récente de la même version majeure se lit.
        root.insert(u"format"_s, u"actcraft-document/1.4"_s);
        QVERIFY(formats::documentFromJson(root, loaderFrom(rich), target).ok);
    }

    void missingOrTamperedResourceIsAWarning()
    {
        const core::Document rich = richdoc::richDocument();
        core::Document back;
        auto r = formats::documentFromJson(
            formats::documentToJson(rich),
            [](const std::string&, const std::string&) { return std::nullopt; }, back);
        QVERIFY(r.ok);
        QCOMPARE(r.warnings.size(), 2);
        QCOMPARE(back.resources().size(), std::size_t{0});
        r = formats::documentFromJson(
            formats::documentToJson(rich),
            [](const std::string&, const std::string&) -> std::optional<QByteArray> {
                return QByteArray("autre");
            },
            back);
        QVERIFY(r.ok);
        QVERIFY(r.warnings.join(u' ').contains(u"empreinte"_s));
        QCOMPARE(back.resources().size(), std::size_t{0});
    }

    void malformedValuesFallBackWithWarnings()
    {
        const core::Document rich = richdoc::richDocument();
        QJsonObject o = object(formats::documentToJson(rich), 0, 4); // ligne
        o.insert(u"points"_s, QJsonArray{1, 2, 3});
        QJsonObject pen = o.value(u"pen"_s).toObject();
        pen.insert(u"color"_s, u"rouge"_s);
        o.insert(u"pen"_s, pen);
        core::Document back;
        const auto r = formats::documentFromJson(withObject(formats::documentToJson(rich), 0, 4, o),
                                                 loaderFrom(rich), back);
        QVERIFY(r.ok);
        QCOMPARE(r.warnings.size(), 2);
        const auto* line = std::get_if<core::Line>(&back.page(0).objects[4]);
        QVERIFY(line);
        QCOMPARE(line->points.size(), std::size_t{1});
        QCOMPARE(line->pen.color, core::Outline{}.color);
    }

    void noPageGivesOneBlankPage()
    {
        QJsonObject root{{u"format"_s, u"actcraft-document/1"_s}, {u"pages"_s, QJsonArray{}}};
        core::Document back;
        const auto r = formats::documentFromJson(
            root, [](const std::string&, const std::string&) { return std::nullopt; }, back);
        QVERIFY(r.ok);
        QCOMPARE(back.pageCount(), std::size_t{1});
        QVERIFY(!r.warnings.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TstFormatsNativeJson)
#include "tst_formats_nativejson.moc"
