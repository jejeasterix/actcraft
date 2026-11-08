#include <actcraft/features/screenkeyboard.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <cstddef>
#include <vector>
#include <windows.h>

// Seul appel de SendInput d'ActCraft (F-294). Jamais appelé par les tests automatiques : KeyboardController
// prend un injecteur remplaçable. Aucune frappe n'est journalisée.

namespace actcraft::features {

namespace {

void appendVirtualKey(std::vector<INPUT>& inputs, WORD virtualKey)
{
    INPUT down{};
    down.type = INPUT_KEYBOARD;
    down.ki.wVk = virtualKey;
    INPUT up = down;
    up.ki.dwFlags = KEYEVENTF_KEYUP;
    inputs.push_back(down);
    inputs.push_back(up);
}

// Une unité UTF-16 (une paire de substitution part en deux unités, comme Windows l'attend).
void appendUnicode(std::vector<INPUT>& inputs, char16_t unit)
{
    INPUT down{};
    down.type = INPUT_KEYBOARD;
    down.ki.wScan = static_cast<WORD>(unit);
    down.ki.dwFlags = KEYEVENTF_UNICODE;
    INPUT up = down;
    up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    inputs.push_back(down);
    inputs.push_back(up);
}

} // namespace

bool sendUnicodeToForeground(const QString& text)
{
    if (text.isEmpty()) {
        return false;
    }
    std::vector<INPUT> inputs;
    inputs.reserve(static_cast<std::size_t>(text.size()) * 2);
    for (const QChar c : text) {
        switch (c.unicode()) {
        case u'\b':
            appendVirtualKey(inputs, VK_BACK);
            break;
        case u'\r':
        case u'\n':
            appendVirtualKey(inputs, VK_RETURN);
            break;
        case u'\t':
            appendVirtualKey(inputs, VK_TAB);
            break;
        default:
            appendUnicode(inputs, c.unicode());
            break;
        }
    }
    const auto count = static_cast<UINT>(inputs.size());
    return SendInput(count, inputs.data(), static_cast<int>(sizeof(INPUT))) == count;
}

} // namespace actcraft::features
