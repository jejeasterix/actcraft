#pragma once

#include <QByteArray>
#include <QString>

#include <optional>

namespace actcraft::ui {

// Secrets propres au poste (D6-8). Windows : Gestionnaire d'identification ; ailleurs : indisponible.
// Un secret est rangé sous « ActCraft/<nom> » (identifiant générique, persistance du poste).
class SecretStore
{
public:
    static bool available();
    static bool write(const QString& name, const QByteArray& secret);
    static std::optional<QByteArray> read(const QString& name); // absent ou indisponible : vide
    static bool remove(const QString& name);                    // faux si rien n'a été effacé
};

} // namespace actcraft::ui
