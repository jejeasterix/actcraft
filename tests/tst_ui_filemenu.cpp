#include "startup.h"
#include "support/flipchartwriter.h"

#include <actcraft/core/document.h>
#include <actcraft/formats/nativefile.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documentio.h>
#include <actcraft/ui/documenttabs.h>
#include <actcraft/ui/pageoptions.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QFile>
#include <QImage>
#include <QJSValue>
#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QSemaphore>
#include <QSignalSpy>
#include <QStringList>
#include <QTabletEvent>
#include <QTemporaryDir>
#include <QTest>
#include <QThreadPool>
#include <QtQml/qqmlextensionplugin.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <variant>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

struct Shell
{
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::PageView* view = nullptr;
    render::FixedPageItem* fixed = nullptr;
};

bool ready(Shell& s)
{
    return QTest::qWaitFor([&] { return s.fixed->isReady() && s.view->cacheReady(); }, 10000);
}

bool start(Shell& s)
{
    s.window = loadMainWindow(s.engine, StartupOptions{});
    if (!s.window || !QTest::qWaitForWindowExposed(s.window)) {
        return false;
    }
    s.view = s.window->findChild<ui::PageView*>(QStringLiteral("pageView"));
    s.fixed = s.window->findChild<render::FixedPageItem*>(QStringLiteral("fixedPage"));
    return s.view && s.fixed && ready(s);
}

// Les délégués d'un Repeater n'ont pas toujours la fenêtre pour parent QObject : on suit l'arbre des
// éléments.
QQuickItem* findItem(QQuickItem* root, const QString& name)
{
    if (!root) {
        return nullptr;
    }
    if (root->objectName() == name) {
        return root;
    }
    const QList<QQuickItem*> children = root->childItems();
    for (QQuickItem* child : children) {
        if (QQuickItem* found = findItem(child, name)) {
            return found;
        }
    }
    return nullptr;
}

QQuickItem* item(Shell& s, const char* name)
{
    return findItem(s.window->contentItem(), QString::fromLatin1(name));
}

bool run(Shell& s, const QString& id, const QVariant& argument = QVariant())
{
    QVariant result;
    const bool invoked = QMetaObject::invokeMethod(s.window, "runCommand", Q_RETURN_ARG(QVariant, result),
                                                   Q_ARG(QVariant, QVariant(id)), Q_ARG(QVariant, argument));
    return invoked && result.toBool();
}

ui::DocumentController* controller(Shell& s)
{
    return s.window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
}

ui::DocumentTabs* tabsOf(Shell& s)
{
    return s.window->findChild<ui::DocumentTabs*>(QStringLiteral("appTabs"));
}

bool dialogVisible(Shell& s, const char* name)
{
    QObject* dialog = s.window->findChild<QObject*>(QString::fromLatin1(name));
    return dialog && dialog->property("visible").toBool();
}

QObject* namedObject(Shell& s, const char* name)
{
    return s.window->findChild<QObject*>(QString::fromLatin1(name));
}

