#include "support/flipchartwriter.h"

#include <actcraft/core/document.h>
#include <actcraft/core/interactivity.h>
#include <actcraft/formats/flipchart.h>

#include <QFile>
#include <QRegularExpression>
#include <QtTest>

#include <utility>
#include <vector>

// Lecteur .flipchart de l'interactivité (lot 8, L8-T5). Chaque cas est écrit par l'écrivain synthétique
// (tests/support/flipchartwriter.h), jamais tiré d'un paperboard réel. Le type d'action 27 n'est écrit nulle
// part : le lecteur ne le reconnaît pas (rapport L8-T0, D1).

using namespace actcraft;
using namespace Qt::StringLiterals;
using fctest::Entry;
using fctest::Node;
using fctest::Obj;

namespace {

constexpr QRgb kBlack = 0xFF000000u;

// Une page (nœud 1003) : arbre et objets de son conteneur ; `resources` à la racine.
formats::flipchart::FlipchartReadResult readPage(std::vector<Node> tree, std::vector<Entry> entries,
                                                 std::vector<Entry> resources = {})
{
    fctest::synth::Sheet s(1003);
    s.tree = std::move(tree);
    s.entries = std::move(entries);
    return formats::flipchart::readFlipchart(fctest::synth::paperboard(u"Interactivite"_s, {s}, resources));
}

Obj shape(quint32 storage, const char* name, double z)
{
    return fctest::synth::rectangle(storage, name, 1, z, fctest::synth::place(100 * z, 50),
                                    fctest::synth::ink(1, 1, 32, 128, kBlack));
}

QString str(const std::string& s)
{
    return QString::fromStdString(s);
}

} // namespace

class TstFormatsFlipchartInteractivity : public QObject
{
    Q_OBJECT
private slots:
    // Commande 184 d'une action sur objet : notre « afficher ou masquer », cible résolue vers l'objet visé.
    void toggleHiddenMapsToOurId()
    {
        using fctest::synth::io;
        const auto r = readPage({Node(1010, 7, 1011), Node(1012, 28, 1013), Node(1014, 7, 1015)},
                                {io(1011, shape(1011, "Declencheur", 1)),
                                 io(1013, fctest::synth::objectAction(1013, 184, 1014, 1015, 7)),
                                 io(1015, shape(1015, "Etiquette", 2))});
        QVERIFY2(r.ok, qPrintable(r.error));
        QVERIFY2(r.warnings.isEmpty(), qPrintable(r.warnings.join(u'\n')));
        const auto& objects = r.document.page(0).objects;
        QCOMPARE(objects.size(), std::size_t{2});
        QCOMPARE(str(objects[0].common.name), u"Declencheur"_s);
        QCOMPARE(str(objects[1].common.name), u"Etiquette"_s);
        QCOMPARE(objects[0].common.actions.size(), std::size_t{1});
        QVERIFY(objects[1].common.actions.empty());

        const core::Action& a = objects[0].common.actions[0];
        QCOMPARE(a.kind, core::ActionKind::OnObject);
        QCOMPARE(a.sourceType, std::uint16_t{28});
        QCOMPARE(a.command, 184);
        QCOMPARE(str(a.catalogId), u"object.toggle-hidden"_s);
        QVERIFY(a.parameters.empty());
        QCOMPARE(a.target, objects[1].common.id);
        QVERIFY(core::isSupported(a));
    }

