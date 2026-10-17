#pragma once

#include <actcraft/core/document.h>

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <functional>
#include <optional>
#include <string>

namespace actcraft::formats {

// Format natif (DA3, décision 17) : « document.json » de l'archive .actcraft. Clés et noms d'énumérations en
// anglais, écrits en clair un par un, figés par nativejson.cpp et par tst_formats_nativejson (keysAreFrozen).
// Un champ du modèle = une clé. Tout changement incompatible passe par « actcraft-document/2 » et un lecteur
// des deux versions ; une version mineure plus récente (« actcraft-document/1.x ») se lit.
inline constexpr char kNativeFormat[] = "actcraft-document/1";
inline constexpr int kNativeMajorVersion = 1;

// Tout le document sauf les octets des ressources (déclarées par clé SHA-256 et extension ; l'archive les
// porte, L2-T6). Parité : documentFromJson(documentToJson(d)) a l'empreinte core::fingerprint(d) et le même
// nextId(). Deux limites de QJsonValue, mesurées : un réel −0 revient +0 (QJsonValue range un réel entier
// comme un entier ; l'empreinte hache −0 comme +0, donc elle ne change pas) ; un identifiant au-delà de 2^53
// n'est pas porté exactement (la lecture le refuse).
QJsonObject documentToJson(const core::Document& document);

// Octets d'une ressource déclarée (clé SHA-256, extension) ; nullopt si l'archive ne les a pas. N'est appelé
// qu'avec une clé [0-9a-f]{64} et une extension [a-z0-9]{1,8} : jamais avec un nom qui viserait ailleurs.
using ResourceLoader =
    std::function<std::optional<QByteArray>(const std::string& key, const std::string& extension)>;

struct JsonReadResult
{
    bool ok = false;
    QString error;        // si !ok
    QStringList warnings; // valeurs remplacées par défaut, objets sautés, ressources écartées
};

// Remplace `out` par le document lu. Tolérant (valeurs absentes = défauts du modèle, valeur mal formée
// remplacée par le défaut avec un avertissement) ; refuse un « format » absent, étranger ou d'une autre
// version majeure, sans toucher à `out`. Une ressource au nom invalide est écartée avant tout appel au
// chargeur ; une ressource dont les octets ne redonnent pas la clé est écartée ; une ressource déclarée est
// gardée, qu'un objet connu la cite ou non.
//
// Compatibilité ascendante (L2-C8, § 1 de l'amendement 1 du cadrage) : ce qu'une version plus récente a écrit
// et que celle-ci ne connaît pas n'est jamais perdu. Une clé inconnue (document, page, objet, et dans leurs
// sous-objets) est gardée sans avertissement (`unknownKeys`) et réécrite telle quelle ; un objet de type
// inconnu est gardé tel quel à sa place (core::UnknownObject : son JSON entier), avec un avertissement, et
// réécrit à l'identique. Rien de ce qu'il cite n'est chargé : ses textes restent des données.
JsonReadResult documentFromJson(const QJsonObject& json, const ResourceLoader& loadResource,
                                core::Document& out);

} // namespace actcraft::formats
