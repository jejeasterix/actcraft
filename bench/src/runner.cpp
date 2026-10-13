#include <actcraft/bench/runner.h>
#include <actcraft/bench/verdict.h>
#include <actcraft/core/clock.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/viewtransform.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QScreen>
#include <QTimer>

#include <algorithm>

namespace actcraft::bench {

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
    if (m_options.fullScreen) {
        m_window->setGeometry(screen->geometry());
        m_window->showFullScreen();
        result.windowMode = QStringLiteral("fullscreen");
    } else {
        m_window->resize(m_options.windowSize);
        const QRect available = screen->availableGeometry();
        m_window->setPosition(available.center() -
                              QPoint(m_options.windowSize.width() / 2, m_options.windowSize.height() / 2));
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
    result.refreshHz = screen->refreshRate(); // nominale ; la période utilisée est mesurée (warmUp)
    result.devicePixelRatio = m_window->devicePixelRatio();
    return true;
}

std::int64_t Runner::warmUp(int ms)
{
    // Rendu continu : chaque image présentée en demande une autre. La médiane des intervalles est la période
    // effective de présentation (synchronisation verticale), pas la valeur nominale de QScreen (N1-1).
    const auto pump = QObject::connect(m_window, &QQuickWindow::frameSwapped, m_window, &QQuickWindow::update,
                                       Qt::QueuedConnection);
    m_recorder->begin();
    m_window->update();
    waitMs(ms);
    // Rendu lent (debug, logiciel hors écran : une image prend des dizaines de ms) : la chauffe se prolonge,
    // bornée, jusqu'à avoir assez d'intervalles pour mesurer ; au-delà, la période reste inconnue (invalide).
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
    // Attendre la fin du redessin du cache (scheduledUpdateCompleted) avant la fenêtre de mesure (décision
    // 17).
    const bool ready = waitFor([this] { return m_view->cacheReady(); }, 10000);
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
    // Pendant un scénario, la vue n'écoute que les périphériques synthétiques (décision 23).
    m_view->setExclusiveDevices({m_injector->stylus(), m_injector->wheel()});
    m_view->setInkProbe(m_recorder.get());

    std::vector<std::int64_t> freezes;
    const auto frozen = QObject::connect(m_view, &ui::PageView::strokeFrozen, m_view,
                                         [&freezes] { freezes.push_back(core::monotonicNowNs()); });
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

    m_recorder->begin();
    s.startNs = core::monotonicNowNs() + 20'000'000;
    startInputs(plan.inputs, s.startNs);
    if (!waitFor([this] { return m_injector->finished(); },
                 static_cast<int>(planDurationNs(plan) / 1'000'000) + 10000)) {
        m_injector->stop(); // abandon : plus rien n'est posté ; finished() reste faux (run() l'invalide)
    }
    waitMs(150); // dernières images : figement, redessin du cache
    m_recorder->end();

    exposure.stop();
    QObject::disconnect(frozen);
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
    std::sort(s.frames.inputNs.begin(), s.frames.inputNs.end());
    s.frames.freezeNs = freezes;
    s.frames.refreshPeriodNs = refreshPeriodNs; // mesurée à la chauffe ; ≤ 0 → invalid-refresh-period
    s.intervals = classifyIntervals(s.frames);

    const auto received = m_recorder->received();
    if (!sequenceMatchesPlan(inputs, received)) {
        invalidReasons << QStringLiteral("sequence-mismatch in %1").arg(plan.name);
    }
    const bool hasMeasured =
        std::any_of(inputs.begin(), inputs.end(), [](const PlannedInput& in) { return in.measured; });
    if (hasMeasured) {
        const auto records = m_recorder->syncRecords();
        if (releasesNotSynced(inputs, received, s.frames.swapNs, records) > 0) {
            invalidReasons << QStringLiteral("release-not-synced in %1").arg(plan.name);
        }
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
    return s;
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
    if (!m_view || !m_fixed || m_recorder) {
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
    if (!waitFor([this] { return m_fixed->isReady() && m_view->cacheReady(); }, 30000)) {
        result.invalidReasons << QStringLiteral("error: page not ready");
        applyVerdict(result);
        return result;
    }
    result.graphicsApi = apiName(m_window->rendererInterface()->graphicsApi());
    result.cache = m_view->cacheEnabled();
    const std::int64_t refreshPeriodNs = warmUp(2000 / durationDivisor(m_options.scenarioSet));

    for (const ScenarioPlan& plan : allScenarios(m_options.scenarioSet)) {
        if (!setLevel(plan)) {
            result.invalidReasons << QStringLiteral("error: cache not ready before %1").arg(plan.name);
            break;
        }
        result.scenarios.push_back(play(plan, refreshPeriodNs, result.invalidReasons));
        if (!m_injector->finished()) {
            result.invalidReasons << QStringLiteral("error: injection incomplete in %1").arg(plan.name);
            break;
        }
    }
    result.renderLoop = m_recorder->renderLoop();
    result.gpuName = m_recorder->gpuName().isEmpty() ? result.graphicsApi : m_recorder->gpuName();
    applyVerdict(result);
    return result;
}

} // namespace actcraft::bench
