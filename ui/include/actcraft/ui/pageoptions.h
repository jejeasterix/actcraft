#pragma once

#include <actcraft/ui/documentcontroller.h>

#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Réglages de la page affichée : grille (F-620 à F-623), fond (F-610), extension (F-301 a), zones (F-603).
class PageOptions : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PageSettings)
    Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                   setDocumentController NOTIFY wiringChanged)
    Q_PROPERTY(bool gridVisible READ gridVisible NOTIFY changed)
    Q_PROPERTY(bool gridSnap READ gridSnap NOTIFY changed)
    Q_PROPERTY(bool gridAbove READ gridAbove NOTIFY changed)
    // {visible, snap, above, previewZoom, baseZoom, zoomIncrement, levels: [{visible, snap, startX, startY,
    //  spacingX, spacingY, color, pattern, angle, line, alongX, alongY}]} ; noms de nos clés
    Q_PROPERTY(QVariantMap grid READ grid NOTIFY changed)
    Q_PROPERTY(
        QVariantMap background READ background NOTIFY changed) // {kind, color, color2, gradient, image}
    Q_PROPERTY(qreal pageHeight READ pageHeight NOTIFY changed)
    Q_PROPERTY(QString zones READ zones NOTIFY changed) // « <horizontales>x<verticales> », « 0x0 » par défaut

public:
    explicit PageOptions(QObject* parent = nullptr);
    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* controller);
    bool gridVisible() const;
    bool gridSnap() const;
    bool gridAbove() const;
    QVariantMap grid() const;
    QVariantMap background() const;
    qreal pageHeight() const;
    QString zones() const;

    Q_INVOKABLE void toggleGridHidden();               // F-623
    Q_INVOKABLE void toggleSnap();                     // F-622
    Q_INVOKABLE void beginDesigner();                  // ouvre une séance : les réglages suivants fusionnent
    Q_INVOKABLE void setGrid(const QVariantMap& grid); // aperçu immédiat (F-620)
    Q_INVOKABLE void addLevel();
    Q_INVOKABLE void removeLevel(int level);
    Q_INVOKABLE void resetGrid();           // un niveau sans accrochage
    Q_INVOKABLE void stepSpacing(int sign); // « + » et « - » : × (1 ± zoomIncrement)
    Q_INVOKABLE void endDesigner();
    Q_INVOKABLE void setBackground(const QVariantMap& background); // F-610 (L4-T20)
    Q_INVOKABLE void extendPage();                                 // F-301 a (L4-T26)
    Q_INVOKABLE void setZones(const QString& zones);               // F-603 (L4-T28)

signals:
    void wiringChanged();
    void changed();

private:
    // Grille de la page affichée (grille neuve sans contrôleur ni page).
    core::Grid currentGrid() const;
    // Remplace la grille de la page affichée par une SetGridCommand (fusion pendant une séance du créateur).
    void applyGrid(core::Grid grid);
    QPointer<DocumentController> m_controller;
    bool m_designer = false;
};

} // namespace actcraft::ui
