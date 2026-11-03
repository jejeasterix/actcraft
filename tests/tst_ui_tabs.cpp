#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documenttabs.h>
#include <actcraft/ui/pageview.h>

#include <QElapsedTimer>
#include <QImage>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSignalSpy>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

// Trait rouge de `a` à `b` (coordonnées de page).
core::Stroke redStroke(QPointF a, QPointF b)
{
    core::Stroke stroke;
    stroke.color = 0xFFFF0000u;
    stroke.baseWidth = 12;
    const int steps = 20;
    for (int i = 0; i <= steps; ++i) {
        const double t = double(i) / steps;
        stroke.points.push_back(
            core::StrokePoint{a.x() + (b.x() - a.x()) * t, a.y() + (b.y() - a.y()) * t, 0.5f});
    }
    return stroke;
}

bool near(const QColor& a, const QColor& b, int tolerance)
{
    return qAbs(a.red() - b.red()) <= tolerance && qAbs(a.green() - b.green()) <= tolerance &&
           qAbs(a.blue() - b.blue()) <= tolerance;
}

// Un DocumentController et ses onglets ; avec `withView`, la zone de page réelle (PageArea : vue unique,
// cache dont chaque rendu terminé appelle notifyCacheUpdated, D21) dans une fenêtre hors écran, rendu
// logiciel.
struct Fixture
{
    QQmlEngine engine;
    std::unique_ptr<QObject> root;
    ui::DocumentController document;
    ui::DocumentTabs tabs;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
    render::FixedPageItem* fixed = nullptr;

    explicit Fixture(bool withView = false)
    {
        document.newDocument(800, 600);
        if (withView) {
            QQmlComponent component(&engine);
            component.setData("import QtQuick\nimport QtQuick.Window\nimport ActCraft.Ui\n"
                              "Window { width: 800; height: 600; visible: true\n"
                              "  PageArea { objectName: \"area\"; anchors.fill: parent } }",
                              QUrl(QStringLiteral("qrc:/tst_ui_tabs/Window.qml")));
            root.reset(component.create());
            if (!root) {
                qWarning() << component.errors();
                return;
            }
            window = qobject_cast<QQuickWindow*>(root.get());
            QQuickItem* area = window->findChild<QQuickItem*>(QStringLiteral("area"));
            area->setProperty("documentController", QVariant::fromValue(&document));
            view = window->findChild<ui::PageView*>(QStringLiteral("pageView"));
            fixed = window->findChild<render::FixedPageItem*>(QStringLiteral("fixedPage"));
            document.setFixedPage(fixed);
            tabs.setView(view);
        }
        tabs.setDocument(&document);
        document.commitStroke(redStroke({100, 100}, {300, 100})); // un trait versé
        if (withView) {
            QVERIFY(QTest::qWaitForWindowExposed(window));
            QVERIFY(waitCacheStable());
        }
    }

    core::ObjectId firstId() const { return document.document().page(0).objects.front().common.id; }
    bool pageHasObjects(int page) const
    {
        return !document.document().page(static_cast<std::size_t>(page)).objects.empty();
    }

    // cacheReady, puis kCacheFoldIdleMs + 300 ms sans rendu terminé (D21), borné à 20 s.
    bool waitCacheStable()
    {
        if (!QTest::qWaitFor([this] { return fixed->isReady() && view->cacheReady(); }, 10000)) {
            return false;
        }
        int last = view->cacheRefreshCount();
        QElapsedTimer quiet;
        quiet.start();
        QElapsedTimer total;
        total.start();
        while (total.elapsed() < 20000) {
            QTest::qWait(50);
            if (view->cacheRefreshCount() != last || !view->cacheReady()) {
                last = view->cacheRefreshCount();
                quiet.restart();
            } else if (quiet.elapsed() >= ui::kCacheFoldIdleMs + 300) {
                return true;
            }
        }
        return false;
    }

    void drawStroke(QPointF a, QPointF b) { document.commitStroke(redStroke(a, b)); }
    void drawStrokeOnShown() { drawStroke({100, 100}, {300, 100}); }

