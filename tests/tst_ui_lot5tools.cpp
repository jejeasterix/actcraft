#include <actcraft/core/connectors.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/core/undo.h>
#include <actcraft/tools/selecttool.h>
#include <actcraft/tools/tool.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QBuffer>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>
#include <QtQml/qqmlextensionplugin.h>

#include <cmath>
#include <memory>
#include <utility>
#include <variant>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

// Fenêtre principale hors écran (Main.qml), page vierge : comme tst_ui_tools.
struct App
{
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
    ui::DocumentController* document = nullptr;
    ui::ToolHost* tools = nullptr;
    ui::Commands* commands = nullptr;

    bool trigger(const char* id, const QVariant& argument = QVariant())
    {
        return commands->trigger(QString::fromLatin1(id), argument);
    }
    QPoint scene(QPointF page) const { return view->pageToScene(page).toPoint(); }
    const core::Page& page() const
    {
        return document->document().page(static_cast<std::size_t>(document->currentIndex()));
    }
    core::ObjectId idAt(std::size_t index) const { return page().objects.at(index).common.id; }
    const core::PageObject& object(core::ObjectId id) const
    {
        return page().objects.at(core::indexOf(page(), id));
    }
    core::ObjectId addShape(const core::RectF& box)
    {
        core::ShapeStyle style;
        style.fill = 0xFFFFFFFFu;
        document->execute(
            std::make_unique<core::AddObjectCommand>(0, core::makeShapeObject("rectangle", box, style)));
        return page().objects.back().common.id;
    }
    void mouseDrag(QPointF from, QPointF to, int steps = 10)
    {
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, scene(from));
        for (int i = 1; i <= steps; ++i) {
            QTest::mouseMove(window, scene(from + (to - from) * i / steps));
        }
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, scene(to));
    }
    void click(QPointF at) { QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, scene(at)); }
    tools::SelectTool* select() const { return dynamic_cast<tools::SelectTool*>(tools->activeTool()); }
};

const core::Connector* lastConnector(const App& app)
{
    for (auto it = app.page().objects.rbegin(); it != app.page().objects.rend(); ++it) {
        if (const auto* connector = std::get_if<core::Connector>(&*it)) {
            return connector;
        }
    }
    return nullptr;
}

// Délégués d'un Repeater : cherchés dans l'arbre des éléments visuels, pas dans celui des QObject.
QQuickItem* itemNamed(QQuickItem* root, const QString& name)
{
    if (root->objectName() == name) {
        return root;
    }
    for (QQuickItem* child : root->childItems()) {
        if (QQuickItem* found = itemNamed(child, name)) {
            return found;
        }
    }
    return nullptr;
}

} // namespace

