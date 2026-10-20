#pragma once

#include "curtaincontroller.h"
#include "spotlightcontroller.h"

#include <actcraft/core/undo.h>
#include <actcraft/ui/documentcontroller.h>

#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QString>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Outils de page (F-591 à F-593) : à chaque changement de page affichée, et quand le document est remplacé
// (documentReplaced, L7-T2), les réglages de la page (core::PagePresentation) sont résolus
// (features::resolvePageTools) et appliqués au rideau et au spot. Jamais sur documentChanged : il part après
// chaque modification, trait compris, et remettrait à zéro le rideau tiré à la main. La page appliquée est
// suivie par l'observateur de changements (core::applyToCache), pas par son indice : une page ajoutée ou
// retirée avant elle ne la change pas. Ouvrir le spot ferme le rideau ; le cadre du spot déplacé sur une page
// « spot » y est écrit au relâché. Les commandes page.* écrivent les réglages de la page affichée, une étape
// chacune.
class PageToolsController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                   setDocumentController NOTIFY documentControllerChanged)
    Q_PROPERTY(QObject* curtain READ curtain WRITE setCurtain NOTIFY curtainChanged)
    Q_PROPERTY(QObject* spotlight READ spotlight WRITE setSpotlight NOTIFY spotlightChanged)

public:
    explicit PageToolsController(QObject* parent = nullptr);
    ~PageToolsController() override;

    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* controller);
    QObject* curtain() const noexcept;
    void setCurtain(QObject* curtain); // un CurtainController, sinon rien
    QObject* spotlight() const noexcept;
    void setSpotlight(QObject* spotlight); // un SpotlightController, sinon rien

    // Applique les réglages de la page affichée au rideau et au spot.
    Q_INVOKABLE void applyShownPage();
    // Réglages de la page affichée, une étape ; faux si la valeur est inconnue ou s'il n'y a pas de document.
    Q_INVOKABLE bool setToolsMode(const QString& mode);   // keep, curtain, spotlight, off
    Q_INVOKABLE bool setCurtainMode(const QString& mode); // off, top, bottom, left, right, full
    Q_INVOKABLE bool setCurtainDistance(double distance); // pixels de page, fini et positif
    Q_INVOKABLE bool setSpotShape(const QString& mode);   // off, circle, square, solid-circle, solid-square

signals:
    void documentControllerChanged();
    void curtainChanged();
    void spotlightChanged();

private:
    void onPagesChanged();
    void onSpotlightChanged();
    void followShownPage(const core::Changes& applied);

    QPointer<DocumentController> m_document;
    QPointer<CurtainController> m_curtain;
    QPointer<SpotlightController> m_spotlight;
    QMetaObject::Connection m_pagesConnection;
    QMetaObject::Connection m_replacedConnection;
    QMetaObject::Connection m_spotlightConnection;
    int m_observer = 0;    // jeton de l'observateur de changements du document
    int m_shownIndex = -1; // indice actuel de la page dont les réglages ont été appliqués ; -1 : retirée
    bool m_spotlightWasActive = false;
    bool m_spotlightWasDragging = false;
    QRectF m_frameAtPress; // cadre du spot à la saisie
};

// Mémoriser le rideau pour la page (P9 de L7-T0) : le côté et la distance du rideau vivant deviennent les
// réglages de la page affichée (outil « rideau », côté, distance), en une étape
// (DocumentInsertions::setPagePresentation). Porté par la surcouche du rideau, pour l'entrée « Mémoriser pour
// cette page » de son menu.
class CurtainPageMemory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                   setDocumentController NOTIFY documentControllerChanged)
    Q_PROPERTY(QObject* curtain READ curtain WRITE setCurtain NOTIFY curtainChanged)

public:
    explicit CurtainPageMemory(QObject* parent = nullptr);

    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* controller);
    QObject* curtain() const noexcept;
    void setCurtain(QObject* curtain);

    // Faux sans document ou sans rideau actif.
    Q_INVOKABLE bool remember();

signals:
    void documentControllerChanged();
    void curtainChanged();

private:
    QPointer<DocumentController> m_document;
    QPointer<CurtainController> m_curtain;
};

} // namespace actcraft::ui
