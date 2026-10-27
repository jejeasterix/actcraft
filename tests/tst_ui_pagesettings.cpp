#include <actcraft/core/model.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/core/pagesettings.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageoptions.h>
#include <actcraft/ui/pagescontroller.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTabletEvent>
#include <QTemporaryDir>
#include <QTest>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlextensionplugin.h>

#include <cmath>
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

// Espion du cache de la page fixe (D-S7) : ne compte que ce qui lui parvient.
struct CacheSpy final : core::PageCache
{
    int inserts = 0;
    int removes = 0;
    int invalidates = 0;
    int wholePage = 0; // invalidate(0, rien) : toute la page
    int calls() const { return inserts + removes + invalidates; }
    void insert(core::ObjectId) override { ++inserts; }
    void remove(core::ObjectId, const core::RectF&) override { ++removes; }
    void invalidate(core::ObjectId object, const core::CacheArea& area) override
    {
        ++invalidates;
        if (object == 0 && !area) {
            ++wholePage;
        }
    }
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

// Élément visuel nommé, cherché dans le contenu d'une fenêtre flottante (ses éléments ne sont pas enfants
// QObject de la fenêtre).
QObject* findItem(QObject* popup, const QString& name)
{
    std::vector<QQuickItem*> todo{popup->property("contentItem").value<QQuickItem*>()};
    while (!todo.empty()) {
        QQuickItem* item = todo.back();
        todo.pop_back();
        if (!item) {
            continue;
        }
        if (item->objectName() == name) {
            return item;
        }
        for (QQuickItem* child : item->childItems()) {
            todo.push_back(child);
        }
    }
    return nullptr;
}

// La fenêtre réelle (Main.qml), réglages dans un dossier jetable.
struct MainRig
{
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::ToolHost* tools = nullptr;
    ui::PageOptions* options = nullptr;
    ui::DocumentController* document = nullptr;
    ui::PagesController* pages = nullptr;
    ui::PageView* view = nullptr;
    QObject* dialog = nullptr;

    bool start(const QString& settingsFile)
    {
        engine.setInitialProperties(
            {{QStringLiteral("cacheEnabled"), true}, {QStringLiteral("generatedPage"), false}});
        engine.loadFromModule("ActCraft.Ui", "Main");
        if (engine.rootObjects().isEmpty()) {
            return false;
        }
        window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
        if (!window) {
            return false;
        }
        tools = window->findChild<ui::ToolHost*>(QStringLiteral("appTools"));
        if (!tools) {
            return false;
        }
        tools->setSettingsFile(settingsFile); // jamais le profil réel
        options = window->findChild<ui::PageOptions*>(QStringLiteral("appPageSettings"));
        document = window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        pages = window->findChild<ui::PagesController*>(QStringLiteral("appPages"));
        view = window->findChild<ui::PageView*>(QStringLiteral("pageView"));
        dialog = window->findChild<QObject*>(QStringLiteral("backgroundDialog"));
        return options && document && pages && view && dialog &&
               (window->isVisible() || QTest::qWaitForWindowExposed(window));
    }
    // Bouton de la boîte, par nom.
    QObject* button(const char* name) const { return findItem(dialog, QString::fromLatin1(name)); }
    bool click(const char* name) const
    {
        QObject* b = button(name);
        return b && QMetaObject::invokeMethod(b, "click");
    }
    bool openDialog() const
    {
        return QMetaObject::invokeMethod(dialog, "open") &&
               QTest::qWaitFor([this] { return dialog->property("visible").toBool(); }, 5000);
    }
    core::Argb fillColor(int page = 0) const
    {
        return document->document().page(static_cast<std::size_t>(page)).background.fill.color;
    }
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

