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

bool buildThreePages(const QString& path, QString& error)
{
    const QByteArray archive = formats::encodeActcraft(threePages(), {}, &error);
    return !archive.isEmpty() && formats::writeUrl(QUrl::fromLocalFile(path), archive, &error);
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
    const QByteArray archive = formats::encodeActcraft(document, {}, &error);
    return !archive.isEmpty() && formats::writeUrl(QUrl::fromLocalFile(path), archive, &error);
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
