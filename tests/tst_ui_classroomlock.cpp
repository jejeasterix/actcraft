#include <actcraft/actions/profilestore.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/objectflags.h>
#include <actcraft/core/shapegeometry.h>
#include <actcraft/ui/classroomlock.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documenttabs.h>
#include <actcraft/ui/notescontroller.h>
#include <actcraft/ui/objecttree.h>
#include <actcraft/ui/pagescontroller.h>
#include <actcraft/ui/pagethumbnails.h>
#include <actcraft/ui/preferences.h>
#include <actcraft/ui/profilecontroller.h>
#include <actcraft/ui/propertymodel.h>
#include <actcraft/ui/resourcelibrary.h>
#include <actcraft/ui/shortcutbar.h>
#include <actcraft/ui/spelling.h>
#include <actcraft/ui/toolhost.h>

#include <QDir>
#include <QFile>
#include <QImage>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThread>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include <algorithm>
#include <memory>
#include <vector>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

namespace actions = actcraft::actions;
namespace core = actcraft::core;
namespace ui = actcraft::ui;

namespace {

constexpr int kTestIterations = 1000;
const QString kPassword = QStringLiteral("essai-1234");

QString key(const char* name)
{
    return QString::fromLatin1(name);
}

// Booléen du premier signal reçu ; faux si rien n'est venu.
bool waitResult(QSignalSpy& spy)
{
    if (spy.isEmpty() && !spy.wait(10000)) {
        return false;
    }
    return spy.takeFirst().at(0).toBool();
}

// Commandes, réglages du poste (INI) et verrou ; open() rejoue une relance sur le même fichier.
struct LockFixture
{
    QTemporaryDir dir;
    std::unique_ptr<ui::Commands> commands;
    std::unique_ptr<ui::Preferences> prefs;
    std::unique_ptr<ui::ClassroomLock> lock;

    LockFixture() { open(); }
    QString ini() const { return dir.filePath(QStringLiteral("poste.ini")); }
    void close()
    {
        lock.reset();
        prefs.reset(); // écrit le fichier
        commands.reset();
    }
    void open()
    {
        close();
        commands = std::make_unique<ui::Commands>();
        commands->setProfileDir(dir.filePath(QStringLiteral("disposition")));
        prefs = std::make_unique<ui::Preferences>(ini(), nullptr);
        lock = std::make_unique<ui::ClassroomLock>(kTestIterations, nullptr);
        lock->setCommands(commands.get());
        lock->setPreferences(prefs.get());
    }
    bool create(const QString& password = kPassword)
    {
        QSignalSpy created(lock.get(), &ui::ClassroomLock::passwordCreated);
        return lock->createPassword(password, password) && waitResult(created);
    }
    bool createAndLock() { return create() && lock->lock() && lock->isLocked(); }
    bool unlock(const QString& password)
    {
        QSignalSpy done(lock.get(), &ui::ClassroomLock::unlocked);
        return lock->unlock(password) && waitResult(done);
    }
};

std::vector<core::ObjectId> idsOnPage(const ui::DocumentController& controller)
{
    std::vector<core::ObjectId> ids;
    for (const core::PageObject& object : controller.document().page(0).objects) {
        ids.push_back(object.common.id);
    }
    return ids;
}

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

core::PageObject textObject(const char* html)
{
    core::Text text;
    text.html = html;
    text.bounds = core::RectF{100, 100, 400, 80};
    return core::PageObject{text};
}

// Délégués d'une vue : enfants visuels, pas enfants QObject de la fenêtre.
QQuickItem* itemNamed(QQuickItem* root, const char* name)
{
    if (!root) {
        return nullptr;
    }
    if (root->objectName() == QLatin1String(name)) {
        return root;
    }
    for (QQuickItem* child : root->childItems()) {
        if (QQuickItem* found = itemNamed(child, name)) {
            return found;
        }
    }
    return nullptr;
}

int treeRowOf(const ui::ObjectTreeModel& tree, core::ObjectId id)
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

} // namespace

class TstUiClassroomlock : public QObject
{
    Q_OBJECT
private slots:
    void firstLockCreatesThePassword();
    void everyLockableGreyed();
    void wrongPasswordKeepsTheLock();
    void rightPasswordUnlocks();
    void lockSurvivesRestart();
    void passwordNeverStoredInClear();
    void saltDiffersPerPassword();
    void qmlCannotBypass();
    void disabledDuringLockStaysDisabled();
    void everyLot6ControllerRefuses();
    void hashingOffTheMainThread();
};

