#include "actcraft/ui/secretstore.h"

// clang-format off
#include <windows.h>
#include <wincred.h>
// clang-format on

#include <string>

namespace actcraft::ui {

namespace {

// Nom de l'identifiant générique : « ActCraft/<nom> » (UTF-16, comme l'attend l'API large).
std::wstring targetName(const QString& name)
{
    return (QStringLiteral("ActCraft/") + name).toStdWString();
}

} // namespace

bool SecretStore::available()
{
    return true;
}

bool SecretStore::write(const QString& name, const QByteArray& secret)
{
    // CredentialBlobSize est en octets ; le Gestionnaire refuse au-delà de CRED_MAX_CREDENTIAL_BLOB_SIZE.
    if (name.isEmpty() || secret.size() > CRED_MAX_CREDENTIAL_BLOB_SIZE)
        return false;
    std::wstring target = targetName(name);
    CREDENTIALW credential{};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = target.data();
    credential.CredentialBlobSize = static_cast<DWORD>(secret.size());
    // CredWriteW ne modifie pas le tampon : l'API le déclare sans const.
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char*>(secret.constData()));
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE; // propre au poste, jamais itinérant
    return CredWriteW(&credential, 0) != FALSE;
}

std::optional<QByteArray> SecretStore::read(const QString& name)
{
    if (name.isEmpty())
        return std::nullopt;
    const std::wstring target = targetName(name);
    PCREDENTIALW credential = nullptr;
    if (CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &credential) == FALSE)
        return std::nullopt;
    QByteArray secret;
    if (credential->CredentialBlob != nullptr && credential->CredentialBlobSize > 0)
        secret = QByteArray(reinterpret_cast<const char*>(credential->CredentialBlob),
                            static_cast<qsizetype>(credential->CredentialBlobSize));
    SecureZeroMemory(credential->CredentialBlob, credential->CredentialBlobSize);
    CredFree(credential);
    return secret;
}

bool SecretStore::remove(const QString& name)
{
    if (name.isEmpty())
        return false;
    const std::wstring target = targetName(name);
    return CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0) != FALSE;
}

} // namespace actcraft::ui
