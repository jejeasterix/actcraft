#include "flipchartids.h"

#include <actcraft/formats/bamboo.h>
#include <actcraft/formats/flipchart.h>
#include <actcraft/formats/objectstream.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <optional>
#include <utility>

namespace actcraft::formats::flipchart {

using namespace Qt::StringLiterals;

namespace {

// Objets imbriqués lus au plus sur 32 niveaux : la lecture est récursive et l'index en admet 256 (en
// débogage, 200 niveaux débordaient la pile du fil principal). Les paperboards réels n'imbriquent que
// quelques niveaux. Écart gardé (sécurité, L2-C4) : la référence ne borne pas ; l'oracle synthétique ne
// l'exerce pas.
constexpr int kMaxNesting = 32;
// Côté de page accepté, en pixels de page ; au-delà (ou NaN, nul, négatif), taille par défaut et
// avertissement.
constexpr double kMaxPageSide = 100000;
constexpr double kDefaultPageWidth = 1024;
constexpr double kDefaultPageHeight = 768;

// Borne de FlipchartLimits dépassée : remonte jusqu'à readFlipchart, qui en fait un échec de lecture.
struct LimitExceeded
{
    QString message;
};

core::Argb argb(const Color& c) noexcept
{
    return (core::Argb{c.a} << 24) | (core::Argb{c.r} << 16) | (core::Argb{c.g} << 8) | core::Argb{c.b};
}

double numberOr(const StreamObject* o, quint16 id, double fallback)
{
    return o ? asNumber(o->find(id)).value_or(fallback) : fallback;
}

// Entier lu du fichier (non fiable) : NaN, infini ou hors des bornes d'un int → valeur par défaut.
int intOr(const StreamObject* o, quint16 id, int fallback)
{
    const double v = numberOr(o, id, fallback);
    if (!std::isfinite(v) || v < static_cast<double>(std::numeric_limits<int>::min()) ||
        v > static_cast<double>(std::numeric_limits<int>::max())) {
        return fallback;
    }
    return static_cast<int>(v);
}

bool flagOr(const StreamObject* o, quint16 id, bool fallback)
{
    return o ? asBool(o->find(id)).value_or(fallback) : fallback;
}

core::Argb colorOr(const StreamObject* o, quint16 id, core::Argb fallback)
{
    const std::optional<Color> c = o ? asColor(o->find(id)) : std::nullopt;
    return c ? argb(*c) : fallback;
}

std::string stringOf(const StreamObject* o, quint16 id)
{
    return o ? asString(o->find(id)).toStdString() : std::string();
}

// Points d'une ressource, sans copie : la ressource est en cache, partagée par tous les objets qui la
// désignent.
const Polyline* polylineOf(const Value* v)
{
    return v ? std::get_if<Polyline>(&v->data) : nullptr;
}

const std::vector<Polyline>* polylinesOf(const Value* v)
{
    return v ? std::get_if<std::vector<Polyline>>(&v->data) : nullptr;
}

bool isPositiveFinite(double v) noexcept
{
    return std::isfinite(v) && v > 0;
}

bool isUsablePageSize(const SizeF& s) noexcept
{
    return isPositiveFinite(s.width) && isPositiveFinite(s.height) && s.width <= kMaxPageSide &&
           s.height <= kMaxPageSide;
}

bool isFinite(const core::Transform& m) noexcept
{
    return std::isfinite(m.m11) && std::isfinite(m.m12) && std::isfinite(m.m21) && std::isfinite(m.m22) &&
           std::isfinite(m.dx) && std::isfinite(m.dy);
}

// Extension devinée par la signature, sinon par le nom (comme la référence).
std::string extensionOf(const QByteArray& data, const QString& entryName)
{
    if (data.startsWith("\x89PNG\r\n\x1a\n")) {
        return "png";
    }
    if (data.startsWith("\xff\xd8\xff")) {
        return "jpg";
    }
    if (data.startsWith("GIF87a") || data.startsWith("GIF89a")) {
        return "gif";
    }
    if (data.startsWith("BM")) {
        return "bmp";
    }
    if (data.startsWith(QByteArray("II*\0", 4)) || data.startsWith(QByteArray("MM\0*", 4))) {
        return "tif";
    }
    if (data.startsWith("RIFF") && data.mid(8, 4) == "WAVE") {
        return "wav";
    }
    if (data.startsWith("ID3") || data.startsWith("\xff\xfb") || data.startsWith("\xff\xf3")) {
        return "mp3";
    }
    if (data.mid(4, 4) == "ftyp") {
        return "mp4";
    }
    const qsizetype dot = entryName.lastIndexOf(u'.');
    return dot >= 0 ? entryName.mid(dot + 1).toLower().toStdString() : std::string("bin");
}

// Graisse Qt 4 (0 à 99, 50 normal, 75 gras) → Qt 6 (100 à 900) : la valeur de la table la plus proche. 0 vaut
// la graisse normale, comme dans la référence.
std::int32_t qt6Weight(int w)
{
    if (w == 0) {
        w = 50;
    }
    if (w >= 100) {
        return std::min(900, w);
    }
    static constexpr int table[][2] = {{0, 100},  {12, 200}, {25, 300}, {50, 400}, {57, 500},
                                       {63, 600}, {75, 700}, {81, 800}, {87, 900}};
    int best = 400;
    int distance = 1000;
    for (const auto& [qt4, qt6] : table) {
        if (std::abs(qt4 - w) < distance) {
            distance = std::abs(qt4 - w);
            best = qt6;
        }
    }
    return best;
}

core::Outline outlineFrom(const StreamObject* pen)
{
    core::Outline o;
    if (!pen) {
        return o;
    }
    const int style = intOr(pen, ids::penKeys::kDashPattern, 1);
    o.style = style >= 0 && style <= 5 ? static_cast<core::PenStyle>(style) : core::PenStyle::Solid;
    o.color = colorOr(pen, ids::penKeys::kOutlineColor, 0xFF000000u);
    o.width = numberOr(pen, ids::penKeys::kOutlineWidth, 1);
    if (o.width <= 0) {
        o.style = core::PenStyle::None;
    }
    const int cap = intOr(pen, ids::penKeys::kEndShape, 32);
    o.cap = cap == 0 ? core::PenCap::Flat : cap == 16 ? core::PenCap::Square : core::PenCap::Round;
    const int join = intOr(pen, ids::penKeys::kCornerShape, 128);
    o.join = join == 0 ? core::PenJoin::Miter : join == 64 ? core::PenJoin::Bevel : core::PenJoin::Round;
    return o;
}

core::GradientKind gradientOf(int field)
{
    return field >= 2 && field <= 5 ? static_cast<core::GradientKind>(field) : core::GradientKind::Vertical;
}

core::Fill fillFrom(const StreamObject* pen)
{
    core::Fill f;
    f.color = colorOr(pen, ids::penKeys::kFillTint, 0xFFFFFFFFu);
    f.color2 = colorOr(pen, ids::penKeys::kSecondaryColor, 0xFF000000u);
    f.opaqueBackground = intOr(pen, ids::penKeys::kOpaqueHatch, 0) != 0;
    f.backgroundColor = colorOr(pen, ids::penKeys::kHatchBaseColor, 0xFF000000u);
    const int style = intOr(pen, ids::penKeys::kFillPattern, 0);
    const int gradient = intOr(pen, ids::penKeys::kBlendDirection, 0);
    if (style == 0) {
        f.kind = core::FillKind::None;
    } else if (gradient != 0 || style == 25) {
        f.kind = core::FillKind::Gradient;
        f.gradient = gradientOf(gradient);
    } else if (style >= 2 && style <= 14) {
        f.kind = core::FillKind::Pattern;
        f.pattern = static_cast<std::uint8_t>(style);
    } else {
        f.kind = core::FillKind::Solid;
    }
    return f;
}

// § 7.4 : translate(t + o) ; rotate(angle) ; scale(sx, sy) ; translate(-o) ; miroir éventuel appliqué
// d'abord.
core::Transform localTransform(const StreamObject& o, const core::RectF& bounds)
{
    const StreamObject* t = asObject(o.find(ids::itemKeys::kPlacement));
    if (!t) {
        return {};
    }
    const core::PointF offset = asPoint(t->find(ids::placementKeys::kOffset)).value_or(core::PointF{});
    const core::PointF factors =
        asPoint(t->find(ids::placementKeys::kAxisFactors)).value_or(core::PointF{1, 1});
    const double degrees = numberOr(t, ids::placementKeys::kTurnDegrees, 0);
    const bool mirrorX = flagOr(t, ids::placementKeys::kMirrorX, false);
    const bool mirrorY = flagOr(t, ids::placementKeys::kMirrorY, false);
    core::PointF pivot;
    if (const StreamObject* p = asObject(o.find(ids::itemKeys::kPivot))) {
        pivot = asPoint(p->find(ids::pivotKeys::kPivotPoint)).value_or(core::PointF{});
    }
    core::Transform m = core::compose(core::compose(core::compose(core::translation(-pivot.x, -pivot.y),
                                                                  core::scaling(factors.x, factors.y)),
                                                    core::rotation(degrees)),
                                      core::translation(offset.x + pivot.x, offset.y + pivot.y));
    if (mirrorX || mirrorY) {
        const core::Transform mirror =
            core::compose(core::scaling(mirrorX ? -1 : 1, mirrorY ? -1 : 1),
                          core::translation(mirrorX ? bounds.width : 0, mirrorY ? bounds.height : 0));
        m = core::compose(mirror, m);
    }
    return m;
}

struct PendingAction
{
    core::Action action;
    quint32 targetNode = 0;
};

// Objet lu, avant le tri et la résolution des références.
struct Pending
{
    int layer = 1;
    double depth = 0;
    int order = 0;
    quint32 nodeId = 0;
    core::PageObject object;
    std::vector<quint32> members; // groupe
    quint32 connectorA = 0;
    quint32 connectorB = 0;
    std::vector<PendingAction> actions;
    std::vector<Pending> children;
    // Genre non lu (formule, texte vertical…) : absent du modèle, mais ses enfants y entrent, à sa place dans
    // l'ordre de dessin et composés avec sa transformation, comme dans la référence.
    bool unread = false;
};

bool drawsBefore(const Pending& a, const Pending& b)
{
    if (a.layer != b.layer) {
        return a.layer < b.layer;
    }
    if (a.depth != b.depth) {
        return a.depth < b.depth;
    }
    return a.order < b.order;
}

class Reader
{
public:
    Reader(const QByteArray& data, const FlipchartLimits& limits) : m_data(data), m_limits(limits) {}

