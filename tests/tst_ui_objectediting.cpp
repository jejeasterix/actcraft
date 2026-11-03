#include <actcraft/core/connectors.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/formats/clipboard.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/objectediting.h>
#include <actcraft/ui/toolhost.h>

#include <QClipboard>
#include <QGuiApplication>
#include <QImage>
#include <QMimeData>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <variant>
#include <vector>

using namespace actcraft;

namespace {

// Un document d'une page de 1024 x 768, `count` rectangles (de 100 x 50, espacés de 200), le contrôleur
// d'édition branché sur un ToolHost sans fenêtre, les rectangles sélectionnés.
struct Fixture
{
    QTemporaryDir dir;
    ui::DocumentController document;
    ui::ToolHost tools;
    ui::ObjectEditing editing;
    std::string kinds; // un caractère par changement : i, r, v
    ui::DocumentController::ChangeOrigin lastOrigin = ui::DocumentController::ChangeOrigin::Stroke;

    explicit Fixture(int count = 1, bool selected = true)
    {
        document.newDocument(1024, 768);
        tools.setSettingsFile(dir.filePath(QStringLiteral("outils.ini")));
        tools.setDocumentController(&document);
        editing.setDocument(&document);
        editing.setTools(&tools);
        for (int i = 0; i < count; ++i) {
            core::Shape shape;
            shape.bounds = {100.0 + 200.0 * i, 100, 100, 50};
            core::PageObject object = shape;
            object.common.layer = core::Layer::Middle;
            document.execute(std::make_unique<core::AddObjectCommand>(0, std::move(object)));
        }
        document.addChangeObserver([this](const core::Changes& changes, auto origin) {
            lastOrigin = origin;
            for (const core::Change& change : changes) {
                switch (change.kind) {
                case core::Change::Kind::ObjectInserted:
                    kinds += 'i';
                    break;
                case core::Change::Kind::ObjectRemoved:
                    kinds += 'r';
                    break;
                case core::Change::Kind::ObjectChanged:
                    kinds += 'v';
                    break;
                default:
                    break;
                }
            }
        });
        if (selected)
            tools.user().setSelection(ids());
    }

    const core::Page& page() const { return document.document().page(0); }
    std::vector<core::ObjectId> ids() const
    {
        std::vector<core::ObjectId> out;
        for (const core::PageObject& object : page().objects)
            out.push_back(object.common.id);
        return out;
    }
    int objectCount() const { return static_cast<int>(page().objects.size()); }
    const std::vector<core::ObjectId>& selection() { return tools.user().selection(); }
    void clearSelection() { tools.user().setSelection({}); }
    template <typename T> bool lastObjectIs() const
    {
        return !page().objects.empty() && std::holds_alternative<T>(core::body(page().objects.back()));
    }
    core::RectF lastBounds() const { return core::boundsOnPage(page().objects.back()); }
    void clearSpy() { kinds.clear(); }
    bool has(char kind) const { return kinds.find(kind) != std::string::npos; }
    // Rectangle B (le dernier) relié à A par un connecteur (extrémité de A en B).
    core::ObjectId addConnector()
    {
        const auto all = ids();
        core::PageObject connector = core::makeConnector({200, 100}, all.front(), {300, 100}, all.back(), 0);
        document.execute(std::make_unique<core::AddObjectCommand>(0, std::move(connector)));
        return page().objects.back().common.id;
    }
    const core::Connector* connector(core::ObjectId id) const
    {
        for (const core::PageObject& object : page().objects)
            if (object.common.id == id)
                return std::get_if<core::Connector>(&core::body(object));
        return nullptr;
    }
};

} // namespace

class TestObjectEditing : public QObject
{
    Q_OBJECT
private slots:
    void init() { QGuiApplication::clipboard()->clear(); }

    void copyPasteThroughSystemClipboard()
    {
        Fixture f;
        f.editing.copy();
        QVERIFY(QGuiApplication::clipboard()->mimeData()->hasFormat(formats::kObjectsMimeType));
        const core::ObjectId first = f.ids().front();
        f.editing.paste();
        QCOMPARE(f.objectCount(), 2);
        QCOMPARE(f.selection().size(), std::size_t(1));
        QVERIFY(f.selection()[0] != first);
        QCOMPARE(f.tools.currentTool(), QStringLiteral("select"));
        f.document.undo();
        QCOMPARE(f.objectCount(), 1);
    }

    void pasteTextFromAnotherApp()
    {
        Fixture f;
        QGuiApplication::clipboard()->setText(QStringLiteral("bonjour"));
        f.editing.paste();
        QVERIFY(f.lastObjectIs<core::Text>());
    }

