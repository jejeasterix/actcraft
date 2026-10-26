#include <actcraft/core/edits.h>
#include <actcraft/core/pageedits.h>
#include <actcraft/ui/pagescontroller.h>

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <utility>

namespace actcraft::ui {

namespace {

constexpr int kShift = static_cast<int>(Qt::ShiftModifier);
constexpr int kCtrl = static_cast<int>(Qt::ControlModifier);

std::vector<std::size_t> toIndices(const QList<int>& selection)
{
    std::vector<std::size_t> indices;
    indices.reserve(static_cast<std::size_t>(selection.size()));
    for (const int index : selection) {
        indices.push_back(static_cast<std::size_t>(index));
    }
    return indices;
}

QList<int> range(int first, int count)
{
    QList<int> list;
    for (int i = 0; i < count; ++i) {
        list.push_back(first + i);
    }
    return list;
}

// Garde : les signaux de la page affichée émis pendant une opération n'y changent pas la sélection.
struct Working
{
    explicit Working(bool& flag) : m_flag(flag) { m_flag = true; }
    ~Working() { m_flag = false; }
    Working(const Working&) = delete;
    Working& operator=(const Working&) = delete;
    bool& m_flag;
};

// L4-C2 : avant une commande de pages, ou une copie qui lit les pages, l'édition en cours est validée par le
// chemin d'announceEdit (documentcontroller.cpp) : editAboutToApply (le texte se valide, les outils
// abandonnent leur geste), puis dépôt forcé de ce qui resterait soulevé. execute() n'annonce rien (B1,
// L3-T12).
void settleEditing(DocumentController& controller)
{
    emit controller.editAboutToApply();
    if (controller.isLifting()) {
        controller.dropLifted(nullptr);
    }
}

} // namespace

PagesController::PagesController(QObject* parent) : QObject(parent) {}

DocumentController* PagesController::documentController() const noexcept
{
    return m_controller.data();
}

void PagesController::setDocumentController(DocumentController* controller)
{
    if (m_controller == controller) {
        return;
    }
    QObject::disconnect(m_pagesConnection);
    m_controller = controller;
    if (m_controller) {
        m_pagesConnection = connect(m_controller, &DocumentController::pagesChanged, this, [this] {
            if (!m_working) {
                sync();
            }
        });
        m_anchor = m_controller->currentIndex();
        setSelection({m_anchor});
    } else {
        setSelection({});
    }
    emit wiringChanged();
}

QList<int> PagesController::selection() const
{
    return m_selection;
}

bool PagesController::canPaste() const noexcept
{
    return !m_clipboard.pages.empty();
}

void PagesController::setSelection(const QList<int>& selection)
{
    if (m_selection != selection) {
        m_selection = selection;
        emit selectionChanged();
    }
}

void PagesController::sync()
{
    if (!m_controller) {
        return;
    }
    const int count = m_controller->pageCount();
    const int current = m_controller->currentIndex();
    QList<int> kept;
    for (const int index : std::as_const(m_selection)) {
        if (index >= 0 && index < count) {
            kept.push_back(index);
        }
    }
    if (kept.isEmpty() || !kept.contains(current)) {
        kept = {current};
        m_anchor = current;
    }
    m_anchor = std::clamp(m_anchor, 0, count - 1);
    setSelection(kept);
}

void PagesController::apply(const QList<int>& selection, int shown)
{
    if (!m_controller) {
        return;
    }
    // M-J (L4-C11) : la page déjà affichée ne se remontre pas. showPage annoncerait un changement de page et
    // referait la page fixe (show → attach → FixedPageItem::setSource) sans que rien ne change ; après une
    // commande, settle() a déjà recalé la page fixe sur la page affichée.
    if (shown != m_controller->currentIndex()) {
        const Working working(m_working);
        m_controller->showPage(shown);
    }
    setSelection(selection);
    sync();
}

void PagesController::click(int index, int modifiers)
{
    if (!m_controller || index < 0 || index >= m_controller->pageCount()) {
        return;
    }
    if ((modifiers & kShift) != 0) {
        const int anchor = std::clamp(m_anchor, 0, m_controller->pageCount() - 1);
        apply(range(std::min(anchor, index), std::abs(index - anchor) + 1), index);
        return;
    }
    if ((modifiers & kCtrl) != 0) {
        QList<int> next = m_selection;
        if (next.contains(index)) {
            if (next.size() == 1) {
                return; // la dernière page sélectionnée ne se retire pas
            }
            next.removeAll(index);
            m_anchor = index;
            const int current = m_controller->currentIndex();
            apply(next, next.contains(current) ? current : next.last());
            return;
        }
        next.push_back(index);
        std::sort(next.begin(), next.end());
        m_anchor = index;
        apply(next, index);
        return;
    }
    m_anchor = index;
    apply({index}, index);
}

void PagesController::goTo(int index)
{
    if (!m_controller) {
        return;
    }
    const int shown = std::clamp(index, 0, m_controller->pageCount() - 1);
    m_anchor = shown;
    apply({shown}, shown);
}

void PagesController::moveSelection(int before)
{
    if (!m_controller) {
        return;
    }
    settleEditing(*m_controller);
    const std::size_t count = static_cast<std::size_t>(m_controller->pageCount());
    const std::size_t target = static_cast<std::size_t>(std::clamp(before, 0, m_controller->pageCount()));
    const std::vector<std::size_t> pages = toIndices(m_selection);
    const std::vector<std::size_t> after = core::positionsAfterMove(count, pages, target);
    {
        const Working working(m_working);
        m_controller->execute(std::make_unique<core::MovePagesCommand>(pages, target));
    }
    QList<int> moved;
    for (const std::size_t index : after) {
        moved.push_back(static_cast<int>(index));
    }
    if (moved.isEmpty()) {
        sync();
        return;
    }
    m_anchor = moved.first();
    apply(moved, moved.first());
}

void PagesController::insertBlank(bool afterShown)
{
    if (!m_controller) {
        return;
    }
    settleEditing(*m_controller);
    const int current = m_controller->currentIndex();
    const int at = current + (afterShown ? 1 : 0);
    const core::Page& shown = m_controller->document().page(static_cast<std::size_t>(current));
    {
        const Working working(m_working);
        m_controller->execute(
            std::make_unique<core::AddPageCommand>(static_cast<std::size_t>(at), shown.width, shown.height));
    }
    m_anchor = at;
    apply({at}, at);
}

std::vector<core::Page> PagesController::selectedPages() const
{
    std::vector<core::Page> pages;
    for (const int index : m_selection) {
        pages.push_back(m_controller->document().page(static_cast<std::size_t>(index)));
    }
    return pages;
}

// M-K (L4-C11) : les clés seules ne suffisent pas, un autre document ouvert depuis n'a pas leurs octets.
void PagesController::capture()
{
    std::vector<core::Page> pages = selectedPages();
    std::map<std::string, core::Resource> resources = core::pageResources(m_controller->document(), pages);
    m_clipboard = Clipboard{std::move(pages), std::move(resources)};
    emit clipboardChanged();
}

void PagesController::copySelection()
{
    if (!m_controller) {
        return;
    }
    settleEditing(*m_controller);
    capture();
}

// La validation précède selectedPages() : celle de deleteSelection, qui suit, viendrait après la lecture des
// pages et le presse-papiers recevrait la page sans le texte en cours.
void PagesController::cutSelection()
{
    if (!m_controller || m_selection.size() >= m_controller->pageCount()) {
        return; // couper toutes les pages est refusé, comme supprimer
    }
    settleEditing(*m_controller);
    capture();
    deleteSelection();
}

void PagesController::insertCopies(std::size_t index, const std::vector<core::Page>& pages,
                                   std::map<std::string, core::Resource> resources)
{
    const int before = m_controller->pageCount();
    {
        const Working working(m_working);
        m_controller->execute(std::make_unique<core::InsertPagesCommand>(index, pages, std::move(resources)));
    }
    const int added = m_controller->pageCount() - before;
    if (added <= 0) {
        sync(); // identifiants épuisés : rien n'est inséré
        return;
    }
    m_anchor = static_cast<int>(index);
    apply(range(static_cast<int>(index), added), static_cast<int>(index));
}

void PagesController::paste()
{
    if (!m_controller || m_clipboard.pages.empty()) {
        return;
    }
    settleEditing(*m_controller);
    insertCopies(static_cast<std::size_t>(m_controller->currentIndex()), m_clipboard.pages,
                 m_clipboard.resources);
}

void PagesController::duplicateSelection()
{
    if (!m_controller) {
        return;
    }
    settleEditing(*m_controller);
    insertCopies(static_cast<std::size_t>(m_selection.last()) + 1, selectedPages());
}

void PagesController::deleteSelection()
{
    if (!m_controller || m_selection.size() >= m_controller->pageCount()) {
        return; // la dernière page ne se supprime pas
    }
    settleEditing(*m_controller);
    {
        const Working working(m_working);
        m_controller->execute(std::make_unique<core::RemovePagesCommand>(toIndices(m_selection)));
    }
    m_selection.clear();
    sync();
}

} // namespace actcraft::ui