    FlipchartReadResult read()
    {
        FlipchartReadResult result;
        QString error;
        m_root = BambooContainer::parse(m_data, error);
        if (!m_root) {
            result.error = u"paperboard illisible : %1"_s.arg(error);
            return result;
        }
        QString indexName;
        for (const QString& entryName : m_root->names()) {
            if (entryName.endsWith(u".in"_s)) {
                indexName = entryName;
                break;
            }
        }
        std::optional<IndexNode> tree;
        if (!indexName.isEmpty()) {
            if (const std::optional<QByteArray> bytes = readEntry(*m_root, indexName, error)) {
                tree = parseIndex(*bytes, error);
            }
        }
        if (!tree) {
            result.error = u"index racine absent ou illisible %1"_s.arg(error);
            return result;
        }
        const std::optional<StreamObject> doc = load(*m_root, tree->storageName(), error);
        if (!doc) {
            result.error = u"objet paperboard illisible : %1"_s.arg(error);
            return result;
        }
        if (const StreamObject* table = asObject(doc->find(ids::docKeys::kResourceTable))) {
            for (const auto& [id, field] : table->properties) {
                for (const ResourceItem& item : asResourceItems(&field)) {
                    m_register[item.id] = item;
                }
            }
        }
        std::string heading = stringOf(&*doc, ids::docKeys::kTitleText);
        if (heading.empty()) {
            heading = stringOf(&*doc, ids::docKeys::kFallbackTitle);
        }
        SizeF startSize{kDefaultPageWidth, kDefaultPageHeight};
        if (const std::optional<SizeF> dims = asSize(doc->find(ids::docKeys::kStartPageSize))) {
            if (isUsablePageSize(*dims)) {
                startSize = *dims;
            } else {
                warn(u"taille de page par défaut invalide (%1 × %2), 1024 × 768"_s.arg(dims->width)
                         .arg(dims->height));
            }
        }
        std::vector<core::Page> pages;
        for (const IndexNode& child : tree->children) {
            if (child.type != ids::kind::kPageObject) {
                continue;
            }
            QByteArray thumbnail;
            if (std::optional<core::Page> sheet = readPage(child, startSize, thumbnail)) {
                pages.push_back(std::move(*sheet));
                result.thumbnails.push_back(std::move(thumbnail));
            }
        }
        if (pages.empty()) {
            result.error = u"aucune page lisible"_s;
            result.warnings = m_warnings;
            return result;
        }
        core::Document document(std::move(pages));
        document.setTitle(std::move(heading));
        for (const auto& [key, stored] : m_store.resources()) {
            document.addResource(stored.data, stored.extension);
        }
        result.document = std::move(document);
        result.warnings = m_warnings;
        result.ok = true;
        return result;
    }

private:
    // Toute lecture d'entrée passe ici : le volume décompressé cumulé est borné (une entrée désignée par
    // mille nœuds est décompressée mille fois).
    std::optional<QByteArray> readEntry(const BambooContainer& c, const QString& entryName, QString& error)
    {
        std::optional<QByteArray> bytes = c.read(entryName, error);
        if (bytes) {
            m_decompressed += bytes->size();
            if (m_decompressed > m_limits.maxDecompressedBytes) {
                throw LimitExceeded{u"paperboard trop volumineux : plus de %1 Mio décompressés"_s.arg(
                    m_limits.maxDecompressedBytes >> 20)};
            }
        }
        return bytes;
    }

