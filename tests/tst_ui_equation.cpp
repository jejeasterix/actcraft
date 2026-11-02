#include "equationcontroller.h"
#include "equationpreviewitem.h"
#include "presentationdocument.h"
#include "support/flipchartwriter.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/core/model.h>
#include <actcraft/render/equationpaint.h>
#include <actcraft/render/pagepainter.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>

#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPicture>
#include <QSignalSpy>
#include <QStyleHints>
#include <QTemporaryDir>
#include <QtTest>

#include <algorithm>
#include <cmath>
#include <functional>
#include <variant>

namespace {

using actcraft::ui::DocumentController;
using actcraft::ui::EquationController;
using actcraft::ui::EquationPreview;
using presentationharness::MainWindow;
using presentationharness::Pointer;
namespace core = actcraft::core;
using namespace Qt::StringLiterals;

DocumentController* controllerOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<DocumentController*>(u"documentController"_s) : nullptr;
}

EquationController* equationOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<EquationController*>(u"equation"_s) : nullptr;
}

const core::Page& shown(const DocumentController& controller)
{
    return controller.document().page(static_cast<std::size_t>(controller.currentIndex()));
}

QPointF centreOf(const QQuickItem* item)
{
    return item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
}

// Appui bref au stylet, comme Qt : l'événement de tablette part non accepté ; si la fenêtre ne l'accepte pas
// (pas de gestionnaire de tablette sous le point), Qt le rejoue en souris
// (Qt::AA_SynthesizeMouseForUnhandledTabletEvents, actif par défaut). Faux si la fenêtre a pris un événement
// : Qt ne rejouerait rien.
bool stylusTap(QQuickWindow* window, QPointF at)
{
    bool replayed = true;
    for (const QEvent::Type type : {QEvent::TabletPress, QEvent::TabletRelease}) {
        const bool press = type == QEvent::TabletPress;
        QTabletEvent event(type, presentationharness::stylusDevice(), at, window->mapToGlobal(at),
                           press ? 0.5 : 0.0, 0, 0, 0, 0, 0, Qt::NoModifier, Qt::LeftButton,
                           press ? Qt::LeftButton : Qt::NoButton);
        event.setAccepted(false); // comme QGuiApplication avant l'envoi
        QCoreApplication::sendEvent(window, &event);
        if (event.isAccepted()) {
            replayed = false;
            continue;
        }
        if (press) {
            QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, at.toPoint());
        } else {
            QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, at.toPoint());
        }
    }
    QCoreApplication::processEvents();
    return replayed;
}

// Ouvre l'éditeur par sa commande et le pose, son coin haut gauche sur le point de page (300, 20).
QQuickItem* openEditor(const MainWindow& main)
{
    if (!main.run(u"insert.equation"_s)) {
        return nullptr;
    }
    QQuickItem* panel = main.item(u"equationRoot"_s);
    QQuickItem* layer = main.item(u"presentationLayer"_s);
    if (!panel || !layer || !panel->isVisible()) {
        return nullptr;
    }
    panel->setPosition(layer->mapFromScene(main.pageView()->pageToScene(QPointF(300, 20))));
    return panel;
}

// Clic de souris sur un élément du panneau, cherché dans le panneau seul. Faux s'il manque.
bool clickIn(const MainWindow& main, QQuickItem* panel, const QString& name)
{
    QQuickItem* item = presentationharness::findItem(panel, name);
    if (!item || !item->isVisible()) {
        return false;
    }
    QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, centreOf(item).toPoint());
    QCoreApplication::processEvents();
    return true;
}

const core::Equation* lastEquation(const DocumentController& controller)
{
    const auto& objects = shown(controller).objects;
    return objects.empty() ? nullptr : std::get_if<core::Equation>(&core::body(objects.back()));
}

// Fraction 1/2 saisie par le contrôleur ; le curseur reste dans le dénominateur.
void typeOneHalf(EquationController& equation)
{
    equation.insertTemplate(u"fraction"_s);
    equation.type(u"1"_s);
    equation.moveRight();
    equation.type(u"2"_s);
}

QString oneHalf()
{
    return u"<math><mfrac><mn>1</mn><mn>2</mn></mfrac></math>"_s;
}

// Aperçu de taille `item`, posé en `at` sur une image blanche de taille `size`.
QImage previewImage(const QString& mathml, QSizeF item, QSize size, QPointF at)
{
    EquationPreview preview;
    preview.setSize(item);
    preview.setMathml(mathml);
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(at);
    preview.paint(&painter);
    painter.end();
    return image;
}

