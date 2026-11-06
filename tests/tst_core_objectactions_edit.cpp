#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/objectactions.h>
#include <actcraft/core/objectflags.h>
#include <actcraft/core/undo.h>

#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QtTest>

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

using namespace actcraft::core;

// Actions d'objet d'édition (L8-T8). Même montage que tst_core_objectactions (L8-T7) : une forme 1 ; ici
// aussi un texte 2 contenant « 5 ».
class TstCoreObjectactionsEdit : public QObject
{
    Q_OBJECT
    static ActionContext on(ObjectId source) { return ActionContext{0, source, 0, 1024, 768}; }

    static PageObject shape(ObjectId id)
    {
        Shape s;
        s.bounds = {0, 0, 100, 50};
        s.outlineWidth = 0;
        PageObject o{s};
        o.common.id = id;
        o.common.transform = translation(200, 100);
        return o;
    }
    static PageObject text(ObjectId id, const char* html)
    {
        Text t;
        t.bounds = {0, 0, 120, 30};
        t.html = html;
        PageObject o{t};
        o.common.id = id;
        o.common.transform = translation(400, 300);
        return o;
    }
    static PageObject topStroke(ObjectId id)
    {
        Stroke s;
        s.points = {{10, 10}, {60, 40}};
        PageObject o{s};
        o.common.id = id;
        o.common.layer = Layer::Top;
        return o;
    }
    static Document page(const char* html = "<p>5</p>")
    {
        Document d(1024, 768);
        d.page(0).objects.push_back(shape(d.allocateId()));      // 1 ; nextId suit, comme un document réel
        d.page(0).objects.push_back(text(d.allocateId(), html)); // 2
        return d;
    }
    static const PageObject* find(const Document& d, ObjectId id)
    {
        for (const PageObject& o : d.page(0).objects) {
            if (o.common.id == id) {
                return &o;
            }
        }
        return nullptr;
    }
    static std::string htmlOf(const PageObject& o)
    {
        const Text* t = std::get_if<Text>(&body(o));
        return t ? t->html : std::string{};
    }
    // Texte lu d'un HTML, sans dépendre du moteur : en-tête et styles retirés, balises retirées, entités
    // décodées, blancs des bords retirés.
    static QString plain(const std::string& html)
    {
        QString s = QString::fromStdString(html);
        s.remove(QRegularExpression(QStringLiteral("<head[^>]*>.*</head>"),
                                    QRegularExpression::DotMatchesEverythingOption |
                                        QRegularExpression::InvertedGreedinessOption));
        s.remove(QRegularExpression(QStringLiteral("<style[^>]*>.*</style>"),
                                    QRegularExpression::DotMatchesEverythingOption |
                                        QRegularExpression::InvertedGreedinessOption));
        s.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
        s.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
        s.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
        s.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
        s.replace(QStringLiteral("&#39;"), QStringLiteral("'"));
        s.replace(QStringLiteral("&nbsp;"), QStringLiteral(" "));
        s.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
        return s.trimmed();
    }
    static std::unique_ptr<UndoCommand> command(const Document& d, const char* id, const char* params,
                                                ObjectId target)
    {
        return objectActionCommand(d, id, params, on(target));
    }
    static void apply(Document& d, UndoCommand& c)
    {
        Changes changes;
        c.redo(d, changes);
    }
    static void revert(Document& d, UndoCommand& c)
    {
        Changes changes;
        c.undo(d, changes);
    }

private slots:
    void toggleHiddenTwiceRestores()
    {
        Document d = page();
        auto first = command(d, "object.toggle-hidden", "{}", 1);
        QVERIFY(first);
        apply(d, *first);
        QCOMPARE(find(d, 1)->common.visible, false);
        // Deuxième exécution, lue sur le document modifié : une seconde étape, qui rend l'objet visible.
        auto second = command(d, "object.toggle-hidden", "{}", 1);
        QVERIFY(second);
        apply(d, *second);
        QCOMPARE(find(d, 1)->common.visible, true);
        revert(d, *second);
        QCOMPARE(find(d, 1)->common.visible, false);
        revert(d, *first);
        QCOMPARE(fingerprint(d), fingerprint(page()));
    }

