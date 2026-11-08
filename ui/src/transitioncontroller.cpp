#include "transitioncontroller.h"

#include "presentationdocument.h"

#include <QQuickItem>
#include <QSettings>
#include <QVariantMap>

#include <algorithm>
#include <utility>

namespace actcraft::ui {

namespace {

constexpr auto kGroup = "presentation/transition";
// Au-delà, un instantané sans image rendue est abandonné. Large : une image du rendu logiciel en débogage,
// sous charge, dépasse parfois une demi-seconde ; la fenêtre ne montre alors rien de toute façon.
constexpr int kCaptureTimeoutMs = 2000;

using Origin = QQuickItem::TransformOrigin;

bool known(const QString& id)
{
    const auto& list = features::transitionCatalogue();
    return std::any_of(list.begin(), list.end(),
                       [&id](const features::TransitionEffect& e) { return e.id == id; });
}

struct Piece
{
    double x = 0;
    double y = 0;
    double width = 1;
    double height = 1;
    double dx = 0;
    double dy = 0;
    double scale = 1;
    double opacity = 1;
    Origin origin = Origin::Center;
    double span = 1;
};

QVariantMap toMap(const Piece& p)
{
    return {{QStringLiteral("x"), p.x},
            {QStringLiteral("y"), p.y},
            {QStringLiteral("width"), p.width},
            {QStringLiteral("height"), p.height},
            {QStringLiteral("dx"), p.dx},
            {QStringLiteral("dy"), p.dy},
            {QStringLiteral("scale"), p.scale},
            {QStringLiteral("opacity"), p.opacity},
            {QStringLiteral("origin"), static_cast<int>(p.origin)},
            {QStringLiteral("span"), p.span}};
}

// Grille de cols × rows morceaux ; `shape` pose la cible de chacun (colonne, ligne, indice).
template <typename Shape> std::vector<Piece> grid(int cols, int rows, Shape shape)
{
    std::vector<Piece> pieces;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            Piece p;
            p.x = static_cast<double>(c) / cols;
            p.y = static_cast<double>(r) / rows;
            p.width = 1.0 / cols;
            p.height = 1.0 / rows;
            shape(p, c, r, r * cols + c);
            pieces.push_back(p);
        }
    }
    return pieces;
}

