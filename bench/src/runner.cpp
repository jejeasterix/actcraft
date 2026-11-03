#include <actcraft/bench/contentcheck.h>
#include <actcraft/bench/memory.h>
#include <actcraft/bench/runner.h>
#include <actcraft/bench/scenarios.h>
#include <actcraft/bench/spinningload.h>
#include <actcraft/bench/verdict.h>
#include <actcraft/core/clock.h>
#include <actcraft/core/generator.h>
#include <actcraft/core/hittest.h>
#include <actcraft/core/pagesettings.h>
#include <actcraft/core/pagezones.h>
#include <actcraft/export/pdfexport.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documenttabs.h>
#include <actcraft/ui/objectediting.h>
#include <actcraft/ui/pagethumbnails.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>
#include <actcraft/ui/viewtransform.h>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QImage>
#include <QMetaObject>
#include <QQuickWindow>
#include <QScreen>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <QVariant>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <thread>
#include <utility>
#include <vector>

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// clang-format off
#include <windows.h>
#include <dwmapi.h>
// clang-format on
#endif

namespace actcraft::bench {

namespace {

// Période de composition de DWM (écran principal), 0 si indisponible ou hors Windows.
std::int64_t readDwmRefreshPeriodNs()
{
#if defined(Q_OS_WIN)
    DWM_TIMING_INFO info{};
    info.cbSize = sizeof(info);
    LARGE_INTEGER frequency{};
    if (FAILED(DwmGetCompositionTimingInfo(nullptr, &info)) || info.qpcRefreshPeriod == 0 ||
        !QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0) {
        return 0;
    }
    return static_cast<std::int64_t>(static_cast<double>(info.qpcRefreshPeriod) * 1e9 /
                                     static_cast<double>(frequency.QuadPart));
#else
    return 0;
#endif
}

// Hors écran (QT_QPA_PLATFORM=offscreen, scripts/bench.ps1 -HorsEcran), « plein écran » prend l'écran fictif
// de la plateforme : relevé 800 de large, la vue réduite par la barre et la palette ne contient plus les
// témoins à 100 % (witness-image « hors-image », content-check-failed). Taille fixe, celle de la fumée, des
// captures et de benchTargetsAreVisibleInTheShell ; une taille LxH explicite reste prioritaire.
constexpr QSize kOffscreenWindowSize(1280, 800);
constexpr int kPageShownTimeoutMs = 30000; // images chargées et premier cache dessiné

bool offscreenPlatform()
{
    return QGuiApplication::platformName() == QLatin1String("offscreen");
}

// Centre des points lus par les contrôles de contenu de runDemo : centres des témoins de forme et d'image,
// sonde du témoin de trait (core/src/generator.cpp), encre de mi-trait. Ces points, et non l'enveloppe des
// témoins (y 16 à 732), tiennent dans une vue de 680 de haut (fenêtre de 1280 × 720).
QPointF demoTargetsCenter()
{
    const core::Witnesses& w = core::witnesses();
    const QPointF ink = demoInkProbePoint();
    const double xs[] = {w.shapeRect.center().x, w.imageRect.center().x, w.strokeProbe.x, ink.x()};
    const double ys[] = {w.shapeRect.center().y, w.imageRect.center().y, w.strokeProbe.y, ink.y()};
    const auto [left, right] = std::minmax_element(std::begin(xs), std::end(xs));
    const auto [top, bottom] = std::minmax_element(std::begin(ys), std::end(ys));
    return QPointF((*left + *right) / 2, (*top + *bottom) / 2);
}

// Commande nommée jouée par la fenêtre (runCommand de Main.qml) : le chemin des boutons, des menus et des
// raccourcis, sans événement posté (pas de piège de raccourci sous macOS, L1-C4). Vrai si elle a été
// exécutée.
bool runWindowCommand(QQuickWindow* window, const QString& id, const QVariant& argument = QVariant())
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(window, "runCommand", Q_RETURN_ARG(QVariant, result),
                                                   Q_ARG(QVariant, QVariant(id)), Q_ARG(QVariant, argument));
    return invoked && result.toBool();
}

// L5-T19 : gestes d'édition du scénario « edition », passés par ObjectEditing et ToolHost (le chemin réel).
struct EditionGear
{
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
    render::FixedPageItem* fixed = nullptr;
    ui::DocumentController* document = nullptr;
    ui::ToolHost* tools = nullptr;
    ui::ObjectEditing* editing = nullptr;
    std::vector<core::ObjectId> pasted; // objets collés par « edition.paste », groupés ensuite
};

bool waitUntil(const std::function<bool()>& condition, int timeoutMs)
{
    QElapsedTimer clock;
    clock.start();
    QTimer wake;
    wake.start(10);
    while (!condition()) {
        if (clock.elapsed() > timeoutMs) {
            return false;
        }
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }
    return true;
}

std::vector<core::ObjectId> pageIds(const core::Page& page)
{
    std::vector<core::ObjectId> ids;
    for (const core::PageObject& object : page.objects) {
        ids.push_back(object.common.id);
    }
    return ids;
}

// L5-T31 : onglets du scénario « onglets » (bascules mesurées par la commande `tabs.switch`).
struct TabsGear
{
    ui::DocumentTabs* tabs = nullptr;
    ui::PageView* view = nullptr;
    render::FixedPageItem* fixed = nullptr;
};

// Affiche l'onglet `index` et mesure le temps jusqu'à un rendu terminé : un redessin du cache de plus
// (PageView::cacheRefreshCount, D21), page prête, plus rien en attente. Faux si l'onglet n'est pas affiché ou
// si le rendu n'arrive pas.
bool runTabSwitch(TabsGear& g, int index, std::vector<double>& out)
{
    const int refreshesBefore = g.view->cacheRefreshCount();
    QElapsedTimer clock;
    clock.start();
    g.tabs->activate(index);
    if (g.tabs->currentIndex() != index) {
        return false;
    }
    const bool shown = waitUntil(
        [&g, refreshesBefore] {
            return g.view->cacheRefreshCount() > refreshesBefore && g.fixed->isReady() &&
                   g.view->cacheReady() && g.fixed->pendingCount() == 0;
        },
        15000);
    out.push_back(static_cast<double>(clock.nsecsElapsed()) / 1e6);
    return shown;
}

// L5-T31 : exports PDF du document synthétique sur leur fil dédié (exporting::exportPdf), relancés l'un après
// l'autre tant que le scénario « export-fond » dure ; durées et échecs relevés.
class BackgroundExports
{
public:
    explicit BackgroundExports(ui::DocumentController* document) : m_document(document)
    {
        QObject::connect(&m_watcher, &QFutureWatcher<exporting::ExportResult>::finished, &m_watcher, [this] {
            const exporting::ExportResult result = m_watcher.result();
            m_running = false;
            ++m_done;
            m_failed += result.ok ? 0 : 1;
            m_maxMs = std::max(m_maxMs, static_cast<double>(m_clock.nsecsElapsed()) / 1e6);
            if (m_active) {
                launch();
            }
        });
    }
    bool valid() const { return m_dir.isValid() && m_document != nullptr; }
    void start()
    {
        m_active = true;
        launch();
    }
    // Attend l'export en cours (au plus `timeoutMs`) sans en relancer.
    bool finish(const std::function<bool(const std::function<bool()>&, int)>& wait, int timeoutMs)
    {
        m_active = false;
        return wait([this] { return !m_running; }, timeoutMs);
    }
    int done() const { return m_done; }
    int failed() const { return m_failed; }
    double maxMs() const { return m_maxMs; }

private:
    void launch()
    {
        m_running = true;
        m_clock.start();
        m_watcher.setFuture(exporting::exportPdf(
            m_document->document(), QUrl::fromLocalFile(m_dir.filePath(QStringLiteral("fond.pdf")))));
    }

