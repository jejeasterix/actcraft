#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/formats/fragment.h>
#include <actcraft/formats/nativefile.h>

#include <QBuffer>
#include <QtTest>

#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>

using namespace actcraft;

namespace {

// Document de 2 pages : page 1 = un trait, l'image A, une forme ; page 2 = l'image B.
struct Source
{
    core::Document document;
    std::string keyA;
    std::string keyB;
    core::ObjectId stroke = 0;
    core::ObjectId imageA = 0;
    core::ObjectId shape = 0;
    core::ObjectId imageB = 0;
};

core::PageObject withId(core::PageObject object, core::ObjectId id)
{
    object.common.id = id;
    return object;
}

core::PageObject imageOf(const std::string& key, double x)
{
    core::ImageObject image;
    image.bounds = {x, 100, 64, 64};
    image.resource = key;
    return core::PageObject(image);
}

Source makeSource()
{
    Source s;
    core::Document& d = s.document;
    d.setTitle("Cours de chimie");
    d.appendPage();
    s.keyA = d.addResource(QByteArray("\x89PNG", 4) + QByteArray("image-A"), "png");
    s.keyB = d.addResource(QByteArray("\x89PNG", 4) + QByteArray("image-B"), "png");

    core::Stroke stroke;
    stroke.points = {{10, 10, 0.5f}, {50, 60, 0.7f}};
    core::Shape shape;
    shape.bounds = {200, 200, 80, 40};

    s.stroke = d.allocateId();
    s.imageA = d.allocateId();
    s.shape = d.allocateId();
    s.imageB = d.allocateId();
    d.page(0).objects = {withId(core::PageObject(stroke), s.stroke), withId(imageOf(s.keyA, 100), s.imageA),
                         withId(core::PageObject(shape), s.shape)};
    d.page(1).objects = {withId(imageOf(s.keyB, 300), s.imageB)};
    d.page(0).description = "Acides et bases";
    d.page(0).keywords = {"acide", "pH"};
    return s;
}

// Bouton de la page 1 dont l'action « masquer » vise `target` (ActionKind::OnObject).
core::ObjectId addButton(Source& s, core::ObjectId target)
{
    core::Shape shape;
    shape.bounds = {400, 400, 60, 30};
    core::PageObject button(shape);
    core::Action hide;
    hide.kind = core::ActionKind::OnObject;
    hide.target = target;
    button.common.actions.push_back(hide);
    const core::ObjectId id = s.document.allocateId();
    s.document.page(0).objects.push_back(withId(std::move(button), id));
    return id;
}

// Le seul objet de `objects` qui porte une action.
const core::PageObject* withAction(const std::vector<core::PageObject>& objects)
{
    for (const core::PageObject& object : objects) {
        if (!object.common.actions.empty())
            return &object;
    }
    return nullptr;
}

QByteArray writeToBytes(const core::Document& document, int page, const std::vector<core::ObjectId>& objects,
                        QString& error)
{
    QBuffer out;
    out.open(QIODevice::WriteOnly);
    if (!formats::writeFragment(document, page, objects, out, error))
        return {};
    return out.data();
}

std::optional<formats::Fragment> readFromBytes(const QByteArray& bytes, QString& error)
{
    QBuffer in;
    in.setData(bytes);
    in.open(QIODevice::ReadOnly);
    return formats::readFragment(in, error);
}

// Empreinte d'un objet seul : celle d'une page par défaut qui ne contient que lui.
std::uint64_t objectPrint(const core::PageObject& object)
{
    core::Page page;
    page.objects.push_back(object);
    return core::fingerprint(page);
}

QStringList resourceEntries(const QByteArray& archive)
{
    QStringList names;
    for (const QString& name : formats::archiveEntryNames(archive)) {
        if (name.startsWith(QLatin1String("resources/")))
            names << name;
    }
    return names;
}

QString resourceEntry(const std::string& key, const char* extension)
{
    return QStringLiteral("resources/%1.%2").arg(QString::fromStdString(key), QLatin1String(extension));
}

} // namespace

