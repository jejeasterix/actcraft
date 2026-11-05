#pragma once
// Vérificateur d'orthographe (F-297, L6-T33). QtCore seulement ; le moteur est une interface (SpellEngine).
#include <actcraft/core/document.h>
#include <actcraft/core/undo.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace actcraft::core {

// Règles d'ignorance (D5, F-930). Clés du profil, section « spelling » : ignore-capitalized,
// ignore-uppercase, ignore-digits, ignore-mixed-case, ignore-addresses (défaut faux) ; flag-repeated (défaut
// vrai).
struct SpellRules
{
    bool ignoreCapitalized = false; // « Paris » : initiale majuscule, le reste en minuscules
    bool ignoreUppercase = false;   // « NASA » : au moins deux lettres, toutes en majuscules
    bool ignoreDigits = false;      // « abc123 » : un chiffre dans le mot
    bool ignoreMixedCase =
        false; // « iPhone », « McDo » : une majuscule après la première lettre et une minuscule
    bool ignoreAddresses = false; // « a@b.fr », « https://… », « www.… »
    bool flagRepeated = true;     // « le le » : le mot qui répète le précédent est signalé
};

class SpellEngine
{
public:
    virtual ~SpellEngine() = default;
    virtual bool isCorrect(const std::string& word) = 0;
    virtual std::vector<std::string> suggest(const std::string& word) = 0;
    // Ajoute le mot au moteur pour la durée de sa vie (rien n'est écrit sur disque).
    virtual void addWord(const std::string& word) = 0;
};

// Moteur Hunspell sur un couple .aff / .dic (UTF-8). nullptr si l'un des deux fichiers est illisible.
std::unique_ptr<SpellEngine> makeHunspellEngine(const std::string& affPath, const std::string& dicPath);

// Dictionnaire de l'utilisateur : un mot par ligne, dans le fichier que l'appelant désigne (jamais sous
// third_party/). Lire un fichier absent n'ajoute rien. Rend le nombre de mots ajoutés au moteur.
std::size_t loadUserDictionary(SpellEngine& engine, const std::string& path);
// « Ajouter au dictionnaire » : le mot entre dans le moteur et, s'il n'y est pas déjà, à la fin du fichier
// (créé au besoin). Rend faux si le fichier ne peut pas être écrit (le moteur garde le mot) ou si le mot est
// vide.
bool addToUserDictionary(SpellEngine& engine, const std::string& path, const std::string& word);

// Mot signalé. start et length sont en unités UTF-16 du texte brut du texte riche (balises retirées, entités
// décodées, saut de ligne à chaque bloc) ; page est l'indice de la page (0 : la première).
struct SpellHit
{
    int page = 0;
    ObjectId object = 0;
    std::size_t start = 0;
    std::size_t length = 0;
    std::string word;
    bool repeated = false; // vrai : mot correct répétant le précédent ; faux : mot inconnu du moteur
};

class SpellChecker
{
public:
    SpellChecker(SpellEngine& engine, SpellRules rules);

    // Premier mot à signaler à partir du curseur : pages depuis la première, objets texte dans l'ordre de la
    // page (couches du fond vers le dessus). Rend le même mot tant que ni ignore(), ni ignoreAlways()
    // n'avance le curseur ; std::nullopt à la fin du document.
    std::optional<SpellHit> next(const Document& document);
    // Passe le mot courant (celui que le dernier next() a rendu).
    void ignore();
    // Passe le mot courant et tous ses semblables (casse ignorée) jusqu'à restart() ; liste de la session.
    void ignoreAlways(const std::string& word);
    // Curseur au début de la première page ; la liste « ignorer toujours » est gardée.
    void restart();
    // Remplace le mot par une étape d'annulation (SetTextCommand). nullptr si l'objet a disparu ou n'est plus
    // un texte, ou si le mot n'est plus à cette place.
    std::unique_ptr<UndoCommand> replace(const Document& document, const SpellHit& hit,
                                         const std::string& replacement) const;

private:
    SpellEngine& m_engine;
    SpellRules m_rules;
    std::set<std::string> m_always; // mots en minuscules
    std::size_t m_page = 0;
    std::size_t m_object = 0; // indice dans Page::objects
    std::size_t m_offset = 0; // dans le texte brut de l'objet
    std::optional<SpellHit> m_current;
};

} // namespace actcraft::core
