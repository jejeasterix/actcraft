#include <actcraft/core/clock.h>
#include <actcraft/core/document.h>
#include <actcraft/core/generator.h>
#include <actcraft/core/model.h>

#include <QTest>

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <variant>

using namespace actcraft::core;

// Empreinte de la page générée du banc (generatePage(kDefaultSeed)) relevée avant le socle des lots 3 et 7 :
// valeur figée de tst_core_generator (kExpectedFingerprint, fingerprintIsPinned), verte sur b904dcf.
constexpr std::uint64_t kBenchPageFingerprint = 0x3551EB2438FDFDC1ull;

class TstCoreModel : public QObject
{
    Q_OBJECT

private:
    static Page samplePage()
    {
        Page page;
        Shape shape;
        shape.kind = ShapeKind::Polygon;
        shape.bounds = {10, 20, 30, 40};
        shape.polygon = {{10, 20}, {40, 20}, {25, 60}};
        shape.fill = 0xFF102030u;
        page.objects.emplace_back(shape);
        ImageObject image;
        image.bounds = {100, 100, 64, 48};
        image.pattern = ImagePattern::Checker;
        page.objects.emplace_back(image);
        Stroke stroke;
        stroke.points = {{1, 2, 0.25f}, {3, 4, 0.5f}};
        page.objects.emplace_back(stroke);
        return page;
    }

    // Un objet de chaque type ajouté par le lot 2, champs hors défaut.
    static Page richPage()
    {
        Page page = samplePage();
        Line line;
        line.points = {{0, 0}, {10, 5}};
        line.pen.width = 3;
        page.objects.emplace_back(line);
        Text text;
        text.bounds = {0, 0, 90, 36};
        text.html = "<p>Bonjour</p>";
        TextSpan run;
        run.text = "Bonjour";
        run.fontFamily = "Arial";
        run.pointSize = 24;
        text.runs.push_back(run);
        page.objects.emplace_back(text);
        Media media;
        media.bounds = {0, 0, 64, 48};
        media.media = "abc";
        page.objects.emplace_back(media);
        Group group;
        group.members = {1, 2};
        page.objects.emplace_back(group);
        Connector connector;
        connector.secondEnd = {100, 100};
        page.objects.emplace_back(connector);
        Equation equation;
        equation.bounds = {0, 0, 100, 40};
        equation.mathml = "<math><mn>1</mn></math>";
        page.objects.emplace_back(equation);
        page.grid.levels.push_back(GridLevel{});
        return page;
    }

private slots:
    // L4-C19 : l'égalité d'un remplissage couvre chaque champ (un champ ajouté et oublié se verrait ici).
    void fillEqualityCoversEveryField()
    {
        const Fill base;
        QVERIFY(base == Fill{});
        const auto differs = [&](auto change) {
            Fill other = base;
            change(other);
            return !(other == base);
        };
        QVERIFY(differs([](Fill& f) { f.kind = FillKind::Solid; }));
        QVERIFY(differs([](Fill& f) { f.color = 0x11223344u; }));
        QVERIFY(differs([](Fill& f) { f.color2 = 0x11223344u; }));
        QVERIFY(differs([](Fill& f) { f.gradient = GradientKind::DiagonalDown; }));
        QVERIFY(differs([](Fill& f) { f.pattern = 3; }));
        QVERIFY(differs([](Fill& f) { f.opaqueBackground = true; }));
        QVERIFY(differs([](Fill& f) { f.backgroundColor = 0x11223344u; }));
    }

