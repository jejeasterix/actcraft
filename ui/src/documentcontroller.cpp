#include <actcraft/core/edits.h>
#include <actcraft/core/generator.h>
#include <actcraft/core/objectedits.h>
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

// Empreinte en 16 chiffres hexadécimaux (journal, propriété `fingerprint`).
QString fingerprintHex(std::uint64_t fingerprint)
{
    return QStringLiteral("%1").arg(fingerprint, 16, 16, QLatin1Char('0'));
}

QString untitled()
{
    return QStringLiteral("Sans titre"); // F-656
}

// Page vierge de même taille : référence de Rétablir la page pour une page jamais enregistrée (D9).
core::Page blankLike(const core::Page& page)
{
    core::Page blank;
    blank.width = page.width;
    blank.height = page.height;
    return blank;
}

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
    return fingerprintHex(core::fingerprint(m_document));
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
        ++m_revision;
        m_undo.pushExecuted(std::move(command)); // déjà exécutée ci-dessus
    }
    return changes;
}

void DocumentController::execute(std::unique_ptr<core::UndoCommand> command)
{
    executeAs(std::move(command), ChangeOrigin::Command);
}

void DocumentController::executeAs(std::unique_ptr<core::UndoCommand> command, ChangeOrigin origin)
{
    const core::Changes changes = run(std::move(command));
    if (changes.empty()) {
        return; // sans effet : ni pile, ni signal, ni observateur
    }
    notifyState(settle(changes, origin));
}

// L2-T15 : l'annonce d'abord. La vue y termine son trait, qui entre dans la pile et devient l'étape annulée ;
// versé après, il irait à la pile déjà bougée et effacerait la branche à rétablir. Ensuite, ce qui reste
// soulevé revient sans étape (l'outil a abandonné son geste à l'annonce).
void DocumentController::undo()
{
    emit editAboutToApply();
    dropRemaining();
    if (!m_undo.canUndo()) {
        return;
    }
    ++m_revision;
    notifyState(settle(m_undo.undo(m_document), ChangeOrigin::Undo));
}

void DocumentController::redo()
{
    emit editAboutToApply();
    dropRemaining();
    if (!m_undo.canRedo()) {
        return;
    }
    ++m_revision;
    notifyState(settle(m_undo.redo(m_document), ChangeOrigin::Redo));
}

void DocumentController::commitStroke(core::Stroke stroke)
{
    core::PageObject object = std::move(stroke);
    object.common.layer = core::Layer::Top; // F-510 : les annotations vont sur la couche supérieure
    executeAs(std::make_unique<core::AddObjectCommand>(m_current, std::move(object)), ChangeOrigin::Stroke);
}

int DocumentController::shownObjectCount() const noexcept
{
    return static_cast<int>(m_document.page(m_current).objects.size());
}