    void pasteImageFromAnotherApp()
    {
        Fixture f;
        QImage big(4000, 100, QImage::Format_ARGB32);
        big.fill(Qt::red);
        QGuiApplication::clipboard()->setImage(big);
        f.editing.paste();
        QTRY_VERIFY(f.lastObjectIs<core::ImageObject>()); // décodage hors du fil principal (L5-T15)
        QVERIFY(f.lastBounds().width <= 1024 * 0.8 + 0.5);
    }

    void withoutSelectionActsOnPage()
    {
        Fixture f;
        f.clearSelection();
        f.editing.duplicate(); // D3 : duplique la page
        QCOMPARE(f.document.pageCount(), 2);
    }

    void groupThenUndoClearsSelection()
    {
        Fixture f(2); // deux rectangles sélectionnés
        f.editing.toggleGroup();
        QVERIFY(f.editing.selectionIsGroup());
        f.document.undo();
        QVERIFY(f.selection().empty());
    }

    void alignGoesThroughTransformEachCommand()
    {
        Fixture f(2);
        f.clearSpy();
        f.editing.align(QStringLiteral("left"));
        QCOMPARE(f.lastOrigin, ui::DocumentController::ChangeOrigin::Command);
        QVERIFY(f.has('v')); // ObjectChanged : la transformation passe par la pile, pas par la vue
        QCOMPARE(f.lastBounds().x, core::boundsOnPage(f.page().objects.front()).x);
    }

    void cutGoesThroughReplaceCommand()
    {
        Fixture f;
        f.clearSpy();
        f.editing.cut();
        QVERIFY(f.has('r'));
        QCOMPARE(f.objectCount(), 0);
        QVERIFY(QGuiApplication::clipboard()->mimeData()->hasFormat(formats::kObjectsMimeType));
        f.document.undo();
        QCOMPARE(f.objectCount(), 1);
    }

    void pasteHtmlLoadsNothing()
    {
        Fixture f;
        auto* mime = new QMimeData;
        mime->setHtml(QStringLiteral("<p>riche</p><img src=\"file:///etc/passwd\">"));
        mime->setText(QStringLiteral("brut"));
        QGuiApplication::clipboard()->setMimeData(mime);
        const std::size_t resources = f.document.document().resources().size();
        f.editing.paste();
        QVERIFY(f.lastObjectIs<core::Text>());
        const auto* text = std::get_if<core::Text>(&core::body(f.page().objects.back()));
        QVERIFY(text->html.find("brut") != std::string::npos);
        QVERIFY(text->html.find("file:") == std::string::npos);
        QVERIFY(text->html.find("riche") == std::string::npos);
        QCOMPARE(f.document.document().resources().size(), resources);
    }

    void insertedObjectIsSelected()
    {
        Fixture f;
        QImage image(200, 100, QImage::Format_ARGB32);
        image.fill(Qt::blue);
        QGuiApplication::clipboard()->setImage(image);
        f.editing.paste();
        QTRY_VERIFY(f.lastObjectIs<core::ImageObject>());
        QTRY_COMPARE(f.selection().size(), std::size_t(1));
        QCOMPARE(f.selection()[0], f.page().objects.back().common.id);
        QCOMPARE(f.tools.currentTool(), QStringLiteral("select"));
        // Texte : même règle.
        QGuiApplication::clipboard()->setText(QStringLiteral("abc"));
        f.editing.paste();
        QVERIFY(f.lastObjectIs<core::Text>());
        QCOMPARE(f.selection().size(), std::size_t(1));
        QCOMPARE(f.selection()[0], f.page().objects.back().common.id);
    }

    void everyEditGoesThroughConnectorFollower()
    {
        { // aligner : B rejoint A, l'extrémité suit
            Fixture f(2, false);
            const core::ObjectId connector = f.addConnector();
            f.tools.user().setSelection({f.ids()[0], f.ids()[1]});
            f.editing.align(QStringLiteral("left"));
            const core::Connector* c = f.connector(connector);
            QVERIFY(c != nullptr);
            QVERIFY(c->objectB == 0 || c->secondEnd.x != 300.0);
            f.document.undo(); // une seule étape : connecteur remis
            QCOMPARE(f.connector(connector)->secondEnd.x, 300.0);
        }
        { // transformer : B retourné, le bout garde son point d'attache
            Fixture f(2, false);
            const core::ObjectId connector = f.addConnector();
            f.tools.user().setSelection({f.ids()[1]});
            f.editing.transform(QStringLiteral("flip-x"));
            const core::Connector* c = f.connector(connector);
            QVERIFY(c->objectB == 0 || c->secondEnd.y != 100.0);
        }
        { // couper : B retiré, l'extrémité se détache
            Fixture f(2, false);
            const core::ObjectId connector = f.addConnector();
            f.tools.user().setSelection({f.ids()[1]});
            f.editing.cut();
            QCOMPARE(f.connector(connector)->objectB, core::ObjectId(0));
        }
    }

