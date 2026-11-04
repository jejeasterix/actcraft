// L6-T17 : étiquettes des objets (F-576, D12).
#include <actcraft/core/edits.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/labeltips.h>

#include <QSignalSpy>
#include <QTest>

#include <memory>
#include <string>
#include <utility>

using namespace actcraft;

class TstUiLabels : public QObject
{
    Q_OBJECT

private:
    // Une forme (100,100)-(300,200), de centre (200,150).
    static core::PageObject shape(const std::string& label, bool visible = true)
    {
        core::Shape s;
        s.bounds = {100, 100, 200, 100};
        core::PageObject object{s};
        object.common.label = label;
        object.common.visible = visible;
        return object;
    }

    struct Fixture
    {
        ui::DocumentController document;
        ui::LabelTips tips;
        Fixture() { tips.setDocument(&document); }
        void add(core::PageObject object)
        {
            document.execute(std::make_unique<core::AddObjectCommand>(0, std::move(object)));
        }
    };

private slots:
    void hoverShowsTheLabel()
    {
        Fixture f;
        f.add(shape("Soleil"));
        QVERIFY(!f.tips.isVisible());
        f.tips.hoverAt(QPointF(200, 150));
        QVERIFY(f.tips.isVisible());
        QCOMPARE(f.tips.text(), QStringLiteral("Soleil"));
        f.tips.hoverAt(QPointF(900, 900)); // hors de l'objet
        QVERIFY(!f.tips.isVisible());
        f.tips.hoverAt(QPointF(200, 150));
        f.tips.leave();
        QVERIFY(!f.tips.isVisible());
    }

    void noLabelNoTip()
    {
        Fixture f;
        f.add(shape(""));
        f.tips.hoverAt(QPointF(200, 150));
        QVERIFY(!f.tips.isVisible());
    }

    void hiddenObjectHasNoTip()
    {
        Fixture f;
        f.add(shape("Soleil", false));
        f.tips.hoverAt(QPointF(200, 150));
        QVERIFY(!f.tips.isVisible());
    }

    void styleIsApplied()
    {
        Fixture f;
        core::PageObject object = shape("Soleil");
        object.common.labelStyle.pointSize = 18;
        object.common.labelStyle.color = 0xFFFF0000;
        object.common.labelStyle.fontFamily = "Serif";
        object.common.labelStyle.fill = 0xFFFFFF00;
        object.common.labelStyle.outline = core::PenStyle::Solid;
        object.common.labelStyle.outlineColor = 0xFF0000FF;
        f.add(std::move(object));
        f.tips.hoverAt(QPointF(200, 150));
        QVERIFY(f.tips.isVisible());
        QCOMPARE(f.tips.pointSize(), 18.0);
        QCOMPARE(f.tips.color(), QColor(255, 0, 0));
        QCOMPARE(f.tips.fontFamily(), QStringLiteral("Serif"));
        QCOMPARE(f.tips.fill(), QColor(255, 255, 0));
        QCOMPARE(f.tips.outlineColor(), QColor(0, 0, 255));
    }

    void noOutlineIsTransparent()
    {
        Fixture f;
        f.add(shape("Soleil"));
        f.tips.hoverAt(QPointF(200, 150));
        QVERIFY(f.tips.isVisible());
        QCOMPARE(f.tips.outlineColor().alpha(), 0);
    }

    void hoverCreatesNoStep()
    {
        Fixture f;
        f.add(shape("Soleil"));
        f.document.undo(); // vide la pile ; l'objet disparaît
        f.add(shape("Soleil"));
        const bool canRedo = f.document.canRedo();
        QSignalSpy content(&f.document, &ui::DocumentController::pageContentChanged);
        f.tips.hoverAt(QPointF(200, 150));
        f.tips.leave();
        QCOMPARE(content.count(), 0);
        QCOMPARE(f.document.canRedo(), canRedo);
    }

    void removedObjectClearsTheTip()
    {
        Fixture f;
        f.add(shape("Soleil"));
        f.tips.hoverAt(QPointF(200, 150));
        QVERIFY(f.tips.isVisible());
        f.document.undo();
        QVERIFY(!f.tips.isVisible());
    }
};

QTEST_MAIN(TstUiLabels)
#include "tst_ui_labels.moc"
