#pragma once

// Écrivain de paperboards .flipchart synthétiques pour les tests (DA4, décision 19). Écrit d'après notre
// documentation du format (REA - Activinspire/docs/re/format-flipchart.md), jamais à partir de fichiers réels
// : aucun octet de samples/ n'est copié ici. Complété par L2-T8 (flux objet) et L2-T9 (paperboard complet).

#include <QByteArray>
#include <QString>
#include <QtEndian>

#include <cstring>
#include <utility>
#include <vector>

namespace fctest {

// Octets petit-boutistes, à la suite.
class Bytes
{
public:
    Bytes& u8(quint8 v)
    {
        m_data.append(static_cast<char>(v));
        return *this;
    }
    Bytes& u16(quint16 v) { return put(qToLittleEndian(v)); }
    Bytes& u32(quint32 v) { return put(qToLittleEndian(v)); }
    Bytes& u64(quint64 v) { return put(qToLittleEndian(v)); }
    Bytes& i32(qint32 v) { return put(qToLittleEndian(v)); }
    Bytes& f64(double v)
    {
        quint64 bits = 0;
        std::memcpy(&bits, &v, sizeof bits);
        return u64(bits);
    }
    Bytes& tag(char c)
    {
        m_data.append(c);
        return *this;
    }
    Bytes& raw(const QByteArray& bytes)
    {
        m_data.append(bytes);
        return *this;
    }
    // QString : u32 = longueur en octets, puis UTF-16LE.
    Bytes& qstr(const QString& s)
    {
        u32(static_cast<quint32>(s.size() * 2));
        for (const QChar c : s) {
            u16(c.unicode());
        }
        return *this;
    }
    Bytes& nullQstr() { return u32(0xFFFFFFFFu); }
    // Chaîne d'une variante : u16 = 1 puis QString.
    Bytes& str16(const QString& s)
    {
        u16(1);
        return qstr(s);
    }
    const QByteArray& data() const noexcept { return m_data; }

private:
    template <typename T> Bytes& put(T v)
    {
        m_data.append(reinterpret_cast<const char*>(&v), static_cast<qsizetype>(sizeof v));
        return *this;
    }
    QByteArray m_data;
};

struct Entry
{
    Entry(QString n, QByteArray d, bool c = true) : name(std::move(n)), data(std::move(d)), compress(c) {}
    QString name;
    QByteArray data;
    bool compress;
};

// Conteneur « Bamboo » (§ 2) ; données compressées par qCompress (u32 gros-boutiste + zlib), comme le format.
inline QByteArray container(const std::vector<Entry>& entries)
{
    Bytes b;
    b.raw(QByteArray(" Bamboo ", 8))
        .u16(7)
        .qstr(QStringLiteral("1.0"))
        .raw(QByteArray("\0\0\0\0\0\0\x01\0", 8));
    for (const Entry& e : entries) {
        b.qstr(e.name);
        for (int i = 0; i < 3; ++i) {
            b.qstr(QStringLiteral("lun. 1. janv. 00:00:00 2024"));
        }
        const QByteArray stored = e.compress ? qCompress(e.data) : e.data;
        b.u64(static_cast<quint64>(e.data.size()))
            .u8(e.compress ? 1 : 0)
            .u64(static_cast<quint64>(stored.size()));
        b.raw(stored);
    }
    return b.data();
}

struct Node
{
    Node(quint32 i, quint16 t, quint32 s, std::vector<Node> c = {}, std::vector<quint32> r = {})
        : id(i), type(t), storage(s), children(std::move(c)), refs(std::move(r))
    {}
    quint32 id;
    quint16 type;
    quint32 storage;
    std::vector<Node> children;
    std::vector<quint32> refs;
};

inline void writeNode(Bytes& b, const Node& n)
{
    b.tag('N').u32(n.id).u16(n.type).u32(n.storage);
    for (const quint32 ref : n.refs) {
        b.tag('S').u32(ref).tag('s');
    }
    for (const Node& child : n.children) {
        writeNode(b, child);
    }
    b.tag('n');
}

// Index « culm » (§ 3).
inline QByteArray index(const Node& root)
{
    Bytes b;
    b.raw(QByteArray("# culm #", 8)).u16(1).u16(7);
    writeNode(b, root);
    return b.data();
}

} // namespace fctest
