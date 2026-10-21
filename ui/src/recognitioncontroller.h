#pragma once

#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>
#include <actcraft/features/recognitionsession.h>
#include <actcraft/ui/documentcontroller.h>

#include <QColor>
#include <QElapsedTimer>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <cstdint>
#include <functional>
#include <vector>

namespace actcraft::ui {

// Reconnaissance de forme (F-296, L7-T21). Chaque trait du stylo versé dans le document (observateur de
// DocumentController, origine Stroke) entre dans une session (features::RecognitionSession) ; quand le délai
// commun s'est écoulé depuis le DERNIER trait, chaque trait reconnu est remplacé par sa forme, couche
// intermédiaire, en une étape d'annulation (DocumentInsertions::replaceObjects). La session ne se vide que sur
// documentReplaced : documentChanged part après chaque trait (décision 13 de la phase 2). Les traits mûrs
// passent par processDue, seul point de répartition : la reconnaissance d'écriture (L7-T23) s'y branche.
class RecognitionController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(DocumentController* documentController READ documentController WRITE setDocumentController NOTIFY
                   documentControllerChanged)
    Q_PROPERTY(bool shapeMode READ shapeMode WRITE setShapeMode NOTIFY shapeModeChanged)
    Q_PROPERTY(bool fillShapes READ fillShapes WRITE setFillShapes NOTIFY fillShapesChanged)
    // Couleur de remplissage choisie ; invalide (défaut) : celle du contour, donc du trait.
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)
    // Délai commun (RecognitionSession::kDefaultDelayMs, borné de 0 à kMaxDelayMs) ; 0 : au relâché.
    Q_PROPERTY(int delayMs READ delayMs WRITE setDelayMs NOTIFY delayMsChanged)
    // Genre de la dernière forme posée (« circle », « polygon »…) ; vide tant qu'aucune.
    Q_PROPERTY(QString lastKind READ lastKind NOTIFY lastKindChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY pendingCountChanged)

public:
    using Clock = std::function<std::int64_t()>; // instant monotone, en nanosecondes

    explicit RecognitionController(QObject* parent = nullptr);
    ~RecognitionController() override;

    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* controller);
    bool shapeMode() const noexcept;
    void setShapeMode(bool on); // fermé : la session est vidée
    bool fillShapes() const noexcept;
    void setFillShapes(bool on);
    QColor fillColor() const;
    void setFillColor(const QColor& color);
    int delayMs() const noexcept;
    void setDelayMs(int delayMs);
    QString lastKind() const;
    int pendingCount() const noexcept;

    // Tests : temps injecté (aucune attente réelle du délai) ; nul : horloge monotone du contrôleur.
    void setClock(Clock clock);
    // Traits mûrs à l'instant de l'horloge : reconnus et remplacés. Le minuteur l'appelle tant qu'un trait
    // attend.
    Q_INVOKABLE void processDue();
    // recognition.convert-to-shape : les traits en attente sont reconnus tout de suite, sans le délai.
    Q_INVOKABLE void convertPending();

signals:
    void documentControllerChanged();
    void shapeModeChanged();
    void fillShapesChanged();
    void fillColorChanged();
    void delayMsChanged();
    void lastKindChanged();
    void pendingCountChanged();

private:
    void onChanges(const core::Changes& applied, DocumentController::ChangeOrigin origin);
    void resetSession();
    void recognizeShapes(const std::vector<core::ObjectId>& strokes);
    void updateTimer();
    std::int64_t now() const;

    QPointer<DocumentController> m_document;
    QMetaObject::Connection m_replacedConnection;
    int m_observer = 0;
    features::RecognitionSession m_session;
    QTimer m_timer;
    QElapsedTimer m_elapsed;
    Clock m_clock;
    bool m_shapeMode = false;
    bool m_fillShapes = false;
    QColor m_fillColor;
    QString m_lastKind;
};

} // namespace actcraft::ui
