#include <actcraft/core/equation.h>

#include <QtTest>

#include <string>
#include <vector>

using namespace actcraft::core;
using namespace Qt::StringLiterals;

namespace {

// Forme de l'arbre lu : genre de chaque nœud, texte entre crochets, enfants entre parenthèses. Un aller-retour à
// l'identique ne prouve pas qu'un modèle est lu en arbre (un nœud Raw se réécrit aussi à l'identique) : la forme, si.
QString outline(const EquationNode& node)
{
    QString s;
    switch (node.kind) {
    case EquationNode::Kind::Math: s = u"math"_s; break;
    case EquationNode::Kind::Row: s = u"row"_s; break;
    case EquationNode::Kind::Identifier: s = u"mi"_s; break;
    case EquationNode::Kind::Number: s = u"mn"_s; break;
    case EquationNode::Kind::Operator: s = u"mo"_s; break;
    case EquationNode::Kind::Text: s = u"mtext"_s; break;
    case EquationNode::Kind::Fraction: s = u"frac"_s; break;
    case EquationNode::Kind::Sup: s = u"sup"_s; break;
    case EquationNode::Kind::Sub: s = u"sub"_s; break;
    case EquationNode::Kind::SubSup: s = u"subsup"_s; break;
    case EquationNode::Kind::Sqrt: s = u"sqrt"_s; break;
    case EquationNode::Kind::Root: s = u"root"_s; break;
    case EquationNode::Kind::UnderOver: s = u"underover"_s; break;
    case EquationNode::Kind::Under: s = u"under"_s; break;
    case EquationNode::Kind::Over: s = u"over"_s; break;
    case EquationNode::Kind::Table: s = u"table"_s; break;
    case EquationNode::Kind::TableRow: s = u"tr"_s; break;
    case EquationNode::Kind::TableCell: s = u"td"_s; break;
    case EquationNode::Kind::Placeholder: s = u"placeholder"_s; break;
    case EquationNode::Kind::Raw: s = u"raw"_s; break;
    }
    if (!node.text.empty())
        s += u'[' + QString::fromStdString(node.text) + u']';
    if (!node.children.empty()) {
        QStringList children;
        for (const EquationNode& child : node.children)
            children << outline(child);
        s += u'(' + children.join(u',') + u')';
    }
    return s;
}

QString written(const EquationNode& node)
{
    return QString::fromStdString(toMathml(node));
}

std::string nested(const char* open, const char* close, int count)
{
    std::string s = "<math>";
    for (int i = 0; i < count; ++i)
        s += open;
    for (int i = 0; i < count; ++i)
        s += close;
    return s + "</math>";
}

} // namespace

