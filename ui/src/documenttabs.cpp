#include "parkedsession.h"

#include <actcraft/core/pageedits.h>
#include <actcraft/ui/documenttabs.h>

#include <algorithm>
#include <utility>

namespace actcraft::ui {

using ParkedSession = DocumentController::ParkedSession;

// Contrainte 3 (plan 5a) : un seul DocumentController et une seule PageView pour tous les onglets ; un onglet
// inactif est une session garée, hors de la vue (≈ 39,6 Mio de cache par vue évités).
struct DocumentTabs::Tab
{
    std::unique_ptr<ParkedSession> parked;
};

namespace {

// Document vierge « Sans titre » d'une page de `size` (titre vide : resumeSession met « Sans titre »).
std::unique_ptr<ParkedSession> emptySession(QSizeF size)
{
    auto session = std::make_unique<ParkedSession>();
    session->document = core::Document(size.width(), size.height());
    return session;
}

} // namespace

DocumentTabs::DocumentTabs(QObject* parent) : QAbstractListModel(parent) {}

DocumentTabs::~DocumentTabs() = default;

int DocumentTabs::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_tabs.size());
}

QVariant DocumentTabs::data(const QModelIndex& index, int role) const
{
    if (!m_document || !index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }
    const int row = index.row();
    const ParkedSession* parked = m_tabs[static_cast<std::size_t>(row)].parked.get();
    switch (role) {
    case Qt::DisplayRole:
    case TitleRole:
        return parked ? parked->title : m_document->title();
    case ModifiedRole:
        return parked ? parked->isModified() : m_document->isModified();
    case CurrentRole:
        return row == m_current;
    case UrlRole:
        return parked ? parked->fileUrl : m_document->fileUrl();
    default:
        return {};
    }
}

QHash<int, QByteArray> DocumentTabs::roleNames() const
{
    return {{TitleRole, "title"}, {ModifiedRole, "modified"}, {CurrentRole, "current"}, {UrlRole, "url"}};
}

DocumentController* DocumentTabs::document() const
{
    return m_document;
}

// Le document affiché devient l'onglet unique ; les sessions garées d'un contrôleur précédent sont oubliées.
void DocumentTabs::setDocument(DocumentController* document)
{
    if (document == m_document) {
        return;
    }
    disconnect(m_titleConnection);
    disconnect(m_modifiedConnection);
    disconnect(m_urlConnection);
    beginResetModel();
    m_document = document;
    m_tabs.clear();
    if (m_document) {
        m_tabs.resize(1);
        const auto current = [this] { emitRow(m_current); };
        m_titleConnection = connect(m_document, &DocumentController::titleChanged, this, current);
        m_modifiedConnection = connect(m_document, &DocumentController::modifiedChanged, this, current);
        m_urlConnection = connect(m_document, &DocumentController::fileUrlChanged, this, current);
    }
    m_current = 0;
    endResetModel();
    emit documentChanged();
    emit countChanged();
    emit currentIndexChanged();
}

PageView* DocumentTabs::view() const
{
    return m_view;
}

// La vue unique (T21 la montre). Aucune demande de redessin n'en part ici : resumeSession rattache la page,
// et restart vide déjà la couche d'attente.
void DocumentTabs::setView(PageView* view)
{
    if (view == m_view) {
        return;
    }
    m_view = view;
    emit viewChanged();
}

int DocumentTabs::currentIndex() const
{
    return m_current;
}

QString DocumentTabs::position() const
{
    return m_position;
}

void DocumentTabs::setPosition(const QString& position)
{
    if (position == m_position) {
        return;
    }
    m_position = position;
    emit positionChanged();
}

Commands* DocumentTabs::commands() const
{
    return m_commands.data();
}

void DocumentTabs::setCommands(Commands* commands)
{
    if (commands == m_commands) {
        return;
    }
    m_commands = commands;
    emit commandsChanged();
}

// Pendant une lecture ou une écriture, le document affiché ne change pas (son résultat lui revient).
bool DocumentTabs::canSwitch() const
{
    return m_document && !m_document->isBusy();
}

// Amendement 12 (L6-T31) : le modèle refuse lui-même, une garde de la barre seule laisserait passer tout
// autre QML.
bool DocumentTabs::allows(const char* command) const
{
    return !m_commands || m_commands->allowsWhileLocked(QString::fromLatin1(command));
}

void DocumentTabs::emitRow(int row)
{
    if (row >= 0 && row < rowCount()) {
        const QModelIndex at = index(row);
        emit dataChanged(at, at);
    }
}

// L'onglet `index` (déjà dans m_tabs) devient l'onglet affiché ; `leaving` (session garée de l'onglet quitté,
// ou nulle s'il est fermé) reste à sa place. Un garage, une reprise : une reconstruction, un redessin.
void DocumentTabs::switchTo(int index, std::unique_ptr<ParkedSession> leaving)
{
    const int old = m_current;
    if (old >= 0 && old < rowCount() && old != index) {
        m_tabs[static_cast<std::size_t>(old)].parked = std::move(leaving);
    }
    std::unique_ptr<ParkedSession> arriving = std::move(m_tabs[static_cast<std::size_t>(index)].parked);
    m_current = index;
    m_document->resumeSession(std::move(arriving));
    emit currentIndexChanged();
    emitRow(old);
    emitRow(index);
}

