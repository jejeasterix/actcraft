#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/core/pagesettings.h>
#include <actcraft/core/undo.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/gridoverlayitem.h>

#include <QQuickWindow>
#include <QTest>

#include <cstdlib>
#include <memory>
#include <utility>
#include <vector>

using namespace actcraft;

namespace {

bool near(QColor a, QColor b, int tolerance)
{
    return std::abs(a.red() - b.red()) <= tolerance && std::abs(a.green() - b.green()) <= tolerance &&
           std::abs(a.blue() - b.blue()) <= tolerance;
}

// Un niveau, pas de 32, noir, lignes pleines (valeurs par défaut de GridLevel).
core::Grid oneLevelGrid(bool drawnAbove)
{
    core::Grid grid;
    grid.visible = true;
    grid.drawnAbove = drawnAbove;
    grid.levels.push_back(core::GridLevel{});
    return grid;
}

core::Document documentWithGrid(const core::Grid& grid)
{
    core::Page page;
    page.grid = grid;
    std::vector<core::Page> pages;
    pages.push_back(std::move(page));
    return core::Document(std::move(pages));
}

} // namespace

// Lot 4 (F-620, L4-T7) : la grille du dessus est une surcouche hors de la scène que le cache photographie ;
// elle couvre tout ce qui est posé (L4-O2, D17 bis). Rendu logiciel hors écran, comme tous les tests.
class TstRenderGridoverlay : public QObject
{
    Q_OBJECT

private:
    struct Fixture
    {
        QQuickWindow window;
        render::FixedPageItem* page = nullptr;
        render::GridOverlayItem* overlay = nullptr;
    };

    // Page fixe (si `withPage`, sinon hors de la scène : seule la surcouche est dessinée) puis surcouche, dans
    // cet ordre, sur la page 0 de `document`. `zoom` : la page est agrandie comme par la vue (échelle de son
    // parent) et la surcouche le sait (propriété zoom, branchée sur view.zoom dans PageArea.qml).
    static void open(Fixture& f, const core::Document& document, bool withPage, qreal zoom = 1)
    {
        f.window.setColor(Qt::white);
        f.window.resize(320, 320);
        QQuickItem* parent = f.window.contentItem();
        if (zoom != 1) {
            auto* zoomed = new QQuickItem(parent);
            zoomed->setTransformOrigin(QQuickItem::TopLeft);
            zoomed->setScale(zoom);
            parent = zoomed;
        }
        const QSizeF size(320 / zoom, 320 / zoom);
        f.page = new render::FixedPageItem;
        f.page->setSize(size);
        if (withPage) {
            f.page->setParentItem(parent);
        } else {
            f.page->setParent(&f.window);
        }
        f.overlay = new render::GridOverlayItem;
        f.overlay->setSize(size);
        f.overlay->setZoom(zoom);
        f.overlay->setParentItem(parent);
        f.page->setSource(&document, 0);
        f.overlay->setFixedPage(f.page);
        f.window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&f.window));
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
    }

    static QImage grab(Fixture& f) { return f.window.grabWindow(); }