class TstCoreEquation : public QObject
{
    Q_OBJECT

private slots:
    // Les modèles de l'éditeur (rapport L7-T0, « Modèles d'équation retenus » : les 10 formes du rapport, puis les 7
    // proposées), sous la forme canonique : lus en arbre sans nœud Raw, écrits et relus à l'identique.
    void templatesRoundTrip_data()
    {
        QTest::addColumn<QString>("mathml");
        QTest::addColumn<QString>("shape");
        QTest::newRow("fraction") << u"<math><mfrac><mrow/><mrow/></mfrac></math>"_s << u"math(frac(row,row))"_s;
        QTest::newRow("power") << u"<math><msup><mrow/><mrow/></msup></math>"_s << u"math(sup(row,row))"_s;
        QTest::newRow("subscript") << u"<math><msub><mrow/><mrow/></msub></math>"_s << u"math(sub(row,row))"_s;
        QTest::newRow("square-root") << u"<math><msqrt><mrow/></msqrt></math>"_s << u"math(sqrt(row))"_s;
        QTest::newRow("nth-root") << u"<math><mroot><mrow/><mrow/></mroot></math>"_s << u"math(root(row,row))"_s;
        QTest::newRow("parentheses") << u"<math><mrow><mo>(</mo><mrow/><mo>)</mo></mrow></math>"_s
                                     << u"math(row(mo[(],row,mo[)]))"_s;
        QTest::newRow("sum") << u"<math><munderover><mo>∑</mo><mrow/><mrow/></munderover><mrow/></math>"_s
                             << u"math(underover(mo[∑],row,row),row)"_s;
        QTest::newRow("integral") << u"<math><msubsup><mo>∫</mo><mrow/><mrow/></msubsup><mrow/></math>"_s
                                  << u"math(subsup(mo[∫],row,row),row)"_s;
        QTest::newRow("matrix-2x2")
            << u"<math><mrow><mo>(</mo><mtable><mtr><mtd/><mtd/></mtr><mtr><mtd/><mtd/></mtr></mtable><mo>)</mo></mrow></math>"_s
            << u"math(row(mo[(],table(tr(td,td),tr(td,td)),mo[)]))"_s;
        QTest::newRow("greek") << u"<math><mi>α</mi><mi>ω</mi><mi>Δ</mi><mi>Ω</mi></math>"_s
                               << u"math(mi[α],mi[ω],mi[Δ],mi[Ω])"_s;
        QTest::newRow("absolute-value") << u"<math><mrow><mo>|</mo><mrow/><mo>|</mo></mrow></math>"_s
                                        << u"math(row(mo[|],row,mo[|]))"_s;
        QTest::newRow("matrix-1x2")
            << u"<math><mrow><mo>(</mo><mtable><mtr><mtd/><mtd/></mtr></mtable><mo>)</mo></mrow></math>"_s
            << u"math(row(mo[(],table(tr(td,td)),mo[)]))"_s;
        QTest::newRow("matrix-2x1")
            << u"<math><mrow><mo>(</mo><mtable><mtr><mtd/></mtr><mtr><mtd/></mtr></mtable><mo>)</mo></mrow></math>"_s
            << u"math(row(mo[(],table(tr(td),tr(td)),mo[)]))"_s;
        QTest::newRow("matrix-3x3") << u"<math><mrow><mo>(</mo><mtable><mtr><mtd/><mtd/><mtd/></mtr><mtr><mtd/><mtd/>"
                                       u"<mtd/></mtr><mtr><mtd/><mtd/><mtd/></mtr></mtable><mo>)</mo></mrow></math>"_s
                                    << u"math(row(mo[(],table(tr(td,td,td),tr(td,td,td),tr(td,td,td)),mo[)]))"_s;
        QTest::newRow("double-integral") << u"<math><msub><mo>∬</mo><mrow/></msub><mrow/></math>"_s
                                         << u"math(sub(mo[∬],row),row)"_s;
        QTest::newRow("triple-integral") << u"<math><msub><mo>∭</mo><mrow/></msub><mrow/></math>"_s
                                         << u"math(sub(mo[∭],row),row)"_s;
        QTest::newRow("trig-function") << u"<math><mrow><mi>sin</mi><mo>(</mo><mrow/><mo>)</mo></mrow></math>"_s
                                       << u"math(row(mi[sin],mo[(],row,mo[)]))"_s;
    }
    void templatesRoundTrip()
    {
        QFETCH(QString, mathml);
        QFETCH(QString, shape);
        QString error;
        const auto tree = parseMathml(mathml.toStdString(), error);
        QVERIFY2(tree.has_value(), qPrintable(error));
        QCOMPARE(outline(*tree), shape);
        const std::string again = toMathml(*tree);
        QCOMPARE(QString::fromStdString(again), mathml); // écrit à l'identique
        const auto tree2 = parseMathml(again, error);
        QVERIFY2(tree2.has_value(), qPrintable(error));
        QCOMPARE(outline(*tree2), shape); // relu à l'identique
    }

