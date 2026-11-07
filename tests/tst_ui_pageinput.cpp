#include "startup.h"

#include <actcraft/core/model.h>
#include <actcraft/input/routing.h>
#include <actcraft/input/sample.h>
#include <actcraft/input/touchmodes.h>
#include <actcraft/input/twouserpolicy.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/tools/userstate.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTabletEvent>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <algorithm>
#include <cstddef>
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

    // L11-C1 (revue T7, M1) : mode à deux, le stylet 11 écrit, le stylet 22 appuie sur une couverture
    // déclarée. La vue ne rend au routeur que le contact du stylet 22 : le trait du stylet 11 continue et
    // se termine à son relâché. Ancien patron (release(0) et un seul m_stylusDrawing) : la suite du stylet
    // 11 n'atteint plus l'outil et son trait n'est jamais versé.
    void coverPressOfOneStylusKeepsTheOther()
    {
        App app;
        QVERIFY(start(app));
        app.tools->setDualUsers(true);
        app.view->setRoutingPolicy(std::make_unique<input::TwoUserPolicy>(QRectF()));
        QQuickItem cover(app.window->contentItem());
        cover.setPosition(app.view->pageToScene(QPointF(600, 500)) - QPointF(40, 40));
        cover.setSize(QSizeF(80, 80));
        app.view->addInputCover(&cover);
        bool covered = false;
        int afterCover = 0;
        app.view->setSampleFilter([&](const input::Sample& sample) {
            if (covered && sample.deviceId == 11) {
                ++afterCover;
            }
            return sample;
        });
        const int before = app.fixed->objectCount();
        stylusDown(app, m_stylus11.get(), QPointF(300, 300));
        QVERIFY(app.view->contactActive());
        covered = true;
        tablet(app, m_stylus22.get(), QEvent::TabletPress, QPointF(600, 500));
        QVERIFY(app.view->contactActive()); // le stylet 11 écrit toujours
        tablet(app, m_stylus11.get(), QEvent::TabletMove, QPointF(360, 315));
        tablet(app, m_stylus11.get(), QEvent::TabletMove, QPointF(380, 320));
        tablet(app, m_stylus11.get(), QEvent::TabletRelease, QPointF(380, 320));
        tablet(app, m_stylus22.get(), QEvent::TabletRelease, QPointF(600, 500));
        app.view->setSampleFilter({});
        QCOMPARE(afterCover, 3); // deux mouvements et le relâché du stylet 11, versés à son outil
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->objectCount(), before + 1, 3000);
        QVERIFY(!app.view->contactActive());
    }

    // L11-C1 (revue T7, M4) : mode à deux ; un doigt de l'utilisateur 1 et un de l'utilisateur 0 écrivent ;
    // le stylet de l'utilisateur 0 se pose : le trait de son doigt se termine (gardé), le doigt de
    // l'utilisateur 1 continue d'écrire. Aire en coordonnées de page (échantillons routés).
    void stylusEndsOnlyItsUsersFinger()
    {
        App app;
        QVERIFY(start(app));
        app.tools->setDualUsers(true);
        app.view->setRoutingPolicy(std::make_unique<input::TwoUserPolicy>(QRectF(0, 0, 500, 5000)));
        bool stylusDown = false;
        int ownMoves = 0;
        int otherMoves = 0;
        app.view->setSampleFilter([&](const input::Sample& sample) {
            if (stylusDown && sample.device == input::DeviceKind::Touch &&
                sample.phase == input::Phase::Move) {
                ++(sample.user == input::kFirstUser ? ownMoves : otherMoves);
            }
            return sample;
        });
        const int before = app.fixed->objectCount();
        QTest::touchEvent(app.window, m_touch).press(0, scenePoint(app, QPointF(300, 300)));
        QTest::touchEvent(app.window, m_touch).stationary(0).press(1, scenePoint(app, QPointF(700, 300)));
        QTest::touchEvent(app.window, m_touch)
            .move(0, scenePoint(app, QPointF(320, 300)))
            .move(1, scenePoint(app, QPointF(720, 300)));
        QCOMPARE(app.view->lastContactUser(), 0);
        stylusDown = true;
        tablet(app, m_stylus11.get(), QEvent::TabletPress, QPointF(700, 500));
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->objectCount(), before + 1, 3000); // trait du doigt gardé
        for (int i = 1; i <= 2; ++i) {
            QTest::touchEvent(app.window, m_touch)
                .move(0, scenePoint(app, QPointF(320 + 20 * i, 300)))
                .move(1, scenePoint(app, QPointF(720 + 20 * i, 300)));
        }
        // Qt Quick regroupe les mouvements de toucher jusqu'à l'image suivante : chaque événement livré
        // porte les deux doigts.
        QTRY_VERIFY_WITH_TIMEOUT(otherMoves >= 1, 3000);
        QTest::qWait(50);
        app.view->setSampleFilter({});
        QCOMPARE(ownMoves, 0);
        QTest::touchEvent(app.window, m_touch)
            .release(0, scenePoint(app, QPointF(360, 300)))
            .release(1, scenePoint(app, QPointF(760, 300)));
        tablet(app, m_stylus11.get(), QEvent::TabletRelease, QPointF(700, 500));
        QTRY_VERIFY_WITH_TIMEOUT(!app.view->contactActive(), 3000);
    }

    // L11-C1 (revue T7, M3) : l'aire du second utilisateur est posée en coordonnées de vue et suit le zoom :
    // à zoom 2 autour du bord gauche, un doigt dans la moitié droite de la vue reste à l'utilisateur 0 (une
    // aire laissée en coordonnées de page le donnerait à l'utilisateur 1).
    void secondUserViewAreaFollowsZoom()
    {
        App app;
        QVERIFY(start(app));
        app.tools->setDualUsers(true);
        app.view->setRoutingPolicy(std::make_unique<input::TwoUserPolicy>(QRectF()));
        const qreal w = app.view->width();
        const qreal h = app.view->height();
        app.view->setSecondUserViewArea(QRectF(0, 0, w / 2, h));
        const auto tap = [&](QPointF viewPoint) {
            const QPoint scene = app.view->mapToScene(viewPoint).toPoint();
            QTest::touchEvent(app.window, m_touch).press(0, scene);
            QTest::touchEvent(app.window, m_touch).release(0, scene);
            return app.view->lastContactUser();
        };
        QCOMPARE(tap(QPointF(w * 0.25, h / 2)), 1);
        QCOMPARE(tap(QPointF(w * 0.75, h / 2)), 0);
        const qreal before = app.view->zoom();
        app.view->zoomAround(QPointF(0, h / 2), 2 * before);
        QVERIFY(app.view->zoom() > before * 1.5);
        QCOMPARE(tap(QPointF(w * 0.25, h / 2)), 1);
        QCOMPARE(tap(QPointF(w * 0.75, h / 2)), 0);
        // Revue de C1, m2 : politique remplacée, l'aire de vue posée prévaut sur celle de sa construction.
        app.view->setRoutingPolicy(std::make_unique<input::TwoUserPolicy>(QRectF(-1e6, -1e6, 2e6, 2e6)));
        QCOMPARE(tap(QPointF(w * 0.75, h / 2)), 0);
        QCOMPARE(tap(QPointF(w * 0.25, h / 2)), 1);
        // Revue de C1, m3 : politique remplacée par le routeur ; la vue la retrouve par lui (aucun pointeur
        // gardé qui pendrait) et lui donne l'aire au zoom suivant.
        app.view->router().setPolicy(std::make_unique<input::TwoUserPolicy>(QRectF()));
        app.view->zoomAround(QPointF(0, h / 2), app.view->zoom() / 2);
        QCOMPARE(tap(QPointF(w * 0.75, h / 2)), 0);
        QCOMPARE(tap(QPointF(w * 0.25, h / 2)), 1);
    }

    // Revue de C1, m1 : à la seconde livraison d'un événement à appui [A bouge, B appuyé], le mouvement de A
    // ne repasse pas : l'outil reçoit chaque point une fois.
    void secondDeliveryHasNoEffect()
    {
        App app;
        QVERIFY(start(app));
        app.tools->setDualUsers(true);
        app.view->setRoutingPolicy(std::make_unique<input::TwoUserPolicy>(QRectF(0, 0, 500, 5000)));
        int presses = 0;
        int movesOfA = 0;
        app.view->setSampleFilter([&](const input::Sample& sample) {
            if (sample.device == input::DeviceKind::Touch) {
                presses += sample.phase == input::Phase::Press ? 1 : 0;
                movesOfA += sample.phase == input::Phase::Move && sample.user == input::kSecondUser ? 1 : 0;
            }
            return sample;
        });
        QTest::touchEvent(app.window, m_touch).press(0, scenePoint(app, QPointF(300, 300)));
        QTest::touchEvent(app.window, m_touch)
            .move(0, scenePoint(app, QPointF(330, 300)))
            .press(1, scenePoint(app, QPointF(700, 300)));
        QCOMPARE(presses, 2);
        QCOMPARE(movesOfA, 1);
        QTest::touchEvent(app.window, m_touch)
            .release(0, scenePoint(app, QPointF(330, 300)))
            .release(1, scenePoint(app, QPointF(700, 300)));
        app.view->setSampleFilter({});
        QTRY_VERIFY_WITH_TIMEOUT(!app.view->contactActive(), 3000);
    }

    // L11-C1 (revue T5, M2, D11-4) : mode select, outil courant le stylo ; un appui au doigt sur un objet le
    // sélectionne par le sélecteur que fournit ToolHost, sans changer l'outil courant. Sans fournisseur, le
    // doigt n'atteint aucun outil.
    void imposedSelectorSelectsWhilePenIsCurrent()
    {
        App app;
        QVERIFY(start(app));
        app.tools->activate(QStringLiteral("pen"));
        const int before = app.fixed->objectCount();
        fingerStroke(app, QPointF(300, 400), QPointF(500, 400));
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->objectCount(), before + 1, 3000);
        QCOMPARE(app.tools->selectionCount(), 0);
        app.view->setRoutingPolicy(input::makeTouchPolicy(input::TouchMode::Select));
        QTest::touchEvent(app.window, m_touch).press(0, scenePoint(app, QPointF(400, 400)));
        QTest::touchEvent(app.window, m_touch).release(0, scenePoint(app, QPointF(400, 400)));
        QTRY_COMPARE_WITH_TIMEOUT(app.tools->selectionCount(), 1, 3000);
        QCOMPARE(app.tools->user(input::kFirstUser).currentTool(), QStringLiteral("pen"));
        QCOMPARE(app.fixed->objectCount(), before + 1); // aucun trait au doigt
    }

    // L11-C1 (revue T5, hors périmètre) : un événement [A relâché, B appuyé] ; l'appui de B n'est décidé et
    // versé à l'outil qu'une fois, même si Qt Quick livre l'événement deux fois à la vue.
    void releaseAndPressInOneEventDecidedOnce()
    {
        App app;
        QVERIFY(start(app));
        int presses = 0;
        app.view->setSampleFilter([&presses](const input::Sample& sample) {
            if (sample.device == input::DeviceKind::Touch && sample.phase == input::Phase::Press) {
                ++presses;
            }
            return sample;
        });
        const int before = app.fixed->objectCount();
        QTest::touchEvent(app.window, m_touch).press(0, scenePoint(app, QPointF(300, 300)));
        QTest::touchEvent(app.window, m_touch).move(0, scenePoint(app, QPointF(340, 300)));
        QCOMPARE(presses, 1);
        QTest::touchEvent(app.window, m_touch)
            .release(0, scenePoint(app, QPointF(340, 300)))
            .press(1, scenePoint(app, QPointF(300, 450)));
        QCOMPARE(presses, 2); // B : une décision, un appui versé
        QTest::touchEvent(app.window, m_touch).move(1, scenePoint(app, QPointF(340, 450)));
        QTest::touchEvent(app.window, m_touch).release(1, scenePoint(app, QPointF(340, 450)));
        app.view->setSampleFilter({});
        QCOMPARE(presses, 2);
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->objectCount(), before + 2, 3000);
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
        std::vector<input::Sample> seen;
        App app;
        QVERIFY(start(app));
        app.view->setSampleFilter([&toolSamples, &seen](const input::Sample& sample) {
            ++toolSamples;
            seen.push_back(sample);
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

        // Revue M1 : le doigt qui écrit se relève dans l'événement où un autre se pose ([A Released,
        // B Pressed]) : le relâché passe au routeur avant les appuis ; A garde son trait, B est un premier
        // doigt (outil), pas un deuxième (geste).
        const std::size_t markA = seen.size();
        QTest::touchEvent(app.window, m_touch).press(0, scenePoint(app, QPointF(300, 550)));
        QTest::touchEvent(app.window, m_touch).move(0, scenePoint(app, QPointF(360, 550)));
        QTRY_VERIFY_WITH_TIMEOUT(app.ink->pointCount() >= 2, 3000);
        QVERIFY(seen.size() > markA);
        const std::uint64_t pointerA = seen[markA].pointer;
        const std::size_t markHandover = seen.size();
        QTest::touchEvent(app.window, m_touch)
            .release(0, scenePoint(app, QPointF(360, 550)))
            .press(1, scenePoint(app, QPointF(300, 600)));
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->objectCount(), before + 2, 3000);
        const auto handover = [&](input::Phase phase, bool isA) {
            return std::any_of(seen.cbegin() + static_cast<std::ptrdiff_t>(markHandover), seen.cend(),
                               [&](const input::Sample& sample) {
                                   return sample.phase == phase && (sample.pointer == pointerA) == isA;
                               });
        };
        QVERIFY2(handover(input::Phase::Release, true), "le relâché de A n'a pas atteint l'outil");
        QVERIFY2(handover(input::Phase::Press, false), "l'appui de B n'a pas atteint l'outil");
        QCOMPARE(app.view->router().gestureTouches(), 0);
        QVERIFY(app.view->contactActive());
        QTest::touchEvent(app.window, m_touch).move(1, scenePoint(app, QPointF(360, 600)));
        QTest::touchEvent(app.window, m_touch).release(1, scenePoint(app, QPointF(360, 600)));
        QTRY_COMPARE_WITH_TIMEOUT(app.fixed->objectCount(), before + 3, 3000);
        QVERIFY(!app.view->contactActive());
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
        // Revue M3 : la translucidité du stylo de l'utilisateur suit le trait imposé.
        tools::InkSettings pen = app.tools->user().settings(QStringLiteral("pen"));
        pen.translucency = 100;
        app.tools->user().setSettings(QStringLiteral("pen"), pen);
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
        auto* document = app.window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        QVERIFY(document);
        const core::Page& page =
            document->document().page(static_cast<std::size_t>(document->currentIndex()));
        QVERIFY(!page.objects.empty());
        QCOMPARE(int(page.objects.back().common.translucency), 100);
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
        QVERIFY(!app.view->touchHoldTimerActive());

        // Relâché avant 500 ms : rien, minuteur arrêté au relâché.
        QTest::touchEvent(app.window, m_touch).press(0, scenePoint(app, QPointF(400, 300)));
        QTest::qWait(200);
        QVERIFY(app.view->touchHoldTimerActive());
        QTest::touchEvent(app.window, m_touch).release(0, scenePoint(app, QPointF(400, 300)));
        QVERIFY(!app.view->touchHoldTimerActive());
        QTest::qWait(400);
        QCOMPARE(held.count(), 1);

        // Tremblement de 5 dp, dans la tolérance : le signal part quand même.
        const QPoint still = scenePoint(app, QPointF(300, 400));
        QTest::touchEvent(app.window, m_touch).press(0, still);
        QTest::touchEvent(app.window, m_touch).move(0, still + QPoint(5, 0));
        QTest::qWait(600);
        QCOMPARE(held.count(), 2);
        QTest::touchEvent(app.window, m_touch).release(0, still + QPoint(5, 0));

        // Mouvement de 20 dp : minuteur arrêté, aucun signal.
        const QPoint at = scenePoint(app, QPointF(300, 500));
        QTest::touchEvent(app.window, m_touch).press(0, at);
        QTest::touchEvent(app.window, m_touch).move(0, at + QPoint(20, 0));
        // Avant l'échéance de 500 ms : arrêté par le mouvement (Qt Quick livre les mouvements à l'image).
        QTRY_VERIFY_WITH_TIMEOUT(!app.view->touchHoldTimerActive(), 200);
        QTest::qWait(600);
        QTest::touchEvent(app.window, m_touch).release(0, at + QPoint(20, 0));
        QCOMPARE(held.count(), 2);

        // Stylet : aucun minuteur armé hors d'un contact de doigt.
        tablet(app, m_stylus11.get(), QEvent::TabletPress, QPointF(500, 300));
        QVERIFY(!app.view->touchHoldTimerActive());
        tablet(app, m_stylus11.get(), QEvent::TabletRelease, QPointF(500, 300));
        QCOMPARE(held.count(), 2);
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