    ui::DocumentController* m_document;
    QTemporaryDir m_dir;
    QFutureWatcher<exporting::ExportResult> m_watcher;
    QElapsedTimer m_clock;
    bool m_active = false;
    bool m_running = false;
    int m_done = 0;
    int m_failed = 0;
    double m_maxMs = 0;
};

// Joue le geste `command` et relève les redessins du cache terminés (PageView::cacheRefreshCount) de sa
// fenêtre : ouverte quand plus rien n'attend au-dessus du cache et que le cache est prêt, fermée à la
// stabilité (cache prêt, rien en attente, aucun redessin pendant kCacheFoldIdleMs + 300 ms). Faux si le geste
// n'a rien changé au document (un geste sans effet mesurerait 0 pour de fausses raisons).
// Rang des gestes dans editionGestures() (scenarios.h ; le quatrième, « front », est le cas par défaut) et
// résultat qui leur répond.
constexpr int kPasteGesture = 0;
constexpr int kGroupGesture = 1;
constexpr int kAlignGesture = 2;
constexpr int EditionRefreshes::* kGestureSlots[] = {&EditionRefreshes::paste, &EditionRefreshes::group,
                                                     &EditionRefreshes::align, &EditionRefreshes::front};
// Annotations alignées (rangs dans la page d'« edition ») et nombre minimal d'annotations qui les contient.
constexpr std::size_t kAlignPicks[] = {25, 50, 75, 100, 125};
constexpr std::size_t kMinAnnotations = 130;
static_assert(kAlignPicks[std::size(kAlignPicks) - 1] < kMinAnnotations);

bool runEditionGesture(EditionGear& g, const QString& command, EditionRefreshes& out)
{
    const core::Page& page =
        g.document->document().page(static_cast<std::size_t>(g.document->currentIndex()));
    std::vector<core::ObjectId> annotations;
    for (const core::PageObject& object : page.objects) {
        if (core::isAnnotation(object)) {
            annotations.push_back(object.common.id);
        }
    }
    if (annotations.size() < kMinAnnotations) {
        return false;
    }
    const auto pick = [&annotations](std::span<const std::size_t> indexes) {
        std::vector<core::ObjectId> ids;
        for (const std::size_t i : indexes) {
            ids.push_back(annotations[i]);
        }
        return ids;
    };
    int gesture = -1;
    for (std::size_t i = 0; i < kEditionGestureCount; ++i) {
        if (command == QLatin1String(editionGestures()[i])) {
            gesture = static_cast<int>(i);
        }
    }
    if (gesture < 0) {
        return false;
    }
    const bool paste = gesture == kPasteGesture;
    // Préparation, hors fenêtre : la sélection ne redessine pas le cache.
    std::vector<core::ObjectId> before = pageIds(page);
    if (paste) {
        std::vector<core::ObjectId> ids;
        for (std::size_t i = 1; i <= static_cast<std::size_t>(kEditionPasted); ++i) {
            ids.push_back(annotations[i]);
        }
        g.tools->selectObjects(ids);
        g.editing->copy();
    } else if (gesture == kGroupGesture) {
        if (g.pasted.empty()) {
            return false;
        }
        g.tools->selectObjects(g.pasted);
    } else if (gesture == kAlignGesture) {
        g.tools->selectObjects(pick(kAlignPicks));
    } else {
        constexpr std::size_t kFirst[] = {0};
        g.tools->selectObjects(pick(kFirst));
    }
    const auto quiet = [&g] { return g.fixed->pendingCount() == 0 && g.view->cacheReady(); };
    if (!waitUntil(quiet, 15000)) {
        return false;
    }
    int edits = 0;
    const auto edited =
        QObject::connect(g.document, &ui::DocumentController::undoChanged, g.document, [&edits] { ++edits; });
    std::int64_t lastNs = core::monotonicNowNs();
    const auto updated = QObject::connect(g.view, &ui::PageView::cacheUpdated, g.view,
                                          [&lastNs] { lastNs = core::monotonicNowNs(); });
    const int refreshesBefore = g.view->cacheRefreshCount();
    switch (gesture) {
    case kPasteGesture:
        g.editing->paste();
        break;
    case kGroupGesture:
        g.editing->toggleGroup();
        break;
    case kAlignGesture:
        g.editing->align(QStringLiteral("left"));
        break;
    default:
        g.editing->arrange(QStringLiteral("front"));
        break;
    }
    runWindowCommand(g.window, QStringLiteral("tool.pen")); // les traits suivants sont écrits au stylo
    const std::int64_t idleNs = std::int64_t{ui::kCacheFoldIdleMs + 300} * 1'000'000;
    const bool stable =
        waitUntil([&] { return quiet() && core::monotonicNowNs() - lastNs >= idleNs; }, 20000);
    const int refreshes = g.view->cacheRefreshCount() - refreshesBefore;
    QObject::disconnect(edited);
    QObject::disconnect(updated);
    if (!stable || edits < 1) {
        return false;
    }
    if (paste) {
        g.pasted.clear();
        const std::set<core::ObjectId> known(before.begin(), before.end());
        const core::Page& after =
            g.document->document().page(static_cast<std::size_t>(g.document->currentIndex()));
        for (const core::ObjectId id : pageIds(after)) {
            if (known.count(id) == 0) {
                g.pasted.push_back(id);
            }
        }
        if (g.pasted.size() != static_cast<std::size_t>(kEditionPasted)) {
            return false;
        }
    }
    out.*kGestureSlots[gesture] = refreshes;
    return true;
}

} // namespace

void showDemoTargets(ui::PageView& view)
{
    view.setZoom(1.0);
    // À 100 %, page et vue ont la même échelle : défilement = centre de la vue − centre des points (setPan le
    // borne). Sans lui, la vue garde le centre laissé par le défilement (400 %, haut à gauche) : à 1366 ×
    // 768, la sonde du témoin de trait tombait sous le bas de la fenêtre.
    view.setPan(QPointF(view.width() / 2, view.height() / 2) - demoTargetsCenter());
}

Runner::Runner(BenchOptions options) : m_options(std::move(options)) {}

Runner::~Runner() = default;

bool Runner::waitFor(const std::function<bool()>& condition, int timeoutMs)
{
    QElapsedTimer clock;
    clock.start();
    QTimer wake; // réveille la boucle toutes les 10 ms sans la faire tourner à vide
    wake.start(10);
    while (!condition()) {
        if (clock.elapsed() > timeoutMs) {
            return false;
        }
        QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);
    }
    return true;
}

bool Runner::pageShown() const
{
    return m_fixed->isReady() && m_view->cacheReady();
}

