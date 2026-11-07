#include "startup.h"

#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documentio.h>
#include <actcraft/ui/documenttabs.h>
#include <actcraft/ui/lifecycle.h>
#include <actcraft/ui/pageview.h>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointingDevice>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSaveFile>
#include <QScopeGuard>
#include <QSemaphore>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QStringList>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QUrl>
#include <QtGui/qevent.h>
#include <QtQml/qqmlextensionplugin.h>

#include <atomic>
#include <memory>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

void addStroke(ui::DocumentController& document, double x, double y)
{
    core::Stroke stroke;
    stroke.points = {{x, y, 0.5f}, {x + 100, y + 60, 0.5f}, {x + 200, y + 20, 0.5f}};
    core::PageObject object = stroke;
    object.common.layer = core::Layer::Middle;
    document.execute(std::make_unique<core::AddObjectCommand>(0, std::move(object)));
}

// Mise en arrière-plan simulée par la voie publique (A22) : l'événement envoyé à qApp, comme l'étape
// « appState » du joueur. Il n'émet pas QGuiApplication::applicationStateChanged : une reprise qui
// n'écouterait que ce signal resterait sourde ici.
void suspend()
{
    QApplicationStateChangeEvent event(Qt::ApplicationSuspended);
    QCoreApplication::sendEvent(qApp, &event);
}

// Document et reprise montés sur un dossier temporaire : jamais le vrai dossier de reprise.
struct Fixture
{
    QTemporaryDir dir;
    ui::DocumentController document;
    ui::LifecycleKeeper keeper;
    QString recovery;

    Fixture() : recovery(dir.filePath(QStringLiteral("reprise")))
    {
        document.setDocumentsRoot(dir.filePath(QStringLiteral("documents")));
        document.newDocument(1024, 768);
        keeper.setRecoveryDir(recovery);
        keeper.setDocument(&document);
        keeper.start();
    }
    QString copyPath() const { return recovery + QStringLiteral("/reprise.actcraft"); }
    QString infoPath() const { return recovery + QStringLiteral("/reprise.json"); }
};

// Mise en arrière-plan d'un document modifié ; vrai si la copie est écrite.
bool writeCopy(ui::LifecycleKeeper& keeper)
{
    QSignalSpy finished(&keeper, &ui::LifecycleKeeper::writeFinished);
    suspend();
    if (finished.count() == 0 && !finished.wait(5000))
        return false;
    return finished.count() == 1 && finished.at(0).at(0).toBool() && keeper.lastWriteOk();
}

// Enregistrement attendu jusqu'à sa fin ; vrai s'il a réussi.
bool saveAndWait(ui::DocumentController& document, const QUrl& url)
{
    QSignalSpy saved(&document, &ui::DocumentController::saveFinished);
    document.saveToUrl(url);
    if (saved.count() == 0 && !saved.wait(5000))
        return false;
    return saved.count() == 1 && saved.at(0).at(0).toBool();
}

// Copie de reprise laissée dans `recovery` par un processus « tué » : document modifié, enregistré d'abord
// sous `original` s'il est donné ; rend l'empreinte du document copié.
QString leaveCopy(const QString& recovery, const QUrl& original = QUrl())
{
    ui::DocumentController document;
    document.newDocument(1024, 768);
    addStroke(document, 100, 100);
    if (!original.isEmpty() && !saveAndWait(document, original))
        return {};
    ui::LifecycleKeeper keeper;
    keeper.setRecoveryDir(recovery);
    keeper.setDocument(&document);
    keeper.start();
    addStroke(document, 300, 300);
    if (!writeCopy(keeper))
        return {};
    return document.fingerprintText();
}

