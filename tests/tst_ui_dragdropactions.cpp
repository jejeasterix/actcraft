// L8-T13 : actions glisser-déposer (F-892, D8-11).
//
// Invariants gardés :
// - Liste lue dans le catalogue : 18 entrées de glisser-déposer, aucune de vote.
// - Une étape : un dépôt, un retrait par la corbeille, s'annulent d'un seul undo.
// - Les actions au clic ne sont jamais effacées par un dépôt ni par un retrait : SetObjectActionsCommand
//   (L8-T11) remplace toute la liste, le modèle recompose donc la liste entière.
// - La surimpression ne liste que les objets dont une action a dragDrop.
// - Fermé par défaut : sans Commands, ou sous le verrou, rien ne change et refused est émis.
#include <actcraft/actions/objectactions.h>
#include <actcraft/core/actionedits.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/dragdropactions.h>

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace core = actcraft::core;
namespace actions = actcraft::actions;
namespace ui = actcraft::ui;

namespace {

core::ObjectId addShape(ui::DocumentController& controller, double x)
{
    core::PageObject object =
        core::makeShapeObject("rectangle", core::RectF{x, 100, 80, 60}, core::ShapeStyle{});
    object.common.layer = core::Layer::Middle;
    std::vector<core::ObjectId> before;
    for (const core::PageObject& o : controller.document().page(0).objects) {
        before.push_back(o.common.id);
    }
    controller.execute(std::make_unique<core::AddObjectCommand>(0, std::move(object)));
    for (const core::PageObject& o : controller.document().page(0).objects) {
        if (std::find(before.begin(), before.end(), o.common.id) == before.end()) {
            return o.common.id;
        }
    }
    return 0;
}

const core::PageObject* objectOf(const ui::DocumentController& controller, core::ObjectId id)
{
    for (const core::PageObject& object : controller.document().page(0).objects) {
        if (object.common.id == id) {
            return &object;
        }
    }
    return nullptr;
}

const std::vector<core::Action>& actionsOf(const ui::DocumentController& controller, core::ObjectId id)
{
    static const std::vector<core::Action> none;
    const core::PageObject* object = objectOf(controller, id);
    return object ? object->common.actions : none;
}

struct Fixture
{
    QTemporaryDir dir;
    ui::DocumentController controller;
    ui::Commands commands;
    ui::DragDropActions model;
    core::ObjectId first = 0;
    core::ObjectId second = 0;

    explicit Fixture(bool withCommands = true)
    {
        controller.newDocument(800, 600);
        commands.setProfileDir(dir.filePath(QStringLiteral("disposition")));
        if (withCommands) {
            model.setCommands(&commands);
        }
        model.setDocument(&controller);
        first = addShape(controller, 100);
        second = addShape(controller, 400);
        QCoreApplication::processEvents();
    }

    void putClickAction(core::ObjectId id)
    {
        core::Action action;
        action.catalogId = "page.next";
        controller.execute(std::make_unique<core::SetObjectActionsCommand>(
            0, std::vector<core::ObjectId>{id}, std::vector<core::Action>{action}));
    }
};

} // namespace

class TstUiDragdropactions : public QObject
{
    Q_OBJECT
private slots:
    void eighteenEntriesNoVote()
    {
        Fixture f;
        QCOMPARE(f.model.rowCount(), 18);
        actions::ObjectActionCatalog catalog;
        QString error;
        QVERIFY2(catalog.load(actions::defaultObjectActionsJson(), error), qPrintable(error));
        const auto expected = catalog.dragDropList();
        QCOMPARE(expected.size(), 18);
        for (int row = 0; row < f.model.rowCount(); ++row) {
            const QString id = f.model.data(f.model.index(row), ui::DragDropActions::IdRole).toString();
            QCOMPARE(id, expected[row]->id);
            QVERIFY2(!id.contains(QLatin1String("vote")), qPrintable(id));
            QVERIFY(!f.model.data(f.model.index(row), ui::DragDropActions::LabelRole).toString().isEmpty());
            QVERIFY(!f.model.data(f.model.index(row), ui::DragDropActions::IconRole).toString().isEmpty());
        }
    }

    void dropAssignsInOneStep()
    {
        Fixture f;
        QVERIFY(actionsOf(f.controller, f.first).empty());
        QVERIFY(f.model.dropOn(f.first, QStringLiteral("command.tool.pen")));
        const std::vector<core::Action>& list = actionsOf(f.controller, f.first);
        QCOMPARE(list.size(), std::size_t(1));
        QCOMPARE(QString::fromStdString(list[0].catalogId), QStringLiteral("command.tool.pen"));
        QVERIFY(list[0].dragDrop);
        QVERIFY(actionsOf(f.controller, f.second).empty()); // l'autre objet n'est pas touché
        QVERIFY(
            !f.model.dropOn(f.first, QStringLiteral("command.tool.pen"))); // déjà posée : aucune étape vide
        f.controller.undo();                                               // une seule étape
        QVERIFY(actionsOf(f.controller, f.first).empty());
        // Une entrée hors de la liste de l'onglet, un objet absent : rien.
        QVERIFY(!f.model.dropOn(f.first, QStringLiteral("page.inconnue")));
        QVERIFY(!f.model.dropOn(999999, QStringLiteral("command.tool.pen")));
        QVERIFY(actionsOf(f.controller, f.first).empty());
    }

