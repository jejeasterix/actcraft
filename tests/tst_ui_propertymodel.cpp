// L6-T12 : modèle du navigateur de propriétés (F-570 à F-576, F-581, F-583, F-840).
//
// Invariants gardés :
// - Verrou (L6-C2, H6-15) : verrouiller ne rend aucun core::Change (objectflags.h) ; le contrôleur émet alors
//   objectFlagsChanged, et lui seul, quel que soit le chemin. Un modèle qui suivrait Commands::triggered
//   (« object.lock », « object.drag-copy ») garderait la valeur « locked » périmée après un verrou posé par
//   un autre chemin (DocumentController::execute direct, annulation, dépôt) : lockFromElsewhereRefreshes le
//   distingue, sans aucun triggered.
// - Étapes (L6-T11) : SetObjectPropertiesCommand fusionne avec la suivante de même page, mêmes objets, même
//   propriété (réglette glissée). Une modification ponctuelle est close aussitôt (closeMerge) : sans cela,
//   deux saisies successives ne feraient qu'une étape. pointEditsStaySeparateSteps le distingue.
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/objectedits.h>
#include <actcraft/core/objectflags.h>
#include <actcraft/core/propertyedits.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/propertymodel.h>
#include <actcraft/ui/toolhost.h>
#include <actcraft/ui/viewtransform.h>

#include <QElapsedTimer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSet>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include <algorithm>
#include <limits>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

namespace core = actcraft::core;
namespace render = actcraft::render;
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

const core::PageObject* objectOf(const ui::DocumentController& controller, core::ObjectId id)
{
    for (const core::PageObject& object : controller.document().page(0).objects) {
        if (object.common.id == id) {
            return &object;
        }
    }
    return nullptr;
}

// Valeur de `property` dans le document (pas dans le modèle).
std::optional<core::PropertyValue> inDocument(const ui::DocumentController& controller, core::ObjectId id,
                                              core::ObjectProperty property)
{
    const core::PageObject* object = objectOf(controller, id);
    return object ? core::readProperty(*object, property) : std::nullopt;
}

double outlineWidth(const ui::DocumentController& controller, core::ObjectId id)
{
    const std::optional<core::PropertyValue> value =
        inDocument(controller, id, core::ObjectProperty::OutlineWidth);
    return value && std::holds_alternative<double>(*value) ? std::get<double>(*value) : -1.0;
}

bool lockedInDocument(const ui::DocumentController& controller, core::ObjectId id)
{
    return core::flagOfAll(controller.document().page(0), {id}, core::ObjectFlag::Locked);
}

bool hiddenInDocument(const ui::DocumentController& controller, core::ObjectId id)
{
    return core::flagOfAll(controller.document().page(0), {id}, core::ObjectFlag::Hidden);
}

double translucencyOf(const ui::DocumentController& controller, core::ObjectId id)
{
    const std::optional<core::PropertyValue> value =
        inDocument(controller, id, core::ObjectProperty::Translucency);
    return value && std::holds_alternative<double>(*value) ? std::get<double>(*value) : -1.0;
}

QVariant roleOf(const ui::PropertyModel& model, const QString& key, int role)
{
    const int row = model.rowOf(key);
    return row < 0 ? QVariant() : model.data(model.index(row), role);
}

QVariant valueOf(const ui::PropertyModel& model, const QString& key)
{
    return roleOf(model, key, ui::PropertyModel::ValueRole);
}

QSet<QString> sectionsOf(const ui::PropertyModel& model)
{
    QSet<QString> sections;
    for (int row = 0; row < model.rowCount(); ++row) {
        sections.insert(model.data(model.index(row), ui::PropertyModel::SectionRole).toString());
    }
    return sections;
}

// Document de la section : un trait (couche du dessus) et deux formes (couche intermédiaire).
struct Fixture
{
    QTemporaryDir dir;
    ui::DocumentController controller;
    ui::Commands commands;
    ui::ToolHost tools;
    ui::PropertyModel model;
    core::ObjectId stroke = 0;
    core::ObjectId shape1 = 0;
    core::ObjectId shape2 = 0;

    Fixture()
    {
        controller.newDocument(800, 600);
        commands.setProfileDir(dir.filePath(QStringLiteral("disposition")));
        tools.setSettingsFile(dir.filePath(QStringLiteral("outils.ini")));
        tools.setCommands(&commands);
        tools.setDocumentController(&controller);
        model.setCommands(&commands); // amendement 3, n1
        model.setDocument(&controller);
        model.setTools(&tools);
        stroke = addObject(controller, strokeObject(), core::Layer::Top);
        shape1 = addObject(controller, shapeAt(100), core::Layer::Middle);
        shape2 = addObject(controller, shapeAt(250), core::Layer::Middle);
        QCoreApplication::processEvents(); // remise à jour regroupée faite
    }

