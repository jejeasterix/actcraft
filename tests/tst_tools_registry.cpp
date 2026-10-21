#include <actcraft/tools/pentool.h>
#include <actcraft/tools/toolcontext.h>
#include <actcraft/tools/toolregistry.h>
#include <actcraft/tools/userstate.h>

#include <QString>
#include <QStringList>
#include <QTest>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

using namespace actcraft;
using namespace actcraft::tools;

namespace {

// Doublures muettes : le registre ne fait que créer les outils, aucun ne travaille ici.
class NullEditor final : public DocumentEditor
{
public:
    const core::Page& page() const override { return m_page; }
    std::size_t pageIndex() const override { return 0; }
    void execute(std::unique_ptr<core::UndoCommand>) override {}
    std::vector<core::PageObject> lift(const std::vector<core::ObjectId>&) override { return {}; }
    void drop(std::unique_ptr<core::UndoCommand>) override {}
    bool isLifting() const override { return false; }

private:
    core::Page m_page;
};

class NullInk final : public InkSurface
{
public:
    void beginStroke(core::Argb, double) override {}
    void addPoint(const core::StrokePoint&, std::uint64_t) override {}
    void clear() override {}
};

class NullStrokes final : public DocumentSink
{
public:
    void commitStroke(core::Stroke) override {}
};

class NullLift final : public LiftSurface
{
public:
    void show(std::vector<core::PageObject>) override {}
    void setTransform(const core::Transform&) override {}
    void clear() override {}
};

class NullSelection final : public SelectionSurface
{
public:
    void show(const SelectionView&) override {}
};

class NullView final : public ViewInfo
{
public:
    double zoom() const override { return 1.0; }
    bool overTrash(core::PointF) const override { return false; }
    void setTrashHover(bool) override {}
    QStringList objectBarCommands() const override { return {}; }
};

class NullCommands final : public CommandSink
{
public:
    bool trigger(const QString&, const QVariant&) override { return false; }
};

struct Fixture
{
    NullEditor document;
    NullInk ink;
    NullStrokes strokes;
    NullLift lift;
    NullSelection selection;
    NullView view;
    NullCommands commands;
    UserState user;
    ToolContext context{document, ink, strokes, lift, selection, view, commands, user};
};

ToolRegistry::Factory penFactory()
{
    return [](ToolContext& context) { return std::make_unique<PenTool>(context.ink, context.strokes); };
}

} // namespace

class TstToolsRegistry : public QObject
{
    Q_OBJECT

private slots:
    void builtinsRegistered()
    {
        ToolRegistry registry;
        registerBuiltinTools(registry);
        // Lot 4 (L4-T1) : les outils « shape », « text » et « text-vertical » s'ajoutent en fin.
        QCOMPARE(registry.ids(),
                 (QStringList{"select", "pen", "highlighter", "eraser", "shape", "text", "text-vertical"}));
        QVERIFY(!registry.add(QStringLiteral("pen"), {}));
        QVERIFY(!registry.add(QString(), {}));
        // Les deux refus ci-dessus tiendraient aussi par la seule fabrique vide : on les reprend avec une
        // fabrique réelle, pour que ce soit bien l'identifiant pris ou vide qui les refuse.
        QVERIFY(!registry.add(QStringLiteral("pen"), penFactory()));
        QVERIFY(!registry.add(QString(), penFactory()));
        QVERIFY(registry.contains(QStringLiteral("eraser")));
        QVERIFY(!registry.contains(QStringLiteral("compass")));
        QVERIFY(registry.add(QStringLiteral("compass"), penFactory()));
        QCOMPARE(registry.ids().size(), qsizetype{8});              // 7 outils intégrés (lot 4) + « compass »
        QCOMPARE(registry.ids().last(), QStringLiteral("compass")); // ordre d'ajout
    }

    // Le patron écarté (L3-T6) : une classe de marqueur copiée du stylo, deux chemins d'encre à tenir en
    // parité. Le marqueur du registre est le stylo lui-même, de genre Highlighter ; sous le patron écarté,
    // dynamic_cast<PenTool*> rendrait nullptr. Chaque outil d'encre naît avec les réglages de son
    // utilisateur (F-136).
    void markerIsThePenWithTheUserSettings()
    {
        Fixture f;
        f.user.setSettings(QStringLiteral("pen"), {0xFFE53935u, 8, 0});
        ToolRegistry registry;
        registerBuiltinTools(registry);

        const std::unique_ptr<Tool> markerTool = registry.create(QStringLiteral("highlighter"), f.context);
        QVERIFY(markerTool);
        const auto* marker = dynamic_cast<const PenTool*>(markerTool.get());
        QVERIFY(marker);
        QCOMPARE(marker->id(), QStringLiteral("highlighter"));
        QCOMPARE(marker->kind(), core::StrokeKind::Highlighter);
        QCOMPARE(marker->color(), core::Argb{0xFFFFFF00u});
        QCOMPARE(marker->baseWidth(), 20.0);
        QCOMPARE(marker->translucency(), std::uint8_t{127});

        const std::unique_ptr<Tool> penTool = registry.create(QStringLiteral("pen"), f.context);
        const auto* pen = dynamic_cast<const PenTool*>(penTool.get());
        QVERIFY(pen);
        QCOMPARE(pen->id(), QStringLiteral("pen"));
        QCOMPARE(pen->kind(), core::StrokeKind::Pen);
        QCOMPARE(pen->color(), core::Argb{0xFFE53935u});
        QCOMPARE(pen->baseWidth(), 8.0);
        QCOMPARE(pen->translucency(), std::uint8_t{0});

        // Gomme et sélection : créées par leurs constructeurs (talons de L3-T7 et L3-T8).
        QVERIFY(registry.create(QStringLiteral("select"), f.context));
        QVERIFY(registry.create(QStringLiteral("eraser"), f.context));
        QVERIFY(!registry.create(QStringLiteral("compass"), f.context));
    }

