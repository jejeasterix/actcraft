#include <actcraft/ui/commands.h>
#include <actcraft/ui/documentcontroller.h>
#include <actcraft/ui/pageview.h>
#include <actcraft/ui/toolhost.h>

#include <QQmlApplicationEngine>
#include <QQuickWindow>
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
        engine.setInitialProperties(
            {{QStringLiteral("cacheEnabled"), true}, {QStringLiteral("generatedPage"), false}});
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

} // namespace

class TstUiEscape : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;

private slots:
    void initTestCase() { QVERIFY(m_dir.isValid()); }

    void modalDialogClosesOnEscape_data()
    {
        QTest::addColumn<QString>("box");
        QTest::addColumn<QString>("opener"); // commande, ou « open » : open() de la boîte
        QTest::newRow("fond") << "backgroundDialog" << "open";
        QTest::newRow("grille") << "gridDesigner" << "open";
        QTest::newRow("taille") << "pageSizeDialog" << "file.new-custom";
        QTest::newRow("a-propos") << "aboutDialog" << "help.about";
        QTest::newRow("enregistrer") << "tabSaveChangesDialog" << "file.close";
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
};

QTEST_MAIN(TstUiEscape)
#include "tst_ui_escape.moc"
