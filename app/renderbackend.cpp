#include "renderbackend.h"

#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSurfaceFormat>

bool parseRenderBackend(const QStringList& args, RenderBackend& out, QString& error)
{
#ifdef Q_OS_WIN
    out = RenderBackend::Direct3D11;
#else
    out = RenderBackend::Auto;
#endif
    const qsizetype index = args.indexOf(QStringLiteral("--rendu"));
    if (index < 0) {
        return true;
    }
    if (index + 1 >= args.size()) {
        error = QStringLiteral("--rendu attend une valeur : auto, d3d11 ou logiciel");
        return false;
    }
    const QString value = args.at(index + 1);
    if (value == QLatin1String("auto")) {
        out = RenderBackend::Auto;
    } else if (value == QLatin1String("d3d11")) {
        out = RenderBackend::Direct3D11;
    } else if (value == QLatin1String("logiciel")) {
        out = RenderBackend::Software;
    } else {
        error = QStringLiteral("valeur de --rendu inconnue : %1").arg(value);
        return false;
    }
    return true;
}

void applyRenderBackend(RenderBackend backend)
{
    if (backend != RenderBackend::Software) {
        // Traits lissés en RHI comme en rendu logiciel (L10-T5) : la fenêtre (encre, couche d'attente) est
        // multi-échantillonnée ; la texture du cache l'est par ShaderEffectSource.samples (PageArea.qml).
        QSurfaceFormat format = QSurfaceFormat::defaultFormat();
        format.setSamples(kMsaaSamples);
        QSurfaceFormat::setDefaultFormat(format);
    }
    switch (backend) {
    case RenderBackend::Auto:
        break;
    case RenderBackend::Direct3D11:
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
        break;
    case RenderBackend::Software:
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
        break;
    }
}
