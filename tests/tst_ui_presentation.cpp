#include "desktopcontroller.h"
#include "presentationcommands.h"
#include "presentationdocument.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/actions/commandregistry.h>
#include <actcraft/core/document.h>
#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/formats/nativejson.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>

#include <QBuffer>
#include <QColor>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

namespace {

using actcraft::ui::Commands;
using actcraft::ui::DocumentController;
using actcraft::ui::DocumentInsertions;
using presentationharness::Pointer;
namespace core = actcraft::core;

DocumentController* controllerOf(const presentationharness::MainWindow& main)
{
    return main.window() ? main.window()->findChild<DocumentController*>(QStringLiteral("documentController"))
                         : nullptr;
}

const core::PageObject* findObject(const core::Page& page, core::ObjectId id)
{
    const std::size_t index = core::indexOf(page, id);
    return index < page.objects.size() ? &page.objects[index] : nullptr;
}

const core::Page& shown(const DocumentController& controller)
{
    return controller.document().page(static_cast<std::size_t>(controller.currentIndex()));
}

// Départ commun des tests du retour à la page de départ : 3 pages, page 2 affichée (indice 1).
bool startOnSecondOfThree(DocumentController& controller)
{
    controller.loadGeneratedPages(3);
    controller.nextPage();
    return controller.pageCount() == 3 && controller.currentIndex() == 1;
}

core::PageObject squareShape()
{
    core::Shape shape;
    shape.kind = core::ShapeKind::Path;
    shape.paths = {{{0, 0}, {60, 0}, {60, 60}, {0, 60}}};
    shape.bounds = core::RectF{-2, -2, 64, 64};
    shape.pen.width = 4;
    core::PageObject object = shape;
    object.common.layer = core::Layer::Top;
    object.common.transform = core::translation(300, 200);
    return object;
}

QByteArray pngOf(const QColor& color)
{
    QImage image(8, 8, QImage::Format_ARGB32);
    image.fill(color);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    return bytes;
}

bool jsonHasResource(const core::Document& document, const std::string& key)
{
    const QJsonArray resources = actcraft::formats::documentToJson(document).value(u"resources").toArray();
    return std::any_of(resources.begin(), resources.end(), [&key](const QJsonValue& entry) {
        return entry.toObject().value(u"key").toString() == QString::fromStdString(key);
    });
}

Commands* commandsOf(const presentationharness::MainWindow& main)
{
    return main.window() ? main.window()->findChild<Commands*>(QStringLiteral("commands")) : nullptr;
}

// Identifiants (clé « id ») d'une liste de menus ou de palette, sous-menus compris.
void collectIds(const QVariant& node, QStringList& ids)
{
    if (node.metaType().id() == QMetaType::QVariantList) {
        for (const QVariant& child : node.toList()) {
            collectIds(child, ids);
        }
    } else if (node.metaType().id() == QMetaType::QVariantMap) {
        const QVariantMap map = node.toMap();
        if (map.contains(QStringLiteral("id"))) {
            ids << map.value(QStringLiteral("id")).toString();
        }
        for (const QVariant& child : map) {
            collectIds(child, ids);
        }
    }
}

// Les identifiants du bloc du lot 7 de commands.json : de « view.page-zoom » (première commande du lot) à la
// fin.
QStringList lot7CommandIds()
{
    QStringList ids;
    bool inBlock = false;
    const QJsonArray all = QJsonDocument::fromJson(actcraft::actions::defaultCommandsJson())
                               .object()
                               .value(u"commands")
                               .toArray();
    for (const QJsonValue& entry : all) {
        const QString id = entry.toObject().value(u"id").toString();
        inBlock = inBlock || id == QStringLiteral("view.page-zoom");
        if (inBlock) {
            ids << id;
        }
    }
    return ids;
}

// Panneau flottant du lot qui prend l'entrée : commande qui l'ouvre, racine.
struct PanelRoot
{
    const char* command;
    const char* root;
};

// Trait au stylet injecté sous la page (coordonnées de page) ; le trait versé est le dernier objet de la page
// (couche supérieure).
void stylusStroke(const presentationharness::MainWindow& main, QPointF from, QPointF to)
{
    presentationharness::strokeAt(main.window(), Pointer::Stylus, main.pageView()->pageToScene(from),
                                  main.pageView()->pageToScene(to));
}

// Panneau flottant de 200 × 120 posé sur la couche des panneaux, visible dès sa création, sa barre de titre
// centrée sur le point de page (400, 300). `declared` : la zone de page est liée dans la déclaration du
// panneau (comme dans une racine du lot), sinon posée après sa création (setProperty, changement notifié).
QQuickItem* openPanel(const presentationharness::MainWindow& main, bool declared = false)
{
    QQuickItem* layer = main.item(QStringLiteral("presentationLayer"));
    QQuickItem* area = main.item(QStringLiteral("pageArea"));
    if (!layer || !area) {
        return nullptr;
    }
    QQmlComponent component(qmlEngine(main.window()));
    component.setData(
        declared ? "import QtQuick\n"
                   "import ActCraft.Ui\n"
                   "FloatingPanel { width: 200; height: 120; title: \"Essai\"; pageArea: panelArea }\n"
                 : "import QtQuick\n"
                   "import ActCraft.Ui\n"
                   "FloatingPanel { width: 200; height: 120; title: \"Essai\" }\n",
        QUrl());
    // Contexte propre au panneau (détruit avec la couche) : il porte la zone de page de la liaison.
    auto* context = new QQmlContext(qmlContext(main.window()), layer);
    context->setContextProperty(QStringLiteral("panelArea"), area);
    auto* panel = qobject_cast<QQuickItem*>(component.beginCreate(context));
    if (!panel) {
        qWarning("%s", qPrintable(component.errorString()));
        return nullptr;
    }
    panel->setParent(layer);
    panel->setParentItem(layer);
    if (!declared) {
        panel->setProperty("pageArea", QVariant::fromValue(area));
    }
    const QPointF titleCentre = main.pageView()->pageToScene(QPointF(400, 300));
    panel->setPosition(layer->mapFromScene(titleCentre - QPointF(100, 22)));
    component.completeCreate();
    return panel;
}

// Comme presentationharness::strokeAt, avec une lecture à mi-chemin (contact encore posé).
void strokeWithProbe(QQuickWindow* window, Pointer pointer, QPointF from, QPointF to,
                     const std::function<void()>& midway)
{
    constexpr int steps = 8;
    const auto at = [&](int i) { return from + (to - from) * (static_cast<qreal>(i) / steps); };
    switch (pointer) {
    case Pointer::Stylus:
        presentationharness::sendTablet(window, QEvent::TabletPress, from);
        for (int i = 1; i <= steps; ++i) {
            presentationharness::sendTablet(window, QEvent::TabletMove, at(i));
            if (i == steps / 2) {
                midway();
            }
        }
        presentationharness::sendTablet(window, QEvent::TabletRelease, to);
        break;
    case Pointer::Finger:
        QTest::touchEvent(window, presentationharness::touchDevice()).press(0, from.toPoint());
        presentationharness::settleTouch();
        for (int i = 1; i <= steps; ++i) {
            QTest::touchEvent(window, presentationharness::touchDevice()).move(0, at(i).toPoint());
            presentationharness::settleTouch();
            if (i == steps / 2) {
                midway();
            }
        }
        QTest::touchEvent(window, presentationharness::touchDevice()).release(0, to.toPoint());
        presentationharness::settleTouch();
        break;
    case Pointer::Mouse:
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, from.toPoint());
        for (int i = 1; i <= steps; ++i) {
            QTest::mouseMove(window, at(i).toPoint());
            if (i == steps / 2) {
                midway();
            }
        }
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, to.toPoint());
        break;
    }
    QCoreApplication::processEvents();
}

