#include <actcraft/render/pagepainter.h>
#include <actcraft/ui/pagethumbnails.h>

#include <QElapsedTimer>
#include <QMetaObject>
#include <QPainter>
#include <QThread>
#include <QThreadPool>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <variant>

namespace actcraft::ui {

// Fil du réservoir → modèle, patron d'IoRelay (documentcontroller.cpp) : le fil poste son résultat sous le
// verrou, le destructeur du modèle efface la cible sous le même verrou ; un résultat posté avant la
// destruction est retiré par ~QObject. Un QPointer lu depuis un autre fil ne le garantirait pas.
struct ThumbnailRelay
{
    std::mutex mutex;
    PageThumbnails* target = nullptr;

    template <typename F> void post(F&& function)
    {
        const std::lock_guard<std::mutex> lock(mutex);
        if (target) {
            QMetaObject::invokeMethod(target, std::forward<F>(function), Qt::QueuedConnection);
        }
    }
};

namespace {

constexpr int kMinWidth = 80;
constexpr int kMaxWidth = 320;
// Comme les miniatures de l'enregistrement (documentio.cpp, kThumbnailImageSide) : une image n'est jamais
// décodée plus grande que le double de la plus grande miniature.
constexpr int kImageSide = 2 * kMaxWidth;

using ResourceMap = std::map<std::string, core::Resource>;

// Ressources que le peintre cherche pour cette page (pagepainter.cpp : paintBackground, paintImage,
// paintMedia). Leurs octets sont partagés implicitement : la copie ne duplique rien.
ResourceMap resourcesOf(const core::Page& page, const core::Document& document)
{
    ResourceMap out;
    const auto add = [&out, &document](const std::string& key) {
        if (key.empty() || out.count(key) != 0) {
            return;
        }
        if (const core::Resource* resource = document.resource(key)) {
            out.emplace(key, *resource);
        }
    };
    add(page.background.image);
    for (const core::PageObject& object : page.objects) {
        if (const auto* image = std::get_if<core::ImageObject>(&object)) {
            add(image->resource);
        } else if (const auto* media = std::get_if<core::Media>(&object)) {
            add(media->placeholder);
        }
    }
    return out;
}

// Fil du réservoir : ne lit que la copie. Images décodées par la même voie que les miniatures de
// l'enregistrement (documentio.cpp, saveDocument : render::decodeImages puis sa recherche d'images), sur un
// document qui ne porte que les ressources de la page (mêmes octets, donc même clé SHA-256).
QImage renderThumbnail(const core::Page& page, const ResourceMap& resources, int width)
{
    if (!std::isfinite(page.width) || !std::isfinite(page.height) || page.width <= 0 || page.height <= 0) {
        QImage blank(width, width * 3 / 4, QImage::Format_ARGB32_Premultiplied);
        blank.fill(Qt::white);
        return blank;
    }
    core::Document holder;
    for (const auto& [key, resource] : resources) {
        static_cast<void>(key);
        static_cast<void>(holder.addResource(resource.data, resource.extension));
    }
    const render::DecodedImages images = render::decodeImages(holder, kImageSide);
    const double scale = static_cast<double>(width) / page.width;
    const int height =
        std::clamp(static_cast<int>(std::lround(page.height * scale)), 1, render::kThumbnailMaxHeight);
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.setDotsPerMeterX(3780); // 96 ppp, comme render::renderPage
    image.setDotsPerMeterY(3780);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.scale(scale, scale);
    render::PaintOptions options;
    options.thumbnail = true;
    render::paintPage(painter, page, images.lookup(), options);
    painter.end();
    return image;
}

} // namespace

PageThumbnails::PageThumbnails(QObject* parent)
    : QAbstractListModel(parent), m_relay(std::make_shared<ThumbnailRelay>())
{
    m_relay->target = this;
    m_settle.setSingleShot(true);
    m_settle.setInterval(kSettleMs);
    connect(&m_settle, &QTimer::timeout, this, &PageThumbnails::pump);
}

PageThumbnails::~PageThumbnails()
{
    {
        const std::lock_guard<std::mutex> lock(m_relay->mutex);
        m_relay->target = nullptr; // un rendu encore en vol ne poste plus rien
    }
    detachController();
    disconnect(m_viewConnection);
}

int PageThumbnails::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_images.size());
}

