// Champs du lot 4 en .actcraft (L4-T6) : préréglage et points intelligents d'une forme, texte de haut en bas,
// hauteur d'origine d'une page. Clés facultatives de actcraft-document/1 : écrites seulement hors défaut,
// lues comme des clés connues (jamais rangées parmi les clés inconnues gardées, L2-C8).
#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/formats/nativejson.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

#include <optional>
#include <string>

using namespace actcraft;
using namespace Qt::StringLiterals;

namespace {

// Clés du lot 4, sous la forme compacte « "clé": » : une valeur de texte égale à un nom ne les imite pas.
constexpr const char* kLot4Keys[] = {"\"preset\":", "\"parameters\":", "\"vertical\":", "\"baseHeight\":"};

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
};

QTEST_GUILESS_MAIN(TstFormatsLot4)
#include "tst_formats_lot4.moc"