    void select(const std::vector<core::ObjectId>& ids)
    {
        tools.selectObjects(ids);
        QCoreApplication::processEvents();
    }
};

// Élément de l'arbre visuel nommé `name` sous `from` (les délégués d'une vue n'ont pas toujours `from` pour
// parent QObject).
QQuickItem* findItem(QQuickItem* from, const QString& name)
{
    if (from == nullptr) {
        return nullptr;
    }
    if (from->objectName() == name) {
        return from;
    }
    const QList<QQuickItem*> children = from->childItems();
    for (QQuickItem* child : children) {
        if (QQuickItem* found = findItem(child, name)) {
            return found;
        }
    }
    return nullptr;
}

// Le montage ci-dessus, plus la zone de page réelle (PageArea : vue unique et son cache) dans une fenêtre
// hors écran, rendu logiciel, comme tst_ui_tabs.
struct ViewFixture
{
    QQmlEngine engine;
    std::unique_ptr<QObject> root;
    Fixture f;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
    render::FixedPageItem* fixed = nullptr;

    bool mount()
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport QtQuick.Window\nimport ActCraft.Ui\n"
                          "Window { width: 800; height: 600; visible: true\n"
                          "  PageArea { objectName: \"area\"; anchors.fill: parent } }",
                          QUrl(QStringLiteral("qrc:/tst_ui_propertymodel/Window.qml")));
        root.reset(component.create());
        if (!root) {
            qWarning() << component.errors();
            return false;
        }
        window = qobject_cast<QQuickWindow*>(root.get());
        QQuickItem* area = window ? window->findChild<QQuickItem*>(QStringLiteral("area")) : nullptr;
        if (!area) {
            return false;
        }
        area->setProperty("documentController", QVariant::fromValue(&f.controller));
        view = window->findChild<ui::PageView*>(QStringLiteral("pageView"));
        fixed = window->findChild<render::FixedPageItem*>(QStringLiteral("fixedPage"));
        if (!view || !fixed) {
            return false;
        }
        f.controller.setFixedPage(fixed);
        f.tools.setView(view);
        return QTest::qWaitForWindowExposed(window) && waitCacheStable();
    }

    // cacheReady, puis kCacheFoldIdleMs + 300 ms sans rendu terminé (D21), borné à 20 s.
    bool waitCacheStable()
    {
        if (!QTest::qWaitFor([this] { return fixed->isReady() && view->cacheReady(); }, 10000)) {
            return false;
        }
        int last = view->cacheRefreshCount();
        QElapsedTimer quiet;
        quiet.start();
        QElapsedTimer total;
        total.start();
        while (total.elapsed() < 20000) {
            QTest::qWait(50);
            if (view->cacheRefreshCount() != last || !view->cacheReady()) {
                last = view->cacheRefreshCount();
                quiet.restart();
            } else if (quiet.elapsed() >= ui::kCacheFoldIdleMs + 300) {
                return true;
            }
        }
        return false;
    }
};

} // namespace

class TstUiPropertymodel : public QObject
{
    Q_OBJECT

private slots:
    void interactivityDelegatesLoad();
    void pageWhenNothingSelected()
    {
        Fixture f;
        QCOMPARE(f.model.title(), QStringLiteral("Page"));
        for (const char* key : {"name", "description", "keywords", "width", "height"}) {
            QVERIFY2(f.model.rowOf(QString::fromLatin1(key)) >= 0, key);
        }
        QCOMPARE(f.model.rowOf(QStringLiteral("outline-width")), -1);
        QCOMPARE(valueOf(f.model, QStringLiteral("width")).toDouble(), 800.0);
        QCOMPARE(valueOf(f.model, QStringLiteral("height")).toDouble(), 600.0);
    }

    void sectionsOfAStroke()
    {
        Fixture f;
        f.select({f.stroke});
        QCOMPARE(f.model.title(), QStringLiteral("Trait"));
        const QSet<QString> sections = sectionsOf(f.model);
        for (const char* section : {"identification", "appearance", "outline", "position", "label"}) {
            QVERIFY2(sections.contains(QString::fromLatin1(section)), section);
        }
        QVERIFY(!sections.contains(QStringLiteral("fill")));
        QVERIFY(!sections.contains(QStringLiteral("page")));
    }

