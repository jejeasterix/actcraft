#pragma once

#include <actcraft/core/document.h>

#include <cstddef>

namespace actcraft::core {

// Indice de page présent dans le document ? (en-tête interne de core/src)
inline bool validPage(const Document& document, int page) noexcept
{
    return page >= 0 && static_cast<std::size_t>(page) < document.pageCount();
}

} // namespace actcraft::core