    void kindOfFollowsVariantOrder()
    {
        const Page page = richPage();
        QCOMPARE(kindOf(page.objects[0]), ObjectKind::Shape);
        QCOMPARE(kindOf(page.objects[1]), ObjectKind::Image);
        QCOMPARE(kindOf(page.objects[2]), ObjectKind::Stroke);
        QCOMPARE(kindOf(page.objects[3]), ObjectKind::Line);
        QCOMPARE(kindOf(page.objects[4]), ObjectKind::Text);
        QCOMPARE(kindOf(page.objects[5]), ObjectKind::Media);
        QCOMPARE(kindOf(page.objects[6]), ObjectKind::Group);
        QCOMPARE(kindOf(page.objects[7]), ObjectKind::Connector);
        QCOMPARE(kindOf(page.objects[8]), ObjectKind::Equation);
        QCOMPARE(std::variant_size_v<ObjectVariant>, kObjectKindCount);
    }

    // Socle des lots 3 et 7 : l'équation prend le rang 8, avant le type inconnu, toujours le dernier.
    void equationKindIsBeforeUnknown()
    {
        QCOMPARE(static_cast<int>(ObjectKind::Equation), 8);
        QCOMPARE(static_cast<int>(ObjectKind::Unknown), 10); // lot 8 : le cadre web prend le rang 9 (L8-T3)
        QCOMPARE(kObjectKindCount, std::size_t{11});
        PageObject o{Equation{RectF{0, 0, 100, 40}, "<math><mn>1</mn></math>", 24, 0xFF000000u}};
        QCOMPARE(kindOf(o), ObjectKind::Equation);
    }

    void defaultPresentationIsDefault()
    {
        QVERIFY(isDefault(PagePresentation{}));
        PagePresentation p;
        p.curtain = CurtainMode::Top;
        QVERIFY(!isDefault(p));
        PagePresentation kept;
        kept.unknownKeys = "{\"future\":1}";
        QVERIFY(!isDefault(kept)); // une sous-clé gardée doit être réécrite
    }

    // I1 : une page sans réglages de présentation garde son empreinte d'avant ; un réglage y entre.
    void fingerprintUnchangedWithoutPresentation()
    {
        Page page = generatePage(kDefaultSeed);
        QCOMPARE(fingerprint(page), kBenchPageFingerprint);
        page.presentation.transition = "fade";
        QVERIFY(fingerprint(page) != kBenchPageFingerprint);
        const std::uint64_t withTransition = fingerprint(page);
        page.presentation.unknownKeys = "{\"future\":1}";
        QVERIFY(fingerprint(page) != withTransition);
        page.presentation = PagePresentation{};
        QCOMPARE(fingerprint(page), kBenchPageFingerprint);
    }

    void pageObjectKeepsVariantAccessAndCommonProperties()
    {
        Page page = samplePage();
        PageObject& object = page.objects[2];
        QVERIFY(std::get_if<Stroke>(&object) != nullptr);
        QVERIFY(std::get_if<Shape>(&object) == nullptr);
        QCOMPARE(object.common.layer, Layer::Middle);
        QCOMPARE(object.common.id, ObjectId{0});
        QVERIFY(object.common.transform.isIdentity());
        object.common.layer = Layer::Top;
        const PageObject copy = object;
        QCOMPARE(copy.common.layer, Layer::Top);
        QCOMPARE(std::get<Stroke>(copy).points.size(), std::size_t{2});
        QCOMPARE(body(copy).index(), std::size_t{2});
    }

    void defaultPageIs1024By768OnWhite()
    {
        const Page page;
        QCOMPARE(page.width, 1024.0);
        QCOMPARE(page.height, 768.0);
        QVERIFY(page.objects.empty());
        QCOMPARE(page.background.fill.kind, FillKind::Solid);
        QCOMPARE(page.background.fill.color, Argb{0xFFFFFFFFu});
        QVERIFY(page.background.image.empty());
        QVERIFY(!page.grid.visible);
        QVERIFY(page.grid.levels.empty());
    }

