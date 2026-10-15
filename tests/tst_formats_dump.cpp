#include "support/flipchartwriter.h"

#include <actcraft/formats/flipchart.h>
#include <actcraft/formats/flipchartdump.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

using namespace actcraft::formats::flipchart;
using namespace Qt::StringLiterals;

namespace {

using namespace actcraft::core;

// Document dont chaque clé de l'export a une valeur distincte et non nulle ; identifiants loin des indices.
Document handBuiltDocument()
{
    Group refs;
    refs.members = {30, 10, 99}; // 99 : absent de la page
    PageObject group(refs);
    group.common.id = 70;
    group.common.layer = Layer::Bottom;

    Shape patterned;
    patterned.kind = ShapeKind::Path;
    patterned.pen.style = PenStyle::Dash;
    patterned.pen.width = 2.5;
    patterned.pen.color = 0xFF112233u;
    patterned.pen.cap = PenCap::Square;
    patterned.pen.join = PenJoin::Bevel;
    patterned.brush.kind = FillKind::Pattern;
    patterned.brush.pattern = 7;
    patterned.brush.color = 0xFF223344u;
    patterned.brush.color2 = 0xFF334455u;
    patterned.brush.opaqueBackground = true;
    patterned.brush.backgroundColor = 0xFF445566u;
    patterned.paths = {{{1, 2}, {3, 4}, {5, 6}}, {{7, 8}, {9, 10}, {11, 12}, {13, 14}}};
    patterned.open = true;
    patterned.windingFill = true;
    patterned.bounds = {1, 2, 30, 40};
    PageObject shape(patterned);
    shape.common.id = 10;
    shape.common.layer = Layer::Middle;
    shape.common.name = "forme";
    shape.common.visible = false;
    shape.common.locked = true;
    shape.common.translucency = 17;
    shape.common.restrictions.movable = false;
    shape.common.restrictions.rotatable = false;
    shape.common.label = u"étiquette"_s.toStdString();
    shape.common.transform = Transform{2, 0.5, -0.25, 3, 40, 50};
    Action onGroup;
    onGroup.sourceType = 28;
    onGroup.command = 184;
    onGroup.target = 30;
    Action openFile;
    openFile.sourceType = 26;
    openFile.command = 270;
    openFile.target = 99;
    openFile.resource = "cafe";
    shape.common.actions = {onGroup, openFile};

    Shape shaded;
    shaded.kind = ShapeKind::Path;
    shaded.pen.style = PenStyle::Dash; // sans largeur : style 0 dans l'export
    shaded.pen.width = 0;
    shaded.pen.cap = PenCap::Flat;
    shaded.pen.join = PenJoin::Miter;
    shaded.brush.kind = FillKind::Gradient;
    shaded.brush.color = 0xFF556677u;
    shaded.brush.color2 = 0xFF667788u;
    shaded.brush.gradient = GradientKind::DiagonalUp;
    shaded.brush.pattern = 9; // hors motif : 0 dans l'export
    shaded.paths = {{{0.5, 0.5}}};
    PageObject gradient(shaded);
    gradient.common.id = 30;
    gradient.common.layer = Layer::Top;
    gradient.common.name = u"dégradé"_s.toStdString();

    Stroke ink;
    ink.points = {StrokePoint{10, 20, 0.5f}, StrokePoint{30, 45, 0.5f}};
    ink.baseWidth = 4.5;
    ink.color = 0x80FFFF00u;
    ink.kind = StrokeKind::Revealer;
    ink.cap = PenCap::Round;
    PageObject stroke(ink);
    stroke.common.id = 40;
    stroke.common.layer = Layer::Top;

    Line segment;
    segment.points = {{1, 1}, {2, 3}};
    segment.pen.color = 0xFF0000FFu;
    segment.pen.width = 1.5;
    segment.pen.style = PenStyle::DashDot;
    segment.pen.cap = PenCap::Flat;
    PageObject line(segment);
    line.common.id = 50;

    ImageObject picture;
    picture.bounds = {5, 6, 70, 80};
    picture.resource = "beef";
    picture.hasKeyColor = true;
    picture.keyColor = 0xFFABCDEFu;
    picture.sourceWidth = 320;
    picture.sourceHeight = 240;
    PageObject image(picture);
    image.common.id = 60;

    TextSpan span;
    span.text = "Bonjour";
    span.fontFamily = "Arial";
    span.pointSize = 14.5;
    span.weight = 700;
    span.italic = true;
    span.strikeOut = true;
    span.color = 0xFF123456u;
    span.baseline = {2, 3};
    span.bounds = {1, 2, 3, 4};
    Text words;
    words.bounds = {9, 8, 7, 6};
    words.html = u"<p>é</p>"_s.toStdString();
    words.runs = {span};
    PageObject text(words);
    text.common.id = 80;

    Media clip;
    clip.bounds = {11, 12, 13, 14};
    clip.placeholder = "d00d";
    clip.media = "f00d";
    clip.fileName = "son.mp3";
    clip.playOnOpen = true;
    PageObject media(clip);
    media.common.id = 90;

    Connector link;
    link.firstEnd = {1, 2};
    link.secondEnd = {3, 4};
    link.objectA = 60;
    link.pen.color = 0xFF00FF00u;
    link.pen.width = 3;
    link.pen.style = PenStyle::Dot;
    link.pen.cap = PenCap::Square;
    PageObject connector(link);
    connector.common.id = 95;

    Page page;
    page.width = 1111;
    page.height = 777;
    page.name = "Page une";
    page.notes = "Notes une";
    page.background.fill.kind = FillKind::Gradient;
    page.background.fill.color = 0xFF102030u;
    page.background.fill.color2 = 0xFF405060u;
    page.background.fill.gradient = GradientKind::Radial;
    page.background.image = "ab12";
    page.grid.visible = true;
    page.grid.snapEnabled = true;
    page.grid.drawnAbove = true;
    page.grid.previewZoom = 0.25;
    page.grid.baseZoom = 1.5;
    page.grid.zoomIncrement = 0.75;
    GridLevel tier;
    tier.snap = false;
    tier.start = {3, 4};
    tier.spacingX = 21;
    tier.spacingY = 22;
    tier.color = 0xFF0A0B0Cu;
    tier.pattern = GridPattern::Diagonal45;
    tier.angle = 33;
    tier.line = GridLine::Dashed;
    tier.linesAlongX = false;
    page.grid.levels = {tier};
    page.objects = {group, shape, gradient, stroke, line, image, text, media, connector};
    std::vector<Page> pages;
    pages.push_back(std::move(page));
    Document document(std::move(pages));
    document.setTitle("Titre d'essai");
    return document;
}

// Export attendu, écrit à la main d'après les règles (pas recalculé par le code testé). Les clés communes
// absentes d'un objet prennent les valeurs de kCommon.
constexpr char kExpected[] = R"({
 "format": "actcraft-flipchart-dump/1", "title": "Titre d\u0027essai", "warnings": 2,
 "pages": [{
  "width": 1111, "height": 777, "name": "Page une", "notes": "Notes une",
  "background": {"color": "#102030ff", "gradient": 5, "color2": "#405060ff", "image": "ab12"},
  "grid": {"visible": true, "snapEnabled": true, "drawnAbove": true, "previewZoom": 0.25, "baseZoom": 1.5,
           "zoomIncrement": 0.75,
           "levels": [{"visible": true, "snap": false, "start": [3, 4], "spacingX": 21, "spacingY": 22,
                       "color": "#0a0b0cff", "pattern": 1, "angle": 33, "line": 4, "linesAlongX": false,
                       "linesAlongY": true}]},
  "objects": [
   {"kind": "group", "layer": 0, "members": [1, 2]},
   {"kind": "shape", "layer": 1, "name": "forme", "visible": false, "locked": true, "translucency": 17,
    "movable": false, "resizable": true, "rotatable": false, "label": "\u00e9tiquette",
    "matrix": [2, 0.5, -0.25, 3, 40, 50],
    "pen": {"color": "#112233ff", "width": 2.5, "style": 2, "cap": 1, "join": 1},
    "brush": {"kind": "pattern", "color": "#223344ff", "color2": "", "gradient": 0, "pattern": 7,
              "opaque": true, "hatch": "#445566ff"},
    "paths": [[[1, 2], [3, 4], [5, 6]], [[7, 8], [9, 10], [11, 12], [13, 14]]],
    "open": true, "winding": true, "bounds": [1, 2, 30, 40],
    "actions": [{"type": 28, "command": 184, "target": 2, "resource": ""},
                {"type": 26, "command": 270, "target": -1, "resource": "cafe"}]},
   {"kind": "shape", "layer": 2, "name": "d\u00e9grad\u00e9",
    "pen": {"color": "#000000ff", "width": 0, "style": 0, "cap": 0, "join": 0},
    "brush": {"kind": "gradient", "color": "#556677ff", "color2": "#667788ff", "gradient": 4, "pattern": 0,
              "opaque": false, "hatch": "#000000ff"},
    "paths": [[[0.5, 0.5]]], "open": false, "winding": false, "bounds": [0, 0, 0, 0]},
   {"kind": "annotation", "layer": 2, "pen": {"color": "#ffff0080", "width": 4.5, "cap": 2}, "strokeMode": 2,
    "points": [[10, 20], [30, 45]]},
   {"kind": "line", "layer": 1, "pen": {"color": "#0000ffff", "width": 1.5, "style": 4, "cap": 0, "join": 2},
    "points": [[1, 1], [2, 3]]},
   {"kind": "image", "layer": 1, "bounds": [5, 6, 70, 80], "image": "beef", "transparent": true,
    "keyColor": "#abcdefff", "pixels": [320, 240]},
   {"kind": "text", "layer": 1, "bounds": [9, 8, 7, 6],
    "html": "5229bddeb25ca85b2dafff052ef9e9868e906e71e4b39f9a17a72b0aee7903d3",
    "runs": [{"text": "Bonjour", "font": "Arial", "size": 14.5, "weight": 700, "italic": true,
              "underline": false, "strikeOut": true, "color": "#123456ff", "baseline": [2, 3],
              "bounds": [1, 2, 3, 4]}]},
   {"kind": "media", "layer": 1, "bounds": [11, 12, 13, 14], "image": "d00d", "media": "f00d", "file": "son.mp3",
    "playOnOpen": true, "loop": false},
   {"kind": "connector", "layer": 1, "pen": {"color": "#00ff00ff", "width": 3, "style": 3, "cap": 1, "join": 2},
    "firstEnd": [1, 2], "secondEnd": [3, 4], "objectA": 5, "objectB": -1}
  ]}]})";

