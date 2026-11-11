#include <actcraft/ui/webframeengine.h>
#include <actcraft/ui/webframeitem.h> // ACTCRAFT_WEBFRAME

#include <QCoreApplication>
#include <QtGlobal>

#if ACTCRAFT_WEBFRAME && defined(Q_OS_ANDROID)
#include <QtWebView/QtWebView>
#endif

namespace actcraft::ui {

void prepareWebFrameEngine()
{
#if ACTCRAFT_WEBFRAME
#ifdef Q_OS_ANDROID
    QtWebView::initialize();
#else
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    if (qEnvironmentVariableIsEmpty("QTWEBENGINE_CHROMIUM_FLAGS"))
        qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--renderer-process-limit=2");
#endif
#endif
}

} // namespace actcraft::ui
