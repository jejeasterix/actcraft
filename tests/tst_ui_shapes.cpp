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
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QSignalSpy>
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

// Élément d'interface par son objectName, cherché dans l'arbre des éléments (délégués de Repeater compris).
QQuickItem* findItem(QQuickItem* root, const QString& name)
{
    for (QQuickItem* child : root->childItems()) {
        if (child->objectName() == name) {
            return child;
        }
        if (QQuickItem* found = findItem(child, name)) {
            return found;
        }
    }
    return nullptr;
}

core::Argb argb(const QColor& color)
{
    return static_cast<core::Argb>(color.rgba());
}

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

    // Case `index` du nuancier des formes, une fois la palette posée par la disposition et la case placée.
    static QQuickItem* swatchAt(const App& app, int index)
    {
        QQuickItem* item = findItem(app.window->contentItem(), QStringLiteral("shape-swatch-%1").arg(index));
        if (!item || !QTest::qWaitFor([&] { return item->isVisible() && item->width() > 0; }, 5000)) {
            return nullptr;
        }
        return item;
    }

    // Vrai clic de souris au centre de l'élément (TapHandler de la case).
    static void tap(const App& app, const QQuickItem* item)
    {
        const QPointF centre = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
        QTest::mouseClick(app.window, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
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
        const QSignalSpy resized(app.view, &QQuickItem::widthChanged);
        app.tools->chooseShape(QStringLiteral("rectangle"));
        // L4-T12 : la palette des formes s'insère dans la disposition au choix de l'outil et redimensionne la
        // zone de page à la prochaine passe de disposition. Un geste en cours est abandonné par ce
        // redimensionnement ; QTest::keyPress fait tourner la boucle d'événements en plein geste. Dans
        // l'application, la palette est là avant le premier appui.
        QQuickItem* palette = app.window->findChild<QQuickItem*>(QStringLiteral("shapePalette"));
        QVERIFY(palette);
        // m6 (L4-C7) : attente exacte de la passe de disposition qui redimensionne la vue
        // (PageView::geometryChange), au lieu d'un délai de 100 ms.
        QVERIFY(QTest::qWaitFor(
            [&] { return palette->isVisible() && palette->width() > 0 && resized.count() > 0; }, 5000));
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

    // L4-C15 : la miniature de la forme courante accompagne le pointeur pendant le glisser. Qt Quick ne met
    // pas à jour un HoverHandler à bouton enfoncé : sans autre source, elle restait au point d'appui.
    void thumbnailFollowsThePointerWhileDragging()
    {
        App app;
        QVERIFY(start(app, freshFile("miniature.ini")));
        const QSignalSpy resized(app.view, &QQuickItem::widthChanged);
        app.tools->chooseShape(QStringLiteral("rectangle"));
        QQuickItem* palette = app.window->findChild<QQuickItem*>(QStringLiteral("shapePalette"));
        QVERIFY(palette);
        QVERIFY(QTest::qWaitFor(
            [&] { return palette->isVisible() && palette->width() > 0 && resized.count() > 0; }, 5000));
        QQuickItem* thumbnail = findItem(app.window->contentItem(), QStringLiteral("shapeCursorThumbnail"));
        QVERIFY(thumbnail);
        const QPoint start = app.view->pageToScene(QPointF(300, 300)).toPoint();
        const QPoint middle = app.view->pageToScene(QPointF(420, 350)).toPoint();
        const QPoint end = app.view->pageToScene(QPointF(560, 430)).toPoint();
        QTest::mouseMove(app.window, start);
        QTRY_VERIFY(thumbnail->isVisible());
        QTest::mousePress(app.window, Qt::LeftButton, Qt::NoModifier, start);
        QVERIFY(app.busy());
        for (const QPoint& target : {middle, end}) {
            QTest::mouseMove(app.window, target);
            QVERIFY(thumbnail->isVisible());
            QCOMPARE(thumbnail->mapToScene(QPointF(0, 0)).toPoint() - QPoint(14, 14), target);
        }
        QTest::mouseRelease(app.window, Qt::LeftButton, Qt::NoModifier, end);
        // Les outils reçoivent toujours le geste complet : la forme est tracée du point d'appui au relâché.
        QCOMPARE(app.document->shownObjectCount(), 1);
    }

    // L4-C15 : même cause pour le cercle de largeur (F-138) du marqueur et de la gomme, dont le curseur
    // système est masqué : il doit suivre la souris pendant le glisser.
    void widthIndicatorFollowsThePointerWhileDragging()
    {
        App app;
        QVERIFY(start(app, freshFile("cercle.ini")));
        QQuickItem* indicator = findItem(app.window->contentItem(), QStringLiteral("widthIndicator"));
        QVERIFY(indicator);
        for (const char* tool : {"highlighter", "eraser"}) {
            app.tools->activate(QString::fromLatin1(tool));
            const QPoint start = app.view->pageToScene(QPointF(300, 300)).toPoint();
            const QPoint end = app.view->pageToScene(QPointF(520, 380)).toPoint();
            QTest::mouseMove(app.window, start);
            QTRY_VERIFY(indicator->isVisible());
            QTest::mousePress(app.window, Qt::LeftButton, Qt::NoModifier, start);
            QTest::mouseMove(app.window, end);
            QVERIFY2(indicator->isVisible(), tool);
            const QPointF centre =
                indicator->mapToScene(QPointF(indicator->width() / 2, indicator->height() / 2));
            QVERIFY2((centre - QPointF(end)).manhattanLength() < 1.5, tool);
            QTest::mouseRelease(app.window, Qt::LeftButton, Qt::NoModifier, end);
        }
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

    // L4-C7 (M-A, N1, C-M2) : le nuancier 2 × 4 de la palette des formes règle le remplissage. 1re case : le
    // défaut de l'outil, surlignée au départ ; 8e case : « sans remplissage » (une valeur du remplissage, la
    // couleur gardée). Un clic garde l'outil des formes et ne touche ni le contour de la forme ni le stylo.
    // Router la case par setColor (patron écarté) basculerait sur le stylo, ou écrirait le contour.
    void swatchSetsTheFill()
    {
        App app;
        QVERIFY(start(app, freshFile("nuancier.ini")));
        app.tools->chooseShape(QStringLiteral("rectangle"));
        const core::Argb pen = app.tools->user().settings(QStringLiteral("pen")).color;
        const core::Argb outline = app.tools->user().settings(QStringLiteral("shape")).color;
        QQuickItem* first = swatchAt(app, 0);
        QQuickItem* chosen = swatchAt(app, 4);
        QQuickItem* unfilled = swatchAt(app, 7);
        QVERIFY(first && chosen && unfilled);
        QCOMPARE(first->property("color").value<QColor>(), QColor(QStringLiteral("#FFAA00")));
        QVERIFY(first->property("current").toBool());
        QVERIFY(!unfilled->property("current").toBool());

        app.tools->setShapeFilled(false); // une couleur choisie remplit de nouveau
        const QColor colour = chosen->property("color").value<QColor>();
        QVERIFY(argb(colour) != app.tools->user().shapeSettings().fill);
        tap(app, chosen);
        QCOMPARE(app.tools->currentTool(), QStringLiteral("shape"));
        QVERIFY(app.tools->shapeFilled());
        QCOMPARE(app.tools->user().shapeSettings().fill, argb(colour));
        QCOMPARE(app.tools->user().settings(QStringLiteral("shape")).color, outline);
        QCOMPARE(app.tools->user().settings(QStringLiteral("pen")).color, pen);
        QVERIFY(chosen->property("current").toBool());
        QVERIFY(!first->property("current").toBool());
        drag(app, QPointF(100, 100), QPointF(260, 200));
        const core::Shape* shape = lastShape(app);
        QVERIFY(shape);
        QCOMPARE(shape->fill, argb(colour));
        QCOMPARE(shape->outline, outline);

        tap(app, unfilled);
        QCOMPARE(app.tools->currentTool(), QStringLiteral("shape"));
        QVERIFY(!app.tools->shapeFilled());
        QCOMPARE(app.tools->user().shapeSettings().fill, argb(colour));
        QVERIFY(unfilled->property("current").toBool());
        QVERIFY(!chosen->property("current").toBool());
        QCOMPARE(app.tools->user().settings(QStringLiteral("pen")).color, pen);
    }

    // L4-C7 (N1, P6) : outil des formes actif, la palette principale règle le contour de l'outil des formes
    // (réglage propre, gardé dans le fichier) au lieu de basculer sur le stylo. Sous Sélectionner, rien ne
    // change : la couleur va au dernier outil d'encre (L3-T0).
    void mainPaletteSetsTheShapeOutline()
    {
        const QString file = freshFile("contour.ini");
        App app;
        QVERIFY(start(app, file));
        app.tools->chooseShape(QStringLiteral("rectangle"));
        const core::Argb pen = app.tools->user().settings(QStringLiteral("pen")).color;
        app.tools->setColor(QColor(QStringLiteral("#1E88E5")));
        QCOMPARE(app.tools->currentTool(), QStringLiteral("shape"));
        QCOMPARE(app.tools->user().settings(QStringLiteral("shape")).color, 0xFF1E88E5u);
        QCOMPARE(app.tools->user().settings(QStringLiteral("pen")).color, pen);
        QCOMPARE(app.tools->color(),
                 QColor(QStringLiteral("#1E88E5"))); // la palette principale suit le contour
        drag(app, QPointF(100, 100), QPointF(260, 200));
        const core::Shape* shape = lastShape(app);
        QVERIFY(shape);
        QCOMPARE(shape->outline, 0xFF1E88E5u);
        QCOMPARE(shape->fill, 0xFFFFAA00u);
        const QSettings probe(file, QSettings::IniFormat);
        QCOMPARE(probe.value(QStringLiteral("outils/shape/couleur")).toString(), QStringLiteral("#1e88e5"));

        app.tools->activate(QStringLiteral("select"));
        app.tools->setColor(QColor(QStringLiteral("#E53935")));
        QCOMPARE(app.tools->currentTool(), QStringLiteral("pen"));
        QCOMPARE(app.tools->user().settings(QStringLiteral("pen")).color, 0xFFE53935u);
        QCOMPARE(app.tools->user().settings(QStringLiteral("shape")).color, 0xFF1E88E5u);
    }
};

QTEST_MAIN(TstUiShapes)
#include "tst_ui_shapes.moc"
