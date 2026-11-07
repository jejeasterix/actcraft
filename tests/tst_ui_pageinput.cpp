#include "startup.h"

#include <actcraft/input/routing.h>
#include <actcraft/input/sample.h>
#include <actcraft/input/touchmodes.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTabletEvent>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <cstdint>
#include <memory>
#include <vector>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

struct App
{
    QTemporaryDir profiles; // profils d'essai, détruits après le moteur
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
    render::FixedPageItem* fixed = nullptr;
    render::InkItem* ink = nullptr;
    ui::ToolHost* tools = nullptr;
};

// Montage de tst_ui_drawing : Main.qml chargé, page vide (aucun objet sous le doigt du sélecteur).
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
    app.fixed = app.window->findChild<render::FixedPageItem*>(QStringLiteral("fixedPage"));
    app.ink = app.window->findChild<render::InkItem*>(QStringLiteral("ink"));
    app.tools = app.window->findChild<ui::ToolHost*>(QStringLiteral("appTools"));
    if (!app.view || !app.fixed || !app.ink || !app.tools) {
        return false;
    }
    return QTest::qWaitFor([&] { return app.fixed->isReady() && app.view->cacheReady(); }, 20000);
}

QPoint scenePoint(const App& app, QPointF page)
{
    return app.view->pageToScene(page).toPoint();
}

QPointF viewCentre(const App& app)
{
    return QPointF(app.view->width() / 2, app.view->height() / 2);
}

// Politique d'essai : tout appui va à l'outil de l'utilisateur 1 (deuxième utilisateur).
class SecondUserPolicy final : public input::RoutingPolicy
{
public:
    input::Decision decide(const input::Sample&, const input::RouterState&) const override
    {
        input::Decision decision;
        decision.route = input::Route::Tool;
        decision.user = 1;
        return decision;
    }
};

} // namespace

class TstUiPageinput : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_settings;
    QPointingDevice* m_touch = nullptr;
    std::unique_ptr<QPointingDevice> m_stylus11; // uniqueId 11, systemId 1011
    std::unique_ptr<QPointingDevice> m_stylus22; // uniqueId 22, systemId 1022

    static std::unique_ptr<QPointingDevice> makeStylus(qint64 systemId, qint64 uniqueId)
    {
        return std::make_unique<QPointingDevice>(
            QStringLiteral("tst-pageinput-stylus-%1").arg(uniqueId), systemId,
            QInputDevice::DeviceType::Stylus, QPointingDevice::PointerType::Pen,
            QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3, QString(),
            QPointingDeviceUniqueId::fromNumericId(uniqueId));
    }

    // Livré tout de suite à la fenêtre (le filtre de la vue capte le stylet).
    static void tablet(App& app, const QPointingDevice* device, QEvent::Type type, QPointF page)
    {
        const bool press = type == QEvent::TabletPress;
        const bool release = type == QEvent::TabletRelease;
        const QPointF scene = app.view->pageToScene(page);
        QTabletEvent event(type, device, scene, app.window->mapToGlobal(scene), release ? 0.0 : 0.7, 0, 0, 0,
                           0, 0, Qt::NoModifier, (press || release) ? Qt::LeftButton : Qt::NoButton,
                           release ? Qt::NoButton : Qt::LeftButton);
        QCoreApplication::sendEvent(app.window, &event);
    }

    // Appui et deux mouvements, sans relâché.
    static void stylusDown(App& app, const QPointingDevice* device, QPointF from)
    {
        tablet(app, device, QEvent::TabletPress, from);
        tablet(app, device, QEvent::TabletMove, from + QPointF(20, 5));
        tablet(app, device, QEvent::TabletMove, from + QPointF(40, 10));
    }

    void fingerStroke(App& app, QPointF from, QPointF to)
    {
        QTest::touchEvent(app.window, m_touch).press(0, scenePoint(app, from));
        for (int i = 1; i <= 5; ++i) {
            QTest::touchEvent(app.window, m_touch).move(0, scenePoint(app, from + (to - from) * i / 5));
        }
        QTest::touchEvent(app.window, m_touch).release(0, scenePoint(app, to));
    }

