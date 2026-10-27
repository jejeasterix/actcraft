#include "keyboardcontroller.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QVariantMap>

#include <utility>

namespace actcraft::ui {

namespace {

features::ScreenKeyboard::Layout toFeatures(KeyboardController::Layout layout) noexcept
{
    switch (layout) {
    case KeyboardController::Layout::Compact:
        return features::ScreenKeyboard::Layout::Compact;
    case KeyboardController::Layout::Extended:
        return features::ScreenKeyboard::Layout::Extended;
    case KeyboardController::Layout::Standard:
        break;
    }
    return features::ScreenKeyboard::Layout::Standard;
}

KeyboardController::Layout fromFeatures(features::ScreenKeyboard::Layout layout) noexcept
{
    switch (layout) {
    case features::ScreenKeyboard::Layout::Compact:
        return KeyboardController::Layout::Compact;
    case features::ScreenKeyboard::Layout::Extended:
        return KeyboardController::Layout::Extended;
    case features::ScreenKeyboard::Layout::Standard:
        break;
    }
    return KeyboardController::Layout::Standard;
}

// Texte confié à l'injecteur : les touches de commande y deviennent leur caractère de contrôle.
QString injectedText(const features::KeyOutput& output)
{
    switch (output.key) {
    case Qt::Key_Backspace:
        return QStringLiteral("\b");
    case Qt::Key_Return:
        return QStringLiteral("\r");
    default:
        return output.text;
    }
}

} // namespace

KeyboardController::KeyboardController(QObject* parent)
    : QObject(parent), m_keyboard(features::ScreenKeyboard::Layout::Standard),
      m_injector(&features::sendUnicodeToForeground)
{
    if (qGuiApp) {
        connect(qGuiApp, &QGuiApplication::focusWindowChanged, this,
                &KeyboardController::onFocusWindowChanged);
        onFocusWindowChanged(QGuiApplication::focusWindow());
    }
}

KeyboardController::~KeyboardController() = default;

bool KeyboardController::isOpen() const noexcept
{
    return m_open;
}

void KeyboardController::setOpen(bool open)
{
    if (open == m_open) {
        return;
    }
    m_open = open;
    emit openChanged(); // la racine crée ou détruit la fenêtre
}

KeyboardController::Layout KeyboardController::layout() const noexcept
{
    return fromFeatures(m_keyboard.layout());
}

void KeyboardController::setLayout(Layout layout)
{
    if (layout == this->layout()) {
        return;
    }
    m_keyboard.setLayout(toFeatures(layout));
    emit layoutChanged();
    emit modifiersChanged(); // la majuscule d'une touche tombe avec la disposition
}

QVariantList KeyboardController::rows() const
{
    QVariantList rows;
    for (const features::KeyCap& cap : m_keyboard.keys()) {
        while (rows.size() <= cap.row) {
            rows.append(QVariant(QVariantList{}));
        }
        QVariantList row = rows[cap.row].toList();
        row.append(QVariantMap{{QStringLiteral("id"), cap.id},
                               {QStringLiteral("label"), cap.label},
                               {QStringLiteral("width"), cap.width}});
        rows[cap.row] = row;
    }
    return rows;
}

bool KeyboardController::shifted() const noexcept
{
    return m_keyboard.shifted();
}

bool KeyboardController::capsLocked() const noexcept
{
    return m_keyboard.capsLocked();
}

QQuickWindow* KeyboardController::window() const noexcept
{
    return m_window;
}

void KeyboardController::setWindow(QQuickWindow* window)
{
    if (window == m_window) {
        return;
    }
    m_window = window;
    emit windowChanged();
}

void KeyboardController::setInjector(Injector injector)
{
    m_injector = std::move(injector);
}

void KeyboardController::toggle()
{
    setOpen(!m_open);
}

bool KeyboardController::press(const QString& id)
{
    const bool shiftBefore = m_keyboard.shifted();
    const bool capsBefore = m_keyboard.capsLocked();
    const features::KeyOutput output = m_keyboard.press(id);
    if (m_keyboard.shifted() != shiftBefore || m_keyboard.capsLocked() != capsBefore) {
        emit modifiersChanged();
    }
    if (output.text.isEmpty() && output.key == Qt::Key_unknown) {
        return false; // touche absente de la disposition
    }
    if (output.key == Qt::Key_Shift || output.key == Qt::Key_CapsLock) {
        return true; // état du clavier seulement : rien ne part
    }
    if (sendToFocus(output)) {
        return true;
    }
    return m_injector && m_injector(injectedText(output));
}

QWindow* KeyboardController::focusTarget() const
{
    QWindow* focus = QGuiApplication::focusWindow();
    if (focus != nullptr && focus == m_window.data()) {
        // Plateforme qui a donné le focus au clavier malgré son drapeau : la frappe va à la fenêtre d'avant.
        focus = m_lastFocus.data();
    }
    return focus;
}

bool KeyboardController::sendToFocus(const features::KeyOutput& output)
{
    QWindow* target = focusTarget();
    if (target == nullptr) {
        return false; // aucune fenêtre d'ActCraft n'a le focus : une autre application est au premier plan
    }
    // À la fenêtre qui a le focus, qui remet la frappe à son objet de focus (QGuiApplication::focusObject)
    // comme une frappe du clavier physique.
    QKeyEvent press(QEvent::KeyPress, output.key, Qt::NoModifier, output.text);
    QCoreApplication::sendEvent(target, &press);
    QKeyEvent release(QEvent::KeyRelease, output.key, Qt::NoModifier, output.text);
    QCoreApplication::sendEvent(target, &release);
    return true;
}

void KeyboardController::onFocusWindowChanged(QWindow* focus)
{
    if (focus == nullptr) {
        return;
    }
    if (focus == m_window.data()) {
        // Le clavier ne garde jamais le focus (Qt::WindowDoesNotAcceptFocus ignoré par la plateforme).
        if (m_lastFocus) {
            m_lastFocus->requestActivate();
        }
        return;
    }
    m_lastFocus = focus;
}

} // namespace actcraft::ui
