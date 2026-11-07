// L11-T9 : mode à deux utilisateurs (F-110, F-303, N2-A7) — attribut opensDialog, commandes à boîte refusées
// au second utilisateur par Commands::trigger (et par lui seul), bascule users.dual, deuxième palette.
#include "startup.h"

#include <actcraft/actions/commandregistry.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/lift.h>
#include <actcraft/core/model.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/input/touchmodes.h>
#include <actcraft/input/twouserpolicy.h>
#include <actcraft/tools/tool.h>
#include <actcraft/tools/userstate.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/dualmode.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/toolhost.h>
#include <actcraft/ui/touchmode.h>

#include <QJSEngine>
#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTabletEvent>
#include <QTemporaryDir>
#include <QtTest>

#include <cmath>
#include <cstdint>
#include <memory>

using namespace actcraft;

namespace {

// Gestionnaires JavaScript qui comptent leurs appels et retiennent (argument, user), comme ceux de Main.qml.
// Commands appelle ses gestionnaires par son moteur (qjsEngine) : il est associé à celui-ci.
struct HandlerProbe
{
    QJSEngine engine;

    explicit HandlerProbe(ui::Commands& commands)
    {
        QJSEngine::setObjectOwnership(&commands, QJSEngine::CppOwnership);
        engine.newQObject(&commands);
        engine.evaluate(QStringLiteral("var calls = {}; var users = {}; var args = {};"));
    }
    QJSValue handler(const QString& name)
    {
        return engine.evaluate(
            QStringLiteral("(function (argument, user) { calls['%1'] = (calls['%1'] || 0) + "
                           "1; args['%1'] = argument; users['%1'] = user; })")
                .arg(name));
    }
    int calls(const QString& name)
    {
        return engine.evaluate(QStringLiteral("calls['%1'] || 0").arg(name)).toInt();
    }
    QJSValue lastUser(const QString& name)
    {
        return engine.evaluate(QStringLiteral("users['%1']").arg(name));
    }
    QJSValue lastArgument(const QString& name)
    {
        return engine.evaluate(QStringLiteral("args['%1']").arg(name));
    }
};

// Main.qml chargé (réglages d'essai d'initTestCase).
struct App
{
    QTemporaryDir profiles;
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
    ui::DualMode* dual = nullptr;
    ui::Commands* commands = nullptr;
    ui::ToolHost* tools = nullptr;
};

bool start(App& app)
{
    StartupOptions options;
    options.cacheEnabled = false;
    options.generatedPage = false;
    options.profileRoot = app.profiles.path();
    app.window = loadMainWindow(app.engine, options);
    if (!app.window || !QTest::qWaitForWindowExposed(app.window)) {
        return false;
    }
    app.view = app.window->findChild<ui::PageView*>(QStringLiteral("pageView"));
    app.dual = app.window->findChild<ui::DualMode*>(QStringLiteral("appDualMode"));
    app.commands = app.dual ? app.dual->commands() : nullptr;
    app.tools = app.dual ? app.dual->tools() : nullptr;
    return app.view && app.dual && app.commands && app.tools;
}

std::unique_ptr<QPointingDevice> makeStylus(qint64 systemId, qint64 uniqueId)
{
    return std::make_unique<QPointingDevice>(
        QStringLiteral("tst-secondpalette-stylus-%1").arg(uniqueId), systemId,
        QInputDevice::DeviceType::Stylus, QPointingDevice::PointerType::Pen,
        QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3, QString(),
        QPointingDeviceUniqueId::fromNumericId(uniqueId));
}

void tablet(App& app, const QPointingDevice* device, QEvent::Type type, QPointF page)
{
    const bool press = type == QEvent::TabletPress;
    const bool release = type == QEvent::TabletRelease;
    const QPointF scene = app.view->pageToScene(page);
    QTabletEvent event(type, device, scene, app.window->mapToGlobal(scene), release ? 0.0 : 0.7, 0, 0, 0, 0,
                       0, Qt::NoModifier, (press || release) ? Qt::LeftButton : Qt::NoButton,
                       release ? Qt::NoButton : Qt::LeftButton);
    QCoreApplication::sendEvent(app.window, &event);
}

// Les délégués d'un Repeater ne sont pas enfants QObject de la palette : recherche dans l'arbre visuel.
QQuickItem* findItem(QQuickItem* root, const QString& name)
{
    if (!root) {
        return nullptr;
    }
    if (root->objectName() == name) {
        return root;
    }
    for (QQuickItem* child : root->childItems()) {
        if (QQuickItem* found = findItem(child, name)) {
            return found;
        }
    }
    return nullptr;
}

// Décision de la politique de la vue pour un deuxième doigt posé pendant qu'un premier touche l'outil.
input::Route secondFingerRoute(ui::PageView& view)
{
    input::Sample press;
    press.device = input::DeviceKind::Touch;
    press.phase = input::Phase::Press;
    press.x = 700;
    press.y = 400;
    press.pointer = 2;
    input::RouterState state;
    state.toolTouches = 1;
    return view.router().policy()->decide(press, state).route;
}

QColor penColor(ui::ToolHost& tools, input::UserId user)
{
    return QColor::fromRgba(tools.user(user).settings(QStringLiteral("pen")).color);
}

// Glisser d'un stylet, de `from` à `to` en six mouvements, relâché à l'arrivée (coordonnées de page).
void stylusDrag(App& app, const QPointingDevice* device, QPointF from, QPointF to)
{
    tablet(app, device, QEvent::TabletPress, from);
    for (int i = 1; i <= 6; ++i) {
        tablet(app, device, QEvent::TabletMove, from + (to - from) * i / 6);
    }
    tablet(app, device, QEvent::TabletRelease, to);
}

// Rectangle ajouté à la page affichée par la pile (une étape) ; `locked` : drapeau du verrou.
core::ObjectId addBox(ui::DocumentController& controller, core::RectF bounds, bool locked = false)
{
    core::PageObject object = core::makeShapeObject("rectangle", bounds, core::ShapeStyle{});
    object.common.locked = locked;
    const auto page = static_cast<std::size_t>(controller.currentIndex());
    controller.execute(std::make_unique<core::AddObjectCommand>(page, std::move(object)));
    return controller.document().page(page).objects.back().common.id;
}

const core::PageObject* objectOf(const ui::DocumentController& controller, core::ObjectId id)
{
    const core::Page& page = controller.document().page(static_cast<std::size_t>(controller.currentIndex()));
    const std::size_t index = core::indexOf(page, id);
    return index == page.objects.size() ? nullptr : &page.objects[index];
}

} // namespace