// B1 : aucune annonce. editAboutToApply ferait abandonner son geste à l'outil qui soulève (L3-T14), et un
// seul utilisateur ne trace aucun trait pendant qu'il soulève. Hors pile : ni révision, ni étape.
// Contrat de tools::DocumentEditor::lift : un second appel pendant le même soulèvement AJOUTE aux objets
// soulevés (la gomme soulève chaque trait à son premier contact). Les objets déjà soulevés restent hors de la
// page et du cache. Les remettre à chaque appel (L3-C5) rendait au cache les traits que la gomme recoupait ;
// elle les soulevait de nouveau au contact suivant (au banc : 792 traits soulevés pour 24 touchés).
std::vector<core::PageObject> DocumentController::liftObjects(const std::vector<core::ObjectId>& ids)
{
    forgetStaleLift();
    core::Page& page = m_document.page(m_current);
    std::vector<core::ObjectId> fresh; // demandés, dans la page, pas encore soulevés
    for (const core::ObjectId id : ids) {
        if (core::indexOf(page, id) < page.objects.size() &&
            std::find(fresh.begin(), fresh.end(), id) == fresh.end()) {
            fresh.push_back(id);
        }
    }
    if (fresh.empty()) {
        return {};
    }
    const bool wasLifting = !m_lifted.empty();
    if (!wasLifting) {
        // M2 (L3-C8) : la réponse de « Rétablir la page » sur la page entière, rendue pendant tout le geste
        // (déjà en cache dans l'application : Main.qml la relit à chaque révision).
        canRevertPage();
    }
    // Une seule liste aux rangs de la page entière (ceux qu'attend restoreObjects) : les objets déjà soulevés
    // reviennent un instant dans la page, sans changement pour le cache, puis tous sont repris ensemble.
    std::vector<core::ObjectId> all = fresh;
    for (const core::LiftedObject& l : m_lifted) {
        all.push_back(l.object.common.id);
    }
    core::restoreObjects(page, m_lifted);
    m_lifted = core::takeObjects(page, all); // avant settle : un observateur voit isLifting()
    std::vector<core::LiftedObject> added;
    std::vector<core::PageObject> copies;
    for (const core::LiftedObject& l : m_lifted) {
        if (std::find(fresh.begin(), fresh.end(), l.object.common.id) != fresh.end()) {
            added.push_back(l);
            copies.push_back(l.object);
        }
    }
    const core::Changes changes =
        core::liftChanges(m_current, added); // le cache n'a plus que ceux-là à retirer
    m_liftGeneration = m_generation;
    settle(changes, ChangeOrigin::Lift);
    emit
    documentChanged(); // hors pile : la réponse de canRevertPage ne change pas, pas de canRevertPageChanged
    if (!wasLifting) {
        emit liftingChanged();
    }
    return copies;
}

// Les objets reviennent à leurs rangs, la commande s'exécute sur la page restaurée, puis le cache reçoit
// core::changesAfterLift : il n'a plus les objets soulevés, il doit les recevoir insérés. Les changements
// bruts de la commande (ObjectChanged d'un objet qu'il n'a plus) ne vont jamais au cache.
void DocumentController::dropLifted(std::unique_ptr<core::UndoCommand> command)
{
    forgetStaleLift();
    const std::vector<core::LiftedObject> lifted = std::exchange(m_lifted, {});
    core::restoreObjects(m_document.page(m_current), lifted);
    const core::Changes commandChanges = run(std::move(command));
    if (lifted.empty() && commandChanges.empty()) {
        return; // rien soulevé, commande sans effet
    }
    const core::Page none{};
    const core::Page& after = m_current < m_document.pageCount() ? m_document.page(m_current) : none;
    const core::Changes changes = core::changesAfterLift(m_current, lifted, commandChanges, after);
    bool pages = false;
    if (!changes.empty()) {
        pages = settle(changes, ChangeOrigin::Drop);
    }
    if (commandChanges.empty()) {
        // Dépôt sans étape : ni la pile ni la réponse de canRevertPage n'ont bougé (la page est celle d'avant
        // le soulèvement), pas de canRevertPageChanged (M2, L3-C8).
        emit documentChanged();
    } else {
        notifyState(pages);
    }
    if (!lifted.empty()) {
        emit liftingChanged();
    }
}

bool DocumentController::isLifting() const noexcept
{
    return !m_lifted.empty() && m_liftGeneration == m_generation;
}

void DocumentController::dropRemaining()
{
    forgetStaleLift();
    if (isLifting()) {
        dropLifted(nullptr);
    }
}

// replaceDocument (que L3-T12 ne touche pas) ne dépose rien : les copies d'un document remplacé n'ont plus de
// page où revenir.
void DocumentController::forgetStaleLift()
{
    if (!m_lifted.empty() && m_liftGeneration != m_generation) {
        m_lifted.clear();
        emit liftingChanged();
    }
}