class TstUiPresentation : public QObject
{
    Q_OBJECT

private slots:
    void lot7RootsExist()
    {
        presentationharness::MainWindow main;
        QVERIFY(main.window() != nullptr);
        // QtObject, pas un Item (socle L0-S9).
        QObject* presentation = main.window()->findChild<QObject*>(QStringLiteral("presentation"));
        QVERIFY(presentation != nullptr);
        const char* roots[] = {"curtainRoot",    "spotlightRoot", "pageZoomRoot",  "pageToolsRoot",
                               "transitionRoot", "effectsRoot",   "mathToolsRoot", "diceRoot",
                               "calculatorRoot", "clockRoot",     "tickerRoot",    "recognitionRoot",
                               "equationRoot",   "cameraRoot",    "keyboardRoot",  "desktopRoot",
                               "otherToolsRoot"};
        for (const char* name : roots) {
            QObject* root = main.window()->findChild<QObject*>(QString::fromLatin1(name));
            QVERIFY2(root != nullptr, name);
            // Les quatre liaisons posées par Presentation.qml (décision 4).
            for (const char* bound : {"commands", "documentController", "tools", "pageArea"}) {
                QVERIFY2(root->property(bound).value<QObject*>() != nullptr,
                         qPrintable(QStringLiteral("%1.%2").arg(QLatin1String(name), QLatin1String(bound))));
            }
        }
        auto* overlays = main.window()->findChild<QQuickItem*>(QStringLiteral("pageOverlays"));
        QVERIFY(overlays != nullptr);
        for (const char* name : {"curtainRoot", "spotlightRoot", "mathToolsRoot"}) {
            auto* root = main.window()->findChild<QQuickItem*>(QString::fromLatin1(name));
            QVERIFY2(root != nullptr && root->parentItem() == overlays, name);
        }
        auto* layer = main.window()->findChild<QQuickItem*>(QStringLiteral("presentationLayer"));
        auto* ticker = main.window()->findChild<QQuickItem*>(QStringLiteral("tickerRoot"));
        QVERIFY(layer != nullptr && ticker != nullptr);
        QCOMPARE(ticker->parentItem(), layer);
        // Empilement fixé par z (décision 3) : chaque valeur, pas seulement un ordre.
        const std::pair<const char*, qreal> stacking[] = {
            {"curtainRoot", 10},     {"spotlightRoot", 20}, {"mathToolsRoot", 30},  {"transitionRoot", 10},
            {"tickerRoot", 20},      {"diceRoot", 30},      {"calculatorRoot", 31}, {"clockRoot", 32},
            {"recognitionRoot", 40}, {"equationRoot", 41},  {"effectsRoot", 50}};
        for (const auto& [name, z] : stacking) {
            auto* root = main.window()->findChild<QQuickItem*>(QString::fromLatin1(name));
            QVERIFY2(root != nullptr, name);
            QVERIFY2(qFuzzyCompare(root->z(), z), name);
        }
        for (const char* name : {"cameraRoot", "keyboardRoot", "desktopRoot"}) {
            auto* root = main.window()->findChild<QQuickItem*>(QString::fromLatin1(name));
            QVERIFY2(root != nullptr && !root->isVisible(), name); // racine non visuelle (décision 4 bis)
        }
        // Parent visuel de chacune des 17 racines (décision 3) : 12 placées, 5 sans parent.
        auto* slot = main.window()->findChild<QQuickItem*>(QStringLiteral("presentationSlot"));
        QVERIFY(slot != nullptr);
        const std::pair<const char*, QQuickItem*> parents[] = {
            {"curtainRoot", overlays},  {"spotlightRoot", overlays}, {"mathToolsRoot", overlays},
            {"pageZoomRoot", slot},     {"transitionRoot", layer},   {"effectsRoot", layer},
            {"diceRoot", layer},        {"calculatorRoot", layer},   {"clockRoot", layer},
            {"tickerRoot", layer},      {"recognitionRoot", layer},  {"equationRoot", layer},
            {"pageToolsRoot", nullptr}, {"otherToolsRoot", nullptr}, {"cameraRoot", nullptr},
            {"keyboardRoot", nullptr},  {"desktopRoot", nullptr}};
        for (const auto& [name, expected] : parents) {
            auto* root = main.window()->findChild<QQuickItem*>(QString::fromLatin1(name));
            QVERIFY2(root != nullptr && root->parentItem() == expected, name);
        }
    }