    // Un pixel proche de `color` dans le carré de 7 px autour du point de page `page`.
    bool grabHasColorNear(QPointF page, const QColor& color)
    {
        const QImage image = window->grabWindow();
        const QPoint center = (view->pageToScene(page) * window->effectiveDevicePixelRatio()).toPoint();
        for (int dy = -3; dy <= 3; ++dy) {
            for (int dx = -3; dx <= 3; ++dx) {
                const QPoint p = center + QPoint(dx, dy);
                if (image.rect().contains(p) && near(image.pixelColor(p), color, 60)) {
                    return true;
                }
            }
        }
        return false;
    }
};

} // namespace

class TestTabs : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase() { QQuickWindow::setGraphicsApi(QSGRendererInterface::Software); }

    // Tout ce qui appartient au document part avec lui : la pile d'annulation est garée, puis rendue.
    void parkResumeKeepsUndoHistory()
    {
        Fixture f;
        QCOMPARE(f.tabs.rowCount(), 1);
        f.tabs.newTab();
        QCOMPARE(f.tabs.rowCount(), 2);
        QCOMPARE(f.tabs.currentIndex(), 1);
        QVERIFY(!f.document.canUndo()); // le même contrôleur montre le document neuf
        QCOMPARE(f.document.shownObjectCount(), 0);
        f.tabs.activate(0);
        QCOMPARE(f.tabs.currentIndex(), 0);
        QVERIFY(f.document.canUndo());
        QCOMPARE(f.document.shownObjectCount(), 1);
        f.document.undo();
        QCOMPARE(f.document.shownObjectCount(), 0);
    }

    void parkDropsALiftFirst()
    {
        Fixture f;
        const auto lifted = f.document.liftObjects({f.firstId()});
        QCOMPARE(lifted.size(), std::size_t(1));
        QVERIFY(f.document.isLifting());
        f.tabs.newTab();
        QVERIFY(!f.document.isLifting());
        f.tabs.activate(0);
        QVERIFY(!f.document.isLifting());
        QCOMPARE(f.document.shownObjectCount(), 1); // l'objet soulevé est revenu, sans étape
        f.document.undo();                          // la seule étape reste le trait
        QCOMPARE(f.document.shownObjectCount(), 0);
        QVERIFY(!f.document.canUndo());
    }

    // Risque visé : un trait encore dessiné par la couche d'attente ne reste jamais, en fantôme, sur la page
    // de l'autre onglet. Rouge si resumeSession ne rattache pas la page (attach → setSource → restart).
    void parkFoldsPendingFirst()
    {
        Fixture f(/*withView=*/true);
        QVERIFY(f.view);
        f.document.undo(); // onglet 0 vide
        QVERIFY(f.waitCacheStable());
        f.tabs.newTab(); // onglet 1, vide
        QVERIFY(f.waitCacheStable());
        f.tabs.activate(0);
        QVERIFY(f.waitCacheStable());
        f.drawStroke({100, 100}, {300, 100}); // trait rouge versé, encore en attente
        QCOMPARE(f.view->fixedPage()->pendingCount(), 1);
        QVERIFY(f.grabHasColorNear({200, 100}, Qt::red)); // dessiné par la couche d'attente
        f.tabs.activate(1);                               // bascule pendant l'attente
        QTest::qWait(ui::kCacheFoldIdleMs + 300);
        QCOMPARE(f.view->fixedPage()->pendingCount(), 0);
        QVERIFY(!f.grabHasColorNear({200, 100}, Qt::red)); // aucun fantôme sur la page de l'onglet 1
        f.tabs.activate(0);
        QVERIFY(f.waitCacheStable());
        QCOMPARE(f.document.shownObjectCount(), 1);
        QVERIFY(f.grabHasColorNear({200, 100}, Qt::red)); // le trait est revenu avec son document
    }

    // Test qui distingue (amendement 3, M1) : une bascule coûte UN rendu terminé (D21). Une demande de
    // redessin avant le garage (verser la couche d'attente) en donne deux ou trois (m_refreshAgain).
    void switchRedrawsOnce()
    {
        Fixture f(/*withView=*/true);
        QVERIFY(f.view);
        f.tabs.newTab();
        QVERIFY(f.waitCacheStable());
        const int before = f.view->cacheRefreshCount();
        f.tabs.activate(0);
        QTest::qWait(ui::kCacheFoldIdleMs + 300);
        QVERIFY(f.waitCacheStable());
        QCOMPARE(f.view->cacheRefreshCount(), before + 1);
        QVERIFY(f.grabHasColorNear({200, 100}, Qt::red));
    }

    void closingModifiedAsksToSave()
    {
        Fixture f;
        QVERIFY(f.document.isModified());
        QSignalSpy asked(&f.tabs, &ui::DocumentTabs::saveRequested);
        QVERIFY(!f.tabs.requestClose(0));
        QCOMPARE(asked.count(), 1);
        QCOMPARE(asked.at(0).at(0).toInt(), 0);
        QCOMPARE(f.tabs.data(f.tabs.index(0), ui::DocumentTabs::ModifiedRole).toBool(), true);
        f.tabs.closeWithoutSaving(0);
        QCOMPARE(f.tabs.rowCount(), 1); // un document vide reste toujours ouvert
        QCOMPARE(f.document.pageCount(), 1);
        QCOMPARE(f.document.shownObjectCount(), 0);
        QVERIFY(!f.document.isModified());
        QVERIFY(!f.document.canUndo());
        QCOMPARE(f.tabs.data(f.tabs.index(0), ui::DocumentTabs::TitleRole).toString(),
                 QStringLiteral("Sans titre"));
        QVERIFY(
            f.tabs.requestClose(0)); // document vide non modifié : fermé sans demande, remplacé par un vide
        QCOMPARE(asked.count(), 1);
        QCOMPARE(f.tabs.rowCount(), 1);
    }

    void closingAParkedTabKeepsTheShownDocument()
    {
        Fixture f;
        f.tabs.newTab(); // onglet 1 actif, onglet 0 garé et modifié
        QSignalSpy asked(&f.tabs, &ui::DocumentTabs::saveRequested);
        QVERIFY(!f.tabs.requestClose(0));
        QCOMPARE(asked.count(), 1);
        f.tabs.closeWithoutSaving(0);
        QCOMPARE(f.tabs.rowCount(), 1);
        QCOMPARE(f.tabs.currentIndex(), 0);
        QVERIFY(!f.document.canUndo()); // le document affiché (l'ancien onglet 1) n'a pas bougé
    }

    void copyPagesGoBeforeFirstPage()
    {
        Fixture f;         // onglet 0 : page 1 avec un trait
        f.document.undo(); // page 1 vide
        f.document.nextPage();
        f.drawStrokeOnShown();
        f.document.showPage(0);
        QCOMPARE(f.document.pageCount(), 2); // page 2 avec un trait
        f.tabs.newTab();                     // onglet 1 : 1 page, actif
        f.tabs.activate(0);
        f.tabs.copyPagesToTab({1}, 1);
        QCOMPARE(f.document.pageCount(), 2); // l'onglet de départ n'a pas changé
        f.tabs.activate(1);
        QCOMPARE(f.document.pageCount(), 2);
        QCOMPARE(f.document.currentIndex(), 0);
        QVERIFY(f.pageHasObjects(0)); // la page copiée est la première
        QVERIFY(!f.pageHasObjects(1));
        QVERIFY(f.document.isModified());
        f.document.undo(); // une étape dans l'onglet d'arrivée
        QCOMPARE(f.document.pageCount(), 1);
        QVERIFY(!f.document.canUndo());
    }

    void titlesFollowTheDocuments()
    {
        Fixture f;
        QSignalSpy count(&f.tabs, &ui::DocumentTabs::countChanged);
        f.tabs.newTab();
        QCOMPARE(count.count(), 1);
        QCOMPARE(f.tabs.data(f.tabs.index(0), ui::DocumentTabs::ModifiedRole).toBool(), true);
        QCOMPARE(f.tabs.data(f.tabs.index(1), ui::DocumentTabs::ModifiedRole).toBool(), false);
        QCOMPARE(f.tabs.data(f.tabs.index(1), ui::DocumentTabs::CurrentRole).toBool(), true);
        QCOMPARE(f.tabs.data(f.tabs.index(0), ui::DocumentTabs::CurrentRole).toBool(), false);
        f.drawStrokeOnShown();
        QCOMPARE(f.tabs.data(f.tabs.index(1), ui::DocumentTabs::ModifiedRole).toBool(), true);
        f.tabs.previous();
        QCOMPARE(f.tabs.currentIndex(), 0);
        f.tabs.next();
        QCOMPARE(f.tabs.currentIndex(), 1);
    }
};

QTEST_MAIN(TestTabs)
#include "tst_ui_tabs.moc"
