#include "support/indexedimage.h"
#include "support/renderhost.h"

#include <actcraft/core/document.h>
#include <actcraft/core/generator.h>
#include <actcraft/render/imagestore.h>
#include <actcraft/render/imagesynth.h>

#include <QBuffer>
#include <QSignalSpy>
#include <QTest>
#include <QThread>

using namespace actcraft;

namespace {

QByteArray pngOf(const QImage& image)
{
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return png;
}

} // namespace

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

    void resourceImagesAreDecodedOffThreadAndMasked()
    {
        QImage source(4, 4, QImage::Format_ARGB32);
        source.fill(Qt::white);
        source.setPixelColor(1, 1, Qt::red);
        core::Document document;
        const std::string key = document.addResource(pngOf(source), "png");
        core::ImageObject plain;
        plain.bounds = {0, 0, 4, 4};
        plain.resource = key;
        core::ImageObject masked = plain;
        masked.hasKeyColor = true;
        masked.keyColor = 0xFFFFFFFFu;
        core::Page page;
        page.objects.emplace_back(plain);
        page.objects.emplace_back(masked);
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(page, &document);
        QVERIFY(ready.wait(5000));
        QVERIFY(store.generatedOffOwnerThread());
        QCOMPARE(store.resourceImage(key).size(), QSize(4, 4));
        QCOMPARE(store.objectImage(plain, 0).pixelColor(0, 0), QColor(Qt::white));
        const QImage cut = store.objectImage(masked, 1);
        QCOMPARE(cut.pixelColor(0, 0).alpha(), 0);
        QCOMPARE(cut.pixelColor(1, 1), QColor(Qt::red));
        QVERIFY(render::ImageStore::variantKey(masked) != render::ImageStore::variantKey(plain));
        QCOMPARE(store.imageCount(), std::size_t{0}); // aucune image procédurale
    }

    // L2-C7 : une ressource indexée est rendue sans table (même règle que le peintre partagé : rien à lire
    // hors de la table), ses couleurs intactes, et sa variante à couleur transparente se calcule sur l'image
    // convertie, même quand la couleur n'est pas dans la table.
    void indexedResourceHasNoColorTable()
    {
        QImage source = indexedtest::indexedImage(4, 4, 2, 0, 0);
        source.setColor(1, qRgb(255, 255, 255));
        source.scanLine(1)[1] = 1; // blanc
        core::Document document;
        const std::string key = document.addResource(indexedtest::indexedPng(source), "png");
        core::ImageObject plain;
        plain.bounds = {0, 0, 4, 4};
        plain.resource = key;
        core::ImageObject white = plain;
        white.hasKeyColor = true;
        white.keyColor = 0xFFFFFFFFu;
        core::ImageObject green = plain;
        green.hasKeyColor = true;
        green.keyColor = 0xFF00FF00u; // absent de la table
        core::Page page;
        page.objects.emplace_back(plain);
        page.objects.emplace_back(white);
        page.objects.emplace_back(green);
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(page, &document);
        QVERIFY(ready.wait(5000));
        const QImage image = store.objectImage(plain, 0);
        QCOMPARE(image.colorCount(), 0);
        QCOMPARE(image.pixelColor(0, 0), QColor(Qt::red));
        QCOMPARE(image.pixelColor(1, 1), QColor(Qt::white));
        QCOMPARE(store.objectImage(white, 1).pixelColor(1, 1).alpha(), 0);
        QCOMPARE(store.objectImage(white, 1).pixelColor(0, 0), QColor(Qt::red));
        QCOMPARE(store.objectImage(green, 2).pixelColor(1, 1), QColor(Qt::white));
    }

    void placeholderAndBackgroundAreDecoded()
    {
        QImage source(2, 2, QImage::Format_ARGB32);
        source.fill(Qt::blue);
        core::Document document;
        const std::string key = document.addResource(pngOf(source), "png");
        core::Media media;
        media.bounds = {0, 0, 10, 10};
        media.placeholder = key;
        core::Page page;
        page.background.image = key;
        page.objects.emplace_back(media);
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(page, &document);
        QVERIFY(ready.wait(5000));
        QCOMPARE(store.resourceImage(key).pixelColor(0, 0), QColor(Qt::blue));
    }

    void unreadableOrMissingResourcesGiveNullImages()
    {
        core::Document document;
        const std::string bad = document.addResource(QByteArray("pas une image"), "png");
        core::ImageObject a;
        a.resource = bad;
        core::ImageObject b;
        b.resource = "absente";
        core::Page page;
        page.objects.emplace_back(a);
        page.objects.emplace_back(b);
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(page, &document);
        QVERIFY(ready.wait(5000));
        QVERIFY(store.isReady());
        QVERIFY(store.resourceImage(bad).isNull());
        QVERIFY(store.objectImage(b, 1).isNull());
    }

    void resourceImageRendersInSoftware()
    {
        QImage source(8, 8, QImage::Format_ARGB32);
        source.fill(Qt::red);
        core::Document document;
        core::ImageObject im;
        im.bounds = {20, 20, 64, 64};
        im.resource = document.addResource(pngOf(source), "png");
        core::Page page;
        page.objects.emplace_back(im);
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(page, &document);
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
        QVERIFY(colorNear(window.grabWindow().pixelColor(52, 52), QColor(Qt::red), 3));
    }

    void coversTellsWhetherAPageNeedsDecoding()
    {
        core::Page page = core::generatePage();
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(page);
        QVERIFY(ready.wait(20000));
        QVERIFY(store.covers(page, nullptr));
        core::Stroke stroke;
        stroke.points = {{1, 1, 0.5f}};
        page.objects.emplace_back(stroke); // trait ajouté en fin : les images gardent leurs indices
        QVERIFY(store.covers(page, nullptr));
        page.objects.insert(page.objects.begin(), core::PageObject(stroke)); // indices décalés
        QVERIFY(!store.covers(page, nullptr));
    }

    void coversSeesAMissingTransparentVariant()
    {
        QImage source(4, 4, QImage::Format_ARGB32);
        source.fill(Qt::white);
        core::Document document;
        core::ImageObject plain;
        plain.resource = document.addResource(pngOf(source), "png");
        core::ImageObject masked = plain;
        masked.hasKeyColor = true;
        masked.keyColor = 0xFFFFFFFFu;
        core::Page withPlain;
        withPlain.objects.emplace_back(plain);
        core::Page withMasked;
        withMasked.objects.emplace_back(masked);
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(withPlain, &document);
        QVERIFY(ready.wait(5000));
        QVERIFY(store.covers(withPlain, &document));
        QVERIFY(!store.covers(withMasked, &document)); // même ressource, variante jamais calculée
    }

    // L2-C9, constat 3 : une nouvelle génération (PgDown répétés) annule les décodages de la précédente qui
    // n'ont pas commencé ; sans cela, toutes les pages traversées sont décodées. 64 ressources de 512 × 512
    // (quelques ms chacune) : les fils du pool n'en ont commencé qu'une poignée quand la page suivante
    // arrive.
    void newGenerationDropsTheDecodesNotStarted()
    {
        constexpr int kCount = 64;
        core::Document document;
        core::Page crossed;
        for (int i = 0; i < kCount; ++i) {
            QImage noise(512, 512, QImage::Format_RGB32);
            for (int y = 0; y < noise.height(); ++y) {
                auto* line = reinterpret_cast<QRgb*>(noise.scanLine(y));
                for (int x = 0; x < noise.width(); ++x) {
                    line[x] = qRgb((x * 7 + y * 13 + i * 31) & 0xFF, (x * y + i) & 0xFF, (x ^ y ^ i) & 0xFF);
                }
            }
            core::ImageObject im;
            im.bounds = {0, 0, 10, 10};
            im.resource = document.addResource(pngOf(noise), "png");
            crossed.objects.emplace_back(im);
        }
        QImage small(4, 4, QImage::Format_RGB32);
        small.fill(Qt::red);
        core::ImageObject im;
        im.bounds = {0, 0, 4, 4};
        im.resource = document.addResource(pngOf(small), "png");
        core::Page shown;
        shown.objects.emplace_back(im);
        render::ImageStore store;
        QSignalSpy ready(&store, &render::ImageStore::ready);
        store.generate(crossed, &document);
        store.generate(shown, &document); // page suivante aussitôt
        QVERIFY(ready.wait(20000));
        QVERIFY(store.covers(shown, &document));
        QVERIFY(store.waitForIdle(60000));
        const std::size_t started = store.startedTasks();
        QVERIFY2(started < std::size_t{kCount + 1}, qPrintable(QString::number(started)));
    }
};

QTEST_MAIN(TstRenderImages)
#include "tst_render_images.moc"
