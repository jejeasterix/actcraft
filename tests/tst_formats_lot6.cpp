#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/formats/nativejson.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

#include <string>
#include <utility>
#include <vector>

using namespace actcraft;
using namespace Qt::StringLiterals;

namespace {

// Aucune ressource déclarée par ces documents : le chargeur n'est jamais appelé.
formats::LoadedResource noResource(const std::string&, const std::string&)
{
    return std::nullopt;
}

// Une page de deux objets (une forme, un texte), sans aucun champ du lot 6.
core::Document plainDocument()
{
    core::Page page;
    page.name = "Planètes";
    page.notes = "notes";
    core::PageObject shape{core::Shape{}};
    shape.common.id = 1;
    shape.common.label = "Soleil";
    core::PageObject text{core::Text{}};
    text.common.id = 2;
    page.objects = {shape, text};
    return core::Document(std::vector<core::Page>{page});
}

// Écrit en texte compact puis relit, comme un fichier enregistré puis rouvert.
formats::JsonReadResult roundTrip(const core::Document& source, core::Document& back)
{
    const QByteArray text = QJsonDocument(formats::documentToJson(source)).toJson(QJsonDocument::Compact);
    return formats::documentFromJson(QJsonDocument::fromJson(text).object(), noResource, back);
}

QJsonObject pageJsonOf(const core::Document& document)
{
    return formats::documentToJson(document).value(u"pages"_s).toArray().at(0).toObject();
}

QJsonObject objectJsonOf(const core::Document& document, qsizetype index)
{
    return pageJsonOf(document).value(u"objects"_s).toArray().at(index).toObject();
}

} // namespace

// L6-T10 : mots-clés (page, objet), description de page, style de légende. Format natif : clés omises à leur
// valeur par défaut, relues sans perte ; un fichier qui ne les a pas se lit avec les défauts.
class TstFormatsLot6 : public QObject
{
    Q_OBJECT
private slots:
    void keywordsRoundTrip()
    {
        core::Document source = plainDocument();
        source.page(0).keywords = {"soleil", "système", "lune"};
        source.page(0).objects[0].common.keywords = {"étoile", "jaune"};
        const QJsonObject page = pageJsonOf(source);
        QCOMPARE(page.value(u"keywords"_s).toArray(), (QJsonArray{u"soleil"_s, u"système"_s, u"lune"_s}));
        QCOMPARE(objectJsonOf(source, 0).value(u"keywords"_s).toArray(),
                 (QJsonArray{u"étoile"_s, u"jaune"_s}));
        QVERIFY(!objectJsonOf(source, 1).contains(u"keywords"_s)); // l'objet sans mot-clé n'a pas la clé

        core::Document back;
        const auto r = roundTrip(source, back);
        QVERIFY2(r.ok && r.warnings.isEmpty(), qPrintable(r.error + r.warnings.join(u'\n')));
        QCOMPARE(back.page(0).keywords, source.page(0).keywords);
        QCOMPARE(back.page(0).objects[0].common.keywords, source.page(0).objects[0].common.keywords);
        QVERIFY(back.page(0).objects[1].common.keywords.empty());
        QVERIFY(back.page(0).unknownKeys.empty() && back.page(0).objects[0].unknownKeys.empty());
        QCOMPARE(core::fingerprint(back), core::fingerprint(source));
    }

    void pageDescriptionRoundTrip()
    {
        core::Document source = plainDocument();
        source.page(0).description = "Le système solaire\nen deux lignes";
        QCOMPARE(pageJsonOf(source).value(u"description"_s).toString(),
                 u"Le système solaire\nen deux lignes"_s);

        core::Document back;
        const auto r = roundTrip(source, back);
        QVERIFY2(r.ok && r.warnings.isEmpty(), qPrintable(r.error + r.warnings.join(u'\n')));
        QCOMPARE(back.page(0).description, source.page(0).description);
        QVERIFY(back.page(0).unknownKeys.empty());
        QCOMPARE(core::fingerprint(back), core::fingerprint(source));
    }

