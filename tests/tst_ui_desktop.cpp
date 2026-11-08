#include "desktopcontroller.h"
#include "presentationdocument.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QColor>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QImage>
#include <QJSEngine>
#include <QKeySequence>
#include <QPointer>
#include <QQuickWindow>
#include <QScreen>
#include <QSignalSpy>
#include <QSurfaceFormat>
#include <QTemporaryDir>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>
#include <QtTest>

#ifdef Q_OS_WIN
#include "calculatorcontroller.h" // après les en-têtes Qt : Q_OS_WIN y est défini
#endif

#include <cstddef>
#include <vector>

namespace {

using actcraft::ui::DesktopController;
using actcraft::ui::DocumentController;
using presentationharness::MainWindow;
using presentationharness::Pointer;

// Contrôleur porté par la racine `desktopRoot` de Presentation.qml (QtObject : cherché par l'arbre des
// objets).
DesktopController* desktopOf(const MainWindow& main)
{
    if (!main.window()) {
        return nullptr;
    }
    QObject* root = main.window()->findChild<QObject*>(QStringLiteral("desktopRoot"));
    if (!root) {
        return nullptr;
    }
    return qobject_cast<DesktopController*>(root->property("controller").value<QObject*>());
}

// Document ouvert de la fenêtre principale (celui de la vue principale).
DocumentController* openDocumentOf(const MainWindow& main)
{
    return main.pageView() ? main.pageView()->documentController() : nullptr;
}

const actcraft::core::Page& shownPage(const DocumentController& doc)
{
    return doc.document().page(static_cast<std::size_t>(doc.currentIndex()));
}

// Fond uni d'alpha nul (le fond par défaut d'une page est blanc opaque, solidFill(0xFFFFFFFF)).
bool isTransparentFill(const actcraft::core::Fill& fill)
{
    return fill.kind == actcraft::core::FillKind::Solid && (fill.color & 0xFF000000u) == 0;
}

// Fenêtre d'annotation ouverte, page prête, cache à jour : un trait peut y être injecté.
bool waitDesktopReady(DesktopController* desktop)
{
    if (!desktop || !desktop->window()) {
        return false;
    }
    QQuickWindow* window = desktop->window();
    if (!QTest::qWaitForWindowExposed(window)) {
        return false;
    }
    auto* view = window->findChild<actcraft::ui::PageView*>(QStringLiteral("pageView"));
    auto* fixed = window->findChild<actcraft::render::FixedPageItem*>(QStringLiteral("fixedPage"));
    return view && fixed &&
           QTest::qWaitFor([view, fixed] { return fixed->isReady() && view->cacheReady(); }, 10000);
}

QImage fakeDesktop()
{
    QImage image(64, 48, QImage::Format_ARGB32);
    image.fill(QColor(20, 120, 200));
    return image;
}

// Capture du bureau par la commande, attendue jusqu'à sa fin (M4 : asynchrone) ; vrai si le fond est posé.
bool captureDesktop(MainWindow& main, DesktopController* desktop)
{
    QSignalSpy finished(desktop, &DesktopController::backgroundCaptured);
    // Fin comptée même si elle vient dans la commande : seul captureDoesNotBlockTheMainThread garde
    // l'asynchrone.
    return main.run(QStringLiteral("page.background-desktop-capture")) &&
           (!finished.isEmpty() || finished.wait(5000)) && finished.first().at(0).toBool();
}

// Trait au milieu de la fenêtre d'annotation (la page couvre l'écran principal).
void strokeInDesktopWindow(DesktopController* desktop, Pointer pointer)
{
    QQuickWindow* window = desktop->window();
    const QSizeF size = window->size();
    presentationharness::strokeAt(window, pointer, QPointF(size.width() * 0.35, size.height() * 0.45),
                                  QPointF(size.width() * 0.6, size.height() * 0.55));
}

// Trait au stylet commencé au milieu de la fenêtre d'annotation, pas encore relâché ; rend le point du
// relâché.
QPointF beginStylusStroke(QQuickWindow* window)
{
    const QSizeF size = window->size();
    const QPointF from(size.width() * 0.35, size.height() * 0.45);
    const QPointF to(size.width() * 0.6, size.height() * 0.55);
    presentationharness::sendTablet(window, QEvent::TabletPress, from);
    for (int i = 1; i <= 8; ++i) {
        presentationharness::sendTablet(window, QEvent::TabletMove, from + (to - from) * (i / 8.0));
    }
    QCoreApplication::processEvents();
    return to;
}

std::size_t objectsOn(const DocumentController& doc, std::size_t page)
{
    return doc.document().page(page).objects.size();
}

// Fenêtre de premier niveau nommée (roue, sortie du passage des clics : créées sans parent par la racine).
QQuickWindow* windowNamed(const QString& name)
{
    const QWindowList windows = QGuiApplication::allWindows();
    for (QWindow* window : windows) {
        if (window->objectName() == name) {
            return qobject_cast<QQuickWindow*>(window);
        }
    }
    return nullptr;
}

QQuickWindow* wheelWindow()
{
    return windowNamed(QStringLiteral("desktopToolsWheel"));
}

// Roue ouverte (repliée) : déploie l'icône, ouvre l'anneau qui porte `command`, puis clique cette entrée.
bool clickRingEntry(DesktopController* desktop, QQuickWindow* wheel, const QString& command)
{
    QVariantList entries;
    if (!QMetaObject::invokeMethod(desktop, "wheelEntries", Q_RETURN_ARG(QVariantList, entries),
                                   Q_ARG(double, 110.0), Q_ARG(double, 205.0))) {
        return false;
    }
    int ringIndex = -1;
    for (qsizetype i = 0; i < entries.size(); ++i) {
        for (const QVariant& inner : entries[i].toMap().value(QStringLiteral("ring")).toList()) {
            if (inner.toMap().value(QStringLiteral("command")).toString() == command) {
                ringIndex = static_cast<int>(i);
            }
        }
    }
    const auto click = [wheel](const QString& name) {
        const auto find = [wheel, &name] {
            return presentationharness::findItem(wheel->contentItem(), name);
        };
        return QTest::qWaitFor([&find] { return find() != nullptr; }) &&
               QMetaObject::invokeMethod(find(), "click");
    };
    return ringIndex >= 0 && click(QStringLiteral("desktopWheelIcon")) &&
           click(QStringLiteral("desktopWheelRing_%1").arg(ringIndex)) &&
           click(QStringLiteral("desktopWheelEntry_") + command);
}

// Ce qu'un outil montre : la racine de son panneau, ou l'élément visuel de sa fenêtre surgissante (dialogue
// du message défilant). Cherché dans l'arbre de la fenêtre principale seulement.
QQuickItem* shownItemOf(const MainWindow& main, const QString& name)
{
    QObject* object = main.window() ? main.window()->findChild<QObject*>(name) : nullptr;
    if (auto* item = qobject_cast<QQuickItem*>(object)) {
        return item;
    }
    auto* content = object ? object->property("contentItem").value<QQuickItem*>() : nullptr;
    return content ? content->parentItem() : nullptr;
}

// Ma1 : affiché ENTIER dans sa fenêtre, à une taille non nulle. isVisible() seul ne verrait ni un panneau
// rogné ni un panneau hors de la fenêtre.
bool shownWholeIn(const QQuickItem* item, const QQuickWindow* window)
{
    if (!item || !window || !item->isVisible() || item->window() != window || item->width() <= 0 ||
        item->height() <= 0) {
        return false;
    }
    const QRectF inScene = item->mapRectToScene(QRectF(0, 0, item->width(), item->height()));
    return QRectF(QPointF(0, 0), QSizeF(window->size())).contains(inScene);
}

QString placeOf(const QQuickItem* item, const QQuickWindow* window)
{
    if (!item || !window) {
        return QStringLiteral("élément ou fenêtre absent");
    }
    const QRectF r = item->mapRectToScene(QRectF(0, 0, item->width(), item->height()));
    return QStringLiteral("élément (%1, %2, %3 x %4) visible=%5 ; fenêtre %6 x %7")
        .arg(r.x())
        .arg(r.y())
        .arg(r.width())
        .arg(r.height())
        .arg(item->isVisible())
        .arg(window->width())
        .arg(window->height());
}

// Place faite pour un outil de la fenêtre principale (annotation fermée, fenêtre plus réduite) : attente
// bornée. Une attente QTRY_* par défaut qui échoue dure 15 s (5 s, puis 10 s de diagnostic) : sous le mutant
// « clé mainWindow non lue », 24 lignes Ma1/M8 en échec dépassaient le délai de 300 s de ctest, qui semblait
// pendre (essai 3). Ici, une ligne en échec dure 6 s.
constexpr int kRoomMadeMs = 2000;

// Ma1 : annotation ouverte, la fenêtre principale n'est plus que la palette. `before` : sa géométrie d'avant.
void openAnnotationWithPaletteOnly(MainWindow& main, QRect& before)
{
    QQuickWindow* window = main.window();
    QVERIFY(window);
    DesktopController* desktop = desktopOf(main);
    QVERIFY(desktop);
    QVERIFY(main.waitReady());
    before = window->geometry();
    QVERIFY(main.run(QStringLiteral("desktop.annotate")));
    QVERIFY(waitDesktopReady(desktop));
    QTRY_VERIFY(window->geometry().width() < before.width()); // la palette seule
}

// Ma1 : le panneau `shown` s'est ouvert ENTIER dans la fenêtre principale, l'annotation fermée et la fenêtre
// revenue à sa géométrie d'avant l'annotation.
void expectWholeInTheMainWindow(MainWindow& main, const QString& shown, const QRect& before)
{
    QQuickWindow* window = main.window();
    QQuickItem* item = shownItemOf(main, shown);
    QVERIFY2(item, qPrintable(shown));
    QTRY_VERIFY_WITH_TIMEOUT(!desktopOf(main)->isAnnotating(), kRoomMadeMs);
    QTRY_VERIFY2(item->isVisible(), qPrintable(shown + QStringLiteral(" jamais montré")));
    QTRY_VERIFY2(shownWholeIn(item, window), qPrintable(placeOf(item, window)));
    QTRY_COMPARE(window->geometry(), before);
}

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
class TstUiDesktop : public QObject
{
    Q_OBJECT

private slots:
    // Fenêtre créée par le contrôleur : sans cadre, toujours devant, couleur transparente, surface à canal
    // alpha demandée avant sa création native. Le format explicite (setFormat, demandé par 7b) double
    // setColor(Qt::transparent), qui demande déjà ce canal : retirer setFormat est un mutant équivalent ;
    // seule la vue à l'écran en D3D11 prouve la transparence. Mutant : drapeau « toujours devant » retiré.
    void annotationWindowIsTransparentAndAbove()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(!desktop->window());
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(desktop->isAnnotating());
        QVERIFY(waitDesktopReady(desktop));
        QQuickWindow* window = desktop->window();
        QVERIFY(window != main.window());
        QVERIFY(window->flags().testFlag(Qt::FramelessWindowHint));
        QVERIFY(window->flags().testFlag(Qt::WindowStaysOnTopHint));
        QVERIFY(!window->flags().testFlag(Qt::WindowTransparentForInput));
        QCOMPARE(window->color().alpha(), 0);
        QCOMPARE(window->requestedFormat().alphaBufferSize(), 8);
        QVERIFY(window->isVisible());
        // Désactiver : la fenêtre est fermée et détruite.
        QPointer<QQuickWindow> guard(window);
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(!desktop->isAnnotating());
        QVERIFY(!desktop->window());
        QTRY_VERIFY(guard.isNull());
    }

