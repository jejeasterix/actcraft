#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/render/liftlayeritem.h>
#include <actcraft/tools/pentool.h>
#include <actcraft/tools/tool.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QDir>
#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QScopedPointer>
#include <QSettings>
#include <QTabletEvent>
#include <QTemporaryDir>
#include <QTest>
#include <QVariant>
#include <QtQml/qqmlextensionplugin.h>

#include <memory>
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
    render::LiftLayerItem* lift = nullptr;
    QStringList journal; // « r:<id>:Lift »… un par changement, posé par observe()

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
    bool busy() const { return tools->activeTool() && tools->activeTool()->isBusy(); }
};

QString originName(ui::DocumentController::ChangeOrigin origin)
{
    switch (origin) {
    case ui::DocumentController::ChangeOrigin::Command:
        return QStringLiteral("Command");
    case ui::DocumentController::ChangeOrigin::Undo:
        return QStringLiteral("Undo");
    case ui::DocumentController::ChangeOrigin::Redo:
        return QStringLiteral("Redo");
    case ui::DocumentController::ChangeOrigin::Stroke:
        return QStringLiteral("Stroke");
    case ui::DocumentController::ChangeOrigin::Lift:
        return QStringLiteral("Lift");
    case ui::DocumentController::ChangeOrigin::Drop:
        return QStringLiteral("Drop");
    }
    return QStringLiteral("?");
}

QString kindName(core::Change::Kind kind)
{
    switch (kind) {
    case core::Change::Kind::ObjectInserted:
        return QStringLiteral("i");
    case core::Change::Kind::ObjectRemoved:
        return QStringLiteral("r");
    case core::Change::Kind::ObjectChanged:
        return QStringLiteral("v");
    case core::Change::Kind::PageInserted:
    case core::Change::Kind::PageRemoved:
    case core::Change::Kind::PageChanged:
    case core::Change::Kind::PageSettingsChanged:
        return QStringLiteral("p");
    }
    return QStringLiteral("?");
}

void observe(App& app)
{
    app.document->addChangeObserver(
        [&app](const core::Changes& applied, ui::DocumentController::ChangeOrigin origin) {
            for (const core::Change& change : applied) {
                app.journal.push_back(QStringLiteral("%1:%2:%3")
                                          .arg(kindName(change.kind))
                                          .arg(change.object)
                                          .arg(originName(origin)));
            }
        });
}

QString entry(const char* kind, core::ObjectId id, const char* origin)
{
    return QStringLiteral("%1:%2:%3").arg(QString::fromLatin1(kind)).arg(id).arg(QString::fromLatin1(origin));
}

} // namespace

