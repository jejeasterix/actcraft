// Diffusion protégée II (L8-T32, S18) : la porte du registre refuse les commandes des catégories interdites
// par le document ouvert, comme le verrou de classe (D6-7), sans jamais écrire l'état propre des commandes.
#include <actcraft/actions/commandregistry.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/core/pageedits.h>
#include <actcraft/ui/actionrunner.h>
#include <actcraft/ui/broadcastgate.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documenttabs.h>
#include <actcraft/ui/resourcelibrary.h>
#include <actcraft/ui/textediting.h>
#include <actcraft/ui/toolhost.h>

#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlExpression>
#include <QQuickItem>
#include <QQuickTextDocument>
#include <QQuickWindow>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace actcraft;

namespace {

// Déclarations réelles (actions/data/commands.json) : les drapeaux lockable et restrictable sont ceux livrés.
void loadDefaults(actions::CommandRegistry& registry)
{
    QString error;
    QVERIFY2(registry.loadDeclarations(actions::defaultCommandsJson(), error), qPrintable(error));
}

core::BroadcastRules copyAndPrint()
{
    core::BroadcastRules rules;
    rules.noCopy = true;
    rules.noPrint = true;
    return rules;
}

constexpr core::ObjectId kRound = 1;
const QPointF kOnRound(150, 150); // rond 1 : 100…200

core::Action action(const std::string& catalogId)
{
    core::Action a;
    a.catalogId = catalogId;
    a.parameters = "{}";
    return a;
}

// Fixture du scénario : `pages` pages, un rond porteur de `actions` sur la première ; règles `rules`.
core::Document documentWith(const core::BroadcastRules& rules, std::vector<core::Action> actions = {},
                            int pages = 1)
{
    core::Document document(1024, 768);
    core::Shape shape;
    shape.kind = core::ShapeKind::Rectangle;
    shape.bounds = core::RectF{100, 100, 100, 100};
    core::PageObject round{shape};
    round.common.id = kRound;
    round.common.actions = std::move(actions);
    document.page(0).objects.push_back(std::move(round));
    for (int i = 1; i < pages; ++i) {
        document.insertPage(document.pageCount(), core::Page{});
    }
    document.setBroadcast(rules);
    return document;
}

// Commandes et document réels, porte branchée comme dans Main.qml.
struct Mount
{
    QTemporaryDir dir;
    ui::Commands commands;
    ui::DocumentController document;
    ui::BroadcastGate gate;

    Mount()
    {
        gate.setCommands(&commands);
        gate.setDocument(&document);
    }
    void open(core::Document d)
    {
        document.replaceDocument(std::move(d),
                                 QUrl::fromLocalFile(dir.filePath(QStringLiteral("essai.actcraft"))),
                                 QStringLiteral("essai"));
    }
};

// Une page de plus : le document est modifié.
void modify(ui::DocumentController& document)
{
    document.execute(std::make_unique<core::InsertPagesCommand>(1, std::vector<core::Page>(1),
                                                                std::map<std::string, core::Resource>{}));
}

core::BroadcastRules noSaveRules()
{
    core::BroadcastRules rules;
    rules.noSave = true;
    return rules;
}

bool isSet(const QVariant& value)
{
    return value.isValid() && !value.isNull();
}

// Enfant QML d'un type donné (nom de classe « Type_QMLTYPE_n ») : la boîte « Enregistrer les modifications ?
// » de FileActions n'a pas de nom d'objet.
QObject* childOfType(QObject* parent, const char* type)
{
    for (QObject* child : parent->children()) {
        if (QByteArray(child->metaObject()->className()).startsWith(type)) {
            return child;
        }
    }
    return nullptr;
}

// FileActions et DocumentTabBar branchés comme dans Main.qml (porte en `broadcast`), dans une fenêtre : leurs
// boîtes sont des Popup.
const char* kFileShellQml = R"(
import QtQuick
import ActCraft.Ui
Item {
    id: shell
    width: 800; height: 600
    required property DocumentController doc
    required property DocumentTabs docTabs
    required property Commands cmds
    required property BroadcastGate gate
    property alias fileActions: fa
    property alias tabBar: bar
    FileActions { id: fa; document: shell.doc; tabs: shell.docTabs; broadcast: shell.gate }
    DocumentTabBar { id: bar; width: shell.width; tabs: shell.docTabs; commands: shell.cmds }
}
)";

