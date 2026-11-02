#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/formats/clipboard.h>
#include <actcraft/formats/nativejson.h>

#include <QByteArray>
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>

#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

// Presse-papiers d'objets (L5-T3). Le patron écarté : sérialiser chaque objet à la main dans clipboard.cpp.
// Le presse-papiers et l'enregistrement divergeraient (clés inconnues, objets d'un type inconnu, clés
// facultatives comme « firstBaseline », hors empreinte). Les tests qui le distinguent :
// objectsWrittenByNativeWriter (écriture) et objectsReadByNativeReader (lecture) ; roundTripKeepsEveryKind
// compare aussi le JSON natif de chaque objet, pas seulement son empreinte.

using namespace actcraft;
using namespace Qt::StringLiterals;
using core::PageObject;

namespace {

const QString kFormat = QStringLiteral("actcraft-objects/1");

std::string sha256Hex(const QByteArray& bytes)
{
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex().toStdString();
}

// Image PNG d'un pixel.
QByteArray pngBytes()
{
    return QByteArray::fromBase64(
        "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVO"
        "RK5CYII=");
}

QByteArray soundBytes()
{
    return QByteArray("RIFF\x24\x00\x00\x00WAVEfmt \x10\x00\x00\x00", 24);
}

QByteArray fileBytes()
{
    return QByteArrayLiteral("%PDF-1.4 fichier ouvert par une action");
}

QByteArray compact(const QJsonArray& array)
{
    return QJsonDocument(array).toJson(QJsonDocument::Compact);
}

core::Document documentOf(const std::vector<PageObject>& objects)
{
    core::Page page;
    page.objects = objects;
    std::vector<core::Page> pages;
    pages.push_back(std::move(page));
    return core::Document(std::move(pages));
}

// Objets tels que l'enregistrement (document.json) les écrit : le chemin de référence.
QJsonArray nativeObjects(const std::vector<PageObject>& objects)
{
    const QJsonObject json = formats::documentToJson(documentOf(objects));
    return json.value(u"pages").toArray().at(0).toObject().value(u"objects").toArray();
}

// Document relu par le format natif, ressources déclarées et fournies : `objects` sur sa première page.
core::Document readNative(const std::vector<PageObject>& objects,
                          const std::map<std::string, std::string>& resources,
                          const std::map<std::string, std::string>& extensions)
{
    QJsonObject json = formats::documentToJson(documentOf(objects));
    QJsonArray declared;
    for (const auto& [key, bytes] : resources) {
        declared.append(QJsonObject{{u"key"_s, QString::fromStdString(key)},
                                    {u"extension"_s, QString::fromStdString(extensions.at(key))}});
    }
    json.insert(u"resources"_s, declared);
    core::Document out;
    const formats::JsonReadResult result = formats::documentFromJson(
        json,
        [&resources](const std::string& key, const std::string&) -> formats::LoadedResource {
            const auto it = resources.find(key);
            if (it == resources.end()) {
                return std::nullopt;
            }
            return QByteArray::fromStdString(it->second);
        },
        out);
    if (!result.ok) {
        qWarning("%s", qPrintable(result.error));
    }
    return out;
}

QJsonArray objectsOfFirstPage(const core::Document& document)
{
    return formats::documentToJson(document)
        .value(u"pages")
        .toArray()
        .at(0)
        .toObject()
        .value(u"objects")
        .toArray();
}

std::uint64_t objectFingerprint(const PageObject& object)
{
    core::Page page;
    page.objects = {object};
    return core::fingerprint(page);
}

core::RectF rect(double x, double y, double width, double height)
{
    return core::RectF{x, y, width, height};
}

// Un objet de chaque genre du modèle (ObjectKind, Equation et Unknown compris), avec les champs du lot 4
// (préréglage de forme, texte de haut en bas, firstBaseline) et des ressources citées par une image, un média
// et une action.
formats::ClipboardPayload sampleOfEveryKind()
{
    const QByteArray png = pngBytes();
    const QByteArray sound = soundBytes();
    const QByteArray file = fileBytes();
    formats::ClipboardPayload payload;
    payload.resources = {{sha256Hex(png), png.toStdString()},
                         {sha256Hex(sound), sound.toStdString()},
                         {sha256Hex(file), file.toStdString()}};

    core::Shape shape;
    shape.kind = core::ShapeKind::Rectangle;
    shape.bounds = rect(0, 0, 120, 80);
    shape.fill = 0xFF336699u;
    shape.preset = "rectangle";
    shape.parameters = {0.2};
    PageObject shapeObject = shape;
    shapeObject.common.id = 1;
    shapeObject.common.name = "forme nommée";
    shapeObject.common.locked = true;
    shapeObject.common.dragCopy = true;
    shapeObject.common.translucency = 40;
    shapeObject.common.restrictions.movable = false;
    shapeObject.common.label = "légende";
    core::Action open;
    open.resource = sha256Hex(file);
    shapeObject.common.actions = {open};

    core::ImageObject image;
    image.bounds = rect(0, 0, 64, 64);
    image.resource = sha256Hex(png);
    image.hasKeyColor = true;
    image.keyColor = 0xFF00FF00u;
    PageObject imageObject = image;
    imageObject.common.id = 2;

    core::Stroke stroke;
    stroke.points = {core::StrokePoint{0, 0, 0.25f}, core::StrokePoint{10, 5, 0.75f}};
    stroke.baseWidth = 4;
    stroke.kind = core::StrokeKind::Highlighter;
    PageObject strokeObject = stroke;
    strokeObject.common.id = 3;

    core::Line line;
    line.points = {core::PointF{0, 0}, core::PointF{50, 50}};
    PageObject lineObject = line;
    lineObject.common.id = 4;
    lineObject.unknownKeys = R"({"futur":{"profondeur":1}})";

    core::Text text;
    text.bounds = rect(0, 0, 200, 40);
    text.html = "<p>Bonjour</p>";
    core::TextSpan run;
    run.text = "Bonjour";
    run.fontFamily = "Sans";
    run.pointSize = 18;
    run.weight = 700;
    run.italic = true;
    run.baseline = core::PointF{2, 16};
    run.bounds = rect(0, 0, 80, 20);
    text.runs = {run};
    text.firstBaseline = core::PointF{4, 18};
    PageObject textObject = text;
    textObject.common.id = 5;

    core::Text column;
    column.bounds = rect(0, 0, 20, 200);
    column.html = "<p>Haut</p>";
    column.vertical = true;
    PageObject columnObject = column;
    columnObject.common.id = 6;

    core::Media media;
    media.bounds = rect(0, 0, 160, 90);
    media.placeholder = sha256Hex(png);
    media.media = sha256Hex(sound);
    media.fileName = "son.wav";
    media.playOnOpen = true;
    media.loop = true;
    PageObject mediaObject = media;
    mediaObject.common.id = 7;

    core::Group group;
    group.members = {1, 2};
    PageObject groupObject = group;
    groupObject.common.id = 8;

    core::Connector connector;
    connector.firstEnd = core::PointF{120, 40};
    connector.secondEnd = core::PointF{200, 40};
    connector.objectA = 1;
    connector.objectB = 2;
    connector.style = 2;
    connector.capA = core::ConnectorCap::Round;
    connector.capB = core::ConnectorCap::Arrow;
    PageObject connectorObject = connector;
    connectorObject.common.id = 9;

    core::Equation equation;
    equation.bounds = rect(0, 0, 100, 30);
    equation.mathml = "<math><mi>x</mi></math>";
    equation.pointSize = 30;
    PageObject equationObject = equation;
    equationObject.common.id = 10;

    // Objet d'une version plus récente : son JSON, tel que le lecteur natif le garde (compact, clés triées).
    const QJsonObject futureJson{{u"type"_s, u"objet-du-futur"_s},
                                 {u"id"_s, 11},
                                 {u"bounds"_s, QJsonArray{0, 0, 10, 10}},
                                 {u"magie"_s, 42}};
    core::UnknownObject future;
    future.type = "objet-du-futur";
    future.json = QJsonDocument(futureJson).toJson(QJsonDocument::Compact).toStdString();
    future.bounds = rect(0, 0, 10, 10);
    future.hasBounds = true;
    PageObject futureObject = future;
    futureObject.common.id = 11;

    payload.objects = {shapeObject, imageObject, strokeObject,    lineObject,     textObject,  columnObject,
                       mediaObject, groupObject, connectorObject, equationObject, futureObject};
    return payload;
}

std::map<std::string, std::string> extensionsOf(const formats::ClipboardPayload& payload)
{
    std::map<std::string, std::string> extensions;
    for (const auto& [key, bytes] : payload.resources) {
        const QByteArray data = QByteArray::fromStdString(bytes);
        extensions[key] = data == pngBytes() ? "png" : (data == soundBytes() ? "wav" : "pdf");
    }
    return extensions;
}

formats::ClipboardPayload groupOfTwo()
{
    core::Shape first;
    first.bounds = rect(0, 0, 10, 10);
    core::Shape second;
    second.bounds = rect(20, 0, 10, 10);
    core::Group group;
    group.members = {21, 22};
    formats::ClipboardPayload payload;
    payload.objects = {PageObject(first), PageObject(second), PageObject(group)};
    payload.objects[0].common.id = 21;
    payload.objects[1].common.id = 22;
    payload.objects[2].common.id = 23;
    return payload;
}

// Charge utile écrite à la main (sans encodeClipboard) : une image qui cite `cited`, et `resources`.
QByteArray handWrittenPayload(const std::string& cited, const QJsonObject& resources)
{
    core::ImageObject image;
    image.bounds = rect(0, 0, 64, 64);
    image.resource = cited;
    PageObject object = image;
    object.common.id = 31;
    const QJsonObject root{
        {u"format"_s, kFormat}, {u"objects"_s, nativeObjects({object})}, {u"resources"_s, resources}};
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

QByteArray payloadWithWrongResourceKey()
{
    // Les octets ne correspondent pas à leur clé SHA-256.
    return handWrittenPayload(sha256Hex(pngBytes()),
                              QJsonObject{{QString::fromStdString(sha256Hex(pngBytes())),
                                           QString::fromLatin1(soundBytes().toBase64())}});
}

} // namespace

class TestClipboard : public QObject
{
    Q_OBJECT
private slots:
    void roundTripKeepsEveryKind();
    void objectsWrittenByNativeWriter();
    void objectsReadByNativeReader();
    void resourcesTravelWithObjects();
    void groupMembersTravelTogether();
    void resourceExtensionsTravel();
    void foreignPayloadRejected();
};

void TestClipboard::roundTripKeepsEveryKind()
{
    // Un objet de chaque type d'ObjectKind (Shape, Image, Stroke, Line, Text, Media, Group, Connector,
    // Equation, Unknown).
    formats::ClipboardPayload payload = sampleOfEveryKind();
    std::set<std::size_t> kinds;
    for (const PageObject& object : payload.objects) {
        kinds.insert(object.index());
    }
    QCOMPARE(kinds.size(), core::kObjectKindCount);
    // Précondition : l'échantillon traverse lui-même l'enregistrement sans changer (sinon l'écart viendrait
    // du format natif, pas du presse-papiers).
    QCOMPARE(
        compact(objectsOfFirstPage(readNative(payload.objects, payload.resources, extensionsOf(payload)))),
        compact(nativeObjects(payload.objects)));

    const auto back = formats::decodeClipboard(formats::encodeClipboard(payload));
    QVERIFY(back);
    QCOMPARE(back->objects.size(), payload.objects.size());
    for (std::size_t i = 0; i < payload.objects.size(); ++i)
        QCOMPARE(objectFingerprint(back->objects[i]), objectFingerprint(payload.objects[i]));
    // L'empreinte ne voit pas tout (firstBaseline en est hors) : chaque objet relu s'écrit comme l'original.
    QCOMPARE(compact(nativeObjects(back->objects)), compact(nativeObjects(payload.objects)));
    const auto* text = std::get_if<core::Text>(&back->objects[4]);
    QVERIFY(text);
    QVERIFY(text->firstBaseline);
    QCOMPARE(text->firstBaseline->x, 4.0);
    QCOMPARE(text->firstBaseline->y, 18.0);
    QCOMPARE(back->resources, payload.resources);
}

void TestClipboard::objectsWrittenByNativeWriter()
{
    // Le test qui distingue, côté écriture : chaque objet de la charge utile est, clé pour clé, celui que
    // l'enregistrement écrit. Un écrivain à la main diverge (unknownKeys fusionnées, objet inconnu gardé tel
    // quel, clés facultatives).
    const formats::ClipboardPayload payload = sampleOfEveryKind();
    const QJsonDocument written = QJsonDocument::fromJson(formats::encodeClipboard(payload));
    QVERIFY(written.isObject());
    const QJsonObject root = written.object();
    QCOMPARE(root.value(u"format").toString(), kFormat);
    QCOMPARE(compact(root.value(u"objects").toArray()), compact(nativeObjects(payload.objects)));
    const QJsonObject resources = root.value(u"resources").toObject();
    QCOMPARE(std::size_t(resources.size()), payload.resources.size());
    for (const auto& [key, bytes] : payload.resources) {
        const QJsonValue value = resources.value(QString::fromStdString(key));
        QVERIFY(value.isString());
        QCOMPARE(QByteArray::fromBase64(value.toString().toLatin1()), QByteArray::fromStdString(bytes));
    }
}

void TestClipboard::objectsReadByNativeReader()
{
    // Le test qui distingue, côté lecture : une charge utile d'une version plus récente (clé inconnue d'un
    // objet connu, objet d'un type inconnu) est lue comme le fichier la lit, et réécrite à l'identique.
    core::Shape shape;
    shape.bounds = rect(0, 0, 30, 30);
    PageObject shapeObject = shape;
    shapeObject.common.id = 41;
    QJsonArray objects = nativeObjects({shapeObject});
    QJsonObject shapeJson = objects.at(0).toObject();
    shapeJson.insert(u"futur"_s, QJsonObject{{u"profondeur"_s, 2}});
    objects.replace(0, shapeJson);
    objects.append(QJsonObject{{u"type"_s, u"objet-du-futur"_s}, {u"id"_s, 42}, {u"magie"_s, true}});
    const QJsonObject root{{u"format"_s, kFormat}, {u"objects"_s, objects}, {u"resources"_s, QJsonObject{}}};

    const auto back = formats::decodeClipboard(QJsonDocument(root).toJson(QJsonDocument::Compact));
    QVERIFY(back);
    QCOMPARE(back->objects.size(), std::size_t(2));
    QCOMPARE(back->objects[0].unknownKeys, std::string(R"({"futur":{"profondeur":2}})"));
    QVERIFY(std::get_if<core::UnknownObject>(&back->objects[1]));
    QCOMPARE(back->objects[1].common.id, core::ObjectId(42));
    QCOMPARE(compact(nativeObjects(back->objects)), compact(objects));
}

void TestClipboard::resourcesTravelWithObjects()
{
    // Une ImageObject et sa ressource PNG, dans un document relu par le format natif.
    const QByteArray png = pngBytes();
    core::ImageObject image;
    image.bounds = rect(0, 0, 64, 64);
    image.resource = sha256Hex(png);
    PageObject imageObject = image;
    imageObject.common.id = 51;
    const std::vector<PageObject> objects{imageObject};
    const core::Document document =
        readNative(objects, {{sha256Hex(png), png.toStdString()}}, {{sha256Hex(png), "png"}});

    const auto resources = formats::resourcesFor(document, objects);
    QCOMPARE(resources.size(), std::size_t(1));
    QCOMPARE(QByteArray::fromStdString(resources.at(sha256Hex(png))), png);
    formats::ClipboardPayload payload{objects, resources};
    const auto back = formats::decodeClipboard(formats::encodeClipboard(payload));
    QVERIFY(back);
    QCOMPARE(back->resources, resources);
}

void TestClipboard::groupMembersTravelTogether()
{
    // Le format ne remappe rien : identifiants et références du groupe identiques après l'aller-retour.
    formats::ClipboardPayload payload = groupOfTwo();
    const auto back = formats::decodeClipboard(formats::encodeClipboard(payload));
    QVERIFY(back);
    const auto* group = std::get_if<core::Group>(&back->objects.back());
    QVERIFY(group);
    QCOMPARE(group->members,
             (std::vector<core::ObjectId>{payload.objects[0].common.id, payload.objects[1].common.id}));
}

void TestClipboard::foreignPayloadRejected()
{
    // Témoin : la même charge utile écrite à la main, ressource juste, est acceptée.
    const QByteArray png = pngBytes();
    const auto accepted = formats::decodeClipboard(handWrittenPayload(
        sha256Hex(png),
        QJsonObject{{QString::fromStdString(sha256Hex(png)), QString::fromLatin1(png.toBase64())}}));
    QVERIFY(accepted);
    QCOMPARE(accepted->resources.size(), std::size_t(1));

    QVERIFY(!formats::decodeClipboard(QByteArray("{\"format\":\"autre/1\",\"objects\":[]}")));
    QVERIFY(!formats::decodeClipboard(QByteArray("pas du json")));
    // Ressource citée absente, ou octets qui ne correspondent pas à leur clé SHA-256 : refusée.
    QVERIFY(!formats::decodeClipboard(payloadWithWrongResourceKey()));
    QVERIFY(!formats::decodeClipboard(handWrittenPayload(sha256Hex(png), QJsonObject{})));
}

void TestClipboard::resourceExtensionsTravel()
{
    // Document::addResource exige l'extension : elle voyage avec la ressource (H5-13, L5-T4).
    const QByteArray png = pngBytes();
    const std::string key = sha256Hex(png);
    formats::ClipboardPayload payload;
    payload.resources = {{key, png.toStdString()}};
    payload.extensions = {{key, "png"}};
    auto back = formats::decodeClipboard(formats::encodeClipboard(payload));
    QVERIFY(back);
    QCOMPARE(back->extensions, payload.extensions);

    // Charge sans table (celle de L5-T3) : lisible, aucune extension (« bin » au collage).
    payload.extensions.clear();
    const QByteArray bytes = formats::encodeClipboard(payload);
    QVERIFY(!QJsonDocument::fromJson(bytes).object().contains(u"extensions"_s));
    back = formats::decodeClipboard(bytes);
    QVERIFY(back);
    QVERIFY(back->extensions.empty());
    QCOMPARE(back->resources, payload.resources);

    // Extension mal formée, ou clé sans ressource : ignorée.
    const QJsonObject root{
        {u"format"_s, kFormat},
        {u"objects"_s, QJsonArray{}},
        {u"resources"_s, QJsonObject{{QString::fromStdString(key), QString::fromLatin1(png.toBase64())}}},
        {u"extensions"_s, QJsonObject{{QString::fromStdString(key), u"P NG"_s}, {u"abc"_s, u"png"_s}}}};
    back = formats::decodeClipboard(QJsonDocument(root).toJson(QJsonDocument::Compact));
    QVERIFY(back);
    QVERIFY(back->extensions.empty());

    // Table d'extensions qui n'est pas un objet : charge refusée, comme pour « resources ».
    QJsonObject wrong = root;
    wrong.insert(u"extensions"_s, u"png"_s);
    QVERIFY(!formats::decodeClipboard(QJsonDocument(wrong).toJson(QJsonDocument::Compact)));
}

QTEST_GUILESS_MAIN(TestClipboard)
#include "tst_formats_clipboard.moc"
