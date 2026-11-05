#include <actcraft/core/spellcheck.h>

namespace actcraft::core {

std::size_t loadUserDictionary(SpellEngine&, const std::string&)
{
    return 0;
}
bool addToUserDictionary(SpellEngine&, const std::string&, const std::string&)
{
    return false;
}
SpellChecker::SpellChecker(SpellEngine& engine, SpellRules rules) : m_engine(engine), m_rules(rules) {}
std::optional<SpellHit> SpellChecker::next(const Document&)
{
    m_page = m_object = m_offset = 0;
    (void)&m_engine;
    return std::nullopt;
}
void SpellChecker::ignore() {}
void SpellChecker::ignoreAlways(const std::string&) {}
void SpellChecker::restart() {}
std::unique_ptr<UndoCommand> SpellChecker::replace(const Document&, const SpellHit&, const std::string&) const
{
    return nullptr;
}

} // namespace actcraft::core