private slots:
    void initTestCase()
    {
        // Main.qml porte ToolHost et Preferences : réglages d'essai en INI temporaire, jamais le profil réel.
        QStandardPaths::setTestModeEnabled(true);
        QVERIFY(m_settings.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
        m_touch = QTest::createTouchDevice();
        m_stylus11 = makeStylus(1011, 11);
        m_stylus22 = makeStylus(1022, 22);
        QSettings probe;
        QCOMPARE(probe.format(), QSettings::IniFormat);
        QVERIFY(probe.fileName().startsWith(m_settings.path()));
    }

    // Point 1 : la politique du routeur se remplace ; le témoin (politique complète remise) écrit.
    void policyReplaceable()
    {
        App app;
        QVERIFY(start(app));
        const int before = app.fixed->objectCount();
        app.view->setRoutingPolicy(input::makeTouchPolicy(input::TouchMode::Ignore));
        fingerStroke(app, QPointF(300, 300), QPointF(500, 300));
        QTest::qWait(100);
        QCOMPARE(app.fixed->objectCount(), before);
        app.view->setRoutingPolicy(input::makeTouchPolicy(input::TouchMode::Full));
        fingerStroke(app, QPointF(300, 400), QPointF(500, 400));
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->objectCount(), before + 1, 3000);
    }

    // Point 2 : Sample::deviceId vient de QPointingDevice::uniqueId (11, 22), pas de systemId (1011, 1022).
    void deviceIdFilled()
    {
        std::vector<input::Sample> seen;
        App app;
        QVERIFY(start(app));
        app.view->setSampleFilter([&seen](const input::Sample& sample) {
            seen.push_back(sample);
            return sample;
        });
        stylusDown(app, m_stylus11.get(), QPointF(300, 300));
        tablet(app, m_stylus11.get(), QEvent::TabletRelease, QPointF(340, 310));
        const std::size_t first = seen.size();
        QVERIFY(first >= 3);
        stylusDown(app, m_stylus22.get(), QPointF(300, 400));
        tablet(app, m_stylus22.get(), QEvent::TabletRelease, QPointF(340, 410));
        app.view->setSampleFilter({});
        QVERIFY(seen.size() > first);
        for (std::size_t i = 0; i < seen.size(); ++i) {
            QCOMPARE(seen[i].deviceId, i < first ? std::uint64_t{11} : std::uint64_t{22});
        }
    }

    // Point 3 : deux appuis d'un même QTouchEvent sont décidés ensemble : tous deux au geste, aucun
    // échantillon à l'outil. Puis H11-O4 : un doigt qui écrit, deux doigts posés ensemble → les deux au
    // geste, le trait du premier terminé et gardé (il passe à Ignore, il ne rejoint pas le geste).
    void sameEventPressesGrouped()
    {
        int toolSamples = 0;
        App app;
        QVERIFY(start(app));
        app.view->setSampleFilter([&toolSamples](const input::Sample& sample) {
            ++toolSamples;
            return sample;
        });
        const int before = app.fixed->objectCount();
        QTest::touchEvent(app.window, m_touch)
            .press(0, scenePoint(app, QPointF(300, 300)))
            .press(1, scenePoint(app, QPointF(500, 300)));
        QTRY_COMPARE_WITH_TIMEOUT(app.view->router().gestureTouches(), 2, 3000);
        QCOMPARE(toolSamples, 0);
        QCOMPARE(app.ink->pointCount(), std::size_t{0});
        QTest::touchEvent(app.window, m_touch)
            .release(0, scenePoint(app, QPointF(300, 300)))
            .release(1, scenePoint(app, QPointF(500, 300)));
        QTRY_COMPARE_WITH_TIMEOUT(app.view->router().gestureTouches(), 0, 3000);
        QCOMPARE(app.fixed->objectCount(), before);

        // H11-O4.
        QTest::touchEvent(app.window, m_touch).press(0, scenePoint(app, QPointF(300, 500)));
        QTest::touchEvent(app.window, m_touch).move(0, scenePoint(app, QPointF(340, 500)));
        QTest::touchEvent(app.window, m_touch).move(0, scenePoint(app, QPointF(380, 500)));
        // Qt Quick peut fusionner deux mouvements consécutifs : on attend un trait d'au moins deux points.
        QTRY_VERIFY_WITH_TIMEOUT(app.ink->pointCount() >= 2, 3000);
        QVERIFY(toolSamples >= 2);
        QTest::touchEvent(app.window, m_touch)
            .stationary(0)
            .press(1, scenePoint(app, QPointF(300, 200)))
            .press(2, scenePoint(app, QPointF(500, 200)));
        QTRY_COMPARE_WITH_TIMEOUT(app.view->router().gestureTouches(), 2, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->objectCount(), before + 1, 3000);
        QTest::touchEvent(app.window, m_touch)
            .release(0, scenePoint(app, QPointF(380, 500)))
            .release(1, scenePoint(app, QPointF(300, 200)))
            .release(2, scenePoint(app, QPointF(500, 200)));
        QTRY_VERIFY_WITH_TIMEOUT(!app.view->contactActive(), 3000);
        QCOMPARE(app.fixed->objectCount(), before + 1);
        app.view->setSampleFilter({});
    }

    // Point 4 : l'utilisateur de la décision atteint la vue (la réception par l'utilisateur 1 est prouvée par
    // L11-T7) ; la décision suivante, de la politique complète, rend l'utilisateur 0.
    void decisionUserReachesTheView()
    {
        std::vector<input::Sample> seen;
        App app;
        QVERIFY(start(app));
        app.view->setSampleFilter([&seen](const input::Sample& sample) {
            seen.push_back(sample);
            return sample;
        });
        app.view->setRoutingPolicy(std::make_unique<SecondUserPolicy>());
        tablet(app, m_stylus11.get(), QEvent::TabletPress, QPointF(300, 300));
        QCOMPARE(app.view->lastContactUser(), 1);
        tablet(app, m_stylus11.get(), QEvent::TabletRelease, QPointF(300, 300));
        QVERIFY(!seen.empty());
        for (const input::Sample& sample : seen) {
            QCOMPARE(int(sample.user), 1);
        }
        app.view->setRoutingPolicy(input::makeTouchPolicy(input::TouchMode::Full));
        tablet(app, m_stylus11.get(), QEvent::TabletPress, QPointF(300, 400));
        QCOMPARE(app.view->lastContactUser(), 0);
        tablet(app, m_stylus11.get(), QEvent::TabletRelease, QPointF(300, 400));
        app.view->setSampleFilter({});
    }

    // Point 4 (D11-4) : en mode pen, le doigt écrit avec le stylo de l'utilisateur ; l'outil courant reste le
    // sélecteur, pendant le trait comme après (aucun aller-retour d'outil).
    void imposedPenDrawsWhileSelectIsCurrent()
    {
        App app;
        QVERIFY(start(app));
        app.tools->activate(QStringLiteral("select"));
        QCOMPARE(app.tools->currentTool(), QStringLiteral("select"));
        app.view->setRoutingPolicy(input::makeTouchPolicy(input::TouchMode::Pen));
        QSignalSpy toolChanged(app.tools, &ui::ToolHost::currentToolChanged);
        const int before = app.fixed->objectCount();
        QTest::touchEvent(app.window, m_touch).press(0, scenePoint(app, QPointF(300, 300)));
        QTest::touchEvent(app.window, m_touch).move(0, scenePoint(app, QPointF(350, 320)));
        QTest::touchEvent(app.window, m_touch).move(0, scenePoint(app, QPointF(400, 340)));
        QCOMPARE(app.tools->currentTool(), QStringLiteral("select"));
        QTest::touchEvent(app.window, m_touch).release(0, scenePoint(app, QPointF(400, 340)));
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->objectCount(), before + 1, 3000);
        QCOMPARE(app.tools->currentTool(), QStringLiteral("select"));
        QCOMPARE(toolChanged.count(), 0);
    }

    // Point 5 : finishContacts termine le contact comme un relâché (trait gardé) et le rend : contactActive
    // faux. La suite du contact n'écrit plus rien.
    void finishContactsKeepsTheStroke()
    {
        App app;
        QVERIFY(start(app));
        const int before = app.fixed->objectCount();
        stylusDown(app, m_stylus11.get(), QPointF(300, 300));
        QVERIFY(app.view->contactActive());
        app.view->finishContacts();
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->objectCount(), before + 1, 3000);
        QVERIFY(!app.view->contactActive());
        tablet(app, m_stylus11.get(), QEvent::TabletMove, QPointF(400, 330));
        tablet(app, m_stylus11.get(), QEvent::TabletRelease, QPointF(400, 330));
        QTest::qWait(50);
        QCOMPARE(app.fixed->objectCount(), before + 1);
        QVERIFY(!app.view->contactActive());
    }

    // Point 6 (D11-7) : doigt routé vers l'outil, immobile 500 ms → touchHeld une fois ; doigt qui bouge de
    // 20 dp, ou stylet immobile → rien.
    void touchHeldAfter500ms()
    {
        App app;
        QVERIFY(start(app));
        QSignalSpy held(app.view, &ui::PageView::touchHeld);
        const QPointF page(300, 300);
        QTest::touchEvent(app.window, m_touch).press(0, scenePoint(app, page));
        QTest::qWait(600);
        QCOMPARE(held.count(), 1);
        QVERIFY(QLineF(held.at(0).at(0).toPointF(), page).length() <= 2.0);
        QVERIFY(QLineF(held.at(0).at(1).toPointF(), app.view->pageToScene(page)).length() <= 2.0);
        QCOMPARE(held.at(0).at(2).toInt(), 0);
        QTest::touchEvent(app.window, m_touch).release(0, scenePoint(app, page));

        const QPoint at = scenePoint(app, QPointF(300, 500));
        QTest::touchEvent(app.window, m_touch).press(0, at);
        QTest::touchEvent(app.window, m_touch).move(0, at + QPoint(20, 0));
        QTest::qWait(600);
        QTest::touchEvent(app.window, m_touch).release(0, at + QPoint(20, 0));
        QCOMPARE(held.count(), 1);

        tablet(app, m_stylus11.get(), QEvent::TabletPress, QPointF(500, 300));
        QTest::qWait(600);
        tablet(app, m_stylus11.get(), QEvent::TabletRelease, QPointF(500, 300));
        QCOMPARE(held.count(), 1);
    }

    // Point 7 (D11-10) : hors « Ajuster », le point de page au centre de la vue y reste à la rotation.
    void resizeKeepsTheCentre()
    {
        App app;
        QVERIFY(start(app));
        app.window->resize(1280, 800);
        QTRY_COMPARE_WITH_TIMEOUT(app.window->size(), QSize(1280, 800), 3000);
        QTest::qWait(50);
        app.view->setZoom(3.0);
        const QPointF centrePage = app.view->viewTransform().viewToPage(viewCentre(app));
        const QSizeF before = app.view->size();
        app.window->resize(800, 1280);
        QTRY_VERIFY_WITH_TIMEOUT(app.view->size() != before, 3000);
        QTest::qWait(50);
        const QPointF after = app.view->viewTransform().pageToView(centrePage);
        const QPointF centre = viewCentre(app);
        QVERIFY2(qAbs(after.x() - centre.x()) <= 1.0 && qAbs(after.y() - centre.y()) <= 1.0,
                 qPrintable(QStringLiteral("point (%1, %2), centre (%3, %4)")
                                .arg(after.x())
                                .arg(after.y())
                                .arg(centre.x())
                                .arg(centre.y())));
    }

    // Point 7 : un contact de stylet en cours au redimensionnement est terminé par finishContacts : trait
    // gardé, contactActive faux.
    void resizeEndsTheContact()
    {
        App app;
        QVERIFY(start(app));
        const int before = app.fixed->objectCount();
        stylusDown(app, m_stylus11.get(), QPointF(300, 300));
        QVERIFY(app.view->contactActive());
        const QSizeF size = app.view->size();
        app.window->resize(app.window->height(), app.window->width());
        QTRY_VERIFY_WITH_TIMEOUT(app.view->size() != size, 3000);
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->objectCount(), before + 1, 3000);
        QVERIFY(!app.view->contactActive());
        tablet(app, m_stylus11.get(), QEvent::TabletRelease, QPointF(340, 310));
        QTest::qWait(50);
        QCOMPARE(app.fixed->objectCount(), before + 1);
    }
};

QTEST_MAIN(TstUiPageinput)
#include "tst_ui_pageinput.moc"
