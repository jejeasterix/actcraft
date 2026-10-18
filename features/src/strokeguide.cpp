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

GuideSet::Nearest GuideSet::nearest(const input::Sample& sample) const
{
    Nearest best;
    double bestDistance = 0;
    for (StrokeGuide* guide : m_guides) {
        const auto projected = guide->project(sample, m_tolerance);
        if (!projected) {
            continue;
        }
        const double d = std::hypot(projected->x - sample.x, projected->y - sample.y);
        if (!best.guide || d < bestDistance) {
            best = {guide, *projected};
            bestDistance = d;
        }
    }
    return best;
}

input::Sample GuideSet::filter(const input::Sample& sample)
{
    // Le guide n'est choisi qu'à l'appui ; le déplacement et le relâché suivent ce guide-là, sans jamais en
    // chercher un autre : sinon le trait sauterait d'un bord à l'autre (ou sortirait du guide) en cours de
    // route.
    switch (sample.phase) {
    case input::Phase::Press: {
        m_active = nullptr;
        // Seul le guide choisi mémorise son bord : les autres, seulement interrogés, ne changent pas.
        StrokeGuide* const chosen = nearest(sample).guide;
        if (!chosen) {
            return sample;
        }
        const auto seized = chosen->capture(sample, m_tolerance); // la projection que project() a trouvée
        if (!seized) {
            return sample;
        }
        m_active = chosen;
        return *seized;
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

std::optional<input::Sample> GuideSet::snapPreview(const input::Sample& hover) const
{
    if (m_active) {
        return std::nullopt;
    }
    const Nearest best = nearest(hover);
    if (!best.guide) {
        return std::nullopt;
    }
    return best.sample;
}

} // namespace actcraft::features
