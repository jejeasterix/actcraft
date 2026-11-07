#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/media/displayrecorder.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/recorders.h>
#include <actcraft/ui/toolhost.h>

#include <QDir>
#include <QFileInfo>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QPointingDevice>
#include <QSettings>
#include <QTabletEvent>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

// La fenêtre réelle (Main.qml), réglages dans un dossier jetable.
struct Rig
{
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::ToolHost* tools = nullptr;
    ui::DocumentController* document = nullptr;
    ui::PageView* view = nullptr;

    bool start(const QString& settingsFile)
    {
        // Profils d'essai à côté des réglages des outils (amendement 3, N1) : jamais ceux du poste.
        const QString profileRoot = QFileInfo(settingsFile).dir().filePath(QStringLiteral("profiles"));
        engine.setInitialProperties({{QStringLiteral("cacheEnabled"), true},
                                     {QStringLiteral("generatedPage"), false},
                                     {QStringLiteral("profileRoot"), profileRoot}});
        engine.loadFromModule("ActCraft.Ui", "Main");
        if (engine.rootObjects().isEmpty()) {
            return false;
        }
        window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst());
        if (!window) {
            return false;
        }
        tools = window->findChild<ui::ToolHost*>(QStringLiteral("appTools"));
        if (!tools) {
            return false;
        }
        tools->setSettingsFile(settingsFile); // jamais le profil réel
        document = window->findChild<ui::DocumentController*>(QStringLiteral("documentController"));
        view = window->findChild<ui::PageView*>(QStringLiteral("pageView"));
        return document && view && (window->isVisible() || QTest::qWaitForWindowExposed(window));
    }
    QObject* named(const char* name) const { return window->findChild<QObject*>(QString::fromLatin1(name)); }
    bool run(const QString& id) const
    {
        QVariant result;
        return QMetaObject::invokeMethod(window, "runCommand", Q_RETURN_ARG(QVariant, result),
                                         Q_ARG(QVariant, QVariant(id)), Q_ARG(QVariant, QVariant())) &&
               result.toBool();
    }
};

// Écran simulé : l'enregistreur d'écran est disponible, rien n'est capturé.
class FakeScreen final : public media::DisplayEngine
{
public:
    void start(const QString&) override {}
    void encode(const QVideoFrame&) override {}
    void stop() override {}
};

} // namespace