    void sectionsOfAShape()
    {
        Fixture f;
        f.select({f.shape1});
        QCOMPARE(f.model.title(), QStringLiteral("Forme"));
        const QSet<QString> sections = sectionsOf(f.model);
        QVERIFY(sections.contains(QStringLiteral("fill")));
        QVERIFY(sections.contains(QStringLiteral("outline")));
        QVERIFY(f.model.rowOf(QStringLiteral("fill-color")) >= 0);
    }

    void commonValuesOnly()
    {
        Fixture f;
        f.controller.execute(std::make_unique<core::SetObjectPropertiesCommand>(
            0, std::vector<core::ObjectId>{f.shape2}, core::ObjectProperty::OutlineWidth, 9.0));
        f.controller.closeMerge();
        QVERIFY(outlineWidth(f.controller, f.shape1) != outlineWidth(f.controller, f.shape2));
        f.select({f.shape1, f.shape2});
        QCOMPARE(f.model.title(), QStringLiteral("2 objets"));
        const int width = f.model.rowOf(QStringLiteral("outline-width"));
        QVERIFY(width >= 0);
        QVERIFY(!f.model.data(f.model.index(width), ui::PropertyModel::ValueRole).isValid());
        QVERIFY(valueOf(f.model, QStringLiteral("outline-color")).isValid()); // même couleur par défaut
        // Trait et forme : le remplissage ne s'applique pas au trait, il n'est pas commun.
        f.select({f.stroke, f.shape1});
        QCOMPARE(f.model.rowOf(QStringLiteral("fill-color")), -1);
    }

    void editGoesThroughTheUndoStack()
    {
        Fixture f;
        f.select({f.shape1});
        const double before = outlineWidth(f.controller, f.shape1);
        QVERIFY(before != 12.0);
        QVERIFY(f.model.setValue(f.model.rowOf(QStringLiteral("outline-width")), 12));
        QVERIFY(f.controller.canUndo());
        QCOMPARE(outlineWidth(f.controller, f.shape1), 12.0);
        QTRY_COMPARE(valueOf(f.model, QStringLiteral("outline-width")).toDouble(), 12.0);
        f.controller.undo(); // une étape
        QCOMPARE(outlineWidth(f.controller, f.shape1), before);
        // Annuler vide la sélection (L3-T0, ToolHost) : le modèle montre la page, puis relit l'objet.
        QTRY_COMPARE(f.model.title(), QStringLiteral("Page"));
        f.select({f.shape1});
        QCOMPARE(valueOf(f.model, QStringLiteral("outline-width")).toDouble(), before);
    }

    // Distingue (brief, point 2) : sans closeMerge après une modification ponctuelle, la seconde saisie
    // fusionnerait avec la première (SetObjectPropertiesCommand::mergeWith) et Annuler les défairait toutes
    // les deux.
    void pointEditsStaySeparateSteps()
    {
        Fixture f;
        f.select({f.shape1});
        const double before = outlineWidth(f.controller, f.shape1);
        const int row = f.model.rowOf(QStringLiteral("outline-width"));
        QCOMPARE(roleOf(f.model, QStringLiteral("outline-width"), ui::PropertyModel::EditorRole).toString(),
                 QStringLiteral("number"));
        QVERIFY(f.model.setValue(row, 12));
        QVERIFY(f.model.setValue(row, 14));
        QCOMPARE(outlineWidth(f.controller, f.shape1), 14.0);
        f.controller.undo();
        QCOMPARE(outlineWidth(f.controller, f.shape1), 12.0);
        f.controller.undo();
        QCOMPARE(outlineWidth(f.controller, f.shape1), before);
        // Page : même règle (SetPagePropertiesCommand fusionne aussi).
        f.tools.clearSelection();
        QCoreApplication::processEvents();
        const int name = f.model.rowOf(QStringLiteral("name"));
        QVERIFY(f.model.setValue(name, QStringLiteral("Un")));
        QVERIFY(f.model.setValue(name, QStringLiteral("Deux")));
        f.controller.undo();
        QTRY_COMPARE(valueOf(f.model, QStringLiteral("name")).toString(), QStringLiteral("Un"));
    }

    void refreshIsDeferredDuringContact()
    {
        Fixture f;
        f.select({f.shape1});
        const int before = f.model.refreshCount();
        f.model.setContactActive(true);
        for (int i = 0; i < 3; ++i) {
            f.controller.execute(std::make_unique<core::SetObjectPropertiesCommand>(
                0, std::vector<core::ObjectId>{f.shape1}, core::ObjectProperty::OutlineWidth, 20.0 + i));
            f.controller.closeMerge();
        }
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        QCOMPARE(f.model.refreshCount(), before);
        f.model.setContactActive(false);
        QCoreApplication::processEvents();
        QCOMPARE(f.model.refreshCount(), before + 1);
        QCoreApplication::processEvents();
        QCOMPARE(f.model.refreshCount(), before + 1);
        QCOMPARE(valueOf(f.model, QStringLiteral("outline-width")).toDouble(), 22.0);
    }

