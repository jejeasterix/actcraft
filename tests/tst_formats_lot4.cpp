// Champs du lot 4 en .actcraft (L4-T6) : préréglage et points intelligents d'une forme, texte de haut en bas,
// hauteur d'origine d'une page. Clés facultatives de actcraft-document/1 : écrites seulement hors défaut,
// lues comme des clés connues (jamais rangées parmi les clés inconnues gardées, L2-C8). Texte de haut en bas
// d'un .flipchart (L4-T24, D9). Zones de page (L4-T27, D11).
#include "support/flipchartwriter.h"

#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/core/pageedits.h>
#include <actcraft/core/undo.h>
#include <actcraft/formats/flipchart.h>
#include <actcraft/formats/nativefile.h>
#include <actcraft/formats/nativejson.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace actcraft;
using namespace Qt::StringLiterals;

namespace {

// Clés du lot 4, sous la forme compacte « "clé": » : une valeur de texte égale à un nom ne les imite pas.
constexpr const char* kLot4Keys[] = {
    "\"preset\":", "\"parameters\":", "\"vertical\":", "\"baseHeight\":", "\"zones\":"}; // « zones » : L4-T27

core::Document sample()
{
    core::Document document;
    core::PageObject shape{core::Shape{}};
    shape.common.id = 1;
    std::get<core::Shape>(core::body(shape)).preset = "arrow";
    std::get<core::Shape>(core::body(shape)).parameters = {0.4, 0.2};
    core::PageObject text{core::Text{}};
    text.common.id = 2;
    std::get<core::Text>(core::body(text)).html = "<p>ABC</p>";
    std::get<core::Text>(core::body(text)).vertical = true;
    document.page(0).objects = {shape, text};
    document.page(0).baseHeight = 768;
    document.page(0).height = 844.8;
    document.refreshNextId();
    return document;
}

// Un document sans aucune donnée du lot 4 : ce qu'écrivait le lot 3.
core::Document plain()
{
    core::Document document;
    core::PageObject shape{core::Shape{}};
    shape.common.id = 1;
    core::PageObject text{core::Text{}};
    text.common.id = 2;
    document.page(0).objects = {shape, text};
    document.refreshNextId();
    return document;
}

struct ReadBack
{
    formats::JsonReadResult result;
    core::Document document;
};

ReadBack readBack(const QJsonObject& json)
{
    ReadBack back;
    back.result = formats::documentFromJson(
        json, [](const std::string&, const std::string&) -> formats::LoadedResource { return std::nullopt; },
        back.document);
    return back;
}

QByteArray bytes(const core::Document& document)
{
    return QJsonDocument(formats::documentToJson(document)).toJson(QJsonDocument::Compact);
}

QString report(const formats::JsonReadResult& result)
{
    return result.error + result.warnings.join(u'\n');
}

QJsonObject firstPage(const QJsonObject& root)
{
    return root.value(u"pages"_s).toArray().at(0).toObject();
}

QJsonObject objectAt(const QJsonObject& root, qsizetype index)
{
    return firstPage(root).value(u"objects"_s).toArray().at(index).toObject();
}

// Paperboard d'une page (nœud 2003) : un texte de haut en bas (genre 469, nœud 2020) avec son cadre, dont la
// propriété 38 désigne une ressource de texte qui porte du HTML, puis une image (genre 9, nœud 2030). Le
// numéro 38 de ce genre n'est pas dans notre documentation du format : un lecteur qui le suivrait comme celui
// d'un texte fabriquerait un texte, comme celui d'une image en devinerait une.
QByteArray verticalTextPaperboard()
{
    using namespace fctest;
    using namespace fctest::synth;
    Sheet s(2003);
    s.tree = {Node(2020, 469, 2021), Node(2030, 9, 2031)};
    s.entries = {
        io(2021, item(469, 2021, "texte de haut en bas", 1, 1, place(120, 80))
                     .userRect(7, 0, 0, 30, 90)
                     .integer(38, 3001)),
        io(2031, item(9, 2031, "image", 1, 2, place(300, 40)).userRect(7, 0, 0, 20, 10).integer(38, 3002))};
    return paperboard(u"Texte de haut en bas"_s, {s},
                      {ir(3001, Obj(12, 3001).string(5, u"<p>SOURCE-OPAQUE</p>"_s)),
                       ir(3002, Obj(10, 3002).sizeF(5, 20, 10)),
                       Entry(u"3002.png"_s, pngBytes(20, 10, 0xFF3366CCu))});
}

} // namespace