class TstUiSecondpalette : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void opensDialogParsed();
    void everyDialogCommandMarked();
    void secondUserCannotOpenADialog();
    void firstUserStillCan();
    void toggleSetsTheTwoUserPolicy();
    void offRestoresTheTouchMode();
    void pinchZoomsOnlyWhenOff();
    void secondUserReceivesInDualMode();
    // L11-C6 (revue de L11-T9).
    void undoCancelsTheSecondUsersGesture();
    void touchModeChangeKeepsTheUsers();
    void designFlagsReachTheSecondUser();
    void revealInkKeepsTheUser();
    void nestedTriggerKeepsTheUser();
    void toggleWaitsForTheContactToEnd();

private:
    QTemporaryDir m_settings;
    QTemporaryDir m_profile;
};

void TstUiSecondpalette::initTestCase()
{
    QVERIFY(m_settings.isValid());
    QVERIFY(m_profile.isValid());
    QStandardPaths::setTestModeEnabled(true);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
    QSettings probe;
    QCOMPARE(probe.format(), QSettings::IniFormat);
    QVERIFY(probe.fileName().startsWith(m_settings.path()));
}

void TstUiSecondpalette::opensDialogParsed()
{
    ui::Commands commands;
    commands.setProfileDir(m_profile.path());
    QVERIFY2(commands.errors().isEmpty(), qPrintable(commands.errors().join(u'\n')));
    const actions::CommandInfo* open = commands.registry().find(QStringLiteral("file.open"));
    QVERIFY(open);
    QVERIFY(open->opensDialog);
    const actions::CommandInfo* color = commands.registry().find(QStringLiteral("pen.color"));
    QVERIFY(color);
    QVERIFY(!color->opensDialog); // clé absente : faux
}

