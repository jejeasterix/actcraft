#include <actcraft/ui/keyboardinset.h>

#include <algorithm>

namespace actcraft::ui {

KeyboardInset::KeyboardInset(QObject* parent) : QObject(parent) {}

QRectF KeyboardInset::keyboardRect() const noexcept
{
    return m_keyboardRect;
}

void KeyboardInset::setKeyboardRect(const QRectF& rect)
{
    if (rect == m_keyboardRect) {
        return;
    }
    m_keyboardRect = rect;
    emit keyboardRectChanged();
    update();
}

QRectF KeyboardInset::caretRect() const noexcept
{
    return m_caretRect;
}

void KeyboardInset::setCaretRect(const QRectF& rect)
{
    if (rect == m_caretRect) {
        return;
    }
    m_caretRect = rect;
    emit caretRectChanged();
    update();
}

qreal KeyboardInset::margin() const noexcept
{
    return m_margin;
}

void KeyboardInset::setMargin(qreal margin)
{
    if (qFuzzyCompare(margin, m_margin)) {
        return;
    }
    m_margin = margin;
    emit marginChanged();
    update();
}

qreal KeyboardInset::shift() const noexcept
{
    return m_shift;
}

void KeyboardInset::update()
{
    // Clavier masqué : rectangle vide. Le curseur est dégagé si son bas, plus la marge, tient au-dessus du clavier.
    qreal shift = 0;
    if (m_keyboardRect.height() > 0 && !m_caretRect.isNull()) {
        shift = std::max<qreal>(0, m_caretRect.bottom() + m_margin - m_keyboardRect.top());
    }
    if (qFuzzyCompare(shift + 1, m_shift + 1)) {
        return;
    }
    m_shift = shift;
    emit shiftChanged();
}

} // namespace actcraft::ui
