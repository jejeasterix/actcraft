#pragma once

// Écrivain de paperboards .flipchart synthétiques pour les tests (DA4, décision 19). Écrit d'après notre
// documentation du format (REA - Activinspire/docs/re/format-flipchart.md), jamais à partir de fichiers réels
// : aucun octet de samples/ n'est copié ici. Complété par L2-T8 (flux objet) et L2-T9 (paperboard complet).

#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QImage>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QtEndian>

#include <cstring>
#include <optional>
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

// Entrée du registre de ressources (sous-type 3).
struct Res
{
    Res(quint32 i, QString e, QString f, QString d = QStringLiteral("unknown"), quint32 r = 1)
        : id(i), entry(std::move(e)), dataFile(std::move(f)), digest(std::move(d)), refCount(r)
    {}
    quint32 id;
    QString entry;
    QString dataFile;
    QString digest;
    quint32 refCount;
};

// Objet du flux (§ 4.1) : 'O' type stockage 'M' n tailles enregistrements 'p' ['R' nœud 'r'] 'o'. Les
// propriétés sont des numéros : l'écrivain ne connaît pas leurs noms.
class Obj
{
public:
    Obj(quint16 type, quint32 storage) : m_type(type), m_storage(storage) {}

    Obj& boolean(quint16 id, bool v) { return add(id, Bytes().u16(1).u8(v ? 1 : 0)); }
    Obj& integer(quint16 id, qint32 v) { return add(id, Bytes().u16(2).i32(v)); }
    Obj& uinteger(quint16 id, quint32 v) { return add(id, Bytes().u16(3).u32(v)); }
    Obj& real(quint16 id, double v) { return add(id, Bytes().u16(6).f64(v)); }
    Obj& string(quint16 id, const QString& v) { return add(id, Bytes().u16(10).str16(v)); }
    Obj& stringList(quint16 id, const QStringList& v)
    {
        Bytes b;
        b.u16(11).u16(1).i32(static_cast<qint32>(v.size()));
        for (const QString& s : v) {
            b.qstr(s);
        }
        return add(id, b);
    }
    Obj& bytes(quint16 id, const QByteArray& v)
    {
        return add(id, Bytes().u16(12).u32(static_cast<quint32>(v.size())).raw(v));
    }
    Obj& rectF(quint16 id, double x, double y, double w, double h)
    {
        return add(id, Bytes().u16(20).f64(x).f64(y).f64(w).f64(h));
    }
    Obj& size(quint16 id, qint32 w, qint32 h) { return add(id, Bytes().u16(21).i32(w).i32(h)); }
    Obj& sizeF(quint16 id, double w, double h) { return add(id, Bytes().u16(22).f64(w).f64(h)); }
    Obj& pointF(quint16 id, double x, double y) { return add(id, Bytes().u16(26).f64(x).f64(y)); }
    Obj& color(quint16 id, quint8 r, quint8 g, quint8 b, quint8 a = 255)
    {
        return add(id, Bytes().u16(67).u8(r).u8(g).u8(b).u8(a));
    }
    Obj& points(quint16 id, const std::vector<QPointF>& v)
    {
        Bytes b;
        b.u16(127).u16(1).i32(static_cast<qint32>(v.size()));
        appendPoints(b, v);
        return add(id, b);
    }
    Obj& polygons(quint16 id, const std::vector<std::vector<QPointF>>& v)
    {
        Bytes b;
        b.u16(127).u16(2).i32(static_cast<qint32>(v.size()));
        for (const auto& poly : v) {
            b.i32(static_cast<qint32>(poly.size()));
            appendPoints(b, poly);
        }
        return add(id, b);
    }
    Obj& resources(quint16 id, const std::vector<Res>& v)
    {
        Bytes b;
        b.u16(127).u16(3).i32(static_cast<qint32>(v.size()));
        for (const Res& r : v) {
            b.u32(r.id).str16(r.entry).str16(r.dataFile).str16(r.digest).u32(r.refCount);
        }
        return add(id, b);
    }
    // Objet embarqué (sous-types 4, 6, 7, 8, 12, 14, 18).
    Obj& object(quint16 id, quint16 subtype, const Obj& inner)
    {
        return add(id, Bytes().u16(127).u16(subtype).raw(inner.body()));
    }
    Obj& userRect(quint16 id, double x, double y, double w, double h)
    {
        return add(id, Bytes().u16(127).u16(5).f64(x).f64(y).f64(w).f64(h));
    }
    Obj& pointer(quint16 id, quint32 node, quint32 storage, quint16 type)
    {
        return add(id, Bytes().u16(127).u16(9).u32(node).u32(storage).u16(type));
    }
    Obj& runs(quint16 id, const std::vector<Obj>& v)
    {
        Bytes b;
        b.u16(127).u16(10).i32(static_cast<qint32>(v.size()));
        for (const Obj& o : v) {
            b.raw(o.body());
        }
        return add(id, b);
    }
    // Variante quelconque (type et octets donnés) : types inconnus, valeurs illisibles.
    Obj& raw(quint16 id, const QByteArray& variant) { return add(id, Bytes().raw(variant)); }
    Obj& owner(quint32 node)
    {
        m_owner = node;
        return *this;
    }

