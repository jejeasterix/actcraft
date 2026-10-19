#include "support/renderhost.h"

#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/generator.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/render/fixedpageitem.h>

#include <QBuffer>
#include <QImage>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTest>

using namespace actcraft;

namespace {
QColor argb(core::Argb c)
{
    return QColor::fromRgba(c);
}
} // namespace

class TstRenderFixedPage : public QObject
{
    Q_OBJECT

private:
    struct Fixture
    {
        QQuickWindow window;
        render::FixedPageItem* page = nullptr;
    };

    static void open(Fixture& f)
    {
        f.window.setColor(QColor(0x3c, 0x3f, 0x45));
        f.window.resize(1024, 768);
        f.page = new render::FixedPageItem;
        f.page->setSize(QSizeF(1024, 768));
        f.page->setParentItem(f.window.contentItem());
        f.window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&f.window));
    }

    // Rendu logiciel de Qt 6.12 : un nœud ajouté sous une racine QSGNode déjà rendue perd la transformation
    // de l'élément. Élément en (100, 50), échelle 2 : (x, y) de l'élément -> (100 + 2x, 50 + 2y) de la
    // fenêtre.
    static void openTransformed(Fixture& f)
    {
        f.window.setColor(QColor(0x3c, 0x3f, 0x45));
        f.window.resize(800, 600);
        f.page = new render::FixedPageItem;
        f.page->setSize(QSizeF(300, 200));
        f.page->setPosition(QPointF(100, 50));
        f.page->setTransformOrigin(QQuickItem::TopLeft);
        f.page->setScale(2.0);
        f.page->setParentItem(f.window.contentItem());
        f.window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&f.window));
    }

    static core::Stroke lineAt(double y)
    {
        core::Stroke stroke;
        stroke.baseWidth = 6;
        stroke.color = 0xFF000000u;
        stroke.points = {{10, y, 0.5f}, {60, y, 0.5f}, {110, y, 0.5f}};
        return stroke;
    }

    // Marqueur à bouts ronds : moteur peint (usesPaintedRenderer), donc un nœud rastérisé par trait sous
    // setForceRasterized. Amendé par L3-T11 : un stylo à bouts ronds passe désormais par la géométrie ; le
    // marqueur (opaque ici, la couleur du test est gardée) reste au moteur peint.
    static core::Stroke roundLineAt(double y)
    {
        core::Stroke stroke = lineAt(y);
        stroke.cap = core::PenCap::Round;
        stroke.kind = core::StrokeKind::Highlighter;
        return stroke;
    }

    static void waitFrame(Fixture& f)
    {
        QSignalSpy swapped(&f.window, &QQuickWindow::frameSwapped);
        f.page->update();
        QVERIFY(swapped.wait(2000));
    }

    static void verifyTransformedStroke(Fixture& f, const char* label)
    {
        const QImage image = f.window.grabWindow();
        QVERIFY2(colorNear(image.pixelColor(220, 70), QColor(Qt::black), 40), label);
        QVERIFY2(colorNear(image.pixelColor(60, 10), f.window.color(), 1), label);
        QVERIFY2(colorNear(image.pixelColor(650, 400), QColor(Qt::white), 1), label);
        QVERIFY2(colorNear(image.pixelColor(50, 10), f.window.color(), 1), label);
    }

