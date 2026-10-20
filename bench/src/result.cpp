#include <actcraft/bench/result.h>

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>

namespace actcraft::bench {

namespace {

QJsonObject percentiles(const Percentiles& p)
{
    return {{"p50", p.p50},
            {"p95", p.p95},
            {"p99", p.p99},
            {"max", p.max},
            {"count", static_cast<double>(p.count)}};
}

QJsonArray relative(const std::vector<std::int64_t>& values, std::int64_t origin)
{
    QJsonArray out;
    for (std::int64_t v : values) {
        out.append(static_cast<double>(v - origin));
    }
    return out;
}

QJsonArray doubles(const std::vector<double>& values)
{
    QJsonArray out;
    for (double v : values) {
        out.append(v);
    }
    return out;
}

QJsonValue bytes(const std::optional<std::uint64_t>& v)
{
    return v ? QJsonValue(static_cast<double>(*v)) : QJsonValue(QJsonValue::Null);
}

// A actif, F figement, R reprise après pause, I repos.
QString classes(const std::vector<IntervalClass>& values)
{
    QString out;
    for (IntervalClass c : values) {
        switch (c) {
        case IntervalClass::Active:
            out += QLatin1Char('A');
            break;
        case IntervalClass::Freeze:
            out += QLatin1Char('F');
            break;
        case IntervalClass::Resume:
            out += QLatin1Char('R');
            break;
        case IntervalClass::Idle:
            out += QLatin1Char('I');
            break;
        }
    }
    return out;
}

QJsonObject scenarioJson(const ScenarioResult& s)
{
    const IntervalStats& iv = s.intervals;
    const std::size_t intervals = iv.classes.size();
    const std::size_t counted = iv.activeMs.size() + iv.freezeMs.size() + iv.resumeMs.size();
    const QJsonObject frames{
        {"count", static_cast<double>(s.frames.swapNs.size())},
        {"intervals", static_cast<double>(intervals)},
        {"active", static_cast<double>(iv.activeMs.size())},
        {"freeze", static_cast<double>(iv.freezeMs.size())},
        {"resume", static_cast<double>(iv.resumeMs.size())},
        {"idle", static_cast<double>(intervals > counted ? intervals - counted : 0)},
        {"fps", iv.fps},
        {"p50_ms", iv.active.p50},
        {"p95_ms", iv.active.p95},
        {"p99_ms", iv.active.p99},
        {"max_ms", iv.active.max},
        {"over_25ms_ratio", iv.over25Ratio},
        {"freeze_max_ms", iv.freezeMaxMs},
        {"freeze_over_limit", static_cast<double>(iv.freezeOverLimit)},
        {"freeze_unmeasured", static_cast<double>(iv.freezeUnmeasured)},
        {"resume_max_ms", iv.resumeMaxMs},
        {"resume_over_limit", static_cast<double>(iv.resumeOverLimit)},
        {"invalid_refresh_period", iv.invalidRefreshPeriod || s.frames.refreshPeriodNs <= 0},
        {"refresh_period_ms", static_cast<double>(s.frames.refreshPeriodNs) / 1'000'000.0},
    };
    QJsonValue latency(QJsonValue::Null);
    QJsonArray latencySeq;
    if (s.latency) {
        latency = QJsonObject{
            {"samples", static_cast<double>(s.latency->seq.size())},
            {"unmatched", static_cast<double>(s.latency->unmatched)},
            {"total_ms", percentiles(s.latency->total)},
            {"planned_to_sync_ms", percentiles(s.latency->plannedToSync)},
            {"sync_to_swap_ms", percentiles(s.latency->syncToSwap)},
            {"releases_checked", static_cast<double>(s.releases.checked)},
            {"releases_not_synced", static_cast<double>(s.releases.notSynced)},
        };
        for (std::uint64_t seq : s.latency->seq) {
            latencySeq.append(static_cast<double>(seq));
        }
    }
    const QJsonObject input{
        {"count", static_cast<double>(s.frames.inputNs.size())},
        {"injection_delay_ms", percentiles(summarize(s.injectionDelayMs))},
        {"reception_delay_ms", percentiles(summarize(s.receptionDelayMs))},
    };
    const QJsonObject memory{
        {"process_private_start_bytes", static_cast<double>(s.memory.privateStart)},
        {"process_private_end_bytes", static_cast<double>(s.memory.privateEnd)},
        {"process_private_peak_bytes", static_cast<double>(s.memory.privatePeak)},
        {"working_set_peak_bytes", static_cast<double>(s.memory.workingSetPeak)},
        {"gpu_start_bytes", bytes(s.memory.gpuStart)},
        {"gpu_end_bytes", bytes(s.memory.gpuEnd)},
        {"gpu_peak_bytes", bytes(s.memory.gpuPeak)},
    };
    const QJsonObject raw{
        {"swap_ns", relative(s.frames.swapNs, s.startNs)},
        {"interval_class", classes(iv.classes)},
        {"input_ns", relative(s.frames.inputNs, s.startNs)},
        {"freeze_ns", relative(s.frames.freezeNs, s.startNs)},
        {"cache_request_ns", relative(s.cacheRequestNs, s.startNs)},
        {"cache_updated_ns", relative(s.cacheUpdatedNs, s.startNs)},
        {"latency_seq", latencySeq},
        {"latency_total_ms", s.latency ? doubles(s.latency->totalMs) : QJsonArray()},
        {"latency_planned_to_sync_ms", s.latency ? doubles(s.latency->plannedToSyncMs) : QJsonArray()},
        {"latency_sync_to_swap_ms", s.latency ? doubles(s.latency->syncToSwapMs) : QJsonArray()},
    };
    QJsonObject cache{{"refreshes", static_cast<double>(s.cacheRequestNs.size())},
                      {"pending_max", s.pendingMax}};
    if (s.touchedStrokes >= 0) {
        cache.insert(QStringLiteral("touched_strokes"), s.touchedStrokes);
    }
    if (s.liftedObjects >= 0) {
        cache.insert(QStringLiteral("lifted_objects"), s.liftedObjects);
    }
    if (s.pendingCut >= 0) {
        cache.insert(QStringLiteral("pending_cut"), s.pendingCut);
    }
    if (s.rebuilds >= 0) {
        cache.insert(QStringLiteral("rebuilds"), s.rebuilds);
    }
    if (s.decodes >= 0) {
        cache.insert(QStringLiteral("decodes"), s.decodes);
    }
    return {{"name", s.name},   {"exposure_lost", s.exposureLost},
            {"frames", frames}, {"latency", latency},
            {"input", input},   {"memory", memory},
            {"cache", cache},   {"raw", raw}};
}

} // namespace

QString apiName(QSGRendererInterface::GraphicsApi api)
{
    switch (api) {
    case QSGRendererInterface::Software:
        return QStringLiteral("software");
    case QSGRendererInterface::Direct3D11:
        return QStringLiteral("direct3d11");
    case QSGRendererInterface::Direct3D12:
        return QStringLiteral("direct3d12");
    case QSGRendererInterface::Metal:
        return QStringLiteral("metal");
    case QSGRendererInterface::Vulkan:
        return QStringLiteral("vulkan");
    case QSGRendererInterface::OpenGL:
        return QStringLiteral("opengl");
    case QSGRendererInterface::Null:
        return QStringLiteral("null");
    default:
        return QStringLiteral("unknown");
    }
}

bool contentCheckOk(const BenchResult& result)
{
    if (result.contentChecks.empty() || result.pageFlash > 0) {
        return false;
    }
    for (const ContentCheck& c : result.contentChecks) {
        if (!c.ok) {
            return false;
        }
    }
    return true;
}

QJsonObject toJson(const BenchResult& r)
{
    const QJsonObject meta{
        {"commit", r.machine.commit},
        {"commit_dirty", r.machine.commitDirty},
        {"qt_version", r.machine.qtVersion},
        {"os", r.machine.os},
        {"kernel", r.machine.kernel},
        {"cpu", r.machine.cpu},
        {"machine_model", r.machine.model},
        {"gpu", r.gpuName},
        {"requested_backend", r.requestedBackend},
        {"render_loop", r.renderLoop},
        {"build_type", r.machine.buildType},
        {"compiler_flags", r.machine.compilerFlags},
        {"scenario_set", r.scenarioSet},
        {"activated", r.activated},
        {"screen", QJsonObject{{"name", r.screenName},
                               {"width", r.screenSize.width()},
                               {"height", r.screenSize.height()},
                               {"refresh_hz", r.refreshHz},
                               {"device_pixel_ratio", r.devicePixelRatio}}},
        {"refresh",
         QJsonObject{{"reference_period_ms", static_cast<double>(r.refresh.referenceNs) / 1'000'000.0},
                     {"source", r.refresh.source},
                     {"measured_median_ms", static_cast<double>(r.refresh.measuredMedianNs) / 1'000'000.0}}},
        {"window", QJsonObject{{"mode", r.windowMode},
                               {"width", r.windowSize.width()},
                               {"height", r.windowSize.height()}}},
        {"limits",
         QStringLiteral("Latence interne : de l'instant prévu de l'échantillon au frameSwapped de la "
                        "première image qui le contient. Ni le numériseur, ni la pile d'entrée du "
                        "système, ni la composition (DWM), ni le balayage de l'écran. Entrées "
                        "synthétiques postées à la fenêtre (QTabletEvent, QWheelEvent).")},
    };
    QJsonArray scenarios;
    for (const ScenarioResult& s : r.scenarios) {
        scenarios.append(scenarioJson(s));
    }
    QJsonArray checks;
    for (const ContentCheck& c : r.contentChecks) {
        checks.append(
            QJsonObject{{"name", c.name}, {"ok", c.ok}, {"expected", c.expected}, {"actual", c.actual}});
    }
    QJsonObject contentCheck{{"ok", contentCheckOk(r)}, {"checks", checks}};
    if (r.pageFlash >= 0) { // scénario « page » joué (L2-T23)
        contentCheck.insert("page_flash", r.pageFlash);
        contentCheck.insert("page_flash_probes", r.pageFlashProbes);
        contentCheck.insert("page_flash_cached", r.pageFlashCached);
        contentCheck.insert("page_rebuilds_during_zoom", r.pageRebuildsDuringZoom);
        contentCheck.insert("page_rasterizations_during_zoom", r.pageRasterizationsDuringZoom);
    }
    QJsonArray criteria;
    for (const Criterion& c : r.criteria) {
        criteria.append(QJsonObject{{"scenario", c.scenario},
                                    {"criterion", c.name},
                                    {"value", c.value},
                                    {"limit", c.limit},
                                    {"ok", c.ok},
                                    {"excluded", c.excluded}});
    }
    const QJsonObject verdict{
        {"valid_for_measurement", r.valid},
        {"invalid_reasons", QJsonArray::fromStringList(r.invalidReasons)},
        {"within_budget", r.withinBudget},
        {"informative", r.informative},
        {"criteria", criteria},
    };
    return {
        {"format", QString::fromLatin1(kResultFormat)},
        {"started_at", r.startedAt.toString(Qt::ISODate)},
        {"graphics_api", r.graphicsApi},
        {"cache", r.cache},
        {"valid_for_measurement", r.valid},
        {"within_budget", r.withinBudget},
        {"informative", r.informative},
        {"meta", meta},
        {"scenarios", scenarios},
        {"content_check", contentCheck},
        {"verdict", verdict},
    };
}

QString resultFileName(const BenchResult& r)
{
    return r.startedAt.toString(QStringLiteral("yyyy-MM-dd_HHmmss")) + QLatin1Char('-') +
           sanitizeForFileName(r.machine.model) + QLatin1Char('-') + sanitizeForFileName(r.graphicsApi) +
           QStringLiteral(".json");
}

bool writeResult(const BenchResult& result, const QString& path, QString& error)
{
    const QFileInfo info(path);
    if (!QDir().mkpath(info.absolutePath())) {
        error = QStringLiteral("dossier impossible à créer : %1").arg(info.absolutePath());
        return false;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        error = file.errorString();
        return false;
    }
    file.write(QJsonDocument(toJson(result)).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        error = file.errorString();
        return false;
    }
    return true;
}

} // namespace actcraft::bench