    // L'objet sans en-tête de flux (objet embarqué).
    QByteArray body() const
    {
        std::vector<QByteArray> records;
        for (std::size_t i = 0; i < m_props.size(); ++i) {
            Bytes r;
            r.tag(i == 0 ? 'm' : 'p').tag('P').u16(m_props[i].first).raw(m_props[i].second);
            records.push_back(r.data());
        }
        Bytes b;
        b.tag('O').u16(m_type).u32(m_storage).tag('M').u32(static_cast<quint32>(records.size()));
        for (const QByteArray& r : records) {
            b.u32(static_cast<quint32>(r.size()));
        }
        for (const QByteArray& r : records) {
            b.raw(r);
        }
        b.tag('p');
        if (m_owner) {
            b.tag('R').u32(*m_owner).tag('r');
        }
        b.tag('o');
        return b.data();
    }
    // Fichier .io / .ir : en-tête de flux " Bamboo " u16 1 u16 7, puis l'objet.
    QByteArray stream() const
    {
        Bytes b;
        b.raw(QByteArray(" Bamboo ", 8)).u16(1).u16(7).raw(body());
        return b.data();
    }

private:
    static void appendPoints(Bytes& b, const std::vector<QPointF>& v)
    {
        Bytes doubles;
        for (const QPointF& p : v) {
            doubles.f64(p.x()).f64(p.y());
        }
        b.u32(static_cast<quint32>(doubles.data().size())).raw(doubles.data());
    }
    Obj& add(quint16 id, const Bytes& variant)
    {
        m_props.emplace_back(id, variant.data());
        return *this;
    }

    quint16 m_type;
    quint32 m_storage;
    std::optional<quint32> m_owner;
    std::vector<std::pair<quint16, QByteArray>> m_props;
};

// PNG w × h d'une couleur, produit par Qt (tests et outils de preuve seulement).
inline QByteArray pngBytes(int w, int h, QRgb color)
{
    QImage image(w, h, QImage::Format_ARGB32);
    image.fill(color);
    QByteArray out;
    QBuffer buffer(&out);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return out;
}

inline const QByteArray& sampleMp3()
{
    static const QByteArray bytes("ID3\x04\0\0\0\0\0\0son", 13);
    return bytes;
}

