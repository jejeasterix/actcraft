#pragma once

#include <actcraft/core/model.h>

#include <QByteArray>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace actcraft::core {

// Ressource d'un document (image, son, vidéo) : octets d'origine, jamais recompressés (DA3).
struct Resource
{
    QByteArray data;
    std::string extension;   // [a-z0-9]{1,8}, sans point : « png », « jpg », « mp3 »… ; sinon « bin »
    std::string unknownKeys; // clés de sa déclaration inconnues de cette version (L2-C8)
};

// Entrée d'archive que cette version ne connaît pas (version plus récente, L2-C8) : octets d'origine,
// réécrits tels quels sous le même nom ; `sha256` (calculé à l'ajout) les résume dans l'empreinte.
struct ArchiveEntry
{
    QByteArray data;
    std::string sha256;
};

// Document (DA2, décision 15) : une suite de pages, jamais vide ; ressources par empreinte SHA-256 ;
// identifiants d'objets uniques dans tout le document. Les modifications passent par la pile d'annulation
// (undo.h, edits.h) ; ces méthodes sont ses briques.
class Document
{
public:
    Document(); // une page vierge de 1024 × 768
    Document(double pageWidth, double pageHeight);
    // Liste vide : une page vierge (un document n'est jamais vide).
    explicit Document(std::vector<Page> pages);

    std::size_t pageCount() const noexcept;
    // Exige index < pageCount() (assert).
    const Page& page(std::size_t index) const;
    Page& page(std::size_t index);
    // Ajoute à la fin une page vierge de la taille de la dernière ; retourne son indice.
    std::size_t appendPage();
    // Exige index <= pageCount().
    void insertPage(std::size_t index, Page page);
    // Retire et rend la page `index` ; exige pageCount() > 1.
    Page takePage(std::size_t index);

    // Identifiant neuf, de 1 à 2^64 − 2. Le dernier (2^64 − 1) n'est jamais attribué : nextId() == lui,
    // l'espace est épuisé, et allocateId() rend 0 (« aucun ») avec un avertissement, jamais un identifiant
    // déjà pris.
    ObjectId allocateId() noexcept;
    ObjectId nextId() const noexcept;
    // Le prochain identifiant dépasse tous ceux des pages (après une lecture ou une page posée telle quelle).
    // Borné à 2^64 − 1 (épuisé) si une page porte déjà ce dernier identifiant.
    void refreshNextId() noexcept;
    // Le prochain identifiant vaut au moins `next` ; jamais en arrière.
    void reserveIdsBelow(ObjectId next) noexcept;

    // Clé = SHA-256 des octets en hexadécimal minuscule (64 caractères) ; même contenu, même clé, une seule
    // copie.
    std::string addResource(QByteArray data, std::string extension);
    // nullptr si la clé est inconnue.
    const Resource* resource(const std::string& key) const noexcept;
    const std::map<std::string, Resource>& resources() const noexcept;
    // Sans effet si la clé est inconnue.
    void setResourceUnknownKeys(const std::string& key, std::string keys);

    const std::string& title() const noexcept;
    void setTitle(std::string title);

    // Clés du document que cette version ne connaît pas (comme PageObject::unknownKeys) ; vide : aucune.
    const std::string& unknownKeys() const noexcept;
    void setUnknownKeys(std::string keys);
    // Entrées d'archive inconnues, par nom ; formats/ ne garde que des noms sûrs, hors des siens.
    const std::map<std::string, ArchiveEntry>& unknownEntries() const noexcept;
    void setUnknownEntry(std::string name, QByteArray data);

private:
    std::vector<Page> m_pages;
    std::map<std::string, Resource> m_resources;
    ObjectId m_nextId = 1;
    std::string m_title;
    std::string m_unknownKeys;
    std::map<std::string, ArchiveEntry> m_unknownEntries;
};

// « Page suivante » (F-125) : indice suivant ; sur la dernière page, ajoute d'abord une page vierge.
std::size_t nextPageIndex(Document& document, std::size_t current);
// « Page précédente » : indice précédent, borné à 0.
std::size_t formerPageIndex(std::size_t current) noexcept;

// SHA-256 en hexadécimal minuscule.
std::string sha256Hex(const QByteArray& data);
// Vrai si chaque objet a un identifiant non nul, unique dans tout le document.
bool hasUniqueIds(const Document& document);

// Empreinte FNV-1a 64 bits : nombre de pages, empreinte de chaque page, titre, ressources (clé et extension ;
// la clé résume les octets ; clés inconnues de leur déclaration), puis, s'il y en a, les clés inconnues du
// document et les entrées d'archive inconnues (nom et SHA-256). Le prochain identifiant n'y entre pas : un
// document relu a la même empreinte.
std::uint64_t fingerprint(const Document& document) noexcept;

} // namespace actcraft::core