    void overlayOnlyForDragDrop()
    {
        Fixture f;
        QVERIFY(f.model.overlayIcons().isEmpty());
        f.putClickAction(f.first); // une action au clic : aucune icône
        QCoreApplication::processEvents();
        QVERIFY(f.model.overlayIcons().isEmpty());
        QVERIFY(f.model.overlays().isEmpty());
        QSignalSpy changed(&f.model, &ui::DragDropActions::overlaysChanged);
        QVERIFY(f.model.dropOn(f.second, QStringLiteral("command.tool.pen")));
        QCoreApplication::processEvents();
        const QVariantList icons = f.model.overlayIcons();
        QCOMPARE(icons.size(), 1);
        const QVariantMap icon = icons.front().toMap();
        QCOMPARE(icon.value(QStringLiteral("objectId")).toULongLong(), quint64(f.second));
        QCOMPARE(icon.value(QStringLiteral("actionId")).toString(), QStringLiteral("command.tool.pen"));
        QVERIFY(!icon.value(QStringLiteral("icon")).toString().isEmpty());
        QVERIFY(qAbs(icon.value(QStringLiteral("x")).toDouble() - 400.0) <=
                4.0); // emprise : marge du contour
        QVERIFY(qAbs(icon.value(QStringLiteral("width")).toDouble() - 80.0) <= 8.0);
        QCOMPARE(changed.count(), 1);
        QCOMPARE(f.model.overlays(), icons);
    }

    void trashRemovesInOneStep()
    {
        Fixture f;
        QVERIFY(f.model.dropOn(f.first, QStringLiteral("command.tool.pen")));
        QVERIFY(f.model.dropOnTrash(f.first));
        QVERIFY(actionsOf(f.controller, f.first).empty());
        QVERIFY(!f.model.dropOnTrash(f.first)); // plus rien à retirer : aucune étape vide
        f.controller.undo();                    // une seule étape : l'action revient
        QCOMPARE(actionsOf(f.controller, f.first).size(), std::size_t(1));
        QVERIFY(actionsOf(f.controller, f.first)[0].dragDrop);
        QCoreApplication::processEvents();
        f.model.dropOnTrash(f.first);
        QCoreApplication::processEvents();
        QVERIFY(f.model.overlayIcons().isEmpty());
    }

    void clickActionSurvivesDropAndTrash()
    {
        Fixture f;
        f.putClickAction(f.first);
        QVERIFY(f.model.dropOn(f.first, QStringLiteral("command.tool.pen")));
        QCOMPARE(actionsOf(f.controller, f.first).size(), std::size_t(2));
        QCOMPARE(QString::fromStdString(actionsOf(f.controller, f.first)[0].catalogId),
                 QStringLiteral("page.next"));
        QVERIFY(!actionsOf(f.controller, f.first)[0].dragDrop);
        // Un second dépôt remplace l'action de glisser-déposer, pas celle du clic.
        QVERIFY(f.model.dropOn(f.first, QStringLiteral("command.tool.eraser")));
        QCOMPARE(actionsOf(f.controller, f.first).size(), std::size_t(2));
        QCOMPARE(QString::fromStdString(actionsOf(f.controller, f.first)[1].catalogId),
                 QStringLiteral("command.tool.eraser"));
        QVERIFY(f.model.dropOnTrash(f.first));
        QCOMPARE(actionsOf(f.controller, f.first).size(), std::size_t(1));
        QCOMPARE(QString::fromStdString(actionsOf(f.controller, f.first)[0].catalogId),
                 QStringLiteral("page.next"));
    }

    void dropAtFindsTheObject()
    {
        Fixture f;
        QVERIFY(!f.model.dropAt(QPointF(300, 400), QStringLiteral("command.tool.pen"), 1.0)); // page vide
        QVERIFY(f.model.dropAt(QPointF(430, 130), QStringLiteral("command.tool.pen"), 1.0));
        QVERIFY(actionsOf(f.controller, f.first).empty());
        QCOMPARE(actionsOf(f.controller, f.second).size(), std::size_t(1));
    }

    // Revue L8-C13 : la tolérance est en pixels d'écran, donc plus étroite en page quand on zoome.
    void dropAtToleranceFollowsTheZoom()
    {
        Fixture f;
        const QPointF outside(483, 130); // 3 px hors du bord droit de la seconde forme (400..480)
        QVERIFY(!f.model.dropAt(outside, QStringLiteral("command.tool.pen"), 4.0)); // 6 / 4 = 1,5 px
        QVERIFY(actionsOf(f.controller, f.second).empty());
        QVERIFY(f.model.dropAt(outside, QStringLiteral("command.tool.pen"), 1.0)); // 6 px
        QCOMPARE(actionsOf(f.controller, f.second).size(), std::size_t(1));
    }

    void closedByDefault()
    {
        {
            Fixture f(false); // sans commandes
            QSignalSpy refused(&f.model, &ui::DragDropActions::refused);
            QVERIFY(!f.model.dropOn(f.first, QStringLiteral("command.tool.pen")));
            QCOMPARE(refused.count(), 1);
            QVERIFY(actionsOf(f.controller, f.first).empty());
        }
        Fixture f;
        QVERIFY(f.model.dropOn(f.first, QStringLiteral("command.tool.pen")));
        f.commands.setLocked(true);
        QSignalSpy refused(&f.model, &ui::DragDropActions::refused);
        QVERIFY(!f.model.dropOn(f.second, QStringLiteral("command.tool.pen")));
        QVERIFY(!f.model.dropOnTrash(f.first));
        QCOMPARE(refused.count(), 2);
        QCOMPARE(actionsOf(f.controller, f.first).size(), std::size_t(1));
        QVERIFY(actionsOf(f.controller, f.second).empty());
    }
};

QTEST_MAIN(TstUiDragdropactions)
#include "tst_ui_dragdropactions.moc"
