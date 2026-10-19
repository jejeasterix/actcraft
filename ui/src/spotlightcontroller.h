#pragma once

#include <actcraft/core/model.h>
#include <actcraft/features/spotlight.h>
#include <actcraft/ui/documentcontroller.h>

#include <QColor>
#include <QImage>
#include <QObject>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QSizeF>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Spot (F-270) : l'état du store (actcraft::features::Spotlight) offert à QML. Une instance par fenêtre ;
// aucun lien au document sauf la capture de la zone éclairée (insertCapture), aucune demande de redessin du
// cache : la surcouche ne fait que dessiner un masque, en coordonnées de page.
class SpotlightController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int shape READ shape NOTIFY changed) // valeur de core::SpotlightShape (0 fermé à 4)
    Q_PROPERTY(bool active READ active NOTIFY changed)
    Q_PROPERTY(bool hidesInside READ hidesInside NOTIFY changed)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QSizeF area READ area WRITE setArea NOTIFY changed)
    Q_PROPERTY(QRectF frame READ frame NOTIFY changed)
    Q_PROPERTY(qreal centerX READ centerX NOTIFY changed)
    Q_PROPERTY(qreal centerY READ centerY NOTIFY changed)
    Q_PROPERTY(qreal frameWidth READ frameWidth NOTIFY changed)
    Q_PROPERTY(qreal frameHeight READ frameHeight NOTIFY changed)
    Q_PROPERTY(qreal outline READ outline CONSTANT) // largeur du contour : Spotlight::kDefaultOutline
    Q_PROPERTY(bool dragging READ dragging NOTIFY changed)

public:
    // Zone sous un point : ce que l'appui y ferait.
    enum Zone
    {
        None = 0,             // l'appui traverse vers la page
        Move = 1,             // déplace le spot
        ResizeHorizontal = 2, // pourtour gauche ou droit
        ResizeVertical = 3    // pourtour haut ou bas
    };
    Q_ENUM(Zone)

    explicit SpotlightController(QObject* parent = nullptr);

    // Conversion de la forme du store vers celle du document, et retour : mêmes valeurs (static_assert).
    static core::SpotlightShape toCore(features::Spotlight::Shape shape) noexcept;
    static features::Spotlight::Shape fromCore(core::SpotlightShape shape) noexcept;

    int shape() const noexcept;
    bool active() const noexcept;
    bool hidesInside() const noexcept;
    QColor color() const noexcept;
    void setColor(const QColor& color);
    QSizeF area() const noexcept;
    void setArea(QSizeF area);
    QRectF frame() const noexcept;
    qreal centerX() const noexcept;
    qreal centerY() const noexcept;
    qreal frameWidth() const noexcept;
    qreal frameHeight() const noexcept;
    qreal outline() const noexcept;
    bool dragging() const noexcept;

    Q_INVOKABLE bool press(QPointF point);
    Q_INVOKABLE void move(QPointF point);
    Q_INVOKABLE void release();
    // shape : valeur de core::SpotlightShape. La forme déjà active ferme le spot ; une autre la remplace en
    // gardant le cadre ; depuis le spot fermé, le cadre revient au centre.
    Q_INVOKABLE void toggle(int shape);
    // L'appui en `point` serait pris (sinon il traverse vers la page) : lu par le masque de saisie de la vue.
    Q_INVOKABLE bool covers(QPointF point) const;
    Q_INVOKABLE int zoneAt(QPointF point) const;   // Zone
    Q_INVOKABLE int cursorAt(QPointF point) const; // Qt::CursorShape de la zone
    // Contour de la zone éclairée en coordonnées de page (vide pour un cache plein ou un spot fermé).
    Q_INVOKABLE QPolygonF litPolygon() const;
    // Capture : `page` est l'image de la page entière (un pixel par unité de page ou à l'échelle de la zone)
    // ; la zone éclairée en est découpée (features::cropToPolygon), codée en PNG et insérée en une étape.
    Q_INVOKABLE bool insertCapture(const QImage& page, actcraft::ui::DocumentController* document);

signals:
    void colorChanged();
    void changed();

private:
    features::Spotlight m_spot;
    QSizeF m_area;
    QColor m_color{Qt::black};
};

} // namespace actcraft::ui
