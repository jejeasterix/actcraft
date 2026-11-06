// L8-T11 : modèle de l'onglet « Sélection » du navigateur d'actions (F-850, F-851, D8-10).
//
// Invariants gardés :
// - Aucun pixel (interdit de la tâche, L0-S3, A20) : une action ne change aucun pixel de la page. La commande
//   rend un PageSettingsChanged par objet modifié, comme SetObjectPropertiesCommand pour le nom et les
//   mots-clés (propertyedits.h, D6-6), que applyToCache ignore. Un ObjectChanged (patron de
//   ReplaceObjectsCommand, de TransformObjectsCommand) ferait redessiner le cache, et toute la page avec une
//   zone vide (undo.h : « vide = toute la page »). applyToManyIsOneStep et removeExistingIsOneStep le
//   distinguent sur la vraie zone de page : cacheRefreshCount() ne bouge pas.
// - Une étape : appliquer à N objets, ou retirer leurs actions, s'annule d'un seul undo.
// - Fermé par défaut : sans Commands, ou sous le verrou, rien ne change et refused est émis.
// - Remise à jour regroupée, reportée pendant un contact (D6-5).
#include <actcraft/actions/objectactions.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/ui/actionbrowser.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QElapsedTimer>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include <algorithm>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

namespace actions = actcraft::actions;
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

core::ObjectId addShape(ui::DocumentController& controller, double x)
{
    core::PageObject object =
        core::makeShapeObject("rectangle", core::RectF{x, 100, 80, 60}, core::ShapeStyle{});
    object.common.layer = core::Layer::Middle;
    const std::vector<core::ObjectId> before = idsOnPage(controller);
    controller.execute(std::make_unique<core::AddObjectCommand>(0, std::move(object)));
    for (const core::ObjectId id : idsOnPage(controller)) {
        if (std::find(before.begin(), before.end(), id) == before.end()) {
            return id;
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

template <typename T> QString text(const T& value)
{
    if constexpr (std::is_convertible_v<T, std::string>) {
        return QString::fromStdString(std::string(value));
    } else {
        return QString(value);
    }
}

// Actions portées par l'objet, par identifiant de catalogue.
QStringList actionsOf(const ui::DocumentController& controller, core::ObjectId id)
{
    QStringList ids;
    if (const core::PageObject* object = objectOf(controller, id)) {
        for (const core::Action& action : object->common.actions) {
            ids.append(text(action.catalogId));
        }
    }
    return ids;
}

core::ObjectId targetOf(const ui::DocumentController& controller, core::ObjectId id)
{
    const core::PageObject* object = objectOf(controller, id);
    return object && object->common.actions.size() == 1 ? object->common.actions.front().target : 0;
}

const actions::ObjectActionCatalog& catalog()
{
    static const actions::ObjectActionCatalog loaded = [] {
        actions::ObjectActionCatalog c;
        QString error;
        if (!c.load(actions::defaultObjectActionsJson(), error)) {
            qWarning() << error;
        }
        return c;
    }();
    return loaded;
}

QString firstTargetAction()
{
    for (const actions::ActionCategory category :
         {actions::ActionCategory::Command, actions::ActionCategory::Page, actions::ActionCategory::Object,
          actions::ActionCategory::Document}) {
        for (const actions::ObjectActionInfo* info : catalog().inCategory(category)) {
            if (info->target) {
                return info->id;
            }
        }
    }
    return {};
}

QString idAt(const ui::ActionBrowserModel& model, int row)
{
    return model.data(model.index(row), ui::ActionBrowserModel::IdRole).toString();
}

// Trois formes 1, 2, 3 sur la page 1 ; Commands, ToolHost et le modèle réels (réglages d'essai, M7).
struct Fixture
{
    QTemporaryDir dir;
    ui::DocumentController controller;
    ui::Commands commands;
    ui::ToolHost tools;
    ui::ActionBrowserModel model;
    core::ObjectId shape1 = 0;
    core::ObjectId shape2 = 0;
    core::ObjectId shape3 = 0;

    explicit Fixture(bool withCommands = true)
    {
        controller.newDocument(800, 600);
        commands.setProfileDir(dir.filePath(QStringLiteral("disposition")));
        tools.setSettingsFile(dir.filePath(QStringLiteral("outils.ini")));
        tools.setCommands(&commands);
        tools.setDocumentController(&controller);
        if (withCommands) {
            model.setCommands(&commands);
        }
        model.setDocument(&controller);
        model.setToolHost(&tools);
        shape1 = addShape(controller, 100);
        shape2 = addShape(controller, 250);
        shape3 = addShape(controller, 400);
        QCoreApplication::processEvents(); // remise à jour regroupée faite
    }

    void select(const std::vector<core::ObjectId>& ids)
    {
        tools.selectObjects(ids); // A12 : C++ seul, active l'outil Sélection (clic simulé)
        QCoreApplication::processEvents();
    }
};

// Le montage ci-dessus, plus la zone de page réelle (vue et cache) hors écran, comme tst_ui_propertymodel.
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
                          QUrl(QStringLiteral("qrc:/tst_ui_actionbrowser/Window.qml")));
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

    // Aucun rendu du cache depuis `before` : le délai de regroupement passé, le compte n'a pas bougé.
    bool cacheUntouchedSince(int before)
    {
        QTest::qWait(ui::kCacheFoldIdleMs + 300);
        return view->cacheRefreshCount() == before;
    }
};

} // namespace

