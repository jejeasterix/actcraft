#include "renderbackend.h"

#include <QSurfaceFormat>
#include <QTest>

class TstRenderBackend : public QObject
{
    Q_OBJECT

private slots:
    void defaultFollowsPlatform()
    {
        RenderBackend backend = RenderBackend::Software;
        QString error;
        QVERIFY(parseRenderBackend({QStringLiteral("actcraft")}, backend, error));
#ifdef Q_OS_WIN
        QCOMPARE(backend, RenderBackend::Direct3D11);
#else
        QCOMPARE(backend, RenderBackend::Auto);
#endif
    }

    void softwareIsSelectable()
    {
        RenderBackend backend = RenderBackend::Auto;
        QString error;
        QVERIFY(parseRenderBackend(
            {QStringLiteral("actcraft"), QStringLiteral("--rendu"), QStringLiteral("logiciel")}, backend,
            error));
        QCOMPARE(backend, RenderBackend::Software);
    }

    void unknownValueIsRejected()
    {
        RenderBackend backend = RenderBackend::Auto;
        QString error;
        QVERIFY(!parseRenderBackend(
            {QStringLiteral("actcraft"), QStringLiteral("--rendu"), QStringLiteral("vulkan")}, backend,
            error));
        QVERIFY(!error.isEmpty());
    }

    void missingValueIsRejected()
    {
        RenderBackend backend = RenderBackend::Auto;
        QString error;
        QVERIFY(!parseRenderBackend({QStringLiteral("actcraft"), QStringLiteral("--rendu")}, backend, error));
        QVERIFY(!error.isEmpty());
    }

    // L10-T5 (S33) : en RHI, la fenêtre (encre, couche d'attente) est multi-échantillonnée. Le cache seul ne
    // suffirait pas : ce test est rouge si seule la texture du cache demande l'échantillonnage multiple.
    void rhiRequestsMultisampling()
    {
        QSurfaceFormat::setDefaultFormat(QSurfaceFormat());
        applyRenderBackend(RenderBackend::Direct3D11);
        QCOMPARE(QSurfaceFormat::defaultFormat().samples(), kMsaaSamples);
        QCOMPARE(kMsaaSamples, 4);
    }

    void softwareKeepsTheDefaultFormat()
    {
        QSurfaceFormat::setDefaultFormat(QSurfaceFormat());
        applyRenderBackend(RenderBackend::Software);
        QVERIFY(QSurfaceFormat::defaultFormat().samples() <= 1); // le peintre lisse déjà
    }
};

QTEST_MAIN(TstRenderBackend)
#include "tst_renderbackend.moc"