    // Tous les champs hors défaut sur la page 0 ; la carte lue, appliquée à la page 1 (grille par défaut),
    // redonne la même grille champ par champ.
    void gridMapRoundTrips()
    {
        Rig rig;
        core::Grid source;
        source.visible = true;
        source.snapEnabled = true;
        source.drawnAbove = true;
        source.previewZoom = 1.75;
        source.baseZoom = 2.5;
        source.zoomIncrement = 0.25;
        core::GridLevel a;
        a.visible = false;
        a.snap = false;
        a.start = {7, 9};
        a.spacingX = 40;
        a.spacingY = 24;
        a.color = 0xFF2E7D32u;
        a.pattern = core::GridPattern::Angle;
        a.angle = 61;
        a.line = core::GridLine::Dashed;
        a.linesAlongX = false;
        a.linesAlongY = false;
        core::GridLevel b;
        b.start = {-3, 5};
        b.spacingX = 12;
        b.spacingY = 15;
        b.color = 0xFFD32F2Fu;
        b.pattern = core::GridPattern::Diagonal45;
        b.line = core::GridLine::BigDots;
        source.levels = {a, b};
        rig.controller.execute(std::make_unique<core::SetGridCommand>(std::size_t{0}, source));
        rig.controller.closeMerge();

        const QVariantMap map = rig.options.grid();
        rig.controller.showPage(1);
        QVERIFY(rig.grid(1).levels.empty()); // grille par défaut : rien à retrouver sans la carte
        rig.options.setGrid(map);

        const core::Grid& got = rig.grid(1);
        QCOMPARE(got.visible, source.visible);
        QCOMPARE(got.snapEnabled, source.snapEnabled);
        QCOMPARE(got.drawnAbove, source.drawnAbove);
        QCOMPARE(got.previewZoom, source.previewZoom);
        QCOMPARE(got.baseZoom, source.baseZoom);
        QCOMPARE(got.zoomIncrement, source.zoomIncrement);
        QCOMPARE(got.levels.size(), source.levels.size());
        for (std::size_t i = 0; i < source.levels.size(); ++i) {
            const core::GridLevel& x = got.levels.at(i);
            const core::GridLevel& y = source.levels.at(i);
            QCOMPARE(x.visible, y.visible);
            QCOMPARE(x.snap, y.snap);
            QCOMPARE(x.start.x, y.start.x);
            QCOMPARE(x.start.y, y.start.y);
            QCOMPARE(x.spacingX, y.spacingX);
            QCOMPARE(x.spacingY, y.spacingY);
            QCOMPARE(x.color, y.color);
            QCOMPARE(static_cast<int>(x.pattern), static_cast<int>(y.pattern));
            QCOMPARE(x.angle, y.angle);
            QCOMPARE(static_cast<int>(x.line), static_cast<int>(y.line));
            QCOMPARE(x.linesAlongX, y.linesAlongX);
            QCOMPARE(x.linesAlongY, y.linesAlongY);
        }
    }

    // Une valeur non finie (QML, fichier) ne passe pas : `std::clamp` laisserait passer un NaN.
    void nonFiniteValuesAreIgnored()
    {
        Rig rig;
        rig.seed(0, false);
        const core::Grid before = rig.grid(0);
        const double nan = std::numeric_limits<double>::quiet_NaN();
        const double inf = std::numeric_limits<double>::infinity();
        QVariantMap map = rig.options.grid();
        map.insert(QStringLiteral("previewZoom"), nan);
        map.insert(QStringLiteral("baseZoom"), inf);
        map.insert(QStringLiteral("zoomIncrement"), nan);
        QVariantMap level = map.value(QStringLiteral("levels")).toList().value(0).toMap();
        level.insert(QStringLiteral("spacingX"), nan);
        level.insert(QStringLiteral("spacingY"), inf);
        level.insert(QStringLiteral("startX"), nan);
        level.insert(QStringLiteral("startY"), -inf);
        level.insert(QStringLiteral("angle"), nan);
        map.insert(QStringLiteral("levels"), QVariantList{level});
        rig.options.setGrid(map);
        const core::Grid& after = rig.grid(0);
        QCOMPARE(after.previewZoom, before.previewZoom);
        QCOMPARE(after.baseZoom, before.baseZoom);
        QCOMPARE(after.zoomIncrement, before.zoomIncrement);
        const core::GridLevel& l = after.levels.at(0);
        QCOMPARE(l.spacingX, before.levels.at(0).spacingX);
        QCOMPARE(l.spacingY, before.levels.at(0).spacingY);
        QCOMPARE(l.start.x, before.levels.at(0).start.x);
        QCOMPARE(l.start.y, before.levels.at(0).start.y);
        QCOMPARE(l.angle, before.levels.at(0).angle);
    }