    // Le genre de l'action ne choisit pas l'identifiant : seule la paire (genre, commande) de la table le
    // fait. Une commande absente de la table garde son numéro et sa cible, sans identifiant (« action non
    // prise en charge »), même dans un genre dont une autre commande est connue (ouverture : 270), et une
    // commande connue dans un genre ne vaut pas dans un autre (184 en ouverture, 270 sur objet). Aucun
    // avertissement.
    void unknownCommandKeptWithoutId()
    {
        using fctest::synth::io;
        const auto r =
            readPage({Node(1010, 7, 1011), Node(1012, 28, 1013), Node(1014, 26, 1015), Node(1016, 26, 1017),
                      Node(1020, 26, 1021), Node(1022, 28, 1023), Node(1018, 7, 1019)},
                     {io(1011, shape(1011, "Declencheur", 1)),
                      io(1013, fctest::synth::objectAction(1013, 9999, 1018, 1019, 7)),
                      io(1015, fctest::synth::openFileAction(1015, 9999)),
                      io(1017, fctest::synth::openFileAction(1017, 270)),
                      io(1021, fctest::synth::openFileAction(1021, 184)),
                      io(1023, fctest::synth::objectAction(1023, 270, 1018, 1019, 7)),
                      io(1019, shape(1019, "Cible", 2))});
        QVERIFY2(r.ok, qPrintable(r.error));
        QVERIFY2(r.warnings.isEmpty(), qPrintable(r.warnings.join(u'\n')));
        const auto& objects = r.document.page(0).objects;
        QCOMPARE(objects.size(), std::size_t{2});
        const std::vector<core::Action>& actions = objects[0].common.actions;
        QCOMPARE(actions.size(), std::size_t{5});

        const core::Action& onObject = actions[0];
        QCOMPARE(onObject.kind, core::ActionKind::OnObject);
        QCOMPARE(onObject.command, 9999);
        QVERIFY(onObject.catalogId.empty());
        QVERIFY(onObject.parameters.empty());
        QCOMPARE(onObject.target, objects[1].common.id);
        QVERIFY(!core::isSupported(onObject));

        const core::Action& unknownOpen = actions[1];
        QCOMPARE(unknownOpen.kind, core::ActionKind::OpenExternal);
        QCOMPARE(unknownOpen.sourceType, std::uint16_t{26});
        QCOMPARE(unknownOpen.command, 9999);
        QVERIFY(unknownOpen.catalogId.empty());
        QVERIFY(!core::isSupported(unknownOpen));

        const core::Action& open = actions[2];
        QCOMPARE(open.kind, core::ActionKind::OpenExternal);
        QCOMPARE(open.command, 270);
        QCOMPARE(str(open.catalogId), u"document.open-file"_s);
        QVERIFY(core::isSupported(open));

        const core::Action& openWithObjectCommand = actions[3];
        QCOMPARE(openWithObjectCommand.kind, core::ActionKind::OpenExternal);
        QCOMPARE(openWithObjectCommand.command, 184);
        QVERIFY(openWithObjectCommand.catalogId.empty());

        const core::Action& objectWithOpenCommand = actions[4];
        QCOMPARE(objectWithOpenCommand.kind, core::ActionKind::OnObject);
        QCOMPARE(objectWithOpenCommand.command, 270);
        QVERIFY(objectWithOpenCommand.catalogId.empty());
        QCOMPARE(objectWithOpenCommand.target, objects[1].common.id);
    }

    // Les booléens du lot 2 restent la source du « bloqué » : le lecteur les lit, puis tient les modes
    // d'accord (normalize). Un objet bloqué en déplacement et en rotation sort en modes Fixed, l'empreinte ne
    // change pas (isDefault) ; un objet libre garde les modes par défaut.
    void limitersNormalized()
    {
        using fctest::synth::io;
        const auto r =
            readPage({Node(1010, 7, 1011), Node(1012, 7, 1013)},
                     {io(1011, fctest::synth::limited(shape(1011, "Bloque", 1), false, true, false)),
                      io(1013, shape(1013, "Libre", 2))});
        QVERIFY2(r.ok, qPrintable(r.error));
        const auto& objects = r.document.page(0).objects;
        QCOMPARE(objects.size(), std::size_t{2});

        const core::Restrictions& held = objects[0].common.restrictions;
        QVERIFY(!held.movable);
        QVERIFY(held.resizable);
        QVERIFY(!held.rotatable);
        QCOMPARE(held.travel, core::Travel::Fixed);
        QCOMPARE(held.turning, core::Turning::Fixed);
        QVERIFY(core::isDefault(held));

        const core::Restrictions& free = objects[1].common.restrictions;
        QVERIFY(free.movable && free.resizable && free.rotatable);
        QCOMPARE(free.travel, core::Travel::Free);
        QCOMPARE(free.turning, core::Turning::Free);
        QVERIFY(core::isDefault(free));
    }

