#include "equationlayout.h"

#include <actcraft/core/equation.h>
#include <actcraft/core/model.h>
#include <actcraft/render/equationpaint.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/render/paintedrenderer.h>

#include <QColor>
#include <QHash>
#include <QImage>
#include <QPainter>
#include <QQuickWindow>
#include <QSemaphore>
#include <QTest>
#include <QThread>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace actcraft;
using namespace Qt::StringLiterals;

namespace {

const render::ImageLookup kNoImages = [](const std::string&) { return QImage(); };

constexpr double kPoints = 24;

// paintFromTwoThreads : textes distincts, plus que les 64 entrées du cache, et dessins par fil.
constexpr std::size_t kThreadTexts = 80;
constexpr int kThreadRounds = 200;

std::optional<core::EquationNode> tree(const std::string& mathml)
{
    QString error;
    return core::parseMathml(mathml, error);
}

core::Equation equation(const std::string& mathml, core::RectF bounds = {20, 10, 360, 180})
{
    core::Equation e;
    e.bounds = bounds;
    e.mathml = mathml;
    e.pointSize = kPoints;
    return e;
}

QImage blank(int width = 400, int height = 200)
{
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    return image;
}

// paintEquation sur une image blanche, avec le lissage du peintre partagé (paintObjectLocal).
QImage painted(const core::Equation& e, render::EquationPaintMode mode)
{
    QImage image = blank();
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    render::paintEquation(p, e, mode);
    p.end();
    return image;
}

QImage pageOf(const core::Equation& e)
{
    core::Page page;
    page.objects.push_back(core::PageObject(e));
    return render::renderPage(page, kNoImages, 1.0).copy(0, 0, 400, 200);
}

// Empreinte des pixels d'une image : deux fils comparent leurs dessins sans garder 80 images chacun.
std::size_t hashOf(const QImage& image)
{
    return qHashBits(image.constBits(), std::size_t(image.sizeInBytes()));
}

bool isWhite(QColor c)
{
    return c.red() >= 250 && c.green() >= 250 && c.blue() >= 250;
}

bool isDark(QColor c)
{
    return c.red() + c.green() + c.blue() < 3 * 110;
}

// Surlignage du curseur : un bleu translucide sur le blanc.
bool isBluish(QColor c)
{
    return c.blue() - c.red() > 30;
}

// Pointillé gris de la case vide.
bool isGrey(QColor c)
{
    return !isWhite(c) && qAbs(c.red() - c.blue()) <= 10 && qAbs(c.green() - c.blue()) <= 10;
}

// Lignes qui portent au moins un pixel non blanc : première et dernière (-1 si aucune), et vrai si elles se
// suivent sans trou.
struct InkedRows
{
    int first = -1;
    int last = -1;
    bool contiguous = true;
};

InkedRows inkedRows(const QImage& image)
{
    InkedRows rows;
    int previous = -1;
    for (int y = 0; y < image.height(); ++y) {
        bool inked = false;
        for (int x = 0; x < image.width() && !inked; ++x) {
            inked = !isWhite(image.pixelColor(x, y));
        }
        if (!inked) {
            continue;
        }
        if (rows.first < 0) {
            rows.first = y;
        } else if (y != previous + 1) {
            rows.contiguous = false;
        }
        rows.last = y;
        previous = y;
    }
    return rows;
}

int count(const QImage& image, int fromRow, int toRow, bool (*match)(QColor))
{
    int n = 0;
    for (int y = std::max(0, fromRow); y < std::min(image.height(), toRow); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            n += match(image.pixelColor(x, y)) ? 1 : 0;
        }
    }
    return n;
}

// Pixels sombres à l'intérieur de la boîte (bord exclu : le repère neutre n'y compte pas).
int darkInside(const QImage& image, const core::RectF& b)
{
    int n = 0;
    for (int y = int(b.y) + 3; y < int(b.y + b.height) - 3; ++y) {
        for (int x = int(b.x) + 3; x < int(b.x + b.width) - 3; ++x) {
            n += isDark(image.pixelColor(x, y)) ? 1 : 0;
        }
    }
    return n;
}

} // namespace