struct FileShell
{
    ui::DocumentTabs tabs;
    QQmlEngine engine;
    QQuickWindow window;
    std::unique_ptr<QQuickItem> root;
    QObject* actions = nullptr;
    QObject* bar = nullptr;

    bool start(Mount& m)
    {
        tabs.setCommands(&m.commands);
        tabs.setDocument(&m.document);
        QQmlComponent component(&engine);
        component.setData(kFileShellQml, QUrl());
        root.reset(qobject_cast<QQuickItem*>(
            component.createWithInitialProperties({{QStringLiteral("doc"), QVariant::fromValue(&m.document)},
                                                   {QStringLiteral("docTabs"), QVariant::fromValue(&tabs)},
                                                   {QStringLiteral("cmds"), QVariant::fromValue(&m.commands)},
                                                   {QStringLiteral("gate"), QVariant::fromValue(&m.gate)}})));
        if (!root) {
            qWarning("%s", qPrintable(component.errorString()));
            return false;
        }
        root->setParentItem(window.contentItem());
        window.resize(800, 600);
        window.show();
        actions = root->property("fileActions").value<QObject*>();
        bar = root->property("tabBar").value<QObject*>();
        return actions != nullptr && bar != nullptr && QTest::qWaitForWindowExposed(&window);
    }
};

} // namespace

class TstUiBroadcast : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void restrictedCommandsGreyed();
    void enableCannotBypassTheRestriction();
    void triggerShowsTheRefusal();
    void lockAndRestrictionCombine();
    void resourceDragRefused();
    void pagesDraggedToTabRefused();
    void pageActionStillRuns();
    void objectCopyActionRefused();
    void rulesFollowTheOpenDocument();
    void propertiesFollowTheRules();
    void nativeTextCopyRefused();
    void notesCopyRefused();
    void pdfExportButtonGoesThroughTheRegistry();
    void fileActionsSaveRefused();
    void quitQuestionKeptWhenSaveRefused();
    void closingProtectedTabKeepsIt();

private:
    QTemporaryDir m_settings;
};

