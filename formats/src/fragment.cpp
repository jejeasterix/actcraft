#include <actcraft/formats/fragment.h>

namespace actcraft::formats {

bool writeFragment(const core::Document& /*document*/, int /*page*/, const std::vector<core::ObjectId>& /*objects*/,
                   QIODevice& /*out*/, QString& error)
{
    error = QStringLiteral("non implémenté");
    return false;
}

std::optional<Fragment> readFragment(QIODevice& /*in*/, QString& error)
{
    error = QStringLiteral("non implémenté");
    return std::nullopt;
}

std::vector<core::PageObject> prepareInsertion(const Fragment& /*fragment*/, core::Document& /*target*/)
{
    return {};
}

} // namespace actcraft::formats
