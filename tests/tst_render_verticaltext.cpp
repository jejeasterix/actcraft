// Texte de haut en bas (L4-T24, H4-3, D9) : une colonne d'un caractère, chaque lettre sous la précédente,
// mise en forme gardée, et le même dessin dans le peintre partagé (renderPage) et dans la texture du rendu
// (rasterizeLocal), qui l'appelle.
#include <actcraft/core/model.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/render/paintedrenderer.h>

#include <QFont>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QtTest>

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

using namespace actcraft;

namespace {

constexpr const char* kAbc = "<p style=\"font-size:40pt\">ABC</p>";
constexpr const char* kNarrow = "<p style=\"font-size:40pt\">Wlll</p>";
constexpr const char* kBoldAbc = "<p style=\"font-size:40pt\"><b>ABC</b></p>";
constexpr int kInkLevel = 160; // gris plus sombre que ce niveau : encre sur le blanc de la page

core::PageObject verticalText(const char* html, double x, double y, double width, double height)
{
    core::PageObject object{core::Text{}};
    auto& text = std::get<core::Text>(core::body(object));
    text.html = html;
    text.vertical = true;
    text.bounds.x = x;
    text.bounds.y = y;
    text.bounds.width = width;
    text.bounds.height = height;
    return object;
}

core::Page pageWith(const core::PageObject& object)
{
    core::Page page;
    page.width = 400;
    page.height = 600;
    page.objects = {object};
    return page;
}

QImage painted(const core::Page& page)
{
    return render::renderPage(page, render::ImageLookup{}, 1.0, render::PaintOptions{});
}

// Lignes d'encre contiguës : une bande par lettre empilée.
struct Band
{
    int top = 0;
    int bottom = 0;
    int left = 0;
    int right = 0;
};

struct Ink
{
    int pixels = 0;
    QRect box;
    std::vector<Band> bands;
};

Ink inkOf(const QImage& image)
{
    Ink ink;
    bool inBand = false;
    for (int y = 0; y < image.height(); ++y) {
        int left = image.width();
        int right = -1;
        for (int x = 0; x < image.width(); ++x) {
            if (qGray(image.pixel(x, y)) < kInkLevel) {
                ++ink.pixels;
                left = std::min(left, x);
                right = std::max(right, x);
            }
        }
        if (right < 0) {
            inBand = false;
            continue;
        }
        ink.box = ink.box.united(QRect(left, y, right - left + 1, 1));
        if (!inBand) {
            Band band;
            band.top = y;
            band.left = left;
            band.right = right;
            ink.bands.push_back(band);
            inBand = true;
        }
        Band& band = ink.bands.back();
        band.bottom = y;
        band.left = std::min(band.left, left);
        band.right = std::max(band.right, right);
    }
    return ink;
}

// Plus grande avance des lettres de « ABC » à 40 points, police par défaut du document.
double widestLetter(const QImage& device)
{
    QFont font = QGuiApplication::font();
    font.setPointSizeF(40);
    const QFontMetricsF metrics(font, &device);
    return std::max(
        {metrics.horizontalAdvance(u'A'), metrics.horizontalAdvance(u'B'), metrics.horizontalAdvance(u'C')});
}

QString describe(const Ink& ink)
{
    QString out = QStringLiteral("%1 bande(s), encre %2 × %3 :")
                      .arg(ink.bands.size())
                      .arg(ink.box.width())
                      .arg(ink.box.height());
    for (const Band& b : ink.bands) {
        out += QStringLiteral(" [y %1-%2, x %3-%4]").arg(b.top).arg(b.bottom).arg(b.left).arg(b.right);
    }
    return out;
}

} // namespace

