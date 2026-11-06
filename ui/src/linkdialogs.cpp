#include "actcraft/ui/linkdialogs.h"

#include <actcraft/core/actionedits.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/replace.h>
#include <actcraft/core/undo.h>

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextDocument>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <variant>
#include <vector>

namespace actcraft::ui {

// Fonctions libres de ui/src/textediting.cpp : le document isolé du peintre et de la mesure (comme
// tst_ui_text), pour que l'emprise du texte créé soit celle que le peintre dessine.
QString safeTextHtml(const QString& html, const std::function<void(const QUrl&, const QVariant&)>& observer);
std::unique_ptr<QTextDocument>
isolatedTextDocument(std::function<void(const QUrl&, const QVariant&)> observer);

namespace {

constexpr qint64 kMaxStoredBytes =
    100LL * 1024 * 1024; // un fichier plus gros ne se range pas dans un document

std::mutex g_probeMutex;
std::function<void()> g_readProbe;
std::atomic<int> g_insertions{0};

void runReadProbe()
{
    std::function<void()> probe;
    {
        const std::lock_guard<std::mutex> lock(g_probeMutex);
        probe = g_readProbe;
    }
    if (probe)
        probe();
}

// Une étape : la ressource du fichier rangé (clé SHA-256 tirée au redo, la même à chaque rejeu) puis la
// commande qui pose le lien, construite avec cette clé au premier redo.
class LinkCommand final : public core::UndoCommand
{
public:
    using Build = std::function<std::unique_ptr<core::UndoCommand>(const std::string& resourceKey)>;
    LinkCommand(QByteArray data, std::string extension, Build build)
        : m_data(std::move(data)), m_extension(std::move(extension)), m_build(std::move(build))
    {}
    void redo(core::Document& document, core::Changes& changes) override
    {
        std::string key;
        if (!m_data.isEmpty()) {
            key = document.addResource(m_data, m_extension);
            ++g_insertions;
        }
        if (!m_inner)
            m_inner = m_build(key);
        m_inner->redo(document, changes);
    }
    void undo(core::Document& document, core::Changes& changes) override
    {
        if (m_inner)
            m_inner->undo(document, changes);
    }

private:
    QByteArray m_data;
    std::string m_extension;
    Build m_build;
    std::unique_ptr<core::UndoCommand> m_inner;
};

QString compactJson(const QJsonObject& object)
{
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

// Extension de ressource : [a-z0-9]{1,8}, sinon « bin » (document.h).
std::string resourceExtension(const QString& fileName)
{
    const QString suffix = QFileInfo(fileName).suffix().toLower();
    const bool valid =
        !suffix.isEmpty() && suffix.size() <= 8 && std::all_of(suffix.begin(), suffix.end(), [](QChar c) {
            return (c >= u'a' && c <= u'z') || (c >= u'0' && c <= u'9');
        });
    return valid ? suffix.toStdString() : std::string("bin");
}

core::RectF centered(double pageWidth, double pageHeight, double width, double height)
{
    return core::RectF{std::max(0.0, (pageWidth - width) / 2), std::max(0.0, (pageHeight - height) / 2),
                       width, height};
}

} // namespace

LinkDialogs::LinkDialogs(QObject* parent) : QObject(parent) {}

LinkDialogs::~LinkDialogs() = default;

void LinkDialogs::setReadProbe(std::function<void()> probe)
{
    const std::lock_guard<std::mutex> lock(g_probeMutex);
    g_readProbe = std::move(probe);
}

int LinkDialogs::resourceInsertions() noexcept
{
    return g_insertions.load();
}

void LinkDialogs::setCommands(Commands* commands)
{
    if (commands == m_commands.data())
        return;
    m_commands = commands;
    emit sourcesChanged();
}

void LinkDialogs::setDocument(DocumentController* document)
{
    if (document == m_document.data())
        return;
    m_document = document;
    emit sourcesChanged();
}

void LinkDialogs::setToolHost(ToolHost* tools)
{
    if (tools == m_tools.data())
        return;
    m_tools = tools;
    emit sourcesChanged();
}

void LinkDialogs::setLinks(LinkLauncher* links)
{
    if (links == m_links.data())
        return;
    m_links = links;
    emit sourcesChanged();
}

void LinkDialogs::setArguments(const QString& arguments)
{
    if (arguments == m_arguments)
        return;
    m_arguments = arguments;
    emit argumentsChanged();
}

bool LinkDialogs::refuse(const QString& reason)
{
    emit refused(reason);
    return false;
}

// Fermé par défaut : sans registre de commandes, une commande absente, désactivée ou verrouillée refuse.
bool LinkDialogs::locked(const char* command) const
{
    return m_commands.isNull() || m_commands->isLocked() || !m_commands->isEnabled(QLatin1String(command));
}

bool LinkDialogs::createWebsiteLink(const QString& address, int form, quint64 existing)
{
    if (m_links.isNull() || m_document.isNull())
        return refuse(tr("Les liens ne sont pas disponibles."));
    if (locked("link.website"))
        return refuse(tr("Cette fonction est verrouillée."));
    // Même validation que l'ouverture (LinkLauncher::request pour un site).
    const QUrl url(address.trimmed(), QUrl::StrictMode);
    if (const QString reason = LinkLauncher::refusalFor(url, LinkLauncher::Website); !reason.isEmpty())
        return refuse(reason);
    const QString shown = url.toString(QUrl::FullyEncoded);
    QJsonObject parameters;
    parameters.insert(QStringLiteral("url"), shown);
    return commit(shown, QStringLiteral("document.open-website"), compactJson(parameters), form, existing, {},
                  {});
}

bool LinkDialogs::createFileLink(const QUrl& file, int form, int storage, quint64 existing)
{
    if (m_links.isNull() || m_document.isNull())
        return refuse(tr("Les liens ne sont pas disponibles."));
    if (locked("link.file"))
        return refuse(tr("Cette fonction est verrouillée."));
    // Un chemin nu (sans schéma, lettre de lecteur comprise) se lit comme un fichier local (request).
    const QUrl url = file.scheme().size() > 1 ? file : QUrl::fromLocalFile(file.toString());
    const bool stored = storage == DocumentStorage;
    if (const QString reason =
            LinkLauncher::refusalFor(url, stored ? LinkLauncher::StoredFile : LinkLauncher::File);
        !reason.isEmpty())
        return refuse(reason);
    const QString path = url.toLocalFile();
    const QString name = QFileInfo(path).fileName();
    QJsonObject parameters;
    QByteArray bytes;
    if (stored) {
        runReadProbe();
        QFile source(path);
        if (source.size() > kMaxStoredBytes)
            return refuse(tr("Ce fichier est trop gros pour être rangé dans le paperboard : %1").arg(name));
        if (!source.open(QIODevice::ReadOnly))
            return refuse(tr("Fichier illisible : %1").arg(name));
        bytes = source.readAll();
        if (bytes.isEmpty())
            return refuse(tr("Fichier vide : %1").arg(name));
        parameters.insert(QStringLiteral("file"), name); // jamais le dossier de l'auteur
    } else {
        parameters.insert(QStringLiteral("file"), path);
    }
    if (!m_arguments.trimmed().isEmpty()) // gardés, jamais passés (aucun QProcess, H8-3)
        parameters.insert(QStringLiteral("arguments"), m_arguments);
    return commit(name, QStringLiteral("document.open-file"), compactJson(parameters), form, existing, bytes,
                  QString::fromStdString(resourceExtension(name)));
}

bool LinkDialogs::commit(const QString& label, const QString& catalogId, const QString& parameters, int form,
                         quint64 existing, const QByteArray& stored, const QString& extension)
{
    DocumentController* controller = m_document.data();
    if (controller->pageCount() <= 0 || controller->currentIndex() < 0)
        return refuse(tr("Aucune page ne reçoit le lien."));
    const std::size_t pageIndex = static_cast<std::size_t>(controller->currentIndex());
    const core::Page& page = controller->document().page(pageIndex);

    core::Action action;
    action.catalogId = catalogId.toStdString();
    action.parameters = parameters.toStdString();
    const bool needsResource = !stored.isEmpty();

    const core::ObjectId firstNew = controller->document().nextId();
    LinkCommand::Build build;
    if (form == ExistingForm) {
        core::ObjectId target = existing;
        if (target == 0 && m_tools) {
            const std::vector<core::ObjectId>& selection = m_tools->user().selection();
            if (selection.size() == 1)
                target = selection.front();
        }
        const auto found =
            std::find_if(page.objects.begin(), page.objects.end(),
                         [target](const core::PageObject& o) { return o.common.id == target; });
        if (target == 0 || found == page.objects.end())
            return refuse(tr("Choisissez d'abord un objet de la page."));
        std::vector<core::Action> actions = found->common.actions;
        build = [pageIndex, target, actions, action, needsResource](const std::string& key) mutable {
            core::Action added = action;
            if (needsResource)
                added.resource = key;
            actions.push_back(std::move(added));
            return std::make_unique<core::SetObjectActionsCommand>(
                pageIndex, std::vector<core::ObjectId>{target}, std::move(actions));
        };
    } else {
        const double pageWidth = controller->pageSize().width();
        const double pageHeight = controller->pageSize().height();
        core::PageObject object;
        switch (form) {
        case IconForm: { // image d'espace réservé : dégradé de l'application, 64 × 64
            core::ImageObject icon;
            icon.bounds = centered(pageWidth, pageHeight, 64, 64);
            icon.sourceWidth = 64;
            icon.sourceHeight = 64;
            icon.colorA = 0xFF1E88E5u;
            icon.colorB = 0xFFFFFFFFu;
            object = icon;
            break;
        }
        case ActionForm: { // objet d'action : rectangle légendé
            core::Shape shape;
            shape.kind = core::ShapeKind::Rectangle;
            shape.bounds = centered(pageWidth, pageHeight, 160, 60);
            shape.fill = 0xFF1E88E5u;
            object = shape;
            object.common.label = label.toStdString();
            break;
        }
        default: { // TextForm
            const QString html = label.toHtmlEscaped();
            std::unique_ptr<QTextDocument> measure = isolatedTextDocument({});
            measure->setHtml(safeTextHtml(html, {}));
            const double width = std::ceil(measure->idealWidth()) + 1;
            measure->setTextWidth(width);
            core::Text text;
            text.html = html.toStdString();
            text.bounds = centered(pageWidth, pageHeight, width, std::ceil(measure->size().height()));
            object = text;
            break;
        }
        }
        build = [pageIndex, object, action, needsResource](const std::string& key) mutable {
            core::Action added = action;
            if (needsResource)
                added.resource = key;
            object.common.actions.push_back(std::move(added));
            return std::make_unique<core::ReplaceObjectsCommand>(pageIndex, std::vector<core::ObjectId>{},
                                                                 std::vector<core::PageObject>{object});
        };
    }

    controller->execute(std::make_unique<LinkCommand>(stored, extension.toStdString(), std::move(build)));

    core::ObjectId made = existing;
    if (form != ExistingForm) {
        made = 0;
        for (const core::PageObject& o : controller->document().page(pageIndex).objects)
            if (o.common.id >= firstNew)
                made = std::max(made, o.common.id);
        if (made == 0)
            return refuse(tr("Le lien n'a pas pu être créé."));
        if (m_tools) // l'outil de sélection porte les actions : le clic suivant les exécute
            m_tools->selectObjects({made});
    }
    emit created(made);
    return true;
}

} // namespace actcraft::ui
