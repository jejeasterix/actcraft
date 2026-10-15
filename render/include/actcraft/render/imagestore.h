#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QThreadPool>

#include <cstdint>
#include <string>
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

    // Images de la page sur des fils secondaires : procédurales par indice d'objet, ressources de `resources`
    // par clé (images, espaces réservés des médias, image de fond), une fois chacune, et variantes à couleur
    // transparente. ready() quand tout est prêt. Sans `resources`, les ressources restent absentes (images
    // nulles).
    void generate(const core::Page& page, const core::Document* resources = nullptr);
    // Ressource décodée par sa clé, sans masque ; nulle si absente ou illisible. Sûre depuis le fil de rendu.
    QImage resourceImage(const std::string& key) const;
    // Image à dessiner pour cet objet : ressource (variante masquée si couleur transparente) ou image
    // procédurale d'indice `objectIndex`.
    QImage objectImage(const core::ImageObject& object, std::size_t objectIndex) const;
    // Clé de la ressource, suivie de « #aarrggbb » si l'objet a une couleur transparente.
    static std::string variantKey(const core::ImageObject& object);
    // Vrai si les images de la dernière génération suffisent à `page` (mêmes images procédurales aux mêmes
    // indices, toutes les ressources voulues décodées) : la page fixe se reconstruit alors sans redécoder
    // (L2-T14).
    bool covers(const core::Page& page, const core::Document* resources) const;
    bool isReady() const noexcept;
    QImage image(std::size_t objectIndex) const;
    std::size_t imageCount() const;
    // Crochet de test : vrai si toutes les images de la dernière génération ont été synthétisées hors du fil
    // de l'objet.
    bool generatedOffOwnerThread() const noexcept;

signals:
    void ready();

private:
    void onImageReady(std::uint64_t generation, std::size_t index, std::string key, const QImage& image,
                      bool offOwnerThread);

    QThreadPool m_pool;
    mutable QMutex m_mutex;
    std::unordered_map<std::size_t, QImage> m_images;
    std::unordered_map<std::string, QImage> m_resources;
    std::unordered_map<std::size_t, core::ImageObject> m_procedural;
    std::size_t m_pending = 0;
    std::uint64_t m_generation = 0;
    bool m_ready = false;
    bool m_offOwnerThread = true;
};

} // namespace actcraft::render
