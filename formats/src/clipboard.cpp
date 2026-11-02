#include <actcraft/formats/clipboard.h>

namespace actcraft::formats {

// Talon L5-T1 : rempli par L5-T3.
QByteArray encodeClipboard(const ClipboardPayload& payload)
{
    static_cast<void>(payload);
    return {};
}

// Talon L5-T1 : rempli par L5-T3.
std::optional<ClipboardPayload> decodeClipboard(const QByteArray& bytes)
{
    static_cast<void>(bytes);
    return std::nullopt;
}

// Talon L5-T1 : rempli par L5-T3.
std::map<std::string, std::string> resourcesFor(const core::Document& document,
                                                const std::vector<core::PageObject>& objects)
{
    static_cast<void>(document);
    static_cast<void>(objects);
    return {};
}

} // namespace actcraft::formats