void TstUiSecondpalette::everyDialogCommandMarked()
{
    // Liste relevée dans les gestionnaires QML (corps des setHandler et fonctions qu'ils appellent) : Dialog,
    // Popup modale, sélecteur de fichier (argument d'URL facultatif : boîte sans argument). Non retenus :
    // menus et surcouches sans boîte (resources.add, L11-C6 : ajout direct à la bibliothèque ; object.menu,
    // object.translucency, color.more, color.pick, view.dashboard, help.contents). Exacte dans les deux sens
    // : une commande marquée hors de la liste rougit aussi.
    const QStringList dialogs = {
        QStringLiteral("classroom.lock"),     QStringLiteral("classroom.password"),
        QStringLiteral("color.dialog"),       QStringLiteral("file.new-custom"),
        QStringLiteral("file.open"),          QStringLiteral("file.save-as"),
        QStringLiteral("file.quit"),          QStringLiteral("file.print"),
        QStringLiteral("file.publish"),       QStringLiteral("help.about"),
        QStringLiteral("page.background"),    QStringLiteral("grid.designer"),
        QStringLiteral("profile.choose"),     QStringLiteral("profile.edit"),
        QStringLiteral("profile.save-as"),    QStringLiteral("profile.import"),
        QStringLiteral("profile.export"),     QStringLiteral("resources.other"),
        QStringLiteral("resources.delete"),   QStringLiteral("resources.new-folder"),
        QStringLiteral("resources.rename"),   QStringLiteral("spell.check"),
        QStringLiteral("insert.web-frame"),   QStringLiteral("webframe.unblock"),
        QStringLiteral("link.file"),          QStringLiteral("link.website"),
        QStringLiteral("insert.image"),       QStringLiteral("insert.media"),
        QStringLiteral("export.page-image"),  QStringLiteral("export.pdf"),
        QStringLiteral("file.import-pdf"),    QStringLiteral("file.import-powerpoint-images"),
        QStringLiteral("help.check-updates"), // boîte en L11-T22
    };
    ui::Commands commands;
    commands.setProfileDir(m_profile.path());
    for (const QString& id : dialogs) {
        const actions::CommandInfo* info = commands.registry().find(id);
        QVERIFY2(info, qPrintable(id));
        QVERIFY2(info->opensDialog, qPrintable(id));
    }
    QStringList extra;
    for (const QString& id : commands.registry().ids()) {
        if (commands.registry().find(id)->opensDialog && !dialogs.contains(id)) {
            extra << id;
        }
    }
    QVERIFY2(extra.isEmpty(), qPrintable(extra.join(u' ')));
}

void TstUiSecondpalette::secondUserCannotOpenADialog()
{
    ui::Commands commands;
    commands.setProfileDir(m_profile.path());
    HandlerProbe probe(commands);
    commands.setHandler(QStringLiteral("file.open"), probe.handler(QStringLiteral("open")));
    QSignalSpy refused(&commands, &ui::Commands::refusedForSecondUser);
    QSignalSpy triggered(&commands, &ui::Commands::triggered);

    // Le refus est dans trigger : le gestionnaire n'est jamais appelé, quelle que soit la voie.
    QVERIFY(!commands.trigger(QStringLiteral("file.open"), QVariant(), 1));
    QCOMPARE(probe.calls(QStringLiteral("open")), 0);
    QCOMPARE(refused.count(), 1);
    QCOMPARE(refused.at(0).at(0).toString(), QStringLiteral("file.open"));
    QCOMPARE(triggered.count(), 0);

    // Une commande sans boîte passe, et l'utilisateur atteint le gestionnaire (argument, user).
    commands.setHandler(QStringLiteral("pen.color"), probe.handler(QStringLiteral("color")));
    QVERIFY(commands.trigger(QStringLiteral("pen.color"), QStringLiteral("#1E88E5"), 1));
    QCOMPARE(probe.calls(QStringLiteral("color")), 1);
    QCOMPARE(probe.lastArgument(QStringLiteral("color")).toString(), QStringLiteral("#1E88E5"));
    QCOMPARE(probe.lastUser(QStringLiteral("color")).toInt(), 1);
    QCOMPARE(refused.count(), 1);
}

