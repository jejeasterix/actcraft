// L6-T14 : modèle du navigateur d'objet (F-820, F-821).
//
// Invariant gardé (amendement 4, A9) : verrouiller ne rend aucun core::Change (objectflags.h) ; le contrôleur
// n'émet alors ni observateur, ni pageContentChanged, ni documentChanged. Un modèle qui ne suivrait que les
// signaux du document (comme PageThumbnails) garderait LockedRole périmé : lockFromElsewhereRefreshes et
// toggleHiddenAndLocked le distinguent.
#include <actcraft/actions/commandregistry.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/objectflags.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/objecttree.h>
#include <actcraft/ui/toolhost.h>

#include <QTemporaryDir>
#include <QtTest>

#include <algorithm>
#include <memory>
#include <vector>

namespace core = actcraft::core;
namespace ui = actcraft::ui;

namespace {

std::vector<core::ObjectId> idsOnPage(const ui::DocumentController& controller)
{
    std::vector<core::ObjectId> ids;
    for (const core::PageObject& object : controller.document().page(0).objects) {
        ids.push_back(object.common.id);
    }
    return ids;
}

// Ajoute `object` à la page 1 par la pile, dans `layer` ; rend son identifiant.
core::ObjectId addObject(ui::DocumentController& controller, core::PageObject object, core::Layer layer)
{
    object.common.layer = layer;
    const std::vector<core::ObjectId> before = idsOnPage(controller);
    controller.execute(std::make_unique<core::AddObjectCommand>(0, std::move(object)));
    for (const core::ObjectId id : idsOnPage(controller)) {
        if (std::find(before.begin(), before.end(), id) == before.end()) {
            return id;
        }
    }
    return 0;
}

core::PageObject shapeAt(double x)
{
    return core::makeShapeObject("rectangle", core::RectF{x, 100, 80, 60}, core::ShapeStyle{});
}

core::PageObject strokeObject()
{
    core::Stroke stroke;
    stroke.points = {{10, 10}, {60, 40}, {120, 30}};
    return core::PageObject{stroke};
}

core::PageObject imageObject()
{
    core::ImageObject image;
    image.bounds = core::RectF{300, 300, 64, 64};
    return core::PageObject{image};
}

// ui::Commands n'expose que registry() const ; le verrou du mode classe (L6-T31) n'a pas encore de voie dans
// Commands. Le registre n'est pas un objet const : const_cast est sûr ici.
void setLocked(ui::Commands& commands, bool locked)
{
    const_cast<actcraft::actions::CommandRegistry&>(commands.registry()).setLocked(locked);
}

int rowOf(const ui::ObjectTreeModel& tree, core::ObjectId id)
{
    for (int row = 0; row < tree.rowCount(); ++row) {
        const QModelIndex index = tree.index(row);
        if (tree.data(index, ui::ObjectTreeModel::DepthRole).toInt() == 1 &&
            tree.data(index, ui::ObjectTreeModel::ObjectIdRole).toULongLong() == id) {
            return row;
        }
    }
    return -1;
}

int layerRow(const ui::ObjectTreeModel& tree, const QString& layer)
{
    for (int row = 0; row < tree.rowCount(); ++row) {
        const QModelIndex index = tree.index(row);
        if (tree.data(index, ui::ObjectTreeModel::DepthRole).toInt() == 0 &&
            tree.data(index, ui::ObjectTreeModel::LayerRole).toString() == layer) {
            return row;
        }
    }
    return -1;
}

QVariant roleOf(const ui::ObjectTreeModel& tree, core::ObjectId id, int role)
{
    return tree.data(tree.index(rowOf(tree, id)), role);
}

bool lockedInDocument(const ui::DocumentController& controller, core::ObjectId id)
{
    return core::flagOfAll(controller.document().page(0), {id}, core::ObjectFlag::Locked);
}

bool hiddenInDocument(const ui::DocumentController& controller, core::ObjectId id)
{
    return core::flagOfAll(controller.document().page(0), {id}, core::ObjectFlag::Hidden);
}

// Document de la section : un trait couche du dessus, deux formes intermédiaires, une image inférieure.
struct Fixture
{
    QTemporaryDir dir;
    ui::DocumentController controller;
    ui::Commands commands;
    ui::ToolHost tools;
    ui::ObjectTreeModel tree;
    core::ObjectId stroke = 0;
    core::ObjectId shape1 = 0;
    core::ObjectId shape2 = 0;
    core::ObjectId image = 0;