    void delegatedEditsUseTheLotCommands()
    {
        Fixture f;
        f.select({f.shape1});
        const QVariant translucencyBefore = valueOf(f.model, QStringLiteral("translucency"));
        QVERIFY(translucencyBefore.isValid());

        const QStringList layers =
            roleOf(f.model, QStringLiteral("layer"), ui::PropertyModel::ChoicesRole).toStringList();
        const int top = static_cast<int>(layers.indexOf(QStringLiteral("Couche du dessus")));
        QVERIFY(top >= 0);
        QVERIFY(f.model.setValue(f.model.rowOf(QStringLiteral("layer")), top));
        QVERIFY(objectOf(f.controller, f.shape1)->common.layer == core::Layer::Top);
        QTRY_COMPARE(valueOf(f.model, QStringLiteral("layer")).toInt(), top);

        QVERIFY(f.model.setValue(f.model.rowOf(QStringLiteral("visible")), false));
        QVERIFY(hiddenInDocument(f.controller, f.shape1));
        QTRY_COMPARE(valueOf(f.model, QStringLiteral("visible")), QVariant(false));

        QCOMPARE(roleOf(f.model, QStringLiteral("translucency"), ui::PropertyModel::EditorRole).toString(),
                 QStringLiteral("slider"));
        // Réglette glissée : deux positions sans closeMerge entre elles fusionnent (SetTranslucencyCommand),
        // le relâcher clôt l'étape.
        const int translucency = f.model.rowOf(QStringLiteral("translucency"));
        QVERIFY(f.model.setValue(translucency, 100));
        QVERIFY(f.model.setValue(translucency, 128));
        f.model.endEdit(); // relâcher de la réglette
        QTRY_COMPARE(valueOf(f.model, QStringLiteral("translucency")).toInt(), 128);

        // Verrou : hors de la pile (règle du lot 5) ; valeur relue vraie par objectFlagsChanged.
        QVERIFY(f.model.setValue(f.model.rowOf(QStringLiteral("locked")), true));
        QVERIFY(lockedInDocument(f.controller, f.shape1));
        QTRY_VERIFY(valueOf(f.model, QStringLiteral("locked")).toBool());

        // Une étape par édition (le glisser compris), aucune pour le verrou : chaque Annuler défait l'édition
        // d'avant. Annuler vide la sélection (L3-T0) : les valeurs se lisent dans le document.
        const auto translucencyOf = [&f] {
            const std::optional<core::PropertyValue> value =
                inDocument(f.controller, f.shape1, core::ObjectProperty::Translucency);
            return value ? std::get<double>(*value) : -1.0;
        };
        QCOMPARE(translucencyOf(), 128.0);
        f.controller.undo();
        QVERIFY(lockedInDocument(f.controller, f.shape1)); // le verrou reste
        QCOMPARE(translucencyOf(), translucencyBefore.toDouble());
        f.controller.undo();
        QVERIFY(!hiddenInDocument(f.controller, f.shape1));
        f.controller.undo();
        QVERIFY(objectOf(f.controller, f.shape1)->common.layer == core::Layer::Middle);
        f.select({f.shape1});
        QCOMPARE(valueOf(f.model, QStringLiteral("translucency")), translucencyBefore);
        QVERIFY(valueOf(f.model, QStringLiteral("visible")).toBool());
        QVERIFY(valueOf(f.model, QStringLiteral("locked")).toBool());
    }

    // Interdit de la section : un changement de sélection ne touche pas la page ; le modèle se remet à jour
    // (trois fois) sans aucun redessin du cache de la vue.
    void noCacheRefreshOnSelectionChange()
    {
        ViewFixture v;
        QVERIFY(v.mount());
        const int cache = v.view->cacheRefreshCount();
        const int refreshes = v.f.model.refreshCount();
        v.f.select({v.f.stroke});
        v.f.select({v.f.shape1, v.f.shape2});
        v.f.tools.clearSelection();
        QCoreApplication::processEvents();
        QCOMPARE(v.f.model.refreshCount(), refreshes + 3);
        QCOMPARE(v.f.model.title(), QStringLiteral("Page"));
        QTest::qWait(ui::kCacheFoldIdleMs + 300);
        QCOMPARE(v.view->cacheRefreshCount(), cache);
    }

