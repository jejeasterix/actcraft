#pragma once

#include <actcraft/core/model.h>
#include <actcraft/features/equationeditor.h>
#include <actcraft/ui/documentcontroller.h>

#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QSizeF>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

#include <string>

namespace actcraft::ui {

// Éditeur d'équations à modèles (F-301) offert à QML : features::EquationEditor, ses palettes, l'aperçu et
// l'insertion. `editorMathml` porte les marques de l'éditeur (cases vides, curseur) et ne sert qu'à l'aperçu
// ; le document ne reçoit que features::EquationEditor::documentMathml(), sans aucune marque. `accept` refuse
// une équation dont une case reste vide (`message` dit pourquoi) ; sinon il pose l'équation sur la page
// affichée, couche intermédiaire, centrée sur le point donné, en une étape d'annulation (DocumentInsertions).
// La boîte posée est la taille naturelle de l'équation, mesurée par le dessin de la page lui-même
// (render::paintEquation enregistré dans un QPicture, à l'échelle 1) : render/ n'offre pas de mesure
// publique.
class EquationController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString mathml READ mathml NOTIFY equationChanged)
    Q_PROPERTY(QString editorMathml READ editorMathml NOTIFY equationChanged)
    Q_PROPERTY(bool hasPlaceholder READ hasPlaceholder NOTIFY equationChanged)
    Q_PROPERTY(QSizeF editorSize READ editorSize NOTIFY equationChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(QVariantList palettes READ palettes CONSTANT)
    Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                   setDocumentController NOTIFY documentControllerChanged)

public:
    explicit EquationController(QObject* parent = nullptr);

    QString mathml() const;
    QString editorMathml() const;
    bool hasPlaceholder() const;
    QSizeF editorSize() const; // taille naturelle de l'équation éditée, marques comprises
    QString message() const;
    // Une entrée par catégorie : {id, label, entries: [{id, label, mathml}]} ; mathml : le modèle tel qu'il
    // s'insère (cases vides marquées), pour le dessin du bouton.
    static QVariantList palettes();
    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* controller);

    // Taille naturelle (pixels de page, à `pointSize` points) d'un MathML dessiné comme sur la page ; vide si
    // rien ne se dessine.
    static QSizeF naturalSize(const std::string& mathml, double pointSize);

    Q_INVOKABLE void open(); // une nouvelle équation, une case vide
    Q_INVOKABLE bool insertTemplate(const QString& id);
    Q_INVOKABLE void type(const QString& text);
    Q_INVOKABLE void backspace();
    Q_INVOKABLE void moveLeft();
    Q_INVOKABLE void moveRight();
    Q_INVOKABLE void moveUp();
    Q_INVOKABLE void moveDown();
    // Pose l'équation centrée sur `pageCentre` (point de page ; le centre de la page s'il n'est pas fini).
    // Faux, rien posé : case vide (message renseigné), pas de document ou de page.
    Q_INVOKABLE bool accept(QPointF pageCentre);

signals:
    void equationChanged();
    void messageChanged();
    void documentControllerChanged();

private:
    void edited();
    void setMessage(const QString& message);

    features::EquationEditor m_editor;
    QString m_message;
    QSizeF m_editorSize;
    QPointer<DocumentController> m_document;
};

} // namespace actcraft::ui
