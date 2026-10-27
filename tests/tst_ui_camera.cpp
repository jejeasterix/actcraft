#include "cameracontroller.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QColor>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRectF>
#include <QScreen>
#include <QtTest>

namespace {

using actcraft::ui::CameraController;
using actcraft::ui::DocumentController;
using presentationharness::MainWindow;
using presentationharness::Pointer;

// Contrôleur porté par la racine `cameraRoot` de Presentation.qml (cherché par l'arbre des objets).
CameraController* cameraOf(const MainWindow& main)
{
    if (!main.window()) {
        return nullptr;
    }
    QObject* root = main.window()->findChild<QObject*>(QStringLiteral("cameraRoot"));
    if (!root) {
        return nullptr;
    }
    return qobject_cast<CameraController*>(root->property("controller").value<QObject*>());
}

DocumentController* openDocumentOf(const MainWindow& main)
{
    return main.pageView() ? main.pageView()->documentController() : nullptr;
}

// Écran figé injecté (aucune capture réelle dans les tests) : la taille de l'écran principal, moitié gauche
// bleue, moitié droite orange.
QImage fakeScreen()
{
    const QSize size = QGuiApplication::primaryScreen()->geometry().size();
    QImage image(size, QImage::Format_ARGB32);
    image.fill(QColor(20, 120, 200));
    QPainter painter(&image);
    painter.fillRect(QRect(size.width() / 2, 0, size.width() - size.width() / 2, size.height()),
                     QColor(230, 140, 30));
    return image;
}

// Fenêtre de capture ouverte par la commande et exposée.
QQuickWindow* openCapture(MainWindow& main, CameraController* camera, const QString& command)
{
    if (!main.run(command)) {
        return nullptr;
    }
    QQuickWindow* window = camera->window();
    if (!window || !QTest::qWaitForWindowExposed(window)) {
        return nullptr;
    }
    return window;
}

bool nearRect(const QRectF& a, const QRectF& b, qreal tolerance)
{
    return qAbs(a.left() - b.left()) <= tolerance && qAbs(a.top() - b.top()) <= tolerance &&
           qAbs(a.right() - b.right()) <= tolerance && qAbs(a.bottom() - b.bottom()) <= tolerance;
}

QString describe(const QRectF& r)
{
    return QStringLiteral("(%1, %2, %3 x %4)").arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
}

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
class TstUiCamera : public QObject
{
    Q_OBJECT

private slots:
    // LE test qui garde l'invariant (décision 4 bis) : la sélection se fait dans une fenêtre à elle, créée à la
    // demande et sans parent (avec la principale pour parent, elle se réduirait avec elle) ; la palette est un
    // élément de CETTE fenêtre, pas une couverture de la vue principale. Chaque pointeur, appuyé et glissé sur
    // l'image figée, trace le rectangle de sélection ; un appui sur la palette ne le touche pas ; la page
    // principale ne reçoit rien. Témoin : un trait sur la vue principale, aux coordonnées de scène où la
    // palette se trouve dans sa fenêtre, est versé. Mutants : palette déclarée couverture de la vue principale
    // (patron des panneaux du lot, pageArea: presentation.pageArea) → le témoin au stylet est arrêté ;
    // sélection en surcouche de la fenêtre principale → aucune fenêtre de capture.
    void cameraWindowTakesEveryPointer_data() { presentationharness::addPointerRows(); }
    void cameraWindowTakesEveryPointer()
    {
        QFETCH(Pointer, pointer);
        MainWindow main;
        QVERIFY(main.window());
        QVERIFY(main.waitReady());
        CameraController* camera = cameraOf(main);
        QVERIFY(camera);
        camera->setCaptureSource([] { return fakeScreen(); });
        DocumentController* open = openDocumentOf(main);
        QVERIFY(open);
        const QString openBefore = open->fingerprintText();
        const int mainObjects = main.pageObjectCount();
        QVERIFY(!camera->window());

        QQuickWindow* window = openCapture(main, camera, QStringLiteral("tool.camera-area"));
        QVERIFY(window);
        QVERIFY(window != main.window());
        QVERIFY(window->transientParent() == nullptr);
        QVERIFY(window->flags().testFlag(Qt::FramelessWindowHint));
        QVERIFY(window->flags().testFlag(Qt::WindowStaysOnTopHint));
        QQuickItem* palette =
            presentationharness::findItem(window->contentItem(), QStringLiteral("cameraPalette"));
        QVERIFY(palette);
        QVERIFY(!presentationharness::findItem(main.window()->contentItem(), QStringLiteral("cameraPalette")));

        // La palette posée au milieu de la vue principale : mêmes coordonnées de scène, autre fenêtre.
        actcraft::ui::PageView* view = main.pageView();
        QVERIFY(view);
        const QPointF spot = view->mapRectToScene(view->boundingRect()).center();
        palette->setPosition(palette->parentItem()->mapFromScene(spot) -
                             QPointF(palette->width() / 2, palette->height() / 2));
        const QRectF paletteRect = palette->mapRectToScene(palette->boundingRect());
        QVERIFY2(paletteRect.contains(spot), qPrintable(describe(paletteRect)));

        // Appui et glisser sur l'image figée, hors de la sélection d'origine et loin de la palette.
        const QSizeF size = window->size();
        const QPointF from(size.width() * 0.06, size.height() * 0.06);
        const QPointF to(size.width() * 0.42, size.height() * 0.2);
        const QRectF expected = QRectF(from, to).normalized();
        QVERIFY2(!camera->selection().contains(from), qPrintable(describe(camera->selection())));
        QVERIFY(!paletteRect.intersects(expected));
        QVERIFY(window->contentItem()->contains(window->contentItem()->mapFromScene(to)));
        presentationharness::strokeAt(window, pointer, from, to);
        QTRY_VERIFY2(nearRect(camera->selection(), expected, 2),
                     qPrintable(describe(camera->selection()) + QStringLiteral(" / ") + describe(expected)));
        const QRectF drawn = camera->selection();

        // Appui sur la palette : la sélection ne bouge pas, la capture reste ouverte.
        presentationharness::strokeAt(window, pointer, spot, spot + QPointF(-50, -30));
        QTest::qWait(50);
        QCOMPARE(camera->selection(), drawn);
        QCOMPARE(camera->window(), window);

        // La page principale n'a rien reçu.
        QCOMPARE(main.pageObjectCount(), mainObjects);
        QCOMPARE(open->fingerprintText(), openBefore);

        // Témoin : la palette n'arrête rien sur la vue principale.
        const QPointF witness = palette->mapRectToScene(palette->boundingRect()).center();
        QVERIFY2(view->contains(view->mapFromScene(witness)),
                 qPrintable(QStringLiteral("témoin (%1, %2) hors de la vue %3")
                                .arg(witness.x())
                                .arg(witness.y())
                                .arg(describe(view->mapRectToScene(view->boundingRect())))));
        presentationharness::strokeAt(main.window(), pointer, witness, witness + QPointF(40, 30));
        QTRY_COMPARE(main.pageObjectCount(), mainObjects + 1);
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("camera", TstUiCamera)
#include "tst_ui_camera.moc"
