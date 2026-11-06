// Diffusion protégée II (L8-T32, S18) : la porte du registre refuse les commandes des catégories interdites
// par le document ouvert, comme le verrou de classe (D6-7), sans jamais écrire l'état propre des commandes.
#include <actcraft/actions/commandregistry.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
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
#include <QGuiApplication>
#include <QKeyEvent>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickTextDocument>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

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

// Glisser un objet vers les ressources (ResourceBrowser.qml appelle addSelection hors registre) : refusé et
// signalé dans un document qui interdit la copie ; accepté sans règles.
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

// Vignettes glissées sur un autre onglet (DocumentTabBar.qml appelle copyPagesToTab hors registre) : les pages
// d'un document qui interdit la copie ne vont pas dans un document sans règles ; refus signalé (page.copy).
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

// Constat 3 : une action d'objet qui copie passe par le registre et y est refusée comme le menu.
void TstUiBroadcast::objectCopyActionRefused()
{
    Mount m;
    int copies = 0;
    m.commands.setNativeHandler(QStringLiteral("edit.copy"), [&copies](const QVariant&) { ++copies; });
    ui::ActionRunner runner;
    runner.setCommands(&m.commands);
    runner.setDocument(&m.document);
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

QTEST_MAIN(TstUiBroadcast)
#include "tst_ui_broadcast.moc"
