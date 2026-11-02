#include "nativeobjects.h"

#include <actcraft/formats/clipboard.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QStringList>

#include <utility>
#include <variant>

// Presse-papiers d'objets (L5-T3). Chaque objet est écrit et lu par le code du format natif
// (nativeobjects.h), jamais champ par champ ici : un objet copié garde tout ce que l'enregistrement garde
// (types inconnus, clés inconnues, clés facultatives). Les ressources voyagent en base64, contrôlées par leur
// clé SHA-256.

namespace actcraft::formats {

using namespace Qt::StringLiterals;

namespace {

constexpr QStringView kFormatPrefix = u"actcraft-objects/";
constexpr QStringView kFormatMajor = u"1";

// « actcraft-objects/<majeure>[.<mineure>…] » : la majeure seule décide, comme pour document.json.
bool isClipboardFormat(const QJsonValue& value)
{
    if (!value.isString()) {
        return false;
    }
    const QString format = value.toString();
    return format.startsWith(kFormatPrefix) &&
           format.mid(kFormatPrefix.size()).section(u'.', 0, 0) == kFormatMajor;
}

// Clés de ressource qu'un objet cite : image, espace réservé et média d'un Media, fichier ouvert par une
// action. Un objet d'un type inconnu ne cite rien : rien de ce qu'il cite n'est chargé (L2-C8).
template <typename F> void forEachCitedKey(const core::PageObject& object, F&& visit)
{
    if (std::holds_alternative<core::UnknownObject>(core::body(object))) {
        return;
    }
    const auto cite = [&visit](const std::string& key) {
        if (!key.empty()) {
            visit(key);
        }
    };
    for (const core::Action& action : object.common.actions) {
        cite(action.resource);
    }
    if (const auto* image = std::get_if<core::ImageObject>(&object)) {
        cite(image->resource);
    } else if (const auto* media = std::get_if<core::Media>(&object)) {
        cite(media->placeholder);
        cite(media->media);
    }
}

} // namespace

QByteArray encodeClipboard(const ClipboardPayload& payload)
{
    QJsonArray objects;
    for (const core::PageObject& object : payload.objects) {
        objects.append(native::objectJson(object));
    }
    QJsonObject resources;
    for (const auto& [key, bytes] : payload.resources) {
        resources.insert(QString::fromStdString(key),
                         QString::fromLatin1(QByteArray::fromStdString(bytes).toBase64()));
    }
    const QJsonObject root{
        {u"format"_s, u"actcraft-objects/1"_s}, {u"objects"_s, objects}, {u"resources"_s, resources}};
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

std::optional<ClipboardPayload> decodeClipboard(const QByteArray& bytes)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return std::nullopt;
    }
    const QJsonObject root = document.object();
    const QJsonValue objects = root.value(u"objects"_s);
    const QJsonValue resources = root.value(u"resources"_s);
    if (!isClipboardFormat(root.value(u"format"_s)) || !objects.isArray() ||
        (!resources.isUndefined() && !resources.isObject())) {
        return std::nullopt;
    }

    ClipboardPayload payload;
    const QJsonObject declared = resources.toObject();
    for (auto it = declared.begin(); it != declared.end(); ++it) {
        if (!it.value().isString()) {
            return std::nullopt;
        }
        const QByteArray::FromBase64Result data = QByteArray::fromBase64Encoding(
            it.value().toString().toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
        const std::string key = it.key().toStdString();
        if (!data || core::sha256Hex(data.decoded) != key) {
            return std::nullopt; // octets illisibles, ou qui ne correspondent pas à leur clé
        }
        payload.resources.emplace(key, data.decoded.toStdString());
    }

    QStringList warnings; // valeurs mal formées : défaut du modèle, comme à l'ouverture d'un fichier
    payload.objects = native::readObjects(objects.toArray(), warnings);
    bool complete = true;
    for (const core::PageObject& object : payload.objects) {
        forEachCitedKey(
            object, [&](const std::string& key) { complete = complete && payload.resources.contains(key); });
    }
    if (!complete) {
        return std::nullopt; // ressource citée absente
    }
    return payload;
}

std::map<std::string, std::string> resourcesFor(const core::Document& document,
                                                const std::vector<core::PageObject>& objects)
{
    std::map<std::string, std::string> resources;
    for (const core::PageObject& object : objects) {
        forEachCitedKey(object, [&](const std::string& key) {
            if (const core::Resource* resource = document.resource(key)) {
                resources.emplace(key, resource->data.toStdString());
            }
        });
    }
    return resources;
}

} // namespace actcraft::formats