void TstUiClassroomlock::firstLockCreatesThePassword()
{
    LockFixture f;
    QVERIFY(!f.lock->hasPassword());
    QVERIFY(!f.lock->lock()); // aucun mot de passe par défaut
    QVERIFY(!f.lock->isLocked());
    QVERIFY(!f.commands->isLocked());
    QVERIFY(!f.lock->createPassword(kPassword, QStringLiteral("autre-1234")));
    QVERIFY(!f.lock->createPassword(QString(), QString()));
    QVERIFY(!f.lock->isBusy());
    QVERIFY(f.create());
    QVERIFY(f.lock->hasPassword());
    const QStringList keys = f.prefs->allKeys();
    QVERIFY(keys.contains(key(ui::ClassroomLock::kHashKey)));
    QVERIFY(keys.contains(key(ui::ClassroomLock::kSaltKey)));
    QVERIFY(keys.contains(key(ui::ClassroomLock::kIterationsKey)));
    QVERIFY(!f.lock->createPassword(QStringLiteral("autre-5678"), QStringLiteral("autre-5678"))); // déjà créé
    QVERIFY(f.lock->lock());
    QVERIFY(f.lock->isLocked());
    QVERIFY(f.commands->isLocked());
    QVERIFY(f.prefs->value(key(ui::ClassroomLock::kLockedKey)).toBool());
    // m3 : empreinte effacée pendant le verrou (fichier relu) : aucun mot de passe ne se crée, le verrou
    // tient.
    f.prefs->remove(key(ui::ClassroomLock::kHashKey));
    f.prefs->remove(key(ui::ClassroomLock::kSaltKey));
    f.prefs->remove(key(ui::ClassroomLock::kIterationsKey));
    QVERIFY(!f.lock->createPassword(QStringLiteral("autre-5678"), QStringLiteral("autre-5678")));
    QVERIFY(!f.lock->isBusy());
    QVERIFY(f.lock->isLocked());
    QVERIFY(f.commands->isLocked());
}

void TstUiClassroomlock::everyLockableGreyed()
{
    LockFixture f;
    QVERIFY(f.createAndLock());
    int lockable = 0;
    for (const QString& id : f.commands->registry().ids()) {
        if (!f.commands->isAvailable(id)) {
            continue;
        }
        const bool isLockable = f.commands->registry().find(id)->lockable;
        QVERIFY2(f.commands->isEnabled(id) == !isLockable, qPrintable(id));
        lockable += isLockable ? 1 : 0;
    }
    QVERIFY(lockable > 0);
    QCOMPARE(f.commands->lockedCount(), lockable);
    QCOMPARE(f.commands->property("lockedCount").toInt(), lockable);
    QVERIFY(f.commands->property("locked").toBool());
    QVERIFY(f.unlock(kPassword));
    QCOMPARE(f.commands->lockedCount(), 0);
    QVERIFY(!f.commands->property("locked").toBool());
    for (const QString& id : f.commands->registry().ids()) {
        if (f.commands->isAvailable(id)) {
            QVERIFY2(f.commands->isEnabled(id), qPrintable(id));
        }
    }
}

void TstUiClassroomlock::wrongPasswordKeepsTheLock()
{
    LockFixture f;
    QVERIFY(f.createAndLock());
    QSignalSpy done(f.lock.get(), &ui::ClassroomLock::unlocked);
    QVERIFY(f.lock->unlock(QStringLiteral("faux")));
    QVERIFY(f.lock->isBusy());
    QVERIFY(!f.lock->unlock(kPassword)); // un calcul à la fois
    QVERIFY(done.wait(10000));
    QCOMPARE(done.takeFirst().at(0).toBool(), false);
    QVERIFY(!f.lock->isBusy());
    QVERIFY(f.lock->isLocked());
    QVERIFY(f.commands->isLocked());
    QVERIFY(f.prefs->value(key(ui::ClassroomLock::kLockedKey)).toBool());
    QVERIFY(!f.unlock(QString()));
    QVERIFY(f.lock->isLocked());

    // M2 : `classroom.password` n'est pas verrouillable ; changer le mot de passe avec un ancien faux ne
    // change rien, et le nouveau ne lève pas le verrou.
    const QString hash = f.prefs->value(key(ui::ClassroomLock::kHashKey)).toString();
    const QString salt = f.prefs->value(key(ui::ClassroomLock::kSaltKey)).toString();
    const QString intruder = QStringLiteral("pirate-1");
    QSignalSpy replaced(f.lock.get(), &ui::ClassroomLock::passwordReplaced);
    if (f.lock->changePassword(QStringLiteral("faux"), intruder)) {
        QVERIFY(!waitResult(replaced));
    }
    QVERIFY(!f.lock->isBusy());
    QCOMPARE(f.prefs->value(key(ui::ClassroomLock::kHashKey)).toString(), hash);
    QCOMPARE(f.prefs->value(key(ui::ClassroomLock::kSaltKey)).toString(), salt);
    QVERIFY(!f.unlock(intruder));
    QVERIFY(f.lock->isLocked());
    QVERIFY(f.commands->isLocked());
    QVERIFY(f.unlock(kPassword)); // l'ancien reste le bon
}