class TstFormatsFragment : public QObject
{
    Q_OBJECT
private slots:
    void objectsAndResourcesRoundTrip();
    void onlyReferencedResourcesAreWritten();
    void insertGivesFreshIdentifiers();
    void unknownObjectKept();
    void notAFragmentRefused();
    void resourceExtensionsKept();
    void danglingActionTargetIsCleared();
    void selectedActionTargetFollowsTheFreshIds();
    void missingResourceRefused();
};

void TstFormatsFragment::objectsAndResourcesRoundTrip()
{
    const Source s = makeSource();
    QString error;
    const QByteArray bytes = writeToBytes(s.document, 0, {s.stroke, s.imageA}, error);
    QVERIFY2(!bytes.isEmpty(), qPrintable(error));

    const std::optional<formats::Fragment> fragment = readFromBytes(bytes, error);
    QVERIFY2(fragment.has_value(), qPrintable(error));
    QCOMPARE(fragment->page.objects.size(), std::size_t(2));
    const core::Page& source = s.document.page(0);
    QCOMPARE(objectPrint(fragment->page.objects[0]), objectPrint(source.objects[0])); // le trait
    QCOMPARE(objectPrint(fragment->page.objects[1]), objectPrint(source.objects[1])); // l'image A

    const core::Resource* a = fragment->resources.resource(s.keyA);
    QVERIFY(a != nullptr);
    QCOMPARE(a->data, s.document.resource(s.keyA)->data);
    QVERIFY(fragment->resources.resource(s.keyB) == nullptr);

    // Mots-clés et description de la page (L6-T10) suivent le fragment : la recherche de la bibliothèque
    // (L6-T20) les lit.
    QCOMPARE(QString::fromStdString(fragment->page.description), QStringLiteral("Acides et bases"));
    QVERIFY(fragment->page.keywords == source.keywords);
    QCOMPARE(fragment->page.keywords.size(), std::size_t(2));
}

void TstFormatsFragment::onlyReferencedResourcesAreWritten()
{
    const Source s = makeSource();
    // Témoin du patron écarté : le document entier, élagué ou non, écrit A et B.
    const QByteArray whole = formats::encodeActcraft(s.document, {});
    QCOMPARE(resourceEntries(whole).size(), qsizetype(2));

    QString error;
    const QByteArray bytes = writeToBytes(s.document, 0, {s.stroke, s.imageA}, error);
    QVERIFY2(!bytes.isEmpty(), qPrintable(error));
    QCOMPARE(resourceEntries(bytes), QStringList{resourceEntry(s.keyA, "png")});

    // Un objet sans ressource : aucune ressource écrite.
    const QByteArray shapeOnly = writeToBytes(s.document, 0, {s.shape}, error);
    QVERIFY2(!shapeOnly.isEmpty(), qPrintable(error));
    QCOMPARE(resourceEntries(shapeOnly), QStringList{});

    // Une page, rien du document d'origine hors des objets choisis.
    const formats::DecodeResult decoded = formats::decodeActcraft(bytes);
    QVERIFY2(decoded.ok, qPrintable(decoded.error));
    QCOMPARE(decoded.document.pageCount(), std::size_t(1));
    QCOMPARE(decoded.document.resources().size(), std::size_t(1));
    QVERIFY(decoded.document.title().empty());
}