class TstUiTools : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_settingsDir;
    std::unique_ptr<QPointingDevice> m_stylus;
    QPointingDevice* m_touch = nullptr;

    bool start(App& app, const QString& settingsFile = QString())
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
        app.lift = app.window->findChild<render::LiftLayerItem*>(QStringLiteral("liftLayer"));
        if (!app.view || !app.document || !app.tools || !app.commands || !app.lift) {
            return false;
        }
        // Jamais le profil réel : un fichier sous le dossier temporaire du test.
        app.tools->setSettingsFile(
            settingsFile.isEmpty() ? m_settingsDir.filePath(QStringLiteral("vierge.ini")) : settingsFile);
        if (!QTest::qWaitForWindowExposed(app.window)) {
            return false;
        }
        app.window->requestActivate();
        if (!QTest::qWaitForWindowActive(app.window)) {
            return false;
        }
        return QTest::qWaitFor([&] { return app.view->cacheReady(); }, 20000);
    }

    void tablet(App& app, QEvent::Type type, QPointF page, Qt::MouseButton button, Qt::MouseButtons buttons)
    {
        const QPointF scene = app.view->pageToScene(page);
        QTabletEvent event(type, m_stylus.get(), scene, app.window->mapToGlobal(scene),
                           type == QEvent::TabletRelease ? 0.0 : 0.7, 0, 0, 0, 0, 0, Qt::NoModifier, button,
                           buttons);
        QCoreApplication::sendEvent(app.window, &event);
    }

    // Trait du stylet de `from` à `to` ; sans relâché si `release` est faux.
    void stylus(App& app, QPointF from, QPointF to, int steps = 20, bool release = true)
    {
        tablet(app, QEvent::TabletPress, from, Qt::LeftButton, Qt::LeftButton);
        for (int i = 1; i <= steps; ++i) {
            tablet(app, QEvent::TabletMove, from + (to - from) * i / steps, Qt::NoButton, Qt::LeftButton);
        }
        if (release) {
            tablet(app, QEvent::TabletRelease, to, Qt::LeftButton, Qt::NoButton);
        }
    }

    // Glissement à la souris de `from` à `to` en `steps` mouvements ; sans relâché si `release` est faux.
    void mouseDrag(App& app, QPointF from, QPointF to, int steps, bool release = true)
    {
        QTest::mousePress(app.window, Qt::LeftButton, Qt::NoModifier, app.scene(from));
        for (int i = 1; i <= steps; ++i) {
            QTest::mouseMove(app.window, app.scene(from + (to - from) * i / steps));
        }
        if (release) {
            QTest::mouseRelease(app.window, Qt::LeftButton, Qt::NoModifier, app.scene(to));
        }
    }

    void click(App& app, QPointF page, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
    {
        QTest::mouseClick(app.window, Qt::LeftButton, modifiers, app.scene(page));
    }

    // Deux traits au stylo, (300, 300) → (500, 300) puis (300, 450) → (500, 450).
    bool twoStrokes(App& app)
    {
        stylus(app, QPointF(300, 300), QPointF(500, 300));
        stylus(app, QPointF(300, 450), QPointF(500, 450));
        return app.document->shownObjectCount() == 2;
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_settingsDir.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
        const QSettings probe;
        QVERIFY2(QDir::fromNativeSeparators(probe.fileName()).startsWith(m_settingsDir.path()),
                 qPrintable(probe.fileName()));
        m_stylus = std::make_unique<QPointingDevice>(
            QStringLiteral("tst-ui-tools-stylus"), 5180, QInputDevice::DeviceType::Stylus,
            QPointingDevice::PointerType::Pen,
            QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
        m_touch = QTest::createTouchDevice();
    }

    void oneUserForNow()
    {
        actcraft::ui::ToolHost host;
        actcraft::tools::UserState& a = host.user();
        actcraft::tools::UserState& b = host.user(actcraft::input::kFirstUser);
        QCOMPARE(&a, &b);
        QVERIFY(host.activeTool() == nullptr); // rien de branché : ni contrôleur, ni vue, ni couche
    }

    void liftLayerTypeIsRegistered()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport ActCraft.Ui\nLiftLayer {}\n", QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
    }

    // H-S2 : une propriété requise accepte null comme valeur initiale (talons QML à required property).
    void requiredPropertyAcceptsNull()
    {
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nItem { required property Item t }\n", QUrl());
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> object(component.createWithInitialProperties(
            {{QStringLiteral("t"), QVariant::fromValue<QObject*>(nullptr)}}));
        QVERIFY2(object, qPrintable(component.errorString()));
        QVERIFY(component.errors().isEmpty());
    }

    void activatingAToolChecksItsCommand()
    {
        App app;
        QVERIFY(start(app));
        QCOMPARE(app.tools->currentTool(), QStringLiteral("pen"));
        QVERIFY(app.trigger("tool.eraser"));
        QCOMPARE(app.tools->currentTool(), QStringLiteral("eraser"));
        QVERIFY(app.commands->isChecked(QStringLiteral("tool.eraser")));
        QVERIFY(!app.commands->isChecked(QStringLiteral("tool.pen")));
        QVERIFY(app.tools->activeTool());
        QCOMPARE(app.tools->activeTool()->id(), QStringLiteral("eraser"));
    }

    void colorGoesToActiveTool()
    {
        App app;
        QVERIFY(start(app));
        const QColor red(QStringLiteral("#E53935"));
        QVERIFY(app.trigger("pen.color", QStringLiteral("#E53935")));
        QCOMPARE(app.tools->color(), red);
        QCOMPARE(app.view->inkColor(), red);
        auto* pen = dynamic_cast<tools::PenTool*>(app.tools->activeTool());
        QVERIFY(pen);
        QCOMPARE(pen->color(), static_cast<core::Argb>(red.rgba()));

        QVERIFY(app.trigger("tool.highlighter"));
        QCOMPARE(app.tools->color(), QColor(QStringLiteral("#FFFF00"))); // D5
        QCOMPARE(app.view->inkColor().rgb(), QColor(QStringLiteral("#FFFF00")).rgb());
        QCOMPARE(app.view->inkColor().alpha(), 255 - 127); // alpha du marqueur

        QVERIFY(app.trigger("tool.pen"));
        QCOMPARE(app.tools->color(), red);
        QCOMPARE(app.view->inkColor(), red);
    }

    // L3-T0 : une couleur choisie sous Sélectionner ou sous la gomme va au dernier outil d'encre et le
    // réactive.
    void colorUnderSelectOrEraserGoesToLastInkTool()
    {
        App app;
        QVERIFY(start(app));
        QVERIFY(app.trigger("tool.select"));
        QVERIFY(app.trigger("pen.color", QStringLiteral("#1E88E5")));
        QCOMPARE(app.tools->currentTool(), QStringLiteral("pen"));
        QCOMPARE(app.tools->color(), QColor(QStringLiteral("#1E88E5")));
        QVERIFY(app.commands->isChecked(QStringLiteral("tool.pen")));

        QVERIFY(app.trigger("tool.highlighter"));
        QVERIFY(app.trigger("tool.eraser"));
        QVERIFY(app.trigger("pen.color", QStringLiteral("#43A047")));
        QCOMPARE(app.tools->currentTool(), QStringLiteral("highlighter"));
        QCOMPARE(app.tools->color(), QColor(QStringLiteral("#43A047")));
        QVERIFY(app.commands->isChecked(QStringLiteral("tool.highlighter")));
        QVERIFY(!app.commands->isChecked(QStringLiteral("tool.eraser")));
        QVERIFY(app.trigger("tool.pen"));
        QCOMPARE(app.tools->color(), QColor(QStringLiteral("#1E88E5"))); // le stylo garde la sienne
    }

    void settingsSurviveRestart()
    {
        const QString file = m_settingsDir.filePath(QStringLiteral("redemarrage.ini"));
        {
            App app;
            QVERIFY(start(app, file));
            QVERIFY(app.trigger("pen.color", QStringLiteral("#E53935")));
            QVERIFY(app.trigger("pen.width", 8));
            QCOMPARE(app.tools->width(), 8.0);
        }
        const QSettings probe(file, QSettings::IniFormat);
        QVERIFY(QDir::fromNativeSeparators(probe.fileName()).startsWith(m_settingsDir.path()));
        QCOMPARE(probe.value(QStringLiteral("outils/pen/epaisseur")).toDouble(), 8.0);
        App app;
        QVERIFY(start(app, file));
        QCOMPARE(app.tools->currentTool(), QStringLiteral("pen"));
        QCOMPARE(app.tools->color(), QColor(QStringLiteral("#E53935")));
        QCOMPARE(app.tools->width(), 8.0);
        QCOMPARE(app.view->inkColor(), QColor(QStringLiteral("#E53935")));
        QCOMPARE(app.view->inkWidth(), 8.0);
        auto* pen = dynamic_cast<tools::PenTool*>(app.tools->activeTool());
        QVERIFY(pen);
        QCOMPARE(pen->color(), static_cast<core::Argb>(QColor(QStringLiteral("#E53935")).rgba()));
        QCOMPARE(pen->baseWidth(), 8.0);
    }

    void eraseGoesThroughTheCacheInterface()
    {
        App app;
        QVERIFY(start(app));
        QVERIFY(twoStrokes(app));
        const core::ObjectId first = app.idAt(0);
        QVERIFY(app.trigger("tool.eraser"));
        observe(app);
        stylus(app, QPointF(400, 240), QPointF(400, 360), 12, false);
        QCOMPARE(app.journal, QStringList{entry("r", first, "Lift")});
        QVERIFY(app.lift->objectCount() >= 1);
        tablet(app, QEvent::TabletRelease, QPointF(400, 360), Qt::LeftButton, Qt::NoButton);
        QCOMPARE(app.journal.size(), 3);
        for (qsizetype i = 1; i < app.journal.size(); ++i) {
            QVERIFY2(app.journal.at(i).startsWith(QStringLiteral("i:")) &&
                         app.journal.at(i).endsWith(QStringLiteral(":Drop")),
                     qPrintable(app.journal.join(QStringLiteral(" "))));
        }
        QCOMPARE(app.document->shownObjectCount(), 3);
        QCOMPARE(app.lift->objectCount(), 0);
    }

    void dragGoesThroughTheCacheInterface()
    {
        App app;
        QVERIFY(start(app));
        stylus(app, QPointF(300, 300), QPointF(500, 300));
        const core::ObjectId id = app.idAt(0);
        QVERIFY(app.trigger("tool.select"));
        observe(app);
        mouseDrag(app, QPointF(400, 300), QPointF(480, 300), 8);
        QCOMPARE(app.journal, (QStringList{entry("r", id, "Lift"), entry("i", id, "Drop")}));
        QCOMPARE(app.lift->objectCount(), 0);
    }

    void undoAfterEraseRestores()
    {
        App app;
        QVERIFY(start(app));
        QVERIFY(twoStrokes(app));
        const QString before = app.document->fingerprintText();
        QVERIFY(app.trigger("tool.eraser"));
        stylus(app, QPointF(400, 240), QPointF(400, 360), 12);
        QCOMPARE(app.document->shownObjectCount(), 3);
        QVERIFY(app.trigger("edit.undo"));
        QCOMPARE(app.document->shownObjectCount(), 2);
        QCOMPARE(app.document->fingerprintText(), before);
    }

    // L3-T0 : Annuler et Rétablir vident la sélection, même quand l'objet sélectionné reste sur la page.
    void undoAndRedoClearTheSelection()
    {
        App app;
        QVERIFY(start(app));
        QVERIFY(twoStrokes(app));
        QVERIFY(app.trigger("tool.select"));
        click(app, QPointF(400, 300));
        QCOMPARE(app.tools->selectionCount(), 1);
        QVERIFY(app.trigger("edit.undo")); // retire le second trait, pas le sélectionné
        QCOMPARE(app.document->shownObjectCount(), 1);
        QCOMPARE(app.tools->selectionCount(), 0);
        QVERIFY(!app.tools->selectionFrame().value(QStringLiteral("visible")).toBool());
        click(app, QPointF(400, 300));
        QCOMPARE(app.tools->selectionCount(), 1);
        QVERIFY(app.trigger("edit.redo"));
        QCOMPARE(app.document->shownObjectCount(), 2);
        QCOMPARE(app.tools->selectionCount(), 0);
    }

    // Le marqueur garde sa translucidité jusqu'au document (ControllerSink::commitAnnotation).
    void highlighterKeepsItsTranslucency()
    {
        App app;
        QVERIFY(start(app));
        QVERIFY(app.trigger("tool.highlighter"));
        stylus(app, QPointF(300, 300), QPointF(500, 300));
        QCOMPARE(app.document->shownObjectCount(), 1);
        const core::PageObject& object = app.page().objects.back();
        QVERIFY(std::get_if<core::Stroke>(&object));
        QCOMPARE(int(object.common.translucency), 127);
        QVERIFY(app.trigger("tool.pen"));
        stylus(app, QPointF(300, 450), QPointF(500, 450));
        QCOMPARE(int(app.page().objects.back().common.translucency), 0);
    }

    // L3-T8 : Maj et Ctrl arrivent à l'outil actif ; avant chaque appui, l'état des touches est remis à celui
    // de l'événement (une touche restée collée après une perte de focus est corrigée).
    void shiftAndControlReachTheActiveTool()
    {
        App app;
        QVERIFY(start(app));
        QVERIFY(twoStrokes(app));
        QVERIFY(app.trigger("tool.select"));
        click(app, QPointF(400, 300));
        QCOMPARE(app.tools->selectionCount(), 1);
        QTest::keyPress(app.window, Qt::Key_Shift, Qt::ShiftModifier);
        click(app, QPointF(400, 450), Qt::ShiftModifier);
        QCOMPARE(app.tools->selectionCount(), 2);
        // Maj jamais relâchée pour l'outil : un clic sans modificateur dans le vide désélectionne quand même.
        click(app, QPointF(700, 600));
        QCOMPARE(app.tools->selectionCount(), 0);
        QTest::keyRelease(app.window, Qt::Key_Shift, Qt::NoModifier);
        click(app, QPointF(400, 300));
        QCOMPARE(app.tools->selectionCount(), 1);
        click(app, QPointF(400, 450), Qt::ControlModifier);
        QCOMPARE(app.tools->selectionCount(), 2);
    }

    // L3-T12 : execute() ne dépose pas d'abord ce qui est soulevé. Une commande de la sélection lancée
    // pendant un glissement (Suppr) fait d'abord abandonner son geste à l'outil.
    void commandDuringDragAbandonsTheGestureFirst()
    {
        App app;
        QVERIFY(start(app));
        stylus(app, QPointF(300, 300), QPointF(500, 300));
        QVERIFY(app.trigger("tool.select"));
        click(app, QPointF(400, 300));
        QCOMPARE(app.tools->selectionCount(), 1);
        const QString before = app.document->fingerprintText();
        mouseDrag(app, QPointF(400, 300), QPointF(440, 300), 4, false);
        QCOMPARE(app.lift->objectCount(), 1);
        QVERIFY(app.trigger("edit.delete"));
        QCOMPARE(app.lift->objectCount(), 0);
        QVERIFY(!app.document->isLifting());
        QCOMPARE(app.document->shownObjectCount(), 0);
        QTest::mouseRelease(app.window, Qt::LeftButton, Qt::NoModifier, app.scene(QPointF(440, 300)));
        QCOMPARE(app.document->shownObjectCount(), 0);
        QVERIFY(app.trigger("edit.undo")); // une seule étape : la suppression, l'objet à sa place d'origine
        QCOMPARE(app.document->fingerprintText(), before);
    }

    // B1 : le soulèvement n'annule pas l'outil qui soulève.
    void liftDoesNotCancelTheLiftingTool()
    {
        App app;
        QVERIFY(start(app));
        QVERIFY(twoStrokes(app));
        const QString before = app.document->fingerprintText();
        QVERIFY(app.trigger("tool.select"));
        mouseDrag(app, QPointF(400, 300), QPointF(480, 300), 8, false);
        QCOMPARE(app.lift->objectCount(), 1);
        QVERIFY(app.busy());
        QVERIFY(app.document->isLifting());
        QTest::mouseRelease(app.window, Qt::LeftButton, Qt::NoModifier, app.scene(QPointF(480, 300)));
        QCOMPARE(app.document->shownObjectCount(), 2); // déplacé, pas perdu
        QVERIFY(app.document->fingerprintText() != before);
        QCOMPARE(app.lift->objectCount(), 0);

        QVERIFY(app.trigger("tool.eraser"));
        stylus(app, QPointF(400, 390), QPointF(400, 510), 12, false);
        QVERIFY(app.busy());
        QVERIFY(app.document->isLifting());
        tablet(app, QEvent::TabletRelease, QPointF(400, 510), Qt::LeftButton, Qt::NoButton);
        QCOMPARE(app.document->shownObjectCount(), 3); // les morceaux sont dans la page
        QCOMPARE(app.lift->objectCount(), 0);
    }

    // L3-C8 (m1) : l'encre de la vue remplacée pendant un glissement, puis pendant un coup de gomme. Les
    // outils sont reconstruits ; celui qui avait un geste l'abandonne d'abord (dépôt sans étape, couche
    // soulevée vidée). Un cancelGesture() dans le relais de inkChanged ne suffirait pas : il passe par
    // adapters(), qui jette sans les annuler les outils d'une encre déjà remplacée.
    void inkChangeWhileLiftingDropsTheLift()
    {
        App app;
        QVERIFY(start(app));
        QVERIFY(twoStrokes(app));
        const QString before = app.document->fingerprintText();
        render::InkItem* first = app.view->ink();
        QVERIFY(first);
        auto* second = new render::InkItem(first->parentItem());

        QVERIFY(app.trigger("tool.select"));
        mouseDrag(app, QPointF(400, 300), QPointF(440, 300), 4, false);
        QCOMPARE(app.lift->objectCount(), 1);
        QVERIFY(app.document->isLifting());
        app.view->setInk(second);
        QCOMPARE(app.lift->objectCount(), 0);
        QVERIFY(!app.document->isLifting());
        QVERIFY(!app.busy());
        QCOMPARE(app.document->fingerprintText(), before);
        QTest::mouseRelease(app.window, Qt::LeftButton, Qt::NoModifier, app.scene(QPointF(440, 300)));
        QCOMPARE(app.document->fingerprintText(), before); // rien déplacé

        QVERIFY(app.trigger("tool.eraser"));
        stylus(app, QPointF(400, 390), QPointF(400, 510), 12, false);
        QVERIFY(app.document->isLifting());
        QVERIFY(app.lift->objectCount() >= 1);
        app.view->setInk(first);
        QCOMPARE(app.lift->objectCount(), 0);
        QVERIFY(!app.document->isLifting());
        QVERIFY(!app.busy());
        QCOMPARE(app.document->fingerprintText(), before); // ce qui était gommé revient
        tablet(app, QEvent::TabletRelease, QPointF(400, 510), Qt::LeftButton, Qt::NoButton);
        QCOMPARE(app.document->fingerprintText(), before);
        QCOMPARE(app.document->shownObjectCount(), 2);
    }

    // M2 : page suivante pendant un glissement.
    void pageChangeWhileDraggingClearsTheLiftLayer()
    {
        App app;
        QVERIFY(start(app));
        stylus(app, QPointF(300, 300), QPointF(500, 300));
        const QString before = app.document->fingerprintText();
        QVERIFY(app.trigger("tool.select"));
        mouseDrag(app, QPointF(400, 300), QPointF(440, 300), 4, false);
        QCOMPARE(app.lift->objectCount(), 1);
        QVERIFY(app.trigger("page.next"));
        QCOMPARE(app.lift->objectCount(), 0);
        QVERIFY(!app.document->isLifting());
        QVERIFY(!app.busy());
        QCOMPARE(app.document->currentIndex(), 1);
        QCOMPARE(app.document->document().page(0).objects.size(), std::size_t{1});
        observe(app);
        QTest::mouseRelease(app.window, Qt::LeftButton, Qt::NoModifier, app.scene(QPointF(440, 300)));
        QVERIFY2(app.journal.isEmpty(), qPrintable(app.journal.join(QStringLiteral(" ")))); // rien déposé
        QVERIFY(app.trigger("edit.undo")); // la page ajoutée ; aucune étape de déplacement avant elle
        QCOMPARE(app.document->fingerprintText(), before);
    }

    // M2 : Annuler pendant un glissement. Le geste est abandonné sans étape (le relâché synthétisé par
    // finishStroke ne doit pas le terminer par un déplacement), puis Annuler défait l'étape d'avant : le
    // trait.
    void undoWhileDraggingDropsWithoutAStep()
    {
        App app;
        QVERIFY(start(app));
        stylus(app, QPointF(300, 300), QPointF(500, 300));
        QVERIFY(app.trigger("tool.select"));
        mouseDrag(app, QPointF(400, 300), QPointF(440, 300), 4, false);
        QCOMPARE(app.lift->objectCount(), 1);
        QVERIFY(app.trigger("edit.undo"));
        QCOMPARE(app.lift->objectCount(), 0);
        QVERIFY(!app.document->isLifting());
        QCOMPARE(app.document->shownObjectCount(), 0);
        QTest::mouseRelease(app.window, Qt::LeftButton, Qt::NoModifier, app.scene(QPointF(440, 300)));
        QCOMPARE(app.document->shownObjectCount(), 0);
    }

    // M9 : Échap pendant un glissement = abandon ; au repos, outil Sélectionner. Jamais livré à onKey.
    void escapeCancelsTheGesture()
    {
        App app;
        QVERIFY(start(app));
        stylus(app, QPointF(300, 300), QPointF(500, 300));
        const QString before = app.document->fingerprintText();
        QVERIFY(app.trigger("tool.select"));
        mouseDrag(app, QPointF(400, 300), QPointF(440, 300), 4, false);
        QCOMPARE(app.lift->objectCount(), 1);
        QTest::keyClick(app.window, Qt::Key_Escape);
        QCOMPARE(app.lift->objectCount(), 0);
        QVERIFY(!app.busy());
        QCOMPARE(app.document->fingerprintText(), before);
        QCOMPARE(app.tools->currentTool(), QStringLiteral("select"));
        QTest::mouseRelease(app.window, Qt::LeftButton, Qt::NoModifier, app.scene(QPointF(440, 300)));
        QCOMPARE(app.document->fingerprintText(), before);

        QVERIFY(app.trigger("tool.pen"));
        QTest::keyClick(app.window, Qt::Key_Escape);
        QCOMPARE(app.tools->currentTool(), QStringLiteral("select"));
        QVERIFY(app.commands->isChecked(QStringLiteral("tool.select")));
    }

    // D-S9 : un outil occupé que le relâché de finishStroke ne termine pas (le glissement d'un doigt : le
    // relâché synthétisé ne porte pas son contact) reçoit cancel().
    void finishStrokeCancelsTheActiveTool()
    {
        App app;
        QVERIFY(start(app));
        stylus(app, QPointF(300, 300), QPointF(500, 300));
        const QString before = app.document->fingerprintText();
        QVERIFY(app.trigger("tool.select"));
        QTest::touchEvent(app.window, m_touch).press(0, app.scene(QPointF(400, 300)));
        for (int i = 1; i <= 4; ++i) {
            QTest::touchEvent(app.window, m_touch).move(0, app.scene(QPointF(400 + 10 * i, 300)));
        }
        // Qt Quick retient les déplacements d'un doigt et ne les livre qu'à la synchronisation de l'image
        // suivante (compression du toucher, QML_NO_TOUCH_COMPRESSION) : le glissement commence alors, d'où
        // l'attente bornée sur la condition.
        qInfo("souleve avant l'image suivante : %d", app.lift->objectCount());
        QTRY_COMPARE_WITH_TIMEOUT(app.lift->objectCount(), 1, 5000);
        QVERIFY(app.busy());
        app.view->finishStroke();
        QVERIFY(!app.busy());
        QCOMPARE(app.lift->objectCount(), 0);
        QVERIFY(!app.document->isLifting());
        QCOMPARE(app.document->fingerprintText(), before);
        QTest::touchEvent(app.window, m_touch).release(0, app.scene(QPointF(440, 300)));
        QCOMPARE(app.document->fingerprintText(), before);
    }
};

QTEST_MAIN(TstUiTools)
#include "tst_ui_tools.moc"
