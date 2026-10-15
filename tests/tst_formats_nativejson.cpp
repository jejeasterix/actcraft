#include "support/richdocument.h"

#include <actcraft/formats/nativejson.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

#include <optional>
#include <string>

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
            QVERIFY(!canMove && !canSize && !canRotate);
            const auto& [m11, m12, m21, m22, dx, dy] = transform;
            QVERIFY(m11 != 1 && m12 != 0 && m21 != 0 && m22 != 1 && dx != 0 && dy != 0);
            QVERIFY(!label.empty() && actions.size() == 1);
            const auto& [kind, sourceType, command, target, resource] = actions[0];
            QVERIFY(kind != ActionKind::Other && sourceType != 0 && command != 0 && target != 0 &&
                    !resource.empty());
        }
        {
            const auto& [kind, bounds, polygon, fill, outline, outlineWidth, opacity, paths, open,
                         windingFill, pen, brush] = std::get<Shape>(p1.objects[3]);
            QVERIFY(kind != ShapeKind::Rectangle && bounds.width != 0 && !polygon.empty() &&
                    fill != 0xFFFFFFFFu && outline != 0xFF000000u && outlineWidth != 1 && opacity != 1 &&
                    !paths.empty() && open && windingFill);
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
            QVERIFY(bounds.width != 0 && !html.empty() && !runs.empty());
            const auto& [text, family, size, weight, italic, underline, strikeOut, color, baseline,
                         runBounds] = runs[0];
            QVERIFY(!text.empty() && !family.empty() && size != 12 && weight != 400 && italic && underline &&
                    strikeOut && color != 0xFF000000u && baseline.y != 0 && runBounds.width != 0);
        }
        {
            const auto& [bounds, placeholder, media, fileName, autoPlay, loop] =
                std::get<Media>(p1.objects[6]);
            QVERIFY(bounds.width != 0 && !placeholder.empty() && !media.empty() && !fileName.empty() &&
                    autoPlay && loop);
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
            QVERIFY(visible && allowSnap && onTop && thumbnailScale != 1 && defaultScale != 1 &&
                    scaleStep != 0.5 && levels.size() == 2);
            const auto& [lvVisible, snap, start, stepX, stepY, color, pattern, angle, line, drawX, drawY] =
                levels[1];
            QVERIFY(!lvVisible && !snap && start.x != 0 && stepX != 32 && stepY != 32 &&
                    color != 0xFF000000u && pattern != GridPattern::Normal && angle != 30 &&
                    line != GridLine::Solid && !drawX && !drawY);
        }
        {
            const auto& [data, extension] = d.resources().begin()->second;
            QVERIFY(!data.isEmpty() && !extension.empty());
        }
        QVERIFY(!d.title().empty() && d.resources().size() == 2 && d.nextId() == 20);
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
        QJsonObject o = object(formats::documentToJson(rich), 0, 1); // image procédurale
        o.insert(u"sourceWidth"_s, 1e30);
        o.insert(u"seed"_s, -5);
        core::Document back;
        const auto r = formats::documentFromJson(withObject(formats::documentToJson(rich), 0, 1, o),
                                                 loaderFrom(rich), back);
        QVERIFY(r.ok);
        QCOMPARE(r.warnings.size(), 2);
        const auto* image = std::get_if<core::ImageObject>(&back.page(0).objects[1]);
        QVERIFY(image);
        QCOMPARE(image->sourceWidth, std::int32_t{256}); // hors limites : défaut du modèle, pas 1e30 tronqué
        QCOMPARE(image->seed, std::uint32_t{0});         // négatif pour un non signé : défaut du modèle
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