    Fixture()
    {
        controller.newDocument(800, 600);
        commands.setProfileDir(dir.filePath(QStringLiteral("disposition")));
        tools.setSettingsFile(dir.filePath(QStringLiteral("outils.ini")));
        tools.setCommands(&commands);
        tools.setDocumentController(&controller);
        tree.setCommands(&commands); // amendement 3, n1 : fermé par défaut
        tree.setDocument(&controller);
        tree.setTools(&tools);
        stroke = addObject(controller, strokeObject(), core::Layer::Top);
        shape1 = addObject(controller, shapeAt(100), core::Layer::Middle);
        shape2 = addObject(controller, shapeAt(250), core::Layer::Middle);
        image = addObject(controller, imageObject(), core::Layer::Bottom);
        QCoreApplication::processEvents(); // remise à jour regroupée faite
    }
};

} // namespace

class TstUiObjecttree : public QObject
{
    Q_OBJECT
private slots:
    void fourLayerNodesTopFirst()
    {
        Fixture f;
        QVERIFY(f.stroke != 0 && f.shape1 != 0 && f.shape2 != 0 && f.image != 0);
        QStringList layers;
        for (int row = 0; row < f.tree.rowCount(); ++row) {
            const QModelIndex index = f.tree.index(row);
            if (f.tree.data(index, ui::ObjectTreeModel::DepthRole).toInt() == 0) {
                layers << f.tree.data(index, ui::ObjectTreeModel::LayerRole).toString();
                QCOMPARE(f.tree.data(index, ui::ObjectTreeModel::ObjectIdRole).toULongLong(), 0ULL);
            }
        }
        QCOMPARE(layers, (QStringList{QStringLiteral("top"), QStringLiteral("middle"),
                                      QStringLiteral("bottom"), QStringLiteral("background")}));
        QCOMPARE(f.tree.rowCount(), 8); // quatre couches, quatre objets
    }

    void objectsTopmostFirst()
    {
        Fixture f;
        const int top = layerRow(f.tree, QStringLiteral("top"));
        const int middle = layerRow(f.tree, QStringLiteral("middle"));
        const int bottom = layerRow(f.tree, QStringLiteral("bottom"));
        QCOMPARE(rowOf(f.tree, f.stroke), top + 1);
        QCOMPARE(rowOf(f.tree, f.shape2), middle + 1); // ajoutée en dernier : au-dessus
        QCOMPARE(rowOf(f.tree, f.shape1), middle + 2);
        QCOMPARE(rowOf(f.tree, f.image), bottom + 1);
        QCOMPARE(roleOf(f.tree, f.stroke, ui::ObjectTreeModel::KindRole).toString(),
                 QStringLiteral("stroke"));
        QCOMPARE(roleOf(f.tree, f.shape1, ui::ObjectTreeModel::KindRole).toString(), QStringLiteral("shape"));
        QCOMPARE(roleOf(f.tree, f.image, ui::ObjectTreeModel::KindRole).toString(), QStringLiteral("image"));
        QCOMPARE(roleOf(f.tree, f.shape1, ui::ObjectTreeModel::LayerRole).toString(),
                 QStringLiteral("middle"));
        QVERIFY(!roleOf(f.tree, f.shape1, ui::ObjectTreeModel::NameRole).toString().isEmpty());
        QCOMPARE(roleOf(f.tree, f.shape1, ui::ObjectTreeModel::HiddenRole).toBool(), false);
        QCOMPARE(roleOf(f.tree, f.shape1, ui::ObjectTreeModel::LockedRole).toBool(), false);
    }

    void clickSelects()
    {
        Fixture f;
        const bool couldUndo = f.controller.canUndo();
        const QString fingerprint = f.controller.fingerprintText();
        f.tree.select(rowOf(f.tree, f.shape1), 0);
        QCOMPARE(f.tools.user().selection(), (std::vector<core::ObjectId>{f.shape1}));
        QCOMPARE(f.tools.currentTool(), QStringLiteral("select"));
        QCOMPARE(f.controller.canUndo(), couldUndo);
        QCOMPARE(f.controller.fingerprintText(), fingerprint); // aucun Change du document
        QCoreApplication::processEvents();
        QCOMPARE(roleOf(f.tree, f.shape1, ui::ObjectTreeModel::SelectedRole).toBool(), true);
        QCOMPARE(roleOf(f.tree, f.shape2, ui::ObjectTreeModel::SelectedRole).toBool(), false);

        f.tree.select(layerRow(f.tree, QStringLiteral("middle")), 0); // ligne de couche : rien
        QCOMPARE(f.tools.user().selection(), (std::vector<core::ObjectId>{f.shape1}));
    }