// Réglages d'essai (M7) : aucun réglage ni dossier du poste touché.
void TstUiBroadcast::initTestCase()
{
    QVERIFY(m_settings.isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
    QStandardPaths::setTestModeEnabled(true);
    QSettings probe;
    QCOMPARE(probe.format(), QSettings::IniFormat);
    QVERIFY(probe.fileName().startsWith(m_settings.path()));
}

// Copie et impression interdites : edit.copy et file.print grisées ; page.copy et page.cut aussi (H8-8), bien
// qu'elles ne soient pas déclarées restreignables ; la navigation reste ouverte.
void TstUiBroadcast::restrictedCommandsGreyed()
{
    actions::CommandRegistry registry;
    loadDefaults(registry);
    QVERIFY(registry.find(QStringLiteral("page.copy")) != nullptr);
    QVERIFY2(!registry.find(QStringLiteral("page.copy"))->restrictable,
             "H8-8 : page.copy n'est pas restreignable ; la diffusion la refuse quand même");
    QVERIFY(!registry.find(QStringLiteral("page.cut"))->restrictable);

    registry.setRestricted(ui::BroadcastGate::restrictedIds(registry, copyAndPrint()));

    for (const char* id : {"edit.copy", "edit.cut", "file.print", "page.copy", "page.cut"}) {
        QVERIFY2(registry.isRestricted(QString::fromLatin1(id)), id);
        QVERIFY2(!registry.isEnabled(QString::fromLatin1(id)), id);
        QVERIFY2(registry.isOwnEnabled(QString::fromLatin1(id)), id); // l'état propre n'est pas touché
    }
    for (const char* id : {"page.next", "file.save", "edit.paste"}) {
        QVERIFY2(!registry.isRestricted(QString::fromLatin1(id)), id);
        QVERIFY2(registry.isEnabled(QString::fromLatin1(id)), id);
    }
}

// Un contrôleur (sélection, mode conception) qui rouvre edit.copy par setEnabled ne lève pas la restriction ;
// à la levée des règles, l'état propre posé par ce contrôleur revient tel quel, ni forcé à vrai ni à faux.
void TstUiBroadcast::enableCannotBypassTheRestriction()
{
    actions::CommandRegistry registry;
    loadDefaults(registry);
    const QString copy = QStringLiteral("edit.copy");
    int calls = 0;
    registry.setHandler(copy, [&calls](const QVariant&) { ++calls; });

    registry.setEnabled(copy, false); // rien de sélectionné
    registry.setRestricted(ui::BroadcastGate::restrictedIds(registry, copyAndPrint()));
    registry.setEnabled(copy, true); // la sélection change : le contrôleur rouvre la commande
    QVERIFY(!registry.isEnabled(copy));
    QVERIFY(registry.isOwnEnabled(copy));
    QVERIFY(!registry.trigger(copy, {}, actions::currentPlatform()));
    QCOMPARE(calls, 0);

    registry.setEnabled(copy, false); // sélection vidée pendant la diffusion
    registry.setRestricted({});
    QVERIFY(!registry.isRestricted(copy));
    QVERIFY2(!registry.isEnabled(copy), "la levée rend l'état propre du contrôleur, pas un « tout ouvert »");
    registry.setEnabled(copy, true);
    QVERIFY(registry.isEnabled(copy));
    QVERIFY(registry.trigger(copy, {}, actions::currentPlatform()));
    QCOMPARE(calls, 1);
}

// Un essai refusé par la restriction : gestionnaire non appelé, message de sa catégorie émis par la porte.
void TstUiBroadcast::triggerShowsTheRefusal()
{
    Mount m;
    int copies = 0;
    int prints = 0;
    m.commands.setNativeHandler(QStringLiteral("edit.copy"), [&copies](const QVariant&) { ++copies; });
    m.commands.setNativeHandler(QStringLiteral("file.print"), [&prints](const QVariant&) { ++prints; });
    m.open(documentWith(copyAndPrint()));
    QSignalSpy refused(&m.gate, &ui::BroadcastGate::refused);

    QVERIFY(!m.commands.trigger(QStringLiteral("edit.copy")));
    QCOMPARE(copies, 0);
    QCOMPARE(refused.size(), 1);
    QCOMPARE(refused.at(0).at(0).toString(), QStringLiteral("edit.copy"));
    const QString copyMessage = m.gate.message();
    QVERIFY(!copyMessage.isEmpty());
    QCOMPARE(refused.at(0).at(1).toString(), copyMessage);

    QVERIFY(!m.commands.trigger(QStringLiteral("file.print")));
    QCOMPARE(prints, 0);
    QCOMPARE(refused.size(), 2);
    QVERIFY(!m.gate.message().isEmpty());
    QVERIFY(m.gate.message() != copyMessage); // un message par catégorie
}

// Verrou et restriction se combinent : lever le verrou ne lève pas la restriction, et inversement ; sous le
// verrou, l'essai est refusé par le verrou, sans message de diffusion.
void TstUiBroadcast::lockAndRestrictionCombine()
{
    Mount m;
    const QString copy = QStringLiteral("edit.copy");
    QVERIFY(m.commands.registry().find(copy)->lockable);
    m.commands.setNativeHandler(copy, [](const QVariant&) {});
    m.open(documentWith(copyAndPrint()));
    QSignalSpy refused(&m.gate, &ui::BroadcastGate::refused);

    m.commands.setLocked(true);
    QVERIFY(!m.commands.isEnabled(copy));
    QVERIFY(!m.commands.trigger(copy));
    QCOMPARE(refused.size(), 0);
    m.commands.setLocked(false);
    QVERIFY(!m.commands.isEnabled(copy)); // verrou levé, restriction tenue
    QVERIFY(m.commands.registry().isOwnEnabled(copy));

    m.open(documentWith({}));
    QVERIFY(m.commands.isEnabled(copy));
    m.commands.setLocked(true);
    QVERIFY(!m.commands.isEnabled(copy)); // restriction levée, verrou tenu
    m.commands.setLocked(false);
    QVERIFY(m.commands.isEnabled(copy));
}

// Ajouter un objet aux ressources (addSelection, gestionnaire de resources.add dans ResourceBrowser.qml) :
// resources.add n'est d'aucune catégorie restreinte, addSelection refuse donc elle-même, et le signale, dans
// un document qui interdit la copie ; accepté sans règles.
void TstUiBroadcast::resourceDragRefused()
{
    Mount m;
    ui::ToolHost tools;
    tools.setSettingsFile(m.dir.filePath(QStringLiteral("outils.ini")));
    tools.setDocumentController(&m.document);
    const QString mine = m.dir.filePath(QStringLiteral("mes-ressources"));
    const QString shared = m.dir.filePath(QStringLiteral("partagees"));
    QVERIFY(QDir().mkpath(mine));
    QVERIFY(QDir().mkpath(shared));
    ui::ResourceLibrary::setRootPathsForTests(mine, shared);
    ui::ResourceLibrary::setTrashForTests(false);
    ui::ResourceLibrary library;
    library.setDocument(&m.document);
    library.setCommands(&m.commands);
    library.setTools(&tools);
    library.setRoot(QStringLiteral("mine"));

    core::BroadcastRules noCopy;
    noCopy.noCopy = true;
    m.open(documentWith(noCopy));
    tools.user().setSelection({kRound});
    QSignalSpy refused(&m.gate, &ui::BroadcastGate::refused);
    QVERIFY(library.addSelection().isEmpty());
    QCOMPARE(refused.size(), 1);
    QVERIFY(!m.gate.message().isEmpty());
    QVERIFY(QDir(mine).entryList(QDir::Files).isEmpty());

    m.open(documentWith({}));
    tools.user().setSelection({kRound});
    QVERIFY(!library.addSelection().isEmpty()); // le montage ajoute bien sans règles
    QCOMPARE(refused.size(), 1);
}

// Vignettes glissées sur un autre onglet (DocumentTabBar.qml appelle copyPagesToTab hors registre) : les
// pages d'un document qui interdit la copie ne vont pas dans un document sans règles ; refus signalé
// (page.copy).
void TstUiBroadcast::pagesDraggedToTabRefused()
{
    Mount m;
    ui::DocumentTabs tabs;
    tabs.setCommands(&m.commands);
    tabs.setDocument(&m.document);
    core::BroadcastRules noCopy;
    noCopy.noCopy = true;
    m.open(documentWith(noCopy));
    tabs.newTab();
    QCOMPARE(tabs.rowCount(), 2);
    tabs.activate(0);
    QCOMPARE(tabs.currentIndex(), 0);
    QVERIFY(m.gate.copyRestricted());
    QSignalSpy refused(&m.gate, &ui::BroadcastGate::refused);
    QVERIFY(!tabs.copyPagesToTab({0}, 1));
    QCOMPARE(refused.size(), 1);
    QCOMPARE(refused.at(0).at(0).toString(), QStringLiteral("page.copy"));
    QVERIFY(!m.gate.message().isEmpty());
    QVERIFY(!tabs.isModifiedAt(1)); // aucune page déposée

    m.open(documentWith({}));
    QVERIFY(!m.gate.copyRestricted());
    QVERIFY(tabs.copyPagesToTab({0}, 1)); // le montage dépose bien sans règles
    QVERIFY(tabs.isModifiedAt(1));
    QCOMPARE(refused.size(), 1);
}

// Actions de page d'un objet : elles continuent dans un document protégé (V58).
void TstUiBroadcast::pageActionStillRuns()
{
    Mount m;
    m.commands.setNativeHandler(QStringLiteral("page.next"),
                                [&m](const QVariant&) { m.document.nextPage(); });
    ui::ActionRunner runner;
    runner.setCommands(&m.commands);
    runner.setDocument(&m.document);
    m.open(documentWith(copyAndPrint(), {action("page.next")}, 2));
    QVERIFY(m.gate.active());
    QCOMPARE(m.document.currentIndex(), 0);
    runner.setContactActive(true);
    runner.press(kOnRound, Qt::LeftButton);
    runner.release(kOnRound);
    runner.setContactActive(false);
    QTRY_COMPARE(m.document.currentIndex(), 1);
}

// Constat 3 : une action d'objet qui copie passe par le registre et y est refusée comme le menu, avec le
// message de la diffusion ; sous le verrou, refus du verrou seul (CommandRegistry::trigger).
void TstUiBroadcast::objectCopyActionRefused()
{
    Mount m;
    int copies = 0;
    m.commands.setNativeHandler(QStringLiteral("edit.copy"), [&copies](const QVariant&) { ++copies; });
    ui::ToolHost tools; // sans outils, runOnTarget refuserait « non prise en charge », avant la porte
    tools.setSettingsFile(m.dir.filePath(QStringLiteral("outils.ini")));
    tools.setDocumentController(&m.document);
    ui::ActionRunner runner;
    runner.setCommands(&m.commands);
    runner.setDocument(&m.document);
    runner.setTools(&tools);
    QSignalSpy refusedByRunner(&runner, &ui::ActionRunner::actionRefused);
    m.open(documentWith(copyAndPrint(), {action("object.copy")}));
    QSignalSpy refused(&m.gate, &ui::BroadcastGate::refused);
    runner.setContactActive(true);
    runner.press(kOnRound, Qt::LeftButton);
    runner.release(kOnRound);
    runner.setContactActive(false);
    QTRY_COMPARE(refusedByRunner.size(), 1);
    QCOMPARE(copies, 0);
    QCOMPARE(refused.size(), 1); // message de la diffusion (V58), pas seulement le refus du lanceur
    QCOMPARE(refused.at(0).at(0).toString(), QStringLiteral("edit.copy"));

    m.commands.setLocked(true); // verrou et restriction : refus du verrou seul, comme trigger
    runner.setContactActive(true);
    runner.press(kOnRound, Qt::LeftButton);
    runner.release(kOnRound);
    runner.setContactActive(false);
    QTRY_COMPARE(refusedByRunner.size(), 2);
    QCOMPARE(refused.size(), 1);
    QCOMPARE(copies, 0);

    m.commands.setLocked(false);
    m.open(documentWith({}, {action("object.copy")}));
    runner.setContactActive(true);
    runner.press(kOnRound, Qt::LeftButton);
    runner.release(kOnRound);
    runner.setContactActive(false);
    QTRY_COMPARE(copies, 1); // le montage copie bien sans règles : le refus venait de la diffusion
    QCOMPARE(refusedByRunner.size(), 2);
    QCOMPARE(refused.size(), 1);
}

// Ouvrir un document sans règles rouvre tout ; rouvrir un document protégé referme.
void TstUiBroadcast::rulesFollowTheOpenDocument()
{
    Mount m;
    const QStringList ids{QStringLiteral("edit.copy"), QStringLiteral("page.copy"),
                          QStringLiteral("file.print")};
    m.open(documentWith(copyAndPrint()));
    for (const QString& id : ids) {
        QVERIFY2(!m.commands.isEnabled(id), qPrintable(id));
    }
    m.open(documentWith({}));
    for (const QString& id : ids) {
        QVERIFY2(m.commands.isEnabled(id), qPrintable(id));
        QVERIFY2(!m.commands.isRestricted(id), qPrintable(id));
    }
    m.open(documentWith(copyAndPrint()));
    QVERIFY(!m.commands.isEnabled(QStringLiteral("edit.copy")));
    m.commands.reload(); // un profil activé relit le registre : la restriction est gardée
    QVERIFY(!m.commands.isEnabled(QStringLiteral("edit.copy")));
}

// Amendement 2, N1 : propriétés lues par le scénario, calculées des règles du document ouvert ; une édition
// du document ne les renotifie pas.
void TstUiBroadcast::propertiesFollowTheRules()
{
    Mount m;
    QSignalSpy changed(&m.gate, &ui::BroadcastGate::rulesChanged);
    m.open(documentWith(copyAndPrint()));
    QVERIFY(m.gate.active());
    QVERIFY(m.gate.copyRestricted());
    QVERIFY(m.gate.printRestricted());
    QVERIFY(!m.gate.saveRestricted());
    QVERIFY(!m.gate.cameraRestricted());
    QCOMPARE(m.gate.property("copyRestricted").toBool(), true);
    const qsizetype afterOpen = changed.size();
    QCOMPARE(afterOpen, 1);
    core::Stroke stroke;
    stroke.points = {{10, 10, 0.5f}, {60, 40, 0.5f}};
    m.document.execute(std::make_unique<core::AddObjectCommand>(0, core::PageObject{stroke}));
    QCOMPARE(changed.size(), afterOpen);

    m.open(documentWith({}));
    QVERIFY(!m.gate.active());
    QVERIFY(!m.gate.copyRestricted());
    QVERIFY(!m.gate.printRestricted());
    QVERIFY(!m.gate.saveRestricted());
    QVERIFY(!m.gate.cameraRestricted());
    QCOMPARE(changed.size(), afterOpen + 1);
}

// Constat 2 : Copier et Couper du clavier dans le champ de texte de la page (TextEdit de TextEditor.qml, hors
// commande) sont refusés et signalés dans un document qui interdit la copie ; sans règles, la copie native
// reste celle du champ.
void TstUiBroadcast::nativeTextCopyRefused()
{
    Mount m;
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nTextEdit { objectName: \"textEdit\"; text: \"secret\" }", QUrl());
    std::unique_ptr<QObject> created(component.create());
    QVERIFY2(created, qPrintable(component.errorString()));
    auto* field = qobject_cast<QQuickItem*>(created.get());
    QVERIFY(field != nullptr);
    auto* textDocument = field->property("textDocument").value<QQuickTextDocument*>();
    QVERIFY(textDocument != nullptr);
    ui::TextEditing editing;
    editing.setTextDocument(textDocument);
    m.gate.setTextEditing(&editing);
    QClipboard* clipboard = QGuiApplication::clipboard();

    m.open(documentWith(copyAndPrint()));
    QSignalSpy refused(&m.gate, &ui::BroadcastGate::refused);
    QVERIFY(QMetaObject::invokeMethod(field, "selectAll"));
    clipboard->setText(QStringLiteral("avant"));
    QKeyEvent copyPress(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier, QStringLiteral("c"));
    QVERIFY(copyPress.matches(QKeySequence::Copy));
    QCoreApplication::sendEvent(field, &copyPress);
    QCOMPARE(clipboard->text(), QStringLiteral("avant"));
    QCOMPARE(refused.size(), 1);
    QKeyEvent cutPress(QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier, QStringLiteral("x"));
    QCoreApplication::sendEvent(field, &cutPress);
    QCOMPARE(field->property("text").toString(), QStringLiteral("secret"));
    QCOMPARE(refused.size(), 2);

    m.open(documentWith({}));
    QKeyEvent copyAgain(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier, QStringLiteral("c"));
    QCoreApplication::sendEvent(field, &copyAgain);
    QCOMPARE(clipboard->text(), QStringLiteral("secret")); // le montage copie bien sans règles
    QCOMPARE(refused.size(), 2);
}

// H8-16 : les notes de la page font partie du contenu protégé. Copier et Couper du clavier y sont refusés, et
// leur menu contextuel n'a plus Copier ni Couper ; sans règles, le menu de Qt et la copie reviennent.
void TstUiBroadcast::notesCopyRefused()
{
    Mount m;
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick\nimport ActCraft.Ui\nNotesBrowser { width: 400; height: 300 }", QUrl());
    std::unique_ptr<QObject> notes(component.create());
    QVERIFY2(notes, qPrintable(component.errorString()));
    notes->setProperty("commands", QVariant::fromValue(&m.commands));
    notes->setProperty("documentController", QVariant::fromValue(&m.document));
    notes->setProperty("broadcast", QVariant::fromValue(&m.gate));
    auto* editor = notes->findChild<QQuickItem*>(QStringLiteral("notesEditor"));
    QVERIFY(editor != nullptr);
    QQmlExpression menuName(
        qmlContext(editor), editor,
        QStringLiteral("editor.ContextMenu.menu ? editor.ContextMenu.menu.objectName : \"aucun\""));
    QClipboard* clipboard = QGuiApplication::clipboard();

    m.open(documentWith(copyAndPrint()));
    QVERIFY(editor->setProperty("text", QStringLiteral("secret")));
    const int length = editor->property("length").toInt();
    QVERIFY(length > 0);
    QSignalSpy refused(&m.gate, &ui::BroadcastGate::refused);
    QVERIFY(QMetaObject::invokeMethod(editor, "selectAll"));
    clipboard->setText(QStringLiteral("avant"));
    QKeyEvent copyPress(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier, QStringLiteral("c"));
    QCoreApplication::sendEvent(editor, &copyPress);
    QCOMPARE(clipboard->text(), QStringLiteral("avant"));
    QCOMPARE(refused.size(), 1);
    QCOMPARE(refused.at(0).at(0).toString(), QStringLiteral("edit.copy"));
    QKeyEvent cutPress(QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier, QStringLiteral("x"));
    QCoreApplication::sendEvent(editor, &cutPress);
    QCOMPARE(editor->property("length").toInt(), length);
    QCOMPARE(refused.size(), 2);
    QCOMPARE(menuName.evaluate().toString(), QStringLiteral("notesRestrictedMenu"));

    m.open(documentWith({}));
    QVERIFY(menuName.evaluate().toString() != QStringLiteral("notesRestrictedMenu")); // menu de Qt rendu
    QVERIFY(editor->setProperty("text", QStringLiteral("secret")));
    QVERIFY(QMetaObject::invokeMethod(editor, "selectAll"));
    QKeyEvent copyAgain(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier, QStringLiteral("c"));
    QCoreApplication::sendEvent(editor, &copyAgain);
    QVERIFY(clipboard->text().contains(QStringLiteral("secret"))); // le montage copie bien sans règles
    QCOMPARE(refused.size(), 2);
}

// « Exporter en PDF… » de la mise en page passe par la commande export.pdf, donc par la porte : refusé et
// signalé dans un document qui interdit l'enregistrement, sans ouvrir le dialogue d'export en direct.
void TstUiBroadcast::pdfExportButtonGoesThroughTheRegistry()
{
    Mount m;
    int exports = 0;
    m.commands.setNativeHandler(QStringLiteral("export.pdf"), [&exports](const QVariant&) { ++exports; });
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
import QtQuick
import ActCraft.Ui
Item {
    property alias dialog: dlg
    property alias chooser: fx
    PrintController { id: ctl }
    QtObject { id: exchange }
    QtObject { id: fx; property int chosen: 0; function chooseExportPdf() { chosen = chosen + 1 } }
    PrintSetupDialog { id: dlg; printing: ctl; exchange: exchange; fileExchangeActions: fx }
}
)",
                      QUrl());
    std::unique_ptr<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    auto* dialog = root->property("dialog").value<QObject*>();
    auto* chooser = root->property("chooser").value<QObject*>();
    QVERIFY(dialog != nullptr && chooser != nullptr);
    dialog->setProperty("commands", QVariant::fromValue(&m.commands));
    auto* button = dialog->findChild<QQuickItem*>(QStringLiteral("exportButton"));
    QVERIFY(button != nullptr);
    QVERIFY(button->isEnabled());

    m.open(documentWith(noSaveRules()));
    QSignalSpy refused(&m.gate, &ui::BroadcastGate::refused);
    QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
    QCOMPARE(refused.size(), 1);
    QCOMPARE(refused.at(0).at(0).toString(), QStringLiteral("export.pdf"));
    QCOMPARE(exports, 0);
    QCOMPARE(chooser->property("chosen").toInt(), 0);

    m.open(documentWith({}));
    QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
    QCOMPARE(exports, 1); // le gestionnaire de la commande, qui ouvre lui-même le dialogue sans URL
    QCOMPARE(chooser->property("chosen").toInt(), 0);
    QCOMPARE(refused.size(), 1);
}

