#include <actcraft/render/imagestore.h>
#include <actcraft/render/imagesynth.h>

#include <QMutexLocker>
#include <QThread>

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <utility>
#include <vector>

namespace actcraft::render {

ImageStore::ImageStore(QObject* parent) : QObject(parent)
{
    m_pool.setMaxThreadCount(std::max(1, QThread::idealThreadCount() - 1));
}

ImageStore::~ImageStore()
{
    // Les tâches postent vers `this` : on les attend avant que l'objet disparaisse.
    m_pool.clear();
    m_pool.waitForDone();
}

namespace {

// Pixels de la couleur `transparent` (alpha ignoré) rendus transparents.
QImage withKeyColor(const QImage& source, core::Argb transparent)
{
    QImage image = source.convertToFormat(QImage::Format_ARGB32);
    const QRgb key = transparent & 0x00FFFFFFu;
    for (int y = 0; y < image.height(); ++y) {
        auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            if ((line[x] & 0x00FFFFFFu) == key) {
                line[x] = 0;
            }
        }
    }
    return image;
}

} // namespace

std::string ImageStore::variantKey(const core::ImageObject& object)
{
    if (!object.hasKeyColor) {
        return object.resource;
    }
    return object.resource +
           QStringLiteral("#%1").arg(object.keyColor, 8, 16, QLatin1Char('0')).toStdString();
}

void ImageStore::generate(const core::Page& page, const core::Document* resources)
{
    const std::uint64_t generation = ++m_generation;
    {
        QMutexLocker lock(&m_mutex);
        m_images.clear();
        m_resources.clear();
        m_procedural.clear();
    }
    m_ready = false;
    m_pending = 0;
    m_offOwnerThread = true;
    QThread* const owner = thread();
    const auto post = [this, generation, owner](std::size_t index, std::string key, QImage image) {
        const bool off = QThread::currentThread() != owner;
        QMetaObject::invokeMethod(
            this,
            [this, generation, index, key = std::move(key), image = std::move(image), off]() mutable {
                onImageReady(generation, index, std::move(key), image, off);
            },
            Qt::QueuedConnection);
    };
    // Ressources à décoder : clé -> couleurs transparentes demandées (nullopt = image telle quelle).
    std::map<std::string, std::set<std::optional<core::Argb>>> wanted;
    const auto want = [&wanted, resources](const std::string& key, std::optional<core::Argb> transparent) {
        if (!key.empty() && resources && resources->resource(key)) {
            wanted[key].insert(transparent);
        }
    };
    want(page.background.image, std::nullopt);
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        if (const auto* object = std::get_if<core::ImageObject>(&page.objects[i])) {
            if (object->resource.empty()) {
                ++m_pending;
                const core::ImageObject copy = *object;
                {
                    QMutexLocker lock(&m_mutex);
                    m_procedural[i] = copy;
                }
                m_pool.start([post, i, copy] { post(i, std::string(), synthesizeImage(copy)); });
            } else {
                want(object->resource,
                     object->hasKeyColor ? std::optional<core::Argb>(object->keyColor) : std::nullopt);
                if (object->hasKeyColor) {
                    want(object->resource, std::nullopt);
                }
            }
        } else if (const auto* media = std::get_if<core::Media>(&page.objects[i])) {
            want(media->placeholder, std::nullopt);
        }
    }
    for (const auto& [key, variants] : wanted) {
        ++m_pending;
        const QByteArray bytes =
            resources->resource(key)->data; // partagée implicitement : copie sûre entre fils
        m_pool.start([this, generation, owner, key, variants, bytes] {
            const QImage decoded = QImage::fromData(bytes);
            const bool off = QThread::currentThread() != owner;
            std::vector<std::pair<std::string, QImage>> out{{key, decoded}};
            for (const std::optional<core::Argb>& color : variants) {
                if (color && !decoded.isNull()) {
                    core::ImageObject probe;
                    probe.resource = key;
                    probe.hasKeyColor = true;
                    probe.keyColor = *color;
                    out.emplace_back(variantKey(probe), withKeyColor(decoded, *color));
                }
            }
            QMetaObject::invokeMethod(
                this,
                [this, generation, out = std::move(out), off] {
                    if (generation != m_generation) {
                        return;
                    }
                    {
                        QMutexLocker lock(&m_mutex);
                        for (const auto& [k, image] : out) {
                            m_resources[k] = image;
                        }
                    }
                    onImageReady(generation, 0, std::string(), QImage(), off);
                },
                Qt::QueuedConnection);
        });
    }
    if (m_pending == 0) {
        QMetaObject::invokeMethod(
            this,
            [this, generation] {
                if (generation == m_generation) {
                    m_ready = true;
                    emit ready();
                }
            },
            Qt::QueuedConnection);
    }
}

void ImageStore::onImageReady(std::uint64_t generation, std::size_t index, std::string key,
                              const QImage& image, bool offOwnerThread)
{
    if (generation != m_generation) {
        return;
    }
    m_offOwnerThread = m_offOwnerThread && offOwnerThread;
    if (key.empty() && !image.isNull()) {
        QMutexLocker lock(&m_mutex);
        m_images[index] = image;
    }
    if (--m_pending == 0) {
        m_ready = true;
        emit ready();
    }
}

QImage ImageStore::resourceImage(const std::string& key) const
{
    QMutexLocker lock(&m_mutex);
    const auto it = m_resources.find(key);
    return it == m_resources.end() ? QImage() : it->second;
}

QImage ImageStore::objectImage(const core::ImageObject& object, std::size_t objectIndex) const
{
    return object.resource.empty() ? image(objectIndex) : resourceImage(variantKey(object));
}

bool ImageStore::covers(const core::Page& page, const core::Document* resources) const
{
    if (!m_ready) {
        return false;
    }
    QMutexLocker lock(&m_mutex);
    const auto has = [this, resources](const std::string& key) {
        return key.empty() || !resources || !resources->resource(key) || m_resources.count(key) > 0;
    };
    const auto hasVariant = [this, resources](const core::ImageObject& im) {
        return !resources || !resources->resource(im.resource) || m_resources.count(variantKey(im)) > 0;
    };
    std::size_t procedural = 0;
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        if (const auto* im = std::get_if<core::ImageObject>(&page.objects[i])) {
            if (im->resource.empty()) {
                ++procedural;
                const auto it = m_procedural.find(i);
                if (it == m_procedural.end() || it->second.pattern != im->pattern ||
                    it->second.seed != im->seed || it->second.colorA != im->colorA ||
                    it->second.colorB != im->colorB || it->second.sourceWidth != im->sourceWidth ||
                    it->second.sourceHeight != im->sourceHeight) {
                    return false;
                }
            } else if (!has(im->resource) || !hasVariant(*im)) {
                return false;
            }
        } else if (const auto* m = std::get_if<core::Media>(&page.objects[i]); m && !has(m->placeholder)) {
            return false;
        }
    }
    return procedural == m_procedural.size() && has(page.background.image);
}

bool ImageStore::isReady() const noexcept
{
    return m_ready;
}

bool ImageStore::generatedOffOwnerThread() const noexcept
{
    return m_offOwnerThread;
}

QImage ImageStore::image(std::size_t objectIndex) const
{
    QMutexLocker lock(&m_mutex);
    const auto it = m_images.find(objectIndex);
    return it == m_images.end() ? QImage() : it->second;
}

std::size_t ImageStore::imageCount() const
{
    QMutexLocker lock(&m_mutex);
    return m_images.size();
}

} // namespace actcraft::render
