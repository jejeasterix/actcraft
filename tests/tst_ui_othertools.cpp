#include "presentationcommands.h"
#include "support/lot7registry.h"
#include "support/presentationharness.h"

#include <actcraft/core/model.h>
#include <actcraft/ui/documentcontroller.h>

#include <QSettings>
#include <QStandardPaths>
#include <QtTest>

#include <variant>

namespace {

using actcraft::ui::DocumentController;
using actcraft::ui::PresentationCommands;
using presentationharness::MainWindow;
namespace core = actcraft::core;

DocumentController* controllerOf(const MainWindow& main)
{
    return main.window() ? main.window()->findChild<DocumentController*>(QStringLiteral("documentController"))
                         : nullptr;
}

PresentationCommands* commandsOf(const MainWindow& main)
{
    return main.window()
               ? main.window()->findChild<PresentationCommands*>(QStringLiteral("presentationCommands"))
               : nullptr;
}

const core::Page& shown(const DocumentController& controller)
{
    return controller.document().page(static_cast<std::size_t>(controller.currentIndex()));
}

// Tout le fichier dans cet espace de noms anonyme (exécutable unique : aucun nom global hors du registre).
class TstUiOtherTools : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("ActCraftTest"));
        QCoreApplication::setApplicationName(QStringLiteral("tst_ui_othertools"));
    }

    // F-300 : horloge injectée au 2026-10-18 14:05, un seul texte, une seule étape d'annulation.
    void dateTimeInsertsOneText()
    {
        MainWindow main;
        DocumentController* controller = controllerOf(main);
        PresentationCommands* commands = commandsOf(main);
        QVERIFY(controller != nullptr);
        QVERIFY(commands != nullptr);
        commands->setNowOverride(QDateTime(QDate(2026, 10, 18), QTime(14, 5)));

        const std::size_t before = shown(*controller).objects.size();
        QVERIFY(main.run(QStringLiteral("insert.date-time")));
        QCOMPARE(shown(*controller).objects.size(), before + 1);
        const core::PageObject& written = shown(*controller).objects.back();
        const auto* text = std::get_if<core::Text>(&core::body(written));
        QVERIFY(text != nullptr);
        const QString html = QString::fromStdString(text->html);
        QVERIFY2(html.contains(QStringLiteral("18 octobre 2026")), qPrintable(html));
        QVERIFY2(html.contains(QStringLiteral("14:05")), qPrintable(html));
        QVERIFY(written.common.layer == core::Layer::Middle);

        controller->undo();
        QCOMPARE(shown(*controller).objects.size(), before);
        QVERIFY(!controller->canUndo()); // une seule étape
    }

    // F-299 : l'adresse d'accueil par défaut est la page du projet ; le réglage la remplace.
    void webBrowserOpensTheHomeUrl()
    {
        MainWindow main;
        PresentationCommands* commands = commandsOf(main);
        QVERIFY(commands != nullptr);
        QList<QUrl> opened;
        commands->setUrlOpener([&opened](const QUrl& url) {
            opened.append(url);
            return true;
        });

        QVERIFY(main.run(QStringLiteral("tool.web-browser")));
        QCOMPARE(opened.size(), 1);
        QCOMPARE(opened.back(), QUrl(QStringLiteral("https://github.com/jejeasterix/actcraft")));
        QVERIFY(commands->message().isEmpty());

        QSettings().setValue(QStringLiteral("tools/webHome"), QStringLiteral("http://exemple.test/accueil"));
        QVERIFY(main.run(QStringLiteral("tool.web-browser")));
        QCOMPARE(opened.size(), 2);
        QCOMPARE(opened.back(), QUrl(QStringLiteral("http://exemple.test/accueil")));
    }

    // Tout autre schéma est refusé avec un message, l'ouvreur n'est jamais appelé.
    void webBrowserRefusesOtherSchemes()
    {
        MainWindow main;
        PresentationCommands* commands = commandsOf(main);
        QVERIFY(commands != nullptr);
        int calls = 0;
        commands->setUrlOpener([&calls](const QUrl&) {
            ++calls;
            return true;
        });
        const QStringList refused{QStringLiteral("file:///C:/Windows/notepad.exe"),
                                  QStringLiteral("javascript:alert(1)"),
                                  QStringLiteral("//serveur/partage"),
                                  QStringLiteral("ftp://exemple.test/"),
                                  QStringLiteral("C:/Windows/notepad.exe"),
                                  QStringLiteral("exemple.test")};
        for (const QString& address : refused) {
            QSettings().setValue(QStringLiteral("tools/webHome"), address);
            QVERIFY2(!commands->openWebHome(), qPrintable(address));
            QVERIFY2(!commands->message().isEmpty(), qPrintable(address));
        }
        QCOMPARE(calls, 0);
        // Une adresse valide efface le message.
        QSettings().setValue(QStringLiteral("tools/webHome"), QStringLiteral("https://exemple.test/"));
        QVERIFY(commands->openWebHome());
        QVERIFY(commands->message().isEmpty());
        QCOMPARE(calls, 1);
    }
};

} // namespace

ACTCRAFT_LOT7_TEST("othertools", TstUiOtherTools)
#include "tst_ui_othertools.moc"