void TstUiClassroomlock::rightPasswordUnlocks()
{
    LockFixture f;
    QVERIFY(f.createAndLock());
    QVERIFY(f.unlock(kPassword));
    QVERIFY(!f.lock->isLocked());
    QVERIFY(!f.commands->isLocked());
    QVERIFY(!f.prefs->value(key(ui::ClassroomLock::kLockedKey)).toBool());
    QCOMPARE(f.commands->lockedCount(), 0);
    QVERIFY(f.commands->isEnabled(QStringLiteral("edit.undo")));
    QVERIFY(!f.lock->unlock(kPassword)); // rien à lever
}

void TstUiClassroomlock::lockSurvivesRestart()
{
    LockFixture f;
    QVERIFY(f.createAndLock());
    f.open();
    QVERIFY(f.lock->hasPassword());
    QVERIFY(f.lock->isLocked());
    QVERIFY(f.commands->isLocked());
    QVERIFY(!f.commands->isEnabled(QStringLiteral("edit.undo")));
    QVERIFY(f.commands->lockedCount() > 0);
    {
        // Ordre inverse des liaisons (réglages, puis commandes) : même résultat.
        ui::Commands other;
        ui::ClassroomLock reversed(kTestIterations, nullptr);
        reversed.setPreferences(f.prefs.get());
        reversed.setCommands(&other);
        QVERIFY(reversed.isLocked());
        QVERIFY(other.isLocked());
    }
    QVERIFY(f.unlock(kPassword));
    f.open();
    QVERIFY(!f.lock->isLocked());
    QVERIFY(!f.commands->isLocked());
    // m4 : le verrou n'est reposé à la relance que si une empreinte existe (sinon, rien ne le lèverait).
    QVERIFY(f.lock->lock());
    f.prefs->remove(key(ui::ClassroomLock::kHashKey));
    f.open();
    QVERIFY(!f.lock->hasPassword());
    QVERIFY(!f.lock->isLocked());
    QVERIFY(!f.commands->isLocked());
}

void TstUiClassroomlock::passwordNeverStoredInClear()
{
    LockFixture f;
    QVERIFY(f.createAndLock());
    const QString second = QStringLiteral("deuxieme-5678");
    QSignalSpy replaced(f.lock.get(), &ui::ClassroomLock::passwordReplaced);
    QVERIFY(f.lock->changePassword(kPassword, second));
    QVERIFY(waitResult(replaced));
    QVERIFY(f.unlock(second));
    const QStringList secrets{kPassword, second, QString::fromLatin1(kPassword.toUtf8().toBase64()),
                              QString::fromLatin1(second.toUtf8().toBase64())};
    QStringList classroomKeys;
    for (const QString& name : f.prefs->allKeys()) {
        const QString value = f.prefs->value(name).toString();
        for (const QString& secret : secrets) {
            QVERIFY2(!value.contains(secret), qPrintable(name));
        }
        if (name.startsWith(QLatin1String("classroom/"))) {
            classroomKeys << name;
        }
    }
    classroomKeys.sort();
    QCOMPARE(classroomKeys,
             (QStringList{key(ui::ClassroomLock::kHashKey), key(ui::ClassroomLock::kIterationsKey),
                          key(ui::ClassroomLock::kLockedKey), key(ui::ClassroomLock::kSaltKey)}));
    f.close();
    QFile file(f.ini());
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QByteArray raw = file.readAll();
    QVERIFY(raw.contains("classroom"));
    for (const QString& secret : secrets) {
        QVERIFY(!raw.contains(secret.toUtf8()));
    }
}

void TstUiClassroomlock::saltDiffersPerPassword()
{
    LockFixture a;
    LockFixture b;
    QVERIFY(a.create());
    QVERIFY(b.create()); // même mot de passe
    const QString saltA = a.prefs->value(key(ui::ClassroomLock::kSaltKey)).toString();
    const QString saltB = b.prefs->value(key(ui::ClassroomLock::kSaltKey)).toString();
    const QString hashA = a.prefs->value(key(ui::ClassroomLock::kHashKey)).toString();
    QCOMPARE(QByteArray::fromBase64(saltA.toLatin1()).size(), ui::ClassroomLock::kSaltBytes);
    QCOMPARE(QByteArray::fromBase64(hashA.toLatin1()).size(), ui::ClassroomLock::kKeyBytes);
    QVERIFY(saltA != saltB);
    QVERIFY(hashA != b.prefs->value(key(ui::ClassroomLock::kHashKey)).toString());
    // Un nouveau mot de passe, même identique, tire un nouveau sel.
    QSignalSpy replaced(a.lock.get(), &ui::ClassroomLock::passwordReplaced);
    QVERIFY(a.lock->changePassword(kPassword, kPassword));
    QVERIFY(waitResult(replaced));
    QVERIFY(a.prefs->value(key(ui::ClassroomLock::kSaltKey)).toString() != saltA);
}