    // Octets recopiés dans le modèle (points, textes, ressources), comptés avant la copie.
    void chargeModel(qint64 bytes)
    {
        m_modelBytes += bytes;
        if (m_modelBytes > m_limits.maxModelBytes) {
            throw LimitExceeded{
                u"paperboard trop volumineux : plus de %1 Mio de points, textes et ressources"_s.arg(
                    m_limits.maxModelBytes >> 20)};
        }
    }

    template <typename T> void chargeItems(std::size_t count)
    {
        chargeModel(static_cast<qint64>(count * sizeof(T)));
    }

    std::optional<StreamObject> load(const BambooContainer& c, const QString& entryName, QString& error)
    {
        const std::optional<QByteArray> bytes = readEntry(c, entryName, error);
        return bytes ? parseObjectStream(*bytes, error) : std::nullopt;
    }

    void warn(const QString& message) { m_warnings << message; }

    const StreamObject* resourceObject(quint32 rid)
    {
        if (const auto it = m_resourceObjects.find(rid); it != m_resourceObjects.end()) {
            return it->second.get();
        }
        const auto item = m_register.find(rid);
        const QString entry = item != m_register.end() && !item->second.entry.isEmpty() ? item->second.entry
                                                                                        : u"%1.ir"_s.arg(rid);
        std::shared_ptr<const StreamObject> object;
        if (m_root->contains(entry)) {
            QString error;
            if (std::optional<StreamObject> o = load(*m_root, entry, error)) {
                object = std::make_shared<const StreamObject>(std::move(*o));
            } else {
                warn(u"ressource %1 illisible : %2"_s.arg(entry, error));
            }
        }
        m_resourceObjects[rid] = object;
        return object.get();
    }

    std::optional<std::pair<QString, QByteArray>> resourceFile(quint32 rid, const StreamObject* res)
    {
        QStringList names;
        if (const auto item = m_register.find(rid);
            item != m_register.end() && !item->second.dataFile.isEmpty()) {
            names << item->second.dataFile;
        }
        if (res) {
            if (const QString original = asString(res->find(ids::resourceKeys::kSourceFileName));
                !original.isEmpty()) {
                names << original;
            }
        }
        for (const char* ext : {"png", "jpg", "ird", "gif", "bmp", "tif"}) {
            names << u"%1.%2"_s.arg(rid).arg(QLatin1String(ext));
        }
        for (const QString& entryName : names) {
            if (!m_root->contains(entryName)) {
                continue;
            }
            QString error;
            if (std::optional<QByteArray> data = readEntry(*m_root, entryName, error)) {
                return std::make_pair(entryName, std::move(*data));
            }
            warn(u"données %1 illisibles : %2"_s.arg(entryName, error));
        }
        return std::nullopt;
    }

    // Range les octets par empreinte ; une copie neuve compte dans le budget du modèle.
    std::string storeResource(const std::pair<QString, QByteArray>& file)
    {
        const std::size_t before = m_store.resources().size();
        std::string key = m_store.addResource(file.second, extensionOf(file.second, file.first));
        if (m_store.resources().size() > before) {
            chargeModel(file.second.size());
        }
        return key;
    }

    // Clé de la ressource image `rid` (vide, avec un avertissement, si ses données manquent). Mémorisée par
    // identifiant : une image partagée par cent objets n'est lue, décompressée et hachée qu'une fois.
    std::string imageKey(quint32 rid)
    {
        if (const auto it = m_fileKeys.find(rid); it != m_fileKeys.end()) {
            return it->second.first;
        }
        const auto file = resourceFile(rid, resourceObject(rid));
        if (!file) {
            warn(u"données image introuvables pour la ressource %1"_s.arg(rid));
            m_fileKeys[rid] = {};
            return {};
        }
        const std::string key = storeResource(*file);
        m_fileKeys[rid] = {key, file->first.toStdString()};
        return key;
    }

