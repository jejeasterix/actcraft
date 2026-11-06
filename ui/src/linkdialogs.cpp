#include "actcraft/ui/linkdialogs.h"

#include <actcraft/core/actionedits.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/replace.h>
#include <actcraft/core/undo.h>

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextDocument>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
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

// Une étape : la ressource du fichier rangé (clé SHA-256 calculée une fois, hors du fil de l'interface, par
// readStoredFile) puis la commande qui pose le lien, construite avec cette clé au premier redo. Les octets ne
// sont rangés qu'une fois : un rejeu retrouve la ressource par sa clé et ne rehache rien.
class LinkCommand final : public core::UndoCommand
{
public:
    using Build = std::function<std::unique_ptr<core::UndoCommand>(const std::string& resourceKey)>;
    LinkCommand(QByteArray data, std::string key, std::string extension, Build build)
        : m_data(std::move(data)), m_key(std::move(key)), m_extension(std::move(extension)),
          m_build(std::move(build))
    {}
    void redo(core::Document& document, core::Changes& changes) override
    {
        if (!m_data.isEmpty() && (m_key.empty() || document.resource(m_key) == nullptr)) {
            m_key = document.addResource(m_data, m_extension);
            ++g_insertions;
        }
        if (!m_inner)
            m_inner = m_build(m_key);
        m_inner->redo(document, changes);
    }
    void undo(core::Document& document, core::Changes& changes) override
    {
        if (m_inner)
            m_inner->undo(document, changes);
    }

private:
    QByteArray m_data;
    std::string m_key;
    std::string m_extension;
    Build m_build;
    std::unique_ptr<core::UndoCommand> m_inner;
};

// Résultat du travail hors du fil de l'interface : les octets du fichier et leur clé SHA-256 (hexadécimal
// minuscule, la clé de Document::addResource), ou la raison de l'échec.
struct StoredRead
{
    enum class Failure
    {
        None,
        Unreadable,
        Empty,
        TooBig
    };
    Failure failure = Failure::None;
    QByteArray bytes;
    std::string key;
};

// Fil du pool global : n'écrit nulle part, ne touche ni le document ni un objet de l'interface.
StoredRead readStoredFile(const QString& path)
{
    runReadProbe();
    StoredRead result;
    QFile source(path);
    if (!source.open(QIODevice::ReadOnly)) {
        result.failure = StoredRead::Failure::Unreadable;
        return result;
    }
    result.bytes = source.read(kMaxStoredBytes + 1); // un fichier qui a grossi depuis le contrôle reste borné
    if (result.bytes.size() > kMaxStoredBytes) {
        result.failure = StoredRead::Failure::TooBig;
        result.bytes.clear();
    } else if (result.bytes.isEmpty()) {
        result.failure = StoredRead::Failure::Empty;
    } else {
        result.key = QCryptographicHash::hash(result.bytes, QCryptographicHash::Sha256).toHex().toStdString();
    }
    return result;
}

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

QString LinkDialogs::localPath(const QUrl& url) const
{
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}

bool LinkDialogs::createFileLink(const QString& path, int form, int storage, quint64 existing)
{
    if (m_links.isNull() || m_document.isNull())
        return refuse(tr("Les liens ne sont pas disponibles."));
    if (locked("link.file"))
        return refuse(tr("Cette fonction est verrouillée."));
    // Un chemin nu (sans schéma, lettre de lecteur comprise) est un fichier local, pris tel quel : jamais lu
    // comme une URL, où `#`, `?` et `%` ouvrent un fragment, une requête ou un échappement.
    static const QRegularExpression scheme(QStringLiteral("^[A-Za-z][A-Za-z0-9+.-]+:"));
    const QUrl url = scheme.match(path).hasMatch() ? QUrl(path) : QUrl::fromLocalFile(path);
    const bool stored = storage == DocumentStorage;
    if (const QString reason =
            LinkLauncher::refusalFor(url, stored ? LinkLauncher::StoredFile : LinkLauncher::File);
        !reason.isEmpty())
        return refuse(reason);
    const QString localFile = url.toLocalFile();
    const QString name = QFileInfo(localFile).fileName();
    QJsonObject parameters;
    parameters.insert(QStringLiteral("file"),
                      stored ? name : localFile); // jamais le dossier de l'auteur si rangé
    if (!m_arguments.trimmed().isEmpty())         // gardés, jamais passés (aucun QProcess, H8-3)
        parameters.insert(QStringLiteral("arguments"), m_arguments);
    const QString json = compactJson(parameters);
    if (!stored)
        return commit(name, QStringLiteral("document.open-file"), json, form, existing, {}, {});
    if (m_busy)
        return refuse(tr("Un fichier est déjà en cours de lecture."));
    if (QFileInfo(localFile).size() > kMaxStoredBytes)
        return refuse(tr("Ce fichier est trop gros pour être rangé dans le paperboard : %1").arg(name));
    storeFile(localFile, name, json, form, existing);
    return true; // accepté : created ou refused suivra, une fois la lecture finie
}

void LinkDialogs::setBusy(bool busy)
{
    if (busy == m_busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

// Lecture et SHA-256 sur le pool global (même patron que DocumentController::saveToUrl), résultat relayé sur
// le fil de l'interface par un QFutureWatcher fils de cet objet : détruit avec lui, il ne livre plus rien. Le
// résultat est jeté si le document n'est plus celui du clic (remplacé, détruit) ; l'objet visé est revérifié
// par commit, au retour.
void LinkDialogs::storeFile(const QString& path, const QString& name, const QString& parameters, int form,
                            quint64 existing)
{
    DocumentController* controller = m_document.data();
    const std::uint64_t generation = controller->generation();
    setBusy(true);
    auto* watcher = new QFutureWatcher<StoredRead>(this);
    connect(watcher, &QFutureWatcher<StoredRead>::finished, this,
            [this, watcher, document = m_document, generation, name, parameters, form, existing] {
                StoredRead read = watcher->result();
                watcher->deleteLater();
                setBusy(false);
                if (document.isNull() || document.data() != m_document.data() ||
                    document->generation() != generation)
                    return; // jeté : le document du clic n'est plus à l'écran
                switch (read.failure) {
                case StoredRead::Failure::Unreadable:
                    refuse(tr("Fichier illisible : %1").arg(name));
                    return;
                case StoredRead::Failure::Empty:
                    refuse(tr("Fichier vide : %1").arg(name));
                    return;
                case StoredRead::Failure::TooBig:
                    refuse(tr("Ce fichier est trop gros pour être rangé dans le paperboard : %1").arg(name));
                    return;
                case StoredRead::Failure::None:
                    break;
                }
                // L'état a pu changer pendant la lecture : verrou de classe, lanceur retiré.
                if (m_links.isNull() || locked("link.file")) {
                    refuse(m_links.isNull() ? tr("Les liens ne sont pas disponibles.")
                                            : tr("Cette fonction est verrouillée."));
                    return;
                }
                commit(name, QStringLiteral("document.open-file"), parameters, form, existing, read.bytes,
                       QString::fromStdString(resourceExtension(name)), std::move(read.key));
            });
    watcher->setFuture(
        QtConcurrent::run(QThreadPool::globalInstance(), [path] { return readStoredFile(path); }));
}

bool LinkDialogs::commit(const QString& label, const QString& catalogId, const QString& parameters, int form,
                         quint64 existing, const QByteArray& stored, const QString& extension,
                         std::string storedKey)
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

    controller->execute(std::make_unique<LinkCommand>(stored, std::move(storedKey), extension.toStdString(),
                                                      std::move(build)));

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