QImage previewImage(const QString& mathml, QSize size)
{
    return previewImage(mathml, QSizeF(size), size, QPointF());
}

// Grossissement des mesures : QPicture borne son dessin au pixel entier du périphérique, soit 0,01 px de
// page sous ×100.
constexpr double kMeasureZoom = 100;

// Taille de l'encre d'un dessin, en pixels de page.
QSizeF inkSize(const std::function<void(QPainter&)>& draw)
{
    QPicture picture;
    QPainter painter(&picture);
    painter.scale(kMeasureZoom, kMeasureZoom);
    draw(painter);
    painter.end();
    const QRect ink = picture.boundingRect();
    return QSizeF(ink.width() / kMeasureZoom, ink.height() / kMeasureZoom);
}

// La même équation sans bornes : paintEquation la dessine alors à l'échelle 1, le coin haut gauche de son
// cadre en (0, 0).
core::Equation atScaleOne(core::Equation equation)
{
    equation.bounds = core::RectF{0, 0, 0, 0};
    return equation;
}

// Plus grand écart d'une composante entre deux images de même taille.
int maxDifference(const QImage& a, const QImage& b)
{
    int worst = 0;
    for (int y = 0; y < a.height(); ++y) {
        for (int x = 0; x < a.width(); ++x) {
            const QRgb p = a.pixel(x, y);
            const QRgb q = b.pixel(x, y);
            worst = std::max({worst, std::abs(qRed(p) - qRed(q)), std::abs(qGreen(p) - qGreen(q)),
                              std::abs(qBlue(p) - qBlue(q)), std::abs(qAlpha(p) - qAlpha(q))});
        }
    }
    return worst;
}

QImage pageImage(const QString& mathml, QSize size)
{
    core::Equation equation;
    equation.bounds.x = 0;
    equation.bounds.y = 0;
    equation.bounds.width = size.width();
    equation.bounds.height = size.height();
    equation.mathml = mathml.toStdString();
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    actcraft::render::paintEquation(painter, equation, actcraft::render::EquationPaintMode::Page);
    painter.end();
    return image;
}

// Pose une équation déjà dans le document (couche intermédiaire), comme le ferait un fichier lu.
core::ObjectId addEquation(DocumentController& controller, const std::string& mathml)
{
    core::Equation equation;
    equation.bounds = core::RectF{100, 100, 160, 60};
    equation.mathml = mathml;
    core::PageObject object = std::move(equation);
    object.common.layer = core::Layer::Middle;
    return actcraft::ui::DocumentInsertions(controller).insertObject(std::move(object));
}

const core::Equation* equationById(const DocumentController& controller, core::ObjectId id)
{
    for (const core::PageObject& object : shown(controller).objects) {
        if (object.common.id == id) {
            return std::get_if<core::Equation>(&core::body(object));
        }
    }
    return nullptr;
}

// Ouvre l'éditeur sur l'équation `id` par sa commande ; le panneau, ou nul.
QQuickItem* editEquation(const MainWindow& main, core::ObjectId id)
{
    if (!main.run(u"equation.edit"_s, QVariant(static_cast<double>(id)))) {
        return nullptr;
    }
    QQuickItem* panel = main.item(u"equationRoot"_s);
    return panel && panel->isVisible() ? panel : nullptr;
}

bool saveTo(DocumentController& controller, const QUrl& url)
{
    QSignalSpy saved(&controller, &DocumentController::saveFinished);
    controller.saveToUrl(url);
    return saved.wait(60000) && saved.first().at(0).toBool();
}

