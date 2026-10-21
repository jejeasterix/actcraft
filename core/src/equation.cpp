#include <actcraft/core/equation.h>

namespace actcraft::core {

std::optional<EquationNode> parseMathml(const std::string& mathml, QString& error)
{
    (void)mathml;
    error = QStringLiteral("MathML : lecture pas encore écrite");
    return std::nullopt;
}

std::string toMathml(const EquationNode& root)
{
    (void)root;
    return {};
}

} // namespace actcraft::core