    void refusedWhileLocked()
    {
        Fixture f;
        f.select({f.shape1});
        const double before = outlineWidth(f.controller, f.shape1);
        f.commands.setLocked(true);
        QVERIFY(!f.model.setValue(f.model.rowOf(QStringLiteral("outline-width")), 12));
        QCOMPARE(outlineWidth(f.controller, f.shape1), before);
    }

    void refusedWithoutCommands()
    {
        Fixture f;
        f.select({f.shape1});
        const double before = outlineWidth(f.controller, f.shape1);
        f.model.setCommands(nullptr);
        QVERIFY(!f.model.setValue(f.model.rowOf(QStringLiteral("outline-width")), 12));
        QCOMPARE(outlineWidth(f.controller, f.shape1), before);
    }

    void editableFollowsTheLock()
    {
        Fixture f;
        QVERIFY(f.model.editable());
        QSignalSpy changed(&f.model, &ui::PropertyModel::editableChanged);
        f.commands.setLocked(true);
        QVERIFY(!f.model.editable());
        QCOMPARE(changed.count(), 1);
        f.commands.setLocked(false);
        QVERIFY(f.model.editable());
        QCOMPARE(changed.count(), 2);
        f.model.setCommands(nullptr);
        QVERIFY(!f.model.editable());
        QCOMPARE(changed.count(), 3);
    }

    // Relecture M1 : un glissé fusionne ses pas jusqu'au relâcher (endEdit) ; le glissé suivant fait une
    // autre étape (D10). Sans endEdit, SetTranslucencyCommand::mergeWith fondrait les deux glissés en une
    // étape.
    void slideStepsMergeUntilEndEdit()
    {
        Fixture f;
        f.select({f.shape1});
        const double before = translucencyOf(f.controller, f.shape1);
        const int row = f.model.rowOf(QStringLiteral("translucency"));
        QVERIFY(f.model.setValue(row, 100)); // premier glissé : deux pas
        QVERIFY(f.model.setValue(row, 120));
        f.model.endEdit();
        QVERIFY(f.model.setValue(row, 200)); // second glissé
        f.model.endEdit();
        QCOMPARE(translucencyOf(f.controller, f.shape1), 200.0);
        f.controller.undo();
        QCOMPARE(translucencyOf(f.controller, f.shape1), 120.0);
        f.controller.undo();
        QCOMPARE(translucencyOf(f.controller, f.shape1), before);
    }

    // Relecture M1 : une modification ponctuelle clôt aussi l'étape restée ouverte avant elle (ici une
    // commande de même famille, mêmes objets, même propriété, exécutée ailleurs sans closeMerge).
    void pointEditClosesAStepLeftOpen()
    {
        Fixture f;
        f.select({f.shape1});
        const double before = outlineWidth(f.controller, f.shape1);
        QVERIFY(before != 9.0);
        f.controller.execute(std::make_unique<core::SetObjectPropertiesCommand>(
            0, std::vector<core::ObjectId>{f.shape1}, core::ObjectProperty::OutlineWidth, 9.0));
        QCoreApplication::processEvents();
        QVERIFY(f.model.setValue(f.model.rowOf(QStringLiteral("outline-width")), 12));
        f.controller.undo();
        QCOMPARE(outlineWidth(f.controller, f.shape1), 9.0);
        f.controller.undo();
        QCOMPARE(outlineWidth(f.controller, f.shape1), before);
    }

    // Relecture M2 : une valeur changée, lignes inchangées : dataChanged sur ValueRole, jamais modelReset
    // (un reset détruirait les délégués, la réglette tenue sous le doigt comprise). Le reset reste pour un
    // changement de structure.
    void valueEditKeepsTheRows()
    {
        Fixture f;
        f.select({f.shape1});
        QSignalSpy resets(&f.model, &QAbstractItemModel::modelReset);
        QSignalSpy changed(&f.model, &QAbstractItemModel::dataChanged);
        const int refreshes = f.model.refreshCount();
        const int row = f.model.rowOf(QStringLiteral("outline-width"));
        QVERIFY(f.model.setValue(row, 12));
        QCoreApplication::processEvents();
        QCOMPARE(resets.count(), 0);
        QCOMPARE(f.model.refreshCount(), refreshes + 1);
        QCOMPARE(f.model.rowOf(QStringLiteral("outline-width")), row);
        QCOMPARE(valueOf(f.model, QStringLiteral("outline-width")).toDouble(), 12.0);
        // dataChanged ne couvre que les lignes dont la valeur a changé, une par une (l'épaisseur change aussi
        // l'étendue, donc d'autres lignes de position) : jamais toutes les lignes d'un coup.
        QSet<int> emitted;
        for (const QList<QVariant>& arguments : std::as_const(changed)) {
            const int first = arguments.at(0).value<QModelIndex>().row();
            QCOMPARE(arguments.at(1).value<QModelIndex>().row(), first);
            QCOMPARE(arguments.at(2).value<QList<int>>(), QList<int>{ui::PropertyModel::ValueRole});
            emitted.insert(first);
        }
        QVERIFY(emitted.contains(row));
        QVERIFY(emitted.size() < f.model.rowCount());
        // Un pas de réglette, puis la même structure sur un autre objet : toujours aucun reset.
        QVERIFY(f.model.setValue(f.model.rowOf(QStringLiteral("translucency")), 50));
        QCoreApplication::processEvents();
        f.select({f.shape2});
        QCOMPARE(resets.count(), 0);
        QCOMPARE(f.model.refreshCount(), refreshes + 3);
        // Structure changée (trait : pas de remplissage) : un reset.
        f.select({f.stroke});
        QCOMPARE(resets.count(), 1);
        QCOMPARE(f.model.rowOf(QStringLiteral("fill-color")), -1);
        QCOMPARE(f.model.title(), QStringLiteral("Trait"));
    }

