#include "startup.h"
#include "support/flipchartwriter.h"

#include <actcraft/core/document.h>
#include <actcraft/formats/nativefile.h>
#include <actcraft/render/fixedpageitem.h>
#include <actcraft/render/inkitem.h>
#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documentio.h>
#include <actcraft/ui/pageview.h>

#include <QFile>
#include <QImage>
#include <QJSValue>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScopeGuard>
#include <QScreen>
#include <QSemaphore>
#include <QSignalSpy>
#include <QStringList>
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
        QObject* dialog = namedObject(s, "saveChangesDialog");
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
        QVERIFY(run(s, QStringLiteral("file.new")));
        QObject* dialog = namedObject(s, "saveChangesDialog");
        QVERIFY(dialog);
        QTRY_VERIFY(dialog->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(dialog, "cancel"));
        QTRY_VERIFY(!dialog->property("visible").toBool());
        QCOMPARE(doc->pageCount(), 2);
        QVERIFY(doc->isModified());
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

    // L2-C9, constat 5 : « Quitter » pendant un enregistrement, puis « Enregistrer » : l'enregistrement
    // demandé attend la fin de celui en cours, puis « Quitter » reprend. Avant : refusé (occupé) sans
    // erreur, et l'action en attente oubliée.
    void quitDuringASaveWaitsForItThenQuits()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QTemporaryDir dir;
        QSignalSpy saved(doc, &ui::DocumentController::saveFinished);
        QVERIFY(run(s, QStringLiteral("file.save-as"), dir.filePath(QStringLiteral("cours.actcraft"))));
        QVERIFY(saved.wait(10000));
        QVERIFY(!doc->isModified());
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(doc->isModified());
        // Fils du pool global tenus : l'enregistrement lancé reste en cours tant que le test ne les rend pas.
        const int threads = QThreadPool::globalInstance()->maxThreadCount();
        auto started = std::make_shared<QSemaphore>();
        auto gate = std::make_shared<QSemaphore>();
        for (int i = 0; i < threads; ++i) {
            QThreadPool::globalInstance()->start([started, gate] {
                started->release();
                gate->acquire();
            });
        }
        const auto releasePool = qScopeGuard([gate, threads] {
            gate->release(threads);
            QThreadPool::globalInstance()->waitForDone(60000);
        });
        QVERIFY(started->tryAcquire(threads, 60000));
        QVERIFY(run(s, QStringLiteral("file.save")));
        QVERIFY(doc->isBusy());
        QVERIFY(run(s, QStringLiteral("file.quit")));
        QObject* ask = namedObject(s, "saveChangesDialog");
        QVERIFY(ask);
        QTRY_VERIFY(ask->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(ask, "save"));
        QObject* actions = namedObject(s, "fileActions");
        QVERIFY(actions);
        QTest::qWait(50);                                 // un refus serait livré ici, en file
        QVERIFY(!actions->property("quitting").toBool()); // pas avant la fin des enregistrements
        gate->release(threads);
        QTRY_VERIFY_WITH_TIMEOUT(actions->property("quitting").toBool(), 20000);
        QVERIFY(!doc->isModified()); // le second enregistrement a écrit l'écran
        QVERIFY(doc->lastError().isEmpty());
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

    // Même chose par « Quitter » puis « Enregistrer » pendant l'ouverture (modifications abandonnées à
    // l'ouverture) : l'action en attente est réévaluée sur le document ouvert, propre : on quitte sans
    // écrire.
    void quitThenSaveDuringASuccessfulOpenQuitsWithoutWriting()
    {
        Shell s;
        QVERIFY(start(s));
        auto* doc = controller(s);
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("deux.actcraft"));
        QVERIFY(writeTwoPages(path));
        const QByteArray before = fileBytes(path);
        QVERIFY(run(s, QStringLiteral("page.next")));
        QVERIFY(doc->isModified());
        QSignalSpy saved(doc, &ui::DocumentController::saveFinished);
        QSignalSpy loaded(doc, &ui::DocumentController::loadFinished);
        QObject* ask = namedObject(s, "saveChangesDialog");
        QObject* actions = namedObject(s, "fileActions");
        QVERIFY(ask && actions);
        PoolHold pool;
        QVERIFY(pool.held());
        QVERIFY(run(s, QStringLiteral("file.open"), path));
        QTRY_VERIFY(ask->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(ask, "discard")); // ouvrir sans enregistrer
        QTRY_VERIFY(!ask->property("visible").toBool());
        QVERIFY(doc->isBusy());
        QVERIFY(run(s, QStringLiteral("file.quit")));
        QTRY_VERIFY(ask->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(ask, "save"));
        pool.release();
        QVERIFY(loaded.wait(20000));
        QCOMPARE(loaded.first().at(0).toBool(), true);
        QTRY_VERIFY_WITH_TIMEOUT(actions->property("quitting").toBool(), 20000);
        QTest::qWait(300);
        QCOMPARE(saved.count(), 0);
        QCOMPARE(fileBytes(path), before);
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
        QObject* ask = namedObject(s, "saveChangesDialog");
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
        QObject* ask = namedObject(s, "saveChangesDialog");
        QVERIFY(ask);
        QTRY_VERIFY(ask->property("visible").toBool());
        QVERIFY(QMetaObject::invokeMethod(ask, "save")); // document sans fichier : « Enregistrer sous »
        QObject* saveDialog = namedObject(s, "saveDialog");
        QVERIFY(saveDialog);
        QObject* actions = namedObject(s, "fileActions");
        QVERIFY(actions);
        QVERIFY(!isNullAction(actions->property("afterSave")));
        QVERIFY(QMetaObject::invokeMethod(saveDialog, "reject"));
        QTRY_VERIFY(isNullAction(actions->property("afterSave")));
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