// FileActions, chemins hors commande (saveRefused, abandonRefusedSave) : Enregistrer et Enregistrer sous d'un
// document protégé sont refusés et signalés, rien n'est écrit, ce qui attendait l'enregistrement est oublié
// et une sortie en attente (quitResume) est gardée ; sans règles, l'enregistrement écrit le fichier.
void TstUiBroadcast::fileActionsSaveRefused()
{
    Mount m;
    FileShell s;
    QVERIFY(s.start(m));
    m.open(documentWith(noSaveRules()));
    modify(m.document);
    QVERIFY(m.document.isModified());
    const QString path = m.dir.filePath(QStringLiteral("essai.actcraft"));
    const QString other = m.dir.filePath(QStringLiteral("autre.actcraft"));
    QSignalSpy refused(&m.gate, &ui::BroadcastGate::refused);
    QSignalSpy abandoned(s.actions, SIGNAL(saveAbandoned()));

    s.actions->setProperty("afterSave", QStringLiteral("fermer l'onglet"));
    s.actions->setProperty("saveWaiting", true);
    s.actions->setProperty("quitResume", 3);
    QVERIFY(QMetaObject::invokeMethod(s.actions, "save"));
    QCOMPARE(refused.size(), 1);
    QCOMPARE(refused.at(0).at(0).toString(), QStringLiteral("file.save"));
    QCOMPARE(abandoned.size(), 1);
    QVERIFY(!isSet(s.actions->property("afterSave")));
    QVERIFY(!s.actions->property("saveWaiting").toBool());
    QCOMPARE(s.actions->property("quitResume").toInt(), 3);
    s.actions->setProperty("quitResume", -1);

    QVERIFY(QMetaObject::invokeMethod(s.actions, "saveAs",
                                      Q_ARG(QVariant, QVariant(QUrl::fromLocalFile(other)))));
    QCOMPARE(refused.size(), 2);
    QCOMPARE(refused.at(1).at(0).toString(), QStringLiteral("file.save-as"));
    QCOMPARE(abandoned.size(), 2);
    QTest::qWait(50);
    QVERIFY(!QFile::exists(path));
    QVERIFY(!QFile::exists(other));
    QVERIFY(m.document.isModified());

    m.open(documentWith({}));
    modify(m.document);
    QVERIFY(QMetaObject::invokeMethod(s.actions, "save"));
    QTRY_VERIFY(QFile::exists(path)); // le montage enregistre bien sans règles
    QCOMPARE(refused.size(), 2);
    QCOMPARE(abandoned.size(), 2);
}

