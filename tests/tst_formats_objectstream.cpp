#include "support/flipchartwriter.h"

#include <actcraft/formats/objectstream.h>

#include <QTest>

#include <cmath>
#include <limits>

using namespace actcraft::formats::flipchart;
using namespace Qt::StringLiterals;

// Flux objet (format-flipchart.md § 4) : chaque type de variante et de sous-type, saut des propriétés
// illisibles.
class TstFormatsObjectStream : public QObject
{
    Q_OBJECT

private slots:
    void readsEveryScalarType()
    {
        const QByteArray data = fctest::Obj(7, 335)
                                    .boolean(2, true)
                                    .integer(3, -5)
                                    .uinteger(4, 4000000000u)
                                    .real(5, 2.5)
                                    .string(6, u"Forme24"_s)
                                    .stringList(7, {u"a"_s, u"b"_s})
                                    .bytes(8, "xyz")
                                    .floatRect(9, 1, 2, 3, 4)
                                    .size(10, 320, 240)
                                    .sizeF(11, 1024, 768)
                                    .pointF(12, 5, 6)
                                    .color(13, 10, 20, 30, 128)
                                    .owner(0x67)
                                    .stream();
        QString error;
        const auto o = parseObjectStream(data, error);
        QVERIFY2(o, qPrintable(error));
        QCOMPARE(o->type, quint16{7});
        QCOMPARE(o->storageId, quint32{335});
        QCOMPARE(o->nodeId.value_or(0), quint32{0x67});
        QCOMPARE(asBool(o->find(2)).value_or(false), true);
        QCOMPARE(asNumber(o->find(3)).value_or(0), -5.0);
        QCOMPARE(asNumber(o->find(4)).value_or(0), 4000000000.0);
        QCOMPARE(asNumber(o->find(5)).value_or(0), 2.5);
        QCOMPARE(asString(o->find(6)), u"Forme24"_s);
        QCOMPARE(std::get<QStringList>(o->find(7)->data), (QStringList{u"a"_s, u"b"_s}));
        QCOMPARE(std::get<QByteArray>(o->find(8)->data), QByteArray("xyz"));
        QCOMPARE(asRect(o->find(9))->width, 3.0);
        QCOMPARE(asSize(o->find(10))->width, 320.0);
        QCOMPARE(asSize(o->find(11))->height, 768.0);
        QCOMPARE(asPoint(o->find(12))->y, 6.0);
        const Color c = asColor(o->find(13)).value_or(Color{});
        QCOMPARE(c.r, quint8{10});
        QCOMPARE(c.a, quint8{128});
        QVERIFY(o->find(99) == nullptr);
        QVERIFY(!asNumber(o->find(6))); // une chaîne n'est pas un nombre
    }

    void readsUserSubtypes()
    {
        const fctest::Obj ink = fctest::Obj(20, 0).real(3, 50).color(7, 255, 255, 0, 128);
        const fctest::Obj run = fctest::Obj(31, 0).string(6, u"Bonjour"_s);
        const QByteArray data = fctest::Obj(3, 111)
                                    .points(2, {QPointF(0, 0), QPointF(100, 0), QPointF(100, 50)})
                                    .polygons(3, {{QPointF(0, 0), QPointF(1, 1)}, {QPointF(2, 2)}})
                                    .resources(4, {fctest::Res(165, u"165.ir"_s, u"115.png"_s, u"7370"_s, 1)})
                                    .object(5, 6, ink)
                                    .userRect(6, -3, -3, 107.5, 107.5)
                                    .pointer(7, 116, 117, 11)
                                    .runs(8, {run, run})
                                    .stream();
        QString error;
        const auto o = parseObjectStream(data, error);
        QVERIFY2(o, qPrintable(error));
        QCOMPARE(asPolyline(o->find(2)).size(), std::size_t{3});
        QCOMPARE(asPolyline(o->find(2))[2].y, 50.0);
        QCOMPARE(asPolylines(o->find(3)).size(), std::size_t{2});
        QCOMPARE(asPolylines(o->find(3))[1].size(), std::size_t{1});
        const auto items = asResourceItems(o->find(4));
        QCOMPARE(items.size(), std::size_t{1});
        QCOMPARE(items[0].id, quint32{165});
        QCOMPARE(items[0].dataFile, u"115.png"_s);
        const StreamObject* embedded = asObject(o->find(5));
        QVERIFY(embedded);
        QCOMPARE(embedded->type, quint16{20});
        QCOMPARE(asNumber(embedded->find(3)).value_or(0), 50.0);
        QCOMPARE(asRect(o->find(6))->width, 107.5);
        const Pointer p = asPointer(o->find(7)).value_or(Pointer{});
        QCOMPARE(p.nodeId, quint32{116});
        QCOMPARE(p.type, quint16{11});
        QVERIFY(!p.isNull());
        QCOMPARE(asObjects(o->find(8)).size(), std::size_t{2});
        QCOMPARE(asString(asObjects(o->find(8))[0]->find(6)), u"Bonjour"_s);
    }