QVariant PageThumbnails::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || static_cast<std::size_t>(index.row()) >= m_images.size()) {
        return {};
    }
    const int row = index.row();
    const auto at = static_cast<std::size_t>(row);
    switch (role) {
    case PageNumberRole:
        return row + 1;
    case RevisionRole:
        return m_revisions[at];
    case AspectRole: {
        if (!m_controller || at >= m_controller->document().pageCount()) {
            return 0.75;
        }
        const core::Page& page = m_controller->document().page(at);
        return page.width > 0 && std::isfinite(page.height / page.width) ? page.height / page.width : 0.75;
    }
    case CurrentRole:
        return row == m_current;
    default:
        return {};
    }
}

QHash<int, QByteArray> PageThumbnails::roleNames() const
{
    return {{PageNumberRole, "pageNumber"},
            {RevisionRole, "revision"},
            {AspectRole, "aspect"},
            {CurrentRole, "current"}};
}

DocumentController* PageThumbnails::documentController() const noexcept
{
    return m_controller.data();
}

void PageThumbnails::setDocumentController(DocumentController* controller)
{
    if (m_controller == controller) {
        return;
    }
    detachController();
    m_controller = controller;
    attachController();
    resetRows(nullptr);
    emit wiringChanged();
}

void PageThumbnails::attachController()
{
    if (!m_controller) {
        return;
    }
    DocumentController* controller = m_controller.data();
    m_observerToken = controller->addChangeObserver(
        [this](const core::Changes& changes, DocumentController::ChangeOrigin) { onChanges(changes); });
    m_controllerConnections = {
        connect(controller, &DocumentController::pageContentChanged, this, &PageThumbnails::mark),
        connect(controller, &DocumentController::documentChanged, this, &PageThumbnails::onDocumentChanged),
        connect(controller, &DocumentController::pagesChanged, this, &PageThumbnails::onPagesChanged),
        // M-G : fin d'un soulèvement, la page affichée retenue attend encore le délai de calme.
        connect(controller, &DocumentController::liftingChanged, this,
                [this] {
                    if (m_controller && !m_controller->isLifting() && m_pending > 0) {
                        m_settle.start();
                    }
                }),
    };
    m_sawChanges = false;
}

void PageThumbnails::detachController()
{
    for (const QMetaObject::Connection& connection : std::as_const(m_controllerConnections)) {
        disconnect(connection);
    }
    m_controllerConnections.clear();
    if (m_controller && m_observerToken > 0) {
        m_controller->removeChangeObserver(m_observerToken);
    }
    m_observerToken = 0;
}

PageView* PageThumbnails::view() const noexcept
{
    return m_view.data();
}

void PageThumbnails::setView(PageView* view)
{
    if (m_view == view) {
        return;
    }
    disconnect(m_viewConnection);
    m_view = view;
    if (m_view) {
        m_viewConnection =
            connect(m_view.data(), &PageView::contactActiveChanged, this, &PageThumbnails::onContactChanged);
    }
    emit wiringChanged();
}

int PageThumbnails::thumbnailWidth() const noexcept
{
    return m_width;
}

void PageThumbnails::setThumbnailWidth(int width)
{
    width = std::clamp(width, kMinWidth, kMaxWidth);
    if (m_width == width) {
        return;
    }
    m_width = width;
    emit thumbnailWidthChanged();
    markAll();
}

int PageThumbnails::pendingCount() const noexcept
{
    return m_pending;
}

int PageThumbnails::renderCount() const noexcept
{
    return m_renders;
}

double PageThumbnails::lastCopyMs() const noexcept
{
    return m_lastCopyMs;
}

int PageThumbnails::launchCount() const noexcept
{
    return m_launches;
}

QImage PageThumbnails::image(int page) const
{
    return page >= 0 && static_cast<std::size_t>(page) < m_images.size()
               ? m_images[static_cast<std::size_t>(page)]
               : QImage();
}

void PageThumbnails::refreshAll()
{
    markAll();
}

QThread* PageThumbnails::lastRenderThread() const noexcept
{
    return m_lastRenderThread;
}

bool PageThumbnails::active() const noexcept
{
    return m_active;
}

void PageThumbnails::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    m_active = active;
    if (m_active && m_pending > 0) {
        m_settle.start(); // pages marquées pendant la suspension : rendues kSettleMs plus tard
    }
    emit activeChanged();
}

void PageThumbnails::setSettleDelay(int ms)
{
    m_settle.setInterval(std::max(ms, 0));
}

int PageThumbnails::settleDelay() const noexcept
{
    return m_settle.interval();
}