    // Relecture n1 : l'épaisseur publie un minimum de 0, que le cœur refuse pour un trait. setValue rend faux
    // quand rien n'est appliqué (aucune étape entrée dans la pile), et la valeur affichée reste celle du
    // document.
    void refusedByTheCoreIsNotApplied()
    {
        Fixture f;
        f.select({f.stroke});
        const double before = outlineWidth(f.controller, f.stroke);
        const int row = f.model.rowOf(QStringLiteral("outline-width"));
        QVERIFY(before != 7.0);
        QVERIFY(f.model.setValue(row, 7));
        QVERIFY(!f.model.setValue(row, 0)); // le cœur refuse 0 pour un trait
        QCoreApplication::processEvents();
        QCOMPARE(outlineWidth(f.controller, f.stroke), 7.0);
        QCOMPARE(valueOf(f.model, QStringLiteral("outline-width")).toDouble(), 7.0);
        QVERIFY(!f.model.setValue(row, 7)); // valeur inchangée : rien d'appliqué
        // Aucune étape entrée pour les deux refus : un Annuler défait la saisie de 7.
        f.controller.undo();
        QCOMPARE(outlineWidth(f.controller, f.stroke), before);
    }

    // Relecture n3 : le premier pas d'un glissé clôt l'étape restée ouverte avant lui.
    void firstSlideStepClosesAStepLeftOpen()
    {
        Fixture f;
        f.select({f.shape1});
        const double before = translucencyOf(f.controller, f.shape1);
        QVERIFY(before != 77.0);
        f.controller.execute(std::make_unique<core::SetTranslucencyCommand>(
            0, std::vector<core::ObjectId>{f.shape1}, static_cast<std::uint8_t>(77)));
        QCoreApplication::processEvents();
        const int row = f.model.rowOf(QStringLiteral("translucency"));
        QVERIFY(f.model.setValue(row, 150));
        QVERIFY(f.model.setValue(row, 160));
        f.model.endEdit();
        f.controller.undo();
        QCOMPARE(translucencyOf(f.controller, f.shape1), 77.0);
        f.controller.undo();
        QCOMPARE(translucencyOf(f.controller, f.shape1), before);
    }

    // Relecture n4 : voie sans reset (select), valeur affichée et cible de setValue.
    void selectionWithoutResetShowsAndTargetsTheNewObject()
    {
        Fixture f;
        f.select({f.shape1});
        const int row = f.model.rowOf(QStringLiteral("outline-width"));
        const double second = outlineWidth(f.controller, f.shape2);
        QVERIFY(f.model.setValue(row, 12));
        QCoreApplication::processEvents();
        QSignalSpy resets(&f.model, &QAbstractItemModel::modelReset);
        f.select({f.shape2});
        QCOMPARE(resets.count(), 0);
        QCOMPARE(valueOf(f.model, QStringLiteral("outline-width")).toDouble(), second);
        QVERIFY(second != 12.0);
        QVERIFY(f.model.setValue(f.model.rowOf(QStringLiteral("outline-width")), 31));
        QCOMPARE(outlineWidth(f.controller, f.shape2), 31.0);
        QCOMPARE(outlineWidth(f.controller, f.shape1), 12.0);
    }

