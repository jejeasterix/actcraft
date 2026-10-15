#include "support/flipchartwriter.h"

#include <actcraft/core/document.h>
#include <actcraft/formats/flipchart.h>

#include <QTest>

#include <cmath>
#include <limits>

using namespace actcraft;
using namespace Qt::StringLiterals;
using fctest::Entry;
using fctest::Node;
using fctest::Obj;

namespace {

template <typename T> const T& as(const core::PageObject& o)
{
    const T* p = std::get_if<T>(&o);
    if (!p) {
        qFatal("type d'objet inattendu");
    }
    return *p;
}

// Paperboard d'une page (nœud 103, stockage 102) : `page` = entrées du conteneur de la page, sans l'objet
// page ; `root` = entrées de la racine en plus de l'index et de l'objet paperboard.
QByteArray onePage(const Node& pageIndex, std::vector<Entry> page, std::vector<Entry> root = {})
{
    page.emplace(page.begin(), u"102.io"_s, Obj(2, 102).stream());
    page.emplace(page.begin(), u"103.in"_s, fctest::index(pageIndex));
    root.emplace(root.begin(), u"103.fpg"_s, fctest::container(page));
    root.emplace(root.begin(), u"100.io"_s, Obj(1, 100).stream());
    root.emplace(root.begin(), u"101.in"_s, fctest::index(Node(101, 1, 100, {Node(103, 2, 102)})));
    return fctest::container(root);
}

bool near(double a, double b)
{
    return std::abs(a - b) <= 1e-9 * std::max(1.0, std::abs(b));
}

} // namespace

