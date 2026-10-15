#include <actcraft/core/document.h>

#include <cassert>
#include <utility>

namespace actcraft::core {

namespace {

// FNV-1a 64 bits sur les 8 octets de `value`, poids faible d'abord (même ordre que model.cpp).
std::uint64_t mix(std::uint64_t hash, std::uint64_t value) noexcept
{
    for (unsigned i = 0; i < 8; ++i) {
        hash ^= (value >> (8u * i)) & 0xFFu;
        hash *= 0x100000001B3ull;
    }
    return hash;
}

} // namespace

Document::Document() : m_pages(1) {}

std::size_t Document::pageCount() const noexcept
{
    return m_pages.size();
}

const Page& Document::page(std::size_t index) const
{
    assert(index < m_pages.size());
    return m_pages[index];
}

Page& Document::page(std::size_t index)
{
    assert(index < m_pages.size());
    return m_pages[index];
}

std::size_t Document::appendPage()
{
    Page blank;
    blank.width = m_pages.back().width;
    blank.height = m_pages.back().height;
    m_pages.push_back(std::move(blank));
    return m_pages.size() - 1;
}

std::size_t nextPageIndex(Document& document, std::size_t current)
{
    if (current + 1 < document.pageCount()) {
        return current + 1;
    }
    return document.appendPage();
}

std::size_t previousPageIndex(std::size_t current) noexcept
{
    return current == 0 ? 0 : current - 1;
}

std::uint64_t fingerprint(const Document& document) noexcept
{
    std::uint64_t hash = 0xCBF29CE484222325ull;
    hash = mix(hash, static_cast<std::uint64_t>(document.pageCount()));
    for (std::size_t i = 0; i < document.pageCount(); ++i) {
        hash = mix(hash, fingerprint(document.page(i)));
    }
    return hash;
}

} // namespace actcraft::core
