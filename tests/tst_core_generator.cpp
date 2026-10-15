#include <actcraft/core/generator.h>

#include <QTest>

#include <algorithm>
#include <array>
#include <limits>
#include <utility>

using namespace actcraft::core;

namespace {
// Valeur relevée au premier passage sous Windows après l'ajout des propriétés communes du lot 2 (DA2 :
// empreinte recalculée), vérifiée sous macOS par la CI et recoupée hors dépôt par la réimplémentation Python
// du générateur du lot 1, étendue aux champs du lot 2. Le contenu des 300 objets n'a pas changé ; seuls des
// champs à valeur par défaut et les identifiants 1 à 300 entrent dans l'empreinte. Valeur du lot 1 :
// 0xEC92781F3ED73B1E. Ne jamais la « corriger » : si elle change, c'est le générateur qui a changé.
constexpr std::uint64_t kExpectedFingerprint = 0x3551EB2438FDFDC1ull;

bool nearBlack(Argb c)
{
    const auto r = (c >> 16) & 0xFFu;
    const auto g = (c >> 8) & 0xFFu;
    const auto b = c & 0xFFu;
    return r < 0x40u && g < 0x40u && b < 0x40u;
}

// Vrai si le segment [a, b] coupe `r` agrandi de `margin` (méthode des tranches, bornes incluses).
bool segmentHitsRect(PointF a, PointF b, const RectF& r, double margin)
{
    const std::array<double, 2> from{a.x, a.y};
    const std::array<double, 2> delta{b.x - a.x, b.y - a.y};
    const std::array<double, 2> lo{r.x - margin, r.y - margin};
    const std::array<double, 2> hi{r.x + r.width + margin, r.y + r.height + margin};
    double t0 = 0;
    double t1 = 1;
    for (std::size_t i = 0; i < 2; ++i) {
        if (delta[i] == 0) {
            if (from[i] < lo[i] || from[i] > hi[i]) {
                return false;
            }
            continue;
        }
        double ta = (lo[i] - from[i]) / delta[i];
        double tb = (hi[i] - from[i]) / delta[i];
        if (ta > tb) {
            std::swap(ta, tb);
        }
        t0 = std::max(t0, ta);
        t1 = std::min(t1, tb);
        if (t0 > t1) {
            return false;
        }
    }
    return true;
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

    // Écarts extrêmes : hi − lo + 1 vaut 2^32 pour la plage entière, plus que int32 et uint32 n'en tiennent.
    void portableRngBetweenHandlesWideRanges()
    {
        constexpr std::int32_t kMin = std::numeric_limits<std::int32_t>::min();
        constexpr std::int32_t kMax = std::numeric_limits<std::int32_t>::max();
        PortableRng rng(5489u);
        QCOMPARE(rng.between(kMin, kMax), 1351727964);              // −2^31 + 3499211612
        QCOMPARE(rng.between(7, 7), 7);                             // 581869302 consommé
        QCOMPARE(rng.between(-2000000000, 2000000000), 1890346734); // −2·10^9 + 3890346734
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
            default: // types du lot 2 : absents de la page du banc (et -Wswitch de Clang exige ce cas)
                QFAIL("type d'objet inattendu dans la page du banc");
            }
        }
        QCOMPARE(shapes, kShapeCount);
        QCOMPARE(images, kImageCount);
        QCOMPARE(strokes, kStrokeCount);
    }

    void objectsHaveIdsOneToThreeHundredOnTheMiddleLayer()
    {
        const Page page = generatePage();
        QCOMPARE(page.objects.size(), std::size_t{300});
        for (std::size_t i = 0; i < page.objects.size(); ++i) {
            QCOMPARE(page.objects[i].common.id, static_cast<ObjectId>(i + 1));
            QCOMPARE(page.objects[i].common.layer, Layer::Middle);
            QVERIFY(page.objects[i].common.transform.isIdentity());
        }
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

    // Le générateur garde chaque sommet à plus de 16 px des témoins ; un pas fait au plus 8 px par axe, donc
    // tout point d'un segment reste à plus de 16 − 8 / 2 = 12 px (par axe) du témoin le plus proche.
    void generatedStrokesAvoidWitnesses()
    {
        const Witnesses& w = witnesses();
        const std::array<RectF, 3> rects{w.shapeRect, w.imageRect, w.strokeRect};
        // Contrôle de l'aide : cette diagonale passe près du coin sans l'atteindre à 8 px, l'atteint à 12 px.
        QVERIFY(!segmentHitsRect({0, 10}, {10, 0}, w.shapeRect, 8.0));
        QVERIFY(segmentHitsRect({0, 10}, {10, 0}, w.shapeRect, 12.0));
        const Page page = generatePage();
        for (std::size_t i = 0; i + 3 < page.objects.size(); ++i) {
            if (const auto* s = std::get_if<Stroke>(&page.objects[i])) {
                for (std::size_t k = 0; k < s->points.size(); ++k) {
                    const PointF p{s->points[k].x, s->points[k].y};
                    QVERIFY2(!hitsWitness(p, 16.0), qPrintable(QString::number(i)));
                    if (k == 0) {
                        continue;
                    }
                    const PointF q{s->points[k - 1].x, s->points[k - 1].y};
                    for (const RectF& r : rects) {
                        QVERIFY2(!segmentHitsRect(q, p, r, 12.0), qPrintable(QString::number(i)));
                    }
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
                    // À 8 px au moins du bord de la page.
                    QVERIFY(p.x >= 8 && p.x <= 1024 - 8 && p.y >= 8 && p.y <= 768 - 8);
                    QVERIFY(p.pressure >= 0.25f && p.pressure < 1.0f);
                }
            } else if (const auto* im = std::get_if<ImageObject>(&o)) {
                QVERIFY(im->sourceWidth >= 256 && im->sourceWidth <= 1024);
                QVERIFY(im->sourceHeight >= 256 && im->sourceHeight <= 1024);
                QVERIFY(im->pattern != ImagePattern::SolidCenter);
                QVERIFY(!nearBlack(im->colorA) && !nearBlack(im->colorB));
                QVERIFY(im->bounds.width >= 48 && im->bounds.width <= 240);
                QVERIFY(im->bounds.height >= 16 && im->bounds.height <= 768 - 16);
                QVERIFY(im->bounds.x >= 0 && im->bounds.x + im->bounds.width <= 1024);
                QVERIFY(im->bounds.y >= 0 && im->bounds.y + im->bounds.height <= 768);
            } else if (const auto* sh = std::get_if<Shape>(&o)) {
                QVERIFY(!nearBlack(sh->fill) && !nearBlack(sh->outline));
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
