#include <actcraft/core/document.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/ui/designmode.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pagethumbnails.h>

#include <QtTest>

using namespace actcraft;

namespace {

const QColor kBackground(Qt::white);

// Page de 400 × 300, fond blanc, sans grille, un rectangle rouge plein masqué au centre (150..250, 100..200).
core::Page hiddenShapePage()
{
    core::Page page;
    page.width = 400;
    page.height = 300;
    page.grid.visible = false;
    core::Shape shape;
    shape.geometry = core::ShapeGeometry::Rectangle;
    shape.rect = core::Rect{150, 100, 100, 100};
    shape.fill = 0xFFFF0000u;
    shape.common.visible = false;
    page.objects.emplace_back(std::move(shape));
    return page;
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
    // garde le fond au centre de l'objet, et l'empreinte du document ne bouge pas à la bascule.
    void thumbnailsNeverShowHidden()
    {
        QVERIFY(!render::PaintOptions{}.showHiddenTranslucent);

        ui::DocumentController controller;
        core::Document document;
        document.pages.push_back(hiddenShapePage());
        controller.setDocument(std::move(document));
        const QString before = controller.fingerprintText();

        ui::DesignMode mode;
        mode.setActive(true);
        QVERIFY(mode.active());
        QVERIFY(mode.hiddenShown());
        QCOMPARE(controller.fingerprintText(), before);

        ui::PageThumbnails thumbnails;
        thumbnails.setController(&controller);
        thumbnails.refreshAll();
        QTRY_VERIFY(!thumbnails.image(0).isNull());
        QCOMPARE(centerOf(thumbnails.image(0)), kBackground);

        const QImage png = QImage::fromData(render::thumbnailPng(controller.document().pages[0], {}), "PNG");
        QCOMPARE(centerOf(png), kBackground);

        mode.setActive(false);
        QCOMPARE(controller.fingerprintText(), before);
    }
};

QTEST_MAIN(TstUiDesignmode)
#include "tst_ui_designmode.moc"
