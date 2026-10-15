#include <actcraft/formats/objectstream.h>

#include <QStringDecoder>
#include <QtEndian>

#include <cstring>

namespace actcraft::formats::flipchart {

using namespace Qt::StringLiterals;

namespace {

constexpr int kMaxDepth = 32;
constexpr qint32 kMaxCount = 5'000'000;
constexpr quint32 kMaxProperties = 10'000;

// Lecteur borné d'un intervalle [pos, end) ; `ok` passe à faux à la première lecture impossible.
class Reader
{
public:
    Reader(const QByteArray& data, qsizetype pos, qsizetype end) : m_data(data), m_pos(pos), m_end(end) {}

    bool ok() const noexcept { return m_ok; }
    const QByteArray& data() const noexcept { return m_data; }
    qsizetype pos() const noexcept { return m_pos; }
    void seek(qsizetype pos) noexcept { m_pos = pos; }
    qsizetype end() const noexcept { return m_end; }
    bool has(qsizetype n) const noexcept { return n >= 0 && m_pos <= m_end - n; }
    char peek() const noexcept { return m_pos < m_end ? m_data.at(m_pos) : '\0'; }

    template <typename T> T le()
    {
        constexpr auto n = static_cast<qsizetype>(sizeof(T));
        if (!m_ok || !has(n)) {
            m_ok = false;
            return T{};
        }
        const T v = qFromLittleEndian<T>(m_data.constData() + m_pos);
        m_pos += n;
        return v;
    }
    double f64()
    {
        const quint64 bits = le<quint64>();
        double v = 0;
        std::memcpy(&v, &bits, sizeof v);
        return v;
    }
    char tag() { return static_cast<char>(le<quint8>()); }
    QByteArray take(qsizetype n)
    {
        if (!m_ok || !has(n)) {
            m_ok = false;
            return {};
        }
        QByteArray out = m_data.mid(m_pos, n);
        m_pos += n;
        return out;
    }
    qint32 count()
    {
        const qint32 n = le<qint32>();
        if (n < 0 || n > kMaxCount) {
            m_ok = false;
            return 0;
        }
        return n;
    }
    QString qstring()
    {
        const quint32 n = le<quint32>();
        if (!m_ok || n == 0xFFFFFFFFu) {
            return QString();
        }
        if (n % 2 != 0) {
            m_ok = false;
            return QString();
        }
        const QByteArray b = take(static_cast<qsizetype>(n));
        return m_ok ? QString(QStringDecoder(QStringDecoder::Utf16LE)(b)) : QString();
    }
    QByteArray qbytearray()
    {
        const quint32 n = le<quint32>();
        if (!m_ok || n == 0xFFFFFFFFu) {
            return QByteArray();
        }
        return take(static_cast<qsizetype>(n));
    }
    QString str16()
    {
        le<quint16>(); // préfixe constant 1 (§ 8 : rôle inconnu)
        return qstring();
    }

private:
    const QByteArray& m_data;
    qsizetype m_pos;
    qsizetype m_end;
    bool m_ok = true;
};

Polyline pointsOf(const QByteArray& blob)
{
    Polyline out;
    const qsizetype n = blob.size() / 16;
    out.reserve(static_cast<std::size_t>(n));
    for (qsizetype i = 0; i < n; ++i) {
        double xy[2] = {0, 0};
        std::memcpy(xy, blob.constData() + i * 16, 16);
        out.push_back(core::PointF{qFromLittleEndian(xy[0]), qFromLittleEndian(xy[1])});
    }
    return out;
}

std::optional<StreamObject> readObject(Reader& r, int depth, QString& error);

Value readUser(Reader& r, quint16 sub, int depth, QString& error)
{
    Value v;
    switch (sub) {
    case 1: {
        r.le<qint32>();
        v.data = pointsOf(r.qbytearray());
        break;
    }
    case 2: {
        std::vector<Polyline> polys;
        const qint32 n = r.count();
        for (qint32 i = 0; i < n && r.ok(); ++i) {
            r.le<qint32>();
            polys.push_back(pointsOf(r.qbytearray()));
        }
        v.data = std::move(polys);
        break;
    }
    case 3: {
        std::vector<ResourceItem> items;
        const qint32 n = r.count();
        for (qint32 i = 0; i < n && r.ok(); ++i) {
            ResourceItem item;
            item.id = r.le<quint32>();
            item.entry = r.str16();
            item.dataFile = r.str16();
            item.digest = r.str16();
            item.refCount = r.le<quint32>();
            items.push_back(std::move(item));
        }
        v.data = std::move(items);
        break;
    }
    case 4:
    case 6:
    case 7:
    case 8:
    case 12:
    case 14:
    case 18: {
        std::optional<StreamObject> o = readObject(r, depth + 1, error);
        if (!o) {
            r.seek(r.end() + 1); // marque l'échec : la propriété deviendra brute
            r.le<quint8>();
            break;
        }
        v.data = ObjectPtr(std::make_shared<const StreamObject>(std::move(*o)));
        break;
    }
    case 5:
        v.data = core::RectF{r.f64(), r.f64(), r.f64(), r.f64()};
        break;
    case 9: {
        Pointer p;
        p.nodeId = r.le<quint32>();
        p.storageId = r.le<quint32>();
        p.type = r.le<quint16>();
        v.data = p;
        break;
    }
    case 10: {
        std::vector<ObjectPtr> objects;
        const qint32 n = r.count();
        for (qint32 i = 0; i < n && r.ok(); ++i) {
            std::optional<StreamObject> o = readObject(r, depth + 1, error);
            if (!o) {
                r.seek(r.end() + 1);
                r.le<quint8>();
                break;
            }
            objects.push_back(std::make_shared<const StreamObject>(std::move(*o)));
        }
        v.data = std::move(objects);
        break;
    }
    default:
        v.data = RawValue{127, sub, r.take(r.end() - r.pos())};
        break;
    }
    return v;
}

Value readStreamValue(Reader& r, int depth, QString& error)
{
    Value v;
    if (depth > kMaxDepth) {
        r.seek(r.end() + 1);
        r.le<quint8>();
        return v;
    }
    const quint16 t = r.le<quint16>();
    switch (t) {
    case 1:
        v.data = r.le<quint8>() != 0;
        break;
    case 2:
        v.data = r.le<qint32>();
        break;
    case 3:
        v.data = r.le<quint32>();
        break;
    case 6:
        v.data = r.f64();
        break;
    case 9: {
        ValueList list;
        const qint32 n = r.count();
        for (qint32 i = 0; i < n && r.ok(); ++i) {
            list.push_back(readStreamValue(r, depth + 1, error));
        }
        v.data = std::move(list);
        break;
    }
    case 10:
        v.data = r.str16();
        break;
    case 11: {
        r.le<quint16>();
        QStringList list;
        const qint32 n = r.count();
        for (qint32 i = 0; i < n && r.ok(); ++i) {
            list << r.qstring();
        }
        v.data = std::move(list);
        break;
    }
    case 12:
        v.data = r.qbytearray();
        break;
    case 15: {
        const int h = r.le<quint8>();
        const int m = r.le<quint8>();
        const int s = r.le<quint8>();
        v.data = u"%1:%2:%3"_s.arg(h, 2, 10, QLatin1Char('0'))
                     .arg(m, 2, 10, QLatin1Char('0'))
                     .arg(s, 2, 10, QLatin1Char('0'));
        break;
    }
    case 16: {
        const int year = r.le<quint16>();
        int parts[5] = {};
        for (int& p : parts) {
            p = r.le<quint8>();
        }
        v.data = u"%1-%2-%3T%4:%5:%6"_s.arg(year, 4, 10, QLatin1Char('0'))
                     .arg(parts[0], 2, 10, QLatin1Char('0'))
                     .arg(parts[1], 2, 10, QLatin1Char('0'))
                     .arg(parts[2], 2, 10, QLatin1Char('0'))
                     .arg(parts[3], 2, 10, QLatin1Char('0'))
                     .arg(parts[4], 2, 10, QLatin1Char('0'));
        break;
    }
    case 19: {
        const qint32 x = r.le<qint32>();
        const qint32 y = r.le<qint32>();
        const qint32 w = r.le<qint32>();
        const qint32 h = r.le<qint32>();
        v.data = core::RectF{double(x), double(y), double(w), double(h)};
        break;
    }
    case 20:
        v.data = core::RectF{r.f64(), r.f64(), r.f64(), r.f64()};
        break;
    case 21: {
        const qint32 w = r.le<qint32>();
        const qint32 h = r.le<qint32>();
        v.data = SizeF{double(w), double(h)};
        break;
    }
    case 22:
        v.data = SizeF{r.f64(), r.f64()};
        break;
    case 26:
        v.data = core::PointF{r.f64(), r.f64()};
        break;
    case 67: {
        Color c;
        c.r = r.le<quint8>();
        c.g = r.le<quint8>();
        c.b = r.le<quint8>();
        c.a = r.le<quint8>();
        v.data = c;
        break;
    }
    case 127:
        return readUser(r, r.le<quint16>(), depth, error);
    default:
        v.data = RawValue{t, 0, r.take(r.end() - r.pos())};
        break;
    }
    return v;
}

std::optional<StreamObject> readObject(Reader& r, int depth, QString& error)
{
    if (depth > kMaxDepth) {
        error = u"imbrication trop profonde"_s;
        return std::nullopt;
    }
    const qsizetype at = r.pos();
    if (r.tag() != 'O') {
        error = u"balise 'O' attendue à l'octet %1"_s.arg(at);
        return std::nullopt;
    }
    StreamObject o;
    o.type = r.le<quint16>();
    o.storageId = r.le<quint32>();
    if (r.tag() != 'M') {
        error = u"balise 'M' attendue après l'octet %1"_s.arg(at);
        return std::nullopt;
    }
    const quint32 n = r.le<quint32>();
    if (!r.ok() || n > kMaxProperties) {
        error = u"table des tailles illisible après l'octet %1"_s.arg(at);
        return std::nullopt;
    }
    std::vector<quint32> sizes(n);
    for (quint32& s : sizes) {
        s = r.le<quint32>();
    }
    if (!r.ok()) {
        error = u"table des tailles tronquée après l'octet %1"_s.arg(at);
        return std::nullopt;
    }
    for (const quint32 size : sizes) {
        const qsizetype start = r.pos();
        if (size < 6 || !r.has(static_cast<qsizetype>(size))) {
            error = u"enregistrement de propriété invalide à l'octet %1"_s.arg(start);
            return std::nullopt;
        }
        const qsizetype end = start + static_cast<qsizetype>(size);
        Reader header(r.data(), start, end);
        header.tag();
        if (header.tag() != 'P') {
            error = u"balise 'P' attendue à l'octet %1"_s.arg(start + 1);
            return std::nullopt;
        }
        const quint16 id = header.le<quint16>();
        Reader sub(r.data(), start + 4, end);
        QString ignored;
        Value value = readStreamValue(sub, depth, ignored);
        if (!sub.ok()) {
            Reader rawType(r.data(), start + 4, end);
            const quint16 qtype = rawType.le<quint16>();
            value.data = RawValue{qtype, 0, r.data().mid(start + 6, static_cast<qsizetype>(size) - 6)};
        }
        o.properties[id] = std::move(value);
        r.seek(end);
    }
    // Fin d'objet : 'p' ['R' u32 ['r']] ['o'].
    if (r.peek() == 'p') {
        r.tag();
        if (r.peek() == 'R') {
            r.tag();
            o.nodeId = r.le<quint32>();
            if (r.peek() == 'r') {
                r.tag();
            }
        }
        if (r.peek() == 'o') {
            r.tag();
        }
    }
    if (!r.ok()) {
        error = u"fin d'objet tronquée après l'octet %1"_s.arg(at);
        return std::nullopt;
    }
    return o;
}

} // namespace

const Value* StreamObject::find(quint16 id) const
{
    const auto it = properties.find(id);
    return it == properties.end() ? nullptr : &it->second;
}

std::optional<StreamObject> parseObjectStream(const QByteArray& data, QString& error)
{
    if (!data.startsWith(" Bamboo ") || data.size() < 12) {
        error = u"signature de flux absente"_s;
        return std::nullopt;
    }
    Reader r(data, 12, data.size());
    return readObject(r, 0, error);
}

std::optional<double> asNumber(const Value* v)
{
    if (!v) {
        return std::nullopt;
    }
    if (const auto* b = std::get_if<bool>(&v->data)) {
        return *b ? 1.0 : 0.0;
    }
    if (const auto* i = std::get_if<qint32>(&v->data)) {
        return double(*i);
    }
    if (const auto* u = std::get_if<quint32>(&v->data)) {
        return double(*u);
    }
    if (const auto* d = std::get_if<double>(&v->data)) {
        return *d;
    }
    return std::nullopt;
}

std::optional<bool> asBool(const Value* v)
{
    if (v && std::holds_alternative<bool>(v->data)) {
        return std::get<bool>(v->data);
    }
    if (const std::optional<double> n = asNumber(v)) {
        return *n != 0;
    }
    return std::nullopt;
}

QString asString(const Value* v)
{
    if (const auto* s = v ? std::get_if<QString>(&v->data) : nullptr) {
        return *s;
    }
    return QString();
}

std::optional<core::PointF> asPoint(const Value* v)
{
    if (const auto* p = v ? std::get_if<core::PointF>(&v->data) : nullptr) {
        return *p;
    }
    return std::nullopt;
}

std::optional<SizeF> asSize(const Value* v)
{
    if (const auto* s = v ? std::get_if<SizeF>(&v->data) : nullptr) {
        return *s;
    }
    return std::nullopt;
}

std::optional<core::RectF> asRect(const Value* v)
{
    if (const auto* r = v ? std::get_if<core::RectF>(&v->data) : nullptr) {
        return *r;
    }
    return std::nullopt;
}

std::optional<Color> asColor(const Value* v)
{
    if (const auto* c = v ? std::get_if<Color>(&v->data) : nullptr) {
        return *c;
    }
    return std::nullopt;
}

std::optional<Pointer> asPointer(const Value* v)
{
    if (const auto* p = v ? std::get_if<Pointer>(&v->data) : nullptr) {
        return *p;
    }
    return std::nullopt;
}

const StreamObject* asObject(const Value* v)
{
    if (const auto* o = v ? std::get_if<ObjectPtr>(&v->data) : nullptr) {
        return o->get();
    }
    return nullptr;
}

std::vector<const StreamObject*> asObjects(const Value* v)
{
    std::vector<const StreamObject*> out;
    if (const auto* list = v ? std::get_if<std::vector<ObjectPtr>>(&v->data) : nullptr) {
        for (const ObjectPtr& o : *list) {
            out.push_back(o.get());
        }
    }
    return out;
}

Polyline asPolyline(const Value* v)
{
    if (const auto* p = v ? std::get_if<Polyline>(&v->data) : nullptr) {
        return *p;
    }
    return {};
}

std::vector<Polyline> asPolylines(const Value* v)
{
    if (const auto* p = v ? std::get_if<std::vector<Polyline>>(&v->data) : nullptr) {
        return *p;
    }
    return {};
}

std::vector<ResourceItem> asResourceItems(const Value* v)
{
    if (const auto* p = v ? std::get_if<std::vector<ResourceItem>>(&v->data) : nullptr) {
        return *p;
    }
    return {};
}

} // namespace actcraft::formats::flipchart