// Un mouvement par identifiant, déduit de son nom (rapport L7-T0 : à préciser à l'écran à la clôture) ;
// opacité, translation et échelle seulement, aucun nuanceur.
std::vector<Piece> piecesFor(const QString& id)
{
    if (id == QLatin1String("dissolve")) { // fondu : l'ancienne page devient transparente
        Piece p;
        p.opacity = 0;
        return {p};
    }
    if (id == QLatin1String("corner-peel")) { // se replie vers le coin haut gauche
        Piece p;
        p.scale = 0;
        p.origin = Origin::TopLeft;
        return {p};
    }
    if (id == QLatin1String("box-turn")) { // part à gauche en rétrécissant
        Piece p;
        p.dx = -1;
        p.scale = 0.6;
        p.opacity = 0.2;
        p.origin = Origin::Right;
        return {p};
    }
    if (id == QLatin1String("card-flip")) { // se réduit au centre
        Piece p;
        p.scale = 0;
        p.opacity = 0.4;
        return {p};
    }
    if (id == QLatin1String("tiles-spin")) { // carreaux qui disparaissent en diagonale
        return grid(4, 3, [](Piece& p, int c, int r, int) {
            p.scale = 0;
            p.opacity = 0;
            p.span = 0.4 + 0.6 * (c + r) / 5.0;
        });
    }
    if (id == QLatin1String("fold-half")) { // les deux moitiés se replient sur la ligne du milieu
        return grid(1, 2, [](Piece& p, int, int r, int) {
            p.scale = 0;
            p.origin = r == 0 ? Origin::Bottom : Origin::Top;
        });
    }
    if (id == QLatin1String("slide-right")) { // glisse vers la droite
        Piece p;
        p.dx = 1;
        return {p};
    }
    if (id == QLatin1String("strips-drop")) { // bandes verticales qui tombent l'une après l'autre
        return grid(6, 1, [](Piece& p, int c, int, int) {
            p.dy = 1;
            p.span = 0.5 + 0.5 * c / 5.0;
        });
    }
    if (id == QLatin1String("corner-swing")) { // tombe vers le coin bas gauche
        Piece p;
        p.dy = 1;
        p.scale = 0.3;
        p.origin = Origin::BottomLeft;
        return {p};
    }
    if (id == QLatin1String("spiral-out")) { // morceaux chassés du centre, l'un après l'autre
        static constexpr int order[9] = {1, 2, 3, 8, 0, 4, 7, 6, 5};
        return grid(3, 3, [](Piece& p, int c, int r, int index) {
            p.dx = (c - 1) * 1.2;
            p.dy = (r - 1) * 1.2;
            p.scale = 0.2;
            p.opacity = 0;
            p.span = 0.5 + 0.5 * order[index] / 8.0;
        });
    }
    if (id == QLatin1String("turn-reveal")) { // se referme sur son bord gauche
        Piece p;
        p.scale = 0;
        p.origin = Origin::Left;
        return {p};
    }
    if (id == QLatin1String("drapes")) { // deux tentures qui s'ouvrent
        return grid(2, 1, [](Piece& p, int c, int, int) { p.dx = c == 0 ? -0.5 : 0.5; });
    }
    if (id == QLatin1String("star-iris")) { // quatre quarts chassés vers les coins
        return grid(2, 2, [](Piece& p, int c, int r, int) {
            p.dx = c == 0 ? -0.5 : 0.5;
            p.dy = r == 0 ? -0.5 : 0.5;
        });
    }
    if (id == QLatin1String("shards")) { // éclats projetés
        return grid(4, 3, [](Piece& p, int, int, int index) {
            p.dx = ((index * 7) % 5 - 2) * 0.4;
            p.dy = ((index * 3) % 5 - 2) * 0.4 + 0.6;
            p.scale = 0.5;
            p.opacity = 0;
            p.span = 0.6 + 0.4 * ((index * 5) % 12) / 11.0;
        });
    }
    if (id == QLatin1String("sweep")) { // balayage de gauche à droite
        return grid(8, 1, [](Piece& p, int c, int, int) {
            p.opacity = 0;
            p.span = 0.25 + 0.75 * c / 7.0;
        });
    }
    if (id == QLatin1String("funnel")) { // aspirée vers le bas
        Piece p;
        p.scale = 0;
        p.opacity = 0.5;
        p.origin = Origin::Bottom;
        return {p};
    }
    return {};
}

} // namespace

TransitionController::TransitionController(QObject* parent)
    : QObject(parent), m_random(QRandomGenerator::global()->generate())
{
    m_captureTimer.setSingleShot(true);
    m_captureTimer.setInterval(kCaptureTimeoutMs);
    connect(&m_captureTimer, &QTimer::timeout, this, [this] {
        if (m_phase == Capturing) {
            settle(); // aucune image rendue (fenêtre cachée) : la page change sans effet
        }
    });
    QSettings settings;
    settings.beginGroup(QLatin1String(kGroup));
    const QString global = settings.value(QStringLiteral("global"), m_settings.global).toString();
    if (known(global)) {
        m_settings.global = global;
    }
    m_settings.durationTenths = features::clampDurationTenths(
        settings.value(QStringLiteral("duration"), m_settings.durationTenths).toInt());
    settings.endGroup();
}

DocumentController* TransitionController::documentController() const noexcept
{
    return m_document;
}

void TransitionController::setDocumentController(DocumentController* controller)
{
    if (m_document == controller) {
        return;
    }
    m_document = controller;
    emit documentControllerChanged();
}

QString TransitionController::globalEffect() const
{
    return m_settings.global;
}

void TransitionController::setGlobalEffect(const QString& id)
{
    if (!known(id) || id == m_settings.global) {
        return;
    }
    m_settings.global = id;
    save();
    emit settingsChanged();
}

int TransitionController::durationTenths() const noexcept
{
    return m_settings.durationTenths;
}

void TransitionController::setDurationTenths(int tenths)
{
    const int clamped = features::clampDurationTenths(tenths);
    if (clamped == m_settings.durationTenths) {
        return;
    }
    m_settings.durationTenths = clamped;
    save();
    emit settingsChanged();
}

bool TransitionController::blocked() const noexcept
{
    return m_blocked;
}

void TransitionController::setBlocked(bool blocked)
{
    if (m_blocked == blocked) {
        return;
    }
    m_blocked = blocked;
    emit blockedChanged();
}

bool TransitionController::desktopAnnotation() const noexcept
{
    return m_desktopAnnotation;
}