    void duplicateSelectsCopies()
    {
        Fixture f;
        const core::ObjectId first = f.ids().front();
        f.editing.duplicate();
        QCOMPARE(f.objectCount(), 2);
        QCOMPARE(f.selection().size(), std::size_t(1));
        QVERIFY(f.selection()[0] != first);
        QCOMPARE(f.tools.currentTool(), QStringLiteral("select"));
    }

    // D2 : dupliquer ou coller un groupe sélectionne la seule copie du groupe, pas ses membres copiés.
    void duplicateGroupSelectsOnlyTheCopy()
    {
        Fixture f(2);
        const std::vector<core::ObjectId> members = f.ids();
        core::PageObject group = core::Group{members};
        group.common.layer = core::Layer::Middle;
        f.document.execute(std::make_unique<core::AddObjectCommand>(0, std::move(group)));
        const core::ObjectId groupId = f.page().objects.back().common.id;
        f.tools.user().setSelection({groupId});
        f.editing.duplicate();
        QCOMPARE(f.objectCount(), 6);
        QCOMPARE(f.selection().size(), std::size_t(1));
        QVERIFY(f.selection()[0] != groupId);
        QVERIFY(std::holds_alternative<core::Group>(core::body(f.page().objects.back())));
        QCOMPARE(f.selection()[0], f.page().objects.back().common.id);
    }

    void pasteGroupSelectsOnlyTheCopy()
    {
        Fixture f(2);
        const std::vector<core::ObjectId> members = f.ids();
        core::PageObject group = core::Group{members};
        group.common.layer = core::Layer::Middle;
        f.document.execute(std::make_unique<core::AddObjectCommand>(0, std::move(group)));
        const core::ObjectId groupId = f.page().objects.back().common.id;
        f.tools.user().setSelection({groupId});
        f.editing.copy();
        f.editing.paste();
        QCOMPARE(f.objectCount(), 6);
        QCOMPARE(f.selection().size(), std::size_t(1));
        QVERIFY(f.selection()[0] != groupId);
        QVERIFY(std::holds_alternative<core::Group>(core::body(f.page().objects.back())));
    }

    void duplicateTwoSimpleObjectsSelectsBothCopies()
    {
        Fixture f(2);
        const std::vector<core::ObjectId> before = f.ids();
        f.editing.duplicate();
        QCOMPARE(f.objectCount(), 4);
        QCOMPARE(f.selection().size(), std::size_t(2));
        for (const core::ObjectId id : f.selection())
            QVERIFY(std::find(before.begin(), before.end(), id) == before.end());
    }

    void flagsAreToggledAndReported()
    {
        Fixture f;
        QVERIFY(f.editing.hasSelection());
        QVERIFY(!f.editing.selectionLocked());
        f.editing.toggleFlag(QStringLiteral("lock"));
        QVERIFY(f.editing.selectionLocked());
        f.editing.toggleFlag(QStringLiteral("lock"));
        QVERIFY(!f.editing.selectionLocked());
        f.editing.toggleFlag(QStringLiteral("hide"));
        QVERIFY(f.editing.selectionHidden());
        f.editing.toggleFlag(QStringLiteral("drag-copy"));
        QVERIFY(f.editing.selectionDragCopy());
    }

    void arrangeFrontMovesToTop()
    {
        Fixture f(3, false);
        const core::ObjectId first = f.ids().front();
        f.tools.user().setSelection({first});
        f.editing.arrange(QStringLiteral("front"));
        QCOMPARE(f.ids().back(), first);
        f.editing.arrange(QStringLiteral("back"));
        QCOMPARE(f.ids().front(), first);
    }

    void pasteAtCentersOnPoint()
    {
        Fixture f;
        f.editing.copy();
        f.editing.pasteAt(QPointF(500, 400));
        QCOMPARE(f.objectCount(), 2);
        const core::RectF bounds = f.lastBounds();
        QVERIFY(qAbs(bounds.center().x - 500.0) < 1.0);
        QVERIFY(qAbs(bounds.center().y - 400.0) < 1.0);
    }

    void mirrorAddsCopyAndFlipKeepsCount()
    {
        Fixture f;
        f.editing.transform(QStringLiteral("mirror-y"));
        QCOMPARE(f.objectCount(), 2);
        Fixture g;
        g.editing.transform(QStringLiteral("flip-y"));
        QCOMPARE(g.objectCount(), 1);
    }

    void originFlipIsInertWithoutOrigin()
    {
        Fixture f;
        f.clearSpy();
        f.editing.transform(QStringLiteral("origin-flip-x"));
        QVERIFY(f.kinds.empty());
    }