    // Encre de type 2 : encre révélatrice ; sa couche est lue telle quelle (1 : intermédiaire), le lecteur
    // n'en invente pas.
    void revealInkRead()
    {
        using fctest::synth::io;
        using fctest::synth::ir;
        Obj stroke = fctest::synth::item(3, 1011, "Encre", 1, 1, fctest::synth::place(10, 10));
        stroke.object(10, 6, Obj(20, 0).integer(2, 2).real(3, 6).integer(5, 32).argb(7, kBlack))
            .integer(39, 2201);
        const auto r = readPage({Node(1010, 3, 1011)}, {io(1011, stroke)},
                                {ir(2201, Obj(4, 2201).points(4, {QPointF(0, 0), QPointF(20, 10)}))});
        QVERIFY2(r.ok, qPrintable(r.error));
        const auto& objects = r.document.page(0).objects;
        QCOMPARE(objects.size(), std::size_t{1});
        QCOMPARE(core::kindOf(objects[0]), core::ObjectKind::Stroke);
        const auto* s = std::get_if<core::Stroke>(&objects[0]);
        QVERIFY(s);
        QCOMPARE(s->kind, core::StrokeKind::Revealer);
        QCOMPARE(objects[0].common.layer, core::Layer::Middle);
    }

    // L8-C6 (plan L8-T5 étape 2) : la combinaison qui découpe vraiment. Le rendu ne découpe que si l'encre
    // révélatrice est sur la couche du dessus (render/src/pagepainter.cpp:885-888) ; le plan 2 du fichier
    // donne Layer::Top (formats/src/flipchart.cpp:999). Le lecteur lit le plan du fichier, il ne force rien :
    // le test précédent (plan 1) garde le cas de la couche intermédiaire.
    void revealInkUpperLayerRead()
    {
        using fctest::synth::io;
        using fctest::synth::ir;
        Obj stroke = fctest::synth::item(3, 1011, "Encre", 2, 1, fctest::synth::place(10, 10));
        stroke.object(10, 6, Obj(20, 0).integer(2, 2).real(3, 6).integer(5, 32).argb(7, kBlack))
            .integer(39, 2201);
        const auto r = readPage({Node(1010, 3, 1011)}, {io(1011, stroke)},
                                {ir(2201, Obj(4, 2201).points(4, {QPointF(0, 0), QPointF(20, 10)}))});
        QVERIFY2(r.ok, qPrintable(r.error));
        const auto& objects = r.document.page(0).objects;
        QCOMPARE(objects.size(), std::size_t{1});
        const auto* s = std::get_if<core::Stroke>(&objects[0]);
        QVERIFY(s);
        QCOMPARE(s->kind, core::StrokeKind::Revealer);
        QCOMPARE(objects[0].common.layer, core::Layer::Top);
    }

    // Aucun identifiant au préfixe des noms de l'original dans nos numéros du format.
    void noNameOfTheOriginal()
    {
        QFile file(QStringLiteral(ACTCRAFT_SOURCE_DIR) + u"/formats/src/flipchartids.h"_s);
        QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text), qPrintable(file.fileName()));
        const QRegularExpression original(u"\\bk?As[A-Z]"_s);
        QStringList offending;
        int lineNumber = 0;
        while (!file.atEnd()) {
            const QString line = QString::fromUtf8(file.readLine());
            ++lineNumber;
            if (original.match(line).hasMatch()) {
                offending << u"%1 : %2"_s.arg(lineNumber).arg(line.trimmed());
            }
        }
        QVERIFY(lineNumber > 0);
        QVERIFY2(offending.isEmpty(), qPrintable(offending.join(u'\n')));
    }
};

QTEST_MAIN(TstFormatsFlipchartInteractivity)
#include "tst_formats_flipchart_interactivity.moc"