class TstUiLot5Tools : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;

    bool start(App& app, const QString& name = QStringLiteral("vierge.ini"))
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
        if (!name.isEmpty()) {
            app.tools->setSettingsFile(m_dir.filePath(name)); // jamais le profil réel
        }
        if (!QTest::qWaitForWindowExposed(app.window)) {
            return false;
        }
        app.window->requestActivate();
        if (!QTest::qWaitForWindowActive(app.window)) {
            return false;
        }
        return QTest::qWaitFor([&] { return app.view->cacheReady(); }, 20000);
    }

    // Une image 20 x 20 blanche en ressource, posée sur (100, 100, 200, 200).
    static core::Document documentWithImage(core::ObjectId* imageId)
    {
        QImage image(20, 20, QImage::Format_ARGB32);
        image.fill(Qt::white);
        QByteArray png;
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        core::Document document;
        core::ImageObject object;
        object.bounds = {100, 100, 200, 200};
        object.resource = document.addResource(png, "png");
        core::PageObject placed{object};
        placed.common.id = document.allocateId();
        *imageId = placed.common.id;
        document.page(0).objects.push_back(std::move(placed));
        return document;
    }

    QImage imageOf(const App& app, core::ObjectId id) const
    {
        const auto* image = std::get_if<core::ImageObject>(&app.object(id));
        const core::Resource* resource = image ? app.document->document().resource(image->resource) : nullptr;
        return resource ? QImage::fromData(resource->data) : QImage();
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_dir.path());
    }

    // La commande tool.fill active le pot ; sa couleur est la sienne (celle du stylo ne bouge pas) et un
    // clic sur une image passe par le remplisseur injecté (render::floodFill).
    void fillToolActivatesFromCommand()
    {
        App app;
        QVERIFY(start(app));
        core::ObjectId imageId = 0;
        app.document->replaceDocument(documentWithImage(&imageId), QUrl(), QStringLiteral("image"));
        QVERIFY(QTest::qWaitFor([&] { return app.view->cacheReady(); }, 20000));
        const QColor toolColor = app.tools->color();

        QVERIFY(app.trigger("tool.fill"));
        QCOMPARE(app.tools->currentTool(), QStringLiteral("fill"));
        QVERIFY(app.commands->isChecked(QStringLiteral("tool.fill")));
        QVERIFY(!app.commands->isChecked(QStringLiteral("tool.pen")));

        app.tools->setColor(QColor(QStringLiteral("#00aa00")));
        QCOMPARE(app.tools->currentTool(),
                 QStringLiteral("fill")); // le pot garde sa couleur, sans changer d'outil
        QCOMPARE(app.tools->color(), QColor(QStringLiteral("#00aa00")));
        QCOMPARE(imageOf(app, imageId).pixelColor(10, 10), QColor(Qt::white));

        app.click(QPointF(200, 200));
        QTRY_COMPARE_WITH_TIMEOUT(imageOf(app, imageId).pixelColor(10, 10), QColor(QStringLiteral("#00aa00")),
                                  10000);

        QVERIFY(app.trigger("tool.pen"));
        QCOMPARE(app.tools->color(), toolColor); // la mémoire est par outil
        QVERIFY(app.trigger("tool.fill"));
        QCOMPARE(app.tools->color(), QColor(QStringLiteral("#00aa00")));
    }

    // La couleur du pot est gardée dans le fichier de réglages (mémoire par outil, F-136).
    void fillColorRemembered()
    {
        {
            App app;
            QVERIFY(start(app, QStringLiteral("pot.ini")));
            QVERIFY(app.trigger("tool.fill"));
            app.tools->setColor(QColor(QStringLiteral("#3366cc")));
        }
        App app;
        QVERIFY(start(app, QStringLiteral("pot.ini")));
        QVERIFY(app.trigger("tool.fill"));
        QCOMPARE(app.tools->color(), QColor(QStringLiteral("#3366cc")));
    }

    // F-221 : la modification de stylo ne vaut que sous le stylo et le marqueur ; le marqueur reçoit une
    // couleur à alpha (seule l'alpha de stroke.color rend un Highlighter translucide).
    void penModifierOnlyWithInkTools()
    {
        App app;
        QVERIFY(start(app));
        for (const char* other : {"tool.eraser", "tool.select", "tool.fill", "tool.connector"}) {
            QVERIFY(app.trigger(other));
            const QString before = app.tools->currentTool();
            app.tools->activate(QStringLiteral("pen-shape"), QStringLiteral("circle"));
            QCOMPARE(app.tools->currentTool(), before); // sans effet
        }

        QVERIFY(app.trigger("tool.pen"));
        app.tools->activate(QStringLiteral("pen-shape"), QStringLiteral("rectangle"));
        QCOMPARE(app.tools->currentTool(), QStringLiteral("pen-shape"));
        app.mouseDrag(QPointF(300, 300), QPointF(500, 400));
        QCOMPARE(app.page().objects.size(), std::size_t(1));
        const auto* pen = std::get_if<core::Stroke>(&app.page().objects.back());
        QVERIFY(pen != nullptr);
        QCOMPARE(pen->kind, core::StrokeKind::Pen);
        QCOMPARE(pen->color >> 24, core::Argb{255}); // le stylo reste opaque
        QVERIFY(pen->points.size() >= 4);

        app.tools->activate(QStringLiteral("pen-shape"), QStringLiteral("free")); // recliquer : trait libre
        QCOMPARE(app.tools->currentTool(), QStringLiteral("pen"));

        QVERIFY(app.trigger("tool.highlighter"));
        app.tools->activate(QStringLiteral("pen-shape"), QStringLiteral("rectangle"));
        QCOMPARE(app.tools->currentTool(), QStringLiteral("pen-shape"));
        app.mouseDrag(QPointF(300, 500), QPointF(500, 600));
        QCOMPARE(app.page().objects.size(), std::size_t(2));
        const auto* marker = std::get_if<core::Stroke>(&app.page().objects.back());
        QVERIFY(marker != nullptr);
        QCOMPARE(marker->kind, core::StrokeKind::Highlighter);
        QVERIFY2((marker->color >> 24) < 255u,
                 "le marqueur doit porter une alpha"); // translucide par l'alpha
        QVERIFY((marker->color >> 24) > 0u);
    }

    // La mémoire du connecteur (style) survit à la relance du ToolHost sur le même fichier de réglages.
    void connectorRemembersStyle()
    {
        {
            App app;
            QVERIFY(start(app, QStringLiteral("connecteur.ini")));
            app.tools->setConnectorStyle(5);
        }
        App app;
        QVERIFY(start(app, QStringLiteral("connecteur.ini")));
        const core::ObjectId a = app.addShape({100, 100, 100, 100});
        const core::ObjectId b = app.addShape({400, 100, 100, 100});
        const qreal widthBefore = app.view->width();
        const QPoint originBefore = app.scene({0, 0});
        QVERIFY(app.trigger("tool.connector"));
        QCOMPARE(app.tools->currentTool(), QStringLiteral("connector"));
        // L5-T18 : le panneau des connecteurs s'ouvre à côté de la page et la redimensionne au tour suivant
        // de la boucle d'événements ; on attend que la vue ait changé (taille ou position de la page) avant
        // de convertir les points en pixels.
        QTRY_VERIFY(app.view->width() != widthBefore || app.scene({0, 0}) != originBefore);
        app.mouseDrag(QPointF(198, 102), QPointF(402, 102));
        const core::Connector* connector = lastConnector(app);
        QVERIFY(connector != nullptr);
        QCOMPARE(connector->style, std::uint8_t{5});
        QCOMPARE(connector->objectA, a);
        QCOMPARE(connector->objectB, b);

        app.tools->setConnectorStyle(99); // borné au panneau (14 styles)
        app.tools->setConnectorStyle(-3);
        app.mouseDrag(QPointF(198, 198), QPointF(402, 198));
        QCOMPARE(lastConnector(app)->style, std::uint8_t{0});
    }

    // M3 (L5-C18, D11) : les extrémités A et B se règlent par connector.caps (« A,B », rangs 0 aucune, 1
    // rayon, 2 flèche), sont gardées comme le style et vont au connecteur tracé.
    void connectorRemembersCaps()
    {
        {
            App app;
            QVERIFY(start(app, QStringLiteral("extremites.ini")));
            QVERIFY(app.trigger("connector.caps", QStringLiteral("1,2")));
        }
        App app;
        QVERIFY(start(app, QStringLiteral("extremites.ini")));
        QCOMPARE(app.tools->property("connectorCapA").toInt(), 1);
        QCOMPARE(app.tools->property("connectorCapB").toInt(), 2);
        app.addShape({100, 100, 100, 100});
        app.addShape({400, 100, 100, 100});
        const qreal widthBefore = app.view->width();
        const QPoint originBefore = app.scene({0, 0});
        QVERIFY(app.trigger("tool.connector"));
        QTRY_VERIFY(app.view->width() != widthBefore || app.scene({0, 0}) != originBefore);
        app.mouseDrag(QPointF(198, 102), QPointF(402, 102));
        const core::Connector* connector = lastConnector(app);
        QVERIFY(connector != nullptr);
        QCOMPARE(connector->capA, core::ConnectorCap::Round);
        QCOMPARE(connector->capB, core::ConnectorCap::Arrow);

        QVERIFY(app.trigger("connector.caps", QStringLiteral("9,-1"))); // bornés aux trois formes
        QCOMPARE(app.tools->property("connectorCapA").toInt(), 2);
        QCOMPARE(app.tools->property("connectorCapB").toInt(), 0);
    }

    // M18 (L5-C18) : après relance, le panneau surligne le style et les extrémités gardés, pas le style 1.
    void connectorPanelShowsRememberedChoice()
    {
        {
            App app;
            QVERIFY(start(app, QStringLiteral("panneau.ini")));
            app.tools->setConnectorStyle(5);
            app.trigger("connector.caps", QStringLiteral("2,1"));
        }
        App app;
        QVERIFY(start(app, QStringLiteral("panneau.ini")));
        QObject* panel = app.window->findChild<QObject*>(QStringLiteral("connectorPanel"));
        QVERIFY(panel != nullptr);
        QCOMPARE(panel->property("chosen").toInt(),
                 5); // « Pointillés, flèche au départ », celui que dessine le connecteur
        QCOMPARE(panel->property("capA").toInt(), 2);
        QCOMPARE(panel->property("capB").toInt(), 1);
        app.tools->setConnectorStyle(8);
        QCOMPARE(panel->property("chosen").toInt(), 8); // le panneau suit la mémoire
    }

    // L5-C23 (E) : sans fichier de réglages posé (le cas de l'application), la mémoire se relit dans le
    // magasin par défaut à la première ouverture : le panneau montre le style et les extrémités gardés.
    void connectorPanelShowsRememberedChoiceFromTheDefaultStore()
    {
        // Comme l'application (main.cpp) : sans nom d'organisation, il n'y a pas de magasin par défaut.
        QCoreApplication::setOrganizationName(QStringLiteral("ActCraftTest"));
        struct Restore
        {
            ~Restore() { QCoreApplication::setOrganizationName(QString()); }
        } restore;
        {
            App app;
            QVERIFY(start(app, QString())); // magasin par défaut (redirigé vers le dossier du test)
            app.tools->setConnectorStyle(7);
            app.trigger("connector.caps", QStringLiteral("1,2"));
        }
        App app;
        QVERIFY(start(app, QString()));
        QObject* panel = app.window->findChild<QObject*>(QStringLiteral("connectorPanel"));
        QVERIFY(panel != nullptr);
        QCOMPARE(panel->property("chosen").toInt(), 7);
        QCOMPARE(panel->property("capA").toInt(), 1);
        QCOMPARE(panel->property("capB").toInt(), 2);
    }

    // L5-C3 : chaque style du panneau porte un libellé tiré de son dessin, plus de « Style N ».
    void connectorPanelNamesTheStyles()
    {
        App app;
        QVERIFY(start(app));
        const QStringList expected = {
            QStringLiteral("Ligne"),
            QStringLiteral("Flèche au départ"),
            QStringLiteral("Flèche à l'arrivée"),
            QStringLiteral("Deux flèches"),
            QStringLiteral("Pointillés"),
            QStringLiteral("Pointillés, flèche au départ"),
            QStringLiteral("Pointillés, flèche à l'arrivée"),
            QStringLiteral("Pointillés, deux flèches"),
            QStringLiteral("Courbe, départ vertical"),
            QStringLiteral("Courbe, départ horizontal"),
            QStringLiteral("Marches"),
            QStringLiteral("Coude"),
            QStringLiteral("Oblique puis horizontal"),
            QStringLiteral("Oblique puis horizontal, flèche au départ"),
        };
        for (int rank = 0; rank < core::kConnectorStyleCount; ++rank) {
            QQuickItem* entry =
                itemNamed(app.window->contentItem(), QStringLiteral("connectorStyle-%1").arg(rank));
            QVERIFY(entry != nullptr);
            QCOMPARE(entry->property("label").toString(), expected.at(rank));
            QVERIFY(itemNamed(entry, QStringLiteral("connectorSketch-%1").arg(rank)) != nullptr);
        }
    }

    // H5-33 : choisir un style au panneau (connector.style) règle les extrémités de l'outil selon ses flèches
    // (départ → A, arrivée → B : 2 flèche, sinon 0) ; elles restent réglables ensuite ; un connecteur déjà
    // posé garde les siennes ; le réglage du rang seul (relecture de la mémoire) ne touche pas aux
    // extrémités.
    void choosingAConnectorStyleSetsTheCaps()
    {
        App app;
        QVERIFY(start(app));
        app.document->execute(std::make_unique<core::AddObjectCommand>(
            0, core::makeConnector({100, 100}, 0, {300, 100}, 0, 0)));
        const core::ObjectId placed = app.page().objects.back().common.id;
        const auto caps = [&] {
            return std::pair{app.tools->property("connectorCapA").toInt(),
                             app.tools->property("connectorCapB").toInt()};
        };
        QVERIFY(app.trigger("connector.style", 3)); // deux flèches
        QCOMPARE(app.tools->connectorStyle(), 3);
        QCOMPARE(caps(), (std::pair{2, 2}));
        QVERIFY(app.trigger("connector.style", 13)); // oblique puis horizontal, flèche au départ
        QCOMPARE(caps(), (std::pair{2, 0}));
        QVERIFY(app.trigger("connector.caps", QStringLiteral("1,2"))); // toujours réglables
        QCOMPARE(caps(), (std::pair{1, 2}));
        QCOMPARE(app.tools->connectorStyle(), 13);
        QVERIFY(app.trigger("connector.style", 8)); // courbe sans flèche
        QCOMPARE(caps(), (std::pair{0, 0}));
        app.tools->setConnectorCaps(1, 1);
        app.tools->setConnectorStyle(6);
        QCOMPARE(caps(), (std::pair{1, 1}));
        const auto& kept = std::get<core::Connector>(app.object(placed));
        QCOMPARE(kept.capA, core::ConnectorCap::None);
        QCOMPARE(kept.capB, core::ConnectorCap::None);
        QCOMPARE(kept.style, std::uint8_t{0});
    }

    // L5-C3 : la vignette du panneau dessine le tracé de core (connectorPath) dans sa boîte, a en haut à
    // gauche, b en bas à droite, à 4 du bord ; flèches et pointillés du style.
    void connectorStyleSketchUsesTheSharedRoute()
    {
        App app;
        QVERIFY(start(app));
        const QVariantList step =
            app.tools->connectorStyleSketch(10, 44, 24).value(QStringLiteral("points")).toList();
        QCOMPARE(step.size(), 4);
        QCOMPARE(step.at(0).toPointF(), QPointF(4, 4));
        QCOMPARE(step.at(1).toPointF(), QPointF(22, 4));
        QCOMPARE(step.at(2).toPointF(), QPointF(22, 20));
        QCOMPARE(step.at(3).toPointF(), QPointF(40, 20));
        const QVariantList curve =
            app.tools->connectorStyleSketch(8, 44, 24).value(QStringLiteral("points")).toList();
        QCOMPARE(curve.size(), core::kConnectorCurveSegments + 1);
        const QVariantMap angle = app.tools->connectorStyleSketch(13, 44, 24);
        QVERIFY(angle.value(QStringLiteral("arrowStart")).toBool());
        QVERIFY(!angle.value(QStringLiteral("arrowEnd")).toBool());
        QCOMPARE(angle.value(QStringLiteral("outwardB")).toPointF(), QPointF(1, 0));
        QVERIFY(app.tools->connectorStyleSketch(4, 44, 24).value(QStringLiteral("dashed")).toBool());
        QVERIFY(!app.tools->connectorStyleSketch(8, 44, 24).value(QStringLiteral("dashed")).toBool());
    }

    // Échap (tool.select, au repos) et la désactivation quittent le mode points de forme.
    void escapeLeavesShapePoints()
    {
        App app;
        QVERIFY(start(app));
        const core::ObjectId a = app.addShape({100, 100, 100, 100});
        app.tools->selectObjects({a});
        QVERIFY(app.trigger("object.shape-points"));
        QCOMPARE(app.tools->currentTool(), QStringLiteral("select"));
        QVERIFY(app.select() != nullptr);
        QVERIFY(app.select()->shapePointsMode());

        QVERIFY(app.trigger("tool.select")); // Échap
        QVERIFY(!app.select()->shapePointsMode());
        QCOMPARE(app.tools->currentTool(), QStringLiteral("select"));

        QVERIFY(app.trigger("object.shape-points"));
        QVERIFY(app.select()->shapePointsMode());
        QVERIFY(app.trigger("tool.pen")); // deactivate() quitte le mode
        QVERIFY(app.trigger("tool.select"));
        QVERIFY(!app.select()->shapePointsMode());
    }

    // Glisser un objet relié : l'extrémité suit, en une étape (le dépôt est enveloppé dans l'outil, une
    // fois).
    void dropMovesLinkedConnectors()
    {
        App app;
        QVERIFY(start(app));
        const core::ObjectId a = app.addShape({100, 100, 100, 100});
        const core::ObjectId b = app.addShape({400, 100, 100, 100});
        app.document->execute(std::make_unique<core::AddObjectCommand>(
            0, core::makeConnector({200, 100}, a, {400, 100}, b, 0)));
        const core::ObjectId connectorId = app.page().objects.back().common.id;
        QVERIFY(app.trigger("tool.select"));
        app.tools->selectObjects({a});
        const double top = core::boundsOnPage(app.object(a)).y;

        app.mouseDrag(QPointF(150, 150), QPointF(150, 250));
        const double shift = core::boundsOnPage(app.object(a)).y - top;
        QVERIFY(shift > 50);
        const auto* moved = std::get_if<core::Connector>(&app.object(connectorId));
        QVERIFY(moved != nullptr);
        QVERIFY2(std::abs((moved->firstEnd.y - 100) - shift) < 0.01, "l'extrémité doit suivre l'objet");
        QCOMPARE(moved->secondEnd.y, 100.0); // l'autre bout ne bouge pas

        app.document->undo(); // une seule étape : l'objet et le connecteur reviennent
        QVERIFY(std::abs(core::boundsOnPage(app.object(a)).y - top) < 0.01);
        QCOMPARE(std::get<core::Connector>(app.object(connectorId)).firstEnd.y, 100.0);
    }

    // M2 (L5-C18) : le suivi se fait au point unique par où passe toute commande du document (le contrôleur),
    // pas commande par commande. Une commande brute, qu'aucun outil n'enveloppe, emmène le bout de l'objet
    // qu'elle déplace et détache celui de l'objet qu'elle retire, chaque fois en une étape.
    void anyCommandCarriesLinkedConnectors()
    {
        App app;
        QVERIFY(start(app));
        const core::ObjectId a = app.addShape({100, 100, 100, 100});
        const core::ObjectId b = app.addShape({400, 100, 100, 100});
        app.document->execute(std::make_unique<core::AddObjectCommand>(
            0, core::makeConnector({200, 100}, a, {400, 100}, b, 0)));
        const core::ObjectId connectorId = app.page().objects.back().common.id;

        const double topBefore = core::boundsOnPage(app.object(a)).y;
        app.document->execute(std::make_unique<core::TransformObjectsCommand>(
            0, std::vector<core::ObjectId>{a}, core::translation(0, 80)));
        {
            const auto& moved = std::get<core::Connector>(app.object(connectorId));
            QCOMPARE(moved.firstEnd.x, 200.0);
            QCOMPARE(moved.firstEnd.y, 180.0); // le bout suit le sommet déplacé
            QCOMPARE(moved.secondEnd.y, 100.0);
        }
        app.document->undo(); // une étape : l'objet et le bout reviennent ensemble
        QCOMPARE(std::get<core::Connector>(app.object(connectorId)).firstEnd.y, 100.0);
        QCOMPARE(core::boundsOnPage(app.object(a)).y, topBefore); // emprise d'avant, contour compris

        app.document->execute(
            std::make_unique<core::RemoveObjectsCommand>(0, std::vector<core::ObjectId>{a}));
        {
            const auto& detached = std::get<core::Connector>(app.object(connectorId));
            QCOMPARE(detached.objectA, core::ObjectId{0}); // l'objet retiré ne retient plus le bout
            QCOMPARE(detached.objectB, b);
            QCOMPARE(detached.firstEnd.x, 200.0); // le bout reste où il est
        }
        app.document->undo();
        QCOMPARE(std::get<core::Connector>(app.object(connectorId)).objectA, a);
        QCOMPARE(app.page().objects.size(), std::size_t(3));
    }

    // M2 : les commandes de la sélection (object.enlarge, edit.delete) et la corbeille emmènent aussi les
    // bouts.
    void selectionCommandsCarryLinkedConnectors()
    {
        App app;
        QVERIFY(start(app));
        const core::ObjectId a = app.addShape({100, 100, 100, 100});
        const core::ObjectId b = app.addShape({400, 100, 100, 100});
        app.document->execute(std::make_unique<core::AddObjectCommand>(
            0, core::makeConnector({200, 100}, a, {400, 100}, b, 0)));
        const core::ObjectId connectorId = app.page().objects.back().common.id;
        QVERIFY(app.trigger("tool.select"));
        app.tools->selectObjects({a});

        QVERIFY(app.trigger("object.enlarge"));
        const core::PointF corner = core::attachPoints(app.object(a)).at(1); // sommet en haut à droite
        QVERIFY(corner.x != 200.0 || corner.y != 100.0);
        {
            const auto& followed = std::get<core::Connector>(app.object(connectorId));
            QVERIFY2(std::abs(followed.firstEnd.x - corner.x) < 0.01 &&
                         std::abs(followed.firstEnd.y - corner.y) < 0.01,
                     "le bout doit suivre le sommet agrandi");
            QCOMPARE(followed.secondEnd.x, 400.0);
        }

        app.tools->selectObjects({a});
        QVERIFY(app.trigger("edit.delete"));
        QCOMPARE(std::get<core::Connector>(app.object(connectorId)).objectA, core::ObjectId{0});
        app.document->undo();
        QCOMPARE(std::get<core::Connector>(app.object(connectorId)).objectA, a);

        app.document->deleteObjects({b}); // la corbeille
        QCOMPARE(std::get<core::Connector>(app.object(connectorId)).objectB, core::ObjectId{0});
    }

    // M2 (doute du relecteur, tooladapters.cpp:77) : le seul connecteur lié, soulevé avec son objet A, garde
    // son bout B sur B au dépôt ; le suivi ne se décide pas sur la page privée des objets soulevés.
    void connectorLiftedWithItsObjectKeepsOtherEnd()
    {
        App app;
        QVERIFY(start(app));
        const core::ObjectId a = app.addShape({100, 100, 100, 100});
        const core::ObjectId b = app.addShape({400, 100, 100, 100});
        app.document->execute(std::make_unique<core::AddObjectCommand>(
            0, core::makeConnector({200, 100}, a, {400, 100}, b, 0)));
        const core::ObjectId connectorId = app.page().objects.back().common.id;
        QVERIFY(app.trigger("tool.select"));
        app.tools->selectObjects({a, connectorId});
        const double top = core::boundsOnPage(app.object(a)).y;

        app.mouseDrag(QPointF(150, 150), QPointF(150, 250));
        const double shift = core::boundsOnPage(app.object(a)).y - top;
        QVERIFY(shift > 50);
        const core::PageObject& object = app.object(connectorId);
        const auto& connector = std::get<core::Connector>(object);
        const core::PointF firstTip = object.common.transform.map(connector.firstEnd);
        const core::PointF secondTip = object.common.transform.map(connector.secondEnd);
        QVERIFY2(std::abs(firstTip.y - (100 + shift)) < 0.01, "le bout A suit A");
        QVERIFY2(std::abs(secondTip.x - 400) < 0.01 && std::abs(secondTip.y - 100) < 0.01,
                 "le bout B reste sur B");
        QCOMPARE(connector.objectB, b);
    }
};

QTEST_MAIN(TstUiLot5Tools)
#include "tst_ui_lot5tools.moc"