constexpr char kCommon[] =
    R"({"name": "", "visible": true, "locked": false, "translucency": 0, "movable": true,
 "resizable": true, "rotatable": true, "label": "", "matrix": [1, 0, 0, 1, 0, 0], "actions": []})";

QJsonObject parse(const char* json)
{
    QJsonParseError error{};
    const QJsonDocument d = QJsonDocument::fromJson(QByteArray(json), &error);
    if (error.error != QJsonParseError::NoError) {
        qFatal("JSON attendu illisible : %s", qPrintable(error.errorString()));
    }
    return d.object();
}

QJsonObject expectedExport()
{
    const QJsonObject common = parse(kCommon);
    QJsonObject root = parse(kExpected);
    QJsonArray pages = root.value(u"pages"_s).toArray();
    QJsonObject page = pages.at(0).toObject();
    QJsonArray objects = page.value(u"objects"_s).toArray();
    for (qsizetype i = 0; i < objects.size(); ++i) {
        QJsonObject o = objects.at(i).toObject();
        for (auto it = common.begin(); it != common.end(); ++it) {
            if (!o.contains(it.key())) {
                o.insert(it.key(), it.value());
            }
        }
        objects.replace(i, o);
    }
    page.insert(u"objects"_s, objects);
    pages.replace(0, page);
    root.insert(u"pages"_s, pages);
    return root;
}

} // namespace

