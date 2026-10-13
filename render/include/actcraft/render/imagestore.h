#pragma once

#include <actcraft/core/model.h>

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QThreadPool>

#include <cstdint>
#include <unordered_map>

namespace actcraft::render {

// Génère les images d'une page sur des fils secondaires (règle de fluidité 1) ; ready() quand toutes sont
// prêtes. image() est sûre depuis le fil de rendu.
class ImageStore : public QObject
{
    Q_OBJECT

public:
    explicit ImageStore(QObject* parent = nullptr);
    ~ImageStore() override;

    void generate(const core::Page& page);
    bool isReady() const noexcept;
    QImage image(std::size_t objectIndex) const;
    std::size_t imageCount() const;
    // Crochet de test : vrai si toutes les images de la dernière génération ont été synthétisées hors du fil
    // de l'objet.
    bool generatedOffOwnerThread() const noexcept;

signals:
    void ready();

private:
    void onImageReady(std::uint64_t generation, std::size_t index, const QImage& image, bool offOwnerThread);

    QThreadPool m_pool;
    mutable QMutex m_mutex;
    std::unordered_map<std::size_t, QImage> m_images;
    std::size_t m_pending = 0;
    std::uint64_t m_generation = 0;
    bool m_ready = false;
    bool m_offOwnerThread = true;
};

} // namespace actcraft::render
