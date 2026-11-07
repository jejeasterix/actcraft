#include "startup.h"

#include <actcraft/core/edits.h>
#include <actcraft/core/model.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/documentio.h>
#include <actcraft/ui/lifecycle.h>
#include <actcraft/ui/pageview.h>

#include <QCoreApplication>
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
};

QTEST_MAIN(TstUiLifecycle)
#include "tst_ui_lifecycle.moc"
