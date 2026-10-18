#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/pendinglayeritem.h>

#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>

#include <algorithm>
#include <cstdlib>

using namespace actcraft;

namespace {

bool near(QColor a, QColor b, int tolerance)
{
    return std::abs(a.red() - b.red()) <= tolerance && std::abs(a.green() - b.green()) <= tolerance &&
           std::abs(a.blue() - b.blue()) <= tolerance;
}

core::Stroke lineAt(double y, core::Argb color = 0xFF000000u)
{
    core::Stroke s;
    s.baseWidth = 8;
    s.color = color;
    for (int i = 0; i <= 20; ++i) {
        s.points.push_back(core::StrokePoint{100.0 + 20.0 * i, y, 0.5f});
    }
    return s;
}

} // namespace

// Lot 10 (DA11) : objets en attente au-dessus du cache. Rendu logiciel hors écran (comme tous les tests).
class TstRenderPendingLayer : public QObject
{
    Q_OBJECT

private:
    struct Fixture
    {
        QQuickWindow window;
        render::FixedPageItem* page = nullptr;
        render::PendingLayerItem* layer = nullptr;
    };

    // `withLayer` faux : la page fixe seule, pour voir ce que SA scène dessine (ce que le cache
    // photographie).
    static void open(Fixture& f, bool withLayer)
    {
        f.window.setColor(Qt::white);
        f.window.resize(640, 480);
        f.page = new render::FixedPageItem;
        f.page->setSize(QSizeF(640, 480));
        f.page->setParentItem(f.window.contentItem());
        if (withLayer) {
            f.layer = new render::PendingLayerItem;
            f.layer->setSize(QSizeF(640, 480));
            f.layer->setParentItem(f.window.contentItem());
            f.layer->setFixedPage(f.page);
        }
        f.page->setPage(core::Page{});
        f.window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&f.window));
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
    }

    static QColor at(Fixture& f, int x, int y) { return f.window.grabWindow().pixelColor(x, y); }