    // LE test qui garde l'invariant : la zone de la fenêtre d'annotation écrit dans le paperboard du bureau,
    // jamais dans le document ouvert que Presentation.qml passe à la racine (documentController). Mutant :
    // PageArea { documentController: root.documentController } (patron des autres racines) → le trait part
    // dans le document ouvert.
    void strokeGoesToTheDesktopPaperboard()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        DocumentController* open = openDocumentOf(main);
        QVERIFY(open);
        QVERIFY(desktop->desktopDocument());
        QVERIFY(desktop->desktopDocument() != open);
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        auto* view = desktop->window()->findChild<actcraft::ui::PageView*>(QStringLiteral("pageView"));
        QVERIFY(view);
        QCOMPARE(view->documentController(), desktop->desktopDocument());
        QVERIFY(view->tools() == nullptr);
        const QString openBefore = open->fingerprintText();
        const int openObjects = open->shownObjectCount();
        QCOMPARE(desktop->desktopDocument()->shownObjectCount(), 0);
        strokeInDesktopWindow(desktop, Pointer::Mouse);
        QTRY_COMPARE(desktop->desktopDocument()->shownObjectCount(), 1);
        QCOMPARE(open->shownObjectCount(), openObjects);
        QCOMPARE(open->fingerprintText(), openBefore);
        QCOMPARE(main.pageObjectCount(), openObjects);
        QVERIFY(!open->isModified());
    }

    // Laisser passer les clics : la fenêtre devient transparente aux entrées, et le redevient pas à
    // l'inverse. Mutant : le drapeau n'est jamais posé.
    void passThroughMakesTheWindowTransparentForInput()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        QQuickWindow* window = desktop->window();
        QVERIFY(!window->flags().testFlag(Qt::WindowTransparentForInput));
        desktop->setPassThrough(true);
        QVERIFY(desktop->passThrough());
        QVERIFY(window->flags().testFlag(Qt::WindowTransparentForInput));
        QVERIFY(window->flags().testFlag(Qt::WindowStaysOnTopHint));
        desktop->setPassThrough(false);
        QVERIFY(!window->flags().testFlag(Qt::WindowTransparentForInput));
        // Le bouton de la fenêtre fait la même chose.
        QObject* button = window->findChild<QObject*>(QStringLiteral("desktopPassThroughButton"));
        QVERIFY(button);
        QVERIFY(QMetaObject::invokeMethod(button, "click"));
        QVERIFY(desktop->passThrough());
        QVERIFY(window->flags().testFlag(Qt::WindowTransparentForInput));
    }

    // Capture du bureau (F-071) : image figée en fond de la page affichée du document ouvert, ressource PNG
    // du document, en une étape. Mutant : deux commandes (ressource puis fond) → une annulation ne rend pas
    // l'état d'avant.
    void desktopCaptureBecomesTheBackground()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        DocumentController* open = openDocumentOf(main);
        QVERIFY(open);
        desktop->setCaptureSource([] { return fakeDesktop(); });
        const QString before = open->fingerprintText();
        QVERIFY(shownPage(*open).background.image.empty());
        QVERIFY(!open->canUndo());
        QVERIFY(captureDesktop(main, desktop));
        const std::string key = shownPage(*open).background.image;
        QVERIFY(!key.empty());
        const actcraft::core::Resource* resource = open->document().resource(key);
        QVERIFY(resource);
        const QByteArray bytes = resource->data;
        QVERIFY(bytes.startsWith(QByteArray("\x89PNG\r\n\x1a\n", 8)));
        QImage decoded;
        QVERIFY(decoded.loadFromData(bytes, "PNG"));
        QCOMPARE(decoded.size(), QSize(64, 48));
        QVERIFY(open->fingerprintText() != before);
        QVERIFY(open->canUndo());
        open->undo();
        QCOMPARE(open->fingerprintText(), before);
        QVERIFY(!open->canUndo());
    }

    // Superposition du bureau : état de la vue, jamais écrit dans le document ; enregistrer écrit le fond
    // d'origine (ici la capture). Mutant : la superposition exécute un fond transparent dans le document.
    void liveDesktopBackgroundIsNotSaved()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        desktop->setCaptureSource([] { return fakeDesktop(); });
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        DocumentController* paperboard = desktop->desktopDocument();
        QVERIFY(captureDesktop(main, desktop));
        QVERIFY(!shownPage(*paperboard).background.image.empty());
        const QString captured = paperboard->fingerprintText();
        const bool undoBefore = paperboard->canUndo();
        QVERIFY(!desktop->liveDesktopBackground());
        QVERIFY(main.run(QStringLiteral("page.background-desktop-overlay")));
        QVERIFY(desktop->liveDesktopBackground());
        QCOMPARE(paperboard->fingerprintText(), captured);
        QCOMPARE(paperboard->canUndo(), undoBefore);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QUrl url = QUrl::fromLocalFile(dir.filePath(QStringLiteral("bureau.actcraft")));
        QSignalSpy saved(paperboard, &DocumentController::saveFinished);
        paperboard->saveToUrl(url);
        QVERIFY(saved.wait(10000));
        QVERIFY(saved.first().at(0).toBool());
        DocumentController reopened;
        QSignalSpy loaded(&reopened, &DocumentController::loadFinished);
        reopened.openUrl(url);
        QVERIFY(loaded.wait(10000));
        QVERIFY(loaded.first().at(0).toBool());
        QCOMPARE(reopened.fingerprintText(), captured);
        QVERIFY(!shownPage(reopened).background.image.empty());
    }

    // P13 : page suivante du paperboard du bureau = nouvelle page transparente vierge, même après une page à
    // fond capturé, en une étape. Mutant : DocumentController::nextPage (F-125, page du fond par défaut).
    void nextPageIsANewTransparentPage()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        desktop->setCaptureSource([] { return fakeDesktop(); });
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        DocumentController* paperboard = desktop->desktopDocument();
        QCOMPARE(paperboard->pageCount(), 1);
        QVERIFY(isTransparentFill(shownPage(*paperboard).background.fill));
        QVERIFY(captureDesktop(main, desktop));
        QVERIFY(!shownPage(*paperboard).background.image.empty());
        const QSizeF size = paperboard->pageSize();
        const QString captured = paperboard->fingerprintText();
        desktop->nextPage();
        QCOMPARE(paperboard->pageCount(), 2);
        QCOMPARE(paperboard->currentIndex(), 1);
        QCOMPARE(paperboard->pageSize(), size);
        QVERIFY(isTransparentFill(shownPage(*paperboard).background.fill));
        QVERIFY(shownPage(*paperboard).background.image.empty());
        QVERIFY(shownPage(*paperboard).objects.empty());
        paperboard->undo();
        QCOMPARE(paperboard->pageCount(), 1);
        QCOMPARE(paperboard->fingerprintText(), captured);
    }

    // P14 : le paperboard du bureau survit à la fermeture de la fenêtre et revient à l'activation suivante.
    // Mutant : un document neuf à chaque activation.
    void desktopPaperboardKeptBetweenActivations()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        DocumentController* paperboard = desktop->desktopDocument();
        strokeInDesktopWindow(desktop, Pointer::Mouse);
        QTRY_COMPARE(paperboard->shownObjectCount(), 1);
        const QString drawn = paperboard->fingerprintText();
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(!desktop->isAnnotating());
        QVERIFY(!desktop->window());
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(desktop->isAnnotating());
        QVERIFY(waitDesktopReady(desktop));
        QCOMPARE(desktop->desktopDocument(), paperboard);
        QCOMPARE(paperboard->fingerprintText(), drawn);
        QCOMPARE(paperboard->shownObjectCount(), 1);
        auto* view = desktop->window()->findChild<actcraft::ui::PageView*>(QStringLiteral("pageView"));
        QVERIFY(view);
        QCOMPARE(view->documentController(), paperboard);
    }

    // P22 : la superposition ne vaut que pour la page qui la porte. Mutant : un booléen global.
    void onlyItsPageSeesTheDesktop()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        DocumentController* paperboard = desktop->desktopDocument();
        QVERIFY(main.run(QStringLiteral("page.background-desktop-overlay")));
        QVERIFY(desktop->liveDesktopBackground());
        desktop->nextPage();
        QCOMPARE(paperboard->currentIndex(), 1);
        QVERIFY(!desktop->liveDesktopBackground());
        paperboard->pageBack();
        QCOMPARE(paperboard->currentIndex(), 0);
        QVERIFY(desktop->liveDesktopBackground());
        // Commande cochable : la décocher retire la superposition de cette page seulement.
        QVERIFY(main.run(QStringLiteral("page.background-desktop-overlay")));
        QVERIFY(!desktop->liveDesktopBackground());
    }

    // Chaque pointeur : le trait sur la fenêtre de bureau est versé dans le paperboard du bureau, la page
    // principale reste inchangée ; un trait commencé sur la barre de la fenêtre n'est pas versé. Le stylet
    // est pris par le filtre de la fenêtre avant tout élément : seule la couverture déclarée par la racine
    // l'arrête (témoin : couverture retirée, le même trait est versé). La souris et le doigt vont d'abord à
    // la MouseArea de la barre, couverture ou non. Mutants : documentController de la racine (document
    // ouvert) ; barre jamais déclarée couverture (M9).
    void desktopWindowTakesEveryPointer_data() { presentationharness::addPointerRows(); }
    void desktopWindowTakesEveryPointer()
    {
        QFETCH(Pointer, pointer);
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        DocumentController* open = openDocumentOf(main);
        QVERIFY(open);
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        QQuickWindow* window = desktop->window();
        DocumentController* paperboard = desktop->desktopDocument();
        auto* view = window->findChild<actcraft::ui::PageView*>(QStringLiteral("pageView"));
        QVERIFY(view);
        const QString openBefore = open->fingerprintText();
        const int mainObjects = main.pageObjectCount();
        QQuickItem* toolbar =
            presentationharness::findItem(window->contentItem(), QStringLiteral("desktopToolbar"));
        QVERIFY(toolbar);
        // Appui dans la marge haute de la barre, près de son bord droit : hors écran (800 x 800), la barre,
        // plus large que l'écran, déborde à gauche et son bord gauche est hors de la fenêtre (cause de M9
        // survivant).
        const QPointF start = toolbar->mapToScene(QPointF(toolbar->width() - 40, 3));
        const QPointF end = toolbar->mapToScene(QPointF(toolbar->width() - 200, -150));
        // L'appui est sur la vue : sans couverture, le filtre du stylet le prendrait.
        const QRectF viewRect = view->mapRectToScene(view->boundingRect());
        QVERIFY2(view->contains(view->mapFromScene(start)),
                 qPrintable(QStringLiteral("appui (%1, %2) hors de la vue (%3, %4, %5 x %6), fenêtre %7 x %8")
                                .arg(start.x())
                                .arg(start.y())
                                .arg(viewRect.x())
                                .arg(viewRect.y())
                                .arg(viewRect.width())
                                .arg(viewRect.height())
                                .arg(window->width())
                                .arg(window->height())));
        presentationharness::strokeAt(window, pointer, start, end);
        QTest::qWait(50);
        QCOMPARE(paperboard->shownObjectCount(), 0);
        // Témoin : couverture retirée, le stylet verse le même trait ; la souris et le doigt, non.
        const int witness = pointer == Pointer::Stylus ? 1 : 0;
        view->removeInputCover(toolbar);
        presentationharness::strokeAt(window, pointer, start, end);
        QTRY_COMPARE(paperboard->shownObjectCount(), witness);
        view->addInputCover(toolbar);
        QVERIFY(!desktop->passThrough());
        QVERIFY(desktop->isAnnotating());
        strokeInDesktopWindow(desktop, pointer);
        QTRY_COMPARE(paperboard->shownObjectCount(), witness + 1);
        // Plus rien en route (cache à jour) : aucun trait versé en retard, les affirmations négatives
        // tiennent.
        QVERIFY(waitDesktopReady(desktop));
        QCOMPARE(paperboard->shownObjectCount(), witness + 1);
        QCOMPARE(open->fingerprintText(), openBefore);
        QCOMPARE(main.pageObjectCount(), mainObjects);
    }

    // F2 : un trait en cours est versé sur sa page avant tout changement de page du paperboard du bureau
    // (comme Main.qml pour le document ouvert : pageAboutToChange → finishStroke), par la barre comme par le
    // paperboard lui-même. Mutant : rien n'écoute pageAboutToChange.
    void strokeInProgressEndsBeforePageChange()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        QQuickWindow* window = desktop->window();
        DocumentController* paperboard = desktop->desktopDocument();
        QPointF release = beginStylusStroke(window);
        QCOMPARE(objectsOn(*paperboard, 0), std::size_t{0}); // en cours : rien de versé
        desktop->nextPage();
        QCOMPARE(paperboard->currentIndex(), 1);
        QCOMPARE(objectsOn(*paperboard, 0), std::size_t{1});
        presentationharness::sendTablet(window, QEvent::TabletRelease, release);
        QCoreApplication::processEvents();
        QCOMPARE(objectsOn(*paperboard, 1), std::size_t{0});
        QCOMPARE(objectsOn(*paperboard, 0), std::size_t{1});
        // Retour par le paperboard (DocumentController::pageBack émet pageAboutToChange).
        QVERIFY(waitDesktopReady(desktop));
        release = beginStylusStroke(window);
        paperboard->pageBack();
        QCOMPARE(paperboard->currentIndex(), 0);
        QCOMPARE(objectsOn(*paperboard, 1), std::size_t{1});
        presentationharness::sendTablet(window, QEvent::TabletRelease, release);
        QCoreApplication::processEvents();
        QCOMPARE(objectsOn(*paperboard, 0), std::size_t{1});
        QCOMPARE(objectsOn(*paperboard, 1), std::size_t{1});
    }

    // F2 : fermer la fenêtre d'annotation pendant un trait le verse avant de détruire la vue. Mutant : la
    // fermeture ne termine pas le trait (perdu avec la vue).
    void strokeInProgressEndsBeforeClose()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        DocumentController* paperboard = desktop->desktopDocument();
        beginStylusStroke(desktop->window());
        QCOMPARE(paperboard->shownObjectCount(), 0);
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(!desktop->isAnnotating());
        QCOMPARE(paperboard->shownObjectCount(), 1);
    }

    // F3 : la capture ne contient aucune fenêtre d'ActCraft : nos fenêtres visibles sont masquées le temps de
    // la saisie, puis rendues dans leur état, et l'annotation continue (le masquage n'est pas une fermeture).
    // Mutant : saisie sans masquer.
    void captureHidesOurWindows()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QQuickWindow* mainWindow = main.window();
        QVERIFY(mainWindow->isVisible());
        const Qt::WindowStates mainStates = mainWindow->windowStates();
        QPointer<QQuickWindow> annotation;
        int calls = 0;
        bool mainShown = true;
        bool annotationShown = true;
        desktop->setCaptureSource([&] {
            ++calls;
            mainShown = mainWindow->isVisible();
            annotationShown = annotation && annotation->isVisible();
            return fakeDesktop();
        });
        // Hors annotation : la fenêtre principale.
        QVERIFY(captureDesktop(main, desktop));
        QCOMPARE(calls, 1);
        QVERIFY(!mainShown);
        QVERIFY(mainWindow->isVisible());
        QCOMPARE(mainWindow->windowStates(), mainStates);
        // Pendant l'annotation : les deux fenêtres.
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        annotation = desktop->window();
        const Qt::WindowFlags flags = annotation->flags();
        QVERIFY(captureDesktop(main, desktop));
        QCOMPARE(calls, 2);
        QVERIFY(!mainShown);
        QVERIFY(!annotationShown);
        QVERIFY(mainWindow->isVisible());
        QVERIFY(annotation->isVisible());
        QTest::qWait(50); // l'arrêt sur fermeture passe par un appel en file : il ne doit pas venir
        QVERIFY(desktop->isAnnotating());
        QCOMPARE(desktop->window(), annotation.data());
        QVERIFY(annotation->isVisible());
        QCOMPARE(annotation->flags(), flags);
        QCOMPARE(mainWindow->windowStates(), mainStates);
        QVERIFY(!shownPage(*desktop->desktopDocument()).background.image.empty());
    }

    // m6 : le paperboard du bureau prend la taille de l'écran à chaque ouverture (résolution changée entre
    // deux activations), toutes ses pages, son contenu gardé. Mutant : taille figée à la construction.
    void desktopPaperboardTakesTheScreenSize()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        DocumentController* paperboard = desktop->desktopDocument();
        actcraft::core::Page small;
        small.width = 100;
        small.height = 80;
        paperboard->replaceDocument(actcraft::core::Document(std::vector<actcraft::core::Page>{small, small}),
                                    QUrl(), QStringLiteral("bureau"));
        QCOMPARE(paperboard->pageSize(), QSizeF(100, 80));
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        const QSizeF screen(QGuiApplication::primaryScreen()->geometry().size());
        QCOMPARE(paperboard->pageCount(), 2);
        for (std::size_t i = 0; i < 2; ++i) {
            const actcraft::core::Page& page = paperboard->document().page(i);
            QCOMPARE(QSizeF(page.width, page.height), screen);
        }
    }

    // --- L7-T31 : outils de bureau (F-080), roue et anneaux ---

    // Roue : les identifiants de ses entrées (anneaux compris) sont tous des commandes déclarées du registre
    // chargé de commands.json (celui de la fenêtre principale), sous les préfixes desktop. et tool. ; aucune
    // entrée de vote ni de service de l'éditeur d'origine. Mutant : une entrée « vote.start » dans la liste.
    void wheelHasNoVoteNorVendorEntry()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QObject* root = main.window()->findChild<QObject*>(QStringLiteral("desktopRoot"));
        QVERIFY(root);
        QObject* commands = root->property("commands").value<QObject*>();
        QVERIFY(commands);
        QVariantList entries;
        QVERIFY(QMetaObject::invokeMethod(desktop, "wheelEntries", Q_RETURN_ARG(QVariantList, entries),
                                          Q_ARG(double, 100.0), Q_ARG(double, 200.0)));
        QVERIFY(!entries.isEmpty());
        QStringList ids;
        for (const QVariant& entry : entries) {
            const QVariantMap map = entry.toMap();
            if (!map.value(QStringLiteral("command")).toString().isEmpty()) {
                ids << map.value(QStringLiteral("command")).toString();
            }
            for (const QVariant& inner : map.value(QStringLiteral("ring")).toList()) {
                ids << inner.toMap().value(QStringLiteral("command")).toString();
            }
        }
        QVERIFY(ids.size() >= 10);
        for (const QString& id : std::as_const(ids)) {
            bool available = false;
            QVERIFY(QMetaObject::invokeMethod(commands, "isAvailable", Q_RETURN_ARG(bool, available),
                                              Q_ARG(QString, id)));
            QVERIFY2(available, qPrintable(id));
            QVERIFY2(id.startsWith(QStringLiteral("desktop.")) || id.startsWith(QStringLiteral("tool.")),
                     qPrintable(id));
            for (const char* forbidden : {"vote", "survey", "poll", "promethean", "inspire", "recorder"}) {
                QVERIFY2(!id.contains(QLatin1String(forbidden), Qt::CaseInsensitive), qPrintable(id));
            }
        }
        QVERIFY(ids.contains(QStringLiteral("desktop.restore")));
        QVERIFY(ids.contains(QStringLiteral("tool.keyboard")));
        QVERIFY(ids.contains(QStringLiteral("tool.camera-screen")));
        QVERIFY(ids.contains(QStringLiteral("tool.dice")));
    }

    // Positions : sens horaire depuis le haut (y vers le bas).
    void wheelEntriesAreLaidOutClockwiseFromTheTop()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVariantList entries;
        QVERIFY(QMetaObject::invokeMethod(desktop, "wheelEntries", Q_RETURN_ARG(QVariantList, entries),
                                          Q_ARG(double, 100.0), Q_ARG(double, 200.0)));
        QCOMPARE(entries.size(), 4);
        const QVariantMap first = entries[0].toMap();
        const QVariantMap second = entries[1].toMap();
        QVERIFY(qAbs(first.value(QStringLiteral("x")).toDouble()) < 0.01);
        QVERIFY(qAbs(first.value(QStringLiteral("y")).toDouble() + 100) < 0.01);
        QVERIFY(qAbs(second.value(QStringLiteral("x")).toDouble() - 100) < 0.01);
        QVERIFY(qAbs(second.value(QStringLiteral("y")).toDouble()) < 0.01);
        // Anneau secondaire (caméra) : cinq entrées sur le cercle extérieur.
        const QVariantList ring = second.value(QStringLiteral("ring")).toList();
        QCOMPARE(ring.size(), 5);
        QVERIFY(qAbs(ring[0].toMap().value(QStringLiteral("y")).toDouble() + 200) < 0.01);
    }

    // desktop.tools : la fenêtre principale est réduite, la roue s'ouvre (fenêtre sans cadre, toujours
    // devant, SANS parent : réduite avec la principale, elle disparaîtrait). Mutant : transientParent = la
    // principale.
    void toolsReduceTheMainWindowAndShowTheWheel()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(!desktop->property("toolsShown").toBool());
        QVERIFY(main.run(QStringLiteral("desktop.tools")));
        QVERIFY(desktop->property("toolsShown").toBool());
        QQuickWindow* wheel = wheelWindow();
        QVERIFY(wheel);
        QTRY_VERIFY(wheel->isVisible());
        QVERIFY(wheel->flags().testFlag(Qt::WindowStaysOnTopHint));
        QVERIFY(wheel->flags().testFlag(Qt::FramelessWindowHint));
        QVERIFY(wheel->transientParent() == nullptr);
        QTRY_COMPARE(main.window()->visibility(), QWindow::Minimized);
        QVERIFY(main.run(QStringLiteral("desktop.restore")));
    }

    // Revenir à ActCraft (desktop.restore) ferme la roue et rend la fenêtre principale. Mutant : restore ne
    // ferme pas la roue.
    void restoreClosesTheWheel()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(main.run(QStringLiteral("desktop.tools")));
        QQuickWindow* wheel = wheelWindow();
        QVERIFY(wheel);
        QTRY_VERIFY(wheel->isVisible());
        QVERIFY(main.run(QStringLiteral("desktop.restore")));
        QVERIFY(!desktop->property("toolsShown").toBool());
        QTRY_VERIFY(!wheel->isVisible());
        QTRY_VERIFY(main.window()->visibility() != QWindow::Minimized);
    }

    // La roue se ferme aussi si la fenêtre principale est restaurée autrement (barre des tâches).
    // Mutant : aucune écoute de la visibilité de la fenêtre principale.
    void wheelClosesWhenTheWindowIsRestored()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(main.run(QStringLiteral("desktop.tools")));
        QQuickWindow* wheel = wheelWindow();
        QVERIFY(wheel);
        QTRY_VERIFY(wheel->isVisible());
        QTRY_COMPARE(main.window()->visibility(), QWindow::Minimized);
        main.window()->showNormal();
        QTRY_VERIFY(!desktop->property("toolsShown").toBool());
        QTRY_VERIFY(!wheel->isVisible());
    }

    // Icône puis roue : un clic déploie ; les entrées déclenchent des commandes nommées (« Revenir »
    // ferme la roue) ; un anneau secondaire s'ouvre sur un clic et se referme sur un autre.
    void wheelEntriesRunNamedCommands()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(main.run(QStringLiteral("desktop.tools")));
        QQuickWindow* wheel = wheelWindow();
        QVERIFY(wheel);
        QTRY_VERIFY(wheel->isVisible());
        const auto entry = [wheel](const QString& name) {
            return presentationharness::findItem(wheel->contentItem(), name);
        };
        QVERIFY(!entry(QStringLiteral("desktopWheelEntry_desktop.restore")));
        QQuickItem* icon = entry(QStringLiteral("desktopWheelIcon"));
        QVERIFY(icon);
        QVERIFY(QMetaObject::invokeMethod(icon, "click"));
        QTRY_VERIFY(entry(QStringLiteral("desktopWheelEntry_desktop.restore")));
        // Anneau « Caméra » (deuxième entrée).
        QQuickItem* ring = entry(QStringLiteral("desktopWheelRing_1"));
        QVERIFY(ring);
        QVERIFY(QMetaObject::invokeMethod(ring, "click"));
        for (const char* id : {"tool.camera-area", "tool.camera-polygon", "tool.camera-freehand",
                               "tool.camera-window", "tool.camera-screen"}) {
            const QString name = QStringLiteral("desktopWheelEntry_") + QLatin1String(id);
            QTRY_VERIFY2(entry(name), qPrintable(name));
        }
        QVERIFY(QMetaObject::invokeMethod(ring, "click"));
        QTRY_VERIFY(!entry(QStringLiteral("desktopWheelEntry_tool.camera-area")));
        // « Revenir à ActCraft » : commande nommée, roue fermée.
        QQuickItem* restore = entry(QStringLiteral("desktopWheelEntry_desktop.restore"));
        QVERIFY(restore);
        QVERIFY(QMetaObject::invokeMethod(restore, "click"));
        QVERIFY(!desktop->property("toolsShown").toBool());
        QTRY_VERIFY(!wheel->isVisible());
    }

    // F4a (revue L7-T30) : la palette principale reste devant la fenêtre d'annotation ; le drapeau « toujours
    // devant » est posé pendant l'annotation et rendu ensuite. Mutant : jamais posé.
    void mainWindowStaysInFrontOfTheAnnotation()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(!main.window()->flags().testFlag(Qt::WindowStaysOnTopHint));
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        QVERIFY(main.window()->flags().testFlag(Qt::WindowStaysOnTopHint));
        QVERIFY(desktop->window()->flags().testFlag(Qt::WindowStaysOnTopHint));
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(!desktop->isAnnotating());
        QVERIFY(!main.window()->flags().testFlag(Qt::WindowStaysOnTopHint));
    }

    // Décision de l'orchestrateur (essai 2, d'après l'original) : pendant l'annotation le paperboard
    // disparaît et seule la palette reste devant. La fenêtre principale est réduite à l'emprise de la palette
    // (barre du haut au-dessus), à partir de son état normal ; à la fermeture, visibilité, géométrie et
    // drapeaux d'avant sont rendus (plein écran, maximisée, ou fenêtrée). Mutants : pas de réduction ; pas de
    // retour à l'état d'avant.
    void mainWindowShrinksToThePaletteAndComesBack_data()
    {
        QTest::addColumn<int>("visibility");
        QTest::newRow("fenetree") << static_cast<int>(QWindow::Windowed);
        QTest::newRow("plein ecran") << static_cast<int>(QWindow::FullScreen);
        QTest::newRow("maximisee") << static_cast<int>(QWindow::Maximized);
    }
    void mainWindowShrinksToThePaletteAndComesBack()
    {
        QFETCH(int, visibility);
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QQuickWindow* window = main.window();
        if (visibility == static_cast<int>(QWindow::FullScreen)) {
            window->showFullScreen();
        } else if (visibility == static_cast<int>(QWindow::Maximized)) {
            window->showMaximized();
        }
        QTRY_COMPARE(static_cast<int>(window->visibility()), visibility);
        QVERIFY(main.waitReady());
        QQuickItem* palette =
            presentationharness::findItem(window->contentItem(), QStringLiteral("toolPalette"));
        QVERIFY(palette);
        const QRect screen = QGuiApplication::primaryScreen()->geometry();
        const QRect before = window->geometry();
        const Qt::WindowFlags flagsBefore = window->flags();
        const int paletteWidth = static_cast<int>(palette->width());
        QVERIFY(paletteWidth > 0);
        QVERIFY(before.width() > paletteWidth);
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        // Réduite à l'emprise de la palette : plus étroite que l'écran, de la largeur de la palette.
        QTRY_COMPARE(window->geometry().width(), paletteWidth);
        QVERIFY(window->geometry().width() < screen.width());
        QCOMPARE(static_cast<int>(window->visibility()), static_cast<int>(QWindow::Windowed));
        QVERIFY(window->flags().testFlag(Qt::WindowStaysOnTopHint));
        // Fermeture : l'état d'avant, exactement.
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(!desktop->isAnnotating());
        QTRY_COMPARE(static_cast<int>(window->visibility()), visibility);
        QTRY_COMPARE(window->geometry(), before);
        QCOMPARE(window->flags(), flagsBefore);
    }

    // F4b (revue L7-T30) : en passage des clics, une fenêtre toujours devant qui reçoit les clics rend la
    // saisie à l'annotation (la fenêtre d'annotation et la principale n'en reçoivent plus). Mutant : le
    // bouton ne rend pas la saisie.
    void passThroughHasAnAccessibleExit()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        QQuickWindow* before = windowNamed(QStringLiteral("desktopResumeWindow"));
        QVERIFY(!before || !before->isVisible());
        desktop->setPassThrough(true);
        QQuickWindow* resume = windowNamed(QStringLiteral("desktopResumeWindow"));
        QVERIFY(resume);
        QTRY_VERIFY(resume->isVisible());
        QVERIFY(resume->flags().testFlag(Qt::WindowStaysOnTopHint));
        QVERIFY(!resume->flags().testFlag(Qt::WindowTransparentForInput));
        QVERIFY(desktop->window()->flags().testFlag(Qt::WindowTransparentForInput));
        QQuickItem* button =
            presentationharness::findItem(resume->contentItem(), QStringLiteral("desktopResumeButton"));
        QVERIFY(button);
        QVERIFY(QMetaObject::invokeMethod(button, "click"));
        QVERIFY(!desktop->passThrough());
        QVERIFY(desktop->isAnnotating());
        QVERIFY(!desktop->window()->flags().testFlag(Qt::WindowTransparentForInput));
        QTRY_VERIFY(!resume->isVisible());
    }

    // F-070 (L7-T34) : l'outil Sélectionner de l'utilisateur (outils du lot 3, pas ceux de la zone de
    // l'annotation, qui n'en a pas) fait passer les clics vers le bureau ; un autre outil rend la saisie à
    // l'annotation. Hors annotation, rien. Mutant : le passage ne suit pas l'outil courant.
    void selectToolPassesClicksThrough()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(main.run(QStringLiteral("tool.pen")));
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        QQuickWindow* window = desktop->window();
        QVERIFY(!desktop->passThrough());

        QVERIFY(main.run(QStringLiteral("tool.select")));
        QTRY_VERIFY(desktop->passThrough());
        QVERIFY(window->flags().testFlag(Qt::WindowTransparentForInput));
        QVERIFY(desktop->isAnnotating());

        QVERIFY(main.run(QStringLiteral("tool.pen")));
        QTRY_VERIFY(!desktop->passThrough());
        QVERIFY(!window->flags().testFlag(Qt::WindowTransparentForInput));

        // Le bouton de la fenêtre reste : le passage manuel tient avec le stylo.
        QObject* button = window->findChild<QObject*>(QStringLiteral("desktopPassThroughButton"));
        QVERIFY(button);
        QVERIFY(QMetaObject::invokeMethod(button, "click"));
        QVERIFY(desktop->passThrough());
        QVERIFY(QMetaObject::invokeMethod(button, "click"));
        QVERIFY(!desktop->passThrough());

        QVERIFY(main.run(QStringLiteral("desktop.annotate"))); // fermée
        QTRY_VERIFY(!desktop->isAnnotating());
        QVERIFY(main.run(QStringLiteral("tool.select")));
        // Témoin, sans attente fixe : l'outil a changé ; followTool suit ce changement dans le même appel.
        QObject* root = main.window()->findChild<QObject*>(QStringLiteral("desktopRoot"));
        QVERIFY(root);
        QObject* tools = root->property("tools").value<QObject*>();
        QVERIFY(tools);
        QTRY_COMPARE(tools->property("currentTool").toString(), QStringLiteral("select"));
        QVERIFY(!desktop->passThrough());
    }

    // M4 (relecture du lot 7) : la capture n'endort pas le fil principal. La commande rend la main aussitôt,
    // nos fenêtres masquées ; la saisie vient après le délai, puis le fond est posé et les fenêtres
    // reviennent ; une seconde demande pendant ce temps est refusée. La source de production est enveloppée,
    // pas remplacée (mineur du tour 2 : remplacée, un sommeil remis dans grabPrimaryScreen passait). Deux
    // durées du fil principal sont mesurées, chacune sous le délai de 250 ms qu'un sommeil du défaut M4
    // dépasse : la commande (captureBackground, hideOurWindows : mineur de la relecture de l'essai 1) et la
    // fin de la capture, de l'entrée dans la source à backgroundCaptured (grabPrimaryScreen,
    // showOurWindows). Mutants : saisie dans la commande même ; QThread::msleep(250) remis dans
    // grabPrimaryScreen, puis dans hideOurWindows.
    void captureDoesNotBlockTheMainThread()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        DocumentController* open = openDocumentOf(main);
        QVERIFY(open);
        const DesktopController::CaptureSource screen = desktop->captureSource();
        QVERIFY(screen);
        int calls = 0;
        QElapsedTimer finishing;
        desktop->setCaptureSource([&calls, &screen, &finishing] {
            ++calls;
            finishing.start();
            static_cast<void>(screen()); // l'image d'un écran hors écran ne sert pas : seule sa durée compte
            return fakeDesktop();
        });
        qint64 finishMs = -1;
        qint64 totalMs = -1;
        QElapsedTimer running;
        connect(desktop, &DesktopController::backgroundCaptured, this,
                [&finishing, &finishMs, &running, &totalMs] {
                    finishMs = finishing.elapsed();
                    totalMs = running.elapsed();
                });
        QSignalSpy finished(desktop, &DesktopController::backgroundCaptured);
        running.start();
        QVERIFY(main.run(QStringLiteral("page.background-desktop-capture")));
        const qint64 runMs = running.elapsed();
        QCOMPARE(calls, 0);
        QVERIFY(!main.window()->isVisible());
        QVERIFY(shownPage(*open).background.image.empty());
        QVERIFY(!desktop->captureBackground()); // une capture à la fois
        QVERIFY(finished.wait(5000));
        QCOMPARE(finished.count(), 1);
        QVERIFY(finished.first().at(0).toBool());
        QCOMPARE(calls, 1);
        // Ni la commande ni la fin de la capture n'endorment le fil principal (sous le délai de 250 ms).
        QVERIFY2(runMs < 200, qPrintable(QStringLiteral("commande : %1 ms").arg(runMs)));
        QVERIFY2(finishMs >= 0 && finishMs < 200, qPrintable(QStringLiteral("fin : %1 ms").arg(finishMs)));
        // Toute la chaîne, de la commande à backgroundCaptured, moins le délai de 250 ms des fenêtres
        // masquées : un sommeil entre ce délai et l'appel de la source (finishCapture) échappe aux deux
        // mesures précédentes, pas à celle-ci (mineur du tour 2, mutant : msleep(250) avant m_capture()).
        QVERIFY2(totalMs >= 0 && totalMs < 250 + 200,
                 qPrintable(QStringLiteral("commande à fin : %1 ms").arg(totalMs)));
        QVERIFY(main.window()->isVisible());
        QVERIFY(!shownPage(*open).background.image.empty());
    }

    // Relecture du lot 7 : la roue fermée déployée (fenêtre principale revenue par la barre des tâches)
    // reparaît repliée, l'icône à sa place. Mutant : le repli à la fermeture ne rend pas le décalage du
    // déploiement (224 px).
    void wheelReopensFoldedWhereItWas()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(main.run(QStringLiteral("desktop.tools")));
        QQuickWindow* wheel = wheelWindow();
        QVERIFY(wheel);
        QTRY_VERIFY(wheel->isVisible());
        const QPoint before = wheel->position();
        QQuickItem* icon =
            presentationharness::findItem(wheel->contentItem(), QStringLiteral("desktopWheelIcon"));
        QVERIFY(icon);
        QVERIFY(QMetaObject::invokeMethod(icon, "click"));
        QTRY_VERIFY(wheel->property("expanded").toBool());
        QVERIFY(wheel->position() != before);
        QTRY_COMPARE(main.window()->visibility(), QWindow::Minimized);
        main.window()->showNormal();
        QTRY_VERIFY(!wheel->isVisible());
        QVERIFY(!wheel->property("expanded").toBool());
        QVERIFY(main.run(QStringLiteral("desktop.tools")));
        QTRY_VERIFY(wheel->isVisible());
        QCOMPARE(wheel->position(), before);
        QVERIFY(main.run(QStringLiteral("desktop.restore")));
    }

    // M3 (relecture du lot 7) : Sélectionner déjà actif à l'ouverture de l'annotation fait passer les clics
    // dès l'ouverture (F-070), sans attendre un changement d'outil ; ouverte avec le stylo, elle prend la
    // saisie. Mutants : seul currentToolChanged est suivi ; l'outil est relu à la création de la fenêtre
    // (avant que l'annotation soit ouverte : followTool l'ignore).
    void selectToolBeforeOpeningPassesClicksThrough()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(main.run(QStringLiteral("tool.select")));
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        QTRY_VERIFY(desktop->passThrough());
        QVERIFY(desktop->window()->flags().testFlag(Qt::WindowTransparentForInput));
        QVERIFY(main.run(QStringLiteral("tool.pen")));
        QTRY_VERIFY(!desktop->passThrough());
        QVERIFY(main.run(QStringLiteral("desktop.annotate"))); // fermée
        QTRY_VERIFY(!desktop->isAnnotating());
        QVERIFY(main.run(QStringLiteral("desktop.annotate"))); // rouverte avec le stylo
        QVERIFY(waitDesktopReady(desktop));
        QVERIFY(!desktop->passThrough());
        QVERIFY(!desktop->window()->flags().testFlag(Qt::WindowTransparentForInput));
    }

    // M5 (relecture du lot 7) : revenir de la roue rend l'état d'avant de la fenêtre principale (ici le plein
    // écran), même quand l'annotation a été fermée pendant que la roue était ouverte. Mutant : restore() fait
    // toujours showNormal (l'état gardé par l'annotation est perdu à sa fermeture, roue ouverte).
    void restoreFromTheWheelGivesBackFullScreen_data()
    {
        QTest::addColumn<bool>("annotating");
        QTest::newRow("annotation fermee roue ouverte") << true;
        QTest::newRow("sans annotation") << false;
    }
    void restoreFromTheWheelGivesBackFullScreen()
    {
        QFETCH(bool, annotating);
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QQuickWindow* window = main.window();
        window->showFullScreen();
        QTRY_COMPARE(window->visibility(), QWindow::FullScreen);
        QVERIFY(main.waitReady());
        if (annotating) {
            QVERIFY(main.run(QStringLiteral("desktop.annotate")));
            QVERIFY(waitDesktopReady(desktop));
            QTRY_COMPARE(window->visibility(), QWindow::Windowed);
        }
        QVERIFY(main.run(QStringLiteral("desktop.tools")));
        QTRY_COMPARE(window->visibility(), QWindow::Minimized);
        if (annotating) {
            QVERIFY(main.run(QStringLiteral("desktop.annotate"))); // fermée, roue ouverte
            QVERIFY(!desktop->isAnnotating());
            QVERIFY(desktop->property("toolsShown").toBool());
            QCOMPARE(window->visibility(), QWindow::Minimized); // elle ne revient pas d'elle-même
        }
        QVERIFY(main.run(QStringLiteral("desktop.restore")));
        QVERIFY(!desktop->property("toolsShown").toBool());
        QTRY_COMPARE(window->visibility(), QWindow::FullScreen);
    }

    // M8 (relecture du lot 7) : un outil lancé depuis un anneau de la roue (horloge, dés, calculatrice,
    // message défilant : tous dans la fenêtre principale) se voit : la fenêtre qui le porte n'est plus
    // réduite, le panneau est ouvert. Mutant : l'entrée déclenche seulement la commande de l'outil (le
    // panneau s'ouvre dans la fenêtre principale réduite, invisible).
    void wheelToolIsVisible_data()
    {
        QTest::addColumn<QString>("command");
        QTest::addColumn<QString>("panel"); // racine du panneau ; vide : le message défile sur la page
        QTest::newRow("horloge") << QStringLiteral("tool.clock") << QStringLiteral("clockRoot");
        QTest::newRow("des") << QStringLiteral("tool.dice") << QStringLiteral("diceRoot");
        QTest::newRow("calculatrice")
            << QStringLiteral("tool.calculator") << QStringLiteral("calculatorRoot");
        QTest::newRow("message defilant") << QStringLiteral("tool.ticker") << QString();
    }
    void wheelToolIsVisible()
    {
        QFETCH(QString, command);
        QFETCH(QString, panel);
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(main.waitReady());
        QVERIFY(main.run(QStringLiteral("desktop.tools")));
        QQuickWindow* wheel = wheelWindow();
        QVERIFY(wheel);
        QTRY_VERIFY(wheel->isVisible());
        QTRY_COMPARE(main.window()->visibility(), QWindow::Minimized);
        QVERIFY2(clickRingEntry(desktop, wheel, command), qPrintable(command));
        QTRY_VERIFY_WITH_TIMEOUT(main.window()->visibility() != QWindow::Minimized, kRoomMadeMs);
        QVERIFY(main.window()->isVisible());
        QVERIFY(!desktop->property("toolsShown").toBool());
        if (!panel.isEmpty()) {
            QQuickItem* root = main.item(panel);
            QVERIFY2(root, qPrintable(panel));
            QTRY_VERIFY(root->isVisible());
            QCOMPARE(root->window(), main.window());
        }
    }

    // Ma1 (tour 2 de relecture du lot 7) : M8 pendant l'annotation du bureau. La fenêtre principale n'y est
    // que la palette (keepMainInFront, décision « seule la palette reste devant ») : un outil de l'anneau «
    // Autres outils », élément de cette fenêtre (H-M8), y serait rogné ou hors champ. L'entrée ferme d'abord
    // l'annotation (comme desktop.annotate : son paperboard est gardé) ; la fenêtre principale reprend son
    // état d'avant l'annotation (chemin M5), puis l'outil s'y ouvre ENTIER. Mutant : l'entrée fait seulement
    // desktop.restore (showNormal à l'emprise de la palette) ; isVisible() du panneau resterait vrai.
    void wheelToolDuringAnnotationIsWholeInTheMainWindow_data()
    {
        QTest::addColumn<QString>("command");
        QTest::addColumn<QString>("shown"); // racine du panneau, ou dialogue (fenêtre surgissante)
        QTest::newRow("horloge") << QStringLiteral("tool.clock") << QStringLiteral("clockRoot");
        QTest::newRow("des") << QStringLiteral("tool.dice") << QStringLiteral("diceRoot");
        QTest::newRow("calculatrice")
            << QStringLiteral("tool.calculator") << QStringLiteral("calculatorRoot");
        QTest::newRow("message defilant") << QStringLiteral("tool.ticker") << QStringLiteral("tickerDialog");
    }
    void wheelToolDuringAnnotationIsWholeInTheMainWindow()
    {
        QFETCH(QString, command);
        QFETCH(QString, shown);
        MainWindow main;
        QQuickWindow* window = main.window();
        QVERIFY(window);
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QVERIFY(main.waitReady());
        const QRect before = window->geometry();
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        QTRY_VERIFY(window->geometry().width() < before.width()); // la palette seule
        QVERIFY(main.run(QStringLiteral("desktop.tools")));
        QQuickWindow* wheel = wheelWindow();
        QVERIFY(wheel);
        QTRY_VERIFY(wheel->isVisible());
        QTRY_COMPARE(window->visibility(), QWindow::Minimized);
        QVERIFY2(clickRingEntry(desktop, wheel, command), qPrintable(command));
        QTRY_VERIFY_WITH_TIMEOUT(window->visibility() != QWindow::Minimized, kRoomMadeMs);
        QVERIFY(window->isVisible());
        QVERIFY(!desktop->property("toolsShown").toBool());
        QQuickItem* item = shownItemOf(main, shown);
        QVERIFY2(item, qPrintable(shown));
        QTRY_VERIFY2(shownWholeIn(item, window), qPrintable(placeOf(item, window)));
        // Comportement retenu : annotation fermée, fenêtre principale à sa géométrie d'avant l'annotation.
        QVERIFY(!desktop->isAnnotating());
        QTRY_COMPARE(window->geometry(), before);
    }

    // Ma1, essai 2 (tour 2 de relecture du lot 7) : la règle vit au niveau des commandes, plus dans la seule
    // roue. Pendant l'annotation, la fenêtre principale n'est que la palette : toute commande qui ouvre un
    // panneau de la fenêtre principale (posé dans presentation.layer ; drapeau « mainWindow » de
    // commands.json) ferme d'abord l'annotation, son paperboard gardé ; la fenêtre reprend sa géométrie
    // d'avant et le panneau s'y ouvre ENTIER, quel que soit le chemin. Ici, la commande directe. Mutant : la
    // règle dans la seule roue (essai 1) ; le panneau s'ouvre rogné dans la fenêtre de la palette.
    void mainWindowToolDuringAnnotationIsWhole_data()
    {
        QTest::addColumn<QString>("command");
        QTest::addColumn<QString>("shown"); // racine du panneau, ou dialogue (fenêtre surgissante)
        QTest::newRow("horloge") << QStringLiteral("tool.clock") << QStringLiteral("clockRoot");
        QTest::newRow("des") << QStringLiteral("tool.dice") << QStringLiteral("diceRoot");
        QTest::newRow("calculatrice")
            << QStringLiteral("tool.calculator") << QStringLiteral("calculatorRoot");
        QTest::newRow("message defilant") << QStringLiteral("tool.ticker") << QStringLiteral("tickerDialog");
        QTest::newRow("effets de page")
            << QStringLiteral("view.page-effects") << QStringLiteral("effectsRoot");
        QTest::newRow("equation") << QStringLiteral("insert.equation") << QStringLiteral("equationRoot");
    }
    void mainWindowToolDuringAnnotationIsWhole()
    {
        QFETCH(QString, command);
        QFETCH(QString, shown);
        MainWindow main;
        QRect before;
        openAnnotationWithPaletteOnly(main, before);
        if (QTest::currentTestFailed()) {
            return;
        }
        QVERIFY(main.run(command));
        expectWholeInTheMainWindow(main, shown, before);
    }

    // Ma1, essai 3 (tour 2 de relecture du lot 7) : la place n'est faite que pour un panneau qui s'ouvre
    // vraiment. Une commande « mainWindow » dont le gestionnaire ne montre rien (calculatrice du système :
    // réglage tools/calculator/mode à 1 ou 2, programme lancé, panneau fermé) garde l'annotation et la
    // palette seule. Le gestionnaire est remplacé par un gestionnaire qui ne montre rien : le cas de la
    // calculatrice du système, reproductible hors Windows et sans lancer de programme. Mutant : l'essai 2,
    // place faite par bindHandler avant tout gestionnaire marqué (annotation fermée pour rien).
    void mainWindowCommandShowingNothingKeepsTheAnnotation()
    {
        MainWindow main;
        QRect before;
        openAnnotationWithPaletteOnly(main, before);
        if (QTest::currentTestFailed()) {
            return;
        }
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        actcraft::ui::Commands* commands = desktop->commands();
        QVERIFY(commands);
        QJSEngine* engine = qjsEngine(commands);
        QVERIFY(engine);
        const QRect palette = main.window()->geometry();
        commands->setHandler(QStringLiteral("tool.calculator"),
                             engine->evaluate(QStringLiteral("(function () {})")));
        QVERIFY(main.run(QStringLiteral("tool.calculator")));
        QVERIFY(desktop->isAnnotating());
        QCOMPARE(main.window()->geometry(), palette);
        QQuickItem* panel = main.item(QStringLiteral("calculatorRoot"));
        QVERIFY(panel);
        QVERIFY(!panel->isVisible());
    }