    void ctrlClickTogglesTheSelection()
    {
        Fixture f;
        f.tree.select(rowOf(f.tree, f.shape1), 0);
        f.tree.select(rowOf(f.tree, f.shape2), Qt::ControlModifier);
        std::vector<core::ObjectId> selection = f.tools.user().selection();
        std::sort(selection.begin(), selection.end());
        std::vector<core::ObjectId> both{f.shape1, f.shape2};
        std::sort(both.begin(), both.end());
        QCOMPARE(selection, both);
        f.tree.select(rowOf(f.tree, f.shape1), Qt::ControlModifier);
        QCOMPARE(f.tools.user().selection(), (std::vector<core::ObjectId>{f.shape2}));
    }

    void dragReordersInOneStep()
    {
        Fixture f;
        const QString before = f.controller.fingerprintText();
        // La seconde ligne sous « middle » (shape1) passe au-dessus de la première (shape2).
        const int middle = layerRow(f.tree, QStringLiteral("middle"));
        QVERIFY(f.tree.move(middle + 2, middle + 1));
        QCoreApplication::processEvents();
        QCOMPARE(rowOf(f.tree, f.shape1), middle + 1);
        QCOMPARE(rowOf(f.tree, f.shape2), middle + 2);
        QVERIFY(f.controller.fingerprintText() != before);
        f.controller.undo(); // une seule étape la rend
        QCOMPARE(f.controller.fingerprintText(), before);
        QCoreApplication::processEvents();
        QCOMPARE(rowOf(f.tree, f.shape2), middle + 1);

        // Vers une autre couche : l'image passe au sommet de la couche intermédiaire, une étape.
        QVERIFY(f.tree.move(rowOf(f.tree, f.image), middle + 1));
        QCoreApplication::processEvents();
        QCOMPARE(roleOf(f.tree, f.image, ui::ObjectTreeModel::LayerRole).toString(),
                 QStringLiteral("middle"));
        QCOMPARE(rowOf(f.tree, f.image), layerRow(f.tree, QStringLiteral("middle")) + 1);
        f.controller.undo();
        QCOMPARE(f.controller.fingerprintText(), before);
        QCoreApplication::processEvents();
        QCOMPARE(roleOf(f.tree, f.image, ui::ObjectTreeModel::LayerRole).toString(),
                 QStringLiteral("bottom"));
    }

    void dragToBackgroundRefused()
    {
        Fixture f;
        const QString before = f.controller.fingerprintText();
        const bool couldUndo = f.controller.canUndo();
        const int background = layerRow(f.tree, QStringLiteral("background"));
        QVERIFY(!f.tree.move(rowOf(f.tree, f.shape1), background + 1));       // sous « background »
        QVERIFY(!f.tree.move(layerRow(f.tree, QStringLiteral("middle")), 0)); // une couche seule
        QCOMPARE(f.controller.fingerprintText(), before);
        QCOMPARE(f.controller.canUndo(), couldUndo);
        QCoreApplication::processEvents();
        QCOMPARE(roleOf(f.tree, f.shape1, ui::ObjectTreeModel::LayerRole).toString(),
                 QStringLiteral("middle"));
    }

    void toggleHiddenAndLocked()
    {
        Fixture f;
        QVERIFY(f.tree.setHidden(rowOf(f.tree, f.shape1), true)); // une étape
        QVERIFY(hiddenInDocument(f.controller, f.shape1));
        QCoreApplication::processEvents();
        QCOMPARE(roleOf(f.tree, f.shape1, ui::ObjectTreeModel::HiddenRole).toBool(), true);

        const int resets = f.tree.resetCount();
        QVERIFY(f.tree.setLocked(rowOf(f.tree, f.shape1), true));
        QVERIFY(lockedInDocument(f.controller, f.shape1));
        QCoreApplication::processEvents();
        // Le document n'a rien émis (aucun Change) : le modèle s'est remis à jour lui-même.
        QCOMPARE(f.tree.resetCount(), resets + 1);
        QCOMPARE(roleOf(f.tree, f.shape1, ui::ObjectTreeModel::LockedRole).toBool(), true);

        // Aucune étape ajoutée par le verrou : Annuler défait le masquage, le verrou reste.
        f.controller.undo();
        QVERIFY(!hiddenInDocument(f.controller, f.shape1));
        QVERIFY(lockedInDocument(f.controller, f.shape1));
    }