class TstFormatsLot4 : public QObject
{
    Q_OBJECT

private slots:
    void presetAndParametersRoundTrip()
    {
        const core::Document original = sample();
        const QJsonObject shape = objectAt(formats::documentToJson(original), 0);
        QCOMPARE(shape.value(u"preset"_s).toString(), u"arrow"_s);
        QCOMPARE(shape.value(u"parameters"_s).toArray(), (QJsonArray{0.4, 0.2}));
        const ReadBack back = readBack(formats::documentToJson(original));
        QVERIFY2(back.result.ok && back.result.warnings.isEmpty(), qPrintable(report(back.result)));
        const auto& s = std::get<core::Shape>(core::body(back.document.page(0).objects[0]));
        QCOMPARE(QString::fromStdString(s.preset), u"arrow"_s);
        QCOMPARE(s.parameters.size(), std::size_t{2});
        QCOMPARE(s.parameters[0], 0.4);
        QCOMPARE(s.parameters[1], 0.2);
        QCOMPARE(core::fingerprint(back.document), core::fingerprint(original));
    }

    void defaultsAreOmitted()
    {
        const QByteArray json = bytes(plain());
        for (const char* key : kLot4Keys) {
            QVERIFY2(!json.contains(key), key);
        }
        // Relu puis réécrit sans modification : les mêmes octets (un fichier du lot 3 ne change pas).
        const ReadBack back = readBack(formats::documentToJson(plain()));
        QVERIFY2(back.result.ok && back.result.warnings.isEmpty(), qPrintable(report(back.result)));
        QCOMPARE(bytes(back.document), json);
    }

    void verticalTextRoundTrip()
    {
        const QJsonObject root = formats::documentToJson(sample());
        QCOMPARE(objectAt(root, 1).value(u"vertical"_s), QJsonValue(true));
        QVERIFY(!objectAt(root, 0).contains(u"vertical"_s)); // une forme n'a pas de sens d'écriture
        const ReadBack back = readBack(root);
        QVERIFY2(back.result.ok && back.result.warnings.isEmpty(), qPrintable(report(back.result)));
        QVERIFY(std::get<core::Text>(core::body(back.document.page(0).objects[1])).vertical);
    }

    void baseHeightRoundTrip()
    {
        const QJsonObject root = formats::documentToJson(sample());
        QCOMPARE(firstPage(root).value(u"baseHeight"_s).toDouble(), 768.0);
        const ReadBack back = readBack(root);
        QVERIFY2(back.result.ok && back.result.warnings.isEmpty(), qPrintable(report(back.result)));
        QCOMPARE(back.document.page(0).baseHeight, 768.0);
        QCOMPARE(back.document.page(0).height, 844.8);
    }

    void olderFileReadsWithDefaults()
    {
        const core::Document older = plain();
        const ReadBack back = readBack(formats::documentToJson(older));
        QVERIFY2(back.result.ok && back.result.warnings.isEmpty(), qPrintable(report(back.result)));
        const core::Page& page = back.document.page(0);
        const auto& s = std::get<core::Shape>(core::body(page.objects[0]));
        QVERIFY(s.preset.empty() && s.parameters.empty());
        QVERIFY(!std::get<core::Text>(core::body(page.objects[1])).vertical);
        QVERIFY(page.objects[0].unknownKeys.empty() && page.objects[1].unknownKeys.empty());
        QVERIFY(page.unknownKeys.empty());
        QCOMPARE(page.baseHeight, 0.0);
        QCOMPARE(core::fingerprint(back.document), core::fingerprint(older));
    }