    // Texte : un core::Text de la couche intermédiaire, HTML échappé ; une étape.
    void insertTextIsUndoneAtOnce()
    {
        presentationharness::MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        DocumentInsertions insertions(*controller);
        const int before = main.pageObjectCount();
        const core::ObjectId id = insertions.insertText(QStringLiteral("12"), QRectF(100, 100, 80, 40));
        QVERIFY(id != 0);
        QTRY_COMPARE(main.pageObjectCount(), before + 1);
        const core::PageObject* object = findObject(shown(*controller), id);
        QVERIFY(object != nullptr);
        const auto* text = std::get_if<core::Text>(&core::body(*object));
        QVERIFY(text != nullptr);
        QVERIFY(object->common.layer == core::Layer::Middle);
        QVERIFY(QString::fromStdString(text->html).contains(QStringLiteral(">12<")));
        controller->undo();
        QTRY_COMPARE(main.pageObjectCount(), before);
        QVERIFY(findObject(shown(*controller), id) == nullptr);
        QVERIFY(!controller->canUndo());
        // Texte brut : jamais interprété comme du HTML.
        const core::ObjectId escaped = insertions.insertText(QStringLiteral("a<b>&"), QRectF(0, 0, 80, 40));
        const auto* plain = std::get_if<core::Text>(&core::body(*findObject(shown(*controller), escaped)));
        QVERIFY(plain != nullptr);
        QVERIFY(QString::fromStdString(plain->html).contains(QStringLiteral("a&lt;b&gt;&amp;")));
        // Page affichée d'indice 1 : le texte y va, pas sur la page 0.
        QVERIFY(startOnSecondOfThree(*controller));
        const std::size_t first = controller->document().page(0).objects.size();
        const core::ObjectId second = insertions.insertText(QStringLiteral("34"), QRectF(100, 100, 80, 40));
        QVERIFY(second != 0);
        QVERIFY(findObject(controller->document().page(1), second) != nullptr);
        QCOMPARE(controller->document().page(0).objects.size(), first);
    }

    // Page ajoutée en fin et objet posé dessus : une seule étape (le patron « nextPage() puis insertObject »
    // en ferait deux) ; l'annulation ramène à la page de départ.
    void insertObjectOnNewPageIsUndoneAtOnce()
    {
        presentationharness::MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(startOnSecondOfThree(*controller));
        DocumentInsertions insertions(*controller);
        const core::ObjectId id = insertions.insertObjectOnNewPage(squareShape());
        QVERIFY(id != 0);
        QCOMPARE(controller->pageCount(), 4);
        QCOMPARE(controller->currentIndex(), 3);
        QCOMPARE(controller->document().page(3).objects.size(), std::size_t{1});
        QCOMPARE(controller->document().page(3).objects.front().common.id, id);
        controller->undo();
        QCOMPARE(controller->pageCount(), 3);
        QTRY_COMPARE(controller->currentIndex(), 1);
        QVERIFY(!controller->canUndo());
    }

    // Parti de la page ajoutée avant l'annulation : on reste là où l'on est.
    void undoAfterNavigatingAwayKeepsThePage()
    {
        presentationharness::MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(startOnSecondOfThree(*controller));
        DocumentInsertions insertions(*controller);
        QVERIFY(insertions.insertObjectOnNewPage(squareShape()) != 0);
        QCOMPARE(controller->currentIndex(), 3);
        controller->pageBack();
        QCOMPARE(controller->currentIndex(), 2);
        controller->undo();
        QTest::qWait(50);
        QCOMPARE(controller->pageCount(), 3);
        QCOMPARE(controller->currentIndex(), 2);
    }

    // Deux insertions sur une page neuve : chaque annulation ramène à la page de départ de la sienne. Jamais
    // QTRY_COMPARE ici : il n'attendrait rien si la valeur est déjà la bonne avant la navigation différée.
    void twoNewPageInsertionsUndoInOrder()
    {
        presentationharness::MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(startOnSecondOfThree(*controller));
        DocumentInsertions insertions(*controller);
        QVERIFY(insertions.insertObjectOnNewPage(squareShape()) != 0);
        QVERIFY(insertions.insertObjectOnNewPage(squareShape()) != 0);
        QCOMPARE(controller->pageCount(), 5);
        QCOMPARE(controller->currentIndex(), 4);
        controller->undo();
        QTest::qWait(50);
        QCOMPARE(controller->pageCount(), 4);
        QCOMPARE(controller->currentIndex(), 3);
        controller->undo();
        QTest::qWait(50);
        QCOMPARE(controller->pageCount(), 3);
        QCOMPARE(controller->currentIndex(), 1);
    }

    // « Nouveau » oublie la page de départ : une annulation dans le document suivant n'y ramène jamais.
    void replacedDocumentForgetsTheReturnPage()
    {
        presentationharness::MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(startOnSecondOfThree(*controller));
        DocumentInsertions insertions(*controller);
        QVERIFY(insertions.insertObjectOnNewPage(squareShape()) != 0);
        QCOMPARE(controller->currentIndex(), 3);
        controller->newDocument(1024, 768);
        controller->loadGeneratedPages(3);
        controller->nextPage();
        controller->nextPage();
        QCOMPARE(controller->currentIndex(), 2);
        controller->nextPage(); // sur la dernière : ajoute la page d'indice 3 (une étape) et l'affiche
        QCOMPARE(controller->pageCount(), 4);
        QCOMPARE(controller->currentIndex(), 3);
        controller->undo();
        QTest::qWait(50);
        QCOMPARE(controller->pageCount(), 3);
        QCOMPARE(controller->currentIndex(), 2);
    }

