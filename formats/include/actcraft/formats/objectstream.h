#pragma once

#include <actcraft/core/model.h>

#include <QByteArray>
#include <QString>
#include <QStringList>

#include <map>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace actcraft::formats::flipchart {

struct SizeF
{
    double width = 0;
    double height = 0;
};

struct Color
{
    quint8 r = 0;
    quint8 g = 0;
    quint8 b = 0;
    quint8 a = 255;
};

// Référence vers un nœud de l'arbre (sous-type 9) ; (0, 0, 0) = nulle.
struct Pointer
{
    quint32 nodeId = 0;
    quint32 storageId = 0;
    quint16 type = 0;
    bool isNull() const noexcept { return nodeId == 0 && storageId == 0; }
};

// Entrée du registre de ressources (sous-type 3).
struct ResourceItem
{
    quint32 id = 0;
    QString entry;    // entrée qui porte l'objet ressource, ex. « 165.ir »
    QString dataFile; // fichier de données brutes, ex. « 115.png » ; vide si aucun
    QString digest;
    quint32 refCount = 0;
};

// Valeur non décodée : octets bruts conservés (type Qt 4, sous-type éventuel).
struct RawValue
{
    quint16 qtype = 0;
    quint16 subtype = 0;
    QByteArray data;
};

struct StreamObject;
using ObjectPtr = std::shared_ptr<const StreamObject>;
using Polyline = std::vector<core::PointF>;
struct Value;
using ValueList = std::vector<Value>;

// Valeur d'une propriété (format-flipchart.md § 4.3). QRect et QSize sont rendus en doubles (RectF, SizeF) ;
// QTime et QDateTime en texte ISO.
struct Value
{
    std::variant<std::monostate, bool, qint32, quint32, double, QString, QStringList, QByteArray,
                 core::PointF, SizeF, core::RectF, Color, Polyline, std::vector<Polyline>,
                 std::vector<ResourceItem>, Pointer, ObjectPtr, std::vector<ObjectPtr>, ValueList, RawValue>
        data;
};

// Objet générique d'un flux .io / .ir (§ 4.1) : propriétés par numéro (les noms sont l'affaire de L2-T9).
struct StreamObject
{
    quint16 type = 0;
    quint32 storageId = 0;
    std::optional<quint32> nodeId; // 'R' : nœud propriétaire (objets de premier niveau)
    std::map<quint16, Value> properties;

    const Value* find(quint16 id) const;
};

// Décode un fichier .io / .ir (en-tête « Bamboo » u16 1 u16 7, puis un objet). nullopt et `error` si
// l'en-tête ou l'objet racine est illisible ; une propriété illisible devient RawValue sans faire échouer
// l'objet.
std::optional<StreamObject> parseObjectStream(const QByteArray& data, QString& error);

// Accesseurs tolérants : nullopt (ou vide) si la valeur est absente ou d'un autre type.
std::optional<double> asNumber(const Value* v); // booléen, entier ou réel
std::optional<bool> asBool(const Value* v);     // booléen ou entier
QString asString(const Value* v);               // chaîne nulle si absente
std::optional<core::PointF> asPoint(const Value* v);
std::optional<SizeF> asSize(const Value* v);
std::optional<core::RectF> asRect(const Value* v); // QRectF, QRect ou AsRectF
std::optional<Color> asColor(const Value* v);
std::optional<Pointer> asPointer(const Value* v);
const StreamObject* asObject(const Value* v);
std::vector<const StreamObject*> asObjects(const Value* v);
Polyline asPolyline(const Value* v);
std::vector<Polyline> asPolylines(const Value* v);
std::vector<ResourceItem> asResourceItems(const Value* v);

} // namespace actcraft::formats::flipchart