    // Discrimine le patron écarté : les gabarits des clés connues sont le JSON d'objets vierges (KnownKeys) ;
    // une clé écrite seulement hors défaut y manquerait, serait lue comme inconnue (unknownKeys) et mergeKept
    // la réécrirait même remise au défaut. Vert si la clé lue sort des inconnues ; rouge sinon, comme si on
    // l'écrivait toujours (defaultsAreOmitted).
    void knownKeysLeaveUnknownKeys()
    {
        ReadBack back = readBack(formats::documentToJson(sample()));
        QVERIFY2(back.result.ok && back.result.warnings.isEmpty(), qPrintable(report(back.result)));
        core::Page& page = back.document.page(0);
        QVERIFY2(page.unknownKeys.empty(), page.unknownKeys.c_str());
        for (const core::PageObject& object : page.objects) {
            QVERIFY2(object.unknownKeys.empty(), object.unknownKeys.c_str());
        }
        auto& s = std::get<core::Shape>(core::body(page.objects[0]));
        auto& t = std::get<core::Text>(core::body(page.objects[1]));
        QCOMPARE(QString::fromStdString(s.preset), u"arrow"_s);
        QVERIFY(t.vertical && page.baseHeight == 768.0);
        // Remis aux défauts après lecture : plus aucune clé du lot 4 dans le fichier réécrit.
        s.preset.clear();
        s.parameters.clear();
        t.vertical = false;
        page.baseHeight = 0;
        const QByteArray json = bytes(back.document);
        for (const char* key : kLot4Keys) {
            QVERIFY2(!json.contains(key), key);
        }
    }

    // Valeur mal formée : un avertissement qui nomme la clé, le défaut du modèle, rien de gardé en inconnu.
    void invalidValuesWarnAndFallBack()
    {
        QJsonObject root = formats::documentToJson(sample());
        QJsonArray pages = root.value(u"pages"_s).toArray();
        QJsonObject page = pages.at(0).toObject();
        QJsonArray objects = page.value(u"objects"_s).toArray();
        QJsonObject shape = objects.at(0).toObject();
        shape.insert(u"preset"_s, 5);
        shape.insert(u"parameters"_s, QJsonArray{0.4, u"x"_s}); // un seul élément mal formé : tous au défaut
        QJsonObject text = objects.at(1).toObject();
        text.insert(u"vertical"_s, u"oui"_s);
        page.insert(u"baseHeight"_s, u"grande"_s);
        objects.replace(0, shape);
        objects.replace(1, text);
        page.insert(u"objects"_s, objects);
        pages.replace(0, page);
        root.insert(u"pages"_s, pages);

        const ReadBack back = readBack(root);
        QVERIFY2(back.result.ok, qPrintable(back.result.error));
        QCOMPARE(back.result.warnings.size(), qsizetype{4});
        const QString all = back.result.warnings.join(u'\n');
        for (const QString& key : {u"preset"_s, u"parameters"_s, u"vertical"_s, u"baseHeight"_s}) {
            QVERIFY2(all.contains(u"« %1 »"_s.arg(key)), qPrintable(all));
        }
        const core::Page& got = back.document.page(0);
        const auto& s = std::get<core::Shape>(core::body(got.objects[0]));
        QVERIFY(s.preset.empty() && s.parameters.empty());
        QVERIFY(!std::get<core::Text>(core::body(got.objects[1])).vertical);
        QCOMPARE(got.baseHeight, 0.0);
        QVERIFY(got.unknownKeys.empty());
        QVERIFY(got.objects[0].unknownKeys.empty() && got.objects[1].unknownKeys.empty());
    }

