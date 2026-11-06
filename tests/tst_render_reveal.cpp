#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/liftlayeritem.h>
#include <actcraft/render/pagepainter.h>

#include <QBuffer>
#include <QImage>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QSGRendererInterface>
#include <QSignalSpy>
#include <QTest>

#include <algorithm>
#include <cstdlib>
#include <vector>

using namespace actcraft;

// Lot 8 (L8-T18, F-255, D8-8) : encre révélatrice. Une encre révélatrice visible de la couche du dessus
// découpe les objets de la couche du dessus placés SOUS elle (avant elle dans la superposition) ; les couches
// intermédiaire et inférieure et le fond restent ; le trait ne se dessine pas. Page 200 x 200, rendu par
// `paintPage` sur une QImage transparente (une découpe qui percerait la cible entière laisserait un pixel
// transparent), puis par la page fixe hors écran (même résultat attendu : le cache est un dessin du peintre).
namespace {

constexpr core::Argb kGreen = 0xFF00C000u;  // couche intermédiaire
constexpr core::Argb kBlue = 0xFF0000FFu;   // couche intermédiaire, au-dessus du vert
constexpr core::Argb kCyan = 0xFF00FFFFu;   // couche inférieure
constexpr core::Argb kRed = 0xFFFF0000u;    // couche du dessus, sous le trait
constexpr core::Argb kYellow = 0xFFFFFF00u; // couche du dessus, après le trait
constexpr core::Argb kInk = 0xFFFF00FFu;    // couleur du trait : ne doit jamais se voir

bool near(QColor a, QColor b, int tolerance)
{
    return a.alpha() == 255 && std::abs(a.red() - b.red()) <= tolerance &&
           std::abs(a.green() - b.green()) <= tolerance && std::abs(a.blue() - b.blue()) <= tolerance;
}

QColor argb(core::Argb c)
{
    return QColor::fromRgba(c);
}

// Rectangle plein, sans contour, en coordonnées de page.
core::PageObject rect(core::Layer layer, double x, double y, double w, double h, core::Argb color)
{
    core::Shape s;
    s.kind = core::ShapeKind::Rectangle;
    s.bounds = core::RectF{x, y, w, h};
    s.fill = color;
    s.outlineWidth = 0;
    core::PageObject o = std::move(s);
    o.common.layer = layer;
    return o;
}

// Encre révélatrice horizontale de 40 px (bande y 80..120 à pression 0,5), de x = 10 à x = 195.
core::Stroke revealerStroke()
{
    core::Stroke s;
    s.kind = core::StrokeKind::Revealer;
    s.baseWidth = 40;
    s.color = kInk;
    for (int i = 0; i <= 37; ++i) {
        s.points.push_back(core::StrokePoint{10.0 + 5.0 * i, 100.0, 0.5f});
    }
    return s;
}

core::PageObject revealer(double dy = 0)
{
    core::PageObject o = revealerStroke();
    o.common.layer = core::Layer::Top;
    o.common.transform.dy = dy;
    return o;
}

core::Page page200(std::vector<core::PageObject> objects)
{
    core::Page p;
    p.width = 200;
    p.height = 200;
    p.background.fill.kind = core::FillKind::Solid; // fond opaque : une découpe qui le percerait se voit
    p.background.fill.color = 0xFFFFFFFFu;
    p.objects = std::move(objects);
    return p;
}

QImage paint(const core::Page& page)
{
    QImage image(200, 200, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    render::paintPage(painter, page, [](const std::string&) { return QImage(); });
    painter.end();
    return image;
}

// Page complète des comparaisons : vert et bleu (intermédiaire), cyan (inférieure), rouge, trait, jaune
// (dessus).
core::Page fullPage()
{
    return page200({rect(core::Layer::Middle, 20, 20, 120, 160, kGreen),
                    rect(core::Layer::Middle, 60, 40, 30, 120, kBlue),
                    rect(core::Layer::Bottom, 150, 20, 40, 160, kCyan),
                    rect(core::Layer::Top, 20, 20, 170, 160, kRed), revealer(),
                    rect(core::Layer::Top, 100, 40, 30, 120, kYellow)});
}

int medianDifference(const QImage& a, const QImage& b)
{
    std::vector<int> d;
    d.reserve(static_cast<std::size_t>(a.width()) * static_cast<std::size_t>(a.height()));
    for (int y = 0; y < a.height(); ++y) {
        for (int x = 0; x < a.width(); ++x) {
            const QColor ca = a.pixelColor(x, y);
            const QColor cb = b.pixelColor(x, y);
            d.push_back(std::max({std::abs(ca.red() - cb.red()), std::abs(ca.green() - cb.green()),
                                  std::abs(ca.blue() - cb.blue())}));
        }
    }
    const auto middle = d.begin() + static_cast<std::ptrdiff_t>(d.size() / 2);
    std::nth_element(d.begin(), middle, d.end());
    return *middle;
}

// Page fixe et couche soulevée dans une fenêtre hors écran de `side` pixels.
struct Scene
{
    QQuickWindow window;
    render::FixedPageItem* page = nullptr;
    render::LiftLayerItem* lift = nullptr;
};

bool openScene(Scene& s, core::Page page, int side = 200)
{
    s.window.setColor(Qt::white);
    s.window.resize(side, side);
    s.page = new render::FixedPageItem;
    s.page->setSize(QSizeF(200, 200));
    s.page->setParentItem(s.window.contentItem());
    s.page->setPage(std::move(page));
    s.lift = new render::LiftLayerItem;
    s.lift->setSize(QSizeF(200, 200));
    s.lift->setParentItem(s.window.contentItem());
    s.lift->setFixedPage(s.page);
    s.window.show();
    return QTest::qWaitForWindowExposed(&s.window) &&
           QTest::qWaitFor([&] { return s.page->isReady(); }, 5000);
}

QImage grab(Scene& s)
{
    return s.window.grabWindow().convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

int inkedPixels(const QImage& image)
{
    int inked = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            inked += near(image.pixelColor(x, y), argb(kInk), 60) ? 1 : 0;
        }
    }
    return inked;
}

core::Page greenUnderRed()
{
    return page200({rect(core::Layer::Middle, 20, 20, 120, 160, kGreen),
                    rect(core::Layer::Top, 20, 20, 170, 160, kRed)});
}

} // namespace