// F-225 : le marqueur verse une couleur opaque et sa translucidité à part ; le document garde les deux.
void DocumentController::commitAnnotation(core::Stroke stroke, std::uint8_t translucency)
{
    core::PageObject object = std::move(stroke);
    object.common.layer = core::Layer::Top; // F-510
    object.common.translucency = translucency;
    executeAs(std::make_unique<core::AddObjectCommand>(m_current, std::move(object)), ChangeOrigin::Stroke);
}

void DocumentController::deleteObjects(const std::vector<core::ObjectId>& ids)
{
    emit editAboutToApply();
    dropRemaining();
    execute(std::make_unique<core::RemoveObjectsCommand>(m_current, ids));
}

void DocumentController::clearPage(int kind)
{
    if (kind < static_cast<int>(core::ClearKind::Annotations) ||
        kind > static_cast<int>(core::ClearKind::Page)) {
        return; // genre inconnu : rien à annoncer
    }
    emit editAboutToApply();
    dropRemaining();
    execute(std::make_unique<core::ClearPageCommand>(m_current, static_cast<core::ClearKind>(kind)));
}

// D9 : une étape (ReplacePageCommand), et le document reste modifié : la pile avance, elle n'est jamais
// remontée jusqu'à son état propre.
void DocumentController::revertPage()
{
    emit editAboutToApply();
    dropRemaining();
    const core::Page* saved = savedPage();
    core::Page reference = saved ? *saved : blankLike(m_document.page(m_current));
    execute(std::make_unique<core::ReplacePageCommand>(m_current, std::move(reference)));
}

const core::Page* DocumentController::savedPage() const
{
    if (m_reference && m_referenceGeneration == m_generation && m_current < m_reference->pageCount()) {
        return &m_reference->page(m_current);
    }
    return nullptr; // jamais enregistrée ni ouverte (document neuf, page ajoutée depuis)
}

// Relue par Main.qml à chaque documentChanged et undoChanged : une comparaison d'empreintes par révision et
// par page affichée. M2 (L3-C8) : pendant un soulèvement, la page est incomplète (sa réponse serait fausse)
// et le soulèvement, hors pile, ne change pas la réponse : la dernière calculée, sur la page entière
// (liftObjects la tient à jour avant de soulever), sans aucune empreinte pendant le geste.
bool DocumentController::canRevertPage() const
{
    const auto differs = [this] {
        ++m_revertComparisons;
        const core::Page& shown = m_document.page(m_current);
        const core::Page* saved = savedPage();
        const std::uint64_t reference =
            saved ? core::fingerprint(*saved) : core::fingerprint(blankLike(shown));
        return core::fingerprint(shown) != reference;
    };
    if (isLifting()) {
        return m_revertCheck.answer;
    }
    if (!m_revertCheck.valid || m_revertCheck.revision != m_revision || m_revertCheck.page != m_current) {
        m_revertCheck = RevertCheck{true, m_revision, m_current, differs()};
    }
    return m_revertCheck.answer;
}

int DocumentController::addChangeObserver(ChangeObserver observer)
{
    const int token = ++m_lastObserverToken;
    m_observers.push_back(ObserverEntry{token, std::move(observer)});
    return token;
}

void DocumentController::removeChangeObserver(int token)
{
    std::erase_if(m_observers, [token](const ObserverEntry& entry) { return entry.token == token; });
}

void DocumentController::notifyObservers(const core::Changes& changes, ChangeOrigin origin)
{
    if (m_observers.empty()) {
        return;
    }
    // Copie : un observateur peut en ajouter ou en retirer un pendant l'appel. Un retiré n'est plus appelé,
    // même plus loin dans cette diffusion ; un ajouté attend la suivante.
    const std::vector<ObserverEntry> observers = m_observers;
    for (const ObserverEntry& entry : observers) {
        const bool kept = std::any_of(m_observers.begin(), m_observers.end(),
                                      [&entry](const ObserverEntry& o) { return o.token == entry.token; });
        if (kept && entry.call) {
            entry.call(changes, origin);
        }
    }
}