void TstUiSecondpalette::firstUserStillCan()
{
    ui::Commands commands;
    commands.setProfileDir(m_profile.path());
    HandlerProbe probe(commands);
    commands.setHandler(QStringLiteral("file.open"), probe.handler(QStringLiteral("open")));
    QSignalSpy refused(&commands, &ui::Commands::refusedForSecondUser);

    QVERIFY(commands.trigger(QStringLiteral("file.open"), QVariant(), 0));
    QCOMPARE(probe.calls(QStringLiteral("open")), 1);
    QCOMPARE(probe.lastUser(QStringLiteral("open")).toInt(), 0);
    // L'ancienne signature vaut user = 0.
    QVERIFY(commands.trigger(QStringLiteral("file.open")));
    QCOMPARE(probe.calls(QStringLiteral("open")), 2);
    QVERIFY(probe.lastUser(QStringLiteral("open")).isNumber());
    QCOMPARE(probe.lastUser(QStringLiteral("open")).toInt(), 0);
    QCOMPARE(refused.count(), 0);
}

void TstUiSecondpalette::toggleSetsTheTwoUserPolicy()
{
    ui::Commands commands;
    commands.setProfileDir(m_profile.path());
    ui::PageView view;
    ui::ToolHost tools;
    ui::DualMode dual;
    dual.setCommands(&commands);
    dual.setView(&view);
    dual.setTools(&tools);
    QSignalSpy changed(&dual, &ui::DualMode::activeChanged);

    QVERIFY(commands.trigger(QStringLiteral("users.dual")));
    QVERIFY(dual.active());
    QCOMPARE(changed.count(), 1);
    QVERIFY(tools.dualUsers());
    QVERIFY(commands.isChecked(QStringLiteral("users.dual")));
    QVERIFY(dynamic_cast<input::TwoUserPolicy*>(view.router().policy()));

    QVERIFY(commands.trigger(QStringLiteral("users.dual")));
    QVERIFY(!dual.active());
    QCOMPARE(changed.count(), 2);
    QVERIFY(!tools.dualUsers());
    QVERIFY(!commands.isChecked(QStringLiteral("users.dual")));
    QVERIFY(dynamic_cast<input::FullTouchPolicy*>(view.router().policy())); // sans contrôleur du toucher
}

void TstUiSecondpalette::offRestoresTheTouchMode()
{
    ui::Commands commands;
    commands.setProfileDir(m_profile.path());
    ui::Preferences prefs(m_settings.filePath(QStringLiteral("touch.ini")), nullptr);
    ui::PageView view;
    ui::TouchModeController touch;
    touch.setCommands(&commands);
    touch.setPreferences(&prefs);
    touch.setView(&view);
    QVERIFY(touch.setMode(QStringLiteral("pen")));
    QVERIFY(dynamic_cast<input::PenTouchPolicy*>(view.router().policy()));
    ui::DualMode dual;
    dual.setCommands(&commands);
    dual.setView(&view);
    dual.setTouchMode(&touch);

    QVERIFY(commands.trigger(QStringLiteral("users.dual")));
    QVERIFY(dynamic_cast<input::TwoUserPolicy*>(view.router().policy()));
    QVERIFY(commands.trigger(QStringLiteral("users.dual")));
    QVERIFY(dynamic_cast<input::PenTouchPolicy*>(view.router().policy()));

    // L11-T6 : un changement du mode du toucher pendant le mode à deux ne remplace pas sa politique ; il vaut
    // à l'extinction.
    QVERIFY(commands.trigger(QStringLiteral("users.dual")));
    QVERIFY(touch.setMode(QStringLiteral("select")));
    QVERIFY(dynamic_cast<input::TwoUserPolicy*>(view.router().policy()));
    QVERIFY(commands.trigger(QStringLiteral("users.dual")));
    QVERIFY(dynamic_cast<input::SelectTouchPolicy*>(view.router().policy()));
}