    // Relecture n4 : une valeur illisible (NaN, texte) est refusée par un éditeur numérique.
    void unreadableNumbersAreRefused()
    {
        Fixture f;
        f.select({f.shape1});
        const double outline = outlineWidth(f.controller, f.shape1);
        const double translucency = translucencyOf(f.controller, f.shape1);
        const int width = f.model.rowOf(QStringLiteral("outline-width"));
        const int slider = f.model.rowOf(QStringLiteral("translucency"));
        QVERIFY(!f.model.setValue(width, std::numeric_limits<double>::quiet_NaN()));
        QVERIFY(!f.model.setValue(width, QStringLiteral("abc")));
        QVERIFY(!f.model.setValue(slider, std::numeric_limits<double>::quiet_NaN()));
        QVERIFY(!f.model.setValue(slider, QStringLiteral("abc")));
        QCOMPARE(outlineWidth(f.controller, f.shape1), outline);
        QCOMPARE(translucencyOf(f.controller, f.shape1), translucency);
    }

    // Relecture M3 : « number » et « slider » refusent une valeur hors de [MinRole, MaxRole] ; rien n'est
    // exécuté (une page de 1e6 px contournerait le plafond du contrôleur).
    void valuesOutOfRangeAreRefused()
    {
        Fixture f;
        const int width = f.model.rowOf(QStringLiteral("width"));
        QCOMPARE(roleOf(f.model, QStringLiteral("width"), ui::PropertyModel::MaxRole).toDouble(),
                 ui::DocumentController::kMaxPageSide);
        QVERIFY(!f.model.setValue(width, 1e6));
        QVERIFY(!f.model.setValue(width, 0));
        QCOMPARE(f.controller.document().page(0).width, 800.0);
        QVERIFY(f.model.setValue(width, ui::DocumentController::kMaxPageSide)); // borne comprise
        QCOMPARE(f.controller.document().page(0).width, ui::DocumentController::kMaxPageSide);
        f.select({f.shape1});
        const double outline = outlineWidth(f.controller, f.shape1);
        const double translucency = translucencyOf(f.controller, f.shape1);
        QVERIFY(!f.model.setValue(f.model.rowOf(QStringLiteral("outline-width")), -5));
        QVERIFY(!f.model.setValue(f.model.rowOf(QStringLiteral("translucency")), 300));
        QCOMPARE(outlineWidth(f.controller, f.shape1), outline);
        QCOMPARE(translucencyOf(f.controller, f.shape1), translucency);
    }

    // Relecture m2 : mots-clés en une chaîne, séparés par des espaces (L6-T13) ou des virgules.
    void keywordsSplitOnSpacesAndCommas()
    {
        Fixture f;
        f.select({f.shape1});
        QVERIFY(f.model.setValue(f.model.rowOf(QStringLiteral("keywords")),
                                 QStringLiteral("maths  géométrie, angles")));
        const std::vector<std::string> expected{"maths", "géométrie", "angles"};
        const std::optional<core::PropertyValue> stored =
            inDocument(f.controller, f.shape1, core::ObjectProperty::Keywords);
        QVERIFY(stored && std::holds_alternative<std::vector<std::string>>(*stored));
        QVERIFY(std::get<std::vector<std::string>>(*stored) == expected);
        QTRY_COMPARE(
            valueOf(f.model, QStringLiteral("keywords")).toStringList(),
            (QStringList{QStringLiteral("maths"), QStringLiteral("géométrie"), QStringLiteral("angles")}));
    }

    // Distingue (brief, point 1) : le verrou est posé par DocumentController::execute, sans
    // Commands::triggered. Le patron du plan (m_lockConnection sur « object.lock ») ne remettrait rien à
    // jour : refreshCount inchangé, « locked » faux.
    void lockFromElsewhereRefreshes()
    {
        Fixture f;
        f.select({f.stroke});
        QCOMPARE(valueOf(f.model, QStringLiteral("locked")), QVariant(false));
        QSignalSpy triggered(&f.commands, &ui::Commands::triggered);
        const int before = f.model.refreshCount();
        f.controller.execute(std::make_unique<core::SetObjectFlagCommand>(
            0, std::vector<core::ObjectId>{f.stroke}, core::ObjectFlag::Locked, true));
        QVERIFY(lockedInDocument(f.controller, f.stroke));
        QCoreApplication::processEvents();
        QCOMPARE(triggered.count(), 0);
        QCOMPARE(f.model.refreshCount(), before + 1);
        QVERIFY(valueOf(f.model, QStringLiteral("locked")).toBool());
        // Annuler n'a rien à défaire (hors pile) ; déverrouiller par le même chemin remet aussi à jour.
        f.controller.execute(std::make_unique<core::SetObjectFlagCommand>(
            0, std::vector<core::ObjectId>{f.stroke}, core::ObjectFlag::Locked, false));
        QCoreApplication::processEvents();
        QCOMPARE(f.model.refreshCount(), before + 2);
        QCOMPARE(valueOf(f.model, QStringLiteral("locked")), QVariant(false));
    }