    // L'observateur sert une fois : annulé, rétabli, puis annulé depuis la page rétablie, on reste à la page
    // recalée par le contrôleur (indice 2), jamais à la page de départ.
    void secondUndoAfterRedoDoesNotReturn()
    {
        presentationharness::MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(startOnSecondOfThree(*controller));
        DocumentInsertions insertions(*controller);
        QVERIFY(insertions.insertObjectOnNewPage(squareShape()) != 0);
        controller->undo();
        QTest::qWait(50);
        QCOMPARE(controller->currentIndex(), 1);
        controller->redo();
        QCOMPARE(controller->pageCount(), 4);
        QCOMPARE(controller->currentIndex(), 1);
        controller->nextPage();
        controller->nextPage();
        QCOMPARE(controller->currentIndex(), 3);
        controller->undo();
        QTest::qWait(50);
        QCOMPARE(controller->pageCount(), 3);
        QCOMPARE(controller->currentIndex(), 2);
    }

    // Le retour attend la fin de l'annulation : un observateur appelé après celui du retour lit encore
    // l'indice passé au cache (la page retirée), et aucun changement de page ne part pendant l'annulation.
    void returnWaitsForTheEndOfTheUndo()
    {
        std::vector<int> seen;
        bool undoing = false;
        int changesDuringUndo = 0;
        presentationharness::MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(startOnSecondOfThree(*controller));
        DocumentInsertions insertions(*controller);
        QVERIFY(insertions.insertObjectOnNewPage(squareShape()) != 0);
        const int token = controller->addChangeObserver(
            [&seen, controller](const core::Changes&, DocumentController::ChangeOrigin origin) {
                if (origin == DocumentController::ChangeOrigin::Undo) {
                    seen.push_back(controller->currentIndex());
                }
            });
        const QMetaObject::Connection aboutToChange =
            QObject::connect(controller, &DocumentController::pageAboutToChange, controller,
                             [&undoing, &changesDuringUndo] { changesDuringUndo += undoing ? 1 : 0; });
        undoing = true;
        controller->undo();
        undoing = false;
        controller->removeChangeObserver(token);
        QObject::disconnect(aboutToChange);
        QCOMPARE(seen.size(), std::size_t{1});
        QCOMPARE(seen.front(), 3);
        QCOMPARE(changesDuringUndo, 0);
        QTest::qWait(50);
        QCOMPARE(controller->currentIndex(), 1);
    }

    // Remplacement : un groupe, une étape ; l'annulation rend le trait à son rang.
    void replaceObjectsIsUndoneAtOnce()
    {
        presentationharness::MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        const int before = main.pageObjectCount();
        stylusStroke(main, QPointF(200, 200), QPointF(320, 260));
        QTRY_COMPARE(main.pageObjectCount(), before + 1);
        const core::ObjectId stroke = shown(*controller).objects.back().common.id;
        QVERIFY(std::get_if<core::Stroke>(&core::body(shown(*controller).objects.back())) != nullptr);
        const std::size_t rank = core::indexOf(shown(*controller), stroke);
        DocumentInsertions insertions(*controller);
        std::vector<core::PageObject> added;
        added.push_back(squareShape());
        const std::vector<core::ObjectId> ids = insertions.replaceObjects({stroke}, std::move(added));
        QCOMPARE(ids.size(), std::size_t{1});
        QVERIFY(ids.front() != 0);
        QVERIFY(findObject(shown(*controller), stroke) == nullptr);
        QVERIFY(findObject(shown(*controller), ids.front()) != nullptr);
        QTRY_COMPARE(main.pageObjectCount(), before + 1);
        controller->undo();
        QVERIFY(findObject(shown(*controller), ids.front()) == nullptr);
        QCOMPARE(core::indexOf(shown(*controller), stroke), rank);
        QTRY_COMPARE(main.pageObjectCount(), before + 1);
        // Page affichée d'indice 1 : le remplacement vise ses objets, pas ceux de la page 0.
        QVERIFY(startOnSecondOfThree(*controller));
        const core::ObjectId generated = controller->document().page(1).objects.front().common.id;
        std::vector<core::PageObject> shape;
        shape.push_back(squareShape());
        const std::vector<core::ObjectId> onSecond = insertions.replaceObjects({generated}, std::move(shape));
        QCOMPARE(onSecond.size(), std::size_t{1});
        QVERIFY(findObject(controller->document().page(1), generated) == nullptr);
        QVERIFY(findObject(controller->document().page(1), onSecond.front()) != nullptr);
    }

