// Deux utilisateurs I (L11-T7, D11-2) : politique de routage à deux utilisateurs et un état par utilisateur.
#include <actcraft/input/routing.h>
#include <actcraft/input/sample.h>
#include <actcraft/input/twouserpolicy.h>
#include <actcraft/tools/userstate.h>
#include <actcraft/ui/toolhost.h>

#include <QPointF>
#include <QRectF>
#include <QTemporaryDir>
#include <QtTest>

#include <cstdint>
#include <memory>

using namespace actcraft;
using input::DeviceKind;
using input::Route;
using input::RouterState;
using input::UserId;

namespace {

const QRectF kLeftHalf(0, 0, 640, 800); // moitié de la deuxième palette (à gauche) : l'utilisateur 1

input::Sample sampleOf(input::Phase phase, DeviceKind device, double x, double y, std::uint64_t deviceId = 0,
                       std::uint64_t pointer = 0)
{
    input::Sample s;
    s.x = x;
    s.y = y;
    s.device = device;
    s.phase = phase;
    s.deviceId = deviceId;
    s.pointer = pointer;
    return s;
}

input::Sample press(DeviceKind device, double x, double y, std::uint64_t deviceId = 0,
                    std::uint64_t pointer = 0)
{
    return sampleOf(input::Phase::Press, device, x, y, deviceId, pointer);
}

input::Sample moved(DeviceKind device, double x, double y, std::uint64_t deviceId = 0,
                    std::uint64_t pointer = 0)
{
    return sampleOf(input::Phase::Move, device, x, y, deviceId, pointer);
}

input::Sample released(DeviceKind device, double x, double y, std::uint64_t deviceId = 0,
                       std::uint64_t pointer = 0)
{
    return sampleOf(input::Phase::Release, device, x, y, deviceId, pointer);
}

std::unique_ptr<input::TwoUserPolicy> twoUsers()
{
    return std::make_unique<input::TwoUserPolicy>(kLeftHalf);
}

} // namespace

