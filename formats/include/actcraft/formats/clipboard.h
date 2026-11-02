#pragma once
#include <actcraft/core/document.h>
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
    // Clé SHA-256 → extension (core::Resource::extension, qu'exige Document::addResource), remplie depuis
    // document.resource(clé)->extension. Clé absente, ou extension hors [a-z0-9]{1,8} : « bin », le défaut du
    // modèle (une charge sans cette table reste lisible). Ajout de L5-T4 (H5-13).
    std::map<std::string, std::string> extensions = {};
};
// {"format": "actcraft-objects/1", "objects": […], "resources": {clé: base64}, "extensions": {clé:
// extension}}
// ("extensions" facultatif, écrit seulement s'il n'est pas vide) ; objets écrits par le code natif.
QByteArray encodeClipboard(const ClipboardPayload& payload);
std::optional<ClipboardPayload> decodeClipboard(const QByteArray& bytes);
std::map<std::string, std::string> resourcesFor(const core::Document& document,
                                                const std::vector<core::PageObject>& objects);

} // namespace actcraft::formats
