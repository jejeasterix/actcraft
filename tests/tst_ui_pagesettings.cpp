#include <actcraft/core/model.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/core/pagesettings.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageoptions.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlextensionplugin.h>

#include <cstddef>
#include <memory>
#include <utility>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

// Espion du cache de la page fixe (D-S7) : ne compte que ce qui lui parvient.
struct CacheSpy final : core::PageCache
{
    int inserts = 0;
    int removes = 0;
    int invalidates = 0;
    int calls() const { return inserts + removes + invalidates; }
    void insert(core::ObjectId) override { ++inserts; }
    void remove(core::ObjectId, const core::RectF&) override { ++removes; }
    void invalidate(core::ObjectId, const core::CacheArea&) override { ++invalidates; }
};

// Contrôleur réel (document de deux pages), réglages de page branchés, espion de cache.
struct Rig
{
    ui::DocumentController controller;
    ui::PageOptions options;
    CacheSpy spy;
    int token = 0;

    Rig()
    {
        controller.newDocument(1024, 768);
        controller.nextPage();
        controller.showPage(0);
        options.setDocumentController(&controller);
        token = controller.addChangeObserver(
            [this](const core::Changes& applied, ui::DocumentController::ChangeOrigin) {
                core::applyToCache(applied, static_cast<std::size_t>(controller.currentIndex()), spy);
            });
    }
    ~Rig() { controller.removeChangeObserver(token); }

    const core::Grid& grid(int page) const
    {
        return controller.document().page(static_cast<std::size_t>(page)).grid;
    }
    // Grille de départ posée par la pile, sans fusion ; l'espion repart de zéro.
    void seed(int page, bool above)
    {
        core::Grid g;
        g.visible = true;
        g.drawnAbove = above;
        g.levels.push_back(core::GridLevel{});
        controller.execute(std::make_unique<core::SetGridCommand>(static_cast<std::size_t>(page), g));
        controller.closeMerge();
        spy = CacheSpy{};
    }
    // Réglage du pas du premier niveau par la carte de nos clés.
    void setSpacing(double value)
    {
        QVariantMap map = options.grid();
        QVariantList levels = map.value(QStringLiteral("levels")).toList();
        QVariantMap level = levels.value(0).toMap();
        level.insert(QStringLiteral("spacingX"), value);
        level.insert(QStringLiteral("spacingY"), value);
        levels[0] = level;
        map.insert(QStringLiteral("levels"), levels);
        options.setGrid(map);
    }
    double spacing(int page) const { return grid(page).levels.at(0).spacingX; }
};

} // namespace

class TstUiPagesettings : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;