void TstUiSecondpalette::pinchZoomsOnlyWhenOff()
{
    // Le pincement réel (zoom inchangé allumé, changé éteint) est joué par le scénario S28 : son injection
    // passe par l'interface système privée de Qt, interdite aux tests (N4). Ici, sa cause : un deuxième doigt
    // fait un geste en gestion complète, mais va à l'outil d'un utilisateur en mode à deux, et le geste
    // revient éteint.
    App app;
    QVERIFY(start(app));
    auto* touch = app.window->findChild<ui::TouchModeController*>(QStringLiteral("appTouchMode"));
    QVERIFY(touch);
    QVERIFY(touch->setMode(QStringLiteral("full")));
    QCOMPARE(secondFingerRoute(*app.view), input::Route::Gesture);
    const qreal z0 = app.view->zoom();
    QVERIFY(app.commands->trigger(QStringLiteral("users.dual")));
    QCOMPARE(secondFingerRoute(*app.view), input::Route::Tool);
    QVERIFY(app.commands->trigger(QStringLiteral("users.dual")));
    QCOMPARE(secondFingerRoute(*app.view), input::Route::Gesture);
    QCOMPARE(app.view->zoom(), z0);
}

void TstUiSecondpalette::secondUserReceivesInDualMode()
{
    App app;
    QVERIFY(start(app));
    auto* palette = app.window->findChild<QQuickItem*>(QStringLiteral("secondPalette"));
    QVERIFY(palette);
    QVERIFY(!palette->isVisible());
    QVERIFY(app.commands->trigger(QStringLiteral("users.dual")));
    QVERIFY(app.dual->active());
    QVERIFY(app.tools->dualUsers());
    QVERIFY(dynamic_cast<input::TwoUserPolicy*>(app.view->router().policy()));
    QVERIFY(palette->isVisible());

    // Gestionnaires (argument, user) de Main.qml : l'outil et les réglages de l'utilisateur 1 changent,
    // jamais ceux du premier (activate("highlighter", 1) prendrait 1 pour l'argument de l'outil du premier ;
    // setColor(color) à un argument ignorerait l'utilisateur).
    ui::ToolHost& tools = *app.tools;
    QVERIFY(app.commands->trigger(QStringLiteral("tool.highlighter"), QVariant(), 1));
    QCOMPARE(tools.user(1).currentTool(), QStringLiteral("highlighter"));
    QCOMPARE(tools.user(0).currentTool(), QStringLiteral("pen"));
    QCOMPARE(tools.currentTool(), QStringLiteral("pen"));
    QVERIFY(app.commands->trigger(QStringLiteral("tool.pen"), QVariant(), 1));
    QCOMPARE(tools.user(1).currentTool(), QStringLiteral("pen"));
    const QColor first = penColor(tools, 0);
    const double firstWidth = tools.user(0).settings(QStringLiteral("pen")).width;
    QVERIFY(app.commands->trigger(QStringLiteral("pen.color"), QStringLiteral("#1E88E5"), 1));
    QCOMPARE(penColor(tools, 1), QColor(QStringLiteral("#1E88E5")));
    QCOMPARE(penColor(tools, 0), first);
    QVERIFY(app.commands->trigger(QStringLiteral("pen.width"), 2, 1));
    QCOMPARE(tools.user(1).settings(QStringLiteral("pen")).width, 2.0);
    QCOMPARE(tools.user(0).settings(QStringLiteral("pen")).width, firstWidth);

    // La deuxième palette passe par trigger (signal triggered), pour l'utilisateur 1.
    QSignalSpy triggered(app.commands, &ui::Commands::triggered);
    QQuickItem* green = findItem(palette, QStringLiteral("secondColor_2"));
    QVERIFY(green);
    QVariant done;
    QVERIFY(QMetaObject::invokeMethod(green, "press", Q_RETURN_ARG(QVariant, done)));
    QVERIFY(done.toBool());
    QCOMPARE(triggered.count(), 1);
    QCOMPARE(triggered.at(0).at(0).toString(), QStringLiteral("pen.color"));
    QCOMPARE(penColor(tools, 1), QColor(QStringLiteral("#43A047")));
    QCOMPARE(penColor(tools, 0), first);

    // Stylets : l'appareil 11, vu le premier, va à l'utilisateur 0 ; l'appareil 22 à l'utilisateur 1, à son
    // outil, pas à celui du premier.
    const std::unique_ptr<QPointingDevice> stylus11 = makeStylus(1011, 11);
    const std::unique_ptr<QPointingDevice> stylus22 = makeStylus(1022, 22);
    tablet(app, stylus11.get(), QEvent::TabletPress, QPointF(600, 200));
    tablet(app, stylus11.get(), QEvent::TabletMove, QPointF(640, 210));
    tablet(app, stylus22.get(), QEvent::TabletPress, QPointF(100, 400));
    tablet(app, stylus22.get(), QEvent::TabletMove, QPointF(140, 410));
    QCOMPARE(app.view->lastContactUser(), 1);
    tools::Tool* own = tools.activeTool(0);
    tools::Tool* other = tools.activeTool(1);
    QVERIFY(own && other);
    QVERIFY(own != other);
    QVERIFY(own->isBusy());
    QVERIFY(other->isBusy());
    tablet(app, stylus22.get(), QEvent::TabletRelease, QPointF(140, 410));
    QVERIFY(own->isBusy()); // le relâché de l'un ne termine pas l'autre
    QVERIFY(!other->isBusy());
    tablet(app, stylus11.get(), QEvent::TabletRelease, QPointF(640, 210));
    QVERIFY(!own->isBusy());
}