// Aucune voie QML ne lève le verrou ni ne rouvre une commande verrouillable : setLocked et isLocked restent
// en C++ (le registre décide), `locked` et `lockedCount` sont en lecture seule ; Ctrl+Alt+N de la barre
// d'onglets passe par le registre (file.new), pas par une garde QML.
void TstUiClassroomlock::qmlCannotBypass()
{
    LockFixture f;
    int calls = 0;
    f.commands->setNativeHandler(QStringLiteral("edit.undo"), [&calls](const QVariant&) { ++calls; });
    QVERIFY(f.createAndLock());
    QQmlEngine engine;
    QJSEngine::setObjectOwnership(f.commands.get(), QJSEngine::CppOwnership);
    QJSEngine::setObjectOwnership(f.lock.get(), QJSEngine::CppOwnership);
    QQmlComponent probeComponent(&engine);
    probeComponent.setData(
        "import QtQml\nimport ActCraft.Ui\n"
        "QtObject {\n"
        "  required property Commands commands\n"
        "  required property ClassroomLock classroom\n"
        "  function kinds() { return [typeof commands.setLocked, typeof commands.isLocked,\n"
        "    typeof classroom.setLockedState, typeof classroom.applyStoredState].join(',') }\n"
        "  function write() {\n"
        "    try { commands.locked = false } catch (e) {}\n"
        "    try { commands.lockedCount = 0 } catch (e) {}\n"
        "    try { classroom.locked = false } catch (e) {}\n"
        "    return commands.locked && classroom.locked }\n"
        "  function reopen(id) { commands.setEnabled(id, true); return commands.trigger(id) }\n"
        "}",
        QUrl(QStringLiteral("qrc:/tst_ui_classroomlock/Probe.qml")));
    std::unique_ptr<QObject> probe(probeComponent.createWithInitialProperties(
        {{QStringLiteral("commands"), QVariant::fromValue(f.commands.get())},
         {QStringLiteral("classroom"), QVariant::fromValue(f.lock.get())}}));
    QVERIFY2(probe, qPrintable(probeComponent.errorString()));
    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(probe.get(), "kinds", Q_RETURN_ARG(QVariant, result)));
    QCOMPARE(result.toString(), QStringLiteral("undefined,undefined,undefined,undefined"));
    QVERIFY(QMetaObject::invokeMethod(probe.get(), "write", Q_RETURN_ARG(QVariant, result)));
    QVERIFY(result.toBool());
    QVERIFY(f.commands->isLocked());
    QVERIFY(f.lock->isLocked());
    QVERIFY(QMetaObject::invokeMethod(probe.get(), "reopen", Q_RETURN_ARG(QVariant, result),
                                      Q_ARG(QVariant, QStringLiteral("edit.undo"))));
    QVERIFY(!result.toBool());
    QCOMPARE(calls, 0);

    // Ctrl+Alt+N et le bouton « + » de la barre d'onglets.
    ui::DocumentController document;
    document.newDocument(800, 600);
    ui::DocumentTabs tabs;
    QVERIFY(tabs.setProperty("commands", QVariant::fromValue(f.commands.get()))); // câblage de Main.qml
    tabs.setDocument(&document);
    QQmlComponent windowComponent(&engine);
    windowComponent.setData("import QtQuick\nimport QtQuick.Window\nimport ActCraft.Ui\n"
                            "Window {\n"
                            "  id: w; width: 800; height: 200; visible: true\n"
                            "  required property DocumentTabs tabs\n"
                            "  required property Commands commands\n"
                            "  DocumentTabBar { tabs: w.tabs; commands: w.commands\n"
                            "    anchors.left: parent.left; anchors.right: parent.right }\n"
                            "}",
                            QUrl(QStringLiteral("qrc:/tst_ui_classroomlock/Window.qml")));
    std::unique_ptr<QObject> root(windowComponent.createWithInitialProperties(
        {{QStringLiteral("tabs"), QVariant::fromValue(&tabs)},
         {QStringLiteral("commands"), QVariant::fromValue(f.commands.get())}}));
    QVERIFY2(root, qPrintable(windowComponent.errorString()));
    auto* window = qobject_cast<QQuickWindow*>(root.get());
    QVERIFY(window);
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    const auto pressNewTab = [window] {
        QTest::keyClick(window, Qt::Key_N, Qt::ControlModifier | Qt::AltModifier);
        QCoreApplication::processEvents();
    };
    const int before = tabs.rowCount();
    pressNewTab(); // verrou posé
    QCOMPARE(tabs.rowCount(), before);
    QVERIFY(f.unlock(kPassword));
    pressNewTab();
    QCOMPARE(tabs.rowCount(), before + 1);                     // le raccourci marche hors du verrou
    f.commands->setEnabled(QStringLiteral("file.new"), false); // le registre décide, pas un drapeau de verrou
    pressNewTab();
    QCOMPARE(tabs.rowCount(), before + 1);
    f.commands->setEnabled(QStringLiteral("file.new"), true);

    auto* add = window->findChild<QQuickItem*>(QStringLiteral("documentTabAdd"));
    QVERIFY(add);
    QTRY_VERIFY(add->isVisible() && add->height() > 0);
    const auto clickAdd = [window, add] {
        QTest::mouseClick(window, Qt::LeftButton, {},
                          add->mapToScene(QPointF(add->width() / 2, add->height() / 2)).toPoint());
        QCoreApplication::processEvents();
    };
    QVERIFY(f.lock->lock());
    clickAdd();
    pressNewTab();
    QCOMPARE(tabs.rowCount(), before + 1);
    QVERIFY(f.unlock(kPassword));
    clickAdd();
    QCOMPARE(tabs.rowCount(), before + 2);

    // M1, amendement 12 : sous verrou, fermer un onglet (« × », clic du milieu, « Ne pas enregistrer ») suit
    // file.close et déposer des pages suit sa commande. Le modèle refuse lui-même : un appel direct, sans la
    // barre (tout autre QML), ne passe pas non plus ; une garde QML seule le laisserait passer.
    QCOMPARE(tabs.rowCount(), 3);
    QCOMPARE(tabs.currentIndex(), 2);
    QVERIFY(addObject(document, shapeAt(100), core::Layer::Middle) != 0); // onglet affiché modifié
    QVERIFY(tabs.isModifiedAt(2));
    QVERIFY(!tabs.isModifiedAt(0));
    const auto item = [window](const char* name) { return itemNamed(window->contentItem(), name); };
    QTRY_VERIFY(item("documentTabClose0") && item("documentTabSelect1") && item("documentTabClose2"));
    auto* saveChanges = window->findChild<QObject*>(QStringLiteral("tabSaveChangesDialog"));
    QVERIFY(saveChanges);
    // Faux si l'élément manque (onglet fermé à tort) : un échec lisible plutôt qu'un plantage.
    const auto click = [window](QQuickItem* target, Qt::MouseButton button) {
        if (!target) {
            return false;
        }
        QTest::mouseClick(window, button, {},
                          target->mapToScene(QPointF(target->width() / 2, target->height() / 2)).toPoint());
        QCoreApplication::processEvents();
        return true;
    };
    QSignalSpy asked(&tabs, &ui::DocumentTabs::saveRequested);
    QVERIFY(f.lock->lock());
    QVERIFY(click(item("documentTabClose0"), Qt::LeftButton));           // « × » d'un onglet non modifié
    QVERIFY(click(item("documentTabSelect1"), Qt::MiddleButton));        // clic du milieu
    QVERIFY(click(item("documentTabClose2"), Qt::LeftButton));           // onglet modifié : aucune boîte
    QVERIFY(QMetaObject::invokeMethod(saveChanges, "discardRequested")); // « Ne pas enregistrer »
    QCOMPARE(tabs.rowCount(), 3);
    QCOMPARE(asked.count(), 0);
    QVERIFY(tabs.isModifiedAt(2));
    QVERIFY(QMetaObject::invokeMethod(&tabs, "newTab"));
    QVERIFY(QMetaObject::invokeMethod(
        &tabs, "openInNewTab",
        Q_ARG(QUrl, QUrl::fromLocalFile(f.dir.filePath(QStringLiteral("absent.actcraft"))))));
    bool closed = true;
    QVERIFY(QMetaObject::invokeMethod(&tabs, "requestClose", Q_RETURN_ARG(bool, closed), Q_ARG(int, 0)));
    QVERIFY(!closed);
    QVERIFY(QMetaObject::invokeMethod(&tabs, "closeWithoutSaving", Q_ARG(int, 2)));
    QVERIFY(QMetaObject::invokeMethod(&tabs, "copyPagesToTab", Q_ARG(QVariantList, QVariantList{0}),
                                      Q_ARG(int, 0)));
    QCOMPARE(tabs.rowCount(), 3);
    QCOMPARE(tabs.currentIndex(), 2);
    QVERIFY(tabs.isModifiedAt(2));
    QVERIFY(!tabs.isModifiedAt(0)); // aucune page déposée
    QCOMPARE(asked.count(), 0);
    QVERIFY(f.unlock(kPassword));
    QVERIFY(
        click(item("documentTabClose0"), Qt::LeftButton)); // hors du verrou, « × » ferme l'onglet non modifié
    QCOMPARE(tabs.rowCount(), 2);

    // Contre-relecture : un dépôt de pages sur un onglet qui refuse (verrou) ne retombe pas sur un
    // déplacement de la sélection dans le document affiché.
    ui::DocumentController shown;
    shown.newDocument(800, 600);
    ui::DocumentTabs shownTabs;
    QVERIFY(shownTabs.setProperty("commands", QVariant::fromValue(f.commands.get())));
    shownTabs.setDocument(&shown);
    shownTabs.newTab(); // l'onglet 0 devient inactif : il reçoit le dépôt
    ui::PagesController shownPages;
    QVERIFY(shownPages.setProperty("commands", QVariant::fromValue(f.commands.get())));
    shownPages.setDocumentController(&shown);
    shownPages.insertBlank(true);
    shownPages.insertBlank(true);
    QCOMPARE(shown.pageCount(), 3);
    QVERIFY(addObject(shown, shapeAt(100), core::Layer::Middle) != 0); // marque la première page
    ui::PageThumbnails thumbnails;
    QVERIFY(thumbnails.setProperty("documentController", QVariant::fromValue(&shown)));
    QVERIFY(thumbnails.setProperty("active", true));
    QQmlComponent navComponent(&engine);
    navComponent.setData(
        "import QtQuick\nimport QtQuick.Window\nimport ActCraft.Ui\n"
        "Window {\n"
        "  id: w; width: 600; height: 700; visible: true\n"
        "  required property DocumentTabs tabs\n"
        "  required property Commands commands\n"
        "  required property Pages pages\n"
        "  required property DocumentController documentController\n"
        "  required property PageThumbnails thumbnails\n"
        "  DocumentTabBar { tabs: w.tabs; commands: w.commands\n"
        "    anchors.left: parent.left; anchors.right: parent.right }\n"
        "  PageNavigator { y: 80; width: 220; height: 600; thumbnails: w.thumbnails\n"
        "    pages: w.pages; documentController: w.documentController; commands: w.commands }\n"
        "}",
        QUrl(QStringLiteral("qrc:/tst_ui_classroomlock/NavWindow.qml")));
    std::unique_ptr<QObject> navRoot(navComponent.createWithInitialProperties(
        {{QStringLiteral("tabs"), QVariant::fromValue(&shownTabs)},
         {QStringLiteral("commands"), QVariant::fromValue(f.commands.get())},
         {QStringLiteral("pages"), QVariant::fromValue(&shownPages)},
         {QStringLiteral("documentController"), QVariant::fromValue(&shown)},
         {QStringLiteral("thumbnails"), QVariant::fromValue(&thumbnails)}}));
    QVERIFY2(navRoot, qPrintable(navComponent.errorString()));
    auto* navWindow = qobject_cast<QQuickWindow*>(navRoot.get());
    QVERIFY(navWindow);
    navWindow->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(navWindow));
    const auto navItem = [navWindow](const char* name) { return itemNamed(navWindow->contentItem(), name); };
    QTRY_VERIFY(navItem("pageThumb2") && navItem("documentTab0") && navItem("pageThumb2")->height() > 0);
    const auto centre = [](QQuickItem* target) {
        return target->mapToScene(QPointF(target->width() / 2, target->height() / 2)).toPoint();
    };
    const QPoint from = centre(navItem("pageThumb2"));
    const QPoint to = centre(navItem("documentTab0"));
    QVERIFY(f.lock->lock());
    QTest::mousePress(navWindow, Qt::LeftButton, {}, from);
    for (int step = 1; step <= 10; ++step) {
        QTest::mouseMove(navWindow, from + (to - from) * step / 10);
        QCoreApplication::processEvents();
    }
    QTest::mouseRelease(navWindow, Qt::LeftButton, {}, to);
    QCoreApplication::processEvents();
    QCOMPARE(shown.pageCount(), 3);
    QCOMPARE(static_cast<int>(shown.document().page(0).objects.size()), 1); // ordre des pages inchangé
    QVERIFY(!shownTabs.isModifiedAt(0));                                    // aucune page reçue
    QVERIFY(f.unlock(kPassword));
}

