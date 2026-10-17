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
    Obj& floatRect(quint16 id, double x, double y, double w, double h)
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
    Obj& argb(quint16 id, QRgb c)
    {
        return color(id, static_cast<quint8>(qRed(c)), static_cast<quint8>(qGreen(c)),
                     static_cast<quint8>(qBlue(c)), static_cast<quint8>(qAlpha(c)));
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
    const auto offsetBy = [](double x, double y) { return Obj(15, 0).pointF(2, x, y); };

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
                  .object(2, 7, offsetBy(300, 200))
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
                                            .object(2, 7, offsetBy(500, 500))
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
                        .floatRect(2, 0, 0, 90, 36)
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

// Oracle synthétique (L2-C4) : paperboards qui exercent ce que les paperboards réels n'exercent pas, lus par
// notre lecteur ET par la référence Python (tst_formats_oracle_synth). Mêmes numéros que sampleFlipchart ; en
// plus : 21 origine de rotation, 469 texte vertical (non lu). Les objets imbriqués portent un nom en
// « enfant… » ou « petit-enfant… », ceux d'un objet non lu « …non-lu… » : le test vérifie ainsi, dans
// l'export de la référence, que chaque cas est bien exercé.
namespace synth {

inline Entry io(quint32 storage, const Obj& o)
{
    return Entry(QStringLiteral("%1.io").arg(storage), o.stream());
}

inline Entry ir(quint32 rid, const Obj& o)
{
    return Entry(QStringLiteral("%1.ir").arg(rid), o.stream());
}

inline Obj place(double x, double y, double degrees = 0, double sx = 1, double sy = 1, bool mirrorX = false,
                 bool mirrorY = false)
{
    Obj t(15, 0);
    t.pointF(2, x, y).real(3, degrees).pointF(4, sx, sy).boolean(5, mirrorX).boolean(6, mirrorY);
    return t;
}

inline Obj pivot(double x, double y)
{
    Obj o(21, 0);
    o.pointF(2, x, y);
    return o;
}

// Encre : largeur, motif du trait (valeurs de Qt::PenStyle), bouts, jointures, couleur.
inline Obj ink(double width, qint32 dash, qint32 cap, qint32 join, QRgb color)
{
    Obj o(20, 0);
    o.real(3, width).integer(4, dash).integer(5, cap).integer(6, join).argb(7, color);
    return o;
}

// Remplissage ajouté à une encre : 0 aucun, 1 plein, 2 à 14 motifs, 25 dégradé ; dégradé 0 à 5 ; fond des
// motifs.
inline Obj filled(Obj o, qint32 style, QRgb color, QRgb color2, qint32 gradient, bool opaque = false,
                  QRgb hatch = 0xFF000000u)
{
    o.integer(8, style)
        .argb(9, color)
        .argb(13, color2)
        .integer(14, gradient)
        .integer(10, opaque ? 1 : 0)
        .argb(11, hatch);
    return o;
}

// Propriétés communes : nom, couche, profondeur, transformation.
inline Obj item(quint16 kind, quint32 storage, const char* name, qint32 layer, double z, const Obj& placement)
{
    Obj o(kind, storage);
    o.string(4, QString::fromLatin1(name)).integer(8, layer).real(9, z).object(2, 7, placement);
    return o;
}

inline std::vector<QPointF> box(double w, double h)
{
    return {QPointF(0, 0), QPointF(w, 0), QPointF(w, h), QPointF(0, h)};
}

// Forme rectangulaire à points portés par l'objet.
inline Obj rectangle(quint32 storage, const char* name, qint32 layer, double z, const Obj& placement,
                     const Obj& pen, double w = 60, double h = 40)
{
    Obj o = item(7, storage, name, layer, z, placement);
    o.userRect(7, 0, 0, w, h).object(10, 6, pen).points(49, box(w, h));
    return o;
}

// Une page : nœud `node`, objet de page rangé sous node − 1, arbre et objets de son conteneur.
struct Sheet
{
    explicit Sheet(quint32 n) : node(n), props(2, n - 1) {}
    quint32 node;
    Obj props;
    std::vector<Node> tree;
    std::vector<Entry> entries;
};

inline QByteArray paperboard(const QString& title, const std::vector<Sheet>& sheets,
                             const std::vector<Entry>& resources = {},
                             const std::optional<Obj>& registry = std::nullopt)
{
    std::vector<Node> pages;
    std::vector<Entry> all;
    std::vector<Entry> fpg;
    for (const Sheet& s : sheets) {
        pages.emplace_back(s.node, 2, s.node - 1);
        std::vector<Entry> inner{
            Entry(QStringLiteral("%1.in").arg(s.node), index(Node(s.node, 2, s.node - 1, s.tree))),
            io(s.node - 1, s.props)};
        inner.insert(inner.end(), s.entries.begin(), s.entries.end());
        fpg.emplace_back(QStringLiteral("%1.fpg").arg(s.node), container(inner));
    }
    Obj doc(1, 100);
    doc.string(12, title).sizeF(18, 1024, 768);
    if (registry) {
        doc.object(7, 4, *registry);
    }
    doc.owner(101);
    all.emplace_back(QStringLiteral("101.in"), index(Node(101, 1, 100, pages)));
    all.push_back(io(100, doc));
    all.insert(all.end(), fpg.begin(), fpg.end());
    all.insert(all.end(), resources.begin(), resources.end());
    return container(all);
}

// Lignes (points, extrémités, motifs) et connecteurs (extrémités, accroches, étiquette).
inline QByteArray linesAndConnectors()
{
    const auto line = [](quint32 storage, const char* name, double z, const Obj& placement, const Obj& pen,
                         qint32 rid) {
        Obj o = item(5, storage, name, 1, z, placement);
        o.object(10, 6, pen).integer(38, rid).integer(39, 1).integer(40, 2);
        return io(storage, o);
    };
    const auto connector = [](quint32 storage, const char* name, qint32 layer, double z) {
        return item(52, storage, name, layer, z, place(0, 0));
    };
    Sheet s(1003);
    s.tree = {Node(1010, 7, 1011),  Node(1012, 7, 1013), Node(1014, 5, 1015), Node(1016, 5, 1017),
              Node(1018, 5, 1019),  Node(1020, 5, 1021), Node(1022, 5, 1023), Node(1024, 52, 1025),
              Node(1026, 52, 1027), Node(1028, 52, 1029)};
    Obj label(53, 0);
    label.string(2, QStringLiteral("Lien"));
    s.entries = {
        io(1011, rectangle(1011, "A", 1, 1, place(100, 100),
                           filled(ink(1, 1, 32, 128, 0xFF000000u), 1, 0xFFFFCC00u, 0xFF000000u, 0), 50, 50)),
        io(1013, rectangle(1013, "B", 1, 2, place(400, 300), ink(2, 1, 32, 128, 0xFF0000FFu), 50, 50)),
        line(1015, "L1 tirets", 3, place(10, 20, 30), ink(3, 2, 0, 0, 0xFFFF0000u), 2001),
        line(1017, "L2 points", 4, place(50, 400, 0, 2, 0.5), ink(2, 3, 16, 64, 0xFF00AA00u), 2002),
        line(1019, "L3 sans trait", 5, place(0, 0), ink(4, 0, 32, 128, 0xFF0000FFu), 2003),
        line(1021, "L4 largeur 0", 6, place(5, 5), ink(0, 1, 32, 128, 0xFF000000u), 2002),
        line(1023, "L5 motif inconnu", 7, place(7, 9, -45), ink(1.5, 9, 7, 9, 0x80102030u), 2001),
        io(1025, connector(1025, "K1", 1, 8)
                     .object(10, 6, ink(2, 4, 16, 0, 0xFF884400u))
                     .pointF(39, 125, 125)
                     .pointF(40, 425, 325)
                     .pointer(34, 1010, 1011, 7)
                     .pointer(35, 1012, 1013, 7)
                     .object(33, 12, label)
                     .integer(36, 1)),
        io(1027, connector(1027, "K2", 1, 9)
                     .object(10, 6, ink(1, 5, 32, 64, 0xFF000000u))
                     .pointF(39, 10, 20)
                     .pointF(40, 30, 40)
                     .pointer(34, 1014, 1015, 5)
                     .pointer(35, 0, 0, 0)),
        io(1029, connector(1029, "K3", 2, 1).pointF(39, 1, 2).pointF(40, 3, 4)),
    };
    return paperboard(QStringLiteral("Lignes et connecteurs"), {s},
                      {ir(2001, Obj(6, 2001).points(4, {QPointF(0, 0), QPointF(100, 0), QPointF(100, 50)})),
                       ir(2002, Obj(6, 2002).points(4, {QPointF(-5, 3), QPointF(40, 80)})),
                       ir(2003, Obj(6, 2003).points(4, {QPointF(0, 0), QPointF(10, 10), QPointF(20, 0),
                                                        QPointF(30, 10), QPointF(40, 0)}))});
}

// Remplissages plein, motifs (fond opaque ou non), dégradés 0 à 5 ; plumes 0 à 5 ; bouts, jointures ;
// miroirs.
inline QByteArray fillsAndPens()
{
    constexpr QRgb black = 0xFF000000u;
    Sheet s(1003);
    std::vector<std::pair<Obj, Obj>> shapes{
        {place(20, 20), filled(ink(2, 0, 32, 128, black), 1, 0xFF3366CCu, black, 0)},
        {place(100, 20, 15, 1, 1, true, false),
         filled(ink(1, 1, 32, 128, black), 2, 0xFFFF0000u, black, 0, true, 0xFFEEEEEEu)},
        {place(200, 20, 0, 1.5, 1, false, true),
         filled(ink(2, 2, 16, 64, 0xFF00FF00u), 9, 0xFF0000FFu, black, 0)},
        {place(300, 20, 90), filled(ink(3, 3, 0, 0, black), 14, 0xFF00FFFFu, black, 0, true, 0x80FFFFFFu)},
        {place(20, 120), filled(ink(1, 4, 32, 0, black), 25, 0xFFFFFFFFu, 0xFF000080u, 0)},
        {place(100, 120, 0, 1, 1, true, true),
         filled(ink(1, 5, 16, 128, black), 1, 0xFFFF8800u, 0xFF0088FFu, 2)},
        {place(200, 120, 10, 0.5, 2), filled(ink(1, 1, 0, 64, black), 1, 0xFF112233u, 0xFF445566u, 3)},
        {place(300, 120), filled(ink(1, 1, 32, 128, black), 25, 0xFF778899u, 0xFFAABBCCu, 4)},
        {place(20, 220), filled(ink(1, 1, 32, 128, black), 1, 0xFFFFFF00u, 0xFF00FF00u, 5)},
        {place(100, 220), filled(ink(1, 1, 32, 128, black), 25, 0xFF101010u, 0xFF202020u, 7)},
        {place(200, 220), filled(ink(1, 1, 32, 128, black), 15, 0xFF00AA00u, black, 0)},
        {place(300, 220), filled(ink(1, 1, 32, 128, black), 0, 0xFF00AA00u, 0xFFAA0000u, 3)},
    };
    for (std::size_t i = 0; i < shapes.size(); ++i) {
        const quint32 node = 1010 + 2 * static_cast<quint32>(i);
        s.tree.emplace_back(node, 7, node + 1);
        Obj o = rectangle(node + 1, "F", 1, static_cast<double>(i), shapes[i].first, shapes[i].second);
        if (i == 1) {
            o.object(12, 8, pivot(30, 20));
        }
        if (i == 10) {
            o.boolean(42, true).integer(39, 2);
        }
        s.entries.push_back(io(node + 1, o));
    }
    // Forme à polygones portés par une ressource.
    s.tree.emplace_back(1100, 7, 1101);
    Obj withPaths = item(7, 1101, "chemins", 2, 1, place(400, 400, 45));
    withPaths.userRect(7, 0, 0, 80, 60)
        .object(10, 6, filled(ink(2, 2, 16, 64, black), 6, 0xFF0000FFu, black, 0, true, 0xFFFFFF00u))
        .integer(38, 2101);
    s.entries.push_back(io(1101, withPaths));
    return paperboard(QStringLiteral("Remplissages et plumes"), {s},
                      {ir(2101, Obj(8, 2101).polygons(
                                    4, {box(80, 60), {QPointF(20, 20), QPointF(60, 20), QPointF(40, 50)}}))});
}

// Fonds : dégradé, image dans le conteneur de la page, image dans le conteneur racine, dégradé sans motif,
// couleur seule ; images avec et sans couleur transparente.
inline QByteArray backgrounds()
{
    const auto fill = [](quint32 storage, qint32 source, const Obj& pen, std::optional<quint32> picture) {
        Obj o(29, storage);
        o.integer(2, source).object(3, 6, pen);
        if (picture) {
            o.pointer(4, 0, *picture, 9);
        }
        return io(storage, o);
    };
    Sheet p1(1003);
    p1.tree = {Node(1010, 29, 1011)};
    p1.entries = {fill(1011, 0, filled(Obj(20, 0), 1, 0xFFCCE0FFu, 0xFF003366u, 2), std::nullopt)};
    Sheet p2(2003);
    p2.tree = {Node(2010, 29, 2011), Node(2012, 9, 2013), Node(2014, 9, 2015)};
    Obj keyed = item(9, 2013, "image transparente", 1, 1, place(10, 10, 0, 20, 20));
    keyed.userRect(7, 0, 0, 2, 2).integer(38, 3001).boolean(39, true).argb(40, 0xFFFFFFFFu);
    Obj plain = item(9, 2015, "image", 1, 2, place(100, 10));
    plain.userRect(7, 0, 0, 3, 1).integer(38, 3002);
    p2.entries = {fill(2011, 1, Obj(20, 0), 2105), io(2105, Obj(9, 2105).integer(38, 3001)), io(2013, keyed),
                  io(2015, plain)};
    Sheet p3(3003);
    p3.tree = {Node(3010, 29, 3011)};
    p3.entries = {fill(3011, 2, filled(Obj(20, 0), 25, 0xFF00FF00u, 0xFFFF00FFu, 5), 3105)};
    Sheet p4(4003);
    p4.tree = {Node(4010, 29, 4011)};
    p4.entries = {fill(4011, 0, filled(Obj(20, 0), 0, 0xFF123456u, 0xFF654321u, 4), std::nullopt)};
    Sheet p5(5003);
    p5.props.string(2, QStringLiteral("Cinq")).string(5, QStringLiteral("Notes")).sizeF(6, 800, 600);
    p5.tree = {Node(5010, 29, 5011)};
    p5.entries = {fill(5011, 0, filled(Obj(20, 0), 1, 0xFF336699u, 0xFF000000u, 0), std::nullopt)};
    return paperboard(QStringLiteral("Fonds"), {p1, p2, p3, p4, p5},
                      {ir(3001, Obj(10, 3001).size(5, 2, 2)),
                       Entry(QStringLiteral("3001.png"), pngBytes(2, 2, qRgb(255, 0, 0)), false),
                       ir(3002, Obj(10, 3002).size(5, 3, 1)),
                       Entry(QStringLiteral("3002.png"), pngBytes(3, 1, qRgb(0, 0, 255)), false),
                       io(3105, Obj(9, 3105).integer(38, 3002))});
}

// Traits d'encre : translucidité 0, 1, 128, 255 et absente ; modes 0 à 2 et inconnu ; bouts.
inline QByteArray strokes()
{
    const auto stroke = [](quint32 storage, const char* name, double z, qint32 mode, double width, qint32 cap,
                           QRgb color, std::optional<qint32> seeThrough, qint32 rid) {
        Obj o = item(3, storage, name, 1, z, place(10 * z, 5 * z));
        Obj pen(20, 0);
        pen.integer(2, mode).real(3, width).integer(5, cap).argb(7, color);
        o.object(10, 6, pen);
        if (seeThrough) {
            o.integer(38, *seeThrough);
        }
        if (rid != 0) {
            o.integer(39, rid);
        }
        return io(storage, o);
    };
    Sheet s(1003);
    s.tree = {Node(1010, 3, 1011), Node(1012, 3, 1013), Node(1014, 3, 1015), Node(1016, 3, 1017),
              Node(1018, 3, 1019)};
    s.entries = {stroke(1011, "T opaque", 1, 0, 2, 0, 0xFF000000u, 0, 2201),
                 stroke(1013, "T translucide", 2, 1, 20, 16, 0x64FFFF00u, 128, 2202),
                 stroke(1015, "T invisible", 3, 2, 8, 32, 0xFF00FF00u, 255, 2201),
                 stroke(1017, "T sans translucidite", 4, 5, 3, 7, 0xFF0000FFu, std::nullopt, 2202),
                 stroke(1019, "T sans points", 5, 0, 1, 32, 0xFF000000u, 1, 0)};
    return paperboard(QStringLiteral("Traits"), {s},
                      {ir(2201, Obj(4, 2201).points(4, {QPointF(0, 0), QPointF(5, 5), QPointF(10, 0),
                                                        QPointF(15, 5), QPointF(20, 0)})),
                       ir(2202, Obj(4, 2202).points(4, {QPointF(1.5, 2.25), QPointF(30, 40)}))});
}

// Enfants imbriqués : S1 (couche 1) a pour enfants C (couche 2, elle-même parent de D, couche 0), E et F
// (couche 1) et une action ; puis S2 (couche 1) ; puis un groupe G (couche 1, membres S1 et S2) qui a un
// enfant H (couche 0) et une action.
inline QByteArray nesting()
{
    constexpr QRgb black = 0xFF000000u;
    Sheet s(1003);
    s.tree = {Node(1010, 7, 1011,
                   {Node(1012, 7, 1013, {Node(1014, 3, 1015)}), Node(1016, 7, 1017), Node(1018, 7, 1019),
                    Node(1020, 28, 1021)}),
              Node(1030, 7, 1031),
              Node(1040, 18, 1041, {Node(1042, 5, 1043), Node(1044, 28, 1045)}, {1010, 1030})};
    Obj d = item(3, 1015, "petit-enfant D", 0, 1, place(5, 5));
    d.object(10, 6, ink(3, 1, 32, 128, 0xFFFF0000u)).integer(39, 2301);
    Obj g = item(18, 1041, "G", 1, 3, place(0, 0));
    Obj h = item(5, 1043, "enfant H", 0, 1, place(15, 15, 45));
    h.object(10, 6, ink(2, 2, 32, 128, black)).integer(38, 2302);
    s.entries = {
        io(1011, rectangle(1011, "S1", 1, 1, place(100, 100, 30), ink(1, 1, 32, 128, black))),
        io(1013, rectangle(1013, "enfant C", 2, 1, place(10, 0, 0, 2, 1), ink(1, 1, 32, 128, black))),
        io(1015, d),
        io(1017, rectangle(1017, "enfant E", 1, 5, place(0, 20), ink(1, 1, 32, 128, black))),
        io(1019,
           rectangle(1019, "enfant F", 1, 2, place(20, 0, 0, 1, 1, true, false), ink(1, 1, 32, 128, black))),
        io(1021, Obj(28, 1021).pointer(2, 1030, 1031, 7).integer(5, 184)),
        io(1031, rectangle(1031, "S2", 1, 2, place(300, 300), ink(1, 1, 32, 128, black))),
        io(1041, g),
        io(1043, h),
        io(1045, Obj(28, 1045).pointer(2, 1012, 1013, 7).integer(5, 185))};
    return paperboard(QStringLiteral("Imbrication"), {s},
                      {ir(2301, Obj(4, 2301).points(4, {QPointF(0, 0), QPointF(8, 8)})),
                       ir(2302, Obj(6, 2302).points(4, {QPointF(0, 0), QPointF(50, 0), QPointF(50, 30)}))});
}

// Objets non lus (formule 460, texte vertical 469) avec enfants, transformés, et un connecteur qui vise l'un
// d'eux. L'ordre obtenu est déjà rangé par couche.
inline QByteArray unreadParents()
{
    constexpr QRgb black = 0xFF000000u;
    Sheet s(1003);
    s.tree = {Node(1010, 7, 1011),
              Node(1012, 460, 1013, {Node(1014, 7, 1015), Node(1016, 3, 1017), Node(1018, 28, 1019)}),
              Node(1034, 28, 1035),
              Node(1020, 469, 1021, {Node(1022, 5, 1023, {Node(1024, 7, 1025)})}),
              Node(1030, 7, 1031),
              Node(1032, 52, 1033),
              Node(1040, 460, 1041)};
    Obj c2 = item(3, 1017, "enfant-non-lu C2", 1, 1, place(3, 4));
    c2.object(10, 6, ink(2, 1, 32, 128, black)).integer(39, 2501);
    Obj l = item(5, 1023, "enfant-non-lu L", 2, 1, place(10, 10, 0, 2, 2));
    l.object(10, 6, ink(2, 3, 16, 64, black)).integer(38, 2502);
    Obj k = item(52, 1033, "K vers formule", 2, 6, place(0, 0));
    k.object(10, 6, ink(1, 1, 32, 128, black))
        .pointF(39, 0, 0)
        .pointF(40, 50, 50)
        .pointer(34, 1010, 1011, 7)
        .pointer(35, 1012, 1013, 460);
    s.entries = {
        io(1011, rectangle(1011, "A", 1, 1, place(20, 20), ink(1, 1, 32, 128, black))),
        io(1013, item(460, 1013, "formule", 1, 2, place(100, 50, 90, 1, 1)).userRect(7, 0, 0, 40, 20)),
        io(1015, rectangle(1015, "enfant-non-lu C1", 2, 1, place(5, 0), ink(1, 2, 32, 128, black))),
        io(1017, c2),
        io(1019, Obj(28, 1019).pointer(2, 1010, 1011, 7).integer(5, 184)),
        io(1021, item(469, 1021, "texte vertical", 2, 1, place(300, 300, 0, 1, 1, false, true))
                     .userRect(7, 0, 0, 30, 90)),
        io(1023, l),
        io(1025, rectangle(1025, "petit-enfant-non-lu N", 2, 1, place(0, 5, 20), ink(1, 1, 0, 0, black))),
        io(1031, rectangle(1031, "B", 2, 5, place(500, 500), ink(1, 1, 32, 128, black))),
        io(1033, k)};
    // Action juste après la formule : la référence la rattache à la formule. Formule de couche 7 (hors 0..2).
    s.entries.push_back(io(1035, Obj(28, 1035).pointer(2, 1010, 1011, 7).integer(5, 184)));
    s.entries.push_back(io(1041, item(460, 1041, "formule couche 7", 7, 1, place(0, 0))));
    return paperboard(QStringLiteral("Objets non lus"), {s},
                      {ir(2501, Obj(4, 2501).points(4, {QPointF(0, 0), QPointF(4, 4), QPointF(8, 0)})),
                       ir(2502, Obj(6, 2502).points(4, {QPointF(0, 0), QPointF(25, 25)}))});
}

// Deux grilles sur la page 1 (la seconde a deux niveaux) ; une grille vide sur la page 2.
inline QByteArray twoGrids()
{
    const auto level = [](quint32 storage, qint32 line, qint32 pattern, QRgb tint, bool shown, QPointF start,
                          double sx, double sy, double angle, bool alongX, bool alongY, bool magnetic) {
        Obj o(14, storage);
        o.integer(2, line)
            .integer(3, pattern)
            .argb(4, tint)
            .boolean(5, shown)
            .pointF(6, start.x(), start.y())
            .real(7, sx)
            .real(8, sy)
            .real(9, angle)
            .boolean(11, alongX)
            .boolean(12, alongY)
            .boolean(13, magnetic);
        return io(storage, o);
    };
    Sheet p1(1003);
    p1.props.boolean(17, true);
    p1.tree = {Node(1010, 13, 1011, {Node(1012, 14, 1013)}),
               Node(1020, 13, 1021, {Node(1022, 14, 1023), Node(1024, 14, 1025)}), Node(1030, 7, 1031)};
    p1.entries = {io(1011, Obj(13, 1011).boolean(16, true).boolean(17, true).real(13, 2)),
                  level(1013, 4, 2, 0xFF00FF00u, true, QPointF(0, 0), 20, 20, 30, true, true, true),
                  io(1021, Obj(13, 1021).real(14, 3).real(15, 0.25)),
                  level(1023, 6, 3, 0xFF0000FFu, false, QPointF(5, 5), 0, 24, 45, false, true, false),
                  level(1025, 1, 1, 0x0AFF0000u, true, QPointF(-3, 7), 16, 8, 0, true, false, true),
                  io(1031, rectangle(1031, "objet", 1, 1, place(10, 10), ink(1, 1, 32, 128, 0xFF000000u)))};
    Sheet p2(2003);
    p2.tree = {Node(2010, 13, 2011)};
    p2.entries = {io(2011, Obj(13, 2011))};
    return paperboard(QStringLiteral("Grilles"), {p1, p2});
}

// Lignes de texte : taille et graisse 0, absentes, graisses de la table et hors table.
inline QByteArray fonts()
{
    const auto run = [](const char* text, std::optional<double> size, std::optional<qint32> weight,
                        bool styled = false) {
        Obj r(31, 0);
        r.floatRect(2, 0, 0, 100, 20)
            .pointF(3, 0, 16)
            .argb(5, 0xFF102030u)
            .string(6, QString::fromLatin1(text))
            .string(7, QStringLiteral("Arial"));
        if (size) {
            r.real(8, *size);
        }
        if (weight) {
            r.integer(9, *weight);
        }
        r.boolean(10, styled).boolean(11, styled).boolean(12, styled);
        return r;
    };
    Sheet s(1003);
    s.tree = {Node(1010, 11, 1011), Node(1012, 11, 1013)};
    Obj text = item(11, 1011, "texte", 1, 1, place(40, 40));
    text.userRect(7, 0, 0, 200, 150).integer(38, 2401);
    Obj empty = item(11, 1013, "texte sans ressource", 1, 2, place(40, 300));
    empty.userRect(7, 0, 0, 50, 20);
    s.entries = {io(1011, text), io(1013, empty)};
    Obj res(12, 2401);
    res.runs(4, {run("taille 0 graisse 0", 0.0, 0), run("sans taille ni graisse", std::nullopt, std::nullopt),
                 run("gras italique", 18.5, 75, true), run("taille 0 graisse 12", 0.0, 12),
                 run("graisse 63", 10, 63), run("graisse 99", 9, 99), run("graisse 150", 11, 150),
                 run("graisse 1000", 12, 1000)})
        .string(5, QStringLiteral("<p>Polices</p>"))
        .integer(6, 2);
    return paperboard(QStringLiteral("Polices"), {s}, {ir(2401, res)});
}

// Médias : affiche et son (lecture à l'ouverture, en boucle), vidéo seule, média vide.
inline QByteArray media()
{
    Sheet s(1003);
    s.tree = {Node(1010, 253, 1011), Node(1012, 253, 1013), Node(1014, 253, 1015)};
    Obj m1 = item(253, 1011, "M1", 1, 1, place(50, 60));
    m1.userRect(7, 0, 0, 64, 48).integer(38, 4001).integer(45, 4002).boolean(41, true).boolean(42, true);
    Obj m2 = item(253, 1013, "M2", 1, 2, place(200, 60, 0, 2, 2));
    m2.userRect(7, 0, 0, 32, 24).integer(45, 4003);
    Obj m3 = item(253, 1015, "M3", 2, 1, place(300, 60));
    m3.userRect(7, 0, 0, 16, 16);
    s.entries = {io(1011, m1), io(1013, m2), io(1015, m3)};
    const QByteArray video("\0\0\0\x18"
                           "ftypisom\0\0\0\x01"
                           "isom",
                           20);
    const Obj registry =
        Obj(16, 0).resources(11, {Res(4002, QStringLiteral("4002.ir"), QStringLiteral("4002.mp3")),
                                  Res(4003, QStringLiteral("4003.ir"), QStringLiteral("film.mp4"))});
    return paperboard(QStringLiteral("Medias"), {s},
                      {ir(4001, Obj(10, 4001).size(5, 4, 3)),
                       Entry(QStringLiteral("4001.png"), pngBytes(4, 3, qRgb(0, 160, 0)), false),
                       ir(4002, Obj(30, 4002).string(3, QStringLiteral("4002.mp3"))),
                       Entry(QStringLiteral("4002.mp3"), sampleMp3(), false),
                       ir(4003, Obj(30, 4003).string(3, QStringLiteral("film.mp4"))),
                       Entry(QStringLiteral("film.mp4"), video, false)},
                      registry);
}

// Tous les cas, par nom de fichier (sans extension).
inline std::vector<std::pair<QString, QByteArray>> oracleCases()
{
    return {{QStringLiteral("fonds"), backgrounds()},
            {QStringLiteral("grilles"), twoGrids()},
            {QStringLiteral("imbrication"), nesting()},
            {QStringLiteral("lignes-connecteurs"), linesAndConnectors()},
            {QStringLiteral("medias"), media()},
            {QStringLiteral("non-lus"), unreadParents()},
            {QStringLiteral("polices"), fonts()},
            {QStringLiteral("remplissages-plumes"), fillsAndPens()},
            {QStringLiteral("traits"), strokes()}};
}

} // namespace synth

} // namespace fctest
