#pragma once

#include <actcraft/core/model.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>

#include <QObject>
#include <QPointer>
#include <QQuickTextDocument>
#include <QRectF>
#include <QString>
#include <QTextBlockFormat>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

#include <cstddef>
#include <functional>

namespace actcraft::ui {

// Édition d'un texte (F-250 à F-253, F-100, F-301 b) dans le TextEdit de TextEditor.qml : un texte existant
// est soulevé pendant l'édition (DA11) ; validation = une étape (AddObjectCommand ou SetTextCommand) ; texte
// vide : rien n'est gardé.
class TextEditing : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(TextEditing)
    Q_PROPERTY(actcraft::ui::DocumentController* documentController READ documentController WRITE
                   setDocumentController NOTIFY wiringChanged)
    Q_PROPERTY(actcraft::ui::PageView* view READ view WRITE setView NOTIFY wiringChanged)
    Q_PROPERTY(QQuickTextDocument* textDocument READ textDocument WRITE setTextDocument NOTIFY wiringChanged)
    Q_PROPERTY(int selectionStart READ selectionStart WRITE setSelectionStart NOTIFY formatChanged)
    Q_PROPERTY(int selectionEnd READ selectionEnd WRITE setSelectionEnd NOTIFY formatChanged)
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
    Q_PROPERTY(bool vertical READ isVertical NOTIFY activeChanged)
    Q_PROPERTY(QRectF frame READ frame NOTIFY frameChanged) // coordonnées de page
    // Largeur imposée (texte existant, poignée d'extension) : le TextEdit y passe à la ligne ; sinon non
    // (le cadre mesure alors idealWidth, qui s'effondrerait sur un document replié).
    Q_PROPERTY(bool fixedWidth READ hasFixedWidth NOTIFY frameChanged)
    Q_PROPERTY(QString plainText READ plainText NOTIFY formatChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily NOTIFY formatChanged)
    Q_PROPERTY(qreal fontSize READ fontSize NOTIFY formatChanged)
    Q_PROPERTY(bool bold READ bold NOTIFY formatChanged)
    Q_PROPERTY(bool italic READ italic NOTIFY formatChanged)
    Q_PROPERTY(bool underline READ underline NOTIFY formatChanged)

public:
    explicit TextEditing(QObject* parent = nullptr);
    DocumentController* documentController() const noexcept;
    void setDocumentController(DocumentController* controller);
    PageView* view() const noexcept;
    void setView(PageView* view);
    QQuickTextDocument* textDocument() const noexcept;
    void setTextDocument(QQuickTextDocument* document);
    int selectionStart() const noexcept;
    void setSelectionStart(int position);
    int selectionEnd() const noexcept;
    void setSelectionEnd(int position);
    bool isActive() const noexcept;
    bool isVertical() const noexcept;
    QRectF frame() const noexcept;
    bool hasFixedWidth() const noexcept { return m_fixedWidth > 0; }
    QString plainText() const;
    QString fontFamily() const;
    qreal fontSize() const;
    bool bold() const;
    bool italic() const;
    bool underline() const;

    // Texte touché au point (page) : édition ; sinon texte neuf « Votre texte » tout sélectionné, coin du
    // cadre au point. Une édition en cours est d'abord validée. Vrai si une édition commence.
    Q_INVOKABLE bool beginAt(QPointF pagePoint, bool vertical);
    Q_INVOKABLE void commit(); // F-252
    Q_INVOKABLE void cancel();
    // Commandes « text.* » de la barre (F-100) sur la sélection ; argument selon la commande.
    Q_INVOKABLE void apply(const QString& command, const QVariant& argument);
    Q_INVOKABLE void setFrameWidth(qreal width); // poignée d'extension (page)
    Q_INVOKABLE void moveFrame(QPointF delta);   // poignée de déplacement (page)

signals:
    void wiringChanged();
    void activeChanged();
    void frameChanged();
    void formatChanged();
    void focusRequested();

private:
    QTextDocument* editDocument() const; // document du TextEdit ; nullptr sans textDocument
    QTextCursor selectionCursor() const;
    QTextCharFormat currentCharFormat() const;
    void mergeFormat(const QTextCharFormat& format);
    void changeBlocks(const std::function<void(QTextBlockFormat&)>& change);
    QTextCharFormat newTextFormat() const;
    core::RectF measuredBounds(const QString& safeHtml) const;
    void updateFrame();
    void finish(); // fin d'édition (avant tout appel au contrôleur : réentrance)
    void rememberFormat();

    QPointer<DocumentController> m_controller;
    QPointer<PageView> m_view;
    QPointer<QQuickTextDocument> m_document;
    QPointer<QTextDocument> m_watched; // document dont contentsChanged recalcule le cadre
    core::ObjectId m_editing = 0;      // 0 : texte neuf
    bool m_active = false;
    bool m_vertical = false;
    QRectF m_frame;
    int m_selectionStart = 0;
    int m_selectionEnd = 0;
    std::size_t m_page = 0;     // page de l'édition
    core::RectF m_objectBounds; // emprise en coordonnées de l'objet (origine gardée, taille mesurée)
    qreal m_fixedWidth = 0;     // > 0 : largeur imposée (texte existant, poignée d'extension)
    bool m_geometryChanged = false;
    QString m_initialHtml; // HTML du TextEdit au début de l'édition : inchangé, aucune étape
    // Mémoire de la police et de la taille (D7) : celles des textes suivants. L'en-tête figé (L4-T1) ne donne
    // pas accès à tools::UserState : la mémoire est gardée ici.
    QString m_nextFamily; // vide : police de l'application (N2-3)
    qreal m_nextSize = 14;
};

} // namespace actcraft::ui