void TstFormatsFragment::insertGivesFreshIdentifiers()
{
    Source s = makeSource();
    QString error;
    const std::optional<formats::Fragment> fragment =
        readFromBytes(writeToBytes(s.document, 0, {s.stroke, s.imageA}, error), error);
    QVERIFY2(fragment.has_value(), qPrintable(error));

    std::set<core::ObjectId> taken;
    for (std::size_t p = 0; p < s.document.pageCount(); ++p) {
        for (const core::PageObject& object : s.document.page(p).objects)
            taken.insert(object.common.id);
    }
    const std::size_t resourcesBefore = s.document.resources().size();

    const std::vector<core::PageObject> first = formats::prepareInsertion(*fragment, s.document);
    QCOMPARE(first.size(), std::size_t(2));
    for (const core::PageObject& object : first) {
        QVERIFY(object.common.id != 0);
        QVERIFY2(taken.count(object.common.id) == 0, "identifiant déjà pris dans le document");
        taken.insert(object.common.id);
    }
    QCOMPARE(s.document.resources().size(), resourcesBefore); // A n'est pas dupliquée
    const auto* image = std::get_if<core::ImageObject>(&first[1]);
    QVERIFY(image != nullptr);
    QCOMPARE(QString::fromStdString(image->resource), QString::fromStdString(s.keyA));

    // Les identifiants sont pris dans le document : une seconde insertion en reçoit d'autres.
    const std::vector<core::PageObject> second = formats::prepareInsertion(*fragment, s.document);
    QCOMPARE(second.size(), std::size_t(2));
    for (const core::PageObject& object : second)
        QVERIFY2(taken.count(object.common.id) == 0, "identifiant déjà rendu par la première insertion");

    // Un cours qui n'a pas A : la préparation l'y copie.
    core::Document other;
    QVERIFY(other.resource(s.keyA) == nullptr);
    QCOMPARE(formats::prepareInsertion(*fragment, other).size(), std::size_t(2));
    const core::Resource* copied = other.resource(s.keyA);
    QVERIFY2(copied != nullptr, "ressource citée absente du document cible après la préparation");
    QCOMPARE(copied->data, s.document.resource(s.keyA)->data);
}

void TstFormatsFragment::unknownObjectKept()
{
    Source s = makeSource();
    core::UnknownObject hologram;
    hologram.type = "hologram";
    hologram.json = R"({"depth":3,"type":"hologram"})";
    const core::ObjectId id = s.document.allocateId();
    s.document.page(0).objects.push_back(withId(core::PageObject(hologram), id));

    QString error;
    const QByteArray first = writeToBytes(s.document, 0, {id}, error);
    QVERIFY2(!first.isEmpty(), qPrintable(error));
    const std::optional<formats::Fragment> fragment = readFromBytes(first, error);
    QVERIFY2(fragment.has_value(), qPrintable(error));
    QCOMPARE(fragment->page.objects.size(), std::size_t(1));
    const auto* kept = std::get_if<core::UnknownObject>(&fragment->page.objects[0]);
    QVERIFY(kept != nullptr);
    QCOMPARE(QString::fromStdString(kept->type), QStringLiteral("hologram"));
    QVERIFY2(QString::fromStdString(kept->json).contains(QLatin1String(R"("depth":3)")), kept->json.c_str());

    // Réécrit depuis le fragment relu : mêmes octets.
    core::Document again(std::vector<core::Page>{fragment->page});
    again.refreshNextId();
    const QByteArray second = writeToBytes(again, 0, {fragment->page.objects[0].common.id}, error);
    QVERIFY2(!second.isEmpty(), qPrintable(error));
    QCOMPARE(second, first);
}

void TstFormatsFragment::notAFragmentRefused()
{
    const Source s = makeSource();
    const QByteArray whole = formats::encodeActcraft(s.document, {});
    QVERIFY(!whole.isEmpty());

    QString error;
    const std::optional<formats::Fragment> fragment = readFromBytes(whole, error);
    QVERIFY(!fragment.has_value());
    QVERIFY(!error.isEmpty());
}

void TstFormatsFragment::resourceExtensionsKept()
{
    const Source s = makeSource();
    QString error;
    const std::optional<formats::Fragment> fragment =
        readFromBytes(writeToBytes(s.document, 0, {s.imageA}, error), error);
    QVERIFY2(fragment.has_value(), qPrintable(error));
    const core::Resource* carried = fragment->resources.resource(s.keyA);
    QVERIFY(carried != nullptr);
    QCOMPARE(QString::fromStdString(carried->extension), QStringLiteral("png"));

    core::Document target;
    const std::vector<core::PageObject> inserted = formats::prepareInsertion(*fragment, target);
    QCOMPARE(inserted.size(), std::size_t(1));
    const core::Resource* copied = target.resource(s.keyA);
    QVERIFY(copied != nullptr);
    QCOMPARE(QString::fromStdString(copied->extension), QStringLiteral("png"));
}

