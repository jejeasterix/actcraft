#include <actcraft/core/spellcheck.h>

#include <hunspell.hxx>

#include <QFileInfo>

#include <memory>

namespace actcraft::core {

namespace {

class HunspellEngine final : public SpellEngine
{
public:
    HunspellEngine(const std::string& aff, const std::string& dic) : m_hunspell(aff.c_str(), dic.c_str()) {}

    bool isCorrect(const std::string& word) override { return m_hunspell.spell(word); }
    std::vector<std::string> suggest(const std::string& word) override { return m_hunspell.suggest(word); }
    void addWord(const std::string& word) override { m_hunspell.add(word); }

private:
    Hunspell m_hunspell;
};

} // namespace

std::unique_ptr<SpellEngine> makeHunspellEngine(const std::string& affPath, const std::string& dicPath)
{
    // Hunspell ne signale pas un fichier absent : on le vérifie avant.
    if (!QFileInfo(QString::fromStdString(affPath)).isFile() || !QFileInfo(QString::fromStdString(dicPath)).isFile()) {
        return nullptr;
    }
    return std::make_unique<HunspellEngine>(affPath, dicPath);
}

} // namespace actcraft::core
