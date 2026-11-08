#pragma once

#include <actcraft/features/curtain.h>

#include <QColor>
#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Rideau (F-265) : l'état du store (actcraft::features::Curtain) offert à QML. Une instance par fenêtre ;
// aucun lien au document, aucune demande de redessin du cache : la surcouche ne fait que dessiner un
// rectangle, en coordonnées de page.
class CurtainController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QSizeF area READ area WRITE setArea NOTIFY areaChanged)
    Q_PROPERTY(QRectF covered READ covered NOTIFY changed)
    Q_PROPERTY(qreal coverage READ coverage NOTIFY changed) // arrondie à 0,01
    Q_PROPERTY(bool dragging READ dragging NOTIFY changed)

public:
    explicit CurtainController(QObject* parent = nullptr);

    bool active() const noexcept;
    void setActive(bool active); // n'ouvre ni ne ferme le store : voir toggle()
    QColor color() const noexcept;
    void setColor(const QColor& color);
    QSizeF area() const noexcept;
    void setArea(QSizeF area);
    QRectF covered() const noexcept;
    qreal coverage() const noexcept;
    bool dragging() const noexcept;

    Q_INVOKABLE bool press(QPointF point);
    Q_INVOKABLE void move(QPointF point);
    Q_INVOKABLE void release();
    // start : valeur de features::Curtain::Start (0 plein, 1 haut, 2 bas, 3 gauche, 4 droite).
    Q_INVOKABLE void reset(int start, qreal distance);
    // Active le rideau, fermé, ou le désactive.
    Q_INVOKABLE void toggle();

signals:
    void activeChanged();
    void colorChanged();
    void areaChanged();
    void changed();

private:
    features::Curtain m_curtain;
    QSizeF m_area;
    QColor m_color{Qt::black};
    bool m_active = false;
};

} // namespace actcraft::ui
