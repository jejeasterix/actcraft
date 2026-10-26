#include "equationcontroller.h"
#include "equationpreviewitem.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/core/model.h>
#include <actcraft/render/equationpaint.h>
#include <actcraft/ui/documentcontroller.h>

#include <QImage>
#include <QPainter>
#include <QSignalSpy>
#include <QtTest>

#include <cmath>
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

QImage previewImage(const QString& mathml, QSize size)
{
    EquationPreview preview;
    preview.setSize(QSizeF(size));
    preview.setMathml(mathml);
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    preview.paint(&painter);
    painter.end();
    return image;
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
            QTest::keyClick(main.window(), Qt::Key_Escape);
        }

        // Hors du panneau : un trait est versé.
        presentationharness::strokeAt(main.window(), pointer, main.pageView()->pageToScene(QPointF(40, 700)),
                                      main.pageView()->pageToScene(QPointF(140, 700)));
        QTRY_COMPARE(main.pageObjectCount(), before + 1);
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("equation", TstUiEquation)
#include "tst_ui_equation.moc"
