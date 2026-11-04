#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/textedits.h>
#include <actcraft/core/undo.h>

#include <QtTest>

using namespace actcraft::core;

class TstCoreTextedits : public QObject
{
    Q_OBJECT

    static Document withText()
    {
        Document d;
        Text before;
        before.bounds = {10, 10, 100, 30};
        before.html = "<p>un</p>";
        PageObject t{before};
        t.common.id = 5;
        d.page(0).objects.push_back(t);
        d.refreshNextId();
        return d;
    }

    // L6-P1 : textes 5 et 6, trait 7.
    static Document withTwoTextsAndAStroke()
    {
        Document d = withText();
        Text second;
        second.bounds = {10, 60, 100, 30};
        second.html = "<p>deux</p>";
        PageObject t{second};
        t.common.id = 6;
        d.page(0).objects.push_back(t);
        PageObject stroke{Stroke{}};
        stroke.common.id = 7;
        d.page(0).objects.push_back(stroke);
        d.refreshNextId();
        return d;
    }

private slots:
    void setTextIsOneStep()
    {
        Document d = withText();
        UndoStack stack;
        Text after;
        after.bounds = {10, 10, 200, 30};
        after.html = "<p>deux</p>";
        const Changes c = stack.push(d, std::make_unique<SetTextCommand>(0, 5, after));
        QCOMPARE(c.size(), std::size_t{1});
        QCOMPARE(c[0].kind, Change::Kind::ObjectChanged);
        QCOMPARE(c[0].area.width, 200.0);
        QCOMPARE(QString::fromStdString(std::get<Text>(body(d.page(0).objects[0])).html),
                 QStringLiteral("<p>deux</p>"));
        QCOMPARE(stack.count(), std::size_t{1});
    }
    // L6-P1 : plusieurs textes remplacés en une seule étape d'annulation.
    void setTextsIsOneStep()
    {
        Document d = withTwoTextsAndAStroke();
        UndoStack stack;
        Text a = std::get<Text>(body(d.page(0).objects[0]));
        Text b = std::get<Text>(body(d.page(0).objects[1]));
        a.html = "<p style=\"color:#ff0000\">un</p>";
        b.html = "<p style=\"color:#ff0000\">deux</p>";
        const Changes c = stack.push(
            d, std::make_unique<SetTextsCommand>(0, std::vector<std::pair<ObjectId, Text>>{{5, a}, {6, b}}));
        QCOMPARE(c.size(), std::size_t{2});
        QCOMPARE(c[0].kind, Change::Kind::ObjectChanged);
        QCOMPARE(c[0].object, ObjectId{5}); // dans l'ordre donné
        QCOMPARE(c[1].object, ObjectId{6});
        QVERIFY(stack.canUndo());
        QCOMPARE(QString::fromStdString(std::get<Text>(body(d.page(0).objects[0])).html),
                 QString::fromStdString(a.html));
        QCOMPARE(QString::fromStdString(std::get<Text>(body(d.page(0).objects[1])).html),
                 QString::fromStdString(b.html));
    }

    // Une seule annulation rend les deux textes ; la pile est alors vide.
    void setTextsUndoRestoresEach()
    {
        Document d = withTwoTextsAndAStroke();
        UndoStack stack;
        Text a = std::get<Text>(body(d.page(0).objects[0]));
        Text b = std::get<Text>(body(d.page(0).objects[1]));
        a.html = "<p>A</p>";
        b.html = "<p>B</p>";
        stack.push(
            d, std::make_unique<SetTextsCommand>(0, std::vector<std::pair<ObjectId, Text>>{{5, a}, {6, b}}));
        const Changes c = stack.undo(d);
        QCOMPARE(c.size(), std::size_t{2});
        QCOMPARE(QString::fromStdString(std::get<Text>(body(d.page(0).objects[0])).html),
                 QStringLiteral("<p>un</p>"));
        QCOMPARE(QString::fromStdString(std::get<Text>(body(d.page(0).objects[1])).html),
                 QStringLiteral("<p>deux</p>"));
        QVERIFY(!stack.canUndo());
    }

    // Trait et objet absent : ignorés ; rien trouvé, aucun Change, le trait intact.
    void setTextsSkipsWhatIsNotAText()
    {
        Document d = withTwoTextsAndAStroke();
        UndoStack stack;
        Text any;
        any.html = "<p>x</p>";
        const Changes c = stack.push(d, std::make_unique<SetTextsCommand>(
                                            0, std::vector<std::pair<ObjectId, Text>>{{7, any}, {99, any}}));
        QVERIFY(c.empty());
        QVERIFY(std::holds_alternative<Stroke>(body(d.page(0).objects[2])));
    }

    void undoRestoresHtmlAndBounds()
    {
        Document d = withText();
        UndoStack stack;
        Text after;
        after.html = "<p>deux</p>";
        stack.push(d, std::make_unique<SetTextCommand>(0, 5, after));
        stack.undo(d);
        const auto& t = std::get<Text>(body(d.page(0).objects[0]));
        QCOMPARE(QString::fromStdString(t.html), QStringLiteral("<p>un</p>"));
        QCOMPARE(t.bounds.width, 100.0);
    }
    void notATextIsIgnored()
    {
        Document d = withText();
        PageObject s{Shape{}};
        s.common.id = 6;
        d.page(0).objects.push_back(s);
        UndoStack stack;
        QVERIFY(stack.push(d, std::make_unique<SetTextCommand>(0, 6, Text{})).empty());
        QVERIFY(stack.push(d, std::make_unique<SetTextCommand>(0, 42, Text{})).empty());
    }
    void baselineFromFirstRun()
    {
        Text t;
        TextSpan a;
        a.baseline = {0, 22};
        TextSpan b;
        b.baseline = {0, 44};
        t.runs = {a, b};
        QCOMPARE(textBaseline(t)->y, 22.0);
    }
    void noRunsNoBaseline() { QVERIFY(!textBaseline(Text{}).has_value()); }
    // L4-C5 : la ligne de base mesurée ne vaut que pour un texte horizontal, dans son emprise.
    void measuredBaselineIsUsedInsideTheBounds()
    {
        Text t;
        t.bounds = {10, 10, 100, 40};
        t.firstBaseline = PointF{12, 30};
        QCOMPARE(textBaseline(t)->y, 30.0);
    }
    void measuredBaselineOutsideTheBoundsIsIgnored()
    {
        Text t;
        t.bounds = {10, 10, 100, 40};
        t.firstBaseline = PointF{12, 90}; // périmée : le texte a rétréci depuis
        QVERIFY(!textBaseline(t).has_value());
    }
    void measuredBaselineOfAVerticalTextIsIgnored()
    {
        Text t;
        t.bounds = {10, 10, 100, 40};
        t.vertical = true;
        t.firstBaseline = PointF{12, 30};
        QVERIFY(!textBaseline(t).has_value());
    }
};

QTEST_GUILESS_MAIN(TstCoreTextedits)
#include "tst_core_textedits.moc"
