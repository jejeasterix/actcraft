#include <actcraft/features/strokeguide.h>

#include <algorithm>
#include <cmath>

namespace actcraft::features {

void GuideSet::add(StrokeGuide* guide)
{
    if (guide && std::find(m_guides.begin(), m_guides.end(), guide) == m_guides.end()) {
        m_guides.push_back(guide);
    }
}

void GuideSet::remove(StrokeGuide* guide)
{
    m_guides.erase(std::remove(m_guides.begin(), m_guides.end(), guide), m_guides.end());
    if (m_active == guide) {
        m_active = nullptr;
    }
}

void GuideSet::setTolerance(double pagePixels) noexcept
{
    m_tolerance = pagePixels;
}

input::Sample GuideSet::filter(const input::Sample& sample)
{
    // Le guide n'est choisi qu'à l'appui ; le déplacement et le relâché suivent ce guide-là, sans jamais en
    // chercher un autre : sinon le trait sauterait d'un bord à l'autre (ou sortirait du guide) en cours de
    // route.
    switch (sample.phase) {
    case input::Phase::Press: {
        m_active = nullptr;
        double best = 0;
        input::Sample result = sample;
        for (StrokeGuide* guide : m_guides) {
            const auto snapped = guide->capture(sample, m_tolerance);
            if (!snapped) {
                continue;
            }
            const double d = std::hypot(snapped->x - sample.x, snapped->y - sample.y);
            if (!m_active || d < best) {
                m_active = guide;
                best = d;
                result = *snapped;
            }
        }
        return result; // chaque guide garde le bord de son propre capture() : rien à re-saisir
    }
    case input::Phase::Move:
        return m_active ? m_active->follow(sample) : sample;
    case input::Phase::Release: {
        // Projeté comme le dernier déplacement : un relâché synthétisé au dernier point
        // (PageView::finishStroke) retombe sur le point déjà versé et n'en ajoute aucun.
        const input::Sample out = m_active ? m_active->follow(sample) : sample;
        m_active = nullptr;
        return out;
    }
    }
    return sample;
}

bool GuideSet::guiding() const noexcept
{
    return m_active != nullptr;
}

std::optional<input::Sample> GuideSet::snapPreview(const input::Sample& hover)
{
    // capture() note le bord saisi dans le guide : pendant un trait guidé, l'appeler changerait le bord que
    // suit follow(). Sortie avant toute capture, donc.
    if (m_active) {
        return std::nullopt;
    }
    std::optional<input::Sample> best;
    double bestDistance = 0;
    for (StrokeGuide* guide : m_guides) {
        const auto snapped = guide->capture(hover, m_tolerance);
        if (!snapped) {
            continue;
        }
        const double d = std::hypot(snapped->x - hover.x, snapped->y - hover.y);
        if (!best || d < bestDistance) {
            best = snapped;
            bestDistance = d;
        }
    }
    return best;
}

} // namespace actcraft::features
