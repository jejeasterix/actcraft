#include "support/richdocument.h"

#include <actcraft/core/document.h>
#include <actcraft/core/interactivity.h>
#include <actcraft/core/model.h>

#include <QtTest>

#include <cstdint>

using namespace actcraft::core;

namespace {

// Empreinte du document d'unknownObjectFingerprintUnchanged, relevée à la tête avant L8-T3 (c61013da, étape 1
// bis) : un objet inconnu garde son empreinte quand le cadre web prend le rang 9 de ObjectKind.
constexpr std::uint64_t kFingerprintBeforeLot8 = 0x0608DB86176E5A60ull;
// Le même document où le cadre web remplace l'objet inconnu, haché avec son code 10 (avec le code 9, la
// valeur diffère : mutation vérifiée par la revue de L8-T3).
constexpr std::uint64_t kWebFrameFingerprint = 0xB9B98F43167E3DFFull;

} // namespace

class TstCoreInteractivity : public QObject
{
    Q_OBJECT
    static Document oneShape()
    {
        Document d(1024, 768);
        PageObject o{Shape{}};
        o.common.id = 7;
        d.page(0).objects.push_back(o);
        return d;
    }
private slots:
    void defaultsChangeNoFingerprint()
    {
        const Document d = oneShape();
        const auto before = fingerprint(d);
        Document e = d;
        e.page(0).objects[0].common.container = ContainerRule{};
        e.page(0).objects[0].common.restrictions = Restrictions{};
        e.setBroadcast(BroadcastRules{});
        QCOMPARE(fingerprint(e), before);
        e.page(0).objects[0].common.container.accepts = Accepts::Anything;
        QVERIFY(fingerprint(e) != before);
        e.page(0).objects[0].common.container.accepts = Accepts::Nothing;
        QCOMPARE(fingerprint(e), before);
    }
    void fixedTravelMeansNotMovable()
    {
        Restrictions r;
        setTravel(r, Travel::Fixed);
        QVERIFY(!r.movable);
        setTravel(r, Travel::Vertical);
        QVERIFY(r.movable);
        Restrictions fromFile;
        fromFile.movable = false; // lu d'un fichier du lot 2
        normalize(fromFile);
        QCOMPARE(fromFile.travel, Travel::Fixed);
        Restrictions byMode;
        byMode.travel = Travel::Fixed; // mode Fixed posé seul : le booléen suit
        normalize(byMode);
        QVERIFY(!byMode.movable);
    }
    void fixedTurningMeansNotRotatable()
    {
        Restrictions r;
        setTurning(r, Turning::Fixed);
        QVERIFY(!r.rotatable);
        setTurning(r, Turning::Clockwise);
        QVERIFY(r.rotatable);
        Restrictions fromFile;
        fromFile.rotatable = false;
        normalize(fromFile);
        QCOMPARE(fromFile.turning, Turning::Fixed);
        Restrictions byMode;
        byMode.turning = Turning::Fixed;
        normalize(byMode);
        QVERIFY(!byMode.rotatable);
    }
    void webFrameKindBeforeUnknown()
    {
        QCOMPARE(static_cast<int>(ObjectKind::WebFrame), 9);
        QCOMPARE(static_cast<int>(ObjectKind::Unknown), 10);
        QCOMPARE(kObjectKindCount, std::size_t(11));
        PageObject w{WebFrame{}};
        QCOMPARE(kindOf(w), ObjectKind::WebFrame);
        PageObject u{UnknownObject{}};
        QCOMPARE(kindOf(u), ObjectKind::Unknown);
    }
    void unknownObjectFingerprintUnchanged()
    {
        // m6 : kindOf est haché (model.cpp) ; Unknown passe de 9 à 10. L'empreinte hache pour UnknownObject
        // le code stable 9 (constante kUnknownKindTag) : la valeur relevée avant L8-T3 (étape 1 bis) ne
        // change pas.
        Document d(1024, 768);
        PageObject u{UnknownObject{}};
        u.common.id = 3;
        d.page(0).objects.push_back(u);
        QCOMPARE(fingerprint(d), kFingerprintBeforeLot8);

        // Le cadre web a son propre code : il ne se confond pas avec un objet inconnu.
        Document web(1024, 768);
        PageObject w{WebFrame{}};
        w.common.id = 3;
        web.page(0).objects.push_back(w);
        QCOMPARE(fingerprint(web), kWebFrameFingerprint);
    }
    void normalizeKeepsFingerprint()
    {
        // Revue de L8-T3 : formats/ normalise après lecture (L8-T4) ; un objet bloqué au lot 2 (booléen faux,
        // mode par défaut) garde son empreinte une fois normalisé, et le mode Fixed seul vaut le booléen
        // faux.
        Document rich = richdoc::richDocument();
        const auto richBefore = fingerprint(rich);
        for (std::size_t i = 0; i < rich.pageCount(); ++i) {
            for (PageObject& object : rich.page(i).objects) {
                normalize(object.common.restrictions);
            }
        }
        QCOMPARE(rich.page(0).objects[0].common.restrictions.travel, Travel::Fixed); // o1 : movable faux
        QCOMPARE(fingerprint(rich), richBefore);

        Document blocked = oneShape();
        blocked.page(0).objects[0].common.restrictions.movable = false;
        blocked.page(0).objects[0].common.restrictions.rotatable = false;
        const auto blockedBefore = fingerprint(blocked);
        Document normalized = blocked;
        normalize(normalized.page(0).objects[0].common.restrictions);
        QCOMPARE(normalized.page(0).objects[0].common.restrictions.travel, Travel::Fixed);
        QCOMPARE(normalized.page(0).objects[0].common.restrictions.turning, Turning::Fixed);
        QCOMPARE(fingerprint(normalized), blockedBefore);
        Document byMode = oneShape();
        byMode.page(0).objects[0].common.restrictions.travel = Travel::Fixed;
        byMode.page(0).objects[0].common.restrictions.turning = Turning::Fixed;
        QCOMPARE(fingerprint(byMode), blockedBefore);
    }
    void actionKeepsFlipchartFields()
    {
        Action a;
        a.kind = ActionKind::OnObject;
        a.sourceType = 28;
        a.command = 184;
        a.target = 9;
        QVERIFY(a.catalogId.empty());
        QVERIFY(!isSupported(a));
        a.catalogId = "object.toggle-hidden";
        QVERIFY(isSupported(a));
        QCOMPARE(a.command, 184); // jamais réécrit
    }
    void broadcastDefaultsAllowEverything()
    {
        const BroadcastRules b;
        QVERIFY(b.isDefault());
        QVERIFY(!b.noCopy && !b.noSave && !b.noPrint && !b.noCamera);
        QVERIFY(b.author.empty());
        const Document d(1024, 768);
        QVERIFY(d.broadcast().isDefault());
    }
};

QTEST_MAIN(TstCoreInteractivity)
#include "tst_core_interactivity.moc"
