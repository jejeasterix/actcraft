#pragma once

#include <QObject>
#include <QRectF>
#include <QtQml/qqmlregistration.h>

namespace actcraft::ui {

// Clavier logiciel (D11-11) : de combien faut-il faire défiler la vue pour que le curseur de texte reste
// au-dessus du clavier ? Entrées : le rectangle du clavier et celui du curseur, tous deux en coordonnées de
// fenêtre (dp), et une marge. Sortie : `shift`, défilement vertical à appliquer (>= 0) ; 0 quand le clavier
// est masqué ou que le curseur est déjà dégagé. Pur calcul : aucun accès à la fenêtre ni au clavier ici, le
// QML branche les entrées.
class KeyboardInset : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QRectF keyboardRect READ keyboardRect WRITE setKeyboardRect NOTIFY keyboardRectChanged)
    Q_PROPERTY(QRectF caretRect READ caretRect WRITE setCaretRect NOTIFY caretRectChanged)
    Q_PROPERTY(qreal margin READ margin WRITE setMargin NOTIFY marginChanged)
    Q_PROPERTY(qreal shift READ shift NOTIFY shiftChanged FINAL)

public:
    static constexpr qreal kDefaultMargin = 16;

    explicit KeyboardInset(QObject* parent = nullptr);

    QRectF keyboardRect() const noexcept;
    void setKeyboardRect(const QRectF& rect);
    QRectF caretRect() const noexcept;
    void setCaretRect(const QRectF& rect);
    qreal margin() const noexcept;
    void setMargin(qreal margin);
    qreal shift() const noexcept;

signals:
    void keyboardRectChanged();
    void caretRectChanged();
    void marginChanged();
    void shiftChanged();

private:
    void update();

    QRectF m_keyboardRect;
    QRectF m_caretRect;
    qreal m_margin = kDefaultMargin;
    qreal m_shift = 0;
};

} // namespace actcraft::ui
