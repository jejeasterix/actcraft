#include <actcraft/core/alignment.h>
#include <actcraft/core/arrange.h>
#include <actcraft/core/clipboardedits.h>
#include <actcraft/core/connectors.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/objectflags.h>
#include <actcraft/formats/clipboard.h>
#include <actcraft/ui/insertion.h>
#include <actcraft/ui/objectediting.h>
#include <actcraft/ui/pagescontroller.h>

#include <QClipboard>
#include <QGuiApplication>
#include <QImage>
#include <QMimeData>
#include <QRegularExpression>
#include <QUrl>

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace actcraft::ui {

namespace {

using core::ObjectId;

// Collage : ajoute les ressources citées (une fois, au premier redo comme à chaque suivant : même clé, même
// copie) puis colle les objets ; une seule étape.
class PasteWithResources final : public core::UndoCommand
{
public:
    PasteWithResources(std::size_t page, std::vector<core::PageObject> prepared,
                       formats::ClipboardPayload payload)
        : m_inner(page, std::move(prepared)), m_payload(std::move(payload))
    {}
    void redo(core::Document& document, core::Changes& changes) override
    {
        for (const auto& [key, bytes] : m_payload.resources) {
            const auto ext = m_payload.extensions.find(key);
            document.addResource(QByteArray(bytes.data(), static_cast<qsizetype>(bytes.size())),
                                 ext != m_payload.extensions.end() ? ext->second : std::string("bin"));
        }
        m_inner.redo(document, changes);
    }
    void undo(core::Document& document, core::Changes& changes) override { m_inner.undo(document, changes); }

private:
    core::PasteObjectsCommand m_inner;
    formats::ClipboardPayload m_payload;
};

std::size_t shownIndex(const DocumentController& c)
{
    return static_cast<std::size_t>(c.currentIndex());
}

const core::Page& shownPage(const DocumentController& c)
{
    return c.document().page(shownIndex(c));
}

std::set<ObjectId> idsOf(const core::Page& page)
{
    std::set<ObjectId> out;
    for (const core::PageObject& object : page.objects)
        out.insert(object.common.id);
    return out;
}

// Sélection de l'utilisateur, limitée aux objets présents sur la page affichée.
std::vector<ObjectId> presentSelection(DocumentController* document, ToolHost* tools)
{
    std::vector<ObjectId> out;
    if (document == nullptr || tools == nullptr)
        return out;
    const std::set<ObjectId> present = idsOf(shownPage(*document));
    for (const ObjectId id : tools->usableSelection())
        if (present.count(id) != 0)
            out.push_back(id);
    return out;
}

const core::PageObject* find(const core::Page& page, ObjectId id)
{
    for (const core::PageObject& object : page.objects)
        if (object.common.id == id)
            return &object;
    return nullptr;
}

void selectNewObjects(DocumentController& document, ToolHost* tools, const std::set<ObjectId>& before)
{
    // Objets de premier niveau seulement : les membres d'un groupe inséré ne sont pas sélectionnés (D2).
    std::vector<ObjectId> fresh;
    std::set<ObjectId> members;
    for (const core::PageObject& object : shownPage(document).objects)
        if (before.count(object.common.id) == 0)
            if (const auto* group = std::get_if<core::Group>(&core::body(object)))
                members.insert(group->members.begin(), group->members.end());
    for (const core::PageObject& object : shownPage(document).objects)
        if (before.count(object.common.id) == 0 && members.count(object.common.id) == 0)
            fresh.push_back(object.common.id);
    if (tools != nullptr && !fresh.empty())
        tools->selectObjects(fresh);
}

void run(DocumentController& document, std::unique_ptr<core::UndoCommand> command)
{
    if (command)
        document.execute(std::move(command)); // le suivi des connecteurs est fait par execute (L5-C18)
}

bool isGroup(const core::PageObject& object)
{
    return std::holds_alternative<core::Group>(core::body(object));
}

} // namespace

ObjectEditing::ObjectEditing(QObject* parent) : QObject(parent)
{
    m_insertion = new Insertion(this);
    m_pages = new PagesController(this);
    connect(m_insertion, &Insertion::failed, this, &ObjectEditing::insertionFailed);
    connect(m_insertion, &Insertion::webFramesChanged, this, &ObjectEditing::webFramesChanged);
    connect(m_insertion, &Insertion::inserted, this, [this](int count) {
        if (m_document == nullptr || m_tools == nullptr || count <= 0)
            return;
        // Les objets insérés portent les plus grands identifiants de la page (compteur du document).
        const std::set<ObjectId> present = idsOf(shownPage(*m_document));
        std::vector<ObjectId> all(present.begin(), present.end());
        std::sort(all.begin(), all.end());
        const std::size_t n = std::min(all.size(), static_cast<std::size_t>(count));
        m_tools->selectObjects(std::vector<ObjectId>(all.end() - static_cast<std::ptrdiff_t>(n), all.end()));
    });
    connect(m_pages, &PagesController::clipboardChanged, this, &ObjectEditing::clipboardChanged);
    if (QClipboard* clipboard = QGuiApplication::clipboard()) {
        connect(clipboard, &QClipboard::dataChanged, this, [this]() {
            if (!m_writing)
                m_pageClipboard = false;
            emit clipboardChanged();
        });
    }
}

DocumentController* ObjectEditing::document() const
{
    return m_document;
}

void ObjectEditing::setDocument(DocumentController* document)
{
    if (m_document == document)
        return;
    if (m_document != nullptr)
        disconnect(m_document, nullptr, this, nullptr);
    m_document = document;
    m_insertion->setDocument(document);
    m_pages->setDocumentController(document);
    if (document != nullptr) {
        connect(document, &DocumentController::documentChanged, this, &ObjectEditing::selectionStateChanged);
        connect(document, &DocumentController::pagesChanged, this, &ObjectEditing::selectionStateChanged);
        // Un drapeau posé ailleurs (arbre, propriétés) ne rend aucun Change : documentChanged reste muet.
        connect(document, &DocumentController::objectFlagsChanged, this,
                &ObjectEditing::selectionStateChanged);
    }
    emit documentChanged();
    emit selectionStateChanged();
}

ToolHost* ObjectEditing::tools() const
{
    return m_tools;
}

void ObjectEditing::setTools(ToolHost* tools)
{
    if (m_tools == tools)
        return;
    if (m_tools != nullptr)
        disconnect(m_tools, nullptr, this, nullptr);
    m_tools = tools;
    if (tools != nullptr)
        connect(tools, &ToolHost::selectionChanged, this, &ObjectEditing::selectionStateChanged);
    emit toolsChanged();
    emit selectionStateChanged();
}

bool ObjectEditing::hasSelection() const
{
    return !presentSelection(m_document, m_tools).empty();
}

bool ObjectEditing::canPaste() const
{
    if (m_pageClipboard && m_pages->canPaste())
        return true;
    const QMimeData* mime = QGuiApplication::clipboard()->mimeData();
    return mime != nullptr &&
           (mime->hasFormat(formats::kObjectsMimeType) || mime->hasImage() || mime->hasText());
}

namespace {
bool flagState(DocumentController* document, ToolHost* tools, core::ObjectFlag flag)
{
    const std::vector<ObjectId> ids = presentSelection(document, tools);
    return !ids.empty() && core::flagOfAll(shownPage(*document), ids, flag);
}
} // namespace

bool ObjectEditing::selectionLocked() const
{
    return flagState(m_document, m_tools, core::ObjectFlag::Locked);
}

bool ObjectEditing::selectionHidden() const
{
    return flagState(m_document, m_tools, core::ObjectFlag::Hidden);
}

bool ObjectEditing::selectionDragCopy() const
{
    return flagState(m_document, m_tools, core::ObjectFlag::DragCopy);
}

bool ObjectEditing::selectionIsGroup() const
{
    const std::vector<ObjectId> ids = presentSelection(m_document, m_tools);
    if (ids.empty())
        return false;
    const core::Page& page = shownPage(*m_document);
    return std::all_of(ids.begin(), ids.end(), [&page](ObjectId id) {
        const core::PageObject* object = find(page, id);
        return object != nullptr && isGroup(*object);
    });
}

bool ObjectEditing::selectionIsAnnotation() const
{
    const std::vector<ObjectId> ids = presentSelection(m_document, m_tools);
    if (ids.empty())
        return false;
    const core::Page& page = shownPage(*m_document);
    return std::all_of(ids.begin(), ids.end(), [&page](ObjectId id) {
        const core::PageObject* object = find(page, id);
        return object != nullptr && std::holds_alternative<core::Stroke>(core::body(*object));
    });
}

void ObjectEditing::copy()
{
    if (m_document == nullptr)
        return;
    const std::vector<ObjectId> ids = presentSelection(m_document, m_tools);
    if (ids.empty()) { // D3 : la page
        m_pages->focusPage(m_document->currentIndex());
        m_pages->copySelection();
        m_pageClipboard = true;
        return;
    }
    const core::Page& page = shownPage(*m_document);
    formats::ClipboardPayload payload;
    payload.objects = core::closureForCopy(page, ids);
    if (payload.objects.empty())
        return;
    payload.resources = formats::resourcesFor(m_document->document(), payload.objects);
    for (const auto& entry : payload.resources)
        if (const core::Resource* resource = m_document->document().resource(entry.first))
            payload.extensions[entry.first] = resource->extension;
    auto mime = std::make_unique<QMimeData>();
    mime->setData(formats::kObjectsMimeType, formats::encodeClipboard(payload));
    if (payload.objects.size() == 1) {
        if (const auto* image = std::get_if<core::ImageObject>(&core::body(payload.objects.front()))) {
            if (const core::Resource* resource = m_document->document().resource(image->resource)) {
                const QImage decoded = QImage::fromData(resource->data);
                if (!decoded.isNull())
                    mime->setImageData(decoded);
            }
        }
    }
    m_writing = true;
    QGuiApplication::clipboard()->setMimeData(mime.release());
    m_writing = false;
    m_pageClipboard = false;
    emit clipboardChanged();
}

void ObjectEditing::cut()
{
    if (m_document == nullptr)
        return;
    const std::vector<ObjectId> ids = presentSelection(m_document, m_tools);
    if (ids.empty()) {
        m_pages->focusPage(m_document->currentIndex());
        m_pages->cutSelection();
        m_pageClipboard = true;
        return;
    }
    copy();
    run(*m_document, core::makeCutCommand(shownPage(*m_document), shownIndex(*m_document), ids));
    m_tools->clearSelection();
}

namespace {
// Pose la charge `payload` sur la page ; `dx`, `dy` ou `target` selon le mode. Vrai si quelque chose a été
// collé.
bool pastePayload(DocumentController& document, ToolHost* tools, formats::ClipboardPayload payload, double dx,
                  double dy, const core::PageObject* target)
{
    if (payload.objects.empty())
        return false;
    const std::set<ObjectId> before = idsOf(shownPage(document));
    std::vector<core::PageObject> prepared =
        target != nullptr ? core::preparePasteOnto(payload.objects, shownPage(document), *target)
                          : core::preparePaste(payload.objects, dx, dy);
    document.execute(
        std::make_unique<PasteWithResources>(shownIndex(document), std::move(prepared), std::move(payload)));
    selectNewObjects(document, tools, before);
    return true;
}

std::optional<formats::ClipboardPayload> clipboardObjects()
{
    const QMimeData* mime = QGuiApplication::clipboard()->mimeData();
    if (mime == nullptr || !mime->hasFormat(formats::kObjectsMimeType))
        return std::nullopt;
    return formats::decodeClipboard(mime->data(formats::kObjectsMimeType));
}

// Une URL n'est reconnue que si le texte entier, sur une ligne, en est une (n° 22).
bool singleWebAddress(const QString& text, QUrl& url)
{
    const QString trimmed = text.trimmed();
    static const QRegularExpression space(QStringLiteral("\\s"));
    if (trimmed.isEmpty() || trimmed.contains(space))
        return false;
    url = QUrl(trimmed, QUrl::StrictMode);
    return url.isValid() && !url.host().isEmpty() &&
           (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https"));
}
} // namespace

WebFrameItems* ObjectEditing::webFrames() const
{
    return m_insertion->webFrames();
}

void ObjectEditing::setWebFrames(WebFrameItems* frames)
{
    m_insertion->setWebFrames(frames);
}

void ObjectEditing::paste()
{
    if (m_document == nullptr)
        return;
    if (m_pageClipboard && m_pages->canPaste()) {
        m_pages->paste();
        return;
    }
    if (auto payload = clipboardObjects();
        payload && pastePayload(*m_document, m_tools, std::move(*payload), core::kPasteOffset,
                                core::kPasteOffset, nullptr))
        return;
    const QMimeData* mime = QGuiApplication::clipboard()->mimeData();
    if (mime == nullptr)
        return;
    if (mime->hasImage()) {
        const QImage image = qvariant_cast<QImage>(mime->imageData());
        if (!image.isNull()) {
            m_insertion->insertImageData(image);
            return;
        }
    }
    if (mime->hasText()) { // texte brut seulement : un HTML venu d'ailleurs n'est jamais interprété
        const QSizeF size = m_document->pageSize();
        const QPointF center(size.width() / 2, size.height() / 2);
        QUrl address;
        if (singleWebAddress(mime->text(), address))
            m_insertion->acceptWebDrop({address}, mime->text(), QString(), center);
        else
            m_insertion->acceptDrop({}, mime->text(), center);
    }
}

void ObjectEditing::pasteAt(QPointF pagePoint)
{
    if (m_document == nullptr)
        return;
    auto payload = clipboardObjects();
    if (!payload) {
        const QMimeData* mime = QGuiApplication::clipboard()->mimeData();
        if (mime != nullptr && !mime->hasImage() && mime->hasText()) {
            QUrl address;
            if (singleWebAddress(mime->text(), address))
                m_insertion->acceptWebDrop({address}, mime->text(), QString(), pagePoint);
            else
                m_insertion->acceptDrop({}, mime->text(), pagePoint);
        } else {
            paste();
        }
        return;
    }
    std::optional<core::RectF> united;
    for (const core::PageObject& object : payload->objects) {
        if (isGroup(object) || !core::hasExtent(object))
            continue;
        const core::RectF b = core::boundsOnPage(object);
        united = united ? core::enclosingRect(*united, b) : b;
    }
    const core::PointF center = united ? united->center() : core::PointF{0, 0};
    pastePayload(*m_document, m_tools, std::move(*payload), pagePoint.x() - center.x,
                 pagePoint.y() - center.y, nullptr);
}

void ObjectEditing::pasteOnto()
{
    if (m_document == nullptr)
        return;
    const std::vector<ObjectId> ids = presentSelection(m_document, m_tools);
    auto payload = clipboardObjects();
    if (ids.empty() || !payload) {
        paste();
        return;
    }
    const core::PageObject target = *find(shownPage(*m_document), ids.front());
    pastePayload(*m_document, m_tools, std::move(*payload), 0, 0, &target);
}

void ObjectEditing::duplicate()
{
    if (m_document == nullptr)
        return;
    const std::vector<ObjectId> ids = presentSelection(m_document, m_tools);
    if (ids.empty()) {
        m_pages->focusPage(m_document->currentIndex());
        m_pages->duplicateSelection();
        return;
    }
    const std::set<ObjectId> before = idsOf(shownPage(*m_document));
    run(*m_document, core::makeDuplicateCommand(shownPage(*m_document), shownIndex(*m_document), ids));
    selectNewObjects(*m_document, m_tools, before);
}

void ObjectEditing::toggleGroup()
{
    if (m_document == nullptr)
        return;
    const std::vector<ObjectId> ids = presentSelection(m_document, m_tools);
    if (ids.empty())
        return;
    const core::Page& page = shownPage(*m_document);
    if (selectionIsGroup()) {
        std::vector<ObjectId> members;
        for (const ObjectId id : ids)
            if (const auto* group = std::get_if<core::Group>(&core::body(*find(page, id))))
                members.insert(members.end(), group->members.begin(), group->members.end());
        run(*m_document, core::makeUngroupCommand(page, shownIndex(*m_document), ids));
        m_tools->selectObjects(members);
        return;
    }
    const std::set<ObjectId> before = idsOf(page);
    run(*m_document, core::makeGroupCommand(page, shownIndex(*m_document), ids));
    selectNewObjects(*m_document, m_tools, before);
}

void ObjectEditing::arrange(const QString& how)
{
    if (m_document == nullptr)
        return;
    const std::vector<ObjectId> ids = presentSelection(m_document, m_tools);
    if (ids.empty())
        return;
    const core::Page& page = shownPage(*m_document);
    const std::size_t index = shownIndex(*m_document);
    static const std::map<QString, core::ArrangeMove> moves = {
        {QStringLiteral("front"), core::ArrangeMove::Front},
        {QStringLiteral("back"), core::ArrangeMove::Back},
        {QStringLiteral("forward"), core::ArrangeMove::Forward},
        {QStringLiteral("backward"), core::ArrangeMove::Backward}};
    static const std::map<QString, core::Layer> layers = {
        {QStringLiteral("layer-top"), core::Layer::Top},
        {QStringLiteral("layer-middle"), core::Layer::Middle},
        {QStringLiteral("layer-bottom"), core::Layer::Bottom}};
    if (const auto move = moves.find(how); move != moves.end()) {
        std::vector<ObjectId> order = core::arrangedOrder(page, ids, move->second);
        std::vector<ObjectId> current;
        for (const core::PageObject& object : page.objects)
            current.push_back(object.common.id);
        if (order != current)
            run(*m_document, std::make_unique<core::ReorderObjectsCommand>(index, std::move(order)));
    } else if (const auto layer = layers.find(how); layer != layers.end()) {
        run(*m_document,
            std::make_unique<core::SetLayerCommand>(index, core::expandGroups(page, ids), layer->second));
    }
}

void ObjectEditing::align(const QString& how)
{
    if (m_document == nullptr)
        return;
    static const std::map<QString, core::AlignMode> modes = {
        {QStringLiteral("left"), core::AlignMode::Left},
        {QStringLiteral("center-x"), core::AlignMode::CenterX},
        {QStringLiteral("right"), core::AlignMode::Right},
        {QStringLiteral("top"), core::AlignMode::Top},
        {QStringLiteral("center-y"), core::AlignMode::CenterY},
        {QStringLiteral("bottom"), core::AlignMode::Bottom},
        {QStringLiteral("width"), core::AlignMode::Width},
        {QStringLiteral("height"), core::AlignMode::Height},
        {QStringLiteral("angle"), core::AlignMode::Angle},
        {QStringLiteral("all"), core::AlignMode::All}};
    const auto mode = modes.find(how);
    const std::vector<ObjectId> selected = presentSelection(m_document, m_tools);
    if (mode == modes.end() || selected.empty())
        return;
    const core::Page& page = shownPage(*m_document);
    core::TransformList list = core::alignTransforms(page, selected, mode->second);
    if (!list.empty())
        run(*m_document,
            std::make_unique<core::TransformEachCommand>(shownIndex(*m_document), std::move(list)));
}

void ObjectEditing::transform(const QString& how)
{
    if (m_document == nullptr)
        return;
    const std::vector<ObjectId> selected = presentSelection(m_document, m_tools);
    if (selected.empty())
        return;
    const core::Page& page = shownPage(*m_document);
    const std::size_t index = shownIndex(*m_document);
    const std::vector<ObjectId>& members = selected; // un groupe est transformé comme un bloc (D6)
    core::TransformList list;
    if (how == QLatin1String("original-size"))
        list = core::fitTransforms(page, members, core::FitMode::OriginalSize);
    else if (how == QLatin1String("fit-width"))
        list = core::fitTransforms(page, members, core::FitMode::Width);
    else if (how == QLatin1String("fit-height"))
        list = core::fitTransforms(page, members, core::FitMode::Height);
    else if (how == QLatin1String("fit-page"))
        list = core::fitTransforms(page, members, core::FitMode::Page);
    else if (how == QLatin1String("flip-x") || how == QLatin1String("flip-y"))
        list = core::flipTransforms(page, members,
                                    how.endsWith(QLatin1Char('x')) ? core::Axis::X : core::Axis::Y);
    else if (how == QLatin1String("origin-flip-x") || how == QLatin1String("origin-flip-y")) {
        if (!page.presentation.hasOrigin)
            return; // commande inerte sans origine mémorisée
        list = core::originFlipTransforms(page, members,
                                          how.endsWith(QLatin1Char('x')) ? core::Axis::X : core::Axis::Y,
                                          page.presentation.origin);
    } else if (how == QLatin1String("mirror-x") || how == QLatin1String("mirror-y")) {
        const std::set<ObjectId> before = idsOf(page);
        std::vector<core::PageObject> copies = core::mirrorCopies(
            page, selected, how.endsWith(QLatin1Char('x')) ? core::Axis::X : core::Axis::Y);
        if (copies.empty())
            return;
        run(*m_document,
            std::make_unique<core::PasteObjectsCommand>(index, core::preparePaste(std::move(copies), 0, 0)));
        selectNewObjects(*m_document, m_tools, before);
        return;
    }
    if (!list.empty())
        run(*m_document, std::make_unique<core::TransformEachCommand>(index, std::move(list)));
}

void ObjectEditing::toggleFlag(const QString& flag)
{
    if (m_document == nullptr)
        return;
    const std::vector<ObjectId> ids = presentSelection(m_document, m_tools);
    if (ids.empty())
        return;
    core::ObjectFlag which;
    if (flag == QLatin1String("lock"))
        which = core::ObjectFlag::Locked;
    else if (flag == QLatin1String("hide"))
        which = core::ObjectFlag::Hidden;
    else if (flag == QLatin1String("drag-copy"))
        which = core::ObjectFlag::DragCopy;
    else
        return;
    // Règle (D9) : verrou et masquage posés sur un groupe sont propagés à ses membres, que le dessin,
    // l'export, la sélection et le déplacement lisent chacun sur leur objet.
    const std::vector<ObjectId> targets =
        which == core::ObjectFlag::DragCopy ? ids : core::expandGroups(shownPage(*m_document), ids);
    const bool value = !core::flagOfAll(shownPage(*m_document), targets, which);
    run(*m_document,
        std::make_unique<core::SetObjectFlagCommand>(shownIndex(*m_document), targets, which, value));
}

} // namespace actcraft::ui
