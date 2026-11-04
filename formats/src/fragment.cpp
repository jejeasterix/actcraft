#include <actcraft/core/clipboardedits.h>
#include <actcraft/core/pageedits.h>
#include <actcraft/formats/clipboard.h>
#include <actcraft/formats/fragment.h>
#include <actcraft/formats/nativefile.h>

#include <QByteArray>

#include <cstddef>
#include <string>
#include <utility>

namespace actcraft::formats {

namespace {

// Ressources que citent `objects` (resourcesFor, A10 : aucun second calcul), copiées de `from` dans `to` avec
// leur extension ; une clé déjà dans `to` n'est pas recopiée.
void copyCitedResources(const core::Document& from, const std::vector<core::PageObject>& objects,
                        core::Document& to)
{
    for (const auto& [key, bytes] : resourcesFor(from, objects)) {
        if (to.resource(key) != nullptr)
            continue;
        const core::Resource* source = from.resource(key);
        to.addResource(QByteArray::fromStdString(bytes), source ? source->extension : std::string("bin"));
    }
}

} // namespace

bool writeFragment(const core::Document& document, int page, const std::vector<core::ObjectId>& objects,
                   QIODevice& out, QString& error)
{
    if (page < 0 || static_cast<std::size_t>(page) >= document.pageCount()) {
        error = QStringLiteral("fragment : la page %1 n'existe pas").arg(page);
        return false;
    }
    const core::Page& source = document.page(static_cast<std::size_t>(page));
    std::vector<core::PageObject> chosen = core::closureForCopy(source, objects);
    if (chosen.empty()) {
        error = QStringLiteral("fragment : aucun des objets choisis n'est sur la page %1").arg(page);
        return false;
    }

    // Un document neuf, jamais celui d'origine élagué : l'écrivain natif écrit toute sa table de ressources
    // et ses entrées inconnues (nativefile.cpp), celles d'un autre cours comprises.
    core::Document fragment(source.width, source.height);
    copyCitedResources(document, chosen, fragment);
    fragment.page(0).objects = std::move(chosen);
    fragment.refreshNextId();

    QString encodeError;
    const QByteArray archive = encodeActcraft(fragment, {}, &encodeError);
    if (archive.isEmpty()) {
        error = QStringLiteral("fragment : %1").arg(encodeError);
        return false;
    }
    if (out.write(archive) != archive.size()) {
        error = QStringLiteral("fragment : écriture incomplète (%1)").arg(out.errorString());
        return false;
    }
    return true;
}

std::optional<Fragment> readFragment(QIODevice& in, QString& error)
{
    if (!in.isSequential() && in.size() > kMaxArchiveSize) {
        error = QStringLiteral("fragment : fichier de plus de %1 octets").arg(kMaxArchiveSize);
        return std::nullopt;
    }
    DecodeResult decoded = decodeActcraft(in.readAll(), ArchiveLimits{}, Thumbnails::Skip);
    if (!decoded.ok) {
        error = decoded.error;
        return std::nullopt;
    }
    if (decoded.document.pageCount() != 1) {
        error = QStringLiteral("fragment : %1 pages, une seule attendue").arg(decoded.document.pageCount());
        return std::nullopt;
    }

    Fragment fragment;
    fragment.page = std::move(decoded.document.page(0));
    for (const auto& entry : decoded.document.resources())
        fragment.resources.addResource(entry.second.data, entry.second.extension);
    return fragment;
}

std::vector<core::PageObject> prepareInsertion(const Fragment& fragment, core::Document& target)
{
    core::Page fresh = core::withFreshIds(target, fragment.page);
    for (const core::PageObject& object : fresh.objects) {
        if (object.common.id == 0)
            return {}; // identifiants épuisés : la copie n'est pas à insérer (withFreshIds)
    }
    copyCitedResources(fragment.resources, fresh.objects, target);
    return std::move(fresh.objects);
}

} // namespace actcraft::formats
