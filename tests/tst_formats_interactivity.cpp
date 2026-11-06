#include "support/richdocument.h"

#include <actcraft/core/interactivity.h>
#include <actcraft/formats/nativejson.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

using namespace actcraft;
using namespace Qt::StringLiterals;

// Format natif des champs de l'interactivité (L8-T4). Toutes les clés du lot 8 sont facultatives : écrites
// hors défaut seulement. Le gabarit des clés connues doit donc les contenir (comme everyKeyObject pour celles
// du lot 5 et de L6-T10) : tiré d'un objet, d'une action ou d'un document vierges, il ne les a pas, une clé
// lue irait aussi aux clés inconnues et reviendrait au fichier une fois le champ remis à son défaut. Chaque
// aller-retour ci-dessous le vérifie (clés inconnues vides, clé absente après remise au défaut).
class TstFormatsInteractivity : public QObject
{
    Q_OBJECT

    // m9 : API réelle du format natif (nativejson.h) ; chargeur de ressources vide.
    static QByteArray toJson(const core::Document& d)
    {
        return QJsonDocument(formats::documentToJson(d)).toJson(QJsonDocument::Compact);
    }
    static bool fromJson(const QByteArray& json, core::Document& out, QString& error)
    {
        const formats::JsonReadResult result =
            formats::documentFromJson(QJsonDocument::fromJson(json).object(), formats::ResourceLoader{}, out);
        error = result.error;
        return result.ok;
    }
    static core::Document base()
    {
        core::Document d(1024, 768);
        core::PageObject o{core::Shape{}};
        o.common.id = 1;
        d.page(0).objects.push_back(o);
        return d;
    }
    static core::Document roundTrip(const core::Document& d)
    {
        QString error;
        const QByteArray json = toJson(d);
        core::Document back;
        [&] { QVERIFY2(fromJson(json, back, error), qPrintable(error)); }();
        return back;
    }
    static void verifyAbsent(const QByteArray& json, std::initializer_list<const char*> keys)
    {
        for (const char* key : keys) {
            const QByteArray quoted = QByteArray("\"") + key + "\"";
            QVERIFY2(!json.contains(quoted), key);
        }
    }

private slots:
    void actionRoundTrip()
    {
        core::Document d = base();
        core::Action a;
        a.catalogId = "page.goto";
        a.parameters = R"({"page":3})";
        a.dragDrop = true;
        d.page(0).objects[0].common.actions.push_back(a);
        core::Action plain;
        plain.catalogId = "link.open";
        plain.parameters = "texte libre"; // pas du JSON : écrit comme une chaîne
        d.page(0).objects[0].common.actions.push_back(plain);
        core::Action unordered; // JSON lisible mais pas sous sa forme compacte : écrit comme une chaîne
        unordered.catalogId = "page.goto";
        unordered.parameters = R"({"b":1,"a":2})";
        d.page(0).objects[0].common.actions.push_back(unordered);
        core::Action list; // tableau JSON compact : inséré tel quel
        list.catalogId = "page.goto";
        list.parameters = "[1,2]";
        d.page(0).objects[0].common.actions.push_back(list);
        core::Document back = roundTrip(d);
        QCOMPARE(core::fingerprint(back), core::fingerprint(d));
        auto& actions = back.page(0).objects[0].common.actions;
        QCOMPARE(actions.size(), std::size_t{4});
        QCOMPARE(QString::fromStdString(actions[0].catalogId), u"page.goto"_s);
        QCOMPARE(QString::fromStdString(actions[0].parameters), uR"({"page":3})"_s);
        QVERIFY(actions[0].dragDrop);
        QCOMPARE(QString::fromStdString(actions[1].parameters), u"texte libre"_s);
        QCOMPARE(QString::fromStdString(actions[2].parameters), uR"({"b":1,"a":2})"_s);
        QCOMPARE(QString::fromStdString(actions[3].parameters), u"[1,2]"_s);
        const QByteArray asText = "\"parameters\":\"{\\\"b\\\":1,\\\"a\\\":2}\"";
        QVERIFY(toJson(d).contains(asText));
        QVERIFY(toJson(d).contains(R"("parameters":[1,2])"));
        // Paramètres objet : insérés tels quels, pas en chaîne échappée.
        QVERIFY(toJson(d).contains(R"("parameters":{"page":3})"));
        QVERIFY2(actions[0].unknownKeys.empty(), actions[0].unknownKeys.c_str());
        for (core::Action& action : actions) {
            action.catalogId.clear();
            action.parameters.clear();
            action.dragDrop = false;
        }
        verifyAbsent(toJson(back), {"catalog-id", "drag-drop"});
    }
    void containerRoundTrip()
    {
        core::Document d = base();
        auto& c = d.page(0).objects[0].common.container;
        c.accepts = core::Accepts::Keywords;
        c.keywords = {"fruit"};
        c.fit = core::FitRule::FullyInside;
        c.tolerance = 4;
        c.bounceBack = true;
        c.chime = true;
        c.chimeSound = "son";
        core::Document back = roundTrip(d);
        QCOMPARE(core::fingerprint(back), core::fingerprint(d));
        auto& object = back.page(0).objects[0];
        QCOMPARE(object.common.container.keywords, std::vector<std::string>{"fruit"});
        QVERIFY2(object.unknownKeys.empty(), object.unknownKeys.c_str());
        object.common.container = core::ContainerRule{};
        verifyAbsent(toJson(back), {"container", "bounce-back"});
        // Un drapeau à la fois : deux champs frères échangés dans l'écrivain ou le lecteur garderaient
        // l'empreinte d'une règle où les deux sont vrais.
        for (int k = 0; k < 2; ++k) {
            core::Document one = base();
            auto& only = one.page(0).objects[0].common.container;
            only.bounceBack = k == 0;
            only.chime = k == 1;
            QCOMPARE(core::fingerprint(roundTrip(one)), core::fingerprint(one));
        }
    }
    void restrictorsRoundTrip()
    {
        core::Document d = base();
        auto& r = d.page(0).objects[0].common.restrictions;
        core::setTravel(r, core::Travel::AlongTrack);
        r.track = 4;
        core::setTurning(r, core::Turning::Clockwise);
        r.turnStep = 45;
        r.pivot = core::Mark::Point;
        r.pivotPoint = {3, 5};
        r.pivotObject = 7;
        r.blocksOthers = true;
        r.magnet = true;
        r.magnetMark = core::Mark::FirstTextLine;
        r.magnetPoint = {11, 13};
        core::Document back = roundTrip(d);
        QCOMPARE(core::fingerprint(back), core::fingerprint(d));
        auto& object = back.page(0).objects[0];
        QCOMPARE(object.common.restrictions.travel, core::Travel::AlongTrack);
        QCOMPARE(object.common.restrictions.turning, core::Turning::Clockwise);
        QCOMPARE(object.common.restrictions.magnetMark, core::Mark::FirstTextLine);
        QVERIFY2(object.unknownKeys.empty(), object.unknownKeys.c_str());
        object.common.restrictions = core::Restrictions{};
        verifyAbsent(toJson(back),
                     {"travel", "track", "turning", "turn-step", "pivot", "pivot-point", "pivot-object",
                      "blocks-others", "magnet", "magnet-mark", "magnet-point"});
        for (int k = 0; k < 2; ++k) { // un drapeau à la fois (champs frères)
            core::Document one = base();
            auto& only = one.page(0).objects[0].common.restrictions;
            only.blocksOthers = k == 0;
            only.magnet = k == 1;
            QCOMPARE(core::fingerprint(roundTrip(one)), core::fingerprint(one));
        }

        // Objet bloqué du lot 2 (déplacement et rotation interdits) : mode Fixed, aucune clé du lot 8.
        core::Document locked = base();
        auto& l = locked.page(0).objects[0].common.restrictions;
        l.movable = false;
        l.rotatable = false;
        const QByteArray json = toJson(locked);
        verifyAbsent(json, {"travel", "turning"});
        core::Document lockedBack;
        QString error;
        QVERIFY2(fromJson(json, lockedBack, error), qPrintable(error));
        QCOMPARE(lockedBack.page(0).objects[0].common.restrictions.travel, core::Travel::Fixed);
        QCOMPARE(lockedBack.page(0).objects[0].common.restrictions.turning, core::Turning::Fixed);
        QCOMPARE(core::fingerprint(lockedBack), core::fingerprint(locked));
    }
    void mediaOptionsRoundTrip()
    {
        core::Document d = base();
        core::Media m;
        m.controlsHidden = true;
        m.aspectLocked = true;
        m.clearBackground = true;
        m.wrapText = true;
        core::PageObject o{m};
        o.common.id = 2;
        d.page(0).objects.push_back(o);
        core::Document back = roundTrip(d);
        QCOMPARE(core::fingerprint(back), core::fingerprint(d));
        auto& object = back.page(0).objects[1];
        QVERIFY2(object.unknownKeys.empty(), object.unknownKeys.c_str());
        auto* media = std::get_if<core::Media>(&core::body(object));
        QVERIFY(media);
        QVERIFY(media->controlsHidden && media->aspectLocked && media->clearBackground && media->wrapText);
        *media = core::Media{};
        verifyAbsent(toJson(back), {"controls-hidden", "aspect-locked", "clear-background", "wrap-text"});
        for (int k = 0; k < 4; ++k) { // un drapeau à la fois (champs frères)
            core::Document one = base();
            core::Media only;
            only.controlsHidden = k == 0;
            only.aspectLocked = k == 1;
            only.clearBackground = k == 2;
            only.wrapText = k == 3;
            core::PageObject po{only};
            po.common.id = 2;
            one.page(0).objects.push_back(po);
            QCOMPARE(core::fingerprint(roundTrip(one)), core::fingerprint(one));
        }
    }
    void webFrameRoundTrip()
    {
        core::Document d = base();
        core::WebFrame w;
        w.bounds = {10, 10, 300, 200};
        w.html = "<p>essai</p>";
        w.address = "https://exemple.org/";
        core::PageObject o{w};
        o.common.id = 3;
        d.page(0).objects.push_back(o);
        const QByteArray json = toJson(d);
        QVERIFY(json.contains(R"("type":"web-frame")"));
        core::Document back = roundTrip(d);
        QCOMPARE(core::kindOf(back.page(0).objects[1]), core::ObjectKind::WebFrame);
        QCOMPARE(core::fingerprint(back), core::fingerprint(d));
        const auto* frame = std::get_if<core::WebFrame>(&core::body(back.page(0).objects[1]));
        QVERIFY(frame);
        QCOMPARE(QString::fromStdString(frame->html), u"<p>essai</p>"_s);
        QCOMPARE(QString::fromStdString(frame->address), u"https://exemple.org/"_s);
        QVERIFY2(back.page(0).objects[1].unknownKeys.empty(), back.page(0).objects[1].unknownKeys.c_str());
    }
    void broadcastRoundTrip()
    {
        core::Document d = base();
        core::BroadcastRules rules;
        rules.noCopy = true;
        rules.noSave = true;
        rules.noPrint = true;
        rules.noCamera = true;
        rules.author = "auteur";
        d.setBroadcast(rules);
        core::Document back = roundTrip(d);
        QCOMPARE(core::fingerprint(back), core::fingerprint(d));
        QVERIFY(back.broadcast().noCopy && back.broadcast().noSave && back.broadcast().noPrint &&
                back.broadcast().noCamera);
        QCOMPARE(QString::fromStdString(back.broadcast().author), u"auteur"_s);
        QVERIFY2(back.unknownKeys().empty(), back.unknownKeys().c_str());
        back.setBroadcast(core::BroadcastRules{});
        verifyAbsent(toJson(back), {"broadcast", "no-copy"});
        for (int k = 0; k < 4; ++k) { // un drapeau à la fois (champs frères)
            core::Document one = base();
            core::BroadcastRules only;
            only.noCopy = k == 0;
            only.noSave = k == 1;
            only.noPrint = k == 2;
            only.noCamera = k == 3;
            one.setBroadcast(only);
            QCOMPARE(core::fingerprint(roundTrip(one)), core::fingerprint(one));
        }
    }
    void defaultsAreOmitted()
    {
        const QByteArray json = toJson(base());
        for (const char* key : {"container", "catalog-id", "drag-drop", "travel", "turning", "blocks-others",
                                "broadcast", "controls-hidden"})
            QVERIFY2(!json.contains(key), key);

        // Hors défaut, noms figés des clés et des valeurs du lot 8 : une clé renommée des deux côtés
        // garderait l'aller-retour, pas les fichiers déjà écrits.
        const QJsonObject root = formats::documentToJson(richdoc::richInteractiveDocument());
        const QJsonArray objects =
            root.value(u"pages"_s).toArray().at(0).toObject().value(u"objects"_s).toArray();
        const QJsonObject o1 = objects.at(0).toObject();
        const QJsonObject jr = o1.value(u"restrictions"_s).toObject();
        QCOMPARE(jr.value(u"travel"_s).toString(), u"along-track"_s);
        QCOMPARE(jr.value(u"turning"_s).toString(), u"counter-clockwise"_s);
        QCOMPARE(jr.value(u"turn-step"_s).toDouble(), 15.0);
        QCOMPARE(jr.value(u"pivot"_s).toString(), u"other-object"_s);
        QCOMPARE(jr.value(u"magnet-mark"_s).toString(), u"bottom-right"_s);
        QVERIFY(jr.value(u"blocks-others"_s).toBool());
        const QJsonObject box = o1.value(u"container"_s).toObject();
        QVERIFY(box.value(u"bounce-back"_s).toBool());
        QCOMPARE(box.value(u"accepts"_s).toString(), u"keywords"_s);
        QCOMPARE(box.value(u"fit"_s).toString(), u"fully-inside"_s);
        const QJsonObject action = o1.value(u"actions"_s).toArray().at(0).toObject();
        QCOMPARE(action.value(u"catalog-id"_s).toString(), u"page.goto"_s);
        QVERIFY(action.value(u"drag-drop"_s).toBool());
        const QJsonObject clip = objects.at(6).toObject();
        QVERIFY(clip.value(u"controls-hidden"_s).toBool() && clip.value(u"aspect-locked"_s).toBool() &&
                clip.value(u"clear-background"_s).toBool() && clip.value(u"wrap-text"_s).toBool());
        const QJsonObject rules = root.value(u"broadcast"_s).toObject();
        QVERIFY(rules.value(u"no-copy"_s).toBool() && rules.value(u"no-camera"_s).toBool());
        QCOMPARE(rules.value(u"author"_s).toString(), u"Auteur"_s);
    }
    void olderFileReadsWithDefaults()
    {
        const QByteArray json = toJson(base()); // forme du lot 2 : aucun champ du lot 8
        core::Document back;
        QString error;
        QVERIFY(fromJson(json, back, error));
        QVERIFY(core::isDefault(back.page(0).objects[0].common.container));
        QVERIFY(core::isDefault(back.page(0).objects[0].common.restrictions));
        QVERIFY(back.broadcast().isDefault());
        QVERIFY(back.page(0).objects[0].unknownKeys.empty());
    }
    void unknownActionKeyKept()
    {
        core::Document d = base();
        core::Action a;
        a.catalogId = "page.next";
        d.page(0).objects[0].common.actions.push_back(a);
        QByteArray json = toJson(d);
        json.replace("\"catalog-id\"", "\"future-key\":42,\"catalog-id\"");
        core::Document back;
        QString error;
        QVERIFY(fromJson(json, back, error));
        QVERIFY(toJson(back).contains("\"future-key\":42"));
        QCOMPARE(QString::fromStdString(back.page(0).objects[0].common.actions[0].catalogId), u"page.next"_s);
    }
};

QTEST_MAIN(TstFormatsInteractivity)
#include "tst_formats_interactivity.moc"