    void unreadablePropertyBecomesRawAndTheRestIsRead()
    {
        // Variante de type 70 (QImage, inconnue) puis une propriété lisible : la table M permet de sauter.
        fctest::Bytes unknown;
        unknown.u16(70).raw("????");
        // Chaîne qui annonce plus d'octets qu'elle n'en a : illisible, gardée brute.
        fctest::Bytes broken;
        broken.u16(10).u16(1).u32(1000).raw("ab");
        const QByteArray data =
            fctest::Obj(9, 1).raw(2, unknown.data()).raw(3, broken.data()).integer(4, 42).stream();
        QString error;
        const auto o = parseObjectStream(data, error);
        QVERIFY2(o, qPrintable(error));
        QVERIFY(std::holds_alternative<RawValue>(o->find(2)->data));
        QCOMPARE(std::get<RawValue>(o->find(2)->data).qtype, quint16{70});
        QVERIFY(std::holds_alternative<RawValue>(o->find(3)->data));
        QCOMPARE(asNumber(o->find(4)).value_or(0), 42.0);
    }

    void nullStringAndNullPointer()
    {
        fctest::Bytes nullString;
        nullString.u16(10).u16(1).nullQstr();
        const QByteArray data = fctest::Obj(2, 1).raw(2, nullString.data()).pointer(3, 0, 0, 0).stream();
        QString error;
        const auto o = parseObjectStream(data, error);
        QVERIFY(o);
        QVERIFY(asString(o->find(2)).isNull());
        QVERIFY(asPointer(o->find(3))->isNull());
    }

    void badStreamsAreRefused()
    {
        QString error;
        QVERIFY(!parseObjectStream("pas un flux", error));
        QVERIFY(error.contains(u"signature"_s));
        const QByteArray good = fctest::Obj(7, 1).integer(2, 1).stream();
        QVERIFY(!parseObjectStream(good.left(15), error));
        fctest::Bytes wrongTag;
        wrongTag.raw(QByteArray(" Bamboo ", 8)).u16(1).u16(7).tag('X');
        QVERIFY(!parseObjectStream(wrongTag.data(), error));
        QVERIFY(error.contains(u"'O'"_s));
    }