    void labelStyleRoundTrip()
    {
        core::Document source = plainDocument();
        core::LabelStyle& style = source.page(0).objects[0].common.labelStyle;
        style.fontFamily = "DejaVu Sans";
        style.pointSize = 18.5;
        style.color = 0xFFFF8000u;
        style.outline = core::PenStyle::Dash;
        style.outlineColor = 0x80112233u;
        style.fill = 0x40FFFFFFu;
        source.page(0).objects[1].common.labelStyle.pointSize = 9; // un seul champ hors défaut
        const QJsonObject written = objectJsonOf(source, 0).value(u"labelStyle"_s).toObject();
        QCOMPARE(written.value(u"fontFamily"_s).toString(), u"DejaVu Sans"_s);
        QCOMPARE(written.value(u"pointSize"_s).toDouble(), 18.5);
        QCOMPARE(written.value(u"color"_s).toString(), u"#ffff8000"_s);
        QCOMPARE(written.value(u"outlineColor"_s).toString(), u"#80112233"_s);
        QCOMPARE(written.value(u"fill"_s).toString(), u"#40ffffff"_s);
        QVERIFY(written.value(u"outline"_s).isString());

        core::Document back;
        const auto r = roundTrip(source, back);
        QVERIFY2(r.ok && r.warnings.isEmpty(), qPrintable(r.error + r.warnings.join(u'\n')));
        QVERIFY(back.page(0).objects[0].common.labelStyle == style);
        QVERIFY(back.page(0).objects[1].common.labelStyle == source.page(0).objects[1].common.labelStyle);
        QVERIFY(back.page(0).objects[0].unknownKeys.empty() && back.page(0).objects[1].unknownKeys.empty());
        QCOMPARE(core::fingerprint(back), core::fingerprint(source));
    }

    void labelStyleUnknownKeysKept()
    {
        // Une sous-clé inconnue de « labelStyle » va au style, pas à l'objet, et est réécrite avec lui.
        core::Document source = plainDocument();
        source.page(0).objects[0].common.labelStyle.pointSize = 18;
        QJsonObject root = formats::documentToJson(source);
        QJsonArray pages = root.value(u"pages"_s).toArray();
        QJsonObject page = pages.at(0).toObject();
        QJsonArray objects = page.value(u"objects"_s).toArray();
        QJsonObject object = objects.at(0).toObject();
        QJsonObject style = object.value(u"labelStyle"_s).toObject();
        style.insert(u"futureKey"_s, 7);
        object.insert(u"labelStyle"_s, style);
        objects.replace(0, object);
        page.insert(u"objects"_s, objects);
        pages.replace(0, page);
        root.insert(u"pages"_s, pages);

        core::Document back;
        const auto r = formats::documentFromJson(root, noResource, back);
        QVERIFY2(r.ok && r.warnings.isEmpty(), qPrintable(r.error + r.warnings.join(u'\n')));
        const core::PageObject& read = back.page(0).objects[0];
        QVERIFY(read.common.labelStyle.unknownKeys.find("futureKey") != std::string::npos);
        QVERIFY(read.unknownKeys.empty());
        QCOMPARE(read.common.labelStyle.pointSize, 18.0);
        const QJsonObject again = objectJsonOf(back, 0).value(u"labelStyle"_s).toObject();
        QCOMPARE(again.value(u"futureKey"_s).toInt(), 7);
        QCOMPARE(again.value(u"pointSize"_s).toDouble(), 18.0);

        // Même remis aux valeurs par défaut, le style qui porte une clé inconnue est réécrit.
        back.page(0).objects[0].common.labelStyle.pointSize = core::LabelStyle{}.pointSize;
        QCOMPARE(objectJsonOf(back, 0).value(u"labelStyle"_s).toObject().value(u"futureKey"_s).toInt(), 7);
    }

