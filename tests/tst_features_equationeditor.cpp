#include <actcraft/features/equationeditor.h>

#include <QtTest>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

using actcraft::features::EquationEditor;
using namespace Qt::StringLiterals;

class TstFeaturesEquationEditor : public QObject
{
    Q_OBJECT

private slots:
    // Modèle fraction, « 1 », case suivante, « 2 » : la forme canonique de l'équation ne porte aucune marque
    // de l'éditeur, alors que le curseur est sur le dénominateur ; la forme de l'aperçu, elle, le porte.
    void fractionTemplateThenTwoDigits()
    {
        EquationEditor e;
        QVERIFY(e.insertTemplate(u"fraction"_s));
        e.type(u"1"_s);
        e.moveRight(); // case suivante : dénominateur
        e.type(u"2"_s);
        QCOMPARE(QString::fromStdString(e.mathml()), u"<math><mfrac><mn>1</mn><mn>2</mn></mfrac></math>"_s);
        QVERIFY(!e.hasPlaceholder());
        QCOMPARE(QString::fromStdString(e.editorMathml()),
                 u"<math><mfrac><mn>1</mn><mn class=\"actcraft-cursor\">2</mn></mfrac></math>"_s);
        QCOMPARE(QString::fromStdString(e.documentMathml().value_or("<refusée>")),
                 u"<math><mfrac><mn>1</mn><mn>2</mn></mfrac></math>"_s);
    }

    // Effacer le contenu d'une case d'un modèle laisse une case vide : l'arité de l'élément reste juste (un
    // effacement qui retirerait le nœud écrirait un <msup> à un seul enfant).
    void backspaceLeavesAPlaceholder()
    {
        EquationEditor e;
        QVERIFY(e.insertTemplate(u"power"_s));
        e.type(u"x"_s);
        QCOMPARE(QString::fromStdString(e.mathml()),
                 u"<math><msup><mi>x</mi><mrow class=\"actcraft-placeholder\"/></msup></math>"_s);
        e.backspace();
        QVERIFY(e.hasPlaceholder());
        QCOMPARE(QString::fromStdString(e.mathml()),
                 u"<math><msup><mrow class=\"actcraft-placeholder\"/><mrow class=\"actcraft-placeholder\"/>"
                 u"</msup></math>"_s);
        QCOMPARE(e.cursorPath(), (std::vector<int>{0, 0}));
        QVERIFY(!e.documentMathml().has_value()); // incomplète : refusée
    }

    // Les flèches passent d'une case à l'autre dans l'ordre du texte ; haut et bas passent du numérateur au
    // dénominateur ; aux bouts, le curseur reste.
    void arrowsWalkTheTree()
    {
        EquationEditor e;
        QVERIFY(e.insertTemplate(u"fraction"_s));
        QCOMPARE(e.cursorPath(), (std::vector<int>{0, 0}));
        e.moveRight();
        QCOMPARE(e.cursorPath(), (std::vector<int>{0, 1}));
        e.moveRight();
        QCOMPARE(e.cursorPath(), (std::vector<int>{0, 1}));
        e.moveLeft();
        QCOMPARE(e.cursorPath(), (std::vector<int>{0, 0}));
        e.moveLeft();
        QCOMPARE(e.cursorPath(), (std::vector<int>{0, 0}));
        e.moveDown();
        QCOMPARE(e.cursorPath(), (std::vector<int>{0, 1}));
        e.moveUp();
        QCOMPARE(e.cursorPath(), (std::vector<int>{0, 0}));
    }

    void digitsGroupIntoOneNumber()
    {
        EquationEditor e;
        e.type(u"1"_s);
        e.type(u"2"_s);
        QCOMPARE(QString::fromStdString(e.mathml()), u"<math><mn>12</mn></math>"_s);
        e.type(u"x+"_s);
        QCOMPARE(QString::fromStdString(e.mathml()), u"<math><mn>12</mn><mi>x</mi><mo>+</mo></math>"_s);
    }

    void loadThenEditKeepsTheTree()
    {
        EquationEditor e;
        QString error;
        QVERIFY2(e.load("<math><msqrt><mi>x</mi></msqrt></math>", error), qPrintable(error));
        e.moveRight();
        e.type(u"+"_s);
        e.type(u"1"_s);
        // msqrt est une suite (arité libre, core/src/equation.cpp) : la saisie s'y ajoute sans mrow.
        QCOMPARE(QString::fromStdString(e.mathml()),
                 u"<math><msqrt><mi>x</mi><mo>+</mo><mn>1</mn></msqrt></math>"_s);
        // Sous une fraction (arité fixe), la saisie après un nœud l'enveloppe dans un mrow.
        QVERIFY(e.load("<math><mfrac><mi>a</mi><mi>b</mi></mfrac></math>", error));
        e.type(u"+1"_s);
        QCOMPARE(QString::fromStdString(e.mathml()),
                 u"<math><mfrac><mrow><mi>a</mi><mo>+</mo><mn>1</mn></mrow><mi>b</mi></mfrac></math>"_s);
        QVERIFY(!e.load("<math><mfrac>", error));
        QVERIFY(!error.isEmpty());
    }

