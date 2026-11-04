#include <actcraft/core/clipboardedits.h>
#include <actcraft/core/edits.h>
#include <actcraft/formats/clipboard.h>
#include <actcraft/ui/shortcutbar.h>

#include <QByteArray>
#include <QJsonObject>
#include <QUuid>
#include <QVariantMap>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace actcraft::ui {

namespace {

constexpr auto kObjectsKey = "objects";
constexpr auto kLabelKey = "label";

// Collage : ajoute les ressources citées (une fois, au premier redo comme à chaque suivant : même clé, même
// copie) puis colle les objets ; une seule étape. Même patron que le collage du lot 5 (objectediting.cpp).
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

bool isToolLike(const QString& id)
{
    static const QStringList prefixes = {QStringLiteral("tool."),   QStringLiteral("present."),
                                         QStringLiteral("math."),   QStringLiteral("desktop."),
                                         QStringLiteral("record."), QStringLiteral("view.page-zoom")};
    return std::any_of(prefixes.begin(), prefixes.end(),
                       [&id](const QString& prefix) { return id.startsWith(prefix); });
}

bool inPalette(const Commands& commands, const QString& id)
{
    const QVariantList groups = commands.palette();
    for (const QVariant& group : groups) {
        const QVariantList items = group.toMap().value(QStringLiteral("items")).toList();
        for (const QVariant& item : items)
            if (item.toMap().value(QStringLiteral("id")).toString() == id)
                return true;
    }
    return false;
}

} // namespace

ShortcutBar::ShortcutBar(QObject* parent) : QObject(parent) {}

ProfileController* ShortcutBar::profile() const noexcept
{
    return m_profile;
}

void ShortcutBar::setProfile(ProfileController* profile)
{
    if (m_profile == profile)
        return;
    if (m_profile != nullptr)
        disconnect(m_profile, nullptr, this, nullptr);
    m_profile = profile;
    if (profile != nullptr)
        connect(profile, &ProfileController::profileChanged, this, &ShortcutBar::changed);
    emit changed();
}

Commands* ShortcutBar::commands() const noexcept
{
    return m_commands;
}

void ShortcutBar::setCommands(Commands* commands)
{
    if (m_commands == commands)
        return;
    if (m_commands != nullptr)
        disconnect(m_commands, nullptr, this, nullptr);
    m_commands = commands;
    if (commands != nullptr) {
        commands->setNativeHandler(QStringLiteral("shortcutbar.add-selection"),
                                   [this](const QVariant&) { addSelection(); });
        connect(commands, &Commands::triggered, this, [this](const QString& id) { onTriggered(id); });
        connect(commands, &Commands::layoutChanged, this, &ShortcutBar::changed);
    }
    emit changed();
}

bool ShortcutBar::open() const
{
    return m_commands != nullptr && m_profile != nullptr && m_document != nullptr && !m_commands->isLocked();
}

QVariantList ShortcutBar::buttons() const
{
    QVariantList out;
    if (m_profile == nullptr)
        return out;
    for (const QString& id : m_profile->recentTools()) {
        out.append(QVariantMap{
            {QStringLiteral("kind"), QStringLiteral("tool")},
            {QStringLiteral("id"), id},
            {QStringLiteral("label"), m_commands != nullptr ? m_commands->label(id) : QString()},
            {QStringLiteral("icon"), m_commands != nullptr ? m_commands->iconUrl(id) : QString()},
        });
    }
    const QJsonObject& objects = m_profile->profile().shortcutObjects;
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        out.append(QVariantMap{
            {QStringLiteral("kind"), QStringLiteral("object")},
            {QStringLiteral("id"), it.key()},
            {QStringLiteral("label"), it.value().toObject().value(QLatin1String(kLabelKey)).toString()},
            {QStringLiteral("icon"), QString()},
        });
    }
    return out;
}

void ShortcutBar::onTriggered(const QString& id)
{
    // Un outil de la palette n'entre pas (palette, menu ou raccourci) : le filtre sur la palette suffit.
    if (m_profile == nullptr || m_commands == nullptr || !isToolLike(id) || inPalette(*m_commands, id))
        return;
    m_profile->pushRecentTool(id);
}