#ifdef Q_OS_WIN
    // Ma1, essai 3 : le vrai chemin de la calculatrice du système (Windows, réglage tools/calculator/mode à
    // 1). Annotation ouverte, tool.calculator lance le programme par le lanceur injecté, ne montre aucun
    // panneau et garde l'annotation et la palette seule. Mutant : requestMainWindow avant le `if
    // (calculatorController.open())` de CalculatorPanel.qml (annotation fermée pour rien). Ne tourne pas sur
    // le Mac : preuves-windows.yml.
    void systemCalculatorDuringAnnotationKeepsTheAnnotation()
    {
        MainWindow main;
        QRect before;
        openAnnotationWithPaletteOnly(main, before);
        if (QTest::currentTestFailed()) {
            return;
        }
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        auto* calc =
            main.window()->findChild<actcraft::ui::CalculatorController*>(QStringLiteral("calculator"));
        QVERIFY(calc);
        QStringList launched;
        calc->setLauncher([&launched](const QString& program) {
            launched.append(program);
            return true;
        });
        calc->setMode(1);
        const QRect palette = main.window()->geometry();
        QVERIFY(main.run(QStringLiteral("tool.calculator")));
        QCOMPARE(launched, QStringList{QStringLiteral("calc.exe")});
        QVERIFY(desktop->isAnnotating());
        QCOMPARE(main.window()->geometry(), palette);
        QQuickItem* panel = main.item(QStringLiteral("calculatorRoot"));
        QVERIFY(panel);
        QVERIFY(!panel->isVisible());
    }
