#include <actcraft/core/edits.h>
#include <actcraft/core/generator.h>
#include <actcraft/core/pagecache.h>
#include <actcraft/ui/documentcontroller.h>

#include <algorithm>
#include <optional>
#include <utility>

namespace actcraft::ui {

namespace {

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

DocumentController::DocumentController(QObject* parent) : QObject(parent), m_title(untitled()) {}

DocumentController::~DocumentController()
{
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
    show(0);
    emit fileUrlChanged();
    emit titleChanged();
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

} // namespace actcraft::ui
