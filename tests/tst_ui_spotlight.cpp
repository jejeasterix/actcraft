#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/features/spotlight.h>
#include <actcraft/ui/documentcontroller.h>

#include <QBuffer>
#include <QImage>
#include <QSignalSpy>
#include <QtTest>

#include <variant>

namespace {

using actcraft::ui::DocumentController;
using presentationharness::MainWindow;
using presentationharness::Pointer;
namespace core = actcraft::core;
namespace features = actcraft::features;

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
QObject* spotlightOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<QObject*>(QStringLiteral("spotlight")) : nullptr;
}

DocumentController* controllerOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<DocumentController*>(QStringLiteral("documentController"))
                         : nullptr;
}

QPointF inWindow(const MainWindow& main, QPointF pagePoint)
{
    return main.pageView()->pageToScene(pagePoint);
}

double read(const QObject* spotlight, const char* name)
{
    return spotlight->property(name).toDouble();
}

bool closeTo(double actual, double expected)
{
    return qAbs(actual - expected) <= 1.5; // arrondi du point entier envoyé à la fenêtre, zoom de la vue
}

int imageCount(const DocumentController& controller)
{
    const core::Page& page = controller.document().page(static_cast<std::size_t>(controller.currentIndex()));
    int count = 0;
    for (const core::PageObject& object : page.objects) {
        if (std::holds_alternative<core::ImageObject>(core::body(object))) {
            ++count;
        }
    }
    return count;
}

class TstUiSpotlight : public QObject
{
    Q_OBJECT

private slots:
    // Surcouche : aucune demande de redessin du cache (ouverture, déplacement, redimensionnement, fermeture)
    // ; le spot ne verse aucun objet.
    void spotlightNeverRefreshesTheCache()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* spotlight = spotlightOf(main);
        QVERIFY(spotlight != nullptr);
        QSignalSpy spy(main.pageView(), &actcraft::ui::PageView::cacheRefreshRequested);
        QVERIFY(spy.isValid());
        const int before = main.pageObjectCount();

        QVERIFY(main.run(QStringLiteral("present.spotlight-circle")));
        QCOMPARE(spotlight->property("shape").toInt(), 1);
        QVERIFY(closeTo(read(spotlight, "centerX"), 512));
        QVERIFY(closeTo(read(spotlight, "centerY"), 384));

        // Déplacé par la partie masquée.
        presentationharness::strokeAt(main.window(), Pointer::Mouse, inWindow(main, QPointF(100, 100)),
                                      inWindow(main, QPointF(150, 120)));
        QTest::qWait(100);
        QVERIFY2(
            closeTo(read(spotlight, "centerX"), 562) && closeTo(read(spotlight, "centerY"), 404),
            qPrintable(
                QStringLiteral("%1,%2").arg(read(spotlight, "centerX")).arg(read(spotlight, "centerY"))));

        // Redimensionné par le pourtour droit (500 + 150 + 50 = bord à x = 712) : ovale.
        presentationharness::strokeAt(main.window(), Pointer::Mouse, inWindow(main, QPointF(712, 404)),
                                      inWindow(main, QPointF(812, 404)));
        QTest::qWait(100);
        QVERIFY2(closeTo(read(spotlight, "frameWidth"), 400) && closeTo(read(spotlight, "frameHeight"), 300),
                 qPrintable(QStringLiteral("%1x%2")
                                .arg(read(spotlight, "frameWidth"))
                                .arg(read(spotlight, "frameHeight"))));

