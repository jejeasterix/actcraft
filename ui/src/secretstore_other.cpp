#include "actcraft/ui/secretstore.h"

namespace actcraft::ui {

// Hors Windows, pas de trousseau du poste (D6-8) : rien n'est gardé, rien n'est lu.
bool SecretStore::available()
{
    return false;
}

bool SecretStore::write(const QString&, const QByteArray&)
{
    return false;
}

std::optional<QByteArray> SecretStore::read(const QString&)
{
    return std::nullopt;
}

bool SecretStore::remove(const QString&)
{
    return false;
}

} // namespace actcraft::ui