// Fil sur lequel la ligne « ACTCRAFT COPIE DE REPRISE » a été journalisée.
std::atomic<QThread*> g_copyLogThread{nullptr};
QtMessageHandler g_previousHandler = nullptr;
void copyLogWatcher(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    if (message.contains(QStringLiteral("ACTCRAFT COPIE DE REPRISE")))
        g_copyLogThread.store(QThread::currentThread());
    if (g_previousHandler != nullptr)
        g_previousHandler(type, context, message);
}

// Main.qml chargé sur des dossiers d'essai (profils, reprise) ; `open` : fichier ouvert au lancement.
struct Window
{
    QTemporaryDir profiles;
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::DocumentController* document = nullptr;
    ui::LifecycleKeeper* keeper = nullptr;
    ui::DocumentTabs* tabs = nullptr;
};

bool load(Window& w, const QString& recovery, const QUrl& open = QUrl())
{
    StartupOptions options;
    options.cacheEnabled = false;
    options.generatedPage = false;
    options.profileRoot = w.profiles.path();
    options.recoveryDir = recovery;
    options.openUrl = open;
    w.window = loadMainWindow(w.engine, options);
    if (w.window == nullptr || !QTest::qWaitForWindowExposed(w.window))
        return false;
    w.document = w.window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
    w.keeper = w.window->findChild<ui::LifecycleKeeper*>(QStringLiteral("appLifecycle"));
    w.tabs = w.window->findChild<ui::DocumentTabs*>(QStringLiteral("appTabs"));
    return w.document != nullptr && w.keeper != nullptr && w.tabs != nullptr;
}

} // namespace

class TstUiLifecycle : public QObject
{
    Q_OBJECT
    QTemporaryDir m_settings;
    std::unique_ptr<QPointingDevice> m_stylus; // détruit après chaque moteur QML (Qt Quick le garde)