bool DocumentController::settle(const core::Changes& changes, ChangeOrigin origin)
{
    const bool pages = std::any_of(changes.begin(), changes.end(), [](const core::Change& c) {
        return c.kind == core::Change::Kind::PageInserted || c.kind == core::Change::Kind::PageRemoved;
    });
    NoCache none;
    core::PageCache& cache =
        m_fixed ? static_cast<core::PageCache&>(*m_fixed) : static_cast<core::PageCache&>(none);
    const std::optional<std::size_t> shown = core::applyToCache(changes, m_current, cache);
    // Observateur unique (D-S7) : exactement ces changements, après la page fixe et avant le recalage
    // ci-dessous (currentIndex() vaut encore l'indice passé à applyToCache).
    notifyObservers(changes, origin);
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
    emit canRevertPageChanged();
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
    ++m_revision;
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
    // L'outil n'a pas déposé à pageAboutToChange : les copies de l'ancien document sont oubliées.
    forgetStaleLift();
}

void DocumentController::nextPage()
{
    emit pageAboutToChange(); // le trait en cours rejoint la page affichée avant le déplacement
    // M2 : l'outil a abandonné son geste à l'annonce ; ce qui reste soulevé revient sans étape.
    dropRemaining();
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
    settle(changes, ChangeOrigin::Command); // ajout après la page affichée : rien pour son cache
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
    dropRemaining(); // M2
    show(target);
}