double luminance(const QColor& c)
{
    const auto channel = [](double v) {
        v /= 255.0;
        return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * channel(c.red()) + 0.7152 * channel(c.green()) + 0.0722 * channel(c.blue());
}

// Tient tous les fils du pool global jusqu'à release() ou la destruction : une lecture ou une écriture lancée
// entre-temps reste en cours. Les sémaphores sont partagés avec les tâches : aucune ne touche un objet
// détruit.
class PoolHold
{
public:
    PoolHold()
        : m_threads(QThreadPool::globalInstance()->maxThreadCount()),
          m_started(std::make_shared<QSemaphore>()), m_gate(std::make_shared<QSemaphore>())
    {
        for (int i = 0; i < m_threads; ++i) {
            QThreadPool::globalInstance()->start([started = m_started, gate = m_gate] {
                started->release();
                gate->acquire();
            });
        }
    }
    ~PoolHold()
    {
        release();
        QThreadPool::globalInstance()->waitForDone(60000);
    }
    PoolHold(const PoolHold&) = delete;
    PoolHold& operator=(const PoolHold&) = delete;

    bool held() { return m_started->tryAcquire(m_threads, 60000); }
    void release()
    {
        if (!m_released) {
            m_released = true;
            m_gate->release(m_threads);
        }
    }

private:
    int m_threads;
    std::shared_ptr<QSemaphore> m_started;
    std::shared_ptr<QSemaphore> m_gate;
    bool m_released = false;
};

QByteArray fileBytes(const QString& path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

// Nombre de pages du paperboard natif écrit en `path` (0 s'il ne se relit pas).
std::size_t pagesIn(const QString& path)
{
    const formats::DecodeResult r = formats::decodeActcraft(fileBytes(path));
    return r.ok ? r.document.pageCount() : 0;
}

// Paperboard natif de deux pages, écrit sur le fil du test.
bool writeTwoPages(const QString& path)
{
    return ui::saveDocument(core::Document(std::vector<core::Page>(2)), QUrl::fromLocalFile(path)).ok;
}

bool isNullAction(const QVariant& value)
{
    return !value.isValid() || value.isNull() || value.value<QJSValue>().isNull() ||
           value.value<QJSValue>().isUndefined();
}

} // namespace

// Menu Fichier et ses commandes (F-700, L2-T18), sans dialogue de fichier : commandes avec URL.
class TstUiFileMenu : public QObject
{
    Q_OBJECT

private slots:
    void fileMenuListsTheLotTwoCommands()
    {
        Shell s;
        QVERIFY(start(s));
        auto* commands = s.window->findChild<ui::Commands*>(QStringLiteral("commands"));
        QVERIFY(commands);
        QStringList ids;
        for (const QVariant& menu : commands->menus()) {
            const QVariantMap m = menu.toMap();
            if (m.value(QStringLiteral("id")).toString() == QLatin1String("file")) {
                for (const QVariant& entry : m.value(QStringLiteral("items")).toList()) {
                    ids << entry.toMap().value(QStringLiteral("id")).toString();
                }
            }
        }
        for (const char* id :
             {"file.new", "file.new-screen", "file.new-1024x768", "file.new-1152x864", "file.new-1280x1024",
              "file.new-custom", "file.open", "file.save", "file.save-as", "file.close", "file.quit"}) {
            QVERIFY2(ids.contains(QString::fromLatin1(id)), id);
        }
    }

    void fileMenuOpensFromTheTopBar()
    {
        Shell s;
        QVERIFY(start(s));
        QQuickItem* button = item(s, "menu-file");
        QVERIFY(button);
        QVERIFY(QMetaObject::invokeMethod(button, "clicked"));
        QTRY_COMPARE(s.window->property("openPopups").toInt(), 1);
        QTest::qWait(200);
        if (const QString out = qEnvironmentVariable("QT_ACTCRAFT_CAPTURES"); !out.isEmpty()) {
            s.window->grabWindow().save(out + QStringLiteral("/menu-fichier.png"));
        }
    }

    void saveAsThenOpenByCommands()
    {
        Shell s;
        QVERIFY(start(s));
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("essai.actcraft"));
        auto* doc = controller(s);
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(doc->isModified());
        QSignalSpy saved(doc, &ui::DocumentController::saveFinished);
        QVERIFY(run(s, QStringLiteral("file.save-as"), path));
        QVERIFY(saved.wait(10000));
        QVERIFY(!doc->isModified());
        QVERIFY(s.window->title().startsWith(QStringLiteral("essai — ActCraft")));
        QVERIFY(run(s, QStringLiteral("file.close")));
        QCOMPARE(doc->pageCount(), 1);
        QSignalSpy loaded(doc, &ui::DocumentController::loadFinished);
        QVERIFY(run(s, QStringLiteral("file.open"), path));
        QVERIFY(loaded.wait(10000));
        QCOMPARE(doc->pageCount(), 2);
    }

    void modifiedDocumentAsksBeforeClosing()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(run(s, QStringLiteral("file.close")));
        // Un Dialog est un Popup, pas un QQuickItem : item() ne le voit pas.
        QObject* dialog = namedObject(s, "tabSaveChangesDialog");
        QVERIFY(dialog);
        QTRY_VERIFY(dialog->property("visible").toBool());
        QCOMPARE(doc->pageCount(), 2); // rien n'est fermé avant la réponse
        QVERIFY(QMetaObject::invokeMethod(dialog, "discard"));
        QTRY_COMPARE(doc->pageCount(), 1);
        QVERIFY(!doc->isModified());
    }

    void cancelKeepsTheDocument()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(
            run(s, QStringLiteral("file.close"))); // L5-C10 : « Nouveau » n'écrase plus, « Fermer » demande
        QObject* dialog = namedObject(s, "tabSaveChangesDialog"); // L5-C11 : Fermer suit le chemin de la croix
        QVERIFY(dialog);
        QTRY_VERIFY(dialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(dialog, "cancel"));
        QTRY_VERIFY(!dialog->property("visible").toBool());
        QCOMPARE(doc->pageCount(), 2);
        QVERIFY(doc->isModified());
    }

    // L5-C10 (F-010) : « Nouveau » ouvre un onglet, sans rien demander ; le document modifié reste dans le
    // sien.
    void newOpensATabAndKeepsTheModifiedDocument()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        auto* tabs = tabsOf(s);
        QVERIFY(tabs);
        QQuickItem* bar = item(s, "documentTabBar");
        QVERIFY(bar);
        QVERIFY(!bar->isVisible()); // D14 : un seul document, pas de barre
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(run(s, QStringLiteral("file.new-1280x1024")));
        QVERIFY(!dialogVisible(s, "saveChangesDialog"));
        QCOMPARE(tabs->rowCount(), 2);
        QCOMPARE(tabs->currentIndex(), 1);
        QCOMPARE(doc->pageSize(), QSizeF(1280, 1024));
        QCOMPARE(doc->pageCount(), 1);
        QVERIFY(!doc->isModified());
        QVERIFY(tabs->data(tabs->index(0), ui::DocumentTabs::ModifiedRole).toBool());
        QTRY_VERIFY(bar->isVisible()); // D14 : visible dès deux documents ouverts
        tabs->activate(0);
        QCOMPARE(doc->pageCount(), 2);
        QVERIFY(doc->isModified());
    }

    // L5-C10 (F-010, H5-23) : « Ouvrir » ouvre un onglet, sauf sur un « Sans titre » vierge, remplacé.
    void openUsesATabUnlessTheShownDocumentIsBlank()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        auto* tabs = tabsOf(s);
        QVERIFY(tabs);
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("deux.actcraft"));
        QVERIFY(writeTwoPages(path));
        QSignalSpy loaded(doc, &ui::DocumentController::loadFinished);
        QVERIFY(run(s, QStringLiteral("file.open"), path)); // vierge : remplacé dans son onglet
        QVERIFY(loaded.wait(10000));
        QCOMPARE(tabs->rowCount(), 1);
        QCOMPARE(doc->pageCount(), 2);
        QVERIFY(run(s, QStringLiteral("file.open"), path)); // document avec fichier : un onglet
        QVERIFY(loaded.wait(10000));
        QCOMPARE(tabs->rowCount(), 2);
        QCOMPARE(tabs->currentIndex(), 1);
        QVERIFY(run(s, QStringLiteral("page.insert-after")));
        QVERIFY(doc->isModified());
        QVERIFY(run(s, QStringLiteral("file.open"), path)); // modifié : un onglet, sans demande
        QVERIFY(loaded.wait(10000));
        QVERIFY(!dialogVisible(s, "saveChangesDialog"));
        QCOMPARE(tabs->rowCount(), 3);
        QVERIFY(tabs->data(tabs->index(1), ui::DocumentTabs::ModifiedRole).toBool());
    }

    // L5-C10 : fermer un onglet modifié sans titre, « Enregistrer », puis « Enregistrer sous » abandonné : la
    // fermeture est abandonnée (onglet ouvert, rien d'armé) ; un enregistrement plus tard ne le ferme pas.
    void cancelledSaveAsAbandonsTheTabClose()
    {
        Shell s;
        QVERIFY(start(s));
        QTemporaryDir root;
        auto* doc = controller(s);
        doc->setDocumentsRoot(root.path());
        auto* tabs = tabsOf(s);
        QVERIFY(tabs);
        QObject* bar = namedObject(s, "documentTabBar");
        QObject* ask = namedObject(s, "tabSaveChangesDialog");
        QObject* saveDialog = namedObject(s, "saveDialog");
        QVERIFY(bar && ask && saveDialog);
        QVERIFY(run(s, QStringLiteral("page.next")));
        tabs->newTab(); // le document modifié sans titre est garé dans l'onglet 0
        QCOMPARE(tabs->rowCount(), 2);
        tabs->requestClose(0);
        QTRY_VERIFY(ask->property("visible").toBool());
        QCOMPARE(tabs->currentIndex(), 0);
        QVERIFY(QMetaObject::invokeMethod(ask, "save"));
        QVERIFY(bar->property("closeAfterSave").toBool());
        QVERIFY(QMetaObject::invokeMethod(saveDialog, "reject"));
        QTRY_VERIFY(!bar->property("closeAfterSave").toBool());
        QCOMPARE(tabs->rowCount(), 2);
        QSignalSpy saved(doc, &ui::DocumentController::saveFinished);
        QVERIFY(run(s, QStringLiteral("file.save-as"), root.filePath(QStringLiteral("x.actcraft"))));
        QVERIFY(saved.wait(10000));
        QTest::qWait(100);
        QCOMPARE(tabs->rowCount(), 2); // enregistrer plus tard ne ferme pas l'onglet
    }

    void quitAsksBeforeLeavingAModifiedDocument()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(run(s, QStringLiteral("file.quit")));
        QObject* dialog = namedObject(s, "saveChangesDialog");
        QVERIFY(dialog);
        QTRY_VERIFY(dialog->property("visible").toBool());
        QObject* actions = namedObject(s, "fileActions");
        QVERIFY(actions);
        QVERIFY(!actions->property("quitting").toBool()); // on ne quitte pas avant la réponse
        QVERIFY(QMetaObject::invokeMethod(dialog, "cancel"));
        QTRY_VERIFY(!dialog->property("visible").toBool());
        QVERIFY(doc->isModified());
    }

    // L5-C11 : deux onglets modifiés dont un garé : « Quitter » demande deux fois, l'un après l'autre, en
    // basculant vers l'onglet concerné ; « Annuler » à la deuxième abandonne la sortie, rien n'est perdu.
    void quitAsksForEachModifiedTabAndCancelKeepsAll()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        auto* tabs = tabsOf(s);
        QVERIFY(tabs);
        QVERIFY(run(s, QStringLiteral("page.next")));
        tabs->newTab(); // le document modifié est garé dans l'onglet 0
        QVERIFY(run(s, QStringLiteral("page.next")));
        QCOMPARE(tabs->currentIndex(), 1);
        QObject* dialog = namedObject(s, "saveChangesDialog");
        QObject* actions = namedObject(s, "fileActions");
        QVERIFY(dialog && actions);
        QVERIFY(run(s, QStringLiteral("file.quit")));
        QTRY_VERIFY(dialog->property("visible").toBool());
        QCOMPARE(tabs->currentIndex(), 0); // l'onglet garé est montré avant la demande
        QVERIFY(QMetaObject::invokeMethod(dialog, "discard")); // « Ne pas enregistrer » : le suivant
        QTRY_VERIFY(dialog->property("visible").toBool());
        QCOMPARE(tabs->currentIndex(), 1);
        QVERIFY(!actions->property("quitting").toBool());
        QVERIFY(QMetaObject::invokeMethod(dialog, "cancel"));
        QTRY_VERIFY(!dialog->property("visible").toBool());
        QTest::qWait(50);
        QVERIFY(!actions->property("quitting").toBool());
        QCOMPARE(tabs->rowCount(), 2);
        QVERIFY(tabs->data(tabs->index(0), ui::DocumentTabs::ModifiedRole).toBool());
        QVERIFY(tabs->data(tabs->index(1), ui::DocumentTabs::ModifiedRole).toBool());
        QVERIFY(doc->isModified());
    }

    // L5-C11 : seul un onglet garé est modifié (l'affiché est propre) : « Quitter » le demande quand même.
    void quitAsksForAParkedModifiedTab()
    {
        Shell s;
        QVERIFY(start(s));
        auto* tabs = tabsOf(s);
        QVERIFY(tabs);
        QVERIFY(run(s, QStringLiteral("page.next")));
        tabs->newTab();
        QVERIFY(!controller(s)->isModified());
        QObject* dialog = namedObject(s, "saveChangesDialog");
        QObject* actions = namedObject(s, "fileActions");
        QVERIFY(dialog && actions);
        QVERIFY(run(s, QStringLiteral("file.quit")));
        QTRY_VERIFY(dialog->property("visible").toBool());
        QCOMPARE(tabs->currentIndex(), 0);
        QVERIFY(QMetaObject::invokeMethod(dialog, "cancel"));
        QTRY_VERIFY(!dialog->property("visible").toBool());
        QVERIFY(!actions->property("quitting").toBool());
        QVERIFY(tabs->data(tabs->index(0), ui::DocumentTabs::ModifiedRole).toBool());
    }

    // L5-C11 : la fermeture de la fenêtre passe par la même demande (onglet garé modifié).
    void closingTheWindowAsksForAParkedModifiedTab()
    {
        Shell s;
        QVERIFY(start(s));
        auto* tabs = tabsOf(s);
        QVERIFY(tabs);
        QVERIFY(run(s, QStringLiteral("page.next")));
        tabs->newTab();
        QObject* dialog = namedObject(s, "saveChangesDialog");
        QVERIFY(dialog);
        s.window->close();
        QTRY_VERIFY(dialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(dialog, "cancel"));
        QTRY_VERIFY(!dialog->property("visible").toBool());
        QVERIFY(tabs->data(tabs->index(0), ui::DocumentTabs::ModifiedRole).toBool());
    }

    // L5-C11 : « Enregistrer » d'un sans-titre ouvre « Enregistrer sous » ; son annulation abandonne la sortie.
    void quitSaveAsCancelledAbandonsTheQuit()
    {
        Shell s;
        QVERIFY(start(s));
        QTemporaryDir root;
        auto* doc = controller(s);
        doc->setDocumentsRoot(root.path());
        QVERIFY(run(s, QStringLiteral("page.next")));
        QObject* dialog = namedObject(s, "saveChangesDialog");
        QObject* saveDialog = namedObject(s, "saveDialog");
        QObject* actions = namedObject(s, "fileActions");
        QVERIFY(dialog && saveDialog && actions);
        QVERIFY(run(s, QStringLiteral("file.quit")));
        QTRY_VERIFY(dialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(dialog, "save"));
        QVERIFY(QMetaObject::invokeMethod(saveDialog, "reject"));
        QTest::qWait(50);
        QVERIFY(!actions->property("quitting").toBool());
        QVERIFY(doc->isModified());
    }

    // L5-C11 : « Fermer » ferme l'onglet affiché, comme sa croix ; le dernier devient un « Sans titre » vierge.
    void closeClosesTheShownTab()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        auto* tabs = tabsOf(s);
        QVERIFY(tabs);
        tabs->newTab();
        QCOMPARE(tabs->rowCount(), 2);
        QVERIFY(run(s, QStringLiteral("file.close")));
        QCOMPARE(tabs->rowCount(), 1); // onglet propre : fermé sans demande
        QVERIFY(!dialogVisible(s, "tabSaveChangesDialog"));
        QVERIFY(run(s, QStringLiteral("page.next"))); // un seul onglet, modifié
        QVERIFY(run(s, QStringLiteral("file.close")));
        QObject* dialog = namedObject(s, "tabSaveChangesDialog");
        QVERIFY(dialog);
        QTRY_VERIFY(dialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(dialog, "discard"));
        QTRY_COMPARE(doc->pageCount(), 1);
        QCOMPARE(tabs->rowCount(), 1); // jamais zéro onglet
        QVERIFY(!doc->isModified());
        QVERIFY(doc->fileUrl().isEmpty());
    }

    // L5-C11 : « Fermer » sur un onglet modifié d'un jeu de deux : la demande, puis l'onglet est fermé.
    void closeAskedForAModifiedTabAmongTwo()
    {
        Shell s;
        QVERIFY(start(s));
        auto* tabs = tabsOf(s);
        QVERIFY(tabs);
        tabs->newTab();
        QVERIFY(run(s, QStringLiteral("page.next"))); // onglet 1 modifié
        QVERIFY(run(s, QStringLiteral("file.close")));
        QObject* dialog = namedObject(s, "tabSaveChangesDialog");
        QVERIFY(dialog);
        QTRY_VERIFY(dialog->property("visible").toBool());
        QCOMPARE(tabs->rowCount(), 2); // rien n'est fermé avant la réponse
        QVERIFY(QMetaObject::invokeMethod(dialog, "discard"));
        QTRY_COMPARE(tabs->rowCount(), 1);
    }

    // L5-C11 : les messages de L5-T25 (échec ou fin d'un échange de fichier) arrivent à la bannière.
    void exchangeMessagesReachTheBanner()
    {
        Shell s;
        QVERIFY(start(s));
        auto* exchange = s.window->findChild<QObject*>(QStringLiteral("appExchange"));
        QQuickItem* banner = item(s, "messageBanner");
        QQuickItem* text = item(s, "messageText");
        QVERIFY(exchange && banner && text);
        QVERIFY(!banner->isVisible());
        QVERIFY(QMetaObject::invokeMethod(exchange, "failed", Q_ARG(QString, QStringLiteral("export échoué"))));
        QTRY_VERIFY(banner->isVisible());
        QCOMPARE(text->property("text").toString(), QStringLiteral("export échoué"));
        QVERIFY(QMetaObject::invokeMethod(exchange, "finished", Q_ARG(QString, QStringLiteral("export fini"))));
        QTRY_COMPARE(text->property("text").toString(), QStringLiteral("export fini"));
    }

    // L2-C9, constat 5 : « Quitter » pendant un enregistrement, puis « Enregistrer » : l'enregistrement
    // demandé attend la fin de celui en cours, puis « Quitter » reprend. Avant : refusé (occupé) sans
    // erreur, et l'action en attente oubliée.
    void quitDuringASaveWaitsForItThenQuits()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("cours.actcraft"));
        QSignalSpy saved(doc, &ui::DocumentController::saveFinished);
        QVERIFY(run(s, QStringLiteral("file.save-as"), path));
        QVERIFY(saved.wait(10000));
        QVERIFY(!doc->isModified());
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(doc->isModified());
        PoolHold pool; // l'enregistrement lancé reste en cours tant que le test ne rend pas les fils
        QVERIFY(pool.held());
        QVERIFY(run(s, QStringLiteral("file.save"))); // écrit 2 pages
        QVERIFY(doc->isBusy());
        // Revue de L2-C9, mineur 2 : une page de plus PENDANT l'écriture. Seul un second enregistrement
        // l'écrit : une version qui quitterait après le premier laisserait 2 pages dans le fichier.
        QVERIFY(run(s, QStringLiteral("page.next")));
        QCOMPARE(doc->pageCount(), 3);
        QVERIFY(run(s, QStringLiteral("file.quit")));
        QObject* ask = namedObject(s, "saveChangesDialog");
        QVERIFY(ask);
        QTRY_VERIFY(ask->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(ask, "save"));
        QObject* actions = namedObject(s, "fileActions");
        QVERIFY(actions);
        QTest::qWait(50);                                 // un refus serait livré ici, en file
        QVERIFY(!actions->property("quitting").toBool()); // pas avant la fin des enregistrements
        pool.release();
        QTRY_VERIFY_WITH_TIMEOUT(actions->property("quitting").toBool(), 20000);
        QCOMPARE(pagesIn(path), std::size_t{3}); // le second enregistrement a écrit l'écran
        QVERIFY(!doc->isModified());
        QVERIFY(doc->lastError().isEmpty());
    }

    // Revue de L2-C9, mineur 4 : rien n'a changé pendant l'écriture en cours, le document est propre à sa fin
    // : « Quitter » reprend sans une seconde écriture inutile.
    void quitDuringASaveOfTheShownContentWritesOnce()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("cours.actcraft"));
        QSignalSpy saved(doc, &ui::DocumentController::saveFinished);
        QVERIFY(run(s, QStringLiteral("file.save-as"), path));
        QVERIFY(saved.wait(10000));
        QVERIFY(run(s, QStringLiteral("page.next")));
        PoolHold pool;
        QVERIFY(pool.held());
        QVERIFY(run(s, QStringLiteral("file.save")));
        QVERIFY(run(s, QStringLiteral("file.quit")));
        QObject* ask = namedObject(s, "saveChangesDialog");
        QObject* actions = namedObject(s, "fileActions");
        QVERIFY(ask && actions);
        QTRY_VERIFY(ask->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(ask, "save"));
        pool.release();
        QTRY_VERIFY_WITH_TIMEOUT(actions->property("quitting").toBool(), 20000);
        QTest::qWait(300);          // une seconde écriture finirait ici
        QCOMPARE(saved.count(), 2); // « Enregistrer sous », puis l'enregistrement en cours : pas de troisième
        QCOMPARE(pagesIn(path), std::size_t{2});
        QVERIFY(!doc->isModified());
    }

    // Revue de L2-C9, important 1 : « Enregistrer » demandé pendant une ouverture qui RÉUSSIT visait le
    // document remplacé ; il est abandonné, jamais relancé sur le document ouvert (qui serait réécrit sans
    // demande, avec la perte de ce que la lecture a écarté).
    void saveRequestedDuringASuccessfulOpenIsDropped()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("deux.actcraft"));
        QVERIFY(writeTwoPages(path));
        const QByteArray before = fileBytes(path);
        QVERIFY(!doc->isModified());
        QSignalSpy saved(doc, &ui::DocumentController::saveFinished);
        QSignalSpy loaded(doc, &ui::DocumentController::loadFinished);
        PoolHold pool;
        QVERIFY(pool.held());
        QVERIFY(run(s, QStringLiteral("file.open"), path));
        QVERIFY(doc->isBusy());
        QVERIFY(run(s, QStringLiteral("file.save"))); // Ctrl+S pendant la lecture
        pool.release();
        QVERIFY(loaded.wait(20000));
        QCOMPARE(loaded.first().at(0).toBool(), true);
        QTest::qWait(300); // un enregistrement relancé partirait et finirait ici
        QCOMPARE(saved.count(), 0);
        QCOMPARE(fileBytes(path), before);
        QCOMPARE(doc->pageCount(), 2);
    }

    // L5-C10 : « Ouvrir » sur un document modifié ouvre un onglet, sans demande : plus de « Quitter » pendant
    // une ouverture qui écarte les modifications. « Enregistrer » demandé pendant la lecture visait l'onglet
    // chargé : il est abandonné à la fin d'une ouverture réussie, rien n'est écrit ; l'onglet modifié reste.
    void saveRequestedDuringAnOpenInANewTabIsDropped()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        auto* tabs = tabsOf(s);
        QVERIFY(tabs);
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("deux.actcraft"));
        QVERIFY(writeTwoPages(path));
        const QByteArray before = fileBytes(path);
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(doc->isModified());
        QSignalSpy saved(doc, &ui::DocumentController::saveFinished);
        QSignalSpy loaded(doc, &ui::DocumentController::loadFinished);
        PoolHold pool;
        QVERIFY(pool.held());
        QVERIFY(run(s, QStringLiteral("file.open"), path));
        QVERIFY(!dialogVisible(s, "saveChangesDialog"));
        QCOMPARE(tabs->rowCount(), 2);
        QVERIFY(doc->isBusy());
        QVERIFY(run(s, QStringLiteral("file.save"))); // Ctrl+S pendant la lecture
        pool.release();
        QVERIFY(loaded.wait(20000));
        QCOMPARE(loaded.first().at(0).toBool(), true);
        QTest::qWait(300);
        QCOMPARE(saved.count(), 0);
        QCOMPARE(fileBytes(path), before);
        QCOMPARE(doc->pageCount(), 2);
        QVERIFY(tabs->data(tabs->index(0), ui::DocumentTabs::ModifiedRole).toBool());
    }

    void newSizesComeFromTheirCommands()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QVERIFY(run(s, QStringLiteral("file.new-1280x1024")));
        QCOMPARE(doc->pageSize(), QSizeF(1280, 1024));
        QVERIFY(run(s, QStringLiteral("file.new-1152x864")));
        QCOMPARE(doc->pageSize(), QSizeF(1152, 864));
        QVERIFY(run(s, QStringLiteral("file.new-screen")));
        QCOMPARE(doc->pageSize(), QSizeF(s.window->screen()->size()));
        QVERIFY(run(s, QStringLiteral("file.new-custom")));
        QObject* sizeDialog = namedObject(s, "pageSizeDialog");
        QVERIFY(sizeDialog);
        QTRY_VERIFY(sizeDialog->property("visible").toBool());
        QVERIFY(
            QMetaObject::invokeMethod(sizeDialog, "acceptSize", Q_ARG(QVariant, 640), Q_ARG(QVariant, 480)));
        QTRY_COMPARE(doc->pageSize(), QSizeF(640, 480));
    }

    void customSizeTakesFrames() // L4-C1 (D16)
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QVERIFY(run(s, QStringLiteral("file.new-custom")));
        QObject* sizeDialog = namedObject(s, "pageSizeDialog");
        QVERIFY(sizeDialog);
        QTRY_VERIFY(sizeDialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(sizeDialog, "acceptFramedSize", Q_ARG(QVariant, 400),
                                          Q_ARG(QVariant, 300), Q_ARG(QVariant, 2), Q_ARG(QVariant, 3)));
        // Amendement 5 (L4-O2) : un cadre de 400 x 300 devient 500 x 500 (H4-20) ; page = 2 x 3 cadres.
        QCOMPARE(namedObject(s, "pageWidthBox")->property("value").toInt(), 500);
        QCOMPARE(namedObject(s, "pageHeightBox")->property("value").toInt(), 500);
        QTRY_COMPARE(doc->pageSize(), QSizeF(1000, 1500));
        QCOMPARE(doc->document().page(0).zones.horizontal, 2);
        QCOMPARE(doc->document().page(0).zones.vertical, 3);
        QCOMPARE(doc->document().page(0).zones.frameWidth, 500.0); // D11 : taille d'un cadre stockée
        QCOMPARE(doc->document().page(0).zones.frameHeight, 500.0);
        QVERIFY(!doc->canUndo());
        QVERIFY(!doc->isModified());
        // Le paperboard neuf montre ses séparations : réglage « 2x3 » (2 + 3 cadres : 1 + 2 séparations).
        auto* options = s.window->findChild<ui::PageOptions*>(QStringLiteral("appPageSettings"));
        QVERIFY(options);
        QTRY_COMPARE(options->zones(), QStringLiteral("2x3"));
        QQuickItem* overlay = item(s, "pageZones");
        QVERIFY(overlay);
        QTRY_COMPARE(overlay->property("separatorCount").toInt(), 3);
        QVERIFY(
            QMetaObject::invokeMethod(sizeDialog, "acceptSize", Q_ARG(QVariant, 640), Q_ARG(QVariant, 480)));
        QTRY_COMPARE(doc->pageSize(), QSizeF(640, 480)); // sans cadre : aucune zone, minimum du lot 1 (200)
        QCOMPARE(doc->document().page(0).zones.frameWidth, 0.0);
        QCOMPARE(doc->document().page(0).zones.horizontal, 0);
        doc->newDocumentWithFrames(4000, 300, 3,
                                   0); // cadre de 4000 x 500 (H4-20) ; 3 x 4000 dépasse 8000 : 2 cadres
        QCOMPARE(doc->pageSize(), QSizeF(8000, 500));
        QCOMPARE(doc->document().page(0).zones.horizontal, 2);
    }

    // Plafond défini une fois (maxPageSide) ; les champs de cadres suivent la taille d'un cadre, sans rien
    // ramener en silence : à 500 px par cadre, 16 cadres au plus.
    void frameBoxesFollowTheCeiling()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QCOMPARE(doc->property("maxPageSide").toDouble(), 8000.0);
        QCOMPARE(doc->property("minFrameSide").toDouble(), 500.0);
        QVERIFY(run(s, QStringLiteral("file.new-custom")));
        QObject* sizeDialog = namedObject(s, "pageSizeDialog");
        QVERIFY(sizeDialog);
        QTRY_VERIFY(sizeDialog->property("visible").toBool());
        QObject* width = namedObject(s, "pageWidthBox");
        QObject* height = namedObject(s, "pageHeightBox");
        QObject* across = namedObject(s, "pageFramesAcrossBox");
        QObject* down = namedObject(s, "pageFramesDownBox");
        QVERIFY(width && height && across && down);
        QVERIFY(width->setProperty("value", 500));
        QVERIFY(height->setProperty("value", 500));
        QTRY_COMPARE(across->property("to").toInt(), 16);
        QTRY_COMPARE(down->property("to").toInt(), 16);
        QVERIFY(across->setProperty("value", 16));
        QCOMPARE(across->property("value").toInt(), 16);
        QTRY_COMPARE(width->property("to").toInt(), 500); // 16 cadres : la largeur ne peut plus grandir
        QVERIFY(width->setProperty("value", 2000));
        QCOMPARE(width->property("value").toInt(), 500); // refusée, visible : jamais ramenée après coup
        QCOMPARE(across->property("value").toInt(), 16);
    }

    // Un cadre plus grand que le plafond est ramené au plafond avant le calcul : page = cadres x cadre.
    void oversizedFrameIsCapped()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        doc->newDocumentWithFrames(10000, 600, 1, 1);
        QCOMPARE(doc->pageSize(), QSizeF(8000, 600));
        QCOMPARE(doc->document().page(0).zones.frameWidth, 8000.0);
        QCOMPARE(doc->document().page(0).zones.horizontal, 1);
        QCOMPARE(doc->document().page(0).zones.vertical, 1);
    }

    // L4-C19 : les tailles se posent avant les nombres de cadres, le plafond suit la nouvelle largeur
    // (1024 par défaut aurait borné à 7 cadres).
    void framedSizeCeilingFollowsTheNewSize()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QVERIFY(run(s, QStringLiteral("file.new-custom")));
        QObject* sizeDialog = namedObject(s, "pageSizeDialog");
        QVERIFY(sizeDialog);
        QTRY_VERIFY(sizeDialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(sizeDialog, "acceptFramedSize", Q_ARG(QVariant, 500),
                                          Q_ARG(QVariant, 500), Q_ARG(QVariant, 10), Q_ARG(QVariant, 10)));
        QCOMPARE(namedObject(s, "pageFramesAcrossBox")->property("value").toInt(), 10);
        QTRY_COMPARE(doc->document().page(0).zones.horizontal, 10);
    }

    // Même règle que « Nouveau » (L5-C10) : la taille avec cadres ouvre un onglet sans rien demander, le
    // document modifié reste dans le sien.
    void framedSizeOpensATabAndKeepsTheModifiedDocument()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(doc->isModified());
        const QSizeF before = doc->pageSize();
        QVERIFY(run(s, QStringLiteral("file.new-custom")));
        QObject* sizeDialog = namedObject(s, "pageSizeDialog");
        QObject* ask = namedObject(s, "saveChangesDialog");
        QVERIFY(sizeDialog && ask);
        QTRY_VERIFY(sizeDialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(sizeDialog, "acceptFramedSize", Q_ARG(QVariant, 500),
                                          Q_ARG(QVariant, 500), Q_ARG(QVariant, 2), Q_ARG(QVariant, 3)));
        QTRY_COMPARE(doc->pageSize(), QSizeF(1000, 1500));
        QVERIFY(!ask->property("visible").toBool());
        QVERIFY(!doc->isModified());
        auto* tabs = tabsOf(s);
        QVERIFY(tabs);
        QCOMPARE(tabs->rowCount(), 2);
        QVERIFY(tabs->data(tabs->index(0), ui::DocumentTabs::ModifiedRole).toBool());
        tabs->activate(0);
        QCOMPARE(doc->pageSize(), before);
        QVERIFY(doc->isModified());
    }

    // « Valider » sans rien changer : le paperboard du lot 1, sans zones, comme newDocument.
    void validatingTheDefaultsMakesNoFrames()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QVERIFY(run(s, QStringLiteral("file.new-custom")));
        QObject* sizeDialog = namedObject(s, "pageSizeDialog");
        QVERIFY(sizeDialog);
        QTRY_VERIFY(sizeDialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(sizeDialog, "accept"));
        QTRY_VERIFY(!sizeDialog->property("visible").toBool());
        QCOMPARE(doc->pageSize(), QSizeF(1024, 768));
        QVERIFY(core::isDefault(doc->document().page(0).zones));
        QVERIFY(!doc->canUndo());
        QVERIFY(!doc->isModified());
        // Une valeur lue est rendue telle quelle ; négatifs et excès sont ramenés, sans rien bloquer.
        doc->newDocumentWithFrames(1000, 800, -3, -1);
        QCOMPARE(doc->pageSize(), QSizeF(1000, 800));
        QVERIFY(core::isDefault(doc->document().page(0).zones));
        doc->newDocumentWithFrames(300, 200, 0, 2); // un seul sens en cadres : cadres de 500 x 500
        QCOMPARE(doc->pageSize(), QSizeF(500, 1000));
        QCOMPARE(doc->document().page(0).zones.horizontal, 0);
        QCOMPARE(doc->document().page(0).zones.vertical, 2);
        doc->newDocumentWithFrames(500, 500, 99, 99); // 40 x 53 au plus, puis 8000 par côté : 16 x 16
        QCOMPARE(doc->pageSize(), QSizeF(8000, 8000));
        QCOMPARE(doc->document().page(0).zones.horizontal, 16);
        QCOMPARE(doc->document().page(0).zones.vertical, 16);
    }

    // B1 : boîte ouverte, le stylet posé sur un de ses champs ne pose pas d'encre dessous.
    void sizeDialogBlocksTheStylus()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QVERIFY(run(s, QStringLiteral("file.new-custom")));
        QObject* sizeDialog = namedObject(s, "pageSizeDialog");
        QVERIFY(sizeDialog);
        QTRY_VERIFY(sizeDialog->property("visible").toBool());
        QTRY_VERIFY(s.view->inputBlocked());
        auto* box = qobject_cast<QQuickItem*>(namedObject(s, "pageFramesAcrossBox"));
        QVERIFY(box);
        const QPointF at = box->mapToScene(QPointF(box->width() / 2, box->height() / 2));
        QPointingDevice stylus(QStringLiteral("tst-taille-stylet"), 5162, QInputDevice::DeviceType::Stylus,
                               QPointingDevice::PointerType::Pen,
                               QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 0);
        const auto stroke = [&] {
            const auto send = [&](QEvent::Type type, QPointF scene, Qt::MouseButtons buttons) {
                QTabletEvent event(type, &stylus, scene, s.window->mapToGlobal(scene), buttons ? 0.5 : 0.0, 0,
                                   0, 0, 0, 0, Qt::NoModifier, Qt::LeftButton, buttons);
                QCoreApplication::sendEvent(s.window, &event);
            };
            send(QEvent::TabletPress, at, Qt::LeftButton);
            for (int i = 1; i <= 3; ++i) {
                send(QEvent::TabletMove, at + QPointF(i * 6, 0), Qt::LeftButton);
            }
            send(QEvent::TabletRelease, at + QPointF(18, 0), Qt::NoButton);
        };
        s.view->tools()->activate(QStringLiteral("pen"));
        const std::size_t before = doc->document().page(0).objects.size();
        stroke();
        QCOMPARE(doc->document().page(0).objects.size(), before);
        // Témoin : boîte fermée, le même geste pose bien de l'encre.
        QVERIFY(QMetaObject::invokeMethod(sizeDialog, "close"));
        QTRY_VERIFY(!s.view->inputBlocked());
        stroke();
        QTRY_VERIFY(doc->document().page(0).objects.size() > before);
    }

    // L4-C18 : les menus du navigateur de pages recouvrent la vue : ouverts, ils bloquent le stylet (contrat
    // du lot).
    void navigatorMenusBlockTheStylus_data()
    {
        QTest::addColumn<QByteArray>("menuName");
        QTest::newRow("menu de page") << QByteArray("pageNavigatorMenu");
        QTest::newRow("menu du navigateur") << QByteArray("pageNavigatorOwnMenu");
    }

    void navigatorMenusBlockTheStylus()
    {
        QFETCH(QByteArray, menuName);
        Shell s;
        QVERIFY(start(s));
        QVERIFY(run(s, QStringLiteral("view.browsers")));
        QObject* menu = namedObject(s, menuName.constData());
        QVERIFY(menu);
        QVERIFY(!s.view->inputBlocked());
        QVERIFY(QMetaObject::invokeMethod(menu, "open"));
        QTRY_VERIFY(menu->property("visible").toBool());
        QTRY_VERIFY(s.view->inputBlocked());
        QVERIFY(QMetaObject::invokeMethod(menu, "close"));
        QTRY_VERIFY(!menu->property("visible").toBool());
        QTRY_VERIFY(!s.view->inputBlocked());
    }

    // L4-C21 : le menu contextuel de la page recouvre la vue : ouvert, il bloque le stylet (contrat du lot).
    void pageContextMenuBlocksTheStylus()
    {
        Shell s;
        QVERIFY(start(s));
        QObject* menu = namedObject(s, "pageContextMenu");
        QVERIFY(menu);
        QVERIFY(!s.view->inputBlocked());
        QVERIFY(QMetaObject::invokeMethod(menu, "popupAt", Q_ARG(QVariant, 200), Q_ARG(QVariant, 200)));
        QTRY_VERIFY(menu->property("visible").toBool());
        QTRY_VERIFY(s.view->inputBlocked());
        QVERIFY(QMetaObject::invokeMethod(menu, "close"));
        QTRY_VERIFY(!menu->property("visible").toBool());
        QTRY_VERIFY(!s.view->inputBlocked());
    }

    // L4-C18 : un lâcher sur la moitié basse de la dernière miniature créée, des pages suivant, insère après
    // elle (et non en fin de document).
    void dropOnLowerHalfOfLastCreatedThumbnail()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(run(s, QStringLiteral("view.browsers")));
        for (int i = 0; i < 11; ++i) {
            QVERIFY(run(s, QStringLiteral("page.insert-after")));
        }
        QVERIFY(run(s, QStringLiteral("page.goto"), QVariant(1)));
        auto* list = qobject_cast<QQuickItem*>(namedObject(s, "pageList"));
        QObject* area = namedObject(s, "pageListArea");
        QVERIFY(list && area);
        const int count = list->property("count").toInt();
        QCOMPARE(count, 12);
        s.window->resize(1280, 800);
        QTRY_VERIFY(list->width() > 100 && list->height() > 200);
        QTRY_VERIFY(list->property("contentHeight").toDouble() > list->height());
        int last = -1;
        QQuickItem* cell = nullptr;
        for (int i = 0; i < count; ++i) {
            QQuickItem* item = nullptr;
            QVERIFY(QMetaObject::invokeMethod(list, "itemAtIndex", Q_RETURN_ARG(QQuickItem*, item),
                                              Q_ARG(int, i)));
            if (item) {
                last = i;
                cell = item;
            }
        }
        QVERIFY(last >= 0 && last < count - 1);
        QVERIFY(cell);
        const double contentY = list->property("contentY").toDouble();
        const double y = cell->y() + cell->height() * 0.75 - contentY;
        QVERIFY(QMetaObject::invokeMethod(area, "locate", Q_ARG(QVariant, QVariant(y))));
        QCOMPARE(area->property("before").toInt(), last + 1);
    }

    void bannerShowsReadingWarnings()
    {
        Shell s;
        QVERIFY(start(s));
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("cours.flipchart"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(fctest::sampleFlipchart()); // une formule non lue : un avertissement
        file.close();
        auto* doc = controller(s);
        QSignalSpy loaded(doc, &ui::DocumentController::loadFinished);
        QVERIFY(run(s, QStringLiteral("file.open"), path));
        QVERIFY(loaded.wait(10000));
        QQuickItem* banner = item(s, "messageBanner");
        QQuickItem* text = item(s, "messageText");
        QVERIFY(banner && text);
        QTRY_VERIFY(banner->isVisible());
        QVERIFY(text->property("text").toString().contains(QStringLiteral("1 avertissement")));
        QVERIFY(item(s, "messageClose"));
        // Contraste sur les couleurs RENDUES (leçon L1-C5) : fond lu dans la capture, texte lu sur l'élément.
        QTest::qWait(100);
        const QImage shot = s.window->grabWindow();
        const QPointF inside = banner->mapToScene(QPointF(4, banner->height() / 2));
        const QColor background = shot.pixelColor((inside * s.window->effectiveDevicePixelRatio()).toPoint());
        QVERIFY(qAbs(background.red() - banner->property("color").value<QColor>().red()) <= 2);
        QVERIFY(qAbs(background.blue() - banner->property("color").value<QColor>().blue()) <= 2);
        const QColor ink = text->property("color").value<QColor>();
        const double a = luminance(background);
        const double b = luminance(ink);
        const double ratio = (std::max(a, b) + 0.05) / (std::min(a, b) + 0.05);
        QVERIFY2(ratio >= 4.5, qPrintable(QString::number(ratio)));
        if (const QString out = qEnvironmentVariable("QT_ACTCRAFT_CAPTURES"); !out.isEmpty()) {
            shot.save(out + QStringLiteral("/bandeau.png"));
        }
        QVERIFY(QMetaObject::invokeMethod(banner, "dismiss"));
        QTRY_VERIFY(!banner->isVisible());
    }

    // Sécurité : un avertissement ou un nom de fichier venu d'un fichier ouvert reste du texte brut.
    void messagesAreShownAsPlainText()
    {
        Shell s;
        QVERIFY(start(s));
        QQuickItem* banner = item(s, "messageBanner");
        QQuickItem* text = item(s, "messageText");
        QVERIFY(banner && text);
        const QString hostile = QStringLiteral("<img src=\"file:///C:/secret.png\"><b>gras</b>");
        QVERIFY(QMetaObject::invokeMethod(banner, "show", Q_ARG(QVariant, hostile)));
        QCOMPARE(text->property("text").toString(), hostile);
        QCOMPARE(text->property("textFormat").toInt(), int(Qt::PlainText));
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(run(s, QStringLiteral("file.close")));
        QObject* ask = namedObject(s, "tabSaveChangesDialog");
        QVERIFY(ask);
        QObject* message = ask->findChild<QObject*>(QStringLiteral("saveChangesText"));
        QVERIFY(message);
        QCOMPARE(message->property("textFormat").toInt(), int(Qt::PlainText));
    }

    void cancelledSaveAsForgetsThePendingAction()
    {
        Shell s;
        QVERIFY(start(s));
        QTemporaryDir root; // le dialogue crée le dossier par défaut : jamais dans le profil de l'utilisateur
        auto* doc = controller(s);
        doc->setDocumentsRoot(root.path());
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(run(s, QStringLiteral("file.close")));
        QObject* ask = namedObject(s, "tabSaveChangesDialog");
        QVERIFY(ask);
        QTRY_VERIFY(ask->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(ask, "save")); // document sans fichier : « Enregistrer sous »
        QObject* saveDialog = namedObject(s, "saveDialog");
        QObject* bar = namedObject(s, "documentTabBar");
        QVERIFY(saveDialog && bar);
        QVERIFY(bar->property("closeAfterSave").toBool()); // L5-C11 : la fermeture attend, dans la barre
        QVERIFY(QMetaObject::invokeMethod(saveDialog, "reject"));
        QTRY_VERIFY(!bar->property("closeAfterSave").toBool());
        QObject* actions = namedObject(s, "fileActions");
        QVERIFY(actions);
        QVERIFY(isNullAction(actions->property("afterSave")));
        // Un enregistrement plus tard ne doit pas fermer le document (l'action abandonnée est oubliée).
        QTemporaryDir dir;
        QSignalSpy saved(doc, &ui::DocumentController::saveFinished);
        QVERIFY(run(s, QStringLiteral("file.save-as"), dir.filePath(QStringLiteral("plus-tard.actcraft"))));
        QVERIFY(saved.wait(10000));
        QCOMPARE(doc->pageCount(), 2);
    }

    void titleShowsTheModifiedMark()
    {
        Shell s;
        QVERIFY(start(s));
        QVERIFY(s.window->title().startsWith(QStringLiteral("Sans titre — ActCraft — rendu : ")));
        QVERIFY(run(s, QStringLiteral("page.next")));
        QTRY_VERIFY(s.window->title().startsWith(QStringLiteral("*Sans titre")));
    }
};

QTEST_MAIN(TstUiFileMenu)
#include "tst_ui_filemenu.moc"
