#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/liftlayeritem.h>

#include <QImage>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>

#include <cstdlib>
#include <utility>
#include <vector>

using namespace actcraft;

namespace {

const QColor kRed(0xE5, 0x39, 0x35);
const QColor kBlue(0x1E, 0x3A, 0xE5);

bool near(QColor a, QColor b, int tolerance)
{
    return std::abs(a.red() - b.red()) <= tolerance && std::abs(a.green() - b.green()) <= tolerance &&
           std::abs(a.blue() - b.blue()) <= tolerance;
}

// Rectangle bleu plein, sans contour : moteur du prototype (forme).
core::PageObject blueRectangle()
{
    core::Shape s;
    s.kind = core::ShapeKind::Rectangle;
    s.bounds = core::RectF{200, 150, 160, 120};
    s.fill = 0xFF1E3AE5u;
    s.outlineWidth = 0;
    core::PageObject object = s;
    object.common.id = 1;
    return object;
}

// Trait rouge horizontal de x = 100 à x = 500 : traverse le rectangle bleu (x de 200 à 360).
core::PageObject redStroke()
{
    core::Stroke s;
    s.baseWidth = 8;
    s.color = 0xFFE53935u;
    for (int i = 0; i <= 20; ++i) {
        s.points.push_back(core::StrokePoint{100.0 + 20.0 * i, 210.0, 0.5f});
    }
    core::PageObject object = s;
    object.common.id = 2;
    return object;
}

// Ligne verte oblique : moteur peint (vectoriel en logiciel, texture quand la page fixe rastérise).
core::PageObject greenLine()
{
    core::Line l;
    l.points = {core::PointF{120, 320}, core::PointF{520, 400}};
    l.pen.color = 0xFF2E7D32u;
    l.pen.width = 6;
    core::PageObject object = l;
    object.common.id = 3;
    return object;
}

core::Page pageWith(std::vector<core::PageObject> objects)
{
    core::Page page;
    page.objects = std::move(objects);
    return page;
}

// Vide si les deux images sont identiques au pixel ; sinon le nombre de pixels qui diffèrent et le premier.
QString firstDifference(const QImage& a, const QImage& b)
{
    if (a.size() != b.size()) {
        return QStringLiteral("tailles %1x%2 / %3x%4")
            .arg(a.width())
            .arg(a.height())
            .arg(b.width())
            .arg(b.height());
    }
    const QImage left = a.convertToFormat(QImage::Format_ARGB32);
    const QImage right = b.convertToFormat(QImage::Format_ARGB32);
    int count = 0;
    QPoint first;
    for (int y = 0; y < left.height(); ++y) {
        for (int x = 0; x < left.width(); ++x) {
            if (left.pixel(x, y) != right.pixel(x, y)) {
                if (count == 0) {
                    first = QPoint(x, y);
                }
                ++count;
            }
        }
    }
    if (count == 0) {
        return {};
    }
    return QStringLiteral("%1 pixels diffèrent, premier en (%2, %3) : %4 / %5")
        .arg(count)
        .arg(first.x())
        .arg(first.y())
        .arg(left.pixelColor(first).name(QColor::HexArgb), right.pixelColor(first).name(QColor::HexArgb));
}

} // namespace

// Lot 3 (DA11) : couche soulevée. Elle dessine les copies des objets soulevés avec les moteurs et le contexte
// de la page fixe, sous une transformation supplémentaire qui ne reconstruit aucun nœud. Rendu logiciel hors
// écran (comme tous les tests).
class TstRenderLiftlayer : public QObject
{
    Q_OBJECT

private:
    struct Fixture
    {
        QQuickWindow window;
        render::FixedPageItem* page = nullptr;
        render::LiftLayerItem* layer = nullptr;
    };

    // Page fixe vide et couche soulevée sœur, au-dessus, à la même place (ordre de PageArea.qml).
    static void open(Fixture& f)
    {
        f.window.setColor(Qt::white);
        f.window.resize(640, 480);
        f.page = new render::FixedPageItem;
        f.page->setSize(QSizeF(640, 480));
        f.page->setParentItem(f.window.contentItem());
        f.layer = new render::LiftLayerItem;
        f.layer->setSize(QSizeF(640, 480));
        f.layer->setParentItem(f.window.contentItem());
        f.layer->setFixedPage(f.page);
        f.page->setPage(core::Page{});
        f.window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&f.window));
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
    }

    static QColor at(Fixture& f, int x, int y) { return f.window.grabWindow().pixelColor(x, y); }

