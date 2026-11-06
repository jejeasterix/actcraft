#include <actcraft/ui/pagehistory.h>

#include <iterator>

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

int PageHistory::back()
{
    if (m_pages.empty() || m_position == 0) {
        return -1;
    }
    --m_position;
    return m_pages[m_position];
}

int PageHistory::forward()
{
    if (m_pages.empty() || m_position + 1 >= m_pages.size()) {
        return -1;
    }
    ++m_position;
    return m_pages[m_position];
}

void PageHistory::clear()
{
    m_pages.clear();
    m_position = 0;
}

} // namespace actcraft::ui