    // La boîte réelle : les coches de « Cacher la grille », « Aimanter » et « Poignée d'allongement » suivent
    // l'état (y compris l'annulation), le créateur règle l'angle libre, le bouton de niveau n'est pas à
    // bascule.
    void gridCommandsAreChecked()
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
        tools->setSettingsFile(
            m_dir.filePath(QStringLiteral("reglages-coches.ini"))); // jamais le profil réel
        auto* options = window->findChild<ui::PageOptions*>(QStringLiteral("appPageSettings"));
        auto* document = window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        auto* commands = window->findChild<ui::Commands*>(QStringLiteral("commands"));
        QObject* designer = window->findChild<QObject*>(QStringLiteral("gridDesigner"));
        QObject* handle = window->findChild<QObject*>(QStringLiteral("pageExtensionHandle"));
        QVERIFY(options && document && commands && designer && handle);
        QVERIFY(window->isVisible() || QTest::qWaitForWindowExposed(window));
        const QString hide = QStringLiteral("grid.hide");
        const QString snap = QStringLiteral("grid.snap");
        const QString extension = QStringLiteral("view.page-extension");
        QVERIFY(!commands->isChecked(hide)); // page sans grille : rien n'est caché
        QVERIFY(!commands->isChecked(snap));
        QVERIFY(!commands->isChecked(extension));

        // Une grille sur la page : le créateur l'ouvre.
        QVERIFY(commands->trigger(QStringLiteral("grid.designer")));
        QTRY_VERIFY(designer->property("visible").toBool());
        QVERIFY(options->gridVisible());
        QVERIFY(!commands->isChecked(hide));

        // Bouton de niveau : à pression simple, pas à bascule (un second clic ne le décoche pas).
        QObject* levelButton = findItem(designer, QStringLiteral("gridLevelButton0"));
        QVERIFY(levelButton);
        QVERIFY(!levelButton->property("checkable").toBool());

        // Angle libre : réglage visible seulement avec le style « Angle libre », écrit dans le niveau.
        QObject* angle = findItem(designer, QStringLiteral("gridAngle"));
        QVERIFY(angle);
        QVERIFY(!angle->property("visible").toBool()); // style « Normal »
        QVariantMap map = options->grid();
        QVariantList levels = map.value(QStringLiteral("levels")).toList();
        QVariantMap level = levels.value(0).toMap();
        level.insert(QStringLiteral("pattern"), static_cast<int>(core::GridPattern::Angle));
        levels[0] = level;
        map.insert(QStringLiteral("levels"), levels);
        options->setGrid(map);
        QTRY_VERIFY(angle->property("visible").toBool());
        QVERIFY(angle->setProperty("value", 60));
        QVERIFY(QMetaObject::invokeMethod(angle, "valueModified"));
        const auto angleOf = [&] {
            return options->grid()
                .value(QStringLiteral("levels"))
                .toList()
                .value(0)
                .toMap()
                .value(QStringLiteral("angle"))
                .toDouble();
        };
        QCOMPARE(angleOf(), 60.0);

        QVERIFY(QMetaObject::invokeMethod(designer, "close"));
        QTRY_VERIFY(!designer->property("visible").toBool());

        // Cacher la grille : la coche suit, y compris le retour.
        QVERIFY(commands->trigger(hide));
        QVERIFY(!options->gridVisible());
        QVERIFY(commands->isChecked(hide));
        QVERIFY(commands->trigger(hide));
        QVERIFY(options->gridVisible());
        QVERIFY(!commands->isChecked(hide));

        // Accrochage : la coche suit la commande et l'annulation.
        QVERIFY(commands->trigger(snap));
        QVERIFY(options->gridSnap());
        QVERIFY(commands->isChecked(snap));
        document->undo();
        QVERIFY(!options->gridSnap());
        QVERIFY(!commands->isChecked(snap));

        // Poignée d'allongement.
        QVERIFY(commands->trigger(extension));
        QVERIFY(handle->property("visible").toBool());
        QVERIFY(commands->isChecked(extension));
        QVERIFY(commands->trigger(extension));
        QVERIFY(!handle->property("visible").toBool());
        QVERIFY(!commands->isChecked(extension));
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

    // F-301 a, D10 cumulé (L4-O1) : trois clics font 768 × 1,1³ ; trois annulations rendent 768.
    void extendPageThreeTimes()
    {
        Rig rig;
        const auto height = [&] { return rig.controller.document().page(0).height; };
        QCOMPARE(rig.options.pageHeight(), 768.0);
        rig.options.extendPage();
        rig.options.extendPage();
        rig.options.extendPage();
        QVERIFY(std::abs(height() - 1022.208) < 1e-3);
        QVERIFY(std::abs(rig.options.pageHeight() - 1022.208) < 1e-3);
        rig.controller.undo();
        QVERIFY(std::abs(height() - 929.28) < 1e-3); // une annulation = une seule étape
        QVERIFY(std::abs(rig.options.pageHeight() - 929.28) < 1e-3);
        rig.controller.undo();
        rig.controller.undo();
        QCOMPARE(height(), 768.0);
        QCOMPARE(rig.options.pageHeight(), 768.0);
    }

