#include <actcraft/core/model.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/input/sample.h>
#include <actcraft/render/liftlayeritem.h>
#include <actcraft/tools/tool.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <cstddef>
#include <variant>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

// Fenêtre principale hors écran (Main.qml), page vierge.
struct App
{
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
    ui::DocumentController* document = nullptr;
    ui::ToolHost* tools = nullptr;
    ui::Commands* commands = nullptr;

    const core::Page& page() const
    {
        return document->document().page(static_cast<std::size_t>(document->currentIndex()));
    }
    bool busy() const { return tools->activeTool() && tools->activeTool()->isBusy(); }
};

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

input::Sample sampleAt(QPointF page, input::Phase phase)
{
    input::Sample sample;
    sample.x = page.x();
    sample.y = page.y();
    sample.phase = phase;
    sample.device = input::DeviceKind::Mouse;
    return sample;
}

} // namespace

class TstUiShapes : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;

    QString freshFile(const char* name) const { return m_dir.filePath(QString::fromLatin1(name)); }

    bool start(App& app, const QString& settingsFile)
    {
        app.engine.setInitialProperties(
            {{QStringLiteral("cacheEnabled"), true}, {QStringLiteral("generatedPage"), false}});
        app.engine.loadFromModule("ActCraft.Ui", "Main");
        if (app.engine.rootObjects().isEmpty()) {
            return false;
        }
        app.window = qobject_cast<QQuickWindow*>(app.engine.rootObjects().constFirst());
        if (!app.window) {
            return false;
        }
        app.view = app.window->findChild<ui::PageView*>(QStringLiteral("pageView"));
        app.document = app.window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        app.tools = app.window->findChild<ui::ToolHost*>(QStringLiteral("appTools"));
        app.commands = app.window->findChild<ui::Commands*>(QStringLiteral("commands"));
        if (!app.view || !app.document || !app.tools || !app.commands) {
            return false;
        }
        app.tools->setSettingsFile(settingsFile); // jamais le profil réel
        if (!QTest::qWaitForWindowExposed(app.window)) {
            return false;
        }
        app.window->requestActivate();
        if (!QTest::qWaitForWindowActive(app.window)) {
            return false;
        }
        return QTest::qWaitFor([&] { return app.view->cacheReady(); }, 20000);
    }

    static void press(App& app, QPointF page)
    {
        app.tools->activeTool()->onSample(sampleAt(page, input::Phase::Press));
    }
    static void move(App& app, QPointF page)
    {
        app.tools->activeTool()->onSample(sampleAt(page, input::Phase::Move));
    }
    static void release(App& app, QPointF page)
    {
        app.tools->activeTool()->onSample(sampleAt(page, input::Phase::Release));
    }
    // Glisser complet de `from` à `to`, injecté dans l'outil actif.
    static void drag(App& app, QPointF from, QPointF to)
    {
        press(app, from);
        move(app, to);
        release(app, to);
    }

    static const core::Shape* lastShape(const App& app)
    {
        const auto& objects = app.page().objects;
        return objects.empty() ? nullptr : std::get_if<core::Shape>(&objects.back());
    }