    // Image d'une ressource neuve : ressource et objet en une étape ; l'annulation retire la ressource
    // (empreinte revenue, fichier sans elle) ; une ressource déjà présente reste.
    void insertImageIsUndoneAtOnce()
    {
        presentationharness::MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        DocumentInsertions insertions(*controller);
        const QByteArray png = pngOf(QColor(200, 30, 30));
        const std::string key = core::sha256Hex(png);
        const QString fingerprint = controller->fingerprintText();
        const int before = main.pageObjectCount();
        QVERIFY(controller->document().resource(key) == nullptr);
        core::ImageObject image;
        image.bounds = core::RectF{100, 100, 64, 64};
        const core::ObjectId id = insertions.insertImage(png, "png", image, false);
        QVERIFY(id != 0);
        QTRY_COMPARE(main.pageObjectCount(), before + 1);
        const core::PageObject* object = findObject(shown(*controller), id);
        QVERIFY(object != nullptr);
        const auto* inserted = std::get_if<core::ImageObject>(&core::body(*object));
        QVERIFY(inserted != nullptr);
        QVERIFY(!inserted->resource.empty());
        QVERIFY(inserted->resource == key);
        QVERIFY(controller->document().resources().count(key) == 1);
        controller->undo();
        QTRY_COMPARE(main.pageObjectCount(), before);
        QVERIFY(findObject(shown(*controller), id) == nullptr);
        QVERIFY(controller->document().resources().count(key) == 0);
        QCOMPARE(controller->fingerprintText(), fingerprint);
        QVERIFY(!jsonHasResource(controller->document(), key));
        controller->redo();
        QVERIFY(findObject(shown(*controller), id) != nullptr);
        QVERIFY(controller->document().resource(key) != nullptr);
        QVERIFY(jsonHasResource(controller->document(), key));
        // Déjà présente : insérée puis annulée, elle reste (l'objet du rétablissement la cite).
        const core::ObjectId again = insertions.insertImage(png, "png", image, false);
        QVERIFY(again != 0 && again != id);
        controller->undo();
        QVERIFY(findObject(shown(*controller), again) == nullptr);
        QVERIFY(findObject(shown(*controller), id) != nullptr);
        QVERIFY(controller->document().resource(key) != nullptr);
        // Sur une page neuve, depuis la page d'indice 1 sur 3 (depuis la page 0, le recalage du contrôleur y
        // ramènerait seul) : page et ressource retirées ensemble, retour à la page de départ.
        QVERIFY(startOnSecondOfThree(*controller));
        const QByteArray other = pngOf(QColor(30, 30, 200));
        const std::string otherKey = core::sha256Hex(other);
        const core::ObjectId onNew = insertions.insertImage(other, "png", image, true);
        QVERIFY(onNew != 0);
        QCOMPARE(controller->pageCount(), 4);
        QCOMPARE(controller->currentIndex(), 3);
        QVERIFY(findObject(controller->document().page(3), onNew) != nullptr);
        QVERIFY(controller->document().resource(otherKey) != nullptr);
        controller->undo();
        QTest::qWait(50);
        QCOMPARE(controller->pageCount(), 3);
        QCOMPARE(controller->currentIndex(), 1);
        QVERIFY(controller->document().resource(otherKey) == nullptr);
    }

    // Réglages de présentation de la page affichée : une étape, aucun redessin du cache (I3).
    void setPagePresentationIsUndoneAtOnce()
    {
        presentationharness::MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        DocumentInsertions insertions(*controller);
        QSignalSpy refreshes(main.pageView(), &actcraft::ui::PageView::cacheRefreshRequested);
        core::PagePresentation value;
        value.curtain = core::CurtainMode::Top;
        value.curtainDistance = 120;
        insertions.setPagePresentation(value);
        QVERIFY(insertions.shownPage() != nullptr);
        QCOMPARE(insertions.shownPage(), &shown(*controller));
        QCOMPARE(insertions.shownIndex(), static_cast<std::size_t>(controller->currentIndex()));
        QVERIFY(insertions.shownPage()->presentation.curtain == core::CurtainMode::Top);
        QCOMPARE(insertions.shownPage()->presentation.curtainDistance, 120.0);
        QVERIFY(controller->canUndo());
        controller->undo();
        QVERIFY(insertions.shownPage()->presentation.curtain == core::CurtainMode::Off);
        QCOMPARE(insertions.shownPage()->presentation.curtainDistance, 0.0);
        QVERIFY(!controller->canUndo());
        QTest::qWait(50);
        QCOMPARE(refreshes.count(), 0);
        // Page affichée d'indice 1 : ses réglages changent, pas ceux de la page 0.
        QVERIFY(startOnSecondOfThree(*controller));
        insertions.setPagePresentation(value);
        QVERIFY(controller->document().page(1).presentation.curtain == core::CurtainMode::Top);
        QVERIFY(controller->document().page(0).presentation.curtain == core::CurtainMode::Off);
    }

    // Observateur des modifications (D-S7) : chaque trait, origine Stroke, avec son identifiant.
    void changeObserverSeesEveryStroke()
    {
        std::vector<std::pair<core::Changes, DocumentController::ChangeOrigin>> calls;
        presentationharness::MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        const int token = controller->addChangeObserver(
            [&calls](const core::Changes& applied, DocumentController::ChangeOrigin origin) {
                calls.emplace_back(applied, origin);
            });
        std::vector<core::ObjectId> strokes;
        for (const qreal y : {200.0, 400.0}) {
            const int before = main.pageObjectCount();
            stylusStroke(main, QPointF(200, y), QPointF(320, y + 50));
            QTRY_COMPARE(main.pageObjectCount(), before + 1);
            strokes.push_back(shown(*controller).objects.back().common.id);
        }
        controller->removeChangeObserver(token);
        QCOMPARE(calls.size(), std::size_t{2});
        for (std::size_t i = 0; i < calls.size(); ++i) {
            QVERIFY(calls[i].second == DocumentController::ChangeOrigin::Stroke);
            const core::Changes& applied = calls[i].first;
            QVERIFY(std::any_of(applied.begin(), applied.end(), [&](const core::Change& change) {
                return change.kind == core::Change::Kind::ObjectInserted && change.object == strokes[i];
            }));
        }
    }

    // Les trois chargements émettent documentReplaced une fois chacun ; l'observateur n'est pas appelé.
    void loadEmitsDocumentReplaced()
    {
        int observed = 0;
        presentationharness::MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        const int token = controller->addChangeObserver(
            [&observed](const core::Changes&, DocumentController::ChangeOrigin) { ++observed; });
        QSignalSpy replaced(controller, &DocumentController::documentReplaced);
        controller->loadGeneratedFirstPage();
        QCOMPARE(replaced.count(), 1);
        replaced.clear();
        controller->loadGeneratedPages(3);
        QCOMPARE(replaced.count(), 1);
        replaced.clear();
        controller->replaceDocument(core::Document(800, 600), QUrl(), QString());
        QCOMPARE(replaced.count(), 1);
        controller->removeChangeObserver(token);
        QCOMPARE(observed, 0);
    }