// Quitter avec un document protégé modifié : « Enregistrer » est refusé et signalé, la question est reposée
// (réouverture), l'action en attente gardée ; rien n'est écrit, rien ne quitte.
void TstUiBroadcast::quitQuestionKeptWhenSaveRefused()
{
    Mount m;
    FileShell s;
    QVERIFY(s.start(m));
    m.open(documentWith(noSaveRules()));
    modify(m.document);
    QObject* question = childOfType(s.actions, "SaveChangesDialog");
    QVERIFY(question != nullptr);
    QSignalSpy opened(question, SIGNAL(opened()));
    QSignalSpy refused(&m.gate, &ui::BroadcastGate::refused);

    QVERIFY(QMetaObject::invokeMethod(s.actions, "quitFrom", Q_ARG(QVariant, QVariant(0))));
    QTRY_COMPARE(opened.size(), 1);
    QVERIFY(isSet(s.actions->property("pending")));
    QVERIFY(QMetaObject::invokeMethod(question, "save")); // « Enregistrer » : la boîte se ferme, puis émet
    QCOMPARE(refused.size(), 1);
    QCOMPARE(refused.at(0).at(0).toString(), QStringLiteral("file.save"));
    QTRY_COMPARE(opened.size(), 2); // la question est reposée
    QVERIFY(isSet(s.actions->property("pending")));
    QVERIFY(!isSet(s.actions->property("afterSave")));
    QVERIFY(!s.actions->property("quitting").toBool());
    QVERIFY(!QFile::exists(m.dir.filePath(QStringLiteral("essai.actcraft"))));
    QVERIFY(m.document.isModified());

    QVERIFY(QMetaObject::invokeMethod(question, "cancel")); // « Annuler » : la sortie est abandonnée
    QTRY_VERIFY(!isSet(s.actions->property("pending")));
    QVERIFY(!s.actions->property("quitting").toBool());
}