bool openFrom(DocumentController& controller, const QUrl& url)
{
    QSignalSpy loaded(&controller, &DocumentController::loadFinished);
    controller.openUrl(url);
    return loaded.wait(60000) && loaded.first().at(0).toBool();
}

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
class TstUiEquation : public QObject
{
    Q_OBJECT

private slots:
    // « OK » pose l'équation sur la couche intermédiaire, au centre de la vue, en une étape d'annulation, et
    // ferme l'éditeur.
    void insertEquationIsUndoneAtOnce()
    {
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QQuickItem* panel = openEditor(main);
        QVERIFY(panel != nullptr);
        EquationController* equation = equationOf(main);
        QVERIFY(equation != nullptr);
        typeOneHalf(*equation);

        const std::size_t before = shown(*controller).objects.size();
        QVERIFY(clickIn(main, panel, u"equationOkButton"_s));
        QCOMPARE(shown(*controller).objects.size(), before + 1);
        const core::PageObject& written = shown(*controller).objects.back();
        const core::Equation* inserted = lastEquation(*controller);
        QVERIFY(inserted != nullptr);
        QVERIFY(written.common.layer == core::Layer::Middle);
        QCOMPARE(QString::fromStdString(inserted->mathml), oneHalf());
        QVERIFY(inserted->bounds.width > 0 && inserted->bounds.height > 0);
        actcraft::ui::PageView* view = main.pageView();
        const QPointF centre =
            view->sceneToPage(view->mapToScene(QPointF(view->width() / 2, view->height() / 2)));
        QVERIFY2(std::abs(inserted->bounds.x + inserted->bounds.width / 2 - centre.x()) < 1.0, "centre x");
        QVERIFY2(std::abs(inserted->bounds.y + inserted->bounds.height / 2 - centre.y()) < 1.0, "centre y");
        QTRY_VERIFY(!panel->isVisible());

        controller->undo();
        QCOMPARE(shown(*controller).objects.size(), before);
        QVERIFY(!controller->canUndo());
    }

    // Le curseur de l'éditeur est dans le dénominateur (marque présente dans le MathML de l'aperçu) ; l'objet
    // inséré n'en porte aucune, ni curseur ni case vide.
    void insertedEquationHasNoEditorMark()
    {
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QQuickItem* panel = openEditor(main);
        QVERIFY(panel != nullptr);
        EquationController* equation = equationOf(main);
        QVERIFY(equation != nullptr);
        typeOneHalf(*equation);
        QVERIFY(equation->editorMathml().contains(u"actcraft-cursor"_s));

        QVERIFY(clickIn(main, panel, u"equationOkButton"_s));
        const core::Equation* inserted = lastEquation(*controller);
        QVERIFY(inserted != nullptr);
        const QString written = QString::fromStdString(inserted->mathml);
        QVERIFY2(!written.contains(u"actcraft-cursor"_s), qPrintable(written));
        QVERIFY2(!written.contains(u"actcraft-placeholder"_s), qPrintable(written));
    }

    // Fraction vide, « OK » : rien n'est posé, l'éditeur reste ouvert et dit pourquoi ; le bouton reste
    // actif.
    void incompleteEquationIsRefused()
    {
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QQuickItem* panel = openEditor(main);
        QVERIFY(panel != nullptr);
        EquationController* equation = equationOf(main);
        QVERIFY(equation != nullptr);
        equation->insertTemplate(u"fraction"_s);

        const std::size_t before = shown(*controller).objects.size();
        QQuickItem* ok = presentationharness::findItem(panel, u"equationOkButton"_s);
        QVERIFY(ok != nullptr);
        QVERIFY(clickIn(main, panel, u"equationOkButton"_s));
        QCOMPARE(shown(*controller).objects.size(), before);
        QVERIFY(equation->hasPlaceholder());
        QVERIFY(panel->isVisible());
        QQuickItem* message = presentationharness::findItem(panel, u"equationMessage"_s);
        QVERIFY(message != nullptr);
        QTRY_VERIFY(message->isVisible());
        QVERIFY(!message->property("text").toString().isEmpty());
        QVERIFY(ok->isEnabled());
        QVERIFY(!controller->canUndo());
    }