    // Une marque d'éditeur lue dans un MathML (équation d'un document plus ancien) n'en sort jamais vers le
    // document : le curseur est retiré, la case vide fait refuser l'équation.
    void documentMathmlHasNoEditorMark()
    {
        EquationEditor e;
        QString error;
        QVERIFY2(e.load("<math><mi class=\"actcraft-cursor\">x</mi></math>", error), qPrintable(error));
        QCOMPARE(QString::fromStdString(e.documentMathml().value_or("<refusée>")), u"<math><mi>x</mi></math>"_s);
        QVERIFY2(e.load("<math><mfrac><mn>1</mn><mrow class=\"actcraft-placeholder\"/></mfrac></math>", error),
                 qPrintable(error));
        QVERIFY(e.hasPlaceholder());
        QVERIFY(!e.documentMathml().has_value());
    }

    // Les 17 modèles de L7-T24 (tst_core_equation, templatesRoundTrip) ont chacun une entrée dans une des
    // douze palettes (rapport L7-T0) ; toute entrée nomme un modèle connu, et tout modèle s'insère.
    void everyTemplateHasAPaletteEntry()
    {
        const std::vector<QString> expected = {
            u"<math><mfrac><mrow/><mrow/></mfrac></math>"_s,
            u"<math><msup><mrow/><mrow/></msup></math>"_s,
            u"<math><msub><mrow/><mrow/></msub></math>"_s,
            u"<math><msqrt><mrow/></msqrt></math>"_s,
            u"<math><mroot><mrow/><mrow/></mroot></math>"_s,
            u"<math><mrow><mo>(</mo><mrow/><mo>)</mo></mrow></math>"_s,
            u"<math><munderover><mo>∑</mo><mrow/><mrow/></munderover><mrow/></math>"_s,
            u"<math><msubsup><mo>∫</mo><mrow/><mrow/></msubsup><mrow/></math>"_s,
            u"<math><mrow><mo>(</mo><mtable><mtr><mtd/><mtd/></mtr><mtr><mtd/><mtd/></mtr></mtable><mo>)</mo>"
            u"</mrow></math>"_s,
            u"<math><mi>α</mi><mi>ω</mi><mi>Δ</mi><mi>Ω</mi></math>"_s,
            u"<math><mrow><mo>|</mo><mrow/><mo>|</mo></mrow></math>"_s,
            u"<math><mrow><mo>(</mo><mtable><mtr><mtd/><mtd/></mtr></mtable><mo>)</mo></mrow></math>"_s,
            u"<math><mrow><mo>(</mo><mtable><mtr><mtd/></mtr><mtr><mtd/></mtr></mtable><mo>)</mo></mrow></math>"_s,
            u"<math><mrow><mo>(</mo><mtable><mtr><mtd/><mtd/><mtd/></mtr><mtr><mtd/><mtd/><mtd/></mtr><mtr><mtd/>"
            u"<mtd/><mtd/></mtr></mtable><mo>)</mo></mrow></math>"_s,
            u"<math><msub><mo>∬</mo><mrow/></msub><mrow/></math>"_s,
            u"<math><msub><mo>∭</mo><mrow/></msub><mrow/></math>"_s,
            u"<math><mrow><mi>sin</mi><mo>(</mo><mrow/><mo>)</mo></mrow></math>"_s,
        };
        QCOMPARE(expected.size(), std::size_t(17));

        const auto& templates = EquationEditor::templates();
        const auto& palettes = EquationEditor::palettes();
        const std::vector<QString> categories = {
            u"fractions"_s, u"roots"_s,    u"scripts"_s,  u"underover"_s, u"accents"_s,   u"delimiters"_s,
            u"operators"_s, u"integrals"_s, u"matrices"_s, u"functions"_s, u"relations"_s, u"greek"_s,
        };
        QCOMPARE(palettes.size(), categories.size());
        std::set<QString> listed;
        for (std::size_t i = 0; i < palettes.size(); ++i) {
            QCOMPARE(palettes[i].id, categories[i]);
            QVERIFY2(!palettes[i].label.isEmpty(), qPrintable(palettes[i].id));
            QVERIFY2(!palettes[i].entries.empty(), qPrintable(palettes[i].id));
            for (const QString& id : palettes[i].entries) {
                listed.insert(id);
                const bool known = std::any_of(templates.begin(), templates.end(),
                                               [&](const auto& t) { return t.id == id; });
                QVERIFY2(known, qPrintable(id));
            }
        }
        for (const QString& mathml : expected) {
            const auto found = std::find_if(templates.begin(), templates.end(), [&](const auto& t) {
                return QString::fromStdString(t.mathml) == mathml;
            });
            QVERIFY2(found != templates.end(), qPrintable(mathml));
            QVERIFY2(listed.count(found->id) == 1, qPrintable(found->id));
        }
        for (const auto& t : templates) {
            QVERIFY2(!t.label.isEmpty(), qPrintable(t.id));
            EquationEditor e;
            QVERIFY2(e.insertTemplate(t.id), qPrintable(t.id));
            QVERIFY2(!e.mathml().empty(), qPrintable(t.id));
        }
        EquationEditor e;
        QVERIFY(!e.insertTemplate(u"no-such-template"_s));
    }
};

QTEST_GUILESS_MAIN(TstFeaturesEquationEditor)
#include "tst_features_equationeditor.moc"
