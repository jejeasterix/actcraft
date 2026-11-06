// Exécuteur d'actions (lot 8, L8-T9) : une action s'exécute au relâché d'un clic, jamais à l'appui ni pendant
// un contact ; un glisser, un clic droit ou le mode conception laissent tout à l'outil de sélection.
#include <actcraft/core/document.h>
#include <actcraft/core/model.h>
#include <actcraft/input/sample.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/liftlayeritem.h>
#include <actcraft/tools/tool.h>
#include <actcraft/ui/actionrunner.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/designmode.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/linklauncher.h>
#include <actcraft/ui/pagehistory.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QMouseEvent>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QtQml/qqmlextensionplugin.h>
#include <QtTest>

#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <vector>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace actcraft::ui {
// Fonctions libres de ui/src/textediting.cpp (comme tst_ui_text) : le document isolé du peintre et de la
// mesure.
QString safeTextHtml(const QString& html, const std::function<void(const QUrl&, const QVariant&)>& observer);
std::unique_ptr<QTextDocument>
isolatedTextDocument(std::function<void(const QUrl&, const QVariant&)> observer);
} // namespace actcraft::ui

namespace {

constexpr core::ObjectId kRound = 1;
constexpr core::ObjectId kLabel = 2;
const QPointF kOnRound(150, 150); // rond 1 : 100…200
const QPointF kOnLabel(430, 120); // étiquette 2 : 400…480 × 100…140
const QPointF kNowhere(900, 700);

core::Action action(const std::string& catalogId, core::ObjectId target = 0,
                    const std::string& parameters = std::string("{}"))
{
    core::Action a;
    a.catalogId = catalogId;
    a.target = target;
    a.parameters = parameters;
    return a;
}

core::PageObject roundObject(std::vector<core::Action> actions)
{
    core::Shape shape;
    shape.kind = core::ShapeKind::Rectangle;
    shape.bounds = core::RectF{100, 100, 100, 100};
    core::PageObject object{shape};
    object.common.id = kRound;
    object.common.actions = std::move(actions);
    return object;
}

core::PageObject labelObject(const std::string& text, std::vector<core::Action> actions = {})
{
    core::Text label;
    label.bounds = core::RectF{400, 100, 80, 40};
    label.html = text;
    core::PageObject object{label};
    object.common.id = kLabel;
    object.common.actions = std::move(actions);
    return object;
}

// Emprise mesurée comme le peintre la dessine (marge 0, largeur naturelle + 1).
core::RectF measured(const QString& html, double x, double y)
{
    std::unique_ptr<QTextDocument> doc = ui::isolatedTextDocument({});
    doc->setHtml(ui::safeTextHtml(html, {}));
    const double width = std::ceil(doc->idealWidth()) + 1;
    doc->setTextWidth(width);
    return core::RectF{x, y, width, std::ceil(doc->size().height())};
}

// Une page : le rond 1 porteur des actions, l'étiquette 2 ; `pages` pages en tout.
core::Document documentWith(std::vector<core::Action> roundActions, int pages = 1,
                            std::vector<core::Action> labelActions = {})
{
    core::Document document(1024, 768);
    document.page(0).objects.push_back(roundObject(std::move(roundActions)));
    document.page(0).objects.push_back(labelObject("Étiquette", std::move(labelActions)));
    for (int i = 1; i < pages; ++i) {
        document.insertPage(document.pageCount(), core::Page{});
    }
    return document;
}

double roundX(const core::Document& document)
{
    for (const core::PageObject& o : document.page(0).objects) {
        if (o.common.id == kRound) {
            return std::get<core::Shape>(core::body(o)).bounds.x;
        }
    }
    return -1;
}

// Zone de page réelle hors écran, rendu logiciel (comme ViewFixture de tst_ui_propertymodel) : la porte de
// l'exécuteur ne s'arme qu'avec une vue (ToolHost::armGate), l'outil de sélection n'existe qu'avec elle.
struct ViewMount
{
    QQmlEngine engine;
    std::unique_ptr<QObject> root;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;

