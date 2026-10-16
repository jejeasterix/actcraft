#include <actcraft/core/edits.h>
#include <actcraft/core/generator.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/formats/urlio.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documentio.h>

#include <QLoggingCategory>
#include <QMetaObject>
#include <QThreadPool>

#include <algorithm>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>

namespace actcraft::ui {

// Fil secondaire → contrôleur. Le fil poste son résultat sous le verrou, et le destructeur du contrôleur
// efface la cible sous le même verrou : jamais un envoi vers un objet en cours de destruction (un QPointer lu
// depuis un autre fil ne le garantit pas). Un résultat posté avant la destruction est retiré par ~QObject.
struct IoRelay
{
    std::mutex mutex;
    DocumentController* target = nullptr;

    template <typename F> void post(F&& function)
    {
        const std::lock_guard<std::mutex> lock(mutex);
        if (target) {
            QMetaObject::invokeMethod(target, std::forward<F>(function), Qt::QueuedConnection);
        }
    }
};

namespace {

Q_STATIC_LOGGING_CATEGORY(lcDocument, "actcraft.document", QtInfoMsg)

QString untitled()
{
    return QStringLiteral("Sans titre"); // F-656
}

// Commande déjà exécutée une fois par le contrôleur, pour savoir si elle change quelque chose : le redo()
// qu'UndoStack::push fait aussitôt ne la rejoue pas ; les suivants (rétablir) la rejouent.
class Executed final : public core::UndoCommand
{
public:
    explicit Executed(std::unique_ptr<core::UndoCommand> command) : m_command(std::move(command)) {}

    void redo(core::Document& document, core::Changes& changes) override
    {
        if (m_pending) {
            m_pending = false;
            return;
        }
        m_command->redo(document, changes);
    }

    void undo(core::Document& document, core::Changes& changes) override
    {
        m_command->undo(document, changes);
    }

    int mergeId() const noexcept override { return m_command->mergeId(); }

