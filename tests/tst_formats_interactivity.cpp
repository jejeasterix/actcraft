#include <actcraft/core/interactivity.h>
#include <actcraft/formats/nativejson.h>

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
        core::Document back = roundTrip(d);
        QCOMPARE(core::fingerprint(back), core::fingerprint(d));
        auto& actions = back.page(0).objects[0].common.actions;
        QCOMPARE(actions.size(), std::size_t{2});
        QCOMPARE(QString::fromStdString(actions[0].catalogId), u"page.goto"_s);
        QCOMPARE(QString::fromStdString(actions[0].parameters), uR"({"page":3})"_s);
        QVERIFY(actions[0].dragDrop);
        QCOMPARE(QString::fromStdString(actions[1].parameters), u"texte libre"_s);
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
    }
    void defaultsAreOmitted()
    {
        const QByteArray json = toJson(base());
        for (const char* key : {"container", "catalog-id", "drag-drop", "travel", "turning", "blocks-others",
                                "broadcast", "controls-hidden"})
            QVERIFY2(!json.contains(key), key);
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
