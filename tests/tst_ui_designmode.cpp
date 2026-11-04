#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/ui/designmode.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pagethumbnails.h>

#include <QtTest>

#include <memory>

using namespace actcraft;

namespace {

// Rectangle rouge plein, sans contour, masqué (drapeau du document), centré sur la page de 400 × 300.
core::PageObject hiddenRedRectangle()
{
    core::Shape shape;
    shape.kind = core::ShapeKind::Rectangle;
    shape.bounds = core::RectF{150, 100, 100, 100};
    shape.fill = 0xFFFF0000u;
    shape.outlineWidth = 0;
    core::PageObject object{shape};
    object.common.visible = false;
    return object;
}

// Teinte rouge de l'objet (opaque ou à 40 %) ; fond blanc et grille grise ne le sont pas.
bool tinted(const QColor& c)
{
    return c.red() - c.green() > 40;
}

QColor centerOf(const QImage& image)
{
    return image.pixelColor(image.width() / 2, image.height() / 2);
}

} // namespace

class TstUiDesignmode : public QObject
{
    Q_OBJECT
private slots:
    // Discriminant du patron écarté (amendements 1 et 5) : le mode ne bascule pas `visible` dans le document
    // et ne pose aucune option globale du peintre. Mode actif, la miniature (L4-T17) d'une page à objet masqué
    // n'est pas teintée au centre de l'objet, et l'empreinte du document ne bouge pas à la bascule.
    void thumbnailsNeverShowHidden()
    {
        QVERIFY(!render::PaintOptions{}.showHiddenTranslucent);

        ui::DocumentController controller;
        controller.newDocument(400, 300);
        controller.execute(std::make_unique<core::AddObjectCommand>(0, hiddenRedRectangle()));
        const QString before = controller.fingerprintText();

        ui::DesignMode mode;
        mode.setActive(true);
        QVERIFY(mode.active());
        QVERIFY(mode.hiddenShown());
        QCOMPARE(controller.fingerprintText(), before);

        ui::PageThumbnails thumbnails;
        thumbnails.setDocumentController(&controller);
        thumbnails.refreshAll();
        QTRY_VERIFY_WITH_TIMEOUT(!thumbnails.image(0).isNull(), 10000);
        QVERIFY(!tinted(centerOf(thumbnails.image(0))));

        const QImage png = QImage::fromData(render::thumbnailPng(controller.document().page(0), {}), "PNG");
        QVERIFY(!tinted(centerOf(png)));

        mode.setActive(false);
        QCOMPARE(controller.fingerprintText(), before);
    }
};

QTEST_MAIN(TstUiDesignmode)
#include "tst_ui_designmode.moc"