class TstUiActionbrowser : public QObject
{
    Q_OBJECT

private slots:
    void greyedWithoutSelection()
    {
        Fixture f;
        QVERIFY(!f.model.enabled());
        f.model.setCurrentId(QStringLiteral("page.next"));
        QVERIFY(!f.model.apply());
        QVERIFY(!f.model.removeExisting());
        for (const core::ObjectId id : {f.shape1, f.shape2, f.shape3}) {
            QVERIFY(actionsOf(f.controller, id).isEmpty());
        }
        QSignalSpy enabled(&f.model, &ui::ActionBrowserModel::enabledChanged);
        f.select({f.shape1});
        QVERIFY(f.model.enabled());
        QVERIFY(enabled.count() >= 1);
        f.tools.clearSelection();
        QCoreApplication::processEvents();
        QVERIFY(!f.model.enabled());
    }

    void categoriesInOrder()
    {
        Fixture f;
        QCOMPARE(f.model.categoryLabels(),
                 (QStringList{QStringLiteral("Toutes"), QStringLiteral("Commandes"), QStringLiteral("Pages"),
                              QStringLiteral("Objets"), QStringLiteral("Document")}));
        QCOMPARE(f.model.category(), 0);
        const int all = f.model.rowCount();
        QVERIFY(all > 0);
        QCOMPARE(qsizetype(all), catalog().size());
        int sum = 0;
        const actions::ActionCategory order[] = {
            actions::ActionCategory::Command, actions::ActionCategory::Page, actions::ActionCategory::Object,
            actions::ActionCategory::Document};
        for (int category = 1; category <= 4; ++category) {
            f.model.setCategory(category);
            QCOMPARE(f.model.category(), category);
            const QList<const actions::ObjectActionInfo*> expected =
                catalog().inCategory(order[category - 1]);
            QCOMPARE(f.model.rowCount(), int(expected.size()));
            for (int row = 0; row < f.model.rowCount(); ++row) {
                QCOMPARE(idAt(f.model, row), expected[row]->id);
                QCOMPARE(f.model.data(f.model.index(row), ui::ActionBrowserModel::LabelRole).toString(),
                         expected[row]->label);
                QCOMPARE(f.model.data(f.model.index(row), ui::ActionBrowserModel::IconRole).toString(),
                         expected[row]->icon);
            }
            sum += f.model.rowCount();
        }
        QCOMPARE(sum, all);
        // « Toutes » : les catégories dans l'ordre, chacune dans l'ordre du catalogue.
        f.model.setCategory(0);
        int row = 0;
        for (const actions::ActionCategory category : order) {
            for (const actions::ObjectActionInfo* info : catalog().inCategory(category)) {
                QCOMPARE(idAt(f.model, row++), info->id);
            }
        }
        const QHash<int, QByteArray> roles = f.model.roleNames();
        QCOMPARE(roles.value(ui::ActionBrowserModel::IdRole), QByteArray("id"));
        QCOMPARE(roles.value(ui::ActionBrowserModel::LabelRole), QByteArray("label"));
        QCOMPARE(roles.value(ui::ActionBrowserModel::IconRole), QByteArray("icon"));
    }