    void resetsAreBatched()
    {
        Fixture f;
        const int resets = f.tree.resetCount();
        addObject(f.controller, shapeAt(400), core::Layer::Middle);
        addObject(f.controller, shapeAt(500), core::Layer::Middle);
        addObject(f.controller, shapeAt(600), core::Layer::Top);
        QCOMPARE(f.tree.resetCount(), resets); // rien dans le même tour
        QCoreApplication::processEvents();
        QCOMPARE(f.tree.resetCount(), resets + 1);
        QCOMPARE(f.tree.rowCount(), 11);
    }

    void noRefreshDuringContact()
    {
        Fixture f;
        const int resets = f.tree.resetCount();
        QVERIFY(f.tree.setProperty("contactActive", true));
        addObject(f.controller, shapeAt(400), core::Layer::Middle);
        QCoreApplication::processEvents();
        QCOMPARE(f.tree.resetCount(), resets);
        QCOMPARE(f.tree.rowCount(), 8);
        QVERIFY(f.tree.setProperty("contactActive", false));
        QCoreApplication::processEvents();
        QCOMPARE(f.tree.resetCount(), resets + 1);
        QCOMPARE(f.tree.rowCount(), 9);
    }

    void refusedWhileLocked()
    {
        Fixture f;
        const QString before = f.controller.fingerprintText();
        const int middle = layerRow(f.tree, QStringLiteral("middle"));
        setLocked(f.commands, true);
        QVERIFY(!f.tree.move(middle + 2, middle + 1));
        QVERIFY(!f.tree.setHidden(middle + 1, true));
        QVERIFY(!f.tree.setLocked(middle + 1, true));
        QCOMPARE(f.controller.fingerprintText(), before);
        QVERIFY(!lockedInDocument(f.controller, f.shape2));
        setLocked(f.commands, false);
        QVERIFY(f.tree.setHidden(middle + 1, true));
    }

    void refusedWithoutCommands()
    {
        Fixture f;
        f.tree.setCommands(nullptr);
        const QString before = f.controller.fingerprintText();
        const int middle = layerRow(f.tree, QStringLiteral("middle"));
        QVERIFY(!f.tree.move(middle + 2, middle + 1));
        QVERIFY(!f.tree.setHidden(middle + 1, true));
        QVERIFY(!f.tree.setLocked(middle + 1, true));
        QCOMPARE(f.controller.fingerprintText(), before);
        QVERIFY(!lockedInDocument(f.controller, f.shape2));
    }

    // Le test qui distingue : sous le patron de PageThumbnails (signaux du document seulement), la seconde
    // moitié resterait à `resets` et LockedRole à faux.
    void lockFromElsewhereRefreshes()
    {
        Fixture f;
        const int resets = f.tree.resetCount();
        f.controller.execute(std::make_unique<core::SetObjectFlagCommand>(0, std::vector{f.shape1},
                                                                          core::ObjectFlag::Locked, true));
        QVERIFY(lockedInDocument(f.controller, f.shape1));
        QCoreApplication::processEvents();
        QCOMPARE(f.tree.resetCount(), resets); // le document n'a rien émis

        emit f.commands.triggered(QStringLiteral("view.fit"), QVariant()); // commande étrangère : rien
        QCoreApplication::processEvents();
        QCOMPARE(f.tree.resetCount(), resets);

        emit f.commands.triggered(QStringLiteral("object.lock"), QVariant());
        QCoreApplication::processEvents();
        QCOMPARE(f.tree.resetCount(), resets + 1);
        QCOMPARE(roleOf(f.tree, f.shape1, ui::ObjectTreeModel::LockedRole).toBool(), true);

        emit f.commands.triggered(QStringLiteral("object.drag-copy"), QVariant());
        QCoreApplication::processEvents();
        QCOMPARE(f.tree.resetCount(), resets + 2);
    }
};

QTEST_MAIN(TstUiObjecttree)
#include "tst_ui_objecttree.moc"