void TransitionController::setDesktopAnnotation(bool annotating)
{
    if (m_desktopAnnotation == annotating) {
        return;
    }
    m_desktopAnnotation = annotating;
    emit desktopAnnotationChanged();
}

int TransitionController::phase() const noexcept
{
    return m_phase;
}

bool TransitionController::playing() const noexcept
{
    return m_phase != Idle;
}

QString TransitionController::effect() const
{
    return m_effect;
}

QString TransitionController::played() const
{
    return m_played;
}

QVariantList TransitionController::catalogue() const
{
    QVariantList list;
    for (const features::TransitionEffect& e : features::transitionCatalogue()) {
        list.append(QVariantMap{{QStringLiteral("id"), e.id}, {QStringLiteral("label"), e.label}});
    }
    return list;
}

void TransitionController::navigate(int kind)
{
    settle();
    if (!m_document) {
        return;
    }
    const auto navigation = kind == 1 ? features::NavigationKind::Previous : features::NavigationKind::Next;
    // L'effet de la page QUITTÉE, lu dans le document avant le changement.
    const DocumentInsertions insertions(*m_document);
    const core::Page* leaving = insertions.shownPage();
    const std::string pageEffect = leaving ? leaving->presentation.transition : std::string();
    features::TransitionBlockers blockers;
    blockers.curtainOrSpotlight = m_blocked;
    blockers.desktopAnnotation = m_desktopAnnotation;
    const QString id = features::pickEffect(
        features::effectForLeaving(m_settings, pageEffect, navigation, blockers), m_random);
    if (id == QLatin1String("none") || piecesFor(id).empty()) {
        turn(navigation); // sans effet : le corps d'origine de la commande, aussitôt
        return;
    }
    m_pending = navigation;
    m_effect = id;
    setPhase(Capturing);
    emit captureRequested(id);
}

void TransitionController::frozen()
{
    if (m_phase != Capturing) {
        return;
    }
    if (!turn(m_pending)) { // rien à quitter (première page, changement refusé) : pas d'effet
        m_effect.clear();
        setPhase(Idle);
        emit stopRequested();
        return;
    }
    m_played = m_effect;
    emit playedChanged();
    setPhase(Animating);
    emit playRequested(m_effect, m_settings.durationTenths * 100);
}

void TransitionController::finished()
{
    if (m_phase == Animating) {
        settle();
    }
}

void TransitionController::setPageEffect(const QString& id)
{
    if (!m_document || (!id.isEmpty() && !known(id))) {
        return;
    }
    DocumentInsertions insertions(*m_document);
    const core::Page* page = insertions.shownPage();
    if (!page) {
        return;
    }
    core::PagePresentation value = page->presentation;
    value.transition = id.toStdString();
    insertions.setPagePresentation(std::move(value));
}

void TransitionController::seedRandom(quint32 seed)
{
    m_random.seed(seed);
}

QVariantList TransitionController::motion(const QString& effect) const
{
    QVariantList list;
    for (const Piece& p : piecesFor(effect)) {
        list.append(toMap(p));
    }
    return list;
}

bool TransitionController::turn(features::NavigationKind kind)
{
    if (!m_document) {
        return false;
    }
    const int before = m_document->currentIndex();
    if (kind == features::NavigationKind::Previous) {
        m_document->pageBack();
    } else {
        m_document->nextPage();
    }
    return m_document->currentIndex() != before;
}

void TransitionController::settle()
{
    if (m_phase == Idle) {
        return;
    }
    if (m_phase == Capturing) {
        turn(m_pending); // la page demandée change d'abord : aucune n'est perdue
    }
    m_effect.clear();
    setPhase(Idle);
    emit stopRequested();
}

void TransitionController::setPhase(Phase phase)
{
    if (m_phase == phase) {
        return;
    }
    m_phase = phase;
    if (m_phase == Capturing) {
        m_captureTimer.start(); // repli : sans image rendue, la page change sans effet
    } else {
        m_captureTimer.stop();
    }
    emit phaseChanged();
}

void TransitionController::save() const
{
    QSettings settings;
    settings.beginGroup(QLatin1String(kGroup));
    settings.setValue(QStringLiteral("global"), m_settings.global);
    settings.setValue(QStringLiteral("duration"), m_settings.durationTenths);
    settings.endGroup();
}

} // namespace actcraft::ui
