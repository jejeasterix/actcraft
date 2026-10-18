#include <actcraft/ui/viewtransform.h>

#include <QTest>

#include <cmath>

using namespace actcraft::ui;

class TstViewTransform : public QObject
{
    Q_OBJECT

private slots:
    void zoomIsClamped()
    {
        QCOMPARE(clampZoom(0.1), 0.25);
        QCOMPARE(clampZoom(20.0), 8.0);
        QCOMPARE(clampZoom(1.5), 1.5);
    }

    void wheelFactorFollowsDelta()
    {
        QVERIFY(qFuzzyCompare(wheelZoomFactor(120), 1.15));
        QVERIFY(qFuzzyCompare(wheelZoomFactor(-120), 1.0 / 1.15));
        QVERIFY(qFuzzyCompare(wheelZoomFactor(60), std::sqrt(1.15)));
    }

    void zoomKeepsAnchor()
    {
        ViewTransform t;
        t.zoom = 2.0;
        t.pan = QPointF(-100, -50);
        const QPointF anchor(300, 200);
        const QPointF before = t.viewToPage(anchor);
        const ViewTransform z = zoomAt(t, anchor, 3.0);
        QCOMPARE(z.zoom, 3.0);
        const QPointF after = z.viewToPage(anchor);
        QVERIFY(qAbs(after.x() - before.x()) < 1e-9 && qAbs(after.y() - before.y()) < 1e-9);
    }

    void fitShowsWholePageCentered()
    {
        const ViewTransform t = fitPage(QSizeF(1280, 800), QSizeF(1024, 768));
        QVERIFY(qFuzzyCompare(t.zoom, (800.0 - 64.0) / 768.0));
        QVERIFY(qFuzzyCompare(t.pan.x(), (1280.0 - 1024.0 * t.zoom) / 2));
        QVERIFY(qFuzzyCompare(t.pan.y(), 32.0));
    }

    void clampCentersSmallPageAndBoundsLargePage()
    {
        ViewTransform small;
        small.zoom = 0.5;
        small.pan = QPointF(-999, 999);
        const ViewTransform s = clampPan(small, QSizeF(1280, 800), QSizeF(1024, 768));
        QCOMPARE(s.pan, QPointF((1280 - 512) / 2.0, (800 - 384) / 2.0));

        ViewTransform big;
        big.zoom = 4.0;
        big.pan = QPointF(500, -99999);
        const ViewTransform b = clampPan(big, QSizeF(1280, 800), QSizeF(1024, 768));
        QCOMPARE(b.pan.x(), 32.0);
        QCOMPARE(b.pan.y(), 800.0 - 3072.0 - 32.0);
    }

    void wheelScrollRules()
    {
        QCOMPARE(wheelScrollDelta(QPoint(0, 120), QPoint(), Qt::NoModifier), QPointF(0, 60));
        QCOMPARE(wheelScrollDelta(QPoint(0, -120), QPoint(), Qt::ShiftModifier), QPointF(-60, 0));
        QCOMPARE(wheelScrollDelta(QPoint(120, 0), QPoint(), Qt::ShiftModifier), QPointF(60, 0));
        QCOMPARE(wheelScrollDelta(QPoint(0, 120), QPoint(0, 7), Qt::NoModifier), QPointF(0, 7));
    }

    void cacheTextureSizeIsCapped()
    {
        QCOMPARE(cacheTextureSize(QSizeF(1024, 768), 1.0, 1.0), QSize(1024, 768));
        QCOMPARE(cacheTextureSize(QSizeF(1024, 768), 4.0, 1.0), QSize(4096, 3072));
        QCOMPARE(cacheTextureSize(QSizeF(1024, 768), 8.0, 1.0), QSize(4096, 3072));
        QCOMPARE(cacheTextureSize(QSizeF(1024, 768), 0.25, 1.0), QSize(256, 192));
        QCOMPARE(cacheTextureSize(QSizeF(1024, 768), 1.0, 2.0), QSize(2048, 1536));
        QCOMPARE(cacheTextureSize(QSizeF(1024, 768), 3.0, 2.0), QSize(4096, 3072));
    }

    void multisampledCacheCapKeepsHundredPercentAndFitSharp()
    {
        QCOMPARE(cacheMaxSide(QSizeF(1024, 768), 1.0, 1), kMaxCacheSide);
        QCOMPARE(cacheMaxSide(QSizeF(1024, 768), 1.0, 4), kMaxMsaaCacheSide);
        QCOMPARE(cacheTextureSize(QSizeF(1024, 768), 8.0, 1.0, cacheMaxSide(QSizeF(1024, 768), 1.0, 4)),
                 QSize(3072, 2304));
        // Page de 1920 à DPR 2 : jamais sous le côté à 100 %.
        const int big = cacheMaxSide(QSizeF(1920, 1080), 2.0, 4);
        QCOMPARE(cacheTextureSize(QSizeF(1920, 1080), 1.0, 2.0, big), QSize(3840, 2160));
        QCOMPARE(cacheTextureSize(QSizeF(1920, 1080), 8.0, 2.0, big), QSize(3840, 2160));
        // « Ajuster » au-delà de 100 % (page de 1024 à DPR 2, zoom 1,25) : nette aussi.
        const int fit = cacheMaxSide(QSizeF(1024, 768), 2.0, 4, 1.25);
        QCOMPARE(cacheTextureSize(QSizeF(1024, 768), 1.25, 2.0, fit), QSize(2560, 1920));
        QCOMPARE(cacheMaxSide(QSizeF(8000, 6000), 1.0, 4), kMaxCacheSide); // jamais au-dessus de N1-6
    }

    void scrollAxisCoversPageAndMargins()
    {
        const ScrollAxis fits = scrollAxis(10, 1000, 500);
        QCOMPARE(fits.size, 1.0);
        QCOMPARE(fits.position, 0.0);
        const double extent = 2000 + 2 * kViewMargin;
        const ScrollAxis start = scrollAxis(kViewMargin, 1000, 2000);
        QCOMPARE(start.size, 1000 / extent);
        QCOMPARE(start.position, 0.0);
        const ScrollAxis end = scrollAxis(1000 - 2000 - kViewMargin, 1000, 2000);
        QVERIFY(qAbs(end.position - (1.0 - 1000 / extent)) < 1e-12);
    }

    void panForScrollPositionInvertsScrollAxis()
    {
        for (const double position : {0.0, 0.2, 0.4}) {
            const double pan = panForScrollPosition(position, 1000, 2000);
            QVERIFY(qAbs(scrollAxis(pan, 1000, 2000).position - position) < 1e-12);
        }
        QCOMPARE(panForScrollPosition(0.5, 1000, 500), 250.0); // page plus petite que la vue : centrée
        QCOMPARE(panForScrollPosition(-1.0, 1000, 2000), kViewMargin); // borné au début
    }
};

QTEST_GUILESS_MAIN(TstViewTransform)
#include "tst_viewtransform.moc"