// Export canonique (L2-T10) et comparaison tolérante.
class TstFormatsDump : public QObject
{
    Q_OBJECT

private slots:
    // Chaque clé de l'export, sur un document dont chaque valeur est distincte : nom vidé, booléen inversé,
    // matrice ou rectangle à zéro, empreinte vidée, ligne de texte en trop, clé en trop ou en moins → rouge.
    void dumpMatchesAHandWrittenExport()
    {
        FlipchartReadResult read;
        read.ok = true;
        read.warnings = {u"un"_s, u"deux"_s};
        read.document = handBuiltDocument();
        const QStringList diffs = compareCanonical(expectedExport(), canonicalDump(read), 100);
        QVERIFY2(diffs.isEmpty(), qPrintable(diffs.join(u'\n')));
    }

    // Lecteur et export ensemble, sur le paperboard synthétique de l'écrivain.
    void dumpFollowsTheRules()
    {
        const FlipchartReadResult r = readFlipchart(fctest::sampleFlipchart());
        QVERIFY(r.ok);
        const QJsonObject d = canonicalDump(r);
        QCOMPARE(d.value(u"format"_s).toString(), u"actcraft-flipchart-dump/1"_s);
        QCOMPARE(d.value(u"title"_s).toString(), u"Essai synthétique"_s);
        QCOMPARE(d.value(u"warnings"_s).toInt(), static_cast<int>(r.warnings.size()));
        const QJsonArray pages = d.value(u"pages"_s).toArray();
        QCOMPARE(pages.size(), 2);
        const QJsonObject p1 = pages.at(0).toObject();
        QCOMPARE(p1.value(u"background"_s).toObject().value(u"gradient"_s).toInt(), 1);
        QCOMPARE(p1.value(u"background"_s).toObject().value(u"color"_s).toString(), u"#c8dcffff"_s);
        QCOMPARE(p1.value(u"grid"_s).toObject().value(u"levels"_s).toArray().size(), 1);
        const QJsonArray objects = p1.value(u"objects"_s).toArray();
        QCOMPARE(objects.size(), 4);
        const QJsonObject group = objects.at(0).toObject();
        QCOMPARE(group.value(u"kind"_s).toString(), u"group"_s);
        QCOMPARE(group.value(u"members"_s).toArray(), (QJsonArray{1, 3}));
        const QJsonObject shape = objects.at(1).toObject();
        QCOMPARE(shape.value(u"brush"_s).toObject().value(u"kind"_s).toString(), u"solid"_s);
        QCOMPARE(shape.value(u"brush"_s).toObject().value(u"color2"_s).toString(), QString());
        QCOMPARE(shape.value(u"pen"_s).toObject().value(u"style"_s).toInt(), 2);
        const QJsonArray paths = shape.value(u"paths"_s).toArray();
        QCOMPARE(paths.size(), 2);
        QCOMPARE(paths.at(0).toArray().size() + paths.at(1).toArray().size(), 7);
        const QJsonObject action = shape.value(u"actions"_s).toArray().at(0).toObject();
        QCOMPARE(action.value(u"type"_s).toInt(), 28);
        QCOMPARE(action.value(u"command"_s).toInt(), 184);
        QCOMPARE(action.value(u"target"_s).toInt(), 2);
        const QJsonObject annotation = objects.at(3).toObject();
        QCOMPARE(annotation.value(u"kind"_s).toString(), u"annotation"_s);
        QCOMPARE(annotation.value(u"layer"_s).toInt(), 2);
        QCOMPARE(annotation.value(u"strokeMode"_s).toInt(), 1);
        QCOMPARE(annotation.value(u"pen"_s).toObject().value(u"color"_s).toString(), u"#ffff0080"_s);
        double sx = 0;
        double sy = 0;
        for (const QJsonValue p : annotation.value(u"points"_s).toArray()) {
            sx += p.toArray().at(0).toDouble();
            sy += p.toArray().at(1).toDouble();
        }
        QCOMPARE(sx, 200.0);
        QCOMPARE(sy, 50.0);
        QCOMPARE(annotation.value(u"matrix"_s).toArray().size(), 6);
        const QJsonObject p2 = pages.at(1).toObject();
        QVERIFY(!p2.value(u"background"_s).toObject().value(u"image"_s).toString().isEmpty());
        QCOMPARE(p2.value(u"objects"_s).toArray().at(0).toObject().value(u"transparent"_s).toBool(), true);
    }

