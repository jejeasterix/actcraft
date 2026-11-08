#pragma once

#include <actcraft/core/model.h>

#include <cstdint>
#include <vector>

namespace actcraft::features {

// Session de reconnaissance (F-295, F-296) : les traits versés s'accumulent ; quand le délai s'est écoulé
// depuis le DERNIER trait, ils sont rendus ensemble (un mot, une forme), une seule fois.
class RecognitionSession
{
public:
    static constexpr int kMaxDelayMs = 5000;
    // Délai par défaut 1 s, bornes 0 à 5 s : réglage embarqué relevé par L7-T0 (la spécification disait 5 s,
    // 1 à 5).
    static constexpr int kDefaultDelayMs = 1000;
    explicit RecognitionSession(int delayMs = kDefaultDelayMs);
    void setDelayMs(int delayMs) noexcept; // borné à [0, kMaxDelayMs]
    int delayMs() const noexcept;
    void addStroke(core::ObjectId id, std::int64_t nowNs);
    std::vector<core::ObjectId> due(std::int64_t nowNs); // mûrs : rendus puis oubliés
    const std::vector<core::ObjectId>& pending() const noexcept;
    void clear() noexcept;
    // Traits convertis par une autre voie (la sélection) : retirés de l'attente, le délai des autres tient.
    void remove(const std::vector<core::ObjectId>& ids);

private:
    int m_delayMs = 0;
    std::vector<core::ObjectId> m_pending;
    std::int64_t m_lastNs = 0;
};

} // namespace actcraft::features