private slots:
    void strokeAppendedAfterFirstFrameKeepsTransform()
    {
        Fixture f;
        openTransformed(f);
        f.page->setPage(core::Page{});
        QTRY_VERIFY_WITH_TIMEOUT(f.page->isReady(), 2000);
        waitFrame(f);
        f.page->appendStroke(lineAt(10));
        verifyTransformedStroke(f, "trait ajouté après la première image");
    }

    void rebuildKeepsTransform()
    {
        Fixture f;
        openTransformed(f);
        f.page->setPage(core::Page{});
        QTRY_VERIFY_WITH_TIMEOUT(f.page->isReady(), 2000);
        waitFrame(f);
        core::Page page;
        page.objects.emplace_back(lineAt(10));
        f.page->setPage(std::move(page));
        QTRY_VERIFY_WITH_TIMEOUT(f.page->isReady(), 20000);
        verifyTransformedStroke(f, "reconstruction (setPage)");
        f.page->setSize(QSizeF(310, 200));
        f.page->setSize(QSizeF(300, 200));
        verifyTransformedStroke(f, "reconstruction (geometryChange)");
    }
    void generatedPageBecomesReadyWithWitnesses()
    {
        Fixture f;
        open(f);
        f.page->loadGeneratedPage();
        QTRY_VERIFY_WITH_TIMEOUT(f.page->isReady(), 20000);
        QCOMPARE(f.page->objectCount(), 300);
        const QImage image = f.window.grabWindow();
        const core::Witnesses& w = core::witnesses();
        const auto at = [&](core::PointF p) {
            return image.pixelColor(static_cast<int>(p.x), static_cast<int>(p.y));
        };
        QVERIFY(colorNear(at(w.shapeRect.center()), argb(w.shapeColor), 3));
        QVERIFY(colorNear(at(w.imageRect.center()), argb(w.imageCenterColor), 3));
        QVERIFY(colorNear(at(w.strokeProbe), argb(w.strokeColor), 10));
    }

    void appendedStrokeIsDrawnAtTop()
    {
        Fixture f;
        open(f);
        f.page->loadGeneratedPage();
        QTRY_VERIFY_WITH_TIMEOUT(f.page->isReady(), 20000);
        QSignalSpy appended(f.page, &render::FixedPageItem::strokeAppended);
        core::Stroke stroke;
        stroke.baseWidth = 6;
        stroke.color = 0xFF000000u;
        stroke.points = {{300, 690, 0.5f}, {400, 690, 0.5f}, {500, 690, 0.5f}};
        f.page->appendStroke(stroke);
        QCOMPARE(appended.count(), 1);
        QCOMPARE(f.page->objectCount(), 301);
        const QImage image = f.window.grabWindow();
        QVERIFY(colorNear(image.pixelColor(400, 690), QColor(Qt::black), 40));
    }

    void emptyPageIsWhite()
    {
        Fixture f;
        open(f);
        f.page->setPage(core::Page{});
        QTRY_VERIFY_WITH_TIMEOUT(f.page->isReady(), 2000);
        QCOMPARE(f.page->objectCount(), 0);
        const QImage image = f.window.grabWindow();
        QVERIFY(colorNear(image.pixelColor(512, 384), QColor(Qt::white), 1));
    }

    void solidBackgroundColorIsDrawn()
    {
        Fixture f;
        open(f);
        core::Page page;
        page.background.fill = core::solidFill(0xFF00FF00u);
        f.page->setPage(page);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(500, 400), QColor(Qt::green), 2));
    }

    // Revue L1-T9 (L2-T23) : pendant le décodage d'une autre page, l'ancienne reste affichée (jamais un fond
    // blanc nu) et la scène n'est reconstruite qu'une fois, au prêt.
    void keepsTheOldContentUntilTheNewPageIsReady()
    {
        Fixture f;
        open(f);
        core::Page first;
        first.background.fill = core::solidFill(0xFF0000FFu);
        f.page->setPage(first);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(500, 400), QColor(Qt::blue), 2));
        const int rebuilds = f.page->rebuildCount();
        f.page->setPage(
            core::generatePage(core::kDefaultSeed)); // 30 images à synthétiser : pas prêt aussitôt
        QVERIFY(!f.page->isReady());
        waitFrame(f);
        QVERIFY(!f.page->isReady()); // sinon la capture suivante ne prouverait rien
        // Tant que la page suivante n'est pas prête, l'ancienne reste affichée (jamais un fond blanc nu).
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(500, 400), QColor(Qt::blue), 2));
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        QVERIFY(!colorNear(f.window.grabWindow().pixelColor(500, 400), QColor(Qt::blue), 2));
        QCOMPARE(f.page->rebuildCount() - rebuilds,
                 1); // ni fond blanc intermédiaire, ni reconstruction à vide
    }

    // Revue L2-T23 : des traits posés pendant le décodage d'une page plus courte que l'ancienne ne relancent
    // pas le décodage (sinon des traits plus rapides que lui repoussent le prêt sans fin) ; la page s'affiche
    // avec.
    void strokesDuringDecodeDoNotRestartIt()
    {
        Fixture f;
        open(f);
        f.page->setPage(core::generatePage(core::kDefaultSeed)); // 300 objets
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        core::Page five;
        for (const core::PageObject& object : core::generatePage(core::kDefaultSeed + 1).objects) {
            if (std::get_if<core::ImageObject>(&object) && five.objects.size() < 5) {
                five.objects.push_back(object);
            }
        }
        QCOMPARE(five.objects.size(), std::size_t{5});
        const int decodes = f.page->decodeCount();
        f.page->setPage(five);
        QVERIFY(!f.page->isReady());
        QSignalSpy appended(f.page, &render::FixedPageItem::strokeAppended);
        f.page->appendStroke(lineAt(700));
        f.page->appendStroke(lineAt(720));
        // Chemin rapide de fin de page, comme avant L2-T23 : le changement de page oublie les 300 nœuds de
        // l'ancienne (aucun ajout « en fin de scène » ne se compare à eux).
        QCOMPARE(appended.count(), 2);
        // Notification du cache sur la même page hors fin de scène (reconstruction complète) : elle non plus
        // ne relance pas le décodage en cours.
        f.page->remove(core::ObjectId{1}, core::RectF{});
        QCOMPARE(f.page->decodeCount(), decodes + 1);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        QCOMPARE(f.page->decodeCount(), decodes + 1);
        QCOMPARE(f.page->objectCount(), 7);
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(60, 700), QColor(Qt::black), 40));
    }

    // Revue L2-T23 : page suivante plus grande, pendant son décodage : l'ancienne page reste, posée sur un
    // fond de page blanc à la nouvelle taille (pas de trou à la couleur de la fenêtre).
    void largerNextPageGetsAWhiteBackgroundWhileDecoding()
    {
        Fixture f;
        open(f);
        core::Page first;
        first.width = 600;
        first.height = 400;
        first.background.fill = core::solidFill(0xFF0000FFu);
        f.page->setSize(QSizeF(600, 400));
        f.page->setPage(first);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        f.page->setPage(core::generatePage(core::kDefaultSeed));
        f.page->setSize(QSizeF(1024, 768)); // comme PageArea : l'élément prend la taille de la page affichée
        waitFrame(f);
        QVERIFY(!f.page->isReady());
        const QImage image = f.window.grabWindow();
        QVERIFY(colorNear(image.pixelColor(300, 200), QColor(Qt::blue), 2));
        QVERIFY(colorNear(image.pixelColor(800, 600), QColor(Qt::white), 1));
        // Revenue à la taille de la page montrée avant le prêt : le fond blanc ne déborde plus (2e revue).
        f.page->setSize(QSizeF(600, 400));
        waitFrame(f);
        QVERIFY(!f.page->isReady());
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(800, 600), QColor(0x3c, 0x3f, 0x45), 2));
        QTRY_VERIFY(f.page->isReady());
    }

    // Revue L2-T23 (2e tour) : le magasin range les images synthétiques par INDICE d'objet. Une suppression
    // avant elles pendant le décodage décale ces indices : le décodage en cours ne couvre plus la page et
    // repart, sinon B prend l'image de A jusqu'au décodage suivant.
    void removalBeforeImagesDuringDecodeRedecodes()
    {
        Fixture f;
        open(f);
        const auto image = [](double x, core::Argb color) -> core::PageObject {
            core::ImageObject im;
            im.bounds = {x, 100, 200, 200};
            im.pattern = core::ImagePattern::SolidCenter;
            im.colorA = color;
            im.colorB = color;
            return im;
        };
        core::Document d;
        const core::PageObject stroke = lineAt(600);
        for (core::PageObject object : {stroke, image(100, 0xFF43A047u), image(500, 0xFF1E88E5u)}) {
            object.common.id = d.allocateId();
            d.page(0).objects.push_back(object);
        }
        f.page->setSource(&d, 0);
        QVERIFY(!f.page->isReady());
        const core::ObjectId strokeId = d.page(0).objects.front().common.id;
        d.page(0).objects.erase(d.page(0).objects.begin());
        f.page->remove(strokeId, core::RectF{});
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        const QImage shot = f.window.grabWindow();
        QVERIFY(colorNear(shot.pixelColor(200, 200), QColor(0x43, 0xa0, 0x47), 4)); // A, verte
        QVERIFY(colorNear(shot.pixelColor(600, 200), QColor(0x1e, 0x88, 0xe5), 4)); // B, bleue
    }

    void showsThePageOfItsDocument()
    {
        Fixture f;
        open(f);
        core::Document d;
        d.appendPage();
        core::PageObject stroke = lineAt(300);
        stroke.common.id = d.allocateId();
        stroke.common.layer = core::Layer::Top;
        core::insertAtTopOfLayer(d.page(1), stroke);
        f.page->setSource(&d, 1);
        QCOMPARE(f.page->sourceDocument(), &d);
        QCOMPARE(f.page->sourcePage(), std::size_t{1});
        QCOMPARE(&f.page->page(), &d.page(1)); // lue dans le document, jamais copiée
        QCOMPARE(f.page->objectCount(), 1);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(60, 300), QColor(Qt::black), 40));
    }

    void insertAtTheEndAppendsWithoutRebuilding()
    {
        Fixture f;
        open(f);
        core::Document d;
        f.page->setSource(&d, 0);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        const int rebuilds = f.page->rebuildCount();
        QSignalSpy changed(f.page, &render::FixedPageItem::contentChanged);
        QSignalSpy appended(f.page, &render::FixedPageItem::strokeAppended);
        core::PageObject stroke = lineAt(200);
        stroke.common.id = d.allocateId();
        stroke.common.layer = core::Layer::Top;
        core::insertAtTopOfLayer(d.page(0), stroke);
        f.page->insert(stroke.common.id);
        waitFrame(f);
        QCOMPARE(f.page->rebuildCount(), rebuilds);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(appended.count(), 1);
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(60, 200), QColor(Qt::black), 40));
    }

    // Fluidité : un objet ajouté en fin de page par la pile (AddObjectCommand → applyToCache) ne construit
    // que son propre nœud ; les N objets déjà là ne sont ni reconstruits ni rastérisés à nouveau.
    void insertedObjectBuildsOnlyItsOwnNode()
    {
        Fixture f;
        open(f);
        f.page->setForceRasterized(true);
        core::Document d;
        constexpr int kObjects = 40;
        for (int i = 0; i < kObjects; ++i) {
            core::PageObject stroke = roundLineAt(10.0 + 15.0 * i);
            stroke.common.id = d.allocateId();
            core::insertAtTopOfLayer(d.page(0), stroke);
        }
        f.page->setSource(&d, 0);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        const int rebuilds = f.page->rebuildCount();
        const int rasterized = f.page->rasterizationCount();
        QVERIFY(rasterized >= kObjects); // le compteur voit bien les nœuds des objets de la page
        core::AddObjectCommand add(0, roundLineAt(700));
        core::Changes changes;
        add.redo(d, changes);
        core::applyToCache(changes, 0, *f.page);
        waitFrame(f);
        QCOMPARE(f.page->rebuildCount(), rebuilds);
        QCOMPARE(f.page->rasterizationCount() - rasterized, 1);
        QCOMPARE(f.page->objectCount(), kObjects + 1);
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(60, 700), QColor(Qt::black), 40));
    }

    void insertBelowAnotherObjectRebuilds()
    {
        Fixture f;
        open(f);
        core::Document d;
        core::PageObject top = lineAt(200);
        top.common.id = d.allocateId();
        top.common.layer = core::Layer::Top;
        core::insertAtTopOfLayer(d.page(0), top);
        f.page->setSource(&d, 0);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        const int rebuilds = f.page->rebuildCount();
        core::PageObject middle = lineAt(400);
        middle.common.id = d.allocateId();
        core::insertAtTopOfLayer(d.page(0), middle); // couche intermédiaire : sous le trait du dessus
        f.page->insert(middle.common.id);
        waitFrame(f);
        QCOMPARE(f.page->rebuildCount(), rebuilds + 1);
        QCOMPARE(f.page->objectCount(), 2);
    }

    void removeAndInvalidateRebuild()
    {
        Fixture f;
        open(f);
        core::Document d;
        core::PageObject stroke = lineAt(200);
        stroke.common.id = d.allocateId();
        core::insertAtTopOfLayer(d.page(0), stroke);
        f.page->setSource(&d, 0);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        const int rebuilds = f.page->rebuildCount();
        d.page(0).objects.clear();
        f.page->remove(stroke.common.id, core::boundsOnPage(stroke));
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        QCOMPARE(f.page->rebuildCount(), rebuilds + 1);
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(60, 200), QColor(Qt::white), 1));
        d.page(0).background = core::Background{};
        d.page(0).background.fill = core::solidFill(0xFF00FF00u);
        f.page->invalidate(0, std::nullopt);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        QCOMPARE(f.page->rebuildCount(), rebuilds + 2);
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(500, 400), QColor(Qt::green), 2));
    }

    void undoingAStrokeDoesNotRedecode()
    {
        Fixture f;
        open(f);
        std::vector<core::Page> pages;
        pages.push_back(core::generatePage(core::kDefaultSeed));
        core::Document d(std::move(pages));
        f.page->setSource(&d, 0);
        QTRY_VERIFY_WITH_TIMEOUT(f.page->isReady(), 20000);
        waitFrame(f);
        QSignalSpy ready(f.page, &render::FixedPageItem::readyChanged);
        core::PageObject stroke = lineAt(690);
        stroke.common.layer = core::Layer::Top;
        core::AddObjectCommand add(0, stroke);
        core::Changes changes;
        add.redo(d, changes);
        core::applyToCache(changes, 0, *f.page);
        QVERIFY(f.page->isReady());
        waitFrame(f);
        changes.clear();
        add.undo(d, changes);
        core::applyToCache(changes, 0, *f.page);
        QVERIFY(f.page->isReady());
        waitFrame(f);
        QCOMPARE(ready.count(), 0);
        QVERIFY(f.page->isReady());
        QCOMPARE(f.page->objectCount(), 300);
    }

    // Relais du lot 1 (contrôleur, banc) : setPage repart de zéro, readyChanged compris ; la vue ne redessine
    // son cache que sur readyChanged tant qu'elle n'écoute pas contentChanged (L2-T15).
    void setPageRestartsReadiness()
    {
        Fixture f;
        open(f);
        f.page->setPage(core::Page{});
        QTRY_VERIFY(f.page->isReady());
        QSignalSpy ready(f.page, &render::FixedPageItem::readyChanged);
        core::Page next;
        next.objects.emplace_back(lineAt(200));
        f.page->setPage(std::move(next));
        QTRY_COMPARE(ready.count(), 2);
        QVERIFY(f.page->isReady());
    }

    void appendStrokeIsRefusedWhileShowingADocument()
    {
        Fixture f;
        open(f);
        core::Document d;
        f.page->setSource(&d, 0);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("appendStroke")));
        f.page->appendStroke(lineAt(100));
        QVERIFY(d.page(0).objects.empty());
        QCOMPARE(f.page->objectCount(), 0);
    }

    void detachingFallsBackToAnEmptyInternalPage()
    {
        Fixture f;
        open(f);
        core::Page former;
        former.objects.emplace_back(lineAt(400));
        f.page->setPage(std::move(former)); // ancienne page interne : ne doit pas revenir au détachement
        core::Document d;
        core::PageObject stroke = lineAt(200);
        stroke.common.id = d.allocateId();
        core::insertAtTopOfLayer(d.page(0), stroke);
        f.page->setSource(&d, 0);
        QCOMPARE(f.page->objectCount(), 1);
        f.page->setSource(nullptr, 0);
        QVERIFY(f.page->sourceDocument() == nullptr);
        QCOMPARE(f.page->objectCount(), 0);
    }

    // Une image et un média ajoutés en fin de page par la pile sont décodés avant d'être montrés.
    void insertedImagesAreDecoded()
    {
        Fixture f;
        open(f);
        core::Document d;
        QImage blue(32, 32, QImage::Format_ARGB32);
        blue.fill(Qt::blue);
        QByteArray png;
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        QVERIFY(blue.save(&buffer, "PNG"));
        const std::string key = d.addResource(png, "png");
        f.page->setSource(&d, 0);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        // Média d'abord : un média décodé par la reconstruction de l'image ne prouverait rien. Chaque objet
        // est vérifié dès son ajout.
        core::Media media;
        media.bounds = core::RectF{500, 100, 200, 150};
        media.placeholder = key;
        core::ImageObject image;
        image.bounds = core::RectF{100, 100, 200, 150};
        image.resource = key;
        const std::pair<core::PageObject, QPoint> inserted[] = {{core::PageObject(media), QPoint(600, 175)},
                                                                {core::PageObject(image), QPoint(200, 175)}};
        for (const auto& [shown, center] : inserted) {
            core::PageObject object = shown;
            object.common.layer = core::Layer::Top;
            core::AddObjectCommand add(0, object);
            core::Changes changes;
            add.redo(d, changes);
            core::applyToCache(changes, 0, *f.page);
            QTRY_VERIFY(f.page->isReady());
            waitFrame(f);
            QVERIFY(colorNear(f.window.grabWindow().pixelColor(center), QColor(Qt::blue), 2));
        }
    }

    void setSourceRefusesAMissingPage()
    {
        Fixture f;
        open(f);
        core::Document d;
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("setSource")));
        f.page->setSource(&d, 3);
        QVERIFY(f.page->sourceDocument() == nullptr);
        QCOMPARE(f.page->objectCount(), 0);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
    }

    // La page affichée retirée du document avant que le contrôleur ne rattache l'élément : jamais de lecture
    // hors des bornes, l'élément montre la page interne vide.
    void removedShownPageIsNeverReadOutOfBounds()
    {
        Fixture f;
        open(f);
        core::Document d;
        d.appendPage();
        core::PageObject stroke = lineAt(200);
        stroke.common.id = d.allocateId();
        core::insertAtTopOfLayer(d.page(1), stroke);
        f.page->setSource(&d, 1);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        d.takePage(1);
        QCOMPARE(f.page->objectCount(), 0);
        f.page->invalidate(0, std::nullopt);
        QTRY_VERIFY(f.page->isReady());
        waitFrame(f);
        QVERIFY(colorNear(f.window.grabWindow().pixelColor(60, 200), QColor(Qt::white), 1));
    }
};

QTEST_MAIN(TstRenderFixedPage)
#include "tst_render_fixedpage.moc"