private slots:
    void withoutDeferralNothingWaits()
    {
        Fixture f;
        open(f, false);
        f.page->appendStroke(lineAt(100));
        QCOMPARE(f.page->pendingCount(), 0);
        QCOMPARE(f.page->cachedObjectCount(), std::size_t{1});
        QVERIFY(near(at(f, 300, 100), Qt::black, 60)); // lot 2 : dans la scène tout de suite
    }

    void deferredStrokeIsNotInTheCachedScene()
    {
        Fixture f;
        open(f, false);
        f.page->setDeferredFolding(true);
        QSignalSpy content(f.page, &render::FixedPageItem::contentChanged);
        QSignalSpy pending(f.page, &render::FixedPageItem::pendingChanged);
        f.page->appendStroke(lineAt(100));
        QCOMPARE(f.page->objectCount(), 1);
        QCOMPARE(f.page->pendingCount(), 1);
        QCOMPARE(f.page->cachedObjectCount(), std::size_t{0});
        QCOMPARE(content.count(), 0); // aucun redessin du cache demandé
        QCOMPARE(pending.count(), 1);
        QVERIFY(near(at(f, 300, 100), Qt::white, 8)); // la scène du cache ne l'a pas
        f.page->foldPending();
        QCOMPARE(f.page->pendingCount(), 0);
        QVERIFY(near(at(f, 300, 100), Qt::black, 60)); // versé
    }

    void layerDrawsWhatWaits()
    {
        Fixture f;
        open(f, true);
        f.page->setDeferredFolding(true);
        f.page->appendStroke(lineAt(100));
        f.page->appendStroke(lineAt(200));
        QVERIFY(near(at(f, 300, 100), Qt::black, 60));
        QVERIFY(near(at(f, 300, 200), Qt::black, 60));
        QCOMPARE(f.layer->builtCount(), 2);
    }

    // Même image avant et après le versement : la couche ne dessine plus ce que la scène dessine désormais.
    // Un trait translucide le prouve : dessiné deux fois, il serait plus foncé ; par personne, blanc.
    void foldShowsTheSameImage()
    {
        Fixture f;
        open(f, true);
        // P5 : un trait translucide en attente par-dessus un trait translucide déjà versé (deux passages
        // foncent).
        f.page->appendStroke(lineAt(100, 0x80E53935u)); // sans différé : versé tout de suite
        const QColor single = at(f, 300, 100);
        QVERIFY(!near(single, Qt::white, 8));
        f.page->setDeferredFolding(true);
        f.page->appendStroke(lineAt(100, 0x80E53935u)); // en attente, sur le premier
        QCOMPARE(f.page->pendingCount(), 1);
        const QColor before = at(f, 300, 100);
        QVERIFY2(!near(before, single, 8), "deux passages translucides doivent foncer");
        f.page->foldPending();
        const QColor after = at(f, 300, 100);
        QCOMPARE(f.layer->builtCount(), 0);
        QVERIFY2(near(after, before, 4), qPrintable(after.name() + QStringLiteral(" / ") + before.name()));
    }

    // Le piège du patron écarté : un objet inséré après une demande de versement et AVANT la synchronisation
    // suivante. La scène ne prend que les cachedObjectCount() premiers objets lus à la synchronisation, la
    // couche les suivants : chacun est dessiné une fois. Une scène qui prendrait tous les objets de la page
    // (chemin « fin de page » du lot 2) dessinerait le second dans la scène ET dans la couche (plus foncé).
    void strokeInsertedBeforeTheNextSyncIsDrawnOnce()
    {
        Fixture f;
        open(f, true);
        f.page->appendStroke(lineAt(300, 0x80E53935u)); // repère : un seul passage, versé tout de suite
        const QColor single = at(f, 300, 300);
        QVERIFY(!near(single, Qt::white, 8));
        f.page->setDeferredFolding(true);
        f.page->appendStroke(lineAt(100, 0x80E53935u));
        QVERIFY(near(at(f, 300, 100), single, 4));      // en attente, dessiné par la couche
        f.page->foldPending();                          // versement demandé…
        f.page->appendStroke(lineAt(200, 0x80E53935u)); // …et nouveau trait avant toute synchronisation
        QCOMPARE(f.page->pendingCount(), 1);
        QCOMPARE(f.page->cachedObjectCount(), std::size_t{2});
        const QImage image = f.window.grabWindow();
        const QColor folded = image.pixelColor(300, 100);
        const QColor waiting = image.pixelColor(300, 200);
        QVERIFY2(near(folded, single, 4), qPrintable(folded.name() + QStringLiteral(" / ") + single.name()));
        QVERIFY2(near(waiting, single, 4),
                 qPrintable(waiting.name() + QStringLiteral(" / ") + single.name()));
        QCOMPARE(f.layer->builtCount(), 1);
    }

    // Annulation d'un trait encore en attente, par le chemin de la pile : l'objet quitte la page du document,
    // puis le cache reçoit remove(id, zone). Ni reconstruction, ni redessin du cache demandé.
    void undoingAPendingStrokeRebuildsNothing()
    {
        Fixture f;
        open(f, true);
        core::Document d;
        f.page->setSource(&d, 0);
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        f.page->setDeferredFolding(true);
        core::PageObject stroke = lineAt(300);
        stroke.common.id = d.allocateId();
        stroke.common.layer = core::Layer::Top;
        const core::ObjectId id = stroke.common.id;
        core::insertAtTopOfLayer(d.page(0), std::move(stroke));
        f.page->insert(id);
        QCOMPARE(f.page->pendingCount(), 1);
        QVERIFY(near(at(f, 300, 300), Qt::black, 60));
        const int rebuilds = f.page->rebuildCount();
        QSignalSpy content(f.page, &render::FixedPageItem::contentChanged);
        d.page(0).objects.pop_back();
        f.page->remove(id, core::RectF{100, 296, 400, 8});
        QCOMPARE(f.page->pendingCount(), 0);
        QCOMPARE(content.count(), 0);
        QVERIFY(near(at(f, 300, 300), Qt::white, 8));
        QCOMPARE(f.page->rebuildCount(), rebuilds);
        f.page->setSource(nullptr, 0); // le document meurt avant l'élément
    }

    void anyOtherChangeFoldsEverything()
    {
        Fixture f;
        open(f, true);
        core::Document d;
        f.page->setSource(&d, 0);
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        f.page->setDeferredFolding(true);
        core::PageObject a = lineAt(100);
        a.common.id = d.allocateId();
        a.common.layer = core::Layer::Top;
        const core::ObjectId aId = a.common.id;
        core::insertAtTopOfLayer(d.page(0), std::move(a));
        f.page->insert(aId);
        QCOMPARE(f.page->pendingCount(), 1);
        QSignalSpy content(f.page, &render::FixedPageItem::contentChanged);
        // Objet inséré en tête de page (couche inférieure) : pas en fin de page, reconstruction complète.
        core::PageObject b = lineAt(200);
        b.common.id = d.allocateId();
        b.common.layer = core::Layer::Bottom;
        const core::ObjectId bId = b.common.id;
        core::insertAtTopOfLayer(d.page(0), std::move(b));
        f.page->insert(bId);
        QCOMPARE(f.page->pendingCount(), 0);
        QCOMPARE(content.count(), 1);
        QCOMPARE(f.page->cachedObjectCount(), std::size_t{2});
        f.page->setSource(nullptr, 0); // le document meurt avant l'élément
    }

    // Un trait en attente réordonné en tête de page (ObjectChanged) n'est plus en queue : tout est
    // reconstruit.
    void pendingStrokeMovedToTheFrontFoldsEverything()
    {
        Fixture f;
        open(f, true);
        core::Document d;
        core::PageObject first = lineAt(100);
        first.common.id = d.allocateId();
        first.common.layer = core::Layer::Top;
        core::insertAtTopOfLayer(d.page(0), std::move(first));
        f.page->setSource(&d, 0);
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        f.page->setDeferredFolding(true);
        core::PageObject moved = lineAt(300);
        moved.common.id = d.allocateId();
        moved.common.layer = core::Layer::Top;
        const core::ObjectId id = moved.common.id;
        core::insertAtTopOfLayer(d.page(0), std::move(moved));
        f.page->insert(id);
        QCOMPARE(f.page->pendingCount(), 1);
        QVERIFY(near(at(f, 300, 300), Qt::black, 60));
        const int rebuilds = f.page->rebuildCount();
        auto& objects = d.page(0).objects;
        std::rotate(objects.begin(), objects.end() - 1, objects.end()); // en tête de page
        f.page->invalidate(id, core::RectF{100, 296, 400, 8});
        QCOMPARE(f.page->pendingCount(), 0);
        QVERIFY(near(at(f, 300, 300), Qt::black, 60)); // dans la scène, plus dans la couche
        QCOMPARE(f.layer->builtCount(), 0);
        QVERIFY(f.page->rebuildCount() > rebuilds);
        f.page->setSource(nullptr, 0); // le document meurt avant l'élément
    }

    void pendingAreAlwaysTheLastObjects()
    {
        Fixture f;
        open(f, true);
        f.page->setDeferredFolding(true);
        for (int i = 0; i < 5; ++i) {
            f.page->appendStroke(lineAt(50.0 + 40 * i));
        }
        const core::Page& page = f.page->page();
        const auto& ids = f.page->pendingIds();
        QCOMPARE(ids.size(), std::size_t{5});
        const std::size_t first = page.objects.size() - ids.size();
        QCOMPARE(first, f.page->cachedObjectCount());
        for (std::size_t k = 0; k < ids.size(); ++k) {
            QCOMPARE(page.objects[first + k].common.id, ids[k]);
        }
    }

    void turningDeferralOffFoldsThePending()
    {
        Fixture f;
        open(f, true);
        f.page->setDeferredFolding(true);
        f.page->appendStroke(lineAt(100));
        f.page->setDeferredFolding(false);
        QCOMPARE(f.page->pendingCount(), 0);
        QVERIFY(near(at(f, 300, 100), Qt::black, 60));
    }

    // P8 (L10-T0) : l'encre révélatrice rend transparents les objets sous elle ; jamais en attente, redessin
    // immédiat. Rouge tant que la condition manque à insert (le trait serait mis en attente).
    void revealerStrokeIsNeverDeferred()
    {
        Fixture f;
        open(f, true);
        f.page->setDeferredFolding(true);
        QSignalSpy content(f.page, &render::FixedPageItem::contentChanged);
        core::Stroke revealer = lineAt(100);
        revealer.kind = core::StrokeKind::Revealer;
        f.page->appendStroke(std::move(revealer));
        QCOMPARE(f.page->pendingCount(), 0);
        QCOMPARE(content.count(), 1);
        QCOMPARE(f.page->cachedObjectCount(), std::size_t{1});
    }

    void gridDrawnAboveIsNeverDeferred()
    {
        Fixture f;
        open(f, true);
        core::Page page;
        page.grid.drawnAbove = true;
        f.page->setPage(std::move(page));
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        f.page->setDeferredFolding(true);
        f.page->appendStroke(lineAt(100));
        QCOMPARE(f.page->pendingCount(), 0); // il doit passer sous la grille : dans la scène
    }

    // Revue M2 : un trait en attente modifié en place (couleur), puis invalidate : seule la couche change ;
    // ni reconstruction, ni redessin du cache, et la nouvelle couleur est à l'écran.
    void pendingStrokeChangedInPlaceRebuildsNothing()
    {
        Fixture f;
        open(f, true);
        core::Document d;
        f.page->setSource(&d, 0);
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        f.page->setDeferredFolding(true);
        core::PageObject stroke = lineAt(300);
        stroke.common.id = d.allocateId();
        stroke.common.layer = core::Layer::Top;
        const core::ObjectId id = stroke.common.id;
        core::insertAtTopOfLayer(d.page(0), std::move(stroke));
        f.page->insert(id);
        QVERIFY(near(at(f, 300, 300), Qt::black, 60));
        const int rebuilds = f.page->rebuildCount();
        QSignalSpy content(f.page, &render::FixedPageItem::contentChanged);
        auto* changed = std::get_if<core::Stroke>(&d.page(0).objects.back());
        QVERIFY(changed);
        changed->color = 0xFF1E88E5u;
        f.page->invalidate(id, core::RectF{100, 296, 400, 8});
        QCOMPARE(content.count(), 0);
        QCOMPARE(f.page->pendingCount(), 1);
        const QColor now = at(f, 300, 300);
        QVERIFY2(near(now, QColor(0x1E, 0x88, 0xE5), 60), qPrintable(now.name()));
        QCOMPARE(f.page->rebuildCount(), rebuilds);
        f.page->setSource(nullptr, 0); // le document meurt avant l'élément
    }

    // Revue M1 : une reconstruction qui redécode (image insérée en fin de page pendant qu'un trait attend)
    // vide l'attente, et la page fixe garde ses anciens nœuds jusqu'au prêt. La couche garde les siens
    // jusque-là, sinon le trait disparaît de l'écran ; au prêt, la scène l'a et la couche est vide.
    void pendingStrokeStaysVisibleWhileAnImageDecodes()
    {
        Fixture f;
        open(f, true);
        core::Document d;
        f.page->setSource(&d, 0);
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        f.page->setDeferredFolding(true);
        core::PageObject stroke = lineAt(300);
        stroke.common.id = d.allocateId();
        stroke.common.layer = core::Layer::Top;
        const core::ObjectId strokeId = stroke.common.id;
        core::insertAtTopOfLayer(d.page(0), std::move(stroke));
        f.page->insert(strokeId);
        QCOMPARE(f.page->pendingCount(), 1);
        QVERIFY(near(at(f, 300, 300), Qt::black, 60));
        core::ImageObject picture;
        picture.bounds = core::RectF{400, 20, 200, 100};
        picture.sourceWidth = 2048;
        picture.sourceHeight = 2048;
        core::PageObject image = picture;
        image.common.id = d.allocateId();
        image.common.layer = core::Layer::Top;
        const core::ObjectId imageId = image.common.id;
        core::insertAtTopOfLayer(d.page(0), std::move(image));
        f.page->insert(imageId);
        QCOMPARE(f.page->pendingCount(), 0);
        QVERIFY(!f.page->isReady()); // sinon la capture suivante ne prouverait rien
        QVERIFY2(near(at(f, 300, 300), Qt::black, 60), "trait en attente invisible pendant le décodage");
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 20000));
        QVERIFY(near(at(f, 300, 300), Qt::black, 60));
        QCOMPARE(f.layer->builtCount(), 0);
        f.page->setSource(nullptr, 0); // le document meurt avant l'élément
    }

    // Revue m1 : la couche retirée de la fenêtre puis remise perd son nœud (updatePaintNode reçoit old nul) :
    // elle redessine ce qui attend, au lieu de croire ses anciens nœuds encore posés.
    void layerPutBackInTheWindowRedrawsThePending()
    {
        Fixture f;
        open(f, true);
        f.page->setDeferredFolding(true);
        f.page->appendStroke(lineAt(100));
        QVERIFY(near(at(f, 300, 100), Qt::black, 60));
        f.layer->setParentItem(nullptr);
        QVERIFY(near(at(f, 300, 100), Qt::white, 8));
        f.layer->setParentItem(f.window.contentItem());
        QVERIFY(near(at(f, 300, 100), Qt::black, 60));
        QCOMPARE(f.layer->builtCount(), 1);
    }

    // Revue m4 : la couche passe à une autre page fixe : elle refait tout, même quand les identifiants en
    // attente coïncident (chaque page interne compte les siens à partir du même nombre).
    void switchingPageRebuildsTheLayer()
    {
        Fixture f;
        open(f, true);
        auto* other = new render::FixedPageItem;
        other->setSize(QSizeF(640, 480));
        other->setParentItem(f.window.contentItem());
        other->setVisible(false); // seule la couche dessine ses objets en attente
        other->setPage(core::Page{});
        QVERIFY(QTest::qWaitFor([&] { return other->isReady(); }, 5000));
        f.page->setDeferredFolding(true);
        other->setDeferredFolding(true);
        f.page->appendStroke(lineAt(100));
        other->appendStroke(lineAt(200));
        QVERIFY(f.page->pendingIds() == other->pendingIds()); // mêmes identifiants : le piège
        QVERIFY(near(at(f, 300, 100), Qt::black, 60));
        f.layer->setFixedPage(other);
        QVERIFY(near(at(f, 300, 100), Qt::white, 8));
        QVERIFY(near(at(f, 300, 200), Qt::black, 60));
    }

    // Revue m2 : annuler le dernier trait en attente retire son seul nœud ; les autres ne sont pas refaits.
    void undoingTheLastPendingStrokeKeepsTheOthers()
    {
        Fixture f;
        open(f, true);
        core::Document d;
        f.page->setSource(&d, 0);
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        f.page->setDeferredFolding(true);
        core::ObjectId last = 0;
        for (int i = 0; i < 3; ++i) {
            core::PageObject stroke = lineAt(100.0 + 100 * i);
            stroke.common.id = d.allocateId();
            stroke.common.layer = core::Layer::Top;
            last = stroke.common.id;
            core::insertAtTopOfLayer(d.page(0), std::move(stroke));
            f.page->insert(last);
        }
        QVERIFY(near(at(f, 300, 300), Qt::black, 60));
        QCOMPARE(f.layer->createdCount(), 3);
        d.page(0).objects.pop_back();
        f.page->remove(last, core::RectF{100, 296, 400, 8});
        QVERIFY(near(at(f, 300, 300), Qt::white, 8));
        QVERIFY(near(at(f, 300, 100), Qt::black, 60));
        QCOMPARE(f.layer->builtCount(), 2);
        QCOMPARE(f.layer->createdCount(), 3); // les deux restants n'ont pas été refaits
        f.page->setSource(nullptr, 0);        // le document meurt avant l'élément
    }

    // Revue : l'échelle de rastérisation change pendant qu'un objet attend : la couche le refait (en RHI, un
    // objet peint en attente suit l'échelle comme ceux de la scène).
    void rasterScaleChangeRedrawsThePending()
    {
        Fixture f;
        open(f, true);
        f.page->setDeferredFolding(true);
        f.page->appendStroke(lineAt(100));
        const std::uint64_t revision = f.page->pendingRevision();
        QSignalSpy pending(f.page, &render::FixedPageItem::pendingChanged);
        f.page->setRasterScale(2.0);
        QCOMPARE(pending.count(), 1);
        QVERIFY(f.page->pendingRevision() > revision);
        QVERIFY(near(at(f, 300, 100), Qt::black, 60));
    }
};

QTEST_MAIN(TstRenderPendingLayer)
#include "tst_render_pendinglayer.moc"
