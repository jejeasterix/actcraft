#include "desktopcontroller.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QColor>
#include <QImage>
#include <QPointer>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QSurfaceFormat>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>

#include <cstddef>

namespace {

using actcraft::ui::DesktopController;
using actcraft::ui::DocumentController;
using presentationharness::MainWindow;
using presentationharness::Pointer;

// Contrôleur porté par la racine `desktopRoot` de Presentation.qml (QtObject : cherché par l'arbre des objets).
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
    return doc.document().pages().at(static_cast<std::size_t>(doc.currentIndex()));
}

bool isTransparentFill(const actcraft::core::Fill& fill)
{
    return fill == actcraft::core::solidFill(0x00000000u);
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

// Trait au milieu de la fenêtre d'annotation (la page couvre l'écran principal).
void strokeInDesktopWindow(DesktopController* desktop, Pointer pointer)
{
    QQuickWindow* window = desktop->window();
    const QSizeF size = window->size();
    presentationharness::strokeAt(window, pointer, QPointF(size.width() * 0.35, size.height() * 0.45),
                                  QPointF(size.width() * 0.6, size.height() * 0.55));
}

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
class TstUiDesktop : public QObject
{
    Q_OBJECT

private slots:
    // Fenêtre créée par le contrôleur : sans cadre, toujours devant, couleur transparente, surface à canal alpha
    // demandée AVANT sa création (format posé entre beginCreate et completeCreate). Mutant : fenêtre créée par
    // createObject en QML (format par défaut, alphaBufferSize -1) ; ou drapeau « toujours devant » retiré.
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
    // PageArea { documentController: root.documentController } (patron des autres racines) → le trait part dans
    // le document ouvert.
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
        QCOMPARE(view->tools(), nullptr);
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

    // Laisser passer les clics : la fenêtre devient transparente aux entrées, et le redevient pas à l'inverse.
    // Mutant : le drapeau n'est jamais posé.
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

    // Capture du bureau (F-071) : image figée en fond de la page affichée du document ouvert, ressource PNG du
    // document, en une étape. Mutant : deux commandes (ressource puis fond) → une annulation ne rend pas l'état
    // d'avant.
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
        QVERIFY(main.run(QStringLiteral("page.background-desktop-capture")));
        const std::string key = shownPage(*open).background.image;
        QVERIFY(!key.empty());
        const QByteArray bytes = open->document().resourceBytes(key);
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
        QVERIFY(main.run(QStringLiteral("page.background-desktop-capture")));
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

    // P13 : page suivante du paperboard du bureau = nouvelle page transparente vierge, même après une page à fond
    // capturé, en une étape. Mutant : DocumentController::nextPage (F-125, page du fond par défaut).
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
        QVERIFY(main.run(QStringLiteral("page.background-desktop-capture")));
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
    // principale reste inchangée. Mutant : documentController de la racine (document ouvert).
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
        const QString openBefore = open->fingerprintText();
        const int mainObjects = main.pageObjectCount();
        strokeInDesktopWindow(desktop, pointer);
        QTRY_COMPARE(desktop->desktopDocument()->shownObjectCount(), 1);
        QCOMPARE(open->fingerprintText(), openBefore);
        QCOMPARE(main.pageObjectCount(), mainObjects);
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("desktop", TstUiDesktop)
#include "tst_ui_desktop.moc"
