#include <actcraft/render/imagestore.h>
#include <actcraft/render/imagesynth.h>

#include <QMutexLocker>
#include <QThread>

#include <algorithm>

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

void ImageStore::generate(const core::Page& page)
{
    const std::uint64_t generation = ++m_generation;
    {
        QMutexLocker lock(&m_mutex);
        m_images.clear();
    }
    m_ready = false;
    m_pending = 0;
    m_offOwnerThread = true;
    QThread* const owner = thread();
    for (std::size_t i = 0; i < page.objects.size(); ++i) {
        const auto* object = std::get_if<core::ImageObject>(&page.objects[i]);
        if (!object) {
            continue;
        }
        ++m_pending;
        const core::ImageObject copy = *object;
        m_pool.start([this, generation, i, copy, owner] {
            const QImage image = synthesizeImage(copy);
            const bool off = QThread::currentThread() != owner;
            QMetaObject::invokeMethod(
                this, [this, generation, i, image, off] { onImageReady(generation, i, image, off); },
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

void ImageStore::onImageReady(std::uint64_t generation, std::size_t index, const QImage& image,
                              bool offOwnerThread)
{
    if (generation != m_generation) {
        return;
    }
    m_offOwnerThread = m_offOwnerThread && offOwnerThread;
    {
        QMutexLocker lock(&m_mutex);
        m_images[index] = image;
    }
    if (--m_pending == 0) {
        m_ready = true;
        emit ready();
    }
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