#endif

    // Ma1, essai 3 (mineur du tour 2) : equation.edit ouvre le même panneau (equationRoot) que
    // insert.equation, et se déclenche pendant l'annotation (trigger d'un scénario). Équation connue : le
    // panneau s'ouvre entier dans la fenêtre principale ; équation inconnue : aucun panneau, l'annotation
    // reste. Mutants : drapeau « mainWindow » retiré d'equation.edit (place refusée, panneau rogné dans la
    // palette) ; place demandée avant equationController.edit (annotation fermée pour rien).
    void equationEditDuringAnnotation_data()
    {
        QTest::addColumn<bool>("known");
        QTest::newRow("equation connue") << true;
        QTest::newRow("equation inconnue") << false;
    }
    void equationEditDuringAnnotation()
    {
        QFETCH(bool, known);
        MainWindow main;
        QRect before;
        openAnnotationWithPaletteOnly(main, before);
        if (QTest::currentTestFailed()) {
            return;
        }
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        DocumentController* open = openDocumentOf(main);
        QVERIFY(open);
        actcraft::core::Equation equation;
        equation.bounds = actcraft::core::RectF{100, 100, 160, 60};
        equation.mathml = "<math><mi>x</mi><mo>+</mo><mi>y</mi></math>";
        actcraft::core::PageObject object = std::move(equation);
        object.common.layer = actcraft::core::Layer::Middle;
        const actcraft::core::ObjectId id =
            actcraft::ui::DocumentInsertions(*open).insertObject(std::move(object));
        const QRect palette = main.window()->geometry();
        const double target = static_cast<double>(id) + (known ? 0.0 : 1000.0);
        QVERIFY(main.run(QStringLiteral("equation.edit"), QVariant(target)));
        if (known) {
            expectWholeInTheMainWindow(main, QStringLiteral("equationRoot"), before);
            return;
        }
        QVERIFY(desktop->isAnnotating());
        QCOMPARE(main.window()->geometry(), palette);
        QQuickItem* panel = main.item(QStringLiteral("equationRoot"));
        QVERIFY(panel);
        QVERIFY(!panel->isVisible());
    }

    // Ma1, essai 2 : même règle par le raccourci clavier de la commande. Mutant : la règle dans la seule
    // roue.
    void mainWindowToolShortcutDuringAnnotationIsWhole_data()
    {
        QTest::addColumn<QString>("sequence");
        QTest::addColumn<QString>("shown");
        QTest::newRow("horloge") << QStringLiteral("Ctrl+Shift+U") << QStringLiteral("clockRoot");
        QTest::newRow("message defilant") << QStringLiteral("Ctrl+Shift+T") << QStringLiteral("tickerDialog");
    }
    void mainWindowToolShortcutDuringAnnotationIsWhole()
    {
        QFETCH(QString, sequence);
        QFETCH(QString, shown);
        MainWindow main;
        QRect before;
        openAnnotationWithPaletteOnly(main, before);
        if (QTest::currentTestFailed()) {
            return;
        }
        QTest::keySequence(main.window(), QKeySequence(sequence));
        expectWholeInTheMainWindow(main, shown, before);
    }

    // Ma1, essai 2 : même règle par la palette (« Horloge », seul outil de la fenêtre principale qu'elle
    // porte), cherchée dans la palette elle-même. Mutant : la règle dans la seule roue.
    void mainWindowToolFromThePaletteDuringAnnotationIsWhole()
    {
        MainWindow main;
        QRect before;
        openAnnotationWithPaletteOnly(main, before);
        if (QTest::currentTestFailed()) {
            return;
        }
        QQuickItem* palette =
            presentationharness::findItem(main.window()->contentItem(), QStringLiteral("toolPalette"));
        QVERIFY(palette);
        QQuickItem* button = presentationharness::findItem(palette, QStringLiteral("button-tool.clock"));
        QVERIFY(button);
        QVERIFY(QMetaObject::invokeMethod(button, "click"));
        expectWholeInTheMainWindow(main, QStringLiteral("clockRoot"), before);
    }

    // Ma1, essai 2 : même règle par le menu « Outils » de la barre du haut, que la fenêtre réduite garde
    // (« Outils de maths » : dés, calculatrice ; « Autres outils » : horloge, message défilant). Mutant : la
    // règle dans la seule roue.
    void mainWindowToolFromTheMenuDuringAnnotationIsWhole_data()
    {
        QTest::addColumn<QString>("command");
        QTest::addColumn<QString>("shown");
        QTest::newRow("des") << QStringLiteral("tool.dice") << QStringLiteral("diceRoot");
        QTest::newRow("calculatrice")
            << QStringLiteral("tool.calculator") << QStringLiteral("calculatorRoot");
        QTest::newRow("horloge") << QStringLiteral("tool.clock") << QStringLiteral("clockRoot");
        QTest::newRow("message defilant") << QStringLiteral("tool.ticker") << QStringLiteral("tickerDialog");
    }
    void mainWindowToolFromTheMenuDuringAnnotationIsWhole()
    {
        QFETCH(QString, command);
        QFETCH(QString, shown);
        MainWindow main;
        QRect before;
        openAnnotationWithPaletteOnly(main, before);
        if (QTest::currentTestFailed()) {
            return;
        }
        QQuickItem* menuButton =
            presentationharness::findItem(main.window()->contentItem(), QStringLiteral("menu-tools"));
        QVERIFY(menuButton);
        QVERIFY(QMetaObject::invokeMethod(menuButton, "clicked"));
        auto* entry = menuButton->findChild<QQuickItem*>(QStringLiteral("menuItem-") + command);
        QVERIFY2(entry, qPrintable(command));
        QVERIFY(QMetaObject::invokeMethod(entry, "click"));
        expectWholeInTheMainWindow(main, shown, before);
    }

    // M2 (relecture du lot 7 ; plan 7b, L7-T30 ; F-640 : effets désactivés pendant l'annotation de bureau) :
    // annotation ouverte, page.next du document ouvert change de page aussitôt, sans instantané ni effet ;
    // fermée, l'effet revient. Mutant : TransitionBlockers::desktopAnnotation jamais posé.
    void transitionsBlockedWhileAnnotating()
    {
        MainWindow main;
        QVERIFY(main.window());
        DesktopController* desktop = desktopOf(main);
        QVERIFY(desktop);
        QObject* transition = main.window()->findChild<QObject*>(QStringLiteral("transition"));
        QQuickItem* layer = main.item(QStringLiteral("transitionRoot"));
        DocumentController* open = openDocumentOf(main);
        QVERIFY(transition && layer && open);
        QVERIFY(main.run(QStringLiteral("view.page-effect"), QStringLiteral("dissolve")));
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(waitDesktopReady(desktop));
        const int before = open->currentIndex();
        QVERIFY(main.run(QStringLiteral("page.next")));
        QCOMPARE(open->currentIndex(), before + 1);
        QCOMPARE(transition->property("phase").toInt(), 0);
        QCOMPARE(layer->property("snapshotCount").toInt(), 0);
        QCOMPARE(transition->property("played").toString(), QString());
        // Fermée : l'effet revient.
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QVERIFY(!desktop->isAnnotating());
        QVERIFY(main.waitReady());
        QVERIFY(main.run(QStringLiteral("page.previous")));
        QTRY_COMPARE_WITH_TIMEOUT(transition->property("played").toString(), QStringLiteral("dissolve"),
                                  5000);
        QCOMPARE(layer->property("snapshotCount").toInt(), 1);
        QTRY_COMPARE_WITH_TIMEOUT(transition->property("phase").toInt(), 0, 8000);
        QCOMPARE(open->currentIndex(), before);
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("desktop", TstUiDesktop)
#include "tst_ui_desktop.moc"