    void transformsComposeLikeQTransform()
    {
        // Mise à l'échelle puis rotation de 90° (sens horaire, y vers le bas) puis translation.
        const Transform t = compose(compose(scaling(2, 1), rotation(90)), translation(100, 100));
        const PointF p = t.map({100, 0});
        QCOMPARE(p.x, 100.0);
        QCOMPARE(p.y, 300.0);
        const Transform r = rotation(90);
        QCOMPARE(r.m11, 0.0);
        QCOMPARE(r.m12, 1.0);
        QCOMPARE(r.m21, -1.0);
        QCOMPARE(r.m22, 0.0);
        QVERIFY(rotation(360).isIdentity());
        // Pas de −0 : l'empreinte hache le motif binaire, et −0 == 0 cacherait l'écart à isIdentity().
        QVERIFY(!std::signbit(rotation(0).m21));
        QVERIFY(!std::signbit(rotation(180).m21));
        QCOMPARE(rotation(-90).m12, -1.0);
        const PointF q = rotation(30).map({1, 0});
        QVERIFY(qAbs(q.x - 0.8660254037844386) < 1e-12);
        QVERIFY(qAbs(q.y - 0.5) < 1e-12);
    }

    void solidFillSetsKindAndColor()
    {
        const Fill f = solidFill(0xFF00FF00u);
        QCOMPARE(f.kind, FillKind::Solid);
        QCOMPARE(f.color, Argb{0xFF00FF00u});
    }

    void fingerprintIsStable() { QCOMPARE(fingerprint(richPage()), fingerprint(richPage())); }