    // Un élément que cette version ne connaît pas est gardé tel qu'écrit (guillemets simples, blancs dans les
    // balises), pas relu puis réécrit : en tête, au milieu et en fin de son parent, vide ou non.
    void unknownElementKeptVerbatim()
    {
        const std::string in = "<math><mrow><mi>a</mi><menclose  notation='box'><mi>b</mi ></menclose></mrow></math>";
        QString error;
        const auto tree = parseMathml(in, error);
        QVERIFY2(tree.has_value(), qPrintable(error));
        QCOMPARE(outline(*tree), u"math(row(mi[a],raw[<menclose  notation='box'><mi>b</mi ></menclose>]))"_s);
        QCOMPARE(written(*tree), QString::fromStdString(in));

        const std::string in2 = "<math><mspace width=\"1em\"/><mi>x</mi>\n <mstyle mathcolor=\"red\"><mi>y</mi>"
                                "</mstyle></math>";
        const auto tree2 = parseMathml(in2, error);
        QVERIFY2(tree2.has_value(), qPrintable(error));
        QCOMPARE(outline(*tree2),
                 u"math(raw[<mspace width=\"1em\"/>],mi[x],raw[<mstyle mathcolor=\"red\"><mi>y</mi></mstyle>])"_s);
        QCOMPARE(written(*tree2),
                 u"<math><mspace width=\"1em\"/><mi>x</mi><mstyle mathcolor=\"red\"><mi>y</mi></mstyle></math>"_s);
    }

    // Les références de caractères sont décodées dans l'arbre ; l'écriture canonique les rend en caractères UTF-8,
    // sauf < > & échappés. La forme gardée par L7-T1 (QString du .flipchart passée par toStdString) se lit.
    void entitiesDecoded()
    {
        QString error;
        const auto tree = parseMathml("<math><mi>&#x3B1;</mi></math>", error);
        QVERIFY2(tree.has_value(), qPrintable(error));
        QCOMPARE(tree->children.size(), std::size_t{1});
        QCOMPARE(QString::fromStdString(tree->children.front().text), u"α"_s);
        QCOMPARE(written(*tree), u"<math><mi>α</mi></math>"_s);

        const auto escaped = parseMathml("<math><mo>&lt;</mo><mo>&amp;</mo><mo>&gt;</mo><mi>&#945;</mi></math>", error);
        QVERIFY2(escaped.has_value(), qPrintable(error));
        QCOMPARE(outline(*escaped), u"math(mo[<],mo[&],mo[>],mi[α])"_s);
        QCOMPARE(written(*escaped), u"<math><mo>&lt;</mo><mo>&amp;</mo><mo>&gt;</mo><mi>α</mi></math>"_s);

        const QString fromFlipchart = u"<math><mfrac><mi>&#x3B1;</mi><mn>2</mn></mfrac></math>"_s;
        const auto read = parseMathml(fromFlipchart.toStdString(), error);
        QVERIFY2(read.has_value(), qPrintable(error));
        QCOMPARE(outline(*read), u"math(frac(mi[α],mn[2]))"_s);
    }

    // Une entrée malformée, trop grande ou hors du sous-ensemble rend une erreur, jamais un plantage ni un accès
    // réseau (aucun DTD, aucune entité externe).
    void malformedIsAnErrorNotACrash()
    {
        const std::vector<std::string> inputs = {
            "<math><mfrac><mn>1</mn></math>",
            "<math><mfrac><mn>1</mn></mfrac></math>",
            "<math><msubsup><mi>x</mi><mn>1</mn></msubsup></math>",
            "<math><mfrac>",
            "",
            "<mrow><mi>x</mi></mrow>",
            "<math/><math/>",
            "<math><math/></math>",
            "<math><mi>&alpha;</mi></math>",
            "<!DOCTYPE math [<!ENTITY a \"b\">]><math><mi>&a;</mi></math>",
            "<!DOCTYPE math SYSTEM \"http://example.invalid/math.dtd\"><math/>",
            "<math><mrow>texte</mrow></math>",
            "<math><mtable><mi>x</mi></mtable></math>",
            "<math><mrow><mtd/></mrow></math>",
            "<math><mi>\xC3\x28</mi></math>",
            "<math><mi>x</mi></math>\xC3",
            std::string(300 * 1024, 'a'),
            "<math><mtext>" + std::string(300 * 1024, 'a') + "</mtext></math>",
        };
        for (const std::string& in : inputs) {
            QString error;
            const QByteArray what = QByteArray::fromStdString(in.substr(0, 60));
            QVERIFY2(!parseMathml(in, error).has_value(), what.constData());
            QVERIFY2(!error.isEmpty(), what.constData());
        }
    }

