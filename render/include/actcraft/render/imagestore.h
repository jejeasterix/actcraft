#pragma once

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QThreadPool>

#include <atomic>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace actcraft::render {

// Génère les images d'une page sur des fils secondaires (règle de fluidité 1) ; ready() quand toutes sont
// prêtes. image() est sûre depuis le fil de rendu.
class ImageStore : public QObject
{
    Q_OBJECT

public:
    explicit ImageStore(QObject* parent = nullptr);
    ~ImageStore() override;

    // Images de la page sur des fils secondaires : synthétiques par synthesisKey, ressources de `resources`
    // par clé (images, espaces réservés des médias, image de fond), une fois chacune, et variantes à couleur
    // transparente. ready() quand tout est prêt. Sans `resources`, les ressources restent absentes (images
    // nulles).
    void generate(const core::Page& page, const core::Document* resources = nullptr);
    // Ressource décodée par sa clé, sans masque ; nulle si absente ou illisible. Sûre depuis le fil de rendu.
    QImage resourceImage(const std::string& key) const;
    // Image à dessiner pour cet objet : ressource (variante masquée si couleur transparente) ou image
    // synthétique de même synthesisKey (L3-C13 : plus par indice d'objet, qu'un soulèvement décale).
    QImage objectImage(const core::ImageObject& object) const;
    // Clé de la ressource, suivie de « #aarrggbb » si l'objet a une couleur transparente.
    static std::string variantKey(const core::ImageObject& object);
    // Vrai si les images de la dernière génération suffisent à `page` (chaque image synthétique par sa
    // synthesisKey, chaque ressource voulue par sa clé, quel que soit son indice) : la page fixe se
    // reconstruit alors sans redécoder (L2-T14, L3-C13).
    bool covers(const core::Page& page, const core::Document* resources) const;
    bool isReady() const noexcept;
    // Image synthétique de l'objet, par sa synthesisKey ; nulle si absente. Sûre depuis le fil de rendu.
    QImage image(const core::ImageObject& object) const;
    std::size_t imageCount() const; // images synthétiques distinctes (une par synthesisKey)
    // Crochet de test : vrai si toutes les images de la dernière génération ont été synthétisées hors du fil
    // de l'objet.
    bool generatedOffOwnerThread() const noexcept;
    // Crochets de test : tâches de décodage ou de synthèse commencées depuis la création (une tâche annulée
    // avant de commencer n'est pas comptée), et attente de la fin des tâches commencées.
    std::size_t startedTasks() const noexcept;
    bool waitForIdle(int msecs);

signals:
    void ready();

private:
    void onImageReady(std::uint64_t generation, bool offOwnerThread);

    QThreadPool m_pool;
    mutable QMutex m_mutex;
    std::unordered_map<std::string, QImage> m_images; // synthétiques, par synthesisKey
    std::unordered_map<std::string, QImage> m_resources;
    std::unordered_set<std::string> m_procedural; // synthesisKey demandées à la dernière génération
    std::size_t m_pending = 0;
    std::uint64_t m_generation = 0;
    bool m_ready = false;
    bool m_offOwnerThread = true;
    std::atomic<std::size_t> m_started{0};
};

} // namespace actcraft::render