    // Les touches du panneau (« 1 », flèche droite, « 2 ») donnent le même MathML que le clavier physique ;
    // le champ caché prend le focus au toucher de l'aperçu, et ce qu'il reçoit va dans la case.
    void editorHasItsOwnKeys()
    {
        MainWindow main;
        QQuickItem* panel = openEditor(main);
        QVERIFY(panel != nullptr);
        EquationController* equation = equationOf(main);
        QVERIFY(equation != nullptr);
        QVERIFY(clickIn(main, panel, u"equationTemplate_fraction"_s));
        QVERIFY(clickIn(main, panel, u"equationKey1"_s));
        QVERIFY(clickIn(main, panel, u"equationKeyRight"_s));
        QVERIFY(clickIn(main, panel, u"equationKey2"_s));
        QCOMPARE(equation->mathml(), oneHalf());

        panel = openEditor(main); // une nouvelle équation
        QVERIFY(panel != nullptr);
        QCOMPARE(equation->mathml(), u"<math><mrow class=\"actcraft-placeholder\"/></math>"_s);
        QVERIFY(clickIn(main, panel, u"equationTemplate_fraction"_s));
        QTest::keyClick(main.window(), Qt::Key_1);
        QTest::keyClick(main.window(), Qt::Key_Right);
        QTest::keyClick(main.window(), Qt::Key_2);
        QCOMPARE(equation->mathml(), oneHalf());

        QQuickItem* hidden = presentationharness::findItem(panel, u"equationHiddenInput"_s);
        QQuickItem* preview = presentationharness::findItem(panel, u"equationPreview"_s);
        QVERIFY(hidden != nullptr && preview != nullptr);
        QVERIFY(!hidden->hasActiveFocus());
        QPointingDevice* touch = QTest::createTouchDevice();
        const QPoint at = centreOf(preview).toPoint();
        QTest::touchEvent(main.window(), touch).press(0, at);
        QTest::touchEvent(main.window(), touch).release(0, at);
        QTRY_VERIFY(hidden->hasActiveFocus());
        QTest::keyClick(main.window(), Qt::Key_3); // le clavier du système écrit dans le champ caché
        QCOMPARE(equation->mathml(), u"<math><mfrac><mn>1</mn><mn>23</mn></mfrac></math>"_s);
    }

    // L'aperçu d'une équation sans marque est, pixel à pixel, le dessin de la page ; il diffère du repère
    // neutre d'un MathML mal formé ; avec les marques, il montre la case vide et le curseur, que la page ne
    // dessine pas.
    void previewMatchesThePageWithoutMarks()
    {
        const QSize size(240, 120);
        const QImage preview = previewImage(oneHalf(), size);
        QCOMPARE(preview, pageImage(oneHalf(), size));
        QVERIFY(preview != previewImage(u"<math><mfrac>"_s, size));
        const QString marked =
            u"<math><mfrac><mn class=\"actcraft-cursor\">1</mn><mrow class=\"actcraft-placeholder\"/></mfrac></math>"_s;
        QVERIFY(previewImage(marked, size) != pageImage(marked, size));

        // L'objet que l'éditeur pose : la page le dessine (paintObjectLocal, à ses bornes) comme à l'échelle
        // 1, au lissage près ; l'aperçu à ces bornes est ce dessin.
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QQuickItem* panel = openEditor(main);
        QVERIFY(panel != nullptr);
        EquationController* equation = equationOf(main);
        QVERIFY(equation != nullptr);
        typeOneHalf(*equation);
        QVERIFY(clickIn(main, panel, u"equationOkButton"_s));
        const core::Equation* inserted = lastEquation(*controller);
        QVERIFY(inserted != nullptr);
        const core::PageObject object = shown(*controller).objects.back();
        const core::RectF& b = inserted->bounds;
        const QSize box(int(std::ceil(b.width)) + 2, int(std::ceil(b.height)) + 2);
        QImage page(box, QImage::Format_ARGB32_Premultiplied);
        page.fill(Qt::white);
        {
            QPainter painter(&page);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.translate(1 - b.x, 1 - b.y);
            actcraft::render::paintObjectLocal(painter, object, {});
        }
        QImage natural(box, QImage::Format_ARGB32_Premultiplied);
        natural.fill(Qt::white);
        {
            QPainter painter(&natural);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.translate(1, 1);
            actcraft::render::paintEquation(painter, atScaleOne(*inserted),
                                            actcraft::render::EquationPaintMode::Page);
        }
        const int drift = maxDifference(page, natural);
        QVERIFY2(drift <= 32, qPrintable(u"écart à l'échelle 1 : %1"_s.arg(drift)));
        const QImage shownPreview = previewImage(oneHalf(), QSizeF(b.width, b.height), box, QPointF(1, 1));
        const int previewDrift = maxDifference(shownPreview, page);
        QVERIFY2(previewDrift <= 2, qPrintable(u"écart de l'aperçu : %1"_s.arg(previewDrift)));
    }

    void insertedEquationIsDrawnAtScaleOne_data()
    {
        QTest::addColumn<QString>("model");
        QTest::addColumn<QString>("first");
        QTest::addColumn<QString>("second");
        QTest::addColumn<QString>("written");
        QTest::newRow("chiffre") << QString() << u"1"_s << QString() << u"<math><mn>1</mn></math>"_s;
        QTest::newRow("fraction") << u"fraction"_s << u"1"_s << u"2"_s << oneHalf();
        QTest::newRow("descendante") << QString() << u"y"_s << QString() << u"<math><mi>y</mi></math>"_s;
    }

