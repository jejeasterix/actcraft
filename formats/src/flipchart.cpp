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
#include <optional>
#include <utility>

namespace actcraft::formats::flipchart {

using namespace Qt::StringLiterals;

namespace {

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

std::string text(const StreamObject* o, quint16 id)
{
    return o ? asString(o->find(id)).toStdString() : std::string();
}

// Extension devinée par la signature, sinon par le nom (comme _sniff de la référence).
std::string extensionOf(const QByteArray& data, const QString& name)
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
    const qsizetype dot = name.lastIndexOf(u'.');
    return dot >= 0 ? name.mid(dot + 1).toLower().toStdString() : std::string("bin");
}

// Graisse Qt 4 (0 à 99, 50 normal, 75 gras) → Qt 6 (100 à 900) : la valeur de la table la plus proche.
std::int32_t qt6Weight(int w)
{
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

core::Outline outlineFrom(const StreamObject* ink)
{
    core::Outline o;
    if (!ink) {
        return o;
    }
    const int style = intOr(ink, ids::ink::PenStyle, 1);
    o.style = style >= 0 && style <= 5 ? static_cast<core::PenStyle>(style) : core::PenStyle::Solid;
    o.color = colorOr(ink, ids::ink::PenColor, 0xFF000000u);
    o.width = numberOr(ink, ids::ink::PenWidth, 1);
    if (o.width <= 0) {
        o.style = core::PenStyle::None;
    }
    const int cap = intOr(ink, ids::ink::PenCap, 32);
    o.cap = cap == 0 ? core::PenCap::Flat : cap == 16 ? core::PenCap::Square : core::PenCap::Round;
    const int join = intOr(ink, ids::ink::PenJoin, 128);
    o.join = join == 0 ? core::PenJoin::Miter : join == 64 ? core::PenJoin::Bevel : core::PenJoin::Round;
    return o;
}

core::GradientKind gradientOf(int value)
{
    return value >= 2 && value <= 5 ? static_cast<core::GradientKind>(value) : core::GradientKind::Vertical;
}

core::Fill fillFrom(const StreamObject* ink)
{
    core::Fill f;
    f.color = colorOr(ink, ids::ink::BrushColor, 0xFFFFFFFFu);
    f.color2 = colorOr(ink, ids::ink::BrushColor2, 0xFF000000u);
    f.opaqueBackground = intOr(ink, ids::ink::BackgroundMode, 0) != 0;
    f.backgroundColor = colorOr(ink, ids::ink::BackgroundColor, 0xFF000000u);
    const int style = intOr(ink, ids::ink::BrushStyle, 0);
    const int gradient = intOr(ink, ids::ink::Gradient, 0);
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

// § 7.4 : translate(t + o) ; rotate(angle) ; scale(sx, sy) ; translate(-o) ; puis miroir éventuel.
// Objets imbriqués lus au plus sur 32 niveaux : la lecture est récursive et l'index en admet 256 (en
// débogage, 200 niveaux débordaient la pile du fil principal). Les paperboards réels n'imbriquent que
// quelques niveaux.
constexpr int kMaxNesting = 32;

bool isFinite(const core::Transform& m) noexcept
{
    return std::isfinite(m.m11) && std::isfinite(m.m12) && std::isfinite(m.m21) && std::isfinite(m.m22) &&
           std::isfinite(m.dx) && std::isfinite(m.dy);
}

core::Transform localTransform(const StreamObject& o, const core::RectF& bounds)
{
    const StreamObject* t = asObject(o.find(ids::object::Transform));
    if (!t) {
        return {};
    }
    const core::PointF translate = asPoint(t->find(ids::transform::Translate)).value_or(core::PointF{});
    const core::PointF scale = asPoint(t->find(ids::transform::Scale)).value_or(core::PointF{1, 1});
    const double angle = numberOr(t, ids::transform::Angle, 0);
    const bool mirrored = flagOr(t, ids::transform::Mirrored, false);
    const bool flipped = flagOr(t, ids::transform::Flipped, false);
    core::PointF origin;
    if (const StreamObject* rot = asObject(o.find(ids::object::RotateOrigin))) {
        origin = asPoint(rot->find(ids::origin::Point)).value_or(core::PointF{});
    }
    core::Transform m = core::compose(
        core::compose(core::compose(core::translation(-origin.x, -origin.y), core::scaling(scale.x, scale.y)),
                      core::rotation(angle)),
        core::translation(translate.x + origin.x, translate.y + origin.y));
    if (mirrored || flipped) {
        const core::Transform flip =
            core::compose(core::scaling(mirrored ? -1 : 1, flipped ? -1 : 1),
                          core::translation(mirrored ? bounds.width : 0, flipped ? bounds.height : 0));
        m = core::compose(flip, m);
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
    explicit Reader(const QByteArray& data) : m_data(data) {}

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
        for (const QString& name : m_root->names()) {
            if (name.endsWith(u".in"_s)) {
                indexName = name;
                break;
            }
        }
        std::optional<IndexNode> tree;
        if (!indexName.isEmpty()) {
            if (const std::optional<QByteArray> bytes = m_root->read(indexName, error)) {
                tree = parseIndex(*bytes, error);
            }
        }
        if (!tree) {
            result.error = u"index racine absent ou illisible %1"_s.arg(error);
            return result;
        }
        const std::optional<StreamObject> fc = load(*m_root, tree->storageName(), error);
        if (!fc) {
            result.error = u"objet paperboard illisible : %1"_s.arg(error);
            return result;
        }
        if (const StreamObject* registry = asObject(fc->find(ids::flipchart::ResourceRegister))) {
            for (const auto& [id, value] : registry->properties) {
                for (const ResourceItem& item : asResourceItems(&value)) {
                    m_register[item.id] = item;
                }
            }
        }
        std::string title = text(&*fc, ids::flipchart::Title);
        if (title.empty()) {
            title = text(&*fc, ids::flipchart::Name);
        }
        const SizeF defaultSize =
            asSize(fc->find(ids::flipchart::DefaultPageSize)).value_or(SizeF{1024, 768});
        std::vector<core::Page> pages;
        for (const IndexNode& child : tree->children) {
            if (child.type != ids::type::Page) {
                continue;
            }
            QByteArray thumbnail;
            if (std::optional<core::Page> page = readPage(child, defaultSize, thumbnail)) {
                pages.push_back(std::move(*page));
                result.thumbnails.push_back(std::move(thumbnail));
            }
        }
        if (pages.empty()) {
            result.error = u"aucune page lisible"_s;
            result.warnings = m_warnings;
            return result;
        }
        core::Document document(std::move(pages));
        document.setTitle(std::move(title));
        for (const auto& [key, resource] : m_resources.resources()) {
            document.addResource(resource.data, resource.extension);
        }
        result.document = std::move(document);
        result.warnings = m_warnings;
        result.ok = true;
        return result;
    }

private:
    std::optional<StreamObject> load(const BambooContainer& c, const QString& name, QString& error)
    {
        const std::optional<QByteArray> bytes = c.read(name, error);
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
            if (const QString original = asString(res->find(ids::resource::OriginalName));
                !original.isEmpty()) {
                names << original;
            }
        }
        for (const char* ext : {"png", "jpg", "ird", "gif", "bmp", "tif"}) {
            names << u"%1.%2"_s.arg(rid).arg(QLatin1String(ext));
        }
        for (const QString& name : names) {
            if (!m_root->contains(name)) {
                continue;
            }
            QString error;
            if (std::optional<QByteArray> data = m_root->read(name, error)) {
                return std::make_pair(name, std::move(*data));
            }
            warn(u"données %1 illisibles : %2"_s.arg(name, error));
        }
        return std::nullopt;
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
        const std::string key = m_resources.addResource(file->second, extensionOf(file->second, file->first));
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
        const std::string key = m_resources.addResource(file->second, extensionOf(file->second, file->first));
        m_fileKeys[rid] = {key, file->first.toStdString()};
        if (fileName) {
            *fileName = file->first.toStdString();
        }
        return key;
    }

    std::optional<core::Page> readPage(const IndexNode& pageNode, const SizeF& defaultSize,
                                       QByteArray& thumbnail)
    {
        const QString where = u"page %1"_s.arg(pageNode.nodeId);
        QString error;
        const std::optional<QByteArray> bytes = m_root->read(u"%1.fpg"_s.arg(pageNode.nodeId), error);
        std::optional<BambooContainer> container =
            bytes ? BambooContainer::parse(*bytes, error) : std::nullopt;
        if (!container) {
            warn(u"%1 illisible : %2"_s.arg(where, error));
            return std::nullopt;
        }
        IndexNode tree = pageNode;
        const QString indexName = u"%1.in"_s.arg(pageNode.nodeId);
        if (container->contains(indexName)) {
            const std::optional<QByteArray> indexBytes = container->read(indexName, error);
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
        core::Page page;
        const SizeF size = asSize(pageObject->find(ids::page::Size)).value_or(defaultSize);
        page.width = size.width;
        page.height = size.height;
        page.name = text(&*pageObject, ids::page::Name);
        page.notes = text(&*pageObject, ids::page::Notes);
        page.grid.visible = flagOr(&*pageObject, ids::page::GridVisible, false);
        const QString thumbName = u"%1.thm"_s.arg(pageNode.nodeId);
        if (m_root->contains(thumbName)) {
            thumbnail = m_root->read(thumbName, error).value_or(QByteArray());
        }

        std::vector<Pending> pending;
        int order = 0;
        std::optional<std::size_t> last; // dernier objet non-groupe : déclencheur des actions qui suivent
        for (const IndexNode& child : tree.children) {
            const QString name = child.storageName();
            if (!container->contains(name)) {
                warn(u"%1 : %2 absent (nœud %3)"_s.arg(where, name).arg(child.nodeId));
                continue;
            }
            const std::optional<StreamObject> o = load(*container, name, error);
            if (!o) {
                warn(u"%1 : %2 illisible : %3"_s.arg(where, name, error));
                continue;
            }
            if (ids::isAction(child.type)) {
                PendingAction action = actionFrom(child, *o);
                if (last) {
                    pending[*last].actions.push_back(std::move(action));
                } else {
                    warn(u"%1 : action sans objet déclencheur (nœud %2)"_s.arg(where).arg(child.nodeId));
                }
                continue;
            }
            if (child.type == ids::type::Background) {
                background(*container, *o, page);
                continue;
            }
            if (child.type == ids::type::Grid) {
                grid(*container, child, *o, page);
                continue;
            }
            if (child.type == ids::type::Label) {
                continue;
            }
            std::optional<Pending> p = toPending(*container, child, *o, where, 1);
            if (!p) {
                last.reset(); // comme la référence : l'action suivante viserait cet objet non lu
                continue;
            }
            p->order = order++;
            const bool isGroup = std::get_if<core::Group>(&p->object) != nullptr;
            pending.push_back(std::move(*p));
            if (!isGroup) {
                last = pending.size() - 1;
            }
        }

        std::stable_sort(pending.begin(), pending.end(), drawsBefore);
        std::vector<Pending*> flat;
        std::map<quint32, core::ObjectId> idOf;
        flatten(pending, core::Transform{}, flat, idOf);
        const auto resolve = [&idOf](quint32 node) {
            const auto it = idOf.find(node);
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
            if (auto* c = std::get_if<core::Connector>(&p->object)) {
                c->objectA = resolve(p->connectorA);
                c->objectB = resolve(p->connectorB);
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
            page.objects.push_back(std::move(p->object));
        }
        if (!std::is_sorted(page.objects.begin(), page.objects.end(),
                            [](const core::PageObject& a, const core::PageObject& b) {
                                return a.common.layer < b.common.layer;
                            })) {
            std::stable_sort(page.objects.begin(), page.objects.end(),
                             [](const core::PageObject& a, const core::PageObject& b) {
                                 return a.common.layer < b.common.layer;
                             });
            warn(u"%1 : enfants d'objets rangés par couche (ordre de l'arbre modifié)"_s.arg(where));
        }
        return page;
    }

    // Parent puis enfants (triés de même), transformations composées, identifiants dans l'ordre de dessin.
    void flatten(std::vector<Pending>& list, const core::Transform& parent, std::vector<Pending*>& flat,
                 std::map<quint32, core::ObjectId>& idOf)
    {
        for (Pending& p : list) {
            p.object.common.transform = core::compose(p.object.common.transform, parent);
            p.object.common.id = m_nextId++;
            idOf[p.nodeId] = p.object.common.id;
            flat.push_back(&p);
            std::stable_sort(p.children.begin(), p.children.end(), drawsBefore);
            flatten(p.children, p.object.common.transform, flat, idOf);
        }
    }

    PendingAction actionFrom(const IndexNode& node, const StreamObject& o)
    {
        PendingAction p;
        p.action.sourceType = node.type;
        quint16 command = 0;
        quint16 target = 0;
        switch (node.type) {
        case ids::type::ActionTargetObject:
            p.action.kind = core::ActionKind::OnObject;
            command = ids::action::TargetObjectCommand;
            target = ids::action::TargetObjectTarget;
            break;
        case ids::type::ActionOpenFile: {
            p.action.kind = core::ActionKind::OpenExternal;
            command = ids::action::OpenFileCommand;
            target = ids::action::OpenFileTarget;
            const int rid = intOr(&o, ids::action::OpenFileResource, 0);
            if (rid > 0) {
                p.action.resource = fileKey(static_cast<quint32>(rid), nullptr);
            }
            break;
        }
        case ids::type::ActionGoToPage:
            p.action.kind = core::ActionKind::Navigate;
            command = ids::action::GoToPageCommand;
            break;
        case ids::type::ActionTool:
            command = ids::action::ToolCommand;
            break;
        case ids::type::ActionShift:
            command = ids::action::ShiftCommand;
            break;
        case ids::type::ActionProgram:
            command = ids::action::ProgramCommand;
            break;
        case ids::type::ActionTargetText:
            command = ids::action::TargetTextCommand;
            target = ids::action::TargetTextTarget;
            break;
        case ids::type::ActionLaunch:
            command = ids::action::LaunchCommand;
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

    void background(const BambooContainer& container, const StreamObject& bg, core::Page& page)
    {
        const StreamObject* ink = asObject(bg.find(ids::background::Ink));
        if (intOr(ink, ids::ink::BrushStyle, 0) != 0) {
            page.background.fill = core::solidFill(colorOr(ink, ids::ink::BrushColor, 0xFFFFFFFFu));
            if (const int gradient = intOr(ink, ids::ink::Gradient, 0); gradient != 0) {
                page.background.fill.kind = core::FillKind::Gradient;
                page.background.fill.gradient = gradientOf(gradient);
                page.background.fill.color2 = colorOr(ink, ids::ink::BrushColor2, 0xFF000000u);
            }
        }
        const int kind = intOr(&bg, ids::background::Kind, 0);
        const std::optional<Pointer> ptr = asPointer(bg.find(ids::background::Image));
        if ((kind == 1 || kind == 2) && ptr && !ptr->isNull()) {
            const QString name = u"%1.io"_s.arg(ptr->storageId);
            const BambooContainer& source = container.contains(name) ? container : *m_root;
            QString error;
            const std::optional<StreamObject> image = load(source, name, error);
            if (!image) {
                warn(u"image de fond illisible : %1"_s.arg(error));
                return;
            }
            const int rid = intOr(&*image, ids::image::Resource, 0);
            if (rid > 0) {
                page.background.image = imageKey(static_cast<quint32>(rid));
            }
        }
    }

    void grid(const BambooContainer& container, const IndexNode& node, const StreamObject& g,
              core::Page& page)
    {
        page.grid.allowSnap = flagOr(&g, ids::grid::AllowSnap, false);
        page.grid.onTop = flagOr(&g, ids::grid::OnTop, false);
        page.grid.thumbnailScale = numberOr(&g, ids::grid::ThumbnailScale, page.grid.thumbnailScale);
        page.grid.defaultScale = numberOr(&g, ids::grid::DefaultScale, page.grid.defaultScale);
        page.grid.scaleStep = numberOr(&g, ids::grid::ScaleStep, page.grid.scaleStep);
        for (const IndexNode& child : node.children) {
            if (child.type != ids::type::GridLevel || !container.contains(child.storageName())) {
                continue;
            }
            QString error;
            const std::optional<StreamObject> s = load(container, child.storageName(), error);
            if (!s) {
                warn(u"niveau de grille illisible : %1"_s.arg(error));
                continue;
            }
            const StreamObject* l = &*s;
            core::GridLevel level;
            const auto orDefault = [](double v, double fallback) { return v != 0 ? v : fallback; };
            level.stepX = orDefault(numberOr(l, ids::gridlevel::StepX, 32), 32);
            level.stepY = orDefault(numberOr(l, ids::gridlevel::StepY, 32), 32);
            level.color = colorOr(l, ids::gridlevel::Color, 0xFF000000u) | 0xFF000000u;
            level.visible = flagOr(l, ids::gridlevel::Visible, true);
            level.snap = flagOr(l, ids::gridlevel::Snap, true);
            level.start = asPoint(l->find(ids::gridlevel::Start)).value_or(core::PointF{});
            level.angle = orDefault(numberOr(l, ids::gridlevel::Angle, 30), 30);
            level.drawX = flagOr(l, ids::gridlevel::DrawX, true);
            level.drawY = flagOr(l, ids::gridlevel::DrawY, true);
            const int pattern = intOr(l, ids::gridlevel::Pattern, 1);
            level.pattern = pattern >= 1 && pattern <= 3 ? static_cast<core::GridPattern>(pattern - 1)
                                                         : core::GridPattern::Normal;
            const int line = intOr(l, ids::gridlevel::LineStyle, 3);
            level.line =
                line >= 1 && line <= 6 ? static_cast<core::GridLine>(line - 1) : core::GridLine::Solid;
            page.grid.levels.push_back(level);
        }
    }

    std::optional<Pending> toPending(const BambooContainer& container, const IndexNode& node,
                                     const StreamObject& o, const QString& where, int nesting)
    {
        const quint16 type = node.type;
        Pending p;
        p.nodeId = node.nodeId;
        const core::RectF bounds = asRect(o.find(ids::object::BoundingRect)).value_or(core::RectF{});
        const StreamObject* ink = asObject(o.find(ids::object::Ink));
        switch (type) {
        case ids::type::Annotation: {
            core::Stroke s;
            const core::Outline pen = outlineFrom(ink);
            s.baseWidth = numberOr(ink, ids::ink::PenWidth, 1);
            s.color = pen.color;
            s.cap = pen.cap;
            const int kind = intOr(ink, ids::ink::Kind, 0);
            s.kind = kind == 1   ? core::StrokeKind::Highlighter
                     : kind == 2 ? core::StrokeKind::Revealer
                                 : core::StrokeKind::Pen;
            const int rid = intOr(&o, ids::annotation::Resource, 0);
            if (const StreamObject* res = rid > 0 ? resourceObject(static_cast<quint32>(rid)) : nullptr) {
                for (const core::PointF& q : asPolyline(res->find(ids::resource::Points))) {
                    s.points.push_back(core::StrokePoint{q.x, q.y, 0.5f});
                }
            }
            p.object = std::move(s);
            p.object.common.translucency =
                static_cast<std::uint8_t>(std::clamp(intOr(&o, ids::annotation::Translucency, 0), 0, 255));
            break;
        }
        case ids::type::Line: {
            core::Line l;
            l.pen = outlineFrom(ink);
            const int rid = intOr(&o, ids::line::Resource, 0);
            if (const StreamObject* res = rid > 0 ? resourceObject(static_cast<quint32>(rid)) : nullptr) {
                l.points = asPolyline(res->find(ids::resource::Points));
            }
            p.object = std::move(l);
            break;
        }
        case ids::type::Shape: {
            core::Shape s;
            s.kind = core::ShapeKind::Path;
            s.bounds = bounds;
            const int rid = intOr(&o, ids::shape::Resource, 0);
            if (const StreamObject* res = rid > 0 ? resourceObject(static_cast<quint32>(rid)) : nullptr) {
                s.paths = asPolylines(res->find(ids::resource::Polygons));
            }
            if (s.paths.empty()) {
                if (Polyline local = asPolyline(o.find(ids::shape::LocalPoints)); !local.empty()) {
                    s.paths.push_back(std::move(local));
                }
            }
            s.open = flagOr(&o, ids::shape::Open, false);
            s.windingFill = intOr(&o, ids::shape::FillMode, 0) == 2;
            s.pen = outlineFrom(ink);
            s.brush = fillFrom(ink);
            p.object = std::move(s);
            break;
        }
        case ids::type::Image: {
            core::ImageObject im;
            im.bounds = bounds;
            const int rid = intOr(&o, ids::image::Resource, 0);
            if (rid > 0) {
                im.resource = imageKey(static_cast<quint32>(rid));
                if (const StreamObject* res = resourceObject(static_cast<quint32>(rid))) {
                    if (const std::optional<SizeF> size = asSize(res->find(ids::resource::ImageSize));
                        size && std::isfinite(size->width) && std::isfinite(size->height) &&
                        size->width >= 0 && size->height >= 0 && size->width < 1e6 && size->height < 1e6) {
                        im.sourceWidth = static_cast<std::int32_t>(size->width);
                        im.sourceHeight = static_cast<std::int32_t>(size->height);
                    }
                }
            }
            if (flagOr(&o, ids::image::Transparent, false)) {
                im.hasTransparentColor = true;
                im.transparentColor = colorOr(&o, ids::image::TransparentColor, 0xFF000000u);
            }
            p.object = std::move(im);
            break;
        }
        case ids::type::Text: {
            core::Text t;
            t.bounds = bounds;
            const int rid = intOr(&o, ids::text::Resource, 0);
            if (const StreamObject* res = rid > 0 ? resourceObject(static_cast<quint32>(rid)) : nullptr) {
                t.html = text(res, ids::resource::TextHtml);
                for (const StreamObject* r : asObjects(res->find(ids::resource::TextRuns))) {
                    core::TextRun run;
                    run.text = text(r, ids::run::Text);
                    run.fontFamily = text(r, ids::run::Family);
                    run.pointSize = numberOr(r, ids::run::PointSize, 12);
                    run.weight = qt6Weight(intOr(r, ids::run::Weight, 50));
                    run.italic = flagOr(r, ids::run::Italic, false);
                    run.underline = flagOr(r, ids::run::Underline, false);
                    run.strikeOut = flagOr(r, ids::run::StrikeOut, false);
                    run.color = colorOr(r, ids::run::Color, 0xFF000000u);
                    run.baseline = asPoint(r->find(ids::run::Baseline)).value_or(core::PointF{});
                    run.bounds = asRect(r->find(ids::run::Bounds)).value_or(core::RectF{});
                    t.runs.push_back(std::move(run));
                }
            }
            p.object = std::move(t);
            break;
        }
        case ids::type::Group:
            p.object = core::Group{};
            p.members = node.refs;
            break;
        case ids::type::Media: {
            core::Media m;
            m.bounds = bounds;
            if (const int placeholder = intOr(&o, ids::media::Placeholder, 0); placeholder > 0) {
                m.placeholder = imageKey(static_cast<quint32>(placeholder));
            }
            if (const int file = intOr(&o, ids::media::File, 0); file > 0) {
                m.media = fileKey(static_cast<quint32>(file), &m.fileName);
            }
            m.autoPlay = flagOr(&o, ids::media::AutoPlay, false);
            m.loop = flagOr(&o, ids::media::Loop, false);
            p.object = std::move(m);
            break;
        }
        case ids::type::Connector: {
            core::Connector c;
            c.endA = asPoint(o.find(ids::connector::PointA)).value_or(core::PointF{});
            c.endB = asPoint(o.find(ids::connector::PointB)).value_or(core::PointF{});
            c.pen = outlineFrom(ink);
            if (const std::optional<Pointer> a = asPointer(o.find(ids::connector::ObjectA));
                a && !a->isNull()) {
                p.connectorA = a->nodeId;
            }
            if (const std::optional<Pointer> b = asPointer(o.find(ids::connector::ObjectB));
                b && !b->isNull()) {
                p.connectorB = b->nodeId;
            }
            p.object = std::move(c);
            break;
        }
        default:
            warn(u"%1 : objet de type %2 non lu (nœud %3)"_s.arg(where).arg(type).arg(node.nodeId));
            return std::nullopt;
        }

        core::ObjectCommon& c = p.object.common;
        c.name = text(&o, ids::object::Name);
        p.layer = intOr(&o, ids::object::Layer, 1);
        c.layer = p.layer == 0 ? core::Layer::Bottom : p.layer == 2 ? core::Layer::Top : core::Layer::Middle;
        if (p.layer < 0 || p.layer > 2) {
            warn(u"%1 : couche %2 inconnue (nœud %3), couche intermédiaire"_s.arg(where).arg(p.layer).arg(
                node.nodeId));
        }
        p.depth = numberOr(&o, ids::object::Depth, 0);
        if (!std::isfinite(p.depth)) {
            p.depth = 0; // NaN romprait l'ordre strict du tri
        }
        c.visible = flagOr(&o, ids::object::Visible, true);
        c.locked = flagOr(&o, ids::object::Locked, false);
        c.restrictions.canMove = flagOr(&o, ids::object::CanMove, true);
        c.restrictions.canSize = flagOr(&o, ids::object::CanSize, true);
        c.restrictions.canRotate = flagOr(&o, ids::object::CanRotate, true);
        c.transform = localTransform(o, bounds);
        if (!isFinite(c.transform)) {
            warn(u"%1 : transformation non finie (nœud %2), identité"_s.arg(where).arg(node.nodeId));
            c.transform = {};
        }
        if (type != ids::type::Connector) {
            if (const StreamObject* label = asObject(o.find(ids::object::Label))) {
                c.label = text(label, ids::label::Caption);
            }
        }

        if (nesting >= kMaxNesting && !node.children.empty()) {
            warn(u"%1 : objets imbriqués au-delà de %2 niveaux sautés (nœud %3)"_s.arg(where)
                     .arg(kMaxNesting)
                     .arg(node.nodeId));
            return p;
        }
        int order = 0;
        for (const IndexNode& child : node.children) {
            QString error;
            const std::optional<StreamObject> co = load(container, child.storageName(), error);
            if (!co) {
                warn(u"%1 : %2 illisible : %3"_s.arg(where, child.storageName(), error));
                continue;
            }
            if (ids::isAction(child.type)) {
                p.actions.push_back(actionFrom(child, *co));
                continue;
            }
            if (child.type == ids::type::Label) {
                continue;
            }
            if (std::optional<Pending> cp = toPending(container, child, *co, where, nesting + 1)) {
                cp->order = order++;
                p.children.push_back(std::move(*cp));
            }
        }
        return p;
    }

    const QByteArray& m_data;
    std::optional<BambooContainer> m_root;
    std::map<quint32, ResourceItem> m_register;
    std::map<quint32, std::shared_ptr<const StreamObject>> m_resourceObjects;
    std::map<quint32, std::pair<std::string, std::string>> m_fileKeys; // ressource → (clé, nom de fichier)
    core::Document m_resources; // ne sert qu'à ranger les ressources par empreinte
    QStringList m_warnings;
    core::ObjectId m_nextId = 1;
};

} // namespace

FlipchartReadResult readFlipchart(const QByteArray& data)
{
    return Reader(data).read();
}

bool looksLikeFlipchart(const QByteArray& head) noexcept
{
    return head.startsWith(" Bamboo ");
}

} // namespace actcraft::formats::flipchart