    // La poignée (Main.qml) et la commande ne passent pas par l'outil courant : le stylo reste, un clic sur
    // la poignée allonge la page d'une étape.
    void extendKeepsTheCurrentTool()
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
        tools->setSettingsFile(
            m_dir.filePath(QStringLiteral("reglages-poignee.ini"))); // jamais le profil réel
        auto* options = window->findChild<ui::PageOptions*>(QStringLiteral("appPageSettings"));
        auto* commands = window->findChild<ui::Commands*>(QStringLiteral("commands"));
        auto* handle = window->findChild<QQuickItem*>(QStringLiteral("pageExtensionHandle"));
        QVERIFY(options && commands && handle);
        QVERIFY(window->isVisible() || QTest::qWaitForWindowExposed(window));
        auto* view = window->findChild<ui::PageView*>(QStringLiteral("pageView"));
        QVERIFY(view);
        QTRY_VERIFY(view->width() > 100 && view->height() > 100); // mise en page faite
        auto* controller = window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        QVERIFY(controller);
        const auto objectCount = [&] {
            return controller->document()
                .page(static_cast<std::size_t>(controller->currentIndex()))
                .objects.size();
        };
        const std::size_t objectsBefore = objectCount();
        tools->activate(QStringLiteral("pen"));
        QCOMPARE(tools->currentTool(), QStringLiteral("pen"));

        QVERIFY(commands->trigger(QStringLiteral("page.extend")));
        QVERIFY(std::abs(options->pageHeight() - 844.8) < 1e-3);
        QCOMPARE(tools->currentTool(), QStringLiteral("pen"));