    bool mount(ui::DocumentController& controller, ui::ToolHost& tools)
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick\nimport QtQuick.Window\nimport ActCraft.Ui\n"
                          "Window { width: 800; height: 600; visible: true\n"
                          "  PageArea { objectName: \"area\"; anchors.fill: parent } }",
                          QUrl(QStringLiteral("qrc:/tst_ui_actionrunner/Window.qml")));
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
        area->setProperty("documentController", QVariant::fromValue(&controller));
        view = window->findChild<ui::PageView*>(QStringLiteral("pageView"));
        auto* fixed = window->findChild<render::FixedPageItem*>(QStringLiteral("fixedPage"));
        auto* lift = window->findChild<render::LiftLayerItem*>(QStringLiteral("liftLayer"));
        if (!view || !fixed || !lift) {
            return false;
        }
        controller.setFixedPage(fixed);
        area->setProperty("tools", QVariant::fromValue(&tools));
        tools.setView(view);
        tools.setLiftLayer(lift); // comme Main.qml : sans couche, aucun outil (ToolHost::adapters)
        return QTest::qWaitForWindowExposed(window) &&
               QTest::qWaitFor([&] { return fixed->isReady() && view->cacheReady(); }, 10000);
    }
};

// Appui gauche tel que le filtre de l'application le voit (ToolHost::armGate), avant que la vue n'en fasse
// un échantillon.
void armAt(const ui::PageView& view, QPointF page)
{
    const QPointF scene = view.pageToScene(page);
    QMouseEvent press(QEvent::MouseButtonPress, scene, scene, scene, Qt::LeftButton, Qt::LeftButton,
                      Qt::NoModifier);
    QObject receiver;
    QCoreApplication::sendEvent(&receiver, &press);
}

// Échantillon en coordonnées de page, comme ToolDispatch le rend à l'outil actif.
input::Sample sampleAt(QPointF page, input::Phase phase)
{
    input::Sample sample{};
    sample.x = static_cast<decltype(sample.x)>(page.x());
    sample.y = static_cast<decltype(sample.y)>(page.y());
    sample.phase = phase;
    sample.pointer = 0; // souris
    return sample;
}

} // namespace

class TstUiActionrunner : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void clickRunsEveryActionInOrder();
    void dragDoesNotRun();
    void rightClickSelectsWithoutRunning();
    void designModeSelectsInstead();
    void lockedCommandActionRefused();
    void pageActionCreatesNoStep();
    void objectActionIsOneStep();
    void neverDuringContact();
    void hiddenObjectStillClickable();
    void lockedObjectStillRuns();
    void historyBackAndForward();
    void unsupportedActionDoesNothing();
    void documentOpenLaunchesNothing();
    void refusedWithoutCommands();
    // Travail ajouté par l'orchestrateur (constats des tâches précédentes).
    void editActionsTriggerEditCommands();
    void commandWithoutHandlerIsUnsupported();
    void textActionRemeasuresBounds();
    void alignGroupAndSelectAllUseRegistry();
    // Correctifs de la revue de L8-T9 : le geste réel, par l'outil actif de l'hôte.
    void gateClickRunsThroughActiveTool();
    void gateDragMovesWithoutRunning();

private:
    void load(core::Document document);
    const core::PageObject* object(core::ObjectId id, std::size_t page = 0) const;
    bool hidden(core::ObjectId id) const;
    // Clic réel : contact posé, appui, relâché, contact retombé, puis un tour de boucle.
    void click(QPointF point, Qt::MouseButton button = Qt::LeftButton);
    core::Document linkedDocument() const;

    QTemporaryDir m_settings;
    QTemporaryDir m_dir;
    std::unique_ptr<ui::DocumentController> m_controllerOwner;
    std::unique_ptr<ui::Commands> m_commands;
    std::unique_ptr<ui::DesignMode> m_design;
    ui::LinkLauncher* m_links = nullptr;
    std::unique_ptr<ui::LinkLauncher> m_linksOwner;
    std::unique_ptr<ui::ActionRunner> m_runner;
    int m_openerCalls = 0;
    int m_penCalls = 0;
};