    // L'équation que l'éditeur pose est dessinée par la page (paintObjectLocal) à l'échelle 1, à 1 % près :
    // ses bornes sont le cadre que la page met à l'échelle (boîte de la mise en page et encre), pas l'encre
    // seule. Un chiffre (boîte plus haute que l'encre), une fraction, un « y » (jambage).
    void insertedEquationIsDrawnAtScaleOne()
    {
        QFETCH(QString, model);
        QFETCH(QString, first);
        QFETCH(QString, second);
        QFETCH(QString, written);
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QQuickItem* panel = openEditor(main);
        QVERIFY(panel != nullptr);
        EquationController* equation = equationOf(main);
        QVERIFY(equation != nullptr);
        if (!model.isEmpty()) {
            QVERIFY(equation->insertTemplate(model));
        }
        equation->type(first);
        if (!second.isEmpty()) {
            equation->moveRight();
            equation->type(second);
        }
        QCOMPARE(equation->mathml(), written);
        QVERIFY(clickIn(main, panel, u"equationOkButton"_s));
        const core::Equation* inserted = lastEquation(*controller);
        QVERIFY(inserted != nullptr);
        const core::PageObject object = shown(*controller).objects.back();

        const QSizeF onPage =
            inkSize([&](QPainter& p) { actcraft::render::paintObjectLocal(p, object, {}); });
        const core::Equation natural = atScaleOne(*inserted);
        const QSizeF reference = inkSize([&](QPainter& p) {
            actcraft::render::paintEquation(p, natural, actcraft::render::EquationPaintMode::Page);
        });
        QVERIFY(reference.width() > 1 && reference.height() > 1);
        const double sx = onPage.width() / reference.width();
        const double sy = onPage.height() / reference.height();
        QVERIFY2(std::abs(sx - 1) < 0.01 && std::abs(sy - 1) < 0.01,
                 qPrintable(u"échelle %1 × %2"_s.arg(sx).arg(sy)));
    }

    // Éditeur ouvert, un objet sélectionné sur la page : Suppr ne l'efface pas, Échap ferme l'éditeur sans
    // changer d'outil. Les raccourcis de l'application (edit.delete sur Suppr, tool.select sur Échap) ne
    // partent pas d'une touche tapée dans l'éditeur. Témoin : l'éditeur fermé, Suppr déclenche edit.delete.
    void editorKeysDoNotReachTheShortcuts()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        auto* commands = main.window()->findChild<actcraft::ui::Commands*>(u"commands"_s);
        QVERIFY(commands != nullptr);
        QSignalSpy fired(commands, &actcraft::ui::Commands::triggered);
        // Un raccourci d'application n'agit que dans une fenêtre active.
        main.window()->requestActivate();
        QVERIFY(QTest::qWaitForWindowActive(main.window()));
        const auto deleteCount = [&fired] {
            int n = 0;
            for (const QList<QVariant>& call : fired) {
                n += call.at(0).toString() == u"edit.delete"_s ? 1 : 0;
            }
            return n;
        };
        const auto* tool = main.pageView()->currentTool();

        // Témoin, éditeur fermé : Suppr déclenche edit.delete.
        QTest::keyClick(main.window(), Qt::Key_Delete);
        QTRY_COMPARE(deleteCount(), 1);

        QQuickItem* panel = openEditor(main);
        QVERIFY(panel != nullptr);

        // Les deux touches sont jouées avant de conclure : un raccourci qui fuit est nommé, sans cacher
        // l'autre.
        QStringList leaks;

        // Dans l'éditeur, Suppr efface la saisie et ne déclenche pas edit.delete.
        QTest::keyClick(main.window(), Qt::Key_Delete);
        QTest::qWait(50);
        if (deleteCount() != 1) {
            leaks << u"Suppr a déclenché edit.delete"_s;
        }
        QVERIFY(panel->isVisible() || deleteCount() != 1);