void TstUiSecondpalette::undoCancelsTheSecondUsersGesture()
{
    // Revue de L11-T9, majeur 1 : Annuler (editAboutToApply → finishStroke) abandonne aussi le glisser du
    // second utilisateur, sans étape. Sans cela, le relâché synthétisé termine son glisser par un déplacement
    // (une étape), et Annuler retire ce déplacement au lieu du trait du premier.
    App app;
    QVERIFY(start(app));
    ui::DocumentController* controller = app.tools->documentController();
    QVERIFY(controller);
    QVERIFY(app.commands->trigger(QStringLiteral("users.dual")));
    QVERIFY(app.commands->trigger(QStringLiteral("tool.select"), QVariant(), 1));
    const int before = controller->shownObjectCount();
    const core::ObjectId box = addBox(*controller, {100, 100, 200, 200});
    QTRY_VERIFY(app.view->cacheReady());
    const double left = core::boundsOnPage(*objectOf(*controller, box)).x;
    const std::unique_ptr<QPointingDevice> stylus11 = makeStylus(1011, 11);
    const std::unique_ptr<QPointingDevice> stylus22 = makeStylus(1022, 22);
    tablet(app, stylus11.get(), QEvent::TabletPress, QPointF(600, 200));
    tablet(app, stylus11.get(), QEvent::TabletMove, QPointF(640, 210));
    tablet(app, stylus11.get(), QEvent::TabletMove, QPointF(680, 220));
    tablet(app, stylus11.get(), QEvent::TabletRelease, QPointF(680, 220));
    QCOMPARE(controller->shownObjectCount(), before + 2); // la boîte, puis le trait du premier : une étape

    tablet(app, stylus22.get(), QEvent::TabletPress, QPointF(200, 200));
    for (int i = 1; i <= 6; ++i) {
        tablet(app, stylus22.get(), QEvent::TabletMove, QPointF(200 + 10 * i, 200 + 8 * i));
    }
    QCOMPARE(app.view->lastContactUser(), 1);
    tools::Tool* other = app.tools->activeTool(1);
    QVERIFY(other && other->isBusy());
    QVERIFY(controller->isLifting()); // la boîte glisse

    controller->undo();
    QVERIFY(!other->isBusy());
    QVERIFY(!controller->isLifting());
    QCOMPARE(controller->shownObjectCount(), before + 1); // trait du premier retiré, la boîte reste
    QVERIFY(std::abs(core::boundsOnPage(*objectOf(*controller, box)).x - left) < 0.01); // jamais déplacée
    tablet(app, stylus22.get(), QEvent::TabletMove, QPointF(280, 260));
    tablet(app, stylus22.get(), QEvent::TabletRelease, QPointF(280, 260));
    QVERIFY(std::abs(core::boundsOnPage(*objectOf(*controller, box)).x - left) < 0.01);
    QCOMPARE(controller->shownObjectCount(), before + 1);
}