void TstUiActionrunner::initTestCase()
{
    QVERIFY(m_settings.isValid());
    QVERIFY(m_dir.isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
    QStandardPaths::setTestModeEnabled(true);
    QSettings probe;
    QCOMPARE(probe.format(), QSettings::IniFormat);
    QVERIFY(probe.fileName().startsWith(m_settings.path()));
}

void TstUiActionrunner::init()
{
    m_controllerOwner = std::make_unique<ui::DocumentController>();
    m_commands = std::make_unique<ui::Commands>();
    m_design = std::make_unique<ui::DesignMode>();
    m_linksOwner = std::make_unique<ui::LinkLauncher>();
    m_links = m_linksOwner.get();
    m_openerCalls = 0;
    m_penCalls = 0;
    m_links->setOpener([this](const QUrl&) {
        ++m_openerCalls;
        return true;
    });
    m_commands->setNativeHandler(QStringLiteral("tool.pen"), [this](const QVariant&) { ++m_penCalls; });
    // Gestionnaires de Main.qml pour la navigation des pages.
    m_commands->setNativeHandler(QStringLiteral("page.next"),
                                 [this](const QVariant&) { m_controllerOwner->nextPage(); });
    m_runner = std::make_unique<ui::ActionRunner>();
    m_runner->setCommands(m_commands.get());
    m_runner->setDocument(m_controllerOwner.get());
    m_runner->setDesignMode(m_design.get());
    m_runner->setLinks(m_links);
}

void TstUiActionrunner::cleanup()
{
    m_runner.reset();
    m_linksOwner.reset();
    m_links = nullptr;
    m_design.reset();
    m_commands.reset();
    m_controllerOwner.reset();
}

void TstUiActionrunner::load(core::Document document)
{
    m_controllerOwner->replaceDocument(std::move(document),
                                       QUrl::fromLocalFile(m_dir.filePath(QStringLiteral("essai.actcraft"))),
                                       QStringLiteral("essai"));
}

const core::PageObject* TstUiActionrunner::object(core::ObjectId id, std::size_t page) const
{
    for (const core::PageObject& o : m_controllerOwner->document().page(page).objects) {
        if (o.common.id == id) {
            return &o;
        }
    }
    return nullptr;
}

bool TstUiActionrunner::hidden(core::ObjectId id) const
{
    const core::PageObject* o = object(id);
    return o != nullptr && !o->common.visible;
}

void TstUiActionrunner::click(QPointF point, Qt::MouseButton button)
{
    m_runner->setContactActive(true);
    m_runner->press(point, button);
    m_runner->release(point);
    m_runner->setContactActive(false);
    QCoreApplication::processEvents();
}

core::Document TstUiActionrunner::linkedDocument() const
{
    core::Document document(1024, 768);
    document.page(0).objects.push_back(
        roundObject({action("document.open-website", 0, R"({"url":"https://example.org/"})")}));
    return document;
}

// Deux actions sur la même cible : la seconde voit l'effet de la première (masquer puis réafficher).
void TstUiActionrunner::clickRunsEveryActionInOrder()
{
    load(documentWith({action("object.toggle-hidden", kLabel), action("object.toggle-hidden", kLabel)}));
    QVERIFY(!hidden(kLabel));
    click(kOnRound);
    QVERIFY(m_controllerOwner->canUndo());
    QVERIFY(!hidden(kLabel)); // calculées d'avance, les deux masqueraient
    m_controllerOwner->undo();
    QVERIFY(hidden(kLabel)); // la première a bien masqué
    m_controllerOwner->undo();
    QVERIFY(!hidden(kLabel));
    QVERIFY(!m_controllerOwner->canUndo());
}

// Patron interdit : une exécution à l'appui masquerait l'étiquette dès le début du glisser.
void TstUiActionrunner::dragDoesNotRun()
{
    load(documentWith({action("object.toggle-hidden", kLabel)}));
    QSignalSpy refused(m_runner.get(), &ui::ActionRunner::actionRefused);
    m_runner->setContactActive(true);
    QVERIFY(m_runner->press(kOnRound, Qt::LeftButton));
    QVERIFY(!hidden(kLabel));
    QVERIFY(m_runner->move(kOnRound + QPointF(2, 2)));   // sous 4 px : l'échantillon reste au runner
    QVERIFY(!m_runner->move(kOnRound + QPointF(10, 0))); // au-delà : rendu à l'outil
    QVERIFY(!m_runner->move(kOnRound + QPointF(20, 0)));
    QVERIFY(!m_runner->release(kOnRound + QPointF(20, 0)));
    m_runner->setContactActive(false);
    QCoreApplication::processEvents();
    QVERIFY(!hidden(kLabel));
    QVERIFY(!m_controllerOwner->canUndo());
    QCOMPARE(refused.size(), 0);
}

void TstUiActionrunner::rightClickSelectsWithoutRunning()
{
    load(documentWith({action("object.toggle-hidden", kLabel)}));
    m_runner->setContactActive(true);
    QVERIFY(!m_runner->press(kOnRound, Qt::RightButton)); // l'outil de sélection sélectionne
    QVERIFY(!m_runner->release(kOnRound));
    m_runner->setContactActive(false);
    QCoreApplication::processEvents();
    QVERIFY(!hidden(kLabel));
    QVERIFY(!m_controllerOwner->canUndo());
}

void TstUiActionrunner::designModeSelectsInstead()
{
    load(documentWith({action("object.toggle-hidden", kLabel)}));
    m_design->setSetting(ui::DesignMode::Setting::DisableActions, true);
    m_design->setActive(true);
    QVERIFY(m_design->actionsDisabled());
    m_runner->setContactActive(true);
    QVERIFY(!m_runner->press(kOnRound, Qt::LeftButton));
    QVERIFY(!m_runner->release(kOnRound));
    m_runner->setContactActive(false);
    QCoreApplication::processEvents();
    QVERIFY(!hidden(kLabel));
    // Hors du mode conception, le même clic exécute.
    m_design->setActive(false);
    click(kOnRound);
    QVERIFY(hidden(kLabel));
}

void TstUiActionrunner::lockedCommandActionRefused()
{
    load(documentWith({action("command.tool.pen")}));
    click(kOnRound);
    QCOMPARE(m_penCalls, 1); // déverrouillé : la commande passe par le registre
    m_commands->setLocked(true);
    QSignalSpy refused(m_runner.get(), &ui::ActionRunner::actionRefused);
    click(kOnRound);
    QCOMPARE(m_penCalls, 1);
    QCOMPARE(refused.size(), 1);
}

void TstUiActionrunner::pageActionCreatesNoStep()
{
    load(documentWith({action("page.next")}, 2));
    QCOMPARE(m_controllerOwner->currentIndex(), 0);
    click(kOnRound);
    QCOMPARE(m_controllerOwner->currentIndex(), 1);
    QVERIFY(!m_controllerOwner->canUndo());
}

void TstUiActionrunner::objectActionIsOneStep()
{
    load(documentWith({action("object.toggle-hidden", kLabel)}));
    click(kOnRound);
    QVERIFY(hidden(kLabel));
    QVERIFY(m_controllerOwner->canUndo());
    m_controllerOwner->undo();
    QVERIFY(!hidden(kLabel));
    QVERIFY(!m_controllerOwner->canUndo()); // une seule étape
}

// Patron interdit : à l'appui, ou au relâché sans attendre, l'action tournerait pendant le contact.
void TstUiActionrunner::neverDuringContact()
{
    load(documentWith({action("object.toggle-hidden", kLabel)}));
    m_runner->setContactActive(true);
    QVERIFY(m_runner->press(kOnRound, Qt::LeftButton));
    QVERIFY(!hidden(kLabel));
    QVERIFY(m_runner->release(kOnRound));
    QCoreApplication::processEvents();
    QVERIFY(!hidden(kLabel)); // contact encore vrai : rien
    QVERIFY(!m_controllerOwner->canUndo());
    m_runner->setContactActive(false);
    QCoreApplication::processEvents();
    QVERIFY(hidden(kLabel)); // à la retombée : exécution, une fois
    m_runner->setContactActive(true);
    m_runner->setContactActive(false);
    QCoreApplication::processEvents();
    QVERIFY(hidden(kLabel));
    m_controllerOwner->undo();
    QVERIFY(!m_controllerOwner->canUndo());
}

// D5 : un objet masqué reste cliquable.
void TstUiActionrunner::hiddenObjectStillClickable()
{
    core::Document document = documentWith({action("object.toggle-hidden", kLabel)});
    document.page(0).objects[0].common.visible = false;
    load(std::move(document));
    click(kOnRound);
    QVERIFY(hidden(kLabel));
}

// D4 : un objet verrouillé exécute ses actions.
void TstUiActionrunner::lockedObjectStillRuns()
{
    core::Document document = documentWith({action("object.toggle-hidden", kLabel)});
    document.page(0).objects[0].common.locked = true;
    load(std::move(document));
    click(kOnRound);
    QVERIFY(hidden(kLabel));
}

void TstUiActionrunner::historyBackAndForward()
{
    // Historique seul : une navigation par l'historique n'est pas réinscrite, pile bornée à 100.
    ui::PageHistory history;
    QCOMPARE(history.back(), -1);
    history.visited(0);
    history.visited(1);
    history.visited(2);
    QCOMPARE(history.back(), 1);
    QCOMPARE(history.back(), 0);
    QCOMPARE(history.back(), -1);
    QCOMPARE(history.forward(), 1);
    QCOMPARE(history.forward(), 2);
    QCOMPARE(history.forward(), -1);
    ui::PageHistory bounded;
    for (int i = 0; i < 150; ++i) {
        bounded.visited(i);
    }
    int steps = 0;
    while (bounded.back() >= 0) {
        ++steps;
    }
    QCOMPARE(steps, 99);

    // Par les actions : rond 1 « page précédente de l'historique », étiquette 2 « suivante ».
    core::Document document =
        documentWith({action("page.history-back")}, 3, {action("page.history-forward")});
    document.page(1).objects.push_back(document.page(0).objects[0]);
    document.page(1).objects.push_back(document.page(0).objects[1]);
    document.page(2).objects.push_back(document.page(0).objects[0]);
    document.page(2).objects.push_back(document.page(0).objects[1]);
    load(std::move(document));
    m_controllerOwner->showPage(1);
    m_controllerOwner->showPage(2);
    click(kOnRound);
    QCOMPARE(m_controllerOwner->currentIndex(), 1);
    click(kOnRound);
    QCOMPARE(m_controllerOwner->currentIndex(), 0);
    click(kOnLabel);
    QCOMPARE(m_controllerOwner->currentIndex(), 1);
    click(kOnLabel);
    QCOMPARE(m_controllerOwner->currentIndex(), 2);
    QVERIFY(!m_controllerOwner->canUndo());
}

void TstUiActionrunner::unsupportedActionDoesNothing()
{
    load(documentWith({action(""), action("object.does-not-exist", kLabel)}));
    QSignalSpy refused(m_runner.get(), &ui::ActionRunner::actionRefused);
    click(kOnRound);
    QCOMPARE(refused.size(), 2);
    QVERIFY(!hidden(kLabel));
    QVERIFY(!m_controllerOwner->canUndo());
    // Un clic hors de tout objet d'action : rien à prendre.
    QVERIFY(!m_runner->press(kNowhere, Qt::LeftButton));
    QVERIFY(!m_runner->release(kNowhere));
    QCOMPARE(refused.size(), 2);
}

void TstUiActionrunner::documentOpenLaunchesNothing()
{
    // Document « reçu » : un rond portant document.open-website vers https://example.org/.
    // replaceDocument(core::Document, const QUrl& fileUrl, const QString& title) : documentcontroller.h:94.
    // Espions posés avant l'ouverture : une demande faite pendant replaceDocument (à l'affichage) se voit.
    QSignalSpy ask(m_links, &ui::LinkLauncher::confirmationNeeded);
    QSignalSpy refusedLink(m_links, &ui::LinkLauncher::refused);
    m_controllerOwner->replaceDocument(linkedDocument(),
                                       QUrl::fromLocalFile(m_dir.filePath(QStringLiteral("recu.actcraft"))),
                                       QStringLiteral("recu"));
    QTest::qWait(500);
    QCOMPARE(ask.size(), 0);
    QCOMPARE(refusedLink.size(), 0);
    QCOMPARE(m_openerCalls, 0);
    // Le clic demande une confirmation, et n'ouvre toujours rien.
    click(kOnRound);
    QCOMPARE(ask.size(), 1);
    QCOMPARE(m_openerCalls, 0);
}

void TstUiActionrunner::refusedWithoutCommands()
{
    load(documentWith({action("object.toggle-hidden", kLabel), action("command.tool.pen")}));
    m_runner->setCommands(nullptr);
    QSignalSpy refused(m_runner.get(), &ui::ActionRunner::actionRefused);
    click(kOnRound);
    QVERIFY(refused.size() >= 1);
    QVERIFY(!hidden(kLabel));
    QCOMPARE(m_penCalls, 0);
    QVERIFY(!m_controllerOwner->canUndo());
}

// Copier, couper, coller ne sont pas des actions du moteur : l'exécuteur déclenche edit.* du registre.
void TstUiActionrunner::editActionsTriggerEditCommands()
{
    ui::ToolHost tools;
    tools.setSettingsFile(m_dir.filePath(QStringLiteral("outils.ini")));
    tools.setDocumentController(m_controllerOwner.get());
    tools.setActionRunner(m_runner.get());
    QStringList fired;
    QList<int> selected; // sélection au moment de la commande : la cible seule
    for (const QString& id :
         {QStringLiteral("edit.copy"), QStringLiteral("edit.cut"), QStringLiteral("edit.paste")}) {
        m_commands->setNativeHandler(id, [&, id](const QVariant&) {
            fired.append(id);
            selected.append(tools.selectionCount());
        });
    }
    load(documentWith({action("object.copy", kLabel), action("object.cut", kLabel), action("object.paste")}));
    click(kOnRound);
    QCOMPARE(fired, (QStringList{QStringLiteral("edit.copy"), QStringLiteral("edit.cut"),
                                 QStringLiteral("edit.paste")}));
    QCOMPARE(selected.at(0), 1);
    QCOMPARE(selected.at(1), 1);
    QCOMPARE(tools.currentTool(), QStringLiteral("select"));
    tools.setActionRunner(nullptr);
}

// Aligner, grouper, tout sélectionner : aucune table du moteur, les commandes existantes du registre.
void TstUiActionrunner::alignGroupAndSelectAllUseRegistry()
{
    ui::ToolHost tools;
    tools.setSettingsFile(m_dir.filePath(QStringLiteral("outils.ini")));
    tools.setDocumentController(m_controllerOwner.get());
    tools.setActionRunner(m_runner.get());
    QStringList fired;
    for (const QString& id :
         {QStringLiteral("align.left"), QStringLiteral("object.group"), QStringLiteral("edit.select-all")}) {
        m_commands->setNativeHandler(id, [&fired, id](const QVariant&) { fired.append(id); });
    }
    load(documentWith({action("object.align-left", kLabel), action("object.toggle-group", kLabel),
                       action("object.select-all")}));
    QSignalSpy refused(m_runner.get(), &ui::ActionRunner::actionRefused);
    click(kOnRound);
    QCOMPARE(fired, (QStringList{QStringLiteral("align.left"), QStringLiteral("object.group"),
                                 QStringLiteral("edit.select-all")}));
    QCOMPARE(refused.size(), 0);
    // Sans hôte des outils, la cible ne peut être sélectionnée : non prise en charge, rien de déclenché.
    tools.setActionRunner(nullptr);
    fired.clear();
    click(kOnRound);
    QCOMPARE(fired, QStringList{QStringLiteral("edit.select-all")});
    QCOMPARE(refused.size(), 2);
}

// H8-7 : une commande déclarée sans gestionnaire sur la tête (lot 7 absent) est une action non prise en
// charge.
void TstUiActionrunner::commandWithoutHandlerIsUnsupported()
{
    load(documentWith({action("command.present.curtain"), action("object.toggle-hidden", kLabel)}));
    QSignalSpy refused(m_runner.get(), &ui::ActionRunner::actionRefused);
    click(kOnRound);
    QCOMPARE(refused.size(), 1);
    QVERIFY(hidden(kLabel)); // l'action suivante tourne quand même
}

// « 9 » + 1 → « 10 » : l'emprise du texte est remesurée (le peintre découpe le texte à son cadre), dans la
// même étape d'annulation que la valeur.
void TstUiActionrunner::textActionRemeasuresBounds()
{
    core::Document document = documentWith({action("object.text-value", kLabel, R"({"value":1})")});
    auto* label = std::get_if<core::Text>(&core::body(document.page(0).objects[1]));
    QVERIFY(label != nullptr);
    label->html = "9";
    label->bounds = measured(QStringLiteral("9"), 400, 100); // cadre juste à « 9 »
    const double before = label->bounds.width;
    load(std::move(document));
    click(kOnRound);
    const auto* after = std::get_if<core::Text>(&core::body(*object(kLabel)));
    QVERIFY(after != nullptr);
    QCOMPARE(QTextDocumentFragment::fromHtml(QString::fromStdString(after->html)).toPlainText().trimmed(),
             QStringLiteral("10"));
    const core::RectF ten = measured(QString::fromStdString(after->html), 400, 100);
    QVERIFY2(after->bounds.width > before, "emprise élargie");
    QVERIFY(after->bounds.width >= ten.width); // « 10 » entier dans son cadre
    QVERIFY(after->bounds.height >= ten.height);
    m_controllerOwner->undo(); // valeur et remesure : une seule étape
    QVERIFY(!m_controllerOwner->canUndo());
    const auto* undone = std::get_if<core::Text>(&core::body(*object(kLabel)));
    QCOMPARE(undone->bounds.width, before);
}

// Constat bloquant de la revue : à l'appui, la porte se désarmait (drop appelait done) ; le relâché allait à
// la sélection nue, l'exécuteur ne le voyait jamais. Sous ce patron, l'étiquette reste visible.
void TstUiActionrunner::gateClickRunsThroughActiveTool()
{
    load(documentWith({action("object.toggle-hidden", kLabel)}));
    ViewMount mount;
    ui::ToolHost tools;
    tools.setSettingsFile(m_dir.filePath(QStringLiteral("outils.ini")));
    tools.setDocumentController(m_controllerOwner.get());
    QVERIFY(mount.mount(*m_controllerOwner, tools));
    tools.setActionRunner(m_runner.get());
    tools.selectObjects({}); // l'outil de sélection
    QCOMPARE(tools.currentTool(), QStringLiteral("select"));
    m_runner->setContactActive(true);
    armAt(*mount.view, kOnRound);
    QVERIFY(tools.activeTool() != nullptr);
    tools.activeTool()->onSample(sampleAt(kOnRound, input::Phase::Press));
    tools.activeTool()->onSample(sampleAt(kOnRound, input::Phase::Release));
    m_runner->setContactActive(false);
    QCoreApplication::processEvents();
    QVERIFY2(hidden(kLabel), "le clic exécute l'action");
    tools.setActionRunner(nullptr);
}

// Même constat : les déplacements allaient à une sélection qui n'avait pas reçu l'appui ; le glisser ne
// démarrait pas. Sous ce patron, le rond reste à 100.
void TstUiActionrunner::gateDragMovesWithoutRunning()
{
    load(documentWith({action("object.toggle-hidden", kLabel)}));
    ViewMount mount;
    ui::ToolHost tools;
    tools.setSettingsFile(m_dir.filePath(QStringLiteral("outils.ini")));
    tools.setDocumentController(m_controllerOwner.get());
    QVERIFY(mount.mount(*m_controllerOwner, tools));
    tools.setActionRunner(m_runner.get());
    tools.selectObjects({});
    QCOMPARE(tools.currentTool(), QStringLiteral("select"));
    QCOMPARE(roundX(m_controllerOwner->document()), 100.0);
    m_runner->setContactActive(true);
    armAt(*mount.view, kOnRound);
    QVERIFY(tools.activeTool() != nullptr);
    const QPointF end = kOnRound + QPointF(30, 0);
    tools.activeTool()->onSample(sampleAt(kOnRound, input::Phase::Press));
    tools.activeTool()->onSample(sampleAt(kOnRound + QPointF(10, 0), input::Phase::Move));
    tools.activeTool()->onSample(sampleAt(kOnRound + QPointF(20, 0), input::Phase::Move));
    tools.activeTool()->onSample(sampleAt(end, input::Phase::Move));
    tools.activeTool()->onSample(sampleAt(end, input::Phase::Release));
    m_runner->setContactActive(false);
    QCoreApplication::processEvents();
    QVERIFY(!hidden(kLabel)); // aucune action
    QVERIFY2(roundX(m_controllerOwner->document()) > 110.0, "le glisser déplace le rond");
    tools.setActionRunner(nullptr);
}

QTEST_MAIN(TstUiActionrunner)
#include "tst_ui_actionrunner.moc"
