#include <actcraft/ui/pagehistory.h>

#include <iterator>
#include <utility>

namespace actcraft::ui {

void PageHistory::visited(int page)
{
    if (page < 0) {
        return;
    }
    if (!m_pages.empty()) {
        if (m_pages[m_position] == page) {
            return;
        }
        m_pages.erase(m_pages.begin() + static_cast<std::ptrdiff_t>(m_position) + 1, m_pages.end());
    }
    m_pages.push_back(page);
    if (m_pages.size() > kLimit) {
        m_pages.erase(m_pages.begin(),
                      m_pages.begin() + static_cast<std::ptrdiff_t>(m_pages.size() - kLimit));
    }
    m_position = m_pages.size() - 1;
}

int PageHistory::back(int pageCount)
{
    for (std::size_t i = m_position; !m_pages.empty() && i > 0; --i) {
        if (m_pages[i - 1] < pageCount) {
            m_position = i - 1;
            return m_pages[m_position];
        }
    }
    return -1;
}

int PageHistory::forward(int pageCount)
{
    for (std::size_t i = m_position + 1; i < m_pages.size(); ++i) {
        if (m_pages[i] < pageCount) {
            m_position = i;
            return m_pages[m_position];
        }
    }
    return -1;
}

void PageHistory::pageInserted(int index)
{
    for (int& page : m_pages) {
        if (page >= index) {
            ++page;
        }
    }
}

void PageHistory::pageRemoved(int index)
{
    std::vector<int> kept;
    kept.reserve(m_pages.size());
    std::size_t position = 0; // dernière entrée gardée à la position courante ou avant
    for (std::size_t i = 0; i < m_pages.size(); ++i) {
        int page = m_pages[i];
        if (page == index) {
            continue;
        }
        if (page > index) {
            --page;
        }
        if (kept.empty() || kept.back() != page) {
            kept.push_back(page);
        }
        if (i <= m_position) {
            position = kept.size() - 1;
        }
    }
    m_pages = std::move(kept);
    m_position = m_pages.empty() ? 0 : position;
}

void PageHistory::clear()
{
    m_pages.clear();
    m_position = 0;
}

} // namespace actcraft::ui