    // D9 : l'original garde d'un texte de haut en bas une image rendue et une source opaque, pas le texte en
    // clair. Notre documentation du format donne le genre (469) et le cadre (propriété commune 7), pas le
    // champ de l'image : l'objet est déclaré non lu (repli du plan), sous le message des genres non lus
    // qu'attend l'oracle synthétique. Discrimine les patrons écartés : lu comme un texte (core::Text à
    // vertical vrai tiré de la ressource), ou comme une image d'un champ deviné ; l'un et l'autre ajoutent un
    // objet.
    void flipchartVerticalTextRead()
    {
        const formats::flipchart::FlipchartReadResult r =
            formats::flipchart::readFlipchart(verticalTextPaperboard());
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.warnings, QStringList{u"page 2003 : objet de genre 469 non lu (nœud 2020)"_s});
        const core::Page& page = r.document.page(0);
        QCOMPARE(page.objects.size(), std::size_t{1});
        const auto* image = std::get_if<core::ImageObject>(&core::body(page.objects[0]));
        QVERIFY(image);
        QCOMPARE(image->bounds.width, 20.0);
        QCOMPARE(page.objects[0].common.transform.dx, 300.0);
        // Aucun texte fabriqué : ni objet texte, ni la source parmi les ressources du document.
        for (const core::PageObject& object : page.objects) {
            QVERIFY(!std::get_if<core::Text>(&core::body(object)));
        }
        QCOMPARE(r.document.resources().size(), std::size_t{1});
        for (const auto& [key, resource] : r.document.resources()) {
            QVERIFY2(!resource.data.contains("SOURCE-OPAQUE"), key.c_str());
        }
    }

    // L4-C4 : un objet de type inconnu est réécrit depuis son JSON d'origine ; son identifiant courant y
    // remplace celui du JSON. Sans cela, la page collée (identifiants neufs, withFreshIds) garde dans le
    // fichier l'identifiant de sa source : doublon à la relecture. Discrimine aussi le patron écarté,
    // regénérer l'objet par writeCommon : l'objet à venir n'a pas de « label », que writeCommon ajouterait.
    // Seule la clé « id » change, et l'objet source, dont l'identifiant n'a pas bougé, s'écrit avec les
    // octets lus.
    void unknownObjectWritesItsCurrentId()
    {
        QJsonObject root = formats::documentToJson(plain());
        QJsonArray pages = root.value(u"pages"_s).toArray();
        QJsonObject page = pages.at(0).toObject();
        QJsonArray objects = page.value(u"objects"_s).toArray();
        QJsonObject future = objects.at(0).toObject(); // clés communes écrites par cette version
        future.insert(u"type"_s, u"objet-a-venir"_s);
        future.insert(u"id"_s, 9);
        QVERIFY(future.contains(u"label"_s));
        future.remove(u"label"_s); // clé commune absente du fichier d'une version future
        objects.append(future);
        page.insert(u"objects"_s, objects);
        pages.replace(0, page);
        root.insert(u"pages"_s, pages);
        ReadBack back = readBack(root);
        QVERIFY2(back.result.ok, qPrintable(report(back.result)));
        core::Document& d = back.document;
        QCOMPARE(d.page(0).objects.size(), std::size_t{3});
        QCOMPARE(d.page(0).objects[2].common.id, core::ObjectId{9});
        core::UndoStack stack;
        stack.push(d, std::make_unique<core::InsertPagesCommand>(1, std::vector<core::Page>{d.page(0)}));
        QCOMPARE(d.pageCount(), std::size_t{2});
        const core::ObjectId fresh = d.page(1).objects[2].common.id;
        QVERIFY(fresh != 0 && fresh != core::ObjectId{9});
        const QJsonObject written = formats::documentToJson(d);
        const QJsonArray writtenPages = written.value(u"pages"_s).toArray();
        const QJsonObject source =
            writtenPages.at(0).toObject().value(u"objects"_s).toArray().at(2).toObject();
        const QJsonObject copied =
            writtenPages.at(1).toObject().value(u"objects"_s).toArray().at(2).toObject();
        QCOMPARE(copied.value(u"id"_s).toDouble(), static_cast<double>(fresh)); // rouge avant le correctif
        QCOMPARE(copied.value(u"type"_s).toString(), u"objet-a-venir"_s);
        // Rouge sous writeCommon (« label » ajouté) : le JSON d'origine, la clé « id » seule remplacée.
        QJsonObject expected = future;
        expected.insert(u"id"_s, static_cast<double>(fresh));
        QCOMPARE(QJsonDocument(copied).toJson(QJsonDocument::Compact),
                 QJsonDocument(expected).toJson(QJsonDocument::Compact));
        QCOMPARE(QJsonDocument(source).toJson(QJsonDocument::Compact),
                 QJsonDocument(future).toJson(QJsonDocument::Compact));
        ReadBack again = readBack(written);
        QVERIFY2(again.result.ok, qPrintable(report(again.result)));
        QCOMPARE(again.document.page(1).objects[2].common.id, fresh);
        QCOMPARE(again.document.page(0).objects[2].common.id, core::ObjectId{9});
    }

    // L4-C4 (relecture) : un objet inconnu sans « id », ou d'un « id » que cette version ne sait pas lire (un
    // texte), est lu avec l'identifiant 0 (readId) ; réécrit sans avoir été collé, il garde les octets lus.
    // La clé « id » ne s'écrit que pour un identifiant courant non nul : après withFreshIds, il l'est
    // toujours. Rouge si elle s'écrivait sans condition (« id »: 0 ajouté, ou le texte écrasé par 0).
    void unknownObjectWithoutIdKeepsItsBytes()
    {
        QJsonObject root = formats::documentToJson(plain());
        QJsonArray pages = root.value(u"pages"_s).toArray();
        QJsonObject page = pages.at(0).toObject();
        QJsonArray objects = page.value(u"objects"_s).toArray();
        QJsonObject withoutId = objects.at(0).toObject();
        withoutId.insert(u"type"_s, u"objet-a-venir"_s);
        QVERIFY(withoutId.contains(u"id"_s));
        withoutId.remove(u"id"_s);
        QJsonObject textId = withoutId;
        textId.insert(u"id"_s, u"a1b2"_s);
        objects.append(withoutId);
        objects.append(textId);
        page.insert(u"objects"_s, objects);
        pages.replace(0, page);
        root.insert(u"pages"_s, pages);
        const ReadBack back = readBack(root);
        QVERIFY2(back.result.ok, qPrintable(report(back.result)));
        QCOMPARE(back.document.page(0).objects.size(), std::size_t{4});
        QCOMPARE(back.document.page(0).objects[2].common.id, core::ObjectId{0});
        QCOMPARE(back.document.page(0).objects[3].common.id, core::ObjectId{0});
        const QJsonArray written =
            firstPage(formats::documentToJson(back.document)).value(u"objects"_s).toArray();
        const QByteArray writtenWithoutId =
            QJsonDocument(written.at(2).toObject()).toJson(QJsonDocument::Compact);
        const QByteArray writtenTextId =
            QJsonDocument(written.at(3).toObject()).toJson(QJsonDocument::Compact);
        QCOMPARE(writtenWithoutId, QJsonDocument(withoutId).toJson(QJsonDocument::Compact));
        QCOMPARE(writtenTextId, QJsonDocument(textId).toJson(QJsonDocument::Compact));
    }

    // L4-T27 (D11, amendement 5) : 2 × 1 cadres de 500 × 600 écrits sous « zones », relus tels quels, taille
    // comprise ; un document sans zones n'a pas la clé. Le lecteur passe par clampZones : des valeurs hors
    // bornes dans le fichier sont ramenées (D11, H4-20), jamais gardées telles quelles.
    void zonesRoundTrip()
    {
        core::Document d = plain();
        d.page(0).zones =
            core::PageZones{.horizontal = 2, .vertical = 1, .frameWidth = 500, .frameHeight = 600};
        const QJsonObject root = formats::documentToJson(d);
        const QJsonObject written = firstPage(root).value(u"zones"_s).toObject();
        QCOMPARE(written.size(), qsizetype{4});
        QCOMPARE(written.value(u"horizontal"_s).toInt(), 2);
        QCOMPARE(written.value(u"vertical"_s).toInt(), 1);
        QCOMPARE(written.value(u"frameWidth"_s).toDouble(), 500.0);
        QCOMPARE(written.value(u"frameHeight"_s).toDouble(), 600.0);
        const ReadBack back = readBack(root);
        QVERIFY2(back.result.ok && back.result.warnings.isEmpty(), qPrintable(report(back.result)));
        const core::PageZones& z = back.document.page(0).zones;
        QCOMPARE(z.horizontal, 2);
        QCOMPARE(z.vertical, 1);
        QCOMPARE(z.frameWidth, 500.0);
        QCOMPARE(z.frameHeight, 600.0);
        QCOMPARE(core::fingerprint(back.document), core::fingerprint(d));
        QVERIFY(!firstPage(formats::documentToJson(plain())).contains(u"zones"_s));
        // Par l'archive .actcraft entière (document.json dans le zip) : mêmes zones, même empreinte.
        QString error;
        const QByteArray archive = formats::encodeActcraft(d, {}, &error);
        QVERIFY2(!archive.isEmpty(), qPrintable(error));
        const formats::DecodeResult opened = formats::decodeActcraft(archive);
        QVERIFY2(opened.ok && opened.warnings.isEmpty(),
                 qPrintable(opened.error + opened.warnings.join(u'\n')));
        QCOMPARE(opened.document.page(0).zones.horizontal, 2);
        QCOMPARE(opened.document.page(0).zones.frameHeight, 600.0);
        QCOMPARE(core::fingerprint(opened.document), core::fingerprint(d));

        QJsonObject page = firstPage(root);
        page.insert(u"zones"_s, QJsonObject{{u"horizontal"_s, 99},
                                            {u"vertical"_s, -3},
                                            {u"frameWidth"_s, 400},
                                            {u"frameHeight"_s, -1}});
        QJsonObject wild = root;
        wild.insert(u"pages"_s, QJsonArray{page});
        const ReadBack clamped = readBack(wild);
        QVERIFY2(clamped.result.ok, qPrintable(report(clamped.result)));
        const core::PageZones& c = clamped.document.page(0).zones;
        QCOMPARE(c.horizontal, 40);
        QCOMPARE(c.vertical, 0);
        QCOMPARE(c.frameWidth, 500.0);
        QCOMPARE(c.frameHeight, 0.0);
        QVERIFY2(clamped.document.page(0).unknownKeys.empty(), clamped.document.page(0).unknownKeys.c_str());
    }

    // Valeur de « zones » mal formée (L4-T27) : la règle commune des valeurs lues
    // (invalidValuesWarnAndFallBack) : un avertissement qui la nomme, le défaut, rien de gardé en inconnu.
    // Discrimine le patron écarté d'une « zones » qui n'est pas un objet gardée telle quelle parmi les clés
    // inconnues.
    void invalidZonesWarnAndFallBack()
    {
        const auto readWith = [](const QJsonValue& zones) {
            QJsonObject root = formats::documentToJson(plain());
            QJsonArray pages = root.value(u"pages"_s).toArray();
            QJsonObject page = pages.at(0).toObject();
            page.insert(u"zones"_s, zones);
            pages.replace(0, page);
            root.insert(u"pages"_s, pages);
            return readBack(root);
        };
        for (const QJsonValue& notAnObject :
             {QJsonValue(QJsonArray{QJsonObject{{u"name"_s, u"haut"_s}}}), QJsonValue(3)}) {
            const ReadBack back = readWith(notAnObject);
            QVERIFY2(back.result.ok, qPrintable(back.result.error));
            QCOMPARE(back.result.warnings.size(), qsizetype{1});
            QVERIFY2(back.result.warnings.first().contains(u"« zones »"_s), qPrintable(report(back.result)));
            const core::Page& page = back.document.page(0);
            QVERIFY(core::isDefault(page.zones));
            QVERIFY2(page.unknownKeys.empty(), page.unknownKeys.c_str());
        }
        const ReadBack half = readWith(QJsonObject{
            {u"horizontal"_s, 2.5}, {u"vertical"_s, 2}, {u"frameWidth"_s, 0}, {u"frameHeight"_s, 0}});
        QVERIFY2(half.result.ok, qPrintable(half.result.error));
        QCOMPARE(half.result.warnings.size(), qsizetype{1});
        QVERIFY2(half.result.warnings.first().contains(u"« horizontal »"_s), qPrintable(report(half.result)));
        const core::Page& page = half.document.page(0);
        QCOMPARE(page.zones.horizontal, 0);
        QCOMPARE(page.zones.vertical, 2);
        QVERIFY2(page.unknownKeys.empty(), page.unknownKeys.c_str());
    }

    // Amendement 5 (trouvé par L4-T6) : « zones », omise à zéro, est dans le gabarit everyPageKey avec ses
    // quatre sous-clés hors défaut ; sinon elle serait lue comme inconnue (unknownKeys) et l'empreinte
    // changerait.
    void zonesKeyIsKnown()
    {
        core::Document d = plain();
        d.page(0).zones =
            core::PageZones{.horizontal = 2, .vertical = 3, .frameWidth = 500, .frameHeight = 600};
        ReadBack back = readBack(formats::documentToJson(d));
        QVERIFY2(back.result.ok && back.result.warnings.isEmpty(), qPrintable(report(back.result)));
        core::Page& page = back.document.page(0);
        QVERIFY2(page.unknownKeys.empty(), page.unknownKeys.c_str());
        QCOMPARE(core::fingerprint(back.document), core::fingerprint(d));
        page.zones = core::PageZones{}; // remise au défaut : plus de clé « zones » dans le fichier réécrit
        QVERIFY(!bytes(back.document).contains("\"zones\":"));
    }
};

QTEST_GUILESS_MAIN(TstFormatsLot4)
#include "tst_formats_lot4.moc"
