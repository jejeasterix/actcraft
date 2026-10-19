#pragma once

#include <actcraft/features/pagezoom.h>

#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

class PageView;

// Zoom de page (F-260) : l'état de features::PageZoom offert à QML. Tant que le mode est actif, un appui tenu
// zoome autour du point (gauche : avant ; droit : arrière), glisser fait un panoramique, un double appui
// revient à « Ajuster ». Le document n'est jamais lu ni modifié. Le cache n'est pas redessiné pendant l'appui
// : la vue le fait une fois, après le dernier zoom (PageView::zoomAround).
class PageZoomController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY viewChanged)
    Q_PROPERTY(bool panning READ panning NOTIFY panningChanged)

public:
    explicit PageZoomController(QObject* parent = nullptr);

    bool active() const noexcept;
    void setActive(bool active);
    PageView* view() const noexcept;
    void setView(PageView* view);
    bool panning() const noexcept;

    // Points en coordonnées de scène ; le bouton droit dézoome.
    Q_INVOKABLE void press(QPointF scenePoint, bool right);
    Q_INVOKABLE void move(QPointF scenePoint);
    Q_INVOKABLE void release();
    Q_INVOKABLE void toggle();

signals:
    void activeChanged();
    void viewChanged();
    void panningChanged();

private:
    void tick();

    features::PageZoom m_zoom{1.0};
    QPointer<PageView> m_view;
    QTimer m_timer;
    bool m_active = false;
    bool m_panning = false;
};

} // namespace actcraft::ui
