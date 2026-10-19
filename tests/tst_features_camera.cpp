#include <actcraft/features/camera.h>

#include <QColor>
#include <QImage>
#include <QtTest>

using namespace actcraft::features;

class TstFeaturesCamera : public QObject
{
    Q_OBJECT

private slots:
    void clickNearFirstPointClosesThePolygon()
    {
        PolygonPicker p;
        p.click(QPointF(10, 10));
        p.click(QPointF(100, 10));
        p.click(QPointF(100, 100));
        QVERIFY(!p.closed());
        p.click(QPointF(14, 12)); // à moins de kClosePixels du premier
        QVERIFY(p.closed());
        QCOMPARE(p.polygon().size(), qsizetype{3});
    }

    void freehandClosesOnRelease()
    {
        FreehandPicker f;
        f.press(QPointF(0, 0));
        f.move(QPointF(50, 0));
        f.move(QPointF(50, 50));
        f.release();
        QVERIFY(f.closed());
        QCOMPARE(f.polygon().first(), f.polygon().last());
    }

    void cropOutsideIsTransparent()
    {
        QImage screen(100, 100, QImage::Format_ARGB32_Premultiplied);
        screen.fill(Qt::red);
        const QImage out = cropToPolygon(screen, QPolygonF({{10, 10}, {90, 10}, {50, 90}}));
        QCOMPARE(out.size(), QSize(80, 80));
        QCOMPARE(qAlpha(out.pixel(1, 79)), 0); // coin bas gauche : hors du triangle
        QCOMPARE(QColor(out.pixel(40, 20)), QColor(Qt::red));
    }

    void rectangleNormalized()
    {
        QCOMPARE(normalizedRect(QPointF(50, 50), QPointF(10, 20)), QRectF(10, 20, 40, 30));
    }

    void windowRectsInLogicalPixels()
    {
        QCOMPARE(toLogicalPixels(QRect(300, 150, 900, 600), 1.5), QRectF(200, 100, 600, 400));
        QCOMPARE(toLogicalPixels(QRect(10, 20, 30, 40), 0), QRectF(10, 20, 30, 40)); // rapport invalide : 1
    }
};

QTEST_MAIN(TstFeaturesCamera)
#include "tst_features_camera.moc"