class TstInputTwousers : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() { QVERIFY(m_dir.isValid()); }
    void cleanupTestCase() {}

    // Souris → utilisateur 0, même dans la moitié de l'utilisateur 1 ; ce n'est pas un stylet.
    void mouseIsFirstUser()
    {
        const input::TwoUserPolicy policy(kLeftHalf);
        const RouterState state;
        for (const QPointF p : {QPointF(100, 100), QPointF(900, 100)}) {
            const input::Decision d = policy.decide(press(DeviceKind::Mouse, p.x(), p.y()), state);
            QCOMPARE(d.route, Route::Tool);
            QCOMPARE(d.user, UserId{0});
            QVERIFY(!d.endToolContacts);
        }
        QCOMPARE(policy.firstStylusDevice(), std::uint64_t{0});
    }

    // Premier appareil de stylet vu → 0, un second distinct → 1, le premier revu → 0 ; la moitié ne
    // compte pas pour un stylet.
    void secondStylusIsSecondUser()
    {
        const input::TwoUserPolicy policy(kLeftHalf);
        QCOMPARE(policy.firstStylusDevice(), std::uint64_t{0});
        RouterState state;
        state.toolTouches =
            1; // un doigt de l'utilisateur 1 écrit : un stylet de l'utilisateur 0 ne le coupe pas
        state.toolTouchesByUser[1] = 1;

        input::Decision d = policy.decide(press(DeviceKind::Stylus, 100, 100, 11), state);
        QCOMPARE(d.route, Route::Tool);
        QCOMPARE(d.user, UserId{0});
        QVERIFY(!d.endToolContacts);
        QCOMPARE(policy.firstStylusDevice(), std::uint64_t{11});

        d = policy.decide(press(DeviceKind::Stylus, 900, 100, 22), state);
        QCOMPARE(d.route, Route::Tool);
        QCOMPARE(d.user, UserId{1});
        QVERIFY(!d.endToolContacts);

        d = policy.decide(press(DeviceKind::Stylus, 900, 100, 11), state);
        QCOMPARE(d.route, Route::Tool);
        QCOMPARE(d.user, UserId{0});
        QCOMPARE(policy.firstStylusDevice(), std::uint64_t{11});

        // Une politique neuve n'a vu aucun stylet : le premier qu'elle voit est l'utilisateur 0.
        const input::TwoUserPolicy fresh(kLeftHalf);
        QCOMPARE(fresh.firstStylusDevice(), std::uint64_t{0});
        QCOMPARE(fresh.decide(press(DeviceKind::Stylus, 900, 100, 22), RouterState{}).user, UserId{0});
        QCOMPARE(fresh.firstStylusDevice(), std::uint64_t{22});

        // Par le routeur réel : deux stylets posés ensemble sont deux contacts, chacun à son utilisateur.
        input::PointerRouter router;
        router.setPolicy(twoUsers());
        QCOMPARE(router.route(press(DeviceKind::Stylus, 900, 100, 11)).user, UserId{0});
        QCOMPARE(router.route(press(DeviceKind::Stylus, 100, 100, 22)).user, UserId{1});
        QCOMPARE(router.route(moved(DeviceKind::Stylus, 905, 105, 11)).user, UserId{0});
        QCOMPARE(router.route(moved(DeviceKind::Stylus, 105, 105, 22)).user, UserId{1});
        QCOMPARE(router.route(released(DeviceKind::Stylus, 910, 110, 11)).user, UserId{0});
        const input::Decision still = router.route(moved(DeviceKind::Stylus, 110, 110, 22));
        QCOMPARE(still.route, Route::Tool); // le relâché du premier ne termine pas le second
        QCOMPARE(still.user, UserId{1});

        // Mode à un utilisateur inchangé : le stylet compte pour la paume ; release(0) de la vue l'oublie.
        input::PointerRouter single;
        QCOMPARE(single.route(press(DeviceKind::Stylus, 900, 100, 11)).route, Route::Tool);
        QCOMPARE(single.route(press(DeviceKind::Touch, 300, 300, 0, 1)).route, Route::Ignore);
        QCOMPARE(single.route(released(DeviceKind::Stylus, 900, 100, 11)).route, Route::Tool);
        QCOMPARE(single.route(press(DeviceKind::Touch, 300, 300, 0, 2)).route, Route::Tool);
        single.reset();
        QCOMPARE(single.route(press(DeviceKind::Stylus, 900, 100, 11)).route, Route::Tool);
        single.release(0); // contact du stylet ou de la souris non pris par la vue
        QCOMPARE(single.route(moved(DeviceKind::Stylus, 905, 105, 11)).route, Route::Ignore);
        QCOMPARE(single.route(press(DeviceKind::Touch, 300, 300, 0, 3)).route, Route::Tool);
    }

    // Doigt → 1 dans la moitié de l'utilisateur 1, sinon 0 ; la moitié suit setSecondUserArea.
    void fingerByHalf()
    {
        input::TwoUserPolicy policy(kLeftHalf);
        const RouterState state;
        input::Decision d = policy.decide(press(DeviceKind::Touch, 100, 100, 0, 1), state);
        QCOMPARE(d.route, Route::Tool);
        QCOMPARE(d.user, UserId{1});
        QCOMPARE(d.fingerTool, input::FingerTool::Current);
        d = policy.decide(press(DeviceKind::Touch, 900, 100, 0, 2), state);
        QCOMPARE(d.route, Route::Tool);
        QCOMPARE(d.user, UserId{0});

        policy.setSecondUserArea(QRectF(640, 0, 640, 800));
        QCOMPARE(policy.decide(press(DeviceKind::Touch, 100, 100, 0, 1), state).user, UserId{0});
        QCOMPARE(policy.decide(press(DeviceKind::Touch, 900, 100, 0, 2), state).user, UserId{1});
    }

    // Gestes suspendus : le deuxième doigt d'un utilisateur ne fait rien (jamais Gesture) et ne coupe pas
    // le premier ; deux doigts posés ensemble ne font pas de geste ; le doigt de l'autre utilisateur compte
    // à part.
    void noGestureInDualMode()
    {
        const input::TwoUserPolicy policy(kLeftHalf);
        RouterState state;
        input::Decision d = policy.decide(press(DeviceKind::Touch, 900, 100, 0, 1), state);
        QCOMPARE(d.route, Route::Tool);
        QCOMPARE(d.user, UserId{0});
        QVERIFY(!d.endToolContacts);

        state.toolTouches = 1; // le premier doigt de l'utilisateur 0 écrit
        state.toolTouchesByUser[0] = 1;
        d = policy.decide(press(DeviceKind::Touch, 1000, 200, 0, 2), state);
        QCOMPARE(d.route, Route::Ignore);
        QCOMPARE(d.user, UserId{0});
        QVERIFY(!d.endToolContacts);

        // L'utilisateur 1 pose son premier doigt pendant celui de l'utilisateur 0 : il écrit.
        d = policy.decide(press(DeviceKind::Touch, 100, 100, 0, 3), state);
        QCOMPARE(d.route, Route::Tool);
        QCOMPARE(d.user, UserId{1});
        QVERIFY(!d.endToolContacts);
        state.toolTouches = 2;
        state.toolTouchesByUser[1] = 1;
        d = policy.decide(press(DeviceKind::Touch, 200, 200, 0, 4), state);
        QCOMPARE(d.route, Route::Ignore);
        QCOMPARE(d.user, UserId{1});

        // Deux doigts posés dans le même événement : le premier écrit, aucun geste.
        RouterState together;
        together.pressesInEvent = 2;
        d = policy.decide(press(DeviceKind::Touch, 900, 100, 0, 5), together);
        QCOMPARE(d.route, Route::Tool);
        QVERIFY(!d.endToolContacts);
        together.toolTouches = 1;
        together.toolTouchesByUser[0] = 1;
        d = policy.decide(press(DeviceKind::Touch, 1000, 100, 0, 6), together);
        QCOMPARE(d.route, Route::Ignore);
        QVERIFY(!d.endToolContacts);
    }

    // Paume : un doigt pendant un contact de stylet ne fait rien pour le même utilisateur seulement.
    void palmOfOneUserOnly()
    {
        const input::TwoUserPolicy policy(kLeftHalf);
        RouterState state;
        state.stylusDown = true; // le stylet de l'utilisateur 0 est posé
        state.stylusDownByUser[0] = true;
        input::Decision d = policy.decide(press(DeviceKind::Touch, 100, 100, 0, 1), state);
        QCOMPARE(d.route, Route::Tool);
        QCOMPARE(d.user, UserId{1});
        d = policy.decide(press(DeviceKind::Touch, 900, 100, 0, 2), state);
        QCOMPARE(d.route, Route::Ignore);
        QCOMPARE(d.user, UserId{0});

        RouterState other;
        other.stylusDown = true; // le stylet de l'utilisateur 1 est posé
        other.stylusDownByUser[1] = true;
        d = policy.decide(press(DeviceKind::Touch, 900, 100, 0, 1), other);
        QCOMPARE(d.route, Route::Tool);
        QCOMPARE(d.user, UserId{0});
        d = policy.decide(press(DeviceKind::Touch, 100, 100, 0, 2), other);
        QCOMPARE(d.route, Route::Ignore);
        QCOMPARE(d.user, UserId{1});

        // Par le routeur réel : la paume de l'utilisateur 1 est ignorée pendant que son stylet écrit ;
        // l'utilisateur 0 garde son doigt.
        input::PointerRouter router;
        router.setPolicy(twoUsers());
        router.route(press(DeviceKind::Stylus, 900, 100, 11)); // l'appareil 11 est l'utilisateur 0
        router.route(released(DeviceKind::Stylus, 900, 100, 11));
        QCOMPARE(router.route(press(DeviceKind::Touch, 900, 300, 0, 1)).user, UserId{0});
        const input::Decision pen = router.route(press(DeviceKind::Stylus, 100, 100, 22));
        QCOMPARE(pen.route, Route::Tool);
        QCOMPARE(pen.user, UserId{1});
        const input::Decision palm = router.route(press(DeviceKind::Touch, 120, 300, 0, 2));
        QCOMPARE(palm.route, Route::Ignore);
        QCOMPARE(palm.user, UserId{1});
        const input::Decision kept = router.route(moved(DeviceKind::Touch, 905, 305, 0, 1));
        QCOMPARE(kept.route, Route::Tool);
        QCOMPARE(kept.user, UserId{0});
    }

    // Deux UserState : chacun son outil courant et ses réglages ; un utilisateur neuf commence au stylo.
    void eachUserKeepsItsTool()
    {
        ui::ToolHost host;
        host.setSettingsFile(m_dir.filePath(QStringLiteral("each.ini")));
        QVERIFY(!host.dualUsers());
        host.setDualUsers(true);
        QVERIFY(host.dualUsers());

        tools::UserState& second = host.user(1);
        QCOMPARE(second.id(), UserId{1});
        QCOMPARE(second.currentTool(), QStringLiteral("pen"));
        tools::UserState& first = host.user(0);
        QCOMPARE(first.id(), UserId{0});
        QVERIFY(&first != &second);

        const core::Argb red = 0xFFFF0000u;
        first.setCurrentTool(QStringLiteral("pen"));
        tools::InkSettings ink = first.settings(QStringLiteral("pen"));
        ink.color = red;
        first.setSettings(QStringLiteral("pen"), ink);
        second.setCurrentTool(QStringLiteral("eraser"));

        QCOMPARE(host.user(0).currentTool(), QStringLiteral("pen"));
        QCOMPARE(host.user(1).currentTool(), QStringLiteral("eraser"));
        QCOMPARE(host.user(0).settings(QStringLiteral("pen")).color, red);
        QCOMPARE(host.user(1).settings(QStringLiteral("pen")).color,
                 tools::defaultSettings(QStringLiteral("pen")).color);
        QCOMPARE(host.currentTool(),
                 QStringLiteral("pen")); // la propriété de la palette suit l'utilisateur 0
    }

    // Mode éteint : user(1) est l'utilisateur 0, comme le socle ; rallumé, le second état revient.
    void singleUserWhenOff()
    {
        ui::ToolHost host;
        host.setSettingsFile(m_dir.filePath(QStringLiteral("single.ini")));
        QCOMPARE(&host.user(1), &host.user(0));
        QCOMPARE(host.user(1).id(), UserId{0});

        host.setDualUsers(true);
        tools::UserState* second = &host.user(1);
        QVERIFY(second != &host.user(0));
        second->setCurrentTool(QStringLiteral("eraser"));

        host.setDualUsers(false);
        QCOMPARE(&host.user(1), &host.user(0));
        QCOMPARE(host.user(1).currentTool(), QStringLiteral("pen"));

        host.setDualUsers(true);
        QCOMPARE(&host.user(1), second);
        QCOMPARE(host.user(1).currentTool(), QStringLiteral("eraser"));
    }

private:
    QTemporaryDir m_dir;
};

QTEST_MAIN(TstInputTwousers)
#include "tst_input_twousers.moc"
