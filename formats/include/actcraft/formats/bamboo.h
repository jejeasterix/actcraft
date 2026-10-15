#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>

#include <cstddef>
#include <optional>
#include <vector>

namespace actcraft::formats::flipchart {

// Une entrée décompressée de plus de 512 Mio est refusée (fichier non fiable).
inline constexpr qint64 kMaxEntrySize = 512ll * 1024 * 1024;

struct BambooEntry
{
    QString name;
    quint64 size = 0; // taille décompressée annoncée
    bool compressed = false;
    QByteArray raw; // octets stockés
};

// Conteneur « Bamboo » d'un .flipchart (format-flipchart.md § 2) : entrées nommées, compressées par zlib avec
// l'en-tête de qCompress. Les pages (*.fpg) sont des conteneurs imbriqués : parse() sur leurs octets.
class BambooContainer
{
public:
    static std::optional<BambooContainer> parse(const QByteArray& data, QString& error);
    QStringList names() const; // ordre du fichier
    bool contains(const QString& name) const;
    // Contenu décompressé ; nullopt et `error` si absente, trop grosse, illisible ou d'une autre taille que
    // l'annoncée.
    std::optional<QByteArray> read(const QString& name, QString& error) const;
    quint16 version() const noexcept;

private:
    std::vector<BambooEntry> m_entries;
    QHash<QString, std::size_t> m_byName; // un nom en double : la dernière entrée l'emporte
    quint16 m_version = 0;
};

// Nœud de l'index « culm » (§ 3) : la hiérarchie fait foi, pas la liste des entrées.
struct IndexNode
{
    quint32 nodeId = 0;
    quint16 type = 0;
    quint32 storageId = 0;
    std::vector<IndexNode> children;
    std::vector<quint32> refs;   // balises 'S' : membres d'un groupe, rangés ailleurs dans l'arbre
    QString storageName() const; // « <storageId>.io »
};

std::optional<IndexNode> parseIndex(const QByteArray& data, QString& error);

} // namespace actcraft::formats::flipchart
