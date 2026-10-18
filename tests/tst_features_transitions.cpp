#include <actcraft/features/transitions.h>

#include <QRandomGenerator>
#include <QSet>
#include <QtTest>

using namespace actcraft::features;

class TstFeaturesTransitions : public QObject
{
    Q_OBJECT

private slots:
    void catalogueStartsWithNone()
    {
        const auto& list = transitionCatalogue();
        QVERIFY(list.size() >= 3);
        QCOMPARE(list.front().id, QStringLiteral("none"));
        for (const auto& e : list)
            QVERIFY(!e.label.isEmpty());
    }
    void catalogueHasSixteenEffectsPlusTwo()
    {
        // L7-T0 (docs/verifications/2026-10-18-lot7-principes.md) : 16 mouvements retenus (17 relevés,
        // l'effet à nom de marque écarté), plus « aucun » et « au hasard ».
        const auto& list = transitionCatalogue();
        QCOMPARE(list.size(), std::size_t{18});
        QCOMPARE(list[1].id, QStringLiteral("random"));
        QSet<QString> ids;
        for (const auto& e : list)
            ids.insert(e.id);
        QCOMPARE(ids.size(), qsizetype{18}); // identifiants uniques
    }
    void pageEffectWinsOverGlobal()
    {
        TransitionSettings s;
        s.global = QStringLiteral("dissolve");
        QCOMPARE(effectForLeaving(s, std::string("slide-right"), NavigationKind::Next, {}),
                 QStringLiteral("slide-right"));
        QCOMPARE(effectForLeaving(s, std::string(), NavigationKind::Next, {}), QStringLiteral("dissolve"));
    }
    void effectPlayedWhenLeavingThePage()
    {
        TransitionSettings s; // global « none »
        // l'effet de la page QUITTÉE compte, pas celui de la page d'arrivée
        QCOMPARE(effectForLeaving(s, std::string("dissolve"), NavigationKind::Previous, {}),
                 QStringLiteral("dissolve"));
    }
    void disabledWhileCurtainActive()
    {
        TransitionSettings s;
        s.global = QStringLiteral("dissolve");
        QCOMPARE(effectForLeaving(s, {}, NavigationKind::Next, {true, false, false}), QStringLiteral("none"));
        QCOMPARE(effectForLeaving(s, {}, NavigationKind::Jump, {}), QStringLiteral("none"));
    }
    void unknownEffectFallsBackToNone()
    {
        TransitionSettings s;
        s.global = QStringLiteral("does-not-exist");
        QCOMPARE(effectForLeaving(s, {}, NavigationKind::Next, {}), QStringLiteral("none"));
    }
    void durationClampedTo5To30()
    {
        QCOMPARE(clampDurationTenths(2), 5);
        QCOMPARE(clampDurationTenths(15), 15);
        QCOMPARE(clampDurationTenths(99), 30);
    }
    void randomPicksARealEffect()
    {
        QRandomGenerator generator(2026);
        for (int i = 0; i < 50; ++i) {
            const QString id = pickEffect(QStringLiteral("random"), generator);
            QVERIFY(id != QStringLiteral("random"));
            QVERIFY(id != QStringLiteral("none"));
            QVERIFY(id != QStringLiteral("does-not-exist"));
        }
        QCOMPARE(pickEffect(QStringLiteral("dissolve"), generator),
                 QStringLiteral("dissolve")); // pas de tirage
    }
    void desktopAnnotationAndDesignModeBlock()
    {
        TransitionSettings s;
        s.global = QStringLiteral("dissolve");
        QCOMPARE(effectForLeaving(s, {}, NavigationKind::Next, {false, true, false}), QStringLiteral("none"));
        QCOMPARE(effectForLeaving(s, {}, NavigationKind::Next, {false, false, true}), QStringLiteral("none"));
    }
    void randomIsAKnownIdentifier()
    {
        TransitionSettings s;
        s.global = QStringLiteral("random");
        QCOMPARE(effectForLeaving(s, {}, NavigationKind::Next, {}), QStringLiteral("random"));
    }
};

QTEST_GUILESS_MAIN(TstFeaturesTransitions)
#include "tst_features_transitions.moc"