    bool mergeWith(const core::UndoCommand& other) override
    {
        const auto* executed = dynamic_cast<const Executed*>(&other);
        return m_command->mergeWith(executed ? *executed->m_command : other);
    }

private:
    std::unique_ptr<core::UndoCommand> m_command;
    bool m_pending = true;
};

// Sans page fixe : seul l'indice recalé de la page affichée compte.
class NoCache final : public core::PageCache
{
public:
    void insert(core::ObjectId) override {}
    void remove(core::ObjectId, const core::RectF&) override {}
    void invalidate(core::ObjectId, const core::CacheArea&) override {}
};

} // namespace

DocumentController::DocumentController(QObject* parent)
    : QObject(parent), m_title(untitled()), m_relay(std::make_shared<IoRelay>())
{
    m_relay->target = this;
}

DocumentController::~DocumentController()
{
    {
        const std::lock_guard<std::mutex> lock(m_relay->mutex);
        m_relay->target = nullptr; // une opération en cours finit seule, sans rien rendre
    }
    if (m_fixed) {
        m_fixed->setSource(nullptr, 0);
    }
}

render::FixedPageItem* DocumentController::fixedPage() const noexcept
{
    return m_fixed.data();
}

void DocumentController::setFixedPage(render::FixedPageItem* page)
{
    if (page == m_fixed) {
        return;
    }
    if (m_fixed) {
        m_fixed->setSource(nullptr, 0);
    }
    m_fixed = page;
    attach();
    emit fixedPageChanged();
}

int DocumentController::pageCount() const noexcept
{
    return static_cast<int>(m_document.pageCount());
}

int DocumentController::currentIndex() const noexcept
{
    return static_cast<int>(m_current);
}

QString DocumentController::pageLabel() const
{
    return QStringLiteral("%1 / %2").arg(m_current + 1).arg(m_document.pageCount());
}

QSizeF DocumentController::pageSize() const
{
    const core::Page& page = m_document.page(m_current);
    return QSizeF(page.width, page.height);
}

bool DocumentController::isModified() const noexcept
{
    return m_modifiedOutsideStack || !m_undo.isClean();
}

QString DocumentController::title() const
{
    return m_title;
}

QUrl DocumentController::fileUrl() const
{
    return m_fileUrl;
}

QString DocumentController::fingerprintText() const
{
    return QStringLiteral("%1").arg(core::fingerprint(m_document), 16, 16, QLatin1Char('0'));
}

bool DocumentController::canUndo() const noexcept
{
    return m_undo.canUndo();
}

bool DocumentController::canRedo() const noexcept
{
    return m_undo.canRedo();
}

const core::Document& DocumentController::document() const noexcept
{
    return m_document;
}

core::Changes DocumentController::run(std::unique_ptr<core::UndoCommand> command)
{
    core::Changes changes;
    if (!command) {
        return changes;
    }
    command->redo(m_document, changes);
    if (!changes.empty()) {
        m_undo.push(m_document, std::make_unique<Executed>(std::move(command)));
    }
    return changes;
}

void DocumentController::execute(std::unique_ptr<core::UndoCommand> command)
{
    const core::Changes changes = run(std::move(command));
    if (changes.empty()) {
        return; // sans effet : ni pile, ni signal
    }
    notifyState(settle(changes));
}

// Lot 3 (annuler ou rétablir pendant un trait) : terminer le trait en cours AVANT m_undo.undo / m_undo.redo.
// Aujourd'hui aucune commande ni raccourci n'annule : non atteignable.
void DocumentController::undo()
{
    if (!m_undo.canUndo()) {
        return;
    }
    notifyState(settle(m_undo.undo(m_document)));
}

void DocumentController::redo()
{
    if (!m_undo.canRedo()) {
        return;
    }
    notifyState(settle(m_undo.redo(m_document)));
}

void DocumentController::commitStroke(core::Stroke stroke)
{
    core::PageObject object = std::move(stroke);
    object.common.layer = core::Layer::Top; // F-510 : les annotations vont sur la couche supérieure
    execute(std::make_unique<core::AddObjectCommand>(m_current, std::move(object)));
}

bool DocumentController::settle(const core::Changes& changes)
{
    const bool pages = std::any_of(changes.begin(), changes.end(), [](const core::Change& c) {
        return c.kind == core::Change::Kind::PageInserted || c.kind == core::Change::Kind::PageRemoved;
    });
    NoCache none;
    core::PageCache& cache =
        m_fixed ? static_cast<core::PageCache&>(*m_fixed) : static_cast<core::PageCache&>(none);
    const std::optional<std::size_t> shown = core::applyToCache(changes, m_current, cache);
    if (!shown || *shown != m_current) {
        // Page affichée retirée (même indice, borné au nombre de pages) ou déplacée par un ajout ou un
        // retrait placé avant elle : la page fixe la relit à son indice.
        m_current = shown ? *shown : std::min(m_current, m_document.pageCount() - 1);
        attach();
    }
    return pages;
}

void DocumentController::notifyState(bool pages)
{
    if (pages) {
        emit pagesChanged();
    }
    emit documentChanged();
    emit undoChanged();
    updateModified();
}

void DocumentController::updateModified()
{
    if (isModified() != m_wasModified) {
        m_wasModified = isModified();
        emit modifiedChanged();
    }
}

void DocumentController::attach()
{
    if (m_fixed) {
        m_fixed->setSource(&m_document, m_current);
    }
}

void DocumentController::show(std::size_t index)
{
    m_current = index;
    attach();
    emit pagesChanged();
}

void DocumentController::replaceDocument(core::Document document, const QUrl& fileUrl, const QString& title)
{
    emit pageAboutToChange();
    m_document = std::move(document);
    m_undo.clear();
    m_fileUrl = fileUrl;
    m_title = title.isEmpty() ? untitled() : title;
    m_modifiedOutsideStack = false;
    ++m_generation;
    show(0);
    emit fileUrlChanged();
    emit titleChanged();
    setWarnings({});
    setLastError({});
    notifyState(false);
}

void DocumentController::newDocument(double width, double height)
{
    replaceDocument(core::Document(width, height), QUrl(), untitled());
}

void DocumentController::nextPage()
{
    emit pageAboutToChange(); // le trait en cours rejoint la page affichée avant le déplacement
    if (m_current + 1 < m_document.pageCount()) {
        show(m_current + 1);
        return;
    }
    // La page ajoutée prend la taille de la dernière (relevée avant l'insertion, qui déplace les pages).
    const core::Page& last = m_document.page(m_document.pageCount() - 1);
    const std::size_t added = m_document.pageCount();
    const core::Changes changes = run(std::make_unique<core::AddPageCommand>(added, last.width, last.height));
    if (changes.empty()) {
        return;
    }
    settle(changes); // ajout après la page affichée : rien pour son cache
    m_current = added;
    attach();
    notifyState(true); // un seul pagesChanged, la nouvelle page déjà montrée
}

void DocumentController::pageBack()
{
    const std::size_t target = core::formerPageIndex(m_current);
    if (target == m_current) {
        return;
    }
    emit pageAboutToChange();
    show(target);
}

void DocumentController::loadGeneratedFirstPage()
{
    const QSizeF shownSize = pageSize();
    const bool modified = isModified();
    core::Page& first = m_document.page(0);
    first = core::generatePage(core::kDefaultSeed);
    // Identifiants tirés du compteur du document (1 à 300 dans un document neuf, comme le générateur) :
    // aucun doublon avec les autres pages ni avec les traits suivants. La page générée ne porte aucune
    // référence d'identifiant (groupe, connecteur, action) : la renuméroter ne casse rien.
    for (core::PageObject& object : first.objects) {
        object.common.id = m_document.allocateId();
    }
    m_undo.clear(); // les commandes visaient l'ancienne page 1
    // La page du banc n'est pas une modification ; celles des autres pages restent à enregistrer.
    m_modifiedOutsideStack = modified;
    if (m_current == 0) {
        attach();
    }
    notifyState(pageSize() != shownSize);
}

void DocumentController::loadGeneratedPages(int count, bool painted)
{
    std::vector<core::Page> pages;
    for (int i = 0; i < std::max(1, count); ++i) {
        pages.push_back(core::generatePage(core::kDefaultSeed + static_cast<std::uint32_t>(i)));
        core::Page& page = pages.back();
        if (!painted || i % 2 == 0) {
            continue;
        }
        for (int k = 0; k < 20; ++k) {
            core::Text text;
            text.bounds = core::RectF{0, 0, 220, 50};
            text.html = "<p style=\"font-size:32px; color:#202020\">Texte du banc</p>";
            core::PageObject t = text;
            t.common.transform = core::translation(40 + 45.0 * k, 60 + 30.0 * k);
            page.objects.push_back(std::move(t));
            core::Shape shape;
            shape.kind = core::ShapeKind::Path;
            shape.paths = {{{0, 0}, {80, 10}, {60, 70}, {-10, 50}}};
            shape.bounds = core::RectF{-12, -2, 94, 74}; // épaisseur de plume comprise
            shape.pen.width = 4;
            shape.brush = core::solidFill(0xFF26A69Au);
            core::PageObject s = shape;
            s.common.transform = core::translation(500 + 20.0 * k, 120 + 25.0 * k);
            page.objects.push_back(std::move(s));
        }
    }
    core::Document document(std::move(pages));
    // Chaque page générée numérote ses objets de 1 à 300 : renumérotés par le compteur du document (aucune
    // référence d'identifiant dans une page générée, voir loadGeneratedFirstPage).
    for (std::size_t p = 0; p < document.pageCount(); ++p) {
        for (core::PageObject& object : document.page(p).objects) {
            object.common.id = document.allocateId();
        }
    }
    replaceDocument(std::move(document), QUrl(), QString());
}

bool DocumentController::isBusy() const noexcept
{
    return m_busy;
}

QStringList DocumentController::warnings() const
{
    return m_warnings;
}

QString DocumentController::lastError() const
{
    return m_lastError;
}

QString DocumentController::suggestedFileName() const
{
    return m_title + QStringLiteral(".actcraft");
}

bool DocumentController::lastIoOffMainThread() const noexcept
{
    return m_lastIoOffMainThread;
}

QUrl DocumentController::urlFromArgument(const QVariant& argument) const
{
    return formats::urlFromArgument(argument);
}

QUrl DocumentController::defaultFolderUrl(bool create) const
{
    return QUrl::fromLocalFile(defaultDocumentsFolder(m_documentsRoot, create));
}

void DocumentController::setDocumentsRoot(const QString& documents)
{
    m_documentsRoot = documents;
}

void DocumentController::setBusy(bool busy)
{
    if (busy != m_busy) {
        m_busy = busy;
        emit busyChanged();
    }
}

void DocumentController::setWarnings(const QStringList& warnings)
{
    if (warnings != m_warnings) {
        m_warnings = warnings;
        emit warningsChanged();
    }
}

void DocumentController::setLastError(const QString& error)
{
    if (error != m_lastError) {
        m_lastError = error;
        emit lastErrorChanged();
    }
}

void DocumentController::setLastIo(bool offMainThread)
{
    m_lastIoOffMainThread = offMainThread;
    emit lastIoChanged(); // à chaque fin de lecture ou d'écriture, succès ou échec
}

void DocumentController::openUrl(const QUrl& url)
{
    if (m_busy) {
        QMetaObject::invokeMethod(
            this, [this] { emit loadFinished(false, tr("Une opération est déjà en cours.")); },
            Qt::QueuedConnection);
        return;
    }
    setBusy(true);
    // Relevés à l'appel : un trait, « Nouveau » ou « Fermer » pendant la lecture n'est jamais écrasé en
    // silence.
    const std::uint64_t generation = m_generation;
    const std::uint64_t before = core::fingerprint(m_document);
    std::shared_ptr<IoRelay> relay = m_relay;
    QThreadPool::globalInstance()->start([relay, url, generation, before] {
        auto outcome = std::make_shared<LoadOutcome>(loadDocument(url));
        relay->post([relay, url, outcome, generation, before] {
            DocumentController& self = *relay->target; // livré au contrôleur vivant (IoRelay)
            self.setBusy(false);
            self.setLastIo(outcome->offMainThread);
            if (outcome->ok &&
                (generation != self.m_generation || core::fingerprint(self.m_document) != before)) {
                const QString changed =
                    tr("Le paperboard a changé pendant l'ouverture : ouverture annulée, rien n'est perdu.");
                self.setLastError(changed);
                emit self.loadFinished(false, changed);
                return;
            }
            if (!outcome->ok) {
                self.setWarnings(outcome->warnings);
                self.setLastError(outcome->error);
                emit self.loadFinished(false, outcome->error);
                return;
            }
            // Un .flipchart devient un document sans fichier : « Enregistrer » proposera <nom>.actcraft.
            self.replaceDocument(std::move(outcome->document), outcome->native ? url : QUrl(),
                                 outcome->title);
            self.setWarnings(outcome->warnings); // après replaceDocument, qui les efface
            qCInfo(lcDocument, "ACTCRAFT OUVERT %s", qPrintable(self.fingerprintText()));
            emit self.loadFinished(true, QString());
        });
    });
}

void DocumentController::saveToUrl(const QUrl& url)
{
    if (m_busy) {
        QMetaObject::invokeMethod(
            this, [this] { emit saveFinished(false, tr("Une opération est déjà en cours.")); },
            Qt::QueuedConnection);
        return;
    }
    QString refusal;
    const QUrl target = actcraftTarget(url, &refusal);
    if (target.isEmpty()) {
        QMetaObject::invokeMethod(
            this,
            [this, refusal] {
                setLastError(refusal);
                emit saveFinished(false, refusal);
            },
            Qt::QueuedConnection);
        return;
    }
    setBusy(true);
    // Copie prise ici, sur le fil principal (ressources partagées implicitement) : le fil secondaire ne lit
    // jamais le document affiché, que l'utilisateur continue de modifier.
    auto snapshot = std::make_shared<const core::Document>(m_document);
    const std::uint64_t generation = m_generation;
    std::shared_ptr<IoRelay> relay = m_relay;
    QThreadPool::globalInstance()->start([relay, url = target, snapshot, generation] {
        const SaveOutcome outcome = saveDocument(*snapshot, url);
        relay->post([relay, url, outcome, generation] {
            DocumentController& self = *relay->target; // livré au contrôleur vivant (IoRelay)
            self.setBusy(false);
            self.setLastIo(outcome.offMainThread);
            if (!outcome.ok) {
                self.setLastError(outcome.error);
                emit self.saveFinished(false, outcome.error);
                return;
            }
            self.setLastError({});
            // Même document (pas de « Nouveau » entre-temps) : il prend ce fichier. « Modifié » faux ⇔
            // l'écran est le contenu du fichier : propre si rien n'a changé pendant l'écriture ; sinon
            // modifié jusqu'au prochain enregistrement, car l'ancien repère propre de la pile désigne un
            // contenu que le fichier n'a plus (trait annulé pendant l'écriture).
            if (generation == self.m_generation) {
                if (core::fingerprint(self.m_document) == outcome.fingerprint) {
                    self.m_undo.setClean();
                    self.m_modifiedOutsideStack = false; // la page du banc est dans le fichier
                } else {
                    self.m_modifiedOutsideStack = true;
                }
                if (url != self.m_fileUrl) {
                    self.m_fileUrl = url;
                    emit self.fileUrlChanged();
                }
                const QString title = titleFromUrl(url);
                if (title != self.m_title) {
                    self.m_title = title;
                    emit self.titleChanged();
                }
                self.updateModified(); // le document n'a pas changé : ni documentChanged, ni undoChanged
            }
            qCInfo(lcDocument, "ACTCRAFT ENREGISTRE %s",
                   qPrintable(QStringLiteral("%1").arg(outcome.fingerprint, 16, 16, QLatin1Char('0'))));
            emit self.saveFinished(true, QString());
        });
    });
}

} // namespace actcraft::ui