bool Runner::waitPageShown()
{
    return waitFor([this] { return pageShown(); }, kPageShownTimeoutMs);
}

void Runner::waitMs(int ms)
{
    QElapsedTimer clock;
    clock.start();
    waitFor([&clock, ms] { return clock.elapsed() >= ms; }, ms + 1000);
}

bool Runner::placeWindow(BenchResult& result)
{
    QScreen* screen = QGuiApplication::primaryScreen();
    m_window->setScreen(screen);
    if (m_options.fullScreen && !offscreenPlatform()) {
        m_window->setGeometry(screen->geometry());
        m_window->showFullScreen();
        result.windowMode = QStringLiteral("fullscreen");
    } else {
        const QSize size = m_options.fullScreen ? kOffscreenWindowSize : m_options.windowSize;
        m_window->resize(size);
        const QRect available = screen->availableGeometry();
        m_window->setPosition(available.center() - QPoint(size.width() / 2, size.height() / 2));
        m_window->showNormal();
        result.windowMode = QStringLiteral("window");
    }
    m_window->requestActivate();
    if (!waitFor([this] { return m_window->isExposed(); }, 10000)) {
        return false;
    }
    // Relevé, sans invalider : les entrées sont postées à la fenêtre (décision 28).
    result.activated = waitFor([this] { return m_window->isActive(); }, 2000);
    waitMs(200);
    result.windowSize = m_window->size();
    result.screenName = screen->name();
    result.screenSize = screen->size();
    result.refreshHz = screen->refreshRate(); // nominale
    result.refresh = referenceRefreshPeriod(readDwmRefreshPeriodNs(), result.refreshHz);
    result.devicePixelRatio = m_window->devicePixelRatio();
    return true;
}

std::int64_t Runner::warmUp(int ms)
{
    // Rendu continu : chaque image présentée en demande une autre. La médiane des intervalles ne sert que de
    // contrôle : hors synchronisation verticale (logiciel, hors écran), elle suit le cadencement de
    // requestUpdate et le coût du rendu, pas l'écran ; la référence est la période de l'écran (placeWindow).
    const auto pump = QObject::connect(m_window, &QQuickWindow::frameSwapped, m_window, &QQuickWindow::update,
                                       Qt::QueuedConnection);
    m_recorder->begin();
    m_window->update();
    waitMs(ms);
    // Rendu lent (debug, logiciel hors écran : une image prend des dizaines de ms) : la chauffe se prolonge,
    // bornée, jusqu'à avoir assez d'intervalles pour mesurer ; au-delà, la médiane reste 0.
    waitFor([this] { return m_recorder->swapNs().size() > kMinRefreshIntervals; }, 5000);
    m_recorder->end();
    QObject::disconnect(pump);
    return measureRefreshPeriodNs(m_recorder->swapNs());
}

bool Runner::setLevel(const ScenarioPlan& plan)
{
    m_view->setZoom(plan.zoom);
    if (plan.topLeft) {
        m_view->setPan(QPointF(ui::kViewMargin, ui::kViewMargin));
    }
    // L10-T4 : ce qui attend encore au-dessus du cache (traits du scénario précédent) est versé avant la
    // mesure ; sinon il passerait au scénario suivant (pending_max, versement à son premier zoom).
    if (m_fixed->pendingCount() > 0) {
        m_view->requestCacheRefresh();
    }
    // Attendre la fin du redessin du cache (scheduledUpdateCompleted), et du versement, avant la fenêtre de
    // mesure (décision 17).
    const bool ready =
        waitFor([this] { return m_fixed->pendingCount() == 0 && m_view->cacheReady(); }, 10000);
    waitMs(100);
    return ready;
}

void Runner::startInputs(std::vector<PlannedInput> inputs, std::int64_t t0Ns)
{
    m_view->resetInputSequence(0);
    m_injector->start(std::move(inputs), t0Ns);
}