        // Un clic sur la poignée montrée : une étape de plus, l'outil ne change pas.
        QVERIFY(commands->trigger(QStringLiteral("view.page-extension")));
        QTRY_VERIFY(handle->isVisible());
        QTRY_VERIFY(handle->property("atPageBottom").toBool());
        const QPointF inside = handle->mapToScene(QPointF(handle->width() / 2, handle->height() / 4));
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, inside.toPoint());
        QTRY_VERIFY(std::abs(options->pageHeight() - 929.28) < 1e-3);
        QCOMPARE(tools->currentTool(), QStringLiteral("pen"));
        QCOMPARE(objectCount(), objectsBefore); // le clic n'a posé aucun trait
        QTRY_VERIFY(handle->property("atPageBottom").toBool());
    }

    // La vue suit la nouvelle hauteur : zone de défilement et contenu à la hauteur de la page.
    void viewFollowsTheNewHeight()
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
        tools->setSettingsFile(m_dir.filePath(QStringLiteral("reglages-vue.ini"))); // jamais le profil réel
        auto* options = window->findChild<ui::PageOptions*>(QStringLiteral("appPageSettings"));
        auto* view = window->findChild<ui::PageView*>(QStringLiteral("pageView"));
        QVERIFY(options && view);
        QVERIFY(window->isVisible() || QTest::qWaitForWindowExposed(window));
        QTRY_VERIFY(view->width() > 100 && view->height() > 100); // mise en page faite
        view->setZoom(2.0); // hors ajustement, la page dépasse la vue : la zone de défilement dépend alors de
                            // la hauteur de la page
        const qreal before = view->scrollSizeY();
        QVERIFY(before > 0);
        options->extendPage();
        QTRY_VERIFY(view->scrollSizeY() < before - 1e-6);
        QVERIFY(std::abs(view->content()->height() - 844.8) < 1e-3);
        auto* controller = window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        QVERIFY(controller);
        controller->undo(); // l'annulation rend la hauteur d'origine à la vue
        QTRY_VERIFY(std::abs(view->content()->height() - 768.0) < 1e-3);
        QTRY_VERIFY(std::abs(view->scrollSizeY() - before) < 1e-6);
    }

    // Une page allongée change de taille sans changer de page ; un trait n'annonce rien de tel.
    void strokeDoesNotEmitPagesChanged()
    {
        Rig rig;
        QSignalSpy pages(&rig.controller, &ui::DocumentController::pagesChanged);
        const std::size_t before = rig.controller.document().page(0).objects.size();
        core::Stroke stroke;
        stroke.points = {{10, 10, 0.5f}, {40, 30, 0.5f}, {80, 20, 0.5f}};
        rig.controller.commitStroke(std::move(stroke));
        QCOMPARE(rig.controller.document().page(0).objects.size(), before + 1);
        QCOMPARE(pages.count(), 0);
        rig.options.extendPage(); // témoin : une page allongée, elle, l'annonce
        QVERIFY(pages.count() >= 1);
    }
    // F-610 : une couleur unie est une étape ; un undo rend le blanc.
    void backgroundColorIsOneStep()
    {
        Rig rig;
        const auto yellow = [&] { return rig.controller.document().page(0).background.fill; };
        QCOMPARE(yellow().color, core::Argb{0xFFFFFFFFu});
        rig.options.setBackground({{QStringLiteral("kind"), QStringLiteral("color")},
                                   {QStringLiteral("color"), QColor(QStringLiteral("#ffff00"))}});
        QCOMPARE(yellow().kind, core::FillKind::Solid);
        QCOMPARE(yellow().color, core::Argb{0xFFFFFF00u});
        QCOMPARE(rig.options.background().value(QStringLiteral("kind")).toString(), QStringLiteral("color"));
        QCOMPARE(rig.options.background().value(QStringLiteral("color")).value<QColor>(),
                 QColor(QStringLiteral("#ffff00")));
        rig.controller.undo();
        QCOMPARE(yellow().color, core::Argb{0xFFFFFFFFu});
    }

    // F-610 : deux couleurs et un sens font un dégradé.
    void backgroundGradient()
    {
        Rig rig;
        rig.options.setBackground({{QStringLiteral("kind"), QStringLiteral("gradient")},
                                   {QStringLiteral("color"), QColor(QStringLiteral("#ff0000"))},
                                   {QStringLiteral("color2"), QColor(QStringLiteral("#0000ff"))},
                                   {QStringLiteral("gradient"), QStringLiteral("horizontal")}});
        const core::Fill& fill = rig.controller.document().page(0).background.fill;
        QCOMPARE(fill.kind, core::FillKind::Gradient);
        QCOMPARE(fill.color, core::Argb{0xFFFF0000u});
        QCOMPARE(fill.color2, core::Argb{0xFF0000FFu});
        QCOMPARE(fill.gradient, core::GradientKind::Horizontal);
        const QVariantMap map = rig.options.background();
        QCOMPARE(map.value(QStringLiteral("kind")).toString(), QStringLiteral("gradient"));
        QCOMPARE(map.value(QStringLiteral("gradient")).toString(), QStringLiteral("horizontal"));
        QCOMPARE(map.value(QStringLiteral("color2")).value<QColor>(), QColor(QStringLiteral("#0000ff")));
    }

    // Un fond redessine toute la page, une seule fois.
    void backgroundRefreshesOnce()
    {
        Rig rig;
        QSignalSpy changed(&rig.options, &ui::PageOptions::changed);
        rig.options.setBackground({{QStringLiteral("kind"), QStringLiteral("color")},
                                   {QStringLiteral("color"), QColor(QStringLiteral("#00ff00"))}});
        QCOMPARE(rig.spy.invalidates, 1);
        QCOMPARE(rig.spy.wholePage, 1);
        QCOMPARE(rig.spy.calls(), 1);
        QVERIFY(changed.count() >= 1);
    }

    // D15 : chaque « appliquer » est sa propre étape (jaune puis vert : deux undo pour le blanc).
    void eachApplyIsOneStep()
    {
        Rig rig;
        const auto color = [&] { return rig.controller.document().page(0).background.fill.color; };
        rig.options.setBackground({{QStringLiteral("kind"), QStringLiteral("color")},
                                   {QStringLiteral("color"), QColor(QStringLiteral("#ffff00"))}});
        rig.options.setBackground({{QStringLiteral("kind"), QStringLiteral("color")},
                                   {QStringLiteral("color"), QColor(QStringLiteral("#00ff00"))}});
        QCOMPARE(color(), core::Argb{0xFF00FF00u});
        rig.controller.undo();
        QCOMPARE(color(), core::Argb{0xFFFFFF00u});
        rig.controller.undo();
        QCOMPARE(color(), core::Argb{0xFFFFFFFFu});
    }

    // Le fond vaut pour la page affichée.
    void backgroundTargetsTheShownPage()
    {
        Rig rig;
        rig.controller.showPage(1);
        rig.options.setBackground({{QStringLiteral("kind"), QStringLiteral("color")},
                                   {QStringLiteral("color"), QColor(QStringLiteral("#ffff00"))}});
        QCOMPARE(rig.controller.document().page(1).background.fill.color, core::Argb{0xFFFFFF00u});
        QCOMPARE(rig.controller.document().page(0).background.fill.color, core::Argb{0xFFFFFFFFu});
    }

    // D15 : la boîte réelle a trois boutons. « Appliquer » garde la boîte ouverte, « annuler » ferme sans
    // rien défaire de ce qui est appliqué, « valider » applique ce qui ne l'est pas puis ferme.
    void backgroundDialogThreeButtons()
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
        tools->setSettingsFile(m_dir.filePath(QStringLiteral("reglages-fond.ini"))); // jamais le profil réel
        auto* options = window->findChild<ui::PageOptions*>(QStringLiteral("appPageSettings"));
        auto* document = window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        QObject* dialog = window->findChild<QObject*>(QStringLiteral("backgroundDialog"));
        QVERIFY(options && document && dialog);
        QVERIFY(window->isVisible() || QTest::qWaitForWindowExposed(window));
        const auto fillColor = [&] { return document->document().page(0).background.fill.color; };
        const auto click = [&](const char* name) {
            auto* button = dialog->findChild<QObject*>(QString::fromLatin1(name));
            if (!button) { // le contenu d'un Popup n'est pas enfant QObject de la boîte
                button = findItem(dialog, QString::fromLatin1(name));
            }
            return button && QMetaObject::invokeMethod(button, "click");
        };

        QVERIFY(QMetaObject::invokeMethod(dialog, "open"));
        QTRY_VERIFY(dialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(dialog, "chooseColor", Q_ARG(QVariant, QStringLiteral("#ffff00"))));
        QVERIFY(click("backgroundApply"));
        QCOMPARE(fillColor(), core::Argb{0xFFFFFF00u});
        QVERIFY(dialog->property("visible").toBool()); // appliquer laisse la boîte ouverte
        QVERIFY(click("backgroundCancel"));
        QTRY_VERIFY(!dialog->property("visible").toBool());
        QCOMPARE(fillColor(), core::Argb{0xFFFFFF00u}); // ce qui est appliqué reste

        QVERIFY(QMetaObject::invokeMethod(dialog, "open"));
        QTRY_VERIFY(dialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(dialog, "chooseColor", Q_ARG(QVariant, QStringLiteral("#00ff00"))));
        QVERIFY(click("backgroundOk")); // valider applique ce qui ne l'est pas, puis ferme
        QTRY_VERIFY(!dialog->property("visible").toBool());
        QCOMPARE(fillColor(), core::Argb{0xFF00FF00u});
        document->undo(); // un seul pas pour « valider »
        QCOMPARE(fillColor(), core::Argb{0xFFFFFF00u});

        // Annuler sans rien appliquer ne touche à rien.
        QVERIFY(QMetaObject::invokeMethod(dialog, "open"));
        QTRY_VERIFY(dialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(dialog, "chooseColor", Q_ARG(QVariant, QStringLiteral("#ff0000"))));
        QVERIFY(click("backgroundCancel"));
        QTRY_VERIFY(!dialog->property("visible").toBool());
        QCOMPARE(fillColor(), core::Argb{0xFFFFFF00u});
    }

    // F-771 : le menu de page ouvert sur la page agit sur la page affichée, seule sélectionnée.
    void pageMenuSelectsTheShownPage()
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
        tools->setSettingsFile(m_dir.filePath(QStringLiteral("reglages-menu.ini")));
        auto* document = window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        auto* pages = window->findChild<ui::PagesController*>(QStringLiteral("appPages"));
        auto* view = window->findChild<ui::PageView*>(QStringLiteral("pageView"));
        QVERIFY(document && pages && view);
        QVERIFY(window->isVisible() || QTest::qWaitForWindowExposed(window));
        document->nextPage();
        document->nextPage();
        pages->click(0, 0);
        pages->click(2, Qt::ShiftModifier);
        QCOMPARE(pages->selection(), (QList<int>{0, 1, 2}));
        document->showPage(1);
        emit view->contextMenuRequested(QPointF(10, 10));
        QCOMPARE(pages->selection(), (QList<int>{1}));
        QCOMPARE(document->currentIndex(), 1);
    }
    // B1 : boîte ouverte, la page ne reçoit plus d'appui (inputBlocked) : un appui au stylet sur la boîte ne
    // pose pas d'encre dessous. Même défaut sur le créateur de grille.
    void dialogsBlockTheStylus()
    {
        MainRig r;
        QVERIFY(r.start(m_dir.filePath(QStringLiteral("reglages-stylet.ini"))));
        QObject* designer = r.window->findChild<QObject*>(QStringLiteral("gridDesigner"));
        QVERIFY(designer);
        QVERIFY(!r.view->inputBlocked());
        QVERIFY(QMetaObject::invokeMethod(designer, "open"));
        QTRY_VERIFY(r.view->inputBlocked());
        QVERIFY(QMetaObject::invokeMethod(designer, "close"));
        QTRY_VERIFY(!r.view->inputBlocked());

        r.tools->activate(QStringLiteral("pen"));
        const std::size_t before = r.document->document().page(0).objects.size();
        QVERIFY(r.openDialog());
        QVERIFY(r.view->inputBlocked());
        auto* apply = qobject_cast<QQuickItem*>(r.button("backgroundApply"));
        QVERIFY(apply);
        const QPointF at = apply->mapToScene(QPointF(apply->width() / 2, apply->height() / 2));
        QPointingDevice stylus(QStringLiteral("tst-fond-stylet"), 5161, QInputDevice::DeviceType::Stylus,
                               QPointingDevice::PointerType::Pen,
                               QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 0);
        const auto send = [&](QEvent::Type type, QPointF scene, Qt::MouseButtons buttons) {
            QTabletEvent event(type, &stylus, scene, r.window->mapToGlobal(scene), buttons ? 0.5 : 0.0, 0, 0,
                               0, 0, 0, Qt::NoModifier, Qt::LeftButton, buttons);
            QCoreApplication::sendEvent(r.window, &event);
        };
        send(QEvent::TabletPress, at, Qt::LeftButton);
        for (int i = 1; i <= 3; ++i) {
            send(QEvent::TabletMove, at + QPointF(i * 6, 0), Qt::LeftButton);
        }
        send(QEvent::TabletRelease, at + QPointF(18, 0), Qt::NoButton);
        QCOMPARE(r.document->document().page(0).objects.size(), before);
    }

    // M1 : « appliquer » sans rien changer ne touche pas au fond (image, genre non offert, sens gardés) et ne
    // pousse aucune étape.
    void applyWithoutChangeKeepsTheBackground()
    {
        MainRig r;
        QVERIFY(r.start(m_dir.filePath(QStringLiteral("reglages-fond-inchange.ini"))));
        core::Background background;
        background.fill.kind = core::FillKind::Gradient;
        background.fill.color = 0xFF112233u;
        background.fill.color2 = 0xFF445566u;
        background.fill.gradient = core::GradientKind::DiagonalDown;
        background.image = "fond.png";
        r.document->execute(std::make_unique<core::SetBackgroundCommand>(0, background));
        const std::string before = r.document->document().page(0).background.image;
        r.document->undo(); // la pile repart d'une étape de moins : on compare à l'état « défait »
        r.document->redo();
        const bool undoBefore = r.document->canUndo();
        QVERIFY(r.openDialog());
        QVERIFY(r.click("backgroundApply"));
        const core::Background& after = r.document->document().page(0).background;
        QCOMPARE(after.image, before);
        QCOMPARE(after.fill.kind, core::FillKind::Gradient);
        QCOMPARE(after.fill.gradient, core::GradientKind::DiagonalDown);
        QCOMPARE(after.fill.color, core::Argb{0xFF112233u});
        QCOMPARE(after.fill.color2, core::Argb{0xFF445566u});
        QCOMPARE(r.document->canUndo(), undoBefore);
        r.document->undo(); // l'étape défaite est bien la nôtre, pas un « appliquer » de plus
        QVERIFY(r.document->document().page(0).background.image.empty());
        QVERIFY(r.click("backgroundOk"));
        QTRY_VERIFY(!r.dialog->property("visible").toBool());
        QVERIFY(r.document->document().page(0).background.image.empty());
    }

    // Un fond identique ne fait pas d'étape.
    void identicalBackgroundPushesNoStep()
    {
        Rig rig;
        const QVariantMap yellow{{QStringLiteral("kind"), QStringLiteral("color")},
                                 {QStringLiteral("color"), QColor(QStringLiteral("#ffff00"))}};
        rig.options.setBackground(yellow);
        rig.controller.undo();
        rig.controller.redo();
        rig.spy = CacheSpy{};
        rig.options.setBackground(yellow);
        QCOMPARE(rig.spy.calls(), 0);
        rig.controller.undo(); // un seul pas : le blanc d'origine
        QCOMPARE(rig.controller.document().page(0).background.fill.color, core::Argb{0xFFFFFFFFu});
    }

    // Un genre que la boîte n'offre pas (image seule) n'est jamais posé par setBackground.
    void unknownKindIsIgnored()
    {
        Rig rig;
        const bool undoBefore = rig.controller.canUndo();
        rig.options.setBackground({{QStringLiteral("kind"), QStringLiteral("image")},
                                   {QStringLiteral("image"), QStringLiteral("x.png")}});
        QVERIFY(rig.controller.document().page(0).background.image.empty());
        QCOMPARE(rig.controller.canUndo(), undoBefore);
    }

    // Mineur 5 : la boîte relit la page à chaque ouverture ; un choix non appliqué puis annulé est oublié.
    void dialogReloadsThePageOnReopen()
    {
        MainRig r;
        QVERIFY(r.start(m_dir.filePath(QStringLiteral("reglages-fond-relit.ini"))));
        QVERIFY(r.openDialog());
        QVERIFY(
            QMetaObject::invokeMethod(r.dialog, "chooseColor", Q_ARG(QVariant, QStringLiteral("#ff0000"))));
        QVERIFY(r.click("backgroundCancel"));
        QTRY_VERIFY(!r.dialog->property("visible").toBool());
        QCOMPARE(r.fillColor(), core::Argb{0xFFFFFFFFu});
        QVERIFY(r.openDialog());
        QCOMPARE(r.dialog->property("color").value<QColor>(), QColor(QStringLiteral("#ffffff")));
        QVERIFY(r.dialog->property("applied").toBool());
        QVERIFY(r.click("backgroundCancel"));
    }

    // Mineur 5 : un dégradé passé par les boutons de la boîte (genre, deuxième couleur, sens).
    void dialogBuildsAGradientWithItsButtons()
    {
        MainRig r;
        QVERIFY(r.start(m_dir.filePath(QStringLiteral("reglages-fond-degrade.ini"))));
        QVERIFY(r.openDialog());
        QVERIFY(r.click("backgroundKindGradient"));
        QVERIFY(
            QMetaObject::invokeMethod(r.dialog, "chooseColor", Q_ARG(QVariant, QStringLiteral("#ff0000"))));
        QVERIFY(
            QMetaObject::invokeMethod(r.dialog, "chooseColor2", Q_ARG(QVariant, QStringLiteral("#0000ff"))));
        QVERIFY(r.click("backgroundHorizontal"));
        QVERIFY(r.click("backgroundApply"));
        const core::Fill& fill = r.document->document().page(0).background.fill;
        QCOMPARE(fill.kind, core::FillKind::Gradient);
        QCOMPARE(fill.color, core::Argb{0xFFFF0000u});
        QCOMPARE(fill.color2, core::Argb{0xFF0000FFu});
        QCOMPARE(fill.gradient, core::GradientKind::Horizontal);
        QVERIFY(r.dialog->property("applied").toBool());
        QVERIFY(r.click("backgroundHorizontal")); // un second clic ne décoche pas le sens
        QCOMPARE(r.dialog->property("direction").toString(), QStringLiteral("horizontal"));
        QVERIFY(r.dialog->property("applied").toBool()); // et ne change rien à appliquer
        QVERIFY(r.click("backgroundKindGradient"));
        QVERIFY(r.dialog->property("applied").toBool());
        QVERIFY(r.click("backgroundCancel"));
    }

    // M2 : le menu de page d'une miniature agit sur la page cliquée : elle devient la page affichée, et la
    // sélection multiple n'est pas réduite si la miniature en fait partie.
    void focusPageKeepsASelectionContainingIt()
    {
        Rig rig;
        rig.controller.nextPage(); // trois pages, affichée : 2
        ui::PagesController pages;
        pages.setDocumentController(&rig.controller);
        pages.click(0, 0);
        pages.click(1, Qt::ShiftModifier);
        QCOMPARE(pages.selection(), (QList<int>{0, 1}));
        QCOMPARE(rig.controller.currentIndex(), 1);
        QVERIFY(QMetaObject::invokeMethod(&pages, "focusPage", Q_ARG(int, 0)));
        QCOMPARE(pages.selection(), (QList<int>{0, 1}));
        QCOMPARE(rig.controller.currentIndex(), 0);
        QVERIFY(QMetaObject::invokeMethod(&pages, "focusPage", Q_ARG(int, 2)));
        QCOMPARE(pages.selection(), (QList<int>{2}));
        QCOMPARE(rig.controller.currentIndex(), 2);
    }
};

QTEST_MAIN(TstUiPagesettings)
#include "tst_ui_pagesettings.moc"