// Fermer l'onglet d'un document protégé modifié (DocumentTabBar, « Enregistrer ») : file.save est refusé par
// le registre et signalé ; l'onglet reste ouvert et modifié, et aucune fermeture n'attend un enregistrement
// qui ne viendra pas (closeAfterSave rendu faux).
void TstUiBroadcast::closingProtectedTabKeepsIt()
{
    Mount m;
    int saves = 0;
    m.commands.setNativeHandler(QStringLiteral("file.save"), [&saves](const QVariant&) { ++saves; });
    FileShell s;
    QVERIFY(s.start(m));
    m.open(documentWith(noSaveRules()));
    modify(m.document);
    s.tabs.newTab();
    s.tabs.activate(0);
    QCOMPARE(s.tabs.rowCount(), 2);
    QCOMPARE(s.tabs.currentIndex(), 0);
    QObject* question = s.bar->findChild<QObject*>(QStringLiteral("tabSaveChangesDialog"));
    QVERIFY(question != nullptr);
    QSignalSpy opened(question, SIGNAL(opened()));
    QSignalSpy refused(&m.gate, &ui::BroadcastGate::refused);

    QVERIFY(!s.tabs.requestClose(0));
    QTRY_COMPARE(opened.size(), 1);
    QVERIFY(QMetaObject::invokeMethod(question, "save"));
    QCOMPARE(refused.size(), 1);
    QCOMPARE(refused.at(0).at(0).toString(), QStringLiteral("file.save"));
    QCOMPARE(saves, 0);
    QCoreApplication::processEvents();
    QCOMPARE(s.tabs.rowCount(), 2);
    QVERIFY(s.tabs.isModifiedAt(0));
    QVERIFY(!s.bar->property("closeAfterSave").toBool());
}

QTEST_MAIN(TstUiBroadcast)
#include "tst_ui_broadcast.moc"
