#include <actcraft/formats/bamboo.h>

#include <QStringDecoder>
#include <QtEndian>

#include <miniz.h>

namespace actcraft::formats::flipchart {

using namespace Qt::StringLiterals;

namespace {

// Lecture bornée : chaque méthode rend faux au lieu de lire hors des données.
class Cursor
{
public:
    Cursor(const QByteArray& data, qsizetype pos) : m_data(data), m_pos(pos) {}

    bool atEnd() const noexcept { return m_pos >= m_data.size(); }
    qsizetype pos() const noexcept { return m_pos; }
    bool has(qsizetype n) const noexcept { return n >= 0 && m_pos <= m_data.size() - n; }

    template <typename T> bool le(T& out)
    {
        constexpr auto n = static_cast<qsizetype>(sizeof(T));
        if (!has(n)) {
            return false;
        }
        out = qFromLittleEndian<T>(m_data.constData() + m_pos);
        m_pos += n;
        return true;
    }
    bool bytes(qsizetype n, QByteArray& out)
    {
        if (!has(n)) {
            return false;
        }
        out = m_data.mid(m_pos, n);
        m_pos += n;
        return true;
    }
    bool tag(char& out)
    {
        if (!has(1)) {
            return false;
        }
        out = m_data.at(m_pos);
        ++m_pos;
        return true;
    }
    // u32 longueur en octets (0xFFFFFFFF = chaîne nulle), puis UTF-16LE.
    bool qstring(QString& out)
    {
        quint32 n = 0;
        if (!le(n)) {
            return false;
        }
        if (n == 0xFFFFFFFFu) {
            out.clear();
            return true;
        }
        QByteArray b;
        if (n % 2 != 0 || !bytes(static_cast<qsizetype>(n), b)) {
            return false;
        }
        out = QStringDecoder(QStringDecoder::Utf16LE)(b);
        return true;
    }

private:
    const QByteArray& m_data;
    qsizetype m_pos;
};

} // namespace

std::optional<BambooContainer> BambooContainer::parse(const QByteArray& data, QString& error)
{
    if (!data.startsWith(" Bamboo ")) {
        error = u"signature Bamboo absente"_s;
        return std::nullopt;
    }
    Cursor c(data, 8);
    BambooContainer out;
    QString format;
    QByteArray reserved;
    if (!c.le(out.m_version) || !c.qstring(format) || !c.bytes(8, reserved)) {
        error = u"en-tête du conteneur tronqué"_s;
        return std::nullopt;
    }
    while (!c.atEnd()) {
        const qsizetype at = c.pos();
        BambooEntry e;
        QString created;
        QString modified;
        QString accessed;
        quint8 compressed = 0;
        quint64 stored = 0;
        if (!c.qstring(e.name) || !c.qstring(created) || !c.qstring(modified) || !c.qstring(accessed) ||
            !c.le(e.size) || !c.le(compressed) || !c.le(stored)) {
            error = u"entrée tronquée à l'octet %1"_s.arg(at);
            return std::nullopt;
        }
        if (stored > static_cast<quint64>(data.size()) || !c.bytes(static_cast<qsizetype>(stored), e.raw)) {
            error = u"données de l'entrée « %1 » tronquées"_s.arg(e.name);
            return std::nullopt;
        }
        e.compressed = compressed != 0;
        out.m_byName.insert(e.name, out.m_entries.size());
        out.m_entries.push_back(std::move(e));
    }
    return out;
}

QStringList BambooContainer::names() const
{
    QStringList out;
    for (const BambooEntry& e : m_entries) {
        out << e.name;
    }
    return out;
}

bool BambooContainer::contains(const QString& name) const
{
    return m_byName.contains(name);
}

std::optional<QByteArray> BambooContainer::read(const QString& name, QString& error) const
{
    const auto it = m_byName.constFind(name);
    if (it == m_byName.cend()) {
        error = u"entrée « %1 » absente"_s.arg(name);
        return std::nullopt;
    }
    const BambooEntry& e = m_entries[it.value()];
    if (e.size > static_cast<quint64>(kMaxEntrySize)) {
        error = u"entrée « %1 » trop volumineuse"_s.arg(name);
        return std::nullopt;
    }
    if (!e.compressed) {
        if (static_cast<quint64>(e.raw.size()) != e.size) {
            error =
                u"entrée « %1 » : %2 octets stockés, %3 annoncés"_s.arg(name).arg(e.raw.size()).arg(e.size);
            return std::nullopt;
        }
        return e.raw;
    }
    if (e.raw.size() < 4) {
        error = u"entrée « %1 » : bloc compressé trop court"_s.arg(name);
        return std::nullopt;
    }
    if (e.size == 0) {
        return QByteArray();
    }
    // Décompression dans un tampon de la taille annoncée par l'entrée (≤ kMaxEntrySize), jamais plus :
    // l'en-tête de 4 octets de qCompress est fixé par le fichier et la fonction de Qt qui le suit le dépasse (relecture du
    // plan, tour 1 : en-tête de 16 octets, entrée de 597 Kio → 600 Mio). mz_uncompress s'arrête au bout du
    // tampon (MZ_BUF_ERROR).
    QByteArray out(static_cast<qsizetype>(e.size), Qt::Uninitialized);
    mz_ulong length = static_cast<mz_ulong>(e.size);
    const int status = mz_uncompress(reinterpret_cast<unsigned char*>(out.data()), &length,
                                     reinterpret_cast<const unsigned char*>(e.raw.constData()) + 4,
                                     static_cast<mz_ulong>(e.raw.size() - 4));
    if (status != MZ_OK || length != static_cast<mz_ulong>(e.size)) {
        error = u"entrée « %1 » : flux zlib illisible ou d'une autre taille que les %2 octets annoncés"_s
                    .arg(name)
                    .arg(e.size);
        return std::nullopt;
    }
    return out;
}

quint16 BambooContainer::version() const noexcept
{
    return m_version;
}

QString IndexNode::storageName() const
{
    return u"%1.io"_s.arg(storageId);
}

std::optional<IndexNode> parseIndex(const QByteArray& data, QString& error)
{
    if (!data.startsWith("# culm #")) {
        error = u"signature d'index absente"_s;
        return std::nullopt;
    }
    Cursor c(data, 12);
    std::optional<IndexNode> root;
    std::vector<IndexNode*> stack; // chemin de la racine au nœud ouvert ; seul le dernier enfant est ouvert
    while (!c.atEnd()) {
        const qsizetype at = c.pos();
        char tag = 0;
        c.tag(tag);
        if (tag == 'N') {
            IndexNode node;
            if (!c.le(node.nodeId) || !c.le(node.type) || !c.le(node.storageId)) {
                error = u"nœud tronqué à l'octet %1"_s.arg(at);
                return std::nullopt;
            }
            if (!stack.empty()) {
                stack.back()->children.push_back(std::move(node));
                stack.push_back(&stack.back()->children.back());
            } else if (!root) {
                root = std::move(node);
                stack.push_back(&*root);
            } else {
                error = u"plusieurs racines dans l'index"_s;
                return std::nullopt;
            }
            if (stack.size() > 256) {
                error = u"arbre trop profond"_s;
                return std::nullopt;
            }
        } else if (tag == 'S') {
            quint32 ref = 0;
            char close = 0;
            if (!c.le(ref) || !c.tag(close) || close != 's') {
                error = u"référence mal fermée à l'octet %1"_s.arg(at);
                return std::nullopt;
            }
            if (stack.empty()) {
                error = u"référence hors nœud à l'octet %1"_s.arg(at);
                return std::nullopt;
            }
            stack.back()->refs.push_back(ref);
        } else if (tag == 'n') {
            if (stack.empty()) {
                error = u"'n' sans nœud ouvert à l'octet %1"_s.arg(at);
                return std::nullopt;
            }
            stack.pop_back();
        } else {
            error = u"balise d'index inconnue à l'octet %1"_s.arg(at);
            return std::nullopt;
        }
    }
    if (!root) {
        error = u"index vide"_s;
        return std::nullopt;
    }
    if (!stack.empty()) {
        error = u"nœud non fermé en fin d'index"_s;
        return std::nullopt;
    }
    return root;
}

} // namespace actcraft::formats::flipchart