private slots:
    void drawsLikeTheFixedPage_data()
    {
        QTest::addColumn<bool>("rasterized");
        QTest::newRow("vectoriel") << false;
        // Contexte de la page fixe changé (chemin RHI forcé, échelle 2) : la couche doit le suivre, pas le
        // sien.
        QTest::newRow("rasterise") << true;
    }

    // Capture A : page fixe vide, couche qui montre les objets. Capture B : la page fixe les contient, couche
    // vide. A et B identiques au pixel (mêmes moteurs, même contexte, même ordre : le trait recouvre le
    // rectangle).
    void drawsLikeTheFixedPage()
    {
        QFETCH(bool, rasterized);
        Fixture f;
        open(f);
        if (rasterized) {
            f.page->setForceRasterized(true);
            f.page->setRasterScale(2.0);
            QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        }
        const std::vector<core::PageObject> objects{blueRectangle(), redStroke(), greenLine()};
        f.layer->setObjects(objects);
        QCOMPARE(f.layer->objectCount(), 3);
        const QImage lifted = f.window.grabWindow();
        f.layer->clear();
        f.page->setPage(pageWith(objects));
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        const QImage fixed = f.window.grabWindow();
        // Repères : la page fixe dessine bien les trois objets, le trait au-dessus du rectangle.
        QVERIFY(near(fixed.pixelColor(280, 210), kRed, 60));
        QVERIFY(near(fixed.pixelColor(280, 170), kBlue, 8));
        QVERIFY(
            !near(fixed.pixelColor(320, 360), Qt::white, 8)); // ligne en x = 320 : y = 320 + 200 * 80 / 400
        const QString difference = firstDifference(lifted, fixed);
        QVERIFY2(difference.isEmpty(), qPrintable(difference));
    }

    // Le patron écarté (nœuds refaits à chaque mouvement) ferait builtCount = 2 + 2 k. Le trait suit le
    // décalage, et l'ancienne place est effacée (zone sale du moteur logiciel).
    void transformDoesNotRebuild()
    {
        Fixture f;
        open(f);
        f.layer->setObjects({blueRectangle(), redStroke()});
        QVERIFY(near(at(f, 450, 210), kRed, 60));
        QCOMPARE(f.layer->builtCount(), 2);
        for (int k = 1; k <= 10; ++k) {
            f.layer->setExtraTransform(core::translation(10.0 * k, 0));
            const QImage image = f.window.grabWindow();
            QCOMPARE(f.layer->builtCount(), 2);
            const QColor end = image.pixelColor(496 + 10 * k, 210); // bout droit (x = 500 au repos) décalé
            QVERIFY2(near(end, kRed, 60), qPrintable(QStringLiteral("k = %1 : %2").arg(k).arg(end.name())));
            QVERIFY(near(image.pixelColor(90 + 10 * k, 210), Qt::white, 8)); // à gauche du début décalé
        }
        const QImage moved = f.window.grabWindow();
        QVERIFY(near(moved.pixelColor(450 + 100, 210), kRed, 60)); // contrôle au pixel à x + 100
        QVERIFY(near(moved.pixelColor(150, 210), Qt::white, 8));   // ancien début du trait
        QVERIFY(near(moved.pixelColor(280 + 100, 170), kBlue, 8)); // le rectangle suit aussi
        QVERIFY(near(moved.pixelColor(220, 170), Qt::white, 8));   // ancienne place du rectangle
        QCOMPARE(f.layer->builtCount(), 2);
    }

    void clearEmpties()
    {
        Fixture f;
        open(f);
        QSignalSpy changed(f.layer, &render::LiftLayerItem::objectsChanged);
        f.layer->setObjects({redStroke()});
        QCOMPARE(changed.count(), 1);
        QCOMPARE(f.layer->objectCount(), 1);
        QVERIFY(near(at(f, 300, 210), kRed, 60));
        const int built = f.layer->builtCount();
        f.layer->clear();
        QCOMPARE(f.layer->objectCount(), 0);
        QCOMPARE(changed.count(), 2);
        QVERIFY(near(at(f, 300, 210), Qt::white, 8));
        QCOMPARE(f.layer->builtCount(), built); // vider ne construit rien
    }

    // Translucidité 128 : le pixel de la couche est celui de la page fixe (nœud d'opacité gardé).
    void translucencyKept()
    {
        Fixture f;
        open(f);
        core::PageObject veiled = blueRectangle();
        veiled.common.translucency = 128;
        f.layer->setObjects({veiled});
        const QColor lifted = at(f, 280, 180);
        QVERIFY2(!near(lifted, kBlue, 8) && !near(lifted, Qt::white, 8), qPrintable(lifted.name()));
        f.layer->clear();
        f.page->setPage(pageWith({veiled}));
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        const QColor fixed = at(f, 280, 180);
        QCOMPARE(lifted.name(QColor::HexArgb), fixed.name(QColor::HexArgb));
    }

    // Contrat de setObjects : la transformation repart de l'identité.
    void setObjectsResetsTheTransform()
    {
        Fixture f;
        open(f);
        f.layer->setObjects({redStroke()});
        f.layer->setExtraTransform(core::translation(100, 0));
        QVERIFY(near(at(f, 550, 210), kRed, 60));
        f.layer->setObjects({redStroke()});
        const QImage image = f.window.grabWindow();
        QVERIFY(near(image.pixelColor(150, 210), kRed, 60));
        QVERIFY(near(image.pixelColor(560, 210), Qt::white, 8));
    }

    // La transformation supplémentaire s'applique en coordonnées de page, après celle de l'objet : carré de
    // 20 posé en (100, 100) par sa propre translation, puis agrandi deux fois → (200..240). Appliquée avant
    // (dans le repère de l'objet), il serait en (100..140).
    void extraTransformAppliesInPageCoordinates()
    {
        Fixture f;
        open(f);
        core::Shape s;
        s.kind = core::ShapeKind::Rectangle;
        s.bounds = core::RectF{0, 0, 20, 20};
        s.fill = 0xFF1E3AE5u;
        s.outlineWidth = 0;
        core::PageObject square = s;
        square.common.id = 4;
        square.common.transform = core::translation(100, 100);
        f.layer->setObjects({square});
        QVERIFY(near(at(f, 110, 110), kBlue, 8));
        f.layer->setExtraTransform(core::scaling(2, 2));
        const QImage image = f.window.grabWindow();
        QVERIFY(near(image.pixelColor(220, 220), kBlue, 8));
        QVERIFY(near(image.pixelColor(110, 110), Qt::white, 8));
        QCOMPARE(f.layer->builtCount(), 1);
    }
};

QTEST_MAIN(TstRenderLiftlayer)
#include "tst_render_liftlayer.moc"
