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
// S4 en CI (L2-T21) : paperboard .flipchart de notre écrivain de tests (deux pages, fond dégradé, forme
// verte, fond image rouge en page 2), écrit octet pour octet. Jamais réencodé en .actcraft : il doit passer
// par le lecteur .flipchart à l'ouverture (document sans fichier, avertissements de lecture).
constexpr QLatin1StringView kSyntheticFlipchart("flipchart-synthetique");

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

bool buildSyntheticFlipchart(const QString& path, QString& error)
{
    return formats::writeUrl(QUrl::fromLocalFile(path), fctest::sampleFlipchart(), &error);
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
