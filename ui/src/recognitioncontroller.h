#pragma once

#include <actcraft/core/model.h>
#include <actcraft/core/undo.h>
#include <actcraft/features/handwriting.h>
#include <actcraft/features/recognitionsession.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/toolhost.h> // type complet exigé par moc pour la propriété

#include <QColor>
#include <QElapsedTimer>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace actcraft::ui {

// Reconnaissance de forme (F-296, L7-T21). Chaque trait du stylo versé dans le document (observateur de
// DocumentController, origine Stroke) entre dans une session (features::RecognitionSession) ; quand le délai
// commun s'est écoulé depuis le DERNIER trait, chaque trait reconnu est remplacé par sa forme, couche
// intermédiaire, en une étape d'annulation (DocumentInsertions::replaceObjects). La session ne se vide que
// sur documentReplaced : documentChanged part après chaque trait (décision 13 de la phase 2). Les traits mûrs
// passent par processDue, seul point de répartition. Mode écriture (F-295, L7-T23) : exclusif avec le mode
// forme ; le contrôleur possède UN HandwritingThread (fil dédié, créé une fois, gardé). Les traits mûrs sont
// coupés en mots à chaque pause, chaque mot est reconnu sur ce fil, puis tous les mots reconnus remplacent
// leurs traits par un objet texte chacun, en une étape. Les autres lectures du dernier mot restent proposées
// (alternatives) jusqu'à la validation : un trait loin du texte, un Annuler ou un autre mode.
class RecognitionController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(DocumentController* documentController READ documentController WRITE setDocumentController
                   NOTIFY documentControllerChanged)
    // Outils du lot 3 : leur sélection (UserState::selection) est ce que les conversions a posteriori
    // convertissent (F-295, F-296, L7-T34).
    Q_PROPERTY(actcraft::ui::ToolHost* tools READ tools WRITE setTools NOTIFY toolsChanged)
    Q_PROPERTY(bool shapeMode READ shapeMode WRITE setShapeMode NOTIFY shapeModeChanged)
    Q_PROPERTY(bool fillShapes READ fillShapes WRITE setFillShapes NOTIFY fillShapesChanged)
    // Couleur de remplissage choisie ; invalide (défaut) : celle du contour, donc du trait.
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)
    // Délai commun (RecognitionSession::kDefaultDelayMs, borné de 0 à kMaxDelayMs) ; 0 : au relâché.
    Q_PROPERTY(int delayMs READ delayMs WRITE setDelayMs NOTIFY delayMsChanged)
    // Genre de la dernière forme posée (« circle », « polygon »…) ; vide tant qu'aucune.
    Q_PROPERTY(QString lastKind READ lastKind NOTIFY lastKindChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY pendingCountChanged)
    // Mode écriture : refusé tant que le reconnaisseur n'a pas répondu disponible.
    Q_PROPERTY(
        bool handwritingMode READ handwritingMode WRITE setHandwritingMode NOTIFY handwritingModeChanged)
    Q_PROPERTY(bool handwritingAvailable READ handwritingAvailable NOTIFY handwritingProbed)
    // Explication du moteur (info-bulle de la commande grisée) ; vide avant la réponse de probe().
    Q_PROPERTY(QString handwritingStatus READ handwritingStatus NOTIFY handwritingProbed)
    // Autres lectures du dernier texte reconnu (hors celle du texte) ; vide : rien à proposer.
    Q_PROPERTY(QStringList alternatives READ alternatives NOTIFY alternativesChanged)
    // Boîte, en pixels de page, du dernier texte reconnu.
    Q_PROPERTY(QRectF alternativesAnchor READ alternativesAnchor NOTIFY alternativesChanged)

