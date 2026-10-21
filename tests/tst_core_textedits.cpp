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
};

QTEST_GUILESS_MAIN(TstCoreTextedits)
#include "tst_core_textedits.moc"
