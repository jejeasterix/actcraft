#pragma once
#include <actcraft/core/model.h>

#include <QByteArray>
#include <QString>

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace actcraft::formats {

inline const QString kObjectsMimeType = QStringLiteral("application/x-actcraft-objects");

struct ClipboardPayload
{
    std::vector<core::PageObject> objects;        // fermés (closureForCopy), ordre de la page
    std::map<std::string, std::string> resources; // clé SHA-256 → octets
};
// {"format": "actcraft-objects/1", "objects": […], "resources": {clé: base64}} ; objets écrits par le code
// natif.
QByteArray encodeClipboard(const ClipboardPayload& payload);
std::optional<ClipboardPayload> decodeClipboard(const QByteArray& bytes);
std::map<std::string, std::string> resourcesFor(const core::Document& document,
                                                const std::vector<core::PageObject>& objects);

} // namespace actcraft::formats