class TstRenderVerticalText : public QObject
{
    Q_OBJECT

private slots:
    // « ABC » à 40 points dans un cadre large : trois bandes d'encre disjointes, l'une sous l'autre et
    // centrées l'une sur l'autre, dans une colonne de moins de 1,5 fois la largeur d'une lettre. Rouge si le
    // texte reste sur une ligne.
    void lettersStackOneUnderAnother()
    {
        const QImage image = painted(pageWith(verticalText(kAbc, 40, 30, 200, 300)));
        const Ink ink = inkOf(image);
        QVERIFY2(ink.bands.size() == 3, qPrintable(describe(ink)));
        const double letter = widestLetter(image);
        QVERIFY2(ink.box.width() < 1.5 * letter,
                 qPrintable(describe(ink) + QStringLiteral(" ; lettre de %1 px").arg(letter)));
        const double first = (ink.bands[0].left + ink.bands[0].right) / 2.0;
        for (const Band& b : ink.bands) {
            QVERIFY2(std::abs((b.left + b.right) / 2.0 - first) < letter / 2, qPrintable(describe(ink)));
        }
    }

    // Une lettre par ligne, quelle que soit sa largeur (L4-C12) : « Wlll » donne quatre bandes, alors que la
    // colonne a la largeur du « W » et que trois « l » étroits tiendraient ensemble sur une ligne. Rouge tant
    // que le remplissage glouton range les lettres étroites côte à côte.
    void narrowLettersGetOneLineEach()
    {
        const Ink ink = inkOf(painted(pageWith(verticalText(kNarrow, 40, 30, 200, 300))));
        QVERIFY2(ink.bands.size() == 4, qPrintable(describe(ink)));
    }

    // <b>ABC</b> : toujours en colonne, et plus d'encre qu'en maigre (la mise en forme du HTML est gardée).
    void boldIsKept()
    {
        const Ink plain = inkOf(painted(pageWith(verticalText(kAbc, 40, 30, 200, 300))));
        const Ink bold = inkOf(painted(pageWith(verticalText(kBoldAbc, 40, 30, 200, 300))));
        QVERIFY2(bold.bands.size() == 3, qPrintable(describe(bold)));
        QVERIFY2(bold.pixels > plain.pixels,
                 qPrintable(QStringLiteral("gras %1 px, maigre %2 px").arg(bold.pixels).arg(plain.pixels)));
    }

    // Discrimine le patron écarté. Cadre de 8 × 20, plus étroit et plus bas que la colonne : le peintre
    // dessine la colonne entière ; la texture doit couvrir cette même colonne, donc tenir son étendue de la
    // mise en page du peintre et non du seul cadre du modèle (ni d'une mise en page à elle). Rouge si la
    // texture rogne au cadre ou place la colonne autrement ; la comparaison ne vaut que si la colonne est
    // vraiment dessinée.
    void sameInPainterAndTexture()
    {
        const core::PageObject object = verticalText(kAbc, 60, 40, 8, 20);
        const QImage page = painted(pageWith(object));
        const Ink ink = inkOf(page);
        QVERIFY2(ink.bands.size() == 3, qPrintable(describe(ink)));

        QRectF rect;
        const QImage texture = render::rasterizeLocal(object, render::ImageLookup{}, 1.0, &rect);
        QVERIFY2(rect.contains(QRectF(ink.box)),
                 qPrintable(describe(ink) + QStringLiteral(" ; texture %1,%2 %3 × %4")
                                                .arg(rect.x())
                                                .arg(rect.y())
                                                .arg(rect.width())
                                                .arg(rect.height())));
        QImage composed(page.size(), QImage::Format_ARGB32_Premultiplied);
        composed.fill(Qt::white);
        {
            QPainter p(&composed);
            p.drawImage(rect, texture);
        }
        int worst = 0;
        for (int y = 0; y < page.height(); ++y) {
            for (int x = 0; x < page.width(); ++x) {
                const QRgb a = page.pixel(x, y);
                const QRgb b = composed.pixel(x, y);
                worst = std::max({worst, std::abs(qRed(a) - qRed(b)), std::abs(qGreen(a) - qGreen(b)),
                                  std::abs(qBlue(a) - qBlue(b))});
            }
        }
        QVERIFY2(worst <= 2, qPrintable(QStringLiteral("écart de %1 niveaux").arg(worst)));
    }
};

QTEST_MAIN(TstRenderVerticalText)
#include "tst_render_verticaltext.moc"
