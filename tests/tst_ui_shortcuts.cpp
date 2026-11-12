#include "startup.h"

#include <actcraft/actions/commandregistry.h>
#include <actcraft/ui/commands.h>

#include <QFile>
#include <QKeySequence>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QtQml/qqmlextensionplugin.h>

#include <utility>
#include <vector>

Q_IMPORT_QML_PLUGIN(ActCraft_UiPlugin)

using namespace actcraft;

namespace {

struct Shell
{
    QTemporaryDir profiles; // profils d'essai, détruit après le moteur
    QQmlApplicationEngine engine;
    QQuickWindow* window = nullptr;
    ui::Commands* commands = nullptr;
};

bool start(Shell& s)
{
    StartupOptions options;
    options.profileRoot = s.profiles.path();
    s.window = loadMainWindow(s.engine, options);
    if (!s.window || !QTest::qWaitForWindowExposed(s.window)) {
        return false;
    }
    s.commands = s.window->findChild<ui::Commands*>(QStringLiteral("commands"));
    return s.commands != nullptr;
}

// Table des raccourcis des fonctions incluses (identifiant → touches, séquences portables de Qt).
// Chaque entrée est une exigence : en changer une est une décision, pas un réglage.
const std::vector<std::pair<const char*, const char*>> kTable = {
    {"file.new", "Ctrl+N"},
    {"file.open", "Ctrl+O"},
    {"file.save", "Ctrl+S"},
    {"file.save-as", "Ctrl+Shift+S"},
    {"file.close", "Ctrl+W"},
    {"file.quit", "Ctrl+Q"},
    {"view.fullscreen", "F5"},
    {"page.previous", "PgUp"},
    {"page.next", "PgDown"},
    {"tool.pen", "Ctrl+Shift+P"},
    {"tool.select", "Esc"},
    {"tool.highlighter", "Ctrl+H"},
    {"tool.eraser", "Ctrl+E"},
    {"edit.undo", "Ctrl+Z"},
    {"edit.redo", "Ctrl+Y"},
    {"edit.delete", "Del"},
    {"edit.select-all", "Ctrl+A"},
    {"object.enlarge", "Ctrl+="},
    {"object.reduce", "Ctrl+-"},
    {"view.page-zoom", "Ctrl+Shift+Z"},
    {"present.curtain", "Ctrl+R"},
    {"present.spotlight-circle", "Ctrl+Shift+O"},
    {"tool.clock", "Ctrl+Shift+U"},
    {"tool.ticker", "Ctrl+Shift+T"},
    {"tool.handwriting", "Ctrl+Shift+H"},
    {"tool.camera-area", "Ctrl+Shift+K"},
    {"tool.keyboard", "Ctrl+K"},
    {"desktop.annotate", "Ctrl+Shift+A"},
    {"desktop.tools", "F6"},
    {"tool.text", "Ctrl+T"},
    {"view.browsers", "Ctrl+B"},
    {"page.insert-after", "Ctrl+I"},
    {"grid.designer", "Ctrl+Shift+G"},
    {"edit.cut", "Ctrl+X"},
    {"edit.copy", "Ctrl+C"},
    {"edit.paste", "Ctrl+V"},
    {"edit.duplicate", "Ctrl+D"},
    {"object.group", "Ctrl+G"},
    {"object.lock", "Ctrl+Shift+L"},
    {"object.hide", "Ctrl+Shift+I"},
    {"object.drag-copy", "Ctrl+Shift+J"},
    {"transform.flip-x", "Ctrl+Shift+X"},
    {"transform.flip-y", "Ctrl+Shift+Y"},
    {"tool.fill", "Ctrl+F"},
    {"tool.connector", "Ctrl+Shift+C"},
    {"insert.media", "Ctrl+M"},
    {"tab.next", "Ctrl+Tab"},
    {"tab.previous", "Ctrl+Shift+Tab"},
    {"export.page-image", "Ctrl+Shift+E"},
    {"file.print", "Ctrl+P"},
    {"mode.design", "F2"},
    {"view.dashboard", "F11"},
    {"spell.check", "F7"},
    {"help.contents", "F1"},
    {"tool.reveal-ink", "Ctrl+Shift+M"},
    {"link.file", "Ctrl+L"},
    {"record.sound", "Ctrl+Shift+R"},
    // L11-T23-PC : touches de F-950 à F-952 relevées au PC, pour les commandes qui existent. Sans commande :
    // F3, F8, Ctrl+U, Ctrl+Maj+Q ; F12 (site du projet) reste libre (tst_actions_layout, lot 6). Ctrl+Q et
    // Ctrl+Maj+S restent à quitter et à enregistrer sous (usage du système), pas à la question et aux formes.
    {"users.dual", "F4"},
    {"browser.properties", "F9"},
    {"browser.actions", "F10"},
    {"profile.edit", "Ctrl+J"},
    {"arrange.back", "Ctrl+Shift+B"},
    {"tool.camera-screen", "Ctrl+Shift+D"},
    {"arrange.front", "Ctrl+Shift+F"},
    {"browser.notes", "Ctrl+Shift+N"},
    {"arrange.forward", "Ctrl+Shift+V"},
    {"arrange.backward", "Ctrl+Shift+W"},
};

// Écrit en français dans la page d'aide : « Maj », « Échap », « Suppr ».
QString portable(QString text)
{
    text.replace(QStringLiteral("Maj"), QStringLiteral("Shift"))
        .replace(QStringLiteral("Échap"), QStringLiteral("Esc"))
        .replace(QStringLiteral("Suppr"), QStringLiteral("Del"));
    return QKeySequence(text, QKeySequence::PortableText).toString(QKeySequence::PortableText);
}

} // namespace

