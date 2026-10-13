#include <actcraft/bench/contentcheck.h>

#include <QImage>
#include <QTest>

using namespace actcraft::bench;

class TstBenchContent : public QObject
{
    Q_OBJECT

private slots:
    void colorNearUsesEveryChannel()
    {
        QVERIFY(colorNear(QColor(100, 100, 100), QColor(110, 90, 100), 10));
        QVERIFY(!colorNear(QColor(100, 100, 100), QColor(100, 100, 111), 10));
    }

    void sampleColorAveragesNine()
    {
        QImage image(10, 10, QImage::Format_ARGB32);
        image.fill(QColor(0, 0, 0));
        image.setPixelColor(5, 5, QColor(90, 90, 90));
        const QColor c = sampleColor(image, QPoint(5, 5));
        QCOMPARE(c.red(), 10);
        QCOMPARE(sampleColor(image, QPoint(0, 0)).red(), 0); // bord : voisins hors image ignorés
    }

    void sceneToImageScalesByRatio()
    {
        QCOMPARE(sceneToImage(QPointF(10.4, 20.6), 1.0), QPoint(10, 21));
        QCOMPARE(sceneToImage(QPointF(10, 20), 1.5), QPoint(15, 30));
    }

    void checkColorReportsExpectedAndActual()
    {
        QImage image(4, 4, QImage::Format_ARGB32);
        image.fill(QColor(0x1E, 0x88, 0xE5));
        const ContentCheck ok = checkColor(QStringLiteral("witness-shape"), image, QPoint(2, 2),
                                           QColor(0x1E, 0x88, 0xE5), kWitnessTolerance);
        QVERIFY(ok.ok);
        QCOMPARE(ok.expected, QStringLiteral("#ff1e88e5"));
        QCOMPARE(ok.actual, QStringLiteral("#ff1e88e5"));
        const ContentCheck ko =
            checkColor(QStringLiteral("ink"), image, QPoint(2, 2), QColor(Qt::black), kInkTolerance);
        QVERIFY(!ko.ok);
        const ContentCheck outside = checkColor(QStringLiteral("x"), image, QPoint(50, 50),
                                                QColor(0x1E, 0x88, 0xE5), kWitnessTolerance);
        QVERIFY(!outside.ok);
    }

    void captureNames()
    {
        QCOMPARE(captureSteps(), QStringList({QStringLiteral("mi-trait"), QStringLiteral("fin-trait"),
                                              QStringLiteral("zoom-800"), QStringLiteral("retour-100")}));
        QCOMPARE(captureFileName(QStringLiteral("direct3d11"), QStringLiteral("mi-trait")),
                 QStringLiteral("direct3d11-mi-trait.png"));
    }
};

QTEST_GUILESS_MAIN(TstBenchContent)
#include "tst_bench_content.moc"