void DocumentTabs::newTab()
{
    if (!canSwitch() || !allows("file.new")) {
        return;
    }
    const QSizeF size = m_document->pageSize();
    std::unique_ptr<ParkedSession> leaving = m_document->parkSession();
    const int row = rowCount();
    beginInsertRows({}, row, row);
    m_tabs.push_back(Tab{emptySession(size)});
    endInsertRows();
    emit countChanged();
    switchTo(row, std::move(leaving));
}

// Le document est lu dans un nouvel onglet ; un échec laisse l'onglet vide « Sans titre ».
void DocumentTabs::openInNewTab(const QUrl& url)
{
    if (!canSwitch()) {
        return;
    }
    const int before = rowCount();
    newTab();
    if (rowCount() == before) {
        return; // onglet refusé : le document affiché n'est pas remplacé
    }
    m_document->openUrl(url);
}

void DocumentTabs::activate(int index)
{
    if (!canSwitch() || index < 0 || index >= rowCount() || index == m_current) {
        return;
    }
    switchTo(index, m_document->parkSession());
}

bool DocumentTabs::isModifiedAt(int index) const
{
    return index >= 0 && index < rowCount() && data(this->index(index), ModifiedRole).toBool();
}

bool DocumentTabs::requestClose(int index)
{
    if (!m_document || index < 0 || index >= rowCount() || !allows("file.close")) {
        return false;
    }
    const ParkedSession* parked = m_tabs[static_cast<std::size_t>(index)].parked.get();
    if (parked ? parked->isModified() : m_document->isModified()) {
        emit saveRequested(index);
        return false;
    }
    closeWithoutSaving(index);
    return true;
}

// Un document vide reste toujours ouvert : le dernier onglet fermé est remplacé par un vide.
void DocumentTabs::closeWithoutSaving(int index)
{
    if (!m_document || index < 0 || index >= rowCount() || !allows("file.close")) {
        return;
    }
    if (index != m_current) {
        beginRemoveRows({}, index, index);
        m_tabs.erase(m_tabs.begin() + index);
        endRemoveRows();
        emit countChanged();
        if (index < m_current) {
            --m_current;
            emit currentIndexChanged();
        }
        return;
    }
    if (!canSwitch()) {
        return;
    }
    const QSizeF size = m_document->pageSize();
    m_document->parkSession(); // le document fermé est abandonné
    if (rowCount() == 1) {
        m_document->resumeSession(emptySession(size));
        emitRow(0);
        return;
    }
    // L'onglet de droite prend la place, ou celui de gauche pour le dernier.
    const int neighbour = index + 1 < rowCount() ? index + 1 : index - 1;
    std::unique_ptr<ParkedSession> arriving = std::move(m_tabs[static_cast<std::size_t>(neighbour)].parked);
    beginRemoveRows({}, index, index);
    m_tabs.erase(m_tabs.begin() + index);
    endRemoveRows();
    m_current = neighbour > index ? neighbour - 1 : neighbour;
    m_document->resumeSession(std::move(arriving));
    emit countChanged();
    emit currentIndexChanged();
    emitRow(m_current);
}

// F-011 : les pages copiées vont avant la première page de l'onglet d'arrivée, en une étape de sa pile (A15,
// core::InsertPagesCommand) ; un onglet garé la reçoit sur sa pile garée, sans être affiché, et montrera la
// première page copiée.
bool DocumentTabs::copyPagesToTab(const QVariantList& pageIndexes, int tabIndex)
{
    if (!canSwitch() || tabIndex < 0 || tabIndex >= rowCount() || !allows("page.paste")) {
        return false;
    }
    const core::Document& source = m_document->document();
    std::vector<std::size_t> indexes;
    for (const QVariant& value : pageIndexes) {
        bool ok = false;
        const int i = value.toInt(&ok);
        if (ok && i >= 0 && static_cast<std::size_t>(i) < source.pageCount()) {
            indexes.push_back(static_cast<std::size_t>(i));
        }
    }
    std::sort(indexes.begin(), indexes.end());
    indexes.erase(std::unique(indexes.begin(), indexes.end()), indexes.end());
    if (indexes.empty()) {
        return false;
    }
    std::vector<core::Page> pages;
    pages.reserve(indexes.size());
    for (const std::size_t i : indexes) {
        pages.push_back(source.page(i));
    }
    std::map<std::string, core::Resource> resources = core::pageResources(source, pages);
    auto command = std::make_unique<core::InsertPagesCommand>(0, std::move(pages), std::move(resources));
    if (tabIndex == m_current) {
        m_document->execute(std::move(command));
        return true;
    }
    ParkedSession& target = *m_tabs[static_cast<std::size_t>(tabIndex)].parked;
    if (target.undo.push(target.document, std::move(command)).empty()) {
        return false;
    }
    target.current = 0;
    emitRow(tabIndex);
    return true;
}

void DocumentTabs::next()
{
    if (rowCount() > 1) {
        activate((m_current + 1) % rowCount());
    }
}

void DocumentTabs::previous()
{
    if (rowCount() > 1) {
        activate((m_current + rowCount() - 1) % rowCount());
    }
}

} // namespace actcraft::ui