    // Clé d'un fichier quelconque (son, vidéo, document ouvert par une action) ; `fileName` reçoit son nom.
    std::string fileKey(quint32 rid, std::string* fileName)
    {
        if (const auto it = m_fileKeys.find(rid); it != m_fileKeys.end()) {
            if (fileName) {
                *fileName = it->second.second;
            }
            return it->second.first;
        }
        const auto file = resourceFile(rid, resourceObject(rid));
        if (!file) {
            warn(u"données introuvables pour la ressource %1"_s.arg(rid));
            m_fileKeys[rid] = {};
            return {};
        }
        const std::string key = storeResource(*file);
        m_fileKeys[rid] = {key, file->first.toStdString()};
        if (fileName) {
            *fileName = file->first.toStdString();
        }
        return key;
    }

    std::optional<core::Page> readPage(const IndexNode& pageNode, const SizeF& startSize,
                                       QByteArray& thumbnail)
    {
        const QString where = u"page %1"_s.arg(pageNode.nodeId);
        QString error;
        const std::optional<QByteArray> bytes = readEntry(*m_root, u"%1.fpg"_s.arg(pageNode.nodeId), error);
        std::optional<BambooContainer> container =
            bytes ? BambooContainer::parse(*bytes, error) : std::nullopt;
        if (!container) {
            warn(u"%1 illisible : %2"_s.arg(where, error));
            return std::nullopt;
        }
        IndexNode tree = pageNode;
        const QString indexName = u"%1.in"_s.arg(pageNode.nodeId);
        if (container->contains(indexName)) {
            const std::optional<QByteArray> indexBytes = readEntry(*container, indexName, error);
            std::optional<IndexNode> parsed = indexBytes ? parseIndex(*indexBytes, error) : std::nullopt;
            if (!parsed) {
                warn(u"%1 : index illisible : %2"_s.arg(where, error));
                return std::nullopt;
            }
            tree = std::move(*parsed);
        }
        const std::optional<StreamObject> pageObject = load(*container, tree.storageName(), error);
        if (!pageObject) {
            warn(u"%1 illisible : %2"_s.arg(where, error));
            return std::nullopt;
        }
        core::Page sheet;
        SizeF dims = startSize;
        if (const std::optional<SizeF> own = asSize(pageObject->find(ids::pageKeys::kSheetSize))) {
            if (isUsablePageSize(*own)) {
                dims = *own;
            } else {
                warn(u"%1 : taille invalide (%2 × %3), taille par défaut"_s.arg(where)
                         .arg(own->width)
                         .arg(own->height));
            }
        }
        sheet.width = dims.width;
        sheet.height = dims.height;
        sheet.name = stringOf(&*pageObject, ids::pageKeys::kPageTitle);
        sheet.notes = stringOf(&*pageObject, ids::pageKeys::kPresenterNotes);
        sheet.grid.visible = flagOr(&*pageObject, ids::pageKeys::kShowGuides, false);
        const QString thumbName = u"%1.thm"_s.arg(pageNode.nodeId);
        if (m_root->contains(thumbName)) {
            thumbnail = readEntry(*m_root, thumbName, error).value_or(QByteArray());
        }

        std::vector<Pending> pending;
        int order = 0;
        std::optional<std::size_t> last; // dernier objet non-groupe : déclencheur des actions qui suivent
        for (const IndexNode& child : tree.children) {
            const QString entryName = child.storageName();
            if (!container->contains(entryName)) {
                warn(u"%1 : %2 absent (nœud %3)"_s.arg(where, entryName).arg(child.nodeId));
                continue;
            }
            const std::optional<StreamObject> o = load(*container, entryName, error);
            if (!o) {
                warn(u"%1 : %2 illisible : %3"_s.arg(where, entryName, error));
                continue;
            }
            if (ids::isActionKind(child.type)) {
                PendingAction action = actionFrom(child, *o);
                if (last) {
                    pending[*last].actions.push_back(std::move(action));
                } else {
                    warn(u"%1 : action sans objet déclencheur (nœud %2)"_s.arg(where).arg(child.nodeId));
                }
                continue;
            }
            if (child.type == ids::kind::kBackgroundObject) {
                applyPageFill(*container, *o, sheet);
                continue;
            }
            if (child.type == ids::kind::kGridObject) {
                applyGuides(*container, child, *o, sheet);
                continue;
            }
            if (child.type == ids::kind::kTagObject) {
                continue;
            }
            Pending p = toPending(*container, child, *o, where, 1);
            p.order = order++;
            const bool isGroup = !p.unread && std::get_if<core::Group>(&p.object) != nullptr;
            pending.push_back(std::move(p));
            if (!isGroup) {
                // Un objet non lu reste déclencheur, comme dans la référence : l'action qui le suit lui
                // revient et disparaît avec lui (il n'a pas d'identifiant ici), sans autre avertissement que
                // « non lu ».
                last = pending.size() - 1;
            }
        }

        std::stable_sort(pending.begin(), pending.end(), drawsBefore);
        std::vector<Pending*> flat;
        std::map<quint32, core::ObjectId> idOf;
        flatten(pending, core::Transform{}, flat, idOf, where);
        const auto resolve = [&idOf](quint32 treeNode) {
            const auto it = idOf.find(treeNode);
            return it == idOf.end() ? core::ObjectId{0} : it->second;
        };
        for (Pending* p : flat) {
            if (auto* group = std::get_if<core::Group>(&p->object)) {
                for (const quint32 member : p->members) {
                    if (const core::ObjectId id = resolve(member)) {
                        group->members.push_back(id);
                    }
                }
            }
            if (auto* link = std::get_if<core::Connector>(&p->object)) {
                link->objectA = resolve(p->connectorA);
                link->objectB = resolve(p->connectorB);
            }
            for (PendingAction& a : p->actions) {
                if (a.targetNode != 0) {
                    a.action.target = resolve(a.targetNode);
                    if (a.action.target == 0) {
                        warn(u"%1 : cible d'action introuvable (nœud %2)"_s.arg(where).arg(a.targetNode));
                    }
                }
                p->object.common.actions.push_back(std::move(a.action));
            }
            sheet.objects.push_back(std::move(p->object));
        }
        // Contrat du modèle : Page::objects est rangé par couche (model.h, isLayerOrdered ; nativejson.cpp le
        // rétablit au rechargement). La référence garde l'ordre de l'arbre (parent puis enfants, seuls les
        // frères sont triés) : un enfant d'une autre couche que son parent est ici déplacé, avec un
        // avertissement. Écart nommé de l'oracle synthétique (« enfants-ranges-par-couche »), gardé en
        // attente de décision (L2-C4).
        if (!std::is_sorted(sheet.objects.begin(), sheet.objects.end(),
                            [](const core::PageObject& a, const core::PageObject& b) {
                                return a.common.layer < b.common.layer;
                            })) {
            std::stable_sort(sheet.objects.begin(), sheet.objects.end(),
                             [](const core::PageObject& a, const core::PageObject& b) {
                                 return a.common.layer < b.common.layer;
                             });
            warn(u"%1 : enfants d'objets rangés par couche (ordre de l'arbre modifié)"_s.arg(where));
        }
        return sheet;
    }