    // D3, D4, D5 (docs/verifications/2026-10-18-lot3-principes.md) : gomme 50 (et non 20) ; marqueur à
    // l'opacité 128 sur 255 de l'original, soit translucidité 127 dans notre convention (0 opaque … 255
    // invisible).
    void defaultsPerTool()
    {
        QCOMPARE(defaultSettings(QStringLiteral("pen")).color, core::Argb{0xFF000000u});
        QCOMPARE(defaultSettings(QStringLiteral("pen")).width, 4.0);
        QCOMPARE(defaultSettings(QStringLiteral("pen")).translucency, std::uint8_t{0});
        QCOMPARE(defaultSettings(QStringLiteral("highlighter")).color, core::Argb{0xFFFFFF00u});
        QCOMPARE(defaultSettings(QStringLiteral("highlighter")).width, 20.0);
        QCOMPARE(defaultSettings(QStringLiteral("highlighter")).translucency, std::uint8_t{127});
        QCOMPARE(defaultSettings(QStringLiteral("eraser")).width, 50.0);
        QCOMPARE(defaultSettings(QStringLiteral("eraser")).translucency, std::uint8_t{0});
        // Autre outil : ceux du stylo.
        QCOMPARE(defaultSettings(QStringLiteral("select")).color, core::Argb{0xFF000000u});
        QCOMPARE(defaultSettings(QStringLiteral("select")).width, 4.0);
    }

    // Une mémoire par outil, pas un réglage courant partagé : sous ce patron, le marqueur hériterait du
    // rouge du stylo.
    void userKeepsSettingsPerTool()
    {
        UserState user;
        QCOMPARE(user.currentTool(), QStringLiteral("pen")); // P37 : outil de départ
        user.setSettings(QStringLiteral("pen"), {0xFFE53935u, 8, 0});
        user.setCurrentTool(QStringLiteral("highlighter"));
        QCOMPARE(user.currentTool(), QStringLiteral("highlighter"));
        QCOMPARE(user.settings(QStringLiteral("highlighter")).color, core::Argb{0xFFFFFF00u});
        QCOMPARE(user.settings(QStringLiteral("highlighter")).translucency, std::uint8_t{127});
        user.setSettings(QStringLiteral("highlighter"), {0xFF43A047u, 30, 60});
        user.setCurrentTool(QStringLiteral("pen"));
        QCOMPARE(user.settings(QStringLiteral("pen")).color, core::Argb{0xFFE53935u});
        QCOMPARE(user.settings(QStringLiteral("pen")).width, 8.0);
        QCOMPARE(user.settings(QStringLiteral("highlighter")).color, core::Argb{0xFF43A047u});
        QCOMPARE(user.settings(QStringLiteral("highlighter")).width, 30.0);
        QCOMPARE(user.settings(QStringLiteral("highlighter")).translucency, std::uint8_t{60});
        QCOMPARE(user.settings(QStringLiteral("eraser")).width, 50.0); // jamais réglé : défaut
    }

    void selectionBelongsToTheUser()
    {
        UserState user(1);
        QCOMPARE(user.id(), input::UserId{1});
        QVERIFY(user.selection().empty());
        user.setSelection({4, 5});
        QCOMPARE(user.selection().size(), std::size_t{2});
        QCOMPARE(user.selection()[1], core::ObjectId{5});
    }

    void openMathToolsKept()
    {
        actcraft::tools::UserState user;
        QCOMPARE(user.id(), actcraft::input::kFirstUser);
        QVERIFY(user.openMathTools().empty());
        user.setOpenMathTools({QStringLiteral("ruler"), QStringLiteral("compass")});
        QCOMPARE(user.openMathTools().size(), std::size_t{2});
        QCOMPARE(user.openMathTools()[1], QStringLiteral("compass"));
    }
};

QTEST_GUILESS_MAIN(TstToolsRegistry)
#include "tst_tools_registry.moc"