class TstUiShortcuts : public QObject
{
    Q_OBJECT
    QTemporaryDir m_settingsDir;

private slots:
    void initTestCase()
    {
        QVERIFY(m_settingsDir.isValid());
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, m_settingsDir.path());
        QSettings probe;
        QCOMPARE(probe.format(), QSettings::IniFormat);
        QVERIFY(probe.fileName().startsWith(m_settingsDir.path()));
    }

    // La table du test et commands.json se recouvrent exactement : rien d'oublié, rien de surnuméraire.
    void everyListedShortcutDeclared()
    {
        actions::CommandRegistry registry;
        QString error;
        QVERIFY2(registry.loadDeclarations(actions::defaultCommandsJson(), error), qPrintable(error));
        QSet<QString> listed;
        for (const auto& [id, keys] : kTable) {
            const actions::CommandInfo* info = registry.find(QString::fromLatin1(id));
            QVERIFY2(info, id);
            QCOMPARE(info->shortcut, QString::fromLatin1(keys));
            QVERIFY2(!QKeySequence(info->shortcut, QKeySequence::PortableText).isEmpty(), id);
            listed.insert(QString::fromLatin1(id));
        }
        for (const QString& id : registry.ids()) {
            const QString shortcut = registry.find(id)->shortcut;
            QVERIFY2(
                shortcut.isEmpty() || listed.contains(id),
                qPrintable(id + QStringLiteral(" : ") + shortcut + QStringLiteral(" absent de la table")));
            // Aucune touche réservée au système.
            QVERIFY2(!shortcut.startsWith(QStringLiteral("Alt+F4")) &&
                         !shortcut.startsWith(QStringLiteral("Meta")),
                     qPrintable(id));
        }
    }

    // Chaque raccourci de la table, joué sur la fenêtre, déclenche sa commande et elle seule.
    void shortcutTriggersItsCommand()
    {
        Shell s;
        QVERIFY(start(s));
        QStringList fired;
        for (const auto& [id, keys] : kTable) {
            const QString command = QString::fromLatin1(id);
            if (!s.commands->isAvailable(command)) {
                continue; // commande propre à une autre plateforme
            }
            // Gestionnaire d'essai : aucun effet de bord (quitter, imprimer, boîtes) pendant le parcours.
            s.commands->setNativeHandler(command, [&fired, command](const QVariant&) { fired << command; });
            s.commands->setEnabled(command, true);
        }
        int played = 0;
        for (const auto& [id, keys] : kTable) {
            const QString command = QString::fromLatin1(id);
            if (!s.commands->isAvailable(command)) {
                continue;
            }
            fired.clear();
            QTest::keySequence(s.window, QKeySequence(QString::fromLatin1(keys), QKeySequence::PortableText));
            QVERIFY2(QTest::qWaitFor([&] { return !fired.isEmpty(); }, 500), id);
            QCOMPARE(fired, QStringList{command});
            ++played;
        }
        QVERIFY(played > 40);
    }

    // Une touche seule (Suppr) appartient au champ de texte quand il a le focus : l'objet reste.
    void noSingleKeyShortcutWhileTyping()
    {
        Shell s;
        QVERIFY(start(s));
        int deletes = 0;
        s.commands->setNativeHandler(QStringLiteral("edit.delete"),
                                     [&deletes](const QVariant&) { ++deletes; });
        s.commands->setEnabled(QStringLiteral("edit.delete"), true);

        QQmlComponent component(&s.engine);
        component.setData(
            "import QtQuick\nTextInput { width: 200; height: 30; text: \"abc\"; cursorPosition: 0 }", QUrl());
        auto* input = qobject_cast<QQuickItem*>(component.create());
        QVERIFY2(input, qPrintable(component.errorString()));
        input->setParentItem(s.window->contentItem());
        input->forceActiveFocus();
        QVERIFY(input->hasActiveFocus());
        input->setProperty("cursorPosition", 0); // le focus place le curseur en fin de texte

        QTest::keyClick(s.window, Qt::Key_Delete);
        QTRY_COMPARE(input->property("text").toString(), QStringLiteral("bc"));
        QCOMPARE(deletes, 0);

        // Contrôle : sans champ de texte actif, la même touche déclenche la commande.
        input->setFocus(false);
        s.window->contentItem()->forceActiveFocus();
        QTest::keyClick(s.window, Qt::Key_Delete);
        QTRY_COMPARE(deletes, 1);
        delete input;
    }

    // La page d'aide liste chaque séquence déclarée d'une commande qui agit (gestionnaire posé) : celles du
    // lot 7, déclarées sans gestionnaire tant qu'il n'est pas fusionné, n'y figurent pas encore.
    void helpPageListsEveryShortcut()
    {
        Shell s;
        QVERIFY(start(s));
        QFile page(QStringLiteral(":/actcraft/help/raccourcis.html"));
        QVERIFY(page.open(QIODevice::ReadOnly));
        const QString html = QString::fromUtf8(page.readAll());
        QSet<QString> onPage;
        QRegularExpression kbd(QStringLiteral("<kbd>([^<]+)</kbd>"));
        for (auto it = kbd.globalMatch(html); it.hasNext();) {
            onPage.insert(portable(it.next().captured(1)));
        }
        QStringList missing;
        int checked = 0;
        for (const auto& [id, keys] : kTable) {
            if (!s.commands->registry().hasHandler(QString::fromLatin1(id))) {
                continue;
            }
            ++checked;
            const QString wanted = QKeySequence(QString::fromLatin1(keys), QKeySequence::PortableText)
                                       .toString(QKeySequence::PortableText);
            const QString command = QString::fromLatin1(id);
            if (!onPage.contains(wanted)) {
                missing << command + QStringLiteral(" ") + wanted;
            }
        }
        QVERIFY2(missing.isEmpty(), qPrintable(missing.join(QStringLiteral(", "))));
        qInfo("RACCOURCIS CONTROLES %d", checked);
        QVERIFY2(checked > 40, qPrintable(QStringLiteral("%1 séquences contrôlées").arg(checked)));
    }
};

QTEST_MAIN(TstUiShortcuts)
#include "tst_ui_shortcuts.moc"
