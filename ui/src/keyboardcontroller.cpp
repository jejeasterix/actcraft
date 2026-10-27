#include "keyboardcontroller.h"

#include <utility>

// Talon du test rouge (L7-T29) : déclarations seules, rien n'est branché.

namespace actcraft::ui {

KeyboardController::KeyboardController(QObject* parent)
    : QObject(parent), m_keyboard(features::ScreenKeyboard::Layout::Standard)
{}

KeyboardController::~KeyboardController() = default;

bool KeyboardController::isOpen() const noexcept
{
    return m_open;
}

void KeyboardController::setOpen(bool) {}

KeyboardController::Layout KeyboardController::layout() const noexcept
{
    return Layout::Standard;
}

void KeyboardController::setLayout(Layout) {}

QVariantList KeyboardController::keys() const
{
    return {};
}

int KeyboardController::rowCount() const noexcept
{
    return 0;
}

bool KeyboardController::shifted() const noexcept
{
    return false;
}

bool KeyboardController::capsLocked() const noexcept
{
    return false;
}

QQuickWindow* KeyboardController::window() const noexcept
{
    return m_window;
}

void KeyboardController::setWindow(QQuickWindow*) {}

void KeyboardController::setInjector(Injector injector)
{
    m_injector = std::move(injector);
}

void KeyboardController::toggle() {}

bool KeyboardController::press(const QString&)
{
    return false;
}

bool KeyboardController::sendToFocus(const features::KeyOutput&)
{
    return false;
}

void KeyboardController::onFocusWindowChanged(QWindow*) {}

} // namespace actcraft::ui