QString ShortcutBar::addObjects(const QList<quint64>& objectIds)
{
    if (!open() || objectIds.isEmpty())
        return {};
    const core::Page& page =
        m_document->document().page(static_cast<std::size_t>(m_document->currentIndex()));
    formats::ClipboardPayload payload;
    payload.objects =
        core::closureForCopy(page, std::vector<core::ObjectId>(objectIds.begin(), objectIds.end()));
    if (payload.objects.empty())
        return {};
    payload.resources = formats::resourcesFor(m_document->document(), payload.objects);
    for (const auto& entry : payload.resources)
        if (const core::Resource* resource = m_document->document().resource(entry.first))
            payload.extensions[entry.first] = resource->extension;
    const QString id = QStringLiteral("object-") + QUuid::createUuid().toString(QUuid::Id128).left(12);
    QJsonObject entry;
    entry.insert(QLatin1String(kLabelKey),
                 tr("Objet %1").arg(m_profile->profile().shortcutObjects.size() + 1));
    entry.insert(QLatin1String(kObjectsKey),
                 QString::fromLatin1(formats::encodeClipboard(payload).toBase64()));
    m_profile->editProfile().shortcutObjects.insert(id, entry);
    m_profile->touch();
    return id;
}

QString ShortcutBar::addSelection()
{
    if (!open() || m_tools == nullptr)
        return {};
    const core::Page& page =
        m_document->document().page(static_cast<std::size_t>(m_document->currentIndex()));
    QList<quint64> ids;
    const std::vector<core::ObjectId>& selected = m_tools->user().selection();
    for (const core::PageObject& object : page.objects)
        if (std::find(selected.begin(), selected.end(), object.common.id) != selected.end())
            ids.append(object.common.id);
    return addObjects(ids);
}

bool ShortcutBar::insert(const QString& buttonId)
{
    if (!open())
        return false;
    const QJsonObject& objects = m_profile->profile().shortcutObjects;
    if (!objects.contains(buttonId))
        return false;
    const QByteArray encoded = QByteArray::fromBase64(
        objects.value(buttonId).toObject().value(QLatin1String(kObjectsKey)).toString().toLatin1());
    std::optional<formats::ClipboardPayload> payload = formats::decodeClipboard(encoded);
    if (!payload || payload->objects.empty())
        return false;
    // Centre de l'ensemble : union des emprises des objets qui en ont une (un groupe n'en a pas : ses
    // membres).
    bool any = false;
    double left = 0, top = 0, right = 0, bottom = 0;
    for (const core::PageObject& object : payload->objects) {
        if (!core::hasExtent(object))
            continue;
        const core::RectF box = core::boundsOnPage(object);
        if (!any) {
            left = box.x;
            top = box.y;
            right = box.x + box.width;
            bottom = box.y + box.height;
            any = true;
        } else {
            left = std::min(left, box.x);
            top = std::min(top, box.y);
            right = std::max(right, box.x + box.width);
            bottom = std::max(bottom, box.y + box.height);
        }
    }
    const std::size_t index = static_cast<std::size_t>(m_document->currentIndex());
    const core::Page& page = m_document->document().page(index);
    const double dx = any ? page.width / 2 - (left + right) / 2 : 0;
    const double dy = any ? page.height / 2 - (top + bottom) / 2 : 0;
    std::vector<core::PageObject> prepared = core::preparePaste(payload->objects, dx, dy);
    m_document->execute(
        std::make_unique<PasteWithResources>(index, std::move(prepared), std::move(*payload)));
    return true;
}

void ShortcutBar::removeButton(const QString& buttonId)
{
    if (m_profile == nullptr || !m_profile->profile().shortcutObjects.contains(buttonId))
        return;
    m_profile->editProfile().shortcutObjects.remove(buttonId);
    m_profile->touch();
}

} // namespace actcraft::ui
