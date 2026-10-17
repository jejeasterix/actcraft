#include <actcraft/core/document.h>

#include <QCryptographicHash>
#include <QtLogging>

#include <algorithm>
#include <cassert>
#include <limits>
#include <set>
#include <utility>

namespace actcraft::core {

namespace {

// Dernier identifiant, jamais attribué : marque l'espace épuisé (allocateId rend alors 0).
constexpr ObjectId kExhausted = std::numeric_limits<ObjectId>::max();

// FNV-1a 64 bits sur les 8 octets de `value`, poids faible d'abord (même ordre que model.cpp).
std::uint64_t mix(std::uint64_t hash, std::uint64_t value) noexcept
{
    for (unsigned i = 0; i < 8; ++i) {
        hash ^= (value >> (8u * i)) & 0xFFu;
        hash *= 0x100000001B3ull;
    }
    return hash;
}

std::uint64_t mixText(std::uint64_t hash, const std::string& text) noexcept
{
    hash = mix(hash, static_cast<std::uint64_t>(text.size()));
    for (const char c : text) {
        hash ^= static_cast<std::uint8_t>(c);
        hash *= 0x100000001B3ull;
    }
    return hash;
}

Page blankPage(double width, double height)
{
    Page page;
    page.width = width;
    page.height = height;
    return page;
}

} // namespace

Document::Document() : m_pages(1) {}

Document::Document(double pageWidth, double pageHeight) : m_pages{blankPage(pageWidth, pageHeight)} {}

Document::Document(std::vector<Page> pages) : m_pages(std::move(pages))
{
    if (m_pages.empty()) {
        m_pages.emplace_back();
    }
    refreshNextId();
}

std::size_t Document::pageCount() const noexcept
{
    return m_pages.size();
}

const Page& Document::page(std::size_t index) const
{
    assert(index < m_pages.size());
    return m_pages[index];
}

Page& Document::page(std::size_t index)
{
    assert(index < m_pages.size());
    return m_pages[index];
}

std::size_t Document::appendPage()
{
    m_pages.push_back(blankPage(m_pages.back().width, m_pages.back().height));
    return m_pages.size() - 1;
}

void Document::insertPage(std::size_t index, Page page)
{
    assert(index <= m_pages.size());
    m_pages.insert(m_pages.begin() + static_cast<std::ptrdiff_t>(index), std::move(page));
}

Page Document::takePage(std::size_t index)
{
    assert(index < m_pages.size() && m_pages.size() > 1);
    Page taken = std::move(m_pages[index]);
    m_pages.erase(m_pages.begin() + static_cast<std::ptrdiff_t>(index));
    return taken;
}

ObjectId Document::allocateId() noexcept
{
    if (m_nextId == kExhausted) {
        qWarning("Document : identifiants d'objets épuisés");
        return 0;
    }
    return m_nextId++;
}

ObjectId Document::nextId() const noexcept
{
    return m_nextId;
}

void Document::refreshNextId() noexcept
{
    ObjectId largest = 0;
    for (const Page& p : m_pages) {
        for (const PageObject& o : p.objects) {
            largest = std::max(largest, o.common.id);
        }
    }
    reserveIdsBelow(largest == kExhausted ? kExhausted : largest + 1);
}

void Document::reserveIdsBelow(ObjectId next) noexcept
{
    m_nextId = std::max(m_nextId, next);
}

const std::string& Document::unknownKeys() const noexcept
{
    return m_unknownKeys;
}

void Document::setUnknownKeys(std::string keys)
{
    m_unknownKeys = std::move(keys);
}

std::string Document::addResource(QByteArray data, std::string extension)
{
    // Minuscules ASCII à la main : std::tolower suit la locale du processus (Qt appelle setlocale(LC_ALL, "")
    // sous Unix ; en locale turque mono-octet, « I » ne devient pas « i »).
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; });
    // Extension sûre : elle forme un nom d'entrée d'archive (« resources/<clé>.<ext> ») et, au lot 5, un nom
    // de fichier exporté. [a-z0-9]{1,8}, sinon « bin » (relecture du plan, tour 1 : glissement de chemin).
    const bool safe = !extension.empty() && extension.size() <= 8 &&
                      std::all_of(extension.begin(), extension.end(),
                                  [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'); });
    if (!safe) {
        extension = "bin";
    }
    std::string key = sha256Hex(data);
    Resource resource;
    resource.data = std::move(data);
    resource.extension = std::move(extension);
    m_resources.try_emplace(key, std::move(resource));
    return key;
}

void Document::setResourceUnknownKeys(const std::string& key, std::string keys)
{
    if (const auto it = m_resources.find(key); it != m_resources.end()) {
        it->second.unknownKeys = std::move(keys);
    }
}

const std::map<std::string, ArchiveEntry>& Document::unknownEntries() const noexcept
{
    return m_unknownEntries;
}

void Document::setUnknownEntry(std::string name, QByteArray data)
{
    ArchiveEntry entry;
    entry.sha256 = sha256Hex(data);
    entry.data = std::move(data);
    m_unknownEntries[std::move(name)] = std::move(entry);
}

const Resource* Document::resource(const std::string& key) const noexcept
{
    const auto it = m_resources.find(key);
    return it == m_resources.end() ? nullptr : &it->second;
}

const std::map<std::string, Resource>& Document::resources() const noexcept
{
    return m_resources;
}

const std::string& Document::title() const noexcept
{
    return m_title;
}

void Document::setTitle(std::string title)
{
    m_title = std::move(title);
}

std::size_t nextPageIndex(Document& document, std::size_t current)
{
    if (current + 1 < document.pageCount()) {
        return current + 1;
    }
    return document.appendPage();
}

std::size_t formerPageIndex(std::size_t current) noexcept
{
    return current == 0 ? 0 : current - 1;
}

std::string sha256Hex(const QByteArray& data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex().toStdString();
}

bool hasUniqueIds(const Document& document)
{
    std::set<ObjectId> seen;
    for (std::size_t i = 0; i < document.pageCount(); ++i) {
        for (const PageObject& o : document.page(i).objects) {
            if (o.common.id == 0 || !seen.insert(o.common.id).second) {
                return false;
            }
        }
    }
    return true;
}

std::uint64_t fingerprint(const Document& document) noexcept
{
    std::uint64_t hash = 0xCBF29CE484222325ull;
    hash = mix(hash, static_cast<std::uint64_t>(document.pageCount()));
    for (std::size_t i = 0; i < document.pageCount(); ++i) {
        hash = mix(hash, fingerprint(document.page(i)));
    }
    hash = mixText(hash, document.title());
    hash = mix(hash, static_cast<std::uint64_t>(document.resources().size()));
    for (const auto& [key, resource] : document.resources()) {
        hash = mixText(hash, key);
        hash = mixText(hash, resource.extension);
        hash = mixText(hash, resource.unknownKeys);
    }
    // En dernier et seulement s'il y en a (L2-C8) : clés inconnues du document, entrées d'archive inconnues.
    if (!document.unknownKeys().empty() || !document.unknownEntries().empty()) {
        hash = mixText(hash, document.unknownKeys());
        hash = mix(hash, static_cast<std::uint64_t>(document.unknownEntries().size()));
        for (const auto& [name, entry] : document.unknownEntries()) {
            hash = mixText(hash, name);
            hash = mixText(hash, entry.sha256);
        }
    }
    return hash;
}

} // namespace actcraft::core