    // Parent puis enfants (triés de même), transformations composées et revérifiées, identifiants dans
    // l'ordre de dessin.
    void flatten(std::vector<Pending>& list, const core::Transform& parent, std::vector<Pending*>& flat,
                 std::map<quint32, core::ObjectId>& idOf, const QString& where)
    {
        for (Pending& p : list) {
            core::Transform composed = core::compose(p.object.common.transform, parent);
            if (!isFinite(composed)) {
                warn(u"%1 : transformation non finie une fois composée avec le parent (nœud %2), identité"_s
                         .arg(where)
                         .arg(p.nodeId));
                composed = core::Transform{};
            }
            p.object.common.transform = composed;
            if (!p.unread) {
                p.object.common.id = m_nextId++;
                idOf[p.nodeId] = p.object.common.id;
                flat.push_back(&p);
            }
            std::stable_sort(p.children.begin(), p.children.end(), drawsBefore);
            flatten(p.children, p.object.common.transform, flat, idOf, where);
        }
    }

    PendingAction actionFrom(const IndexNode& treeNode, const StreamObject& o)
    {
        PendingAction p;
        p.action.sourceType = treeNode.type;
        quint16 command = 0;
        quint16 target = 0;
        switch (treeNode.type) {
        case ids::kind::kObjectAction:
            p.action.kind = core::ActionKind::OnObject;
            command = ids::actionKeys::kObjectActionCommand;
            target = ids::actionKeys::kObjectActionTarget;
            break;
        case ids::kind::kOpenFileAction: {
            p.action.kind = core::ActionKind::OpenExternal;
            command = ids::actionKeys::kOpenFileCommand;
            target = ids::actionKeys::kOpenFileTarget;
            const int rid = intOr(&o, ids::actionKeys::kOpenFileResource, 0);
            if (rid > 0) {
                p.action.resource = fileKey(static_cast<quint32>(rid), nullptr);
            }
            break;
        }
        case ids::kind::kGoToPageAction:
            p.action.kind = core::ActionKind::Navigate;
            command = ids::actionKeys::kGoToPageCommand;
            break;
        case ids::kind::kToolAction:
            command = ids::actionKeys::kToolCommand;
            break;
        case ids::kind::kShiftAction:
            command = ids::actionKeys::kShiftCommand;
            break;
        case ids::kind::kProgramAction:
            command = ids::actionKeys::kProgramCommand;
            break;
        case ids::kind::kTextAction:
            command = ids::actionKeys::kTextActionCommand;
            target = ids::actionKeys::kTextActionTarget;
            break;
        case ids::kind::kLaunchAction:
            command = ids::actionKeys::kLaunchCommand;
            break;
        default:
            break;
        }
        p.action.command = command ? intOr(&o, command, 0) : 0;
        if (target) {
            if (const std::optional<Pointer> ptr = asPointer(o.find(target)); ptr && !ptr->isNull()) {
                p.targetNode = ptr->nodeId;
            }
        }
        return p;
    }

    void applyPageFill(const BambooContainer& container, const StreamObject& fill, core::Page& sheet)
    {
        const StreamObject* pen = asObject(fill.find(ids::fillKeys::kFillStyle));
        if (intOr(pen, ids::penKeys::kFillPattern, 0) != 0) {
            sheet.background.fill = core::solidFill(colorOr(pen, ids::penKeys::kFillTint, 0xFFFFFFFFu));
            if (const int gradient = intOr(pen, ids::penKeys::kBlendDirection, 0); gradient != 0) {
                sheet.background.fill.kind = core::FillKind::Gradient;
                sheet.background.fill.gradient = gradientOf(gradient);
                sheet.background.fill.color2 = colorOr(pen, ids::penKeys::kSecondaryColor, 0xFF000000u);
            }
        }
        const int held = intOr(&fill, ids::fillKeys::kFillSource, 0);
        const std::optional<Pointer> ptr = asPointer(fill.find(ids::fillKeys::kPictureRef));
        if ((held == 1 || held == 2) && ptr && !ptr->isNull()) {
            const QString entryName = u"%1.io"_s.arg(ptr->storageId);
            const BambooContainer& from = container.contains(entryName) ? container : *m_root;
            QString error;
            const std::optional<StreamObject> picture = load(from, entryName, error);
            if (!picture) {
                warn(u"image de fond illisible : %1"_s.arg(error));
                return;
            }
            const int rid = intOr(&*picture, ids::imageKeys::kPixelsRef, 0);
            if (rid > 0) {
                sheet.background.image = imageKey(static_cast<quint32>(rid));
            }
        }
    }

