#include <actcraft/bench/contentcheck.h>
#include <actcraft/bench/runner.h>

#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>

using namespace actcraft::bench;

class TstBenchContent : public QObject
{
    Q_OBJECT

private slots:
    void captureSavingReportsEveryFailure()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QImage image(4, 4, QImage::Format_ARGB32);
        image.fill(Qt::red);
        const QList<QImage> images{image, image, image, image};

        QStringList errors;
        saveCaptures(dir.filePath(QStringLiteral("ok")), QStringLiteral("d3d11"), images, errors);
        QVERIFY2(errors.isEmpty(), qPrintable(errors.join('|')));
        QCOMPARE(QDir(dir.filePath(QStringLiteral("ok"))).entryList(QDir::Files).size(), 4);

        // Un fichier à la place du dossier : mkpath et save échouent, chacun est remonté.
        QFile blocker(dir.filePath(QStringLiteral("bloque")));
        QVERIFY(blocker.open(QIODevice::WriteOnly));
        blocker.close();
        errors.clear();
        saveCaptures(blocker.fileName(), QStringLiteral("d3d11"), images, errors);
        QVERIFY(!errors.isEmpty());
        for (const QString& e : errors) {
            QVERIFY2(e.startsWith(QLatin1String("error: capture")), qPrintable(e));
        }
    }

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
