#pragma once

#include <actcraft/core/hittest.h>
#include <actcraft/core/model.h>
#include <actcraft/ui/documentcontroller.h>

#include <QColor>
#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Légende d'un objet montrée au survol (F-576, D12). `hoverAt` cherche l'objet visible le plus haut sous le
// point de page (core::objectAt) ; s'il porte une légende, `visible` devient vrai et `text`, la police, la
// taille, les couleurs reprennent la légende et son style (ObjectCommon::labelStyle). Sans objet, sans
// légende ou pour un objet masqué, `visible` est faux. Le survol ne passe pas par le routeur d'entrée : aucun
// trait, aucune étape d'annulation.
class LabelTips : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
        actcraft::ui::DocumentController* document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(QString text READ text NOTIFY tipChanged)
    Q_PROPERTY(bool visible READ isVisible NOTIFY tipChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily NOTIFY tipChanged)
    Q_PROPERTY(qreal pointSize READ pointSize NOTIFY tipChanged)
    Q_PROPERTY(QColor color READ color NOTIFY tipChanged)
    // Transparente quand le style n'a pas de contour.
    Q_PROPERTY(QColor outlineColor READ outlineColor NOTIFY tipChanged)
    Q_PROPERTY(QColor fill READ fill NOTIFY tipChanged)
    // Tolérance de désignation, en unités de page (core::kHitToleranceScreen / zoom, posée par la vue).
    Q_PROPERTY(qreal tolerance READ tolerance WRITE setTolerance NOTIFY toleranceChanged)

public:
    explicit LabelTips(QObject* parent = nullptr);

    DocumentController* document() const noexcept;
    void setDocument(DocumentController* document);
    QString text() const;
    bool isVisible() const noexcept;
    QString fontFamily() const;
    qreal pointSize() const noexcept;
    QColor color() const;
    QColor outlineColor() const;
    QColor fill() const;
    qreal tolerance() const noexcept;
    void setTolerance(qreal tolerance);

    Q_INVOKABLE void hoverAt(QPointF pagePoint);
    Q_INVOKABLE void leave();

signals:
    void documentChanged();
    void tipChanged();
    void toleranceChanged();

private:
    void clear();

    QPointer<DocumentController> m_document;
    qreal m_tolerance = core::kHitToleranceScreen;
    bool m_visible = false;
    QString m_text;
    QString m_fontFamily;
    qreal m_pointSize = 12;
    QColor m_color = Qt::black;
    QColor m_outlineColor = Qt::transparent;
    QColor m_fill = Qt::transparent;
};

} // namespace actcraft::ui