// Lecteur .flipchart (DA4) sur le paperboard synthétique de fctest::sampleFlipchart.
class TstFormatsFlipchart : public QObject
{
    Q_OBJECT

private slots:
    void readsTheSyntheticFlipchart()
    {
        const auto r = formats::flipchart::readFlipchart(
            fctest::sampleFlipchart({QByteArray("thm-1"), QByteArray("thm-2")}));
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.warnings.size(), 1);
        QVERIFY2(r.warnings.first().contains(u"460"_s), qPrintable(r.warnings.first()));
        const core::Document& d = r.document;
        QCOMPARE(d.pageCount(), std::size_t{2});
        QCOMPARE(d.title(), std::string("Essai synthétique"));
        QCOMPARE(d.resources().size(), std::size_t{2});
        QVERIFY(core::hasUniqueIds(d));
        QCOMPARE(r.thumbnails.size(), std::size_t{2});
        QCOMPARE(r.thumbnails[1], QByteArray("thm-2"));
    }

    void pageOneOrderLayersAndGroup()
    {
        const auto r = formats::flipchart::readFlipchart(fctest::sampleFlipchart());
        const core::Page& p = r.document.page(0);
        QCOMPARE(p.name, std::string("Page un"));
        QCOMPARE(p.notes, std::string("Notes 1"));
        QCOMPARE(p.objects.size(), std::size_t{4});
        // Couche, puis profondeur, puis arbre : groupe (z 0), forme (z 2), texte (z 3), puis l'annotation
        // (couche 2).
        QCOMPARE(core::kindOf(p.objects[0]), core::ObjectKind::Group);
        QCOMPARE(core::kindOf(p.objects[1]), core::ObjectKind::Shape);
        QCOMPARE(core::kindOf(p.objects[2]), core::ObjectKind::Text);
        QCOMPARE(core::kindOf(p.objects[3]), core::ObjectKind::Stroke);
        QCOMPARE(p.objects[3].common.layer, core::Layer::Top);
        QCOMPARE(p.objects[1].common.layer, core::Layer::Middle);
        const auto& group = as<core::Group>(p.objects[0]);
        QCOMPARE(group.members,
                 (std::vector<core::ObjectId>{p.objects[3].common.id, p.objects[1].common.id}));
    }

    void annotationBecomesAHighlighterStroke()
    {
        const auto r = formats::flipchart::readFlipchart(fctest::sampleFlipchart());
        const core::PageObject& o = r.document.page(0).objects[3];
        const auto& s = as<core::Stroke>(o);
        QCOMPARE(o.common.name, std::string("Stylo1"));
        QCOMPARE(s.kind, core::StrokeKind::Highlighter);
        QCOMPARE(s.cap, core::PenCap::Round);
        QCOMPARE(s.baseWidth, 20.0);
        QCOMPARE(s.color, core::Argb{0x80FFFF00u});
        QCOMPARE(s.points.size(), std::size_t{3});
        QCOMPARE(s.points[2].pressure, 0.5f);
        const core::PointF mapped = o.common.transform.map({100, 0});
        QCOMPARE(mapped.x, 100.0);
        QCOMPARE(mapped.y, 300.0);
    }

    void shapeTextAndAction()
    {
        const auto r = formats::flipchart::readFlipchart(fctest::sampleFlipchart());
        const core::Page& p = r.document.page(0);
        const core::PageObject& shapeObject = p.objects[1];
        const auto& shape = as<core::Shape>(shapeObject);
        QCOMPARE(shape.kind, core::ShapeKind::Path);
        QCOMPARE(shape.paths.size(), std::size_t{2});
        QCOMPARE(shape.bounds.width, 86.0);
        QVERIFY(shape.windingFill);
        QCOMPARE(shape.pen.style, core::PenStyle::Dash);
        QCOMPARE(shape.pen.width, 6.0);
        QCOMPARE(shape.brush.kind, core::FillKind::Solid);
        QCOMPARE(shape.brush.color, core::Argb{0xFF008000u});
        QCOMPARE(shapeObject.common.label, std::string("Étiquette"));
        QCOMPARE(shapeObject.common.transform.dx, 300.0);
        QCOMPARE(shapeObject.common.actions.size(), std::size_t{1});
        const core::Action& a = shapeObject.common.actions[0];
        QCOMPARE(a.kind, core::ActionKind::OnObject);
        QCOMPARE(a.sourceType, std::uint16_t{28});
        QCOMPARE(a.command, std::int32_t{184});
        QCOMPARE(a.target, p.objects[2].common.id);
        const core::PageObject& textObject = p.objects[2];
        const auto& text = as<core::Text>(textObject);
        QVERIFY(textObject.common.locked);
        QCOMPARE(text.html, std::string("<p>Bonjour</p>"));
        QCOMPARE(text.runs.size(), std::size_t{1});
        QCOMPARE(text.runs[0].weight, std::int32_t{700});
        QCOMPARE(text.runs[0].pointSize, 24.0);
        QCOMPARE(text.runs[0].color, core::Argb{0xFFFF0000u});
        QCOMPARE(text.runs[0].baseline.y, 30.0);
    }

    void backgroundAndGrid()
    {
        const auto r = formats::flipchart::readFlipchart(fctest::sampleFlipchart());
        const core::Page& p1 = r.document.page(0);
        QCOMPARE(p1.background.fill.kind, core::FillKind::Gradient);
        QCOMPARE(p1.background.fill.color, core::Argb{0xFFC8DCFFu});
        QCOMPARE(p1.background.fill.color2, core::Argb{0xFFFFFFFFu});
        QCOMPARE(p1.background.fill.gradient, core::GradientKind::Vertical);
        QVERIFY(p1.grid.visible);
        QVERIFY(p1.grid.allowSnap);
        QCOMPARE(p1.grid.levels.size(), std::size_t{1});
        QCOMPARE(p1.grid.levels[0].stepX, 50.0);
        QCOMPARE(p1.grid.levels[0].stepY, 25.0);
        QCOMPARE(p1.grid.levels[0].color, core::Argb{0xFF0000FFu});
        QCOMPARE(p1.grid.levels[0].line, core::GridLine::Solid);
        const core::Page& p2 = r.document.page(1);
        QCOMPARE(p2.width, 800.0);
        QCOMPARE(p2.height, 600.0);
        QCOMPARE(p2.background.fill.kind, core::FillKind::Solid);
        QCOMPARE(p2.background.fill.color, core::Argb{0xFFFFFFFFu});
        const core::Resource* image = r.document.resource(p2.background.image);
        QVERIFY(image);
        QCOMPARE(image->extension, std::string("png"));
    }

    void pageTwoImageMediaLineConnector()
    {
        const auto r = formats::flipchart::readFlipchart(fctest::sampleFlipchart());
        const core::Page& p = r.document.page(1);
        QCOMPARE(p.objects.size(), std::size_t{4}); // la formule (460) est sautée
        const auto& image = as<core::ImageObject>(p.objects[0]);
        QCOMPARE(image.resource, p.background.image);
        QVERIFY(image.hasTransparentColor);
        QCOMPARE(image.transparentColor, core::Argb{0xFFFFFFFFu});
        QCOMPARE(image.sourceWidth, std::int32_t{2});
        const core::PointF corner = p.objects[0].common.transform.map({1, 1});
        QCOMPARE(corner.x, 60.0);
        QCOMPARE(corner.y, 60.0);
        const auto& line = as<core::Line>(p.objects[1]);
        QCOMPARE(line.points.size(), std::size_t{2});
        QCOMPARE(line.pen.color, core::Argb{0xFF0000FFu});
        QCOMPARE(line.pen.width, 3.0);
        const auto& media = as<core::Media>(p.objects[2]);
        QCOMPARE(media.placeholder, image.resource);
        QCOMPARE(media.fileName, std::string("351.mp3"));
        QVERIFY(media.autoPlay);
        QCOMPARE(r.document.resource(media.media)->extension, std::string("mp3"));
        const auto& connector = as<core::Connector>(p.objects[3]);
        QCOMPARE(connector.objectA, p.objects[0].common.id);
        QCOMPARE(connector.objectB, p.objects[1].common.id);
        QCOMPARE(connector.endB.x, 100.0);
    }

    // Repères et numéros propres à chaque classe (§ 7.1, § 7.3, § 7.4). Les matrices attendues sont calculées
    // par la référence (flipchart_reader.py : Transform.matrix, compose), pas par notre code. Discrimine :
    // couche 0 prise pour l'arrière-plan, table de propriétés à plat (38 = ressource d'une forme mais
    // translucidité d'une annotation, dont la ressource est 39), translucidité convertie au lieu d'être
    // gardée sur 0-255, miroir appliqué après la transformation au lieu d'avant, origine de rotation ignorée,
    // enfant composé à l'envers.
    void geometryLayersAndTranslucencyFollowTheFormat()
    {
        const QByteArray data = onePage(
            Node(103, 2, 102,
                 {Node(110, 7, 111, {Node(120, 5, 121), Node(122, 28, 123)}), Node(130, 3, 131)}),
            {Entry(u"111.io"_s,
                   Obj(7, 111)
                       .object(2, 7,
                               Obj(15, 0).pointF(2, 200, 100).real(3, 30).pointF(4, 2, 3).boolean(5, true))
                       .object(12, 8, Obj(21, 0).pointF(2, 10, 5))
                       .userRect(7, 0, 0, 40, 20)
                       .integer(38, 320)
                       .stream()),
             Entry(u"121.io"_s, Obj(5, 121).object(2, 7, Obj(15, 0).pointF(2, 5, 0)).stream()),
             Entry(u"123.io"_s, Obj(28, 123).pointer(2, 120, 121, 5).integer(5, 184).stream()),
             Entry(u"131.io"_s, Obj(3, 131).integer(8, 0).integer(38, 100).integer(39, 310).stream())},
            {Entry(u"310.ir"_s, Obj(4, 310).points(4, {QPointF(0, 0), QPointF(10, 10)}).stream()),
             Entry(u"320.ir"_s,
                   Obj(8, 320).polygons(4, {{QPointF(0, 0), QPointF(40, 0), QPointF(40, 20)}}).stream())});
        const auto r = formats::flipchart::readFlipchart(data);
        QVERIFY2(r.ok, qPrintable(r.error));
        QVERIFY2(r.warnings.isEmpty(), qPrintable(r.warnings.join(u'\n')));
        const auto& objects = r.document.page(0).objects;
        QCOMPARE(objects.size(), std::size_t{3});

        // Annotation : couche 0 = inférieure (pas l'arrière-plan), translucidité brute sur 0-255, ressource
        // en 39.
        const core::PageObject& ink = objects[0];
        QCOMPARE(core::kindOf(ink), core::ObjectKind::Stroke);
        QCOMPARE(ink.common.layer, core::Layer::Bottom);
        QCOMPARE(ink.common.translucency, std::uint8_t{100});
        QCOMPARE(as<core::Stroke>(ink).points.size(), std::size_t{2});

        // Forme : 38 est sa ressource, pas une translucidité.
        const core::PageObject& shape = objects[1];
        QCOMPARE(core::kindOf(shape), core::ObjectKind::Shape);
        QCOMPARE(shape.common.layer, core::Layer::Middle);
        QCOMPARE(shape.common.translucency, std::uint8_t{0});
        QCOMPARE(as<core::Shape>(shape).paths.size(), std::size_t{1});
        // translate(t + o) ; rotate(30) ; scale(2, 3) ; translate(-o), miroir horizontal sur la largeur 40
        // d'abord.
        const core::Transform& m = shape.common.transform;
        QVERIFY2(near(m.m11, -1.7320508075688774) && near(m.m12, -1.0) && near(m.m21, -1.5) &&
                     near(m.m22, 2.598076211353316) && near(m.dx, 269.4615242270663) &&
                     near(m.dy, 122.00961894323342),
                 qPrintable(
                     u"%1 %2 %3 %4 %5 %6"_s.arg(m.m11).arg(m.m12).arg(m.m21).arg(m.m22).arg(m.dx).arg(m.dy)));

        // Enfant : suit son parent ; sa transformation est la sienne, puis celle du parent.
        const core::PageObject& child = objects[2];
        QCOMPARE(core::kindOf(child), core::ObjectKind::Line);
        const core::Transform& c = child.common.transform;
        QVERIFY2(near(c.m11, m.m11) && near(c.m22, m.m22) && near(c.dx, 260.80127018922195) &&
                     near(c.dy, 117.00961894323342),
                 qPrintable(u"%1 %2"_s.arg(c.dx).arg(c.dy)));
        // Action enfant d'un objet : rattachée à cet objet, cible résolue.
        QCOMPARE(shape.common.actions.size(), std::size_t{1});
        QCOMPARE(shape.common.actions[0].target, child.common.id);
    }

    void unreadablePagesAreSkippedWithAWarning()
    {
        // Deux pages dans l'index, une seule dans le conteneur.
        const QByteArray page = fctest::container(
            {Entry(u"103.in"_s, fctest::index(Node(103, 2, 102))), Entry(u"102.io"_s, Obj(2, 102).stream())});
        const QByteArray data = fctest::container(
            {Entry(u"101.in"_s, fctest::index(Node(101, 1, 100, {Node(103, 2, 102), Node(203, 2, 202)}))),
             Entry(u"100.io"_s, Obj(1, 100).stream()), Entry(u"103.fpg"_s, page)});
        const auto r = formats::flipchart::readFlipchart(data);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.document.pageCount(), std::size_t{1});
        QVERIFY(r.warnings.join(u' ').contains(u"203"_s));
    }

    void absurdNumbersFallBackToDefaults()
    {
        // Couche NaN, profondeur 1e30, taille d'image hors bornes : défauts, aucune conversion indéfinie.
        const QByteArray page = fctest::container(
            {Entry(u"103.in"_s, fctest::index(Node(103, 2, 102, {Node(110, 7, 111), Node(112, 9, 113)}))),
             Entry(u"102.io"_s, Obj(2, 102).stream()),
             Entry(u"111.io"_s,
                   Obj(7, 111).real(8, std::numeric_limits<double>::quiet_NaN()).real(9, 1e30).stream()),
             Entry(u"113.io"_s, Obj(9, 113).integer(38, 300).stream())});
        const QByteArray page2 = fctest::container(
            {Entry(u"203.in"_s, fctest::index(Node(203, 2, 202, {Node(210, 9, 211), Node(212, 9, 213)}))),
             Entry(u"202.io"_s, Obj(2, 202).stream()),
             Entry(u"211.io"_s, Obj(9, 211).integer(38, 999).stream()),
             Entry(u"213.io"_s, Obj(9, 213).integer(38, 999).stream())});
        const QByteArray data = fctest::container(
            {Entry(u"101.in"_s, fctest::index(Node(101, 1, 100, {Node(103, 2, 102), Node(203, 2, 202)}))),
             Entry(u"100.io"_s, Obj(1, 100).stream()), Entry(u"103.fpg"_s, page), Entry(u"203.fpg"_s, page2),
             Entry(u"300.ir"_s, Obj(10, 300).sizeF(5, 1e30, 1e30).stream()),
             Entry(u"300.png"_s, fctest::pngBytes(2, 2, qRgb(0, 0, 255)), false)});
        const auto r = formats::flipchart::readFlipchart(data);
        QVERIFY2(r.ok, qPrintable(r.error));
        const auto& objects = r.document.page(0).objects;
        QCOMPARE(objects.size(), std::size_t{2});
        // Avec un cast direct, NaN donne INT_MIN sous MSVC : « couche -2147483648 inconnue ». Ici, défaut
        // sans bruit.
        QVERIFY2(!r.warnings.join(u' ').contains(u"couche"_s), qPrintable(r.warnings.join(u'\n')));
        for (const core::PageObject& o : objects) {
            if (const auto* im = std::get_if<core::ImageObject>(&o)) {
                QCOMPARE(im->sourceWidth, std::int32_t{256}); // taille 1e30 × 1e30 : défaut du modèle
            } else {
                QCOMPARE(o.common.layer, core::Layer::Middle);
            }
        }
        // Mémorisation par ressource : deux images sur la même ressource absente → un seul avertissement.
        QCOMPARE(r.warnings.filter(u"999"_s).size(), 1);
    }

    // Entrée hostile : objet tronqué, propriétés d'un type inattendu, transformation non finie, nœud absent.
    // L'objet illisible est sauté et signalé ; les autres sont lus avec leurs défauts ; jamais d'exception.
    void hostileObjectsAreSkippedOrDefaulted()
    {
        const double nan = std::numeric_limits<double>::quiet_NaN();
        const QByteArray full = Obj(7, 111).integer(8, 2).integer(38, 320).stream();
        const QByteArray data = onePage(
            Node(103, 2, 102, {Node(110, 7, 111), Node(112, 7, 113), Node(114, 7, 115), Node(116, 7, 117)}),
            {Entry(u"111.io"_s, full.left(full.size() - 6)),
             Entry(u"113.io"_s, Obj(7, 113)
                                    .integer(2, 5)              // transformation : un entier
                                    .string(8, u"haut"_s)       // couche : un texte
                                    .color(7, 1, 2, 3)          // rectangle englobant : une couleur
                                    .string(10, u"encre"_s)     // encre : un texte
                                    .string(38, u"320"_s)       // ressource : un texte
                                    .object(34, 12, Obj(15, 0)) // étiquette sans légende
                                    .pointer(39, 1, 2, 3)       // mode de remplissage : un pointeur
                                    .stream()),
             Entry(u"115.io"_s,
                   Obj(7, 115).object(2, 7, Obj(15, 0).pointF(2, nan, 0)).real(9, nan).stream())},
            {Entry(u"320.ir"_s, Obj(8, 320).polygons(4, {{QPointF(0, 0), QPointF(1, 1)}}).stream())});
        const auto r = formats::flipchart::readFlipchart(data);
        QVERIFY2(r.ok, qPrintable(r.error));
        const QString warnings = r.warnings.join(u'\n');
        QVERIFY2(warnings.contains(u"111.io"_s), qPrintable(warnings)); // tronqué : illisible
        QVERIFY2(warnings.contains(u"117.io"_s), qPrintable(warnings)); // absent
        QVERIFY2(warnings.contains(u"transformation"_s), qPrintable(warnings));
        const auto& objects = r.document.page(0).objects;
        QCOMPARE(objects.size(), std::size_t{2});
        for (const core::PageObject& o : objects) {
            const auto& shape = as<core::Shape>(o);
            QVERIFY(o.common.transform.isIdentity());
            QCOMPARE(o.common.layer, core::Layer::Middle);
            QVERIFY(shape.paths.empty());
            QVERIFY(!shape.windingFill);
            QCOMPARE(shape.pen.width, 1.0);
            QVERIFY(o.common.label.empty());
        }
    }

    // Arbre d'objets imbriqués sur 200 niveaux (l'index en admet 256) : la lecture est bornée, sans
    // débordement de pile ; les niveaux au-delà de la borne sont sautés et signalés.
    void deepNestingIsBounded()
    {
        constexpr quint32 kChain = 200;
        std::vector<Entry> entries;
        Node chain(1000 + kChain - 1, 7, 2000 + kChain - 1);
        for (quint32 i = 0; i < kChain; ++i) {
            entries.emplace_back(u"%1.io"_s.arg(2000 + i), Obj(7, 2000 + i).stream());
        }
        for (quint32 i = kChain - 1; i-- > 0;) {
            chain = Node(1000 + i, 7, 2000 + i, {chain});
        }
        const auto r = formats::flipchart::readFlipchart(onePage(Node(103, 2, 102, {chain}), entries));
        QVERIFY2(r.ok, qPrintable(r.error));
        const std::size_t count = r.document.page(0).objects.size();
        QVERIFY2(count >= 16 && count < kChain, qPrintable(QString::number(count)));
        QVERIFY2(r.warnings.join(u' ').contains(u"imbriqu"_s), qPrintable(r.warnings.join(u'\n')));
        QVERIFY(core::hasUniqueIds(r.document));
    }

    void refusesWhatIsNotAFlipchart()
    {
        QVERIFY(!formats::flipchart::looksLikeFlipchart("PK\x03\x04"));
        QVERIFY(formats::flipchart::looksLikeFlipchart(fctest::sampleFlipchart().left(8)));
        auto r = formats::flipchart::readFlipchart("pas un paperboard");
        QVERIFY(!r.ok);
        QVERIFY(!r.error.isEmpty());
        r = formats::flipchart::readFlipchart(fctest::container({Entry(u"100.io"_s, Obj(1, 100).stream())}));
        QVERIFY(!r.ok);
        QVERIFY2(r.error.contains(u"index"_s), qPrintable(r.error));
        r = formats::flipchart::readFlipchart(
            fctest::container({Entry(u"101.in"_s, fctest::index(Node(101, 1, 100, {Node(103, 2, 102)}))),
                               Entry(u"100.io"_s, Obj(1, 100).stream())}));
        QVERIFY(!r.ok);
        QVERIFY2(r.error.contains(u"aucune page"_s), qPrintable(r.error));
    }
};

QTEST_GUILESS_MAIN(TstFormatsFlipchart)
#include "tst_formats_flipchart.moc"