    // Un trait, annuler, rétablir, une insertion : documentChanged oui, documentReplaced jamais.
    void strokeDoesNotEmitDocumentReplaced()
    {
        presentationharness::MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        QSignalSpy replaced(controller, &DocumentController::documentReplaced);
        QSignalSpy changed(controller, &DocumentController::documentChanged);
        const int before = main.pageObjectCount();
        stylusStroke(main, QPointF(200, 200), QPointF(320, 260));
        QTRY_COMPARE(main.pageObjectCount(), before + 1);
        controller->undo();
        controller->redo();
        DocumentInsertions insertions(*controller);
        QVERIFY(insertions.insertText(QStringLiteral("12"), QRectF(100, 100, 80, 40)) != 0);
        QVERIFY(changed.count() >= 4);
        QCOMPARE(replaced.count(), 0);
    }

    // Non-régression : les identifiants viennent de la cellule remplie par redo(), jamais d'une commande
    // qu'execute() a pu détruire ; rien d'appliqué → rien rendu, pile inchangée.
    void insertReadsIdsOnlyWhenApplied()
    {
        presentationharness::MainWindow main;
        DocumentController* controller = controllerOf(main);
        QVERIFY(controller != nullptr);
        DocumentInsertions insertions(*controller);
        const QString fingerprint = controller->fingerprintText();
        const bool couldUndo = controller->canUndo();
        const std::vector<core::ObjectId> none = insertions.replaceObjects({987654321}, {});
        QVERIFY(none.empty());
        QCOMPARE(controller->canUndo(), couldUndo);
        QCOMPARE(controller->fingerprintText(), fingerprint);
        const core::ObjectId id = insertions.insertObject(squareShape());
        QVERIFY(id != 0);
        QVERIFY(findObject(shown(*controller), id) != nullptr);
        QVERIFY(controller->canUndo());
    }

    void floatingPanelIsACover_data() { presentationharness::addPointerRows(); }

    // Ouvert sur la page, le panneau couvre : un trait sur sa barre ne verse rien, et la barre le déplace ;
    // fermé, le même trait est versé.
    void floatingPanelIsACover()
    {
        QFETCH(Pointer, pointer);
        presentationharness::MainWindow main;
        QVERIFY(main.window() != nullptr);
        QQuickItem* panel = openPanel(main, true);
        QVERIFY(panel != nullptr);
        QObject* drag = panel->findChild<QObject*>(QStringLiteral("floatingPanelDrag"));
        QVERIFY(drag != nullptr);
        const int before = main.pageObjectCount();
        const QPointF start = panel->position();
        const QPointF from = panel->mapToScene(QPointF(100, 22));
        const QPointF to = from + QPointF(60, 40);
        bool activeMidway = false;
        strokeWithProbe(main.window(), pointer, from, to,
                        [&] { activeMidway = drag->property("active").toBool(); });
        QTest::qWait(50);
        QCOMPARE(main.pageObjectCount(), before);
        if (pointer == Pointer::Stylus) {
            QVERIFY2(panel->position() != start, "panneau déplacé au stylet");
        } else {
            QVERIFY2(activeMidway, "barre saisie pendant le trait");
        }
        // Sur le corps, sans gestionnaire : seule la couverture retient le trait (sous la barre, la saisie du
        // DragHandler annule aussi le trait de la souris : sans ce second trait, la ligne souris ne
        // prouverait rien).
        const QPointF bodyFrom = panel->mapToScene(QPointF(30, 70));
        presentationharness::strokeAt(main.window(), pointer, bodyFrom, bodyFrom + QPointF(120, 30));
        QTest::qWait(50);
        QCOMPARE(main.pageObjectCount(), before);
        QVERIFY(QMetaObject::invokeMethod(panel, "close"));
        QVERIFY(!panel->isVisible());
        presentationharness::strokeAt(main.window(), pointer, from, to);
        QTRY_COMPARE(main.pageObjectCount(), before + 1);
    }

    // --- L7-T32 : contrôle du lot ---

    // Chaque commande du bloc du lot 7 de commands.json (lu, non recopié) a un gestionnaire ; les trois
    // commandes de l'annotation du bureau en font partie.
    void everyLot7CommandHasAHandler()
    {
        presentationharness::MainWindow main;
        Commands* commands = commandsOf(main);
        QVERIFY(commands != nullptr);
        const QStringList ids = lot7CommandIds();
        QVERIFY2(ids.size() >= 46, qPrintable(QString::number(ids.size())));
        for (const char* named : {"desktop.page-next", "desktop.page-previous", "desktop.pass-through"}) {
            QVERIFY2(ids.contains(QLatin1String(named)), named);
        }
        for (const QString& id : ids) {
            if (id == QLatin1String("recognition.convert-to-text")) {
                continue; // gestionnaire branché par L7-T34 (menu contextuel) ; hors de ce contrôle
            }
            QVERIFY2(commands->registry().hasHandler(id), qPrintable(id));
        }
    }

    // Sous Android, ni les menus ni la palette ne portent une commande de bureau ou d'écriture.
    void androidMenusHaveNoDesktopCommand()
    {
        QTemporaryDir profile;
        Commands commands;
        commands.setProfileDir(profile.path());
        commands.setPlatform(actcraft::actions::Platform::Windows);
        QStringList onWindows;
        collectIds(commands.menus(), onWindows);
        QVERIFY(onWindows.contains(QStringLiteral("desktop.annotate"))); // le témoin voit ce qu'il doit voir
        commands.setPlatform(actcraft::actions::Platform::Android);
        QStringList ids;
        collectIds(commands.menus(), ids);
        collectIds(commands.palette(), ids);
        QVERIFY(!ids.isEmpty());
        for (const QString& id : std::as_const(ids)) {
            for (const char* forbidden : {"tool.camera-", "tool.keyboard", "desktop.", "tool.handwriting",
                                          "recognition.convert-to-text", "page.background-desktop"}) {
                QVERIFY2(!id.startsWith(QLatin1String(forbidden)), qPrintable(id));
            }
        }
    }

