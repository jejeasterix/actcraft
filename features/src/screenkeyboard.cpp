#include <actcraft/features/screenkeyboard.h>

#include <algorithm>

namespace actcraft::features {

namespace {

QString commandLabel(const QString& id)
{
    if (id == QLatin1String("shift"))
        return QStringLiteral("Maj");
    if (id == QLatin1String("caps"))
        return QStringLiteral("Verr. maj");
    if (id == QLatin1String("backspace"))
        return QStringLiteral("Effacer");
    if (id == QLatin1String("enter"))
        return QStringLiteral("Entrée");
    if (id == QLatin1String("space"))
        return QStringLiteral("Espace");
    return id;
}

void addCharacters(std::vector<KeyCap>& keys, int row, const QString& characters)
{
    for (const QChar c : characters)
        keys.push_back({QString(c), QString(c), 1, row});
}

void addCommand(std::vector<KeyCap>& keys, int row, const QString& id, double width)
{
    keys.push_back({id, commandLabel(id), width, row});
}

std::vector<KeyCap> buildKeys(ScreenKeyboard::Layout layout)
{
    using Layout = ScreenKeyboard::Layout;
    std::vector<KeyCap> keys;
    int row = 0;
    if (layout == Layout::Extended) {
        addCharacters(keys, row++, QStringLiteral("1234567890"));
        addCharacters(keys, row++, QStringLiteral("éèàçù'-()\"?."));
    }
    addCharacters(keys, row++, QStringLiteral("azertyuiop"));
    addCharacters(keys, row++, QStringLiteral("qsdfghjklm"));
    addCommand(keys, row, QStringLiteral("shift"), 1.5);
    addCharacters(keys, row,
                  layout == Layout::Compact ? QStringLiteral("wxcvbn") : QStringLiteral("wxcvbn,;:!"));
    addCommand(keys, row++, QStringLiteral("backspace"), 1.5);
    if (layout != Layout::Compact)
        addCommand(keys, row, QStringLiteral("caps"), 1.5);
    addCommand(keys, row, QStringLiteral("space"), 6);
    addCommand(keys, row, QStringLiteral("enter"), 2);
    return keys;
}

} // namespace

ScreenKeyboard::ScreenKeyboard(Layout layout) : m_layout(layout), m_keys(buildKeys(layout)) {}

void ScreenKeyboard::setLayout(Layout layout)
{
    m_layout = layout;
    m_keys = buildKeys(layout);
    m_shift = false;
}

ScreenKeyboard::Layout ScreenKeyboard::layout() const noexcept
{
    return m_layout;
}

const std::vector<KeyCap>& ScreenKeyboard::keys() const noexcept
{
    return m_keys;
}

bool ScreenKeyboard::shifted() const noexcept
{
    return m_shift;
}

bool ScreenKeyboard::capsLocked() const noexcept
{
    return m_caps;
}

KeyOutput ScreenKeyboard::press(const QString& id)
{
    const bool present =
        std::any_of(m_keys.begin(), m_keys.end(), [&](const KeyCap& cap) { return cap.id == id; });
    if (!present)
        return {};
    if (id == QLatin1String("shift")) {
        m_shift = !m_shift;
        return {QString(), Qt::Key_Shift};
    }
    if (id == QLatin1String("caps")) {
        m_caps = !m_caps;
        return {QString(), Qt::Key_CapsLock};
    }
    if (id == QLatin1String("backspace"))
        return {QString(), Qt::Key_Backspace};
    if (id == QLatin1String("enter"))
        return {QString(), Qt::Key_Return};
    if (id == QLatin1String("space"))
        return {QStringLiteral(" "), Qt::Key_Space};

    QString text = id;
    if (m_shift || m_caps)
        text = text.toUpper();
    m_shift = false;
    // Les touches Latin-1 de Qt valent la majuscule du caractère (Key_A, Key_Eacute, Key_Ccedilla…).
    const char16_t code = text.at(0).toUpper().unicode();
    return {text, code < 0x100 ? static_cast<Qt::Key>(code) : Qt::Key_unknown};
}

} // namespace actcraft::features