    void toggleHiddenDelegatesToObjectFlags()
    {
        // A16 : masquer et verrouiller passent par SetObjectFlagCommand (signal objectFlagsChanged), jamais
        // par un remplacement d'objet qui basculerait le champ.
        const Document d = page();
        auto hide = command(d, "object.toggle-hidden", "{}", 1);
        QVERIFY(hide);
        QVERIFY(dynamic_cast<const SetObjectFlagCommand*>(hide.get()));
        auto lock = command(d, "object.toggle-locked", "{}", 1);
        QVERIFY(lock);
        QVERIFY(dynamic_cast<const SetObjectFlagCommand*>(lock.get()));
        Document locked = page();
        apply(locked, *lock);
        QCOMPARE(find(locked, 1)->common.locked, true);
        // H8-11 : l'action agit aussi sur un objet verrouillé ; elle le déverrouille.
        auto unlock = command(locked, "object.toggle-locked", "{}", 1);
        QVERIFY(unlock);
        apply(locked, *unlock);
        QCOMPARE(find(locked, 1)->common.locked, false);
    }

    void textValueArithmetic()
    {
        Document d = page();
        auto c = command(d, "object.text-value", R"({"value":-2})", 2);
        QVERIFY(c);
        apply(d, *c);
        QCOMPARE(plain(htmlOf(*find(d, 2))), QStringLiteral("3"));
        Document decimal = page("<p>2.5</p>");
        auto half = command(decimal, "object.text-value", R"({"value":1})", 2);
        QVERIFY(half);
        apply(decimal, *half);
        QCOMPARE(plain(htmlOf(*find(decimal, 2))), QStringLiteral("3.5"));
        revert(d, *c);
        QCOMPARE(fingerprint(d), fingerprint(page()));
    }

    void textValueNonNumericUnchanged()
    {
        // Le texte entier et la valeur doivent être des nombres.
        QVERIFY(!command(page("<p>cinq</p>"), "object.text-value", R"({"value":-2})", 2));
        QVERIFY(!command(page("<p>5 pommes</p>"), "object.text-value", R"({"value":1})", 2));
        QVERIFY(!command(page(), "object.text-value", R"({"value":"deux"})", 2));
        QVERIFY(!command(page(), "object.text-value", "{}", 2));
        // Une forme n'a pas de texte.
        QVERIFY(!command(page(), "object.text-value", R"({"value":1})", 1));
    }

    void addTextIsPlainEscaped()
    {
        // D8-5 : aucun HTML reçu n'est écrit dans un texte ; « ajouter », « remplacer » et « nouveau »
        // écrivent du texte brut, échappé.
        const char* params = R"({"text":"<b>x</b>"})";
        for (const char* id : {"object.add-text", "object.set-text", "object.new-text"}) {
            Document d = page();
            auto c = command(d, id, params, 2);
            QVERIFY2(c, id);
            apply(d, *c);
            bool written = false;
            for (const PageObject& o : d.page(0).objects) {
                const std::string html = htmlOf(o);
                QVERIFY2(html.find("<b>") == std::string::npos, id);
                if (html.find("&lt;b&gt;") != std::string::npos) {
                    written = true;
                    QVERIFY2(plain(html).contains(QStringLiteral("<b>x</b>")), id);
                }
            }
            QVERIFY2(written, id);
        }
        // « Ajouter » garde le texte de départ.
        Document d = page();
        auto add = command(d, "object.add-text", params, 2);
        QVERIFY(add);
        apply(d, *add);
        QVERIFY(plain(htmlOf(*find(d, 2))).startsWith(QStringLiteral("5")));
        // « Remplacer » ne le garde pas.
        Document r = page();
        auto set = command(r, "object.set-text", params, 2);
        QVERIFY(set);
        apply(r, *set);
        QCOMPARE(plain(htmlOf(*find(r, 2))), QStringLiteral("<b>x</b>"));
    }

    void extractTextOneObjectPerWord()
    {
        Document d = page("<p>un deux trois</p>");
        const Document before = d;
        auto c = command(d, "object.extract-text", "{}", 2);
        QVERIFY(c);
        apply(d, *c);
        // Une seule commande ajoute les trois objets ; le texte de départ reste.
        QCOMPARE(d.page(0).objects.size(), std::size_t{5});
        QVERIFY(find(d, 2));
        QCOMPARE(plain(htmlOf(*find(d, 2))), QStringLiteral("un deux trois"));
        QStringList words;
        for (std::size_t i = 2; i < d.page(0).objects.size(); ++i) {
            const PageObject& o = d.page(0).objects[i];
            QVERIFY(std::holds_alternative<Text>(body(o)));
            QVERIFY(o.common.id != 1 && o.common.id != 2);
            words << plain(htmlOf(o));
        }
        QCOMPARE(words, (QStringList{QStringLiteral("un"), QStringLiteral("deux"), QStringLiteral("trois")}));
        revert(d, *c);
        QCOMPARE(fingerprint(d), fingerprint(before));
    }

