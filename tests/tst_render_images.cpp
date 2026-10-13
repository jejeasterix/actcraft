#include "support/renderhost.h"

#include <actcraft/core/generator.h>
#include <actcraft/render/imagestore.h>
#include <actcraft/render/imagesynth.h>

#include <QSignalSpy>
#include <QTest>
#include <QThread>

using namespace actcraft;

class TstRenderImages : public QObject
{
    Q_OBJECT

private slots:
    void solidCenterIsUniform()
    {
        core::ImageObject object;
        object.sourceWidth = 256;
        object.sourceHeight = 256;
        object.pattern = core::ImagePattern::SolidCenter;
        object.colorA = 0xFF43A047u;
        object.colorB = 0xFF7E57C2u;
        const QImage image = render::synthesizeImage(object);
        QCOMPARE(image.size(), QSize(256, 256));
        QCOMPARE(image.pixel(128, 128), QRgb(0xFF43A047u));
        QCOMPARE(image.pixel(70, 70), QRgb(0xFF43A047u));
        QCOMPARE(image.pixel(2, 2), QRgb(0xFF7E57C2u));
    }

    void everyPatternHasSourceSizeAndBothColors()
    {
        for (auto pattern :
             {core::ImagePattern::LinearGradient, core::ImagePattern::Checker, core::ImagePattern::Rings}) {
            core::ImageObject object;
            object.sourceWidth = 320;
            object.sourceHeight = 512;
            object.pattern = pattern;
            object.colorA = 0xFFFF0000u;
            object.colorB = 0xFF0000FFu;
            const QImage image = render::synthesizeImage(object);
            QCOMPARE(image.size(), QSize(320, 512));
            bool hasA = false;
            bool hasB = false;
            for (int y = 0; y < image.height(); ++y) {
                for (int x = 0; x < image.width(); ++x) {
                    hasA = hasA || image.pixel(x, y) == 0xFFFF0000u;
                    hasB = hasB || image.pixel(x, y) == 0xFF0000FFu;
                }
            }
            QVERIFY(hasA && hasB);
        }
    }

    void storeGeneratesAllImagesOffThread()
    {
        const core::Page page = core::generatePage();
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(page);
        QVERIFY(!store.isReady()); // generate() rend la main avant la fin de la génération
        QVERIFY(ready.wait(20000));
        QVERIFY(store.isReady());
        QVERIFY(store.generatedOffOwnerThread()); // chaque image a été synthétisée sur un fil secondaire
        QCOMPARE(store.imageCount(), std::size_t{30});
        for (std::size_t i = 0; i < page.objects.size(); ++i) {
            if (const auto* im = std::get_if<core::ImageObject>(&page.objects[i])) {
                const QImage image = store.image(i);
                QCOMPARE(image.size(), QSize(im->sourceWidth, im->sourceHeight));
            } else {
                QVERIFY(store.image(i).isNull());
            }
        }
    }

    void storeWithoutImagesBecomesReady()
    {
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(core::Page{});
        QVERIFY(ready.wait(1000));
        QVERIFY(store.isReady());
    }

    void imageRendersInSoftware()
    {
        core::ImageObject witness;
        witness.bounds = {20, 20, 64, 64};
        witness.pattern = core::ImagePattern::SolidCenter;
        witness.colorA = 0xFF43A047u;
        witness.colorB = 0xFF7E57C2u;
        core::Page page;
        page.objects.emplace_back(witness);
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(page);
        QVERIFY(ready.wait(5000));

        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(200, 200);
        auto* host = new RenderHostItem;
        host->setSize(QSizeF(200, 200));
        host->objects = page.objects;
        host->images = &store;
        host->setParentItem(window.contentItem());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const QImage image = window.grabWindow();
        QVERIFY(colorNear(image.pixelColor(52, 52), QColor(0x43, 0xA0, 0x47), 3));
        QVERIFY(colorNear(image.pixelColor(23, 23), QColor(0x7E, 0x57, 0xC2), 3));
        QVERIFY(colorNear(image.pixelColor(150, 150), QColor(Qt::white), 1));
    }
};

QTEST_MAIN(TstRenderImages)
#include "tst_render_images.moc"
