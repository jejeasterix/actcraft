// Diffusion protégée, règles (L8-T31, F-680, S18) : catégorie de chaque commande touchée, interdictions sans
// exception d'auteur, copie diffusée qui porte les règles sans toucher à la source.
#include <actcraft/core/broadcast.h>
#include <actcraft/core/document.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QtTest>

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using actcraft::core::BroadcastRules;
using actcraft::core::Document;
using actcraft::core::forbids;
using actcraft::core::published;
using actcraft::core::Restriction;
using actcraft::core::restrictionFor;

namespace {

// Identifiants classés Copy sans être restreignables (H8-8 : « restrictable » inchangé dans commands.json).
const std::vector<std::string> kCopyWithoutRestrictable = {"page.copy", "page.cut"};

bool isCopyWithoutRestrictable(const std::string& id)
{
    for (const std::string& known : kCopyWithoutRestrictable) {
        if (known == id) {
            return true;
        }
    }
    return false;
}

// Chaque objet de commands.json qui a un « id » texte, avec son drapeau « restrictable » (faux s'il manque).
void collect(const QJsonValue& value, std::map<std::string, bool>& out)
{
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        for (const QJsonValue& item : array) {
            collect(item, out);
        }
        return;
    }
    if (!value.isObject()) {
        return;
    }
    const QJsonObject object = value.toObject();
    const QJsonValue id = object.value(QStringLiteral("id"));
    if (id.isString()) {
        out[id.toString().toStdString()] = object.value(QStringLiteral("restrictable")).toBool(false);
    }
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (it.value().isArray() || it.value().isObject()) {
            collect(it.value(), out);
        }
    }
}

