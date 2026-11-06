#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/pagepainter.h>

#include <QImage>
#include <QPainter>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSignalSpy>
#include <QTest>

#include <algorithm>
#include <cstdlib>
#include <vector>

using namespace actcraft;

// Lot 8 (L8-T18, F-255, D8-8) : encre révélatrice. Une encre révélatrice visible de la couche du dessus découpe
// les objets de la couche du dessus placés SOUS elle (avant elle dans la superposition) ; les couches
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

QColor argb(core::Argb c) { return QColor::fromRgba(c); }

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

// Page complète des comparaisons : vert et bleu (intermédiaire), cyan (inférieure), rouge, trait, jaune (dessus).
core::Page fullPage()
{
    return page200({rect(core::Layer::Middle, 20, 20, 120, 160, kGreen), rect(core::Layer::Middle, 60, 40, 30, 120, kBlue),
                    rect(core::Layer::Bottom, 150, 20, 40, 160, kCyan), rect(core::Layer::Top, 20, 20, 170, 160, kRed),
                    revealer(), rect(core::Layer::Top, 100, 40, 30, 120, kYellow)});
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

} // namespace

class TstRenderReveal : public QObject
{
    Q_OBJECT
private slots:
    // Sous le trait, la couche intermédiaire (vert) et la couche inférieure (cyan) apparaissent ; entre les deux,
    // le fond blanc reste opaque : seule la couche du dessus est percée.
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

    // Un objet de la couche intermédiaire placé avant le trait dans la liste n'est pas découpé : la découpe suit
    // la couche, pas l'ordre de la liste.
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
        const QImage image =
            paint(page200({rect(core::Layer::Middle, 20, 20, 120, 160, kGreen), rect(core::Layer::Top, 20, 20, 170, 160, kRed),
                           revealer(), rect(core::Layer::Top, 100, 40, 30, 120, kYellow)}));
        QVERIFY2(near(image.pixelColor(115, 100), argb(kYellow), 8), "le jaune au-dessus du trait reste");
        QVERIFY2(near(image.pixelColor(40, 100), argb(kGreen), 8), "le rouge sous le trait est découpé");
    }

    // Le trait ne se dessine pas : aucun pixel de sa couleur, même sans objet du dessus à découper.
    void noOwnPaint()
    {
        const QImage image = paint(page200({rect(core::Layer::Middle, 20, 20, 120, 160, kGreen), revealer()}));
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

    // Trait déplacé de 50 px vers le bas (transformation de l'objet, comme le déplacement par la sélection) : la
    // découpe suit.
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

    // Même page par le peintre et par la page fixe hors écran : écart médian <= 2 sur 255, et la découpe est bien
    // dans la page fixe (le médian seul ne verrait pas une bande de 40 px absente).
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

        QQuickWindow window;
        window.setColor(Qt::white);
        window.resize(200, 200);
        auto* page = new render::FixedPageItem;
        page->setSize(QSizeF(200, 200));
        page->setParentItem(window.contentItem());
        page->setPage(fullPage());
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QVERIFY(QTest::qWaitFor([&] { return page->isReady(); }, 5000));
        QImage fixed;
        QVERIFY(QTest::qWaitFor(
            [&] {
                fixed = window.grabWindow();
                return !fixed.isNull() && near(fixed.pixelColor(40, 50), argb(kRed), 8);
            },
            5000));
        fixed = fixed.convertToFormat(QImage::Format_ARGB32_Premultiplied).scaled(200, 200);

        QVERIFY2(near(fixed.pixelColor(40, 100), argb(kGreen), 8), "page fixe : découpe sous le trait");
        QVERIFY2(near(fixed.pixelColor(75, 100), argb(kBlue), 8), "page fixe : intermédiaire gardé");
        QVERIFY2(near(fixed.pixelColor(170, 100), argb(kCyan), 8), "page fixe : inférieure révélée");
        QVERIFY2(near(fixed.pixelColor(115, 100), argb(kYellow), 8), "page fixe : dessus après le trait gardé");
        const int median = medianDifference(opaque, fixed);
        QVERIFY2(median <= 2, qPrintable(QStringLiteral("écart médian %1").arg(median)));

        const auto api = window.rendererInterface()->graphicsApi();
        if (api == QSGRendererInterface::Software) {
            QSKIP("Chemin RHI indisponible hors écran ici : comparaison faite au graphe logiciel de la page fixe ; "
                  "le chemin RHI se compare sur le PC.");
        }
    }
};

QTEST_MAIN(TstRenderReveal)
#include "tst_render_reveal.moc"