    // Profondeur d'imbrication bornée (kMaxEquationDepth, math compris), y compris dans un élément gardé en Raw.
    void tooDeepIsAnError()
    {
        QString error;
        QVERIFY2(parseMathml(nested("<mrow>", "</mrow>", kMaxEquationDepth - 1), error).has_value(), qPrintable(error));
        QVERIFY(!parseMathml(nested("<mrow>", "</mrow>", kMaxEquationDepth), error).has_value());
        QVERIFY(!error.isEmpty());
        QVERIFY(!parseMathml(nested("<mrow>", "</mrow>", 80), error).has_value());
        QVERIFY2(parseMathml(nested("<menclose>", "</menclose>", kMaxEquationDepth - 1), error).has_value(),
                 qPrintable(error));
        QVERIFY(!parseMathml(nested("<menclose>", "</menclose>", kMaxEquationDepth), error).has_value());
    }

    // La tranche d'un nœud Raw se prend dans le texte décodé, pas dans les octets UTF-8 : des caractères de 2 et 4
    // octets (une et deux unités UTF-16) avant et dans l'élément inconnu ne la décalent pas, et une référence de
    // caractère y reste écrite comme dans l'original.
    void nonAsciiRawKeptVerbatim()
    {
        const std::string raw = "<menclose notation=\"box\"><mi>é</mi><mo>&#x2264;</mo><mi>𝑥</mi></menclose>";
        const std::string in = "<math><mi>α</mi><mi>𝑥</mi>" + raw + "<mi>ü</mi></math>";
        QString error;
        const auto tree = parseMathml(in, error);
        QVERIFY2(tree.has_value(), qPrintable(error));
        QCOMPARE(tree->children.size(), std::size_t{4});
        QVERIFY(tree->children[2].kind == EquationNode::Kind::Raw);
        QCOMPARE(QString::fromStdString(tree->children[2].text), QString::fromStdString(raw));
        QCOMPARE(written(*tree), QString::fromStdString(in));
    }

    // Les attributs d'un élément reconnu sont gardés et réécrits dans leur ordre (l'espace de noms d'un MathML reçu,
    // par exemple) ; la case vide marquée de l'éditeur se lit en Placeholder et se réécrit avec sa marque.
    void attributesAndPlaceholderKept()
    {
        const std::string in = "<math xmlns=\"http://www.w3.org/1998/Math/MathML\" display=\"block\"><mfrac>"
                               "<mrow class=\"actcraft-placeholder\"/><mi mathvariant=\"normal\">x</mi></mfrac></math>";
        QString error;
        const auto tree = parseMathml(in, error);
        QVERIFY2(tree.has_value(), qPrintable(error));
        QCOMPARE(outline(*tree), u"math(frac(placeholder,mi[x]))"_s);
        using Attributes = std::vector<std::pair<std::string, std::string>>;
        QVERIFY(tree->attributes
                == (Attributes{{"xmlns", "http://www.w3.org/1998/Math/MathML"}, {"display", "block"}}));
        QVERIFY(tree->children[0].children[1].attributes == (Attributes{{"mathvariant", "normal"}}));
        QCOMPARE(written(*tree), QString::fromStdString(in));

        EquationNode math;
        math.kind = EquationNode::Kind::Math;
        EquationNode box;
        box.kind = EquationNode::Kind::Placeholder;
        math.children.push_back(box);
        QCOMPARE(written(math), u"<math><mrow class=\"actcraft-placeholder\"/></math>"_s);
    }

    // Un MathML reçu indenté, avec déclaration XML et commentaire, se lit ; il s'écrit sous la forme canonique.
    void indentedInputIsCanonical()
    {
        const std::string in = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<math>\n  <!-- une moitié -->\n  <mfrac>\n"
                               "    <mn>1</mn>\n    <mn>2</mn>\n  </mfrac>\n</math>\n";
        QString error;
        const auto tree = parseMathml(in, error);
        QVERIFY2(tree.has_value(), qPrintable(error));
        QCOMPARE(written(*tree), u"<math><mfrac><mn>1</mn><mn>2</mn></mfrac></math>"_s);
    }
};

QTEST_GUILESS_MAIN(TstCoreEquation)
#include "tst_core_equation.moc"