        QVERIFY(main.run(QStringLiteral("present.spotlight-circle"))); // la commande active ferme le spot
        QCOMPARE(spotlight->property("shape").toInt(), 0);
        QCOMPARE(spy.count(), 0);
        QCOMPARE(main.pageObjectCount(), before);
    }

    void spotlightBlocksTheStylus_data() { presentationharness::addPointerRows(); }

    // Le store est une couverture (addInputCover) : rien n'est versé sous la partie masquée, quel que soit le
    // périphérique ; elle déplace le spot (effet du gestionnaire lu) ; le centre éclairé reçoit le trait ; un
    // cache plein se déplace par son intérieur sans rien verser.
    void spotlightBlocksTheStylus()
    {
        QFETCH(Pointer, pointer);
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QObject* spotlight = spotlightOf(main);
        QVERIFY(spotlight != nullptr);
        const int before = main.pageObjectCount();
        QVERIFY(main.run(QStringLiteral("present.spotlight-circle")));

        // Glissé dans le masque : spot déplacé de (+50, +20), aucun trait.
        presentationharness::strokeAt(main.window(), pointer, inWindow(main, QPointF(100, 100)),
                                      inWindow(main, QPointF(150, 120)));
        QTest::qWait(150);
        QCOMPARE(main.pageObjectCount(), before);
        QVERIFY2(
            closeTo(read(spotlight, "centerX"), 562) && closeTo(read(spotlight, "centerY"), 404),
            qPrintable(
                QStringLiteral("%1,%2").arg(read(spotlight, "centerX")).arg(read(spotlight, "centerY"))));

        // Glissé dans la zone éclairée (loin du pourtour) : un trait versé, le spot n'a pas bougé.
        presentationharness::strokeAt(main.window(), pointer, inWindow(main, QPointF(530, 404)),
                                      inWindow(main, QPointF(600, 404)));
        QTRY_COMPARE_WITH_TIMEOUT(main.pageObjectCount(), before + 1, 5000);
        QVERIFY(closeTo(read(spotlight, "centerX"), 562) && closeTo(read(spotlight, "centerY"), 404));

        // Cache plein : on le saisit par l'intérieur, déplacé de (+40, 0), aucun trait de plus.
        QVERIFY(main.run(QStringLiteral("present.spotlight-solid-circle")));
        QCOMPARE(spotlight->property("shape").toInt(), 3);
        presentationharness::strokeAt(main.window(), pointer, inWindow(main, QPointF(540, 404)),
                                      inWindow(main, QPointF(580, 404)));
        QTest::qWait(150);
        QCOMPARE(main.pageObjectCount(), before + 1);
        QVERIFY2(closeTo(read(spotlight, "centerX"), 602),
                 qPrintable(QString::number(read(spotlight, "centerX"))));
    }

    // Le contour par défaut est celui du réglage embarqué (10), lu sur la surcouche ; jamais 15.
    void spotlightOutlineIsTheDefault()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QQuickItem* root = main.item(QStringLiteral("spotlightRoot"));
        QVERIFY(root != nullptr);
        QCOMPARE(root->property("outline").toDouble(), 10.0);
        QCOMPARE(root->property("outline").toDouble(), features::Spotlight::kDefaultOutline);
        QObject* spotlight = spotlightOf(main);
        QVERIFY(spotlight != nullptr);
        QCOMPARE(spotlight->property("outline").toDouble(), 10.0);
    }

    // Curseur selon la zone survolée : tout déplacer à l'intérieur d'un cache plein, redimensionner sur le
    // bord (horizontal à droite, vertical en haut), flèche hors de la zone saisissable.
    void spotlightCursorFollowsTheZone()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        QQuickItem* root = main.item(QStringLiteral("spotlightRoot"));
        QVERIFY(root != nullptr);
        QVERIFY(main.run(QStringLiteral("present.spotlight-solid-circle")));
        auto* hover = root->findChild<QObject*>(QStringLiteral("spotlightHover"));
        QVERIFY(hover != nullptr);
        const auto cursorAfterHoverAt = [&](QPointF pagePoint) {
            QTest::mouseMove(main.window(), inWindow(main, pagePoint).toPoint());
            QTest::qWait(60);
            QTest::mouseMove(main.window(), inWindow(main, pagePoint + QPointF(1, 0)).toPoint());
            QTest::qWait(60);
            return hover->property("cursorShape").toInt();
        };

        const int center = cursorAfterHoverAt(QPointF(500, 384));
        QCOMPARE(center, static_cast<int>(Qt::SizeAllCursor));
        const int right = cursorAfterHoverAt(QPointF(661, 384)); // bord droit du cadre : 512 + 150
        QCOMPARE(right, static_cast<int>(Qt::SizeHorCursor));
        const int top = cursorAfterHoverAt(QPointF(512, 235)); // bord haut : 384 - 150
        QCOMPARE(top, static_cast<int>(Qt::SizeVerCursor));
        QVERIFY(center != right && right != top && center != top);
    }

    // Capture de la zone éclairée : une image de plus (une étape), l'annulation la retire avec sa ressource ;
    // le trait déjà écrit reste.
    void spotlightCaptureIsUndoneAtOnce()
    {
        MainWindow main;
        QVERIFY(main.window() != nullptr);
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QQuickItem* root = main.item(QStringLiteral("spotlightRoot"));
        QVERIFY(root != nullptr);
        QVERIFY(main.run(QStringLiteral("present.spotlight-square")));

        // Un trait dans la zone éclairée (stylet).
        const int base = main.pageObjectCount();
        presentationharness::strokeAt(main.window(), Pointer::Stylus, inWindow(main, QPointF(420, 384)),
                                      inWindow(main, QPointF(600, 384)));
        QTRY_COMPARE_WITH_TIMEOUT(main.pageObjectCount(), base + 1, 5000);
        const int strokes = main.pageObjectCount();
        QCOMPARE(imageCount(*controller), 0);
        const std::size_t resources = controller->document().resources().size();

        bool started = false;
        QVERIFY(QMetaObject::invokeMethod(root, "captureLit", Q_RETURN_ARG(bool, started)));
        QVERIFY(started);
        QTRY_COMPARE_WITH_TIMEOUT(imageCount(*controller), 1, 10000);
        QTRY_COMPARE_WITH_TIMEOUT(main.pageObjectCount(), strokes + 1, 5000);
        QCOMPARE(controller->document().resources().size(), resources + 1);
        QVERIFY(root->isVisible()); // le masque est revenu après la prise

        // Une seule annulation : l'image et sa ressource partent, le trait reste.
        controller->undo();
        QTRY_COMPARE_WITH_TIMEOUT(main.pageObjectCount(), strokes, 5000);
        QCOMPARE(imageCount(*controller), 0);
        QCOMPARE(controller->document().resources().size(), resources);
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("spotlight", TstUiSpotlight)
#include "tst_ui_spotlight.moc"
