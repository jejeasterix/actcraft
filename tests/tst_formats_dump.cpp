#include "support/flipchartwriter.h"

#include <actcraft/formats/flipchart.h>
#include <actcraft/formats/flipchartdump.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QTest>

using namespace actcraft::formats::flipchart;
using namespace Qt::StringLiterals;

// Export canonique (L2-T10) sur le paperboard synthétique, et comparaison tolérante.
class TstFormatsDump : public QObject
{
    Q_OBJECT

private slots:
    void dumpFollowsTheRules()
    {
        const FlipchartReadResult r = readFlipchart(fctest::sampleFlipchart());
        QVERIFY(r.ok);
        const QJsonObject d = canonicalDump(r.document);
        QCOMPARE(d.value(u"format"_s).toString(), u"actcraft-flipchart-dump/1"_s);
        QCOMPARE(d.value(u"title"_s).toString(), u"Essai synthétique"_s);
        const QJsonArray pages = d.value(u"pages"_s).toArray();
        QCOMPARE(pages.size(), 2);
        const QJsonObject p1 = pages.at(0).toObject();
        QCOMPARE(p1.value(u"background"_s).toObject().value(u"gradient"_s).toInt(), 1);
        QCOMPARE(p1.value(u"background"_s).toObject().value(u"color"_s).toString(), u"#c8dcffff"_s);
        QCOMPARE(p1.value(u"grid"_s).toObject().value(u"levels"_s).toInt(), 1);
        const QJsonArray objects = p1.value(u"objects"_s).toArray();
        QCOMPARE(objects.size(), 4);
        const QJsonObject group = objects.at(0).toObject();
        QCOMPARE(group.value(u"kind"_s).toString(), u"group"_s);
        QCOMPARE(group.value(u"members"_s).toArray(), (QJsonArray{1, 3}));
        const QJsonObject shape = objects.at(1).toObject();
        QCOMPARE(shape.value(u"brush"_s).toObject().value(u"kind"_s).toString(), u"solid"_s);
        QCOMPARE(shape.value(u"brush"_s).toObject().value(u"color2"_s).toString(), QString());
        QCOMPARE(shape.value(u"pen"_s).toObject().value(u"style"_s).toInt(), 2);
        QCOMPARE(shape.value(u"geometry"_s).toObject().value(u"polygons"_s).toInt(), 2);
        QCOMPARE(shape.value(u"geometry"_s).toObject().value(u"points"_s).toInt(), 7);
        const QJsonObject action = shape.value(u"actions"_s).toArray().at(0).toObject();
        QCOMPARE(action.value(u"type"_s).toInt(), 28);
        QCOMPARE(action.value(u"command"_s).toInt(), 184);
        QCOMPARE(action.value(u"target"_s).toInt(), 2);
        const QJsonObject annotation = objects.at(3).toObject();
        QCOMPARE(annotation.value(u"kind"_s).toString(), u"annotation"_s);
        QCOMPARE(annotation.value(u"layer"_s).toInt(), 2);
        QCOMPARE(annotation.value(u"strokeMode"_s).toInt(), 1);
        QCOMPARE(annotation.value(u"pen"_s).toObject().value(u"color"_s).toString(), u"#ffff0080"_s);
        QCOMPARE(annotation.value(u"geometry"_s).toObject().value(u"sum"_s).toArray(),
                 (QJsonArray{200.0, 50.0}));
        QCOMPARE(annotation.value(u"matrix"_s).toArray().size(), 6);
        const QJsonObject p2 = pages.at(1).toObject();
        QVERIFY(!p2.value(u"background"_s).toObject().value(u"image"_s).toString().isEmpty());
        QCOMPARE(p2.value(u"objects"_s).toArray().at(0).toObject().value(u"transparent"_s).toBool(), true);
    }

    // Les règles de la table, pas les valeurs brutes du modèle : couche 0 à 2 (le modèle numérote 1 à 3),
    // indices dans la liste exportée (pas les identifiants), style nul sans largeur, color2 vide hors
    // dégradé. Identifiants choisis loin des indices : un export qui recopierait le modèle échoue ici.
    void dumpProjectsTheTableNotTheModel()
    {
        using namespace actcraft::core;
        Shape drawn;
        drawn.kind = ShapeKind::Path;
        drawn.pen.style = PenStyle::Dash;
        drawn.pen.width = 0;
        drawn.brush = solidFill(0xFF112233u);
        drawn.brush.color2 = 0xFF445566u;
        PageObject shape(drawn);
        shape.common.id = 10;
        shape.common.layer = Layer::Top;
        Action onGroup;
        onGroup.sourceType = 28;
        onGroup.command = 184;
        onGroup.target = 30;
        Action elsewhere;
        elsewhere.sourceType = 23;
        elsewhere.target = 99; // objet absent de la page
        shape.common.actions = {onGroup, elsewhere};
        Group refs;
        refs.members = {10, 99};
        PageObject group(refs);
        group.common.id = 30;
        group.common.layer = Layer::Bottom;
        Page page;
        page.objects = {group, shape};
        std::vector<Page> pages;
        pages.push_back(std::move(page));
        const Document document(std::move(pages));

        const QJsonArray objects = canonicalDump(document)
                                       .value(u"pages"_s)
                                       .toArray()
                                       .at(0)
                                       .toObject()
                                       .value(u"objects"_s)
                                       .toArray();
        QCOMPARE(objects.size(), 2);
        const QJsonObject g = objects.at(0).toObject();
        QCOMPARE(g.value(u"layer"_s).toInt(), 0);
        QCOMPARE(g.value(u"members"_s).toArray(), (QJsonArray{1}));
        const QJsonObject s = objects.at(1).toObject();
        QCOMPARE(s.value(u"layer"_s).toInt(), 2);
        QCOMPARE(s.value(u"pen"_s).toObject().value(u"style"_s).toInt(), 0);
        QCOMPARE(s.value(u"brush"_s).toObject().value(u"color2"_s).toString(), QString());
        QCOMPARE(s.value(u"brush"_s).toObject().value(u"color"_s).toString(), u"#112233ff"_s);
        const QJsonArray actions = s.value(u"actions"_s).toArray();
        QCOMPARE(actions.size(), 2);
        QCOMPARE(actions.at(0).toObject().value(u"target"_s).toInt(), 0);
        QCOMPARE(actions.at(1).toObject().value(u"target"_s).toInt(), -1);
        QCOMPARE(actions.at(1).toObject().value(u"command"_s).toInt(), 0);
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
        QVERIFY(!compareCanonical(a, QJsonObject{{u"x"_s, 1.0}, {u"l"_s, QJsonArray{1, 2}}}).isEmpty());
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
