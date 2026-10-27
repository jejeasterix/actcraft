#include "cameracontroller.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/features/camera.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QClipboard>
#include <QColor>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRect>
#include <QRectF>
#include <QScreen>
#include <QStringList>
#include <QTemporaryDir>
#include <QVariantList>
#include <QVariantMap>
#include <QtTest>

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <dwmapi.h>
#include <windows.h>
#endif

namespace {

namespace core = actcraft::core;
using actcraft::ui::CameraController;
using actcraft::ui::Commands;
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

// Fenêtre de capture ouverte par la commande et exposée (la photographie attend que le menu quitte l'écran).
QQuickWindow* openCapture(MainWindow& main, CameraController* camera, const QString& command)
{
    if (!main.run(command) || !QTest::qWaitFor([camera] { return camera->window() != nullptr; }, 5000)) {
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

const QColor kLeftColor(20, 120, 200);
const QColor kRightColor(230, 140, 30);

// Zone à cheval sur les deux moitiés de l'écran figé : 100 x 60 pixels.
QRectF straddlingZone()
{
    const qreal middle = QGuiApplication::primaryScreen()->geometry().width() / 2.0;
    return QRectF(middle - 50, 100, 100, 60);
}

QStringList imageKeys(const core::Page& page)
{
    QStringList keys;
    for (const core::PageObject& object : page.objects) {
        if (const auto* image = std::get_if<core::ImageObject>(&core::body(object))) {
            keys << QString::fromStdString(image->resource);
        }
    }
    return keys;
}

// Objet image d'une ressource que `known` ne contient pas ; nul sinon.
const core::ImageObject* newImageOn(const core::Page& page, const QStringList& known)
{
    for (const core::PageObject& object : page.objects) {
        const auto* image = std::get_if<core::ImageObject>(&core::body(object));
        if (image && !image->resource.empty() && !known.contains(QString::fromStdString(image->resource))) {
            return image;
        }
    }
    return nullptr;
}

QImage decodedResource(const DocumentController& doc, const std::string& key)
{
    QImage decoded;
    if (const core::Resource* resource = doc.document().resource(key)) {
        decoded.loadFromData(resource->data, "PNG");
    }
    return decoded;
}

const core::Page& shownPage(const DocumentController& doc)
{
    return doc.document().page(static_cast<std::size_t>(doc.currentIndex()));
}

// Identifiants de toutes les entrées des menus, sous-menus compris.
void collectMenuIds(const QVariantList& items, QStringList& ids)
{
    for (const QVariant& entry : items) {
        const QVariantMap map = entry.toMap();
        ids << map.value(QStringLiteral("id")).toString();
        collectMenuIds(map.value(QStringLiteral("items")).toList(), ids);
    }
}

QStringList cameraCommands()
{
    return {QStringLiteral("tool.camera-area"), QStringLiteral("tool.camera-polygon"),
            QStringLiteral("tool.camera-freehand"), QStringLiteral("tool.camera-window"),
            QStringLiteral("tool.camera-screen")};
}

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
class TstUiCamera : public QObject
{
    Q_OBJECT

private slots:
    // LE test qui garde l'invariant (décision 4 bis) : la sélection se fait dans une fenêtre à elle, créée à
    // la demande et sans parent (avec la principale pour parent, elle se réduirait avec elle) ; la palette
    // est un élément de CETTE fenêtre, pas une couverture de la vue principale. Chaque pointeur, appuyé et
    // glissé sur l'image figée, trace le rectangle de sélection ; un appui sur la palette ne le touche pas ;
    // la page principale ne reçoit rien. Témoin : un trait sur la vue principale, aux coordonnées de scène où
    // la palette se trouve dans sa fenêtre, est versé. Mutants : palette déclarée couverture de la vue
    // principale (patron des panneaux du lot, pageArea: presentation.pageArea) → le témoin au stylet est
    // arrêté ; sélection en surcouche de la fenêtre principale → aucune fenêtre de capture.
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
        QVERIFY(
            !presentationharness::findItem(main.window()->contentItem(), QStringLiteral("cameraPalette")));

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

    // Page affichée : la zone devient une ressource PNG du document et UN objet image, à la taille de la zone
    // (pixels logiques), centré dans la vue, en une étape ; la fenêtre se ferme. Mutants : ressource puis
    // objet en deux commandes (une annulation ne rend pas l'état d'avant) ; découpe ignorée (écran entier).
    void captureToPageAddsOneImage()
    {
        MainWindow main;
        QVERIFY(main.window());
        CameraController* camera = cameraOf(main);
        QVERIFY(camera);
        camera->setCaptureSource([] { return fakeScreen(); });
        DocumentController* open = openDocumentOf(main);
        QVERIFY(open);
        const QString before = open->fingerprintText();
        const int objects = main.pageObjectCount();
        const int pages = open->pageCount();
        const QStringList known = imageKeys(shownPage(*open));
        QVERIFY(!open->canUndo());
        QVERIFY(openCapture(main, camera, QStringLiteral("tool.camera-area")));
        camera->setSelection(straddlingZone());
        QVERIFY(camera->deliver(CameraController::CurrentPage));
        QTRY_VERIFY(!camera->window());
        QTRY_COMPARE(main.pageObjectCount(), objects + 1);
        QCOMPARE(open->pageCount(), pages);
        const core::ImageObject* image = newImageOn(shownPage(*open), known);
        QVERIFY(image);
        const QImage decoded = decodedResource(*open, image->resource);
        QCOMPARE(decoded.size(), QSize(100, 60));
        QCOMPARE(QColor(decoded.pixel(10, 30)), kLeftColor);
        QCOMPARE(QColor(decoded.pixel(90, 30)), kRightColor);
        QCOMPARE(image->bounds.width, 100.0);
        QCOMPARE(image->bounds.height, 60.0);
        actcraft::ui::PageView* view = main.pageView();
        const QPointF center =
            view->sceneToPage(view->mapToScene(QPointF(view->width() / 2, view->height() / 2)));
        QVERIFY(qAbs(image->bounds.x + image->bounds.width / 2 - center.x()) < 1);
        QVERIFY(qAbs(image->bounds.y + image->bounds.height / 2 - center.y()) < 1);
        // Une étape : annuler rend le document d'avant (ressource comprise).
        open->undo();
        QCOMPARE(open->fingerprintText(), before);
        QVERIFY(!open->canUndo());
    }

    // Nouvelle page : une page ajoutée en fin, affichée, qui porte l'image ; une étape. Mutant : image posée
    // sur la page affichée.
    void captureToNewPageAddsAPage()
    {
        MainWindow main;
        QVERIFY(main.window());
        CameraController* camera = cameraOf(main);
        QVERIFY(camera);
        camera->setCaptureSource([] { return fakeScreen(); });
        DocumentController* open = openDocumentOf(main);
        QVERIFY(open);
        const QString before = open->fingerprintText();
        const int pages = open->pageCount();
        const int shown = open->currentIndex();
        const std::size_t objectsBefore = shownPage(*open).objects.size();
        QVERIFY(openCapture(main, camera, QStringLiteral("tool.camera-area")));
        camera->setSelection(straddlingZone());
        QVERIFY(camera->deliver(CameraController::NewPage));
        QTRY_VERIFY(!camera->window());
        QCOMPARE(open->pageCount(), pages + 1);
        QTRY_COMPARE(open->currentIndex(), pages);
        const core::Page& added = open->document().page(static_cast<std::size_t>(pages));
        QCOMPARE(added.objects.size(), std::size_t{1});
        const core::ImageObject* image = newImageOn(added, {});
        QVERIFY(image);
        QCOMPARE(decodedResource(*open, image->resource).size(), QSize(100, 60));
        QCOMPARE(open->document().page(static_cast<std::size_t>(shown)).objects.size(), objectsBefore);
        open->undo();
        QCOMPARE(open->pageCount(), pages);
        QCOMPARE(open->fingerprintText(), before);
    }

    // Presse-papiers : l'image découpée, le document inchangé. Mutant : rien n'est copié.
    void captureToClipboard()
    {
        MainWindow main;
        QVERIFY(main.window());
        CameraController* camera = cameraOf(main);
        QVERIFY(camera);
        camera->setCaptureSource([] { return fakeScreen(); });
        DocumentController* open = openDocumentOf(main);
        QVERIFY(open);
        QClipboard* clipboard = QGuiApplication::clipboard();
        QVERIFY(clipboard);
        clipboard->clear();
        const QString before = open->fingerprintText();
        QVERIFY(openCapture(main, camera, QStringLiteral("tool.camera-area")));
        camera->setSelection(straddlingZone());
        QVERIFY(camera->deliver(CameraController::Clipboard));
        QTRY_VERIFY(!camera->window());
        const QImage copied = clipboard->image();
        QCOMPARE(copied.size(), QSize(100, 60));
        QCOMPARE(QColor(copied.pixel(10, 30)), kLeftColor);
        QCOMPARE(QColor(copied.pixel(90, 30)), kRightColor);
        QCOMPARE(open->fingerprintText(), before);
        QVERIFY(!open->canUndo());
    }

    // Absente d'Android (platforms de commands.json) : ni commande disponible, ni entrée de menu ; présente
    // sous Windows.
    void cameraAbsentOnAndroid()
    {
        QTemporaryDir profile;
        QVERIFY(profile.isValid());
        Commands commands;
        commands.setProfileDir(profile.path());
        commands.setPlatform(actcraft::actions::Platform::Windows);
        QStringList ids;
        collectMenuIds(commands.menus(), ids);
        for (const QString& id : cameraCommands()) {
            QVERIFY2(commands.isAvailable(id), qPrintable(id));
            QVERIFY2(ids.contains(id), qPrintable(id));
        }
        commands.setPlatform(actcraft::actions::Platform::Android);
        ids.clear();
        collectMenuIds(commands.menus(), ids);
        for (const QString& id : cameraCommands()) {
            QVERIFY2(!commands.isAvailable(id), qPrintable(id));
            QVERIFY2(!ids.contains(id), qPrintable(id));
            QVERIFY2(!commands.trigger(id), qPrintable(id));
        }
    }

    // Image vide (Wayland, macOS sans autorisation) : un message en français dans la fenêtre principale,
    // aucune fenêtre de sélection, page inchangée. Mutant : image vide non contrôlée (fenêtre ouverte).
    void emptyGrabShowsAMessage()
    {
        MainWindow main;
        QVERIFY(main.window());
        CameraController* camera = cameraOf(main);
        QVERIFY(camera);
        camera->setCaptureSource([] { return QImage(); });
        DocumentController* open = openDocumentOf(main);
        QVERIFY(open);
        const QString before = open->fingerprintText();
        QVERIFY(camera->message().isEmpty());
        QVERIFY(main.run(QStringLiteral("tool.camera-area")));
        QTRY_VERIFY(!camera->message().isEmpty());
        QVERIFY(camera->message().contains(QStringLiteral("écran")));
        QTest::qWait(100);
        QVERIFY(!camera->window());
        QQuickItem* box =
            presentationharness::findItem(main.window()->contentItem(), QStringLiteral("cameraMessage"));
        QVERIFY(box);
        QVERIFY(box->isVisible());
        // Écran entier : de même, rien n'est posé.
        const int objects = main.pageObjectCount();
        camera->clearMessage();
        QVERIFY(main.run(QStringLiteral("tool.camera-screen")));
        QTRY_VERIFY(!camera->message().isEmpty());
        QCOMPARE(main.pageObjectCount(), objects);
        QCOMPARE(open->fingerprintText(), before);
    }

    // P11 : la zone d'une capture partielle est gardée pour la suivante (posée, ou annulée). Mutant : zone
    // par défaut à chaque ouverture.
    void captureAreaKeptForTheNextCapture()
    {
        MainWindow main;
        QVERIFY(main.window());
        CameraController* camera = cameraOf(main);
        QVERIFY(camera);
        camera->setCaptureSource([] { return fakeScreen(); });
        QVERIFY(openCapture(main, camera, QStringLiteral("tool.camera-area")));
        const QRectF zone(120, 140, 200, 90);
        QVERIFY(zone != camera->selection());
        camera->setSelection(zone);
        QVERIFY(camera->deliver(CameraController::Clipboard));
        QTRY_VERIFY(!camera->window());
        QVERIFY(openCapture(main, camera, QStringLiteral("tool.camera-area")));
        QCOMPARE(camera->selection(), zone);
        const QRectF moved(60, 70, 150, 110);
        camera->setSelection(moved);
        camera->cancel();
        QVERIFY(!camera->window());
        QVERIFY(openCapture(main, camera, QStringLiteral("tool.camera-area")));
        QCOMPARE(camera->selection(), moved);
    }

    // Échap ferme la fenêtre de sélection (jamais ouverte sans moyen de sortie) ; rien n'est posé.
    void escapeClosesTheSelection()
    {
        MainWindow main;
        QVERIFY(main.window());
        CameraController* camera = cameraOf(main);
        QVERIFY(camera);
        camera->setCaptureSource([] { return fakeScreen(); });
        const int objects = main.pageObjectCount();
        QQuickWindow* window = openCapture(main, camera, QStringLiteral("tool.camera-area"));
        QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowActive(window));
        const QPointer<QQuickWindow> guard(window);
        QTest::keyClick(window, Qt::Key_Escape);
        QTRY_VERIFY(!camera->window());
        QTRY_VERIFY(guard.isNull());
        QCOMPARE(main.pageObjectCount(), objects);
    }

    // Échap ferme la sélection quel que soit le focus dans sa fenêtre : une case de la palette qui a pris le
    // focus au clic ne laisse pas Échap au raccourci de tool.select (Main.qml, Qt.ApplicationShortcut).
    // Mutant : Échap traité par la seule surface (focus initial) → tool.select déclenché, fenêtre ouverte.
    void escapeClosesTheSelectionFromThePalette()
    {
        MainWindow main;
        QVERIFY(main.window());
        CameraController* camera = cameraOf(main);
        QVERIFY(camera);
        camera->setCaptureSource([] { return fakeScreen(); });
        QObject* root = main.window()->findChild<QObject*>(QStringLiteral("cameraRoot"));
        QVERIFY(root);
        auto* commands = qobject_cast<Commands*>(root->property("commands").value<QObject*>());
        QVERIFY(commands);
        const int objects = main.pageObjectCount();
        QQuickWindow* window = openCapture(main, camera, QStringLiteral("tool.camera-area"));
        QVERIFY(window);
        QVERIFY(QTest::qWaitForWindowActive(window));
        QQuickItem* field =
            presentationharness::findItem(window->contentItem(), QStringLiteral("cameraZoneLeft"));
        QVERIFY(field);
        QTRY_VERIFY(field->isVisible() && field->width() > 0);
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier,
                          field->mapToScene(QPointF(field->width() / 2, field->height() / 2)).toPoint());
        QQuickItem* focused = window->activeFocusItem();
        QVERIFY2(focused && (focused == field || field->isAncestorOf(focused)),
                 qPrintable(focused ? QString::fromLatin1(focused->metaObject()->className())
                                    : QStringLiteral("aucun élément")));
        const QPointer<QQuickWindow> guard(window);
        QSignalSpy triggered(commands, &Commands::triggered);
        QTest::keyClick(window, Qt::Key_Escape);
        QStringList ids;
        for (qsizetype i = 0; i < triggered.count(); ++i) {
            ids << triggered.at(i).at(0).toString();
        }
        QVERIFY2(!ids.contains(QStringLiteral("tool.select")), qPrintable(ids.join(QStringLiteral(", "))));
        QTRY_VERIFY(!camera->window());
        QTRY_VERIFY(guard.isNull());
        QCOMPARE(main.pageObjectCount(), objects);
    }

    // Écran entier : capture immédiate, posée sur la page affichée, sans fenêtre de sélection.
    void screenCaptureGoesStraightToThePage()
    {
        MainWindow main;
        QVERIFY(main.window());
        CameraController* camera = cameraOf(main);
        QVERIFY(camera);
        camera->setCaptureSource([] { return fakeScreen(); });
        DocumentController* open = openDocumentOf(main);
        QVERIFY(open);
        const int objects = main.pageObjectCount();
        const QStringList known = imageKeys(shownPage(*open));
        QVERIFY(main.run(QStringLiteral("tool.camera-screen")));
        QTRY_COMPARE(main.pageObjectCount(), objects + 1);
        QVERIFY(!camera->window());
        const core::ImageObject* image = newImageOn(shownPage(*open), known);
        QVERIFY(image);
        QCOMPARE(decodedResource(*open, image->resource).size(), fakeScreen().size());
    }

    // Écran entier sans document où poser l'image : un message dans la fenêtre principale, comme l'image
    // vide, au lieu de rien. Mutant : échec de la pose ignoré → aucun message.
    void screenCaptureWithoutDocumentShowsAMessage()
    {
        MainWindow main;
        QVERIFY(main.window());
        CameraController* camera = cameraOf(main);
        QVERIFY(camera);
        camera->setCaptureSource([] { return fakeScreen(); });
        camera->setDocumentController(nullptr);
        const int objects = main.pageObjectCount();
        QVERIFY(camera->message().isEmpty());
        QVERIFY(main.run(QStringLiteral("tool.camera-screen")));
        QTRY_VERIFY(!camera->message().isEmpty());
        QVERIFY(!camera->window());
        QVERIFY(camera->frozenImage().isNull());
        QCOMPARE(main.pageObjectCount(), objects);
        QQuickItem* box =
            presentationharness::findItem(main.window()->contentItem(), QStringLiteral("cameraMessage"));
        QVERIFY(box);
        QVERIFY(box->isVisible());
    }

    // Point par point : un clic près du premier sommet ferme ; hors du polygone, l'image est transparente.
    void polygonCaptureIsTransparentOutside()
    {
        MainWindow main;
        QVERIFY(main.window());
        CameraController* camera = cameraOf(main);
        QVERIFY(camera);
        camera->setCaptureSource([] { return fakeScreen(); });
        QVERIFY(openCapture(main, camera, QStringLiteral("tool.camera-polygon")));
        for (const QPointF& vertex :
             {QPointF(100, 100), QPointF(300, 100), QPointF(300, 260), QPointF(104, 102)}) {
            camera->press(vertex);
            camera->release(vertex);
        }
        QVERIFY(camera->isReady());
        QVERIFY(camera->deliver(CameraController::Clipboard));
        const QImage copied = QGuiApplication::clipboard()->image();
        QCOMPARE(copied.size(), QSize(200, 160));
        QCOMPARE(qAlpha(copied.pixel(2, 157)), 0);
        QCOMPARE(QColor(copied.pixel(190, 20)).rgb(), kLeftColor.rgb());
    }

    // Fenêtre : la plus haute sous le pointeur est encadrée ; hors de toute fenêtre, l'écran entier.
    void windowCaptureFramesTheWindowUnderThePointer()
    {
        MainWindow main;
        QVERIFY(main.window());
        CameraController* camera = cameraOf(main);
        QVERIFY(camera);
        camera->setCaptureSource([] { return fakeScreen(); });
        const QRect screen = QGuiApplication::primaryScreen()->geometry();
        const QPoint origin = screen.topLeft();
        camera->setWindowSource([origin] {
            return std::vector<QRect>{QRect(origin + QPoint(50, 60), QSize(300, 200)),
                                      QRect(origin, QSize(700, 500))};
        });
        QVERIFY(openCapture(main, camera, QStringLiteral("tool.camera-window")));
        camera->hover(QPointF(100, 100));
        QCOMPARE(camera->selection(), QRectF(50, 60, 300, 200));
        camera->hover(QPointF(600, 400));
        QCOMPARE(camera->selection(), QRectF(0, 0, 700, 500));
        camera->hover(QPointF(screen.width() - 5, screen.height() - 5));
        QCOMPARE(camera->selection(), QRectF(QPointF(0, 0), QSizeF(screen.size())));
        camera->press(QPointF(100, 100));
        camera->release(QPointF(100, 100));
        QVERIFY(camera->deliver(CameraController::Clipboard));
        QCOMPARE(QGuiApplication::clipboard()->image().size(), QSize(300, 200));
    }

    // Fenêtre : le clic fige la fenêtre choisie ; le survol qui suit (le pointeur monte vers la palette en
    // passant sur une autre fenêtre, ou hors de toute fenêtre) ne la change plus. Mutant : survol qui réécrit
    // la sélection après le clic → l'autre fenêtre (ou l'écran entier) est posée.
    void windowCaptureKeepsTheClickedWindow()
    {
        MainWindow main;
        QVERIFY(main.window());
        CameraController* camera = cameraOf(main);
        QVERIFY(camera);
        camera->setCaptureSource([] { return fakeScreen(); });
        const QRect screen = QGuiApplication::primaryScreen()->geometry();
        const QPoint origin = screen.topLeft();
        camera->setWindowSource([origin] {
            return std::vector<QRect>{QRect(origin + QPoint(50, 60), QSize(300, 200)),
                                      QRect(origin + QPoint(400, 300), QSize(250, 150))};
        });
        QVERIFY(openCapture(main, camera, QStringLiteral("tool.camera-window")));
        const QRectF chosen(50, 60, 300, 200);
        camera->hover(QPointF(100, 100));
        camera->press(QPointF(100, 100));
        camera->release(QPointF(100, 100));
        QCOMPARE(camera->selection(), chosen);
        camera->hover(QPointF(450, 350)); // l'autre fenêtre
        QCOMPARE(camera->selection(), chosen);
        camera->hover(QPointF(screen.width() - 5, screen.height() - 5)); // hors de toute fenêtre
        QCOMPARE(camera->selection(), chosen);
        QVERIFY(camera->deliver(CameraController::Clipboard));
        QCOMPARE(QGuiApplication::clipboard()->image().size(), QSize(300, 200));
    }

    // Liste des fenêtres du bureau (Windows) : une fenêtre native de test y figure, à sa place. Les fenêtres
    // Qt hors écran n'ont pas de fenêtre native : la fenêtre de test est créée par Windows, sans cadre,
    // invisible à l'œil (calque d'opacité nulle), en pixels physiques ; hors écran, l'écran de Qt a un
    // rapport 1, pixels logiques et physiques se confondent. Ailleurs : liste vide (capture de l'écran
    // entier).
    void topLevelWindowsListsOurWindow()
    {
#ifdef _WIN32
        // Exclues : une fenêtre transparente aux clics (WS_EX_TRANSPARENT : surcouche d'un outil
        // d'enregistrement, d'un pilote), qui masquerait toutes les autres dans la capture d'une fenêtre, et
        // une fenêtre masquée par le compositeur (DWMWA_CLOAK). Mutants : l'un ou l'autre filtre retiré.
        const QRect expected(123, 145, 234, 156);
        const QRect clickThrough(321, 247, 211, 133);
        const QRect cloaked(517, 389, 187, 121);
        const DPI_AWARENESS_CONTEXT previous =
            SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        const auto create = [](const QRect& r, DWORD extra) {
            return CreateWindowExW(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | extra, L"STATIC",
                                   L"", WS_POPUP, r.x(), r.y(), r.width(), r.height(), nullptr, nullptr,
                                   GetModuleHandleW(nullptr), nullptr);
        };
        HWND window = create(expected, 0);
        HWND transparent = create(clickThrough, WS_EX_TRANSPARENT);
        HWND hidden = create(cloaked, 0);
        if (previous) {
            SetThreadDpiAwarenessContext(previous);
        }
        std::vector<QRect> listed;
        bool cloakSet = false;
        if (window && transparent && hidden) {
            const BOOL cloak = TRUE;
            cloakSet = SUCCEEDED(DwmSetWindowAttribute(hidden, DWMWA_CLOAK, &cloak, sizeof(cloak)));
            for (HWND each : {window, transparent, hidden}) {
                SetLayeredWindowAttributes(each, 0, 0, LWA_ALPHA);
                ShowWindow(each, SW_SHOWNOACTIVATE);
            }
            listed = actcraft::features::topLevelWindows();
        }
        for (HWND each : {window, transparent, hidden}) {
            if (each) {
                DestroyWindow(each);
            }
        }
        QVERIFY(window && transparent && hidden);
        QVERIFY(cloakSet);
        const auto listedHas = [&listed](const QRect& r) {
            return std::find(listed.begin(), listed.end(), r) != listed.end();
        };
        QVERIFY2(listedHas(expected), qPrintable(QStringLiteral("%1 fenêtres listées").arg(listed.size())));
        QVERIFY2(!listedHas(clickThrough), "fenêtre transparente aux clics listée");
        QVERIFY2(!listedHas(cloaked), "fenêtre masquée par le compositeur listée");
#else
        QVERIFY(actcraft::features::topLevelWindows().empty());
#endif
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("camera", TstUiCamera)
#include "tst_ui_camera.moc"
