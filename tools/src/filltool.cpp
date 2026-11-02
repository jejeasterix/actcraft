#include <actcraft/core/edits.h>
#include <actcraft/core/fill.h>
#include <actcraft/core/hittest.h>
#include <actcraft/render/imagefill.h>
#include <actcraft/tools/filltool.h>

#include <QBuffer>
#include <QByteArray>
#include <QImage>
#include <QPoint>
#include <QtConcurrent/QtConcurrentRun>

#include <algorithm>
#include <utility>
#include <variant>

namespace actcraft::tools {

namespace {

// Calcul d'une zone d'image : décodage, remplissage, encodage PNG. Tourne hors du fil principal.
std::string computeImageFill(const QByteArray& data, core::RectF bounds, core::PointF point, core::Argb color,
                             int tolerance)
{
    const QImage image = QImage::fromData(data);
    if (image.isNull() || bounds.width <= 0 || bounds.height <= 0) {
        return {};
    }
    const int x = std::clamp(static_cast<int>((point.x - bounds.x) / bounds.width * image.width()), 0,
                             image.width() - 1);
    const int y = std::clamp(static_cast<int>((point.y - bounds.y) / bounds.height * image.height()), 0,
                             image.height() - 1);
    const QImage filled = render::floodFill(image, QPoint(x, y), color, tolerance);
    QByteArray png;
    QBuffer buffer(&png);
    buffer.open(QIODevice::WriteOnly);
    if (!filled.save(&buffer, "PNG")) {
        return {};
    }
    return std::string(png.constData(), static_cast<std::size_t>(png.size()));
}

} // namespace

FillTool::FillTool(ToolContext& context) : m_context(context) {}

FillTool::~FillTool()
{
    for (QFutureWatcher<std::string>* watcher : m_jobs) {
        watcher->disconnect();
        delete watcher;
    }
}

QString FillTool::id() const
{
    return QStringLiteral("fill");
}

// Un clic agit à la presse ; aucun geste ne dure : l'outil n'est jamais occupé.
bool FillTool::isBusy() const noexcept
{
    return false;
}

void FillTool::cancel()
{
    // Les calculs d'image en cours ne sont pas interrompus : leur résultat est jeté s'il est périmé.
}

void FillTool::setColor(core::Argb color)
{
    m_color = color;
}

void FillTool::setComputeObserver(std::function<void(QThread*)> observer)
{
    m_observer = std::move(observer);
}

int FillTool::staleImageFillDropped() const noexcept
{
    return m_stale;
}

void FillTool::onSample(const input::Sample& sample)
{
    if (sample.phase != input::Phase::Press) {
        return;
    }
    const core::PointF point{sample.x, sample.y};
    const core::Page& page = m_context.document.page();
    const double zoom = m_context.view.zoom() > 0 ? m_context.view.zoom() : 1.0;
    // Toutes les règles (forme du dessus, verrouillés remplis, fond de page) viennent du cœur.
    const core::FillHit hit = core::fillHitAt(page, point, core::kHitToleranceScreen / zoom);
    if (hit.target == core::FillTarget::Nothing) {
        return;
    }
    if (hit.target == core::FillTarget::Image) {
        fillImage(hit.object, point);
        return;
    }
    if (auto command = core::makeFillCommand(page, m_context.document.pageIndex(), hit, m_color)) {
        m_context.document.execute(std::move(command));
    }
}

void FillTool::fillImage(core::ObjectId image, core::PointF point)
{
    const core::Page& page = m_context.document.page();
    const std::size_t index = core::indexOf(page, image);
    if (index >= page.objects.size()) {
        return;
    }
    const auto* object = std::get_if<core::ImageObject>(&page.objects[index]);
    if (object == nullptr || object->resource.empty()) {
        return; // image procédurale : aucune ressource à remplir
    }
    const std::string key = object->resource;
    const QByteArray data = m_context.document.resourceData(key);
    if (data.isEmpty()) {
        return;
    }
    const core::RectF bounds = core::boundsOnPage(page.objects[index]);
    const std::size_t pageIndex = m_context.document.pageIndex();
    const std::uint64_t revision = m_context.document.revision();
    const core::Argb color = m_color;
    const auto observer = m_observer;

    auto* watcher = new QFutureWatcher<std::string>();
    m_jobs.push_back(watcher);
    QObject::connect(watcher, &QFutureWatcher<std::string>::finished, watcher,
                     [this, watcher, pageIndex, revision, image, key] {
                         finishImageFill(watcher, pageIndex, revision, image, key);
                     });
    watcher->setFuture(QtConcurrent::run([=] {
        std::string png = computeImageFill(data, bounds, point, color, core::kDefaultImageFillTolerance);
        if (observer) {
            observer(QThread::currentThread());
        }
        return png;
    }));
}

// Retour sur le fil principal : le résultat est appliqué seulement si rien n'a changé depuis le clic.
void FillTool::finishImageFill(QFutureWatcher<std::string>* watcher, std::size_t pageIndex,
                               std::uint64_t revision, core::ObjectId image, const std::string& key)
{
    std::string png = watcher->result();
    m_jobs.erase(std::remove(m_jobs.begin(), m_jobs.end(), watcher), m_jobs.end());
    watcher->deleteLater();
    if (png.empty()) {
        return;
    }
    const core::Page& page = m_context.document.page();
    bool same = m_context.document.pageIndex() == pageIndex && m_context.document.revision() == revision;
    if (same) {
        const std::size_t index = core::indexOf(page, image);
        const auto* object =
            index < page.objects.size() ? std::get_if<core::ImageObject>(&page.objects[index]) : nullptr;
        same = object != nullptr && object->resource == key;
    }
    if (!same) {
        ++m_stale;
        return;
    }
    m_context.document.execute(core::makeImageFillCommand(pageIndex, image, std::move(png)));
}

} // namespace actcraft::tools