void TstUiSecondpalette::touchModeChangeKeepsTheUsers()
{
    // Revue de L11-T9, majeur 3 : changer le mode du toucher en mode à deux repose une TwoUserPolicy ; elle
    // garde le stylet du premier utilisateur. Sans cela, le stylet qui se pose le premier après le changement
    // devient l'utilisateur 0 : utilisateurs permutés.
    App app;
    QVERIFY(start(app));
    auto* touch = app.window->findChild<ui::TouchModeController*>(QStringLiteral("appTouchMode"));
    QVERIFY(touch);
    QVERIFY(app.commands->trigger(QStringLiteral("users.dual")));
    const std::unique_ptr<QPointingDevice> stylus11 = makeStylus(1011, 11);
    const std::unique_ptr<QPointingDevice> stylus22 = makeStylus(1022, 22);
    tablet(app, stylus11.get(), QEvent::TabletPress, QPointF(600, 200));
    tablet(app, stylus11.get(), QEvent::TabletMove, QPointF(640, 210));
    QCOMPARE(app.view->lastContactUser(), 0);
    tablet(app, stylus11.get(), QEvent::TabletRelease, QPointF(640, 210));
    const auto* policy = dynamic_cast<const input::TwoUserPolicy*>(app.view->router().policy());
    QVERIFY(policy);
    const std::uint64_t first = policy->firstStylusDevice();
    QVERIFY(first != 0);

    const QString next =
        touch->mode() == QStringLiteral("select") ? QStringLiteral("pen") : QStringLiteral("select");
    QVERIFY(touch->setMode(next));
    policy = dynamic_cast<const input::TwoUserPolicy*>(app.view->router().policy());
    QVERIFY(policy);
    QCOMPARE(policy->firstStylusDevice(), first);

    tablet(app, stylus22.get(), QEvent::TabletPress, QPointF(100, 400));
    tablet(app, stylus22.get(), QEvent::TabletMove, QPointF(140, 410));
    QCOMPARE(app.view->lastContactUser(), 1); // le premier posé après le changement reste le second
    tablet(app, stylus22.get(), QEvent::TabletRelease, QPointF(140, 410));
    tablet(app, stylus11.get(), QEvent::TabletPress, QPointF(600, 300));
    tablet(app, stylus11.get(), QEvent::TabletMove, QPointF(640, 310));
    QCOMPARE(app.view->lastContactUser(), 0);
    tablet(app, stylus11.get(), QEvent::TabletRelease, QPointF(640, 310));
}

void TstUiSecondpalette::designFlagsReachTheSecondUser()
{
    // Revue de L11-T9, majeur 2 : le mode conception (verrou ignoré, accrochage coupé) atteint aussi le
    // contexte des outils du second utilisateur, créés avant le changement.
    App app;
    QVERIFY(start(app));
    ui::DocumentController* controller = app.tools->documentController();
    QVERIFY(controller);
    QVERIFY(app.commands->trigger(QStringLiteral("users.dual")));
    QVERIFY(app.commands->trigger(QStringLiteral("tool.select")));
    QVERIFY(app.commands->trigger(QStringLiteral("tool.select"), QVariant(), 1));
    const core::ObjectId box = addBox(*controller, {100, 100, 200, 200}, true);
    QTRY_VERIFY(app.view->cacheReady());
    const std::unique_ptr<QPointingDevice> stylus11 = makeStylus(1011, 11);
    const std::unique_ptr<QPointingDevice> stylus22 = makeStylus(1022, 22);
    stylusDrag(app, stylus11.get(), QPointF(900, 600), QPointF(900, 600)); // l'appareil 11 : utilisateur 0
    const double left = core::boundsOnPage(*objectOf(*controller, box)).x;
    stylusDrag(app, stylus22.get(), QPointF(200, 200), QPointF(260, 248));
    QCOMPARE(app.view->lastContactUser(), 1);
    QCOMPARE(core::boundsOnPage(*objectOf(*controller, box)).x, left); // verrouillé : immobile

    app.tools->setLockIgnored(true); // outils du second déjà créés
    stylusDrag(app, stylus22.get(), QPointF(200, 200), QPointF(260, 248));
    const double moved = core::boundsOnPage(*objectOf(*controller, box)).x;
    QVERIFY2(moved - left > 30, qPrintable(QString::number(moved - left)));
    QVERIFY(objectOf(*controller, box)->common.locked);

    app.tools->setLockIgnored(false);
    stylusDrag(app, stylus22.get(), QPointF(260, 250), QPointF(320, 298));
    QCOMPARE(core::boundsOnPage(*objectOf(*controller, box)).x, moved);
}