// Paperboard synthétique de deux pages qui passe par chaque branche du lecteur (L2-T9). Numéros de types : 1
// paperboard, 2 page, 3 annotation, 4 ressource d'annotation, 5 ligne, 6 ressource de ligne, 7 forme, 8
// ressource de forme, 9 image, 10 ressource d'image, 11 texte, 12 ressource de texte, 13 grille, 14 niveau de
// grille, 15 transformation, 16 registre, 18 groupe, 20 encre, 28 action sur objet, 29 fond, 30 ressource de
// média, 31 ligne de texte, 52 connecteur, 53 étiquette, 253 média, 460 formule (non lue). `thumbnails` :
// octets des miniatures « 103.thm » et « 203.thm » (aucune si vide).
inline QByteArray sampleFlipchart(const std::vector<QByteArray>& thumbnails = {})
{
    const QByteArray png = pngBytes(2, 2, qRgb(255, 0, 0));
    const auto translate = [](double x, double y) { return Obj(15, 0).pointF(2, x, y); };

    const QByteArray page1 = container({
        Entry(QStringLiteral("103.in"),
              index(Node(103, 2, 102,
                         {Node(104, 29, 105), Node(106, 13, 107, {Node(108, 14, 109)}), Node(110, 3, 111),
                          Node(112, 7, 113), Node(114, 28, 115), Node(116, 11, 117),
                          Node(118, 18, 119, {}, {110, 112})}))),
        Entry(QStringLiteral("102.io"), Obj(2, 102)
                                            .string(2, QStringLiteral("Page un"))
                                            .string(5, QStringLiteral("Notes 1"))
                                            .sizeF(6, 1024, 768)
                                            .boolean(17, true)
                                            .owner(103)
                                            .stream()),
        Entry(QStringLiteral("105.io"),
              Obj(29, 105)
                  .integer(2, 0)
                  .object(3, 6,
                          Obj(20, 0)
                              .integer(8, 1)
                              .color(9, 200, 220, 255)
                              .integer(14, 1)
                              .color(13, 255, 255, 255))
                  .stream()),
        Entry(QStringLiteral("107.io"), Obj(13, 107).boolean(16, true).real(14, 1).stream()),
        Entry(QStringLiteral("109.io"),
              Obj(14, 109).real(7, 50).real(8, 25).color(4, 0, 0, 255).integer(2, 3).integer(3, 1).stream()),
        Entry(
            QStringLiteral("111.io"),
            Obj(3, 111)
                .string(4, QStringLiteral("Stylo1"))
                .integer(8, 2)
                .real(9, 1)
                .object(2, 7, Obj(15, 0).pointF(2, 100, 100).real(3, 90).pointF(4, 2, 1))
                .object(10, 6, Obj(20, 0).integer(2, 1).real(3, 20).color(7, 255, 255, 0, 128).integer(5, 32))
                .integer(39, 310)
                .owner(110)
                .stream()),
        Entry(QStringLiteral("113.io"),
              Obj(7, 113)
                  .string(4, QStringLiteral("Forme1"))
                  .integer(8, 1)
                  .real(9, 2)
                  .object(2, 7, translate(300, 200))
                  .userRect(7, -3, -3, 86, 66)
                  .object(
                      10,
                      6,
                      Obj(20, 0).integer(4, 2).real(3, 6).color(7, 0, 0, 0).integer(8, 1).color(9, 0, 128, 0))
                  .integer(38, 320)
                  .integer(39, 2)
                  .object(34, 12, Obj(53, 0).string(2, QStringLiteral("Étiquette")))
                  .owner(112)
                  .stream()),
        Entry(QStringLiteral("115.io"),
              Obj(28, 115).pointer(2, 116, 117, 11).integer(3, 42).integer(5, 184).stream()),
        Entry(QStringLiteral("117.io"), Obj(11, 117)
                                            .integer(8, 1)
                                            .real(9, 3)
                                            .object(2, 7, translate(500, 500))
                                            .userRect(7, 0, 0, 90, 36)
                                            .boolean(18, true)
                                            .integer(38, 330)
                                            .owner(116)
                                            .stream()),
        Entry(QStringLiteral("119.io"), Obj(18, 119).owner(118).stream()),
    });

    const QByteArray page2 = container({
        Entry(QStringLiteral("203.in"),
              index(Node(203, 2, 202,
                         {Node(204, 29, 205), Node(208, 9, 209), Node(210, 5, 211), Node(212, 253, 213),
                          Node(214, 52, 215), Node(216, 460, 217)}))),
        Entry(QStringLiteral("202.io"), Obj(2, 202).sizeF(6, 800, 600).stream()),
        Entry(QStringLiteral("205.io"), Obj(29, 205).integer(2, 1).pointer(4, 0, 207, 9).stream()),
        Entry(QStringLiteral("207.io"), Obj(9, 207).integer(38, 300).stream()),
        Entry(QStringLiteral("209.io"), Obj(9, 209)
                                            .integer(8, 1)
                                            .userRect(7, 0, 0, 2, 2)
                                            .object(2, 7, Obj(15, 0).pointF(2, 10, 10).pointF(4, 50, 50))
                                            .integer(38, 300)
                                            .boolean(39, true)
                                            .color(40, 255, 255, 255)
                                            .stream()),
        Entry(QStringLiteral("211.io"),
              Obj(5, 211)
                  .integer(8, 1)
                  .object(10, 6, Obj(20, 0).real(3, 3).color(7, 0, 0, 255).integer(4, 1))
                  .integer(38, 340)
                  .stream()),
        Entry(QStringLiteral("213.io"), Obj(253, 213)
                                            .integer(8, 1)
                                            .userRect(7, 0, 0, 64, 48)
                                            .integer(38, 300)
                                            .integer(45, 350)
                                            .boolean(41, true)
                                            .stream()),
        Entry(QStringLiteral("215.io"), Obj(52, 215)
                                            .pointF(39, 0, 0)
                                            .pointF(40, 100, 100)
                                            .pointer(34, 208, 209, 9)
                                            .pointer(35, 210, 211, 5)
                                            .stream()),
        Entry(QStringLiteral("217.io"), Obj(460, 217).stream()),
    });

    const Obj registry = Obj(16, 0)
                             .resources(5, {Res(310, QStringLiteral("310.ir"), QString())})
                             .resources(6, {Res(300, QStringLiteral("300.ir"), QStringLiteral("301.png"))})
                             .resources(7, {Res(340, QStringLiteral("340.ir"), QString())})
                             .resources(8, {Res(320, QStringLiteral("320.ir"), QString())})
                             .resources(9, {Res(330, QStringLiteral("330.ir"), QString())})
                             .resources(11, {Res(350, QStringLiteral("350.ir"), QStringLiteral("351.mp3"))});
    const Obj run = Obj(31, 0)
                        .rectF(2, 0, 0, 90, 36)
                        .pointF(3, 0, 30)
                        .color(5, 255, 0, 0)
                        .string(6, QStringLiteral("Bonjour"))
                        .string(7, QStringLiteral("Arial"))
                        .real(8, 24)
                        .integer(9, 75);
    std::vector<Entry> root{
        Entry(QStringLiteral("101.in"), index(Node(101, 1, 100, {Node(103, 2, 102), Node(203, 2, 202)}))),
        Entry(QStringLiteral("100.io"), Obj(1, 100)
                                            .string(12, QStringLiteral("Essai synthétique"))
                                            .sizeF(18, 1024, 768)
                                            .object(7, 4, registry)
                                            .owner(101)
                                            .stream()),
        Entry(QStringLiteral("103.fpg"), page1),
        Entry(QStringLiteral("203.fpg"), page2),
        Entry(QStringLiteral("300.ir"), Obj(10, 300).size(5, 2, 2).stream()),
        Entry(QStringLiteral("301.png"), png, false),
        Entry(QStringLiteral("310.ir"),
              Obj(4, 310).points(4, {QPointF(0, 0), QPointF(100, 0), QPointF(100, 50)}).stream()),
        Entry(QStringLiteral("320.ir"),
              Obj(8, 320)
                  .polygons(4, {{QPointF(0, 0), QPointF(80, 0), QPointF(80, 60), QPointF(0, 60)},
                                {QPointF(20, 20), QPointF(60, 20), QPointF(40, 50)}})
                  .stream()),
        Entry(QStringLiteral("330.ir"),
              Obj(12, 330).runs(4, {run}).string(5, QStringLiteral("<p>Bonjour</p>")).integer(6, 2).stream()),
        Entry(QStringLiteral("340.ir"), Obj(6, 340).points(4, {QPointF(0, 0), QPointF(200, 100)}).stream()),
        Entry(QStringLiteral("350.ir"), Obj(30, 350).string(3, QStringLiteral("351.mp3")).stream()),
        Entry(QStringLiteral("351.mp3"), sampleMp3(), false),
    };
    if (thumbnails.size() >= 1) {
        root.emplace_back(QStringLiteral("103.thm"), thumbnails[0], false);
    }
    if (thumbnails.size() >= 2) {
        root.emplace_back(QStringLiteral("203.thm"), thumbnails[1], false);
    }
    return container(root);
}

} // namespace fctest
