#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/liftlayeritem.h>

#include <QBuffer>
#include <QByteArray>
#include <QImage>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>

#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

using namespace actcraft;

namespace {

const QColor kRed(0xE5, 0x39, 0x35);
const QColor kBlue(0x1E, 0x3A, 0xE5);
const QColor kOrange(0xFF, 0x80, 0x00); // ressource PNG
const QColor kGreen(0x2E, 0x7D, 0x32);  // image synthétique de la page fixe
const QColor kAmber(0xFF, 0xB3, 0x00);  // image synthétique soulevée

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

// PNG uni de 32 × 32 : octets d'une ressource de document.
QByteArray solidPng(const QColor& color)
{
    QImage image(32, 32, QImage::Format_ARGB32);
    image.fill(color);
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    if (!image.save(&buffer, "PNG")) {
        png.clear(); // ressource illisible : les tests le verraient au pixel (repère du peintre)
    }
    return png;
}

// Image à ressource (clé du document) en (100, 100), 120 × 90 : centre en (160, 145).
core::PageObject resourceImage(const std::string& key)
{
    core::ImageObject im;
    im.bounds = core::RectF{100, 100, 120, 90};
    im.sourceWidth = 32;
    im.sourceHeight = 32;
    im.resource = key;
    core::PageObject object = im;
    object.common.id = 7;
    return object;
}

// Image synthétique (ressource vide) unie de la couleur donnée.
core::PageObject syntheticImage(const core::RectF& bounds, core::Argb color, core::ObjectId id)
{
    core::ImageObject im;
    im.bounds = bounds;
    im.sourceWidth = 64;
    im.sourceHeight = 64;
    im.pattern = core::ImagePattern::SolidCenter;
    im.colorA = color;
    im.colorB = color;
    core::PageObject object = im;
    object.common.id = id;
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

    // L3-C1 : une image à ressource soulevée est dessinée par la couche avec le magasin de la page fixe, au
    // pixel près comme la page fixe la dessinait, dès la première image après setObjects et pendant tout le
    // geste. Sans magasin (images == nullptr, défaut relevé par L3-T8) : repère d'image illisible du peintre.
    // La page fixe lit ensuite un document SANS les octets de la ressource (clé absente : couverte, rien
    // n'est redécodé) : une couche qui décoderait l'image une seconde fois, même sur place, n'aurait rien à
    // décoder.
    void liftedResourceImageIsDrawn()
    {
        core::Document decoded;
        const std::string key = decoded.addResource(solidPng(kOrange), "png");
        decoded.page(0).objects.push_back(resourceImage(key));
        core::Document bare(std::vector<core::Page>{decoded.page(0)}); // même page, aucune ressource
        QVERIFY(bare.resource(key) == nullptr);
        Fixture f; // après les documents : la page fixe meurt avant eux
        open(f);
        f.page->setSource(&decoded, 0);
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        const int decodes = f.page->decodeCount();
        f.page->setSource(&bare, 0);
        QVERIFY(f.page->isReady());
        const QImage before = f.window.grabWindow();
        QVERIFY2(near(before.pixelColor(160, 145), kOrange, 8),
                 qPrintable(before.pixelColor(160, 145).name()));
        QCOMPARE(f.page->decodeCount(), decodes); // la page fixe dessine l'image de son magasin
        // Soulever comme DocumentController : l'objet quitte la page, le cache l'apprend par remove.
        core::Page& shown = bare.page(0);
        const core::PageObject lifted = shown.objects[0];
        shown.objects.erase(shown.objects.begin());
        f.page->remove(lifted.common.id, core::RectF{100, 100, 120, 90});
        QVERIFY(near(at(f, 160, 145), Qt::white, 8));
        QCOMPARE(f.page->decodeCount(), decodes); // une ressource ne quitte pas le magasin au soulèvement
        QVERIFY(f.page->isReady());
        const int rebuilds = f.page->rebuildCount();
        f.layer->setObjects({lifted});
        const QImage during = f.window.grabWindow();
        QVERIFY2(near(during.pixelColor(160, 145), kOrange, 8),
                 qPrintable(during.pixelColor(160, 145).name()));
        const QString difference = firstDifference(before, during);
        QVERIFY2(difference.isEmpty(), qPrintable(difference));
        QCOMPARE(f.page->rebuildCount(), rebuilds); // la couche ne reconstruit pas la page fixe
        QCOMPARE(f.page->decodeCount(), decodes);
        QCOMPARE(f.layer->builtCount(), 1);
        // Pendant le glissement : l'image suit la transformation, sans nœud refait.
        f.layer->setExtraTransform(core::translation(200, 0));
        const QImage moved = f.window.grabWindow();
        QVERIFY2(near(moved.pixelColor(360, 145), kOrange, 8), qPrintable(moved.pixelColor(360, 145).name()));
        QVERIFY(near(moved.pixelColor(160, 145), Qt::white, 8));
        QCOMPARE(f.layer->builtCount(), 1);
        QCOMPARE(f.page->decodeCount(), decodes);
    }

    // L3-C1 : le magasin de la page fixe range les images synthétiques par INDICE de page ; la couche a ses
    // propres indices. Le patron de la page fixe repris tel quel (magasin passé pour tout objet) poserait ici
    // l'image de l'objet 0 de la page (verte) à la place de l'image soulevée (ambre).
    void liftedSyntheticImageIsNotLookedUpByIndex()
    {
        Fixture f;
        open(f);
        f.page->setPage(pageWith({syntheticImage(core::RectF{20, 20, 100, 100}, 0xFF2E7D32u, 8)}));
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        QVERIFY(near(at(f, 70, 70), kGreen, 8));
        f.layer->setObjects({syntheticImage(core::RectF{300, 200, 120, 120}, 0xFFFFB300u, 9)});
        const QColor center = at(f, 360, 260);
        QVERIFY2(near(center, kAmber, 8), qPrintable(center.name()));
    }

    // L3-C1 : au dépôt, l'image revient dans la page fixe à l'image suivante, sans passer par une page qui ne
    // l'a pas : le magasin la couvre encore (clé de ressource), insert ne relance aucun décodage et la page
    // reste prête. Le patron craint par L3-T8 (insert qui redécode toute image ajoutée) garderait l'ancienne
    // scène, sans l'image, jusqu'au prêt : pixel blanc à l'image qui suit le dépôt, couche déjà vidée.
    void droppedImageDoesNotBlink()
    {
        core::Document d;
        const std::string key = d.addResource(solidPng(kOrange), "png");
        d.page(0).objects.push_back(resourceImage(key));
        d.page(0).objects.push_back(blueRectangle()); // dépôt hors fin de page, comme un vrai soulèvement
        Fixture f;
        open(f);
        f.page->setSource(&d, 0);
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        const QImage before = f.window.grabWindow();
        QVERIFY2(near(before.pixelColor(160, 145), kOrange, 8),
                 qPrintable(before.pixelColor(160, 145).name()));
        const int decodes = f.page->decodeCount();
        core::Page& shown = d.page(0);
        const core::PageObject lifted = shown.objects[0];
        shown.objects.erase(shown.objects.begin());
        f.page->remove(lifted.common.id, core::RectF{100, 100, 120, 90});
        f.layer->setObjects({lifted});
        const QImage gesture = f.window.grabWindow(); // geste en cours : page fixe sans l'image
        QVERIFY(!gesture.isNull());
        QCOMPARE(f.page->decodeCount(), decodes);
        // Déposer comme DocumentController : l'objet revient à son rang, la couche se vide, le cache
        // l'apprend par insert, dans le même tour.
        shown.objects.insert(shown.objects.begin(), lifted);
        f.layer->clear();
        f.page->insert(lifted.common.id);
        QVERIFY(f.page->isReady());
        const QImage after = f.window.grabWindow();
        QVERIFY2(near(after.pixelColor(160, 145), kOrange, 8), qPrintable(after.pixelColor(160, 145).name()));
        const QString difference = firstDifference(before, after);
        QVERIFY2(difference.isEmpty(), qPrintable(difference));
        QCOMPARE(f.page->decodeCount(), decodes);
    }

    // L3-C13 (banc, scénario « soulever » : 2 décodages pour 7 objets soulevés, aucune image parmi eux) : le
    // magasin rangeait les images synthétiques par INDICE de page. Soulever un objet placé avant une image
    // décalait son indice : covers() faux, tout était redécodé au soulèvement puis au dépôt, et la page fixe
    // gardait l'ancienne scène jusqu'au prêt (objet en double pendant le geste). Rangées par synthesisKey,
    // comme les ressources par clé (droppedImageDoesNotBlink) : ni décodage, ni ancienne scène.
    void liftBeforeSyntheticImageDoesNotDecode()
    {
        core::Document d;
        d.page(0).objects.push_back(blueRectangle()); // avant l'image : le soulever décale son indice
        d.page(0).objects.push_back(syntheticImage(core::RectF{420, 300, 100, 100}, 0xFF2E7D32u, 8));
        Fixture f;
        open(f);
        f.page->setSource(&d, 0);
        QVERIFY(QTest::qWaitFor([&] { return f.page->isReady(); }, 5000));
        const QImage before = f.window.grabWindow();
        QVERIFY2(near(before.pixelColor(280, 210), kBlue, 8), qPrintable(before.pixelColor(280, 210).name()));
        QVERIFY2(near(before.pixelColor(470, 350), kGreen, 8),
                 qPrintable(before.pixelColor(470, 350).name()));
        const int decodes = f.page->decodeCount();
        core::Page& shown = d.page(0);
        const core::PageObject lifted = shown.objects[0];
        shown.objects.erase(shown.objects.begin());
        f.page->remove(lifted.common.id, core::RectF{200, 150, 160, 120});
        f.layer->setObjects({lifted});
        core::Transform moved;
        moved.dy = 200; // la copie glisse sous l'image, hors de son ancienne place
        f.layer->setExtraTransform(moved);
        QCOMPARE(f.page->decodeCount(), decodes);
        QVERIFY(f.page->isReady());
        const QImage gesture = f.window.grabWindow();
        // Aucun objet en double : l'ancienne place est vide, la copie est sous la translation, l'image reste.
        QVERIFY2(near(gesture.pixelColor(280, 210), Qt::white, 8),
                 qPrintable(gesture.pixelColor(280, 210).name()));
        QVERIFY2(near(gesture.pixelColor(280, 410), kBlue, 8),
                 qPrintable(gesture.pixelColor(280, 410).name()));
        QVERIFY2(near(gesture.pixelColor(470, 350), kGreen, 8),
                 qPrintable(gesture.pixelColor(470, 350).name()));
        // Dépôt comme DocumentController : l'objet revient à son rang, la couche se vide, insert dans le même
        // tour.
        shown.objects.insert(shown.objects.begin(), lifted);
        f.layer->clear();
        f.page->insert(lifted.common.id);
        QCOMPARE(f.page->decodeCount(), decodes);
        QVERIFY(f.page->isReady());
        const QImage after = f.window.grabWindow();
        const QString difference = firstDifference(before, after);
        QVERIFY2(difference.isEmpty(), qPrintable(difference));
    }
};

QTEST_MAIN(TstRenderLiftlayer)
#include "tst_render_liftlayer.moc"
