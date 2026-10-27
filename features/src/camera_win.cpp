#include <actcraft/features/camera.h>

#include <QGuiApplication>
#include <QList>
#include <QPoint>
#include <QScreen>
#include <QSizeF>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <dwmapi.h>

namespace actcraft::features {

namespace {

// Fenêtre masquée par le compositeur (application d'un autre bureau virtuel, fenêtre suspendue) : invisible
// bien que IsWindowVisible le dise.
bool isCloaked(HWND window)
{
    DWORD cloaked = 0;
    return SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked != 0;
}

// Cadre visible en pixels physiques : sans les bordures invisibles de redimensionnement (Windows 10 et 11).
bool visibleFrame(HWND window, RECT& frame)
{
    if (SUCCEEDED(DwmGetWindowAttribute(window, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(frame)))) {
        return true;
    }
    return GetWindowRect(window, &frame) != 0;
}

// Pixels physiques vers pixels logiques de Qt : l'écran qui porte le centre garde son origine, le reste est
// divisé par son rapport (règle de Qt pour les écrans à haute densité) ; hors de tout écran connu, rapport 1.
QRect toQtPixels(const QRect& physical)
{
    const QPoint center = physical.center();
    const QList<QScreen*> screens = QGuiApplication::screens();
    for (const QScreen* screen : screens) {
        const QRect logical = screen->geometry();
        const qreal ratio = screen->devicePixelRatio();
        const QRect native(logical.topLeft(), (QSizeF(logical.size()) * ratio).toSize());
        if (native.contains(center)) {
            const QPoint origin = logical.topLeft();
            return toLogicalPixels(physical.translated(-origin), ratio).translated(origin).toAlignedRect();
        }
    }
    return physical;
}

BOOL CALLBACK collectWindow(HWND window, LPARAM data)
{
    auto* windows = reinterpret_cast<std::vector<QRect>*>(data);
    if (!IsWindowVisible(window) || IsIconic(window) || isCloaked(window)) {
        return TRUE;
    }
    RECT frame{};
    if (!visibleFrame(window, frame) || frame.right <= frame.left || frame.bottom <= frame.top) {
        return TRUE;
    }
    const QRect physical(QPoint(frame.left, frame.top), QPoint(frame.right - 1, frame.bottom - 1));
    windows->push_back(toQtPixels(physical));
    return TRUE;
}

} // namespace

std::vector<QRect> topLevelWindows()
{
    // Toutes les mesures en pixels physiques, quel que soit le réglage du processus : une application Qt est
    // déjà « par écran (v2) », un processus sans réglage (tests hors écran) recevrait des coordonnées
    // virtualisées de GetWindowRect mais physiques de DwmGetWindowAttribute.
    const DPI_AWARENESS_CONTEXT previous =
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    std::vector<QRect> windows;
    EnumWindows(collectWindow, reinterpret_cast<LPARAM>(&windows));
    if (previous) {
        SetThreadDpiAwarenessContext(previous);
    }
    return windows;
}

} // namespace actcraft::features