    // L'oracle Python écrit « 1.0 » et des réels à 17 chiffres, Qt écrit « 1 » : les textes diffèrent, les
    // valeurs non. Comparer les textes signalerait un écart qui n'en est pas un.
    void comparisonReadsValuesNotJsonText()
    {
        const QByteArray python = "{\n \"a\": \"x\",\n \"b\": [\n  1.0,\n  0.30000000000000004\n ]\n}";
        const QJsonObject qt{{u"a"_s, u"x"_s}, {u"b"_s, QJsonArray{1, 0.3}}};
        QVERIFY(QJsonDocument(qt).toJson(QJsonDocument::Indented) != python);
        QVERIFY(QJsonDocument(qt).toJson(QJsonDocument::Compact) != QJsonDocument::fromJson(python).toJson());
        const QStringList diffs = compareCanonical(QJsonDocument::fromJson(python).object(), qt);
        QVERIFY2(diffs.isEmpty(), qPrintable(diffs.join(u'\n')));
    }

    void comparisonToleratesTinyNumericDifferences()
    {
        const QJsonObject a{{u"x"_s, 1.0}, {u"s"_s, u"abc"_s}, {u"l"_s, QJsonArray{1, 2}}};
        QVERIFY(
            compareCanonical(a, QJsonObject{{u"x"_s, 1.0005}, {u"s"_s, u"abc"_s}, {u"l"_s, QJsonArray{1, 2}}})
                .isEmpty());
        QCOMPARE(
            compareCanonical(a, QJsonObject{{u"x"_s, 1.01}, {u"s"_s, u"abc"_s}, {u"l"_s, QJsonArray{1, 2}}})
                .size(),
            1);
        const QStringList diffs =
            compareCanonical(a, QJsonObject{{u"x"_s, 1.0}, {u"s"_s, u"abd"_s}, {u"l"_s, QJsonArray{1}}});
        QCOMPARE(diffs.size(), 2);
        QVERIFY2(diffs.join(u' ').contains(u"/s"_s), qPrintable(diffs.join(u'\n')));
        // Clé absente du côté C++, puis clé en trop du côté C++.
        QVERIFY(!compareCanonical(a, QJsonObject{{u"x"_s, 1.0}, {u"l"_s, QJsonArray{1, 2}}}).isEmpty());
        const QStringList extra =
            compareCanonical(QJsonObject{{u"x"_s, 1}}, QJsonObject{{u"x"_s, 1}, {u"y"_s, 2}});
        QCOMPARE(extra.size(), 1);
        QVERIFY2(extra.first().startsWith(u"/y"_s), qPrintable(extra.first()));
        // −0 et +0 sont égaux (L2-C1).
        QVERIFY(compareCanonical(QJsonObject{{u"x"_s, -0.0}}, QJsonObject{{u"x"_s, 0.0}}).isEmpty());
    }

    void comparisonStopsAtTheLimit()
    {
        QJsonArray expected;
        QJsonArray actual;
        for (int i = 0; i < 50; ++i) {
            expected.append(i);
            actual.append(i + 10);
        }
        QCOMPARE(compareCanonical(expected, actual, 20).size(), 20);
    }
};

QTEST_GUILESS_MAIN(TstFormatsDump)
#include "tst_formats_dump.moc"
