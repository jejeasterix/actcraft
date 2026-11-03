#include <actcraft/export/officeconvert.h>

namespace actcraft::exporting {

namespace detail {
// Couture de test : délai du convertisseur en millisecondes, 0 = 120 s.
void setConvertTimeoutMs(int milliseconds)
{
    static_cast<void>(milliseconds);
}
} // namespace detail

// Talon L5-T1 : rempli par L5-T29.
std::optional<Converter> findConverter()
{
    return std::nullopt;
}

// Talon L5-T1 : rempli par L5-T29.
QFuture<ConvertResult> convertToPdf(const Converter& converter, const QString& presentationPath)
{
    static_cast<void>(converter);
    static_cast<void>(presentationPath);
    return QtFuture::makeReadyValueFuture(ConvertResult{});
}

} // namespace actcraft::exporting
