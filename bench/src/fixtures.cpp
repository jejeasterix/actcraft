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

QStringList fixtureNames()
{
    return {QString(kThreePages)};
}

bool buildFixture(const QString& name, const QString& path, QString& error)
{
    core::Document document;
    if (name == kThreePages) {
        document = threePages();
    } else {
        error = QStringLiteral("fixture inconnue « %1 »").arg(name);
        return false;
    }
    const QByteArray archive = formats::encodeActcraft(document, {}, &error);
    return !archive.isEmpty() && formats::writeUrl(QUrl::fromLocalFile(path), archive, &error);
}

} // namespace actcraft::bench