    void applyGuides(const BambooContainer& container, const IndexNode& treeNode, const StreamObject& g,
                     core::Page& sheet)
    {
        // Plusieurs grilles sur une page : la dernière remplace les précédentes, niveaux compris, comme la
        // référence (chaque grille y remplace la grille de la page).
        const bool shown = sheet.grid.visible;
        sheet.grid = core::Grid{};
        sheet.grid.visible = shown;
        sheet.grid.snapEnabled = flagOr(&g, ids::guideKeys::kSnapEnabled, false);
        sheet.grid.drawnAbove = flagOr(&g, ids::guideKeys::kDrawnAbove, false);
        sheet.grid.previewZoom = numberOr(&g, ids::guideKeys::kPreviewZoom, sheet.grid.previewZoom);
        sheet.grid.baseZoom = numberOr(&g, ids::guideKeys::kBaseZoom, sheet.grid.baseZoom);
        sheet.grid.zoomIncrement = numberOr(&g, ids::guideKeys::kZoomIncrement, sheet.grid.zoomIncrement);
        for (const IndexNode& child : treeNode.children) {
            if (child.type != ids::kind::kGridLevelObject || !container.contains(child.storageName())) {
                continue;
            }
            QString error;
            const std::optional<StreamObject> s = load(container, child.storageName(), error);
            if (!s) {
                // Écart gardé (robustesse, L2-C4) : le niveau est sauté ; la référence perd toute la page.
                // L'oracle synthétique ne l'exerce pas.
                warn(u"niveau de grille illisible : %1"_s.arg(error));
                continue;
            }
            const StreamObject* l = &*s;
            core::GridLevel tier;
            // 0 vaut la valeur par défaut, comme dans la référence.
            const auto orDefault = [](double v, double fallback) { return v != 0 ? v : fallback; };
            tier.spacingX = orDefault(numberOr(l, ids::guideLevelKeys::kSpacingX, 32), 32);
            tier.spacingY = orDefault(numberOr(l, ids::guideLevelKeys::kSpacingY, 32), 32);
            tier.angle = orDefault(numberOr(l, ids::guideLevelKeys::kSlant, 30), 30);
            if (!isPositiveFinite(tier.spacingX) || !isPositiveFinite(tier.spacingY) ||
                !std::isfinite(tier.angle)) {
                warn(u"niveau de grille %1 : pas ou angle invalide, valeurs par défaut"_s.arg(child.nodeId));
                tier.spacingX = isPositiveFinite(tier.spacingX) ? tier.spacingX : 32;
                tier.spacingY = isPositiveFinite(tier.spacingY) ? tier.spacingY : 32;
                tier.angle = std::isfinite(tier.angle) ? tier.angle : 30;
            }
            tier.color = colorOr(l, ids::guideLevelKeys::kTint, 0xFF000000u) | 0xFF000000u;
            tier.visible = flagOr(l, ids::guideLevelKeys::kShown, true);
            tier.snap = flagOr(l, ids::guideLevelKeys::kMagnetic, true);
            tier.start = asPoint(l->find(ids::guideLevelKeys::kAnchor)).value_or(core::PointF{});
            tier.linesAlongX = flagOr(l, ids::guideLevelKeys::kLinesAlongX, true);
            tier.linesAlongY = flagOr(l, ids::guideLevelKeys::kLinesAlongY, true);
            const int pattern = intOr(l, ids::guideLevelKeys::kLayoutStyle, 1);
            tier.pattern = pattern >= 1 && pattern <= 3 ? static_cast<core::GridPattern>(pattern - 1)
                                                        : core::GridPattern::Normal;
            const int line = intOr(l, ids::guideLevelKeys::kMarkStyle, 3);
            tier.line =
                line >= 1 && line <= 6 ? static_cast<core::GridLine>(line - 1) : core::GridLine::Solid;
            sheet.grid.levels.push_back(tier);
        }
    }