    // Livré tout de suite à la fenêtre (le filtre de la vue capte le stylet), comme tst_ui_pageinput.
    void tablet(QQuickWindow* window, ui::PageView* view, QEvent::Type type, QPointF page)
    {
        const bool press = type == QEvent::TabletPress;
        const QPointF scene = view->pageToScene(page);
        QTabletEvent event(type, m_stylus.get(), scene, window->mapToGlobal(scene), 0.7, 0, 0, 0, 0, 0,
                           Qt::NoModifier, press ? Qt::LeftButton : Qt::NoButton, Qt::LeftButton);
        QCoreApplication::sendEvent(window, &event);
    }

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settings.path());
        m_stylus = std::make_unique<QPointingDevice>(
            QStringLiteral("tst-lifecycle-stylus"), 1311, QInputDevice::DeviceType::Stylus,
            QPointingDevice::PointerType::Pen,
            QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3, QString(),
            QPointingDeviceUniqueId::fromNumericId(31));
        QSettings probe;
        QCOMPARE(probe.format(), QSettings::IniFormat);
        QVERIFY(probe.fileName().startsWith(m_settings.path()));
    }

    // Document intact : la mise en arrière-plan n'écrit rien, pas même le dossier ; le démarrage non plus.
    // Dossier par défaut : <AppDataLocation>/reprise (le dossier privé de l'application).
    void nothingWrittenWhenClean()
    {
        ui::LifecycleKeeper fresh;
        QCOMPARE(fresh.recoveryDir(), QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                                          QStringLiteral("/reprise"));

        Fixture f;
        QVERIFY(!f.document.isModified());
        QSignalSpy finished(&f.keeper, &ui::LifecycleKeeper::writeFinished);
        suspend();
        QTest::qWait(200);
        QCOMPARE(finished.count(), 0);
        QVERIFY(!QFileInfo::exists(f.recovery));
        QVERIFY(!f.keeper.recovered());
    }

    // Document modifié, état Suspended : reprise.actcraft écrit par l'écrivain natif (relu par le lecteur
    // natif, même empreinte que le modèle) et reprise.json, qui ne porte que l'URL, la page, l'échelle,
    // l'heure et l'empreinte (jamais un second format du document).
    void suspendWritesRecovery()
    {
        Fixture f;
        addStroke(f.document, 100, 100);
        QVERIFY(f.document.isModified());
        QVERIFY(writeCopy(f.keeper));
        QVERIFY(QFileInfo::exists(f.copyPath()));
        QVERIFY(QFileInfo::exists(f.infoPath()));

        QFile info(f.infoPath());
        QVERIFY(info.open(QIODevice::ReadOnly));
        const QJsonObject o = QJsonDocument::fromJson(info.readAll()).object();
        QCOMPARE(o.keys(),
                 QStringList({QStringLiteral("fingerprint"), QStringLiteral("page"), QStringLiteral("scale"),
                              QStringLiteral("time"), QStringLiteral("url")}));
        QCOMPARE(o.value(QStringLiteral("fingerprint")).toString(), f.document.fingerprintText());
        QCOMPARE(o.value(QStringLiteral("page")).toInt(-1), 0);
        QVERIFY(o.value(QStringLiteral("scale")).toDouble() > 0);
        QVERIFY(o.value(QStringLiteral("time")).toDouble() > 0);

        ui::DocumentController reader;
        QSignalSpy loaded(&reader, &ui::DocumentController::loadFinished);
        reader.openUrl(QUrl::fromLocalFile(f.copyPath()));
        QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, 5000);
        QVERIFY2(loaded.at(0).at(0).toBool(), qPrintable(loaded.at(0).at(1).toString()));
        // Lu comme un .actcraft (un document d'un autre format s'ouvrirait sans fichier, décision 18).
        QCOMPARE(reader.fileUrl(), QUrl::fromLocalFile(f.copyPath()));
        QCOMPARE(reader.fingerprintText(), f.document.fingerprintText());
    }

    // L'écriture se fait sur un autre fil que le fil principal, qui ne l'attend jamais : l'écrivain d'essai
    // ne finit que quand le fil principal le libère, après le retour de la mise en arrière-plan.
    void writeIsOffTheMainThread()
    {
        std::atomic<QThread*> writer{nullptr};
        QSemaphore gate;
        Fixture f;
        ui::setArchiveWriterForTests([&writer, &gate](const QUrl& url, const QByteArray& archive,
                                                      QString* error) {
            writer.store(QThread::currentThread());
            gate.tryAcquire(1, 5000);
            QSaveFile file(url.toLocalFile());
            if (!file.open(QIODevice::WriteOnly) || file.write(archive) != archive.size() || !file.commit()) {
                if (error != nullptr)
                    *error = file.errorString();
                return false;
            }
            return true;
        });
        const auto reset = qScopeGuard([&gate] {
            gate.release();
            ui::setArchiveWriterForTests({});
        });

        addStroke(f.document, 100, 100);
        QSignalSpy finished(&f.keeper, &ui::LifecycleKeeper::writeFinished);
        QElapsedTimer clock;
        clock.start();
        suspend();
        QVERIFY2(clock.elapsed() < 1000, "la mise en arrière-plan a attendu l'écriture");
        QCOMPARE(finished.count(), 0);
        gate.release();
        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 5000);
        QVERIFY(finished.at(0).at(0).toBool());
        QVERIFY(writer.load() != nullptr);
        QVERIFY(writer.load() != QThread::currentThread());
        QVERIFY(writer.load() != qApp->thread());
    }

    // Contact de stylet en cours à la mise en arrière-plan (Main.qml chargé, dossier de reprise d'essai posé
    // par StartupOptions::recoveryDir) : il est terminé, le trait est dans la copie et la vue n'a plus de
    // contact.
    void strokeInProgressIsKept()
    {
        QTemporaryDir profiles;
        QTemporaryDir dir;
        StartupOptions options;
        options.cacheEnabled = false;
        options.generatedPage = false;
        options.profileRoot = profiles.path();
        options.recoveryDir = dir.filePath(QStringLiteral("reprise"));
        QQmlApplicationEngine engine;
        QQuickWindow* window = loadMainWindow(engine, options);
        QVERIFY(window != nullptr);
        QVERIFY(QTest::qWaitForWindowExposed(window));
        auto* view = window->findChild<ui::PageView*>(QStringLiteral("pageView"));
        auto* keeper = window->findChild<ui::LifecycleKeeper*>(QStringLiteral("appLifecycle"));
        auto* document = window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        QVERIFY(view && keeper && document);
        QCOMPARE(keeper->recoveryDir(), options.recoveryDir);
        QCOMPARE(keeper->view(), view);
        QVERIFY(QTest::qWaitFor([&] { return view->cacheReady(); }, 20000));
        QVERIFY(!document->isModified());
        const std::size_t before = document->document().page(0).objects.size();

        tablet(window, view, QEvent::TabletPress, QPointF(300, 300));
        tablet(window, view, QEvent::TabletMove, QPointF(320, 305));
        tablet(window, view, QEvent::TabletMove, QPointF(340, 310));
        QVERIFY(view->contactActive());
        QVERIFY(writeCopy(*keeper));
        QVERIFY(!view->contactActive());
        QCOMPARE(document->document().page(0).objects.size(), before + 1);

        ui::DocumentController reader;
        QSignalSpy loaded(&reader, &ui::DocumentController::loadFinished);
        reader.openUrl(QUrl::fromLocalFile(options.recoveryDir + QStringLiteral("/reprise.actcraft")));
        QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, 5000);
        QVERIFY(loaded.at(0).at(0).toBool());
        QCOMPARE(reader.document().page(0).objects.size(), before + 1);
        QCOMPARE(reader.fingerprintText(), document->fingerprintText());
        tablet(window, view, QEvent::TabletRelease, QPointF(340, 310));
    }

    // Processus tué en arrière-plan (ni enregistrement ni fermeture) : au relancement, un nouvel objet sur le
    // même dossier rouvre le document, même empreinte, URL d'origine, état « modifié », bannière.
    void processKilledInBackgroundLosesNothing()
    {
        QTemporaryDir dir;
        const QString recovery = dir.filePath(QStringLiteral("reprise"));
        const QUrl original = QUrl::fromLocalFile(dir.filePath(QStringLiteral("lecon.actcraft")));
        QString fingerprint;
        {
            ui::DocumentController document;
            document.setDocumentsRoot(dir.filePath(QStringLiteral("documents")));
            document.newDocument(1024, 768);
            addStroke(document, 100, 100);
            QSignalSpy saved(&document, &ui::DocumentController::saveFinished);
            document.saveToUrl(original);
            QTRY_COMPARE_WITH_TIMEOUT(saved.count(), 1, 5000);
            QVERIFY2(saved.at(0).at(0).toBool(), qPrintable(saved.at(0).at(1).toString()));
            QVERIFY(!document.isModified());

            ui::LifecycleKeeper keeper;
            keeper.setRecoveryDir(recovery);
            keeper.setDocument(&document);
            keeper.start();
            addStroke(document, 300, 300);
            QVERIFY(document.isModified());
            fingerprint = document.fingerprintText();
            QVERIFY(writeCopy(keeper));
        }

        ui::DocumentController document;
        document.setDocumentsRoot(dir.filePath(QStringLiteral("documents")));
        document.newDocument(1024, 768);
        ui::LifecycleKeeper keeper;
        keeper.setRecoveryDir(recovery);
        keeper.setDocument(&document);
        keeper.start();
        QTRY_VERIFY_WITH_TIMEOUT(keeper.recovered(), 5000);
        QCOMPARE(document.fingerprintText(), fingerprint);
        QCOMPARE(document.fileUrl(), original);
        QVERIFY(document.isModified());
    }

    // Un enregistrement réussi supprime la copie.
    void saveRemovesRecovery()
    {
        Fixture f;
        addStroke(f.document, 100, 100);
        QVERIFY(writeCopy(f.keeper));
        QVERIFY(QFileInfo::exists(f.copyPath()));
        QSignalSpy saved(&f.document, &ui::DocumentController::saveFinished);
        f.document.saveToUrl(QUrl::fromLocalFile(f.dir.filePath(QStringLiteral("lecon.actcraft"))));
        QTRY_COMPARE_WITH_TIMEOUT(saved.count(), 1, 5000);
        QVERIFY(saved.at(0).at(0).toBool());
        QTRY_VERIFY_WITH_TIMEOUT(!QFileInfo::exists(f.copyPath()) && !QFileInfo::exists(f.infoPath()), 5000);
        QVERIFY(!f.keeper.recovered());
    }

    // « Abandonner » (recovery.discard) : copie rouverte puis supprimée, bannière retirée.
    void discardRemovesRecovery()
    {
        QTemporaryDir dir;
        const QString recovery = dir.filePath(QStringLiteral("reprise"));
        {
            ui::DocumentController document;
            document.newDocument(1024, 768);
            ui::LifecycleKeeper keeper;
            keeper.setRecoveryDir(recovery);
            keeper.setDocument(&document);
            keeper.start();
            addStroke(document, 100, 100);
            QVERIFY(writeCopy(keeper));
        }
        ui::DocumentController document;
        document.newDocument(1024, 768);
        ui::LifecycleKeeper keeper;
        keeper.setRecoveryDir(recovery);
        keeper.setDocument(&document);
        keeper.start();
        QTRY_VERIFY_WITH_TIMEOUT(keeper.recovered(), 5000);
        keeper.discard();
        QTRY_VERIFY_WITH_TIMEOUT(!QFileInfo::exists(recovery + QStringLiteral("/reprise.actcraft")) &&
                                     !QFileInfo::exists(recovery + QStringLiteral("/reprise.json")),
                                 5000);
        QVERIFY(!keeper.recovered());
    }

    // Revue B1 : un objet soulevé (texte existant en édition : textediting soulève l'objet) est dans la
    // copie, comme dans un enregistrement (même annonce que saveToUrl).
    void liftedObjectIsInTheCopy()
    {
        Fixture f;
        addStroke(f.document, 100, 100);
        const core::ObjectId id = f.document.document().page(0).objects.back().common.id;
        QCOMPARE(f.document.liftObjects({id}).size(), std::size_t(1));
        QVERIFY(f.document.isLifting());
        QVERIFY(writeCopy(f.keeper));
        ui::DocumentController reader;
        QSignalSpy loaded(&reader, &ui::DocumentController::loadFinished);
        reader.openUrl(QUrl::fromLocalFile(f.copyPath()));
        QTRY_COMPARE_WITH_TIMEOUT(loaded.count(), 1, 5000);
        QVERIFY(loaded.at(0).at(0).toBool());
        QCOMPARE(reader.document().page(0).objects.size(), std::size_t(1));
    }

    // Revue M1 : un enregistrement réussi pendant lequel le document a encore changé laisse le document
    // modifié : la copie reste.
    void saveWhileChangingKeepsRecovery()
    {
        Fixture f;
        addStroke(f.document, 100, 100);
        QVERIFY(writeCopy(f.keeper));
        QSemaphore gate;
        ui::setArchiveWriterForTests([&gate](const QUrl& url, const QByteArray& archive, QString* error) {
            gate.tryAcquire(1, 5000);
            QSaveFile file(url.toLocalFile());
            if (!file.open(QIODevice::WriteOnly) || file.write(archive) != archive.size() || !file.commit()) {
                if (error != nullptr)
                    *error = file.errorString();
                return false;
            }
            return true;
        });
        const auto reset = qScopeGuard([&gate] {
            gate.release();
            ui::setArchiveWriterForTests({});
        });
        QSignalSpy saved(&f.document, &ui::DocumentController::saveFinished);
        f.document.saveToUrl(QUrl::fromLocalFile(f.dir.filePath(QStringLiteral("lecon.actcraft"))));
        addStroke(f.document, 300, 300); // pendant l'écriture
        gate.release();
        QTRY_COMPARE_WITH_TIMEOUT(saved.count(), 1, 5000);
        QVERIFY(saved.at(0).at(0).toBool());
        QVERIFY(f.document.isModified());
        QTest::qWait(200);
        QVERIFY(QFileInfo::exists(f.copyPath()));
    }

    // Revue M2 : des traits faits pendant la relecture de la copie ne sont jamais écrasés par la reprise.
    void strokesDuringRecoveryReadAreKept()
    {
        QTemporaryDir dir;
        const QString recovery = dir.filePath(QStringLiteral("reprise"));
        QVERIFY(!leaveCopy(recovery).isEmpty());
        ui::DocumentController document;
        document.newDocument(1024, 768);
        ui::LifecycleKeeper keeper;
        keeper.setRecoveryDir(recovery);
        keeper.setDocument(&document);
        keeper.start();
        addStroke(document, 500, 500); // avant l'arrivée de la reprise
        const QString mine = document.fingerprintText();
        QTest::qWait(300);
        QCOMPARE(document.fingerprintText(), mine);
        QVERIFY(QFileInfo::exists(recovery + QStringLiteral("/reprise.actcraft")));
    }

    // Revue M3 : une copie gardée (non rouverte) n'est pas effacée par l'enregistrement d'un autre document.
    void keptCopyIsNotRemovedByAnotherSave()
    {
        QTemporaryDir dir;
        const QString recovery = dir.filePath(QStringLiteral("reprise"));
        QVERIFY(!leaveCopy(recovery).isEmpty());
        ui::DocumentController document;
        document.newDocument(1024, 768);
        ui::LifecycleKeeper keeper;
        keeper.setRecoveryDir(recovery);
        keeper.setDocument(&document);
        keeper.start();
        addStroke(document, 500, 500);
        QTest::qWait(300);
        QVERIFY(!keeper.recovered());
        QVERIFY(saveAndWait(document, QUrl::fromLocalFile(dir.filePath(QStringLiteral("autre.actcraft")))));
        QTest::qWait(300);
        QVERIFY(QFileInfo::exists(recovery + QStringLiteral("/reprise.actcraft")));
    }

    // Revue M4 : une copie plus ancienne que le fichier d'origine n'est pas rouverte et elle est effacée ;
    // une copie illisible aussi.
    void staleOrUnreadableCopyIsRemoved()
    {
        QTemporaryDir dir;
        const QString recovery = dir.filePath(QStringLiteral("reprise"));
        const QString copy = recovery + QStringLiteral("/reprise.actcraft");
        const QUrl original = QUrl::fromLocalFile(dir.filePath(QStringLiteral("lecon.actcraft")));
        QVERIFY(!leaveCopy(recovery, original).isEmpty());
        QTest::qWait(50);
        {
            ui::DocumentController other; // enregistré après la copie, sans gardien
            other.newDocument(1024, 768);
            addStroke(other, 700, 100);
            QVERIFY(saveAndWait(other, original));
        }
        {
            ui::DocumentController document;
            document.newDocument(1024, 768);
            const QString blank = document.fingerprintText();
            ui::LifecycleKeeper keeper;
            keeper.setRecoveryDir(recovery);
            keeper.setDocument(&document);
            keeper.start();
            QTRY_VERIFY_WITH_TIMEOUT(!QFileInfo::exists(copy), 5000);
            QVERIFY(!keeper.recovered());
            QCOMPARE(document.fingerprintText(), blank);
        }
        QVERIFY(QDir().mkpath(recovery));
        QFile garbage(copy);
        QVERIFY(garbage.open(QIODevice::WriteOnly));
        garbage.write("pas une archive");
        garbage.close();
        ui::DocumentController document;
        document.newDocument(1024, 768);
        ui::LifecycleKeeper keeper;
        keeper.setRecoveryDir(recovery);
        keeper.setDocument(&document);
        keeper.start();
        QTRY_VERIFY_WITH_TIMEOUT(!QFileInfo::exists(copy), 5000);
        QVERIFY(!keeper.recovered());
    }

    // Revue m1 : reprise.json n'est cru que s'il décrit la copie (même empreinte) ; sinon le document est
    // rouvert sans URL (jamais attaché au fichier d'un autre document).
    void mismatchedInfoOpensWithoutUrl()
    {
        QTemporaryDir dir;
        const QString recovery = dir.filePath(QStringLiteral("reprise"));
        const QUrl original = QUrl::fromLocalFile(dir.filePath(QStringLiteral("lecon.actcraft")));
        QVERIFY(!leaveCopy(recovery, original).isEmpty());
        QFile info(recovery + QStringLiteral("/reprise.json"));
        QVERIFY(info.open(QIODevice::ReadOnly));
        QJsonObject o = QJsonDocument::fromJson(info.readAll()).object();
        info.close();
        o.insert(QStringLiteral("fingerprint"), QStringLiteral("0000000000000000"));
        QVERIFY(info.open(QIODevice::WriteOnly | QIODevice::Truncate));
        info.write(QJsonDocument(o).toJson());
        info.close();
        ui::DocumentController document;
        document.newDocument(1024, 768);
        ui::LifecycleKeeper keeper;
        keeper.setRecoveryDir(recovery);
        keeper.setDocument(&document);
        keeper.start();
        QTRY_VERIFY_WITH_TIMEOUT(keeper.recovered(), 5000);
        QVERIFY(document.fileUrl().isEmpty());
        QVERIFY(document.isModified());
    }

    // Revue (Android : Hidden puis Suspended) : un document inchangé depuis sa copie n'est pas réécrit.
    void unchangedDocumentIsWrittenOnce()
    {
        Fixture f;
        addStroke(f.document, 100, 100);
        QVERIFY(writeCopy(f.keeper));
        QSignalSpy finished(&f.keeper, &ui::LifecycleKeeper::writeFinished);
        QApplicationStateChangeEvent active(Qt::ApplicationActive);
        QCoreApplication::sendEvent(qApp, &active);
        suspend();
        QTest::qWait(300);
        QCOMPARE(finished.count(), 0);
    }

    // Revue : la ligne « ACTCRAFT COPIE DE REPRISE » est journalisée par le fil de l'écriture (sous Android,
    // la boucle principale peut être arrêtée après Suspended : l'essai ne la verrait jamais).
    void copyIsLoggedFromTheWriteThread()
    {
        Fixture f;
        addStroke(f.document, 100, 100);
        g_copyLogThread.store(nullptr);
        g_previousHandler = qInstallMessageHandler(copyLogWatcher);
        const auto restore = qScopeGuard([] { qInstallMessageHandler(g_previousHandler); });
        QVERIFY(writeCopy(f.keeper));
        QVERIFY(g_copyLogThread.load() != nullptr);
        QVERIFY(g_copyLogThread.load() != QThread::currentThread());
    }

    // Point consigné 1 : un onglet garé modifié est copié et rouvert au relancement.
    void parkedModifiedTabIsCopied()
    {
        QTemporaryDir dir;
        const QString recovery = dir.filePath(QStringLiteral("reprise"));
        QString parked;
        {
            Window w;
            QVERIFY(load(w, recovery));
            addStroke(*w.document, 100, 100);
            parked = w.document->fingerprintText();
            w.tabs->newTab();
            QCOMPARE(w.tabs->rowCount(), 2);
            QVERIFY(!w.document->isModified());
            QVERIFY(writeCopy(*w.keeper));
        }
        Window w;
        QVERIFY(load(w, recovery));
        QTRY_VERIFY_WITH_TIMEOUT(w.keeper->recovered(), 5000);
        QCOMPARE(w.document->fingerprintText(), parked);
        QVERIFY(w.document->isModified());
    }

    // Point consigné 2 : « Quitter » accepté (tout enregistré ou « Ne pas enregistrer ») efface la copie :
    // elle n'est pas rouverte au lancement suivant.
    void quitWithoutSavingRemovesRecovery()
    {
        QTemporaryDir dir;
        const QString recovery = dir.filePath(QStringLiteral("reprise"));
        Window w;
        QVERIFY(load(w, recovery));
        addStroke(*w.document, 100, 100);
        QVERIFY(writeCopy(*w.keeper));
        QVERIFY(QFileInfo::exists(recovery + QStringLiteral("/reprise.actcraft")));
        QObject* fileActions = w.window->findChild<QObject*>(QStringLiteral("fileActions"));
        QVERIFY(fileActions != nullptr);
        fileActions->setProperty("quitting", true); // « Ne pas enregistrer » choisi pour chaque onglet
        w.window->close();
        QTRY_VERIFY_WITH_TIMEOUT(!QFileInfo::exists(recovery + QStringLiteral("/reprise.actcraft")), 5000);
    }

    // Point consigné 3 : un fichier ouvert au lancement ne remplace pas le travail repris : chacun a son
    // onglet.
    void openAtStartupKeepsRecoveredWork()
    {
        QTemporaryDir dir;
        const QString recovery = dir.filePath(QStringLiteral("reprise"));
        const QString recovered = leaveCopy(recovery);
        QVERIFY(!recovered.isEmpty());
        const QUrl opened = QUrl::fromLocalFile(dir.filePath(QStringLiteral("ouvert.actcraft")));
        {
            ui::DocumentController other;
            other.newDocument(1024, 768);
            addStroke(other, 600, 600);
            QVERIFY(saveAndWait(other, opened));
        }
        Window w;
        QVERIFY(load(w, recovery, opened));
        QTRY_VERIFY_WITH_TIMEOUT(w.keeper->recovered() && w.tabs->rowCount() == 2 && !w.document->isBusy(),
                                 10000);
        QTest::qWait(200);
        QStringList seen;
        for (int i = 0; i < w.tabs->rowCount(); ++i) {
            w.tabs->activate(i);
            QTRY_COMPARE_WITH_TIMEOUT(w.tabs->currentIndex(), i, 3000);
            seen << (w.document->fileUrl() == opened ? QStringLiteral("ouvert")
                                                     : w.document->fingerprintText());
        }
        QVERIFY2(seen.contains(QStringLiteral("ouvert")) && seen.contains(recovered),
                 qPrintable(seen.join(u' ')));
    }

    // Point consigné 4 : après une reprise, « Rétablir la page » revient au fichier d'origine.
    void revertPageUsesTheOriginal()
    {
        QTemporaryDir dir;
        const QString recovery = dir.filePath(QStringLiteral("reprise"));
        const QUrl original = QUrl::fromLocalFile(dir.filePath(QStringLiteral("lecon.actcraft")));
        QVERIFY(!leaveCopy(recovery, original).isEmpty()); // un trait enregistré, un trait de plus copié
        ui::DocumentController document;
        document.newDocument(1024, 768);
        ui::LifecycleKeeper keeper;
        keeper.setRecoveryDir(recovery);
        keeper.setDocument(&document);
        keeper.start();
        QTRY_VERIFY_WITH_TIMEOUT(keeper.recovered(), 5000);
        QCOMPARE(document.document().page(0).objects.size(), std::size_t(2));
        QVERIFY(document.canRevertPage());
        document.revertPage();
        QCOMPARE(document.document().page(0).objects.size(), std::size_t(1));
    }
};

QTEST_MAIN(TstUiLifecycle)
#include "tst_ui_lifecycle.moc"
