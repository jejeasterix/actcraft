#include "pagetoolscontroller.h"

#include "presentationdocument.h"

#include <actcraft/core/pagecache.h>
#include <actcraft/features/pagetools.h>

#include <QRectF>
#include <QSizeF>

#include <array>
#include <cmath>
#include <functional>
#include <optional>
#include <utility>

namespace actcraft::ui {

namespace {

template <typename Value> struct Named
{
    const char* name;
    Value value;
};

constexpr std::array<Named<core::PageToolsMode>, 4> kToolsModes{
    {{"keep", core::PageToolsMode::Keep},
     {"curtain", core::PageToolsMode::Curtain},
     {"spotlight", core::PageToolsMode::Spotlight},
     {"off", core::PageToolsMode::Off}}};
constexpr std::array<Named<core::CurtainMode>, 6> kCurtainModes{{{"off", core::CurtainMode::Off},
                                                                 {"top", core::CurtainMode::Top},
                                                                 {"bottom", core::CurtainMode::Bottom},
                                                                 {"left", core::CurtainMode::Left},
                                                                 {"right", core::CurtainMode::Right},
                                                                 {"full", core::CurtainMode::Full}}};
constexpr std::array<Named<core::SpotlightShape>, 5> kSpotShapes{
    {{"off", core::SpotlightShape::Off},
     {"circle", core::SpotlightShape::Circle},
     {"square", core::SpotlightShape::Square},
     {"solid-circle", core::SpotlightShape::SolidCircle},
     {"solid-square", core::SpotlightShape::SolidSquare}}};

template <typename Value, std::size_t N>
bool lookUp(const std::array<Named<Value>, N>& table, const QString& name, Value& out)
{
    for (const Named<Value>& entry : table) {
        if (name == QLatin1StringView(entry.name)) {
            out = entry.value;
            return true;
        }
    }
    return false;
}

// Forme voulue par la page, par la seule bascule publique du spot : la forme active le ferme, une autre la
// remplace.
void showShape(SpotlightController& spotlight, features::Spotlight::Shape shape)
{
    const int wanted = static_cast<int>(SpotlightController::toCore(shape));
    const int current = spotlight.shape();
    if (wanted == current) {
        return;
    }
    spotlight.toggle(wanted == 0 ? current : wanted);
}

// Cache qui ne garde rien : core::applyToCache ne sert ici qu'à suivre l'indice d'une page.
class IndexOnlyCache final : public core::PageCache
{
public:
    void insert(core::ObjectId) override {}
    void remove(core::ObjectId, const core::RectF&) override {}
    void invalidate(core::ObjectId, const core::CacheArea&) override {}
};

// Réglages de la page affichée, modifiés par `edit` puis écrits en une étape
// (DocumentInsertions::setPagePresentation). Faux sans document, sans page, ou si `edit` rend faux (rien à
// écrire).
bool writeShownPresentation(DocumentController* document,
                            const std::function<bool(core::PagePresentation&)>& edit)
{
    if (!document) {
        return false;
    }
    DocumentInsertions insertions(*document);
    const core::Page* page = insertions.shownPage();
    if (!page) {
        return false;
    }
    core::PagePresentation value = page->presentation;
    if (!edit(value)) {
        return false;
    }
    insertions.setPagePresentation(std::move(value));
    return true;
}

} // namespace

PageToolsController::PageToolsController(QObject* parent) : QObject(parent) {}

PageToolsController::~PageToolsController()
{
    if (m_document && m_observer > 0) {
        m_document->removeChangeObserver(m_observer);
    }
}

DocumentController* PageToolsController::documentController() const noexcept
{
    return m_document;
}

void PageToolsController::setDocumentController(DocumentController* controller)
{
    if (m_document == controller) {
        return;
    }
    disconnect(m_pagesConnection);
    disconnect(m_replacedConnection);
    if (m_document && m_observer > 0) {
        m_document->removeChangeObserver(m_observer);
    }
    m_observer = 0;
    m_document = controller;
    m_shownIndex = controller ? controller->currentIndex() : -1;
    if (controller) {
        m_pagesConnection = connect(controller, &DocumentController::pagesChanged, this,
                                    &PageToolsController::onPagesChanged);
        m_replacedConnection = connect(controller, &DocumentController::documentReplaced, this,
                                       &PageToolsController::applyShownPage);
        m_observer = controller->addChangeObserver(
            [this](const core::Changes& applied, DocumentController::ChangeOrigin) {
                followShownPage(applied);
            });
    }
    emit documentControllerChanged();
}

QObject* PageToolsController::curtain() const noexcept
{
    return m_curtain;
}

void PageToolsController::setCurtain(QObject* curtain)
{
    auto* controller = qobject_cast<CurtainController*>(curtain);
    if (m_curtain == controller) {
        return;
    }
    m_curtain = controller;
    emit curtainChanged();
}

QObject* PageToolsController::spotlight() const noexcept
{
    return m_spotlight;
}

void PageToolsController::setSpotlight(QObject* spotlight)
{
    auto* controller = qobject_cast<SpotlightController*>(spotlight);
    if (m_spotlight == controller) {
        return;
    }
    disconnect(m_spotlightConnection);
    m_spotlight = controller;
    m_spotlightWasActive = controller && controller->active();
    m_spotlightWasDragging = controller && controller->dragging();
    if (controller) {
        m_spotlightConnection = connect(controller, &SpotlightController::changed, this,
                                        &PageToolsController::onSpotlightChanged);
    }
    emit spotlightChanged();
}

void PageToolsController::onPagesChanged()
{
    if (m_document && m_document->currentIndex() != m_shownIndex) {
        applyShownPage();
    }
}

// Page dont les outils sont appliqués, suivie à travers les ajouts et retraits de pages (Annuler et Rétablir
// compris) : un ajout ou un retrait placé avant elle la déplace sans la changer ; retirée, la page qui la
// remplace à l'écran reçoit ses outils au pagesChanged qui suit, même à indice égal.
void PageToolsController::followShownPage(const core::Changes& applied)
{
    if (m_shownIndex < 0) {
        return;
    }
    IndexOnlyCache ignored;
    const std::optional<std::size_t> moved =
        core::applyToCache(applied, static_cast<std::size_t>(m_shownIndex), ignored);
    m_shownIndex = moved ? static_cast<int>(*moved) : -1;
}

void PageToolsController::onSpotlightChanged()
{
    if (!m_spotlight) {
        return;
    }
    const bool active = m_spotlight->active();
    if (active && !m_spotlightWasActive && m_curtain) {
        m_curtain->setActive(false); // ouvrir le spot ferme le rideau
    }
    m_spotlightWasActive = active;
    // Cadre du spot déplacé ou redimensionné sur une page « spot » : écrit dans la page au relâché (une
    // étape).
    const bool dragging = m_spotlight->dragging();
    const QRectF frame = m_spotlight->frame();
    if (dragging && !m_spotlightWasDragging) {
        m_frameAtPress = frame;
    } else if (!dragging && m_spotlightWasDragging && frame != m_frameAtPress) {
        const core::RectF value{frame.x(), frame.y(), frame.width(), frame.height()};
        writeShownPresentation(m_document, [value](core::PagePresentation& p) {
            if (p.tools != core::PageToolsMode::Spotlight) {
                return false;
            }
            p.spotlightFrame = value;
            return true;
        });
    }
    m_spotlightWasDragging = dragging;
}

void PageToolsController::applyShownPage()
{
    if (!m_document) {
        return;
    }
    const int index = m_document->currentIndex();
    m_shownIndex = index;
    if (!m_curtain || !m_spotlight || index < 0 ||
        static_cast<std::size_t>(index) >= m_document->document().pageCount()) {
        return;
    }
    const features::PageToolsSettings page = features::toPageToolsSettings(
        m_document->document().page(static_cast<std::size_t>(index)).presentation);
    const features::PresentationState now{
        m_curtain->active(),
        SpotlightController::fromCore(static_cast<core::SpotlightShape>(m_spotlight->shape()))};
    const features::PageToolsResult next = features::resolvePageTools(now, page);
    if (next.resetCurtain) {
        m_curtain->reset(static_cast<int>(next.curtainStart), next.curtainDistance);
    }
    m_curtain->setActive(next.curtain);
    showShape(*m_spotlight, next.spotlight);
}

bool PageToolsController::setToolsMode(const QString& mode)
{
    core::PageToolsMode value{};
    return lookUp(kToolsModes, mode, value) &&
           writeShownPresentation(m_document, [value](core::PagePresentation& p) {
               p.tools = value;
               return true;
           });
}

bool PageToolsController::setCurtainMode(const QString& mode)
{
    core::CurtainMode value{};
    return lookUp(kCurtainModes, mode, value) &&
           writeShownPresentation(m_document, [value](core::PagePresentation& p) {
               p.curtain = value;
               return true;
           });
}

bool PageToolsController::setCurtainDistance(double distance)
{
    return std::isfinite(distance) && distance >= 0 &&
           writeShownPresentation(m_document, [distance](core::PagePresentation& p) {
               p.curtainDistance = distance;
               return true;
           });
}

bool PageToolsController::setSpotShape(const QString& mode)
{
    core::SpotlightShape value{};
    return lookUp(kSpotShapes, mode, value) &&
           writeShownPresentation(m_document, [value](core::PagePresentation& p) {
               p.spotlight = value;
               return true;
           });
}

CurtainPageMemory::CurtainPageMemory(QObject* parent) : QObject(parent) {}

DocumentController* CurtainPageMemory::documentController() const noexcept
{
    return m_document;
}

void CurtainPageMemory::setDocumentController(DocumentController* controller)
{
    if (m_document == controller) {
        return;
    }
    m_document = controller;
    emit documentControllerChanged();
}

QObject* CurtainPageMemory::curtain() const noexcept
{
    return m_curtain;
}

void CurtainPageMemory::setCurtain(QObject* curtain)
{
    auto* controller = qobject_cast<CurtainController*>(curtain);
    if (m_curtain == controller) {
        return;
    }
    m_curtain = controller;
    emit curtainChanged();
}

bool CurtainPageMemory::remember()
{
    if (!m_curtain || !m_curtain->active()) {
        return false;
    }
    // Côté le plus découvert du store, et sa distance ; store fermé : rideau complet.
    const QRectF covered = m_curtain->covered();
    const QSizeF area = m_curtain->area();
    const std::array<std::pair<core::CurtainMode, double>, 4> openings{
        {{core::CurtainMode::Top, covered.top()},
         {core::CurtainMode::Bottom, area.height() - covered.bottom()},
         {core::CurtainMode::Left, covered.left()},
         {core::CurtainMode::Right, area.width() - covered.right()}}};
    std::pair<core::CurtainMode, double> side{core::CurtainMode::Full, 0.0};
    for (const auto& candidate : openings) {
        if (candidate.second > side.second + 0.5) {
            side = candidate;
        }
    }
    const double distance = side.first == core::CurtainMode::Full ? 0.0 : side.second;
    return writeShownPresentation(m_document, [side, distance](core::PagePresentation& p) {
        p.tools = core::PageToolsMode::Curtain;
        p.curtain = side.first;
        p.curtainDistance = distance;
        return true;
    });
}

} // namespace actcraft::ui
