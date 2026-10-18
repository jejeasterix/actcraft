#pragma once

#include <QString>
#include <QtCore/qnamespace.h>

#include <vector>

namespace actcraft::features {

struct KeyCap
{
    QString id;       // caractère tapé (minuscule) ou "shift", "caps", "backspace", "enter", "space"
    QString label;    // libellé à nous
    double width = 1; // en largeurs de touche
    int row = 0;
};

struct KeyOutput
{
    QString text; // vide pour une touche de commande
    Qt::Key key = Qt::Key_unknown;
};

// Clavier à l'écran (F-294), AZERTY, écrit par nous.
class ScreenKeyboard
{
public:
    enum class Layout { Compact, Standard, Extended };
    explicit ScreenKeyboard(Layout layout);
    void setLayout(Layout layout);
    Layout layout() const noexcept;
    const std::vector<KeyCap>& keys() const noexcept;
    KeyOutput press(const QString& id); // identifiant absent de la disposition : sortie vide
    bool shifted() const noexcept;      // majuscule pour la prochaine touche seulement
    bool capsLocked() const noexcept;

private:
    Layout m_layout;
    std::vector<KeyCap> m_keys;
    bool m_shift = false;
    bool m_caps = false;
};

} // namespace actcraft::features