    void defaultsAreOmitted()
    {
        const core::Document plain = plainDocument();
        const QJsonObject page = pageJsonOf(plain);
        QVERIFY(!page.contains(u"keywords"_s));
        QVERIFY(!page.contains(u"description"_s));
        for (qsizetype i = 0; i < 2; ++i) {
            const QJsonObject object = objectJsonOf(plain, i);
            QVERIFY(!object.contains(u"keywords"_s));
            QVERIFY(!object.contains(u"labelStyle"_s));
        }

        // Lues puis remises au défaut, les clés ne reviennent pas : elles ne sont pas gardées comme
        // inconnues.
        core::Document source = plainDocument();
        source.page(0).keywords = {"soleil"};
        source.page(0).description = "description";
        source.page(0).objects[0].common.keywords = {"étoile"};
        source.page(0).objects[0].common.labelStyle.pointSize = 30;
        core::Document back;
        const auto r = roundTrip(source, back);
        QVERIFY2(r.ok && r.warnings.isEmpty(), qPrintable(r.error + r.warnings.join(u'\n')));
        back.page(0).keywords.clear();
        back.page(0).description.clear();
        back.page(0).objects[0].common.keywords.clear();
        back.page(0).objects[0].common.labelStyle = core::LabelStyle{};
        const QJsonObject reset = pageJsonOf(back);
        QVERIFY(!reset.contains(u"keywords"_s));
        QVERIFY(!reset.contains(u"description"_s));
        QVERIFY(!objectJsonOf(back, 0).contains(u"keywords"_s));
        QVERIFY(!objectJsonOf(back, 0).contains(u"labelStyle"_s));
        QCOMPARE(core::fingerprint(back), core::fingerprint(plain));
    }

    void olderFileReadsWithDefaults()
    {
        // Un fichier d'avant L6-T10 : le JSON d'un document riche en champs du lot 6, ces clés retirées.
        core::Document rich = plainDocument();
        rich.page(0).keywords = {"soleil"};
        rich.page(0).description = "description";
        rich.page(0).objects[0].common.keywords = {"étoile"};
        rich.page(0).objects[0].common.labelStyle.color = 0xFF0000FFu;
        QJsonObject root = formats::documentToJson(rich);
        QJsonArray pages = root.value(u"pages"_s).toArray();
        QJsonObject page = pages.at(0).toObject();
        QVERIFY(page.contains(u"keywords"_s) && page.contains(u"description"_s));
        page.remove(u"keywords"_s);
        page.remove(u"description"_s);
        QJsonArray objects = page.value(u"objects"_s).toArray();
        QJsonObject object = objects.at(0).toObject();
        QVERIFY(object.contains(u"keywords"_s) && object.contains(u"labelStyle"_s));
        object.remove(u"keywords"_s);
        object.remove(u"labelStyle"_s);
        objects.replace(0, object);
        page.insert(u"objects"_s, objects);
        pages.replace(0, page);
        root.insert(u"pages"_s, pages);

        core::Document back;
        const auto r = formats::documentFromJson(root, noResource, back);
        QVERIFY2(r.ok && r.warnings.isEmpty(), qPrintable(r.error + r.warnings.join(u'\n')));
        QVERIFY(back.page(0).keywords.empty());
        QVERIFY(back.page(0).description.empty());
        for (const core::PageObject& read : back.page(0).objects) {
            QVERIFY(read.common.keywords.empty());
            QVERIFY(read.common.labelStyle == core::LabelStyle{});
            QVERIFY(read.unknownKeys.empty());
        }
        QVERIFY(back.page(0).unknownKeys.empty());
        QCOMPARE(core::fingerprint(back), core::fingerprint(plainDocument()));
    }
};

QTEST_GUILESS_MAIN(TstFormatsLot6)
#include "tst_formats_lot6.moc"