    void applyToManyIsOneStep()
    {
        ViewFixture v;
        QVERIFY(v.mount());
        Fixture& f = v.f;
        f.select({f.shape1, f.shape2});
        f.model.setCurrentId(QStringLiteral("page.next"));
        QVERIFY(v.waitCacheStable());
        const int refreshes = v.view->cacheRefreshCount();
        QSignalSpy refused(&f.model, &ui::ActionBrowserModel::refused);

        QVERIFY(f.model.apply());
        QCOMPARE(refused.count(), 0);
        QCOMPARE(actionsOf(f.controller, f.shape1), QStringList{QStringLiteral("page.next")});
        QCOMPARE(actionsOf(f.controller, f.shape2), QStringList{QStringLiteral("page.next")});
        QVERIFY(actionsOf(f.controller, f.shape3).isEmpty());
        // L'invariant de la tâche : aucune demande de redessin du cache (un ObjectChanged en ferait une).
        QVERIFY2(v.cacheUntouchedSince(refreshes), "appliquer une action a redessiné le cache");

        f.controller.undo(); // une seule étape pour les deux objets
        QVERIFY(actionsOf(f.controller, f.shape1).isEmpty());
        QVERIFY(actionsOf(f.controller, f.shape2).isEmpty());
        QVERIFY2(v.cacheUntouchedSince(refreshes), "annuler une action a redessiné le cache");
    }

    void removeExistingIsOneStep()
    {
        ViewFixture v;
        QVERIFY(v.mount());
        Fixture& f = v.f;
        f.model.setCurrentId(QStringLiteral("page.next"));
        f.select({f.shape3});
        QVERIFY(f.model.apply());
        f.select({f.shape1, f.shape2});
        QVERIFY(f.model.apply());
        QVERIFY(v.waitCacheStable());
        const int refreshes = v.view->cacheRefreshCount();

        QVERIFY(f.model.removeExisting());
        QVERIFY(actionsOf(f.controller, f.shape1).isEmpty());
        QVERIFY(actionsOf(f.controller, f.shape2).isEmpty());
        QCOMPARE(actionsOf(f.controller, f.shape3),
                 QStringList{QStringLiteral("page.next")}); // hors sélection
        QVERIFY2(v.cacheUntouchedSince(refreshes), "retirer les actions a redessiné le cache");

        f.controller.undo(); // une seule étape pour les deux objets
        QCOMPARE(actionsOf(f.controller, f.shape1), QStringList{QStringLiteral("page.next")});
        QCOMPARE(actionsOf(f.controller, f.shape2), QStringList{QStringLiteral("page.next")});
        QCOMPARE(actionsOf(f.controller, f.shape3), QStringList{QStringLiteral("page.next")});
    }

    void propertiesFollowTheAction()
    {
        Fixture f;
        const actions::ObjectActionInfo* go = catalog().find(QStringLiteral("page.goto"));
        QVERIFY(go);
        QVERIFY(!go->parameters.isEmpty());
        QSignalSpy changed(&f.model, &ui::ActionBrowserModel::parametersChanged);
        f.model.setCurrentId(QStringLiteral("page.goto"));
        QCOMPARE(f.model.currentId(), QStringLiteral("page.goto"));
        QVERIFY(changed.count() >= 1);
        QCOMPARE(f.model.parameters().size(), go->parameters.size());
        for (const actions::ActionParameter& parameter : go->parameters) {
            QVERIFY2(f.model.parameters().contains(parameter.name), qPrintable(parameter.name));
        }
        QVERIFY(f.model.parameters().contains(QStringLiteral("page"))); // plan : page.goto porte « page »
        QStringList rows;
        for (const QVariant& row : f.model.propertyRows()) {
            rows.append(row.toMap().value(QStringLiteral("name")).toString());
        }
        QCOMPARE(rows.size(), go->parameters.size()); // page.goto ne vise aucun objet
        QVERIFY(!rows.contains(QStringLiteral("target")));

        const QString targeted = firstTargetAction();
        QVERIFY(!targeted.isEmpty());
        f.model.setCurrentId(targeted);
        const QVariantList targetedRows = f.model.propertyRows();
        QCOMPARE(targetedRows.size(), catalog().find(targeted)->parameters.size() + 1);
        QCOMPARE(targetedRows.last().toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("target"));
        QCOMPARE(targetedRows.last().toMap().value(QStringLiteral("label")).toString(),
                 QStringLiteral("Objet visé"));

        f.model.setCurrentId(QStringLiteral("page.next"));
        QVERIFY(f.model.parameters().isEmpty());
    }