ScenarioResult Runner::play(const ScenarioPlan& plan, std::int64_t refreshPeriodNs,
                            QStringList& invalidReasons)
{
    ScenarioResult s;
    s.name = plan.name;
    s.touchedStrokes = plan.touchedStrokes;
    s.pendingCut = plan.pendingCut;
    s.liftedObjects = plan.liftedObjects;
    s.freezeExempt = plan.freezeExempt;
    s.maxCacheRefreshes = plan.maxCacheRefreshes;
    s.placedShapes = plan.placedShapes;
    const int rebuildsAtStart = m_fixed->rebuildCount();
    const int decodesAtStart = m_fixed->decodeCount();
    // Pendant un scénario, la vue n'écoute que les périphériques synthétiques (décision 23).
    m_view->setExclusiveDevices({m_injector->stylus(), m_injector->wheel()});
    m_view->setInkProbe(m_recorder.get());

    std::vector<std::int64_t> freezes;
    const auto frozen = QObject::connect(m_view, &ui::PageView::strokeFrozen, m_view,
                                         [&freezes] { freezes.push_back(core::monotonicNowNs()); });
    const auto requested = QObject::connect(m_view, &ui::PageView::cacheRefreshRequested, m_view,
                                            [&s] { s.cacheRequestNs.push_back(core::monotonicNowNs()); });
    const auto updated = QObject::connect(m_view, &ui::PageView::cacheUpdated, m_view,
                                          [&s] { s.cacheUpdatedNs.push_back(core::monotonicNowNs()); });
    // L10-T4 : objets en attente au-dessus du cache (versement différé). Relevé aussi sur objectCountChanged,
    // émis après la mise en attente et avant pendingChanged : au plafond, la vue (branchée avant le banc)
    // verse pendant pendingChanged, qui n'arrive ici qu'avec une attente déjà vide.
    const auto notePending = [this, &s] { s.pendingMax = std::max(s.pendingMax, m_fixed->pendingCount()); };
    const auto pendingWatch =
        QObject::connect(m_fixed, &render::FixedPageItem::pendingChanged, m_view, notePending);
    const auto countWatch =
        QObject::connect(m_fixed, &render::FixedPageItem::objectCountChanged, m_view, notePending);
    int viewChanges = 0; // la molette postée a-t-elle atteint la vue ?
    const auto zoomed =
        QObject::connect(m_view, &ui::PageView::zoomChanged, m_view, [&viewChanges] { ++viewChanges; });
    const auto panned =
        QObject::connect(m_view, &ui::PageView::panChanged, m_view, [&viewChanges] { ++viewChanges; });
    bool exposureLost = false;
    QTimer exposure;
    exposure.setInterval(50);
    QObject::connect(&exposure, &QTimer::timeout, &exposure, [this, &exposureLost] {
        const auto v = m_window->visibility();
        if (!m_window->isExposed() || v == QWindow::Minimized || v == QWindow::Hidden) {
            exposureLost = true;
        }
    });
    exposure.start();

    // Commandes nommées (InputKind::Command) : appelées d'ici à leur heure, jamais postées ; l'injecteur
    // poste le reste (tablette, molette) et ne les voit pas (sequenceMatchesPlan ne compte que les siennes).
    std::vector<PlannedInput> toPost;
    std::vector<PlannedInput> commands;
    for (const PlannedInput& in : plan.inputs) {
        (in.kind == InputKind::Command ? commands : toPost).push_back(in);
    }
    std::vector<std::int64_t> commandPlanned(commands.size(), 0);
    std::vector<std::int64_t> commandRun(commands.size(), -1);
    std::vector<bool> commandAwaits(commands.size(), true); // un prêt (page, cache) doit suivre la commande
    std::size_t nextCommand = 0;
    QTimer commandTimer; // coup unique réarmé sur l'échéance suivante, comme l'injecteur
    commandTimer.setSingleShot(true);
    commandTimer.setTimerType(Qt::PreciseTimer);
    EditionGear edition;
    if (plan.measuresEdition) {
        edition.window = m_window;
        edition.view = m_view;
        edition.fixed = m_fixed;
        edition.document = m_window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        edition.tools = m_window->findChild<ui::ToolHost*>(QStringLiteral("appTools"));
        edition.editing = m_window->findChild<ui::ObjectEditing*>(QStringLiteral("appEditing"));
        if (!edition.document || !edition.tools || !edition.editing) {
            invalidReasons << QStringLiteral("error: edition gear missing in %1").arg(plan.name);
        }
        s.edition.emplace();
    }
    TabsGear tabsGear;
    if (plan.tabCount > 0) {
        tabsGear.tabs = m_window->findChild<ui::DocumentTabs*>(QStringLiteral("appTabs"));
        tabsGear.view = m_view;
        tabsGear.fixed = m_fixed;
        if (!tabsGear.tabs) {
            invalidReasons << QStringLiteral("error: tabs missing in %1").arg(plan.name);
        }
        s.tabs.emplace();
    }
    std::unique_ptr<BackgroundExports> exports;
    if (plan.exportsInBackground) {
        exports = std::make_unique<BackgroundExports>(
            m_window->findChild<ui::DocumentController*>(QStringLiteral("documentController")));
        if (!exports->valid()) {
            invalidReasons << QStringLiteral("error: background export not possible in %1").arg(plan.name);
            exports.reset();
        }
    }
    // L5-C14 : charge témoin de la référence, vivante jusqu'à la fin de la mesure (m_recorder->end()).
    std::unique_ptr<SpinningLoad> spin;
    if (plan.spinsInBackground) {
        spin = std::make_unique<SpinningLoad>();
    }
    const auto runDueCommands = [&] {
        while (nextCommand < commands.size() && commandPlanned[nextCommand] <= core::monotonicNowNs()) {
            const PlannedInput& in = commands[nextCommand];
            commandRun[nextCommand] = core::monotonicNowNs();
            const bool gesture = plan.measuresEdition && in.command.startsWith(QLatin1String("edition."));
            const bool tabSwitch = plan.tabCount > 0 && in.command == QLatin1String("tabs.switch");
            const bool done = tabSwitch ? (tabsGear.tabs != nullptr &&
                                           runTabSwitch(tabsGear, in.argument.toInt(), s.tabs->switchMs))
                              : gesture ? (edition.editing != nullptr && edition.tools != nullptr &&
                                           edition.document != nullptr &&
                                           runEditionGesture(edition, in.command, *s.edition))
                                        : runWindowCommand(m_window, in.command, in.argument);
            if (!done) {
                invalidReasons
                    << QStringLiteral("error: command %1 refused in %2").arg(in.command, plan.name);
            }
            // Page et cache encore prêts : aucun prêt ne viendra (zoom sans cache), la fenêtre se ferme à la
            // première image qui suit la commande.
            commandAwaits[nextCommand] = !(m_fixed->isReady() && m_view->cacheReady());
            ++nextCommand;
        }
        if (nextCommand < commands.size()) {
            const std::int64_t waitNs = commandPlanned[nextCommand] - core::monotonicNowNs();
            commandTimer.start(waitNs <= 0 ? 0 : static_cast<int>((waitNs + 999'999) / 1'000'000));
        }
    };
    QObject::connect(&commandTimer, &QTimer::timeout, &commandTimer, runDueCommands);
    // Changements de page : sans entrée continue, quelques images par changement seulement. Rendu continu
    // pendant la mesure (comme la chauffe) : les intervalles existent entre deux changements ; ceux d'un
    // changement (de la commande à la première image après le prêt) sont comptés actifs (pageChangeMarks).
    QMetaObject::Connection pump;
    // Instants où la page affichée est prête et son cache redessiné : fin d'un changement de page.
    std::vector<std::int64_t> shownNs;
    QMetaObject::Connection readyWatch;
    QMetaObject::Connection cacheWatch;
    if (!commands.empty()) {
        const auto noteShown = [this, &shownNs] {
            if (m_fixed->isReady() && m_view->cacheReady()) {
                shownNs.push_back(core::monotonicNowNs());
            }
        };
        readyWatch = QObject::connect(m_fixed, &render::FixedPageItem::readyChanged, m_view, noteShown);
        cacheWatch = QObject::connect(m_view, &ui::PageView::cacheReadyChanged, m_view, noteShown);
        pump = QObject::connect(m_window, &QQuickWindow::frameSwapped, m_window, &QQuickWindow::update,
                                Qt::QueuedConnection);
        m_window->update();
    }

    // L4-T29 : miniatures. Rendus terminés dans la fenêtre, rendus lancés entre un appui et son relâché
    // (launchCount lu à chaque changement de contact et de rendu), copie la plus longue (lastCopyMs, posé au
    // lancement).
    ui::PageThumbnails* const thumbs = plan.measuresThumbnails ? m_thumbnails : nullptr;
    const int thumbRenders0 = thumbs ? thumbs->renderCount() : 0;
    int lastLaunch = thumbs ? thumbs->launchCount() : 0;
    // Instants relevés (monotonicNowNs) : lancements de rendu et fenêtres de contact (appui à relâché) ;
    // `launchesDuringContact` en tire le compte à la fin (bornes comprises).
    std::vector<std::int64_t> launchNs;
    std::vector<std::pair<std::int64_t, std::int64_t>> contactWindows;
    double maxCopyMs = 0;
    bool inContact = false;
    QMetaObject::Connection thumbWatch;
    QMetaObject::Connection contactWatch;
    if (thumbs) {
        const auto note = [this, thumbs, &lastLaunch, &launchNs, &contactWindows, &maxCopyMs, &inContact] {
            const std::int64_t at = core::monotonicNowNs();
            const int now = thumbs->launchCount();
            for (int k = lastLaunch; k < now; ++k) {
                launchNs.push_back(at);
            }
            if (now != lastLaunch) {
                maxCopyMs = std::max(maxCopyMs, thumbs->lastCopyMs());
            }
            lastLaunch = now;
            const bool contact = m_view->contactActive();
            if (contact && !inContact) {
                contactWindows.emplace_back(at, std::numeric_limits<std::int64_t>::max()); // ouverte
            } else if (!contact && inContact && !contactWindows.empty()) {
                contactWindows.back().second = at;
            }
            inContact = contact;
        };
        thumbWatch = QObject::connect(thumbs, &ui::PageThumbnails::renderingChanged, m_view, note);
        contactWatch = QObject::connect(m_view, &ui::PageView::contactActiveChanged, m_view, note);
    }

    m_recorder->begin();
    s.startNs = core::monotonicNowNs() + 20'000'000;
    startInputs(std::move(toPost), s.startNs);
    for (std::size_t i = 0; i < commands.size(); ++i) {
        commandPlanned[i] = s.startNs + commands[i].offsetNs;
    }
    if (exports) {
        exports->start(); // les traits partent à la même heure : l'export tourne pendant toute la mesure
    }
    runDueCommands();
    const auto allSent = [this, &nextCommand, &commands] {
        return m_injector->finished() && nextCommand == commands.size();
    };
    if (!waitFor(allSent, static_cast<int>(planDurationNs(plan) / 1'000'000) + 10000)) {
        m_injector->stop(); // abandon : plus rien n'est posté ; finished() reste faux (run() l'invalide)
        commandTimer.stop();
        if (nextCommand < commands.size()) {
            invalidReasons << QStringLiteral("error: commands incomplete in %1").arg(plan.name);
        }
    }
    // Fin du dernier changement (L3-C3) : la mesure attend le prêt qui ferme sa fenêtre (relevé par
    // noteShown), puis deux images, avant les 150 ms. Sans cela, un changement plus long que ces 150 ms
    // (debug, machine chargée) restait ouvert jusqu'à la fin de la mesure : aucun intervalle au repos après
    // la première commande (tst_bench_result, pageChangesAreActiveUntilShown, rouge sous charge). Sans prêt
    // relevé, l'attente va au plafond et la fenêtre reste ouverte, comme avant.
    if (!commands.empty()) {
        std::int64_t awaited = -1; // dernière commande jouée qui attend un prêt
        for (std::size_t i = 0; i < commands.size(); ++i) {
            if (commandRun[i] >= 0 && commandAwaits[i]) {
                awaited = commandRun[i];
            }
        }
        const bool closed =
            awaited < 0 ||
            waitFor([&shownNs, awaited] { return !shownNs.empty() && shownNs.back() >= awaited; },
                    kPageShownTimeoutMs);
        if (closed) {
            const std::size_t swaps = m_recorder->swapNs().size();
            waitFor([this, swaps] { return m_recorder->swapNs().size() >= swaps + 2; }, 5000);
        }
    }
    // Versement différé du dernier trait (L3-C14, plan.awaitFinalFold : « pauses ») : la mesure attend que le
    // trait passe en attente (son relâché posté est traité), puis que le délai d'inactivité le verse et que
    // le cache soit redessiné ; aucune entrée ne suit, rien ne le repousse. Sans cela, les seuls versements
    // mesurés étaient ceux des reprises k = 1 et k = 2 (24 et 48 ms au-dessus du délai en court) : un relâché
    // traité en retard d'autant sous charge relançait la minuterie, aucun versement n'était relevé et
    // tst_bench_result (cacheRefreshesAreRecorded) échouait. Sans versement, l'attente va au plafond.
    if (plan.awaitFinalFold && waitFor([this] { return m_fixed->pendingCount() > 0; }, kPageShownTimeoutMs)) {
        waitFor([this] { return m_fixed->pendingCount() == 0 && m_view->cacheReady(); },
                ui::kCacheFoldIdleMs + kPageShownTimeoutMs);
    }
    // Panneau ouvert : le dernier trait laisse sa miniature à refaire kSettleMs plus tard ; la mesure attend
    // ce rendu pour qu'il tombe dans la fenêtre. Sans rendu, l'attente va au plafond (renders = 0 relevé).
    if (thumbs && plan.browsersOpen) {
        waitFor([thumbs, thumbRenders0] { return thumbs->renderCount() > thumbRenders0; },
                ui::PageThumbnails::kSettleMs + 10000);
    }
    waitMs(150); // dernières images : figement, redessin du cache
    m_recorder->end();
    if (spin) {
        s.controlLoadIterations = spin->iterations();
        if (s.controlLoadIterations <= 0) { // le fil n'a pas tourné : la référence ne vaudrait rien
            invalidReasons << QStringLiteral("error: control load did not run in %1").arg(plan.name);
        }
        spin.reset();
    }
    if (exports) {
        const bool finished = exports->finish([this](const std::function<bool()>& condition,
                                                     int timeoutMs) { return waitFor(condition, timeoutMs); },
                                              30000);
        if (!finished || exports->done() == 0 || exports->failed() > 0) {
            invalidReasons
                << QStringLiteral("error: background export incomplete or failed in %1").arg(plan.name);
        }
        s.exportBackground.emplace();
        s.exportBackground->exports = exports->done();
        s.exportBackground->failures = exports->failed();
        s.exportBackground->maxExportMs = exports->maxMs();
    }
    QObject::disconnect(thumbWatch);
    QObject::disconnect(contactWatch);
    if (thumbs) {
        s.thumbnails = ThumbnailStats{thumbs->renderCount() - thumbRenders0,
                                      launchesDuringContact(launchNs, contactWindows), maxCopyMs};
    }
    QObject::disconnect(pump);
    QObject::disconnect(readyWatch);
    QObject::disconnect(cacheWatch);

    exposure.stop();
    QObject::disconnect(frozen);
    QObject::disconnect(requested);
    QObject::disconnect(updated);
    QObject::disconnect(pendingWatch);
    QObject::disconnect(countWatch);
    QObject::disconnect(zoomed);
    QObject::disconnect(panned);
    m_view->setInkProbe(nullptr);
    m_view->setExclusiveDevices({});

    s.exposureLost = exposureLost;
    const auto& planned = m_injector->plannedNs();
    const auto& posted = m_injector->postedNs();
    const auto& inputs = m_injector->inputs();
    s.frames.swapNs = m_recorder->swapNs();
    for (std::size_t i = 0; i < posted.size(); ++i) {
        if (posted[i] >= 0) {
            // Instants prévus (décision 23) : un fil principal bloqué qui poste en retard ne rend pas
            // l'intervalle « au repos » ; l'instant d'envoi ne sert qu'au retard d'injection.
            s.frames.inputNs.push_back(planned[i]);
            s.injectionDelayMs.push_back(static_cast<double>(posted[i] - planned[i]) / 1e6);
        }
    }
    for (std::size_t i = 0; i < commands.size(); ++i) {
        if (commandRun[i] >= 0) {
            s.frames.inputNs.push_back(commandPlanned[i]);
            s.injectionDelayMs.push_back(static_cast<double>(commandRun[i] - commandPlanned[i]) / 1e6);
        }
    }
    std::sort(s.frames.inputNs.begin(), s.frames.inputNs.end());
    s.frames.freezeNs = freezes;
    s.frames.refreshPeriodNs = refreshPeriodNs; // période de l'écran ; ≤ 0 → invalid-refresh-period
    // Commandes : leurs changements (commande → première image après le prêt) comptent actifs.
    FrameSeries classified = s.frames;
    const std::vector<std::int64_t> marks =
        pageChangeMarks(s.frames.swapNs, pageChangeWindows(commandRun, commandAwaits, shownNs));
    classified.inputNs.insert(classified.inputNs.end(), marks.begin(), marks.end());
    std::sort(classified.inputNs.begin(), classified.inputNs.end());
    s.intervals = classifyIntervals(classified);

    const auto received = m_recorder->received();
    if (!sequenceMatchesPlan(inputs, received)) {
        invalidReasons << QStringLiteral("sequence-mismatch in %1").arg(plan.name);
    }
    const bool hasMeasured =
        std::any_of(inputs.begin(), inputs.end(), [](const PlannedInput& in) { return in.measured; });
    if (hasMeasured) {
        const auto records = m_recorder->syncRecords();
        s.releases =
            releaseCoverage(inputs, received, m_recorder->syncStartNs(), records); // jugé par applyVerdict
        std::vector<std::int64_t> measured(planned.size(), -1);
        for (std::size_t i = 0; i < inputs.size(); ++i) {
            if (inputs[i].measured) {
                measured[i] = planned[i];
            }
        }
        s.latency = computeLatency(measured, records);
        for (const auto& [seq, at] : received) {
            if (seq < planned.size()) {
                s.receptionDelayMs.push_back(static_cast<double>(at - planned[seq]) / 1e6);
            }
        }
    }
    const bool hasWheel = std::any_of(inputs.begin(), inputs.end(),
                                      [](const PlannedInput& in) { return in.kind == InputKind::Wheel; });
    if (hasWheel && viewChanges == 0) {
        invalidReasons << QStringLiteral("error: wheel not delivered in %1").arg(plan.name);
    }
    s.memory = m_recorder->memory();
    if (s.exportBackground) { // latence d'encre et figements : la référence « stroke » vient du runner
        s.exportBackground->inkP95Ms = s.latency ? s.latency->total.p95 : 0;
        s.exportBackground->freezes = static_cast<int>(s.intervals.freezeOverLimit);
    }
    s.rebuilds = m_fixed->rebuildCount() - rebuildsAtStart;
    s.decodes = m_fixed->decodeCount() - decodesAtStart;
    if (plan.measuresShapePreview) { // L4-T14 (V9) : trames pendant les glissés de forme
        std::vector<std::pair<std::int64_t, std::int64_t>> windows;
        for (const auto& [from, to] : dragWindows(plan)) {
            windows.emplace_back(s.startNs + from, s.startNs + to);
        }
        s.shapePreview = shapePreviewStats(s.frames.swapNs, windows);
    }
    return s;
}

void saveCaptures(const QString& dirPath, const QString& graphicsApi, const QList<QImage>& images,
                  QStringList& errors)
{
    QDir dir(dirPath);
    if (!dir.mkpath(QStringLiteral("."))) {
        errors << QStringLiteral("error: capture directory not created: %1").arg(dirPath);
        return;
    }
    const QStringList steps = captureSteps();
    for (qsizetype i = 0; i < steps.size() && i < images.size(); ++i) {
        const QString file = dir.filePath(captureFileName(graphicsApi, steps.at(i)));
        if (!images.at(i).save(file)) {
            errors << QStringLiteral("error: capture not saved: %1").arg(file);
        }
    }
}

void Runner::runDemo(BenchResult& result)
{
    // Séquence non mesurée (décision 31) : captures en mémoire, 5 contrôles, PNG seulement avec --captures.
    const auto must = [&](const char* step, const std::function<bool()>& condition, int timeoutMs) {
        if (!waitFor(condition, timeoutMs)) {
            result.invalidReasons << QStringLiteral("error: demo %1").arg(QLatin1String(step));
        }
    };
    const auto cacheReady = [this] { return m_view->cacheReady(); };
    const auto injected = [this] { return m_injector->finished(); };
    showDemoTargets(*m_view);
    must("cache not ready at start", cacheReady, 10000);
    waitMs(100);
    m_view->setExclusiveDevices({m_injector->stylus(), m_injector->wheel()});

    const std::vector<PlannedInput> demo = demoStroke();
    const auto mid = static_cast<std::ptrdiff_t>(kDemoMidIndex);
    const std::vector<PlannedInput> firstHalf(demo.begin(), demo.begin() + mid);
    std::vector<PlannedInput> secondHalf(demo.begin() + mid, demo.end());
    const std::int64_t shift = secondHalf.front().offsetNs;
    for (PlannedInput& in : secondHalf) {
        in.offsetNs -= shift;
    }

    startInputs(firstHalf, core::monotonicNowNs());
    must("first half not injected", injected, 5000);
    waitMs(100);
    const QPoint inkPixel =
        sceneToImage(m_view->pageToScene(demoInkProbePoint()), m_window->devicePixelRatio());
    const QImage midStroke = m_window->grabWindow();

    startInputs(secondHalf, core::monotonicNowNs());
    must("second half not injected", injected, 5000);
    waitMs(50);
    must("cache not ready after stroke", cacheReady, 10000); // trait figé dans le cache
    waitMs(100);
    const QImage endStroke = m_window->grabWindow();
    // Lot 10 : le trait de démonstration attend au-dessus du cache, puis y est versé ; il doit y être encore.
    must(
        "stroke not folded", [this] { return m_fixed->pendingCount() == 0 && m_view->cacheReady(); },
        ui::kCacheFoldIdleMs + 5000);
    waitMs(100);
    const QImage folded = m_window->grabWindow();

    m_view->setZoom(8.0);
    must("cache not ready at zoom 800", cacheReady, 10000);
    waitMs(100);
    const QImage zoom800 = m_window->grabWindow();

    showDemoTargets(*m_view);
    must("cache not ready back at 100", cacheReady, 10000);
    waitMs(100);
    const QImage back100 = m_window->grabWindow();
    m_view->setExclusiveDevices({});

    const core::Witnesses& w = core::witnesses();
    const qreal dpr = m_window->devicePixelRatio();
    const auto at = [&](core::PointF p) { return sceneToImage(m_view->pageToScene(QPointF(p.x, p.y)), dpr); };
    result.contentChecks = {
        checkColor(QStringLiteral("witness-shape"), back100, at(w.shapeRect.center()),
                   QColor::fromRgba(w.shapeColor), kWitnessTolerance),
        checkColor(QStringLiteral("witness-image"), back100, at(w.imageRect.center()),
                   QColor::fromRgba(w.imageCenterColor), kWitnessTolerance),
        checkColor(QStringLiteral("witness-stroke"), back100, at(w.strokeProbe),
                   QColor::fromRgba(w.strokeColor), kWitnessTolerance),
        checkColor(QStringLiteral("ink-mid-stroke"), midStroke, inkPixel, QColor(Qt::black), kInkTolerance),
        checkColor(QStringLiteral("stroke-folded"), folded, inkPixel, QColor(Qt::black), kInkTolerance),
    };

    if (!m_options.capturesDir.isEmpty()) {
        saveCaptures(m_options.capturesDir, result.graphicsApi, {midStroke, endStroke, zoom800, back100},
                     result.invalidReasons);
    }
}

// Contrôle « page_flash » (L2-T23, revue). À la première image rendue après un changement de page, tant
// que la nouvelle page n'est pas prête, le centre du témoin de forme garde la couleur qu'il avait sur
// l'ancienne page (relevée juste avant) : ni fond blanc, ni page transparente, ni autre couleur. Une
// capture n'est retenue que si la nouvelle page n'est pas encore prête (sinon elle ne prouve rien) :
// jusqu'à 6 changements pour 3 captures. Le verdict repose sur les captures SANS cache : avec cache, la
// vue ne redessine sa texture qu'au prêt (PageView::setFixedPage) et masque la page fixe ; celles-là sont
// un relevé à part.
void Runner::checkPageFlash(BenchResult& out, ui::DocumentController& document)
{
    const bool cached = m_view->cacheEnabled();
    const core::PointF center = core::witnesses().shapeRect.center();
    const auto witness = [this, center](const QImage& image) {
        return sampleColor(image, sceneToImage(m_view->pageToScene(QPointF(center.x, center.y)),
                                               m_window->devicePixelRatio()));
    };
    int swaps = 0;
    const auto counter =
        QObject::connect(m_window, &QQuickWindow::frameSwapped, m_window, [&swaps] { ++swaps; });
    int flashes = 0;
    int cachedFlashes = 0;
    int probes = 0;
    bool failed = false;
    for (const bool withCache : {true, false}) {
        if ((withCache && !cached) || failed) {
            continue;
        }
        m_view->setCacheEnabled(withCache);
        if (!waitPageShown()) {
            out.invalidReasons << QStringLiteral("error: page flash check: page not shown");
            failed = true;
            break;
        }
        int kept = 0;
        for (int attempt = 0; attempt < 6 && kept < 3; ++attempt) {
            waitMs(50);
            const QColor expected = witness(m_window->grabWindow()); // ancienne page, prête
            const int before = swaps;
            const QString command =
                document.currentIndex() == 0 ? QStringLiteral("page.next") : QStringLiteral("page.previous");
            if (!runWindowCommand(m_window, command) ||
                !waitFor([&swaps, before] { return swaps > before; }, 5000)) {
                out.invalidReasons << QStringLiteral("error: page flash check: %1 not shown").arg(command);
                failed = true;
                break;
            }
            const bool decoding = !m_fixed->isReady();
            const QColor color = witness(m_window->grabWindow());
            if (decoding) {
                ++kept;
                if (!expected.isValid() || !color.isValid() ||
                    !colorNear(color, expected, kWitnessTolerance)) {
                    ++(withCache ? cachedFlashes : flashes);
                }
            }
            if (!waitPageShown()) {
                out.invalidReasons << QStringLiteral("error: page flash check: %1 never ready").arg(command);
                failed = true;
                break;
            }
        }
        if (!withCache) {
            probes = kept;
        }
    }
    QObject::disconnect(counter);
    m_view->setCacheEnabled(cached);
    if (!failed && probes == 0) {
        out.invalidReasons << QStringLiteral("page-flash-unverified"); // décodage plus court qu'une image
    }
    // La démonstration se joue sur la page 1 (mêmes témoins que la page générée de départ).
    if (document.currentIndex() != 0) {
        runWindowCommand(m_window, QStringLiteral("page.previous"));
    }
    if (!waitPageShown()) {
        out.invalidReasons << QStringLiteral("error: page 1 not ready after page flash check");
    }
    waitMs(100);
    out.pageFlash = flashes;
    out.pageFlashCached = cached ? cachedFlashes : -1;
    out.pageFlashProbes = probes;
}

BenchResult Runner::run(QQuickWindow* window)
{
    BenchResult result;
    result.startedAt = QDateTime::currentDateTime();
    result.machine = readMachineInfo();
    result.requestedBackend = m_options.requestedBackend;
    result.scenarioSet =
        m_options.scenarioSet == ScenarioSet::Court ? QStringLiteral("court") : QStringLiteral("complet");

    m_window = window;
    m_view = m_window ? m_window->findChild<ui::PageView*>(QStringLiteral("pageView")) : nullptr;
    m_fixed = m_window ? m_window->findChild<render::FixedPageItem*>(QStringLiteral("fixedPage")) : nullptr;
    auto* const document =
        m_window ? m_window->findChild<ui::DocumentController*>(QStringLiteral("documentController"))
                 : nullptr;
    if (!m_view || !m_fixed || !document || m_recorder) {
        result.invalidReasons << QStringLiteral("error: interface incomplete");
        applyVerdict(result);
        return result;
    }
    m_recorder = std::make_unique<FrameRecorder>(m_window);
    m_injector = std::make_unique<Injector>(m_window, [this](QPointF p) { return m_view->pageToScene(p); });

    if (!placeWindow(result)) {
        result.invalidReasons << QStringLiteral("error: window not exposed");
        applyVerdict(result);
        return result;
    }
    // Page prête : images chargées et premier cache dessiné.
    if (!waitPageShown()) {
        result.invalidReasons << QStringLiteral("error: page not ready");
        applyVerdict(result);
        return result;
    }
    m_thumbnails = m_window->findChild<ui::PageThumbnails*>(QStringLiteral("appThumbnails"));
    result.graphicsApi = apiName(m_window->rendererInterface()->graphicsApi());
    result.cache = m_view->cacheEnabled();
    // Médiane d'un rendu continu : contrôle de cohérence de la période de référence (applyVerdict).
    result.refresh.measuredMedianNs = warmUp(2000 / durationDivisor(m_options.scenarioSet));
    const std::int64_t refreshPeriodNs = result.refresh.referenceNs;

    for (const ScenarioPlan& plan : allScenarios(m_options.scenarioSet)) {
        if (plan.generatedPages > 0) {
            // Scénario « page » (L2-T23) : document de pages générées (pages paires peintes), page 1 prête.
            document->loadGeneratedPages(plan.generatedPages, true);
            if (!waitPageShown()) {
                result.invalidReasons << QStringLiteral("error: pages not ready before %1").arg(plan.name);
                break;
            }
        }
        // L3-T19 : annotations de départ (versées par setLevel, comme des traits passés), puis l'outil du
        // scénario choisi par sa commande nommée, avant la mesure.
        for (const auto& points : plan.seedStrokes) {
            document->commitStroke(seedAnnotation(points));
        }
        std::optional<TabsStats> tabsSetup;
        if (plan.fixtureDocument) { // L5-T31 : document synthétique à exporter
            document->replaceDocument(tabsFixtureDocument(0), QUrl(), QStringLiteral("essai"));
            if (!waitPageShown()) {
                result.invalidReasons << QStringLiteral("error: fixture not ready before %1").arg(plan.name);
                break;
            }
        }
        if (plan.tabCount > 0) { // L5-T31 : un onglet, mémoire, puis tous les onglets, mémoire
            auto* const tabs = m_window->findChild<ui::DocumentTabs*>(QStringLiteral("appTabs"));
            const auto quiet = [this] { return pageShown() && m_fixed->pendingCount() == 0; };
            if (!tabs || tabs->rowCount() != 1) {
                result.invalidReasons << QStringLiteral("error: tabs not at one before %1").arg(plan.name);
                break;
            }
            document->replaceDocument(tabsFixtureDocument(0), QUrl(), QStringLiteral("essai 1"));
            waitFor(quiet, 15000);
            waitMs(500);
            TabsStats stats;
            stats.memoryOneTabMb = static_cast<double>(readProcessMemory().privateBytes) / 1048576.0;
            for (int i = 1; i < plan.tabCount; ++i) {
                tabs->newTab();
                if (tabs->rowCount() != i + 1) {
                    result.invalidReasons << QStringLiteral("error: tab %1 not opened").arg(i + 1);
                    break;
                }
                document->replaceDocument(tabsFixtureDocument(i), QUrl(),
                                          QStringLiteral("essai %1").arg(i + 1));
                waitFor(quiet, 15000);
            }
            tabs->activate(0);
            waitFor(quiet, 15000);
            waitMs(500);
            stats.memoryFourTabsMb = static_cast<double>(readProcessMemory().privateBytes) / 1048576.0;
            stats.memoryDeltaMb = stats.memoryFourTabsMb - stats.memoryOneTabMb;
            stats.memoryBudgetMb = tabsMemoryBudgetMb(m_window->size());
            tabsSetup = stats;
            if (tabs->rowCount() != plan.tabCount) {
                break;
            }
        }
        if (plan.gridAbove) { // L4-T14 : grille du dessus, versée dans le cache avant la mesure (setLevel)
            document->execute(std::make_unique<core::SetGridCommand>(0, gridAboveFixture()));
        }
        if (plan.zones) { // L4-T28 : 40 x 53 cadres, séparations en surcouche pendant la mesure
            core::PageZones zones;
            zones.horizontal = kBenchZonesAcross;
            zones.vertical = kBenchZonesDown;
            document->execute(std::make_unique<core::SetPageZonesCommand>(0, zones));
            const core::PageZones& applied = document->document().page(0).zones;
            if (applied.horizontal != kBenchZonesAcross || applied.vertical != kBenchZonesDown) {
                result.invalidReasons << QStringLiteral("error: zones not applied before %1").arg(plan.name);
                break;
            }
        }
        bool setupRefused = false;
        for (const auto& [command, argument] : plan.setupCommands) {
            if (!runWindowCommand(m_window, command, argument)) {
                result.invalidReasons
                    << QStringLiteral("error: command %1 refused before %2").arg(command, plan.name);
                setupRefused = true;
                break;
            }
        }
        if (setupRefused) {
            break;
        }
        if (!plan.toolCommand.isEmpty() && !runWindowCommand(m_window, plan.toolCommand)) {
            result.invalidReasons
                << QStringLiteral("error: command %1 refused before %2").arg(plan.toolCommand, plan.name);
            break;
        }
        if (!setLevel(plan)) {
            result.invalidReasons << QStringLiteral("error: cache not ready before %1").arg(plan.name);
            break;
        }
        if (plan.measuresThumbnails) { // L4-T29 : le panneau est dans l'état voulu, les miniatures à jour
            if (m_thumbnails == nullptr || m_thumbnails->active() != plan.browsersOpen) {
                result.invalidReasons
                    << QStringLiteral("error: browsers panel not %1 before %2")
                           .arg(plan.browsersOpen ? QStringLiteral("open") : QStringLiteral("closed"),
                                plan.name);
                break;
            }
            if (plan.browsersOpen && !waitFor([this] { return m_thumbnails->pendingCount() == 0; }, 30000)) {
                result.invalidReasons
                    << QStringLiteral("error: thumbnails not ready before %1").arg(plan.name);
                break;
            }
        }
        // Compteurs de la page fixe au zoom posé (view.zoom 150) : ce qu'il refait jusqu'à la fin du
        // scénario.
        std::optional<std::pair<int, int>> atZoom;
        bool zoomSeen = false;
        const auto zoomWatch =
            QObject::connect(m_view, &ui::PageView::zoomChanged, m_view, [this, &atZoom, &zoomSeen, &plan] {
                if (plan.pageChecks && !zoomSeen && std::abs(m_view->zoom() - 1.5) < 1e-9) {
                    zoomSeen = true;
                    // Relevé seulement si la page peinte est prête et dessinée : sinon sa reconstruction au
                    // prêt gonflerait le compte (-1 : non mesuré).
                    if (pageShown()) {
                        atZoom = std::make_pair(m_fixed->rebuildCount(), m_fixed->rasterizationCount());
                    }
                }
            });
        result.scenarios.push_back(play(plan, refreshPeriodNs, result.invalidReasons));
        QObject::disconnect(zoomWatch);
        if (tabsSetup && result.scenarios.back().tabs) {
            tabsSetup->switchMs = result.scenarios.back().tabs->switchMs;
            result.scenarios.back().tabs = tabsSetup;
        }
        if (result.scenarios.back().exportBackground) { // références du même passage
            const auto findScenario = [&result](const QString& name) {
                return std::find_if(result.scenarios.begin(), result.scenarios.end(),
                                    [&name](const ScenarioResult& r) { return r.name == name; });
            };
            const auto baseline = findScenario(QStringLiteral("export-fond-reference"));
            ExportBackgroundStats& ex = *result.scenarios.back().exportBackground;
            if (baseline == result.scenarios.end() || !baseline->latency) {
                result.invalidReasons << QStringLiteral("error: no baseline reference for %1").arg(plan.name);
            } else {
                ex.baselineInkP95Ms = baseline->latency->total.p95;
                ex.baselineFreezes = static_cast<int>(baseline->intervals.freezeOverLimit);
                ex.baselineControlLoadIterations = baseline->controlLoadIterations;
            }
            const auto stroke = findScenario(QStringLiteral("stroke"));
            if (stroke == result.scenarios.end() || !stroke->latency) {
                result.invalidReasons << QStringLiteral("error: no stroke reference for %1").arg(plan.name);
            } else {
                ex.strokeInkP95Ms = stroke->latency->total.p95;
                ex.strokeFreezes = static_cast<int>(stroke->intervals.freezeOverLimit);
            }
        }
        if (plan.tabCount > 0 || plan.fixtureDocument) { // le banc retrouve un onglet et la page générée
            if (auto* const tabs = m_window->findChild<ui::DocumentTabs*>(QStringLiteral("appTabs"))) {
                while (tabs->rowCount() > 1) {
                    tabs->closeWithoutSaving(tabs->rowCount() - 1);
                }
            }
            document->loadGeneratedPages(1, false);
            if (!waitPageShown()) {
                result.invalidReasons << QStringLiteral("error: page not restored after %1").arg(plan.name);
                break;
            }
        }
        for (const auto& [command, argument] : plan.teardownCommands) { // L4-T29 : panneau refermé
            if (!runWindowCommand(m_window, command, argument)) {
                result.invalidReasons
                    << QStringLiteral("error: command %1 refused after %2").arg(command, plan.name);
            }
        }
        if (!plan.toolCommand.isEmpty() && !runWindowCommand(m_window, QStringLiteral("tool.pen"))) {
            result.invalidReasons
                << QStringLiteral("error: command tool.pen refused after %1").arg(plan.name);
        }
        if (!m_injector->finished()) {
            result.invalidReasons << QStringLiteral("error: injection incomplete in %1").arg(plan.name);
            break;
        }
        if (plan.pageChecks) {
            if (atZoom) {
                result.pageRebuildsDuringZoom = m_fixed->rebuildCount() - atZoom->first;
                result.pageRasterizationsDuringZoom = m_fixed->rasterizationCount() - atZoom->second;
            }
            checkPageFlash(result, *document);
        }
    }
    if (result.invalidReasons.isEmpty()) { // à ce stade, seules des erreurs y figurent
        runDemo(result);
    }
    result.renderLoop = m_recorder->renderLoop();
    result.gpuName = m_recorder->gpuName().isEmpty() ? result.graphicsApi : m_recorder->gpuName();
    applyVerdict(result);
    return result;
}

} // namespace actcraft::bench