// M-H (L4-C10) : les marques suivent leur page comme les images ; seules les pages insérées et celle du rendu
// en vol (jeté à son retour) sont à rendre. Tout marquer refaisait N rendus par opération de page, la page
// insérée en dernier.
void PageThumbnails::resetRows(const core::Changes* changes)
{
    beginResetModel();
    ++m_generation; // un rendu en vol vise l'ancienne disposition : jeté à son retour
    if (changes) {
        if (m_inFlight && m_inFlightPage >= 0 && static_cast<std::size_t>(m_inFlightPage) < m_marked.size()) {
            m_marked[static_cast<std::size_t>(m_inFlightPage)] = 1; // à refaire, suit sa page
        }
        for (const core::Change& change : *changes) {
            const auto at = static_cast<std::ptrdiff_t>(change.page);
            if (change.kind == core::Change::Kind::PageRemoved && change.page < m_images.size()) {
                m_images.erase(m_images.begin() + at);
                m_revisions.erase(m_revisions.begin() + at);
                m_marked.erase(m_marked.begin() + at);
            } else if (change.kind == core::Change::Kind::PageInserted && change.page <= m_images.size()) {
                m_images.insert(m_images.begin() + at, QImage());
                m_revisions.insert(m_revisions.begin() + at, 0);
                m_marked.insert(m_marked.begin() + at, char{1});
            }
        }
    } else {
        m_images.clear();
        m_revisions.clear();
        m_marked.clear();
    }
    const std::size_t count = m_controller ? m_controller->document().pageCount() : 0;
    m_images.resize(count);
    m_revisions.resize(count, 0);
    m_marked.resize(count, char{1}); // document remplacé : tout ; sinon déjà à la taille
    m_current = m_controller ? m_controller->currentIndex() : -1;
    endResetModel();
    if (std::find(m_marked.begin(), m_marked.end(), char{1}) != m_marked.end()) {
        m_settle.start();
    }
    updatePending();
    emit renderingChanged();
}

void PageThumbnails::onChanges(const core::Changes& changes)
{
    m_sawChanges = true;
    // Défaut 11 : tout PageInserted ou PageRemoved refait les lignes, même à nombre de pages égal (un
    // déplacement retire puis insère). Les autres pages touchées arrivent par pageContentChanged.
    const bool pages = std::any_of(changes.begin(), changes.end(), [](const core::Change& change) {
        return change.kind == core::Change::Kind::PageInserted ||
               change.kind == core::Change::Kind::PageRemoved;
    });
    if (pages) {
        resetRows(&changes);
    }
}

void PageThumbnails::onDocumentChanged()
{
    // Toute modification versée passe par l'observateur avant documentChanged (DocumentController::settle
    // puis notifyState). Un documentChanged sans lui vient d'un document remplacé (replaceDocument : nouveau,
    // ouverture) ou de la page générée du banc (loadGeneratedFirstPage) : tout est refait.
    if (!m_sawChanges) {
        resetRows(nullptr);
    }
    m_sawChanges = false;
}

void PageThumbnails::onPagesChanged()
{
    if (!m_controller) {
        return;
    }
    if (static_cast<std::size_t>(rowCount()) != m_controller->document().pageCount()) {
        resetRows(nullptr);
        m_sawChanges = true; // déjà refait : le documentChanged qui suit (replaceDocument) n'y revient pas
        return;
    }
    const int current = m_controller->currentIndex();
    if (current == m_current) {
        return;
    }
    const int former = m_current;
    m_current = current;
    for (const int row : {former, current}) {
        if (row >= 0 && row < rowCount()) {
            emit dataChanged(index(row), index(row), {CurrentRole, AspectRole});
        }
    }
}

void PageThumbnails::onContactChanged()
{
    // Fin d'un contact : le rendu attend encore kSettleMs (l'écriture reprend souvent au trait suivant).
    if (!contactInProgress() && m_pending > 0) {
        m_settle.start();
    }
}

void PageThumbnails::mark(int page)
{
    if (page < 0 || static_cast<std::size_t>(page) >= m_marked.size()) {
        return;
    }
    m_marked[static_cast<std::size_t>(page)] = 1;
    m_settle.start(); // relancé : kSettleMs après la dernière marque
    updatePending();
    emit renderingChanged();
}

void PageThumbnails::markAll()
{
    if (m_marked.empty()) {
        return;
    }
    std::fill(m_marked.begin(), m_marked.end(), char{1});
    m_settle.start();
    updatePending();
    emit renderingChanged();
}

bool PageThumbnails::contactInProgress() const
{
    return m_view && m_view->contactActive();
}

