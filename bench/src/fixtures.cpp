#include "flipchartwriter.h"

#include <actcraft/bench/fixtures.h>
#include <actcraft/core/document.h>
#include <actcraft/formats/nativefile.h>
#include <actcraft/formats/urlio.h>

#include <QBuffer>
#include <QImage>
#include <QPainter>
#include <QUrl>

#include <utility>
#include <vector>

namespace actcraft::bench {

namespace {

constexpr QLatin1StringView kThreePages("trois-pages");
// L10-T5 (S33) : page blanche à grille visible, lignes noires d'un pixel.
constexpr QLatin1StringView kGrid("grille");
// S4 en CI (L2-T21) : paperboard .flipchart de notre écrivain de tests (deux pages, fond dégradé, forme
// verte, fond image rouge en page 2), écrit octet pour octet. Jamais réencodé en .actcraft : il doit passer
// par le lecteur .flipchart à l'ouverture (document sans fichier, avertissements de lecture).
constexpr QLatin1StringView kSyntheticFlipchart("flipchart-synthetique");
// L3-T15 (S34, S35) : un rectangle gris et deux traits ; un rectangle libre et un rectangle verrouillé.
constexpr QLatin1StringView kRectangleAndStrokes("rectangle-et-traits");
constexpr QLatin1StringView kLockedObject("objet-verrouille");
// L8-T16 : un panier qui n'accepte que « fruit », renvoie le reste et sonne ; une pomme, une carotte.
constexpr QLatin1StringView kContainers("conteneurs");
// L8-T12 (S11.2) : deux pages ; un rond gris en page 1 (centre en 300, 300), un rectangle bleu en page 2
// (centre en 600, 300).
constexpr QLatin1StringView kActionBrowser("navigateur-actions");
// L8-T13 (S11.3) : une page, un rectangle bleu (centre en 750, 400) qui reçoit l'icône du stylo.
constexpr QLatin1StringView kDragDrop("glisser-deposer");
// L8-T19 (S11.1, S11.4, S11.7) : un rond rouge qui masque une étiquette, un panier « fruit », une carotte, un
// objet lié à https://example.org/, un média « lecture auto ».
constexpr QLatin1StringView kInteractivity("interactivite");
// L8-T32 (S18) : deux pages ; un rond gris en page 1 (centre en 300, 300) qui porte « page suivante » ; copie
// et impression interdites par les règles de diffusion du document.
constexpr QLatin1StringView kBroadcast("diffusion-protegee");

// PNG de 64 × 64 produit par Qt à chaque passage : moitié gauche rouge, moitié droite blanche.
QByteArray redAndWhitePng()
{
    QImage image(64, 64, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.fillRect(QRect(0, 0, 32, 64), Qt::red);
    painter.end();
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return png;
}

core::Document threePages()
{
    std::vector<core::Page> pages(3);

    core::Shape rect;
    rect.kind = core::ShapeKind::Rectangle;
    rect.bounds = {200, 200, 300, 200};
    rect.fill = 0xFF1E88E5u;
    core::PageObject shape = rect;
    shape.common.id = 1;
    pages[1].objects.push_back(shape);

    core::Text text;
    text.bounds = {600, 200, 300, 80};
    text.html = "<p style=\"font-size:48px; color:#000000\">Bonjour</p>";
    core::PageObject textObject = text;
    textObject.common.id = 2;
    pages[1].objects.push_back(textObject);

    const QByteArray png = redAndWhitePng();
    core::ImageObject image;
    image.bounds = {0, 0, 64, 64};
    image.sourceWidth = 64;
    image.sourceHeight = 64;
    image.resource = core::sha256Hex(png);
    core::PageObject imageObject = image;
    imageObject.common.id = 3;
    // Agrandie 4 fois puis posée en (300, 200) : moitié rouge de x = 300 à 428.
    imageObject.common.transform = core::compose(core::scaling(4, 4), core::translation(300, 200));
    pages[2].objects.push_back(imageObject);

    core::Document document(std::move(pages));
    document.addResource(png, "png");
    document.setTitle("Trois pages");
    return document;
}

} // namespace

namespace {

bool writeActcraft(const core::Document& document, const QString& path, QString& error)
{
    const QByteArray archive = formats::encodeActcraft(document, {}, &error);
    return !archive.isEmpty() && formats::writeUrl(QUrl::fromLocalFile(path), archive, &error);
}

bool buildThreePages(const QString& path, QString& error)
{
    return writeActcraft(threePages(), path, error);
}

// L10-T5 : une page blanche à grille visible (un niveau, pas de 32, lignes noires pleines), pour vérifier que
// le lissage des traits ne touche pas la grille (lignes d'un pixel, nettes, comme la référence).
bool buildGrid(const QString& path, QString& error)
{
    std::vector<core::Page> pages(1);
    core::GridLevel level;
    level.spacingX = 32;
    level.spacingY = 32;
    level.color = 0xFF000000u;
    pages[0].grid.visible = true;
    pages[0].grid.levels.push_back(level);
    core::Document document(std::move(pages));
    document.setTitle("Grille");
    return writeActcraft(document, path, error);
}

bool buildSyntheticFlipchart(const QString& path, QString& error)
{
    return formats::writeUrl(QUrl::fromLocalFile(path), fctest::sampleFlipchart(), &error);
}

core::PageObject grayRectangle(core::ObjectId id, core::Argb fill, double x, double y, bool locked)
{
    core::Shape rect;
    rect.kind = core::ShapeKind::Rectangle;
    rect.bounds = {x, y, 100, 100};
    rect.fill = fill;
    core::PageObject object = rect;
    object.common.id = id;
    object.common.layer = core::Layer::Middle;
    object.common.locked = locked;
    return object;
}

core::PageObject horizontalStroke(core::ObjectId id, double y)
{
    core::Stroke stroke;
    for (const double x : {300.0, 400.0, 500.0}) {
        stroke.points.push_back({x, y, 0.5f});
    }
    core::PageObject object = stroke;
    object.common.id = id;
    object.common.layer = core::Layer::Top;
    return object;
}

bool writeOnePage(std::vector<core::PageObject> objects, const char* title, const QString& path,
                  QString& error)
{
    std::vector<core::Page> pages(1);
    pages[0].objects = std::move(objects);
    core::Document document(std::move(pages));
    document.setTitle(title);
    const QByteArray archive = formats::encodeActcraft(document, {}, &error);
    return !archive.isEmpty() && formats::writeUrl(QUrl::fromLocalFile(path), archive, &error);
}

// Page 1024 × 768 : un rectangle plein gris #808080 (100, 100, 100, 100) sur la couche intermédiaire, deux
// traits sur la couche du dessus (y 300 et 400, x 300 à 500).
bool buildRectangleAndStrokes(const QString& path, QString& error)
{
    return writeOnePage(
        {grayRectangle(1, 0xFF808080u, 100, 100, false), horizontalStroke(2, 300), horizontalStroke(3, 400)},
        "Rectangle et traits", path, error);
}

// Un rectangle plein #808080 libre (100, 100) et un rectangle plein #404040 verrouillé (400, 100).
bool buildLockedObject(const QString& path, QString& error)
{
    return writeOnePage(
        {grayRectangle(1, 0xFF808080u, 100, 100, false), grayRectangle(2, 0xFF404040u, 400, 100, true)},
        "Objet verrouillé", path, error);
}

// Page 1024 × 768 : panier gris #808080 (500, 300), derrière ; pomme rouge « fruit » (100, 150) ; carotte
// orange « légume » (100, 450). Tous 100 × 100. Le panier accepte le mot-clé « fruit », renvoie ce qu'il
// refuse et sonne à l'acceptation.
bool buildContainers(const QString& path, QString& error)
{
    core::PageObject basket = grayRectangle(1, 0xFF808080u, 500, 300, false);
    basket.common.container.accepts = core::Accepts::Keywords;
    basket.common.container.keywords = {"fruit"};
    basket.common.container.bounceBack = true;
    basket.common.container.chime = true;
    core::PageObject apple = grayRectangle(2, 0xFFFF0000u, 100, 150, false);
    apple.common.keywords = {"fruit"};
    core::PageObject carrot = grayRectangle(3, 0xFFFF8000u, 100, 450, false);
    carrot.common.keywords = {"légume"};
    return writeOnePage({std::move(basket), std::move(apple), std::move(carrot)}, "Conteneurs", path, error);
}

// Page 1024 × 768, tous 100 × 100 : rond rouge (100, 100) qui porte « masquer ou afficher » vers l'étiquette
// bleue (600, 100) ; panier gris « fruit » (500, 300) qui renvoie ce qu'il refuse ; carotte orange « légume »
// (100, 450) ; objet vert lié à https://example.org/ (800, 500) ; média « lecture auto » (800, 100).
bool buildInteractivity(const QString& path, QString& error)
{
    core::Shape disc;
    disc.kind = core::ShapeKind::Ellipse;
    disc.bounds = {100, 100, 100, 100};
    disc.fill = 0xFFFF0000u;
    core::PageObject round = disc;
    round.common.id = 1;
    round.common.layer = core::Layer::Middle;
    core::Action toggle;
    toggle.catalogId = "object.toggle-hidden";
    toggle.target = 2;
    toggle.parameters = "{}";
    round.common.actions.push_back(toggle);

    core::PageObject label = grayRectangle(2, 0xFF1E88E5u, 600, 100, false);
    core::PageObject basket = grayRectangle(3, 0xFF808080u, 500, 300, false);
    basket.common.container.accepts = core::Accepts::Keywords;
    basket.common.container.keywords = {"fruit"};
    basket.common.container.bounceBack = true;
    core::PageObject carrot = grayRectangle(4, 0xFFFF8000u, 100, 450, false);
    carrot.common.keywords = {"légume"};

    core::PageObject linked = grayRectangle(5, 0xFF00A000u, 800, 500, false);
    core::Action open;
    open.catalogId = "document.open-website";
    open.parameters = R"({"url":"https://example.org/"})";
    linked.common.actions.push_back(open);

    core::Media media;
    media.bounds = {800, 100, 100, 100};
    media.playOnOpen = true;
    core::PageObject sound = media;
    sound.common.id = 6;
    sound.common.layer = core::Layer::Middle;

    return writeOnePage({std::move(round), std::move(label), std::move(basket), std::move(carrot),
                         std::move(linked), std::move(sound)},
                        "Interactivité", path, error);
}

bool buildActionBrowser(const QString& path, QString& error)
{
    std::vector<core::Page> pages(2);
    core::Shape disc;
    disc.kind = core::ShapeKind::Ellipse;
    disc.bounds = {250, 250, 100, 100};
    disc.fill = 0xFF808080u;
    core::PageObject round = disc;
    round.common.id = 1;
    round.common.layer = core::Layer::Middle;
    pages[0].objects.push_back(round);

    core::Shape rect;
    rect.kind = core::ShapeKind::Rectangle;
    rect.bounds = {550, 250, 100, 100};
    rect.fill = 0xFF1E88E5u;
    core::PageObject blue = rect;
    blue.common.id = 2;
    blue.common.layer = core::Layer::Middle;
    pages[1].objects.push_back(blue);

    core::Document document(std::move(pages));
    document.setTitle("Navigateur d'actions");
    return writeActcraft(document, path, error);
}

bool buildDragDrop(const QString& path, QString& error)
{
    std::vector<core::Page> pages(1);
    core::Shape rect;
    rect.kind = core::ShapeKind::Rectangle;
    rect.bounds = {600, 300, 300, 200};
    rect.fill = 0xFF1E88E5u;
    core::PageObject blue = rect;
    blue.common.id = 1;
    blue.common.layer = core::Layer::Middle;
    pages[0].objects.push_back(blue);

    core::Document document(std::move(pages));
    document.setTitle("Glisser-déposer");
    return writeActcraft(document, path, error);
}

bool buildBroadcast(const QString& path, QString& error)
{
    std::vector<core::Page> pages(2);
    core::Shape disc;
    disc.kind = core::ShapeKind::Ellipse;
    disc.bounds = {250, 250, 100, 100};
    disc.fill = 0xFF808080u;
    core::PageObject round = disc;
    round.common.id = 1;
    round.common.layer = core::Layer::Middle;
    core::Action next;
    next.catalogId = "page.next";
    next.parameters = "{}";
    round.common.actions.push_back(next);
    pages[0].objects.push_back(round);

    core::Document document(std::move(pages));
    document.setTitle("Diffusion protégée");
    core::BroadcastRules rules;
    rules.noCopy = true;
    rules.noPrint = true;
    document.setBroadcast(rules);
    return writeActcraft(document, path, error);
}

// Table des fixtures : nom, écriture du fichier (un seul endroit pour la liste et pour la construction).
struct FixtureEntry
{
    QLatin1StringView name;
    bool (*build)(const QString& path, QString& error);
};

constexpr FixtureEntry kFixtures[] = {
    {kThreePages, buildThreePages},
    {kSyntheticFlipchart, buildSyntheticFlipchart},
    {kGrid, buildGrid},
    {kRectangleAndStrokes, buildRectangleAndStrokes},
    {kLockedObject, buildLockedObject},
    {kContainers, buildContainers},
    {kActionBrowser, buildActionBrowser},
    {kDragDrop, buildDragDrop},
    {kInteractivity, buildInteractivity},
    {kBroadcast, buildBroadcast},
};

} // namespace

QStringList fixtureNames()
{
    QStringList names;
    for (const FixtureEntry& entry : kFixtures) {
        names << QString(entry.name);
    }
    return names;
}

bool buildFixture(const QString& name, const QString& path, QString& error)
{
    for (const FixtureEntry& entry : kFixtures) {
        if (name == entry.name) {
            return entry.build(path, error);
        }
    }
    error = QStringLiteral("fixture inconnue « %1 »").arg(name);
    return false;
}

} // namespace actcraft::bench
