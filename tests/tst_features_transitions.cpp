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
        const QStringList expected{
            QStringLiteral("none"),        QStringLiteral("random"),       QStringLiteral("dissolve"),
            QStringLiteral("corner-peel"), QStringLiteral("box-turn"),     QStringLiteral("card-flip"),
            QStringLiteral("tiles-spin"),  QStringLiteral("fold-half"),    QStringLiteral("slide-right"),
            QStringLiteral("strips-drop"), QStringLiteral("corner-swing"), QStringLiteral("spiral-out"),
            QStringLiteral("turn-reveal"), QStringLiteral("drapes"),       QStringLiteral("star-iris"),
            QStringLiteral("shards"),      QStringLiteral("sweep"),        QStringLiteral("funnel")};
        QStringList actual; // liste exacte du rapport L7-T0, dans l'ordre
        for (const auto& e : list)
            actual.append(e.id);
        QCOMPARE(actual, expected);
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
        QCOMPARE(effectForLeaving(s, {}, NavigationKind::Next, {.curtainOrSpotlight = true}),
                 QStringLiteral("none"));
    }
    // Saut direct (navigateur de pages, S22) : aucun effet, sans aucun bloqueur.
    void jumpPlaysNoEffect()
    {
        TransitionSettings s;
        s.global = QStringLiteral("dissolve");
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
        QSet<QString> catalogueIds;
        for (const auto& e : transitionCatalogue())
            catalogueIds.insert(e.id);
        QSet<QString> seen;
        for (quint32 seed = 1; seed <= 50; ++seed) {
            QRandomGenerator generator(seed);
            const QString id = pickEffect(QStringLiteral("random"), generator);
            QVERIFY(catalogueIds.contains(id));
            QVERIFY(id != QStringLiteral("random"));
            QVERIFY(id != QStringLiteral("none"));
            seen.insert(id);
        }
        QVERIFY(seen.size() >= 2); // le tirage varie
        QRandomGenerator generator(2026);
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