// Amendement 1 : Commands::setEnabled compare à l'état propre (isOwnEnabled), pas à isEnabled, qui voit à
// travers le verrou. Sous l'ancienne comparaison, désactiver une commande verrouillable pendant le verrou
// n'écrit rien (isEnabled est déjà faux) et elle revient active au déverrouillage.
void TstUiClassroomlock::disabledDuringLockStaysDisabled()
{
    LockFixture f;
    QVERIFY(f.commands->registry().find(QStringLiteral("edit.undo"))->lockable);
    QVERIFY(f.createAndLock());
    QVERIFY(!f.commands->isEnabled(QStringLiteral("edit.undo")));
    f.commands->setEnabled(QStringLiteral("edit.undo"), false); // historique vidé pendant le verrou
    QVERIFY(f.unlock(kPassword));
    QVERIFY(!f.commands->isEnabled(QStringLiteral("edit.undo")));
    QVERIFY(!f.commands->registry().isOwnEnabled(QStringLiteral("edit.undo")));
    f.commands->setEnabled(QStringLiteral("edit.undo"), true);
    QVERIFY(f.commands->isEnabled(QStringLiteral("edit.undo")));
}

void TstUiClassroomlock::everyLot6ControllerRefuses()
{
    LockFixture f;
    actions::ProfileStore store(f.dir.filePath(QStringLiteral("profiles")),
                                QStringLiteral(":/actcraft/actions/profiles"),
                                ui::ProfileController::builtinOrder());
    ui::ProfileController profile(std::move(store), f.prefs.get(), f.commands.get());
    profile.start();
    ui::DocumentController document;
    document.newDocument(1024, 768);
    ui::ToolHost tools;
    tools.setSettingsFile(f.dir.filePath(QStringLiteral("outils.ini")));
    tools.setCommands(f.commands.get());
    tools.setDocumentController(&document);
    const core::ObjectId stroke = addObject(document, strokeObject(), core::Layer::Top);
    const core::ObjectId shape1 = addObject(document, shapeAt(100), core::Layer::Middle);
    const core::ObjectId shape2 = addObject(document, shapeAt(250), core::Layer::Middle);
    ui::PropertyModel properties;
    properties.setCommands(f.commands.get());
    properties.setDocument(&document);
    properties.setTools(&tools);
    ui::ObjectTreeModel tree;
    tree.setCommands(f.commands.get());
    tree.setDocument(&document);
    tree.setTools(&tools);
    ui::NotesController notes;
    notes.setDocument(&document);
    notes.setCommands(f.commands.get());
    ui::ShortcutBar bar;
    bar.setCommands(f.commands.get());
    bar.setProfile(&profile);
    bar.setProperty("document", QVariant::fromValue(&document));
    bar.setProperty("tools", QVariant::fromValue(&tools));
    const QString mine = f.dir.filePath(QStringLiteral("mes-ressources"));
    const QString shared = f.dir.filePath(QStringLiteral("partagees"));
    QVERIFY(QDir().mkpath(mine) && QDir().mkpath(shared));
    ui::ResourceLibrary::setRootPathsForTests(mine, shared);
    ui::ResourceLibrary::setTrashForTests(false);
    ui::ResourceLibrary library;
    library.setDocument(&document);
    library.setProfile(&profile);
    library.setCommands(f.commands.get());
    library.setTools(&tools);
    library.setRoot(QStringLiteral("mine"));
    const QString image = QDir(mine).filePath(QStringLiteral("photo.png"));
    QImage pixels(16, 16, QImage::Format_ARGB32);
    pixels.fill(Qt::red);
    QVERIFY(pixels.save(image, "PNG"));
    const QString button = bar.addObjects({stroke});
    QVERIFY(!button.isEmpty());
    tools.selectObjects({shape1});
    QCoreApplication::processEvents();
    const int row1 = treeRowOf(tree, shape1);
    const int row2 = treeRowOf(tree, shape2);
    QVERIFY(row1 >= 0 && row2 >= 0);
    const std::vector<core::ObjectId> order = idsOnPage(document);
    // Amendement 12 : les gestes du navigateur de pages (Suppr, Ctrl+X, Ctrl+V, Ctrl+D) appellent le
    // contrôleur de pages sans passer par le registre ; il refuse lui-même sous verrou.
    ui::DocumentController pagesDocument;
    pagesDocument.newDocument(1024, 768);
    ui::PagesController pages;
    QVERIFY(pages.setProperty("commands", QVariant::fromValue(f.commands.get()))); // câblage de Main.qml
    pages.setDocumentController(&pagesDocument);
    pages.insertBlank(true);
    pages.click(0, 0);
    pages.copySelection();
    QCOMPARE(pagesDocument.pageCount(), 2);
    QVERIFY(pages.canPaste());
    // Amendement 12 : la boîte d'orthographe, ouverte avant le verrou, appelle le contrôleur sans passer par le
    // registre ; il refuse lui-même « Ajouter au dictionnaire », « Remplacer » et « Remplacer tout ».
    ui::DocumentController spellDocument;
    spellDocument.newDocument(1024, 768);
    addObject(spellDocument, textObject("<p>Un motr et un motr</p>"), core::Layer::Middle);
    const std::string spellHtml = std::get<core::Text>(core::body(spellDocument.document().page(0).objects.at(0))).html;
    ui::Spelling spelling;
    QVERIFY(spelling.setProperty("commands", QVariant::fromValue(f.commands.get()))); // câblage de SpellingDialog.qml
    spelling.setDocument(&spellDocument);
    spelling.setDictionaryFolder(f.dir.filePath(QStringLiteral("dico")));
    const QString userWords = f.dir.filePath(QStringLiteral("mots.txt"));
    spelling.setUserDictionaryPath(userWords);
    spelling.start();
    QCOMPARE(spelling.word(), QStringLiteral("motr"));

    QVERIFY(f.createAndLock());
    const auto pageCountAfter = [&pages, &pagesDocument](const char* gesture) {
        return QMetaObject::invokeMethod(&pages, gesture) ? pagesDocument.pageCount() : -1;
    };
    QCOMPARE(pageCountAfter("deleteSelection"), 2);    // Suppr
    QCOMPARE(pageCountAfter("cutSelection"), 2);       // Ctrl+X
    QCOMPARE(pageCountAfter("paste"), 2);              // Ctrl+V
    QCOMPARE(pageCountAfter("duplicateSelection"), 2); // Ctrl+D
    QVERIFY(!properties.setValue(properties.rowOf(QStringLiteral("outline-width")), 12));
    QVERIFY(!tree.move(row2, row1));
    QVERIFY(!tree.setHidden(row1, true));
    QVERIFY(!tree.setLocked(row1, true));
    notes.setHtml(QStringLiteral("<p>x</p>"));
    QVERIFY(!bar.insert(button));
    QVERIFY(bar.addSelection().isEmpty());
    QVERIFY(!library.insert(image, QPointF(300, 300)));
    QVERIFY(library.addSelection().isEmpty());
    spelling.replace(QStringLiteral("mot"));
    spelling.replaceAll(QStringLiteral("mot"));
    spelling.addToDictionary();
    QCOMPARE(std::get<core::Text>(core::body(spellDocument.document().page(0).objects.at(0))).html, spellHtml);
    QFile written(userWords);
    QVERIFY(!written.open(QIODevice::ReadOnly) || !written.readAll().contains("motr"));
    QTest::qWait(300); // aucune insertion différée

    const core::Page& page = document.document().page(0);
    QCOMPARE(idsOnPage(document), order);
    QVERIFY(!core::flagOfAll(page, {shape1}, core::ObjectFlag::Hidden));
    QVERIFY(!core::flagOfAll(page, {shape1}, core::ObjectFlag::Locked));
    QCOMPARE(page.notes, std::string());
    QCOMPARE(static_cast<int>(profile.profile().shortcutObjects.size()), 1);
    QVERIFY(!QFile::exists(QDir(mine).filePath(QStringLiteral("Ressource1.actcraft"))));
    int steps = 0;
    while (document.canUndo()) {
        document.undo();
        ++steps;
    }
    QCOMPARE(steps, 3); // les trois ajouts, rien d'autre
    steps = 0;
    while (spellDocument.canUndo()) {
        spellDocument.undo();
        ++steps;
    }
    QCOMPARE(steps, 1); // l'ajout du texte, aucun remplacement
}

void TstUiClassroomlock::hashingOffTheMainThread()
{
    QCOMPARE(ui::ClassroomLock::kIterations, 200000);
    LockFixture f;
    QSignalSpy created(f.lock.get(), &ui::ClassroomLock::passwordCreated);
    QVERIFY(f.lock->createPassword(kPassword, kPassword));
    QVERIFY(f.lock->isBusy()); // rendu avant le calcul
    QVERIFY(!f.lock->hasPassword());
    QVERIFY(waitResult(created));
    QVERIFY(f.lock->lastHashThread() != nullptr);
    QVERIFY(f.lock->lastHashThread() != QThread::currentThreadId());
    QVERIFY(f.lock->lock());
    QSignalSpy done(f.lock.get(), &ui::ClassroomLock::unlocked);
    QVERIFY(f.lock->unlock(kPassword));
    QVERIFY(f.lock->isBusy());
    QVERIFY(f.lock->isLocked());
    QVERIFY(waitResult(done));
    QVERIFY(f.lock->lastHashThread() != QThread::currentThreadId());
}

QTEST_MAIN(TstUiClassroomlock)
#include "tst_ui_classroomlock.moc"