    Pending toPending(const BambooContainer& container, const IndexNode& treeNode, const StreamObject& o,
                      const QString& where, int nesting)
    {
        const quint16 type = treeNode.type;
        Pending p;
        p.nodeId = treeNode.nodeId;
        const core::RectF bounds = asRect(o.find(ids::itemKeys::kLocalBox)).value_or(core::RectF{});
        const StreamObject* pen = asObject(o.find(ids::itemKeys::kPenAndFill));
        switch (type) {
        case ids::kind::kStrokeObject: {
            core::Stroke s;
            const core::Outline outline = outlineFrom(pen);
            s.baseWidth = numberOr(pen, ids::penKeys::kOutlineWidth, 1);
            s.color = outline.color;
            s.cap = outline.cap;
            const int mode = intOr(pen, ids::penKeys::kStrokeMode, 0);
            s.kind = mode == 1   ? core::StrokeKind::Highlighter
                     : mode == 2 ? core::StrokeKind::Revealer
                                 : core::StrokeKind::Pen;
            const int rid = intOr(&o, ids::strokeKeys::kPointsRef, 0);
            if (const StreamObject* res = rid > 0 ? resourceObject(static_cast<quint32>(rid)) : nullptr) {
                if (const Polyline* held = polylineOf(res->find(ids::resourceKeys::kPointList))) {
                    chargeItems<core::StrokePoint>(held->size());
                    s.points.reserve(held->size());
                    for (const core::PointF& q : *held) {
                        s.points.push_back(core::StrokePoint{q.x, q.y, 0.5f});
                    }
                }
            }
            p.object = std::move(s);
            // Seuls les traits portent une translucidité dans le format. Bornée à 0..255 : écart gardé
            // (L2-C4), la référence garde l'entier brut ; l'oracle synthétique ne l'exerce pas.
            p.object.common.translucency =
                static_cast<std::uint8_t>(std::clamp(intOr(&o, ids::strokeKeys::kSeeThrough, 0), 0, 255));
            break;
        }
        case ids::kind::kLineObject: {
            core::Line l;
            l.pen = outlineFrom(pen);
            const int rid = intOr(&o, ids::lineKeys::kPointsRef, 0);
            if (const StreamObject* res = rid > 0 ? resourceObject(static_cast<quint32>(rid)) : nullptr) {
                if (const Polyline* held = polylineOf(res->find(ids::resourceKeys::kPointList))) {
                    chargeItems<core::PointF>(held->size());
                    l.points = *held;
                }
            }
            p.object = std::move(l);
            break;
        }
        case ids::kind::kShapeObject: {
            core::Shape s;
            s.kind = core::ShapeKind::Path;
            s.bounds = bounds;
            const int rid = intOr(&o, ids::shapeKeys::kOutlineRef, 0);
            if (const StreamObject* res = rid > 0 ? resourceObject(static_cast<quint32>(rid)) : nullptr) {
                if (const std::vector<Polyline>* held =
                        polylinesOf(res->find(ids::resourceKeys::kPathOutlines))) {
                    std::size_t count = held->size();
                    for (const Polyline& outline : *held) {
                        count += outline.size();
                    }
                    chargeItems<core::PointF>(count);
                    s.paths = *held;
                }
            }
            if (s.paths.empty()) {
                if (const Polyline* local = polylineOf(o.find(ids::shapeKeys::kInlinePoints));
                    local && !local->empty()) {
                    chargeItems<core::PointF>(local->size());
                    s.paths.push_back(*local);
                }
            }
            s.open = flagOr(&o, ids::shapeKeys::kUnclosed, false);
            s.windingFill = intOr(&o, ids::shapeKeys::kWindingRule, 0) == 2;
            s.pen = outlineFrom(pen);
            s.brush = fillFrom(pen);
            p.object = std::move(s);
            break;
        }
        case ids::kind::kImageObject: {
            core::ImageObject im;
            im.bounds = bounds;
            const int rid = intOr(&o, ids::imageKeys::kPixelsRef, 0);
            if (rid > 0) {
                im.resource = imageKey(static_cast<quint32>(rid));
                if (const StreamObject* res = resourceObject(static_cast<quint32>(rid))) {
                    if (const std::optional<SizeF> dims = asSize(res->find(ids::resourceKeys::kPixelSize));
                        dims && std::isfinite(dims->width) && std::isfinite(dims->height) &&
                        dims->width >= 0 && dims->height >= 0 && dims->width < 1e6 && dims->height < 1e6) {
                        im.sourceWidth = static_cast<std::int32_t>(dims->width);
                        im.sourceHeight = static_cast<std::int32_t>(dims->height);
                    }
                }
            }
            if (flagOr(&o, ids::imageKeys::kKeyed, false)) {
                im.hasKeyColor = true;
                im.keyColor = colorOr(&o, ids::imageKeys::kKeyColor, 0xFF000000u);
            }
            p.object = std::move(im);
            break;
        }
        case ids::kind::kTextObject: {
            core::Text t;
            t.bounds = bounds;
            const int rid = intOr(&o, ids::textKeys::kContentRef, 0);
            if (const StreamObject* res = rid > 0 ? resourceObject(static_cast<quint32>(rid)) : nullptr) {
                const QString markup = asString(res->find(ids::resourceKeys::kMarkup));
                chargeModel(2 * markup.size());
                t.html = markup.toStdString();
                for (const StreamObject* r : asObjects(res->find(ids::resourceKeys::kLayoutRuns))) {
                    if (!r) {
                        continue;
                    }
                    const QString characters = asString(r->find(ids::runKeys::kCharacters));
                    const QString typeface = asString(r->find(ids::runKeys::kTypeface));
                    chargeModel(2 * (characters.size() + typeface.size()) +
                                static_cast<qint64>(sizeof(core::TextSpan)));
                    core::TextSpan run;
                    run.text = characters.toStdString();
                    run.fontFamily = typeface.toStdString();
                    // 0 vaut 12 et la graisse 0 vaut 50 (qt6Weight), comme dans la référence.
                    run.pointSize = numberOr(r, ids::runKeys::kPointHeight, 12);
                    if (run.pointSize == 0) {
                        run.pointSize = 12;
                    }
                    run.weight = qt6Weight(intOr(r, ids::runKeys::kBoldness, 50));
                    run.italic = flagOr(r, ids::runKeys::kSlanted, false);
                    run.underline = flagOr(r, ids::runKeys::kUnderlined, false);
                    run.strikeOut = flagOr(r, ids::runKeys::kStruckThrough, false);
                    run.color = colorOr(r, ids::runKeys::kGlyphColor, 0xFF000000u);
                    run.baseline = asPoint(r->find(ids::runKeys::kBaselinePoint)).value_or(core::PointF{});
                    run.bounds = asRect(r->find(ids::runKeys::kBox)).value_or(core::RectF{});
                    t.runs.push_back(std::move(run));
                }
            }
            p.object = std::move(t);
            break;
        }
        case ids::kind::kGroupObject:
            p.object = core::Group{};
            p.members = treeNode.refs;
            break;
        case ids::kind::kMediaObject: {
            core::Media m;
            m.bounds = bounds;
            if (const int poster = intOr(&o, ids::mediaKeys::kPosterRef, 0); poster > 0) {
                m.placeholder = imageKey(static_cast<quint32>(poster));
            }
            if (const int clip = intOr(&o, ids::mediaKeys::kClipRef, 0); clip > 0) {
                m.media = fileKey(static_cast<quint32>(clip), &m.fileName);
            }
            m.playOnOpen = flagOr(&o, ids::mediaKeys::kPlayOnOpen, false);
            m.loop = flagOr(&o, ids::mediaKeys::kRepeat, false);
            p.object = std::move(m);
            break;
        }
        case ids::kind::kConnectorObject: {
            core::Connector link;
            link.firstEnd = asPoint(o.find(ids::connectorKeys::kFromPoint)).value_or(core::PointF{});
            link.secondEnd = asPoint(o.find(ids::connectorKeys::kToPoint)).value_or(core::PointF{});
            link.pen = outlineFrom(pen);
            if (const std::optional<Pointer> a = asPointer(o.find(ids::connectorKeys::kFromItem));
                a && !a->isNull()) {
                p.connectorA = a->nodeId;
            }
            if (const std::optional<Pointer> b = asPointer(o.find(ids::connectorKeys::kToItem));
                b && !b->isNull()) {
                p.connectorB = b->nodeId;
            }
            p.object = std::move(link);
            break;
        }
        // Texte de haut en bas (D9) : l'original en garde une image rendue et une source opaque, pas le texte
        // en clair. Notre documentation du format donne son genre et son cadre (propriété commune, lu plus
        // haut : il sert au miroir de la transformation de ses enfants), pas le champ de son image. Il est
        // donc déclaré non lu comme tout genre non lu : ses enfants entrent, et aucun texte n'est tiré de sa
        // source. Le lire comme un texte (kTextObject) fabriquerait un texte que le fichier ne contient pas
        // (tst_formats_lot4, flipchartVerticalTextRead).
        case ids::kind::kTopToBottomTextObject:
        default:
            // Signalé ; la référence se tait (écart nommé « objet-non-lu-signale » de l'oracle synthétique).
            warn(u"%1 : objet de genre %2 non lu (nœud %3)"_s.arg(where).arg(type).arg(treeNode.nodeId));
            p.unread = true;
            break;
        }

        core::ObjectCommon& c = p.object.common;
        c.name = stringOf(&o, ids::itemKeys::kItemTitle);
        p.layer = intOr(&o, ids::itemKeys::kPlane, 1);
        c.layer = p.layer == 0 ? core::Layer::Bottom : p.layer == 2 ? core::Layer::Top : core::Layer::Middle;
        // Couche hors 0..2 : couche intermédiaire, que le modèle sait représenter. Écart gardé (L2-C4) : la
        // référence garde la valeur brute ; l'oracle synthétique ne l'exerce pas.
        // Un objet non lu n'entre pas dans le modèle : sa couche n'est pas signalée (la référence se tait).
        if (!p.unread && (p.layer < 0 || p.layer > 2)) {
            warn(u"%1 : couche %2 inconnue (nœud %3), couche intermédiaire"_s.arg(where).arg(p.layer).arg(
                treeNode.nodeId));
        }
        p.depth = numberOr(&o, ids::itemKeys::kStackOrder, 0);
        if (!std::isfinite(p.depth)) {
            p.depth = 0; // NaN romprait l'ordre strict du tri
        }
        c.visible = flagOr(&o, ids::itemKeys::kShown, true);
        c.locked = flagOr(&o, ids::itemKeys::kFrozen, false);
        c.restrictions.movable = flagOr(&o, ids::itemKeys::kMovable, true);
        c.restrictions.resizable = flagOr(&o, ids::itemKeys::kResizable, true);
        c.restrictions.rotatable = flagOr(&o, ids::itemKeys::kRotatable, true);
        c.transform = localTransform(o, bounds);
        if (!isFinite(c.transform)) {
            warn(u"%1 : transformation non finie (nœud %2), identité"_s.arg(where).arg(treeNode.nodeId));
            c.transform = core::Transform{};
        }
        // L'étiquette d'un connecteur est sa propriété 33 : sa 34 désigne une extrémité.
        const quint16 tagKey =
            type == ids::kind::kConnectorObject ? ids::itemKeys::kConnectorTag : ids::itemKeys::kTag;
        if (const StreamObject* tag = asObject(o.find(tagKey))) {
            c.label = stringOf(tag, ids::tagKeys::kTagText);
        }

        if (nesting >= kMaxNesting && !treeNode.children.empty()) {
            warn(u"%1 : objets imbriqués au-delà de %2 niveaux sautés (nœud %3)"_s.arg(where)
                     .arg(kMaxNesting)
                     .arg(treeNode.nodeId));
            return p;
        }
        int order = 0;
        for (const IndexNode& child : treeNode.children) {
            QString error;
            const std::optional<StreamObject> co = load(container, child.storageName(), error);
            if (!co) {
                warn(u"%1 : %2 illisible : %3"_s.arg(where, child.storageName(), error));
                continue;
            }
            if (ids::isActionKind(child.type)) {
                p.actions.push_back(actionFrom(child, *co));
                continue;
            }
            if (child.type == ids::kind::kTagObject) {
                continue;
            }
            Pending cp = toPending(container, child, *co, where, nesting + 1);
            cp.order = order++;
            p.children.push_back(std::move(cp));
        }
        return p;
    }

    const QByteArray& m_data;
    FlipchartLimits m_limits;
    std::optional<BambooContainer> m_root;
    std::map<quint32, ResourceItem> m_register;
    std::map<quint32, std::shared_ptr<const StreamObject>> m_resourceObjects;
    std::map<quint32, std::pair<std::string, std::string>> m_fileKeys; // ressource → (clé, nom de fichier)
    core::Document m_store; // ne sert qu'à ranger les ressources par empreinte
    QStringList m_warnings;
    core::ObjectId m_nextId = 1;
    qint64 m_decompressed = 0;
    qint64 m_modelBytes = 0;
};

} // namespace

FlipchartReadResult readFlipchart(const QByteArray& data, const FlipchartLimits& limits)
{
    try {
        return Reader(data, limits).read();
    } catch (const LimitExceeded& e) {
        FlipchartReadResult result;
        result.error = e.message;
        return result;
    } catch (const std::bad_alloc&) {
        FlipchartReadResult result;
        result.error = u"paperboard illisible : mémoire insuffisante"_s;
        return result;
    }
}

bool looksLikeFlipchart(const QByteArray& head) noexcept
{
    return head.startsWith(" Bamboo ");
}

} // namespace actcraft::formats::flipchart