void TstFormatsFragment::danglingActionTargetIsCleared()
{
    // Bouton « masquer la forme » ; la forme n'est pas choisie.
    Source s = makeSource();
    const core::ObjectId button = addButton(s, s.shape);
    QString error;
    const std::optional<formats::Fragment> fragment =
        readFromBytes(writeToBytes(s.document, 0, {button}, error), error);
    QVERIFY2(fragment.has_value(), qPrintable(error));
    QCOMPARE(fragment->page.objects.size(), std::size_t(1));
    const std::vector<core::Action>& actions = fragment->page.objects[0].common.actions;
    QCOMPARE(actions.size(), std::size_t(1));
    QVERIFY(actions[0].kind == core::ActionKind::OnObject);
    QCOMPARE(actions[0].target, core::ObjectId(0)); // pas l'identifiant du cours d'origine

    // Inséré dans un autre cours où cet identifiant désigne un autre objet : le bouton ne le vise pas.
    Source other = makeSource();
    QVERIFY(other.document.page(0).objects.at(2).common.id == s.shape);
    const std::vector<core::PageObject> inserted = formats::prepareInsertion(*fragment, other.document);
    QCOMPARE(inserted.size(), std::size_t(1));
    QCOMPARE(inserted[0].common.actions.at(0).target, core::ObjectId(0));
}

void TstFormatsFragment::selectedActionTargetFollowsTheFreshIds()
{
    // Bouton « masquer la forme » ; la forme est choisie avec lui.
    Source s = makeSource();
    const core::ObjectId button = addButton(s, s.shape);
    QString error;
    const std::optional<formats::Fragment> fragment =
        readFromBytes(writeToBytes(s.document, 0, {button, s.shape}, error), error);
    QVERIFY2(fragment.has_value(), qPrintable(error));
    QCOMPARE(fragment->page.objects.size(), std::size_t(2));
    const core::PageObject* carried = withAction(fragment->page.objects);
    QVERIFY(carried != nullptr);
    QCOMPARE(carried->common.actions.at(0).target, s.shape); // cible choisie : gardée

    const std::vector<core::PageObject> inserted = formats::prepareInsertion(*fragment, s.document);
    QCOMPARE(inserted.size(), std::size_t(2));
    const core::PageObject* insertedButton = withAction(inserted);
    QVERIFY(insertedButton != nullptr);
    const core::PageObject& insertedShape = &inserted[0] == insertedButton ? inserted[1] : inserted[0];
    QVERIFY(insertedShape.common.id != s.shape);
    QCOMPARE(insertedButton->common.actions.at(0).target, insertedShape.common.id); // suit la renumérotation
}

void TstFormatsFragment::missingResourceRefused()
{
    const Source s = makeSource();
    QString error;
    const QByteArray bytes = writeToBytes(s.document, 0, {s.imageA}, error);
    QVERIFY2(!bytes.isEmpty(), qPrintable(error));
    QVERIFY2(readFromBytes(bytes, error).has_value(), qPrintable(error)); // témoin : intact, il est lu

    // Illisible : les octets de A, rangés tels quels dans l'archive, ne sont plus les siens.
    QByteArray corrupt = bytes;
    QCOMPARE(corrupt.count("image-A"), qsizetype(1));
    corrupt.replace("image-A", "image-Z");
    error.clear();
    QVERIFY(!readFromBytes(corrupt, error).has_value());
    QVERIFY2(error.contains(QString::fromStdString(s.keyA)), qPrintable(error));

    // Absente : l'entrée de A porte un autre nom (en-tête local et répertoire central).
    QByteArray absent = bytes;
    const QByteArray entry = resourceEntry(s.keyA, "png").toLatin1();
    QCOMPARE(absent.count(entry), qsizetype(2));
    absent.replace(entry, resourceEntry(s.keyA, "pnx").toLatin1());
    error.clear();
    QVERIFY(!readFromBytes(absent, error).has_value());
    QVERIFY2(error.contains(QString::fromStdString(s.keyA)), qPrintable(error));
}

QTEST_MAIN(TstFormatsFragment)
#include "tst_formats_fragment.moc"
