#include <actcraft/bench/contentcheck.h>
#include <actcraft/bench/runner.h>

#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>

#include <cmath>

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
        // Le pixel i couvre [i, i + 1[ : 20,6 tombe dans le pixel 20 (son centre est à 20,5), pas dans le 21.
        QCOMPARE(sceneToImage(QPointF(10.4, 20.6), 1.0), QPoint(10, 20));
        QCOMPARE(sceneToImage(QPointF(10, 20), 1.5), QPoint(15, 30));
        QCOMPARE(sceneToImage(QPointF(10.4, 20.6), 1.5), QPoint(15, 30)); // 15,6 ; 30,9
        QCOMPARE(sceneToImage(QPointF(-0.25, 0.25), 1.0), QPoint(-1, 0)); // à gauche de l'image : hors image
    }

    // L2-C11 : un trait sans anticrénelage (D3D11) de 3,6 pixels (4 × zoom 0,901) couvre les lignes dont le
    // centre est dans [c - 1,8 ; c + 1,8]. Lu au point de son axe, il doit sortir noir quelle que soit la
    // position de l'axe dans le pixel : l'échantillon 3 × 3 centré sur le pixel qui contient l'axe reste dans
    // le trait. Centré un demi-pixel trop bas (arrondi), il déborde d'une ligne : #555555 à l'écran.
    void thinAliasedStrokeReadsDarkAtEveryPhase()
    {
        constexpr double kHalf = 1.8;
        for (int step = 0; step < 20; ++step) {
            const double axis = 616.0 + step / 20.0;
            QImage image(40, 640, QImage::Format_RGB32);
            image.fill(Qt::white);
            for (int y = 0; y < image.height(); ++y) {
                if (std::abs(y + 0.5 - axis) <= kHalf) {
                    for (int x = 0; x < image.width(); ++x) {
                        image.setPixelColor(x, y, Qt::black);
                    }
                }
            }
            const QColor onAxis = sampleColor(image, sceneToImage(QPointF(20.3, axis), 1.0));
            QVERIFY2(colorNear(onAxis, Qt::black, 60),
                     qPrintable(QStringLiteral("axe %1 : lu %2").arg(axis).arg(onAxis.name())));
            // Toujours discriminant : à 3 pixels de l'axe, hors du trait, la lecture n'est pas noire.
            const QColor beside = sampleColor(image, sceneToImage(QPointF(20.3, axis + 3.0), 1.0));
            QVERIFY2(!colorNear(beside, Qt::black, 60),
                     qPrintable(QStringLiteral("axe %1 + 3 : lu %2").arg(axis).arg(beside.name())));
        }
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