std::map<std::string, bool> declaredCommands()
{
    QFile file(QStringLiteral(ACTCRAFT_SOURCE_DIR) + QStringLiteral("/actions/data/commands.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QJsonDocument json = QJsonDocument::fromJson(file.readAll());
    std::map<std::string, bool> commands;
    collect(json.isArray() ? QJsonValue(json.array()) : QJsonValue(json.object()), commands);
    return commands;
}

const char* nameOf(std::optional<Restriction> restriction)
{
    if (!restriction) {
        return "aucune";
    }
    switch (*restriction) {
    case Restriction::Copy:
        return "copy";
    case Restriction::Save:
        return "save";
    case Restriction::Print:
        return "print";
    case Restriction::Camera:
        return "camera";
    }
    return "?";
}

BroadcastRules allForbidden()
{
    return BroadcastRules{
        .noCopy = true, .noSave = true, .noPrint = true, .noCamera = true, .author = "Auteur de la classe"};
}

} // namespace

class TstCoreBroadcast : public QObject
{
    Q_OBJECT
private slots:
    void publishedCopyCarriesTheRules()
    {
        const Document source(800.0, 600.0);
        const BroadcastRules rules{
            .noCopy = true, .noSave = false, .noPrint = true, .noCamera = true, .author = "Classe de CM2"};
        const Document copy = published(source, rules);
        QVERIFY(copy.broadcast().noCopy);
        QVERIFY(!copy.broadcast().noSave);
        QVERIFY(copy.broadcast().noPrint);
        QVERIFY(copy.broadcast().noCamera);
        QCOMPARE(copy.broadcast().author, std::string("Classe de CM2"));
        QCOMPARE(copy.pageCount(), source.pageCount());
        for (std::size_t i = 0; i < source.pageCount(); ++i) {
            QCOMPARE(actcraft::core::fingerprint(copy.page(i)), actcraft::core::fingerprint(source.page(i)));
        }
        // Le bloc 0xB9 de l'empreinte : la copie diffusée n'est pas la source.
        QVERIFY(actcraft::core::fingerprint(copy) != actcraft::core::fingerprint(source));
    }

    void originalIsUntouched()
    {
        const Document source(800.0, 600.0);
        const std::uint64_t before = actcraft::core::fingerprint(source);
        const Document copy = published(source, allForbidden());
        QCOMPARE(actcraft::core::fingerprint(source), before);
        QVERIFY(source.broadcast().isDefault());
        QVERIFY(!copy.broadcast().isDefault());
    }

    void rulesApplyToTheAuthorToo()
    {
        // Les règles nomment l'auteur : aucune exception pour lui, chaque interdiction vaut.
        const BroadcastRules all = allForbidden();
        QVERIFY(!all.author.empty());
        for (Restriction r :
             {Restriction::Copy, Restriction::Save, Restriction::Print, Restriction::Camera}) {
            QVERIFY2(forbids(all, r), nameOf(r));
        }
        const std::vector<std::pair<BroadcastRules, Restriction>> single = {
            {BroadcastRules{.noCopy = true, .author = "Auteur"}, Restriction::Copy},
            {BroadcastRules{.noSave = true, .author = "Auteur"}, Restriction::Save},
            {BroadcastRules{.noPrint = true, .author = "Auteur"}, Restriction::Print},
            {BroadcastRules{.noCamera = true, .author = "Auteur"}, Restriction::Camera},
        };
        for (const auto& [rules, only] : single) {
            for (Restriction r :
                 {Restriction::Copy, Restriction::Save, Restriction::Print, Restriction::Camera}) {
                QVERIFY2(forbids(rules, r) == (r == only), nameOf(r));
            }
        }
        const BroadcastRules authorOnly{.author = "Auteur"};
        for (Restriction r :
             {Restriction::Copy, Restriction::Save, Restriction::Print, Restriction::Camera}) {
            QVERIFY2(!forbids(authorOnly, r), nameOf(r));
        }
    }

    void defaultRulesAreOmitted()
    {
        const Document source(800.0, 600.0);
        const Document plain = published(source, BroadcastRules{});
        QVERIFY(plain.broadcast().isDefault());
        QCOMPARE(actcraft::core::fingerprint(plain), actcraft::core::fingerprint(source));
        // Diffuser sans règles une copie protégée retire ses règles (published remplace, ne fusionne pas).
        const Document protectedCopy = published(source, allForbidden());
        const Document again = published(protectedCopy, BroadcastRules{});
        QVERIFY(again.broadcast().isDefault());
        QCOMPARE(actcraft::core::fingerprint(again), actcraft::core::fingerprint(source));
    }

    void categoryOfEveryRestrictableCommand()
    {
        const std::map<std::string, bool> commands = declaredCommands();
        QVERIFY2(commands.size() > 100, "commands.json introuvable ou vide");
        for (const auto& [id, restrictable] : commands) {
            const std::optional<Restriction> category = restrictionFor(id);
            if (restrictable) {
                QVERIFY2(category.has_value(), ("restreignable sans catégorie : " + id).c_str());
            } else if (category.has_value()) {
                // Seules page.copy et page.cut ont une catégorie sans être restreignables (H8-8).
                QVERIFY2(isCopyWithoutRestrictable(id),
                         ("catégorie sans « restrictable » : " + id + " -> " + nameOf(category)).c_str());
            }
        }
    }

    // Test qui discrimine (H8-8) : sous le patron « catégorie lue du drapeau restrictable », page.copy et
    // page.cut n'auraient aucune catégorie et la diffusion protégée laisserait copier les pages.
    void pageCopyAndCutAreCopyWithoutRestrictable()
    {
        const std::map<std::string, bool> commands = declaredCommands();
        for (const std::string& id : kCopyWithoutRestrictable) {
            const auto found = commands.find(id);
            QVERIFY2(found != commands.end(), ("absente de commands.json : " + id).c_str());
            QVERIFY2(!found->second, ("« restrictable » ne doit pas changer (H8-8) : " + id).c_str());
            QVERIFY2(restrictionFor(id) == Restriction::Copy,
                     (id + " -> " + nameOf(restrictionFor(id))).c_str());
        }
    }

    void categoriesOfTheFourProhibitions()
    {
        const std::vector<std::pair<std::string, Restriction>> expected = {
            {"edit.copy", Restriction::Copy},
            {"edit.cut", Restriction::Copy},
            {"edit.duplicate", Restriction::Copy},
            {"page.copy", Restriction::Copy},
            {"page.cut", Restriction::Copy},
            {"file.save", Restriction::Save},
            {"file.save-as", Restriction::Save},
            {"export.page-image", Restriction::Save},
            {"export.pdf", Restriction::Save},
            {"file.publish", Restriction::Save},
            {"file.print", Restriction::Print},
            {"tool.camera-area", Restriction::Camera},
            {"tool.camera-polygon", Restriction::Camera},
            {"tool.camera-freehand", Restriction::Camera},
            {"tool.camera-window", Restriction::Camera},
            {"tool.camera-screen", Restriction::Camera},
            {"webframe.snapshot", Restriction::Camera},
        };
        for (const auto& [id, restriction] : expected) {
            QVERIFY2(restrictionFor(id) == restriction, (id + " -> " + nameOf(restrictionFor(id))).c_str());
        }
        for (const char* free : {"edit.paste", "page.goto", "file.open", "tool.pen", "tool.select"}) {
            QVERIFY2(!restrictionFor(free).has_value(), free);
        }
    }

    // Caméra par préfixe « tool.camera- » (D10, arbitré), et rien d'autre.
    void cameraIsAPrefix()
    {
        QVERIFY(restrictionFor("tool.camera-future") == Restriction::Camera);
        for (const char* other : {"tool.camera", "tool.cameraarea", "camera.area", "tool.camer-area", ""}) {
            QVERIFY2(!restrictionFor(other).has_value(), other);
        }
    }
};

QTEST_MAIN(TstCoreBroadcast)
#include "tst_core_broadcast.moc"
