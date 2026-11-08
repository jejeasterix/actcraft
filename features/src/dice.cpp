#include <actcraft/features/dice.h>

#include <algorithm>
#include <numeric>

namespace actcraft::features {

Dice::Dice(quint32 seed) : m_generator(seed) {}

void Dice::setCount(int count) noexcept
{
    m_count = std::clamp(count, kMinCount, kMaxCount);
}

int Dice::count() const noexcept
{
    return m_count;
}

std::vector<int> Dice::roll()
{
    m_faces.clear();
    for (int i = 0; i < m_count; ++i)
        m_faces.push_back(static_cast<int>(m_generator.bounded(1, 7)));
    return m_faces;
}

const std::vector<int>& Dice::faces() const noexcept
{
    return m_faces;
}

int Dice::total() const noexcept
{
    return std::accumulate(m_faces.begin(), m_faces.end(), 0);
}

int Dice::animationMs(int seconds) noexcept
{
    return std::clamp(seconds, 3, 11) * 1000;
}

} // namespace actcraft::features