    void nestingDepthIsBounded()
    {
        fctest::Obj inner(20, 0);
        for (int depth = 0; depth < 40; ++depth) {
            inner = fctest::Obj(20, 0).object(2, 6, inner);
        }
        QString error;
        const auto o = parseObjectStream(inner.stream(), error);
        // Trop profond : la propriété fautive devient brute, l'objet racine reste lisible.
        QVERIFY2(o, qPrintable(error));
    }
    void hugeCountsAreRefusedWithoutAllocating()
    {
        // Compteurs annoncés (polygones, liste de chaînes) hors borne : propriété brute, objet lisible.
        fctest::Bytes polygons;
        polygons.u16(127).u16(2).i32(0x7FFFFFFF);
        fctest::Bytes negative;
        negative.u16(127).u16(2).i32(-1);
        fctest::Bytes strings;
        strings.u16(11).u16(1).i32(5'000'001);
        const QByteArray data = fctest::Obj(9, 1)
                                    .raw(2, polygons.data())
                                    .raw(3, negative.data())
                                    .raw(4, strings.data())
                                    .integer(5, 7)
                                    .stream();
        QString error;
        const auto o = parseObjectStream(data, error);
        QVERIFY2(o, qPrintable(error));
        QVERIFY(std::holds_alternative<RawValue>(o->find(2)->data));
        QVERIFY(std::holds_alternative<RawValue>(o->find(3)->data));
        QVERIFY(std::holds_alternative<RawValue>(o->find(4)->data));
        QCOMPARE(asNumber(o->find(5)).value_or(0), 7.0);
    }

    void tooManyPropertiesAreRefused()
    {
        fctest::Bytes b;
        b.raw(QByteArray(" Bamboo ", 8)).u16(1).u16(7).tag('O').u16(7).u32(1).tag('M').u32(10001);
        QString error;
        QVERIFY(!parseObjectStream(b.data(), error));
        QVERIFY(!error.isEmpty());
    }

    void recordLargerThanTheStreamIsRefused()
    {
        fctest::Bytes b;
        b.raw(QByteArray(" Bamboo ", 8))
            .u16(1)
            .u16(7)
            .tag('O')
            .u16(7)
            .u32(1)
            .tag('M')
            .u32(1)
            .u32(0xFFFFFFF0u)
            .raw("abc");
        QString error;
        QVERIFY(!parseObjectStream(b.data(), error));
        QVERIFY(error.contains(u"enregistrement"_s));
    }

    void everyTruncationOfAValidStreamIsSafe()
    {
        const QByteArray good = fctest::Obj(3, 1)
                                    .string(2, u"abc"_s)
                                    .points(3, {QPointF(1, 2), QPointF(3, 4)})
                                    .object(4, 6, fctest::Obj(20, 0).integer(2, 1))
                                    .owner(5)
                                    .stream();
        for (qsizetype n = 0; n < good.size(); ++n) {
            QString error;
            const auto o = parseObjectStream(good.left(n), error); // ne doit ni planter ni boucler
            if (!o) {
                QVERIFY(!error.isEmpty());
            }
        }
    }

    void oddStringLengthAndDeepListAreRaw()
    {
        fctest::Bytes odd;
        odd.u16(10).u16(1).u32(3).raw("abc");
        fctest::Bytes deep;
        for (int i = 0; i < 40; ++i) {
            deep.u16(9).i32(1);
        }
        deep.u16(1).u8(1);
        const QByteArray data =
            fctest::Obj(9, 1).raw(2, odd.data()).raw(3, deep.data()).integer(4, 1).stream();
        QString error;
        const auto o = parseObjectStream(data, error);
        QVERIFY2(o, qPrintable(error));
        QVERIFY(std::holds_alternative<RawValue>(o->find(2)->data));
        QVERIFY(std::holds_alternative<RawValue>(o->find(3)->data));
        QCOMPARE(asNumber(o->find(4)).value_or(0), 1.0);
    }

    void nanAndHugeRealsAreKeptAsIs()
    {
        const QByteArray data =
            fctest::Obj(9, 1).real(2, std::numeric_limits<double>::quiet_NaN()).real(3, 1e30).stream();
        QString error;
        const auto o = parseObjectStream(data, error);
        QVERIFY2(o, qPrintable(error));
        QVERIFY(std::isnan(asNumber(o->find(2)).value_or(0)));
        QCOMPARE(asNumber(o->find(3)).value_or(0), 1e30);
    }
};

QTEST_GUILESS_MAIN(TstFormatsObjectStream)
#include "tst_formats_objectstream.moc"