    void noSelectionStillStacksNothingForObjectCommands()
    {
        Fixture f;
        f.clearSelection();
        f.clearSpy();
        f.editing.align(QStringLiteral("left"));
        f.editing.toggleGroup();
        f.editing.arrange(QStringLiteral("front"));
        f.editing.toggleFlag(QStringLiteral("lock"));
        QVERIFY(f.kinds.empty());
        QVERIFY(!f.editing.hasSelection());
    }

    // L5-C17 (M1, D6) : un groupe s'aligne comme un bloc ; ses membres gardent leurs positions relatives.
    void alignMovesGroupAsABlock()
    {
        Fixture f(3, false);
        const std::vector<core::ObjectId> all = f.ids();
        core::PageObject group = core::Group{{all[0], all[1]}};
        group.common.layer = core::Layer::Middle;
        f.document.execute(std::make_unique<core::AddObjectCommand>(0, std::move(group)));
        const core::ObjectId groupId = f.page().objects.back().common.id;
        f.tools.user().setSelection({groupId, all[2]});
        f.editing.align(QStringLiteral("left"));
        const auto at = [&f](core::ObjectId id) {
            return core::boundsOnPage(f.page().objects[core::indexOf(f.page(), id)]);
        };
        QVERIFY(qAbs(at(all[0]).x - 100.0) < 1.0);
        QVERIFY(qAbs(at(all[1]).x - 300.0) < 1.0); // le membre ne rejoint pas le bord du groupe
        QVERIFY(qAbs(at(all[2]).x - 100.0) < 1.0); // l'objet seul rejoint le bord gauche du groupe
    }

    void alignOfASingleGroupDoesNothing()
    {
        Fixture f(2, false);
        const std::vector<core::ObjectId> all = f.ids();
        core::PageObject group = core::Group{all};
        group.common.layer = core::Layer::Middle;
        f.document.execute(std::make_unique<core::AddObjectCommand>(0, std::move(group)));
        f.tools.user().setSelection({f.page().objects.back().common.id});
        f.clearSpy();
        f.editing.align(QStringLiteral("left"));
        QVERIFY(f.kinds.empty());
    }

    void flipOfAGroupFlipsTheWholeBlock()
    {
        Fixture f(2, false);
        const std::vector<core::ObjectId> all = f.ids();
        core::PageObject group = core::Group{all};
        group.common.layer = core::Layer::Middle;
        f.document.execute(std::make_unique<core::AddObjectCommand>(0, std::move(group)));
        f.tools.user().setSelection({f.page().objects.back().common.id});
        f.editing.transform(QStringLiteral("flip-y")); // gauche / droite autour du centre du groupe
        QVERIFY(qAbs(core::boundsOnPage(f.page().objects[0]).x - 300.0) < 1.0);
        QVERIFY(qAbs(core::boundsOnPage(f.page().objects[1]).x - 100.0) < 1.0);
    }

    // L5-C17 (M7, D9) : règle retenue, le drapeau posé sur le groupe est propagé à ses membres.
    void lockAndHideOfAGroupReachItsMembers()
    {
        Fixture f(2, false);
        const std::vector<core::ObjectId> all = f.ids();
        core::PageObject group = core::Group{all};
        group.common.layer = core::Layer::Middle;
        f.document.execute(std::make_unique<core::AddObjectCommand>(0, std::move(group)));
        f.tools.user().setSelection({f.page().objects.back().common.id});
        f.editing.toggleFlag(QStringLiteral("lock"));
        QVERIFY(f.page().objects[0].common.locked && f.page().objects[1].common.locked);
        f.editing.toggleFlag(QStringLiteral("hide"));
        QVERIFY(!f.page().objects[0].common.visible && !f.page().objects[1].common.visible);
        f.document.undo();
        QVERIFY(f.page().objects[0].common.visible && f.page().objects[1].common.visible);
        QVERIFY(f.page().objects[0].common.locked); // le verrou ne change aucun pixel : hors de la pile
    }

    // L5-C17 (mineur) : un objet sans emprise ne compte pas dans le centrage de « Coller ici ».
    void pasteAtIgnoresObjectsWithoutExtent()
    {
        Fixture f(1, false);
        core::PageObject empty = core::Line{};
        empty.common.layer = core::Layer::Middle;
        f.document.execute(std::make_unique<core::AddObjectCommand>(0, std::move(empty)));
        f.tools.user().setSelection(f.ids());
        f.editing.copy();
        f.editing.pasteAt(QPointF(500, 400));
        const core::RectF pasted = core::boundsOnPage(f.page().objects[f.page().objects.size() - 2]);
        QCOMPARE(pasted.center().x, 500.0);
        QCOMPARE(pasted.center().y, 400.0);
    }
};

QTEST_MAIN(TestObjectEditing)
#include "tst_ui_objectediting.moc"
