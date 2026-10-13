#include <actcraft/core/generator.h>

#include <QTest>

using namespace actcraft::core;

namespace {
// Valeur relevée au premier passage sous Windows (étape 7), vérifiée sous macOS par la CI ; égale à celle
// d'une implémentation indépendante en Python (mt19937 écrit à la main, même générateur, même FNV-1a).
// Ne jamais la « corriger » : si elle change, c'est le générateur qui a changé.
constexpr std::uint64_t kExpectedFingerprint = 0xEC92781F3ED73B1Eull;

bool nearBlack(Argb c)
{
    const auto r = (c >> 16) & 0xFFu;
    const auto g = (c >> 8) & 0xFFu;
    const auto b = c & 0xFFu;
    return r < 0x40u && g < 0x40u && b < 0x40u;
}
} // namespace

class TstCoreGenerator : public QObject
{
    Q_OBJECT

private slots:
    // Suite de std::mt19937 pour la graine 5489, fixée par la norme ; tirages déduits à la main :
    // below(n) = x mod n après rejet des x >= 2^32 - (2^32 mod n). Les distributions de la bibliothèque
    // standard laissent leur algorithme à l'implémentation : elles ne garantissent pas cette suite.
    void portableRngFollowsStandardSequence()
    {
        PortableRng rng(5489u);
        QCOMPARE(rng.next(), 3499211612u);
        QCOMPARE(rng.next(), 581869302u);
        QCOMPARE(rng.below(10u), 4u);                 // 3890346734 mod 10
        QCOMPARE(rng.between(16, 160), 156);          // 16 + 3586334585 mod 145
        QCOMPARE(rng.below(0x80000001u), 545404204u); // accepté : < 2^31 + 1
        QCOMPARE(rng.below(0x80000001u), 949333985u); // 4161255391 et 3922919429 rejetés
    }

    void countsPerKind()
    {
        const Page page = generatePage();
        QCOMPARE(page.objects.size(), std::size_t{300});
        int shapes = 0;
        int images = 0;
        int strokes = 0;
        for (const PageObject& o : page.objects) {
            switch (kindOf(o)) {
            case ObjectKind::Shape:
                ++shapes;
                break;
            case ObjectKind::Image:
                ++images;
                break;
            case ObjectKind::Stroke:
                ++strokes;
                break;
            }
        }
        QCOMPARE(shapes, kShapeCount);
        QCOMPARE(images, kImageCount);
        QCOMPARE(strokes, kStrokeCount);
    }

    void deterministic()
    {
        QCOMPARE(fingerprint(generatePage()), fingerprint(generatePage()));
        QVERIFY(fingerprint(generatePage(kDefaultSeed)) != fingerprint(generatePage(kDefaultSeed + 1)));
    }

    void fingerprintIsPinned()
    {
        const std::uint64_t actual = fingerprint(generatePage());
        if (actual != kExpectedFingerprint) {
            qWarning("fingerprint = 0x%016llx", static_cast<unsigned long long>(actual));
        }
        QCOMPARE(actual, kExpectedFingerprint);
    }

    void witnessesOnTop()
    {
        const Page page = generatePage();
        const Witnesses& w = witnesses();
        const std::size_t n = page.objects.size();
        const auto& shape = std::get<Shape>(page.objects[n - 3]);
        QCOMPARE(shape.bounds.x, w.shapeRect.x);
        QCOMPARE(shape.fill, w.shapeColor);
        QCOMPARE(shape.opacity, 1.0);
        const auto& image = std::get<ImageObject>(page.objects[n - 2]);
        QCOMPARE(image.pattern, ImagePattern::SolidCenter);
        QCOMPARE(image.colorA, w.imageCenterColor);
        QCOMPARE(image.bounds.x, w.imageRect.x);
        const auto& stroke = std::get<Stroke>(page.objects[n - 1]);
        QCOMPARE(stroke.color, w.strokeColor);
        QVERIFY(w.strokeRect.contains(w.strokeProbe));
        QVERIFY(stroke.baseWidth >= 12.0);
    }

    void semiTransparentShare()
    {
        const Page page = generatePage();
        int translucent = 0;
        for (const PageObject& o : page.objects) {
            if (const auto* s = std::get_if<Shape>(&o); s && s->opacity < 1.0) {
                ++translucent;
            }
        }
        QCOMPARE(translucent, 30); // 20 % de 150
    }

    void generatedStrokesAvoidWitnesses()
    {
        const Page page = generatePage();
        for (std::size_t i = 0; i + 3 < page.objects.size(); ++i) {
            if (const auto* s = std::get_if<Stroke>(&page.objects[i])) {
                for (const StrokePoint& p : s->points) {
                    QVERIFY2(!hitsWitness({p.x, p.y}, 8.0), qPrintable(QString::number(i)));
                }
            }
        }
    }

    void rangesRespected()
    {
        const Page page = generatePage();
        for (std::size_t i = 0; i + 3 < page.objects.size(); ++i) {
            const PageObject& o = page.objects[i];
            if (const auto* s = std::get_if<Stroke>(&o)) {
                QVERIFY(s->points.size() >= 50 && s->points.size() <= 300);
                QVERIFY(!nearBlack(s->color));
                for (const StrokePoint& p : s->points) {
                    QVERIFY(p.x >= 0 && p.x <= 1024 && p.y >= 0 && p.y <= 768);
                    QVERIFY(p.pressure >= 0.25f && p.pressure < 1.0f);
                }
            } else if (const auto* im = std::get_if<ImageObject>(&o)) {
                QVERIFY(im->sourceWidth >= 256 && im->sourceWidth <= 1024);
                QVERIFY(im->sourceHeight >= 256 && im->sourceHeight <= 1024);
                QVERIFY(im->pattern != ImagePattern::SolidCenter);
                QVERIFY(im->bounds.x >= 0 && im->bounds.x + im->bounds.width <= 1024);
                QVERIFY(im->bounds.y >= 0 && im->bounds.y + im->bounds.height <= 768);
            } else if (const auto* sh = std::get_if<Shape>(&o)) {
                QVERIFY(!nearBlack(sh->fill));
                QVERIFY(sh->bounds.x >= 0 && sh->bounds.x + sh->bounds.width <= 1024);
                QVERIFY(sh->bounds.y >= 0 && sh->bounds.y + sh->bounds.height <= 768);
                if (sh->kind == ShapeKind::Polygon) {
                    QVERIFY(sh->polygon.size() >= 3 && sh->polygon.size() <= 8);
                }
            }
        }
    }
};

QTEST_GUILESS_MAIN(TstCoreGenerator)
#include "tst_core_generator.moc"