private slots:
    void initTestCase() { QVERIFY(m_dir.isValid()); }

    // La boîte réelle (Main.qml) : l'ouverture montre une grille, la fermeture clôt la séance en une étape.
    void designerDialogOpensAndClosesOneSession()
    {
        QQmlApplicationEngine engine;
        engine.setInitialProperties(
            {{QStringLiteral("cacheEnabled"), true}, {QStringLiteral("generatedPage"), false}});
        engine.loadFromModule("ActCraft.Ui", "Main");
        QVERIFY(!engine.rootObjects().isEmpty());
        auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
        QVERIFY(window);
        auto* tools = window->findChild<ui::ToolHost*>(QStringLiteral("appTools"));
        QVERIFY(tools);
        tools->setSettingsFile(m_dir.filePath(QStringLiteral("reglages.ini"))); // jamais le profil réel
        auto* options = window->findChild<ui::PageOptions*>(QStringLiteral("appPageSettings"));
        auto* document = window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        QObject* designer = window->findChild<QObject*>(QStringLiteral("gridDesigner"));
        QVERIFY(options && document && designer);
        QVERIFY(window->isVisible() || QTest::qWaitForWindowExposed(window));
        QVERIFY(!options->gridVisible());
        const bool undoBefore = document->canUndo();

        QVERIFY(QMetaObject::invokeMethod(designer, "open"));
        QTRY_VERIFY(designer->property("visible").toBool());
        QVERIFY(options->gridVisible());
        QVERIFY(!options->grid().value(QStringLiteral("levels")).toList().isEmpty());
        QVERIFY(QMetaObject::invokeMethod(designer, "close"));
        QTRY_VERIFY(!designer->property("visible").toBool());
        QVERIFY(options->gridVisible());
        document->undo(); // toute la séance en une étape
        QVERIFY(!options->gridVisible());
        QCOMPARE(document->canUndo(), undoBefore);
    }

    // Grille du dessus : la cacher ne redessine pas le cache (D-S7, P14).
    void hideGridAboveRefreshesNoCache()
    {
        Rig rig;
        rig.seed(0, true);
        QVERIFY(rig.options.gridVisible());
        QSignalSpy changed(&rig.options, &ui::PageOptions::changed);
        rig.options.toggleGridHidden();
        QVERIFY(!rig.options.gridVisible());
        QVERIFY(!rig.grid(0).visible);
        QCOMPARE(rig.spy.calls(), 0);
        QVERIFY(changed.count() >= 1);
    }

    // Grille du dessous : un redessin de toute la page.
    void hideGridBelowRefreshesOnce()
    {
        Rig rig;
        rig.seed(0, false);
        rig.options.toggleGridHidden();
        QVERIFY(!rig.options.gridVisible());
        QCOMPARE(rig.spy.invalidates, 1);
        QCOMPARE(rig.spy.calls(), 1);
    }

    // L'accrochage est une propriété de la page affichée : changer de page relit l'autre.
    void snapToggleFollowsThePage()
    {
        Rig rig;
        QVERIFY(!rig.options.gridSnap());
        rig.options.toggleSnap();
        QVERIFY(rig.options.gridSnap());
        QVERIFY(rig.grid(0).snapEnabled);
        QSignalSpy changed(&rig.options, &ui::PageOptions::changed);
        rig.controller.showPage(1);
        QVERIFY(changed.count() >= 1);
        QVERIFY(!rig.options.gridSnap());
        QVERIFY(!rig.grid(1).snapEnabled);
        rig.controller.showPage(0);
        QVERIFY(rig.options.gridSnap());
        // Annulable (P11).
        rig.controller.undo();
        QVERIFY(!rig.options.gridSnap());
    }

    void addRemoveResetLevels()
    {
        Rig rig;
        rig.seed(0, false);
        rig.options.toggleSnap();
        QCOMPARE(rig.grid(0).levels.size(), std::size_t{1});
        rig.options.addLevel();
        QCOMPARE(rig.grid(0).levels.size(), std::size_t{2});
        QCOMPARE(rig.options.grid().value(QStringLiteral("levels")).toList().size(), 2);
        rig.options.removeLevel(1);
        QCOMPARE(rig.grid(0).levels.size(), std::size_t{1});
        rig.options.removeLevel(7); // hors bornes : sans effet
        rig.options.removeLevel(0); // le dernier niveau reste
        QCOMPARE(rig.grid(0).levels.size(), std::size_t{1});
        rig.options.addLevel();
        rig.options.resetGrid();
        QCOMPARE(rig.grid(0).levels.size(), std::size_t{1});
        QVERIFY(!rig.options.gridSnap());
        QVERIFY(!rig.grid(0).snapEnabled);
    }

    void stepSpacingScalesTheLevels()
    {
        Rig rig;
        rig.seed(0, false);
        const double before = rig.spacing(0);
        rig.options.stepSpacing(1);
        QCOMPARE(rig.spacing(0), before * 1.5);
        rig.options.stepSpacing(-1);
        QCOMPARE(rig.spacing(0), before * 1.5 * 0.5);
    }

    void gridMapRoundTrips()
    {
        Rig rig;
        rig.seed(0, true);
        const QVariantMap map = rig.options.grid();
        QVERIFY(map.value(QStringLiteral("visible")).toBool());
        QVERIFY(map.value(QStringLiteral("above")).toBool());
        const core::Grid before = rig.grid(0);
        rig.options.setGrid(map); // identique : même grille
        QCOMPARE(rig.grid(0).levels.size(), before.levels.size());
        QCOMPARE(rig.grid(0).levels.at(0).color, before.levels.at(0).color);
        QCOMPARE(rig.grid(0).levels.at(0).spacingX, before.levels.at(0).spacingX);
        QCOMPARE(rig.grid(0).drawnAbove, before.drawnAbove);
    }

    // Trois réglages d'une séance, une seule étape : un undo() rend la grille d'origine.
    void designerSessionMakesASingleStep()
    {
        Rig rig;
        rig.seed(0, false);
        const double origin = rig.spacing(0);
        rig.options.beginDesigner();
        rig.setSpacing(origin + 8);
        rig.setSpacing(origin + 16);
        rig.setSpacing(origin + 24);
        QCOMPARE(rig.spacing(0), origin + 24);
        rig.options.endDesigner();
        rig.controller.undo();
        QCOMPARE(rig.spacing(0), origin);
        QVERIFY(rig.grid(0).visible); // la grille de départ, pas la page neuve
    }

    // Amendement 4 : deux séances font deux étapes.
    void twoDesignerSessionsMakeTwoSteps()
    {
        Rig rig;
        rig.seed(0, false);
        const double origin = rig.spacing(0);
        rig.options.beginDesigner();
        rig.setSpacing(origin + 8);
        rig.setSpacing(origin + 16);
        rig.options.endDesigner();
        rig.options.beginDesigner();
        rig.setSpacing(origin + 40);
        rig.options.endDesigner();
        QCOMPARE(rig.spacing(0), origin + 40);
        rig.controller.undo();
        QCOMPARE(rig.spacing(0), origin + 16);
        rig.controller.undo();
        QCOMPARE(rig.spacing(0), origin);
    }

    // Un réglage fait juste avant l'ouverture ne fusionne pas avec la séance.
    void settingBeforeTheSessionStaysApart()
    {
        Rig rig;
        rig.seed(0, false);
        rig.options.toggleSnap();
        const double origin = rig.spacing(0);
        rig.options.beginDesigner();
        rig.setSpacing(origin + 8);
        rig.options.endDesigner();
        rig.controller.undo();
        QVERIFY(rig.options.gridSnap());
        QCOMPARE(rig.spacing(0), origin);
        rig.controller.undo();
        QVERIFY(!rig.options.gridSnap());
    }
};

QTEST_MAIN(TstUiPagesettings)
#include "tst_ui_pagesettings.moc"