class TstUiEscape : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        // Réglages du poste (Preferences de Main.qml) : INI dans le dossier jetable, jamais ceux du poste
        // (N1).
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           m_dir.filePath(QStringLiteral("poste")));
    }

    void modalDialogClosesOnEscape_data()
    {
        QTest::addColumn<QString>("box");
        QTest::addColumn<QString>("opener"); // commande, ou « open » : open() de la boîte
        QTest::newRow("fond") << "backgroundDialog" << "open";
        QTest::newRow("grille") << "gridDesigner" << "open";
        QTest::newRow("taille") << "pageSizeDialog" << "file.new-custom";
        QTest::newRow("a-propos") << "aboutDialog" << "help.about";
        QTest::newRow("enregistrer") << "tabSaveChangesDialog" << "file.close";
        QTest::newRow("lien") << "linkConfirmDialog" << "open"; // L8-C7 : popupOpened/popupClosed relayés
        QTest::newRow("diffusion") << "publishDialogBox" << "file.publish";
    }

    // Boîte modale ouverte : Échap la ferme comme « Annuler », sans changer d'outil ni créer d'étape.
    void modalDialogClosesOnEscape()
    {
        QFETCH(QString, box);
        QFETCH(QString, opener);
        Rig r;
        QVERIFY(r.start(m_dir.filePath(QStringLiteral("reglages.ini"))));
        QVERIFY(r.run(QStringLiteral("page.next"))); // document modifié : « Fermer » demande
        r.tools->activate(QStringLiteral("pen"));
        QCOMPARE(r.tools->currentTool(), QStringLiteral("pen"));
        QObject* dialog = r.named(box.toLatin1().constData());
        QVERIFY(dialog);
        if (opener == QLatin1String("open")) {
            QVERIFY(QMetaObject::invokeMethod(dialog, "open"));
        } else {
            QVERIFY(r.run(opener));
        }
        QTRY_VERIFY(dialog->property("visible").toBool());
        QTRY_VERIFY(r.view->inputBlocked());
        // La touche va à la boîte une fois qu'elle a le focus.
        QTRY_VERIFY(dialog->property("activeFocus").toBool());
        const QString before = r.document->fingerprintText();
        const int pages = r.document->pageCount();

        QTest::keyClick(r.window, Qt::Key_Escape);

        QTRY_VERIFY(!dialog->property("visible").toBool());
        QTRY_VERIFY(!r.view->inputBlocked()); // popupClosed émis : le stylet atteint de nouveau la page
        QCOMPARE(r.tools->currentTool(), QStringLiteral("pen"));
        QCOMPARE(r.document->fingerprintText(), before);
        QCOMPARE(r.document->pageCount(), pages);
    }

    // Garde : sans boîte, Échap garde son effet (outil Sélectionner).
    void escapeWithoutDialogSelectsTheSelectTool()
    {
        Rig r;
        QVERIFY(r.start(m_dir.filePath(QStringLiteral("reglages.ini"))));
        r.tools->activate(QStringLiteral("pen"));
        QCOMPARE(r.tools->currentTool(), QStringLiteral("pen"));
        QVERIFY(!r.view->inputBlocked());
        QTest::keyClick(r.window, Qt::Key_Escape);
        QCOMPARE(r.tools->currentTool(), QStringLiteral("select"));
    }
    // L8-C10, essai 2 (revue, majeurs 2 et 3) : le calque de tracé de zone prend Échap (l'outil ne change pas) et
    // l'entrée du stylet (le trait ne s'écrit pas sur la page derrière).
    void areaSelectorKeepsEscapeAndCoversTheStylus()
    {
        struct Reset
        {
            ~Reset() { ui::Recorders::resetDefaultEngines(); }
        } reset;
        ui::Recorders::setDefaultEngines(
            {{}, {}, [] { return std::unique_ptr<media::DisplayEngine>(new FakeScreen); }});
        Rig r;
        QVERIFY(r.start(m_dir.filePath(QStringLiteral("reglages.ini"))));
        auto* recorders = r.window->findChild<ui::Recorders*>();
        QVERIFY(recorders);
        QVERIFY(recorders->screenAvailable());
        r.tools->activate(QStringLiteral("pen"));
        QObject* selector = r.named("areaSelector");
        QVERIFY(selector);
        QMetaObject::invokeMethod(recorders, "toggleScreen", Q_ARG(bool, true));
        QTRY_VERIFY(recorders->isSelectingArea());
        QTRY_VERIFY(selector->property("activeFocus").toBool());

        QPointingDevice stylus(QStringLiteral("tst-ui-escape-stylus"), 5171, QInputDevice::DeviceType::Stylus,
                               QPointingDevice::PointerType::Pen,
                               QInputDevice::Capability::Position | QInputDevice::Capability::Pressure, 1, 3);
        const QString before = r.document->fingerprintText();
        const QPointF from(r.window->width() / 2.0, r.window->height() / 2.0);
        const auto tablet = [&](QEvent::Type type, QPointF at, Qt::MouseButtons buttons) {
            QTabletEvent event(type, &stylus, at, r.window->mapToGlobal(at), buttons ? 0.5 : 0.0, 0, 0, 0, 0, 0,
                               Qt::NoModifier, type == QEvent::TabletMove ? Qt::NoButton : Qt::LeftButton,
                               buttons);
            QCoreApplication::sendEvent(r.window, &event);
        };
        tablet(QEvent::TabletPress, from, Qt::LeftButton);
        for (int i = 1; i <= 4; ++i)
            tablet(QEvent::TabletMove, from + QPointF(20 * i, 10 * i), Qt::LeftButton);
        tablet(QEvent::TabletRelease, from + QPointF(80, 40), Qt::NoButton);
        QTest::qWait(50);
        QCOMPARE(r.document->fingerprintText(), before);

        QTest::keyClick(r.window, Qt::Key_Escape);
        QTRY_VERIFY(!recorders->isSelectingArea());
        QCOMPARE(r.tools->currentTool(), QStringLiteral("pen"));
    }
};

QTEST_MAIN(TstUiEscape)
#include "tst_ui_escape.moc"