    // Filtre de objectFlagsChanged : un objet hors de la sélection affichée ne remet rien à jour.
    void lockOfAnUnselectedObjectDoesNotRefresh()
    {
        Fixture f;
        f.select({f.stroke});
        const int before = f.model.refreshCount();
        f.controller.execute(std::make_unique<core::SetObjectFlagCommand>(
            0, std::vector<core::ObjectId>{f.shape2}, core::ObjectFlag::Locked, true));
        QCoreApplication::processEvents();
        QCOMPARE(f.model.refreshCount(), before);
    }
};

// L8-C2 : délégués de l'interactivité dans PropertyEditor, chargés par une vue comme PropertyBrowser. Ligne
// grisée (rôle « enabled » faux) : désactivée et visiblement atténuée. « object » : liste des objets de la
// page par leur nom, « Aucun » d'abord. « actions » : bouton qui ouvre l'onglet « actions » du navigateur
// (showNavigator de BrowserPanel, trouvé en remontant les parents). « point » : deux nombres x et y.
void TstUiPropertymodel::interactivityDelegatesLoad()
{
    Fixture f;
    f.select({f.stroke});
    QVERIFY(f.model.setValue(f.model.rowOf(QStringLiteral("name")), QStringLiteral("chemin")));
    QCoreApplication::processEvents();
    f.select({f.shape1});
    QCoreApplication::processEvents();

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nimport QtQuick.Window\nimport ActCraft.Ui\n"
                      "Window { id: win; width: 480; height: 800; visible: true\n"
                      "  property PropertyModel properties: null\n"
                      "  property string opened: \"\"\n"
                      "  Item { anchors.fill: parent\n"
                      "    function showNavigator(id) { win.opened = id }\n"
                      "    ListView { anchors.fill: parent; cacheBuffer: 100000; model: win.properties\n"
                      "      delegate: PropertyEditor { width: 480; properties: win.properties } } } }",
                      QUrl(QStringLiteral("qrc:/tst_ui_propertymodel/Delegates.qml")));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root != nullptr, qPrintable(component.errorString()));
    root->setProperty("properties", QVariant::fromValue(&f.model));
    auto* window = qobject_cast<QQuickWindow*>(root.get());
    QVERIFY(window != nullptr);
    const auto item = [window](const char* name) {
        return findItem(window->contentItem(), QString::fromLatin1(name));
    };
    QVERIFY(QTest::qWaitFor(
        [&item] { return item("prop-track") && item("prop-pivot-point") && item("prop-actions"); }, 5000));

    // Ligne grisée : désactivée et atténuée ; une ligne du lot 6 ne l'est jamais.
    QQuickItem* track = item("prop-track");
    QVERIFY(!track->isEnabled());
    QVERIFY(track->opacity() < 1);
    QVERIFY(item("prop-name")->isEnabled());
    QCOMPARE(item("prop-name")->opacity(), 1.0);

    // Objet choisi sur la page : « Aucun », puis les objets par leur nom (le chemin : traits et lignes).
    QQuickItem* objects = findItem(track, QStringLiteral("object-editor"));
    QVERIFY(objects != nullptr);
    QStringList texts;
    const int count = objects->property("count").toInt();
    for (int i = 0; i < count; ++i) {
        QString shown;
        QVERIFY(QMetaObject::invokeMethod(objects, "textAt", Q_RETURN_ARG(QString, shown), Q_ARG(int, i)));
        texts.append(shown);
    }
    QVERIFY2(!texts.isEmpty() && texts.first() == QStringLiteral("Aucun"), qPrintable(texts.join(u'|')));
    QVERIFY2(texts.contains(QStringLiteral("chemin")), qPrintable(texts.join(u'|')));

    // Actions : un bouton qui ouvre l'onglet « actions » du navigateur.
    QQuickItem* open = findItem(item("prop-actions"), QStringLiteral("actions-open"));
    QVERIFY(open != nullptr);
    QVERIFY(open->isEnabled());
    QVERIFY(QMetaObject::invokeMethod(open, "clicked"));
    QCOMPARE(root->property("opened").toString(), QStringLiteral("actions"));

    // Point : deux nombres, grisés tant que le pivot n'est pas « Point choisi ».
    QQuickItem* pivot = item("prop-pivot-point");
    QVERIFY(!pivot->isEnabled());
    QVERIFY(findItem(pivot, QStringLiteral("point-x")) != nullptr);
    QVERIFY(findItem(pivot, QStringLiteral("point-y")) != nullptr);
}

QTEST_MAIN(TstUiPropertymodel)
#include "tst_ui_propertymodel.moc"