void DocumentController::loadGeneratedFirstPage()
{
    // m2 (L3-C8) : comme les autres éditions, l'annonce d'abord (l'outil abandonne son geste, la vue termine
    // son trait), puis ce qui reste soulevé revient sans étape : son dépôt ne viserait plus la page
    // remplacée.
    emit editAboutToApply();
    dropRemaining();
    const QSizeF shownSize = pageSize();
    const bool modified = isModified();
    core::Page& first = m_document.page(0);
    first = core::generatePage(core::kDefaultSeed);
    ++m_revision;
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

void DocumentController::refuseWhileBusy(const QString& message,
                                         void (DocumentController::*finished)(bool, const QString&))
{
    // Refus visible (revue de L2-C9) : dernière erreur et bandeau, comme tout échec d'ouverture ou
    // d'enregistrement.
    QMetaObject::invokeMethod(
        this,
        [this, message, finished] {
            setLastError(message);
            emit(this->*finished)(false, message);
        },
        Qt::QueuedConnection);
}

void DocumentController::openUrl(const QUrl& url)
{
    if (m_busy) {
        refuseWhileBusy(tr("Une opération est déjà en cours : ouverture refusée."),
                        &DocumentController::loadFinished);
        return;
    }
    // Relevée à l'appel : un trait (même annulé), « Nouveau » ou « Fermer » pendant la lecture n'est jamais
    // écrasé en silence. Une révision, pas une empreinte : rien à parcourir sur le fil principal (L2-C9).
    // Relevée AVANT l'annonce : un trait que la vue termine à l'annonce fait refuser l'ouverture, comme tout
    // changement pendant la lecture (rien n'est perdu).
    const std::uint64_t revision = m_revision;
    emit editAboutToApply(); // l'outil abandonne son geste, la vue termine son trait
    dropRemaining();         // M2 : Ouvrir n'attend pas un dépôt
    setBusy(true);
    std::shared_ptr<IoRelay> relay = m_relay;
    QThreadPool::globalInstance()->start([relay, url, revision] {
        auto outcome = std::make_shared<LoadOutcome>(loadDocument(url));
        // Référence de Rétablir la page (D9) : le document tel qu'ouvert, copié ici, hors du fil principal.
        std::shared_ptr<const core::Document> reference;
        if (outcome->ok) {
            reference = std::make_shared<const core::Document>(outcome->document);
        }
        relay->post([relay, url, outcome, revision, reference] {
            DocumentController& self = *relay->target; // livré au contrôleur vivant (IoRelay)
            self.setBusy(false);
            self.setLastIo(outcome->offMainThread);
            // Le trait en cours (stylet encore posé) est versé AVANT le contrôle : versé seulement par
            // replaceDocument, il rejoindrait l'ancien document, aussitôt remplacé (L2-C9). S'il change le
            // document, l'ouverture est refusée comme pour tout changement pendant la lecture.
            if (outcome->ok) {
                emit self.pageAboutToChange();
                // M2 : soulevé pendant la lecture, l'outil a abandonné à l'annonce ; le reste revient.
                self.dropRemaining();
            }
            if (outcome->ok && revision != self.m_revision) {
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
            // replaceDocument avance la génération d'une unité : la référence vaut dès ses signaux (Main.qml
            // relit canRevertPage à documentChanged), puis elle est recalée sur la génération rendue.
            self.m_reference = reference;
            self.m_referenceGeneration = self.m_generation + 1;
            // Un .flipchart devient un document sans fichier : « Enregistrer » proposera <nom>.actcraft.
            self.replaceDocument(std::move(outcome->document), outcome->native ? url : QUrl(),
                                 outcome->title);
            self.m_referenceGeneration = self.m_generation;
            self.m_revertCheck.valid = false;
            self.setWarnings(outcome->warnings); // après replaceDocument, qui les efface
            // Empreinte calculée par la lecture, hors du fil principal : celle du document affiché.
            qCInfo(lcDocument, "ACTCRAFT OUVERT %s", qPrintable(fingerprintHex(outcome->fingerprint)));
            emit self.loadFinished(true, QString());
        });
    });
}

void DocumentController::saveToUrl(const QUrl& url)
{
    if (m_busy) {
        refuseWhileBusy(tr("Une opération est déjà en cours : enregistrement refusé."),
                        &DocumentController::saveFinished);
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
    // La vue termine son trait, qui entre dans le fichier ; l'outil abandonne son geste. Puis M2 :
    // Enregistrer n'écrit jamais un document dont des objets sont soulevés.
    emit editAboutToApply();
    dropRemaining();
    setBusy(true);
    // Copie prise ici, sur le fil principal (ressources partagées implicitement) : le fil secondaire ne lit
    // jamais le document affiché, que l'utilisateur continue de modifier.
    auto snapshot = std::make_shared<const core::Document>(m_document);
    const std::uint64_t generation = m_generation;
    const std::uint64_t revision = m_revision;
    std::shared_ptr<IoRelay> relay = m_relay;
    QThreadPool::globalInstance()->start([relay, url = target, snapshot, generation, revision] {
        const SaveOutcome outcome = saveDocument(*snapshot, url);
        relay->post([relay, url, outcome, generation, revision, snapshot] {
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
            // l'écran est le contenu du fichier : propre si la révision n'a pas bougé pendant l'écriture ;
            // sinon modifié jusqu'au prochain enregistrement, car l'ancien repère propre de la pile désigne
            // un contenu que le fichier n'a plus (trait annulé pendant l'écriture). Un trait puis son
            // annulation pendant l'écriture laissent aussi « modifié » : prudent, sans perte (L2-C9 : aucune
            // empreinte du document sur le fil principal).
            if (generation == self.m_generation) {
                // Référence de Rétablir la page (D9) : le contenu écrit, partagé avec la copie du fil.
                self.m_reference = snapshot;
                self.m_referenceGeneration = generation;
                self.m_revertCheck.valid = false;
                if (self.m_revision == revision) {
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
                // La référence de « Rétablir la page » vient de changer : canRevertPage est renotifié
                // (Main.qml relit l'entrée), mais pas documentChanged : le document affiché n'a pas changé.
                emit self.canRevertPageChanged();
            }
            qCInfo(lcDocument, "ACTCRAFT ENREGISTRE %s", qPrintable(fingerprintHex(outcome.fingerprint)));
            emit self.saveFinished(true, QString());
        });
    });
}

} // namespace actcraft::ui