    void targetPickedFromThePage()
    {
        Fixture f;
        const QString targeted = firstTargetAction();
        QVERIFY(!targeted.isEmpty());
        f.select({f.shape1});
        f.model.setCurrentId(targeted);
        QSignalSpy targetChanged(&f.model, &ui::ActionBrowserModel::targetChanged);
        f.model.pickTarget();
        QCOMPARE(f.model.target(), quint64(0));
        f.select({f.shape3}); // le prochain clic sur la page, par ToolHost
        QCOMPARE(f.model.target(), quint64(f.shape3));
        QCOMPARE(targetChanged.count(), 1);
        // Le choix de la cible ne change pas les objets auxquels on applique : la sélection d'avant revient.
        QVERIFY(f.model.apply());
        QCOMPARE(actionsOf(f.controller, f.shape1), QStringList{targeted});
        QCOMPARE(targetOf(f.controller, f.shape1), f.shape3);
        QVERIFY(actionsOf(f.controller, f.shape3).isEmpty());
        // Hors choix, un clic ne change plus la cible.
        f.select({f.shape2});
        QCOMPARE(f.model.target(), quint64(f.shape3));
    }

    void refreshDeferredDuringContact()
    {
        Fixture f;
        QSignalSpy resets(&f.model, &QAbstractItemModel::modelReset);
        f.model.setContactActive(true);
        f.select({f.shape1});
        f.select({f.shape1, f.shape2});
        QCOMPARE(resets.count(), 0); // rien pendant le contact
        f.model.setContactActive(false);
        QCoreApplication::processEvents();
        QCOMPARE(resets.count(), 1); // une seule remise à jour à la retombée
        QVERIFY(f.model.enabled());

        // Hors contact : deux changements dans le même tour, une seule remise à jour.
        resets.clear();
        f.tools.selectObjects({f.shape2});
        f.tools.selectObjects({f.shape3});
        QCOMPARE(resets.count(), 0);
        QCoreApplication::processEvents();
        QCOMPARE(resets.count(), 1);
    }

    void refusedWhileLocked()
    {
        Fixture f;
        f.select({f.shape1});
        f.model.setCurrentId(QStringLiteral("page.next"));
        QVERIFY(f.model.enabled());
        f.commands.setLocked(true);
        QCoreApplication::processEvents();
        QVERIFY(!f.model.enabled());
        QSignalSpy refused(&f.model, &ui::ActionBrowserModel::refused);
        QVERIFY(!f.model.apply());
        QVERIFY(!f.model.removeExisting());
        QCOMPARE(refused.count(), 2);
        QVERIFY(actionsOf(f.controller, f.shape1).isEmpty());
        f.commands.setLocked(false);
        QCoreApplication::processEvents();
        QVERIFY(f.model.enabled());
    }

    void refusedWithoutCommands()
    {
        Fixture f(false); // commands nul : fermé par défaut
        f.select({f.shape1});
        f.model.setCurrentId(QStringLiteral("page.next"));
        QVERIFY(!f.model.enabled());
        QSignalSpy refused(&f.model, &ui::ActionBrowserModel::refused);
        QVERIFY(!f.model.apply());
        QVERIFY(!f.model.removeExisting());
        QCOMPARE(refused.count(), 2);
        QVERIFY(actionsOf(f.controller, f.shape1).isEmpty());
    }
};

QTEST_MAIN(TstUiActionbrowser)
#include "tst_ui_actionbrowser.moc"