class TstRenderReveal : public QObject
{
    Q_OBJECT
private slots:
    // Sous le trait, la couche intermédiaire (vert) et la couche inférieure (cyan) apparaissent ; entre les
    // deux, le fond blanc reste opaque : seule la couche du dessus est percée.
    void revealsMiddleLayer()
    {
        const QImage image = paint(page200({rect(core::Layer::Middle, 20, 20, 120, 160, kGreen),
                                            rect(core::Layer::Bottom, 150, 20, 40, 160, kCyan),
                                            rect(core::Layer::Top, 20, 20, 170, 160, kRed), revealer()}));
        QVERIFY2(near(image.pixelColor(40, 100), argb(kGreen), 8), "sous le trait : la couche intermédiaire");
        QVERIFY2(near(image.pixelColor(170, 100), argb(kCyan), 8), "sous le trait : la couche inférieure");
        QVERIFY2(near(image.pixelColor(145, 100), Qt::white, 8), "sous le trait : le fond, opaque");
        QVERIFY2(near(image.pixelColor(40, 50), argb(kRed), 8), "hors du trait : le dessus reste");
        QVERIFY2(near(image.pixelColor(40, 150), argb(kRed), 8), "hors du trait : le dessus reste");
    }

    // Un objet de la couche intermédiaire placé avant le trait dans la liste n'est pas découpé : la découpe
    // suit la couche, pas l'ordre de la liste.
    void onlyTopObjectsBelowIt()
    {
        const QImage image = paint(page200({rect(core::Layer::Middle, 20, 20, 120, 160, kGreen),
                                            rect(core::Layer::Middle, 60, 40, 30, 120, kBlue),
                                            rect(core::Layer::Top, 20, 20, 170, 160, kRed), revealer()}));
        QVERIFY2(near(image.pixelColor(75, 100), argb(kBlue), 8), "le bleu intermédiaire reste bleu");
        QVERIFY2(near(image.pixelColor(40, 100), argb(kGreen), 8), "le vert intermédiaire apparaît");
        QVERIFY2(near(image.pixelColor(75, 50), argb(kRed), 8), "hors du trait : le dessus reste");
    }

