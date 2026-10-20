#include <actcraft/render/imagecolor.h>
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

namespace {} // namespace

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
    // Les tâches de la génération précédente pas encore commencées sont annulées (L2-C9) : des PgDown répétés
    // ne décodent pas toutes les pages traversées. Une tâche commencée finit, son résultat est écarté.
    m_pool.clear();
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
                // L3-C13 : rangée par synthesisKey, comme une ressource par sa clé, jamais par indice d'objet
                // : un objet soulevé ou retiré avant elle ne la déplace pas. Deux images de même clé n'en
                // font qu'une.
                const std::string key = synthesisKey(*object);
                bool fresh = false;
                {
                    QMutexLocker lock(&m_mutex);
                    fresh = m_procedural.insert(key).second;
                }
                if (!fresh) {
                    continue;
                }
                ++m_pending;
                const core::ImageObject copy = *object;
                m_pool.start([this, generation, owner, key, copy] {
                    m_started.fetch_add(1);
                    // Couleur transparente appliquée comme la miniature (pagepainter.cpp, paintImage), L2-C9.
                    const QImage synthesized = synthesizeImage(copy);
                    const QImage image =
                        copy.hasKeyColor ? withKeyColor(synthesized, copy.keyColor) : synthesized;
                    const bool off = QThread::currentThread() != owner;
                    QMetaObject::invokeMethod(
                        this,
                        [this, generation, key, image, off] {
                            if (generation != m_generation) {
                                return;
                            }
                            if (!image.isNull()) {
                                QMutexLocker lock(&m_mutex);
                                m_images[key] = image;
                            }
                            onImageReady(generation, off);
                        },
                        Qt::QueuedConnection);
                });
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
            m_started.fetch_add(1);
            const QImage decoded = withoutColorTable(QImage::fromData(bytes)); // L2-C7
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
                    onImageReady(generation, off);
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

void ImageStore::onImageReady(std::uint64_t generation, bool offOwnerThread)
{
    if (generation != m_generation) {
        return;
    }
    m_offOwnerThread = m_offOwnerThread && offOwnerThread;
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

QImage ImageStore::objectImage(const core::ImageObject& object) const
{
    return object.resource.empty() ? image(object) : resourceImage(variantKey(object));
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
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        if (const auto* im = std::get_if<core::ImageObject>(&page.objects[i])) {
            if (im->resource.empty()) {
                // L3-C13 : par synthesisKey, quel que soit l'indice : soulever un objet placé avant l'image
                // ne demande aucun décodage. Une image en plus dans le magasin ne gêne pas (comme une
                // ressource).
                if (m_procedural.count(synthesisKey(*im)) == 0) {
                    return false;
                }
            } else if (!has(im->resource) || !hasVariant(*im)) {
                return false;
            }
        } else if (const auto* m = std::get_if<core::Media>(&page.objects[i]); m && !has(m->placeholder)) {
            return false;
        }
    }
    return has(page.background.image);
}

bool ImageStore::isReady() const noexcept
{
    return m_ready;
}

bool ImageStore::generatedOffOwnerThread() const noexcept
{
    return m_offOwnerThread;
}

std::size_t ImageStore::startedTasks() const noexcept
{
    return m_started.load();
}

bool ImageStore::waitForIdle(int msecs)
{
    return m_pool.waitForDone(msecs);
}

QImage ImageStore::image(const core::ImageObject& object) const
{
    QMutexLocker lock(&m_mutex);
    const auto it = m_images.find(synthesisKey(object));
    return it == m_images.end() ? QImage() : it->second;
}

std::size_t ImageStore::imageCount() const
{
    QMutexLocker lock(&m_mutex);
    return m_images.size();
}

} // namespace actcraft::render