    void fingerprintSeesEveryField()
    {
        const std::uint64_t reference = fingerprint(richPage());
        const auto differs = [reference](auto mutate) {
            Page p = richPage();
            mutate(p);
            return fingerprint(p) != reference;
        };
        QVERIFY(differs([](Page& p) { std::get<Stroke>(p.objects[2]).points[1].pressure = 0.5001f; }));
        QVERIFY(differs([](Page& p) { std::get<Shape>(p.objects[0]).polygon[2].y = 61; }));
        QVERIFY(differs([](Page& p) { std::get<ImageObject>(p.objects[1]).pattern = ImagePattern::Rings; }));
        QVERIFY(differs([](Page& p) { std::swap(p.objects[0], p.objects[2]); }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.id = 7; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.name = "Forme1"; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.layer = Layer::Bottom; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.visible = false; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.locked = true; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.translucency = 1; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.restrictions.resizable = false; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.transform.dx = 0.5; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.label = "Légende"; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.actions.push_back(Action{}); }));
        QVERIFY(differs([](Page& p) { std::get<Shape>(p.objects[0]).paths.push_back({{1, 1}}); }));
        QVERIFY(
            differs([](Page& p) { std::get<Shape>(p.objects[0]).brush.gradient = GradientKind::Radial; }));
        QVERIFY(differs([](Page& p) { std::get<Shape>(p.objects[0]).pen.join = PenJoin::Bevel; }));
        QVERIFY(differs([](Page& p) { std::get<ImageObject>(p.objects[1]).resource = "x"; }));
        QVERIFY(differs([](Page& p) { std::get<Stroke>(p.objects[2]).kind = StrokeKind::Highlighter; }));
        QVERIFY(differs([](Page& p) { std::get<Line>(p.objects[3]).pen.style = PenStyle::Dash; }));
        QVERIFY(differs([](Page& p) { std::get<Text>(p.objects[4]).html = "<p>Bonsoir</p>"; }));
        QVERIFY(differs([](Page& p) { std::get<Text>(p.objects[4]).runs[0].weight = 700; }));
        QVERIFY(differs([](Page& p) { std::get<Media>(p.objects[5]).loop = true; }));
        QVERIFY(differs([](Page& p) { std::get<Group>(p.objects[6]).members.push_back(3); }));
        QVERIFY(differs([](Page& p) { std::get<Connector>(p.objects[7]).objectA = 9; }));
        QVERIFY(differs([](Page& p) { p.background.fill.color2 = 0xFF000000u; }));
        QVERIFY(differs([](Page& p) { p.background.image = "cle"; }));
        QVERIFY(differs([](Page& p) { p.grid.visible = true; }));
        QVERIFY(differs([](Page& p) { p.grid.levels[0].spacingY = 31; }));
        QVERIFY(differs([](Page& p) { p.name = "Page un"; }));
        QVERIFY(differs([](Page& p) { p.notes = "notes"; }));
    }

    // Valeurs recoupées hors C++ par le script Python de L2-T1 (étape 7), qui hache les octets
    // petit-boutistes des champs dans l'ordre de model.cpp : FNV-1a 64 bits, base 0xCBF29CE484222325,
    // multiplicateur 0x100000001B3. Valeurs du lot 1 avant l'ajout des champs du lot 2 : 0xD93C4EE35CD928AD
    // et 0x21FF926A276797BC.
    void fingerprintMatchesIndependentReference()
    {
        QCOMPARE(fingerprint(Page{}), std::uint64_t{0xB59B2DE2A9B3C823ull});
        QCOMPARE(fingerprint(samplePage()), std::uint64_t{0x29E4AA55B1A00D2Eull});
    }

    // −0 et +0 se valent (même rendu) : l'empreinte ne doit pas les distinguer. Un NaN, lui, garde son motif
    // binaire (inchangé) : égal à lui-même, différent de 0.
    void fingerprintTreatsNegativeZeroAsPositiveZero()
    {
        const auto withZero = [](double zero) {
            Page p = richPage();
            std::get<Shape>(p.objects[0]).polygon[0].x = zero;
            std::get<Stroke>(p.objects[2]).points[0].y = zero;
            std::get<Stroke>(p.objects[2]).points[0].pressure = static_cast<float>(zero);
            p.objects[0].common.transform.m12 = zero;
            p.objects[1].common.transform.dx = zero;
            std::get<ImageObject>(p.objects[1]).bounds.x = zero;
            p.grid.levels[0].angle = zero;
            return p;
        };
        const Page positive = withZero(0.0);
        const Page negative = withZero(-0.0);
        QVERIFY(std::signbit(std::get<Shape>(negative.objects[0]).polygon[0].x));
        QCOMPARE(fingerprint(negative), fingerprint(positive));

        const auto withNan = [](double v) {
            Page p = richPage();
            p.objects[0].common.transform.m12 = v;
            return p;
        };
        const double nan = std::numeric_limits<double>::quiet_NaN();
        QCOMPARE(fingerprint(withNan(nan)), fingerprint(withNan(nan)));
        QVERIFY(fingerprint(withNan(nan)) != fingerprint(withNan(0.0)));
    }

    // L2-C8 : un objet d'un type inconnu a son rang, le dernier, après tous les types connus.
    void unknownObjectIsTheLastKind()
    {
        const PageObject o = UnknownObject{};
        QCOMPARE(kindOf(o), ObjectKind::Unknown);
        QCOMPARE(static_cast<std::size_t>(ObjectKind::Unknown) + 1, kObjectKindCount);
        QCOMPARE(kObjectKindCount, std::size_t{11}); // lot 8 : cadre web (L8-T3)
    }

    // L2-C8 : ce que la page garde pour une version plus récente (objet inconnu, clés inconnues) est écrit à
    // l'enregistrement, donc entre dans l'empreinte ; une page qui n'en a pas garde son empreinte d'avant
    // (fingerprintMatchesIndependentReference).
    void fingerprintSeesWhatIsKeptForANewerVersion()
    {
        const auto withUnknown = [](const std::string& type, const std::string& json, RectF bounds,
                                    bool hasBounds) {
            Page p = samplePage();
            UnknownObject u;
            u.type = type;
            u.json = json;
            u.bounds = bounds;
            u.hasBounds = hasBounds;
            p.objects.insert(p.objects.begin() + 1, PageObject(u));
            return p;
        };
        const std::string json = "{\"latex\":\"x\",\"type\":\"equation\"}";
        const std::uint64_t reference = fingerprint(withUnknown("equation", json, {1, 2, 3, 4}, true));
        QVERIFY(reference != fingerprint(samplePage()));
        QVERIFY(reference != fingerprint(withUnknown("chart", json, {1, 2, 3, 4}, true)));
        QVERIFY(reference != fingerprint(withUnknown("equation", "{\"latex\":\"y\",\"type\":\"equation\"}",
                                                     {1, 2, 3, 4}, true)));
        QVERIFY(reference != fingerprint(withUnknown("equation", json, {1, 2, 3, 5}, true)));
        QVERIFY(reference != fingerprint(withUnknown("equation", json, {1, 2, 3, 4}, false)));

        const std::string keys = "{\"future\":1}";
        Page first = samplePage();
        first.objects[0].unknownKeys = keys;
        Page last = samplePage();
        last.objects[2].unknownKeys = keys;
        Page page = samplePage();
        page.unknownKeys = keys;
        const std::uint64_t plain = fingerprint(samplePage());
        QVERIFY(fingerprint(first) != plain);
        QVERIFY(fingerprint(last) != plain);
        QVERIFY(fingerprint(page) != plain);
        QVERIFY(fingerprint(first) != fingerprint(last)); // l'objet qui porte la clé compte
        QVERIFY(fingerprint(first) != fingerprint(page));
        first.objects[0].unknownKeys = "{\"future\":2}";
        QVERIFY(fingerprint(first) != fingerprint(last));

        // Éléments des tableaux (action, ligne de texte, niveau de grille) : avec leur élément.
        Page acted = richPage();
        acted.objects[0].common.actions.push_back(Action{});
        Page actedKeyed = acted;
        actedKeyed.objects[0].common.actions[0].unknownKeys = keys;
        QVERIFY(fingerprint(actedKeyed) != fingerprint(acted));
        Page run = richPage();
        std::get<Text>(run.objects[4]).runs[0].unknownKeys = keys;
        QVERIFY(fingerprint(run) != fingerprint(richPage()));
        Page level = richPage();
        level.grid.levels[0].unknownKeys = keys;
        QVERIFY(fingerprint(level) != fingerprint(richPage()));
    }

    // L2-C8 : action, ligne de texte et niveau de grille sans clé inconnue n'ajoutent rien au flux. Valeur
    // calculée sur la base du chantier (avant L2-C8, 999d548) pour la page de richPage() avec une action.
    void fingerprintOfElementsWithoutUnknownKeysIsUnchanged()
    {
        Page page = richPage();
        QCOMPARE(kindOf(page.objects.back()), ObjectKind::Equation);
        page.objects.pop_back(); // l'équation de richPage() (socle des lots 3 et 7) n'existait pas à 999d548
        page.objects[0].common.actions.push_back(Action{});
        QCOMPARE(fingerprint(page), std::uint64_t{0x109437226CD48FFBull});
    }

    void strokeWidthFollowsPressure()
    {
        QCOMPARE(strokeWidthAt(3.0, 0.5f), 3.0);
        QCOMPARE(strokeWidthAt(3.0, 0.0f), 1.5);
        QCOMPARE(strokeWidthAt(3.0, 1.0f), 4.5);
    }

    void rectContainsAndCenter()
    {
        const RectF r{10, 20, 30, 40};
        QVERIFY(r.contains({10, 20}));
        QVERIFY(r.contains({40, 60}));
        QVERIFY(!r.contains({41, 60}));
        QCOMPARE(r.center().x, 25.0);
        QCOMPARE(r.center().y, 40.0);
    }

    void clockIsMonotonic()
    {
        const std::int64_t a = monotonicNowNs();
        const std::int64_t b = monotonicNowNs();
        QVERIFY(b >= a);
        QVERIFY(a > 0);
    }
    void lot4FieldsKeepOldFingerprints()
    {
        using namespace actcraft::core;
        Page page;
        PageObject shape{Shape{}};
        shape.common.id = 1;
        PageObject text{Text{}};
        text.common.id = 2;
        page.objects = {shape, text};
        const auto before = fingerprint(page);
        std::get<Shape>(body(page.objects[0])).preset = "arrow";
        QVERIFY(fingerprint(page) != before);
        std::get<Shape>(body(page.objects[0])).preset.clear();
        QCOMPARE(fingerprint(page), before);
        std::get<Shape>(body(page.objects[0])).parameters = {0.3};
        QVERIFY(fingerprint(page) != before);
        std::get<Shape>(body(page.objects[0])).parameters.clear();
        std::get<Text>(body(page.objects[1])).vertical = true;
        QVERIFY(fingerprint(page) != before);
        std::get<Text>(body(page.objects[1])).vertical = false;
        page.baseHeight = 768;
        QVERIFY(fingerprint(page) != before);
        page.baseHeight = 0;
        QCOMPARE(fingerprint(page), before);
    }

    // L6-T10 : mots-clés (page, objet), description de page, style de légende. Absents, rien dans le flux :
    // les valeurs figées restent. Présents, ils entrent en toute fin de flux derrière l'octet 0x6A :
    // l'empreinte est alors la suite FNV-1a de celle de la même page sans eux. Un champ glissé au milieu du
    // flux, même seulement hors défaut (comme dragCopy), donnerait une autre valeur.
    // Lot 8 (L8-T3) : les champs de l'interactivité à leur défaut n'entrent pas dans l'empreinte. Valeurs
    // figées à la tête avant L8-T3 (c61013da) : la page riche, son document, la page riche avec une action
    // par objet.
    void lot8FieldsKeepOldFingerprints()
    {
        constexpr std::uint64_t kRichPage = 0x98D4A66CE413F16Dull;
        constexpr std::uint64_t kRichDocument = 0x3E73AE77B3BB378Full;
        constexpr std::uint64_t kRichActed = 0x6E89DB6AA5FC4D04ull;
        const auto toDefaults = [](Page& page) {
            for (PageObject& object : page.objects) {
                Restrictions& r = object.common.restrictions;
                r.travel = Travel::Free;
                r.track = 0;
                r.turning = Turning::Free;
                r.turnStep = 0;
                r.pivot = Mark::Center;
                r.pivotPoint = PointF{};
                r.pivotObject = 0;
                r.blocksOthers = false;
                r.magnet = false;
                r.magnetMark = Mark::Center;
                r.magnetPoint = PointF{};
                object.common.container = ContainerRule{};
                for (Action& a : object.common.actions) {
                    a.catalogId.clear();
                    a.parameters.clear();
                    a.dragDrop = false;
                }
                if (Media* m = std::get_if<Media>(&body(object))) {
                    m->controlsHidden = false;
                    m->aspectLocked = false;
                    m->clearBackground = false;
                    m->wrapText = false;
                }
            }
        };
        QCOMPARE(fingerprint(richPage()), kRichPage);
        Page page = richPage();
        toDefaults(page);
        QCOMPARE(fingerprint(page), kRichPage);

        Page acted = richPage();
        Action action;
        action.kind = ActionKind::OnObject;
        action.sourceType = 28;
        action.command = 184;
        action.target = 9;
        for (PageObject& object : acted.objects) {
            object.common.actions.push_back(action);
        }
        QCOMPARE(fingerprint(acted), kRichActed);
        toDefaults(acted);
        QCOMPARE(fingerprint(acted), kRichActed);

        Document document(std::vector<Page>{richPage()});
        QCOMPARE(fingerprint(document), kRichDocument);
        document.setBroadcast(BroadcastRules{});
        QCOMPARE(fingerprint(document), kRichDocument);

        // Chaque champ hors défaut compte, sur l'objet qui le porte.
        const auto differs = [&acted](auto mutate) {
            Page p = acted;
            mutate(p);
            return fingerprint(p) != kRichActed;
        };
        constexpr std::size_t kMedia = 5;
        QVERIFY(differs([](Page& p) { p.objects[0].common.restrictions.travel = Travel::Vertical; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.restrictions.track = 4; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.restrictions.turning = Turning::Clockwise; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.restrictions.turnStep = 15; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.restrictions.pivot = Mark::TopLeft; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.restrictions.pivotPoint = PointF{3, 4}; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.restrictions.pivotObject = 2; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.restrictions.blocksOthers = true; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.restrictions.magnet = true; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.restrictions.magnetMark = Mark::Bottom; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.restrictions.magnetPoint = PointF{1, 2}; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.container.accepts = Accepts::Keywords; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.container.object = 3; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.container.keywords = {"lune"}; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.container.fit = FitRule::FullyInside; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.container.tolerance = 20; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.container.bounceBack = true; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.container.chime = true; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.container.chimeSound = "son"; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.actions[0].catalogId = "object.toggle-hidden"; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.actions[0].parameters = "{}"; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.actions[0].dragDrop = true; }));
        QVERIFY(differs([](Page& p) { std::get<Media>(p.objects[kMedia]).controlsHidden = true; }));
        QVERIFY(differs([](Page& p) { std::get<Media>(p.objects[kMedia]).aspectLocked = true; }));
        QVERIFY(differs([](Page& p) { std::get<Media>(p.objects[kMedia]).clearBackground = true; }));
        QVERIFY(differs([](Page& p) { std::get<Media>(p.objects[kMedia]).wrapText = true; }));
        Page onFirst = acted;
        onFirst.objects[0].common.container.accepts = Accepts::Anything;
        Page onSecond = acted;
        onSecond.objects[1].common.container.accepts = Accepts::Anything;
        QVERIFY(fingerprint(onFirst) != fingerprint(onSecond));
        Page joined = acted;
        joined.objects[0].common.container.keywords = {"ab"};
        Page split = acted;
        split.objects[0].common.container.keywords = {"a", "b"};
        QVERIFY(fingerprint(joined) != fingerprint(split));

        // Le cadre web est haché par ses trois champs.
        Page framed = richPage();
        framed.objects.push_back(PageObject{WebFrame{}});
        const auto framedPrint = fingerprint(framed);
        std::get<WebFrame>(framed.objects.back()).html = "<p>a</p>";
        QVERIFY(fingerprint(framed) != framedPrint);
        std::get<WebFrame>(framed.objects.back()).html.clear();
        std::get<WebFrame>(framed.objects.back()).address = "https://exemple.org";
        QVERIFY(fingerprint(framed) != framedPrint);
        std::get<WebFrame>(framed.objects.back()).address.clear();
        std::get<WebFrame>(framed.objects.back()).bounds = RectF{0, 0, 10, 10};
        QVERIFY(fingerprint(framed) != framedPrint);

        // Règles de diffusion : chacune hors défaut compte, au niveau du document.
        const auto documentDiffers = [kRichDocument](auto mutate) {
            Document d(std::vector<Page>{richPage()});
            BroadcastRules rules;
            mutate(rules);
            d.setBroadcast(rules);
            return fingerprint(d) != kRichDocument;
        };
        QVERIFY(documentDiffers([](BroadcastRules& b) { b.noCopy = true; }));
        QVERIFY(documentDiffers([](BroadcastRules& b) { b.noSave = true; }));
        QVERIFY(documentDiffers([](BroadcastRules& b) { b.noPrint = true; }));
        QVERIFY(documentDiffers([](BroadcastRules& b) { b.noCamera = true; }));
        QVERIFY(documentDiffers([](BroadcastRules& b) { b.author = "Jeanne"; }));
    }

    void lot6FieldsKeepOldFingerprints()
    {
        QCOMPARE(fingerprint(samplePage()), std::uint64_t{0x29E4AA55B1A00D2Eull}); // figée avant L6-T10
        Page page = samplePage();
        page.objects[0].common.label = "Soleil";
        page.notes = "notes";
        const auto before = fingerprint(page);
        page.description.clear();
        page.keywords.clear();
        page.objects[0].common.keywords.clear();
        page.objects[0].common.labelStyle = LabelStyle{};
        QCOMPARE(fingerprint(page), before);

        // Suite du flux : octet de marque, description, mots-clés de la page, objets qui en portent.
        Page described = page;
        described.description = "Système solaire";
        FnvTail tail{before};
        tail.byte(0x6A);
        tail.text("Système solaire");
        tail.u64(0); // mots-clés de la page
        tail.u64(0); // objets porteurs
        QCOMPARE(fingerprint(described), tail.hash);

        Page tagged = page;
        tagged.objects[1].common.keywords = {"soleil", "lune"};
        FnvTail objectTail{before};
        objectTail.byte(0x6A);
        objectTail.text("");
        objectTail.u64(0);
        objectTail.u64(1); // un objet porteur
        objectTail.u64(1); // son rang
        objectTail.u64(2);
        objectTail.text("soleil");
        objectTail.text("lune");
        objectTail.text(""); // style par défaut : police, taille, couleur, contour, couleur du contour, fond
        objectTail.f64(12);
        objectTail.u32(0xFF000000u);
        objectTail.byte(0);
        objectTail.u32(0xFF000000u);
        objectTail.u32(0x00000000u);
        QCOMPARE(fingerprint(tagged), objectTail.hash);

        // Chaque champ compte, et les frontières entre mots-clés aussi.
        const auto differs = [&page, before](auto mutate) {
            Page p = page;
            mutate(p);
            return fingerprint(p) != before;
        };
        QVERIFY(differs([](Page& p) { p.keywords = {"soleil"}; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.labelStyle.pointSize = 18; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.labelStyle.fontFamily = "Arial"; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.labelStyle.color = 0xFFFF0000u; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.labelStyle.outline = PenStyle::Solid; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.labelStyle.outlineColor = 0xFF00FF00u; }));
        QVERIFY(differs([](Page& p) { p.objects[0].common.labelStyle.fill = 0xFFFFFFFFu; }));
        Page joined = page;
        joined.keywords = {"ab"};
        Page split = page;
        split.keywords = {"a", "b"};
        QVERIFY(fingerprint(joined) != fingerprint(split));
        Page asDescription = page;
        asDescription.description = "lune";
        Page asKeyword = page;
        asKeyword.keywords = {"lune"};
        QVERIFY(fingerprint(asDescription) != fingerprint(asKeyword));
        Page onFirst = page;
        onFirst.objects[0].common.keywords = {"lune"};
        Page onSecond = page;
        onSecond.objects[1].common.keywords = {"lune"};
        QVERIFY(fingerprint(onFirst) != fingerprint(onSecond)); // l'objet qui porte le mot-clé compte
    }

private:
    // Suite d'un flux FNV-1a 64 bits, codé comme model.cpp : entiers petit-boutistes, réels par leur motif
    // IEEE 754, textes par leurs octets UTF-8 précédés de leur longueur.
    struct FnvTail
    {
        std::uint64_t hash;
        void byte(std::uint8_t b)
        {
            hash ^= b;
            hash *= 0x100000001B3ull;
        }
        void u32(std::uint32_t v)
        {
            for (unsigned i = 0; i < 4; ++i) {
                byte(static_cast<std::uint8_t>((v >> (8u * i)) & 0xFFu));
            }
        }
        void u64(std::uint64_t v)
        {
            for (unsigned i = 0; i < 8; ++i) {
                byte(static_cast<std::uint8_t>((v >> (8u * i)) & 0xFFu));
            }
        }
        void f64(double v) { u64(std::bit_cast<std::uint64_t>(v)); }
        void text(const std::string& s)
        {
            u64(static_cast<std::uint64_t>(s.size()));
            for (const char c : s) {
                byte(static_cast<std::uint8_t>(c));
            }
        }
    };
};

QTEST_GUILESS_MAIN(TstCoreModel)
#include "tst_core_model.moc"