        // Échap ferme l'éditeur sans changer d'outil.
        QTest::keyClick(main.window(), Qt::Key_Escape);
        QTest::qWait(100);
        if (panel->isVisible()) {
            leaks << u"Échap n'a pas fermé l'éditeur (pris par tool.select)"_s;
        }
        if (main.pageView()->currentTool() != tool) {
            leaks << u"Échap a changé d'outil"_s;
        }
        QVERIFY2(leaks.isEmpty(), qPrintable(leaks.join(u" ; "_s)));
    }

    void equationEditorBlocksTheStylus_data() { presentationharness::addPointerRows(); }

    // Éditeur ouvert, il couvre la page quel que soit le périphérique : la touche « 1 » saisit sans tracer,
    // la barre déplace sans tracer, le corps retient le trait ; hors du panneau, un trait est versé. À la
    // souris, le clic droit sur le panneau n'ouvre pas le menu de la page, hors du panneau il l'ouvre.
    void equationEditorBlocksTheStylus()
    {
        QFETCH(Pointer, pointer);
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QQuickItem* panel = openEditor(main);
        QVERIFY(panel != nullptr);
        EquationController* equation = equationOf(main);
        QVERIFY(equation != nullptr);
        QQuickItem* one = presentationharness::findItem(panel, u"equationKey1"_s);
        QQuickItem* hint = presentationharness::findItem(panel, u"equationHint"_s);
        QVERIFY(one != nullptr && hint != nullptr);
        const int before = main.pageObjectCount();

        // Touche « 1 » : la case reçoit 1, aucun trait.
        const QPointF button = centreOf(one);
        if (pointer == Pointer::Stylus) {
            QVERIFY2(stylusTap(main.window(), button), "tablette non acceptée : rejouée en souris par Qt");
        } else {
            presentationharness::strokeAt(main.window(), pointer, button - QPointF(6, 0),
                                          button + QPointF(6, 0), 3);
        }
        QTRY_COMPARE(equation->mathml(), u"<math><mn>1</mn></math>"_s);
        QTest::qWait(50);
        QCOMPARE(main.pageObjectCount(), before);

        // Barre de titre : le panneau se déplace, aucun trait.
        const QPointF start = panel->position();
        const QPointF bar = panel->mapToScene(QPointF(100, 22));
        presentationharness::strokeAt(main.window(), pointer, bar, bar + QPointF(60, 40));
        QTest::qWait(50);
        QCOMPARE(main.pageObjectCount(), before);
        QVERIFY2(panel->position() != start, "panneau déplacé par sa barre");

        // Corps, sans gestionnaire : seule la couverture retient le trait.
        const QPointF body = centreOf(hint) - QPointF(50, 0);
        presentationharness::strokeAt(main.window(), pointer, body, body + QPointF(100, 0));
        QTest::qWait(50);
        QCOMPARE(main.pageObjectCount(), before);

        if (pointer == Pointer::Mouse) {
            QSignalSpy menu(main.pageView(), &actcraft::ui::PageView::contextMenuRequested);
            QVERIFY(menu.isValid());
            QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier, centreOf(hint).toPoint());
            QTest::qWait(50);
            QCOMPARE(menu.count(), 0);
            QTest::mouseClick(main.window(), Qt::RightButton, Qt::NoModifier,
                              main.pageView()->pageToScene(QPointF(40, 700)).toPoint());
            QTRY_COMPARE(menu.count(), 1);
            // Lot 3 : le clic droit ouvre le menu de la page. Échap, raccourci de « Sélectionner »
            // (commands.json), ne le ferme pas ici et changerait l'outil (consigné) ; ouvert, le menu
            // prendrait l'appui du trait : un clic hors du menu, sur le panneau, le ferme.
            QObject* pageMenu = main.window()->findChild<QObject*>(QStringLiteral("pageContextMenu"));
            QVERIFY(pageMenu);
            QTest::mouseClick(main.window(), Qt::LeftButton, Qt::NoModifier, centreOf(hint).toPoint());
            QTRY_VERIFY(!pageMenu->property("visible").toBool());
        }

        // Hors du panneau : un trait est versé.
        presentationharness::strokeAt(main.window(), pointer, main.pageView()->pageToScene(QPointF(40, 700)),
                                      main.pageView()->pageToScene(QPointF(140, 700)));
        QTRY_COMPARE(main.pageObjectCount(), before + 1);
    }
    // « equation.edit <id> » rouvre l'éditeur sur l'équation ; la saisie va à la fin (m1 de la revue de
    // L7-T26) ; « OK » la remplace sous le même identifiant, à la même place, en une étape, sans marque de
    // l'éditeur.
    void editKeepsTheId()
    {
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        const std::string original = "<math><mi>a</mi><mo>+</mo><mi>b</mi></math>";
        const core::ObjectId id = addEquation(*controller, original);
        QVERIFY(id != 0);
        const std::size_t count = shown(*controller).objects.size();
        const std::size_t rank = count - 1;
        QQuickItem* panel = editEquation(main, id);
        QVERIFY(panel != nullptr);
        EquationController* equation = equationOf(main);
        QVERIFY(equation != nullptr);
        QCOMPARE(equation->mathml(), QString::fromStdString(original));
        equation->type(u"c"_s);
        QCOMPARE(equation->mathml(), u"<math><mi>a</mi><mo>+</mo><mi>b</mi><mi>c</mi></math>"_s);

        QVERIFY(clickIn(main, panel, u"equationOkButton"_s));
        QTRY_VERIFY(!panel->isVisible());
        QCOMPARE(shown(*controller).objects.size(), count);
        QCOMPARE(shown(*controller).objects[rank].common.id, id);
        const core::Equation* edited = equationById(*controller, id);
        QVERIFY(edited != nullptr);
        const QString written = QString::fromStdString(edited->mathml);
        QCOMPARE(written, u"<math><mi>a</mi><mo>+</mo><mi>b</mi><mi>c</mi></math>"_s);
        QVERIFY2(!written.contains(u"actcraft-cursor"_s), qPrintable(written));
        QVERIFY2(!written.contains(u"actcraft-placeholder"_s), qPrintable(written));

        controller->undo(); // une seule étape rend l'équation d'origine, sous le même identifiant
        const core::Equation* back = equationById(*controller, id);
        QVERIFY(back != nullptr);
        QCOMPARE(back->mathml, original);
        QCOMPARE(shown(*controller).objects.size(), count);
        controller->redo();
        QCOMPARE(QString::fromStdString(equationById(*controller, id)->mathml), written);
    }

    // F-301, L7-T34 : avec l'outil Sélectionner, un double appui sur une équation de la page la rouvre dans
    // l'éditeur (equation.edit <id>) ; un appui simple ne l'ouvre pas ; avec le stylo, rien ne s'ouvre.
    void doubleClickEditsTheEquation()
    {
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        const core::ObjectId id = addEquation(*controller, "<math><mi>x</mi><mo>+</mo><mi>y</mi></math>");
        QVERIFY(id != 0);
        const core::Equation* equation = equationById(*controller, id);
        QVERIFY(equation != nullptr);
        const QPointF centre = main.pageView()->pageToScene(QPointF(equation->bounds.x + equation->bounds.width / 2,
                                                                    equation->bounds.y + equation->bounds.height / 2));
        QQuickItem* panel = main.item(u"equationRoot"_s);
        QVERIFY(panel != nullptr);
        QVERIFY(!panel->isVisible());
        const int interval = QGuiApplication::styleHints()->mouseDoubleClickInterval();

        // Stylo : deux appuis rapprochés versent des points, aucun éditeur ; les points sont annulés.
        const std::size_t count = shown(*controller).objects.size();
        QVERIFY(main.run(u"tool.pen"_s));
        stylusTap(main.window(), centre);
        stylusTap(main.window(), centre);
        QTest::qWait(50);
        QVERIFY2(!panel->isVisible(), "stylo : pas d'éditeur");
        for (int i = 0; i < 4 && shown(*controller).objects.size() > count; ++i) {
            controller->undo();
        }
        QCOMPARE(shown(*controller).objects.size(), count);
        QTest::qWait(interval + 100);

        // Sélectionner : un appui simple n'ouvre rien ; deux appuis rapprochés ouvrent l'éditeur sur elle.
        QVERIFY(main.run(u"tool.select"_s));
        stylusTap(main.window(), centre);
        QTest::qWait(interval + 100);
        QVERIFY2(!panel->isVisible(), "appui simple : pas d'éditeur");
        stylusTap(main.window(), centre);
        stylusTap(main.window(), centre);
        QTRY_VERIFY2(panel->isVisible(), "double appui : l'éditeur s'ouvre");
        EquationController* editor = equationOf(main);
        QVERIFY(editor != nullptr);
        QCOMPARE(editor->editingId(), static_cast<quint64>(id));
        QCOMPARE(editor->mathml(), u"<math><mi>x</mi><mo>+</mo><mi>y</mi></math>"_s);
    }

    // Une équation rééditée puis enregistrée en .actcraft se relit identique (empreinte du document
    // comprise).
    void equationRoundTripIdentical()
    {
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        const core::ObjectId id = addEquation(*controller, "<math><mi>a</mi><mo>+</mo><mi>b</mi></math>");
        QQuickItem* panel = editEquation(main, id);
        QVERIFY(panel != nullptr);
        equationOf(main)->type(u"c"_s);
        QVERIFY(clickIn(main, panel, u"equationOkButton"_s));
        const QString before = controller->property("fingerprint").toString();
        const core::Equation* edited = equationById(*controller, id);
        QVERIFY(edited != nullptr);
        const core::Equation keep = *edited;

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QUrl url = QUrl::fromLocalFile(dir.filePath(u"eq.actcraft"_s));
        QVERIFY(saveTo(*controller, url));
        controller->newDocument(1024, 768);
        QVERIFY(controller->property("fingerprint").toString() != before);
        QVERIFY(openFrom(*controller, url));
        QCOMPARE(controller->property("fingerprint").toString(), before);
        const core::Equation* reread = equationById(*controller, id);
        QVERIFY(reread != nullptr);
        QCOMPARE(reread->mathml, keep.mathml);
        QCOMPARE(reread->pointSize, keep.pointSize);
        QCOMPARE(reread->bounds.x, keep.bounds.x);
        QCOMPARE(reread->bounds.width, keep.bounds.width);
    }

    // MathML non canonique (espace de noms, blancs, entité) : lu, rouvert dans l'éditeur, « Annuler » puis
    // « OK » sans modification laissent les mêmes octets ; seule une modification écrit la forme canonique.
    void untouchedEquationKeepsItsBytes()
    {
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        const std::string odd =
            "<math xmlns=\"http://www.w3.org/1998/Math/MathML\">\n  <mi>&#x3B1;</mi> <mo>+</mo>\n"
            "  <mn>1</mn>\n</math>";
        const core::ObjectId id = addEquation(*controller, odd);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QUrl url = QUrl::fromLocalFile(dir.filePath(u"odd.actcraft"_s));
        QVERIFY(saveTo(*controller, url));
        controller->newDocument(1024, 768);
        QVERIFY(openFrom(*controller, url));
        QVERIFY(equationById(*controller, id) != nullptr);
        QCOMPARE(equationById(*controller, id)->mathml, odd);
        QVERIFY(!controller->canUndo());

        QQuickItem* panel = editEquation(main, id);
        QVERIFY(panel != nullptr);
        QVERIFY(clickIn(main, panel, u"equationCancelButton"_s));
        QTRY_VERIFY(!panel->isVisible());
        QCOMPARE(equationById(*controller, id)->mathml, odd);
        QVERIFY(!controller->canUndo());

        panel = editEquation(main, id);
        QVERIFY(panel != nullptr);
        QVERIFY(clickIn(main, panel, u"equationOkButton"_s));
        QTRY_VERIFY(!panel->isVisible());
        QCOMPARE(equationById(*controller, id)->mathml, odd);
        QVERIFY(!controller->canUndo());

        panel = editEquation(main, id);
        QVERIFY(panel != nullptr);
        equationOf(main)->type(u"2"_s);
        QVERIFY(clickIn(main, panel, u"equationOkButton"_s));
        QVERIFY(equationById(*controller, id)->mathml != odd);
        QVERIFY(controller->canUndo());
    }

    // Une équation lue d'un .flipchart (objet MathML, L7-T1) est dessinée : de l'encre sombre dans sa boîte.
    void flipchartEquationIsDrawn()
    {
        namespace synth = fctest::synth;
        synth::Sheet sheet(1003);
        sheet.tree = {fctest::Node(1010, 460, 1011)};
        sheet.entries = {
            synth::io(1011, synth::mathmlObject(1011, u"<math><mfrac><mn>1</mn><mn>2</mn></mfrac></math>"_s,
                                                100, 100, 200, 120))};
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(u"eq.flipchart"_s);
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(synth::paperboard(u"eq"_s, {sheet}));
        file.close();

        MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(openFrom(*controller, QUrl::fromLocalFile(path)));
        const auto& objects = shown(*controller).objects;
        QCOMPARE(objects.size(), std::size_t{1});
        QVERIFY(std::get_if<core::Equation>(&core::body(objects[0])) != nullptr);
        const QImage image = actcraft::render::renderPage(shown(*controller), {}, 1.0);
        QVERIFY(!image.isNull());
        int dark = 0;
        for (int y = 100; y < 220 && y < image.height(); ++y) {
            for (int x = 100; x < 300 && x < image.width(); ++x) {
                dark += qGray(image.pixel(x, y)) < 100 ? 1 : 0;
            }
        }
        QVERIFY2(dark > 20, qPrintable(u"pixels sombres : %1"_s.arg(dark)));
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("equation", TstUiEquation)
#include "tst_ui_equation.moc"