void PageThumbnails::pump()
{
    if (m_inFlight || !m_controller || m_settle.isActive()) {
        return; // un rendu en vol relance à son retour ; une marque récente, à l'échéance du minuteur
    }
    if (contactInProgress()) {
        return; // repris kSettleMs après la fin du contact (onContactChanged)
    }
    // M-G : pendant un soulèvement (texte en édition, objets déplacés), la page affichée n'a plus les objets
    // soulevés ; sa miniature garde l'image d'avant et attend le dépôt, comme Enregistrer (liftObjects,
    // documentcontroller.h). Les autres pages se rendent.
    const int shown = m_controller->currentIndex();
    const int held = m_controller->isLifting() ? shown : -1;
    const auto due = [this, held](int row) {
        return row >= 0 && row != held && static_cast<std::size_t>(row) < m_marked.size() &&
               m_marked[static_cast<std::size_t>(row)] != 0;
    };
    // M-H : la page affichée d'abord, puis les autres dans l'ordre.
    if (due(shown)) {
        launch(shown);
        return;
    }
    for (int row = 0; row < static_cast<int>(m_marked.size()); ++row) {
        if (due(row)) {
            launch(row);
            return;
        }
    }
}

void PageThumbnails::launch(int page)
{
    const auto at = static_cast<std::size_t>(page);
    const core::Document& document = m_controller->document();
    if (at >= document.pageCount()) {
        resetRows(nullptr);
        return;
    }
    // Fil principal : la copie seule (page et ressources qu'elle cite), temps relevé.
    QElapsedTimer clock;
    clock.start();
    auto copy = std::make_shared<const core::Page>(document.page(at));
    auto resources = std::make_shared<const ResourceMap>(resourcesOf(*copy, document));
    m_lastCopyMs = static_cast<double>(clock.nsecsElapsed()) / 1e6;
    m_marked[at] = 0;
    m_inFlight = true;
    m_inFlightPage = page;
    ++m_launches; // amendement 3 (m4) : compté au lancement, sur le fil principal
    const std::uint64_t generation = m_generation;
    const int width = m_width;
    std::shared_ptr<ThumbnailRelay> relay = m_relay;
    QThreadPool::globalInstance()->start([relay, copy, resources, width, generation, page] {
        const QImage rendered = renderThumbnail(*copy, *resources, width);
        QThread* thread = QThread::currentThread();
        relay->post([relay, rendered, thread, generation, page] {
            relay->target->finish(generation, page, rendered, thread); // livré au modèle vivant
        });
    });
    updatePending();
    emit renderingChanged(); // en dernier : un récepteur peut remplacer le document ici
}

void PageThumbnails::finish(std::uint64_t generation, int page, const QImage& image, QThread* thread)
{
    m_inFlight = false;
    m_inFlightPage = -1;
    m_lastRenderThread = thread;
    const auto at = static_cast<std::size_t>(page);
    if (generation == m_generation && at < m_images.size()) {
        m_images[at] = image;
        m_revisions[at] = ++m_revisionCounter;
        ++m_renders;
        emit dataChanged(index(page), index(page), {RevisionRole, AspectRole});
    }
    updatePending();
    emit renderingChanged();
    pump(); // page suivante marquée, une à la fois
}

void PageThumbnails::updatePending()
{
    m_pending =
        static_cast<int>(std::count(m_marked.begin(), m_marked.end(), char{1})) + (m_inFlight ? 1 : 0);
}

ThumbnailItem::ThumbnailItem(QQuickItem* parent) : QQuickPaintedItem(parent) {}

PageThumbnails* ThumbnailItem::source() const noexcept
{
    return m_source.data();
}

void ThumbnailItem::setSource(PageThumbnails* source)
{
    if (m_source == source) {
        return;
    }
    m_source = source;
    emit sourceChanged();
    update();
}

int ThumbnailItem::page() const noexcept
{
    return m_page;
}

void ThumbnailItem::setPage(int page)
{
    if (m_page == page) {
        return;
    }
    m_page = page;
    emit pageChanged();
    update();
}

int ThumbnailItem::revision() const noexcept
{
    return m_revision;
}

void ThumbnailItem::setRevision(int revision)
{
    if (m_revision == revision) {
        return;
    }
    m_revision = revision;
    emit revisionChanged();
    update(); // nouvelle image de la page
}

void ThumbnailItem::paint(QPainter* painter)
{
    // Lu pendant la synchronisation (fil principal bloqué) : images écrites sur le fil principal seulement.
    const QRectF target = boundingRect();
    const QImage image = m_source ? m_source->image(m_page) : QImage();
    if (image.isNull()) {
        painter->fillRect(target, Qt::white); // avant le premier rendu
        return;
    }
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->drawImage(target, image);
}

} // namespace actcraft::ui