    // Le refus d'un schéma par le navigateur web (message de PresentationCommands) se lit dans le bandeau ;
    // une adresse valide le retire.
    void webBrowserRefusalShowsInTheBanner()
    {
        presentationharness::MainWindow main;
        QVERIFY(main.window() != nullptr);
        auto* pc = main.window()->findChild<actcraft::ui::PresentationCommands*>(
            QStringLiteral("presentationCommands"));
        QVERIFY(pc != nullptr);
        pc->setUrlOpener([](const QUrl&) { return true; });
        QQuickItem* banner = main.item(QStringLiteral("messageBanner"));
        QVERIFY(banner != nullptr);
        QQuickItem* text = presentationharness::findItem(banner, QStringLiteral("messageText"));
        QVERIFY(text != nullptr);
        QVERIFY(!banner->isVisible());
        QSettings().setValue(QStringLiteral("tools/webHome"),
                             QStringLiteral("file:///C:/Windows/notepad.exe"));
        QVERIFY(!pc->openWebHome());
        QVERIFY(!pc->message().isEmpty());
        QTRY_VERIFY(banner->isVisible());
        QCOMPARE(text->property("text").toString(), pc->message());
        QSettings().setValue(QStringLiteral("tools/webHome"), QStringLiteral("https://exemple.test/"));
        QVERIFY(pc->openWebHome());
        QTRY_VERIFY(!banner->isVisible());
    }

    // Les boutons de l'annotation du bureau passent par leurs commandes nommées (témoin :
    // Commands::triggered).
    void desktopButtonsTriggerTheirCommands()
    {
        presentationharness::MainWindow main;
        Commands* commands = commandsOf(main);
        QVERIFY(commands != nullptr);
        auto* desktop =
            main.window()->findChild<actcraft::ui::DesktopController*>(QStringLiteral("desktopController"));
        QVERIFY(desktop != nullptr);
        QVERIFY(main.run(QStringLiteral("desktop.annotate")));
        QTRY_VERIFY(desktop->window() != nullptr);
        QQuickWindow* window = desktop->window();
        QVERIFY(QTest::qWaitForWindowExposed(window));
        QSignalSpy triggered(commands, &Commands::triggered);
        const std::pair<const char*, const char*> buttons[] = {
            {"desktopNextPageButton", "desktop.page-next"},
            {"desktopPageBackButton", "desktop.page-previous"},
            {"desktopPassThroughButton", "desktop.pass-through"}};
        for (const auto& [button, command] : buttons) {
            triggered.clear();
            QObject* item = window->findChild<QObject*>(QString::fromLatin1(button));
            QVERIFY2(item != nullptr, button);
            QVERIFY(QMetaObject::invokeMethod(item, "click"));
            QVERIFY2(triggered.count() >= 1 &&
                         triggered.constFirst().constFirst().toString() == QLatin1String(command),
                     button);
        }
        QVERIFY(desktop->passThrough()); // le dernier bouton a basculé le passage des clics
        desktop->setPassThrough(false);
    }