private slots:
    void drawsOnlyTheGridAbove()
    {
        {
            // Grille du dessous : dessinée par la page fixe (sous les objets) ; la surcouche ne dessine rien.
            const core::Document d = documentWithGrid(oneLevelGrid(false));
            Fixture f;
            open(f, d, false);
            const QImage image = grab(f);
            for (int k = 1; k <= 9; ++k) {
                QVERIFY2(near(image.pixelColor(32 * k, 50), Qt::white, 8), qPrintable(QString::number(k)));
                QVERIFY2(near(image.pixelColor(50, 32 * k), Qt::white, 8), qPrintable(QString::number(k)));
            }
        }
        {
            // Grille du dessus : la surcouche seule dessine ses lignes x = 32 × k et y = 32 × k.
            const core::Document d = documentWithGrid(oneLevelGrid(true));
            Fixture f;
            open(f, d, false);
            const QImage image = grab(f);
            for (int k = 1; k <= 9; ++k) {
                QVERIFY2(near(image.pixelColor(32 * k, 50), Qt::black, 8), qPrintable(QString::number(k)));
                QVERIFY2(near(image.pixelColor(50, 32 * k), Qt::black, 8), qPrintable(QString::number(k)));
            }
            QVERIFY(near(image.pixelColor(48, 50), Qt::white, 8)); // entre deux lignes
        }
    }

    // Le patron écarté (grille du dessus dans la scène de la page fixe, objets insérés avant elle) ne passe
    // pas : masquer la grille du dessus est un PageSettingsChanged, que le cache ignore ; seule la surcouche
    // se refait, et la ligne disparaît sans que la page fixe soit reconstruite.
    void hidingTheGridAboveRebuildsNoCache()
    {
        core::Document d = documentWithGrid(oneLevelGrid(true));
        Fixture f;
        open(f, d, true);
        QVERIFY(near(grab(f).pixelColor(64, 50), Qt::black, 8));
        const int pageRebuilds = f.page->rebuildCount();
        const int overlayRebuilds = f.overlay->rebuildCount();
        core::Grid hidden = oneLevelGrid(true);
        hidden.visible = false;
        core::UndoStack stack;
        const core::Changes changes = stack.push(d, std::make_unique<core::SetGridCommand>(0, hidden));
        core::applyToCache(changes, 0, *f.page);
        f.overlay->refresh(); // ce que fait PageArea.qml sur pageContentChanged de la page affichée
        QVERIFY(near(grab(f).pixelColor(64, 50), Qt::white, 8));
        QCOMPARE(f.page->rebuildCount(), pageRebuilds);
        QCOMPARE(f.overlay->rebuildCount(), overlayRebuilds + 1);
    }

    // Zoom 2 : une ligne fait exactement un pixel d'écran, sans pixel intermédiaire (comme BORD grille-nette
    // du lot 10).
    void sharpOnePixelLines()
    {
        const core::Document d = documentWithGrid(oneLevelGrid(true));
        Fixture f;
        open(f, d, false, 2);
        const QImage image = grab(f);
        std::vector<int> darkColumns; // le long de y = 50
        std::vector<int> darkRows;    // le long de x = 50
        QStringList intermediate;
        const auto classify = [&intermediate](QColor c, int i, std::vector<int>& dark) {
            if (near(c, Qt::white, 8)) {
                return;
            }
            dark.push_back(i);
            if (!near(c, Qt::black, 8)) {
                intermediate << QStringLiteral("%1 : %2").arg(i).arg(c.name());
            }
        };
        for (int i = 16; i < 304; ++i) {
            classify(image.pixelColor(i, 50), i, darkColumns);
            classify(image.pixelColor(50, i), i, darkRows);
        }
        QVERIFY2(intermediate.isEmpty(), qPrintable(intermediate.join(QStringLiteral(", "))));
        const std::vector<int> expected{64, 128, 192, 256}; // 32 × k pixels de page, × 2
        QCOMPARE(darkColumns, expected);
        QCOMPARE(darkRows, expected);
    }

    // PageSettingsChanged n'atteint pas la page fixe : la surcouche relit la grille sur refresh(), pas à
    // chaque rendu.
    void refreshRereadsTheGrid()
    {
        core::Document d = documentWithGrid(oneLevelGrid(true));
        Fixture f;
        open(f, d, false);
        QVERIFY(near(grab(f).pixelColor(64, 50), Qt::black, 8));
        const int rebuilds = f.overlay->rebuildCount();
        d.page(0).grid.visible = false; // la page source change, sans signal
        f.overlay->update();
        grab(f);
        QCOMPARE(f.overlay->rebuildCount(), rebuilds);
        f.overlay->refresh();
        const QImage image = grab(f);
        QCOMPARE(f.overlay->rebuildCount(), rebuilds + 1);
        for (int k = 1; k <= 9; ++k) {
            QVERIFY2(near(image.pixelColor(32 * k, 50), Qt::white, 8), qPrintable(QString::number(k)));
        }
    }

    // Amendement 5 (D17 bis, L4-O2) : la grille du dessus passe sur un objet posé, quel que soit son calque ;
    // la surcouche ne découpe rien autour de lui.
    void gridAboveCoversAPlacedShape()
    {
        core::Document d = documentWithGrid(oneLevelGrid(true));
        core::Shape shape;
        shape.kind = core::ShapeKind::Rectangle;
        shape.preset = "rectangle";
        shape.bounds = core::RectF{100, 100, 200, 200};
        shape.fill = 0xFFFF0000u;
        shape.outline = 0x00000000u;
        core::PageObject object = shape;
        object.common.id = d.allocateId();
        object.common.layer = core::Layer::Middle;
        core::insertAtTopOfLayer(d.page(0), std::move(object));
        Fixture f;
        open(f, d, true);
        const QImage image = grab(f);
        const QColor red(255, 0, 0);
        QVERIFY2(near(image.pixelColor(128, 150), Qt::black, 8), qPrintable(image.pixelColor(128, 150).name()));
        QVERIFY2(near(image.pixelColor(140, 150), red, 8), qPrintable(image.pixelColor(140, 150).name()));
        QVERIFY2(near(image.pixelColor(150, 160), Qt::black, 8), qPrintable(image.pixelColor(150, 160).name()));
    }
};

QTEST_MAIN(TstRenderGridoverlay)
#include "tst_render_gridoverlay.moc"
