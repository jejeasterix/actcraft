#include <actcraft/core/spellcheck.h>

#include <QDir>
#include <QFileInfo>

#include <hunspell.hxx>
#include <memory>

namespace actcraft::core {

namespace {

// Hunspell ouvre ses fichiers par std::ifstream::open(const char*) : sous Windows, la page de code ANSI perd
// les lettres qui n'y sont pas (profil « C:\Users\Hélène ») et le moteur reste vide. Un chemin préfixé `\\?\`
// est lu en UTF-8 et ouvert en chemin large (third_party/hunspell/csutil.cxx, myopen) ; cette forme veut un
// chemin absolu, nettoyé, en séparateurs natifs (`\\?\UNC\` pour un partage réseau).
std::string hunspellPath(const std::string& path)
{
#ifdef Q_OS_WIN
    const QString absolute =
        QDir::toNativeSeparators(QDir::cleanPath(QFileInfo(QString::fromStdString(path)).absoluteFilePath()));
    const QString prefixed = absolute.startsWith(QLatin1String("\\\\"))
                                 ? QStringLiteral("\\\\?\\UNC\\") + absolute.mid(2)
                                 : QStringLiteral("\\\\?\\") + absolute;
    return prefixed.toStdString();
#else
    return path;
#endif
}

class HunspellEngine final : public SpellEngine
{
public:
    HunspellEngine(const std::string& aff, const std::string& dic)
        : m_hunspell(hunspellPath(aff).c_str(), hunspellPath(dic).c_str())
    {}

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
    if (!QFileInfo(QString::fromStdString(affPath)).isFile() ||
        !QFileInfo(QString::fromStdString(dicPath)).isFile()) {
        return nullptr;
    }
    return std::make_unique<HunspellEngine>(affPath, dicPath);
}

} // namespace actcraft::core