public:
    using Clock = std::function<std::int64_t()>; // instant monotone, en nanosecondes

    explicit RecognitionController(QObject* parent = nullptr);
    ~RecognitionController() override;

    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* controller);
    ToolHost* tools() const noexcept;
    void setTools(ToolHost* tools);
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
    bool handwritingMode() const noexcept;
    void setHandwritingMode(bool on);
    bool handwritingAvailable() const noexcept;
    QString handwritingStatus() const;
    QStringList alternatives() const;
    QRectF alternativesAnchor() const;
    // Le fil dédié (nul avant le premier probeHandwriting()).
    QThread* handwritingThread() const noexcept;

    // Tests : fabrique du reconnaisseur, appelée sur le fil dédié (nul : celui de la plateforme). Le fil est
    // recréé et interrogé de nouveau.
    void setHandwritingFactoryForTest(features::HandwritingThread::Factory factory);
    std::size_t trackedArrivalsForTest() const { return m_arrivedAt.size(); } // pour les tests
    // Crée le fil au premier appel et lui demande si le moteur est disponible : handwritingProbed en réponse.
    Q_INVOKABLE void probeHandwriting();
    // Remplace le texte reconnu par l'une des autres lectures (index dans alternatives()), en une étape.
    Q_INVOKABLE void chooseAlternative(int index);
    // Valide le texte tel qu'il est : les alternatives se ferment.
    Q_INVOKABLE void validate();

    // Tests : temps injecté (aucune attente réelle du délai) ; nul : horloge monotone du contrôleur.
    void setClock(Clock clock);
    // Traits mûrs à l'instant de l'horloge : reconnus et remplacés. Le minuteur l'appelle tant qu'un trait
    // attend.
    Q_INVOKABLE void processDue();
    // Les traits en attente sont reconnus tout de suite, sans le délai, selon le mode ouvert.
    Q_INVOKABLE void convertPending();
    // recognition.convert-to-shape : les annotations sélectionnées deviennent des formes, mode fermé ou non,
    // en une étape ; sans sélection, les traits en attente (convertPending) ; une sélection toute tenue par
    // l'autre utilisateur ne convertit rien.
    Q_INVOKABLE void convertToShape();
    // recognition.convert-to-text : les annotations sélectionnées sont lues comme de l'écriture et
    // deviennent un texte, en une étape ; sans sélection, les traits en attente. Rien sans reconnaisseur, ni
    // pour une sélection toute tenue par l'autre utilisateur.
    Q_INVOKABLE void convertToText();

signals:
    void documentControllerChanged();
    void toolsChanged();
    void shapeModeChanged();
    void fillShapesChanged();
    void fillColorChanged();
    void delayMsChanged();
    void lastKindChanged();
    void pendingCountChanged();
    void handwritingModeChanged();
    void handwritingProbed();
    void alternativesChanged();

private:
    void forgetArrivalsOffTheDocument();
    void onChanges(const core::Changes& applied, DocumentController::ChangeOrigin origin);
    void resetSession();
    void forgetPending(const std::vector<core::ObjectId>& converted);
    std::vector<core::ObjectId> selectedObjects() const; // vide sans outils ou sans sélection
    bool selectionAllHeld() const; // sélection brute non vide, mais tout y est tenu par l'autre utilisateur
    void recognizeShapes(const std::vector<core::ObjectId>& strokes);
    void dispatch(const std::vector<core::ObjectId>& strokes);
    void recognizeHandwriting(const std::vector<core::ObjectId>& strokes);
    void startThread();
    void closeAlternatives();
    void showAlternatives(QStringList readings, QRectF bounds, core::ObjectId text);
    core::PageObject textFor(const QRectF& bounds, const QString& text) const;
    void updateTimer();
    std::int64_t now() const;

    QPointer<DocumentController> m_document;
    QPointer<ToolHost> m_tools;
    QMetaObject::Connection m_replacedConnection;
    QMetaObject::Connection m_liftingConnection;
    int m_observer = 0;
    features::RecognitionSession m_session;
    QTimer m_timer;
    QElapsedTimer m_elapsed;
    Clock m_clock;
    bool m_shapeMode = false;
    bool m_fillShapes = false;
    QColor m_fillColor;
    QString m_lastKind;
    // Écriture. Le fil est déclaré en dernier : détruit en premier, avant ce qui reçoit ses réponses.
    bool m_handwritingMode = false;
    bool m_available = false;
    QString m_status;
    std::uint64_t m_epoch = 0;     // session remise à zéro : les reconnaissances en vol sont abandonnées
    std::uint64_t m_threadGen = 0; // fil recréé : ses réponses en vol sont abandonnées
    // Instant d'arrivée de chaque trait tracé depuis la dernière remise à zéro, tant qu'il est au document
    // (retiré par la gomme ou une commande, il en sort ; voir forgetArrivalsOffTheDocument).
    std::map<core::ObjectId, std::int64_t> m_arrivedAt;
    features::HandwritingThread::Factory m_factory;
    QStringList m_readings; // lectures du dernier texte, la première est celle du texte posé
    int m_current = 0;      // lecture actuellement dans le texte
    QRectF m_anchor;
    core::ObjectId m_lastText = 0;
    std::unique_ptr<features::HandwritingThread> m_thread;
};

} // namespace actcraft::ui