private slots:
    void initTestCase() { QVERIFY(m_dir.isValid()); }

    // Choisir une forme dans la palette active l'outil des formes et coche sa commande.
    void shapeToolActivatesWithPalette()
    {
        App app;
        QVERIFY(start(app, freshFile("palette.ini")));
        QCOMPARE(app.tools->currentTool(), QStringLiteral("pen"));
        QVERIFY(!app.commands->isChecked(QStringLiteral("tool.shape")));
        app.tools->chooseShape(QStringLiteral("circle"));
        QCOMPARE(app.tools->currentTool(), QStringLiteral("shape"));
        QCOMPARE(app.tools->shapePreset(), QStringLiteral("circle"));
        QVERIFY(app.commands->isChecked(QStringLiteral("tool.shape")));
        QVERIFY(!app.commands->isChecked(QStringLiteral("tool.pen")));
        // Un préréglage inconnu ne change rien.
        app.tools->chooseShape(QStringLiteral("inconnu"));
        QCOMPARE(app.tools->shapePreset(), QStringLiteral("circle"));
        // L'ordre de la palette est celui du catalogue.
        const QStringList presets = app.tools->shapePresets();
        QCOMPARE(static_cast<std::size_t>(presets.size()), core::shapeCatalog().size());
        QCOMPARE(presets.constFirst(), QString::fromStdString(core::shapeCatalog().front().id));
        QCOMPARE(presets.constLast(), QString::fromStdString(core::shapeCatalog().back().id));
    }

    // Les choix de la palette sont gardés dans le fichier de réglages et relus au démarrage suivant.
    void presetChoiceSurvivesRestart()
    {
        const QString file = freshFile("redemarrage.ini");
        {
            App app;
            QVERIFY(start(app, file));
            app.tools->chooseShape(QStringLiteral("circle"));
            app.tools->setShapeFromCenter(true);
            app.tools->setShapeFilled(false);
            app.tools->setShapeFillColor(QColor(QStringLiteral("#336699")));
        }
        const QSettings probe(file, QSettings::IniFormat);
        QCOMPARE(probe.value(QStringLiteral("outils/shape/preset")).toString(), QStringLiteral("circle"));
        App app;
        QVERIFY(start(app, file));
        QCOMPARE(app.tools->shapePreset(), QStringLiteral("circle"));
        QVERIFY(app.tools->shapeFromCenter());
        QVERIFY(!app.tools->shapeFilled());
        QCOMPARE(app.tools->user().shapeSettings().fill, 0xFF336699u);
        // Fichier neuf : les défauts du lot (ligne, remplie, depuis le coin, orange).
        App fresh;
        QVERIFY(start(fresh, freshFile("neuf.ini")));
        QCOMPARE(fresh.tools->shapePreset(), QStringLiteral("line"));
        QVERIFY(fresh.tools->shapeFilled());
        QVERIFY(!fresh.tools->shapeFromCenter());
        QCOMPARE(fresh.tools->user().shapeSettings().fill, 0xFFFFAA00u);
    }

    // Chacun des 22 préréglages a un contour normalisé : deux points au moins, tous dans [0, 1]. Les trois
    // formes sans boîte (main levée, ligne et courbe brisées) rendent le segment de remplacement.
    void outlineOfEveryPreset()
    {
        App app;
        QVERIFY(start(app, freshFile("contours.ini")));
        QCOMPARE(core::shapeCatalog().size(), std::size_t{22});
        for (const core::ShapePreset& preset : core::shapeCatalog()) {
            const QString id = QString::fromStdString(preset.id);
            const QVariantList outline = app.tools->shapeOutline(id);
            QVERIFY2(outline.size() >= 2, qPrintable(id));
            for (const QVariant& value : outline) {
                const QVariantMap point = value.toMap();
                const double x = point.value(QStringLiteral("x")).toDouble();
                const double y = point.value(QStringLiteral("y")).toDouble();
                QVERIFY2(x >= 0.0 && x <= 1.0 && y >= 0.0 && y <= 1.0, qPrintable(id));
            }
        }
        for (const char* chain : {"freeform", "polyline", "curve-chain"}) {
            QCOMPARE(app.tools->shapeOutline(QString::fromLatin1(chain)).size(), 2);
        }
        QVERIFY(app.tools->shapeOutline(QStringLiteral("inconnu")).isEmpty());
    }

    // F-242 : le cadre de la sélection porte les points intelligents d'une forme sélectionnée seule.
    void smartPointsInSelectionFrame()
    {
        App app;
        QVERIFY(start(app, freshFile("points.ini")));
        const auto smartPoints = [&](const char* preset) {
            app.tools->chooseShape(QString::fromLatin1(preset));
            drag(app, QPointF(300, 300), QPointF(500, 420));
            app.tools->selectAll(); // une seule forme : sélectionnée seule
            const QVariantMap frame = app.tools->selectionFrame();
            app.tools->deleteSelection();
            return frame.value(QStringLiteral("smartPoints")).toList();
        };
        for (const char* preset : {"rounded-rectangle", "arrow", "callout"}) {
            QVERIFY2(!smartPoints(preset).isEmpty(), preset);
        }
        QVERIFY(smartPoints("star").isEmpty());
        // Sans sélection, la clé existe et est vide.
        QVERIFY(app.tools->selectionFrame().contains(QStringLiteral("smartPoints")));
        QVERIFY(app.tools->selectionFrame().value(QStringLiteral("smartPoints")).toList().isEmpty());
    }

    // L'outil ne touche jamais le cache de la page fixe pendant le tracé : l'aperçu est celui de la couche
    // soulevée ; au relâché, un seul insert.
    void shapeGoesThroughTheCacheInterface()
    {
        App app;
        QVERIFY(start(app, freshFile("cache.ini")));
        CacheSpy spy;
        app.document->addChangeObserver(
            [&](const core::Changes& changes, ui::DocumentController::ChangeOrigin) {
                core::applyToCache(changes, static_cast<std::size_t>(app.document->currentIndex()), spy);
            });
        app.tools->chooseShape(QStringLiteral("rectangle"));
        press(app, QPointF(100, 100));
        move(app, QPointF(150, 130));
        move(app, QPointF(200, 170));
        QVERIFY(app.busy());
        QCOMPARE(spy.calls(), 0);
        QCOMPARE(app.document->shownObjectCount(), 0);
        release(app, QPointF(200, 170));
        QCOMPARE(spy.inserts, 1);
        QCOMPARE(spy.calls(), 1);
        QCOMPARE(app.document->shownObjectCount(), 1);
    }

    // Amendement 5 (D5) : Maj atteint l'outil actif (relais du lot 3) et ne change rien au tracé : pas
    // d'interruption, pas de carré.
    void shiftReachesTheShapeTool()
    {
        App app;
        QVERIFY(start(app, freshFile("maj.ini")));
        app.tools->chooseShape(QStringLiteral("rectangle"));
        press(app, QPointF(100, 100));
        move(app, QPointF(200, 170));
        QTest::keyPress(app.view->window(), Qt::Key_Shift, Qt::ShiftModifier);
        QVERIFY(app.tools->activeTool()->isBusy());
        move(app, QPointF(200, 170));
        QTest::keyRelease(app.view->window(), Qt::Key_Shift, Qt::NoModifier);
        QVERIFY(app.tools->activeTool()->isBusy());
        release(app, QPointF(200, 170));
        QCOMPARE(app.document->shownObjectCount(), 1);
        const core::Shape* shape = lastShape(app);
        QVERIFY(shape);
        QCOMPARE(shape->bounds.width, 100.0);
        QCOMPARE(shape->bounds.height, 70.0);
    }

    // D2, D3 : remplie en orange, contour noir de 4, pour un fichier de réglages vide.
    void newShapeIsOrangeWithBlackOutline()
    {
        App app;
        QVERIFY(start(app, freshFile("orange.ini")));
        app.tools->chooseShape(QStringLiteral("rectangle"));
        drag(app, QPointF(100, 100), QPointF(260, 200));
        const core::Shape* shape = lastShape(app);
        QVERIFY(shape);
        QCOMPARE(shape->fill, 0xFFFFAA00u);
        QCOMPARE(shape->outline, 0xFF000000u);
        QCOMPARE(shape->outlineWidth, 4.0);
    }
};

QTEST_MAIN(TstUiShapes)
#include "tst_ui_shapes.moc"