void TstUiSecondpalette::revealInkKeepsTheUser()
{
    // Revue de L11-T9, mineur : tool.reveal-ink (ActionOverlays.qml) change l'outil de l'utilisateur qui le
    // déclenche, jamais celui du premier pour le second.
    App app;
    QVERIFY(start(app));
    QVERIFY(app.commands->trigger(QStringLiteral("users.dual")));
    QVERIFY(app.commands->trigger(QStringLiteral("tool.reveal-ink"), QVariant(), 1));
    QCOMPARE(app.tools->user(1).currentTool(), QStringLiteral("reveal-ink"));
    QCOMPARE(app.tools->user(0).currentTool(), QStringLiteral("pen"));
    QVERIFY(app.commands->trigger(QStringLiteral("tool.reveal-ink")));
    QCOMPARE(app.tools->user(0).currentTool(), QStringLiteral("reveal-ink"));
}

void TstUiSecondpalette::nestedTriggerKeepsTheUser()
{
    // Revue de L11-T9, mineur : un déclenchement imbriqué par l'ancienne signature garde l'utilisateur du
    // déclenchement en cours ; une commande à boîte relancée pour le second reste refusée.
    ui::Commands commands;
    commands.setProfileDir(m_profile.path());
    HandlerProbe probe(commands);
    commands.setHandler(QStringLiteral("file.open"), probe.handler(QStringLiteral("open")));
    bool nested = true;
    commands.setNativeHandler(QStringLiteral("pen.color"), [&commands, &nested](const QVariant&) {
        nested = commands.trigger(QStringLiteral("file.open"));
    });
    QSignalSpy refused(&commands, &ui::Commands::refusedForSecondUser);
    QVERIFY(commands.trigger(QStringLiteral("pen.color"), QStringLiteral("#1E88E5"), 1));
    QVERIFY(!nested);
    QCOMPARE(probe.calls(QStringLiteral("open")), 0);
    QCOMPARE(refused.count(), 1);
    QVERIFY(commands.trigger(QStringLiteral("pen.color"), QStringLiteral("#1E88E5"), 0));
    QVERIFY(nested);
    QCOMPARE(probe.calls(QStringLiteral("open")), 1);
    QVERIFY(commands.trigger(QStringLiteral("file.open"))); // hors déclenchement : le premier
    QCOMPARE(probe.calls(QStringLiteral("open")), 2);
}

void TstUiSecondpalette::toggleWaitsForTheContactToEnd()
{
    // Revue de L11-T9, mineur : la bascule demandée pendant un contact attend sa fin (applyPending).
    App app;
    QVERIFY(start(app));
    const std::unique_ptr<QPointingDevice> stylus11 = makeStylus(1011, 11);
    tablet(app, stylus11.get(), QEvent::TabletPress, QPointF(600, 200));
    tablet(app, stylus11.get(), QEvent::TabletMove, QPointF(640, 210));
    QVERIFY(app.view->contactActive());
    QVERIFY(app.commands->trigger(QStringLiteral("users.dual")));
    QVERIFY(!app.dual->active());
    QVERIFY(!app.tools->dualUsers());
    tablet(app, stylus11.get(), QEvent::TabletRelease, QPointF(640, 210));
    QVERIFY(app.dual->active());
    QVERIFY(app.tools->dualUsers());
    QVERIFY(dynamic_cast<input::TwoUserPolicy*>(app.view->router().policy()));
}

QTEST_MAIN(TstUiSecondpalette)
#include "tst_ui_secondpalette.moc"