    void translucencyClamped()
    {
        Document d = page();
        auto set = command(d, "object.translucency", R"({"value":300})", 1);
        QVERIFY(set);
        apply(d, *set);
        QCOMPARE(int(find(d, 1)->common.translucency), 255);
        // Pas de 32 ; « moins translucide » à 0 n'a rien à faire.
        QVERIFY(!command(page(), "object.less-translucent", "{}", 1));
        Document more = page();
        auto step = command(more, "object.more-translucent", "{}", 1);
        QVERIFY(step);
        apply(more, *step);
        QCOMPARE(int(find(more, 1)->common.translucency), 32);
        auto less = command(more, "object.less-translucent", "{}", 1);
        QVERIFY(less);
        apply(more, *less);
        QCOMPARE(int(find(more, 1)->common.translucency), 0);
        Document high = page();
        high.page(0).objects[0].common.translucency = 240;
        auto top = command(high, "object.more-translucent", "{}", 1);
        QVERIFY(top);
        apply(high, *top);
        QCOMPARE(int(find(high, 1)->common.translucency), 255);
    }

    void clearAnnotationsOnlyTopStrokes()
    {
        Document d = page();
        d.page(0).objects.push_back(topStroke(d.allocateId())); // 3
        const Document before = d;
        auto c = command(d, "object.clear-annotations", "{}", 1);
        QVERIFY(c);
        apply(d, *c);
        QVERIFY(!find(d, 3));
        QVERIFY(find(d, 1));
        QVERIFY(find(d, 2));
        revert(d, *c);
        QCOMPARE(fingerprint(d), fingerprint(before));
    }

    void everyEditIsOneStep()
    {
        // Identifiants d'édition du catalogue (actions/data/object-actions.json, L8-T6), chacun sur une cible
        // où il a un effet. Montage : forme 1 (translucidité 64), texte 2 « 5 », trait 3 de la couche du
        // dessus.
        struct Case
        {
            const char* id;
            const char* params;
            ObjectId target;
        };
        const Case cases[] = {
            {"object.forward", "{}", 1},
            {"object.backward", "{}", 2},
            {"object.front", "{}", 1},
            {"object.back", "{}", 2},
            {"object.layer-top", "{}", 1},
            {"object.layer-middle", "{}", 3},
            {"object.layer-bottom", "{}", 1},
            {"object.toggle-hidden", "{}", 1},
            {"object.toggle-locked", "{}", 1},
            {"object.translucency", R"({"value":128})", 1},
            {"object.more-translucent", "{}", 1},
            {"object.less-translucent", "{}", 1},
            {"object.add-text", R"({"text":"x"})", 2},
            {"object.set-text", R"({"text":"x"})", 2},
            {"object.text-value", R"({"value":1})", 2},
            {"object.new-text", R"({"text":"x"})", 2},
            {"object.extract-text", "{}", 2},
            {"object.clear-annotations", "{}", 1},
            {"object.clear-background", "{}", 1},
            {"object.clear-grid", "{}", 1},
            {"object.clear-objects", "{}", 1},
            {"object.delete", "{}", 1},
            {"object.duplicate", "{}", 1},
        };
        Document start = page();
        start.page(0).objects[0].common.translucency = 64;
        start.page(0).objects.push_back(topStroke(start.allocateId())); // 3
        for (const Case& k : cases) {
            QVERIFY2(isEditAction(k.id), k.id);
            QVERIFY2(!isGeometryAction(k.id), k.id);
            Document d = start;
            auto c = objectActionCommand(d, k.id, k.params, on(k.target));
            QVERIFY2(c, k.id); // une action permise rend sa commande : le test ne passe pas à vide
            apply(d, *c);
            revert(d, *c);
            QVERIFY2(fingerprint(d) == fingerprint(start), k.id);
        }
        // Copier, couper et coller passent par le presse-papiers hors de core (A18) : l'exécuteur (L8-T9)
        // déclenche edit.copy, edit.cut, edit.paste ; le moteur ne les connaît pas.
        for (const char* id : {"object.copy", "object.cut", "object.paste"}) {
            QVERIFY2(!isEditAction(id), id);
            QVERIFY2(!objectActionCommand(start, id, "{}", on(1)), id);
        }
    }
};

QTEST_MAIN(TstCoreObjectactionsEdit)
#include "tst_core_objectactions_edit.moc"