class TstRenderEquation : public QObject
{
    Q_OBJECT

private slots:
    // Numérateur et dénominateur empilés, réduits à 80 % : la fraction est plus haute que 1,8 chiffre.
    void fractionIsTallerThanItsParts()
    {
        const QFont font = render::equationFont(kPoints);
        const auto digit = tree("<math><mn>2</mn></math>");
        const auto fraction = tree("<math><mfrac><mn>1</mn><mn>2</mn></mfrac></math>");
        QVERIFY(digit && fraction);
        const render::EquationBox d = render::layoutEquation(*digit, font);
        const render::EquationBox f = render::layoutEquation(*fraction, font);
        QVERIFY(d.ascent + d.descent > 0);
        QVERIFY2(
            f.ascent + f.descent > 1.8 * (d.ascent + d.descent),
            qPrintable(QStringLiteral("%1 contre %2").arg(f.ascent + f.descent).arg(d.ascent + d.descent)));
    }

    // Ligne de base de l'exposant au-dessus de celle de sa base (y vers le bas).
    void superscriptIsRaised()
    {
        const QFont font = render::equationFont(kPoints);
        const auto power = tree("<math><msup><mi>x</mi><mn>2</mn></msup></math>");
        QVERIFY(power);
        const render::EquationBox root = render::layoutEquation(*power, font);
        QCOMPARE(root.children.size(), std::size_t(1));
        const render::EquationBox& sup = root.children.front();
        QCOMPARE(sup.children.size(), std::size_t(2));
        const double em = font.pixelSize();
        QVERIFY2(
            sup.children[1].baseline < sup.children[0].baseline - 0.25 * em,
            qPrintable(
                QStringLiteral("%1 contre %2").arg(sup.children[1].baseline).arg(sup.children[0].baseline)));
        QVERIFY(sup.children[1].ascent < sup.children[0].ascent); // réduit
    }

    // Le signe de la racine s'ajoute au contenu (plus d'un demi-cadratin) et sa barre passe au-dessus.
    void rootCoversItsContent()
    {
        const QFont font = render::equationFont(kPoints);
        const auto root = tree("<math><msqrt><mi>x</mi><mo>+</mo><mn>1</mn></msqrt></math>");
        const auto content = tree("<math><mrow><mi>x</mi><mo>+</mo><mn>1</mn></mrow></math>");
        QVERIFY(root && content);
        const render::EquationBox r = render::layoutEquation(*root, font);
        const render::EquationBox c = render::layoutEquation(*content, font);
        const double em = font.pixelSize();
        QVERIFY(c.width > 0);
        QVERIFY2(r.width > c.width + 0.5 * em,
                 qPrintable(QStringLiteral("%1 contre %2").arg(r.width).arg(c.width)));
        QVERIFY(r.ascent > c.ascent);
    }

    // Une équation illisible (XML mal formé, arité fausse), ou lisible mais faite seulement d'éléments
    // inconnus de cette version (rien à mettre en page), garde le repère neutre : même image que paintUnknown
    // sur la même boîte, et rien de plus en mode Editor. Une équation lisible sans encre ni élément inconnu,
    // elle, ne dessine rien en mode Page : le repère neutre ne remplace pas toute équation sans encre.
    void malformedDrawsTheNeutralFrame()
    {
        const core::RectF box{40, 30, 300, 120};
        core::UnknownObject u;
        u.type = "equation";
        u.bounds = box;
        u.hasBounds = true;
        QImage unknown = blank();
        {
            QPainter p(&unknown);
            render::paintObjectLocal(p, core::PageObject(u), kNoImages);
        }
        QVERIFY(unknown != blank());
        const std::string unknownOnly =
            "<math><mstyle displaystyle=\"true\"><mfrac><mn>1</mn><mn>2</mn></mfrac></mstyle></math>";
        QVERIFY(tree(unknownOnly)); // lisible : mstyle est gardé tel quel, élément inconnu
        for (const std::string& mathml :
             {std::string("<math><mfrac><mn>1</mn></math>"),
              std::string("<math><mfrac><mn>1</mn></mfrac></math>"), std::string(), unknownOnly}) {
            const core::Equation e = equation(mathml, box);
            QImage local = blank();
            {
                QPainter p(&local);
                render::paintObjectLocal(p, core::PageObject(e), kNoImages);
            }
            QVERIFY2(local == unknown, mathml.c_str());
            QVERIFY2(painted(e, render::EquationPaintMode::Editor) ==
                         painted(e, render::EquationPaintMode::Page),
                     mathml.c_str());
        }
        const core::Equation empty = equation("<math><msup><mrow/><mrow/></msup></math>", box);
        QVERIFY(tree(empty.mathml));
        QCOMPARE(painted(empty, render::EquationPaintMode::Page), blank());
    }

