#include <actcraft/features/desktopwheel.h>

#include <QtTest>

#include <cmath>
#include <functional>

using namespace actcraft::features;

class TstFeaturesDesktopWheel : public QObject
{
    Q_OBJECT

private slots:
    void wheelIsClockwiseFromTop()
    {
        const auto p = wheelPositions(4, 100);
        QCOMPARE(p.size(), std::size_t{4});
        QVERIFY(std::abs(p[0].x() - 0) < 0.01 && std::abs(p[0].y() + 100) < 0.01); // en haut
        QVERIFY(std::abs(p[1].x() - 100) < 0.01 && std::abs(p[1].y() - 0) < 0.01); // puis à droite (y vers le bas)
    }

    void defaultWheelUsesDesktopAndToolCommands()
    {
        const auto wheel = defaultWheel();
        QVERIFY(!wheel.empty());
        QCOMPARE(wheel.front().command, QStringLiteral("desktop.restore"));
        std::function<void(const std::vector<WheelEntry>&)> check = [&](const std::vector<WheelEntry>& ring) {
            for (const auto& e : ring) {
                QVERIFY(!e.label.isEmpty());
                if (e.ring.empty())
                    QVERIFY(e.command.startsWith(QStringLiteral("desktop.")) || e.command.startsWith(QStringLiteral("tool.")));
                else {
                    QVERIFY(e.command.isEmpty()); // un anneau secondaire n'est pas une commande
                    check(e.ring);
                }
            }
        };
        check(wheel);
    }
};

QTEST_GUILESS_MAIN(TstFeaturesDesktopWheel)
#include "tst_features_desktopwheel.moc"