    // Un objet du dessus placé après le trait reste entier.
    void objectAboveItStays()
    {
        const QImage image = paint(page200({rect(core::Layer::Middle, 20, 20, 120, 160, kGreen),
                                            rect(core::Layer::Top, 20, 20, 170, 160, kRed), revealer(),
                                            rect(core::Layer::Top, 100, 40, 30, 120, kYellow)}));
        QVERIFY2(near(image.pixelColor(115, 100), argb(kYellow), 8), "le jaune au-dessus du trait reste");
        QVERIFY2(near(image.pixelColor(40, 100), argb(kGreen), 8), "le rouge sous le trait est découpé");
    }

    // Le trait ne se dessine pas : aucun pixel de sa couleur, même sans objet du dessus à découper.
    void noOwnPaint()
    {
        const QImage image =
            paint(page200({rect(core::Layer::Middle, 20, 20, 120, 160, kGreen), revealer()}));
        int inked = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                inked += near(image.pixelColor(x, y), argb(kInk), 60) ? 1 : 0;
            }
        }
        QCOMPARE(inked, 0);
        QVERIFY2(near(image.pixelColor(40, 100), argb(kGreen), 8), "le vert sous le trait");
        QVERIFY2(near(image.pixelColor(15, 100), Qt::white, 8), "le fond sous le trait");
    }

    // Trait déplacé de 50 px vers le bas (transformation de l'objet, comme le déplacement par la sélection) :
    // la découpe suit.
    void movingMovesTheHole()
    {
        const QImage image = paint(page200({rect(core::Layer::Middle, 20, 20, 120, 160, kGreen),
                                            rect(core::Layer::Top, 20, 20, 170, 160, kRed), revealer(50)}));
        QVERIFY2(near(image.pixelColor(40, 150), argb(kGreen), 8), "la découpe a suivi le trait");
        QVERIFY2(near(image.pixelColor(40, 100), argb(kRed), 8), "l'ancienne place est refermée");
    }

    // Lot 10 (A2) : une encre révélatrice n'attend jamais au-dessus du cache ; versée tout de suite.
    void revealStrokeIsNotDeferred()
    {
        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(640, 480);
        auto* page = new render::FixedPageItem;
        page->setSize(QSizeF(640, 480));
        page->setParentItem(window.contentItem());
        page->setPage(core::Page{});
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(QTest::qWaitFor([&] { return page->isReady(); }, 5000));
        page->setDeferredFolding(true);
        QSignalSpy content(page, &render::FixedPageItem::contentChanged);
        page->appendStroke(revealerStroke());
        QCOMPARE(page->pendingCount(), 0);
        QCOMPARE(content.count(), 1);
        QCOMPARE(page->cachedObjectCount(), std::size_t{1});
    }

    // Encre soulevée par la sélection (glisser, lot 3) : jamais un trait coloré ; le trou la suit pendant le
    // glisser, au-dessus du cache (la page fixe, sans l'objet soulevé, garde le dessus entier). La matrice
    // change après le premier rendu, comme pendant un vrai glisser.
    void liftedRevealerMovesTheHole()
    {
        Scene s;
        QVERIFY(openScene(s, greenUnderRed()));
        s.lift->setObjects({revealer()});
        QVERIFY(QTest::qWaitFor([&] { return near(grab(s).pixelColor(40, 100), argb(kGreen), 8); }, 5000));
        core::Transform down;
        down.dy = 50;
        s.lift->setExtraTransform(down);
        QImage image;
        QVERIFY(QTest::qWaitFor(
            [&] {
                image = grab(s);
                return near(image.pixelColor(40, 150), argb(kGreen), 8);
            },
            5000));
        QCOMPARE(inkedPixels(image), 0);
        QVERIFY2(near(image.pixelColor(170, 150), Qt::white, 8), "sous l'encre, hors du vert : le fond");
        QVERIFY2(near(image.pixelColor(40, 100), argb(kRed), 8), "l'ancienne place : le dessus entier");
    }

    // Fluidité (revue de L8-T18) : pendant le glisser d'une encre, seule sa fenêtre suit ; aucun nœud soulevé
    // n'est reconstruit à chaque changement de matrice.
    void liftedRevealerRebuildsNothingWhileDragging()
    {
        Scene s;
        QVERIFY(openScene(s, greenUnderRed()));
        s.lift->setObjects({revealer(), rect(core::Layer::Top, 150, 150, 20, 20, kYellow)});
        QVERIFY(QTest::qWaitFor([&] { return near(grab(s).pixelColor(40, 100), argb(kGreen), 8); }, 5000));
        const int built = s.lift->builtCount();
        QVERIFY(built >= 2);
        for (int i = 1; i <= 5; ++i) {
            core::Transform step;
            step.dy = 10.0 * i;
            s.lift->setExtraTransform(step);
            (void)grab(s);
        }
        QCOMPARE(s.lift->builtCount(), built);
        const QImage image = grab(s);
        QVERIFY2(near(image.pixelColor(40, 150), argb(kGreen), 8), "la fenêtre a suivi");
        QVERIFY2(near(image.pixelColor(40, 100), argb(kRed), 8), "l'ancienne place : le dessus entier");
    }

    // Revue (point 4) : la part découpée rastérisée suit l'échelle réelle (densité × zoom) que la vue pose
    // par setRasterScale, ici 2 : un filet du dessus de 0,5 unité de page couvre exactement un pixel de la
    // fenêtre, plein rouge (flou à l'échelle 1). L8-C6 : l'échelle de l'élément n'est plus relue (le test la
    // posait par setScale seul) ; la vue pose l'échelle une fois le zoom fini, comme pour le fond et la
    // grille.
    void revealedLayerFollowsScreenScale()
    {
        Scene s;
        QVERIFY(openScene(s,
                          page200({rect(core::Layer::Middle, 20, 20, 120, 160, kGreen),
                                   rect(core::Layer::Top, 50, 20, 0.5, 160, kRed), revealer()}),
                          400));
        s.page->setTransformOrigin(QQuickItem::TopLeft);
        s.page->setScale(2);
        s.page->setRasterScale(2);
        s.page->setForceRasterized(true);
        QVERIFY(QTest::qWaitFor([&] { return near(grab(s).pixelColor(80, 200), argb(kGreen), 8); }, 5000));
        const QImage image = grab(s);
        QVERIFY2(near(image.pixelColor(100, 60), argb(kRed), 40),
                 qPrintable(QStringLiteral("filet : %1").arg(image.pixelColor(100, 60).name())));
        const int rasterized = s.page->rasterizationCount();
        (void)grab(s);
        s.page->update();
        (void)grab(s);
        QCOMPARE(s.page->rasterizationCount(), rasterized); // refaite au changement d'échelle seulement
    }

    // L8-C6 (revue de L8-T18) : un zoom continu ne rastérise rien. La part découpée suit l'échelle que la vue
    // pose une fois le zoom fini (setRasterScale, ui/src/pageview.cpp:1349-1353), jamais l'échelle de
    // l'élément relue à chaque image ou à chaque redessin : sinon chaque image d'un zoom rastérise toute la
    // page sur le fil de rendu. Rouge sous « relire screenScale » : le compteur monte à chaque pas.
    void revealedLayerIsNotRasterizedWhileScaleChanges()
    {
        Scene s;
        QVERIFY(openScene(s,
                          page200({rect(core::Layer::Middle, 20, 20, 120, 160, kGreen),
                                   rect(core::Layer::Top, 50, 20, 0.5, 160, kRed), revealer()}),
                          400));
        s.page->setTransformOrigin(QQuickItem::TopLeft);
        s.page->setForceRasterized(true);
        QVERIFY(QTest::qWaitFor([&] { return near(grab(s).pixelColor(40, 100), argb(kGreen), 8); }, 5000));
        const int rasterized = s.page->rasterizationCount();
        QVERIFY(rasterized > 0); // la part découpée est bien un nœud rastérisé
        for (const double scale : {1.2, 1.4, 1.6, 1.8, 2.0}) {
            s.page->setScale(scale);
            s.page->update(); // tout redessin pendant le zoom (trait, curseur, animation)
            (void)grab(s);
            QCOMPARE(s.page->rasterizationCount(), rasterized);
        }
        // Le zoom est posé : la vue pousse l'échelle, la part découpée est refaite, une fois, à cette
        // échelle.
        s.page->setRasterScale(2.0);
        const QImage image = grab(s);
        QVERIFY(s.page->rasterizationCount() > rasterized);
        QVERIFY2(near(image.pixelColor(100, 60), argb(kRed), 40),
                 qPrintable(QStringLiteral("filet : %1").arg(image.pixelColor(100, 60).name())));
        const int posed = s.page->rasterizationCount();
        (void)grab(s);
        s.page->update();
        (void)grab(s);
        QCOMPARE(s.page->rasterizationCount(), posed); // à échelle égale, rien
    }

    // Revue (point 6) : à l'export PDF (périphérique à 96 ppp), la couche découpée est rastérisée à 300 ppp
    // au moins : 200 unités de page, 625 pixels.
    void printedRevealLayerIsAtLeast300Dpi()
    {
        QBuffer buffer;
        QVERIFY(buffer.open(QIODevice::WriteOnly));
        {
            QPdfWriter writer(&buffer);
            writer.setResolution(96);
            writer.setPageSize(QPageSize(QSizeF(150, 150), QPageSize::Point));
            writer.setPageMargins(QMarginsF(0, 0, 0, 0));
            QPainter painter(&writer);
            render::paintPage(painter, fullPage(), [](const std::string&) { return QImage(); });
        }
        int widest = 0;
        auto it = QRegularExpression(QStringLiteral("/Width (\\d+)"))
                      .globalMatch(QString::fromLatin1(buffer.data()));
        while (it.hasNext()) {
            widest = std::max(widest, it.next().captured(1).toInt());
        }
        QVERIFY2(widest >= 600, qPrintable(QStringLiteral("image la plus large : %1 px").arg(widest)));
    }

    // Même page par le peintre et par la page fixe hors écran : écart médian <= 2 sur 255, et la découpe est
    // bien dans la page fixe (le médian seul ne verrait pas une bande de 40 px absente).
    void softwareAndRhiAgree()
    {
        const core::Page source = fullPage();
        QImage software = paint(source);
        QImage opaque(software.size(), QImage::Format_ARGB32_Premultiplied);
        opaque.fill(Qt::white);
        {
            QPainter p(&opaque);
            p.drawImage(0, 0, software);
        }
        Scene s;
        QVERIFY(openScene(s, fullPage()));
        // Graphe de la fenêtre (vectoriel en logiciel), puis chemin rastérisé forcé : la texture de la part
        // découpée, celle du RHI ; puis une autre échelle (remise à l'échelle des nœuds rastérisés).
        for (const int pass : {0, 1, 2}) {
            if (pass == 1) {
                s.page->setForceRasterized(true);
            } else if (pass == 2) {
                s.page->setRasterScale(3);
            }
            QImage fixed;
            QVERIFY(QTest::qWaitFor(
                [&] {
                    fixed = grab(s);
                    return near(fixed.pixelColor(40, 50), argb(kRed), 8) &&
                           near(fixed.pixelColor(40, 100), argb(kGreen), 8);
                },
                5000));
            if (pass > 0) {
                QVERIFY(s.page->rasterizationCount() > 0);
            }
            QVERIFY2(near(fixed.pixelColor(75, 100), argb(kBlue), 8), "page fixe : intermédiaire gardé");
            QVERIFY2(near(fixed.pixelColor(170, 100), argb(kCyan), 8), "page fixe : inférieure révélée");
            QVERIFY2(near(fixed.pixelColor(115, 100), argb(kYellow), 8),
                     "page fixe : dessus après le trait gardé");
            QCOMPARE(inkedPixels(fixed), 0);
            const int median = medianDifference(opaque, fixed);
            QVERIFY2(median <= 2,
                     qPrintable(QStringLiteral("passe %1 : écart médian %2").arg(pass).arg(median)));
        }
    }
};

QTEST_MAIN(TstRenderReveal)
#include "tst_render_reveal.moc"