    // La barre d'une fraction : une ligne sombre continue dans le tiers du milieu de la hauteur dessinée, sur
    // au moins 80 % de la largeur dessinée (numérateur plus large que le dénominateur).
    void fractionBarIsDrawn()
    {
        const core::Equation e =
            equation("<math><mfrac><mrow><mi>x</mi><mo>+</mo><mn>1</mn></mrow><mn>2</mn></mfrac></math>");
        const QImage image = painted(e, render::EquationPaintMode::Page);
        const InkedRows rows = inkedRows(image);
        QVERIFY(rows.first >= 0);
        int left = image.width();
        int right = -1;
        for (int y = rows.first; y <= rows.last; ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (!isWhite(image.pixelColor(x, y))) {
                    left = std::min(left, x);
                    right = std::max(right, x);
                }
            }
        }
        const int width = right - left + 1;
        const int third = (rows.last - rows.first + 1) / 3;
        int longest = 0;
        for (int y = rows.first + third; y <= rows.last - third; ++y) {
            int run = 0;
            for (int x = left; x <= right; ++x) {
                run = isDark(image.pixelColor(x, y)) ? run + 1 : 0;
                longest = std::max(longest, run);
            }
        }
        QVERIFY2(longest >= 0.8 * width, qPrintable(QStringLiteral("%1 sur %2").arg(longest).arg(width)));
    }

    // Les deux chemins du peintre partagé (page logicielle, texture de la page fixe) donnent la même image à
    // 2 niveaux près ; une ligne par forme de L7-T24 et quelques équations remplies.
    void paintedMatchesSoftware_data()
    {
        QTest::addColumn<QString>("mathml");
        QTest::addColumn<bool>("inked");
        QTest::newRow("fraction") << u"<math><mfrac><mrow/><mrow/></mfrac></math>"_s << true;
        QTest::newRow("power") << u"<math><msup><mrow/><mrow/></msup></math>"_s << false;
        QTest::newRow("subscript") << u"<math><msub><mrow/><mrow/></msub></math>"_s << false;
        QTest::newRow("square-root") << u"<math><msqrt><mrow/></msqrt></math>"_s << true;
        QTest::newRow("nth-root") << u"<math><mroot><mrow/><mrow/></mroot></math>"_s << true;
        QTest::newRow("parentheses") << u"<math><mrow><mo>(</mo><mrow/><mo>)</mo></mrow></math>"_s << true;
        QTest::newRow("sum") << u"<math><munderover><mo>∑</mo><mrow/><mrow/></munderover><mrow/></math>"_s
                             << true;
        QTest::newRow("integral") << u"<math><msubsup><mo>∫</mo><mrow/><mrow/></msubsup><mrow/></math>"_s
                                  << true;
        QTest::newRow("matrix-2x2")
            << u"<math><mrow><mo>(</mo><mtable><mtr><mtd/><mtd/></mtr><mtr><mtd/><mtd/></mtr></mtable><mo>)</mo></mrow></math>"_s
            << true;
        QTest::newRow("greek") << u"<math><mi>α</mi><mi>ω</mi><mi>Δ</mi><mi>Ω</mi></math>"_s << true;
        QTest::newRow("absolute-value") << u"<math><mrow><mo>|</mo><mrow/><mo>|</mo></mrow></math>"_s << true;
        QTest::newRow("matrix-1x2")
            << u"<math><mrow><mo>(</mo><mtable><mtr><mtd/><mtd/></mtr></mtable><mo>)</mo></mrow></math>"_s
            << true;
        QTest::newRow("matrix-2x1")
            << u"<math><mrow><mo>(</mo><mtable><mtr><mtd/></mtr><mtr><mtd/></mtr></mtable><mo>)</mo></mrow></math>"_s
            << true;
        QTest::newRow("matrix-3x3")
            << u"<math><mrow><mo>(</mo><mtable><mtr><mtd/><mtd/><mtd/></mtr><mtr><mtd/><mtd/>"
               u"<mtd/></mtr><mtr><mtd/><mtd/><mtd/></mtr></mtable><mo>)</mo></mrow></math>"_s
            << true;
        QTest::newRow("double-integral") << u"<math><msub><mo>∬</mo><mrow/></msub><mrow/></math>"_s << true;
        QTest::newRow("triple-integral") << u"<math><msub><mo>∭</mo><mrow/></msub><mrow/></math>"_s << true;
        QTest::newRow("trig-function")
            << u"<math><mrow><mi>sin</mi><mo>(</mo><mrow/><mo>)</mo></mrow></math>"_s << true;
        QTest::newRow("filled-fraction")
            << u"<math><mfrac><mrow><mi>x</mi><mo>+</mo><mn>1</mn></mrow><mn>2</mn></mfrac></math>"_s << true;
        QTest::newRow("filled-scripts")
            << u"<math><msubsup><mi>a</mi><mi>n</mi><mn>2</mn></msubsup><mo>=</mo><msup><mi>x</mi><mn>3</mn></msup></math>"_s
            << true;
        QTest::newRow("filled-root")
            << u"<math><mroot><mrow><mi>x</mi><mo>−</mo><mn>1</mn></mrow><mn>3</mn></mroot></math>"_s << true;
        QTest::newRow("filled-sum")
            << u"<math><munderover><mo>∑</mo><mrow><mi>i</mi><mo>=</mo><mn>1</mn></mrow><mi>n</mi></munderover><mi>i</mi></math>"_s
            << true;
        QTest::newRow("filled-matrix")
            << u"<math><mrow><mo>(</mo><mtable><mtr><mtd><mn>1</mn></mtd><mtd><mn>0</mn></mtd></"
               u"mtr><mtr><mtd><mn>0</mn>"
               u"</mtd><mtd><mn>1</mn></mtd></mtr></mtable><mo>)</mo></mrow></math>"_s
            << true;
        QTest::newRow("text-and-over")
            << u"<math><mover><mi>u</mi><mo>→</mo></mover><mtext> aire </mtext><munder><mi>lim</mi><mi>n</mi></munder></math>"_s
            << true;
        QTest::newRow("unknown-element") << u"<math><mi>x</mi><mfoo>?</mfoo></math>"_s << true;
    }
    void paintedMatchesSoftware()
    {
        QFETCH(QString, mathml);
        QFETCH(bool, inked);
        const core::Equation e = equation(mathml.toStdString());
        QVERIFY(tree(e.mathml));
        const QImage software = pageOf(e);
        QRectF rect;
        const QImage texture = render::rasterizeLocal(core::PageObject(e), kNoImages, 1.0, &rect);
        QImage composed = blank();
        {
            QPainter p(&composed);
            p.drawImage(rect.topLeft(), texture);
        }
        int worst = 0;
        for (int y = 0; y < software.height(); ++y) {
            for (int x = 0; x < software.width(); ++x) {
                const QColor a = software.pixelColor(x, y);
                const QColor b = composed.pixelColor(x, y);
                worst = std::max(
                    {worst, qAbs(a.red() - b.red()), qAbs(a.green() - b.green()), qAbs(a.blue() - b.blue())});
            }
        }
        QVERIFY2(worst <= 2, qPrintable(QString::number(worst)));
        if (inked) {
            QVERIFY(darkInside(software, e.bounds) > 0);
        }
    }

    // Le peintre partagé sert aussi les fils des miniatures et des exports : deux fils, partis ensemble,
    // dessinent 200 fois chacun, tour à tour, plus d'équations distinctes que le cache n'en garde (64) :
    // chaque dessin relit, remet en page et écrit le cache de son fil. Images identiques à celles du fil
    // principal.
    void paintFromTwoThreads()
    {
        std::vector<core::Equation> equations;
        std::vector<std::size_t> reference;
        for (std::size_t k = 0; k < kThreadTexts; ++k) {
            equations.push_back(
                equation("<math><mfrac><mrow><mi>x</mi><mo>+</mo><mn>" + std::to_string(k) +
                         "</mn></mrow><msqrt><msup><mi>y</mi><mn>2</mn></msup></msqrt></mfrac>"
                         "<mo>=</mo><mi>k</mi></math>"));
            const QImage image = painted(equations.back(), render::EquationPaintMode::Page);
            QVERIFY(darkInside(image, equations.back().bounds) > 0);
            reference.push_back(hashOf(image));
        }
        std::atomic<int> drawn{0};
        std::atomic<int> mismatches{0};
        std::atomic<bool> stop{false};
        QSemaphore start;
        auto work = [&](int index) {
            start.acquire();
            for (int i = 0; i < kThreadRounds && !stop; ++i) {
                const std::size_t k = (std::size_t(i) + std::size_t(index) * kThreadTexts / 2) % kThreadTexts;
                if (hashOf(painted(equations[k], render::EquationPaintMode::Page)) != reference[k]) {
                    ++mismatches;
                }
                ++drawn;
            }
        };
        std::unique_ptr<QThread> a(QThread::create(work, 0));
        std::unique_ptr<QThread> b(QThread::create(work, 1));
        a->start();
        b->start();
        start.release(2);
        const bool inTime = a->wait(120000) && b->wait(120000);
        if (!inTime) {
            // Détruire un QThread encore actif arrête le programme (qFatal) : les fils sortent d'abord de
            // leur boucle, puis le test échoue proprement.
            stop = true;
            a->wait();
            b->wait();
        }
        QVERIFY2(inTime, "les deux fils n'ont pas fini en 120 s");
        QCOMPARE(drawn.load(), 2 * kThreadRounds);
        QCOMPARE(mismatches.load(), 0);
    }

    // Page fixe hors écran (FixedPageItem, moteur peint) avec une équation : elle se dessine dans sa boîte
    // dès cette tâche.
    void fixedPageDrawsTheEquation()
    {
        QQuickWindow window;
        window.setColor(QColor(0x3c, 0x3f, 0x45));
        window.resize(400, 200);
        auto* item = new render::FixedPageItem;
        item->setSize(QSizeF(400, 200));
        item->setParentItem(window.contentItem());
        const core::Equation e = equation("<math><mfrac><mn>1</mn><mn>2</mn></mfrac></math>");
        core::Page page;
        page.width = 400;
        page.height = 200;
        page.objects.push_back(core::PageObject(e));
        item->setPage(std::move(page));
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QTRY_VERIFY(darkInside(window.grabWindow(), e.bounds) > 0);
    }

    // En mode Editor, la case marquée actcraft-placeholder se dessine en pointillé et celle marquée
    // actcraft-cursor surlignée ; en mode Page, ni l'un ni l'autre.
    void editorModeDrawsPlaceholderAndCursor()
    {
        const core::Equation e =
            equation("<math><mfrac><mrow class=\"actcraft-placeholder actcraft-cursor\"/>"
                     "<mrow class=\"actcraft-placeholder\"/></mfrac></math>");
        const QImage page = painted(e, render::EquationPaintMode::Page);
        const QImage editor = painted(e, render::EquationPaintMode::Editor);
        const InkedRows bar = inkedRows(page);
        QVERIFY(bar.first >= 0);
        QCOMPARE(count(page, 0, page.height(), isBluish), 0);
        QVERIFY(count(editor, 0, bar.first - 1, isBluish) > 0);              // numérateur : le curseur
        QCOMPARE(count(editor, bar.last + 2, editor.height(), isBluish), 0); // dénominateur : pas de curseur
        QVERIFY(count(editor, bar.last + 2, editor.height(), isGrey) > 0);   // mais son pointillé
    }

    // Rendu de page de <mfrac><mrow/><mrow/></mfrac>, avec et sans marques : rien hors de la barre de
    // fraction.
    void pageDrawsNoPlaceholder()
    {
        const core::Equation plain = equation("<math><mfrac><mrow/><mrow/></mfrac></math>");
        const core::Equation marked =
            equation("<math><mfrac><mrow class=\"actcraft-placeholder\"/>"
                     "<mrow class=\"actcraft-placeholder actcraft-cursor\"/></mfrac></math>");
        const QImage plainPage = pageOf(plain);
        const QImage markedPage = pageOf(marked);
        QCOMPARE(markedPage, plainPage);
        const InkedRows bar = inkedRows(plainPage);
        QVERIFY(bar.first >= 0); // la barre
        QVERIFY(bar.contiguous);
        QVERIFY2(bar.last - bar.first + 1 <= int(0.1 * plain.bounds.height),
                 qPrintable(QStringLiteral("%1 à %2").arg(bar.first).arg(bar.last)));
    }
};

QTEST_MAIN(TstRenderEquation)
#include "tst_render_equation.moc"