    // Chaque panneau du lot qui prend l'entrée est une couverture déclarée une fois ouvert : un trait au
    // stylet sous son corps ne verse rien, et le clic de souris sur chacun de ses boutons de palette ou de
    // modèle ne laisse aucun point sur la page.
    void everyInputPanelIsACover()
    {
        const PanelRoot panels[] = {{"tool.dice", "diceRoot"},
                                    {"tool.calculator", "calculatorRoot"},
                                    {"tool.clock", "clockRoot"},
                                    {"insert.equation", "equationRoot"},
                                    {"view.page-effects", "effectsRoot"}};
        for (const PanelRoot& entry : panels) {
            presentationharness::MainWindow main;
            QVERIFY(main.window() != nullptr);
            QVERIFY2(main.run(QLatin1String(entry.command)), entry.command);
            QQuickItem* panel = main.item(QLatin1String(entry.root));
            QVERIFY2(panel != nullptr && panel->isVisible(), entry.root);
            QQuickItem* layer = main.item(QStringLiteral("presentationLayer"));
            // Tout le panneau dans la fenêtre : le coin haut gauche en (300, 10) de la couche.
            panel->setPosition(QPointF(300, 10));
            QVERIFY2(layer->height() >= panel->y() + panel->height(), entry.root);
            QTest::qWait(50);
            const int before = main.pageObjectCount();
            const QPointF body = panel->mapToScene(QPointF(30, panel->height() - 20));
            presentationharness::strokeAt(main.window(), Pointer::Stylus, body, body + QPointF(60, 0));
            QTest::qWait(50);
            QVERIFY2(main.pageObjectCount() == before, entry.root);

            // Les boutons, par leur nom (relus à chaque clic : un clic peut reconstruire la liste).
            QStringList names;
            const QList<QQuickItem*> items = panel->findChildren<QQuickItem*>();
            for (QQuickItem* item : items) {
                if (item->inherits("QQuickAbstractButton") && !item->objectName().isEmpty()) {
                    names << item->objectName();
                }
            }
            QVERIFY2(!names.isEmpty(), entry.root);
            for (const QString& name : std::as_const(names)) {
                if (name == QLatin1String("floatingPanelClose") || name.endsWith(QLatin1String("OkButton")) ||
                    name == QLatin1String("calculatorPlaceButton")) {
                    continue; // fermer ou poser sur la page : le but de ces boutons
                }
                QQuickItem* button = presentationharness::findItem(panel, name);
                if (!button || !button->isVisible() || !button->isEnabled()) {
                    continue;
                }
                const QPointF centre = button->mapToScene(QPointF(button->width() / 2, button->height() / 2));
                // Le bouton est dans les bornes du panneau (la vue teste la couverture sur ces bornes) ; un
                // bouton qui en déborde laisse passer l'appui vers la page.
                QVERIFY2(panel->contains(panel->mapFromScene(centre)),
                         qPrintable(
                             QStringLiteral("%1 : %2 hors du panneau").arg(QLatin1String(entry.root), name)));
                // Avec un léger tremblement de main, comme un vrai clic.
                QTest::mouseMove(main.window(), centre.toPoint());
                QTest::mousePress(main.window(), Qt::LeftButton, Qt::NoModifier, centre.toPoint());
                QTest::mouseMove(main.window(), (centre + QPointF(2, 1)).toPoint());
                QTest::mouseRelease(main.window(), Qt::LeftButton, Qt::NoModifier,
                                    (centre + QPointF(2, 1)).toPoint());
                QTest::qWait(30);
                QVERIFY2(main.pageObjectCount() == before,
                         qPrintable(QStringLiteral("%1 : %2").arg(QLatin1String(entry.root), name)));
            }
        }

        // Surcouches : la partie qui prend l'entrée, un trait au stylet n'y verse rien.
        // Rideau fermé : couvre toute la page (CurtainOverlay.qml:8-10, L7-T3). Zoom de page : couvre toute
        // la zone (PageZoomOverlay.qml:8-9, L7-T5). Spot : la partie masquée couvre, la zone éclairée laisse
        // passer l'encre par conception (SpotlightOverlay.qml:9-12, L7-T4) : le coin de la page est masqué.
        const struct
        {
            const char* command;
            QPointF page;
        } pageOverlays[] = {{"present.curtain", {500, 400}},
                            {"view.page-zoom", {500, 400}},
                            {"present.spotlight-circle", {30, 30}}};
        for (const auto& overlay : pageOverlays) {
            presentationharness::MainWindow main;
            QVERIFY(main.window() != nullptr);
            QVERIFY2(main.run(QLatin1String(overlay.command)), overlay.command);
            QTest::qWait(50);
            const int before = main.pageObjectCount();
            stylusStroke(main, overlay.page, overlay.page + QPointF(80, 0));
            QTest::qWait(50);
            QVERIFY2(main.pageObjectCount() == before, overlay.command);
        }

        // Message défilant : la bande (containmentMask de la racine, TickerOverlay.qml:9-12, L7-T18) couvre.
        {
            presentationharness::MainWindow main;
            QVERIFY(main.window() != nullptr);
            QVERIFY(main.run(QStringLiteral("tool.ticker")));
            QQuickItem* ticker = main.item(QStringLiteral("tickerRoot"));
            QVERIFY(ticker != nullptr);
            QTest::qWait(100);
            QPointF probe;
            for (int y = 4; y < 796 && probe.isNull(); y += 8) {
                if (ticker->contains(ticker->mapFromScene(QPointF(640, y)))) {
                    probe = QPointF(640, y);
                }
            }
            QVERIFY2(!probe.isNull(), "bande du message introuvable");
            const int before = main.pageObjectCount();
            presentationharness::strokeAt(main.window(), Pointer::Stylus, probe, probe + QPointF(80, 0));
            QTest::qWait(50);
            QVERIFY(main.pageObjectCount() == before);
        }

        // Outils de maths : la règle ne dessine aucune encre et le stylo la traverse par conception (guide du
        // trait, MathToolsLayer.qml:4-8, L7-T9) ; la partie qui prend l'entrée est sa zone de déplacement.
        {
            presentationharness::MainWindow main;
            QVERIFY(main.window() != nullptr);
            QVERIFY(main.run(QStringLiteral("math.ruler")));
            QQuickItem* zone = main.item(QStringLiteral("rulerMoveZone"));
            QVERIFY(zone != nullptr);
            QTest::qWait(50);
            const int before = main.pageObjectCount();
            const QPointF centre = zone->mapToScene(QPointF(zone->width() / 2, zone->height() / 2));
            presentationharness::strokeAt(main.window(), Pointer::Stylus, centre, centre + QPointF(40, 30));
            QTest::qWait(50);
            QVERIFY(main.pageObjectCount() == before);
        }
        // Alternatives d'écriture : FloatingPanel (même code que les cinq panneaux ci-dessus), prouvées avec
        // leur reconnaisseur simulé par alternativesBlockTheStylus (tst_ui_recognition.cpp:751). Couche des
        // transitions : sans entrée (aucune couverture, aucun gestionnaire dans TransitionLayer.qml).
    }

    // Stylet posé sur le panneau (couvert) : il ne retient pas la page, un doigt hors du panneau trace.
    void coveredStylusLeavesFingersFree()
    {
        presentationharness::MainWindow main;
        QVERIFY(main.window() != nullptr);
        QQuickItem* panel = openPanel(main);
        QVERIFY(panel != nullptr);
        const int before = main.pageObjectCount();
        const QPointF onPanel = panel->mapToScene(QPointF(100, 90));
        presentationharness::sendTablet(main.window(), QEvent::TabletPress, onPanel);
        presentationharness::strokeAt(main.window(), Pointer::Finger,
                                      main.pageView()->pageToScene(QPointF(650, 600)),
                                      main.pageView()->pageToScene(QPointF(800, 620)));
        QTRY_COMPARE(main.pageObjectCount(), before + 1);
        presentationharness::sendTablet(main.window(), QEvent::TabletRelease, onPanel);
        QTest::qWait(50);
        QCOMPARE(main.pageObjectCount(), before + 1);
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("presentation", TstUiPresentation)
#include "tst_ui_presentation.moc"
